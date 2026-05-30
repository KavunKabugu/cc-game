#ifndef CC_GAME_SOUND_H
#define CC_GAME_SOUND_H

#include <SDL3_mixer/SDL_mixer.h>
#include <memory>
#include <atomic>
#include "AudioBuffer.h"

namespace Game {

class Sound : public std::enable_shared_from_this<Sound> {
public:
    Sound(std::shared_ptr<MIX_Audio> audio, MIX_Track* track, AudioCategory category, bool loop = false);
    ~Sound();

    void Play();
    void Pause();
    void Resume();
    void Stop();

    void SetVolume(float volume); // Sets local volume (0.0 to 1.0)
    [[nodiscard]] float GetVolume() const; // Returns local volume
    [[nodiscard]] AudioCategory GetCategory() const { return category; }

    // Calculates final gain, localVolume * globalVolume * categoryVolume
    void RefreshVolume(float globalVolume, float categoryVolume) const;

    void SetPitch(float pitch) const;
    [[nodiscard]] float GetPitch() const;

    void SetLooping(const bool loop) { looping.store(loop); }
    [[nodiscard]] bool IsLooping() const { return looping.load(); }

    [[nodiscard]] bool IsPlaying() const;
    [[nodiscard]] bool IsPaused() const { return paused.load(); }

    // Position in source audio (0.0 to Duration)
    [[nodiscard]] double GetRawPosition() const;
    // Real-world time elapsed since start. Currently same as raw position.
    [[nodiscard]] double GetPitchAwarePosition() const;
    
    [[nodiscard]] double GetDuration() const;

private:
    std::shared_ptr<MIX_Audio> audio;
    MIX_Track* track;
    AudioCategory category;
    
    std::atomic<float> localVolume{1.0f};
    std::atomic<bool> looping{false};
    std::atomic<bool> paused{false};
    std::atomic<bool> stopped{false};
};

} // namespace Game

#endif //CC_GAME_SOUND_H
