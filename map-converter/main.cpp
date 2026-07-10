#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cctype>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Game/PathUtf8.h"
#include "Game/Gameplay/LaneRemap.h"
#include "miniz.h"
#include "ThirdParty/json.hpp"
#include "font8x8.h"

using json = nlohmann::json;
namespace fs = std::filesystem;
using Game::PathFromSdlBasePath;
using Game::PathToUtf8String;
using Game::Utf8StringToPath;

// App State
struct AppState {
    std::mutex mutex;
    std::string inputPath;
    std::string outputPath;
    std::vector<std::string> logs;
    bool isConverting{false};
    bool conversionFinished{false};
    bool conversionSuccess{false};
    std::string conversionMessage;
};

AppState g_state;

void Log(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    g_state.logs.push_back(message);
    std::cout << message << std::endl;
}

void LogPath(std::string_view prefix, const fs::path& path) {
    Log(std::string(prefix) + PathToUtf8String(path));
}

#ifdef _WIN32
std::string WideArgToUtf8(const wchar_t* warg) {
    return PathToUtf8String(fs::path(warg));
}
#endif

// Helper Functions
std::string Trim(std::string_view str) {
    if (str.empty()) return "";
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return std::string(str.substr(first, (last - first + 1)));
}

std::vector<std::string> Split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string MakeSafeFilename(const std::string& name) {
    std::string safe = name;
    safe.erase(std::remove_if(safe.begin(), safe.end(), [](char c) {
        return c == '\\' || c == '/' || c == '*' || c == '?' || c == ':' || c == '"' || c == '<' || c == '>' || c == '|';
    }), safe.end());
    safe = Trim(safe);
    while (!safe.empty() && safe.back() == '.') {
        safe.pop_back();
        safe = Trim(safe);
    }
    return safe;
}

// ZIP Extraction Wrapper
bool ExtractZip(const fs::path& zipPath, const fs::path& destDir) {
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    FILE* pFile = nullptr;
#ifdef _WIN32
    pFile = _wfopen(zipPath.c_str(), L"rb");
#else
    pFile = fopen(zipPath.c_str(), "rb");
#endif
    if (!pFile) {
        LogPath("Failed to open zip file: ", zipPath);
        return false;
    }

    if (!mz_zip_reader_init_cfile(&zip_archive, pFile, 0, 0)) {
        LogPath("Failed to initialize zip reader for: ", zipPath);
        fclose(pFile);
        return false;
    }

    mz_uint num_files = mz_zip_reader_get_num_files(&zip_archive);
    for (mz_uint i = 0; i < num_files; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
            Log("Failed to get file info for index " + std::to_string(i));
            mz_zip_reader_end(&zip_archive);
            fclose(pFile);
            return false;
        }

#ifdef _WIN32
        fs::path entryPath = destDir / Utf8StringToPath(file_stat.m_filename);
#else
        fs::path entryPath = destDir / file_stat.m_filename;
#endif

        if (mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
            std::error_code ec;
            fs::create_directories(entryPath, ec);
            if (ec) {
                LogPath("Failed to create directory: ", entryPath);
                Log("(" + ec.message() + ")");
                mz_zip_reader_end(&zip_archive);
                fclose(pFile);
                return false;
            }
        } else {
            std::error_code ec;
            fs::create_directories(entryPath.parent_path(), ec);
            if (ec) {
                LogPath("Failed to create parent directory: ", entryPath.parent_path());
                Log("(" + ec.message() + ")");
                mz_zip_reader_end(&zip_archive);
                fclose(pFile);
                return false;
            }

            size_t size = 0;
            void* pData = mz_zip_reader_extract_to_heap(&zip_archive, i, &size, 0);
            if (!pData) {
                Log("Failed to extract file from zip: " + std::string(file_stat.m_filename));
                mz_zip_reader_end(&zip_archive);
                fclose(pFile);
                return false;
            }

            std::ofstream out(entryPath, std::ios::binary);
            if (!out.is_open()) {
                LogPath("Failed to open output file for writing: ", entryPath);
                mz_free(pData);
                mz_zip_reader_end(&zip_archive);
                fclose(pFile);
                return false;
            }
            out.write(static_cast<const char*>(pData), size);
            out.close();
            mz_free(pData);
        }
    }

    mz_zip_reader_end(&zip_archive);
    fclose(pFile);
    return true;
}

