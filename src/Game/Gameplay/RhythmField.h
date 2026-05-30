#ifndef CC_GAME_RHYTHM_FIELD_H
#define CC_GAME_RHYTHM_FIELD_H

#include <memory>

#include <SDL3/SDL.h>

#include "Game/objects/Drawable.h"

namespace Game::Gameplay {

class NoteSimulation;

// Renders the crosshair and currently active note arcs over the playfield.
// Reads its state from the supplied NoteSimulation each frame.
class RhythmField final : public Drawable {
public:
    RhythmField(UnitBounds bounds, std::shared_ptr<SDL_Texture> arcTexture, const NoteSimulation* sim);

    void Update() override {}
    void Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) override;

private:
    std::shared_ptr<SDL_Texture> arcTexture;
    const NoteSimulation* sim;
};

} // namespace Game::Gameplay

#endif // CC_GAME_RHYTHM_FIELD_H
