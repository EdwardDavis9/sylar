#include "sylar/log.hh"

#include <iostream>

#define choice 2

void testFormatter_1()
{
    sylar::Logger::ptr logger(new sylar::Logger);

    auto formatter = std::make_shared<sylar::LogFormatter>(
        "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%N%n");

#if choice == 0
    auto std_appender = std::make_shared<sylar::StdoutLogAppender>();
    std_appender->setFormatter(formatter);
    logger->addAppender(sylar::LogAppender::ptr(std_appender));
    sylar::LogEvent::ptr event(
        new sylar::LogEvent(logger->getLevel(), __FILE__, __LINE__, 0, 1, 1,
                            time(0), "测试名字: 自定义测试内容0"));
#else

    logger->initFormatter(); // 初始化默认日志格式解析器
    logger->addAppender(
        sylar::LogAppender::ptr(std::make_shared<sylar::StdoutLogAppender>()));

    // 添加重复的默认的 appender 测试
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

    sylar::LogEvent::ptr event(
        new sylar::LogEvent(logger->getLevel(), __FILE__, __LINE__, 0, 1, 1,
                            time(0), "测试名字: 自定义测试内容1"));
#endif

    logger->log(sylar::LogLevel::DEBUG, event);

    std::cout << std::endl << logger->toYamlString() << std::endl << std::endl;
}

void testFormatter_2()
{

    sylar::Logger::ptr logger(std::make_shared<sylar::Logger>());
    // logger->initFormatter(); // 初始化默认日志格式解析器

    logger->addAppender(
        sylar::LogAppender::ptr(std::make_shared<sylar::StdoutLogAppender>()));

    sylar::FileLogAppender::ptr file_appender(
        // std::make_shared<sylar::FileLogAppender>("./log.txt"));
        std::make_shared<sylar::FileLogAppender>("./filelog_test.txt"));
    sylar::LogFormatter::ptr fmt(
        std::make_shared<sylar::LogFormatter>("%d%T%p%T%c%T%f%T%f:%l%T%m%T%N%n"));

    file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::DEBUG); // 设置当前输出器的级别

    logger->addAppender(file_appender);

    SYLAR_LOG_ERROR(logger) << "[test macro error]";
    SYLAR_LOG_DEBUG(logger) << "[test macro debug]";
    SYLAR_LOG_INFO(logger) << "[test macro info]";

    std::cout << std::endl << logger->toYamlString() << std::endl << std::endl;

    // SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");

    // auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
    // SYLAR_LOG_INFO(l) << "xxx";
}

int main(int argc, char *argv[])
{

#if choice < 2
    testFormatter_1();
#else
    testFormatter_2();
#endif

    return 0;
}
