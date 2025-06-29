#include <iostream>
#include "log.hh"
#include "util.hh"

#include <yaml-cpp/yaml.h>
#include "config.hh"

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
        logger, logger->getLevel(), __FILE__, __LINE__, 0, 1, 2, time(0)));

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
        std::make_shared<sylar::LogFormatter>("%d%T%p%T%m%n"));

    file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::DEBUG); // 设置当前输出器的级别

    logger->addAppender(file_appender);

    SYLAR_LOG_ERROR((logger)) << "test macro error";
    // SYLAR_LOG_DEBUG((logger)) << "test macro debug";
    // SYLAR_LOG_INFO((logger)) << "test macro info";

    // SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");

    // auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
    // SYLAR_LOG_INFO(l) << "xxx";
}

sylar::ConfigVar<int>::ptr g_int_value_config =
    sylar::Config::Lookup("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_float_value_config =
    sylar::Config::Lookup("system.value", (float)10.2f, "system value");

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
    YAML::Node root = YAML::LoadFile("../bin/conf/log.yaml");

    print_yaml(root, 0);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root.Scalar();
}

void test_config()
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "before: " << g_int_value_config->getValue();

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "before: " << g_float_value_config->getValue();

    YAML::Node root = YAML::LoadFile("../bin/conf/log.yaml");

    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << " after: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << " after: " << g_float_value_config->getValue();
}

void test_1()
{
    // test
    std::cout << "1" << std::endl;
}
void test_2()
{
    // test
    std::cout << "2" << std::endl;
}
void test_3()
{
    // test
    std::cout << "3" << std::endl;
}

int main(int argc, char *argv[])
{
    // testFormatter_1();
    // testFormatter_2();
    test_config();

    return 0;
}
