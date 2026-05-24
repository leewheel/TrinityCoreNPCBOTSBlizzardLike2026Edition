/*
 * SuperMenu (SMU) — GM debug addon protocol
 */

#ifndef SUPER_MENU_ADDON_H
#define SUPER_MENU_ADDON_H

#include <string_view>

class Player;

namespace SuperMenuAddon
{
    inline constexpr std::string_view PREFIX = "SMU";

    void SendToClient(Player* player, std::string const& payload);
    bool TryHandleIncoming(Player* player, std::string_view message);
    void OpenUI(Player* player);
}

#endif
