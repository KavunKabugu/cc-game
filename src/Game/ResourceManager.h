#ifndef CC_GAME_RESOURCEMANAGER_H
#define CC_GAME_RESOURCEMANAGER_H

#include <string>
#include <expected>
#include <memory>
#include <vector>
#include <filesystem>
#include <unordered_map>

#include "SDL3/SDL_render.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "AudioBuffer.h"

enum class ResourceError {
    FileNotFound,
    InvalidFormat,
    RendererNotInitialized,
    SDLError,
    UnknownType,
    TTFNotInitialized,
    AudioNotLoaded,
    MixerNotInitialized
};

class ResourceManager {
public:
    static ResourceManager& getInstance() {
        static ResourceManager instance;
        return instance;
    }
    void Init(const SDL_Renderer *r);
    void Shutdown();

    void AddSearchPath(const std::string& path);
    std::expected<std::filesystem::path, ResourceError> ResolveCanonicalPath(std::string_view rawPath) const;
    [[nodiscard]] MIX_Mixer* GetMixer() const { return mixer; }

    // Multiplier for TTF font size (sizes passed to Get<TTF_Font> are in reference logical pixels).
    void SetUiFontScale(float scale);
    [[nodiscard]] float GetUiFontScale() const { return uiFontScale; }

    template<typename T>
    std::expected<std::shared_ptr<T>, ResourceError> Get(const std::string_view &path, const float fontSize = 24.0f) {
        if constexpr (std::is_same_v<T, SDL_Texture>) {
            return GetTexture(path);
        } else if constexpr (std::is_same_v<T, TTF_Font>) {
            return GetFont(path, fontSize);
        } else if constexpr (std::is_same_v<T, Game::AudioBuffer>) {
            return GetAudioBuffer(path);
        } else if constexpr (std::is_same_v<T, MIX_Audio>) {
            return GetAudio(path);
        }
        return std::unexpected(ResourceError::UnknownType);
    }

private:
    std::expected<std::shared_ptr<SDL_Texture>, ResourceError> GetTexture(const std::string_view &path);
    std::expected<std::shared_ptr<TTF_Font>, ResourceError> GetFont(const std::string_view &path, float fontSize);
    std::expected<std::shared_ptr<Game::AudioBuffer>, ResourceError> GetAudioBuffer(const std::string_view &path);
    std::expected<std::shared_ptr<MIX_Audio>, ResourceError> GetAudio(const std::string_view &path);

    const SDL_Renderer *renderer = nullptr;
    MIX_Mixer *mixer = nullptr;
    std::filesystem::path basePath;
    std::vector<std::filesystem::path> searchPaths;
    
    std::unordered_map<std::filesystem::path, std::weak_ptr<SDL_Texture>> textures;
    std::unordered_map<std::string, std::weak_ptr<TTF_Font>> fonts; // Key path:designSize@effectiveSize
    std::unordered_map<std::filesystem::path, std::weak_ptr<Game::AudioBuffer>> audioBuffers;
    std::unordered_map<std::filesystem::path, std::weak_ptr<MIX_Audio>> audio;

    float uiFontScale = 1.0f;
};

#endif //CC_GAME_RESOURCEMANAGER_H
