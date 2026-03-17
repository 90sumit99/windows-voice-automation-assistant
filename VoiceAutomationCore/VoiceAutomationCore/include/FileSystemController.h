#pragma once
#include <vector>
#include <string>

class FileSystemController {
public:
    // Search for a file by name across common user folders
    bool find(const std::vector<std::string>& args);

    // Open a file (reuses lastResults if available)
    bool openFile(const std::vector<std::string>& args);

    // Open a file from last find() results by number (e.g. "open 2")
    bool openByIndex(int index);

    // Delete a file by name
    bool deleteFile(const std::vector<std::string>& args);

    // Rename a file (args: oldname newname)
    bool renameFile(const std::vector<std::string>& args);

    // Get last search results (used by InputRouter for SELECT_OPTION)
    const std::vector<std::string>& getLastResults() const { return lastResults; }

private:
    void searchRecursive(const std::string& directory,
                         const std::string& target,
                         std::vector<std::string>& results,
                         int depth,
                         int maxDepth);

    std::vector<std::string> getSearchRoots();
    bool openPath(const std::string& path);
    void printResults(const std::vector<std::string>& results);

    std::vector<std::string> lastResults;
};
