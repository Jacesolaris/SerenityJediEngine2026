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

//
// NPC.cpp - generic functions
//
#include "b_local.h"
#include "anims.h"
#include "say.h"
#include "icarus/Q3_Interface.h"

extern vec3_t player_mins;
extern vec3_t player_maxs;
extern void G_SoundOnEnt(gentity_t* ent, soundChannel_t channel, const char* sound_path);
extern void NPC_BSNoClip(void);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
extern void NPC_ApplyRoff(void);
extern void NPC_TempLookTarget(const gentity_t* self, int lookEntNum, int minLookTime, int maxLookTime);
extern qboolean NPC_CheckLookTarget(const gentity_t* self);
extern void NPC_SetLookTarget(const gentity_t* self, int entNum, int clear_time);
extern void Mark1_dying(gentity_t* self);
extern void NPC_BSCinematic(void);
extern int GetTime(int lastTime);
extern void G_CheckCharmed(gentity_t* self);
extern qboolean Jedi_CultistDestroyer(const gentity_t* self);
extern void NPC_BSGM_Default(void);
extern qboolean Boba_Flying(const gentity_t* self);

//Local Variables
npcStatic_t NPCS;
gNPC_t* NPCInfo;
gclient_t* client;
usercmd_t ucmd;
visibility_t enemyVisibility;

extern vmCvar_t g_saberRealisticCombat;

void NPC_SetAnim(gentity_t* ent, int setAnimParts, int anim, int setAnimFlags);
static bState_t G_CurrentBState(gNPC_t* gNPC);
void pitch_roll_for_slope(gentity_t* forwhom, vec3_t pass_slope);
extern void GM_Dying(gentity_t* self);

extern int eventClearTime;

qboolean NPC_IsTrooper(const gentity_t* self)
{
	return self && self->NPC && self->s.weapon && !!(self->NPC->scriptFlags & SCF_NO_GROUPS);
}

static void CorpsePhysics(gentity_t* self)
{
	// run the bot through the server like it was a real client
	memset(&NPCS.ucmd, 0, sizeof NPCS.ucmd);
	ClientThink(self->s.number, &NPCS.ucmd);
	VectorCopy(self->s.origin, self->s.origin2);

	if (self->client->NPC_class == CLASS_GALAKMECH)
	{
		GM_Dying(self);
	}

	/*if (self->client->pers.botclass == BCLASS_GALAKMECH)
	{
		GM_Dying(self);
	}*/
	//FIXME: match my pitch and roll for the slope of my groundPlane
	if (self->client->ps.groundEntityNum != ENTITYNUM_NONE && !(self->s.eFlags & EF_DISINTEGRATION))
	{
		//on the ground
		//FIXME: check 4 corners
		pitch_roll_for_slope(self, NULL);
	}

	if (eventClearTime == level.time + ALERT_CLEAR_TIME)
	{
		//events were just cleared out so add me again
		if (!(self->client->ps.eFlags & EF_NODRAW))
		{
			AddSightEvent(self->enemy, self->r.currentOrigin, 384, AEL_DISCOVERED, 0.0f);
		}
	}

	if (level.time - self->s.time > 3000)
	{
		//been dead for 3 seconds
		if (g_dismember.integer < 11381138 && !g_saberRealisticCombat.integer)
		{
			//can't be dismembered once dead
			if (self->client->NPC_class != CLASS_PROTOCOL)
			{
				//self->client->dismembered = qtrue;
			}
		}
	}

	if (level.time - self->s.time > 1000)
		//if (self->client->respawnTime < (level.time+ 3000))
	{
		//don't turn "nonsolid" until about 1 second after actual death
		if (self->client->NPC_class != CLASS_MARK1 && self->client->NPC_class != CLASS_INTERROGATOR)
			// The Mark1 & Interrogator stays solid.
		{
			self->r.contents = CONTENTS_CORPSE;
		}

		if (self->message)
		{
			self->r.contents |= CONTENTS_TRIGGER;
		}
	}
}

/*
----------------------------------------
NPC_RemoveBody

Determines when it's ok to ditch the corpse
----------------------------------------
*/
#define REMOVE_DISTANCE		128
#define REMOVE_DISTANCE_SQR (REMOVE_DISTANCE * REMOVE_DISTANCE)

extern float DistancetoClosestPlayer(vec3_t position, int enemy_team);
extern qboolean InPlayersFOV(vec3_t position, int enemy_team, int h_fov, int v_fov, qboolean check_clear_los);

qboolean G_OkayToRemoveCorpse(gentity_t* self)
{
	//racc - check to see if it's ok to remove this body.
	//if we're still on a vehicle, we won't remove ourselves until we get ejected
	if (self->client && self->client->NPC_class != CLASS_VEHICLE && self->s.m_iVehicleNum != 0)
	{
		Vehicle_t* p_veh = g_entities[self->s.m_iVehicleNum].m_pVehicle;
		if (p_veh)
		{
			if (!p_veh->m_pVehicleInfo->Eject(p_veh, p_veh->m_pPilot, qtrue))
			{
				//dammit, still can't get off the vehicle...
				return qfalse;
			}
		}
		else
		{
			//racc - this is bad.  We're on a vehicle, but the vehicle doesn't exist.
			//assert(0);
#ifndef FINAL_BUILD
			Com_Printf(S_COLOR_RED"ERROR: Dead pilot's vehicle removed while corpse was riding it (pilot: %s)???\n", self->targetname);
#endif
		}
	}

	if (self->message)
	{
		//I still have a key
		return qfalse;
	}

	if (trap->ICARUS_IsRunning(self->s.number))
	{
		//still running a script
		return qfalse;
	}

	if (self->activator
		&& self->activator->client
		&& self->activator->client->ps.eFlags2 & EF2_HELD_BY_MONSTER)
	{
		//still holding a victim?
		return qfalse;
	}

	if (self->client
		&& self->client->ps.eFlags2 & EF2_HELD_BY_MONSTER)
	{
		//being held by a creature
		return qfalse;
	}

	if (self->client
		&& self->client->ps.heldByClient)
	{
		//being dragged
		return qfalse;
	}

	//okay, well okay to remove us...?
	return qtrue;
}

void NPC_RemoveBody(gentity_t* ent)
{
	ent->nextthink = level.time + FRAMETIME / 2;

	//run physics at 20fps
	CorpsePhysics(ent);

	if (ent->NPC->nextBStateThink <= level.time)
	{
		//racc - run logic at 20fps
		trap->ICARUS_MaintainTaskManager(ent->s.number);
		ent->NPC->nextBStateThink = level.time + FRAMETIME;

		if (!G_OkayToRemoveCorpse(ent))
		{
			return;
		}
		if (ent->client->NPC_class == CLASS_MARK1)
		{
			Mark1_dying(ent);
		}

		// Since these blow up, remove the bounding box.
		if (ent->client->NPC_class == CLASS_REMOTE
			|| ent->client->NPC_class == CLASS_SENTRY
			|| ent->client->NPC_class == CLASS_PROBE
			|| ent->client->NPC_class == CLASS_INTERROGATOR
			|| ent->client->NPC_class == CLASS_PROBE
			|| ent->client->NPC_class == CLASS_MARK2)
		{
			G_FreeEntity(ent);
			return;
		}
		ent->r.maxs[2] = ent->client->renderInfo.eyePoint[2] - ent->r.currentOrigin[2] + 4;
		if (ent->r.maxs[2] < -8)
		{
			ent->r.maxs[2] = -8;
		}

		if (ent->NPC->aiFlags & NPCAI_HEAL_ROSH)
		{
			//kothos twins' bodies are never removed
			return;
		}

		if (ent->client->NPC_class == CLASS_GALAKMECH)
		{
			//never disappears
			return;
		}

		//if (self->client->pers.botclass == BCLASS_GALAKMECH)
		//{//never disappears
		//	return;
		//}
		if (ent->NPC && ent->NPC->timeOfDeath <= level.time)
		{
			ent->NPC->timeOfDeath = level.time + 1000;
			if (ent->client->playerTeam == NPCTEAM_ENEMY || ent->client->NPC_class == CLASS_PROTOCOL)
			{
				ent->nextthink = level.time + FRAMETIME; // try back in a second

				if (DistancetoClosestPlayer(ent->r.currentOrigin, -1) <= REMOVE_DISTANCE)
				{
					return;
				}

				if (InPlayersFOV(ent->r.currentOrigin, -1, 110, 90, qtrue))
				{
					//a player sees the body, delay removal.
					return;
				}
			}
			//if ( self->enemy )
			{
				if (ent->client && ent->client->ps.saberEntityNum > 0 && ent->client->ps.saberEntityNum <
					ENTITYNUM_WORLD)
				{
					gentity_t* saberent = &g_entities[ent->client->ps.saberEntityNum];
					if (saberent)
					{
						G_FreeEntity(saberent);
					}
				}
				G_FreeEntity(ent);
			}
		}
	}
}

