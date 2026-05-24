/*
 * Bot Manager UI addon protocol (BMU) - C++ server handler
 * By leewheel 20260520
 */

#include "bot_manager_addon.h"

#include "Bag.h"
#include "Chat.h"
#include "ChatPackets.h"
#include "Creature.h"
#include "CreatureData.h"
#include "Item.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Util.h"
#include "WorldSession.h"
#include "bot_ai.h"
#include "botcommon.h"
#include "botdatamgr.h"
#include "botmgr.h"

#include <sstream>
#include <unordered_map>
#include <vector>

namespace
{
    char const* const SLOT_KEYS[BOT_INVENTORY_SIZE] =
    {
        "MAINHAND", "OFFHAND", "RANGED", "HEAD", "SHOULDER", "CHEST", "WAIST", "LEGS", "FEET",
        "WRIST", "HANDS", "BACK", "BODY", "FINGER1", "FINGER2", "TRINKET1", "TRINKET2", "NECK"
    };

    std::unordered_map<std::string_view, uint8> const SlotKeyToIndex = []
    {
        std::unordered_map<std::string_view, uint8> map;
        for (uint8 i = 0; i < BOT_INVENTORY_SIZE; ++i)
            map.emplace(SLOT_KEYS[i], i);
        return map;
    }();

    bool TryStripAddonPrefix(std::string_view& message, std::string_view prefix)
    {
        // Client may send "\tBMU\tpayload" or "BMU\tpayload"
        if (message.starts_with('\t'))
        {
            size_t secondTab = message.find('\t', 1);
            if (secondTab == std::string_view::npos)
                return false;

            if (message.substr(1, secondTab - 1) != prefix)
                return false;

            message = message.substr(secondTab + 1);
            return true;
        }

        if (!message.starts_with(prefix))
            return false;

        if (message.size() == prefix.size())
        {
            message = {};
            return true;
        }

        if (message[prefix.size()] != '\t')
            return false;

        message = message.substr(prefix.size() + 1);
        return true;
    }

    std::vector<std::string_view> SplitSemicolon(std::string_view message)
    {
        std::vector<std::string_view> parts;
        while (!message.empty())
        {
            size_t pos = message.find(';');
            if (pos == std::string_view::npos)
            {
                parts.push_back(message);
                break;
            }
            parts.push_back(message.substr(0, pos));
            message.remove_prefix(pos + 1);
        }
        return parts;
    }

    void SendBMU(Player* player, std::string const& payload)
    {
        WorldPackets::Chat::Chat packet;
        packet.Initialize(CHAT_MSG_WHISPER, LANG_ADDON, player, player, payload, 0, "", LOCALE_enUS, std::string(BotManagerAddon::PREFIX));
        player->SendDirectMessage(packet.Write());
    }

    bool PlayerOwnsBot(Player const* player, uint32 entry)
    {
        NpcBotData const* data = BotDataMgr::SelectNpcBotData(entry);
        if (!data)
            return false;

        uint32 const ownerLow = player->GetGUID().GetCounter();
        if (data->owner == ownerLow)
            return true;

        return data->shared_owners.contains(ownerLow);
    }

    std::string GetBotClassDisplayName(uint8 botClass)
    {
        switch (botClass)
        {
            case BOT_CLASS_BM: return "Blademaster";
            case BOT_CLASS_SPHYNX: return "Sphynx";
            case BOT_CLASS_ARCHMAGE: return "Archmage";
            case BOT_CLASS_DREADLORD: return "Dreadlord";
            case BOT_CLASS_SPELLBREAKER: return "Spellbreaker";
            case BOT_CLASS_DARK_RANGER: return "Dark Ranger";
            case BOT_CLASS_NECROMANCER: return "Necromancer";
            case BOT_CLASS_SEA_WITCH: return "Sea Witch";
            case BOT_CLASS_CRYPT_LORD: return "Crypt Lord";
            default:
                break;
        }

        uint8 const playerClass = BotMgr::GetBotPlayerClass(botClass);
        if (playerClass != CLASS_NONE)
            return GetClassName(playerClass, DEFAULT_LOCALE);

        return "Bot";
    }

    std::string EscapeAddonField(std::string_view value)
    {
        std::string out(value);
        for (char& c : out)
        {
            if (c == ';')
                c = ',';
        }
        return out;
    }

