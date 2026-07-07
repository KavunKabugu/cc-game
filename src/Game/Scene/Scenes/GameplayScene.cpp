#include "GameplayScene.h"

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <functional>

#include "Game/AudioManager.h"
#include "Game/Events/Interfaces.h"
#include "Game/Gameplay/ChartLoader.h"
#include "Game/Gameplay/GameplayConstants.h"
#include "Game/Gameplay/GameplayMath.h"
#include "Game/Gameplay/HitScoring.h"
#include "Game/Gameplay/LaneInputHandler.h"
#include "Game/Gameplay/RhythmField.h"
#include "Game/Gameplay/TimingRuler.h"
#include "Game/ResourceManager.h"
#include "Game/Game.h"
#include "Game/Scene/SceneManager.h"
#include "Game/Scene/Scenes/SongSelectScene.h"
#include "Game/Song/SongManager.h"
#include "Game/objects/Label.h"
#include "Game/objects/PanelRect.h"
#include "Game/objects/TextButton.h"
#include "Game/Profile.h"

namespace Game {

namespace {

constexpr double kResultsDelaySeconds = 1.0;

// Top-left HUD stack (unit coordinates, logical 1920×1080).
constexpr UnitBounds kHudScoreBounds{{0.02f, 0.02f}, {0.70f, 0.058f}};
constexpr UnitBounds kHudJudgementsBounds{{0.02f, 0.058f}, {0.92f, 0.098f}};
constexpr UnitBounds kHudAccuracyBounds{{0.02f, 0.098f}, {0.70f, 0.138f}};
constexpr UnitBounds kHudTimingBounds{{0.02f, 0.138f}, {0.92f, 0.178f}};
constexpr UnitBounds kTimingRulerBounds{{0.41f, 0.88f}, {0.59f, 0.93f}};
// Label::Render still draws at the slot's top-left when width/height are 0, park text off-screen.
constexpr UnitBounds kHiddenBounds{{0.0f, 0.0f}, {0.0f, 0.0f}};
constexpr UnitBounds kOffscreenBounds{{2.0f, 2.0f}, {2.01f, 2.01f}};

constexpr UnitBounds kResultsDimBounds{{0.00f, 0.00f}, {1.00f, 1.00f}};
constexpr UnitBounds kResultsScoreBounds{{0.06f, 0.20f}, {0.48f, 0.28f}};
constexpr UnitBounds kResultsAccuracyBounds{{0.06f, 0.29f}, {0.48f, 0.345f}};
constexpr UnitBounds kResultsJudgementsBounds{{0.06f, 0.355f}, {0.48f, 0.405f}};
constexpr UnitBounds kResultsRatioBounds{{0.06f, 0.405f}, {0.48f, 0.435f}};
constexpr UnitBounds kResultsBiasBounds{{0.06f, 0.438f}, {0.48f, 0.493f}};
constexpr UnitBounds kResultsStdDevBounds{{0.06f, 0.496f}, {0.48f, 0.551f}};
constexpr UnitBounds kResultsGraphBounds{{0.52f, 0.24f}, {0.94f, 0.64f}};
constexpr UnitBounds kResultsGraphToggleBounds{{0.52f, 0.17f}, {0.80f, 0.225f}};
constexpr UnitBounds kResultsBackButtonBounds{{0.38f, 0.88f}, {0.62f, 0.96f}};
constexpr UnitBounds kPauseTitleBounds{{0.30f, 0.08f}, {0.70f, 0.16f}};
constexpr UnitBounds kPauseResumeButtonBounds{{0.22f, 0.88f}, {0.48f, 0.96f}};
constexpr UnitBounds kPauseQuitButtonBounds{{0.52f, 0.88f}, {0.78f, 0.96f}};

class EscapeMenuHandler final : public GameObject, public IKeyHandler {
public:
    explicit EscapeMenuHandler(const UnitBounds bounds, std::function<void()> onEscape)
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

} // namespace

GameplayScene::GameplayScene(
    SceneManager& sceneManager,
    GameInstance& gameInstance,
    std::shared_ptr<Song::SongMetadata> selectedSong,
    const int selectedDifficultyIndex,
    Gameplay::GameplaySettings settings)
    : sceneManager(sceneManager),
      game(gameInstance),
      selectedSong(std::move(selectedSong)),
      selectedDifficultyIndex(selectedDifficultyIndex),
      settings(settings) {
    root->CreateChild<PanelRect>(UnitBounds{{0.0f, 0.0f}, {1.0f, 1.0f}}, SDL_Color{14, 20, 30, 255});
    root->CreateChild<PanelRect>(UnitBounds{{0.04f, 0.06f}, {0.96f, 0.94f}}, SDL_Color{20, 28, 42, 245});

    const auto titleFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 36.0f);
    const auto textFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 24.0f);
    const auto graphLabelFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 12.0f);
    const auto resultScoreFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 72.0f);
    const auto resultBodyFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 32.0f);
    const auto buttonFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 28.0f);
    const auto buttonTextureRes = ResourceManager::getInstance().Get<SDL_Texture>("button.png");
    const auto arcTextureRes = ResourceManager::getInstance().Get<SDL_Texture>("arc-quarter.png");

    if (!titleFontRes || !textFontRes || !resultScoreFontRes || !resultBodyFontRes || !buttonFontRes) {
        SDL_Log("GameplayScene: missing required fonts");
        return;
    }
    if (!arcTextureRes) {
        SDL_Log("GameplayScene: missing arc-quarter.png");
        return;
    }

    if (!this->selectedSong || this->selectedSong->difficulties.empty() ||
        selectedDifficultyIndex < 0 ||
        selectedDifficultyIndex >= static_cast<int>(this->selectedSong->difficulties.size())) {
        SDL_Log("GameplayScene: invalid song / difficulty selection");
        return;
    }

    const auto& difficulty = this->selectedSong->difficulties[selectedDifficultyIndex];
    const std::filesystem::path chartPath =
        Song::SongManager::ResolveSongFile(*this->selectedSong, difficulty.chartPath);

    auto chartRes = Gameplay::LoadChartFromFile(chartPath);
    if (!chartRes) {
        SDL_Log("GameplayScene: failed to load chart '%s'", chartPath.string().c_str());
        return;
    }

    if (this->selectedSong->audioFile.empty()) {
        SDL_Log("GameplayScene: song has no audio file");
        return;
    }

    const std::filesystem::path audioPath =
        Song::SongManager::ResolveSongFile(*this->selectedSong, this->selectedSong->audioFile);
    auto audioRes = ResourceManager::getInstance().Get<MIX_Audio>(audioPath.string());
    if (!audioRes || !*audioRes) {
        SDL_Log("GameplayScene: failed to load audio '%s'", audioPath.string().c_str());
        return;
    }

    const auto& video = game.GetVideoSettings();
    const int logicalW = video.logicalWidth;
    const int logicalH = video.logicalHeight;
    const float resolutionScale =
        std::max(Gameplay::ResolutionUniformScale(logicalW, logicalH), 0.001f);
    const float tunedCrosshairRadius = this->settings.crosshairRadius * resolutionScale;

    simulation.Configure(this->settings.noteSpeed, tunedCrosshairRadius, logicalW);
    simulation.LoadChart(*chartRes);
    chartNoteCount = simulation.ChartNoteCount();

    {
        const double tf = simulation.FirstNoteHitTime();
        const double tl = simulation.LastNoteHitTime();
        chartDomainFirst = std::isfinite(tf) ? tf : 0.0;
        chartDomainLast = std::isfinite(tl) ? tl : chartDomainFirst;
        resultsGraphEvents.reserve(chartNoteCount + 128);
        accuracySteps.reserve(chartNoteCount + 128);
    }

    rhythmField = root->CreateChild<Gameplay::RhythmField>(
        UnitBounds{{0.0f, 0.0f}, {1.0f, 1.0f}},
        *arcTextureRes,
        &simulation);

    scoreLabel = root->CreateChild<Label>(
        kHudScoreBounds,
        *titleFontRes,
        "Score: 0");
    scoreLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    judgementsLabel = root->CreateChild<Label>(
        kHudJudgementsBounds,
        *textFontRes,
        "P 0   G 0   B 0   M 0");
    judgementsLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    accuracyLabel = root->CreateChild<Label>(
        kHudAccuracyBounds,
        *textFontRes,
        "Acc: 100.0%");
    accuracyLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    timingStatsLabel = root->CreateChild<Label>(
        kHudTimingBounds,
        *textFontRes,
        "Bias: 0.00ms   Std.Dev.: 0.00ms");
    timingStatsLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    timingRuler = root->CreateChild<Gameplay::TimingRuler>(kTimingRulerBounds);

    dimLayer = root->CreateChild<PanelRect>(kHiddenBounds, SDL_Color{0, 0, 0, 170});
    resultsGraphDisplay = root->CreateChild<Gameplay::ResultsGraphDisplay>(kHiddenBounds);
    resultsGraphDisplay->SetLabelFont(graphLabelFontRes ? *graphLabelFontRes : *textFontRes);

    resultScoreLabel = root->CreateChild<Label>(kOffscreenBounds, *resultScoreFontRes, "");
    resultScoreLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    resultAccuracyLabel = root->CreateChild<Label>(kOffscreenBounds, *resultBodyFontRes, "");
    resultAccuracyLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    resultJudgementsLabel = root->CreateChild<Label>(kOffscreenBounds, *resultBodyFontRes, "");
    resultJudgementsLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    resultRatioLabel = root->CreateChild<Label>(kOffscreenBounds, *resultBodyFontRes, "");
    resultRatioLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    resultBiasLabel = root->CreateChild<Label>(kOffscreenBounds, *resultBodyFontRes, "");
    resultBiasLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    resultStdDevLabel = root->CreateChild<Label>(kOffscreenBounds, *resultBodyFontRes, "");
    resultStdDevLabel->SetAlignment(HorizontalAlignment::Left, VerticalAlignment::Top);

    laneInput = root->CreateChild<Gameplay::LaneInputHandler>(
        UnitBounds{{0.0f, 0.0f}, {0.0f, 0.0f}},
        this->settings.keyBindings);

    root->CreateChild<EscapeMenuHandler>(
        UnitBounds{{0.0f, 0.0f}, {0.0f, 0.0f}},
        [this]() { HandleEscapeKey(); });

    resultsBackButton = root->CreateChild<TextButton>(
        kOffscreenBounds,
        *buttonFontRes,
        "Back",
        [this]() { ReturnToSongSelect(); },
        buttonTextureRes ? *buttonTextureRes : nullptr);
    resultsBackButton->SetEnabled(false);

    pauseTitleLabel = root->CreateChild<Label>(kOffscreenBounds, *titleFontRes, "Paused");
    pauseTitleLabel->SetAlignment(HorizontalAlignment::Center, VerticalAlignment::Top);

    pauseResumeButton = root->CreateChild<TextButton>(
        kOffscreenBounds,
        *buttonFontRes,
        "Back",
        [this]() { ResumeFromPause(); },
        buttonTextureRes ? *buttonTextureRes : nullptr);
    pauseResumeButton->SetEnabled(false);

    pauseQuitButton = root->CreateChild<TextButton>(
        kOffscreenBounds,
        *buttonFontRes,
        "Quit",
        [this]() { HandleUserQuit(); },
        buttonTextureRes ? *buttonTextureRes : nullptr);
    pauseQuitButton->SetEnabled(false);

    graphToggleButton = root->CreateChild<TextButton>(
        kOffscreenBounds,
        *buttonFontRes,
        "Graph: Delta",
        [this]() {
            if (!resultsGraphDisplay || !graphToggleButton) {
                return;
            }
            using enum Gameplay::ResultsGraphMode;
            const auto next = resultsGraphDisplay->Mode() == Delta ? Accuracy : Delta;
            resultsGraphDisplay->SetMode(next);
            graphToggleButton->SetText(next == Delta ? "Graph: Delta" : "Graph: Accuracy");
        },
        buttonTextureRes ? *buttonTextureRes : nullptr);
    graphToggleButton->SetEnabled(false);

    const double spawnLead = simulation.SpawnLeadSeconds();
    const double firstHit = simulation.FirstNoteHitTime();
    const double offset = this->settings.audioOffsetSeconds;
    const double leadShortfall =
        std::isfinite(firstHit) ? std::max(0.0, spawnLead - firstHit - offset) : 0.0;
    // const double effectiveDelay = std::max(Gameplay::kStartDelaySeconds, leadShortfall);
    const double effectiveDelay = Gameplay::kStartDelaySeconds + leadShortfall;

    clock = std::make_unique<Gameplay::SongClock>(*audioRes, effectiveDelay, offset);
    simulationReady = true;
    UpdateHud();
}

