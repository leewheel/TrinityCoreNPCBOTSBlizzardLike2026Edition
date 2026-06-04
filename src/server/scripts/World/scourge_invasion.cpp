/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
#include "Containers.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "CreatureTextMgr.h"
#include "DatabaseEnv.h"
#include "GameEventMgr.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
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
#include "Weather.h"
#include "WeatherMgr.h"
#include "World.h"
#include "WorldSession.h"
#include <mutex>
#include <set>
#include <map>

using TimePoint = std::chrono::steady_clock::time_point;

// ===== Scourge Invasion Manager =====

struct ScourgeInvasionData
{
    SIState state = SI_STATE_DISABLED;
    TimePoint timers[SI_TIMER_MAX];
    uint32 battlesWon = 0;
    uint32 lastAttackZone = 0;
    uint32 remaining[SI_REMAINING_MAX] = {};
    std::set<uint32> pendingInvasions;
    std::set<uint32> pendingPallids;
    std::map<uint32, ObjectGuid> mouthGuids;
    std::map<uint32, ObjectGuid> pallidGuids;
};

struct InvasionZoneDef
{
    uint32 map;
    uint32 zoneId;
    uint32 necropolisCount;
    SIRemaining remainingIdx;
    SITimers timerIdx;
    float mouthX, mouthY, mouthZ;
};

static InvasionZoneDef const g_invasionZoneDefs[] =
{
    { 0, AREA_WINTERSPRING,       3, SI_REMAINING_WINTERSPRING,       SI_TIMER_WINTERSPRING,       7736.56f,  -4033.75f, 696.327f },
    { 1, AREA_TANARIS,            3, SI_REMAINING_TANARIS,            SI_TIMER_TANARIS,           -8352.68f, -3972.68f,  10.0753f },
    { 1, AREA_AZSHARA,            2, SI_REMAINING_AZSHARA,            SI_TIMER_AZSHARA,            3273.75f,  -4276.98f, 125.509f },
    { 0, AREA_BLASTED_LANDS,      2, SI_REMAINING_BLASTED_LANDS,      SI_TIMER_BLASTED_LANDS,    -11429.3f,  -3327.82f,   7.73628f },
    { 0, AREA_EASTERN_PLAGUELANDS, 2, SI_REMAINING_EASTERN_PLAGUELANDS, SI_TIMER_EASTERN_PLAGUELANDS, 2014.55f, -4934.52f, 73.9846f },
    { 0, AREA_BURNING_STEPPES,    2, SI_REMAINING_BURNING_STEPPES,    SI_TIMER_BURNING_STEPPES,    -8229.53f, -1118.11f, 144.012f },
};

struct PallidAttackDef
{
    uint32 map;
    uint32 zoneId;
    SITimers timerIdx;
    float pallidX[2], pallidY[2], pallidZ[2];
};

static PallidAttackDef const g_pallidDefs[] =
{
    { 0, AREA_UNDERCITY, SI_TIMER_UNDERCITY, { 1914.89f, 1834.36f }, { 240.815f, 217.958f }, { 54.3627f, 58.1834f } },
    { 0, AREA_STORMWIND, SI_TIMER_STORMWIND, { -8810.69f, -8449.83f }, { 624.104f, 340.891f }, { 101.348f, 113.409f } },
};

static InvasionZoneDef const* FindInvasionZoneByZoneId(uint32 zoneId)
{
    for (auto const& def : g_invasionZoneDefs)
        if (def.zoneId == zoneId)
            return &def;
    return nullptr;
}

class ScourgeInvasionMgr
{
public:
    static ScourgeInvasionMgr* instance()
    {
        static ScourgeInvasionMgr instance;
        return &instance;
    }

    void LoadFromDB()
    {
        QueryResult result = WorldDatabase.Query("SELECT zoneId, attackTimer, remainingNecropoli, battlesWon, lastAttackZone, state FROM scourge_invasion_state");
        if (!result)
            return;

        do
        {
            Field* fields = result->Fetch();
            uint32 zoneId = fields[0].GetUInt32();
            uint32 attackTimer = fields[1].GetUInt32();
            uint32 remaining = fields[2].GetUInt32();
            _data.battlesWon = fields[3].GetUInt32();
            _data.lastAttackZone = fields[4].GetUInt32();
            _data.state = SIState(fields[5].GetUInt32());

            if (InvasionZoneDef const* def = FindInvasionZoneByZoneId(zoneId))
            {
                _data.remaining[def->remainingIdx] = remaining;
                if (attackTimer)
                    _data.timers[def->timerIdx] = std::chrono::steady_clock::now() + std::chrono::seconds(attackTimer);
            }
            else if (zoneId == AREA_UNDERCITY || zoneId == AREA_STORMWIND)
            {
                if (zoneId == AREA_UNDERCITY && attackTimer)
                    _data.timers[SI_TIMER_UNDERCITY] = std::chrono::steady_clock::now() + std::chrono::seconds(attackTimer);
                if (zoneId == AREA_STORMWIND && attackTimer)
                    _data.timers[SI_TIMER_STORMWIND] = std::chrono::steady_clock::now() + std::chrono::seconds(attackTimer);
            }
        } while (result->NextRow());

        if (_data.state == SI_STATE_ENABLED)
            StartEvents();
    }

