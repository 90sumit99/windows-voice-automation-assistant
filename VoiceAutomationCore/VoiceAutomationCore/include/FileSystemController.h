#pragma once
#include <vector>
#include <string>

class FileSystemController {
public:
    bool find(const std::vector<std::string>& args);
    bool openFile(const std::vector<std::string>& args);

private:
    void searchRecursive(const std::string& directory,
        const std::string& target,
        std::vector<std::string>& results,
        int depth,
        int maxDepth);

    std::vector<std::string> getUserSearchRoots();
};
