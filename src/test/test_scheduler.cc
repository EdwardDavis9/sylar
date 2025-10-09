#include "sylar/sylar.hh"
#include "sylar/hook.hh"
#include "sylar/iomanager.hh"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_fiber() {
	sylar::set_hook_enable(false);
	static int s_count = 5;
	SYLAR_LOG_INFO(g_logger) << "test_fiber, s_count" << s_count;


	sleep(1);
	if(--s_count >= 0) {
		sylar::Scheduler::GetThis()->schedule(&test_fiber, sylar::GetThreadId());
	}
}

int main() {

	SYLAR_LOG_INFO(g_logger) << "main";
	// sylar::Scheduler sc;
	sylar::Scheduler sc(2);
	// sylar::Scheduler sc(3);
	// sylar::Scheduler sc(3, true, "test");
	// sylar::Scheduler sc(3, false, "test");

	// sylar::set_hook_enable(false);


	sc.start();

	// sleep(1);
	// SYLAR_LOG_INFO(g_logger) << "schedule";
	sc.schedule(&test_fiber);

	// SYLAR_LOG_INFO(g_logger) << "------------------------";

	sc.stop();
	// SYLAR_LOG_INFO(g_logger) << "over";
	return 0;
}
