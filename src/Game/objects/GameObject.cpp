//
// Created by karp on 09/04/2026.
//

#include "GameObject.h"

#include "Game/EventManager.h"

namespace Game {

GameObject::~GameObject() {
    EventManager::getInstance().NotifyObjectDestroyed(this);
}

} // namespace Game