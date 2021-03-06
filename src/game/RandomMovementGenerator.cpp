/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2008-2014 Hellground <http://hellground.net/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "Creature.h"
#include "MapManager.h"
#include "RandomMovementGenerator.h"
#include "Map.h"
#include "Util.h"

#include "movement/MoveSplineInit.h"
#include "movement/MoveSpline.h"

template<>
void RandomMovementGenerator<Creature>::_setRandomLocation(Creature &creature)
{
    Position dest;
    creature.GetRespawnCoord(dest.x, dest.y, dest.z, &dest.o, &wander_distance);

    bool is_air_ok = creature.CanFly();

    const float angle = frand(0.0f, M_PI*2.0f);
    const float range = frand(0.0f, wander_distance);

    creature.GetValidPointInAngle(dest, range, angle, false);

    static_cast<MovementGenerator*>(this)->_recalculateTravel = false;

    if (is_air_ok)
        i_nextMoveTime.Reset(0);
    else
    {
        if (roll_chance_i(MOVEMENT_RANDOM_MMGEN_CHANCE_NO_BREAK))
            i_nextMoveTime.Reset(0);
        else
            i_nextMoveTime.Reset(urand(500, 10000));
    }

    creature.addUnitState(UNIT_STAT_ROAMING);

    Movement::MoveSplineInit init(creature);
    init.MoveTo(dest.x, dest.y, dest.z);
    init.SetWalk(true);
    init.Launch();

    // Call for creature group update
    if (creature.GetFormation() && creature.GetFormation()->getLeader() == &creature)
        creature.GetFormation()->LeaderMoveTo(dest.x, dest.y, dest.z);
}

template<>
void RandomMovementGenerator<Creature>::Initialize(Creature &creature)
{
    if (!creature.isAlive())
        return;

    if (!wander_distance)
        wander_distance = creature.GetRespawnRadius();

    creature.addUnitState(UNIT_STAT_ROAMING);
    _setRandomLocation(creature);
}

template<>
void RandomMovementGenerator<Creature>::Reset(Creature &creature)
{
    Initialize(creature);
}

template<>
void RandomMovementGenerator<Creature>::Interrupt(Creature &creature)
{
    creature.clearUnitState(UNIT_STAT_ROAMING);
    creature.SetWalk(false);
}

template<>
void RandomMovementGenerator<Creature>::Finalize(Creature &creature)
{
    creature.clearUnitState(UNIT_STAT_ROAMING);
    creature.SetWalk(false);
}

template<>
bool RandomMovementGenerator<Creature>::Update(Creature &creature, const uint32 &diff)
{
    if (creature.hasUnitState(UNIT_STAT_ROOT | UNIT_STAT_STUNNED | UNIT_STAT_DISTRACTED))
    {
        i_nextMoveTime.Reset(0);  // Expire the timer
        creature.clearUnitState(UNIT_STAT_ROAMING);
        return true;
    }

    if (creature.IsStopped() || static_cast<MovementGenerator*>(this)->_recalculateTravel)
    {
        i_nextMoveTime.Update(diff);
        if (i_nextMoveTime.Passed() || static_cast<MovementGenerator*>(this)->_recalculateTravel)
            _setRandomLocation(creature);
    }
    return true;
}

template<>
bool RandomMovementGenerator<Creature>::GetResetPosition(Creature& creature, float& x, float& y, float& z)
{
    float radius;
    creature.GetRespawnCoord(x, y, z, NULL, &radius);

    // use current if in range
    if (creature.IsInRange2d(x,y, 0, radius))
        creature.GetPosition(x,y,z);

    return true;
}
