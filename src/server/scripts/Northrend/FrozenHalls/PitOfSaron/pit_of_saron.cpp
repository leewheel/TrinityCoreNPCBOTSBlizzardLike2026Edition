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

#include "Creature.h"
#include "CreatureGroups.h"
#include "CreatureTextMgr.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "PassiveAI.h"
#include "pit_of_saron.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Vehicle.h"
#include <limits>

// ---- Enumerations for event texts ----

enum eIntroTexts
{
    SAY_TYRANNUS_INTRO_1                        = 4,
    SAY_JAINA_INTRO_1                           = 2,
    SAY_SYLVANAS_INTRO_1                        = 3,
    SAY_TYRANNUS_INTRO_2                        = 5,
    SAY_TYRANNUS_INTRO_3                        = 6,
    SAY_JAINA_INTRO_2                           = 6,
    SAY_SYLVANAS_INTRO_2                        = 7,
    SAY_TYRANNUS_INTRO_4                        = 7,
    SAY_JAINA_INTRO_3                           = 9,
    SAY_JAINA_INTRO_4                           = 10,
    SAY_SYLVANAS_INTRO_3                        = 11,
    SAY_JAINA_INTRO_5                           = 12,
    SAY_SYLVANAS_INTRO_4                        = 13,
};

enum eFBSTexts
{
    SAY_GENERAL_GARFROST                        = 21,
    SAY_TYRANNUS_GARFROST                       = 0,
};

enum eSBSTexts
{
    SAY_OUTRO_KRICK_1                           = 6,
    SAY_JAINA_KRICK_1                           = 0,
    SAY_SYLVANAS_KRICK_1                        = 0,
    SAY_OUTRO_KRICK_2                           = 7,
    SAY_JAINA_KRICK_2                           = 1,
    SAY_SYLVANAS_KRICK_2                        = 1,
    SAY_OUTRO_KRICK_3                           = 8,
    SAY_TYRANNUS_KRICK_1                        = 0,
    SAY_OUTRO_KRICK_4                           = 9,
    SAY_TYRANNUS_KRICK_2                        = 1,
    SAY_JAINA_KRICK_3                           = 45,
    SAY_SYLVANAS_KRICK_3                        = 2,
};

enum ePTSTexts
{
    SAY_TYRANNUS_AMBUSH_1                       = 2,
    SAY_TYRANNUS_AMBUSH_2                       = 3,
    SAY_TYRANNUS_TRAP_TUNNEL                    = 1,
};

enum eTSTexts
{
    SAY_BOSS_TYRANNUS_INTRO_1                   = 0,
    SAY_GENERAL_HORDE_TRASH                     = 51,
    SAY_BOSS_TYRANNUS_INTRO_2                   = 1,
    SAY_GENERAL_HORDE_OUTRO_1                   = 61,
    SAY_GENERAL_OUTRO_2                         = 62,
    SAY_JAINA_OUTRO_1                           = 63,
    SAY_SYLVANAS_OUTRO_1                        = 64,
    SAY_JAINA_OUTRO_2                           = 65,
    SAY_JAINA_OUTRO_3                           = 66,
    SAY_SYLVANAS_OUTRO_2                        = 67,
    SAY_GENERAL_ALLIANCE_OUTRO_1                = 68,
    SAY_GENERAL_ALLIANCE_TRASH                  = 69,
};

// ---- Position constants ----

Position const PortalPos = {424.46f, 212.16f, 528.8f, 0.0f};
Position const LeaderIntroPos = {440.788f, 213.76f, 528.711f, 0.0f};
Position const NecrolytePos1 = {506.304f, 211.78f, 528.71f, M_PI};
Position const NecrolytePos2 = {506.127f, 231.46f, 528.71f, M_PI};
Position const FBSSpawnPos = {695.685f, -118.825f, 513.877f, 3 * M_PI / 2};
Position const KrickCenterPos = {836.65f, 115.08f, 510.0f, 0.0f};
Position const SBSTyrannusStartPos = {781.127f, 265.825f, 552.31f, 0.0f};
Position const SBSLeaderStartPos = {772.716f, 111.517f, 510.81f, 0.0f};
Position const SBSLeaderEndPos = {823.2f, -4.4497f, 509.49f, 0.86f};
Position const PTSTyrannusWaitPos1 = {923.45f, 82.65f, 582.44f, 3.59f};
Position const PTSTyrannusWaitPos2 = {907.27f, -53.86f, 617.31f, 1.69f};
Position const PTSTyrannusWaitPos3 = {1117.93f, -125.16f, 760.34f, 0.10f};
Position const TSSpawnPos = {1069.49f, 88.99f, 631.5f, 2.0f};
Position const TSMidPos = {1051.475f, 126.56f, 628.157f, 2.02f};
float const TSHeight = 628.157f;
Position const TSLeaderSpawnPos = {1064.3761f, 99.10f, 631.0f, 2.1f};
Position const TSCenterPos = {990.48f, 165.37f, 628.157f, 5.7f};
Position const TSDistCheckPos = {1009.29f, 163.15f, 628.157f, 0.0f};
Position const TSSindragosaPos1 = {919.10f, 249.83f, 556.34f, 5.49f};
Position const TSSindragosaPos2 = {948.39f, 215.47f, 653.71f, 5.51f};
Position const EventLeaderPos2 = {1054.368f, 107.14620f, 628.4467f, 0.0f};
Position const SlaveLeaderPos  = {689.7158f, -104.8736f, 513.7360f, 0.0f};

// ---- Data arrays for events ----

