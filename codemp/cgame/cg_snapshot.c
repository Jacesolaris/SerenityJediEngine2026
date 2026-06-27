/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

// cg_snapshot.c -- things that happen on snapshot transition,
// not necessarily every single rendered frame

#include "cg_local.h"

/*
==================
CG_ResetEntity
==================
*/
static void CG_ResetEntity(centity_t* cent)
{
	// if the previous snapshot this entity was updated in is at least
	// an event window back in time then we can reset the previous event
	if (cent->snapShotTime < cg.time - EVENT_VALID_MSEC)
	{
		cent->previousEvent = 0;
	}

	cent->trailTime = cg.snap->serverTime;

	VectorCopy(cent->currentState.origin, cent->lerpOrigin);
	VectorCopy(cent->currentState.angles, cent->lerpAngles);

	if (cent->currentState.eFlags & EF_G2ANIMATING)
	{
		//reset the animation state
		cent->pe.torso.animationNumber = -1;
		cent->pe.legs.animationNumber = -1;
	}

#if 0
	if (cent->isRagging && (cent->currentState.eFlags & EF_DEAD))
	{
		VectorAdd(cent->lerpOrigin, cent->lerpOriginOffset, cent->lerpOrigin);
	}
#endif

	if (cent->currentState.eType == ET_PLAYER || cent->currentState.eType == ET_NPC)
	{
		CG_ResetPlayerEntity(cent);
	}
}

/*
===============
CG_TransitionEntity

cent->nextState is moved to cent->currentState and events are fired
===============
*/
static void CG_TransitionEntity(centity_t* cent)
{
	cent->currentState = cent->nextState;
	cent->currentValid = qtrue;

	// reset if the entity wasn't in the last frame or was teleported
	if (!cent->interpolate)
	{
		CG_ResetEntity(cent);
	}

	// clear the next state.  if will be set by the next CG_SetNextSnap
	cent->interpolate = qfalse;

	// check for events
	CG_CheckEvents(cent);
}

/*
==================
CG_SetInitialSnapshot

This will only happen on the very first snapshot, or
on tourney restarts.  All other times will use
CG_TransitionSnapshot instead.

FIXME: Also called by map_restart?
==================
*/
static void CG_SetInitialSnapshot(snapshot_t* snap)
{
	cg.snap = snap;

	if (cg_entities[snap->ps.clientNum].ghoul2 == NULL && trap->G2_HaveWeGhoul2Models(
		cgs.clientinfo[snap->ps.clientNum].ghoul2Model))
	{
		trap->G2API_DuplicateGhoul2Instance(cgs.clientinfo[snap->ps.clientNum].ghoul2Model,
			&cg_entities[snap->ps.clientNum].ghoul2);
		CG_CopyG2WeaponInstance(&cg_entities[snap->ps.clientNum], FIRST_WEAPON,
			cg_entities[snap->ps.clientNum].ghoul2);

		if (trap->G2API_AddBolt(cg_entities[snap->ps.clientNum].ghoul2, 0, "face") == -1)
		{
			//check now to see if we have this bone for setting anims and such
			cg_entities[snap->ps.clientNum].noFace = qtrue;
		}
	}
	BG_PlayerStateToEntityState(&snap->ps, &cg_entities[snap->ps.clientNum].currentState, qfalse);

	// sort out solid entities
	CG_BuildSolidList();

	CG_ExecuteNewServerCommands(snap->serverCommandSequence);

	// set our local weapon selection pointer to
	// what the server has indicated the current weapon is
	CG_Respawn();

	for (int i = 0; i < cg.snap->numEntities; i++)
	{
		const entityState_t* state = &cg.snap->entities[i];
		centity_t* cent = &cg_entities[state->number];

		memcpy(&cent->currentState, state, sizeof(entityState_t));
		//cent->currentState = *state;
		cent->interpolate = qfalse;
		cent->currentValid = qtrue;

		CG_ResetEntity(cent);

		// check for events
		CG_CheckEvents(cent);
	}
}

