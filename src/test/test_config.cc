#include "sylar/config.hh"
#include "sylar/log.hh"

#include <yaml-cpp/yaml.h>

#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "sylar/env.hh"
// #include "fiber.hh"

sylar::ConfigVar<int>::ptr g_int_value_config =
    sylar::Config::Lookup("system.port", (int)8080, "test: system int port");

sylar::ConfigVar<float>::ptr g_int_valuex_config = sylar::Config::Lookup(
    "systems.port", (float)8080, "test: system float ports");

sylar::ConfigVar<float>::ptr g_float_value_config = sylar::Config::Lookup(
    "system.value", (float)10.2f, "test: system float value");

sylar::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config =
    sylar::Config::Lookup("system.int_vec", std::vector<int>{1, 2},
                          "test: system int_vec");

sylar::ConfigVar<std::list<int>>::ptr g_int_list_value_config =
    sylar::Config::Lookup("system.int_list", std::list<int>{1, 2},
                          "test: system int list");

sylar::ConfigVar<std::set<int>>::ptr g_int_set_value_config =
    sylar::Config::Lookup("system.int_set", std::set<int>{1, 2},
                          "test: system int set");

sylar::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config =
    sylar::Config::Lookup("system.int_uset", std::unordered_set<int>{1, 2},
                          "test: system int uset");

sylar::ConfigVar<std::map<std::string, int>>::ptr g_str_int_map_value_config =
    sylar::Config::Lookup("system.str_int_map",
                          std::map<std::string, int>{{"k", 2}},
                          "test: system str int map");

sylar::ConfigVar<std::unordered_map<std::string, int>>::ptr
    g_str_int_umap_value_config =
        sylar::Config::Lookup("system.str_int_umap",
                              std::unordered_map<std::string, int>{{"k", 2}},
                              "test: system str int map");

void test_yaml()
{
    YAML::Node root =
        YAML::LoadFile("/home/edward/Code/cc/sylar/bin/conf/log.yaml.bak");

    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root.IsMap();

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root["test"].IsDefined(); // 0
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root["tags"].IsDefined(); // 0
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root["logs"].IsDefined(); // 1
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;
}

void test_config()
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "test: before g_int_value_config: "
                                     << g_int_value_config->getValue();

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "test: before g_float_value_config: "
                                     << g_float_value_config->getValue();

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

    XX(g_int_vec_value_config, int_vec, Before change);
    XX(g_int_list_value_config, int_list, Before change);
    XX(g_int_set_value_config, int_set, Before change);
    XX(g_int_uset_value_config, int_uset, Before change);
    XX_M(g_str_int_map_value_config, str_int_map, Before change);
    XX_M(g_str_int_umap_value_config, str_int_umap, Before change);
#if 0

#endif

    YAML::Node root =
        // YAML::LoadFile("/home/edward/Code/cc/sylar/bin/conf/log.yaml");
        YAML::LoadFile("/home/edward/Code/cc/sylar/bin/conf/test.yaml.bak");
    sylar::Config::LoadFromYaml(root);

    // 验证每个变量
    // std::cout << g_int_value_config->getValue() << std::endl;      //
    // 是否等于 8080 std::cout << g_float_value_config->getValue() << std::endl;
    // // 是否等于 10.2
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "Before 8080, After: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "Before 10.2, After: " << g_float_value_config->toString();

    for (auto &x : g_int_vec_value_config->getValue()) {
        // 是否等于 YAML 中的 1,2,...
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::to_string(x);
    }

    for (auto &x : g_int_list_value_config->getValue())
        std::cout << x << " "; // 是否等于 YAML 中的 1,2,...

    std::cout << std::endl; // 是否等于 YAML 中的 1,2,...

    XX(g_int_vec_value_config, int_vec, After change:);
    XX(g_int_list_value_config, int_list, After change:);
    XX(g_int_set_value_config, int_set, After change:);
    XX(g_int_uset_value_config, int_uset, After change:);
    XX_M(g_str_int_map_value_config, str_int_map, After change:);
    XX_M(g_str_int_umap_value_config, str_int_umap, After change:);
#if 0
#endif
}

class Person {
  public:
    Person() {}
    Person(const std::string &name, int age, bool sex)
        : m_name(name), m_age(age), m_sex(sex)
    {}

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

