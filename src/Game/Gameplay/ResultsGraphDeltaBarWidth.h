#ifndef CC_GAME_RESULTS_GRAPH_DELTA_BAR_WIDTH_H
#define CC_GAME_RESULTS_GRAPH_DELTA_BAR_WIDTH_H

#include <algorithm>
#include <limits>
#include <span>
#include <vector>

#include "GameplayConstants.h"

namespace Game::Gameplay {

[[nodiscard]] constexpr double DomainSpanForBarWidth(const double firstSec, const double lastSec) noexcept {
    const double span = lastSec - firstSec;
    return span > 1e-12 ? span : 1e-12;
}

[[nodiscard]] inline float ConcurrentSlotEpsilonNx(
    const double chartFirstSec,
    const double chartLastSec) noexcept {
    const double span = DomainSpanForBarWidth(chartFirstSec, chartLastSec);
    return static_cast<float>(kResultsGraphDeltaBarConcurrentEpsSec / span);
}

// Uniform bar width, at most baseBarW, shrunk by min pixel gap between distinct nx slots
// (concurrent notes within epsNx do not reduce the cap), then capped by maxBarFractionOfPlot
// of plotW, then floored to minWidthPx.
[[nodiscard]] inline float ResultsGraphDeltaBarWidthPx(
    const float plotW,
    const float baseBarW,
    const float maxBarFractionOfPlot,
    const float minWidthPx,
    const float epsNx,
    const std::span<const float> nxValues) {
    float overlapCap = baseBarW;
    if (nxValues.size() >= 2) {
        std::vector sorted(nxValues.begin(), nxValues.end());
        std::ranges::sort(sorted);

        float minGapPx = std::numeric_limits<float>::infinity();
        float refNx = sorted.front();
        for (size_t i = 1; i < sorted.size(); ++i) {
            if (const float nx = sorted[i]; nx - refNx > epsNx) {
                const float gapPx = (nx - refNx) * plotW;
                minGapPx = std::min(minGapPx, gapPx);
                refNx = nx;
            }
        }
        if (std::isfinite(minGapPx)) {
            overlapCap = std::min(baseBarW, minGapPx);
        }
    }

    float w = overlapCap;
    w = std::min(w, plotW * maxBarFractionOfPlot);
    w = std::max(minWidthPx, w);
    return w;
}

} // namespace Game::Gameplay

#endif // CC_GAME_RESULTS_GRAPH_DELTA_BAR_WIDTH_H
