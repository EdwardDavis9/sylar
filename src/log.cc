#include "log.hh"
#include <stdarg.h>
#include <memory>
#include <tuple>
#include <map>
#include <vector>
#include <functional>
#include <string>
#include <iostream>

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

Logger::Logger(const std::string &name) : m_name(name), m_level(LogLevel::DEBUG)
{
    m_formatter.reset(new LogFormatter(
        "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::initFormatter()
{

    // auto formatter = std::make_shared<sylar::LogFormatter>("%d{%Y-%m-%d
    // %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"); m_formatter.reset(new
    // std::make_shared<LogFormatter>("%d{%Y-%m-%d
    // %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n")); m_formatter.reset(new
    // std::make_shared<LogFormatter>("%d{%Y-%m-%d
    // %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    // m_formatter.reset(m_formatter);

    m_formatter.reset(new LogFormatter(
        "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

/**
 * @brief 添加输出器
 * @details 只用来添加输出器
 * @param[in] appender 输出器
 */
void Logger::addAppender(LogAppender::ptr appender)
{
    if (!appender->getFormatter()) {

        // std::cout << "set appender fortmatter" << std::endl;
        // std::cout << m_formatter->getPattern() << std::endl;
        appender->setFormatter(m_formatter); // 将日志的默认解析器设置到输出器中
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
    for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
        if (*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
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

        for (auto &i : m_appenders) {

            // std::cerr << "[debug] formatter in appender = " <<
            // i->getFormatter().get() << std::endl; std::cerr << "[debug]
            // formatter in logger = " << m_formatter.get() << std::endl;

            i->log(self, level, event);

            // std::cout << i->getFormatter()->getPattern() << std::endl;
        }
    }
}

void Logger::debug(LogEvent::ptr event) { log(LogLevel::DEBUG, event); }

void Logger::info(LogEvent::ptr event) { log(LogLevel::DEBUG, event); }

void Logger::warn(LogEvent::ptr event) { log(LogLevel::WARN, event); }

void Logger::error(LogEvent::ptr event) { log(LogLevel::ERROR, event); }

void Logger::fatal(LogEvent::ptr event) { log(LogLevel::FATAL, event); }

/**
 * @brief 重新打开文件
 * @details 如果输出流已经打开，那么关闭该流，并以追加的方式，重新打开该文件流
 * @return 1 -> success, 0 -> error
 */
bool FileLogAppender::reopen()
{
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
        m_filestream << m_formatter->format(logger, level, event);
    }
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event)
{

    // std::cerr << "[log] StdoutLogAppender is loggine before..." << " and
    // level is " << level << std::endl;

    // std::cerr << "[debug] logger level = " << LogLevel::ToString(m_level)
    // << ", event level = " << LogLevel::ToString(level) << std::endl;

    if (level >= m_level) { // 事件的日志级别 是否大于 当前日志的默认级别

        // 只有大于日志的
        // 默认级别，才会尝试将其输出，但是否输出，还依赖于输出器的输出级别

        // std::cerr << "[log] StdoutLogAppender is logging...\n";

        std::cout << m_formatter->format(logger, level, event);
    }

    // std::cerr << "[log] StdoutLogAppender is logging end...\n";
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

class DateTimeFormatItem : public LogFormatter::FormatItem {
  public:
    DateTimeFormatItem(const std::string format = "%Y:%m:%d %H:%M:%S")
        : m_format(format)
    {
        if (m_format.empty()) {
            m_format = "%y-%m-%d  %H:%M:%S"; // eward append
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
        os << logger->getName();
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

            ++n; // 这里需要调整 #2

            if (n == m_pattern.size()) {
                if (str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }
        if (fmt_status == 0) { // 没有花括号格式，普通的 %d %p 这种
            if (!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, "", 0));
                nstr.clear(); // #3
            }

            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        }
        else if (fmt_status == 1) { // 花括号未闭合
            std::cout << "pattern parse error: " << m_pattern << " - "
                      << m_pattern.substr(i) << std::endl;
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

            XX(m, MessageFormatItem),  XX(p, LevelFormatItem),
            XX(r, ElapseFormatItem),   XX(c, NameFormatItem),
            XX(t, ThreadIdFormatItem), XX(n, NewLineFormatItem),
            XX(d, DateTimeFormatItem), XX(f, FilenameFormatItem),
            XX(l, LineFormatItem),     XX(T, TabFormatItem),
            XX(F, FiberIdFormatItem)

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
            }
            else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
    }
}

// void LogFormatter::init() {
//   // str, format, type
//   std::vector<std::tuple<std::string, std::string, int> > vec;
//   std::string nstr;
//   for (size_t i = 0; i < m_pattern.size(); ++i) {
//     if (m_pattern[i] != '%') {
//       nstr.append(1, m_pattern[i]);
//       continue;
//     }

//     if ((i + 1) < m_pattern.size()) {
//       if (m_pattern[i + 1] == '%') {
//         nstr.append(1, '%');
//         continue;
//       }
//     }

//     size_t n = i + 1;
//     int fmt_status = 0;
//     size_t fmt_begin = 0;

//     std::string str;
//     std::string fmt;
//     while (n < m_pattern.size()) {
//       if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' &&
//       m_pattern[n] != '}')) {
//         str = m_pattern.substr(i + 1, n - i - 1);
//         break;
//       }
//       if (fmt_status == 0) {
//         if (m_pattern[n] == '{') {
//           str = m_pattern.substr(i + 1, n - i - 1);
//           // std::cout << "*" << str << std::endl;
//           fmt_status = 1; // 解析格式
//           fmt_begin = n;
//           ++n;
//           continue;
//         }
//       } else if (fmt_status == 1) {
//         if (m_pattern[n] == '}') {
//           fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
//           // std::cout << "#" << fmt << std::endl;
//           fmt_status = 0;
//           ++n;
//           break;
//         }
//       }
//       ++n;
//       if (n == m_pattern.size()) {
//         if (str.empty()) {
//           str = m_pattern.substr(i + 1);
//         }
//       }
//     }

//     if (fmt_status == 0) {
//       if (!nstr.empty()) {
//         vec.push_back(std::make_tuple(nstr, std::string(), 0));
//         nstr.clear();
//       }
//       vec.push_back(std::make_tuple(str, fmt, 1));
//       i = n - 1;
//     } else if (fmt_status == 1) {
//       std::cout << "pattern parse error: " << m_pattern << " - " <<
//       m_pattern.substr(i) << std::endl;
//       vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
//     }
//   }

//   if (!nstr.empty()) {
//     vec.push_back(std::make_tuple(nstr, "", 0));
//   }
//   static std::map<std::string, std::function<FormatItem::ptr(const
//   std::string& str)> > s_format_items = {
#define XX(str, C) \
    {#str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); }}

//     XX(m, MessageFormatItem),  XX(p, LevelFormatItem),   XX(r,
//     ElapseFormatItem),   XX(c, NameFormatItem), XX(t, ThreadIdFormatItem),
//     XX(n, NewLineFormatItem), XX(d, DateTimeFormatItem), XX(f,
//     FilenameFormatItem), XX(l, LineFormatItem),     XX(T, TabFormatItem),
//     XX(F, FiberIdFormatItem),
#undef XX
//   };

//   for (auto& i : vec) {
//     if (std::get<2>(i) == 0) {
//       m_items.push_back(FormatItem::ptr(new
//       StringFormatItem(std::get<0>(i))));
//     } else {
//       auto it = s_format_items.find(std::get<0>(i));
//       if (it == s_format_items.end()) {
//         m_items.push_back(FormatItem::ptr(new
//         StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
//       } else {
//         m_items.push_back(it->second(std::get<1>(i)));
//       }
//     }

//     // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ")
//     - (" << std::get<2>(i) << ")" << std::endl;
//   }
//   // std::cout << m_items.size() << std::endl;
// }

// LogEvent::LogEvent(const char *file, int32_t line, uint32_t elapse,
//                    uint32_t thread_id, uint32_t fiber_id, uint64_t time)
//     : m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id),
//       m_fiberId(fiber_id), m_time(time)
// {

//     // std::cout << " 无 logger 构造" << std::endl;
// }

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   const char *file, int32_t line, uint32_t elapse,
                   uint32_t thread_id, uint32_t fiber_id, uint64_t time)
    : m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id),
      m_fiberId(fiber_id), m_time(time), m_logger(logger), m_level(level)
{

    // std::cout << "有 logger 构造" << std::endl;
}

Logger::ptr LoggerManager::getLogger(const std::string &name)
{
    // Auto Generate

    auto it = m_loggers.find(name);
    return it == m_loggers.end() ? m_root : it->second;
}

LoggerManager::LoggerManager()
{
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
}

void LoggerManager::init()
{
    // Auto Generate
}

} // namespace sylar
