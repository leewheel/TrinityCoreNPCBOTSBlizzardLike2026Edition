/*
 * .超级菜单 — open Arcane Grimoire (SuperMenu client addon)
 */

#include "super_menu_addon.h"

#include "Chat.h"
#include "Common.h"
#include "Language.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

using namespace Trinity::ChatCommands;

class super_menu_commandscript : public CommandScript
{
public:
    super_menu_commandscript() : CommandScript("super_menu_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "超级菜单", HandleSuperMenuCommand, LANG_COMMAND_SUPER_MENU_HELP, rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "supermenu", HandleSuperMenuCommand, LANG_COMMAND_SUPER_MENU_HELP, rbac::RBAC_PERM_COMMAND_GM, Console::No },
        };
        return commandTable;
    }

    static bool HandleSuperMenuCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetPlayer();
        if (!player || !player->GetSession())
            return false;

        if (player->GetSession()->GetSecurity() < SEC_GAMEMASTER)
        {
            handler->SendSysMessage(LANG_YOU_NOT_HAVE_PERMISSION);
            handler->SetSentErrorMessage(true);
            return false;
        }

        SuperMenuAddon::OpenUI(player);
        return true;
    }
};

class super_menu_player_script : public PlayerScript
{
public:
    super_menu_player_script() : PlayerScript("super_menu_player_script") { }

    bool OnAddonMessage(Player* player, std::string_view message) override
    {
        return SuperMenuAddon::TryHandleIncoming(player, message);
    }
};

void AddSC_super_menu_commandscript()
{
    new super_menu_player_script();
    new super_menu_commandscript();
}
