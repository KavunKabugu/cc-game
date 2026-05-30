#include <cassert>
#include <cmath>
#include <vector>

#include "Game/Gameplay/GameplayConstants.h"
#include "Game/Gameplay/ResultsGraphDeltaBarWidth.h"

namespace {

using Game::Gameplay::ConcurrentSlotEpsilonNx;
using Game::Gameplay::kResultsGraphDeltaBarMinWidthPx;
using Game::Gameplay::ResultsGraphDeltaBarWidthPx;

void TestConcurrentOnlyChord() {
    const std::vector nx{0.5f, 0.5f, 0.5f};
    constexpr float base = 5.0f;
    constexpr float plotW = 1000.0f;
    const float w = ResultsGraphDeltaBarWidthPx(
        plotW, base, 0.01f, kResultsGraphDeltaBarMinWidthPx, 1e-6f, nx);
    assert(std::abs(w - std::min(base, plotW * 0.01f)) < 1e-5f);
    assert(w >= kResultsGraphDeltaBarMinWidthPx);
}

void TestDenseDistinct() {
    const std::vector nx{0.0f, 0.1f};
    constexpr float plotW = 100.0f;
    constexpr float base = 5.0f;
    const float w = ResultsGraphDeltaBarWidthPx(
        plotW, base, 0.01f, kResultsGraphDeltaBarMinWidthPx, 1e-6f, nx);
    // Overlap cap 10px, then min(10, plotW*1%) = 1, then floor to min width 2px.
    assert(std::abs(w - kResultsGraphDeltaBarMinWidthPx) < 1e-5f);
}

void TestChordPlusJack() {
    const std::vector nx{0.5f, 0.5f, 0.51f};
    constexpr float plotW = 1000.0f;
    constexpr float base = 5.0f;
    const float w = ResultsGraphDeltaBarWidthPx(
        plotW, base, 0.01f, kResultsGraphDeltaBarMinWidthPx, 1e-6f, nx);
    assert(std::abs(w - 5.0f) < 1e-5f);
}

void TestEmptyAndSingle() {
    constexpr std::vector<float> empty{};
    constexpr float base = 4.0f;
    constexpr float plotW = 200.0f;
    float w = ResultsGraphDeltaBarWidthPx(
        plotW, base, 0.01f, kResultsGraphDeltaBarMinWidthPx, 1e-6f, empty);
    assert(std::abs(w - 2.0f) < 1e-5f);

    const std::vector one{0.3f};
    w = ResultsGraphDeltaBarWidthPx(
        plotW, base, 0.01f, kResultsGraphDeltaBarMinWidthPx, 1e-6f, one);
    assert(std::abs(w - 2.0f) < 1e-5f);
}

void TestMinWidthFloor() {
    const std::vector nx{0.0f, 0.001f};
    constexpr float plotW = 100.0f;
    constexpr float base = 5.0f;
    const float w = ResultsGraphDeltaBarWidthPx(
        plotW, base, 0.01f, kResultsGraphDeltaBarMinWidthPx, 1e-6f, nx);
    assert(std::abs(w - kResultsGraphDeltaBarMinWidthPx) < 1e-5f);
}

void TestEpsilonNxFromDomain() {
    const float e = ConcurrentSlotEpsilonNx(0.0, 10.0);
    constexpr float expected = 0.5e-3 / 10.0;
    assert(std::abs(e - expected) < 1e-7f);
}

} // namespace

int main() {
    TestConcurrentOnlyChord();
    TestDenseDistinct();
    TestChordPlusJack();
    TestEmptyAndSingle();
    TestMinWidthFloor();
    TestEpsilonNxFromDomain();
    return 0;
}
