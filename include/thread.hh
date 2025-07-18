#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <string>
#include <pthread.h>

namespace sylar {

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
};
} // namespace sylar

#endif // __SYLAR_THREAD_H__
