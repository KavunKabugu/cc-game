#ifndef CC_GAME_LANE_INPUT_HANDLER_H
#define CC_GAME_LANE_INPUT_HANDLER_H

#include <array>
#include <vector>

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_stdinc.h>

#include "Game/Events/Interfaces.h"
#include "Game/objects/GameObject.h"

namespace Game::Gameplay {

struct LanePress {
    int lane = -1;
    Uint64 sdlTimestampNs = 0; // SDL event timestamp (SDL_GetTicksNS clock).
};

// Four lanes, two physical key slots per lane. The same keycode may appear on
// multiple lanes, a single key press triggers every matching lane once.
inline constexpr int kLaneBindingCount = 4;
inline constexpr int kKeysPerLane = 2;
using LaneKeyBindings = std::array<std::array<SDL_Keycode, kKeysPerLane>, kLaneBindingCount>;

[[nodiscard]] LaneKeyBindings DefaultLaneKeyBindings() noexcept;
void NormalizeLaneKeyBindings(LaneKeyBindings& bindings) noexcept;

// Invisible GameObject that listens for the configured keys and queues
// (lane, SDL timestamp) pairs for the gameplay scene to consume each frame.
class LaneInputHandler final : public GameObject, public IKeyHandler {
public:
    explicit LaneInputHandler(
        UnitBounds bounds = UnitBounds{{0.0f, 0.0f}, {0.0f, 0.0f}},
        const LaneKeyBindings &bindings = DefaultLaneKeyBindings());

    void Update() override {}

    IKeyHandler* AsKeyHandler() override { return this; }

    bool OnKeyDown(SDL_Keycode key, Uint64 timestamp) override;
    bool OnKeyUp(SDL_Keycode key, Uint64 timestamp) override;

    [[nodiscard]] std::vector<LanePress> Drain();

    void SetBindings(const LaneKeyBindings &newBindings);
    [[nodiscard]] const LaneKeyBindings& Bindings() const { return mBindings; }

private:
    LaneKeyBindings mBindings{};
    std::vector<LanePress> mPending;
};

} // namespace Game::Gameplay

#endif // CC_GAME_LANE_INPUT_HANDLER_H
