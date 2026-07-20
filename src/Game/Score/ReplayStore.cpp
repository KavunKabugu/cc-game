#include "ReplayStore.h"

#include <chrono>
#include <format>
#include <fstream>
#include <system_error>

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>

#include "Game/PathUtf8.h"
#include "ThirdParty/json.hpp"

namespace Game::Score {
namespace {
using json = nlohmann::json;
using Game::PathFromSdlBasePath;
using Game::PathToUtf8String;

[[nodiscard]] std::filesystem::path SongReplaysDir(const std::filesystem::path& songFolder) {
    return (ReplayStore::ResolveReplaysRoot() / songFolder).lexically_normal();
}

[[nodiscard]] std::filesystem::path ReplayPath(
    const std::filesystem::path& songFolder,
    const std::string& replayId) {
    return SongReplaysDir(songFolder) / (replayId + ".json");
}

[[nodiscard]] std::string MakeReplayId() {
    const auto now = std::chrono::system_clock::now();
    const auto unixSec =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    return std::format("{}-{:016x}", unixSec, SDL_GetTicksNS());
}

[[nodiscard]] json SettingsToJson(const ReplaySettingsSnapshot& s) {
    return {
        {"noteSpeed", s.noteSpeed},
        {"crosshairRadius", s.crosshairRadius},
        {"logicalWidth", s.logicalWidth},
        {"logicalHeight", s.logicalHeight},
        {"audioOffsetSeconds", s.audioOffsetSeconds},
        {"swapUpDownLanes", s.swapUpDownLanes},
        {"useWallClockForJudgementTiming", s.useWallClockForJudgementTiming},
    };
}

[[nodiscard]] bool SettingsFromJson(const json& j, ReplaySettingsSnapshot& s) {
    if (!j.is_object()) {
        return false;
    }
    s.noteSpeed = j.value("noteSpeed", 0.0f);
    s.crosshairRadius = j.value("crosshairRadius", 0.0f);
    s.logicalWidth = j.value("logicalWidth", 0);
    s.logicalHeight = j.value("logicalHeight", 0);
    s.audioOffsetSeconds = j.value("audioOffsetSeconds", 0.0);
    s.swapUpDownLanes = j.value("swapUpDownLanes", false);
    s.useWallClockForJudgementTiming = j.value("useWallClockForJudgementTiming", true);
    return true;
}

[[nodiscard]] json PressesToJson(const std::vector<ReplayPress>& presses) {
    json arr = json::array();
    for (const auto& p : presses) {
        arr.push_back({{"t", p.t}, {"lane", p.lane}});
    }
    return arr;
}

[[nodiscard]] std::vector<ReplayPress> PressesFromJson(const json& j) {
    std::vector<ReplayPress> presses;
    if (!j.is_array()) {
        return presses;
    }
    presses.reserve(j.size());
    for (const auto& item : j) {
        if (!item.is_object()) {
            continue;
        }
        ReplayPress p;
        p.t = item.value("t", 0.0);
        p.lane = item.value("lane", -1);
        presses.push_back(p);
    }
    return presses;
}

} // namespace

std::filesystem::path ReplayStore::ResolveReplaysRoot() {
    if (const auto base = PathFromSdlBasePath()) {
        return (*base / "replays").lexically_normal();
    }
    return "replays";
}

std::optional<std::string> ReplayStore::Save(
    const Song::SongMetadata& song,
    const int difficultyIndex,
    const ReplaySettingsSnapshot& settings,
    const std::vector<ReplayPress>& presses) {
    if (difficultyIndex < 0 || difficultyIndex >= static_cast<int>(song.difficulties.size())) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ReplayStore::Save: invalid difficulty index");
        return std::nullopt;
    }

    const auto& diff = song.difficulties[difficultyIndex];
    ReplayRecord record;
    record.replayId = MakeReplayId();
    record.songFolder = PathToUtf8String(song.songFolder);
    record.chartPath = PathToUtf8String(diff.chartPath);
    record.difficultyIndex = difficultyIndex;
    record.difficultyName = diff.name;
    record.recordedAtUnixSec = std::chrono::duration_cast<std::chrono::seconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count();
    record.settings = settings;
    record.presses = presses;

    const auto dir = SongReplaysDir(song.songFolder);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION,
            "ReplayStore::Save: failed to create '%s' (%s)",
            PathToUtf8String(dir).c_str(),
            ec.message().c_str());
        return std::nullopt;
    }

    const json j = {
        {"schemaVersion", kReplaySchemaVersion},
        {"replayId", record.replayId},
        {"songFolder", record.songFolder},
        {"chartPath", record.chartPath},
        {"difficultyIndex", record.difficultyIndex},
        {"difficultyName", record.difficultyName},
        {"recordedAtUnixSec", record.recordedAtUnixSec},
        {"settings", SettingsToJson(record.settings)},
        {"presses", PressesToJson(record.presses)},
    };

    const auto path = ReplayPath(song.songFolder, record.replayId);
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION,
            "ReplayStore::Save: failed to write '%s'",
            PathToUtf8String(path).c_str());
        return std::nullopt;
    }
    out << j.dump(2);
    if (out.fail()) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION,
            "ReplayStore::Save: write failed for '%s'",
            PathToUtf8String(path).c_str());
        return std::nullopt;
    }
    return record.replayId;
}

std::optional<ReplayRecord> ReplayStore::Load(
    const std::filesystem::path& songFolder,
    const std::string& replayId) {
    if (replayId.empty()) {
        return std::nullopt;
    }
    const auto path = ReplayPath(songFolder, replayId);
    std::ifstream in(path);
    if (!in) {
        return std::nullopt;
    }
    try {
        json j;
        in >> j;
        if (!j.is_object() || j.value("schemaVersion", 0) != kReplaySchemaVersion) {
            return std::nullopt;
        }
        ReplayRecord record;
        record.replayId = j.value("replayId", replayId);
        record.songFolder = j.value("songFolder", PathToUtf8String(songFolder));
        record.chartPath = j.value("chartPath", "");
        record.difficultyIndex = j.value("difficultyIndex", -1);
        record.difficultyName = j.value("difficultyName", "");
        record.recordedAtUnixSec = j.value("recordedAtUnixSec", static_cast<std::int64_t>(0));
        if (j.contains("settings") && !SettingsFromJson(j["settings"], record.settings)) {
            return std::nullopt;
        }
        if (j.contains("presses")) {
            record.presses = PressesFromJson(j["presses"]);
        }
        return record;
    } catch (...) {
        return std::nullopt;
    }
}

bool ReplayStore::Exists(const std::filesystem::path& songFolder, const std::string_view replayId) {
    if (replayId.empty()) {
        return false;
    }
    std::error_code ec;
    return std::filesystem::is_regular_file(ReplayPath(songFolder, std::string(replayId)), ec);
}

} // namespace Game::Score
