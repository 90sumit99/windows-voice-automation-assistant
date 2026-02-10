#include "SystemExecutor.h"
#include <windows.h>
#include <string>
#include <iostream>

bool SystemExecutor::run(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "No command provided.\n";
        return false;
    }

    // Join arguments into a single command string
    std::string command;
    for (const auto& arg : args) {
        command += arg + " ";
    }

    // Prepare command line for cmd.exe
    std::string cmdLine = "cmd.exe /C " + command;

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};

    si.cb = sizeof(si);

    // IMPORTANT:
    // Flags = 0 ? allows console output to be visible
    BOOL success = CreateProcessA(
        nullptr,
        cmdLine.data(),   // mutable buffer required
        nullptr,
        nullptr,
        FALSE,
        0,                // <-- DO NOT use CREATE_NO_WINDOW
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!success) {
        std::cout << "Failed to execute command.\n";
        return false;
    }

    // Wait until the command finishes
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}
