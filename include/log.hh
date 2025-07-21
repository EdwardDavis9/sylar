#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <memory>
#include <stdint.h>
#include <iostream>
#include <list>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include "util.hh"
#include <fstream>
#include "singleton.hh"
#include "thread.hh"

// std::cout << logger->getLevel() << ", and " << level << std::endl;
#define SYLAR_LOG_LEVEL(logger, level)                                  \
    if (logger->getLevel() <= level)                                    \
    sylar::LogEventWrap(                                                \
        sylar::LogEvent::ptr(new sylar::LogEvent(                       \
            logger, level, __FILE__, __LINE__, 0, sylar::GetThreadId(), \
            sylar::GetFiberId(), time(0))))                             \
        .getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger)  SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger)  SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

// 返回 logger 指针
#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)

// 通过可变参数将格式内容写入到字符串流中
#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...)                    \
    if (logger->getLevel() <= level)                                    \
    sylar::LogEventWrap(                                                \
        sylar::LogEvent::ptr(new sylar::LogEvent(                       \
            logger, level, __FILE__, __LINE__, 0, sylar::GetThreadId(), \
            sylar::GetFiberId(), time(0))))                             \
        .getEvent()                                                     \
        ->format(fmt, __VA_ARGS__)

// 手动指定可变参数的输出格式
#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) \
    SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...) \
    SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...) \
    SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) \
    SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) \
    SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

// define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()

namespace sylar {

class Logger;
class LoggerManager;

/**
 * @class 日志级别
 */
class LogLevel {
  public:
    enum Level {
        UNKNOW = 0,
        DEBUG  = 1,
        INFO   = 2,
        WARN   = 3,
        ERROR  = 4,
        FATAL  = 5
    };

    static const char *ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string& str);
};

/**
 * @class 日志事件, 定义了一些和日志属性相关的内容信息
 */
class LogEvent {
  public:
    using ptr = std::shared_ptr<LogEvent>;

    // LogEvent(const char *file, int32_t line, uint32_t elapse,
    //          uint32_t thread_id, uint32_t fiber_id, uint64_t time);

    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
             const char *file, int32_t line, uint32_t elapse,
             uint32_t thread_id, uint32_t fiber_id, uint64_t time);

    // 获得日志事件的属性

    /**
     * @brief 返回文件名
     */
    const char *getFile() const { return m_file; }

    int32_t getLine() const { return m_line; }

    uint32_t getElapse() const { return m_elapse; }

    uint32_t getFiberId() const { return m_fiberId; }

    uint32_t getThreadId() const { return m_threadId; }

    uint32_t getTime() const { return m_time; }

    std::string getContent() const { return m_ss.str(); }

    std::shared_ptr<Logger> getLogger() const { return m_logger; }

    /**
     * @brief 获得字符串流对象
     * @return m_ss: std::stringstream&
     */
    std::stringstream &getSS() { return m_ss; }

    LogLevel::Level getLevel() const { return m_level; }

    /**
     * @brief 格式化输出内容到字符串流中
     * @param[in] fmt 格式化内容
     */
    void format(const char *fmt, ...);

    /**
     * @brief 将可变参数的内容写入到字符串流中
     * @param[in] fmt 格式化字符串
     * @param[in] al 可变参数列表
     */
    void format(const char *fmt, va_list al);

  private:
    const char *m_file  = nullptr; /**< 文件名 */
    int32_t m_line      = 0;       /**< 行号 */
    uint32_t m_elapse   = 0;       /**< 程序启动开始到现在的毫秒数 */
    uint32_t m_threadId = 0;       /**< 线程id */
    uint32_t m_fiberId  = 0;       /**< 协程id */
    uint64_t m_time;               /**< 时间戳 */
    std::stringstream m_ss;

    std::shared_ptr<Logger> m_logger; /**< Log */
    LogLevel::Level m_level;          /**< Log的等级 */
};

/**
 * @class LogEventWrap 日志事件输出包装类
 * @details RAII
 */
class LogEventWrap {
  public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    LogEvent::ptr getEvent() const { return m_event; }
    std::stringstream &getSS();

  private:
    LogEvent::ptr m_event;
};

/**
 * @class 日志格式器
 */
class LogFormatter {

  public:
    using ptr = std::shared_ptr<LogFormatter>;

    LogFormatter(const std::string &pattern);

