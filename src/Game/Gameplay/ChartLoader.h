#ifndef CC_GAME_CHART_LOADER_H
#define CC_GAME_CHART_LOADER_H

#include <expected>
#include <filesystem>

#include "ChartData.h"

namespace Game::Gameplay {

enum class ChartLoadError {
    FileNotFound,
    InvalidJson,
    UnsupportedVersion,
    NoNotes
};

std::expected<ChartData, ChartLoadError> LoadChartFromFile(const std::filesystem::path& path);

} // namespace Game::Gameplay

#endif // CC_GAME_CHART_LOADER_H
