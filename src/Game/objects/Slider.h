#ifndef CC_GAME_SLIDER_H
#define CC_GAME_SLIDER_H

#include <SDL3/SDL.h>

#include <functional>

#include "Drawable.h"
#include "Game/Events/Interfaces.h"

namespace Game {

class Slider final : public Drawable, public IMouseClickable, public IMouseHoverable, public IMouseDraggable {
public:
    Slider(
        UnitBounds bounds,
        float minV,
        float maxV,
        float stepV,
        float initialValue
    );

    void Update() override {}
    void Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) override;

    IMouseClickable* AsMouseClickable() override { return this; }
    IMouseHoverable* AsMouseHoverable() override { return this; }
    IMouseDraggable* AsMouseDraggable() override { return this; }

    bool OnMouseButtonDown(int button, UnitPoint localPos) override;
    bool OnMouseButtonUp(int button, UnitPoint localPos) override;
    void OnMouseButtonClicked(int button, UnitPoint localPos) override;

    void OnMouseEnter() override;
    void OnMouseLeave() override;
    void OnMouseMove(UnitPoint localPos) override;

    bool OnDragStart(int button, UnitPoint localPos) override;
    void OnDragMove(UnitPoint localPos) override;
    void OnDragEnd(int button, UnitPoint localPos) override;

    void SetColors(SDL_Color track, SDL_Color thumb, SDL_Color thumbHover);
    void SetOnChanged(std::function<void(float)> callback);

    [[nodiscard]] float GetValue() const { return value; }
    void SetValue(float v);

private:
    void ApplyLocalToValue(UnitPoint localPos);
    [[nodiscard]] static float NormalizedFromLocalX(float localXNorm);

    float minValue = 0.0f;
    float maxValue = 1.0f;
    float step = 0.01f;
    float value = 0.0f;

    std::function<void(float)> onChanged;

    SDL_Color trackColor{.r = 50, .g = 52, .b = 62, .a = 255};
    SDL_Color thumbColor{.r = 200, .g = 202, .b = 215, .a = 255};
    SDL_Color thumbHoverColor{.r = 232, .g = 235, .b = 245, .a = 255};

    bool hovered = false;
};

} // namespace Game

#endif // CC_GAME_SLIDER_H
