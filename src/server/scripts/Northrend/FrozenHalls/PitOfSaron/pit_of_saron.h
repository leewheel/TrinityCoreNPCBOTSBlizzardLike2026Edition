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

#ifndef PIT_OF_SARON_H_
#define PIT_OF_SARON_H_

#include "CreatureAIImpl.h"
#include "EventProcessor.h"

#define PoSScriptName "instance_pit_of_saron"
#define DataHeader "POS"

uint32 const EncounterCount = 3;

enum DataTypes
{
    DATA_GARFROST,
    DATA_ICK,
    DATA_TYRANNUS,
    MAX_ENCOUNTER,

    DATA_INSTANCE_PROGRESS,
    DATA_TEAMID_IN_INSTANCE,
    DATA_TYRANNUS_EVENT_GUID,
    DATA_NECROLYTE_1_GUID,
    DATA_NECROLYTE_2_GUID,
    DATA_GUARD_1_GUID,
    DATA_GUARD_2_GUID,
    DATA_LEADER_FIRST_GUID,
    DATA_GARFROST_GUID,
    DATA_MARTIN_OR_GORKUN_GUID,
    DATA_RIMEFANG_GUID,
    DATA_TYRANNUS_GUID,
    DATA_LEADER_SECOND_GUID,
    DATA_SINDRAGOSA_GUID,
    DATA_ACHIEV_ELEVEN,
    DATA_ACHIEV_DONT_LOOK_UP,
    DATA_START_INTRO,

    DATA_KRICK,
    DATA_RIMEFANG,
    DATA_TYRANNUS_EVENT,
    DATA_JAINA_SYLVANAS_1,
    DATA_JAINA_SYLVANAS_2,
    DATA_ICE_SHARDS_HIT,
    DATA_CAVERN_ACTIVE

    // Backward-compatible aliases for TC boss files
};

#define DATA_TEAM_IN_INSTANCE DATA_TEAMID_IN_INSTANCE
#define NPC_TYRANNUS_EVENTS NPC_TYRANNUS_EVENT

enum InstanceProgressConst
{
    INSTANCE_PROGRESS_NONE,
    INSTANCE_PROGRESS_FINISHED_INTRO,
    INSTANCE_PROGRESS_FINISHED_KRICK_SCENE,
    INSTANCE_PROGRESS_AFTER_WARN_1,
    INSTANCE_PROGRESS_AFTER_WARN_2,
    INSTANCE_PROGRESS_AFTER_TUNNEL_WARN,
    INSTANCE_PROGRESS_TYRANNUS_INTRO,
};

enum CreatureIds
{
    NPC_GARFROST                                = 36494,
    NPC_KRICK                                   = 36477,
    NPC_ICK                                     = 36476,
    NPC_TYRANNUS                                = 36658,
    NPC_RIMEFANG                                = 36661,
    NPC_SINDRAGOSA                              = 37755,

    NPC_TYRANNUS_EVENT                          = 36794,
    NPC_TYRANNUS_VOICE                          = 36795,
    NPC_SYLVANAS_PART1                          = 36990,
    NPC_SYLVANAS_PART2                          = 38189,
    NPC_JAINA_PART1                             = 36993,
    NPC_JAINA_PART2                             = 38188,

    NPC_KILARA                                  = 37583,
    NPC_ELANDRA                                 = 37774,
    NPC_KORALEN                                 = 37779,
    NPC_KORLAEN                                 = 37582,
    NPC_LORALEN                                 = 37779,  // alias for NPC_KORALEN
    NPC_KALIRA                                  = 37583,  // alias for NPC_KILARA

    NPC_CHAMPION_1_HORDE                        = 37584,
    NPC_CHAMPION_2_HORDE                        = 37587,
    NPC_CHAMPION_3_HORDE                        = 37588,
    NPC_CHAMPION_1_ALLIANCE                     = 37496,
    NPC_CHAMPION_2_ALLIANCE                     = 37497,

    NPC_HORDE_SLAVE_1                           = 36770,
    NPC_HORDE_SLAVE_2                           = 36771,
    NPC_HORDE_SLAVE_3                           = 36772,
    NPC_HORDE_SLAVE_4                           = 36773,
    NPC_ALLIANCE_SLAVE_1                        = 36764,
    NPC_ALLIANCE_SLAVE_2                        = 36765,
    NPC_ALLIANCE_SLAVE_3                        = 36766,
    NPC_ALLIANCE_SLAVE_4                        = 36767,

