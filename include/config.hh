#ifndef __SYLAR_CONFIG_H_
#define __SYLAR_CONFIG_H_
#include <memory>
#include <string>
#include <sstream>
#include "util.hh"
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <list>
#include <yaml-cpp/yaml.h>
#include "log.hh"
#include <set>
#include <unordered_set>
#include <map>
#include <functional>

#include "thread.hh"
#include "log.hh"

namespace sylar {

/**
 * @brief 配置变量的抽象基类，用于定义通用接口。
 *
 * 该类提供配置项的名称、描述以及字符串序列化与反序列化接口。
 */
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
    virtual std::string getTypeName() const = 0;

    virtual std::string toString()                  = 0;
    virtual bool fromString(const std::string &val) = 0;

  private:
    std::string m_name;
    std::string m_description;
};

/**
 * @berif 转换类型
 * @details 模板转换，通过lexical_cast 直接将输入类型转换
 *
 * @params[in. out] F 源类型
 * @params[in. out] T 目标类型
 * @return 指定类型
 */
template <class F, class T>
class LexicalCast {
  public:
    T operator()(const F &v) { return boost::lexical_cast<T>(v); }
};

/**
 * @berif string2vector
 */
template <class T>
class LexicalCast<std::string, std::vector<T>> {
  public:
    std::vector<T> operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        // typename std::vector<T> vec;
        typename std::vector<T> vec;
        std::stringstream ss;

