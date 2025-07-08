#include "Config.h"
#include "Utils.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <Windows.h> 

bool Config::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::Log(L"Could not open config file: " + std::wstring(filename.begin(), filename.end()), Logger::Level::ERROR);
        return false;
    }

    std::string line;
    std::string currentSection;

    while (std::getline(file, line)) {
        line = trim(line);

        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }

        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }

        size_t equalsPos = line.find('=');
        if (equalsPos != std::string::npos) {
            std::string key = trim(line.substr(0, equalsPos));
            std::string value = trim(line.substr(equalsPos + 1));

            std::wstring wideValue = to_lower_wide(value);

            if (currentSection == "Settings" && key == "PollingInterval") {
                try {
                    data.pollingInterval = std::stoi(value);
                }
                catch (const std::invalid_argument&) {
                    Logger::Log(L"Invalid PollingInterval value: " + std::wstring(value.begin(), value.end()), Logger::Level::WARN);
                }
            }
            else if (currentSection == "Games") {
                data.game_list.insert(wideValue);
            }
            else if (currentSection == "SuspendList") {
                data.suspend_list.insert(wideValue);
            }
            else if (currentSection == "Whitelist") {
                data.whitelist.insert(wideValue);
            }
        }
    }

    Logger::Log(L"Configuration loaded successfully.", Logger::Level::SUCCESS);
    Logger::Log(L"Polling Interval: " + std::to_wstring(data.pollingInterval) + L"ms");
    Logger::Log(L"Games to monitor: " + std::to_wstring(data.game_list.size()));
    Logger::Log(L"Processes to suspend: " + std::to_wstring(data.suspend_list.size()));
    Logger::Log(L"Whitelisted processes: " + std::to_wstring(data.whitelist.size()));

    return true;
}

const ConfigData& Config::get() const {
    return data;
}

std::string Config::trim(const std::string& str) {
    const std::string whitespace = " \t\r\n";
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos) return ""; 
    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;
    return str.substr(strBegin, strRange);
}

std::wstring Config::to_lower_wide(const std::string& str) {
    if (str.empty()) {
        return L"";
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    if (size_needed <= 0) {
        return L"";
    }

    std::wstring wide_str(size_needed, 0);

    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wide_str[0], size_needed);

    std::transform(wide_str.begin(), wide_str.end(), wide_str.begin(),
        [](wchar_t c) { return towlower(c); });

    return wide_str;
}