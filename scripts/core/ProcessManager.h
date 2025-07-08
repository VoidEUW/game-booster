#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "Config.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <winternl.h>

typedef NTSTATUS(NTAPI* pfnNtSuspendProcess)(IN HANDLE ProcessHandle);
typedef NTSTATUS(NTAPI* pfnNtResumeProcess)(IN HANDLE ProcessHandle);

class ProcessManager {
public:
    ProcessManager(const ConfigData& config);

    void main_loop();

private:
    enum class State {
        IDLE,
        BOOSTING
    };

    void check_state();
    void start_boosting(const std::wstring& game_process_name);
    void stop_boosting();
    std::wstring get_active_proc_name();

    void suspend_target_processes(const std::wstring& game_process_name);
    void resume_targeted_processes();
    bool suspend_process(DWORD processID);
    bool resume_process(DWORD processID);

    const ConfigData& config;
    State current_state;

    std::vector<DWORD> suspended_pids;

    pfnNtSuspendProcess nt_suspend_process;
    pfnNtResumeProcess nt_resume_process;
};