/*
 * This file is part of the LWCore Project.
 * Ported from AzerothCore with modifications for TrinityCore framework.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "scourge_invasion.h"
#include "CellImpl.h"
#include "ChannelMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Containers.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "DatabaseEnv.h"
#include "GameEventMgr.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "Loot.h"
#include "LootMgr.h"
#include "Map.h"
#include "MapManager.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "Weather.h"
#include "WeatherMgr.h"
#include "World.h"
#include "WorldSession.h"

#include <chrono>
#include <map>
#include <mutex>
#include <set>

using namespace Trinity::ChatCommands;

using TimePoint = std::chrono::steady_clock::time_point;

/*
 * Scourge Invasion - design notes (mirrors the AzerothCore reference implementation):
 *
 *  - The manager only ever summons two creatures directly: the Herald of the Lich King
 *    ("Mouth of Kel'Thuzad") per invaded zone and the Pallid Horror for city attacks.
 *  - Everything else (Necropolis anchors/health/proxy/relay, minion finders, summoning
 *    circles, necropolis gameobjects, doodads) is spawned by the game event system
 *    (events 121-126) from game_event_creature / game_event_gameobject.
 *  - The purple communication chain runs through serverside/dbc spells with
 *    `conditions` rows restricting their implicit targets:
 *        Necropolis(16401) -28395 aura-> 28373 -> Proxy(16398) -> 28366 -> Relay(16386)
 *        -> 28326 -> Camp crystal (16136/16172)
 *    The death chain runs backwards (28351) and ends with the Necropolis Health
 *    NPC (16421) zapping itself to death, decrementing the zone counter.
 *  - Necrotic Shards (16136) are created by the Summoning Circle GO (181136, SmartAI
 *    casts 28344 on init). Minions are spawned through Minion Finder NPCs (16356)
 *    and Minion Spawner NPCs (16306/16336/16338), never directly by this script.
 */

// ===== Static zone/city definitions =====

struct InvasionZoneDef
{
    uint32 map;
    uint32 zoneId;
    uint32 necropolisCount;
    SIRemaining remainingIdx;
    SITimers timerIdx;
    uint32 gameEventId;
    Position mouth;
};

static InvasionZoneDef const g_invasionZoneDefs[] =
{
    { 1, AREA_WINTERSPRING,        3, SI_REMAINING_WINTERSPRING,        SI_TIMER_WINTERSPRING,        GAME_EVENT_SCOURGE_INVASION_WINTERSPRING,        { 7736.56f,  -4033.75f, 696.327f,  5.51524f  } },
    { 1, AREA_TANARIS,             3, SI_REMAINING_TANARIS,             SI_TIMER_TANARIS,             GAME_EVENT_SCOURGE_INVASION_TANARIS,             { -8352.68f, -3972.68f, 10.0753f,  2.14675f  } },
    { 1, AREA_AZSHARA,             2, SI_REMAINING_AZSHARA,             SI_TIMER_AZSHARA,             GAME_EVENT_SCOURGE_INVASION_AZSHARA,             { 3273.75f,  -4276.98f, 125.509f,  5.44543f  } },
    { 0, AREA_BLASTED_LANDS,       2, SI_REMAINING_BLASTED_LANDS,       SI_TIMER_BLASTED_LANDS,       GAME_EVENT_SCOURGE_INVASION_BLASTED_LANDS,       { -11429.3f, -3327.82f, 7.73628f,  1.0821f   } },
    { 0, AREA_EASTERN_PLAGUELANDS, 2, SI_REMAINING_EASTERN_PLAGUELANDS, SI_TIMER_EASTERN_PLAGUELANDS, GAME_EVENT_SCOURGE_INVASION_EASTERN_PLAGUELANDS, { 2014.55f,  -4934.52f, 73.9846f,  0.0698132f} },
    { 0, AREA_BURNING_STEPPES,     2, SI_REMAINING_BURNING_STEPPES,     SI_TIMER_BURNING_STEPPES,     GAME_EVENT_SCOURGE_INVASION_BURNING_STEPPES,     { -8229.53f, -1118.11f, 144.012f,  6.17846f  } },
};

struct CityAttackDef
{
    uint32 map;
    uint32 zoneId;
    SITimers timerIdx;
    Position pallid[2];
    uint32 path[2];
};

static CityAttackDef const g_cityAttackDefs[] =
{
    { 0, AREA_UNDERCITY, SI_TIMER_UNDERCITY,
        { { 1595.87f,  440.539f, -46.3349f, 2.28207f  },   // Royal Quarter
          { 1659.2f,   265.988f, -62.1788f, 3.64283f  } }, // Trade Quarter
        { PATH_UNDERCITY_ROYAL_QUARTER, PATH_UNDERCITY_TRADE_QUARTER } },
    { 0, AREA_STORMWIND, SI_TIMER_STORMWIND,
        { { -8578.15f, 886.382f, 87.3148f,  0.586275f },   // Stormwind Keep
          { -8578.15f, 886.382f, 87.3148f,  0.586275f } }, // Trade District (same spawn, other path)
        { PATH_STORMWIND_KEEP, PATH_STORMWIND_TRADE_DISTRICT } },
};

static InvasionZoneDef const* FindInvasionZoneByZoneId(uint32 zoneId)
{
    for (InvasionZoneDef const& def : g_invasionZoneDefs)
        if (def.zoneId == zoneId)
            return &def;
    return nullptr;
}

static CityAttackDef const* FindCityAttackByZoneId(uint32 zoneId)
{
    for (CityAttackDef const& def : g_cityAttackDefs)
        if (def.zoneId == zoneId)
            return &def;
    return nullptr;
}

// Communique chain helpers — explicit targets prevent area-lightning from hitting bystanders.
static bool IsNecropolisGameObjectEntry(uint32 entry)
{
    return entry == GO_NECROPOLIS_TINY || entry == GO_NECROPOLIS_SMALL || entry == GO_NECROPOLIS_MEDIUM
        || entry == GO_NECROPOLIS_BIG || entry == GO_NECROPOLIS_HUGE;
}

static Creature* GetClosestNecroticShard(WorldObject* source, float range)
{
    if (Creature* shard = GetClosestCreatureWithEntry(source, NPC_NECROTIC_SHARD, range))
        return shard;
    return GetClosestCreatureWithEntry(source, NPC_DAMAGED_NECROTIC_SHARD, range);
}

// TrinityCore requires explicit activation for off-grid AI; wander distance matches Acore (1.0f).
static void ActivateScourgeMinion(Creature* minion)
{
    if (!minion)
        return;

    minion->setActive(true);
    minion->SetWanderDistance(1.0f);
    minion->SetCorpseDelay(60);
}

static bool IsScourgeInvasionMinionEntry(uint32 entry)
{
    switch (entry)
    {
        case NPC_SKELETAL_SHOCKTROOPER:
        case NPC_GHOUL_BERSERKER:
        case NPC_SPECTRAL_SOLDIER:
        case NPC_LUMBERING_HORROR:
        case NPC_BONE_WITCH:
        case NPC_SPIRIT_OF_THE_DAMNED:
            return true;
        default:
            return false;
    }
}

static bool IsScourgeCommuniqueTarget(WorldObject* obj, uint32 spellId)
{
    if (!obj)
        return false;

    if (GameObject* go = obj->ToGameObject())
        return spellId == SPELL_COMMUNIQUE_NECROPOLIS_TO_PROXIES && IsNecropolisGameObjectEntry(go->GetEntry());

    Creature* creature = obj->ToCreature();
    if (!creature)
        return false;

    uint32 entry = creature->GetEntry();
    switch (spellId)
    {
        case SPELL_COMMUNIQUE_NECROPOLIS_TO_PROXIES: return entry == NPC_NECROPOLIS_PROXY;
        case SPELL_COMMUNIQUE_PROXY_TO_RELAY:      return entry == NPC_NECROPOLIS_RELAY;
        case SPELL_COMMUNIQUE_RELAY_TO_CAMP:       return entry == NPC_NECROTIC_SHARD || entry == NPC_DAMAGED_NECROTIC_SHARD;
        case SPELL_COMMUNIQUE_RELAY_TO_PROXY:      return entry == NPC_NECROPOLIS_PROXY;
        case SPELL_COMMUNIQUE_PROXY_TO_NECROPOLIS: return entry == NPC_NECROPOLIS;
        case SPELL_COMMUNIQUE_CAMP_TO_RELAY:       return entry == NPC_NECROPOLIS_RELAY;
        case SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH: return entry == NPC_NECROPOLIS_RELAY || entry == NPC_NECROPOLIS_PROXY || entry == NPC_NECROPOLIS_HEALTH;
        case SPELL_ZAP_NECROPOLIS:                 return entry == NPC_NECROPOLIS_HEALTH;
        case SPELL_ZAP_CRYSTAL:                      return entry == NPC_NECROTIC_SHARD;
        case SPELL_ZAP_CRYSTAL_CORPSE:               return entry == NPC_NECROTIC_SHARD || entry == NPC_DAMAGED_NECROTIC_SHARD;
        case SPELL_DAMAGE_CRYSTAL:                 return entry == NPC_NECROTIC_SHARD || entry == NPC_DAMAGED_NECROTIC_SHARD;
        case SPELL_DAMAGE_VS_GUARDS:               return entry == 1756 || entry == NPC_ROYAL_DREADGUARD; // city attack event
        default:                                     return false;
    }
}

static bool IsPlayerOrCompanion(WorldObject* obj)
{
    if (!obj)
        return false;

    if (obj->IsPlayer())
        return true;

    Unit const* unit = obj->ToUnit();
    if (!unit)
        return false;

    if (unit->IsPet() || unit->IsCharmedOwnedByPlayerOrPlayer())
        return true;

    if (Creature const* creature = unit->ToCreature())
        if (creature->IsNPCBotOrPet())
            return true;

    return false;
}

// Scourge Strike (28265) must never hit players, bots, guards, or other friendly/neutral NPCs.
static bool IsProtectedFromScourgeStrike(WorldObject* obj)
{
    if (!obj || IsPlayerOrCompanion(obj))
        return true;

    Creature const* creature = obj->ToCreature();
    if (!creature)
        return true;

    if (creature->IsGuard())
        return true;

    return !IsScourgeInvasionMinionEntry(creature->GetEntry());
}

