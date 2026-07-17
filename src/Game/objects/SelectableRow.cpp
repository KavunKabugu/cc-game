#include "SelectableRow.h"

#include <utility>

namespace Game {

SelectableRow::SelectableRow(const UnitBounds bounds, std::function<void()> onClick)
    : Container(bounds),
      onClick(std::move(onClick)) {}

void SelectableRow::Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) {
    const SDL_FRect rect = {
        .x = parentRect.x + bounds.min.x * parentRect.w,
        .y = parentRect.y + bounds.min.y * parentRect.h,
        .w = (bounds.max.x - bounds.min.x) * parentRect.w,
        .h = (bounds.max.y - bounds.min.y) * parentRect.h
    };

    if (rect.w <= 0.0f || rect.h <= 0.0f) {
        return;
    }

    lastRenderRect = rect;

    const SDL_Color activeNormal = selected ? selectedNormalColor : normalColor;
    const SDL_Color activeHover = selected ? selectedHoverColor : hoverColor;
    const SDL_Color activePressed = selected ? selectedPressedColor : pressedColor;
    const auto [r, g, b, a] = !enabled ? SDL_Color{.r = 80, .g = 80, .b = 80, .a = 255}
                                       : pressed ? activePressed : hovered ? activeHover : activeNormal;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderFillRect(renderer, &rect);

    for (const auto& child : children) {
        if (auto* d = child->AsDrawable()) {
            d->Render(renderer, rect);
        }
    }
}

bool SelectableRow::OnMouseButtonDown(const int button, const UnitPoint localPos) {
    (void)localPos;
    if (!enabled || button != SDL_BUTTON_LEFT) {
        return false;
    }

    pressed = true;
    return true;
}

bool SelectableRow::OnMouseButtonUp(const int button, const UnitPoint localPos) {
    (void)localPos;
    if (button != SDL_BUTTON_LEFT) {
        return false;
    }

    const bool wasPressed = pressed;
    pressed = false;
    return wasPressed;
}

void SelectableRow::OnMouseButtonClicked(const int button, const UnitPoint localPos) {
    (void)localPos;
    if (!enabled || button != SDL_BUTTON_LEFT || !onClick) {
        return;
    }

    onClick();
}

void SelectableRow::OnMouseEnter() {
    hovered = true;
    NotifyVisualStateChanged();
}

void SelectableRow::OnMouseLeave() {
    hovered = false;
    pressed = false;
    NotifyVisualStateChanged();
}

void SelectableRow::OnMouseMove(const UnitPoint localPos) {
    (void)localPos;
}

void SelectableRow::SetEnabled(const bool value) {
    if (enabled == value) {
        return;
    }
    enabled = value;
    if (!enabled) {
        hovered = false;
        pressed = false;
        NotifyVisualStateChanged();
    }
}

void SelectableRow::SetColors(const SDL_Color normal, const SDL_Color hover, const SDL_Color pressCol) {
    normalColor = normal;
    hoverColor = hover;
    pressedColor = pressCol;
}

void SelectableRow::SetSelectedColors(const SDL_Color selectedNormal, const SDL_Color selectedHover,
                                      const SDL_Color selectedPressed) {
    selectedNormalColor = selectedNormal;
    selectedHoverColor = selectedHover;
    selectedPressedColor = selectedPressed;
}

void SelectableRow::SetSelected(const bool value) {
    if (selected == value) {
        return;
    }
    selected = value;
    NotifyVisualStateChanged();
}

void SelectableRow::SetOnVisualStateChanged(std::function<void(bool, bool)> callback) {
    onVisualStateChanged = std::move(callback);
    NotifyVisualStateChanged();
}

void SelectableRow::NotifyVisualStateChanged() const {
    if (onVisualStateChanged) {
        onVisualStateChanged(selected, hovered);
    }
}

} // namespace Game
