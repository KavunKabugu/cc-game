#include "VBoxLayout.h"
#include "Game/objects/Container.h"

namespace Game::Layout {

void VBoxLayout::Apply(Container& container) const {
    const auto& children = container.GetChildren();
    if (children.empty()) return;

    auto [x, y] = CalculateTotalSize(container);
    if (y <= 0.0f) return;

    float currentY = m_padding;
    // We assume %100 width for vertical box
    for (const auto& child : children) {
        if (!child) continue;

        const float normYMin = currentY / y;
        const float normYMax = (currentY + m_fixedItemHeight) / y;

        // X is 0 to 1 normally.
        // TODO: Respect horizontal padding if needed.
        child->SetBounds(UnitBounds{{0.0f, normYMin}, {1.0f, normYMax}});

        currentY += m_fixedItemHeight + m_spacing;
    }
}

UnitPoint VBoxLayout::CalculateTotalSize(const Container& container) const {
    const auto& children = container.GetChildren();
    if (children.empty()) return {0.0f, 0.0f};

    float totalHeight = m_padding * 2.0f;
    totalHeight += static_cast<float>(children.size()) * m_fixedItemHeight;
    totalHeight += static_cast<float>(children.size() - 1) * m_spacing;

    // Width should be parent width
    return {container.GetLastRenderRect().w, totalHeight};
}

} // namespace Game::Layout
