#pragma once
#include <vector>
#include <string>

class SystemExecutor {
public:
    // Run a shell command (blocking — waits for it to finish)
    bool run(const std::vector<std::string>& args);

    // Run a shell command (non-blocking — fire and forget)
    bool runAsync(const std::vector<std::string>& args);

    // Power control
    bool shutdown();
    bool restart();
    bool sleep();
    bool lock();
};
