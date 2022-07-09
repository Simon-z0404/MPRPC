#include <iostream>
#include <string>
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "user.pb.h"

class UserService : public fixbug::UserServiceRpc {
public:
    bool Login(std::string name, std::string pwd) {
        std::cout << "doing local service login" << std::endl;
        std::cout << "name:" << name << " " << "pwd: " << pwd << std::endl;
        return true;
    }

    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)
    {
        // 本地重写业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool loginResult = Login(name, pwd);  // 做本地业务

        // 把响应写入
        response->set_success(loginResult);
        fixbug::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");

        // 执行回调  执行响应对象的序列化和网络发送，由框架完成
        done->Run();
    } 
};

int main(int argc, char **argv) {

    // 调用框架的初始化动作
    MprpcApplication::Init(argc, argv);

    // provider是一个rpc网络服务对象。将UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new UserService());

    // 启动一个网络服务对象 run之后进入阻塞，等待远程rpc的调用
    provider.run();

    return 0;
}
