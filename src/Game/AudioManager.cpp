#include "AudioManager.h"
#include <SDL3/SDL_log.h>
#include <algorithm>
#include <cassert>
#include "ResourceManager.h"

namespace {

[[nodiscard]] float ClampUnitVolume(const float volume) {
    return std::clamp(volume, 0.0f, 1.0f);
}

} // namespace

namespace Game {

AudioManager::AudioManager() {
    categoryVolumes.fill(1.0f);
}

bool AudioManager::Init() {
    mixer = ResourceManager::getInstance().GetMixer();
    if (!mixer) {
        SDL_Log("AudioManager: mixer is not initialized.");
        return false;
    }

    return true;
}

void AudioManager::Shutdown() {
    activeSounds.clear();
    mixer = nullptr;
}

void AudioManager::Update() {
    // Clean up stopped sounds that are not looping.
    std::erase_if(activeSounds,
                  [](const std::shared_ptr<Sound>& sound) {
                      return !sound->IsLooping() && !sound->IsPlaying() && !sound->IsPaused();
                  });
}

void AudioManager::SetGlobalVolume(const float volume) {
    globalVolume = ClampUnitVolume(volume);
    RefreshAllVolumes();
}

void AudioManager::SetCategoryVolume(AudioCategory category, const float volume) {
    assert(category < AudioCategory::Count);
    categoryVolumes[static_cast<size_t>(category)] = ClampUnitVolume(volume);
    RefreshAllVolumes();
}

float AudioManager::GetCategoryVolume(AudioCategory category) const {
    assert(category < AudioCategory::Count);
    return categoryVolumes[static_cast<size_t>(category)];
}

void AudioManager::RefreshAllVolumes() const {
    for (auto& sound : activeSounds) {
        sound->RefreshVolume(globalVolume, categoryVolumes[static_cast<size_t>(sound->GetCategory())]);
    }
}

void AudioManager::PauseAll() const {
    for (auto& s : activeSounds) s->Pause();
}

void AudioManager::ResumeAll() const {
    for (auto& s : activeSounds) s->Resume();
}

void AudioManager::StopAll() const {
    for (auto& s : activeSounds) s->Stop();
}

void AudioManager::PauseCategory(const AudioCategory category) const {
    for (auto& s : activeSounds) {
        if (s->GetCategory() == category) s->Pause();
    }
}

void AudioManager::ResumeCategory(const AudioCategory category) const {
    for (auto& s : activeSounds) {
        if (s->GetCategory() == category) s->Resume();
    }
}

void AudioManager::StopCategory(const AudioCategory category) const {
    for (auto& s : activeSounds) {
        if (s->GetCategory() == category) s->Stop();
    }
}

std::shared_ptr<Sound> AudioManager::Play(const std::shared_ptr<MIX_Audio>& audio, AudioCategory category, bool loop) {
    if (!audio || !mixer) {
        return nullptr;
    }

    MIX_Track* track = MIX_CreateTrack(mixer);
    if (!track) {
        SDL_Log("Failed to create mixer track: %s", SDL_GetError());
        return nullptr;
    }

    if (!MIX_SetTrackAudio(track, audio.get())) {
        SDL_Log("Failed to assign audio to track: %s", SDL_GetError());
        MIX_DestroyTrack(track);
        return nullptr;
    }

    auto sound = std::make_shared<Sound>(audio, track, category, loop);
    sound->Play();
    activeSounds.push_back(sound);
    return sound;
}

} // namespace Game
