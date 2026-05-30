#ifndef CC_GAME_AUDIO_MANAGER_H
#define CC_GAME_AUDIO_MANAGER_H

#include <vector>
#include <memory>
#include <array>
#include <SDL3_mixer/SDL_mixer.h>
#include "AudioBuffer.h"
#include "Sound.h"

namespace Game {

class AudioManager {
public:
    static AudioManager& getInstance() {
        static AudioManager instance;
        return instance;
    }

    bool Init();
    void Shutdown();

    // Updates the manager (only cleans up finished sounds for now)
    void Update();

    // Volume Control
    void SetGlobalVolume(float volume);
    [[nodiscard]] float GetGlobalVolume() const { return globalVolume; }

    void SetCategoryVolume(AudioCategory category, float volume);
    [[nodiscard]] float GetCategoryVolume(AudioCategory category) const;

    // Global State Control
    void PauseAll() const;
    void ResumeAll() const;
    void StopAll() const;

    // Category State Control
    void PauseCategory(AudioCategory category) const;
    void ResumeCategory(AudioCategory category) const;
    void StopCategory(AudioCategory category) const;

    // Creates and plays a sound from decoded mixer audio
    std::shared_ptr<Sound> Play(const std::shared_ptr<MIX_Audio>& audio, AudioCategory category = AudioCategory::Sfx, bool loop = false);

private:
    AudioManager();
    ~AudioManager() = default;

    MIX_Mixer* mixer = nullptr;
    float globalVolume = 1.0f;
    std::array<float, static_cast<size_t>(AudioCategory::Count)> categoryVolumes{};
    std::vector<std::shared_ptr<Sound>> activeSounds;

    void RefreshAllVolumes() const;
};

} // namespace Game

#endif //CC_GAME_AUDIO_MANAGER_H
