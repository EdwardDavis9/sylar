#ifndef __SYLAR_IMANAGER_H__
#define __SYLAR_IMANAGER_H__

#include "scheduler.hh"
#include "timer.hh"

namespace sylar {
class IOManager : public Scheduler, public TimerManager {
  public:
    using ptr         = std::shared_ptr<IOManager>;
    using RWMutexType = RWMutex;

    enum Event {
        NONE  = 0x0,
        READ  = 0x1, // EPOLLIN
        WRITE = 0x4, // EPOLLOUT
    };

  private:
    struct FdContext {
        using MutexType = Mutex;

        struct EventContext {
            Scheduler *scheduler;     /**< 事件执行的scheduler */
            Fiber::ptr fiber;         /**< 事件协程 */
            std::function<void()> cb; /**< 事件的回调函数 */
        };

        EventContext &getContext(Event event);
        void resetContext(EventContext &ctx);
        void triggerEvent(Event event);

        EventContext read;   /**< 读事件 */
        EventContext write;  /**< 写事件 */
        int fd       = 0;    /**< 事件关联的句柄 */
        Event events = NONE; /**< 已经注册的事件 */
        MutexType mutex;
    };

  public:
    IOManager(size_t threads = 1, bool use_caller = true,
              const std::string &name = "");

    ~IOManager();

	// 0 success, -1 error
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

	bool delEvent(int fd, Event event, std::function<void()> cb = nullptr);
	bool delEvent(int fd, Event event);
	bool cancelEvent(int fd, Event event);
	bool cancelAll(int fd);
	static IOManager* GetThis();

protected:
	void tickle() override;
	bool stopping() override;
	bool stopping(uint64_t& timeout) ;
	void idle() override;
	void onTimerInsertedAtFront() override;

	void contextResize(size_t size);
	void onTimerInsertedAtFront(uint64_t timeout);

private:
    int m_epfd {0};
	int m_tickleFds[2];

	std::atomic<size_t> m_pendingEventCount {0}; /**< 全局待处理事件计数 */
	RWMutexType m_mutex;

	// fd 的事件上下文数组,每个 fd 对应一个读/写事件上下文(EventContext)
	std::vector<FdContext*> m_fdContexts;
};

} // namespace sylar

#endif // __SYLAR_IMANAGER_H__
