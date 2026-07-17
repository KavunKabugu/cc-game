#ifndef CC_GAME_GAMEPLAY_MATH_H
#define CC_GAME_GAMEPLAY_MATH_H

#include <algorithm>
#include <numbers>

#include "GameplayConstants.h"

namespace Game::Gameplay {

// Uniform scale of the active logical resolution vs the reference (kLogicalWidth × kLogicalHeight).
// Crosshair radius from GameplaySettings is defined at reference resolution, multiply by this factor
// and pass the real logical width into NoteSimulation so z-space and on-screen radii stay aligned.
constexpr float ResolutionUniformScale(const int logicalWidth, const int logicalHeight) noexcept {
    const float sx = static_cast<float>(logicalWidth) / static_cast<float>(kLogicalWidth);
    const float sy = static_cast<float>(logicalHeight) / static_cast<float>(kLogicalHeight);
    return std::min(sx, sy);
}

// Convert lane id (0..3) to its arc angle in degrees.
constexpr float LaneToAngleDegrees(const int lane) {
    if (lane < 0 || lane > 3) return 0.0f;
    return kLaneAngleDegrees[lane];
}

// Crosshair radius -> z-location such that an arc at noteTime reaches that radius.
constexpr float CrosshairZLocation(const float crosshairRadius, const int screenWidth = kLogicalWidth) {
    if (crosshairRadius <= 0.0f) return 0.0f;
    return kArcReferenceWidth / crosshairRadius * (static_cast<float>(screenWidth) * 0.5f);
}

// noteSpeed (player setting) -> z-units per millisecond used by the formulas below.
constexpr float SpeedZPerMs(const float noteSpeed) {
    return noteSpeed / kSpeedDivisor;
}

// z = (currentTimeMs - noteTimeMs) * speed + crosshairZ.
// At currentTime == noteTime, z == crosshairZ, so radius == crosshair radius.
constexpr float ZLocation(
    const double currentTimeMs, const double noteTimeMs,
    const float speedZPerMs, const float crosshairZ) {
    return static_cast<float>(currentTimeMs - noteTimeMs) * speedZPerMs + crosshairZ;
}

// Radius of an arc with the given z-location, in pixels at the given screen width.
constexpr float ArcRadius(const float zLocation, const int screenWidth = kLogicalWidth) {
    if (zLocation <= 0.0f) return 0.0f;
    return kArcReferenceWidth / zLocation * (static_cast<float>(screenWidth) * 0.5f);
}

// Time (in seconds) when an arc first becomes visible (Pulsarc-style IsSeenAt).
// I love stealing code!
constexpr double SpawnTimeSeconds(
    const double noteTimeSeconds, const float speedZPerMs, const float crosshairZ) {
    if (speedZPerMs <= 0.0f) return noteTimeSeconds;
    const double leadMs = static_cast<double>(crosshairZ) / static_cast<double>(speedZPerMs);
    return noteTimeSeconds - leadMs * 1e-3;
}

// Quarter-arc texture rotation in degrees for a given direction angle.
constexpr float ArcRotationDegrees(float angleDegrees) {
    while (angleDegrees < 0.0f) angleDegrees += 360.0f;
    while (angleDegrees >= 360.0f) angleDegrees -= 360.0f;

    if (angleDegrees >= 315.0f || angleDegrees < 45.0f) return kArcRotationRightDegrees;
    if (angleDegrees < 135.0f) return kArcRotationBottomDegrees;
    if (angleDegrees < 225.0f) return kArcRotationLeftDegrees;
    return kArcRotationTopDegrees;
}

constexpr double DegToRad(const double degrees) {
    return degrees * std::numbers::pi / 180.0;
}

} // namespace Game::Gameplay

#endif // CC_GAME_GAMEPLAY_MATH_H
