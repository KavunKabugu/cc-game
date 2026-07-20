#include "GameplayScene.h"

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <functional>

#include "Game/PathUtf8.h"

using Game::PathToUtf8String;
using Game::Utf8StringToPath;

#include "Game/AudioManager.h"
#include "Game/Events/Interfaces.h"
#include "Game/Gameplay/ChartLoader.h"
#include "Game/Gameplay/GameplayConstants.h"
#include "Game/Gameplay/GameplayMath.h"
#include "Game/Gameplay/HitScoring.h"
#include "Game/Gameplay/LaneInputHandler.h"
#include "Game/Gameplay/LaneRemap.h"
#include "Game/Gameplay/RhythmField.h"
#include "Game/Gameplay/TimingRuler.h"
#include "Game/ResourceManager.h"
#include "Game/Game.h"
#include "Game/Score/ReplayStore.h"
#include "Game/Score/ScoreStore.h"
#include "Game/Score/ResultsViewData.h"
#include "Game/Scene/SceneManager.h"
#include "Game/Scene/Scenes/ResultsOverlayScene.h"
#include "Game/Scene/Scenes/SongSelectScene.h"
#include "Game/Song/SongManager.h"
#include "Game/objects/Label.h"
#include "Game/objects/PanelRect.h"
#include "Game/objects/Sprite.h"
#include "Game/Profile.h"

