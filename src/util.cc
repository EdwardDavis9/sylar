#include <execinfo.h>

#include "sylar/util.hh"
#include "sylar/log.hh"
#include "sylar/fiber.hh"

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>

#include <iostream>

namespace sylar {

auto FSUtil::ListAllFile(std::vector<std::string> &files,
                         const std::string &path, const std::string &subfix)
    -> void
{
    if (access(path.c_str(), F_OK) != 0) {
        return;
    }
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return;
    }
    struct dirent *dp = nullptr;
    while ((dp = readdir(dir)) != nullptr) {
        if (dp->d_type == DT_DIR) {
            if (!(strcmp(dp->d_name, ".")) || !strcmp(dp->d_name, "..")) {
                continue;
            }
            ListAllFile(files, path + "/" + dp->d_name, subfix);
        }
        else if (dp->d_type == DT_REG) {
            std::string file_name(dp->d_name);
            if (subfix.empty()) {
                // 若后缀为空, 那么存放所有文件
                files.push_back(path + "/" + file_name);
            }
            else {
                if (file_name.size() < subfix.size()) {
                    continue;
                }
                if (file_name.substr(file_name.length() - subfix.size())
                    == subfix)
                {
                    files.push_back(path + "/" + file_name);
                }
            }
        }
    }

    closedir(dir);
}
static int __LStat(const char *file, struct stat *st = nullptr)
{
    struct stat lst;
    int ret = lstat(file, &lst);
    if (st) {
        *st = lst;
    }
    return ret;
}

static int __MkDir(const char *dirname)
{
    if (access(dirname, F_OK) == 0) {
        return 0;
    }
    return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

auto FSUtil::MKDir(const std::string &dirname) -> bool
{
    if (__LStat(dirname.c_str()) == 0) {
        return true;
    }
    char *path = strdup(dirname.c_str());

    // 定位第二个路径分隔符，如果路径是以根开头的话，第一个就是根目录，因此需要过滤
    // 如果传入的路径不是根目录的话，或者说不是以根目录开头的话，可能会存在问题
    char *ptr  = strchr(path + 1, '/');

    for (; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/')) {

        // 临时使用\0手动截断, 在循环时, 然后恢复这个截断的内容
        *ptr = '\0';
        if (__MkDir(path) != 0) {
            break;
        }
    }

    // ptr指针不为空, 或者最后一级目录创建失败的话
    if (ptr != nullptr || __MkDir(path) != 0) {
        free(path);
        return false;
    }
    free(path);
    return true;
}

auto FSUtil::ISRunningPidFile(const std::string &pidfile) -> bool
{
    // 读取 pidfile, 然后向指定 pid 发送 0 号信号来判断进程是否存在
    if ((__LStat(pidfile.c_str())) != 0) {
        return false;
    }
    std::ifstream ifs(pidfile);
    std::string line;
    if (!ifs || !std::getline(ifs, line)) {
        return false;
    }
    if (line.empty()) {
        return false;
    }
    pid_t pid = atoi(line.c_str());
    if (pid <= 1) {
        return false;
    }
    if (kill(pid, 0) != 0) {
        return false;
    }
    return true;
}

std::string Time2Str(time_t ts, const std::string &format)
{
    struct tm tm;
    localtime_r(&ts, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    return buf;
}

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

pid_t GetThreadId() { return syscall(SYS_gettid); }

void Backtrace(std::vector<std::string> &bt, int size, int skip)
{
    void **array = static_cast<void **>(malloc(sizeof(void *) * size));
    size_t s     = ::backtrace(array, size);

    char **strings = backtrace_symbols(array, s);
    if (strings == NULL) {
        SYLAR_LOG_ERROR(g_logger) << "backtrace_symbols error";
        return;
    }

    for (size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
}

std::string BacktraceToString(int size, int skip, const std::string &prefix)
{
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for (size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }

    return ss.str();
}

uint64_t GetFiberId() { return sylar::Fiber::GetFiberId(); }

uint64_t GetCurrentMS()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    // 秒 -> 毫秒 <- 微秒
    // 将秒变成毫秒
    // 微秒变成毫秒
    return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

uint64_t GetCurrentUS()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    // 秒 -> 毫秒 -> 微秒
    return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}

bool OpenForRead(std::ifstream &ifs, const std::string &file_name,
                 std::ios_base::openmode mode)
{
    ifs.open(file_name.c_str(), mode);
    return ifs.is_open();
}

auto FSUtil::OpenForWrite(std::ofstream &ofs, const std::string &file_name,
                          std::ios_base::openmode mode) -> bool
{
    ofs.open(file_name.c_str(), mode);
    if (!ofs.is_open()) {
        std::string dir = Dirname(file_name);
        MKDir(dir);
        ofs.open(file_name.c_str(), mode);
    }
    return ofs.is_open();
}

std::string FSUtil::Dirname(const std::string &file_name)
{
    if (file_name.empty()) {
        return ".";
    }
    auto pos = file_name.rfind('/');
    if (pos == 0) {
        return "/";
    }
    else if (pos == std::string::npos) {
        return ".";
    }
    else {
        return file_name.substr(0, pos);
    }
}

std::string FSUtil::Basename(const std::string &file_name)
{
    if (file_name.empty()) {
        return file_name;
    }
    auto pos = file_name.rfind('/');
    if (pos == std::string::npos) {
        return file_name;
    }
    else {
        return file_name.substr(pos + 1);
    }
}
} // namespace sylar