// .osu Parser Data Structures
struct TimingPoint {
    int timeMs;
    double bpm;
};

struct Note {
    int timeMs;
    int lane;
    std::string type;
    int durationMs{0};
};

struct OsuMetadata {
    std::string title;
    std::string artist;
    std::string version;
    double bpm{120.0};
    std::string audioFile;
    std::string bgFile;
};

struct OsuChart {
    OsuMetadata metadata;
    std::vector<TimingPoint> timingPoints;
    std::vector<Note> notes;
};

// Parses a single .osu file
std::optional<OsuChart> ParseOsu(const fs::path& filePath) {
    std::ifstream f(filePath);
    if (!f.is_open()) {
        LogPath("Failed to open file: ", filePath);
        return std::nullopt;
    }

    std::string line;
    std::string currentSection;
    std::map<std::string, std::string> general;
    std::map<std::string, std::string> metadata;
    std::map<std::string, std::string> difficulty;
    std::vector<std::string> eventsLines;
    std::vector<std::string> timingPointsLines;
    std::vector<std::string> hitObjectsLines;

    while (std::getline(f, line)) {
        line = Trim(line);
        if (line.empty() || line.starts_with("//")) continue;

        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }

        if (currentSection == "General") {
            auto parts = Split(line, ':');
            if (parts.size() >= 2) {
                general[Trim(parts[0])] = Trim(line.substr(parts[0].length() + 1));
            }
        } else if (currentSection == "Metadata") {
            auto parts = Split(line, ':');
            if (parts.size() >= 2) {
                metadata[Trim(parts[0])] = Trim(line.substr(parts[0].length() + 1));
            }
        } else if (currentSection == "Difficulty") {
            auto parts = Split(line, ':');
            if (parts.size() >= 2) {
                difficulty[Trim(parts[0])] = Trim(line.substr(parts[0].length() + 1));
            }
        } else if (currentSection == "Events") {
            eventsLines.push_back(line);
        } else if (currentSection == "TimingPoints") {
            timingPointsLines.push_back(line);
        } else if (currentSection == "HitObjects") {
            hitObjectsLines.push_back(line);
        }
    }

    // Only support Mania mode (Mode: 3)
    if (general["Mode"] != "3") {
        return std::nullopt;
    }

    OsuChart chart;
    chart.metadata.title = metadata["Title"].empty() ? "Unknown" : metadata["Title"];
    chart.metadata.artist = metadata["Artist"].empty() ? "Unknown" : metadata["Artist"];
    chart.metadata.version = metadata["Version"].empty() ? "Standard" : metadata["Version"];
    chart.metadata.audioFile = general["AudioFilename"];

    int colCount = 4;
    if (difficulty.count("CircleSize")) {
        try {
            colCount = std::stoi(difficulty["CircleSize"]);
        } catch (...) {
            Log("Rejected " + PathToUtf8String(filePath.filename()) + ": invalid CircleSize");
            return std::nullopt;
        }
    }
    if (colCount != 4) {
        Log("Rejected " + PathToUtf8String(filePath.filename()) + ": CircleSize must be 4 (got " +
            std::to_string(colCount) + ")");
        return std::nullopt;
    }

    // Parse background from events
    for (const auto& ev : eventsLines) {
        if (ev.starts_with("0,0,")) {
            auto parts = Split(ev, ',');
            if (parts.size() > 2) {
                std::string bg = Trim(parts[2]);
                if (bg.length() >= 2 && bg.front() == '"' && bg.back() == '"') {
                    bg = bg.substr(1, bg.length() - 2);
                }
                chart.metadata.bgFile = bg;
            }
            break;
        }
    }

    // Parse timing points & base BPM
    double baseBpm = 120.0;
    bool hasBaseBpm = false;
    for (const auto& tpLine : timingPointsLines) {
        auto parts = Split(tpLine, ',');
        if (parts.size() < 8) continue;

        try {
            int timeMs = static_cast<int>(std::round(std::stod(parts[0])));
            double beatLength = std::stod(parts[1]);
            bool uninherited = parts[6] == "1";

            if (uninherited && beatLength > 0.0) {
                double bpm = 60000.0 / beatLength;
                if (!hasBaseBpm) {
                    baseBpm = bpm;
                    hasBaseBpm = true;
                }
                chart.timingPoints.push_back({timeMs, std::round(bpm * 100.0) / 100.0});
            }
        } catch (...) {}
    }
    chart.metadata.bpm = std::round(baseBpm * 100.0) / 100.0;

    // Parse notes
    for (const auto& hoLine : hitObjectsLines) {
        auto parts = Split(hoLine, ',');
        if (parts.size() < 5) continue;

        try {
            int x = std::stoi(parts[0]);
            int timeMs = std::stoi(parts[2]);
            int objType = std::stoi(parts[3]);

            int lane = static_cast<int>(std::floor(x * colCount / 512.0));
            lane = std::max(0, std::min(lane, colCount - 1));
            lane = Game::Gameplay::RemapOsuLaneToCompass(lane);

            Note note;
            note.timeMs = timeMs;
            note.lane = lane;
            note.type = "tap";

            // Long note (hold) has bit 7 set
            if (objType & 128) {
                note.type = "hold";
                if (parts.size() >= 6) {
                    auto endParts = Split(parts[5], ':');
                    if (!endParts.empty()) {
                        int endTimeMs = std::stoi(endParts[0]);
                        note.durationMs = endTimeMs - timeMs;
                    }
                }
            }
            chart.notes.push_back(note);
        } catch (...) {}
    }

    return chart;
}

