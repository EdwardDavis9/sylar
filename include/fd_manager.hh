#ifndef __SYLAR_FD_MANAGER_H__
#define __SYLAR_FD_MANAGER_H__

#include "singleton.hh"
#include "thread.hh"
#include <vector>
#include <memory>

namespace sylar {

class FdCtx : public std::enable_shared_from_this<FdCtx> {
  public:
    using ptr = std::shared_ptr<FdCtx>;
    FdCtx(int fd);
    ~FdCtx();

    bool init();
    bool isInit() const { return m_isInit; }
    bool isSocket() const { return m_isSocket; }
    bool isClose() const { return m_isClosed; }
    bool close();

    void setUserNonblock(bool v) { m_userNonblock = v; }
    bool getUserNonblock() const { return m_userNonblock; }

    void setSysNonblock(bool v) { m_sysNonblock = v; }
    bool getSysNonblock() const { return m_sysNonblock; }

    void setTimeout(int type, uint64_t v);
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
    FdManager();

    FdCtx::ptr get(int fd, bool auto_create = false);
    void del(int fd);

  private:
    RWMutexType m_mutex;
    std::vector<FdCtx::ptr> m_datas;
};

using FdMgr = Singleton<FdManager>;
// typedef Singleton<FdManager> FdMgr;

}; // namespace sylar

#endif // __SYLAR_FD_MANAGER_H__
