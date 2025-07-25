#include <execinfo.h>

#include "util.hh"
#include "log.hh"
#include "fiber.hh"

namespace sylar {

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

pid_t GetThreadId() { return syscall(SYS_gettid); }

void Backtrace(std::vector<std::string>& bt, int size, int skip) {
	void ** array = static_cast<void**>(malloc(sizeof(void*) * size));
	size_t s = ::backtrace(array, size);

	char ** strings = backtrace_symbols(array, s);
	if(strings == NULL) {
		SYLAR_LOG_ERROR(g_logger) << "backtrace_symbols error";
		return;
	}

	for(size_t i = skip; i < s; ++i) {
		bt.push_back(strings[i]);
	}

	free(strings);
	free(array);
}

std::string BacktraceToString(int size, int skip, const std::string& prefix) {
	std::vector<std::string> bt;
	Backtrace(bt, size, skip);
	std::stringstream ss;
	for(size_t i = 0; i < bt.size(); ++i) {
		ss << prefix << bt[i] << std::endl;
	}

	return ss.str();
}

uint64_t GetFiberId() {
	return sylar::Fiber::GetFiberId();
}

} // namespace sylar
