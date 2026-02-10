#pragma once
#include <vector>
#include <string>

class FileSystemController {
public:
    bool find(const std::vector<std::string>& args);
    bool openFile(const std::vector<std::string>& args);
    bool openByIndex(int index);   

private:
    void searchRecursive(const std::string& directory,
        const std::string& target,
        std::vector<std::string>& results,
        int depth,
        int maxDepth);

    std::vector<std::string> getUserSearchRoots();

private:
    // NEW: store last results
    std::vector<std::string> lastResults;
};
