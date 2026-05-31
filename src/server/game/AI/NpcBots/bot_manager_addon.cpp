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
#include "ObjectMgr.h"
#include "Player.h"
#include "Util.h"
#include "WorldSession.h"
#include "bot_ai.h"
#include "botcommon.h"
#include "botdatamgr.h"
#include "botmgr.h"
#include "Log.h"

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
        // 3.3.5 客户端 CHAT_MSG_ADDON：arg1=前缀，arg2=正文（BMU\tpayload，勿用 \tBMU\t）
        WorldPackets::Chat::Chat packet;
        packet.Initialize(CHAT_MSG_WHISPER, LANG_ADDON, player, player, payload, 0, "", DEFAULT_LOCALE, std::string(BotManagerAddon::PREFIX));
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

    std::string GetBotClassDisplayName(uint8 botClass, LocaleConstant locale)
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
            return GetClassName(playerClass, locale);

        return "Bot";
    }

    Creature const* FindPlayerBotByEntry(Player const* player, uint32 entry)
    {
        if (!player)
            return nullptr;

        BotMgr const* mgr = player->GetBotMgr();
        if (!mgr)
            return nullptr;

        BotMap const* botMap = mgr->GetBotMap();
        if (!botMap)
            return nullptr;

        for (auto const& [guid, bot] : *botMap)
        {
            if (bot && bot->GetEntry() == entry)
                return bot;
        }

        return nullptr;
    }

    void ResolveBotIdentityForBmu(Player const* player, uint32 entry, NpcBotData const* data,
        uint32& playerRace, uint8& gender, std::string& className)
    {
        LocaleConstant const locale = player && player->GetSession() ?
            player->GetSession()->GetSessionDbLocaleIndex() : DEFAULT_LOCALE;

        playerRace = RACE_HUMAN;
        gender = GENDER_MALE;
        className = "Bot";

        uint8 botClass = BOT_CLASS_NONE;

        if (Creature const* bot = FindPlayerBotByEntry(player, entry))
        {
            if (bot->GetBotAI())
            {
                botClass = bot->GetBotAI()->GetBotClass();
                playerRace = BotMgr::GetBotPlayerRace(bot);
            }
        }
        else if (NpcBotExtras const* extras = BotDataMgr::SelectNpcBotExtras(entry))
        {
            if (botClass == BOT_CLASS_NONE)
                botClass = extras->bclass;
            playerRace = BotMgr::GetBotPlayerRace(extras->bclass, extras->race);
        }

        if (botClass != BOT_CLASS_NONE)
            className = GetBotClassDisplayName(botClass, locale);

        if (className.empty())
            className = "Bot";

        if (NpcBotAppearanceData const* appearance = BotDataMgr::SelectNpcBotAppearance(entry))
            gender = appearance->gender;

        (void)data;
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

        return 0;
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

    void SendBotStatsPacket(Player* player, NpcBotStats const& stats, float resiliencePct)
    {
        std::ostringstream ss;
        ss << "S;" << stats.entry
           << ';' << stats.maxhealth
           << ';' << stats.maxpower
           << ';' << stats.strength
           << ';' << stats.agility
           << ';' << stats.stamina
           << ';' << stats.intellect
           << ';' << stats.spirit
           << ';' << stats.armor
           << ';' << stats.defense
           << ';' << stats.resHoly
           << ';' << stats.resFire
           << ';' << stats.resNature
           << ';' << stats.resFrost
           << ';' << stats.resShadow
           << ';' << stats.resArcane
           << ';' << stats.blockPct
           << ';' << stats.dodgePct
           << ';' << stats.parryPct
           << ';' << stats.critPct
           << ';' << stats.attackPower
           << ';' << stats.spellPower
           << ';' << stats.spellPen
           << ';' << stats.hastePct
           << ';' << stats.hitBonusPct
           << ';' << stats.expertise
           << ';' << stats.armorPenPct
           << ';' << resiliencePct;

        SendBMU(player, ss.str());
    }

    void TrySendBotStats(Player* player, uint32 entry)
    {
        Creature const* bot = BotDataMgr::FindBot(entry);
        if (!bot || !bot->IsInWorld() || !bot->GetBotAI())
            return;

        NpcBotStats stats{};
        bot->GetBotAI()->FillNpcBotStats(stats);
        // By leewheel 20260528 - expose bot resilience value to BMU addon stats panel.
        SendBotStatsPacket(player, stats, bot->GetBotAI()->GetBotResilience());
    }

    void AppendItemExtraFields(std::ostringstream& ss, Item const* item)
    {
        if (!item)
        {
            ss << ";0;0;0;0;0;0;0";
            return;
        }

        ss << ';' << item->GetEnchantmentId(EnchantmentSlot::PERM_ENCHANTMENT_SLOT);

        uint32 gem1 = 0;
        uint32 gem2 = 0;
        uint32 gem3 = 0;
        for (uint32 sock = SOCK_ENCHANTMENT_SLOT; sock < SOCK_ENCHANTMENT_SLOT + MAX_ITEM_PROTO_SOCKETS; ++sock)
        {
            uint32 const enchantId = item->GetEnchantmentId(EnchantmentSlot(sock));
            switch (sock - SOCK_ENCHANTMENT_SLOT)
            {
                case 0: gem1 = enchantId; break;
                case 1: gem2 = enchantId; break;
                case 2: gem3 = enchantId; break;
                default: break;
            }
        }

        ss << ';' << gem1 << ';' << gem2 << ';' << gem3 << ";0";
        ss << ';' << item->GetItemRandomPropertyId();
        ss << ';' << item->GetItemSuffixFactor();
    }

    void SendGearSlotPacket(Player* player, uint32 entry, uint8 slot, Item const* item, uint32 fallbackEntry = 0)
    {
        uint32 const itemEntry = item ? item->GetEntry() : fallbackEntry;
        std::ostringstream ss;
        ss << "G;" << entry << ';' << SLOT_KEYS[slot] << ';' << itemEntry;
        AppendItemExtraFields(ss, item);
        SendBMU(player, ss.str());
    }

    void SendGearFromLiveBot(Player* player, uint32 entry, Creature const* bot)
    {
        bot_ai const* ai = bot->GetBotAI();
        if (!ai)
            return;

        for (uint8 slot = 0; slot < BOT_INVENTORY_SIZE; ++slot)
            SendGearSlotPacket(player, entry, slot, ai->GetEquips(slot));
    }

    void SendBotGear(Player* player, uint32 entry, NpcBotData const* data, std::unordered_map<uint32, uint32> const& instanceMap)
    {
        Creature const* bot = BotDataMgr::FindBot(entry);
        bot_ai const* ai = bot && bot->IsInWorld() ? bot->GetBotAI() : nullptr;

        for (uint8 slot = 0; slot < BOT_INVENTORY_SIZE; ++slot)
        {
            Item const* item = ai ? ai->GetEquips(slot) : nullptr;
            uint32 const fallbackEntry = ResolveItemEntry(data->equips[slot], instanceMap);
            SendGearSlotPacket(player, entry, slot, item, fallbackEntry);
        }
    }

    // By leewheel 20260530 - BMU name must match party frame (GetNpcBotDisplayName / NAME_QUERY rules).
    std::string ResolveBotNameForBmu(Player const* player, uint32 entry, CreatureTemplate const* proto)
    {
        if (player && player->GetSession())
        {
            std::string const displayName = BotDataMgr::GetNpcBotDisplayName(entry, player->GetSession()->GetSessionDbLocaleIndex());
            if (!displayName.empty())
                return displayName;
        }

        return proto ? proto->Name : "Bot";
    }

    uint32 ResolveBotDisplayIdForBmu(Player const* player, uint32 entry, CreatureTemplate const* proto)
    {
        if (Creature const* bot = FindPlayerBotByEntry(player, entry))
            return bot->GetDisplayId();

        if (Creature const* bot = BotDataMgr::FindBot(entry))
            return bot->GetDisplayId();

        return proto ? proto->GetFirstValidModelId() : 0;
    }

    void SendBotMeta(Player* player, uint32 entry, NpcBotData const* data)
    {
        CreatureTemplate const* proto = sObjectMgr->GetCreatureTemplate(entry);
        if (!proto)
            return;

        std::string const botName = ResolveBotNameForBmu(player, entry, proto);
        uint32 const displayId = ResolveBotDisplayIdForBmu(player, entry, proto);
        uint32 playerRace = RACE_HUMAN;
        uint8 gender = GENDER_MALE;
        std::string className = "Bot";
        ResolveBotIdentityForBmu(player, entry, data, playerRace, gender, className);

        std::ostringstream ss;
        ss << "B;" << entry << ';' << EscapeAddonField(botName) << ';' << data->roles << ';' << EscapeAddonField(className)
           << ';' << displayId << ';' << playerRace << ';' << uint32(gender)
           << ';' << uint32(player->GetLevel()) << ';' << uint32(data->spec);

        SendBMU(player, ss.str());
    }

    void SendBotData(Player* player, uint32 entry)
    {
        NpcBotData const* data = BotDataMgr::SelectNpcBotData(entry);
        if (!data || !PlayerOwnsBot(player, entry))
            return;

        SendBotMeta(player, entry, data);

        if (Creature const* bot = BotDataMgr::FindBot(entry))
        {
            if (bot->IsInWorld() && bot->GetBotAI())
            {
                SendGearFromLiveBot(player, entry, bot);
                TrySendBotStats(player, entry);
                return;
            }
        }

        std::unordered_map<uint32, uint32> const emptyMap;
        SendBotGear(player, entry, data, emptyMap);
    }

    void SendOwnedBots(Player* player, std::vector<uint32> const& entries)
    {
        if (entries.empty())
        {
            SendBMU(player, "E;END");
            return;
        }

        std::unordered_map<uint32, uint32> const emptyMap;

        for (uint32 entry : entries)
        {
            NpcBotData const* data = BotDataMgr::SelectNpcBotData(entry);
            if (!data || !PlayerOwnsBot(player, entry))
                continue;

            SendBotMeta(player, entry, data);

            if (Creature const* bot = BotDataMgr::FindBot(entry))
            {
                if (bot->IsInWorld() && bot->GetBotAI())
                {
                    SendGearFromLiveBot(player, entry, bot);
                    TrySendBotStats(player, entry);
                    continue;
                }
            }

            SendBotGear(player, entry, data, emptyMap);
        }

        SendBMU(player, "E;END");
    }

    // 仅当前跟随/已召唤的 Bot（与 BotMgr 一致），避免 BMU 列出历史上雇佣过但未跟队的 entry
    std::vector<uint32> LoadActiveBotEntries(Player const* player)
    {
        std::vector<uint32> entries;
        if (!player)
            return entries;

        BotMgr const* mgr = player->GetBotMgr();
        if (!mgr)
            return entries;

        BotMap const* botMap = mgr->GetBotMap();
        if (!botMap)
            return entries;

        entries.reserve(botMap->size());
        for (auto const& [guid, bot] : *botMap)
        {
            if (!bot)
                continue;
            entries.push_back(bot->GetEntry());
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
        std::vector<uint32> entries = LoadActiveBotEntries(player);
        BOT_LOG_INFO("npcbots", "BMU REFRESH: player '{}' guid {} active bots {}",
            player->GetName(), player->GetGUID().GetCounter(), entries.size());

        if (entries.empty())
        {
            ChatHandler(player->GetSession()).SendSysMessage("[机器人管理] 当前没有跟随你的机器人，请先雇佣、快速组队或召唤机器人。");
            SendBMU(player, "E;END");
            return;
        }

        for (uint32 entry : entries)
        {
            NpcBotData const* data = BotDataMgr::SelectNpcBotData(entry);
            Creature const* bot = BotDataMgr::FindBot(entry);
            bool const live = bot && bot->IsInWorld() && bot->GetBotAI();
            uint32 gearNonZero = 0;
            if (live)
            {
                for (uint8 slot = 0; slot < BOT_INVENTORY_SIZE; ++slot)
                    if (bot->GetBotAI()->GetEquips(slot))
                        ++gearNonZero;
            }
            else if (data)
            {
                for (uint32 itemGuid : data->equips)
                    if (itemGuid)
                        ++gearNonZero;
            }

            BOT_LOG_INFO("npcbots", "BMU   bot entry {} owner={} live={} gearSlots={}",
                entry, data ? data->owner : 0u, live, gearNonZero);
        }

        SendOwnedBots(player, std::move(entries));
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

// By leewheel 20260528 - gossip「让我看看你的装备」: sync data + tell client addon to open inspect window
void BotManagerAddon::SendBotSnapshotAndOpenUI(Player* player, uint32 botEntry)
{
    if (!player || !botEntry)
        return;

    HandleQuery(player, botEntry);

    std::ostringstream openUi;
    openUi << "U;OPEN;" << botEntry;
    SendBMU(player, openUi.str());
}

bool BotManagerAddon::TryHandleIncoming(Player* player, std::string_view message)
{
    if (!player)
        return false;

    std::string_view const raw = message;
    if (!TryStripAddonPrefix(message, PREFIX))
        return false;

    std::vector<std::string_view> parts = SplitSemicolon(message);
    if (parts.empty())
        return true;

    std::string_view const cmd = parts[0];
    BOT_LOG_INFO("npcbots", "BMU IN: player '{}' cmd='{}' raw='{}'",
        player->GetName(), cmd, raw.substr(0, std::min<size_t>(raw.size(), 64)));

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
