#pragma once
#include <string>
#include <unordered_set>

class SecurityValidator {
public:
    bool isSafe(const std::string& command) const {
        return allowedCommands.find(command) != allowedCommands.end();
    }

private:
    const std::unordered_set<std::string> allowedCommands{
        "open",
        "close",
        "volume",
        "mute",
        "unmute",
        "exit"
    };
};