    void SaveToDB()
    {
        WorldDatabase.Execute("DELETE FROM scourge_invasion_state");
        for (auto const& def : g_invasionZoneDefs)
        {
            uint32 timerVal = 0;
            auto it = _data.timers[def.timerIdx];
            if (it != TimePoint())
            {
                auto secs = std::chrono::duration_cast<std::chrono::seconds>(it - std::chrono::steady_clock::now()).count();
                timerVal = std::max<uint32>(0, secs);
            }
            WorldDatabase.Execute(fmt::format("INSERT INTO scourge_invasion_state (zoneId, attackTimer, remainingNecropoli, battlesWon, lastAttackZone, state) VALUES ({}, {}, {}, {}, {}, {})",
                def.zoneId, timerVal, _data.remaining[def.remainingIdx], _data.battlesWon, _data.lastAttackZone, uint32(_data.state)).c_str());
        }
        // Save city timers
        for (auto const& def : g_pallidDefs)
        {
            uint32 timerVal = 0;
            auto it = _data.timers[def.timerIdx];
            if (it != TimePoint())
            {
                auto secs = std::chrono::duration_cast<std::chrono::seconds>(it - std::chrono::steady_clock::now()).count();
                timerVal = std::max<uint32>(0, secs);
            }
            WorldDatabase.Execute(fmt::format("INSERT INTO scourge_invasion_state (zoneId, attackTimer, remainingNecropoli, battlesWon, lastAttackZone, state) VALUES ({}, {}, 0, {}, {}, {})",
                def.zoneId, timerVal, _data.battlesWon, _data.lastAttackZone, uint32(_data.state)).c_str());
        }
    }

    void SetState(SIState state)
    {
        _data.state = state;
        if (state == SI_STATE_ENABLED)
            StartEvents();
        else
            StopEvents();
        SaveToDB();
    }

    SIState GetState() const { return _data.state; }

    uint32 GetSIRemaining(SIRemaining idx) const { return _data.remaining[idx]; }
    void SetSIRemaining(SIRemaining idx, uint32 val)
    {
        _data.remaining[idx] = val;
        SaveToDB();
    }

    uint32 GetSIRemainingByZone(uint32 zoneId) const
    {
        for (uint32 i = 0; i < SI_REMAINING_MAX; ++i)
            if (g_invasionZoneDefs[i].zoneId == zoneId)
                return _data.remaining[i];
        return 0;
    }

    TimePoint GetSITimer(SITimers idx) const { return _data.timers[idx]; }
    void SetSITimer(SITimers idx, TimePoint tp) { _data.timers[idx] = tp; }

    uint32 GetBattlesWon() const { return _data.battlesWon; }
    void AddBattlesWon(int32 count)
    {
        _data.battlesWon += count;
        HandleDefendedZones();
        SaveToDB();
    }

    uint32 GetLastAttackZone() const { return _data.lastAttackZone; }
    void SetLastAttackZone(uint32 z) { _data.lastAttackZone = z; }

    void SetMouthGuid(uint32 zoneId, ObjectGuid guid) { _data.mouthGuids[zoneId] = guid; }
    void SetPallidGuid(uint32 zoneId, ObjectGuid guid) { _data.pallidGuids[zoneId] = guid; }
    ObjectGuid GetMouthGuid(uint32 zoneId) const
    {
        auto it = _data.mouthGuids.find(zoneId);
        return it != _data.mouthGuids.end() ? it->second : ObjectGuid::Empty;
    }
    ObjectGuid GetPallidGuid(uint32 zoneId) const
    {
        auto it = _data.pallidGuids.find(zoneId);
        return it != _data.pallidGuids.end() ? it->second : ObjectGuid::Empty;
    }

    bool HasPendingInvasion(uint32 zoneId) const { return _data.pendingInvasions.count(zoneId) > 0; }
    bool HasPendingPallid(uint32 zoneId) const { return _data.pendingPallids.count(zoneId) > 0; }
    void AddPendingInvasion(uint32 z) { _data.pendingInvasions.insert(z); }
    void RemovePendingInvasion(uint32 z) { _data.pendingInvasions.erase(z); }
    void AddPendingPallid(uint32 z) { _data.pendingPallids.insert(z); }
    void RemovePendingPallid(uint32 z) { _data.pendingPallids.erase(z); }

