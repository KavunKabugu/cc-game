#include "ScoreStore.h"

#include "ReplayStore.h"

#include <algorithm>
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
using Game::Utf8StringToPath;

[[nodiscard]] std::filesystem::path SongScoresDir(const std::filesystem::path& songFolder) {
    return (ScoreStore::ResolveScoresRoot() / songFolder).lexically_normal();
}

[[nodiscard]] std::filesystem::path IndexPath(const std::filesystem::path& songFolder) {
    return SongScoresDir(songFolder) / "index.json";
}

[[nodiscard]] std::filesystem::path RunPath(const std::filesystem::path& songFolder, const std::string& runId) {
    return SongScoresDir(songFolder) / (runId + ".json");
}

[[nodiscard]] std::string MakeRunId() {
    const auto now = std::chrono::system_clock::now();
    const auto unixSec =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    return std::format("{}-{:016x}", unixSec, SDL_GetTicksNS());
}

[[nodiscard]] json JudgementCountsToJson(
    const std::array<int, static_cast<int>(Gameplay::Judgement::Count)>& counts) {
    return json::array({
        counts[static_cast<size_t>(Gameplay::Judgement::Perfect)],
        counts[static_cast<size_t>(Gameplay::Judgement::Great)],
        counts[static_cast<size_t>(Gameplay::Judgement::Good)],
        counts[static_cast<size_t>(Gameplay::Judgement::Bad)],
        counts[static_cast<size_t>(Gameplay::Judgement::Miss)],
    });
}

void JudgementCountsFromJson(
    const json& j,
    std::array<int, static_cast<int>(Gameplay::Judgement::Count)>& counts) {
    counts = {};
    if (!j.is_array()) {
        return;
    }
    for (size_t i = 0; i < j.size() && i < counts.size(); ++i) {
        if (j[i].is_number_integer()) {
            counts[i] = j[i].get<int>();
        }
    }
}

[[nodiscard]] json GraphEventsToJson(const std::vector<Gameplay::ResultsGraphEvent>& events) {
    json arr = json::array();
    for (const auto& e : events) {
        json item = {
            {"judgement", static_cast<int>(e.judgement)},
            {"missReason", static_cast<int>(e.missReason)},
            {"deltaMs", e.deltaMs},
            {"pressTimeSeconds", e.pressTimeSeconds},
        };
        if (e.noteTargetTimeSeconds) {
            item["noteTargetTimeSeconds"] = *e.noteTargetTimeSeconds;
        }
        arr.push_back(std::move(item));
    }
    return arr;
}

[[nodiscard]] std::vector<Gameplay::ResultsGraphEvent> GraphEventsFromJson(const json& j) {
    std::vector<Gameplay::ResultsGraphEvent> events;
    if (!j.is_array()) {
        return events;
    }
    events.reserve(j.size());
    for (const auto& item : j) {
        if (!item.is_object()) {
            continue;
        }
        Gameplay::ResultsGraphEvent e;
        if (item.contains("judgement") && item["judgement"].is_number_integer()) {
            e.judgement = static_cast<Gameplay::Judgement>(item["judgement"].get<int>());
        }
        if (item.contains("missReason") && item["missReason"].is_number_integer()) {
            e.missReason = static_cast<Gameplay::MissReason>(item["missReason"].get<int>());
        }
        if (item.contains("deltaMs") && item["deltaMs"].is_number()) {
            e.deltaMs = item["deltaMs"].get<double>();
        }
        if (item.contains("pressTimeSeconds") && item["pressTimeSeconds"].is_number()) {
            e.pressTimeSeconds = item["pressTimeSeconds"].get<double>();
        }
        if (item.contains("noteTargetTimeSeconds") && item["noteTargetTimeSeconds"].is_number()) {
            e.noteTargetTimeSeconds = item["noteTargetTimeSeconds"].get<double>();
        }
        events.push_back(e);
    }
    return events;
}

[[nodiscard]] json AccuracyStepsToJson(const std::vector<std::pair<double, double>>& steps) {
    json arr = json::array();
    for (const auto& [x, y] : steps) {
        arr.push_back(json::array({x, y}));
    }
    return arr;
}

[[nodiscard]] std::vector<std::pair<double, double>> AccuracyStepsFromJson(const json& j) {
    std::vector<std::pair<double, double>> steps;
    if (!j.is_array()) {
        return steps;
    }
    steps.reserve(j.size());
    for (const auto& item : j) {
        if (!item.is_array() || item.size() < 2) {
            continue;
        }
        if (!item[0].is_number() || !item[1].is_number()) {
            continue;
        }
        steps.emplace_back(item[0].get<double>(), item[1].get<double>());
    }
    return steps;
}

