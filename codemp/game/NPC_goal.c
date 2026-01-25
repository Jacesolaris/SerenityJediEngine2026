/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, SerenityJediEngine2026 contributors

This file is part of the SerenityJediEngine2026 source code.

SerenityJediEngine2026 is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

//b_goal.cpp
#include "b_local.h"
#include "icarus/Q3_Interface.h"

extern qboolean FlyingCreature(const gentity_t* ent);
/*
SetGoal
*/

void SetGoal(gentity_t* goal, float rating)
{
	NPCS.NPCInfo->goalEntity = goal;
	NPCS.NPCInfo->goalTime = level.time;

	if (goal)
	{
		//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_INFO, "NPC_SetGoal: %s @ %s (%f)\n", goal->classname, vtos( goal->currentOrigin), rating );
	}
	else
	{
		//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_INFO, "NPC_SetGoal: NONE\n" );
	}
}

/*
NPC_SetGoal
*/

void NPC_SetGoal(gentity_t* goal, const float rating)
{
	if (goal == NPCS.NPCInfo->goalEntity)
	{
		return;
	}

	if (!goal)
	{
		return;
	}

	if (goal->client)
	{
		return;
	}

	if (NPCS.NPCInfo->goalEntity)
	{
		NPCS.NPCInfo->lastGoalEntity = NPCS.NPCInfo->goalEntity;
	}

	SetGoal(goal, rating);
}

/*
NPC_ClearGoal
*/

void NPC_ClearGoal(void)
{
	if (!NPCS.NPCInfo->lastGoalEntity)
	{
		SetGoal(NULL, 0.0);
		return;
	}

	gentity_t* goal = NPCS.NPCInfo->lastGoalEntity;
	NPCS.NPCInfo->lastGoalEntity = NULL;

	if (goal->inuse && !(goal->s.eFlags & EF_NODRAW))
	{
		SetGoal(goal, 0); //, NPCInfo->lastGoalEntityNeed
		return;
	}

	SetGoal(NULL, 0.0);
}

/*
-------------------------
G_BoundsOverlap
-------------------------
*/

qboolean G_BoundsOverlap(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2)
{
	//NOTE: flush up against counts as overlapping
	if (mins1[0] > maxs2[0])
		return qfalse;

	if (mins1[1] > maxs2[1])
		return qfalse;

	if (mins1[2] > maxs2[2])
		return qfalse;

	if (maxs1[0] < mins2[0])
		return qfalse;

	if (maxs1[1] < mins2[1])
		return qfalse;

	if (maxs1[2] < mins2[2])
		return qfalse;

	return qtrue;
}

void NPC_ReachedGoal(void)
{
	//	Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_INFO, "UpdateGoal: reached goal entity\n" );
	NPC_ClearGoal();
	NPCS.NPCInfo->goalTime = level.time;

	//MCG - Begin
	NPCS.NPCInfo->aiFlags &= ~NPCAI_MOVING;
	NPCS.ucmd.forwardmove = 0;
	//Return that the goal was reached
	trap->ICARUS_TaskIDComplete((sharedEntity_t*)NPCS.NPC, TID_MOVE_NAV);
	//MCG - End
}

/*
ReachedGoal

id removed checks against waypoints and is now checking surfaces
*/
//qboolean NAV_HitNavGoal( vec3_t point, vec3_t mins, vec3_t maxs, gentity_t *goal, qboolean flying );
static qboolean ReachedGoal(gentity_t* goal)
{
	if (NPCS.NPCInfo->aiFlags & NPCAI_TOUCHED_GOAL)
	{
		NPCS.NPCInfo->aiFlags &= ~NPCAI_TOUCHED_GOAL;
		return qtrue;
	}
	return NAV_HitNavGoal(NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, goal->r.currentOrigin, NPCS.NPCInfo->goalRadius, FlyingCreature(NPCS.NPC));
}

/*
static gentity_t *UpdateGoal( void )

Id removed a lot of shit here... doesn't seem to handle waypoints independently of goalentity

In fact, doesn't seem to be any waypoint info on entities at all any more?

MCG - Since goal is ALWAYS goalEntity, took out a lot of sending goal entity pointers around for no reason
*/

gentity_t* UpdateGoal(void)
{
	if (!NPCS.NPCInfo->goalEntity)
	{
		return NULL;
	}

	if (!NPCS.NPCInfo->goalEntity->inuse)
	{
		//Somehow freed it, but didn't clear it
		NPC_ClearGoal();
		return NULL;
	}

	gentity_t* goal = NPCS.NPCInfo->goalEntity;

	if (ReachedGoal(goal))
	{
		NPC_ReachedGoal();
		goal = NULL; //so they don't keep trying to move to it
	} //else if fail, need to tell script so?

	return goal;
}