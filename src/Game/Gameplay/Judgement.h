#ifndef CC_GAME_JUDGEMENT_H
#define CC_GAME_JUDGEMENT_H

#include <optional>

namespace Game::Gameplay {

enum class Judgement {
    Perfect,
    Great,
    Good,
    Bad,
    Miss,
    Count, // Count of Judgement enum, not a valid judgement.
};
static_assert(static_cast<int>(Judgement::Count) == static_cast<int>(Judgement::Miss) + 1);

// Why a Miss happened (only meaningful when Judgement == Miss).
enum class MissReason {
    None,
    NoteExpired, // Note left the hittable area without being hit.
    EmptyLane,   // Player pressed a lane key with no hittable note in that lane.
};

// One judgement result produced by the simulation. Stored for stats/UI.
struct HitResult {
    Judgement judgement = Judgement::Miss;
    MissReason missReason = MissReason::None;
    int lane = -1;
    double deltaMs = 0.0; // Signed, hitTime - noteTime in ms. 0 if no note.
    // Chart-relative seconds for graphs, unset for EmptyLane.
    std::optional<double> noteTargetTimeSeconds;
    // Press time when judged (chart-relative), Tick misses use currentTime when emitted.
    double pressTimeSeconds = 0.0;
};

} // namespace Game::Gameplay

#endif // CC_GAME_JUDGEMENT_H
