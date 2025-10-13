#include "sylar/env.hh"
#include "sylar/log.hh"
#include <unistd.h>
#include <string.h>
// #include <iostream>
#include <iomanip>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

auto Env::init(int argc, char **argv) -> bool
{
    char link[1024] = {0};
    char path[1024] = {0};
    sprintf(link, "/proc/%d/exe", getpid()); //拼接出内核中的链接路径
    readlink(link, path, sizeof(path)); // 获得内核中链接对应的实际路径

    m_exe = path;

    auto pos = m_exe.find_last_of("/");
    m_cwd    = m_exe.substr(0, pos) + "/";

    m_program = argv[0];

    const char *now_key = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (strlen(argv[i]) > 1) {
                if (now_key) {
                    add(now_key, ""); // 保存上一轮的参数
                }
                now_key = argv[i] + 1;
            }
            else {
                SYLAR_LOG_ERROR(g_logger)
                    << "invaild arg idx=" << i << " val=" << argv[i];
                return false;
            }
        }
        else {
            if (now_key) {
                add(now_key, argv[i]);
                now_key = nullptr;
            }
            else {
                SYLAR_LOG_ERROR(g_logger)
                    << "invaild arg idx=" << i << " val=" << argv[i];
                return false;
            }
        }
    }

    if (now_key) {
        add(now_key, "");
    }

    return true;
}

auto Env::add(const std::string &key, const std::string &val) -> void
{
    RWMutexType::WriteLock lock(m_mutex);
    m_args[key] = val;
}

auto Env::del(const std::string &key) -> void
{
    RWMutexType::WriteLock lock(m_mutex);
    m_args.erase(key);
}

auto Env::has(const std::string &key) -> bool
{
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end();
}

auto Env::get(const std::string &key, const std::string &default_value)
    -> std::string
{
    RWMutexType::WriteLock lock(m_mutex);
    auto it = m_args.find(key);
    return it == m_args.end() ? it->second : default_value;
}

auto Env::addHelp(const std::string &key, const std::string &desc) -> void
{
    removeHelp(key);
    RWMutexType::WriteLock lock(m_mutex);
    m_helps.push_back(std::make_pair(key, desc));
}

auto Env::removeHelp(const std::string &key) -> void
{
    RWMutexType::WriteLock lock(m_mutex);
    for (auto it = m_helps.begin(); it != m_helps.end();) {
        if (it->first == key) {
            it = m_helps.erase(it);
        }
        else {
            ++it;
        }
    }
}

auto Env::printHelp() -> void
{
    RWMutexType::ReadLock lock(m_mutex);
    std::cout << "Usage: " << m_program << " [Options]" << std::endl;
    for (auto &i : m_helps) {
        std::cout << std::setw(5) << "-" << i.first << " : " << i.second
                  << std::endl;
    }
}

auto Env::setEnv(const std::string &key, const std::string &val) -> bool
{
    return !setenv(key.c_str(), val.c_str(), 1);
}

auto Env::getEnv(const std::string &key, const std::string &default_value)
    -> std::string
{
    const char *v = getenv(key.c_str());
    if (v == nullptr) {
        return default_value;
    }
    return v;
}

}; // namespace sylar
