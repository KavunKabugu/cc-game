#ifndef CC_GAME_MAIN_MENU_SCENE_H
#define CC_GAME_MAIN_MENU_SCENE_H

#include "Game/Scene/SceneBase.h"

namespace Game {

class GameInstance;
class SceneManager;

class MainMenuScene final : public SceneBase {
public:
    MainMenuScene(SceneManager& sceneManager, GameInstance& gameInstance, int selectedSongIndex);

private:
    SceneManager& sceneManager;
    GameInstance& game;
    int selectedSongIndex = 0;
};

} // namespace Game

#endif // CC_GAME_MAIN_MENU_SCENE_H
