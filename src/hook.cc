#include "sylar/hook.hh"
#include <dlfcn.h>

#include "sylar/config.hh"
#include "sylar/fiber.hh"
#include "sylar/iomanager.hh"
#include "sylar/fd_manager.hh"
#include "sylar/log.hh"
#include <stdarg.h>
#include "sylar/macro.hh"

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace sylar {

static thread_local bool t_hook_enable = false;
static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
    sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(writev)       \
    XX(write)        \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    XX(fcntl)        \
    XX(getsockopt)   \
    XX(setsockopt)   \
    XX(close)        \
    XX(ioctl)

void hook_init()
{
    static bool is_inited = false;
    if (is_inited) {
        return;
    }

// HOOK 的关键
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter()
    {
        hook_init();

        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener(
            [](const int &old_value, const int &new_value) {
                SYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from"
                                         << old_value << " to " << new_value;

                s_connect_timeout = new_value;
            });
    }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() { return t_hook_enable; }

void set_hook_enable(bool flag) { t_hook_enable = flag; }

} // namespace sylar

struct timer_info {
    int cancelled = 0;
};

 /**
 * @brief   执行IO操作的模板函数,实现了协程化异步IO
 * @details 该函数是sylar框架Hook系统的核心,负责将同步阻塞的IO操作
 *          转换为异步非阻塞的协程化操作.处理了超时,信号中断,协程调度等复杂逻辑.
 *          支持任意类型和数量的参数,通过完美转发保持参数特性.
 *
 * @tparam OriginFun 原始IO函数的类型(如read_f,write_f等)
 * @tparam Args 可变参数模板,表示原始IO函数的参数类型列表
 *
 * @param[in] fd            文件描述符,要进行IO操作的目标
 * @param[in] fun           原始IO函数指针(如read_f,write_f等系统调用)
 * @param[in] hook_fun_name Hook函数名称,用于日志记录和调试(如"read","write")
 * @param[in] event         IO事件类型,指定IOManager::READ或IOManager::WRITE
 * @param[in] timeout_so    超时选项类型,指定是SO_RCVTIMEO或SO_SNDTIMEO
 * @param[in] args          可变参数,传递给原始IO函数的参数(完美转发)
 *
 * @return ssize_t IO操作的结果:
 *         - >0:成功读取/写入的字节数
 *         - 0:对端关闭连接(对于TCP读操作)
 *         - -1:发生错误,错误码存储在errno中
 *
 * @note 该函数仅在以下条件下进行协程化处理:
 *       1. Hook功能已启用(sylar::t_hook_enable为true)
 *       2. fd对应的FdCtx存在且未关闭
 *       3. fd是socket类型且用户未设置非阻塞模式
 *       否则将直接调用原始IO函数
 *
 * @attention 该函数可能会挂起当前协程,等待IO事件就绪或超时
 * @warning 在协程环境中,应避免直接使用原始系统调用,而应通过此Hook机制
 *
 * @example
 * // 在hook的read函数中调用:
 * ssize_t read(int fd, void* buf, size_t count) {
 *     return do_io(fd, read_f, "read",
 *                  IOManager::READ, SO_RCVTIMEO,
 *                  buf, count);
 * }
 *
 * @see sylar::FdCtx
 * @see sylar::IOManager
 * @see sylar::Fiber
 */
template <typename OriginFun, class... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&...args)
{
    if (!sylar::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }

    // 获取这个 fd 对应的上下文对象
    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
    if (!ctx) {
        // 不存在上下文对象的话, 调用原始的函数
        return fun(fd, std::forward<Args>(args)...);
    }

    // 如果当前 fd 的上下文被关闭, 那么设置错误信息
    if (ctx->isClose()) {
        errno = EBADF; // errno 为 EBADF(Bad file descriptor)
        return -1;
    }

    // 如果不是 socket, 或在用户层上设置了非阻塞模式, 那么调用原始函数
    if (!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    // 获取当前 fd 的超时时间: 是SO_RCVTIMEO 或 SO_SNDTIMEO
    uint64_t to = ctx->getTimeout(timeout_so);

    // 创建一个定时器, 进行定时操作
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:

    // 调用原始函数
    ssize_t n = fun(fd, std::forward<Args>(args)...);

    // 如果被中断, 那么进行重试
    while (n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }

    // 如果是 EAGAIN(资源暂时不可用, 非阻塞IO常见返回), 阻塞状态, 进入等待事件流程
    if (n == -1 && errno == EAGAIN) {

        // 获取当前线程的 iomanager. 即事件调度器
        sylar::IOManager *iom = sylar::IOManager::GetThis();
        sylar::Timer::ptr condition_timer_to_cancel_event;
        std::weak_ptr<timer_info> winfo(tinfo);

        // 如果设置了超时时间, 添加一个超时定时器, 定时取消当前事件
        if (to != (uint64_t)-1) {
            condition_timer_to_cancel_event = iom->addConditionTimer(
                to,                         // 超时时间
                [winfo, fd, iom, event]() { // 条件定时器的超时逻辑
                    auto t = winfo.lock();  // 临时提升为智能指针
                    if (!t || t->cancelled) {
                        return; // 避免重复取消
                    }
                    t->cancelled = ETIMEDOUT; // 超时类型

                    // 取消 EPOLL 监听
                    iom->cancelEvent(fd, (sylar::IOManager::Event)(event));
                },
                winfo); // 条件变量: 定时器只在未取消状态下有效
        }

        // 添加一个IO事件监听, 比如 EPOLLIN(读) 或 EPOLLOUT(写), 因为无论accpet成功与否都会进行通知
        int rt = iom->addEvent(fd, (sylar::IOManager::Event)(event));

        if (SYLAR_UNLICKLY(rt)) {
            // 添加失败时, 进行日志提醒
            SYLAR_LOG_ERROR(g_logger)
                << hook_fun_name << " addEvent(" << fd << ", " << event << " )";

            if (condition_timer_to_cancel_event) {
                condition_timer_to_cancel_event->cancel();
            }
            return -1;
        }
        else {
            // 让出协程的执行权
            sylar::Fiber::YieldToHold();


            // epoll_wait 监听到对应的事件发生后
            // 会在 idle 中返回到 schedule,最后在正常回调回来
            if (condition_timer_to_cancel_event) {
                condition_timer_to_cancel_event->cancel();
            }
            if (tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }

            // SYLAR_ASSERT(sylar::Fiber::GetThis()->getState()
            //              == sylar::Fiber::EXEC);
            goto retry;
        }
    }

    // if (n >= 0) {
    //     errno = 0;  // 清除错误码
    // }

    return n;
}

extern "C" {
#define XX(name) name##_fun name##_f = nullptr;
HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds)
{
    if (!sylar::t_hook_enable) {
        return sleep_f(seconds);
    }

    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager *iom = sylar::IOManager::GetThis();
    iom->addTimer(seconds * 1000, [iom, fiber]() { iom->schedule(fiber, -1); });

    sylar::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec)
{
    if (!sylar::t_hook_enable) {
        return usleep_f(usec);
    }

    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager *iom   = sylar::IOManager::GetThis();
    iom->addTimer(usec / 1000, [iom, fiber]() { iom->schedule(fiber, -1); });
    sylar::Fiber::YieldToHold();

    return 0;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    if (!sylar::t_hook_enable) {
        return nanosleep_f(rqtp, rmtp);
    }

    int timeout_ms = rqtp->tv_sec * 1000 + rmtp->tv_nsec / 1000 / 1000;

    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager *iom   = sylar::IOManager::GetThis();
    iom->addTimer(timeout_ms, [iom, fiber]() { iom->schedule(fiber, -1); });

    sylar::Fiber::YieldToHold();

    return 0;
}

int socket(int domain, int type, int protocol)
{
    if (!sylar::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }

    int fd = socket_f(domain, type, protocol);
    if (fd == -1) {
        return fd;
    }

    sylar::FdMgr::GetInstance()->get(fd, true);

    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen,
                         uint64_t timeout_ms)
{
    if (!sylar::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }

    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);

    if (!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if (!ctx->isSocket()) {
        return connect_f(fd, addr, addrlen);
    }

    // 若用户主动设置非阻塞,则直接调用原函数,让用户自己处理异步连接
    if (ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }

    int n = connect_f(fd, addr, addrlen);
    if (n == 0) {
        return 0; // 直接成功
    }
    else if (n != -1 || errno != EINPROGRESS) {
        return n; // 如果不是"正在连接中",那么发生了错误
    }

    // if (n == -1 && errno == EINPROGRESS) 的情况

    // 设置超时取消写事件
    sylar::IOManager *iom = sylar::IOManager::GetThis();
    sylar::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo); // 用来记录定时器的状态

    if (timeout_ms != (uint64_t)-1) {
        timer = iom->addConditionTimer(
            timeout_ms,
            [winfo, fd, iom]() {
                auto t = winfo.lock();
                if (!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, sylar::IOManager::WRITE);
            },
            winfo);
    }

    // 注册 WRITE 事件,并挂起协程
    int rt = iom->addEvent(fd, sylar::IOManager::WRITE);
    if (rt == 0) {
        sylar::Fiber::YieldToHold();

        // epoll 回调回来后,在后面检测异步connect中的 fd 是否最终连接失败
        // 先提前取消条件定时器
        if (timer) {
            timer->cancel();
        }
        if (tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    }
    else { // 错误处理
        if (timer) {
            timer->cancel(); // 取消 timer
        }
        SYLAR_LOG_ERROR(g_logger)
            << "connect addEvent(" << fd << ", WRITE error";
    }

    // 检查异步回来的连接是否失败
    int error     = 0;
    socklen_t len = sizeof(int);
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if (!error) {
        return 0;
    }
    else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return connect_with_timeout(sockfd, addr, addrlen,
                                sylar::s_connect_timeout);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int fd = do_io(s, accept_f, "accept", sylar::IOManager::READ, SO_RCVTIMEO,
                   addr, addrlen);

    if (fd >= 0) {
        sylar::FdMgr::GetInstance()->get(fd, true);
    }

    return fd;
}

ssize_t read(int fd, void *buf, size_t count)
{
    return do_io(fd, read_f, "read", sylar::IOManager::READ, SO_RCVTIMEO, buf,
                 count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    return do_io(fd, readv_f, "readv", sylar::IOManager::READ, SO_RCVTIMEO, iov,
                 iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
    return do_io(sockfd, recv_f, "recv", sylar::IOManager::READ, SO_RCVTIMEO,
                 buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen)
{
    return do_io(sockfd, recvfrom_f, "recvfrom", sylar::IOManager::READ,
                 SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
    return do_io(sockfd, recvmsg_f, "recvmsg", sylar::IOManager::READ,
                 SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count)
{
    return do_io(fd, write_f, "write", sylar::IOManager::WRITE, SO_SNDTIMEO,
                 buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    return do_io(fd, writev_f, "writev", sylar::IOManager::WRITE, SO_SNDTIMEO,
                 iov, iovcnt);
}

ssize_t send(int s, const void *mag, size_t len, int flags)
{
    return do_io(s, send_f, "send", sylar::IOManager::WRITE, SO_SNDTIMEO, mag,
                 len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags,
               const struct sockaddr *to, socklen_t tolen)
{
    return do_io(s, sendto_f, "sendto", sylar::IOManager::WRITE, SO_SNDTIMEO,
                 msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags)
{
    return do_io(s, sendmsg_f, "sendmsg", sylar::IOManager::WRITE, SO_SNDTIMEO,
                 msg, flags);
}

int close(int fd)
{
    if (!sylar::t_hook_enable) {
        return close_f(fd);
    }

    // 获得上下文对象
    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);

    if (ctx) {
        // 获得事件调度器
        auto iom = sylar::IOManager::GetThis();

        if (iom) {

            // 取消事件调度器上所有注册的事件
            iom->cancelAll(fd);
        }

        // 删除这个上下文对象
        sylar::FdMgr::GetInstance()->del(fd);
    }

    // 最后通过原有的 close 进行关闭
    return close_f(fd);
}

int fcntl(int fd, int cmd, ...)
{
    va_list va;
    va_start(va, cmd);

    switch (cmd) {
        case F_SETFL: {
            int arg = va_arg(va, int);
            va_end(va);

            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);

            if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                return fcntl_f(fd, cmd, arg);
            }

            // 设置"用户级别"的非阻塞状态信息(记录下来)
            ctx->setUserNonblock(arg & O_NONBLOCK);

            if (ctx->getSysNonblock()) {
                // 如果系统设置过非阻塞, 则强制加上 O_NONBLOCK
                arg |= O_NONBLOCK;
            }
            else {
                arg &= ~O_NONBLOCK;
            }

            return fcntl_f(fd, cmd, arg);
        } break;
        case F_GETFL: {
            va_end(va);
            int arg               = fcntl_f(fd, cmd);
            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);

            if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                return arg;
            }
            // 如果用户设置了非阻塞,则返回带 O_NONBLOCK
            if (ctx->getUserNonblock()) {
                return arg | O_NONBLOCK;
            }
            else {
                return arg & ~O_NONBLOCK;
            }
        } break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
/* 规范化不同的系统 */
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
        {
            int arg = va_arg(va, int);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        } break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
/* 规范化不同的系统 */
#ifdef F_SETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            va_end(va);
            return fcntl_f(fd, cmd);
        } break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK: {
            struct flock *arg = va_arg(va, struct flock *);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        } break;
        case F_GETOWN_EX:
        case F_SETOWN_EX: {
            struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        } break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int fd, unsigned long request, ...)
{
    va_list va;
    va_start(va, request);
    void *arg = va_arg(va, void *);
    va_end(va);

    if (FIONBIO == request) {
        bool user_nonblock    = !!*(int *)arg;
        sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
        if (!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(fd, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(fd, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval,
               socklen_t *optlen)
{
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval,
               socklen_t optlen)
{
    if (!sylar::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if (level == SOL_SOCKET) {
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(sockfd);
            if (ctx) {
                const timeval *v = static_cast<const timeval *>(optval);
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }

    return setsockopt_f(sockfd, level, optname, optval, optlen);
}
}
