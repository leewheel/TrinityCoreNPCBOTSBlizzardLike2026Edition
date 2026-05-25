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
#include "Containers.h"
#include "Creature.h"
#include "Group.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"

#include <algorithm>
#include <functional>
#include <set>
#include <vector>

using namespace Bcore::ChatCommands;

namespace BotQuickGroup
{
namespace
{
enum class PlayerPartySlot : uint8
{
    Tank,
    Healer,
    Dps
};

char const* PlayerPartySlotName(PlayerPartySlot slot)
{
    switch (slot)
    {
        case PlayerPartySlot::Tank:   return "坦克";
        case PlayerPartySlot::Healer: return "治疗";
        default:                    return "输出";
    }
}

PlayerPartySlot DetectPlayerPartySlot(Player* player)
{
    switch (player->GetClass())
    {
        case CLASS_WARRIOR:
            if (player->HasAura(71) || player->HasSpell(23922) || player->HasSpell(20243)) // Defensive / Shield Slam / Devastate
                return PlayerPartySlot::Tank;
            return PlayerPartySlot::Dps;
        case CLASS_DEATH_KNIGHT:
            if (player->HasAura(48263)) // Blood Presence
                return PlayerPartySlot::Tank;
            return PlayerPartySlot::Dps;
        case CLASS_PRIEST:
            if (player->HasAura(15473) || player->HasSpell(34914)) // Shadowform / Vampiric Touch
                return PlayerPartySlot::Dps;
            if (player->HasSpell(53007) || player->HasSpell(2060)) // Penance / Greater Heal
                return PlayerPartySlot::Healer;
            return PlayerPartySlot::Healer;
        case CLASS_SHAMAN:
            if (player->HasSpell(17364) || player->HasSpell(60103)) // Stormstrike / Lava Lash
                return PlayerPartySlot::Dps;
            if (player->HasSpell(974) || player->HasSpell(61295)) // Earth Shield / Riptide
                return PlayerPartySlot::Healer;
            return PlayerPartySlot::Healer;
        case CLASS_HUNTER:
        case CLASS_ROGUE:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return PlayerPartySlot::Dps;
        case CLASS_PALADIN:
            if (player->HasSpell(31935)) // Avenger's Shield
                return PlayerPartySlot::Tank;
            if (player->HasSpell(20473)) // Holy Shock
                return PlayerPartySlot::Healer;
            return PlayerPartySlot::Dps;
        case CLASS_DRUID:
        {
            ShapeshiftForm const form = player->GetShapeshiftForm();
            if (form == FORM_BEAR || form == FORM_DIREBEAR)
                return PlayerPartySlot::Tank;
            if (form == FORM_CAT)
                return PlayerPartySlot::Dps;
            if (player->HasAura(24858) || player->HasSpell(5176)) // Moonkin / Wrath
                return PlayerPartySlot::Dps;
            if (player->HasSpell(18562)) // Swiftmend
                return PlayerPartySlot::Healer;
            return PlayerPartySlot::Healer;
        }
        default:
            return PlayerPartySlot::Dps;
    }
}

// 5/10/25/40 共用：按玩家职责从编制中扣减（团本含副坦位）
void ApplyPlayerSlotToComposition(PlayerPartySlot playerSlot, uint8& tanksNeeded, uint8& offTanksNeeded, uint8& healersNeeded, uint8& dpsNeeded)
{
    switch (playerSlot)
    {
        case PlayerPartySlot::Tank:
            if (tanksNeeded)
                --tanksNeeded;
            else if (offTanksNeeded)
                --offTanksNeeded;
            break;
        case PlayerPartySlot::Healer:
            if (healersNeeded)
                --healersNeeded;
            break;
        case PlayerPartySlot::Dps:
            if (dpsNeeded)
                --dpsNeeded;
            break;
    }
}

void ApplyRandomEquipToQuickGroupBots(BotMgr* mgr)
{
    if (!mgr)
        return;

    for (auto const& [guid, bot] : *mgr->GetBotMap())
    {
        if (!bot || !bot->IsNPCBot() || !bot->GetBotAI())
            continue;
        if (BotDataMgr::GetNpcBotHireSource(bot->GetEntry()) != NPCBOT_HIRE_QUICK_GROUP)
            continue;
        bot->GetBotAI()->ApplyServiceRandomEquip();
    }
}

bool HireBotForRole(Player* player, BotMgr* mgr, uint32 botRole, std::set<uint8>& usedClasses, std::function<bool(uint8)> const& classOk, bool uniqueClassPerRole)
{
    static std::set<uint8> const emptyUsed;

    std::vector<uint8> candidates;
    candidates.reserve(16);

    for (uint8 botClass = BOT_CLASS_WARRIOR; botClass < BOT_CLASS_END; ++botClass)
    {
        if (!classOk(botClass))
            continue;

        std::set<uint8> const& classFilter = uniqueClassPerRole ? usedClasses : emptyUsed;
        if (classFilter.count(botClass))
            continue;

        if (BotDataMgr::FindFreeHireBotForQuickGroup(player, botClass, classFilter))
            candidates.push_back(botClass);
    }

    if (candidates.empty())
        return false;

    Bcore::Containers::RandomShuffle(candidates);

    for (uint8 botClass : candidates)
    {
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

uint8 HireRoleSlots(Player* player, BotMgr* mgr, uint8 slotsNeeded, uint32 botRole, uint8& botsToHire,
    std::set<uint8>& usedClasses, std::function<bool(uint8)> const& classOk, bool uniqueClassPerRole)
{
    uint8 hired = 0;
    uint8 failStreak = 0;
    uint8 const maxFails = uint8(std::min<uint32>(slotsNeeded * 8u, 32u));

    while (slotsNeeded > 0 && botsToHire > 0 && failStreak < maxFails)
    {
        if (!HireBotForRole(player, mgr, botRole, usedClasses, classOk, uniqueClassPerRole))
        {
            ++failStreak;
            continue;
        }

        --slotsNeeded;
        --botsToHire;
        ++hired;
        failStreak = 0;
    }

    return hired;
}
}

bool Fill(Player* player, uint8 partySize, uint8 tanksNeeded, uint8 offTanksNeeded, uint8 healersNeeded, uint8 dpsNeeded)
{
    if (!player || !BotCfg::IsNpcBotModEnabled())
        return false;

    BotMgr* mgr = player->GetBotMgr();
    if (!mgr)
        return false;

    PlayerPartySlot const playerSlot = DetectPlayerPartySlot(player);

    std::set<uint8> usedClasses;
    usedClasses.insert(player->GetClass());

    uint8 const targetBots = partySize > 0 ? partySize - 1 : 0;
    bool const uniqueClassPerRole = partySize < 10;
    bool const raidSize = partySize >= 10;

    uint8 botTanks = tanksNeeded;
    uint8 botOffTanks = offTanksNeeded;
    uint8 botHeals = healersNeeded;
    uint8 botDps = dpsNeeded;
    ApplyPlayerSlotToComposition(playerSlot, botTanks, botOffTanks, botHeals, botDps);

    uint8 const slotsToFill = botTanks + botOffTanks + botHeals + botDps;
    uint8 const headroom = targetBots > player->GetNpcBotsCount() ? targetBots - player->GetNpcBotsCount() : 0;
    uint8 botsToHire = std::min(headroom, slotsToFill);

    ChatHandler ch(player->GetSession());
    ch.PSendSysMessage(
        "队伍编制 %u 人（含你）：%u坦 %u副坦 %u治疗 %u输出 | 你担任：%s | 将招募 Bot：%u坦 %u副坦 %u治疗 %u输出",
        partySize, tanksNeeded, offTanksNeeded, healersNeeded, dpsNeeded,
        PlayerPartySlotName(playerSlot), botTanks, botOffTanks, botHeals, botDps);

    if (!botsToHire)
    {
        ch.SendSysMessage("你的机器人数量已满足该规模队伍。");
        return true;
    }

    uint8 const hiredTanks = HireRoleSlots(player, mgr, botTanks, BOT_ROLE_TANK, botsToHire, usedClasses,
        [player](uint8 c) {
            return c == BOT_CLASS_WARRIOR || c == BOT_CLASS_PALADIN ||
                (c == BOT_CLASS_DEATH_KNIGHT && player->GetLevel() >= 55);
        }, uniqueClassPerRole);

    uint8 const hiredOffTanks = HireRoleSlots(player, mgr, botOffTanks, BOT_ROLE_TANK_OFF, botsToHire, usedClasses,
        [player](uint8 c) {
            return c == BOT_CLASS_WARRIOR || c == BOT_CLASS_PALADIN || c == BOT_CLASS_DRUID ||
                (c == BOT_CLASS_DEATH_KNIGHT && player->GetLevel() >= 55);
        }, uniqueClassPerRole);

    uint8 const hiredHealers = HireRoleSlots(player, mgr, botHeals, BOT_ROLE_HEAL, botsToHire, usedClasses,
        [](uint8 c) {
            return c == BOT_CLASS_PALADIN || c == BOT_CLASS_PRIEST || c == BOT_CLASS_SHAMAN || c == BOT_CLASS_DRUID;
        }, uniqueClassPerRole);

    uint8 const hiredDps = HireRoleSlots(player, mgr, botDps, BOT_ROLE_DPS, botsToHire, usedClasses,
        [player, raidSize](uint8 c) {
            if (raidSize && (c == BOT_CLASS_WARRIOR || c == BOT_CLASS_PALADIN))
                return false;
            return c == BOT_CLASS_HUNTER || c == BOT_CLASS_ROGUE || c == BOT_CLASS_MAGE || c == BOT_CLASS_WARLOCK ||
                c == BOT_CLASS_PRIEST || c == BOT_CLASS_SHAMAN || c == BOT_CLASS_DRUID ||
                (c == BOT_CLASS_DEATH_KNIGHT && player->GetLevel() >= 55) ||
                (!raidSize && (c == BOT_CLASS_WARRIOR || c == BOT_CLASS_PALADIN));
        }, uniqueClassPerRole);

    for (auto const& [guid, bot] : *mgr->GetBotMap())
    {
        if (bot)
            mgr->AddBotToGroup(bot);
    }

    ApplyRandomEquipToQuickGroupBots(mgr);

    if (botsToHire)
        ch.PSendSysMessage("未能招满机器人（可雇池不足）。剩余缺口约 %u 名。", botsToHire);

    ch.PSendSysMessage("招募完成: T:%u OT:%u 治疗:%u DPS:%u | 当前跟随 %u",
        hiredTanks, hiredOffTanks, hiredHealers, hiredDps, uint32(player->GetNpcBotsCount()));

    return true;
}

// 5 人队：1 坦 1 奶 3 输出 + 玩家（玩家通常占 1 个输出位）
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
