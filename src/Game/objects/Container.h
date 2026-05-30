//
// Created by karp on 14/04/2026.
//

#ifndef CC_GAME_CONTAINER_H
#define CC_GAME_CONTAINER_H

#include <vector>
#include <memory>
#include "Drawable.h"
#include "Game/Events/Interfaces.h"

namespace Game {

class Container : public Drawable {
public:
    using Drawable::Drawable;

    void Update() override;
    void Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) override;

    Container* AsContainer() override { return this; }

    // Recursive event dispatch
    virtual bool HandleEvent(const SDL_Event& event, UnitPoint parentLocalPos, GameObject*& pressedObject, GameObject*& hoveredObject);

    template<typename T, typename... Args>
    T* CreateChild(UnitBounds bounds, Args&&... args) {
        auto child = std::make_unique<T>(bounds, std::forward<Args>(args)...);
        T* ptr = child.get();
        children.push_back(std::move(child));
        return ptr;
    }

    [[nodiscard]] const std::vector<std::unique_ptr<GameObject>>& GetChildren() const { return children; }
    [[nodiscard]] std::vector<std::unique_ptr<GameObject>>& GetChildren() { return children; }

    [[nodiscard]] SDL_FRect GetLastRenderRect() const { return lastRenderRect; }

protected:
    std::vector<std::unique_ptr<GameObject>> children;
    SDL_FRect lastRenderRect{0, 0, 0, 0};
};

} // namespace Game

#endif //CC_GAME_CONTAINER_H
