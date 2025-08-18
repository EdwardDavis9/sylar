#include "sylar/scheduler.hh"
#include "sylar/log.hh"
#include "sylar/hook.hh"
#include "sylar/macro.hh"

namespace sylar {
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static thread_local Scheduler *t_scheduler = nullptr;                                  // 标记当前线程所属的调度器

// 当前线程对应的协程对象
static thread_local Fiber *t_fiber = nullptr;//sylar::Fiber::GetThis().get();

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
    : m_name(name)
{
    SYLAR_ASSERT(threads > 0);

    if (use_caller) {
        sylar::Fiber::GetThis();    // 创建一个主协程
        --threads;                  // 当前线程也会被调度，所以 总线程数 = 需要创建的线程数-1

        // 确保当前线程中没有已存在的调度器，防止重复绑定。
        SYLAR_ASSERT(GetThis() == nullptr);

        // 将当前构造的这个 Scheduler 实例绑定到当前线程
        t_scheduler = this;
        // SYLAR_LOG_INFO(g_logger) << this;
        // SYLAR_LOG_INFO(g_logger) <<  t_scheduler;

        // 新建一个主协程
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        sylar::Thread::SetName(m_name);

        // 设置线程对应的主协程
        t_fiber      = m_rootFiber.get();
        m_rootThread = sylar::GetThreadId();
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

Scheduler* Scheduler::GetThis() {

    // if(t_scheduler) {
    //     return t_scheduler->shared_from_this().get();
    // }

    // Scheduler::ptr main_scheduler(new Scheduler);
    // SYLAR_ASSERT(t_scheduler == main_scheduler.get());
    // t_scheduler = main_scheduler.get();
    // return t_scheduler->shared_from_this().get();

    return t_scheduler;
}

Fiber *Scheduler::GetMainFiber() { return t_fiber; }

void Scheduler::start()
{
    {
        MutexType::Lock lock(m_mutex);
        if (!m_stopping) {
            return; // 不允许在处于启动状态时，再次启动
        }
        m_stopping = false; // false 表示未停止，即处于运行状态
        SYLAR_ASSERT(m_threads.empty());

        m_threads.resize(m_threadCount);
        // 线程池的创建
        for (size_t i = 0; i < m_threadCount; ++i) {
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                          m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
    }

    // if (m_rootFiber) {
    //     m_rootFiber->call();
    //     SYLAR_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    // }
}

void Scheduler::stop()
{
    m_autoStop = true;
    if (m_rootFiber
        && m_threadCount == 0
        && (m_rootFiber->getState() == Fiber::TERM
            || m_rootFiber->getState() == Fiber::INIT))
    {
        SYLAR_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if (stopping()) {
            return;
        }
    }

    if (m_rootThread != -1) {
        SYLAR_ASSERT(GetThis() == this);
    }
    else {
        SYLAR_ASSERT(GetThis() != this);
    }
    m_stopping = true;

    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if (m_rootFiber) {
        // tickle();

        // while(!stopping()) {
        //     if(m_rootFiber->getState() == Fiber::TERM
        //        || m_rootFiber->getState() == Fiber::EXCEPT) {
        //         m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this),
        //                                     0, true));
        //         SYLAR_LOG_INFO(g_logger) << " root fiber is term, reset";

        //         t_fiber = m_rootFiber.get();
        //     }
        //     m_rootFiber->call();
        // }

        if(!stopping()) {
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i: thrs) {
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
        t_fiber = Fiber::GetThis().get();
    }

    // 创建了 idle 协程
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));

    // 创建回调的任务协程对象
    Fiber::ptr cb_fiber;

    FiberAndThread ft; // 临时任务对象

    // 调度循环
    while (true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();

            // 遍历任务队列，寻找当前线程可以执行的任务
            while (it != m_fibers.end()) {

                // 任务绑定了特定线程且不是当前线程，跳过，标记需要唤醒其他线程
                if (it->thread != -1 && it->thread != sylar::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);
                // 任务是协程且正在执行中，跳过(避免重复执行)
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                // 找到一个合适的任务，拿出来执行，删除队列里的引用
                ft = *it;
                m_fibers.erase(it);
                ++m_activeThreadCount;
                is_active = true;

                break;
            }
        }

        // 如果发现还有其他线程绑定的任务，唤醒它们
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
                // 若未彻底执行结束，例如 yield  --> 重新加入调度队列
                schedule(ft.fiber);
            }
            else if (ft.fiber->getState() != Fiber::TERM
                     && ft.fiber->getState() != Fiber::EXCEPT)
            {
                ft.fiber->m_state = Fiber::HOLD; // 还没结束或出错 -> 挂起
            }
            ft.reset(); // 清空临时任务对象
        }
        else if (ft.cb) { // 任务是回调函数
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
        else { // 没有任务的话，执行 idle 任务
            if(is_active) {
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

void Scheduler::idle() {
    SYLAR_LOG_INFO(g_logger) << "idle";

    while(!stopping()) {
        sylar::Fiber::YieldToHold();
    }
}

} // namespace sylar