GameplayScene::~GameplayScene() {
    if (clock) {
        clock->Stop();
    }
}

void GameplayScene::Update(const double dt) {
    CC_PROFILE("GameplayScene.Update");
    if (clock && simulationReady) {
        const bool clockShouldAdvance =
            !sessionEnded && phase != Phase::Paused && phase != Phase::ResumeGrace;
        if (clockShouldAdvance) {
            clock->Update(dt);
        }

        double songTime = 0.0;
        {
            CC_PROFILE("SongClock.SongTime");
            songTime = sessionEnded ? frozenSongTime : clock->SongTime();
        }

        switch (phase) {
        case Phase::Playing:
            ProcessInputs(songTime);
            simulation.Tick(songTime);
            ConsumeJudgements();
            HandleSongEnd();
            break;
        case Phase::Paused:
            DrainLaneInput();
            break;
        case Phase::ResumeGrace:
            DrainLaneInput();
            resumeGraceRemaining -= dt;
            if (resumeGraceRemaining <= 0.0) {
                FinishResumeGrace();
            }
            break;
        case Phase::EndDelay:
            DrainLaneInput();
            simulation.Tick(songTime);
            ConsumeJudgements();
            resultsDelayRemaining -= dt;
            if (resultsDelayRemaining <= 0.0) {
                EnterResults();
            }
            break;
        case Phase::Results:
            DrainLaneInput();
            break;
        }
    }

    SceneBase::Update(dt);
}

