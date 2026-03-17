#include "AudioController.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <iostream>

// ─────────────────────────────────────────────────────────────
//  INTERNAL HELPER
//  CoInitializeEx is called ONCE per function scope here.
//  Each public method initializes and uninitializes COM itself
//  so they are safe to call independently or together.
// ─────────────────────────────────────────────────────────────

bool AudioController::getEndpointVolume(void** endpointVolume) {
    *endpointVolume = nullptr;

    IMMDeviceEnumerator* deviceEnumerator = nullptr;
    IMMDevice*           defaultDevice    = nullptr;

    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_INPROC_SERVER,
        __uuidof(IMMDeviceEnumerator),
        (void**)&deviceEnumerator
    );
    if (FAILED(hr)) {
        std::cout << "[Audio] Failed to create device enumerator.\n";
        return false;
    }

    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    if (FAILED(hr)) {
        std::cout << "[Audio] Failed to get default audio endpoint.\n";
        deviceEnumerator->Release();
        return false;
    }

    hr = defaultDevice->Activate(
        __uuidof(IAudioEndpointVolume),
        CLSCTX_INPROC_SERVER,
        nullptr,
        endpointVolume
    );

    defaultDevice->Release();
    deviceEnumerator->Release();

    if (FAILED(hr)) {
        std::cout << "[Audio] Failed to activate endpoint volume.\n";
        return false;
    }

    return true;
}

// ─────────────────────────────────────────────────────────────
//  SET VOLUME  (0 – 100)
// ─────────────────────────────────────────────────────────────
bool AudioController::setVolume(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "[Audio] No volume level provided.\n";
        return false;
    }

    int volume = 0;
    try {
        volume = std::stoi(args[0]);
    } catch (...) {
        std::cout << "[Audio] Invalid volume value.\n";
        return false;
    }

    volume = max(0, min(100, volume));
    float normalized = volume / 100.0f;

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    IAudioEndpointVolume* ep = nullptr;
    if (!getEndpointVolume((void**)&ep)) {
        CoUninitialize();
        return false;
    }

    HRESULT hr = ep->SetMasterVolumeLevelScalar(normalized, nullptr);
    ep->Release();
    CoUninitialize();

    if (SUCCEEDED(hr))
        std::cout << "[Audio] Volume set to " << volume << "%\n";

    return SUCCEEDED(hr);
}

// ─────────────────────────────────────────────────────────────
//  MUTE
// ─────────────────────────────────────────────────────────────
bool AudioController::mute() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    IAudioEndpointVolume* ep = nullptr;
    if (!getEndpointVolume((void**)&ep)) { CoUninitialize(); return false; }

    HRESULT hr = ep->SetMute(TRUE, nullptr);
    ep->Release();
    CoUninitialize();

    if (SUCCEEDED(hr)) std::cout << "[Audio] Muted.\n";
    return SUCCEEDED(hr);
}

// ─────────────────────────────────────────────────────────────
//  UNMUTE
// ─────────────────────────────────────────────────────────────
bool AudioController::unmute() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    IAudioEndpointVolume* ep = nullptr;
    if (!getEndpointVolume((void**)&ep)) { CoUninitialize(); return false; }

    HRESULT hr = ep->SetMute(FALSE, nullptr);
    ep->Release();
    CoUninitialize();

    if (SUCCEEDED(hr)) std::cout << "[Audio] Unmuted.\n";
    return SUCCEEDED(hr);
}

// ─────────────────────────────────────────────────────────────
//  VOLUME UP  (+10%)
// ─────────────────────────────────────────────────────────────
bool AudioController::volumeUp() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    IAudioEndpointVolume* ep = nullptr;
    if (!getEndpointVolume((void**)&ep)) { CoUninitialize(); return false; }

    float current = 0.0f;
    ep->GetMasterVolumeLevelScalar(&current);

    float next = min(1.0f, current + VOLUME_STEP);
    HRESULT hr = ep->SetMasterVolumeLevelScalar(next, nullptr);

    ep->Release();
    CoUninitialize();

    if (SUCCEEDED(hr))
        std::cout << "[Audio] Volume up → " << static_cast<int>(next * 100) << "%\n";

    return SUCCEEDED(hr);
}

// ─────────────────────────────────────────────────────────────
//  VOLUME DOWN  (-10%)
// ─────────────────────────────────────────────────────────────
bool AudioController::volumeDown() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    IAudioEndpointVolume* ep = nullptr;
    if (!getEndpointVolume((void**)&ep)) { CoUninitialize(); return false; }

    float current = 0.0f;
    ep->GetMasterVolumeLevelScalar(&current);

    float next = max(0.0f, current - VOLUME_STEP);
    HRESULT hr = ep->SetMasterVolumeLevelScalar(next, nullptr);

    ep->Release();
    CoUninitialize();

    if (SUCCEEDED(hr))
        std::cout << "[Audio] Volume down → " << static_cast<int>(next * 100) << "%\n";

    return SUCCEEDED(hr);
}

// ─────────────────────────────────────────────────────────────
//  GET CURRENT VOLUME  (returns 0-100, -1 on failure)
// ─────────────────────────────────────────────────────────────
int AudioController::getCurrentVolume() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    IAudioEndpointVolume* ep = nullptr;
    if (!getEndpointVolume((void**)&ep)) { CoUninitialize(); return -1; }

    float level = 0.0f;
    ep->GetMasterVolumeLevelScalar(&level);
    ep->Release();
    CoUninitialize();

    return static_cast<int>(level * 100);
}

// ─────────────────────────────────────────────────────────────
//  MEDIA KEY HELPER  (SendInput with virtual key)
// ─────────────────────────────────────────────────────────────
static bool sendMediaKey(WORD vkCode) {
    INPUT inputs[2] = {};

    // Key down
    inputs[0].type       = INPUT_KEYBOARD;
    inputs[0].ki.wVk     = vkCode;
    inputs[0].ki.dwFlags = 0;

    // Key up
    inputs[1].type       = INPUT_KEYBOARD;
    inputs[1].ki.wVk     = vkCode;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    UINT sent = SendInput(2, inputs, sizeof(INPUT));
    return sent == 2;
}

// ─────────────────────────────────────────────────────────────
//  PLAY / PAUSE
// ─────────────────────────────────────────────────────────────
bool AudioController::playPause() {
    bool ok = sendMediaKey(VK_MEDIA_PLAY_PAUSE);
    if (ok) std::cout << "[Audio] Play/Pause toggled.\n";
    return ok;
}

// ─────────────────────────────────────────────────────────────
//  NEXT TRACK
// ─────────────────────────────────────────────────────────────
bool AudioController::nextTrack() {
    bool ok = sendMediaKey(VK_MEDIA_NEXT_TRACK);
    if (ok) std::cout << "[Audio] Next track.\n";
    return ok;
}

// ─────────────────────────────────────────────────────────────
//  PREVIOUS TRACK
// ─────────────────────────────────────────────────────────────
bool AudioController::prevTrack() {
    bool ok = sendMediaKey(VK_MEDIA_PREV_TRACK);
    if (ok) std::cout << "[Audio] Previous track.\n";
    return ok;
}
