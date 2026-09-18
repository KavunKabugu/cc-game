#ifndef CC_GAME_OPTIONS_OVERLAY_SCENE_H
#define CC_GAME_OPTIONS_OVERLAY_SCENE_H

#include <memory>

#include <SDL3/SDL_keycode.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "Game/Scene/SceneBase.h"

namespace Game {

class SceneManager;
class ScrollContainer;
class Label;
class TextButton;
class GameInstance;
class PanelRect;
class InputField;

class OptionsOverlayScene final : public SceneBase {
public:
    OptionsOverlayScene(SceneManager& sceneManager, GameInstance& gameInstance);

    [[nodiscard]] bool BlocksLowerInput() const override { return true; }
    [[nodiscard]] bool BlocksLowerRendering() const override { return false; }

    enum class Category {
        Video,
        Audio,
        Input,
        Gameplay
    };

private:
    enum class KeyBind {
        Lane1Slot1,
        Lane2Slot1,
        Lane3Slot1,
        Lane4Slot1,
        Lane1Slot2,
        Lane2Slot2,
        Lane3Slot2,
        Lane4Slot2,
        Restart
    };

    void RebuildContent(Category category);
    void RefreshVideoControlTexts() const;
    void RefreshGameplayControlTexts() const;
    void RefreshAudioControlTexts() const;
    void RefreshInputBindButtonTexts() const;
    void CloseOverlay();

    void BeginKeyRebind(KeyBind keyBind);
    void EndKeyRebind();
    void CompleteKeyRebind(SDL_Keycode key);
    void CancelKeyRebind();

    SceneManager& sceneManager;
    GameInstance& game;

    int overlayOpenedLogicalW = 0;
    int overlayOpenedLogicalH = 0;

    Category currentCategory = Category::Video;
    ScrollContainer* contentContainer = nullptr;
    std::shared_ptr<TTF_Font> font;

    TextButton* videoResolutionButton = nullptr;
    TextButton* videoVsyncButton = nullptr;
    TextButton* videoWindowModeButton = nullptr;
    TextButton* videoLimitFramerateButton = nullptr;
    Label* videoFramerateLimitValueLabel = nullptr;

    Label* gameplayScrollSpeedValueLabel = nullptr;
    Label* gameplayCrosshairValueLabel = nullptr;
    Label* gameplayAudioOffsetValueLabel = nullptr;
    InputField* gameplayPlayerNameField = nullptr;
    TextButton* gameplaySongClockTimingCheckbox = nullptr;
    TextButton* gameplayEnableBackgroundCheckbox = nullptr;
    Label* gameplayBackgroundOpacityValueLabel = nullptr;
    Label* gameplayBackgroundColorRValueLabel = nullptr;
    Label* gameplayBackgroundColorGValueLabel = nullptr;
    Label* gameplayBackgroundColorBValueLabel = nullptr;
    PanelRect* gameplayBackgroundColorPreview = nullptr;
    TextButton* gameplayEnablePlayfieldBorderCheckbox = nullptr;
    TextButton* gameplaySwapUpDownLanesCheckbox = nullptr;
    Label* gameplayPlayfieldBorderOpacityValueLabel = nullptr;
    Label* gameplayPlayfieldBorderSizeValueLabel = nullptr;

    Label* audioMasterValueLabel = nullptr;
    Label* audioMusicValueLabel = nullptr;
    Label* audioSfxValueLabel = nullptr;
    Label* audioUiValueLabel = nullptr;

    TextButton* inputBindButtons[4][2]{};
    TextButton* restartBindButton{};
    bool keyCaptureActive = false;
    KeyBind keyCaptureKeyBind;
    PanelRect* keyCaptureBackdrop = nullptr;
    Label* keyCapturePrompt = nullptr;
};

} // namespace Game

#endif // CC_GAME_OPTIONS_OVERLAY_SCENE_H
