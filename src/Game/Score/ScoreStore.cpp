#include "ScoreStore.h"

#include "Game/PathUtf8.h"

namespace Game::Score {

std::filesystem::path ScoreStore::ResolveScoresRoot() {
    if (const auto base = PathFromSdlBasePath()) {
        return (*base / "scores").lexically_normal();
    }
    return "scores";
}

std::vector<ScoreSummary> ScoreStore::ListForSong(const Song::SongMetadata& song) {
    (void)ResolveScoresRoot();

    ScoreSummary placeholder;
    placeholder.score = 0;
    placeholder.accuracyPercent = 0.0;
    placeholder.difficultyName = song.difficulties.empty() ? "-" : song.difficulties.front().name;
    placeholder.statsText = "Not implemented";
    placeholder.timestampUnixSec = 0;
    return {std::move(placeholder)};
}

} // namespace Game::Score
