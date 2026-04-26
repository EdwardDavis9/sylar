#include "sylar/sylar.hh"
#include "sylar/hook.hh"
#include "sylar/iomanager.hh"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_fiber()
{
    sylar::set_hook_enable(false);
    static int s_count = 3;
    SYLAR_LOG_INFO(g_logger) << "test_fiber, s_count " << s_count;

    sleep(1);
    if (--s_count >= 0) {
        sylar::Scheduler::GetCurrentScheduler()->schedule(&test_fiber);

        // sylar::Scheduler::GetCurrentScheduler()->schedule(&test_fiber,
        //                                                   sylar::GetThreadId());
    }
}

int main()
{


#define WAY 1
#if WAY == 1
    sylar::Scheduler sc(1);
    SYLAR_LOG_INFO(g_logger) << "main 1";
#elif WAY == 2
    sylar::Scheduler sc(2);
    SYLAR_LOG_INFO(g_logger) << "main 2";
#elif WAY == 3
    sylar::Scheduler sc(3);
    SYLAR_LOG_INFO(g_logger) << "main 3";
#elif WAY == 4
    sylar::Scheduler sc(3, true, "test-main-thread");
    SYLAR_LOG_INFO(g_logger) << "main 3";
#elif WAY == 5
    sylar::Scheduler sc(3, false, "test-main-thread");
    SYLAR_LOG_INFO(g_logger) << "main 3";
#endif

#define choice 0
#if choice == 0
    sc.schedule(&test_fiber);
    // 这种方式能观察到的现象是创建的4个基本的协程对象自然析构
    // 0 主协程
    // 1 调度协程
    // 2 idle 协程
    // 3 回调函数使用的协程
#elif choice == 1
    sylar::Fiber::ptr job(new sylar::Fiber(&test_fiber));
    sc.schedule(job);
    // 这种方式能观察到的现象是创建的5个基本的协程对象自然析构
    // 0 主协程
    // 1 调度协程
    // 2 外部创建的协程对象
    //
    // 3 idle 协程
    // 4 回调函数使用的协程
    //
    // 其中， 3 和 4 会在调度器的 run 函数运行结束的时候去析构
    // 而 2 1 0 协程会在创建变量时的作用域结束时退出， 在本例中就是 main 结束时析构
    // 因为创建一个外部的智能指针的意思就是让外部来管理生命周期，使用者仅仅使用，不去处理生命周期
    // 因此需要慎重使用这种方式
#elif choice == 2
    sc.schedule(sylar::Fiber::ptr(new sylar::Fiber(&test_fiber)));
    // 更合理的方式是这样在外部创建协程对象
#endif


    // sylar::set_hook_enable(false);

    sc.start();

    // sleep(1);
    SYLAR_LOG_INFO(g_logger) << "ready to schedule------------------------";
    SYLAR_LOG_INFO(g_logger) << "schedule done----------------------------";

    sc.stop();
    SYLAR_LOG_INFO(g_logger) << "over";
    return 0;
}
