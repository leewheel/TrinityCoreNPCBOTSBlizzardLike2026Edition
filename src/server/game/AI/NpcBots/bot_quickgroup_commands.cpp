/*
 * Quick group NPCBot commands — top-level + shared handlers for .npcbot quick5
 * By leewheel 20260523
 */

#include "bot_quickgroup.h"

#include "bot_ai.h"
#include "botcommon.h"
#include "botconfig.h"
#include "botdatamgr.h"
#include "botmgr.h"
#include "Chat.h"
#include "Creature.h"
#include "Group.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"

#include <functional>
#include <set>

using namespace Bcore::ChatCommands;

namespace BotQuickGroup
{
namespace
{
void DetectPlayerRole(Player* player, bool& isTank, bool& isHealer, bool& isDPS)
{
    isTank = isHealer = isDPS = false;
    switch (player->GetClass())
    {
        case CLASS_WARRIOR:
        case CLASS_DEATH_KNIGHT:
            isTank = true;
            break;
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
            isHealer = true;
            break;
        case CLASS_PALADIN:
        case CLASS_DRUID:
            isTank = true;
            isHealer = true;
            break;
        default:
            isDPS = true;
            break;
    }
}

bool HireBotForRole(Player* player, BotMgr* mgr, uint32 botRole, std::set<uint8>& usedClasses, std::function<bool(uint8)> const& classOk, bool uniqueClassPerRole)
{
    static std::set<uint8> const emptyUsed;

    for (uint8 botClass = BOT_CLASS_WARRIOR; botClass < BOT_CLASS_END; ++botClass)
    {
        if (!classOk(botClass))
            continue;

        std::set<uint8> const& classFilter = uniqueClassPerRole ? usedClasses : emptyUsed;
        Creature* bot = BotDataMgr::FindFreeHireBotForQuickGroup(player, botClass, classFilter);
        if (!bot)
            continue;

        if (mgr->AddServiceBot(bot, botRole) != BOT_ADD_SUCCESS)
            continue;

        if (uniqueClassPerRole)
            usedClasses.insert(bot->GetBotClass());
        mgr->AddBotToGroup(bot);
        return true;
    }

    return false;
}
}

bool Fill(Player* player, uint8 partySize, uint8 tanksNeeded, uint8 offTanksNeeded, uint8 healersNeeded, uint8 dpsNeeded)
{
    if (!player || !BotCfg::IsNpcBotModEnabled())
        return false;

    BotMgr* mgr = player->GetBotMgr();
    if (!mgr)
        return false;

    bool isTank = false, isHealer = false, isDPS = false;
    DetectPlayerRole(player, isTank, isHealer, isDPS);

    std::set<uint8> usedClasses;
    usedClasses.insert(player->GetClass());

    uint8 const targetBots = partySize > 0 ? partySize - 1 : 0;
    uint8 botsToHire = targetBots > player->GetNpcBotsCount() ? targetBots - player->GetNpcBotsCount() : 0;
    bool const uniqueClassPerRole = partySize < 10;

    if (isTank && tanksNeeded) { --tanksNeeded; --botsToHire; }
    if (isHealer && healersNeeded) { --healersNeeded; --botsToHire; }
    if (isDPS || (!isTank && !isHealer)) { if (dpsNeeded) { --dpsNeeded; --botsToHire; } }

    if (!botsToHire)
    {
        ChatHandler(player->GetSession()).SendSysMessage("你的机器人数量已满足该规模队伍。");
        return true;
    }

    while (botsToHire > 0 && tanksNeeded > 0)
    {
        if (!HireBotForRole(player, mgr, BOT_ROLE_TANK, usedClasses, [player](uint8 c) {
            return c == BOT_CLASS_WARRIOR || c == BOT_CLASS_PALADIN ||
                (c == BOT_CLASS_DEATH_KNIGHT && player->GetLevel() >= 55);
        }, uniqueClassPerRole))
            break;
        --tanksNeeded;
        --botsToHire;
    }

    while (botsToHire > 0 && offTanksNeeded > 0)
    {
        if (!HireBotForRole(player, mgr, BOT_ROLE_TANK_OFF, usedClasses, [player](uint8 c) {
            return c == BOT_CLASS_WARRIOR || c == BOT_CLASS_PALADIN || c == BOT_CLASS_DRUID ||
                (c == BOT_CLASS_DEATH_KNIGHT && player->GetLevel() >= 55);
        }, uniqueClassPerRole))
            break;
        --offTanksNeeded;
        --botsToHire;
    }

    while (botsToHire > 0 && healersNeeded > 0)
    {
        if (!HireBotForRole(player, mgr, BOT_ROLE_HEAL, usedClasses, [](uint8 c) {
            return c == BOT_CLASS_PALADIN || c == BOT_CLASS_PRIEST || c == BOT_CLASS_SHAMAN || c == BOT_CLASS_DRUID;
        }, uniqueClassPerRole))
            break;
        --healersNeeded;
        --botsToHire;
    }

    while (botsToHire > 0 && dpsNeeded > 0)
    {
        if (!HireBotForRole(player, mgr, BOT_ROLE_DPS, usedClasses, [player](uint8 c) {
            return c == BOT_CLASS_WARRIOR || c == BOT_CLASS_PALADIN || c == BOT_CLASS_HUNTER || c == BOT_CLASS_ROGUE ||
                c == BOT_CLASS_PRIEST || c == BOT_CLASS_SHAMAN || c == BOT_CLASS_MAGE || c == BOT_CLASS_WARLOCK ||
                c == BOT_CLASS_DRUID || (c == BOT_CLASS_DEATH_KNIGHT && player->GetLevel() >= 55);
        }, uniqueClassPerRole))
            break;
        --dpsNeeded;
        --botsToHire;
    }

    for (auto const& [guid, bot] : *mgr->GetBotMap())
    {
        if (bot)
            mgr->AddBotToGroup(bot);
    }

    if (botsToHire)
        ChatHandler(player->GetSession()).PSendSysMessage("未能招满机器人（全服可雇池不足或职业重复）。剩余缺口约 %u 名。", botsToHire);

    return true;
}

bool Handle5(Player* player) { return Fill(player, 5, 1, 0, 1, 3); }

bool Handle10(Player* player)
{
    if (player->GetLevel() < 60)
    {
        ChatHandler(player->GetSession()).SendSysMessage("等级需达到 60 才能使用 10 人团命令。");
        return false;
    }
    return Fill(player, 10, 1, 1, 2, 5);
}

bool Handle25(Player* player)
{
    if (player->GetLevel() < 60)
    {
        ChatHandler(player->GetSession()).SendSysMessage("等级需达到 60 才能使用 25 人团命令。");
        return false;
    }
    return Fill(player, 25, 2, 2, 5, 14);
}

bool Handle40(Player* player)
{
    if (player->GetLevel() < 60)
    {
        ChatHandler(player->GetSession()).SendSysMessage("等级需达到 60 才能使用 40 人团命令。");
        return false;
    }
    return Fill(player, 40, 4, 4, 10, 22);
}

bool HandleDissolve(Player* player)
{
    BotMgr* mgr = player->GetBotMgr();
    if (!mgr)
        return false;

    uint8 const before = player->GetNpcBotsCount();
    mgr->DismissQuickGroupBots();

    ChatHandler ch(player->GetSession());
    if (before == player->GetNpcBotsCount())
        ch.SendSysMessage("没有可解散的快速组队机器人（仅解雇 hire_source=快速组队 的 Bot）。");
    else
        ch.SendSysMessage("已解散快速组队机器人，随机装备已销毁且未进入你的背包。");

    return true;
}

bool Handle5Cmd(ChatHandler* handler, char const* /*args*/) { return Handle5(handler->GetPlayer()); }
bool Handle10Cmd(ChatHandler* handler, char const* /*args*/) { return Handle10(handler->GetPlayer()); }
bool Handle25Cmd(ChatHandler* handler, char const* /*args*/) { return Handle25(handler->GetPlayer()); }
bool Handle40Cmd(ChatHandler* handler, char const* /*args*/) { return Handle40(handler->GetPlayer()); }
bool HandleDissolveCmd(ChatHandler* handler, char const* /*args*/) { return HandleDissolve(handler->GetPlayer()); }
}