ChampionPosition const introPositions[] =
{
    { { NPC_CHAMPION_1_ALLIANCE, NPC_CHAMPION_1_HORDE }, { 452.884f, 209.141f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_1_ALLIANCE, NPC_CHAMPION_1_HORDE }, { 450.541f, 212.28f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_1_ALLIANCE, NPC_CHAMPION_1_HORDE }, { 449.835f, 206.68f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_2_ALLIANCE, NPC_CHAMPION_2_HORDE }, { 446.542f, 209.986f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_2_ALLIANCE, NPC_CHAMPION_2_HORDE }, { 447.29f, 213.916f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_2_ALLIANCE, NPC_CHAMPION_2_HORDE }, { 445.794f, 206.057f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_1_ALLIANCE, NPC_CHAMPION_1_HORDE }, { 446.74f, 228.577f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_1_ALLIANCE, NPC_CHAMPION_1_HORDE }, { 449.19f, 226.21f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_1_ALLIANCE, NPC_CHAMPION_1_HORDE }, { 447.352f, 222.754f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_1_ALLIANCE, NPC_CHAMPION_1_HORDE }, { 443.346f, 192.343f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_1_ALLIANCE, NPC_CHAMPION_1_HORDE }, { 446.293f, 195.047f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_1_ALLIANCE, NPC_CHAMPION_1_HORDE }, { 444.035f, 197.67f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_2_ALLIANCE, NPC_CHAMPION_3_HORDE }, { 442.69f, 223.525f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_2_ALLIANCE, NPC_CHAMPION_3_HORDE }, { 442.967f, 219.535f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_2_ALLIANCE, NPC_CHAMPION_3_HORDE }, { 442.526f, 199.361f, 528.84f, 0.0f } },
    { { NPC_CHAMPION_2_ALLIANCE, NPC_CHAMPION_3_HORDE }, { 442.843f, 203.193f, 528.84f, 0.0f } },
    { { NPC_LORALEN, NPC_KALIRA }, { 438.505f, 211.54f, 528.71f, 0.0f } },
    { { NPC_KALIRA, NPC_LORALEN }, { 438.946f, 215.427f, 528.71f, 0.0f } },
    { { 0, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f } }
};

FBSPosition const FBSData[] =
{
    { NPC_HORDE_SLAVE_1, PATH_BEGIN_VALUE + 0 },
    { NPC_HORDE_SLAVE_1, PATH_BEGIN_VALUE + 1 },
    { NPC_HORDE_SLAVE_2, PATH_BEGIN_VALUE + 2 },
    { NPC_HORDE_SLAVE_2, PATH_BEGIN_VALUE + 3 },
    { NPC_HORDE_SLAVE_2, PATH_BEGIN_VALUE + 4 },
    { NPC_HORDE_SLAVE_3, PATH_BEGIN_VALUE + 5 },
    { NPC_HORDE_SLAVE_3, PATH_BEGIN_VALUE + 6 },
    { NPC_HORDE_SLAVE_4, PATH_BEGIN_VALUE + 7 },
    { NPC_HORDE_SLAVE_4, PATH_BEGIN_VALUE + 8 },
    { 0, 0 }
};

TSPosition const TSData[] =
{
    { NPC_FREED_SLAVE_3_HORDE, 1047.8f, 126.01f },
    { NPC_FREED_SLAVE_3_HORDE, 1049.21f, 127.10f },
    { NPC_FREED_SLAVE_3_HORDE, 1051.68f, 129.02f },
    { NPC_FREED_SLAVE_3_HORDE, 1053.24f, 130.23f },
    { NPC_FREED_SLAVE_1_HORDE, 1044.82f, 121.30f },
    { NPC_FREED_SLAVE_1_HORDE, 1049.33f, 124.01f },
    { NPC_FREED_SLAVE_1_HORDE, 1056.79f, 130.86f },
    { NPC_FREED_SLAVE_2_HORDE, 1045.56f, 118.46f },
    { NPC_FREED_SLAVE_2_HORDE, 1047.75f, 120.85f },
    { NPC_FREED_SLAVE_2_HORDE, 1052.93f, 124.156f },
    { NPC_FREED_SLAVE_2_HORDE, 1057.35f, 127.95f },
    { NPC_FREED_SLAVE_2_HORDE, 1059.18f, 129.86f },
    { NPC_FREED_SLAVE_2_HORDE, 1049.865f, 118.735f },
    { NPC_FREED_SLAVE_2_HORDE, 1052.32f, 121.827f },
    { NPC_FREED_SLAVE_2_HORDE, 1055.38f, 123.99f },
    { NPC_FREED_SLAVE_2_HORDE, 1058.723f, 125.98f },
    { 0, 0.0f, 0.0f }
};

// ---- TC existing: Icicle summons for cavern achievement ----

bool ScheduledIcicleSummons::Execute(uint64 /*time*/, uint32 /*diff*/)
{
    if (roll_chance_i(12))
    {
        _trigger->CastSpell(_trigger, SPELL_TUNNEL_ICICLE, true);
        _trigger->m_Events.AddEvent(new ScheduledIcicleSummons(_trigger), _trigger->m_Events.CalculateTime(randtime(20s, 35s)));
    }
    else
        _trigger->m_Events.AddEvent(new ScheduledIcicleSummons(_trigger), _trigger->m_Events.CalculateTime(randtime(1s, 20s)));

    return true;
}

// ---- Intro: Jaina/Sylvanas leader escort ----

class npc_pos_leader : public CreatureScript
{
public:
    npc_pos_leader() : CreatureScript("npc_pos_leader") { }

    struct npc_pos_leaderAI : public NullCreatureAI
    {
        npc_pos_leaderAI(Creature* creature) : NullCreatureAI(creature), summons(me)
        {
            pInstance = me->GetInstanceScript();
        }

        EventMap events;
        SummonList summons;
        InstanceScript* pInstance;
        uint8 counter;

        void Reset() override
        {
            counter = 0;
            events.Reset();
            summons.DespawnAll();
            me->SetVisible(true);

            if (pInstance)
                if (pInstance->GetData(DATA_INSTANCE_PROGRESS) == INSTANCE_PROGRESS_NONE)
                    me->SetVisible(false);
        }

