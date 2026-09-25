#ifndef CC_GAME_SONG_CLOCK_H
#define CC_GAME_SONG_CLOCK_H

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

    // Chart-relative seconds for an input event mapped from SDL_GetTicksNS.
    // Pre-music: (eventTimeNs - songTimelineWallStartNs) / 1e9 - startDelaySeconds - audioOffsetSeconds.
    // After music: (eventTimeNs - musicWallStartNs) / 1e9 - audioOffsetSeconds.
    // songTimelineWallStartNs is the wall time when the delay countdown begins (first Update),
    // not construction, so a fade-in that builds the scene early does not shift judgements.
    // Pause/resume shifts these origins forward so a pause during the delay is not
    // subtracted again from music hit times.
    [[nodiscard]] double WallTimeSongSecondsAt(std::uint64_t eventTimeNs) const;

    [[nodiscard]] double AudioOffsetSeconds() const { return audioOffsetSeconds; }

    void Pause();
    void Resume();
    [[nodiscard]] bool IsPaused() const { return paused; }

    void Stop();

private:
    void StartMusic();
    void BeginSongTimelineIfNeeded(double deltaTimeSeconds);
    void ShiftTimelineOriginsByPausedWall(std::uint64_t pausedNs);

    std::shared_ptr<MIX_Audio> audio;
    std::shared_ptr<Sound> sound;
    double delayRemaining;
    double startDelaySeconds;
    double audioOffsetSeconds;
    double frozenSongTime = 0.0;
    bool musicStarted = false;
    bool paused = false;
    std::uint64_t songTimelineWallStartNs = 0;
    std::uint64_t musicWallStartNs = 0;
    std::uint64_t pauseWallStartNs = 0;
};

} // namespace Game::Gameplay

#endif // CC_GAME_SONG_CLOCK_H
