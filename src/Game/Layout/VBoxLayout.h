#ifndef CC_GAME_VBOX_LAYOUT_H
#define CC_GAME_VBOX_LAYOUT_H

#include "ILayoutHelper.h"

namespace Game::Layout {

class VBoxLayout final : public ILayoutHelper {
public:
    VBoxLayout(const float spacing, const float padding, const float fixedItemHeight)
        : m_spacing(spacing), m_padding(padding), m_fixedItemHeight(fixedItemHeight) {}

    void Apply(Container& container) const override;
    [[nodiscard]] UnitPoint CalculateTotalSize(const Container& container) const override;

private:
    float m_spacing;
    float m_padding;
    float m_fixedItemHeight;
};

} // namespace Game::Layout

#endif // CC_GAME_VBOX_LAYOUT_H
