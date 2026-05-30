#ifndef CC_GAME_AUDIO_BUFFER_H
#define CC_GAME_AUDIO_BUFFER_H

#include <SDL3/SDL_audio.h>
#include <vector>
#include <cstddef>

namespace Game {

enum class AudioCategory {
    Sfx,
    Music,
    Ui,
    Generic,
    Count // Count of enum, should not be used as a category
};

struct AudioBuffer {
    SDL_AudioSpec spec;
    std::vector<std::byte> data;

    ~AudioBuffer() = default;
};

} // namespace Game

#endif //CC_GAME_AUDIO_BUFFER_H
