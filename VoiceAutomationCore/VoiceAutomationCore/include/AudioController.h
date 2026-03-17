#pragma once
#include <vector>
#include <string>

class AudioController {
public:
    // Volume set (0-100)
    bool setVolume(const std::vector<std::string>& args);

    // Mute / Unmute
    bool mute();
    bool unmute();

    // Step up/down by 10%
    bool volumeUp();
    bool volumeDown();

    // Get current volume (0-100), returns -1 on failure
    int  getCurrentVolume();

    // Media keys (uses SendInput)
    bool playPause();
    bool nextTrack();
    bool prevTrack();

private:
    bool getEndpointVolume(void** endpointVolume);

    // Step size for volumeUp / volumeDown
    static constexpr float VOLUME_STEP = 0.10f;  // 10%
};
