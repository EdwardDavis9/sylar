#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <string>
#include <pthread.h>
#include <iostream>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>

#include "sylar/noncopyable.hh"


namespace sylar {

// 对 semaphore 的一个基本的封装
class Semaphore : Noncopyable{
public:
	Semaphore(uint32_t count = 0);
	~Semaphore();

	void wait();
	void notify();


private:
	sem_t m_semaphore;
};

// RAII ()->acquire, ~()->release

template<class T>
struct ScopedLockImpl {
	public:
	ScopedLockImpl(T& mutex):m_mutex(mutex)
	{
		m_mutex.lock();
		m_locked = true;
	}

    ~ScopedLockImpl() {
        unlock();
    }

	void lock() {
		if(!m_locked) {
			m_mutex.lock();
			m_locked = true;
		}
	}

	void unlock() {
		if(m_locked) {
			m_mutex.unlock();
			m_locked = false;
		}
	}

	private:
	T & m_mutex; // 实际的锁对象
	bool m_locked;
};

template <class T>
struct ReadScopedLockImpl {
public:
	ReadScopedLockImpl(T & mutex)
		:m_mutex(mutex) {
		m_mutex.rdlock();
		m_locked = true;
	}

	~ReadScopedLockImpl() {
		unlock();
	}

	void lock(){
		if(!m_locked) {
			m_mutex.rdlock();
			m_locked = true;
		}
	}

	void unlock() {
		if(m_locked) {
			m_mutex.unlock();
			m_locked = false;
		}
	}

private:
	T & m_mutex;
	bool m_locked;
};

template <class T>
struct WriteScopedLockImpl {
public:
	WriteScopedLockImpl(T& mutex)
		:m_mutex(mutex) {
		m_mutex.wrlock();
		m_locked = true;
	}

	~WriteScopedLockImpl() {
		unlock();
	}

	void lock() {
		if(!m_locked) {
			m_mutex.wrlock();
			m_locked = true;
		}
	}

	void unlock() {
		if(m_locked) {
			m_mutex.unlock();
			m_locked = false;
		}
	}

private:
	T & m_mutex;
	bool m_locked;
};

class Mutex : Noncopyable {
public:

	using Lock = ScopedLockImpl<Mutex>;

	Mutex() {
		pthread_mutex_init(&m_mutex, nullptr);
	}

	~Mutex() {
		pthread_mutex_destroy(&m_mutex);
	}

	void lock() {
		pthread_mutex_lock(&m_mutex);
	}

	void unlock() {
		pthread_mutex_unlock(&m_mutex);
	}

	private:
		pthread_mutex_t m_mutex;
};

class NullMutex : Noncopyable {
public:
	using Lock = ScopedLockImpl<NullMutex>;
	NullMutex() {}
	~NullMutex() {}

	void lock() {}
	void unlock() {}
};

class RWMutex:Noncopyable {
	public:
	using ReadLock  = ReadScopedLockImpl <RWMutex>;
	using WriteLock = WriteScopedLockImpl<RWMutex>;

	RWMutex() {
		pthread_rwlock_init(&m_lock, nullptr);
	}

	~RWMutex() {
		pthread_rwlock_destroy(&m_lock);
	}

	void rdlock() {
		pthread_rwlock_rdlock(&m_lock);
	}

	void wrlock() {
		pthread_rwlock_wrlock(&m_lock);
	}

	void unlock() {
		pthread_rwlock_unlock(&m_lock);
	}

private:
	pthread_rwlock_t m_lock;
};

class NullRWMutex:Noncopyable {
public:

	using ReadLock  = ReadScopedLockImpl<NullMutex>;
	using WriteLock = WriteScopedLockImpl<NullMutex>;

	NullRWMutex() {}
	~NullRWMutex() {}

	void rdlock() {};
	void wrlock() {};
	void unlock() {};
};

// 依赖系统的用户态的自旋
class Spinlock:Noncopyable {
public:
	using Lock = ScopedLockImpl<Spinlock>;

	// 系统层面上的自旋锁

	Spinlock() {
		pthread_spin_init(&m_mutex, 0);
	}

	~Spinlock() {
		pthread_spin_destroy(&m_mutex);
	}

	void lock() {
		pthread_spin_lock(&m_mutex);
	}

	void unlock() {
		pthread_spin_unlock(&m_mutex);
	}
private:
	pthread_spinlock_t m_mutex;
};


// 完全基于用户态的自旋
class CASLock : Noncopyable {
public:
	using Lock = ScopedLockImpl<CASLock>;


	CASLock() {
		m_mutex.clear();
	}

	~CASLock() {
	}

	void lock() {
		while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
	}

	void unlock() {
		std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
	}

private:
	volatile std::atomic_flag m_mutex;
};

class Thread {
public:
    using ptr = std::shared_ptr<Thread>;
	Thread(std::function<void()> cb, const std::string& name);
	~Thread();

	pid_t getId() const { return m_id;}
	const std::string& getName() const { return m_name;}

	void join();

	static Thread* GetThis();
	static const std::string& GetName();
	static void SetName(const std::string& name);

private:
	Thread(const Thread&) = delete;
	Thread(const Thread&&) = delete;
	Thread& operator=(const Thread&) = delete;

	static void* run(void* arg);

private:
	pid_t m_id         = 1;  /**< 线程标识 */
	pthread_t m_thread = 0;  /**< 线程 */

	std::function<void()> m_cb; /**< 线程回调函数 */
	std::string m_name = "Thread_";         /**< 线程名 */

	Semaphore m_semaphore;
};
} // namespace sylar

#endif // __SYLAR_THREAD_H__
