#ifndef __SYLAR_FD_MANAGER_H__
#define __SYLAR_FD_MANAGER_H__

#include "sylar/singleton.hh"
#include "sylar/thread.hh"
#include <vector>
#include <memory>

namespace sylar {

class FdCtx : public std::enable_shared_from_this<FdCtx> {
  public:
    using ptr = std::shared_ptr<FdCtx>;
    FdCtx(int fd);
    ~FdCtx();

    /**
     * @brief 初始化
     */
    bool init();

    /**
     * @brief 是否初始化完成
     */
    bool isInit() const { return m_isInit; }

    /**
     * @brief 是否socket
     */
    bool isSocket() const { return m_isSocket; }

    /**
     * @brief 是否已关闭
     */
    bool isClose() const { return m_isClosed; }

    /**
     * @brief     设置用户主动设置非阻塞
     * @param[in] v 是否阻塞
     */
    void setUserNonblock(bool v) { m_userNonblock = v; }

    /**
     * @brief 获取是否用户主动设置的非阻塞
     */
    bool getUserNonblock() const { return m_userNonblock; }

    /**
     * @brief     设置系统非阻塞
     * @param[in] v 是否阻塞
     */
    void setSysNonblock(bool v) { m_sysNonblock = v; }

    /**
     * @brief 获取系统非阻塞
     */
    bool getSysNonblock() const { return m_sysNonblock; }

    /**
     * @brief     设置超时时间
     * @param[in] type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
     * @param[in] v 时间毫秒
     */
    void setTimeout(int type, uint64_t v);

    /**
     * @brief     获取超时时间
     * @param[in] type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
     * @return    超时时间毫秒
     */
    uint64_t getTimeout(int type);

  private:
    bool m_isInit      :1;  /**< 是否初始化完成 */
    bool m_isSocket    :1;  /**< 是否是 socket 类型的 fd */
    bool m_sysNonblock :1;  /**< 系统是否设置非阻塞 */
    bool m_userNonblock:1;  /**< 用户是否主动要求非阻塞 */
    bool m_isClosed    :1;  /**< 是否已经关闭 */
    int m_fd;               /**< 当前文件描述符 */
    uint64_t m_recvTimeout; /**< 接收超时时间 */
    uint64_t m_sendTimeout; /**< 发送超时时间 */
};

class FdManager {
  public:
    using RWMutexType = RWMutex;

    /**
     * @brief 无参构造函数
     */
    FdManager();

    /**
     * @brief     获取/创建文件句柄类FdCtx
     * @param[in] fd 文件句柄
     * @param[in] auto_create 是否自动创建
     * @return    返回对应文件句柄类FdCtx::ptr
     */
    FdCtx::ptr get(int fd, bool auto_create = false);

    /**
     * @brief     删除文件句柄类
     * @param[in] fd 文件句柄
     */
    void del(int fd);

  private:
    RWMutexType m_mutex;
    std::vector<FdCtx::ptr> m_datas;
};

using FdMgr = Singleton<FdManager>;
// typedef Singleton<FdManager> FdMgr;

}; // namespace sylar

#endif // __SYLAR_FD_MANAGER_H__
