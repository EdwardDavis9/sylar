#ifndef __SYLAR_ENV_H__
#define __SYLAR_ENV_H__

#include "sylar/singleton.hh"
#include "sylar/thread.hh"
#include <map>
#include <vector>

namespace sylar {

/**
 * @brief Env - 程序运行环境管理类
 *
 * 功能:
 *  - 保存程序运行时的路径, 名称, 工作目录
 *  - 解析命令行参数并存储为 key-value 形式
 *  - 提供访问与修改环境变量的接口
 *  - 支持参数帮助信息输出
 *
 * 典型用途:
 *  1. 启动程序时解析命令行参数
 *  2. 在程序任意地方读取参数或环境变量
 *  3. 打印帮助信息
 */
class Env {
  public:
    using RWMutexType = RWMutex;

    /**
     * @brief  初始化环境(解析命令行)
     * @param  argc 参数个数(来自 main 函数)
     * @param  argv 参数数组(来自 main 函数)
     * @return true 初始化成功
     * @return false 参数解析失败
     *
     * 功能:
     *  - 读取 /proc/self/exe 获取程序路径
     *  - 记录当前工作目录与程序名
     *  - 解析形如 "-config /path -d" 的命令行参数
     */
    bool init(int argc, char **argv);

    /**
     * @brief 添加一个命令行参数键值对
     * @param key 参数名(不含"-")
     * @param val 参数值(可为空字符串)
     *
     * 例如:
     *  add("config", "/etc/app.yaml")
     */
    void add(const std::string &key, const std::string &val);

    /**
     * @brief  判断是否存在指定参数
     * @param  key 参数名
     * @return true 存在
     * @return false 不存在
     */
    bool has(const std::string &key);

    /**
     * @brief 删除某个参数
     */
    void del(const std::string &key);

    /**
     * @brief  获取参数值
     * @param  key 参数名
     * @param  default_value 默认值(如果没找到)
     * @return std::string 参数值
     */
    std::string get(const std::string &key,
                    const std::string &default_value = "");

    /**
     * @brief 添加帮助信息
     * @param key 参数名
     * @param desc 参数说明文字
     *
     * 用于 =--help= 输出友好的说明.
     */
    void addHelp(const std::string &key, const std::string &desc);

    /**
     * @brief 移除帮助信息
     */
    void removeHelp(const std::string &key);

    /**
     * @brief 打印所有帮助信息到日志或标准输出
     */
    void printHelp();

    /**
     * @brief 获取当前可执行文件的完整路径
     *
     * 例如： /xx/xx/xx/test_daemon
     */
    const std::string &getExe() const { return m_exe; };

    /**
     * @brief 获取当前程序所在目录（可执行文件所在路径）
     *
     * 例如： /xx/xx/xx/current_exec_bin_directory/
     */
    const std::string &getCwd() const { return m_cwd; };

    /**
     * @brief  设置系统环境变量
     * @param  key 环境变量名
     * @param  val 环境变量值
     * @return true 设置成功
     *
     * 内部使用 setenv(key.c_str(), val.c_str(), 1)
     */
    bool setEnv(const std::string &key, const std::string &val);

    /**
     * @brief  获取系统环境变量
     * @param  key 环境变量名
     * @param  default_value 默认值（若不存在）
     * @return 环境变量值
     */
    std::string getEnv(const std::string &key,
                       const std::string &default_value = "");

  private:
    RWMutexType m_mutex;

    std::map<std::string, std::string> m_args; /**< 命令行参数的键值对 */
    std::vector<std::pair<std::string, std::string>> m_helps; /**< 帮助信息 */
    std::string m_program; /**< 程序名称 */
    std::string m_exe; /**< 可执行程序的绝对路径 */
    std::string m_cwd; /**< 可执行程序的相对路径 */
};

using EnvMgr = sylar::Singleton<Env>;

}; // namespace sylar

#endif // __SYLAR_ENV_H__
