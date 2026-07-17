#include "SongSelectScene.h"

#include <functional>
#include <format>

#include "Game/Game.h"
#include "Game/Layout/VBoxLayout.h"
#include "Game/ResourceManager.h"
#include "Game/Scene/SceneManager.h"
#include "Game/Scene/Scenes/GameplayScene.h"
#include "Game/Scene/Scenes/MainMenuScene.h"
#include "Game/objects/Label.h"
#include "Game/objects/PanelRect.h"
#include "Game/objects/ScrollContainer.h"
#include "Game/objects/TextButton.h"
#include "Game/Song/SongManager.h"

namespace Game {

SongSelectScene::SongSelectScene(SceneManager& sceneManager, GameInstance& gameInstance, const std::string& errorMessage)
    : sceneManager(sceneManager),
      game(gameInstance) {
    root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}},
                                 SDL_Color{.r = 18, .g = 22, .b = 32, .a = 255});
    root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.05f, .y = 0.14f}, .max = {.x = 0.48f, .y = 0.88f}},
                                 SDL_Color{.r = 24, .g = 30, .b = 45, .a = 220});
    root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.52f, .y = 0.14f}, .max = {.x = 0.95f, .y = 0.88f}},
                                 SDL_Color{.r = 24, .g = 30, .b = 45, .a = 220});

    auto titleFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 56.0f);
    auto sectionFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 30.0f);
    auto buttonFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 34.0f);
    const auto rowFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 26.0f);
    auto buttonTextureRes = ResourceManager::getInstance().Get<SDL_Texture>("button.png");

    if (!titleFontRes || !sectionFontRes || !buttonFontRes || !rowFontRes) {
        return;
    }
    rowFont = *rowFontRes;

    if (!errorMessage.empty()) {
        [[maybe_unused]] auto* errPanel = root->CreateChild<PanelRect>(
            UnitBounds{.min = {.x = 0.10f, .y = 0.015f}, .max = {.x = 0.90f, .y = 0.055f}},
            SDL_Color{.r = 180, .g = 40, .b = 40, .a = 220}
        );
        auto* errLabel = root->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.10f, .y = 0.015f}, .max = {.x = 0.90f, .y = 0.055f}},
            *rowFontRes,
            errorMessage
        );
        errLabel->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);
        errLabel->SetColor(SDL_Color{.r = 255, .g = 255, .b = 255, .a = 255});
    }

    auto* title = root->CreateChild<Label>(
        UnitBounds{.min = {.x = 0.0f, .y = 0.06f}, .max = {.x = 1.0f, .y = 0.16f}},
        *titleFontRes,
        "Song Selection");
    title->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);

    auto* chartLabel = root->CreateChild<Label>(
        UnitBounds{.min = {.x = 0.08f, .y = 0.16f}, .max = {.x = 0.45f, .y = 0.23f}},
        *sectionFontRes,
        "Charts");
    chartLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Middle);

    auto* songsLabel = root->CreateChild<Label>(
        UnitBounds{.min = {.x = 0.55f, .y = 0.16f}, .max = {.x = 0.92f, .y = 0.23f}},
        *sectionFontRes,
        "Songs");
    songsLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Middle);

    chartScroll = root->CreateChild<ScrollContainer>(UnitBounds{.min = {.x = 0.08f, .y = 0.24f}, .max = {.x = 0.45f, .y = 0.82f}});
    songScroll = root->CreateChild<ScrollContainer>(UnitBounds{.min = {.x = 0.55f, .y = 0.24f}, .max = {.x = 0.92f, .y = 0.82f}});

    chartScroll->SetLayout(std::make_unique<Layout::VBoxLayout>(8.0f, 8.0f, 52.0f));
    songScroll->SetLayout(std::make_unique<Layout::VBoxLayout>(8.0f, 8.0f, 52.0f));

    // NOTE: GetLibrary() currently returns a const reference under the
    // assumption there is no concurrent background refresh. Snapshot locally.
    const auto& library = Song::SongManager::GetInstance().GetLibrary();
    songs.assign(library.begin(), library.end());
    BuildSongList();
    if (!songs.empty()) {
        SelectSong(0);
    } else {
        BuildChartList();
    }

    root->CreateChild<TextButton>(
        UnitBounds{.min = {.x = 0.42f, .y = 0.88f}, .max = {.x = 0.58f, .y = 0.96f}},
        *buttonFontRes,
        "Back",
        [this] {
            this->sceneManager.QueueReplace<MainMenuScene>(std::ref(this->sceneManager), std::ref(this->game));
        },
        buttonTextureRes ? *buttonTextureRes : nullptr);
}