    /**
     * @brief 日志格式
     * @detail 获取事件，并将其进行输出
     * %t %thread_id %m %n
     *
     * @param[in] event 待输出的事件
     * @return 事件对应的信息
     */

    //%t    %thread_it %m%n
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                       LogEvent::ptr event);

    std::string getPattern() const { return m_pattern; }

  public:
    class FormatItem {
      public:
        using ptr = std::shared_ptr<FormatItem>;
        // FormatItem(const std::string& fmt = "") {}

        virtual ~FormatItem() {}
        virtual void format(std::ostream &ofs, std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event) = 0;
    };
    void init();

    bool isError() const { return m_error;}
    const std::string getPatrtern() const { return m_pattern;}

  private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
    bool m_error = false;
};

/**
 * @class 日志输出器
 */
class LogAppender {
friend class Logger;
  public:
    using ptr = std::shared_ptr<LogAppender>;

    using MutexType = Spinlock;

    virtual ~LogAppender() {}

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event) = 0;

    virtual std::string toYamlString() = 0;

    void setFormatter(LogFormatter::ptr val);

    LogFormatter::ptr getFormatter();

    LogLevel::Level getLevel() const { return m_level; }

    void setLevel(LogLevel::Level val) { m_level = val; }

  protected:
    LogLevel::Level m_level = LogLevel::DEBUG;/**< 日志输出器的默认提示日志级别*/
    bool m_hasFormatter = false;
    MutexType m_mutex;  /**< 系统层面上自旋锁 */
    LogFormatter::ptr m_formatter; /**< 日志输出器所属的解析器 */
};

/**
 * @class 日志器
 */
class Logger : public std::enable_shared_from_this<Logger> {
friend class LoggerManager;
  public:
    using ptr = std::shared_ptr<Logger>;
    using MutexType = Spinlock;

    Logger(const std::string &name = "root");

    /**
     * @brief 初始化格式解析器
     * @details 直接初始化格式解析器，这样就不同在手动添加格式解析器了
     */
    void initFormatter();

    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders();

    void setLevel(LogLevel::Level val) { m_level = val; }
    LogLevel::Level getLevel() const { return m_level; }

    const std::string &getName() const { return m_name; }

    void setFormatter(LogFormatter::ptr var);
    void setFormatter(const std::string& val);
    LogFormatter::ptr getFormatter();
    std::string toYamlString();

  private:
    std::string m_name;                      /**< 日志名 */
    LogLevel::Level m_level;                 /**< Logger 的日志级别 */
    std::list<LogAppender::ptr> m_appenders; /**< 输出器集合 */
    LogFormatter::ptr m_formatter;           /**< 日志器所属的格式解析器 */
    Logger::ptr m_root;
    MutexType m_mutex;                       /**< 系统层面上的自旋锁 */
};

/**
 * @class 输出到控制台的 Appender
 */
class StdoutLogAppender : public LogAppender {

  public:
    using ptr = std::shared_ptr<StdoutLogAppender>;
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
             LogEvent::ptr event) override;

    std::string toYamlString() override;
};

/**
 * @class 输出到文件的 Appender
 */
class FileLogAppender : public LogAppender {

  public:
    using ptr = std::shared_ptr<FileLogAppender>;

    void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
             LogEvent::ptr event) override;

    FileLogAppender(const std::string &filename);

    std::string toYamlString() override;

    /**
     * @brief 重新打开文件
     * @return 成功 -> true， 失败 -> false
     */
    bool reopen();

  private:
    std::string m_filename;
    std::ofstream m_filestream;
    uint64_t m_lastTime = 0;
};

inline sylar::LogEvent::ptr MakeLogEvent(sylar::Logger::ptr logger,
                                         sylar::LogLevel::Level level,
                                         const char *file, int line,
                                         uint64_t thread_id, uint64_t fiber_id)
{
    return std::make_shared<sylar::LogEvent>(logger, level, file, line, 0,
                                             thread_id, fiber_id, time(0));
};

class LoggerManager {
  public:
    using MutexType = Spinlock;

    LoggerManager();

    Logger::ptr getLogger(const std::string &name);

    void init();

    /**
     * @brief 返回当前管理器的根指针
     * @return m_root : Logger::ptr
     */
    Logger::ptr getRoot() const { return m_root; }

    std::string toYamlString();

  private:
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr m_root;
    MutexType m_mutex;
};

using LoggerMgr = sylar::Singleton<LoggerManager>;
} // namespace sylar
#endif /* __SYLAR_LOG_H__ */
