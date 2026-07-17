#include <cassert>
#include <clocale>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "Game/PathUtf8.h"
#include "ThirdParty/json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

using Game::AsciiToLowerUtf8;
using Game::PathEqualsAsciiICaseUtf8;
using Game::PathEqualsUtf8;
using Game::PathExtensionAsciiLowerUtf8;
using Game::PathExtensionUtf8;
using Game::PathFromSdlUtf8;
using Game::PathToUtf8String;
using Game::Utf8StringToPath;

namespace {

// Split adjacent hex escapes from following hex digits (C++ \x greedily continues).
// "Straße"
constexpr char kStrasse[] = "\x53\x74\x72\x61\xC3\x9F" "e";
// "Känguru - Für Elise"
constexpr char kGermanSongFolder[] =
    "K\xC3\xA4" "nguru - F\xC3\xBC" "r Elise";
// "audio_für_dich.mp3"
constexpr char kGermanAudioFile[] =
    "audio_f\xC3\xBC" "r_dich.mp3";
// "charts/Schwierigkeiten.json" difficulty chart relative path
constexpr char kGermanChartRel[] =
    "charts/Schwierigkeiten.json";
// CJK + emoji ("日本語_🎵")
constexpr char kCjkEmoji[] =
    "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E_\xF0\x9F\x8E\xB5";

void ExpectRoundTrip(const std::string_view utf8) {
    const std::string in(utf8);
    const fs::path p = Utf8StringToPath(in);
    assert(PathToUtf8String(p) == in);
}

void TestRoundTripHexFixtures() {
    const char* cases[] = {
        "",
        "ascii.txt",
        "songs/normal/chart.json",
        kStrasse,
        kGermanSongFolder,
        kGermanAudioFile,
        kGermanChartRel,
        kCjkEmoji,
        "a b/c#d/%20e&f?.ogg",
        "mix\\ed/separators\\here.json",
    };
    for (const char* c : cases) {
        ExpectRoundTrip(c);
    }
}

void TestSdlUtf8RoundTrip() {
    assert(PathFromSdlUtf8(nullptr).empty());
    assert(PathFromSdlUtf8("").empty());

    const char* cases[] = {
        "ascii.txt",
        kStrasse,
        kGermanSongFolder,
        kGermanAudioFile,
        kCjkEmoji,
    };
    for ([[maybe_unused]] const char* c : cases) {
        assert(PathToUtf8String(PathFromSdlUtf8(c)) == c);
    }
}

void TestJoinPreservesUtf8() {
    const fs::path root = Utf8StringToPath(kGermanSongFolder);
    const fs::path full = root / Utf8StringToPath(kGermanChartRel);
    assert(PathToUtf8String(full.filename()) == "Schwierigkeiten.json");
    assert(PathEqualsUtf8(full.parent_path().filename(), Utf8StringToPath("charts")));
}

void TestExtensionHelpers() {
    assert(PathExtensionUtf8(Utf8StringToPath("a.OSU")) == ".OSU");
    assert(PathExtensionAsciiLowerUtf8(Utf8StringToPath("a.OSU")) == ".osu");
    assert(PathExtensionAsciiLowerUtf8(Utf8StringToPath("pack.ZiP")) == ".zip");
    assert(PathExtensionAsciiLowerUtf8(Utf8StringToPath("pack.OSZ")) == ".osz");
    assert(PathExtensionAsciiLowerUtf8(Utf8StringToPath(kGermanAudioFile)) == ".mp3");
    assert(PathExtensionUtf8(Utf8StringToPath("noext")) == "");

    // ASCII fold only, ß is not folded to "ss" leading ASCII still lowercases.
    assert(AsciiToLowerUtf8(kStrasse) == "stra\xC3\x9F" "e");
    assert(PathEqualsAsciiICaseUtf8("Audio.MP3", "audio.mp3"));
    assert(!PathEqualsAsciiICaseUtf8(kStrasse, "strasse"));
}

void TestLocaleDoesNotChangeUtf8Bytes() {
    const auto before = PathToUtf8String(Utf8StringToPath(kStrasse));
    assert(before == kStrasse);

    // Stress C locale and a German locale if available, UTF-8 helpers must ignore them.
    (void)std::setlocale(LC_ALL, "C");
    assert(PathToUtf8String(Utf8StringToPath(kStrasse)) == kStrasse);
    assert(PathToUtf8String(Utf8StringToPath(kGermanSongFolder)) == kGermanSongFolder);

    if (std::setlocale(LC_ALL, "de_DE.UTF-8") || std::setlocale(LC_ALL, "de-DE") ||
        std::setlocale(LC_ALL, "German")) {
        assert(PathToUtf8String(Utf8StringToPath(kStrasse)) == kStrasse);
        assert(PathToUtf8String(Utf8StringToPath(kGermanSongFolder)) == kGermanSongFolder);
        assert(PathExtensionAsciiLowerUtf8(Utf8StringToPath("x.OSU")) == ".osu");
    }

    (void)std::setlocale(LC_ALL, "C");
}

#ifdef _WIN32
void TestStringIsNotCultureInvariant() {
    const fs::path p = Utf8StringToPath(kStrasse);
    const std::string utf8 = PathToUtf8String(p);
    assert(utf8 == kStrasse);

    // path.string() is locale/ACP-dependent on Windows (MSVC STL). Production code
    // must never use it for JSON/SDL/logs. When ACP != UTF-8 (65001), bytes often
    // diverge from UTF-8, some STLs (libstdc++) may still emit UTF-8 here, that
    // does not make .string() a portable contract.
    if (GetACP() != 65001) {
        const std::string narrow = p.string();
        (void)narrow; // TODO: canary only, do not assert inequality (STL-dependent)
    }
}
#endif

void TestGermanSongPipeline() {
    const fs::path base =
        fs::temp_directory_path() / Utf8StringToPath("cc_game_path_utf8_de_test");
    std::error_code ec;
    fs::remove_all(base, ec);
    ec.clear();

    const fs::path songFolder = base / Utf8StringToPath(kGermanSongFolder);
    const fs::path chartsDir = songFolder / "charts";
    fs::create_directories(chartsDir, ec);
    assert(!ec);

    const fs::path chartPath = songFolder / Utf8StringToPath(kGermanChartRel);
    const fs::path audioPath = songFolder / Utf8StringToPath(kGermanAudioFile);
    const fs::path metaPath = songFolder / "song_metadata.json";

    {
        std::ofstream chart(chartPath);
        assert(chart.is_open());
        chart << R"({"formatVersion":2,"notes":[]})";
    }
    {
        std::ofstream audio(audioPath, std::ios::binary);
        assert(audio.is_open());
        audio << "fake-audio";
    }

    {
        // Converter-shaped metadata, UTF-8 path strings in JSON
        json meta = {
                {"title", "F\xC3\xBC" "r Elise"},
                {"artist", "K\xC3\xA4" "nguru"},
                {"bpm", 120.0},
                {"audioFile", kGermanAudioFile},
                {"thumbnailPath", ""},
                {"difficulties",
                    json::array({{{"name", "Normal"}, {"chartFile", kGermanChartRel}, {"level", 1}}})},
            };
        std::ofstream out(metaPath);
        assert(out.is_open());
        out << meta.dump(2);
    }

    // Library scan stand-in, folder name from directory_iterator -> UTF-8
    for (const auto& entry : fs::directory_iterator(base)) {
        if (!entry.is_directory()) {
            continue;
        }
        if (PathToUtf8String(entry.path().filename()) == kGermanSongFolder) {
            bool foundFolder = false;
            foundFolder = true;
        }
    }
    assert(foundFolder);

    // Chart path, keep fs::path (GameplayScene chart load)
    assert(fs::exists(chartPath));
    {
        std::ifstream in(chartPath);
        assert(in.is_open());
        json j;
        in >> j;
        assert(j.at("formatVersion").get<int>() == 2);
    }

    // Audio path cross SDL boundary via PathToUtf8String then back (ResourceManager stand-in)
    const std::string audioUtf8 = PathToUtf8String(audioPath);
    assert(audioUtf8.find(kGermanAudioFile) != std::string::npos);
    {
        std::ifstream in(Utf8StringToPath(audioUtf8), std::ios::binary);
        assert(in.is_open());
        std::string body;
        in >> body;
        assert(body == "fake-audio");
    }

    // Cache / JSON round-trip like SongManager serialize
    const std::string folderStored = PathToUtf8String(songFolder.filename());
    const std::string chartStored = PathToUtf8String(Utf8StringToPath(kGermanChartRel));
    assert(folderStored == kGermanSongFolder);
    assert(chartStored == kGermanChartRel);

    json cacheEntry = {
        {"songFolder", folderStored},
        {"audioFile", kGermanAudioFile},
        {"chartPath", chartStored},
    };
    const std::string dumped = cacheEntry.dump();
    const json parsed = json::parse(dumped);
    assert(PathToUtf8String(Utf8StringToPath(parsed.at("songFolder").get<std::string>())) ==
           kGermanSongFolder);
    assert(parsed.at("audioFile").get<std::string>() == kGermanAudioFile);
    assert(PathToUtf8String(Utf8StringToPath(parsed.at("chartPath").get<std::string>())) ==
           kGermanChartRel);

    // ResolveSongFile stand-in, songsRoot / songFolder / relative
    const fs::path resolvedChart =
        (base / Utf8StringToPath(folderStored) / Utf8StringToPath(chartStored)).lexically_normal();
    const fs::path resolvedAudio =
        (base / Utf8StringToPath(folderStored) / Utf8StringToPath(kGermanAudioFile))
            .lexically_normal();
    assert(fs::exists(resolvedChart));
    assert(fs::exists(resolvedAudio));
    assert(PathEqualsUtf8(resolvedChart.filename(), Utf8StringToPath("Schwierigkeiten.json")));

    fs::remove_all(base, ec);
}

} // namespace

int main() {
    TestRoundTripHexFixtures();
    TestSdlUtf8RoundTrip();
    TestJoinPreservesUtf8();
    TestExtensionHelpers();
    TestLocaleDoesNotChangeUtf8Bytes();
#ifdef _WIN32
    TestStringIsNotCultureInvariant();
#endif
    TestGermanSongPipeline();
    return 0;
}
