#include "MusicController.h"
#include <windows.h>
#include <shellapi.h>
#include <iostream>
#include <algorithm>
#include <sstream>

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

// ─────────────────────────────────────────────────────────────
//  URL ENCODE  (replace spaces with +, basic encoding)
// ─────────────────────────────────────────────────────────────
static std::string urlEncode(const std::string& s) {
    std::string result;
    for (char c : s) {
        if (c == ' ')       result += '+';
        else if (std::isalnum((unsigned char)c) ||
                 c == '-' || c == '_' || c == '.' || c == '~')
            result += c;
        else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
            result += buf;
        }
    }
    return result;
}

// ─────────────────────────────────────────────────────────────
//  PARSE TARGET from user input
// ─────────────────────────────────────────────────────────────
MusicTarget MusicController::parseTarget(const std::string& input) {
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.find("spotify")       != std::string::npos) return MusicTarget::SPOTIFY;
    if (lower.find("youtube music") != std::string::npos) return MusicTarget::YOUTUBE_MUSIC_CHROME;
    if (lower.find("yt music")      != std::string::npos) return MusicTarget::YOUTUBE_MUSIC_CHROME;
    if (lower.find("youtube")       != std::string::npos) return MusicTarget::YOUTUBE_CHROME;
    if (lower.find("chrome")        != std::string::npos) return MusicTarget::YOUTUBE_CHROME;
    if (lower.find("browser")       != std::string::npos) return MusicTarget::YOUTUBE_CHROME;

    return MusicTarget::SYSTEM_DEFAULT;
}

// ─────────────────────────────────────────────────────────────
//  OPEN SPOTIFY SEARCH
//  Uses Spotify URI scheme — works if Spotify is installed
// ─────────────────────────────────────────────────────────────
bool MusicController::openSpotifySearch(const std::string& songName) {
    // Spotify URI: spotify:search:song+name
    std::string encoded = urlEncode(songName);
    std::string uri     = "spotify:search:" + encoded;
    std::wstring wUri   = toWide(uri);

    std::cout << "[Music] Opening Spotify search: " << songName << "\n";

    HINSTANCE res = ShellExecuteW(nullptr, L"open", wUri.c_str(),
                                  nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<intptr_t>(res) > 32) {
        std::cout << "[Music] Spotify search opened. Press play on the result.\n";
        return true;
    }

    std::cout << "[Music] Spotify not found. Is it installed?\n";
    return false;
}

// ─────────────────────────────────────────────────────────────
//  OPEN YOUTUBE / YOUTUBE MUSIC SEARCH IN CHROME
// ─────────────────────────────────────────────────────────────
bool MusicController::openYouTubeSearch(const std::string& songName, bool ytMusic) {
    std::string encoded = urlEncode(songName);
    std::string url;

    if (ytMusic) {
        url = "https://music.youtube.com/search?q=" + encoded;
        std::cout << "[Music] Opening YouTube Music: " << songName << "\n";
    } else {
        url = "https://www.youtube.com/results?search_query=" + encoded;
        std::cout << "[Music] Opening YouTube: " << songName << "\n";
    }

    std::wstring wUrl = toWide(url);
    HINSTANCE res = ShellExecuteW(nullptr, L"open", wUrl.c_str(),
                                  nullptr, nullptr, SW_SHOWNORMAL);

    if (reinterpret_cast<intptr_t>(res) > 32) {
        std::cout << "[Music] Opened in browser. Click the first result to play.\n";
        return true;
    }

    std::cout << "[Music] Failed to open browser.\n";
    return false;
}

// ─────────────────────────────────────────────────────────────
//  PLAY SONG — main entry point
// ─────────────────────────────────────────────────────────────
bool MusicController::playSong(const std::string& songName, MusicTarget target) {
    if (songName.empty()) {
        // No song name — just send play/pause
        return playPause();
    }

    switch (target) {
    case MusicTarget::SPOTIFY:
        return openSpotifySearch(songName);

    case MusicTarget::YOUTUBE_MUSIC_CHROME:
        return openYouTubeSearch(songName, true);

    case MusicTarget::YOUTUBE_CHROME:
        return openYouTubeSearch(songName, false);

    case MusicTarget::SYSTEM_DEFAULT:
    default:
        // No target specified — try Spotify first, fallback to YouTube
        std::cout << "[Music] No target specified, trying Spotify...\n";
        if (!openSpotifySearch(songName)) {
            std::cout << "[Music] Falling back to YouTube...\n";
            return openYouTubeSearch(songName, false);
        }
        return true;
    }
}

// ─────────────────────────────────────────────────────────────
//  MEDIA KEYS
// ─────────────────────────────────────────────────────────────
bool MusicController::sendMediaKey(unsigned short vkCode) {
    INPUT inputs[2] = {};
    inputs[0].type     = INPUT_KEYBOARD;
    inputs[0].ki.wVk   = vkCode;
    inputs[1].type     = INPUT_KEYBOARD;
    inputs[1].ki.wVk   = vkCode;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(2, inputs, sizeof(INPUT)) == 2;
}

bool MusicController::playPause() {
    bool ok = sendMediaKey(VK_MEDIA_PLAY_PAUSE);
    if (ok) std::cout << "[Music] Play/Pause toggled.\n";
    return ok;
}

bool MusicController::nextTrack() {
    bool ok = sendMediaKey(VK_MEDIA_NEXT_TRACK);
    if (ok) std::cout << "[Music] Next track.\n";
    return ok;
}

bool MusicController::prevTrack() {
    bool ok = sendMediaKey(VK_MEDIA_PREV_TRACK);
    if (ok) std::cout << "[Music] Previous track.\n";
    return ok;
}
