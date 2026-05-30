#include "Sound.h"
#include "AudioManager.h"
#include <cmath>
#include <algorithm>
#include <utility>

namespace Game {

Sound::Sound(std::shared_ptr<MIX_Audio> audio, MIX_Track* track, const AudioCategory category, const bool loop)
    : audio(std::move(audio)), track(track), category(category), looping(loop) {
    // Initial volume calculation
    RefreshVolume(AudioManager::getInstance().GetGlobalVolume(),
                  AudioManager::getInstance().GetCategoryVolume(category));
    SetPitch(1.0f);
}

Sound::~Sound() {
    if (track) {
        MIX_DestroyTrack(track);
        track = nullptr;
    }
}

void Sound::Play() {
    if (!track) {
        return;
    }

    if (stopped.load()) {
        MIX_SetTrackPlaybackPosition(track, 0);
    }

    MIX_SetTrackLoops(track, looping.load() ? -1 : 0);
    if (!MIX_PlayTrack(track, 0)) {
        return;
    }

    paused.store(false);
    stopped.store(false);
}

void Sound::Pause() {
    if (!track) {
        return;
    }

    if (MIX_PauseTrack(track)) {
        paused.store(true);
    }
}

void Sound::Resume() {
    if (!track) {
        return;
    }

    if (MIX_ResumeTrack(track)) {
        paused.store(false);
        stopped.store(false);
    }
}

void Sound::Stop() {
    if (!track) {
        return;
    }

    if (MIX_StopTrack(track, 0)) {
        paused.store(false);
        stopped.store(true);
    }
}

void Sound::SetVolume(const float volume) {
    localVolume.store(volume);
    RefreshVolume(AudioManager::getInstance().GetGlobalVolume(), 
                  AudioManager::getInstance().GetCategoryVolume(category));
}

float Sound::GetVolume() const {
    return localVolume.load();
}

void Sound::RefreshVolume(const float globalVolume, const float categoryVolume) const {
    const float finalGain = localVolume.load() * globalVolume * categoryVolume;
    if (track) {
        MIX_SetTrackGain(track, finalGain);
    }
}

void Sound::SetPitch(const float pitch) const {
    if (track) {
        MIX_SetTrackFrequencyRatio(track, pitch);
    }
}

float Sound::GetPitch() const {
    if (!track) {
        return 1.0f;
    }
    const float ratio = MIX_GetTrackFrequencyRatio(track);
    return (ratio <= 0.0f) ? 1.0f : ratio;
}

bool Sound::IsPlaying() const {
    if (!track) {
        return false;
    }
    return MIX_TrackPlaying(track);
}

double Sound::GetRawPosition() const {
    if (!track) {
        return 0.0;
    }

    const Sint64 frames = MIX_GetTrackPlaybackPosition(track);
    if (frames < 0) {
        return 0.0;
    }

    const Sint64 ms = MIX_TrackFramesToMS(track, frames);
    if (ms < 0) {
        return 0.0;
    }

    const double pos = static_cast<double>(ms) / 1000.0;
    const double duration = GetDuration();
    if (duration > 0.0 && looping.load()) {
        return std::fmod(pos, duration);
    }
    return (duration > 0.0) ? std::min(pos, duration) : pos;
}

double Sound::GetPitchAwarePosition() const {
    return GetRawPosition();
}

double Sound::GetDuration() const {
    if (!audio || !track) {
        return 0.0;
    }

    const Sint64 frames = MIX_GetAudioDuration(audio.get());
    if (frames < 0) {
        return 0.0;
    }

    const Sint64 ms = MIX_TrackFramesToMS(track, frames);
    if (ms < 0) {
        return 0.0;
    }
    return static_cast<double>(ms) / 1000.0;
}

} // namespace Game
