#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>

#include "sylar/fiber.hh"

namespace sylar {

pid_t GetThreadId();

uint64_t GetFiberId();

void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

// 获取当前的毫秒的时间戳
uint64_t GetCurrentMS();

// 获取当前的微秒的时间戳
uint64_t GetCurrentUS();

} // namespace sylar

#endif /* __SYLAR_UTIL_H__ */
