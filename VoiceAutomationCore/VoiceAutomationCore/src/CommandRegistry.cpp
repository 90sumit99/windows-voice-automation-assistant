#include "CommandRegistry.h"

bool CommandRegistry::registerCommand(const std::string& name,
    CommandHandler handler) {
    commands[name] = handler;
    return true;
}

bool CommandRegistry::execute(const std::string& name,
    const std::vector<std::string>& args) {
    auto it = commands.find(name);
    if (it == commands.end())
        return false;

    return it->second(args);
}
