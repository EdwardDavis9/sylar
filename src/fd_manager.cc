#include "sylar/fd_manager.hh"
#include "sylar/hook.hh"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace sylar {

FdCtx::FdCtx(int fd)
    : m_isInit(false), m_isSocket(false), m_sysNonblock(false),
      m_userNonblock(false), m_fd(fd), m_recvTimeout(-1), m_sendTimeout(-1)
{
    init();
}

FdCtx::~FdCtx() {}

auto FdCtx::init() -> bool
{
    if (m_isInit) {
        return true;
    }

    m_recvTimeout = -1;
    m_sendTimeout = -1;

    struct stat fd_stat;
    if (-1 == fstat(m_fd, &fd_stat)) {
        m_isInit   = false;
        m_isSocket = false;
    }
    else {
        m_isInit   = true;
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }

    if (m_isSocket) {
        int flags = fcntl_f(m_fd, F_GETFL, 0);

        // 当前 socket_fd 不是非阻塞模式的话
        if (!(flags & O_NONBLOCK)) {
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        }

        m_sysNonblock = true;
    }
    else {
        m_sysNonblock = false;
    }

    m_userNonblock = false;
    m_isClosed     = false;

    return m_isInit;
}

auto FdCtx::setTimeout(int type, uint64_t v) -> void
{
    if (type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    }
    else {
        m_sendTimeout = v;
    }
}

auto FdCtx::getTimeout(int type) -> uint64_t
{
    if (type == SO_RCVTIMEO) {
        return m_recvTimeout;
    }
    else {
        return m_sendTimeout;
    }
}

FdManager::FdManager() { m_datas.resize(64); }

auto FdManager::get(int fd, bool auto_create) -> FdCtx::ptr
{
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_datas.size() <= fd) {
        if (auto_create == false) {
            return nullptr;
        }
    }
    else {
        if (m_datas[fd] || !auto_create) {
            return m_datas[fd];
        }
    }

    lock.unlock();

    RWMutexType::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    m_datas[fd] = ctx;
    return ctx;
}

auto FdManager::del(int fd) -> void
{
    RWMutexType::WriteLock lock(m_mutex);
    if ((int)m_datas.size() <= fd) {
        return;
    }
    m_datas[fd].reset();
}

}; // namespace sylar
