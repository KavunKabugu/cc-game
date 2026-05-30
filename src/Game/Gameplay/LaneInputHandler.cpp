#include "LaneInputHandler.h"

namespace Game::Gameplay {

LaneKeyBindings DefaultLaneKeyBindings() noexcept {
    // Lanes follow the chart's lane numbering:
    //   0 = "left",  1 = "top",  2 = "right",  3 = "bottom".
    return LaneKeyBindings{{
        {SDLK_LEFT, SDLK_A},
        {SDLK_UP, SDLK_W},
        {SDLK_RIGHT, SDLK_D},
        {SDLK_DOWN, SDLK_S},
    }};
}

void NormalizeLaneKeyBindings(LaneKeyBindings& bindings) noexcept {
    const LaneKeyBindings defaults = DefaultLaneKeyBindings();
    for (int lane = 0; lane < kLaneBindingCount; ++lane) {
        for (int slot = 0; slot < kKeysPerLane; ++slot) {
            if (const SDL_Keycode k = bindings[static_cast<size_t>(lane)][static_cast<size_t>(slot)]; k == SDLK_UNKNOWN || static_cast<int>(k) == 0) {
                bindings[static_cast<size_t>(lane)][static_cast<size_t>(slot)] =
                    defaults[static_cast<size_t>(lane)][static_cast<size_t>(slot)];
            }
        }
    }
}

LaneInputHandler::LaneInputHandler(const UnitBounds bounds, const LaneKeyBindings &bindings)
    : GameObject(bounds), mBindings(bindings) {
    NormalizeLaneKeyBindings(mBindings);
}

bool LaneInputHandler::OnKeyDown(const SDL_Keycode key, const Uint64 timestamp) {
    bool anyLane = false;
    for (int lane = 0; lane < kLaneBindingCount; ++lane) {
        bool laneMatches = false;
        for (int slot = 0; slot < kKeysPerLane; ++slot) {
            if (mBindings[static_cast<size_t>(lane)][static_cast<size_t>(slot)] == key) {
                laneMatches = true;
                break;
            }
        }
        if (laneMatches) {
            mPending.push_back(LanePress{.lane = lane, .sdlTimestampNs = timestamp});
            anyLane = true;
        }
    }
    return anyLane;
}

bool LaneInputHandler::OnKeyUp(SDL_Keycode, Uint64) {
    return false;
}

std::vector<LanePress> LaneInputHandler::Drain() {
    if (mPending.empty()) {
        return {};
    }
    std::vector<LanePress> out;
    out.swap(mPending);
    return out;
}

void LaneInputHandler::SetBindings(const LaneKeyBindings &newBindings) {
    mBindings = newBindings;
    NormalizeLaneKeyBindings(mBindings);
}

} // namespace Game::Gameplay
