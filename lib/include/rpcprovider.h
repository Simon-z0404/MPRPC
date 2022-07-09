#pragma once
#include <google/protobuf/service.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <functional>
#include <google/protobuf/descriptor.h>
#include <unordered_map>

// 框架提供的专门用于发布rpc服务的网络对象类
class RpcProvider
{
public:
    // 框架提供给外部使用 可以发布RPC方法
    void NotifyService(google::protobuf::Service *service);

    // 启动一个RPC服务发布节点 开始提供RPC远程网络调用服务
    void run();
private:
    std::unique_ptr<muduo::net::TcpServer> m_tcpserverPtr; 

    // 组合EventLoop;
    muduo::net::EventLoop m_eventLoop;

    // 新的socket连接回调
    void OnConnection(const muduo::net::TcpConnectionPtr&);

    // 已建立连接用户的读写回调函数
    void OnMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*, muduo::Timestamp);

    // service服务类型信息
    struct ServiceInfo
    {
        google::protobuf::Service *m_service; // 保存服务对象
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> 
            m_methondMap; // 保存服务方法
    };
    // 映射表：注册成功的服务对象与其服务方法的所有信息
    std::unordered_map<std::string, ServiceInfo> m_serviceMap;    
    // Closure的回调函数，用于序列化rpc的响应和网络发送
    void SendRpcRespone(const muduo::net::TcpConnectionPtr&, google::protobuf::Message*);                                          
};

