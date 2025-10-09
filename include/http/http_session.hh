#ifndef __HTTP_SESSION_H__
#define __HTTP_SESSION_H__

#include "http/socketstream.hh"
#include "http/http.hh"

namespace sylar {
namespace http {

/**
 * @brief HTTPSession 封装
 */
class HttpSession : public SocketStream {

  public:
    using ptr = std::shared_ptr<HttpSession>;

    /**
     * @brief     构造函数
     * @param[in] sock Socket 类型
     * @param[in] owner 是否托管
     */
    HttpSession(Socket::ptr sock, bool owner = true);

    /**
     * @brief 接收HTTP请求
     */
    HttpRequest::ptr recvRequest();

    /**
     * @brief     发送HTTP 响应
     * @param[in] rsp HTTP 响应
     * @return    >0 发送成功
     *         =0 对方关闭
     *         <0 Socket 异常
     */
    int sendResponse(HttpResponse::ptr rsp);
};

}; // namespace http
}; // namespace sylar

#endif // __HTTP_SESSION_H__
