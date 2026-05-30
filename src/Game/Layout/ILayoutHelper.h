#ifndef CC_GAME_I_LAYOUT_HELPER_H
#define CC_GAME_I_LAYOUT_HELPER_H

#include "Game/objects/GameObject.h"

namespace Game {
    class Container;
}

namespace Game::Layout {

class ILayoutHelper {
public:
    virtual ~ILayoutHelper() = default;

    virtual void Apply(Container& container) const = 0;
    
    // Returns the total size in pixels required for the content
    [[nodiscard]] virtual UnitPoint CalculateTotalSize(const Container& container) const = 0;
};

} // namespace Game::Layout

#endif // CC_GAME_I_LAYOUT_HELPER_H