// Main conversion logic
void RunConversion(const std::string& inputPath, const std::string& outputPath) {
    try {
        {
            std::lock_guard<std::mutex> lock(g_state.mutex);
            g_state.isConverting = true;
            g_state.conversionFinished = false;
            g_state.conversionSuccess = false;
            g_state.conversionMessage = "";
        }

        Log("Preparing input path: " + inputPath);
        fs::path inPath = fs::absolute(Utf8StringToPath(inputPath));
        fs::path outRoot = fs::absolute(Utf8StringToPath(outputPath));
        fs::path tempDir;

        // Determine if input is a valid .osz/.zip archive case-insensitively
        std::error_code isFileEc, isDirEc;
        bool isFile = fs::is_regular_file(inPath, isFileEc);
        bool isDir = fs::is_directory(inPath, isDirEc);

        std::string ext = inPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
        bool isArchive = (ext == ".osz" || ext == ".zip");

        // Prepare source directory
        fs::path sourceDir;
        if (isFile && isArchive) {
            std::error_code ec;
            tempDir = fs::temp_directory_path(ec) / ("cc_game_conv_" + std::to_string(SDL_GetTicks()));
            if (ec) {
                Log("Error: Could not retrieve temporary directory path.");
                std::lock_guard<std::mutex> lock(g_state.mutex);
                g_state.isConverting = false;
                g_state.conversionFinished = true;
                g_state.conversionMessage = "Could not locate system temp directory.";
                return;
            }

            fs::create_directories(tempDir, ec);
            if (ec) {
                LogPath("Error: Could not create temporary directory: ", tempDir);
                std::lock_guard<std::mutex> lock(g_state.mutex);
                g_state.isConverting = false;
                g_state.conversionFinished = true;
                g_state.conversionMessage = "Failed to create temp directory.";
                return;
            }

            LogPath("Extracting ", inPath);
            Log(" to temporary directory...");
            if (!ExtractZip(inPath, tempDir)) {
                Log("Error: Failed to extract ZIP archive.");
                std::error_code cleanEc;
                fs::remove_all(tempDir, cleanEc);
                std::lock_guard<std::mutex> lock(g_state.mutex);
                g_state.isConverting = false;
                g_state.conversionFinished = true;
                g_state.conversionMessage = "Failed to extract ZIP/OSZ archive.";
                return;
            }
            sourceDir = tempDir;
        } else if (isDir) {
            sourceDir = inPath;
        } else {
            LogPath("Error: ", inPath);
            Log(" is not a valid file or directory.");
            std::lock_guard<std::mutex> lock(g_state.mutex);
            g_state.isConverting = false;
            g_state.conversionFinished = true;
            g_state.conversionMessage = "Invalid input file or folder.";
            return;
        }

        // Find and parse all .osu files
        LogPath("Searching for .osu files in: ", sourceDir);
        std::vector<OsuChart> parsedCharts;
        std::error_code dirIterEc;
        for (const auto& entry : fs::directory_iterator(sourceDir, dirIterEc)) {
            std::error_code checkEc;
            std::string entryExt = entry.path().extension().string();
            std::transform(entryExt.begin(), entryExt.end(), entryExt.begin(), [](unsigned char c) { return std::tolower(c); });

            if (entry.is_regular_file(checkEc) && entryExt == ".osu") {
                Log("Parsing chart file: " + PathToUtf8String(entry.path().filename()));
                if (auto chart = ParseOsu(entry.path())) {
                    parsedCharts.push_back(*chart);
                }
            }
        }

        if (parsedCharts.empty()) {
            Log("Error: No valid osu!mania charts found.");
            if (!tempDir.empty()) {
                std::error_code cleanEc;
                fs::remove_all(tempDir, cleanEc);
            }
            std::lock_guard<std::mutex> lock(g_state.mutex);
            g_state.isConverting = false;
            g_state.conversionFinished = true;
            g_state.conversionMessage = "No valid 4-key osu!mania (Mode 3) charts found.";
            return;
        }

        // Group charts by their Audio File
        std::map<std::string, std::vector<OsuChart>> groups;
        for (const auto& chart : parsedCharts) {
            groups[chart.metadata.audioFile].push_back(chart);
        }

        // Process each group as a separate map entry
        int convertedCount = 0;
        for (const auto& [audioFile, charts] : groups) {
            if (charts.empty()) continue;

            const auto& main = charts[0].metadata;
            std::string folderName;
            if (groups.size() > 1) {
                folderName = main.artist + " - " + main.title + " [" + main.version + "]";
            } else {
                folderName = main.artist + " - " + main.title;
            }

            folderName = MakeSafeFilename(folderName);
            fs::path finalOutputPath = outRoot / Utf8StringToPath(folderName);
            fs::path chartsDir = finalOutputPath / "charts";
            
            std::error_code createChartsEc;
            fs::create_directories(chartsDir, createChartsEc);
            if (createChartsEc) {
                LogPath("Error: Failed to create charts directory: ", chartsDir);
                Log("(" + createChartsEc.message() + ")");
                continue;
            }

            json songMetadata = {
                {"title", main.title},
                {"artist", main.artist},
                {"bpm", main.bpm},
                {"audioFile", main.audioFile},
                {"thumbnailPath", main.bgFile},
                {"difficulties", json::array()}
            };

            // Save individual charts
            for (const auto& chart : charts) {
                std::string diffName = chart.metadata.version;
                std::string safeDiffName = MakeSafeFilename(diffName);
                std::string chartFilename = safeDiffName + ".json";

                songMetadata["difficulties"].push_back({
                    {"name", diffName},
                    {"chartFile", "charts/" + chartFilename}
                });

                json chartJson;
                chartJson["formatVersion"] = 2;
                chartJson["song"] = {
                    {"title", chart.metadata.title},
                    {"difficulty", diffName},
                    {"bpm", chart.metadata.bpm},
                    {"offsetMs", 0}
                };

                json timingPointsJ = json::array();
                for (const auto& tp : chart.timingPoints) {
                    timingPointsJ.push_back({
                        {"timeMs", tp.timeMs},
                        {"bpm", tp.bpm}
                    });
                }
                chartJson["timingPoints"] = timingPointsJ;

                json notesJ = json::array();
                for (const auto& note : chart.notes) {
                    json noteJ = {
                        {"timeMs", note.timeMs},
                        {"lane", note.lane},
                        {"type", note.type}
                    };
                    if (note.type == "hold") {
                        noteJ["durationMs"] = note.durationMs;
                    }
                    notesJ.push_back(noteJ);
                }
                chartJson["notes"] = notesJ;

                fs::path chartFilePath = chartsDir / Utf8StringToPath(chartFilename);
                std::ofstream out(chartFilePath);
                if (out.is_open()) {
                    // Safe JSON dump to prevent crash on invalid UTF-8 bytes
                    out << chartJson.dump(2, ' ', false, json::error_handler_t::replace);
                } else {
                    LogPath("Failed to write chart: ", chartFilePath);
                }
            }

            // Save master metadata
            std::ofstream outMeta(finalOutputPath / "song_metadata.json");
            if (outMeta.is_open()) {
                // Safe JSON dump to prevent crash on invalid UTF-8 bytes
                outMeta << songMetadata.dump(2, ' ', false, json::error_handler_t::replace);
            }

            // Copy assets
            std::set<std::string> assetsToCopy;
            if (!main.audioFile.empty()) assetsToCopy.insert(main.audioFile);
            if (!main.bgFile.empty()) assetsToCopy.insert(main.bgFile);

            for (const auto& asset : assetsToCopy) {
                fs::path srcPath = sourceDir / Utf8StringToPath(asset);
                // Case-insensitive search on disk for Linux matching
                std::error_code existsEc;
                if (!fs::exists(srcPath, existsEc)) {
                    std::error_code assetDirEc;
                    for (const auto& entry : fs::directory_iterator(sourceDir, assetDirEc)) {
                        std::error_code checkEc;
                        if (entry.is_regular_file(checkEc)) {
                            const std::string entryName = PathToUtf8String(entry.path().filename());
                            if (SDL_strcasecmp(entryName.c_str(), asset.c_str()) == 0) {
                                srcPath = entry.path();
                                break;
                            }
                        }
                    }
                }

                std::error_code existsCheckEc;
                if (fs::exists(srcPath, existsCheckEc)) {
                    fs::path destPath = finalOutputPath / srcPath.filename();
                    std::error_code parentDirEc;
                    fs::create_directories(destPath.parent_path(), parentDirEc);
                    std::error_code copyEc;
                    fs::copy_file(srcPath, destPath, fs::copy_options::overwrite_existing, copyEc);
                    if (copyEc) {
                        Log("Warning: Failed to copy asset " + asset + ": " + copyEc.message());
                    } else {
                        Log("Copied asset: " + PathToUtf8String(srcPath.filename()));
                    }
                } else {
                    Log("Warning: Asset not found: " + asset);
                }
            }

            Log("Converted group: " + folderName + " (" + std::to_string(charts.size()) + " difficulties)");
            convertedCount++;
        }

        // Cleanup temp dir
        if (!tempDir.empty()) {
            std::error_code cleanEc;
            fs::remove_all(tempDir, cleanEc);
        }

        LogPath("All conversions complete. Saved in: ", outRoot);

        std::lock_guard<std::mutex> lock(g_state.mutex);
        g_state.isConverting = false;
        g_state.conversionFinished = true;
        g_state.conversionSuccess = true;
        g_state.conversionMessage = "Successfully converted " + std::to_string(convertedCount) + " song groups.";
        
    } catch (const std::exception& e) {
        Log("Exception caught during conversion: " + std::string(e.what()));
        std::lock_guard<std::mutex> lock(g_state.mutex);
        g_state.isConverting = false;
        g_state.conversionFinished = true;
        g_state.conversionSuccess = false;
        g_state.conversionMessage = "Critical Error: " + std::string(e.what());
    } catch (...) {
        Log("Unknown exception caught during conversion.");
        std::lock_guard<std::mutex> lock(g_state.mutex);
        g_state.isConverting = false;
        g_state.conversionFinished = true;
        g_state.conversionSuccess = false;
        g_state.conversionMessage = "Critical Error: An unknown exception occurred.";
    }
}

