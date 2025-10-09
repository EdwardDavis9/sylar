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
			SYLAR_LOG_DEBUG(g_logger) << "recv http request fail,"
				" errno=" << errno << " errstr=" << strerror(errno)
				<< " client:" << *client << " keep-alive=" << m_isKeepAlive;
			break;
		}

		HttpResponse::ptr rsp(
			new HttpResponse(req->getVersion(),
							 req->isClose() || !m_isKeepAlive));

		rsp->setHeader("Server", getName());
		m_dispatcher->handle(req, rsp, session);

		session->sendResponse(rsp);

		if(!m_isKeepAlive || req->isClose()) {
				break;
		}
	} while(m_isKeepAlive);
	session->close();
}

} // namespace http
} // namespace sylar
