//
// Created by ludeo on 9/18/26.
//

#ifndef CC_GAME_PERFORMANCEPOINTSCONSTANTS_H
#define CC_GAME_PERFORMANCEPOINTSCONSTANTS_H

namespace Game::PerformancePoints {

inline constexpr float baseValueAtDifficultyTen = 400.0;
inline constexpr float difficultyExponent = 2.6;

inline constexpr float accuracyFloor = 0.6;
inline constexpr float accuracyExponent = 3.0;

inline constexpr float missPenalty = 0.97;
inline constexpr float badPenalty = 0.99;

inline constexpr float ratioBaseline = 0.5;
inline constexpr float ratioWeight = 0.3;

inline constexpr float fcBonus = 1.05;

}

#endif //CC_GAME_PERFORMANCEPOINTSCONSTANTS_H
