#include "friend.pb.h"
#include "mprpcapplication.h"
#include "mprpccontroller.h"


int main(int argc, char** argv) {
    MprpcApplication::Init(argc, argv);
    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());

    // 填写参数
    fixbug::GetFriendListRequest request;
    request.set_id(100);
    request.set_name("IS-Simon");
    request.set_pwd("666666");
    // 输出参数 得到的响应
    fixbug::GetFriendListRespone respone;
    // 控制对象 如果发生了错误 会记录在controller中
    MprpcController controller;
    // rpc远程调用 其实是调用channel callmethod
    stub.GetFriendList(&controller, &request, &respone, nullptr);

    if (controller.Failed()) {
        std::cout << controller.ErrorText() << std::endl;
    } else {
        if (respone.result().errcode() == 0) {
            std::cout << "rpc GetFriendList success!" << std::endl;
            // 打印好友列表
            for (int i = 0; i < respone.friendlist_size(); ++i) {
                std::cout << "idx:" << i << " " << respone.friendlist(i) << std::endl;
            }
        } else {
            std::cout << "rpc GetFriendList fail! error code:" << respone.result().errcode()
                << "error msg:" << respone.result().errmsg() << std::endl;
        }
    }   
    exit(0);
}