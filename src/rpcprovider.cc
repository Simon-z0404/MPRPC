#include "rpcprovider.h"
#include "mprpcapplication.h"
#include "logger.h"
#include "rpcheader.pb.h"
#include "zookeeperutil.h"

// 框架提供给外部使用 可以发布RPC方法
void RpcProvider::NotifyService(google::protobuf::Service *service) {
    this->m_newServiceQue.Push(service);
}

void RpcProvider::NotifyThread() { 
    // 把当前RPC节点上要发布的服务注册到ZK上 让RPC client可以在zk上发现rpc服务
    ZkClient zkCli;
    zkCli.Start();

        // 读取配置文件的rpc信息
    std::string ip = MprpcApplication::GetInstance().getConfig().Load("rpcserverip");
    u_int16_t port = atoi(MprpcApplication::GetInstance().getConfig().Load("rpcserverport").c_str());
    
    while (true) {
        google::protobuf::Service *service = m_newServiceQue.Pop();

        m_newServiceQue.Push(service);
        // 获取服务对象的描述信息
        const google::protobuf::ServiceDescriptor* pserviceDesc = service->GetDescriptor();
        // 获取服务的名字
        std::string service_name = pserviceDesc->name();
        // 获取服务方法的数量
        int methodCnt = pserviceDesc->method_count();

        LOG_INFO("service name:%s", service_name.c_str( ));

        ServiceInfo service_info;

        for (int i = 0; i < methodCnt; ++i) {
            // 获取服务对象指定下标的方法描述
            const google::protobuf::MethodDescriptor* pmethodDesc = pserviceDesc->method(i);
            std::string methodName = pmethodDesc->name();
            service_info.m_methondMap.insert({methodName, pmethodDesc});
        }
        service_info.m_service = service;
        m_serviceMap.insert({service_name, service_info});

        // service_name 为永久性节点 methond name为临时性节点
        for (auto &sp : m_serviceMap) {
            // service name
            std::string service_path = "/" + sp.first;
            zkCli.Create(service_path.c_str(), nullptr, 0); // state为0 默认为临时性节点
            for (auto &mp : sp.second.m_methondMap) {
                std::string method_path = service_path + "/" + mp.first;
                char method_path_data[128] = {0};
                sprintf(method_path_data, "%s:%d", ip.c_str(), port);
                zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), 
                            ZOO_EPHEMERAL);
            }
        }
    }   
}

// 启动一个RPC服务发布节点 开始提供RPC远程网络调用服务
void RpcProvider::run() {
    // 开启子线程从服务注册队列中获取需要注册的服务，注册在ZK上
    std::thread Notify_Thread(std::bind(&RpcProvider::NotifyThread, this));
    Notify_Thread.detach();
    
    // 读取配置文件的rpc信息
    std::string ip = MprpcApplication::GetInstance().getConfig().Load("rpcserverip");
    u_int16_t port = atoi(MprpcApplication::GetInstance().getConfig().Load("rpcserverport").c_str()); 
    muduo::net::InetAddress address(ip, port);

    // 创建TCP对象
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");
    // 绑定连接回调和消息读写回调方法 分离网络和业务代码
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1, 
        std::placeholders::_2, std::placeholders::_3));
    
    // 设置muduo库的线程数量
    server.setThreadNum(3);
    // 启动网络服务
    server.start();
    m_eventLoop.loop();
}


// 新的socket连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn) {
    
    if (!conn->connected()) {
        // 和rpc的连接断开了 关闭连接
        conn->shutdown();
    }
}

/*
    在框架内部RpcProvider和RpcComsumer需要协商好通信的protobuf格式
    service name => method => args
    因此，需要定义proto的数据格式（message）进行序列化和反序列化
*/

 // 已建立连接用户的读写回调函数
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buffer, muduo::Timestamp) {
    // 网络上远程调用rpc请求的字节流
    std::string rev_buf = buffer->retrieveAllAsString();
    // 读取前面的4个字节作为header size
    u_int32_t header_size = 0;
    rev_buf.copy((char*)&header_size, 4, 0); 
    // 根据header size得到数据头 
    std::string rpc_header_str = rev_buf.substr(4, header_size);
    // 进行反序列化
    std::string service_name;
    std::string method_name;
    u_int32_t args_size;
    mprpc::RpcHeader rpcheader;
    if (rpcheader.ParseFromString(rpc_header_str)) {
        // 反序列化成功
        service_name = rpcheader.service_name();
        method_name = rpcheader.method_name();
        args_size = rpcheader.args_size();
    } else {
        // 反序列化失败
        // std::cout << "rpc_header_str:" << rpc_header_str << "parse error" << std::endl; 
        // LOG_ERR("rpc_header_str:%s\t parse error", rpc_header_str.c_str());
        return;
    }

    std::string args_str = rev_buf.substr(4 + header_size, args_size);

    // // Test
    // std::cout << "===================================================" << std::endl;
    // std::cout << "header_size: " << header_size << std::endl;
    // std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    // std::cout << "service_name: " << service_name << std::endl;
    // std::cout << "method_name: " << method_name << std::endl;
    // std::cout << "args_size: " << args_size << std::endl;
    // std::cout << "args_str: " << args_str << std::endl;
    // std::cout << "===================================================" << std::endl; 
    
    // 验证服务对象是否存在
    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end()) {
        // std::cout << "service name is not exist!" << std::endl;
        LOG_ERR("service name is not exist!");
        return;
    }
    // 验证服务对象的方法是否存在
    auto mit = it->second.m_methondMap.find(method_name);
    if (mit == it->second.m_methondMap.end()) {
        std::cout << service_name << ":" << method_name << " is not exist!" << std::endl;
        return;
    }

    // 获取服务对象
    google::protobuf::Service *service = it->second.m_service;
    // 获取服务方法
    const google::protobuf::MethodDescriptor *method = mit->second; 

    // 生成rpc方法调用的request和respone参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    google::protobuf::Message *respone = service->GetResponsePrototype(method).New();

    if (!request->ParseFromString(args_str)) {
        // std::cout << "request args parse error! content:" << args_str << std::endl;
        LOG_ERR("request args parse error! content:%s", args_str.c_str());
        return;
    }

    // 给下面的method调用
    google::protobuf::Closure* done = 
                        google::protobuf::NewCallback<RpcProvider, 
                        const muduo::net::TcpConnectionPtr&, 
                        google::protobuf::Message*>
                        (this, &RpcProvider::SendRpcRespone, conn, respone);

    // 框架根据远端的rpc请求，调用当前节点上RPC发布的方法 
    service->CallMethod(method, nullptr, request, respone, done);
}

// callee最后调用的done方法会跑到这儿过来
// Closure的回调函数，用于序列化rpc的响应和网络发送
void RpcProvider::SendRpcRespone(const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* respone) {
    std::string respone_str;
    if (respone->SerializeToString(&respone_str)) { // 对respone进行序列化
        // 序列化成功，通过网络把rpc请求方法的结果返回
        conn->send(respone_str);
        
    } else {
        // std::cout << "serialize responese error!" << std::endl; 
        LOG_ERR("serialize responese error!");
    }
    // 模拟http的短链接服务（提供方主动断开），给更多的用户提供服务
    conn->shutdown(); 
}