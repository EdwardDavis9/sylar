#include "sylar/config.hh"
#include "sylar/env.hh"

#include <sstream>
#include <boost/regex.hpp>
#include <sys/stat.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

ConfigVarBase::ptr Config::LookupBase(const std::string &name)
{
    RWMutex::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

/**
 * @brief     列出所有的节点成员
 * @details   遍历节点, 并将节点的内容放到一个list中
 * @param[in] &prefix, 表示当前节点的前置信息
 * @param[in] &node, 当前待遍历的节点
 * @param[in] &output, 存放处理完成信息的list
 */
static void
ListAllMember(const std::string &prefix, const YAML::Node &node,
              std::list<std::pair<std::string, const YAML::Node>> &output)
{
    boost::regex error_pattern("[^a-zA-Z0-9._]");

    if (boost::regex_search(prefix, error_pattern)) {
        SYLAR_LOG_ERROR(g_logger)
            << "Config invalid name: " << prefix << " : " << YAML::Dump(node);
        return;
    }

    // 注意, 这里可能将 prefix 为空的情况, 即顶层节点添加进去
    // 后续使用需要判断: key->first.empty()
    output.push_back(std::make_pair(prefix, node));

    if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            ListAllMember(prefix.empty() ? it->first.Scalar()
                                         : prefix + "." + it->first.Scalar(),
                          it->second, output);
        }
    }
}

void Config::LoadFromYaml(const YAML::Node &root)
{
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;

    // 递归加载配置节点到容器
    ListAllMember("", root, all_nodes);

    for (auto &i : all_nodes) {
        std::string key = i.first;
        if (key.empty()) {
            continue;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        // 通过给定的 yaml文件中的 string,  找与之相应的底层结构
        // 来判断是需要重新调整属性
        ConfigVarBase::ptr var = LookupBase(key);

        if (var) {
            if (i.second.IsScalar()) {
                var->fromString(i.second.Scalar());
            } else {
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

static std::map<std::string, uint64_t> S_FileModifyTime;
static sylar::Mutex s_mutex;

auto Config::LoadFromConfDir(const std::string &path) -> void
{
    std::string absolute_path =
        sylar::EnvMgr::GetInstance()->getAbsolutePath(path);
    std::vector<std::string> files;
    FSUtil::ListAllFile(files, absolute_path, ".yaml");

    for (auto &i : files) {
        {
            // 简化起见, 只判断访问时间, 来判断文件是否需要重新加载
            // 严格上通过比对 md5 更好
            struct stat st;
            lstat(i.c_str(), &st);
            sylar::Mutex::Lock lock(s_mutex);
            if (S_FileModifyTime[i] == (uint64_t)st.st_mtime) {
                continue;
            }
            S_FileModifyTime[i] = st.st_mtime;
        }
        try {
            YAML::Node root = YAML::LoadFile(i);
            LoadFromYaml(root);
            SYLAR_LOG_INFO(g_logger) << "LoadConfFile file=" << i << " ok";
        } catch (...) {
            SYLAR_LOG_ERROR(g_logger) << "LoadConfFile file=" << i << " failed";
        }
    }
}

// 访问者模式
void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb)
{
    RWMutexType::ReadLock lock(GetMutex());

    ConfigVarMap &m = GetDatas();
    for (auto it = m.begin(); it != m.end(); ++it) {
        cb(it->second);
    }
}
} // namespace sylar
