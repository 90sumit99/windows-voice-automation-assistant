#include "FileSystemController.h"
#include <filesystem>
#include <iostream>
#include <windows.h>
#include <cstdlib>

namespace fs = std::filesystem;

// ---------------- SEARCH ROOTS ----------------
std::vector<std::string> FileSystemController::getUserSearchRoots() {
    char* userProfile = nullptr;
    size_t len = 0;

    if (_dupenv_s(&userProfile, &len, "USERPROFILE") != 0 || !userProfile) {
        return {};
    }

    std::string base(userProfile);
    free(userProfile);

    return {
        base + "\\Downloads",
        base + "\\Documents",
        base + "\\Desktop"
    };
}


// ---------------- RECURSIVE SEARCH ----------------
void FileSystemController::searchRecursive(const std::string& directory,
    const std::string& target,
    std::vector<std::string>& results,
    int depth,
    int maxDepth) {
    if (depth > maxDepth) return;

    try {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_directory()) {
                searchRecursive(entry.path().string(), target,
                    results, depth + 1, maxDepth);
            }
            else {
                if (entry.path().filename().string().find(target) != std::string::npos) {
                    results.push_back(entry.path().string());
                }
            }
        }
    }
    catch (...) {
        // ignore protected folders
    }
}

// ---------------- FIND ----------------
bool FileSystemController::find(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Please specify a file name.\n";
        return false;
    }

    std::string target = args[0];
    std::vector<std::string> results;

    for (const auto& root : getUserSearchRoots()) {
        searchRecursive(root, target, results, 0, 5);
    }

    if (results.empty()) {
        std::cout << "No files found.\n";
        return false;
    }

    std::cout << "Found files:\n";
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << i + 1 << ": " << results[i] << "\n";
    }

    return true;
}

// ---------------- OPENFILE (SMART) ----------------
bool FileSystemController::openFile(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Please specify a file name.\n";
        return false;
    }

    std::string target = args[0];
    std::vector<std::string> results;

    for (const auto& root : getUserSearchRoots()) {
        searchRecursive(root, target, results, 0, 5);
    }

    if (results.empty()) {
        std::cout << "File not found.\n";
        return false;
    }

    if (results.size() > 1) {
        std::cout << "Multiple files found. Please refine your request:\n";
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << i + 1 << ": " << results[i] << "\n";
        }
        return false;
    }

    // exactly one file → open it
    HINSTANCE result = ShellExecuteA(
        nullptr,
        "open",
        results[0].c_str(),
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );

    if (reinterpret_cast<intptr_t>(result) <= 32) {
        std::cout << "Failed to open file.\n";
        return false;
    }

    return true;
}
