#include "ViewportMatchLayout.h"

#include "Game/objects/Container.h"

namespace Game::Layout {

void ViewportMatchLayout::Apply(Container& /*container*/) const {}

UnitPoint ViewportMatchLayout::CalculateTotalSize(const Container& container) const {
    const SDL_FRect r = container.GetLastRenderRect();
    return {.x = r.w, .y = r.h};
}

} // namespace Game::Layout
