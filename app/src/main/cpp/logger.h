#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
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
    
    // 获取当前时间戳字符串
    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    // 获取日志级别字符串
    static std::string getLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "[调试]";
            case LogLevel::INFO:  return "[信息]";
            case LogLevel::WARN:  return "[警告]";
            case LogLevel::ERROR: return "[错误]";
            default: return "[未知]";
        }
    }
    
public:
    // 写入日志的核心函数
    static void writeLog(LogLevel level, const std::string& message, const std::string& function = "") {
        std::lock_guard<std::mutex> lock(log_mutex);
        
        std::ofstream logFile(LOG_FILE_PATH, std::ios::app);
        if (!logFile.is_open()) {
            // 如果无法打开日志文件，输出到标准错误
            std::cerr << "无法打开日志文件: " << LOG_FILE_PATH << std::endl;
            return;
        }
        
        std::string timestamp = getCurrentTimestamp();
        std::string levelStr = getLevelString(level);
        
        // 格式: [时间戳] [级别] [函数名] 消息
        logFile << "[" << timestamp << "] " << levelStr;
        if (!function.empty()) {
            logFile << " [" << function << "]";
        }
        logFile << " " << message << std::endl;
        
        logFile.close();
    }
    
    // 便捷的日志记录宏
    static void debug(const std::string& message, const std::string& function = "") {
        writeLog(LogLevel::DEBUG, message, function);
    }
    
    static void info(const std::string& message, const std::string& function = "") {
        writeLog(LogLevel::INFO, message, function);
    }
    
    static void warn(const std::string& message, const std::string& function = "") {
        writeLog(LogLevel::WARN, message, function);
    }
    
    static void error(const std::string& message, const std::string& function = "") {
        writeLog(LogLevel::ERROR, message, function);
    }
};

// 静态成员定义
std::mutex Logger::log_mutex;
const std::string Logger::LOG_FILE_PATH = "/cache/OplusKey.log";

// 便捷宏定义
#define LOG_DEBUG(msg) Logger::debug(msg, __FUNCTION__)
#define LOG_INFO(msg) Logger::info(msg, __FUNCTION__)
#define LOG_WARN(msg) Logger::warn(msg, __FUNCTION__)
#define LOG_ERROR(msg) Logger::error(msg, __FUNCTION__)

#endif // LOGGER_H