/*
----------------------------------------
NPC_RemoveBody

Determines when it's ok to ditch the corpse
----------------------------------------
*/

static int BodyRemovalPadTime(const gentity_t* ent)
{
	int time;

	if (!ent || !ent->client)
		return 0;

	// team no longer indicates species/race, so in this case we'd use NPC_class, but
	switch (ent->client->NPC_class)
	{
	case CLASS_MOUSE:
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_PROBE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
	case CLASS_SENTRY:
	case CLASS_INTERROGATOR:
		time = 0;
		break;
	default:
		// never go away
		if (g_corpseRemovalTime.integer <= 0)
		{
			time = Q3_INFINITE;
		}
		else
		{
			time = g_corpseRemovalTime.integer * 1000;
		}
		break;
	}
	return time;
}

/*
----------------------------------------
NPC_RemoveBodyEffect

Effect to be applied when ditching the corpse
----------------------------------------
*/

static void NPC_RemoveBodyEffect(void)
{
	if (!NPCS.NPC || !NPCS.NPC->client || NPCS.NPC->s.eFlags & EF_NODRAW)
		return;

	// team no longer indicates species/race, so in this case we'd use NPC_class, but

	// stub code
	switch (NPCS.NPC->client->NPC_class)
	{
	case CLASS_PROBE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
	case CLASS_SENTRY:
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_INTERROGATOR:
	case CLASS_ATST:
		break;
	default:
		break;
	}
}

/*
====================================================================
void pitch_roll_for_slope( gentity_t *forwhom, vec3_t pass_slope )

MG

This will adjust the pitch and roll of a monster to match
a given slope - if a non-'0 0 0' slope is passed, it will
use that value, otherwise it will use the ground underneath
the monster.  If it doesn't find a surface, it does nothing
and returns.
====================================================================
*/

void pitch_roll_for_slope(gentity_t* forwhom, vec3_t pass_slope)
{
	vec3_t slope;
	vec3_t nvf, ovf, ovr, new_angles = { 0, 0, 0 };

	//if we don't have a slope, get one
	if (!pass_slope || VectorCompare(vec3_origin, pass_slope))
	{
		vec3_t endspot;
		vec3_t startspot;
		trace_t trace;

		VectorCopy(forwhom->r.currentOrigin, startspot);
		startspot[2] += forwhom->r.mins[2] + 4;
		VectorCopy(startspot, endspot);
		endspot[2] -= 300;
		trap->Trace(&trace, forwhom->r.currentOrigin, vec3_origin, vec3_origin, endspot, forwhom->s.number, MASK_SOLID,
			qfalse, 0, 0);

		if (trace.fraction >= 1.0)
			return;

		if (VectorCompare(vec3_origin, trace.plane.normal))
			return;

		VectorCopy(trace.plane.normal, slope);
	}
	else
	{
		VectorCopy(pass_slope, slope);
	}

	AngleVectors(forwhom->r.currentAngles, ovf, ovr, NULL);

	vectoangles(slope, new_angles);
	const float pitch = new_angles[PITCH] + 90;
	new_angles[ROLL] = new_angles[PITCH] = 0;

	AngleVectors(new_angles, nvf, NULL, NULL);

	float mod = DotProduct(nvf, ovr);

	if (mod < 0)
		mod = -1;
	else
		mod = 1;

	const float dot = DotProduct(nvf, ovf);

	if (forwhom->client)
	{
		forwhom->client->ps.viewangles[PITCH] = dot * pitch;
		forwhom->client->ps.viewangles[ROLL] = (1 - Q_fabs(dot)) * pitch * mod;
		const float oldmins2 = forwhom->r.mins[2];
		forwhom->r.mins[2] = -24 + 12 * fabs(forwhom->client->ps.viewangles[PITCH]) / 180.0f;
		//FIXME: if it gets bigger, move up
		if (oldmins2 > forwhom->r.mins[2])
		{
			//our mins is now lower, need to move up
			//FIXME: trace?
			forwhom->client->ps.origin[2] += oldmins2 - forwhom->r.mins[2];
			forwhom->r.currentOrigin[2] = forwhom->client->ps.origin[2];
			trap->LinkEntity((sharedEntity_t*)forwhom);
		}
	}
	else
	{
		forwhom->r.currentAngles[PITCH] = dot * pitch;
		forwhom->r.currentAngles[ROLL] = (1 - Q_fabs(dot)) * pitch * mod;
	}
}

/*
----------------------------------------
DeadThink
----------------------------------------
*/
static void DeadThink(void)
{
	trace_t trace;

	const float oldMaxs2 = NPCS.NPC->r.maxs[2];

	NPCS.NPC->r.maxs[2] = NPCS.NPC->client->renderInfo.eyePoint[2] - NPCS.NPC->r.currentOrigin[2] + 4;
	if (NPCS.NPC->r.maxs[2] < -8)
	{
		NPCS.NPC->r.maxs[2] = -8;
	}

	if (NPCS.NPC->r.maxs[2] > oldMaxs2)
	{
		//inflating maxs, make sure we're not inflating into solid
		trap->Trace(&trace, NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, NPCS.NPC->r.currentOrigin,
			NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0);
		if (trace.allsolid)
		{
			//must be inflating
			NPCS.NPC->r.maxs[2] = oldMaxs2;
		}
	}
	//death anim done (or were given a specific amount of time to wait before removal), wait the requisite amount of time them remove
	if (level.time >= NPCS.NPCInfo->timeOfDeath + BodyRemovalPadTime(NPCS.NPC))
	{
		if (NPCS.NPC->client->ps.eFlags & EF_NODRAW)
		{
			if (!trap->ICARUS_IsRunning(NPCS.NPC->s.number))
			{
				NPCS.NPC->think = G_FreeEntity;
				NPCS.NPC->nextthink = level.time + FRAMETIME;
			}
		}
		else
		{
			// Start the body effect first, then delay 400ms before ditching the corpse
			NPC_RemoveBodyEffect();
			NPCS.NPC->think = NPC_RemoveBody;
			NPCS.NPC->nextthink = level.time + FRAMETIME / 2;

			const class_t npc_class = NPCS.NPC->client->NPC_class;
			// check for droids
			if (npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE ||
				npc_class == CLASS_SBD || npc_class == CLASS_BATTLEDROID || npc_class == CLASS_DROIDEKA ||
				npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 || npc_class ==
				CLASS_REMOTE ||
				npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 || npc_class == CLASS_PROTOCOL ||
				npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY)
			{
				// special droid only behaviors
				NPCS.NPC->client->ps.eFlags |= EF_NODRAW;
				NPCS.NPCInfo->timeOfDeath = level.time + FRAMETIME * 8;
			}
			else
				NPCS.NPCInfo->timeOfDeath = level.time + FRAMETIME * 4;
		}
		return;
	}

	// If the player is on the ground and the resting position contents haven't been set yet...(BounceCount tracks the contents)
	if (NPCS.NPC->bounceCount < 0 && NPCS.NPC->s.groundEntityNum >= 0)
	{
		// if client is in a nodrop area, make him/her nodraw
		const int contents = NPCS.NPC->bounceCount = trap->PointContents(NPCS.NPC->r.currentOrigin, -1);

		if (contents & CONTENTS_NODROP)
		{
			NPCS.NPC->client->ps.eFlags |= EF_NODRAW;
		}
	}

	CorpsePhysics(NPCS.NPC);
}

