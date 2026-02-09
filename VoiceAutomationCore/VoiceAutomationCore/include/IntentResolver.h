#pragma once
#pragma once
#include <string>
#include <vector>
#include "Intent.h"

class IntentResolver {
public:
    Intent resolve(const std::string& command,
        const std::vector<std::string>& args);
};