[[nodiscard]] json SummaryToJson(const ScoreSummary& s) {
    return {
        {"runId", s.runId},
        {"songFolder", s.songFolder},
        {"songTitle", s.songTitle},
        {"songArtist", s.songArtist},
        {"chartPath", s.chartPath},
        {"difficultyName", s.difficultyName},
        {"playerName", s.playerName},
        {"timestampUnixSec", s.timestampUnixSec},
        {"score", s.score},
        {"accuracyPercent", s.accuracyPercent},
        {"judgementCounts", JudgementCountsToJson(s.judgementCounts)},
        {"biasMs", s.biasMs},
        {"stdDevMs", s.stdDevMs},
        {"perfectGreatRatioText", s.perfectGreatRatioText},
        {"replayId", s.replayId},
    };
}

[[nodiscard]] bool SummaryFromJson(const json& j, ScoreSummary& s) {
    if (!j.is_object() || !j.contains("runId") || !j["runId"].is_string()) {
        return false;
    }
    s.runId = j["runId"].get<std::string>();
    s.songFolder = j.value("songFolder", "");
    s.songTitle = j.value("songTitle", "");
    s.songArtist = j.value("songArtist", "");
    s.chartPath = j.value("chartPath", "");
    s.difficultyName = j.value("difficultyName", "");
    s.playerName = j.value("playerName", "Player");
    s.timestampUnixSec = j.value("timestampUnixSec", static_cast<std::int64_t>(0));
    s.score = j.value("score", static_cast<std::int64_t>(0));
    s.accuracyPercent = j.value("accuracyPercent", 0.0);
    if (j.contains("judgementCounts")) {
        JudgementCountsFromJson(j["judgementCounts"], s.judgementCounts);
    }
    s.biasMs = j.value("biasMs", 0.0);
    s.stdDevMs = j.value("stdDevMs", 0.0);
    s.perfectGreatRatioText = j.value("perfectGreatRatioText", "");
    s.replayId = j.value("replayId", "");
    return true;
}

[[nodiscard]] json RecordToJson(const ScoreRecord& record) {
    json j = SummaryToJson(record.summary);
    j["schemaVersion"] = kScoreSchemaVersion;
    j["chartDomainFirst"] = record.detail.chartDomainFirst;
    j["chartDomainLast"] = record.detail.chartDomainLast;
    j["graphEvents"] = GraphEventsToJson(record.detail.graphEvents);
    j["accuracySteps"] = AccuracyStepsToJson(record.detail.accuracySteps);
    return j;
}

[[nodiscard]] bool RecordFromJson(const json& j, ScoreRecord& record) {
    if (!SummaryFromJson(j, record.summary)) {
        return false;
    }
    record.detail.score = record.summary.score;
    record.detail.accuracyPercent = record.summary.accuracyPercent;
    record.detail.judgementCounts = record.summary.judgementCounts;
    record.detail.biasMs = record.summary.biasMs;
    record.detail.stdDevMs = record.summary.stdDevMs;
    record.detail.perfectGreatRatioText = record.summary.perfectGreatRatioText;
    record.detail.songTitle = record.summary.songTitle;
    record.detail.difficultyName = record.summary.difficultyName;
    record.detail.chartDomainFirst = j.value("chartDomainFirst", 0.0);
    record.detail.chartDomainLast = j.value("chartDomainLast", 0.0);
    if (j.contains("graphEvents")) {
        record.detail.graphEvents = GraphEventsFromJson(j["graphEvents"]);
    }
    if (j.contains("accuracySteps")) {
        record.detail.accuracySteps = AccuracyStepsFromJson(j["accuracySteps"]);
    }
    return true;
}

[[nodiscard]] std::vector<ScoreSummary> ReadIndexFile(const std::filesystem::path& path) {
    std::vector<ScoreSummary> out;
    std::ifstream in(path);
    if (!in) {
        return out;
    }
    try {
        json j;
        in >> j;
        if (!j.is_object() || j.value("schemaVersion", 0) != kScoreSchemaVersion) {
            return out;
        }
        if (!j.contains("summaries") || !j["summaries"].is_array()) {
            return out;
        }
        for (const auto& item : j["summaries"]) {
            ScoreSummary s;
            if (SummaryFromJson(item, s)) {
                out.push_back(std::move(s));
            }
        }
    } catch (...) {
        out.clear();
    }
    return out;
}

bool WriteIndexFile(const std::filesystem::path& path, const std::vector<ScoreSummary>& summaries) {
    json arr = json::array();
    for (const auto& s : summaries) {
        arr.push_back(SummaryToJson(s));
    }
    const json j = {
        {"schemaVersion", kScoreSchemaVersion},
        {"summaries", std::move(arr)},
    };
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        return false;
    }
    out << j.dump(2);
    return !out.fail();
}

