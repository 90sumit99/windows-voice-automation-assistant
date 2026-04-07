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

static bool launchCmd(const std::string& command, bool blocking, bool captureOutput = false) {
    std::string cmdStr  = "cmd.exe /C " + command;
    std::wstring wCmd   = toWide(cmdStr);
    std::vector<wchar_t> buf(wCmd.begin(), wCmd.end());
    buf.push_back(L'\0');

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (captureOutput) {
        if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) {
            std::cout << "[Executor] Failed to create output pipe.\n";
            return false;
        }
        SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = writePipe;
        si.hStdError = writePipe;
    }

    BOOL ok = CreateProcessW(nullptr, buf.data(),
                             nullptr, nullptr, captureOutput ? TRUE : FALSE, 0,
                             nullptr, nullptr, &si, &pi);
    if (captureOutput && writePipe) {
        CloseHandle(writePipe);
        writePipe = nullptr;
    }
    if (!ok) {
        std::cout << "[Executor] Failed to launch: " << command << "\n";
        if (readPipe) CloseHandle(readPipe);
        return false;
    }

    if (blocking) {
        if (captureOutput && readPipe) {
            char outBuf[4096];
            DWORD bytesRead = 0;
            while (ReadFile(readPipe, outBuf, sizeof(outBuf) - 1, &bytesRead, nullptr) && bytesRead > 0) {
                outBuf[bytesRead] = '\0';
                std::cout << "[CMD-OUT] " << outBuf;
            }
            CloseHandle(readPipe);
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        std::cout << "[Executor] Exit code: " << exitCode << "\n";
        return exitCode == 0;
    }

    if (readPipe) CloseHandle(readPipe);
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
    return launchCmd(command, true, true);
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
    return launchCmd(command, false, false);
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
