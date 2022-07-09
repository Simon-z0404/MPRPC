

# 基于muduo和protobuf的微服务RPC框架

## [框架结构]

![框架示意图](D:\C++实习秋招学习文件夹\示意图\MPRPC\MPRPC.JPG)

## [功能描述]：
本项目主要是基于muduo+protobuf+zookeeper的微服务RPC框架。服务方可以通过mprpc框架注册rpc节点发布服务。客户端可以通过zookeeper服务配置中心发现服务，并进行远程方法的调用。

## [开发环境]：
1. protobuf 3.11.0
2. gcc version 7.5.0
3. Zookeeper version: 3.4.10


## [测试DEMO]：
位于bin目录下的provider和consumer。

    ./provider -i test.conf 
    ./consumer -i test.conf

