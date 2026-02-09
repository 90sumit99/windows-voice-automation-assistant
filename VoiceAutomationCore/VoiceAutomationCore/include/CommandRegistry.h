#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

class CommandRegistry {
public:
    using CommandHandler = std::function<bool(const std::vector<std::string>&)>;

    bool registerCommand(const std::string& name, CommandHandler handler);
    bool execute(const std::string& name, const std::vector<std::string>& args);

private:
    std::unordered_map<std::string, CommandHandler> commands;
};
