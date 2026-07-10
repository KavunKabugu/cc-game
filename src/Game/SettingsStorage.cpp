#include "SettingsStorage.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>

#include <SDL3/SDL_keycode.h>

#include "Game/PathUtf8.h"
#include "ThirdParty/json.hpp"

namespace Game::SettingsStorage {

namespace {

using json = nlohmann::json;

constexpr int kSettingsVersion = 2;
constexpr auto kSettingsFileName = "settings.json";

// Legacy pref-path single-purpose file (version 1).
// TODO: This eventually needs to be removed
constexpr int kLegacyGameplayFileVersion = 1;
constexpr auto kLegacyGameplayFileName = "gameplay_settings.json";

[[nodiscard]] std::optional<std::filesystem::path> ExecutableDirectory() {
    return Game::PathFromSdlBasePath();
}

[[nodiscard]] std::optional<std::filesystem::path> SettingsFilePath() {
    const auto dir = ExecutableDirectory();
    if (!dir) {
        return std::nullopt;
    }
    return *dir / kSettingsFileName;
}

[[nodiscard]] std::optional<std::filesystem::path> LegacyPrefDirectory() {
    return Game::PathFromSdlPrefPath("com.karp", "cc-game");
}

void ClampGameplayFields(Gameplay::GameplaySettings& s) {
    s.noteSpeed = std::clamp(s.noteSpeed, Gameplay::kGameplayScrollSpeedMin, Gameplay::kGameplayScrollSpeedMax);
    s.crosshairRadius =
        std::clamp(s.crosshairRadius, Gameplay::kGameplayCrosshairRadiusMin, Gameplay::kGameplayCrosshairRadiusMax);
    constexpr double minSec = static_cast<double>(Gameplay::kGameplayAudioOffsetMsMin) * 1e-3;
    constexpr double maxSec = static_cast<double>(Gameplay::kGameplayAudioOffsetMsMax) * 1e-3;
    s.audioOffsetSeconds = std::clamp(s.audioOffsetSeconds, minSec, maxSec);
    s.backgroundOpacity =
        std::clamp(s.backgroundOpacity, Gameplay::kGameplayOpacityMin, Gameplay::kGameplayOpacityMax);
    s.playfieldBorderOpacity =
        std::clamp(s.playfieldBorderOpacity, Gameplay::kGameplayOpacityMin, Gameplay::kGameplayOpacityMax);
    s.playfieldBorderSize =
        std::clamp(s.playfieldBorderSize, Gameplay::kGameplayBorderSizeMin, Gameplay::kGameplayBorderSizeMax);
    Gameplay::NormalizeLaneKeyBindings(s.keyBindings);
}

void SnapVideoResolutionToNearestPreset(VideoSettings& vs) {
    int bestW = kVideoResolutionPresets[0].width;
    int bestH = kVideoResolutionPresets[0].height;
    auto bestCost = std::numeric_limits<long>::max();
    for (const auto&[width, height] : kVideoResolutionPresets) {
        const long dw = static_cast<long>(vs.logicalWidth) - width;
        const long dh = static_cast<long>(vs.logicalHeight) - height;
        if (const long cost = dw * dw + dh * dh; cost < bestCost) {
            bestCost = cost;
            bestW = width;
            bestH = height;
        }
    }
    vs.logicalWidth = bestW;
    vs.logicalHeight = bestH;
}

[[nodiscard]] const char* WindowModeToString(const VideoWindowMode mode) noexcept {
    switch (mode) {
        case VideoWindowMode::Windowed:
            return "Windowed";
        case VideoWindowMode::BorderlessFullscreen:
            return "BorderlessFullscreen";
        case VideoWindowMode::ExclusiveFullscreen:
            return "ExclusiveFullscreen";
    }
    return "Windowed";
}

[[nodiscard]] VideoWindowMode WindowModeFromString(const std::string_view s) {
    if (s == "BorderlessFullscreen") {
        return VideoWindowMode::BorderlessFullscreen;
    }
    if (s == "ExclusiveFullscreen") {
        return VideoWindowMode::ExclusiveFullscreen;
    }
    return VideoWindowMode::Windowed;
}

// TODO: Template for now, expand later
[[nodiscard]] const char* AspectRatioToString(const VideoAspectRatioPreset preset) noexcept {
    switch (preset) {
        case VideoAspectRatioPreset::Ratio16x9:
            return "Ratio16x9";
    }
    return "Ratio16x9";
}

// TODO: Template for now, expand later
[[nodiscard]] VideoAspectRatioPreset AspectRatioFromString(const std::string_view s) {
    if (s == "Ratio16x9") {
        return VideoAspectRatioPreset::Ratio16x9;
    }
    return VideoAspectRatioPreset::Ratio16x9;
}

void ApplyAudioJson(const json& j, AudioSettings& settings) {
    if (j.contains("masterVolume") && j["masterVolume"].is_number()) {
        settings.masterVolume = j["masterVolume"].get<float>();
    }
    if (j.contains("musicVolume") && j["musicVolume"].is_number()) {
        settings.musicVolume = j["musicVolume"].get<float>();
    }
    if (j.contains("sfxVolume") && j["sfxVolume"].is_number()) {
        settings.sfxVolume = j["sfxVolume"].get<float>();
    }
    if (j.contains("uiVolume") && j["uiVolume"].is_number()) {
        settings.uiVolume = j["uiVolume"].get<float>();
    }
}

void ApplyKeyBindingsJson(const json& j, Gameplay::LaneKeyBindings& bindings) {
    if (!j.is_array() || j.size() != static_cast<size_t>(Gameplay::kLaneBindingCount)) {
        return;
    }
    for (size_t lane = 0; lane < static_cast<size_t>(Gameplay::kLaneBindingCount); ++lane) {
        const auto& laneJ = j[lane];
        if (!laneJ.is_array() || laneJ.size() != static_cast<size_t>(Gameplay::kKeysPerLane)) {
            continue;
        }
        for (size_t slot = 0; slot < static_cast<size_t>(Gameplay::kKeysPerLane); ++slot) {
            if (laneJ[slot].is_number_integer()) {
                bindings[lane][slot] = static_cast<SDL_Keycode>(laneJ[slot].get<std::int64_t>());
            }
        }
    }
}

void ApplyGameplayJson(const json& j, Gameplay::GameplaySettings& settings) {
    if (j.contains("noteSpeed") && j["noteSpeed"].is_number()) {
        settings.noteSpeed = j["noteSpeed"].get<float>();
    }
    if (j.contains("crosshairRadius") && j["crosshairRadius"].is_number()) {
        settings.crosshairRadius = j["crosshairRadius"].get<float>();
    }
    if (j.contains("audioOffsetSeconds") && j["audioOffsetSeconds"].is_number()) {
        settings.audioOffsetSeconds = j["audioOffsetSeconds"].get<double>();
    }
    if (j.contains("useWallClockForJudgementTiming") && j["useWallClockForJudgementTiming"].is_boolean()) {
        settings.useWallClockForJudgementTiming = j["useWallClockForJudgementTiming"].get<bool>();
    }
    if (j.contains("enableBackgroundImage") && j["enableBackgroundImage"].is_boolean()) {
        settings.enableBackgroundImage = j["enableBackgroundImage"].get<bool>();
    }
    if (j.contains("backgroundOpacity") && j["backgroundOpacity"].is_number()) {
        settings.backgroundOpacity = j["backgroundOpacity"].get<float>();
    }
    if (j.contains("backgroundColorR") && j["backgroundColorR"].is_number_integer()) {
        settings.backgroundColorR = static_cast<unsigned char>(
            std::clamp(j["backgroundColorR"].get<int>(), 0, 255));
    }
    if (j.contains("backgroundColorG") && j["backgroundColorG"].is_number_integer()) {
        settings.backgroundColorG = static_cast<unsigned char>(
            std::clamp(j["backgroundColorG"].get<int>(), 0, 255));
    }
    if (j.contains("backgroundColorB") && j["backgroundColorB"].is_number_integer()) {
        settings.backgroundColorB = static_cast<unsigned char>(
            std::clamp(j["backgroundColorB"].get<int>(), 0, 255));
    }
    if (j.contains("enablePlayfieldBorder") && j["enablePlayfieldBorder"].is_boolean()) {
        settings.enablePlayfieldBorder = j["enablePlayfieldBorder"].get<bool>();
    }
    if (j.contains("playfieldBorderOpacity") && j["playfieldBorderOpacity"].is_number()) {
        settings.playfieldBorderOpacity = j["playfieldBorderOpacity"].get<float>();
    }
    if (j.contains("playfieldBorderSize") && j["playfieldBorderSize"].is_number()) {
        settings.playfieldBorderSize = j["playfieldBorderSize"].get<float>();
    }
    if (j.contains("keyBindings") && j["keyBindings"].is_array()) {
        ApplyKeyBindingsJson(j["keyBindings"], settings.keyBindings);
    }
}

void ApplyVideoJson(const json& j, VideoSettings& video, int& savedWindowedWidth, int& savedWindowedHeight) {
    if (j.contains("logicalWidth") && j["logicalWidth"].is_number_integer()) {
        video.logicalWidth = j["logicalWidth"].get<int>();
    }
    if (j.contains("logicalHeight") && j["logicalHeight"].is_number_integer()) {
        video.logicalHeight = j["logicalHeight"].get<int>();
    }
    if (video.logicalWidth <= 0 || video.logicalHeight <= 0) {
        constexpr VideoSettings defaults{};
        video.logicalWidth = defaults.logicalWidth;
        video.logicalHeight = defaults.logicalHeight;
    } else {
        SnapVideoResolutionToNearestPreset(video);
    }

    if (j.contains("vsyncEnabled") && j["vsyncEnabled"].is_boolean()) {
        video.vsyncEnabled = j["vsyncEnabled"].get<bool>();
    }

    if (j.contains("windowMode") && j["windowMode"].is_string()) {
        video.windowMode = WindowModeFromString(j["windowMode"].get<std::string>());
    }

    if (j.contains("aspectRatio") && j["aspectRatio"].is_string()) {
        video.aspectRatio = AspectRatioFromString(j["aspectRatio"].get<std::string>());
    }

    if (j.contains("enableFrameLimiter") && j["enableFrameLimiter"].is_boolean()) {
        video.enableFrameLimiter = j["enableFrameLimiter"].get<bool>();
    }

    if (j.contains("maxFps") && j["maxFps"].is_number_integer()) {
        video.maxFps = std::clamp(static_cast<int>(j["maxFps"].get<int>()), 30, 1000);
    }

    if (j.contains("windowedWidth") && j["windowedWidth"].is_number_integer()) {
        savedWindowedWidth = std::max(j["windowedWidth"].get<int>(), 640);
    }
    if (j.contains("windowedHeight") && j["windowedHeight"].is_number_integer()) {
        savedWindowedHeight = std::max(j["windowedHeight"].get<int>(), 360);
    }
}

void TryLoadLegacyGameplayPrefs(Gameplay::GameplaySettings& settings) {
    const auto prefDir = LegacyPrefDirectory();
    if (!prefDir) {
        return;
    }
    const auto path = *prefDir / kLegacyGameplayFileName;
    if (!std::filesystem::exists(path)) {
        return;
    }

    std::ifstream in(path);
    if (!in) {
        return;
    }

    try {
        json j;
        in >> j;
        // If you are here with clangd, ignore the error here.
        // It fails to find the correct nlohmann::json::value template
        // and thinks one doesn't exist, when it does.
        // The code does compile and run correctly, so this *should be* a cosmetic issue.
        if (j.value("version", 0) != kLegacyGameplayFileVersion) {
            return;
        }
        ApplyGameplayJson(j, settings);
    } catch (...) {}
}

} // namespace

void LoadAllInto(
    VideoSettings& video,
    Gameplay::GameplaySettings& gameplay,
    AudioSettings& audio,
    int& savedWindowedWidth,
    int& savedWindowedHeight
) {
    ClampGameplayFields(gameplay);
    ClampAudioSettings(audio);

    const auto pathOpt = SettingsFilePath();
    if (!pathOpt || !std::filesystem::exists(*pathOpt)) {
        TryLoadLegacyGameplayPrefs(gameplay);
        ClampGameplayFields(gameplay);
        ClampAudioSettings(audio);
        return;
    }

    std::ifstream in(*pathOpt);
    if (!in) {
        TryLoadLegacyGameplayPrefs(gameplay);
        ClampGameplayFields(gameplay);
        ClampAudioSettings(audio);
        return;
    }

    try {
        json j;
        in >> j;
        // If you are here with clangd, ignore the error here, see above.
        if (const int version = j.value("version", 0); version != kSettingsVersion) {
            TryLoadLegacyGameplayPrefs(gameplay);
            ClampGameplayFields(gameplay);
            ClampAudioSettings(audio);
            return;
        }
        if (j.contains("video") && j["video"].is_object()) {
            ApplyVideoJson(j["video"], video, savedWindowedWidth, savedWindowedHeight);
        }
        if (j.contains("gameplay") && j["gameplay"].is_object()) {
            ApplyGameplayJson(j["gameplay"], gameplay);
        }
        if (j.contains("audio") && j["audio"].is_object()) {
            ApplyAudioJson(j["audio"], audio);
        }
    } catch (...) {
        TryLoadLegacyGameplayPrefs(gameplay);
    }

    ClampGameplayFields(gameplay);
    ClampAudioSettings(audio);
}

bool SaveAll(
    const VideoSettings& video,
    const Gameplay::GameplaySettings& gameplay,
    const AudioSettings& audio,
    const int savedWindowedWidth,
    const int savedWindowedHeight
) {
    Gameplay::GameplaySettings gCopy = gameplay;
    ClampGameplayFields(gCopy);

    AudioSettings aCopy = audio;
    ClampAudioSettings(aCopy);

    VideoSettings vCopy = video;
    if (vCopy.logicalWidth <= 0 || vCopy.logicalHeight <= 0) {
        vCopy = VideoSettings{};
    }
    SnapVideoResolutionToNearestPreset(vCopy);

    json keyBindingsJson = json::array();
    for (int lane = 0; lane < Gameplay::kLaneBindingCount; ++lane) {
        json laneJ = json::array();
        for (int slot = 0; slot < Gameplay::kKeysPerLane; ++slot) {
            laneJ.push_back(gCopy.keyBindings[static_cast<size_t>(lane)][static_cast<size_t>(slot)]);
        }
        keyBindingsJson.push_back(std::move(laneJ));
    }

    const int ww = std::max(savedWindowedWidth, 640);
    const int wh = std::max(savedWindowedHeight, 360);

    const auto pathOpt = SettingsFilePath();
    if (!pathOpt) {
        return false;
    }

    if (const auto parent = pathOpt->parent_path(); !parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            return false;
        }
    }

