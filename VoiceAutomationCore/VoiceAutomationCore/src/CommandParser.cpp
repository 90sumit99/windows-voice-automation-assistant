#include "CommandParser.h"
#include <sstream>

ParsedCommand CommandParser::parse(const std::string& input) {
    std::istringstream iss(input);
    ParsedCommand result;
    iss >> result.command;

    std::string arg;
    while (iss >> arg) {
        result.args.push_back(arg);
    }
    return result;
}
