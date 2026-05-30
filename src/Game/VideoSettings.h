//
// Created by karp on 05/05/2026.
// Video / display settings for the SDL renderer and window.
//

#ifndef CC_GAME_VIDEO_SETTINGS_H
#define CC_GAME_VIDEO_SETTINGS_H

#include <array>
#include <cstddef>

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
    {640, 360},
    {1024, 576},
    {1280, 720},
    {1600, 900},
    {1920, 1080},
    {2560, 1440},
    {3840, 2160},
    {7680, 4320},
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