        void SetData(uint32 type, uint32 /*val*/) override
        {
            if (type == DATA_START_INTRO && pInstance && pInstance->GetData(DATA_INSTANCE_PROGRESS) == INSTANCE_PROGRESS_NONE && counter == 0 && !me->IsVisible())
            {
                me->setActive(true);
                events.ScheduleEvent(1, 0ms);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case 0:
                    break;
                case 1:
                {
                    if (counter == 0)
                    {
                        me->SetVisible(true);
                        me->GetMotionMaster()->MovePoint(1, LeaderIntroPos);
                    }

                    uint8 idx = 0;
                    if (pInstance)
                        idx = (pInstance->GetData(DATA_TEAMID_IN_INSTANCE) == TEAM_ALLIANCE ? 0 : 1);
                    if (introPositions[counter].entry[idx] != 0)
                    {
                        if (Creature* summon = me->SummonCreature(introPositions[counter].entry[idx], PortalPos))
                        {
                            summon->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                            summon->SetSpeedRate(MOVE_RUN, 0.8f);
                            summon->GetMotionMaster()->MovePoint(1, introPositions[counter].endPosition);
                            summon->SetEmoteState(Emote(EMOTE_STATE_READY1H));
                        }

                        ++counter;
                        events.Repeat(Milliseconds(150));
                    }
                    else
                    {
                        events.ScheduleEvent(2, 2500ms);
                    }
                }
                break;
                case 2:
                    if (pInstance)
                        if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_TYRANNUS_EVENT_GUID)))
                        {
                            c->setActive(true);
                            c->AI()->Talk(SAY_TYRANNUS_INTRO_1);
                        }
                    events.ScheduleEvent(3, 7s);
                    break;
                case 3:
                    if (pInstance)
                        if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_TYRANNUS_EVENT_GUID)))
                            c->AI()->Talk(SAY_TYRANNUS_INTRO_2);
                    events.ScheduleEvent(4, 14s);
                    break;
                case 4:
                    if (pInstance)
                    {
                        Creature* n1 = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_NECROLYTE_1_GUID));
                        Creature* n2 = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_NECROLYTE_2_GUID));
                        if (n1 && n2)
                        {
                            if (!n1->IsInCombat() && n1->IsAlive())
                            {
                                n1->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                                n1->GetMotionMaster()->MovePoint(1, NecrolytePos1);
                                n1->SetEmoteState(Emote(EMOTE_STATE_READY1H));
                            }
                            if (!n2->IsInCombat() && n2->IsAlive())
                            {
                                n2->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                                n2->GetMotionMaster()->MovePoint(1, NecrolytePos2);
                                n2->SetEmoteState(Emote(EMOTE_STATE_READY1H));
                            }
                            n1->RemoveAura(SPELL_NECROLYTE_CHANNELING);
                            n2->RemoveAura(SPELL_NECROLYTE_CHANNELING);

                            for (SummonList::const_iterator itr = summons.begin(); itr != summons.end(); ++itr)
                                if (Creature* c = pInstance->instance->GetCreature(*itr))
                                {
                                    if (c->GetPositionX() < 440.0f)
                                        continue;
                                    if (c->GetPositionY() > 215.0f)
                                        c->GetMotionMaster()->MoveChase(n2, 0.0f, rand_norm() * 2 * M_PI);
                                    else
                                        c->GetMotionMaster()->MoveChase(n1, 0.0f, rand_norm() * 2 * M_PI);
                                }
                        }
                    }
                    events.ScheduleEvent(5, 1ms);
                    break;
                case 5:
                    Talk(me->GetEntry() == NPC_JAINA_PART1 ? SAY_JAINA_INTRO_1 : SAY_SYLVANAS_INTRO_1);
                    events.ScheduleEvent(6, 1s);
                    break;
                case 6:
                    if (pInstance)
                        if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_TYRANNUS_EVENT_GUID)))
                            c->AI()->Talk(SAY_TYRANNUS_INTRO_3);
                    events.ScheduleEvent(7, 5s);
                    break;
                case 7:
                    if (pInstance)
                    {
                        if (Creature* n1 = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_NECROLYTE_1_GUID)))
                            n1->AI()->DoAction(1337);
                        if (Creature* n2 = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_NECROLYTE_2_GUID)))
                            n2->AI()->DoAction(1337);

                        for (SummonList::const_iterator itr = summons.begin(); itr != summons.end(); ++itr)
                            if (Creature* c = pInstance->instance->GetCreature(*itr))
                            {
                                if (c->GetPositionX() < 450.0f)
                                    continue;
                                c->GetMotionMaster()->Clear();
                                c->GetMotionMaster()->MoveIdle();
                                c->StopMoving();
                                c->CastSpell(c, 69413, true);
                                c->SetCanFly(true);
                                c->SetDisableGravity(true);
                                c->UpdateMovementFlags();
                                float dist = rand_norm() * 2.0f;
                                float angle = rand_norm() * 2 * M_PI;
                                c->GetMotionMaster()->MoveTakeoff(0, Position(c->GetPositionX() + dist * cos(angle), c->GetPositionY() + dist * std::sin(angle), c->GetPositionZ() + 6.0f + (float)urand(0, 4)));
                            }
                    }
                    events.ScheduleEvent(8, 7s);
                    break;
                case 8:
                    if (pInstance)
                        if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_TYRANNUS_EVENT_GUID)))
                            c->CastSpell(c, 69753, false);
                    events.ScheduleEvent(9, 400ms);
                    break;
                case 9:
                    if (pInstance)
                        for (SummonList::const_iterator itr = summons.begin(); itr != summons.end(); ++itr)
                            if (Creature* c = pInstance->instance->GetCreature(*itr))
                            {
                                if (c->GetPositionX() < 450.0f)
                                    continue;
                                Unit::Kill(c, c);
                                c->RemoveAllAuras();
                            }
                    events.ScheduleEvent(10, 1s);
                    break;
                case 10:
                    Talk(me->GetEntry() == NPC_JAINA_PART1 ? SAY_JAINA_INTRO_2 : SAY_SYLVANAS_INTRO_2);
                    events.ScheduleEvent(11, 1s);
                    break;
                case 11:
                    if (pInstance)
                        for (SummonList::const_iterator itr = summons.begin(); itr != summons.end(); ++itr)
                            if (Creature* c = pInstance->instance->GetCreature(*itr))
                            {
                                if (c->GetPositionX() < 450.0f)
                                    continue;
                                c->SetCanFly(false);
                                c->SetDisableGravity(false);
                                c->UpdateMovementFlags();
                                c->CastSpell(c, 69350, true);
                            }
                    events.ScheduleEvent(12, 2s);
                    break;
                case 12:
                    if (pInstance)
                    {
                        if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_TYRANNUS_EVENT_GUID)))
                            c->AI()->Talk(SAY_TYRANNUS_INTRO_4);

                        for (SummonList::const_iterator itr = summons.begin(); itr != summons.end(); ++itr)
                            if (Creature* c = pInstance->instance->GetCreature(*itr))
                            {
                                if (c->GetPositionX() < 450.0f)
                                    continue;
                                c->SetHomePosition(c->GetPositionX(), c->GetPositionY(), c->GetPositionZ(), c->GetOrientation());
                                c->Respawn(true);
                                c->UpdateEntry(36796, 0, false);
                                c->SetFacingTo(M_PI);
                                c->SetReactState(REACT_PASSIVE);
                            }
                    }
                    events.ScheduleEvent(13, 3s);
                    break;
                case 13:
                    if (pInstance)
                    {
                        for (SummonList::const_iterator itr = summons.begin(); itr != summons.end(); ++itr)
                            if (Creature* c = pInstance->instance->GetCreature(*itr))
                            {
                                if (c->GetPositionX() < 450.0f)
                                    continue;
                                c->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                                float dist = rand_norm();
                                float angle = rand_norm() * 2 * M_PI;
                                c->SetSpeedRate(MOVE_RUN, 0.8f);
                                c->SetCombatPulseDelay(5);
                                c->GetMotionMaster()->MoveChase(me, dist, angle);
                                c->SetHomePosition(me->GetPositionX() + dist * cos(angle), me->GetPositionY() + dist * std::sin(angle), me->GetPositionZ(), 0.0f);
                            }
                    }
                    events.ScheduleEvent(14, 2s);
                    break;
                case 14:
                    if (pInstance)
                    {
                        if (me->GetEntry() == NPC_JAINA_PART1)
                        {
                            Talk(SAY_JAINA_INTRO_3);
                            me->CastSpell(me, 70132, false);
                        }
                        else
                        {
                            me->CastSpell(me, 59514, false);
                            for (uint8 i = 0; i < 2; ++i)
                                if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_GUARD_1_GUID + i)))
                                    c->AI()->DoAction(1337);
                        }
                        pInstance->SetData(DATA_INSTANCE_PROGRESS, INSTANCE_PROGRESS_FINISHED_INTRO);
                    }
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetPitOfSaronAI<npc_pos_leaderAI>(creature);
    }
};

