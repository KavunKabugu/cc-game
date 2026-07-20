#include "SongSelectScene.h"

#include <format>
#include <functional>

#include <SDL3/SDL_log.h>

#include "Game/Game.h"
#include "Game/Layout/VBoxLayout.h"
#include "Game/PathUtf8.h"
#include "Game/ResourceManager.h"
#include "Game/Score/ScoreStore.h"
#include "Game/Score/ResultsViewData.h"
#include "Game/Scene/SceneManager.h"
#include "Game/Scene/Scenes/GameplayScene.h"
#include "Game/Scene/Scenes/MainMenuScene.h"
#include "Game/Scene/Scenes/ResultsOverlayScene.h"
#include "Game/Song/SongManager.h"
#include "Game/objects/Label.h"
#include "Game/objects/PanelRect.h"
#include "Game/objects/ScrollContainer.h"
#include "Game/objects/SelectableRow.h"
#include "Game/objects/Sprite.h"
#include "Game/objects/TextButton.h"

namespace Game {
namespace {
constexpr UnitBounds kFullBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}};
constexpr Uint8 kCoverAlpha = 191; // ~75%

[[nodiscard]] const char* ResourceErrorToString(const ResourceError err) {
    switch (err) {
    case ResourceError::FileNotFound:
        return "FileNotFound";
    case ResourceError::InvalidFormat:
        return "InvalidFormat";
    case ResourceError::RendererNotInitialized:
        return "RendererNotInitialized";
    case ResourceError::SDLError:
        return "SDLError";
    case ResourceError::UnknownType:
        return "UnknownType";
    case ResourceError::TTFNotInitialized:
        return "TTFNotInitialized";
    case ResourceError::AudioNotLoaded:
        return "AudioNotLoaded";
    case ResourceError::MixerNotInitialized:
        return "MixerNotInitialized";
    }
    return "UnknownError";
}

void ApplySongRowColors(SelectableRow* row) {
    row->SetColors(
        SDL_Color{.r = 44, .g = 64, .b = 92, .a = 255},
        SDL_Color{.r = 58, .g = 82, .b = 114, .a = 255},
        SDL_Color{.r = 35, .g = 52, .b = 75, .a = 255});
    row->SetSelectedColors(
        SDL_Color{.r = 80, .g = 130, .b = 88, .a = 255},
        SDL_Color{.r = 96, .g = 152, .b = 105, .a = 255},
        SDL_Color{.r = 66, .g = 108, .b = 73, .a = 255});
}

void ApplyChartRowColors(SelectableRow* row) {
    row->SetColors(
        SDL_Color{.r = 60, .g = 60, .b = 92, .a = 255},
        SDL_Color{.r = 76, .g = 76, .b = 112, .a = 255},
        SDL_Color{.r = 46, .g = 46, .b = 76, .a = 255});
    row->SetSelectedColors(
        SDL_Color{.r = 130, .g = 95, .b = 66, .a = 255},
        SDL_Color{.r = 154, .g = 110, .b = 76, .a = 255},
        SDL_Color{.r = 108, .g = 80, .b = 55, .a = 255});
}

void WireMarqueeLabel(SelectableRow* row, Label* label) {
    if (!row || !label) {
        return;
    }
    label->SetOverflowMode(LabelOverflowMode::Marquee);
    row->SetOnVisualStateChanged([label](const bool selected, const bool hovered) {
        label->SetMarqueeActive(selected || hovered);
    });
}
} // namespace

