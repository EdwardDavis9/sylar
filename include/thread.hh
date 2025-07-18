#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>


namespace sylar {

class Semaphore {
public:
	Semaphore(uint32_t count = 0);
	~Semaphore();

	void wait();
	void notify();

private:
	Semaphore(const Semaphore&) = delete;
	Semaphore(const Semaphore&&) = delete;
	Semaphore& operator=(const Semaphore&) = delete;

private:
	sem_t m_semaphore;
};

template<class T>
struct ScopedLockImpl{
	public:
	ScopedLockImpl(T& mutex):m_mutex(mutex)
	{
		m_mutex.lock();
		m_locked = true;
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
	T & m_mutex;
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


class RWMutex {
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
	std::string m_name;         /**< 线程名 */

	Semaphore m_semaphore;
};
} // namespace sylar

#endif // __SYLAR_THREAD_H__