[[nodiscard]] std::vector<ScoreSummary> RebuildIndexFromRuns(const std::filesystem::path& dir) {
    std::vector<ScoreSummary> summaries;
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
        return summaries;
    }
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec || !entry.is_regular_file()) {
            continue;
        }
        if (const auto name = PathToUtf8String(entry.path().filename());
            name == "index.json" ||
            !name.ends_with(".json")) {
            continue;
        }
        std::ifstream in(entry.path());
        if (!in) {
            continue;
        }
        try {
            json j;
            in >> j;
            if (ScoreRecord record; RecordFromJson(j, record)) {
                summaries.push_back(std::move(record.summary));
            }
        } catch (...) {
        }
    }
    std::ranges::sort(summaries, [](const ScoreSummary& a, const ScoreSummary& b) {
        return a.timestampUnixSec > b.timestampUnixSec;
    });
    return summaries;
}

} // namespace

std::filesystem::path ScoreStore::ResolveScoresRoot() {
    if (const auto base = PathFromSdlBasePath()) {
        return (*base / "scores").lexically_normal();
    }
    return "scores";
}

std::filesystem::path ScoreStore::ResolveReplaysRoot() {
    return ReplayStore::ResolveReplaysRoot();
}

std::optional<std::string> ScoreStore::Save(
    const Song::SongMetadata& song,
    const int difficultyIndex,
    const ResultsViewData& detail,
    const std::string_view playerName,
    const std::string_view replayId) {
    if (difficultyIndex < 0 || difficultyIndex >= static_cast<int>(song.difficulties.size())) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ScoreStore::Save: invalid difficulty index");
        return std::nullopt;
    }

    const auto& diff = song.difficulties[difficultyIndex];
    ScoreRecord record;
    record.summary.runId = MakeRunId();
    record.summary.songFolder = PathToUtf8String(song.songFolder);
    record.summary.songTitle = song.title;
    record.summary.songArtist = song.artist;
    record.summary.chartPath = PathToUtf8String(diff.chartPath);
    record.summary.difficultyName = diff.name;
    record.summary.playerName = std::string(playerName);
    if (record.summary.playerName.empty()) {
        record.summary.playerName = "Player";
    }
    record.summary.timestampUnixSec = std::chrono::duration_cast<std::chrono::seconds>(
                                          std::chrono::system_clock::now().time_since_epoch())
                                          .count();
    record.summary.score = detail.score;
    record.summary.accuracyPercent = detail.accuracyPercent;
    record.summary.judgementCounts = detail.judgementCounts;
    record.summary.biasMs = detail.biasMs;
    record.summary.stdDevMs = detail.stdDevMs;
    record.summary.perfectGreatRatioText = detail.perfectGreatRatioText;
    record.summary.replayId = std::string(replayId);
    record.detail = detail;
    record.detail.songTitle = song.title;
    record.detail.difficultyName = diff.name;

    const auto dir = SongScoresDir(song.songFolder);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION,
            "ScoreStore::Save: failed to create '%s' (%s)",
            PathToUtf8String(dir).c_str(),
            ec.message().c_str());
        return std::nullopt;
    }

    {
        const auto runFile = RunPath(song.songFolder, record.summary.runId);
        std::ofstream out(runFile, std::ios::trunc);
        if (!out) {
            SDL_LogWarn(
                SDL_LOG_CATEGORY_APPLICATION,
                "ScoreStore::Save: failed to write '%s'",
                PathToUtf8String(runFile).c_str());
            return std::nullopt;
        }
        out << RecordToJson(record).dump(2);
        if (out.fail()) {
            SDL_LogWarn(
                SDL_LOG_CATEGORY_APPLICATION,
                "ScoreStore::Save: write failed for '%s'",
                PathToUtf8String(runFile).c_str());
            return std::nullopt;
        }
    }

    auto summaries = ListForSong(song);
    std::erase_if(summaries, [&](const ScoreSummary& s) { return s.runId == record.summary.runId; });
    summaries.insert(summaries.begin(), record.summary);
    if (!WriteIndexFile(IndexPath(song.songFolder), summaries)) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION,
            "ScoreStore::Save: failed to update index for '%s'",
            record.summary.songFolder.c_str());
    }

    return record.summary.runId;
}

std::vector<ScoreSummary> ScoreStore::ListForSong(const Song::SongMetadata& song) {
    const auto indexFile = IndexPath(song.songFolder);
    auto summaries = ReadIndexFile(indexFile);
    if (!summaries.empty()) {
        return summaries;
    }

    summaries = RebuildIndexFromRuns(SongScoresDir(song.songFolder));
    if (!summaries.empty()) {
        (void)WriteIndexFile(indexFile, summaries);
    }
    return summaries;
}

std::optional<ScoreRecord> ScoreStore::Load(
    const std::filesystem::path& songFolder,
    const std::string& runId) {
    if (runId.empty()) {
        return std::nullopt;
    }
    const auto path = RunPath(songFolder, runId);
    std::ifstream in(path);
    if (!in) {
        return std::nullopt;
    }
    try {
        json j;
        in >> j;
        ScoreRecord record;
        if (!RecordFromJson(j, record)) {
            return std::nullopt;
        }
        return record;
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace Game::Score
