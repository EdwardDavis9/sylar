#include "sylar/config.hh"
#include "sylar/log.hh"
#include "sylar/util.hh"
#include "sylar/env.hh"

#include <iostream>
#include <yaml-cpp/yaml.h>
// #include "fiber.hh"

sylar::ConfigVar<int>::ptr g_int_value_config =
    sylar::Config::Lookup("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_int_valuex_config =
    sylar::Config::Lookup("systems.port", (float)8080, "system ports");

sylar::ConfigVar<float>::ptr g_float_value_config =
    sylar::Config::Lookup("system.value", (float)10.2f, "system value");

sylar::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config =
    sylar::Config::Lookup("system.int_vec", std::vector<int>{1, 2},
                          "system int_vec");

sylar::ConfigVar<std::list<int>>::ptr g_int_list_value_config =
    sylar::Config::Lookup("system.int_list", std::list<int>{1, 2},
                          "system int list");
sylar::ConfigVar<std::set<int>>::ptr g_int_set_value_config =
    sylar::Config::Lookup("system.int_set", std::set<int>{1, 2},
                          "system int set");

sylar::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config =
    sylar::Config::Lookup("system.int_uset", std::unordered_set<int>{1, 2},
                          "system int uset");

sylar::ConfigVar<std::map<std::string, int>>::ptr g_str_int_map_value_config =
    sylar::Config::Lookup("system.str_int_map",
                          std::map<std::string, int>{{"k", 2}},
                          "system str int map");

sylar::ConfigVar<std::unordered_map<std::string, int>>::ptr
    g_str_int_umap_value_config =
        sylar::Config::Lookup("system.str_int_umap",
                              std::unordered_map<std::string, int>{{"k", 2}},
                              "system str int map");

void testFormatter_1()
{

    sylar::Logger::ptr logger(new sylar::Logger);

    auto formatter = std::make_shared<sylar::LogFormatter>(
        "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n");

    auto std_appender = std::make_shared<sylar::StdoutLogAppender>();
    std_appender->setFormatter(formatter);

    // 1
    // logger->addAppender(sylar::LogAppender::ptr(std_appender));

    // 2
    logger->initFormatter(); // 初始化默认日志格式解析器
    // logger->addAppender(sylar::LogAppender::ptr(std::make_shared<sylar::StdoutLogAppender>()));
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

    sylar::LogEvent::ptr event(new sylar::LogEvent(
        logger->getLevel(), __FILE__, __LINE__, 0, 1, 2, time(0), "test"));

    // sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__, __LINE__, 0, 1,
    // 2, time(0)));
    logger->log(sylar::LogLevel::DEBUG, event);
}

void testFormatter_2()
{

    sylar::Logger::ptr logger(std::make_shared<sylar::Logger>());
    // logger->initFormatter(); // 初始化默认日志格式解析器

    logger->addAppender(
        sylar::LogAppender::ptr(std::make_shared<sylar::StdoutLogAppender>()));

    sylar::FileLogAppender::ptr file_appender(
        std::make_shared<sylar::FileLogAppender>("./log.txt"));
    sylar::LogFormatter::ptr fmt(
        std::make_shared<sylar::LogFormatter>("%d%T%p%T%c%T%f%T:%l%T%m%n"));

    file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::DEBUG); // 设置当前输出器的级别

    logger->addAppender(file_appender);

    SYLAR_LOG_ERROR(logger) << "test macro error";
    SYLAR_LOG_DEBUG(logger) << "test macro debug";
    SYLAR_LOG_INFO(logger) << "test macro info";

    // SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");

    // auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
    // SYLAR_LOG_INFO(l) << "xxx";
}

