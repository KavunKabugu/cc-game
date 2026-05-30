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
    if (texture) {
        const SDL_FRect rect = {
            parentRect.x + (bounds.min.x * parentRect.w),
            parentRect.y + (bounds.min.y * parentRect.h),
            (bounds.max.x - bounds.min.x) * parentRect.w,
            (bounds.max.y - bounds.min.y) * parentRect.h
        };
        SDL_RenderTexture(renderer, texture.get(), nullptr, &rect);
    }
}

} // namespace Game
