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

#include "ahnkahet.h"
#include "AreaBoundary.h"
#include "Containers.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellScript.h"

enum NadoxTexts
{
    SAY_AGGRO = 0,
    SAY_SLAY = 1,
    SAY_DEATH = 2,
    SAY_EGG_SAC = 3,
    EMOTE_HATCHES = 4
};

enum NadoxSpells
{
    // Elder Nadox
    SPELL_BROOD_PLAGUE = 56130,
    SPELL_BROOD_RAGE = 59465,
    SPELL_BERSERK = 26662, // Enraged if too far away from home
    SPELL_SUMMON_SWARMERS = 56119, // 2x 30178  -- cast on egg
    SPELL_SUMMON_SWARM_GUARD = 56120, // 1x 30176 -- cast on guardian egg

    // Adds
    SPELL_SWARM_BUFF = 56281,
    SPELL_SPRINT = 56354,
    SPELL_GUARDIAN_AURA = 56151,
    SPELL_SWARMER_AURA = 56158
};

enum NadoxEvents
{
    EVENT_PLAGUE = 1,
    EVENT_BROOD_RAGE,
    EVENT_SUMMON_SWARMER,
    EVENT_CHECK_ENRAGE,
    EVENT_SPRINT,
    DATA_RESPECT_YOUR_ELDERS
};

ParallelogramBoundary const ElderNadoxRoomBoundary = ParallelogramBoundary(Position(690.96f, -858.93f, -25.69f), Position(571.f, -937.f, -25.69f), Position(722.93f, -908.05f, -25.69f));

struct boss_elder_nadox : public BossAI
{
    boss_elder_nadox(Creature* creature) : BossAI(creature, DATA_ELDER_NADOX)
    {
        Initialize();
    }

    void Initialize()
    {
        _guardianSummoned = false;
        _guardianDied = false;
        _previousSwarmEggGUID.Clear();
    }

    void Reset() override
    {
        _Reset();
        Initialize();
        _swarmEggs.clear();
        _guardianEggs.clear();
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);

        events.ScheduleEvent(EVENT_PLAGUE, 13s);
        events.ScheduleEvent(EVENT_SUMMON_SWARMER, 10s);

        if (IsHeroic())
        {
            events.ScheduleEvent(EVENT_BROOD_RAGE, 12s);
            events.ScheduleEvent(EVENT_CHECK_ENRAGE, 5s);
        }

        // Cache eggs
        std::list<Creature*> eggs;
        // Swarm eggs
        me->GetCreatureListWithEntryInGrid(eggs, NPC_AHNKAHAR_SWARM_EGG, 250.0f);
        for (Creature* egg : eggs)
            if (egg)
                _swarmEggs.push_back(egg->GetGUID());

        eggs.clear();

        // Guardian eggs
        me->GetCreatureListWithEntryInGrid(eggs, NPC_AHNKAHAR_GUARDIAN_EGG, 250.0f);
        for (Creature* egg : eggs)
            if (egg)
                _guardianEggs.push_back(egg->GetGUID());
    }

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
    {
        if (summon->GetEntry() == NPC_AHNKAHAR_GUARDIAN)
            _guardianDied = true;
    }

    uint32 GetData(uint32 type) const override
    {
        if (type == DATA_RESPECT_YOUR_ELDERS)
            return !_guardianDied ? 1 : 0;

        return 0;
    }

    void KilledUnit(Unit* who) override
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_SLAY);
    }

    void JustDied(Unit* /*killer*/) override
    {
        _JustDied();
        Talk(SAY_DEATH);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_PLAGUE:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100, true))
                        DoCast(target, SPELL_BROOD_PLAGUE, true);
                    events.Repeat(15s);
                    break;
                case EVENT_BROOD_RAGE:
                    DoCast(SPELL_BROOD_RAGE);
                    events.Repeat(10s, 50s);
                    break;
                case EVENT_SUMMON_SWARMER:
                    SummonHelpers(true);
                    events.Repeat(10s);
                    break;
                case EVENT_CHECK_ENRAGE:
                    if (me->HasAura(SPELL_BERSERK))
                        return;
                    if (!ElderNadoxRoomBoundary.IsWithinBoundary(me))
                        DoCast(me, SPELL_BERSERK, true);
                    else
                        events.Repeat(5s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        if (!_guardianSummoned && me->HealthBelowPct(50))
        {
            SummonHelpers(false);
            _guardianSummoned = true;
        }

        DoMeleeAttackIfReady();
    }

