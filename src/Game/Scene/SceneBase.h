#ifndef CC_GAME_SCENE_BASE_H
#define CC_GAME_SCENE_BASE_H

#include <memory>

#include "IScene.h"
#include "Game/objects/Container.h"

namespace Game {

class SceneBase : public IScene {
public:
    SceneBase() : root(std::make_unique<Container>(UnitBounds{
            .min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}
        })) {}
    ~SceneBase() override = default;

    void Update(const double dt) override {
        if (root) {
            DispatchDeltaTime(root.get(), dt);
            root->Update();
        }
    }

    void Render(SDL_Renderer* renderer, const SDL_FRect& logicalViewport) override {
        if (!root) {
            return;
        }

        root->Render(renderer, logicalViewport);
    }

    [[nodiscard]] Container* GetRoot() const override { return root.get(); }

    [[nodiscard]] bool BlocksLowerInput() const override { return true; }
    [[nodiscard]] bool BlocksLowerRendering() const override { return true; }
    [[nodiscard]] bool BlocksLowerUpdates() const override { return true; }

protected:
    static void DispatchDeltaTime(GameObject* object, double dt) {
        if (!object) {
            return;
        }

        if (auto* timeAware = object->AsTimeAware()) {
            timeAware->UpdateWithDeltaTime(dt);
        }

        if (auto* container = object->AsContainer()) {
            for (const auto& child : container->GetChildren()) {
                DispatchDeltaTime(child.get(), dt);
            }
        }
    }

    std::unique_ptr<Container> root;
};

} // namespace Game

#endif // CC_GAME_SCENE_BASE_H
