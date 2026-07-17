#ifndef CC_GAME_SELECTABLE_ROW_H
#define CC_GAME_SELECTABLE_ROW_H

#include <SDL3/SDL.h>

#include <functional>

#include "Container.h"
#include "Game/Events/Interfaces.h"

namespace Game {

class SelectableRow final : public Container, public IMouseClickable, public IMouseHoverable {
public:
    SelectableRow(UnitBounds bounds, std::function<void()> onClick);

    void Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) override;

    IMouseClickable* AsMouseClickable() override { return this; }
    IMouseHoverable* AsMouseHoverable() override { return this; }

    bool OnMouseButtonDown(int button, UnitPoint localPos) override;
    bool OnMouseButtonUp(int button, UnitPoint localPos) override;
    void OnMouseButtonClicked(int button, UnitPoint localPos) override;

    void OnMouseEnter() override;
    void OnMouseLeave() override;
    void OnMouseMove(UnitPoint localPos) override;

    void SetEnabled(bool value);
    void SetColors(SDL_Color normal, SDL_Color hover, SDL_Color pressCol);
    void SetSelectedColors(SDL_Color selectedNormal, SDL_Color selectedHover, SDL_Color selectedPressed);
    void SetSelected(bool value);
    [[nodiscard]] bool IsSelected() const { return selected; }
    [[nodiscard]] bool IsHovered() const { return hovered; }

    void SetOnVisualStateChanged(std::function<void(bool selected, bool hovered)> callback);

private:
    void NotifyVisualStateChanged() const;

    std::function<void()> onClick;
    std::function<void(bool selected, bool hovered)> onVisualStateChanged;

    SDL_Color normalColor{.r = 50, .g = 80, .b = 140, .a = 255};
    SDL_Color hoverColor{.r = 70, .g = 105, .b = 170, .a = 255};
    SDL_Color pressedColor{.r = 35, .g = 60, .b = 110, .a = 255};
    SDL_Color selectedNormalColor{.r = 76, .g = 136, .b = 86, .a = 255};
    SDL_Color selectedHoverColor{.r = 92, .g = 160, .b = 102, .a = 255};
    SDL_Color selectedPressedColor{.r = 58, .g = 114, .b = 69, .a = 255};

    bool hovered = false;
    bool pressed = false;
    bool enabled = true;
    bool selected = false;
};

} // namespace Game

#endif // CC_GAME_SELECTABLE_ROW_H
