//
// Created by karp on 09/04/2026.
//

#include "Sprite.h"

#include <utility>

namespace Game {

Sprite::Sprite(const UnitBounds bounds, std::shared_ptr<SDL_Texture> texture)
    : Drawable(bounds), texture(std::move(texture)) {}

void Sprite::Update() {}

void Sprite::Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) {
    if (!texture) {
        return;
    }
    const auto drawAlpha = static_cast<Uint8>((static_cast<unsigned>(alpha) * color.a) / 255u);
    if (drawAlpha == 0) {
        return;
    }
    const SDL_FRect rect = {
        .x = parentRect.x + bounds.min.x * parentRect.w,
        .y = parentRect.y + bounds.min.y * parentRect.h,
        .w = (bounds.max.x - bounds.min.x) * parentRect.w,
        .h = (bounds.max.y - bounds.min.y) * parentRect.h
    };
    SDL_SetTextureColorMod(texture.get(), color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(texture.get(), drawAlpha);
    SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);
    SDL_RenderTexture(renderer, texture.get(), nullptr, &rect);
}

} // namespace Game
