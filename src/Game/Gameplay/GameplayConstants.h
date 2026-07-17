#ifndef CC_GAME_GAMEPLAY_CONSTANTS_H
#define CC_GAME_GAMEPLAY_CONSTANTS_H

#include <SDL3/SDL_pixels.h>

namespace Game::Gameplay {

// Logical playfield reference dimensions (gameplay tuning + arc math are normalized to this size).
inline constexpr int kLogicalWidth = 1920;
inline constexpr int kLogicalHeight = 1080;

// Reference half-width used for the radius <-> z mapping at kLogicalWidth.
inline constexpr float kArcReferenceWidth = 960.0f;

// noteSpeed (units) is divided by this to get z-units per millisecond.
inline constexpr float kSpeedDivisor = 5.0f;

// Pre-music silent delay so the player can settle.
inline constexpr double kStartDelaySeconds = 1.0;

// Post-pause settle period before gameplay and audio resume.
inline constexpr double kResumeGraceSeconds = 1.0;

// Default playfield tuning (options defaults).
inline constexpr float kDefaultNoteSpeed = 10.0f;
inline constexpr float kDefaultCrosshairRadius = 150.0f;

// Gameplay options slider clamps (scroll speed = noteSpeed units).
inline constexpr float kGameplayScrollSpeedMin = 5.0f;
inline constexpr float kGameplayScrollSpeedMax = 100.0f;
inline constexpr float kGameplayCrosshairRadiusMin = 50.0f;
inline constexpr float kGameplayCrosshairRadiusMax = 250.0f;

// Audio hardware latency UI uses milliseconds, stored as seconds in GameplaySettings::audioOffsetSeconds.
inline constexpr float kGameplayAudioOffsetMsMin = -100.0f;
inline constexpr float kGameplayAudioOffsetMsMax = 100.0f;

// Background / playfield border options (opacity stored 0–1, border size stored 0–100 UI units).
inline constexpr float kGameplayOpacityMin = 0.0f;
inline constexpr float kGameplayOpacityMax = 1.0f;
inline constexpr float kGameplayBorderSizeMin = 0.0f;
inline constexpr float kGameplayBorderSizeMax = 100.0f;
inline constexpr float kDefaultBackgroundOpacity = 1.0f;
inline constexpr float kDefaultPlayfieldBorderOpacity = 0.24f;
inline constexpr float kDefaultPlayfieldBorderSize = 16.0f;
inline constexpr unsigned char kDefaultBackgroundColorR = 20;
inline constexpr unsigned char kDefaultBackgroundColorG = 28;
inline constexpr unsigned char kDefaultBackgroundColorB = 42;

// Judgement timing windows (absolute |delta| in milliseconds).
inline constexpr double kPerfectWindowMs = 18.0;
inline constexpr double kGreatWindowMs = 60.0;
inline constexpr double kGoodWindowMs = 135.0;

// How long after its hit time an unhit note stays hittable, past this it is
// auto-resolved as Miss and despawned.
inline constexpr double kMissExpireMs = 230.0;
inline constexpr double kMissExpireSeconds = kMissExpireMs * 1e-3;

// Bottom timing feedback strip: symmetric +-half window (ms), marker visibility.
inline constexpr double kTimingRulerHalfWindowMs = kMissExpireMs;
inline constexpr double kTimingRulerMarkerHoldSeconds = 0.5;
inline constexpr double kTimingRulerMarkerFadeSeconds = 1.0;
inline constexpr float kTimingRulerMarkerBaseAlpha = 0.42f;

// Results graph: vertical cue lines (empty lane / expired miss).
inline constexpr float kResultsGraphCueLineAlpha = 0.35f;

// Results delta graph: bar width from non-overlap of distinct note positions; concurrent
// notes (within this time window on the chart domain) share one horizontal slot for spacing.
inline constexpr float kResultsGraphDeltaBarMinWidthPx = 2.0f;
inline constexpr double kResultsGraphDeltaBarConcurrentEpsSec = 0.5e-3;

// Accuracy graph: gradient color range limits
inline constexpr float kAccuracyGraphGradientMax = 90.0f;
inline constexpr float kAccuracyGraphGradientMin = 50.0f;
inline constexpr float kAccuracyGraphGradientRange = kAccuracyGraphGradientMax - kAccuracyGraphGradientMin;

// Lane -> angle (degrees), direct mapping.
// Lane 0 (Left  key, SDLK_LEFT  / SDLK_A) -> 180° drawn at left
// Lane 1 (Top   key, SDLK_UP    / SDLK_W) -> 270° drawn at top
// Lane 2 (Right key, SDLK_RIGHT / SDLK_D) -> 0°   drawn at right
// Lane 3 (Down  key, SDLK_DOWN  / SDLK_S) -> 90°  drawn at bottom
inline constexpr float kLaneAngleDegrees[4] = {180.0f, 270.0f, 0.0f, 90.0f};

// Quarter-arc texture rotations per direction (degrees).
inline constexpr float kArcRotationTopDegrees = 45.0f;     // 270°
inline constexpr float kArcRotationRightDegrees = 135.0f;  // 0°
inline constexpr float kArcRotationBottomDegrees = 225.0f; // 90°
inline constexpr float kArcRotationLeftDegrees = 315.0f;   // 180°

// Crosshair direction angles used to render the four overlapping quarters.
inline constexpr float kCrosshairAngles[4] = {0.0f, 90.0f, 180.0f, 270.0f};

// Crosshair appearance.
inline constexpr SDL_Color kCrosshairColor{.r = 255, .g = 255, .b = 255, .a = 125};

// Note colors per state.
inline constexpr SDL_Color kNoteColorDefault{.r = 255, .g = 255, .b = 255, .a = 255};
inline constexpr SDL_Color kNoteColorHit{.r = 100, .g = 255, .b = 100, .a = 255};
inline constexpr SDL_Color kNoteColorMiss{.r = 255, .g = 100, .b = 100, .a = 255};

} // namespace Game::Gameplay

#endif // CC_GAME_GAMEPLAY_CONSTANTS_H
