#include <cassert>
#include <functional>
#include <string>
#include <vector>

#include "Game/Scene/SceneManager.h"

namespace Game {

namespace {
    class FakeScene final : public IScene {
    public:
        FakeScene(
            std::string name,
            std::vector<std::string>& lifecycleLog,
            const bool blocksLowerInput = true,
            const bool blocksLowerRendering = true,
            const bool blocksLowerUpdates = true
        )
            : name(std::move(name)),
              lifecycleLog(lifecycleLog),
              blocksLowerInput(blocksLowerInput),
              blocksLowerRendering(blocksLowerRendering),
              blocksLowerUpdates(blocksLowerUpdates) {}

        void OnEnter() override { lifecycleLog.push_back(name + ":Enter"); }
        void OnPause() override { lifecycleLog.push_back(name + ":Pause"); }
        void OnResume() override { lifecycleLog.push_back(name + ":Resume"); }
        void OnExit() override { lifecycleLog.push_back(name + ":Exit"); }
        void Update(const double dt) override {
            (void)dt;
            lifecycleLog.push_back(name + ":Update");
        }
        void Render(SDL_Renderer* renderer, const SDL_FRect& logicalViewport) override {
            (void)renderer;
            (void)logicalViewport;
            lifecycleLog.push_back(name + ":Render");
        }

        [[nodiscard]] Container* GetRoot() const override { return nullptr; }
        [[nodiscard]] bool BlocksLowerInput() const override { return blocksLowerInput; }
        [[nodiscard]] bool BlocksLowerRendering() const override { return blocksLowerRendering; }
        [[nodiscard]] bool BlocksLowerUpdates() const override { return blocksLowerUpdates; }

    private:
        std::string name;
        std::vector<std::string>& lifecycleLog;
        bool blocksLowerInput;
        bool blocksLowerRendering;
        bool blocksLowerUpdates;
    };
}

static void AssertSequence(const std::vector<std::string>& actual, const std::vector<std::string>& expected) {
    assert(actual == expected);
}

static void TestLifecycleOrdering() {
    std::vector<std::string> log;
    SceneManager manager;

    manager.QueuePush<FakeScene>("A", std::ref(log), true, true, true);
    assert(manager.CommitQueuedTransitions());
    manager.QueuePush<FakeScene>("B", std::ref(log), true, true, true);
    assert(manager.CommitQueuedTransitions());
    manager.QueuePop();
    assert(manager.CommitQueuedTransitions());

    AssertSequence(log, {
        "A:Enter",
        "A:Pause", "B:Enter",
        "B:Exit", "A:Resume"
    });
}

static void TestPopInvariant() {
    std::vector<std::string> log;
    SceneManager manager;

    manager.QueuePush<FakeScene>("A", std::ref(log), true, true, true);
    assert(manager.CommitQueuedTransitions());
    manager.QueuePop();
    assert(!manager.CommitQueuedTransitions());

    AssertSequence(log, {"A:Enter"});
    assert(manager.SceneCount() == 1);
}

static void TestInputTargets() {
    std::vector<std::string> log;
    SceneManager manager;

    manager.QueuePush<FakeScene>("A", std::ref(log), true, true, true);
    manager.QueuePush<FakeScene>("B", std::ref(log), false, false, false);
    manager.QueuePush<FakeScene>("C", std::ref(log), false, false, false);
    assert(manager.CommitQueuedTransitions());

    const std::vector<IScene*> targets = manager.GetInputScenes();
    assert(targets.size() == 3);
}

static void TestUpdateAndRenderBlocking() {
    std::vector<std::string> log;
    SceneManager manager;

    manager.QueuePush<FakeScene>("A", std::ref(log), true, true, true);
    manager.QueuePush<FakeScene>("B", std::ref(log), true, true, true);
    manager.QueuePush<FakeScene>("C", std::ref(log), false, false, false);
    assert(manager.CommitQueuedTransitions());

    log.clear();
    manager.UpdateActiveScenes(0.016);
    AssertSequence(log, {"B:Update", "C:Update"});

    log.clear();
    constexpr SDL_FRect logicalViewport{.x = 0.0f, .y = 0.0f, .w = 1920.0f, .h = 1080.0f};
    manager.RenderScenes(nullptr, logicalViewport);
    AssertSequence(log, {"B:Render", "C:Render"});
}

static void TestClearKeepingBaseScene() {
    std::vector<std::string> log;
    SceneManager manager;

    manager.QueuePush<FakeScene>("A", std::ref(log), true, true, true);
    manager.QueuePush<FakeScene>("B", std::ref(log), true, true, true);
    manager.QueuePush<FakeScene>("C", std::ref(log), true, true, true);
    assert(manager.CommitQueuedTransitions());

    log.clear();
    manager.QueueClear(false);
    assert(manager.CommitQueuedTransitions());

    AssertSequence(log, {"C:Exit", "B:Exit", "A:Resume"});
    assert(manager.SceneCount() == 1);
}

} // namespace Game

int main() {
    Game::TestLifecycleOrdering();
    Game::TestPopInvariant();
    Game::TestInputTargets();
    Game::TestUpdateAndRenderBlocking();
    Game::TestClearKeepingBaseScene();
    return 0;
}
