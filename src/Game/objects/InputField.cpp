#include "InputField.h"

#include <algorithm>
#include <utility>

#include "Game/EventManager.h"

namespace Game {

InputField::InputField(
    const UnitBounds bounds,
    std::shared_ptr<TTF_Font> font,
    std::string initialText,
    const std::size_t maxLength)
    : Drawable(bounds),
      font(std::move(font)),
      text(std::move(initialText)),
      maxLength(maxLength) {
    if (this->text.size() > this->maxLength) {
        this->text.resize(this->maxLength);
    }
}

void InputField::SetText(std::string newText) {
    if (newText.size() > maxLength) {
        newText.resize(maxLength);
    }
    if (newText == text) {
        return;
    }
    text = std::move(newText);
    textDirty = true;
}

void InputField::SetMaxLength(const std::size_t length) {
    maxLength = length;
    if (text.size() > maxLength) {
        text.resize(maxLength);
        textDirty = true;
    }
}

void InputField::SetOnChanged(std::function<void(const std::string&)> callback) {
    onChanged = std::move(callback);
}

void InputField::SetColors(const SDL_Color normal, const SDL_Color focusedCol, const SDL_Color textCol) {
    normalColor = normal;
    focusedColor = focusedCol;
    textColor = textCol;
    textDirty = true;
}

void InputField::Focus() {
    if (focused) {
        return;
    }
    textAtFocusStart = text;
    focused = true;
    EventManager::getInstance().SetTextInputFocus(this);
}

void InputField::Blur(const bool notifyChanged) {
    if (!focused) {
        return;
    }
    focused = false;
    if (EventManager::getInstance().GetTextInputFocus() == this) {
        EventManager::getInstance().ClearTextInputFocus();
    }
    if (notifyChanged && onChanged && text != textAtFocusStart) {
        onChanged(text);
    }
}

void InputField::OnTextInputFocusLost() {
    focused = false;
    if (onChanged && text != textAtFocusStart) {
        onChanged(text);
    }
}

bool InputField::OnTextInput(const char* utf8) {
    if (!focused || !utf8 || utf8[0] == '\0') {
        return false;
    }
    InsertUtf8(utf8);
    return true;
}

void InputField::InsertUtf8(const std::string_view utf8) {
    if (utf8.empty() || text.size() >= maxLength) {
        return;
    }
    const std::size_t room = maxLength - text.size();
    // Truncate by bytes for maxLength (Latin names), avoid splitting multi-byte mid-sequence roughly.
    // TODO: I am pretty sure this breaks on non-latin names, check and confirm
    std::string insert(utf8);
    if (insert.size() > room) {
        insert.resize(room);
        while (!insert.empty() && (static_cast<unsigned char>(insert.back()) & 0xC0) == 0x80) {
            insert.pop_back();
        }
    }
    if (insert.empty()) {
        return;
    }
    text += insert;
    textDirty = true;
}

void InputField::DeleteBackward() {
    if (text.empty()) {
        return;
    }
    // Remove one UTF-8 codepoint from the end.
    do {
        text.pop_back();
    } while (!text.empty() && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80);
    textDirty = true;
}

bool InputField::OnKeyDown(const SDL_Keycode key, const Uint64 timestamp) {
    (void)timestamp;
    if (!focused) {
        return false;
    }

    if (key == SDLK_ESCAPE || key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        Blur(true);
        return true;
    }
    if (key == SDLK_BACKSPACE) {
        DeleteBackward();
        return true;
    }
    return true; // Consume other keys while focused so they don't hit overlay Esc / binds.
}

bool InputField::OnKeyUp(const SDL_Keycode key, const Uint64 timestamp) {
    (void)key;
    (void)timestamp;
    return focused;
}

bool InputField::OnMouseButtonDown(const int button, const UnitPoint localPos) {
    (void)localPos;
    if (button != SDL_BUTTON_LEFT) {
        return false;
    }
    pressed = true;
    return true;
}

bool InputField::OnMouseButtonUp(const int button, const UnitPoint localPos) {
    (void)localPos;
    if (button != SDL_BUTTON_LEFT) {
        return false;
    }
    const bool wasPressed = pressed;
    pressed = false;
    return wasPressed;
}

void InputField::OnMouseButtonClicked(const int button, const UnitPoint localPos) {
    (void)localPos;
    if (button != SDL_BUTTON_LEFT) {
        return;
    }
    Focus();
}

void InputField::OnMouseEnter() {
    hovered = true;
}

void InputField::OnMouseLeave() {
    hovered = false;
    pressed = false;
}

void InputField::OnMouseMove(const UnitPoint localPos) {
    (void)localPos;
}

void InputField::RebuildTextTexture(SDL_Renderer* renderer) {
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
    SDL_Texture* raw = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (raw) {
        textTexture = std::shared_ptr<SDL_Texture>(raw, [](SDL_Texture* t) { SDL_DestroyTexture(t); });
    } else {
        textTexture.reset();
        textW = 0;
        textH = 0;
    }
    textDirty = false;
}

void InputField::Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) {
    const SDL_FRect rect = {
        .x = parentRect.x + bounds.min.x * parentRect.w,
        .y = parentRect.y + bounds.min.y * parentRect.h,
        .w = (bounds.max.x - bounds.min.x) * parentRect.w,
        .h = (bounds.max.y - bounds.min.y) * parentRect.h
    };
    if (rect.w <= 0.0f || rect.h <= 0.0f) {
        return;
    }

    const auto [r, g, b, a] = focused ? focusedColor : hovered ? SDL_Color{
                                      .r = static_cast<Uint8>(std::min(255, normalColor.r + 20)),
                                      .g = static_cast<Uint8>(std::min(255, normalColor.g + 20)),
                                      .b = static_cast<Uint8>(std::min(255, normalColor.b + 20)),
                                      .a = normalColor.a
                                  }
                                  : normalColor;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderFillRect(renderer, &rect);

    if (textDirty) {
        RebuildTextTexture(renderer);
    }

    constexpr float pad = 8.0f;
    if (textTexture) {
        const SDL_FRect textRect = {
            .x = rect.x + pad,
            .y = rect.y + (rect.h - static_cast<float>(textH)) * 0.5f,
            .w = static_cast<float>(textW),
            .h = static_cast<float>(textH)
        };
        SDL_RenderTexture(renderer, textTexture.get(), nullptr, &textRect);
    }

    if (focused) {
        const float caretX = rect.x + pad + static_cast<float>(textW) + 2.0f;
        const float caretH = rect.h * 0.55f;
        const SDL_FRect caret = {
            .x = caretX,
            .y = rect.y + (rect.h - caretH) * 0.5f,
            .w = 2.0f,
            .h = caretH
        };
        SDL_SetRenderDrawColor(renderer, textColor.r, textColor.g, textColor.b, 220);
        SDL_RenderFillRect(renderer, &caret);
    }
}

} // namespace Game
