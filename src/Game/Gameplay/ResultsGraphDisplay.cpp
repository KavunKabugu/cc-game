#include "ResultsGraphDisplay.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <SDL3_ttf/SDL_ttf.h>

#include "GameplayConstants.h"
#include "JudgementDisplayColors.h"
#include "ResultsGraphDeltaBarWidth.h"

namespace Game::Gameplay {

namespace {

[[nodiscard]] double DomainSpan(const double firstSec, const double lastSec) {
    const double span = lastSec - firstSec;
    return span > 1e-12 ? span : 1e-12;
}

[[nodiscard]] double NormalizedFromTime(const double tSec, const double firstSec, const double lastSec) {
    const double span = DomainSpan(firstSec, lastSec);
    const double u = (tSec - firstSec) / span;
    return std::clamp(u, 0.0, 1.0);
}

[[nodiscard]] double NormalizedPlotX(
    const ResultsGraphEvent& e,
    const double chartFirstSec,
    const double chartLastSec) {
    using enum MissReason;
    if (e.missReason == EmptyLane) {
        const double p = e.pressTimeSeconds;
        if (p < chartFirstSec) {
            return 0.0;
        }
        if (p > chartLastSec) {
            return 1.0;
        }
        return NormalizedFromTime(p, chartFirstSec, chartLastSec);
    }
    if (e.noteTargetTimeSeconds.has_value()) {
        const double t = std::clamp(*e.noteTargetTimeSeconds, chartFirstSec, chartLastSec);
        return NormalizedFromTime(t, chartFirstSec, chartLastSec);
    }
    return 0.5;
}

[[nodiscard]] double DisplayDeltaMs(const ResultsGraphEvent& e) {
    using enum Judgement;
    using enum MissReason;
    if (e.judgement == Miss && e.missReason == EmptyLane) {
        return 0.0;
    }
    if (e.judgement == Miss && e.missReason == NoteExpired) {
        return kMissExpireMs;
    }
    return std::clamp(e.deltaMs, -kMissExpireMs, kMissExpireMs);
}

[[nodiscard]] bool NeedsVerticalCue(const ResultsGraphEvent& e) {
    using enum MissReason;
    return e.missReason == EmptyLane || e.missReason == NoteExpired;
}

[[nodiscard]] SDL_Color BarColor(const ResultsGraphEvent& e) {
    using enum Judgement;
    const double absForGradient =
        e.judgement == Perfect || e.judgement == Great || e.judgement == Good || e.judgement == Bad
            ? std::abs(std::clamp(e.deltaMs, -kMissExpireMs, kMissExpireMs))
            : 0.0;
    if (e.judgement == Perfect || e.judgement == Great || e.judgement == Good || e.judgement == Bad) {
        return ResultsJudgementFillColor(e.judgement, absForGradient);
    }
    return ResultsJudgementFillColor(Miss, 0.0);
}

[[nodiscard]] float YFromDeltaMs(
    const float centerY,
    const float halfH,
    const double deltaMs) {
    return centerY - static_cast<float>(deltaMs / kMissExpireMs) * halfH;
}

void DrawHorizontalGridLine(
    SDL_Renderer* renderer,
    const SDL_FRect& plot,
    const float centerY,
    const float lineH,
    const Uint8 r,
    const Uint8 g,
    const Uint8 b,
    const Uint8 a) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    const SDL_FRect line = {
        .x = plot.x,
        .y = centerY - lineH * 0.5f,
        .w = plot.w,
        .h = lineH,
    };
    SDL_RenderFillRect(renderer, &line);
}

[[nodiscard]] float GridLineHeightPx(const float plotH) {
    return std::max(2.0f, plotH * 0.008f);
}

void DrawTextRightOf(
    SDL_Renderer* renderer,
    const std::shared_ptr<TTF_Font>& font,
    const std::string& text,
    const float rightEdge,
    const float centerY,
    const SDL_Color color) {
    if (!font || text.empty()) {
        return;
    }
    SDL_Surface* surface = TTF_RenderText_Blended(font.get(), text.c_str(), 0, color);
    if (!surface) {
        return;
    }
    const auto tw = static_cast<float>(surface->w);
    const auto th = static_cast<float>(surface->h);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        return;
    }
    const SDL_FRect dst = {.x = rightEdge - tw, .y = centerY - th * 0.5f, .w = tw, .h = th};
    SDL_RenderTexture(renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

ResultsGraphDisplay::CachedText CreateTextTexture(
    SDL_Renderer* renderer,
    const std::shared_ptr<TTF_Font>& font,
    const std::string& text,
    const SDL_Color color) {
    if (!font || text.empty()) {
        return {};
    }
    SDL_Surface* surface = TTF_RenderText_Blended(font.get(), text.c_str(), 0, color);
    if (!surface) {
        return {};
    }
    const int tw = surface->w;
    const int th = surface->h;
    SDL_Texture* rawTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!rawTexture) {
        return {};
    }
    const auto shared = std::shared_ptr<SDL_Texture>(rawTexture, [](SDL_Texture* t) {
        SDL_DestroyTexture(t);
    });
    return {.texture = shared, .w = tw, .h = th};
}

void DrawCachedTextRightOf(
    SDL_Renderer* renderer,
    const ResultsGraphDisplay::CachedText& cached,
    const float rightEdge,
    const float centerY) {
    if (!cached.texture) {
        return;
    }
    const auto tw = static_cast<float>(cached.w);
    const auto th = static_cast<float>(cached.h);
    const SDL_FRect dst = {.x = rightEdge - tw, .y = centerY - th * 0.5f, .w = tw, .h = th};
    SDL_RenderTexture(renderer, cached.texture.get(), nullptr, &dst);
}

[[nodiscard]] double NormalizedPlotXFromHit(
    const HitResult& result,
    const double chartFirstSec,
    const double chartLastSec) {
    const ResultsGraphEvent e{
        .judgement = result.judgement,
        .missReason = result.missReason,
        .deltaMs = result.deltaMs,
        .noteTargetTimeSeconds = result.noteTargetTimeSeconds,
        .pressTimeSeconds = result.pressTimeSeconds,
    };
    return NormalizedPlotX(e, chartFirstSec, chartLastSec);
}

} // namespace

