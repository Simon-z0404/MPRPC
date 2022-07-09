# pragma once
#include "mprpcconfig.h"
#include "mprpccontroller.h"
#include "mprpcchannel.h"
#include "logger.h"

// mprpc框架的基础类 
// 负责框架的初始化操作
class MprpcApplication
{

public:
    static void Init(int argc, char **argv);
    static MprpcApplication& GetInstance();
    static MprpcConfig& getConfig();
private:
    static MprpcConfig m_config;
    // 设计成单例模式
    MprpcApplication(){};
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication(const MprpcApplication&&) = delete;
};


