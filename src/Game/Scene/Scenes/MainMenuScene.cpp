#include "MainMenuScene.h"

#include <functional>

#include "Game/Game.h"
#include "Game/ResourceManager.h"
#include "Game/Scene/SceneManager.h"
#include "Game/Scene/Scenes/OptionsOverlayScene.h"
#include "Game/Scene/Scenes/SongSelectScene.h"
#include "Game/objects/Label.h"
#include "Game/objects/PanelRect.h"
#include "Game/objects/Sprite.h"
#include "Game/objects/TextButton.h"

namespace Game {

MainMenuScene::MainMenuScene(SceneManager& sceneManager, GameInstance& gameInstance)
    : sceneManager(sceneManager),
      game(gameInstance) {
    if (auto bgRes = ResourceManager::getInstance().Get<SDL_Texture>("background.png")) {
        root->CreateChild<Sprite>(UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}}, *bgRes);
    } else {
        root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}},
                                     SDL_Color{.r = 20, .g = 22, .b = 35, .a = 255});
    }

    root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.3f, .y = 0.12f}, .max = {.x = 0.7f, .y = 0.88f}},
                                 SDL_Color{.r = 10, .g = 10, .b = 18, .a = 180});

    const auto buttonTextureRes = ResourceManager::getInstance().Get<SDL_Texture>("button.png");
    std::shared_ptr<SDL_Texture> buttonTexture = buttonTextureRes ? *buttonTextureRes : nullptr;

    auto titleFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 72.0f);
    auto buttonFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 42.0f);
    if (!titleFontRes || !buttonFontRes) {
        return;
    }

    auto* title = root->CreateChild<Label>(
        UnitBounds{.min = {.x = 0.0f, .y = 0.14f}, .max = {.x = 1.0f, .y = 0.28f}},
        *titleFontRes,
        "CC GAME");
    title->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);

    auto* playButton = root->CreateChild<TextButton>(
        UnitBounds{.min = {.x = 0.38f, .y = 0.38f}, .max = {.x = 0.62f, .y = 0.48f}},
        *buttonFontRes,
        "Play",
        [this] {
            this->sceneManager.QueueReplace<SongSelectScene>(std::ref(this->sceneManager), std::ref(this->game));
        },
        buttonTexture);
    playButton->SetColors(
        SDL_Color{.r = 40, .g = 90, .b = 140, .a = 255},
        SDL_Color{.r = 60, .g = 120, .b = 180, .a = 255},
        SDL_Color{.r = 28, .g = 65, .b = 105, .a = 255},
        SDL_Color{.r = 245, .g = 245, .b = 245, .a = 255});

    auto* optionsButton = root->CreateChild<TextButton>(
        UnitBounds{.min = {.x = 0.38f, .y = 0.52f}, .max = {.x = 0.62f, .y = 0.62f}},
        *buttonFontRes,
        "Options",
        [this] {
            this->sceneManager.QueuePush<OptionsOverlayScene>(std::ref(this->sceneManager), std::ref(this->game));
        },
        buttonTexture);
    optionsButton->SetColors(
        SDL_Color{.r = 90, .g = 80, .b = 130, .a = 255},
        SDL_Color{.r = 120, .g = 102, .b = 175, .a = 255},
        SDL_Color{.r = 70, .g = 60, .b = 102, .a = 255},
        SDL_Color{.r = 245, .g = 245, .b = 245, .a = 255});

    auto* quitButton = root->CreateChild<TextButton>(
        UnitBounds{.min = {.x = 0.38f, .y = 0.66f}, .max = {.x = 0.62f, .y = 0.76f}},
        *buttonFontRes,
        "Quit",
        [] {
            SDL_Event quitEvent{};
            quitEvent.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quitEvent);
        },
        buttonTexture);
    quitButton->SetColors(
        SDL_Color{.r = 135, .g = 60, .b = 70, .a = 255},
        SDL_Color{.r = 170, .g = 80, .b = 92, .a = 255},
        SDL_Color{.r = 100, .g = 45, .b = 52, .a = 255},
        SDL_Color{.r = 245, .g = 245, .b = 245, .a = 255});
}

} // namespace Game