/*
===============
SetNPCGlobals

local function to set globals used throughout the AI code
===============
*/
void SetNPCGlobals(gentity_t* ent)
{
	NPCS.NPC = ent;
	NPCS.NPCInfo = ent->NPC;
	NPCS.client = ent->client;
	memset(&NPCS.ucmd, 0, sizeof(usercmd_t));
}

npcStatic_t _saved_NPCS;

void SaveNPCGlobals(void)
{
	memcpy(&_saved_NPCS, &NPCS, sizeof _saved_NPCS);
}

void RestoreNPCGlobals(void)
{
	memcpy(&NPCS, &_saved_NPCS, sizeof _saved_NPCS);
}

//We MUST do this, other funcs were using NPC illegally when "self" wasn't the global NPC
void ClearNPCGlobals(void)
{
	NPCS.NPC = NULL;
	NPCS.NPCInfo = NULL;
	NPCS.client = NULL;
}

//===============

extern qboolean showBBoxes;
vec3_t NPCDEBUG_RED = { 1.0, 0.0, 0.0 };
vec3_t NPCDEBUG_GREEN = { 0.0, 1.0, 0.0 };
vec3_t NPCDEBUG_BLUE = { 0.0, 0.0, 1.0 };
vec3_t NPCDEBUG_LIGHT_BLUE = { 0.3f, 0.7f, 1.0 };
extern void G_Cube(vec3_t mins, vec3_t maxs, vec3_t color, float alpha);
extern void G_Line(vec3_t start, vec3_t end, vec3_t color, float alpha);
extern qboolean InPlayersPVS(vec3_t point);

static void NPC_ApplyScriptFlags(void)
{
	if (NPCS.NPCInfo->scriptFlags & SCF_CROUCHED)
	{
		if (NPCS.NPCInfo->charmedTime > level.time && (NPCS.ucmd.forwardmove || NPCS.ucmd.rightmove))
		{
			//ugh, if charmed and moving, ignore the crouched command
		}
		else
		{
			NPCS.ucmd.upmove = -127;
		}
	}

	if (NPCS.NPCInfo->scriptFlags & SCF_RUNNING)
	{
		NPCS.ucmd.buttons &= ~BUTTON_WALKING;
	}
	else if (NPCS.NPCInfo->scriptFlags & SCF_WALKING)
	{
		if (NPCS.NPCInfo->charmedTime > level.time && (NPCS.ucmd.forwardmove || NPCS.ucmd.rightmove))
		{
			//ugh, if charmed and moving, ignore the walking command
		}
		else
		{
			NPCS.ucmd.buttons |= BUTTON_WALKING;
		}
	}

	if (NPCS.NPCInfo->scriptFlags & SCF_LEAN_RIGHT)
	{
		NPCS.ucmd.buttons |= BUTTON_USE;
		NPCS.ucmd.rightmove = 127;
		NPCS.ucmd.forwardmove = 0;
		NPCS.ucmd.upmove = 0;
	}
	else if (NPCS.NPCInfo->scriptFlags & SCF_LEAN_LEFT)
	{
		NPCS.ucmd.buttons |= BUTTON_USE;
		NPCS.ucmd.rightmove = -127;
		NPCS.ucmd.forwardmove = 0;
		NPCS.ucmd.upmove = 0;
	}

	if (NPCS.NPCInfo->scriptFlags & SCF_altFire && NPCS.ucmd.buttons & BUTTON_ATTACK)
	{
		//Use altfire instead
		// new code so snipers shoot their altfire properly
		if (NPCS.NPC->client->ps.weapon != WP_DISRUPTOR || NPCS.NPC->client->ps.zoomMode == 0)
		{
			NPCS.ucmd.buttons &= ~BUTTON_ATTACK; // if using alt fire, dont use normal fire
			NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
		}

		if (NPCS.NPC->client->ps.weapon == WP_DISRUPTOR && NPCS.NPC->client->ps.zoomMode != 0 && NPCS.NPCInfo->
			attackHold == 0)
		{
			// snipers can use charged shot
			NPCS.NPCInfo->attackHold = 2000;
		}
		else if (NPCS.NPC->client->ps.weapon == WP_ROCKET_LAUNCHER)
		{
			// npcs can use homing missiles properly now
			NPCS.NPCInfo->attackHold = 2000;
		}
		else if (NPCS.NPC->client->ps.weapon == WP_BRYAR_PISTOL && NPCS.ucmd.buttons & BUTTON_ALT_ATTACK && !
			Q_irand(0, 1))
		{
			// npcs can use charged pistol
			NPCS.NPCInfo->attackHold = 2000;
		}
	}
	else if (NPCS.NPC->client->ps.weapon == WP_DISRUPTOR && !(NPCS.NPCInfo->scriptFlags & SCF_altFire) && NPCS.NPC->
		client->ps.zoomMode != 0)
	{
		// reset sniper zoomMode when switching back to primary fire
		NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
	}
	else if (NPCS.NPC->client->ps.weapon == WP_BOWCASTER && NPCS.ucmd.buttons & BUTTON_ATTACK && !Q_irand(0, 1))
	{
		// npcs can use charged bowcaster
		NPCS.NPCInfo->attackHold = 2000;
	}
	// only removes NPC when it's safe too (Player is out of PVS)
	if (NPCS.NPCInfo->scriptFlags & SCF_SAFE_REMOVE)
	{
		// take from BSRemove
		if (!InPlayersPVS(NPCS.NPC->r.currentOrigin))
		{
			//racc - we can be safely removed.
			G_UseTargets2(NPCS.NPC, NPCS.NPC, NPCS.NPC->target3);
			NPCS.NPC->s.eFlags |= EF_NODRAW;
			NPCS.NPC->s.eType = ET_INVISIBLE;
			NPCS.NPC->r.contents = 0;
			NPCS.NPC->health = 0;
			NPCS.NPC->targetname = NULL;

			//Disappear in half a second
			NPCS.NPC->think = G_FreeEntity;
			NPCS.NPC->nextthink = level.time + FRAMETIME;
		}
	}

	// npcs with thermals must calculate distance to enemy and hold fire properly
	if (NPCS.NPC->client->ps.weapon == WP_THERMAL && NPCS.NPC->enemy)
	{
		NPCS.NPCInfo->attackHold = (int)Distance(NPCS.client->ps.origin, NPCS.NPC->enemy->client->ps.origin) * 2;
	}
}

void Q3_DebugPrint(int level, const char* format, ...);