/*
===================
CG_TransitionSnapshot

The transition point from snap to nextSnap has passed
===================
*/
extern qboolean CG_UsingEWeb(void); //cg_predict.c
static void CG_TransitionSnapshot(void)
{
	centity_t* cent = NULL;
	int i = 0;

	// Validate cg.snap
	if (cg.snap == NULL)
	{
		Com_Printf(S_COLOR_RED "CG_TransitionSnapshot: cg.snap was NULL — aborting transition.\n");
		return;
	}

	// Validate cg.nextSnap
	if (cg.nextSnap == NULL)
	{
		Com_Printf(S_COLOR_RED "CG_TransitionSnapshot: cg.nextSnap was NULL — aborting transition.\n");
		return;
	}

	// Execute any server string commands before transitioning entities
	CG_ExecuteNewServerCommands(cg.nextSnap->serverCommandSequence);

	// Clear the currentValid flag for all entities in the existing snapshot
	for (i = 0; i < cg.snap->numEntities; i++)
	{
		cent = &cg_entities[cg.snap->entities[i].number];
		cent->currentValid = qfalse;
	}

	// Move nextSnap to snap and do the transitions
	snapshot_t* oldFrame = cg.snap;
	cg.snap = cg.nextSnap;

	BG_PlayerStateToEntityState(
		&cg.snap->ps,
		&cg_entities[cg.snap->ps.clientNum].currentState,
		qfalse
	);

	cg_entities[cg.snap->ps.clientNum].interpolate = qfalse;

	for (i = 0; i < cg.snap->numEntities; i++)
	{
		cent = &cg_entities[cg.snap->entities[i].number];
		CG_TransitionEntity(cent);

		// Remember time of snapshot this entity was last updated in
		cent->snapShotTime = cg.snap->serverTime;
	}

	// Clear nextSnap pointer
	cg.nextSnap = NULL;

	// Check for playerstate transition events
	if (oldFrame != NULL)
	{
		playerState_t* ops = &oldFrame->ps;
		const playerState_t* ps = &cg.snap->ps;

		// Teleporting checks are irrespective of prediction
		if (((ps->eFlags ^ ops->eFlags) & EF_TELEPORT_BIT) != 0)
		{
			cg.thisFrameTeleport = qtrue; // will be cleared by prediction code
		}

		// If we are not doing client-side movement prediction for any reason,
		// then the client events and view changes will be issued now
		if (cg.demoPlayback == qtrue ||
			(cg.snap->ps.pm_flags & PMF_FOLLOW) != 0 ||
			cg_noPredict.integer != 0 ||
			g_synchronousClients.integer != 0 ||
			CG_UsingEWeb() == qtrue)
		{
			CG_TransitionPlayerState(ps, ops);
		}
	}
}

/*
===================
CG_SetNextSnap

A new snapshot has just been read in from the client system.
===================
*/
extern void CGCam_Fade(vec4_t source, vec4_t dest, float duration);

static void CG_SetNextSnap(snapshot_t* snap)
{
	cg.nextSnap = snap;

	BG_PlayerStateToEntityState(&snap->ps, &cg_entities[snap->ps.clientNum].nextState, qfalse);

	// check for extrapolation errors
	for (int num = 0; num < snap->numEntities; num++)
	{
		const entityState_t* es = &snap->entities[num];
		centity_t* cent = &cg_entities[es->number];

		memcpy(&cent->nextState, es, sizeof(entityState_t));

		// if this frame is a teleport, or the entity wasn't in the
		// previous frame, don't interpolate
		if (!cent->currentValid || (cent->currentState.eFlags ^ es->eFlags) & EF_TELEPORT_BIT)
		{
			cent->interpolate = qfalse;
		}
		else
		{
			cent->interpolate = qtrue;
		}
	}

	// if the next frame is a teleport for the playerstate, we
	// can't interpolate during demos
	if (cg.snap && (snap->ps.eFlags ^ cg.snap->ps.eFlags) & EF_TELEPORT_BIT)
	{
		cg.nextFrameTeleport = qtrue;
	}
	else
	{
		cg.nextFrameTeleport = qfalse;
	}

	// if changing follow mode, don't interpolate
	if (cg.nextSnap->ps.clientNum != cg.snap->ps.clientNum)
	{
		cg.nextFrameTeleport = qtrue;
	}

	// if changing server restarts, don't interpolate
	if ((cg.nextSnap->snapFlags ^ cg.snap->snapFlags) & SNAPFLAG_SERVERCOUNT)
	{
		//hack to turn off camera fade on map_restarts
		vec4_t vec4_origin = { 0, 0, 0, 0 };
		CGCam_Fade(vec4_origin, vec4_origin, 0);
		cg.nextFrameTeleport = qtrue;
	}

	// sort out solid entities
	CG_BuildSolidList();
}

