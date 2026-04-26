#include "sylar/fiber.hh"
#include "sylar/config.hh"
#include "sylar/macro.hh"
#include <atomic>
#include "sylar/util.hh"

#include "sylar/log.hh"
#include "sylar/scheduler.hh"

namespace sylar {

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");


/**
 * @var t_thread_current_fiber
 * @brief 当前线程下对应的当前的协程对象
 */
static thread_local Fiber *t_thread_current_fiber = nullptr;


/**
 * @var t_thread_main_fiber
 * @brief 线程的主协程
 */
static thread_local Fiber::ptr t_thread_main_fiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>(
    "fiber.stack_size", 1024 * 1024, "fiber stack size");

class MallocStackAllocator {
  public:
    static void *Alloc(size_t size) { return malloc(size); }

    static void Dealloc(void *vp, size_t size) { return free(vp); }
};

using StackAllocator = MallocStackAllocator;

Fiber::Fiber()
{
    // 无参构造的 m_id 只依赖 默认的 m_id, 默认 m_id 为0
    m_state = EXEC;
    m_id    = s_fiber_id++;

    SetThreadCurrentFiber(this);

    if (getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber(root fiber)  id = " << m_id;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize,
             bool return_to_mainFiber)
    : m_id(s_fiber_id++), m_cb(cb)
{
    // 有参构造的 m_id 依赖 ++s_fiber_id
    // sylar::Thread::SetName(sylar::Thread::GetName() + std::to_string(m_id));
    ++s_fiber_count;

    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    SetThreadCurrentFiber(this);

    m_stack = StackAllocator::Alloc(m_stacksize);
    m_state = INIT;
    if (getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link          = nullptr;
    m_ctx.uc_stack.ss_sp   = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    if (return_to_mainFiber) {
        // 执行者是调度器：返回到主协程
        makecontext(&m_ctx, &Fiber::MainFiberFunc, 0);
    }
    else {
        // 执行者是任务协程：返回到调度器
        makecontext(&m_ctx, &Fiber::SchedulerFiberFunc, 0);
    }

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber  id = " << m_id;
}

Fiber::~Fiber()
{
    --s_fiber_count;
    // --s_fiber_id;

    if (m_stack) {
        SYLAR_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);

        StackAllocator::Dealloc(m_stack, m_stacksize);
    }
    else {
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC);

        Fiber *cur = t_thread_current_fiber;
        if (cur == this) {
            SetThreadCurrentFiber(nullptr);
        }
    }
    SetThreadCurrentFiber(this);
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id = " << m_id;
    // std::cout << GetFiberId() << " ;;3;; " << m_id << std::endl;
}

void Fiber::reset(std::function<void()> cb)
{
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
    m_cb = cb;
    if (getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link          = nullptr;     // 设置关联的上下文
    m_ctx.uc_stack.ss_sp   = m_stack;     // 设置使用的栈空间
    m_ctx.uc_stack.ss_size = m_stacksize; // 设置栈空间的大小

    makecontext(&m_ctx, &Fiber::SchedulerFiberFunc, 0); // 创建这个上下文
    m_state = INIT;
}

void Fiber::call()
{
    SetThreadCurrentFiber(this);
    m_state = EXEC;
    if (swapcontext(&t_thread_main_fiber->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

void Fiber::back()
{
    SetThreadCurrentFiber(t_thread_main_fiber.get());
    if (swapcontext(&m_ctx, &t_thread_main_fiber->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

void Fiber::swapIn()
{
    SetThreadCurrentFiber(this);
    SYLAR_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if (swapcontext(&Scheduler::GetThreadMainFiber()->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

void Fiber::swapOut()
{
    SetThreadCurrentFiber(Scheduler::GetThreadMainFiber());
    if (swapcontext(&m_ctx, &Scheduler::GetThreadMainFiber()->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

void Fiber::SetThreadCurrentFiber(Fiber *f) { t_thread_current_fiber = f; }

Fiber::ptr Fiber::GetCurrentFiber()
{
    if (t_thread_current_fiber) {
        return t_thread_current_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    SYLAR_ASSERT(t_thread_current_fiber == main_fiber.get());
    t_thread_main_fiber = main_fiber;
    return t_thread_current_fiber->shared_from_this();
}

void Fiber::YieldToReady()
{
    Fiber::ptr cur = GetCurrentFiber();
    SYLAR_ASSERT(cur->m_state == EXEC);
    cur->m_state = READY;
    cur->swapOut();
}

void Fiber::YieldToHold()
{
    Fiber::ptr cur = GetCurrentFiber();
    SYLAR_ASSERT(cur->m_state == EXEC);
    // cur->m_state = HOLD;
    cur->swapOut();
}

uint64_t Fiber::TotalFibers() { return s_fiber_count; }

void Fiber::SchedulerFiberFunc()
{
    Fiber::ptr cur = GetCurrentFiber();
    SYLAR_ASSERT(cur);

    try {
        cur->m_cb();
        cur->m_cb    = nullptr;
        cur->m_state = TERM;
    } catch (std::exception &e) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except:" << e.what()
                                  << " fiber_id = " << cur->getId() << std::endl
                                  << sylar::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger)
            << "Fiber Except" << ", fiber_id=" << cur->getId() << std::endl
            << sylar::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset(); // 减少一个引用计数
    raw_ptr->swapOut();

    SYLAR_ASSERT2(false,
                  "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

void Fiber::MainFiberFunc()
{
    Fiber::ptr cur = GetCurrentFiber();
    SYLAR_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb    = nullptr;
        cur->m_state = TERM;
    } catch (std::exception &e) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except:" << e.what()
                                  << " fiber_id = " << cur->getId() << std::endl
                                  << sylar::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger)
            << "Fiber Except" << ", fiber_id=" << cur->getId() << std::endl
            << sylar::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();

    SYLAR_ASSERT2(false,
                  "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

uint64_t Fiber::GetFiberId()
{
    if (t_thread_current_fiber) {
        return t_thread_current_fiber->getId();
    }

    return 0;
}

} // namespace sylar
