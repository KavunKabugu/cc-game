#include <cassert>
#include <cmath>
#include <limits>

#include "Game/Gameplay/GameplayConstants.h"
#include "Game/Gameplay/MapProgress.h"

namespace {

using namespace Game::Gameplay;

void TestMapEndSongTime() {
    assert(std::abs(MapEndSongTime(10.0) - (10.0 + kMissExpireSeconds)) < 1e-9);
    assert(MapEndSongTime(0.0) == kMissExpireSeconds);
    assert(MapEndSongTime(-1.0) == 0.0);
    assert(MapEndSongTime(-std::numeric_limits<double>::infinity()) == 0.0);
    assert(MapEndSongTime(std::numeric_limits<double>::quiet_NaN()) == 0.0);
}

void TestSampleMapProgress() {
    constexpr double end = 10.0;

    const MapProgressSample leadIn = SampleMapProgress(-1.5, end, false);
    assert(leadIn.playedSeconds == 0.0);
    assert(leadIn.remainingSeconds == end);
    assert(leadIn.fraction == 0.0f);

    const MapProgressSample mid = SampleMapProgress(2.5, end, false);
    assert(std::abs(mid.playedSeconds - 2.5) < 1e-9);
    assert(std::abs(mid.remainingSeconds - 7.5) < 1e-9);
    assert(std::abs(mid.fraction - 0.25f) < 1e-6f);

    const MapProgressSample pastEnd = SampleMapProgress(12.0, end, false);
    assert(pastEnd.playedSeconds == end);
    assert(pastEnd.remainingSeconds == 0.0);
    assert(pastEnd.fraction == 1.0f);

    const MapProgressSample earlyClear = SampleMapProgress(3.0, end, true);
    assert(earlyClear.playedSeconds == end);
    assert(earlyClear.remainingSeconds == 0.0);
    assert(earlyClear.fraction == 1.0f);

    const MapProgressSample empty = SampleMapProgress(1.0, 0.0, false);
    assert(empty.playedSeconds == 0.0);
    assert(empty.remainingSeconds == 0.0);
    assert(empty.fraction == 1.0f);
}

void TestFormatRemainingTime() {
    assert(FormatRemainingTime(0.0) == "00:00");
    assert(FormatRemainingTime(-2.0) == "00:00");
    assert(FormatRemainingTime(0.2) == "00:01");
    assert(FormatRemainingTime(59.1) == "01:00");
    assert(FormatRemainingTime(60.0) == "01:00");
    assert(FormatRemainingTime(90.0) == "01:30");
    assert(FormatRemainingTime(3599.2) == "01:00:00");
    assert(FormatRemainingTime(3600.0) == "01:00:00");
    assert(FormatRemainingTime(3661.0) == "01:01:01");
    assert(FormatRemainingTime(std::numeric_limits<double>::quiet_NaN()) == "00:00");
}

} // namespace

int main() {
    TestMapEndSongTime();
    TestSampleMapProgress();
    TestFormatRemainingTime();
    return 0;
}
