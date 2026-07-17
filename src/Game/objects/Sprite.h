//
// Created by karp on 09/04/2026.
//

#ifndef CC_GAME_SPRITE_H
#define CC_GAME_SPRITE_H

#include <memory>
#include <SDL3/SDL.h>

#include "Drawable.h"

namespace Game {

class Sprite : public Drawable {
public:
    Sprite(UnitBounds bounds, std::shared_ptr<SDL_Texture> texture);
    ~Sprite() override = default;
    void Update() override;
    void Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) override;

    // 0 = fully transparent, 255 = fully opaque.
    void SetAlpha(const Uint8 a) { alpha = a; }
    [[nodiscard]] Uint8 GetAlpha() const { return alpha; }

protected:
    std::shared_ptr<SDL_Texture> texture;
    Uint8 alpha{255};
};

} // namespace Game

#endif //CC_GAME_SPRITE_H
