#include "ScrollContainer.h"

#include <algorithm>
#include <cmath>
#include <ranges>

#include "Game/EventManager.h"

namespace Game {

ScrollContainer::ScrollContainer(const UnitBounds bounds) : Container(bounds) {
    m_scrollbarWidth = 16.0f;
    m_trackColor = {60, 60, 70, 200};
    m_thumbColor = {220, 220, 230, 255};
}

ITimeAware* ScrollContainer::AsTimeAware() {
    return this;
}

void ScrollContainer::UpdateWithDeltaTime(const double dt) {
    // Clamp to a sane range so pausing/debugger stalls do not create huge jumps.
    m_frameDeltaTime = static_cast<float>(std::clamp(dt, 0.0, 0.1));
}

void ScrollContainer::Update() {
    Container::Update();

    // Glide logic (smooth scroll)
    const float deltaTime = m_frameDeltaTime;
    
    if (std::abs(m_targetScrollOffsetY - m_scrollOffsetY) > 0.1f) {
        constexpr float glideSpeed = 10.0f;
        m_scrollOffsetY += (m_targetScrollOffsetY - m_scrollOffsetY) * glideSpeed * deltaTime;
    } else {
        m_scrollOffsetY = m_targetScrollOffsetY;
    }
}

void ScrollContainer::Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) {
    const SDL_FRect myRect = {
        parentRect.x + (bounds.min.x * parentRect.w),
        parentRect.y + (bounds.min.y * parentRect.h),
        (bounds.max.x - bounds.min.x) * parentRect.w,
        (bounds.max.y - bounds.min.y) * parentRect.h
    };

    if (myRect.w <= 0.0f || myRect.h <= 0.0f) return;
    
    if (m_contentHeight <= 0.0f) {
        UpdateLayout();
    }

    lastRenderRect = myRect;

    // Save clip rect
    SDL_Rect oldClip{0, 0, 0, 0};
    const bool hadClip = SDL_RenderClipEnabled(renderer);
    if (hadClip) {
        SDL_GetRenderClipRect(renderer, &oldClip);
    }

    // Apply viewport clip
    const SDL_Rect clipRect = {
        static_cast<int>(myRect.x),
        static_cast<int>(myRect.y),
        static_cast<int>(myRect.w),
        static_cast<int>(myRect.h)
    };
    SDL_SetRenderClipRect(renderer, &clipRect);

    // Render Children with scroll offset
    // Content area is shifted by -scrollOffsetY
    const SDL_FRect contentRect = {
        myRect.x,
        myRect.y - m_scrollOffsetY,
        myRect.w - m_scrollbarWidth, // Leave space for scrollbar
        m_contentHeight
    };

    for (const auto& child : children) {
        if (auto* d = child->AsDrawable()) {
            // Culling check
            auto [min, max] = child->GetBounds();
            const float childY = contentRect.y + (min.y * m_contentHeight);

            if (const float childH = (max.y - min.y) * m_contentHeight; childY + childH < myRect.y || childY > myRect.y + myRect.h) {
                continue; // Not in viewport
            }

            d->Render(renderer, contentRect);
        }
    }

    // Render Scrollbar
    if ((m_contentHeight > myRect.h + 0.1f)) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        // Track
        const SDL_FRect track = {myRect.x + myRect.w - m_scrollbarWidth, myRect.y, m_scrollbarWidth, myRect.h};
        SDL_SetRenderDrawColor(renderer, m_trackColor.r, m_trackColor.g, m_trackColor.b, m_trackColor.a);
        SDL_RenderFillRect(renderer, &track);

        // Thumb
        SDL_FRect thumb = GetThumbRect();
        thumb.x += myRect.x + myRect.w - m_scrollbarWidth;
        thumb.y += myRect.y;
        SDL_SetRenderDrawColor(renderer, m_thumbColor.r, m_thumbColor.g, m_thumbColor.b, m_thumbColor.a);
        SDL_RenderFillRect(renderer, &thumb);
    }

    // Restore previous clip after drawing content and scrollbar in this viewport
    SDL_SetRenderClipRect(renderer, hadClip ? &oldClip : nullptr);
}

