#include <cassert>
#include <filesystem>
#include <fstream>
#include "Game/Gameplay/ChartLoader.h"

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
        "formatVersion": 2,
        "song": { "offsetMs": 0 },
        "notes": []
    })");
    auto res = LoadChartFromFile(kTempChartPath);
    CleanupTempFile();

    assert(!res.has_value());
    assert(res.error() == ChartLoadError::UnsupportedVersion);
}

void TestNoNotes() {
    WriteTempFile(R"({
        "formatVersion": 1,
        "song": { "offsetMs": 0 },
        "notes": []
    })");
    auto res = LoadChartFromFile(kTempChartPath);
    CleanupTempFile();

    assert(!res.has_value());
    assert(res.error() == ChartLoadError::NoNotes);
}

void TestValidChartAndFiltering() {
    // Valid formatVersion 1.
    // Has a note with lane 4 (invalid), a note with timeMs < 0 (invalid),
    // and valid notes out of chronological order to test sorting.
    WriteTempFile(R"({
        "formatVersion": 1,
        "song": { "offsetMs": 100.0 },
        "notes": [
            { "type": "tap", "lane": 0, "timeMs": 500.0 },
            { "type": "tap", "lane": 4, "timeMs": 200.0 },
            { "type": "tap", "lane": 1, "timeMs": -50.0 },
            { "type": "tap", "lane": 2, "timeMs": 250.0 },
            { "type": "tap", "lane": 3, "timeMs": 1000.0 }
        ]
    })");

    const auto res = LoadChartFromFile(kTempChartPath);
    CleanupTempFile();

    assert(res.has_value());
    const auto& [formatVersion, offsetSeconds, notes] = *res;

    // offsetSeconds should be 100.0ms * 1e-3 = 0.1s
    assert(offsetSeconds == 0.1);
    assert(formatVersion == 1);

    // Only 3 valid notes: (lane 0, 500ms), (lane 2, 250ms), (lane 3, 1000ms)
    // Sorted chronologically:
    // 1st: lane 2 at 250ms -> 0.25s + 0.1s offset = 0.35s
    // 2nd: lane 0 at 500ms -> 0.5s + 0.1s offset = 0.6s
    // 3rd: lane 3 at 1000ms -> 1.0s + 0.1s offset = 1.1s
    assert(notes.size() == 3);

    assert(notes[0].lane == 2);
    assert(std::abs(notes[0].hitTime - 0.35) < 1e-6);

    assert(notes[1].lane == 0);
    assert(std::abs(notes[1].hitTime - 0.6) < 1e-6);

    assert(notes[2].lane == 3);
    assert(std::abs(notes[2].hitTime - 1.1) < 1e-6);
}

} // namespace

int main() {
    TestFileNotFound();
    TestInvalidJson();
    TestUnsupportedVersion();
    TestNoNotes();
    TestValidChartAndFiltering();
    return 0;
}
