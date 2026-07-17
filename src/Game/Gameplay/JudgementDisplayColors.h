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
        return SDL_Color{.r = 70, .g = 190, .b = 255, .a = 255};
    case Great:
        return SDL_Color{.r = 100, .g = 255, .b = 100, .a = 255};
    case Good: {
        constexpr double span = std::max(1e-6, kGoodWindowMs - kGreatWindowMs);
        const double u = std::clamp((absDeltaMs - kGreatWindowMs) / span, 0.0, 1.0);
        return SDL_Color{
            .r = LerpU8(30.0f, 255.0f, static_cast<float>(u)),
            .g = LerpU8(220.0f, 230.0f, static_cast<float>(u)),
            .b = LerpU8(90.0f, 55.0f, static_cast<float>(u)),
            .a = 255,
        };
    }
    case Bad: {
        constexpr double span = std::max(1e-6, kTimingRulerHalfWindowMs - kGoodWindowMs);
        const double u = std::clamp((absDeltaMs - kGoodWindowMs) / span, 0.0, 1.0);
        return SDL_Color{
            .r = LerpU8(255.0f, 255.0f, static_cast<float>(u)),
            .g = LerpU8(145.0f, 45.0f, static_cast<float>(u)),
            .b = LerpU8(45.0f, 45.0f, static_cast<float>(u)),
            .a = 255,
        };
    }
    case Miss:
        return SDL_Color{.r = 255, .g = 255, .b = 255, .a = 0};
    default:
        return SDL_Color{.r = 255, .g = 255, .b = 255, .a = 0};;
    }
    return SDL_Color{.r = 255, .g = 255, .b = 255, .a = 0};
}

// Results accuracy graph, solid green at or above 90%, solid red at or below 50%,
// linear blend in between (matches prior flat fill at 100%, {120, 210, 160}).
[[nodiscard]] inline SDL_Color ResultsAccuracyGraphFillColor(const double accPercent) {
    const double acc = std::clamp(accPercent, 0.0, 100.0);
    constexpr SDL_Color greenAcc{.r = 120, .g = 210, .b = 160, .a = 255};
    constexpr SDL_Color redAcc{.r = 220, .g = 75, .b = 95, .a = 255};
    if (acc >= kAccuracyGraphGradientMax) {
        return greenAcc;
    }
    if (acc <= kAccuracyGraphGradientMin) {
        return redAcc;
    }
    const auto t = static_cast<float>((kAccuracyGraphGradientMax - acc) / kAccuracyGraphGradientRange);
    return SDL_Color{
        .r = LerpU8(greenAcc.r, redAcc.r, t),
        .g = LerpU8(greenAcc.g, redAcc.g, t),
        .b = LerpU8(greenAcc.b, redAcc.b, t),
        .a = 255,
    };
}

// Results delta bars, Miss uses solid colour (NoteExpired / generic miss).
[[nodiscard]] inline SDL_Color ResultsJudgementFillColor(const Judgement judgement, const double absDeltaMs) {
    using enum Judgement;
    if (judgement == Miss) {
        return SDL_Color{.r = 220, .g = 75, .b = 85, .a = 255};
    }
    return TimingRulerMarkerRgb(judgement, absDeltaMs);
}

} // namespace Game::Gameplay

#endif // CC_GAME_JUDGEMENT_DISPLAY_COLORS_H
