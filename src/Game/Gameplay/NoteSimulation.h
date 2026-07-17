#ifndef CC_GAME_NOTE_SIMULATION_H
#define CC_GAME_NOTE_SIMULATION_H

#include <span>
#include <vector>

#include "ChartData.h"
#include "Judgement.h"

namespace Game::Gameplay {

struct RuntimeNote {
    double hitTime = 0.0; // Seconds, chart-relative.
    int lane = 0;
    bool active = false;        // In the hittable set (between spawn and resolve).
    bool resolved = false;      // Hit or missed, do not consider further.
    Judgement judgement = Judgement::Miss;
    float zLocation = 0.0f;     // Cached current z (for rendering).
};

class NoteSimulation {
public:
    void Configure(float speed, float radius, int width);
    void LoadChart(const ChartData& chart);

    // Advance the simulation to currentTimeSeconds (chart-relative).
    // Returns judgements produced this tick (only Miss/NoteExpired).
    void Tick(double currentTimeSeconds);

    // Resolve a key press in this lane at hit time. Returns a HitResult.
    // If no hittable note matches, returns Miss/EmptyLane.
    HitResult TryHit(int lane, double hitTimeSeconds);

    [[nodiscard]] std::span<const RuntimeNote* const> ActiveNotes() const { return activeView; }
    [[nodiscard]] const std::vector<HitResult>& DrainEvents() { return events; }
    void ClearEvents() { events.clear(); }

    [[nodiscard]] float CrosshairZLocation() const { return crosshairZ; }
    [[nodiscard]] float CrosshairRadius() const { return crosshairRadius; }
    [[nodiscard]] int ScreenWidth() const { return screenWidth; }
    [[nodiscard]] bool AllNotesResolved() const;

    [[nodiscard]] std::size_t ChartNoteCount() const noexcept { return notes.size(); }

    // Time (seconds) it takes a freshly spawned note to reach the crosshair.
    [[nodiscard]] double SpawnLeadSeconds() const;
    // Smallest hit time across loaded notes, or +infinity if no notes.
    [[nodiscard]] double FirstNoteHitTime() const;
    // Largest hit time across loaded notes, or -infinity if no notes.
    [[nodiscard]] double LastNoteHitTime() const;

private:
    void RebuildActiveView();

    std::vector<RuntimeNote> notes;
    std::vector<RuntimeNote*> active;
    std::vector<const RuntimeNote*> activeView;
    std::vector<HitResult> events;

    std::size_t nextSpawnIndex = 0;
    float noteSpeed = 0.0f;
    float speedZPerMs = 0.0f;
    float crosshairRadius = 0.0f;
    float crosshairZ = 0.0f;
    int screenWidth = 0;
};

} // namespace Game::Gameplay

#endif // CC_GAME_NOTE_SIMULATION_H
