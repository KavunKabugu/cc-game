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
        root->CreateChild<Sprite>(UnitBounds{{0.0f, 0.0f}, {1.0f, 1.0f}}, *bgRes);
    } else {
        root->CreateChild<PanelRect>(UnitBounds{{0.0f, 0.0f}, {1.0f, 1.0f}}, SDL_Color{20, 22, 35, 255});
    }

    root->CreateChild<PanelRect>(UnitBounds{{0.3f, 0.12f}, {0.7f, 0.88f}}, SDL_Color{10, 10, 18, 180});

    const auto buttonTextureRes = ResourceManager::getInstance().Get<SDL_Texture>("button.png");
    std::shared_ptr<SDL_Texture> buttonTexture = buttonTextureRes ? *buttonTextureRes : nullptr;

    auto titleFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 72.0f);
    auto buttonFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 42.0f);
    if (!titleFontRes || !buttonFontRes) {
        return;
    }

    auto* title = root->CreateChild<Label>(
        UnitBounds{{0.0f, 0.14f}, {1.0f, 0.28f}},
        *titleFontRes,
        "CC GAME");
    title->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);

    auto* playButton = root->CreateChild<TextButton>(
        UnitBounds{{0.38f, 0.38f}, {0.62f, 0.48f}},
        *buttonFontRes,
        "Play",
        [this]() {
            this->sceneManager.QueueReplace<SongSelectScene>(std::ref(this->sceneManager), std::ref(this->game));
        },
        buttonTexture);
    playButton->SetColors(
        SDL_Color{40, 90, 140, 255},
        SDL_Color{60, 120, 180, 255},
        SDL_Color{28, 65, 105, 255},
        SDL_Color{245, 245, 245, 255});

    auto* optionsButton = root->CreateChild<TextButton>(
        UnitBounds{{0.38f, 0.52f}, {0.62f, 0.62f}},
        *buttonFontRes,
        "Options",
        [this]() {
            this->sceneManager.QueuePush<OptionsOverlayScene>(std::ref(this->sceneManager), std::ref(this->game));
        },
        buttonTexture);
    optionsButton->SetColors(
        SDL_Color{90, 80, 130, 255},
        SDL_Color{120, 102, 175, 255},
        SDL_Color{70, 60, 102, 255},
        SDL_Color{245, 245, 245, 255});

    auto* quitButton = root->CreateChild<TextButton>(
        UnitBounds{{0.38f, 0.66f}, {0.62f, 0.76f}},
        *buttonFontRes,
        "Quit",
        []() {
            SDL_Event quitEvent{};
            quitEvent.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quitEvent);
        },
        buttonTexture);
    quitButton->SetColors(
        SDL_Color{135, 60, 70, 255},
        SDL_Color{170, 80, 92, 255},
        SDL_Color{100, 45, 52, 255},
        SDL_Color{245, 245, 245, 255});
}

} // namespace Game
