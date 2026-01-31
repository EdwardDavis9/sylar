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

void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

std::string
BacktraceToString(int size = 64, int skip = 2, const std::string &prefix = "");

// 获取当前的毫秒时间戳
uint64_t GetCurrentMS();

// 获取当前的微秒的时间戳
uint64_t GetCurrentUS();

std::string Time2Str(time_t ts                 = time(0),
                     const std::string &format = "%Y-%m-%d %H:%M:%S");

class FSUtil {
  public:
    static void ListAllFile(std::vector<std::string> &files,
                            const std::string &path,
                            const std::string &subfix);

    static bool MKDir(const std::string &dirname);

    static bool ISRunningPidFile(const std::string &pidfile);

    static bool RM(const std::string &rm_file);
    static bool MV(const std::string &from, const std::string &to);
    static bool RealPath(const std::string &path, const std::string &read_path);
    static bool SymLink(const std::string &from, const std::string &to);
    static bool UnLink(const std::string &file_name, bool exist = false);
    static bool OpenForRead(std::ifstream &ifs,
                            const std::string &file_name,
                            std::ios_base::openmode mode);
    static bool OpenForWrite(std::ofstream &ofs,
                             const std::string &file_name,
                             std::ios_base::openmode mode);

    static std::string Dirname(const std::string &file_name);
    static std::string Basename(const std::string &file_name);
};

} // namespace sylar

#endif /* __SYLAR_UTIL_H__ */
