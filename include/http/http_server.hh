#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "http/tcp_server.hh"
#include "http/http_session.hh"
#include "http/servlet.hh"

namespace sylar {
namespace http {

/**
 * @brief HTTP服务器类
 */
class HttpServer : public TcpServer {
  public:
    using ptr = std::shared_ptr<HttpServer>;

    /**
     * @brief     构造函数
     * @param[in] keepalive 是否长连接
     * @param[in] worker 工作调度器
     * @param[in] accept_worker 接收连接调度器
     */
    HttpServer(bool isKeepAlice                = false,
               sylar::IOManager *worker        = sylar::IOManager::GetThis(),
               sylar::IOManager *accept_worker = sylar::IOManager::GetThis());

    /**
     * @brief 获取 ServletDispatch
     */
    ServletDispatcher::ptr getServletDispatcher() const { return m_dispatcher; }

    /**
     * @brief 设置 ServletDispatch
     */
    void setServletDispatcher(ServletDispatcher::ptr dispatcher)
    {
        m_dispatcher = dispatcher;
    }

  protected:
    virtual void handleClient(Socket::ptr client) override;

    ServletDispatcher::ptr m_dispatcher; /**< servelet 分发器 */

  private:
    bool m_isKeepAlive; /**< 是否支持长连接 */
};
} // namespace http
} // namespace sylar

#endif // __HTTP_SERVER_H__