    void BroadcastWorldStates()
    {
        WorldPacket data(SMSG_UPDATE_WORLD_STATE, 4 + 4);
        auto sendWS = [&](uint32 variable, uint32 value)
        {
            data << uint32(variable);
            data << uint32(value);
        };

        sendWS(WORLD_STATE_SCOURGE_INVASION_VICTORIES, _data.battlesWon);
        for (auto const& def : g_invasionZoneDefs)
        {
            bool active = _data.remaining[def.remainingIdx] > 0;
            uint32 wsId = 0;
            switch (def.zoneId)
            {
                case AREA_WINTERSPRING:        wsId = WORLD_STATE_SCOURGE_INVASION_WINTERSPRING; break;
                case AREA_TANARIS:             wsId = WORLD_STATE_SCOURGE_INVASION_TANARIS; break;
                case AREA_AZSHARA:             wsId = WORLD_STATE_SCOURGE_INVASION_AZSHARA; break;
                case AREA_BLASTED_LANDS:       wsId = WORLD_STATE_SCOURGE_INVASION_BLASTED_LANDS; break;
                case AREA_EASTERN_PLAGUELANDS: wsId = WORLD_STATE_SCOURGE_INVASION_EASTERN_PLAGUELANDS; break;
                case AREA_BURNING_STEPPES:     wsId = WORLD_STATE_SCOURGE_INVASION_BURNING_STEPPES; break;
            }
            if (wsId)
                sendWS(wsId, active ? 1 : 0);

            uint32 wsNecroId = 0;
            switch (def.zoneId)
            {
                case AREA_WINTERSPRING:        wsNecroId = WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_WINTERSPRING; break;
                case AREA_TANARIS:             wsNecroId = WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_TANARIS; break;
                case AREA_AZSHARA:             wsNecroId = WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_AZSHARA; break;
                case AREA_BLASTED_LANDS:       wsNecroId = WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_BLASTED_LANDS; break;
                case AREA_EASTERN_PLAGUELANDS: wsNecroId = WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_EASTERN_PLAGUELANDS; break;
                case AREA_BURNING_STEPPES:     wsNecroId = WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_BURNING_STEPPES; break;
            }
            if (wsNecroId)
                sendWS(wsNecroId, _data.remaining[def.remainingIdx]);
        }
        sWorld->SendGlobalMessage(&data);
    }

    void StartNewInvasionIfTime(uint32 zoneId)
    {
        InvasionZoneDef const* def = FindInvasionZoneByZoneId(zoneId);
        if (!def)
            return;

        TimePoint now = std::chrono::steady_clock::now();
        if (_data.timers[def->timerIdx] != TimePoint() && now < _data.timers[def->timerIdx])
            return;

        StartNewInvasion(zoneId);
    }

    void StartNewInvasion(uint32 zoneId)
    {
        InvasionZoneDef const* def = FindInvasionZoneByZoneId(zoneId);
        if (!def)
            return;

        // Reset remaining
        _data.remaining[def->remainingIdx] = def->necropolisCount;
        _data.lastAttackZone = zoneId;

        // Summon Mouth
        Map* map = sMapMgr->FindMap(def->map, 0);
        if (!map)
        {
            AddPendingInvasion(zoneId);
            return;
        }

        // Despawn old mouth
        ObjectGuid oldMouth = GetMouthGuid(zoneId);
        if (Creature* old = map->GetCreature(oldMouth))
            old->DespawnOrUnsummon();

        // Summon new Herald
        Creature* mouth = map->SummonCreature(NPC_HERALD_OF_THE_LICH_KING,
            Position(def->mouthX, def->mouthY, def->mouthZ, 0.0f));
        if (mouth)
        {
            SetMouthGuid(zoneId, mouth->GetGUID());
            mouth->AI()->DoAction(EVENT_HERALD_OF_THE_LICH_KING_ZONE_START);
        }

        // Start game event
        uint32 gameEventId = 0;
        switch (zoneId)
        {
            case AREA_WINTERSPRING:        gameEventId = GAME_EVENT_SCOURGE_INVASION_WINTERSPRING; break;
            case AREA_TANARIS:             gameEventId = GAME_EVENT_SCOURGE_INVASION_TANARIS; break;
            case AREA_AZSHARA:             gameEventId = GAME_EVENT_SCOURGE_INVASION_AZSHARA; break;
            case AREA_BLASTED_LANDS:       gameEventId = GAME_EVENT_SCOURGE_INVASION_BLASTED_LANDS; break;
            case AREA_EASTERN_PLAGUELANDS: gameEventId = GAME_EVENT_SCOURGE_INVASION_EASTERN_PLAGUELANDS; break;
            case AREA_BURNING_STEPPES:     gameEventId = GAME_EVENT_SCOURGE_INVASION_BURNING_STEPPES; break;
        }
        if (gameEventId && !sGameEventMgr->IsActiveEvent(gameEventId))
            sGameEventMgr->StartEvent(gameEventId, true);

        BroadcastWorldStates();
        SaveToDB();
    }

    void StartNewCityAttackIfTime(uint32 zoneId)
    {
        PallidAttackDef const* def = nullptr;
        for (auto const& d : g_pallidDefs)
            if (d.zoneId == zoneId)
                def = &d;

        if (!def)
            return;

        TimePoint now = std::chrono::steady_clock::now();
        if (_data.timers[def->timerIdx] != TimePoint() && now < _data.timers[def->timerIdx])
            return;

        StartNewCityAttack(zoneId);
    }

    void StartNewCityAttack(uint32 zoneId)
    {
        PallidAttackDef const* def = nullptr;
        for (auto const& d : g_pallidDefs)
            if (d.zoneId == zoneId)
                def = &d;

        if (!def)
            return;

        Map* map = sMapMgr->FindMap(def->map, 0);
        if (!map)
        {
            AddPendingPallid(zoneId);
            return;
        }

        // Despawn old pallid
        ObjectGuid oldPallid = GetPallidGuid(zoneId);
        if (Creature* old = map->GetCreature(oldPallid))
            old->DespawnOrUnsummon();

        uint32 spawnIdx = urand(0, 1);
        Creature* pallid = map->SummonCreature(NPC_PALLID_HORROR,
            Position(def->pallidX[spawnIdx], def->pallidY[spawnIdx], def->pallidZ[spawnIdx], 0.0f));
        if (pallid)
            SetPallidGuid(zoneId, pallid->GetGUID());

        SaveToDB();
    }

