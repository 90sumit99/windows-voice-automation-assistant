#include "FileSystemController.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <windows.h>

// ─────────────────────────────────────────────────────────────
//  STRING HELPER
// ─────────────────────────────────────────────────────────────
static std::wstring toWide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], len);
    return w;
}

namespace fs = std::filesystem;// ─────────────────────────────────────────────────────────────
//  SEARCH ROOTS
//  Covers all common user folders + drives
// ─────────────────────────────────────────────────────────────
std::vector<std::string> FileSystemController::getSearchRoots() {
    std::vector<std::string> roots;

    // Get USERPROFILE (e.g. C:\Users\YourName)
    char* userProfile = nullptr;
    size_t len = 0;
    if (_dupenv_s(&userProfile, &len, "USERPROFILE") == 0 && userProfile) {
        std::string base(userProfile);
        free(userProfile);

        roots.push_back(base + "\\Desktop");
        roots.push_back(base + "\\Documents");
        roots.push_back(base + "\\Downloads");
        roots.push_back(base + "\\Pictures");
        roots.push_back(base + "\\Videos");
        roots.push_back(base + "\\Music");
        roots.push_back(base + "\\OneDrive");
    }

    // Also search Program Files for app configs etc.
    char* programFiles = nullptr;
    if (_dupenv_s(&programFiles, &len, "ProgramFiles") == 0 && programFiles) {
        roots.push_back(std::string(programFiles));
        free(programFiles);
    }

    return roots;
}

// ─────────────────────────────────────────────────────────────
//  RECURSIVE SEARCH
// ─────────────────────────────────────────────────────────────
void FileSystemController::searchRecursive(const std::string& directory,
                                           const std::string& target,
                                           std::vector<std::string>& results,
                                           int depth, int maxDepth) {
    if (depth > maxDepth) return;

    try {
        for (const auto& entry : fs::directory_iterator(
                directory,
                fs::directory_options::skip_permission_denied)) {

            if (entry.is_directory()) {
                searchRecursive(entry.path().string(), target,
                                results, depth + 1, maxDepth);
            } else {
                std::string filename = entry.path().filename().string();
                std::string lowerFile = filename;
                std::string lowerTarget = target;

                std::transform(lowerFile.begin(),   lowerFile.end(),
                               lowerFile.begin(),   ::tolower);
                std::transform(lowerTarget.begin(), lowerTarget.end(),
                               lowerTarget.begin(), ::tolower);

                if (lowerFile.find(lowerTarget) != std::string::npos) {
                    results.push_back(entry.path().string());
                }
            }
        }
    } catch (...) {
        // skip protected/inaccessible folders silently
    }
}

// ─────────────────────────────────────────────────────────────
//  PRINT RESULTS  (shared by find + openFile)
// ─────────────────────────────────────────────────────────────
void FileSystemController::printResults(const std::vector<std::string>& results) {
    std::cout << "\n[FileSystem] Found " << results.size() << " file(s):\n";
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << "  " << (i + 1) << ": " << results[i] << "\n";
    }
    std::cout << "\n";
}

// ─────────────────────────────────────────────────────────────
//  OPEN PATH  (ShellExecute wrapper)
// ─────────────────────────────────────────────────────────────
bool FileSystemController::openPath(const std::string& path) {
    std::wstring wPath = toWide(path);
    HINSTANCE res = ShellExecuteW(nullptr, L"open", wPath.c_str(),
                                  nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<intptr_t>(res) <= 32) {
        std::cout << "[FileSystem] Failed to open: " << path << "\n";
        return false;
    }
    std::cout << "[FileSystem] Opened: " << path << "\n";
    return true;
}

// ─────────────────────────────────────────────────────────────
//  FIND
// ─────────────────────────────────────────────────────────────
bool FileSystemController::find(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "[FileSystem] Please specify a file name.\n";
        return false;
    }

    std::string target = args[0];
    std::vector<std::string> results;

    std::cout << "[FileSystem] Searching for \"" << target << "\"...\n";

    for (const auto& root : getSearchRoots()) {
        searchRecursive(root, target, results, 0, 5);
    }

    if (results.empty()) {
        std::cout << "[FileSystem] No files found matching \"" << target << "\".\n";
        lastResults.clear();
        return false;
    }

    printResults(results);
    lastResults = results;  // save for openByIndex

    if (results.size() == 1) {
        std::cout << "[FileSystem] Say 'open 1' to open it.\n";
    } else {
        std::cout << "[FileSystem] Say 'open 1', 'open 2', etc. to open a file.\n";
    }

    return true;
}

