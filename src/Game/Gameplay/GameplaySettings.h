#ifndef CC_GAME_GAMEPLAY_SETTINGS_H
#define CC_GAME_GAMEPLAY_SETTINGS_H

#include "GameplayConstants.h"
#include "LaneInputHandler.h"

namespace Game::Gameplay {

// Bundle of values that the user options screen will be allowed to override.
// Audio volume is intentionally absent, AudioManager already owns it through
// SetGlobalVolume / SetCategoryVolume and the gameplay layer never touches it.
struct GameplaySettings {
    // Visible crosshair radius in logical pixels at the reference resolution
    // (GameplayConstants kLogicalWidth × kLogicalHeight). GameplayScene scales into the active
    // logical framebuffer so size and scroll timing stay consistent across resolutions.
    float crosshairRadius = kDefaultCrosshairRadius;

    // Player note-speed setting (z-units/ms via kSpeedDivisor), unchanged across resolutions when used
    // with scaled crosshair radius and actual logical width in NoteSimulation::Configure.
    float noteSpeed = kDefaultNoteSpeed;

    // Compensation for hardware audio delay, in seconds (typically small fractions of a second,
    // settings UI exposes milliseconds mapped into this field).
    double audioOffsetSeconds = 0.0;

    // When true, hit judgement timestamps use monotonic time since music start (wall clock path).
    // When false, press times are derived from SDL event timestamps relative to current song time.
    // Note simulation Tick() still uses audio song time in both modes.
    bool useWallClockForJudgementTiming = true;

    // Lane keys: two slots per lane (see LaneInputHandler).
    LaneKeyBindings keyBindings = DefaultLaneKeyBindings();
};

} // namespace Game::Gameplay

#endif // CC_GAME_GAMEPLAY_SETTINGS_H