    void HandleZoneNecropolisDestroyed(uint32 zoneId)
    {
        InvasionZoneDef const* def = FindInvasionZoneByZoneId(zoneId);
        if (!def)
            return;

        if (_data.remaining[def->remainingIdx] > 0)
            _data.remaining[def->remainingIdx]--;

        if (_data.remaining[def->remainingIdx] == 0)
        {
            // All necropoli destroyed, stop the event
            if (Creature* mouth = GetMouthCreature(zoneId))
                mouth->AI()->DoAction(EVENT_HERALD_OF_THE_LICH_KING_ZONE_STOP);

            _data.timers[def->timerIdx] = std::chrono::steady_clock::now() + std::chrono::seconds(urand(ZONE_ATTACK_TIMER_MIN, ZONE_ATTACK_TIMER_MAX));
            AddBattlesWon(1);

            // Stop game event
            uint32 gameEventId = 0;
            switch (zoneId)
            {
                case AREA_WINTERSPRING:        gameEventId = GAME_EVENT_SCOURGE_INVASION_WINTERSPRING; break;
                case AREA_TANARIS:             gameEventId = GAME_EVENT_SCOURGE_INVASION_TANARIS; break;
                case AREA_AZSHARA:             gameEventId = GAME_EVENT_SCOURGE_INVASION_AZSHARA; break;
                case AREA_BLASTED_LANDS:       gameEventId = GAME_EVENT_SCOURGE_INVASION_BLASTED_LANDS; break;
                case AREA_EASTERN_PLAGUELANDS: gameEventId = GAME_EVENT_SCOURGE_INVASION_EASTERN_PLAGUELANDS; break;
                case AREA_BURNING_STEPPES:     gameEventId = GAME_EVENT_SCOURGE_INVASION_BURNING_STEPPES; break;
            }
            if (gameEventId && sGameEventMgr->IsActiveEvent(gameEventId))
                sGameEventMgr->StopEvent(gameEventId, true);

            BroadcastWorldStates();
            SaveToDB();
        }
    }

    Creature* GetMouthCreature(uint32 zoneId)
    {
        ObjectGuid guid = GetMouthGuid(zoneId);
        if (guid.IsEmpty())
            return nullptr;
        InvasionZoneDef const* def = FindInvasionZoneByZoneId(zoneId);
        if (!def)
            return nullptr;
        Map* map = sMapMgr->FindMap(def->map, 0);
        if (!map)
            return nullptr;
        return map->GetCreature(guid);
    }

    void ProcessPending()
    {
        // Process pending invasions
        for (auto it = _data.pendingInvasions.begin(); it != _data.pendingInvasions.end();)
        {
            uint32 zoneId = *it;
            InvasionZoneDef const* def = FindInvasionZoneByZoneId(zoneId);
            if (def)
                if (Map* map = sMapMgr->FindMap(def->map, 0))
                {
                    it = _data.pendingInvasions.erase(it);
                    StartNewInvasion(zoneId);
                    continue;
                }
            ++it;
        }

        // Process pending pallids
        for (auto it = _data.pendingPallids.begin(); it != _data.pendingPallids.end();)
        {
            uint32 zoneId = *it;
            for (auto const& def : g_pallidDefs)
                if (def.zoneId == zoneId)
                    if (Map* map = sMapMgr->FindMap(def.map, 0))
                    {
                        it = _data.pendingPallids.erase(it);
                        StartNewCityAttack(zoneId);
                        continue;
                    }
            ++it;
        }
    }

    void Update()
    {
        if (_data.state != SI_STATE_ENABLED)
            return;

        ProcessPending();

        // Check zone attack timers
        for (auto const& def : g_invasionZoneDefs)
        {
            if (_data.remaining[def.remainingIdx] > 0)
                continue; // Already active

            TimePoint timer = _data.timers[def.timerIdx];
            if (timer != TimePoint() && std::chrono::steady_clock::now() >= timer)
                StartNewInvasion(def.zoneId);
        }

        // Check city attack timers
        for (auto const& def : g_pallidDefs)
        {
            if (_data.timers[def.timerIdx] != TimePoint() && std::chrono::steady_clock::now() >= _data.timers[def.timerIdx])
            {
                ObjectGuid guid = GetPallidGuid(def.zoneId);
                if (guid.IsEmpty())
                    StartNewCityAttack(def.zoneId);
            }
        }
    }

    void HandleDefendedZones()
    {
        // Handle milestone events (50/100/150 invasions)
        if (_data.battlesWon >= 50 && !sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_50_INVASIONS))
            sGameEventMgr->StartEvent(GAME_EVENT_SCOURGE_INVASION_50_INVASIONS, true);
        if (_data.battlesWon >= 100 && !sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_100_INVASIONS))
            sGameEventMgr->StartEvent(GAME_EVENT_SCOURGE_INVASION_100_INVASIONS, true);
        if (_data.battlesWon >= 150 && !sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_150_INVASIONS))
            sGameEventMgr->StartEvent(GAME_EVENT_SCOURGE_INVASION_150_INVASIONS, true);
    }

