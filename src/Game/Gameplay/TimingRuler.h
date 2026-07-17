#ifndef CC_GAME_TIMING_RULER_H
#define CC_GAME_TIMING_RULER_H

#include <vector>

#include <SDL3/SDL.h>

#include "Game/objects/Drawable.h"
#include "Judgement.h"

namespace Game::Gameplay {

// Bottom timing strip, +/-window ms, hit markers with hold + fade.
class TimingRuler final : public Drawable, public ITimeAware {
public:
    explicit TimingRuler(UnitBounds bounds = UnitBounds{.min = {.x = 0.0f, .y = 0.0f}, .max = {.x = 1.0f, .y = 1.0f}});

    void Update() override {}
    ITimeAware* AsTimeAware() override;
    void UpdateWithDeltaTime(double dt) override;
    void Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) override;

    void PushHit(double deltaMs, Judgement judgement);
    void Clear();

private:
    struct Marker {
        double deltaMs = 0.0;
        Judgement judgement = Judgement::Miss;
        double ageSeconds = 0.0;
    };

    std::vector<Marker> markers;
};

} // namespace Game::Gameplay

#endif // CC_GAME_TIMING_RULER_H
