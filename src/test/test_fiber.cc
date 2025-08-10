#include "sylar/sylar.hh"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run_in_fiber() {
	SYLAR_LOG_INFO(g_logger) << "        run_in_fiber begin";
	sylar::Fiber::YieldToHold();
	SYLAR_LOG_INFO(g_logger) << "        run_in_fiber end";
	sylar::Fiber::YieldToHold();
}

void test_fiber() {
	SYLAR_LOG_INFO(g_logger) << "main begin {";
	{
		// sylar::Fiber::GetThis();
		sylar::Scheduler sc;

		SYLAR_LOG_INFO(g_logger) << "main begin";

		// 在单个线程中，每当创建一个Fiber时，会随之额外产生两个 Fiber 对象
		// 1. 当创建 Scheduler 时， 在构造函数中会创建一个
		// 2, 当通过 YielHold 时，会创建第二个
		// 总共就出现了三个对象
		sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
		fiber->swapIn();

		SYLAR_LOG_INFO(g_logger) << "main affter swapIn";
		fiber->swapIn();

		SYLAR_LOG_INFO(g_logger) << "main after end";
		fiber->swapIn();
	}

	SYLAR_LOG_INFO(g_logger) << "main after end }";
}

int main() {
	sylar::Thread::SetName("main");

	std::vector<sylar::Thread::ptr> thrs;

	for(int i = 0; i < 2; ++i) {
		thrs.push_back(
			sylar::Thread::ptr(
				new sylar::Thread(&test_fiber, "name_" + std::to_string(i))));
	}

	for(size_t i = 0; i < thrs.size(); ++i) {
		thrs[i]->join();
	}

    // for(auto i : thrs) {
    //     i->join();
    // }
	return 0;
}