void GameplayScene::DrainLaneInput() const {
    if (laneInput) {
        (void)laneInput->Drain();
    }
}

void GameplayScene::ProcessInputs(const double songTimeSeconds) {
    if (!laneInput) return;

    const auto presses = laneInput->Drain();
    if (presses.empty()) return;

    if (settings.useWallClockForJudgementTiming) {
        if (!clock) {
            return;
        }
    }

    for (const auto&[lane, sdlTimestampNs] : presses) {
        double pressSongTime;
        if (settings.useWallClockForJudgementTiming) {
            pressSongTime = clock->WallTimeSongSecondsAt(sdlTimestampNs);
        } else {
            const Uint64 nowNs = SDL_GetTicksNS();
            double timeSinceEvent = static_cast<double>(nowNs - sdlTimestampNs) * 1e-9;
            if (timeSinceEvent < 0.0) timeSinceEvent = 0.0;
            pressSongTime = songTimeSeconds - timeSinceEvent;
        }
        simulation.TryHit(lane, pressSongTime);
    }
}

void GameplayScene::ConsumeJudgements() {
    CC_PROFILE("ConsumeJudgements");
    const auto& events = simulation.DrainEvents();
    if (events.empty()) return;

    using enum Gameplay::Judgement;
    for (const auto& result : events) {
        totalScore += Gameplay::ScoreForHitResult(result);
        sumAccuracyNumerator += Gameplay::AccuracyNumeratorContribution(result);
        if (result.missReason != Gameplay::MissReason::EmptyLane) {
            ++chartNotesProcessed;
        }
        if (result.judgement == Perfect || result.judgement == Great || result.judgement == Good || result.judgement == Bad) {
            ++timingHitCount;
            sumTimingDeltaMs += result.deltaMs;
            sumTimingDeltaSqMs += result.deltaMs * result.deltaMs;
            if (timingRuler) {
                timingRuler->PushHit(result.deltaMs, result.judgement);
            }
        }
        if (const auto idx = static_cast<size_t>(result.judgement); idx < judgementCounts.size()) {
            ++judgementCounts[idx];
        }

        resultsGraphEvents.push_back(Gameplay::ResultsGraphEvent{
            .judgement = result.judgement,
            .missReason = result.missReason,
            .deltaMs = result.deltaMs,
            .noteTargetTimeSeconds = result.noteTargetTimeSeconds,
            .pressTimeSeconds = result.pressTimeSeconds,
        });

        const double nx = Gameplay::NormalizedPlotXForHit(result, chartDomainFirst, chartDomainLast);
        const double accPct = AccuracyPercent();
        if (constexpr double kNxEps = 1e-7; !accuracySteps.empty() && std::abs(nx - accuracySteps.back().first) <= kNxEps) {
            accuracySteps.back().second = accPct;
        } else {
            accuracySteps.emplace_back(nx, accPct);
        }
    }
    simulation.ClearEvents();
    UpdateHud();
}

