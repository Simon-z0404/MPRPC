#include "mprpcconfig.h"
#include <cstdio>
#include <iostream>
#include <string>

// 去掉字符串的前后空格
void MprpcConfig::Trim(std::string& str_buf) {
    int index = str_buf.find_first_not_of(' ');
    if (index != -1) {
        str_buf = str_buf.substr(index);
    }
    index = str_buf.find_last_not_of(' ');
    if (index != -1) {
        str_buf = str_buf.substr(0, index + 1);
    }
}


// 负责加载配置文件
void MprpcConfig::LoadConfigFile(const char* config_file) {
    FILE* pf = fopen(config_file, "r");
    if (nullptr == pf) {
        std::cout << config_file << " is invalid!" << std::endl;
        exit(EXIT_FAILURE);
    }
    // 一次性读取一行字符串
    while (!feof(pf)) {
        char buf[512] = {0};
        fgets(buf, 512, pf);

        std::string read_buf(buf);
        // 1.先去掉字符前后的空格
        Trim(read_buf);

        // 2.判断是不是注释
        if (read_buf.empty() || read_buf[0] == '#') {
            continue;
        }
        // 3.解析配置项
        int index = read_buf.find('=');
        if (-1 == index) {
            // 配置项不合法
            continue;
        }
        std::string key;
        std::string value;
        key = read_buf.substr(0, index);
        // 去掉前后空格
        Trim(key);
        
        int endIndex = read_buf.find('\n', index);
        value = read_buf.substr(index + 1, endIndex - index - 1);
        // 去掉前后空格
        Trim(value);
        m_configMap.insert({key, value});
    }
}

// 查询配置项信息
std::string MprpcConfig::Load(const std::string& key) {
    auto it = m_configMap.find(key);
    if (it == m_configMap.end()) {
        return "";
    }
    return it->second;
}
