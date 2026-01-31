#ifndef __SYLAR_IMANAGER_H__
#define __SYLAR_IMANAGER_H__

#include "sylar/scheduler.hh"
#include "sylar/timer.hh"

namespace sylar {

/**
 * @brief 基于Epoll的IO协程调度器
 */
class IOManager : public Scheduler, public TimerManager {
  public:
    using ptr         = std::shared_ptr<IOManager>;
    using RWMutexType = RWMutex;

    /**
     * @enum Event
     */
    enum Event {
        NONE  = 0x0,   /**< 无事件 */
        READ  = 0x1,   /**< 读事件(EPOLLIN)*/
        WRITE = 0x4,   /**< 写事件(EPOLLOUT)*/
    };

  private:

#ifdef EPOLLEXCLUSIVE
#define EPOLL_FLAGS  (EPOLLET | EPOLLEXCLUSIVE)
#else
#define EPOLL_FLAGS  (EPOLLET)
#endif

    /**
     * @struct Socket 文件描述符上下文: 句柄+事件
     */
    struct FdContext {
        using MutexType = Mutex;

        /**
         * @struct 句柄对应事件的上下文: 调度器+协程/回调
         */
        struct EventContext {
            Scheduler *scheduler = nullptr;     /**< 事件执行的scheduler */
            Fiber::ptr fiber;                   /**< 事件协程 */
            std::function<void()> cb;           /**< 事件的回调函数 */
        };

        /**
         * @brief     获取事件上下文
         * @param[in] event 事件类型
         * @return    返回对应事件的上下文
         */
        EventContext &getContext(Event event);

        /**
         * @brief          重置事件上下文
         * @param[in, out] ctx 待重置的上下文
         */
        void resetContext(EventContext &ctx);

        /**
         * @brief     触发事件
         * @param[in] event 事件类型
         */
        void triggerEvent(Event event);

        EventContext read;   /**< 读事件 */
        EventContext write;  /**< 写事件 */
        int fd       = 0;    /**< 事件关联的句柄 */
        Event events = NONE; /**< 已经注册的事件 */
        MutexType mutex;     /**< 锁 */
    };

  public:
    /**
     * @brief     构造函数
     * @param[in] threads 线程数量
     * @param[in] include_caller_thread 调用线程是否也参与任务的执行
     * @param[in] name 调度器的名称
     */
    IOManager(size_t threads = 1, bool include_caller_thread = true,
              const std::string &name = "");

    /**
     * @brief 析构函数
     */
    ~IOManager();

    /**
     * @brief     添加事件
     * @param[in] fd socket句柄
     * @param[in] event 事件类型
     * @param[in] cb 事件回调函数
     * @return    添加成功返回0, 失败返回-1
     */
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

  /**
   * @brief     删除事件
   * @param[in] fd socket 句柄
   * @param[in] event 事件类型
   * @attention 不会触发事件
   */
	bool delEvent(int fd, Event event);

  /**
   * @brief     取消事件
   * @param[in] fd socket 句柄
   * @param[in] event 事件类型
   * @attention 如果事件存在则触发事件
   */
	bool cancelEvent(int fd, Event event);

  /**
   * @brief     取消所有事件
   * @param[in] fd socket 句柄
   * @return    是否成功取消
   */
	bool cancelAll(int fd);

  /**
   * @brief 返回当前的 IOManager
   */
	static IOManager* GetThis();

protected:
	void tickle() override;

	bool stopping() override;

  /**
   * @brief      判断是否可停止
   * @param[out] timeout 最近要出发的时间定时器事件间隔
   * @return     返回是否可停止
   */
	bool stopping(uint64_t& timeout) ;
	void idle() override;
	void onTimerInsertedAtFront() override;

  /**
   * @brief     重置 socket 句柄上下文容器大小
   * @param[in] size 容量大小
   */
	void contextResize(size_t size);
	void onTimerInsertedAtFront(uint64_t timeout);

private:
    int m_epfd {0}; /**< epoll 文件句柄 */
	int m_tickleFds[2]; /**< pipe 文件句柄 */

	std::atomic<size_t> m_pendingEventCount {0}; /**< 全局待处理事件计数 */
	RWMutexType m_mutex;

	// fd 的事件上下文数组,每个 fd 对应一个读/写事件上下文(EventContext)
	std::vector<FdContext*> m_fdContexts; /**< socket 事件上下文容器 */
};

} // namespace sylar

#endif // __SYLAR_IMANAGER_H__
