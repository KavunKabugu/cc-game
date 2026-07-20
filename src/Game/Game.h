//
// Created by karp on 09/04/2026.
//

#ifndef CC_GAME_GAME_H
#define CC_GAME_GAME_H

#include <SDL3/SDL.h>

#include "AudioSettings.h"
#include "Gameplay/GameplaySettings.h"
#include "Scene/SceneManager.h"
#include "VideoSettings.h"

namespace Game {

class Container;

class GameInstance {
public:
    enum class TransitionEventPolicy {
        KeepPendingEvents,
        FlushPendingEventsOnSceneChange
    };

    GameInstance(SDL_Window *window, SDL_Renderer *renderer);
    ~GameInstance();
    void Update();
    void Render() const;

    [[nodiscard]] Container* GetRoot() const;
    [[nodiscard]] SceneManager& GetSceneManager() { return sceneManager; }
    [[nodiscard]] const SceneManager& GetSceneManager() const { return sceneManager; }

    [[nodiscard]] const VideoSettings& GetVideoSettings() const { return videoSettings; }

    [[nodiscard]] const Gameplay::GameplaySettings& GetGameplaySettings() const { return gameplaySettings; }

    [[nodiscard]] const AudioSettings& GetAudioSettings() const { return audioSettings; }

    void SetMasterVolume(float volume);
    void SetMusicVolume(float volume);
    void SetSfxVolume(float volume);
    void SetUiVolume(float volume);

    void SetNoteSpeed(float speed);
    void SetCrosshairRadius(float radius);
    void SetAudioOffsetSeconds(double offsetSeconds);
    void SetUseWallClockForJudgementTiming(bool useWallClock);
    void SetEnableBackgroundImage(bool enabled);
    void SetBackgroundOpacity(float opacity);
    void SetBackgroundColorR(unsigned char value);
    void SetBackgroundColorG(unsigned char value);
    void SetBackgroundColorB(unsigned char value);
    void SetEnablePlayfieldBorder(bool enabled);
    void SetPlayfieldBorderOpacity(float opacity);
    void SetPlayfieldBorderSize(float size);
    void SetSwapUpDownLanes(bool enabled);

    void SetPlayerName(std::string name);

    void SetGameplayLaneKeyBinding(int lane, int slot, SDL_Keycode key);

    void CycleLogicalResolution();
    void SetVsyncEnabled(bool enabled);
    void ToggleVsync();
    void SetFrameLimiterEnabled(bool enabled);
    void SetMaxFps(int fps);
    void SetWindowMode(VideoWindowMode mode);
    void CycleWindowMode();

private:
    void ApplyLogicalPresentation() const;
    void ApplyVsync() const;
    void ApplyWindowModeFromSettings();
    void RefreshExclusiveFullscreenDisplayMode() const;
    void SaveWindowedSizeIfNeeded();
    void ApplyAudioSettingsToRuntime();
    void PersistAllSettings() const;

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    SceneManager sceneManager;
    VideoSettings videoSettings{};
    Gameplay::GameplaySettings gameplaySettings{};
    AudioSettings audioSettings{};
    int savedWindowedWidth = 1280;
    int savedWindowedHeight = 720;

    Uint64 lastTickNs = 0;
    TransitionEventPolicy transitionEventPolicy = TransitionEventPolicy::FlushPendingEventsOnSceneChange;
};

} // Game

#endif //CC_GAME_GAME_H
