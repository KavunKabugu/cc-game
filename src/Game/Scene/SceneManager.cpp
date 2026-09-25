#include "SceneManager.h"

#include <algorithm>

#include <SDL3/SDL_render.h>

namespace Game {

class SceneManager::FadeTransitionScene final : public IScene {
public:
    FadeTransitionScene(
        SceneManager& sceneManager,
        std::function<std::unique_ptr<IScene>()> createScene
    )
        : sceneManager(sceneManager),
          createScene(std::move(createScene)) {}

    void Update(const double dt) override {
        switch (phase) {
            case Phase::FadeOut:
                elapsed += dt;
                if (elapsed < kSceneFadeDurationSeconds) {
                    break;
                }
                elapsed = kSceneFadeDurationSeconds;
                phase = Phase::Swap;
                [[fallthrough]];
            case Phase::Swap:
                if (createScene) {
                    sceneManager.QueueReplaceUnderTop(std::move(createScene));
                }
                phase = Phase::FadeIn;
                elapsed = 0.0;
                break;
            case Phase::FadeIn:
                elapsed += dt;
                if (elapsed >= kSceneFadeDurationSeconds) {
                    elapsed = kSceneFadeDurationSeconds;
                    sceneManager.QueuePop();
                    phase = Phase::Done;
                }
                break;
            case Phase::Done:
                break;
        }
    }

