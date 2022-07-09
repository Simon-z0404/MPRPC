#pragma once
#include<string>
#include "lockqueue.h"
#include <iostream>

enum LogLevel{
    INFO, // 普通信息
    ERRO  // 错误信息
};

class Logger {
public:
    // 获取日志的单例
    static Logger& GetLoggerInstance();
    // 设置日志级别
    void setLogLevel(LogLevel level);
    // 写日志
    void Log(const std::string msg);

private:
    // 日志级别
    int m_loglevel;
    // 日志缓冲队列
    LockQueue<std::string> m_lckQue;

    // 设置成单例模式
    Logger();
    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
};

#define LOG_ERR(logmsgformat, ...) \
    do \
    {  \
        Logger &logger = Logger::GetLoggerInstance(); \
        logger.setLogLevel(ERRO); \
        char c[1024] = {0}; \
        snprintf(c, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.Log(c); \
    } while(0) \

// 定义宏
#define LOG_INFO(logmsgformat, ...) \
    do \
    { \
        Logger& logger = Logger::GetLoggerInstance(); \
        logger.setLogLevel(INFO); \
        char c[1024] = {0}; \
        snprintf(c, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.Log(c); \
    } while (0);




