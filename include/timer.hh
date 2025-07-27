#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <memory>
#include <vector>
#include <set>
#include "thread.hh"

namespace sylar {
class TimerManager;

/**
 * @class Timer
 * @brief 原子定时器对象
 */
class Timer : public std::enable_shared_from_this<Timer> {
    friend class TimerManager;

  public:
    using ptr = std::shared_ptr<Timer>;

    /**
     * @brief 取消定时器，不再触发
     */
    bool cancel();

    /**
     * @brief 重新计算触发时间(用于延后)
     */
    bool refresh();

    /**
     * @brief 重设定时器的间隔或触发时间
     */
    bool reset(uint64_t ms, bool from_now);

  private:
    Timer(uint64_t ms, std::function<void()> cb, bool recurring,
          TimerManager *manager);
    Timer(uint64_t next);

  private:
    bool m_recurring = false;          /**< 是否是循环周期定时器 */
    uint64_t m_ms    = 0;              /**< 定时器间隔时间 */
    uint64_t m_next  = 0;              /**< 定时器下次触发的时间(单位：毫秒)*/
    std::function<void()> m_cb;        /**< 定时器的回调函数 */
    TimerManager *m_manager = nullptr; /**< 定时器管理器 */

  private:
    struct Comparator {
        bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
    };
};

/**
 * @class TimerManager
 * @brief 定时器管理对象
 */
class TimerManager {
    friend class Timer;

  public:
    using RWMutexType = RWMutex;

    TimerManager();

    virtual ~TimerManager();

    /**
     * @brief 添加普通定时器
     */
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb,
                        bool recurring = false);

    /**
     * @brief 添加条件定时器
     */
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                                 std::weak_ptr<void> weak_cond,
                                 bool recurring = false);

    /**
     * @brief 获取下一个定时器的剩余时间
     */
    uint64_t getNextTimer();

    /**
     * @brief 获取所有到期定时器的回调函数
     */
    void listExpiredCb(std::vector<std::function<void()>> &cbs);

    /**
     * @brief 是否存在定时器
     */
    bool hasTimer();

  protected:
    /**
     * @brief 定时器插入时的通知(如 epoll 唤醒)
     */
    virtual void onTimerInsertedAtFront() = 0;

    void addTimer(Timer::ptr val, RWMutexType::WriteLock &lock);

  private:
    /**
     * @berif 判断系统时间是否跳变
     */
    bool detectClockRollover(uint64_t now_ms);

  private:
    RWMutexType m_mutex;

    std::set<Timer::ptr, Timer::Comparator>
        m_timers; /**< 定时器的集合,按触发时间排序 */

    bool m_tickled          = false; /**< 标记是否需要通知主事件循环 */
    uint64_t m_previousTime = 0;     /**< 上一次时间记录,用于检测系统时间回拨 */
};

}; // namespace sylar

#endif // __SYLAR_TIMER_H__