    std::string m_name = "default";
    int m_age          = 0;
    bool m_sex         = 0;
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
    sylar::Config::Lookup("class.person", Person(), "test: system person");

sylar::ConfigVar<std::map<std::string, Person>>::ptr g_person_map_null =
    sylar::Config::Lookup("class.map.null", std::map<std::string, Person>(),
                          "test: system map");

std::map<std::string, Person> create_person_map()
{
    std::map<std::string, Person> m;
    Person p1;
    p1.m_name  = "Alice";
    p1.m_age   = 25;
    p1.m_sex   = true;
    m["alice"] = p1;

    Person p2;
    p2.m_name = "Bob";
    p2.m_age  = 30;
    p2.m_sex  = false;
    m["bob"]  = p2;

    return m;
}

sylar::ConfigVar<std::map<std::string, Person>>::ptr g_person_vec_not_null =
    sylar::Config::Lookup("class.map.not.null", create_person_map(),
                          "test: system map");

sylar::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr
    g_person_vec_null =
        sylar::Config::Lookup("class.vec.null",
                              std::map<std::string, std::vector<Person>>(),
                              "test: system vec_map");

std::map<std::string, std::vector<Person>> person_map = {
    {"people", {Person("Alice", 25555, true), Person("Bob", 3000, false)}}};
sylar::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_person_vec =
    sylar::Config::Lookup("class.map", person_map, "test: system map");

std::map<std::string, std::vector<Person>> create_person_vec()
{
    std::map<std::string, std::vector<Person>> person_map;
    person_map["people"].push_back(Person("Alice", 25111, true));
    person_map["people"].push_back(Person("Bob", 30111, false));

    return person_map;
}
sylar::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_person_map =
    sylar::Config::Lookup("class.vec.not.null", create_person_vec(),
                          "test: system vec_map");

void test_class()
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "before: " << g_person->getValue().toString() << "\n"
        << g_person->toString() << std::endl;

    // define XX_PM(g_var, prefix)
    //     {
    //         auto m = g_var->getValue();
    //         for (auto &i : m) {
    //             SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
    //                 << prefix << " : " << i.first << " - " <<
    //                 i.second.toString();
    //         }
    //         SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << prefix << " : size=" <<
    //         m.size();
    //     }

#define XX_PM(g_var, prefix)                                                  \
    {                                                                         \
        auto m = g_var->getValue();                                           \
        for (auto &i : m) {                                                   \
            std::ostringstream oss;                                           \
            oss << prefix << " : " << i.first << " - ";                       \
            for (auto &p : i.second) {                                        \
                oss << p.toString() << " ";                                   \
            }                                                                 \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << oss.str();                    \
        }                                                                     \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << prefix << " : size=" << m.size(); \
    }

    g_person->addListener([](const Person &old_value, const Person &new_value) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << "++++ old_value=" << old_value.toString()
            << " new_value=" << new_value.toString() << " ++++";
    });

    XX_PM(g_person_map, "class.map before");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "class_vec_map before: " << g_person_map->toString()
        << g_person_map->getValue().size();

    YAML::Node node =
        YAML::LoadFile("/home/edward/Code/cc/sylar/bin/conf/test.yaml.bak");
    sylar::Config::LoadFromYaml(node);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "after: " << g_person->getValue().toString() << "\n"
        << g_person->toString();

    XX_PM(g_person_map, "class.map after");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after:\n" << g_person_map->toString();
#if 0
#endif
}

void test_logs()
{
    static sylar::Logger::ptr system_log = SYLAR_LOG_NAME("system");
    SYLAR_LOG_INFO(system_log) << "hello system" << std::endl;
    std::cout << sylar::LoggerMgr::GetInstance()->toYamlString()
              << ", getInstance-toYamlString" << std::endl;
    std::cout << "============" << std::endl;
    YAML::Node root =
        YAML::LoadFile("/home/edward/Code/cc/sylar/bin/conf/test.yaml.bak");
    // std::cout << root << std::endl;
    std::cout << "=============" << std::endl;
    sylar::Config::LoadFromYaml(root);
    std::cout << sylar::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    SYLAR_LOG_INFO(system_log) << "hello system" << std::endl;
    std::cout << "=============" << std::endl;

    system_log->setFormatter("%d%T%f%T%l%T%m%n");
    std::cout << sylar::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    SYLAR_LOG_INFO(system_log) << "hello system" << std::endl;
}

void test_loadConf()
{

    std::cout << sylar::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "before load conf ---------\r" << std::endl;

    // sylar::Config::Visit([](sylar::ConfigVarBase::ptr var) {
    //     SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
    //         << "name=" << var->getName() << " description="
    //         << var->getDescription()
    //         // << " typename=" << var->getTypeName()
    //         << " value=" << var->toString();
    // });

    sylar::Config::LoadFromConfDir("../../conf");

    std::cout << "---------||||||||||||\r" << std::endl;
    // std::cout << sylar::LoggerMgr::GetInstance()->toYamlString() << std::endl;
}

int test_load(int argc, char **argv)
{

    sylar::EnvMgr::GetInstance()->addHelp("s", "start writh the terminal");
    sylar::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    sylar::EnvMgr::GetInstance()->addHelp("c", "conf path default: ../conf");
    sylar::EnvMgr::GetInstance()->addHelp("p", "print help");
    // sylar::EnvMgr::GetInstance()->init(argc, argv); // 先不要 init 否则影响配置文件读取的目录起点

    test_loadConf();
    std::cout << " ===== " << std::endl;
    sleep(10);
    test_loadConf();
    return 0;
}

#if 0
#endif

int main(int argc, char *argv[])
{

#if 0
    sylar::Config::Visit([](sylar::ConfigVarBase::ptr var) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << "name=" << var->getName() << " description="
            << var->getDescription()
            // << " typename=" << var->getTypeName()
            << " value=" << var->toString();
    });

#endif

    // test_yaml();

    // test_config();

    // test_class();

    // test_logs();

    // std::cout << "-------------------------"<< std::endl;

    // test_loadConf();

    test_load(argc, argv);

    return 0;
}