void SongSelectScene::BuildSongList() {
    if (!songScroll || !rowFont) {
        return;
    }

    songButtons.clear();
    auto& children = songScroll->GetChildren();
    children.clear();

    if (songs.empty()) {
        auto* label = songScroll->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}},
            rowFont,
            "No songs found.");
        label->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);
        songScroll->UpdateLayout();
        return;
    }

    for (int i = 0; i < static_cast<int>(songs.size()); ++i) {
        const auto& song = songs[i];
        // If you are here with clangd, std::format will show an error
        // This is fine, because of some C++ magic with an internal _S_to_enum() function.
        // I didn't really understand what is going on, but the code compiles and runs correctly,
        // so the error *should be* only cosmetic.
        auto* button = songScroll->CreateChild<TextButton>(
            UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}},
            rowFont,
            std::format("{} - {}", song->artist, song->title),
            [this, i] { SelectSong(i); });
        button->SetColors(
            SDL_Color{.r = 44, .g = 64, .b = 92, .a = 255},
            SDL_Color{.r = 58, .g = 82, .b = 114, .a = 255},
            SDL_Color{.r = 35, .g = 52, .b = 75, .a = 255},
            SDL_Color{.r = 245, .g = 245, .b = 245, .a = 255});
        button->SetSelectedColors(
            SDL_Color{.r = 80, .g = 130, .b = 88, .a = 255},
            SDL_Color{.r = 96, .g = 152, .b = 105, .a = 255},
            SDL_Color{.r = 66, .g = 108, .b = 73, .a = 255});
        songButtons.push_back(button);
    }

    songScroll->UpdateLayout();
    UpdateSongSelectionVisuals();
}

void SongSelectScene::BuildChartList() {
    if (!chartScroll || !rowFont) {
        return;
    }

    chartButtons.clear();
    auto& children = chartScroll->GetChildren();
    children.clear();
    selectedDifficultyIndex = -1;

    if (selectedSongIndex < 0 || selectedSongIndex >= static_cast<int>(songs.size())) {
        auto* label = chartScroll->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}},
            rowFont,
            "Select a song.");
        label->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);
        chartScroll->UpdateLayout();
        return;
    }

    const auto& song = songs[selectedSongIndex];
    if (song->difficulties.empty()) {
        auto* label = chartScroll->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}},
            rowFont,
            "No charts in selected song.");
        label->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);
        chartScroll->UpdateLayout();
        return;
    }

    for (int i = 0; i < static_cast<int>(song->difficulties.size()); ++i) {
        const auto& diff = song->difficulties[i];
        auto* button = chartScroll->CreateChild<TextButton>(
            UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}},
            rowFont,
            std::format("{}{}", diff.name, diff.level > 0 ? std::format(" (Lv {})", diff.level) : ""),
            [this, i] {
                if (selectedSongIndex < 0 || selectedSongIndex >= static_cast<int>(songs.size())) {
                    return;
                }
                selectedDifficultyIndex = i;
                UpdateChartSelectionVisuals();
                const auto selectedSong = songs[selectedSongIndex];
                this->sceneManager.QueueReplace<GameplayScene>(
                    std::ref(this->sceneManager),
                    std::ref(this->game),
                    selectedSong,
                    i,
                    this->game.GetGameplaySettings());
            });
        button->SetColors(
            SDL_Color{.r = 60, .g = 60, .b = 92, .a = 255},
            SDL_Color{.r = 76, .g = 76, .b = 112, .a = 255},
            SDL_Color{.r = 46, .g = 46, .b = 76, .a = 255},
            SDL_Color{.r = 245, .g = 245, .b = 245, .a = 255});
        button->SetSelectedColors(
            SDL_Color{.r = 130, .g = 95, .b = 66, .a = 255},
            SDL_Color{.r = 154, .g = 110, .b = 76, .a = 255},
            SDL_Color{.r = 108, .g = 80, .b = 55, .a = 255});
        chartButtons.push_back(button);
    }

    chartScroll->UpdateLayout();
    UpdateChartSelectionVisuals();
}

void SongSelectScene::SelectSong(const int index) {
    if (index < 0 || index >= static_cast<int>(songs.size())) {
        return;
    }

    selectedSongIndex = index;
    selectedDifficultyIndex = -1;
    UpdateSongSelectionVisuals();
    BuildChartList();
}

void SongSelectScene::UpdateSongSelectionVisuals() const {
    for (int i = 0; i < static_cast<int>(songButtons.size()); ++i) {
        songButtons[i]->SetSelected(i == selectedSongIndex);
    }
}

void SongSelectScene::UpdateChartSelectionVisuals() const {
    for (int i = 0; i < static_cast<int>(chartButtons.size()); ++i) {
        chartButtons[i]->SetSelected(i == selectedDifficultyIndex);
    }
}

} // namespace Game
