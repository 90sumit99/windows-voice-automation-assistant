#pragma once
#pragma once
#include <string>
#include <vector>

enum class IntentType {
    UNKNOWN,

    OPEN_APP,
    CLOSE_APP,

    SET_VOLUME,
    MUTE_VOLUME,
    UNMUTE_VOLUME,

    FIND_FILE,
    OPEN_FILE,

    RUN_COMMAND,
    EXIT_APP
};

struct Intent {
    IntentType type = IntentType::UNKNOWN;
    std::vector<std::string> args;
};
