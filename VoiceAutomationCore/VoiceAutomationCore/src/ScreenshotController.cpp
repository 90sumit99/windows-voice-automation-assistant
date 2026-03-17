#include "ScreenshotController.h"
#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "Gdiplus.lib")

using namespace Gdiplus;

// ─────────────────────────────────────────────────────────────
//  GET SAVE PATH  — Desktop\Zyrex_Screenshot_YYYYMMDD_HHMMSS.png
// ─────────────────────────────────────────────────────────────
std::string ScreenshotController::getSavePath() const {
    char* userProfile = nullptr;
    size_t len = 0;
    _dupenv_s(&userProfile, &len, "USERPROFILE");
    std::string base = userProfile ? std::string(userProfile) : "C:\\Users\\Public";
    if (userProfile) free(userProfile);

    // Timestamp
    auto now   = std::chrono::system_clock::now();
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};
    localtime_s(&bt, &timer);

    std::ostringstream oss;
    oss << base << "\\Desktop\\Zyrex_Screenshot_"
        << std::put_time(&bt, "%Y%m%d_%H%M%S")
        << ".png";
    return oss.str();
}

// ─────────────────────────────────────────────────────────────
//  GET ENCODER CLSID helper
// ─────────────────────────────────────────────────────────────
static int getEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num  = 0;
    UINT size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    ImageCodecInfo* pInfo = (ImageCodecInfo*)(malloc(size));
    if (!pInfo) return -1;

    GetImageEncoders(num, size, pInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pInfo[j].MimeType, format) == 0) {
            *pClsid = pInfo[j].Clsid;
            free(pInfo);
            return j;
        }
    }
    free(pInfo);
    return -1;
}

// ─────────────────────────────────────────────────────────────
//  TAKE SCREENSHOT
// ─────────────────────────────────────────────────────────────
bool ScreenshotController::take() {
    // Init GDI+
    GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr);

    // Get screen dimensions
    int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);

    // Create compatible DC and bitmap
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem    = CreateCompatibleDC(hdcScreen);
    HBITMAP hBmp  = CreateCompatibleBitmap(hdcScreen, screenW, screenH);
    SelectObject(hdcMem, hBmp);

    // Copy screen to bitmap
    BitBlt(hdcMem, 0, 0, screenW, screenH,
           hdcScreen, screenX, screenY, SRCCOPY);

    // Save using GDI+
    std::string savePath = getSavePath();
    std::wstring wPath(savePath.begin(), savePath.end());

    Bitmap bitmap(hBmp, nullptr);
    CLSID pngClsid;
    getEncoderClsid(L"image/png", &pngClsid);
    Status status = bitmap.Save(wPath.c_str(), &pngClsid, nullptr);

    // Cleanup
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    GdiplusShutdown(gdiplusToken);

    if (status == Ok) {
        std::cout << "[Screenshot] Saved to: " << savePath << "\n";
        return true;
    }

    std::cout << "[Screenshot] Failed to save screenshot.\n";
    return false;
}
