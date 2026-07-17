#include "OptionsOverlayScene.h"

#include <array>
#include <cmath>
#include <format>
#include <functional>
#include <memory>
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
#include "Game/Layout/VBoxLayout.h"
#include "Game/Layout/ViewportMatchLayout.h"
#include "Game/objects/Container.h"
#include "Game/objects/Label.h"
#include "Game/objects/PanelRect.h"
#include "Game/objects/ScrollContainer.h"
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

class KeyBindingCaptureGate final : public GameObject, public IKeyHandler, public IMouseClickable,
                                    public IMouseScrollable {
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

    IKeyHandler* AsKeyHandler() override { return capturing && capturing() ? this : nullptr; }
    IMouseClickable* AsMouseClickable() override { return capturing && capturing() ? this : nullptr; }
    IMouseScrollable* AsMouseScrollable() override { return capturing && capturing() ? this : nullptr; }

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

    root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}},
                                 SDL_Color{.r = 0, .g = 0, .b = 0, .a = 150});

    auto overlayTextureRes = ResourceManager::getInstance().Get<SDL_Texture>("overlay_bg.png");
    const auto buttonTextureRes = ResourceManager::getInstance().Get<SDL_Texture>("button.png");
    auto titleFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 48.0f);
    auto sectionFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 34.0f);
    const auto bodyFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 28.0f);
    if (!titleFontRes || !sectionFontRes || !bodyFontRes) {
        return;
    }
    font = *bodyFontRes;

    root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.12f, .y = 0.1f}, .max = {.x = 0.88f, .y = 0.9f}},
                                 SDL_Color{.r = 22, .g = 24, .b = 34, .a = 235});
    if (overlayTextureRes) {
        root->CreateChild<Sprite>(UnitBounds{.min = {.x = 0.12f, .y = 0.1f}, .max = {.x = 0.88f, .y = 0.9f}},
                                  *overlayTextureRes);
    }

    auto* title = root->CreateChild<Label>(
        UnitBounds{.min = {.x = 0.16f, .y = 0.13f}, .max = {.x = 0.84f, .y = 0.22f}},
        *titleFontRes,
        "Options");
    title->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);

    root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.16f, .y = 0.24f}, .max = {.x = 0.35f, .y = 0.84f}},
                                 SDL_Color{.r = 32, .g = 36, .b = 53, .a = 235});
    root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.37f, .y = 0.24f}, .max = {.x = 0.84f, .y = 0.84f}},
                                 SDL_Color{.r = 28, .g = 32, .b = 46, .a = 235});

    const std::shared_ptr<SDL_Texture> buttonTexture = buttonTextureRes ? *buttonTextureRes : nullptr;

    root->CreateChild<TextButton>(
        UnitBounds{.min = {.x = 0.17f, .y = 0.27f}, .max = {.x = 0.34f, .y = 0.35f}},
        *sectionFontRes,
        "Video",
        [this] {
            currentCategory = Category::Video;
            RebuildContent(currentCategory);
        },
        buttonTexture);
    root->CreateChild<TextButton>(
        UnitBounds{.min = {.x = 0.17f, .y = 0.37f}, .max = {.x = 0.34f, .y = 0.45f}},
        *sectionFontRes,
        "Audio",
        [this] {
            currentCategory = Category::Audio;
            RebuildContent(currentCategory);
        },
        buttonTexture);
    root->CreateChild<TextButton>(
        UnitBounds{.min = {.x = 0.17f, .y = 0.47f}, .max = {.x = 0.34f, .y = 0.55f}},
        *sectionFontRes,
        "Input",
        [this] {
            currentCategory = Category::Input;
            RebuildContent(currentCategory);
        },
        buttonTexture);
    root->CreateChild<TextButton>(
        UnitBounds{.min = {.x = 0.17f, .y = 0.57f}, .max = {.x = 0.34f, .y = 0.65f}},
        *sectionFontRes,
        "Gameplay",
        [this] {
            currentCategory = Category::Gameplay;
            RebuildContent(currentCategory);
        },
        buttonTexture);

    root->CreateChild<TextButton>(
        UnitBounds{.min = {.x = 0.77f, .y = 0.13f}, .max = {.x = 0.84f, .y = 0.2f}},
        *sectionFontRes,
        "X",
        [this] {
            CloseOverlay();
        },
        buttonTexture);

    contentContainer = root->CreateChild<ScrollContainer>(UnitBounds{
            .min = {.x = 0.39f, .y = 0.26f}, .max = {.x = 0.82f, .y = 0.82f}
        });
    RebuildContent(currentCategory);

    keyCaptureBackdrop =
        root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}},
                                     SDL_Color{.r = 0, .g = 0, .b = 0, .a = 0});
    keyCapturePrompt = root->CreateChild<Label>(
        UnitBounds{.min = {.x = 0.1f, .y = 0.42f}, .max = {.x = 0.9f, .y = 0.5f}},
        *bodyFontRes,
        "");
    keyCapturePrompt->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);

    root->CreateChild<KeyBindingCaptureGate>(
        UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}},
        [this] {
            return keyCaptureActive;
        },
        [this](const SDL_Keycode key) {
            CompleteKeyRebind(key);
        },
        [this] {
            CancelKeyRebind();
        });

    root->CreateChild<OverlayCloseHandler>(
        UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 0.0f, .y = 0.0f}},
        [this] {
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
    if (gameplayEnableBackgroundCheckbox) {
        gameplayEnableBackgroundCheckbox->SetText(gs.enableBackgroundImage ? "X" : "");
    }
    if (gameplayBackgroundOpacityValueLabel) {
        gameplayBackgroundOpacityValueLabel->SetText(
            std::format("{:.0f}%", gs.backgroundOpacity * 100.0f));
    }
    if (gameplayBackgroundColorRValueLabel) {
        gameplayBackgroundColorRValueLabel->SetText(std::to_string(gs.backgroundColorR));
    }
    if (gameplayBackgroundColorGValueLabel) {
        gameplayBackgroundColorGValueLabel->SetText(std::to_string(gs.backgroundColorG));
    }
    if (gameplayBackgroundColorBValueLabel) {
        gameplayBackgroundColorBValueLabel->SetText(std::to_string(gs.backgroundColorB));
    }
    if (gameplayBackgroundColorPreview) {
        gameplayBackgroundColorPreview->SetColor(SDL_Color{
            .r = gs.backgroundColorR,
            .g = gs.backgroundColorG,
            .b = gs.backgroundColorB,
            .a = 255});
    }
    if (gameplayEnablePlayfieldBorderCheckbox) {
        gameplayEnablePlayfieldBorderCheckbox->SetText(gs.enablePlayfieldBorder ? "X" : "");
    }
    if (gameplaySwapUpDownLanesCheckbox) {
        gameplaySwapUpDownLanesCheckbox->SetText(gs.swapUpDownLanes ? "X" : "");
    }
    if (gameplayPlayfieldBorderOpacityValueLabel) {
        gameplayPlayfieldBorderOpacityValueLabel->SetText(
            std::format("{:.0f}%", gs.playfieldBorderOpacity * 100.0f));
    }
    if (gameplayPlayfieldBorderSizeValueLabel) {
        gameplayPlayfieldBorderSizeValueLabel->SetText(
            std::format("{:.0f}", gs.playfieldBorderSize));
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
        keyCaptureBackdrop->SetColor(SDL_Color{.r = 0, .g = 0, .b = 0, .a = 200});
    }
    if (keyCapturePrompt != nullptr) {
        keyCapturePrompt->SetText("Press a key... (ESC to cancel)");
    }
}

