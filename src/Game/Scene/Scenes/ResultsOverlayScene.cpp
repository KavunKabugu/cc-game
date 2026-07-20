#include "ResultsOverlayScene.h"

#include <format>
#include <functional>

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_log.h>

#include "Game/Events/Interfaces.h"
#include "Game/Game.h"
#include "Game/PathUtf8.h"
#include "Game/ResourceManager.h"
#include "Game/Score/ReplayStore.h"
#include "Game/Scene/SceneManager.h"
#include "Game/Scene/Scenes/GameplayScene.h"
#include "Game/Scene/Scenes/SongSelectScene.h"
#include "Game/objects/Label.h"
#include "Game/objects/PanelRect.h"
#include "Game/objects/TextButton.h"

namespace Game {
namespace {

constexpr UnitBounds kResultsDimBounds{.min = {.x = 0.00f, .y = 0.00f}, .max = {.x = 1.00f, .y = 1.00f}};
constexpr UnitBounds kResultsScoreBounds{.min = {.x = 0.06f, .y = 0.20f}, .max = {.x = 0.48f, .y = 0.28f}};
constexpr UnitBounds kResultsAccuracyBounds{.min = {.x = 0.06f, .y = 0.29f}, .max = {.x = 0.48f, .y = 0.345f}};
constexpr UnitBounds kResultsJudgementsBounds{.min = {.x = 0.06f, .y = 0.355f}, .max = {.x = 0.48f, .y = 0.405f}};
constexpr UnitBounds kResultsRatioBounds{.min = {.x = 0.06f, .y = 0.405f}, .max = {.x = 0.48f, .y = 0.435f}};
constexpr UnitBounds kResultsBiasBounds{.min = {.x = 0.06f, .y = 0.438f}, .max = {.x = 0.48f, .y = 0.493f}};
constexpr UnitBounds kResultsStdDevBounds{.min = {.x = 0.06f, .y = 0.496f}, .max = {.x = 0.48f, .y = 0.551f}};
constexpr UnitBounds kResultsGraphBounds{.min = {.x = 0.52f, .y = 0.24f}, .max = {.x = 0.94f, .y = 0.64f}};
constexpr UnitBounds kResultsGraphToggleBounds{.min = {.x = 0.52f, .y = 0.17f}, .max = {.x = 0.80f, .y = 0.225f}};
constexpr UnitBounds kResultsBackButtonAloneBounds{.min = {.x = 0.38f, .y = 0.88f}, .max = {.x = 0.62f, .y = 0.96f}};
constexpr UnitBounds kResultsWatchReplayBounds{.min = {.x = 0.22f, .y = 0.88f}, .max = {.x = 0.48f, .y = 0.96f}};
constexpr UnitBounds kResultsBackWithWatchBounds{.min = {.x = 0.52f, .y = 0.88f}, .max = {.x = 0.78f, .y = 0.96f}};
constexpr UnitBounds kPauseTitleBounds{.min = {.x = 0.30f, .y = 0.08f}, .max = {.x = 0.70f, .y = 0.16f}};
constexpr UnitBounds kPauseResumeButtonBounds{.min = {.x = 0.22f, .y = 0.88f}, .max = {.x = 0.48f, .y = 0.96f}};
constexpr UnitBounds kPauseQuitButtonBounds{.min = {.x = 0.52f, .y = 0.88f}, .max = {.x = 0.78f, .y = 0.96f}};

class EscapeKeyHandler final : public GameObject, public IKeyHandler {
public:
    explicit EscapeKeyHandler(const UnitBounds bounds, std::function<void()> onEscape)
        : GameObject(bounds), onEscape(std::move(onEscape)) {}

    void Update() override {}

    IKeyHandler* AsKeyHandler() override { return this; }

    bool OnKeyDown(const SDL_Keycode key, const Uint64 /*timestamp*/) override {
        if (key == SDLK_ESCAPE && onEscape) {
            onEscape();
            return true;
        }
        return false;
    }

