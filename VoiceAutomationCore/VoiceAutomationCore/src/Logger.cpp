#include "Logger.h"
#include <iostream>

void Logger::log(const std::string& msg) {
    std::cout << "[LOG] " << msg << std::endl;
}