void OptionsOverlayScene::EndKeyRebind() {
    keyCaptureActive = false;
    if (keyCaptureBackdrop != nullptr) {
        keyCaptureBackdrop->SetColor(SDL_Color{.r = 0, .g = 0, .b = 0, .a = 0});
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
    gameplayEnableBackgroundCheckbox = nullptr;
    gameplayBackgroundOpacityValueLabel = nullptr;
    gameplayBackgroundColorRValueLabel = nullptr;
    gameplayBackgroundColorGValueLabel = nullptr;
    gameplayBackgroundColorBValueLabel = nullptr;
    gameplayBackgroundColorPreview = nullptr;
    gameplayEnablePlayfieldBorderCheckbox = nullptr;
    gameplaySwapUpDownLanesCheckbox = nullptr;
    gameplayPlayfieldBorderOpacityValueLabel = nullptr;
    gameplayPlayfieldBorderSizeValueLabel = nullptr;
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

    const std::shared_ptr<TTF_Font>& titleFont = *titleFontRes;
    const std::shared_ptr<TTF_Font>& rowFont = *rowFontRes;
    const std::shared_ptr<SDL_Texture> buttonTexture = buttonTextureRes ? *buttonTextureRes : nullptr;

    if (category == Category::Gameplay) {
        contentContainer->SetLayout(std::make_unique<Layout::VBoxLayout>(6.0f, 8.0f, 52.0f));

        auto makeRow = [this]() -> Container* {
            return contentContainer->CreateChild<Container>(UnitBounds{
                    .min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}
                });
        };

        {
            auto* titleRow = makeRow();
            auto* sectionLabel = titleRow->CreateChild<Label>(
                UnitBounds{.min = {.x = 0.02f, .y = 0.0f}, .max = {.x = 0.98f, .y = 1.0f}},
                titleFont,
                std::string(CategoryTitle(category)));
            sectionLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Middle);
        }

        const Gameplay::GameplaySettings& gs = game.GetGameplaySettings();

        auto addSliderRow = [&](
            const char* labelText,
            Label*& valueLabel,
            const float minV,
            const float maxV,
            const float stepV,
            const float initialV,
            std::function<void(float)> onChanged,
            const float sliderMaxX = 0.98f) {
            auto* row = makeRow();
            row->CreateChild<Label>(
                UnitBounds{.min = {.x = 0.02f, .y = 0.1f}, .max = {.x = 0.34f, .y = 0.9f}},
                rowFont,
                labelText);
            valueLabel = row->CreateChild<Label>(
                UnitBounds{.min = {.x = 0.34f, .y = 0.1f}, .max = {.x = 0.48f, .y = 0.9f}},
                rowFont,
                "");
            valueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);
            auto* slider = row->CreateChild<Slider>(
                UnitBounds{.min = {.x = 0.50f, .y = 0.2f}, .max = {.x = sliderMaxX, .y = 0.8f}},
                minV,
                maxV,
                stepV,
                initialV);
            slider->SetOnChanged(std::move(onChanged));
            return row;
        };

        auto addCheckboxRow = [&](const char* labelText, TextButton*& outCheckbox, const bool checked,
                                  std::function<void()> onClick) {
            auto* row = makeRow();
            row->CreateChild<Label>(
                UnitBounds{.min = {.x = 0.02f, .y = 0.1f}, .max = {.x = 0.78f, .y = 0.9f}},
                rowFont,
                labelText);
            outCheckbox = row->CreateChild<TextButton>(
                UnitBounds{.min = {.x = 0.82f, .y = 0.15f}, .max = {.x = 0.94f, .y = 0.85f}},
                rowFont,
                checked ? "X" : "",
                std::move(onClick),
                buttonTexture);
        };

        addSliderRow(
            "Scroll speed",
            gameplayScrollSpeedValueLabel,
            Gameplay::kGameplayScrollSpeedMin,
            Gameplay::kGameplayScrollSpeedMax,
            0.1f,
            gs.noteSpeed,
            [this](const float v) {
                game.SetNoteSpeed(v);
                RefreshGameplayControlTexts();
            });

        addSliderRow(
            "Crosshair size",
            gameplayCrosshairValueLabel,
            Gameplay::kGameplayCrosshairRadiusMin,
            Gameplay::kGameplayCrosshairRadiusMax,
            1.0f,
            gs.crosshairRadius,
            [this](const float v) {
                game.SetCrosshairRadius(v);
                RefreshGameplayControlTexts();
            });

        addSliderRow(
            "Audio offset",
            gameplayAudioOffsetValueLabel,
            Gameplay::kGameplayAudioOffsetMsMin,
            Gameplay::kGameplayAudioOffsetMsMax,
            0.1f,
            static_cast<float>(gs.audioOffsetSeconds * 1000.0),
            [this](const float v) {
                game.SetAudioOffsetSeconds(static_cast<double>(v) * 1e-3);
                RefreshGameplayControlTexts();
            });

        addCheckboxRow(
            "Use song clock for timing",
            gameplaySongClockTimingCheckbox,
            !gs.useWallClockForJudgementTiming,
            [this] {
                const bool cur = game.GetGameplaySettings().useWallClockForJudgementTiming;
                game.SetUseWallClockForJudgementTiming(!cur);
                RefreshGameplayControlTexts();
            });

        addCheckboxRow(
            "Enable background",
            gameplayEnableBackgroundCheckbox,
            gs.enableBackgroundImage,
            [this] {
                const bool cur = game.GetGameplaySettings().enableBackgroundImage;
                game.SetEnableBackgroundImage(!cur);
                RefreshGameplayControlTexts();
            });

        addSliderRow(
            "Background opacity",
            gameplayBackgroundOpacityValueLabel,
            Gameplay::kGameplayOpacityMin,
            Gameplay::kGameplayOpacityMax,
            0.01f,
            gs.backgroundOpacity,
            [this](const float v) {
                game.SetBackgroundOpacity(v);
                RefreshGameplayControlTexts();
            });

        {
            auto* row = addSliderRow(
                "Background R",
                gameplayBackgroundColorRValueLabel,
                0.0f,
                255.0f,
                1.0f,
                gs.backgroundColorR,
                [this](const float v) {
                    game.SetBackgroundColorR(static_cast<unsigned char>(std::lround(v)));
                    RefreshGameplayControlTexts();
                },
                0.82f);
            gameplayBackgroundColorPreview = row->CreateChild<PanelRect>(
                UnitBounds{.min = {.x = 0.86f, .y = 0.15f}, .max = {.x = 0.98f, .y = 0.85f}},
                SDL_Color{.r = gs.backgroundColorR, .g = gs.backgroundColorG, .b = gs.backgroundColorB, .a = 255});
        }

        addSliderRow(
            "Background G",
            gameplayBackgroundColorGValueLabel,
            0.0f,
            255.0f,
            1.0f,
            gs.backgroundColorG,
            [this](const float v) {
                game.SetBackgroundColorG(static_cast<unsigned char>(std::lround(v)));
                RefreshGameplayControlTexts();
            },
            0.82f);

        addSliderRow(
            "Background B",
            gameplayBackgroundColorBValueLabel,
            0.0f,
            255.0f,
            1.0f,
            gs.backgroundColorB,
            [this](const float v) {
                game.SetBackgroundColorB(static_cast<unsigned char>(std::lround(v)));
                RefreshGameplayControlTexts();
            },
            0.82f);

        addCheckboxRow(
            "Enable playfield border",
            gameplayEnablePlayfieldBorderCheckbox,
            gs.enablePlayfieldBorder,
            [this] {
                const bool cur = game.GetGameplaySettings().enablePlayfieldBorder;
                game.SetEnablePlayfieldBorder(!cur);
                RefreshGameplayControlTexts();
            });

        addSliderRow(
            "Border opacity",
            gameplayPlayfieldBorderOpacityValueLabel,
            Gameplay::kGameplayOpacityMin,
            Gameplay::kGameplayOpacityMax,
            0.01f,
            gs.playfieldBorderOpacity,
            [this](const float v) {
                game.SetPlayfieldBorderOpacity(v);
                RefreshGameplayControlTexts();
            });

        addSliderRow(
            "Border size",
            gameplayPlayfieldBorderSizeValueLabel,
            Gameplay::kGameplayBorderSizeMin,
            Gameplay::kGameplayBorderSizeMax,
            1.0f,
            gs.playfieldBorderSize,
            [this](const float v) {
                game.SetPlayfieldBorderSize(v);
                RefreshGameplayControlTexts();
            });

        addCheckboxRow(
            "Swap Up/Down lanes",
            gameplaySwapUpDownLanesCheckbox,
            gs.swapUpDownLanes,
            [this] {
                const bool cur = game.GetGameplaySettings().swapUpDownLanes;
                game.SetSwapUpDownLanes(!cur);
                RefreshGameplayControlTexts();
            });

        {
            auto* noteRow = makeRow();
            auto* gameplayNote = noteRow->CreateChild<Label>(
                UnitBounds{.min = {.x = 0.02f, .y = 0.1f}, .max = {.x = 0.98f, .y = 0.9f}},
                font,
                "Changes apply on the next chart.");
            gameplayNote->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Middle);
        }

        contentContainer->UpdateLayout();
        RefreshGameplayControlTexts();
        return;
    }

    contentContainer->SetLayout(std::make_unique<Layout::ViewportMatchLayout>());

    auto* sectionLabel = contentContainer->CreateChild<Label>(
        UnitBounds{.min = {.x = 0.05f, .y = 0.05f}, .max = {.x = 0.95f, .y = 0.14f}},
        titleFont,
        std::string(CategoryTitle(category)));
    sectionLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Middle);

    if (category == Category::Video) {
        auto addRow = [&](const float yMin, const float yMax, const char* leftText, TextButton*& outButton, std::function<void()> onClick) {
            contentContainer->CreateChild<Label>(
                UnitBounds{.min = {.x = 0.05f, .y = yMin}, .max = {.x = 0.48f, .y = yMax}},
                rowFont,
                leftText);
            outButton = contentContainer->CreateChild<TextButton>(
                UnitBounds{.min = {.x = 0.52f, .y = yMin}, .max = {.x = 0.95f, .y = yMax}},
                rowFont,
                "",
                std::move(onClick),
                buttonTexture);
        };

        addRow(
            0.18f,
            0.26f,
            "Resolution",
            videoResolutionButton,
            [this] {
                game.CycleLogicalResolution();
                RefreshVideoControlTexts();
            });

        addRow(
            0.28f,
            0.36f,
            "VSync",
            videoVsyncButton,
            [this] {
                game.ToggleVsync();
                RefreshVideoControlTexts();
            });

        addRow(
            0.38f,
            0.46f,
            "Window mode",
            videoWindowModeButton,
            [this] {
                game.CycleWindowMode();
                RefreshVideoControlTexts();
            });

        contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.05f, .y = 0.48f}, .max = {.x = 0.48f, .y = 0.56f}},
            rowFont,
            "Aspect ratio");
        contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.52f, .y = 0.48f}, .max = {.x = 0.95f, .y = 0.56f}},
            rowFont,
            std::string(VideoAspectRatioLabel(game.GetVideoSettings().aspectRatio)) + " (template)");

        const VideoSettings& vs = game.GetVideoSettings();

        addRow(
            0.58f,
            0.66f,
            "Limit framerate",
            videoLimitFramerateButton,
            [this] {
                const bool cur = game.GetVideoSettings().enableFrameLimiter;
                game.SetFrameLimiterEnabled(!cur);
                RefreshVideoControlTexts();
            });

        contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.05f, .y = 0.68f}, .max = {.x = 0.34f, .y = 0.76f}},
            rowFont,
            "Framerate limit");

        videoFramerateLimitValueLabel = contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.36f, .y = 0.68f}, .max = {.x = 0.50f, .y = 0.76f}},
            rowFont,
            "");
        videoFramerateLimitValueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);

        auto* framerateSlider = contentContainer->CreateChild<Slider>(
            UnitBounds{.min = {.x = 0.52f, .y = 0.68f}, .max = {.x = 0.95f, .y = 0.76f}},
            30.0f,
            1000.0f,
            1.0f,
            static_cast<float>(vs.maxFps));
        framerateSlider->SetOnChanged([this](const float v) {
            game.SetMaxFps(static_cast<int>(std::round(v)));
            RefreshVideoControlTexts();
        });

        auto* noteLabel = contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.05f, .y = 0.78f}, .max = {.x = 0.95f, .y = 0.90f}},
            font,
            "Changes apply immediately.");
        noteLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

        auto* resLabel = contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.05f, .y = 0.84f}, .max = {.x = 0.95f, .y = 0.96f}},
            font,
            "Refresh the window for resolution changes.");
        resLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

        contentContainer->UpdateLayout();
        RefreshVideoControlTexts();
        return;
    }

    if (category == Category::Audio) {
        constexpr float kVolMin = 0.0f;
        constexpr float kVolMax = 1.0f;
        constexpr float kVolStep = 0.01f;

        const auto&[masterVolume, musicVolume, sfxVolume, uiVolume] = game.GetAudioSettings();

        contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.05f, .y = 0.16f}, .max = {.x = 0.34f, .y = 0.24f}},
            rowFont,
            "Master volume");

        audioMasterValueLabel = contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.36f, .y = 0.16f}, .max = {.x = 0.50f, .y = 0.24f}},
            rowFont,
            "");
        audioMasterValueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);

        auto* masterSlider = contentContainer->CreateChild<Slider>(
            UnitBounds{.min = {.x = 0.52f, .y = 0.16f}, .max = {.x = 0.95f, .y = 0.24f}},
            kVolMin,
            kVolMax,
            kVolStep,
            masterVolume);
        masterSlider->SetOnChanged([this](const float v) {
            game.SetMasterVolume(v);
            RefreshAudioControlTexts();
        });

        contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.05f, .y = 0.26f}, .max = {.x = 0.34f, .y = 0.34f}},
            rowFont,
            "Music");

        audioMusicValueLabel = contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.36f, .y = 0.26f}, .max = {.x = 0.50f, .y = 0.34f}},
            rowFont,
            "");
        audioMusicValueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);

        auto* musicSlider = contentContainer->CreateChild<Slider>(
            UnitBounds{.min = {.x = 0.52f, .y = 0.26f}, .max = {.x = 0.95f, .y = 0.34f}},
            kVolMin,
            kVolMax,
            kVolStep,
            musicVolume);
        musicSlider->SetOnChanged([this](const float v) {
            game.SetMusicVolume(v);
            RefreshAudioControlTexts();
        });

        contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.05f, .y = 0.36f}, .max = {.x = 0.34f, .y = 0.44f}},
            rowFont,
            "Sound effects");

        audioSfxValueLabel = contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.36f, .y = 0.36f}, .max = {.x = 0.50f, .y = 0.44f}},
            rowFont,
            "");
        audioSfxValueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);

        auto* sfxSlider = contentContainer->CreateChild<Slider>(
            UnitBounds{.min = {.x = 0.52f, .y = 0.36f}, .max = {.x = 0.95f, .y = 0.44f}},
            kVolMin,
            kVolMax,
            kVolStep,
            sfxVolume);
        sfxSlider->SetOnChanged([this](const float v) {
            game.SetSfxVolume(v);
            RefreshAudioControlTexts();
        });

        contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.05f, .y = 0.46f}, .max = {.x = 0.34f, .y = 0.54f}},
            rowFont,
            "UI sounds");

        audioUiValueLabel = contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.36f, .y = 0.46f}, .max = {.x = 0.50f, .y = 0.54f}},
            rowFont,
            "");
        audioUiValueLabel->SetAlignment(HorizontalAlignment::Right, VerticalAlignment::Middle);

        auto* uiSlider = contentContainer->CreateChild<Slider>(
            UnitBounds{.min = {.x = 0.52f, .y = 0.46f}, .max = {.x = 0.95f, .y = 0.54f}},
            kVolMin,
            kVolMax,
            kVolStep,
            uiVolume);
        uiSlider->SetOnChanged([this](const float v) {
            game.SetUiVolume(v);
            RefreshAudioControlTexts();
        });

        auto* audioNote = contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.05f, .y = 0.58f}, .max = {.x = 0.95f, .y = 0.68f}},
            font,
            "Changes apply immediately.");
        audioNote->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

        contentContainer->UpdateLayout();
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
                UnitBounds{.min = {.x = 0.05f, .y = yMin}, .max = {.x = 0.18f, .y = yMax}},
                rowFont,
                kLaneNames[static_cast<size_t>(lane)]);

            for (int slot = 0; slot < Gameplay::kKeysPerLane; ++slot) {
                const float xMin = 0.20f + static_cast<float>(slot) * 0.22f;
                const float xMax = xMin + 0.20f;

                inputBindButtons[lane][slot] = contentContainer->CreateChild<TextButton>(
                    UnitBounds{.min = {.x = xMin, .y = yMin}, .max = {.x = xMax, .y = yMax}},
                    rowFont,
                    KeyLabel(gs.keyBindings[static_cast<size_t>(lane)][static_cast<size_t>(slot)]),
                    [this, lane, slot] {
                        BeginKeyRebind(lane, slot);
                    },
                    buttonTexture);
            }
        }

        auto* inputNote = contentContainer->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.05f, .y = 0.58f}, .max = {.x = 0.95f, .y = 0.68f}},
            font,
            "Changes apply immediately.");
        inputNote->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

        contentContainer->UpdateLayout();
        RefreshInputBindButtonTexts();
    }
}

} // namespace Game
