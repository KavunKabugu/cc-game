#include <cassert>
#include "Game/Gameplay/JudgementDisplayColors.h"

namespace {

using namespace Game::Gameplay;

void TestLerpU8() {
    assert(LerpU8(0.0f, 100.0f, 0.0f) == 0);
    assert(LerpU8(0.0f, 100.0f, 1.0f) == 100);
    assert(LerpU8(0.0f, 100.0f, 0.5f) == 50);

    // Clamping checks
    assert(LerpU8(0.0f, 100.0f, -0.5f) == 0);
    assert(LerpU8(0.0f, 100.0f, 1.5f) == 100);
}

void TestTimingRulerMarkerRgb() {
    using enum Judgement;

    // Perfect
    [[maybe_unused]] SDL_Color cPerf = TimingRulerMarkerRgb(Perfect, 0.0);
    assert(cPerf.r == 70 && cPerf.g == 190 && cPerf.b == 255 && cPerf.a == 255);

    // Great
    [[maybe_unused]] SDL_Color cGreat = TimingRulerMarkerRgb(Great, 0.0);
    assert(cGreat.r == 100 && cGreat.g == 255 && cGreat.b == 100 && cGreat.a == 255);

    // Good (at kGreatWindowMs)
    [[maybe_unused]] SDL_Color cGoodStart = TimingRulerMarkerRgb(Good, kGreatWindowMs);
    assert(cGoodStart.r == 30 && cGoodStart.g == 220 && cGoodStart.b == 90 && cGoodStart.a == 255);

    // Good (at kGoodWindowMs)
    [[maybe_unused]] SDL_Color cGoodEnd = TimingRulerMarkerRgb(Good, kGoodWindowMs);
    assert(cGoodEnd.r == 255 && cGoodEnd.g == 230 && cGoodEnd.b == 55 && cGoodEnd.a == 255);

    // Bad (at kGoodWindowMs)
    [[maybe_unused]] SDL_Color cBadStart = TimingRulerMarkerRgb(Bad, kGoodWindowMs);
    assert(cBadStart.r == 255 && cBadStart.g == 145 && cBadStart.b == 45 && cBadStart.a == 255);

    // Bad (at kTimingRulerHalfWindowMs)
    [[maybe_unused]] SDL_Color cBadEnd = TimingRulerMarkerRgb(Bad, kTimingRulerHalfWindowMs);
    assert(cBadEnd.r == 255 && cBadEnd.g == 45 && cBadEnd.b == 45 && cBadEnd.a == 255);

    // Miss
    [[maybe_unused]] SDL_Color cMiss = TimingRulerMarkerRgb(Miss, 0.0);
    assert(cMiss.a == 0);
}

void TestResultsAccuracyGraphFillColor() {
    [[maybe_unused]] constexpr SDL_Color greenAcc{.r = 120, .g = 210, .b = 160, .a = 255};
    [[maybe_unused]] constexpr SDL_Color redAcc{.r = 220, .g = 75, .b = 95, .a = 255};

    // Acc >= 90% (kAccuracyGraphGradientMax)
    [[maybe_unused]] SDL_Color cGreen = ResultsAccuracyGraphFillColor(95.0);
    assert(cGreen.r == greenAcc.r && cGreen.g == greenAcc.g && cGreen.b == greenAcc.b);

    // Acc <= 50% (kAccuracyGraphGradientMin)
    [[maybe_unused]] SDL_Color cRed = ResultsAccuracyGraphFillColor(45.0);
    assert(cRed.r == redAcc.r && cRed.g == redAcc.g && cRed.b == redAcc.b);

    // Interpolation (70% - midpoint between 50 and 90)
    [[maybe_unused]] SDL_Color cMid = ResultsAccuracyGraphFillColor(70.0);
    assert(cMid.r == LerpU8(greenAcc.r, redAcc.r, 0.5f));
    assert(cMid.g == LerpU8(greenAcc.g, redAcc.g, 0.5f));
    assert(cMid.b == LerpU8(greenAcc.b, redAcc.b, 0.5f));
}

void TestResultsJudgementFillColor() {
    // Miss
    [[maybe_unused]] SDL_Color cMiss = ResultsJudgementFillColor(Judgement::Miss, 0.0);
    assert(cMiss.r == 220 && cMiss.g == 75 && cMiss.b == 85 && cMiss.a == 255);

    // Perfect (delegates to TimingRulerMarkerRgb)
    [[maybe_unused]] SDL_Color cPerf = ResultsJudgementFillColor(Judgement::Perfect, 0.0);
    assert(cPerf.r == 70 && cPerf.g == 190 && cPerf.b == 255 && cPerf.a == 255);
}

} // namespace

int main() {
    TestLerpU8();
    TestTimingRulerMarkerRgb();
    TestResultsAccuracyGraphFillColor();
    TestResultsJudgementFillColor();
    return 0;
}
