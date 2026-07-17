#include <cassert>
#include <vector>
#include "Game/Gameplay/LaneInputHandler.h"

namespace {

using namespace Game::Gameplay;

void TestBindingsNormalization() {
    // 0 / SDLK_UNKNOWN should map to default bindings
    LaneKeyBindings bindings{};
    for (int lane = 0; lane < kLaneBindingCount; ++lane) {
        for (int slot = 0; slot < kKeysPerLane; ++slot) {
            bindings[lane][slot] = SDLK_UNKNOWN;
        }
    }

    NormalizeLaneKeyBindings(bindings);

    [[maybe_unused]] const LaneKeyBindings defaults = DefaultLaneKeyBindings();
    for (int lane = 0; lane < kLaneBindingCount; ++lane) {
        for (int slot = 0; slot < kKeysPerLane; ++slot) {
            assert(bindings[lane][slot] == defaults[lane][slot]);
        }
    }
}

void TestKeyDownAndDrain() {
    LaneInputHandler handler;

    // Default bindings: Lane 0 is Left / A
    // Press SDLK_A at timestamp 1000
    bool processed = handler.OnKeyDown(SDLK_A, 1000);
    assert(processed);

    // Press SDLK_UP (Lane 1) at timestamp 2000
    processed = handler.OnKeyDown(SDLK_UP, 2000);
    assert(processed);

    // Unmapped key
    processed = handler.OnKeyDown(SDLK_X, 3000);
    assert(!processed);

    const std::vector<LanePress> events = handler.Drain();
    assert(events.size() == 2);

    assert(events[0].lane == 0);
    assert(events[0].sdlTimestampNs == 1000);

    assert(events[1].lane == 1);
    assert(events[1].sdlTimestampNs == 2000);

    // Drain should have cleared the queue
    std::vector<LanePress> secondary = handler.Drain();
    assert(secondary.empty());
}

void TestMultiLaneBinding() {
    // Bind SDLK_SPACE to both Lane 0 and Lane 2
    LaneKeyBindings bindings = DefaultLaneKeyBindings();
    bindings[0][0] = SDLK_SPACE;
    bindings[2][0] = SDLK_SPACE;

    LaneInputHandler handler(Game::UnitBounds{.min = {.x = 0,.y = 0}, .max = {.x = 0,.y = 0}}, bindings);

    [[maybe_unused]] const bool processed = handler.OnKeyDown(SDLK_SPACE, 5000);
    assert(processed);

    const std::vector<LanePress> events = handler.Drain();
    // Both lane 0 and lane 2 should receive the event
    assert(events.size() == 2);

    assert(events[0].lane == 0);
    assert(events[0].sdlTimestampNs == 5000);

    assert(events[1].lane == 2);
    assert(events[1].sdlTimestampNs == 5000);
}

void TestSetBindings() {
    LaneInputHandler handler;
    LaneKeyBindings newBindings = DefaultLaneKeyBindings();
    newBindings[0][0] = SDLK_K;
    newBindings[0][1] = SDLK_UNKNOWN; // normalized to defaults[0][1] (which is SDLK_A)

    handler.SetBindings(newBindings);
    [[maybe_unused]] const LaneKeyBindings& current = handler.Bindings();
    assert(current[0][0] == SDLK_K);
    assert(current[0][1] == SDLK_A);
}

} // namespace

int main() {
    TestBindingsNormalization();
    TestKeyDownAndDrain();
    TestMultiLaneBinding();
    TestSetBindings();
    return 0;
}
