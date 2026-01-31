/**
 * @file      scheduler.hh
 * @brief     协程调度器封装
 * @date      Fri Sep  5 18:11:20 2025
 * @author    edward
 * @copyright BSD-3-Clause
 */
#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include "sylar/fiber.hh"
#include "sylar/thread.hh"

namespace sylar {

/**
 * @brief   协程调度器
 * @details 封装的是N-M的协程调度器
 *          内部有一个线程池,支持协程在线程池里面切换
 */
class Scheduler : public std::enable_shared_from_this<Scheduler> {
  public:
    using ptr       = std::shared_ptr<Scheduler>;
    using MutexType = Mutex;

    /**
     * @brief     构造函数
     * @param[in] threads 线程数量
     * @param[in] include_caller_thread 当前调用线程也参与执行任务
     * @param[in] name 协程调度器名称
     */
    Scheduler(size_t threads = 1,
			  bool include_caller_thread = true,
			  const std::string& name= "");


  /**
   * @brief 返回协程调度器名称
   */
	const std::string& getName() const { return m_name; }

  /**
   * @brief 启动协程调度器
   */
	void start();

  /**
   * @brief 停止协程调度器, 并且会进行终止前的任务检测
   */
	void stop();

	virtual ~Scheduler();

  /**
   * @brief 返回当前协程调度器
   */
	static Scheduler* GetThis();

  /**
   * @brief 返回当前协程调度器的调度协程
   */
	static Fiber*     GetMainFiber();


  /**
   * @brief     调度协程
   * @param[in] fc 协程或函数
   * @param[in] thread 协程执行的线程id, -1标识任意线程
   */
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

   /**
    * @brief     批量调度协程
    * @param[in] begin 协程数组的开始
    * @param[in] end 协程数组的结束
    */
	template<class InputIterator>
	void schedule(InputIterator begin, InputIterator end) {
		bool need_tickle = false;
		{
			MutexType::Lock lock(m_mutex);
			while(begin != end) {
				need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
				++begin;
			}
		}

		if(need_tickle) {
			tickle();
		}
	}

  protected:
    std::vector<int>    m_threadIds;            /**< 协程下的线程 ID 数组 */
    std::atomic<size_t> m_activeThreadCount{0}; /**< 工作线程数量 */
    std::atomic<size_t> m_idleThreadCount{0};   /**< 空闲线程数量 */

    bool   m_stopping    = true;  /**< 是否停止 */
    bool   m_autoStop    = false; /**< 是否自动停止 */
    int    m_rootThread  = 0;     /**< 主线程 ID(use_caller) */
    size_t m_threadCount = 0;     /**< 线程数量 */

    /**
     * @brief 是否有空闲线程
     */
	  bool hasIdleThreads() { return m_idleThreadCount > 0; }

    /**
     * @brief 协程调度函数
     */
    void run();

	/**
	 * @brief 设置当前线程对应的调度器对象
	 */
    void setThis();

    /**
     * @brief 通知协程调度器有任务了
     */
    virtual void tickle();

    /**
     * @brief 协程无任务可调度时执行idle协程
     */
    virtual void idle();

    /**
     * @brief 返回是否可以停止
     */
    virtual bool stopping();

  private:

    /**
     * @brief 协程调度启动(无锁)
     */
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

    /**
     * @brief 协程/函数/线程组
     */
    struct FiberAndThread {
        int                   thread; /**< 线程 ID */
        Fiber::ptr            fiber;  /**< 协程 */
        std::function<void()> cb;     /**< 协程执行函数 */


        /**
         * @brief     构造函数
         * @param[in] f 协程
         * @param[in] thr 线程id
         */
        FiberAndThread(Fiber::ptr f, int thr) :thread(thr), fiber(f)  {}

        /**
         * @brief     构造函数
         * @param[in] f 协程指针
         * @param[in] thr 线程id
         * @post      *f = nullptr
         */
        FiberAndThread(Fiber::ptr *f, int thr) : thread(thr) { fiber.swap(*f); }

        /**
         * @brief     构造函数
         * @param[in] f 协程执行函数
         * @param[in] thr 线程id
         */
        FiberAndThread(std::function<void()> f, int thr) : thread(thr), cb(f) {}

        /**
         * @brief     构造函数
         * @param[in] f 协程执行函数指针
         * @param[in] thr 线程id
         * @post      *f = nullptr
         */
        FiberAndThread(std::function<void()> *f, int thr) : thread(thr)
        {
            cb.swap(*f);
        }

        /**
         * @brief 无参构造函数
         */
        FiberAndThread() : thread(-1) {}

        /**
         * @brief 重置数据
         */
        void reset()
        {
            fiber  = nullptr;
            cb     = nullptr;
            thread = -1;
        }
    };

    MutexType m_mutex;                  /**< Mutex */
    std::vector<Thread::ptr> m_threads; /**< 线程池 */
    std::list<FiberAndThread> m_fibers; /**< 待执行的协程队列 */
    Fiber::ptr m_rootFiber;             /**< use_caller 为 true 时有效, 调度协程 */
    std::string m_name;                 /**< 协程调度器名称 */
};

} // namespace sylar

#endif // __SYLAR_SCHEDULER_H__
