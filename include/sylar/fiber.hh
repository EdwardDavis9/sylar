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

    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
	~Fiber();

	// 初始化协程相关内容
	void reset(std::function<void()> cb);

    // 从当前的主协程调用子协程
	void call();

	// 从子协程切回当前的主协程
	void back();

	uint64_t getId() { return m_id;}

	State getState() { return m_state; }

	static uint64_t GetFiberId();

    //重置协程函数，并重置状态
    //INIT，TERM

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
	 * @brief 将协程切换到记录中的主协程执行，并且设置为 READY 状态，即切换到后台
	 */
	static void YieldToReady();

	/**
	 * @brief 将子协程切换到后台，并且设置为 HOLD 状态
	 */
	static void YieldToHold();

	/**
	 * @brief 总协程数
	 */
	static uint64_t TotalFibers();

    /**
	 * @berif 协程调度
	 */
	static void MainFunc();

	static void CallerMainFunc();

  private:
    Fiber();

	uint64_t m_id = 0;
	uint32_t m_stacksize = 0;
	State m_state = INIT;

	ucontext_t m_ctx;
	void * m_stack = nullptr;

	std::function<void()> m_cb; // 协程的回调函数
};

} // namespace sylar

#endif // __SYLAR_FIBER_H__