// ─────────────────────────────────────────────────────────────
//  OPEN FILE  (reuses lastResults if target matches)
// ─────────────────────────────────────────────────────────────
bool FileSystemController::openFile(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "[FileSystem] Please specify a file name.\n";
        return false;
    }

    std::string target = args[0];

    // Check if lastResults already has a match — avoid re-searching
    if (!lastResults.empty()) {
        std::string lowerTarget = target;
        std::transform(lowerTarget.begin(), lowerTarget.end(),
                       lowerTarget.begin(), ::tolower);

        std::vector<std::string> matches;
        for (const auto& path : lastResults) {
            std::string lowerPath = path;
            std::transform(lowerPath.begin(), lowerPath.end(),
                           lowerPath.begin(), ::tolower);
            if (lowerPath.find(lowerTarget) != std::string::npos)
                matches.push_back(path);
        }

        if (!matches.empty()) {
            if (matches.size() == 1)
                return openPath(matches[0]);

            printResults(matches);
            std::cout << "[FileSystem] Multiple matches. Say 'open 1', 'open 2', etc.\n";
            lastResults = matches;
            return false;
        }
    }

    // Fresh search
    std::vector<std::string> results;
    std::cout << "[FileSystem] Searching for \"" << target << "\"...\n";

    for (const auto& root : getSearchRoots()) {
        searchRecursive(root, target, results, 0, 5);
    }

    if (results.empty()) {
        std::cout << "[FileSystem] File not found: \"" << target << "\".\n";
        return false;
    }

    lastResults = results;

    if (results.size() == 1)
        return openPath(results[0]);

    printResults(results);
    std::cout << "[FileSystem] Say 'open 1', 'open 2', etc. to choose.\n";
    return false;
}

// ─────────────────────────────────────────────────────────────
//  OPEN BY INDEX  (after find shows a numbered list)
// ─────────────────────────────────────────────────────────────
bool FileSystemController::openByIndex(int index) {
    if (lastResults.empty()) {
        std::cout << "[FileSystem] No previous search results. Use 'find' first.\n";
        return false;
    }

    if (index < 1 || index > static_cast<int>(lastResults.size())) {
        std::cout << "[FileSystem] Invalid selection. Choose 1 to "
                  << lastResults.size() << ".\n";
        return false;
    }

    return openPath(lastResults[index - 1]);
}

// ─────────────────────────────────────────────────────────────
//  DELETE FILE
// ─────────────────────────────────────────────────────────────
bool FileSystemController::deleteFile(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "[FileSystem] Please specify a file name to delete.\n";
        return false;
    }

    std::string target = args[0];
    std::vector<std::string> results;

    for (const auto& root : getSearchRoots()) {
        searchRecursive(root, target, results, 0, 5);
    }

    if (results.empty()) {
        std::cout << "[FileSystem] File not found: \"" << target << "\".\n";
        return false;
    }

    if (results.size() > 1) {
        std::cout << "[FileSystem] Multiple files found. Please be more specific:\n";
        printResults(results);
        lastResults = results;
        return false;
    }

    // Confirm before deleting
    std::cout << "[FileSystem] Delete: " << results[0] << " ? (yes/no): ";
    std::string confirm;
    std::getline(std::cin, confirm);

    if (confirm != "yes" && confirm != "y") {
        std::cout << "[FileSystem] Delete cancelled.\n";
        return false;
    }

    std::error_code ec;
    fs::remove(results[0], ec);

    if (ec) {
        std::cout << "[FileSystem] Failed to delete: " << ec.message() << "\n";
        return false;
    }

    std::cout << "[FileSystem] Deleted: " << results[0] << "\n";
    lastResults.clear();
    return true;
}

// ─────────────────────────────────────────────────────────────
//  RENAME FILE
// ─────────────────────────────────────────────────────────────
bool FileSystemController::renameFile(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "[FileSystem] Usage: rename <oldname> <newname>\n";
        return false;
    }

    std::string oldName = args[0];
    std::string newName = args[1];
    std::vector<std::string> results;

    for (const auto& root : getSearchRoots()) {
        searchRecursive(root, oldName, results, 0, 5);
    }

    if (results.empty()) {
        std::cout << "[FileSystem] File not found: \"" << oldName << "\".\n";
        return false;
    }

    if (results.size() > 1) {
        std::cout << "[FileSystem] Multiple files found. Be more specific:\n";
        printResults(results);
        return false;
    }

    fs::path oldPath = results[0];
    fs::path newPath = oldPath.parent_path() / newName;

    std::error_code ec;
    fs::rename(oldPath, newPath, ec);

    if (ec) {
        std::cout << "[FileSystem] Rename failed: " << ec.message() << "\n";
        return false;
    }

    std::cout << "[FileSystem] Renamed to: " << newPath.string() << "\n";
    return true;
}
