#ifndef __SYLAR_HTTP_CONNECTION_H__
#define __SYLAR_HTTP_CONNECTION_H__

#include <list>
#include <memory>

#include "sylar/thread.hh"
#include "http/socketstream.hh"
#include "http/http.hh"
#include "http/uri.hh"

namespace sylar {
namespace http {

struct HttpResult {
    using ptr = std::shared_ptr<HttpResult>;

    /**
     * @class Error
     */
    enum class Error {
        OK                      = 0, /**< 正常 */
        INVAILD_URL             = 1, /**< 无效的 URI */
        INVAILD_HOST            = 2, /**< 无效的 HOST */
        CONNECT_FAIL            = 3, /**< 连接失败 */
        SEND_CLOSE_BY_PEER      = 4, /**< 连接被对端关闭 */
        SEND_SOCKET_ERROR       = 5, /**< 发送请求产生 socket 错误 */
        TIMEOUT                 = 6, /**< 超时 */
        CREATE_SOCKET_ERROR     = 7, /**< 创建 socket 失败 */
        POOL_GET_CONNECTION     = 8, /**< 从连接池中获取连接失败 */
        POOL_INVAILD_CONNECTION = 9  /**< 无效的连接 */
    };

    HttpResult(int result, HttpResponse::ptr response, const std::string &error)
        : m_result(result), m_response(response), m_error(error)
    {}

    std::string toString() const;

    int m_result;                 /**< 错误码 */
    HttpResponse::ptr m_response; /**< Http 响应结构体 */
    std::string m_error;          /**< 错误描述 */
};

class HttpConnectionPool;

/**
 * @class HttpConnection
 */
class HttpConnection : public SocketStream { //
    friend class HttpConnectionPool;

  public:
    using ptr = std::shared_ptr<HttpConnection>; /**< HttpConnection 智能指针 */

    ~HttpConnection();

    /**
     * @brief     发送 HTTP 的 GET 请求
     * @param[in] url 请求的url
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP 请求头部参数
     * @param[in] body 请求消息体
     * @return    返回 HTTP 结果结构体
     */
    static HttpResult::ptr
    DoGet(const std::string &url,
          uint64_t timeout_ms,
          const std::map<std::string, std::string> &headers = {},
          const std::string &body                           = "");

    /**
     * @brief     发送 HTTP 的 GET 请求
     * @param[in] url URI 结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP 请求头部参数
     * @param[in] body 请求消息体
     * @return    返回 HTTP 结果结构体
     */
    static HttpResult::ptr
    DoGet(Uri::ptr uri,
          uint64_t timeout_ms,
          const std::map<std::string, std::string> &headers = {},
          const std::string &body                           = "");

    /**
     * @brief     发送 HTTP 的 POST 请求
     * @param[in] url 请求的 url
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP 请求头部参数
     * @param[in] body 请求消息体
     * @return    返回 HTTP 结果结构体
     */
    static HttpResult::ptr
    DoPost(const std::string &url,
           uint64_t timeout_ms,
           const std::map<std::string, std::string> &headers = {},
           const std::string &body                           = "");

    /**
     * @brief     发送 HTTP 的 POST 请求
     * @param[in] url URI 结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP 请求头部参数
     * @param[in] body 请求消息体
     * @return    返回 HTTP 结果结构体
     */
    static HttpResult::ptr
    DoPost(Uri::ptr uri,
           uint64_t timeout_ms,
           const std::map<std::string, std::string> &headers = {},
           const std::string &body                           = "");

    /**
     * @brief     发送 HTTP 的 Request 请求
     * @param[in] method 请求类型
     * @param[in] url 请求的 url
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP 请求头部参数
     * @param[in] body 请求消息体
     * @return    返回 HTTP 结果结构体
     */
    static HttpResult::ptr
    DoRequest(HttpMethod method,
              const std::string &url,
              uint64_t timeout_ms,
              const std::map<std::string, std::string> &headers = {},
              const std::string &body                           = "");

    /**
     * @brief     发送 HTTP 的 Request 请求
     * @param[in] method 请求类型
     * @param[in] url URI 结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP 请求头部参数
     * @param[in] body 请求消息体
     * @return    返回 HTTP 结果结构体
     */
    static HttpResult::ptr
    DoRequest(HttpMethod method,
              Uri::ptr url,
              uint64_t timeout_ms,
              const std::map<std::string, std::string> &headers = {},
              const std::string &body                           = "");

    /**
     * @brief     发送 HTTP 的 Request 请求
     * @param[in] req 请求结构体
     * @param[in] uri URI 结构体
     * @return    返回 HTTP 结果结构体
     */
    static HttpResult::ptr
    DoRequest(HttpRequest::ptr req, Uri::ptr uri, uint64_t timeout_ms);

    /**
     * @brief     构造函数
     * @param[in] sock Socket 类
     * @param[in] owner 是否掌握所有权
     */
    HttpConnection(Socket::ptr sock, bool owner = true);

    /**
     * @brief 析构函数
     */
    HttpResponse::ptr recvResponse();

