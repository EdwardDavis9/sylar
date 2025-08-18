#include "http/http_server.hh"
#include "sylar/log.hh"

namespace sylar {
namespace http {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

HttpServer::HttpServer(bool isKeepAlive,
					   sylar::IOManager *worker,
					   sylar::IOManager *accept_worker)
	:TcpServer(worker, accept_worker),
	m_isKeepAlive(isKeepAlive)
{
	m_dispatcher.reset(new ServletDispatcher);
}

void HttpServer::handleClient(Socket::ptr client)
{
	HttpSession::ptr session(new HttpSession(client));
	do {
		auto req = session->recvRequest();
		if(!req) {
			SYLAR_LOG_WARN(g_logger) << "recv http request fail,"
				" errno=" << errno << " errstr=" << strerror(errno)
				<< " client:" << *client;
			break;
		}

		HttpResponse::ptr rsp(
			new HttpResponse(req->getVersion(),
							 req->isClose() || !m_isKeepAlive));

		rsp->setHeader("Server", getName());
		m_dispatcher->handle(req, rsp, session);

		// rsp->setBody("hello sylar");

       SYLAR_LOG_INFO(g_logger) << "requst:" << std::endl
           << *req;
       SYLAR_LOG_INFO(g_logger) << "response:" << std::endl
           << *rsp;

		session->sendResponse(rsp);
	} while(m_isKeepAlive);
	session->close();
}

} // namespace http
} // namespace sylar
