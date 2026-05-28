/*
 * Bot Manager UI addon protocol (BMU) - C++ server handler
 * By leewheel 20260520
 * Replaces Eluna BotManagerUI_Server.lua for client NPCBotInventory addon.
 */

#ifndef BOT_MANAGER_ADDON_H
#define BOT_MANAGER_ADDON_H

#include <string_view>

class Player;

namespace BotManagerAddon
{
    inline constexpr std::string_view PREFIX = "BMU";

    // Returns true if the message was handled (caller should not forward the whisper).
    bool TryHandleIncoming(Player* player, std::string_view message);

    // Push one bot snapshot to the client addon (QUERY flow, no UI open).
    void SendBotSnapshot(Player* player, uint32 botEntry);

    // By leewheel 20260528 - push snapshot then U;OPEN so client opens inspect UI (gossip menu).
    void SendBotSnapshotAndOpenUI(Player* player, uint32 botEntry);
}

#endif // BOT_MANAGER_ADDON_H
