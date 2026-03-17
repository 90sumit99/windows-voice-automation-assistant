#include "InputRouter.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <windows.h>

// ─────────────────────────────────────────────────────────────
//  CONSTRUCTOR
// ─────────────────────────────────────────────────────────────
InputRouter::InputRouter(CommandRegistry&      registry,
                         PolicyEngine&         policy,
                         Logger&               logger,
                         FileSystemController& fileSystem)
    : registry(registry),
      policy(policy),
      logger(logger),
      fileSystem(fileSystem) {
}

// ─────────────────────────────────────────────────────────────
//  PRINT PROMPT
// ─────────────────────────────────────────────────────────────
void InputRouter::printPrompt() const {
    // Cyan color for prompt
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, 11); // bright cyan
    std::cout << "\nZyrex";
    SetConsoleTextAttribute(h, 7);  // white
    std::cout << " > ";
}

// ─────────────────────────────────────────────────────────────
//  UNKNOWN HELP  — shown when Zyrex doesn't understand
// ─────────────────────────────────────────────────────────────
void InputRouter::printUnknownHelp() const {
    std::cout << "\n  I didn't understand that. Try:\n"
              << "    open chrome          / close spotify\n"
              << "    volume 70            / mute / unmute\n"
              << "    volume up            / volume down\n"
              << "    play / next song     / previous song\n"
              << "    find resume.pdf      / open file report\n"
              << "    open 1               / open second\n"
              << "    delete myfile.txt    / rename old new\n"
              << "    shutdown / restart   / sleep / lock\n"
              << "    screenshot           / ip address\n"
              << "    exit\n\n";
}

// ─────────────────────────────────────────────────────────────
//  INTENT → REGISTRY COMMAND STRING
// ─────────────────────────────────────────────────────────────
const char* InputRouter::intentToCommand(IntentType type) const {
    switch (type) {
    case IntentType::OPEN_APP:          return "open";
    case IntentType::CLOSE_APP:         return "close";
    case IntentType::SET_VOLUME:        return "volume";
    case IntentType::MUTE_VOLUME:       return "mute";
    case IntentType::UNMUTE_VOLUME:     return "unmute";
    case IntentType::VOLUME_UP:         return "volumeup";
    case IntentType::VOLUME_DOWN:       return "volumedown";
    case IntentType::MEDIA_PLAY_PAUSE:  return "mediaplay";
    case IntentType::MEDIA_NEXT:        return "medianext";
    case IntentType::MEDIA_PREV:        return "mediaprev";
    case IntentType::PLAY_MUSIC:        return "playmusic";
    case IntentType::FIND_FILE:         return "find";
    case IntentType::OPEN_FILE:         return "openfile";
    case IntentType::DELETE_FILE:       return "deletefile";
    case IntentType::RUN_COMMAND:       return "run";
    case IntentType::SHUTDOWN:          return "shutdown";
    case IntentType::RESTART:           return "restart";
    case IntentType::SLEEP:             return "sleep";
    case IntentType::LOCK:              return "lock";
    case IntentType::TAKE_SCREENSHOT:   return "screenshot";
    default:                            return nullptr;
    }
}

// ─────────────────────────────────────────────────────────────
//  PROCESS ONCE  — main loop body
// ─────────────────────────────────────────────────────────────
bool InputRouter::processOnce() {
    // 1. Show prompt and read input
    printPrompt();

    std::string input;
    if (!std::getline(std::cin, input)) {
        // EOF / pipe closed
        logger.info("Input stream closed. Shutting down.");
        return false;
    }

    // Trim leading/trailing whitespace
    input.erase(0, input.find_first_not_of(" \t\r\n"));
    input.erase(input.find_last_not_of(" \t\r\n") + 1);

    if (input.empty()) return true;

    logger.info("Input: \"" + input + "\"");

    // 2. Resolve intent from full natural language input
    Intent intent = resolver.resolve(input);

    // 3. Exit check
    if (intent.type == IntentType::EXIT_APP) {
        logger.success("Goodbye! ZYREX shutting down.");
        std::cout << "\n  Goodbye!\n\n";
        return false;
    }

    // 4. Unknown intent
    if (intent.type == IntentType::UNKNOWN) {
        logger.warn("Unknown intent for: \"" + input + "\"");
        printUnknownHelp();
        return true;
    }

    // 5. Policy gate
    if (!policy.isAllowed(intent)) {
        logger.warn("Blocked by policy: " + input);
        return true;
    }

    // 6. SELECT_OPTION — handled directly (not via registry)
    if (intent.type == IntentType::SELECT_OPTION) {
        if (intent.args.empty()) {
            std::cout << "  Please say a number like 'open 1'.\n";
            return true;
        }

        int index = 0;
        try {
            index = std::stoi(intent.args[0]);
        } catch (...) {
            std::cout << "  Invalid selection number.\n";
            return true;
        }

        bool ok = fileSystem.openByIndex(index);
        if (ok)
            logger.success("Opened file at index " + intent.args[0]);
        else
            logger.error("Failed to open index " + intent.args[0]);

        return true;
    }

    // 7. RENAME FILE — handled directly (needs 2 args)
    if (intent.type == IntentType::RENAME_FILE) {
        if (intent.args.size() < 2) {
            std::cout << "  Usage: rename <oldname> <newname>\n";
            return true;
        }
        bool ok = fileSystem.renameFile(intent.args);
        ok ? logger.success("File renamed.")
           : logger.error("Rename failed.");
        return true;
    }

    // 8. All other intents → dispatch via registry
    const char* cmd = intentToCommand(intent.type);

    if (!cmd) {
        // Intent exists but no registry handler yet
        logger.warn("No handler registered for this intent.");
        std::cout << "  This feature is not yet implemented.\n";
        return true;
    }

    bool executed = registry.execute(cmd, intent.args);

    // 9. Result feedback
    if (executed) {
        logger.success("Done: " + std::string(cmd));
    } else {
        logger.error("Failed: " + std::string(cmd));
        std::cout << "  Something went wrong. Check the log for details.\n";
    }

    return true;
}
