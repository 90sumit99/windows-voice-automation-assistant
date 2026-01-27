#pragma once
#include <string>
#include <unordered_map>
#include <functional>

class CommandRegistry {
public:
    using CommandHandler = std::function<void()>;

    bool registerCommand(const std::string& command, CommandHandler handler);
    bool executeCommand(const std::string& command) const;

private:
    std::unordered_map<std::string, CommandHandler> commandMap;
};
