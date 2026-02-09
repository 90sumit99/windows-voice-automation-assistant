#include "ApplicationLauncher.h"
#include <windows.h>
#include <algorithm>
#include <iostream>


static std::string normalizeExe(std::string name) {
    // remove spaces (basic fix)
    name.erase(std::remove(name.begin(), name.end(), ' '), name.end());

    // add .exe if missing
    if (name.size() < 4 || name.substr(name.size() - 4) != ".exe") {
        name += ".exe";
    }
    return name;
}

// Helper function to resolve an executable's full path using the system PATH
static std::string resolveExecutable(const std::string& exeName) {
    char buffer[MAX_PATH];
    DWORD result = SearchPathA(
        nullptr,           // search path (nullptr means use PATH env variable)
        exeName.c_str(),   // file name
        nullptr,           // extension
        MAX_PATH,          // buffer size
        buffer,            // buffer
        nullptr            // file part
    );
    if (result > 0 && result < MAX_PATH) {
        return std::string(buffer);
    }
    return "";
}

bool ApplicationLauncher::open(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "No application name provided.\n";
        return false;
    }

    std::string exe = normalizeExe(args[0]);

    // 1. Try direct execution
    HINSTANCE result = ShellExecuteA(
        nullptr,
        "open",
        exe.c_str(),
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );

    if (reinterpret_cast<intptr_t>(result) > 32) {
        return true;
    }

    // 2. Try PATH lookup
    std::string fullPath = resolveExecutable(exe);
    if (!fullPath.empty()) {
        result = ShellExecuteA(
            nullptr,
            "open",
            fullPath.c_str(),
            nullptr,
            nullptr,
            SW_SHOWNORMAL
        );

        if (reinterpret_cast<intptr_t>(result) > 32) {
            return true;
        }
    }

    // 3. If we reach here ? app not found
    std::cout << "Application not found or cannot be launched.\n";
    return false;
}


bool ApplicationLauncher::close(const std::vector<std::string>& args) {
    if (args.empty()) return false;

    std::string exe = normalizeExe(args[0]);
    std::string cmd = "taskkill /IM " + exe + " /F";

    int result = system(cmd.c_str());

    if (result != 0) {
        // Likely UWP app or protected system app
        std::cout << "Cannot close this application (UWP or protected app).\n";
        return false;
    }

    return true;
}
