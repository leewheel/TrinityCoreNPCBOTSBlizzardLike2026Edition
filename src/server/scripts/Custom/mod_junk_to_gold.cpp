/*
 * mod-junk-to-gold — auto-sell grey loot for vendor gold (TrinityCore 3.3.5)
 * Integrated into Custom scripts; config: JunkToGold.Enable in worldserver.conf
 */

#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "StringFormat.h"
#include "Timer.h"
#include "World.h"

#include <map>
#include <unordered_map>
#include <vector>

namespace
{
bool s_JunkToGoldEnable = true;

std::unordered_map<uint32, std::unordered_map<uint8, std::string>> g_ItemNameLocaleCache;

struct PendingDestroyItem
{
    uint8 bagSlot;
    uint8 slot;
    uint32 destroyTime;
};

std::map<ObjectGuid, std::vector<PendingDestroyItem>> g_PendingDestroyItems;

constexpr uint32 DESTROY_DELAY_MS = 300;

std::string GetLocalizedItemName(Player* player, ItemTemplate const* proto)
{
    if (!player || !proto)
        return "";

    uint32 itemId = proto->ItemId;
    uint8 localeIndex = player->GetSession()->GetSessionDbLocaleIndex();

    if (localeIndex == 0)
        return proto->Name1;

    auto itemIt = g_ItemNameLocaleCache.find(itemId);
    if (itemIt != g_ItemNameLocaleCache.end())
    {
        auto localeIt = itemIt->second.find(localeIndex);
        if (localeIt != itemIt->second.end())
            return localeIt->second;
    }

    static char const* LOCALE_INDEX_TO_STRING[] =
    {
        "enUS", "koKR", "frFR", "deDE",
        "zhCN", "zhTW", "esES", "esMX", "ruRU"
    };

    if (localeIndex >= sizeof(LOCALE_INDEX_TO_STRING) / sizeof(char*))
        return proto->Name1;

    std::string locale = LOCALE_INDEX_TO_STRING[localeIndex];
    std::string localizedName;

    if (QueryResult result = WorldDatabase.PQuery(
        "SELECT Name FROM item_template_locale WHERE ID = {} AND locale = '{}'",
        itemId, locale))
    {
        Field* fields = result->Fetch();
        localizedName = fields[0].Get<std::string>();
    }

    if (localizedName.empty())
        localizedName = proto->Name1;

    g_ItemNameLocaleCache[itemId][localeIndex] = localizedName;
    return localizedName;
}

std::string BuildItemLink(Player* player, Item* item)
{
    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return "";

    uint32 color = ItemQualityColors[proto->Quality];
    std::string itemName = GetLocalizedItemName(player, proto);

    return Trinity::StringFormat(
        "|c{:08x}|Hitem:{}::::::::{}:::::|h[{}]|h|r",
        color,
        proto->ItemId,
        player->GetLevel(),
        itemName);
}

void SendTransactionInformation(Player* player, Item* item, uint32 count)
{
    std::string name = BuildItemLink(player, item);

    if (count > 1)
        name += Trinity::StringFormat("x{}", count);

    uint32 money = item->GetTemplate()->SellPrice * count;

    uint32 gold = money / GOLD;
    uint32 silver = (money % GOLD) / SILVER;
    uint32 copper = (money % SILVER);

    std::string info;

    if (money < SILVER)
        info = Trinity::StringFormat("{} 已出售，获得 {} 铜币。", name, copper);
    else if (money < GOLD)
        info = copper
            ? Trinity::StringFormat("{} 已出售，获得 {} 银币和 {} 铜币。", name, silver, copper)
            : Trinity::StringFormat("{} 已出售，获得 {} 银币。", name, silver);
    else if (silver && copper)
        info = Trinity::StringFormat("{} 已出售，获得 {} 金币、{} 银币和 {} 铜币。", name, gold, silver, copper);
    else if (silver)
        info = Trinity::StringFormat("{} 已出售，获得 {} 金币和 {} 银币。", name, gold, silver);
    else if (copper)
        info = Trinity::StringFormat("{} 已出售，获得 {} 金币和 {} 铜币。", name, gold, copper);
    else
        info = Trinity::StringFormat("{} 已出售，获得 {} 金币。", name, gold);

    ChatHandler(player->GetSession()).SendSysMessage(info);
}

void ProcessPendingDestroyForPlayer(Player* player, uint32 currentTime)
{
    auto it = g_PendingDestroyItems.find(player->GetGUID());
    if (it == g_PendingDestroyItems.end())
        return;

    std::vector<PendingDestroyItem>& pendingList = it->second;
    std::vector<PendingDestroyItem> remainingItems;

    for (PendingDestroyItem const& pending : pendingList)
    {
        if (currentTime >= pending.destroyTime)
        {
            Item* item = player->GetItemByPos(pending.bagSlot, pending.slot);
            if (item)
            {
                ItemTemplate const* proto = item->GetTemplate();
                if (proto && proto->Quality == ITEM_QUALITY_POOR)
                    player->DestroyItem(pending.bagSlot, pending.slot, true);
            }
        }
        else
            remainingItems.push_back(pending);
    }

    if (remainingItems.empty())
        g_PendingDestroyItems.erase(it);
    else
        it->second = std::move(remainingItems);
}

class JunkToGold_PlayerScript : public PlayerScript
{
public:
    JunkToGold_PlayerScript() : PlayerScript("JunkToGold_PlayerScript") { }

    void OnLootItem(Player* player, Item* item, uint32 count, ObjectGuid /*lootguid*/) override
    {
        if (!s_JunkToGoldEnable || !player || !item)
            return;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
            return;

        if (proto->Quality != ITEM_QUALITY_POOR)
            return;

        if (proto->SellPrice == 0)
            return;

        SendTransactionInformation(player, item, count);
        player->ModifyMoney(proto->SellPrice * count);

        uint8 bagSlot = item->GetBagSlot();
        uint8 slot = item->GetSlot();

        if (bagSlot != NULL_BAG && slot != NULL_SLOT)
        {
            PendingDestroyItem pendingItem;
            pendingItem.bagSlot = bagSlot;
            pendingItem.slot = slot;
            pendingItem.destroyTime = getMSTime() + DESTROY_DELAY_MS;
            g_PendingDestroyItems[player->GetGUID()].push_back(pendingItem);
        }
    }

    void OnLogout(Player* player) override
    {
        if (player)
            g_PendingDestroyItems.erase(player->GetGUID());
    }
};

class JunkToGold_WorldScript : public WorldScript
{
public:
    JunkToGold_WorldScript() : WorldScript("JunkToGold_WorldScript") { }

    void OnUpdate(uint32 /*diff*/) override
    {
        if (g_PendingDestroyItems.empty())
            return;

        uint32 const currentTime = getMSTime();
        SessionMap const& sessions = sWorld->GetAllSessions();
        for (auto const& sessionPair : sessions)
        {
            Player* player = sessionPair.second->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;

            ProcessPendingDestroyForPlayer(player, currentTime);
        }
    }
};
} // namespace

void AddSC_mod_junk_to_gold()
{
    s_JunkToGoldEnable = sConfigMgr->GetBoolDefault("JunkToGold.Enable", true);
    new JunkToGold_PlayerScript();
    new JunkToGold_WorldScript();
}