void GameplayScene::UpdateHud() {
    // This is a sanity check, ignore the "always true" warnings.
    // I know that it *is* technically always true, since Update already has a guard,
    // and Update will only be done if that guard passes.
    // So if UpdateHud is being executed then simulationReady must be true,
    // but I had this weird issue with the hud where it wasn't there
    // and since then I don't trust it.
    if (!simulationReady) return;

    if (scoreLabel) {
        scoreLabel->SetText(std::format("Score: {}", totalScore));
    }
    if (judgementsLabel) {
        judgementsLabel->SetText(std::format(
            "P {}   Gr {}   G {}   B {}   M {}",
            judgementCounts[static_cast<size_t>(Gameplay::Judgement::Perfect)],
            judgementCounts[static_cast<size_t>(Gameplay::Judgement::Great)],
            judgementCounts[static_cast<size_t>(Gameplay::Judgement::Good)],
            judgementCounts[static_cast<size_t>(Gameplay::Judgement::Bad)],
            judgementCounts[static_cast<size_t>(Gameplay::Judgement::Miss)]));
    }
    if (accuracyLabel) {
        accuracyLabel->SetText(std::format(
            "Acc: {:.1f}%",
            AccuracyPercent()));
    }
    if (timingStatsLabel) {
        timingStatsLabel->SetText(std::format(
            "Bias: {:.2f}ms   Std.Dev.: {:.2f}ms",
            MeanSignedTimingErrorMs(),
            TimingStandardDeviationMs()));
    }
}

