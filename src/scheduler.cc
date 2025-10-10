#include "sylar/scheduler.hh"
#include "sylar/log.hh"
#include "sylar/hook.hh"
#include "sylar/macro.hh"

namespace sylar {
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static thread_local Scheduler *t_scheduler =
    nullptr; // 标记当前线程所属的调度器

// 当前线程对应的协程对象
static thread_local Fiber *t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
    : m_name(name)
{
    SYLAR_ASSERT(threads > 0);

    if (use_caller) {
        sylar::Fiber::GetThis();
        --threads; // 当前线程也会被调度, 所以 总线程数 = 需要创建的线程数-1

        SYLAR_ASSERT(GetThis() == nullptr);

        t_scheduler = this;

        m_rootFiber.reset(new Fiber([this]() { this->run(); }, 0, true));

        // sylar::Thread::SetName(m_name);

        t_scheduler_fiber = m_rootFiber.get();
        m_rootThread      = sylar::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    }
    else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler()
{
    SYLAR_ASSERT(m_stopping);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler *Scheduler::GetThis()
{

    // if(t_scheduler) {
    //     return t_scheduler->shared_from_this().get();
    // }

    // Scheduler::ptr main_scheduler(new Scheduler);
    // SYLAR_ASSERT(t_scheduler == main_scheduler.get());
    // t_scheduler = main_scheduler.get();
    // return t_scheduler->shared_from_this().get();

    return t_scheduler;
}

Fiber *Scheduler::GetMainFiber() { return t_scheduler_fiber; }

void Scheduler::start()
{
    {
        MutexType::Lock lock(m_mutex);
        if (!m_stopping) { // 默认是停止状态，因此不会去执行
            return;
        }
        m_stopping = false;
        SYLAR_ASSERT(m_threads.empty());

        m_threads.resize(m_threadCount);

        // 线程池的创建, 绑定任务函数
        for (size_t i = 0; i < m_threadCount; ++i) {
            m_threads[i].reset(new Thread(
                [this]() {
                    this->run();
                },
                 "Thread_"));
            m_threadIds.push_back(m_threads[i]->getId());
        }
    }

    // if (m_rootFiber) {
    //     m_rootFiber->call();
    //     // m_rootFiber->swapIn();
    //     SYLAR_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    // }
}

void Scheduler::stop()
{
    m_autoStop = true;
    if (m_rootFiber && m_threadCount == 0
        && (m_rootFiber->getState() == Fiber::TERM
            || m_rootFiber->getState() == Fiber::INIT))
    {
        SYLAR_LOG_INFO(g_logger) << "Schedule stopped";
        m_stopping = true;

        if (stopping()) {
            return;
        }
    }

    // 调度器上下文检查
    if (m_rootThread != -1) {
        // 存在调度器时
        SYLAR_ASSERT(GetThis() == this);
    }
    else {
        // 不存在调度器时, 不能自己调用停止, 防止死锁
        SYLAR_ASSERT(GetThis() != this);
    }
    m_stopping = true;

    // 唤醒 idle 状态下的 worker
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if (m_rootFiber) { // 唤醒 idle 状态的 rootFiber
        tickle();

        // while(!stopping()) {
        //     if(m_rootFiber->getState() == Fiber::TERM
        //        || m_rootFiber->getState() == Fiber::EXCEPT) {
        //         m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this),
        //                                     0, true));
        //         SYLAR_LOG_INFO(g_logger) << " root fiber is term, reset";

        //         t_scheduler_fiber = m_rootFiber.get();
        //     }
        //     m_rootFiber->call();
        // }

        if (!stopping()) {
            m_rootFiber->call();
            // m_rootFiber->swapIn();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for (auto &i : thrs) {
        i->join();
    }
}

void Scheduler::setThis() { t_scheduler = this; }

void Scheduler::run()
{
    SYLAR_LOG_INFO(g_logger) << "run";

    set_hook_enable(true);
    setThis();

    if (sylar::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    // 创建了 idle 协程对象
    // Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr idle_fiber(new Fiber([this](){ this->idle();}));

    // 创建回调的任务协程对象
    Fiber::ptr cb_fiber;

    FiberAndThread ft; // 临时任务对象

    // 调度循环
    while (true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        // 挑选合适的执行者和合适的任务
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();

            // 遍历任务队列, 寻找当前线程可以执行的任务
            while (it != m_fibers.end()) {

                // 任务绑定了特定线程且不是当前线程, 跳过, 标记需要唤醒其他线程
                if (it->thread != -1 && it->thread != sylar::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);
                // 任务是协程且正在执行中, 跳过(避免重复执行)
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                // 找到一个合适的任务, 拿出来执行, 并且将其从任务队列中删除
                ft = *it;
                m_fibers.erase(it);
                ++m_activeThreadCount;
                is_active = true;

                break;
            }
        }

        // 如果发现任务有特定的绑定线程, 那么唤醒一次
        if (tickle_me) {
            tickle();
        }

        // ft 任务处理操作
        if (ft.fiber
            && (ft.fiber->getState() != Fiber::TERM
                && ft.fiber->getState() != Fiber::EXCEPT))
        { // 任务是协程对象
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if (ft.fiber->getState() == Fiber::READY) {
                // 若未彻底执行结束, 例如 yield  --> 重新加入调度队列
                schedule(ft.fiber);
            }
            else if (ft.fiber->getState() != Fiber::TERM
                     && ft.fiber->getState() != Fiber::EXCEPT)
            {
                ft.fiber->m_state = Fiber::HOLD; // 还没结束或出错 -> 挂起
            }
            ft.reset();
        }
        else if (ft.cb) { // 任务是回调函数

            // 创建回调的协程函数对象
            if (cb_fiber) {
                // 复用回调的协程函数对象
                cb_fiber->reset(ft.cb);
            }
            else {
                // 创建回调协程函数对象
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset(); // 清空临时任务对象

            cb_fiber->swapIn();
            --m_activeThreadCount;

            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            }
            else if (cb_fiber->getState() == Fiber::EXCEPT
                     || cb_fiber->getState() == Fiber::TERM)
            {
                cb_fiber->reset(nullptr);
            }
            else {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        }
        else { // 没有绑定任务的话, 执行 idle 任务
            if (is_active) {
                --m_activeThreadCount;
                continue;
            }
            if (idle_fiber->getState() == Fiber::TERM) {
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                break;
                // continue;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if (idle_fiber->getState() != Fiber::TERM
                && idle_fiber->getState() != Fiber::EXCEPT)
            {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

void Scheduler::tickle() { SYLAR_LOG_INFO(g_logger) << "tickle"; }

bool Scheduler::stopping()
{
    MutexType::Lock lock(m_mutex);

    return m_autoStop && m_stopping && m_fibers.empty()
           && m_activeThreadCount == 0;
}

void Scheduler::idle()
{
    SYLAR_LOG_INFO(g_logger) << "idle";

    while (!stopping()) {
        sylar::Fiber::YieldToHold();
    }
}

} // namespace sylar
