//
// Created by karp on 09/04/2026.
//

#ifndef CC_GAME_DRAWABLE_H
#define CC_GAME_DRAWABLE_H

#include <SDL3/SDL.h>

#include "GameObject.h"

namespace Game {

class Drawable : public GameObject {
public:
    using GameObject::GameObject;
    Drawable* AsDrawable() override { return this; }
    virtual void Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) = 0;
};

} // namespace Game

#endif //CC_GAME_DRAWABLE_H
