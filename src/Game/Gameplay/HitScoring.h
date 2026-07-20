#ifndef CC_GAME_HIT_SCORING_H
#define CC_GAME_HIT_SCORING_H

#include <algorithm>
#include <cmath>

#include "GameplayConstants.h"
#include "Judgement.h"

namespace Game::Gameplay {

// Timing derived from judgement windows plus a 10 ms zero-score grace after Good.
inline constexpr double kBadScoreGraceAfterGoodMs = 10.0;
// Good outer edge (kGoodWindowMs) + grace duration.
inline constexpr double kBadScoreGraceEndMs = kGoodWindowMs + kBadScoreGraceAfterGoodMs;

// Bad linear penalty reaches -25 at this delta, beyond that score stays at -25.
// Align with note hittable window so scoring matches gameplay timing.
inline constexpr double kBadScorePenaltyEndMs = kMissExpireMs;

inline constexpr int kMissScore = -25;
inline constexpr int kBadPenaltyCap = -25;
inline constexpr int kPerfectScore = 100;
inline constexpr int kGreatScore = 100;

// Piecewise score as a real value (no ceil). Used for accuracy numerator and as
// the base for integer scoring on Good/Bad.
[[nodiscard]] inline double ContinuousRawScore(const HitResult& result) {
    using enum Judgement;
    switch (result.judgement) {
    case Miss:
        return kMissScore;
    case Perfect:
        return kPerfectScore;
    case Great: {
        return kGreatScore;
    }
    case Good: {
        const double d = std::abs(result.deltaMs);
        return 100.0 * (kGoodWindowMs - d) / (kGoodWindowMs - kGreatWindowMs);
    }
    case Bad: {
        const double d = std::abs(result.deltaMs);
        if (d <= kBadScoreGraceEndMs) {
            return 0.0;
        }
        if (d < kBadScorePenaltyEndMs) {
            return -25.0 * (d - kBadScoreGraceEndMs) /
                   (kBadScorePenaltyEndMs - kBadScoreGraceEndMs);
        }
        return kBadPenaltyCap;
    }
    default: {
            return kMissScore;
        }
    }
    // We should never land here, just in case
    return kMissScore;
}

// Accuracy numerator contribution: max(0, continuous score). EmptyLane presses
// do not correspond to a chart note and do not affect accuracy (0, not −25).
[[nodiscard]] inline double AccuracyNumeratorContribution(const HitResult& result) {
    if (result.missReason == MissReason::EmptyLane) {
        return 0.0;
    }
    return std::max(0.0, ContinuousRawScore(result));
}

// Integer score for one HitResult. Good/Bad use ceil of the continuous value.
[[nodiscard]] inline std::int32_t ScoreForHitResult(const HitResult& result) {
    using enum Judgement;
    switch (result.judgement) {
    case Miss:
        return kMissScore;
    case Perfect:
        return kPerfectScore;
    case Great:
        return kGreatScore;
    case Good:
    case Bad:
        return static_cast<std::int32_t>(std::ceil(ContinuousRawScore(result)));
    default:
        return kMissScore;
    }
    // We should never land here, just in case
    return kMissScore;
}

} // namespace Game::Gameplay

#endif // CC_GAME_HIT_SCORING_H
