#include "ResourceManager.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include "Game/PathUtf8.h"
#include "SDL3/SDL_log.h"
#include "SDL3_image/SDL_image.h"

using Game::PathFromSdlBasePath;
using Game::PathToUtf8String;
using Game::Utf8StringToPath;

void ResourceManager::Init(const SDL_Renderer *r) {
    this->renderer = r;
    this->textures.clear();
    this->searchPaths.clear();
    if (const auto base = PathFromSdlBasePath()) {
        this->basePath = *base;
    }
    
    if (!TTF_Init()) {
        SDL_Log("Failed to initialize SDL_ttf: %s", SDL_GetError());
    }

    if (!MIX_Init()) {
        SDL_Log("Failed to initialize SDL_mixer: %s", SDL_GetError());
    }

    this->mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!this->mixer) {
        SDL_Log("Failed to create mixer device: %s", SDL_GetError());
    }
}

void ResourceManager::Shutdown() {
    this->textures.clear();
    this->fonts.clear();
    this->audioBuffers.clear();
    this->audio.clear();
    this->searchPaths.clear();
    this->basePath.clear();
    this->uiFontScale = 1.0f;
    
    if (this->mixer) {
        MIX_DestroyMixer(this->mixer);
        this->mixer = nullptr;
    }
    MIX_Quit();
    TTF_Quit();
}

std::expected<std::shared_ptr<MIX_Audio>, ResourceError> ResourceManager::GetAudio(const std::string_view &path) {
    auto canonicalRes = ResolveCanonicalPath(path);
    if (!canonicalRes) {
        return std::unexpected(canonicalRes.error());
    }
    const auto& canonicalPath = *canonicalRes;

    if (const auto it = audio.find(canonicalPath); it != audio.end()) {
        if (auto shared = it->second.lock()) {
            return shared;
        }
        audio.erase(it);
    }

    if (!mixer) {
        return std::unexpected(ResourceError::MixerNotInitialized);
    }

    MIX_Audio* rawAudio = MIX_LoadAudio(mixer, PathToUtf8String(canonicalPath).c_str(), true);
    if (!rawAudio) {
        SDL_Log("Failed to load audio (%s): %s", PathToUtf8String(canonicalPath).c_str(), SDL_GetError());
        return std::unexpected(ResourceError::SDLError);
    }

    auto shared = std::shared_ptr<MIX_Audio>(rawAudio, [](MIX_Audio* a) {
        MIX_DestroyAudio(a);
    });

    audio[canonicalPath] = shared;
    return shared;
}

void ResourceManager::AddSearchPath(const std::string& path) {
    std::filesystem::path searchPath = Utf8StringToPath(path);
    if (searchPath.is_relative() && !this->basePath.empty()) {
        searchPath = this->basePath / searchPath;
    }
    searchPath = searchPath.lexically_normal();
    for (const auto& existing : this->searchPaths) {
        if (existing == searchPath) {
            return;
        }
    }
    this->searchPaths.push_back(std::move(searchPath));
}

std::expected<std::filesystem::path, ResourceError> ResourceManager::ResolveCanonicalPath(const std::string_view rawPath) const {
    const std::filesystem::path input = Utf8StringToPath(std::string(rawPath));
    if (input.is_absolute()) {
        if (std::error_code e; std::filesystem::exists(input, e) && !e) {
            return std::filesystem::canonical(input, e);
        }
    }

    std::vector<std::filesystem::path> candidates;
    candidates.reserve(1 + this->searchPaths.size());
    if (!this->basePath.empty()) {
        candidates.push_back(this->basePath / input);
    }
    for (const auto& searchRoot : this->searchPaths) {
        candidates.push_back(searchRoot / input);
    }

    std::error_code e;
    for (const auto& candidate : candidates) {
        e.clear();
        if (!std::filesystem::exists(candidate, e) || e) {
            continue;
        }
        e.clear();
        std::filesystem::path canonical = std::filesystem::canonical(candidate, e);
        if (!e) {
            return canonical;
        }
    }
    return std::unexpected(ResourceError::FileNotFound);
}

