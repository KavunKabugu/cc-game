#ifndef CC_GAME_SONG_CLOCK_H
#define CC_GAME_SONG_CLOCK_H

#include <cstdint>
#include <memory>

#include <SDL3_mixer/SDL_mixer.h>

namespace Game {
class Sound;
} // namespace Game

namespace Game::Gameplay {

// Minimal song clock, counts a silent start delay, then plays the audio and
// reports the song time as the audio playback position (in seconds).
// Shifted by an optional user-defined audio offset (positive = audio is heard later
// than reported by the device, so we treat the song time as that much earlier).
class SongClock {
public:
    SongClock(std::shared_ptr<MIX_Audio> audio,
              double startDelaySeconds,
              double audioOffsetSeconds = 0.0);
    ~SongClock();

    SongClock(const SongClock&) = delete;
    SongClock& operator=(const SongClock&) = delete;

    void Update(double deltaTimeSeconds);

    [[nodiscard]] bool MusicStarted() const { return musicStarted; }
    [[nodiscard]] bool MusicEnded() const;

    // Chart-relative time in seconds. Negative while in the pre-music delay.
    // Includes the user audio offset so callers see a single coherent timeline.
    [[nodiscard]] double SongTime() const;

    // Chart-relative seconds for an input event, (eventTimeNs - musicWallStartNs) / 1e9 - audioOffsetSeconds.
    // eventTimeNs must use the same clock as SDL key events (SDL_GetTicksNS).
    // Only meaningful after StartMusic(), before that, returns SongTime().
    [[nodiscard]] double WallTimeSongSecondsAt(std::uint64_t eventTimeNs) const;

    [[nodiscard]] double AudioOffsetSeconds() const { return audioOffsetSeconds; }

    void Stop();

private:
    void StartMusic();

    std::shared_ptr<MIX_Audio> audio;
    std::shared_ptr<Sound> sound;
    double delayRemaining;
    double audioOffsetSeconds;
    bool musicStarted = false;
    std::uint64_t musicWallStartNs = 0;
};

} // namespace Game::Gameplay

#endif // CC_GAME_SONG_CLOCK_H
