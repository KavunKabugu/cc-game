#ifndef CC_GAME_SCORE_STORE_H
#define CC_GAME_SCORE_STORE_H

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Game/Score/ScoreTypes.h"
#include "Game/Song/SongTypes.h"

namespace Game::Score {

class ScoreStore {
public:
    [[nodiscard]] static std::filesystem::path ResolveScoresRoot();
    [[nodiscard]] static std::filesystem::path ResolveReplaysRoot();

    // Persist a completed run. Logs on I/O errors, still returns runId when written.
    static std::optional<std::string> Save(
        const Song::SongMetadata& song,
        int difficultyIndex,
        const ResultsViewData& detail,
        std::string_view playerName,
        std::string_view replayId = "");

    [[nodiscard]] static std::vector<ScoreSummary> ListForSong(const Song::SongMetadata& song);

    [[nodiscard]] static std::optional<ScoreRecord> Load(
        const std::filesystem::path& songFolder,
        const std::string& runId);
};

} // namespace Game::Score

#endif // CC_GAME_SCORE_STORE_H