void print_yaml(const YAML::Node &node, int level)
{
    if (node.IsScalar()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << std::string(level * 4, ' ') << node.Scalar() << " -  "
            << node.Type() << " - " << level;
    }
    else if (node.IsNull()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << std::string(level * 4, ' ') << " NULL - " << node.Type() << " - "
            << level;
    }
    else if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
                << std::string(level * 4, ' ') << it->first << " - "
                << it->second.Type() << " - " << level;

            print_yaml(it->second, level + 1);
        }
    }
    else if (node.IsSequence()) {
        for (size_t i = 0; i < node.size(); ++i) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
                << std::string(level * 4, ' ') << i << " - " << node[i].Type()
                << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml()
{
    YAML::Node root =
        YAML::LoadFile("/home/edward/Code/cc/sylar/bin/conf/log.yaml.bak");

    // print_yaml(root, 0);
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root.Scalar();

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root["test"].IsDefined();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root["tags"].IsDefined();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;
}

void test_config()
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "before g_int_value_config: " << g_int_value_config->getValue();

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "before g_float_value_config: " << g_float_value_config->getValue();

#define XX(g_var, name, prefix)                                              \
    {                                                                        \
        auto &v = g_var->getValue();                                         \
        for (auto i : v) {                                                   \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": " << i; \
        }                                                                    \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())                                     \
            << #prefix " " #name " yaml: " << std::endl                      \
            << g_var->toString(); /*yaml的格式*/                             \
    }

#define XX_M(g_var, name, prefix)                                          \
    {                                                                      \
        auto &v = g_var->getValue();                                       \
        for (auto &i : v) {                                                \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())                               \
                << #prefix " " #name ": {" << i.first << " - " << i.second \
                << "}";                                                    \
        }                                                                  \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())                                   \
            << #prefix " " #name " yaml: " << g_var->toString();           \
    }

#if 1

    XX(g_int_vec_value_config, int_vec, Before change);
    XX(g_int_list_value_config, int_list, Before change);
    XX(g_int_set_value_config, int_set, Before change);
    XX(g_int_uset_value_config, int_uset, Before change);
    XX_M(g_str_int_map_value_config, str_int_map, Before change);
    XX_M(g_str_int_umap_value_config, str_int_umap, Before change);

#endif

#if 1
    YAML::Node root =
        // YAML::LoadFile("/home/edward/Code/cc/sylar/bin/conf/log.yaml");
        YAML::LoadFile("/home/edward/Code/cc/sylar/bin/conf/test.yaml");
    sylar::Config::LoadFromYaml(root);

    // // 验证每个变量
    // std::cout << g_int_value_config->getValue() << std::endl;   // 是否等于
    // 8080 std::cout << g_float_value_config->getValue() << std::endl; //
    // 是否等于 10.2

    // for (auto &x : g_int_vec_value_config->getValue()) {
    //         SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::to_string(x);
    // }
    // std::cout << x << " "; // 是否等于 YAML 中的 1,2,...

    // for (auto &x : g_int_list_value_config->getValue())
    //     std::cout << x << " "; // 是否等于 YAML 中的 1,2,...
    // std::cout << std::endl;

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "After: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "After: " << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, After change:);
    XX(g_int_list_value_config, int_list, After change:);
    XX(g_int_set_value_config, int_set, After change:);
    XX(g_int_uset_value_config, int_uset, After change:);
    XX_M(g_str_int_map_value_config, str_int_map, After change:);
    XX_M(g_str_int_umap_value_config, str_int_umap, After change:);
#endif
}

class Person {
  public:
    Person() {}
    std::string m_name = "default";
    int m_age          = 0;
    bool m_sex         = 0;

    std::string toString() const
    {
        std::stringstream ss;
        ss << "[Person name=" << m_name << " age=" << m_age << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person &oth) const
    {
        return m_name == oth.m_name && m_age == oth.m_age && m_sex == oth.m_sex;
    }
};

namespace sylar {

template <>
class LexicalCast<std::string, Person> {
  public:
    // configVar 调用 toStr 函数
    Person operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age  = node["age"].as<int>();
        p.m_sex  = node["sex"].as<bool>();

