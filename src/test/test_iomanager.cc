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

// BUG: 全局添加相同的 sock,  就可能会出现错误
// 主要是 addEvent 那里出现的问题, 但是找了找调用 addEvent 的地方
// 简单排查之后, 发现涉及到了 hook, timer, 因此为了方便起见, 暂时先搁置这个bug, 先只使用局部的sock
// int sock = 0;


void test_fiber() {

		int sock = 0;
	SYLAR_LOG_INFO(g_logger) << "test_fiber sock= " << sock;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(sock, F_SETFL, O_NONBLOCK);

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

	if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
	} else if(errno == EINPROGRESS ) {
		SYLAR_LOG_INFO(g_logger) << " add event errno="  << errno << " "
			<< strerror(errno);
		sylar::IOManager::GetThis()->
			addEvent(sock, sylar::IOManager::READ,
					 []() {SYLAR_LOG_INFO(g_logger) << "read callback";});

		sylar::IOManager::GetThis()
			->addEvent(sock, sylar::IOManager::WRITE,
					   [sock](){SYLAR_LOG_INFO(g_logger) << "write callback";
						   sylar::IOManager::GetThis()
							   ->cancelEvent(sock, sylar::IOManager::READ);
							   close(sock);
					   });
	} else {
		SYLAR_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
	}
}

void test1() {
	std::cout << "epollin = " << EPOLLIN
		<< " epollout = " << EPOLLOUT << std::endl;

	sylar::Thread::SetName("main");
	// sylar::IOManager iom;
	sylar::IOManager iom(2, false);

	iom.schedule(&test_fiber);
	iom.schedule(&test_fiber);
}

sylar::Timer::ptr s_timer;

void test_timer() {
	sylar::IOManager iom(2);
	s_timer = iom.addTimer(1000, [](){
		static int i = 0;
		SYLAR_LOG_INFO(g_logger) << "hello time i=" << i;

		if(++i == 3) {
			// s_timer->reset(2000, true);
			s_timer->cancel();
		}
	}, true);
}

int main(int argc, char *argv[]) {

	// test1();

	test_timer();
    return 0;
}
