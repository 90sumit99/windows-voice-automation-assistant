#include "IntentResolver.h"
#include <algorithm>
#include <cctype>
#include <sstream>

// ---------- helper: normalize input ----------
static std::string normalize(const std::string& input) {
    std::string out = input;
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return std::tolower(c); });

    // remove punctuation
    out.erase(std::remove_if(out.begin(), out.end(),
        [](char c) { return std::ispunct(static_cast<unsigned char>(c)); }),
        out.end());

    return out;
}

// ---------- helper: check word existence ----------
static bool contains(const std::string& text, const std::string& word) {
    return text.find(word) != std::string::npos;
}

// ---------- helper: split into words ----------
static std::vector<std::string> splitWords(const std::string& text) {
    std::stringstream ss(text);
    std::string word;
    std::vector<std::string> words;

    while (ss >> word) {
        words.push_back(word);
    }
    return words;
}



// ---------- MAIN RESOLVER ----------
Intent IntentResolver::resolve(const std::string& rawInput,
    const std::vector<std::string>& /*unused*/) {
    Intent intent;

    std::string input = normalize(rawInput);
    std::vector<std::string> words = splitWords(input);

    // ---------- EXIT ----------
    if (contains(input, "exit") || contains(input, "quit")) {
        intent.type = IntentType::EXIT_APP;
        return intent;
    }

    // ---------- MUTE / UNMUTE ----------
    if (contains(input, "mute") && !contains(input, "unmute")) {
        intent.type = IntentType::MUTE_VOLUME;
        return intent;
    }

    if (contains(input, "unmute")) {
        intent.type = IntentType::UNMUTE_VOLUME;
        return intent;
    }

    // ---------- SET VOLUME ----------
    if (contains(input, "volume")) {
        intent.type = IntentType::SET_VOLUME;

        for (const auto& w : words) {
            if (std::isdigit(w[0])) {
                intent.args.push_back(w);
                return intent;
            }
        }

        // default volume
        intent.args.push_back("50");
        return intent;
    }

    // ---------- OPEN FILE ----------
    if (contains(input, "open") &&
        (contains(input, "file") || contains(input, "resume") ||
            contains(input, "document") || contains(input, "pdf"))) {

        intent.type = IntentType::OPEN_FILE;

        for (const auto& w : words) {
            if (w != "open" && w != "my" && w != "the" &&
                w != "file" && w != "document") {
                intent.args.push_back(w);
            }
        }
        return intent;
    }

    // ---------- FIND FILE ----------
    if (contains(input, "find") || contains(input, "search")) {
        intent.type = IntentType::FIND_FILE;

        for (const auto& w : words) {
            if (w != "find" && w != "search" && w != "for") {
                intent.args.push_back(w);
            }
        }
        return intent;
    }

    // ---------- SELECT FILE BY NUMBER ----------
    if (contains(input, "open") &&
        (contains(input, "one") || contains(input, "two") ||
            contains(input, "three") || contains(input, "four") ||
            contains(input, "five") || contains(input, "six") ||
            contains(input, "seven") || contains(input, "eight") ||
            contains(input, "nine"))) {

        intent.type = IntentType::SELECT_OPTION;

        if (contains(input, "one")) intent.args.push_back("1");
        else if (contains(input, "two")) intent.args.push_back("2");
        else if (contains(input, "three")) intent.args.push_back("3");
        else if (contains(input, "four")) intent.args.push_back("4");
        else if (contains(input, "five")) intent.args.push_back("5");
        else if (contains(input, "six")) intent.args.push_back("6");
        else if (contains(input, "seven")) intent.args.push_back("7");
        else if (contains(input, "eight")) intent.args.push_back("8");
        else if (contains(input, "nine")) intent.args.push_back("9");

        return intent;
    }


    // ---------- OPEN APPLICATION ----------
    if (contains(input, "open")) {
        intent.type = IntentType::OPEN_APP;

        for (const auto& w : words) {
            if (w != "open" && w != "app" && w != "application") {
                intent.args.push_back(w);
            }
        }
        return intent;
    }

    // ---------- CLOSE APPLICATION ----------
    if (contains(input, "close")) {
        intent.type = IntentType::CLOSE_APP;

        for (const auto& w : words) {
            if (w != "close" && w != "app" && w != "application") {
                intent.args.push_back(w);
            }
        }
        return intent;
    }

    // ---------- SYSTEM CHECKS ----------
    if (contains(input, "wifi")) {
        intent.type = IntentType::RUN_COMMAND;
        intent.args = { "netsh", "wlan", "show", "interfaces" };
        return intent;
    }

    if (contains(input, "ip")) {
        intent.type = IntentType::RUN_COMMAND;
        intent.args = { "ipconfig" };
        return intent;
    }

    // ---------- FALLBACK ----------
    intent.type = IntentType::UNKNOWN;
    return intent;
}
