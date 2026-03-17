#include "Logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <windows.h>

// -------- Constructor: open log file --------
Logger::Logger() {
    logFile.open("zyrex_log.txt", std::ios::app);
}

Logger::~Logger() {
    if (logFile.is_open())
        logFile.close();
}

// -------- Timestamp: [HH:MM:SS] --------
std::string Logger::timestamp() const {
    auto now   = std::chrono::system_clock::now();
    auto timer = std::chrono::system_clock::to_time_t(now);

    std::tm bt{};
    localtime_s(&bt, &timer);

    std::ostringstream oss;
    oss << std::put_time(&bt, "%H:%M:%S");
    return oss.str();
}

// -------- Level tag --------
std::string Logger::levelTag(LogLevel level) const {
    switch (level) {
    case LogLevel::LOG_INFO:    return "INFO   ";
    case LogLevel::LOG_SUCCESS: return "SUCCESS";
    case LogLevel::LOG_WARNING: return "WARNING";
    case LogLevel::LOG_ERROR:   return "ERROR  ";
    default:                    return "INFO   ";
    }
}

// -------- Console color via Windows API --------
static void setConsoleColor(LogLevel level) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    switch (level) {
    case LogLevel::LOG_INFO:    SetConsoleTextAttribute(hConsole, 7);  break;
    case LogLevel::LOG_SUCCESS: SetConsoleTextAttribute(hConsole, 10); break;
    case LogLevel::LOG_WARNING: SetConsoleTextAttribute(hConsole, 14); break;
    case LogLevel::LOG_ERROR:   SetConsoleTextAttribute(hConsole, 12); break;
    }
}

static void resetConsoleColor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 7); // back to white
}

// -------- Main log function --------
void Logger::log(const std::string& msg, LogLevel level) {
    std::string ts  = timestamp();
    std::string tag = levelTag(level);
    std::string line = "[" + ts + "] [" + tag + "] " + msg;

    // Colored console output
    setConsoleColor(level);
    std::cout << line << "\n";
    resetConsoleColor();

    // File output (no color codes)
    if (logFile.is_open()) {
        logFile << line << "\n";
        logFile.flush();
    }
}
