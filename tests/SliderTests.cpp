#include <cassert>
#include <cmath>
#include "Game/objects/Slider.h"

namespace {

using namespace Game;

void TestSnappingAndClamping() {
    // Slider from 0.0 to 10.0, step 2.0, initial 4.0
    Slider slider(UnitBounds{{0,0},{1,1}}, 0.0f, 10.0f, 2.0f, 4.0f);
    assert(slider.GetValue() == 4.0f);

    // Test snapping: 4.8 snaps to 4.0
    slider.SetValue(4.8f);
    assert(slider.GetValue() == 4.0f);

    slider.SetValue(5.1f);
    assert(slider.GetValue() == 6.0f);

    // Test clamping
    slider.SetValue(-5.0f);
    assert(slider.GetValue() == 0.0f);

    slider.SetValue(15.0f);
    assert(slider.GetValue() == 10.0f);
}

void TestCallback() {
    Slider slider(UnitBounds{{0,0},{1,1}}, 0.0f, 10.0f, 1.0f, 5.0f);

    float callbackValue = -1.0f;
    slider.SetOnChanged([&](const float val) {
        callbackValue = val;
    });

    slider.SetValue(8.0f);
    assert(callbackValue == 8.0f);

    // Calling SetValue with same value should not trigger callback
    callbackValue = -1.0f;
    slider.SetValue(8.0f);
    assert(callbackValue == -1.0f);
}

void TestLocalCoordinatesToValue() {
    // Slider from 0.0 to 100.0, step 1.0
    Slider slider(UnitBounds{{0,0},{1,1}}, 0.0f, 100.0f, 1.0f, 0.0f);

    // Usable range is between kThumbHalfNorm (0.02f) and 1.0 - kThumbHalfNorm (0.98f).
    // Test mapping.
    // At x = 0.02f, t = 0.0 -> value = 0.0
    slider.OnMouseButtonDown(SDL_BUTTON_LEFT, UnitPoint{0.02f, 0.5f});
    assert(slider.GetValue() == 0.0f);

    // At x = 0.98f, t = 1.0 -> value = 100.0
    slider.OnMouseButtonDown(SDL_BUTTON_LEFT, UnitPoint{0.98f, 0.5f});
    assert(slider.GetValue() == 100.0f);

    // Midpoint: x = 0.5f -> t = (0.5 - 0.02) / 0.96 = 0.5 -> value = 50.0
    slider.OnMouseButtonDown(SDL_BUTTON_LEFT, UnitPoint{0.50f, 0.5f});
    assert(slider.GetValue() == 50.0f);
}

void TestMouseAndDragEvents() {
    Slider slider(UnitBounds{{0,0},{1,1}}, 0.0f, 100.0f, 1.0f, 0.0f);

    // Mouse click at local x = 0.5f (value = 50)
    const bool clicked = slider.OnMouseButtonDown(SDL_BUTTON_LEFT, UnitPoint{0.5f, 0.5f});
    assert(clicked);
    assert(slider.GetValue() == 50.0f);

    // Move drag to local x = 0.74f (t = (0.74 - 0.02) / 0.96 = 0.75 -> value = 75)
    slider.OnDragMove(UnitPoint{0.74f, 0.5f});
    assert(slider.GetValue() == 75.0f);

    // End drag at local x = 0.98f (value = 100)
    slider.OnDragEnd(SDL_BUTTON_LEFT, UnitPoint{0.98f, 0.5f});
    assert(slider.GetValue() == 100.0f);
}

} // namespace

int main() {
    TestSnappingAndClamping();
    TestCallback();
    TestLocalCoordinatesToValue();
    TestMouseAndDragEvents();
    return 0;
}