void GameplayScene::HandleSongEnd() {
    if (phase != Phase::Playing) return;
    if (!clock || !clock->MusicStarted()) return;

    if (simulation.AllNotesResolved()) {
        endReason = EndReason::Completed;
        phase = Phase::EndDelay;
        resultsDelayRemaining = kResultsDelaySeconds;
    }
}

void GameplayScene::HandleEscapeKey() {
    switch (phase) {
    case Phase::Playing:
        EnterPaused();
        break;
    case Phase::Paused:
        ResumeFromPause();
        break;
    case Phase::ResumeGrace:
        break;
    case Phase::EndDelay:
        EnterResults();
        break;
    case Phase::Results:
        break;
    }
}

void GameplayScene::HandleUserQuit() {
    if (phase == Phase::Results) {
        return;
    }
    if (phase == Phase::EndDelay) {
        EnterResults();
        return;
    }

    if (clock) {
        frozenSongTime = clock->SongTime();
        clock->Stop();
    }
    sessionEnded = true;
    endReason = EndReason::Stopped;
    DrainLaneInput();
    ConsumeJudgements();
    EnterResults();
}

void GameplayScene::HideHud() {
    if (scoreLabel) scoreLabel->SetBounds(kOffscreenBounds);
    if (judgementsLabel) judgementsLabel->SetBounds(kOffscreenBounds);
    if (accuracyLabel) accuracyLabel->SetBounds(kOffscreenBounds);
    if (timingStatsLabel) timingStatsLabel->SetBounds(kOffscreenBounds);
    if (timingRuler) {
        timingRuler->SetBounds(kOffscreenBounds);
        timingRuler->Clear();
    }
}

