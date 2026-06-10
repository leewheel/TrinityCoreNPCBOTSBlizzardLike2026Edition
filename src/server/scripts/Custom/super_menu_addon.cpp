/*
 * SuperMenu (SMU) — port of SupHeartSrone debug features (no hearthstone bind)
 */

#include "super_menu_addon.h"

#include "Common.h"
#include "Chat.h"
#include "ChatPackets.h"
#include "DBCStores.h"
#include "DisableMgr.h"
#include "Item.h"
#include "Map.h"
#include "MapManager.h"
#include "MotionMaster.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include "Util.h"
#include "WorldSession.h"

#include <array>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace
{
    bool IsAuthorized(Player const* player)
    {
        return player && player->GetSession() && player->GetSession()->GetSecurity() >= SEC_GAMEMASTER;
    }

    bool StartsWith(std::string_view value, std::string_view prefix)
    {
        return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
    }

    bool TryStripAddonPrefix(std::string_view& message, std::string_view prefix)
    {
        if (!message.empty() && message[0] == '\t')
        {
            size_t secondTab = message.find('\t', 1);
            if (secondTab == std::string_view::npos)
                return false;
            if (message.substr(1, secondTab - 1) != prefix)
                return false;
            message = message.substr(secondTab + 1);
            return true;
        }

        if (!StartsWith(message, prefix))
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

    std::string SanitizeQuestTitle(std::string const& title)
    {
        std::string out;
        out.reserve(title.size());
        for (char c : title)
        {
            if (c == ';' || c == '\n' || c == '\r' || c == '\t')
                out.push_back(' ');
            else
                out.push_back(c);
        }
        return out;
    }

    std::unordered_map<ObjectGuid::LowType, uint32> g_SummonCooldown;
    std::unordered_map<ObjectGuid::LowType, uint32> g_BookmarkTeleportCooldown;

    struct SmuBookmark
    {
        uint32 mapId = 0;
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;
        float o = 0.f;
        bool valid = false;
    };

    // slots 1..5 (index 0 unused)
    std::unordered_map<ObjectGuid::LowType, std::array<SmuBookmark, 6>> g_PlayerBookmarks;

    void ClearPlayerSuperMenuState(ObjectGuid::LowType guidLow)
    {
        g_SummonCooldown.erase(guidLow);
        g_BookmarkTeleportCooldown.erase(guidLow);
        g_PlayerBookmarks.erase(guidLow);
    }

    void Notify(Player* player, std::string const& msg)
    {
        if (player && player->GetSession())
            ChatHandler(player->GetSession()).SendSysMessage(msg.c_str());
    }

    SmuBookmark* GetBookmarkSlot(Player* player, uint8 slot)
    {
        if (!player || slot < 1 || slot > 5)
            return nullptr;
        return &g_PlayerBookmarks[player->GetGUID().GetCounter()][slot];
    }

    // By leewheel 20260610 - bookmark teleport must not stack Player::TeleportTo near semaphores (causes progressive server lag).
    bool TeleportPlayerBookmark(Player* player, uint32 mapId, float x, float y, float z, float o)
    {
        if (!player)
            return false;

        if (!MapManager::IsValidMapCoord(mapId, x, y, z, o))
        {
            Notify(player, "书签坐标无效。");
            return false;
        }

        ObjectGuid::LowType const guidLow = player->GetGUID().GetCounter();
        uint32 const now = getMSTime();
        if (g_BookmarkTeleportCooldown[guidLow] > now)
        {
            Notify(player, "书签折跃冷却中，请稍候。");
            return false;
        }
        g_BookmarkTeleportCooldown[guidLow] = now + 750;

        player->SetSemaphoreTeleportNear(false);
        player->SetSemaphoreTeleportFar(false);

        if (mapId == player->GetMapId())
        {
            player->CombatStop();
            player->GetMotionMaster()->InterruptOnTeleport();
            player->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TELEPORTED | AURA_INTERRUPT_FLAG_MOVE | AURA_INTERRUPT_FLAG_TURNING);

            Position const pos(x, y, z, o);
            if (!player->GetSession()->PlayerLogout())
                player->SendTeleportPacket(pos);
            player->UpdatePosition(pos, true);
            player->UpdateObjectVisibility();
            return true;
        }

        return player->TeleportTo(mapId, x, y, z, o, TELE_TO_GM_MODE);
    }

    bool ApplyDebugEnchant(Player* player, int32 enchantId, uint8 equipSlot)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, equipSlot);
        if (!item)
        {
            Notify(player, "你身上没有装备相应的物品。");
            return false;
        }

        for (uint8 slot = PERM_ENCHANTMENT_SLOT; slot <= TEMP_ENCHANTMENT_SLOT; ++slot)
        {
            EnchantmentSlot const enchantSlot = EnchantmentSlot(slot);
            uint32 const existing = item->GetEnchantmentId(enchantSlot);
            if (!existing)
                continue;

            player->ApplyEnchantment(item, enchantSlot, false, false);
            item->ClearEnchantment(enchantSlot);

            if (enchantId <= 0)
            {
                player->ApplyEnchantment(item, true);
                std::string const msg = Trinity::StringFormat("已清除附魔 ({})。", existing);
                Notify(player, msg.c_str());
                return true;
            }

            if (enchantSlot == PERM_ENCHANTMENT_SLOT)
            {
                item->SetEnchantment(TEMP_ENCHANTMENT_SLOT, existing, 0, 0, player->GetGUID());
                break;
            }
        }

        if (enchantId > 0)
        {
            item->SetEnchantment(PERM_ENCHANTMENT_SLOT, uint32(enchantId), 0, 0, player->GetGUID());
            player->CastSpell(player, 36937, true);
            player->ApplyEnchantment(item, true);
            player->SetHealth(player->GetMaxHealth());
            Notify(player, "已经附魔。");
        }

        return true;
    }
}

