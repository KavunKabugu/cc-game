//
// Created by karp on 09/04/2026.
//
#include "Game.h"

#include "Game/Profile.h"

#include <algorithm>
#include <functional>
#include <vector>

#include <SDL3/SDL.h>

#include "AudioManager.h"
#include "EventManager.h"
#include "Gameplay/GameplayMath.h"
#include "Gameplay/LaneInputHandler.h"
#include "ResourceManager.h"
#include "Scene/Scenes/MainMenuScene.h"
#include "SettingsStorage.h"
#include "Song/SongManager.h"

namespace Game {

namespace {

void LogVideoWarning(const char* message) {
    if (const char* err = SDL_GetError(); err && err[0] != '\0') {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "%s (%s)", message, err);
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "%s", message);
    }
}

void LogVideoError(const char* message) {
    if (const char* err = SDL_GetError(); err && err[0] != '\0') {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "%s (%s)", message, err);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "%s", message);
    }
}

} // namespace

GameInstance::GameInstance(SDL_Window* window, SDL_Renderer* renderer) : window(window), renderer(renderer) {
    SDL_ClearError();
    if (!SDL_SetDefaultTextureScaleMode(renderer, SDL_SCALEMODE_NEAREST)) {
        LogVideoWarning("SDL_SetDefaultTextureScaleMode(SDL_SCALEMODE_NEAREST) failed");
    }

    ResourceManager::getInstance().Init(renderer);
    ResourceManager::getInstance().AddSearchPath("resources");

    AudioManager::getInstance().Init();

    SDL_GetWindowSize(window, &savedWindowedWidth, &savedWindowedHeight);
    savedWindowedWidth = std::max(savedWindowedWidth, 640);
    savedWindowedHeight = std::max(savedWindowedHeight, 360);

    SettingsStorage::LoadAllInto(
        videoSettings,
        gameplaySettings,
        audioSettings,
        savedWindowedWidth,
        savedWindowedHeight);

    ApplyAudioSettingsToRuntime();
    ApplyLogicalPresentation();
    ApplyVsync();
    ApplyWindowModeFromSettings();

    // Refresh song library
    const auto s = Song::SongManager::GetInstance().RefreshLibrary();
    (void)s; // Stop analysis from complaining about discarded return value

    sceneManager.QueuePush<MainMenuScene>(std::ref(sceneManager), std::ref(*this));
    sceneManager.CommitQueuedTransitions();
    lastTickNs = SDL_GetTicksNS();
}

GameInstance::~GameInstance() {
    PersistAllSettings();
    sceneManager.QueueClear();
    sceneManager.CommitQueuedTransitions();

    AudioManager::getInstance().Shutdown();
    ResourceManager::getInstance().Shutdown();
}

void GameInstance::SaveWindowedSizeIfNeeded() {
    if (!window) {
        return;
    }

    if (const SDL_WindowFlags flags = SDL_GetWindowFlags(window); (flags & SDL_WINDOW_FULLSCREEN) != 0u) {
        return;
    }

    int w = 0;
    int h = 0;
    SDL_GetWindowSize(window, &w, &h);
    if (w > 0 && h > 0) {
        savedWindowedWidth = std::max(w, 640);
        savedWindowedHeight = std::max(h, 360);
    }
}

void GameInstance::ApplyLogicalPresentation() const {
    if (!renderer) {
        return;
    }

    SDL_ClearError();
    if (!SDL_SetRenderLogicalPresentation(
            renderer,
            videoSettings.logicalWidth,
            videoSettings.logicalHeight,
            SDL_LOGICAL_PRESENTATION_LETTERBOX
        )) {
        LogVideoError("SDL_SetRenderLogicalPresentation failed");
    }

    const float uiScale = std::max(
        Gameplay::ResolutionUniformScale(videoSettings.logicalWidth, videoSettings.logicalHeight),
        0.001f);
    ResourceManager::getInstance().SetUiFontScale(uiScale);
}