SongSelectScene::SongSelectScene(SceneManager& sceneManager, GameInstance& gameInstance,
                                 const std::string& errorMessage)
    : sceneManager(sceneManager),
      game(gameInstance) {
    root->CreateChild<PanelRect>(kFullBounds, SDL_Color{.r = 0, .g = 0, .b = 0, .a = 255});
    coverSprite = root->CreateChild<Sprite>(kFullBounds);
    coverSprite->SetAlpha(kCoverAlpha);

    root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.02f, .y = 0.14f}, .max = {.x = 0.49f, .y = 0.88f}},
                                 SDL_Color{.r = 24, .g = 30, .b = 45, .a = 220});
    root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.51f, .y = 0.14f}, .max = {.x = 0.98f, .y = 0.88f}},
                                 SDL_Color{.r = 24, .g = 30, .b = 45, .a = 220});

    auto titleFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 56.0f);
    auto sectionFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 30.0f);
    auto buttonFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 34.0f);
    auto titleRowFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 28.0f);
    auto bodyRowFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 22.0f);
    auto metaRowFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 18.0f);
    auto buttonTextureRes = ResourceManager::getInstance().Get<SDL_Texture>("button.png");

    if (!titleFontRes || !sectionFontRes || !buttonFontRes || !titleRowFontRes || !bodyRowFontRes ||
        !metaRowFontRes) {
        return;
    }
    titleRowFont = *titleRowFontRes;
    bodyRowFont = *bodyRowFontRes;
    metaRowFont = *metaRowFontRes;

    if (!errorMessage.empty()) {
        root->CreateChild<PanelRect>(
            UnitBounds{.min = {.x = 0.10f, .y = 0.015f}, .max = {.x = 0.90f, .y = 0.055f}},
            SDL_Color{.r = 180, .g = 40, .b = 40, .a = 220});
        auto* errLabel = root->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.10f, .y = 0.015f}, .max = {.x = 0.90f, .y = 0.055f}},
            metaRowFont,
            errorMessage);
        errLabel->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);
        errLabel->SetColor(SDL_Color{.r = 255, .g = 255, .b = 255, .a = 255});
    }

    auto* title = root->CreateChild<Label>(
        UnitBounds{.min = {.x = 0.0f, .y = 0.06f}, .max = {.x = 1.0f, .y = 0.14f}},
        *titleFontRes,
        "Song Selection");
    title->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);

    const std::shared_ptr<SDL_Texture> buttonTexture = buttonTextureRes ? *buttonTextureRes : nullptr;
    leftPanelTabButton = root->CreateChild<TextButton>(
        UnitBounds{.min = {.x = 0.04f, .y = 0.155f}, .max = {.x = 0.30f, .y = 0.215f}},
        *sectionFontRes,
        "Charts",
        [this] { CycleLeftPanelMode(); },
        buttonTexture);
    leftPanelTabButton->SetColors(
        SDL_Color{.r = 50, .g = 70, .b = 100, .a = 255},
        SDL_Color{.r = 65, .g = 90, .b = 125, .a = 255},
        SDL_Color{.r = 40, .g = 55, .b = 80, .a = 255},
        SDL_Color{.r = 245, .g = 245, .b = 245, .a = 255});

    auto* songsLabel = root->CreateChild<Label>(
        UnitBounds{.min = {.x = 0.54f, .y = 0.155f}, .max = {.x = 0.95f, .y = 0.215f}},
        *sectionFontRes,
        "Songs");
    songsLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Middle);

    leftScroll = root->CreateChild<ScrollContainer>(
        UnitBounds{.min = {.x = 0.04f, .y = 0.23f}, .max = {.x = 0.47f, .y = 0.84f}});
    songScroll = root->CreateChild<ScrollContainer>(
        UnitBounds{.min = {.x = 0.54f, .y = 0.23f}, .max = {.x = 0.96f, .y = 0.84f}});

    leftScroll->SetLayout(std::make_unique<Layout::VBoxLayout>(8.0f, 8.0f, 56.0f));
    songScroll->SetLayout(std::make_unique<Layout::VBoxLayout>(8.0f, 8.0f, 92.0f));

    // NOTE: GetLibrary() currently returns a const reference under the
    // assumption there is no concurrent background refresh. Snapshot locally.
    const auto& library = Song::SongManager::GetInstance().GetLibrary();
    songs.assign(library.begin(), library.end());
    BuildSongList();
    if (!songs.empty()) {
        SelectSong(0);
    } else {
        BuildLeftPanel();
    }

    root->CreateChild<TextButton>(
        UnitBounds{.min = {.x = 0.42f, .y = 0.88f}, .max = {.x = 0.58f, .y = 0.96f}},
        *buttonFontRes,
        "Back",
        [this] {
            this->sceneManager.QueueReplace<MainMenuScene>(std::ref(this->sceneManager), std::ref(this->game));
        },
        buttonTexture);
}

