#include <cassert>

#include "Game/Gameplay/GameplayConstants.h"
#include "Game/Gameplay/TimingRulerMath.h"

namespace {

using Game::Gameplay::ClampDeltaMs;
using Game::Gameplay::MarkerCenterX;
using Game::Gameplay::MarkerDisplayAlpha;
using Game::Gameplay::NormalizedRulerT;
using Game::Gameplay::kTimingRulerHalfWindowMs;

void TestClamp() {
    [[maybe_unused]] constexpr double half = kTimingRulerHalfWindowMs;
    assert(ClampDeltaMs(-(half + 50.0), half) == -half);
    assert(ClampDeltaMs(half + 50.0, half) == half);
    assert(ClampDeltaMs(42.0, half) == 42.0);
    assert(ClampDeltaMs(-0.5, half) == -0.5);
}

void TestNormalizedT() {
    [[maybe_unused]] constexpr double half = kTimingRulerHalfWindowMs;
    assert(NormalizedRulerT(0.0, half) == 0.0f);
    assert(NormalizedRulerT(half, half) == 1.0f);
    assert(NormalizedRulerT(-half, half) == -1.0f);
    assert(std::abs(NormalizedRulerT(0.5 * half, half) - 0.5f) < 1e-6f);
    assert(std::abs(NormalizedRulerT(-0.5 * half, half) + 0.5f) < 1e-6f);
}

void TestAlpha() {
    [[maybe_unused]] constexpr float base = 100.0f;
    assert(MarkerDisplayAlpha(0.0, 0.5, 1.0, base) == base);
    assert(MarkerDisplayAlpha(0.25, 0.5, 1.0, base) == base);
    assert(MarkerDisplayAlpha(0.5, 0.5, 1.0, base) == base);
    assert(MarkerDisplayAlpha(1.0, 0.5, 1.0, base) == 50.0f);
    assert(MarkerDisplayAlpha(1.5, 0.5, 1.0, base) == 0.0f);
    assert(MarkerDisplayAlpha(2.0, 0.5, 1.0, base) == 0.0f);
}

void TestMarkerCenterX() {
    assert(std::abs(MarkerCenterX(100.0f, 10.0f, 0.0f) - 50.0f) < 1e-5f);
    assert(std::abs(MarkerCenterX(100.0f, 10.0f, -1.0f) - 10.0f) < 1e-5f);
    assert(std::abs(MarkerCenterX(100.0f, 10.0f, 1.0f) - 90.0f) < 1e-5f);
}

} // namespace

int main() {
    TestClamp();
    TestNormalizedT();
    TestAlpha();
    TestMarkerCenterX();
    return 0;
}