void GameInstance::ApplyVsync() const {
    if (!renderer) {
        return;
    }

    SDL_ClearError();
    if (const int vsyncValue = videoSettings.vsyncEnabled ? 1 : 0; !SDL_SetRenderVSync(renderer, vsyncValue)) {
        LogVideoWarning("SDL_SetRenderVSync failed");
    }
}

void GameInstance::RefreshExclusiveFullscreenDisplayMode() const {
    if (!window || videoSettings.windowMode != VideoWindowMode::ExclusiveFullscreen) {
        return;
    }

    const SDL_DisplayID displayID = SDL_GetDisplayForWindow(window);
    if (displayID == 0) {
        LogVideoError("SDL_GetDisplayForWindow failed while refreshing exclusive fullscreen mode");
        return;
    }

    SDL_DisplayMode closest{};
    SDL_ClearError();
    if (!SDL_GetClosestFullscreenDisplayMode(
            displayID,
            videoSettings.logicalWidth,
            videoSettings.logicalHeight,
            0.0f,
            true,
            &closest
        )) {
        LogVideoError("SDL_GetClosestFullscreenDisplayMode failed");
        return;
    }

    if (!SDL_SetWindowFullscreenMode(window, &closest)) {
        LogVideoWarning("SDL_SetWindowFullscreenMode failed while refreshing exclusive fullscreen");
    }

    SDL_SyncWindow(window);
}

void GameInstance::ApplyWindowModeFromSettings() {
    if (!window) {
        return;
    }

    SDL_ClearError();

    switch (videoSettings.windowMode) {
        case VideoWindowMode::Windowed: {
            if (!SDL_SetWindowFullscreen(window, false)) {
                LogVideoWarning("SDL_SetWindowFullscreen(false) failed entering windowed mode");
            }

            if (!SDL_SetWindowFullscreenMode(window, nullptr)) {
                LogVideoWarning("SDL_SetWindowFullscreenMode(nullptr) failed clearing fullscreen mode");
            }

            if (!SDL_SetWindowBordered(window, true)) {
                LogVideoWarning("SDL_SetWindowBordered(true) failed entering windowed mode");
            }

            const int w = std::max(savedWindowedWidth, 640);
            const int h = std::max(savedWindowedHeight, 360);
            if (!SDL_SetWindowSize(window, w, h)) {
                LogVideoWarning("SDL_SetWindowSize failed restoring windowed size");
            }

            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
            SDL_SyncWindow(window);
            return;
        }

        case VideoWindowMode::BorderlessFullscreen: {
            SaveWindowedSizeIfNeeded();

            if (!SDL_SetWindowFullscreen(window, false)) {
                LogVideoWarning("SDL_SetWindowFullscreen(false) failed before borderless fullscreen");
            }

            if (!SDL_SetWindowFullscreenMode(window, nullptr)) {
                LogVideoWarning("SDL_SetWindowFullscreenMode(nullptr) failed before borderless fullscreen");
            }

            const SDL_DisplayID displayID = SDL_GetDisplayForWindow(window);
            if (displayID == 0) {
                LogVideoError("SDL_GetDisplayForWindow failed for borderless fullscreen");
                return;
            }

            SDL_Rect bounds{};
            if (!SDL_GetDisplayBounds(displayID, &bounds)) {
                LogVideoError("SDL_GetDisplayBounds failed for borderless fullscreen");
                return;
            }

            if (!SDL_SetWindowBordered(window, false)) {
                LogVideoWarning("SDL_SetWindowBordered(false) failed for borderless fullscreen");
            }

            if (!SDL_SetWindowSize(window, bounds.w, bounds.h)) {
                LogVideoWarning("SDL_SetWindowSize failed for borderless fullscreen");
            }

            SDL_SetWindowPosition(window, bounds.x, bounds.y);

            if (!SDL_SetWindowFullscreenMode(window, nullptr)) {
                LogVideoWarning("SDL_SetWindowFullscreenMode(nullptr) optional failure for borderless fullscreen");
            }

            if (!SDL_SetWindowFullscreen(window, true)) {
                LogVideoError("SDL_SetWindowFullscreen(true) failed for borderless fullscreen");
            }

            SDL_SyncWindow(window);
            return;
        }

        case VideoWindowMode::ExclusiveFullscreen: {
            SaveWindowedSizeIfNeeded();

            if (!SDL_SetWindowFullscreen(window, false)) {
                LogVideoWarning("SDL_SetWindowFullscreen(false) failed before exclusive fullscreen");
            }

            if (!SDL_SetWindowFullscreenMode(window, nullptr)) {
                LogVideoWarning("SDL_SetWindowFullscreenMode(nullptr) failed resetting exclusive fullscreen");
            }

            const SDL_DisplayID displayID = SDL_GetDisplayForWindow(window);
            if (displayID == 0) {
                LogVideoError("SDL_GetDisplayForWindow failed for exclusive fullscreen");
                return;
            }

            SDL_DisplayMode closest{};
            if (!SDL_GetClosestFullscreenDisplayMode(
                    displayID,
                    videoSettings.logicalWidth,
                    videoSettings.logicalHeight,
                    0.0f,
                    true,
                    &closest
                )) {
                LogVideoError("SDL_GetClosestFullscreenDisplayMode failed for exclusive fullscreen");
                return;
            }

            if (!SDL_SetWindowBordered(window, false)) {
                LogVideoWarning("SDL_SetWindowBordered(false) failed for exclusive fullscreen");
            }

            if (!SDL_SetWindowFullscreenMode(window, &closest)) {
                LogVideoError("SDL_SetWindowFullscreenMode failed for exclusive fullscreen");
            }

            if (!SDL_SetWindowFullscreen(window, true)) {
                LogVideoError("SDL_SetWindowFullscreen(true) failed for exclusive fullscreen");
            }

            SDL_SyncWindow(window);
        }
    }
}