namespace Game {

static std::string ResourceErrorToString(const ResourceError err) {
    switch (err) {
        case ResourceError::FileNotFound: return "FileNotFound";
        case ResourceError::InvalidFormat: return "InvalidFormat";
        case ResourceError::RendererNotInitialized: return "RendererNotInitialized";
        case ResourceError::SDLError: return "SDLError";
        case ResourceError::UnknownType: return "UnknownType";
        case ResourceError::TTFNotInitialized: return "TTFNotInitialized";
        case ResourceError::AudioNotLoaded: return "AudioNotLoaded";
        case ResourceError::MixerNotInitialized: return "MixerNotInitialized";
        default: return "UnknownError";
    }
}

namespace {

constexpr double kResultsDelaySeconds = 1.0;

// Top-left HUD stack (unit coordinates, logical 1920×1080).
constexpr UnitBounds kHudScoreBounds{.min = {.x = 0.02f, .y = 0.02f}, .max = {.x = 0.70f, .y = 0.058f}};
constexpr UnitBounds kHudJudgementsBounds{.min = {.x = 0.02f, .y = 0.058f}, .max = {.x = 0.92f, .y = 0.098f}};
constexpr UnitBounds kHudAccuracyBounds{.min = {.x = 0.02f, .y = 0.098f}, .max = {.x = 0.70f, .y = 0.138f}};
constexpr UnitBounds kHudTimingBounds{.min = {.x = 0.02f, .y = 0.138f}, .max = {.x = 0.92f, .y = 0.178f}};
constexpr UnitBounds kTimingRulerBounds{.min = {.x = 0.41f, .y = 0.88f}, .max = {.x = 0.59f, .y = 0.93f}};
// Label::Render still draws at the slot's top-left when width/height are 0, park text off-screen.
constexpr UnitBounds kOffscreenBounds{.min = {.x = 2.0f, .y = 2.0f}, .max = {.x = 2.01f, .y = 2.01f}};

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
    Gameplay::GameplaySettings settings,
    const PlayMode playMode,
    std::optional<Score::ReplayRecord> replay)
    : sceneManager(sceneManager),
      game(gameInstance),
      selectedSong(std::move(selectedSong)),
      selectedDifficultyIndex(selectedDifficultyIndex),
      settings(std::move(settings)),
      playMode(playMode) {
    if (playMode == PlayMode::Replay) {
        if (!replay.has_value()) {
            SDL_Log("GameplayScene: Replay mode requires a replay record");
            initFailed = true;
            initErrorMessage = "Missing replay data.";
            return;
        }
        this->settings.noteSpeed = replay->settings.noteSpeed;
        this->settings.crosshairRadius = replay->settings.crosshairRadius;
        this->settings.audioOffsetSeconds = replay->settings.audioOffsetSeconds;
        this->settings.swapUpDownLanes = replay->settings.swapUpDownLanes;
        this->settings.useWallClockForJudgementTiming = replay->settings.useWallClockForJudgementTiming;
        replayLogicalWidth = replay->settings.logicalWidth;
        replayLogicalHeight = replay->settings.logicalHeight;
        replayPresses = std::move(replay->presses);
        recordingEnabled = false;
        if (replay->difficultyIndex >= 0) {
            this->selectedDifficultyIndex = replay->difficultyIndex;
        }
    } else {
        recordingEnabled = true;
        recordedPresses.reserve(4096);
    }

    root->CreateChild<PanelRect>(
        UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}},
        SDL_Color{
            .r = this->settings.backgroundColorR,
            .g = this->settings.backgroundColorG,
            .b = this->settings.backgroundColorB,
            .a = 255
        });

    if (this->settings.enableBackgroundImage && this->selectedSong && !this->selectedSong->coverFile.empty()) {
        const std::filesystem::path coverPath =
            Song::SongManager::ResolveSongFile(*this->selectedSong, Utf8StringToPath(this->selectedSong->coverFile));
        if (auto coverRes = ResourceManager::getInstance().Get<SDL_Texture>(PathToUtf8String(coverPath))) {
            auto* coverSprite = root->CreateChild<Sprite>(
                UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}},
                *coverRes);
            const float opacity = std::clamp(this->settings.backgroundOpacity, 0.0f, 1.0f);
            coverSprite->SetAlpha(static_cast<Uint8>(std::lround(opacity * 255.0f)));
        } else {
            SDL_LogWarn(
                SDL_LOG_CATEGORY_APPLICATION,
                "GameplayScene: failed to load cover '%s' (%s)",
                PathToUtf8String(coverPath).c_str(),
                ResourceErrorToString(coverRes.error()).c_str());
        }
    }

    if (this->settings.enablePlayfieldBorder && this->settings.playfieldBorderSize > 0.0f &&
        this->settings.playfieldBorderOpacity > 0.0f) {
        const float t = std::clamp(this->settings.playfieldBorderSize, 0.0f, 100.0f) / 100.0f * 0.5f;
        const auto borderAlpha =
            static_cast<Uint8>(std::lround(std::clamp(this->settings.playfieldBorderOpacity, 0.0f, 1.0f) * 255.0f));
        const SDL_Color borderColor{.r = 0, .g = 0, .b = 0, .a = borderAlpha};
        // Full-width top/bottom strips, side strips between them (corners covered by top/bottom).
        root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = t}},
                                     borderColor);
        root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.0f, .y = 1.0f - t}, .max = {.x = 1.0f, .y = 1.0f}},
                                     borderColor);
        if (t < 0.5f) {
            root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 0.0f, .y = t}, .max = {.x = t, .y = 1.0f - t}},
                                         borderColor);
            root->CreateChild<PanelRect>(UnitBounds{.min = {.x = 1.0f - t, .y = t}, .max = {.x = 1.0f, .y = 1.0f - t}},
                                         borderColor);
        }
    }

    const auto titleFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 36.0f);
    const auto textFontRes = ResourceManager::getInstance().Get<TTF_Font>("04b_25/04b_25__.ttf", 24.0f);
    const auto arcTextureRes = ResourceManager::getInstance().Get<SDL_Texture>("arc-quarter.png");

    if (!titleFontRes || !textFontRes) {
        SDL_Log("GameplayScene: missing required fonts");
        initFailed = true;
        initErrorMessage = "Missing required gameplay fonts.";
        return;
    }
    if (!arcTextureRes) {
        SDL_Log("GameplayScene: missing arc-quarter.png");
        initFailed = true;
        initErrorMessage = "Missing required textures.";
        return;
    }

    if (!this->selectedSong || this->selectedSong->difficulties.empty() ||
        selectedDifficultyIndex < 0 ||
        selectedDifficultyIndex >= static_cast<int>(this->selectedSong->difficulties.size())) {
        SDL_Log("GameplayScene: invalid song / difficulty selection");
        initFailed = true;
        initErrorMessage = "Invalid song or difficulty selection.";
        return;
    }

    const auto& difficulty = this->selectedSong->difficulties[selectedDifficultyIndex];
    const std::filesystem::path chartPath =
        Song::SongManager::ResolveSongFile(*this->selectedSong, difficulty.chartPath);

    auto chartRes = Gameplay::LoadChartFromFile(chartPath);
    if (!chartRes) {
        SDL_Log("GameplayScene: failed to load chart '%s'", PathToUtf8String(chartPath).c_str());
        initFailed = true;
        initErrorMessage = "Failed to load chart file: " + PathToUtf8String(chartPath.filename());
        return;
    }

    if (this->settings.swapUpDownLanes) {
        Gameplay::ApplySwapUpDownLanes(chartRes->notes);
    }

    if (this->selectedSong->audioFile.empty()) {
        SDL_Log("GameplayScene: song has no audio file");
        initFailed = true;
        initErrorMessage = "Selected song has no audio file.";
        return;
    }

    const std::filesystem::path audioPath =
        Song::SongManager::ResolveSongFile(*this->selectedSong, Utf8StringToPath(this->selectedSong->audioFile));
    auto audioRes = ResourceManager::getInstance().Get<MIX_Audio>(PathToUtf8String(audioPath));
    if (!audioRes || !*audioRes) {
        std::string errStr = audioRes ? "Unknown" : ResourceErrorToString(audioRes.error());
        SDL_Log("GameplayScene: failed to load audio '%s' (Error: %s)", PathToUtf8String(audioPath).c_str(),
                errStr.c_str());
        initFailed = true;
        initErrorMessage = "Failed to load audio file: " + PathToUtf8String(audioPath.filename()) + " (Error: " + errStr
            + ")";
        return;
    }

    const auto& video = game.GetVideoSettings();
    int logicalW = video.logicalWidth;
    int logicalH = video.logicalHeight;
    if (playMode == PlayMode::Replay && replayLogicalWidth > 0 && replayLogicalHeight > 0) {
        logicalW = replayLogicalWidth;
        logicalH = replayLogicalHeight;
    }
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
        UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}},
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

    laneInput = root->CreateChild<Gameplay::LaneInputHandler>(
        UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 0.0f, .y = 0.0f}},
        this->settings.keyBindings);

    root->CreateChild<EscapeMenuHandler>(
        UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 0.0f, .y = 0.0f}},
        [this] { HandleEscapeKey(); });

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

