#ifndef CC_GAME_PATH_UTF8_H
#define CC_GAME_PATH_UTF8_H

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <SDL3/SDL_filesystem.h>

namespace Game {

#ifdef _WIN32
inline std::string PathToUtf8String(const std::filesystem::path& p) {
    const auto u8 = p.u8string();
    return std::string(u8.begin(), u8.end());
}

inline std::filesystem::path Utf8StringToPath(const std::string& str) {
    return std::filesystem::path(std::u8string(str.begin(), str.end()));
}

inline std::filesystem::path PathFromSdlUtf8(const char* raw) {
    if (!raw || raw[0] == '\0') {
        return {};
    }
    const std::string s(raw);
    const int sizeNeeded =
        MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring wstr(sizeNeeded, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), wstr.data(), sizeNeeded);
    return std::filesystem::path(wstr);
}
#else
inline std::string PathToUtf8String(const std::filesystem::path& p) {
    return p.string();
}

inline std::filesystem::path Utf8StringToPath(const std::string& str) {
    return std::filesystem::path(str);
}

inline std::filesystem::path PathFromSdlUtf8(const char* raw) {
    if (!raw || raw[0] == '\0') {
        return {};
    }
    return std::filesystem::path(raw);
}
#endif

// ASCII-only lowercasing for culture-invariant extension / ASCII name compares.
// Does not apply Unicode case folding (e.g. German ß); use only on known-ASCII text.
[[nodiscard]] inline std::string AsciiToLowerUtf8(const std::string_view utf8) {
    std::string out(utf8);
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return out;
}

// Extension as UTF-8 (includes leading '.'). Empty if path has no extension.
[[nodiscard]] inline std::string PathExtensionUtf8(const std::filesystem::path& p) {
    return PathToUtf8String(p.extension());
}

// Extension as UTF-8, ASCII-lowercased (".OSU" -> ".osu"). Culture-invariant (should be).
[[nodiscard]] inline std::string PathExtensionAsciiLowerUtf8(const std::filesystem::path& p) {
    return AsciiToLowerUtf8(PathExtensionUtf8(p));
}

[[nodiscard]] inline bool PathEqualsUtf8(const std::filesystem::path& a, const std::filesystem::path& b) {
    return PathToUtf8String(a) == PathToUtf8String(b);
}

// Byte-identical UTF-8 equality after ASCII-only case fold. For ASCII names/extensions only.
// Pass UTF-8 string views (or PathToUtf8String results), do not pass raw narrow ACP paths.
[[nodiscard]] inline bool PathEqualsAsciiICaseUtf8(const std::string_view aUtf8, const std::string_view bUtf8) {
    return AsciiToLowerUtf8(aUtf8) == AsciiToLowerUtf8(bUtf8);
}

// SDL3 SDL_GetBasePath returns const char* owned by SDL, DO NOT SDL_free.
inline std::optional<std::filesystem::path> PathFromSdlBasePath() {
    const char* raw = SDL_GetBasePath();
    if (!raw || raw[0] == '\0') {
        return std::nullopt;
    }
    const std::filesystem::path path = PathFromSdlUtf8(raw);
    if (path.empty()) {
        return std::nullopt;
    }
    return path;
}

// SDL_GetPrefPath returns char* that the caller must free.
inline std::optional<std::filesystem::path> PathFromSdlPrefPath(const char* org, const char* app) {
    char* raw = SDL_GetPrefPath(org, app);
    if (!raw || raw[0] == '\0') {
        if (raw) {
            SDL_free(raw);
        }
        return std::nullopt;
    }
    const std::filesystem::path path = PathFromSdlUtf8(raw);
    SDL_free(raw);
    if (path.empty()) {
        return std::nullopt;
    }
    return path;
}

} // namespace Game

#endif
