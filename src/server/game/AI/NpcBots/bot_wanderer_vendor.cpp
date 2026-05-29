/*
 * Wanderer bot world-channel vendor (whisper negotiate + COD mail).
 * Items come from item_template catalog (not bot equipment).
 * By leewheel 20260529
 */

#include "bot_wanderer_vendor.h"

#include "botconfig.h"
#include "botcommon.h"
#include "botdatamgr.h"
#include "botdefine.h"
#include "Containers.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Log.h"
#include "Mail.h"
#include "ObjectMgr.h"
#include "GameTime.h"
#include "Player.h"
#include "StringConvert.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    struct SaleOffer
    {
        uint32 itemEntry = 0;
        uint32 itemCount = 1;
        uint32 vendorCopperTotal = 0; // NPC buyout (SellPrice * count)
        uint32 floorCopper = 0;       // bot random minimum acceptable price
        uint32 askCopper = 0;         // initial asking price
        bool isMaterial = false;
        bool active = false;
        uint32 refreshAtMs = 0;
    };

    struct Negotiation
    {
        uint32 lastPlayerOfferCopper = 0;
        uint32 botCounterCopper = 0;
        uint32 lastTalkAtMs = 0;
    };

    struct CatalogBucket
    {
        std::vector<uint32> gearByLevelStep[LEVEL_STEPS];
        std::vector<uint32> materialsByLevelStep[LEVEL_STEPS];
    };

    std::unordered_map<uint32, SaleOffer> s_offerByBotEntry;
    std::unordered_map<uint64, Negotiation> s_negotiationByKey;
    // Min Chance% across world loot tables (lower = harder to drop = higher price factor).
    std::unordered_map<uint32, float> s_itemMinLootChancePercent;
    CatalogBucket s_catalog;
    bool s_catalogBuilt = false;

    uint64 MakeNegotiationKey(ObjectGuid const& playerGuid, uint32 botEntry)
    {
        return (uint64(botEntry) << 32) | playerGuid.GetCounter();
    }

    uint8 LevelToStep(uint8 level)
    {
        level = std::min<uint8>(level, DEFAULT_MAX_LEVEL + 4);
        return std::min<uint8>(level / ITEM_SORTING_LEVEL_STEP, LEVEL_STEPS - 1);
    }

    bool IsDisabledVendorItem(uint32 itemId, std::unordered_set<uint32> const& disabled)
    {
        return disabled.contains(itemId);
    }

    bool IsVendorTradeMaterial(ItemTemplate const* proto)
    {
        if (!proto || proto->Class != ITEM_CLASS_TRADE_GOODS)
            return false;

        switch (proto->SubClass)
        {
            case ITEM_SUBCLASS_TRADE_GOODS:
            case ITEM_SUBCLASS_PARTS:
            case ITEM_SUBCLASS_JEWELCRAFTING:
            case ITEM_SUBCLASS_CLOTH:
            case ITEM_SUBCLASS_LEATHER:
            case ITEM_SUBCLASS_METAL_STONE:
            case ITEM_SUBCLASS_HERB:
            case ITEM_SUBCLASS_ELEMENTAL:
            case ITEM_SUBCLASS_ENCHANTING:
            case ITEM_SUBCLASS_MATERIAL:
            case ITEM_SUBCLASS_ARMOR_ENCHANTMENT:
            case ITEM_SUBCLASS_WEAPON_ENCHANTMENT:
                break;
            default:
                return false;
        }

        return proto->Quality >= ITEM_QUALITY_UNCOMMON && proto->Stackable > 1;
    }

    bool IsVendorScrollOrGem(ItemTemplate const* proto)
    {
        if (!proto)
            return false;
        if (proto->Class == ITEM_CLASS_CONSUMABLE && proto->SubClass == ITEM_SUBCLASS_SCROLL)
            return proto->Quality >= ITEM_QUALITY_UNCOMMON;
        if (proto->Class == ITEM_CLASS_GEM)
            return proto->Quality >= ITEM_QUALITY_UNCOMMON;
        return false;
    }

    bool IsVendorBoEGear(ItemTemplate const* proto)
    {
        if (!proto)
            return false;
        if (proto->GetBonding() != BIND_WHEN_PICKED_UP)
            return false;
        if (proto->Quality < ITEM_QUALITY_RARE)
            return false;
        if (proto->Class != ITEM_CLASS_WEAPON && proto->Class != ITEM_CLASS_ARMOR)
            return false;
        if (proto->HasFlag(ITEM_FLAG_CONJURED))
            return false;
        if (!proto->SellPrice && !proto->BuyPrice)
            return false;
        if (!proto->RequiredLevel || proto->RequiredLevel > DEFAULT_MAX_LEVEL)
            return false;
        return true;
    }

    std::string GetItemDisplayName(uint32 itemEntry, uint32 count)
    {
        if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemEntry))
        {
            if (count > 1)
                return Bcore::StringFormat("{}x{}", proto->Name1, count);
            return proto->Name1;
        }
        return "物品";
    }

    std::string FormatGold(uint32 copper)
    {
        uint32 const gold = copper / GOLD;
        uint32 const silver = (copper % GOLD) / SILVER;
        if (gold)
            return Bcore::StringFormat("{}金{}银", gold, silver);
        if (silver)
            return Bcore::StringFormat("{}银", silver);
        return Bcore::StringFormat("{}铜", copper);
    }

    uint32 GetUnitVendorCopper(ItemTemplate const* proto)
    {
        if (!proto)
            return 100u;
        if (proto->SellPrice)
            return proto->SellPrice;
        if (proto->BuyPrice > 0)
            return std::max(uint32(proto->BuyPrice / 4), 1u);
        return std::max(uint32(proto->ItemLevel) * 50u, 100u);
    }

    void BuildDropChanceIndex()
    {
        s_itemMinLootChancePercent.clear();

        // Aggregate direct item drops; MIN chance = hardest source (lowest drop rate).
        static char const* const query =
            "SELECT Item, MIN(Chance) AS min_chance FROM ("
            "  SELECT Item, Chance FROM creature_loot_template WHERE Item > 0"
            "  UNION ALL SELECT Item, Chance FROM gameobject_loot_template WHERE Item > 0"
            "  UNION ALL SELECT Item, Chance FROM disenchant_loot_template WHERE Item > 0"
            "  UNION ALL SELECT Item, Chance FROM item_loot_template WHERE Item > 0"
            "  UNION ALL SELECT Item, Chance FROM pickpocketing_loot_template WHERE Item > 0"
            "  UNION ALL SELECT Item, Chance FROM skinning_loot_template WHERE Item > 0"
            "  UNION ALL SELECT Item, Chance FROM fishing_loot_template WHERE Item > 0"
            "  UNION ALL SELECT Item, Chance FROM milling_loot_template WHERE Item > 0"
            "  UNION ALL SELECT Item, Chance FROM prospecting_loot_template WHERE Item > 0"
            "  UNION ALL SELECT Item, Chance FROM reference_loot_template WHERE Item > 0"
            ") AS loot_items GROUP BY Item";

        uint32 indexed = 0;
        if (QueryResult result = WorldDatabase.Query(query))
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 const itemId = fields[0].GetUInt32();
                float const chance = fields[1].GetFloat();
                if (chance > 0.f)
                {
                    s_itemMinLootChancePercent[itemId] = chance;
                    ++indexed;
                }
            } while (result->NextRow());
        }

        BOT_LOG_INFO("server.loading", ">> Wanderer vendor drop-rate index: {} items from loot tables", indexed);
    }

    // Lower loot Chance% -> higher multiplier within [minMult, maxMult].
    float GetDropRarityFactor(uint32 itemEntry)
    {
        auto const itr = s_itemMinLootChancePercent.find(itemEntry);
        if (itr == s_itemMinLootChancePercent.end())
            return 1.12f; // craft/vendor-only: slight premium

        float const chance = std::max(itr->second, 0.001f);
        float const factor = 1.0f + std::log10(100.0f / chance) * 0.5f;
        return std::clamp(factor, 1.0f, 3.5f);
    }

    std::optional<float> GetItemMinLootChancePercent(uint32 itemEntry)
    {
        auto const itr = s_itemMinLootChancePercent.find(itemEntry);
        if (itr == s_itemMinLootChancePercent.end())
            return std::nullopt;
        return itr->second;
    }

    // floor = vendor * [minMult..minMult+spread] * dropRarity; ask = vendor * [floorMult+1 .. maxMult] * dropRarity
    void ComputeListingPrices(ItemTemplate const* proto, uint32 count, uint32& vendorCopperTotal, uint32& floorCopper, uint32& askCopper)
    {
        uint32 const unitVendor = GetUnitVendorCopper(proto);
        vendorCopperTotal = unitVendor * std::max(count, 1u);

        float const minMult = BotCfg::GetWandererVendorMinVendorMultiplier();
        float const maxMult = BotCfg::GetWandererVendorMaxVendorMultiplier();
        float const spread = BotCfg::GetWandererVendorFloorSpreadMult();
        float const rarity = GetDropRarityFactor(proto->ItemId);

        float const minEff = minMult * rarity;
        float const maxEff = maxMult * rarity;
        float const floorMultHi = std::min(maxEff, minEff + spread);
        float const floorMult = frand(minEff, std::max(minEff, floorMultHi));
        float const askMultLo = std::min(maxEff, floorMult + 1.0f);
        float const askMult = frand(askMultLo, std::max(askMultLo, maxEff));

        floorCopper = std::max(1u, uint32(float(vendorCopperTotal) * floorMult));
        askCopper = std::max(floorCopper + 1u, uint32(float(vendorCopperTotal) * askMult));
    }

    uint32 RollMaterialCount(ItemTemplate const* proto)
    {
        uint32 const maxStack = std::max(proto->GetMaxStackSize(), 1u);
        uint32 const cap = std::min(maxStack, 20u);
        if (cap <= 1)
            return 1;
        return urand(1, cap);
    }

    std::optional<uint32> ParseCopperFromMessage(std::string_view msg)
    {
        std::string digits;
        bool seenDigit = false;
        for (char c : msg)
        {
            if (std::isdigit(static_cast<unsigned char>(c)))
            {
                digits.push_back(c);
                seenDigit = true;
            }
            else if (seenDigit)
                break;
        }
        if (digits.empty())
            return std::nullopt;

        Optional<uint32> parsed = Bcore::StringTo<uint32>(digits);
        if (!parsed)
            return std::nullopt;
        uint32 const value = *parsed;

        bool const hasGold = msg.find('金') != std::string_view::npos || msg.find('g') != std::string_view::npos || msg.find('G') != std::string_view::npos;
        bool const hasSilver = msg.find('银') != std::string_view::npos || msg.find('s') != std::string_view::npos || msg.find('S') != std::string_view::npos;

        if (hasGold)
            return value * GOLD;
        if (hasSilver)
            return value * SILVER;
        if (value >= 10000)
            return value;
        return value * GOLD;
    }

    bool ContainsBuyIntent(std::string_view msg)
    {
        return msg.find("买") != std::string_view::npos || msg.find("要") != std::string_view::npos ||
            msg.find("成交") != std::string_view::npos || msg.find("拿下") != std::string_view::npos;
    }

    void BotWhisperTo(Player* player, Creature* bot, std::string const& text)
    {
        if (!player || !bot || text.empty())
            return;
        bot->Whisper(text, LANG_UNIVERSAL, player);
    }

    SaleOffer* GetOffer(uint32 botEntry)
    {
        auto itr = s_offerByBotEntry.find(botEntry);
        return itr != s_offerByBotEntry.end() ? &itr->second : nullptr;
    }

    bool PickCatalogItem(uint8 botLevel, bool wantMaterial, uint32& itemEntry, uint32& itemCount)
    {
        if (!s_catalogBuilt)
            return false;

        uint8 const step = LevelToStep(botLevel);
        std::vector<uint32> const* pool = nullptr;

        if (wantMaterial)
        {
            for (int s = int(step); s >= 0; --s)
            {
                if (!s_catalog.materialsByLevelStep[s].empty())
                {
                    pool = &s_catalog.materialsByLevelStep[s];
                    break;
                }
            }
        }
        else
        {
            for (int s = int(step); s >= 0; --s)
            {
                if (!s_catalog.gearByLevelStep[s].empty())
                {
                    pool = &s_catalog.gearByLevelStep[s];
                    break;
                }
            }
        }

        if (!pool || pool->empty())
            return false;

        itemEntry = Bcore::Containers::SelectRandomContainerElement(*pool);
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemEntry);
        if (!proto)
            return false;

        itemCount = (wantMaterial || IsVendorTradeMaterial(proto) || IsVendorScrollOrGem(proto))
            ? RollMaterialCount(proto) : 1u;
        return true;
    }

    bool CreateMailItemForPlayer(Player* player, uint32 itemEntry, uint32 count, Item*& outItem, CharacterDatabaseTransaction& trans)
    {
        outItem = Item::CreateItem(itemEntry, count, player);
        if (!outItem)
            return false;

        outItem->SetBinding(false);
        outItem->SetOwnerGUID(player->GetGUID());
        outItem->FSetState(ITEM_CHANGED);
        outItem->SaveToDB(trans);

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ITEM_OWNER);
        stmt->setUInt32(0, player->GetGUID().GetCounter());
        stmt->setUInt32(1, outItem->GetGUID().GetCounter());
        trans->Append(stmt);
        return true;
    }
}

