#include "SystemExecutor.h"
#include <windows.h>
#include <powrprof.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "PowrProf.lib")

static std::wstring toWide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], len);
    return w;
}

static bool launchCmd(const std::string& command, bool blocking) {
    std::string cmdStr  = "cmd.exe /C " + command;
    std::wstring wCmd   = toWide(cmdStr);
    std::vector<wchar_t> buf(wCmd.begin(), wCmd.end());
    buf.push_back(L'\0');

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    BOOL ok = CreateProcessW(nullptr, buf.data(),
                             nullptr, nullptr, FALSE, 0,
                             nullptr, nullptr, &si, &pi);
    if (!ok) {
        std::cout << "[Executor] Failed to launch: " << command << "\n";
        return false;
    }

    if (blocking) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return exitCode == 0;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

// ─────────────────────────────────────────────────────────────
//  RUN  (blocking)
// ─────────────────────────────────────────────────────────────
bool SystemExecutor::run(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "[Executor] No command provided.\n";
        return false;
    }

    std::string command;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) command += " ";
        command += args[i];
    }

    std::cout << "[Executor] Running: " << command << "\n";
    return launchCmd(command, true);
}

// ─────────────────────────────────────────────────────────────
//  RUN ASYNC  (fire and forget)
// ─────────────────────────────────────────────────────────────
bool SystemExecutor::runAsync(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "[Executor] No command provided.\n";
        return false;
    }

    std::string command;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) command += " ";
        command += args[i];
    }

    std::cout << "[Executor] Running async: " << command << "\n";
    return launchCmd(command, false);
}

// ─────────────────────────────────────────────────────────────
//  SHUTDOWN  (60 second grace, can be cancelled with "shutdown /a")
// ─────────────────────────────────────────────────────────────
bool SystemExecutor::shutdown() {
    std::cout << "[Executor] Shutting down in 60 seconds. Run 'shutdown /a' to cancel.\n";
    return launchCmd("shutdown /s /t 60", false);
}

// ─────────────────────────────────────────────────────────────
//  RESTART
// ─────────────────────────────────────────────────────────────
bool SystemExecutor::restart() {
    std::cout << "[Executor] Restarting in 60 seconds.\n";
    return launchCmd("shutdown /r /t 60", false);
}

// ─────────────────────────────────────────────────────────────
//  SLEEP  (uses SetSuspendState via PowrProf)
// ─────────────────────────────────────────────────────────────
bool SystemExecutor::sleep() {
    std::cout << "[Executor] Sleeping...\n";
    // hibernate=FALSE, forceCritical=FALSE, disableWakeEvent=FALSE
    BOOL ok = SetSuspendState(FALSE, FALSE, FALSE);
    if (!ok) {
        std::cout << "[Executor] Sleep failed. Trying via rundll32...\n";
        return launchCmd("rundll32.exe powrprof.dll,SetSuspendState 0,1,0", false);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────
//  LOCK  (locks the workstation immediately)
// ─────────────────────────────────────────────────────────────
bool SystemExecutor::lock() {
    std::cout << "[Executor] Locking workstation.\n";
    BOOL ok = LockWorkStation();
    if (!ok) {
        std::cout << "[Executor] LockWorkStation failed.\n";
        return false;
    }
    return true;
}
