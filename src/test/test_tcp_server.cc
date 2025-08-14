#include <vector>

#include "sylar/log.hh"
#include "http/tcp_server.hh"
#include "sylar/iomanager.hh"


sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
	// auto addr = sylar::Address::LookupAny("0.0.0.0");
	auto addr = sylar::Address::LookupAny("0.0.0.0:8033");
	// SYLAR_LOG_INFO(g_logger) << *addr;

	// auto addr2 = sylar::UnixAddress::ptr(
	// 	new sylar::UnixAddress("/tmp/unix_addr"));

	// SYLAR_LOG_INFO(g_logger) << *addr2;

	std::vector<sylar::Address::ptr> addrs;
	addrs.push_back(addr);
	// addrs.push_back(addr2);

	sylar::TcpServer::ptr tcp_server(new sylar::TcpServer);
	std::vector<sylar::Address::ptr> fails;

	while(!tcp_server->bind(addrs, fails)) {
		sleep(2);
	}

	tcp_server->start();
}


int main(int argc, char *argv[]) {
	sylar::IOManager iom(2);
	iom.schedule(run);
    return 0;
}

