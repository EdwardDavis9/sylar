#include "sylar/sylar.hh"
#include <assert.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();


void test_assert() {
	SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);

	// SYLAR_ASSERT(false);

	SYLAR_ASSERT2( 0 == 1, "test_assert output message");
}

void test_another()
{
	test_assert();
}

void fun3() {
	int i = 0;
	while(++i < 10) {
		SYLAR_LOG_INFO(g_logger) << i << ":xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
	}
}

void error() {
	// throw std::
}

void test_error() {

	std::vector<sylar::Thread::ptr> thrs;

	for(int i = 0; i < 1; ++i) {
		sylar::Thread::ptr thr(new sylar::Thread(&fun3,  "name_"));
		sylar::Thread::ptr thr2(new sylar::Thread(&fun3, "name_"));

		thrs.push_back(thr);
		thrs.push_back(thr2);
	}
}


int main(int argc, char* argv[]) {
	// test_error();


	// assert false ;
	// test_assert();
	test_another();
	return 0;
}
