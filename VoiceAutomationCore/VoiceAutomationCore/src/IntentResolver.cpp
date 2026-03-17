#include "IntentResolver.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

// ─────────────────────────────────────────────────────────────
//  HELPERS
// ─────────────────────────────────────────────────────────────

std::string IntentResolver::normalize(const std::string& input) const {
    std::string out = input;

    // lowercase
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return std::tolower(c); });

    // remove punctuation except dots (for file extensions)
    std::string cleaned;
    for (char c : out) {
        if (!std::ispunct(static_cast<unsigned char>(c)) || c == '.') {
            cleaned += c;
        }
    }
    return cleaned;
}

std::vector<std::string> IntentResolver::splitWords(const std::string& text) const {
    std::stringstream ss(text);
    std::string word;
    std::vector<std::string> words;
    while (ss >> word) words.push_back(word);
    return words;
}

bool IntentResolver::contains(const std::string& text,
                               const std::string& word) const {
    return text.find(word) != std::string::npos;
}

bool IntentResolver::containsAny(const std::string& text,
                                  const std::vector<std::string>& words) const {
    for (const auto& w : words)
        if (contains(text, w)) return true;
    return false;
}

std::vector<std::string> IntentResolver::extractArgs(
    const std::vector<std::string>& words,
    const std::vector<std::string>& stopWords) const {

    std::vector<std::string> result;
    for (const auto& w : words) {
        bool isStop = false;
        for (const auto& s : stopWords)
            if (w == s) { isStop = true; break; }
        if (!isStop) result.push_back(w);
    }
    return result;
}

// Convert word numbers → digit string
std::string IntentResolver::wordToNumber(const std::string& word) const {
    static const std::unordered_map<std::string, std::string> map = {
        {"one","1"},   {"two","2"},   {"three","3"}, {"four","4"},
        {"five","5"},  {"six","6"},   {"seven","7"}, {"eight","8"},
        {"nine","9"},  {"ten","10"},  {"first","1"}, {"second","2"},
        {"third","3"}, {"fourth","4"},{"fifth","5"}
    };
    auto it = map.find(word);
    return (it != map.end()) ? it->second : "";
}

bool IntentResolver::isNumber(const std::string& word) const {
    if (word.empty()) return false;
    for (char c : word)
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    return true;
}

