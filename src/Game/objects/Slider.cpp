#include "Slider.h"

#include <algorithm>
#include <cmath>

#include "Game/EventManager.h"

namespace Game {

namespace {

constexpr float kThumbHalfNorm = 0.02f;

[[nodiscard]] float ThumbCenterNorm(const float t) {
    return kThumbHalfNorm + std::clamp(t, 0.0f, 1.0f) * (1.0f - 2.0f * kThumbHalfNorm);
}

} // namespace

Slider::Slider(const UnitBounds bounds, const float minV, const float maxV, const float stepV, const float initialValue)
    : Drawable(bounds),
      minValue(minV),
      maxValue(maxV),
      step(stepV) {
    SetValue(initialValue);
}

void Slider::SetColors(const SDL_Color track, const SDL_Color thumb, const SDL_Color thumbHover) {
    trackColor = track;
    thumbColor = thumb;
    thumbHoverColor = thumbHover;
}

void Slider::SetOnChanged(std::function<void(float)> callback) {
    onChanged = std::move(callback);
}

void Slider::SetValue(const float v) {
    const float snapped = std::round(v / step) * step;
    const float clamped = std::clamp(snapped, minValue, maxValue);
    if (clamped == value) {
        return;
    }
    value = clamped;
    if (onChanged) {
        onChanged(value);
    }
}

float Slider::NormalizedFromLocalX(const float localXNorm) {
    constexpr float usableMin = kThumbHalfNorm;
    constexpr float usableMax = 1.0f - kThumbHalfNorm;
    constexpr float denom = std::max(usableMax - usableMin, 1e-6f);
    const float t = (localXNorm - usableMin) / denom;
    return std::clamp(t, 0.0f, 1.0f);
}

void Slider::ApplyLocalToValue(const UnitPoint localPos) {
    const float t = NormalizedFromLocalX(localPos.x);
    SetValue(minValue + t * (maxValue - minValue));
}

void Slider::Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) {
    const SDL_FRect rect = {
        .x = parentRect.x + bounds.min.x * parentRect.w,
        .y = parentRect.y + bounds.min.y * parentRect.h,
        .w = (bounds.max.x - bounds.min.x) * parentRect.w,
        .h = (bounds.max.y - bounds.min.y) * parentRect.h,
    };

    if (rect.w <= 0.0f || rect.h <= 0.0f) {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, trackColor.r, trackColor.g, trackColor.b, trackColor.a);
    const float trackH = std::max(4.0f, rect.h * 0.22f);
    const SDL_FRect trackRect = {
        .x = rect.x,
        .y = rect.y + (rect.h - trackH) * 0.5f,
        .w = rect.w,
        .h = trackH,
    };
    SDL_RenderFillRect(renderer, &trackRect);

    const float tNorm =
        maxValue > minValue ? (value - minValue) / (maxValue - minValue) : 0.0f;
    const float thumbCenterNorm = ThumbCenterNorm(tNorm);
    const float thumbRadiusPx = std::max(6.0f, rect.w * kThumbHalfNorm);
    const float cx = rect.x + thumbCenterNorm * rect.w;

    const auto&[r, g, b, a] = hovered ? thumbHoverColor : thumbColor;
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    const SDL_FRect thumbDst = {
        .x = cx - thumbRadiusPx,
        .y = rect.y + rect.h * 0.5f - thumbRadiusPx,
        .w = thumbRadiusPx * 2.0f,
        .h = thumbRadiusPx * 2.0f,
    };
    SDL_RenderFillRect(renderer, &thumbDst);
}

bool Slider::OnMouseButtonDown(const int button, const UnitPoint localPos) {
    if (button != SDL_BUTTON_LEFT) {
        return false;
    }
    ApplyLocalToValue(localPos);
    EventManager::getInstance().BeginDragCapture(this, button);
    (void)OnDragStart(button, localPos);
    return true;
}

bool Slider::OnMouseButtonUp(const int button, const UnitPoint localPos) {
    (void)button;
    (void)localPos;
    return false;
}

void Slider::OnMouseButtonClicked(const int button, const UnitPoint localPos) {
    (void)button;
    (void)localPos;
}

void Slider::OnMouseEnter() {
    hovered = true;
}

void Slider::OnMouseLeave() {
    hovered = false;
}

void Slider::OnMouseMove(const UnitPoint localPos) {
    (void)localPos;
}

bool Slider::OnDragStart(const int button, const UnitPoint localPos) {
    (void)button;
    ApplyLocalToValue(localPos);
    return true;
}

void Slider::OnDragMove(const UnitPoint localPos) {
    ApplyLocalToValue(localPos);
}

void Slider::OnDragEnd(const int button, const UnitPoint localPos) {
    (void)button;
    ApplyLocalToValue(localPos);
}

} // namespace Game
