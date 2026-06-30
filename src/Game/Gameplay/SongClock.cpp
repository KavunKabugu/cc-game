#include "SongClock.h"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>

#include "Game/AudioManager.h"
#include "Game/Sound.h"

namespace Game::Gameplay {

SongClock::SongClock(std::shared_ptr<MIX_Audio> audio,
                     const double startDelaySeconds,
                     const double audioOffsetSeconds)
    : audio(std::move(audio)),
      delayRemaining(startDelaySeconds),
      startDelaySeconds(startDelaySeconds),
      audioOffsetSeconds(audioOffsetSeconds),
      sceneWallStartNs(SDL_GetTicksNS()) {}

SongClock::~SongClock() {
    Stop();
}

void SongClock::Update(const double deltaTimeSeconds) {
    if (musicStarted) return;

    delayRemaining -= deltaTimeSeconds;
    if (delayRemaining <= 0.0) {
        StartMusic();
    }
}

void SongClock::StartMusic() {
    musicStarted = true;
    musicWallStartNs = SDL_GetTicksNS();
    if (!audio) return;

    sound = AudioManager::getInstance().Play(audio, AudioCategory::Music, false);
    if (!sound) {
        SDL_Log("SongClock: failed to start music playback");
    }
}

double SongClock::SongTime() const {
    // Subtract the user audio offset uniformly so the time curve is continuous
    // across the music-start boundary. With offset = 0 this is the raw clock.
    if (!musicStarted) {
        return -delayRemaining - audioOffsetSeconds;
    }
    if (!sound) {
        return -audioOffsetSeconds;
    }
    return sound->GetRawPosition() - audioOffsetSeconds;
}

double SongClock::WallTimeSongSecondsAt(const std::uint64_t eventTimeNs) const {
    if (musicStarted && musicWallStartNs != 0) {
        double elapsedSec = 0.0;
        if (eventTimeNs >= musicWallStartNs) {
            elapsedSec = static_cast<double>(eventTimeNs - musicWallStartNs) * 1e-9;
        }
        return elapsedSec - audioOffsetSeconds;
    }

    double elapsedSec = 0.0;
    if (eventTimeNs >= sceneWallStartNs) {
        elapsedSec = static_cast<double>(eventTimeNs - sceneWallStartNs) * 1e-9;
    }
    return elapsedSec - startDelaySeconds - audioOffsetSeconds;
}

bool SongClock::MusicEnded() const {
    if (!musicStarted || !sound) return false;
    if (sound->IsPaused()) return false;
    if (sound->IsPlaying()) return false;
    return true;
}

void SongClock::Stop() {
    if (sound) {
        sound->Stop();
        sound.reset();
    }
}

} // namespace Game::Gameplay
