#pragma once
#include <vector>
#include <string>

class AudioController {
public:
    bool setVolume(const std::vector<std::string>& args);
    bool mute();
    bool unmute();

private:
    bool getEndpointVolume(void** endpointVolume);
};