void WandererVendor::BuildCatalog()
{
    s_catalog = {};
    s_catalogBuilt = false;
    BuildDropChanceIndex();

    std::unordered_set<uint32> disabled;
    if (QueryResult dires = WorldDatabase.Query("SELECT id FROM creature_template_npcbot_disabled_items"))
    {
        do
            disabled.insert(dires->Fetch()->GetUInt32());
        while (dires->NextRow());
    }

    uint32 gearCount = 0;
    uint32 matCount = 0;

    ItemTemplateContainer const& all = sObjectMgr->GetItemTemplateStore();
    for (auto const& [_, proto] : all)
    {
        if (IsDisabledVendorItem(proto.ItemId, disabled))
            continue;

        uint8 const reqLevel = std::max<uint8>(proto.RequiredLevel, 1);
        if (reqLevel > DEFAULT_MAX_LEVEL)
            continue;

        uint8 const step = LevelToStep(reqLevel);

        if (IsVendorBoEGear(&proto))
        {
            s_catalog.gearByLevelStep[step].push_back(proto.ItemId);
            ++gearCount;
        }
        else if (IsVendorTradeMaterial(&proto) || IsVendorScrollOrGem(&proto))
        {
            s_catalog.materialsByLevelStep[step].push_back(proto.ItemId);
            ++matCount;
        }
    }

    s_catalogBuilt = gearCount > 0 || matCount > 0;
    BOT_LOG_INFO("server.loading", ">> Wanderer vendor catalog: {} BoE gear entries, {} material/scroll/gem entries",
        gearCount, matCount);

    if (!s_catalogBuilt)
        BOT_LOG_ERROR("server.loading", "Wanderer vendor catalog is empty - disable NpcBot.Wanderer.Vendor.Enable or fix item DB");
}

