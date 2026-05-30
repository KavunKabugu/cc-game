#include "SceneManager.h"

namespace Game {

void SceneManager::QueuePop() {
    queue.emplace_back(SceneCommand{CommandType::Pop, nullptr});
}

void SceneManager::QueueClear(const bool allowEmptyStack) {
    queue.emplace_back(SceneCommand{CommandType::Clear, nullptr, allowEmptyStack});
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
