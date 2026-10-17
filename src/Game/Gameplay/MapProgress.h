#ifndef CC_GAME_MAP_PROGRESS_H
#define CC_GAME_MAP_PROGRESS_H

#include <algorithm>
#include <cmath>
#include <format>
#include <string>

#include "GameplayConstants.h"

namespace Game::Gameplay {

// Song time when an unhit chart leaves Phase::Playing, one miss expire past the
// last note. Empty or non-finite charts have no playable length.
// Who would ever play a non-finite chart? :clueless:
[[nodiscard]] inline double MapEndSongTime(const double lastNoteHitTime) noexcept {
    if (!std::isfinite(lastNoteHitTime)) {
        return 0.0;
    }
    const double end = lastNoteHitTime + kMissExpireSeconds;
    return end > 0.0 ? end : 0.0;
}

struct MapProgressSample {
    double playedSeconds = 0.0;
    double remainingSeconds = 0.0;
    float fraction = 1.0f;
};

// gameplayFinished snaps the readout when Playing has already ended early.
[[nodiscard]] inline MapProgressSample SampleMapProgress(
    const double songTimeSeconds,
    const double mapEndSongTime,
    const bool gameplayFinished) noexcept {
    if (gameplayFinished || !(mapEndSongTime > 0.0)) {
        return MapProgressSample{
            .playedSeconds = mapEndSongTime > 0.0 ? mapEndSongTime : 0.0,
            .remainingSeconds = 0.0,
            .fraction = 1.0f,
        };
    }

    const double played = std::clamp(songTimeSeconds, 0.0, mapEndSongTime);
    return MapProgressSample{
        .playedSeconds = played,
        .remainingSeconds = mapEndSongTime - played,
        .fraction = static_cast<float>(played / mapEndSongTime),
    };
}

// Whole seconds rounded up, so the countdown reaches 00:00 only at zero.
// mm:ss until an hour remains, then hh:mm:ss.
[[nodiscard]] inline std::string FormatRemainingTime(const double remainingSeconds) {
    if (!(remainingSeconds > 0.0) || !std::isfinite(remainingSeconds)) {
        return "00:00";
    }

    const double ceiled = std::ceil(remainingSeconds - 1e-9);
    auto total = static_cast<long long>(ceiled);
    if (total < 0) {
        total = 0;
    }

    const long long hours = total / 3600;
    const long long minutes = total % 3600 / 60;
    const long long seconds = total % 60;
    if (hours > 0) {
        return std::format("{:02}:{:02}:{:02}", hours, minutes, seconds);
    }
    return std::format("{:02}:{:02}", minutes, seconds);
}

} // namespace Game::Gameplay

#endif // CC_GAME_MAP_PROGRESS_H
