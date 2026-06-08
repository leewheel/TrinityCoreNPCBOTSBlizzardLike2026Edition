/*
 * This file is part of the LWCore Project.
 * Ported from AzerothCore with modifications for TrinityCore framework.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Player.h"

enum Npcs
{
    NPC_NETHER_DRAKE    = 20903,
    NPC_IRONWING        = 29154,
    NPC_DABIR           = 19409,
    NPC_BRACK           = 19401,
    NPC_IRENA           = 23413,
    NPC_AYREN           = 25059,
    NPC_DRAGONHAWK      = 25236,
    NPC_VERONIA         = 20162,
    NPC_DEESAK          = 23415,
    NPC_AFRASASTRASZ    = 27575,
    NPC_TARIOLSTRASZ    = 26443,
    NPC_TORASTRASZA     = 26949,
    NPC_CESSA           = 23704,
    NPC_KIELAR          = 17209,
};

enum Misc
{
    REP_SKYGUARD        = 1031,
    QUEST_NETHERY_WINGS = 10438,
    ITEM_DISRUPTOR      = 29778,
    QUEST_BEHIND_ENEMY  = 10652,
    QUEST_GATEWAYS_A    = 10146,
    QUEST_SHATTER_POINT = 10340,
    QUEST_GATEWAYS_H    = 10129,
    QUEST_ABBYSAL       = 10162,
    QUEST_ABBYSAL_DAILY = 10347,
    QUEST_SPINEBREAKER  = 10242,
    QUEST_DEAD_SCAR     = 11532,
    QUEST_AIR_STRIKE    = 11533,
    QUEST_INTERCEPT     = 11542,
    QUEST_KEEP_AT_BEY   = 11543,
    QUEST_SURVEY_ALCAZ  = 11142,
};

struct npc_taxiAI : public ScriptedAI
{
    npc_taxiAI(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipHello(Player* player) override
    {
        if (me->IsQuestGiver())
            player->PrepareQuestMenu(me->GetGUID());

        switch (me->GetEntry())
        {
            case NPC_NETHER_DRAKE:
                if (player->GetQuestStatus(QUEST_NETHERY_WINGS) == QUEST_STATUS_INCOMPLETE && player->HasItemCount(ITEM_DISRUPTOR))
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                break;
            case NPC_IRONWING:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                break;
            case NPC_DABIR:
                if (player->GetQuestStatus(QUEST_GATEWAYS_A) == QUEST_STATUS_INCOMPLETE)
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
                if (!player->GetQuestRewardStatus(QUEST_SHATTER_POINT))
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
                break;
            case NPC_BRACK:
                if (player->GetQuestStatus(QUEST_GATEWAYS_H) == QUEST_STATUS_INCOMPLETE)
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);
                if (player->GetQuestStatus(QUEST_ABBYSAL) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(QUEST_ABBYSAL_DAILY) == QUEST_STATUS_INCOMPLETE)
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 9);
                if (player->GetQuestStatus(QUEST_SPINEBREAKER) == QUEST_STATUS_COMPLETE)
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 10);
                break;
            case NPC_IRENA:
                if (player->GetReputationRank(REP_SKYGUARD) >= REP_HONORED)
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 11);
                break;
            case NPC_AYREN:
                if (player->GetQuestStatus(QUEST_DEAD_SCAR) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(QUEST_AIR_STRIKE) == QUEST_STATUS_INCOMPLETE)
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 12);
                if (player->GetQuestStatus(QUEST_INTERCEPT) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(QUEST_KEEP_AT_BEY) == QUEST_STATUS_INCOMPLETE)
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 13);
                break;
            case NPC_DRAGONHAWK:
                if (player->GetQuestStatus(QUEST_INTERCEPT) == QUEST_STATUS_COMPLETE || player->GetQuestStatus(QUEST_KEEP_AT_BEY) == QUEST_STATUS_COMPLETE)
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 14);
                break;
            case NPC_VERONIA:
                if (player->GetQuestStatus(QUEST_BEHIND_ENEMY) != QUEST_STATUS_REWARDED)
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 15);
                break;
            case NPC_DEESAK:
                if (player->GetReputationRank(REP_SKYGUARD) >= REP_HONORED)
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 16);
                break;
            case NPC_AFRASASTRASZ:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "中层到底层", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 17);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "中层到顶层", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 18);
                break;
            case NPC_TARIOLSTRASZ:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "底层到顶层", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 19);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "底层到中层", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 20);
                break;
            case NPC_TORASTRASZA:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "顶层到中层", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 21);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "顶层到底层", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 22);
                break;
            case NPC_CESSA:
                if (player->GetQuestStatus(QUEST_SURVEY_ALCAZ) == QUEST_STATUS_INCOMPLETE)
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "飞行", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 25);
                break;
            case NPC_KIELAR:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "北关塔", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 26);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "东墙塔", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 27);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "皇冠守卫塔", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 28);
                break;
        }

        SendGossipMenuFor(player, player->GetGossipTextId(me), me->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
    {
        uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
        ClearGossipMenuFor(player);

        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 35731, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 53335, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 4:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 33768, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 5:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 35069, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 8:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 33659, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 9:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 33825, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 10:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 34578, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 11:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 41278, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 12:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 45071, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 13:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 45113, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 14:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 45353, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 15:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 34905, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 16:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 41279, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 17:
                CloseGossipMenuFor(player);
                player->ActivateTaxiPathTo(882);
                break;
            case GOSSIP_ACTION_INFO_DEF + 18:
                CloseGossipMenuFor(player);
                player->ActivateTaxiPathTo(881);
                break;
            case GOSSIP_ACTION_INFO_DEF + 19:
                CloseGossipMenuFor(player);
                player->ActivateTaxiPathTo(878);
                break;
            case GOSSIP_ACTION_INFO_DEF + 20:
                CloseGossipMenuFor(player);
                player->ActivateTaxiPathTo(883);
                break;
            case GOSSIP_ACTION_INFO_DEF + 21:
                CloseGossipMenuFor(player);
                player->ActivateTaxiPathTo(880);
                break;
            case GOSSIP_ACTION_INFO_DEF + 22:
                CloseGossipMenuFor(player);
                player->ActivateTaxiPathTo(879);
                break;
            case GOSSIP_ACTION_INFO_DEF + 25:
                CloseGossipMenuFor(player);
                player->CastSpell(player, 42295, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 26:
                CloseGossipMenuFor(player);
                player->ActivateTaxiPathTo(494);
                break;
            case GOSSIP_ACTION_INFO_DEF + 27:
                CloseGossipMenuFor(player);
                player->ActivateTaxiPathTo(495);
                break;
            case GOSSIP_ACTION_INFO_DEF + 28:
                CloseGossipMenuFor(player);
                player->ActivateTaxiPathTo(496);
                break;
        }
        return true;
    }
};

class npc_taxi : public CreatureScript
{
public:
    npc_taxi() : CreatureScript("npc_taxi") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_taxiAI(creature);
    }
};

void AddSC_npc_taxi()
{
    new npc_taxi;
}
