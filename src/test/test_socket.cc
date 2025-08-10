#include "sylar/socket.hh"
#include "sylar/log.hh"
#include "sylar/iomanager.hh"

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("root");

void test_socket() {


    // std::vector<sylar::Address::ptr> addrs;
    // sylar::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    // sylar::IPAddress::ptr addr;
    // for(auto& i : addrs) {
    //    SYLAR_LOG_INFO(g_logger) << i->toString();
    //    addr = std::dynamic_pointer_cast<sylar::IPAddress>(i);
    //    if(addr) {
    //        break;
    //    }
    // }

	sylar::IPAddress::ptr addr = sylar::Address::LookupAnyIPAddress("www.baidu.com");
	if(addr) {
		SYLAR_LOG_INFO(g_logger) << "get address: " << addr->toString();
	} else {
		SYLAR_LOG_ERROR(g_logger) << "get address faile";
		return;
	}

	sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
	addr->setPort(80);
	SYLAR_LOG_INFO(g_logger) << "addr=" << addr->toString();
	if(!sock->connect(addr)) {
		SYLAR_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
	} else {
		SYLAR_LOG_ERROR(g_logger) << "connect " << addr->toString() << " connected";
	}

	const char buff[] = "GET / HTTP/1.0\r\n\r\n";
	int rt = sock->send(buff, sizeof(buff));
	if(rt <= 0) {
		SYLAR_LOG_INFO(g_logger) << "send fail rt = " << rt;
		return;
	}

	std::string buffs;
	buffs.resize(4096);
	rt = sock->recv(&buffs[0], buffs.size());

	if(rt <= 0) {
		SYLAR_LOG_INFO(g_logger) << "recv fail rt=" << rt;
		return;
	}

	buffs.resize(rt);
	SYLAR_LOG_INFO(g_logger) << buffs;
}

int main(int argc, char *argv[]) {
	sylar::IOManager iom;
	iom.schedule(&test_socket);

	// test_socket();

    return 0;
}
