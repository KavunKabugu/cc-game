#ifndef CC_GAME_SETTINGS_STORAGE_H
#define CC_GAME_SETTINGS_STORAGE_H

#include "AudioSettings.h"
#include "Gameplay/GameplaySettings.h"
#include "VideoSettings.h"

namespace Game::SettingsStorage {

// Loads video + gameplay + audio + windowed restore sizes from settings.json next to the executable.
// Falls back to defaults, attempts one-time migration from legacy gameplay_settings.json in SDL pref path.
void LoadAllInto(
    VideoSettings& video,
    Gameplay::GameplaySettings& gameplay,
    AudioSettings& audio,
    int& savedWindowedWidth,
    int& savedWindowedHeight
);

// Writes all persisted fields to settings.json beside the executable.
[[nodiscard]] bool SaveAll(
    const VideoSettings& video,
    const Gameplay::GameplaySettings& gameplay,
    const AudioSettings& audio,
    int savedWindowedWidth,
    int savedWindowedHeight
);

} // namespace Game::SettingsStorage

#endif // CC_GAME_SETTINGS_STORAGE_H
