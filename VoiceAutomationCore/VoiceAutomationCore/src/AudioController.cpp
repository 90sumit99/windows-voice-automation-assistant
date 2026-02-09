#include "AudioController.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <iostream>

bool AudioController::getEndpointVolume(void** endpointVolume) {
    *endpointVolume = nullptr;

    IMMDeviceEnumerator* deviceEnumerator = nullptr;
    IMMDevice* defaultDevice = nullptr;

    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_INPROC_SERVER,
        __uuidof(IMMDeviceEnumerator),
        (void**)&deviceEnumerator
    );
    if (FAILED(hr)) return false;

    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    if (FAILED(hr)) {
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

    return SUCCEEDED(hr);
}


bool AudioController::setVolume(const std::vector<std::string>& args) {
    if (args.empty()) return false;

    int volume = std::stoi(args[0]);
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;

    float normalizedVolume = volume / 100.0f;

    CoInitialize(nullptr);

    IAudioEndpointVolume* endpointVolume = nullptr;
    if (!getEndpointVolume((void**)&endpointVolume)) {
        CoUninitialize();
        return false;
    }

    endpointVolume->SetMasterVolumeLevelScalar(normalizedVolume, nullptr);
    endpointVolume->Release();

    CoUninitialize();
    return true;
}

bool AudioController::mute() {
    CoInitialize(nullptr);

    IAudioEndpointVolume* endpointVolume = nullptr;
    if (!getEndpointVolume((void**)&endpointVolume)) {
        CoUninitialize();
        return false;
    }

    endpointVolume->SetMute(TRUE, nullptr);
    endpointVolume->Release();

    CoUninitialize();
    return true;
}

bool AudioController::unmute() {
    CoInitialize(nullptr);

    IAudioEndpointVolume* endpointVolume = nullptr;
    if (!getEndpointVolume((void**)&endpointVolume)) {
        CoUninitialize();
        return false;
    }

    endpointVolume->SetMute(FALSE, nullptr);
    endpointVolume->Release();

    CoUninitialize();
    return true;
}
