#include "PanelRect.h"

namespace Game {

PanelRect::PanelRect(const UnitBounds bounds, const SDL_Color color) : Drawable(bounds), color(color) {}

void PanelRect::Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) {
    const SDL_FRect rect = {
        .x = parentRect.x + bounds.min.x * parentRect.w,
        .y = parentRect.y + bounds.min.y * parentRect.h,
        .w = (bounds.max.x - bounds.min.x) * parentRect.w,
        .h = (bounds.max.y - bounds.min.y) * parentRect.h
    };

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

} // namespace Game
