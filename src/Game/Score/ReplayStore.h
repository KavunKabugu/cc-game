#ifndef CC_GAME_REPLAY_STORE_H
#define CC_GAME_REPLAY_STORE_H

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Game/Score/ReplayTypes.h"
#include "Game/Song/SongTypes.h"

namespace Game::Score {

class ReplayStore {
public:
    [[nodiscard]] static std::filesystem::path ResolveReplaysRoot();

    static std::optional<std::string> Save(
        const Song::SongMetadata& song,
        int difficultyIndex,
        const ReplaySettingsSnapshot& settings,
        const std::vector<ReplayPress>& presses);

    [[nodiscard]] static std::optional<ReplayRecord> Load(
        const std::filesystem::path& songFolder,
        const std::string& replayId);

    [[nodiscard]] static bool Exists(
        const std::filesystem::path& songFolder,
        std::string_view replayId);
};

} // namespace Game::Score

#endif // CC_GAME_REPLAY_STORE_H
