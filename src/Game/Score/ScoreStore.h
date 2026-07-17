#ifndef CC_GAME_SCORE_STORE_H
#define CC_GAME_SCORE_STORE_H

#include <filesystem>
#include <vector>

#include "Game/Score/ScoreTypes.h"
#include "Game/Song/SongTypes.h"

namespace Game::Score {

class ScoreStore {
public:
    [[nodiscard]] static std::filesystem::path ResolveScoresRoot();

    // Placeholder until persistence is implemented, one stub entry per song.
    [[nodiscard]] static std::vector<ScoreSummary> ListForSong(const Song::SongMetadata& song);
};

} // namespace Game::Score

#endif // CC_GAME_SCORE_STORE_H
