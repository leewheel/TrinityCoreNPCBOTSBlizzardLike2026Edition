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

#ifndef DEF_ZF_H
#define DEF_ZF_H

#include "CreatureAIImpl.h"

#define ZFScriptName "instance_zulfarrak"
#define DataHeader "ZF"

enum ZFEntries
{
    ENTRY_ZUM_RAH            = 7271,
    ENTRY_BLY                = 7604,
    ENTRY_RAVEN              = 7605,
    ENTRY_ORO                = 7606,
    ENTRY_WEEGLI             = 7607,
    ENTRY_MURTA              = 7608,

    ENTRY_SHADOWPRIEST_SEZZZIZ = 7275,
    ENTRY_SANDFURY_ZEALOT      = 8877,
    ENTRY_SANDFURY_ACOLYTE     = 8876,
    ENTRY_NEKRUM_GUTCHEWER     = 7796,

    GO_END_DOOR         = 146084,

    EVENT_PYRAMID       = 1,
    EVENT_GAHZRILLA
};

enum ZFDataTypes
{
    DATA_ZUM_RAH = 0
};

enum ZFPyramidPhases
{
    PYRAMID_NOT_STARTED, //default
    PYRAMID_CAGES_OPEN, //happens in GO hello for cages
    PYRAMID_ARRIVED_AT_STAIR, //happens in Weegli's movementinform
    PYRAMID_WAVE_1,
    PYRAMID_PRE_WAVE_2,
    PYRAMID_WAVE_2,
    PYRAMID_PRE_WAVE_3,
    PYRAMID_WAVE_3,
    PYRAMID_KILLED_ALL_TROLLS,
    PYRAMID_MOVED_DOWNSTAIRS,
    PYRAMID_DESTROY_GATES,
    PYRAMID_GATES_DESTROYED,
    PYRAMID_DONE,
};

template <class AI, class T>
inline AI* GetZulFarrakAI(T* obj)
{
    return GetInstanceAI<AI>(obj, ZFScriptName);
}

#endif
