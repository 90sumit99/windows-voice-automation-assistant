#include <iostream>
#include <vector>

#include "CommandParser.h"
#include "CommandRegistry.h"

#include "ApplicationLauncher.h"
#include "AudioController.h"
#include "FileSystemController.h"
#include "SystemExecutor.h"

#include "PolicyEngine.h"
#include "Logger.h"
#include "InputRouter.h"

int main() {
    Logger logger;
    PolicyEngine policy;
    CommandRegistry registry;

    ApplicationLauncher appLauncher;
    AudioController audio;
    FileSystemController fs;
    SystemExecutor executor;

    registry.registerCommand("open",
        [&](const std::vector<std::string>& args) {
            return appLauncher.open(args);
        });

    registry.registerCommand("close",
        [&](const std::vector<std::string>& args) {
            return appLauncher.close(args);
        });

    registry.registerCommand("volume",
        [&](const std::vector<std::string>& args) {
            return audio.setVolume(args);
        });

    registry.registerCommand("mute",
        [&](const std::vector<std::string>&) {
            return audio.mute();
        });

    registry.registerCommand("unmute",
        [&](const std::vector<std::string>&) {
            return audio.unmute();
        });

    registry.registerCommand("find",
        [&](const std::vector<std::string>& args) {
            return fs.find(args);
        });

    registry.registerCommand("run",
        [&](const std::vector<std::string>& args) {
            return executor.run(args);
        });
    registry.registerCommand("find",
        [&](const std::vector<std::string>& args) {
            return fs.find(args);
        });

    registry.registerCommand("openfile",
        [&](const std::vector<std::string>& args) {
            return fs.openFile(args);
        });


    InputRouter router(registry, policy, logger);

    std::cout << "ZYREX Core started (type 'exit' to quit)\n";

    while (router.processOnce()) {}

    return 0;
}
