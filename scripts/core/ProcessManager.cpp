#define _CRT_SECURE_NO_WARNINGS
#include "ProcessManager.h"
#include "Utils.h"
#include <TlHelp32.h>
#include <algorithm>
#include <locale>
#include <psapi.h>
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

ProcessManager::ProcessManager(const ConfigData& config)
    : config(config), current_state(State::IDLE), nt_suspend_process(nullptr), nt_resume_process(nullptr) {

    HMODULE ntdll = GetModuleHandle(L"ntdll.dll");
    if (ntdll) {
        nt_suspend_process = (pfnNtSuspendProcess)GetProcAddress(ntdll, "NtSuspendProcess");
        if (!nt_suspend_process) {
            Logger::Log(L"Failed to get address of NtSuspendProcess.", Logger::Level::ERROR);
        }

        nt_resume_process = (pfnNtResumeProcess)GetProcAddress(ntdll, "NtResumeProcess");
        if (!nt_resume_process) {
            Logger::Log(L"Failed to get address of NtResumeProcess.", Logger::Level::ERROR);
        }
    }
    else {
        Logger::Log(L"Failed to get handle to ntdll.dll.", Logger::Level::ERROR);
    }
}

void ProcessManager::main_loop() {
    if (!nt_suspend_process || !nt_resume_process) {
        Logger::Log(L"Cannot run main loop: suspension/resumption functions not available. Exiting.", Logger::Level::ERROR);
        return;
    }

    Logger::Log(L"Starting main monitoring loop...", Logger::Level::INFO);
    while (true) {
        check_state();
        Sleep(config.pollingInterval);
    }
}

void ProcessManager::check_state() {
    std::wstring foreground_process = get_active_proc_name();
    bool game_active = false;
    if (!foreground_process.empty()) {
        if (config.game_list.count(foreground_process)) {
            game_active = true;
        }
    }

    if (game_active && current_state == State::IDLE) {
        start_boosting(foreground_process);
    }
    else if (!game_active && current_state == State::BOOSTING) {
        stop_boosting();
    }
}

void ProcessManager::start_boosting(const std::wstring& game_process_name) {
    Logger::Log(L"Game detected: " + game_process_name + L". Activating boost mode.", Logger::Level::SUCCESS);
    current_state = State::BOOSTING;
    suspend_target_processes(game_process_name);
}

void ProcessManager::stop_boosting() {
    Logger::Log(L"Game no longer in foreground. Deactivating boost mode and resuming processes.", Logger::Level::INFO);
    current_state = State::IDLE;
    resume_targeted_processes();
}

std::wstring ProcessManager::get_active_proc_name() {
    HWND foreground_window = GetForegroundWindow();
    if (foreground_window == NULL) {
        return L"";
    }

    DWORD process_id;
    GetWindowThreadProcessId(foreground_window, &process_id);

    HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
    if (process_handle == NULL) {
        return L"";
    }
    TCHAR process_name[MAX_PATH] = L"";
    if (GetProcessImageFileName(process_handle, process_name, MAX_PATH) > 0) {
        std::wstring wide_process_name = PathFindFileName(process_name);
        std::transform(wide_process_name.begin(), wide_process_name.end(), wide_process_name.begin(),
            [](wchar_t c) { return std::tolower(c, std::locale()); });

        CloseHandle(process_handle);
        return wide_process_name;
    }

    CloseHandle(process_handle);
    return L"";
}

void ProcessManager::suspend_target_processes(const std::wstring& game_process_name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        Logger::Log(L"Failed to create process snapshot.", Logger::Level::ERROR);
        return;
    }


    // windows is fun right? right???
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &pe32)) {
        do {
            std::wstring process_name = pe32.szExeFile;
            std::transform(process_name.begin(), process_name.end(), process_name.begin(),
                [](wchar_t c) { return std::tolower(c, std::locale()); });
            if (config.suspend_list.count(process_name) &&
                process_name != game_process_name &&
                pe32.th32ProcessID != GetCurrentProcessId() &&
                !config.whitelist.count(process_name)) {
                if (suspend_process(pe32.th32ProcessID)) {
                    Logger::Log(L"Suspending process: " + std::wstring(pe32.szExeFile) + L" (PID: " + std::to_wstring(pe32.th32ProcessID) + L")", Logger::Level::INFO);
                    suspended_pids.push_back(pe32.th32ProcessID);
                }
            }
        } while (Process32Next(snapshot, &pe32));
    }

    CloseHandle(snapshot);
    Logger::Log(std::to_wstring(suspended_pids.size()) + L" processes suspended.", Logger::Level::SUCCESS);
}

void ProcessManager::resume_targeted_processes() {
    if (suspended_pids.empty()) {
        Logger::Log(L"No processes were suspended, nothing to resume.", Logger::Level::INFO);
        return;
    }

    Logger::Log(L"Resuming " + std::to_wstring(suspended_pids.size()) + L" processes...", Logger::Level::INFO);
    for (DWORD pid : suspended_pids) {
        if (resume_process(pid)) {
            Logger::Log(L"Resumed process with PID: " + std::to_wstring(pid), Logger::Level::INFO);
        }
    }
    suspended_pids.clear();
    Logger::Log(L"All processes resumed.", Logger::Level::SUCCESS);
}

bool ProcessManager::suspend_process(DWORD processID) {
    HANDLE processHandle = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, processID);
    if (processHandle == NULL) {
        Logger::Log(L"Failed to open process PID " + std::to_wstring(processID) + L" for suspension (maybe closed or protected).", Logger::Level::WARN);
        return false;
    }
    NTSTATUS status = nt_suspend_process(processHandle);
    CloseHandle(processHandle);

    if (status != 0) {
        Logger::Log(L"NtSuspendProcess failed for PID " + std::to_wstring(processID), Logger::Level::WARN);
        return false;
    }

    return true;
}

bool ProcessManager::resume_process(DWORD processID) {
    HANDLE processHandle = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, processID);
    if (processHandle == NULL) {
        Logger::Log(L"Failed to open process PID " + std::to_wstring(processID) + L" for resumption (might've already been closed).", Logger::Level::WARN);
        return false;
    }

    NTSTATUS status = nt_resume_process(processHandle);
    CloseHandle(processHandle);

    if (status != 0) {
        Logger::Log(L"NtResumeProcess failed for PID " + std::to_wstring(processID), Logger::Level::WARN);
        return false;
    }

    return true;
}