bool WandererVendor::IsEnabled()
{
    return BotCfg::IsWandererVendorEnabled() && s_catalogBuilt;
}

void WandererVendor::OnWandererTick(Creature* bot)
{
    if (!BotCfg::IsWandererVendorEnabled() || !bot || !bot->IsWandererBot())
        return;

    if (!s_catalogBuilt)
        return;

    uint32 const nowMs = GameTime::GetGameTimeMS();
    SaleOffer& offer = s_offerByBotEntry[bot->GetEntry()];

    if (offer.active && offer.refreshAtMs > nowMs)
        return;

    offer = {};

    if (!roll_chance_f(BotCfg::GetWandererVendorStockChance()))
    {
        offer.refreshAtMs = nowMs + BotCfg::GetWandererVendorRefreshMs();
        return;
    }

    bool const wantMaterial = roll_chance_f(BotCfg::GetWandererVendorMaterialChance());
    uint32 itemEntry = 0;
    uint32 itemCount = 1;
    if (!PickCatalogItem(bot->GetLevel(), wantMaterial, itemEntry, itemCount))
    {
        if (!PickCatalogItem(bot->GetLevel(), !wantMaterial, itemEntry, itemCount))
        {
            offer.refreshAtMs = nowMs + BotCfg::GetWandererVendorRefreshMs();
            return;
        }
    }

    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemEntry);
    if (!proto)
    {
        offer.refreshAtMs = nowMs + BotCfg::GetWandererVendorRefreshMs();
        return;
    }

    offer.itemEntry = itemEntry;
    offer.itemCount = itemCount;
    offer.isMaterial = wantMaterial || IsVendorTradeMaterial(proto) || IsVendorScrollOrGem(proto);
    ComputeListingPrices(proto, itemCount, offer.vendorCopperTotal, offer.floorCopper, offer.askCopper);
    offer.active = true;
    offer.refreshAtMs = nowMs + BotCfg::GetWandererVendorRefreshMs();
}

