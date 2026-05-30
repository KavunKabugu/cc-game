#ifndef CC_GAME_TEXT_BUTTON_H
#define CC_GAME_TEXT_BUTTON_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <functional>
#include <memory>
#include <string>

#include "Drawable.h"
#include "Game/Events/Interfaces.h"

namespace Game {

class TextButton final : public Drawable, public IMouseClickable, public IMouseHoverable {
public:
    TextButton(
        UnitBounds bounds,
        std::shared_ptr<TTF_Font> font,
        std::string text,
        std::function<void()> onClick,
        std::shared_ptr<SDL_Texture> backgroundTexture = nullptr
    );

    void Update() override {}
    void Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) override;

    IMouseClickable* AsMouseClickable() override { return this; }
    IMouseHoverable* AsMouseHoverable() override { return this; }

    bool OnMouseButtonDown(int button, UnitPoint localPos) override;
    bool OnMouseButtonUp(int button, UnitPoint localPos) override;
    void OnMouseButtonClicked(int button, UnitPoint localPos) override;

    void OnMouseEnter() override;
    void OnMouseLeave() override;
    void OnMouseMove(UnitPoint localPos) override;

    void SetText(std::string newText);
    void SetEnabled(bool value);
    void SetColors(SDL_Color normal, SDL_Color hover, SDL_Color pressCol, SDL_Color textCol);
    void SetSelectedColors(SDL_Color selectedNormal, SDL_Color selectedHover, SDL_Color selectedPressed);
    void SetSelected(bool value);
    [[nodiscard]] bool IsSelected() const { return selected; }

private:
    void RebuildTextTexture(SDL_Renderer* renderer);

    std::shared_ptr<TTF_Font> font;
    std::string text;
    std::shared_ptr<SDL_Texture> textTexture;
    std::shared_ptr<SDL_Texture> backgroundTexture;
    std::function<void()> onClick;

    SDL_Color normalColor{50, 80, 140, 255};
    SDL_Color hoverColor{70, 105, 170, 255};
    SDL_Color pressedColor{35, 60, 110, 255};
    SDL_Color selectedNormalColor{76, 136, 86, 255};
    SDL_Color selectedHoverColor{92, 160, 102, 255};
    SDL_Color selectedPressedColor{58, 114, 69, 255};
    SDL_Color textColor{255, 255, 255, 255};

    int textW = 0;
    int textH = 0;
    bool hovered = false;
    bool pressed = false;
    bool enabled = true;
    bool selected = false;
    bool textDirty = true;
};

} // namespace Game

#endif // CC_GAME_TEXT_BUTTON_H
