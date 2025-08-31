#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <mutex>

// 日志级别枚举
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

// 日志工具类
class Logger {
private:
    static std::mutex log_mutex;
    static const std::string LOG_FILE_PATH;

    static std::string getCurrentTimestamp();
    static std::string getLevelString(LogLevel level);

public:
    static void writeLog(LogLevel level, const std::string& message, const std::string& function = "");

    static void debug(const std::string& message, const std::string& function = "");
    static void info(const std::string& message, const std::string& function = "");
    static void warn(const std::string& message, const std::string& function = "");
    static void error(const std::string& message, const std::string& function = "");
};

// 便捷宏定义
#define LOG_DEBUG(msg) Logger::debug(msg, __FUNCTION__)
#define LOG_INFO(msg)  Logger::info(msg, __FUNCTION__)
#define LOG_WARN(msg)  Logger::warn(msg, __FUNCTION__)
#define LOG_ERROR(msg) Logger::error(msg, __FUNCTION__)

#endif // LOGGER_H
