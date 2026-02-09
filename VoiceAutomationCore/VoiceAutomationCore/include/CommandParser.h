#pragma once
#include <string>
#include <vector>

struct ParsedCommand {
    std::string command;
    std::vector<std::string> args;
};

class CommandParser {
public:
    ParsedCommand parse(const std::string& input);
};
