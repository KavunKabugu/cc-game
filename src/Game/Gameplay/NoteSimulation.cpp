#include "NoteSimulation.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "GameplayMath.h"

namespace Game::Gameplay {

void NoteSimulation::Configure(const float speed, const float radius, const int width) {
    noteSpeed = speed;
    speedZPerMs = SpeedZPerMs(speed);
    crosshairRadius = radius;
    screenWidth = width;
    crosshairZ = Gameplay::CrosshairZLocation(radius, width);
}

void NoteSimulation::LoadChart(const ChartData& chart) {
    notes.clear();
    active.clear();
    activeView.clear();
    events.clear();
    nextSpawnIndex = 0;

    notes.reserve(chart.notes.size());
    for (const auto& [hitTime, lane] : chart.notes) {
        notes.push_back(RuntimeNote{.hitTime = hitTime, .lane = lane});
    }
}

void NoteSimulation::Tick(const double currentTimeSeconds) {
    // Spawn notes whose IsSeenAt has passed.
    while (nextSpawnIndex < notes.size()) {
        RuntimeNote& candidate = notes[nextSpawnIndex];
        if (const double spawnTime = SpawnTimeSeconds(candidate.hitTime, speedZPerMs, crosshairZ); currentTimeSeconds < spawnTime) break;
        candidate.active = true;
        active.push_back(&candidate);
        ++nextSpawnIndex;
    }

    if (active.empty()) {
        activeView.clear();
        return;
    }

    const double currentTimeMs = currentTimeSeconds * 1000.0;
    auto it = active.begin();
    while (it != active.end()) {
        RuntimeNote* note = *it;
        const double noteTimeMs = note->hitTime * 1000.0;
        note->zLocation = ZLocation(currentTimeMs, noteTimeMs, speedZPerMs, crosshairZ);

        const double timeSinceHit = currentTimeSeconds - note->hitTime;
        const bool expired = timeSinceHit > kMissExpireSeconds;
        if (const bool degenerate = note->zLocation <= 0.0f; (expired || degenerate) && !note->resolved) {
            note->resolved = true;
            note->active = false;
            note->judgement = Judgement::Miss;
            events.push_back(HitResult{
                .judgement = Judgement::Miss,
                .missReason = MissReason::NoteExpired,
                .lane = note->lane,
                .deltaMs = timeSinceHit * 1000.0,
                .noteTargetTimeSeconds = note->hitTime,
                .pressTimeSeconds = currentTimeSeconds,
            });
            it = active.erase(it);
            continue;
        }
        ++it;
    }

    RebuildActiveView();
}

HitResult NoteSimulation::TryHit(const int lane, const double hitTimeSeconds) {
    RuntimeNote* best = nullptr;
    double bestAbs = 0.0;
    double bestDelta = 0.0;

    for (RuntimeNote* note : active) {
        if (note->lane != lane || note->resolved) continue;
        const double deltaSec = hitTimeSeconds - note->hitTime;
        if (const double absSec = std::abs(deltaSec); best == nullptr || absSec < bestAbs || (absSec == bestAbs && note->hitTime < best->hitTime)) {
            best = note;
            bestAbs = absSec;
            bestDelta = deltaSec;
        }
    }

    if (best == nullptr) {
        const HitResult result{
            .judgement = Judgement::Miss,
            .missReason = MissReason::EmptyLane,
            .lane = lane,
            .deltaMs = 0.0,
            .noteTargetTimeSeconds = {},
            .pressTimeSeconds = hitTimeSeconds,
        };
        events.push_back(result);
        return result;
    }

    const double absMs = bestAbs * 1000.0;
    Judgement tier;
    if (absMs <= kPerfectWindowMs) {
        tier = Judgement::Perfect;
    } else if (absMs <= kGoodWindowMs) {
        tier = Judgement::Good;
    } else {
        tier = Judgement::Bad;
    }

    best->resolved = true;
    best->active = false;
    best->judgement = tier;

    const HitResult result{
        .judgement = tier,
        .missReason = MissReason::None,
        .lane = lane,
        .deltaMs = bestDelta * 1000.0,
        .noteTargetTimeSeconds = best->hitTime,
        .pressTimeSeconds = hitTimeSeconds,
    };
    events.push_back(result);

    std::erase(active, best);
    RebuildActiveView();
    return result;
}

void NoteSimulation::RebuildActiveView() {
    activeView.clear();
    activeView.reserve(active.size());
    for (const RuntimeNote* note : active) {
        activeView.push_back(note);
    }
}

bool NoteSimulation::AllNotesResolved() const {
    if (nextSpawnIndex < notes.size()) return false;
    return std::ranges::all_of(notes, [](const RuntimeNote& n) { return n.resolved; });
}

double NoteSimulation::SpawnLeadSeconds() const {
    if (speedZPerMs <= 0.0f || crosshairZ <= 0.0f) return 0.0;
    return static_cast<double>(crosshairZ) / static_cast<double>(speedZPerMs) * 1e-3;
}

double NoteSimulation::FirstNoteHitTime() const {
    if (notes.empty()) return std::numeric_limits<double>::infinity();
    return notes.front().hitTime; // notes are sorted ascending in LoadChart.
}

double NoteSimulation::LastNoteHitTime() const {
    if (notes.empty()) return -std::numeric_limits<double>::infinity();
    return notes.back().hitTime;
}

} // namespace Game::Gameplay
