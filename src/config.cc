#include "sylar/config.hh"
#include <sstream>
#include <boost/regex.hpp>

namespace sylar {

ConfigVarBase::ptr Config::LookupBase(const std::string &name)
{
    RWMutex::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

/**
 * @brief     列出所有的节点成员
 * @details   遍历节点，并将节点的内容放到一个list中
 * @param[in] &prefix, 表示当前节点的前置信息
 * @param[in] &node, 当前待遍历的节点
 * @param[in] &output, 存放处理完成信息的list
 */
static void
ListAllMember(const std::string &prefix,
              const YAML::Node &node,
              std::list<std::pair<std::string,
              const YAML::Node>> &output)
{
    boost::regex error_pattern("[^a-zA-Z0-9._]");

    // if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456578")
    //     != std::string::npos)
    if (boost::regex_search(prefix, error_pattern)) {
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
            << "Config invalid name: " << prefix << " : " << YAML::Dump(node);
        return;
    }

    // 注意，这里可能将 prefix 为空的情况，即顶层节点添加进去
    // 后续使用需要判断: key->first.empty()
    output.push_back(std::make_pair(prefix, node));

    if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            ListAllMember(prefix.empty()
                          ? it->first.Scalar()
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

        // 通过给定的 yaml文件中的 string， 找到与之相应的底层结构
        ConfigVarBase::ptr var = LookupBase(key);

        if (var) {
            if (i.second.IsScalar()) {
                var->fromString(i.second.Scalar());
            }
            else {
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }

    }
}

// 访问者模式
void Config::Visit(std::function<void (ConfigVarBase::ptr)> cb)  {
    RWMutexType::ReadLock lock(GetMutex());

    ConfigVarMap& m = GetDatas();
    for(auto it = m.begin(); it != m.end(); ++it) {
        cb(it->second);
    }
}

} // namespace sylar