// Summoned camp minions may miss loot when Unit::Kill skips generation; fix before corpse is opened.
static void EnsureScourgeInvasionMinionLoot(Creature* creature, Player* player)
{
    if (!creature || !player)
        return;

    if (!creature->hasLootRecipient())
        creature->SetLootRecipient(player, true);

    Loot* loot = &creature->loot;
    if (loot->loot_type != LOOT_NONE && !loot->isLooted())
        return;

    loot->clear();
    loot->loot_type = LOOT_CORPSE;

    if (uint32 lootId = creature->GetCreatureTemplate()->lootid)
        loot->FillLoot(lootId, LootTemplates_Creature, player, false, false, creature->GetLootMode());

    if (creature->GetLootMode() > 0)
        loot->generateMoneyLoot(creature->GetCreatureTemplate()->mingold, creature->GetCreatureTemplate()->maxgold);

    if (!loot->isLooted())
        creature->SetDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
}

static bool IsValidScourgeInvasionBoltTarget(WorldObject* obj, uint32 spellId)
{
    if (!obj || IsPlayerOrCompanion(obj))
        return false;

    return IsScourgeCommuniqueTarget(obj, spellId);
}

// ===== Scourge Invasion Manager =====

class ScourgeInvasionMgr
{
public:
    static ScourgeInvasionMgr* instance()
    {
        static ScourgeInvasionMgr instance;
        return &instance;
    }

    // ---- persistence ----

    void LoadFromDB()
    {
        QueryResult result = WorldDatabase.Query("SELECT zoneId, attackTimer, remainingNecropoli, battlesWon, lastAttackZone, state FROM scourge_invasion_state");
        if (!result)
            return;

        std::lock_guard<std::mutex> guard(_mutex);
        TimePoint now = std::chrono::steady_clock::now();
        do
        {
            Field* fields = result->Fetch();
            uint32 zoneId = fields[0].GetUInt32();
            uint32 attackTimer = fields[1].GetUInt32();
            uint32 remaining = fields[2].GetUInt32();
            _battlesWon = fields[3].GetUInt32();
            _lastAttackZone = fields[4].GetUInt32();
            _state = SIState(fields[5].GetUInt32());

            if (InvasionZoneDef const* def = FindInvasionZoneByZoneId(zoneId))
            {
                _remaining[def->remainingIdx] = remaining;
                if (attackTimer)
                    _timers[def->timerIdx] = now + std::chrono::seconds(attackTimer);
                else if (remaining > 0)
                    _timers[def->timerIdx] = now + std::chrono::seconds(ZONE_ATTACK_TIMER_MAX);
            }
            else if (CityAttackDef const* city = FindCityAttackByZoneId(zoneId))
            {
                if (attackTimer)
                    _timers[city->timerIdx] = now + std::chrono::seconds(attackTimer);
            }
        } while (result->NextRow());
    }

    void SaveToDB()
    {
        uint32 battlesWon, lastAttackZone, state;
        uint32 remaining[SI_REMAINING_MAX];
        uint32 timerSecs[SI_TIMER_MAX];
        {
            std::lock_guard<std::mutex> guard(_mutex);
            battlesWon = _battlesWon;
            lastAttackZone = _lastAttackZone;
            state = uint32(_state);
            std::copy(std::begin(_remaining), std::end(_remaining), remaining);

            TimePoint now = std::chrono::steady_clock::now();
            for (uint32 i = 0; i < SI_TIMER_MAX; ++i)
            {
                timerSecs[i] = 0;
                if (_timers[i] != TimePoint())
                {
                    int64 secs = std::chrono::duration_cast<std::chrono::seconds>(_timers[i] - now).count();
                    if (secs > 0)
                        timerSecs[i] = uint32(secs);
                }
            }
        }

        WorldDatabase.Execute("DELETE FROM scourge_invasion_state");
        for (InvasionZoneDef const& def : g_invasionZoneDefs)
            WorldDatabase.Execute(fmt::format("INSERT INTO scourge_invasion_state (zoneId, attackTimer, remainingNecropoli, battlesWon, lastAttackZone, state) VALUES ({}, {}, {}, {}, {}, {})",
                def.zoneId, timerSecs[def.timerIdx], remaining[def.remainingIdx], battlesWon, lastAttackZone, state).c_str());
        for (CityAttackDef const& def : g_cityAttackDefs)
            WorldDatabase.Execute(fmt::format("INSERT INTO scourge_invasion_state (zoneId, attackTimer, remainingNecropoli, battlesWon, lastAttackZone, state) VALUES ({}, {}, 0, {}, {}, {})",
                def.zoneId, timerSecs[def.timerIdx], battlesWon, lastAttackZone, state).c_str());
    }

    // ---- state accessors (thread-safe, may be called from map update threads) ----

    SIState GetState() const
    {
        std::lock_guard<std::mutex> guard(_mutex);
        return _state;
    }

    uint32 GetSIRemaining(SIRemaining idx) const
    {
        std::lock_guard<std::mutex> guard(_mutex);
        return _remaining[idx];
    }

    void SetSIRemaining(SIRemaining idx, uint32 value)
    {
        {
            std::lock_guard<std::mutex> guard(_mutex);
            _remaining[idx] = value;
        }
        SaveToDB();
    }

    uint32 GetSIRemainingByZone(uint32 zoneId) const
    {
        InvasionZoneDef const* def = FindInvasionZoneByZoneId(zoneId);
        return def ? GetSIRemaining(def->remainingIdx) : 0;
    }

    TimePoint GetSITimer(SITimers idx) const
    {
        std::lock_guard<std::mutex> guard(_mutex);
        return _timers[idx];
    }

