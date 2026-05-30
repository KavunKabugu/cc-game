#ifndef CC_GAME_SCENE_MANAGER_H
#define CC_GAME_SCENE_MANAGER_H

#include <deque>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "IScene.h"

namespace Game {

class SceneManager {
public:
    enum class CommandType {
        Push,
        Pop,
        Replace,
        Clear
    };

    struct SceneCommand {
        CommandType type;
        std::function<std::unique_ptr<IScene>()> createScene;
        bool allowEmptyStack = true;
    };

    template <typename T, typename... Args>
    void QueuePush(Args&&... args) {
        queue.emplace_back(SceneCommand{
            CommandType::Push,
            [captured = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                return std::apply(
                    []<typename... T0>(T0&&... unpacked) {
                        return std::make_unique<T>(std::forward<T0>(unpacked)...);
                    },
                    std::move(captured));
            }});
    }

    template <typename T, typename... Args>
    void QueueReplace(Args&&... args) {
        queue.emplace_back(SceneCommand{
            CommandType::Replace,
            [captured = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                return std::apply(
                    []<typename... T0>(T0&&... unpacked) {
                        return std::make_unique<T>(std::forward<T0>(unpacked)...);
                    },
                    std::move(captured));
            }});
    }

    void QueuePop();
    void QueueClear(bool allowEmptyStack = true);

    void UpdateActiveScenes(double dt) const;
    void RenderScenes(SDL_Renderer* renderer, const SDL_FRect& logicalViewport) const;
    [[nodiscard]] std::vector<IScene*> GetInputScenes() const;
    [[nodiscard]] IScene* GetInputScene() const;
    [[nodiscard]] bool HasScenes() const { return !stack.empty(); }
    [[nodiscard]] std::size_t SceneCount() const { return stack.size(); }
    [[nodiscard]] bool CanPop() const { return stack.size() > 1; }

    // Returns true if stack was modified.
    bool CommitQueuedTransitions();

private:
    std::vector<std::unique_ptr<IScene>> stack;
    std::deque<SceneCommand> queue;
};

} // namespace Game

#endif // CC_GAME_SCENE_MANAGER_H
