#ifndef CC_GAME_ISCENE_H
#define CC_GAME_ISCENE_H

#include <SDL3/SDL.h>

namespace Game {

class Container;

class IScene {
public:
    virtual ~IScene() = default;

    virtual void OnEnter() {}
    virtual void OnPause() {}
    virtual void OnResume() {}
    virtual void OnExit() {}

    virtual void Update(double dt) = 0;
    virtual void Render(SDL_Renderer* renderer, const SDL_FRect& logicalViewport) = 0;

    [[nodiscard]] virtual Container* GetRoot() const = 0;

    [[nodiscard]] virtual bool BlocksLowerInput() const = 0;
    [[nodiscard]] virtual bool BlocksLowerRendering() const = 0;
    [[nodiscard]] virtual bool BlocksLowerUpdates() const = 0;
};

} // namespace Game

#endif // CC_GAME_ISCENE_H
