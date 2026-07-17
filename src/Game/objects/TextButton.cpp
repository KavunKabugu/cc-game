#include "TextButton.h"

#include <utility>

namespace Game {

TextButton::TextButton(
    const UnitBounds bounds,
    std::shared_ptr<TTF_Font> font,
    std::string text,
    std::function<void()> onClick,
    std::shared_ptr<SDL_Texture> backgroundTexture
)
    : Drawable(bounds),
      font(std::move(font)),
      text(std::move(text)),
      backgroundTexture(std::move(backgroundTexture)),
      onClick(std::move(onClick)) {}

void TextButton::Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) {
    const SDL_FRect rect = {
        .x = parentRect.x + bounds.min.x * parentRect.w,
        .y = parentRect.y + bounds.min.y * parentRect.h,
        .w = (bounds.max.x - bounds.min.x) * parentRect.w,
        .h = (bounds.max.y - bounds.min.y) * parentRect.h
    };

    const SDL_Color activeNormal = selected ? selectedNormalColor : normalColor;
    const SDL_Color activeHover = selected ? selectedHoverColor : hoverColor;
    const SDL_Color activePressed = selected ? selectedPressedColor : pressedColor;
    const auto [r, g, b, a] = !enabled ? SDL_Color{.r = 80, .g = 80, .b = 80, .a = 255}
                                  : pressed ? activePressed : hovered ? activeHover : activeNormal;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderFillRect(renderer, &rect);

    if (backgroundTexture) {
        // Tint button texture with current state color so selected/hover/pressed
        // visual states remain visible even when a background sprite is used.
        SDL_SetTextureColorMod(backgroundTexture.get(), r, g, b);
        SDL_SetTextureAlphaMod(backgroundTexture.get(), a);
        SDL_RenderTexture(renderer, backgroundTexture.get(), nullptr, &rect);
    }

    if (textDirty) {
        RebuildTextTexture(renderer);
    }

    if (!textTexture) {
        return;
    }

    const SDL_FRect textRect = {
        .x = rect.x + (rect.w - static_cast<float>(textW)) * 0.5f,
        .y = rect.y + (rect.h - static_cast<float>(textH)) * 0.5f,
        .w = static_cast<float>(textW),
        .h = static_cast<float>(textH)
    };
    SDL_RenderTexture(renderer, textTexture.get(), nullptr, &textRect);
}

bool TextButton::OnMouseButtonDown(const int button, const UnitPoint localPos) {
    (void)localPos;
    if (!enabled || button != SDL_BUTTON_LEFT) {
        return false;
    }

    pressed = true;
    return true;
}

bool TextButton::OnMouseButtonUp(const int button, const UnitPoint localPos) {
    (void)localPos;
    if (button != SDL_BUTTON_LEFT) {
        return false;
    }

    const bool wasPressed = pressed;
    pressed = false;
    return wasPressed;
}

void TextButton::OnMouseButtonClicked(const int button, const UnitPoint localPos) {
    (void)localPos;
    if (!enabled || button != SDL_BUTTON_LEFT || !onClick) {
        return;
    }

    onClick();
}

void TextButton::OnMouseEnter() {
    hovered = true;
}

void TextButton::OnMouseLeave() {
    hovered = false;
    pressed = false;
}

void TextButton::OnMouseMove(const UnitPoint localPos) {
    (void)localPos;
}

void TextButton::SetText(std::string newText) {
    if (newText == text) {
        return;
    }

    text = std::move(newText);
    textDirty = true;
}

void TextButton::SetEnabled(const bool value) {
    if (enabled == value) {
        return;
    }
    enabled = value;
    if (!enabled) {
        hovered = false;
        pressed = false;
    }
}

void TextButton::SetColors(const SDL_Color normal, const SDL_Color hover, const SDL_Color pressCol,
                           const SDL_Color textCol) {
    normalColor = normal;
    hoverColor = hover;
    pressedColor = pressCol;
    textColor = textCol;
    textDirty = true;
}

void TextButton::SetSelectedColors(const SDL_Color selectedNormal, const SDL_Color selectedHover,
                                   const SDL_Color selectedPressed) {
    selectedNormalColor = selectedNormal;
    selectedHoverColor = selectedHover;
    selectedPressedColor = selectedPressed;
}

void TextButton::SetSelected(const bool value) {
    if (selected == value) {
        return;
    }
    selected = value;
}

void TextButton::RebuildTextTexture(SDL_Renderer* renderer) {
    if (!font || text.empty()) {
        textTexture.reset();
        textW = 0;
        textH = 0;
        textDirty = false;
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font.get(), text.c_str(), 0, textColor);
    if (!surface) {
        textTexture.reset();
        textW = 0;
        textH = 0;
        textDirty = false;
        return;
    }

    textW = surface->w;
    textH = surface->h;
    SDL_Texture* rawTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (rawTexture) {
        textTexture = std::shared_ptr<SDL_Texture>(rawTexture, [](SDL_Texture* t) { SDL_DestroyTexture(t); });
    } else {
        textTexture.reset();
        textW = 0;
        textH = 0;
    }

    textDirty = false;
}

} // namespace Game