    bool OnKeyUp(SDL_Keycode, Uint64) override { return false; }

private:
    std::function<void()> onEscape;
};

[[nodiscard]] bool CanWatchReplay(const ResultsOverlayContext& context) {
    if (!context.song || context.replayId.empty()) {
        return false;
    }
    return Score::ReplayStore::Exists(context.song->songFolder, context.replayId);
}

} // namespace

ResultsOverlayScene::ResultsOverlayScene(
    SceneManager& sceneManager,
    GameInstance& gameInstance,
    const Mode mode,
    Score::ResultsViewData data,
    ResultsOverlayContext context)
    : sceneManager(sceneManager),
      game(gameInstance),
      mode(mode),
      data(std::move(data)),
      context(std::move(context)) {
    root->CreateChild<PanelRect>(kResultsDimBounds, SDL_Color{.r = 0, .g = 0, .b = 0, .a = 170});

    const auto titleFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 48.0f);
    const auto resultScoreFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 72.0f);
    const auto resultBodyFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 32.0f);
    const auto graphLabelFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 12.0f);
    const auto buttonFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 28.0f);
    const auto buttonTextureRes = ResourceManager::getInstance().Get<SDL_Texture>("button.png");

    if (!resultScoreFontRes || !resultBodyFontRes || !buttonFontRes) {
        return;
    }

    const std::shared_ptr<SDL_Texture> buttonTexture = buttonTextureRes ? *buttonTextureRes : nullptr;

    if (mode == Mode::Pause && titleFontRes) {
        auto* pauseTitle = root->CreateChild<Label>(kPauseTitleBounds, *titleFontRes, "Paused");
        pauseTitle->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Top);
    }

    auto* scoreLabel = root->CreateChild<Label>(
        kResultsScoreBounds,
        *resultScoreFontRes,
        std::format("Score: {}", this->data.score));
    scoreLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    auto* accuracyLabel = root->CreateChild<Label>(
        kResultsAccuracyBounds,
        *resultBodyFontRes,
        std::format("Accuracy: {:.2f}%", this->data.accuracyPercent));
    accuracyLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    auto* judgementsLabel = root->CreateChild<Label>(
        kResultsJudgementsBounds,
        *resultBodyFontRes,
        std::format(
            "Perfect {}   Great {}   Good {}   Bad {}   Miss {}",
            this->data.judgementCounts[static_cast<size_t>(Gameplay::Judgement::Perfect)],
            this->data.judgementCounts[static_cast<size_t>(Gameplay::Judgement::Great)],
            this->data.judgementCounts[static_cast<size_t>(Gameplay::Judgement::Good)],
            this->data.judgementCounts[static_cast<size_t>(Gameplay::Judgement::Bad)],
            this->data.judgementCounts[static_cast<size_t>(Gameplay::Judgement::Miss)]));
    judgementsLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    auto* ratioLabel = root->CreateChild<Label>(
        kResultsRatioBounds,
        *resultBodyFontRes,
        this->data.perfectGreatRatioText);
    ratioLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    auto* biasLabel = root->CreateChild<Label>(
        kResultsBiasBounds,
        *resultBodyFontRes,
        std::format("Bias: {:+.2f} ms", this->data.biasMs));
    biasLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    auto* stdDevLabel = root->CreateChild<Label>(
        kResultsStdDevBounds,
        *resultBodyFontRes,
        std::format("Std Dev: {:.2f} ms", this->data.stdDevMs));
    stdDevLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    resultsGraphDisplay = root->CreateChild<Gameplay::ResultsGraphDisplay>(kResultsGraphBounds);
    resultsGraphDisplay->SetLabelFont(graphLabelFontRes ? *graphLabelFontRes : *resultBodyFontRes);
    resultsGraphDisplay->SetChartDomain(this->data.chartDomainFirst, this->data.chartDomainLast);
    resultsGraphDisplay->SetEvents(this->data.graphEvents);
    resultsGraphDisplay->SetAccuracySteps(this->data.accuracySteps);

    graphToggleButton = root->CreateChild<TextButton>(
        kResultsGraphToggleBounds,
        *buttonFontRes,
        "Graph: Delta",
        [this] {
            if (!resultsGraphDisplay || !graphToggleButton) {
                return;
            }
            using enum Gameplay::ResultsGraphMode;
            const auto next = resultsGraphDisplay->Mode() == Delta ? Accuracy : Delta;
            resultsGraphDisplay->SetMode(next);
            graphToggleButton->SetText(next == Delta ? "Graph: Delta" : "Graph: Accuracy");
        },
        buttonTexture);

    if (mode == Mode::Pause) {
        root->CreateChild<TextButton>(
            kPauseResumeButtonBounds,
            *buttonFontRes,
            "Back",
            [this] { CloseResume(); },
            buttonTexture);
        root->CreateChild<TextButton>(
            kPauseQuitButtonBounds,
            *buttonFontRes,
            "Quit",
            [this] { CloseToSongSelect(); },
            buttonTexture);
    } else {
        const bool showWatch = (mode == Mode::Results || mode == Mode::Browse) && CanWatchReplay(this->context);
        if (showWatch) {
            root->CreateChild<TextButton>(
                kResultsWatchReplayBounds,
                *buttonFontRes,
                "Watch Replay",
                [this] { LaunchWatchReplay(); },
                buttonTexture);
        }
        root->CreateChild<TextButton>(
            showWatch ? kResultsBackWithWatchBounds : kResultsBackButtonAloneBounds,
            *buttonFontRes,
            "Back",
            [this] {
                if (this->mode == Mode::Browse) {
                    CloseBrowse();
                } else {
                    CloseToSongSelect();
                }
            },
            buttonTexture);
    }

    root->CreateChild<EscapeKeyHandler>(
        UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 0.0f, .y = 0.0f}},
        [this] { HandleEscape(); });
}

