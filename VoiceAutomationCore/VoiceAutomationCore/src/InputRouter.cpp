#include "InputRouter.h"
#include <iostream>

InputRouter::InputRouter(CommandRegistry& registry,
    PolicyEngine& policy,
    Logger& logger)
    : registry(registry), policy(policy), logger(logger) {
}

bool InputRouter::processOnce() {
    std::string input;
    std::cout << "\n> ";
    std::getline(std::cin, input);

    if (input.empty())
        return true;

    // 1. Parse raw input
    ParsedCommand parsed = parser.parse(input);

    // 2. Resolve intent (WHAT user wants)
    Intent intent = resolver.resolve(parsed.command, parsed.args);

    // 3. Exit handling
    if (intent.type == IntentType::EXIT_APP) {
        logger.log("Zyrex shutting down.");
        return false;
    }

    // 4. Policy check (SECURITY GATE)
    if (!policy.isAllowed(intent)) {
        logger.log("Blocked by policy engine.");
        std::cout << "Permission denied.\n";
        return true;
    }

    // 5. Execute intent via capabilities (NOT tasks)
    bool executed = false;

    switch (intent.type) {

    case IntentType::OPEN_APP:
        executed = registry.execute("open", intent.args);
        break;

    case IntentType::CLOSE_APP:
        executed = registry.execute("close", intent.args);
        break;

    case IntentType::SET_VOLUME:
        executed = registry.execute("volume", intent.args);
        break;

    case IntentType::MUTE_VOLUME:
        executed = registry.execute("mute", {});
        break;

    case IntentType::UNMUTE_VOLUME:
        executed = registry.execute("unmute", {});
        break;

    case IntentType::FIND_FILE:
        executed = registry.execute("find", intent.args);
        break;

    case IntentType::RUN_COMMAND:
        executed = registry.execute("run", intent.args);
        break;

    case IntentType::OPEN_FILE:
        executed = registry.execute("openfile", intent.args);
        break;


    default:
        std::cout << "Unknown command.\n";
        logger.log("Unknown intent.");
        return true;
    }

    // 6. Result handling
    if (executed) {
        logger.log("Command executed successfully.");
    }
    else {
        logger.log("Command execution failed.");
        std::cout << "Execution failed.\n";
    }

    return true;
}
