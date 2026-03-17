#pragma once
#include <string>

class ScreenshotController {
public:
    // Takes screenshot, saves to Desktop with timestamp
    // Returns saved file path, empty string on failure
    bool take();

private:
    std::string getSavePath() const;
};