private:
    ScourgeInvasionData _data;

    ScourgeInvasionMgr() = default;

    void StartEvents()
    {
        if (!sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION))
            sGameEventMgr->StartEvent(GAME_EVENT_SCOURGE_INVASION, true);
        if (!sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_BOSSES))
            sGameEventMgr->StartEvent(GAME_EVENT_SCOURGE_INVASION_BOSSES, true);

        // Set initial timers if not already set
        TimePoint now = std::chrono::steady_clock::now();
        for (auto const& def : g_invasionZoneDefs)
        {
            if (_data.timers[def.timerIdx] == TimePoint())
                _data.timers[def.timerIdx] = now + std::chrono::seconds(urand(300, 600));
        }
        for (auto const& def : g_pallidDefs)
        {
            if (_data.timers[def.timerIdx] == TimePoint())
                _data.timers[def.timerIdx] = now + std::chrono::seconds(urand(600, 1200));
        }
        SaveToDB();
    }

    void StopEvents()
    {
        if (sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION))
            sGameEventMgr->StopEvent(GAME_EVENT_SCOURGE_INVASION, true);
        if (sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_BOSSES))
            sGameEventMgr->StopEvent(GAME_EVENT_SCOURGE_INVASION_BOSSES, true);

        for (auto const& def : g_invasionZoneDefs)
        {
            if (sGameEventMgr->IsActiveEvent(GAME_EVENT_SCOURGE_INVASION_WINTERSPRING + (&def - g_invasionZoneDefs)))
                sGameEventMgr->StopEvent(GAME_EVENT_SCOURGE_INVASION_WINTERSPRING + (&def - g_invasionZoneDefs), true);
            _data.remaining[def.remainingIdx] = 0;
        }
        BroadcastWorldStates();
        SaveToDB();
    }
};

#define sScourgeInvasionMgr ScourgeInvasionMgr::instance()

// ===== NPC: go_necropolis =====

struct go_necropolis : public GameObjectAI
{
    go_necropolis(GameObject* go) : GameObjectAI(go) { }
};

// ===== NPC: Herald of the Lich King =====

struct npc_herald_of_the_lich_king : public ScriptedAI
{
    npc_herald_of_the_lich_king(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
        me->setActive(true);
    }

    void DoAction(int32 action) override
    {
        if (action == EVENT_HERALD_OF_THE_LICH_KING_ZONE_START)
        {
            Talk(HERALD_OF_THE_LICH_KING_SAY_ATTACK_START);
            UpdateWeather(true);
        }
        else if (action == EVENT_HERALD_OF_THE_LICH_KING_ZONE_STOP)
        {
            Talk(HERALD_OF_THE_LICH_KING_SAY_ATTACK_END);
            UpdateWeather(false);
            me->DespawnOrUnsummon();
        }
    }

    void UpdateWeather(bool start)
    {
        Weather* weather = me->GetMap()->GetOrGenerateZoneDefaultWeather(me->GetZoneId());
        if (weather)
        {
            if (start)
                weather->SetWeather(WEATHER_TYPE_STORM, 0.25f);
            else
                weather->SetWeather(WEATHER_TYPE_RAIN, 0.0f);
        }
    }

    void InitializeAI() override
    {
        _scheduler.Schedule(Minutes(1), [this](TaskContext context)
        {
            Talk(HERALD_OF_THE_LICH_KING_SAY_ATTACK_RANDOM);
            context.Repeat(Minutes(15), Minutes(30));
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// ===== NPC: Necropolis =====

struct npc_necropolis : public ScriptedAI
{
    npc_necropolis(Creature* creature) : ScriptedAI(creature)
    {
        me->setActive(true);
    }

    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spell) override
    {
        if (me->HasAura(SPELL_COMMUNIQUE_TIMER_NECROPOLIS))
            return;

        if (spell->Id == SPELL_COMMUNIQUE_PROXY_TO_NECROPOLIS)
            DoCastSelf(SPELL_COMMUNIQUE_TIMER_NECROPOLIS, true);
    }
};

// ===== NPC: Necropolis Health =====

struct npc_necropolis_health : public ScriptedAI
{
    npc_necropolis_health(Creature* creature) : ScriptedAI(creature)
    {
        me->SetFullHealth();
    }

    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spell) override
    {
        if (spell->Id == SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH)
            DoCastSelf(SPELL_ZAP_NECROPOLIS, true);

        if (spell->Id == SPELL_ZAP_NECROPOLIS)
            if (++_zapCount >= 3)
                me->KillSelf();
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Creature* necropolis = me->FindNearestCreature(NPC_NECROPOLIS, 10.0f))
            me->CastSpell(necropolis, SPELL_DESPAWNER_OTHER, true);

        uint32 zoneId = me->GetZoneId();
        if (FindInvasionZoneByZoneId(zoneId))
            sScourgeInvasionMgr->HandleZoneNecropolisDestroyed(zoneId);
    }

    void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_DESPAWNER_OTHER && target->GetEntry() == NPC_NECROPOLIS)
        {
            DespawnNecropolis();
            if (Creature* c = target->ToCreature())
                c->DespawnOrUnsummon();
            me->DespawnOrUnsummon();
        }
    }

    void DespawnNecropolis()
    {
        uint32 const necropolisEntries[] = { GO_NECROPOLIS_TINY, GO_NECROPOLIS_SMALL, GO_NECROPOLIS_MEDIUM, GO_NECROPOLIS_BIG, GO_NECROPOLIS_HUGE };
        for (uint32 entry : necropolisEntries)
        {
            std::list<GameObject*> necropolisList;
            me->GetGameObjectListWithEntryInGrid(necropolisList, entry, 10.0f);
            for (GameObject* go : necropolisList)
                go->DespawnOrUnsummon();
        }
    }

