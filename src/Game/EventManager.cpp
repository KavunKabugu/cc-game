//
// Created by karp on 14/04/2026.
//

#include "EventManager.h"

#include <algorithm>
#include <vector>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include "Game/Events/Interfaces.h"
#include "Game/objects/Container.h"

namespace Game {

namespace {

[[nodiscard]] bool SubtreeContains(GameObject* root, GameObject* target) {
    if (!root || !target) {
        return false;
    }
    if (root == target) {
        return true;
    }
    auto* container = root->AsContainer();
    if (!container) {
        return false;
    }

    return std::ranges::any_of(container->GetChildren(), [&](const auto& child) {
        return SubtreeContains(child.get(), target);
    });
}

// Maps root-space coordinates into `target`'s local space without containment checks so drag capture
// keeps receiving moves when the cursor leaves the captured widget's bounds.
[[nodiscard]] bool ResolveLocalAlongPathToTarget(
    GameObject* node,
    const UnitPoint parentSpacePos,
    GameObject* target,
    UnitPoint& outLocal
) {
    if (!node) {
        return false;
    }

    const UnitPoint nodeLocal = node->GlobalToLocal(parentSpacePos);
    if (node == target) {
        outLocal = nodeLocal;
        return true;
    }

    auto* container = node->AsContainer();
    if (!container) {
        return false;
    }

    for (const auto& child : container->GetChildren()) {
        if (!SubtreeContains(child.get(), target)) {
            continue;
        }
        return ResolveLocalAlongPathToTarget(child.get(), nodeLocal, target, outLocal);
    }
    return false;
}

bool ResolveLocalForTarget(const std::vector<Container*>& roots, GameObject* target, const UnitPoint rootSpacePos, UnitPoint& outLocal) {
    if (!target) {
        return false;
    }
    for (Container* root : roots) {
        if (root && ResolveLocalAlongPathToTarget(root, rootSpacePos, target, outLocal)) {
            return true;
        }
    }
    return false;
}

} // namespace

void EventManager::Push(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat) return; // Ignore repeats early
    eventQueue.push_back(event);
}

void EventManager::Dispatch(Container* root, const int windowW, const int windowH) {
    Dispatch(std::vector{root}, windowW, windowH);
}

void EventManager::Dispatch(const std::vector<Container*>& roots, const int windowW, const int windowH) {
    while (!eventQueue.empty()) {
        SDL_Event event = eventQueue.front();
        eventQueue.pop_front();

        if (roots.empty()) {
            ProcessInternal(event, {}, windowW, windowH, true);
            continue;
        }

        ProcessInternal(event, roots, windowW, windowH, false);
        ProcessInternal(event, {}, windowW, windowH, true);
    }
}

void EventManager::BeginDragCapture(GameObject* target, const int button) {
    if (!target) {
        return;
    }

    if (dragCaptureTarget != nullptr && dragCaptureTarget != target) {
        if (auto* prev = dragCaptureTarget->AsMouseDraggable()) {
            prev->OnDragCancel();
        }
    }

    dragCaptureTarget = target;
    dragCaptureButton = button;
}

void EventManager::ClearDragCapture() {
    dragCaptureTarget = nullptr;
    dragCaptureButton = 0;
}

void EventManager::ResetInteractionState() {
    if (dragCaptureTarget != nullptr) {
        if (auto* drag = dragCaptureTarget->AsMouseDraggable()) {
            drag->OnDragCancel();
        }
        ClearDragCapture();
    }

    pressedObject = nullptr;
    hoveredObject = nullptr;
}

void EventManager::ClearQueue() {
    eventQueue.clear();
}

bool EventManager::IsKeyDown(const SDL_Keycode key) const {
    const auto it = keyStates.find(key);
    return it != keyStates.end() && it->second.pressed;
}

Uint64 EventManager::GetKeyTimestamp(const SDL_Keycode key) const {
    const auto it = keyStates.find(key);
    return (it != keyStates.end()) ? it->second.timestamp : 0;
}

bool EventManager::IsMouseDown(const int button) const {
    const auto it = mouseButtonStates.find(button);
    return it != mouseButtonStates.end() && it->second;
}

bool EventManager::ProcessInternal(
    const SDL_Event& event,
    const std::vector<Container*>& roots,
    const int windowW,
    const int windowH,
    const bool updateStates
) {
    // Convert mouse event to logical coordinates
    SDL_Event converted = event;
    if (SDL_Window* window = SDL_GetWindowFromEvent(&event)) {
        if (SDL_Renderer* renderer = SDL_GetRenderer(window)) {
            SDL_ConvertEventToRenderCoordinates(renderer, &converted);
        }
    }

    // Update global mouse position using LOGICAL coordinates
    if (converted.type == SDL_EVENT_MOUSE_MOTION) {
        lastMousePos.x = converted.motion.x / static_cast<float>(windowW);
        lastMousePos.y = converted.motion.y / static_cast<float>(windowH);
    } else if (converted.type == SDL_EVENT_MOUSE_BUTTON_DOWN || converted.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        lastMousePos.x = converted.button.x / static_cast<float>(windowW);
        lastMousePos.y = converted.button.y / static_cast<float>(windowH);
    } else if (converted.type == SDL_EVENT_MOUSE_WHEEL) {
        lastMousePos.x = converted.wheel.mouse_x / static_cast<float>(windowW);
        lastMousePos.y = converted.wheel.mouse_y / static_cast<float>(windowH);
    }

    bool consumed = false;

    // Dispatch the CONVERTED event FIRST (allows handlers to see PREVIOUS state in maps below)
    if (!updateStates && !roots.empty()) {
        if (dragCaptureTarget != nullptr) {
            const bool isMotion = converted.type == SDL_EVENT_MOUSE_MOTION;
            const bool isUp = converted.type == SDL_EVENT_MOUSE_BUTTON_UP;

            if (const bool matchingButton = isUp && converted.button.button == dragCaptureButton; isMotion || matchingButton) {
                if (UnitPoint local{}; ResolveLocalForTarget(roots, dragCaptureTarget, lastMousePos, local)) {
                    if (auto* drag = dragCaptureTarget->AsMouseDraggable()) {
                        if (isMotion) {
                            drag->OnDragMove(local);
                        } else {
                            drag->OnDragEnd(dragCaptureButton, local);
                            ClearDragCapture();
                            pressedObject = nullptr;
                        }
                        consumed = true;
                    } else {
                        ClearDragCapture();
                        pressedObject = nullptr;
                        consumed = true;
                    }
                } else {
                    // Target left scene graph mid-drag
                    if (auto* drag = dragCaptureTarget->AsMouseDraggable()) {
                        drag->OnDragCancel();
                    }
                    ClearDragCapture();
                    pressedObject = nullptr;
                    consumed = true;
                }
            }
        }

        if (!consumed) {
            for (Container* root : roots) {
                if (root && root->HandleEvent(converted, lastMousePos, pressedObject, hoveredObject)) {
                    consumed = true;
                    break;
                }
            }
        }
    }

    if (updateStates) {
        // Update state AFTER dispatch
        if (event.type == SDL_EVENT_KEY_DOWN) {
            keyStates[event.key.key] = { true, event.key.timestamp };
        } else if (event.type == SDL_EVENT_KEY_UP) {
            keyStates[event.key.key] = { false, event.key.timestamp };
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            mouseButtonStates[event.button.button] = true;
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            mouseButtonStates[event.button.button] = false;
        }
    }

    return consumed;
}

} // namespace Game
