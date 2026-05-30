//
// Created by karp on 14/04/2026.
//

#ifndef CC_GAME_INTERFACES_H
#define CC_GAME_INTERFACES_H

#include "Game/objects/GameObject.h"
#include <SDL3/SDL_keycode.h>

namespace Game {

class IMouseClickable {
public:
    virtual ~IMouseClickable() = default;
    virtual bool OnMouseButtonDown(int button, UnitPoint localPos) = 0;
    virtual bool OnMouseButtonUp(int button, UnitPoint localPos) = 0;
    virtual void OnMouseButtonClicked(int button, UnitPoint localPos) = 0;
};

class IMouseHoverable {
public:
    virtual ~IMouseHoverable() = default;
    virtual void OnMouseEnter() = 0;
    virtual void OnMouseLeave() = 0;
    virtual void OnMouseMove(UnitPoint localPos) = 0;
};

class IMouseScrollable {
public:
    virtual ~IMouseScrollable() = default;
    virtual bool OnMouseWheel(float x, float y) = 0;
};

// Normalized coordinates local to the implementing widget's bounds (same space as other mouse callbacks).
class IMouseDraggable {
public:
    virtual ~IMouseDraggable() = default;

    // Return false to abort drag setup (caller must not begin capture if false).
    [[nodiscard]] virtual bool OnDragStart(int button, UnitPoint localPos) = 0;
    virtual void OnDragMove(UnitPoint localPos) = 0;
    virtual void OnDragEnd(int button, UnitPoint localPos) = 0;

    // Called when capture is cleared without a matching DragEnd (scene change, reset).
    virtual void OnDragCancel() {}
};

class IKeyHandler {
public:
    virtual ~IKeyHandler() = default;
    virtual bool OnKeyDown(SDL_Keycode key, Uint64 timestamp) = 0;
    virtual bool OnKeyUp(SDL_Keycode key, Uint64 timestamp) = 0;
};

} // namespace Game

#endif //CC_GAME_INTERFACES_H
