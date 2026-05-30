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

protected:
    std::shared_ptr<SDL_Texture> texture;
};

} // namespace Game

#endif //CC_GAME_SPRITE_H
