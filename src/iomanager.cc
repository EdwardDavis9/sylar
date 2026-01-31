#include "sylar/iomanager.hh"
#include "sylar/log.hh"
#include "sylar/macro.hh"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

IOManager::FdContext::EventContext &
IOManager::FdContext::getContext(IOManager::Event event)
{
    switch (event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false, "getContext invalid event: " + std::to_string(static_cast<int>(event)));
            throw std::invalid_argument("Invalid event type in getContext");
    }
}

void IOManager::FdContext::resetContext(EventContext &ctx)
{
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event)
{

    SYLAR_ASSERT(events & event);
    events            = static_cast<Event>(events & ~event);
    EventContext &ctx = getContext(event);

    if (ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    }
    else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t threads,
					 bool include_caller_thread,
					 const std::string &name)
    : Scheduler(threads, include_caller_thread, name)
{
    m_epfd = epoll_create(500);
    SYLAR_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    SYLAR_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0x00, sizeof(epoll_event));
    event.events  = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    SYLAR_ASSERT(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    SYLAR_ASSERT(!rt);

    contextResize(32); // 设置上下文的大小

    start(); // 在这里自动开始进行调度
}

void IOManager::contextResize(size_t size)
{
    m_fdContexts.resize(size);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (!m_fdContexts[i]) {
            m_fdContexts[i]     = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

IOManager::~IOManager()
{
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (auto &i : m_fdContexts) {
        if(i) {
            delete i;
        }
    }

}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
{
    FdContext *fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);

    // 获取句柄事件对应的 fdContext 对象
    if (static_cast<int>(m_fdContexts.size()) > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    }
    else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    // 检查待添加的事件是否已存在, 防止重复注册
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (fd_ctx->events & event) {
        SYLAR_LOG_ERROR(g_logger)
            << "Exist error: addEvent assert fd = " << fd << " event = " << event
            << " fd_ctx.event =" << fd_ctx->events;

        SYLAR_ASSERT(!(fd_ctx->events & event));
    }

    // 构造 epoll_event 并注册事件
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    if(op == EPOLL_CTL_MOD) {
        epevent.events = EPOLLET | fd_ctx->events | event;
    } else if(op & EPOLL_CTL_ADD){
        epevent.events = EPOLL_FLAGS | fd_ctx->events | event;
    }
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);

    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", "
            << epevent.events << ")" << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        return -1;
    }

    // 更新状态并绑定调度器/协程/回调
    ++m_pendingEventCount;
    fd_ctx->events = static_cast<Event>(fd_ctx->events | event);

    // 获取刚才添加的事件上下文, 然后添加调度器, 协程, 回调函数
    FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
    SYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();

    if (cb) {
        event_ctx.cb.swap(cb);
    }
    else {
        event_ctx.fiber = Fiber::GetThis(); // 保存返回点的上下文
        SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC,
                      "state=" << event_ctx.fiber->getState());
    }

    return 0;
}

bool IOManager::delEvent(int fd, Event event)
{
    RWMutexType::ReadLock lock(m_mutex);
    if (static_cast<int>(m_fdContexts.size()) <= fd) {
        return false;
    }

    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }

	// 将数据按位取反, 然后与原来的 events 做与操作,  即删除原来的事件
    Event new_events = static_cast<Event>(fd_ctx->events & ~event);
    int op           = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events   = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", "
            << epevent.events << "):" << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        return false;
    }

    // 更新 IOManager 的内部状态
    --m_pendingEventCount;

    fd_ctx->events                     = new_events; // 更新 fd 的事件集合
    FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx); // 清理当前事件的回调或协程
    return true;
}