void GameplayScene::OnEnter() {
    if (!SDL_HideCursor()) {
        SDL_Log("GameplayScene: failed to hide cursor on enter: %s", SDL_GetError());
    }
}

void GameplayScene::OnExit() {
    if (!SDL_ShowCursor()) {
        SDL_Log("GameplayScene: failed to show cursor on exit: %s", SDL_GetError());
    }
}

void GameplayScene::OnPause() {
    if (!SDL_ShowCursor()) {
        SDL_Log("GameplayScene: failed to show cursor on pause: %s", SDL_GetError());
    }
}

void GameplayScene::OnResume() {
    if (phase == Phase::Paused) {
        BeginResumeGrace();
        return;
    }

    if (phase == Phase::Playing || phase == Phase::ResumeGrace) {
        if (!SDL_HideCursor()) {
            SDL_Log("GameplayScene: failed to hide cursor on resume: %s", SDL_GetError());
        }
    }
}


void GameplayScene::Update(const double dt) {
    CC_PROFILE("GameplayScene.Update");
    if (initFailed) {
        ReturnToSongSelect(initErrorMessage);
        return;
    }
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
            if (playMode == PlayMode::Replay) {
                InjectReplayPresses(songTime);
            } else {
                ProcessInputs(songTime);
            }
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
        if (recordingEnabled) {
            recordedPresses.push_back(Score::ReplayPress{.t = pressSongTime, .lane = lane});
        }
        simulation.TryHit(lane, pressSongTime);
    }
}