// ---- Necrolyte channelers ----

class npc_pos_deathwhisper_necrolyte : public CreatureScript
{
public:
    npc_pos_deathwhisper_necrolyte() : CreatureScript("npc_pos_deathwhisper_necrolyte") { }

    struct npc_pos_deathwhisper_necrolyteAI : public ScriptedAI
    {
        npc_pos_deathwhisper_necrolyteAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
        }

        InstanceScript* pInstance;
        bool invincible;

        void Reset() override
        {
            invincible = true;
            me->SetImmuneToPC(true);
            DoCastSelf(SPELL_NECROLYTE_CHANNELING);

            if (pInstance)
            {
                if (!pInstance->GetGuidData(DATA_NECROLYTE_1_GUID))
                    pInstance->SetGuidData(DATA_NECROLYTE_1_GUID, me->GetGUID());
                else if (!pInstance->GetGuidData(DATA_NECROLYTE_2_GUID))
                    pInstance->SetGuidData(DATA_NECROLYTE_2_GUID, me->GetGUID());
            }
        }

        void DoAction(int32 param) override
        {
            if (param == 1337)
            {
                invincible = false;
                me->SetImmuneToPC(false);
            }
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(1, 3s);
            _events.ScheduleEvent(2, 5s, 8s);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (_events.ExecuteEvent())
            {
                case 1:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0))
                        DoCast(target, 69582);
                    _events.Repeat(Milliseconds(3000));
                    break;
                case 2:
                    DoCast(me, 69580);
                    _events.Repeat(Milliseconds(20000));
                    break;
            }

            DoMeleeAttackIfReady();
        }

        private:
            EventMap _events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetPitOfSaronAI<npc_pos_deathwhisper_necrolyteAI>(creature);
    }
};

// ---- After First Boss (Garfrost) scene ----

class npc_pos_after_first_boss : public CreatureScript
{
public:
    npc_pos_after_first_boss() : CreatureScript("npc_pos_after_first_boss") { }

    struct npc_pos_after_first_bossAI : public NullCreatureAI
    {
        npc_pos_after_first_bossAI(Creature* creature) : NullCreatureAI(creature), summons(me)
        {
            pInstance = me->GetInstanceScript();
        }

        InstanceScript* pInstance;
        EventMap events;
        SummonList summons;
        uint8 counter;

        void Reset() override
        {
            events.Reset();
            counter = 0;

            if (pInstance && pInstance->GetData(DATA_INSTANCE_PROGRESS) >= INSTANCE_PROGRESS_FINISHED_KRICK_SCENE)
                me->DespawnOrUnsummon();
        }

        void DoAction(int32 /*p*/) override
        {
            me->setActive(true);
            events.ScheduleEvent(1, 3s);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case 0:
                    break;
                case 1:
                {
                    if (Creature* summon = me->SummonCreature(FBSData[counter].entry, FBSSpawnPos))
                    {
                        summon->SetWalk(true);
                        summon->GetMotionMaster()->MovePath(FBSData[counter].pathId, false);
                        summons.Summon(summon);
                        ++counter;
                    }

                    if (FBSData[counter].entry != 0)
                        events.Repeat(2s);
                    else
                        events.ScheduleEvent(2, 9s);
                }
                break;
                case 2:
                    if (pInstance)
                        if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_TYRANNUS_EVENT_GUID)))
                            c->AI()->Talk(SAY_TYRANNUS_GARFROST);
                    events.ScheduleEvent(3, 4s);
                    break;
                case 3:
                    Talk(SAY_GENERAL_GARFROST);
                    events.ScheduleEvent(4, 1600ms);
                    break;
                case 4:
                    me->DespawnOrUnsummon();
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetPitOfSaronAI<npc_pos_after_first_bossAI>(creature);
    }
};

