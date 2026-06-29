#include "Profile.h"

#ifdef CC_ENABLE_PROFILING

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <array>
#include <cstdint>

namespace Game {

namespace {

constexpr std::size_t kMaxScopes = 16;
constexpr Uint64 kPrintIntervalNs = 1'000'000'000ULL;

struct ScopeStats {
    const char* name = nullptr;
    Uint64 totalNs = 0;
    Uint64 maxNs = 0;
    std::uint64_t callCount = 0;
};

std::array<ScopeStats, kMaxScopes> g_scopes{};
std::size_t g_scopeCount = 0;
bool g_tableFullWarned = false;

Uint64 g_lastFrameEndNs = 0;
Uint64 g_frameTotalNs = 0;
Uint64 g_frameMaxNs = 0;
std::uint64_t g_frameCount = 0;

Uint64 g_lastPrintNs = 0;

[[nodiscard]] ScopeStats* FindOrCreateScope(const char* name) {
    for (std::size_t i = 0; i < g_scopeCount; ++i) {
        if (g_scopes[i].name == name) {
            return &g_scopes[i];
        }
    }

    if (g_scopeCount >= kMaxScopes) {
        if (!g_tableFullWarned) {
            SDL_Log("[Profile] Scope table full (%zu entries); dropping new scopes.", kMaxScopes);
            g_tableFullWarned = true;
        }
        return nullptr;
    }

    g_scopes[g_scopeCount].name = name;
    ++g_scopeCount;
    return &g_scopes[g_scopeCount - 1];
}

void ResetWindowStats() {
    for (std::size_t i = 0; i < g_scopeCount; ++i) {
        g_scopes[i].totalNs = 0;
        g_scopes[i].maxNs = 0;
        g_scopes[i].callCount = 0;
    }
    g_frameTotalNs = 0;
    g_frameMaxNs = 0;
    g_frameCount = 0;
}

[[nodiscard]] double NsToMs(const Uint64 ns) {
    return static_cast<double>(ns) / 1'000'000.0;
}

} // namespace

ProfileScope::ProfileScope(const char* scopeName) : name(scopeName), startNs(SDL_GetTicksNS()) {}

ProfileScope::~ProfileScope() {
    const Uint64 elapsed = SDL_GetTicksNS() - startNs;
    ProfileRecord(name, elapsed);
}

void ProfileRecord(const char* name, const Uint64 elapsedNs) {
    ScopeStats* stats = FindOrCreateScope(name);
    if (!stats) {
        return;
    }
    stats->totalNs += elapsedNs;
    stats->maxNs = std::max(stats->maxNs, elapsedNs);
    ++stats->callCount;
}

void ProfileEndFrame() {
    const Uint64 nowNs = SDL_GetTicksNS();
    if (g_lastFrameEndNs != 0) {
        const Uint64 frameNs = nowNs - g_lastFrameEndNs;
        g_frameTotalNs += frameNs;
        g_frameMaxNs = std::max(g_frameMaxNs, frameNs);
        ++g_frameCount;
    }
    g_lastFrameEndNs = nowNs;

    if (g_lastPrintNs == 0) {
        g_lastPrintNs = nowNs;
    }
}

void ProfilePrintIfDue() {
    const Uint64 nowNs = SDL_GetTicksNS();
    if (nowNs - g_lastPrintNs < kPrintIntervalNs) {
        return;
    }

    SDL_Log("[Profile] --- 1 s summary ---");

    struct SortEntry {
        const ScopeStats* stats;
    };

    std::array<SortEntry, kMaxScopes + 1> sorted{};
    std::size_t sortedCount = 0;

    for (std::size_t i = 0; i < g_scopeCount; ++i) {
        if (g_scopes[i].callCount == 0) {
            continue;
        }
        sorted[sortedCount++] = SortEntry{&g_scopes[i]};
    }

    if (g_frameCount > 0) {
        ScopeStats frameStats{};
        frameStats.name = "Frame";
        frameStats.totalNs = g_frameTotalNs;
        frameStats.maxNs = g_frameMaxNs;
        frameStats.callCount = g_frameCount;
        static ScopeStats frameStorage;
        frameStorage = frameStats;
        sorted[sortedCount++] = SortEntry{&frameStorage};
    }

    std::ranges::sort(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(sortedCount),
                      [](const SortEntry& a, const SortEntry& b) {
                          return a.stats->totalNs > b.stats->totalNs;
                      });

    for (std::size_t i = 0; i < sortedCount; ++i) {
        const ScopeStats& s = *sorted[i].stats;
        const double avgMs = NsToMs(s.totalNs / s.callCount);
        const double maxMs = NsToMs(s.maxNs);
        SDL_Log("[Profile] %-28s avg=%6.2fms  max=%6.2fms  calls=%llu",
                s.name,
                avgMs,
                maxMs,
                static_cast<unsigned long long>(s.callCount));
    }

    if (sortedCount == 0) {
        SDL_Log("[Profile] (no samples)");
    }

    ResetWindowStats();
    g_lastPrintNs = nowNs;
}

} // namespace Game

#endif // CC_ENABLE_PROFILING
