#include <cassert>

#include "Game/Gameplay/HitScoring.h"

namespace {

using Game::Gameplay::AccuracyNumeratorContribution;
using Game::Gameplay::ContinuousRawScore;
using Game::Gameplay::HitResult;
using Game::Gameplay::Judgement;
using Game::Gameplay::MissReason;
using Game::Gameplay::ScoreForHitResult;
using Game::Gameplay::kBadScoreGraceEndMs;
using Game::Gameplay::kBadScorePenaltyEndMs;
using Game::Gameplay::kGoodWindowMs;
using Game::Gameplay::kPerfectWindowMs;
using Game::Gameplay::kGreatWindowMs;

void TestContinuousVersusCeilGood() {
    constexpr double d = kGreatWindowMs + 1.5;
    constexpr double span = kGoodWindowMs - kGreatWindowMs;
    [[maybe_unused]] constexpr double raw = 100.0 * (kGoodWindowMs - d) / span;
    [[maybe_unused]] constexpr HitResult hit{.judgement = Judgement::Good, .deltaMs = d};
    assert(ContinuousRawScore(hit) == raw);
    assert(ScoreForHitResult(hit) == static_cast<std::int32_t>(std::ceil(raw)));
    assert(ScoreForHitResult(hit) == 98);
}

void TestContinuousVersusCeilBad() {
    constexpr double d = kBadScoreGraceEndMs + 13.0;
    [[maybe_unused]] constexpr double raw =
        -25.0 * (d - kBadScoreGraceEndMs) / (kBadScorePenaltyEndMs - kBadScoreGraceEndMs);
    [[maybe_unused]] constexpr HitResult hit{.judgement = Judgement::Bad, .deltaMs = d};
    assert(std::abs(ContinuousRawScore(hit) - raw) < 1e-9);
    assert(ScoreForHitResult(hit) == static_cast<std::int32_t>(std::ceil(raw)));
    assert(ScoreForHitResult(hit) == -3);
}

void TestAccuracyNumeratorContribution() {
    assert(AccuracyNumeratorContribution(HitResult{
               .judgement = Judgement::Miss,
               .missReason = MissReason::EmptyLane,
               .lane = 0,
               .deltaMs = 0.0,
           }) == 0.0);

    assert(AccuracyNumeratorContribution(HitResult{
               .judgement = Judgement::Miss,
               .missReason = MissReason::NoteExpired,
               .lane = 1,
               .deltaMs = 500.0,
           }) == 0.0);

    assert(AccuracyNumeratorContribution(HitResult{
               .judgement = Judgement::Perfect,
               .deltaMs = 0.0,
           }) == 100.0);

    assert(AccuracyNumeratorContribution(HitResult{
               .judgement = Judgement::Great,
               .deltaMs = 0.0,
           }) == 100.0);
}

void TestRunningAccuracyPercentFormula() {
    constexpr double sum = 150.0;
    constexpr std::size_t processed = 2;
    [[maybe_unused]] constexpr double pct = 100.0 * sum / (static_cast<double>(processed) * 100.0);
    assert(std::abs(pct - 75.0) < 1e-9);

    [[maybe_unused]] constexpr double sumOne = 100.0;
    assert(std::abs(100.0 * sumOne / (1.0 * 100.0) - 100.0) < 1e-9);
}

} // namespace

int main() {
    TestContinuousVersusCeilGood();
    TestContinuousVersusCeilBad();
    TestAccuracyNumeratorContribution();
    TestRunningAccuracyPercentFormula();
    return 0;
}
