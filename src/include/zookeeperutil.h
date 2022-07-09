#pragma once
#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <string>

// 封装zk的客户端类
class ZkClient {
public:
    ZkClient();
    ~ZkClient();
    // zkClient启动连接zkServer
    void Start();
    // 在zkserver上根据指定的path创建znode节点
    void Create(const char* path, const char* data, int datalen, int state=0);
    // 根据参数指定znode节点路径，获取znode节点的值
    std::string GetData(const char* path);
private:
    zhandle_t* m_zhandle;
};