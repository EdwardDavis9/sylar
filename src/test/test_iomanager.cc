#include "sylar/iomanager.hh"
#include "sylar/sylar.hh"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

// // 设置全局的 sock,可能会出现多线程抢占资源，导致向相同的 fd 上添加了相同的事件
// // 不要这样使用
// int sock = 0;

void test_fiber()
{
    int sock = 0;
    SYLAR_LOG_INFO(g_logger) << "test_fiber sock= " << sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(80);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    if (!connect(sock, (const sockaddr *)&addr, sizeof(addr))) {
    }
    else if (errno == EINPROGRESS) {
        SYLAR_LOG_INFO(g_logger)
            << " add event errno=" << errno << " " << strerror(errno);
        sylar::IOManager::GetThis()->addEvent(
            sock, sylar::IOManager::READ,
            []() { SYLAR_LOG_INFO(g_logger) << "read callback"; });

        sylar::IOManager::GetThis()->addEvent(
            sock, sylar::IOManager::WRITE, [sock]() {
            // sock, sylar::IOManager::WRITE, []() {
                SYLAR_LOG_INFO(g_logger) << "write callback";
                sylar::IOManager::GetThis()->cancelEvent(
                    sock, sylar::IOManager::READ);
                close(sock);
            });
    }
    else {
        SYLAR_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }

    SYLAR_LOG_INFO(g_logger) << "end test_fiber";
}

void test1()
{
    std::cout << "epollin = " << EPOLLIN << " epollout = " << EPOLLOUT
              << std::endl;

    sylar::Thread::SetName("main");

    // 单线程情况下, 顺序执行添加任务, 然后在析构中执行任务
    // 任务执行后, 会开始 idle, 然后执行epoll_wait, 等待管道的通知
    // 管道存在消息, 说明有事件, 然后直接 trigerEvent
    // PS: IOManager 中存在一个 fdContext 和fdContext::eventContext
    // , 包含了句柄以及在句柄上添加的任务, 直接在 idle 中 triggerEvent
    // 即可进行特定任务的回调 ,
    // 这样就只需要在使用者里添加任务, 而在框架内部回调函数即可, 实现了逻辑上的分离
    // , 一个只需要添加, 一个只需要执行即可
    // sylar::IOManager iom;

    sylar::IOManager iom(2);
    // sylar::IOManager iom(2, false);

    iom.schedule(&test_fiber);
    iom.schedule(&test_fiber);
}

sylar::Timer::ptr s_timer;

void test_timer()
{
    sylar::IOManager iom(2);
    s_timer = iom.addTimer(
        1000,
        []() {
            static int i = 0;
            SYLAR_LOG_INFO(g_logger) << "hello time i=" << i;

            if (++i == 3) {
                // s_timer->reset(2000, true);
                s_timer->cancel();
            }
        },
        true);
}

int main(int argc, char *argv[])
{

    test1();

    // test_timer();
    return 0;
}