void GameplayScene::ShowHud() {
    if (scoreLabel) scoreLabel->SetBounds(kHudScoreBounds);
    if (judgementsLabel) judgementsLabel->SetBounds(kHudJudgementsBounds);
    if (accuracyLabel) accuracyLabel->SetBounds(kHudAccuracyBounds);
    if (timingStatsLabel) timingStatsLabel->SetBounds(kHudTimingBounds);
    if (timingRuler) timingRuler->SetBounds(kTimingRulerBounds);
}

void GameplayScene::ShowStatsOverlay() {
    HideHud();

    if (dimLayer) dimLayer->SetBounds(kResultsDimBounds);
    if (resultsGraphDisplay) {
        resultsGraphDisplay->SetChartDomain(chartDomainFirst, chartDomainLast);
        resultsGraphDisplay->SetEvents(resultsGraphEvents);
        resultsGraphDisplay->SetAccuracySteps(accuracySteps);
        resultsGraphDisplay->SetBounds(kResultsGraphBounds);
    }

    if (graphToggleButton) {
        graphToggleButton->SetBounds(kResultsGraphToggleBounds);
        graphToggleButton->SetEnabled(true);
        if (resultsGraphDisplay) {
            using enum Gameplay::ResultsGraphMode;
            graphToggleButton->SetText(resultsGraphDisplay->Mode() == Accuracy ? "Graph: Accuracy"
                                                                               : "Graph: Delta");
        }
    }

    if (resultScoreLabel) {
        resultScoreLabel->SetBounds(kResultsScoreBounds);
        resultScoreLabel->SetText(std::format("Score: {}", totalScore));
    }
    if (resultAccuracyLabel) {
        resultAccuracyLabel->SetBounds(kResultsAccuracyBounds);
        resultAccuracyLabel->SetText(std::format("Accuracy: {:.2f}%", AccuracyPercent()));
    }
    if (resultJudgementsLabel) {
        resultJudgementsLabel->SetBounds(kResultsJudgementsBounds);
        resultJudgementsLabel->SetText(std::format(
            "Perfect {}   Great {}   Good {}   Bad {}   Miss {}",
            judgementCounts[static_cast<size_t>(Gameplay::Judgement::Perfect)],
            judgementCounts[static_cast<size_t>(Gameplay::Judgement::Great)],
            judgementCounts[static_cast<size_t>(Gameplay::Judgement::Good)],
            judgementCounts[static_cast<size_t>(Gameplay::Judgement::Bad)],
            judgementCounts[static_cast<size_t>(Gameplay::Judgement::Miss)]));
    }
    if (resultRatioLabel) {
        resultRatioLabel->SetBounds(kResultsRatioBounds);
        resultRatioLabel->SetText(PerfectGreatRatioText());
    }
    if (resultBiasLabel) {
        resultBiasLabel->SetBounds(kResultsBiasBounds);
        resultBiasLabel->SetText(std::format("Bias: {:+.2f} ms", MeanSignedTimingErrorMs()));
    }
    if (resultStdDevLabel) {
        resultStdDevLabel->SetBounds(kResultsStdDevBounds);
        resultStdDevLabel->SetText(std::format("Std Dev: {:.2f} ms", TimingStandardDeviationMs()));
    }
}

void GameplayScene::HideStatsOverlay() {
    if (dimLayer) dimLayer->SetBounds(kHiddenBounds);
    if (resultsGraphDisplay) resultsGraphDisplay->SetBounds(kHiddenBounds);
    if (graphToggleButton) {
        graphToggleButton->SetBounds(kOffscreenBounds);
        graphToggleButton->SetEnabled(false);
    }
    if (resultScoreLabel) resultScoreLabel->SetBounds(kOffscreenBounds);
    if (resultAccuracyLabel) resultAccuracyLabel->SetBounds(kOffscreenBounds);
    if (resultJudgementsLabel) resultJudgementsLabel->SetBounds(kOffscreenBounds);
    if (resultRatioLabel) resultRatioLabel->SetBounds(kOffscreenBounds);
    if (resultBiasLabel) resultBiasLabel->SetBounds(kOffscreenBounds);
    if (resultStdDevLabel) resultStdDevLabel->SetBounds(kOffscreenBounds);
    if (resultsBackButton) {
        resultsBackButton->SetBounds(kOffscreenBounds);
        resultsBackButton->SetEnabled(false);
    }
}