void GameInstance::CycleLogicalResolution() {
    CycleVideoResolution(videoSettings);
    ApplyLogicalPresentation();
    if (videoSettings.windowMode == VideoWindowMode::ExclusiveFullscreen) {
        RefreshExclusiveFullscreenDisplayMode();
    }
    PersistAllSettings();
}

void GameInstance::SetVsyncEnabled(const bool enabled) {
    videoSettings.vsyncEnabled = enabled;
    ApplyVsync();
    PersistAllSettings();
}

void GameInstance::ToggleVsync() {
    SetVsyncEnabled(!videoSettings.vsyncEnabled);
}

void GameInstance::SetFrameLimiterEnabled(const bool enabled) {
    if (videoSettings.enableFrameLimiter == enabled) {
        return;
    }
    videoSettings.enableFrameLimiter = enabled;
    PersistAllSettings();
}

void GameInstance::SetMaxFps(const int fps) {
    const int clamped = std::clamp(fps, 30, 1000);
    if (videoSettings.maxFps == clamped) {
        return;
    }
    videoSettings.maxFps = clamped;
    PersistAllSettings();
}

void GameInstance::SetWindowMode(const VideoWindowMode mode) {
    videoSettings.windowMode = mode;
    ApplyWindowModeFromSettings();
    PersistAllSettings();
}

void GameInstance::CycleWindowMode() {
    CycleVideoWindowMode(videoSettings);
    ApplyWindowModeFromSettings();
    PersistAllSettings();
}

void GameInstance::PersistAllSettings() const {
    (void)SettingsStorage::SaveAll(
        videoSettings,
        gameplaySettings,
        audioSettings,
        savedWindowedWidth,
        savedWindowedHeight);
}

void GameInstance::ApplyAudioSettingsToRuntime() {
    ClampAudioSettings(audioSettings);
    auto& audio = AudioManager::getInstance();
    audio.SetGlobalVolume(audioSettings.masterVolume);
    audio.SetCategoryVolume(AudioCategory::Music, audioSettings.musicVolume);
    audio.SetCategoryVolume(AudioCategory::Sfx, audioSettings.sfxVolume);
    audio.SetCategoryVolume(AudioCategory::Ui, audioSettings.uiVolume);
}

