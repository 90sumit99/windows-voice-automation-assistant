#pragma once
#include <string>
#include <vector>
#include "Intent.h"

class IntentResolver {
public:
    Intent resolve(const std::string& rawInput,
                   const std::vector<std::string>& args = {});

private:
    // Keyword matchers
    bool contains(const std::string& text, const std::string& word) const;
    bool containsAny(const std::string& text,
                     const std::vector<std::string>& words) const;

    // Extraction helpers
    std::string normalize(const std::string& input) const;
    std::vector<std::string> splitWords(const std::string& text) const;

    // Strip filler words, return meaningful args
    std::vector<std::string> extractArgs(
        const std::vector<std::string>& words,
        const std::vector<std::string>& stopWords) const;

    // Convert "one"/"two"/... or "1"/"2"/... to int string
    std::string wordToNumber(const std::string& word) const;
    bool isNumber(const std::string& word) const;
};
