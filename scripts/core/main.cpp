#include "Config.h"
#include "ProcessManager.h"
#include "Utils.h"
#include <iostream>
#include <windows.h>
#include <fcntl.h>
#include <io.h>

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
    case CTRL_CLOSE_EVENT:
        Logger::Log(L"Console close event detected, ignoring to keep process running in background.", Logger::Level::WARN);
        return TRUE;
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        return TRUE;
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
    default:
        return FALSE;
    }
}
int main() {
    _setmode(_fileno(stdout), _O_U16TEXT); // ignore warnings, they stink
    if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
        Logger::Log(L"FATAL ERROR: Could not set console control handler.", Logger::Level::ERROR);
        Sleep(5000);
        return 1;
    }
    HWND console_window = GetConsoleWindow();
    Config config;
    if (!config.load(".\\config\\config.ini")) {
        Logger::Log(L"Configuration file 'config.ini' not found or failed to load. Please create it.", Logger::Level::ERROR);
        Logger::Log(L"Press any key to exit.", Logger::Level::INFO);
        std::wcin.get();
        return 1; 
    }
    ProcessManager process_manager(config.get());
    //ShowWindow(console_window, SW_HIDE); // leave commented for debugging
    process_manager.main_loop();

    return 0; // won't be reached. yet.
    // termiante via client or something idk.
}