std::expected<std::shared_ptr<SDL_Texture>, ResourceError> ResourceManager::GetTexture(const std::string_view &path) {
    auto canonicalRes = ResolveCanonicalPath(path);
    if (!canonicalRes) {
        return std::unexpected(canonicalRes.error());
    }
    const auto& canonicalPath = *canonicalRes;

    if (const auto it = textures.find(canonicalPath); it != textures.end()) {
        if (auto shared = it->second.lock()) {
            return shared;
        }
        textures.erase(it);
    }

    if (!renderer) {
        return std::unexpected(ResourceError::RendererNotInitialized);
    }

    SDL_Texture* rawTexture = IMG_LoadTexture(const_cast<SDL_Renderer*>(renderer), PathToUtf8String(canonicalPath).c_str());
    if (!rawTexture) {
        SDL_Log("Failed to load image (%s): %s", PathToUtf8String(canonicalPath).c_str(), SDL_GetError());
        return std::unexpected(ResourceError::SDLError);
    }

    auto shared = std::shared_ptr<SDL_Texture>(rawTexture, [](SDL_Texture* t) {
        SDL_DestroyTexture(t);
    });

    textures[canonicalPath] = shared;
    return shared;
}

void ResourceManager::SetUiFontScale(const float scale) {
    const float clamped = std::clamp(scale, 0.01f, 100.0f);
    if (clamped == uiFontScale) {
        return;
    }
    uiFontScale = clamped;
    fonts.clear();
}

std::expected<std::shared_ptr<TTF_Font>, ResourceError> ResourceManager::GetFont(const std::string_view &path, const float fontSize) {
    auto canonicalRes = ResolveCanonicalPath(path);
    if (!canonicalRes) {
        return std::unexpected(canonicalRes.error());
    }
    const std::string canonicalPath = PathToUtf8String(*canonicalRes);
    const float effectiveSize = std::max(fontSize * uiFontScale, 1.0f);
    const std::string key =
        canonicalPath + ":" + std::to_string(std::lround(fontSize * 1000)) +
        "@" + std::to_string(std::lround(effectiveSize * 1000));

    if (const auto it = fonts.find(key); it != fonts.end()) {
        if (auto shared = it->second.lock()) {
            return shared;
        }
        fonts.erase(it);
    }

    TTF_Font* rawFont = TTF_OpenFont(canonicalPath.c_str(), effectiveSize);
    if (!rawFont) {
        SDL_Log("Failed to load font (%s): %s", canonicalPath.c_str(), SDL_GetError());
        return std::unexpected(ResourceError::SDLError);
    }

    auto shared = std::shared_ptr<TTF_Font>(rawFont, [](TTF_Font* f) {
        TTF_CloseFont(f);
    });

    fonts[key] = shared;
    return shared;
}

std::expected<std::shared_ptr<Game::AudioBuffer>, ResourceError> ResourceManager::GetAudioBuffer(const std::string_view &path) {
    auto canonicalRes = ResolveCanonicalPath(path);
    if (!canonicalRes) {
        return std::unexpected(canonicalRes.error());
    }
    const auto& canonicalPath = *canonicalRes;

    if (const auto it = audioBuffers.find(canonicalPath); it != audioBuffers.end()) {
        if (auto shared = it->second.lock()) {
            return shared;
        }
        audioBuffers.erase(it);
    }

    SDL_AudioSpec spec;
    Uint8* audioBuf = nullptr;
    Uint32 audioLen = 0;

    if (!SDL_LoadWAV(PathToUtf8String(canonicalPath).c_str(), &spec, &audioBuf, &audioLen)) {
        SDL_Log("Failed to load audio (%s): %s", PathToUtf8String(canonicalPath).c_str(), SDL_GetError());
        return std::unexpected(ResourceError::SDLError);
    }

    auto buffer = std::make_shared<Game::AudioBuffer>();
    buffer->spec = spec;
    buffer->data.resize(audioLen);
    std::memcpy(buffer->data.data(), audioBuf, audioLen);

    SDL_free(audioBuf);

    audioBuffers[canonicalPath] = buffer;
    return buffer;
}
