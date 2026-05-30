#include "OptionsOverlayScene.h"

#include <array>
#include <cmath>
#include <format>
#include <functional>
#include <string>
#include <string_view>

#include <SDL3/SDL_keyboard.h>

#include "Game/Events/Interfaces.h"
#include "Game/Game.h"
#include "Game/Gameplay/GameplayConstants.h"
#include "Game/Gameplay/LaneInputHandler.h"
#include "Game/objects/GameObject.h"
#include "Game/ResourceManager.h"
#include "Game/Scene/SceneManager.h"
#include "Game/Scene/Scenes/MainMenuScene.h"
#include "Game/VideoSettings.h"
#include "Game/objects/Container.h"
#include "Game/objects/Label.h"
#include "Game/objects/PanelRect.h"
#include "Game/objects/Slider.h"
#include "Game/objects/Sprite.h"
#include "Game/objects/TextButton.h"

namespace Game {

namespace {

std::string KeyLabel(const SDL_Keycode key) {
    if (const char* name = SDL_GetKeyName(key); name != nullptr && name[0] != '\0') {
        return std::string{name};
    }
    return "?";
}

class KeyBindingCaptureGate final : public GameObject, public IKeyHandler, public IMouseClickable, public IMouseScrollable {
public:
    KeyBindingCaptureGate(
        const UnitBounds bounds,
        std::function<bool()> capturing,
        std::function<void(SDL_Keycode)> commit,
        std::function<void()> cancelEsc)
        : GameObject(bounds),
          capturing(std::move(capturing)),
          commit(std::move(commit)),
          cancelEsc(std::move(cancelEsc)) {}

    void Update() override {}

    IKeyHandler* AsKeyHandler() override { return (capturing && capturing()) ? this : nullptr; }
    IMouseClickable* AsMouseClickable() override { return (capturing && capturing()) ? this : nullptr; }
    IMouseScrollable* AsMouseScrollable() override { return (capturing && capturing()) ? this : nullptr; }

    bool OnKeyDown(const SDL_Keycode key, const Uint64 timestamp) override {
        (void)timestamp;
        if (!capturing || !capturing()) {
            return false;
        }
        if (key == SDLK_ESCAPE) {
            if (cancelEsc) {
                cancelEsc();
            }
            return true;
        }
        if (commit) {
            commit(key);
        }
        return true;
    }

    bool OnKeyUp(const SDL_Keycode key, const Uint64 timestamp) override {
        (void)key;
        (void)timestamp;
        if (!capturing || !capturing()) {
            return false;
        }
        return true;
    }

    bool OnMouseButtonDown(const int button, const UnitPoint localPos) override {
        (void)button;
        (void)localPos;
        return capturing && capturing();
    }

    bool OnMouseButtonUp(const int button, const UnitPoint localPos) override {
        (void)button;
        (void)localPos;
        return capturing && capturing();
    }

    void OnMouseButtonClicked(const int button, const UnitPoint localPos) override {
        (void)button;
        (void)localPos;
    }

    bool OnMouseWheel(const float x, const float y) override {
        (void)x;
        (void)y;
        return capturing && capturing();
    }

private:
    std::function<bool()> capturing;
    std::function<void(SDL_Keycode)> commit;
    std::function<void()> cancelEsc;
};

class OverlayCloseHandler final : public GameObject, public IKeyHandler {
public:
    explicit OverlayCloseHandler(const UnitBounds bounds, std::function<void()> onClose)
        : GameObject(bounds),
          onClose(std::move(onClose)) {}

    void Update() override {}

    IKeyHandler* AsKeyHandler() override { return this; }

    bool OnKeyDown(const SDL_Keycode key, const Uint64 timestamp) override {
        (void)timestamp;
        if (key != SDLK_ESCAPE) {
            return false;
        }

        if (onClose) {
            onClose();
        }
        return true;
    }

