#ifndef CC_GAME_RESULTS_GRAPH_DISPLAY_H
#define CC_GAME_RESULTS_GRAPH_DISPLAY_H

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "Game/objects/Drawable.h"
#include "Judgement.h"

namespace Game::Gameplay {

enum class ResultsGraphMode {
    Delta,
    Accuracy,
};

struct ResultsGraphEvent {
    Judgement judgement = Judgement::Miss;
    MissReason missReason = MissReason::None;
    double deltaMs = 0.0;
    std::optional<double> noteTargetTimeSeconds;
    double pressTimeSeconds = 0.0;
};

class ResultsGraphDisplay final : public Drawable {
public:
    struct CachedText {
        std::shared_ptr<SDL_Texture> texture;
        int w = 0;
        int h = 0;
    };

    explicit ResultsGraphDisplay(UnitBounds bounds = UnitBounds{{0.0f, 0.0f}, {1.0f, 1.0f}});
    ~ResultsGraphDisplay() override;

    void Update() override {}

    void Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) override;

    void SetMode(ResultsGraphMode mode);
    [[nodiscard]] ResultsGraphMode Mode() const noexcept { return mode; }

    void SetChartDomain(double firstNoteTimeSeconds, double lastNoteTimeSeconds);
    void SetEvents(std::vector<ResultsGraphEvent> events);
    void SetAccuracySteps(std::vector<std::pair<double, double>> steps);
    void SetLabelFont(std::shared_ptr<TTF_Font> font);

private:

    ResultsGraphMode mode = ResultsGraphMode::Delta;
    double chartFirstSec = 0.0;
    double chartLastSec = 0.0;
    std::vector<ResultsGraphEvent> events;
    std::vector<std::pair<double, double>> accuracySteps;
    std::shared_ptr<TTF_Font> labelFont;

    std::vector<CachedText> cachedAccuracyLabels;
    std::vector<CachedText> cachedDeltaLabels;
    bool labelsDirty = true;
    float cachedBarWidth = 1.0f;
    float lastPlotW = 0.0f;

    SDL_Texture* cachedGraphTexture = nullptr;
    float lastCachedW = 0.0f;
    float lastCachedH = 0.0f;
    ResultsGraphMode lastCachedMode = ResultsGraphMode::Delta;
    bool textureDirty = true;

    void RebuildCachedLabels(SDL_Renderer* renderer);
    void ClearCachedLabels();
    void RebuildTexture(SDL_Renderer* renderer, float w, float h);
    void FreeTexture();
};

[[nodiscard]] double NormalizedPlotXForHit(
    const HitResult& result,
    double chartFirstNoteTimeSeconds,
    double chartLastNoteTimeSeconds);

} // namespace Game::Gameplay

#endif // CC_GAME_RESULTS_GRAPH_DISPLAY_H