// ─────────────────────────────────────────────────────────────
//  MAIN RESOLVER
// ─────────────────────────────────────────────────────────────
Intent IntentResolver::resolve(const std::string& rawInput,
                                const std::vector<std::string>&) {
    Intent intent;
    intent.rawInput = rawInput;

    std::string input        = normalize(rawInput);
    std::vector<std::string> words = splitWords(input);

    // ── EXIT ─────────────────────────────────────────────────
    if (containsAny(input, {"exit", "quit", "bye", "goodbye", "shutdown zyrex"})) {
        intent.type = IntentType::EXIT_APP;
        return intent;
    }

    // ── SYSTEM POWER ─────────────────────────────────────────
    if (containsAny(input, {"shut down", "shutdown", "turn off", "power off"})) {
        intent.type = IntentType::SHUTDOWN;
        return intent;
    }
    if (containsAny(input, {"restart", "reboot", "reset pc"})) {
        intent.type = IntentType::RESTART;
        return intent;
    }
    if (containsAny(input, {"sleep", "hibernate", "suspend"})) {
        intent.type = IntentType::SLEEP;
        return intent;
    }
    if (containsAny(input, {"lock", "lock screen", "lock pc", "lock computer"})) {
        intent.type = IntentType::LOCK;
        return intent;
    }

    // ── PLAY MUSIC ON TARGET ─────────────────────────────────
    // "play Blinding Lights on spotify"
    // "play Shape of You on youtube"
    // "play music on youtube music"
    if (containsAny(input, {"play", "listen to", "put on", "search for"})) {
        bool hasTarget = containsAny(input, {"spotify", "youtube", "chrome",
                                             "browser", "yt music", "youtube music"});
        bool hasOnKeyword = containsAny(input, {"on", "in", "via", "through"});

        if (hasTarget || hasOnKeyword) {
            intent.type = IntentType::PLAY_MUSIC;

            // Extract target platform
            if (contains(input, "youtube music") || contains(input, "yt music"))
                intent.args.push_back("ytmusic");
            else if (contains(input, "youtube"))
                intent.args.push_back("youtube");
            else if (contains(input, "spotify"))
                intent.args.push_back("spotify");
            else if (contains(input, "chrome") || contains(input, "browser"))
                intent.args.push_back("youtube");
            else
                intent.args.push_back("default");

            // Extract song name — everything except trigger words and platform
            std::vector<std::string> stopWords = {
                "play", "listen", "to", "put", "on", "in", "via", "through",
                "search", "for", "spotify", "youtube", "chrome", "browser",
                "music", "yt", "song", "track", "the"
            };
            std::string songName;
            for (const auto& w : words) {
                bool isStop = false;
                for (const auto& s : stopWords)
                    if (w == s) { isStop = true; break; }
                if (!isStop) {
                    if (!songName.empty()) songName += " ";
                    songName += w;
                }
            }
            intent.args.push_back(songName); // args[0]=target, args[1]=song
            return intent;
        }
    }

    // ── MEDIA KEYS ───────────────────────────────────────────
    if (containsAny(input, {"play", "pause", "play pause", "resume music",
                             "stop music", "toggle music"})) {
        // Make sure it's not "play <app>" (handled below as OPEN_APP)
        if (!containsAny(input, {"spotify", "vlc", "youtube", "music app"})) {
            intent.type = IntentType::MEDIA_PLAY_PAUSE;
            return intent;
        }
    }
    if (containsAny(input, {"next song", "next track", "skip song",
                             "skip track", "next music"})) {
        intent.type = IntentType::MEDIA_NEXT;
        return intent;
    }
    if (containsAny(input, {"previous song", "previous track", "prev song",
                             "prev track", "go back song", "last song"})) {
        intent.type = IntentType::MEDIA_PREV;
        return intent;
    }

    // ── VOLUME ───────────────────────────────────────────────
    if (containsAny(input, {"unmute", "un mute", "turn sound on"})) {
        intent.type = IntentType::UNMUTE_VOLUME;
        return intent;
    }
    if (containsAny(input, {"mute", "silence", "no sound", "quiet"})) {
        intent.type = IntentType::MUTE_VOLUME;
        return intent;
    }
    if (containsAny(input, {"volume up", "turn up", "louder",
                             "increase volume", "raise volume"})) {
        intent.type = IntentType::VOLUME_UP;
        return intent;
    }
    if (containsAny(input, {"volume down", "turn down", "quieter",
                             "decrease volume", "lower volume", "reduce volume"})) {
        intent.type = IntentType::VOLUME_DOWN;
        return intent;
    }
    if (contains(input, "volume") || containsAny(input, {"set volume", "volume to"})) {
        intent.type = IntentType::SET_VOLUME;
        // Extract the number
        for (const auto& w : words) {
            if (isNumber(w)) {
                intent.args.push_back(w);
                return intent;
            }
            std::string n = wordToNumber(w);
            if (!n.empty()) {
                intent.args.push_back(n);
                return intent;
            }
        }
        intent.args.push_back("50"); // default
        return intent;
    }

    // ── SCREENSHOT ───────────────────────────────────────────
    if (containsAny(input, {"screenshot", "screen capture", "capture screen",
                             "take screenshot", "snap screen"})) {
        intent.type = IntentType::TAKE_SCREENSHOT;
        return intent;
    }

    // ── SELECT OPTION (open 1 / open two / open second) ──────
    // Must check BEFORE open-file and open-app
    if (contains(input, "open")) {
        for (const auto& w : words) {
            if (w == "open") continue;

            // digit directly: "open 2"
            if (isNumber(w)) {
                intent.type = IntentType::SELECT_OPTION;
                intent.args.push_back(w);
                return intent;
            }
            // word number: "open two", "open second"
            std::string n = wordToNumber(w);
            if (!n.empty()) {
                intent.type = IntentType::SELECT_OPTION;
                intent.args.push_back(n);
                return intent;
            }
        }
    }

    // ── DELETE FILE ──────────────────────────────────────────
    if (containsAny(input, {"delete", "remove file", "erase file", "trash"})) {
        intent.type = IntentType::DELETE_FILE;
        intent.args = extractArgs(words, {"delete","remove","erase",
                                          "trash","file","the","my"});
        return intent;
    }

    // ── RENAME FILE ──────────────────────────────────────────
    if (containsAny(input, {"rename", "rename file"})) {
        intent.type = IntentType::RENAME_FILE;
        intent.args = extractArgs(words, {"rename","file","to","the","my"});
        return intent;
    }

    // ── OPEN FILE ────────────────────────────────────────────
    if (contains(input, "open") &&
        containsAny(input, {"file", "document", "pdf", "doc", "txt",
                             "xlsx", "pptx", "csv", "image", "photo",
                             "video", "mp4", "mp3", "resume", "report"})) {
        intent.type = IntentType::OPEN_FILE;
        intent.args = extractArgs(words, {"open","my","the","file",
                                          "document","please"});
        return intent;
    }

    // ── FIND FILE ────────────────────────────────────────────
    if (containsAny(input, {"find", "search", "look for",
                             "where is", "locate", "search for"})) {
        intent.type = IntentType::FIND_FILE;
        intent.args = extractArgs(words, {"find","search","look","for",
                                          "where","is","locate","my","the"});
        return intent;
    }

    // ── OPEN APP ─────────────────────────────────────────────
    if (containsAny(input, {"open", "launch", "start", "run", "load"})) {
        intent.type = IntentType::OPEN_APP;
        intent.args = extractArgs(words, {"open","launch","start","run",
                                          "load","app","application","the","please"});
        return intent;
    }

    // ── CLOSE APP ────────────────────────────────────────────
    if (containsAny(input, {"close", "kill", "stop", "terminate", "end"})) {
        intent.type = IntentType::CLOSE_APP;
        intent.args = extractArgs(words, {"close","kill","stop","terminate",
                                          "end","app","application","the"});
        return intent;
    }

    // ── SYSTEM INFO ──────────────────────────────────────────
    if (containsAny(input, {"wifi", "wi-fi", "wireless", "network status"})) {
        intent.type = IntentType::RUN_COMMAND;
        intent.args = {"netsh", "wlan", "show", "interfaces"};
        return intent;
    }
    if (containsAny(input, {"ip address", "my ip", "ipconfig", "ip config"})) {
        intent.type = IntentType::RUN_COMMAND;
        intent.args = {"ipconfig"};
        return intent;
    }
    if (containsAny(input, {"battery", "battery status", "battery level"})) {
        intent.type = IntentType::RUN_COMMAND;
        intent.args = {"powercfg", "/batteryreport"};
        return intent;
    }
    if (containsAny(input, {"disk space", "storage", "disk usage"})) {
        intent.type = IntentType::RUN_COMMAND;
        intent.args = {"wmic", "logicaldisk", "get", "size,freespace,caption"};
        return intent;
    }

    // ── UNKNOWN ──────────────────────────────────────────────
    intent.type = IntentType::UNKNOWN;
    return intent;
}