private:
    bool _guardianSummoned;
    bool _guardianDied;
    GuidList _swarmEggs;
    GuidList _guardianEggs;
    ObjectGuid _previousSwarmEggGUID;

    void SummonHelpers(bool swarm)
    {
        if (swarm)
        {
            if (_swarmEggs.empty())
                return;

            // Copy list and remove previous egg to avoid repeating
            GuidList swarmEggsCopy = _swarmEggs;
            if (!_previousSwarmEggGUID.IsEmpty())
                swarmEggsCopy.remove(_previousSwarmEggGUID);

            if (swarmEggsCopy.empty())
                return;

            _previousSwarmEggGUID = Trinity::Containers::SelectRandomContainerElement(swarmEggsCopy);

            if (Creature* egg = ObjectAccessor::GetCreature(*me, _previousSwarmEggGUID))
                egg->CastSpell(egg, SPELL_SUMMON_SWARMERS, me->GetGUID());

            if (roll_chance_i(33))
                Talk(SAY_EGG_SAC);
        }
        else
        {
            if (_guardianEggs.empty())
                return;

            ObjectGuid const& guardianEggGUID = Trinity::Containers::SelectRandomContainerElement(_guardianEggs);
            if (Creature* egg = ObjectAccessor::GetCreature(*me, guardianEggGUID))
                egg->CastSpell(egg, SPELL_SUMMON_SWARM_GUARD, me->GetGUID());

            Talk(EMOTE_HATCHES, me);

            if (roll_chance_i(33))
                Talk(SAY_EGG_SAC);
        }
    }
};

struct npc_ahnkahar_nerubian : public ScriptedAI
{
    npc_ahnkahar_nerubian(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_SPRINT, 13s);
        DoCastSelf(me->GetEntry() == NPC_AHNKAHAR_GUARDIAN ? SPELL_GUARDIAN_AURA : SPELL_SWARMER_AURA, true);
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
                case EVENT_SPRINT:
                    DoCast(me, SPELL_SPRINT);
                    _events.ScheduleEvent(EVENT_SPRINT, 20s);
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap _events;
};

// 56159 - Swarm
class spell_ahn_kahet_swarm : public SpellScript
{
    PrepareSpellScript(spell_ahn_kahet_swarm);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_SWARM_BUFF });
    }

    void CountTargets(std::list<WorldObject*>& targets)
    {
        _targetCount = targets.size();
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (_targetCount)
        {
            if (Aura* aura = GetCaster()->GetAura(SPELL_SWARM_BUFF))
            {
                aura->SetStackAmount(_targetCount);
                aura->RefreshDuration();
            }
            else
            {
                CastSpellExtraArgs args;
                args.TriggerFlags = TRIGGERED_FULL_MASK;
                args.AddSpellMod(SPELLVALUE_AURA_STACK, _targetCount);
                GetCaster()->CastSpell(GetCaster(), SPELL_SWARM_BUFF, args);
            }
        }
        else
            GetCaster()->RemoveAurasDueToSpell(SPELL_SWARM_BUFF);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ahn_kahet_swarm::CountTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
        OnEffectHit += SpellEffectFn(spell_ahn_kahet_swarm::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }

    uint32 _targetCount = 0;
};

class achievement_respect_your_elders : public AchievementCriteriaScript
{
    public:
        achievement_respect_your_elders() : AchievementCriteriaScript("achievement_respect_your_elders") { }

        bool OnCheck(Player* /*player*/, Unit* target) override
        {
            return target && target->GetAI()->GetData(DATA_RESPECT_YOUR_ELDERS);
        }
};

void AddSC_boss_elder_nadox()
{
    RegisterAhnKahetCreatureAI(boss_elder_nadox);
    RegisterAhnKahetCreatureAI(npc_ahnkahar_nerubian);
    RegisterSpellScript(spell_ahn_kahet_swarm);
    new achievement_respect_your_elders();
}