    void SetSITimer(SITimers idx, TimePoint tp)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        _timers[idx] = tp;
    }

    uint32 GetBattlesWon() const
    {
        std::lock_guard<std::mutex> guard(_mutex);
        return _battlesWon;
    }

    uint32 GetLastAttackZone() const
    {
        std::lock_guard<std::mutex> guard(_mutex);
        return _lastAttackZone;
    }

    void SetLastAttackZone(uint32 zoneId)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        _lastAttackZone = zoneId;
    }

    ObjectGuid GetMouthGuid(uint32 zoneId) const
    {
        std::lock_guard<std::mutex> guard(_mutex);
        auto itr = _mouthGuids.find(zoneId);
        return itr != _mouthGuids.end() ? itr->second : ObjectGuid::Empty;
    }

    void SetMouthGuid(uint32 zoneId, ObjectGuid guid)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        _mouthGuids[zoneId] = guid;
    }

    ObjectGuid GetPallidGuid(uint32 zoneId) const
    {
        std::lock_guard<std::mutex> guard(_mutex);
        auto itr = _pallidGuids.find(zoneId);
        return itr != _pallidGuids.end() ? itr->second : ObjectGuid::Empty;
    }

    void SetPallidGuid(uint32 zoneId, ObjectGuid guid)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        _pallidGuids[zoneId] = guid;
    }

    // Called from the world thread only (HandleActiveZone).
    void AddBattlesWon(int32 count)
    {
        {
            std::lock_guard<std::mutex> guard(_mutex);
            _battlesWon += count;
        }
        HandleDefendedZones();
        SaveToDB();
    }

    // ---- main control (world thread only) ----

    void SetState(SIState state)
    {
        SIState oldState;
        {
            std::lock_guard<std::mutex> guard(_mutex);
            oldState = _state;
            if (oldState == state)
                return;
            _state = state;
        }

        if (oldState == SI_STATE_DISABLED)
            StartScourgeInvasion();
        else if (state == SI_STATE_DISABLED)
            StopScourgeInvasion();
        SaveToDB();
    }

    void StartScourgeInvasion()
    {
        TC_LOG_INFO("gameevent", "[ScourgeInvasion] Starting Scourge Invasion.");

        if (!sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION))
            sGameEventMgr->StartEvent(GAME_EVENT_SCOURGE_INVASION, true);

        BroadcastWorldStates();

        for (CityAttackDef const& def : g_cityAttackDefs)
            StartNewCityAttackIfTime(def.zoneId);

        // Randomize zone init order so every invasion cycle starts differently
        std::vector<uint32> zoneIds;
        zoneIds.reserve(std::size(g_invasionZoneDefs));
        for (InvasionZoneDef const& def : g_invasionZoneDefs)
            zoneIds.push_back(def.zoneId);
        Trinity::Containers::RandomShuffle(zoneIds);

        for (uint32 zoneId : zoneIds)
        {
            InvasionZoneDef const* def = FindInvasionZoneByZoneId(zoneId);
            if (GetSIRemaining(def->remainingIdx) > 0)
                ResumeInvasion(*def);
            else
                StartNewInvasionIfTime(zoneId);
        }

        if (!sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_BOSSES))
            sGameEventMgr->StartEvent(GAME_EVENT_SCOURGE_INVASION_BOSSES, true);

        HandleDefendedZones();
    }

    void StopScourgeInvasion()
    {
        TC_LOG_INFO("gameevent", "[ScourgeInvasion] Stopping Scourge Invasion.");

        uint16 const events[] =
        {
            GAME_EVENT_SCOURGE_INVASION,
            GAME_EVENT_SCOURGE_INVASION_WINTERSPRING, GAME_EVENT_SCOURGE_INVASION_TANARIS,
            GAME_EVENT_SCOURGE_INVASION_AZSHARA, GAME_EVENT_SCOURGE_INVASION_BLASTED_LANDS,
            GAME_EVENT_SCOURGE_INVASION_EASTERN_PLAGUELANDS, GAME_EVENT_SCOURGE_INVASION_BURNING_STEPPES,
            GAME_EVENT_SCOURGE_INVASION_INVASIONS_DONE, GAME_EVENT_SCOURGE_INVASION_BOSSES
        };
        for (uint16 eventId : events)
            if (sGameEventMgr->IsActiveEvent(eventId))
                sGameEventMgr->StopEvent(eventId, true);

        // Despawn mouths and pallids
        for (InvasionZoneDef const& def : g_invasionZoneDefs)
        {
            if (Map* map = sMapMgr->FindMap(def.map, 0))
                if (Creature* mouth = map->GetCreature(GetMouthGuid(def.zoneId)))
                    mouth->DespawnOrUnsummon();
            SetMouthGuid(def.zoneId, ObjectGuid::Empty);
        }
        for (CityAttackDef const& def : g_cityAttackDefs)
        {
            if (Map* map = sMapMgr->FindMap(def.map, 0))
                if (Creature* pallid = map->GetCreature(GetPallidGuid(def.zoneId)))
                    pallid->DespawnOrUnsummon();
            SetPallidGuid(def.zoneId, ObjectGuid::Empty);
        }

        {
            std::lock_guard<std::mutex> guard(_mutex);
            for (TimePoint& timer : _timers)
                timer = TimePoint();
            memset(_remaining, 0, sizeof(_remaining));
        }

        BroadcastWorldStates();
    }

    // Driven by the WorldScript, world thread, real diff. Internal 10s tick.
    void Update(uint32 diff)
    {
        if (GetState() != SI_STATE_ENABLED)
            return;

        if (_broadcastTimer > diff)
        {
            _broadcastTimer -= diff;
            return;
        }
        _broadcastTimer = 10000;

        BroadcastWorldStates();

        // Keep global scourge patrol (event #17) alive; its DB length is only 1 minute.
        if (!sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION))
            sGameEventMgr->StartEvent(GAME_EVENT_SCOURGE_INVASION, true);

        for (CityAttackDef const& def : g_cityAttackDefs)
            StartNewCityAttackIfTime(def.zoneId);

        TimePoint now = std::chrono::steady_clock::now();
        for (InvasionZoneDef const& def : g_invasionZoneDefs)
            HandleActiveZone(def, now);
    }

    void BroadcastWorldStates()
    {
        uint32 victories = GetBattlesWon();
        uint32 remainingAzshara = GetSIRemaining(SI_REMAINING_AZSHARA);
        uint32 remainingBlastedLands = GetSIRemaining(SI_REMAINING_BLASTED_LANDS);
        uint32 remainingBurningSteppes = GetSIRemaining(SI_REMAINING_BURNING_STEPPES);
        uint32 remainingEasternPlaguelands = GetSIRemaining(SI_REMAINING_EASTERN_PLAGUELANDS);
        uint32 remainingTanaris = GetSIRemaining(SI_REMAINING_TANARIS);
        uint32 remainingWinterspring = GetSIRemaining(SI_REMAINING_WINTERSPRING);

        auto sendToMap = [&](Map* map)
        {
            Map::PlayerList const& players = map->GetPlayers();
            for (auto itr = players.begin(); itr != players.end(); ++itr)
            {
                Player* player = itr->GetSource();
                if (!player || !player->IsInWorld())
                    continue;

                player->SendUpdateWorldState(WORLD_STATE_SCOURGE_INVASION_AZSHARA, remainingAzshara > 0 ? 1 : 0);
                player->SendUpdateWorldState(WORLD_STATE_SCOURGE_INVASION_BLASTED_LANDS, remainingBlastedLands > 0 ? 1 : 0);
                player->SendUpdateWorldState(WORLD_STATE_SCOURGE_INVASION_BURNING_STEPPES, remainingBurningSteppes > 0 ? 1 : 0);
                player->SendUpdateWorldState(WORLD_STATE_SCOURGE_INVASION_EASTERN_PLAGUELANDS, remainingEasternPlaguelands > 0 ? 1 : 0);
                player->SendUpdateWorldState(WORLD_STATE_SCOURGE_INVASION_TANARIS, remainingTanaris > 0 ? 1 : 0);
                player->SendUpdateWorldState(WORLD_STATE_SCOURGE_INVASION_WINTERSPRING, remainingWinterspring > 0 ? 1 : 0);
                player->SendUpdateWorldState(WORLD_STATE_SCOURGE_INVASION_VICTORIES, victories);
                player->SendUpdateWorldState(WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_AZSHARA, remainingAzshara);
                player->SendUpdateWorldState(WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_BLASTED_LANDS, remainingBlastedLands);
                player->SendUpdateWorldState(WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_BURNING_STEPPES, remainingBurningSteppes);
                player->SendUpdateWorldState(WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_EASTERN_PLAGUELANDS, remainingEasternPlaguelands);
                player->SendUpdateWorldState(WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_TANARIS, remainingTanaris);
                player->SendUpdateWorldState(WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_WINTERSPRING, remainingWinterspring);
            }
        };

        sMapMgr->DoForAllMapsWithMapId(0, sendToMap);
        sMapMgr->DoForAllMapsWithMapId(1, sendToMap);
    }

    void HandleDefendedZones()
    {
        uint32 battlesWon = GetBattlesWon();
        if (battlesWon < 50)
        {
            if (sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_50_INVASIONS))
                sGameEventMgr->StopEvent(GAME_EVENT_SCOURGE_INVASION_50_INVASIONS, true);
            if (sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_100_INVASIONS))
                sGameEventMgr->StopEvent(GAME_EVENT_SCOURGE_INVASION_100_INVASIONS, true);
            if (sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_150_INVASIONS))
                sGameEventMgr->StopEvent(GAME_EVENT_SCOURGE_INVASION_150_INVASIONS, true);
        }
        else if (battlesWon < 100)
        {
            if (!sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_50_INVASIONS))
                sGameEventMgr->StartEvent(GAME_EVENT_SCOURGE_INVASION_50_INVASIONS, true);
        }
        else if (battlesWon < 150)
        {
            if (sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_50_INVASIONS))
                sGameEventMgr->StopEvent(GAME_EVENT_SCOURGE_INVASION_50_INVASIONS, true);
            if (!sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_100_INVASIONS))
                sGameEventMgr->StartEvent(GAME_EVENT_SCOURGE_INVASION_100_INVASIONS, true);
        }
        else
        {
            if (sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_50_INVASIONS))
                sGameEventMgr->StopEvent(GAME_EVENT_SCOURGE_INVASION_50_INVASIONS, true);
            if (sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_100_INVASIONS))
                sGameEventMgr->StopEvent(GAME_EVENT_SCOURGE_INVASION_100_INVASIONS, true);
            if (!sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_150_INVASIONS))
                sGameEventMgr->StartEvent(GAME_EVENT_SCOURGE_INVASION_150_INVASIONS, true);
            if (!sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_INVASIONS_DONE))
                sGameEventMgr->StartEvent(GAME_EVENT_SCOURGE_INVASION_INVASIONS_DONE, true);
        }
    }

    // ---- invasion zone handling (world thread only) ----

    void AddPendingInvasion(uint32 zoneId)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        _pendingInvasions.insert(zoneId);
    }

    void RemovePendingInvasion(uint32 zoneId)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        _pendingInvasions.erase(zoneId);
    }

    // Mirrors Acore: always false — duplicate-start prevention uses mouthGuid on the map.
    bool IsActiveZone(uint32 /*zoneId*/) const
    {
        return false;
    }

    uint32 GetActiveZones()
    {
        std::lock_guard<std::mutex> guard(_mutex);
        uint32 count = uint32(_pendingInvasions.size());
        for (InvasionZoneDef const& def : g_invasionZoneDefs)
        {
            ObjectGuid mouthGuid = _mouthGuids.count(def.zoneId) ? _mouthGuids.at(def.zoneId) : ObjectGuid::Empty;
            if (mouthGuid.IsEmpty())
                continue;

            Map* map = sMapMgr->FindMap(def.map, 0);
            if (map && map->GetCreature(mouthGuid))
                ++count;
        }
        return count;
    }

    void StartNewInvasionIfTime(uint32 zoneId)
    {
        InvasionZoneDef const* def = FindInvasionZoneByZoneId(zoneId);
        if (!def)
            return;

        if (std::chrono::steady_clock::now() < GetSITimer(def->timerIdx))
            return;

        StartNewInvasion(zoneId);
    }

    void StartNewInvasion(uint32 zoneId)
    {
        InvasionZoneDef const* def = FindInvasionZoneByZoneId(zoneId);
        if (!def)
            return;

        if (IsActiveZone(zoneId))
            return;

        // Don't attack the same zone as before.
        if (zoneId == GetLastAttackZone())
            return;

        // After the first victory never run more than 2 simultaneous invasions.
        if (GetActiveZones() > 1 && GetBattlesWon() > 0)
            return;

        Map* map = sMapMgr->CreateBaseMap(def->map);
        if (!map)
        {
            TC_LOG_ERROR("gameevent", "[ScourgeInvasion] StartNewInvasion unable to access map {}, retrying next tick.", def->map);
            return;
        }

        TC_LOG_INFO("gameevent", "[ScourgeInvasion] Starting new invasion in zone {}.", zoneId);

        if (!sGameEventMgr->IsActiveEvent(def->gameEventId))
            sGameEventMgr->StartEvent(def->gameEventId, true);

        SummonMouth(map, *def, true);
        BroadcastWorldStates();
        SaveToDB();
    }

    bool ResumeInvasion(InvasionZoneDef const& def)
    {
        uint32 const numRemaining = GetSIRemaining(def.remainingIdx);
        TC_LOG_INFO("gameevent", "[ScourgeInvasion] Resuming invasion in zone {} ({} necropolises remaining).", def.zoneId, numRemaining);

        for (uint32 i = 0; i < numRemaining; ++i)
        {
            if (!sMapMgr->CreateBaseMap(def.map))
            {
                TC_LOG_ERROR("gameevent", "[ScourgeInvasion] ResumeInvasion unable to access map {}, retrying next tick.", def.map);
                return false;
            }
        }

        Map* map = sMapMgr->CreateBaseMap(def.map);
        if (!map)
            return false;

        return SummonMouth(map, def, false);
    }

    bool SummonMouth(Map* map, InvasionZoneDef const& def, bool newInvasion)
    {
        AddPendingInvasion(def.zoneId);

        if (Creature* existingMouth = map->GetCreature(GetMouthGuid(def.zoneId)))
            existingMouth->DespawnOrUnsummon();

        bool success = false;
        if (Creature* mouth = map->SummonCreature(NPC_HERALD_OF_THE_LICH_KING, def.mouth))
        {
            SetMouthGuid(def.zoneId, mouth->GetGUID());
            if (newInvasion)
            {
                SetSIRemaining(def.remainingIdx, def.necropolisCount);
                SetSITimer(def.timerIdx, std::chrono::steady_clock::now() + std::chrono::seconds(urand(ZONE_ATTACK_TIMER_MIN, ZONE_ATTACK_TIMER_MAX)));
            }
            else if (GetSIRemaining(def.remainingIdx) > 0 && GetSITimer(def.timerIdx) == TimePoint())
                SetSITimer(def.timerIdx, std::chrono::steady_clock::now() + std::chrono::seconds(ZONE_ATTACK_TIMER_MAX));

            mouth->AI()->DoAction(EVENT_HERALD_OF_THE_LICH_KING_ZONE_START);
            success = true;
        }
        else
            TC_LOG_ERROR("gameevent", "[ScourgeInvasion] Failed to summon Herald of the Lich King in zone {}.", def.zoneId);

        RemovePendingInvasion(def.zoneId);
        return success;
    }

    void HandleActiveZone(InvasionZoneDef const& def, TimePoint now)
    {
        TimePoint timer = GetSITimer(def.timerIdx);
        uint32 remaining = GetSIRemaining(def.remainingIdx);
        ObjectGuid mouthGuid = GetMouthGuid(def.zoneId);
        Map* map = sMapMgr->FindMap(def.map, 0);

        TimePoint nextAttack = now + std::chrono::seconds(urand(ZONE_ATTACK_TIMER_MIN, ZONE_ATTACK_TIMER_MAX));

        // Remaining > 0 but the per-zone game event stopped (e.g. crash/restart desync).
        if (remaining > 0 && !sGameEventMgr->IsActiveEvent(def.gameEventId))
        {
            if (!mouthGuid.IsEmpty() && map && map->GetCreature(mouthGuid))
                sGameEventMgr->StartEvent(def.gameEventId, true);
            else
                ResumeInvasion(def);
        }

        if (!mouthGuid.IsEmpty())
        {
            Creature* mouth = map ? map->GetCreature(mouthGuid) : nullptr;
            if (!mouth)
                SetMouthGuid(def.zoneId, ObjectGuid::Empty); // re-summon handled next tick
            else if (timer != TimePoint() && timer < now && remaining == 0)
            {
                // Zone defended: all necropolises destroyed.
                SetSITimer(def.timerIdx, nextAttack);
                AddBattlesWon(1);
                SetLastAttackZone(def.zoneId);
                SetMouthGuid(def.zoneId, ObjectGuid::Empty);
                mouth->AI()->DoAction(EVENT_HERALD_OF_THE_LICH_KING_ZONE_STOP);

                TC_LOG_INFO("gameevent", "[ScourgeInvasion] The Scourge has been defeated in zone {} ({} victories).", def.zoneId, GetBattlesWon());

                BroadcastWorldStates();
                SaveToDB();
            }
        }
        else
        {
            // If more than one zone is already being attacked, push the timer.
            if (GetActiveZones() > 1)
                SetSITimer(def.timerIdx, nextAttack);

            if (remaining > 0)
            {
                // Throttle resume attempts (mouth guid is memory-only and lost on restart).
                auto itr = _zoneResumeCooldown.find(def.zoneId);
                if (itr == _zoneResumeCooldown.end() || itr->second <= now)
                {
                    _zoneResumeCooldown[def.zoneId] = now + std::chrono::minutes(2);
                    ResumeInvasion(def);
                }
            }
            else
                StartNewInvasionIfTime(def.zoneId);
        }
    }

    // ---- city attacks (world thread only) ----

    void StartNewCityAttackIfTime(uint32 zoneId)
    {
        CityAttackDef const* def = FindCityAttackByZoneId(zoneId);
        if (!def)
            return;

        TimePoint now = std::chrono::steady_clock::now();
        if (now < GetSITimer(def->timerIdx))
            return;

        if (StartNewCityAttack(zoneId))
            SetSITimer(def->timerIdx, now + std::chrono::seconds(urand(CITY_ATTACK_TIMER_MIN, CITY_ATTACK_TIMER_MAX)));
    }

    bool StartNewCityAttack(uint32 zoneId)
    {
        CityAttackDef const* def = FindCityAttackByZoneId(zoneId);
        if (!def)
            return false;

        Map* map = sMapMgr->CreateBaseMap(def->map);
        if (!map)
            return false;

        uint32 spawnIdx = urand(0, 1);

        if (Creature* existingPallid = map->GetCreature(GetPallidGuid(zoneId)))
            existingPallid->DespawnOrUnsummon();

        Creature* pallid = map->SummonCreature(NPC_PALLID_HORROR, def->pallid[spawnIdx]);
        if (!pallid)
        {
            TC_LOG_ERROR("gameevent", "[ScourgeInvasion] Failed to summon Pallid Horror in zone {}.", zoneId);
            return false;
        }

        pallid->GetMotionMaster()->Clear();
        pallid->GetMotionMaster()->MovePath(def->path[spawnIdx], false);
        SetPallidGuid(zoneId, pallid->GetGUID());

        TC_LOG_INFO("gameevent", "[ScourgeInvasion] City attack started in zone {}.", zoneId);
        SaveToDB();
        return true;
    }

    // Called from npc_pallid_horror::JustDied (map thread): only state updates, no game event calls.
    void OnPallidDeath(uint32 zoneId)
    {
        CityAttackDef const* def = FindCityAttackByZoneId(zoneId);
        if (!def)
            return;

        SetSITimer(def->timerIdx, std::chrono::steady_clock::now() + std::chrono::seconds(urand(CITY_ATTACK_TIMER_MIN, CITY_ATTACK_TIMER_MAX)));
        SetPallidGuid(zoneId, ObjectGuid::Empty);
        SaveToDB();
    }

