#include "CommandRegistry.h"
#include <iostream>

bool CommandRegistry::registerCommand(const std::string& command, CommandHandler handler) {
    return commandMap.emplace(command, handler).second;
}

bool CommandRegistry::executeCommand(const std::string& command) const {
    auto it = commandMap.find(command);
    if (it == commandMap.end()) {
        std::cout << "Command blocked or not found\n";
        return false;
    }
    it->second();
    return true;
}
