//
// Created by ludeo on 9/17/26.
//

#ifndef CC_GAME_DISCORDPRESENCEMANAGER_H
#define CC_GAME_DISCORDPRESENCEMANAGER_H
#include "discordpp.h"

namespace Game {

class DiscordPresenceManager {
public:
    static DiscordPresenceManager& getInstance() {
        static DiscordPresenceManager instance;
        return instance;
    }

    bool Init();
    void Update(const discordpp::Activity &activity) const;
    void Update(std::string state, std::string details) const;
private:
    std::shared_ptr<discordpp::Client> client = nullptr;
};
}
#endif //CC_GAME_DISCORDPRESENCEMANAGER_H
