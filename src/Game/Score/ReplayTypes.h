#ifndef CC_GAME_REPLAY_TYPES_H
#define CC_GAME_REPLAY_TYPES_H

#include <cstdint>
#include <string>
#include <vector>

namespace Game::Score {

inline constexpr int kReplaySchemaVersion = 1;

struct ReplayPress {
    double t{0.0}; // Chart-relative seconds as passed to TryHit.
    int lane{-1};
};

struct ReplaySettingsSnapshot {
    float noteSpeed{0.0f};
    float crosshairRadius{0.0f};
    int logicalWidth{0};
    int logicalHeight{0};
    double audioOffsetSeconds{0.0};
    bool swapUpDownLanes{false};
    bool useWallClockForJudgementTiming{true};
};

struct ReplayRecord {
    std::string replayId;
    std::string songFolder;
    std::string chartPath;
    int difficultyIndex{-1};
    std::string difficultyName;
    std::int64_t recordedAtUnixSec{0};
    ReplaySettingsSnapshot settings{};
    std::vector<ReplayPress> presses;
};

} // namespace Game::Score

#endif // CC_GAME_REPLAY_TYPES_H
