#include "sylar/log.hh"
#include "sylar/macro.hh"
#include <assert.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_assert()
{
    SYLAR_LOG_INFO(g_logger) << std::endl << sylar::BacktraceToString(10);
    std::cout << std::endl;

#define choice 0
#if choice == 1
    SYLAR_ASSERT(false);
    std::cout << std::endl;
#elif choice == 2
    SYLAR_ASSERT2(0 == 1, "test_assert output message");
#endif
}

void test_another() { test_assert(); }

void fun3()
{
    int i = 0;
    while (++i < 10) {
        SYLAR_LOG_INFO(g_logger) << i << ":xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void error()
{
    // throw std::
}

void test_error()
{
    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 1; ++i) {
        sylar::Thread::ptr thr(new sylar::Thread(&fun3, "name_"));
        sylar::Thread::ptr thr2(new sylar::Thread(&fun3, "name_"));
        thrs.push_back(thr);
        thrs.push_back(thr2);

        // 父线程可能提前退出了，这时子线程还正在执行，因此就会出现错误
    }
}

int main(int argc, char *argv[])
{
    // test_error();
    // // sleep(2); //  主动阻塞父线程，避免父线程提前退出

    // test_assert();
    test_another();
    return 0;
}
