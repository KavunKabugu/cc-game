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

    // When true, draw the song cover (coverFile / thumbnailPath) over the solid background colour.
    bool enableBackgroundImage = true;

    // Cover image opacity (0 = transparent, 1 = opaque). Solid colour always draws underneath.
    float backgroundOpacity = kDefaultBackgroundOpacity;

    // Solid playfield / backdrop colour (RGB). Alpha is unused, fill is always opaque.
    unsigned char backgroundColorR = kDefaultBackgroundColorR;
    unsigned char backgroundColorG = kDefaultBackgroundColorG;
    unsigned char backgroundColorB = kDefaultBackgroundColorB;

    // When true, draw darkened (black) borders around the playfield edges.
    bool enablePlayfieldBorder = true;

    // Border opacity (0 = transparent, 1 = opaque). Colour is always black.
    float playfieldBorderOpacity = kDefaultPlayfieldBorderOpacity;

    // Border thickness in UI units: 0 = none, 100 = 50% of half-screen from each edge (full cover).
    float playfieldBorderSize = kDefaultPlayfieldBorderSize;

    // Lane keys: two slots per lane (see LaneInputHandler).
    LaneKeyBindings keyBindings = DefaultLaneKeyBindings();
};

} // namespace Game::Gameplay

#endif // CC_GAME_GAMEPLAY_SETTINGS_H