double NormalizedPlotXForHit(
    const HitResult& result,
    const double chartFirstNoteTimeSeconds,
    const double chartLastNoteTimeSeconds) {
    return NormalizedPlotXFromHit(result, chartFirstNoteTimeSeconds, chartLastNoteTimeSeconds);
}

ResultsGraphDisplay::ResultsGraphDisplay(const UnitBounds bounds) : Drawable(bounds) {}

ResultsGraphDisplay::~ResultsGraphDisplay() {
    FreeTexture();
}

void ResultsGraphDisplay::FreeTexture() {
    if (cachedGraphTexture) {
        SDL_DestroyTexture(cachedGraphTexture);
        cachedGraphTexture = nullptr;
    }
}

void ResultsGraphDisplay::SetMode(const ResultsGraphMode m) {
    if (mode != m) {
        mode = m;
        textureDirty = true;
    }
}

void ResultsGraphDisplay::SetChartDomain(const double firstNoteTimeSeconds, const double lastNoteTimeSeconds) {
    if (chartFirstSec != firstNoteTimeSeconds || chartLastSec != lastNoteTimeSeconds) {
        chartFirstSec = firstNoteTimeSeconds;
        chartLastSec = lastNoteTimeSeconds;
        lastPlotW = 0.0f; // Force bar width recalculation
        textureDirty = true;
    }
}

void ResultsGraphDisplay::SetEvents(std::vector<ResultsGraphEvent> e) {
    events = std::move(e);
    lastPlotW = 0.0f; // Force bar width recalculation
    textureDirty = true;
}

void ResultsGraphDisplay::SetAccuracySteps(std::vector<std::pair<double, double>> steps) {
    accuracySteps = std::move(steps);
    textureDirty = true;
}

void ResultsGraphDisplay::SetLabelFont(std::shared_ptr<TTF_Font> font) {
    if (labelFont != font) {
        labelFont = std::move(font);
        labelsDirty = true;
        ClearCachedLabels();
        textureDirty = true;
    }
}

void ResultsGraphDisplay::ClearCachedLabels() {
    cachedAccuracyLabels.clear();
    cachedDeltaLabels.clear();
}

