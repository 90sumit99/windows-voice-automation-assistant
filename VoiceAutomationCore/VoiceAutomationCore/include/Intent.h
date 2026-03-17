#pragma once
#include <string>
#include <vector>

enum class IntentType {
    UNKNOWN,

    // Application control
    OPEN_APP,
    CLOSE_APP,

    // Volume control
    SET_VOLUME,
    MUTE_VOLUME,
    UNMUTE_VOLUME,
    VOLUME_UP,
    VOLUME_DOWN,

    // Media keys
    MEDIA_PLAY_PAUSE,
    MEDIA_NEXT,
    MEDIA_PREV,
    PLAY_MUSIC,         // play <song> on <target>

    // File operations
    FIND_FILE,
    OPEN_FILE,
    DELETE_FILE,
    RENAME_FILE,

    // System commands
    RUN_COMMAND,
    SHUTDOWN,
    RESTART,
    SLEEP,
    LOCK,

    // Screen / UI
    TAKE_SCREENSHOT,
    SET_BRIGHTNESS,

    // Clipboard
    COPY_TEXT,
    PASTE_TEXT,

    // Selection (after find shows list)
    SELECT_OPTION,

    // Exit Zyrex
    EXIT_APP
};

struct Intent {
    IntentType               type = IntentType::UNKNOWN;
    std::vector<std::string> args;
    std::string              rawInput;  // original user input, useful for logging
};
