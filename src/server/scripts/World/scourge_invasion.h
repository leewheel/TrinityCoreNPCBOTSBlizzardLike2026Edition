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

#ifndef SCOURGE_INVASION_H
#define SCOURGE_INVASION_H

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <map>
#include <set>
#include <mutex>
#include <chrono>

// Game Event IDs
enum ScourgeInvasionGameEvents
{
    GAME_EVENT_SCOURGE_INVASION                         = 17,
    GAME_EVENT_SCOURGE_INVASION_BOSSES                  = 120,
    GAME_EVENT_SCOURGE_INVASION_WINTERSPRING            = 121,
    GAME_EVENT_SCOURGE_INVASION_TANARIS                 = 122,
    GAME_EVENT_SCOURGE_INVASION_AZSHARA                 = 123,
    GAME_EVENT_SCOURGE_INVASION_BLASTED_LANDS           = 124,
    GAME_EVENT_SCOURGE_INVASION_EASTERN_PLAGUELANDS     = 125,
    GAME_EVENT_SCOURGE_INVASION_BURNING_STEPPES         = 126,
    GAME_EVENT_SCOURGE_INVASION_50_INVASIONS            = 127,
    GAME_EVENT_SCOURGE_INVASION_100_INVASIONS           = 128,
    GAME_EVENT_SCOURGE_INVASION_150_INVASIONS           = 129,
    GAME_EVENT_SCOURGE_INVASION_INVASIONS_DONE          = 130,
};

// Spells
enum ScourgeInvasionSpells
{
    SPELL_SPIRIT_PARTICLES_PURPLE               = 28126,
    SPELL_SUMMON_NECROPOLIS_CRITTERS            = 27866,
    SPELL_DESPAWNER_OTHER                       = 28349,
    SPELL_ZAP_NECROPOLIS                        = 28386,
    SPELL_COMMUNIQUE_TIMER_NECROPOLIS           = 28395,
    SPELL_COMMUNIQUE_NECROPOLIS_TO_PROXIES      = 28373,
    SPELL_COMMUNIQUE_PROXY_TO_NECROPOLIS        = 28367,
    SPELL_COMMUNIQUE_PROXY_TO_RELAY             = 28366,
    SPELL_COMMUNIQUE_RELAY_TO_PROXY             = 28365,
    SPELL_COMMUNIQUE_RELAY_TO_CAMP              = 28326,
    SPELL_CREATE_CRYSTAL                        = 28344,
    SPELL_CREATE_CRYSTAL_CORPSE                 = 27895,
    SPELL_CAMP_RECEIVES_COMMUNIQUE              = 28449,
    SPELL_COMMUNIQUE_TIMER_CAMP                 = 28346,
    SPELL_COMMUNIQUE_TRIGGER                    = 28345,
    SPELL_DAMAGE_CRYSTAL                        = 28041,
    SPELL_SOUL_REVIVAL                          = 28681,
    SPELL_CAMP_TYPE_GHOST_SKELETON              = 28197,
    SPELL_CAMP_TYPE_GHOST_GHOUL                 = 28198,
    SPELL_CAMP_TYPE_GHOUL_SKELETON              = 28199,
    SPELL_MINION_SPAWNER_SMALL                  = 27887,
    SPELL_MINION_SPAWNER_BUTTRESS               = 27888,
    SPELL_CHOOSE_CAMP_TYPE                      = 28201,
    SPELL_COMMUNIQUE_CAMP_TO_RELAY              = 28281,
    SPELL_COMMUNIQUE_CAMP_TO_RELAY_DEATH        = 28351,
    SPELL_FIND_CAMP_TYPE                        = 28203,
    SPELL_PH_SUMMON_MINION_TRAP_GHOST_GHOUL     = 27883,
    SPELL_PH_SUMMON_MINION_TRAP_GHOST_SKELETON  = 28186,
    SPELL_PH_SUMMON_MINION_TRAP_GHOUL_SKELETON  = 28187,
    SPELL_ZAP_CRYSTAL                           = 28032,
    SPELL_MINION_SPAWN_IN                       = 28234,
    SPELL_SPIRIT_SPAWN_OUT                      = 17680,
    SPELL_MINION_DESPAWN_TIMER                  = 28090,
    SPELL_CONTROLLER_TIMER                      = 28095,
    SPELL_DESPAWNER_SELF                        = 28091,
    SPELL_SUMMON_SCOURGE_CONTROLLER             = 28092,
    SPELL_SCOURGE_STRIKE                        = 28265,
    SPELL_ENRAGE                                = 8599,
    SPELL_BONE_SHARDS                           = 17014,
    SPELL_INFECTED_BITE                         = 7367,
    SPELL_DEMORALIZING_SHOUT                    = 16244,
    SPELL_SUNDER_ARMOR                          = 21081,
    SPELL_SHADOW_WORD_PAIN                      = 589,
    SPELL_DUAL_WIELD                            = 674,
    SPELL_CREATE_LESSER_MARK_OF_THE_DAWN        = 28319,
    SPELL_CREATE_MARK_OF_THE_DAWN               = 28320,
    SPELL_CREATE_GREATER_MARK_OF_THE_DAWN       = 28321,
    SPELL_KNOCKDOWN                             = 16790,
    SPELL_TRAMPLE                               = 5568,
    SPELL_AURA_OF_FEAR                          = 28313,
    SPELL_RIBBON_OF_SOULS                       = 16243,
    SPELL_PSYCHIC_SCREAM                        = 22884,
    SPELL_MINION_DESPAWN_TIMER_UNCOMMON         = 28292,
    SPELL_ARCANE_BOLT                           = 13748,
    SPELL_CREATE_SUMMONER_SHIELD                = 28132,
    SPELL_BUTTRESS_CHANNEL                      = 28078,
    SPELL_BUTTRESS_TRAP                         = 28054,
    SPELL_KILL_SUMMONER_SUMMON_BOSS             = 28250,
    SPELL_PH_KILL_SUMMONER_BUFF                 = 27852,
    SPELL_KILL_SUMMONER_WHO_WILL_SUMMON_BOSS    = 27894,
    SPELL_QUIET_SUICIDE                         = 3617,
    SPELL_SUMMON_BOSS_BUFF                      = 31316,
    SPELL_SUMMON_BOSS                           = 31315,
    SPELL_ZAP_CRYSTAL_CORPSE                    = 28056,
    SPELL_SUMMON_CRACKED_NECROTIC_CRYSTAL       = 28424,
    SPELL_SUMMON_FAINT_NECROTIC_CRYSTAL         = 28699,
    SPELL_DAMAGE_VS_GUARDS                      = 28364,
    SPELL_FLAMESHOCKERS_TOUCH                   = 28314,
    SPELL_FLAMESHOCKERS_REVENGE                 = 28323,
    SPELL_FLAMESHOCKERS_TOUCH2                  = 28329,
    SPELL_FLAMESHOCKER_IMMOLATE_VISUAL          = 28330,
};