bool IOManager::cancelEvent(int fd, Event event)
{
    RWMutexType::ReadLock lock(m_mutex);
    if (static_cast<int>(m_fdContexts.size()) <= fd) {
        return false;
    }
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }

    Event new_events = static_cast<Event>(fd_ctx->events & ~event);
    int op           = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events   = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", "
            << epevent.events << "):" << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd)
{
    RWMutexType::ReadLock lock(m_mutex);
    if (static_cast<int>(m_fdContexts.size()) <= fd) {
        return false;
    }

    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!fd_ctx->events) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events   = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", "
            << epevent.events << "):" << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        return false;
    }

    if (fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }

    if (fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    SYLAR_ASSERT(fd_ctx->events == 0);
    return true;
}

IOManager *IOManager::GetThis()
{
    return dynamic_cast<IOManager *>(Scheduler::GetThis());
    //  Scheduler* sched = Scheduler::GetThis();
    // IOManager* iom = dynamic_cast<IOManager*>(sched);
    // if (!iom) {
    //     SYLAR_LOG_ERROR(g_logger) << "Current Scheduler is not IOManager!";
    // }
    // return iom;
}

void IOManager::tickle()
{
    if (hasIdleThreads()) {
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    SYLAR_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t& timeout)
{
    timeout = getNextTimer();

    return timeout == ~0ull
        && m_pendingEventCount == 0
        && Scheduler::stopping();
}

bool IOManager::stopping()
{
    uint64_t timeout = 0;
    return stopping(timeout);
}

void IOManager::idle()
{
    SYLAR_LOG_DEBUG(g_logger) << "idle";
    epoll_event *events = new epoll_event[64]();
    std::shared_ptr<epoll_event> shared_events(
        events, [](epoll_event *ptr) { delete[] ptr; });

    // 测试 tcp-server的默认行为
    // 发现，两个 idle 协程会不断输出内容
    // uint64_t test = 0;

    while (true) {
        uint64_t next_timeout = 0;
        if(stopping(next_timeout)) {
            SYLAR_LOG_INFO(g_logger)
                << "name=" << getName() << " idle stopping exit";
            break;
        }

        int rt{0};

        // 测试 tcp-server的默认行为
        // SYLAR_LOG_DEBUG(g_logger)  << test++ << std::endl;

        do {
            // 最大超时时间
            static  const int MAX_TIMEOUT = 3000;
            if(next_timeout != ~0ull) {
                next_timeout = static_cast<int>(next_timeout) > MAX_TIMEOUT
                    ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd, events, 64, static_cast<int>(next_timeout));
            if (rt < 0 && errno == EINTR) {
            }
            else {
                break;
            }
        } while (true);

        std::vector<std::function<void()> > cbs;
        listExpiredCb(cbs);
        if(!cbs.empty()) {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for (int i = 0; i < rt; ++i) {
            epoll_event &event = events[i];
            if (event.data.fd == m_tickleFds[0]) {
                uint8_t dummy;
                int saved_errno = errno;

                while (read(m_tickleFds[0], &dummy, 1) == 1)
                    ;
                // 这里会产生一个错误, 非阻塞式读取了一个内容,因此会设置了errno, 因此后续需要进行恢复
                // SYLAR_LOG_INFO(g_logger) << "read ===2" << " errno=" << errno
                //                          << ", error_msg="<<strerror(errno);

                errno = saved_errno; // 恢复之前的errno
                continue;
            }

            // 事件触发时, 携带了指定的上下文类
            FdContext *fd_ctx = static_cast<FdContext *>(event.data.ptr);
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= EPOLLIN | EPOLLOUT;
            }
            int real_events = NONE;
            if (event.events & EPOLLIN) {
                real_events |= READ;
            }
            if (event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            // 只会关心指定的读写IO事件, 其余事件一律跳过
            if ((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            int left_events = (fd_ctx->events & ~real_events);
            int op          = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events    = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if (rt2) {
                SYLAR_LOG_ERROR(g_logger)
                    << "epoll_ctl(" << m_epfd << ", " << op << ", "
                    << fd_ctx->fd << ", " << event.events << "):" << rt2 << " ("
                    << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            if (real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }

            if (real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

		// 让当前线程 主动让出执行权
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr   = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }
}

void IOManager::onTimerInsertedAtFront() {
    tickle();
}

}; // namespace sylar
