//
// Created by karp on 06/05/2026.
// Audio mix levels persisted to settings and applied via AudioManager.
//

#ifndef CC_GAME_AUDIO_SETTINGS_H
#define CC_GAME_AUDIO_SETTINGS_H

#include <algorithm>

namespace Game {

struct AudioSettings {
    float masterVolume = 1.0f;
    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;
    float uiVolume = 1.0f;
};

inline void ClampAudioSettings(AudioSettings& s) noexcept {
    constexpr float kMin = 0.0f;
    constexpr float kMax = 1.0f;
    s.masterVolume = std::clamp(s.masterVolume, kMin, kMax);
    s.musicVolume = std::clamp(s.musicVolume, kMin, kMax);
    s.sfxVolume = std::clamp(s.sfxVolume, kMin, kMax);
    s.uiVolume = std::clamp(s.uiVolume, kMin, kMax);
}

} // namespace Game

#endif // CC_GAME_AUDIO_SETTINGS_H
