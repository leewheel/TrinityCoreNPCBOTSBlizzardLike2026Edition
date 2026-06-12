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

/* ScriptData
SDName: Zulfarrak
SD%Complete: 50
SDComment: Consider it temporary, no instance script made for this instance yet.
SDCategory: Zul'Farrak
EndScriptData */

/* ContentData
npc_sergeant_bly
npc_weegli_blastfuse
EndContentData */

#include "ScriptMgr.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "InstanceScript.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "zulfarrak.h"

/*######
## npc_sergeant_bly
######*/

enum blySays
{
    SAY_1 = 0,
    SAY_2 = 1
};

enum blySpells
{
    SPELL_BLYS_BAND_ESCAPE     = 11365,
    SPELL_SHIELD_BASH          = 11972,
    SPELL_REVENGE              = 12170,
};

enum blygossip
{
    GOSSIP_BLY_MID             = 941, //That's it!  I'm tired of helping you out.  It's time we settled things on the battlefield!
    GOSSIP_BLY_OID             = 1
};

class npc_sergeant_bly : public CreatureScript
{
public:
    npc_sergeant_bly() : CreatureScript("npc_sergeant_bly") { }

    struct npc_sergeant_blyAI : public ScriptedAI
    {
        npc_sergeant_blyAI(Creature* creature) : ScriptedAI(creature)
        {
            Initialize();
            instance = creature->GetInstanceScript();
            postGossipStep = 0;
            Text_Timer = 0;
        }

        void Initialize()
        {
            ShieldBash_Timer = 5000;
            Revenge_Timer = 8000;
            Porthome_Timer = 0;
            ableToPortHome = false;
        }

        InstanceScript* instance;

        uint32 postGossipStep;
        uint32 Text_Timer;
        uint32 ShieldBash_Timer;
        uint32 Revenge_Timer;
        uint32 Porthome_Timer;
        ObjectGuid PlayerGUID;
        bool ableToPortHome;

        void Reset() override
        {
            Initialize();

            me->SetFaction(FACTION_FRIENDLY);
        }

