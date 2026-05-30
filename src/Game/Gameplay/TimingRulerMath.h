#ifndef CC_GAME_TIMING_RULER_MATH_H
#define CC_GAME_TIMING_RULER_MATH_H

#include <algorithm>

namespace Game::Gameplay {

[[nodiscard]] constexpr double ClampDeltaMs(const double deltaMs, const double halfWindowMs) noexcept {
    return std::clamp(deltaMs, -halfWindowMs, halfWindowMs);
}

// clampedDeltaMs in [-halfWindowMs, halfWindowMs] -> [-1, 1], 0 = on time.
[[nodiscard]] constexpr float NormalizedRulerT(const double clampedDeltaMs, const double halfWindowMs) noexcept {
    if (halfWindowMs <= 0.0) {
        return 0.0f;
    }
    return static_cast<float>(clampedDeltaMs / halfWindowMs);
}

[[nodiscard]] constexpr float MarkerDisplayAlpha(
    const double ageSeconds,
    const double holdSeconds,
    const double fadeSeconds,
    const float baseAlpha) noexcept {
    if (ageSeconds <= holdSeconds) {
        return baseAlpha;
    }
    const double end = holdSeconds + fadeSeconds;
    if (ageSeconds >= end || fadeSeconds <= 0.0) {
        return 0.0f;
    }
    const double t = (end - ageSeconds) / fadeSeconds;
    return baseAlpha * static_cast<float>(t);
}

// Horizontal center of a marker, padding inset from slot edges, t E [-1, 1].
[[nodiscard]] constexpr float MarkerCenterX(
    const float slotWidth,
    const float paddingPx,
    const float normalizedT) noexcept {
    const float usable = std::max(0.0f, slotWidth - 2.0f * paddingPx);
    const float cx = paddingPx + usable * 0.5f;
    return cx + normalizedT * (usable * 0.5f);
}

} // namespace Game::Gameplay

#endif // CC_GAME_TIMING_RULER_MATH_H
