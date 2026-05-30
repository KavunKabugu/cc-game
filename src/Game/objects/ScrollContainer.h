#ifndef CC_GAME_SCROLL_CONTAINER_H
#define CC_GAME_SCROLL_CONTAINER_H

#include "Container.h"
#include "Game/Layout/ILayoutHelper.h"
#include <memory>

namespace Game {

class ScrollContainer : public Container,
                        public IMouseScrollable,
                        public IMouseClickable,
                        public IMouseDraggable,
                        public ITimeAware {
public:
    explicit ScrollContainer(UnitBounds bounds);
    
    void Update() override;
    void Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) override;
    bool HandleEvent(const SDL_Event& event, UnitPoint parentLocalPos, GameObject*& pressedObject, GameObject*& hoveredObject) override;

    IMouseScrollable* AsMouseScrollable() override { return this; }
    IMouseClickable* AsMouseClickable() override { return this; }
    IMouseDraggable* AsMouseDraggable() override { return this; }

    void SetLayout(std::unique_ptr<Layout::ILayoutHelper> layout);
    void UpdateLayout();

    void SetScrollbarWidth(const float width) { m_scrollbarWidth = width; }
    void SetScrollbarColor(const SDL_Color track, const SDL_Color thumb) { m_trackColor = track; m_thumbColor = thumb; }

    // IMouseScrollable
    bool OnMouseWheel(float x, float y) override { return false; } // Handled in HandleEvent

    // IMouseClickable
    bool OnMouseButtonDown(int button, UnitPoint localPos) override { return false; } // Handled in HandleEvent
    bool OnMouseButtonUp(int button, UnitPoint localPos) override { return false; } // Handled in HandleEvent
    void OnMouseButtonClicked(int button, UnitPoint localPos) override {} // Handled in HandleEvent

    // IMouseDraggable (scrollbar thumb; capture routed via EventManager)
    bool OnDragStart(int button, UnitPoint localPos) override;
    void OnDragMove(UnitPoint localPos) override;
    void OnDragEnd(int button, UnitPoint localPos) override;

    // ITimeAware
    ITimeAware* AsTimeAware() override;
    void UpdateWithDeltaTime(double dt) override;

private:
    void ClampScroll();
    [[nodiscard]] SDL_FRect GetThumbRect() const;
    [[nodiscard]] bool IsMouseOnThumb(UnitPoint localPos) const;

    std::unique_ptr<Layout::ILayoutHelper> m_layout;
    
    float m_scrollOffsetY{0.0f};
    float m_targetScrollOffsetY{0.0f};
    float m_contentHeight{0.0f};
    
    float m_scrollbarWidth{12.0f};
    SDL_Color m_trackColor{40, 40, 45, 180};
    SDL_Color m_thumbColor{180, 180, 190, 255};
    
    float m_dragStartY{0.0f};
    float m_dragStartScrollY{0.0f};
    float m_frameDeltaTime{1.0f / 60.0f};
};

} // namespace Game

#endif //CC_GAME_SCROLL_CONTAINER_H
