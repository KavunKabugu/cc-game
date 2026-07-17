//
// Created by karp on 14/04/2026.
//

#include "Container.h"

#include <ranges>

namespace Game {

void Container::Update() {
    for (const auto& child : children) {
        child->Update();
    }
}

void Container::Render(SDL_Renderer* renderer, const SDL_FRect& parentRect) {
    const SDL_FRect myRect = {
        .x = parentRect.x + bounds.min.x * parentRect.w,
        .y = parentRect.y + bounds.min.y * parentRect.h,
        .w = (bounds.max.x - bounds.min.x) * parentRect.w,
        .h = (bounds.max.y - bounds.min.y) * parentRect.h
    };

    if (myRect.w <= 0.0f || myRect.h <= 0.0f) return;
    lastRenderRect = myRect;

    for (const auto& child : children) {
        if (auto* d = child->AsDrawable()) {
            d->Render(renderer, myRect);
        }
    }
}

bool Container::HandleEvent(const SDL_Event& event, const UnitPoint parentLocalPos, GameObject*& pressedObject, GameObject*& hoveredObject) {
    // Check for Keyboard Events (Global broadcast)
    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        for (const auto& child : children) {
            if (auto* sub = child->AsContainer()) {
                if (sub->HandleEvent(event, parentLocalPos, pressedObject, hoveredObject)) {
                    return true;
                }
            } else if (auto* handler = child->AsKeyHandler()) {
                bool consumed = false;
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    consumed = handler->OnKeyDown(event.key.key, event.key.timestamp);
                } else {
                    consumed = handler->OnKeyUp(event.key.key, event.key.timestamp);
                }
                if (consumed) {
                    return true;
                }
            }
        }
        return false;
    }

    // Mouse Coordinate Translation
    const bool inside = Contains(parentLocalPos);
    const UnitPoint localPos = GlobalToLocal(parentLocalPos);

    // Dispatch Mouse Events to children in reverse order (topmost first)
    for (auto & child : std::views::reverse(children)) {
        // If it's a container, recurse
        if (auto* subContainer = child->AsContainer()) {
            if (subContainer->HandleEvent(event, localPos, pressedObject, hoveredObject)) {
                return true; 
            }
        }
        
        // Otherwise, check if it's an interactive object
        if (child->Contains(localPos)) {
            const UnitPoint childLocal = child->GlobalToLocal(localPos);

            // Handle Hover
            if (auto* hoverable = child->AsMouseHoverable()) {
                if (hoveredObject != child.get()) {
                    if (hoveredObject) {
                        if (auto* oldHover = hoveredObject->AsMouseHoverable()) 
                            oldHover->OnMouseLeave();
                    }
                    hoveredObject = child.get();
                    hoverable->OnMouseEnter();
                }
                if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    hoverable->OnMouseMove(childLocal);
                }
            }

            // Handle Clicks
            if (auto* clickable = child->AsMouseClickable()) {
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    pressedObject = child.get();
                    return clickable->OnMouseButtonDown(event.button.button, childLocal);
                }
                if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                    const bool consumed = clickable->OnMouseButtonUp(event.button.button, childLocal);
                    if (pressedObject == child.get()) {
                        clickable->OnMouseButtonClicked(event.button.button, childLocal);
                    }
                    pressedObject = nullptr;
                    return consumed;
                }
            }

            // Handle Scroll
            if (auto* scrollable = child->AsMouseScrollable()) {
                if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                    return scrollable->OnMouseWheel(event.wheel.x, event.wheel.y);
                }
            }
        }
    }

    // Handle case where mouse leaves the currently hovered child.
    if (inside && event.type == SDL_EVENT_MOUSE_MOTION) {
        if (hoveredObject) {
            for (auto& child : children) {
                if (child.get() != hoveredObject) {
                    continue;
                }

                // Keep hover active while cursor is still over this child.
                if (!child->Contains(localPos)) {
                    if (auto* h = hoveredObject->AsMouseHoverable()) {
                        h->OnMouseLeave();
                    }
                    hoveredObject = nullptr;
                }
                break;
            }
        }
    }

    return false;
}

} // namespace Game