// NPCs
enum ScourgeInvasionNPC
{
    NPC_NECROTIC_SHARD                          = 16136,
    NPC_DAMAGED_NECROTIC_SHARD                  = 16172,
    NPC_CULTIST_ENGINEER                        = 16230,
    NPC_SHADOW_OF_DOOM                          = 16143,
    NPC_SCOURGE_INVASION_MINION_FINDER          = 16356,
    NPC_SCOURGE_INVASION_MINION_SPAWNER_GHOST_GHOUL    = 16306,
    NPC_SCOURGE_INVASION_MINION_SPAWNER_GHOST_SKELETON = 16336,
    NPC_SCOURGE_INVASION_MINION_SPAWNER_GHOUL_SKELETON = 16338,
    NPC_NECROPOLIS                              = 16401,
    NPC_NECROPOLIS_HEALTH                       = 16421,
    NPC_NECROPOLIS_PROXY                        = 16398,
    NPC_NECROPOLIS_RELAY                        = 16386,
    NPC_SKELETAL_SHOCKTROOPER                   = 16299,
    NPC_GHOUL_BERSERKER                         = 16141,
    NPC_SPECTRAL_SOLDIER                        = 16298,
    NPC_LUMBERING_HORROR                        = 14697,
    NPC_BONE_WITCH                              = 16380,
    NPC_SPIRIT_OF_THE_DAMNED                    = 16379,
    NPC_ARGENT_DAWN_INITIATE                    = 16384,
    NPC_ARGENT_DAWN_CLERIC                      = 16435,
    NPC_ARGENT_DAWN_PRIEST                      = 16436,
    NPC_ARGENT_DAWN_PALADIN                     = 16395,
    NPC_ARGENT_DAWN_CRUSADER                    = 16433,
    NPC_ARGENT_DAWN_CHAMPION                    = 16434,
    NPC_SKELETAL_TROOPER                        = 16438,
    NPC_SPECTRAL_SPIRIT                         = 16437,
    NPC_SKELETAL_SOLDIER                        = 16422,
    NPC_SPECTRAL_APPARITATION                   = 16423,
    NPC_CRACKED_NECROTIC_CRYSTAL                = 16431,
    NPC_FAINT_NECROTIC_CRYSTAL                  = 16531,
    NPC_FLAMESHOCKER                            = 16383,
    NPC_HIGHLORD_BOLVAR_FORDRAGON               = 1748,
    NPC_LADY_SYLVANAS_WINDRUNNER                = 10181,
    NPC_VARIMATHRAS                             = 2425,
    NPC_ROYAL_DREADGUARD                        = 13839,
    NPC_UNDERCITY_ELITE_GUARDIAN                = 16432,
    NPC_UNDERCITY_GUARDIAN                       = 5624,
    NPC_DEATHGUARD_ELITE                        = 7980,
    NPC_STORMWIND_CITY_GUARD                    = 68,
    NPC_STORMWIND_ELITE_GUARD                   = 16396,
    NPC_PALLID_HORROR                           = 16394,
    NPC_PATCHWORK_TERROR                        = 16382,
    NPC_HERALD_OF_THE_LICH_KING                 = 16995,
    NPC_ARGENT_EMISSARY                         = 16285,
};

