#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include "thread.hh"
#include <functional>
#include <memory>
#include <ucontext.h>

namespace sylar {

class Fiber : public std::enable_shared_from_this<Fiber> {
  public:

	using ptr = std::shared_ptr<Fiber>;

    enum State { INIT, HOLD, EXEC, TERM, READY };

    Fiber(std::function<void()> cb, size_t stacksize = 0);
	~Fiber();

	void reset(std::function<void()> cb);

    //重置协程函数，并重置状态
    //INIT，TERM

	/**
	 * @brief 切换到当前协程执行
	 */
	void swapIn();


	/**
	 * @brief 切换到后台执行
	 */
	void swapOut();

	/**
	 * @brief 设置当前协程
	 */
	static void SetThis(Fiber * f);

	/**
	 * @brief 返回当前协程
	 */
	static Fiber::ptr GetThis();

	/**
	 * @brief 将协程切换到后台，并且设置为 READY 状态
	 */
	static void YieldToReady();

	/**
	 * @brief 将协程切换到后台，并且设置为 HOLD 状态
	 */
	static void YieldToHold();

	/**
	 * @brief 总协程数
	 */
	static uint64_t TotalFibers();

	static void MainFunc();

  private:
    Fiber();

	uint64_t m_id = 0;
	uint32_t m_stacksize = 0;
	State m_state = INIT;

	ucontext_t m_ctx;
	void * m_stack = nullptr;

	std::function<void()> m_cb;
};

} // namespace sylar

#endif // __SYLAR_FIBER_H__
