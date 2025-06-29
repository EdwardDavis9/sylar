#ifndef __SYLAR_CONFIG_H_
#define __SYLAR_CONFIG_H_
#include <memory>
#include <string>
#include <sstream>
#include "util.hh"
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <yaml-cpp/yaml.h>
#include "log.hh"

namespace sylar {

class ConfigVarBase {

  public:
    using ptr = std::shared_ptr<ConfigVarBase>;

    ConfigVarBase(const std::string &name, const std::string &description = "")
        : m_name(name), m_description(description)
    {
        // 将 m_name中的元素 全部引用某个函数，在此处是转换成小写的形式
        // 并写入原地址中
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }

    virtual ~ConfigVarBase() {}

    const std::string &getName() const { return m_name; }
    const std::string &getDescription() const { return m_description; }

    virtual std::string toString()                  = 0;
    virtual bool fromString(const std::string &val) = 0;

  private:
    std::string m_name;
    std::string m_description;
};

template <class T> class ConfigVar : public ConfigVarBase {

  public:
    using ptr = std::shared_ptr<ConfigVar>;

    ConfigVar(const std::string &name, const T &default_value,
              const std::string &description = "")
        : ConfigVarBase(name, description), m_val(default_value)
    {}

    /**
     * @brief 将内容转换成 string 类型
     * @return 出错时，返回"", 否则返还成功转换成的string
     */
    std::string toString() override
    {
        try {
            return boost::lexical_cast<std::string>(m_val);
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                << "<< ConfigVar::toString exception: " << e.what()
                << " convert: " << typeid(m_val).name() << " to string >>";
        }

        return "";
    }

    /**
     * @brief 将传入的string类型 转换成当前类的所属类型, 即赋值操作
     * @param[in] val 传入的待转化的数据
     * @return 出错时，返回false, 否则返回 true
     */
    bool fromString(const std::string &val) override
    {
        try {
            std::cout << "[[debug::Trying to convert string]]: " << val
                      << " to type: " << typeid(T).name() << std::endl;

            m_val = boost::lexical_cast<T>(val);
            return true;
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                << " convert string to " << typeid(m_val).name();
        }
        return false;
    }

    const T getValue() const { return m_val; }
    void setValue(const T &v) { m_val = v; }

  private:
    T m_val;
};

class Config {
  public:
    using ConfigVarMap = std::map<std::string, ConfigVarBase::ptr>;

    /**
     * @berif 查找或创建指定成员
     * @details
     * 在创建指定成员前，会先查找内部数据结构，存在的话，那么直接返回存在的节点，否则就创建当前节点
     * @param[in, out] name : const std::string &, 表示当前键名
     * @param[in, out] default_value : const T&, 表示当前的键值，类型为模板类型
     * @param[in, out] description : const std::string &, 表示当前键名的详细信息
     * @return 返回获得的目标节点信息
     */
    template <class T>
    static typename ConfigVar<T>::ptr
    Lookup(const std::string &name, const T &default_value,
           const std::string &description = "")
    {
        auto tmp = Lookup<T>(name);
        if (tmp) { // 如果存在成员，那么提示已经存在了
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
                << "Lookup name=" << name << " exists";
            return tmp;
        }

        // 这里尝试使用正则来进行检测
        boost::regex error_pat("[^a-zA-Z0-9._]");
        // if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123454678")
        // != std::string::npos)
        if (boost::regex_search(name, error_pat)) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;

            throw std::invalid_argument(name);
        }

        // 创建一个配置实例类
        typename ConfigVar<T>::ptr v(
            // new ConfigVar<T>(name, default_value, description));
            std::make_shared<ConfigVar<T>>(name, default_value, description));

        // 增加一个键值对
        s_datas[name] = v;

        return v;
    }

    template <class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name)
    {
        auto it = s_datas.find(name);
        if (it == s_datas.end()) {
            return nullptr;
        }

        // return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        return std::static_pointer_cast<ConfigVar<T>>(it->second);
        // 使用这个，效率好一点, 编译期进行类型转换
    }

    static void LoadFromYaml(const YAML::Node &root);
    static ConfigVarBase::ptr LookupBase(const std::string &name);

  private:
    static ConfigVarMap s_datas;
};

} // namespace sylar

#endif /* __SYLAR_CONFIG_H_ */
