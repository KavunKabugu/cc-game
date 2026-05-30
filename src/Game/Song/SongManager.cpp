#include "SongManager.h"
#include "ThirdParty/json.hpp"
#include "Game/ResourceManager.h"
#include <SDL3/SDL.h>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <mutex>

namespace Game::Song {
    using json = nlohmann::json;

    namespace {
        constexpr int kCacheVersion = 1;

        struct CacheEntry {
            std::int64_t timestampNs{0};
            std::shared_ptr<SongMetadata> metadata;
        };

        std::int64_t ToTimestampNs(const std::filesystem::file_time_type timePoint) {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(timePoint.time_since_epoch()).count();
        }

        json SerializeSongMetadata(const SongMetadata& metadata) {
            json difficulties = json::array();
            for (const auto&[name, chartPath, level] : metadata.difficulties) {
                difficulties.push_back({
                    {"name", name},
                    {"chartPath", chartPath.string()},
                    {"level", level}
                });
            }

            return {
                {"title", metadata.title},
                {"artist", metadata.artist},
                {"bpm", metadata.bpm},
                {"songFolder", metadata.songFolder.string()},
                {"audioFile", metadata.audioFile},
                {"coverFile", metadata.coverFile},
                {"difficulties", difficulties}
            };
        }

        std::expected<std::shared_ptr<SongMetadata>, SongError> DeserializeSongMetadata(const json& j) {
            try {
                auto metadata = std::make_shared<SongMetadata>();
                metadata->title = j.at("title").get<std::string>();
                metadata->artist = j.at("artist").get<std::string>();
                metadata->bpm = j.at("bpm").get<float>();
                metadata->songFolder = j.at("songFolder").get<std::string>();
                metadata->audioFile = j.at("audioFile").get<std::string>();
                metadata->coverFile = j.at("coverFile").get<std::string>();

                for (const auto& d : j.at("difficulties")) {
                    metadata->difficulties.push_back(SongDifficulty{
                        .name = d.at("name").get<std::string>(),
                        .chartPath = d.at("chartPath").get<std::string>(),
                        .level = d.at("level").get<int>()
                    });
                }
                return metadata;
            } catch (...) {
                return std::unexpected(SongError::InvalidMetadata);
            }
        }

        std::expected<std::shared_ptr<SongMetadata>, SongError> ParseSongFolder(const std::filesystem::path& folderPath) {
            const std::filesystem::path metadataPath = folderPath / "song_metadata.json";
            if (!std::filesystem::exists(metadataPath)) {
                return std::unexpected(SongError::MetadataMissing);
            }

            std::ifstream in(metadataPath);
            if (!in.is_open()) return std::unexpected(SongError::InvalidMetadata);

            try {
                json j;
                in >> j;

                auto metadata = std::make_shared<SongMetadata>();
                metadata->title = j.at("title").get<std::string>();
                metadata->artist = j.at("artist").get<std::string>();
                metadata->bpm = j.at("bpm").get<float>();
                metadata->songFolder = folderPath.filename(); // Relative to songs/
                metadata->audioFile = j.at("audioFile").get<std::string>();
                
                if (j.contains("coverFile")) {
                    metadata->coverFile = j.at("coverFile").get<std::string>();
                } else if (j.contains("thumbnailPath")) { // Support example field name TODO: Don't do this anymore? Not sure.
                    metadata->coverFile = j.at("thumbnailPath").get<std::string>();
                }

                for (const auto& d : j.at("difficulties")) {
                    std::string chartFile = d.contains("chartFile") ? d.at("chartFile").get<std::string>() : d.at("chartPath").get<std::string>();
                    metadata->difficulties.push_back(SongDifficulty{
                        .name = d.at("name").get<std::string>(),
                        .chartPath = chartFile,
                        .level = d.value("level", 0)
                    });
                }

                if (metadata->difficulties.empty()) return std::unexpected(SongError::MissingDifficultyChart);

                return metadata;
            } catch (...) {
                return std::unexpected(SongError::InvalidMetadata);
            }
        }
    }

