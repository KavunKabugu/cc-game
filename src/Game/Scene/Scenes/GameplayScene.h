#ifndef CC_GAME_GAMEPLAY_SCENE_H
#define CC_GAME_GAMEPLAY_SCENE_H

#include <array>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "Game/Gameplay/GameplaySettings.h"
#include "Game/Gameplay/NoteSimulation.h"
#include "Game/Gameplay/ResultsGraphDisplay.h"
#include "Game/Gameplay/SongClock.h"
#include "Game/Scene/SceneBase.h"
#include "Game/Song/SongTypes.h"

namespace Game {

class GameInstance;
class Label;
class PanelRect;
class SceneManager;
class Sound;
class TextButton;

namespace Gameplay {
class LaneInputHandler;
class RhythmField;
class TimingRuler;
} // namespace Gameplay

class GameplayScene final : public SceneBase {
public:
    GameplayScene(
        SceneManager& sceneManager,
        GameInstance& gameInstance,
        std::shared_ptr<Song::SongMetadata> selectedSong,
        int selectedDifficultyIndex,
        Gameplay::GameplaySettings settings = {});
    ~GameplayScene() override;

    void Update(double dt) override;

private:
    enum class Phase {
        Playing,
        EndDelay,
        Results,
    };

    void ProcessInputs(double songTimeSeconds);
    void DrainLaneInput() const;
    void ConsumeJudgements();
    void UpdateHud();
    void HandleSongEnd();
    void EnterResults();
    void ReturnToSongSelect() const;

    [[nodiscard]] double AccuracyPercent() const;
    // Mean (t_input - t_note) in ms over hit judgements only, 0 when none.
    [[nodiscard]] double MeanSignedTimingErrorMs() const;
    // Population deviation of timing errors over hit judgements, 0 when none or one hit.
    [[nodiscard]] double TimingStandardDeviationMs() const;

    SceneManager& sceneManager;
    GameInstance& game;
    std::shared_ptr<Song::SongMetadata> selectedSong;
    int selectedDifficultyIndex = -1;
    Gameplay::GameplaySettings settings;

    std::unique_ptr<Gameplay::SongClock> clock;
    Gameplay::NoteSimulation simulation;
    Gameplay::LaneInputHandler* laneInput = nullptr;
    Gameplay::RhythmField* rhythmField = nullptr;
    Gameplay::TimingRuler* timingRuler = nullptr;

    Label* scoreLabel = nullptr;
    Label* judgementsLabel = nullptr;
    Label* accuracyLabel = nullptr;
    Label* timingStatsLabel = nullptr;

    PanelRect* dimLayer = nullptr;
    Gameplay::ResultsGraphDisplay* resultsGraphDisplay = nullptr;
    TextButton* graphToggleButton = nullptr;
    Label* resultScoreLabel = nullptr;
    Label* resultAccuracyLabel = nullptr;
    Label* resultJudgementsLabel = nullptr;
    Label* resultBiasLabel = nullptr;
    Label* resultStdDevLabel = nullptr;
    TextButton* resultsBackButton = nullptr;

    std::array<int, 4> judgementCounts{}; // Perfect, Good, Bad, Miss.
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

    Phase phase = Phase::Playing;
    double resultsDelayRemaining = 0.0;
    bool simulationReady = false;
};

} // namespace Game

#endif // CC_GAME_GAMEPLAY_SCENE_H