void GameplayScene::InjectReplayPresses(const double songTimeSeconds) {
    DrainLaneInput();
    while (replayCursor < replayPresses.size() && replayPresses[replayCursor].t <= songTimeSeconds) {
        const auto& [t, lane] = replayPresses[replayCursor];
        simulation.TryHit(lane, t);
        ++replayCursor;
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
        if (result.judgement == Perfect || result.judgement == Great || result.judgement == Good || result.judgement ==
            Bad) {
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
        if (constexpr double kNxEps = 1e-7; !accuracySteps.empty() && std::abs(nx - accuracySteps.back().first) <=
            kNxEps) {
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
    case Phase::ResumeGrace:
    case Phase::Results:
        break;
    case Phase::EndDelay:
        EnterResults();
        break;
    }
}

void GameplayScene::HideHud() const {
    if (scoreLabel) scoreLabel->SetBounds(kOffscreenBounds);
    if (judgementsLabel) judgementsLabel->SetBounds(kOffscreenBounds);
    if (accuracyLabel) accuracyLabel->SetBounds(kOffscreenBounds);
    if (timingStatsLabel) timingStatsLabel->SetBounds(kOffscreenBounds);
    if (timingRuler) {
        timingRuler->SetBounds(kOffscreenBounds);
        timingRuler->Clear();
    }
}

void GameplayScene::ShowHud() const {
    if (scoreLabel) scoreLabel->SetBounds(kHudScoreBounds);
    if (judgementsLabel) judgementsLabel->SetBounds(kHudJudgementsBounds);
    if (accuracyLabel) accuracyLabel->SetBounds(kHudAccuracyBounds);
    if (timingStatsLabel) timingStatsLabel->SetBounds(kHudTimingBounds);
    if (timingRuler) timingRuler->SetBounds(kTimingRulerBounds);
}

Score::ResultsViewData GameplayScene::BuildResultsViewData() const {
    Score::ResultsViewData view;
    view.score = totalScore;
    view.accuracyPercent = AccuracyPercent();
    view.judgementCounts = judgementCounts;
    view.biasMs = MeanSignedTimingErrorMs();
    view.stdDevMs = TimingStandardDeviationMs();
    view.perfectGreatRatioText = PerfectGreatRatioText();
    view.chartDomainFirst = chartDomainFirst;
    view.chartDomainLast = chartDomainLast;
    view.graphEvents = resultsGraphEvents;
    view.accuracySteps = accuracySteps;
    if (selectedSong) {
        view.songTitle = selectedSong->title;
        if (selectedDifficultyIndex >= 0 &&
            selectedDifficultyIndex < static_cast<int>(selectedSong->difficulties.size())) {
            view.difficultyName = selectedSong->difficulties[selectedDifficultyIndex].name;
        }
    }
    return view;
}

Score::ReplaySettingsSnapshot GameplayScene::BuildReplaySettingsSnapshot() const {
    const auto& video = game.GetVideoSettings();
    return Score::ReplaySettingsSnapshot{
        .noteSpeed = settings.noteSpeed,
        .crosshairRadius = settings.crosshairRadius,
        .logicalWidth = video.logicalWidth,
        .logicalHeight = video.logicalHeight,
        .audioOffsetSeconds = settings.audioOffsetSeconds,
        .swapUpDownLanes = settings.swapUpDownLanes,
        .useWallClockForJudgementTiming = settings.useWallClockForJudgementTiming,
    };
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
    HideHud();
    phase = Phase::Paused;

    sceneManager.QueuePush<ResultsOverlayScene>(
        std::ref(sceneManager),
        std::ref(game),
        ResultsOverlayScene::Mode::Pause,
        BuildResultsViewData(),
        ResultsOverlayContext{});

    if (!SDL_ShowCursor()) {
        SDL_Log("GameplayScene: failed to show cursor on pause menu entry: %s", SDL_GetError());
    }
}

void GameplayScene::BeginResumeGrace() {
    if (phase != Phase::Paused) {
        return;
    }

    DrainLaneInput();
    ShowHud();
    resumeGraceRemaining = Gameplay::kResumeGraceSeconds;
    phase = Phase::ResumeGrace;

    if (!SDL_HideCursor()) {
        SDL_Log("GameplayScene: failed to hide cursor on resume from pause: %s", SDL_GetError());
    }
}

void GameplayScene::FinishResumeGrace() {
    if (clock) {
        clock->Resume();
    }
    phase = Phase::Playing;
}

void GameplayScene::EnterResults() {
    if (phase == Phase::Results) {
        return;
    }

    HideHud();
    phase = Phase::Results;

    const Score::ResultsViewData viewData = BuildResultsViewData();
    std::string replayId;
    if (playMode == PlayMode::Live && selectedSong) {
        if (const auto savedReplay = Score::ReplayStore::Save(
                *selectedSong,
                selectedDifficultyIndex,
                BuildReplaySettingsSnapshot(),
                recordedPresses)) {
            replayId = *savedReplay;
        }
        (void)Score::ScoreStore::Save(
            *selectedSong,
            selectedDifficultyIndex,
            viewData,
            game.GetGameplaySettings().playerName,
            replayId);
    }

    ResultsOverlayContext overlayContext;
    overlayContext.song = selectedSong;
    overlayContext.difficultyIndex = selectedDifficultyIndex;
    overlayContext.replayId = std::move(replayId);

    sceneManager.QueuePush<ResultsOverlayScene>(
        std::ref(sceneManager),
        std::ref(game),
        ResultsOverlayScene::Mode::Results,
        viewData,
        std::move(overlayContext));

    if (!SDL_ShowCursor()) {
        SDL_Log("GameplayScene: failed to show cursor on results entry: %s", SDL_GetError());
    }
}

void GameplayScene::ReturnToSongSelect(const std::string& errorMessage) const {
    if (clock) {
        clock->Stop();
    }
    sceneManager.QueueReplace<SongSelectScene>(std::ref(sceneManager), std::ref(game), errorMessage);
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
        return std::format("Ratio: -  ({} / 0)", perfect);
    }
    const double ratio = static_cast<double>(perfect) / static_cast<double>(great);
    return std::format("Ratio: {:.2f}  ({} / {})", ratio, perfect, great);
}

} // namespace Game
