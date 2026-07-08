#include "ChartLoader.h"

#include <SDL3/SDL_log.h>
#include <algorithm>
#include <fstream>

#include "Game/PathUtf8.h"
#include "ThirdParty/json.hpp"

namespace Game::Gameplay {
using json = nlohmann::json;
using Game::PathToUtf8String;

namespace {
constexpr int kSupportedFormatVersion = 1;
} // namespace

std::expected<ChartData, ChartLoadError> LoadChartFromFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        SDL_Log("ChartLoader: file not found: %s", PathToUtf8String(path).c_str());
        return std::unexpected(ChartLoadError::FileNotFound);
    }

    std::ifstream in(path);
    if (!in.is_open()) {
        SDL_Log("ChartLoader: failed to open: %s", PathToUtf8String(path).c_str());
        return std::unexpected(ChartLoadError::FileNotFound);
    }

    json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        SDL_Log("ChartLoader: invalid JSON in %s: %s", PathToUtf8String(path).c_str(), e.what());
        return std::unexpected(ChartLoadError::InvalidJson);
    }

    ChartData chart;
    try {
        chart.formatVersion = j.value("formatVersion", 0);
        if (chart.formatVersion != kSupportedFormatVersion) {
            SDL_Log("ChartLoader: unsupported formatVersion %d in %s",
                    chart.formatVersion, PathToUtf8String(path).c_str());
            return std::unexpected(ChartLoadError::UnsupportedVersion);
        }

        const double offsetMs = j.contains("song") ? j["song"].value("offsetMs", 0.0) : 0.0;
        chart.offsetSeconds = offsetMs * 1e-3;

        if (!j.contains("notes") || !j["notes"].is_array()) {
            return std::unexpected(ChartLoadError::NoNotes);
        }

        chart.notes.reserve(j["notes"].size());
        for (const auto& note : j["notes"]) {
            const std::string type = note.value("type", "tap");
            if (type != "tap" && type != "hold") {
                continue; // Skip unknown / unsupported types.
            }

            const int lane = note.value("lane", -1);
            if (lane < 0 || lane > 3) {
                continue;
            }

            const double timeMs = note.value("timeMs", -1.0);
            if (timeMs < 0.0) {
                continue;
            }

            chart.notes.push_back(ChartNote{
                .hitTime = timeMs * 1e-3 + chart.offsetSeconds,
                .lane = lane,
            });
        }
    } catch (const std::exception& e) {
        SDL_Log("ChartLoader: malformed chart %s: %s", PathToUtf8String(path).c_str(), e.what());
        return std::unexpected(ChartLoadError::InvalidJson);
    }

    if (chart.notes.empty()) {
        return std::unexpected(ChartLoadError::NoNotes);
    }

    std::ranges::sort(chart.notes, {}, &ChartNote::hitTime);
    return chart;
}

} // namespace Game::Gameplay