class script_bot_quickgroup_commands : public CommandScript
{
public:
    script_bot_quickgroup_commands() : CommandScript("script_bot_quickgroup_commands") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable table =
        {
            // ASCII names (recommended — always registered correctly)
            { "quick5",        BotQuickGroup::Handle5Cmd,        rbac::RBAC_PERM_COMMAND_QUICK_GROUP, Console::No },
            { "quick10",       BotQuickGroup::Handle10Cmd,       rbac::RBAC_PERM_COMMAND_QUICK_GROUP, Console::No },
            { "quick25",       BotQuickGroup::Handle25Cmd,       rbac::RBAC_PERM_COMMAND_QUICK_GROUP, Console::No },
            { "quick40",       BotQuickGroup::Handle40Cmd,       rbac::RBAC_PERM_COMMAND_QUICK_GROUP, Console::No },
            { "quickdissolve", BotQuickGroup::HandleDissolveCmd, rbac::RBAC_PERM_COMMAND_QUICK_GROUP, Console::No },
            { "5人队",         BotQuickGroup::Handle5Cmd,        rbac::RBAC_PERM_COMMAND_QUICK_GROUP, Console::No },
            { "10人团",        BotQuickGroup::Handle10Cmd,       rbac::RBAC_PERM_COMMAND_QUICK_GROUP, Console::No },
            { "25人团",        BotQuickGroup::Handle25Cmd,       rbac::RBAC_PERM_COMMAND_QUICK_GROUP, Console::No },
            { "40人团",        BotQuickGroup::Handle40Cmd,       rbac::RBAC_PERM_COMMAND_QUICK_GROUP, Console::No },
            { "解散队伍",      BotQuickGroup::HandleDissolveCmd, rbac::RBAC_PERM_COMMAND_QUICK_GROUP, Console::No },
        };
        return table;
    }
};

void AddSC_bot_quickgroup_commands()
{
    new script_bot_quickgroup_commands();
}
