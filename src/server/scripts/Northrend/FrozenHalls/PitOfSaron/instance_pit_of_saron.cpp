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

#include "ScriptMgr.h"
#include "Creature.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Map.h"
#include "pit_of_saron.h"
#include "Player.h"
#include "TemporarySummon.h"

DoorData const Doors[] =
{
    { GO_ICE_WALL,                       DATA_GARFROST,  DOOR_TYPE_PASSAGE },
    { GO_ICE_WALL,                       DATA_ICK,       DOOR_TYPE_PASSAGE },
    { GO_HOR_PORTCULLIS,                 DATA_TYRANNUS,  DOOR_TYPE_PASSAGE },
    { 0,                                 0,              DOOR_TYPE_ROOM    } // END
};

class instance_pit_of_saron : public InstanceMapScript
{
    public:
        instance_pit_of_saron() : InstanceMapScript(PoSScriptName, 658) { }

        struct instance_pit_of_saron_InstanceScript : public InstanceScript
        {
            instance_pit_of_saron_InstanceScript(InstanceMap* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(EncounterCount);
                LoadDoorData(Doors);

                InstanceProgress = INSTANCE_PROGRESS_NONE;
                _teamInInstance = 0;
                _cavernActive = 0;
                _shardsHit = 0;
                bAchievEleven = true;
                bAchievDontLookUp = true;
            }

            uint32 InstanceProgress;
            bool bAchievEleven;
            bool bAchievDontLookUp;

            void Initialize()
            {
                InstanceProgress = INSTANCE_PROGRESS_NONE;
                bAchievEleven = true;
                bAchievDontLookUp = true;
            }

            void OnPlayerEnter(Player* player) override
            {
                if (!_teamInInstance)
                    _teamInInstance = player->GetTeam();

                // Trigger intro if not started yet
                if (Creature* leader = instance->GetCreature(_leaderFirstGUID))
                    leader->AI()->SetData(DATA_START_INTRO, 0);
            }

            uint32 GetCreatureEntry(ObjectGuid::LowType /*guidLow*/, CreatureData const* data) override
            {
                uint32 entry = data->id;
                if (!_teamInInstance)
                {
                    Map::PlayerList const& players = instance->GetPlayers();
                    if (!players.isEmpty())
                        if (Player* player = players.begin()->GetSource())
                            _teamInInstance = player->GetTeam();
                }

                switch (entry)
                {
                    case NPC_SYLVANAS_PART1:
                        return _teamInInstance == ALLIANCE ? NPC_JAINA_PART1 : NPC_SYLVANAS_PART1;
                    case NPC_SYLVANAS_PART2:
                        return _teamInInstance == ALLIANCE ? NPC_JAINA_PART2 : NPC_SYLVANAS_PART2;
                    case NPC_KILARA:
                        return _teamInInstance == ALLIANCE ? NPC_ELANDRA : NPC_KILARA;
                    case NPC_KORALEN:
                        return _teamInInstance == ALLIANCE ? NPC_KORELN : NPC_KORALEN;
                    case NPC_CHAMPION_1_HORDE:
                        return _teamInInstance == ALLIANCE ? NPC_CHAMPION_1_ALLIANCE : NPC_CHAMPION_1_HORDE;
                    case NPC_CHAMPION_2_HORDE:
                        return _teamInInstance == ALLIANCE ? NPC_CHAMPION_2_ALLIANCE : NPC_CHAMPION_2_HORDE;
                    case NPC_CHAMPION_3_HORDE:
                        return _teamInInstance == ALLIANCE ? NPC_CHAMPION_2_ALLIANCE : NPC_CHAMPION_3_HORDE;
                    case NPC_HORDE_SLAVE_1:
                        return _teamInInstance == ALLIANCE ? NPC_ALLIANCE_SLAVE_1 : NPC_HORDE_SLAVE_1;
                    case NPC_HORDE_SLAVE_2:
                        return _teamInInstance == ALLIANCE ? NPC_ALLIANCE_SLAVE_2 : NPC_HORDE_SLAVE_2;
                    case NPC_HORDE_SLAVE_3:
                        return _teamInInstance == ALLIANCE ? NPC_ALLIANCE_SLAVE_3 : NPC_HORDE_SLAVE_3;
                    case NPC_HORDE_SLAVE_4:
                        return _teamInInstance == ALLIANCE ? NPC_ALLIANCE_SLAVE_4 : NPC_HORDE_SLAVE_4;
                    case NPC_FREED_SLAVE_1_HORDE:
                        return _teamInInstance == ALLIANCE ? NPC_FREED_SLAVE_1_ALLIANCE : NPC_FREED_SLAVE_1_HORDE;
                    case NPC_FREED_SLAVE_2_HORDE:
                        return _teamInInstance == ALLIANCE ? NPC_FREED_SLAVE_2_ALLIANCE : NPC_FREED_SLAVE_2_HORDE;
                    case NPC_FREED_SLAVE_3_HORDE:
                        return _teamInInstance == ALLIANCE ? NPC_FREED_SLAVE_3_ALLIANCE : NPC_FREED_SLAVE_3_HORDE;
                    case NPC_RESCUED_HORDE_SLAVE:
                        return _teamInInstance == ALLIANCE ? NPC_RESCUED_ALLIANCE_SLAVE : NPC_RESCUED_HORDE_SLAVE;
                    case NPC_GORKUN_IRONSKULL_1:
                        return _teamInInstance == ALLIANCE ? NPC_MARTIN_VICTUS_1 : NPC_GORKUN_IRONSKULL_1;
                    case NPC_GORKUN_IRONSKULL_2:
                        return _teamInInstance == ALLIANCE ? NPC_MARTIN_VICTUS_2 : NPC_GORKUN_IRONSKULL_2;
                    default:
                        return entry;
                }
            }