    json j = {
        {"version", kSettingsVersion},
        {"video",
         {
             {"logicalWidth", vCopy.logicalWidth},
             {"logicalHeight", vCopy.logicalHeight},
             {"vsyncEnabled", vCopy.vsyncEnabled},
             {"windowMode", WindowModeToString(vCopy.windowMode)},
             {"aspectRatio", AspectRatioToString(vCopy.aspectRatio)},
             {"enableFrameLimiter", vCopy.enableFrameLimiter},
             {"maxFps", vCopy.maxFps},
             {"windowedWidth", ww},
             {"windowedHeight", wh},
         }},
        {"gameplay",
         {
             {"noteSpeed", gCopy.noteSpeed},
             {"crosshairRadius", gCopy.crosshairRadius},
             {"audioOffsetSeconds", gCopy.audioOffsetSeconds},
             {"useWallClockForJudgementTiming", gCopy.useWallClockForJudgementTiming},
             {"enableBackgroundImage", gCopy.enableBackgroundImage},
             {"backgroundOpacity", gCopy.backgroundOpacity},
             {"backgroundColorR", gCopy.backgroundColorR},
             {"backgroundColorG", gCopy.backgroundColorG},
             {"backgroundColorB", gCopy.backgroundColorB},
             {"enablePlayfieldBorder", gCopy.enablePlayfieldBorder},
             {"playfieldBorderOpacity", gCopy.playfieldBorderOpacity},
             {"playfieldBorderSize", gCopy.playfieldBorderSize},
             {"keyBindings", keyBindingsJson},
         }},
        {"audio",
         {
             {"masterVolume", aCopy.masterVolume},
             {"musicVolume", aCopy.musicVolume},
             {"sfxVolume", aCopy.sfxVolume},
             {"uiVolume", aCopy.uiVolume},
         }},
    };

    std::ofstream out(*pathOpt, std::ios::trunc);
    if (!out) {
        return false;
    }
    out << j.dump(2);
    return !out.fail();
}

} // namespace Game::SettingsStorage