    bool OnKeyUp(const SDL_Keycode key, const Uint64 timestamp) override {
        (void)key;
        (void)timestamp;
        return false;
    }

private:
    std::function<void()> onClose;
};

std::string_view CategoryTitle(const OptionsOverlayScene::Category category) {
    switch (category) {
        case OptionsOverlayScene::Category::Video:
            return "Video";
        case OptionsOverlayScene::Category::Audio:
            return "Audio";
        case OptionsOverlayScene::Category::Input:
            return "Input";
        case OptionsOverlayScene::Category::Gameplay:
            return "Gameplay";
    }
    return "Unknown";
}

std::string ResolutionButtonText(const VideoSettings& settings) {
    return std::to_string(settings.logicalWidth) + " x " + std::to_string(settings.logicalHeight);
}

std::string VsyncButtonText(const VideoSettings& settings) {
    return settings.vsyncEnabled ? std::string("On") : std::string("Off");
}

std::string WindowModeButtonText(const VideoSettings& settings) {
    return VideoWindowModeLabel(settings.windowMode);
}

} // namespace

OptionsOverlayScene::OptionsOverlayScene(SceneManager& sceneManager, GameInstance& gameInstance)
    : sceneManager(sceneManager),
      game(gameInstance) {
    const VideoSettings& vOpen = game.GetVideoSettings();
    overlayOpenedLogicalW = vOpen.logicalWidth;
    overlayOpenedLogicalH = vOpen.logicalHeight;

    root->CreateChild<PanelRect>(UnitBounds{{0.0f, 0.0f}, {1.0f, 1.0f}}, SDL_Color{0, 0, 0, 150});

    auto overlayTextureRes = ResourceManager::getInstance().Get<SDL_Texture>("overlay_bg.png");
    const auto buttonTextureRes = ResourceManager::getInstance().Get<SDL_Texture>("button.png");
    auto titleFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 48.0f);
    auto sectionFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 34.0f);
    const auto bodyFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 28.0f);
    if (!titleFontRes || !sectionFontRes || !bodyFontRes) {
        return;
    }
    font = *bodyFontRes;

    root->CreateChild<PanelRect>(UnitBounds{{0.12f, 0.1f}, {0.88f, 0.9f}}, SDL_Color{22, 24, 34, 235});
    if (overlayTextureRes) {
        root->CreateChild<Sprite>(UnitBounds{{0.12f, 0.1f}, {0.88f, 0.9f}}, *overlayTextureRes);
    }

    auto* title = root->CreateChild<Label>(
        UnitBounds{{0.16f, 0.13f}, {0.84f, 0.22f}},
        *titleFontRes,
        "Options");
    title->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);

    root->CreateChild<PanelRect>(UnitBounds{{0.16f, 0.24f}, {0.35f, 0.84f}}, SDL_Color{32, 36, 53, 235});
    root->CreateChild<PanelRect>(UnitBounds{{0.37f, 0.24f}, {0.84f, 0.84f}}, SDL_Color{28, 32, 46, 235});

    const std::shared_ptr<SDL_Texture> buttonTexture = buttonTextureRes ? *buttonTextureRes : nullptr;

    root->CreateChild<TextButton>(
        UnitBounds{{0.17f, 0.27f}, {0.34f, 0.35f}},
        *sectionFontRes,
        "Video",
        [this]() {
            currentCategory = Category::Video;
            RebuildContent(currentCategory);
        },
        buttonTexture);
    root->CreateChild<TextButton>(
        UnitBounds{{0.17f, 0.37f}, {0.34f, 0.45f}},
        *sectionFontRes,
        "Audio",
        [this]() {
            currentCategory = Category::Audio;
            RebuildContent(currentCategory);
        },
        buttonTexture);
    root->CreateChild<TextButton>(
        UnitBounds{{0.17f, 0.47f}, {0.34f, 0.55f}},
        *sectionFontRes,
        "Input",
        [this]() {
            currentCategory = Category::Input;
            RebuildContent(currentCategory);
        },
        buttonTexture);
    root->CreateChild<TextButton>(
        UnitBounds{{0.17f, 0.57f}, {0.34f, 0.65f}},
        *sectionFontRes,
        "Gameplay",
        [this]() {
            currentCategory = Category::Gameplay;
            RebuildContent(currentCategory);
        },
        buttonTexture);

    root->CreateChild<TextButton>(
        UnitBounds{{0.77f, 0.13f}, {0.84f, 0.2f}},
        *sectionFontRes,
        "X",
        [this]() {
            CloseOverlay();
        },
        buttonTexture);

    contentContainer = root->CreateChild<Container>(UnitBounds{{0.39f, 0.26f}, {0.82f, 0.82f}});
    RebuildContent(currentCategory);

    keyCaptureBackdrop =
        root->CreateChild<PanelRect>(UnitBounds{{0.0f, 0.0f}, {1.0f, 1.0f}}, SDL_Color{0, 0, 0, 0});
    keyCapturePrompt = root->CreateChild<Label>(
        UnitBounds{{0.1f, 0.42f}, {0.9f, 0.5f}},
        *bodyFontRes,
        "");
    keyCapturePrompt->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);

    root->CreateChild<KeyBindingCaptureGate>(
        UnitBounds{{0.0f, 0.0f}, {1.0f, 1.0f}},
        [this]() {
            return keyCaptureActive;
        },
        [this](const SDL_Keycode key) {
            CompleteKeyRebind(key);
        },
        [this]() {
            CancelKeyRebind();
        });

    root->CreateChild<OverlayCloseHandler>(
        UnitBounds{{0.0f, 0.0f}, {0.0f, 0.0f}},
        [this]() {
            CloseOverlay();
        });
}