void SuperMenuAddon::SendToClient(Player* player, std::string const& payload)
{
    if (!player)
        return;

    WorldPackets::Chat::Chat packet;
    packet.Initialize(CHAT_MSG_WHISPER, LANG_ADDON, player, player, payload, 0, "", DEFAULT_LOCALE, std::string(PREFIX));
    player->SendDirectMessage(packet.Write());
}

void SuperMenuAddon::OpenUI(Player* player)
{
    if (!IsAuthorized(player))
        return;

    SendToClient(player, "UI_OPEN");
}

void SuperMenuAddon::OnPlayerLogout(Player* player)
{
    if (!player)
        return;
    ClearPlayerSuperMenuState(player->GetGUID().GetCounter());
}

bool SuperMenuAddon::TryHandleIncoming(Player* player, std::string_view message)
{
    if (!player)
        return false;

    std::string_view work = message;
    if (!TryStripAddonPrefix(work, PREFIX))
        return false;

    message = work;

    if (!IsAuthorized(player))
        return true;

    std::vector<std::string_view> parts = SplitSemicolon(message);
    if (parts.empty())
        return true;

    std::string_view const cmd = parts[0];

    if (cmd == "Q_LIST")
    {
        SendToClient(player, "Q_CLEAR");
        uint32 count = 0;

        for (uint32 questId : player->getRewardedQuests())
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest)
                continue;

            std::ostringstream ss;
            ss << "Q;" << questId << ";R;" << SanitizeQuestTitle(quest->GetTitle());
            SendToClient(player, ss.str());
            ++count;
        }

        for (auto const& [questId, status] : player->getQuestStatusMap())
        {
            if (status.Status != QUEST_STATUS_COMPLETE)
                continue;
            if (player->GetQuestRewardStatus(questId))
                continue;

            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest)
                continue;

            std::ostringstream ss;
            ss << "Q;" << questId << ";C;" << SanitizeQuestTitle(quest->GetTitle());
            SendToClient(player, ss.str());
            ++count;
        }

        SendToClient(player, "Q_DONE;" + std::to_string(count));
        return true;
    }

    if (cmd == "Q_RESET" && parts.size() >= 2)
    {
        uint32 done = 0;
        std::string_view ids = parts[1];
        while (!ids.empty())
        {
            size_t comma = ids.find(',');
            std::string_view token = comma == std::string_view::npos ? ids : ids.substr(0, comma);
            if (Optional<uint32> questId = Trinity::StringTo<uint32>(token))
            {
                uint32 const qid = *questId;
                if (player->GetQuestRewardStatus(qid))
                    player->RemoveRewardedQuest(qid);
                else if (player->GetQuestStatus(qid) == QUEST_STATUS_COMPLETE)
                    player->IncompleteQuest(qid);

                if (player->GetQuestStatus(qid) != QUEST_STATUS_NONE)
                {
                    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
                    {
                        if (player->GetQuestSlotQuestId(slot) == qid)
                        {
                            player->SetQuestSlot(slot, 0);
                            if (Quest const* quest = sObjectMgr->GetQuestTemplate(qid))
                            {
                                player->TakeQuestSourceItem(qid, false);
                                if (quest->HasFlag(QUEST_FLAGS_FLAGS_PVP))
                                {
                                    player->pvpInfo.IsHostile = player->pvpInfo.IsInHostileArea || player->HasPvPForcingQuest();
                                    player->UpdatePvPState();
                                }
                            }
                        }
                    }
                    player->RemoveActiveQuest(qid, false);
                }

                player->RemoveRewardedQuest(qid);
                sScriptMgr->OnQuestStatusChange(player, qid);
                ++done;
            }

            if (comma == std::string_view::npos)
                break;
            ids.remove_prefix(comma + 1);
        }

        std::ostringstream ss;
        ss << "Q_RESET_OK;" << done;
        SendToClient(player, ss.str());
        return true;
    }

    if (cmd == "Q_ADD" && parts.size() >= 2)
    {
        Optional<uint32> questId = Trinity::StringTo<uint32>(parts[1]);
        if (!questId)
        {
            Notify(player, "任务 ID 无效。");
            return true;
        }

        Quest const* quest = sObjectMgr->GetQuestTemplate(*questId);
        if (!quest || DisableMgr::IsDisabledFor(DISABLE_TYPE_QUEST, *questId, nullptr))
        {
            Notify(player, "找不到该任务或任务已禁用。");
            return true;
        }

        if (player->IsActiveQuest(*questId))
        {
            Notify(player, "你已有该任务。");
            return true;
        }

        if (player->CanAddQuest(quest, true))
        {
            player->AddQuestAndCheckCompletion(quest, nullptr);
            Notify(player, "已接取任务。");
            SendToClient(player, "Q_ADD_OK;" + std::to_string(*questId));
        }
        else
            Notify(player, "无法接取该任务（等级/前置等条件不满足）。");

        return true;
    }

    if (cmd == "ENC" && parts.size() >= 3)
    {
        Optional<int32> enchantId = Trinity::StringTo<int32>(parts[1]);
        Optional<uint8> equipSlot = Trinity::StringTo<uint8>(parts[2]);
        if (!enchantId || !equipSlot || *equipSlot >= EQUIPMENT_SLOT_END)
        {
            Notify(player, "附魔参数无效。");
            return true;
        }

        ApplyDebugEnchant(player, *enchantId, *equipSlot);
        return true;
    }

    if (cmd == "ACT" && parts.size() >= 2)
    {
        std::string_view const act = parts[1];

        if (act == "HEAL")
        {
            player->SetHealth(player->GetMaxHealth());
            if (player->GetPowerType() == POWER_MANA)
                player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));
            Notify(player, "生命值已回满。");
        }
        else if (act == "COMBAT_CLEAR")
        {
            player->CombatStop(true);
            player->ClearInCombat();
            Notify(player, "已脱离战斗。");
        }
        else if (act == "REPAIR")
        {
            player->DurabilityRepairAll(false, 1.f, false);
            Notify(player, "装备已修理。");
        }
        else if (act == "WEAK")
        {
            if (player->HasAura(15007))
            {
                player->RemoveAurasDueToSpell(15007);
                player->SetHealth(player->GetMaxHealth());
                Notify(player, "已移除复活虚弱。");
            }
            else
                Notify(player, "当前没有复活虚弱。");
        }
        else if (act == "BANK")
        {
            player->GetSession()->SendShowBank(player->GetGUID());
            Notify(player, "已打开银行。");
        }
        else if (act == "MAIL")
        {
            player->GetSession()->SendShowMailBox(player->GetGUID());
            Notify(player, "已打开邮箱。");
        }
        else if (act == "CD_RESET")
        {
            player->GetSpellHistory()->ResetAllCooldowns();
            Notify(player, "技能与物品冷却已重置。");
        }
        else if (act == "SAVE")
        {
            player->SaveToDB();
            Notify(player, "角色数据已保存。");
        }
        else if (act == "LOGOUT")
        {
            player->GetSession()->LogoutPlayer(true);
        }
        else if (act == "LOGOUT_NO_SAVE")
        {
            player->GetSession()->LogoutPlayer(false);
        }
        else if (act == "TALENT_RESET")
        {
            player->ResetTalents(true);
            Notify(player, "天赋已重置。");
        }
        else if (act == "BIND_HOME")
        {
            WorldLocation loc(player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation());
            player->SetHomebind(loc, player->GetAreaId());
            Notify(player, "已记录当前位置为炉石点。");
        }
        else if (act == "GO_HOME")
        {
            player->CastSpell(player, 8690, true);
            Notify(player, "正在传送回炉石点。");
        }
        else if (act == "UNBIND_INST")
        {
            uint32 const mapId = player->GetMapId();
            uint32 n = 0;
            for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
            {
                Player::BoundInstancesMap& binds = player->GetBoundInstances(Difficulty(i));
                for (Player::BoundInstancesMap::iterator itr = binds.begin(); itr != binds.end();)
                {
                    if (itr->first != mapId)
                    {
                        player->UnbindInstance(itr, Difficulty(i));
                        ++n;
                    }
                    else
                        ++itr;
                }
            }
            Notify(player, Trinity::StringFormat("已解除 {} 个副本绑定（不含当前地图）。", n));
        }

        return true;
    }

    if (cmd == "TP" && parts.size() >= 6)
    {
        Optional<uint32> mapId = Trinity::StringTo<uint32>(parts[1]);
        Optional<float> x = Trinity::StringTo<float>(parts[2]);
        Optional<float> y = Trinity::StringTo<float>(parts[3]);
        Optional<float> z = Trinity::StringTo<float>(parts[4]);
        Optional<float> o = Trinity::StringTo<float>(parts[5]);

        if (!mapId || !x || !y || !z || !o)
        {
            Notify(player, "传送参数无效。");
            return true;
        }

        if (!player->TeleportTo(*mapId, *x, *y, *z, *o, TELE_TO_GM_MODE))
            Notify(player, "传送失败。");
        else
            Notify(player, "传送成功。");

        return true;
    }

    if (cmd == "MARK" && parts.size() >= 3)
    {
        Optional<uint8> slot = Trinity::StringTo<uint8>(parts[1]);
        if (!slot || *slot < 1 || *slot > 5)
            return true;

        if (parts[2] == "SET")
        {
            SmuBookmark* mark = GetBookmarkSlot(player, *slot);
            if (!mark)
                return true;

            mark->mapId = player->GetMapId();
            mark->x = player->GetPositionX();
            mark->y = player->GetPositionY();
            mark->z = player->GetPositionZ();
            mark->o = player->GetOrientation();
            mark->valid = true;

            std::ostringstream ss;
            ss << "MARK;" << uint32(*slot) << ";"
               << mark->mapId << ';'
               << mark->x << ';'
               << mark->y << ';'
               << mark->z << ';'
               << mark->o;
            SendToClient(player, ss.str());
            Notify(player, Trinity::StringFormat("已记录 {} 号坐标。", *slot));
        }
        else if (parts[2] == "GO")
        {
            SmuBookmark* mark = GetBookmarkSlot(player, *slot);
            if (!mark)
                return true;

            uint32 mapId = 0;
            float x = 0.f;
            float y = 0.f;
            float z = 0.f;
            float o = 0.f;
            bool haveCoords = false;

            if (mark->valid)
            {
                mapId = mark->mapId;
                x = mark->x;
                y = mark->y;
                z = mark->z;
                o = mark->o;
                haveCoords = true;
            }
            else if (parts.size() >= 8)
            {
                // legacy: client-side bookmark cache, import once then use server storage
                Optional<uint32> parsedMap = Trinity::StringTo<uint32>(parts[3]);
                Optional<float> px = Trinity::StringTo<float>(parts[4]);
                Optional<float> py = Trinity::StringTo<float>(parts[5]);
                Optional<float> pz = Trinity::StringTo<float>(parts[6]);
                Optional<float> po = Trinity::StringTo<float>(parts[7]);
                if (parsedMap && px && py && pz && po)
                {
                    mapId = *parsedMap;
                    x = *px;
                    y = *py;
                    z = *pz;
                    o = *po;
                    haveCoords = true;
                }
            }

            if (!haveCoords)
            {
                Notify(player, Trinity::StringFormat("{} 号书签未铭刻。", *slot));
                return true;
            }

            mark->mapId = mapId;
            mark->x = x;
            mark->y = y;
            mark->z = z;
            mark->o = o;
            mark->valid = true;

            if (TeleportPlayerBookmark(player, mapId, x, y, z, o))
                Notify(player, Trinity::StringFormat("已折跃至 {} 号书签。", *slot));
            else if (mapId != player->GetMapId())
                Notify(player, "书签折跃失败。");
        }
        return true;
    }

    if (cmd == "SUMMON" && parts.size() >= 2)
    {
        Optional<uint32> entry = Trinity::StringTo<uint32>(parts[1]);
        if (!entry)
            return true;

        if (player->IsInCombat())
        {
            Notify(player, "战斗中无法召唤。");
            return true;
        }

        ObjectGuid::LowType const guidLow = player->GetGUID().GetCounter();
        uint32 const now = getMSTime();
        if (g_SummonCooldown[guidLow] > now)
        {
            Notify(player, "召唤冷却中，请稍后再试。");
            return true;
        }

        Map* map = player->GetMap();
        if (!map)
            return true;

        float x = player->GetPositionX() + 1.f;
        float y = player->GetPositionY();
        float z = player->GetPositionZ();
        float nz = map->GetHeight(player->GetPhaseMask(), x, y, z + 5.f, true);
        if (nz > z && nz < z + 5.f)
            z = nz;

        if (TempSummon* summon = player->SummonCreature(*entry, x, y, z, player->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 60s))
        {
            summon->SetFacingToObject(player);
            g_SummonCooldown[guidLow] = now + 60000;
            Notify(player, "召唤成功。");
        }
        else
            Notify(player, "召唤失败。");

        return true;
    }

    return true;
}
