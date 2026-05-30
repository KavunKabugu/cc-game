//
// Created by karp on 14/04/2026.
//

#include "Label.h"

namespace Game {

Label::Label(const UnitBounds bounds, std::shared_ptr<TTF_Font> font, std::string text, const SDL_Color color)
    : Drawable(bounds), text(std::move(text)), color(color), font(std::move(font)) {}

void Label::SetText(const std::string_view newText) {
    if (text != newText) {
        text = newText;
        dirty = true;
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

void Label::RebuildTexture(SDL_Renderer* renderer) {
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

    if (!textTexture) return;

    // Calculate the available area in pixels
    const SDL_FRect slotRect = {
        parentRect.x + (bounds.min.x * parentRect.w),
        parentRect.y + (bounds.min.y * parentRect.h),
        (bounds.max.x - bounds.min.x) * parentRect.w,
        (bounds.max.y - bounds.min.y) * parentRect.h
    };

    // Align the text within that slot
    SDL_FRect destRect = { 0, 0, static_cast<float>(textW), static_cast<float>(textH) };

    switch (hAlign) {
        case HorizontalAlignment::Left:   destRect.x = slotRect.x; break;
        case HorizontalAlignment::Center: destRect.x = slotRect.x + (slotRect.w - static_cast<float>(textW)) / 2.0f; break;
        case HorizontalAlignment::Right:  destRect.x = slotRect.x + (slotRect.w - static_cast<float>(textW)); break;
    }

    switch (vAlign) {
        case VerticalAlignment::Top:    destRect.y = slotRect.y; break;
        case VerticalAlignment::Middle: destRect.y = slotRect.y + (slotRect.h - static_cast<float>(textH)) / 2.0f; break;
        case VerticalAlignment::Bottom: destRect.y = slotRect.y + (slotRect.h - static_cast<float>(textH)); break;
    }

    SDL_RenderTexture(renderer, textTexture.get(), nullptr, &destRect);
}

} // namespace Game
