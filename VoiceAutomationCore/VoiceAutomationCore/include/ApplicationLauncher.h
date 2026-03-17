#pragma once
#include <vector>
#include <string>
#include <unordered_map>

class ApplicationLauncher {
public:
    ApplicationLauncher();  // loads app alias map

    // Open an app by name or alias (e.g. "chrome", "notepad", "vs code")
    bool open(const std::vector<std::string>& args);

    // Open an app with extra arguments (e.g. open chrome https://google.com)
    bool openWithArgs(const std::string& exe, const std::string& params);

    // Close an app by name
    bool close(const std::vector<std::string>& args);

    // Check if a process is currently running
    bool isRunning(const std::string& exeName);

    // Returns exact process name as seen in Task Manager, empty if not found
    std::string findRunningProcess(const std::string& exeName);

private:
    // Maps friendly names → real executable names
    std::unordered_map<std::string, std::string> appAliases;

    void loadAliases();
    std::string resolveAlias(const std::string& name) const;
    std::string resolveExecutable(const std::string& exeName) const;
    std::string normalizeExe(std::string name) const;
};
