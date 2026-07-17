//
// Created by karp on 09/04/2026.
//

#ifndef CC_GAME_GAME_OBJECT_H
#define CC_GAME_GAME_OBJECT_H

namespace Game {

struct UnitPoint {
    float x;
    float y;
};

struct UnitBounds {
    UnitPoint min; // Top-left (0,0 to 1,1)
    UnitPoint max; // Bottom-right (0,0 to 1,1)
};

class ITimeAware;
class Container;
class Drawable;
class IKeyHandler;
class IMouseClickable;
class IMouseHoverable;
class IMouseScrollable;
class IMouseDraggable;

class GameObject {
public:
    explicit GameObject(const UnitBounds bounds) : bounds(bounds) {}
    virtual ~GameObject();

    virtual void Update() = 0;

    virtual ITimeAware* AsTimeAware() { return nullptr; }
    virtual Container* AsContainer() { return nullptr; }
    virtual Drawable* AsDrawable() { return nullptr; }
    virtual IKeyHandler* AsKeyHandler() { return nullptr; }
    virtual IMouseClickable* AsMouseClickable() { return nullptr; }
    virtual IMouseHoverable* AsMouseHoverable() { return nullptr; }
    virtual IMouseScrollable* AsMouseScrollable() { return nullptr; }
    virtual IMouseDraggable* AsMouseDraggable() { return nullptr; }

    void SetBounds(const UnitBounds newBounds) { bounds = newBounds; }

    [[nodiscard]] bool Contains(const UnitPoint p) const {
        return p.x >= bounds.min.x && p.x <= bounds.max.x &&
               p.y >= bounds.min.y && p.y <= bounds.max.y;
    }

    [[nodiscard]] UnitPoint GlobalToLocal(const UnitPoint p) const {
        const float w = bounds.max.x - bounds.min.x;
        const float h = bounds.max.y - bounds.min.y;
        if (w <= 0.0f || h <= 0.0f) return {.x = 0.0f, .y = 0.0f};
        return { .x = (p.x - bounds.min.x) / w, .y = (p.y - bounds.min.y) / h };
    }

    [[nodiscard]] UnitBounds GetBounds() const { return bounds; }

protected:
    UnitBounds bounds;
};

class ITimeAware {
public:
    virtual ~ITimeAware() = default;
    virtual void UpdateWithDeltaTime(double dt) = 0;
};

} // namespace Game

#endif //CC_GAME_GAME_OBJECT_H
