//
// Created by karp on 14/04/2026.
//

#include "Label.h"

#include <algorithm>
#include <cmath>

#include "Game/Profile.h"

namespace Game {
namespace {
constexpr float kMarqueeSpeedPxPerSec = 60.0f;
constexpr float kMarqueeEndPauseSec = 0.75f;
constexpr float kMarqueeGapPx = 32.0f;
} // namespace

Label::Label(const UnitBounds bounds, std::shared_ptr<TTF_Font> font, std::string text, const SDL_Color color)
    : Drawable(bounds), text(std::move(text)), color(color), font(std::move(font)) {}

void Label::SetText(const std::string_view newText) {
    if (text != newText) {
        text = newText;
        dirty = true;
        ResetMarquee();
    }
}

void Label::SetColor(const SDL_Color newColor) {
    if (color.r != newColor.r || color.g != newColor.g || color.b != newColor.b || color.a != newColor.a) {
        color = newColor;
        dirty = true;
    }
}

void Label::SetAlignment(const HorizontalAlignment h, const VerticalAlignment v) {
    hAlign = h;
    vAlign = v;
}

void Label::SetOverflowMode(const LabelOverflowMode mode) {
    if (overflowMode == mode) {
        return;
    }
    overflowMode = mode;
    ResetMarquee();
}

void Label::SetMarqueeActive(const bool active) {
    if (marqueeActive == active) {
        return;
    }
    marqueeActive = active;
    if (!marqueeActive) {
        ResetMarquee();
    } else {
        marqueePauseRemaining = kMarqueeEndPauseSec;
        marqueeReturning = false;
        marqueeOffsetPx = 0.0f;
    }
}

void Label::ResetMarquee() {
    marqueeOffsetPx = 0.0f;
    marqueePauseRemaining = marqueeActive ? kMarqueeEndPauseSec : 0.0f;
    marqueeReturning = false;
}

void Label::UpdateWithDeltaTime(const double dt) {
    if (overflowMode != LabelOverflowMode::Marquee || !marqueeActive || textW <= 0) {
        return;
    }

    const float overflow = static_cast<float>(textW) - lastSlotWidthPx;
    if (overflow <= 0.0f || lastSlotWidthPx <= 0.0f) {
        marqueeOffsetPx = 0.0f;
        return;
    }

    const float maxOffset = overflow + kMarqueeGapPx;
    const float clampedDt = static_cast<float>(std::clamp(dt, 0.0, 0.1));

    if (marqueePauseRemaining > 0.0f) {
        marqueePauseRemaining -= clampedDt;
        return;
    }

    if (!marqueeReturning) {
        marqueeOffsetPx += kMarqueeSpeedPxPerSec * clampedDt;
        if (marqueeOffsetPx >= maxOffset) {
            marqueeOffsetPx = maxOffset;
            marqueeReturning = true;
            marqueePauseRemaining = kMarqueeEndPauseSec;
        }
    } else {
        marqueeOffsetPx -= kMarqueeSpeedPxPerSec * clampedDt;
        if (marqueeOffsetPx <= 0.0f) {
            marqueeOffsetPx = 0.0f;
            marqueeReturning = false;
            marqueePauseRemaining = kMarqueeEndPauseSec;
        }
    }
}

void Label::RebuildTexture(SDL_Renderer* renderer) {
    CC_PROFILE("Label.RebuildTexture");
    if (!font || text.empty()) {
        textTexture.reset();
        textW = 0;
        textH = 0;
        dirty = false;
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font.get(), text.c_str(), 0, color);
    if (!surface) {
        dirty = false;
        return;
    }

    textW = surface->w;
    textH = surface->h;

    SDL_Texture* rawTex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (rawTex) {
        // This will automatically destroy the old texture if it existed
        textTexture = std::shared_ptr<SDL_Texture>(rawTex, [](SDL_Texture* t) {
            SDL_DestroyTexture(t);
        });
    }

    dirty = false;
}

void Label::Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) {
    if (dirty) {
        RebuildTexture(renderer);
    }

    if (!textTexture) {
        return;
    }

    // Calculate the available area in pixels
    const SDL_FRect slotRect = {
        .x = parentRect.x + bounds.min.x * parentRect.w,
        .y = parentRect.y + bounds.min.y * parentRect.h,
        .w = (bounds.max.x - bounds.min.x) * parentRect.w,
        .h = (bounds.max.y - bounds.min.y) * parentRect.h
    };

    if (slotRect.w <= 0.0f || slotRect.h <= 0.0f) {
        return;
    }

    lastSlotWidthPx = slotRect.w;

    // Align the text within that slot
    SDL_FRect destRect = {.x = 0, .y = 0, .w = static_cast<float>(textW), .h = static_cast<float>(textH)};

    const bool useMarquee = overflowMode == LabelOverflowMode::Marquee && marqueeActive &&
                            static_cast<float>(textW) > slotRect.w;

    if (useMarquee) {
        destRect.x = slotRect.x - marqueeOffsetPx;
    } else {
        switch (hAlign) {
        case HorizontalAlignment::Left:
            destRect.x = slotRect.x;
            break;
        case HorizontalAlignment::Center:
            destRect.x = slotRect.x + (slotRect.w - static_cast<float>(textW)) / 2.0f;
            break;
        case HorizontalAlignment::Right:
            destRect.x = slotRect.x + (slotRect.w - static_cast<float>(textW));
            break;
        }
    }

    switch (vAlign) {
    case VerticalAlignment::Top:
        destRect.y = slotRect.y;
        break;
    case VerticalAlignment::Middle:
        destRect.y = slotRect.y + (slotRect.h - static_cast<float>(textH)) / 2.0f;
        break;
    case VerticalAlignment::Bottom:
        destRect.y = slotRect.y + (slotRect.h - static_cast<float>(textH));
        break;
    }

    SDL_Rect oldClip{.x = 0, .y = 0, .w = 0, .h = 0};
    const bool hadClip = SDL_RenderClipEnabled(renderer);
    if (hadClip) {
        SDL_GetRenderClipRect(renderer, &oldClip);
    }

    SDL_Rect clipRect = {
        .x = static_cast<int>(std::lround(slotRect.x)),
        .y = static_cast<int>(std::lround(slotRect.y)),
        .w = static_cast<int>(std::lround(slotRect.w)),
        .h = static_cast<int>(std::lround(slotRect.h))
    };

    // Nested clips (e.g. ScrollContainer viewport) must be intersected,
    // otherwise partially scrolled rows draw text outside the list panel.
    if (hadClip) {
        const int x1 = std::max(clipRect.x, oldClip.x);
        const int y1 = std::max(clipRect.y, oldClip.y);
        const int x2 = std::min(clipRect.x + clipRect.w, oldClip.x + oldClip.w);
        const int y2 = std::min(clipRect.y + clipRect.h, oldClip.y + oldClip.h);
        if (x2 <= x1 || y2 <= y1) {
            return;
        }
        clipRect = {.x = x1, .y = y1, .w = x2 - x1, .h = y2 - y1};
    }

    SDL_SetRenderClipRect(renderer, &clipRect);
    SDL_RenderTexture(renderer, textTexture.get(), nullptr, &destRect);
    SDL_SetRenderClipRect(renderer, hadClip ? &oldClip : nullptr);
}

} // namespace Game