        return p;
    }
};

template <>
class LexicalCast<Person, std::string> {
  public:
    std::string operator()(const Person &v)
    {
        YAML::Node node;
        node["name"] = v.m_name;
        node["age"]  = v.m_age;
        node["sex"]  = v.m_sex;
        std::stringstream ss;
        // ss << "attribute: " << node["name"] << "," << node["age"] << ", " <<
        // node["sex"];
        ss << node;

        return ss.str();
    }
};
}; // namespace sylar

sylar::ConfigVar<Person>::ptr g_person =
    sylar::Config::Lookup("class.person", Person(), "system person");

sylar::ConfigVar<std::map<std::string, Person>>::ptr g_person_map =
    sylar::Config::Lookup("class.map", std::map<std::string, Person>(),
                          "system person");

sylar::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr
    g_person_vec_map =
        sylar::Config::Lookup("class.vec_map",
                              std::map<std::string, std::vector<Person>>(),
                              "system person");

void test_class()
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "before: " << g_person->getValue().toString() << "\n"
        << g_person->toString() << std::endl;

#define XX_PM(g_var, prefix)                                                   \
    {                                                                          \
        auto m = g_var->getValue();                                            \
        for (auto &i : m) {                                                    \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())                                   \
                << prefix << " : " << i.first << " - " << i.second.toString(); \
        }                                                                      \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << prefix << " : size=" << m.size();  \
    }

    g_person->addListener([](const Person &old_value, const Person &new_value) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << "++++ old_value=" << old_value.toString()
            << " new_value=" << new_value.toString() << " ++++";
    });

    XX_PM(g_person_map, "class.map before");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "class_vec_map before: " << g_person_vec_map->toString()
        << g_person_vec_map->getValue().size();

    YAML::Node node =
        YAML::LoadFile("/home/edward/Code/cc/sylar/bin/conf/test.yaml");
    sylar::Config::LoadFromYaml(node);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "after: " << g_person->getValue().toString() << "\n"
        << g_person->toString();

    XX_PM(g_person_map, "class.map after");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after:\n"
                                     << g_person_vec_map->toString();
}

void test_logs()
{
    static sylar::Logger::ptr system_log = SYLAR_LOG_NAME("system");
    SYLAR_LOG_INFO(system_log) << "hello system" << std::endl;
    std::cout << sylar::LoggerMgr::GetInstance()->toYamlString()
              << "getInstance-toYamlString" << std::endl;
    YAML::Node root =
        YAML::LoadFile("/home/edward/Code/cc/sylar/bin/conf/test.yaml");
    sylar::Config::LoadFromYaml(root);
    std::cout << "============" << std::endl;
    std::cout << sylar::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << root << std::endl;
    SYLAR_LOG_INFO(system_log) << "hello system" << std::endl;

    system_log->setFormatter("%d%T%f%T%l%T%m%n");
    SYLAR_LOG_INFO(system_log) << "hello system" << std::endl;
}

void test_loadConf() { sylar::Config::LoadFromConfDir("../conf"); }

int test_load(int argc, char **argv)
{
    sylar::EnvMgr::GetInstance()->init(argc, argv);
    test_loadConf();
    std::cout << " ===== " << std::endl;
    sleep(10);
    test_loadConf();
    return 0;
}

int main(int argc, char *argv[])
{
    // testFormatter_1();
    // testFormatter_2();
    // test_yaml();

    // test_config();

    // test_class();

    // test_logs();

    // std::cout << "-------------------------"<< std::endl;

#if 1
    sylar::Config::Visit([](sylar::ConfigVarBase::ptr var) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << "name=" << var->getName() << " description="
            << var->getDescription()
            // << " typename=" << var->getTypeName()
            << " value=" << var->toString();
    });

#endif

    // test_loadConf();
    // test_load(argc, argv);

    return 0;
}
