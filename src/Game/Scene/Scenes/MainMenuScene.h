#ifndef CC_GAME_MAIN_MENU_SCENE_H
#define CC_GAME_MAIN_MENU_SCENE_H

#include "Game/Scene/SceneBase.h"

namespace Game {

class GameInstance;
class SceneManager;

class MainMenuScene final : public SceneBase {
public:
    MainMenuScene(SceneManager& sceneManager, GameInstance& gameInstance);

private:
    SceneManager& sceneManager;
    GameInstance& game;
};

} // namespace Game

#endif // CC_GAME_MAIN_MENU_SCENE_H