private:
    int _zapCount = 0;
};

// ===== NPC: Necropolis Proxy =====

struct npc_necropolis_proxy : public ScriptedAI
{
    npc_necropolis_proxy(Creature* creature) : ScriptedAI(creature) { me->setActive(true); }

    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spell) override
    {
        switch (spell->Id)
        {
            case SPELL_COMMUNIQUE_NECROPOLIS_TO_PROXIES:
                DoCastSelf(SPELL_COMMUNIQUE_PROXY_TO_RELAY, true);
                break;
            case SPELL_COMMUNIQUE_RELAY_TO_PROXY:
                DoCastSelf(SPELL_COMMUNIQUE_PROXY_TO_NECROPOLIS, true);
                break;
            case SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH:
                if (Creature* health = me->FindNearestCreature(NPC_NECROPOLIS_HEALTH, 200.0f))
                    me->CastSpell(health, SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH, true);
                break;
        }
    }

    void SpellHitTarget(WorldObject* /*target*/, SpellInfo const* spell) override
    {
        if (spell->Id == SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH)
            me->DespawnOrUnsummon();
    }
};

// ===== NPC: Necropolis Relay =====

struct npc_necropolis_relay : public ScriptedAI
{
    npc_necropolis_relay(Creature* creature) : ScriptedAI(creature) { me->setActive(true); }

    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spell) override
    {
        switch (spell->Id)
        {
            case SPELL_COMMUNIQUE_PROXY_TO_RELAY:
                DoCastSelf(SPELL_COMMUNIQUE_RELAY_TO_CAMP, true);
                break;
            case SPELL_COMMUNIQUE_CAMP_TO_RELAY:
                DoCastSelf(SPELL_COMMUNIQUE_RELAY_TO_PROXY, true);
                break;
            case SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH:
                if (Creature* proxy = me->FindNearestCreature(NPC_NECROPOLIS_PROXY, 200.0f))
                    me->CastSpell(proxy, SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH, true);
                break;
        }
    }

    void SpellHitTarget(WorldObject* /*target*/, SpellInfo const* spell) override
    {
        if (spell->Id == SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH)
            me->DespawnOrUnsummon();
    }
};

// ===== NPC: Necrotic Shard =====

struct npc_necrotic_shard : public ScriptedAI
{
    npc_necrotic_shard(Creature* creature) : ScriptedAI(creature)
    {
        me->setActive(true);
        me->SetReactState(REACT_PASSIVE);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_HEAL, true);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_HEAL_PCT, true);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_HEAL_MAX_HEALTH, true);
        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_PERIODIC_HEAL, true);
    }

    void Reset() override
    {
        _events.Reset();
        if (me->GetEntry() == NPC_NECROTIC_SHARD)
        {
            _events.ScheduleEvent(EVENT_SHARD_MINION_SPAWNER_SMALL, 5s);
            _events.ScheduleEvent(EVENT_SHARD_MINION_SPAWNER_BUTTRESS, 5s);
            _events.ScheduleEvent(EVENT_SHARD_FIND_DAMAGED_SHARD, 10s);
        }
        if (me->GetEntry() == NPC_DAMAGED_NECROTIC_SHARD)
        {
            _events.ScheduleEvent(EVENT_CULTIST_CHANNELING, 100ms);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SHARD_MINION_SPAWNER_SMALL:
                    HandleMinionSpawner();
                    _events.Repeat(5s);
                    break;
                case EVENT_SHARD_MINION_SPAWNER_BUTTRESS:
                    HandleCultistSpawner();
                    _events.Repeat(1h);
                    break;
                case EVENT_SHARD_FIND_DAMAGED_SHARD:
                    CheckForDamage();
                    _events.Repeat(10s);
                    break;
                case EVENT_CULTIST_CHANNELING:
                    // Look for cultists channeling
                    break;
            }
        }
    }

    void HandleMinionSpawner()
    {
        DoCastSelf(SPELL_MINION_SPAWNER_SMALL, true);
    }

    void HandleCultistSpawner()
    {
        // Despawn old shadows first
        std::list<Creature*> shadows;
        me->GetCreatureListWithEntryInGrid(shadows, NPC_SHADOW_OF_DOOM, 50.0f);
        for (Creature* s : shadows)
            s->DespawnOrUnsummon();

        // Spawn cultist
        me->SummonCreature(NPC_CULTIST_ENGINEER, me->GetPosition(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 1h);
    }

    void CheckForDamage()
    {
        if (me->GetEntry() != NPC_NECROTIC_SHARD)
            return;

        // Find if there's a damaged shard nearby
        if (Creature* damaged = me->FindNearestCreature(NPC_DAMAGED_NECROTIC_SHARD, 15.0f))
            if (!damaged->IsAlive())
                damaged->Respawn();
    }

    void JustSummoned(Creature* summon) override
    {
        summon->CastSpell(summon, SPELL_MINION_SPAWN_IN, true);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/) override
    {
        if (me->GetEntry() == NPC_NECROTIC_SHARD)
        {
            _zapCount++;
            if (_zapCount >= 4)
            {
                // Transform to damaged shard
                me->UpdateEntry(NPC_DAMAGED_NECROTIC_SHARD);
                me->SetFullHealth();
                _zapCount = 0;
                damage = 0;
            }
        }
    }

private:
    EventMap _events;
    uint8 _zapCount = 0;
};