static void NPC_HandleAIFlags(void)
{
	//FIXME: make these flags checks a function call like NPC_CheckAIFlagsAndTimers
	if (NPCS.NPCInfo->aiFlags & NPCAI_LOST)
	{
		//Print that you need help!
		//FIXME: shouldn't remove this just yet if cg_draw needs it
		NPCS.NPCInfo->aiFlags &= ~NPCAI_LOST;

		if (NPCS.NPCInfo->goalEntity && NPCS.NPCInfo->goalEntity == NPCS.NPC->enemy)
		{
			//We can't nav to our enemy
			//Drop enemy and see if we should search for him
			NPC_LostEnemyDecideChase();
		}
	}

	//been told to play a victory sound after a delay
	if (NPCS.NPCInfo->greetingDebounceTime && NPCS.NPCInfo->greetingDebounceTime < level.time)
	{
		G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_VICTORY1, EV_VICTORY3), Q_irand(2000, 4000));
		NPCS.NPCInfo->greetingDebounceTime = 0;
	}

	if (NPCS.NPCInfo->ffireCount > 0)
	{
		if (NPCS.NPCInfo->ffireFadeDebounce < level.time)
		{
			NPCS.NPCInfo->ffireCount--;
			//Com_Printf( "drop: %d < %d\n", NPCInfo->ffireCount, 3+((2-g_npcspskill.integer)*2) );
			NPCS.NPCInfo->ffireFadeDebounce = level.time + 3000;
		}
	}
	if (d_patched.integer)
	{
		//use patch-style navigation
		if (NPCS.NPCInfo->consecutiveBlockedMoves > 20)
		{
			//been stuck for a while, try again?
			NPCS.NPCInfo->consecutiveBlockedMoves = 0;
		}
	}
}

static void NPC_AvoidWallsAndCliffs(void)
{
	//...
}

static void NPC_CheckAttackScript(void)
{
	if (!(NPCS.ucmd.buttons & BUTTON_ATTACK))
	{
		return;
	}

	G_ActivateBehavior(NPCS.NPC, BSET_ATTACK);
}

float NPC_MaxDistSquaredForWeapon(void);

static void NPC_CheckAttackHold(void)
{
	vec3_t vec;

	// If they don't have an enemy they shouldn't hold their attack anim.
	if (!NPCS.NPC->enemy)
	{
		NPCS.NPCInfo->attackHoldTime = 0;
		return;
	}

	VectorSubtract(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, vec);

	if (VectorLengthSquared(vec) > NPC_MaxDistSquaredForWeapon())
	{
		NPCS.NPCInfo->attackHoldTime = 0;
	}
	else if (NPCS.NPCInfo->attackHoldTime && NPCS.NPCInfo->attackHoldTime > level.time)
	{
		NPCS.ucmd.buttons |= BUTTON_ATTACK;
	}
	else if (NPCS.NPCInfo->attackHold && NPCS.ucmd.buttons & BUTTON_ATTACK)
	{
		NPCS.NPCInfo->attackHoldTime = level.time + NPCS.NPCInfo->attackHold;
	}
	else
	{
		NPCS.NPCInfo->attackHoldTime = 0;
	}
}

qboolean NPC_CanUseAdvancedFighting()
{
	// Evasion/Weapon Switching/etc...
	switch (NPCS.NPC->client->NPC_class)
	{
	case CLASS_BESPIN_COP:
	case CLASS_CLAW:
	case CLASS_COMMANDO:
	case CLASS_DESANN:
	case CLASS_GALAK:
	case CLASS_GRAN:
	case CLASS_IMPERIAL:
	case CLASS_JAN:
	case CLASS_JEDI:
	case CLASS_KYLE:
	case CLASS_LANDO:
	case CLASS_LUKE:
	case CLASS_MORGANKATARN:
	case CLASS_MURJJ:
	case CLASS_REBEL:
	case CLASS_REBORN:
	case CLASS_REELO:
	case CLASS_RODIAN:
	case CLASS_SHADOWTROOPER:
	case CLASS_TAVION:
	case CLASS_TRANDOSHAN:
	case CLASS_WEEQUAY:
	case CLASS_BOBAFETT:
	case CLASS_WOOKIE:
		break;
	default:
		return qfalse;
	}
	return qtrue;
}

static void NPC_KeepCurrentFacing(void)
{
	if (!NPCS.ucmd.angles[YAW])
	{
		NPCS.ucmd.angles[YAW] = ANGLE2SHORT(NPCS.client->ps.viewangles[YAW]) - NPCS.client->ps.delta_angles[YAW];
	}

	if (!NPCS.ucmd.angles[PITCH])
	{
		NPCS.ucmd.angles[PITCH] = ANGLE2SHORT(NPCS.client->ps.viewangles[PITCH]) - NPCS.client->ps.delta_angles[PITCH];
	}
}

/*
-------------------------
NPC_BehaviorSet_Charmed
-------------------------
*/

