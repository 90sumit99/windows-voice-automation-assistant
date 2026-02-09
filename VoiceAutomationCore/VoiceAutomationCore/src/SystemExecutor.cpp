#include "SystemExecutor.h"
#include <cstdlib>

bool SystemExecutor::run(const std::vector<std::string>& args) {
    if (args.empty()) return false;
    system(args[0].c_str());
    return true;
}