void OptionsOverlayScene::CloseOverlay() {
    EndKeyRebind();

    const VideoSettings& now = game.GetVideoSettings();
    const bool logicalResolutionChanged =
        now.logicalWidth != overlayOpenedLogicalW || now.logicalHeight != overlayOpenedLogicalH;

    sceneManager.QueuePop();
    if (logicalResolutionChanged) {
        sceneManager.QueueReplace<MainMenuScene>(std::ref(sceneManager), std::ref(game));
    }
}

void OptionsOverlayScene::RefreshVideoControlTexts() const {
    const VideoSettings& vs = game.GetVideoSettings();
    if (videoResolutionButton) {
        videoResolutionButton->SetText(ResolutionButtonText(vs));
    }
    if (videoVsyncButton) {
        videoVsyncButton->SetText(VsyncButtonText(vs));
    }
    if (videoWindowModeButton) {
        videoWindowModeButton->SetText(WindowModeButtonText(vs));
    }
    if (videoLimitFramerateButton) {
        videoLimitFramerateButton->SetText(vs.enableFrameLimiter ? "On" : "Off");
    }
    if (videoFramerateLimitValueLabel) {
        videoFramerateLimitValueLabel->SetText(std::to_string(vs.maxFps) + " fps");
    }
}

void OptionsOverlayScene::RefreshGameplayControlTexts() const {
    const Gameplay::GameplaySettings& gs = game.GetGameplaySettings();
    if (gameplayScrollSpeedValueLabel) {
        gameplayScrollSpeedValueLabel->SetText(std::format("{:.1f}", gs.noteSpeed));
    }
    if (gameplayCrosshairValueLabel) {
        gameplayCrosshairValueLabel->SetText(std::format("{:.0f}", gs.crosshairRadius));
    }
    if (gameplayAudioOffsetValueLabel) {
        const auto ms = static_cast<float>(gs.audioOffsetSeconds * 1000.0);
        gameplayAudioOffsetValueLabel->SetText(std::format("{:.1f} ms", ms));
    }
    if (gameplaySongClockTimingCheckbox) {
        // Checked (X) = song-time path, empty = wall clock path.
        // TODO: Why not label the option "Use Wall Clock for Song Timing" instead of doing this weird flip?
        gameplaySongClockTimingCheckbox->SetText(gs.useWallClockForJudgementTiming ? "" : "X");
    }
}