        void EnterEvadeMode(EvadeReason /*reason*/) override
        {
            if (ableToPortHome)
                return;

            if (instance->GetData(EVENT_PYRAMID) == PYRAMID_KILLED_ALL_TROLLS)
            {
                ableToPortHome = true;
                Porthome_Timer = 156000;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (postGossipStep>0 && postGossipStep<4)
            {
                if (Text_Timer<diff)
                {
                    switch (postGossipStep)
                    {
                        case 1:
                            //weegli doesn't fight - he goes & blows up the door
                            if (Creature* pWeegli = ObjectAccessor::GetCreature(*me, instance->GetGuidData(ENTRY_WEEGLI)))
                                pWeegli->AI()->DoAction(0);
                            Talk(SAY_1);
                            Text_Timer = 5000;
                            break;
                        case 2:
                            Talk(SAY_2);
                            Text_Timer = 5000;
                            break;
                        case 3:
                            me->SetFaction(FACTION_MONSTER);
                            if (Player* target = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                                AttackStart(target);

                            switchFactionIfAlive(ENTRY_RAVEN);
                            switchFactionIfAlive(ENTRY_ORO);
                            switchFactionIfAlive(ENTRY_MURTA);
                    }
                    postGossipStep++;
                }
                else Text_Timer -= diff;
            }

            // Home-port timer: after 2.5 min of being out of combat after pyramid is done, escape
            if (Porthome_Timer)
            {
                if (Porthome_Timer <= diff)
                {
                    Porthome_Timer = 0;
                    DoCastSelf(SPELL_BLYS_BAND_ESCAPE);
                    me->DespawnOrUnsummon(10s);

                    EscapeCrewMember(ENTRY_RAVEN);
                    EscapeCrewMember(ENTRY_ORO);
                    EscapeCrewMember(ENTRY_MURTA);
                    EscapeCrewMember(ENTRY_WEEGLI);
                }
                else
                    Porthome_Timer -= diff;
            }

            if (!UpdateVictim())
                return;

            if (ShieldBash_Timer <= diff)
            {
                DoCastVictim(SPELL_SHIELD_BASH);
                ShieldBash_Timer = 15000;
            }
            else
                ShieldBash_Timer -= diff;

            if (Revenge_Timer <= diff)
            {
                DoCastVictim(SPELL_REVENGE);
                Revenge_Timer = 10000;
            }
            else
                Revenge_Timer -= diff;

            DoMeleeAttackIfReady();
        }

        void DoAction(int32 /*param*/) override
        {
            postGossipStep=1;
            Text_Timer = 0;
        }

        void switchFactionIfAlive(uint32 entry)
        {
           if (Creature* crew = ObjectAccessor::GetCreature(*me, instance->GetGuidData(entry)))
               if (crew->IsAlive())
                   crew->SetFaction(FACTION_MONSTER);
        }

        void EscapeCrewMember(uint32 entry)
        {
            if (Creature* crew = ObjectAccessor::GetCreature(*me, instance->GetGuidData(entry)))
            {
                if (crew->IsAlive())
                {
                    crew->CastSpell(crew, SPELL_BLYS_BAND_ESCAPE);
                    crew->DespawnOrUnsummon(10s);
                }
            }
        }

        bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
        {
            uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
            ClearGossipMenuFor(player);
            if (action == GOSSIP_ACTION_INFO_DEF + 1)
            {
                CloseGossipMenuFor(player);
                PlayerGUID = player->GetGUID();
                DoAction(0);
            }
            return true;
        }

        bool OnGossipHello(Player* player) override
        {
            InitGossipMenuFor(player, GOSSIP_BLY_MID);
            if (instance->GetData(EVENT_PYRAMID) >= PYRAMID_MOVED_DOWNSTAIRS)
            {
                AddGossipItemFor(player, GOSSIP_BLY_MID, GOSSIP_BLY_OID, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                SendGossipMenuFor(player, 1517, me->GetGUID());
            }
            else
                if (instance->GetData(EVENT_PYRAMID) == PYRAMID_NOT_STARTED)
                    SendGossipMenuFor(player, 1515, me->GetGUID());
                else
                    SendGossipMenuFor(player, 1516, me->GetGUID());
            return true;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetZulFarrakAI<npc_sergeant_blyAI>(creature);
    }
};

/*######
+## go_troll_cage
+######*/

class go_troll_cage : public GameObjectScript
{
public:
    go_troll_cage() : GameObjectScript("go_troll_cage") { }

    struct go_troll_cageAI : public GameObjectAI
    {
        go_troll_cageAI(GameObject* go) : GameObjectAI(go), instance(go->GetInstanceScript()) { }

        InstanceScript* instance;

        bool OnGossipHello(Player* /*player*/) override
        {
            instance->SetData(EVENT_PYRAMID, PYRAMID_CAGES_OPEN);

            // Enable gossip on Bly and Weegli once cages open
            if (Creature* bly = ObjectAccessor::GetCreature(*me, instance->GetGuidData(ENTRY_BLY)))
                bly->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
            if (Creature* weegli = ObjectAccessor::GetCreature(*me, instance->GetGuidData(ENTRY_WEEGLI)))
                weegli->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);

            //set bly & co to aggressive & start moving to top of stairs
            initBlyCrewMember(ENTRY_BLY, 1884.99f, 1263, 41.52f);
            initBlyCrewMember(ENTRY_RAVEN, 1882.5f, 1263, 41.52f);
            initBlyCrewMember(ENTRY_ORO, 1886.47f, 1270.68f, 41.68f);
            initBlyCrewMember(ENTRY_WEEGLI, 1890, 1263, 41.52f);
            initBlyCrewMember(ENTRY_MURTA, 1891.19f, 1272.03f, 41.60f);
            return false;
        }

    private:
        void initBlyCrewMember(uint32 entry, float x, float y, float z)
        {
            if (Creature* crew = ObjectAccessor::GetCreature(*me, instance->GetGuidData(entry)))
            {
                crew->SetReactState(REACT_AGGRESSIVE);
                crew->SetWalk(true);
                crew->SetHomePosition(x, y, z, 0);
                crew->GetMotionMaster()->MovePoint(1, x, y, z);
                crew->SetFaction(FACTION_ESCORTEE_N_NEUTRAL_ACTIVE);
            }
        }
    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return GetZulFarrakAI<go_troll_cageAI>(go);
    }
};

/*######
## npc_weegli_blastfuse
######*/

enum weegliSpells
{
    SPELL_BOMB                  = 8858,
    SPELL_GOBLIN_LAND_MINE      = 21688,
    SPELL_SHOOT                 = 6660,
    SPELL_WEEGLIS_BARREL        = 10772
};

enum weegliSays
{
    SAY_WEEGLI_OHNO             = 0,
    SAY_WEEGLI_OK_I_GO          = 1
};

enum weegligossip
{
    GOSSIP_WEEGLI_MID             = 940, // Will you blow up that door now?
    GOSSIP_WEEGLI_OID             = 0
};

class npc_weegli_blastfuse : public CreatureScript
{
public:
    npc_weegli_blastfuse() : CreatureScript("npc_weegli_blastfuse") { }

    struct npc_weegli_blastfuseAI : public ScriptedAI
    {
        npc_weegli_blastfuseAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            destroyingDoor = false;
            Bomb_Timer = 10000;
            LandMine_Timer = 30000;
            outroTimer = 0;
            outroStage = 0;
        }

        uint32 Bomb_Timer;
        uint32 LandMine_Timer;
        uint32 outroTimer;
        uint8 outroStage;
        bool destroyingDoor;
        InstanceScript* instance;

        void Reset() override
        {
            /*instance->SetData(0, NOT_STARTED);*/
        }

        void AttackStart(Unit* victim) override
        {
            AttackStartCaster(victim, 10);//keep back & toss bombs/shoot
        }

        void JustDied(Unit* /*killer*/) override
        {
            /*instance->SetData(0, DONE);*/
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                // Barrel outro after destroying door
                if (destroyingDoor)
                {
                    if (outroTimer <= diff)
                    {
                        switch (outroStage)
                        {
                            case 0:
                                DoCastSelf(SPELL_WEEGLIS_BARREL);
                                outroTimer = 2000;
                                ++outroStage;
                                break;
                            case 1:
                                me->GetMotionMaster()->MovePoint(0, 1871.18f, 1100.0f, 8.88f);
                                Talk(SAY_WEEGLI_OK_I_GO);
                                me->DespawnOrUnsummon(8s);
                                instance->SetData(EVENT_PYRAMID, PYRAMID_GATES_DESTROYED);
                                destroyingDoor = false;
                                break;
                        }
                    }
                    else
                        outroTimer -= diff;
                }
                return;
            }

            if (Bomb_Timer < diff)
            {
                DoCastVictim(SPELL_BOMB);
                Bomb_Timer = 10000;
            }
            else
                Bomb_Timer -= diff;

            if (LandMine_Timer <= diff)
            {
                DoCastSelf(SPELL_GOBLIN_LAND_MINE);
                LandMine_Timer = 30000;
            }
            else
                LandMine_Timer -= diff;

            if (me->isAttackReady() && !me->IsWithinMeleeRange(me->GetVictim()))
            {
                DoCastVictim(SPELL_SHOOT);
                me->SetSheath(SHEATH_STATE_RANGED);
            }
            else
            {
                me->SetSheath(SHEATH_STATE_MELEE);
                DoMeleeAttackIfReady();
            }
        }

        void MovementInform(uint32 /*type*/, uint32 /*id*/) override
        {
            if (instance->GetData(EVENT_PYRAMID) == PYRAMID_CAGES_OPEN)
            {
                instance->SetData(EVENT_PYRAMID, PYRAMID_ARRIVED_AT_STAIR);
                Talk(SAY_WEEGLI_OHNO);
                me->SetHomePosition(1882.69f, 1272.28f, 41.87f, 0);
            }
            else if (instance->GetData(EVENT_PYRAMID) >= PYRAMID_KILLED_ALL_TROLLS && instance->GetData(EVENT_PYRAMID) < PYRAMID_DESTROY_GATES)
            {
                instance->SetData(EVENT_PYRAMID, PYRAMID_MOVED_DOWNSTAIRS);
            }
            else if (instance->GetData(EVENT_PYRAMID) == PYRAMID_DESTROY_GATES)
            {
                destroyingDoor = true;
            }
        }

        void DestroyDoor()
        {
            if (me->IsAlive())
            {
                me->SetFaction(FACTION_FRIENDLY);
                me->GetMotionMaster()->MovePoint(0, 1858.57f, 1146.35f, 14.745f);
                me->SetHomePosition(1858.57f, 1146.35f, 14.745f, 3.85f); // in case he gets interrupted
                Talk(SAY_WEEGLI_OK_I_GO);
                instance->SetData(EVENT_PYRAMID, PYRAMID_DESTROY_GATES);
            }
        }

        bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
        {
            uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
            ClearGossipMenuFor(player);
            if (action == GOSSIP_ACTION_INFO_DEF + 1)
            {
                CloseGossipMenuFor(player);
                //here we make him run to door, set the charge and run away off to nowhere
                DestroyDoor();
            }
            return true;
        }

        bool OnGossipHello(Player* player) override
        {
            InitGossipMenuFor(player, GOSSIP_WEEGLI_MID);
            switch (instance->GetData(EVENT_PYRAMID))
            {
                case PYRAMID_MOVED_DOWNSTAIRS:
                case PYRAMID_KILLED_ALL_TROLLS:
                    AddGossipItemFor(player, GOSSIP_WEEGLI_MID, GOSSIP_WEEGLI_OID, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                    SendGossipMenuFor(player, 1514, me->GetGUID());  //if event can proceed to end
                    break;
                case PYRAMID_NOT_STARTED:
                    SendGossipMenuFor(player, 1511, me->GetGUID());  //if event not started
                    break;
                default:
                    SendGossipMenuFor(player, 1513, me->GetGUID());  //if event are in progress
            }
            return true;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetZulFarrakAI<npc_weegli_blastfuseAI>(creature);
    }
};

/*######
## boss_shadowpriest_sezziz
######*/

enum SezzizSpells
{
    SPELL_SHADOW_BOLT           = 15537,
    SPELL_PSYCHIC_SCREAM        = 13704,
    SPELL_RENEW                 = 8362,
    SPELL_HEAL                  = 12039
};

enum SezzizEvents
{
    EVENT_SHADOW_BOLT           = 1,
    EVENT_PSYCHIC_SCREAM,
    EVENT_SUMMON_ADDS,
    EVENT_RENEW,
    EVENT_HEAL
};

// 4 waves pattern, matching Acore
static Position const SezzizAddWaves[4][4] =
{
    { { 1874.12f, 1198.90f, 8.87f, 0.0f }, { 1874.12f, 1198.90f, 8.87f, 0.0f }, { 0,0,0,0 }, { 0,0,0,0 } },            // wave 0: Zealot + Acolyte same spot
    { { 895.26f,  1199.09f, 8.87f, 0.0f }, { 895.26f,  1199.09f, 8.87f, 0.0f }, { 0,0,0,0 }, { 0,0,0,0 } },            // wave 1: Acolyte + Acolyte same spot
    { { 1874.12f, 1198.90f, 8.87f, 0.0f }, { 895.26f,  1199.09f, 8.87f, 0.0f }, { 895.26f, 1199.09f, 8.87f, 0.0f }, { 0,0,0,0 } }, // wave 2: Zealot + 2 Acolytes
    { { 895.26f,  1199.09f, 8.87f, 0.0f }, { 1874.12f, 1198.90f, 8.87f, 0.0f }, { 1874.12f, 1198.90f, 8.87f, 0.0f }, { 895.26f, 1199.09f, 8.87f, 0.0f } } // wave 3: 2 Zealots + 2 Acolytes
};

static uint32 const SezzizAddEntries[4][2] =
{
    { ENTRY_SANDFURY_ZEALOT,  ENTRY_SANDFURY_ACOLYTE  },                     // wave 0
    { ENTRY_SANDFURY_ACOLYTE,  ENTRY_SANDFURY_ACOLYTE  },                     // wave 1
    { ENTRY_SANDFURY_ZEALOT,   ENTRY_SANDFURY_ACOLYTE  },                     // wave 2 (Acolyte at index 2 too)
    { ENTRY_SANDFURY_ZEALOT,   ENTRY_SANDFURY_ZEALOT   }                      // wave 3 (then Acolytes at 2,3)
};

static uint32 const SezzizAddCount[4] = { 2, 2, 3, 4 };

class boss_shadowpriest_sezziz : public CreatureScript
{
public:
    boss_shadowpriest_sezziz() : CreatureScript("boss_shadowpriest_sezziz") { }

    struct boss_shadowpriest_sezzizAI : public ScriptedAI
    {
        boss_shadowpriest_sezzizAI(Creature* creature) : ScriptedAI(creature), _summons(me) { }

        void Reset() override
        {
            _events.Reset();
            _summonWave = 0;
            _summons.DespawnAll();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(EVENT_SHADOW_BOLT, 0ms, 1s);
            _events.ScheduleEvent(EVENT_PSYCHIC_SCREAM, 1s, 16s);
            _events.ScheduleEvent(EVENT_SUMMON_ADDS, 12s);
            _events.ScheduleEvent(EVENT_HEAL, 7s, 10s);
            _events.ScheduleEvent(EVENT_RENEW, 10s, 15s);
        }

        void JustSummoned(Creature* summon) override
        {
            _summons.Summon(summon);
            if (Unit* victim = me->GetVictim())
                summon->AI()->AttackStart(victim);
        }

        void AttackStart(Unit* victim) override
        {
            AttackStartCaster(victim, 40.0f);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SHADOW_BOLT:
                        DoCastVictim(SPELL_SHADOW_BOLT);
                        _events.ScheduleEvent(EVENT_SHADOW_BOLT, 3s, 4s);
                        break;
                    case EVENT_PSYCHIC_SCREAM:
                        DoCastVictim(SPELL_PSYCHIC_SCREAM);
                        _events.ScheduleEvent(EVENT_PSYCHIC_SCREAM, 22s, 30s);
                        break;
                    case EVENT_SUMMON_ADDS:
                    {
                        uint32 wave = _summonWave;
                        _summonWave = (_summonWave + 1) % 4;
                        uint32 count = SezzizAddCount[wave];
                        // wave 2 (index 2) is special: 1 Zealot + 2 Acolytes
                        if (wave == 2)
                        {
                            me->SummonCreature(ENTRY_SANDFURY_ZEALOT, SezzizAddWaves[wave][0], TEMPSUMMON_DEAD_DESPAWN, 10s);
                            me->SummonCreature(ENTRY_SANDFURY_ACOLYTE, SezzizAddWaves[wave][1], TEMPSUMMON_DEAD_DESPAWN, 10s);
                            me->SummonCreature(ENTRY_SANDFURY_ACOLYTE, SezzizAddWaves[wave][2], TEMPSUMMON_DEAD_DESPAWN, 10s);
                        }
                        // wave 3 (index 3): 2 Zealots + 2 Acolytes
                        else if (wave == 3)
                        {
                            me->SummonCreature(ENTRY_SANDFURY_ZEALOT, SezzizAddWaves[wave][0], TEMPSUMMON_DEAD_DESPAWN, 10s);
                            me->SummonCreature(ENTRY_SANDFURY_ZEALOT, SezzizAddWaves[wave][1], TEMPSUMMON_DEAD_DESPAWN, 10s);
                            me->SummonCreature(ENTRY_SANDFURY_ACOLYTE, SezzizAddWaves[wave][2], TEMPSUMMON_DEAD_DESPAWN, 10s);
                            me->SummonCreature(ENTRY_SANDFURY_ACOLYTE, SezzizAddWaves[wave][3], TEMPSUMMON_DEAD_DESPAWN, 10s);
                        }
                        else
                        {
                            me->SummonCreature(SezzizAddEntries[wave][0], SezzizAddWaves[wave][0], TEMPSUMMON_DEAD_DESPAWN, 10s);
                            me->SummonCreature(SezzizAddEntries[wave][1], SezzizAddWaves[wave][1], TEMPSUMMON_DEAD_DESPAWN, 10s);
                        }
                        _events.ScheduleEvent(EVENT_SUMMON_ADDS, 10s, 14s);
                        break;
                    }
                    case EVENT_HEAL:
                    {
                        Unit* target = DoSelectLowestHpFriendly(40.0f, 1500);
                        if (target)
                            DoCast(target, SPELL_HEAL);
                        _events.ScheduleEvent(EVENT_HEAL, 7s, 10s);
                        break;
                    }
                    case EVENT_RENEW:
                    {
                        Unit* target = DoSelectLowestHpFriendly(40.0f, 700);
                        if (target)
                            DoCast(target, SPELL_RENEW);
                        _events.ScheduleEvent(EVENT_RENEW, 10s, 15s);
                        break;
                    }
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        SummonList _summons;
        uint8 _summonWave;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetZulFarrakAI<boss_shadowpriest_sezzizAI>(creature);
    }
};

/*######
## go_shallow_grave
######*/

enum ShallowGrave
{
    NPC_ZOMBIE          = 7286,
    NPC_DEAD_HERO       = 7276,
    CHANCE_ZOMBIE       = 65,
    CHANCE_DEAD_HERO    = 10
};

class go_shallow_grave : public GameObjectScript
{
public:
    go_shallow_grave() : GameObjectScript("go_shallow_grave") { }

    struct go_shallow_graveAI : public GameObjectAI
    {
        go_shallow_graveAI(GameObject* go) : GameObjectAI(go) { }

        bool OnGossipHello(Player* /*player*/) override
        {
            // randomly summon a zombie or dead hero the first time a grave is used
            if (me->GetUseCount() == 0)
            {
                uint32 randomchance = urand(0, 100);
                if (randomchance < CHANCE_ZOMBIE)
                    me->SummonCreature(NPC_ZOMBIE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30s);
                else
                    if ((randomchance - CHANCE_ZOMBIE) < CHANCE_DEAD_HERO)
                        me->SummonCreature(NPC_DEAD_HERO, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30s);
            }
            me->AddUse();
            return false;
        }
    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return GetZulFarrakAI<go_shallow_graveAI>(go);
    }
};

/*######
## at_zumrah
######*/

enum zumrahConsts
{
    ZUMRAH_ID = 7271
};

class at_zumrah : public AreaTriggerScript
{
public:
    at_zumrah() : AreaTriggerScript("at_zumrah") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*at*/) override
    {
        Creature* pZumrah = player->FindNearestCreature(ZUMRAH_ID, 30.0f);

        if (!pZumrah)
            return false;

        pZumrah->SetFaction(FACTION_TROLL_FROSTMANE);
        return true;
    }

};

void AddSC_zulfarrak()
{
    new npc_sergeant_bly();
    new npc_weegli_blastfuse();
    new boss_shadowpriest_sezziz();
    new go_shallow_grave();
    new at_zumrah();
    new go_troll_cage();
}
