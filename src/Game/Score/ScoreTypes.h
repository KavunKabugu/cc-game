#ifndef CC_GAME_SCORE_TYPES_H
#define CC_GAME_SCORE_TYPES_H

#include <cstdint>
#include <string>

namespace Game::Score {

struct ScoreSummary {
    std::int64_t score{0};
    double accuracyPercent{0.0};
    std::string difficultyName;
    std::string statsText;
    std::int64_t timestampUnixSec{0};
};

} // namespace Game::Score

#endif // CC_GAME_SCORE_TYPES_H
