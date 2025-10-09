#include "sylar/timer.hh"
#include "sylar/util.hh"

namespace sylar {

auto Timer::Comparator::operator()(const Timer::ptr &lhs,
                                   const Timer::ptr &rhs) const -> bool
{
    if (!lhs && !rhs) {
        return false;
    }

    if (!lhs) {
        return true;
    }

    if (!rhs) {
        return false;
    }

    if (lhs->m_next < rhs->m_next) {
        return true;
    }
    else if (lhs->m_next > rhs->m_next) {
        return false;
    }

    return lhs.get() < rhs.get();
}

Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring,
             TimerManager *manager)
    : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager)
{
    m_next = sylar::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next) : m_next(next) {}

auto Timer::cancel() -> bool
{
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);

    if (m_cb) {
        m_cb    = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }

    return false;
}

auto Timer::refresh() -> bool
{
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);

    if (!m_cb) {
        return false;
    }

    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }

    m_manager->m_timers.erase(it);
    m_next = sylar::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

auto Timer::reset(uint64_t ms, bool from_now) -> bool
{
    if (ms == m_ms && !from_now) {
        return true;
    }

    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }

    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);

    uint64_t start = 0;

    if (from_now) {
        start = sylar::GetCurrentMS();
    }
    else {
        start = m_next - m_ms;
    }

    m_ms   = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock &lock)
{
    auto it       = m_timers.insert(val).first;
    bool at_front = (it == m_timers.begin()) && !m_tickled;

    // 首个定时器对象需要去通知事件循环
    if (at_front) {
        m_tickled = true;
    }

    lock.unlock();

    if (at_front) {
        onTimerInsertedAtFront();
    }
}

bool TimerManager::detectClockRollover(uint64_t now_ms)
{
    bool rollover = false;
    if (now_ms < m_previousTime && now_ms < (m_previousTime - 60 * 60 * 1000)) {
        // 60 分钟, 60 秒, 1000 毫秒, 1小时
        rollover = true;
    }

    m_previousTime = now_ms;
    return rollover;
}

TimerManager::TimerManager() { m_previousTime = sylar::GetCurrentMS(); }

TimerManager::~TimerManager() {}

auto TimerManager::addTimer(uint64_t ms, std::function<void()> cb,
                            bool recurring)
    -> Timer::ptr
{
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock); // 在添加后释放锁
    return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
{
    std::shared_ptr<void> tmp = weak_cond.lock(); // 获得这个条件定时器对象

    if (tmp) {
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms,
                                           std::function<void()> cb,
                                           std::weak_ptr<void> weak_cond,
                                           bool recurring)
{
    // return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
    return addTimer(ms, [=](){
        OnTimer(weak_cond, cb);
    }, recurring);
}

uint64_t TimerManager::getNextTimer()
{
    RWMutexType::ReadLock lock(m_mutex);

    m_tickled = false;

    if (m_timers.empty()) {
        return ~0ull; // 返回一个 ui64 的最大值
    }

    const Timer::ptr &next = *m_timers.begin();
    uint64_t now_ms        = sylar::GetCurrentMS();
    if (now_ms >= next->m_next) {
        return 0;
    }
    else {
        return next->m_next - now_ms;
    }
}

auto TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs)
    -> void
{
    uint64_t now_ms = sylar::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_timers.empty()) {
            return;
        }
    }

    RWMutexType::WriteLock lock(m_mutex);

    if(m_timers.empty()) {
        return;
    }

    // 检测时间是否错误, 以及首个定时器是否过期
    bool rollover = detectClockRollover(now_ms);
    if (!rollover && (*m_timers.begin())->m_next > now_ms) {
        return;
    }

    Timer::ptr now_timer(new Timer(now_ms));

    // 通过 lower_bound 获取一个大于等于当前时间的定时器, 这是还未到期的定时器
    // 而 [begin, it) 范围内的定时器表示到期了
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);

    // 通过 ++ 获取后续所有相同且定时时间的定时器
    while (it != m_timers.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);

    // 需要进行处理的过期定时器的回调函数
    cbs.reserve(expired.size());

    for (auto &timer : expired) {
        cbs.push_back(timer->m_cb);

        if (timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer); // 插入时还需要进行排序判断
        }
        else {
            timer->m_cb = nullptr;
        }
    }
}

}; // namespace sylar
