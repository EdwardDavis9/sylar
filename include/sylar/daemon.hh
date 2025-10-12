#ifndef __SYLAR_DAEMON_H__
#define __SYLAR_DAEMON_H__

#include <unistd.h>
#include <functional>
#include <stdint.h>
#include "sylar/singleton.hh"

namespace sylar {

/**
 * @struct ProcesInfo
 * 进程信息
 */
struct ProcessInfo {
    pid_t parent_id            = 0; /**< 父进程 ID */
    uint64_t parent_start_time = 0; /**< 父进程启动时间 */
    pid_t main_id              = 0; /**< 主进程 ID */
    uint32_t main_start_time   = 0; /**< 主进程启动时间 */
    uint32_t restart_count     = 0; /**< 主进程重启次数 */

    std::string toString() const;
};

typedef sylar::Singleton<ProcessInfo> ProcessInfoMgr;

/**
 * @brief      启动程序
 * @param[in]  argc 参数个数
 * @param[out] argv 参数数组
 * @param[in]  is_daemon 是否是守护进程
 * @return     返回程序的执行结果
 */
int start_daemon(int argc,
                 char **argv,
                 std::function<int(int argc, char **argv)> main_cb,
                 bool is_daemon);

}; // namespace sylar

#endif // __SYLAR_DAEMON_H__
