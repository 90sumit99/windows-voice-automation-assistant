#pragma once
#include "CommandRegistry.h"
#include "PolicyEngine.h"
#include "Logger.h"
#include "IntentResolver.h"
#include "CommandParser.h"
#include "FileSystemController.h"

class InputRouter {
public:
    InputRouter(CommandRegistry& registry,
        PolicyEngine& policy,
        Logger& logger,
        FileSystemController& fileSystem);

    // Process one command from stdin
    // Returns false only when Zyrex should exit
    bool processOnce();

private:
    CommandRegistry& registry;
    PolicyEngine& policy;
    Logger& logger;
    FileSystemController& fileSystem;

    CommandParser   parser;
    IntentResolver  resolver;

    // Pretty print the prompt
    void printPrompt() const;

    // Print unknown command help
    void printUnknownHelp() const;

    // Map intent type to registry command string
    const char* intentToCommand(IntentType type) const;
};