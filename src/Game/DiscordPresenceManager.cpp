#define DISCORDPP_IMPLEMENTATION
#include "DiscordPresenceManager.h"
#include "../lib/discord_social_sdk/include/discordpp.h"
#include <iostream>

//
// Created by ludeo on 9/17/26.
//
namespace Game
{

bool DiscordPresenceManager::Init() {
    constexpr uint64_t APPLICATION_ID = 1550023229031321670;

    client = std::make_shared<discordpp::Client>();
    client->SetApplicationId(APPLICATION_ID);

    return true;
}

void DiscordPresenceManager::Update(const discordpp::Activity &activity) const {
    client->UpdateRichPresence(
        activity, [](const discordpp::ClientResult &result) {
          if (result.Successful()) {
            std::cout << "🎮 Rich Presence updated successfully!\n";
          } else {
            std::cerr << "❌ Rich Presence update failed";
          }
        });
}

void DiscordPresenceManager::Update(std::string state = "", std::string details = "") const {
    discordpp::Activity activity;
    activity.SetType(discordpp::ActivityTypes::Playing);
    if (!details.empty())
    {
      activity.SetDetails(details);
    }
    if (!state.empty())
    {
      activity.SetState(state);
    }
    activity.SetName("CC Game");

    client->UpdateRichPresence(
        activity, [](const discordpp::ClientResult &result) {
          if (result.Successful()) {
            std::cout << "🎮 Rich Presence updated successfully!\n";
          } else {
            std::cerr << "❌ Rich Presence update failed";
          }
        });
}

}