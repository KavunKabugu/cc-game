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
class TextButton;

class SongSelectScene final : public SceneBase {
public:
    SongSelectScene(SceneManager& sceneManager, GameInstance& gameInstance);

private:
    void BuildSongList();
    void BuildChartList();
    void SelectSong(int index);
    void UpdateSongSelectionVisuals() const;
    void UpdateChartSelectionVisuals() const;

    SceneManager& sceneManager;
    GameInstance& game;
    std::vector<std::shared_ptr<Song::SongMetadata>> songs;
    ScrollContainer* chartScroll = nullptr;
    ScrollContainer* songScroll = nullptr;
    std::vector<TextButton*> songButtons;
    std::vector<TextButton*> chartButtons;
    std::shared_ptr<TTF_Font> rowFont;
    int selectedSongIndex = -1;
    int selectedDifficultyIndex = -1;
};

} // namespace Game

#endif // CC_GAME_SONG_SELECT_SCENE_H
