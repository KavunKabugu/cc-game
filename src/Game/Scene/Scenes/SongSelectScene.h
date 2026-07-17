#ifndef CC_GAME_SONG_SELECT_SCENE_H
#define CC_GAME_SONG_SELECT_SCENE_H

#include <memory>
#include <vector>

#include <SDL3_ttf/SDL_ttf.h>

#include "Game/Scene/SceneBase.h"
#include "Game/Song/SongTypes.h"

namespace Game {

class GameInstance;
class SceneManager;
class ScrollContainer;
class SelectableRow;
class Sprite;
class TextButton;

class SongSelectScene final : public SceneBase {
public:
    SongSelectScene(SceneManager& sceneManager, GameInstance& gameInstance, const std::string& errorMessage = "");

private:
    enum class LeftPanelMode {
        Charts,
        Scores
    };

    void BuildSongList();
    void BuildLeftPanel();
    void BuildChartList();
    void BuildScoreList();
    void SelectSong(int index);
    void CycleLeftPanelMode();
    void UpdateLeftPanelTabLabel() const;
    void UpdateSongSelectionVisuals() const;
    void UpdateChartSelectionVisuals() const;
    void UpdateScoreSelectionVisuals() const;
    void UpdateCoverBackground() const;

    SceneManager& sceneManager;
    GameInstance& game;
    std::vector<std::shared_ptr<Song::SongMetadata>> songs;

    Sprite* coverSprite = nullptr;
    ScrollContainer* leftScroll = nullptr;
    ScrollContainer* songScroll = nullptr;
    TextButton* leftPanelTabButton = nullptr;

    std::vector<SelectableRow*> songRows;
    std::vector<SelectableRow*> chartRows;
    std::vector<SelectableRow*> scoreRows;

    std::shared_ptr<TTF_Font> titleRowFont;
    std::shared_ptr<TTF_Font> bodyRowFont;
    std::shared_ptr<TTF_Font> metaRowFont;

    LeftPanelMode leftPanelMode = LeftPanelMode::Charts;
    int selectedSongIndex = -1;
    int selectedDifficultyIndex = -1;
    int selectedScoreIndex = -1;
};

} // namespace Game

#endif // CC_GAME_SONG_SELECT_SCENE_H
