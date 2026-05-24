/*
 * .随机装备 — best random gear for quick-group/LFG service bots or self
 * By leewheel 20260523
 */

#include "ScriptMgr.h"
#include "Chat.h"
#include "Creature.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RBAC.h"

#include "bot_ai.h"
#include "botcommon.h"
#include "botconfig.h"
#include "botdatamgr.h"
#include "botmgr.h"

using namespace Trinity::ChatCommands;

namespace
{
bool IsServiceTempBot(Creature const* bot, Player const* owner)
{
    if (!bot || !owner || !bot->IsNPCBot())
        return false;

    bot_ai const* ai = bot->GetBotAI();
    if (!ai || ai->IAmFree() || bot->GetBotOwner() != owner)
        return false;

    return IsServiceHireSource(BotDataMgr::GetNpcBotHireSource(bot->GetEntry()));
}

bool CanPlayerUseProto(Player* player, ItemTemplate const* proto)
{
    if (!player || !proto)
        return false;

    Item* tempItem = Item::CreateItem(proto->ItemId, 1, player);
    if (!tempItem)
        return false;

    uint16 dest = 0;
    InventoryResult const msg = player->CanEquipItem(NULL_SLOT, dest, tempItem, false);
    delete tempItem;
    return msg == EQUIP_ERR_OK;
}

bool ApplyRandomEquipToPlayer(Player* player, ChatHandler* handler)
{
    uint8 const lvl = player->GetLevel();
    uint8 const playerClass = player->GetClass();
    uint8 const gen_category = BOT_GENERATED_DUNGEON;
    uint32 const max_item_level = BotCfg::GetBotWandererMaxItemLevel(lvl);
    uint32 equipped = 0;

    auto fit_check = [player, lvl](uint8 /*slot*/, ItemTemplate const* proto) {
        if (lvl >= 60 && proto->Quality < ITEM_QUALITY_EPIC)
            return false;
        return CanPlayerUseProto(player, proto);
    };

    for (uint8 i = 0; i < BOT_INVENTORY_SIZE; ++i)
    {
        if (i == BOT_SLOT_OFFHAND && (lvl < 10 && BotDataMgr::IsCastingClass(playerClass)))
            continue;
        if (i == BOT_SLOT_RANGED && !BotDataMgr::UsesGeneratedBotRangedSlot(playerClass))
            continue;
        if ((i == BOT_SLOT_FINGER1 || i == BOT_SLOT_FINGER2 || i == BOT_SLOT_NECK || i == BOT_SLOT_SHOULDERS) && lvl < 20)
            continue;
        if ((i == BOT_SLOT_TRINKET1 || i == BOT_SLOT_TRINKET2 || i == BOT_SLOT_HEAD) && lvl < 30)
            continue;

        Item* item = BotDataMgr::GenerateWanderingBotItem(gen_category, i, playerClass, lvl, max_item_level, fit_check);
        if (!item)
            continue;

        uint16 equipDest = 0;
        if (player->CanEquipItem(NULL_SLOT, equipDest, item, false) == EQUIP_ERR_OK)
        {
            player->EquipItem(equipDest, item, true);
            ++equipped;
            continue;
        }

        ItemPosCountVec storeDest;
        if (player->CanStoreItem(INVENTORY_SLOT_BAG_0, NULL_SLOT, storeDest, item) == EQUIP_ERR_OK)
        {
            player->StoreItem(storeDest, item, true);
            ++equipped;
        }
        else
            delete item;
    }

    if (!equipped)
    {
        handler->SendSysMessage("未能为你生成可装备的物品（等级/职业限制或物品池为空）。");
        return false;
    }

    handler->PSendSysMessage("已为你装备 %u 件当前等级最佳随机装备。", equipped);
    return true;
}

bool ApplyRandomEquipToServiceBot(Creature* bot, ChatHandler* handler)
{
    bot_ai* ai = bot->GetBotAI();
    if (!ai)
    {
        handler->SendSysMessage("机器人 AI 未加载。");
        return false;
    }

    ai->ApplyServiceRandomEquip();
    handler->PSendSysMessage("已为机器人 %s 应用当前等级最佳随机装备。", bot->GetName().c_str());
    return true;
}
}

class bot_random_equip_commandscript : public CommandScript
{
public:
    bot_random_equip_commandscript() : CommandScript("bot_random_equip_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "随机装备", HandleRandomEquipCommand, rbac::RBAC_PERM_COMMAND_QUICK_GROUP, Console::No },
        };
        return commandTable;
    }

    static bool HandleRandomEquipCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetPlayer();
        if (!player || !BotCfg::IsNpcBotModEnabled())
        {
            handler->SendSysMessage("NpcBots 模块未启用。");
            return false;
        }

        Unit* selected = player->GetSelectedUnit();

        if (!selected)
        {
            handler->SendSysMessage("请先左键选中你自己，或一个属于你的快速组队/LFG 临时工机器人。");
            return false;
        }

        if (selected == player || selected->GetTypeId() == TYPEID_PLAYER && selected->ToPlayer() == player)
            return ApplyRandomEquipToPlayer(player, handler);

        if (!selected->IsCreature())
        {
            handler->SendSysMessage("请选中你自己，或一个属于你的快速组队/LFG 临时工机器人。");
            return false;
        }

        Creature* bot = selected->ToCreature();
        if (!IsServiceTempBot(bot, player))
        {
            handler->SendSysMessage("该命令仅对快速组队或 LFG 招募的临时工机器人生效；普通花钱雇佣的机器人不可用。");
            return false;
        }

        return ApplyRandomEquipToServiceBot(bot, handler);
    }
};

void AddSC_bot_random_equip_commands()
{
    new bot_random_equip_commandscript();
}