bool WandererVendor::WantsTradeWorldShout(Creature const* bot)
{
    if (!IsEnabled() || !bot || !bot->IsWandererBot())
        return false;

    SaleOffer const* offer = GetOffer(bot->GetEntry());
    if (!offer || !offer->active)
        return false;

    return roll_chance_f(BotCfg::GetWandererVendorShoutChance());
}

std::string WandererVendor::BuildTradeSceneEvent(Creature const* bot)
{
    SaleOffer const* offer = GetOffer(bot->GetEntry());
    if (!offer || !offer->active)
        return {};

    std::string dropHint = "unknown";
    if (std::optional<float> dropPct = GetItemMinLootChancePercent(offer->itemEntry))
        dropHint = Bcore::StringFormat("{:.4g}%", *dropPct);

    return Bcore::StringFormat(
        "event=WANDERER_TRADE_SHOUT; item={}; count={}; vendor={}; ask={}; floor={}; loot_min_chance={}; bot={}; type={}",
        GetItemDisplayName(offer->itemEntry, offer->itemCount), offer->itemCount,
        FormatGold(offer->vendorCopperTotal), FormatGold(offer->askCopper), FormatGold(offer->floorCopper),
        dropHint, bot->GetName(), offer->isMaterial ? "material" : "gear");
}