    void Render(SDL_Renderer* renderer, const SDL_FRect& logicalViewport) override {
        if (!renderer) {
            return;
        }

        const Uint8 alpha = OverlayAlpha();
        if (alpha == 0) {
            return;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
        SDL_RenderFillRect(renderer, &logicalViewport);
    }

    [[nodiscard]] Container* GetRoot() const override { return nullptr; }
    [[nodiscard]] bool BlocksLowerInput() const override { return true; }
    [[nodiscard]] bool BlocksLowerRendering() const override { return false; }
    [[nodiscard]] bool BlocksLowerUpdates() const override { return true; }

private:
    enum class Phase {
        FadeOut,
        Swap,
        FadeIn,
        Done
    };

    [[nodiscard]] Uint8 OverlayAlpha() const {
        constexpr double duration = kSceneFadeDurationSeconds;
        const double t = duration > 0.0 ? std::clamp(elapsed / duration, 0.0, 1.0) : 1.0;
        const double cover = phase == Phase::FadeIn || phase == Phase::Done ? 1.0 - t : t;
        return static_cast<Uint8>(std::lround(cover * 255.0));
    }

    SceneManager& sceneManager;
    std::function<std::unique_ptr<IScene>()> createScene;
    Phase phase = Phase::FadeOut;
    double elapsed = 0.0;
};

void SceneManager::QueuePop() {
    queue.emplace_back(SceneCommand{.type = CommandType::Pop, .createScene = nullptr});
}

void SceneManager::QueueClear(const bool allowEmptyStack) {
    queue.emplace_back(SceneCommand{
            .type = CommandType::Clear, .createScene = nullptr, .allowEmptyStack = allowEmptyStack
        });
}

void SceneManager::QueueReplaceUnderTop(std::function<std::unique_ptr<IScene>()> createScene) {
    queue.emplace_back(SceneCommand{
            .type = CommandType::ReplaceUnderTop, .createScene = std::move(createScene)
        });
}

void SceneManager::UpdateActiveScenes(const double dt) const {
    if (stack.empty()) {
        return;
    }

    std::size_t firstUpdateIndex = stack.size() - 1;
    for (std::size_t i = stack.size(); i-- > 0;) {
        firstUpdateIndex = i;
        if (stack[i]->BlocksLowerUpdates()) {
            break;
        }
    }

    for (std::size_t i = firstUpdateIndex; i < stack.size(); ++i) {
        stack[i]->Update(dt);
    }
}

void SceneManager::RenderScenes(SDL_Renderer* renderer, const SDL_FRect& logicalViewport) const {
    if (stack.empty()) {
        return;
    }

    std::size_t firstRenderIndex = stack.size() - 1;
    for (std::size_t i = stack.size(); i-- > 0;) {
        firstRenderIndex = i;
        if (stack[i]->BlocksLowerRendering()) {
            break;
        }
    }

    for (std::size_t i = firstRenderIndex; i < stack.size(); ++i) {
        stack[i]->Render(renderer, logicalViewport);
    }
}

std::vector<IScene*> SceneManager::GetInputScenes() const {
    std::vector<IScene*> inputScenes;
    if (stack.empty()) {
        return inputScenes;
    }

    for (std::size_t i = stack.size(); i-- > 0;) {
        inputScenes.push_back(stack[i].get());
        if (stack[i]->BlocksLowerInput()) {
            break;
        }
    }

    return inputScenes;
}

IScene* SceneManager::GetInputScene() const {
    if (stack.empty()) {
        return nullptr;
    }

    return stack.back().get();
}

bool SceneManager::CommitQueuedTransitions() {
    bool changed = false;

    while (!queue.empty()) {
        auto [type, createScene, allowEmptyStack] = std::move(queue.front());
        queue.pop_front();

        switch (type) {
            case CommandType::Push: {
                if (!createScene) {
                    break;
                }
                std::unique_ptr<IScene> scene = createScene();
                if (!scene) {
                    break;
                }
                if (!stack.empty()) {
                    stack.back()->OnPause();
                }
                scene->OnEnter();
                stack.push_back(std::move(scene));
                changed = true;
                break;
            }
            case CommandType::Pop: {
                if (stack.empty() || !CanPop()) {
                    break;
                }
                stack.back()->OnExit();
                stack.pop_back();
                stack.back()->OnResume();
                changed = true;
                break;
            }
            case CommandType::Replace: {
                if (stack.empty()) {
                    if (!createScene) {
                        break;
                    }
                    std::unique_ptr<IScene> scene = createScene();
                    if (!scene) {
                        break;
                    }
                    scene->OnEnter();
                    stack.push_back(std::move(scene));
                    changed = true;
                    break;
                }

                if (!createScene) {
                    break;
                }

                std::unique_ptr<IScene> scene = createScene();
                if (!scene) {
                    break;
                }

                stack.back()->OnExit();
                stack.pop_back();
                scene->OnEnter();
                stack.push_back(std::move(scene));
                changed = true;
                break;
            }
            case CommandType::ReplaceWithFade: {
                if (!createScene) {
                    break;
                }

                if (stack.empty()) {
                    std::unique_ptr<IScene> scene = createScene();
                    if (!scene) {
                        break;
                    }
                    scene->OnEnter();
                    stack.push_back(std::move(scene));
                    changed = true;
                    break;
                }

                stack.back()->OnPause();
                auto fade = std::make_unique<FadeTransitionScene>(*this, std::move(createScene));
                fade->OnEnter();
                stack.push_back(std::move(fade));
                changed = true;
                break;
            }
            case CommandType::ReplaceUnderTop: {
                if (stack.size() < 2 || !createScene) {
                    break;
                }

                std::unique_ptr<IScene> scene = createScene();
                if (!scene) {
                    break;
                }

                const std::size_t coveredIndex = stack.size() - 2;
                stack[coveredIndex]->OnExit();
                stack[coveredIndex] = std::move(scene);
                stack[coveredIndex]->OnEnter();
                changed = true;
                break;
            }
            case CommandType::Clear: {
                const std::size_t keepCount = allowEmptyStack ? 0 : 1;
                // I hate this line, static analysis keeps telling
                // "Local variable used in loop condition is not updated in the loop"
                // But stack.size() is updated, so it's a false flag
                // And no matter what I do, it keeps bitching about it
                // The warning never goes away
                // If you ever find yourself here, just ignore it
                while (stack.size() > keepCount) {
                    stack.back()->OnExit();
                    stack.pop_back();
                    changed = true;
                }
                if (!allowEmptyStack && stack.size() == 1) {
                    stack.back()->OnResume();
                }
                break;
            }
        }
    }

    return changed;
}

} // namespace Game