// ---- Tyrannus event: wave system ----

class npc_pos_tyrannus_events : public CreatureScript
{
public:
    npc_pos_tyrannus_events() : CreatureScript("npc_pos_tyrannus_events") { }

    struct npc_pos_tyrannus_eventsAI : public NullCreatureAI
    {
        npc_pos_tyrannus_eventsAI(Creature* creature) : NullCreatureAI(creature), summons(me)
        {
            pInstance = me->GetInstanceScript();
        }

        InstanceScript* pInstance;
        EventMap events;
        SummonList summons;
        uint8 stage;

        void Reset() override
        {
            events.Reset();
            summons.DespawnAll();
            stage = 0;
        }

        void DoAction(int32 /*p*/) override
        {
            me->setActive(true);
            events.ScheduleEvent(1, 1ms);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case 0:
                    break;
                case 1:
                {
                    static constexpr uint8 wave1Count = 10;
                    static uint32 const wave1Entries[wave1Count] =
                    {
                        NPC_YMIRJAR_DEATHBRINGER, NPC_YMIRJAR_WRATHBRINGER, NPC_YMIRJAR_FLAMEBEARER,
                        NPC_YMIRJAR_DEATHBRINGER, NPC_YMIRJAR_WRATHBRINGER,
                        NPC_YMIRJAR_DEATHBRINGER, NPC_YMIRJAR_WRATHBRINGER, NPC_YMIRJAR_FLAMEBEARER,
                        NPC_YMIRJAR_DEATHBRINGER, NPC_YMIRJAR_WRATHBRINGER
                    };

                    for (uint8 i = 0; i < wave1Count; ++i)
                    {
                        float x = 679.0f + rand_norm() * 50.0f;
                        float y = 141.0f + rand_norm() * 40.0f;
                        if (Creature* c = me->SummonCreature(wave1Entries[i], x, y, 515.0f, 0.0f))
                        {
                            c->SetWalk(true);
                            c->GetMotionMaster()->MovePoint(0, 679.0f + rand_norm() * 50.0f, 88.0f + rand_norm() * 40.0f, 511.0f);
                        }
                    }

                    if (pInstance)
                        if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_TYRANNUS_EVENT_GUID)))
                            c->AI()->Talk(SAY_TYRANNUS_AMBUSH_1);
                    pInstance->SetData(DATA_INSTANCE_PROGRESS, INSTANCE_PROGRESS_AFTER_WARN_1);
                    events.ScheduleEvent(2, 1s);
                    break;
                }
                case 2:
                {
                    uint32 heroicAdd = pInstance && pInstance->instance->GetDifficulty() == DUNGEON_DIFFICULTY_HEROIC ? 3 : 0;
                    for (uint8 i = 0; i < 6 + heroicAdd; ++i)
                    {
                        uint32 entry = (i % 2 == 0) ? NPC_FALLEN_WARRIOR : NPC_WRATHBONE_COLDWRAITH;
                        float x = 849.0f + rand_norm() * 20.0f;
                        float y = 115.0f + rand_norm() * 20.0f - 10.0f;
                        if (Creature* c = me->SummonCreature(entry, x, y, 510.0f, 0.0f))
                        {
                            me->SetFacingTo(3.14f);
                            c->GetMotionMaster()->MovePoint(0, 699.0f + rand_norm() * 50.0f, 115.0f, 510.0f);
                        }
                    }

                    if (pInstance)
                        if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_TYRANNUS_EVENT_GUID)))
                            c->AI()->Talk(SAY_TYRANNUS_AMBUSH_2);
                    pInstance->SetData(DATA_INSTANCE_PROGRESS, INSTANCE_PROGRESS_AFTER_WARN_2);
                    events.ScheduleEvent(3, 2s);
                    break;
                }
                case 3:
                {
                    if (Creature* v = me->FindNearestCreature(NPC_TYRANNUS_VOICE, 200.0f))
                        v->AI()->Talk(SAY_TYRANNUS_TRAP_TUNNEL);

                    pInstance->SetData(DATA_INSTANCE_PROGRESS, INSTANCE_PROGRESS_AFTER_TUNNEL_WARN);
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetPitOfSaronAI<npc_pos_tyrannus_eventsAI>(creature);
    }
};

// ---- Icicle trigger (Acore style) ----

class npc_pos_icicle_trigger : public CreatureScript
{
public:
    npc_pos_icicle_trigger() : CreatureScript("npc_pos_icicle_trigger") { }

    struct npc_pos_icicle_triggerAI : public ScriptedAI
    {
        npc_pos_icicle_triggerAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.ScheduleEvent(1, 1s);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            if (_events.ExecuteEvent() == 1)
            {
                DoCastSelf(SPELL_TUNNEL_ICICLE);
                _events.Repeat(Milliseconds(urand(1200, 3000)));
            }
        }

        private:
            EventMap _events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_pos_icicle_triggerAI(creature);
    }
};


// ---- Martin/Gorkun escort after Krick ----

class npc_pos_martin_or_gorkun_second : public CreatureScript
{
public:
    npc_pos_martin_or_gorkun_second() : CreatureScript("npc_pos_martin_or_gorkun_second") { }