static void NPC_BehaviorSet_Charmed(const int b_state)
{
	switch (b_state)
	{
	case BS_FOLLOW_LEADER: //# 40: Follow your leader and shoot any enemies you come across
		NPC_BSFollowLeader();
		break;
	case BS_REMOVE:
		NPC_BSRemove();
		break;
	case BS_SEARCH: //# 43: Using current waypoint as a base, search the immediate branches of waypoints for enemies
		NPC_BSSearch();
		break;
	case BS_WANDER: //# 46: Wander down random waypoint paths
		NPC_BSWander();
		break;
	case BS_FLEE:
		NPC_BSFlee();
		break;
	default:
	case BS_DEFAULT: //whatever
		NPC_BSDefault();
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Default
-------------------------
*/

void NPC_BehaviorSet_Default(const int b_state)
{
	if (NPCS.NPC->enemy && NPCS.NPC->enemy->inuse && NPCS.NPC->enemy->health > 0)
	{
		// Have an anemy... Check if we should use advanced fighting for this NPC...
		if (NPC_CanUseAdvancedFighting())
		{
			// This NPC can use advanced tactics... Use them!!!
			NPC_BSJedi_Default();
			return;
		}
	}
	switch (b_state)
	{
	case BS_ADVANCE_FIGHT: //head toward captureGoal, shoot anything that gets in the way
		NPC_BSAdvanceFight();
		break;
	case BS_SLEEP: //Follow a path, looking for enemies
		NPC_BSSleep();
		break;
	case BS_FOLLOW_LEADER: //# 40: Follow your leader and shoot any enemies you come across
		NPC_BSFollowLeader();
		break;
	case BS_JUMP: //41: Face navgoal and jump to it.
		NPC_BSJump();
		break;
	case BS_REMOVE:
		NPC_BSRemove();
		break;
	case BS_SEARCH: //# 43: Using current waypoint as a base, search the immediate branches of waypoints for enemies
		NPC_BSSearch();
		break;
	case BS_NOCLIP:
		NPC_BSNoClip();
		break;
	case BS_WANDER: //# 46: Wander down random waypoint paths
		NPC_BSWander();
		break;
	case BS_FLEE:
		NPC_BSFlee();
		break;
	case BS_WAIT:
		NPC_BSWait();
		break;
	case BS_CINEMATIC:
		NPC_BSCinematic();
		break;
	default:
	case BS_DEFAULT: //whatever
		NPC_BSDefault();
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Interrogator
-------------------------
*/
static void NPC_BehaviorSet_Interrogator(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSInterrogator_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_ImperialProbe
-------------------------
*/
static void NPC_BehaviorSet_ImperialProbe(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSImperialProbe_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

void NPC_BSSeeker_Default(void);

/*
-------------------------
NPC_BehaviorSet_Seeker
-------------------------
*/
static void NPC_BehaviorSet_Seeker(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSeeker_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

void NPC_BSRemote_Default(void);

/*
-------------------------
NPC_BehaviorSet_Remote
-------------------------
*/
static void NPC_BehaviorSet_Remote(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSRemote_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

void NPC_BSSentry_Default(void);

/*
-------------------------
NPC_BehaviorSet_Sentry
-------------------------
*/
static void NPC_BehaviorSet_Sentry(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSentry_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Grenadier
-------------------------
*/
static void NPC_BehaviorSet_Grenadier(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSGrenadier_Default();
		break;

	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Tusken
-------------------------
*/
static void NPC_BehaviorSet_Tusken(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		//NPC_BSTusken_Default();
		break;

	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Sniper
-------------------------
*/
static void NPC_BehaviorSet_Sniper(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSniper_Default();
		break;

	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Stormtrooper
-------------------------
*/

void NPC_BehaviorSet_Stormtrooper(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSST_Default();
		break;

	case BS_INVESTIGATE:
		NPC_BSST_Investigate();
		break;

	case BS_SLEEP:
		NPC_BSST_Sleep();
		break;

	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Trooper
-------------------------
*/
void NPC_BehaviorSet_Trooper(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSST_Default();
		break;

	case BS_INVESTIGATE:
		NPC_BSST_Investigate();
		break;

	case BS_SLEEP:
		NPC_BSST_Sleep();
		break;

	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Jedi
-------------------------
*/

static void NPC_BehaviorSet_Jedi(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_INVESTIGATE:
	case BS_DEFAULT:
		NPC_BSJedi_Default();
		break;

	case BS_FOLLOW_LEADER:
		NPC_BSJedi_FollowLeader();
		break;

	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

//qboolean G_JediInNormalAI(const gentity_t* ent)
//{
//	//NOTE: should match above func's switch!
//	//check our b_state
//	const bState_t b_state = G_CurrentBState(ent->NPC);
//	switch (b_state)
//	{
//	case BS_STAND_GUARD:
//	case BS_PATROL:
//	case BS_STAND_AND_SHOOT:
//	case BS_HUNT_AND_KILL:
//	case BS_INVESTIGATE:
//	case BS_DEFAULT:
//	case BS_FOLLOW_LEADER:
//		return qtrue;
//	default: ;
//	}
//	return qfalse;
//}

/*
-------------------------
NPC_BehaviorSet_Droid
-------------------------
*/
static void NPC_BehaviorSet_Droid(const int b_state)
{
	switch (b_state)
	{
	case BS_DEFAULT:
	case BS_STAND_GUARD:
	case BS_PATROL:
		NPC_BSDroid_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Mark1
-------------------------
*/
static void NPC_BehaviorSet_Mark1(const int b_state)
{
	switch (b_state)
	{
	case BS_DEFAULT:
	case BS_STAND_GUARD:
	case BS_PATROL:
		NPC_BSMark1_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Mark2
-------------------------
*/
static void NPC_BehaviorSet_Mark2(const int b_state)
{
	switch (b_state)
	{
	case BS_DEFAULT:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
		NPC_BSMark2_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_ATST
-------------------------
*/
static void NPC_BehaviorSet_ATST(const int b_state)
{
	switch (b_state)
	{
	case BS_DEFAULT:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
		NPC_BSATST_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_MineMonster
-------------------------
*/
static void NPC_BehaviorSet_MineMonster(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSMineMonster_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Howler
-------------------------
*/
static void NPC_BehaviorSet_Howler(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSHowler_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Rancor
-------------------------
*/
static void NPC_BehaviorSet_Rancor(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSRancor_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Wampa
-------------------------
*/
extern void NPC_BSWampa_Default(void);

static void NPC_BehaviorSet_Wampa(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSWampa_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_SandCreature
-------------------------
*/
static void NPC_BehaviorSet_SandCreature(const int b_state)
{
	switch (b_state)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSandCreature_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Droid
-------------------------
*/
// Added 01/21/03 by AReis.
static void NPC_BehaviorSet_Animal(const int b_state)
{
	switch (b_state)
	{
	case BS_DEFAULT:
	case BS_STAND_GUARD:
	case BS_PATROL:
		//NPC_BSAnimal_Default(); //RAFIXME - Impliment.
		NPC_BSDroid_Default();
		break;
	default:
		NPC_BehaviorSet_Default(b_state);
		break;
	}
}

/*
-------------------------
NPC_RunBehavior
-------------------------
*/
extern void NPC_BSRT_Default(void);
extern qboolean RT_Flying(const gentity_t* self);
extern void NPC_BSEmplaced(void);
extern qboolean NPC_CheckSurrender(void);
extern void Boba_FlyStop(gentity_t* self);
extern qboolean Jedi_CultistDestroyer(const gentity_t* self);
extern void bubble_shield_update(void);
extern void NPC_BSSD_Default(void);
extern void NPC_BSCivilian_Default(int b_state);

static void NPC_RunBehavior(const npcteam_t team, const int b_state)
{
	qboolean dontSetAim;

	if (NPCS.NPC->s.NPC_class == CLASS_VEHICLE &&
		NPCS.NPC->m_pVehicle)
	{
		//vehicles don't do AI!
		return;
	}

	if (b_state == BS_CINEMATIC)
	{
		NPC_BSCinematic();
	}
	else if (NPC_JumpBackingUp())
	{
		//racc - NPC is backing up for a large jump
	}
	else if (!TIMER_Done(NPCS.NPC, "DEMP2_StunTime"))
	{
		//we've been stunned by a demp2 shot.
		NPC_UpdateAngles(qtrue, qtrue);
	}
	else if (NPCS.NPC->client->ps.weapon == WP_EMPLACED_GUN)
	{
		NPC_BSEmplaced();
		G_CheckCharmed(NPCS.NPC);
	}
	else if (NPCS.NPC->client->NPC_class == CLASS_HOWLER)
	{
		NPC_BehaviorSet_Howler(b_state);
	}
	else if (Jedi_CultistDestroyer(NPCS.NPC))
	{
		NPC_BSJedi_Default();
		dontSetAim = qtrue;
	}
	else if (NPCS.NPC->client->NPC_class == CLASS_SABER_DROID)
	{
		//saber droid
		NPC_BSSD_Default();
	}
	else if (NPCS.NPC->client->ps.weapon == WP_SABER)
	{
		//jedi
		NPC_BehaviorSet_Jedi(b_state);
		dontSetAim = qtrue;
	}
	else if (NPCS.NPC->client->NPC_class == CLASS_REBORN && NPCS.NPC->client->ps.weapon == WP_MELEE)
	{
		//force-only reborn
		NPC_BehaviorSet_Jedi(b_state);
		dontSetAim = qtrue;
	}
	else if (NPCS.NPC->client->NPC_class == CLASS_JEDI ||
		NPCS.NPC->client->NPC_class == CLASS_REBORN ||
		NPCS.NPC->client->NPC_class == CLASS_TAVION ||
		NPCS.NPC->client->ps.weapon == WP_SABER)
	{
		//jedi
		NPC_BehaviorSet_Jedi(b_state);
	}
	else if (NPCS.NPC->client->NPC_class == CLASS_RANCOR)
	{
		NPC_BehaviorSet_Rancor(b_state);
	}
	else if (NPCS.NPC->client->NPC_class == CLASS_SAND_CREATURE)
	{
		NPC_BehaviorSet_SandCreature(b_state);
	}
	else if (NPCS.NPC->client->NPC_class == CLASS_WAMPA)
	{
		//wampa
		NPC_BSWampa_Default();
		G_CheckCharmed(NPCS.NPC);
	}
	else if (NPCS.NPC->client->NPC_class == CLASS_REMOTE)
	{
		NPC_BehaviorSet_Remote(b_state);
	}
	else if (NPCS.NPC->client->NPC_class == CLASS_SEEKER)
	{
		NPC_BehaviorSet_Seeker(b_state);
	}
	else if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
	{
		//bounty hunter
		//Boba_Update();
		if (Boba_Flying(NPCS.NPC))
		{
			NPC_BehaviorSet_Seeker(b_state);
		}
		else
		{
			NPC_BehaviorSet_Jedi(b_state);
		}
		dontSetAim = qtrue;
	}
	else if (NPCS.NPC->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		//bounty hunter
		if (RT_Flying(NPCS.NPC) || NPCS.NPC->enemy != NULL)
		{
			NPC_BSRT_Default();
		}
		else
		{
			NPC_BehaviorSet_Stormtrooper(b_state);
		}
		G_CheckCharmed(NPCS.NPC);
		dontSetAim = qtrue;
	}
	else if (Jedi_CultistDestroyer(NPCS.NPC))
	{
		NPC_BSJedi_Default();
	}
	else if (NPCS.NPCInfo->scriptFlags & SCF_FORCED_MARCH)
	{
		//being forced to march
		NPC_BSDefault();
	}
	else
	{
		switch (team)
		{
		case (npcteam_t)NPCTEAM_ENEMY:
			// special cases for enemy droids
			switch (NPCS.NPC->client->NPC_class)
			{
			case CLASS_ALORA:
				NPC_BehaviorSet_Jedi(b_state);
				return;
			case CLASS_ATST:
				NPC_BehaviorSet_ATST(b_state);
				return;
			case CLASS_PROBE:
				NPC_BehaviorSet_ImperialProbe(b_state);
				return;
			case CLASS_REMOTE:
				NPC_BehaviorSet_Remote(b_state);
				return;
			case CLASS_SENTRY:
				NPC_BehaviorSet_Sentry(b_state);
				return;
			case CLASS_INTERROGATOR:
				NPC_BehaviorSet_Interrogator(b_state);
				return;
			case CLASS_MINEMONSTER:
				NPC_BehaviorSet_MineMonster(b_state);
				return;
			case CLASS_HOWLER:
				NPC_BehaviorSet_Howler(b_state);
				return;
			case CLASS_RANCOR:
				NPC_BehaviorSet_Rancor(b_state);
				return;
			case CLASS_SAND_CREATURE:
				NPC_BehaviorSet_SandCreature(b_state);
				return;
			case CLASS_SABOTEUR:
			case CLASS_HAZARD_TROOPER:
			case CLASS_REELO:
			case CLASS_COMMANDO:
			case CLASS_IMPERIAL:
			case CLASS_RODIAN:
			case CLASS_TRANDOSHAN:
			{
				if (NPCS.NPC->client->ps.weapon == WP_DISRUPTOR && NPCS.NPCInfo->scriptFlags & SCF_altFire)
				{
					//a sniper
					NPC_BehaviorSet_Sniper(b_state);
					return;
				}
				NPC_BehaviorSet_Stormtrooper(b_state);
				return;
			}
			case CLASS_TUSKEN:
			{
				if (NPCS.NPC->client->ps.weapon == WP_DISRUPTOR /*&& (NPCS.NPCInfo->scriptFlags & SCF_altFire)*/)
				{
					//a sniper
					NPC_BehaviorSet_Sniper(b_state);
					return;
				}
				NPC_BehaviorSet_Stormtrooper(b_state);
				return;
			}
			case CLASS_MARK1:
				NPC_BehaviorSet_Mark1(b_state);
				return;
			case CLASS_MARK2:
				NPC_BehaviorSet_Mark2(b_state);
				return;
			case CLASS_GALAKMECH:
				NPC_BSGM_Default();
				return;
			default:
				break;
			}
			/*if (NPCS.NPC->client->NPC_class == CLASS_ASSASSIN_DROID)
			{
				BubbleShield_Update();
			}*/

			if (NPCS.NPC->client->NPC_class == CLASS_DROIDEKA)
			{
				bubble_shield_update();
			}

			/*if (NPCS.NPC->client->NPC_class == CLASS_GALAKMECH)
			{
				BubbleShield_Update();
			}*/

			if (NPC_IsTrooper(NPCS.NPC))
			{
				NPC_BehaviorSet_Trooper(b_state);
				return;
			}

			if (NPCS.NPC->enemy && NPCS.NPC->s.weapon == WP_NONE && b_state != BS_HUNT_AND_KILL && !trap->
				ICARUS_TaskIDPending(
					(sharedEntity_t*)NPCS.NPC, TID_MOVE_NAV))
			{
				//if in battle and have no weapon, run away, fixme: when in BS_HUNT_AND_KILL, they just stand there
				if (b_state != BS_FLEE)
				{
					NPC_StartFlee(NPCS.NPC->enemy, NPCS.NPC->enemy->r.currentOrigin, AEL_DANGER_GREAT, 5000, 10000);
				}
				else
				{
					NPC_BSFlee();
				}
				return;
			}
			if (NPCS.NPC->client->ps.weapon == WP_SABER)
			{
				//special melee exception
				NPC_BehaviorSet_Default(b_state);
				return;
			}
			if (NPCS.NPC->client->ps.weapon == WP_DISRUPTOR && NPCS.NPCInfo->scriptFlags & SCF_altFire)
			{
				//a sniper
				NPC_BehaviorSet_Sniper(b_state);
				return;
			}
			if (NPCS.NPC->client->ps.weapon == WP_THERMAL || NPCS.NPC->client->ps.weapon == WP_MELEE)
			{
				//a grenadier
				NPC_BehaviorSet_Grenadier(b_state);
				return;
			}
			if (NPC_CheckSurrender())
			{
				return;
			}
			NPC_BehaviorSet_Stormtrooper(b_state);
			break;

		case (npcteam_t)NPCTEAM_NEUTRAL:

			// special cases for enemy droids
			if (NPCS.NPC->client->NPC_class == CLASS_PROTOCOL)
			{
				NPC_BehaviorSet_Default(b_state);
			}
			else if (NPCS.NPC->client->NPC_class == CLASS_UGNAUGHT || NPCS.NPC->client->NPC_class == CLASS_JAWA)
			{
				//others, too?
				NPC_BSCivilian_Default(b_state);
			}
			else if (NPCS.NPC->client->NPC_class == CLASS_VEHICLE)
			{
				const Vehicle_t* pVehicle = NPCS.NPC->m_pVehicle;

				if (!pVehicle->m_pPilot && pVehicle->m_iBoarding == 0)
				{
					//racc - no pilot
					if (pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
					{
						//animal vehicles act like animals while not being rode.
						NPC_BehaviorSet_Animal(b_state);
					}
				}
			}
			else
			{
				// Just one of the average droids
				NPC_BehaviorSet_Droid(b_state);
			}
			break;

		default:
			if (NPCS.NPC->client->NPC_class == CLASS_SEEKER)
			{
				NPC_BehaviorSet_Seeker(b_state);
			}
			else
			{
				if (NPCS.NPCInfo->charmedTime > level.time)
				{
					NPC_BehaviorSet_Charmed(b_state);
				}
				else
				{
					NPC_BehaviorSet_Default(b_state);
				}
				G_CheckCharmed(NPCS.NPC);
				dontSetAim = qtrue;
			}
			break;
		}
	}
}

static bState_t G_CurrentBState(gNPC_t* gNPC)
{
	if (gNPC->tempBehavior != BS_DEFAULT)
	{
		//Overrides normal behavior until cleared
		return gNPC->tempBehavior;
	}

	if (gNPC->behaviorState == BS_DEFAULT)
	{
		gNPC->behaviorState = gNPC->defaultBehavior;
	}

	return gNPC->behaviorState;
}

/*
===============
NPC_ExecuteBState

  MCG

NPC Behavior state thinking

===============
*/
static void NPC_ExecuteBState(const gentity_t* self) //, int msec )
{
	if (self->enemy && self->enemy->client && self->enemy->client->ps.duelInProgress)
	{
		G_ClearEnemy(NPCS.NPC);
	}

	NPC_HandleAIFlags();

	//FIXME: these next three bits could be a function call, some sort of setup/cleanup func
	//Lookmode must be reset every think cycle
	if (NPCS.NPC->delayScriptTime && NPCS.NPC->delayScriptTime <= level.time)
	{
		G_ActivateBehavior(NPCS.NPC, BSET_DELAYED);
		NPCS.NPC->delayScriptTime = 0;
	}

	//Clear this and let b_state set it itself, so it automatically handles changing bStates... but we need a set b_state wrapper func
	NPCS.NPCInfo->combatMove = qfalse;

	//Execute our b_state
	const bState_t b_state = G_CurrentBState(NPCS.NPCInfo);

	//Pick the proper bstate for us and run it
	NPC_RunBehavior(self->client->playerTeam, b_state);

	if (NPCS.NPC->enemy)
	{
		if (!NPCS.NPC->enemy->inuse)
		{
			//just in case b_state doesn't catch this
			G_ClearEnemy(NPCS.NPC);
		}
	}

	if (NPCS.NPC->client->ps.saberLockTime && NPCS.NPC->client->ps.saberLockEnemy != ENTITYNUM_NONE)
	{
		NPC_SetLookTarget(NPCS.NPC, NPCS.NPC->client->ps.saberLockEnemy, level.time + 1000);
	}
	else if (!NPC_CheckLookTarget(NPCS.NPC))
	{
		if (NPCS.NPC->enemy)
		{
			NPC_SetLookTarget(NPCS.NPC, NPCS.NPC->enemy->s.number, 0);
		}
	}

	if (NPCS.NPC->enemy)
	{
		if (NPCS.NPC->enemy->flags & FL_DONT_SHOOT)
		{
			NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
			NPCS.ucmd.buttons &= ~BUTTON_ALT_ATTACK;
		}
		else if (NPCS.NPC->client->playerTeam != NPCTEAM_ENEMY //not an enemy
			&& (NPCS.NPC->client->playerTeam != NPCTEAM_FREE
				|| NPCS.NPC->client->NPC_class == CLASS_TUSKEN && Q_irand(0, 4))
			//not a rampaging creature or I'm a tusken and I feel generous (temporarily)
			&& NPCS.NPC->enemy->NPC
			&& (NPCS.NPC->enemy->NPC->surrenderTime > level.time || NPCS.NPC->enemy->NPC->scriptFlags &
				SCF_FORCED_MARCH))
		{
			//don't shoot someone who's surrendering if you're a good guy
			NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
			NPCS.ucmd.buttons &= ~BUTTON_ALT_ATTACK;
		}

		if (NPCS.client->ps.weaponstate == WEAPON_IDLE)
		{
			NPCS.client->ps.weaponstate = WEAPON_READY;
		}
	}
	else
	{
		if (NPCS.client->ps.weaponstate == WEAPON_READY)
		{
			NPCS.client->ps.weaponstate = WEAPON_IDLE;
		}
	}

	if (!(NPCS.ucmd.buttons & BUTTON_ATTACK) && NPCS.NPC->attackDebounceTime > level.time)
	{
		//We just shot but aren't still shooting, so hold the gun up for a while
		if (NPCS.client->ps.weapon == WP_SABER)
		{
			//One-handed
			NPC_SetAnim(NPCS.NPC, SETANIM_TORSO, BOTH_STAND2, SETANIM_FLAG_NORMAL);
		}
		else if (NPCS.client->ps.weapon == WP_BRYAR_PISTOL)
		{
			//Sniper pose
			if (self->client->NPC_class == CLASS_SBD)
			{
				NPC_SetAnim(NPCS.NPC, SETANIM_TORSO, SBD_WEAPON_OUT_STANDING, SETANIM_FLAG_NORMAL);
			}
			else
			{
				NPC_SetAnim(NPCS.NPC, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_NORMAL);
			}
		}
	}
	else if (!NPCS.NPC->enemy) //HACK!
	{
		if (NPCS.NPC->s.torsoAnim == TORSO_WEAPONREADY1 || NPCS.NPC->s.torsoAnim == TORSO_WEAPONREADY3)
		{
			//we look ready for action, using one of the first 2 weapon, let's rest our weapon on our shoulder
			NPC_SetAnim(NPCS.NPC, SETANIM_TORSO, TORSO_WEAPONIDLE3, SETANIM_FLAG_NORMAL);
		}
	}

	NPC_CheckAttackHold();
	NPC_ApplyScriptFlags();

	//cliff and wall avoidance
	NPC_AvoidWallsAndCliffs();

	// run the bot through the server like it was a real client
	//=== Save the ucmd for the second no-think Pmove ============================
	NPCS.ucmd.serverTime = level.time - 50;
	memcpy(&NPCS.NPCInfo->last_ucmd, &NPCS.ucmd, sizeof(usercmd_t));
	if (!NPCS.NPCInfo->attackHoldTime)
	{
		NPCS.NPCInfo->last_ucmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK);
		//so we don't fire twice in one think
	}
	//============================================================================
	NPC_CheckAttackScript();
	NPC_KeepCurrentFacing();

	if (!NPCS.NPC->next_roff_time || NPCS.NPC->next_roff_time < level.time)
	{
		//If we were following a roff, we don't do normal pmoves.
		ClientThink(NPCS.NPC->s.number, &NPCS.ucmd);
	}
	else
	{
		NPC_ApplyRoff();
	}

	// end of thinking cleanup
	NPCS.NPCInfo->touchedByPlayer = NULL;
}

static void NPC_CheckInSolid(void)
{
	trace_t trace;
	vec3_t point;
	VectorCopy(NPCS.NPC->r.currentOrigin, point);
	point[2] -= 0.25;

	trap->Trace(&trace, NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, point,
		NPCS.NPC->s.number,
		NPCS.NPC->clipmask, qfalse, 0, 0);
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy(NPCS.NPC->r.currentOrigin, NPCS.NPCInfo->lastClearOrigin);
	}
	else
	{
		if (VectorLengthSquared(NPCS.NPCInfo->lastClearOrigin))
		{
			G_SetOrigin(NPCS.NPC, NPCS.NPCInfo->lastClearOrigin);
			trap->LinkEntity((sharedEntity_t*)NPCS.NPC);
		}
	}
}

static void G_DroidSounds(gentity_t* self)
{
	if (self->client)
	{
		//make the noises
		if (TIMER_Done(self, "patrolNoise") && !Q_irand(0, 20))
		{
			switch (self->client->NPC_class)
			{
			case CLASS_R2D2: // droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/r2d2/misc/r2d2talk0%d.wav", Q_irand(1, 3)));
				break;
			case CLASS_R5D2: // droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/r5d2/misc/r5talk%d.wav", Q_irand(1, 4)));
				break;
			case CLASS_PROBE: // droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/probe/misc/probetalk%d.wav", Q_irand(1, 3)));
				break;
			case CLASS_MOUSE: // droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/mouse/misc/mousego%d.wav", Q_irand(1, 3)));
				break;
			case CLASS_GONK: // droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/gonk/misc/gonktalk%d.wav", Q_irand(1, 2)));
				break;
			default:
				break;
			}
			TIMER_Set(self, "patrolNoise", Q_irand(2000, 4000));
		}
	}
}

/*
===============
NPC_Think

Main NPC AI - called once per frame
===============
*/
#if	AI_TIMERS
extern int AITime;
#endif//	AI_TIMERS
void NPC_Think(gentity_t* self)
{
	vec3_t oldMoveDir;
	const int i = 0;

	self->nextthink = level.time + FRAMETIME / 2;

	SetNPCGlobals(self);

	memset(&NPCS.ucmd, 0, sizeof NPCS.ucmd);

	VectorCopy(self->client->ps.moveDir, oldMoveDir);

	if (self->s.NPC_class != CLASS_VEHICLE)
	{
		//YOU ARE BREAKING MY PREDICTION. Bad clear.
		VectorClear(self->client->ps.moveDir);
	}

	if (d_npcfreeze.value || NPCS.NPC->r.svFlags & SVF_ICARUS_FREEZE)
	{
		//our AI is frozen.
		NPC_UpdateAngles(qtrue, qtrue);
		ClientThink(self->s.number, &NPCS.ucmd);
		VectorCopy(self->s.origin, self->s.origin2);
		return;
	}

	if (!self || !self->NPC || !self->client)
	{
		return;
	}

	// dead NPCs have a special think, don't run scripts (for now)
	if (self->health <= 0)
	{
		DeadThink();
		if (NPCS.NPCInfo->nextBStateThink <= level.time)
		{
			trap->ICARUS_MaintainTaskManager(self->s.number);
		}
		return;
	}

	if (self->client
		&& self->client->NPC_class == CLASS_VEHICLE
		&& self->NPC_type
		&& !self->m_pVehicle->m_pVehicleInfo->Inhabited(self->m_pVehicle))
	{
		//empty swoop logic
		if (self->s.owner != ENTITYNUM_NONE)
		{
			//still have attached owner, check and see if can forget him (so he can use me later)
			vec3_t dir2owner;
			const gentity_t* oldOwner = &g_entities[self->s.owner];

			VectorSubtract(g_entities[self->s.owner].r.currentOrigin, self->r.currentOrigin, dir2owner);

			self->s.owner = ENTITYNUM_NONE; //clear here for that SpotWouldTelefrag check...?

			if (VectorLengthSquared(dir2owner) > 128 * 128
				|| !(self->clipmask & oldOwner->clipmask)
				|| DotProduct(self->client->ps.velocity, oldOwner->client->ps.velocity) < -200.0f && !
				G_BoundsOverlap(self->r.absmin, self->r.absmin, oldOwner->r.absmin, oldOwner->r.absmax))
			{
				//all clear, become solid to our owner now
				trap->LinkEntity((sharedEntity_t*)self);
			}
			else
			{
				//blocked, retain owner
				self->s.owner = oldOwner->s.number;
			}
		}
	}
	const gentity_t* player = &g_entities[i];

	if (player->client->ps.viewEntity == self->s.number)
	{
		//being controlled by player
		if (self->client)
		{
			//make the noises
			if (TIMER_Done(self, "patrolNoise") && !Q_irand(0, 20))
			{
				switch (self->client->NPC_class)
				{
				case CLASS_R2D2: // droid
					G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/r2d2/misc/r2d2talk0%d.wav", Q_irand(1, 3)));
					break;
				case CLASS_R5D2: // droid
					G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/r5d2/misc/r5talk%d.wav", Q_irand(1, 4)));
					break;
				case CLASS_PROBE: // droid
					G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/probe/misc/probetalk%d.wav", Q_irand(1, 3)));
					break;
				case CLASS_MOUSE: // droid
					G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/mouse/misc/mousego%d.wav", Q_irand(1, 3)));
					break;
				case CLASS_GONK: // droid
					G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/gonk/misc/gonktalk%d.wav", Q_irand(1, 2)));
					break;
				default:;
				}
				TIMER_Set(self, "patrolNoise", Q_irand(2000, 4000));
			}
		}
		NPCS.NPCInfo->last_ucmd.serverTime = level.time - 50;
		ClientThink(NPCS.NPC->s.number, &NPCS.ucmd);
		VectorCopy(self->s.origin, self->s.origin2);
		return;
	}

	if (NPCS.NPCInfo->nextBStateThink <= level.time)
	{
#if	AI_TIMERS
		int	startTime = GetTime(0);
#endif//	AI_TIMERS
		if (NPCS.NPC->s.eType != ET_NPC)
		{
			//Something drastic happened in our script
			return;
		}

		if (NPCS.NPC->s.weapon == WP_SABER && g_npcspskill.integer >= 2 && NPCS.NPCInfo->rank > RANK_LT_JG)
		{
			//Jedi think faster on hard difficulty, except low-rank (reborn)
			NPCS.NPCInfo->nextBStateThink = level.time + FRAMETIME / 2;
		}
		else
		{
			//Maybe even 200 ms?
			NPCS.NPCInfo->nextBStateThink = level.time + FRAMETIME;
		}

		//nextthink is set before this so something in here can override it
		NPC_ExecuteBState(self);

#if	AI_TIMERS
		int addTime = GetTime(startTime);
		if (addTime > 50)
		{
			gi.Printf(S_COLOR_RED"ERROR: NPC number %d, %s %s at %s, weaponnum: %d, using %d of AI time!!!\n", NPC->s.number, NPC->NPC_type, NPC->targetname, vtos(NPC->currentOrigin), NPC->s.weapon, addTime);
		}
		AITime += addTime;
#endif//	AI_TIMERS
	}
	else
	{
		if (NPCS.NPC->client
			&& NPCS.NPC->client->NPC_class == CLASS_ROCKETTROOPER
			&& NPCS.NPC->client->ps.fd.forceGripBeingGripped > level.time
			&& NPCS.NPC->client->ps.eFlags2 & EF2_FLYING
			&& NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			//reduce velocity
			VectorScale(NPCS.NPC->client->ps.velocity, 0.75f, NPCS.NPC->client->ps.velocity);
		}
		VectorCopy(oldMoveDir, self->client->ps.moveDir);
		//or use client->pers.lastCommand?
		NPCS.NPCInfo->last_ucmd.serverTime = level.time - 50;
		if (!NPCS.NPC->next_roff_time || NPCS.NPC->next_roff_time < level.time)
		{
			//If we were following a roff, we don't do normal pmoves.
			NPC_UpdateAngles(qtrue, qtrue);
			memcpy(&NPCS.ucmd, &NPCS.NPCInfo->last_ucmd, sizeof(usercmd_t));
			ClientThink(NPCS.NPC->s.number, &NPCS.ucmd);
		}
		else
		{
			NPC_ApplyRoff();
		}
		VectorCopy(self->s.origin, self->s.origin2);
	}
	//must update icarus *every* frame because of certain animation completions in the pmove stuff that can leave a 50ms gap between ICARUS animation commands
	trap->ICARUS_MaintainTaskManager(self->s.number);
}

void NPC_InitGame(void)
{
	NPC_LoadParms();
}

void NPC_SetAnim(gentity_t* ent, const int setAnimParts, const int anim, const int setAnimFlags)
{
	G_SetAnim(ent, NULL, setAnimParts, anim, setAnimFlags, 0);
}