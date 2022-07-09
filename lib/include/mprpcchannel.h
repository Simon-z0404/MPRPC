#pragma once

#include <google/protobuf/service.h>
#include <iostream>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <iostream>
#include <string>
#include <sys/types.h>        
#include <sys/socket.h>


/*
    数据格式: header_size + service_name + method_name + args_size + args
*/

class MprpcChannel : public google::protobuf::RpcChannel
{
public:
    // 重写纯虚函数的方法
    // 通过所有stub 调用的rpc方法 都走到了这里 统一做rpc方法的序列化和网络发送
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                          google::protobuf::Message* response, google::protobuf::Closure* done);
};