void OptionsOverlayScene::RefreshAudioControlTexts() const {
    if (!audioMasterValueLabel || !audioMusicValueLabel || !audioSfxValueLabel || !audioUiValueLabel) {
        return;
    }

    const auto&[masterVolume, musicVolume, sfxVolume, uiVolume] = game.GetAudioSettings();
    audioMasterValueLabel->SetText(std::format("{:.0f}%", masterVolume * 100.0f));
    audioMusicValueLabel->SetText(std::format("{:.0f}%", musicVolume * 100.0f));
    audioSfxValueLabel->SetText(std::format("{:.0f}%", sfxVolume * 100.0f));
    audioUiValueLabel->SetText(std::format("{:.0f}%", uiVolume * 100.0f));
}

void OptionsOverlayScene::RefreshInputBindButtonTexts() const {
    const Gameplay::GameplaySettings& gs = game.GetGameplaySettings();
    for (int lane = 0; lane < Gameplay::kLaneBindingCount; ++lane) {
        for (int slot = 0; slot < Gameplay::kKeysPerLane; ++slot) {
            TextButton* btn = inputBindButtons[lane][slot];
            if (!btn) {
                continue;
            }
            btn->SetText(KeyLabel(gs.keyBindings[static_cast<size_t>(lane)][static_cast<size_t>(slot)]));
        }
    }
}

void OptionsOverlayScene::BeginKeyRebind(const int lane, const int slot) {
    keyCaptureActive = true;
    keyCaptureLane = lane;
    keyCaptureSlot = slot;
    if (keyCaptureBackdrop != nullptr) {
        keyCaptureBackdrop->SetColor(SDL_Color{0, 0, 0, 200});
    }
    if (keyCapturePrompt != nullptr) {
        keyCapturePrompt->SetText("Press a key... (ESC to cancel)");
    }
}

void OptionsOverlayScene::EndKeyRebind() {
    keyCaptureActive = false;
    if (keyCaptureBackdrop != nullptr) {
        keyCaptureBackdrop->SetColor(SDL_Color{0, 0, 0, 0});
    }
    if (keyCapturePrompt != nullptr) {
        keyCapturePrompt->SetText("");
    }
}

void OptionsOverlayScene::CompleteKeyRebind(const SDL_Keycode key) {
    if (!keyCaptureActive) {
        return;
    }
    game.SetGameplayLaneKeyBinding(keyCaptureLane, keyCaptureSlot, key);
    EndKeyRebind();
    RefreshInputBindButtonTexts();
}

void OptionsOverlayScene::CancelKeyRebind() {
    if (!keyCaptureActive) {
        return;
    }
    EndKeyRebind();
}

