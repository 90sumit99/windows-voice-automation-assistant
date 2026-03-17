#pragma once
#include <string>

// Returned by every action module back to InputRouter
struct ExecutionResult {
    bool        success = false;
    std::string message;       // human-readable result
    std::string data;          // optional: JSON / file path / output string
};
