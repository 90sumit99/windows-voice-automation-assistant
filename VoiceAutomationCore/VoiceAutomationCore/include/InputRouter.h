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

    bool processOnce();

private:
    CommandRegistry& registry;
    PolicyEngine& policy;
    Logger& logger;
    FileSystemController& fileSystem;   

    CommandParser parser;
    IntentResolver resolver;
};
