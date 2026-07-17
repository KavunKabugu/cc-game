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