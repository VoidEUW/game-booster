#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <windows.h>

#undef ERROR // conflicts with our ERROR enum. we can either change our enum or just keep this undefed. not like we need it. yet.
class Logger {
public:
    enum class Level {
        INFO,
        SUCCESS,
        WARN,
        ERROR
    };

    static void Log(const std::wstring& message, Level level = Level::INFO) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
        WORD saved_attributes = consoleInfo.wAttributes;

        std::wstring prefix;
        switch (level) {
        case Level::INFO:
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            prefix = L"[INFO]   ";
            break;
        case Level::SUCCESS:
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            prefix = L"[SUCCESS]";
            break;
        case Level::WARN:
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            prefix = L"[WARN]   ";
            break;
        case Level::ERROR:
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
            prefix = L"[ERROR]  ";
            break;
        }

        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::wstringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), L"%Y-%m-%d %X");

        std::wcout << L"[" << ss.str() << L"] " << prefix << L" " << message << std::endl;

        SetConsoleTextAttribute(hConsole, saved_attributes);
    }
};