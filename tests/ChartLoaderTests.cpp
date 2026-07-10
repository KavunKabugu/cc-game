#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>

#include "Game/Gameplay/ChartLoader.h"
#include "Game/Gameplay/LaneRemap.h"

namespace {

using namespace Game::Gameplay;

const std::filesystem::path kTempChartPath = "temp_test_chart.json";

void WriteTempFile(const std::string& content) {
    std::ofstream out(kTempChartPath);
    assert(out.is_open());
    out << content;
    out.close();
}

void CleanupTempFile() {
    if (std::filesystem::exists(kTempChartPath)) {
        std::filesystem::remove(kTempChartPath);
    }
}

void TestLaneRemapHelpers() {
    assert(RemapOsuLaneToCompass(0) == 0);
    assert(RemapOsuLaneToCompass(1) == 3);
    assert(RemapOsuLaneToCompass(2) == 1);
    assert(RemapOsuLaneToCompass(3) == 2);
    assert(RemapOsuLaneToCompass(-1) == -1);
    assert(RemapOsuLaneToCompass(4) == 4);

    assert(SwapUpDownLane(0) == 0);
    assert(SwapUpDownLane(1) == 3);
    assert(SwapUpDownLane(2) == 2);
    assert(SwapUpDownLane(3) == 1);
}

void TestFileNotFound() {
    auto res = LoadChartFromFile("non_existent_file.json");
    assert(!res.has_value());
    assert(res.error() == ChartLoadError::FileNotFound);
}

void TestInvalidJson() {
    WriteTempFile("{ invalid json: ");
    auto res = LoadChartFromFile(kTempChartPath);
    CleanupTempFile();

    assert(!res.has_value());
    assert(res.error() == ChartLoadError::InvalidJson);
}

void TestUnsupportedVersion() {
    WriteTempFile(R"({
        "formatVersion": 3,
        "song": { "offsetMs": 0 },
        "notes": [{ "type": "tap", "lane": 0, "timeMs": 100.0 }]
    })");
    auto res = LoadChartFromFile(kTempChartPath);
    CleanupTempFile();

    assert(!res.has_value());
    assert(res.error() == ChartLoadError::UnsupportedVersion);
}

void TestNoNotes() {
    WriteTempFile(R"({
        "formatVersion": 2,
        "song": { "offsetMs": 0 },
        "notes": []
    })");
    auto res = LoadChartFromFile(kTempChartPath);
    CleanupTempFile();

    assert(!res.has_value());
    assert(res.error() == ChartLoadError::NoNotes);
}

void TestValidFormatVersion1RemapsLanes() {
    // formatVersion 1 stores raw osu!mania columns (L,D,U,R).
    // After load they must be remapped to compass (L,U,R,D).
    WriteTempFile(R"({
        "formatVersion": 1,
        "song": { "offsetMs": 100.0 },
        "notes": [
            { "type": "tap", "lane": 0, "timeMs": 500.0 },
            { "type": "tap", "lane": 4, "timeMs": 200.0 },
            { "type": "tap", "lane": 1, "timeMs": -50.0 },
            { "type": "tap", "lane": 2, "timeMs": 250.0 },
            { "type": "hold", "lane": 1, "timeMs": 750.0, "durationMs": 500.0 },
            { "type": "tap", "lane": 3, "timeMs": 1000.0 }
        ]
    })");

    const auto res = LoadChartFromFile(kTempChartPath);
    CleanupTempFile();

    assert(res.has_value());
    const auto& [formatVersion, offsetSeconds, notes] = *res;

    assert(offsetSeconds == 0.1);
    assert(formatVersion == 1);

    // Valid notes (raw osu lanes): 0@500, 2@250, 1@750, 3@1000
    // Remapped compass lanes:      0,     1,     3,     2
    // Sorted chronologically:
    // 1st: osu 2 -> compass 1 at 0.35s
    // 2nd: osu 0 -> compass 0 at 0.6s
    // 3rd: osu 1 -> compass 3 at 0.85s
    // 4th: osu 3 -> compass 2 at 1.1s
    assert(notes.size() == 4);

    assert(notes[0].lane == 1);
    assert(std::abs(notes[0].hitTime - 0.35) < 1e-6);

    assert(notes[1].lane == 0);
    assert(std::abs(notes[1].hitTime - 0.6) < 1e-6);

    assert(notes[2].lane == 3);
    assert(std::abs(notes[2].hitTime - 0.85) < 1e-6);

    assert(notes[3].lane == 2);
    assert(std::abs(notes[3].hitTime - 1.1) < 1e-6);
}

void TestValidFormatVersion2Passthrough() {
    // formatVersion 2 already uses compass lanes, leave them unchanged.
    WriteTempFile(R"({
        "formatVersion": 2,
        "song": { "offsetMs": 0 },
        "notes": [
            { "type": "tap", "lane": 0, "timeMs": 100.0 },
            { "type": "tap", "lane": 1, "timeMs": 200.0 },
            { "type": "tap", "lane": 2, "timeMs": 300.0 },
            { "type": "tap", "lane": 3, "timeMs": 400.0 }
        ]
    })");

    const auto res = LoadChartFromFile(kTempChartPath);
    CleanupTempFile();

    assert(res.has_value());
    assert(res->formatVersion == 2);
    assert(res->notes.size() == 4);
    assert(res->notes[0].lane == 0);
    assert(res->notes[1].lane == 1);
    assert(res->notes[2].lane == 2);
    assert(res->notes[3].lane == 3);
}

} // namespace

int main() {
    TestLaneRemapHelpers();
    TestFileNotFound();
    TestInvalidJson();
    TestUnsupportedVersion();
    TestNoNotes();
    TestValidFormatVersion1RemapsLanes();
    TestValidFormatVersion2Passthrough();
    return 0;
}
