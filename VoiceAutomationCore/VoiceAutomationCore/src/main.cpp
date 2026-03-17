#include <iostream>
#include <vector>

#include "CommandParser.h"
#include "CommandRegistry.h"
#include "ApplicationLauncher.h"
#include "AudioController.h"
#include "FileSystemController.h"
#include "SystemExecutor.h"
#include "ScreenshotController.h"
#include "MusicController.h"
#include "PolicyEngine.h"
#include "Logger.h"
#include "InputRouter.h"

int main() {
    // ── Core services ──────────────────────────────────────────
    Logger              logger;
    PolicyEngine        policy;
    CommandRegistry     registry;

    // ── Action modules ─────────────────────────────────────────
    ApplicationLauncher appLauncher;
    AudioController     audio;
    FileSystemController fileSystem;
    SystemExecutor      executor;
    ScreenshotController screenshot;
    MusicController     music;

    // ── App control ────────────────────────────────────────────
    registry.registerCommand("open",       [&](const std::vector<std::string>& args) { return appLauncher.open(args);       });
    registry.registerCommand("close",      [&](const std::vector<std::string>& args) { return appLauncher.close(args);      });

    // ── Volume ─────────────────────────────────────────────────
    registry.registerCommand("volume",     [&](const std::vector<std::string>& args) { return audio.setVolume(args);        });
    registry.registerCommand("mute",       [&](const std::vector<std::string>&)      { return audio.mute();                 });
    registry.registerCommand("unmute",     [&](const std::vector<std::string>&)      { return audio.unmute();               });
    registry.registerCommand("volumeup",   [&](const std::vector<std::string>&)      { return audio.volumeUp();             });
    registry.registerCommand("volumedown", [&](const std::vector<std::string>&)      { return audio.volumeDown();           });

    // ── Media keys ─────────────────────────────────────────────
    registry.registerCommand("mediaplay",  [&](const std::vector<std::string>&)      { return audio.playPause();            });
    registry.registerCommand("medianext",  [&](const std::vector<std::string>&)      { return audio.nextTrack();            });
    registry.registerCommand("mediaprev",  [&](const std::vector<std::string>&)      { return audio.prevTrack();            });

    // ── Music with target ───────────────────────────────────────
    // args[0] = target (spotify/youtube/ytmusic/default)
    // args[1] = song name
    registry.registerCommand("playmusic",  [&](const std::vector<std::string>& args) {
        std::string target   = args.size() > 0 ? args[0] : "default";
        std::string songName = args.size() > 1 ? args[1] : "";
        MusicTarget mt       = MusicController::parseTarget(target);
        return music.playSong(songName, mt);
    });

    // ── File system ────────────────────────────────────────────
    registry.registerCommand("find",       [&](const std::vector<std::string>& args) { return fileSystem.find(args);        });
    registry.registerCommand("openfile",   [&](const std::vector<std::string>& args) { return fileSystem.openFile(args);    });
    registry.registerCommand("deletefile", [&](const std::vector<std::string>& args) { return fileSystem.deleteFile(args);  });

    // ── System ─────────────────────────────────────────────────
    registry.registerCommand("run",        [&](const std::vector<std::string>& args) { return executor.run(args);           });
    registry.registerCommand("shutdown",   [&](const std::vector<std::string>&)      { return executor.shutdown();          });
    registry.registerCommand("restart",    [&](const std::vector<std::string>&)      { return executor.restart();           });
    registry.registerCommand("sleep",      [&](const std::vector<std::string>&)      { return executor.sleep();             });
    registry.registerCommand("lock",       [&](const std::vector<std::string>&)      { return executor.lock();              });

    // ── Screenshot ─────────────────────────────────────────────
    registry.registerCommand("screenshot", [&](const std::vector<std::string>&)      { return screenshot.take();            });

    // ── Start router ───────────────────────────────────────────
    InputRouter router(registry, policy, logger, fileSystem);

    logger.success("ZYREX Core started. Type a command or 'exit' to quit.");
    std::cout << "\n";

    while (router.processOnce()) {}

    logger.info("ZYREX shut down cleanly.");
    return 0;
}