// SDL3 Font Drawing Helper
void DrawText8x8(SDL_Renderer* renderer, const std::string& text, float x, float y, float scale, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    float startX = x;
    for (char c : text) {
        if (c == '\n') {
            y += 8.0f * scale + 4.0f;
            x = startX;
            continue;
        }

        int idx = static_cast<unsigned char>(c);
        if (idx >= 128) idx = '?';

        for (int row = 0; row < 8; ++row) {
            unsigned char row_bits = font8x8_basic[idx][row];
            for (int col = 0; col < 8; ++col) {
                if ((row_bits >> col) & 1) {
                    SDL_FRect rect{ x + col * scale, y + row * scale, scale, scale };
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }
        x += 8.0f * scale + 1.0f; // Char width + spacing
    }
}

// SDL3 File Dialog Callbacks
void SDLCALL InputFileCallback(void* userdata, const char* const* filelist, int filter) {
    if (!filelist) {
        Log("File dialog error: " + std::string(SDL_GetError()));
        return;
    }
    if (*filelist == nullptr) return; // User cancelled
    
    std::string path = filelist[0];
    {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        g_state.inputPath = path;
    }
    Log("Selected input file: " + path);
}

void SDLCALL InputFolderCallback(void* userdata, const char* const* filelist, int filter) {
    if (!filelist) {
        Log("Folder dialog error: " + std::string(SDL_GetError()));
        return;
    }
    if (*filelist == nullptr) return; // User cancelled
    
    std::string path = filelist[0];
    {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        g_state.inputPath = path;
    }
    Log("Selected input folder: " + path);
}

void SDLCALL OutputFolderCallback(void* userdata, const char* const* filelist, int filter) {
    if (!filelist) {
        Log("Folder dialog error: " + std::string(SDL_GetError()));
        return;
    }
    if (*filelist == nullptr) return; // User cancelled
    
    std::string path = filelist[0];
    {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        g_state.outputPath = path;
    }
    Log("Selected output folder: " + path);
}


// Main Program Entry
int main(int argc, char* argv[]) {
    std::string defaultOutputPath;
    if (const auto base = PathFromSdlBasePath()) {
        defaultOutputPath = PathToUtf8String((*base / "songs").lexically_normal());
    } else {
        defaultOutputPath = "songs";
    }
    {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        g_state.outputPath = defaultOutputPath;
    }

    // CLI MODE: if arguments are passed
#ifdef _WIN32
    int argcW = 0;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argcW);
    if (argvW && argcW > 1) {
        const std::string input = WideArgToUtf8(argvW[1]);
        const std::string output = (argcW > 2) ? WideArgToUtf8(argvW[2]) : defaultOutputPath;
        LocalFree(argvW);

        std::cout << "CC Map Converter - Console Mode" << std::endl;
        std::cout << "Input: " << input << std::endl;
        std::cout << "Output: " << output << std::endl;

        RunConversion(input, output);
        return g_state.conversionSuccess ? 0 : 1;
    }
    if (argvW) {
        LocalFree(argvW);
    }
#else
    if (argc > 1) {
        const std::string input = argv[1];
        const std::string output = (argc > 2) ? argv[2] : defaultOutputPath;

        std::cout << "CC Map Converter - Console Mode" << std::endl;
        std::cout << "Input: " << input << std::endl;
        std::cout << "Output: " << output << std::endl;

        RunConversion(input, output);
        return g_state.conversionSuccess ? 0 : 1;
    }
#endif

    // GUI MODE: SDL3 Windows/Linux native Dialog loop
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("cc-game Map Converter", 680, 500, 0);
    if (!window) {
        std::cerr << "Failed to create SDL Window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "Failed to create SDL Renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Log("GUI Mode");
    Log("Default output path: " + defaultOutputPath);

    struct Button {
        SDL_FRect rect;
        std::string label;
        std::function<void()> onClick;
        bool hovered{false};
    };

    std::vector<Button> buttons = {
        { { 40.0f, 110.0f, 150.0f, 30.0f }, "Browse File...", [window]() {
            SDL_DialogFileFilter filters[] = {
                { "osu!mania map archive", "osz" },
                { "Zip archive", "zip" },
                { "All files", "*" }
            };
            SDL_ShowOpenFileDialog(InputFileCallback, nullptr, window, filters, 3, nullptr, false);
        } },
        { { 200.0f, 110.0f, 150.0f, 30.0f }, "Browse Folder...", [window]() {
            SDL_ShowOpenFolderDialog(InputFolderCallback, nullptr, window, nullptr, false);
        } },
        { { 40.0f, 210.0f, 150.0f, 30.0f }, "Browse...", [window]() {
            SDL_ShowOpenFolderDialog(OutputFolderCallback, nullptr, window, nullptr, false);
        } },
        { { 40.0f, 270.0f, 600.0f, 40.0f }, "CONVERT MAP(S)", [window]() {
            std::string in, out;
            {
                std::lock_guard<std::mutex> lock(g_state.mutex);
                if (g_state.isConverting) return;
                in = g_state.inputPath;
                out = g_state.outputPath;
            }
            if (in.empty()) {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Missing Input", "Please select a valid .osz file or folder first.", window);
                return;
            }
            if (out.empty()) {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Missing Output", "Please select a valid output folder.", window);
                return;
            }

            // Start conversion in separate thread
            std::thread(RunConversion, in, out).detach();
        } }
    };

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                float mx = event.motion.x;
                float my = event.motion.y;
                for (auto& btn : buttons) {
                    btn.hovered = (mx >= btn.rect.x && mx <= btn.rect.x + btn.rect.w &&
                                   my >= btn.rect.y && my <= btn.rect.y + btn.rect.h);
                }
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    float mx = event.button.x;
                    float my = event.button.y;
                    
                    bool converting = false;
                    {
                        std::lock_guard<std::mutex> lock(g_state.mutex);
                        converting = g_state.isConverting;
                    }
                    
                    if (!converting) {
                        for (auto& btn : buttons) {
                            if (mx >= btn.rect.x && mx <= btn.rect.x + btn.rect.w &&
                                my >= btn.rect.y && my <= btn.rect.y + btn.rect.h) {
                                btn.onClick();
                            }
                        }
                    }
                }
            }
        }

        // Handle conversion completion alerts
        bool finished = false;
        bool success = false;
        std::string alertMsg;
        {
            std::lock_guard<std::mutex> lock(g_state.mutex);
            if (g_state.conversionFinished) {
                finished = true;
                success = g_state.conversionSuccess;
                alertMsg = g_state.conversionMessage;
                g_state.conversionFinished = false; // Reset alert flag
            }
        }

        if (finished) {
            if (success) {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Success", alertMsg.c_str(), window);
            } else {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Conversion Failed", alertMsg.c_str(), window);
            }
        }

        // Render UI

        // Background
        SDL_SetRenderDrawColor(renderer, 26, 26, 36, 255); // Dark blue-gray
        SDL_RenderClear(renderer);

        // Title
        DrawText8x8(renderer, "cc-game Map Converter", 40.0f, 25.0f, 2.0f, { 255, 255, 255, 255 });
        SDL_SetRenderDrawColor(renderer, 70, 70, 95, 255);
        SDL_FRect titleBar{ 40.0f, 50.0f, 600.0f, 2.0f };
        SDL_RenderFillRect(renderer, &titleBar);

        // Get snapshot of state variables
        std::string inputPathSnapshot;
        std::string outputPathSnapshot;
        std::vector<std::string> logsSnapshot;
        bool convertingSnapshot = false;
        {
            std::lock_guard<std::mutex> lock(g_state.mutex);
            inputPathSnapshot = g_state.inputPath;
            outputPathSnapshot = g_state.outputPath;
            logsSnapshot = g_state.logs;
            convertingSnapshot = g_state.isConverting;
        }

        // Input section
        DrawText8x8(renderer, "INPUT PATH (.osz or folder):", 40.0f, 65.0f, 1.0f, { 150, 150, 180, 255 });
        std::string displayIn = inputPathSnapshot.empty() ? "<Please select an input path>" : inputPathSnapshot;
        if (displayIn.length() > 70) displayIn = "..." + displayIn.substr(displayIn.length() - 67);
        DrawText8x8(renderer, displayIn, 40.0f, 85.0f, 1.0f, inputPathSnapshot.empty() ? SDL_Color{ 200, 80, 80, 255 } : SDL_Color{ 200, 200, 255, 255 });

        // Output section
        DrawText8x8(renderer, "OUTPUT FOLDER (saves into sub-folders):", 40.0f, 160.0f, 1.0f, { 150, 150, 180, 255 });
        std::string displayOut = outputPathSnapshot.empty() ? "<Please select an output folder>" : outputPathSnapshot;
        if (displayOut.length() > 70) displayOut = "..." + displayOut.substr(displayOut.length() - 67);
        DrawText8x8(renderer, displayOut, 40.0f, 185.0f, 1.0f, { 200, 200, 255, 255 });

        // Draw buttons (only if not actively converting)
        for (const auto& btn : buttons) {
            // "CONVERT MAP(S)" is disabled if converting
            if (convertingSnapshot && btn.label == "CONVERT MAP(S)") {
                SDL_SetRenderDrawColor(renderer, 45, 45, 55, 255);
                SDL_RenderFillRect(renderer, &btn.rect);
                SDL_SetRenderDrawColor(renderer, 80, 80, 90, 255);
                SDL_RenderRect(renderer, &btn.rect);
                // Center text
                float textX = btn.rect.x + (btn.rect.w - btn.label.length() * 9.0f) / 2.0f;
                float textY = btn.rect.y + (btn.rect.h - 8.0f) / 2.0f;
                DrawText8x8(renderer, "CONVERTING...", textX, textY, 1.0f, { 100, 100, 110, 255 });
                continue;
            }

            if (convertingSnapshot) {
                // Dim other buttons during conversion
                SDL_SetRenderDrawColor(renderer, 35, 35, 45, 255);
                SDL_RenderFillRect(renderer, &btn.rect);
                SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
                SDL_RenderRect(renderer, &btn.rect);
                float textX = btn.rect.x + (btn.rect.w - btn.label.length() * 9.0f) / 2.0f;
                float textY = btn.rect.y + (btn.rect.h - 8.0f) / 2.0f;
                DrawText8x8(renderer, btn.label, textX, textY, 1.0f, { 90, 90, 100, 255 });
                continue;
            }

            // Normal button state
            SDL_SetRenderDrawColor(renderer, btn.hovered ? 60 : 45, btn.hovered ? 60 : 45, btn.hovered ? 90 : 75, 255);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 100, 100, 140, 255);
            SDL_RenderRect(renderer, &btn.rect);

            float textX = btn.rect.x + (btn.rect.w - btn.label.length() * 9.0f) / 2.0f;
            float textY = btn.rect.y + (btn.rect.h - 8.0f) / 2.0f;
            DrawText8x8(renderer, btn.label, textX, textY, 1.0f, { 255, 255, 255, 255 });
        }

        // Draw Logs Area
        DrawText8x8(renderer, "PROGRESS LOGS:", 40.0f, 330.0f, 1.0f, { 150, 150, 180, 255 });
        SDL_FRect logBox{ 40.0f, 350.0f, 600.0f, 120.0f };
        SDL_SetRenderDrawColor(renderer, 15, 15, 20, 255); // Dark black-blue bg
        SDL_RenderFillRect(renderer, &logBox);
        SDL_SetRenderDrawColor(renderer, 50, 50, 70, 255);
        SDL_RenderRect(renderer, &logBox);

        // Render log lines inside the box
        float logY = 360.0f;
        int maxLines = 9;
        int startIdx = std::max(0, static_cast<int>(logsSnapshot.size()) - maxLines);
        for (size_t idx = startIdx; idx < logsSnapshot.size(); ++idx) {
            std::string logLine = logsSnapshot[idx];
            if (logLine.length() > 64) logLine = logLine.substr(0, 61) + "...";
            DrawText8x8(renderer, logLine, 50.0f, logY, 1.0f, { 160, 220, 160, 255 }); // Retro green logs
            logY += 11.0f;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