void GameInstance::SetMasterVolume(const float volume) {
    const float clamped = std::clamp(volume, 0.0f, 1.0f);
    if (clamped == audioSettings.masterVolume) {
        return;
    }
    audioSettings.masterVolume = clamped;
    AudioManager::getInstance().SetGlobalVolume(clamped);
    PersistAllSettings();
}

void GameInstance::SetMusicVolume(const float volume) {
    const float clamped = std::clamp(volume, 0.0f, 1.0f);
    if (clamped == audioSettings.musicVolume) {
        return;
    }
    audioSettings.musicVolume = clamped;
    AudioManager::getInstance().SetCategoryVolume(AudioCategory::Music, clamped);
    PersistAllSettings();
}

void GameInstance::SetSfxVolume(const float volume) {
    const float clamped = std::clamp(volume, 0.0f, 1.0f);
    if (clamped == audioSettings.sfxVolume) {
        return;
    }
    audioSettings.sfxVolume = clamped;
    AudioManager::getInstance().SetCategoryVolume(AudioCategory::Sfx, clamped);
    PersistAllSettings();
}

void GameInstance::SetUiVolume(const float volume) {
    const float clamped = std::clamp(volume, 0.0f, 1.0f);
    if (clamped == audioSettings.uiVolume) {
        return;
    }
    audioSettings.uiVolume = clamped;
    AudioManager::getInstance().SetCategoryVolume(AudioCategory::Ui, clamped);
    PersistAllSettings();
}

void GameInstance::SetNoteSpeed(const float speed) {
    const float clamped =
        std::clamp(speed, Gameplay::kGameplayScrollSpeedMin, Gameplay::kGameplayScrollSpeedMax);
    if (clamped == gameplaySettings.noteSpeed) {
        return;
    }
    gameplaySettings.noteSpeed = clamped;
    PersistAllSettings();
}

void GameInstance::SetCrosshairRadius(const float radius) {
    const float clamped =
        std::clamp(radius, Gameplay::kGameplayCrosshairRadiusMin, Gameplay::kGameplayCrosshairRadiusMax);
    if (clamped == gameplaySettings.crosshairRadius) {
        return;
    }
    gameplaySettings.crosshairRadius = clamped;
    PersistAllSettings();
}

void GameInstance::SetAudioOffsetSeconds(const double offsetSeconds) {
    constexpr double minSec = static_cast<double>(Gameplay::kGameplayAudioOffsetMsMin) * 1e-3;
    constexpr double maxSec = static_cast<double>(Gameplay::kGameplayAudioOffsetMsMax) * 1e-3;
    const double clamped = std::clamp(offsetSeconds, minSec, maxSec);
    if (clamped == gameplaySettings.audioOffsetSeconds) {
        return;
    }
    gameplaySettings.audioOffsetSeconds = clamped;
    PersistAllSettings();
}

void GameInstance::SetUseWallClockForJudgementTiming(const bool useWallClock) {
    if (useWallClock == gameplaySettings.useWallClockForJudgementTiming) {
        return;
    }
    gameplaySettings.useWallClockForJudgementTiming = useWallClock;
    PersistAllSettings();
}

void GameInstance::SetEnableBackgroundImage(const bool enabled) {
    if (enabled == gameplaySettings.enableBackgroundImage) {
        return;
    }
    gameplaySettings.enableBackgroundImage = enabled;
    PersistAllSettings();
}

void GameInstance::SetBackgroundOpacity(const float opacity) {
    const float clamped =
        std::clamp(opacity, Gameplay::kGameplayOpacityMin, Gameplay::kGameplayOpacityMax);
    if (clamped == gameplaySettings.backgroundOpacity) {
        return;
    }
    gameplaySettings.backgroundOpacity = clamped;
    PersistAllSettings();
}

void GameInstance::SetBackgroundColorR(const unsigned char value) {
    if (value == gameplaySettings.backgroundColorR) {
        return;
    }
    gameplaySettings.backgroundColorR = value;
    PersistAllSettings();
}

