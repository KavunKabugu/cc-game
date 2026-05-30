#ifndef CC_GAME_SONG_TYPES_H
#define CC_GAME_SONG_TYPES_H

#include <filesystem>
#include <string>
#include <vector>

namespace Game::Song {
    enum class SongError {
        SongsRootMissing,
        MetadataMissing,
        InvalidMetadata,
        MissingAudioFile,
        MissingDifficultyChart,
        CacheReadFailed,
        CacheWriteFailed,
        LevelLoadFailed,
        NotImplemented
    };

    struct SongDifficulty {
        std::string name;
        std::filesystem::path chartPath;
        int level{0};
    };

    struct SongMetadata {
        std::string title;
        std::string artist;
        float bpm{0.0f};
        std::filesystem::path songFolder; // Directory name relative to songs root
        std::string audioFile;
        std::string coverFile;
        std::vector<SongDifficulty> difficulties;
    };
}

#endif // CC_GAME_SONG_TYPES_H
