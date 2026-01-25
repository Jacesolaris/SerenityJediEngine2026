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

// cg_playerstate.c -- this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when
// following another player or playing back a demo, it will be checked
// when the snapshot transitions like all the other entities

#include "cg_media.h"

/*
==============
CG_CheckAmmo

If the ammo has gone low enough to generate the warning, play a sound
==============
*/
static void CG_CheckAmmo()
{
	int total;
	int previous;

#if 0

	// see about how many seconds of ammo we have remaining
	weapons = cg.snap->ps.stats[STAT_WEAPONS];
	total = 0;

	for (i = WP_SABER; i < WP_NUM_WEAPONS i++)
	{
		if (!(weapons & (1 << i)))
			continue;

		if (total >= 5000)
		{
			cg.lowAmmoWarning = 0;
			return;
		}
	}
#endif

	// Don't bother drawing the ammo warning when have no weapon selected
	if (cg.weaponSelect == WP_NONE)
	{
		return;
	}

	total = cg.snap->ps.ammo[weaponData[cg.weaponSelect].ammoIndex];

	if (total > weaponData[cg.weaponSelect].ammoLow) // Low on ammo?
	{
		cg.lowAmmoWarning = 0;
		return;
	}

	previous = cg.lowAmmoWarning;

	if (!total)
	{
		cg.lowAmmoWarning = 2;
	}
	else // Got a little left
	{
		cg.lowAmmoWarning = 1;
	}

	// play a sound on transitions
	if (cg.lowAmmoWarning != previous)
	{
		cgi_S_StartLocalSound(cgs.media.noAmmoSound, CHAN_LOCAL_SOUND);
	}
}

/*
==============
CG_DamageFeedback
==============
*/
static void CG_DamageFeedback(const int yaw_byte, const int pitch_byte, const int damage)
{
	float scale;

	//FIXME: Based on MOD, do different kinds of damage effects,
	//		for example, Borg damage could progressively tint screen green and raise FOV?

	// the lower on health you are, the greater the view kick will be
	const int health = cg.snap->ps.stats[STAT_HEALTH];
	if (health < 40)
	{
		scale = 1;
	}
	else
	{
		scale = 40.0 / health;
	}
	float kick = damage * scale;

	if (kick < 5)
		kick = 5;
	if (kick > 10)
		kick = 10;

	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if (yaw_byte == 255 && pitch_byte == 255)
	{
		cg.damageX = 0;
		cg.damageY = 0;
		cg.v_dmg_roll = 0;
		cg.v_dmg_pitch = -kick;
	}
	else
	{
		vec3_t angles{};
		vec3_t dir;
		// positional
		const float pitch = pitch_byte / 255.0 * 360;
		const float yaw = yaw_byte / 255.0 * 360;

		angles[PITCH] = pitch;
		angles[YAW] = yaw;
		angles[ROLL] = 0;

		AngleVectors(angles, dir, nullptr, nullptr);
		VectorSubtract(vec3_origin, dir, dir);

		float front = DotProduct(dir, cg.refdef.viewaxis[0]);
		const float left = DotProduct(dir, cg.refdef.viewaxis[1]);
		const float up = DotProduct(dir, cg.refdef.viewaxis[2]);

		dir[0] = front;
		dir[1] = left;
		dir[2] = 0;
		float dist = VectorLength(dir);
		if (dist < 0.1)
		{
			dist = 0.1f;
		}

		cg.v_dmg_roll = kick * left;

		cg.v_dmg_pitch = -kick * front;

		if (front <= 0.1)
		{
			front = 0.1f;
		}
		cg.damageX = -left / front;
		cg.damageY = up / dist;
	}

	// clamp the position
	if (cg.damageX > 1.0)
	{
		cg.damageX = 1.0;
	}
	if (cg.damageX < -1.0)
	{
		cg.damageX = -1.0;
	}

	if (cg.damageY > 1.0)
	{
		cg.damageY = 1.0;
	}
	if (cg.damageY < -1.0)
	{
		cg.damageY = -1.0;
	}

	// don't let the screen flashes vary as much
	if (kick > 10)
	{
		kick = 10;
	}
	cg.damageValue = kick;
	cg.v_dmg_time = cg.time + DAMAGE_TIME;
	cg.damageTime = cg.snap->serverTime;
}

/*
================
CG_Respawn

A respawn happened this snapshot
================
*/
void CG_Respawn()
{
	// no error decay on player movement
	cg.thisFrameTeleport = qtrue;

	// display weapons available
	SetWeaponSelectTime();

	// select the weapon the server says we are using
	if (cg.snap->ps.weapon)
		cg.weaponSelect = cg.snap->ps.weapon;
}