void ResultsOverlayScene::CloseResume() const {
    sceneManager.QueuePop();
}

void ResultsOverlayScene::CloseToSongSelect() const {
    sceneManager.QueuePop();
    sceneManager.QueueReplace<SongSelectScene>(std::ref(sceneManager), std::ref(game));
}

void ResultsOverlayScene::CloseBrowse() const {
    sceneManager.QueuePop();
}

void ResultsOverlayScene::LaunchWatchReplay() const {
    if (!context.song || context.replayId.empty()) {
        return;
    }

    auto loaded = Score::ReplayStore::Load(context.song->songFolder, context.replayId);
    if (!loaded) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION,
            "ResultsOverlayScene: replay '%s' missing for song '%s'",
            context.replayId.c_str(),
            PathToUtf8String(context.song->songFolder).c_str());
        return;
    }

    int difficultyIndex = context.difficultyIndex;
    if (difficultyIndex < 0) {
        difficultyIndex = loaded->difficultyIndex;
    }
    if (difficultyIndex < 0 ||
        difficultyIndex >= static_cast<int>(context.song->difficulties.size())) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION,
            "ResultsOverlayScene: invalid difficulty for replay '%s'",
            context.replayId.c_str());
        return;
    }

    sceneManager.QueuePop();
    sceneManager.QueueReplace<GameplayScene>(
        std::ref(sceneManager),
        std::ref(game),
        context.song,
        difficultyIndex,
        game.GetGameplaySettings(),
        GameplayScene::PlayMode::Replay,
        std::move(loaded));
}

void ResultsOverlayScene::HandleEscape() const {
    switch (mode) {
    case Mode::Pause:
        CloseResume();
        break;
    case Mode::Results:
        CloseToSongSelect();
        break;
    case Mode::Browse:
        CloseBrowse();
        break;
    }
}

} // namespace Game