bool ScrollContainer::HandleEvent(const SDL_Event& event, const UnitPoint parentLocalPos, GameObject*& pressedObject, GameObject*& hoveredObject) {
    const bool inside = Contains(parentLocalPos);
    const UnitPoint localPos = GlobalToLocal(parentLocalPos);
    const float pixelW = lastRenderRect.w;
    const float pixelH = lastRenderRect.h;
    // Handle Scrollbar Interaction
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && inside) {
        if (const float threshold = (pixelW > 0.0f) ? (1.0f - (m_scrollbarWidth / pixelW)) : 1.0f; localPos.x > threshold) {
            if (IsMouseOnThumb(localPos)) {
                EventManager::getInstance().BeginDragCapture(this, event.button.button);
                (void)OnDragStart(event.button.button, localPos);
                pressedObject = this;
                return true;
            }
        }
    }

    // Handle Mouse Wheel
    if (event.type == SDL_EVENT_MOUSE_WHEEL && inside) {
        m_targetScrollOffsetY -= event.wheel.y * 50.0f; // 50px per notch
        ClampScroll();
        return true;
    }

    // Translate coordinates for children
    const float localPixelsX = localPos.x * pixelW;
    const float localPixelsY = localPos.y * pixelH + m_scrollOffsetY;

    const UnitPoint contentLocalPos = {
        localPixelsX / (pixelW - m_scrollbarWidth),
        localPixelsY / m_contentHeight
    };

    // If it's outside the content area (on the scrollbar), don't pass mouse events
    if (localPos.x > 1.0f - (m_scrollbarWidth / pixelW)) {
        return false;
    }

    // Pass to children
    for (auto & child : std::views::reverse(GetChildren())) {
        if (child->Contains(contentLocalPos)) {
            const UnitPoint childLocal = child->GlobalToLocal(contentLocalPos);

            // Hover
            if (auto* hoverable = child->AsMouseHoverable()) {
                if (hoveredObject != child.get()) {
                    if (hoveredObject) {
                        if (auto* oldHover = hoveredObject->AsMouseHoverable()) 
                            oldHover->OnMouseLeave();
                    }
                    hoveredObject = child.get();
                    hoverable->OnMouseEnter();
                }
                if (event.type == SDL_EVENT_MOUSE_MOTION) hoverable->OnMouseMove(childLocal);
            }

            // Click
            if (auto* clickable = child->AsMouseClickable()) {
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    pressedObject = child.get();
                    return clickable->OnMouseButtonDown(event.button.button, childLocal);
                }
                if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                    const bool consumed = clickable->OnMouseButtonUp(event.button.button, childLocal);
                    if (pressedObject == child.get()) clickable->OnMouseButtonClicked(event.button.button, childLocal);
                    pressedObject = nullptr;
                    return consumed;
                }
            }
        }
    }

    return false;
}

void ScrollContainer::SetLayout(std::unique_ptr<Layout::ILayoutHelper> layout) {
    m_layout = std::move(layout);
}

void ScrollContainer::UpdateLayout() {
    if (!m_layout) return;
    
    m_contentHeight = m_layout->CalculateTotalSize(*this).y;
    m_layout->Apply(*this);
    ClampScroll();
}

void ScrollContainer::ClampScroll() {
    const float maxScroll = std::max(0.0f, m_contentHeight - lastRenderRect.h);
    m_targetScrollOffsetY = std::clamp(m_targetScrollOffsetY, 0.0f, maxScroll);
}

SDL_FRect ScrollContainer::GetThumbRect() const {
    const float h = lastRenderRect.h;
    if (m_contentHeight <= h) return {0, 0, 0, 0};

    const float thumbHeight = std::max(20.0f, (h / m_contentHeight) * h);
    const float scrollableHeight = m_contentHeight - h;
    const float scrollRatio = m_scrollOffsetY / scrollableHeight;
    const float thumbY = scrollRatio * (h - thumbHeight);

    return {0, thumbY, m_scrollbarWidth, thumbHeight};
}

bool ScrollContainer::IsMouseOnThumb(const UnitPoint localPos) const {
    const SDL_FRect thumb = GetThumbRect();
    const float pxY = localPos.y * lastRenderRect.h;
    const float pxX = localPos.x * lastRenderRect.w;
    const float trackX = lastRenderRect.w - m_scrollbarWidth;

    return pxX >= trackX && pxY >= thumb.y && pxY <= thumb.y + thumb.h;
}

bool ScrollContainer::OnDragStart(const int button, const UnitPoint localPos) {
    (void)button;
    const float pixelH = lastRenderRect.h;
    if (pixelH <= 0.0f) {
        return false;
    }
    m_dragStartY = localPos.y * pixelH;
    m_dragStartScrollY = m_scrollOffsetY;
    return true;
}

void ScrollContainer::OnDragMove(const UnitPoint localPos) {
    const float pixelH = lastRenderRect.h;
    if (pixelH <= 0.0f || m_contentHeight <= pixelH + 0.1f) {
        return;
    }

    const float currentY = localPos.y * pixelH;
    const float deltaY = currentY - m_dragStartY;

    const float scrollableHeight = m_contentHeight - pixelH;
    const float trackHeight = pixelH;
    const float thumbHeight = std::max(20.0f, (pixelH / m_contentHeight) * pixelH);
    const float denom = std::max(trackHeight - thumbHeight, 1e-3f);
    const float dragScale = scrollableHeight / denom;

    m_targetScrollOffsetY = m_dragStartScrollY + (deltaY * dragScale);
    ClampScroll();
}

void ScrollContainer::OnDragEnd(const int button, const UnitPoint localPos) {
    (void)button;
    (void)localPos;
}

} // namespace Game
