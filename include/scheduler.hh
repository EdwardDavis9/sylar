#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include "fiber.hh"
#include "thread.hh"

namespace sylar {

class Scheduler {
  public:
    using ptr       = std::shared_ptr<Scheduler>;
    using MutexType = Mutex;

    Scheduler(size_t threads = 1,
			  bool use_caller = true,
			  const std::string& name= "");


	const std::string& getName() const { return m_name; }

	void start();
	void stop();

	virtual ~Scheduler();
	static Scheduler* GetThis();
	static Fiber*     GetMainFiber();

	template<class FiberOrCb>
	void schedule(FiberOrCb fc, int thread = -1) {
		bool need_tickle = false;
		{
			MutexType::Lock lock(m_mutex);
			need_tickle = scheduleNoLock(fc, thread);
		}

		if(need_tickle) {
			tickle();
		}
	}

	template<class InputIterator>
	void schedule(InputIterator begin, InputIterator end) {
		bool need_tickle = false;
		{
			MutexType::Lock lock(m_mutex);
			while(begin != end) {
				need_tickle = scheduleNoLock(&*begin) || need_tickle;
			}
		}

		if(need_tickle) {
			tickle();
		}
	}

  protected:
    std::vector<int>    m_threadIds;
    std::atomic<size_t> m_activeThreadCount{0};
    std::atomic<size_t> m_idleThreadCount{0};

    bool   m_stopping    = true;
    bool   m_autoStop    = false;
    int    m_rootThread  = 0;
    size_t m_threadCount = 0;

	bool hasIdleThreads() { return m_idleThreadCount > 0; }

    void run();

	/**
	 * @brief 设置线程对应的调度器对象
	 */
    void setThis();

    virtual void tickle();
    virtual void idle();
    virtual bool stopping();

  private:
    template <class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread)
    {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);            // 封装对象

        if (ft.fiber || ft.cb) {
			m_fibers.push_back(ft);
        }
		return need_tickle;
    }

	// 包装的任务对象
    struct FiberAndThread {
        int                   thread; /**< 绑定的执行线程 */
        Fiber::ptr            fiber;
        std::function<void()> cb;

        FiberAndThread(Fiber::ptr f, int thr) :thread(thr), fiber(f)  {}

        FiberAndThread(Fiber::ptr *f, int thr) : thread(thr) { fiber.swap(*f); }

        FiberAndThread(std::function<void()> f, int thr) : thread(thr), cb(f) {}

        FiberAndThread(std::function<void()> *f, int thr) : thread(thr)
        {
            cb.swap(*f);
        }

        FiberAndThread() : thread(-1) {}

        void reset()
        {
            fiber  = nullptr;
            cb     = nullptr;
            thread = -1;
        }
    };

    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;
    std::list<FiberAndThread> m_fibers;
    Fiber::ptr m_rootFiber;
    std::string m_name;
};

} // namespace sylar

#endif // __SYLAR_SCHEDULER_H__