    std::expected<void, SongError> SongManager::RefreshLibrary() {
        const std::filesystem::path songsRoot = ResolveSongsRoot();
        if (!std::filesystem::exists(songsRoot)) {
            std::filesystem::create_directories(songsRoot);
        }

        // Add to ResourceManager search paths
        ResourceManager::getInstance().AddSearchPath(songsRoot.string());

        std::unordered_map<std::string, CacheEntry> cachedEntries;
        const auto cachePath = ResolveCachePath();
        if (std::filesystem::exists(cachePath)) {
            if (std::ifstream in(cachePath); in.is_open()) {
                try {
                    json root;
                    in >> root;
                    if (root.at("cacheVersion").get<int>() == kCacheVersion) {
                        for (const auto& entry : root.at("entries")) {
                            auto dir = entry.at("directory").get<std::string>();
                            if (auto metadataRes = DeserializeSongMetadata(entry.at("metadata"))) {
                                cachedEntries[dir] = {
                                    .timestampNs = entry.at("timestampNs").get<std::int64_t>(),
                                    .metadata = *metadataRes
                                };
                            }
                        }
                    }
                } catch (...) {}
            }
        }

        std::vector<std::shared_ptr<SongMetadata>> nextLibrary;
        std::unordered_map<std::string, CacheEntry> nextCache;

        for (const auto& entry : std::filesystem::directory_iterator(songsRoot)) {
            if (!entry.is_directory()) continue;

            const auto folderName = entry.path().filename().string();
            const std::int64_t timestampNs = ToTimestampNs(std::filesystem::last_write_time(entry.path()));

            if (cachedEntries.contains(folderName) && cachedEntries[folderName].timestampNs == timestampNs) {
                nextLibrary.push_back(cachedEntries[folderName].metadata);
                nextCache[folderName] = cachedEntries[folderName];
                continue;
            }

            if (auto parseResult = ParseSongFolder(entry.path())) {
                nextLibrary.push_back(*parseResult);
                nextCache[folderName] = { .timestampNs = timestampNs, .metadata = *parseResult };
            } else {
                SDL_Log("SongManager: Failed to parse %s", folderName.c_str());
            }
        }

        // Save Cache
        try {
            json root;
            root["cacheVersion"] = kCacheVersion;
            root["entries"] = json::array();
            for (const auto& [dir, entry] : nextCache) {
                root["entries"].push_back({
                    {"directory", dir},
                    {"timestampNs", entry.timestampNs},
                    {"metadata", SerializeSongMetadata(*entry.metadata)}
                });
            }
            std::ofstream out(cachePath);
            out << root.dump(2);
        } catch (...) {
            SDL_Log("SongManager: Failed to save cache");
        }

        std::unique_lock lock(m_mutex);
        m_library = std::move(nextLibrary);
        return {};
    }

    const std::vector<std::shared_ptr<SongMetadata>>& SongManager::GetLibrary() const {
        std::shared_lock lock(m_mutex);
        // See SongManager.h note: read-only reference use is accepted until
        // we need snapshot-based reads for concurrent refresh support.
        // Not sure if I'll ever do it, but it'd be a lot cooler if we did.
        return m_library;
    }

    std::filesystem::path SongManager::ResolveSongFile(
        const SongMetadata& metadata, const std::filesystem::path& relative) {
        return (ResolveSongsRoot() / metadata.songFolder / relative).lexically_normal();
    }

    std::filesystem::path SongManager::ResolveSongsRoot() {
        if (const char* base = SDL_GetBasePath()) {
            return (std::filesystem::path(base) / "songs").lexically_normal();
        }
        return "songs";
    }

    std::filesystem::path SongManager::ResolveCachePath() {
        if (const char* base = SDL_GetBasePath()) {
            return (std::filesystem::path(base) / "songs_cache.json").lexically_normal();
        }
        return "songs_cache.json";
    }
}
