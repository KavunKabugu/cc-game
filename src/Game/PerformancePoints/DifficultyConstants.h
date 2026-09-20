//
// Created by ludeo on 9/18/26.
//

#ifndef CC_GAME_DIFFICULTYCONSTANTS_H
#define CC_GAME_DIFFICULTYCONSTANTS_H

namespace Game::PerformancePoints {

//raw speed
inline constexpr float minDeltaMs = 25.0;
inline constexpr float speedExponent = 0.9;

//chords
inline constexpr float chord_window_ms = 10.0;
inline constexpr float chordWeight = 0.45;

//jacks
inline constexpr float jackWeight = 0.9;
inline constexpr float jackFailoffMs = 200.0;
inline constexpr float chordJackWeight = 0.5;

//anchors
inline constexpr float anchorWindowMs = 1500.0;
inline constexpr int anchorMinNotes = 8;
inline constexpr float anchorThreshold = 0.3;
inline constexpr float anchorWeight = 0.55;

//rhythm irregularity
inline constexpr float rhythmWeight = 0.22;
inline constexpr float rhythmCap = 2.0;

//strain accumulation
inline constexpr float decayPerSecond = 0.28;
inline constexpr float sectionMs = 800.0;

//aggregation
inline constexpr float peakFraction = 0.25;
inline constexpr float sustainThreshold = 0.75;
inline constexpr float lengthWeight = 0.45;
inline constexpr float lengthReferenceS = 60.0;
inline constexpr float compressionExponent = 0.5;
inline constexpr float scale = 0.9715;

}

#endif //CC_GAME_DIFFICULTYCONSTANTS_H