// GameObjects
enum ScourgeInvasionObjects
{
    GO_BUTTRESS_TRAP                            = 181112,
    GO_SUMMON_MINION_TRAP_GHOST_GHOUL            = 181111,
    GO_SUMMON_MINION_TRAP_GHOST_SKELETON         = 181155,
    GO_SUMMON_MINION_TRAP_GHOUL_SKELETON         = 181156,
    GO_SUMMON_CIRCLE                             = 181136,
    GO_SUMMONER_SHIELD                           = 181142,
    GO_UNDEAD_FIRE                               = 181173,
    GO_UNDEAD_FIRE_AURA                          = 181174,
    GO_SKULLPILE_01                              = 181191,
    GO_SKULLPILE_02                              = 181192,
    GO_SKULLPILE_03                              = 181193,
    GO_SKULLPILE_04                              = 181194,
    GO_NECROPOLIS_TINY                           = 181154,
    GO_NECROPOLIS_SMALL                          = 181373,
    GO_NECROPOLIS_MEDIUM                         = 181374,
    GO_NECROPOLIS_BIG                            = 181215,
    GO_NECROPOLIS_HUGE                           = 181223,
    GO_NECROPOLIS_CITY                           = 181172,
};

enum ScourgeInvasionMisc
{
    ITEM_NECROTIC_RUNE                          = 22484,
    QUEST_UNDER_THE_SHADOW                      = 9153,
    QUEST_CRACKED_NECROTIC_CRYSTAL              = 9292,
    QUEST_FAINT_NECROTIC_CRYSTAL                = 9310,
    ACTION_FLAMESHOCKER_SCHEDULE_DESPAWN        = 1,
    MAIL_TEMPLATE_ARGENT_DAWN_NEEDS_YOUR_HELP   = 171,
    ITEM_A_LETTER_FROM_THE_KEEPER_OF_THE_ROLLS  = 22723,
};

enum ScourgeInvasionZoneIds
{
    AREA_WINTERSPRING       = 618,
    AREA_TANARIS            = 440,
    AREA_AZSHARA            = 16,
    AREA_BLASTED_LANDS      = 4,
    AREA_EASTERN_PLAGUELANDS = 139,
    AREA_BURNING_STEPPES    = 46,
    AREA_UNDERCITY          = 1497,
    AREA_STORMWIND          = 1519,
};

enum SIZoneIds
{
    SI_ZONE_AZSHARA,
    SI_ZONE_BLASTED_LANDS,
    SI_ZONE_BURNING_STEPPES,
    SI_ZONE_EASTERN_PLAGUELANDS,
    SI_ZONE_TANARIS,
    SI_ZONE_WINTERSPRING,
    SI_ZONE_STORMWIND,
    SI_ZONE_UNDERCITY
};

enum SITimers
{
    SI_TIMER_AZSHARA,
    SI_TIMER_BLASTED_LANDS,
    SI_TIMER_BURNING_STEPPES,
    SI_TIMER_EASTERN_PLAGUELANDS,
    SI_TIMER_TANARIS,
    SI_TIMER_WINTERSPRING,
    SI_TIMER_STORMWIND,
    SI_TIMER_UNDERCITY,
    SI_TIMER_MAX,
};

enum SIRemaining
{
    SI_REMAINING_AZSHARA,
    SI_REMAINING_BLASTED_LANDS,
    SI_REMAINING_BURNING_STEPPES,
    SI_REMAINING_EASTERN_PLAGUELANDS,
    SI_REMAINING_TANARIS,
    SI_REMAINING_WINTERSPRING,
    SI_REMAINING_MAX,
};

enum SIState : uint32
{
    SI_STATE_DISABLED,
    SI_STATE_ENABLED,
};