std::string WandererVendor::BuildTradeShoutFallback(Creature const* bot)
{
    SaleOffer const* offer = GetOffer(bot->GetEntry());
    if (!offer || !offer->active)
        return {};

    return Bcore::StringFormat("[出售]{} 起拍{}，密我「{}」谈价。",
        GetItemDisplayName(offer->itemEntry, offer->itemCount), FormatGold(offer->askCopper), bot->GetName());
}

bool WandererVendor::TryHandlePlayerWhisper(Player* player, Creature* bot, std::string_view message)
{
    if (!BotCfg::IsWandererVendorEnabled() || !player || !bot || !bot->IsWandererBot())
        return false;

    if (!s_catalogBuilt)
        return false;

    if (!BotCfg::IsBotChatEnabled())
        return false;

    if (player->GetMapId() != bot->GetMapId() || !player->IsWithinDistInMap(bot, 200.0f))
    {
        BotWhisperTo(player, bot, "我现在不在这边，当面再聊吧。");
        return true;
    }

    SaleOffer* offer = GetOffer(bot->GetEntry());
    if (!offer || !offer->active)
    {
        BotWhisperTo(player, bot, "抱歉，我刚把东西卖掉了，下次早点密我。");
        return true;
    }

    uint64 const negKey = MakeNegotiationKey(player->GetGUID(), bot->GetEntry());
    Negotiation& neg = s_negotiationByKey[negKey];
    neg.lastTalkAtMs = GameTime::GetGameTimeMS();

    std::optional<uint32> offerCopper = ParseCopperFromMessage(message);
    if (offerCopper)
        neg.lastPlayerOfferCopper = *offerCopper;

    std::string const itemName = GetItemDisplayName(offer->itemEntry, offer->itemCount);

    if (!offerCopper && ContainsBuyIntent(message))
    {
        BotWhisperTo(player, bot, Bcore::StringFormat(
            "【{}】商人回收价才{}，起拍 {}（我心里底价 {}），直接回金币数。",
            itemName, FormatGold(offer->vendorCopperTotal), FormatGold(offer->askCopper), FormatGold(offer->floorCopper)));
        return true;
    }

    if (!offerCopper)
    {
        BotWhisperTo(player, bot, Bcore::StringFormat(
            "【{}】商人只给{}，我卖 {} 起拍，低于 {} 免谈。",
            itemName, FormatGold(offer->vendorCopperTotal), FormatGold(offer->askCopper), FormatGold(offer->floorCopper)));
        return true;
    }

    uint32 const playerOffer = *offerCopper;
    if (playerOffer >= offer->floorCopper)
    {
        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        Item* saleItem = nullptr;
        if (!CreateMailItemForPlayer(player, offer->itemEntry, offer->itemCount, saleItem, trans))
        {
            BotWhisperTo(player, bot, "打包失败了，稍后再试。");
            return true;
        }

        std::string const subject = Bcore::StringFormat("商品：%s", itemName);
        std::string const body = Bcore::StringFormat("成交了，{}，付款取件 {}。", player->GetName(), FormatGold(playerOffer));
        MailDraft(subject, body)
            .AddItem(saleItem)
            .AddCOD(playerOffer)
            .SendMailTo(trans, MailReceiver(player), MailSender(bot, MAIL_STATIONERY_DEFAULT), MAIL_CHECK_MASK_HAS_BODY);

        CharacterDatabase.CommitTransaction(trans);

        BotWhisperTo(player, bot, Bcore::StringFormat("成交！{} 已寄出，邮箱付款取件 {}。", itemName, FormatGold(playerOffer)));
        offer->active = false;
        s_negotiationByKey.erase(negKey);
        return true;
    }

    neg.botCounterCopper = offer->floorCopper;
    BotWhisperTo(player, bot, Bcore::StringFormat(
        "【{}】{} 太低了，商人回收才{}，起码 {} 我才卖。",
        itemName, FormatGold(playerOffer), FormatGold(offer->vendorCopperTotal), FormatGold(offer->floorCopper)));
    return true;
}