void ResultsGraphDisplay::RebuildCachedLabels(SDL_Renderer* renderer) {
    ClearCachedLabels();
    if (!labelFont) {
        labelsDirty = false;
        return;
    }

    constexpr SDL_Color labelColor{.r = 210, .g = 220, .b = 235, .a = 255};

    // Accuracy mode labels: 100, 90, ..., 0
    cachedAccuracyLabels.push_back(CreateTextTexture(renderer, labelFont, "100", labelColor));
    for (int step = 10; step <= 90; step += 10) {
        cachedAccuracyLabels.push_back(CreateTextTexture(renderer, labelFont, std::format("{}", step), labelColor));
    }
    cachedAccuracyLabels.push_back(CreateTextTexture(renderer, labelFont, "0", labelColor));

    // Delta mode labels: +/-Perfect, +/-Great, +/-Good, 0ms, +/-Miss
    cachedDeltaLabels.push_back(CreateTextTexture(renderer, labelFont, std::format("{:+.0f}", kPerfectWindowMs), labelColor));
    cachedDeltaLabels.push_back(CreateTextTexture(renderer, labelFont, std::format("{:+.0f}", -kPerfectWindowMs), labelColor));
    cachedDeltaLabels.push_back(CreateTextTexture(renderer, labelFont, std::format("{:+.0f}", kGreatWindowMs), labelColor));
    cachedDeltaLabels.push_back(CreateTextTexture(renderer, labelFont, std::format("{:+.0f}", -kGreatWindowMs), labelColor));
    cachedDeltaLabels.push_back(CreateTextTexture(renderer, labelFont, std::format("{:+.0f}", kGoodWindowMs), labelColor));
    cachedDeltaLabels.push_back(CreateTextTexture(renderer, labelFont, std::format("{:+.0f}", -kGoodWindowMs), labelColor));
    cachedDeltaLabels.push_back(CreateTextTexture(renderer, labelFont, "0ms", labelColor));
    cachedDeltaLabels.push_back(CreateTextTexture(renderer, labelFont, std::format("{:+.0f}", kMissExpireMs), labelColor));
    cachedDeltaLabels.push_back(CreateTextTexture(renderer, labelFont, std::format("{:+.0f}", -kMissExpireMs), labelColor));

    labelsDirty = false;
}

