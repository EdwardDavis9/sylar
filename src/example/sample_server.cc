#include "http/http_server.hh"
#include "sylar/log.hh"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

#define to_string(...) #__VA_ARGS__

void run()
{
    sylar::Address::ptr addr =
        sylar::Address::LookupAny(to_string(0.0.0.0:8020));

		if(!addr) {
				SYLAR_LOG_ERROR(g_logger) << "get address error";
				return;
		}

		sylar::http::HttpServer::ptr http_server(new sylar::http::HttpServer(true));
		while(!http_server->bind(addr)) {
				SYLAR_LOG_ERROR(g_logger) << "bind " << *addr << " faile";
				sleep(1);
		}
		http_server->start();
}

int main()
{
		// 设置日志的级别
    g_logger->setLevel(sylar::LogLevel::INFO);
    sylar::IOManager iom(1);
    iom.schedule(run);
    return 0;
}
