#pragma once
#include <string>
#include <vector>

enum class MusicTarget {
    SPOTIFY,
    YOUTUBE_CHROME,
    YOUTUBE_MUSIC_CHROME,
    SYSTEM_DEFAULT   // just use media keys
};

class MusicController {
public:
    // Play a song by name on a specific target
    // e.g. playSong("Blinding Lights", MusicTarget::SPOTIFY)
    bool playSong(const std::string& songName, MusicTarget target);

    // Parse target from user words e.g. "spotify", "youtube", "chrome"
    static MusicTarget parseTarget(const std::string& input);

    // Just send play/pause media key
    bool playPause();
    bool nextTrack();
    bool prevTrack();

private:
    bool openSpotifySearch(const std::string& songName);
    bool openYouTubeSearch(const std::string& songName, bool ytMusic);
    bool sendMediaKey(unsigned short vkCode);
};
