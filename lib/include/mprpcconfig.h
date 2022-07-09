#pragma once

#include <iostream>
#include <unordered_map>
#include <string>

class MprpcConfig {

public:
    // 负责加载配置文件
    void LoadConfigFile(const char* config_file);
    // 查询配置项信息
    std::string Load(const std::string& key);
private:
    std::unordered_map<std::string, std::string> m_configMap;
    // 去掉字符串的前后空格
    void Trim(std::string& str_buf);
};