void SongSelectScene::BuildSongList() {
    if (!songScroll || !titleRowFont || !bodyRowFont || !metaRowFont) {
        return;
    }

    songRows.clear();
    auto& children = songScroll->GetChildren();
    children.clear();

    if (songs.empty()) {
        auto* label = songScroll->CreateChild<Label>(kFullBounds, bodyRowFont, "No songs found.");
        label->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);
        songScroll->UpdateLayout();
        return;
    }

    for (int i = 0; i < static_cast<int>(songs.size()); ++i) {
        const auto& song = songs[i];
        auto* row = songScroll->CreateChild<SelectableRow>(kFullBounds, [this, i] { SelectSong(i); });
        ApplySongRowColors(row);

        auto* titleLabel = row->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.04f, .y = 0.06f}, .max = {.x = 0.96f, .y = 0.42f}},
            titleRowFont,
            song->title);
        titleLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Middle);
        titleLabel->SetOverflowMode(LabelOverflowMode::Marquee);

        auto* artistLabel = row->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.04f, .y = 0.42f}, .max = {.x = 0.96f, .y = 0.68f}},
            bodyRowFont,
            song->artist);
        artistLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Middle);
        artistLabel->SetOverflowMode(LabelOverflowMode::Marquee);
        row->SetOnVisualStateChanged([titleLabel, artistLabel](const bool selected, const bool hovered) {
            const bool active = selected || hovered;
            titleLabel->SetMarqueeActive(active);
            artistLabel->SetMarqueeActive(active);
        });

        const auto chartCount = song->difficulties.size();
        const auto metaText = std::format(
            "BPM {:.0f}  ·  {} chart{}",
            song->bpm,
            chartCount,
            chartCount == 1 ? "" : "s");
        auto* metaLabel = row->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.04f, .y = 0.68f}, .max = {.x = 0.96f, .y = 0.94f}},
            metaRowFont,
            metaText);
        metaLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Middle);
        metaLabel->SetColor(SDL_Color{.r = 200, .g = 210, .b = 220, .a = 255});

        songRows.push_back(row);
    }

    songScroll->UpdateLayout();
    UpdateSongSelectionVisuals();
}

void SongSelectScene::BuildLeftPanel() {
    if (leftPanelMode == LeftPanelMode::Scores) {
        leftScroll->SetLayout(std::make_unique<Layout::VBoxLayout>(8.0f, 8.0f, 92.0f));
        BuildScoreList();
    } else {
        leftScroll->SetLayout(std::make_unique<Layout::VBoxLayout>(8.0f, 8.0f, 56.0f));
        BuildChartList();
    }
    UpdateLeftPanelTabLabel();
}

void SongSelectScene::BuildChartList() {
    if (!leftScroll || !bodyRowFont) {
        return;
    }

    chartRows.clear();
    scoreRows.clear();
    selectedScoreIndex = -1;
    auto& children = leftScroll->GetChildren();
    children.clear();
    selectedDifficultyIndex = -1;

    if (selectedSongIndex < 0 || selectedSongIndex >= static_cast<int>(songs.size())) {
        auto* label = leftScroll->CreateChild<Label>(kFullBounds, bodyRowFont, "Select a song.");
        label->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);
        leftScroll->UpdateLayout();
        return;
    }

    const auto& song = songs[selectedSongIndex];
    if (song->difficulties.empty()) {
        auto* label = leftScroll->CreateChild<Label>(kFullBounds, bodyRowFont, "No charts in selected song.");
        label->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);
        leftScroll->UpdateLayout();
        return;
    }

    for (int i = 0; i < static_cast<int>(song->difficulties.size()); ++i) {
        const auto& diff = song->difficulties[i];
        auto* row = leftScroll->CreateChild<SelectableRow>(
            kFullBounds,
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
        ApplyChartRowColors(row);

        auto* nameLabel = row->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.04f, .y = 0.1f}, .max = {.x = 0.96f, .y = 0.9f}},
            bodyRowFont,
            diff.name);
        nameLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Middle);
        WireMarqueeLabel(row, nameLabel);

        chartRows.push_back(row);
    }

    leftScroll->UpdateLayout();
    UpdateChartSelectionVisuals();
}

