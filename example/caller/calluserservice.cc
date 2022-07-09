#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"

int main(int argc, char** argv) {

    // 程序启动后 想使用mprpc框架享受rpc服务 先初始化框架
    MprpcApplication::Init(argc, argv);

    // 演示远程发布的rpc方法Login
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    fixbug::LoginRequest request; 
    request.set_name("zhang san");
    request.set_pwd("123456");
    // 接受远程rpc响应
    fixbug::LoginResponse respone;
    // 调用远程rpc 其实是调用了MprpcChannel 的callmethod方法
    stub.Login(nullptr, &request, &respone, nullptr);

    // 一次调用rpc完成 读取响应结果
    if (respone.result().errcode() == 0) {
        std::cout << "rpc login respone success:" << respone.success() << std::endl; 
        
        for (int i = 0; i < respone.)

    } else {
        std::cout << "rpc login respone error:" << respone.result().errmsg() << std::endl; 
    }

    return 0;
}
