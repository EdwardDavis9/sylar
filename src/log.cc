#include "sylar/log.hh"
#include <stdarg.h>
#include <memory>
#include <tuple>
#include <map>
#include <vector>
#include <functional>
#include <string>
#include <iostream>
#include "sylar/config.hh"
#include <time.h>

namespace sylar {

const char *LogLevel::ToString(LogLevel::Level level)
{
    switch (level) {

#define XX(name)         \
    case LogLevel::name: \
        return #name;    \
        break;

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
#undef XX
        default:
            return "UNKNOW";
    }
    return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string &str)
{
#define XX(level, v)            \
    if (str == #v) {            \
        return LogLevel::level; \
    }

    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);

    return LogLevel::UNKNOW;
#undef XX
}

LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e) {}

LogEventWrap::~LogEventWrap()
{
    // m_event->getLogger()->log(m_event->getLevel(), m_event);
    getEvent()->getLogger()->log(m_event->getLevel(), m_event);
}

void LogEvent::format(const char *fmt, ...)
{
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

/**
 * @brief 将可变参数的内容写入到字符串流中, 这样 日志事件包装类 可以直接输出
 * @details 通过 vprintf 进行输出
 * @param[in] fmt 格式化字符串
 * @param[in] al 可变参数列表
 */
void LogEvent::format(const char *fmt, va_list al)
{
    char *buf = nullptr;
    int len   = vasprintf(&buf, fmt, al);

    if (len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

std::stringstream &LogEventWrap::getSS() { return m_event->getSS(); }

void LogAppender::setFormatter(LogFormatter::ptr var)
{
    MutexType::Lock lock(m_mutex);

    m_formatter = var;
    if(m_formatter) {
        m_hasFormatter = true;
    } else {
        m_hasFormatter = false;
    }
}

Logger::Logger(const std::string &name) : m_name(name), m_level(LogLevel::DEBUG)
{
    m_formatter.reset(new LogFormatter(
        "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::initFormatter()
{
    m_formatter.reset(new LogFormatter(
        "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::setFormatter(LogFormatter::ptr var) { // 设置默认的 formatter
    MutexType::Lock lock(m_mutex);
    m_formatter = var;
    for(auto &i : m_appenders) {
        MutexType::Lock ll(i->m_mutex);

        if(!i->m_hasFormatter) {
            i->m_formatter = m_formatter;
        }
    }
}

void Logger::setFormatter(const std::string &var)
{
    sylar::LogFormatter::ptr new_var(new sylar::LogFormatter(var));

    if (new_var->isError()) {
        std::cout << "Logger setFormatter name="
                  << m_name << " value=" << var
                  << " invalid formatter" << std::endl;
        return;
    }
    setFormatter(new_var);
}

std::string Logger::toYamlString()
{
    MutexType::Lock lock(m_mutex);

    YAML::Node node;
    node["name"] = m_name;
    if (m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }

    for (auto &i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogFormatter::ptr Logger::getFormatter() {
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

/**
 * @brief 添加输出器
 * @details 只用来添加输出器
 * @param[in] appender 输出器
 */
void Logger::addAppender(LogAppender::ptr appender)
{
    MutexType::Lock lock(m_mutex);

    if (!appender->getFormatter()) {
        MutexType::Lock ll(appender->m_mutex);
        appender->m_formatter = m_formatter; // 将日志的默认解析器设置到输出器中
    }

    // std::cout << appender->getFormatter()->getPattern() << std::endl;

    m_appenders.push_back(appender);
}

/**
 * @brief 删除输出器
 * @details 遍历输出器集合，然后移除与之相应的输出器
 * @param[in] appender 待移除的输出器
 */
void Logger::delAppender(LogAppender::ptr appender)
{
    MutexType::Lock lock(m_mutex);
    for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
        if (*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders() {
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

/**
 * @brief 日志输出
 * @details 如果日志的级别大于默认级别，那么尝试遍历输出器集合，
 * 并通过输出器输出日志内容
 * @param[in] level 日志级别
 * @param[in] event 待输出的事件
 */
void Logger::log(LogLevel::Level level, LogEvent::ptr event)
{
    if (level >= m_level) { // 事件级别 是否 大于 当前日志的警醒级别

        auto self = shared_from_this();

        MutexType::Lock lock(m_mutex);
        // 如果设置了专有输出器
        if (!m_appenders.empty()) {
            for (auto &i : m_appenders) {
                i->log(self, level, event);
            }
        }
        else if (m_root) {
            m_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event) { log(LogLevel::DEBUG, event); }

void Logger::info(LogEvent::ptr event) { log(LogLevel::DEBUG, event); }

void Logger::warn(LogEvent::ptr event) { log(LogLevel::WARN, event); }

void Logger::error(LogEvent::ptr event) { log(LogLevel::ERROR, event); }

void Logger::fatal(LogEvent::ptr event) { log(LogLevel::FATAL, event); }

std::string FileLogAppender::toYamlString()
{
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if (m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

/**
 * @brief 重新打开文件
 * @details
 * 如果输出流已经打开，那么关闭该流，并以追加的方式，重新打开该文件流
 * @return 1 -> success, 0 -> error
 */
bool FileLogAppender::reopen()
{
    MutexType::Lock lock(m_mutex);
    if (m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename, std::ios::app); // 追加方式打开文件

    return !!m_filestream; // tricks: 快速判断是否成功打开文件
}

FileLogAppender::FileLogAppender(const std::string &filename)
    : m_filename(filename)
{
    reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                          LogEvent::ptr event)
{
    if (level >= m_level) { // 事件的日志级别 是否大于 输出器的 默认惊醒级别
        uint64_t now = time(0);

        // 为避免在输出内容时，文件被删除，这里每次在输出前都 ropen
        if(now != m_lastTime){
            reopen();
            m_lastTime = now;
        }

        MutexType::Lock lock(m_mutex);
        if(!(m_filestream << m_formatter->format(logger, level, event))) {
            std::cout << "error " << std::endl;
        }
    }
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger,
                            LogLevel::Level level,
                            LogEvent::ptr event)
{

    // 只有大于日志默认级别，才会尝试将其输出
    // 但是否输出，还依赖于输出器的输出级别
    if (level >= m_level) { // 事件的日志级别 是否大于 当前日志的默认级别
        MutexType::Lock lock(m_mutex);
        std::cout << m_formatter->format(logger, level, event);
    }

}

std::string StdoutLogAppender::toYamlString()
{
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if (m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern)
{
    init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger,
                                 LogLevel::Level level, LogEvent::ptr event)
{

    // std::cerr << "[formatter] m_items size = " << m_items.size() <<
    // std::endl;

    std::stringstream s_stream;

    for (auto &i : m_items) {
        i->format(s_stream, logger, level, event);
    }

    return s_stream.str();
}

LogFormatter::ptr LogAppender::getFormatter() {
    MutexType::Lock lock(m_mutex);

    return m_formatter;
}

/**
 * @brief 日志格式初始化
 * @details Description  //%xxx %xxx{xxx} %%
 */

class MessageFormatItem : public LogFormatter::FormatItem {
  public:
    MessageFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
  public:
    LevelFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
  public:
    ElapseFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getElapse();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
  public:
    ThreadIdFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
  public:
    FiberIdFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
  public:
    ThreadNameFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getThreadNam();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
  public:
    DateTimeFormatItem(const std::string format = "%Y:%m:%d %H:%M:%S")
        : m_format(format)
    {
        if (m_format.empty()) {                                        // 如果显示地传入了一个空字符串
            m_format = "%Y-%m-%d  %H:%M:%S"; // eward append
        }
    }

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm); // 以线程安全的方式将时间戳转换成本地时间
        char buf[64];

        // 将时间格式化输入字符串中
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);

        os << buf;
    }

  private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
  public:
    FilenameFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getFile();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
  public:
    NameFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        // os << logger->getName();
        os << event->getLogger()->getName();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
  public:
    LineFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
  public:
    NewLineFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << std::endl;
    }
};

class TabFormatItem : public LogFormatter::FormatItem {
  public:
    TabFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << "\t";
    }

  private:
    std::string m_string;
};

class StringFormatItem : public LogFormatter::FormatItem {
  public:
    StringFormatItem(const std::string &str) : m_string(str) {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << m_string;
    }

  private:
    std::string m_string;
};

void LogFormatter::init()
{
    // str, format, type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr;

    for (size_t i = 0; i < m_pattern.size(); ++i) {
        if (m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }

        if ((i + 1) < m_pattern.size()) {
            if (m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n         = i + 1;
        int fmt_status   = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while (n < m_pattern.size()) {
            if (!fmt_status
                && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                    && m_pattern[n] != '}'))
            {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }

            // 说明这个占位符带格式，例如：%d{%Y-%m-%d}
            if (fmt_status == 0) {
                if (m_pattern[n] == '{') {
                    str = m_pattern.substr(
                        i + 1, n - i - 1); // 对称字符，只截取关键内容
                    fmt_status = 1;        // 解析格式
                    fmt_begin  = n;
                    ++n;
                    continue;
                }
            }

            // 格式部分结束
            else if (fmt_status == 1) {
                if (m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if (n == m_pattern.size()) {
                if (str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }
        if (fmt_status == 0) { // 没有花括号格式，普通的 %d %p 这种
            if (!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, "", 0));
                nstr.clear();
            }

            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        }
        else if (fmt_status == 1) { // 花括号未闭合
            std::cout << "pattern parse error: " << m_pattern << " - "
                      << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }

    if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }

    //%m --消息体
    //%p -- level
    // %r == 启动后的时间
    //%c -- 日志名称
    //%t -- 线程id
    //%n == 回车换行
    //%d -- 时间
    //%f -- 文件名
    //%l -- 行号
    static std::map<std::string,
                    std::function<FormatItem::ptr(const std::string &str)>>
        s_format_items = {

#define XX(str, C)                                                             \
    {                                                                          \
        #str,                                                                  \
            [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
    }

            XX(m, MessageFormatItem),          //m:消息
            XX(p, LevelFormatItem),            //p:日志级别
            XX(r, ElapseFormatItem),           //r:累计毫秒数
            XX(c, NameFormatItem),             //c:日志名称
            XX(t, ThreadIdFormatItem),         //t:线程id
            XX(n, NewLineFormatItem),          //n:换行
            XX(d, DateTimeFormatItem),         //d:时间
            XX(f, FilenameFormatItem),         //f:文件名
            XX(l, LineFormatItem),             //l:行号
            XX(T, TabFormatItem),              //T:Tab
            XX(F, FiberIdFormatItem),           //F:协程id
            XX(N, ThreadNameFormatItem),       //N:线程名称

#undef XX
        };

    for (auto &i : vec) {
        if (std::get<2>(i) == 0) {
            m_items.push_back(
                FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }
        else {
            auto it = s_format_items.find(std::get<0>(i));
            if (it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(
                    "<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            }
            else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
    }
}


LogEvent::LogEvent(std::shared_ptr<Logger> logger,
                   LogLevel::Level level,
                   const char *file,
                   int32_t line,
                   uint32_t elapse,
                   uint32_t thread_id,
                   uint32_t fiber_id,
                   uint64_t time,
                   const std::string& thread_name)
    : m_file(file),
      m_line(line),
      m_elapse(elapse),
      m_threadId(thread_id),
      m_fiberId(fiber_id),
      m_time(time),
      m_threadName(thread_name),
      m_logger(logger),
      m_level(level)
{ }

Logger::ptr LoggerManager::getLogger(const std::string &name)
{
    MutexType::Lock lock(m_mutex);

    auto it = m_loggers.find(name);

    if (it != m_loggers.end()) {
        return it->second;
    }

    Logger::ptr logger(new Logger(name));
    logger->m_root  = m_root;
    m_loggers[name] = logger;
    return logger;
}

struct LogAppenderDefine {
    int type              = 0; // 1->file, 2->stdout
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine &oth) const
    {
        return type == oth.type
            && level == oth.level
            && formatter == oth.formatter
            && file == oth.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine &oth) const
    {
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter
            && appenders == oth.appenders;
    }

    bool operator<(const LogDefine &oth) const { return name < oth.name; }
};

template <>
class LexicalCast<std::string, std::set<LogDefine>> {
  public:
    std::set<LogDefine> operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        std::set<LogDefine> vec;
        for (size_t i = 0; i < node.size(); ++i) {
            auto n = node[i];
            if (!n["name"].IsDefined()) {
                std::cout << "log config error: name is null, " << n
                          << std::endl;
                continue;
            }

            LogDefine ld;
            ld.name  = n["name"].as<std::string>();
            ld.level = LogLevel::FromString(
                n["level"].IsDefined() ? n["level"].as<std::string>() : "");

            if(n["formatter"].IsDefined()) {
                ld.formatter = n["formatter"].as<std::string>();
            }

            if (n["appenders"].IsDefined()) {
                // std::cout << "==" << ld.name << " = " << n["appenders"].size()
                //           << std::endl;

                for (size_t x = 0; x < n["appenders"].size(); ++x) {
                    auto a = n["appenders"][x];
                    if (!a["type"].IsDefined()) {
                        std::cout << "log config error: appender type"
                            " is null, " << a << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    LogAppenderDefine lad;
                    if (type == "FileLogAppender") {
                        lad.type = 1;
                        if (!a["file"].IsDefined()) {
                            std::cout << "log config error: fileappender"
                                         " file is null, "
                                      << a << std::endl;
                            continue;
                        }
                        lad.file = a["file"].as<std::string>();
                        if (a["formatter"].IsDefined()) {
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    }
                    else if (type == "StdoutLogAppender") {
                        lad.type = 2;
                    }
                    else {
                        std::cout << "Log config error: appender type is"
                            " invaild, " << a << std::endl;
                        continue;
                    }
                    ld.appenders.push_back(lad);
                }
            }
            // std::cout << "---" << ld.name << " - "
            //     << ld.appenders.size() << std::endl;
            vec.insert(ld);
        }
        return vec;
    }
};

template <>
class LexicalCast<std::set<LogDefine>, std::string> {
  public:
    std::string operator()(const std::set<LogDefine> &v)
    { // 构造一个对应的 node 节点
        YAML::Node node;
        for (auto &i : v) {
            YAML::Node n;
            n["name"] = i.name;
            if (i.level != LogLevel::UNKNOW) {
                n["level"] = LogLevel::ToString(i.level);
            }
            if (i.formatter.empty()) {
                n["formatter"] = i.formatter;
            }

            for (auto &a : i.appenders) {
                YAML::Node na;
                if (a.type == 1) { // file
                    na["type"] = "FileLogAppender";
                    na["file"] = a.file;
                }
                else if (a.type == 2) { // std
                    na["type"] = "StdoutLogAppender";
                }
                if(a.level != LogLevel::UNKNOW) {
                    na["level"] = LogLevel::ToString(a.level);
                }

                if (!a.formatter.empty()) {
                    na["formatter"] = a.formatter;
                }

                n["appenders"].push_back(na);
            }
            node.push_back(n);
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

sylar::ConfigVar<std::set<LogDefine>>::ptr
g_log_defines =
    sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter {
    LogIniter()
    {
        g_log_defines->addListener(
            [](const std::set<LogDefine> &old_value,
                        const std::set<LogDefine> &new_value) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_logger_conf_changed";

                for (auto &i : new_value) {
                    sylar::Logger::ptr logger;
                    auto it = old_value.find(i);
                    if (it == old_value.end()) {
                        logger = SYLAR_LOG_NAME(i.name);
                    }
                    else {
                        if (!(i == *it)) {
                            logger = SYLAR_LOG_NAME(i.name);
                        }
                    }
                    logger->setLevel(i.level);
                    if (!i.formatter.empty()) {
                        logger->setFormatter(i.formatter);
                    }

                    logger->clearAppenders();
                    for (auto &a : i.appenders) {
                        sylar::LogAppender::ptr ap;
                        if (a.type == 1) {
                            ap.reset(new FileLogAppender(a.file));
                        }
                        else if (a.type == 2) {
                            ap.reset(new StdoutLogAppender);
                        }
                        ap->setLevel(a.level);
                        if(!a.formatter.empty()) {
                            LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                            if(!fmt->isError()) {
                                ap->setFormatter(fmt);
                            } else {
                            std::cout << "log.name = " << i.name << " appendeer type"
                                << a.type << " formatter=" << a.formatter
                                << " is invaild" << std::endl;
                            }
                        }
                        logger->addAppender(ap);
                    }
                }

                for (auto &i : old_value) {
                    auto it = new_value.find(i);
                    if (it == new_value.end()) {
                        // 删除logger
                        auto logger = SYLAR_LOG_NAME(i.name);
                        logger->setLevel((LogLevel::Level)0);
                        logger->clearAppenders();
                    }
                }
            });
    }
};

static LogIniter _log_init;

std::string LoggerManager::toYamlString()
{
    MutexType::Lock lock(m_mutex);

    YAML::Node node;
    for (auto &i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LoggerManager::LoggerManager()
{
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

    m_loggers[m_root->m_name] = m_root;

    init();
}

void LoggerManager::init() {}

} // namespace sylar
