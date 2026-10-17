#ifndef CC_GAME_HIT_SCORING_H
#define CC_GAME_HIT_SCORING_H

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "GameplayConstants.h"
#include "Judgement.h"

namespace Game::Gameplay {

// Zero score grace after the Good window. Bad stays at 0 until this elapses.
inline constexpr double kBadScoreGraceAfterGoodMs = 55.0;
// Good outer edge (kGoodWindowMs) + grace duration.
inline constexpr double kBadScoreGraceEndMs = kGoodWindowMs + kBadScoreGraceAfterGoodMs;

// Bad linear penalty reaches -25 at this delta, beyond that score stays at -25.
// This sits past kMissExpireMs, an unhit note still misses at 230 ms, so a live
// late hit ends on the slope and does not reach the cap.
// TODO: We might want it to reach the cap, but this works for now
inline constexpr double kBadScorePenaltyEndMs = 265.0;
static_assert(kBadScoreGraceEndMs < kMissExpireMs);
static_assert(kBadScorePenaltyEndMs > kMissExpireMs);

inline constexpr int kMissScore = -25;
inline constexpr int kBadPenaltyCap = -25;
inline constexpr int kMarvelousScore = 100;
inline constexpr int kPerfectScore = 100;
// Great falls from 100 at the Perfect edge to 70 at the Great edge.
inline constexpr double kGreatInnerScore = 100.0;
inline constexpr double kGreatOuterScore = 70.0;

// Piecewise score as a real value (no ceil). Used for the accuracy numerator and as
// the base for integer scoring on Great, Good, and Bad.
[[nodiscard]] inline double ContinuousRawScore(const HitResult& result) {
    using enum Judgement;
    switch (result.judgement) {
    case Miss:
        return kMissScore;
    case Marvelous:
        return kMarvelousScore;
    case Perfect:
        return kPerfectScore;
    case Great: {
        const double d = std::abs(result.deltaMs);
        constexpr double span = kGreatWindowMs - kPerfectWindowMs;
        const double u = std::clamp((d - kPerfectWindowMs) / span, 0.0, 1.0);
        return kGreatInnerScore + (kGreatOuterScore - kGreatInnerScore) * u;
    }
    case Good: {
        const double d = std::abs(result.deltaMs);
        constexpr double span = kGoodWindowMs - kGreatWindowMs;
        const double u = std::clamp((d - kGreatWindowMs) / span, 0.0, 1.0);
        return kGreatOuterScore * (1.0 - u);
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

// Integer score for one HitResult. Great, Good, and Bad use ceil of the continuous value.
[[nodiscard]] inline std::int32_t ScoreForHitResult(const HitResult& result) {
    using enum Judgement;
    switch (result.judgement) {
    case Miss:
        return kMissScore;
    case Marvelous:
        return kMarvelousScore;
    case Perfect:
        return kPerfectScore;
    case Great:
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
