#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include "sylar/thread.hh"
#include <functional>
#include <memory>
#include <ucontext.h>

namespace sylar {

class Scheduler;

class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
  public:

	using ptr = std::shared_ptr<Fiber>;

    enum State {
		INIT,   // 初始态
		HOLD,   // 挂起态
		EXEC,   // 执行态
		TERM,   // 终止态
		READY,  // 准备态
		EXCEPT, // 异常态
	};


    /**
     * @brief     构造函数
     * @param[in] cb 协程执行的函数
     * @param[in] stacksize 协程栈大小
     * @param[in] return_to_mainFiber 是否在MainFiber上调度
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool return_to_mainFiber = false);
	~Fiber();

	// 重置协程相关内容
	void reset(std::function<void()> cb);

    // 从当前的主协程调用子协程
	void call();

	// 从子协程切回当前的主协程
	void back();

  /**
   * @brief  获得 Fiber 对象的ID
   */
	uint64_t getId() { return m_id;}

  /**
   * @brief 返回协程状态
   */
	State getState() { return m_state; }

	/**
	 * @brief  获得实际中的 fiber id
	 * @return fiber id
	 */
	static uint64_t GetFiberId();

   //重置协程函数, 并重置状态
   //INIT, TERM

	/**
	 * @brief 从记录中的主协程切换到子协程执行
	 */
	void swapIn();


	/**
	 * @brief 从子协程切换到记录中的主协程执行
	 */
	void swapOut();

	/**
	 * @brief 设置当前运行的协程
	 */
	static void SetThis(Fiber * f);

	/**
	 * @brief 返回当前运行的协程
	 */
	static Fiber::ptr GetThis();

	/**
	 * @brief 将协程切换到记录中的主协程执行, 并且设置为 READY 状态, 即切换到后台
	 */
	static void YieldToReady();

	/**
	 * @brief 将协程切换到后台, 并且设置为 HOLD 状态
	 */
	static void YieldToHold();

	/**
	 * @brief 总协程数
	 */
	static uint64_t TotalFibers();

  /**
	 * @berif 协程执行函数
	 * @post  执行完成后, 回到线程主协程
	 */
	static void SchedulerFiberFunc();

  /**
	 * @berif 协程执行函数
	 * @post  执行完成后, 回到线程调度协程
	 */
	static void MainFiberFunc();

  private:
    Fiber();

	uint64_t m_id = 0; /**< 协程 ID */
	uint32_t m_stacksize = 0; /**< 协程运行栈大小 */
	State m_state = INIT; /**< 协程状态 */

	ucontext_t m_ctx; /**< 协程上下文 */
	void * m_stack = nullptr; /**< 协程运行栈指针 */

	std::function<void()> m_cb;  /**< 协程运行函数 */
};

} // namespace sylar

#endif // __SYLAR_FIBER_H__