void SongSelectScene::BuildScoreList() {
    if (!leftScroll || !titleRowFont || !bodyRowFont) {
        return;
    }

    chartRows.clear();
    scoreRows.clear();
    selectedDifficultyIndex = -1;
    auto& children = leftScroll->GetChildren();
    children.clear();
    selectedScoreIndex = -1;

    if (selectedSongIndex < 0 || selectedSongIndex >= static_cast<int>(songs.size())) {
        auto* label = leftScroll->CreateChild<Label>(kFullBounds, bodyRowFont, "Select a song.");
        label->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);
        leftScroll->UpdateLayout();
        return;
    }

    const auto& song = songs[selectedSongIndex];
    const auto scores = Score::ScoreStore::ListForSong(*song);
    if (scores.empty()) {
        auto* label = leftScroll->CreateChild<Label>(kFullBounds, bodyRowFont, "No scores.");
        label->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Middle);
        leftScroll->UpdateLayout();
        return;
    }

    for (int i = 0; i < static_cast<int>(scores.size()); ++i) {
        const auto& entry = scores[i];
        auto* row = leftScroll->CreateChild<SelectableRow>(
            kFullBounds,
            [this, i, entry, song] {
                selectedScoreIndex = i;
                UpdateScoreSelectionVisuals();

                if (auto loaded = Score::ScoreStore::Load(song->songFolder, entry.runId)) {
                    ResultsOverlayContext overlayContext;
                    overlayContext.song = song;
                    overlayContext.replayId = loaded->summary.replayId;
                    this->sceneManager.QueuePush<ResultsOverlayScene>(
                        std::ref(this->sceneManager),
                        std::ref(this->game),
                        ResultsOverlayScene::Mode::Browse,
                        std::move(loaded->detail),
                        std::move(overlayContext));
                } else {
                    SDL_LogWarn(
                        SDL_LOG_CATEGORY_APPLICATION,
                        "SongSelectScene: failed to load score '%s' for song '%s'",
                        entry.runId.c_str(),
                        PathToUtf8String(song->songFolder).c_str());
                }
            });
        ApplyChartRowColors(row);

        auto* scoreLabel = row->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.04f, .y = 0.08f}, .max = {.x = 0.96f, .y = 0.48f}},
            titleRowFont,
            std::format("{}", entry.score));
        scoreLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Middle);
        scoreLabel->SetOverflowMode(LabelOverflowMode::Marquee);

        const auto statsLine = std::format(
            "{}  ·  {}  ·  {:.2f}%",
            entry.playerName,
            entry.difficultyName,
            entry.accuracyPercent);
        auto* statsLabel = row->CreateChild<Label>(
            UnitBounds{.min = {.x = 0.04f, .y = 0.48f}, .max = {.x = 0.96f, .y = 0.92f}},
            bodyRowFont,
            statsLine);
        statsLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Middle);
        statsLabel->SetOverflowMode(LabelOverflowMode::Marquee);
        statsLabel->SetColor(SDL_Color{.r = 200, .g = 210, .b = 220, .a = 255});
        row->SetOnVisualStateChanged([scoreLabel, statsLabel](const bool selected, const bool hovered) {
            const bool active = selected || hovered;
            scoreLabel->SetMarqueeActive(active);
            statsLabel->SetMarqueeActive(active);
        });

        scoreRows.push_back(row);
    }

    leftScroll->UpdateLayout();
    UpdateScoreSelectionVisuals();
}

void SongSelectScene::SelectSong(const int index) {
    if (index < 0 || index >= static_cast<int>(songs.size())) {
        return;
    }

    selectedSongIndex = index;
    selectedDifficultyIndex = -1;
    selectedScoreIndex = -1;
    UpdateSongSelectionVisuals();
    UpdateCoverBackground();
    BuildLeftPanel();
}

void SongSelectScene::CycleLeftPanelMode() {
    leftPanelMode = leftPanelMode == LeftPanelMode::Charts ? LeftPanelMode::Scores : LeftPanelMode::Charts;
    BuildLeftPanel();
}

void SongSelectScene::UpdateLeftPanelTabLabel() const {
    if (!leftPanelTabButton) {
        return;
    }
    leftPanelTabButton->SetText(leftPanelMode == LeftPanelMode::Charts ? "Charts" : "Scores");
}

void SongSelectScene::UpdateSongSelectionVisuals() const {
    for (int i = 0; i < static_cast<int>(songRows.size()); ++i) {
        songRows[i]->SetSelected(i == selectedSongIndex);
    }
}

void SongSelectScene::UpdateChartSelectionVisuals() const {
    for (int i = 0; i < static_cast<int>(chartRows.size()); ++i) {
        chartRows[i]->SetSelected(i == selectedDifficultyIndex);
    }
}

void SongSelectScene::UpdateScoreSelectionVisuals() const {
    for (int i = 0; i < static_cast<int>(scoreRows.size()); ++i) {
        scoreRows[i]->SetSelected(i == selectedScoreIndex);
    }
}

void SongSelectScene::UpdateCoverBackground() const {
    if (!coverSprite) {
        return;
    }

    if (selectedSongIndex < 0 || selectedSongIndex >= static_cast<int>(songs.size())) {
        coverSprite->SetTexture(nullptr);
        return;
    }

    const auto& song = songs[selectedSongIndex];
    if (!song || song->coverFile.empty()) {
        coverSprite->SetTexture(nullptr);
        return;
    }

    const std::filesystem::path coverPath =
        Song::SongManager::ResolveSongFile(*song, Utf8StringToPath(song->coverFile));
    if (auto coverRes = ResourceManager::getInstance().Get<SDL_Texture>(PathToUtf8String(coverPath))) {
        coverSprite->SetTexture(*coverRes);
        coverSprite->SetAlpha(kCoverAlpha);
    } else {
        coverSprite->SetTexture(nullptr);
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION,
            "SongSelectScene: failed to load cover '%s' (%s)",
            PathToUtf8String(coverPath).c_str(),
            ResourceErrorToString(coverRes.error()));
    }
}

} // namespace Game
