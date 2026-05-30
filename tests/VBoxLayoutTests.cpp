#include <cassert>
#include <cmath>
#include "Game/Layout/VBoxLayout.h"
#include "Game/objects/Container.h"

namespace {

using namespace Game;
using namespace Game::Layout;

class FakeChild final : public GameObject {
public:
    explicit FakeChild(const UnitBounds bounds) : GameObject(bounds) {}
    void Update() override {}
};

void TestCalculateTotalSize() {
    Container parent{UnitBounds{{0,0},{1,1}}};

    // VBoxLayout(spacing = 10, padding = 15, fixedItemHeight = 50)
    const VBoxLayout layout(10.0f, 15.0f, 50.0f);

    // Empty container
    UnitPoint emptySize = layout.CalculateTotalSize(parent);
    assert(emptySize.x == 0.0f && emptySize.y == 0.0f);

    // Add children
    parent.CreateChild<FakeChild>(UnitBounds{{0,0},{0,0}});
    parent.CreateChild<FakeChild>(UnitBounds{{0,0},{0,0}});

    // 2 children height:
    // padding * 2 = 30
    // 2 * fixedItemHeight = 100
    // 1 * spacing = 10
    // total = 30 + 100 + 10 = 140
    UnitPoint size = layout.CalculateTotalSize(parent);
    assert(size.y == 140.0f);
}

void TestApplyLayout() {
    Container parent{UnitBounds{{0,0},{1,1}}};
    const VBoxLayout layout(10.0f, 15.0f, 50.0f);

    const auto* c1 = parent.CreateChild<FakeChild>(UnitBounds{{0,0},{0,0}});
    const auto* c2 = parent.CreateChild<FakeChild>(UnitBounds{{0,0},{0,0}});

    layout.Apply(parent);

    // Total height = 140
    // c1 bounds: yMin = 15 / 140, yMax = (15 + 50) / 140 = 65 / 140
    UnitBounds b1 = c1->GetBounds();
    assert(std::abs(b1.min.y - (15.0f / 140.0f)) < 1e-5f);
    assert(std::abs(b1.max.y - (65.0f / 140.0f)) < 1e-5f);
    assert(b1.min.x == 0.0f && b1.max.x == 1.0f);

    // c2 bounds: yMin = (15 + 50 + 10) / 140 = 75 / 140, yMax = (75 + 50) / 140 = 125 / 140
    UnitBounds b2 = c2->GetBounds();
    assert(std::abs(b2.min.y - (75.0f / 140.0f)) < 1e-5f);
    assert(std::abs(b2.max.y - (125.0f / 140.0f)) < 1e-5f);
    assert(b2.min.x == 0.0f && b2.max.x == 1.0f);
}

} // namespace

int main() {
    TestCalculateTotalSize();
    TestApplyLayout();
    return 0;
}