/*
========================
CG_ReadNextSnapshot

This is the only place new snapshots are requested
This may increment cgs.processedSnapshotNum multiple
times if the client system fails to return a
valid snapshot.
========================
*/
static snapshot_t* CG_ReadNextSnapshot(void)
{
	snapshot_t* dest;

	if (cg.latestSnapshotNum > cgs.processedSnapshotNum + 1000)
	{
		trap->Print("WARNING: CG_ReadNextSnapshot: way out of range, %i > %i\n",
			cg.latestSnapshotNum, cgs.processedSnapshotNum);
	}

	while (cgs.processedSnapshotNum < cg.latestSnapshotNum)
	{
		// decide which of the two slots to load it into
		if (cg.snap == &cg.activeSnapshots[0])
		{
			dest = &cg.activeSnapshots[1];
		}
		else
		{
			dest = &cg.activeSnapshots[0];
		}

		// try to read the snapshot from the client system
		cgs.processedSnapshotNum++;
		const qboolean r = trap->GetSnapshot(cgs.processedSnapshotNum, dest);

		// FIXME: why would trap->GetSnapshot return a snapshot with the same server time
		if (cg.snap && r && dest->serverTime == cg.snap->serverTime)
		{
			//According to dumbledore, this situation occurs when you're playing back a demo that was record when
			//the game was running in local mode.  As such, we need to skip those snaps or the demo looks laggy.
			if (cg.demoPlayback)
			{
				continue;
			}
		}

		// if it succeeded, return
		if (r)
		{
			CG_AddLagometerSnapshotInfo(dest);
			return dest;
		}

		// a GetSnapshot will return failure if the snapshot
		// never arrived, or  is so old that its entities
		// have been shoved off the end of the circular
		// buffer in the client system.

		// record as a dropped packet
		CG_AddLagometerSnapshotInfo(NULL);

		// If there are additional snapshots, continue trying to
		// read them.
	}

	// nothing left to read
	return NULL;
}

/*
============
CG_ProcessSnapshots

We are trying to set up a renderable view, so determine
what the simulated time is, and try to get snapshots
both before and after that time if available.

If we don't have a valid cg.snap after exiting this function,
then a 3D game view cannot be rendered.  This should only happen
right after the initial connection.  After cg.snap has been valid
once, it will never turn invalid.

Even if cg.snap is valid, cg.nextSnap may not be, if the snapshot
hasn't arrived yet (it becomes an extrapolating situation instead
of an interpolating one)

============
*/
void CG_ProcessSnapshots(void)
{
	snapshot_t* snap = NULL;
	int n = 0;

	// See what the latest snapshot the client system has is
	trap->GetCurrentSnapshotNumber(&n, &cg.latestSnapshotTime);

	if (n != cg.latestSnapshotNum)
	{
		if (n < cg.latestSnapshotNum)
		{
			Com_Printf(S_COLOR_RED "CG_ProcessSnapshots: n < cg.latestSnapshotNum — ignoring invalid snapshot.\n");
			return;
		}

		cg.latestSnapshotNum = n;
	}

	// If we have yet to receive a snapshot, check for it.
	// Once we have gotten the first snapshot, cg.snap will
	// always have valid data for the rest of the game
	while (cg.snap == NULL)
	{
		snap = CG_ReadNextSnapshot();
		if (snap == NULL)
		{
			// We can't continue until we get a snapshot
			return;
		}

		// Set our weapon selection to what the playerstate is currently using
		if ((snap->snapFlags & SNAPFLAG_NOT_ACTIVE) == 0)
		{
			CG_SetInitialSnapshot(snap);
		}
	}

	// Loop until we either have a valid nextSnap with a serverTime
	// greater than cg.time to interpolate towards, or we run
	// out of available snapshots
	for (;;)
	{
		// If we don't have a nextframe, try and read a new one in
		if (cg.nextSnap == NULL)
		{
			snap = CG_ReadNextSnapshot();

			// If we still don't have a nextframe, we will just have to extrapolate
			if (snap == NULL)
			{
				break;
			}

			CG_SetNextSnap(snap);

			// If time went backwards, we have a level restart
			if (cg.nextSnap != NULL &&
				cg.nextSnap->serverTime < cg.snap->serverTime)
			{
				Com_Printf(S_COLOR_RED "CG_ProcessSnapshots: Server time went backwards — aborting.\n");
				cg.nextSnap = NULL;
				return;
			}
		}

		// SAFETY: cg.nextSnap may still be NULL if CG_SetNextSnap failed
		if (cg.nextSnap == NULL)
		{
			// No next snapshot available — extrapolate
			break;
		}

		// If our time is < nextFrame's, we have a nice interpolating state
		if (cg.time >= cg.snap->serverTime &&
			cg.time < cg.nextSnap->serverTime)
		{
			break;
		}

		// We have passed the transition from nextFrame to frame
		CG_TransitionSnapshot();
	}

	// Validate state upon exiting
	if (cg.snap == NULL)
	{
		Com_Printf(S_COLOR_RED "CG_ProcessSnapshots: cg.snap == NULL — aborting.\n");
		return;
	}

	if (cg.time < cg.snap->serverTime)
	{
		// This can happen right after a vid_restart
		cg.time = cg.snap->serverTime;
	}

	if (cg.nextSnap != NULL &&
		cg.nextSnap->serverTime <= cg.time)
	{
		Com_Printf(S_COLOR_RED "CG_ProcessSnapshots: cg.nextSnap->serverTime <= cg.time — clearing nextSnap.\n");
		cg.nextSnap = NULL;
	}
}