/*
==============
CG_CheckPlayerstateEvents

==============
*/
static void CG_CheckPlayerstateEvents(playerState_t* ps, playerState_t* ops)
{
	int event;
	centity_t* cent;

#if 0
	if (ps->externalEvent && ps->externalEvent != ops->externalEvent)
	{
		cent = &cg_entities[ps->clientNum];
		cent->currentState.event = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent(cent, cent->lerpOrigin);
	}
#endif

	for (int i = ps->eventSequence - MAX_PS_EVENTS; i < ps->eventSequence; i++)
	{
		if (ps->events[i & MAX_PS_EVENTS - 1] != ops->events[i & MAX_PS_EVENTS - 1]
			|| i >= ops->eventSequence)
		{
			event = ps->events[i & MAX_PS_EVENTS - 1];

			cent = &cg_entities[ps->clientNum];
			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[i & MAX_PS_EVENTS - 1];
			CG_EntityEvent(cent, cent->lerpOrigin);
		}
	}
}

/*
==================
CG_CheckLocalSounds
==================
*/
void CG_PainEvent(centity_t* cent, int health);

static void CG_CheckLocalSounds(const playerState_t* ps, const playerState_t* ops)
{
	// hit changes
	if (ps->persistant[PERS_HITS] > ops->persistant[PERS_HITS])
	{
		//cgi_S_StartLocalSound(cgs.media.hit_sound, CHAN_LOCAL_SOUND);
	}
	else if (ps->persistant[PERS_HITS] < ops->persistant[PERS_HITS])
	{
		//cgi_S_StartLocalSound(cgs.media.hitTeamSound, CHAN_LOCAL_SOUND);
	}

	// health changes of more than -3 should make pain sounds
	if (ps->stats[STAT_HEALTH] < ops->stats[STAT_HEALTH] - 3)
	{
		if (ps->stats[STAT_HEALTH] > 0)
		{
			CG_PainEvent(&cg_entities[ps->clientNum], ps->stats[STAT_HEALTH]);
		}
	}

	if (cg_awardsounds.integer)
	{
		if (ps->persistant[PERS_EXCELLENT_COUNT] != ops->persistant[PERS_EXCELLENT_COUNT])
		{
			cgi_S_StartLocalSound(cgs.media.excellentSound, CHAN_ANNOUNCER);
		}
		if (ps->persistant[PERS_GAUNTLET_FRAG_COUNT] != ops->persistant[PERS_GAUNTLET_FRAG_COUNT])
		{
			cgi_S_StartLocalSound(cgs.media.humiliationSound, CHAN_ANNOUNCER);
		}
		/*if (ps->persistant[PERS_IMPRESSIVE_COUNT] != ops->persistant[PERS_IMPRESSIVE_COUNT])
		{
			cgi_S_StartLocalSound(cgs.media.impressiveSound, CHAN_ANNOUNCER);
			reward = qtrue;
		}*/
		if (ps->persistant[PERS_PLAYEREVENTS] != ops->persistant[PERS_PLAYEREVENTS])
		{
			if ((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_DENIEDREWARD) != (ops->persistant[PERS_PLAYEREVENTS] &
				PLAYEREVENT_DENIEDREWARD))
			{
				cgi_S_StartLocalSound(cgs.media.deniedSound, CHAN_ANNOUNCER);
			}
			else if ((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_GAUNTLETREWARD) != (ops->persistant[
				PERS_PLAYEREVENTS] & PLAYEREVENT_GAUNTLETREWARD))
			{
				cgi_S_StartLocalSound(cgs.media.humiliationSound, CHAN_ANNOUNCER);
			}
		}
	}
}

/*
===============
CG_TransitionPlayerState

===============
*/
void CG_TransitionPlayerState(playerState_t* ps, playerState_t* ops)
{
	// teleporting
	if ((ps->eFlags ^ ops->eFlags) & EF_TELEPORT_BIT)
	{
		cg.thisFrameTeleport = qtrue;
	}
	else
	{
		cg.thisFrameTeleport = qfalse;
	}

	// check for changing follow mode
	if (ps->clientNum != ops->clientNum)
	{
		cg.thisFrameTeleport = qtrue;
		// make sure we don't get any unwanted transition effects
		*ops = *ps;
	}

	// damage events (player is getting wounded)
	if (ps->damageEvent != ops->damageEvent && ps->damageCount)
	{
		CG_DamageFeedback(ps->damageYaw, ps->damagePitch, ps->damageCount);
	}

	// respawning
	if (ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT])
	{
		CG_Respawn();
	}

	CG_CheckLocalSounds(ps, ops);

	// check for going low on ammo
	CG_CheckAmmo();

	// run events
	CG_CheckPlayerstateEvents(ps, ops);

	// smooth the ducking viewheight change
	if (ps->viewheight != ops->viewheight)
	{
		if (!cg.nextFrameTeleport)
		{
			//when we crouch/uncrouch in mid-air, our viewhieght doesn't actually change in
			//absolute world coordinates, just locally.
			cg.duckChange = ps->viewheight - ops->viewheight;
			cg.duckTime = cg.time;
		}
	}
}