private:
    ScourgeInvasionMgr() = default;

    mutable std::mutex _mutex;
    SIState _state = SI_STATE_DISABLED;
    TimePoint _timers[SI_TIMER_MAX];
    uint32 _battlesWon = 0;
    uint32 _lastAttackZone = 0;
    uint32 _remaining[SI_REMAINING_MAX] = {};
    uint32 _broadcastTimer = 10000;
    std::map<uint32, ObjectGuid> _mouthGuids;
    std::map<uint32, ObjectGuid> _pallidGuids;
    std::set<uint32> _pendingInvasions;
    std::map<uint32, TimePoint> _zoneResumeCooldown;
};

#define sScourgeInvasionMgr ScourgeInvasionMgr::instance()

// ===== GO: Necropolis =====

struct go_necropolis : public GameObjectAI
{
    go_necropolis(GameObject* go) : GameObjectAI(go)
    {
        me->setActive(true);
    }
};

// ===== NPC: Herald of the Lich King (Mouth of Kel'Thuzad) =====

struct npc_herald_of_the_lich_king : public ScriptedAI
{
    npc_herald_of_the_lich_king(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void InitializeAI() override
    {
        me->setActive(true);
        _scheduler.Schedule(Minutes(5), [this](TaskContext context)
        {
            Talk(HERALD_OF_THE_LICH_KING_SAY_ATTACK_RANDOM);
            context.Repeat(Minutes(15), Minutes(30));
        });
    }

    void DoAction(int32 action) override
    {
        if (action == EVENT_HERALD_OF_THE_LICH_KING_ZONE_START)
        {
            Talk(HERALD_OF_THE_LICH_KING_SAY_ATTACK_START);
            ChangeZoneEventStatus(true);
            UpdateWeather(true);
        }
        else if (action == EVENT_HERALD_OF_THE_LICH_KING_ZONE_STOP)
        {
            Talk(HERALD_OF_THE_LICH_KING_SAY_ATTACK_END);
            ChangeZoneEventStatus(false);
            UpdateWeather(false);
            me->DespawnOrUnsummon();
        }
    }

    // Only invoked through DoAction from the manager (world thread): safe to touch GameEventMgr.
    void ChangeZoneEventStatus(bool start)
    {
        InvasionZoneDef const* def = FindInvasionZoneByZoneId(me->GetZoneId());
        if (!def)
            return;

        if (start)
        {
            if (!sGameEventMgr->IsActiveEvent(def->gameEventId))
                sGameEventMgr->StartEvent(def->gameEventId, true);
        }
        else if (sGameEventMgr->IsActiveEvent(def->gameEventId))
            sGameEventMgr->StopEvent(def->gameEventId, true);
    }

    void UpdateWeather(bool start)
    {
        if (Weather* weather = me->GetMap()->GetOrGenerateZoneDefaultWeather(me->GetZoneId()))
            weather->SetWeather(start ? WEATHER_TYPE_STORM : WEATHER_TYPE_RAIN, start ? 0.25f : 0.0f);
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// ===== NPC: Necropolis (invisible anchor below the necropolis GO) =====

struct npc_necropolis : public ScriptedAI
{
    npc_necropolis(Creature* creature) : ScriptedAI(creature)
    {
        me->setActive(true);
    }

    void InitializeAI() override
    {
        // Start the purple lightning pulse shortly after the necropolis anchor spawns.
        _communiqueTimer = 5000;
    }

    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spell) override
    {
        // Proxy acknowledged the chain; keep the pulse running (no timer aura — it auto-casts 28373 via AoE).
        if (spell->Id == SPELL_COMMUNIQUE_PROXY_TO_NECROPOLIS && !_communiqueTimer)
            _communiqueTimer = 15000;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!_communiqueTimer)
            return;

        if (_communiqueTimer <= diff)
        {
            _communiqueTimer = 15000;
            if (Creature* proxy = GetClosestCreatureWithEntry(me, NPC_NECROPOLIS_PROXY, 200.0f))
                me->CastSpell(proxy, SPELL_COMMUNIQUE_NECROPOLIS_TO_PROXIES, true);
        }
        else
            _communiqueTimer -= diff;
    }

private:
    uint32 _communiqueTimer = 0;
};

// ===== NPC: Necropolis Health =====

struct npc_necropolis_health : public ScriptedAI
{
    explicit npc_necropolis_health(Creature* creature) : ScriptedAI(creature)
    {
        me->setActive(true);
        me->SetFullHealth(); // RegenHealth is disabled
    }

    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spell) override
    {
        if (spell->Id == SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH)
            DoCastSelf(SPELL_ZAP_NECROPOLIS, true); // deals damage to self

        // Just to make sure it finally dies!
        if (spell->Id == SPELL_ZAP_NECROPOLIS)
            if (++_zapCount >= 3)
                me->KillSelf();
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Creature* necropolis = GetClosestCreatureWithEntry(me, NPC_NECROPOLIS, ATTACK_DISTANCE))
            me->CastSpell(necropolis, SPELL_DESPAWNER_OTHER, true);

        InvasionZoneDef const* def = FindInvasionZoneByZoneId(me->GetZoneId());
        if (!def)
            return;

