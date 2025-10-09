#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__

#include <memory>
#include <functional>

#include "sylar/address.hh"
#include "sylar/iomanager.hh"
#include "sylar/socket.hh"
#include "sylar/noncopyable.hh"

namespace sylar {

/**
 * @brief TCP 服务器封装
 */
class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
  public:
    using ptr = std::shared_ptr<TcpServer>;

    /**
     * @brief     构造函数
     * @param[in] woker socket 客户端工作的协程调度器
     * @param[in] accept_woker 服务器 socket 执行接收 socket 连接的协程调度器
     */
    TcpServer(sylar::IOManager *worker        = sylar::IOManager::GetThis(),
              sylar::IOManager *accept_worker = sylar::IOManager::GetThis());
    uint64_t getRecvTimeout() const { return m_recvTimeout; }
    std::string getName() const { return m_name; }

    /**
     * @brief 设置服务器名称
     */
    void setName(const std::string &name) { m_name = name; }

    /**
     * @brief 设置读取超时时间(毫秒)
     */
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }
    bool isStop() const { return m_isStop; }

    /**
     * @brief 析构函数
     */
    virtual ~TcpServer();

    /**
     * @brief  绑定地址
     * @return 返回是否绑定成功
     */
    virtual bool bind(sylar::Address::ptr addr);

    /**
     * @brief      绑定地址数组
     * @param[in]  addrs 需要绑定的地址数组
     * @param[out] fails 绑定失败的地址
     * @return     是否绑定成功
     */
    virtual bool bind(const std::vector<Address::ptr> &addrs,
                      std::vector<Address::ptr> &fails);

    /**
     * @brief 启动服务
     * @pre   需要 bind 成功后执行
     */
    virtual bool start();

    /**
     * @brief 是否停止
     */
    virtual void stop();

  protected:
    /**
     * @brief 处理新连接的 Socket 类
     */
    virtual void handleClient(Socket::ptr client);

    /**
     * @brief 开始接受连接
     */
    virtual void startAccept(Socket::ptr sock);

  private:
    std::vector<Socket::ptr> m_socks; /**< 监听 Socket 数组 */
    IOManager *m_worker;              /**< 新连接的 Socket 工作的调度器 */
    IOManager *m_acceptWorker;        /**< 服务器 Socket 接受连接的调度器 */
    uint64_t m_recvTimeout;           /**< 接受超时时间 */
    std::string m_name;               /**< 服务器名称 */
    bool m_isStop;                    /**< 服务是否停止 */
};
}; // namespace sylar

#endif // __SYLAR_TCP_SERVER_H__
