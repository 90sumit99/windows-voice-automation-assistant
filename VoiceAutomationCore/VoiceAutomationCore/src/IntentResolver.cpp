#include "IntentResolver.h"

Intent IntentResolver::resolve(const std::string& command,
    const std::vector<std::string>& args) {
    Intent intent;

    if (command == "open") {
        intent.type = IntentType::OPEN_APP;
        intent.args = args;
    }
    else if (command == "close") {
        intent.type = IntentType::CLOSE_APP;
        intent.args = args;
    }
    else if (command == "volume") {
        intent.type = IntentType::SET_VOLUME;
        intent.args = args;
    }
    else if (command == "mute") {
        intent.type = IntentType::MUTE_VOLUME;
    }
    else if (command == "unmute") {
        intent.type = IntentType::UNMUTE_VOLUME;
    }
    else if (command == "find") {
        intent.type = IntentType::FIND_FILE;
        intent.args = args;
    }
    else if (command == "run") {
        intent.type = IntentType::RUN_COMMAND;
        intent.args = args;
    }
    else if (command == "exit") {
        intent.type = IntentType::EXIT_APP;
    }
    else if (command == "openfile") {
        intent.type = IntentType::OPEN_FILE;
        intent.args = args;
    }

    else {
        intent.type = IntentType::UNKNOWN;
    }

    return intent;
}
