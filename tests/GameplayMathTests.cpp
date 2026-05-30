#include <cassert>
#include <cmath>
#include <numbers>

#include "Game/Gameplay/GameplayMath.h"
#include "Game/Gameplay/GameplayConstants.h"

namespace {

using namespace Game::Gameplay;

void TestResolutionUniformScale() {
    // Reference resolution is 1920x1080.
    assert(ResolutionUniformScale(1920, 1080) == 1.0f);
    assert(ResolutionUniformScale(960, 540) == 0.5f);
    assert(ResolutionUniformScale(1920, 540) == 0.5f); // scaled by min aspect dimension
    assert(ResolutionUniformScale(960, 1080) == 0.5f);
}

void TestLaneToAngleDegrees() {
    assert(LaneToAngleDegrees(0) == kLaneAngleDegrees[0]); // Left
    assert(LaneToAngleDegrees(1) == kLaneAngleDegrees[1]); // Top
    assert(LaneToAngleDegrees(2) == kLaneAngleDegrees[2]); // Right
    assert(LaneToAngleDegrees(3) == kLaneAngleDegrees[3]); // Bottom
    assert(LaneToAngleDegrees(-1) == 0.0f);
    assert(LaneToAngleDegrees(4) == 0.0f);
}

void TestCrosshairZLocation() {
    assert(CrosshairZLocation(0.0f, 1920) == 0.0f);
    assert(CrosshairZLocation(-1.0f, 1920) == 0.0f);

    constexpr float expectedZ = (kArcReferenceWidth / 100.0f) * (1920.0f * 0.5f);
    assert(std::abs(CrosshairZLocation(100.0f, 1920) - expectedZ) < 1e-5f);
}

void TestSpeedZPerMs() {
    assert(SpeedZPerMs(4.0f) == 4.0f / kSpeedDivisor);
}

void TestZLocation() {
    constexpr float speedZ = SpeedZPerMs(4.0f);
    constexpr float crosshairZ = CrosshairZLocation(150.0f, 1920);

    // At note time, depth is exactly crosshairZ
    assert(std::abs(ZLocation(1000.0, 1000.0, speedZ, crosshairZ) - crosshairZ) < 1e-5f);

    // 100ms before hit, depth should be smaller than crosshairZ
    constexpr float zBefore = ZLocation(900.0, 1000.0, speedZ, crosshairZ);
    assert(zBefore < crosshairZ);
    assert(std::abs(zBefore - (-100.0f * speedZ + crosshairZ)) < 1e-5f);
}

void TestArcRadius() {
    assert(ArcRadius(0.0f, 1920) == 0.0f);
    assert(ArcRadius(-5.0f, 1920) == 0.0f);

    constexpr float z = CrosshairZLocation(150.0f, 1920);
    // At crosshairZ, radius should be exactly crosshairRadius (150.0f)
    assert(std::abs(ArcRadius(z, 1920) - 150.0f) < 1e-4f);
}

void TestSpawnTimeSeconds() {
    constexpr float speedZ = SpeedZPerMs(4.0f);
    constexpr float crosshairZ = CrosshairZLocation(150.0f, 1920);
    constexpr double noteTime = 2.0;

    constexpr double leadMs = static_cast<double>(crosshairZ) / static_cast<double>(speedZ);
    constexpr double expectedSpawn = noteTime - leadMs * 1e-3;

    assert(std::abs(SpawnTimeSeconds(noteTime, speedZ, crosshairZ) - expectedSpawn) < 1e-6);
    assert(SpawnTimeSeconds(noteTime, 0.0f, crosshairZ) == noteTime);
}

void TestArcRotationDegrees() {
    // Rotations: Right (0/360), Bottom (90), Left (180), Top (270)
    // 0 degrees -> Right
    assert(ArcRotationDegrees(0.0f) == kArcRotationRightDegrees);
    assert(ArcRotationDegrees(360.0f) == kArcRotationRightDegrees);
    assert(ArcRotationDegrees(-360.0f) == kArcRotationRightDegrees);

    // 90 degrees -> Bottom
    assert(ArcRotationDegrees(90.0f) == kArcRotationBottomDegrees);

    // 180 degrees -> Left
    assert(ArcRotationDegrees(180.0f) == kArcRotationLeftDegrees);

    // 270 degrees -> Top
    assert(ArcRotationDegrees(270.0f) == kArcRotationTopDegrees);

    // Bounds checking / wrapping
    assert(ArcRotationDegrees(44.0f) == kArcRotationRightDegrees);
    assert(ArcRotationDegrees(45.0f) == kArcRotationBottomDegrees);
    assert(ArcRotationDegrees(134.0f) == kArcRotationBottomDegrees);
    assert(ArcRotationDegrees(135.0f) == kArcRotationLeftDegrees);
    assert(ArcRotationDegrees(224.0f) == kArcRotationLeftDegrees);
    assert(ArcRotationDegrees(225.0f) == kArcRotationTopDegrees);
    assert(ArcRotationDegrees(314.0f) == kArcRotationTopDegrees);
    assert(ArcRotationDegrees(315.0f) == kArcRotationRightDegrees);
}

void TestDegToRad() {
    assert(std::abs(DegToRad(180.0) - std::numbers::pi) < 1e-6);
    assert(std::abs(DegToRad(0.0) - 0.0) < 1e-6);
}

} // namespace

int main() {
    TestResolutionUniformScale();
    TestLaneToAngleDegrees();
    TestCrosshairZLocation();
    TestSpeedZPerMs();
    TestZLocation();
    TestArcRadius();
    TestSpawnTimeSeconds();
    TestArcRotationDegrees();
    TestDegToRad();
    return 0;
}
