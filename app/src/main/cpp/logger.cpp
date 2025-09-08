#include "logger.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdlib>

// 静态成员定义
std::mutex Logger::log_mutex;
const std::string Logger::LOG_FILE_PATH = "/cache/OplusKey.log";
std::ofstream Logger::log_stream;
bool Logger::is_open = false;

// 初始化与关闭
void Logger::initialize() {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (is_open) return;
    log_stream.open(LOG_FILE_PATH, std::ios::out | std::ios::app);
    if (!log_stream.is_open()) {
        std::cerr << "无法打开日志文件: " << LOG_FILE_PATH << std::endl;
        is_open = false;
        return;
    }
    is_open = true;
    std::atexit(Logger::shutdown);
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (!is_open) return;
    log_stream.flush();
    log_stream.close();
    is_open = false;
}

// 获取当前时间戳字符串
std::string Logger::getCurrentTimestamp() {
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
std::string Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "[调试]";
        case LogLevel::INFO:  return "[信息]";
        case LogLevel::WARN:  return "[警告]";
        case LogLevel::ERROR: return "[错误]";
        default: return "[未知]";
    }
}

// 写入日志的核心函数
void Logger::writeLog(LogLevel level, const std::string& message, const std::string& function) {
    std::lock_guard<std::mutex> lock(log_mutex);

    if (!is_open) {
        std::cerr << "日志未初始化，无法写入: " << LOG_FILE_PATH << std::endl;
        return;
    }

    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = getLevelString(level);

    log_stream << "[" << timestamp << "] " << levelStr;
    if (!function.empty()) {
        log_stream << " [" << function << "]";
    }
    log_stream << " " << message << std::endl;
    log_stream.flush();
}

// 各级别便捷函数
void Logger::debug(const std::string& message, const std::string& function) {
    writeLog(LogLevel::DEBUG, message, function);
}

void Logger::info(const std::string& message, const std::string& function) {
    writeLog(LogLevel::INFO, message, function);
}

void Logger::warn(const std::string& message, const std::string& function) {
    writeLog(LogLevel::WARN, message, function);
}

void Logger::error(const std::string& message, const std::string& function) {
    writeLog(LogLevel::ERROR, message, function);
}
