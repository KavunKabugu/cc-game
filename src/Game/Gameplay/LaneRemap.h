#ifndef CC_GAME_LANE_REMAP_H
#define CC_GAME_LANE_REMAP_H

namespace Game::Gameplay {

// osu!mania 4K columns are left-to-right: Left, Down, Up, Right (indices 0..3).
// Game lanes are compass order: Left, Up, Right, Down (indices 0..3).
inline constexpr int kOsuToCompassLane[4] = {0, 3, 1, 2};

[[nodiscard]] inline int RemapOsuLaneToCompass(const int lane) noexcept {
    if (lane < 0 || lane > 3) {
        return lane;
    }
    return kOsuToCompassLane[lane];
}

// Swap Up (1) and Down (3), leave Left (0) and Right (2) unchanged.
[[nodiscard]] inline int SwapUpDownLane(const int lane) noexcept {
    if (lane == 1) {
        return 3;
    }
    if (lane == 3) {
        return 1;
    }
    return lane;
}

template <typename NoteRange>
void ApplyOsuToCompassRemap(NoteRange& notes) {
    for (auto& note : notes) {
        note.lane = RemapOsuLaneToCompass(note.lane);
    }
}

template <typename NoteRange>
void ApplySwapUpDownLanes(NoteRange& notes) {
    for (auto& note : notes) {
        note.lane = SwapUpDownLane(note.lane);
    }
}

} // namespace Game::Gameplay

#endif // CC_GAME_LANE_REMAP_H
