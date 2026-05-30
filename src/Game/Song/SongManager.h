#ifndef CC_GAME_SONG_MANAGER_H
#define CC_GAME_SONG_MANAGER_H

#include <expected>
#include <filesystem>
#include <memory>
#include <shared_mutex>
#include <vector>

#include "SongTypes.h"

namespace Game::Song {
    class SongManager {
    public:
        static SongManager& GetInstance() {
            static SongManager instance;
            return instance;
        }

        std::expected<void, SongError> RefreshLibrary();
        // WARN: Callers currently get a direct const-reference view.
        // This is safe for now because we only refresh on startup and do not run
        // any background library updates while scenes are reading the library.
        // TODO: expose a snapshot/thread-safe read API and add asynchronous refresh or hot-reload behavior.
        [[nodiscard]] const std::vector<std::shared_ptr<SongMetadata>>& GetLibrary() const;

        // Resolve a relative path declared inside a SongMetadata (audioFile, chartPath, ...)
        // to an absolute path under the songs root.
        [[nodiscard]] static std::filesystem::path ResolveSongFile(
            const SongMetadata& metadata, const std::filesystem::path& relative);

    private:
        SongManager() = default;
        ~SongManager() = default;
        SongManager(const SongManager&) = delete;
        SongManager& operator=(const SongManager&) = delete;

        static std::filesystem::path ResolveSongsRoot();

        static std::filesystem::path ResolveCachePath();

        mutable std::shared_mutex m_mutex;
        std::vector<std::shared_ptr<SongMetadata>> m_library;
    };
}

#endif // CC_GAME_SONG_MANAGER_H
