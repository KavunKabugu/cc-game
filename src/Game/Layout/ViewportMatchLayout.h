#ifndef CC_GAME_VIEWPORT_MATCH_LAYOUT_H
#define CC_GAME_VIEWPORT_MATCH_LAYOUT_H

#include "ILayoutHelper.h"

namespace Game::Layout {

// Content height equals the scroll viewport, child unit bounds are left unchanged.
// Used for options tabs that still use absolute 0–1 layouts and should not scroll.
// TODO: This is terrible, get rid of it.
class ViewportMatchLayout final : public ILayoutHelper {
public:
    void Apply(Container& container) const override;
    [[nodiscard]] UnitPoint CalculateTotalSize(const Container& container) const override;
};

} // namespace Game::Layout

#endif // CC_GAME_VIEWPORT_MATCH_LAYOUT_H