void ResultsGraphDisplay::RebuildTexture(SDL_Renderer* renderer, const float w, const float h) {
    FreeTexture();

    cachedGraphTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        static_cast<int>(w),
        static_cast<int>(h)
    );
    if (!cachedGraphTexture) {
        return;
    }

    SDL_SetTextureBlendMode(cachedGraphTexture, SDL_BLENDMODE_BLEND);

    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, cachedGraphTexture);

    // Clear texture with transparent background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    const SDL_FRect slotRect = {
        .x = 0.0f,
        .y = 0.0f,
        .w = w,
        .h = h,
    };

    const float pad = std::max(6.0f, slotRect.w * 0.02f);
    const bool axisLabels = labelFont != nullptr;
    const float leftAxisPad = axisLabels ? 34.0f : 0.0f;
    const float marginTop =
        axisLabels && mode == ResultsGraphMode::Delta ? 12.0f : 0.0f;
    const float marginBottom =
        axisLabels && mode == ResultsGraphMode::Delta ? 12.0f : 0.0f;

    const SDL_FRect plot = {
        .x = slotRect.x + pad + leftAxisPad,
        .y = slotRect.y + pad + marginTop,
        .w = std::max(1.0f, slotRect.w - 2.0f * pad - leftAxisPad),
        .h = std::max(1.0f, slotRect.h - 2.0f * pad - marginTop - marginBottom),
    };

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, 30, 40, 60, 220);
    SDL_RenderFillRect(renderer, &slotRect);

    if (mode == ResultsGraphMode::Accuracy) {
        std::vector<std::pair<double, double>> steps = accuracySteps;
        std::ranges::sort(steps, [](const auto& a, const auto& b) { return a.first < b.first; });

        const float bottom = plot.y + plot.h;

        auto segmentAcc = [&](const double x0n, const double x1n, const double accPct) {
            const float x0 = plot.x + static_cast<float>(x0n) * plot.w;
            const float x1 = plot.x + static_cast<float>(x1n) * plot.w;
            const float hi = static_cast<float>(std::clamp(accPct, 0.0, 100.0)) * 0.01f * plot.h;
            const SDL_FRect band = {
                .x = std::min(x0, x1),
                .y = bottom - hi,
                .w = std::max(1.0f, std::abs(x1 - x0)),
                .h = std::max(1.0f, hi),
            };
            const auto [r, g, b, a] = ResultsAccuracyGraphFillColor(accPct);
            SDL_SetRenderDrawColor(renderer, r, g, b, a);
            SDL_RenderFillRect(renderer, &band);
        };

        if (steps.empty()) {
            segmentAcc(0.0, 1.0, 100.0);
        } else {
            double carry = 100.0;
            double prevX = 0.0;
            for (auto & [fst, snd] : steps) {
                const double x = std::clamp(fst, 0.0, 1.0);
                const double acc = std::clamp(snd, 0.0, 100.0);
                segmentAcc(prevX, x, carry);
                carry = acc;
                prevX = x;
            }
            segmentAcc(prevX, 1.0, carry);
        }

        const float gridLineH = GridLineHeightPx(plot.h);

        for (int step = 10; step <= 90; step += 10) {
            const float t = static_cast<float>(step) * 0.01f;
            const float yLine = plot.y + plot.h - t * plot.h;
            DrawHorizontalGridLine(renderer, plot, yLine, gridLineH, 175, 190, 210, 72);
        }

        SDL_SetRenderDrawColor(renderer, 200, 215, 235, 140);
        SDL_RenderRect(renderer, &plot);

        if (labelFont && cachedAccuracyLabels.size() == 11) {
            const float axisRight = plot.x - 5.0f;
            DrawCachedTextRightOf(renderer, cachedAccuracyLabels[0], axisRight, plot.y);
            for (int step = 10; step <= 90; step += 10) {
                const float t = static_cast<float>(step) * 0.01f;
                const float yLine = plot.y + plot.h - t * plot.h;
                auto idx = static_cast<size_t>(step / 10);
                DrawCachedTextRightOf(renderer, cachedAccuracyLabels[idx], axisRight, yLine);
            }
            DrawCachedTextRightOf(renderer, cachedAccuracyLabels[10], axisRight, plot.y + plot.h);
        }
    } else {
        const float centerY = plot.y + plot.h * 0.5f;
        const float halfH = plot.h * 0.5f;

        struct PlotItem {
            float nx = 0.0f;
            double sortAbs = 0.0;
            size_t index = 0;
        };

        std::map<std::int64_t, std::vector<PlotItem>> groups;
        std::vector<float> nxForWidth;
        nxForWidth.reserve(events.size());
        for (size_t i = 0; i < events.size(); ++i) {
            const double nx = NormalizedPlotX(events[i], chartFirstSec, chartLastSec);
            nxForWidth.push_back(static_cast<float>(nx));
            const auto key = std::llround(nx * 1.0e6);
            PlotItem item{};
            item.nx = static_cast<float>(nx);
            item.sortAbs = std::abs(DisplayDeltaMs(events[i]));
            item.index = i;
            groups[key].push_back(item);
        }

        for (auto& [k, vec] : groups) {
            (void)k;
            std::ranges::sort(vec, [](const PlotItem& a, const PlotItem& b) {
                return a.sortAbs > b.sortAbs;
            });
        }

        if (plot.w != lastPlotW) {
            const float baseBarW = std::clamp(plot.w * 0.003f, 0.75f, 5.0f);
            const float epsNx = ConcurrentSlotEpsilonNx(chartFirstSec, chartLastSec);
            cachedBarWidth = ResultsGraphDeltaBarWidthPx(
                plot.w,
                baseBarW,
                0.01f,
                kResultsGraphDeltaBarMinWidthPx,
                epsNx,
                nxForWidth);
            lastPlotW = plot.w;
        }
        const float w_bar = cachedBarWidth;

        for (auto& [k, vec] : groups) {
            (void)k;

            for (const auto & item : vec) {
                const ResultsGraphEvent& e = events[item.index];
                const float cx = plot.x + item.nx * plot.w;
                const float x0 = cx - w_bar * 0.5f;

                const double dDisp = DisplayDeltaMs(e);
                const float dy = static_cast<float>(dDisp / kMissExpireMs) * halfH;
                const float yEnd = centerY - dy;

                const auto [r, g, b, a] = BarColor(e);
                SDL_SetRenderDrawColor(renderer, r, g, b, a);

                const float top = std::min(centerY, yEnd);
                const float barH = std::max(1.0f, std::abs(yEnd - centerY));
                const SDL_FRect bar = {.x = x0, .y = top, .w = w_bar, .h = barH};
                SDL_RenderFillRect(renderer, &bar);

                using enum MissReason;
                if (e.missReason == EmptyLane) {
                    const float dot = std::max(3.0f, w_bar * 0.5f);
                    const SDL_FRect dotRect = {.x = cx - dot * 0.5f, .y = centerY - dot * 0.5f, .w = dot, .h = dot};
                    SDL_RenderFillRect(renderer, &dotRect);
                }
            }
        }

        const float gridLineH = GridLineHeightPx(plot.h);

        auto drawHorizontalGrid = [&](const double deltaMs, const Uint8 alpha) {
            const float y = YFromDeltaMs(centerY, halfH, deltaMs);
            DrawHorizontalGridLine(renderer, plot, y, gridLineH, 175, 190, 210, alpha);
        };

        drawHorizontalGrid(kPerfectWindowMs, 85);
        drawHorizontalGrid(-kPerfectWindowMs, 85);
        drawHorizontalGrid(kGreatWindowMs, 90);
        drawHorizontalGrid(-kGreatWindowMs, 90);
        drawHorizontalGrid(kGoodWindowMs, 95);
        drawHorizontalGrid(-kGoodWindowMs, 95);
        drawHorizontalGrid(kMissExpireMs, 80);
        drawHorizontalGrid(-kMissExpireMs, 80);

        DrawHorizontalGridLine(renderer, plot, centerY, gridLineH, 200, 215, 235, 140);

        const auto cueA = static_cast<Uint8>(std::round(255.0f * kResultsGraphCueLineAlpha));
        SDL_SetRenderDrawColor(renderer, 255, 60, 60, cueA);

        for (const auto & event : events) {
            if (!NeedsVerticalCue(event)) {
                continue;
            }
            const auto nx = static_cast<float>(NormalizedPlotX(event, chartFirstSec, chartLastSec));
            const float vx = plot.x + nx * plot.w;
            const SDL_FRect cue = {.x = vx - 1.0f, .y = plot.y, .w = 3.0f, .h = plot.h};
            SDL_RenderFillRect(renderer, &cue);
        }

        SDL_SetRenderDrawColor(renderer, 200, 215, 235, 140);
        SDL_RenderRect(renderer, &plot);

        if (labelFont && cachedDeltaLabels.size() == 9) {
            const float axisRight = plot.x - 5.0f;
            DrawCachedTextRightOf(renderer, cachedDeltaLabels[0], axisRight, YFromDeltaMs(centerY, halfH, kPerfectWindowMs));
            DrawCachedTextRightOf(renderer, cachedDeltaLabels[1], axisRight, YFromDeltaMs(centerY, halfH, -kPerfectWindowMs));
            DrawCachedTextRightOf(renderer, cachedDeltaLabels[2], axisRight, YFromDeltaMs(centerY, halfH, kGreatWindowMs));
            DrawCachedTextRightOf(renderer, cachedDeltaLabels[3], axisRight, YFromDeltaMs(centerY, halfH, -kGreatWindowMs));
            DrawCachedTextRightOf(renderer, cachedDeltaLabels[4], axisRight, YFromDeltaMs(centerY, halfH, kGoodWindowMs));
            DrawCachedTextRightOf(renderer, cachedDeltaLabels[5], axisRight, YFromDeltaMs(centerY, halfH, -kGoodWindowMs));
            DrawCachedTextRightOf(renderer, cachedDeltaLabels[6], axisRight, centerY);

            const float topStripCenterY = slotRect.y + pad + marginTop * 0.5f;
            const float botStripCenterY = plot.y + plot.h + marginBottom * 0.5f;
            DrawCachedTextRightOf(renderer, cachedDeltaLabels[7], axisRight, topStripCenterY);
            DrawCachedTextRightOf(renderer, cachedDeltaLabels[8], axisRight, botStripCenterY);
        }
    }

    SDL_SetRenderTarget(renderer, oldTarget);

    lastCachedW = w;
    lastCachedH = h;
    lastCachedMode = mode;
    textureDirty = false;
}

void ResultsGraphDisplay::Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) {
    if (labelsDirty) {
        RebuildCachedLabels(renderer);
    }

    const SDL_FRect slotRect = {
        .x = parentRect.x + bounds.min.x * parentRect.w,
        .y = parentRect.y + bounds.min.y * parentRect.h,
        .w = (bounds.max.x - bounds.min.x) * parentRect.w,
        .h = (bounds.max.y - bounds.min.y) * parentRect.h,
    };

    if (slotRect.w <= 1.0f || slotRect.h <= 1.0f) {
        return;
    }

    if (textureDirty || !cachedGraphTexture || slotRect.w != lastCachedW || slotRect.h != lastCachedH || mode != lastCachedMode) {
        RebuildTexture(renderer, slotRect.w, slotRect.h);
    }

    if (cachedGraphTexture) {
        SDL_RenderTexture(renderer, cachedGraphTexture, nullptr, &slotRect);
    }
}

} // namespace Game::Gameplay