    struct npc_pos_martin_or_gorkun_secondAI : public NullCreatureAI
    {
        npc_pos_martin_or_gorkun_secondAI(Creature* creature) : NullCreatureAI(creature), summons(me)
        {
            pInstance = me->GetInstanceScript();
            events.Reset();
            me->setActive(true);
            me->SetVisible(false);

            if (pInstance)
            {
                if (pInstance->GetData(DATA_INSTANCE_PROGRESS) >= INSTANCE_PROGRESS_AFTER_TUNNEL_WARN)
                    me->SetVisible(true);
                if (pInstance->GetData(DATA_INSTANCE_PROGRESS) == INSTANCE_PROGRESS_TYRANNUS_INTRO)
                {
                    me->SetVisible(true);
                    me->UpdatePosition(TSSpawnPos);
                }
            }
        }

        InstanceScript* pInstance;
        EventMap events;
        SummonList summons;

        void DoAction(int32 p) override
        {
            if (p == 1)
            {
                events.ScheduleEvent(1, 0ms);
                me->SetVisible(true);
                me->setActive(true);
            }
            if (p == 2)
                events.ScheduleEvent(1, 0ms);
            if (p == 3)
                me->GetMotionMaster()->MovePoint(0, TSMidPos);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE && id == 0)
                events.ScheduleEvent(100, 0ms);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case 0:
                    break;
                case 1:
                    me->SetWalk(true);
                    me->GetMotionMaster()->MovePoint(0, SBSLeaderEndPos);
                    break;
                case 100:
                    me->SetWalk(true);
                    me->GetMotionMaster()->MovePath(PATH_BEGIN_VALUE + 9, false);
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetPitOfSaronAI<npc_pos_martin_or_gorkun_secondAI>(creature);
    }
};

// ---- Freed slave ----

class npc_pos_freed_slave : public CreatureScript
{
public:
    npc_pos_freed_slave() : CreatureScript("npc_pos_freed_slave") { }

    struct npc_pos_freed_slaveAI : public ScriptedAI
    {
        npc_pos_freed_slaveAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetReactState(REACT_DEFENSIVE);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetPitOfSaronAI<npc_pos_freed_slaveAI>(creature);
    }
};

// ---- Jaina/Sylvanas final escort (before Tyrannus) ----

class npc_pos_leader_second : public CreatureScript
{
public:
    npc_pos_leader_second() : CreatureScript("npc_pos_leader_second") { }

    struct npc_pos_leader_secondAI : public NullCreatureAI
    {
        npc_pos_leader_secondAI(Creature* creature) : NullCreatureAI(creature)
        {
            pInstance = me->GetInstanceScript();
            barrierGUID.Clear();
            events.Reset();
            me->RemoveNpcFlag(UNIT_NPC_FLAG_QUESTGIVER);

            if (pInstance)
            {
                if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_RIMEFANG_GUID)))
                {
                    c->RemoveAllAuras();
                    c->GetMotionMaster()->Clear();
                    c->GetMotionMaster()->MoveIdle();
                    c->SetVisible(false);
                }
                if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_MARTIN_OR_GORKUN_GUID)))
                    c->AI()->DoAction(2);
            }
        }

        InstanceScript* pInstance;
        EventMap events;
        ObjectGuid barrierGUID;

        void DoAction(int32 p) override
        {
            if (p == 1)
            {
                events.ScheduleEvent(1, me->GetEntry() == NPC_JAINA_PART2 ? 15s + 500ms : 18s);
                events.ScheduleEvent(2, me->GetEntry() == NPC_JAINA_PART2 ? 16s + 500ms : 19s);
            }
        }

        void SpellHitTarget(WorldObject* target, SpellInfo const* spell) override
        {
            if ((spell->Id == SPELL_TELEPORT_JAINA || spell->Id == SPELL_TELEPORT_SYLVANAS) && target && target->IsPlayer())
            {
                float angle = rand_norm() * 2 * M_PI;
                float dist = (float)urand(1, 4);
                target->ToPlayer()->NearTeleportTo(me->GetPositionX() + cos(angle) * dist, me->GetPositionY() + std::sin(angle) * dist, me->GetPositionZ(), me->GetOrientation());
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != WAYPOINT_MOTION_TYPE)
                return;

            switch (id)
            {
                case 1:
                    Talk(me->GetEntry() == NPC_JAINA_PART2 ? SAY_JAINA_OUTRO_2 : SAY_SYLVANAS_OUTRO_2);
                    break;
                case 2:
                    if (me->GetEntry() == NPC_JAINA_PART2)
                        Talk(SAY_JAINA_OUTRO_3);
                    break;
                case 7:
                    me->SetNpcFlag(UNIT_NPC_FLAG_QUESTGIVER);
                    if (GameObject* g = me->FindNearestGameObject(GO_HOR_PORTCULLIS, 50.0f))
                        g->SetGoState(GO_STATE_ACTIVE);
                    break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case 0:
                    break;
                case 1:
                    if (pInstance)
                        if (Creature* c = me->SummonCreature(NPC_SINDRAGOSA, TSSindragosaPos1))
                        {
                            c->SetCanFly(true);
                            c->SetDisableGravity(true);
                            c->GetMotionMaster()->MovePoint(0, TSSindragosaPos2);
                        }
                    break;
                case 2:
                    if (pInstance)
                        if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_MARTIN_OR_GORKUN_GUID)))
                            c->AI()->Talk(SAY_GENERAL_OUTRO_2);
                    events.ScheduleEvent(3, me->GetEntry() == NPC_JAINA_PART2 ? 7s : 8s);
                    break;
                case 3:
                    Talk(me->GetEntry() == NPC_JAINA_PART2 ? SAY_JAINA_OUTRO_1 : SAY_SYLVANAS_OUTRO_1);
                    me->CastSpell(me, me->GetEntry() == NPC_JAINA_PART2 ? SPELL_TELEPORT_JAINA_VISUAL : SPELL_TELEPORT_SYLVANAS_VISUAL, true);
                    events.ScheduleEvent(4, 2s);
                    break;
                case 4:
                    me->CastSpell(me, me->GetEntry() == NPC_JAINA_PART2 ? SPELL_TELEPORT_JAINA : SPELL_TELEPORT_SYLVANAS, true);
                    if (GameObject* barrier = me->SummonGameObject(203005, Position(1055.49f, 115.03f, 628.15f, 2.08f), QuaternionData(), 86400s))
                        barrierGUID = barrier->GetGUID();
                    events.ScheduleEvent(5, 1500ms);
                    break;
                case 5:
                    if (pInstance)
                        if (Creature* x = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_MARTIN_OR_GORKUN_GUID)))
                        {
                            if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_SINDRAGOSA_GUID)))
                                c->CastSpell(Position(x->GetPositionX(), x->GetPositionY(), x->GetPositionZ()), SPELL_SINDRAGOSA_FROST_BOMB_POS, true);
                        }
                    events.ScheduleEvent(6, 5s);
                    events.ScheduleEvent(10, 2s);
                    break;
                case 6:
                    if (pInstance)
                        if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_SINDRAGOSA_GUID)))
                            c->GetMotionMaster()->MovePoint(0, TSSindragosaPos1);
                    events.ScheduleEvent(7, 4500ms);
                    break;
                case 7:
                    if (pInstance)
                        if (Creature* c = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_SINDRAGOSA_GUID)))
                            c->SetVisible(false);
                    if (GameObject* barrier = pInstance->instance->GetGameObject(barrierGUID))
                        barrier->Delete();
                    barrierGUID.Clear();
                    events.ScheduleEvent(8, 2s);
                    break;
                case 8:
                    me->GetMotionMaster()->MovePath(me->GetEntry() == NPC_JAINA_PART2 ? PATH_BEGIN_VALUE + 16 : PATH_BEGIN_VALUE + 17, false);
                    break;
                case 10:
                    if (Creature* x = pInstance->instance->GetCreature(pInstance->GetGuidData(DATA_MARTIN_OR_GORKUN_GUID)))
                        x->AI()->DoAction(3);
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetPitOfSaronAI<npc_pos_leader_secondAI>(creature);
    }
};