        // 写入 stringstream 中，然后 push_back 进 vec 中
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief vector2string
 */
template <class T>
class LexicalCast<std::vector<T>, std::string> {
  public:
    std::string operator()(const std::vector<T> &v)
    {
        YAML::Node node;

        // push 进 node 节点中，然后直接读入到 stringstream 中
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @berif string2list
 */
template <class T>
class LexicalCast<std::string, std::list<T>> {
  public:
    std::list<T> operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        std::stringstream ss;
        typename std::list<T> vec;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief list2string
 */
template <class T>
class LexicalCast<std::list<T>, std::string> {
  public:
    std::string operator()(const std::list<T> &v)
    {
        YAML::Node node;

        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @berif string2set
 */
template <class T>
class LexicalCast<std::string, std::set<T>> {
  public:
    std::set<T> operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        std::stringstream ss;
        typename std::set<T> vec;

        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }

        return vec;
    }
};

/**
 * @berif set2string
 */
template <class T>
class LexicalCast<std::set<T>, std::string> {
  public:
    std::string operator()(const std::set<T> &v)
    {
        YAML::Node node;
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;

        return ss.str();
    };
};

/**
 * string2unordered_set
 */
template <class T>
class LexicalCast<std::string, std::unordered_set<T>> {

  public:
    std::unordered_set<T> operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @berif unordered_set2string
 */
template <class T>
class LexicalCast<std::unordered_set<T>, std::string> {
  public:
    std::string operator()(const std::unordered_set<T> &v)
    {
        YAML::Node node;
        std::stringstream ss;
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        ss << node;
        return ss.str();
    }
};

/**
 * @berif string2map
 */
template <class T>
class LexicalCast<std::string, std::map<std::string, T>> {
  public:
    std::map<std::string, T> operator()(const std::string &v)
    {

        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> vec;

        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                                      LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

/**
 * @berif map2string
 */
template <class T>
class LexicalCast<std::map<std::string, T>, std::string> {
  public:
    std::string operator()(const std::map<std::string, T> &v)
    {
        YAML::Node node;
        for (auto &i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * string2unordered_map
 */
template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
  public:
    std::unordered_map<std::string, T> operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;

        for (auto i = node.begin(); i != node.end(); ++i) {
            ss.str("");
            ss << i->second;
            vec.insert(std::make_pair(i->first.Scalar(),
                                      LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};
/**
 * unordered_map2string
 */
template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {

  public:
    std::string operator()(const std::unordered_map<std::string, T> &v)
    {
        YAML::Node node;
        for (auto &i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// FROMSTRING T operator(const std::string&);
// TOString T operator() (const T&);
template <class T, class FromStr = LexicalCast<std::string, T>,
          class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {

  public:
    using RWMutexType = RWMutex;
    using ptr = std::shared_ptr<ConfigVar>;
    using on_change_cb =
        std::function<void(const T &old_value, const T &new_value)>;

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
            // return boost::lexical_cast<std::string>(m_val);
            RWMutexType::ReadLock lock(m_mutex);
            return ToStr()(m_val);
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
            setValue(FromStr()(val));
            return true;
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                << "ConfigVar::toString exception" << e.what()
                << " convert string to " << typeid(m_val).name()
                << "- " << val;
        }
        return false;
    }

    const T getValue() {
        RWMutex::ReadLock lock(m_mutex);
        return m_val;
    }
    void setValue(const T &v)
    {
        {
            RWMutex::ReadLock lock(m_mutex);
            if (v == m_val) {
                return;
            }
            for (auto &i: m_cbs) { // 在设置变量的时候，进行函数的回调，提示属性修改
                i.second(m_val, v);
            }
        }
        RWMutex::WriteLock lock(m_mutex);
        m_val = v;
    }

    std::string getTypeName() const override { return typeid(T).name(); };

    uint64_t addListener(on_change_cb cb) {
        static uint64_t s_fun_id = 0;
        RWMutexType::WriteLock writeLock(m_mutex);
        ++s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }

    void delListener(uint64_t key) {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }

    void clearListener() {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }

    on_change_cb getListener(uint64_t key)
    {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }

  private:
    T m_val;
    RWMutexType m_mutex;

    // 变更回调函数，key 要唯一，用map将回调函数包装，可以方便的添加或删除
    std::map<uint64_t, on_change_cb> m_cbs;
};

class Config {
  public:
    using RWMutexType = RWMutex;
    using ConfigVarMap = std::unordered_map<std::string, ConfigVarBase::ptr>;

    /**
     * @berif 查找或创建指定成员
     * @details
     * 在创建指定成员前，会先查找内部数据结构，存在的话，
     * 那么直接返回存在的节点，否则就创建当前节点
     * @param[in, out] name : const std::string &, 表示当前键名
     * @param[in, out] default_value : const T&,
     * 表示当前的键值，类型为模板类型
     * @param[in, out] description : const std::string &,
     * 表示当前键名的详细信息
     * @return 返回获得的目标节点信息
     */
    template <class T>
    static typename ConfigVar<T>::ptr
    Lookup(const std::string &name, const T &default_value,
           const std::string &description = "")
    {
        // 对重复出现的键值进行额外处理
        RWMutexType::WriteLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if (it != GetDatas().end()) {
            auto tmp =
                std::dynamic_pointer_cast<ConfigVar<T>>( it->second);
            // 向子类转换

            if (tmp) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
                    << "Lookup name=" << name << " exists";
                return tmp;
            }
            else { // 类型转换失败时
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                    << "Lookup name=" << name << " exit but type "
                    << typeid(T).name()
                    << " real_type=" << it->second->getTypeName() << " "
                    << it->second->toString();
                return nullptr;
            }
        }

        // 这里尝试使用正则来进行检测
        boost::regex error_pat("[^a-zA-Z0-9._]");
        // if
        // (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123454678")
        // != std::string::npos)
        if (boost::regex_search(name, error_pat)) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;

            throw std::invalid_argument(name);
        }

        // 创建一个配置实例类
        typename ConfigVar<T>::ptr v(
            // new ConfigVar<T>(name, default_value, description));
            std::make_shared<ConfigVar<T>>(name, default_value, description));

        // 增加一个键值对：name:ConfiVar
        GetDatas()[name] = v;

        return v;
    }

    template <class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name)
    {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if (it == GetDatas().end()) {
            return nullptr;
        }

        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        // return std::static_pointer_cast<ConfigVar<T>>(it->second);
        // 使用这个，效率好一点, 编译期进行类型转换
    }

    static void LoadFromYaml(const YAML::Node &root);
    static ConfigVarBase::ptr LookupBase(const std::string &name);

    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

  private:
    static ConfigVarMap &GetDatas()
    {
        static ConfigVarMap s_datas;
        return s_datas;
    }

    static RWMutexType& GetMutex() {
        static RWMutexType s_mutex;
        return s_mutex;
    }
};

} // namespace sylar

#endif /* __SYLAR_CONFIG_H_ */