enum ScourgeInvasionTalk
{
    HERALD_OF_THE_LICH_KING_SAY_ATTACK_START    = 0,
    HERALD_OF_THE_LICH_KING_SAY_ATTACK_END      = 1,
    HERALD_OF_THE_LICH_KING_SAY_ATTACK_RANDOM   = 2,
    PALLID_HORROR_SAY_RANDOM_YELL               = 0,
    SYLVANAS_SAY_ATTACK_END                     = 3,
    VARIAN_SAY_ATTACK_END                       = 3,
};

enum ScourgeInvasionEvents
{
    EVENT_HERALD_OF_THE_LICH_KING_ZONE_START    = 7,
    EVENT_HERALD_OF_THE_LICH_KING_ZONE_STOP     = 8,
    EVENT_SHARD_MINION_SPAWNER_SMALL            = 1,
    EVENT_SHARD_MINION_SPAWNER_BUTTRESS         = 2,
    EVENT_SPAWNER_SUMMON_MINION                 = 3,
    EVENT_SHARD_FIND_DAMAGED_SHARD              = 4,
    EVENT_CULTIST_CHANNELING                    = 5,
    EVENT_HERALD_OF_THE_LICH_KING_YELL          = 6,
    EVENT_HERALD_OF_THE_LICH_KING_UPDATE        = 9,
    EVENT_DOOM_MINDFLAY                         = 20,
    EVENT_DOOM_FEAR                             = 21,
    EVENT_DOOM_START_ATTACK                     = 22,
    EVENT_RARE_KNOCKDOWN                        = 31,
    EVENT_RARE_TRAMPLE                          = 32,
    EVENT_RARE_RIBBON_OF_SOULS                  = 33,
    EVENT_MINION_ENRAGE                         = 40,
    EVENT_MINION_BONE_SHARDS                    = 41,
    EVENT_MINION_INFECTED_BITE                  = 42,
    EVENT_MINION_DAZED                          = 43,
    EVENT_MINION_DEMORALIZING_SHOUT             = 44,
    EVENT_MINION_SUNDER_ARMOR                   = 45,
    EVENT_MINION_ARCANE_BOLT                    = 46,
    EVENT_MINION_PSYCHIC_SCREAM                 = 47,
    EVENT_MINION_SCOURGE_STRIKE                 = 48,
    EVENT_MINION_SHADOW_WORD_PAIN               = 49,
    EVENT_MINION_FLAMESHOCKERS_TOUCH            = 50,
    EVENT_MINION_FLAMESHOCKERS_DESPAWN          = 51,
    EVENT_PALLID_RANDOM_YELL                    = 52,
    EVENT_PALLID_SPELL_DAMAGE_VS_GUARDS         = 53,
    EVENT_SYLVANAS_ANSWER_YELL                  = 54,
    EVENT_PALLID_RANDOM_SAY                     = 55,
    EVENT_PALLID_SUMMON_FLAMESHOCKER            = 56,
};

// Zone attack timer constants
enum SICityTimers
{
    ZONE_ATTACK_TIMER_MIN   = 45 * 60,
    ZONE_ATTACK_TIMER_MAX   = 60 * 60,
    CITY_ATTACK_TIMER_MIN   = 45 * 60,
    CITY_ATTACK_TIMER_MAX   = 60 * 60,
};

// World states
enum ScourgeInvasionWorldStates
{
    WORLD_STATE_SCOURGE_INVASION_VICTORIES              = 2219,
    WORLD_STATE_SCOURGE_INVASION_WINTERSPRING           = 2259,
    WORLD_STATE_SCOURGE_INVASION_AZSHARA                = 2260,
    WORLD_STATE_SCOURGE_INVASION_BLASTED_LANDS          = 2261,
    WORLD_STATE_SCOURGE_INVASION_BURNING_STEPPES        = 2262,
    WORLD_STATE_SCOURGE_INVASION_TANARIS                = 2263,
    WORLD_STATE_SCOURGE_INVASION_EASTERN_PLAGUELANDS    = 2264,
    WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_AZSHARA             = 2279,
    WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_BLASTED_LANDS       = 2280,
    WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_BURNING_STEPPES     = 2281,
    WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_EASTERN_PLAGUELANDS = 2282,
    WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_TANARIS             = 2283,
    WORLD_STATE_SCOURGE_INVASION_NECROPOLIS_WINTERSPRING        = 2284,
};

struct ScourgeInvasionZone
{
    uint32 map;
    uint32 zoneId;
    uint32 necropolisCount;
    uint32 remainingNecropoli;
};

struct ScourgeInvasionCityAttack
{
    uint32 map;
    uint32 zoneId;
};

#endif