// ===== NPC: Minion Spawner =====

struct npc_minion_spawner : public ScriptedAI
{
    npc_minion_spawner(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void Reset() override
    {
        _events.ScheduleEvent(EVENT_SPAWNER_SUMMON_MINION, 5s);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            if (eventId == EVENT_SPAWNER_SUMMON_MINION)
            {
                uint32 entry;
                switch (me->GetEntry())
                {
                    case NPC_SCOURGE_INVASION_MINION_SPAWNER_GHOST_GHOUL:
                        entry = CanSpawnRare() ? RAND(NPC_SPIRIT_OF_THE_DAMNED, NPC_LUMBERING_HORROR)
                            : RAND(NPC_SPECTRAL_SOLDIER, NPC_GHOUL_BERSERKER);
                        break;
                    case NPC_SCOURGE_INVASION_MINION_SPAWNER_GHOST_SKELETON:
                        entry = CanSpawnRare() ? RAND(NPC_SPIRIT_OF_THE_DAMNED, NPC_BONE_WITCH)
                            : RAND(NPC_SPECTRAL_SOLDIER, NPC_SKELETAL_SHOCKTROOPER);
                        break;
                    case NPC_SCOURGE_INVASION_MINION_SPAWNER_GHOUL_SKELETON:
                        entry = CanSpawnRare() ? RAND(NPC_LUMBERING_HORROR, NPC_BONE_WITCH)
                            : RAND(NPC_GHOUL_BERSERKER, NPC_SKELETAL_SHOCKTROOPER);
                        break;
                    default:
                        entry = NPC_GHOUL_BERSERKER;
                        break;
                }

                if (Creature* minion = me->SummonCreature(entry, me->GetPosition(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 1h))
                {
                    minion->SetWanderDistance(1.0f);
                    DoCastAOE(SPELL_MINION_SPAWN_IN);
                }

                _events.Repeat(5s);
            }
        }
    }

    bool CanSpawnRare()
    {
        std::list<Creature*> rares;
        me->GetCreatureListWithEntryInGrid(rares, NPC_LUMBERING_HORROR, 100.0f);
        me->GetCreatureListWithEntryInGrid(rares, NPC_BONE_WITCH, 100.0f);
        me->GetCreatureListWithEntryInGrid(rares, NPC_SPIRIT_OF_THE_DAMNED, 100.0f);
        for (Creature* r : rares)
            if (r->IsAlive())
                return false;

        return roll_chance_i(1);
    }

private:
    EventMap _events;
};

// ===== NPC: Cultist Engineer =====

struct npc_cultist_engineer : public ScriptedAI
{
    npc_cultist_engineer(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        me->SetReactState(REACT_PASSIVE);
        me->SetCorpseDelay(10);
        _events.ScheduleEvent(1, 1ms);
        _events.ScheduleEvent(2, 1s);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case 1:
                    DoCastSelf(SPELL_CREATE_SUMMONER_SHIELD, true);
                    DoCastSelf(SPELL_MINION_SPAWN_IN, true);
                    break;
                case 2:
                    DoCastSelf(SPELL_BUTTRESS_CHANNEL, true);
                    break;
            }
        }
    }

    bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 /*gossipListId*/) override
    {
        CloseGossipMenuFor(player);
        player->DestroyItemCount(ITEM_NECROTIC_RUNE, 8, true);
        player->CastSpell(nullptr, SPELL_SUMMON_BOSS, true);
        DoCastSelf(SPELL_QUIET_SUICIDE, true);
        return true;
    }

    void JustDied(Unit* /*killer*/) override
    {
        _events.Reset();
        if (Creature* shard = me->FindNearestCreature(NPC_DAMAGED_NECROTIC_SHARD, 15.0f))
            shard->CastSpell(shard, SPELL_DAMAGE_CRYSTAL, true);
        if (GameObject* shield = me->FindNearestGameObject(GO_SUMMONER_SHIELD, 5.0f))
            shield->Delete();
    }

private:
    EventMap _events;
};

// ===== NPC: Flameshocker =====