// ---- Spells ----

class spell_pos_empowered_blizzard_aura : public AuraScript
{
    PrepareAuraScript(spell_pos_empowered_blizzard_aura);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ 70131 });
    }

    void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
    {
        PreventDefaultAction();
        if (Unit* caster = GetCaster())
            caster->CastSpell(Position((float)urand(447, 480), (float)urand(200, 235), 528.71f), 70131, true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_pos_empowered_blizzard_aura::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

const Position slaveFreePos[4] =
{
    {699.82f, -82.68f, 512.6f, 0.0f},
    {643.51f, 79.20f, 511.57f, 0.0f},
    {800.09f, 78.66f, 510.2f, 0.0f},
    {528.26f, 187.04f, 528.75f, 0.0f}
};

class SlaveRunEvent : public BasicEvent
{
public:
    SlaveRunEvent(Creature& owner) : _owner(owner) { }

    bool Execute(uint64 /*eventTime*/, uint32 /*updateTime*/) override
    {
        uint32 pointId = 0;
        float minDist = _owner.GetExactDist2dSq(&slaveFreePos[pointId]);
        for (uint32 i = 1; i < 4; ++i)
        {
            float dist = _owner.GetExactDist2dSq(&slaveFreePos[i]);
            if (dist < minDist)
            {
                minDist = dist;
                pointId = i;
            }
        }
        if (minDist < 200.0f * 200.0f)
            _owner.GetMotionMaster()->MovePoint(0, slaveFreePos[pointId]);
        return true;
    }

private:
    Creature& _owner;
};

class spell_pos_slave_trigger_closest : public SpellScript
{
    PrepareSpellScript(spell_pos_slave_trigger_closest);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetHitUnit())
            if (target->GetEmoteState())
            {
                if (Unit* caster = GetCaster())
                    if (Player* p = caster->ToPlayer())
                    {
                        p->RewardPlayerAndGroupAtEvent(36764, caster);
                        p->RewardPlayerAndGroupAtEvent(36770, caster);

                        target->SetEmoteState(Emote(0));
                        if (Creature* c = target->ToCreature())
                        {
                            c->DespawnOrUnsummon(7s);
                            c->AI()->Talk(0, p);
                            c->m_Events.AddEventAtOffset(new SlaveRunEvent(*c), 3s);
                        }
                    }
            }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_pos_slave_trigger_closest::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_pos_rimefang_frost_nova : public SpellScript
{
    PrepareSpellScript(spell_pos_rimefang_frost_nova);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetHitUnit())
            if (Unit* caster = GetCaster())
            {
                Unit::Kill(caster, target);
                if (target->IsCreature())
                    target->ToCreature()->DespawnOrUnsummon(30s);
            }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_pos_rimefang_frost_nova::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_pos_blight_aura : public AuraScript
{
    PrepareAuraScript(spell_pos_blight_aura);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ 69604 });
    }

    void HandleEffectPeriodic(AuraEffect const* aurEff)
    {
        if (aurEff->GetTotalTicks() >= 0 && aurEff->GetTickNumber() == uint32(aurEff->GetTotalTicks()))
            if (Unit* target = GetTarget())
                target->CastSpell(target, 69604, true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_pos_blight_aura::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
    }
};

class spell_pos_glacial_strike_aura : public AuraScript
{
    PrepareAuraScript(spell_pos_glacial_strike_aura);

    void HandleEffectPeriodic(AuraEffect const* aurEff)
    {
        if (Unit* target = GetTarget())
            if (target->GetHealth() == target->GetMaxHealth())
            {
                PreventDefaultAction();
                aurEff->GetBase()->Remove(AURA_REMOVE_BY_EXPIRE);
                return;
            }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_pos_glacial_strike_aura::HandleEffectPeriodic, EFFECT_2, SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
    }
};

// ===== TC existing content (monster AIs, icicle, achievements) =====

enum Spells
{
    SPELL_FIREBALL              = 69583,
    SPELL_HELLFIRE              = 69586,
    SPELL_TACTICAL_BLINK        = 69584,
    SPELL_FROST_BREATH          = 69527,
    SPELL_LEAPING_FACE_MAUL     = 69504,
};

enum Events
{
    EVENT_FIREBALL              = 1,
    EVENT_TACTICAL_BLINK        = 2,
};