        uint32 remaining = sScourgeInvasionMgr->GetSIRemaining(def->remainingIdx);
        if (remaining > 0)
            sScourgeInvasionMgr->SetSIRemaining(def->remainingIdx, remaining - 1);
    }

    void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
    {
        // Make sure necropolis despawns after SPELL_DESPAWNER_OTHER is triggered.
        if (spellInfo->Id == SPELL_DESPAWNER_OTHER && target->GetEntry() == NPC_NECROPOLIS)
        {
            DespawnNecropolisGO();
            if (Creature* creature = target->ToCreature())
                creature->DespawnOrUnsummon(0ms, Seconds(DAY));
            me->DespawnOrUnsummon(0ms, Seconds(DAY));
        }
    }

    void DespawnNecropolisGO()
    {
        uint32 const entries[] = { GO_NECROPOLIS_TINY, GO_NECROPOLIS_SMALL, GO_NECROPOLIS_MEDIUM, GO_NECROPOLIS_BIG, GO_NECROPOLIS_HUGE };
        for (uint32 entry : entries)
        {
            std::list<GameObject*> goList;
            me->GetGameObjectListWithEntryInGrid(goList, entry, ATTACK_DISTANCE);
            for (GameObject* go : goList)
                go->DespawnOrUnsummon(0ms, Seconds(DAY));
        }
    }

private:
    int _zapCount = 0; // 3 = death
};

// ===== NPC: Necropolis Proxy =====

struct npc_necropolis_proxy : public ScriptedAI
{
    npc_necropolis_proxy(Creature* creature) : ScriptedAI(creature)
    {
        me->setActive(true);
    }

    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spell) override
    {
        switch (spell->Id)
        {
            case SPELL_COMMUNIQUE_NECROPOLIS_TO_PROXIES:
                if (Creature* relay = GetClosestCreatureWithEntry(me, NPC_NECROPOLIS_RELAY, 200.0f))
                    me->CastSpell(relay, SPELL_COMMUNIQUE_PROXY_TO_RELAY, true);
                break;
            case SPELL_COMMUNIQUE_RELAY_TO_PROXY:
                if (Creature* necropolis = GetClosestCreatureWithEntry(me, NPC_NECROPOLIS, 200.0f))
                    me->CastSpell(necropolis, SPELL_COMMUNIQUE_PROXY_TO_NECROPOLIS, true);
                break;
            case SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH:
                if (Creature* health = GetClosestCreatureWithEntry(me, NPC_NECROPOLIS_HEALTH, 200.0f))
                    me->CastSpell(health, SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH, true);
                break;
            default:
                break;
        }
    }

    void SpellHitTarget(WorldObject* /*target*/, SpellInfo const* spell) override
    {
        // Despawn after forwarding the death communique to avoid being hit again.
        if (spell->Id == SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH)
            me->DespawnOrUnsummon(0ms, Seconds(DAY));
    }
};

// ===== NPC: Necropolis Relay =====

struct npc_necropolis_relay : public ScriptedAI
{
    npc_necropolis_relay(Creature* creature) : ScriptedAI(creature)
    {
        me->setActive(true);
    }

    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spell) override
    {
        switch (spell->Id)
        {
            case SPELL_COMMUNIQUE_PROXY_TO_RELAY:
                if (Creature* shard = GetClosestNecroticShard(me, 200.0f))
                    me->CastSpell(shard, SPELL_COMMUNIQUE_RELAY_TO_CAMP, true);
                break;
            case SPELL_COMMUNIQUE_CAMP_TO_RELAY:
                if (Creature* proxy = GetClosestCreatureWithEntry(me, NPC_NECROPOLIS_PROXY, 200.0f))
                    me->CastSpell(proxy, SPELL_COMMUNIQUE_RELAY_TO_PROXY, true);
                break;
            case SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH:
                if (Creature* proxy = GetClosestCreatureWithEntry(me, NPC_NECROPOLIS_PROXY, 200.0f))
                    me->CastSpell(proxy, SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH, true);
                break;
            default:
                break;
        }
    }

    void SpellHitTarget(WorldObject* /*target*/, SpellInfo const* spell) override
    {
        if (spell->Id == SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH)
            me->DespawnOrUnsummon(0ms, Seconds(DAY));
    }
};

// ===== NPC: Necrotic Shard / Damaged Necrotic Shard =====

