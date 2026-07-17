//
// Created by karp on 05/05/2026.
// Video / display settings for the SDL renderer and window.
//

#ifndef CC_GAME_VIDEO_SETTINGS_H
#define CC_GAME_VIDEO_SETTINGS_H

#include <array>

namespace Game {

enum class VideoAspectRatioPreset {
    Ratio16x9,
};

enum class VideoWindowMode {
    Windowed,
    BorderlessFullscreen,
    ExclusiveFullscreen,
};

struct VideoResolutionPreset {
    int width = 1280;
    int height = 720;
};

inline constexpr std::array<VideoResolutionPreset, 8> kVideoResolutionPresets = {{
    {.width = 640, .height = 360},
    {.width = 1024, .height = 576},
    {.width = 1280, .height = 720},
    {.width = 1600, .height = 900},
    {.width = 1920, .height = 1080},
    {.width = 2560, .height = 1440},
    {.width = 3840, .height = 2160},
    // {.width = 7680, .height = 4320},
}};

struct VideoSettings {
    int logicalWidth = 1920;
    int logicalHeight = 1080;
    bool vsyncEnabled = false;
    VideoWindowMode windowMode = VideoWindowMode::Windowed;
    VideoAspectRatioPreset aspectRatio = VideoAspectRatioPreset::Ratio16x9;
    bool enableFrameLimiter = false;
    int maxFps = 200;
};

[[nodiscard]] constexpr std::size_t FindVideoResolutionPresetIndex(const int w, const int h) noexcept {
    for (std::size_t i = 0; i < kVideoResolutionPresets.size(); ++i) {
        if (kVideoResolutionPresets[i].width == w && kVideoResolutionPresets[i].height == h) {
            return i;
        }
    }
    return kVideoResolutionPresets.size() - 1;
}

inline void CycleVideoResolution(VideoSettings& settings) noexcept {
    const std::size_t idx = FindVideoResolutionPresetIndex(settings.logicalWidth, settings.logicalHeight);
    const std::size_t next = (idx + 1) % kVideoResolutionPresets.size();
    settings.logicalWidth = kVideoResolutionPresets[next].width;
    settings.logicalHeight = kVideoResolutionPresets[next].height;
}

inline void CycleVideoWindowMode(VideoSettings& settings) noexcept {
    switch (settings.windowMode) {
        case VideoWindowMode::Windowed:
            settings.windowMode = VideoWindowMode::BorderlessFullscreen;
            break;
        case VideoWindowMode::BorderlessFullscreen:
            settings.windowMode = VideoWindowMode::ExclusiveFullscreen;
            break;
        case VideoWindowMode::ExclusiveFullscreen:
            settings.windowMode = VideoWindowMode::Windowed;
            break;
    }
}

[[nodiscard]] constexpr const char* VideoWindowModeLabel(const VideoWindowMode mode) noexcept {
    switch (mode) {
        case VideoWindowMode::Windowed:
            return "Windowed";
        case VideoWindowMode::BorderlessFullscreen:
            return "Borderless fullscreen";
        case VideoWindowMode::ExclusiveFullscreen:
            return "Fullscreen";
    }
    return "Unknown";
}

[[nodiscard]] constexpr const char* VideoAspectRatioLabel(const VideoAspectRatioPreset preset) noexcept {
    switch (preset) {
        case VideoAspectRatioPreset::Ratio16x9:
            return "16:9";
    }
    return "Unknown";
}

} // namespace Game

#endif // CC_GAME_VIDEO_SETTINGS_H