void OptionsOverlayScene::RebuildContent(const Category category) {
    videoResolutionButton = nullptr;
    videoVsyncButton = nullptr;
    videoWindowModeButton = nullptr;
    videoLimitFramerateButton = nullptr;
    videoFramerateLimitValueLabel = nullptr;
    gameplayScrollSpeedValueLabel = nullptr;
    gameplayCrosshairValueLabel = nullptr;
    gameplayAudioOffsetValueLabel = nullptr;
    gameplaySongClockTimingCheckbox = nullptr;
    audioMasterValueLabel = nullptr;
    audioMusicValueLabel = nullptr;
    audioSfxValueLabel = nullptr;
    audioUiValueLabel = nullptr;
    for (auto & inputBindButton : inputBindButtons) {
        for (auto & slot : inputBindButton) {
            slot = nullptr;
        }
    }

    if (!contentContainer || !font) {
        return;
    }

    auto& children = contentContainer->GetChildren();
    children.clear();

    auto titleFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 36.0f);
    auto rowFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 26.0f);
    const auto buttonTextureRes = ResourceManager::getInstance().Get<SDL_Texture>("button.png");
    if (!titleFontRes || !rowFontRes) {
        return;
    }

    const std::shared_ptr<SDL_Texture> buttonTexture = buttonTextureRes ? *buttonTextureRes : nullptr;

    auto* sectionLabel = contentContainer->CreateChild<Label>(
        UnitBounds{{0.05f, 0.05f}, {0.95f, 0.14f}},
        *titleFontRes,
        std::string(CategoryTitle(category)));
    sectionLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Middle);

    if (category == Category::Video) {
        auto addRow = [&](const float yMin, const float yMax, const char* leftText, TextButton*& outButton, std::function<void()> onClick) {
            contentContainer->CreateChild<Label>(
                UnitBounds{{0.05f, yMin}, {0.48f, yMax}},
                *rowFontRes,
                leftText);
            outButton = contentContainer->CreateChild<TextButton>(
                UnitBounds{{0.52f, yMin}, {0.95f, yMax}},
                *rowFontRes,
                "",
                std::move(onClick),
                buttonTexture);
        };

        addRow(
            0.18f,
            0.26f,
            "Resolution",
            videoResolutionButton,
            [this]() {
                game.CycleLogicalResolution();
                RefreshVideoControlTexts();
            });

        addRow(
            0.28f,
            0.36f,
            "VSync",
            videoVsyncButton,
            [this]() {
                game.ToggleVsync();
                RefreshVideoControlTexts();
            });

        addRow(
            0.38f,
            0.46f,
            "Window mode",
            videoWindowModeButton,
            [this]() {
                game.CycleWindowMode();
                RefreshVideoControlTexts();
            });

        contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.48f}, {0.48f, 0.56f}},
            *rowFontRes,
            "Aspect ratio");
        contentContainer->CreateChild<Label>(
            UnitBounds{{0.52f, 0.48f}, {0.95f, 0.56f}},
            *rowFontRes,
            std::string(VideoAspectRatioLabel(game.GetVideoSettings().aspectRatio)) + " (template)");

        const VideoSettings& vs = game.GetVideoSettings();

        addRow(
            0.58f,
            0.66f,
            "Limit framerate",
            videoLimitFramerateButton,
            [this]() {
                const bool cur = game.GetVideoSettings().enableFrameLimiter;
                game.SetFrameLimiterEnabled(!cur);
                RefreshVideoControlTexts();
            });

        contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.68f}, {0.34f, 0.76f}},
            *rowFontRes,
            "Framerate limit");

        videoFramerateLimitValueLabel = contentContainer->CreateChild<Label>(
            UnitBounds{{0.36f, 0.68f}, {0.50f, 0.76f}},
            *rowFontRes,
            "");
        videoFramerateLimitValueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);

        auto* framerateSlider = contentContainer->CreateChild<Slider>(
            UnitBounds{{0.52f, 0.68f}, {0.95f, 0.76f}},
            30.0f,
            1000.0f,
            1.0f,
            static_cast<float>(vs.maxFps));
        framerateSlider->SetOnChanged([this](const float v) {
            game.SetMaxFps(static_cast<int>(std::round(v)));
            RefreshVideoControlTexts();
        });

        auto* noteLabel = contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.78f}, {0.95f, 0.90f}},
            font,
            "Changes apply immediately.");
        noteLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

        auto* resLabel = contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.84f}, {0.95f, 0.96f}},
            font,
            "Refresh the window for resolution changes.");
        resLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

        RefreshVideoControlTexts();
        return;
    }

    if (category == Category::Gameplay) {
        contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.18f}, {0.34f, 0.26f}},
            *rowFontRes,
            "Scroll speed");

        gameplayScrollSpeedValueLabel = contentContainer->CreateChild<Label>(
            UnitBounds{{0.36f, 0.18f}, {0.50f, 0.26f}},
            *rowFontRes,
            "");
        gameplayScrollSpeedValueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);

        const Gameplay::GameplaySettings& gs = game.GetGameplaySettings();
        auto* scrollSlider = contentContainer->CreateChild<Slider>(
            UnitBounds{{0.52f, 0.18f}, {0.95f, 0.26f}},
            Gameplay::kGameplayScrollSpeedMin,
            Gameplay::kGameplayScrollSpeedMax,
            0.1f,
            gs.noteSpeed);
        scrollSlider->SetOnChanged([this](const float v) {
            game.SetNoteSpeed(v);
            RefreshGameplayControlTexts();
        });

        contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.28f}, {0.34f, 0.36f}},
            *rowFontRes,
            "Crosshair size");

        gameplayCrosshairValueLabel = contentContainer->CreateChild<Label>(
            UnitBounds{{0.36f, 0.28f}, {0.50f, 0.36f}},
            *rowFontRes,
            "");
        gameplayCrosshairValueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);

        auto* crosshairSlider = contentContainer->CreateChild<Slider>(
            UnitBounds{{0.52f, 0.28f}, {0.95f, 0.36f}},
            Gameplay::kGameplayCrosshairRadiusMin,
            Gameplay::kGameplayCrosshairRadiusMax,
            1.0f,
            gs.crosshairRadius);
        crosshairSlider->SetOnChanged([this](const float v) {
            game.SetCrosshairRadius(v);
            RefreshGameplayControlTexts();
        });

        contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.38f}, {0.34f, 0.46f}},
            *rowFontRes,
            "Audio offset");

        gameplayAudioOffsetValueLabel = contentContainer->CreateChild<Label>(
            UnitBounds{{0.36f, 0.38f}, {0.50f, 0.46f}},
            *rowFontRes,
            "");
        gameplayAudioOffsetValueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);

        const auto offsetMs = static_cast<float>(gs.audioOffsetSeconds * 1000.0);
        auto* audioSlider = contentContainer->CreateChild<Slider>(
            UnitBounds{{0.52f, 0.38f}, {0.95f, 0.46f}},
            Gameplay::kGameplayAudioOffsetMsMin,
            Gameplay::kGameplayAudioOffsetMsMax,
            0.1f,
            offsetMs);
        audioSlider->SetOnChanged([this](const float v) {
            game.SetAudioOffsetSeconds(static_cast<double>(v) * 1e-3);
            RefreshGameplayControlTexts();
        });

        contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.48f}, {0.70f, 0.56f}},
            *rowFontRes,
            "Use song clock for timing");

        gameplaySongClockTimingCheckbox = contentContainer->CreateChild<TextButton>(
            UnitBounds{{0.72f, 0.48f}, {0.82f, 0.56f}},
            *rowFontRes,
            gs.useWallClockForJudgementTiming ? "" : "X",
            [this]() {
                const bool cur = game.GetGameplaySettings().useWallClockForJudgementTiming;
                game.SetUseWallClockForJudgementTiming(!cur);
                RefreshGameplayControlTexts();
            },
            buttonTexture);

        auto* gameplayNote = contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.62f}, {0.95f, 0.72f}},
            font,
            "Changes apply immediately.");
        gameplayNote->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

        RefreshGameplayControlTexts();
        return;
    }

    if (category == Category::Audio) {
        constexpr float kVolMin = 0.0f;
        constexpr float kVolMax = 1.0f;
        constexpr float kVolStep = 0.01f;

        const auto&[masterVolume, musicVolume, sfxVolume, uiVolume] = game.GetAudioSettings();

        contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.16f}, {0.34f, 0.24f}},
            *rowFontRes,
            "Master volume");

        audioMasterValueLabel = contentContainer->CreateChild<Label>(
            UnitBounds{{0.36f, 0.16f}, {0.50f, 0.24f}},
            *rowFontRes,
            "");
        audioMasterValueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);

        auto* masterSlider = contentContainer->CreateChild<Slider>(
            UnitBounds{{0.52f, 0.16f}, {0.95f, 0.24f}},
            kVolMin,
            kVolMax,
            kVolStep,
            masterVolume);
        masterSlider->SetOnChanged([this](const float v) {
            game.SetMasterVolume(v);
            RefreshAudioControlTexts();
        });

        contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.26f}, {0.34f, 0.34f}},
            *rowFontRes,
            "Music");

        audioMusicValueLabel = contentContainer->CreateChild<Label>(
            UnitBounds{{0.36f, 0.26f}, {0.50f, 0.34f}},
            *rowFontRes,
            "");
        audioMusicValueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);

        auto* musicSlider = contentContainer->CreateChild<Slider>(
            UnitBounds{{0.52f, 0.26f}, {0.95f, 0.34f}},
            kVolMin,
            kVolMax,
            kVolStep,
            musicVolume);
        musicSlider->SetOnChanged([this](const float v) {
            game.SetMusicVolume(v);
            RefreshAudioControlTexts();
        });

        contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.36f}, {0.34f, 0.44f}},
            *rowFontRes,
            "Sound effects");

        audioSfxValueLabel = contentContainer->CreateChild<Label>(
            UnitBounds{{0.36f, 0.36f}, {0.50f, 0.44f}},
            *rowFontRes,
            "");
        audioSfxValueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);

        auto* sfxSlider = contentContainer->CreateChild<Slider>(
            UnitBounds{{0.52f, 0.36f}, {0.95f, 0.44f}},
            kVolMin,
            kVolMax,
            kVolStep,
            sfxVolume);
        sfxSlider->SetOnChanged([this](const float v) {
            game.SetSfxVolume(v);
            RefreshAudioControlTexts();
        });

        contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.46f}, {0.34f, 0.54f}},
            *rowFontRes,
            "UI sounds");

        audioUiValueLabel = contentContainer->CreateChild<Label>(
            UnitBounds{{0.36f, 0.46f}, {0.50f, 0.54f}},
            *rowFontRes,
            "");
        audioUiValueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);

        auto* uiSlider = contentContainer->CreateChild<Slider>(
            UnitBounds{{0.52f, 0.46f}, {0.95f, 0.54f}},
            kVolMin,
            kVolMax,
            kVolStep,
            uiVolume);
        uiSlider->SetOnChanged([this](const float v) {
            game.SetUiVolume(v);
            RefreshAudioControlTexts();
        });

        auto* audioNote = contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.58f}, {0.95f, 0.68f}},
            font,
            "Changes apply immediately.");
        audioNote->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

        RefreshAudioControlTexts();
        return;
    }

    // Ignore the "Condition is always true" warning, it isn't.
    if (category == Category::Input) {
        const Gameplay::GameplaySettings& gs = game.GetGameplaySettings();

        for (int lane = 0; lane < Gameplay::kLaneBindingCount; ++lane) {
            constexpr std::array<const char*, Gameplay::kLaneBindingCount> kLaneNames = {
                "Left",
                "Up",
                "Right",
                "Down",
            };
            const float yMin = 0.14f + static_cast<float>(lane) * 0.1f;
            const float yMax = yMin + 0.08f;

            contentContainer->CreateChild<Label>(
                UnitBounds{{0.05f, yMin}, {0.18f, yMax}},
                *rowFontRes,
                kLaneNames[static_cast<size_t>(lane)]);

            for (int slot = 0; slot < Gameplay::kKeysPerLane; ++slot) {
                const float xMin = 0.20f + static_cast<float>(slot) * 0.22f;
                const float xMax = xMin + 0.20f;

                inputBindButtons[lane][slot] = contentContainer->CreateChild<TextButton>(
                    UnitBounds{{xMin, yMin}, {xMax, yMax}},
                    *rowFontRes,
                    KeyLabel(gs.keyBindings[static_cast<size_t>(lane)][static_cast<size_t>(slot)]),
                    [this, lane, slot]() {
                        BeginKeyRebind(lane, slot);
                    },
                    buttonTexture);
            }
        }

        auto* inputNote = contentContainer->CreateChild<Label>(
            UnitBounds{{0.05f, 0.58f}, {0.95f, 0.68f}},
            font,
            "Changes apply immediately.");
        inputNote->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

        RefreshInputBindButtonTexts();
    }
}

} // namespace Game