struct npc_necrotic_shard : public ScriptedAI
{
    npc_necrotic_shard(Creature* creature) : ScriptedAI(creature)
    {
        me->setActive(true);
        me->SetReactState(REACT_PASSIVE);
        // No healing possible.
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_HEAL, true);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_HEAL_PCT, true);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_HEAL_MAX_HEALTH, true);
        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_PERIODIC_HEAL, true);
    }

    void Reset() override
    {
        _scheduler.CancelAll();
        ScheduleTasks();
    }

    // Activate camp type and minion spawning when finders are present but the
    // necropolis communique chain has not delivered CAMP_RECEIVES_COMMUNIQUE yet.
    void BootstrapCampActivation()
    {
        if (HasCampTypeAura())
            return;

        UpdateFindersAmount();
        if (_nearbyFinderCount == 0)
            return;

        DoCastSelf(SPELL_CHOOSE_CAMP_TYPE, true);
        ScheduleMinionSpawnTask();
    }

    void ScheduleTasks()
    {
        if (me->GetEntry() == NPC_NECROTIC_SHARD)
        {
            // Remove accidental shard duplicates on the same spot.
            for (uint32 entry : { uint32(NPC_NECROTIC_SHARD), uint32(NPC_DAMAGED_NECROTIC_SHARD) })
            {
                std::list<Creature*> shardList;
                me->GetCreatureListWithEntryInGrid(shardList, entry, CONTACT_DISTANCE);
                for (Creature* shard : shardList)
                    if (shard != me)
                        shard->DespawnOrUnsummon();
            }

            // Check if camp doodads are spawned shortly after spawn. If not: respawn them.
            _scheduler.Schedule(Seconds(10), [this](TaskContext /*context*/)
            {
                uint32 const doodads[] = { GO_UNDEAD_FIRE, GO_UNDEAD_FIRE_AURA, GO_SKULLPILE_01, GO_SKULLPILE_02, GO_SKULLPILE_03, GO_SKULLPILE_04 };
                for (uint32 entry : doodads)
                {
                    std::list<GameObject*> goList;
                    me->GetGameObjectListWithEntryInGrid(goList, entry, 50.0f);
                    for (GameObject* go : goList)
                        if (go && !go->isSpawned())
                        {
                            go->SetRespawnTime(0);
                            go->Respawn();
                        }
                }
            });

            // Fallback: communique chain can stall (missing timer aura, necropolis out of range, etc.).
            _scheduler.Schedule(Seconds(15), [this](TaskContext context)
            {
                BootstrapCampActivation();
                if (!HasCampTypeAura())
                    context.Repeat(Seconds(15));
            });
        }
        else if (me->GetEntry() == NPC_DAMAGED_NECROTIC_SHARD)
        {
            UpdateFindersAmount();
            ScheduleMinionSpawnTask();
            ScheduleCultistSpawnTask();
        }

        // If the summoning circle is gone (game event stopped), clean up and despawn.
        _scheduler.Schedule(Seconds(25), [this](TaskContext context)
        {
            if (!GetClosestGameObjectWithEntry(me, GO_SUMMON_CIRCLE, 2.0f))
            {
                DespawnEventDoodads();
                me->DespawnOrUnsummon();
                return;
            }
            context.Repeat(Seconds(60));
        });
    }

    void ScheduleMinionSpawnTask()
    {
        _scheduler.Schedule(Seconds(5), [this](TaskContext context)
        {
            HandleShardMinionSpawnerSmall();
            context.Repeat(Seconds(5));
        });
    }

    // Placeholder for SPELL_MINION_SPAWNER_BUTTRESS [27888]: respawn the Cultists every hour.
    void ScheduleCultistSpawnTask()
    {
        _scheduler.Schedule(Seconds(5), [this](TaskContext context)
        {
            DespawnShadowsOfDoom();
            SummonCultists();
            context.Repeat(Hours(1));
        });
    }

    bool HasCampTypeAura() const
    {
        return me->HasAura(SPELL_CAMP_TYPE_GHOST_SKELETON) || me->HasAura(SPELL_CAMP_TYPE_GHOST_GHOUL) || me->HasAura(SPELL_CAMP_TYPE_GHOUL_SKELETON);
    }

    void SpellHit(WorldObject* caster, SpellInfo const* spell) override
    {
        switch (spell->Id)
        {
            case SPELL_ZAP_CRYSTAL_CORPSE: // from a dying Shadow of Doom
            {
                Unit::DealDamage(me, me, me->GetMaxHealth() / 4, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
                if (++_zapCount >= 4)
                    me->KillSelf();
                break;
            }
            case SPELL_COMMUNIQUE_RELAY_TO_CAMP:
            {
                me->CastSpell(nullptr, SPELL_CAMP_RECEIVES_COMMUNIQUE, true);
                break;
            }
            case SPELL_CHOOSE_CAMP_TYPE:
            {
                _spellCampType = RAND(SPELL_CAMP_TYPE_GHOUL_SKELETON, SPELL_CAMP_TYPE_GHOST_GHOUL, SPELL_CAMP_TYPE_GHOST_SKELETON);
                DoCastSelf(_spellCampType, true);
                break;
            }
            case SPELL_CAMP_RECEIVES_COMMUNIQUE:
            {
                if (!HasCampTypeAura() && me->GetEntry() == NPC_NECROTIC_SHARD)
                    BootstrapCampActivation();
                break;
            }
            case SPELL_FIND_CAMP_TYPE:
            {
                // Don't spawn more minions than finders.
                if (_nearbyFinderCount < HasMinion(me, 60.0f))
                    return;

                Unit* unitCaster = caster ? caster->ToUnit() : nullptr;
                if (!unitCaster)
                    return;

                static constexpr std::pair<uint32, uint32> auraSpellMap[] =
                {
                    { SPELL_CAMP_TYPE_GHOST_SKELETON, SPELL_PH_SUMMON_MINION_TRAP_GHOST_SKELETON },
                    { SPELL_CAMP_TYPE_GHOST_GHOUL,    SPELL_PH_SUMMON_MINION_TRAP_GHOST_GHOUL    },
                    { SPELL_CAMP_TYPE_GHOUL_SKELETON, SPELL_PH_SUMMON_MINION_TRAP_GHOUL_SKELETON }
                };

                for (auto const& [aura, trapSpell] : auraSpellMap)
                    if (me->HasAura(aura))
                    {
                        unitCaster->CastSpell(unitCaster, trapSpell, true);
                        break;
                    }
                break;
            }
            default:
                break;
        }
    }

    void SpellHitTarget(WorldObject* /*target*/, SpellInfo const* spellInfo) override
    {
        if (me->GetEntry() != NPC_DAMAGED_NECROTIC_SHARD)
            return;

        // Death bolt delivered: remove the destroyed shard.
        if (spellInfo->Id == SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH)
            me->DespawnOrUnsummon();
    }

    // Only same-faction sources (minion deaths, Shadow of Doom zaps, self) may damage the shard.
    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/) override
    {
        if (attacker && attacker->GetFactionTemplateEntry() != me->GetFactionTemplateEntry())
            damage = 0;
    }

    void JustDied(Unit* /*killer*/) override
    {
        switch (me->GetEntry())
        {
            case NPC_NECROTIC_SHARD:
                // Shard destroyed: turns into a Damaged Necrotic Shard with the same camp type.
                if (Creature* shard = me->SummonCreature(NPC_DAMAGED_NECROTIC_SHARD, me->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN))
                {
                    shard->CastSpell(shard, _spellCampType ? _spellCampType : uint32(SPELL_CHOOSE_CAMP_TYPE), true);
                    me->DespawnOrUnsummon();
                }
                break;
            case NPC_DAMAGED_NECROTIC_SHARD:
                DoCastSelf(SPELL_SOUL_REVIVAL, true); // zone-wide player buff
                // Send the death bolt through the communication chain.
                if (Creature* relay = GetClosestCreatureWithEntry(me, NPC_NECROPOLIS_RELAY, 200.0f))
                    me->CastSpell(relay, SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH, true);
                DespawnCultists();
                DespawnEventDoodads();
                break;
            default:
                break;
        }
    }

    // Placeholder for SPELL_MINION_SPAWNER_SMALL [27887]: pick idle finders and let them
    // request a minion spawner matching the camp type.
    void HandleShardMinionSpawnerSmall()
    {
        uint32 spawnLimit = urand(1, 3);
        uint32 spawned = 0;

        std::list<Creature*> finderList;
        me->GetCreatureListWithEntryInGrid(finderList, NPC_SCOURGE_INVASION_MINION_FINDER, 60.0f);
        if (finderList.empty())
            return;

        // On a fresh camp minions spawn close to the shard first, then further out.
        finderList.sort(Trinity::ObjectDistanceOrderPred(me));

        for (Creature* finder : finderList)
        {
            if (spawned == spawnLimit)
                break;

            if (!finder->IsAlive())
                continue;

            finder->setActive(true);

            // Don't take finders that already have minions.
            if (HasMinion(finder))
                continue;

            // A finder despawns after summoning the spawner NPC and respawns 150-200s later.
            if (finder->CastSpell(me, SPELL_FIND_CAMP_TYPE, true) == SPELL_CAST_OK)
            {
                finder->DespawnOrUnsummon(0ms, Seconds(urand(150, 200)));
                ++spawned;
            }
        }
    }

    void SummonCultists()
    {
        std::list<GameObject*> shieldList;
        me->GetGameObjectListWithEntryInGrid(shieldList, GO_SUMMONER_SHIELD, INSPECT_DISTANCE);
        for (GameObject* shield : shieldList)
            shield->DespawnOrUnsummon();

        if (GameObject* circle = GetClosestGameObjectWithEntry(me, GO_SUMMON_CIRCLE, CONTACT_DISTANCE))
        {
            for (int i = 0; i < 4; ++i)
            {
                float angle = (float(i) * float(M_PI / 2)) + circle->GetOrientation();
                float x = circle->GetPositionX() + 6.95f * std::cos(angle);
                float y = circle->GetPositionY() + 6.75f * std::sin(angle);
                float z = circle->GetPositionZ() + 5.0f;
                me->UpdateGroundPositionZ(x, y, z);
                me->SummonCreature(NPC_CULTIST_ENGINEER, x, y, z, angle - float(M_PI), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, Hours(1));
            }
        }
    }

    static uint32 HasMinion(WorldObject* searcher, float searchDistance = ATTACK_DISTANCE)
    {
        uint32 minionCounter = 0;
        uint32 const entries[] = { NPC_SKELETAL_SHOCKTROOPER, NPC_GHOUL_BERSERKER, NPC_SPECTRAL_SOLDIER, NPC_LUMBERING_HORROR, NPC_BONE_WITCH, NPC_SPIRIT_OF_THE_DAMNED };
        for (uint32 entry : entries)
        {
            std::list<Creature*> minionList;
            searcher->GetCreatureListWithEntryInGrid(minionList, entry, searchDistance);
            for (Creature const* minion : minionList)
                if (minion && minion->IsAlive())
                    ++minionCounter;
        }
        return minionCounter;
    }

    void UpdateFindersAmount()
    {
        _nearbyFinderCount = 0;
        std::list<Creature*> finderList;
        me->GetCreatureListWithEntryInGrid(finderList, NPC_SCOURGE_INVASION_MINION_FINDER, 60.0f);
        for (Creature const* finder : finderList)
            if (finder)
                ++_nearbyFinderCount;
    }

    void DespawnCultists()
    {
        std::list<Creature*> cultistList;
        me->GetCreatureListWithEntryInGrid(cultistList, NPC_CULTIST_ENGINEER, INSPECT_DISTANCE);
        for (Creature* cultist : cultistList)
            if (cultist)
                cultist->DespawnOrUnsummon();
    }

    void DespawnShadowsOfDoom()
    {
        std::list<Creature*> shadowList;
        me->GetCreatureListWithEntryInGrid(shadowList, NPC_SHADOW_OF_DOOM, 200.0f);
        for (Creature* shadow : shadowList)
            if (shadow && shadow->IsAlive() && !shadow->IsInCombat())
                shadow->DespawnOrUnsummon();
    }

    // Remove camp objects around the shard (yes, this is blizzlike).
    void DespawnEventDoodads()
    {
        uint32 const doodads[] = { GO_SUMMON_CIRCLE, GO_UNDEAD_FIRE, GO_UNDEAD_FIRE_AURA, GO_SKULLPILE_01, GO_SKULLPILE_02, GO_SKULLPILE_03, GO_SKULLPILE_04, GO_SUMMONER_SHIELD };
        for (uint32 entry : doodads)
        {
            std::list<GameObject*> goList;
            me->GetGameObjectListWithEntryInGrid(goList, entry, 60.0f);
            for (GameObject* doodad : goList)
            {
                doodad->SetRespawnTime(-1);
                doodad->DespawnOrUnsummon();
            }
        }

        std::list<Creature*> finderList;
        me->GetCreatureListWithEntryInGrid(finderList, NPC_SCOURGE_INVASION_MINION_FINDER, 60.0f);
        for (Creature* finder : finderList)
            finder->DespawnOrUnsummon();
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
    uint32 _spellCampType = 0;
    uint32 _nearbyFinderCount = 0;
    uint8 _zapCount = 0; // 4 = death
};

// ===== NPC: Minion Spawner (16306/16336/16338) =====

struct npc_minion_spawner : public ScriptedAI
{
    npc_minion_spawner(Creature* creature) : ScriptedAI(creature)
    {
        me->setActive(true);
        me->SetReactState(REACT_PASSIVE);
    }

    void JustSummoned(Creature* summon) override
    {
        me->SetRespawnTime(0);
        me->SetCorpseDelay(0);
        ActivateScourgeMinion(summon);
        DoCastAOE(SPELL_MINION_SPAWN_IN);
    }

