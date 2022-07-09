#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include <iostream>

// 全局的watcher观察器 zksever给zkclient的通知
void global_wather(zhandle_t* zh, int type, 
                    int state, const char* path, void* watcherCtx) 
{
    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            sem_t *sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);
        }
    }
}

ZkClient::ZkClient() : m_zhandle(nullptr) {

}

ZkClient::~ZkClient() {
    if (m_zhandle != nullptr) {
        zookeeper_close(m_zhandle);
    }
}

// zkClient启动连接zkServer
void ZkClient::Start() {
    std::string host = MprpcApplication::GetInstance().getConfig().Load("zookeeperip");
    std::string ip = MprpcApplication::GetInstance().getConfig().Load("zookeeperport");
    std::string connstr = host + ":" + ip;

    m_zhandle = zookeeper_init(connstr.c_str(), global_wather, 30000, nullptr, nullptr, 0);
    if (nullptr == m_zhandle) {
        // LOG_ERR("zookeeper init error!");
        // std::cout << "zookeeper init error!" << std::endl;
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhandle, &sem);

    sem_wait(&sem);
    std::cout << "zookeeper init success!" << std::endl;
    // LOG_INFO("zookeeper init success!");
}

// 在zkserver上根据指定的path创建znode节点
void ZkClient::Create(const char* path, const char* data, int datalen, int state) {
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
    // 先判断节点是否存在
    flag = zoo_exists(m_zhandle, path, 0, nullptr);
    if (ZNONODE == flag) {
        flag = zoo_create(m_zhandle, path, data, datalen, 
                            &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);

        if (ZOK == flag) {
            std::cout << "znode create success! path:" << path << std::endl;
            // LOG_INFO("znode create success! path:%s", path);
        } else {
            std::cout << "flag:" << flag << std::endl;
            std::cout << "znode create error! path:" << path <<std::endl;
            // LOG_ERR("flag:%d", flag);
            // LOG_ERR("znode create error! path:%s", path);
            exit(EXIT_FAILURE);
        }
    }
}   

// 根据参数指定znode节点路径，获取znode节点的值
std::string ZkClient::GetData(const char* path) {
    char buffer[64] = {0};
    int buffer_len = sizeof(buffer);
    int flag = zoo_get(m_zhandle, path, 0, buffer, &buffer_len, nullptr);
    if (ZOK != flag) {
        std::cout << "get znode error!" << std::endl;
        // LOG_ERR("get znode error!");
        return "";
    } else {
        return buffer;
    }
}