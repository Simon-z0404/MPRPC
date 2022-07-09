#include "rpcheader.pb.h"
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "mprpcapplication.h"
#include "mprpcchannel.h"
#include "zookeeperutil.h"

// 重写纯虚函数的方法
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                          google::protobuf::Message* response, google::protobuf::Closure* done) 
{                                          
    const google::protobuf::ServiceDescriptor* service = method->service();
    // 获取服务名 方法名
    std::string service_name = service->name();
    std::string method_name = method->name();
    // 序列化请求
    u_int32_t args_size = 0;
    std::string args_str;
    if (!request->SerializeToString(&args_str)) {
        // std::cout << "serilize request error!" << std::endl;
        controller->SetFailed("serilize request error!");
        return;
    } else {
        args_size = args_str.size();
    }
    // 设置消息头
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);
    // 序列化消息头
    u_int32_t header_size = 0;
    std::string rpc_header_str;
    if (!rpcHeader.SerializeToString(&rpc_header_str)) {
        // std::cout << "serilize header_str error!" << std::endl;
        controller->SetFailed("serilize header_str error!");
        return;
    } else {
        header_size = rpc_header_str.size();
    }

    // 组织待发送的rpc请求字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char*)&header_size, 4));
    send_rpc_str += rpc_header_str;
    send_rpc_str += args_str;

    // Test
    std::cout << "===================================================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_size: " << args_size << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "===================================================" << std::endl; 

    // 读取配置文件的rpc信息
    // std::string ip = MprpcApplication::GetInstance().getConfig().Load("rpcserverip");
    // u_int16_t port = atoi(MprpcApplication::GetInstance().getConfig().Load("rpcserverport").c_str());

    // 增加了zk之后只需要向zk提供节点名 /service/method 即可返回节点信息 ip:port
    ZkClient zkCli;
    zkCli.Start();

    std::string method_path = "/" + service_name + "/" + method_name;
    std::string host_data = zkCli.GetData(method_path.c_str());

    if (host_data.empty()) {
        controller->SetFailed(method_path + "is not exist!");
        return;
    }
    int index = host_data.find(":");
    if (-1 == index) {
        controller->SetFailed("method_path is invalid!");
        return;
    }

    std::string ip = host_data.substr(0, index);
    uint16_t port = atoi(host_data.substr(index + 1).c_str());

    // 使用tcp编程发送rpc请求
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd) {
        // std::cout << "create socket error. errno:" << errno << std::endl;
        char errtxt[512] = {0};
        sprintf(errtxt, "create socket error:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        // std::cout << "connect error. errno:" << errno << std::endl;
        char errtxt[512] = {0};
        sprintf(errtxt, "connect error. errno:%d", errno);
        controller->SetFailed(errtxt);
        close(clientfd);
        return;
    }

    if (-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0)) {
        // std::cout << "send error. errno:" << errno << std::endl;
        char errtxt[512] = {0};
        sprintf(errtxt, "send error. errno:%d", errno);
        controller->SetFailed(errtxt);
        close(clientfd);
        return;
    }

    // 接受rpc响应
    char rev_buf[1024] = {0};
    u_int32_t rev_size = 0;
    if (-1 == (rev_size = recv(clientfd, rev_buf, 1024, 0))) {
        // std::cout << "recv error. errno:" << errno << std::endl;
        char errtxt[512] = {0};
        sprintf(errtxt, "recv error. errno:%d", errno);
        controller->SetFailed(errtxt);
        close(clientfd);
        return;
    }
    // 反序列化响应
    // std::string respone_str(rev_buf, 0, rev_size);
    if (!response->ParseFromArray(rev_buf, rev_size)) {
        // std::cout << "parse respone error. respone_str:" << std::endl;
        char errtxt[512] = {0};
        sprintf(errtxt, "parse respone error. respone_str:%s", rev_buf);
        controller->SetFailed(errtxt);
        close(clientfd);
        return;
    }

    close(clientfd);
}