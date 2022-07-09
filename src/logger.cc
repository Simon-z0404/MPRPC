#include "logger.h"
#include "time.h"
#include <iostream>

 // 获取日志的单例
Logger& Logger::GetLoggerInstance() {
    static Logger logger;
    return logger;
}

// 启动专门的写日志线程
/*
    注意方法请求方不要用Logger记录日志 
    日志I/O子线程会等待信号量 导致请求方无法正常退出
*/
Logger::Logger() {
    std::thread writeLogTask(
        [&](){
            for (;;) {
                // 获取日期 从队列中获取日志信息 追加到文件中
                time_t now = time(nullptr);
                tm* nowtm = localtime(&now);

                char filename[128] = {0};
                sprintf(filename, "%d-%d-%d-log.txt", 
                                    nowtm->tm_year + 1900, nowtm->tm_mon + 1, nowtm->tm_mday);

                FILE* pf = fopen(filename, "a+");
                if (nullptr == pf) {
                    std::cout << "logger file " << filename <<  " open error" << std::endl; 
                    exit(EXIT_FAILURE);
                }
                // 插入时间前缀
                char time_buf[128] = {0};
                sprintf(time_buf, "%d:%d:%d => [%s]", 
                                    nowtm->tm_hour, 
                                    nowtm->tm_min, 
                                    nowtm->tm_sec,
                                    (m_loglevel == INFO) ? "info" : "error");
                std::string msg = m_lckQue.Pop();
                msg.insert(0, time_buf);
                msg.append("\n");
                fputs(msg.c_str(), pf);
                fclose(pf);
            }
        }
    );
    // 设置线程分离
    writeLogTask.detach();
}


// 设置日志级别
void Logger::setLogLevel(LogLevel level) {
    m_loglevel = level;
}

// 写日志 将日志放到lockqueue缓冲队列中
void Logger::Log(const std::string msg) { 
    m_lckQue.Push(msg);
}


    