#include <cassert>

#include "Game/Gameplay/HitScoring.h"

namespace {

using Game::Gameplay::ContinuousRawScore;
using Game::Gameplay::HitResult;
using Game::Gameplay::Judgement;
using Game::Gameplay::MissReason;
using Game::Gameplay::ScoreForHitResult;
using Game::Gameplay::kBadScoreGraceEndMs;
using Game::Gameplay::kBadScorePenaltyEndMs;
using Game::Gameplay::kGoodWindowMs;
using Game::Gameplay::kGreatWindowMs;
using Game::Gameplay::kPerfectWindowMs;

void AssertCeilScore([[maybe_unused]] const HitResult& h) {
    assert(ScoreForHitResult(h) == static_cast<std::int32_t>(std::ceil(ContinuousRawScore(h))));
}

void TestMiss() {
    assert(ScoreForHitResult(HitResult{
               .judgement = Judgement::Miss,
               .missReason = MissReason::EmptyLane,
               .lane = 0,
               .deltaMs = 0.0,
           }) == -25);
    assert(ScoreForHitResult(HitResult{
               .judgement = Judgement::Miss,
               .missReason = MissReason::NoteExpired,
               .lane = 2,
               .deltaMs = 999.0,
           }) == -25);
}

void TestPerfect() {
    assert(ScoreForHitResult(HitResult{.judgement = Judgement::Perfect, .deltaMs = 0.0}) == 100);
    assert(ScoreForHitResult(HitResult{.judgement = Judgement::Perfect, .deltaMs = kPerfectWindowMs}) == 100);
    assert(ScoreForHitResult(HitResult{.judgement = Judgement::Perfect, .deltaMs = -kPerfectWindowMs}) == 100);
}

void TestGreat() {
    assert(ScoreForHitResult(HitResult{.judgement = Judgement::Great, .deltaMs = 0.0}) == 100);
    assert(ScoreForHitResult(HitResult{.judgement = Judgement::Great, .deltaMs = kGreatWindowMs}) == 100);
    assert(ScoreForHitResult(HitResult{.judgement = Judgement::Great, .deltaMs = -kGreatWindowMs}) == 100);
}

void TestGood() {
    AssertCeilScore(HitResult{.judgement = Judgement::Good, .deltaMs = kGreatWindowMs + 0.5});
    AssertCeilScore(HitResult{.judgement = Judgement::Good, .deltaMs = kGreatWindowMs + 1.0});

    constexpr double dHalfScore = 0.5 * (kGoodWindowMs + kGreatWindowMs);
    [[maybe_unused]] constexpr HitResult mid{.judgement = Judgement::Good, .deltaMs = dHalfScore};
    assert(std::abs(ContinuousRawScore(mid) - 50.0) < 1e-9);
    assert(ScoreForHitResult(mid) == 50);

    [[maybe_unused]] constexpr HitResult atOuter{.judgement = Judgement::Good, .deltaMs = kGoodWindowMs};
    assert(ContinuousRawScore(atOuter) == 0.0);
    assert(ScoreForHitResult(atOuter) == 0);
    assert(ScoreForHitResult(HitResult{.judgement = Judgement::Good, .deltaMs = -kGoodWindowMs}) == 0);
}

void TestBad() {
    [[maybe_unused]] constexpr HitResult inGrace{.judgement = Judgement::Bad, .deltaMs = kGoodWindowMs + 1.0};
    assert(ContinuousRawScore(inGrace) == 0.0);
    assert(ScoreForHitResult(inGrace) == 0);

    [[maybe_unused]] constexpr HitResult atGraceEnd{.judgement = Judgement::Bad, .deltaMs = kBadScoreGraceEndMs};
    assert(ContinuousRawScore(atGraceEnd) == 0.0);
    assert(ScoreForHitResult(atGraceEnd) == 0);

    AssertCeilScore(HitResult{.judgement = Judgement::Bad, .deltaMs = kBadScoreGraceEndMs + 1.0});

    constexpr double midPenalty =
        kBadScoreGraceEndMs + 0.5 * (kBadScorePenaltyEndMs - kBadScoreGraceEndMs);
    AssertCeilScore(HitResult{.judgement = Judgement::Bad, .deltaMs = midPenalty});

    [[maybe_unused]] constexpr HitResult atCap{.judgement = Judgement::Bad, .deltaMs = kBadScorePenaltyEndMs};
    assert(ScoreForHitResult(atCap) == -25);

    [[maybe_unused]] constexpr HitResult pastCap{.judgement = Judgement::Bad, .deltaMs = kBadScorePenaltyEndMs + 40.0};
    assert(ScoreForHitResult(pastCap) == -25);

    AssertCeilScore(HitResult{.judgement = Judgement::Bad, .deltaMs = -150.0});
}

} // namespace

int main() {
    TestMiss();
    TestPerfect();
    TestGreat();
    TestGood();
    TestBad();
    return 0;
}
