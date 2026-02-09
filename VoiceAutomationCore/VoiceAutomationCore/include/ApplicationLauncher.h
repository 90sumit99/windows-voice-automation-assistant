#pragma once
#include <vector>
#include <string>

class ApplicationLauncher {
public:
    bool open(const std::vector<std::string>& args);
    bool close(const std::vector<std::string>& args);
};
