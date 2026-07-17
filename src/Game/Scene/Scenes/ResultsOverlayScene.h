#ifndef CC_GAME_RESULTS_OVERLAY_SCENE_H
#define CC_GAME_RESULTS_OVERLAY_SCENE_H

#include "Game/Score/ResultsViewData.h"
#include "Game/Scene/SceneBase.h"

namespace Game {

class GameInstance;
class SceneManager;
class Label;
class TextButton;

class ResultsOverlayScene final : public SceneBase {
public:
    enum class Mode {
        Pause,
        Results,
        Browse,
    };

    ResultsOverlayScene(
        SceneManager& sceneManager,
        GameInstance& gameInstance,
        Mode mode,
        Score::ResultsViewData data);

    [[nodiscard]] bool BlocksLowerInput() const override { return true; }
    [[nodiscard]] bool BlocksLowerRendering() const override { return false; }

private:
    void CloseResume() const;
    void CloseToSongSelect() const;
    void CloseBrowse() const;
    void HandleEscape() const;

    SceneManager& sceneManager;
    GameInstance& game;
    Mode mode;
    Score::ResultsViewData data;

    Gameplay::ResultsGraphDisplay* resultsGraphDisplay = nullptr;
    TextButton* graphToggleButton = nullptr;
};

} // namespace Game

#endif // CC_GAME_RESULTS_OVERLAY_SCENE_H
