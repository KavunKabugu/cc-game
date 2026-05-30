#ifndef CC_GAME_PANEL_RECT_H
#define CC_GAME_PANEL_RECT_H

#include <SDL3/SDL.h>

#include "Drawable.h"

namespace Game {

class PanelRect final : public Drawable {
public:
    PanelRect(UnitBounds bounds, SDL_Color color);

    void Update() override {}
    void Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) override;

    void SetColor(const SDL_Color newColor) { color = newColor; }

private:
    SDL_Color color{};
};

} // namespace Game

#endif // CC_GAME_PANEL_RECT_H
