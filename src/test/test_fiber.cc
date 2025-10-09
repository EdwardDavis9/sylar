#include "sylar/sylar.hh"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
sylar::Logger::ptr g_test   = SYLAR_LOG_NAME("TEST---");

void run_in_fiber()
{
    std::cout << "2___" << std::endl;
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber begin";
    sylar::Fiber::YieldToHold();
    std::cout << "4___" << std::endl;
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
    sylar::Fiber::YieldToHold();
    std::cout << "6___" << std::endl;
}

void test_fiber()
{
    // sylar::Fiber::GetThis();
    std::cout << "0___" << std::endl;
    sylar::Scheduler sc;

    SYLAR_LOG_INFO(g_logger) << "main begin";

    // 在单个线程中，每当创建一个Fiber时，会随之额外产生两个 Fiber 对象
    // 1. 当创建 Scheduler 时， 在构造函数中会创建一个
    // 2, 当通过 YielHold 时，会创建第二个
    // 总共就出现了三个对象
    sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
    std::cout << "1___" << std::endl;
    fiber->swapIn();
    std::cout << "3___" << std::endl;

    SYLAR_LOG_INFO(g_logger) << "main affter swapIn";
    fiber->swapIn();
    std::cout << "5___" << std::endl;

    SYLAR_LOG_INFO(g_logger) << "main after end";
    fiber->swapIn();
    std::cout << "7___" << std::endl;
}

int main()
{
    sylar::Thread::SetName("main");
    SYLAR_LOG_INFO(g_logger) << "hello from main thread";

    std::vector<sylar::Thread::ptr> thrs;

    for (int i = 0; i < 1; ++i) {
        thrs.push_back(sylar::Thread::ptr(
            new sylar::Thread(&test_fiber, "name_")));
    }

    for (size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }

    // for(auto i : thrs) {
    //     i->join();
    // }
    return 0;
}