struct npc_flameshocker : public ScriptedAI
{
    npc_flameshocker(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
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

// ===== NPC: Pallid Horror =====

struct npc_pallid_horror : public ScriptedAI
{
    npc_pallid_horror(Creature* creature) : ScriptedAI(creature), _summons(me) { }

    void InitializeAI() override
    {
        _summons.DespawnAll();
        me->SetCorpseDelay(10);
        UpdateWeather(true);
        me->AddAura(SPELL_AURA_OF_FEAR, me);
        me->SetWalk(false);
        _scheduler.Schedule(Seconds(0), [this](TaskContext /*context*/) { SummonFlameshockers(); });
        _scheduler.Schedule(Seconds(1), [this](TaskContext context)
        {
            Talk(PALLID_HORROR_SAY_RANDOM_YELL);
            context.Repeat(Seconds(65), Seconds(300));
        });
        _scheduler.Schedule(Seconds(2), [this](TaskContext context)
        {
            if (_summons.size() >= 30)
            {
                context.Repeat(Seconds(10));
                return;
            }
            context.Repeat(Seconds(1));
        });
    }

    void SummonFlameshockers()
    {
        uint32 amount = urand(5, 9);
        for (uint32 i = 0; i < amount; ++i)
        {
            if (Creature* summon = me->SummonCreature(NPC_FLAMESHOCKER, me->GetPosition(), TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 1h))
            {
                float angle = float(i) * (M_PI / (float(amount) / 2.f)) + me->GetOrientation();
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
        if (Creature* sylvanas = me->FindNearestCreature(NPC_LADY_SYLVANAS_WINDRUNNER, VISIBILITY_DISTANCE_NORMAL))
            sylvanas->AI()->Talk(SYLVANAS_SAY_ATTACK_END);

        _summons.DespawnAll();

        DoCastSelf(me->GetZoneId() == AREA_UNDERCITY ? SPELL_SUMMON_FAINT_NECROTIC_CRYSTAL : SPELL_SUMMON_CRACKED_NECROTIC_CRYSTAL, true);

        // Reset city attack timer
        for (auto const& def : g_pallidDefs)
            if (def.zoneId == me->GetZoneId())
                sScourgeInvasionMgr->SetSITimer(def.timerIdx, std::chrono::steady_clock::now() + std::chrono::seconds(urand(CITY_ATTACK_TIMER_MIN, CITY_ATTACK_TIMER_MAX)));

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
        Weather* weather = me->GetMap()->GetOrGenerateZoneDefaultWeather(me->GetZoneId());
        if (weather)
        {
            if (start)
                weather->SetWeather(WEATHER_TYPE_STORM, 0.25f);
            else
                weather->SetWeather(WEATHER_TYPE_RAIN, 0.0f);
        }
    }

private:
    TaskScheduler _scheduler;
    SummonList _summons;
};

// ===== Spell Scripts =====

class spell_despawner_self : public SpellScript
{
    PrepareSpellScript(spell_despawner_self);

    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo({ SPELL_SPIRIT_SPAWN_OUT });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
            if (!caster->IsInCombat())
                caster->CastSpell(caster, SPELL_SPIRIT_SPAWN_OUT, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_despawner_self::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_communique_trigger : public SpellScript
{
    PrepareSpellScript(spell_communique_trigger);

    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo({ SPELL_COMMUNIQUE_CAMP_TO_RELAY });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetHitUnit())
            target->CastSpell(nullptr, SPELL_COMMUNIQUE_CAMP_TO_RELAY, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_communique_trigger::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_scourge_invasion_scourge_strike : public SpellScript
{
    PrepareSpellScript(spell_scourge_invasion_scourge_strike);

    SpellCastResult CheckCast()
    {
        Unit* target = GetExplTargetUnit();
        if (!target || target->IsPlayer() || target->IsCharmedOwnedByPlayerOrPlayer())
            return SPELL_FAILED_BAD_TARGETS;
        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_scourge_invasion_scourge_strike::CheckCast);
    }
};

// ===== SI Controller NPC (invisible, runs SI update logic) =====

struct npc_si_controller : public ScriptedAI
{
    npc_si_controller(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
        me->setActive(true);
        me->SetDisplayId(11686); // invisible
        me->SetUnitFlag(UnitFlags(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_UNINTERACTIBLE));
        _updateTimer = 10000; // Check every 10 seconds
    }

    void UpdateAI(uint32 diff) override
    {
        _updateTimer += diff;
        if (_updateTimer >= 10000)
        {
            _updateTimer = 0;
            sScourgeInvasionMgr->Update();
        }
    }

private:
    uint32 _updateTimer = 0;
};

void AddSC_scourge_invasion()
{
    RegisterGameObjectAI(go_necropolis);
    RegisterCreatureAI(npc_herald_of_the_lich_king);
    RegisterCreatureAI(npc_necropolis);
    RegisterCreatureAI(npc_necropolis_health);
    RegisterCreatureAI(npc_necropolis_proxy);
    RegisterCreatureAI(npc_necropolis_relay);
    RegisterCreatureAI(npc_necrotic_shard);
    RegisterCreatureAI(npc_minion_spawner);
    RegisterCreatureAI(npc_cultist_engineer);
    RegisterCreatureAI(npc_flameshocker);
    RegisterCreatureAI(npc_pallid_horror);
    RegisterCreatureAI(npc_si_controller);
    RegisterSpellScript(spell_communique_trigger);
    RegisterSpellScript(spell_despawner_self);
    RegisterSpellScript(spell_scourge_invasion_scourge_strike);
}
