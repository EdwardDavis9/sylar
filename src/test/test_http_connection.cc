#include "http/http_connection.hh"
#include "sylar/log.hh"
#include "sylar/iomanager.hh"

#include <fstream>
#include <iostream>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

/**
 * @brief 测试连接池
 */
void test_pool()
{
    sylar::http::HttpConnectionPool::ptr pool(
        new sylar::http::HttpConnectionPool("www.baidu.com", "", 80, 1,
                                            1000 * 30, 5));
    sylar::IOManager::GetThis()->addTimer(
        1000,
        [pool]() {
            auto r = pool->doGet("/", 3000);

            // SYLAR_LOG_INFO(g_logger) << r->toString();
            // SYLAR_LOG_INFO(g_logger) << r->m_response->toString();

            auto conn_header = r->m_response->getHeaders("connection", "xxxxxxxxxxxxxxxx");
            SYLAR_LOG_INFO(g_logger) << "Server Connection header: " << conn_header;
        },
        true);
}

void run()
{
#if 0
    sylar::Address::ptr addr =
        sylar::Address::LookupAnyIPAddress("www.baidu.com:80");
        // sylar::Address::LookupAnyIPAddress("www.httpbin.com:80");

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

    req->setPath("/");
    req->setHeader("User-Agent", "curl/7.81.0");
    req->setHeader("host", "www.baidu.com");
    // req->setHeader("host", "www.httpbin.com");

    SYLAR_LOG_INFO(g_logger) << "req:" << std::endl
    	<< *req;

    SYLAR_LOG_INFO(g_logger) << "about to sendRequest";

    // 测试简单的请求和响应
    int64_t n = conn->sendRequest(req);
    SYLAR_LOG_INFO(g_logger) << "sendRequest returned bytes=" << n;

    SYLAR_LOG_INFO(g_logger) << "wait to recvResponse";
    auto rsp = conn->recvResponse();

    if (!rsp) {
        SYLAR_LOG_INFO(g_logger) << "recv response error";
        return;
    }

    SYLAR_LOG_INFO(g_logger) << "rsp:" << std::endl
    	<< *rsp;

    SYLAR_LOG_INFO(g_logger) << "=========================";

    auto r = sylar::http::HttpConnection::DoGet("http://www.baidu.com/", 3000);
    // auto r = sylar::http::HttpConnection::DoGet("http://www.httpbin.com/", 3000);
    SYLAR_LOG_INFO(g_logger)
        << "result=" << r->m_result << " error=" << r->m_error
        << " rsp="
        << (r->m_response ? r->m_response->toString() : "");

    SYLAR_LOG_INFO(g_logger) << "=========================";
#endif

    // 测试连接池
    test_pool();
}

int main(int argc, char *argv[])
{
    sylar::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