    NPC_RESCUED_ALLIANCE_SLAVE                  = 36888,
    NPC_RESCUED_HORDE_SLAVE                     = 36889,

    NPC_YMIRJAR_DEATHBRINGER                    = 36892,
    NPC_YMIRJAR_WRATHBRINGER                    = 36840,
    NPC_YMIRJAR_FLAMEBEARER                     = 36893,

    NPC_FALLEN_WARRIOR                          = 36841,
    NPC_WRATHBONE_COLDWRAITH                    = 36842,

    NPC_MARTIN_VICTUS_1                         = 37591,
    NPC_GORKUN_IRONSKULL_1                      = 37592,

    NPC_MARTIN_VICTUS_2                         = 37580,
    NPC_GORKUN_IRONSKULL_2                      = 37581,
    NPC_FREED_SLAVE_1_ALLIANCE                  = 37576, // mage
    NPC_FREED_SLAVE_2_ALLIANCE                  = 37575, // warr
    NPC_FREED_SLAVE_3_ALLIANCE                  = 37572, // warr
    NPC_FREED_SLAVE_1_HORDE                     = 37579, // mage
    NPC_FREED_SLAVE_2_HORDE                     = 37578, // warr
    NPC_FREED_SLAVE_3_HORDE                     = 37577, // warr

    NPC_FORGEMASTER_STALKER                     = 36495,
    NPC_EXPLODING_ORB                           = 36610,
    NPC_ICY_BLAST                               = 36731,
    NPC_CAVERN_EVENT_TRIGGER                    = 32780
};

enum GameObjectIds
{
    GO_HOR_PORTCULLIS                           = 201848,
    GO_ICE_WALL                                 = 201885,
    GO_SARONITE_ROCK                            = 196485
};

enum eSpells
{
    SPELL_NECROLYTE_CHANNELING                  = 30540,
    SPELL_TUNNEL_ICICLE                         = 69424,
    SPELL_ICICLE_FALL_TRIGGER                   = 69426,
    SPELL_ICICLE_FALL_VISUAL                    = 69428,
    SPELL_TELEPORT_JAINA_VISUAL                 = 70623,
    SPELL_TELEPORT_JAINA                        = 70525,
    SPELL_TELEPORT_SYLVANAS_VISUAL              = 70638,
    SPELL_TELEPORT_SYLVANAS                     = 70639,
    SPELL_SINDRAGOSA_FROST_BOMB_POS             = 70521,
    SPELL_DONT_LOOK_UP_ACHIEV_CREDIT            = 72845
};

#define PATH_BEGIN_VALUE 3000200

// Position and data structures (values defined in pit_of_saron.cpp)
extern Position const PortalPos;
extern Position const LeaderIntroPos;
extern Position const NecrolytePos1;
extern Position const NecrolytePos2;

struct ChampionPosition
{
    uint32 entry[2];
    Position endPosition;
};

extern ChampionPosition const introPositions[];

extern Position const FBSSpawnPos;

struct FBSPosition
{
    uint32 entry;
    uint32 pathId;
};

extern FBSPosition const FBSData[];

extern Position const KrickCenterPos;
extern Position const SBSTyrannusStartPos;
extern Position const SBSLeaderStartPos;
extern Position const SBSLeaderEndPos;

extern Position const PTSTyrannusWaitPos1;
extern Position const PTSTyrannusWaitPos2;
extern Position const PTSTyrannusWaitPos3;

extern Position const TSSpawnPos;
extern Position const TSMidPos;
extern float const TSHeight;
extern Position const TSLeaderSpawnPos;
extern Position const TSCenterPos;
extern Position const TSDistCheckPos;
extern Position const TSSindragosaPos1;
extern Position const TSSindragosaPos2;

extern Position const EventLeaderPos2;
extern Position const SlaveLeaderPos;

struct TSPosition
{
    uint32 entry;
    float x, y;
};

extern TSPosition const TSData[];

// TC-specific: icicle summon event for cavern achievement
class ScheduledIcicleSummons : public BasicEvent
{
    public:
        ScheduledIcicleSummons(Creature* trigger) : _trigger(trigger) { }
        bool Execute(uint64 /*time*/, uint32 /*diff*/) override;
    private:
        Creature* _trigger;
};

template <class AI, class T>
inline AI* GetPitOfSaronAI(T* obj)
{
    return GetInstanceAI<AI>(obj, PoSScriptName);
}

#define RegisterPitOfSaronCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetPitOfSaronAI)

// TC backward-compatible name aliases
#define NPC_KORELN NPC_KORLAEN

#endif // PIT_OF_SARON_H_