            void OnCreatureCreate(Creature* creature) override
            {
                if (!_teamInInstance)
                {
                    Map::PlayerList const& players = instance->GetPlayers();
                    if (!players.isEmpty())
                        if (Player* player = players.begin()->GetSource())
                            _teamInInstance = player->GetTeam();
                }

                switch (creature->GetEntry())
                {
                    case NPC_SYLVANAS_PART1:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_JAINA_PART1);
                        _leaderFirstGUID = creature->GetGUID();
                        if (InstanceProgress >= INSTANCE_PROGRESS_FINISHED_INTRO)
                            creature->UpdatePosition(LeaderIntroPos);
                        break;
                    case NPC_SYLVANAS_PART2:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_JAINA_PART2);
                        _leaderSecondGUID = creature->GetGUID();
                        break;
                    case NPC_TYRANNUS_EVENT:
                        _tyrannusEventGUID = creature->GetGUID();
                        break;
                    case NPC_LORALEN:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_ELANDRA);
                        if (!_guardFirstGUID)
                            _guardFirstGUID = creature->GetGUID();
                        break;
                    case NPC_KALIRA:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_KORELN);
                        if (!_guardSecondGUID)
                            _guardSecondGUID = creature->GetGUID();
                        break;
                    case NPC_HORDE_SLAVE_1:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_ALLIANCE_SLAVE_1);
                        break;
                    case NPC_HORDE_SLAVE_2:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_ALLIANCE_SLAVE_2);
                        break;
                    case NPC_HORDE_SLAVE_3:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_ALLIANCE_SLAVE_3);
                        break;
                    case NPC_HORDE_SLAVE_4:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_ALLIANCE_SLAVE_4);
                        break;
                    case NPC_GORKUN_IRONSKULL_1:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_MARTIN_VICTUS_1);
                        _martinOrGorkunGUID = creature->GetGUID();
                        break;
                    case NPC_GARFROST:
                        _garfrostGUID = creature->GetGUID();
                        break;
                    case NPC_KRICK:
                        _krickGUID = creature->GetGUID();
                        break;
                    case NPC_ICK:
                        _ickGUID = creature->GetGUID();
                        break;
                    case NPC_TYRANNUS:
                        _tyrannusGUID = creature->GetGUID();
                        break;
                    case NPC_RIMEFANG:
                        _rimefangGUID = creature->GetGUID();
                        break;
                    case NPC_SINDRAGOSA:
                        _sindragosaGUID = creature->GetGUID();
                        break;
                    case NPC_FREED_SLAVE_1_HORDE:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_FREED_SLAVE_1_ALLIANCE);
                        break;
                    case NPC_FREED_SLAVE_2_HORDE:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_FREED_SLAVE_2_ALLIANCE);
                        break;
                    case NPC_FREED_SLAVE_3_HORDE:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_FREED_SLAVE_3_ALLIANCE);
                        break;
                    case NPC_GORKUN_IRONSKULL_2:
                        if (!_martinOrGorkunGUID.IsEmpty())
                            if (Creature* c = instance->GetCreature(_martinOrGorkunGUID))
                            {
                                c->AI()->DoAction(1);
                                c->DespawnOrUnsummon();
                            }
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_MARTIN_VICTUS_2);
                        _martinOrGorkunGUID = creature->GetGUID();
                        break;
                    case NPC_CAVERN_EVENT_TRIGGER:
                        _cavernstriggersVector.push_back(creature->GetGUID());
                        break;
                    default:
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* go) override
            {
                switch (go->GetEntry())
                {
                    case GO_ICE_WALL:
                        _iceWallGUID = go->GetGUID();
                        break;
                        break;
                }
            }

            bool SetBossState(uint32 type, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                switch (type)
                {
                    case DATA_GARFROST:
                        if (state == DONE)
                        {
                            if (_teamInInstance == ALLIANCE)
                            {
                                if (TempSummon* summon = instance->SummonCreature(NPC_MARTIN_VICTUS_1, SlaveLeaderPos))
                                    summon->SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);
                            }
                            else
                            {
                                if (TempSummon* summon = instance->SummonCreature(NPC_GORKUN_IRONSKULL_1, SlaveLeaderPos))
                                    summon->SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);
                            }
                        }
                        break;
                    case DATA_TYRANNUS:
                        if (state == DONE)
                        {
                            if (_teamInInstance == ALLIANCE)
                            {
                                if (TempSummon* summon = instance->SummonCreature(NPC_JAINA_PART2, EventLeaderPos2))
                                    summon->SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);
                            }
                            else
                            {
                                if (TempSummon* summon = instance->SummonCreature(NPC_SYLVANAS_PART2, EventLeaderPos2))
                                    summon->SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);
                            }
                        }
                        break;
                    default:
                        break;
                }

                return true;
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case DATA_INSTANCE_PROGRESS:
                        if (InstanceProgress < data)
                        {
                            InstanceProgress = data;
                            if (InstanceProgress == INSTANCE_PROGRESS_TYRANNUS_INTRO && instance->GetDifficulty() == DUNGEON_DIFFICULTY_HEROIC && bAchievDontLookUp)
                                DoUpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, 72845);
                        }
                        break;
                    case DATA_ACHIEV_ELEVEN:
                        bAchievEleven = false;
                        break;
                    case DATA_ACHIEV_DONT_LOOK_UP:
                        bAchievDontLookUp = false;
                        break;
                    case DATA_ICE_SHARDS_HIT:
                        _shardsHit = data;
                        break;
                    case DATA_CAVERN_ACTIVE:
                        if (data)
                        {
                            _cavernActive = data;
                            HandleCavernEventTrigger(true);
                        }
                        else
                            HandleCavernEventTrigger(false);
                        break;
                }

                if (data == DONE || type == DATA_INSTANCE_PROGRESS)
                    SaveToDB();
            }

            uint32 GetData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_INSTANCE_PROGRESS:
                        return InstanceProgress;
                    case DATA_TEAMID_IN_INSTANCE:
                        return _teamInInstance;
                    case DATA_ICE_SHARDS_HIT:
                        return _shardsHit;
                    case DATA_CAVERN_ACTIVE:
                        return _cavernActive;
                    default:
                        return 0;
                }
            }

            void SetGuidData(uint32 type, ObjectGuid data) override
            {
                switch (type)
                {
                    case DATA_NECROLYTE_1_GUID:
                        _necrolyte1GUID = data;
                        break;
                    case DATA_NECROLYTE_2_GUID:
                        _necrolyte2GUID = data;
                        break;
                    case DATA_MARTIN_OR_GORKUN_GUID:
                        _martinOrGorkunGUID = data;
                        break;
                }
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_TYRANNUS_EVENT_GUID:
                        return _tyrannusEventGUID;
                    case DATA_NECROLYTE_1_GUID:
                        return _necrolyte1GUID;
                    case DATA_NECROLYTE_2_GUID:
                        return _necrolyte2GUID;
                    case DATA_GUARD_1_GUID:
                        return _guardFirstGUID;
                    case DATA_GUARD_2_GUID:
                        return _guardSecondGUID;
                    case DATA_LEADER_FIRST_GUID:
                        return _leaderFirstGUID;
                    case DATA_GARFROST_GUID:
                        return _garfrostGUID;
                    case DATA_MARTIN_OR_GORKUN_GUID:
                        return _martinOrGorkunGUID;
                    case DATA_RIMEFANG_GUID:
                        return _rimefangGUID;
                    case DATA_TYRANNUS_GUID:
                        return _tyrannusGUID;
                    case DATA_LEADER_SECOND_GUID:
                        return _leaderSecondGUID;
                    case DATA_SINDRAGOSA_GUID:
                        return _sindragosaGUID;
                    case DATA_GARFROST:
                        return _garfrostGUID;
                    case DATA_KRICK:
                        return _krickGUID;
                    case DATA_ICK:
                        return _ickGUID;
                    case DATA_TYRANNUS:
                        return _tyrannusGUID;
                    case DATA_RIMEFANG:
                        return _rimefangGUID;
                    case DATA_TYRANNUS_EVENT:
                        return _tyrannusEventGUID;
                    case DATA_JAINA_SYLVANAS_1:
                        return _leaderFirstGUID;
                    case DATA_JAINA_SYLVANAS_2:
                        return _leaderSecondGUID;
                    default:
                        return ObjectGuid::Empty;
                }
            }

            bool CheckAchievementCriteriaMeet(uint32 criteria_id, Player const* /*source*/, Unit const* /*target*/, uint32 /*miscvalue1*/) override
            {
                switch (criteria_id)
                {
                    case 12993: // Doesn't Go to Eleven (4524)
                        return bAchievEleven;
                }
                return false;
            }

            void HandleCavernEventTrigger(bool activate)
            {
                for (ObjectGuid guid : _cavernstriggersVector)
                    if (Creature* trigger = instance->GetCreature(guid))
                    {
                        if (activate)
                            trigger->m_Events.AddEvent(new ScheduledIcicleSummons(trigger), trigger->m_Events.CalculateTime(1s));
                        else
                            trigger->m_Events.KillAllEvents(false);
                    }
            }

        private:
            ObjectGuid _necrolyte1GUID;
            ObjectGuid _necrolyte2GUID;
            ObjectGuid _guardFirstGUID;
            ObjectGuid _guardSecondGUID;
            ObjectGuid _leaderFirstGUID;
            ObjectGuid _leaderSecondGUID;
            ObjectGuid _tyrannusEventGUID;
            ObjectGuid _garfrostGUID;
            ObjectGuid _krickGUID;
            ObjectGuid _ickGUID;
            ObjectGuid _tyrannusGUID;
            ObjectGuid _rimefangGUID;
            ObjectGuid _sindragosaGUID;
            ObjectGuid _martinOrGorkunGUID;
            ObjectGuid _iceWallGUID;

            GuidVector _cavernstriggersVector;

            uint32 _teamInInstance;
            uint8 _shardsHit;
            uint8 _cavernActive;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_pit_of_saron_InstanceScript(map);
        }
};

void AddSC_instance_pit_of_saron()
{
    new instance_pit_of_saron();
}
