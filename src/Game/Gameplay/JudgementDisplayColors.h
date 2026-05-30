#ifndef CC_GAME_JUDGEMENT_DISPLAY_COLORS_H
#define CC_GAME_JUDGEMENT_DISPLAY_COLORS_H

#include <algorithm>
#include <cmath>

#include <SDL3/SDL_pixels.h>

#include "GameplayConstants.h"
#include "Judgement.h"

namespace Game::Gameplay {

[[nodiscard]] inline Uint8 LerpU8(const float a, const float b, const float t) {
    return static_cast<Uint8>(std::round(a + (b - a) * std::clamp(t, 0.0f, 1.0f)));
}

// Bottom timing strip, Miss is invisible (alpha 0).
[[nodiscard]] inline SDL_Color TimingRulerMarkerRgb(const Judgement judgement, const double absDeltaMs) {
    using enum Judgement;
    switch (judgement) {
    case Perfect:
        return SDL_Color{70, 190, 255, 255};
    case Good: {
        constexpr double span = std::max(1e-6, kGoodWindowMs - kPerfectWindowMs);
        const double u = std::clamp((absDeltaMs - kPerfectWindowMs) / span, 0.0, 1.0);
        return SDL_Color{
            LerpU8(30.0f, 255.0f, static_cast<float>(u)),
            LerpU8(220.0f, 230.0f, static_cast<float>(u)),
            LerpU8(90.0f, 55.0f, static_cast<float>(u)),
            255,
        };
    }
    case Bad: {
        constexpr double span = std::max(1e-6, kTimingRulerHalfWindowMs - kGoodWindowMs);
        const double u = std::clamp((absDeltaMs - kGoodWindowMs) / span, 0.0, 1.0);
        return SDL_Color{
            LerpU8(255.0f, 255.0f, static_cast<float>(u)),
            LerpU8(145.0f, 45.0f, static_cast<float>(u)),
            LerpU8(45.0f, 45.0f, static_cast<float>(u)),
            255,
        };
    }
    case Miss:
        return SDL_Color{255, 255, 255, 0};
    }
    return SDL_Color{255, 255, 255, 0};
}

// Results accuracy graph, solid green at or above 90%, solid red at or below 50%,
// linear blend in between (matches prior flat fill at 100%, {120, 210, 160}).
[[nodiscard]] inline SDL_Color ResultsAccuracyGraphFillColor(const double accPercent) {
    const double acc = std::clamp(accPercent, 0.0, 100.0);
    constexpr SDL_Color greenAcc{120, 210, 160, 255};
    constexpr SDL_Color redAcc{220, 75, 95, 255};
    if (acc >= kAccuracyGraphGradientMax) {
        return greenAcc;
    }
    if (acc <= kAccuracyGraphGradientMin) {
        return redAcc;
    }
    const auto t = static_cast<float>((kAccuracyGraphGradientMax - acc) / kAccuracyGraphGradientRange);
    return SDL_Color{
        LerpU8(greenAcc.r, redAcc.r, t),
        LerpU8(greenAcc.g, redAcc.g, t),
        LerpU8(greenAcc.b, redAcc.b, t),
        255,
    };
}

// Results delta bars, Miss uses solid colour (NoteExpired / generic miss).
[[nodiscard]] inline SDL_Color ResultsJudgementFillColor(const Judgement judgement, const double absDeltaMs) {
    using enum Judgement;
    if (judgement == Miss) {
        return SDL_Color{220, 75, 85, 255};
    }
    return TimingRulerMarkerRgb(judgement, absDeltaMs);
}

} // namespace Game::Gameplay

#endif // CC_GAME_JUDGEMENT_DISPLAY_COLORS_H