struct npc_ymirjar_flamebearer : public ScriptedAI
{
    npc_ymirjar_flamebearer(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_FIREBALL, 4s);
        _events.ScheduleEvent(EVENT_TACTICAL_BLINK, 15s);
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
                case EVENT_FIREBALL:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0))
                        DoCast(target, SPELL_FIREBALL);
                    _events.Repeat(5s);
                    break;
                case EVENT_TACTICAL_BLINK:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0))
                        DoCast(target, SPELL_TACTICAL_BLINK);
                    DoCast(me, SPELL_HELLFIRE);
                    _events.Repeat(12s);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap _events;
};

struct npc_iceborn_protodrake : public ScriptedAI
{
    npc_iceborn_protodrake(Creature* creature) : ScriptedAI(creature)
    {
        Initialize();
    }

    void Initialize()
    {
        _frostBreathCooldown = 5000;
    }

    void Reset() override
    {
        Initialize();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (Vehicle* _vehicle = me->GetVehicleKit())
            _vehicle->RemoveAllPassengers();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        if (_frostBreathCooldown < diff)
        {
            DoCastVictim(SPELL_FROST_BREATH);
            _frostBreathCooldown = 10000;
        }
        else
            _frostBreathCooldown -= diff;

        DoMeleeAttackIfReady();
    }

private:
    uint32 _frostBreathCooldown;
};

struct npc_geist_ambusher : public ScriptedAI
{
    npc_geist_ambusher(Creature* creature) : ScriptedAI(creature)
    {
        Initialize();
    }

    void Initialize()
    {
        _leapingFaceMaulCooldown = 9000;
    }

    void Reset() override
    {
        Initialize();
    }

    void JustEngagedWith(Unit* who) override
    {
        if (who->GetTypeId() != TYPEID_PLAYER)
            return;

        if (me->GetDistance(who) > 5.0f)
            DoCast(who, SPELL_LEAPING_FACE_MAUL);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        if (_leapingFaceMaulCooldown < diff)
        {
            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 5.0f, true))
                DoCast(target, SPELL_LEAPING_FACE_MAUL);
            _leapingFaceMaulCooldown = urand(9000, 14000);
        }
        else
            _leapingFaceMaulCooldown -= diff;

        DoMeleeAttackIfReady();
    }

private:
    uint32 _leapingFaceMaulCooldown;
};

struct npc_pit_of_saron_icicle : public PassiveAI
{
    npc_pit_of_saron_icicle(Creature* creature) : PassiveAI(creature)
    {
        me->SetDisplayId(me->GetCreatureTemplate()->Modelid1);
    }

    void IsSummonedBy(WorldObject* summoner) override
    {
        _summonerGUID = summoner->GetGUID();

        _scheduler.Schedule(Milliseconds(3650), [this](TaskContext /*context*/)
        {
            DoCastSelf(SPELL_ICICLE_FALL_TRIGGER, true);
            DoCastSelf(SPELL_ICICLE_FALL_VISUAL);

            if (Unit* caster = ObjectAccessor::GetUnit(*me, _summonerGUID))
                caster->RemoveDynObject(SPELL_TUNNEL_ICICLE);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
    ObjectGuid _summonerGUID;
};

class spell_pos_ice_shards : public SpellScript
{
    PrepareSpellScript(spell_pos_ice_shards);

    bool Load() override
    {
        return InstanceHasScript(GetCaster(), PoSScriptName);
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        if (GetHitPlayer())
            GetCaster()->GetInstanceScript()->SetData(DATA_ICE_SHARDS_HIT, 1);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_pos_ice_shards::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

enum TyrannusEventCavernEmote
{
    SAY_TYRANNUS_CAVERN_ENTRANCE = 3
};

class at_pit_cavern_entrance : public AreaTriggerScript
{
    public:
        at_pit_cavern_entrance() : AreaTriggerScript("at_pit_cavern_entrance") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (InstanceScript* instance = player->GetInstanceScript())
            {
                if (instance->GetData(DATA_CAVERN_ACTIVE))
                    return true;

                instance->SetData(DATA_CAVERN_ACTIVE, 1);

                if (Creature* tyrannus = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_TYRANNUS_EVENT_GUID)))
                    tyrannus->AI()->Talk(SAY_TYRANNUS_CAVERN_ENTRANCE);
            }
            return true;
        }
};

class at_pit_cavern_end : public AreaTriggerScript
{
public:
    at_pit_cavern_end() : AreaTriggerScript("at_pit_cavern_end") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            instance->SetData(DATA_CAVERN_ACTIVE, 0);

            if (!instance->GetData(DATA_ICE_SHARDS_HIT))
                instance->DoUpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, SPELL_DONT_LOOK_UP_ACHIEV_CREDIT, 0, player);
        }

        return true;
    }
};

void AddSC_pit_of_saron()
{
    // Acore event scripts
    new npc_pos_leader();
    new npc_pos_deathwhisper_necrolyte();
    new npc_pos_after_first_boss();
    new npc_pos_tyrannus_events();
    new npc_pos_icicle_trigger();
    new npc_pos_martin_or_gorkun_second();
    new npc_pos_freed_slave();
    new npc_pos_leader_second();

    RegisterSpellScript(spell_pos_empowered_blizzard_aura);
    RegisterSpellScript(spell_pos_slave_trigger_closest);
    RegisterSpellScript(spell_pos_rimefang_frost_nova);
    RegisterSpellScript(spell_pos_blight_aura);
    RegisterSpellScript(spell_pos_glacial_strike_aura);

    // TC existing monster AIs, icicles and achievements
    RegisterPitOfSaronCreatureAI(npc_ymirjar_flamebearer);
    RegisterPitOfSaronCreatureAI(npc_iceborn_protodrake);
    RegisterPitOfSaronCreatureAI(npc_geist_ambusher);
    RegisterPitOfSaronCreatureAI(npc_pit_of_saron_icicle);
    RegisterSpellScript(spell_pos_ice_shards);
    new at_pit_cavern_entrance();
    new at_pit_cavern_end();
}
