#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <vector>
#include <unordered_set>

using ProcessSet = std::unordered_set<std::wstring>;

struct ConfigData {
    int pollingInterval = 1000; 
    ProcessSet game_list;
    ProcessSet suspend_list;
    ProcessSet whitelist;
};

class Config {
public:
    bool load(const std::string& filename);
    const ConfigData& get() const;

private:
    ConfigData data;
    std::string trim(const std::string& str);
    std::wstring to_lower_wide(const std::string& str);
};