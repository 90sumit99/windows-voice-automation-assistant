#pragma once

#include <string>

#include "CommandParser.h"
#include "CommandRegistry.h"
#include "IntentResolver.h"
#include "PolicyEngine.h"
#include "Logger.h"

class InputRouter {
public:
    InputRouter(CommandRegistry& registry,
        PolicyEngine& policy,
        Logger& logger);

    // Processes one user input cycle
    // returns false when application should exit
    bool processOnce();

private:
    CommandParser parser;
    IntentResolver resolver;

    CommandRegistry& registry;
    PolicyEngine& policy;
    Logger& logger;
};
