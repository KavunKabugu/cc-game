#include "RhythmField.h"

#include "Game/Profile.h"

#include <algorithm>
#include <utility>

#include "GameplayConstants.h"
#include "GameplayMath.h"
#include "NoteSimulation.h"

namespace Game::Gameplay {

namespace {

struct TextureState {
    SDL_Color color{.r = 0, .g = 0, .b = 0, .a = 0};
    SDL_BlendMode blendMode{SDL_BLENDMODE_NONE};
    bool initialized = false;
};

void SetTextureStateIfNeeded(SDL_Texture* texture, const SDL_Color color, const SDL_BlendMode blendMode, TextureState& current) {
    if (!current.initialized || color.r != current.color.r || color.g != current.color.g || color.b != current.color.b) {
        SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
        current.color.r = color.r;
        current.color.g = color.g;
        current.color.b = color.b;
    }
    if (!current.initialized || color.a != current.color.a) {
        SDL_SetTextureAlphaMod(texture, color.a);
        current.color.a = color.a;
    }
    if (!current.initialized || blendMode != current.blendMode) {
        SDL_SetTextureBlendMode(texture, blendMode);
        current.blendMode = blendMode;
    }
    current.initialized = true;
}

void DrawArcQuarter(SDL_Renderer* renderer, SDL_Texture* texture,
                    const float centerX, const float centerY,
                    const float radius, const float angleDegrees,
                    const SDL_Color color, TextureState& state) {
    if (radius <= 0.0f) return;

    const float diameter = radius * 2.0f;
    const SDL_FRect dst = {
        .x = centerX - radius,
        .y = centerY - radius,
        .w = diameter,
        .h = diameter,
    };

    SetTextureStateIfNeeded(texture, color, SDL_BLENDMODE_BLEND, state);

    const double rotation = ArcRotationDegrees(angleDegrees);
    SDL_RenderTextureRotated(renderer, texture, nullptr, &dst, rotation, nullptr, SDL_FLIP_NONE);
}

SDL_Color NoteColorFor(const RuntimeNote& note) {
    if (note.resolved) {
        return note.judgement == Judgement::Miss ? kNoteColorMiss : kNoteColorHit;
    }
    return kNoteColorDefault;
}

} // namespace

RhythmField::RhythmField(const UnitBounds bounds,
                         std::shared_ptr<SDL_Texture> arcTexture,
                         const NoteSimulation* sim)
    : Drawable(bounds), arcTexture(std::move(arcTexture)), sim(sim) {}

void RhythmField::Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) {
    CC_PROFILE("RhythmField.Render");
    if (!arcTexture || !sim) return;

    const SDL_FRect rect = {
        .x = parentRect.x + bounds.min.x * parentRect.w,
        .y = parentRect.y + bounds.min.y * parentRect.h,
        .w = (bounds.max.x - bounds.min.x) * parentRect.w,
        .h = (bounds.max.y - bounds.min.y) * parentRect.h,
    };

    const float centerX = rect.x + rect.w * 0.5f;
    const float centerY = rect.y + rect.h * 0.5f;

    SDL_Texture* texture = arcTexture.get();
    TextureState state{};

    const float crosshairRadius = sim->CrosshairRadius();
    for (const float angle : kCrosshairAngles) {
        DrawArcQuarter(renderer, texture, centerX, centerY, crosshairRadius, angle, kCrosshairColor, state);
    }

    const auto active = sim->ActiveNotes();
    if (active.empty()) return;

    const int screenWidth = sim->ScreenWidth();
    for (const RuntimeNote* note : active) {
        if (note->zLocation <= 0.0f) continue;
        const float radius = ArcRadius(note->zLocation, screenWidth);
        const float angle = LaneToAngleDegrees(note->lane);
        DrawArcQuarter(renderer, texture, centerX, centerY, radius, angle, NoteColorFor(*note), state);
    }
}

} // namespace Game::Gameplay
