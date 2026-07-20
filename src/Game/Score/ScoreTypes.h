#ifndef CC_GAME_SCORE_TYPES_H
#define CC_GAME_SCORE_TYPES_H

#include <array>
#include <string>

#include "Game/Gameplay/Judgement.h"
#include "Game/Gameplay/ResultsGraphDisplay.h"
#include "Game/Score/ResultsViewData.h"

namespace Game::Score {

inline constexpr int kScoreSchemaVersion = 1;

struct ScoreSummary {
    std::string runId;
    std::string songFolder;
    std::string songTitle;
    std::string songArtist;
    std::string chartPath;
    std::string difficultyName;
    std::string playerName;
    std::int64_t timestampUnixSec{0};

    std::int64_t score{0};
    double accuracyPercent{0.0};
    std::array<int, static_cast<int>(Gameplay::Judgement::Count)> judgementCounts{};
    double biasMs{0.0};
    double stdDevMs{0.0};
    std::string perfectGreatRatioText;
    std::string replayId;
};

struct ScoreRecord {
    ScoreSummary summary;
    ResultsViewData detail;
};

} // namespace Game::Score

#endif // CC_GAME_SCORE_TYPES_H
