#ifndef CC_GAME_GAMEPLAY_SCENE_H
#define CC_GAME_GAMEPLAY_SCENE_H

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Game/Gameplay/GameplaySettings.h"
#include "Game/Gameplay/NoteSimulation.h"
#include "Game/Gameplay/ResultsGraphDisplay.h"
#include "Game/Gameplay/SongClock.h"
#include "Game/Score/ReplayTypes.h"
#include "Game/Score/ResultsViewData.h"
#include "Game/Scene/SceneBase.h"
#include "Game/Song/SongTypes.h"

namespace Game {

class GameInstance;
class Label;
class SceneManager;

namespace Gameplay {
class LaneInputHandler;
class RhythmField;
class TimingRuler;
} // namespace Gameplay

class GameplayScene final : public SceneBase {
public:
    enum class PlayMode {
        Live,
        Replay,
    };

    GameplayScene(
        SceneManager& sceneManager,
        GameInstance& gameInstance,
        std::shared_ptr<Song::SongMetadata> selectedSong,
        int selectedDifficultyIndex,
        Gameplay::GameplaySettings settings = {},
        PlayMode playMode = PlayMode::Live,
        std::optional<Score::ReplayRecord> replay = std::nullopt);
    ~GameplayScene() override;

    void OnEnter() override;
    void OnExit() override;
    void OnPause() override;
    void OnResume() override;

    void Update(double dt) override;

private:
    enum class Phase {
        Playing,
        Paused,
        ResumeGrace,
        EndDelay,
        Results,
    };

    enum class EndReason {
        Completed,
        Stopped,
    };

    void ProcessInputs(double songTimeSeconds);
    void InjectReplayPresses(double songTimeSeconds);
    void DrainLaneInput() const;
    void ConsumeJudgements();
    void UpdateHud();
    void HandleSongEnd();
    void HandleEscapeKey();
    void ShowHud() const;
    void HideHud() const;
    void EnterPaused();
    void BeginResumeGrace();
    void FinishResumeGrace();
    void EnterResults();
    void ReturnToSongSelect(const std::string& errorMessage = "") const;

    [[nodiscard]] Score::ResultsViewData BuildResultsViewData() const;
    [[nodiscard]] Score::ReplaySettingsSnapshot BuildReplaySettingsSnapshot() const;
    [[nodiscard]] double AccuracyPercent() const;
    // Mean (t_input - t_note) in ms over hit judgements only, 0 when none.
    [[nodiscard]] double MeanSignedTimingErrorMs() const;
    // Population deviation of timing errors over hit judgements, 0 when none or one hit.
    [[nodiscard]] double TimingStandardDeviationMs() const;
    [[nodiscard]] std::string PerfectGreatRatioText() const;

    SceneManager& sceneManager;
    GameInstance& game;
    std::shared_ptr<Song::SongMetadata> selectedSong;
    int selectedDifficultyIndex = -1;
    Gameplay::GameplaySettings settings;
    PlayMode playMode = PlayMode::Live;

    std::unique_ptr<Gameplay::SongClock> clock;
    Gameplay::NoteSimulation simulation;
    Gameplay::LaneInputHandler* laneInput = nullptr;
    Gameplay::RhythmField* rhythmField = nullptr;
    Gameplay::TimingRuler* timingRuler = nullptr;

    Label* scoreLabel = nullptr;
    Label* judgementsLabel = nullptr;
    Label* accuracyLabel = nullptr;
    Label* timingStatsLabel = nullptr;

    std::array<int, static_cast<int>(Gameplay::Judgement::Count)> judgementCounts{}; // Perfect, Great, Good, Bad, Miss.
    std::int64_t totalScore = 0;

    std::size_t chartNoteCount = 0;
    std::uint64_t chartNotesProcessed = 0; // Resolutions tied to chart notes (excludes EmptyLane).
    double sumAccuracyNumerator = 0.0;
    std::uint64_t timingHitCount = 0;
    double sumTimingDeltaMs = 0.0;
    double sumTimingDeltaSqMs = 0.0;

    double chartDomainFirst = 0.0;
    double chartDomainLast = 0.0;
    std::vector<Gameplay::ResultsGraphEvent> resultsGraphEvents;
    std::vector<std::pair<double, double>> accuracySteps;

    bool recordingEnabled = false;
    std::vector<Score::ReplayPress> recordedPresses;
    std::vector<Score::ReplayPress> replayPresses;
    std::size_t replayCursor = 0;
    int replayLogicalWidth = 0;
    int replayLogicalHeight = 0;

    Phase phase = Phase::Playing;
    EndReason endReason = EndReason::Completed;
    double resultsDelayRemaining = 0.0;
    double resumeGraceRemaining = 0.0;
    double frozenSongTime = 0.0;
    bool sessionEnded = false;
    bool simulationReady = false;
    bool initFailed = false;
    std::string initErrorMessage;
};

} // namespace Game

#endif // CC_GAME_GAMEPLAY_SCENE_H
