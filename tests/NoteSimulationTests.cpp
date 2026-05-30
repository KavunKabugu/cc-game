#include <cassert>
#include <cmath>
#include <vector>
#include "Game/Gameplay/NoteSimulation.h"
#include "Game/Gameplay/GameplayConstants.h"

namespace {

using namespace Game::Gameplay;

void TestConfiguration() {
    NoteSimulation sim;
    sim.Configure(10.0f, 150.0f, 1920);

    assert(sim.ScreenWidth() == 1920);
    assert(sim.CrosshairRadius() == 150.0f);
    assert(sim.CrosshairZLocation() > 0.0f);
}

void TestSpawnAndBoundaries() {
    NoteSimulation sim;
    sim.Configure(10.0f, 150.0f, 1920);

    ChartData chart;
    chart.notes = {
        ChartNote{.hitTime = 1.0, .lane = 0},
        ChartNote{.hitTime = 2.5, .lane = 2}
    };
    sim.LoadChart(chart);

    assert(sim.ChartNoteCount() == 2);
    assert(sim.FirstNoteHitTime() == 1.0);
    assert(sim.LastNoteHitTime() == 2.5);
    assert(!sim.AllNotesResolved());

    const double lead = sim.SpawnLeadSeconds();
    assert(lead > 0.0);

    // Prior to spawn lead time: 0 active notes
    sim.Tick(1.0 - lead - 0.01);
    assert(sim.ActiveNotes().empty());

    // Hit spawn time for 1st note
    sim.Tick(1.0 - lead + 0.01);
    assert(sim.ActiveNotes().size() == 1);
    assert(sim.ActiveNotes()[0]->lane == 0);
    assert(!sim.AllNotesResolved());
}

void TestExpiration() {
    NoteSimulation sim;
    sim.Configure(10.0f, 150.0f, 1920);

    ChartData chart;
    chart.notes = { ChartNote{.hitTime = 1.0, .lane = 0} };
    sim.LoadChart(chart);

    const double lead = sim.SpawnLeadSeconds();
    sim.Tick(1.0 - lead + 0.01);
    assert(sim.ActiveNotes().size() == 1);

    // Advance past expiration limit (kMissExpireSeconds = 0.23 seconds)
    sim.Tick(1.0 + kMissExpireSeconds + 0.01);

    // ActiveNotes should now be empty because it expired
    assert(sim.ActiveNotes().empty());
    assert(sim.AllNotesResolved());

    const auto& events = sim.DrainEvents();
    assert(events.size() == 1);
    assert(events[0].judgement == Judgement::Miss);
    assert(events[0].missReason == MissReason::NoteExpired);
    assert(events[0].lane == 0);
    assert(events[0].noteTargetTimeSeconds.has_value());
    assert(*events[0].noteTargetTimeSeconds == 1.0);
}

void TestTryHitPerfect() {
    NoteSimulation sim;
    sim.Configure(10.0f, 150.0f, 1920);

    ChartData chart;
    chart.notes = { ChartNote{.hitTime = 1.0, .lane = 0} };
    sim.LoadChart(chart);

    sim.Tick(1.0); // Exactly at note time
    assert(sim.ActiveNotes().size() == 1);

    const HitResult res = sim.TryHit(0, 1.0);
    assert(res.judgement == Judgement::Perfect);
    assert(res.missReason == MissReason::None);
    assert(res.lane == 0);
    assert(res.deltaMs == 0.0);

    assert(sim.ActiveNotes().empty());
    assert(sim.AllNotesResolved());
}

void TestTryHitGoodAndBad() {
    NoteSimulation sim;
    sim.Configure(10.0f, 150.0f, 1920);

    ChartData chart;
    chart.notes = {
        ChartNote{.hitTime = 1.0, .lane = 0},
        ChartNote{.hitTime = 2.0, .lane = 1}
    };
    sim.LoadChart(chart);

    // Spawn 1st note
    sim.Tick(1.0);

    // TODO: convert these to values in GameplayConstants.h
    // Good Window is <= 115.0ms (0.115s) and > 17.0ms (0.017s)
    // Hit at 1.0 + 0.050s (50ms delta)
    const HitResult res1 = sim.TryHit(0, 1.050);
    assert(res1.judgement == Judgement::Good);
    assert(std::abs(res1.deltaMs - 50.0) < 1e-6);

    // Spawn 2nd note
    sim.Tick(2.0);

    // Bad Window is > 115.0ms (0.115s) but within hit threshold
    // Hit at 2.0 + 0.200s (200ms delta)
    const HitResult res2 = sim.TryHit(1, 2.200);
    assert(res2.judgement == Judgement::Bad);
    assert(std::abs(res2.deltaMs - 200.0) < 1e-6);
}

void TestEmptyLaneAndChords() {
    NoteSimulation sim;
    sim.Configure(10.0f, 150.0f, 1920);

    ChartData chart;
    chart.notes = {
        ChartNote{.hitTime = 1.0, .lane = 0},
        ChartNote{.hitTime = 1.01, .lane = 0} // Jack/double tap in same lane
    };
    sim.LoadChart(chart);

    sim.Tick(1.0);

    // Try hit empty lane
    const HitResult resEmpty = sim.TryHit(1, 1.0);
    assert(resEmpty.judgement == Judgement::Miss);
    assert(resEmpty.missReason == MissReason::EmptyLane);
    assert(resEmpty.lane == 1);

    // Try hit lane 0: should hit the first note (1.0) because it's closest to hit time
    const HitResult resFirst = sim.TryHit(0, 0.99);
    assert(resFirst.judgement == Judgement::Perfect);
    assert(resFirst.noteTargetTimeSeconds.has_value() && *resFirst.noteTargetTimeSeconds == 1.0);

    // Try hit lane 0 again: should hit the second note (1.01)
    const HitResult resSecond = sim.TryHit(0, 1.01);
    assert(resSecond.judgement == Judgement::Perfect);
    assert(resSecond.noteTargetTimeSeconds.has_value() && *resSecond.noteTargetTimeSeconds == 1.01);
}

} // namespace

int main() {
    TestConfiguration();
    TestSpawnAndBoundaries();
    TestExpiration();
    TestTryHitPerfect();
    TestTryHitGoodAndBad();
    TestEmptyLaneAndChords();
    return 0;
}
