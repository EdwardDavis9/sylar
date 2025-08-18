#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "http/tcp_server.hh"
#include "http/http_session.hh"
#include "http/servlet.hh"

namespace sylar {
namespace http {
class HttpServer : public TcpServer {
  public:
    using ptr = std::shared_ptr<HttpServer>;

    HttpServer(bool isKeepAlice = false,
               sylar::IOManager *worker
                 = sylar::IOManager::GetThis(),
               sylar::IOManager *accept_worker
                 = sylar::IOManager::GetThis());

  ServletDispatcher::ptr getServletDispatcher() const { return m_dispatcher; }
  void setServletDispatcher(ServletDispatcher::ptr dispatcher)
    { m_dispatcher = dispatcher;}

  protected:
    virtual void handleClient(Socket::ptr client) override;
    ServletDispatcher::ptr m_dispatcher;

  private:
    bool m_isKeepAlive;
};
} // namespace http
} // namespace sylar

#endif // __HTTP_SERVER_H__
