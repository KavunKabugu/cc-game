//
// Created by karp on 14/04/2026.
//

#ifndef CC_GAME_EVENTMANAGER_H
#define CC_GAME_EVENTMANAGER_H

#include <SDL3/SDL.h>
#include <deque>
#include <unordered_map>
#include <vector>
#include "Game/objects/GameObject.h"

namespace Game {

class Container;

struct KeyState {
    bool pressed = false;
    Uint64 timestamp = 0;
};

class EventManager {
public:
    static EventManager& getInstance() {
        static EventManager instance;
        return instance;
    }

    void Push(const SDL_Event& event);
    void Dispatch(Container* root, int windowW, int windowH);
    void Dispatch(const std::vector<Container*>& roots, int windowW, int windowH);
    void ResetInteractionState();
    void ClearQueue();
    void NotifyObjectDestroyed(const GameObject* obj);

    void BeginDragCapture(GameObject* target, int button);

    [[nodiscard]] bool IsKeyDown(SDL_Keycode key) const;
    [[nodiscard]] Uint64 GetKeyTimestamp(SDL_Keycode key) const;
    [[nodiscard]] bool IsMouseDown(int button) const;
    [[nodiscard]] UnitPoint GetMousePos() const { return lastMousePos; }

private:
    void ClearDragCapture();

    bool ProcessInternal(const SDL_Event& event, const std::vector<Container*>& roots, int windowW, int windowH,
                         bool updateStates);

    std::deque<SDL_Event> eventQueue;
    std::unordered_map<SDL_Keycode, KeyState> keyStates;
    std::unordered_map<int, bool> mouseButtonStates;
    UnitPoint lastMousePos{.x = 0.0f, .y = 0.0f};

    // For Clicked & Hover events
    GameObject* pressedObject = nullptr;
    GameObject* hoveredObject = nullptr;

    GameObject* dragCaptureTarget = nullptr;
    int dragCaptureButton = 0;
};

} // namespace Game

#endif //CC_GAME_EVENTMANAGER_H