void GameInstance::SetBackgroundColorG(const unsigned char value) {
    if (value == gameplaySettings.backgroundColorG) {
        return;
    }
    gameplaySettings.backgroundColorG = value;
    PersistAllSettings();
}

void GameInstance::SetBackgroundColorB(const unsigned char value) {
    if (value == gameplaySettings.backgroundColorB) {
        return;
    }
    gameplaySettings.backgroundColorB = value;
    PersistAllSettings();
}

void GameInstance::SetEnablePlayfieldBorder(const bool enabled) {
    if (enabled == gameplaySettings.enablePlayfieldBorder) {
        return;
    }
    gameplaySettings.enablePlayfieldBorder = enabled;
    PersistAllSettings();
}

void GameInstance::SetSwapUpDownLanes(const bool enabled) {
    if (enabled == gameplaySettings.swapUpDownLanes) {
        return;
    }
    gameplaySettings.swapUpDownLanes = enabled;
    PersistAllSettings();
}

void GameInstance::SetPlayfieldBorderOpacity(const float opacity) {
    const float clamped =
        std::clamp(opacity, Gameplay::kGameplayOpacityMin, Gameplay::kGameplayOpacityMax);
    if (clamped == gameplaySettings.playfieldBorderOpacity) {
        return;
    }
    gameplaySettings.playfieldBorderOpacity = clamped;
    PersistAllSettings();
}

void GameInstance::SetPlayfieldBorderSize(const float size) {
    const float clamped =
        std::clamp(size, Gameplay::kGameplayBorderSizeMin, Gameplay::kGameplayBorderSizeMax);
    if (clamped == gameplaySettings.playfieldBorderSize) {
        return;
    }
    gameplaySettings.playfieldBorderSize = clamped;
    PersistAllSettings();
}

void GameInstance::SetGameplayLaneKeyBinding(const int lane, const int slot, const SDL_Keycode key) {
    if (lane < 0 || lane >= Gameplay::kLaneBindingCount) {
        return;
    }
    if (slot < 0 || slot >= Gameplay::kKeysPerLane) {
        return;
    }
    gameplaySettings.keyBindings[static_cast<size_t>(lane)][static_cast<size_t>(slot)] = key;
    Gameplay::NormalizeLaneKeyBindings(gameplaySettings.keyBindings);
    PersistAllSettings();
}

void GameInstance::Update() {
    CC_PROFILE("Game.Update");
    const Uint64 nowNs = SDL_GetTicksNS();
    const double dt = (lastTickNs == 0) ? 0.0 : static_cast<double>(nowNs - lastTickNs) / 1'000'000'000.0;
    lastTickNs = nowNs;

    AudioManager::getInstance().Update();

    std::vector<Container*> inputRoots;
    for (const IScene* scene : sceneManager.GetInputScenes()) {
        if (Container* root = scene->GetRoot()) {
            inputRoots.push_back(root);
        }
    }
    EventManager::getInstance().Dispatch(inputRoots, videoSettings.logicalWidth, videoSettings.logicalHeight);

    sceneManager.UpdateActiveScenes(dt);
    if (sceneManager.CommitQueuedTransitions()) {
        EventManager::getInstance().ResetInteractionState();
        if (transitionEventPolicy == TransitionEventPolicy::FlushPendingEventsOnSceneChange) {
            EventManager::getInstance().ClearQueue();
        }
    }
}

void GameInstance::Render() const {
    CC_PROFILE("Game.Render");
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    const SDL_FRect logicalViewport = {
        0.0f,
        0.0f,
        static_cast<float>(videoSettings.logicalWidth),
        static_cast<float>(videoSettings.logicalHeight),
    };
    sceneManager.RenderScenes(renderer, logicalViewport);

    SDL_RenderPresent(renderer);
}

Container* GameInstance::GetRoot() const {
    if (const IScene* scene = sceneManager.GetInputScene()) {
        return scene->GetRoot();
    }
    return nullptr;
}

} // namespace Game
