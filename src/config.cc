#include "config.hh"
#include <sstream>
#include <boost/regex.hpp>

namespace sylar {

Config::ConfigVarMap Config::s_datas;

/**
 * @brief 查找字符串对应的底层结构
 * @param[in] name 待查找的字符串
 * @return nullptr or ConfigVarBase
 */
ConfigVarBase::ptr Config::LookupBase(const std::string &name)
{
    auto it = s_datas.find(name);
    return it == s_datas.end() ? nullptr : it->second;
}

/**
 * @brief 列出所有的节点成员
 * @details 遍历节点，并将节点的内容放到一个list中
 * @param[in] &prefix, 表示当前节点的前置信息
 * @param[in] &node, 当前待遍历的节点
 * @param[in] &output, 存放处理完成信息的list
 */
static void
ListAllMember(const std::string &prefix, const YAML::Node &node,
              std::list<std::pair<std::string, const YAML::Node>> &output)
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
            ListAllMember(prefix.empty() ? it->first.Scalar()
                                         : prefix + "." + it->first.Scalar(),
                          it->second, output);
        }
    }
}

/**
 * @brief 从yaml文件中加载配置
 * @details Description
 * @param[in] root Description
 */
void Config::LoadFromYaml(const YAML::Node &root)
{
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
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
            if (i.second.IsScalar()) { // 如果是最简类型

                // 开始赋值
                var->fromString(i.second.Scalar());
            }
            else { // 如果是复杂嵌套类型
                std::stringstream ss;
                // ss << i.second;
                // var->fromString(ss.str());
                var->fromString(YAML::Dump(i.second));
            }
        }
    }
}

} // namespace sylar
