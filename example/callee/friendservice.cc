#include "friend.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "logger.h"
#include <vector>
#include <string> 

class FriendService : public fixbug::FriendServiceRpc 
{
    // 获取好友列表 本地实现
    std::vector<std::string> GetFriendList(u_int32_t id, std::string name, std::string pwd) {
        
        std::cout << "GetFriendList Local" << std::endl;
        std::cout << "id:" << id << " name:" << name << " pwd:" << pwd << std::endl;

        std::vector<std::string> friendVec;
        friendVec.push_back("Simon1");
        friendVec.push_back("Simon2");
        friendVec.push_back("Simon3");

        return friendVec;
    }

    void GetFriendList(::google::protobuf::RpcController* controller,
                       const ::fixbug::GetFriendListRequest* request,
                       ::fixbug::GetFriendListRespone* response,
                       ::google::protobuf::Closure* done)
    {
        u_int32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        // Test
        std::cout << "id:" << id << std::endl;
        std::cout << "name:" << name << std::endl;
        std::cout << "pwd:" << pwd << std::endl; 
        // 执行本地getFriendList方法
        std::vector<std::string> friendVec = GetFriendList(id, name, pwd);
        // 填充rpc响应
        for (int i = 0; i < friendVec.size(); ++i) {
            response->add_friendlist(friendVec[i].c_str());
        }
        response->set_success(true);
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        
        // 调用done进行序列化和网络发送 由框架完成
        done->Run();
    }
};

int main(int argc, char **argv) {
    // 测试日志
    // LOG_INFO("first log message");
    // LOG_ERR("%s:%s:%d", __FILE__, __FUNCTION__, __LINE__);
    
    // 初始化框架配置
    MprpcApplication::Init(argc, argv);

    // 注册一个rpc服务对象 将FriendService发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new FriendService());

    // 启动服务
    provider.run();
}