    uint32 ResolveItemEntry(uint32 itemGuid, std::unordered_map<uint32, uint32> const& instanceMap)
    {
        if (!itemGuid)
            return 0;

        if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemGuid))
            return itemGuid;

        auto itr = instanceMap.find(itemGuid);
        if (itr != instanceMap.end())
            return itr->second;

        return itemGuid;
    }

    void CollectItemGuids(NpcBotData const* data, std::vector<uint32>& guids)
    {
        for (uint32 itemGuid : data->equips)
        {
            if (!itemGuid)
                continue;
            if (sObjectMgr->GetItemTemplate(itemGuid))
                continue;
            guids.push_back(itemGuid);
        }
    }

    std::unordered_map<uint32, uint32> LoadItemInstanceMap(std::vector<uint32> const& guids)
    {
        std::unordered_map<uint32, uint32> instanceMap;
        if (guids.empty())
            return instanceMap;

        std::ostringstream ss;
        for (size_t i = 0; i < guids.size(); ++i)
        {
            if (i)
                ss << ',';
            ss << guids[i];
        }

        if (QueryResult result = CharacterDatabase.PQuery("SELECT guid, itemEntry FROM item_instance WHERE guid IN (%s)", ss.str().c_str()))
        {
            do
            {
                Field* fields = result->Fetch();
                instanceMap.emplace(fields[0].GetUInt32(), fields[1].GetUInt32());
            } while (result->NextRow());
        }

        return instanceMap;
    }

    void SendBotStats(Player* player, uint32 entry)
    {
        QueryResult result = CharacterDatabase.PQuery(
            "SELECT entry, maxhealth, maxpower, strength, agility, stamina, intellect, spirit, armor, defense, "
            "resHoly, resFire, resNature, resFrost, resShadow, resArcane, blockPct, dodgePct, parryPct, critPct, "
            "attackPower, spellPower, spellPen, hastePct, hitBonusPct, expertise, armorPenPct "
            "FROM characters_npcbot_stats WHERE entry = %u", entry);

        if (!result)
            return;

        Field* fields = result->Fetch();
        std::ostringstream ss;
        ss << "S;" << entry
           << ';' << fields[1].GetUInt32()  // hp
           << ';' << fields[2].GetUInt32()  // mp
           << ';' << fields[3].GetUInt32()  // str
           << ';' << fields[4].GetUInt32()  // agi
           << ';' << fields[5].GetUInt32()  // sta
           << ';' << fields[6].GetUInt32()  // int
           << ';' << fields[7].GetUInt32()  // spi
           << ';' << fields[8].GetUInt32()  // armor
           << ';' << fields[9].GetUInt32()  // def
           << ';' << fields[10].GetUInt32() // resHoly
           << ';' << fields[11].GetUInt32() // resFire
           << ';' << fields[12].GetUInt32() // resNature
           << ';' << fields[13].GetUInt32() // resFrost
           << ';' << fields[14].GetUInt32() // resShadow
           << ';' << fields[15].GetUInt32() // resArcane
           << ';' << fields[16].GetFloat()  // block
           << ';' << fields[17].GetFloat()  // dodge
           << ';' << fields[18].GetFloat()  // parry
           << ';' << fields[19].GetFloat()  // crit
           << ';' << fields[20].GetUInt32() // ap
           << ';' << fields[21].GetUInt32() // sp
           << ';' << fields[22].GetUInt32() // spellPen
           << ';' << fields[23].GetFloat()  // haste
           << ';' << fields[24].GetFloat()  // hit
           << ';' << fields[25].GetUInt32() // expertise
           << ';' << fields[26].GetFloat(); // arpen

        SendBMU(player, ss.str());
    }

    void SendBotGear(Player* player, uint32 entry, NpcBotData const* data, std::unordered_map<uint32, uint32> const& instanceMap)
    {
        for (uint8 slot = 0; slot < BOT_INVENTORY_SIZE; ++slot)
        {
            uint32 const itemEntry = ResolveItemEntry(data->equips[slot], instanceMap);
            std::ostringstream ss;
            ss << "G;" << entry << ';' << SLOT_KEYS[slot] << ';' << itemEntry;
            SendBMU(player, ss.str());
        }
    }

    void SendBotMeta(Player* player, uint32 entry, NpcBotData const* data)
    {
        CreatureTemplate const* proto = sObjectMgr->GetCreatureTemplate(entry);
        if (!proto)
            return;

        std::string botName = proto->Name;
        if (std::string_view customName = BotDataMgr::GetNpcBotAppearanceName(entry); !customName.empty())
            botName = std::string(customName);

        uint32 displayId = proto->GetFirstValidModelId();
        uint32 race = 0;
        uint8 gender = 0;
        std::string className = "Bot";

        if (NpcBotExtras const* extras = BotDataMgr::SelectNpcBotExtras(entry))
        {
            race = extras->race;
            className = GetBotClassDisplayName(extras->bclass);
        }
        else if (!proto->Title.empty())
        {
            className = proto->Title;
            if (className.ends_with(" Bot"))
                className.resize(className.size() - 4);
        }

        if (NpcBotAppearanceData const* appearance = BotDataMgr::SelectNpcBotAppearance(entry))
            gender = appearance->gender;

        std::ostringstream ss;
        ss << "B;" << entry << ';' << EscapeAddonField(botName) << ';' << data->roles << ';' << EscapeAddonField(className)
           << ';' << displayId << ';' << race << ';' << uint32(gender) << ';' << player->GetLevel() << ';' << uint32(data->spec);

        SendBMU(player, ss.str());
    }

    void SendBotData(Player* player, uint32 entry)
    {
        NpcBotData const* data = BotDataMgr::SelectNpcBotData(entry);
        if (!data || !PlayerOwnsBot(player, entry))
            return;

        std::vector<uint32> guids;
        CollectItemGuids(data, guids);
        std::unordered_map<uint32, uint32> instanceMap = LoadItemInstanceMap(guids);

        SendBotMeta(player, entry, data);
        SendBotGear(player, entry, data, instanceMap);
        SendBotStats(player, entry);
    }

    void SendOwnedBots(Player* player, std::vector<uint32> const& entries)
    {
        if (entries.empty())
        {
            SendBMU(player, "E;END");
            return;
        }

        std::vector<uint32> allGuids;
        for (uint32 entry : entries)
        {
            NpcBotData const* data = BotDataMgr::SelectNpcBotData(entry);
            if (!data || !PlayerOwnsBot(player, entry))
                continue;

            SendBotMeta(player, entry, data);
            CollectItemGuids(data, allGuids);
        }

        std::unordered_map<uint32, uint32> instanceMap = LoadItemInstanceMap(allGuids);

        for (uint32 entry : entries)
        {
            NpcBotData const* data = BotDataMgr::SelectNpcBotData(entry);
            if (!data || !PlayerOwnsBot(player, entry))
                continue;

            SendBotGear(player, entry, data, instanceMap);
            SendBotStats(player, entry);
        }

        SendBMU(player, "E;END");
    }

    std::vector<uint32> LoadOwnedBotEntries(Player const* player)
    {
        std::vector<uint32> entries;
        uint32 const ownerLow = player->GetGUID().GetCounter();

        if (QueryResult result = CharacterDatabase.PQuery("SELECT entry FROM characters_npcbot WHERE owner = %u", ownerLow))
        {
            do
                entries.push_back(result->Fetch()[0].GetUInt32());
            while (result->NextRow());
        }

        return entries;
    }

    std::optional<uint8> SlotKeyToBotSlot(std::string_view slotKey)
    {
        auto itr = SlotKeyToIndex.find(slotKey);
        if (itr == SlotKeyToIndex.end())
            return std::nullopt;
        return itr->second;
    }

    void HandleRefresh(Player* player)
    {
        SendOwnedBots(player, LoadOwnedBotEntries(player));
    }

    void HandleQuery(Player* player, uint32 entry)
    {
        if (!entry || !PlayerOwnsBot(player, entry))
        {
            SendBMU(player, "E;END");
            return;
        }

        SendBotData(player, entry);
        SendBMU(player, "E;END");
    }

    Item* FindPlayerItemByEntry(Player* player, uint32 itemEntry)
    {
        if (!player || !itemEntry)
            return nullptr;

        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                if (item->GetEntry() == itemEntry)
                    return item;
        }

        for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {
            if (Bag const* bag = player->GetBagByPos(i))
            {
                for (uint32 j = 0; j < bag->GetBagSize(); ++j)
                {
                    if (Item* item = player->GetItemByPos(i, j))
                        if (item->GetEntry() == itemEntry)
                            return item;
                }
            }
        }

        return nullptr;
    }

    Creature* FindSummonedBotByEntry(Player* player, uint32 botEntry)
    {
        if (!player || !player->GetBotMgr())
            return nullptr;

        for (auto const& [guid, bot] : *player->GetBotMgr()->GetBotMap())
        {
            if (bot && bot->GetEntry() == botEntry)
                return bot;
        }

        return nullptr;
    }

    void NotifyEquipResult(Player* player, BotEquipResult result)
    {
        if (!player || result == BotEquipResult::BOT_EQUIP_RESULT_OK)
            return;

        ChatHandler handler(player->GetSession());
        switch (result)
        {
            case BotEquipResult::BOT_EQUIP_RESULT_FAIL_NO_ITEM:
                handler.SendSysMessage("[机器人管理] 背包中没有该物品。");
                break;
            case BotEquipResult::BOT_EQUIP_RESULT_FAIL_CANT_EQUIP:
            case BotEquipResult::BOT_EQUIP_RESULT_FAIL_ITEM_CONFLICT:
                handler.SendSysMessage("[机器人管理] 该机器人无法装备此物品或栏位冲突。");
                break;
            case BotEquipResult::BOT_EQUIP_RESULT_FAIL_SAME_ID:
                handler.SendSysMessage("[机器人管理] 该栏位已装备相同物品。");
                break;
            case BotEquipResult::BOT_EQUIP_RESULT_FAIL_NO_BANK_SPACE:
                handler.SendSysMessage("[机器人管理] 机器人银行空间不足。");
                break;
            case BotEquipResult::BOT_EQUIP_RESULT_FAIL_WANDERER:
                handler.SendSysMessage("[机器人管理] 无法修改流浪机器人装备。");
                break;
            default:
                handler.SendSysMessage("[机器人管理] 换装失败，请确认机器人已召唤在身边。");
                break;
        }
    }

    bool ApplyEquipChange(Player* player, uint32 botEntry, uint8 slot, uint32 itemEntry, bool unequip)
    {
        Creature* bot = FindSummonedBotByEntry(player, botEntry);
        if (!bot || !bot->GetBotAI())
        {
            ChatHandler(player->GetSession()).SendSysMessage("[机器人管理] 请先将该机器人召唤到身边后再换装。");
            return false;
        }

        BotEquipResult result;
        if (unequip)
            result = bot->GetBotAI()->UnequipSlotToPlayer(slot);
        else
        {
            Item* item = FindPlayerItemByEntry(player, itemEntry);
            if (!item)
            {
                NotifyEquipResult(player, BotEquipResult::BOT_EQUIP_RESULT_FAIL_NO_ITEM);
                return false;
            }
            result = bot->GetBotAI()->EquipItemFromPlayer(slot, item);
        }

        if (result != BotEquipResult::BOT_EQUIP_RESULT_OK)
        {
            NotifyEquipResult(player, result);
            return false;
        }

        return true;
    }

    void HandleEquip(Player* player, uint32 botEntry, uint32 itemId, std::string_view slotKey)
    {
        if (!botEntry || !itemId || !PlayerOwnsBot(player, botEntry))
            return;

        std::optional<uint8> slot = SlotKeyToBotSlot(slotKey);
        if (!slot)
            return;

        if (ApplyEquipChange(player, botEntry, *slot, itemId, false))
            SendBMU(player, "E;REFRESH");
    }

    void HandleUnequip(Player* player, uint32 botEntry, std::string_view slotKey)
    {
        if (!botEntry || !PlayerOwnsBot(player, botEntry))
            return;

        std::optional<uint8> slot = SlotKeyToBotSlot(slotKey);
        if (!slot)
            return;

        if (ApplyEquipChange(player, botEntry, *slot, 0, true))
            SendBMU(player, "E;REFRESH");
    }
}

void BotManagerAddon::SendBotSnapshot(Player* player, uint32 botEntry)
{
    if (!player || !botEntry)
        return;

    HandleQuery(player, botEntry);
}

bool BotManagerAddon::TryHandleIncoming(Player* player, std::string_view message)
{
    if (!player)
        return false;

    if (!TryStripAddonPrefix(message, PREFIX))
        return false;

    std::vector<std::string_view> parts = SplitSemicolon(message);
    if (parts.empty())
        return true;

    std::string_view const cmd = parts[0];

    if (cmd == "REFRESH")
        HandleRefresh(player);
    else if (cmd == "QUERY" && parts.size() >= 2)
        HandleQuery(player, Bcore::StringTo<uint32>(parts[1]).value_or(0));
    else if (cmd == "EQUIP" && parts.size() >= 4)
        HandleEquip(player, Bcore::StringTo<uint32>(parts[1]).value_or(0), Bcore::StringTo<uint32>(parts[2]).value_or(0), parts[3]);
    else if (cmd == "UNEQUIP" && parts.size() >= 3)
        HandleUnequip(player, Bcore::StringTo<uint32>(parts[1]).value_or(0), parts[2]);

    return true;
}
