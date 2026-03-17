#pragma once
#include <string>
#include <fstream>

enum class LogLevel {
    LOG_INFO,
    LOG_SUCCESS,
    LOG_WARNING,
    LOG_ERROR
};

class Logger {
public:
    Logger();
    ~Logger();

    void log(const std::string& msg, LogLevel level = LogLevel::LOG_INFO);
    void info   (const std::string& msg) { log(msg, LogLevel::LOG_INFO);    }
    void success(const std::string& msg) { log(msg, LogLevel::LOG_SUCCESS); }
    void warn   (const std::string& msg) { log(msg, LogLevel::LOG_WARNING); }
    void error  (const std::string& msg) { log(msg, LogLevel::LOG_ERROR);   }

private:
    std::ofstream logFile;

    std::string timestamp() const;
    std::string levelTag(LogLevel level) const;
};