void GameplayScene::ShowPauseUi() {
    if (pauseTitleLabel) pauseTitleLabel->SetBounds(kPauseTitleBounds);
    if (pauseResumeButton) {
        pauseResumeButton->SetBounds(kPauseResumeButtonBounds);
        pauseResumeButton->SetEnabled(true);
    }
    if (pauseQuitButton) {
        pauseQuitButton->SetBounds(kPauseQuitButtonBounds);
        pauseQuitButton->SetEnabled(true);
    }
}

void GameplayScene::HidePauseUi() {
    if (pauseTitleLabel) pauseTitleLabel->SetBounds(kOffscreenBounds);
    if (pauseResumeButton) {
        pauseResumeButton->SetBounds(kOffscreenBounds);
        pauseResumeButton->SetEnabled(false);
    }
    if (pauseQuitButton) {
        pauseQuitButton->SetBounds(kOffscreenBounds);
        pauseQuitButton->SetEnabled(false);
    }
}

void GameplayScene::EnterPaused() {
    if (phase != Phase::Playing) {
        return;
    }

    if (clock) {
        clock->Pause();
    }
    DrainLaneInput();
    ConsumeJudgements();
    ShowStatsOverlay();
    ShowPauseUi();
    if (resultsBackButton) {
        resultsBackButton->SetBounds(kOffscreenBounds);
        resultsBackButton->SetEnabled(false);
    }
    phase = Phase::Paused;
}

void GameplayScene::ResumeFromPause() {
    if (phase != Phase::Paused) {
        return;
    }

    HidePauseUi();
    HideStatsOverlay();
    ShowHud();
    resumeGraceRemaining = Gameplay::kResumeGraceSeconds;
    phase = Phase::ResumeGrace;
}

void GameplayScene::FinishResumeGrace() {
    if (clock) {
        clock->Resume();
    }
    phase = Phase::Playing;
}

void GameplayScene::EnterResults() {
    ShowStatsOverlay();
    HidePauseUi();
    if (resultsBackButton) {
        resultsBackButton->SetBounds(kResultsBackButtonBounds);
        resultsBackButton->SetEnabled(true);
    }
    phase = Phase::Results;
}

void GameplayScene::ReturnToSongSelect() const {
    if (clock) {
        clock->Stop();
    }
    sceneManager.QueueReplace<SongSelectScene>(std::ref(sceneManager), std::ref(game));
}

double GameplayScene::AccuracyPercent() const {
    if (chartNoteCount == 0) {
        return 100.0;
    }
    if (chartNotesProcessed == 0) {
        return 100.0;
    }
    return 100.0 * sumAccuracyNumerator /
           (static_cast<double>(chartNotesProcessed) * 100.0);
}

double GameplayScene::MeanSignedTimingErrorMs() const {
    if (timingHitCount == 0) {
        return 0.0;
    }
    return sumTimingDeltaMs / static_cast<double>(timingHitCount);
}

double GameplayScene::TimingStandardDeviationMs() const {
    if (timingHitCount == 0) {
        return 0.0;
    }
    const auto n = static_cast<double>(timingHitCount);
    const double mean = sumTimingDeltaMs / n;
    const double var = sumTimingDeltaSqMs / n - mean * mean;
    return std::sqrt(std::max(0.0, var));
}

std::string GameplayScene::PerfectGreatRatioText() const {
    const int perfect = judgementCounts[static_cast<size_t>(Gameplay::Judgement::Perfect)];
    const int great = judgementCounts[static_cast<size_t>(Gameplay::Judgement::Great)];
    if (great == 0) {
        return std::format("Ratio: —  ({} / 0)", perfect);
    }
    const double ratio = static_cast<double>(perfect) / static_cast<double>(great);
    return std::format("Ratio: {:.2f}  ({} / {})", ratio, perfect, great);
}

} // namespace Game