    /**
     * @brief     发送 HTTP 请求
     * @param[in] req HTTP 请求结构
     * @return    发送的字节数
     */
    int sendRequest(HttpRequest::ptr req);

  private:
    uint64_t m_createTime = 0; /**< 创建时间 */
    uint64_t m_request    = 0; /**< 请求次数 */
};

/**
 * @class HttpConnectionPool
 */
class HttpConnectionPool {
  public:
    using ptr       = std::shared_ptr<HttpConnectionPool>;
    using MutexType = Mutex;

    /**
     * @brief     连接池的构造函数
     * @param[in] host 请求的地址
     * @param[in] vhost 虚拟地址
     * @param[in] port 端口
     * @param[in] max_size 最大数量
     * @param[in] max_alive_time 最大存活时间
     * @param[in] request 最大请求量
     */
    HttpConnectionPool(const std::string &host,
                       const std::string &vhost,
                       uint32_t port,
                       uint32_t max_size,
                       uint32_t max_alive_time,
                       uint32_t max_request);

    HttpConnection::ptr getConnection();

    /**
     * @brief     发送 HTTP 的 GET 请求
     * @param[in] url 请求的 url
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return    返回 HTTP 结果结构体
     */
    HttpResult::ptr
    doGet(const std::string &url,
          uint64_t timeout_ms,
          const std::map<std::string, std::string> &headers = {},
          const std::string &body                           = "");

    /**
     * @brief     发送 HTTP 的 GET 请求
     * @param[in] uri URI 结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return    返回 HTTP 结果结构体
     */
    HttpResult::ptr
    doGet(Uri::ptr uri,
          uint64_t timeout_ms,
          const std::map<std::string, std::string> &headers = {},
          const std::string &body                           = "");

    /**
     * @brief     发送 HTTP 的 POST 请求
     * @param[in] url 请求的 url
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return    返回 HTTP 结果结构体
     */
    HttpResult::ptr
    doPost(const std::string &url,
           uint64_t timeout_ms,
           const std::map<std::string, std::string> &headers = {},
           const std::string &body                           = "");

    /**
     * @brief     发送 HTTP 的 POST 请求
     * @param[in] uri URI 结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return    返回 HTTP 结果结构体
     */
    HttpResult::ptr
    doPost(Uri::ptr uri,
           uint64_t timeout_ms,
           const std::map<std::string, std::string> &headers = {},
           const std::string &body                           = "");

    /**
     * @brief     发送 HTTP 请求
     * @param[in] method 请求类型
     * @param[in] url 请求的 url
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return    返回 HTTP 结果结构体
     */
    HttpResult::ptr
    doRequest(HttpMethod method,
              const std::string &url,
              uint64_t timeout_ms,
              const std::map<std::string, std::string> &headers = {},
              const std::string &body                           = "");

    /**
     * @brief     发送 HTTP 请求, 然后接收 HTTP 响应
     * @details   实际上是转发给另一个 doRequest
     * @param[in] method 请求类型
     * @param[in] uri URI结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return    返回 HTTP 结果结构体
     */
    HttpResult::ptr
    doRequest(HttpMethod method,
              Uri::ptr uri,
              uint64_t timeout_ms,
              const std::map<std::string, std::string> &headers = {},
              const std::string &body                           = "");

    /**
     * @brief     发送 HTTP 请求, 然后接收 HTTP 响应
     * @param[in] req 请求结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @return    返回 HTTP 结果结构体
     */
    HttpResult::ptr doRequest(HttpRequest::ptr req, uint64_t timeout_ms);

  private:
    /**
     * @brief         释放无效的连接
     * @param[in,out] ptr 连接对象
     * @param[out]    pool 连接池
     */
    static void ReleasePtr(HttpConnection *ptr, HttpConnectionPool *pool);

  private:
    std::string m_host; /**< 目标主机名(域名或者 IP 地址), 比如 "www.example.com" */

    /// 虚拟主机名(HTTP Host 头用的), 和 m_host 不一定相同
    /// 如:访问 "http://127.0.0.1:8080", m_host 是"127.0.0.1"
    /// 但 Host 头要写 "www.example.com"
    std::string m_vhost;

    uint32_t m_port; /**< 目标主机的端口号, 例如 80 (HTTP) 或 443 (HTTPS) */

    /// 连接池中最多允许的连接数上限
    /// 超过这个上限就不会再新建连接
    uint32_t m_maxSize;

    // 每个连接的最大存活时间(毫秒)
    // 超过这个时间就认为连接过期, 下次归还时会被销毁
    uint32_t m_maxAliveTime; /**<  */

    uint32_t m_maxRequest; /**< 最大请求次数 */

    MutexType m_mutex;

    /// 空闲连接列表(已经建立好但暂时没在用的连接)
    /// getConnection() 会从这里取一个, ReleasePtr() 会把用过的放回来
    std::list<HttpConnection *> m_conns;

    std::atomic<int32_t> m_total = {0}; /**< 连接总数 */
};
}; // namespace http
}; // namespace sylar

#endif // __SYLAR_HTTP_CONNECTION_H__
