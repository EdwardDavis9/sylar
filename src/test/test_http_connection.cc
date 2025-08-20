#include "http/http_connection.hh"
#include "sylar/log.hh"
#include "sylar/iomanager.hh"

#include <fstream>
#include <iostream>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run()
{
    sylar::Address::ptr addr =
        sylar::Address::LookupAnyIPAddress("www.baidu.com:80");

    if (!addr) {
        SYLAR_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
    bool rt                 = sock->connect(addr);
    if (!rt) {
        SYLAR_LOG_INFO(g_logger) << "connect " << *addr << " fails";
        return;
    }

    sylar::http::HttpConnection::ptr conn(
        new sylar::http::HttpConnection(sock));
    sylar::http::HttpRequest::ptr req(new sylar::http::HttpRequest);
	// req->setPath("/blog");
	req->setHeader("host", "www.baidu.com");
	SYLAR_LOG_INFO(g_logger) << "req:" << std::endl
		<< *req;

	conn->sendRequest(req);
	auto rsp = conn->recvResponse();

	if(!rsp) {
		SYLAR_LOG_INFO(g_logger) <<" recv response error";
		return;
	}

	SYLAR_LOG_INFO(g_logger) << "rsp:" << std::endl
		<< *rsp;
}

int main(int argc, char *argv[])
{
    sylar::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
