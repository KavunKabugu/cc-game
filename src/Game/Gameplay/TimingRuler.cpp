#include "TimingRuler.h"

#include <algorithm>
#include <cmath>

#include "JudgementDisplayColors.h"
#include "TimingRulerMath.h"

namespace Game::Gameplay {

TimingRuler::TimingRuler(const UnitBounds bounds) : Drawable(bounds) {}

ITimeAware* TimingRuler::AsTimeAware() {
    return this;
}

void TimingRuler::UpdateWithDeltaTime(const double dt) {
    if (markers.empty() || dt <= 0.0) {
        return;
    }
    constexpr double cap = kTimingRulerMarkerHoldSeconds + kTimingRulerMarkerFadeSeconds;
    for (auto& m : markers) {
        m.ageSeconds += dt;
    }
    std::erase_if(markers, [](const Marker& m) { return m.ageSeconds > cap; });
}

void TimingRuler::PushHit(const double deltaMs, const Judgement judgement) {
    markers.push_back(Marker{.deltaMs = deltaMs, .judgement = judgement, .ageSeconds = 0.0});
}

void TimingRuler::Clear() {
    markers.clear();
}

void TimingRuler::Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) {
    const SDL_FRect slotRect = {
        .x = parentRect.x + bounds.min.x * parentRect.w,
        .y = parentRect.y + bounds.min.y * parentRect.h,
        .w = (bounds.max.x - bounds.min.x) * parentRect.w,
        .h = (bounds.max.y - bounds.min.y) * parentRect.h,
    };

    if (slotRect.w <= 0.0f || slotRect.h <= 0.0f) {
        return;
    }

    const float strokePx = std::max(4.0f, slotRect.h * 0.22f);
    // Ruler track + markers use half the slider-matched stroke for a thinner look.
    const float lineW = strokePx * 0.5f;
    const float paddingPx = strokePx * 2.0f;
    const float centerY = slotRect.y + slotRect.h * 0.5f;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, 200, 215, 235, 90);
    const SDL_FRect baseline = {
        .x = slotRect.x + paddingPx,
        .y = centerY - lineW * 0.5f,
        .w = std::max(0.0f, slotRect.w - 2.0f * paddingPx),
        .h = lineW,
    };
    SDL_RenderFillRect(renderer, &baseline);

    const float notchW = std::max(lineW, 2.0f);
    const float notchH = std::min(slotRect.h * 0.55f, strokePx * 3.0f);
    SDL_SetRenderDrawColor(renderer, 240, 248, 255, 220);
    const float midX = slotRect.x + slotRect.w * 0.5f;
    const SDL_FRect notch = {
        .x = midX - notchW * 0.5f,
        .y = centerY - notchH * 0.5f,
        .w = notchW,
        .h = notchH,
    };
    SDL_RenderFillRect(renderer, &notch);

    constexpr double halfWin = kTimingRulerHalfWindowMs;

    for (const auto& [deltaMs, judgement, ageSeconds] : markers) {
        const float t =
            NormalizedRulerT(ClampDeltaMs(deltaMs, halfWin), halfWin);
        const float mx =
            MarkerCenterX(slotRect.w, paddingPx, t) + slotRect.x;
        const float alphaScale = MarkerDisplayAlpha(
            ageSeconds,
            kTimingRulerMarkerHoldSeconds,
            kTimingRulerMarkerFadeSeconds,
            kTimingRulerMarkerBaseAlpha);
        if (alphaScale <= 0.0f) {
            continue;
        }

        const auto [r, g, b, a] = TimingRulerMarkerRgb(judgement, std::abs(deltaMs));
        const auto alpha = static_cast<Uint8>(std::round(static_cast<float>(a) * alphaScale));
        SDL_SetRenderDrawColor(renderer, r, g, b, alpha);

        const float markerH = std::min(slotRect.h * 0.72f, notchH + strokePx * 2.0f);
        const SDL_FRect markerRect = {
            .x = mx - lineW * 0.5f,
            .y = centerY - markerH * 0.5f,
            .w = lineW,
            .h = markerH,
        };
        SDL_RenderFillRect(renderer, &markerRect);
    }
}

} // namespace Game::Gameplay
