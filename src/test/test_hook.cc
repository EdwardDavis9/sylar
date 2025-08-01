#include "hook.hh"
#include "log.hh"
#include "iomanager.hh"
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_sleep()
{
	sylar::IOManager iom(1);

	iom.schedule([](){
		sleep(2);
		SYLAR_LOG_INFO(g_logger) << " sleep(2) ";
	});


	iom.schedule([](){
		sleep(3);
		SYLAR_LOG_INFO(g_logger) << " sleep(3) ";
	});

	SYLAR_LOG_INFO(g_logger) << "hook sleep end";
}

void test_sock() {

	int sock = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	inet_pton(AF_INET, "180.101.51.73", &addr.sin_addr.s_addr);
	// inet_pton(AF_INET, "183.2.172.177", &addr.sin_addr.s_addr);
	// inet_pton(AF_INET, "115.239.210.2", &addr.sin_addr.s_addr);

	SYLAR_LOG_INFO(g_logger) << "begin connect";
	int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
	SYLAR_LOG_INFO(g_logger) << "connect rt = " << rt << " errno=" << errno;

	if(rt) {
		return;
	}

	const char data[] = "GET / HTTP/1.0\r\n\r\n";
	rt = send(sock, data, sizeof(data), 0);
	SYLAR_LOG_INFO(g_logger) << "send rt = " << rt << " errno=" << errno;

	if(rt <= 0) { return; }

	std::string buff;
	buff.resize(4096);

	rt = recv(sock, &buff[0], buff.size(), 0);

	SYLAR_LOG_INFO(g_logger) << "recv rt = " << rt << " errno = " << errno;

	if(rt <= 0) {
		return;
	}

	buff.resize(rt);
	SYLAR_LOG_INFO(g_logger) << buff;

}

int main()
{
	// test_sleep();

	sylar::IOManager iom;
	iom.schedule(test_sock);

	return 0;
}