    void Reset() override
    {
        _scheduler.CancelAll();
        _scheduler.Schedule(Seconds(5), [this](TaskContext const& /*context*/)
        {
            uint32 entry;
            switch (me->GetEntry())
            {
                case NPC_SCOURGE_INVASION_MINION_SPAWNER_GHOST_GHOUL:
                    entry = CanSpawnRareMinion() ? RAND(NPC_SPIRIT_OF_THE_DAMNED, NPC_LUMBERING_HORROR)
                                                 : RAND(NPC_SPECTRAL_SOLDIER, NPC_GHOUL_BERSERKER);
                    break;
                case NPC_SCOURGE_INVASION_MINION_SPAWNER_GHOST_SKELETON:
                    entry = CanSpawnRareMinion() ? RAND(NPC_SPIRIT_OF_THE_DAMNED, NPC_BONE_WITCH)
                                                 : RAND(NPC_SPECTRAL_SOLDIER, NPC_SKELETAL_SHOCKTROOPER);
                    break;
                case NPC_SCOURGE_INVASION_MINION_SPAWNER_GHOUL_SKELETON:
                    entry = CanSpawnRareMinion() ? RAND(NPC_LUMBERING_HORROR, NPC_BONE_WITCH)
                                                 : RAND(NPC_GHOUL_BERSERKER, NPC_SKELETAL_SHOCKTROOPER);
                    break;
                default:
                    entry = NPC_GHOUL_BERSERKER;
                    break;
            }

            if (Creature* minion = me->SummonCreature(entry, me->GetPosition(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, Hours(1)))
            {
                ActivateScourgeMinion(minion);
                DoCastAOE(SPELL_MINION_SPAWN_IN);
            }
        });
    }

    bool CanSpawnRareMinion()
    {
        uint32 const rares[] = { NPC_LUMBERING_HORROR, NPC_BONE_WITCH, NPC_SPIRIT_OF_THE_DAMNED };
        for (uint32 entry : rares)
        {
            std::list<Creature*> rareList;
            me->GetCreatureListWithEntryInGrid(rareList, entry, 100.0f);
            if (!rareList.empty())
                return false; // already a rare nearby (dead or alive)
        }

        // Sniffed ratio: 19669 minions to 90 rares (~217:1).
        return urand(1, 217) == 1;
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// ===== NPC: Shadow of Doom (summoned by Cultist Engineer) =====

struct npc_shadow_of_doom : public ScriptedAI
{
    npc_shadow_of_doom(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
        DoCastSelf(SPELL_SPAWN_SMOKE, true);

        _scheduler.Schedule(Seconds(0), [this](TaskContext context)
        {
            Talk(0);
            context.Schedule(Seconds(8), [this](TaskContext /*ctx*/)
            {
                me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
            });
        });

        _events.ScheduleEvent(EVENT_DOOM_MINDFLAY, 2s);
        _events.ScheduleEvent(EVENT_DOOM_FEAR, 14s);
    }

    void Reset() override
    {
        _scheduler.CancelAll();
        _events.Reset();
    }

    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spell) override
    {
        if (spell->Id == SPELL_SPIRIT_SPAWN_OUT)
            me->DespawnOrUnsummon(Seconds(3));
    }

    void JustDied(Unit* /*killer*/) override
    {
        DoCastSelf(SPELL_ZAP_CRYSTAL_CORPSE, true);
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);

        if (!UpdateVictim())
            return;

        _events.Update(diff);
        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_DOOM_MINDFLAY:
                    DoCastVictim(SPELL_DOOM_MINDFLAY);
                    _events.Repeat(2s, 6500ms);
                    break;
                case EVENT_DOOM_FEAR:
                    DoCastVictim(SPELL_DOOM_FEAR);
                    _events.Repeat(14500ms);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    enum ShadowOfDoomSpells
    {
        SPELL_SPAWN_SMOKE     = 10389,
        SPELL_DOOM_MINDFLAY   = 16568,
        SPELL_DOOM_FEAR       = 12542,
    };

    TaskScheduler _scheduler;
    EventMap _events;
};

// ===== NPC: Cultist Engineer =====

struct npc_cultist_engineer : public ScriptedAI
{
    npc_cultist_engineer(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _scheduler.CancelAll();
        me->SetReactState(REACT_PASSIVE);
        me->SetCorpseDelay(10); // corpse despawns 10 seconds after a Shadow of Doom spawns

        _scheduler.Schedule(Milliseconds(100), [this](TaskContext /*context*/)
        {
            DoCastSelf(SPELL_CREATE_SUMMONER_SHIELD, true);
            DoCastSelf(SPELL_MINION_SPAWN_IN, true);
        });
        _scheduler.Schedule(Seconds(1), [this](TaskContext /*context*/)
        {
            DoCastSelf(SPELL_BUTTRESS_CHANNEL, true);
        });
    }

    bool OnGossipHello(Player* player) override
    {
        if (player->HasItemCount(ITEM_NECROTIC_RUNE, 8))
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "召唤末日阴影（消耗 8 个死灵符文）", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        else
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "我需要 8 个死灵符文才能召唤首领。", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
    {
        uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
        CloseGossipMenuFor(player);

        if (action == GOSSIP_ACTION_INFO_DEF + 1 && player->HasItemCount(ITEM_NECROTIC_RUNE, 8))
        {
            player->DestroyItemCount(ITEM_NECROTIC_RUNE, 8, true);
            player->CastSpell(nullptr, SPELL_SUMMON_BOSS, true); // summons a Shadow of Doom for 1 hour
            DoCastSelf(SPELL_QUIET_SUICIDE, true);
        }
        return true;
    }

    void JustDied(Unit* /*killer*/) override
    {
        _scheduler.CancelAll();
        if (Creature* shard = GetClosestCreatureWithEntry(me, NPC_DAMAGED_NECROTIC_SHARD, 15.0f))
            shard->CastSpell(shard, SPELL_DAMAGE_CRYSTAL, true);
        if (GameObject* shield = GetClosestGameObjectWithEntry(me, GO_SUMMONER_SHIELD, CONTACT_DISTANCE))
            shield->Delete();
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// ===== NPC: Flameshocker =====

struct npc_flameshocker : public ScriptedAI
{
    npc_flameshocker(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _scheduler.CancelAll();
        _scheduler.Schedule(Seconds(2), [this](TaskContext context)
        {
            if (Unit* victim = me->GetVictim())
                DoCast(victim, RAND(SPELL_FLAMESHOCKERS_TOUCH, SPELL_FLAMESHOCKERS_TOUCH2), true);
            context.Repeat(Seconds(30), Seconds(45));
        });
    }

    void JustDied(Unit* /*killer*/) override
    {
        DoCastSelf(SPELL_FLAMESHOCKERS_REVENGE, true);
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
        if (!UpdateVictim())
            return;
        DoMeleeAttackIfReady();
    }

private:
    TaskScheduler _scheduler;
};

// ===== NPC: Pallid Horror (city attacks) =====

struct npc_pallid_horror : public ScriptedAI
{
    npc_pallid_horror(Creature* creature) : ScriptedAI(creature), _summons(me) { }

    void InitializeAI() override
    {
        _summons.DespawnAll();
        me->SetCorpseDelay(10); // corpse despawns 10 seconds after the crystal spawns
        UpdateWeather(true);
        me->AddAura(SPELL_AURA_OF_FEAR, me);
        me->SetWalk(false);
        ScheduleTasks();
    }

    void ScheduleTasks()
    {
        _scheduler.Schedule(Seconds(0), [this](TaskContext /*context*/)
        {
            SummonFlameshockers();
        });
        _scheduler.Schedule(Seconds(1), [this](TaskContext context)
        {
            Talk(PALLID_HORROR_SAY_RANDOM_YELL);
            context.Repeat(Seconds(65), Seconds(300));
        });
        _scheduler.Schedule(Seconds(11), Seconds(81), [this](TaskContext context)
        {
            DoCastVictim(SPELL_DAMAGE_VS_GUARDS, true);
            context.Repeat(Seconds(11), Seconds(81));
        });
        _scheduler.Schedule(Seconds(2), [this](TaskContext context)
        {
            if (_summons.size() >= 30)
            {
                context.Repeat(Seconds(10));
                return;
            }

            // Spawn a Flameshocker next to a random nearby defender.
            std::list<Creature*> targets;
            FlameshockerSpawnTargetCheck check;
            Trinity::CreatureListSearcher<FlameshockerSpawnTargetCheck> searcher(me, targets, check);
            Cell::VisitGridObjects(me, searcher, 90.0f);

            if (!targets.empty())
            {
                Creature* target = Trinity::Containers::SelectRandomContainerElement(targets);
                float x, y, z;
                target->GetNearPoint(target, x, y, z, 5.0f, 0.0f);
                if (Creature* summon = me->SummonCreature(NPC_FLAMESHOCKER, x, y, z, target->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, Seconds(5)))
                    _summons.Summon(summon);
            }
            context.Repeat(Seconds(2));
        });
    }

    void SummonFlameshockers()
    {
        uint32 const amount = urand(5, 9); // sniffed group sizes of 5-9 shockers on spawn
        for (uint32 i = 0; i < amount; ++i)
        {
            if (Creature* summon = me->SummonCreature(NPC_FLAMESHOCKER, me->GetPosition(), TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, Hours(1)))
            {
                float angle = float(i) * (float(M_PI) / (float(amount) / 2.f)) + me->GetOrientation();
                summon->GetMotionMaster()->Clear();
                summon->GetMotionMaster()->MoveFollow(me, 2.5f, angle);
                _summons.Summon(summon);
            }
        }
    }

    void JustSummoned(Creature* summon) override
    {
        summon->CastSpell(summon, SPELL_MINION_SPAWN_IN, true);
        summon->SetWalk(false);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Creature* sylvanas = GetClosestCreatureWithEntry(me, NPC_LADY_SYLVANAS_WINDRUNNER, VISIBILITY_DISTANCE_NORMAL))
            sylvanas->AI()->Talk(SYLVANAS_SAY_ATTACK_END);

        // Kill remaining flameshockers.
        for (ObjectGuid guid : _summons)
            if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                summon->KillSelf();

        // Spawn the quest crystal.
        DoCastSelf(me->GetZoneId() == AREA_UNDERCITY ? SPELL_SUMMON_FAINT_NECROTIC_CRYSTAL : SPELL_SUMMON_CRACKED_NECROTIC_CRYSTAL, true);

        sScourgeInvasionMgr->OnPallidDeath(me->GetZoneId());
        UpdateWeather(false);
    }

    void CorpseRemoved(uint32& /*respawnDelay*/) override
    {
        _summons.DespawnAll();
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
        if (!UpdateVictim())
            return;
        DoMeleeAttackIfReady();
    }

    void UpdateWeather(bool start)
    {
        if (Weather* weather = me->GetMap()->GetOrGenerateZoneDefaultWeather(me->GetZoneId()))
            weather->SetWeather(start ? WEATHER_TYPE_STORM : WEATHER_TYPE_RAIN, start ? 0.25f : 0.0f);
    }

private:
    struct FlameshockerSpawnTargetCheck
    {
        bool operator()(Creature* creature) const
        {
            return creature->IsAlive() && !creature->IsCivilian() && creature->GetEntry() != NPC_FLAMESHOCKER;
        }
    };

    TaskScheduler _scheduler;
    SummonList _summons;
};

// ===== Spell Scripts =====

// 28091 - Despawner, self (server-side)
class spell_despawner_self : public SpellScript
{
    PrepareSpellScript(spell_despawner_self);

    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo({ SPELL_SPIRIT_SPAWN_OUT });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster || !caster->IsCreature())
            return;

        // Acore despawns infrastructure via Spirit Spawn-out; invasion minions must persist.
        if (IsScourgeInvasionMinionEntry(caster->ToCreature()->GetEntry()))
            return;

        if (!caster->IsInCombat())
            caster->CastSpell(caster, SPELL_SPIRIT_SPAWN_OUT, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_despawner_self::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 28345 - Communique Trigger (server-side)
class spell_communique_trigger : public SpellScript
{
    PrepareSpellScript(spell_communique_trigger);

    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo({ SPELL_COMMUNIQUE_CAMP_TO_RELAY });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* target = GetHitUnit();
        if (!target)
            return;

        if (Creature* relay = GetClosestCreatureWithEntry(target, NPC_NECROPOLIS_RELAY, 200.0f))
            target->CastSpell(relay, SPELL_COMMUNIQUE_CAMP_TO_RELAY, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_communique_trigger::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// Filters communique/lightning spells so only SI chain participants can be hit.
class spell_scourge_invasion_communique_filter : public SpellScript
{
    PrepareSpellScript(spell_scourge_invasion_communique_filter);

    void PreventInvalidHit(SpellEffIndex effIndex)
    {
        if (WorldObject* target = GetHitUnit() ? static_cast<WorldObject*>(GetHitUnit()) : GetHitGObj())
            if (!IsValidScourgeInvasionBoltTarget(target, GetSpellInfo()->Id))
                PreventHitDefaultEffect(effIndex);
    }

    void Register() override
    {
        // Communique bolts are area/chain visuals that must never damage players, pets, or bots.
        OnEffectHitTarget += SpellEffectFn(spell_scourge_invasion_communique_filter::PreventInvalidHit, EFFECT_ALL, SPELL_EFFECT_ANY);
    }
};

// 28265 - Scourge Strike (pink lightning instakill; SmartAI casts on current victim)
class spell_scourge_invasion_scourge_strike : public SpellScript
{
    PrepareSpellScript(spell_scourge_invasion_scourge_strike);

    SpellCastResult CheckCast()
    {
        if (IsProtectedFromScourgeStrike(GetExplTargetUnit()))
            return SPELL_FAILED_BAD_TARGETS;
        return SPELL_CAST_OK;
    }

    void PreventProtectedHit(SpellEffIndex effIndex)
    {
        if (IsProtectedFromScourgeStrike(GetHitUnit()))
            PreventHitDefaultEffect(effIndex);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_scourge_invasion_scourge_strike::CheckCast);
        // Creature SmartAI instant casts may bypass CheckCast; block damage at hit time.
        OnEffectHitTarget += SpellEffectFn(spell_scourge_invasion_scourge_strike::PreventProtectedHit, EFFECT_ALL, SPELL_EFFECT_ANY);
    }
};

// ===== WorldScript: drives the manager =====

struct ScourgeInvasionWorldScript : public WorldScript
{
    ScourgeInvasionWorldScript() : WorldScript("ScourgeInvasionWorldScript") { }

    void OnStartup() override
    {
        sScourgeInvasionMgr->LoadFromDB();
    }

    void OnUpdate(uint32 diff) override
    {
        if (!_initialized)
        {
            _initTimer += diff;
            // Wait 30 seconds to ensure all maps and objects are fully initialized.
            if (_initTimer < 30000)
                return;
            _initialized = true;

            // Auto-create the world channel.
            if (ChannelMgr* channelMgr = ChannelMgr::ForTeam(ALLIANCE))
                channelMgr->CreateCustomChannel("世界");
            if (ChannelMgr* channelMgr = ChannelMgr::ForTeam(HORDE))
                channelMgr->CreateCustomChannel("世界");

            // Auto-join all currently online players to the world channel.
            SessionMap const& sessions = sWorld->GetAllSessions();
            for (auto const& sessionPair : sessions)
            {
                Player* player = sessionPair.second->GetPlayer();
                if (!player || !player->IsInWorld())
                    continue;
                if (ChannelMgr* mgr = ChannelMgr::ForTeam(player->GetTeam()))
                    mgr->GetChannel(0, "世界", player);
            }

            if (sScourgeInvasionMgr->GetState() == SI_STATE_ENABLED)
                sScourgeInvasionMgr->StartScourgeInvasion();
            return;
        }

        sScourgeInvasionMgr->Update(diff);
    }

private:
    uint32 _initTimer = 0;
    bool _initialized = false;
};

// ===== UnitScript: tag camp minions for loot when damaged by players/bots =====

class ScourgeInvasionUnitScript : public UnitScript
{
public:
    ScourgeInvasionUnitScript() : UnitScript("ScourgeInvasionUnitScript") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& /*damage*/) override
    {
        Creature* creature = victim ? victim->ToCreature() : nullptr;
        if (!creature || !attacker || !IsScourgeInvasionMinionEntry(creature->GetEntry()))
            return;

        if (Player* player = attacker->GetCharmerOrOwnerPlayerOrPlayerItself())
        {
            if (!creature->hasLootRecipient())
                creature->SetLootRecipient(player, true);
            creature->LowerPlayerDamageReq(creature->GetMaxHealth());
        }
    }
};

// ===== PlayerScript: auto-join world channel on login =====

class ScourgeInvasionPlayerScript : public PlayerScript
{
public:
    ScourgeInvasionPlayerScript() : PlayerScript("ScourgeInvasionPlayerScript") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        if (!player)
            return;
        if (ChannelMgr* mgr = ChannelMgr::ForTeam(player->GetTeam()))
            mgr->GetChannel(0, "世界", player);
    }

    void OnCreatureKill(Player* killer, Creature* killed) override
    {
        if (!killer || !killed || !IsScourgeInvasionMinionEntry(killed->GetEntry()))
            return;

        EnsureScourgeInvasionMinionLoot(killed, killer);
    }
};

// ===== Command: .si =====

class scourge_invasion_commandscript : public CommandScript
{
public:
    scourge_invasion_commandscript() : CommandScript("scourge_invasion_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable siCommandTable =
        {
            { "",        HandleSIStatusCommand,  rbac::RBAC_PERM_COMMAND_EVENT_INFO,  Console::Yes },
            { "enable",  HandleSIEnableCommand,  rbac::RBAC_PERM_COMMAND_EVENT_START, Console::Yes },
            { "disable", HandleSIDisableCommand, rbac::RBAC_PERM_COMMAND_EVENT_START, Console::Yes },
        };
        static ChatCommandTable commandTable =
        {
            { "si", siCommandTable },
        };
        return commandTable;
    }

    static std::string FormatRemaining(uint32 secs)
    {
        if (secs == 0)
            return "即将开始";
        return fmt::format("{}分{}秒", secs / 60, secs % 60);
    }

    static uint32 SecondsUntil(TimePoint tp)
    {
        if (tp == TimePoint())
            return 0;
        int64 secs = std::chrono::duration_cast<std::chrono::seconds>(tp - std::chrono::steady_clock::now()).count();
        return secs > 0 ? uint32(secs) : 0;
    }

    static bool HandleSIStatusCommand(ChatHandler* handler)
    {
        handler->SendSysMessage("===== 天灾入侵状态 =====");

        SIState state = sScourgeInvasionMgr->GetState();
        handler->PSendSysMessage("系统状态: %s", state == SI_STATE_ENABLED ? "|cff00ff00已启用|r" : "|cffff0000已禁用|r");
        handler->PSendSysMessage("已击败入侵: %u 次", sScourgeInvasionMgr->GetBattlesWon());

        bool const globalEventActive = sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION);
        handler->PSendSysMessage("全球事件#17(主城外围巡逻): %s",
            globalEventActive ? "|cffffcc00活动中|r — 暴风城/幽暗城门外会刷天灾步兵" : "|cff888888未激活|r");

        handler->SendSysMessage("--- 区域入侵 (浮空城+营地，看这里！) ---");
        uint32 activeZoneCount = 0;
        for (InvasionZoneDef const& def : g_invasionZoneDefs)
        {
            std::string_view name;
            switch (def.zoneId)
            {
                case AREA_WINTERSPRING:        name = "冬泉谷"; break;
                case AREA_TANARIS:             name = "塔纳利斯"; break;
                case AREA_AZSHARA:             name = "艾萨拉"; break;
                case AREA_BLASTED_LANDS:       name = "诅咒之地"; break;
                case AREA_EASTERN_PLAGUELANDS: name = "东瘟疫之地"; break;
                case AREA_BURNING_STEPPES:     name = "燃烧平原"; break;
                default:                       name = "未知"; break;
            }

            uint32 remaining = sScourgeInvasionMgr->GetSIRemaining(def.remainingIdx);
            bool const heraldAlive = !sScourgeInvasionMgr->GetMouthGuid(def.zoneId).IsEmpty();
            bool const zoneEventActive = sGameEventMgr->IsActiveEvent(def.gameEventId);

            if (remaining > 0)
            {
                ++activeZoneCount;
                handler->PSendSysMessage("%s: |cffff0000战斗中|r — 剩余浮空城 %u, 先驱%s, 区域事件#%u %s",
                    std::string(name).c_str(), remaining,
                    heraldAlive ? "存活" : "缺失(将自动恢复)",
                    def.gameEventId, zoneEventActive ? "运行中" : "|cffff6600已停止(将自动恢复)|r");

                if (def.zoneId == AREA_BURNING_STEPPES)
                    handler->SendSysMessage("  |cffaaaaaa提示: 燃烧平原浮空城在东部王国中部上空，暴风城外也能看到|r");
            }
            else
                handler->PSendSysMessage("%s: |cff00ff00待命中|r (下次 %s)", std::string(name).c_str(),
                    FormatRemaining(SecondsUntil(sScourgeInvasionMgr->GetSITimer(def.timerIdx))).c_str());
        }

        if (activeZoneCount == 0 && globalEventActive)
            handler->SendSysMessage("|cffffcc00注意: 无区域浮空城战斗，但事件#17仍会在主城门外刷巡逻步兵|r");

        handler->SendSysMessage("--- 主城袭击 (城内苍白恐魔，与城外浮空城无关) ---");
        for (CityAttackDef const& def : g_cityAttackDefs)
        {
            std::string_view name = def.zoneId == AREA_UNDERCITY ? "幽暗城" : "暴风城";
            if (!sScourgeInvasionMgr->GetPallidGuid(def.zoneId).IsEmpty())
                handler->PSendSysMessage("%s城内袭击: |cffff0000苍白恐魔行进中|r", std::string(name).c_str());
            else
                handler->PSendSysMessage("%s城内袭击: |cff00ff00无苍白恐魔|r (下次 %s)", std::string(name).c_str(),
                    FormatRemaining(SecondsUntil(sScourgeInvasionMgr->GetSITimer(def.timerIdx))).c_str());
        }

        return true;
    }

    static bool HandleSIEnableCommand(ChatHandler* handler)
    {
        sScourgeInvasionMgr->SetState(SI_STATE_ENABLED);
        handler->SendSysMessage("天灾入侵已启用。");
        return true;
    }

    static bool HandleSIDisableCommand(ChatHandler* handler)
    {
        sScourgeInvasionMgr->SetState(SI_STATE_DISABLED);
        handler->SendSysMessage("天灾入侵已禁用。");
        return true;
    }
};

void AddSC_scourge_invasion()
{
    new ScourgeInvasionWorldScript();
    new ScourgeInvasionUnitScript();
    new ScourgeInvasionPlayerScript();
    new scourge_invasion_commandscript();
    RegisterGameObjectAI(go_necropolis);
    RegisterCreatureAI(npc_herald_of_the_lich_king);
    RegisterCreatureAI(npc_necropolis);
    RegisterCreatureAI(npc_necropolis_health);
    RegisterCreatureAI(npc_necropolis_proxy);
    RegisterCreatureAI(npc_necropolis_relay);
    RegisterCreatureAI(npc_necrotic_shard);
    RegisterCreatureAI(npc_minion_spawner);
    RegisterCreatureAI(npc_shadow_of_doom);
    RegisterCreatureAI(npc_cultist_engineer);
    RegisterCreatureAI(npc_flameshocker);
    RegisterCreatureAI(npc_pallid_horror);
    RegisterSpellScript(spell_communique_trigger);
    RegisterSpellScript(spell_despawner_self);
    RegisterSpellScript(spell_scourge_invasion_communique_filter);
    RegisterSpellScript(spell_scourge_invasion_scourge_strike);
}
