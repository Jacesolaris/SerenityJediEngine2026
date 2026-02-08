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

/*****************************************************************************
 * name:		ai_main.c
 *
 * desc:		Quake3 bot AI
 *
 * $Archive: /MissionPack/code/game/ai_main.c $
 * $Author: Osman $
 * $Revision: 1.5 $
 * $Modtime: 6/06/01 1:11p $
 * $Date: 2003/03/15 23:43:59 $
 *
 *****************************************************************************/

#include "g_local.h"
#include "qcommon/q_shared.h"
#include "botlib/botlib.h"		//bot lib interface
#include "botlib/be_aas.h"
 //
#include "ai_main.h"
#include "w_saber.h"
#include <math.h>
#include "g_public.h"
#include <stdlib.h>
//

#define BOT_THINK_TIME	1000/bot_fps.integer

//bot states
bot_state_t* botstates[MAX_CLIENTS];
int walktime[MAX_CLIENTS];
int next_kick[MAX_CLIENTS];
int next_gloat[MAX_CLIENTS];
int next_flourish[MAX_CLIENTS];
#define	APEX_HEIGHT		200.0f
//data stuff
qboolean bot_will_fall[MAX_CLIENTS];
vec3_t jumpPos[MAX_CLIENTS];
int next_bot_fallcheck[MAX_CLIENTS];
int next_bot_vischeck[MAX_CLIENTS];
int last_bot_wp_vischeck_result[MAX_CLIENTS];
vec3_t safe_pos[MAX_CLIENTS];
int last_checkfall[MAX_GENTITIES];
//number of bots
int numbots;
//floating point time
float floattime;
//time to do a regular update
float regularupdate_time;

int bot_get_weapon_range(const bot_state_t* bs);
int pass_loved_one_check(const bot_state_t* bs, const gentity_t* ent);
void ExitLevel(void);
qboolean WP_ForcePowerUsable(const gentity_t* self, forcePowers_t forcePower);
extern qboolean G_HeavyMelee(const gentity_t* attacker);
void siege_defend_from_attackers(bot_state_t* bs);
void request_siege_assistance(bot_state_t* bs, int base_class);
qboolean switch_siege_ideal_class(const bot_state_t* bs, const char* idealclass);
extern int rebel_attackers;
extern int imperial_attackers;
void bot_behave_attack_basic(bot_state_t* bs, const gentity_t* target);
extern void G_SoundOnEnt(gentity_t* ent, soundChannel_t channel, const char* sound_path);
extern qboolean PM_SaberInBounce(int move);
extern qboolean PM_SaberInReturn(int move);
extern qboolean PM_SaberInStart(int move);
extern qboolean PM_SaberInTransition(int move);
extern void NPC_SetAnim(gentity_t* ent, int setAnimParts, int anim, int setAnimFlags);
qboolean AttackLocalBreakable(bot_state_t* bs);
boteventtracker_t gBotEventTracker[MAX_CLIENTS];
extern qboolean G_PointInBounds(vec3_t point, vec3_t mins, vec3_t maxs);
extern siegeClass_t* BG_GetClassOnBaseClass(int team, short classIndex, short cntIndex);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
int numberof_siege_specific_class(int team, const char* classname);
void bot_aim_leading(bot_state_t* bs, vec3_t headlevel, float lead_amount);
float bot_weapon_can_lead(const bot_state_t* bs);
int bot_weapon_detpack(bot_state_t* bs, const gentity_t* target);
void trace_move(bot_state_t* bs, vec3_t move_dir, int target_num);
extern qboolean G_NameInTriggerClassList(const char* list, const char* str);
void bot_behave_defend_basic(bot_state_t* bs, vec3_t defpoint);
int bot_select_choice_weapon(bot_state_t* bs, int weapon, int doselection);
void adjustfor_strafe(const bot_state_t* bs, vec3_t move_dir);
void bot_behave_attack(bot_state_t* bs);
extern const gbuyable_t bg_buylist[];
extern qboolean IsSurrendering(const gentity_t* self);
extern qboolean IsRespecting(const gentity_t* self);
extern qboolean IsCowering(const gentity_t* self);
extern qboolean IsAnimRequiresResponce(const gentity_t* self);
qboolean G_ThereIsAMaster(void);
void bot_behave_attack_move(bot_state_t* bs);

//rww - new bot cvars..
vmCvar_t bot_forcepowers;
vmCvar_t bot_forgimmick;
vmCvar_t bot_honorableduelacceptance;
vmCvar_t bot_pvstype;
vmCvar_t bot_normgpath;
#ifndef FINAL_BUILD
vmCvar_t bot_getinthecarrr;
#endif

#ifdef _DEBUG
vmCvar_t bot_nogoals;
vmCvar_t bot_debugmessages;
#endif

vmCvar_t bot_attachments;
vmCvar_t bot_camp;

vmCvar_t bot_wp_info;
vmCvar_t bot_wp_edit;
vmCvar_t bot_wp_clearweight;
vmCvar_t bot_wp_distconnect;
vmCvar_t bot_wp_visconnect;
vmCvar_t bot_fps;
//end rww

wpobject_t* flagRed;
wpobject_t* oFlagRed;
wpobject_t* flagBlue;
wpobject_t* oFlagBlue;

gentity_t* eFlagRed;
gentity_t* droppedRedFlag;
gentity_t* eFlagBlue;
gentity_t* droppedBlueFlag;

char* ctfStateNames[] = {
	"CTFSTATE_NONE",
	"CTFSTATE_ATTACKER",
	"CTFSTATE_DEFENDER",
	"CTFSTATE_RETRIEVAL",
	"CTFSTATE_GUARDCARRIER",
	"CTFSTATE_GETFLAGHOME",
	"CTFSTATE_MAXCTFSTATES"
};

char* ctfStateDescriptions[] = {
	"I'm not occupied",
	"I'm attacking the enemy's base",
	"I'm defending our base",
	"I'm getting our flag back",
	"I'm escorting our flag carrier",
	"I've got the enemy's flag"
};

char* siegeStateDescriptions[] = {
	"I'm not occupied",
	"I'm attempting to complete the current objective",
	"I'm preventing the enemy from completing their objective"
};

char* teamplayStateDescriptions[] = {
	"I'm not occupied",
	"I'm following my squad commander",
	"I'm assisting my commanding",
	"I'm attempting to regroup and form a new squad"
};

#define DEFEND_MINDISTANCE	200

static int force_jump_needed(vec3_t startvect, vec3_t endvect)
{
	vec3_t tempstart;
	vec3_t tempend;

	const float heightdif = endvect[2] - startvect[2];

	VectorCopy(startvect, tempstart);
	VectorCopy(endvect, tempend);

	tempend[2] = tempstart[2] = 0;
	const float lengthdif = Distance(tempstart, tempend);

	if (heightdif < 128 && lengthdif < 128)
	{
		//no force needed
		return FORCE_LEVEL_0;
	}

	if (heightdif > 512)
	{
		//too high
		return FORCE_LEVEL_4;
	}
	if (heightdif > 350 || lengthdif > 700)
	{
		return FORCE_LEVEL_3;
	}
	if (heightdif > 256 || lengthdif > 512)
	{
		return FORCE_LEVEL_2;
	}
	return FORCE_LEVEL_1;
}

//the amount of Force Power you'll need for a given Force Jump level
int ForcePowerforJump[NUM_FORCE_POWER_LEVELS] =
{
	0,
	30,
	30,
	30
};

int MinimumAttackDistance[WP_NUM_WEAPONS] =
{
	0, //WP_NONE,
	0, //WP_STUN_BATON,
	30, //WP_MELEE,
	60, //WP_SABER,
	200, //WP_BRYAR_PISTOL,
	0, //WP_BLASTER,
	0, //WP_DISRUPTOR,
	0, //WP_BOWCASTER,
	0, //WP_REPEATER,
	0, //WP_DEMP2,
	0, //WP_FLECHETTE,
	100, //WP_ROCKET_LAUNCHER,
	100, //WP_THERMAL,
	100, //WP_TRIP_MINE,
	0, //WP_DET_PACK,
	0, //WP_CONCUSSION,
	0, //WP_BRYAR_OLD,
	0, //WP_EMPLACED_GUN,
	0 //WP_TURRET,
	//WP_NUM_WEAPONS
};

int MaximumAttackDistance[WP_NUM_WEAPONS] =
{
	9999, //WP_NONE,
	0, //WP_STUN_BATON,
	100, //WP_MELEE,
	160, //WP_SABER,
	9999, //WP_BRYAR_PISTOL,
	9999, //WP_BLASTER,
	9999, //WP_DISRUPTOR,
	9999, //WP_BOWCASTER,
	9999, //WP_REPEATER,
	9999, //WP_DEMP2,
	9999, //WP_FLECHETTE,
	9999, //WP_ROCKET_LAUNCHER,
	9999, //WP_THERMAL,
	9999, //WP_TRIP_MINE,
	9999, //WP_DET_PACK,
	9999, //WP_CONCUSSION,
	9999, //WP_BRYAR_OLD,
	9999, //WP_EMPLACED_GUN,
	9999 //WP_TURRET,
	//WP_NUM_WEAPONS
};

int IdealAttackDistance[WP_NUM_WEAPONS] =
{
	0, //WP_NONE,
	0, //WP_STUN_BATON,
	40, //WP_MELEE,
	80, //WP_SABER,
	350, //WP_BRYAR_PISTOL,
	1000, //WP_BLASTER,
	1000, //WP_DISRUPTOR,
	1000, //WP_BOWCASTER,
	1000, //WP_REPEATER,
	1000, //WP_DEMP2,
	1000, //WP_FLECHETTE,
	1000, //WP_ROCKET_LAUNCHER,
	1000, //WP_THERMAL,
	1000, //WP_TRIP_MINE,
	1000, //WP_DET_PACK,
	1000, //WP_CONCUSSION,
	1000, //WP_BRYAR_OLD,
	1000, //WP_EMPLACED_GUN,
	1000 //WP_TURRET,
	//WP_NUM_WEAPONS
};

//Find the origin location of a given entity
static void FindOrigin(const gentity_t* ent, vec3_t origin)
{
	if (!ent->classname)
	{
		//some funky entity, just set vec3_origin
		VectorCopy(vec3_origin, origin);
		return;
	}

	if (ent->client)
	{
		VectorCopy(ent->client->ps.origin, origin);
	}
	else
	{
		if (strcmp(ent->classname, "func_breakable") == 0
			|| strcmp(ent->classname, "trigger_multiple") == 0
			|| strcmp(ent->classname, "trigger_once") == 0
			|| strcmp(ent->classname, "func_usable") == 0)
		{
			//func_breakable's don't have normal origin data
			origin[0] = (ent->r.absmax[0] + ent->r.absmin[0]) / 2;
			origin[1] = (ent->r.absmax[1] + ent->r.absmin[1]) / 2;
			origin[2] = (ent->r.absmax[2] + ent->r.absmin[2]) / 2;
		}
		else
		{
			VectorCopy(ent->r.currentOrigin, origin);
		}
	}
}

//find angles/viewangles for entity
static void find_angles(const gentity_t* ent, vec3_t angles)
{
	if (ent->client)
	{
		//player
		VectorCopy(ent->client->ps.viewangles, angles);
	}
	else
	{
		//other stuff
		VectorCopy(ent->s.angles, angles);
	}
}

static float TargetDistance(const bot_state_t* bs, const gentity_t* target, vec3_t targetorigin)
{
	if (strcmp(target->classname, "misc_siege_item") == 0
		|| strcmp(target->classname, "func_breakable") == 0
		|| target->client && target->client->NPC_class == CLASS_RANCOR)
	{
		vec3_t enemy_origin;
		//flatten origin heights and measure
		VectorCopy(targetorigin, enemy_origin);
		if (fabs(enemy_origin[2] - bs->eye[2]) < 150)
		{
			//don't flatten unless you're on the same relative plane
			enemy_origin[2] = bs->eye[2];
		}

		if (target->client && target->client->NPC_class == CLASS_RANCOR)
		{
			//Rancors are big and stuff
			return Distance(bs->eye, enemy_origin) - 60;
		}
		if (strcmp(target->classname, "misc_siege_item") == 0)
		{
			//assume this is a misc_siege_item.  These have absolute based mins/maxs.
			//Scale for the entity's bounding box
			float adjustor;
			const float x = fabs(bs->eye[0] - enemy_origin[0]);
			const float y = fabs(bs->eye[1] - enemy_origin[1]);
			const float z = fabs(bs->eye[2] - enemy_origin[2]);

			//find the general direction of the impact to determine which bbox length to
			//scale with
			if (x > y && x > z)
			{
				//x
				adjustor = target->r.maxs[0];
			}
			else if (y > x && y > z)
			{
				//y
				adjustor = target->r.maxs[1];
			}
			else
			{
				//z
				adjustor = target->r.maxs[2];
			}

			return Distance(bs->eye, enemy_origin) - adjustor + 15;
		}
		if (strcmp(target->classname, "func_breakable") == 0)
		{
			//Scale for the entity's bounding box
			float adjustor;

			//find the smallest min/max and use that.
			if (target->r.absmax[0] - enemy_origin[0] < target->r.absmax[1] - enemy_origin[1])
			{
				adjustor = target->r.absmax[0] - enemy_origin[0];
			}
			else
			{
				adjustor = target->r.absmax[1] - enemy_origin[1];
			}

			return Distance(bs->eye, enemy_origin) - adjustor + 15;
		}
		//func_breakable
		return Distance(bs->eye, enemy_origin);
	}
	//standard stuff
	return Distance(bs->eye, targetorigin);
}

static qboolean visible(const gentity_t* self, const gentity_t* other)
{
	vec3_t spot1, spot2;
	trace_t tr;

	if (!self || !other || !self->inuse || !other->inuse)
		return qfalse;
	if (!self->client || !other->client)
		return qfalse;

	VectorCopy(self->r.currentOrigin, spot1);
	spot1[2] += 48;
	VectorCopy(other->r.currentOrigin, spot2);
	spot2[2] += 48;

	// Standard visibility trace
	trap->Trace(&tr, spot1, NULL, NULL, other->r.currentOrigin, self->s.number, MASK_SHOT, qfalse, 0, 0);

	if (tr.entityNum >= 0 && tr.entityNum < level.num_entities)
	{
		const gentity_t* traceEnt = &g_entities[tr.entityNum];
		if (traceEnt == other)
			return qtrue;
	}
	return qfalse;
}

static gentity_t* find_closest_human_player(vec3_t position, const int enemy_team)
{
	float bestdist = 9999;
	gentity_t* closestplayer = NULL;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t* player = &g_entities[i];
		if (!player || !player->client || !player->inuse)
		{
			//player not active
			continue;
		}

		if (player->r.svFlags & SVF_BOT)
		{
			//player bot.
			continue;
		}

		if (player->client->playerTeam != enemy_team)
		{
			//this player isn't on the team I hate.
			continue;
		}

		if (player->health <= 0 || player->s.eFlags & EF_DEAD)
		{
			//player is dead, don t use
			continue;
		}

		const float dist = Distance(player->client->ps.origin, position);
		if (dist < bestdist)
		{
			closestplayer = player;
			bestdist = dist;
		}
	}

	return closestplayer;
}

static float vector_distancebot(vec3_t v1, vec3_t v2)
{
	vec3_t dir = {0, 0, 0};

	VectorSubtract(v2, v1, dir);
	return VectorLength(dir);
}

static qboolean check_fall_by_vectors(vec3_t origin, vec3_t angles, const gentity_t* ent)
{
	// Check a little in front of us.
	trace_t tr;
	vec3_t down, flat_ang, fwd, use;

	if (last_checkfall[ent->s.number] > level.time - 250) // Only check every 1/4 sec.
	{
		return qtrue;
	}

	last_checkfall[ent->s.number] = level.time;

	VectorCopy(angles, flat_ang);
	flat_ang[PITCH] = 0;

	AngleVectors(flat_ang, fwd, 0, 0);

	use[0] = origin[0] + fwd[0] * 96;
	use[1] = origin[1] + fwd[1] * 96;
	use[2] = origin[2] + fwd[2] * 96;

	VectorCopy(use, down);

	down[2] -= 4096;

	trap->Trace(&tr, use, ent->r.mins, ent->r.maxs, down, ent->s.clientNum, MASK_SOLID, qfalse, 0, 0);
	// Look for ground.

	VectorSubtract(use, tr.endpos, down);

	if (down[2] >= 1000)
		return qtrue; // Long way down!

	use[0] = origin[0] + fwd[0] * 64;
	use[1] = origin[1] + fwd[1] * 64;
	use[2] = origin[2] + fwd[2] * 64;

	VectorCopy(use, down);

	down[2] -= 4096;

	trap->Trace(&tr, use, ent->r.mins, ent->r.maxs, down, ent->s.clientNum, MASK_SOLID, qfalse, 0, 0);
	// Look for ground.

	VectorSubtract(use, tr.endpos, down);

	if (down[2] >= 1000)
		return qtrue; // Long way down!

	use[0] = origin[0] + fwd[0] * 32;
	use[1] = origin[1] + fwd[1] * 32;
	use[2] = origin[2] + fwd[2] * 32;

	VectorCopy(use, down);

	down[2] -= 4096;

	trap->Trace(&tr, use, ent->r.mins, ent->r.maxs, down, ent->s.clientNum, MASK_SOLID, qfalse, 0, 0);
	// Look for ground.

	VectorSubtract(use, tr.endpos, down);

	if (down[2] >= 1000)
		return qtrue; // Long way down!

	return qfalse; // All is ok!
}

static void set_enemy_path(bot_state_t* bs)
{
	// For aborted jumps.
	vec3_t fwd, b_angle;

	bs->goalAngles[PITCH] = 0;
	bs->goalAngles[ROLL] = 0;

	VectorCopy(bs->goalAngles, b_angle);

	AngleVectors(b_angle, fwd, NULL, NULL);

	if (!bs->currentEnemy // No enemy!
		|| check_fall_by_vectors(bs->origin, fwd, &g_entities[bs->entityNum]) == qtrue) // We're gonna fall!!!
	{
		if (bs->wpCurrent)
		{
			// Fall back to the old waypoint info... Should be already set.
		}
		else
		{
			bs->beStill = level.time + 1000;
		}
	}
	else
	{
		// Set enemy as our goal.
		VectorCopy(bs->currentEnemy->r.currentOrigin, bs->goalPosition);
	}
}



// ------------------------------------------------------------
// Jetpack helper (C89-safe)
// ------------------------------------------------------------
static void AI_EnableJetpack(bot_state_t* bs)
{
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
	{
		bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
		bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
		bs->cur_ps.eFlags |= EF3_JETPACK_HOVER;
		bs->jumpHoldTime = level.time + 50000;
	}
}

// ------------------------------------------------------------
// Predict where enemy will be after 't' seconds
// ------------------------------------------------------------
static void AI_PredictEnemyPos(const bot_state_t* bs, vec3_t out, float t)
{
	VectorCopy(bs->currentEnemy->r.currentOrigin, out);

	if (!bs->currentEnemy->client)
		return;

	// Simple linear prediction using enemy velocity
	VectorMA(out, t, bs->currentEnemy->client->ps.velocity, out);
}

// ------------------------------------------------------------
// Try to find a wall in front for wall-jump / parkour
// ------------------------------------------------------------
static qboolean AI_FindWallAhead(gentity_t* bot, vec3_t wallNormal)
{
	trace_t tr;
	vec3_t fwd, end;
	vec3_t ang;

	VectorCopy(bot->client->ps.viewangles, ang);
	ang[PITCH] = 0;
	AngleVectors(ang, fwd, NULL, NULL);

	VectorMA(bot->r.currentOrigin, 64.0f, fwd, end);

	trap->Trace(&tr, bot->r.currentOrigin, bot->r.mins, bot->r.maxs,
		end, bot->s.number, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction < 1.0f && !tr.startsolid)
	{
		VectorCopy(tr.plane.normal, wallNormal);
		return qtrue;
	}

	return qfalse;
}

// ------------------------------------------------------------
// Core ballistic jump solver: from 'start' to 'apex'
// ------------------------------------------------------------
static qboolean AI_ComputeBallisticJump(gentity_t* bot,
	const vec3_t start,
	const vec3_t apex,
	vec3_t outVel)
{
	float height = apex[2] - start[2];
	float time;
	vec3_t flat;

	if (height <= 0.0f)
		height = 1.0f;

	time = sqrtf(height / (0.5f * bot->client->ps.gravity));
	if (time <= 0.0f)
		return qfalse;

	VectorSubtract(apex, start, flat);
	flat[2] = 0.0f;

	if (VectorNormalize(flat) == 0.0f)
		return qfalse;

	// Horizontal speed
	{
		float dist = VectorLength(flat);
		float forward = dist / time;

		VectorScale(flat, forward, outVel);
	}

	// Vertical speed
	outVel[2] = time * bot->client->ps.gravity;

	return qtrue;
}

static void ai_mod_jump(bot_state_t* bs)
{
	gentity_t* bot = &g_entities[bs->entityNum];

	if (!bs->currentEnemy || !bs->currentEnemy->inuse)
	{
		bs->BOTjumpState = JS_WAITING;
		return;
	}

	// Clear movement input before we mess with velocity
	bot->client->pers.cmd.forwardmove = 0;
	bot->client->pers.cmd.rightmove = 0;
	bot->client->pers.cmd.upmove = 0;

	switch (bs->BOTjumpState)
	{
	case JS_FACING:
	case JS_CROUCHING:
	{
		vec3_t start, targetPos, apex, dir;
		float distXY, apexHeight;

		// Predict where enemy will be shortly
		VectorCopy(bot->r.currentOrigin, start);
		AI_PredictEnemyPos(bs, targetPos, 0.6f);

		// Decide: normal jump vs jetpack long jump
		VectorSubtract(targetPos, start, dir);
		dir[2] = 0.0f;
		distXY = VectorNormalize(dir);

		// Try wall-jump / parkour if close to a wall
		{
			vec3_t wallNormal;
			if (AI_FindWallAhead(bot, wallNormal))
			{
				// Push off the wall: velocity away + up
				vec3_t push;
				VectorScale(wallNormal, 300.0f, push);
				push[2] = 300.0f;

				VectorCopy(push, bot->client->ps.velocity);
				NPC_SetAnim(bot, SETANIM_BOTH, BOTH_JUMP1, SETANIM_FLAG_OVERRIDE);
				bs->BOTjumpState = JS_JUMPING;
				bot->flags |= FL_NO_KNOCKBACK;
				return;
			}
		}

		// Long jump? Use jetpack assist if far
		if (distXY > 600.0f)
		{
			AI_EnableJetpack(bs);

			// Aim higher and farther
			apexHeight = 256.0f;
		}
		else
		{
			apexHeight = 192.0f;
		}

		// Build apex point between us and predicted enemy
		VectorMA(start, distXY * 0.5f, dir, apex);
		apex[2] = start[2] + apexHeight;

		{
			vec3_t vel;
			if (!AI_ComputeBallisticJump(bot, start, apex, vel))
			{
				bs->BOTjumpState = JS_WAITING;
				return;
			}

			VectorCopy(vel, bot->client->ps.velocity);
			NPC_SetAnim(bot, SETANIM_BOTH, BOTH_JUMP1, SETANIM_FLAG_OVERRIDE);
			bot->flags |= FL_NO_KNOCKBACK;
			bs->BOTjumpState = JS_JUMPING;
		}
	}
	break;

	case JS_JUMPING:
	{
		// Keep jetpack assist if we have it
		AI_EnableJetpack(bs);

		// If we hit ground, transition to landing
		if (bot->s.groundEntityNum != ENTITYNUM_NONE)
		{
			VectorClear(bot->client->ps.velocity);
			NPC_SetAnim(bot, SETANIM_BOTH, BOTH_LAND1,
				SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			bs->BOTjumpState = JS_LANDING;
			return;
		}

		// If jump anim finished but still in air, use in-air anim
		if (bot->client->ps.legsTimer <= 0)
		{
			NPC_SetAnim(bot, SETANIM_BOTH, BOTH_INAIR1, SETANIM_FLAG_OVERRIDE);
		}
	}
	break;

	case JS_LANDING:
	{
		// Optional: small chance to hover a bit on landing if jetpacker
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK) &&
			Q_irand(0, 5) < 2)
		{
			AI_EnableJetpack(bs);
		}

		if (bot->client->ps.legsTimer > 0)
			return;

		bs->BOTjumpState = JS_WAITING;
		bot->flags &= ~FL_NO_KNOCKBACK;
	}
	break;

	case JS_WAITING:
		// Idle state; allow hover if jetpacker
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
		{
			AI_EnableJetpack(bs);
		}
		break;

	default:
		// Reset to facing state
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
		{
			AI_EnableJetpack(bs);
		}
		bs->BOTjumpState = JS_FACING;
		break;
	}
}

static void bot_straight_tp_order_check(gentity_t* ent, const int ordernum, bot_state_t* bs)
{
	switch (ordernum)
	{
	case 0:
		if (bs->squadLeader == ent)
		{
			bs->teamplayState = 0;
			bs->squadLeader = NULL;
		}
		break;
	case TEAMPLAYSTATE_FOLLOWING:
		bs->teamplayState = ordernum;
		bs->isSquadLeader = 0;
		bs->squadLeader = ent;
		bs->wpDestSwitchTime = 0;
		break;
	case TEAMPLAYSTATE_ASSISTING:
		bs->teamplayState = ordernum;
		bs->isSquadLeader = 0;
		bs->squadLeader = ent;
		bs->wpDestSwitchTime = 0;
		break;
	default:
		bs->teamplayState = ordernum;
		break;
	}
}

static void bot_select_weapon(const int client, const int weapon)
{
	if (weapon <= WP_NONE)
	{
		return;
	}
	trap->EA_SelectWeapon(client, weapon);
}

static void BotReportStatus(const bot_state_t* bs)
{
	if (level.gametype == GT_TEAM)
	{
		trap->EA_SayTeam(bs->client, teamplayStateDescriptions[bs->teamplayState]);
	}
	else if (level.gametype == GT_SIEGE)
	{
		trap->EA_SayTeam(bs->client, siegeStateDescriptions[bs->siegeState]);
	}
	else if (level.gametype == GT_CTF || level.gametype == GT_CTY)
	{
		trap->EA_SayTeam(bs->client, ctfStateDescriptions[bs->ctfState]);
	}
}

//accept a team order from a player
void bot_order(gentity_t* ent, const int clientNum, const int ordernum)
{
	int state_min = 0;
	int state_max = 0;

	if (!ent || !ent->client || !ent->client->sess.teamLeader)
	{
		return;
	}

	if (clientNum != -1 && !botstates[clientNum])
	{
		return;
	}

	if (clientNum != -1 && !OnSameTeam(ent, &g_entities[clientNum]))
	{
		return;
	}

	if (level.gametype != GT_CTF && level.gametype != GT_CTY && level.gametype != GT_SIEGE && level.gametype != GT_TEAM)
	{
		return;
	}

	if (level.gametype == GT_CTF || level.gametype == GT_CTY)
	{
		state_min = CTFSTATE_NONE;
		state_max = CTFSTATE_MAXCTFSTATES;
	}
	else if (level.gametype == GT_SIEGE)
	{
		state_min = SIEGESTATE_NONE;
		state_max = SIEGESTATE_MAXSIEGESTATES;
	}
	else if (level.gametype == GT_TEAM)
	{
		state_min = TEAMPLAYSTATE_NONE;
		state_max = TEAMPLAYSTATE_MAXTPSTATES;
	}

	if (ordernum < state_min && ordernum != -1 || ordernum >= state_max)
	{
		return;
	}

	if (clientNum != -1)
	{
		if (ordernum == -1)
		{
			BotReportStatus(botstates[clientNum]);
		}
		else
		{
			bot_straight_tp_order_check(ent, ordernum, botstates[clientNum]);
			botstates[clientNum]->state_Forced = ordernum;
			botstates[clientNum]->chatObject = ent;
			botstates[clientNum]->chatAltObject = NULL;
			if (BotDoChat(botstates[clientNum], "OrderAccepted", 1))
			{
				botstates[clientNum]->chatTeam = 1;
			}
		}
	}
	else
	{
		int i = 0;
		while (i < MAX_CLIENTS)
		{
			if (botstates[i] && OnSameTeam(ent, &g_entities[i]))
			{
				if (ordernum == -1)
				{
					BotReportStatus(botstates[i]);
				}
				else
				{
					bot_straight_tp_order_check(ent, ordernum, botstates[i]);
					botstates[i]->state_Forced = ordernum;
					botstates[i]->chatObject = ent;
					botstates[i]->chatAltObject = NULL;
					if (BotDoChat(botstates[i], "OrderAccepted", 0))
					{
						botstates[i]->chatTeam = 1;
					}
				}
			}

			i++;
		}
	}
}

//See if bot is mind tricked by the client in question
static int bot_mind_tricked(const int bot_client, const int enemy_client)
{
	if (!g_entities[enemy_client].client)
	{
		return 0;
	}

	const forcedata_t* fd = &g_entities[enemy_client].client->ps.fd;

	if (!fd)
	{
		return 0;
	}

	if (bot_client > 47)
	{
		if (fd->forceMindtrickTargetIndex4 & (1 << (bot_client - 48)))
		{
			return 1;
		}
	}
	else if (bot_client > 31)
	{
		if (fd->forceMindtrickTargetIndex3 & (1 << (bot_client - 32)))
		{
			return 1;
		}
	}
	else if (bot_client > 15)
	{
		if (fd->forceMindtrickTargetIndex2 & (1 << (bot_client - 16)))
		{
			return 1;
		}
	}
	else
	{
		if (fd->forceMindtrickTargetIndex & (1 << bot_client))
		{
			return 1;
		}
	}

	return 0;
}

static int is_teamplay(void)
{
	if (level.gametype < GT_TEAM)
	{
		return 0;
	}

	return 1;
}

/*
==================
BotAI_GetClientState
==================
*/
static int bot_ai_get_client_state(const int clientNum, playerState_t* state)
{
	const gentity_t* ent = &g_entities[clientNum];
	if (!ent->inuse)
	{
		return qfalse;
	}
	if (!ent->client)
	{
		return qfalse;
	}

	memcpy(state, &ent->client->ps, sizeof(playerState_t));
	return qtrue;
}

/*
==================
BotAI_GetEntityState
==================
*/
static int bot_ai_get_entity_state(const int entityNum, entityState_t* state)
{
	const gentity_t* ent = &g_entities[entityNum];
	memset(state, 0, sizeof(entityState_t));
	if (!ent->inuse) return qfalse;
	if (!ent->r.linked) return qfalse;
	if (ent->r.svFlags & SVF_NOCLIENT) return qfalse;
	memcpy(state, &ent->s, sizeof(entityState_t));
	return qtrue;
}

/*
==================
BotAI_GetSnapshotEntity
==================
*/
static int BotAI_GetSnapshotEntity(const int clientNum, const int sequence, entityState_t* state)
{
	const int entNum = trap->BotGetSnapshotEntity(clientNum, sequence);
	if (entNum == -1)
	{
		memset(state, 0, sizeof(entityState_t));
		return -1;
	}

	bot_ai_get_entity_state(entNum, state);

	return sequence + 1;
}

/*
==============
BotEntityInfo
==============
*/
static void BotEntityInfo(const int entnum, aas_entityinfo_t* info)
{
	trap->AAS_EntityInfo(entnum, info);
}

/*
==============
AngleDifference
==============
*/
static float angle_difference(const float ang1, const float ang2)
{
	float diff = ang1 - ang2;
	if (ang1 > ang2)
	{
		if (diff > 180.0) diff -= 360.0;
	}
	else
	{
		if (diff < -180.0) diff += 360.0;
	}
	return diff;
}

/*
==============
BotChangeViewAngle
==============
*/
static float bot_change_view_angle(float angle, float ideal_angle, const float speed)
{
	angle = AngleMod(angle);
	ideal_angle = AngleMod(ideal_angle);

	// Compute shortest angular difference in range [-180, 180]
	float delta = AngleMod(ideal_angle - angle);
	if (delta > 180.0f)
		delta -= 360.0f;

	// Clamp to turn speed
	if (delta > speed)
		delta = speed;
	else if (delta < -speed)
		delta = -speed;

	return AngleMod(angle + delta);
}

/*
==============
BotChangeViewAngles
==============
*/
static void bot_change_view_angles(bot_state_t* bs, float thinktime)
{
	float factor;

	// Normalize pitch
	if (bs->ideal_viewangles[PITCH] > 180.0f)
		bs->ideal_viewangles[PITCH] -= 360.0f;

	// Combat vs idle turn speed
	if (bs->currentEnemy && bs->frame_Enemy_Vis)
		factor = bs->skills.turnspeed_combat * (0.4f + 0.2f * bs->settings.skill);
	else
		factor = bs->skills.turnspeed;

	// Clamp factor
	if (factor < 0.001f) factor = 0.001f;
	if (factor > 1.0f)   factor = 1.0f;

	float maxchange = bs->skills.maxturn * thinktime;

	for (int i = 0; i < 2; i++)
	{
		float cur = AngleMod(bs->viewangles[i]);
		float ideal = AngleMod(bs->ideal_viewangles[i]);

		// Shortest angular difference [-180, 180]
		float diff = AngleMod(ideal - cur);
		if (diff > 180.0f) diff -= 360.0f;

		// Desired angular velocity
		float desired_speed = diff * factor;

		// Smooth velocity convergence
		float speed = bs->viewanglespeed[i];
		speed += (desired_speed - speed) * 0.5f;   // critically damped

		// Clamp to max turn rate
		if (speed > maxchange) speed = maxchange;
		if (speed < -maxchange) speed = -maxchange;

		// Apply
		cur += speed;
		bs->viewangles[i] = AngleMod(cur);
		bs->viewanglespeed[i] = speed;
	}

	// Normalize pitch again
	if (bs->viewangles[PITCH] > 180.0f)
		bs->viewangles[PITCH] -= 360.0f;

	trap->EA_View(bs->client, bs->viewangles);
}

/*
==============
BotInputToUserCommand
==============
*/
static void bot_input_to_user_command(bot_input_t* bi, usercmd_t* ucmd, int delta_angles[3], const int time,
	const int use_time)
{
	vec3_t angles, forward, right;

	//clear the whole structure
	memset(ucmd, 0, sizeof(usercmd_t));
	//the duration for the user command in milli seconds
	ucmd->serverTime = time;
	//
	if (bi->actionflags & ACTION_DELAYEDJUMP)
	{
		bi->actionflags |= ACTION_JUMP;
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	}
	//set the buttons
	if (bi->actionflags & ACTION_RESPAWN) ucmd->buttons = BUTTON_ATTACK;
	if (bi->actionflags & ACTION_ATTACK) ucmd->buttons |= BUTTON_ATTACK;
	if (bi->actionflags & ACTION_ALT_ATTACK) ucmd->buttons |= BUTTON_ALT_ATTACK;
	if (bi->actionflags & ACTION_GESTURE) ucmd->buttons |= BUTTON_GESTURE;
	if (bi->actionflags & ACTION_USE) ucmd->buttons |= BUTTON_USE_HOLDABLE;
	if (bi->actionflags & ACTION_WALK) ucmd->buttons |= BUTTON_WALKING;
	if (bi->actionflags & ACTION_FORCEPOWER) ucmd->buttons |= BUTTON_FORCEPOWER;
	if (bi->actionflags & ACTION_KICK) ucmd->buttons |= BUTTON_KICK;
	if (bi->actionflags & ACTION_USE) ucmd->buttons |= BUTTON_BOTUSE;
	if (bi->actionflags & ACTION_BLOCK) ucmd->buttons |= BUTTON_BLOCK;
	if (bi->actionflags & ACTION_GLOAT) ucmd->buttons |= BUTTON_GLOAT;
	if (bi->actionflags & ACTION_FLOURISH) ucmd->buttons |= BUTTON_FLOURISH;

	if (use_time < level.time && Q_irand(1, 10) < 5)
	{
		//for now just hit use randomly in case there's something useable around
		ucmd->buttons |= BUTTON_BOTUSE;
	}

	if (bi->weapon == WP_NONE)
	{
		bi->weapon = WP_BRYAR_PISTOL;
	}

	//
	ucmd->weapon = bi->weapon;
	//set the view angles
	//NOTE: the ucmd->angles are the angles WITHOUT the delta angles
	ucmd->angles[PITCH] = ANGLE2SHORT(bi->viewangles[PITCH]);
	ucmd->angles[YAW] = ANGLE2SHORT(bi->viewangles[YAW]);
	ucmd->angles[ROLL] = ANGLE2SHORT(bi->viewangles[ROLL]);
	//subtract the delta angles
	for (int j = 0; j < 3; j++)
	{
		const short temp = ucmd->angles[j] - delta_angles[j];
		ucmd->angles[j] = temp;
	}
	//NOTE: movement is relative to the REAL view angles
	//get the horizontal forward and right vector
	//get the pitch in the range [-180, 180]
	if (bi->dir[2]) angles[PITCH] = bi->viewangles[PITCH];
	else angles[PITCH] = 0;
	angles[YAW] = bi->viewangles[YAW];
	angles[ROLL] = 0;
	AngleVectors(angles, forward, right, NULL);

	//bot input speed is in the range [0, 400]
	bi->speed = bi->speed * 127 / 400;

	//set the view independent movement
	float f = DotProduct(forward, bi->dir);
	float r = DotProduct(right, bi->dir);
	float u = fabs(forward[2]) * bi->dir[2];
	float m = fabs(f);

	if (fabs(r) > m)
	{
		m = fabs(r);
	}

	if (fabs(u) > m)
	{
		m = fabs(u);
	}

	if (m > 0)
	{
		f *= bi->speed / m;
		r *= bi->speed / m;
		u *= bi->speed / m;
	}

	ucmd->forwardmove = f;
	ucmd->rightmove = r;
	ucmd->upmove = u;
	//normal keyboard movement
	if (bi->actionflags & ACTION_MOVEFORWARD) ucmd->forwardmove = 127;
	if (bi->actionflags & ACTION_MOVEBACK) ucmd->forwardmove = -127;
	if (bi->actionflags & ACTION_MOVELEFT) ucmd->rightmove = -127;
	if (bi->actionflags & ACTION_MOVERIGHT) ucmd->rightmove = 127;
	//jump/moveup
	if (bi->actionflags & ACTION_JUMP) ucmd->upmove = 127;
	//crouch/movedown
	if (bi->actionflags & ACTION_CROUCH) ucmd->upmove = -127;

	if (bi->actionflags & ACTION_WALK)
	{
		if (ucmd->forwardmove > 46)
		{
			ucmd->forwardmove = 46;
		}
		else if (ucmd->forwardmove < -46)
		{
			ucmd->forwardmove = -46;
		}

		if (ucmd->rightmove > 46)
		{
			ucmd->rightmove = 46;
		}
		else if (ucmd->rightmove < -46)
		{
			ucmd->rightmove = -46;
		}
	}
	ucmd->forcesel = bi->forcesel;
}

/*
==============
BotUpdateInput
==============
*/
static void bot_apply_delta_angles(bot_state_t* bs)
{
	for (int j = 0; j < 3; j++)
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
}

static void bot_unapply_delta_angles(bot_state_t* bs)
{
	for (int j = 0; j < 3; j++)
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
}

static qboolean bot_try_timed_action(
	bot_state_t* bs,
	int* next_time,
	int base_ms,
	float range,
	int action_flag)
{
	if (bot_thinklevel.integer < 0)
		return qfalse;

	const int now = level.time;

	// Cooldown active ? allow continuation
	if (*next_time > now)
		return qtrue;

	// No enemy ? reset
	if (!bs->currentEnemy ||
		!bs->currentEnemy->client ||
		bs->currentEnemy->health <= 0 ||
		bs->currentEnemy->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		*next_time = 0;
		return qfalse;
	}

	// Distance check
	const float dist = VectorDistance(
		g_entities[bs->cur_ps.clientNum].r.currentOrigin,
		bs->currentEnemy->r.currentOrigin);

	if (dist >= range)
	{
		*next_time = 0;
		return qfalse;
	}

	// Visibility check
	if (!visible(&g_entities[bs->cur_ps.clientNum], bs->currentEnemy))
	{
		*next_time = 0;
		return qfalse;
	}

	// Passed all checks ? schedule next time
	int check_val = bot_thinklevel.integer;
	if (check_val <= 0)
		check_val = 1;

	*next_time = now + base_ms / check_val;
	return qtrue;
}

static qboolean bot_should_walk_saber(bot_state_t* bs, bot_input_t* bi)
{
	const int client = bs->cur_ps.clientNum;

	// Saber only
	if (bs->cur_ps.weapon != WP_SABER)
		return qfalse;

	const gentity_t* self = &g_entities[client];
	const gentity_t* enemy = bs->currentEnemy;

	// Validate enemy before doing ANY distance or visibility checks
	if (!enemy || !enemy->client || enemy->health <= 0)
	{
		walktime[client] = 0;
		bi->actionflags &= ~ACTION_WALK;
		return qfalse;
	}

	// Don’t walk if we’re mid‑jump
	if (bs->jumpTime > level.time)
	{
		walktime[client] = 0;
		bi->actionflags &= ~ACTION_WALK;
		return qfalse;
	}

	// Now safe to compute distance
	const float dist = Distance(self->r.currentOrigin, enemy->r.currentOrigin);

	const qboolean closeEnough =
		(dist < 300.0f) ||
		(walktime[client] > level.time);

	if (!closeEnough)
	{
		walktime[client] = 0;
		bi->actionflags &= ~ACTION_WALK;
		return qfalse;
	}

	// Visibility check
	if (visible(self, enemy) || walktime[client] > level.time)
	{
		bi->actionflags |= ACTION_WALK;
		walktime[client] = level.time + 2000;

		// Auto‑unholster saber
		if (bs->cur_ps.saberHolstered)
			bs->cur_ps.saberHolstered = 0;

		return qtrue;
	}

	// Not visible → stop walking
	walktime[client] = 0;
	bi->actionflags &= ~ACTION_WALK;
	return qfalse;
}

static qboolean bot_in_saber_engagement(bot_state_t* bs)
{
	if (bs->cur_ps.weapon != WP_SABER)
		return qfalse;

	gentity_t* self = &g_entities[bs->cur_ps.clientNum];
	gentity_t* enemy = bs->currentEnemy;

	if (!enemy || !enemy->client || enemy->health <= 0)
		return qfalse;

	if (bs->jumpTime > level.time)
		return qfalse;

	if (!visible(self, enemy))
		return qfalse;

	if (Distance(self->r.currentOrigin, enemy->r.currentOrigin) >= 300.0f)
		return qfalse;

	return qtrue;
}

static void bot_update_input(bot_state_t* bs, const int time, const int elapsed_time)
{
	bot_input_t bi;
	const int client = bs->cur_ps.clientNum;

	// Apply delta angles
	bot_apply_delta_angles(bs);

	// View angle update
	const float t = (bs->cur_ps.weapon == WP_SABER)
		? (2.0f * (float)elapsed_time / 1000.0f)
		: ((float)elapsed_time / 1000.0f);

	bot_change_view_angles(bs, t);

	// Retrieve bot input
	trap->EA_GetInput(bs->client, (float)time / 1000.0f, &bi);

	// Respawn hack
	if (bi.actionflags & ACTION_RESPAWN)
	{
		if (bs->lastucmd.buttons & BUTTON_ATTACK)
			bi.actionflags &= ~(ACTION_RESPAWN | ACTION_ATTACK);
	}

	// Saber walk logic
	//bot_should_walk_saber(bs, &bi);

	// Timed actions (kick, gloat, flourish)
	if (bot_try_timed_action(bs, &next_kick[client], 20000, SABER_KICK_RANGE, ACTION_KICK))
		bi.actionflags |= ACTION_KICK;

	// Saber engagement emote system
	if (bot_in_saber_engagement(bs))
	{
		// Only choose an emote if:
		// 1) No emote chosen for this engagement
		// 2) Global cooldown expired
		if (bs->saberEngageEmote == 0 &&
			level.time >= bs->nextSaberEmoteTime)
		{
			bs->saberEngageStartTime = level.time;

			int r = rand() % 10;

			if (r < 3)
				bs->saberEngageEmote = ACTION_GLOAT;
			else if (r < 5)
				bs->saberEngageEmote = ACTION_FLOURISH;
			else
				bs->saberEngageEmote = 0; // no emote this time

			// Set global cooldown (2 minutes)
			bs->nextSaberEmoteTime = level.time + 120000;
		}

		// Trigger the chosen emote ONCE, only if not attacking
		if (bs->saberEngageEmote &&
			!bs->doAttack &&
			!bs->doAltAttack &&
			bs->jumpTime <= level.time)
		{
			bi.actionflags |= bs->saberEngageEmote;

			// Prevent repeating it every frame
			bs->saberEngageEmote = 0;
		}
	}
	else
	{
		// Left saber range → reset for next engagement
		bs->saberEngageEmote = 0;
	}
	
	// Saber combat walking logic
	if (bs->cur_ps.weapon == WP_SABER)
	{
		gentity_t* self = &g_entities[bs->cur_ps.clientNum];
		gentity_t* enemy = bs->currentEnemy;

		qboolean inSaberCombat = qfalse;

		if (enemy &&
			enemy->client &&
			enemy->health > 0 &&
			!bs->cur_ps.saberHolstered) // saber ignited
		{
			float dist = VectorDistance(self->r.currentOrigin, enemy->r.currentOrigin);

			// Duel distance (feels natural in Movie Duels)
			if (dist < 200.0f)
			{
				inSaberCombat = qtrue;
			}
		}

		if (inSaberCombat)
		{
			bi.actionflags |= ACTION_WALK;
		}
		else
		{
			bi.actionflags &= ~ACTION_WALK;
		}
	}

	// Manual overrides
	if (bs->doWalk)       bi.actionflags |= ACTION_WALK;
	if (bs->doBotKick)    bi.actionflags |= ACTION_KICK;
	if (bs->doBotGesture) bi.actionflags |= ACTION_GESTURE;
	if (bs->doBotBlock && bs->cur_ps.weapon == WP_SABER)
		bi.actionflags |= ACTION_BLOCK;

	// Force selection
	bi.forcesel = level.clients[bs->client].ps.fd.forcePowerSelected;

	// Convert to usercmd
	bot_input_to_user_command(&bi, &bs->lastucmd, bs->cur_ps.delta_angles, time, bs->noUseTime);

	// Unapply delta angles
	bot_unapply_delta_angles(bs);
}

/*
==============
BotAIRegularUpdate
==============
*/
static void bot_ai_regular_update(void)
{
	if (regularupdate_time < FloatTime())
	{
		trap->BotUpdateEntityItems();
		regularupdate_time = FloatTime() + 0.3;
	}
}

/*
==============
RemoveColorEscapeSequences
==============
*/
static void remove_color_escape_sequences(char* text)
{
	int l = 0;
	for (int i = 0; text[i]; i++)
	{
		if (Q_IsColorStringExt(&text[i]))
		{
			i++;
			continue;
		}
		if (text[i] > 0x7E)
			continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}

/*
==============
BotAI
==============
*/

static int bot_ai(const int client, const float thinktime)
{
	bot_state_t* bs = botstates[client];
	if (!bs || !bs->inuse)
		return qfalse;

	trap->EA_ResetInput(client);

	// Update client state
	bot_ai_get_client_state(client, &bs->cur_ps);

	// Process pending server commands
	char buf[1024];
	while (trap->BotGetServerCommand(client, buf, sizeof buf))
	{
		char* args = strchr(buf, ' ');
		if (!args)
			continue;

		*args++ = '\0';
		remove_color_escape_sequences(args);

		if (!Q_stricmp(buf, "cp"))
		{
			// CenterPrint ignored
		}
		else if (!Q_stricmp(buf, "cs"))
		{
			// ConfigStringModified ignored
		}
		else if (!Q_stricmp(buf, "scores"))
		{
			// Score parsing ignored
		}
		else if (!Q_stricmp(buf, "clientLevelShot"))
		{
			// Ignored
		}
	}

	// Apply delta angles
	for (int i = 0; i < 3; i++)
		bs->viewangles[i] = AngleMod(bs->viewangles[i] + SHORT2ANGLE(bs->cur_ps.delta_angles[i]));

	// Update bot timing
	bs->ltime += thinktime;
	bs->thinktime = thinktime;

	// Update origin and eye position
	VectorCopy(bs->cur_ps.origin, bs->origin);
	VectorCopy(bs->cur_ps.origin, bs->eye);
	bs->eye[2] += bs->cur_ps.viewheight;

#ifdef _DEBUG
	const int start = trap->Milliseconds();
#endif

	// Run AI
	if (bs->settings.skill <= 3)
		standard_bot_ai(bs);
	else
		Enhanced_bot_ai(bs);

#ifdef _DEBUG
	const int end = trap->Milliseconds();
	trap->Cvar_Update(&bot_debugmessages);

	if (bot_debugmessages.integer)
		Com_Printf("Single AI frametime: %i\n", end - start);
#endif

	// Unapply delta angles
	for (int i = 0; i < 3; i++)
		bs->viewangles[i] = AngleMod(bs->viewangles[i] - SHORT2ANGLE(bs->cur_ps.delta_angles[i]));

	// Decay waypoint penalties
	for (int i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		if (bs->wpFailPenalty[i] > 0.0f)
			bs->wpFailPenalty[i] *= 0.95f;
	}

	return qtrue;
}

/*
==================
BotScheduleBotThink
==================
*/
static void bot_schedule_bot_think(void)
{
	int botnum = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!botstates[i] || !botstates[i]->inuse)
		{
			continue;
		}
		//initialize the bot think residual time
		botstates[i]->botthink_residual = BOT_THINK_TIME * botnum / numbots;
		botnum++;
	}
}

static int players_in_game(void)
{
	int i = 0;
	int pl = 0;

	while (i < MAX_CLIENTS)
	{
		const gentity_t* ent = &g_entities[i];

		if (ent && ent->client && ent->client->pers.connected == CON_CONNECTED)
		{
			pl++;
		}

		i++;
	}

	return pl;
}

/*
==============
BotAISetupClient
==============
*/
int bot_ai_setup_client(const int client, const struct bot_settings_s* settings)
{
	if (!botstates[client])
	{
		botstates[client] = (bot_state_t*)B_Alloc(sizeof(bot_state_t));
	}

	memset(botstates[client], 0, sizeof(bot_state_t));

	bot_state_t* bs = botstates[client];

	if (bs && bs->inuse)
	{
		return qfalse;
	}

	memcpy(&bs->settings, settings, sizeof(bot_settings_t));

	bs->client = client; //need to know the client number before doing personality stuff

	//initialize weapon weight defaults..
	bs->botWeaponWeights[WP_NONE] = 0;
	bs->botWeaponWeights[WP_STUN_BATON] = 1;
	bs->botWeaponWeights[WP_MELEE] = 1;
	bs->botWeaponWeights[WP_SABER] = 10;
	bs->botWeaponWeights[WP_BRYAR_PISTOL] = 11;
	bs->botWeaponWeights[WP_BLASTER] = 12;
	bs->botWeaponWeights[WP_DISRUPTOR] = 13;
	bs->botWeaponWeights[WP_BOWCASTER] = 14;
	bs->botWeaponWeights[WP_REPEATER] = 15;
	bs->botWeaponWeights[WP_DEMP2] = 16;
	bs->botWeaponWeights[WP_FLECHETTE] = 17;
	bs->botWeaponWeights[WP_ROCKET_LAUNCHER] = 18;
	bs->botWeaponWeights[WP_THERMAL] = 14;
	bs->botWeaponWeights[WP_TRIP_MINE] = 0;
	bs->botWeaponWeights[WP_DET_PACK] = 0;
	bs->botWeaponWeights[WP_CONCUSSION] = 15;
	bs->botWeaponWeights[WP_BRYAR_OLD] = 19;

	BotUtilizePersonality(bs);

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		bs->botWeaponWeights[WP_SABER] = 13;
	}

	//allocate a goal state
	bs->gs = trap->BotAllocGoalState(client);

	//allocate a weapon state
	bs->ws = trap->BotAllocWeaponState();

	bs->inuse = qtrue;
	bs->entityNum = client;
	bs->setupcount = 4;
	bs->entergame_time = FloatTime();
	bs->ms = trap->BotAllocMoveState();
	numbots++;

	//NOTE: reschedule the bot thinking
	bot_schedule_bot_think();

	if (players_in_game())
	{
		//don't talk to yourself
		BotDoChat(bs, "GeneralGreetings", 0);
	}

	return qtrue;
}

/*
==============
BotAIShutdownClient
==============
*/
int BotAIShutdownClient(const int client)
{
	bot_state_t* bs = botstates[client];
	if (!bs || !bs->inuse)
	{
		return qfalse;
	}

	trap->BotFreeMoveState(bs->ms);
	//free the goal state`
	trap->BotFreeGoalState(bs->gs);
	//free the weapon weights
	trap->BotFreeWeaponState(bs->ws);
	//
	//clear the bot state
	memset(bs, 0, sizeof(bot_state_t));
	//set the inuse flag to qfalse
	bs->inuse = qfalse;
	//there's one bot less
	numbots--;
	//everything went ok
	return qtrue;
}

/*
==============
BotResetState

called when a bot enters the intermission or observer mode and
when the level is changed
==============
*/
void bot_reset_state(bot_state_t* bs)
{
	bot_settings_t settings;
	playerState_t ps; //current player state

	//save some things that should not be reset here
	memcpy(&settings, &bs->settings, sizeof(bot_settings_t));
	memcpy(&ps, &bs->cur_ps, sizeof(playerState_t));
	const int inuse = bs->inuse;
	const int client = bs->client;
	const int entityNum = bs->entityNum;
	const int movestate = bs->ms;
	const int goalstate = bs->gs;
	const int weaponstate = bs->ws;
	const float entergame_time = bs->entergame_time;
	//reset the whole state
	memset(bs, 0, sizeof(bot_state_t));
	//copy back some state stuff that should not be reset
	bs->ms = movestate;
	bs->gs = goalstate;
	bs->ws = weaponstate;
	memcpy(&bs->cur_ps, &ps, sizeof(playerState_t));
	memcpy(&bs->settings, &settings, sizeof(bot_settings_t));
	bs->inuse = inuse;
	bs->client = client;
	bs->entityNum = entityNum;
	bs->entergame_time = entergame_time;
	//reset several states
	if (bs->ms) trap->BotResetMoveState(bs->ms);
	if (bs->gs) trap->BotResetGoalState(bs->gs);
	if (bs->ws) trap->BotResetWeaponState(bs->ws);
	if (bs->gs) trap->BotResetAvoidGoals(bs->gs);
	if (bs->ms) trap->BotResetAvoidReach(bs->ms);
}

/*
==============
BotAILoadMap
==============
*/
int BotAILoadMap()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (botstates[i] && botstates[i]->inuse)
		{
			bot_reset_state(botstates[i]);
			botstates[i]->setupcount = 4;
		}
	}

	return qtrue;
}

//rww - bot ai

//standard visibility check
int org_visible(vec3_t org1, vec3_t org2, const int ignore)
{
	trace_t tr;

	trap->Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return 1;
	}

	return 0;
}

//special waypoint visibility check
static int wp_org_visible(const gentity_t* bot, vec3_t org1, vec3_t org2, const int ignore)
{
	trace_t tr;

	trap->Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		trap->Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_PLAYERSOLID, qfalse, 0, 0);

		if (tr.fraction != 1 && tr.entityNum != ENTITYNUM_NONE && g_entities[tr.entityNum].s.eType == ET_SPECIAL)
		{
			if (g_entities[tr.entityNum].parent && g_entities[tr.entityNum].parent->client)
			{
				const gentity_t* ownent = g_entities[tr.entityNum].parent;

				if (OnSameTeam(bot, ownent) || bot->s.number == ownent->s.number)
				{
					return 1;
				}
			}
			return 2;
		}

		return 1;
	}

	return 0;
}

//visibility check with hull trace
int org_visible_box(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, const int ignore)
{
	trace_t tr;

	if (RMG.integer)
	{
		trap->Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_SOLID, qfalse, 0, 0);
	}
	else
	{
		trap->Trace(&tr, org1, mins, maxs, org2, ignore, MASK_SOLID, qfalse, 0, 0);
	}

	if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
	{
		return 1;
	}

	return 0;
}

//see if there's a func_* ent under the given pos.
//kind of badly done, but this shouldn't happen
//often.
static int check_for_func(vec3_t org, const int ignore)
{
	vec3_t under;
	trace_t tr;

	VectorCopy(org, under);

	under[2] -= 64;

	trap->Trace(&tr, org, NULL, NULL, under, ignore, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return 0;
	}

	const gentity_t* fent = &g_entities[tr.entityNum];

	if (!fent)
	{
		return 0;
	}

	if (strstr(fent->classname, "func_"))
	{
		return 1; //there's a func brush here
	}

	return 0;
}

//perform pvs check based on rmg or not
static qboolean bot_pvs_check(const vec3_t p1, const vec3_t p2)
{
	if (RMG.integer && bot_pvstype.integer)
	{
		vec3_t subPoint;
		VectorSubtract(p1, p2, subPoint);

		if (VectorLength(subPoint) > 5000)
		{
			return qfalse;
		}
		return qtrue;
	}

	return trap->InPVS(p1, p2);
}

/*
=========================
START A* Pathfinding Code
=========================
*/

typedef struct node_waypoint_s
{
	int wpNum;
	float f;
	float g;
	float h;
	int pNum;
} node_waypoint_t;

//Don't use the 0 slot of this array.  It's a binary heap and it's based on 1 being the first slot.
node_waypoint_t open_list[MAX_WPARRAY_SIZE + 1];

node_waypoint_t close_list[MAX_WPARRAY_SIZE];

static qboolean open_list_empty(void)
{
	//since we're using a binary heap, in theory, if the first slot is empty, the heap
	//is empty.
	if (open_list[1].wpNum != -1)
	{
		return qfalse;
	}

	return qtrue;
}

//Scans for the given wp on the Open List and returns it's OpenList position.
//Returns -1 if not found.
static int find_open_list(const int wpNum)
{
	for (int i = 1; i < MAX_WPARRAY_SIZE + 1 && open_list[i].wpNum != -1; i++)
	{
		if (open_list[i].wpNum == wpNum)
		{
			return i;
		}
	}
	return -1;
}

//Scans for the given wp on the Close List and returns it's CloseList position.
//Returns -1 if not found.
static int find_close_list(const int wpNum)
{
	for (int i = 0; i < MAX_WPARRAY_SIZE && close_list[i].wpNum != -1; i++)
	{
		if (close_list[i].wpNum == wpNum)
		{
			return i;
		}
	}
	return -1;
}

static qboolean carrying_cap_objective(const bot_state_t* bs)
{
	//Carrying the Capture Objective?
	if (level.gametype == GT_SIEGE)
	{
		if (bs->tacticEntity && bs->client == bs->tacticEntity->genericValue8)
			return qtrue;
	}
	else
	{
		if (g_entities[bs->client].client->ps.powerups[PW_REDFLAG]
			|| g_entities[bs->client].client->ps.powerups[PW_BLUEFLAG])
			return qtrue;
	}
	return qfalse;
}

static float route_randomize(const bot_state_t* bs, const float dest_dist)
{
	//this function randomizes the h value (distance to target location) to make the
	//bots take a random path instead of always taking the shortest route.
	//This should vary based on situation to prevent the bots from taking weird routes
	//for inapproprate situations.
	if (bs->currentTactic == BOTORDER_OBJECTIVE
		&& bs->objectiveType == OT_CAPTURE
		&& !carrying_cap_objective(bs))
	{
		//trying to capture something.  Fairly random paths to mix up the defending team.
		return dest_dist * Q_flrand(.5, 1.5);
	}

	//return shortest distance.
	return dest_dist;
}

//Add this wpNum to the open list.
static void add_open_list(const bot_state_t* bs, const int wp_num, const int parent, const int end)
{
	int i;
	float g;

	if (gWPArray[wp_num]->flags & WPFLAG_REDONLY
		&& g_entities[bs->client].client->sess.sessionTeam != TEAM_RED)
	{
		//red only wp, can't use
		return;
	}

	if (gWPArray[wp_num]->flags & WPFLAG_BLUEONLY
		&& g_entities[bs->client].client->sess.sessionTeam != TEAM_BLUE)
	{
		//blue only wp, can't use
		return;
	}

	if (parent != -1 && gWPArray[wp_num]->flags & WPFLAG_JUMP)
	{
		if (force_jump_needed(gWPArray[parent]->origin, gWPArray[wp_num]->origin) > bs->cur_ps.fd.forcePowerLevel[
			FP_LEVITATION])
		{
			//can't make this jump with our level of Force Jump
			return;
		}
	}

	if (parent == -1)
	{
		//This is the start point, just use zero for g
		g = 0;
	}
	else if (wp_num == parent + 1)
	{
		if (gWPArray[wp_num]->flags & WPFLAG_ONEWAY_BACK)
		{
			//can't go down this one way
			return;
		}
		i = find_close_list(parent);
		if (i != -1)
		{
			g = close_list[i].g + gWPArray[parent]->disttonext;
		}
		else
		{
			return;
		}
	}
	else if (wp_num == parent - 1)
	{
		if (gWPArray[wp_num]->flags & WPFLAG_ONEWAY_FWD)
		{
			//can't go down this one way
			return;
		}
		i = find_close_list(parent);
		if (i != -1)
		{
			g = close_list[i].g + gWPArray[wp_num]->disttonext;
		}
		else
		{
			return;
		}
	}
	else
	{
		//nonsequencal parent/wpNum
		//don't try to go thru oneways when you're doing neighbor moves
		if (gWPArray[wp_num]->flags & WPFLAG_ONEWAY_FWD || gWPArray[wp_num]->flags & WPFLAG_ONEWAY_BACK)
		{
			return;
		}

		i = find_close_list(parent);
		if (i != -1)
		{
			g = open_list[i].g + Distance(gWPArray[wp_num]->origin, gWPArray[parent]->origin);
		}
		else
		{
			return;
		}
	}

	//Find first open slot or the slot that this wpNum already occupies.
	for (i = 1; i < MAX_WPARRAY_SIZE + 1 && open_list[i].wpNum != -1; i++)
	{
		if (open_list[i].wpNum == wp_num)
		{
			break;
		}
	}

	float h = Distance(gWPArray[wp_num]->origin, gWPArray[end]->origin);

	//add a bit of a random factor to h to make the bots take a variety of paths.
	h = route_randomize(bs, h);

	const float f = g + h;
	open_list[i].f = f;
	open_list[i].g = g;
	open_list[i].h = h;
	open_list[i].pNum = parent;
	open_list[i].wpNum = wp_num;

	while (open_list[i].f <= open_list[i / 2].f && i != 1)
	{
		//binary parent has higher f than child
		const float ftemp = open_list[i / 2].f;
		const float gtemp = open_list[i / 2].g;
		const float htemp = open_list[i / 2].h;
		const int p_numtemp = open_list[i / 2].pNum;
		const int wptemp = open_list[i / 2].wpNum;

		open_list[i / 2].f = open_list[i].f;
		open_list[i / 2].g = open_list[i].g;
		open_list[i / 2].h = open_list[i].h;
		open_list[i / 2].pNum = open_list[i].pNum;
		open_list[i / 2].wpNum = open_list[i].wpNum;

		open_list[i].f = ftemp;
		open_list[i].g = gtemp;
		open_list[i].h = htemp;
		open_list[i].pNum = p_numtemp;
		open_list[i].wpNum = wptemp;

		i = i / 2;
	}
}

//Remove the first element from the OpenList.
static void remove_first_open_list(void)
{
	int i;
	for (i = 1; i < MAX_WPARRAY_SIZE + 1 && open_list[i].wpNum != -1; i++)
	{
	}

	i--;
	if (open_list[i].wpNum == -1)
	{
		//
	}

	if (open_list[1].wpNum == open_list[i].wpNum)
	{
		//the first slot is the only thing on the list. blank it.
		open_list[1].f = -1;
		open_list[1].g = -1;
		open_list[1].h = -1;
		open_list[1].pNum = -1;
		open_list[1].wpNum = -1;
		return;
	}

	//shift last entry to start
	open_list[1].f = open_list[i].f;
	open_list[1].g = open_list[i].g;
	open_list[1].h = open_list[i].h;
	open_list[1].pNum = open_list[i].pNum;
	open_list[1].wpNum = open_list[i].wpNum;

	open_list[i].f = -1;
	open_list[i].g = -1;
	open_list[i].h = -1;
	open_list[i].pNum = -1;
	open_list[i].wpNum = -1;

	while (open_list[i].f >= open_list[i * 2].f && open_list[i * 2].wpNum != -1
		|| open_list[i].f >= open_list[i * 2 + 1].f && open_list[i * 2 + 1].wpNum != -1)
	{
		if (open_list[i * 2].f < open_list[i * 2 + 1].f || open_list[i * 2 + 1].wpNum == -1)
		{
			const float ftemp = open_list[i * 2].f;
			const float gtemp = open_list[i * 2].g;
			const float htemp = open_list[i * 2].h;
			const int p_numtemp = open_list[i * 2].pNum;
			const int wptemp = open_list[i * 2].wpNum;

			open_list[i * 2].f = open_list[i].f;
			open_list[i * 2].g = open_list[i].g;
			open_list[i * 2].h = open_list[i].h;
			open_list[i * 2].pNum = open_list[i].pNum;
			open_list[i * 2].wpNum = open_list[i].wpNum;

			open_list[i].f = ftemp;
			open_list[i].g = gtemp;
			open_list[i].h = htemp;
			open_list[i].pNum = p_numtemp;
			open_list[i].wpNum = wptemp;

			i = i * 2;
		}
		else if (open_list[i * 2 + 1].wpNum != -1)
		{
			const float ftemp = open_list[i * 2 + 1].f;
			const float gtemp = open_list[i * 2 + 1].g;
			const float htemp = open_list[i * 2 + 1].h;
			const int p_numtemp = open_list[i * 2 + 1].pNum;
			const int wptemp = open_list[i * 2 + 1].wpNum;

			open_list[i * 2 + 1].f = open_list[i].f;
			open_list[i * 2 + 1].g = open_list[i].g;
			open_list[i * 2 + 1].h = open_list[i].h;
			open_list[i * 2 + 1].pNum = open_list[i].pNum;
			open_list[i * 2 + 1].wpNum = open_list[i].wpNum;

			open_list[i].f = ftemp;
			open_list[i].g = gtemp;
			open_list[i].h = htemp;
			open_list[i].pNum = p_numtemp;
			open_list[i].wpNum = wptemp;

			i = i * 2 + 1;
		}
		else
		{
			return;
		}
	}
}

//Adds a given OpenList wp to the closed list
static void add_close_list(const int openListpos)
{
	if (open_list[openListpos].wpNum != -1)
	{
		for (int i = 0; i < MAX_WPARRAY_SIZE; i++)
		{
			if (close_list[i].wpNum == -1)
			{
				//open slot, fill it.  heheh.
				close_list[i].f = open_list[openListpos].f;
				close_list[i].g = open_list[openListpos].g;
				close_list[i].h = open_list[openListpos].h;
				close_list[i].pNum = open_list[openListpos].pNum;
				close_list[i].wpNum = open_list[openListpos].wpNum;
				return;
			}
		}
	}
}

//Clear out the Route
static void clear_route(int Route[MAX_WPARRAY_SIZE])
{
	for (int i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		Route[i] = -1;
	}
}

static void addto_route(const int wp_num, int route[MAX_WPARRAY_SIZE])
{
	int i;
	for (i = 0; i < MAX_WPARRAY_SIZE && route[i] != -1; i++)
	{
	}

	if (route[i] == -1 && i < MAX_WPARRAY_SIZE)
	{
		//found the first empty slot
		while (i > 0)
		{
			route[i] = route[i - 1];
			i--;
		}
	}
	else
	{
		return;
	}
	if (i == 0)
	{
		route[0] = wp_num;
	}
}

//find a given wpNum on the given route and return it's address.  return -1 if not on route.
//use wpNum = -1 to find the last wp on route.
static int find_on_route(const int wp_num, int route[MAX_WPARRAY_SIZE])
{
	int i;
	for (i = 0; i < MAX_WPARRAY_SIZE && route[i] != wp_num; i++)
	{
	}

	//Special find end route command stuff
	if (wp_num == -1)
	{
		i--;
		if (route[i] != -1)
		{
			//found it
			return i;
		}

		//otherwise, this is a empty route list
		return -1;
	}

	if (wp_num == route[i])
	{
		//Success!
		return i;
	}

	//Couldn't find it
	return -1;
}

//Copy Route
static void copy_route(bot_route_t routesource, bot_route_t routedest)
{
	for (int i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		routedest[i] = routesource[i];
	}
}

//Find the ideal (shortest) route between the start wp and the end wp
//badwp is for situations where you need to recalc a path when you dynamically discover
//that a wp is bad (door locked, blocked, etc).
//doRoute = actually set botRoute
static float find_ideal_pathto_wp(bot_state_t* bs, const int start, const int end, const int badwp, bot_route_t route)
{
	int i;

	if (bs->PathFindDebounce > level.time)
	{
		//currently debouncing the path finding to prevent a massive overload of AI thinking
		//in weird situations, like when the target near a waypoint where the bot can't
		//get to (like in an area where you need a specific force power to get to).
		return -1;
	}

	if (start == end)
	{
		clear_route(route);
		addto_route(end, route);
		return 0;
	}

	//reset node lists
	for (i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		open_list[i].wpNum = -1;
		open_list[i].f = -1;
		open_list[i].g = -1;
		open_list[i].h = -1;
		open_list[i].pNum = -1;
	}

	for (i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		close_list[i].wpNum = -1;
		close_list[i].f = -1;
		close_list[i].g = -1;
		close_list[i].h = -1;
		close_list[i].pNum = -1;
	}

	add_open_list(bs, start, -1, end);

	while (!open_list_empty() && find_open_list(end) == -1)
	{
		//open list not empty
		//we're using a binary pile so the first slot always has the lowest f score
		add_close_list(1);
		i = open_list[1].wpNum;
		remove_first_open_list();

		//Add surrounding nodes
		if (gWPArray[i + 1] && gWPArray[i + 1]->inuse)
		{
			if (gWPArray[i]->disttonext < 1000
				&& find_close_list(i + 1) == -1 && i + 1 != badwp)
			{
				//Add next sequential node
				add_open_list(bs, i + 1, i, end);
			}
		}

		if (i > 0)
		{
			if (gWPArray[i - 1]->disttonext < 1000 && gWPArray[i - 1]->inuse
				&& find_close_list(i - 1) == -1 && i - 1 != badwp)
			{
				//Add previous sequential node
				add_open_list(bs, i - 1, i, end);
			}
		}

		if (gWPArray[i]->neighbornum)
		{
			for (int x = 0; x < gWPArray[i]->neighbornum; x++)
			{
				if (x != badwp && find_close_list(gWPArray[i]->neighbors[x].num) == -1)
				{
					add_open_list(bs, gWPArray[i]->neighbors[x].num, i, end);
				}
			}
		}
	}

	i = find_open_list(end);

	if (i != -1)
	{
		//we have a valid route to the end point
		clear_route(route);
		addto_route(end, route);
		const float dist = open_list[i].g;
		i = open_list[i].pNum;
		i = find_close_list(i);
		while (i != -1)
		{
			addto_route(close_list[i].wpNum, route);
			i = find_close_list(close_list[i].pNum);
		}
		//only have the debouncer when we fail to find a route.
		bs->PathFindDebounce = level.time;
		return dist;
	}

	if (bot_wp_edit.integer)
	{
		//print error message if in edit mode.
	}
	bs->PathFindDebounce = level.time + 3000; //try again in 3 seconds.

	return -1;
}

/*
=========================
END A* Pathfinding Code
=========================
*/

//get the index to the nearest visible waypoint in the global trail
int get_nearest_visible_wp(vec3_t org, const int ignore)
{
	float bestdist;
	vec3_t mins, maxs;

	int i = 0;
	if (RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		bestdist = 800; //99999;
		//don't trace over 800 units away to avoid GIANT HORRIBLE SPEED HITS ^_^
	}
	int bestindex = -1;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -1;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 1;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			vec3_t a;
			VectorSubtract(org, gWPArray[i]->origin, a);
			const float flLen = VectorLength(a);

			if (flLen < bestdist && (RMG.integer || bot_pvs_check(org, gWPArray[i]->origin)) && org_visible_box(
				org, mins, maxs, gWPArray[i]->origin, ignore))
			{
				bestdist = flLen;
				bestindex = i;
			}
		}

		i++;
	}

	return bestindex;
}

//visually scanning in the given direction.
static void bot_behave_visual_scan(bot_state_t* bs)
{
	VectorCopy(bs->VisualScanDir, bs->goalAngles);
	bs->wpCurrent = NULL;
}

//Just stand still
static void bot_be_still(bot_state_t* bs)
{
	VectorCopy(bs->origin, bs->goalPosition);
	bs->beStill = level.time + BOT_THINK_TIME;
	VectorClear(bs->DestPosition);
	bs->DestIgnore = -1;
	bs->wpCurrent = NULL;
}

//just like GetNearestVisibleWP except with a bad waypoint input
static int get_nearest_visible_wpsje(const bot_state_t* bs, vec3_t org, const int ignore, const int badwp)
{
	float bestdist;
	vec3_t mins, maxs;

	if (RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		bestdist = 800; //99999;
		//don't trace over 800 units away to avoid GIANT HORRIBLE SPEED HITS ^_^
	}
	int bestindex = -1;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -1;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 1;

	for (int i = 0; i < gWPNum; i++)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
		{
			vec3_t a;
			if (bs)
			{
				//check to make sure that this bot's team can use this waypoint
				if (gWPArray[i]->flags & WPFLAG_REDONLY
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_RED)
				{
					//red only wp, can't use
					continue;
				}

				if (gWPArray[i]->flags & WPFLAG_BLUEONLY
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_BLUE)
				{
					//blue only wp, can't use
					continue;
				}
			}

			VectorSubtract(org, gWPArray[i]->origin, a);
			float fl_len = VectorLength(a);

			if (gWPArray[i]->flags & WPFLAG_WAITFORFUNC
				|| gWPArray[i]->flags & WPFLAG_NOMOVEFUNC
				|| gWPArray[i]->flags & WPFLAG_DESTROY_FUNCBREAK
				|| gWPArray[i]->flags & WPFLAG_FORCEPUSH
				|| gWPArray[i]->flags & WPFLAG_FORCEPULL)
			{
				//boost the distance for these waypoints so that we will try to avoid using them
				//if at all possible
				fl_len = +500;
			}

			if (fl_len < bestdist && (RMG.integer || bot_pvs_check(org, gWPArray[i]->origin)) && org_visible_box(
				org, mins, maxs, gWPArray[i]->origin, ignore))
			{
				bestdist = fl_len;
				bestindex = i;
			}
		}
	}

	return bestindex;
}

//just like GetNearestVisibleWP except without visiblity checks
static int get_nearest_wp(const bot_state_t* bs, vec3_t org, const int badwp)
{
	float bestdist;

	if (RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		//We're not doing traces!
		bestdist = 99999;
	}
	int bestindex = -1;

	for (int i = 0; i < gWPNum; i++)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
		{
			vec3_t a;
			if (bs)
			{
				//check to make sure that this bot's team can use this waypoint
				if (gWPArray[i]->flags & WPFLAG_REDONLY
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_RED)
				{
					//red only wp, can't use
					continue;
				}

				if (gWPArray[i]->flags & WPFLAG_BLUEONLY
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_BLUE)
				{
					//blue only wp, can't use
					continue;
				}
			}

			VectorSubtract(org, gWPArray[i]->origin, a);
			float fl_len = VectorLength(a);

			if (gWPArray[i]->flags & WPFLAG_WAITFORFUNC
				|| gWPArray[i]->flags & WPFLAG_NOMOVEFUNC
				|| gWPArray[i]->flags & WPFLAG_DESTROY_FUNCBREAK
				|| gWPArray[i]->flags & WPFLAG_FORCEPUSH
				|| gWPArray[i]->flags & WPFLAG_FORCEPULL)
			{
				//boost the distance for these waypoints so that we will try to avoid using them
				//if at all possible
				fl_len = fl_len + 500;
			}

			if (fl_len < bestdist)
			{
				bestdist = fl_len;
				bestindex = i;
			}
		}
	}

	return bestindex;
}

static qboolean is_heavy_weapon(const bot_state_t* bs, const int weapon)
{
	//right now we only show positive for weapons that can do this in primary fire mode
	switch (weapon)
	{
	case WP_MELEE:
		if (G_HeavyMelee(&g_entities[bs->client]))
		{
			return qtrue;
		}
		break;

	case WP_SABER:
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
	case WP_DET_PACK:
	case WP_CONCUSSION:
	case WP_FLECHETTE:
		return qtrue;
	default:;
	}

	return qfalse;
}

//use Force push or pull on local func_doors
static qboolean use_forceon_local(bot_state_t* bs, vec3_t origin, const qboolean pull)
{
	gentity_t* test = NULL;
	vec3_t center;
	qboolean skip_push_pull = qfalse;

	if (bs->DontSpamPushPull > level.time)
	{
		//pushed/pulled for this waypoint recently
		skip_push_pull = qtrue;
	}

	if (!skip_push_pull)
	{
		//Force Push/Pull
		while ((test = G_Find(test, FOFS(classname), "func_door")) != NULL)
		{
			vec3_t pos2;
			vec3_t pos1;
			if (!(test->spawnflags & 2))
			{
				//can't use the Force to move this door, ignore
				continue;
			}

			if (test->wait < 0 && test->moverState == MOVER_POS2)
			{
				//locked in position already, ignore.
				continue;
			}

			//find center, pos1, pos2
			if (VectorCompare(vec3_origin, test->s.origin))
			{
				vec3_t size;
				//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
				VectorSubtract(test->r.absmax, test->r.absmin, size);
				VectorMA(test->r.absmin, 0.5, size, center);
				if (test->spawnflags & 1 && test->moverState == MOVER_POS1)
				{
					//if at pos1 and started open, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
					VectorSubtract(center, test->pos1, center);
				}
				else if (!(test->spawnflags & 1) && test->moverState == MOVER_POS2)
				{
					//if at pos2, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
					VectorSubtract(center, test->pos2, center);
				}
				VectorAdd(center, test->pos1, pos1);
				VectorAdd(center, test->pos2, pos2);
			}
			else
			{
				//actually has an origin, pos1 and pos2 are absolute
				VectorCopy(test->r.currentOrigin, center);
				VectorCopy(test->pos1, pos1);
				VectorCopy(test->pos2, pos2);
			}

			if (Distance(origin, center) > 400)
			{
				//too far away
				continue;
			}

			if (Distance(pos1, bs->eye) < Distance(pos2, bs->eye))
			{
				//pos1 is closer
				if (test->moverState == MOVER_POS1)
				{
					//at the closest pos
					if (pull)
					{
						//trying to pull, but already at closest point, so screw it
						continue;
					}
				}
				else if (test->moverState == MOVER_POS2)
				{
					//at farthest pos
					if (!pull)
					{
						//trying to push, but already at farthest point, so screw it
						continue;
					}
				}
			}
			else
			{
				//pos2 is closer
				if (test->moverState == MOVER_POS1)
				{
					//at the farthest pos
					if (!pull)
					{
						//trying to push, but already at farthest point, so screw it
						continue;
					}
				}
				else if (test->moverState == MOVER_POS2)
				{
					//at closest pos
					if (pull)
					{
						//trying to pull, but already at closest point, so screw it
						continue;
					}
				}
			}
			//we have a winner!
			break;
		}

		if (test)
		{
			//have a push/pull able door
			vec3_t view_dir;
			vec3_t ang;

			//doing special wp move
			bs->wpSpecial = qtrue;

			VectorSubtract(center, bs->eye, view_dir);
			vectoangles(view_dir, ang);
			VectorCopy(ang, bs->goalAngles);

			if (in_field_of_vision(bs->viewangles, 5, ang))
			{
				//use the force
				if (pull)
				{
					bs->doForcePull = level.time + 700;
				}
				else
				{
					bs->doForcePush = level.time + 700;
				}
				//Only press the button once every 15 seconds
				//this way the bots will be able to go up/down a lift weither the elevator
				//was up or down.
				//This debounce only applies to this waypoint.
				bs->DontSpamPushPull = level.time + 15000;
			}
			return qtrue;
		}
	}

	if (bs->DontSpamButton > level.time)
	{
		//pressed a button recently
		if (bs->useTime > level.time)
		{
			//holding down button
			//update the hacking buttons to account for lag about crap
			if (g_entities[bs->client].client->isHacking)
			{
				bs->useTime = bs->cur_ps.hackingTime + 100;
				bs->DontSpamButton = bs->useTime + 15000;
				bs->wpSpecial = qtrue;
				return qtrue;
			}

			//lost hack target.  clear things out
			bs->useTime = level.time;
			return qfalse;
		}
		return qfalse;
	}

	//use button checking
	while ((test = G_Find(test, FOFS(classname), "trigger_multiple")) != NULL)
	{
		if (test->flags & FL_INACTIVE)
		{
			//set by target_deactivate
			continue;
		}

		if (!(test->spawnflags & 4))
		{
			//can't use button this trigger
			continue;
		}

		if (test->alliedTeam)
		{
			if (g_entities[bs->client].client->sess.sessionTeam != test->alliedTeam)
			{
				//not useable by this team
				continue;
			}
		}

		FindOrigin(test, center);

		if (Distance(origin, center) > 200)
		{
			//too far away
			continue;
		}

		break;
	}

	if (!test)
	{
		//not luck so far
		while ((test = G_Find(test, FOFS(classname), "trigger_once")) != NULL)
		{
			if (test->flags & FL_INACTIVE)
			{
				//set by target_deactivate
				continue;
			}

			if (!test->use)
			{
				//trigger already fired
				continue;
			}

			if (!(test->spawnflags & 4))
			{
				//can't use button this trigger
				continue;
			}

			if (test->alliedTeam)
			{
				if (g_entities[bs->client].client->sess.sessionTeam != test->alliedTeam)
				{
					//not useable by this team
					continue;
				}
			}

			FindOrigin(test, center);

			if (Distance(origin, center) > 200)
			{
				//too far away
				continue;
			}

			break;
		}
	}

	if (test)
	{
		if (level.gametype == GT_SIEGE && test->idealclass && test->idealclass[0])
		{
			if (!G_NameInTriggerClassList(bgSiegeClasses[g_entities[bs->client].client->siegeClass].name,
				test->idealclass))
			{
				//not the right class to use this button
				if (!switch_siege_ideal_class(bs, test->idealclass))
				{
					//didn't want to switch to the class that hacks this trigger, call
					//for help and then see if there's a local breakable to just
					//smash it open.
					request_siege_assistance(bs, SPC_SUPPORT);
					return AttackLocalBreakable(bs);
				}
				//switched classes to be able to hack this target
				return qtrue;
			}
		}

		{
			vec3_t view_dir;
			vec3_t ang;
			bs->wpSpecial = qtrue;

			//you can use use
			//set view angles.
			if (test->spawnflags & 2)
			{
				//you have to face in the direction of the trigger to have it work
				vectoangles(test->movedir, ang);
			}
			else
			{
				VectorSubtract(center, bs->eye, view_dir);
				vectoangles(view_dir, ang);
			}
			VectorCopy(ang, bs->goalAngles);

			if (G_PointInBounds(bs->origin, test->r.absmin, test->r.absmax))
			{
				//inside trigger zone, press use.
				bs->useTime = level.time + test->genericValue7 + 100;
				bs->DontSpamButton = bs->useTime + 15000;
			}
			else
			{
				//need to move closer
				VectorSubtract(center, bs->origin, view_dir);

				view_dir[2] = 0;
				VectorNormalize(view_dir);

				trap->EA_Move(bs->client, view_dir, 5000);
			}
			return qtrue;
		}
	}
	return qfalse;
}

//just returns the favorite weapon.
static int favorite_weapon(bot_state_t* bs, const gentity_t* target, const qboolean have_check, const qboolean ammoCheck,
	const int ignore_weaps)
{
	int bestweight = -1;
	int bestweapon = 0;

	for (int i = 0; i < WP_NUM_WEAPONS; i++)
	{
		if (have_check &&
			!(bs->cur_ps.stats[STAT_WEAPONS] & 1 << i))
		{
			//check to see if we actually have this weapon.
			continue;
		}

		if (ammoCheck &&
			bs->cur_ps.ammo[weaponData[i].ammoIndex] < weaponData[i].energyPerShot)
		{
			//check to see if we have enough ammo for this weapon.
			continue;
		}

		if (ignore_weaps & 1 << i)
		{
			//check to see if this weapon is on our ignored weapons list
			continue;
		}

		if (target && target->inuse)
		{
			//do additional weapon checks based on our target's attributes.
			if (target->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)
			{
				//don't use weapons that can't damage this target
				if (!is_heavy_weapon(bs, i))
				{
					continue;
				}
			}

			//try to use explosives on breakables if we can.
			if (strcmp(target->classname, "func_breakable") == 0)
			{
				if (i == WP_DET_PACK)
				{
					bestweight = 100;
					bestweapon = i;
				}
				else if (i == WP_THERMAL)
				{
					bestweight = 99;
					bestweapon = i;
				}
				else if (i == WP_ROCKET_LAUNCHER)
				{
					bestweight = 99;
					bestweapon = i;
				}
				else if (bs->botWeaponWeights[i] > bestweight)
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
			else
			{
				//no special requirements for this target.
				if (bs->botWeaponWeights[i] > bestweight)
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
		}
		else
		{
			//no target
			if (bs->botWeaponWeights[i] > bestweight)
			{
				bestweight = bs->botWeaponWeights[i];
				bestweapon = i;
			}
		}
	}

	if (/*level.gametype == GT_SIEGE
		&&*/ bestweapon == WP_NONE
		&& target && target->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)
	{
		//we don't have the weapons to damage this thingy. call for help!
		request_siege_assistance(bs, SPC_DEMOLITIONIST);
	}

	return bestweapon;
}

void reset_wp_timers(bot_state_t* bs);

static qboolean have_heavy_weapon(const int weapons)
{
	if (weapons & 1 << WP_SABER
		|| weapons & 1 << WP_ROCKET_LAUNCHER
		|| weapons & 1 << WP_THERMAL
		|| weapons & 1 << WP_TRIP_MINE
		|| weapons & 1 << WP_DET_PACK
		|| weapons & 1 << WP_CONCUSSION
		|| weapons & 1 << WP_FLECHETTE)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean AttackLocalBreakable(bot_state_t* bs)
{
	gentity_t* test = NULL;
	const gentity_t* valid = NULL;
	qboolean defend = qfalse;
	vec3_t testorigin;

	while ((test = G_Find(test, FOFS(classname), "func_breakable")) != NULL)
	{
		FindOrigin(test, testorigin);

		if (TargetDistance(bs, test, testorigin) < 300)
		{
			if (test->teamnodmg && test->teamnodmg == g_entities[bs->client].client->sess.sessionTeam)
			{
				//on a team that can't damage this breakable, as such defend it from immediate harm
				defend = qtrue;
			}

			valid = test;
			break;
		}

		//reset for next check
		VectorClear(testorigin);
	}

	if (valid)
	{
		//due to crazy stack overflow issues, just attack wildly while moving towards the
		//breakable
		trace_t tr;
		const int desiredweap = favorite_weapon(bs, valid, qtrue, qtrue, 0);

		//visual check

		trap->Trace(&tr, bs->eye, NULL, NULL, testorigin, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

		if (tr.entityNum == test->s.number || tr.fraction == 1.0)
		{
			//we can see the breakable
			//doing special wp move
			bs->wpSpecial = qtrue;

			if (defend)
			{
				//defend this target since we can assume that the other team is going to try to
				bot_behave_defend_basic(bs, testorigin);
			}
			else if (test->flags & FL_DMG_BY_HEAVY_WEAP_ONLY && !is_heavy_weapon(bs, desiredweap))
			{
				//we currently don't have a heavy weap that we can use to destroy this target
				if (have_heavy_weapon(bs->cur_ps.stats[STAT_WEAPONS]))
				{
					//we have a weapon that could destroy this target but we don't have ammo
					bot_behave_defend_basic(bs, testorigin);
				}
				else
				{
					//go hunting for a weapon that can destroy this object
					bot_behave_defend_basic(bs, testorigin);
				}
			}
			else
			{
				//ATTACK!
				//determine which weapon you want to use
				if (desiredweap != bs->virtualWeapon)
				{
					//need to switch to desired weapon
					bot_select_choice_weapon(bs, desiredweap, qtrue);
				}
				//set visible flag so we'll attack this.
				bs->frame_Enemy_Vis = 1;
				bot_behave_attack_basic(bs, valid);
			}

			return qtrue;
		}
	}
	return qfalse;
}

static void wp_visible_update(bot_state_t* bs)
{
	if (org_visible_box(bs->origin, NULL, NULL, bs->wpCurrent->origin, bs->client))
	{
		//see the waypoint hold the counter
		bs->wpSeenTime = level.time + 3000;
	}
}

static qboolean calculate_jump(vec3_t origin, vec3_t dest)
{
	vec3_t flatorigin;
	vec3_t flatdest;
	const float height_dif = dest[2] - origin[2];

	VectorCopy(origin, flatorigin);
	VectorCopy(dest, flatdest);

	const float dist = Distance(flatdest, flatorigin);

	if (height_dif > 30 && dist < 100)
	{
		return qtrue;
	}
	return qfalse;
}

static void bot_move(bot_state_t* bs, vec3_t dest, const qboolean wptravel, qboolean strafe)
{
	vec3_t move_dir;
	vec3_t view_dir;
	vec3_t ang;
	qboolean movetrace = qtrue;

	VectorSubtract(dest, bs->eye, view_dir);
	vectoangles(view_dir, ang);
	VectorCopy(ang, bs->goalAngles);

	if (wptravel)
	{
		//if we're traveling between waypoints, don't bob the view up and down.
		bs->goalAngles[PITCH] = 0;
	}

	VectorSubtract(dest, bs->origin, move_dir);
	move_dir[2] = 0;
	VectorNormalize(move_dir);

	if (wptravel && !bs->wpCurrent)
	{
		return;
	}
	if (wptravel)
	{
		//special wp moves
		if (bs->wpCurrent->flags & WPFLAG_DESTROY_FUNCBREAK)
		{
			//look for nearby func_breakable and break them if we can before we continue
			if (AttackLocalBreakable(bs))
			{
				//found a breakable that we can destroy
				bs->wpSeenTime = level.time + 3000;
				return;
			}
		}

		if (bs->wpCurrent->flags & WPFLAG_FORCEPUSH)
		{
			if (use_forceon_local(bs, bs->wpCurrent->origin, qfalse))
			{
				//found something we can Force Push
				bs->wpSeenTime = level.time + 3000;
				return;
			}
		}

		if (bs->wpCurrent->flags & WPFLAG_FORCEPULL)
		{
			if (use_forceon_local(bs, bs->wpCurrent->origin, qtrue))
			{
				//found something we can Force Pull
				wp_visible_update(bs);
				return;
			}
		}

		if (bs->wpCurrent->flags & WPFLAG_JUMP)
		{
			//jump while travelling to this point
			vec3_t viewang;
			vec3_t velocity;
			vec3_t flatorigin, flatstart, flatend;

			VectorCopy(bs->origin, flatorigin);
			VectorCopy(bs->wpCurrentLoc, flatstart);
			VectorCopy(bs->wpCurrent->origin, flatend);

			flatorigin[2] = flatstart[2] = flatend[2] = 0;

			const float diststart = Distance(flatorigin, flatstart);
			const float distend = Distance(flatorigin, flatend);

			VectorSubtract(dest, bs->origin, viewang);
			vectoangles(viewang, ang);

			//never strafe during when jumping somewhere
			strafe = qfalse;

			if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE &&
				(diststart < distend || bs->origin[2] < bs->wpCurrent->origin[2]))
			{
				//before jump attempt
				if (ForcePowerforJump[force_jump_needed(bs->origin, bs->wpCurrent->origin)] > bs->cur_ps.fd.forcePower)
				{
					//we don't have enough energy to make our jump.  wait here.
					bs->wpSpecial = qtrue;
					return;
				}
			}

			//velocity analysis
			viewang[2] = 0;
			VectorNormalize(viewang);
			VectorCopy(bs->cur_ps.velocity, velocity);
			velocity[2] = 0;
			const float hor_velo = VectorNormalize(velocity);

			//make sure we're stopped or moving towards our goal before jumping
			if (diststart < distend && (VectorCompare(vec3_origin, velocity) || DotProduct(velocity, viewang) > .7)
				|| bs->cur_ps.groundEntityNum == ENTITYNUM_NONE)
			{
				//moving towards to our jump target or not moving at all or already on route and not already near the target.
				//hold down jump until we're pretty sure that we'll hit our target by just falling onto it.
				vec3_t to_dest_flat;
				qboolean hold_jump = qtrue;

				VectorSubtract(flatend, flatorigin, to_dest_flat);
				VectorNormalize(to_dest_flat);

				const float velo_scaler = DotProduct(to_dest_flat, velocity);

				//figure out how long it will take make it to the target with our current horizontal velocity.
				if (hor_velo)
				{
					//can't check when not moving
					const float time_to_end = distend / (hor_velo * velo_scaler);
					//assumes we're moving fully in the correct direction

					//calculate our estimated vertical position if we just let go of the jump now.
					const float estVert = bs->origin[2] + bs->cur_ps.velocity[2] * time_to_end - g_gravity.value *
						time_to_end *
						time_to_end;

					if (estVert >= bs->wpCurrent->origin[2])
					{
						//we're going to make it, let go of jump
						hold_jump = qfalse;
					}
				}

				if (hold_jump)
				{
					//jump
					bs->jumpTime = level.time + 100;
					bs->wpSpecial = qtrue;
					wp_visible_update(bs);
					trap->EA_Move(bs->client, move_dir, 5000);
					return;
				}
			}
		}

		//not doing a special wp move so clear that flag.
		bs->wpSpecial = qfalse;

		if (bs->wpCurrent->flags & WPFLAG_WAITFORFUNC)
		{
			if (!check_for_func(bs->wpCurrent->origin, bs->client))
			{
				wp_visible_update(bs);
				if (!bs->AltRouteCheck && bs->wpTravelTime - level.time < 20000)
				{
					//been waiting for 10 seconds, try looking for alt route if we haven't
					//already
					bot_route_t route_test;
					int newwp = get_nearest_visible_wpsje(bs, bs->origin, bs->client,
						bs->wpCurrent->index);
					bs->AltRouteCheck = qtrue;

					if (newwp == -1)
					{
						newwp = get_nearest_wp(bs, bs->origin, bs->wpCurrent->index);
					}
					if (find_ideal_pathto_wp(bs, newwp, bs->wpDestination->index, bs->wpCurrent->index, route_test) != -
						1)
					{
						//found a new route
						bs->wpCurrent = gWPArray[newwp];
						copy_route(route_test, bs->botRoute);
						reset_wp_timers(bs);
					}
				}
				return;
			}
		}
		if (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
		{
			if (check_for_func(bs->wpCurrent->origin, bs->client))
			{
				wp_visible_update(bs);
				if (!bs->AltRouteCheck && bs->wpTravelTime - level.time < 20000)
				{
					//been waiting for 10 seconds, try looking for alt route if we haven't
					//already
					bot_route_t route_test;
					int newwp = get_nearest_visible_wpsje(bs, bs->origin, bs->client, bs->wpCurrent->index);
					bs->AltRouteCheck = qtrue;

					if (newwp == -1)
					{
						newwp = get_nearest_wp(bs, bs->origin, bs->wpCurrent->index);
					}
					if (find_ideal_pathto_wp(bs, newwp, bs->wpDestination->index, bs->wpCurrent->index, route_test) != -
						1)
					{
						//found a new route
						bs->wpCurrent = gWPArray[newwp];
						copy_route(route_test, bs->botRoute);
						reset_wp_timers(bs);
					}
				}
				return;
			}
		}

		if (bs->wpCurrent->flags & WPFLAG_DUCK)
		{
			//duck while travelling to this point
			bs->duckTime = level.time + 100;
		}

		//visual check
		if (!(bs->wpCurrent->flags & WPFLAG_NOVIS))
		{
			//do visual check
			wp_visible_update(bs);
		}
		else
		{
			movetrace = qfalse;
			strafe = qfalse;
		}
	}
	else
	{
		//jump to dest if we need to.
		if (calculate_jump(bs->origin, dest))
		{
			bs->jumpTime = level.time + 100;
		}
	}

	//set strafing.
	if (strafe)
	{
		if (bs->meleeStrafeTime < level.time)
		{
			//select a new strafing direction, since we're actively navigating, switch strafe
			//directions more often
			//0 = no strafe
			//1 = strafe right
			//2 = strafe left
			bs->meleeStrafeDir = Q_irand(0, 2);
			bs->meleeStrafeTime = level.time + Q_irand(500, 1000);
		}

		//adjust the moveDir to do strafing
		adjustfor_strafe(bs, move_dir);
	}

	if (movetrace)
	{
		trace_move(bs, move_dir, bs->DestIgnore);
	}

	if (DistanceHorizontal(bs->origin, dest) > 10)
	{
		//move if we're not in touch range.
		trap->EA_Move(bs->client, move_dir, 5000);
	}
}

static qboolean dont_block_allies(bot_state_t* bs)
{
	for (int i = 0; i < level.maxclients; i++)
	{
		if (i != bs->client //not the bot
			&& g_entities[i].inuse && g_entities[i].client //valid player
			&& g_entities[i].client->pers.connected == CON_CONNECTED //who is connected
			&& !(g_entities[i].s.eFlags & EF_DEAD) //and isn't dead
			&& g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.sessionTeam
			//is on our team
			&& Distance(g_entities[i].client->ps.origin, bs->origin) < 50) //and we're too close to them.
		{
			//on your team and too close
			vec3_t move_dir, dest_origin;
			VectorSubtract(bs->origin, g_entities[i].client->ps.origin, move_dir);
			VectorAdd(bs->origin, move_dir, dest_origin);
			bot_move(bs, dest_origin, qfalse, qfalse);
			return qtrue;
		}
	}
	return qfalse;
}

static void wp_touch(bot_state_t* bs)
{
	//Touched the target WP
	int i = find_on_route(bs->wpCurrent->index, bs->botRoute);

	if (i == -1)
	{
		//This wp isn't on the route
		if (find_ideal_pathto_wp(bs, bs->wpCurrent->index, bs->wpDestination->index, -2, bs->botRoute) == -1)
		{
			//couldn't find new path to destination!
			bs->wpCurrent = NULL;
			bot_move(bs, bs->DestPosition, qfalse, qfalse);
			return;
		}
		//set wp timers
		reset_wp_timers(bs);
	}

	i = find_on_route(bs->wpCurrent->index, bs->botRoute);
	i++;

	if (i >= MAX_WPARRAY_SIZE || bs->botRoute[i] == -1)
	{
		//at destination wp
		bs->wpCurrent = bs->wpDestination;
		VectorCopy(bs->origin, bs->wpCurrentLoc);
		reset_wp_timers(bs);
		return;
	}
	if (!gWPArray[i] || !gWPArray[i]->inuse)
	{
		return;
	}

	bs->wpCurrent = gWPArray[bs->botRoute[i]];
	VectorCopy(bs->origin, bs->wpCurrentLoc);
	reset_wp_timers(bs);
}

void reset_wp_timers(bot_state_t* bs)
{
	if (bs->wpCurrent->flags & WPFLAG_WAITFORFUNC
		|| bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC
		|| bs->wpCurrent->flags & WPFLAG_DESTROY_FUNCBREAK
		|| bs->wpCurrent->flags & WPFLAG_FORCEPUSH
		|| bs->wpCurrent->flags & WPFLAG_FORCEPULL)
	{
		//it's an elevator or something waypoint time needs to be longer.
		bs->wpSeenTime = level.time + 30000;
		bs->wpTravelTime = level.time + 30000;
	}
	else if (bs->wpCurrent->flags & WPFLAG_NOVIS)
	{
		//10 sec
		bs->wpSeenTime = level.time + 10000;
		bs->wpTravelTime = level.time + 10000;
	}
	else
	{
		//3 sec visual time
		bs->wpSeenTime = level.time + 3000;

		//10 sec move time
		bs->wpTravelTime = level.time + 10000;
	}

	if (bs->wpCurrent->index == bs->wpDestination->index)
	{
		//just touched our destination node
		bs->wpTouchedDest = qtrue;
	}
	else
	{
		//reset the final node touched flag
		bs->wpTouchedDest = qfalse;
	}

	bs->AltRouteCheck = qfalse;
	bs->DontSpamPushPull = 0;
}

//wpDirection
//0 == FORWARD
//1 == BACKWARD

//see if this is a valid waypoint to pick up in our
//current state (whatever that may be)
static int pass_way_check(const bot_state_t* bs, const int windex)
{
	if (!gWPArray[windex] || !gWPArray[windex]->inuse)
	{
		//bad point index
		return 0;
	}

	if (RMG.integer)
	{
		if (gWPArray[windex]->flags & WPFLAG_RED_FLAG ||
			gWPArray[windex]->flags & WPFLAG_BLUE_FLAG)
		{
			//red or blue flag, we'd like to get here
			return 1;
		}
	}

	if (bs->wpDirection && gWPArray[windex]->flags & WPFLAG_ONEWAY_FWD)
	{
		//we're not travelling in a direction on the trail that will allow us to pass this point
		return 0;
	}
	if (!bs->wpDirection && gWPArray[windex]->flags & WPFLAG_ONEWAY_BACK)
	{
		//we're not travelling in a direction on the trail that will allow us to pass this point
		return 0;
	}

	if (bs->wpCurrent && gWPArray[windex]->forceJumpTo &&
		gWPArray[windex]->origin[2] > bs->wpCurrent->origin[2] + 64 &&
		bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] < gWPArray[windex]->forceJumpTo)
	{
		//waypoint requires force jump level greater than our current one to pass
		return 0;
	}

	return 1;
}

//tally up the distance between two waypoints
static float total_trail_distance(const int start, const int end)
{
	int beginat;
	int endat;

	float distancetotal = 0;

	if (start > end)
	{
		beginat = end;
		endat = start;
	}
	else
	{
		beginat = start;
		endat = end;
	}

	while (beginat < endat)
	{
		if (beginat >= gWPNum || !gWPArray[beginat] || !gWPArray[beginat]->inuse)
		{
			//invalid waypoint index
			return -1;
		}

		if (!RMG.integer)
		{
			if (end > start && gWPArray[beginat]->flags & WPFLAG_ONEWAY_BACK ||
				start > end && gWPArray[beginat]->flags & WPFLAG_ONEWAY_FWD)
			{
				//a one-way point, this means this path cannot be travelled to the final point
				return -1;
			}
		}

#if 0 //disabled force jump checks for now
		if (gWPArray[beginat]->forceJumpTo)
		{
			if (gWPArray[beginat - 1] && gWPArray[beginat - 1]->origin[2] + 64 < gWPArray[beginat]->origin[2])
			{
				gdif = gWPArray[beginat]->origin[2] - gWPArray[beginat - 1]->origin[2];
			}

			if (gdif)
			{
				if (bs && bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] < gWPArray[beginat]->forceJumpTo)
				{
					return -1;
				}
			}
		}

		if (bs->wpCurrent && gWPArray[windex]->forceJumpTo &&
			gWPArray[windex]->origin[2] > (bs->wpCurrent->origin[2] + 64) &&
			bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] < gWPArray[windex]->forceJumpTo)
		{
			return -1;
		}
#endif

		distancetotal += gWPArray[beginat]->disttonext;

		beginat++;
	}

	return distancetotal;
}

//see if there's a route shorter than our current one to get
//to the final destination we currently desire
static void check_for_shorter_routes(bot_state_t* bs, const int newwpindex)
{
	int i = 0;
	int fj = 0;

	if (!bs->wpDestination)
	{
		return;
	}

	//set our traversal direction based on the index of the point
	if (newwpindex < bs->wpDestination->index)
	{
		bs->wpDirection = 0;
	}
	else if (newwpindex > bs->wpDestination->index)
	{
		bs->wpDirection = 1;
	}

	//can't switch again yet
	if (bs->wpSwitchTime > level.time)
	{
		return;
	}

	//no neighboring points to check off of
	if (!gWPArray[newwpindex]->neighbornum)
	{
		return;
	}

	//get the trail distance for our wp
	int bestindex = newwpindex;
	float bestlen = total_trail_distance(newwpindex, bs->wpDestination->index);

	while (i < gWPArray[newwpindex]->neighbornum)
	{
		//now go through the neighbors and check the distance to the desired point from each neighbor
		const float checklen = total_trail_distance(gWPArray[newwpindex]->neighbors[i].num, bs->wpDestination->index);

		if (checklen < bestlen - 64 || bestlen == -1)
		{
			//this path covers less distance, let's take it instead
			if (bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] >= gWPArray[newwpindex]->neighbors[i].forceJumpTo)
			{
				bestlen = checklen;
				bestindex = gWPArray[newwpindex]->neighbors[i].num;

				if (gWPArray[newwpindex]->neighbors[i].forceJumpTo)
				{
					fj = gWPArray[newwpindex]->neighbors[i].forceJumpTo;
				}
				else
				{
					fj = 0;
				}
			}
		}

		i++;
	}

	if (bestindex != newwpindex && bestindex != -1)
	{
		//we found a path we want to switch to, let's do it
		bs->wpCurrent = gWPArray[bestindex];
		bs->wpSwitchTime = level.time + 3000;

		if (fj)
		{
			//do we have to force jump to get to this neighbor?
#ifndef FORCEJUMP_INSTANTMETHOD
			bs->forceJumpChargeTime = level.time + 1000;
			bs->beStill = level.time + 1000;
			bs->forceJumping = bs->forceJumpChargeTime;
#else
			bs->beStill = level.time + 500;
			bs->jumpTime = level.time + fj * 1200;
			bs->jDelay = level.time + 200;
			bs->forceJumping = bs->jumpTime;
#endif
		}
	}
}

//Find the origin location of a given entity
static void find_origins(const gentity_t* ent, vec3_t origin)
{
	if (!ent->classname)
	{
		//some funky entity, just set vec3_origin
		VectorCopy(vec3_origin, origin);
		return;
	}

	if (ent->client)
	{
		VectorCopy(ent->client->ps.origin, origin);
	}
	else
	{
		if (strcmp(ent->classname, "func_breakable") == 0
			|| strcmp(ent->classname, "trigger_multiple") == 0
			|| strcmp(ent->classname, "trigger_once") == 0
			|| strcmp(ent->classname, "func_usable") == 0)
		{
			//func_breakable's don't have normal origin data
			origin[0] = (ent->r.absmax[0] + ent->r.absmin[0]) / 2;
			origin[1] = (ent->r.absmax[1] + ent->r.absmin[1]) / 2;
			origin[2] = (ent->r.absmax[2] + ent->r.absmin[2]) / 2;
		}
		else
		{
			VectorCopy(ent->r.currentOrigin, origin);
		}
	}
}

static float target_distances(const bot_state_t* bs, const gentity_t* target, vec3_t targetorigin)
{
	if (strcmp(target->classname, "misc_siege_item") == 0
		|| strcmp(target->classname, "func_breakable") == 0
		|| target->client && target->client->NPC_class == CLASS_RANCOR)
	{
		vec3_t enemy_origin;
		//flatten origin heights and measure
		VectorCopy(targetorigin, enemy_origin);
		if (fabs(enemy_origin[2] - bs->eye[2]) < 150)
		{
			//don't flatten unless you're on the same relative plane
			enemy_origin[2] = bs->eye[2];
		}

		if (target->client && target->client->NPC_class == CLASS_RANCOR)
		{
			//Rancors are big and stuff
			return Distance(bs->eye, enemy_origin) - 60;
		}
		if (strcmp(target->classname, "misc_siege_item") == 0)
		{
			//assume this is a misc_siege_item.  These have absolute based mins/maxs.
			//Scale for the entity's bounding box
			float adjustor;
			const float x = fabs(bs->eye[0] - enemy_origin[0]);
			const float y = fabs(bs->eye[1] - enemy_origin[1]);
			const float z = fabs(bs->eye[2] - enemy_origin[2]);

			//find the general direction of the impact to determine which bbox length to
			//scale with
			if (x > y && x > z)
			{
				//x
				adjustor = target->r.maxs[0];
			}
			else if (y > x && y > z)
			{
				//y
				adjustor = target->r.maxs[1];
			}
			else
			{
				//z
				adjustor = target->r.maxs[2];
			}

			return Distance(bs->eye, enemy_origin) - adjustor + 15;
		}
		if (strcmp(target->classname, "func_breakable") == 0)
		{
			//Scale for the entity's bounding box
			float adjustor;

			//find the smallest min/max and use that.
			if (target->r.absmax[0] - enemy_origin[0] < target->r.absmax[1] - enemy_origin[1])
			{
				adjustor = target->r.absmax[0] - enemy_origin[0];
			}
			else
			{
				adjustor = target->r.absmax[1] - enemy_origin[1];
			}

			return Distance(bs->eye, enemy_origin) - adjustor + 15;
		}
		//func_breakable
		return Distance(bs->eye, enemy_origin);
	}
	//standard stuff
	return Distance(bs->eye, targetorigin);
}

static qboolean isa_heavy_weapon(const bot_state_t* bs, const int weapon)
{
	//right now we only show positive for weapons that can do this in primary fire mode
	switch (weapon)
	{
	case WP_MELEE:
		if (G_HeavyMelee(&g_entities[bs->client]))
		{
			return qtrue;
		}
		break;

	case WP_SABER:
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
	case WP_DET_PACK:
	case WP_CONCUSSION:
	case WP_FLECHETTE:
		return qtrue;
	default:;
	}

	return qfalse;
}

static int favorite_weapons(const bot_state_t* bs, const gentity_t* target, const qboolean have_check,
	const qboolean ammo_check,
	const int ignore_weaps)
{
	int bestweight = -1;
	int bestweapon = 0;

	for (int i = 0; i < WP_NUM_WEAPONS; i++)
	{
		if (have_check &&
			!(bs->cur_ps.stats[STAT_WEAPONS] & 1 << i))
		{
			//check to see if we actually have this weapon.
			continue;
		}

		if (ammo_check &&
			bs->cur_ps.ammo[weaponData[i].ammoIndex] < weaponData[i].energyPerShot)
		{
			//check to see if we have enough ammo for this weapon.
			continue;
		}

		if (ignore_weaps & 1 << i)
		{
			//check to see if this weapon is on our ignored weapons list
			continue;
		}

		if (target && target->inuse)
		{
			//do additional weapon checks based on our target's attributes.
			if (target->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)
			{
				//don't use weapons that can't damage this target
				if (!isa_heavy_weapon(bs, i))
				{
					continue;
				}
			}

			//try to use explosives on breakables if we can.
			if (strcmp(target->classname, "func_breakable") == 0)
			{
				if (i == WP_DET_PACK)
				{
					bestweight = 100;
					bestweapon = i;
				}
				else if (i == WP_THERMAL)
				{
					bestweight = 99;
					bestweapon = i;
				}
				else if (i == WP_ROCKET_LAUNCHER)
				{
					bestweight = 99;
					bestweapon = i;
				}
				else if (bs->botWeaponWeights[i] > bestweight)
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
			else
			{
				//no special requirements for this target.
				if (bs->botWeaponWeights[i] > bestweight)
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
		}
		else
		{
			//no target
			if (bs->botWeaponWeights[i] > bestweight)
			{
				bestweight = bs->botWeaponWeights[i];
				bestweapon = i;
			}
		}
	}

	return bestweapon;
}

//Find the number of players useing this basic class on team.
static int numberof_siege_basic_classes(const int team, const int base_class)
{
	const siegeClass_t* hold_class = BG_GetClassOnBaseClass(team, base_class, 0);
	int num_players = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		const gentity_t* ent = &g_entities[i];
		if (ent && ent->client && ent->client->pers.connected == CON_CONNECTED
			&& ent->client->sess.siegeClass && ent->client->sess.sessionTeam == team)
		{
			if (strcmp(ent->client->sess.siegeClass, hold_class->name) == 0)
			{
				num_players++;
			}
		}
	}
	return num_players;
}

static qboolean have_heavy_weapons(const int weapons)
{
	if (weapons & 1 << WP_SABER
		|| weapons & 1 << WP_ROCKET_LAUNCHER
		|| weapons & 1 << WP_THERMAL
		|| weapons & 1 << WP_TRIP_MINE
		|| weapons & 1 << WP_DET_PACK
		|| weapons & 1 << WP_CONCUSSION
		|| weapons & 1 << WP_FLECHETTE)
	{
		return qtrue;
	}
	return qfalse;
}

static int find_heavy_weapon_classse(const int team, const int index, const qboolean saber)
{
	int num_heavy_weap_classes = 0;

	const siegeTeam_t* stm = BG_SiegeFindThemeForTeam(team);
	if (!stm)
	{
		return -1;
	}

	// Loop through all the classes for this team
	for (int i = 0; i < stm->numClasses; i++)
	{
		if (!saber)
		{
			if (have_heavy_weapons(stm->classes[i]->weapons))
			{
				if (index == num_heavy_weap_classes)
				{
					return stm->classes[i]->playerClass;
				}
				num_heavy_weap_classes++;
			}
		}
		else
		{
			//look for saber
			if (stm->classes[i]->weapons & 1 << WP_SABER)
			{
				if (index == num_heavy_weap_classes)
				{
					return stm->classes[i]->playerClass;
				}
				num_heavy_weap_classes++;
			}
		}
	}

	//no heavy weapons/saber carrying units at this index
	return -1;
}

void request_siege_assistance(bot_state_t* bs, const int base_class)
{
	if (bs->vchatTime > level.time)
	{
		//recently did vchat, don't do it now.
		return;
	}
	switch (base_class)
	{
	case SPC_DEMOLITIONIST:
		trap->EA_Command(bs->client, "voice_cmd req_demo");
		bs->vchatTime = level.time + Q_irand(25000, 45000);
		break;
	case SPC_SUPPORT:
		trap->EA_Command(bs->client, "voice_cmd req_tech");
		bs->vchatTime = level.time + Q_irand(25000, 45000);
		break;
	case SPOTTED_SNIPER:
		trap->EA_Command(bs->client, "voice_cmd spot_sniper");
		bs->vchatTime = level.time + Q_irand(25000, 45000);
		break;
	case SPOTTED_TROOPS:
		trap->EA_Command(bs->client, "voice_cmd spot_troops");
		bs->vchatTime = level.time + Q_irand(12000, 30000);
		break;
	case REQUEST_SUPPLIES:
		trap->EA_Command(bs->client, "voice_cmd req_sup");
		bs->vchatTime = level.time + Q_irand(60000, 75000);
		break;
	case REQUEST_ASSISTANCE:
		trap->EA_Command(bs->client, "voice_cmd req_assist");
		bs->vchatTime = level.time + Q_irand(15000, 30000);
		break;
	case REQUEST_MEDIC:
		trap->EA_Command(bs->client, "voice_cmd req_medic");
		bs->vchatTime = level.time + Q_irand(5000, 10000);
		break;
	case TACTICAL_COVERME:
		trap->EA_Command(bs->client, "voice_cmd tac_cover");
		bs->vchatTime = level.time + Q_irand(10000, 25000);
		break;
	case TACTICAL_HOLDPOSITION:
		trap->EA_Command(bs->client, "voice_cmd tac_hold");
		bs->vchatTime = level.time + Q_irand(5000, 12000);
		break;
	case TACTICAL_FOLLOW:
		trap->EA_Command(bs->client, "voice_cmd tac_follow");
		bs->vchatTime = level.time + Q_irand(25000, 45000);
		break;
	case REPLY_YES:
		trap->EA_Command(bs->client, "voice_cmd reply_yes");
		bs->vchatTime = level.time + Q_irand(2000, 4000);
		break;
	case REPLY_COMING:
		trap->EA_Command(bs->client, "voice_cmd reply_coming");
		bs->vchatTime = level.time + Q_irand(2000, 4000);
		break;
	default:;
	}
}

//Should we switch classes to destroy this breakable or just call for help?
//saber = saber only destroyable?
static void should_switcha_siege_classes(bot_state_t* bs, const qboolean saber)
{
	int i = 0;

	int class_num = find_heavy_weapon_classse(g_entities[bs->client].client->sess.siegeDesiredTeam, i, saber);
	while (class_num != -1)
	{
		const int x = numberof_siege_basic_classes(g_entities[bs->client].client->sess.siegeDesiredTeam, class_num);
		if (x)
		{
			//request assistance for this class since we already have someone
			//playing that class
			request_siege_assistance(bs, SPC_DEMOLITIONIST);
			return;
		}

		//ok, noone is using that class check for the next
		//indexed heavy weapon class
		i++;
		class_num = find_heavy_weapon_classse(g_entities[bs->client].client->sess.siegeDesiredTeam, i, saber);
	}

	//ok, noone else is using a siege class with a heavyweapon.  Switch to
	//one ourselves
	i--;
	class_num = find_heavy_weapon_classse(g_entities[bs->client].client->sess.siegeDesiredTeam, i, saber);

	if (class_num == -1)
	{
		//what the?!
		//G_Printf("couldn't find a siege class with a heavy weapon in ShouldSwitchaSiegeClasses().\n");
	}
	else
	{
		//switch to this class
		siegeClass_t* hold_class = BG_GetClassOnBaseClass(g_entities[bs->client].client->sess.siegeDesiredTeam,
			class_num,
			0);
		trap->EA_Command(bs->client, va("siegeclass \"%s\"\n", hold_class->name));
	}
}

//check/select the chosen weapon
int bot_select_choice_weapon(bot_state_t* bs, const int weapon, const int doselection)
{
	int hasit = 0;

	int i = 0;

	while (i < WP_NUM_WEAPONS)
	{
		if (bs->cur_ps.ammo[weaponData[i].ammoIndex] >= weaponData[i].energyPerShot &&
			i == weapon &&
			bs->cur_ps.stats[STAT_WEAPONS] & 1 << i)
		{
			hasit = 1;
			break;
		}

		i++;
	}

	if (hasit && bs->cur_ps.weapon != weapon && doselection && bs->virtualWeapon != weapon)
	{
		bs->virtualWeapon = weapon;
		bot_select_weapon(bs->client, weapon);
		return 2;
	}

	if (hasit)
	{
		return 1;
	}

	return 0;
}

static qboolean attack_local_breakables(bot_state_t* bs)
{
	gentity_t* test = NULL;
	const gentity_t* valid = NULL;
	qboolean defend = qfalse;
	vec3_t testorigin;

	while ((test = G_Find(test, FOFS(classname), "func_breakable")) != NULL)
	{
		find_origins(test, testorigin);

		if (target_distances(bs, test, testorigin) < 300)
		{
			if (test->teamnodmg && test->teamnodmg == g_entities[bs->client].client->sess.sessionTeam)
			{
				//on a team that can't damage this breakable, as such defend it from immediate harm
				defend = qtrue;
			}

			valid = test;
			break;
		}

		//reset for next check
		VectorClear(testorigin);
	}

	if (valid)
	{
		//due to crazy stack overflow issues, just attack wildly while moving towards the
		//breakable
		trace_t tr;
		const int desiredweap = favorite_weapons(bs, valid, qtrue, qtrue, 0);

		//visual check
		trap->Trace(&tr, bs->eye, NULL, NULL, testorigin, bs->client, MASK_SOLID, qfalse, 0, 0);

		if (tr.entityNum == test->s.number || tr.fraction == 1.0)
		{
			//we can see the breakable
			//doing special wp move
			bs->wpSpecial = qtrue;

			if (defend)
			{
				//defend this target since we can assume that the other team is going to try to
				//destroy this thingy
				siege_defend_from_attackers(bs);
			}
			else if (test->flags & FL_DMG_BY_HEAVY_WEAP_ONLY && !isa_heavy_weapon(bs, desiredweap))
			{
				//we currently don't have a heavy weap that we can use to destroy this target
				if (have_heavy_weapons(bs->cur_ps.stats[STAT_WEAPONS]))
				{
					//we have a weapon that could destroy this target but we don't have ammo
					//RAFIXME:  at this point we should have the bot go look for some ammo
					//but for now just defend this area.
					siege_defend_from_attackers(bs);
				}
				else if (level.gametype == GT_SIEGE)
				{
					//ok, check to see if we should switch classes if noone else can blast this
					should_switcha_siege_classes(bs, qfalse);
					siege_defend_from_attackers(bs);
				}
				else
				{
					//go hunting for a weapon that can destroy this object
					//RAFIXME:  Add this code
					siege_defend_from_attackers(bs);
				}
			}
			else if (test->flags & FL_DMG_BY_SABER_ONLY && !(bs->cur_ps.stats[STAT_WEAPONS] & 1 << WP_SABER))
			{
				//This is only damaged by sabers and we don't have a saber
				should_switcha_siege_classes(bs, qtrue);
				siege_defend_from_attackers(bs);
			}
			else
			{
				//ATTACK!
				//determine which weapon you want to use
				if (desiredweap != bs->virtualWeapon)
				{
					//need to switch to desired weapon
					bot_select_choice_weapon(bs, desiredweap, qtrue);
				}
				//set visible flag so we'll attack this.
				bs->frame_Enemy_Vis = 1;
				trap->EA_Attack(bs->client);
			}

			return qtrue;
		}
	}
	return qfalse;
}

static qboolean use_forceon_locals(bot_state_t* bs, vec3_t origin, const qboolean pull)
{
	gentity_t* test = NULL;
	vec3_t center;
	qboolean skip_push_pull = qfalse;

	if (bs->DontSpamPushPull > level.time)
	{
		//pushed/pulled for this waypoint recently
		skip_push_pull = qtrue;
	}

	if (!skip_push_pull)
	{
		//Force Push/Pull
		while ((test = G_Find(test, FOFS(classname), "func_door")) != NULL)
		{
			vec3_t pos2;
			vec3_t pos1;
			if (!(test->spawnflags & 2))
			{
				//can't use the Force to move this door, ignore
				continue;
			}

			if (test->wait < 0 && test->moverState == MOVER_POS2)
			{
				//locked in position already, ignore.
				continue;
			}

			//find center, pos1, pos2
			if (VectorCompare(vec3_origin, test->s.origin))
			{
				vec3_t size;
				//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
				VectorSubtract(test->r.absmax, test->r.absmin, size);
				VectorMA(test->r.absmin, 0.5, size, center);
				if (test->spawnflags & 1 && test->moverState == MOVER_POS1)
				{
					//if at pos1 and started open, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
					VectorSubtract(center, test->pos1, center);
				}
				else if (!(test->spawnflags & 1) && test->moverState == MOVER_POS2)
				{
					//if at pos2, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
					VectorSubtract(center, test->pos2, center);
				}
				VectorAdd(center, test->pos1, pos1);
				VectorAdd(center, test->pos2, pos2);
			}
			else
			{
				//actually has an origin, pos1 and pos2 are absolute
				VectorCopy(test->r.currentOrigin, center);
				VectorCopy(test->pos1, pos1);
				VectorCopy(test->pos2, pos2);
			}

			if (Distance(origin, center) > 400)
			{
				//too far away
				continue;
			}

			if (Distance(pos1, bs->eye) < Distance(pos2, bs->eye))
			{
				//pos1 is closer
				if (test->moverState == MOVER_POS1)
				{
					//at the closest pos
					if (pull)
					{
						//trying to pull, but already at closest point, so screw it
						continue;
					}
				}
				else if (test->moverState == MOVER_POS2)
				{
					//at farthest pos
					if (!pull)
					{
						//trying to push, but already at farthest point, so screw it
						continue;
					}
				}
			}
			else
			{
				//pos2 is closer
				if (test->moverState == MOVER_POS1)
				{
					//at the farthest pos
					if (!pull)
					{
						//trying to push, but already at farthest point, so screw it
						continue;
					}
				}
				else if (test->moverState == MOVER_POS2)
				{
					//at closest pos
					if (pull)
					{
						//trying to pull, but already at closest point, so screw it
						continue;
					}
				}
			}
			//we have a winner!
			break;
		}

		if (test)
		{
			//have a push/pull able door
			vec3_t view_dir;
			vec3_t ang;

			//doing special wp move
			bs->wpSpecial = qtrue;

			VectorSubtract(center, bs->eye, view_dir);
			vectoangles(view_dir, ang);
			VectorCopy(ang, bs->goalAngles);

			if (in_field_of_vision(bs->viewangles, 5, ang))
			{
				//use the force
				if (pull)
				{
					bs->doForcePull = level.time + 700;
				}
				else
				{
					bs->doForcePush = level.time + 700;
				}
				//Only press the button once every 15 seconds
				//this way the bots will be able to go up/down a lift weither the elevator
				//was up or down.
				//This debounce only applies to this waypoint.
				bs->DontSpamPushPull = level.time + 15000;
			}
			return qtrue;
		}
	}

	if (bs->DontSpamButton > level.time)
	{
		//pressed a button recently
		if (bs->useTime > level.time)
		{
			//holding down button
			//update the hacking buttons to account for lag about crap
			if (g_entities[bs->client].client->isHacking)
			{
				bs->useTime = bs->cur_ps.hackingTime + 100;
				bs->DontSpamButton = bs->useTime + 15000;
				bs->wpSpecial = qtrue;
				return qtrue;
			}

			//lost hack target.  clear things out
			bs->useTime = level.time;
			return qfalse;
		}
		return qfalse;
	}

	//use button checking
	while ((test = G_Find(test, FOFS(classname), "trigger_multiple")) != NULL)
	{
		if (test->flags & FL_INACTIVE)
		{
			//set by target_deactivate
			continue;
		}

		if (!(test->spawnflags & 4))
		{
			//can't use button this trigger
			continue;
		}

		if (test->alliedTeam)
		{
			if (g_entities[bs->client].client->sess.sessionTeam != test->alliedTeam)
			{
				//not useable by this team
				continue;
			}
		}

		find_origins(test, center);

		if (Distance(origin, center) > 200)
		{
			//too far away
			continue;
		}

		break;
	}

	if (!test)
	{
		//not luck so far
		while ((test = G_Find(test, FOFS(classname), "trigger_once")) != NULL)
		{
			if (test->flags & FL_INACTIVE)
			{
				//set by target_deactivate
				continue;
			}

			if (!test->use)
			{
				//trigger already fired
				continue;
			}

			if (!(test->spawnflags & 4))
			{
				//can't use button this trigger
				continue;
			}

			if (test->alliedTeam)
			{
				if (g_entities[bs->client].client->sess.sessionTeam != test->alliedTeam)
				{
					//not useable by this team
					continue;
				}
			}

			find_origins(test, center);

			if (Distance(origin, center) > 200)
			{
				//too far away
				continue;
			}

			break;
		}
	}

	if (test)
	{
		vec3_t view_dir;
		vec3_t ang;
		bs->wpSpecial = qtrue;

		//you can use use
		//set view angles.
		if (test->spawnflags & 2)
		{
			//you have to face in the direction of the trigger to have it work
			vectoangles(test->movedir, ang);
		}
		else
		{
			VectorSubtract(center, bs->eye, view_dir);
			vectoangles(view_dir, ang);
		}
		VectorCopy(ang, bs->goalAngles);

		if (G_PointInBounds(bs->origin, test->r.absmin, test->r.absmax))
		{
			//inside trigger zone, press use.
			bs->useTime = level.time + test->genericValue7 + 100;
			bs->DontSpamButton = bs->useTime + 15000;
		}
		else
		{
			//need to move closer
			VectorSubtract(center, bs->origin, view_dir);

			view_dir[2] = 0;
			VectorNormalize(view_dir);

			trap->EA_Move(bs->client, view_dir, 5000);
		}
		return qtrue;
	}
	return qfalse;
}

static void wp_visible_updates(bot_state_t* bs)
{
	if (org_visible_box(bs->origin, NULL, NULL, bs->wpCurrent->origin, bs->client))
	{
		//see the waypoint hold the counter
		bs->wpSeenTime = level.time + 3000;
	}
}

//check for flags on the waypoint we're currently traveling to
//and perform the desired behavior based on the flag
static void wp_constant_routine(bot_state_t* bs)
{
	if (!bs->wpCurrent)
	{
		return;
	}

	if (bs->wpCurrent->flags & WPFLAG_DUCK)
	{
		//duck while traveling to this point
		bs->duckTime = level.time + 100;
	}

	if (bs->wpCurrent->flags & WPFLAG_FORCEPUSH)
	{
		if (use_forceon_locals(bs, bs->wpCurrent->origin, qfalse))
		{
			//found something we can Force Push
			bs->wpSeenTime = level.time + 1000;
			return;
		}
	}
	if (bs->wpCurrent->flags & WPFLAG_FORCEPULL)
	{
		if (use_forceon_locals(bs, bs->wpCurrent->origin, qtrue))
		{
			//found something we can Force Pull
			wp_visible_updates(bs);
			return;
		}
	}

#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->wpCurrent->flags & WPFLAG_JUMP)
	{
		//jump while traveling to this point
		float height_dif = bs->wpCurrent->origin[2] - bs->origin[2] + 16;

		if (bs->origin[2] + 16 >= bs->wpCurrent->origin[2])
		{
			//don't need to jump, we're already higher than this point
			height_dif = 0;
		}
		if (height_dif > 128 && bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
		{
			// Jet packer.. Jetpack ON!
			bs->cur_ps.eFlags = PM_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->cur_ps.eFlags |= EF3_JETPACK_HOVER;
			bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
		}

		if (height_dif > 40 && bs->cur_ps.fd.forcePowersKnown & 1 << FP_LEVITATION && (bs->cur_ps.fd.forceJumpCharge
			< forceJumpStrength[bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION]] - 100 || bs->cur_ps.groundEntityNum ==
			ENTITYNUM_NONE))
		{
			//alright, let's jump
			bs->forceJumpChargeTime = level.time + 1000;
			if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE && bs->jumpPrep < level.time - 300)
			{
				bs->jumpPrep = level.time + 700;
			}
			bs->beStill = level.time + 300;
			bs->jumpTime = 0;

			if (bs->wpSeenTime < level.time + 600)
			{
				bs->wpSeenTime = level.time + 600;
			}
		}
		else if (height_dif > 64 && !(bs->cur_ps.fd.forcePowersKnown & 1 << FP_LEVITATION))
		{
			//this point needs force jump to reach and we don't have it
			//Kill the current point and turn around
			bs->wpCurrent = NULL;
			if (bs->wpDirection)
			{
				bs->wpDirection = 0;
			}
			else
			{
				bs->wpDirection = 1;
			}

			return;
		}
	}
#endif

	if (bs->wpCurrent->forceJumpTo)
	{
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
		{
			bs->cur_ps.eFlags = PM_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->cur_ps.eFlags |= EF3_JETPACK_HOVER;
			bs->jumpHoldTime = ((bs->forceJumpChargeTime + level.time) / 2) + 50000;

			if (bs->origin[2] + 16 < bs->wpCurrent->origin[2])
			{
				bs->jumpTime = level.time + 100;
			}
		}
		else
		{
			if (bs->cur_ps.fd.forceJumpCharge < forceJumpStrength[bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION]] - 100)
			{
				bs->forceJumpChargeTime = level.time + 200;
			}
		}
	}
}

//check if our ctf state is to guard the base
static qboolean BotCTFGuardDuty(const bot_state_t* bs)
{
	if (level.gametype != GT_CTF && level.gametype != GT_CTY)
	{
		return qfalse;
	}

	if (bs->ctfState == CTFSTATE_DEFENDER)
	{
		return qtrue;
	}

	return qfalse;
}

//when we reach the waypoint we are travelling to,
//this function will be called. We will perform any
//checks for flags on the current wp and activate
//any "touch" events based on that.
static void WPTouchRoutine(bot_state_t* bs)
{
	if (!bs->wpCurrent)
	{
		return;
	}

	bs->wpTravelTime = level.time + 10000;

	if (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
	{
		//don't try to use any nearby map objects for a little while
		bs->noUseTime = level.time + 4000;
	}

#ifdef FORCEJUMP_INSTANTMETHOD
	if ((bs->wpCurrent->flags & WPFLAG_JUMP) && bs->wpCurrent->forceJumpTo)
	{ //jump if we're flagged to but not if this indicates a force jump point. Force jumping is
	  //handled elsewhere.
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
		{
			bs->cur_ps.eFlags = PM_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->cur_ps.eFlags |= EF3_JETPACK_HOVER;
			bs->jumpHoldTime = ((bs->forceJumpChargeTime + level.time) / 2) + 50000;
		}
		bs->jumpTime = level.time + 100;
	}
#else
	if (bs->wpCurrent->flags & WPFLAG_JUMP && !bs->wpCurrent->forceJumpTo)
	{
		//jump if we're flagged to but not if this indicates a force jump point. Force jumping is
		//handled elsewhere.
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
		{
			bs->cur_ps.eFlags = PM_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->cur_ps.eFlags |= EF3_JETPACK_HOVER;
			bs->jumpHoldTime = 50000;
		}
		bs->jumpTime = level.time + 100;
	}
#endif

	if (bs->isCamper && bot_camp.integer && (bot_is_a_chicken_wuss(bs) || BotCTFGuardDuty(bs) || bs->isCamper == 2) && (
		bs
		->wpCurrent->flags & WPFLAG_SNIPEORCAMP || bs->wpCurrent->flags & WPFLAG_SNIPEORCAMPSTAND) &&
		bs->cur_ps.weapon != WP_SABER && bs->cur_ps.weapon != WP_MELEE && bs->cur_ps.weapon != WP_STUN_BATON)
	{
		int last_num;
		//if we're a camper and a chicken then camp
		if (bs->wpDirection)
		{
			last_num = bs->wpCurrent->index + 1;
		}
		else
		{
			last_num = bs->wpCurrent->index - 1;
		}

		if (gWPArray[last_num] && gWPArray[last_num]->inuse && gWPArray[last_num]->index && bs->isCamping < level.time)
		{
			bs->isCamping = level.time + rand() % 15000 + 30000;
			bs->wpCamping = bs->wpCurrent;
			bs->wpCampingTo = gWPArray[last_num];

			if (bs->wpCurrent->flags & WPFLAG_SNIPEORCAMPSTAND)
			{
				bs->campStanding = qtrue;
			}
			else
			{
				bs->campStanding = qfalse;
			}
		}
	}
	else if ((bs->cur_ps.weapon == WP_SABER || bs->cur_ps.weapon == WP_STUN_BATON || bs->cur_ps.weapon == WP_MELEE) &&
		bs->isCamping > level.time)
	{
		//don't snipe/camp with a melee weapon, that would be silly
		bs->isCamping = 0;
		bs->wpCampingTo = NULL;
		bs->wpCamping = NULL;
	}

	if (bs->wpDestination)
	{
		if (bs->wpCurrent->index == bs->wpDestination->index)
		{
			bs->wpDestination = NULL;

			if (bs->runningLikeASissy)
			{
				//this obviously means we're scared and running, so we'll want to keep our navigational priorities less delayed
				bs->destinationGrabTime = level.time + 500;
			}
			else
			{
				bs->destinationGrabTime = level.time + 3500;
			}
		}
		else
		{
			check_for_shorter_routes(bs, bs->wpCurrent->index);
		}
	}
}

//could also slowly lerp toward, but for now
//just copying straight over.
static void move_toward_ideal_angles(bot_state_t* bs)
{
	VectorCopy(bs->goalAngles, bs->ideal_viewangles);
}

#define BOT_STRAFE_AVOIDANCE

#ifdef BOT_STRAFE_AVOIDANCE
#define STRAFEAROUND_RIGHT			1
#define STRAFEAROUND_LEFT			2

//do some trace checks for strafing to get an idea of where we
//are and if we should move to avoid obstacles.
static int bot_trace_strafe(const bot_state_t* bs, vec3_t traceto)
{
	const vec3_t player_mins = { -15, -15, -8 };
	const vec3_t player_maxs = { 15, 15, DEFAULT_MAXS_2 };
	vec3_t from, to;
	vec3_t dir_ang, dir_dif;
	vec3_t forward, right;
	trace_t tr;

	if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//don't do this in the air, it can be.. dangerous.
		return 0;
	}

	VectorSubtract(traceto, bs->origin, dir_ang);
	VectorNormalize(dir_ang);
	vectoangles(dir_ang, dir_ang);

	if (angle_difference(bs->viewangles[YAW], dir_ang[YAW]) > 60 ||
		angle_difference(bs->viewangles[YAW], dir_ang[YAW]) < -60)
	{
		//If we aren't facing the direction we're going here, then we've got enough excuse to be too stupid to strafe around anyway
		return 0;
	}

	VectorCopy(bs->origin, from);
	VectorCopy(traceto, to);

	VectorSubtract(to, from, dir_dif);
	VectorNormalize(dir_dif);
	vectoangles(dir_dif, dir_dif);

	AngleVectors(dir_dif, forward, 0, 0);

	to[0] = from[0] + forward[0] * 32;
	to[1] = from[1] + forward[1] * 32;
	to[2] = from[2] + forward[2] * 32;

	trap->Trace(&tr, from, player_mins, player_maxs, to, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return 0;
	}

	AngleVectors(dir_ang, 0, right, 0);

	from[0] += right[0] * 32;
	from[1] += right[1] * 32;
	from[2] += right[2] * 16;

	to[0] += right[0] * 32;
	to[1] += right[1] * 32;
	to[2] += right[2] * 32;

	trap->Trace(&tr, from, player_mins, player_maxs, to, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return STRAFEAROUND_RIGHT;
	}

	from[0] -= right[0] * 64;
	from[1] -= right[1] * 64;
	from[2] -= right[2] * 64;

	to[0] -= right[0] * 64;
	to[1] -= right[1] * 64;
	to[2] -= right[2] * 64;

	trap->Trace(&tr, from, player_mins, player_maxs, to, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return STRAFEAROUND_LEFT;
	}

	return 0;
}
#endif

//Similar to the trace check, but we want to trace to see
//if there's anything we can jump over.
static int bot_trace_jump(bot_state_t* bs, vec3_t traceto)
{
	vec3_t mins, maxs, a, fwd, traceto_mod, tracefrom_mod;
	trace_t tr;

	VectorSubtract(traceto, bs->origin, a);
	vectoangles(a, a);

	AngleVectors(a, fwd, NULL, NULL);

	traceto_mod[0] = bs->origin[0] + fwd[0] * 4;
	traceto_mod[1] = bs->origin[1] + fwd[1] * 4;
	traceto_mod[2] = bs->origin[2] + fwd[2] * 4;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -18;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	trap->Trace(&tr, bs->origin, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return 0;
	}

	const int or_tr = tr.entityNum;

	VectorCopy(bs->origin, tracefrom_mod);

	tracefrom_mod[2] += 41;
	traceto_mod[2] += 41;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 8;

	trap->Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		if (or_tr >= 0 && or_tr < MAX_CLIENTS && botstates[or_tr] && botstates[or_tr]->jumpTime > level.time)
		{
			return 0; //so bots don't try to jump over each other at the same time
		}
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
		{
			bs->cur_ps.eFlags = PM_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->cur_ps.eFlags |= EF3_JETPACK_HOVER;
			bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
		}

		if (bs->currentEnemy && bs->currentEnemy->s.number == or_tr && (bot_get_weapon_range(bs) == BWEAPONRANGE_SABER
			||
			bot_get_weapon_range(bs) == BWEAPONRANGE_MELEE))
		{
			if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
			{
				bs->cur_ps.eFlags = PM_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
				bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
				bs->cur_ps.eFlags |= EF3_JETPACK_HOVER;
				bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
			}
			else
			{
				return 0;
			}
		}

		return 1;
	}

	return 0;
}

//And yet another check to duck under any obstacles.
static int bot_trace_duck(const bot_state_t* bs, vec3_t traceto)
{
	vec3_t mins, maxs, a, fwd, traceto_mod, tracefrom_mod;
	trace_t tr;

	VectorSubtract(traceto, bs->origin, a);
	vectoangles(a, a);

	AngleVectors(a, fwd, NULL, NULL);

	traceto_mod[0] = bs->origin[0] + fwd[0] * 4;
	traceto_mod[1] = bs->origin[1] + fwd[1] * 4;
	traceto_mod[2] = bs->origin[2] + fwd[2] * 4;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -23;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 8;

	trap->Trace(&tr, bs->origin, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction != 1)
	{
		return 0;
	}

	VectorCopy(bs->origin, tracefrom_mod);

	tracefrom_mod[2] += 31; //33;
	traceto_mod[2] += 31; //33;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	trap->Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction != 1)
	{
		return 1;
	}

	return 0;
}

//check of the potential enemy is a valid one
static int pass_standard_enemy_checks(const bot_state_t* bs, const gentity_t* en)
{
	if (!bs || !en)
	{
		//shouldn't happen
		return 0;
	}

	if (!en->client)
	{
		//not a client, don't care about him
		return 0;
	}

	if (en->health < 1)
	{
		//he's already dead
		return 0;
	}

	if (!en->takedamage)
	{
		//a client that can't take damage?
		return 0;
	}

	if (IsSurrendering(en))
	{
		// Ignore players surrendering
		return 0;
	}

	if (IsRespecting(en))
	{
		// Ignore players surrendering
		return 0;
	}

	if (IsCowering(en))
	{
		// Ignore players surrendering
		return 0;
	}

	if (bs->doingFallback &&
		gLevelFlags & LEVELFLAG_IGNOREINFALLBACK)
	{
		return 0;
	}

	if (en->client->ps.pm_type == PM_INTERMISSION ||
		en->client->ps.pm_type == PM_SPECTATOR ||
		en->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		//don't attack spectators
		return 0;
	}

	if (!en->client->pers.connected)
	{
		//a "zombie" client?
		return 0;
	}

	if (!en->s.solid)
	{
		//shouldn't happen
		return 0;
	}

	if (bs->client == en->s.number)
	{
		//don't attack yourself
		return 0;
	}

	if (OnSameTeam(&g_entities[bs->client], en))
	{
		//don't attack teammates
		return 0;
	}

	if (bot_mind_tricked(bs->client, en->s.number))
	{
		if (bs->currentEnemy && bs->currentEnemy->s.number == en->s.number)
		{
			//if mindtricked by this enemy, then be less "aware" of them, even though
			//we know they're there.
			vec3_t vs;

			VectorSubtract(bs->origin, en->client->ps.origin, vs);
			const float v_len = VectorLength(vs);

			if (v_len > 64)
			{
				return 0;
			}
		}
	}

	if (en->client->ps.duelInProgress && en->client->ps.duelIndex != bs->client)
	{
		//don't attack duelists unless you're dueling them
		return 0;
	}

	if (bs->cur_ps.duelInProgress && en->s.number != bs->cur_ps.duelIndex)
	{
		//ditto, the other way around
		return 0;
	}

	if (level.gametype == GT_JEDIMASTER && !en->client->ps.isJediMaster && !bs->cur_ps.isJediMaster &&
		G_ThereIsAMaster())
	{
		//rules for attacking non-JM in JM mode
		vec3_t vs;

		if (!g_friendlyFire.integer)
		{
			//can't harm non-JM in JM mode if FF is off
			return 0;
		}

		VectorSubtract(bs->origin, en->client->ps.origin, vs);
		const float v_len = VectorLength(vs);

		if (v_len > 350)
		{
			return 0;
		}
	}

	return 1;
}

//Notifies the bot that he has taken damage from "attacker".
void bot_damage_notification(const gclient_t* bot, gentity_t* attacker)
{
	int i;

	if (!bot || !attacker || !attacker->client)
	{
		return;
	}

	if (bot->ps.clientNum >= MAX_CLIENTS)
	{
		//an NPC.. do nothing for them.
		return;
	}

	if (attacker->s.number >= MAX_CLIENTS)
	{
		//if attacker is an npc also don't care I suppose.
		return;
	}

	bot_state_t* bs_a = botstates[attacker->s.number];

	if (bs_a)
	{
		//if the client attacking us is a bot as well
		bs_a->lastAttacked = &g_entities[bot->ps.clientNum];
		i = 0;

		while (i < MAX_CLIENTS)
		{
			if (botstates[i] &&
				i != bs_a->client &&
				botstates[i]->lastAttacked == &g_entities[bot->ps.clientNum])
			{
				botstates[i]->lastAttacked = NULL;
			}

			i++;
		}
	}
	else //got attacked by a real client, so no one gets rights to lastAttacked
	{
		i = 0;

		while (i < MAX_CLIENTS)
		{
			if (botstates[i] &&
				botstates[i]->lastAttacked == &g_entities[bot->ps.clientNum])
			{
				botstates[i]->lastAttacked = NULL;
			}

			i++;
		}
	}

	bot_state_t* bs = botstates[bot->ps.clientNum];

	if (!bs)
	{
		return;
	}

	bs->lastHurt = attacker;

	if (bs->currentEnemy)
	{
		//we don't care about the guy attacking us if we have an enemy already
		return;
	}

	if (!pass_standard_enemy_checks(bs, attacker))
	{
		//the person that hurt us is not a valid enemy
		return;
	}

	if (pass_loved_one_check(bs, attacker))
	{
		//the person that hurt us is the one we love!
		bs->currentEnemy = attacker;
		bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
	}
}

//perform cheap "hearing" checks based on the event catching
//system
static int bot_can_hear(const bot_state_t* bs, const gentity_t* en, const float endist)
{
	float minlen;

	if (!en || !en->client)
	{
		return 0;
	}

	if (en && en->client && en->client->ps.otherSoundTime > level.time)
	{
		//they made a noise in recent time
		minlen = en->client->ps.otherSoundLen;
		goto checkStep;
	}

	if (en && en->client && en->client->ps.footstepTime > level.time)
	{
		//they made a footstep
		minlen = 256;
		goto checkStep;
	}

	if (gBotEventTracker[en->s.number].eventTime < level.time)
	{
		//no recent events to check
		return 0;
	}

	switch (gBotEventTracker[en->s.number].events[gBotEventTracker[en->s.number].eventSequence & MAX_PS_EVENTS - 1])
	{
		//did the last event contain a sound?
	case EV_GLOBAL_SOUND:
		minlen = 256;
		break;
	case EV_FIRE_WEAPON:
	case EV_ALTFIRE:
	case EV_SABER_ATTACK:
		minlen = 512;
		break;
	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:
	case EV_FOOTSTEP:
	case EV_FOOTSTEP_METAL:
	case EV_FOOTWADE:
		minlen = 256;
		break;
	case EV_JUMP:
	case EV_ROLL:
		minlen = 256;
		break;
	default:
		minlen = 999999;
		break;
	}
checkStep:
	if (bot_mind_tricked(bs->client, en->s.number))
	{
		//if mindtricked by this person, cut down on the minlen so they can't "hear" as well
		minlen /= 4;
	}

	if (endist <= minlen)
	{
		//we heard it
		return 1;
	}

	return 0;
}

//check for new events
static void update_event_tracker(void)
{
	int i = 0;

	while (i < MAX_CLIENTS)
	{
		if (gBotEventTracker[i].eventSequence != level.clients[i].ps.eventSequence)
		{
			//updated event
			gBotEventTracker[i].eventSequence = level.clients[i].ps.eventSequence;
			gBotEventTracker[i].events[0] = level.clients[i].ps.events[0];
			gBotEventTracker[i].events[1] = level.clients[i].ps.events[1];
			gBotEventTracker[i].eventTime = level.time + 0.5;
		}

		i++;
	}
}

//check if said angles are within our fov
int in_field_of_vision(vec3_t viewangles, const float fov, vec3_t angles)
{
	for (int i = 0; i < 2; i++)
	{
		const float angle = AngleMod(viewangles[i]);
		angles[i] = AngleMod(angles[i]);
		float diff = angles[i] - angle;
		if (angles[i] > angle)
		{
			if (diff > 180.0)
			{
				diff -= 360.0;
			}
		}
		else
		{
			if (diff < -180.0)
			{
				diff += 360.0;
			}
		}
		if (diff > 0)
		{
			if (diff > fov * 0.5)
			{
				return 0;
			}
		}
		else
		{
			if (diff < -fov * 0.5)
			{
				return 0;
			}
		}
	}
	return 1;
}

//We cannot hurt the ones we love. Unless of course this
//function says we can.
int pass_loved_one_check(const bot_state_t* bs, const gentity_t* ent)
{
	if (!bs->lovednum)
	{
		return 1;
	}

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		//There is no love in 1-on-1
		return 1;
	}

	int i = 0;

	if (!botstates[ent->s.number])
	{
		//not a bot
		return 1;
	}

	if (!bot_attachments.integer)
	{
		return 1;
	}

	const bot_state_t* loved = botstates[ent->s.number];

	while (i < bs->lovednum)
	{
		if (strcmp(level.clients[loved->client].pers.netname, bs->loved[i].name) == 0)
		{
			if (!is_teamplay() && bs->loved[i].level < 2)
			{
				//if FFA and level of love is not greater than 1, just don't care
				return 1;
			}
			if (is_teamplay() && !OnSameTeam(&g_entities[bs->client], &g_entities[loved->client]) && bs->loved[i].level
				< 2)
			{
				//is teamplay, but not on same team and level < 2
				return 1;
			}
			return 0;
		}

		i++;
	}

	return 1;
}

//update the currentEnemy visual data for the current enemy.
static void enemy_visual_update(bot_state_t* bs)
{
	vec3_t a;
	vec3_t enemy_origin;
	vec3_t enemy_angles;
	trace_t tr;

	if (!bs->currentEnemy)
	{
		//bad!  This should never happen
		return;
	}

	FindOrigin(bs->currentEnemy, enemy_origin);
	find_angles(bs->currentEnemy, enemy_angles);

	VectorSubtract(enemy_origin, bs->eye, a);
	const float dist = VectorLength(a);
	vectoangles(a, a);
	a[PITCH] = a[ROLL] = 0;

	trap->Trace(&tr, bs->eye, NULL, NULL, enemy_origin, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.entityNum == bs->currentEnemy->s.number && in_field_of_vision(bs->viewangles, 90, a) && !bot_mind_tricked(
		bs->client, bs->currentEnemy->s.number)
		|| bot_can_hear(bs, bs->currentEnemy, dist))
	{
		//spotted him
		bs->frame_Enemy_Len = TargetDistance(bs, bs->currentEnemy, enemy_origin);
		bs->frame_Enemy_Vis = 1;
		VectorCopy(enemy_origin, bs->lastEnemySpotted);
		VectorCopy(bs->currentEnemy->s.angles, bs->lastEnemyAngles);
		bs->enemySeenTime = level.time + 2000;
	}
	else
	{
		//can't see him
		bs->frame_Enemy_Vis = 0;
	}
}

//standard check to find a new enemy.
static int scan_for_enemies(bot_state_t* bs)
{
	float has_enemy_dist = 0;
	qboolean no_attack_non_jm = qfalse;
	float closest = 999999;
	int i = 0;
	int bestindex = -1;

	if (bs->currentEnemy)
	{
		if (!pass_standard_enemy_checks(bs, bs->currentEnemy))
		{
			//target became invalid, move to next target
			bs->currentEnemy = NULL;
		}
		else
		{
			//only switch to a new enemy if he's significantly closer
			has_enemy_dist = bs->frame_Enemy_Len;
		}
	}

	if (bs->currentEnemy && bs->currentEnemy->client &&
		bs->currentEnemy->client->ps.isJediMaster)
	{
		//The Jedi Master must die.
		enemy_visual_update(bs);

		//override the last seen locations because we can always see them
		FindOrigin(bs->currentEnemy, bs->lastEnemySpotted);
		find_angles(bs->currentEnemy, bs->lastEnemyAngles);
		return -1;
	}
	if (bs->currentTactic == BOTORDER_SEARCHANDDESTROY
		&& bs->tacticEntity)
	{
		//currently going after search and destroy target
		if (bs->tacticEntity->s.number == bs->currentEnemy->s.number)
		{
			enemy_visual_update(bs);
			return -1;
		}
	}
	else if (bs->currentTactic == BOTORDER_OBJECTIVE
		&& bs->tacticEntity)
	{
		if (bs->tacticEntity->s.number == bs->currentEnemy->s.number)
		{
			enemy_visual_update(bs);
			return -1;
		}
	}

	if (level.gametype == GT_JEDIMASTER)
	{
		if (G_ThereIsAMaster() && !bs->cur_ps.isJediMaster)
		{
			if (!g_friendlyFire.integer)
			{
				no_attack_non_jm = qtrue;
			}
			else
			{
				closest = 128; //only get mad at people if they get close enough to you to anger you, or hurt you
			}
		}
	}

	while (i < MAX_CLIENTS)
	{
		if (i != bs->client && g_entities[i].client && !OnSameTeam(&g_entities[bs->client], &g_entities[i]) &&
			pass_standard_enemy_checks(bs, &g_entities[i]) && bot_pvs_check(g_entities[i].client->ps.origin, bs->eye) &&
			pass_loved_one_check(bs, &g_entities[i]))
		{
			vec3_t a;
			VectorSubtract(g_entities[i].client->ps.origin, bs->eye, a);
			float distcheck = VectorLength(a);
			vectoangles(a, a);

			if (g_entities[i].client->ps.isJediMaster)
			{
				//make us think the Jedi Master is close so we'll attack him above all
				distcheck = 1;
			}

			if (g_entities[i].client->ps.weapon != WP_SABER && g_entities[bs->client].health < 30)
			{
				//Sniper!
				request_siege_assistance(bs, REQUEST_MEDIC);
			}
			else if (g_entities[i].client->ps.weapon == WP_DISRUPTOR &&
				g_entities[i].client->sess.sessionTeam != g_entities[bs->client].client->sess.sessionTeam)
			{
				//Sniper!
				request_siege_assistance(bs, SPOTTED_SNIPER);
			}
			else if ((g_entities[i].client->ps.weapon == WP_REPEATER ||
				g_entities[i].client->ps.weapon == WP_BOWCASTER ||
				g_entities[i].client->ps.weapon == WP_FLECHETTE ||
				g_entities[i].client->ps.weapon == WP_THERMAL) &&
				(g_entities[bs->client].client->ps.weapon == WP_REPEATER ||
					g_entities[bs->client].client->ps.weapon == WP_BOWCASTER ||
					g_entities[bs->client].client->ps.weapon == WP_FLECHETTE))
			{
				//Troops, incoming
				request_siege_assistance(bs, SPOTTED_TROOPS);
			}
			else if (g_entities[i].client->ps.weapon == WP_ROCKET_LAUNCHER ||
				g_entities[i].client->ps.weapon == WP_CONCUSSION)
			{
				//Crap, he has a bigger gun than us.
				request_siege_assistance(bs, REQUEST_ASSISTANCE);
			}
			else if (g_entities[bs->client].client->ps.weapon == WP_BLASTER ||
				g_entities[bs->client].client->ps.weapon == WP_BRYAR_PISTOL ||
				g_entities[bs->client].client->ps.weapon == WP_STUN_BATON ||
				g_entities[bs->client].client->ps.weapon == WP_DEMP2)
			{
				//Cover me, I have a shitty weapon.
				request_siege_assistance(bs, TACTICAL_COVERME);

				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					request_siege_assistance(bs, Q_irand(REPLY_YES, REPLY_COMING));
			}
			else if (g_entities[i].client->ps.weapon == WP_BLASTER ||
				g_entities[i].client->ps.weapon == WP_BRYAR_PISTOL ||
				g_entities[i].client->ps.weapon == WP_STUN_BATON ||
				g_entities[i].client->ps.weapon == WP_DEMP2)
			{
				//You have a shitty weapon. Follow me.
				request_siege_assistance(bs, TACTICAL_FOLLOW);

				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					request_siege_assistance(bs, Q_irand(REPLY_YES, REPLY_COMING));
			}
			else if (g_entities[i].client->ps.weapon == WP_DISRUPTOR)
			{
				//You have a sniper. Stay where you're at.
				request_siege_assistance(bs, TACTICAL_HOLDPOSITION);
				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					request_siege_assistance(bs, Q_irand(REPLY_YES, REPLY_COMING));
			}
			else if (level.gametype != GT_FFA && g_entities[i].client->ps.weapon == g_entities[bs->client].client->ps.
				weapon)
			{
				//We have the same weapon. Split up.
				request_siege_assistance(bs, TACTICAL_SPLIT);

				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					request_siege_assistance(bs, REPLY_YES);
			}

			if (distcheck < closest && (in_field_of_vision(bs->viewangles, 90, a) && !bot_mind_tricked(bs->client, i) ||
				bot_can_hear(bs, &g_entities[i], distcheck)) && org_visible(
					bs->eye, g_entities[i].client->ps.origin, -1))
			{
				if (bot_mind_tricked(bs->client, i))
				{
					if (distcheck < 256 || level.time - g_entities[i].client->dangerTime < 100)
					{
						if (!has_enemy_dist || distcheck < has_enemy_dist - 128)
						{
							//if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
							if (!no_attack_non_jm || g_entities[i].client->ps.isJediMaster)
							{
								closest = distcheck;
								bestindex = i;
							}
						}
					}
				}
				else
				{
					if (!has_enemy_dist || distcheck < has_enemy_dist - 128)
					{
						//if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
						if (!no_attack_non_jm || g_entities[i].client->ps.isJediMaster)
						{
							closest = distcheck;
							bestindex = i;
						}
					}
				}
			}
		}
		i++;
	}

	return bestindex;
}

//the main scan regular scan for enemies for the TAB Bot
static void advanced_scanfor_enemies(bot_state_t* bs)
{
	vec3_t a, enemy_origin;
	int close_enemy_num = -1;
	float closestdist = 99999;
	int i;
	float distcheck;
	float has_enemy_dist = 0;
	int current_enemy_num = -1;

	if (bs->currentEnemy)
	{
		if (bs->currentEnemy->health < 1
			|| bs->currentEnemy->client && bs->currentEnemy->client->sess.sessionTeam == TEAM_SPECTATOR
			|| bs->currentEnemy->s.eFlags & EF_DEAD
			|| bs->currentEnemy->client && bs->currentEnemy->client->ps.eFlags & EF_DEAD
			|| OnSameTeam(bs->currentEnemy, &g_entities[bs->cur_ps.clientNum]))
		{
			//target died or because invalid, move to next target
			bs->currentEnemy = NULL;
		}
		else
		{
			return;
		}
	}

	bs->enemyScanTime = level.time + 5000;

	if (bs->currentEnemy)
	{
		//we're already locked onto an enemy
		if (bs->currentTactic == BOTORDER_SEARCHANDDESTROY
			&& bs->tacticEntity)
		{
			//currently going after search and destroy target
			if (bs->tacticEntity->s.number == bs->currentEnemy->s.number)
			{
				enemy_visual_update(bs);
				return;
			}
		}
		//If you're locked onto an objective, don't lose it.
		else if (bs->currentTactic == BOTORDER_OBJECTIVE && bs->tacticEntity)
		{
			if (bs->tacticEntity->s.number == bs->currentEnemy->s.number)
			{
				enemy_visual_update(bs);
				return;
			}
		}
		else if (bs->frame_Enemy_Vis || bs->enemySeenTime > level.time - 2000)
		{
			return;
		}

		has_enemy_dist = bs->frame_Enemy_Len;
		current_enemy_num = bs->currentEnemy->s.number;
	}

	for (i = 0; i <= level.num_entities; i++)
	{
		if (i != bs->client
			&& (g_entities[i].client && !OnSameTeam(&g_entities[bs->client], &g_entities[i]) &&
				pass_standard_enemy_checks(bs, &g_entities[i]) && pass_loved_one_check(bs, &g_entities[i])
				|| g_entities[i].s.eType == ET_NPC && g_entities[i].s.NPC_class != CLASS_VEHICLE && g_entities[i].
				health > 0 && !(g_entities[i].s.eFlags & EF_DEAD)
				|| g_entities[i].s.weapon == WP_EMPLACED_GUN
				&& g_entities[i].teamnodmg != g_entities[bs->client].client->sess.sessionTeam
				&& g_entities[i].s.pos.trType == TR_STATIONARY))
		{
			FindOrigin(&g_entities[i], enemy_origin);

			VectorSubtract(enemy_origin, bs->eye, a);
			distcheck = VectorLength(a);
			vectoangles(a, a);
			a[PITCH] = a[ROLL] = 0;

			if (distcheck < closestdist && (in_field_of_vision(bs->viewangles, 90, a)
				&& !bot_mind_tricked(bs->client, i)
				|| bot_can_hear(bs, &g_entities[i], distcheck))
				&& org_visible(bs->eye, enemy_origin, -1))
			{
				if (!has_enemy_dist || distcheck < has_enemy_dist - 128 || current_enemy_num == i)
				{
					//if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
					closestdist = distcheck;
					close_enemy_num = i;
				}
			}
		}
	}

	if (close_enemy_num == -1)
	{
		if (bs->currentEnemy)
		{
			//no enemies in the area but we were locked on previously.  Clear frame visual data so
			//we know that we should go go try to find them again.
			bs->frame_Enemy_Vis = 0;
		}
		else
		{
			//we're all along.  No real need to update stuff in this case.
		}
	}
	else
	{
		//have a target, update their data
		vec3_t enemy_angles;
		FindOrigin(&g_entities[close_enemy_num], enemy_origin);
		find_angles(&g_entities[close_enemy_num], enemy_angles);

		bs->frame_Enemy_Len = closestdist;
		bs->frame_Enemy_Vis = 1;
		bs->currentEnemy = &g_entities[close_enemy_num];
		VectorCopy(enemy_origin, bs->lastEnemySpotted);
		VectorCopy(enemy_angles, bs->lastEnemyAngles);
		bs->lastEnemyAngles[PITCH] = bs->lastEnemyAngles[ROLL] = 0;
		bs->enemySeenTime = level.time + 2000;
	}

	while (i < MAX_CLIENTS)
	{
		if (i != bs->client && g_entities[i].client && !OnSameTeam(&g_entities[bs->client], &g_entities[i]) &&
			pass_standard_enemy_checks(bs, &g_entities[i]) && bot_pvs_check(g_entities[i].client->ps.origin, bs->eye) &&
			pass_loved_one_check(bs, &g_entities[i]))
		{
			VectorSubtract(g_entities[i].client->ps.origin, bs->eye, a);
			distcheck = VectorLength(a);
			vectoangles(a, a);

			if (g_entities[i].client->ps.isJediMaster)
			{
				//make us think the Jedi Master is close so we'll attack him above all
				distcheck = 1;
			}

			if (g_entities[i].client->ps.weapon != WP_SABER && g_entities[bs->client].health < 30)
			{
				//Sniper!
				request_siege_assistance(bs, REQUEST_MEDIC);
			}
			else if (g_entities[i].client->ps.weapon == WP_DISRUPTOR &&
				g_entities[i].client->sess.sessionTeam != g_entities[bs->client].client->sess.sessionTeam)
			{
				//Sniper!
				request_siege_assistance(bs, SPOTTED_SNIPER);
			}
			else if ((g_entities[i].client->ps.weapon == WP_REPEATER ||
				g_entities[i].client->ps.weapon == WP_BOWCASTER ||
				g_entities[i].client->ps.weapon == WP_FLECHETTE ||
				g_entities[i].client->ps.weapon == WP_THERMAL) &&
				(g_entities[bs->client].client->ps.weapon == WP_REPEATER ||
					g_entities[bs->client].client->ps.weapon == WP_BOWCASTER ||
					g_entities[bs->client].client->ps.weapon == WP_FLECHETTE))
			{
				//Troops, incoming
				request_siege_assistance(bs, SPOTTED_TROOPS);
			}
			else if (g_entities[i].client->ps.weapon == WP_ROCKET_LAUNCHER ||
				g_entities[i].client->ps.weapon == WP_CONCUSSION)
			{
				//Crap, he has a bigger gun than us.
				request_siege_assistance(bs, REQUEST_ASSISTANCE);
			}
			else if (g_entities[bs->client].client->ps.weapon == WP_BLASTER ||
				g_entities[bs->client].client->ps.weapon == WP_BRYAR_PISTOL ||
				g_entities[bs->client].client->ps.weapon == WP_STUN_BATON ||
				g_entities[bs->client].client->ps.weapon == WP_DEMP2)
			{
				//Cover me, I have a shitty weapon.
				request_siege_assistance(bs, TACTICAL_COVERME);

				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					request_siege_assistance(bs, Q_irand(REPLY_YES, REPLY_COMING));
			}
			else if (g_entities[i].client->ps.weapon == WP_BLASTER ||
				g_entities[i].client->ps.weapon == WP_BRYAR_PISTOL ||
				g_entities[i].client->ps.weapon == WP_STUN_BATON ||
				g_entities[i].client->ps.weapon == WP_DEMP2)
			{
				//You have a shitty weapon. Follow me.
				request_siege_assistance(bs, TACTICAL_FOLLOW);

				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					request_siege_assistance(bs, Q_irand(REPLY_YES, REPLY_COMING));
			}
			else if (g_entities[i].client->ps.weapon == WP_DISRUPTOR)
			{
				//You have a sniper. Stay where you're at.
				request_siege_assistance(bs, TACTICAL_HOLDPOSITION);
				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					request_siege_assistance(bs, Q_irand(REPLY_YES, REPLY_COMING));
			}
			else if (level.gametype != GT_FFA && g_entities[i].client->ps.weapon == g_entities[bs->client].client->ps.
				weapon)
			{
				//We have the same weapon. Split up.
				request_siege_assistance(bs, TACTICAL_SPLIT);

				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					request_siege_assistance(bs, REPLY_YES);
			}

			if (distcheck < closestdist && (in_field_of_vision(bs->viewangles, 90, a) && !bot_mind_tricked(
				bs->client, i)
				|| bot_can_hear(bs, &g_entities[i], distcheck)) && org_visible(
					bs->eye, g_entities[i].client->ps.origin, -1))
			{
				if (bot_mind_tricked(bs->client, i))
				{
					if (distcheck < 256 || level.time - g_entities[i].client->dangerTime < 100)
					{
						if (!has_enemy_dist || distcheck < has_enemy_dist - 128)
						{
							//if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
							if (g_entities[i].client->ps.isJediMaster)
							{
								closestdist = distcheck;
							}
						}
					}
				}
				else
				{
					if (!has_enemy_dist || distcheck < has_enemy_dist - 128)
					{
						//if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
						if (g_entities[i].client->ps.isJediMaster)
						{
							closestdist = distcheck;
						}
					}
				}
			}
		}
		i++;
	}
}

static int waiting_for_now(bot_state_t* bs, vec3_t goalpos)
{
	vec3_t xybot;
	vec3_t xywp;
	vec3_t a;

	if (!bs->wpCurrent)
	{
		return 0;
	}

	if ((int)goalpos[0] != (int)bs->wpCurrent->origin[0] ||
		(int)goalpos[1] != (int)bs->wpCurrent->origin[1] ||
		(int)goalpos[2] != (int)bs->wpCurrent->origin[2])
	{
		return 0;
	}

	VectorCopy(bs->origin, xybot);
	VectorCopy(bs->wpCurrent->origin, xywp);

	xybot[2] = 0;
	xywp[2] = 0;

	VectorSubtract(xybot, xywp, a);

	if (VectorLength(a) < 16 && bs->frame_Waypoint_Len > 100)
	{
		if (check_for_func(bs->origin, bs->client))
		{
			return 1; //we're probably standing on an elevator and riding up/down. Or at least we hope so.
		}
	}
	else if (VectorLength(a) < 64 && bs->frame_Waypoint_Len > 64 &&
		check_for_func(bs->origin, bs->client))
	{
		bs->noUseTime = level.time + 2000;
	}

	return 0;
}

//get an ideal distance for us to be at in relation to our opponent
//based on our weapon.
int bot_get_weapon_range(const bot_state_t* bs)
{
	switch (bs->cur_ps.weapon)
	{
	case WP_STUN_BATON:
	case WP_MELEE:
		return BWEAPONRANGE_MELEE;
	case WP_SABER:
		return BWEAPONRANGE_SABER;
	case WP_BRYAR_PISTOL:
	case WP_BLASTER:
	case WP_DISRUPTOR:
	case WP_REPEATER:
		return BWEAPONRANGE_MID;
	case WP_BOWCASTER:
	case WP_DEMP2:
	case WP_FLECHETTE:
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
	case WP_TRIP_MINE:
	case WP_DET_PACK:
		return BWEAPONRANGE_LONG;
	default:
		return BWEAPONRANGE_MID;
	}
}

//see if we want to run away from the opponent for whatever reason
int bot_is_a_chicken_wuss(bot_state_t* bs)
{
	if (gLevelFlags & LEVELFLAG_IMUSTNTRUNAWAY)
	{
		//The level says we mustn't run away!
		return 0;
	}

	if (level.gametype == GT_SINGLE_PLAYER)
	{
		//"missions" (not really)
		return 0;
	}

	if (level.gametype == GT_JEDIMASTER && !bs->cur_ps.isJediMaster)
	{
		//Then you may know no fear.
		//Well, unless he's strong.
		if (bs->currentEnemy && bs->currentEnemy->client &&
			bs->currentEnemy->client->ps.isJediMaster &&
			bs->currentEnemy->health > 40 &&
			bs->cur_ps.weapon < WP_ROCKET_LAUNCHER)
		{
			//explosive weapons are most effective against the Jedi Master
			goto jmPass;
		}
		return 0;
	}

	if (level.gametype == GT_CTF && bs->currentEnemy && bs->currentEnemy->client)
	{
		if (bs->currentEnemy->client->ps.powerups[PW_REDFLAG] ||
			bs->currentEnemy->client->ps.powerups[PW_BLUEFLAG])
		{
			//don't be afraid of flag carriers, they must die!
			return 0;
		}
	}

jmPass:
	if (bs->chickenWussCalculationTime > level.time)
	{
		return 2; //don't want to keep going between two points...
	}

	if (bs->cur_ps.fd.forcePowersActive & 1 << FP_RAGE)
	{
		//don't run while raging
		return 0;
	}

	if (level.gametype == GT_JEDIMASTER && !bs->cur_ps.isJediMaster)
	{
		//be frightened of the jedi master? I guess in this case.
		return 1;
	}

	bs->chickenWussCalculationTime = level.time + MAX_CHICKENWUSS_TIME;

	if (g_entities[bs->client].health < BOT_RUN_HEALTH)
	{
		//we're low on health, let's get away
		return 1;
	}

	const int b_w_range = bot_get_weapon_range(bs);

	if (b_w_range == BWEAPONRANGE_MELEE || b_w_range == BWEAPONRANGE_SABER)
	{
		if (b_w_range != BWEAPONRANGE_SABER || !bs->saberSpecialist)
		{
			//run away if we're using melee, or if we're using a saber and not a "saber specialist"
			return 1;
		}
	}

	if (bs->cur_ps.weapon == WP_BRYAR_PISTOL)
	{
		//the bryar is a weak weapon, so just try to find a new one if it's what you're having to use
		return 1;
	}

	if (bs->currentEnemy && bs->currentEnemy->client &&
		bs->currentEnemy->client->ps.weapon == WP_SABER &&
		bs->frame_Enemy_Len < 512 && bs->cur_ps.weapon != WP_SABER)
	{
		//if close to an enemy with a saber and not using a saber, then try to back off
		return 1;
	}

	if (level.time - bs->cur_ps.electrifyTime < 16000)
	{
		//lightning is dangerous.
		return 1;
	}

	//didn't run, reset the timer
	bs->chickenWussCalculationTime = 0;

	return 0;
}

//look for "bad things". bad things include detpacks, thermal detonators,
//and other dangerous explodey items.
static gentity_t* get_nearest_bad_thing(bot_state_t* bs)
{
	int i = 0;
	float glen;
	int bestindex = 0;
	float bestdist = 800; //if not within a radius of 800, it's no threat anyway
	int foundindex = 0;
	float factor;
	trace_t tr;

	while (i < level.num_entities)
	{
		vec3_t hold;
		const gentity_t* ent = &g_entities[i];

		if (ent &&
			!ent->client &&
			ent->inuse &&
			ent->damage &&
			/*(ent->s.weapon == WP_THERMAL || ent->s.weapon == WP_FLECHETTE)*/
			ent->s.weapon &&
			ent->splashDamage ||
			ent &&
			ent->genericValue5 == 1000 &&
			ent->inuse &&
			ent->health > 0 &&
			ent->genericValue3 != bs->client &&
			g_entities[ent->genericValue3].client && !OnSameTeam(&g_entities[bs->client],
				&g_entities[ent->genericValue3]))
		{
			//try to escape from anything with a non-0 s.weapon and non-0 damage. This hopefully only means dangerous projectiles.
			//Or a sentry gun if bolt_Head == 1000. This is a terrible hack, yes.
			VectorSubtract(bs->origin, ent->r.currentOrigin, hold);
			glen = VectorLength(hold);

			if (ent->s.weapon != WP_THERMAL && ent->s.weapon != WP_FLECHETTE &&
				ent->s.weapon != WP_DET_PACK && ent->s.weapon != WP_TRIP_MINE)
			{
				factor = 0.5;

				if (ent->s.weapon && glen <= 256 && bs->settings.skill > 2)
				{
					//it's a projectile so push it away
					bs->doForcePush = level.time + 700;
					//trap->Print("PUSH PROJECTILE\n");
				}
			}
			else
			{
				factor = 1;
			}

			if (ent->s.weapon == WP_ROCKET_LAUNCHER &&
				(ent->r.ownerNum == bs->client ||
					ent->r.ownerNum > 0 && ent->r.ownerNum < MAX_CLIENTS &&
					g_entities[ent->r.ownerNum].client && OnSameTeam(
						&g_entities[bs->client], &g_entities[ent->r.ownerNum])))
			{
				//don't be afraid of your own rockets or your teammates' rockets
				factor = 0;
			}

			if (ent->s.weapon == WP_DET_PACK &&
				(ent->r.ownerNum == bs->client ||
					ent->r.ownerNum > 0 && ent->r.ownerNum < MAX_CLIENTS &&
					g_entities[ent->r.ownerNum].client && OnSameTeam(
						&g_entities[bs->client], &g_entities[ent->r.ownerNum])))
			{
				//don't be afraid of your own detpacks or your teammates' detpacks
				factor = 0;
			}

			if (ent->s.weapon == WP_TRIP_MINE &&
				(ent->r.ownerNum == bs->client ||
					ent->r.ownerNum > 0 && ent->r.ownerNum < MAX_CLIENTS &&
					g_entities[ent->r.ownerNum].client && OnSameTeam(
						&g_entities[bs->client], &g_entities[ent->r.ownerNum])))
			{
				//don't be afraid of your own trip mines or your teammates' trip mines
				factor = 0;
			}

			if (ent->s.weapon == WP_THERMAL &&
				(ent->r.ownerNum == bs->client ||
					ent->r.ownerNum > 0 && ent->r.ownerNum < MAX_CLIENTS &&
					g_entities[ent->r.ownerNum].client && OnSameTeam(
						&g_entities[bs->client], &g_entities[ent->r.ownerNum])))
			{
				//don't be afraid of your own thermals or your teammates' thermals
				factor = 0;
			}

			if (glen < bestdist * factor && bot_pvs_check(bs->origin, ent->s.pos.trBase))
			{
				trap->Trace(&tr, bs->origin, NULL, NULL, ent->s.pos.trBase, bs->client, MASK_SOLID, qfalse, 0, 0);

				if (tr.fraction == 1 || tr.entityNum == ent->s.number)
				{
					bestindex = i;
					bestdist = glen;
					foundindex = 1;
				}
			}
		}

		if (ent && !ent->client && ent->inuse && ent->damage && ent->s.weapon && ent->r.ownerNum < MAX_CLIENTS && ent->r
			.ownerNum >= 0)
		{
			//if we're in danger of a projectile belonging to someone and don't have an enemy, set the enemy to them
			gentity_t* proj_owner = &g_entities[ent->r.ownerNum];

			if (proj_owner && proj_owner->inuse && proj_owner->client)
			{
				if (!bs->currentEnemy)
				{
					if (pass_standard_enemy_checks(bs, proj_owner))
					{
						if (pass_loved_one_check(bs, proj_owner))
						{
							VectorSubtract(bs->origin, ent->r.currentOrigin, hold);
							glen = VectorLength(hold);

							if (glen < 512)
							{
								bs->currentEnemy = proj_owner;
								bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
							}
						}
					}
				}
			}
		}

		i++;
	}

	if (foundindex)
	{
		bs->dontGoBack = level.time + 1500;
		return &g_entities[bestindex];
	}
	return NULL;
}

//Keep our CTF priorities on defending our team's flag
static int bot_defend_flag(bot_state_t* bs)
{
	wpobject_t* flag_point;
	vec3_t a;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		flag_point = flagRed;
	}
	else if (level.clients[bs->client].sess.sessionTeam == TEAM_BLUE)
	{
		flag_point = flagBlue;
	}
	else
	{
		return 0;
	}

	if (!flag_point)
	{
		return 0;
	}

	VectorSubtract(bs->origin, flag_point->origin, a);

	if (VectorLength(a) > BASE_GUARD_DISTANCE)
	{
		bs->wpDestination = flag_point;
	}

	return 1;
}

//Keep our CTF priorities on getting the other team's flag
static int bot_get_enemy_flag(bot_state_t* bs)
{
	wpobject_t* flagPoint;
	vec3_t a;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		flagPoint = flagBlue;
	}
	else if (level.clients[bs->client].sess.sessionTeam == TEAM_BLUE)
	{
		flagPoint = flagRed;
	}
	else
	{
		return 0;
	}

	if (!flagPoint)
	{
		return 0;
	}

	VectorSubtract(bs->origin, flagPoint->origin, a);

	if (VectorLength(a) > BASE_GETENEMYFLAG_DISTANCE)
	{
		bs->wpDestination = flagPoint;
	}

	return 1;
}

//Our team's flag is gone, so try to get it back
static int bot_get_flag_back(bot_state_t* bs)
{
	int i = 0;
	int my_flag;
	int found_carrier = 0;
	const gentity_t* ent = NULL;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		my_flag = PW_REDFLAG;
	}
	else
	{
		my_flag = PW_BLUEFLAG;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && ent->client->ps.powerups[my_flag] && !OnSameTeam(&g_entities[bs->client], ent))
		{
			found_carrier = 1;
			break;
		}

		i++;
	}

	if (!found_carrier)
	{
		return 0;
	}

	if (!ent)
	{
		return 0;
	}

	if (bs->wpDestSwitchTime < level.time)
	{
		vec3_t usethisvec;
		if (ent->client)
		{
			VectorCopy(ent->client->ps.origin, usethisvec);
		}
		else
		{
			VectorCopy(ent->s.origin, usethisvec);
		}

		const int temp_int = get_nearest_visible_wp(usethisvec, 0);

		if (temp_int != -1 && total_trail_distance(bs->wpCurrent->index, temp_int) != -1)
		{
			bs->wpDestination = gWPArray[temp_int];
			bs->wpDestSwitchTime = level.time + Q_irand(1000, 5000);
		}
	}

	return 1;
}

static void determine_ctf_goal(bot_state_t* bs)
{
	int num_offence = 0; //number of bots on offence
	int num_defense = 0; //number of bots on defence

	//clear out the current tactic
	bs->currentTactic = BOTORDER_OBJECTIVE;
	bs->tacticEntity = NULL;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		const bot_state_t* tempbot = botstates[i];

		if (!tempbot || !tempbot->inuse || tempbot->client == bs->client)
		{
			//this bot isn't in use or this is the current bot
			continue;
		}

		if (g_entities[tempbot->client].client->sess.sessionTeam
			!= g_entities[bs->client].client->sess.sessionTeam)
		{
			continue;
		}

		if (tempbot->currentTactic == BOTORDER_OBJECTIVE)
		{
			//this bot is going for/defending the flag
			if (tempbot->objectiveType == OT_CAPTURE)
			{
				num_offence++;
			}
			else
			{
				//it's on defense
				num_defense++;
			}
		}
	}

	if (num_defense < num_offence)
	{
		//we have less defenders than attackers.  Go on the defense.
		bs->objectiveType = OT_DEFENDCAPTURE;
	}
	else
	{
		//go on the attack
		bs->objectiveType = OT_CAPTURE;
	}
}

//Someone else on our team has the enemy flag, so try to get
//to their assistance
static int bot_guard_flag_carrier(bot_state_t* bs)
{
	int i = 0;
	int enemy_flag;
	int found_carrier = 0;
	const gentity_t* ent = NULL;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		enemy_flag = PW_BLUEFLAG;
	}
	else
	{
		enemy_flag = PW_REDFLAG;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && ent->client->ps.powerups[enemy_flag] && OnSameTeam(&g_entities[bs->client], ent))
		{
			found_carrier = 1;
			break;
		}

		i++;
	}

	if (!found_carrier)
	{
		return 0;
	}

	if (!ent)
	{
		return 0;
	}

	if (bs->wpDestSwitchTime < level.time)
	{
		vec3_t usethisvec;
		if (ent->client)
		{
			VectorCopy(ent->client->ps.origin, usethisvec);
		}
		else
		{
			VectorCopy(ent->s.origin, usethisvec);
		}

		const int temp_int = get_nearest_visible_wp(usethisvec, 0);

		if (temp_int != -1 && total_trail_distance(bs->wpCurrent->index, temp_int) != -1)
		{
			bs->wpDestination = gWPArray[temp_int];
			bs->wpDestSwitchTime = level.time + Q_irand(1000, 5000);
		}
	}

	return 1;
}

//We have the flag, let's get it home.
static int bot_get_flag_home(bot_state_t* bs)
{
	wpobject_t* flag_point;
	vec3_t a;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		flag_point = flagRed;
	}
	else if (level.clients[bs->client].sess.sessionTeam == TEAM_BLUE)
	{
		flag_point = flagBlue;
	}
	else
	{
		return 0;
	}

	if (!flag_point)
	{
		return 0;
	}

	VectorSubtract(bs->origin, flag_point->origin, a);

	if (VectorLength(a) > BASE_FLAGWAIT_DISTANCE)
	{
		bs->wpDestination = flag_point;
	}

	return 1;
}

static void get_new_flag_point(const wpobject_t* wp, const gentity_t* flag_ent, const int team)
{
	//get the nearest possible waypoint to the flag since it's not in its original position
	int i = 0;
	vec3_t a, mins, maxs;
	int bestindex = 0;
	int foundindex = 0;
	trace_t tr;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -5;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 5;

	VectorSubtract(wp->origin, flag_ent->s.pos.trBase, a);

	float bestdist = VectorLength(a);

	if (bestdist <= WP_KEEP_FLAG_DIST)
	{
		trap->Trace(&tr, wp->origin, mins, maxs, flag_ent->s.pos.trBase, flag_ent->s.number, MASK_SOLID, qfalse, 0, 0);

		if (tr.fraction == 1)
		{
			//this point is good
			return;
		}
	}

	while (i < gWPNum)
	{
		VectorSubtract(gWPArray[i]->origin, flag_ent->s.pos.trBase, a);
		const float testdist = VectorLength(a);

		if (testdist < bestdist)
		{
			trap->Trace(&tr, gWPArray[i]->origin, mins, maxs, flag_ent->s.pos.trBase, flag_ent->s.number, MASK_SOLID,
				qfalse, 0, 0);

			if (tr.fraction == 1)
			{
				foundindex = 1;
				bestindex = i;
				bestdist = testdist;
			}
		}

		i++;
	}

	if (foundindex)
	{
		if (team == TEAM_RED)
		{
			flagRed = gWPArray[bestindex];
		}
		else
		{
			flagBlue = gWPArray[bestindex];
		}
	}
}

//See if our CTF state should take priority in our nav routines
static int ctf_takes_priority(bot_state_t* bs)
{
	gentity_t* ent;
	int enemy_flag;
	int my_flag;
	int enemy_has_our_flag = 0;
	//int weHaveEnemyFlag = 0;
	int num_on_my_team = 0;
	int num_on_enemy_team = 0;
	int num_attackers = 0;
	int num_defenders = 0;
	int i;
	int dosw = 0;
	wpobject_t* dest_sw = NULL;
#ifdef BOT_CTF_DEBUG
	vec3_t t;

	trap->Print("CTFSTATE: %s\n", ctfStateNames[bs->ctfState]);
#endif

	if (level.gametype != GT_CTF && level.gametype != GT_CTY)
	{
		return 0;
	}

	if (bs->cur_ps.weapon == WP_BRYAR_PISTOL &&
		level.time - bs->lastDeadTime < BOT_MAX_WEAPON_GATHER_TIME)
	{
		//get the nearest weapon laying around base before heading off for battle
		const int idle_wp = get_best_idle_goal(bs);

		if (idle_wp != -1 && gWPArray[idle_wp] && gWPArray[idle_wp]->inuse)
		{
			if (bs->wpDestSwitchTime < level.time)
			{
				bs->wpDestination = gWPArray[idle_wp];
			}
			return 1;
		}
	}
	else if (bs->cur_ps.weapon == WP_BRYAR_PISTOL &&
		level.time - bs->lastDeadTime < BOT_MAX_WEAPON_CHASE_CTF &&
		bs->wpDestination && bs->wpDestination->weight)
	{
		dest_sw = bs->wpDestination;
		dosw = 1;
	}

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		my_flag = PW_REDFLAG;
	}
	else
	{
		my_flag = PW_BLUEFLAG;
	}

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		enemy_flag = PW_BLUEFLAG;
	}
	else
	{
		enemy_flag = PW_REDFLAG;
	}

	if (!flagRed || !flagBlue ||
		!flagRed->inuse || !flagBlue->inuse ||
		!eFlagRed || !eFlagBlue)
	{
		return 0;
	}

#ifdef BOT_CTF_DEBUG
	VectorCopy(flagRed->origin, t);
	t[2] += 128;
	G_TestLine(flagRed->origin, t, 0x0000ff, 500);

	VectorCopy(flagBlue->origin, t);
	t[2] += 128;
	G_TestLine(flagBlue->origin, t, 0x0000ff, 500);
#endif

	if (droppedRedFlag && droppedRedFlag->flags & FL_DROPPED_ITEM)
	{
		get_new_flag_point(flagRed, droppedRedFlag, TEAM_RED);
	}
	else
	{
		flagRed = oFlagRed;
	}

	if (droppedBlueFlag && droppedBlueFlag->flags & FL_DROPPED_ITEM)
	{
		get_new_flag_point(flagBlue, droppedBlueFlag, TEAM_BLUE);
	}
	else
	{
		flagBlue = oFlagBlue;
	}

	if (!bs->ctfState)
	{
		return 0;
	}

	i = 0;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client)
		{
			/*if (ent->client->ps.powerups[enemyFlag] && OnSameTeam(&g_entities[bs->client], ent))
			{
				weHaveEnemyFlag = 1;
			}
			else */
			if (ent->client->ps.powerups[my_flag] && !OnSameTeam(&g_entities[bs->client], ent))
			{
				enemy_has_our_flag = 1;
			}

			if (OnSameTeam(&g_entities[bs->client], ent))
			{
				num_on_my_team++;
			}
			else
			{
				num_on_enemy_team++;
			}

			if (botstates[ent->s.number])
			{
				if (botstates[ent->s.number]->ctfState == CTFSTATE_ATTACKER ||
					botstates[ent->s.number]->ctfState == CTFSTATE_RETRIEVAL)
				{
					num_attackers++;
				}
				else
				{
					num_defenders++;
				}
			}
			else
			{
				//assume real players to be attackers in our logic
				num_attackers++;
			}
		}
		i++;
	}

	if (bs->cur_ps.powerups[enemy_flag])
	{
		if ((num_on_my_team < 2 || !num_attackers) && enemy_has_our_flag)
		{
			bs->ctfState = CTFSTATE_RETRIEVAL;
		}
		else
		{
			bs->ctfState = CTFSTATE_GETFLAGHOME;
		}
	}
	else if (bs->ctfState == CTFSTATE_GETFLAGHOME)
	{
		bs->ctfState = 0;
	}

	if (bs->state_Forced)
	{
		bs->ctfState = bs->state_Forced;
	}

	if (bs->ctfState == CTFSTATE_DEFENDER)
	{
		if (bot_defend_flag(bs))
		{
			goto success;
		}
	}

	if (bs->ctfState == CTFSTATE_ATTACKER)
	{
		if (bot_get_enemy_flag(bs))
		{
			goto success;
		}
	}

	if (bs->ctfState == CTFSTATE_RETRIEVAL)
	{
		if (bot_get_flag_back(bs))
		{
			goto success;
		}
		//can't find anyone on another team being a carrier, so ignore this priority
		bs->ctfState = 0;
	}

	if (bs->ctfState == CTFSTATE_GUARDCARRIER)
	{
		if (bot_guard_flag_carrier(bs))
		{
			goto success;
		}
		//can't find anyone on our team being a carrier, so ignore this priority
		bs->ctfState = 0;
	}

	if (bs->ctfState == CTFSTATE_GETFLAGHOME)
	{
		if (bot_get_flag_home(bs))
		{
			goto success;
		}
	}

	return 0;

success:
	if (dosw)
	{
		//allow ctf code to run, but if after a particular item then keep going after it
		bs->wpDestination = dest_sw;
	}

	return 1;
}

static int entity_visible_box(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, const int ignore, const int ignore2)
{
	trace_t tr;

	trap->Trace(&tr, org1, mins, maxs, org2, ignore, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
	{
		return 1;
	}
	if (tr.entityNum != ENTITYNUM_NONE && tr.entityNum == ignore2)
	{
		return 1;
	}

	return 0;
}

//Get the closest objective for siege and go after it
static int siege_target_closest_objective(bot_state_t* bs, const int flag)
{
	int i = 0;
	int bestindex = -1;
	float testdistance;
	float bestdistance = 999999999.9f;
	vec3_t a, dif;
	vec3_t mins, maxs;
	gentity_t* goalent = &g_entities[bs->wpDestination->associated_entity];

	mins[0] = -1;
	mins[1] = -1;
	mins[2] = -1;

	maxs[0] = 1;
	maxs[1] = 1;
	maxs[2] = 1;

	if (bs->wpDestination && bs->wpDestination->flags & flag && bs->wpDestination->associated_entity != ENTITYNUM_NONE
		&&
		&g_entities[bs->wpDestination->associated_entity] && g_entities[bs->wpDestination->associated_entity].use)
	{
		goto hasPoint;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->flags & flag && gWPArray[i]->associated_entity !=
			ENTITYNUM_NONE &&
			&g_entities[gWPArray[i]->associated_entity] && g_entities[gWPArray[i]->associated_entity].use)
		{
			VectorSubtract(gWPArray[i]->origin, bs->origin, a);
			testdistance = VectorLength(a);

			if (testdistance < bestdistance)
			{
				bestdistance = testdistance;
				bestindex = i;
			}
		}

		i++;
	}

	if (bestindex != -1)
	{
		bs->wpDestination = gWPArray[bestindex];
	}
	else
	{
		return 0;
	}
hasPoint:

	if (!goalent)
	{
		return 0;
	}

	VectorSubtract(bs->origin, bs->wpDestination->origin, a);

	testdistance = VectorLength(a);

	dif[0] = (goalent->r.absmax[0] + goalent->r.absmin[0]) / 2;
	dif[1] = (goalent->r.absmax[1] + goalent->r.absmin[1]) / 2;
	dif[2] = (goalent->r.absmax[2] + goalent->r.absmin[2]) / 2;
	//brush models can have tricky origins, so this is our hacky method of getting the center point

	if (goalent->takedamage && testdistance < BOT_MIN_SIEGE_GOAL_SHOOT &&
		entity_visible_box(bs->origin, mins, maxs, dif, bs->client, goalent->s.number))
	{
		bs->shootGoal = goalent;
		bs->touchGoal = NULL;
	}
	else if (goalent->use && testdistance < BOT_MIN_SIEGE_GOAL_TRAVEL)
	{
		bs->shootGoal = NULL;
		bs->touchGoal = goalent;
	}
	else
	{
		//don't know how to handle this goal object!
		bs->shootGoal = NULL;
		bs->touchGoal = NULL;
	}

	if (bot_get_weapon_range(bs) == BWEAPONRANGE_MELEE ||
		bot_get_weapon_range(bs) == BWEAPONRANGE_SABER)
	{
		bs->shootGoal = NULL; //too risky
	}

	if (bs->touchGoal)
	{
		//trap->Print("Please, master, let me touch it!\n");
		VectorCopy(dif, bs->goalPosition);
	}

	return 1;
}

void siege_defend_from_attackers(bot_state_t* bs)
{
	//this may be a little cheap, but the best way to find our defending point is probably
	int i = 0;
	int bestindex = -1;
	float bestdist = 999999;

	while (i < MAX_CLIENTS)
	{
		const gentity_t* ent = &g_entities[i];

		if (ent && ent->client && ent->client->sess.sessionTeam != g_entities[bs->client].client->sess.sessionTeam &&
			ent->health > 0 && ent->client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			vec3_t a;
			VectorSubtract(ent->client->ps.origin, bs->origin, a);

			const float testdist = VectorLength(a);

			if (testdist < bestdist)
			{
				bestindex = i;
				bestdist = testdist;
			}
		}

		i++;
	}

	if (bestindex == -1)
	{
		return;
	}

	const int wp_close = get_nearest_visible_wp(g_entities[bestindex].client->ps.origin, -1);

	if (wp_close != -1 && gWPArray[wp_close] && gWPArray[wp_close]->inuse)
	{
		bs->wpDestination = gWPArray[wp_close];
		bs->destinationGrabTime = level.time + 10000;
	}
}

//how many defenders on our team?
static int siege_count_defenders(const bot_state_t* bs)
{
	int i = 0;
	int num = 0;

	while (i < MAX_CLIENTS)
	{
		const gentity_t* ent = &g_entities[i];
		const bot_state_t* bot = botstates[i];

		if (ent && ent->client && bot)
		{
			if (bot->siegeState == SIEGESTATE_DEFENDER &&
				ent->client->sess.sessionTeam == g_entities[bs->client].client->sess.sessionTeam)
			{
				num++;
			}
		}

		i++;
	}

	return num;
}

//how many other players on our team?
static int siege_count_teammates(const bot_state_t* bs)
{
	int i = 0;
	int num = 0;

	while (i < MAX_CLIENTS)
	{
		const gentity_t* ent = &g_entities[i];

		if (ent && ent->client)
		{
			if (ent->client->sess.sessionTeam == g_entities[bs->client].client->sess.sessionTeam)
			{
				num++;
			}
		}

		i++;
	}

	return num;
}

//see if siege objective completion should take priority in our
//nav routines.
static int siege_takes_priority(bot_state_t* bs)
{
	int attacker;
	int flag_for_attackable_objective;
	wpobject_t* dest_sw = NULL;
	int dosw = 0;
	vec3_t dif;
	trace_t tr;

	if (level.gametype != GT_SIEGE)
	{
		return 0;
	}

	const gclient_t* bcl = g_entities[bs->client].client;

	if (!bcl)
	{
		return 0;
	}

	if (bs->cur_ps.weapon == WP_BRYAR_PISTOL &&
		level.time - bs->lastDeadTime < BOT_MAX_WEAPON_GATHER_TIME)
	{
		//get the nearest weapon laying around base before heading off for battle
		const int idle_wp = get_best_idle_goal(bs);

		if (idle_wp != -1 && gWPArray[idle_wp] && gWPArray[idle_wp]->inuse)
		{
			if (bs->wpDestSwitchTime < level.time)
			{
				bs->wpDestination = gWPArray[idle_wp];
			}
			return 1;
		}
	}
	else if (bs->cur_ps.weapon == WP_BRYAR_PISTOL &&
		level.time - bs->lastDeadTime < BOT_MAX_WEAPON_CHASE_TIME &&
		bs->wpDestination && bs->wpDestination->weight)
	{
		dest_sw = bs->wpDestination;
		dosw = 1;
	}

	if (bcl->sess.sessionTeam == SIEGETEAM_TEAM1)
	{
		attacker = imperial_attackers;
		flag_for_attackable_objective = WPFLAG_SIEGE_IMPERIALOBJ;
	}
	else
	{
		attacker = rebel_attackers;
		flag_for_attackable_objective = WPFLAG_SIEGE_REBELOBJ;
	}

	if (attacker)
	{
		bs->siegeState = SIEGESTATE_ATTACKER;
	}
	else
	{
		bs->siegeState = SIEGESTATE_DEFENDER;
		const int defenders = siege_count_defenders(bs);
		const int teammates = siege_count_teammates(bs);

		if (defenders > teammates / 3 && teammates > 1)
		{
			//devote around 1/4 of our team to completing our own side goals even if we're a defender.
			//If we have no side goals we will realize that later on and join the defenders
			bs->siegeState = SIEGESTATE_ATTACKER;
		}
	}

	if (bs->state_Forced)
	{
		bs->siegeState = bs->state_Forced;
	}

	if (bs->siegeState == SIEGESTATE_ATTACKER)
	{
		if (!siege_target_closest_objective(bs, flag_for_attackable_objective))
		{
			//looks like we have no goals other than to keep the other team from completing objectives
			siege_defend_from_attackers(bs);
			if (bs->shootGoal)
			{
				dif[0] = (bs->shootGoal->r.absmax[0] + bs->shootGoal->r.absmin[0]) / 2;
				dif[1] = (bs->shootGoal->r.absmax[1] + bs->shootGoal->r.absmin[1]) / 2;
				dif[2] = (bs->shootGoal->r.absmax[2] + bs->shootGoal->r.absmin[2]) / 2;

				if (!bot_pvs_check(bs->origin, dif))
				{
					bs->shootGoal = NULL;
				}
				else
				{
					trap->Trace(&tr, bs->origin, NULL, NULL, dif, bs->client, MASK_SOLID, qfalse, 0, 0);

					if (tr.fraction != 1 && tr.entityNum != bs->shootGoal->s.number)
					{
						bs->shootGoal = NULL;
					}
				}
			}
		}
	}
	else if (bs->siegeState == SIEGESTATE_DEFENDER)
	{
		siege_defend_from_attackers(bs);
		if (bs->shootGoal)
		{
			dif[0] = (bs->shootGoal->r.absmax[0] + bs->shootGoal->r.absmin[0]) / 2;
			dif[1] = (bs->shootGoal->r.absmax[1] + bs->shootGoal->r.absmin[1]) / 2;
			dif[2] = (bs->shootGoal->r.absmax[2] + bs->shootGoal->r.absmin[2]) / 2;

			if (!bot_pvs_check(bs->origin, dif))
			{
				bs->shootGoal = NULL;
			}
			else
			{
				trap->Trace(&tr, bs->origin, NULL, NULL, dif, bs->client, MASK_SOLID, qfalse, 0, 0);

				if (tr.fraction != 1 && tr.entityNum != bs->shootGoal->s.number)
				{
					bs->shootGoal = NULL;
				}
			}
		}
	}
	else
	{
		//get busy!
		siege_target_closest_objective(bs, flag_for_attackable_objective);
		if (bs->shootGoal)
		{
			dif[0] = (bs->shootGoal->r.absmax[0] + bs->shootGoal->r.absmin[0]) / 2;
			dif[1] = (bs->shootGoal->r.absmax[1] + bs->shootGoal->r.absmin[1]) / 2;
			dif[2] = (bs->shootGoal->r.absmax[2] + bs->shootGoal->r.absmin[2]) / 2;

			if (!bot_pvs_check(bs->origin, dif))
			{
				bs->shootGoal = NULL;
			}
			else
			{
				trap->Trace(&tr, bs->origin, NULL, NULL, dif, bs->client, MASK_SOLID, qfalse, 0, 0);

				if (tr.fraction != 1 && tr.entityNum != bs->shootGoal->s.number)
				{
					bs->shootGoal = NULL;
				}
			}
		}
	}

	if (dosw)
	{
		//allow siege objective code to run, but if after a particular item then keep going after it
		bs->wpDestination = dest_sw;
	}

	return 1;
}

//see if jedi master priorities should take priority in our nav
//routines.
static int jm_takes_priority(bot_state_t* bs)
{
	int i = 0;
	gentity_t* the_important_entity;

	if (level.gametype != GT_JEDIMASTER)
	{
		return 0;
	}

	if (bs->cur_ps.isJediMaster)
	{
		return 0;
	}

	//jmState becomes the index for the one who carries the saber. If jmState is -1 then the saber is currently
	//without an owner
	bs->jmState = -1;

	while (i < MAX_CLIENTS)
	{
		if (g_entities[i].client && g_entities[i].inuse &&
			g_entities[i].client->ps.isJediMaster)
		{
			bs->jmState = i;
			break;
		}

		i++;
	}

	if (bs->jmState != -1)
	{
		the_important_entity = &g_entities[bs->jmState];
	}
	else
	{
		the_important_entity = gJMSaberEnt;
	}

	if (the_important_entity && the_important_entity->inuse && bs->destinationGrabTime < level.time)
	{
		int wp_close;
		if (the_important_entity->client)
		{
			wp_close = get_nearest_visible_wp(the_important_entity->client->ps.origin, the_important_entity->s.number);
		}
		else
		{
			wp_close = get_nearest_visible_wp(the_important_entity->r.currentOrigin, the_important_entity->s.number);
		}

		if (wp_close != -1 && gWPArray[wp_close] && gWPArray[wp_close]->inuse)
		{
			bs->wpDestination = gWPArray[wp_close];
			bs->destinationGrabTime = level.time + 4000;
		}
	}

	return 1;
}

//see if we already have an item/powerup/etc. that is associated
//with this waypoint.
static int bot_has_associated(const bot_state_t* bs, const wpobject_t* wp)
{
	if (wp->associated_entity == ENTITYNUM_NONE)
	{
		//make it think this is an item we have so we don't go after nothing
		return 1;
	}

	const gentity_t* as = &g_entities[wp->associated_entity];

	if (!as || !as->item)
	{
		return 0;
	}

	if (as->item->giType == IT_WEAPON)
	{
		if (bs->cur_ps.stats[STAT_WEAPONS] & 1 << as->item->giTag)
		{
			return 1;
		}

		return 0;
	}
	if (as->item->giType == IT_HOLDABLE)
	{
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << as->item->giTag)
		{
			return 1;
		}

		return 0;
	}
	if (as->item->giType == IT_POWERUP)
	{
		if (bs->cur_ps.powerups[as->item->giTag])
		{
			return 1;
		}

		return 0;
	}
	if (as->item->giType == IT_AMMO)
	{
		if (bs->cur_ps.ammo[as->item->giTag] > 10) //hack
		{
			return 1;
		}

		return 0;
	}

	return 0;
}

//we don't really have anything we want to do right now,
//let's just find the best thing to do given the current
//situation.
int get_best_idle_goal(bot_state_t* bs)
{
	int i = 0;
	int highestweight = 0;
	int desiredindex = -1;

	if (!bs->wpCurrent)
	{
		return -1;
	}

	if (bs->isCamper != 2)
	{
		if (bs->randomNavTime < level.time)
		{
			if (Q_irand(1, 10) < 5)
			{
				bs->randomNav = 1;
			}
			else
			{
				bs->randomNav = 0;
			}

			bs->randomNavTime = level.time + Q_irand(5000, 15000);
		}
	}

	if (bs->randomNav)
	{
		//stop looking for items and/or camping on them
		return -1;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] &&
			gWPArray[i]->inuse &&
			gWPArray[i]->flags & WPFLAG_GOALPOINT &&
			gWPArray[i]->weight > highestweight &&
			!bot_has_associated(bs, gWPArray[i]))
		{
			const int traildist = total_trail_distance(bs->wpCurrent->index, i);

			if (traildist != -1)
			{
				int dist_to_weight = traildist / 10000;
				dist_to_weight = gWPArray[i]->weight - dist_to_weight;

				if (dist_to_weight > highestweight)
				{
					highestweight = dist_to_weight;
					desiredindex = i;
				}
			}
		}

		i++;
	}

	return desiredindex;
}

//go through the list of possible priorities for navigating
//and work out the best destination point.
static void get_ideal_destination(bot_state_t* bs)
{
	int temp_int, idleWP;
	float dist_change, plus_len, minus_len;
	vec3_t usethisvec, a;
	gentity_t* badthing;

#ifdef _DEBUG
	trap->Cvar_Update(&bot_nogoals);

	if (bot_nogoals.integer)
	{
		return;
	}
#endif

	if (!bs->wpCurrent)
	{
		return;
	}

	if (level.time - bs->escapeDirTime > 4000)
	{
		badthing = get_nearest_bad_thing(bs);
	}
	else
	{
		badthing = NULL;
	}

	if (badthing && badthing->inuse &&
		badthing->health > 0 && badthing->takedamage)
	{
		bs->dangerousObject = badthing;
	}
	else
	{
		bs->dangerousObject = NULL;
	}

	if (!badthing && bs->wpDestIgnoreTime > level.time)
	{
		return;
	}

	if (!badthing && bs->dontGoBack > level.time)
	{
		if (bs->wpDestination)
		{
			bs->wpStoreDest = bs->wpDestination;
		}
		bs->wpDestination = NULL;
		return;
	}
	if (!badthing && bs->wpStoreDest)
	{
		//after we finish running away, switch back to our original destination
		bs->wpDestination = bs->wpStoreDest;
		bs->wpStoreDest = NULL;
	}

	if (badthing && bs->wpCamping)
	{
		bs->wpCamping = NULL;
	}

	if (bs->wpCamping)
	{
		bs->wpDestination = bs->wpCamping;
		return;
	}

	if (!badthing && ctf_takes_priority(bs))
	{
		if (bs->ctfState)
		{
			bs->runningToEscapeThreat = 1;
		}
		return;
	}
	if (!badthing && siege_takes_priority(bs))
	{
		if (bs->siegeState)
		{
			bs->runningToEscapeThreat = 1;
		}
		return;
	}
	if (!badthing && jm_takes_priority(bs))
	{
		bs->runningToEscapeThreat = 1;
	}

	if (badthing)
	{
		bs->runningLikeASissy = level.time + 100;

		if (bs->wpDestination)
		{
			bs->wpStoreDest = bs->wpDestination;
		}
		bs->wpDestination = NULL;

		if (bs->wpDirection)
		{
			temp_int = bs->wpCurrent->index + 1;
		}
		else
		{
			temp_int = bs->wpCurrent->index - 1;
		}

		if (gWPArray[temp_int] && gWPArray[temp_int]->inuse && bs->escapeDirTime < level.time)
		{
			VectorSubtract(badthing->s.pos.trBase, bs->wpCurrent->origin, a);
			plus_len = VectorLength(a);
			VectorSubtract(badthing->s.pos.trBase, gWPArray[temp_int]->origin, a);
			minus_len = VectorLength(a);

			if (plus_len < minus_len)
			{
				if (bs->wpDirection)
				{
					bs->wpDirection = 0;
				}
				else
				{
					bs->wpDirection = 1;
				}

				bs->wpCurrent = gWPArray[temp_int];

				bs->escapeDirTime = level.time + Q_irand(500, 1000);
			}
		}
		return;
	}

	dist_change = 0; //keep the compiler from complaining

	temp_int = bot_get_weapon_range(bs);

	if (temp_int == BWEAPONRANGE_MELEE)
	{
		dist_change = 1;
	}
	else if (temp_int == BWEAPONRANGE_SABER)
	{
		dist_change = 1;
	}
	else if (temp_int == BWEAPONRANGE_MID)
	{
		dist_change = 128;
	}
	else if (temp_int == BWEAPONRANGE_LONG)
	{
		dist_change = 300;
	}

	if (bs->revengeEnemy && bs->revengeEnemy->health > 0 &&
		bs->revengeEnemy->client && bs->revengeEnemy->client->pers.connected == CON_CONNECTED)
	{
		//if we hate someone, always try to get to them
		if (bs->wpDestSwitchTime < level.time)
		{
			if (bs->revengeEnemy->client)
			{
				VectorCopy(bs->revengeEnemy->client->ps.origin, usethisvec);
			}
			else
			{
				VectorCopy(bs->revengeEnemy->s.origin, usethisvec);
			}

			temp_int = get_nearest_visible_wp(usethisvec, 0);

			if (temp_int != -1 && total_trail_distance(bs->wpCurrent->index, temp_int) != -1)
			{
				bs->wpDestination = gWPArray[temp_int];
				bs->wpDestSwitchTime = level.time + Q_irand(5000, 10000);
			}
		}
	}
	else if (bs->squadLeader && bs->squadLeader->health > 0 &&
		bs->squadLeader->client && bs->squadLeader->client->pers.connected == CON_CONNECTED)
	{
		if (bs->wpDestSwitchTime < level.time)
		{
			if (bs->squadLeader->client)
			{
				VectorCopy(bs->squadLeader->client->ps.origin, usethisvec);
			}
			else
			{
				VectorCopy(bs->squadLeader->s.origin, usethisvec);
			}

			temp_int = get_nearest_visible_wp(usethisvec, 0);

			if (temp_int != -1 && total_trail_distance(bs->wpCurrent->index, temp_int) != -1)
			{
				bs->wpDestination = gWPArray[temp_int];
				bs->wpDestSwitchTime = level.time + Q_irand(5000, 10000);
			}
		}
	}
	else if (bs->currentEnemy)
	{
		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, usethisvec);
		}
		else
		{
			VectorCopy(bs->currentEnemy->s.origin, usethisvec);
		}

		const int b_chicken = bot_is_a_chicken_wuss(bs);
		bs->runningToEscapeThreat = b_chicken;

		if (bs->frame_Enemy_Len < dist_change || b_chicken && b_chicken != 2)
		{
			const int c_wp_index = bs->wpCurrent->index;

			if (bs->frame_Enemy_Len > 400)
			{
				//good distance away, start running toward a good place for an item or powerup or whatever
				idleWP = get_best_idle_goal(bs);

				if (idleWP != -1 && gWPArray[idleWP] && gWPArray[idleWP]->inuse)
				{
					bs->wpDestination = gWPArray[idleWP];
				}
			}
			else if (gWPArray[c_wp_index - 1] && gWPArray[c_wp_index - 1]->inuse &&
				gWPArray[c_wp_index + 1] && gWPArray[c_wp_index + 1]->inuse)
			{
				VectorSubtract(gWPArray[c_wp_index + 1]->origin, usethisvec, a);
				plus_len = VectorLength(a);
				VectorSubtract(gWPArray[c_wp_index - 1]->origin, usethisvec, a);
				minus_len = VectorLength(a);

				if (minus_len > plus_len)
				{
					bs->wpDestination = gWPArray[c_wp_index - 1];
				}
				else
				{
					bs->wpDestination = gWPArray[c_wp_index + 1];
				}
			}
		}
		else if (b_chicken != 2 && bs->wpDestSwitchTime < level.time)
		{
			temp_int = get_nearest_visible_wp(usethisvec, 0);

			if (temp_int != -1 && total_trail_distance(bs->wpCurrent->index, temp_int) != -1)
			{
				bs->wpDestination = gWPArray[temp_int];

				if (level.gametype == GT_SINGLE_PLAYER)
				{
					//be more aggressive
					bs->wpDestSwitchTime = level.time + Q_irand(300, 1000);
				}
				else
				{
					bs->wpDestSwitchTime = level.time + Q_irand(1000, 5000);
				}
			}
		}
	}

	if (!bs->wpDestination && bs->wpDestSwitchTime < level.time)
	{
		//trap->Print("I need something to do\n");
		idleWP = get_best_idle_goal(bs);

		if (idleWP != -1 && gWPArray[idleWP] && gWPArray[idleWP]->inuse)
		{
			bs->wpDestination = gWPArray[idleWP];
		}
	}
}

//commander CTF AI - tell other bots in the so-called
//"squad" what to do.
static void commander_bot_ctfai(const bot_state_t* bs)
{
	int i = 0;
	gentity_t* ent;
	int squadmates = 0;
	gentity_t* squad[MAX_CLIENTS];
	int defend_attack_priority = 0; //0 == attack, 1 == defend
	int guard_defend_priority = 0; //0 == defend, 1 == guard
	int attack_retrieve_priority = 0; //0 == retrieve, 1 == attack
	int my_flag;
	int enemy_flag;
	int enemy_has_our_flag = 0;
	int we_have_enemy_flag = 0;
	int num_on_my_team = 0;
	int num_on_enemy_team = 0;
	int num_attackers = 0;
	int num_defenders = 0;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		my_flag = PW_REDFLAG;
	}
	else
	{
		my_flag = PW_BLUEFLAG;
	}

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		enemy_flag = PW_BLUEFLAG;
	}
	else
	{
		enemy_flag = PW_REDFLAG;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client)
		{
			if (ent->client->ps.powerups[enemy_flag] && OnSameTeam(&g_entities[bs->client], ent))
			{
				we_have_enemy_flag = 1;
			}
			else if (ent->client->ps.powerups[my_flag] && !OnSameTeam(&g_entities[bs->client], ent))
			{
				enemy_has_our_flag = 1;
			}

			if (OnSameTeam(&g_entities[bs->client], ent))
			{
				num_on_my_team++;
			}
			else
			{
				num_on_enemy_team++;
			}

			if (botstates[ent->s.number])
			{
				if (botstates[ent->s.number]->ctfState == CTFSTATE_ATTACKER ||
					botstates[ent->s.number]->ctfState == CTFSTATE_RETRIEVAL)
				{
					num_attackers++;
				}
				else
				{
					num_defenders++;
				}
			}
			else
			{
				//assume real players to be attackers in our logic
				num_attackers++;
			}
		}
		i++;
	}

	i = 0;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && botstates[i] && botstates[i]->squadLeader && botstates[i]->squadLeader->s.number == bs
			->client && i != bs->client)
		{
			squad[squadmates] = ent;
			squadmates++;
		}

		i++;
	}

	squad[squadmates] = &g_entities[bs->client];
	squadmates++;

	i = 0;

	if (enemy_has_our_flag && !we_have_enemy_flag)
	{
		//start off with an attacker instead of a retriever if we don't have the enemy flag yet so that they can't capture it first.
		//after that we focus on getting our flag back.
		attack_retrieve_priority = 1;
	}

	while (i < squadmates)
	{
		if (squad[i] && squad[i]->client && botstates[squad[i]->s.number])
		{
			if (botstates[squad[i]->s.number]->ctfState != CTFSTATE_GETFLAGHOME)
			{
				//never tell a bot to stop trying to bring the flag to the base
				if (defend_attack_priority)
				{
					if (we_have_enemy_flag)
					{
						if (guard_defend_priority)
						{
							botstates[squad[i]->s.number]->ctfState = CTFSTATE_GUARDCARRIER;
							guard_defend_priority = 0;
						}
						else
						{
							botstates[squad[i]->s.number]->ctfState = CTFSTATE_DEFENDER;
							guard_defend_priority = 1;
						}
					}
					else
					{
						botstates[squad[i]->s.number]->ctfState = CTFSTATE_DEFENDER;
					}
					defend_attack_priority = 0;
				}
				else
				{
					if (enemy_has_our_flag)
					{
						if (attack_retrieve_priority)
						{
							botstates[squad[i]->s.number]->ctfState = CTFSTATE_ATTACKER;
							attack_retrieve_priority = 0;
						}
						else
						{
							botstates[squad[i]->s.number]->ctfState = CTFSTATE_RETRIEVAL;
							attack_retrieve_priority = 1;
						}
					}
					else
					{
						botstates[squad[i]->s.number]->ctfState = CTFSTATE_ATTACKER;
					}
					defend_attack_priority = 1;
				}
			}
			else if ((num_on_my_team < 2 || !num_attackers) && enemy_has_our_flag)
			{
				//I'm the only one on my team who will attack and the enemy has my flag, I have to go after him
				botstates[squad[i]->s.number]->ctfState = CTFSTATE_RETRIEVAL;
			}
		}

		i++;
	}
}

//similar to ctf ai, for siege
static void commander_bot_siege_ai(const bot_state_t* bs)
{
	int i = 0;
	int squadmates = 0;
	int commanded = 0;
	int teammates = 0;
	gentity_t* squad[MAX_CLIENTS];
	bot_state_t* bst;

	while (i < MAX_CLIENTS)
	{
		gentity_t* ent = &g_entities[i];

		if (ent && ent->client && OnSameTeam(&g_entities[bs->client], ent) && botstates[ent->s.number])
		{
			bst = botstates[ent->s.number];

			if (bst && !bst->isSquadLeader && !bst->state_Forced)
			{
				squad[squadmates] = ent;
				squadmates++;
			}
			else if (bst && !bst->isSquadLeader && bst->state_Forced)
			{
				//count them as commanded
				commanded++;
			}
		}

		if (ent && ent->client && OnSameTeam(&g_entities[bs->client], ent))
		{
			teammates++;
		}

		i++;
	}

	if (!squadmates)
	{
		return;
	}

	//tell squad mates to do what I'm doing, up to half of team, let the other half make their own decisions
	i = 0;

	while (i < squadmates && squad[i])
	{
		bst = botstates[squad[i]->s.number];

		if (commanded > teammates / 2)
		{
			break;
		}

		if (bst)
		{
			bst->state_Forced = bs->siegeState;
			bst->siegeState = bs->siegeState;
			commanded++;
		}

		i++;
	}
}

//teamplay ffa squad ai
static void bot_do_teamplay_ai(bot_state_t* bs)
{
	if (bs->state_Forced)
	{
		bs->teamplayState = bs->state_Forced;
	}

	if (bs->teamplayState == TEAMPLAYSTATE_REGROUP)
	{
		//force to find a new leader
		bs->squadLeader = NULL;
		bs->isSquadLeader = 0;
	}
}

//like ctf and siege commander ai, instruct the squad
static void commander_bot_teamplay_ai(bot_state_t* bs)
{
	int i = 0;
	int squadmates = 0;
	int teammates = 0;
	int teammate_indanger = -1;
	int teammate_helped = 0;
	int foundsquadleader = 0;
	int worsthealth = 50;
	gentity_t* squad[MAX_CLIENTS];
	bot_state_t* bst;

	while (i < MAX_CLIENTS)
	{
		gentity_t* ent = &g_entities[i];

		if (ent && ent->client && OnSameTeam(&g_entities[bs->client], ent) && botstates[ent->s.number])
		{
			bst = botstates[ent->s.number];

			if (foundsquadleader && bst && bst->isSquadLeader)
			{
				//never more than one squad leader
				bst->isSquadLeader = 0;
			}

			if (bst && !bst->isSquadLeader)
			{
				squad[squadmates] = ent;
				squadmates++;
			}
			else if (bst)
			{
				foundsquadleader = 1;
			}
		}

		if (ent && ent->client && OnSameTeam(&g_entities[bs->client], ent))
		{
			teammates++;

			if (ent->health < worsthealth)
			{
				teammate_indanger = ent->s.number;
				worsthealth = ent->health;
			}
		}

		i++;
	}

	if (!squadmates)
	{
		return;
	}

	i = 0;

	while (i < squadmates && squad[i])
	{
		bst = botstates[squad[i]->s.number];

		if (bst && !bst->state_Forced)
		{
			//only order if this guy is not being ordered directly by the real player team leader
			if (teammate_indanger >= 0 && !teammate_helped)
			{
				//send someone out to help whoever needs help most at the moment
				bst->teamplayState = TEAMPLAYSTATE_ASSISTING;
				bst->squadLeader = &g_entities[teammate_indanger];
				teammate_helped = 1;
			}
			else if ((teammate_indanger == -1 || teammate_helped) && bst->teamplayState == TEAMPLAYSTATE_ASSISTING)
			{
				//no teammates need help badly, but this guy is trying to help them anyway, so stop
				bst->teamplayState = TEAMPLAYSTATE_FOLLOWING;
				bst->squadLeader = &g_entities[bs->client];
			}

			if (bs->squadRegroupInterval < level.time && Q_irand(1, 10) < 5)
			{
				//every so often tell the squad to regroup for the sake of variation
				if (bst->teamplayState == TEAMPLAYSTATE_FOLLOWING)
				{
					bst->teamplayState = TEAMPLAYSTATE_REGROUP;
				}

				bs->isSquadLeader = 0;
				bs->squadCannotLead = level.time + 500;
				bs->squadRegroupInterval = level.time + Q_irand(45000, 65000);
			}
		}

		i++;
	}
}

//pick which commander ai to use based on gametype
static void commander_bot_ai(bot_state_t* bs)
{
	if (level.gametype == GT_CTF || level.gametype == GT_CTY)
	{
		commander_bot_ctfai(bs);
	}
	else if (level.gametype == GT_SIEGE)
	{
		commander_bot_siege_ai(bs);
	}
	else if (level.gametype == GT_TEAM)
	{
		commander_bot_teamplay_ai(bs);
	}
}

//close range combat routines
static void melee_combat_handling(bot_state_t* bs)
{
	if (!bs->currentEnemy)
		return;

	// -----------------------------
	// SAFE VECTOR INITIALIZATION
	// -----------------------------
	vec3_t enemyPos = { 0 };
	vec3_t downvec = { 0 };
	vec3_t midorg = { 0 };
	vec3_t a = { 0 };
	vec3_t fwd = { 0 };
	vec3_t mins = { -15, -15, -24 };
	vec3_t maxs = { 15,  15,  32 };
	trace_t tr;

	// -----------------------------
	// GET ENEMY POSITION
	// -----------------------------
	if (bs->currentEnemy->client)
		VectorCopy(bs->currentEnemy->client->ps.origin, enemyPos);
	else
		VectorCopy(bs->currentEnemy->s.origin, enemyPos);

	// -----------------------------
	// STRAFE DIRECTION TIMER
	// -----------------------------
	if (bs->meleeStrafeTime < level.time)
	{
		bs->meleeStrafeDir = !bs->meleeStrafeDir;
		bs->meleeStrafeTime = level.time + Q_irand(500, 1800);
	}

	// -----------------------------
	// GROUND CHECKS (SAFE)
	// -----------------------------
	// Enemy ground
	VectorCopy(enemyPos, downvec);
	downvec[2] -= 4096;

	trap->Trace(&tr, enemyPos, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);
	int en_down = tr.endpos[2];

	// Our ground
	VectorCopy(bs->origin, downvec);
	downvec[2] -= 4096;

	trap->Trace(&tr, bs->origin, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);
	int me_down = tr.endpos[2];

	// -----------------------------
	// MIDPOINT GROUND CHECK
	// -----------------------------
	VectorSubtract(enemyPos, bs->origin, a);

	if (VectorLength(a) < 0.001f)
		VectorSet(a, 1, 0, 0); // safe fallback

	vectoangles(a, a);
	AngleVectors(a, fwd, NULL, NULL);

	VectorMA(bs->origin, bs->frame_Enemy_Len * 0.5f, fwd, midorg);

	VectorCopy(midorg, downvec);
	downvec[2] -= 4096;

	trap->Trace(&tr, midorg, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);
	int mid_down = tr.endpos[2];

	// -----------------------------
	// SAME GROUND LEVEL → MOVE IN
	// -----------------------------
	if (me_down == en_down && en_down == mid_down)
	{
		// Melee is simple: just close the distance
		VectorCopy(enemyPos, bs->goalPosition);
	}
}

void adjustfor_strafe(const bot_state_t* bs, vec3_t move_dir)
{
	vec3_t right;

	if (!bs->meleeStrafeDir || bs->meleeStrafeTime < level.time)
	{
		//no strafe
		return;
	}

	AngleVectors(g_entities[bs->client].client->ps.viewangles, NULL, right, NULL);

	//flaten up/down
	right[2] = 0;

	if (bs->meleeStrafeDir == 2)
	{
		//strafing left
		VectorScale(right, -1, right);
	}

	//We assume that moveDir has been normalized before this function.
	VectorAdd(move_dir, right, move_dir);
	VectorNormalize(move_dir);
}

static void movefor_attack_quad(const bot_state_t* bs, vec3_t move_dir, const int Quad)
{
	//set the moveDir to set our attack direction to be towards this Quad.
	vec3_t forward, right;

	AngleVectors(bs->viewangles, forward, right, NULL);

	switch (Quad)
	{
	case Q_B: //down strike.
		VectorCopy(forward, move_dir);
		break;
	case Q_BR: //down right strike
		VectorAdd(forward, right, move_dir);
		VectorNormalize(move_dir);
		break;
	case Q_R: //right strike
		VectorCopy(right, move_dir);
		break;
	case Q_TR: //up right strike
		VectorScale(forward, -1, forward);
		VectorAdd(forward, right, move_dir);
		VectorNormalize(move_dir);
		break;
	case Q_T: //up strike
		VectorScale(forward, -1, forward);
		VectorCopy(forward, move_dir);
		break;
	case Q_TL: //up left strike
		VectorScale(forward, -1, forward);
		VectorScale(right, -1, right);
		VectorAdd(forward, right, move_dir);
		VectorNormalize(move_dir);
		break;
	case Q_L: //left strike
		VectorScale(right, -1, right);
		VectorCopy(right, move_dir);
		break;
	case Q_BL: //down left strike.
		VectorScale(right, -1, right);
		VectorAdd(forward, right, move_dir);
		VectorNormalize(move_dir);
		break;
	default:
		break;
	}
}

static qboolean bot_behave_check_backstab(bot_state_t* bs)
{
	// Check if there is an enemy behind us that we can backstab...
	vec3_t forward, back_org, cur_org, move_dir;
	trace_t tr;

	if (bs->cur_ps.weapon != WP_SABER)
		return qfalse;

	VectorCopy(bs->cur_ps.origin, cur_org);
	cur_org[2] += 24;
	AngleVectors(bs->cur_ps.viewangles, forward, NULL, NULL);
	VectorMA(cur_org, -64, forward, back_org);

	trap->Trace(&tr, cur_org, NULL, NULL, back_org, bs->client, MASK_SHOT, 0, 0, 0);

	if (tr.entityNum < 0 || tr.entityNum > ENTITYNUM_MAX_NORMAL)
		return qfalse;

	const gentity_t* enemy = &g_entities[tr.entityNum];

	if (!enemy
		|| enemy->s.eType != ET_PLAYER && enemy->s.eType != ET_NPC
		|| !enemy->client
		|| enemy->health < 1
		|| enemy->client->ps.eFlags & EF_DEAD)
		return qfalse;

	if (OnSameTeam(&g_entities[bs->client], enemy))
		return qfalse;

	// OK, let's stab away! bwahahaha!
	VectorCopy(bs->cur_ps.origin, cur_org);
	VectorMA(cur_org, -64, forward, back_org);

	VectorSubtract(cur_org, bs->origin, move_dir);

	move_dir[2] = 0;
	VectorNormalize(move_dir);

	//adjust the moveDir to do strafing
	adjustfor_strafe(bs, move_dir);
	trace_move(bs, move_dir, tr.entityNum);
	trap->EA_Move(bs->client, move_dir, 5000);
	trap->EA_Attack(bs->client);

	return qtrue;
}

static qboolean bot_behave_check_use_kata(const bot_state_t* bs)
{
	// Check if there is an in front of us that we can use our kata on...
	vec3_t forward, back_org, cur_org;
	trace_t tr;

	if (bs->cur_ps.weapon != WP_SABER)
	{
		return qfalse;
	}

	VectorCopy(bs->cur_ps.origin, cur_org);
	cur_org[2] += 24;
	AngleVectors(bs->cur_ps.viewangles, forward, NULL, NULL);
	VectorMA(cur_org, 64, forward, back_org);

	trap->Trace(&tr, cur_org, NULL, NULL, back_org, bs->client, MASK_SHOT, 0, 0, 0);

	if (tr.entityNum < 0 || tr.entityNum > ENTITYNUM_MAX_NORMAL)
		return qfalse;

	const gentity_t* enemy = &g_entities[tr.entityNum];

	if (!enemy
		|| enemy->s.eType != ET_PLAYER && enemy->s.eType != ET_NPC
		|| !enemy->client
		|| enemy->health < 1
		|| enemy->client->ps.eFlags & EF_DEAD)
		return qfalse;

	if (OnSameTeam(&g_entities[bs->client], enemy))
		return qfalse;

	// OK. We have a target. First make it not happen constantly with some randomization!
	if (irand(0, 200) > 50)
		return qfalse;

	// OK, let's stab away! bwahahaha!
	trap->EA_Attack(bs->client);
	trap->EA_Alt_Attack(bs->client);

	return qtrue;
}

static qboolean bot_behave_check_use_crouch_attack(bot_state_t* bs)
{
	// Check if there is an in front of us that we can use our special crouch attack on...
	vec3_t forward, back_org, cur_org, moveDir;
	trace_t tr;

	if (bs->cur_ps.weapon != WP_SABER)
		return qfalse;

	VectorCopy(bs->cur_ps.origin, cur_org);
	cur_org[2] += 24;
	AngleVectors(bs->cur_ps.viewangles, forward, NULL, NULL);
	VectorMA(cur_org, 64, forward, back_org);

	trap->Trace(&tr, cur_org, NULL, NULL, back_org, bs->client, MASK_SHOT, 0, 0, 0);

	if (tr.entityNum < 0 || tr.entityNum > ENTITYNUM_MAX_NORMAL)
		return qfalse;

	const gentity_t* enemy = &g_entities[tr.entityNum];

	if (!enemy
		|| enemy->s.eType != ET_PLAYER && enemy->s.eType != ET_NPC
		|| !enemy->client
		|| enemy->health < 1
		|| enemy->client->ps.eFlags & EF_DEAD)
		return qfalse;

	if (OnSameTeam(&g_entities[bs->client], enemy))
		return qfalse;

	// OK. We have a target. First make it not happen constantly with some randomization!
	if (irand(0, 200) > 10)
		return qfalse;

	// OK, let's stab away! bwahahaha!
	VectorCopy(bs->cur_ps.origin, cur_org);
	VectorMA(cur_org, 64, forward, back_org);

	VectorSubtract(cur_org, bs->origin, moveDir);

	moveDir[2] = 0;
	VectorNormalize(moveDir);

	//adjust the moveDir to do strafing
	adjustfor_strafe(bs, moveDir);
	trace_move(bs, moveDir, tr.entityNum);
	trap->EA_Crouch(bs->client);
	trap->EA_Move(bs->client, moveDir, 5000);
	trap->EA_Attack(bs->client);
	trap->EA_Alt_Attack(bs->client);

	return qtrue;
}

void bot_behave_attack_basic(bot_state_t* bs, const gentity_t* target)
{
	vec3_t enemy_origin, view_dir, ang, move_dir;

	FindOrigin(target, enemy_origin);

	const float dist = TargetDistance(bs, target, enemy_origin);

	//adjust angle for target leading.
	const float leadamount = bot_weapon_can_lead(bs);

	bot_aim_leading(bs, enemy_origin, leadamount);

	//face enemy
	VectorSubtract(enemy_origin, bs->eye, view_dir);
	vectoangles(view_dir, ang);
	ang[PITCH] = 0;
	ang[ROLL] = 0;
	VectorCopy(ang, bs->goalAngles);

	//check to see if there's a detpack in the immediate area of the target.
	if (bs->cur_ps.stats[STAT_WEAPONS] & 1 << WP_DET_PACK)
	{
		//only check if you got det packs.
		bot_weapon_detpack(bs, target);
	}

	if (!PM_SaberInKata(bs->cur_ps.saber_move) && bs->cur_ps.fd.forcePower > 80 &&
		bs->cur_ps.weapon == WP_SABER && dist < 128 && in_field_of_vision(bs->viewangles, 90, ang))
	{
		//KATA!
		trap->EA_Attack(bs->client);
		trap->EA_Alt_Attack(bs->client);
		return;
	}

	if (bs->meleeStrafeTime < level.time)
	{
		//select a new strafing direction
		//0 = no strafe
		//1 = strafe right
		//2 = strafe left
		bs->meleeStrafeDir = Q_irand(0, 2);
		bs->meleeStrafeTime = level.time + Q_irand(500, 1800);
	}

	VectorSubtract(enemy_origin, bs->origin, move_dir);

	if (dist < MinimumAttackDistance[bs->virtualWeapon])
	{
		//move back
		VectorScale(move_dir, -1, move_dir);
	}
	else if (dist < IdealAttackDistance[bs->virtualWeapon])
	{
		//we're close enough, quit moving closer
		VectorClear(move_dir);
	}

	move_dir[2] = 0;
	VectorNormalize(move_dir);

	if (bs->virtualWeapon == WP_SABER)
	{
		// Added, backstab check...
		if (bot_behave_check_backstab(bs))
		{
			return;
		}

		// Added, kata check...
		if (bot_behave_check_use_kata(bs))
		{
			return;
		}

		// Added, special crouch attack check...
		if (bot_behave_check_use_crouch_attack(bs))
		{
			return;
		}
	}

	//adjust the moveDir to do strafing
	adjustfor_strafe(bs, move_dir);

	if (bs->cur_ps.weapon == bs->virtualWeapon
		&& bs->virtualWeapon == WP_SABER && in_field_of_vision(bs->viewangles, 100, ang))
	{
		//we're using a lightsaber
		if (PM_SaberInIdle(bs->cur_ps.saber_move)
			|| PM_SaberInBounce(bs->cur_ps.saber_move)
			|| PM_SaberInReturn(bs->cur_ps.saber_move))
		{
			//we want to attack, and we need to choose a new attack swing, pick randomly.
			movefor_attack_quad(bs, move_dir, Q_irand(Q_BR, Q_B));
		}
		else if (bs->cur_ps.userInt3 & 1 << FLAG_ATTACKFAKE)
		{
			//successfully started an attack fake, don't do it again for a while.
			bs->saberBFTime = level.time + Q_irand(3000, 5000); //every 3-5 secs
		}
		else if (bs->saberBFTime < level.time
			&& (PM_SaberInTransition(bs->cur_ps.saber_move)
				|| PM_SaberInStart(bs->cur_ps.saber_move)))
		{
			//we can and want to do a saber attack fake.
			int fake_quad = Q_irand(Q_BR, Q_B);
			while (fake_quad == saber_moveData[bs->cur_ps.saber_move].endQuad)
			{
				//can't fake in the direction we're already trying to attack in
				fake_quad = Q_irand(Q_BR, Q_B);
			}
			//start trying to fake
			movefor_attack_quad(bs, move_dir, fake_quad);
			trap->EA_Alt_Attack(bs->client);
		}
	}

	if (!VectorCompare(vec3_origin, move_dir))
	{
		trace_move(bs, move_dir, target->s.clientNum);
		trap->EA_Move(bs->client, move_dir, 5000);
	}

	if (bs->frame_Enemy_Vis && bs->cur_ps.weapon == bs->virtualWeapon
		&& (in_field_of_vision(bs->viewangles, 30, ang)
			|| bs->virtualWeapon == WP_SABER && in_field_of_vision(bs->viewangles, 100, ang)))
	{
		//not switching weapons so attack
		trap->EA_Attack(bs->client);

		if (bs->virtualWeapon == WP_SABER)
		{
			//only walk while attacking with the saber.
			bs->doWalk = qtrue;
		}
	}
}

//saber combat routines (it's simple, but it works)
static void saber_combat_handling(bot_state_t* bs)
{
	vec3_t usethisvec;
	vec3_t downvec;
	vec3_t midorg;
	vec3_t a;
	vec3_t fwd, ang, move_dir;
	vec3_t mins, maxs;
	trace_t tr;
	int me_down;

	if (!bs->currentEnemy)
	{
		return;
	}

	if (bs->currentEnemy->client)
	{
		VectorCopy(bs->currentEnemy->client->ps.origin, usethisvec);
	}
	else
	{
		VectorCopy(bs->currentEnemy->s.origin, usethisvec);
	}

	if (bs->meleeStrafeTime < level.time)
	{
		if (bs->meleeStrafeDir)
		{
			bs->meleeStrafeDir = 0;
		}
		else
		{
			bs->meleeStrafeDir = 1;
		}

		bs->meleeStrafeTime = level.time + Q_irand(500, 1800);
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -24;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	VectorCopy(usethisvec, downvec);
	downvec[2] -= 4096;

	trap->Trace(&tr, usethisvec, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);

	int en_down = (int)tr.endpos[2];

	if (tr.startsolid || tr.allsolid)
	{
		en_down = 1;
		me_down = 2;
	}
	else
	{
		VectorCopy(bs->origin, downvec);
		downvec[2] -= 4096;

		trap->Trace(&tr, bs->origin, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);

		me_down = (int)tr.endpos[2];

		if (tr.startsolid || tr.allsolid)
		{
			en_down = 1;
			me_down = 2;
		}
	}

	VectorSubtract(usethisvec, bs->origin, a);
	vectoangles(a, a);
	AngleVectors(a, fwd, NULL, NULL);

	midorg[0] = bs->origin[0] + fwd[0] * bs->frame_Enemy_Len / 2;
	midorg[1] = bs->origin[1] + fwd[1] * bs->frame_Enemy_Len / 2;
	midorg[2] = bs->origin[2] + fwd[2] * bs->frame_Enemy_Len / 2;

	VectorCopy(midorg, downvec);
	downvec[2] -= 4096;

	trap->Trace(&tr, midorg, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);

	const int mid_down = (int)tr.endpos[2];

	if (me_down == en_down && en_down == mid_down) // - Both over the same level of ground
	{
		if (bs->frame_Enemy_Len > 128)
		{
			//be ready to attack
			//this should be an attack while moving function but for now we'll just use moveto
			vec3_t enemyOrigin;
			FindOrigin(bs->currentEnemy, enemyOrigin);
			VectorCopy(enemyOrigin, bs->DestPosition);
			bs->DestIgnore = bs->currentEnemy->s.number;
			bot_behave_attack_move(bs);
			return;
		}
		if (bs->saberDefendDecideTime < level.time)
		{
			if (bs->saberDefending)
			{
				bs->saberDefending = 0;
			}
			else
			{
				bs->saberDefending = 1;
			}

			bs->saberDefendDecideTime = level.time + Q_irand(500, 2000);
		}

		if (bs->frame_Enemy_Len < 54) // (How far away you are from him)
		{
			VectorCopy(bs->origin, bs->goalPosition);
			bs->saberBFTime = 0;
		}

		if (bs->currentEnemy && bs->currentEnemy->client)
		{
			if (!PM_SaberInSpecial(bs->currentEnemy->client->ps.saber_move)
				&& bs->frame_Enemy_Len > 90
				&& bs->saberBFTime > level.time
				&& bs->saberBTime > level.time
				&& bs->beStill < level.time
				&& bs->saberSTime < level.time)
			{
				bs->beStill = level.time + Q_irand(500, 1000);
				bs->saberSTime = level.time + Q_irand(1200, 1800);
			}
			else if (bs->currentEnemy->client->ps.weapon == WP_SABER
				&& bs->frame_Enemy_Len < 80.0f
				&& (Q_irand(1, 10) < 8
					&& bs->saberBFTime < level.time || bs->saberBTime > level.time
					|| PM_SaberInKata(bs->currentEnemy->client->ps.saber_move)
					|| bs->currentEnemy->client->ps.saber_move == LS_SPINATTACK
					|| bs->currentEnemy->client->ps.saber_move == LS_SPINATTACK_GRIEV
					|| bs->currentEnemy->client->ps.saber_move == LS_SPINATTACK_DUAL))
			{
				vec3_t vs;
				vec3_t groundcheck;
				int ideal_dist;
				int check_incr = 0;

				VectorSubtract(bs->origin, bs->goalPosition, vs);
				VectorNormalize(vs);

				if (PM_SaberInKata(bs->currentEnemy->client->ps.saber_move)
					|| bs->currentEnemy->client->ps.saber_move == LS_SPINATTACK
					|| bs->currentEnemy->client->ps.saber_move == LS_SPINATTACK_GRIEV
					|| bs->currentEnemy->client->ps.saber_move == LS_SPINATTACK_DUAL)
				{
					ideal_dist = 256;
				}
				else
				{
					ideal_dist = 64;
				}

				while (check_incr < ideal_dist)
				{
					bs->goalPosition[0] = bs->origin[0] + vs[0] * check_incr;
					bs->goalPosition[1] = bs->origin[1] + vs[1] * check_incr;
					bs->goalPosition[2] = bs->origin[2] + vs[2] * check_incr;

					if (bs->saberBTime < level.time)
					{
						bs->saberBFTime = level.time + Q_irand(900, 1300);
						bs->saberBTime = level.time + Q_irand(300, 700);
					}

					VectorCopy(bs->goalPosition, groundcheck);

					groundcheck[2] -= 64;

					trap->Trace(&tr, bs->goalPosition, NULL, NULL, groundcheck, bs->client, MASK_SOLID, qfalse, 0, 0);

					if (tr.fraction == 1.0f)
					{
						//don't back off of a ledge
						VectorCopy(usethisvec, bs->goalPosition);
						break;
					}
					check_incr += 64;
				}
			}
			else if (bs->currentEnemy->client->ps.weapon == WP_SABER && bs->frame_Enemy_Len >= 75)
			{
				bs->saberBFTime = level.time + Q_irand(700, 1300);
				bs->saberBTime = 0;
			}
		}
	}
	else if (bs->frame_Enemy_Len <= 56)
	{
		bot_behave_attack(bs);
		bs->saberDefending = 0;
	}

	if (!VectorCompare(vec3_origin, move_dir))
	{
		trap->EA_Move(bs->client, move_dir, 5000);
	}

	if (bs->frame_Enemy_Vis && bs->cur_ps.weapon == bs->virtualWeapon
		&& (in_field_of_vision(bs->viewangles, 30, ang)
			|| bs->virtualWeapon == WP_SABER && in_field_of_vision(bs->viewangles, 100, ang)))
	{
		//not switching weapons so attack
		trap->EA_Attack(bs->client);

		if (bs->cur_ps.weapon == WP_SABER)
		{
			//only walk while attacking with the saber.
			bs->doWalk = qtrue;
		}
	}
}

static void Enhanced_saber_combat_handling(bot_state_t* bs)
{
	if (!bs->currentEnemy)
		return;

	// -----------------------------
	// SAFE VECTOR INITIALIZATION
	// -----------------------------
	vec3_t enemyPos = { 0 };
	vec3_t downvec = { 0 };
	vec3_t midorg = { 0 };
	vec3_t a = { 0 };
	vec3_t fwd = { 0 };
	vec3_t ang = { 0 };
	vec3_t move_dir = { 0 };
	vec3_t mins = { -15, -15, -24 };
	vec3_t maxs = { 15,  15,  32 };
	trace_t tr;

	// -----------------------------
	// GET ENEMY POSITION
	// -----------------------------
	if (bs->currentEnemy->client)
		VectorCopy(bs->currentEnemy->client->ps.origin, enemyPos);
	else
		VectorCopy(bs->currentEnemy->s.origin, enemyPos);

	// -----------------------------
	// STRAFE DIRECTION TIMER
	// -----------------------------
	if (bs->meleeStrafeTime < level.time)
	{
		bs->meleeStrafeDir = !bs->meleeStrafeDir;
		bs->meleeStrafeTime = level.time + Q_irand(500, 1800);
	}

	// -----------------------------
	// GROUND CHECKS (SAFE)
	// -----------------------------
	// Enemy ground
	VectorCopy(enemyPos, downvec);
	downvec[2] -= 4096;

	trap->Trace(&tr, enemyPos, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);
	int en_down = tr.endpos[2];

	// Our ground
	VectorCopy(bs->origin, downvec);
	downvec[2] -= 4096;

	trap->Trace(&tr, bs->origin, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);
	int me_down = tr.endpos[2];

	// -----------------------------------------------------
   // IDEAL SPACING FOR SABER DUELS
   // -----------------------------------------------------
	const float idealMin = 80.0f;   // too close
	const float idealMax = 120.0f;  // too far

	if (bs->frame_Enemy_Len < idealMin)
	{
		// Step BACKWARD to maintain spacing
		vec3_t back;
		VectorSubtract(bs->origin, bs->currentEnemy->client->ps.origin, back);

		if (VectorNormalize(back) > 0.001f)
		{
			VectorMA(bs->origin, 64.0f, back, bs->goalPosition);
		}

		// Prevent forward movement this frame
		bs->beStill = level.time + 100;
	}
	else if (bs->frame_Enemy_Len > idealMax)
	{
		// Step FORWARD to close distance
		vec3_t fwd;
		VectorSubtract(bs->currentEnemy->client->ps.origin, bs->origin, fwd);

		if (VectorNormalize(fwd) > 0.001f)
		{
			VectorMA(bs->origin, 64.0f, fwd, bs->goalPosition);
		}
	}
	// -----------------------------
	// MIDPOINT GROUND CHECK
	// -----------------------------
	VectorSubtract(enemyPos, bs->origin, a);
	if (VectorLength(a) < 0.001f)
		VectorSet(a, 1, 0, 0);

	vectoangles(a, a);
	AngleVectors(a, fwd, NULL, NULL);

	VectorMA(bs->origin, bs->frame_Enemy_Len * 0.5f, fwd, midorg);

	VectorCopy(midorg, downvec);
	downvec[2] -= 4096;

	trap->Trace(&tr, midorg, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);
	int mid_down = tr.endpos[2];

	// -----------------------------
	// SAME GROUND LEVEL → NORMAL DUEL LOGIC
	// -----------------------------
	if (me_down == en_down && en_down == mid_down)
	{
		// Move toward enemy if far
		if (bs->frame_Enemy_Len > 128)
		{
			vec3_t enemyOrigin;
			FindOrigin(bs->currentEnemy, enemyOrigin);
			VectorCopy(enemyOrigin, bs->DestPosition);
			bs->DestIgnore = bs->currentEnemy->s.number;
			bot_behave_attack_move(bs);
			return;
		}

		// Toggle defending
		if (bs->saberDefendDecideTime < level.time)
		{
			bs->saberDefending = !bs->saberDefending;
			bs->saberDefendDecideTime = level.time + Q_irand(500, 2000);
		}

		// Too close → hold position
		if (bs->frame_Enemy_Len < 54)
		{
			VectorCopy(bs->origin, bs->goalPosition);
			bs->saberBFTime = 0;
		}

		// -----------------------------
		// SPECIAL MOVE REACTIONS
		// -----------------------------
		if (bs->currentEnemy->client)
		{
			const int emove = bs->currentEnemy->client->ps.saber_move;
			const qboolean enemyInKata =
				PM_SaberInKata(emove) ||
				emove == LS_SPINATTACK ||
				emove == LS_SPINATTACK_GRIEV ||
				emove == LS_SPINATTACK_DUAL;

			// Backoff logic
			if (enemyInKata && bs->frame_Enemy_Len < 80.0f)
			{
				vec3_t vs = { 0 };
				VectorSubtract(bs->origin, bs->goalPosition, vs);

				if (VectorNormalize(vs) < 0.001f)
					VectorSet(vs, 1, 0, 0);

				int ideal_dist = enemyInKata ? 256 : 64;
				int check_incr = 0;
				qboolean found_safe = qfalse;

				while (check_incr < ideal_dist)
				{
					VectorMA(bs->origin, check_incr, vs, bs->goalPosition);

					vec3_t groundcheck;
					VectorCopy(bs->goalPosition, groundcheck);
					groundcheck[2] -= 64;

					trap->Trace(&tr, bs->goalPosition, NULL, NULL, groundcheck,
						bs->client, MASK_SOLID, qfalse, 0, 0);

					if (tr.fraction < 1.0f)
					{
						found_safe = qtrue;
						break;
					}

					check_incr += 64;
				}

				if (!found_safe)
					VectorCopy(enemyPos, bs->goalPosition);
			}
		}
	}
	else if (bs->frame_Enemy_Len <= 56)
	{
		bot_behave_attack(bs);
		bs->saberDefending = 0;
	}

	// -----------------------------
	// MOVEMENT EXECUTION
	// -----------------------------
	if (VectorLength(move_dir) > 0.001f)
		trap->EA_Move(bs->client, move_dir, 5000);

	// -----------------------------
	// ATTACK TRIGGER
	// -----------------------------
	if (bs->frame_Enemy_Vis &&
		bs->cur_ps.weapon == bs->virtualWeapon &&
		(in_field_of_vision(bs->viewangles, 30, ang) ||
			(bs->virtualWeapon == WP_SABER &&
				in_field_of_vision(bs->viewangles, 100, ang))))
	{
		trap->EA_Attack(bs->client);

		if (bs->cur_ps.weapon == WP_SABER)
			bs->doWalk = qtrue;
	}
}

//should we be "leading" our aim with this weapon? And if
//so, by how much?
float bot_weapon_can_lead(const bot_state_t* bs)
{
	switch (bs->cur_ps.weapon)
	{
	case WP_BRYAR_PISTOL:
		return 0.5f;
	case WP_BLASTER:
		return 0.35f;
	case WP_BOWCASTER:
		return 0.5f;
	case WP_REPEATER:
		return 0.45f;
	case WP_THERMAL:
		return 0.5f;
	case WP_DEMP2:
		return 0.35f;
	case WP_ROCKET_LAUNCHER:
		return 0.7f;
	default:
		return 0.0f;
	}
}

//offset the desired view angles with aim leading in mind
void bot_aim_leading(bot_state_t* bs, vec3_t headlevel, const float lead_amount)
{
	vec3_t predicted_spot;
	vec3_t movement_vector;
	vec3_t a, ang;

	if (!bs->currentEnemy ||
		!bs->currentEnemy->client)
	{
		return;
	}

	if (!bs->frame_Enemy_Len)
	{
		return;
	}

	float vtotal = 0;

	if (bs->currentEnemy->client->ps.velocity[0] < 0)
	{
		vtotal += -bs->currentEnemy->client->ps.velocity[0];
	}
	else
	{
		vtotal += bs->currentEnemy->client->ps.velocity[0];
	}

	if (bs->currentEnemy->client->ps.velocity[1] < 0)
	{
		vtotal += -bs->currentEnemy->client->ps.velocity[1];
	}
	else
	{
		vtotal += bs->currentEnemy->client->ps.velocity[1];
	}

	if (bs->currentEnemy->client->ps.velocity[2] < 0)
	{
		vtotal += -bs->currentEnemy->client->ps.velocity[2];
	}
	else
	{
		vtotal += bs->currentEnemy->client->ps.velocity[2];
	}

	VectorCopy(bs->currentEnemy->client->ps.velocity, movement_vector);

	VectorNormalize(movement_vector);

	float x; //hardly calculated with an exact science, but it works

	if (vtotal > 400)
	{
		vtotal = 400;
	}

	if (vtotal)
	{
		x = bs->frame_Enemy_Len * 0.9 * lead_amount * (vtotal * 0.0012);
		//hardly calculated with an exact science, but it works
	}
	else
	{
		x = bs->frame_Enemy_Len * 0.9 * lead_amount; //hardly calculated with an exact science, but it works
	}

	predicted_spot[0] = headlevel[0] + movement_vector[0] * x;
	predicted_spot[1] = headlevel[1] + movement_vector[1] * x;
	predicted_spot[2] = headlevel[2] + movement_vector[2] * x;

	VectorSubtract(predicted_spot, bs->eye, a);
	vectoangles(a, ang);
	VectorCopy(ang, bs->goalAngles);
}

//wobble our aim around based on our sk1llz
static void bot_aim_offset_goal_angles(bot_state_t* bs)
{
	if (bs->skills.perfectaim)
	{
		return;
	}

	if (bs->aimOffsetTime > level.time)
	{
		int i = 0;
		if (bs->aimOffsetAmtYaw)
		{
			bs->goalAngles[YAW] += bs->aimOffsetAmtYaw;
		}

		if (bs->aimOffsetAmtPitch)
		{
			bs->goalAngles[PITCH] += bs->aimOffsetAmtPitch;
		}

		while (i <= 2)
		{
			if (bs->goalAngles[i] > 360)
			{
				bs->goalAngles[i] -= 360;
			}

			if (bs->goalAngles[i] < 0)
			{
				bs->goalAngles[i] += 360;
			}

			i++;
		}
		return;
	}

	float acc_val = bs->skills.accuracy / bs->settings.skill;

	if (bs->currentEnemy && bot_mind_tricked(bs->client, bs->currentEnemy->s.number))
	{
		//having to judge where they are by hearing them, so we should be quite inaccurate here
		acc_val *= 7;

		if (acc_val < 30)
		{
			acc_val = 30;
		}
	}

	if (bs->revengeEnemy && bs->revengeHateLevel &&
		bs->currentEnemy == bs->revengeEnemy)
	{
		//bot becomes more skilled as anger level raises
		acc_val = acc_val / bs->revengeHateLevel;
	}

	if (bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		//assume our goal is aiming at the enemy, seeing as he's visible and all
		if (!bs->currentEnemy->s.pos.trDelta[0] &&
			!bs->currentEnemy->s.pos.trDelta[1] &&
			!bs->currentEnemy->s.pos.trDelta[2])
		{
			acc_val = 0; //he's not even moving, so he shouldn't really be hard to hit.
		}
		else
		{
			acc_val += acc_val * 0.25; //if he's moving he's this much harder to hit
		}

		if (g_entities[bs->client].s.pos.trDelta[0] ||
			g_entities[bs->client].s.pos.trDelta[1] ||
			g_entities[bs->client].s.pos.trDelta[2])
		{
			acc_val += acc_val * 0.15; //make it somewhat harder to aim if we're moving also
		}
	}

	if (acc_val > 90)
	{
		acc_val = 90;
	}
	if (acc_val < 1)
	{
		acc_val = 0;
	}

	if (!acc_val)
	{
		bs->aimOffsetAmtYaw = 0;
		bs->aimOffsetAmtPitch = 0;
		return;
	}

	if (rand() % 10 <= 5)
	{
		bs->aimOffsetAmtYaw = rand() % (int)acc_val;
	}
	else
	{
		bs->aimOffsetAmtYaw = -(rand() % (int)acc_val);
	}

	if (rand() % 10 <= 5)
	{
		bs->aimOffsetAmtPitch = rand() % (int)acc_val;
	}
	else
	{
		bs->aimOffsetAmtPitch = -(rand() % (int)acc_val);
	}

	bs->aimOffsetTime = level.time + rand() % 500 + 200;
}

//do we want to alt fire with this weapon?
static int should_secondary_fire(const bot_state_t* bs)
{
	const int weap = bs->cur_ps.weapon;

	if (bs->cur_ps.ammo[weaponData[weap].ammoIndex] < weaponData[weap].altEnergyPerShot)
	{
		return 0;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT && bs->cur_ps.weapon == WP_ROCKET_LAUNCHER)
	{
		const float held_time = level.time - bs->cur_ps.weaponChargeTime;

		float rTime = bs->cur_ps.rocketLockTime;

		if (rTime < 1)
		{
			rTime = bs->cur_ps.rocketLastValidTime;
		}

		if (held_time > 5000)
		{
			//just give up and release it if we can't manage a lock in 5 seconds
			return 2;
		}

		if (rTime > 0)
		{
			const int dif = (level.time - rTime) / (1200.0f / 16.0f);

			if (dif >= 10)
			{
				return 2;
			}
			if (bs->frame_Enemy_Len > 250)
			{
				return 1;
			}
		}
		else if (bs->frame_Enemy_Len > 250)
		{
			return 1;
		}
	}
	else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT && level.time - bs->cur_ps.weaponChargeTime > bs->
		altChargeTime)
	{
		return 2;
	}
	else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
	{
		return 1;
	}

	if (weap == WP_BRYAR_PISTOL && bs->frame_Enemy_Len < 300)
	{
		return 1;
	}
	if (weap == WP_BOWCASTER && bs->frame_Enemy_Len > 300)
	{
		return 1;
	}
	if (weap == WP_REPEATER && bs->frame_Enemy_Len < 600 && bs->frame_Enemy_Len > 250)
	{
		return 1;
	}
	if (weap == WP_BLASTER && bs->frame_Enemy_Len < 300)
	{
		return 1;
	}
	if (weap == WP_ROCKET_LAUNCHER && bs->frame_Enemy_Len > 250)
	{
		return 1;
	}

	return 0;
}

//standard weapon combat routines
static int combat_bot_ai(bot_state_t* bs)
{
	vec3_t eorg, a;

	if (!bs->currentEnemy)
	{
		return 0;
	}

	if (bs->currentEnemy->client)
	{
		VectorCopy(bs->currentEnemy->client->ps.origin, eorg);
	}
	else
	{
		VectorCopy(bs->currentEnemy->s.origin, eorg);
	}

	VectorSubtract(eorg, bs->eye, a);
	vectoangles(a, a);

	if (bot_get_weapon_range(bs) == BWEAPONRANGE_SABER)
	{
		if (bs->frame_Enemy_Len <= SABER_ATTACK_RANGE)
		{
			bs->doAttack = 1;
		}
	}
	else if (bot_get_weapon_range(bs) == BWEAPONRANGE_SABER)
	{
		if (bs->frame_Enemy_Len <= SABER_KICK_RANGE)
		{
			bs->doBotKick = 1;
		}
	}
	else if (bot_get_weapon_range(bs) == BWEAPONRANGE_MELEE)
	{
		if (bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE)
		{
			bs->doAttack = 1;
		}
	}
	else
	{
		float fovcheck;
		if (bs->cur_ps.weapon == WP_THERMAL || bs->cur_ps.weapon == WP_ROCKET_LAUNCHER)
		{
			//be careful with the hurty weapons
			fovcheck = 40;

			if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT &&
				bs->cur_ps.weapon == WP_ROCKET_LAUNCHER)
			{
				//if we're charging the weapon up then we can hold fire down within a normal fov
				fovcheck = 60;
			}
		}
		else
		{
			fovcheck = 60;
		}

		if (bs->cur_ps.weaponstate == WEAPON_CHARGING ||
			bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
		{
			fovcheck = 160;
		}

		if (bs->frame_Enemy_Len < 128)
		{
			fovcheck *= 2;
		}

		if (in_field_of_vision(bs->viewangles, fovcheck, a))
		{
			if (bs->cur_ps.weapon == WP_THERMAL)
			{
				if (level.time - bs->cur_ps.weaponChargeTime < bs->frame_Enemy_Len * 2 &&
					level.time - bs->cur_ps.weaponChargeTime < 4000 &&
					bs->frame_Enemy_Len > 64 ||
					bs->cur_ps.weaponstate != WEAPON_CHARGING &&
					bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT)
				{
					if (bs->cur_ps.weaponstate != WEAPON_CHARGING && bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT)
					{
						if (bs->frame_Enemy_Len > 512 && bs->frame_Enemy_Len < 800)
						{
							bs->doAltAttack = 1;
						}
						else
						{
							bs->doAttack = 1;
						}
					}

					if (bs->cur_ps.weaponstate == WEAPON_CHARGING)
					{
						bs->doAttack = 1;
					}
					else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
					{
						bs->doAltAttack = 1;
					}
				}
			}
			else
			{
				const int sec_fire = should_secondary_fire(bs);

				if (bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT &&
					bs->cur_ps.weaponstate != WEAPON_CHARGING)
				{
					bs->altChargeTime = Q_irand(500, 1000);
				}

				if (sec_fire == 1)
				{
					bs->doAltAttack = 1;
				}
				else if (!sec_fire)
				{
					if (bs->cur_ps.weapon != WP_THERMAL)
					{
						if (bs->cur_ps.weaponstate != WEAPON_CHARGING ||
							bs->altChargeTime > level.time - bs->cur_ps.weaponChargeTime)
						{
							bs->doAttack = 1;
						}
					}
					else
					{
						bs->doAttack = 1;
					}
				}

				if (sec_fire == 2)
				{
					//released a charge
					return 1;
				}
			}
		}
	}

	return 0;
}
static int gunner_bot_fallback_navigation(bot_state_t* bs)
{
	const int client = bs->cur_ps.clientNum;

	vec3_t b_angle, fwd, trto, mins, maxs;
	trace_t tr;

	// If we see an enemy, just push forward aggressively
	if (bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		trap->EA_MoveForward(client);
		return 0;
	}

	// Collision bounds
	mins[0] = -15; mins[1] = -15; mins[2] = 0;
	maxs[0] = 15; maxs[1] = 15; maxs[2] = 32;

	// Ensure pitch/roll are neutral
	bs->goalAngles[PITCH] = 0;
	bs->goalAngles[ROLL] = 0;

	// If we are still in a "turn lock", keep using the chosen yaw
	if (bs->fallbackTurnTime > level.time)
	{
		bs->goalAngles[YAW] = bs->fallbackTurnYaw;
	}
	else
	{
		// Time to pick a new random direction
		bs->fallbackTurnYaw = rand() % 360;
		bs->fallbackTurnTime = level.time + 600; // commit for 600ms
		bs->goalAngles[YAW] = bs->fallbackTurnYaw;
	}

	// Build forward vector from goalAngles
	VectorCopy(bs->goalAngles, b_angle);
	AngleVectors(b_angle, fwd, NULL, NULL);

	// Trace 16 units ahead
	trto[0] = bs->origin[0] + fwd[0] * 16;
	trto[1] = bs->origin[1] + fwd[1] * 16;
	trto[2] = bs->origin[2] + fwd[2] * 16;

	trap->Trace(&tr, bs->origin, mins, maxs, trto, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1.0f)
	{
		// Path is clear → move forward
		trap->EA_MoveForward(client);
	}
	else
	{
		// Blocked → force immediate new direction next frame
		bs->fallbackTurnTime = 0;
	}

	return 0;
}

static int saber_bot_fallback_navigation(bot_state_t* bs)
{
	vec3_t b_angle, fwd, trto, mins, maxs;
	trace_t tr;
	vec3_t a, ang;

	if (bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		trap->EA_MoveForward(bs->client);
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	bs->goalAngles[PITCH] = 0;
	bs->goalAngles[ROLL] = 0;

	VectorCopy(bs->goalAngles, b_angle);

	AngleVectors(b_angle, fwd, NULL, NULL);

	if (bs->currentEnemy && bs->currentEnemy->health > 0
		&& bs->currentEnemy->client->ps.fallingToDeath == 0
		&& bs->frame_Enemy_Vis
		&& bs->currentEnemy->health > 0)
	{
		// head toward any enimies.
		VectorCopy(bs->currentEnemy->r.currentOrigin, trto);
	}
	else
	{
		// No current enemy. Randomize. Fallback code.
		if (next_point[bs->entityNum] < level.time
			|| VectorDistance(bs->goalPosition, bs->origin) < 64
			|| !(bs->velocity[0] < 32
				&& bs->velocity[1] < 32
				&& bs->velocity[2] < 32
				&& bs->velocity[0] > -32
				&& bs->velocity[1] > -32
				&& bs->velocity[2] > -32))
		{
			// Ready for a new point.
			const int choice = rand() % 4;
			qboolean found = qfalse;

			while (found == qfalse)
			{
				if (choice == 2)
				{
					trto[0] = bs->origin[0] - rand() % 1000;
					trto[1] = bs->origin[1] + rand() % 1000;
				}
				else if (choice == 3)
				{
					trto[0] = bs->origin[0] + rand() % 1000;
					trto[1] = bs->origin[1] - rand() % 1000;
				}
				else if (choice == 4)
				{
					trto[0] = bs->origin[0] - rand() % 1000;
					trto[1] = bs->origin[1] - rand() % 1000;
				}
				else
				{
					trto[0] = bs->origin[0] + rand() % 1000;
					trto[1] = bs->origin[1] + rand() % 1000;
				}

				trto[2] = bs->origin[2];

				if (org_visible(bs->origin, trto, -1))
					found = qtrue;
			}

			next_point[bs->entityNum] = level.time + 2000 + rand() % 5 * 1000;
		}
		else
		{
			// Still moving to the last point.
			trap->EA_MoveForward(bs->client);
		}
	}

	// Set the angles forward for checks.
	VectorSubtract(trto, bs->origin, a);
	vectoangles(a, ang);
	VectorCopy(ang, bs->goalAngles);

	trap->Trace(&tr, bs->origin, mins, maxs, trto, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		// Visible point.
		if (check_fall_by_vectors(bs->origin, ang, &g_entities[bs->entityNum]) == qtrue)
		{
			// We would fall.
			VectorCopy(bs->origin, bs->goalPosition); // Stay still.
		}
		else
		{
			// Try to walk to "trto" if we won't fall.
			VectorCopy(trto, bs->goalPosition); // Original.

			VectorSubtract(bs->goalPosition, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);

			bot_change_view_angles(bs, bs->goalAngles[YAW]);
		}
	}
	else
	{
		int tries = 0;

		while (check_fall_by_vectors(bs->origin, ang, &g_entities[bs->entityNum]) == qtrue && tries <= bot_thinklevel.
			integer)
		{
			// Keep trying until we get something valid? Too much CPU maybe?
			bs->goalAngles[YAW] = rand() % 360; // Original.
			bot_change_view_angles(bs, bs->goalAngles[YAW]);
			tries++;

			bs->goalAngles[PITCH] = 0;
			bs->goalAngles[ROLL] = 0;

			VectorCopy(bs->goalAngles, b_angle);

			AngleVectors(b_angle, fwd, NULL, NULL);

			VectorCopy(b_angle, ang);

			trto[0] = bs->origin[0] + rand() % 4 * 64;
			trto[1] = bs->origin[1] + rand() % 4 * 64;
			trto[2] = bs->origin[2];

			VectorCopy(trto, bs->goalPosition); // Move to new goal.
		}

		if (tries > bot_thinklevel.integer)
		{
			// Ran out of random attempts.
			gunner_bot_fallback_navigation(bs);
		}
	}

	return 1; // Success!
}

static int bot_try_another_weapon(bot_state_t* bs)
{
	int i = 1;

	while (i < WP_NUM_WEAPONS)
	{
		if (bs->cur_ps.ammo[weaponData[i].ammoIndex] >= weaponData[i].energyPerShot &&
			bs->cur_ps.stats[STAT_WEAPONS] & 1 << i)
		{
			bs->virtualWeapon = i;
			bot_select_weapon(bs->client, i);
			return 1;
		}

		i++;
	}

	if (bs->cur_ps.weapon != 1 && bs->virtualWeapon != 1)
	{
		//should always have this.. shouldn't we?
		bs->virtualWeapon = 1;
		bot_select_weapon(bs->client, 1);
		return 1;
	}

	return 0;
}

//is this weapon available to us?
static qboolean bot_weapon_selectable(const bot_state_t* bs, const int weapon)
{
	if (weapon == WP_NONE)
	{
		return qfalse;
	}

	if (bs->cur_ps.ammo[weaponData[weapon].ammoIndex] >= weaponData[weapon].energyPerShot &&
		bs->cur_ps.stats[STAT_WEAPONS] & 1 << weapon)
	{
		return qtrue;
	}

	return qfalse;
}

//select the best weapon we can
static int bot_select_ideal_weapon(bot_state_t* bs)
{
	int bestweight = -1;
	int bestweapon = 0;

	int i = 0;

	while (i < WP_NUM_WEAPONS)
	{
		if (bs->cur_ps.ammo[weaponData[i].ammoIndex] >= weaponData[i].energyPerShot &&
			bs->botWeaponWeights[i] > bestweight &&
			bs->cur_ps.stats[STAT_WEAPONS] & 1 << i)
		{
			if (i == WP_THERMAL)
			{
				//special case..
				if (bs->currentEnemy && bs->frame_Enemy_Len < 700)
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
			else
			{
				bestweight = bs->botWeaponWeights[i];
				bestweapon = i;
			}
		}

		i++;
	}

	if (bs->currentEnemy && bs->frame_Enemy_Len < 300 &&
		(bestweapon == WP_BRYAR_PISTOL || bestweapon == WP_BLASTER || bestweapon == WP_BOWCASTER) &&
		bs->cur_ps.stats[STAT_WEAPONS] & 1 << WP_SABER)
	{
		bestweapon = WP_SABER;
		bestweight = 1;
	}

	if (bs->currentEnemy && bs->frame_Enemy_Len > 300 &&
		bs->currentEnemy->client && bs->currentEnemy->client->ps.weapon != WP_SABER &&
		bestweapon == WP_SABER)
	{
		//if the enemy is far away, and we have our saber selected, see if we have any good distance weapons instead
		if (bot_weapon_selectable(bs, WP_DISRUPTOR))
		{
			bestweapon = WP_DISRUPTOR;
			bestweight = 1;
		}
		else if (bot_weapon_selectable(bs, WP_ROCKET_LAUNCHER))
		{
			bestweapon = WP_ROCKET_LAUNCHER;
			bestweight = 1;
		}
		else if (bot_weapon_selectable(bs, WP_BOWCASTER))
		{
			bestweapon = WP_BOWCASTER;
			bestweight = 1;
		}
		else if (bot_weapon_selectable(bs, WP_BLASTER))
		{
			bestweapon = WP_BLASTER;
			bestweight = 1;
		}
		else if (bot_weapon_selectable(bs, WP_REPEATER))
		{
			bestweapon = WP_REPEATER;
			bestweight = 1;
		}
		else if (bot_weapon_selectable(bs, WP_DEMP2))
		{
			bestweapon = WP_DEMP2;
			bestweight = 1;
		}
	}

	if (bestweight != -1 && bs->cur_ps.weapon != bestweapon && bs->virtualWeapon != bestweapon)
	{
		bs->virtualWeapon = bestweapon;
		bot_select_weapon(bs->client, bestweapon);
		//bs->cur_ps.weapon = bestweapon;
		//level.clients[bs->client].ps.weapon = bestweapon;
		return 1;
	}

	return 0;
}

//override our standard weapon choice with a melee weapon
static int bot_select_melee(bot_state_t* bs)
{
	if (bs->cur_ps.weapon != 1 && bs->virtualWeapon != 1)
	{
		bs->virtualWeapon = 1;
		bot_select_weapon(bs->client, 1);
		return 1;
	}

	return 0;
}

//See if we our in love with the potential bot.
static int get_love_level(const bot_state_t* bs, const bot_state_t* love)
{
	int i = 0;

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		//There is no love in 1-on-1
		return 0;
	}

	if (!bs || !love || !g_entities[love->client].client)
	{
		return 0;
	}

	if (!bs->lovednum)
	{
		return 0;
	}

	if (!bot_attachments.integer)
	{
		return 1;
	}

	const char* lname = g_entities[love->client].client->pers.netname;

	if (!lname)
	{
		return 0;
	}

	while (i < bs->lovednum)
	{
		if (strcmp(bs->loved[i].name, lname) == 0)
		{
			return bs->loved[i].level;
		}

		i++;
	}

	return 0;
}

//Our loved one was killed. We must become infuriated!
static void bot_loved_one_died(bot_state_t* bs, const bot_state_t* loved, const int lovelevel)
{
	if (!loved->lastHurt || !loved->lastHurt->client ||
		loved->lastHurt->s.number == loved->client)
	{
		return;
	}

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		//There is no love in 1-on-1
		return;
	}

	if (!is_teamplay())
	{
		if (lovelevel < 2)
		{
			return;
		}
	}
	else if (OnSameTeam(&g_entities[bs->client], loved->lastHurt))
	{
		//don't hate teammates no matter what
		return;
	}

	if (loved->client == loved->lastHurt->s.number)
	{
		return;
	}

	if (bs->client == loved->lastHurt->s.number)
	{
		//oops!
		return;
	}

	if (!bot_attachments.integer)
	{
		return;
	}

	if (!pass_loved_one_check(bs, loved->lastHurt))
	{
		//a loved one killed a loved one.. you cannot hate them
		bs->chatObject = loved->lastHurt;
		bs->chatAltObject = &g_entities[loved->client];
		BotDoChat(bs, "LovedOneKilledLovedOne", 0);
		return;
	}

	if (bs->revengeEnemy == loved->lastHurt)
	{
		if (bs->revengeHateLevel < bs->loved_death_thresh)
		{
			bs->revengeHateLevel++;

			if (bs->revengeHateLevel == bs->loved_death_thresh)
			{
				//broke into the highest anger level
				//CHAT: Hatred section
				bs->chatObject = loved->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Hatred", 1);
			}
		}
	}
	else if (bs->revengeHateLevel < bs->loved_death_thresh - 1)
	{
		//only switch hatred if we don't hate the existing revenge-enemy too much
		//CHAT: BelovedKilled section
		bs->chatObject = &g_entities[loved->client];
		bs->chatAltObject = loved->lastHurt;
		BotDoChat(bs, "BelovedKilled", 0);
		bs->revengeHateLevel = 0;
		bs->revengeEnemy = loved->lastHurt;
	}
}

static void bot_death_notify(const bot_state_t* bs)
{
	//in case someone has an emotional attachment to us, we'll notify them
	int i = 0;

	while (i < MAX_CLIENTS)
	{
		if (botstates[i] && botstates[i]->lovednum)
		{
			int ltest = 0;
			while (ltest < botstates[i]->lovednum)
			{
				if (strcmp(level.clients[bs->client].pers.netname, botstates[i]->loved[ltest].name) == 0)
				{
					bot_loved_one_died(botstates[i], bs, botstates[i]->loved[ltest].level);
					break;
				}

				ltest++;
			}
		}

		i++;
	}
}

//perform strafe trace checks
static void strafe_tracing(bot_state_t* bs)
{
	vec3_t mins, maxs;
	vec3_t right, rorg, drorg;
	trace_t tr;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -22;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	AngleVectors(bs->viewangles, NULL, right, NULL);

	if (bs->meleeStrafeDir)
	{
		rorg[0] = bs->origin[0] - right[0] * 32;
		rorg[1] = bs->origin[1] - right[1] * 32;
		rorg[2] = bs->origin[2] - right[2] * 32;
	}
	else
	{
		rorg[0] = bs->origin[0] + right[0] * 32;
		rorg[1] = bs->origin[1] + right[1] * 32;
		rorg[2] = bs->origin[2] + right[2] * 32;
	}

	trap->Trace(&tr, bs->origin, mins, maxs, rorg, bs->client, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction != 1)
	{
		bs->meleeStrafeDisable = level.time + Q_irand(500, 1500);
	}

	VectorCopy(rorg, drorg);

	drorg[2] -= 32;

	trap->Trace(&tr, rorg, NULL, NULL, drorg, bs->client, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		//this may be a dangerous ledge, so don't strafe over it just in case
		bs->meleeStrafeDisable = level.time + Q_irand(500, 1500);
	}
}

//doing primary weapon fire
static int prim_firing(const bot_state_t* bs)
{
	if (bs->cur_ps.weaponstate != WEAPON_CHARGING &&
		bs->doAttack)
	{
		return 1;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING &&
		!bs->doAttack)
	{
		return 1;
	}

	return 0;
}

//should we keep our primary weapon from firing?
static int keep_prim_from_firing(bot_state_t* bs)
{
	if (bs->cur_ps.weaponstate != WEAPON_CHARGING &&
		bs->doAttack)
	{
		bs->doAttack = 0;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING &&
		!bs->doAttack)
	{
		bs->doAttack = 1;
	}

	return 0;
}

//doing secondary weapon fire
static int alt_firing(const bot_state_t* bs)
{
	if (bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT &&
		bs->doAltAttack)
	{
		return 1;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT &&
		!bs->doAltAttack)
	{
		return 1;
	}

	return 0;
}

//should we keep our alt from firing?
static int keep_alt_from_firing(bot_state_t* bs)
{
	if (bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT &&
		bs->doAltAttack)
	{
		bs->doAltAttack = 0;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT &&
		!bs->doAltAttack)
	{
		bs->doAltAttack = 1;
	}

	return 0;
}

//Try not to shoot our friends in the back. Or in the face. Or anywhere, really.
static gentity_t* check_for_friend_in_lof(const bot_state_t* bs)
{
	vec3_t fwd;
	vec3_t trfrom, trto;
	vec3_t mins, maxs;
	trace_t tr;

	mins[0] = -3;
	mins[1] = -3;
	mins[2] = -3;

	maxs[0] = 3;
	maxs[1] = 3;
	maxs[2] = 3;

	AngleVectors(bs->viewangles, fwd, NULL, NULL);

	VectorCopy(bs->eye, trfrom);

	trto[0] = trfrom[0] + fwd[0] * 2048;
	trto[1] = trfrom[1] + fwd[1] * 2048;
	trto[2] = trfrom[2] + fwd[2] * 2048;

	trap->Trace(&tr, trfrom, mins, maxs, trto, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction != 1 && tr.entityNum <= MAX_CLIENTS)
	{
		gentity_t* trent = &g_entities[tr.entityNum];

		if (trent && trent->client)
		{
			if (is_teamplay() && OnSameTeam(&g_entities[bs->client], trent))
			{
				return trent;
			}

			if (botstates[trent->s.number] && get_love_level(bs, botstates[trent->s.number]) > 1)
			{
				return trent;
			}
		}
	}

	return NULL;
}

static void bot_scan_for_leader(bot_state_t* bs)
{
	//bots will only automatically obtain a leader if it's another bot using this method.
	int i = 0;

	if (bs->isSquadLeader)
	{
		return;
	}

	while (i < MAX_CLIENTS)
	{
		gentity_t* ent = &g_entities[i];

		if (ent && ent->client && botstates[i] && botstates[i]->isSquadLeader && bs->client != i)
		{
			if (OnSameTeam(&g_entities[bs->client], ent))
			{
				bs->squadLeader = ent;
				break;
			}
			if (get_love_level(bs, botstates[i]) > 1 && !is_teamplay())
			{
				//ignore love status regarding squad leaders if we're in teamplay
				bs->squadLeader = ent;
				break;
			}
		}

		i++;
	}
}

//w3rd to the p33pz.
static void bot_reply_greetings(const bot_state_t* bs)
{
	int i = 0;
	int numhello = 0;

	while (i < MAX_CLIENTS)
	{
		if (botstates[i] &&
			botstates[i]->canChat &&
			i != bs->client)
		{
			botstates[i]->chatObject = &g_entities[bs->client];
			botstates[i]->chatAltObject = NULL;
			if (BotDoChat(botstates[i], "ResponseGreetings", 0))
			{
				numhello++;
			}
		}

		if (numhello > 3)
		{
			//don't let more than 4 bots say hello at once
			return;
		}

		i++;
	}
}

//try to move in to grab a nearby flag
static void ctf_flag_movement(bot_state_t* bs)
{
	const gentity_t* desired_drop = NULL;
	vec3_t a, mins, maxs;
	trace_t tr;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -7;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 7;

	if (bs->wantFlag && bs->wantFlag->flags & FL_DROPPED_ITEM)
	{
		if (bs->staticFlagSpot[0] == bs->wantFlag->s.pos.trBase[0] &&
			bs->staticFlagSpot[1] == bs->wantFlag->s.pos.trBase[1] &&
			bs->staticFlagSpot[2] == bs->wantFlag->s.pos.trBase[2])
		{
			VectorSubtract(bs->origin, bs->wantFlag->s.pos.trBase, a);

			if (VectorLength(a) <= BOT_FLAG_GET_DISTANCE)
			{
				VectorCopy(bs->wantFlag->s.pos.trBase, bs->goalPosition);
				return;
			}
			bs->wantFlag = NULL;
		}
		else
		{
			bs->wantFlag = NULL;
		}
	}
	else if (bs->wantFlag)
	{
		bs->wantFlag = NULL;
	}

	if (flagRed && flagBlue)
	{
		if (bs->wpDestination == flagRed ||
			bs->wpDestination == flagBlue)
		{
			int diddrop = 0;
			if (bs->wpDestination == flagRed && droppedRedFlag && droppedRedFlag->flags & FL_DROPPED_ITEM &&
				droppedRedFlag->classname && strcmp(droppedRedFlag->classname, "freed") != 0)
			{
				desired_drop = droppedRedFlag;
				diddrop = 1;
			}
			if (bs->wpDestination == flagBlue && droppedBlueFlag && droppedBlueFlag->flags & FL_DROPPED_ITEM &&
				droppedBlueFlag->classname && strcmp(droppedBlueFlag->classname, "freed") != 0)
			{
				desired_drop = droppedBlueFlag;
				diddrop = 1;
			}

			if (diddrop && desired_drop)
			{
				VectorSubtract(bs->origin, desired_drop->s.pos.trBase, a);

				if (VectorLength(a) <= BOT_FLAG_GET_DISTANCE)
				{
					trap->Trace(&tr, bs->origin, mins, maxs, desired_drop->s.pos.trBase, bs->client, MASK_SOLID, qfalse,
						0, 0);

					if (tr.fraction == 1 || tr.entityNum == desired_drop->s.number)
					{
						VectorCopy(desired_drop->s.pos.trBase, bs->goalPosition);
						VectorCopy(desired_drop->s.pos.trBase, bs->staticFlagSpot);
					}
				}
			}
		}
	}
}

//see if we want to make our detpacks blow up
static void bot_check_det_packs(bot_state_t* bs)
{
	gentity_t* dp = NULL;
	gentity_t* my_det = NULL;
	vec3_t a;

	while ((dp = G_Find(dp, FOFS(classname), "detpack")) != NULL)
	{
		if (dp && dp->parent && dp->parent->s.number == bs->client)
		{
			my_det = dp;
			break;
		}
	}

	if (!my_det)
	{
		return;
	}

	if (!bs->currentEnemy || !bs->currentEnemy->client || !bs->frame_Enemy_Vis)
	{
		//require the enemy to be visilbe just to be fair..
		//unless..
		if (bs->currentEnemy && bs->currentEnemy->client &&
			level.time - bs->plantContinue < 5000)
		{
			//it's a fresh plant (within 5 seconds) so we should be able to guess
			goto stillmadeit;
		}
		return;
	}

stillmadeit:

	VectorSubtract(bs->currentEnemy->client->ps.origin, my_det->s.pos.trBase, a);
	const float en_len = VectorLength(a);

	VectorSubtract(bs->origin, my_det->s.pos.trBase, a);
	const float my_len = VectorLength(a);

	if (en_len > my_len)
	{
		return;
	}

	if (en_len < BOT_PLANT_BLOW_DISTANCE && org_visible(bs->currentEnemy->client->ps.origin, my_det->s.pos.trBase,
		bs->currentEnemy->s.number))
	{
		//we could just call the "blow all my detpacks" function here, but I guess that's cheating.
		bs->plantKillEmAll = level.time + 500;
	}
}

/*
==================
BotBuyItem
==================
*/
static qboolean bot_buy_item(bot_state_t* bs, const char* msg)
{
	int j;

	if (!bs)
	{
		return qfalse;
	}

	if (!g_AllowBotBuyItem.integer)
	{
		return qtrue;
	}

	if (level.time < bs->nextPurchase)
	{
		return qfalse;
	}

	bs->nextPurchase = level.time + 10000; //ten seconds

	for (int i = 0; bg_buylist[i].text; i++)
	{
		// allow partial match
		if (Q_stricmpn(msg, bg_buylist[i].text, strlen(msg)) == 0)
		{
			if (bs->cur_ps.persistant[PERS_SCORE] >= bg_buylist[i].price)
			{
				int tag = bg_buylist[i].giTag;
				const int type = bg_buylist[i].giType;
				int quantity = bg_buylist[i].quantity;
				int cost = bg_buylist[i].price;

				switch (type)
				{
				case IT_WEAPON:

					if (tag <= WP_NONE || tag >= WP_NUM_WEAPONS)
					{
						return qtrue; // invalid weapon
					}

					if (bs->cur_ps.stats[STAT_WEAPONS] & 1 << tag)
					{
						return qtrue; // already holding weapon
					}
					bs->cur_ps.stats[STAT_WEAPONS] |= 1 << tag;
					tag = weaponData[tag].ammoIndex;
				case IT_AMMO:
					// buy ammo for the primary gun
					if (tag == AMMO_NONE)
					{
						const int weapon = bs->cur_ps.weapon;

						if (weapon > WP_NONE && weapon < WP_NUM_WEAPONS)
						{
							const int ammo = weaponData[weapon].ammoIndex;

							if (ammo > AMMO_NONE && ammo < AMMO_MAX)
							{
								for (int i1 = 0; bg_buylist[i1].text; i1++)
								{
									if (bg_buylist[i1].giType == IT_AMMO && bg_buylist[i1].giTag == ammo)
									{
										tag = ammo;
										cost = bg_buylist[i1].giType;
										quantity = bg_buylist[i1].quantity;
									}
								}
							}
							else
							{
								return qfalse; //  weapon doesn't use ammo
							}
						}
						else
						{
							return qfalse; // invalid weapon
						}
					}

					if (tag <= AMMO_NONE || tag >= AMMO_MAX)
					{
						return qfalse; // invald ammo type
					}

					if (bs->cur_ps.ammo[tag] < ammoData[tag].max)
					{
						bs->cur_ps.ammo[tag] += quantity;
						if (bs->cur_ps.ammo[tag] > ammoData[tag].max)
						{
							bs->cur_ps.ammo[tag] = ammoData[tag].max;
						}
					}
					else
					{
						return qfalse; // ammo full
					}

					break;
				case IT_ARMOR:
				{
					int value = bs->cur_ps.stats[STAT_ARMOR];
					const int max = 100 + 25 * tag;

					if (value + 2 >= max)
					{
						return qfalse;
					}

					value += quantity;

					if (value > max)
					{
						value = max;
					}
					bs->cur_ps.stats[STAT_ARMOR] = value;
				}
				break;
				case IT_HEALTH:
				{
					int value = bs->cur_ps.stats[STAT_MAX_HEALTH];
					const int max = bs->cur_ps.stats[STAT_MAX_HEALTH] + 25 * tag;

					if (value + 2 >= max)
					{
						return qfalse;
					}

					value += quantity;

					if (value > max)
					{
						value = max;
					}
					bs->cur_ps.stats[STAT_HEALTH] = g_entities[bs->client].health = value;
				}
				break;
				case IT_POWERUP:
					break;
				case IT_HOLDABLE:
				{
					if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << tag)
					{
						return qfalse; // already have
					}
					bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(tag, IT_HOLDABLE);
					bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << tag;
				}
				break;
				case IT_PERSISTANT_POWERUP:
					break;
				default:
					return qfalse;
				}

				if (cost > 0)
				{
					bs->cur_ps.persistant[PERS_SCORE] -= cost;
				}

				// one frigged-up trick to fake an item pickup
				for (j = 0; j < bg_numItems; j++)
				{
					if (bg_itemlist[j].giType != bg_buylist[i].giType)
					{
						continue;
					}

					switch (bg_buylist[i].giType)
					{
					case IT_WEAPON:
					case IT_AMMO:
					case IT_POWERUP:
					case IT_HOLDABLE:
					case IT_PERSISTANT_POWERUP:
					case IT_TEAM:
						if (bg_itemlist[j].giTag != bg_buylist[i].giTag)
						{
							continue;
						}
						break;
					case IT_HEALTH:
					case IT_ARMOR:
						break;
					default:
						continue;
					}
					break;
				}

				if (j > 0 && j < bg_numItems)
				{
					gentity_t* te = G_TempEntity(g_entities[bs->client].s.pos.trBase, EV_ITEM_PICKUP);
					te->s.eventParm = te - g_entities;
					te->s.modelIndex = j;
					te->s.number = bs->client;
				}
				return qtrue;
			}
			bs->nextPurchase = level.time + 2000;
			// we can make another purchase sooner in case we tried to buy something expensive
			return qfalse; // not enough cash
		}
	}

	return qfalse; // couldn't find item
}

//see if it would be beneficial at this time to use one of our inv items
static int bot_use_inventory_item(bot_state_t* bs)
{
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_MEDPAC)
	{
		if (g_entities[bs->client].health <= 75)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_MEDPAC, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_MEDPAC_BIG)
	{
		if (g_entities[bs->client].health <= 50)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_MEDPAC_BIG, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_SEEKER)
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SEEKER, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_SPHERESHIELD)
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SPHERESHIELD, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_SENTRY_GUN)
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SENTRY_GUN, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_SHIELD)
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis && bs->runningToEscapeThreat)
		{
			//this will (hopefully) result in the bot placing the shield down while facing
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SHIELD, IT_HOLDABLE);
			goto wantuseitem;
		}
	}

	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK && bs->cur_ps.jetpackFuel > 30)
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis && bs->runningToEscapeThreat)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->iHaveNoIdeaWhereIAmGoing)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (g_entities[bs->client].health <= 75)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	else if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_FLAMETHROWER)
	{
		if (bs->currentEnemy && bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_FLAMETHROWER, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->currentEnemy && bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE && bs->cur_ps.groundEntityNum ==
			ENTITYNUM_NONE)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(IT_HOLDABLE, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->currentEnemy && bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE && g_entities[bs->client].health <= 50)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(IT_HOLDABLE, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_CLOAK
		&& !PM_SaberInAttack(bs->cur_ps.saber_move)
		&& !bs->cur_ps.powerups[PW_CLOAKED]
		&& !(bs->cur_ps.communicatingflags & 1 << CLOAK_CHARGE_RESTRICTION))
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis && bs->frame_Enemy_Len <= 1000)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_CLOAK, IT_HOLDABLE);
			goto wantuseitem;
		}
	}

	return 0;

wantuseitem:
	level.clients[bs->client].ps.stats[STAT_HOLDABLE_ITEM] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM];

	return 1;
}

//trace forward to see if we can plant a detpack or something
static int bot_surface_near(const bot_state_t* bs)
{
	trace_t tr;
	vec3_t fwd;

	AngleVectors(bs->viewangles, fwd, NULL, NULL);

	fwd[0] = bs->origin[0] + fwd[0] * 64;
	fwd[1] = bs->origin[1] + fwd[1] * 64;
	fwd[2] = bs->origin[2] + fwd[2] * 64;

	trap->Trace(&tr, bs->origin, NULL, NULL, fwd, bs->client, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction != 1)
	{
		return 1;
	}

	return 0;
}

//could we block projectiles from the weapon potentially with a light saber?
static int bot_weapon_blockable(const int weapon)
{
	switch (weapon)
	{
	case WP_STUN_BATON:
	case WP_MELEE:
		return 0;
	case WP_DISRUPTOR:
		return 0;
	case WP_DEMP2:
		return 0;
	case WP_ROCKET_LAUNCHER:
		return 0;
	case WP_THERMAL:
		return 0;
	case WP_TRIP_MINE:
		return 0;
	case WP_DET_PACK:
		return 0;
	default:
		return 1;
	}
}

void Cmd_EngageDuel_f(gentity_t* ent);
void Cmd_ToggleSaber_f(gentity_t* ent);

//movement overrides
void bot_set_forced_movement(const int bot, const int forward, const int right, const int up)
{
	bot_state_t* bs = botstates[bot];

	if (!bs)
	{
		//not a bot
		return;
	}

	if (forward != -1)
	{
		if (bs->forceMove_Forward)
		{
			bs->forceMove_Forward = 0;
		}
		else
		{
			bs->forceMove_Forward = forward;
		}
	}
	if (right != -1)
	{
		if (bs->forceMove_Right)
		{
			bs->forceMove_Right = 0;
		}
		else
		{
			bs->forceMove_Right = right;
		}
	}
	if (up != -1)
	{
		if (bs->forceMove_Up)
		{
			bs->forceMove_Up = 0;
		}
		else
		{
			bs->forceMove_Up = up;
		}
	}
}

//check to see if the bot should switch player
qboolean switch_siege_ideal_class(const bot_state_t* bs, const char* idealclass)
{
	char cmp[MAX_STRING_CHARS] = { 0 };
	int i = 0;

	while (idealclass[i])
	{
		int j = 0;
		while (idealclass[i] && idealclass[i] != '|')
		{
			cmp[j] = idealclass[i];
			i++;
			j++;
		}
		cmp[j] = 0;

		if (numberof_siege_specific_class(g_entities[bs->client].client->sess.siegeDesiredTeam, cmp))
		{
			//found a player that can hack this trigger
			return qfalse;
		}

		if (idealclass[i] != '|')
		{
			//reached the end, so, switch to a unit that can hack this trigger.
			siegeClass_t* hold_class = BG_SiegeFindClassByName(cmp);
			if (hold_class)
			{
				trap->EA_Command(bs->client, va("siegeclass \"%s\"\n", hold_class->name));
				return qtrue;
			}
			return qfalse;
		}
		i++;
	}

	return qfalse;
}

//find a numberof a specific class on a team.
int numberof_siege_specific_class(const int team, const char* classname)
{
	int num_players = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		const gentity_t* ent = &g_entities[i];
		if (ent && ent->client && ent->client->pers.connected == CON_CONNECTED
			&& ent->client->sess.siegeClass && ent->client->sess.siegeDesiredTeam == team)
		{
			if (strcmp(ent->client->sess.siegeClass, classname) == 0)
			{
				num_players++;
			}
		}
	}
	return num_players;
}

//has the bot select the siege class with the fewest number of players on this team.
static void select_best_siege_class(const int clientNum, const qboolean force_join)
{
	int best_num = MAX_CLIENTS;
	int best_base_class = -1;

	if (clientNum < 0 || clientNum > MAX_CLIENTS)
	{
		//bad clientNum
		return;
	}

	for (int i = 0; i < SPC_MAX; i++)
	{
		const int x = numberof_siege_basic_classes(g_entities[clientNum].client->sess.siegeDesiredTeam, i);
		if (x < best_num)
		{
			//this class has fewer players.
			best_num = x;
			best_base_class = i;
		}
	}

	if (best_base_class != -1)
	{
		//found one, join that class
		siegeClass_t* hold_class = BG_GetClassOnBaseClass(g_entities[clientNum].client->sess.siegeDesiredTeam,
			best_base_class, 0);
		if (Q_stricmp(g_entities[clientNum].client->sess.siegeClass, hold_class->name)
			|| force_join)
		{
			//we're not already this class.
			trap->EA_Command(clientNum, va("siegeclass \"%s\"\n", hold_class->name));
		}
	}
}

static qboolean bot_should_jump_to_enemy(bot_state_t* bs, float xy, qboolean will_fall)
{
	const gentity_t* enemy = bs->currentEnemy;

	if (!enemy || !enemy->client || enemy->health <= 0)
		return qfalse;

	// Never jump if saber-walking
	if (bs->cur_ps.weapon == WP_SABER && bs->doWalk)
		return qfalse;

	// Never jump if it risks falling
	if (will_fall)
		return qfalse;

	// Never jump if already jumping
	if (bs->BOTjumpState > JS_WAITING)
		return qfalse;

	// Jetpack bots prefer flight, not jumps
	const qboolean hasJetpack =
		(bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK));

	const float dz = enemy->r.currentOrigin[2] - bs->origin[2];

	// Enemy above us
	if (dz > 32 && xy < 300)
	{
		if (hasJetpack)
			return qfalse; // jetpack will handle vertical chase
		return qtrue;
	}

	// Enemy below us
	if (dz < -32 && xy > 1000)
	{
		if (hasJetpack)
			return qfalse; // jetpack handles descent
		return qtrue;
	}

	return qfalse;
}

extern saberInfo_t* BG_MySaber(int clientNum, int saberNum);
void bot_check_speak(gentity_t* self, const qboolean moving);
extern void AngleClamp(vec3_t ang);
//the main AI loop.
//please don't be too frightened.
void standard_bot_ai(bot_state_t* bs)
{
	const int saberNum = 0;
	int doing_fallback = 0;
	int fj_halt;
	vec3_t a;
	vec3_t ang;
	vec3_t a_fo;
	float reaction;
	int meleestrafe = 0;
	int use_the_force = 0;
	int forceHostile = 0;
	gentity_t* friend_in_lof = 0;
	vec3_t pre_frame_g_angles;
	vec3_t move_dir;
	const saberInfo_t* saber1 = BG_MySaber(bs->client, 0);
	const saberInfo_t* saber2 = BG_MySaber(bs->client, 1);

	//Reset the action states
	bs->doAttack = qfalse;
	bs->doAltAttack = qfalse;
	bs->doSaberThrow = qfalse;
	bs->doBotKick = qfalse;
	bs->doWalk = qfalse;
	bs->virtualWeapon = bs->cur_ps.weapon;

	if (gDeactivated || g_entities[bs->client].client->tempSpectate > level.time)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		clear_route(bs->botRoute);
		VectorClear(bs->lastDestPosition);
		bs->wpSpecial = qfalse;

		//reset tactical stuff
		bs->tacticEntity = NULL;
		bs->objectiveType = 0;
		bs->MiscBotFlags = 0;
		return;
	}

	if (g_entities[bs->client].inuse &&
		g_entities[bs->client].client &&
		g_entities[bs->client].client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		return;
	}

#ifndef FINAL_BUILD
	if (bot_getinthecarrr.integer)
	{ //stupid vehicle debug, I tire of having to connect another client to test passengers.
		gentity_t* botEnt = &g_entities[bs->client];

		if (botEnt->inuse && botEnt->client && botEnt->client->ps.m_iVehicleNum)
		{ //in a vehicle, so...
			bs->noUseTime = level.time + 5000;

			if (bot_getinthecarrr.integer != 2)
			{
				trap->EA_MoveForward(bs->client);

				if (bot_getinthecarrr.integer == 3)
				{ //use alt fire
					trap->EA_Alt_Attack(bs->client);
				}
			}
		}
		else
		{ //find one, get in
			int i = 0;
			gentity_t* vehicle = NULL;
			//find the nearest, manned vehicle
			while (i < MAX_GENTITIES)
			{
				vehicle = &g_entities[i];

				if (vehicle->inuse && vehicle->client && vehicle->s.eType == ET_NPC &&
					vehicle->s.NPC_class == CLASS_VEHICLE && vehicle->m_pVehicle &&
					(vehicle->client->ps.m_iVehicleNum || bot_getinthecarrr.integer == 2))
				{ //ok, this is a vehicle, and it has a pilot/passengers
					break;
				}
				i++;
			}
			if (i != MAX_GENTITIES && vehicle)
			{ //broke before end so we must've found something
				vec3_t v;

				VectorSubtract(vehicle->client->ps.origin, bs->origin, v);
				VectorNormalize(v);
				vectoangles(v, bs->goalAngles);
				MoveTowardIdealAngles(bs);
				trap->EA_Move(bs->client, v, 5000.0f);

				if (bs->noUseTime < (level.time - 400))
				{
					bs->noUseTime = level.time + 500;
				}
			}
		}

		return;
	}
#endif

	if (bot_forgimmick.integer)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;

		if (bot_forgimmick.integer == 2)
		{
			//for debugging saber stuff, this is handy
			trap->EA_Attack(bs->client);
		}

		if (bot_forgimmick.integer == 3)
		{
			//for testing cpu usage moving around rmg terrain without AI
			vec3_t mdir;

			VectorSubtract(bs->origin, vec3_origin, mdir);
			VectorNormalize(mdir);
			trap->EA_Attack(bs->client);
			trap->EA_Move(bs->client, mdir, 5000);
		}

		if (bot_forgimmick.integer == 4)
		{
			//constantly move toward client 0
			if (g_entities[0].client && g_entities[0].inuse)
			{
				vec3_t mdir;

				VectorSubtract(g_entities[0].client->ps.origin, bs->origin, mdir);
				VectorNormalize(mdir);
				trap->EA_Move(bs->client, mdir, 5000);
			}
		}

		if (bs->forceMove_Forward)
		{
			if (bs->forceMove_Forward > 0)
			{
				trap->EA_MoveForward(bs->client);
			}
			else
			{
				trap->EA_MoveBack(bs->client);
			}
		}
		if (bs->forceMove_Right)
		{
			if (bs->forceMove_Right > 0)
			{
				trap->EA_MoveRight(bs->client);
			}
			else
			{
				trap->EA_MoveLeft(bs->client);
			}
		}
		if (bs->forceMove_Up)
		{
			trap->EA_Jump(bs->client);
		}
		return;
	}

	if (level.gametype == GT_SIEGE && level.time - level.startTime < 10000)
	{
		//make sure that the bots aren't all on the same team after map changes.
		select_best_siege_class(bs->client, qfalse);
	}

	if (bs->cur_ps.pm_type == PM_INTERMISSION
		|| g_entities[bs->client].client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		//in intermission
		//Mash the button to prevent the game from sticking on one level.
		if (level.gametype == GT_SIEGE)
		{
			//hack to get the bots to spawn into seige games after the game has started
			if (g_entities[bs->client].client->sess.siegeDesiredTeam != SIEGETEAM_TEAM1
				&& g_entities[bs->client].client->sess.siegeDesiredTeam != SIEGETEAM_TEAM2)
			{
				//we're not on a team, go onto the best team available.
				g_entities[bs->client].client->sess.siegeDesiredTeam = PickTeam(bs->client);
			}

			select_best_siege_class(bs->client, qtrue);
		}

		if (!(g_entities[bs->client].client->pers.cmd.buttons & BUTTON_ATTACK))
		{
			//only tap the button if it's not currently being pressed
			trap->EA_Attack(bs->client);
		}
		return;
	}

	if (!bs->lastDeadTime)
	{
		//just spawned in?
		bs->lastDeadTime = level.time;
		bs->MiscBotFlags = 0;
		bs->orderEntity = NULL;
		bs->ordererNum = bs->client;
		VectorClear(bs->DestPosition);
		bs->DestIgnore = -1;
	}

	if (g_entities[bs->client].health < 1 || g_entities[bs->client].client->ps.pm_type == PM_DEAD)
	{
		bs->lastDeadTime = level.time;

		if (!bs->deathActivitiesDone && bs->lastHurt && bs->lastHurt->client && bs->lastHurt->s.number != bs->client)
		{
			bot_death_notify(bs);
			if (pass_loved_one_check(bs, bs->lastHurt))
			{
				//CHAT: Died
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Died", 0);
			}
			else if (!pass_loved_one_check(bs, bs->lastHurt) &&
				botstates[bs->lastHurt->s.number] &&
				pass_loved_one_check(botstates[bs->lastHurt->s.number], &g_entities[bs->client]))
			{
				//killed by a bot that I love, but that does not love me
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "KilledOnPurposeByLove", 0);
			}

			bs->deathActivitiesDone = 1;
		}

		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpCamping = NULL;
		bs->wpCampingTo = NULL;
		bs->wpStoreDest = NULL;
		bs->wpDestIgnoreTime = 0;
		bs->wpDestSwitchTime = 0;
		bs->wpSeenTime = 0;
		bs->wpDirection = 0;
		VectorClear(bs->lastDestPosition);
		clear_route(bs->botRoute);
		bs->tacticEntity = NULL;
		bs->objectiveType = 0;
		bs->MiscBotFlags = 0;

		if (rand() % 10 < 5 &&
			(!bs->doChat || bs->chatTime < level.time))
		{
			trap->EA_Attack(bs->client);
			if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_STAFF
				&& g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_DUAL)
			{
				Cmd_SaberAttackCycle_f(&g_entities[bs->client]); // we died lets change the saber style
			}
		}

		return;
	}

	qboolean dualSabers = qfalse;
	qboolean staffSaber = qfalse;

	if (saber2 && saber2->model[0])
	{
		dualSabers = qtrue;
	}

	if (saber1->numBlades > 1)
	{
		staffSaber = qtrue;
	}

	if (dualSabers && bs->cur_ps.fd.saberAnimLevel != SS_DUAL)
	{//dual sabers
		Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
	}

	if (staffSaber && bs->cur_ps.fd.saberAnimLevel != SS_STAFF)
	{//dual sabers
		Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
	}

	if (!dualSabers && !staffSaber
		&& (bs->cur_ps.fd.saberAnimLevel != SS_FAST &&
			bs->cur_ps.fd.saberAnimLevel != SS_TAVION &&
			bs->cur_ps.fd.saberAnimLevel != SS_MEDIUM &&
			bs->cur_ps.fd.saberAnimLevel != SS_STRONG &&
			bs->cur_ps.fd.saberAnimLevel != SS_DESANN))
	{//using a single saber
		Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
	}

	bot_check_speak(&g_entities[bs->client], qtrue);

	if (PM_InLedgeMove(bs->cur_ps.legsAnim))
	{
		//we're in a ledge move, just pull up for now
		trap->EA_MoveForward(bs->client);
		return;
	}

	if (bs->cur_ps.saberLockTime > level.time)
	{
		//bot is in a saber lock
		//AI cheats by knowing their enemy's fp level, if they're low on fP, try to super break finish them.
		if (g_entities[bs->cur_ps.saberLockEnemy].client->ps.fd.forcePower < 50)
		{
			trap->EA_Attack(bs->client);
		}
		if (g_entities[bs->cur_ps.saberLockEnemy].client->ps.fd.blockPoints < BLOCKPOINTS_HALF)
		{
			trap->EA_Attack(bs->client);
		}
		return;
	}

	VectorCopy(bs->goalAngles, pre_frame_g_angles);

	bs->doAttack = 0;
	bs->doAltAttack = 0;
	advanced_scanfor_enemies(bs);

	//determine which tactic we want to use.
	if (carrying_cap_objective(bs))
	{
		//we're carrying the objective, always go into capture mode.
		bs->currentTactic = BOTORDER_OBJECTIVE;
		bs->objectiveType = OT_CAPTURE;
	}
	else
	{
		//otherwise, just pick our tactic based on current situation.
		if (bs->botOrder == BOTORDER_NONE)
		{
			//we don't have a higher level order, use the default for the current situation
			if (bs->currentTactic)
			{
				//already have a tactic, use it.
			}
			else if (level.gametype == GT_SIEGE)
			{
				//hack do objectives
				bs->currentTactic = BOTORDER_OBJECTIVE;
			}
			else if (level.gametype == GT_CTF || level.gametype == GT_CTY)
			{
				determine_ctf_goal(bs);
			}
			else if (level.gametype == GT_SINGLE_PLAYER)
			{
				gentity_t* player = find_closest_human_player(bs->origin, NPCTEAM_PLAYER);
				if (player)
				{
					//a player on our team
					bs->currentTactic = BOTORDER_DEFEND;
					bs->tacticEntity = player;
				}
				else
				{
					//just run around and kill enemies
					bs->currentTactic = BOTORDER_SEARCHANDDESTROY;
					bs->tacticEntity = NULL;
				}
			}
			else if (level.gametype == GT_JEDIMASTER)
			{
				bs->currentTactic = BOTORDER_JEDIMASTER;
			}
			else
			{
				if (bs->isSquadLeader)
				{
					commander_bot_ai(bs);
				}
				else
				{
					bot_do_teamplay_ai(bs);
				}
			}
		}
		else
		{
			if (bs->isSquadLeader)
			{
				commander_bot_ai(bs);
			}
			else
			{
				bot_do_teamplay_ai(bs);
			}
		}
	}

	if (!bs->currentEnemy)
	{
		bs->frame_Enemy_Vis = 0;
	}

	if (bs->revengeEnemy && bs->revengeEnemy->client &&
		bs->revengeEnemy->client->pers.connected != CON_CONNECTED && bs->revengeEnemy->client->pers.connected !=
		CON_CONNECTING)
	{
		bs->revengeEnemy = NULL;
		bs->revengeHateLevel = 0;
	}

	if (bs->currentEnemy && bs->currentEnemy->client &&
		bs->currentEnemy->client->pers.connected != CON_CONNECTED && bs->currentEnemy->client->pers.connected !=
		CON_CONNECTING)
	{
		bs->currentEnemy = NULL;
	}

	fj_halt = 0;

#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->forceJumpChargeTime > level.time)
	{
		use_the_force = 1;
		forceHostile = 0;
	}

	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis && bs->forceJumpChargeTime < level.time)
#else
	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis)
#endif
	{
		VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, a_fo);
		vectoangles(a_fo, a_fo);

		//do this above all things
		if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_PUSH && (bs->doForcePush > level.time || bs->cur_ps.fd.
			forceGripBeingGripped > level.time) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[
				level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH])
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
			use_the_force = 1;
			forceHostile = 1;
		}
		else if (bs->cur_ps.fd.forceSide == FORCE_DARKSIDE)
		{
			//try dark side powers
			//in order of priority top to bottom
			if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_GRIP && bs->cur_ps.fd.forcePowersActive & 1 << FP_GRIP
				&& in_field_of_vision(bs->viewangles, 50, a_fo))
			{
				//already gripping someone, so hold it
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
				use_the_force = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_LIGHTNING && bs->frame_Enemy_Len <
				FORCE_LIGHTNING_RADIUS && level.clients[bs->client].ps.fd.forcePower > 50 && in_field_of_vision(
					bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_LIGHTNING;
				use_the_force = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_GRIP && bs->frame_Enemy_Len < MAX_GRIP_DISTANCE &&
				level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.
				forcePowerLevel[FP_GRIP]][FP_GRIP] && in_field_of_vision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
				use_the_force = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_RAGE && g_entities[bs->client].health < 25 && level.
				clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[
					FP_RAGE]][FP_RAGE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
				use_the_force = 1;
				forceHostile = 0;
			}
			else if (bs->cur_ps.weapon == WP_BOWCASTER && bs->cur_ps.fd.forcePowersKnown & 1 << FP_RAGE &&
				g_entities[bs->client].health < 75 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[
					level.clients[bs->client].ps.fd.forcePowerLevel[FP_RAGE]][FP_RAGE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
				use_the_force = 1;
				forceHostile = 0;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_DRAIN && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE &&
				level.clients[bs->client].ps.fd.forcePower > 50 && in_field_of_vision(bs->viewangles, 50, a_fo) && bs->
				currentEnemy->client->ps.fd.forcePower > 10 && bs->currentEnemy->client->ps.fd.forceSide ==
				FORCE_LIGHTSIDE)
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
				use_the_force = 1;
				forceHostile = 1;
			}
		}
		else if (bs->cur_ps.fd.forceSide == FORCE_LIGHTSIDE)
		{
			//try light side powers
			if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_ABSORB && bs->cur_ps.fd.forceGripCripple &&
				level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.
				forcePowerLevel[FP_ABSORB]][FP_ABSORB])
			{
				//absorb to get out
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
				use_the_force = 1;
				forceHostile = 0;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_ABSORB && bs->cur_ps.electrifyTime >= level.time &&
				level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.
				forcePowerLevel[FP_ABSORB]][FP_ABSORB])
			{
				//absorb lightning
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
				use_the_force = 1;
				forceHostile = 0;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_TELEPATHY && bs->frame_Enemy_Len < MAX_TRICK_DISTANCE
				&& level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.
				forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY] && in_field_of_vision(bs->viewangles, 50, a_fo) && !(bs
					->
					currentEnemy->client->ps.fd.forcePowersActive & 1 << FP_SEE))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TELEPATHY;
				use_the_force = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_ABSORB && g_entities[bs->client].health < 75 && bs->
				currentEnemy->client->ps.fd.forceSide == FORCE_DARKSIDE && level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
				use_the_force = 1;
				forceHostile = 0;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_PROTECT && g_entities[bs->client].health < 35 && level
				.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel
				[FP_PROTECT]][FP_PROTECT])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PROTECT;
				use_the_force = 1;
				forceHostile = 0;
			}
		}
		else if (bs->cur_ps.saberInFlight && !bs->cur_ps.saberEntityNum)
		{
			//we've lost our saber.
			//check to see if we can get the saber back yet.
			if (g_entities[g_entities[bs->client].client->saberStoredIndex].s.pos.trType == TR_STATIONARY)
			{
				//saber is ready to be pulled back
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SABERTHROW;
				use_the_force = 1;
				forceHostile = 0;
			}
		}

		if (!use_the_force)
		{
			//try neutral powers
			if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_PUSH && bs->cur_ps.fd.forceGripBeingGripped > level.time &&
				level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.
				forcePowerLevel[FP_PUSH]][FP_PUSH] && in_field_of_vision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				use_the_force = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_SPEED && g_entities[bs->client].health < 25 && level.
				clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[
					FP_SPEED]][FP_SPEED])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SPEED;
				use_the_force = 1;
				forceHostile = 0;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_SEE &&
				bot_mind_tricked(bs->client, bs->currentEnemy->s.number) && level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SEE]][FP_SEE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SEE;
				use_the_force = 1;
				forceHostile = 0;
			}
			else
			{
				if (rand() % 10 < 5 && bs->doForcePushPullSpamTime > level.time)
				{
					if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_PULL && bs->frame_Enemy_Len < 256 && level.clients[
						bs->client].ps.fd.forcePower > 75 && in_field_of_vision(bs->viewangles, 50, a_fo))
					{
						level.clients[bs->client].ps.fd.forcePowerSelected = FP_PULL;
						use_the_force = 1;
						forceHostile = 1;
					}

					bs->doForcePushPullSpamTime = level.time + 500;
				}
				else
				{
					if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_PUSH && bs->frame_Enemy_Len < 256 && level.clients[
						bs->client].ps.fd.forcePower > 75 && in_field_of_vision(bs->viewangles, 50, a_fo))
					{
						level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
						use_the_force = 1;
						forceHostile = 1;
					}

					bs->doForcePushPullSpamTime = level.time + 500;
				}
			}
		}
	}

	if (!use_the_force)
	{
		//try powers that we don't care if we have an enemy for
		if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_HEAL && g_entities[bs->client].health < 50 && level.clients[bs
			->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][
				FP_HEAL] && bs->cur_ps.fd.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_1)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			use_the_force = 1;
			forceHostile = 0;
		}
		else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_HEAL && g_entities[bs->client].health < 50 && level.
			clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[
				FP_HEAL]][FP_HEAL] && !bs->currentEnemy && bs->isCamping > level.time)
		{
			//only meditate and heal if we're camping
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			use_the_force = 1;
			forceHostile = 0;
		}
	}

	if (use_the_force && forceHostile)
	{
		if (bs->currentEnemy && bs->currentEnemy->client &&
			!ForcePowerUsableOn(&g_entities[bs->client], bs->currentEnemy,
				level.clients[bs->client].ps.fd.forcePowerSelected))
		{
			use_the_force = 0;
			forceHostile = 0;
		}
	}

	doing_fallback = 0;

	bs->deathActivitiesDone = 0;

	if (bot_use_inventory_item(bs))
	{
		if (rand() % 10 < 5)
		{
			trap->EA_Use(bs->client);
		}
	}

	if (g_AllowBotBuyItem.integer)
	{
		if (g_entities[bs->client].health <= 50)
		{
			bot_buy_item(bs, "health");
		}
		else if (bs->cur_ps.stats[STAT_ARMOR] <= 50)
		{
			bot_buy_item(bs, "shield");
		}
		else
		{
			switch (rand() % 35)
			{
			case 5: bot_buy_item(bs, "seeker");
			case 10: bot_buy_item(bs, "blaster");
			case 15: bot_buy_item(bs, "launcher");
			case 16: bot_buy_item(bs, "concussion");
			case 20: bot_buy_item(bs, "bowcaster");
			default:
				bot_buy_item(bs, "sentry");
			}
		}
	}

	if (bs->cur_ps.ammo[weaponData[bs->cur_ps.weapon].ammoIndex] < weaponData[bs->cur_ps.weapon].energyPerShot)
	{
		if (g_AllowBotBuyItem.integer)
		{
			if (bot_buy_item(bs, "ammo"))
			{
				return;
			}
		}
		else
		{
			if (bot_try_another_weapon(bs))
			{
				return;
			}
		}
	}
	else
	{
		int sel_result = 0;
		if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number &&
			bs->frame_Enemy_Vis && bs->forceWeaponSelect)
		{
			bs->forceWeaponSelect = 0;
		}

		if (bs->plantContinue > level.time)
		{
			bs->doAttack = 1;
			bs->destinationGrabTime = 0;
		}

		if (!bs->forceWeaponSelect && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
		{
			bs->forceWeaponSelect = WP_DET_PACK;
		}

		if (bs->forceWeaponSelect)
		{
			sel_result = bot_select_choice_weapon(bs, bs->forceWeaponSelect, 1);
		}

		if (sel_result)
		{
			if (sel_result == 2)
			{
				//newly selected
				return;
			}
		}
		else if (bot_select_ideal_weapon(bs))
		{
			return;
		}
	}

	reaction = bs->skills.reflex / bs->settings.skill;

	if (reaction < 0)
	{
		reaction = 0;
	}
	if (reaction > 2000)
	{
		reaction = 2000;
	}

	if (!bs->currentEnemy)
	{
		bs->timeToReact = level.time + reaction;
	}

	if (bs->cur_ps.weapon == WP_DET_PACK && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
	{
		bs->doAltAttack = 1;
	}

	if (bs->wpCamping)
	{
		if (bs->isCamping < level.time)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}

		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}
	}

	if (bs->wpCurrent &&
		(bs->wpSeenTime < level.time || bs->wpTravelTime < level.time))
	{
		bs->wpCurrent = NULL;
	}

	if (bs->currentEnemy)
	{
		if (bs->enemySeenTime < level.time ||
			!pass_standard_enemy_checks(bs, bs->currentEnemy))
		{
			if (bs->revengeEnemy == bs->currentEnemy &&
				bs->currentEnemy->health < 1 &&
				bs->lastAttacked && bs->lastAttacked == bs->currentEnemy)
			{
				bs->chatObject = bs->revengeEnemy;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "KilledHatedOne", 1);
				bs->revengeEnemy = NULL;
				bs->revengeHateLevel = 0;
			}
			else if (bs->currentEnemy->health < 1 && pass_loved_one_check(bs, bs->currentEnemy) &&
				bs->lastAttacked && bs->lastAttacked == bs->currentEnemy)
			{
				bs->chatObject = bs->currentEnemy;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Killed", 0);
			}

			bs->currentEnemy = NULL;
		}
	}

	if (bot_honorableduelacceptance.integer)
	{
		if (bs->currentEnemy && bs->currentEnemy->client &&
			g_privateDuel.integer &&
			bs->frame_Enemy_Vis &&
			bs->frame_Enemy_Len < 400)

		{
			vec3_t e_ang_vec;

			VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, e_ang_vec);

			if (in_field_of_vision(bs->viewangles, 100, e_ang_vec))
			{
				if (bs->currentEnemy->client->ps.duelIndex == bs->client &&
					bs->currentEnemy->client->ps.duelTime > level.time &&
					!bs->cur_ps.duelInProgress)
				{
					Cmd_EngageDuel_f(&g_entities[bs->client]);
				}

				bs->doAttack = 0;
				bs->doAltAttack = 0;
				bs->doBotKick = 0;
				bs->botChallengingTime = level.time + 100;
			}
		}
	}

	if (!bs->wpCurrent)
	{
		int wp;
		wp = get_nearest_visible_wp(bs->origin, bs->client);

		if (wp != -1)
		{
			bs->wpCurrent = gWPArray[wp];
			bs->wpSeenTime = level.time + 1500;
			bs->wpTravelTime = level.time + 10000; //never take more than 10 seconds to travel to a waypoint
		}
	}

	if (bs->enemySeenTime < level.time || !bs->frame_Enemy_Vis || !bs->currentEnemy ||
		bs->currentEnemy)
	{
		int enemy;
		enemy = scan_for_enemies(bs);

		if (enemy != -1)
		{
			bs->currentEnemy = &g_entities[enemy];
			bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
		}
	}

	if (!bs->squadLeader && !bs->isSquadLeader)
	{
		bot_scan_for_leader(bs);
	}

	if (!bs->squadLeader && bs->squadCannotLead < level.time)
	{
		//if still no leader after scanning, then become a squad leader
		bs->isSquadLeader = 1;
	}

	if (bs->isSquadLeader && bs->squadLeader)
	{
		//we don't follow anyone if we are a leader
		bs->squadLeader = NULL;
	}

	//ESTABLISH VISIBILITIES AND DISTANCES FOR THE WHOLE FRAME HERE
	if (bs->wpCurrent)
	{
		int vis_result = 0;
		if (RMG.integer)
		{
			//this is somewhat hacky, but in RMG we don't really care about vertical placement because points are scattered across only the terrain.
			vec3_t vec_b, vec_c;

			vec_b[0] = bs->origin[0];
			vec_b[1] = bs->origin[1];
			vec_b[2] = bs->origin[2];

			vec_c[0] = bs->wpCurrent->origin[0];
			vec_c[1] = bs->wpCurrent->origin[1];
			vec_c[2] = vec_b[2];

			VectorSubtract(vec_c, vec_b, a);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
		}
		bs->frame_Waypoint_Len = VectorLength(a);

		vis_result = wp_org_visible(&g_entities[bs->client], bs->origin, bs->wpCurrent->origin, bs->client);

		if (vis_result == 2)
		{
			bs->frame_Waypoint_Vis = 0;
			bs->wpSeenTime = 0;
			bs->wpDestination = NULL;
			bs->wpDestIgnoreTime = level.time + 5000;

			if (bs->wpDirection)
			{
				bs->wpDirection = 0;
			}
			else
			{
				bs->wpDirection = 1;
			}
		}
		else if (vis_result)
		{
			bs->frame_Waypoint_Vis = 1;
		}
		else
		{
			bs->frame_Waypoint_Vis = 0;
		}
	}

	if (bs->currentEnemy)
	{
		vec3_t eorg;
		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, eorg);
			eorg[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->s.origin, eorg);
		}

		VectorSubtract(eorg, bs->eye, a);
		bs->frame_Enemy_Len = VectorLength(a);

		if (org_visible(bs->eye, eorg, bs->client))
		{
			bs->frame_Enemy_Vis = 1;
			VectorCopy(eorg, bs->lastEnemySpotted);
			VectorCopy(bs->origin, bs->hereWhenSpotted);
			bs->lastVisibleEnemyIndex = bs->currentEnemy->s.number;
			bs->hitSpotted = 0;
		}
		else
		{
			bs->frame_Enemy_Vis = 0;
		}
	}
	else
	{
		bs->lastVisibleEnemyIndex = ENTITYNUM_NONE;
	}
	//END

	if (bs->frame_Enemy_Vis)
	{
		bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
	}

	if (bs->wpCurrent)
	{
		int goal_wp_index;
		int wp_touch_dist = BOT_WPTOUCH_DISTANCE;
		wp_constant_routine(bs);

		if (!bs->wpCurrent)
		{
			//WPConstantRoutine has the ability to nullify the waypoint if it fails certain checks, so..
			return;
		}

		if (bs->wpCurrent->flags & WPFLAG_WAITFORFUNC)
		{
			if (!check_for_func(bs->wpCurrent->origin, -1))
			{
				bs->beStill = level.time + 500; //no func brush under.. wait
			}
		}
		if (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
		{
			if (check_for_func(bs->wpCurrent->origin, -1))
			{
				bs->beStill = level.time + 500; //func brush under.. wait
			}
		}

		if (bs->frame_Waypoint_Vis || bs->wpCurrent->flags & WPFLAG_NOVIS)
		{
			if (RMG.integer)
			{
				bs->wpSeenTime = level.time + 5000;
				//if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
			else
			{
				bs->wpSeenTime = level.time + 1500;
				//if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
		}
		VectorCopy(bs->wpCurrent->origin, bs->goalPosition);
		if (bs->wpDirection)
		{
			goal_wp_index = bs->wpCurrent->index - 1;
		}
		else
		{
			goal_wp_index = bs->wpCurrent->index + 1;
		}

		if (bs->wpCamping)
		{
			VectorSubtract(bs->wpCampingTo->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);

			VectorSubtract(bs->origin, bs->wpCamping->origin, a);
			if (VectorLength(a) < 64)
			{
				VectorCopy(bs->wpCamping->origin, bs->goalPosition);
				bs->beStill = level.time + 1000;

				if (!bs->campStanding)
				{
					bs->duckTime = level.time + 1000;
				}
			}
		}
		else if (gWPArray[goal_wp_index] && gWPArray[goal_wp_index]->inuse &&
			!(gLevelFlags & LEVELFLAG_NOPOINTPREDICTION))
		{
			VectorSubtract(gWPArray[goal_wp_index]->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}

		if (bs->destinationGrabTime < level.time)
		{
			get_ideal_destination(bs);
		}

		if (bs->wpCurrent && bs->wpDestination)
		{
			if (total_trail_distance(bs->wpCurrent->index, bs->wpDestination->index) == -1)
			{
				bs->wpDestination = NULL;
				bs->destinationGrabTime = level.time + 10000;
			}
		}

		if (RMG.integer)
		{
			if (bs->frame_Waypoint_Vis)
			{
				if (bs->wpCurrent && !bs->wpCurrent->flags)
				{
					wp_touch_dist *= 3;
				}
			}
		}

		if (bs->frame_Waypoint_Len < wp_touch_dist || RMG.integer && bs->frame_Waypoint_Len < wp_touch_dist * 2)
		{
			int desired_index;
			WPTouchRoutine(bs);

			if (!bs->wpDirection)
			{
				desired_index = bs->wpCurrent->index + 1;
			}
			else
			{
				desired_index = bs->wpCurrent->index - 1;
			}

			if (gWPArray[desired_index] &&
				gWPArray[desired_index]->inuse &&
				desired_index < gWPNum &&
				desired_index >= 0 &&
				pass_way_check(bs, desired_index))
			{
				bs->wpCurrent = gWPArray[desired_index];
			}
			else
			{
				if (bs->wpDestination)
				{
					bs->wpDestination = NULL;
					bs->destinationGrabTime = level.time + 10000;
				}

				if (bs->wpDirection)
				{
					bs->wpDirection = 0;
				}
				else
				{
					bs->wpDirection = 1;
				}
			}
		}
	}
	else //We can't find a waypoint, going to need a fallback routine.
	{
		if (bs->cur_ps.weapon == WP_SABER)
		{
			doing_fallback = saber_bot_fallback_navigation(bs);
		}
		else
		{
			if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
			{
				// Jetpacker.. Jetpack ON
				bs->jumpTime = level.time + 1500;
				bs->jDelay = 0;
				bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
				bs->cur_ps.eFlags |= EF_JETPACK_FLAMING; //going up
				bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
			}
			doing_fallback = gunner_bot_fallback_navigation(bs);
		}
	}

	if (bs->currentEnemy && bs->entityNum < MAX_CLIENTS)
	{
		if (g_entities[bs->entityNum].health <= 0 || bs->currentEnemy->health <= 0)
		{
			//dont do it if there dead...doh
		}
		else
		{
			vec3_t dir;
			float xy;
			qboolean will_fall = qfalse;

			if (next_bot_fallcheck[bs->entityNum] < level.time)
			{
				vec3_t fwd;
				if (check_fall_by_vectors(bs->origin, fwd, &g_entities[bs->entityNum]) == qtrue)
				{
					will_fall = qtrue;
				}
				if (bot_will_fall[bs->entityNum])
				{
					next_bot_fallcheck[bs->entityNum] = level.time + 50;
					bot_will_fall[bs->entityNum] = will_fall;
				}
			}
			else
			{
				vec3_t p2;
				vec3_t p1;
				will_fall = bot_will_fall[bs->entityNum];
				if (bs->origin[2] > bs->currentEnemy->r.currentOrigin[2])
				{
					VectorCopy(bs->origin, p1);
					VectorCopy(bs->currentEnemy->r.currentOrigin, p2);
				}
				else if (bs->origin[2] < bs->currentEnemy->r.currentOrigin[2])
				{
					VectorCopy(bs->currentEnemy->r.currentOrigin, p1);
					VectorCopy(bs->origin, p2);
				}
				else
				{
					VectorCopy(bs->origin, p1);
					VectorCopy(bs->currentEnemy->r.currentOrigin, p2);
				}
				VectorSubtract(p2, p1, dir);
				dir[2] = 0;
			}
			//Get xy and z diffs
			xy = VectorNormalize(dir);

			// Jumping Stuff.
			if (bs->cur_ps.weapon == WP_SABER
				&& bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy < 300
				&& bs->currentEnemy->r.currentOrigin[2] > bs->origin[2] + 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				// Jump to enemy. NPC style! They are above us. Do a Jump.
				bs->BOTjumpState = JS_FACING;
				ai_mod_jump(bs);

				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->cur_ps.weapon == WP_SABER
				&& bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy > 1000
				&& bs->currentEnemy->r.currentOrigin[2] < bs->origin[2] - 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				// Jump to enemy. NPC style! They are below us.
				bs->BOTjumpState = JS_FACING;
				ai_mod_jump(bs);

				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK
				&& bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy < 300
				&& bs->currentEnemy->r.currentOrigin[2] > bs->origin[2] + 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				// Jump to enemy. NPC style! They are above us. Do a Jump.
				bs->BOTjumpState = JS_FACING;
				ai_mod_jump(bs);

				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK
				&& bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy > 1400
				&& bs->currentEnemy->r.currentOrigin[2] < bs->origin[2] - 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				// Jump to enemy. NPC style! They are below us.
				bs->BOTjumpState = JS_FACING;
				ai_mod_jump(bs);

				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->BOTjumpState >= JS_CROUCHING)
			{
				// Continue any jumps.
				ai_mod_jump(bs);
			}
		}
	}

	if (!VectorCompare(vec3_origin, move_dir))
	{
		trap->EA_Move(bs->client, move_dir, 5000);
	}

	if (RMG.integer)
	{
		//for RMG if the bot sticks around an area too long, jump around randomly some to spread to a new area (horrible hacky method)
		vec3_t v_sub_dif;

		VectorSubtract(bs->origin, bs->lastSignificantAreaChange, v_sub_dif);
		if (VectorLength(v_sub_dif) > 1500)
		{
			VectorCopy(bs->origin, bs->lastSignificantAreaChange);
			bs->lastSignificantChangeTime = level.time + 20000;
		}

		if (bs->lastSignificantChangeTime < level.time)
		{
			bs->iHaveNoIdeaWhereIAmGoing = level.time + 17000;
		}
	}

	if (bs->iHaveNoIdeaWhereIAmGoing > level.time && !bs->currentEnemy)
	{
		VectorCopy(pre_frame_g_angles, bs->goalAngles);
		bs->wpCurrent = NULL;
		bs->wpSwitchTime = level.time + 150;
		if (bs->cur_ps.weapon == WP_SABER)
		{
			doing_fallback = saber_bot_fallback_navigation(bs);
		}
		else
		{
			if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
			{
				// Jetpacker.. Jetpack ON
				bs->jumpTime = level.time + 1500;
				bs->jDelay = 0;
				bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
				bs->cur_ps.eFlags |= EF_JETPACK_FLAMING; //going up
				bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
			}
			doing_fallback = gunner_bot_fallback_navigation(bs);
		}
		bs->jumpTime = level.time + 150;
		bs->jumpHoldTime = level.time + 150;
		bs->jDelay = 0;
		bs->lastSignificantChangeTime = level.time + 25000;
	}

	if (bs->wpCurrent && RMG.integer)
	{
		qboolean do_j = qfalse;

		if (bs->wpCurrent->origin[2] - 192 > bs->origin[2])
		{
			do_j = qtrue;
		}
		else if (bs->wpTravelTime - level.time < 5000 && bs->wpCurrent->origin[2] - 64 > bs->origin[2])
		{
			do_j = qtrue;
		}
		else if (bs->wpTravelTime - level.time < 7000 && bs->wpCurrent->flags & WPFLAG_RED_FLAG)
		{
			if (level.time - bs->jumpTime > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if (bs->wpTravelTime - level.time < 7000 && bs->wpCurrent->flags & WPFLAG_BLUE_FLAG)
		{
			if (level.time - bs->jumpTime > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if (bs->wpCurrent->index > 0)
		{
			if (bs->wpTravelTime - level.time < 7000)
			{
				if (gWPArray[bs->wpCurrent->index - 1]->flags & WPFLAG_RED_FLAG ||
					gWPArray[bs->wpCurrent->index - 1]->flags & WPFLAG_BLUE_FLAG)
				{
					if (level.time - bs->jumpTime > 200)
					{
						bs->jumpTime = level.time + 100;
						bs->jumpHoldTime = level.time + 100;
						bs->jDelay = 0;
					}
				}
			}
		}

		if (do_j)
		{
			bs->jumpTime = level.time + 1500;
			bs->jumpHoldTime = level.time + 1500;
			bs->jDelay = 0;
		}
	}

	if (doing_fallback)
	{
		bs->doingFallback = qtrue;
	}
	else
	{
		bs->doingFallback = qfalse;
	}

	if (bs->timeToReact < level.time && bs->currentEnemy && bs->enemySeenTime > level.time + (ENEMY_FORGET_MS -
		ENEMY_FORGET_MS * 0.2))
	{
		vec3_t headlevel;
		if (bs->frame_Enemy_Vis)
		{
			combat_bot_ai(bs);
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
		{
			//keep charging in case we see him again before we lose track of him
			bs->doAltAttack = 1;
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING)
		{
			//keep charging in case we see him again before we lose track of him
			bs->doAttack = 1;
		}

		if (bs->destinationGrabTime > level.time + 100)
		{
			bs->destinationGrabTime = level.time + 100;
			//assures that we will continue staying within a general area of where we want to be in a combat situation
		}

		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
			headlevel[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
		}

		if (!bs->frame_Enemy_Vis)
		{
			if (org_visible(bs->eye, bs->lastEnemySpotted, -1))
			{
				VectorCopy(bs->lastEnemySpotted, headlevel);
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);

				if (bs->cur_ps.weapon == WP_FLECHETTE &&
					bs->cur_ps.weaponstate == WEAPON_READY &&
					bs->currentEnemy && bs->currentEnemy->client)
				{
					float m_len;
					m_len = VectorLength(a) > 128;
					if (m_len > 128 && m_len < 1024)
					{
						VectorSubtract(bs->currentEnemy->client->ps.origin, bs->lastEnemySpotted, a);

						if (VectorLength(a) < 300)
						{
							bs->doAltAttack = 1;
						}
					}
				}
			}
		}
		else
		{
			float b_lead_amount;
			b_lead_amount = bot_weapon_can_lead(bs);
			if (bs->skills.accuracy / bs->settings.skill <= 8 &&
				b_lead_amount)
			{
				bot_aim_leading(bs, headlevel, b_lead_amount);
			}
			else
			{
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);
			}

			bot_aim_offset_goal_angles(bs);
		}
	}

	if (bs->cur_ps.saberInFlight)
	{
		bs->saberThrowTime = level.time + Q_irand(4000, 10000);
	}

	if (bs->currentEnemy)
	{
		if (bot_get_weapon_range(bs) == BWEAPONRANGE_SABER)
		{
			int saber_range = SABER_ATTACK_RANGE;

			VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, a_fo);
			vectoangles(a_fo, a_fo);

			if (bs->saberPowerTime < level.time)
			{
				//Don't just use strong attacks constantly, switch around a bit
				if (Q_irand(1, 10) <= 5)
				{
					bs->saberPower = qtrue;
				}
				else
				{
					bs->saberPower = qfalse;
				}

				bs->saberPowerTime = level.time + Q_irand(3000, 15000);
			}

			if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_STAFF
				&& g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_DUAL) // dont change staff or dual styles
			{
				if (bs->currentEnemy->client->ps.fd.blockPoints > BLOCKPOINTS_FULL   // enemy has high BP
					&& g_entities[bs->client].client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] > 2) // We have offense level 3
				{
					if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_STRONG
						&& g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_DESANN && bs->saberPower)  // should swap from desann to strong and vise versa
					{ //if we are up against someone with a lot of blockpoints and we have a strong attack available, then h4q them
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
				else if (bs->currentEnemy->client->ps.fd.blockPoints > BLOCKPOINTS_HALF
					&& g_entities[bs->client].client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] > 1)
				{
					if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_MEDIUM)
					{ //they're down on blockpoints a little, use level 2 if we can
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
				else
				{
					if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_FAST
						&& g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_TAVION)
					{ //they've gone below 40 blockpoints, go at them with quick attacks
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
			}

			if (level.gametype == GT_SINGLE_PLAYER)
			{
				saber_range *= 3;
			}

			if (bs->frame_Enemy_Len <= saber_range)
			{
				saber_combat_handling(bs);

				if (bs->frame_Enemy_Len < 80)
				{
					meleestrafe = 1;
				}
			}
			else if (bs->saberThrowTime < level.time && !bs->cur_ps.saberInFlight &&
				bs->cur_ps.fd.forcePowersKnown & 1 << FP_SABERTHROW &&
				in_field_of_vision(bs->viewangles, 30, a_fo) &&
				(bs->frame_Enemy_Len > 512 && bs->frame_Enemy_Len < BOT_SABER_THROW_RANGE))
			{
				bs->doAltAttack = 1;
				bs->doAttack = 0;
			}
			else if (bs->cur_ps.saberInFlight && bs->frame_Enemy_Len > 300 && bs->frame_Enemy_Len <
				BOT_SABER_THROW_RANGE)
			{
				bs->doAltAttack = 1;
				bs->doAttack = 0;
			}
		}
		else if (bot_get_weapon_range(bs) == BWEAPONRANGE_MELEE)
		{
			if (bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE)
			{
				melee_combat_handling(bs);
				meleestrafe = 1;
			}
		}
	}

	if (doing_fallback && bs->currentEnemy) //just stand and fire if we have no idea where we are
	{
		VectorCopy(bs->origin, bs->goalPosition);
	}

	if (bs->forceJumping > level.time)
	{
		vec3_t noz_y;
		vec3_t noz_x;
		VectorCopy(bs->origin, noz_x);
		VectorCopy(bs->goalPosition, noz_y);

		noz_x[2] = noz_y[2];

		VectorSubtract(noz_x, noz_y, noz_x);

		if (VectorLength(noz_x) < 32)
		{
			fj_halt = 1;
		}
	}

	if (bs->cur_ps.lastOnGround + 300 < level.time //haven't been on the ground for a while
		&& (!bs->cur_ps.fd.forceJumpZStart || bs->origin[2] < bs->cur_ps.fd.forceJumpZStart))
		//and not safely landing from a jump
	{
		//been off the ground for a little while
		float speed = VectorLength(bs->cur_ps.velocity);
		if (speed >= 100 + g_entities[bs->client].health && bs->virtualWeapon != WP_SABER || speed >= 700)
		{
			//moving fast enough to get hurt get ready to crouch roll
			if (bs->virtualWeapon != WP_SABER)
			{
				//try switching to saber
				if (!bot_select_choice_weapon(bs, WP_SABER, 1))
				{
					//ok, try switching to melee
					bot_select_choice_weapon(bs, WP_MELEE, 1);
				}
			}

			if (bs->virtualWeapon == WP_MELEE || bs->virtualWeapon == WP_SABER)
			{
				//in or switching to a weapon that allows us to do roll landings
				bs->duckTime = level.time + 300;
				if (!bs->lastucmd.forwardmove && !bs->lastucmd.rightmove)
				{
					//not trying to move at all so we should at least attempt to move
					trap->EA_MoveForward(bs->client);
				}
			}
		}
	}

	if (bs->doChat && bs->chatTime > level.time && (!bs->currentEnemy || !bs->frame_Enemy_Vis))
	{
		return;
	}
	if (bs->doChat && bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		bs->doChat = 0; //do we want to keep the bot waiting to chat until after the enemy is gone?
		bs->chatTeam = 0;
	}
	else if (bs->doChat && bs->chatTime <= level.time)
	{
		if (bs->chatTeam)
		{
			trap->EA_SayTeam(bs->client, bs->currentChat);
			bs->chatTeam = 0;
		}
		else
		{
			trap->EA_Say(bs->client, bs->currentChat);
		}
		if (bs->doChat == 2)
		{
			bot_reply_greetings(bs);
		}
		bs->doChat = 0;
	}

	ctf_flag_movement(bs);

	if (bs->shootGoal &&
		bs->shootGoal->health > 0 && bs->shootGoal->takedamage)
	{
		vec3_t dif;
		dif[0] = (bs->shootGoal->r.absmax[0] + bs->shootGoal->r.absmin[0]) / 2;
		dif[1] = (bs->shootGoal->r.absmax[1] + bs->shootGoal->r.absmin[1]) / 2;
		dif[2] = (bs->shootGoal->r.absmax[2] + bs->shootGoal->r.absmin[2]) / 2;

		if (!bs->currentEnemy || bs->frame_Enemy_Len > 256)
		{
			//if someone is close then don't stop shooting them for this
			VectorSubtract(dif, bs->eye, a);
			vectoangles(a, a);
			VectorCopy(a, bs->goalAngles);

			if (in_field_of_vision(bs->viewangles, 30, a) &&
				entity_visible_box(bs->origin, NULL, NULL, dif, bs->client, bs->shootGoal->s.number))
			{
				bs->doAttack = 1;
			}
		}
	}

	if (bs->cur_ps.hasDetPackPlanted)
	{
		//check if our enemy gets near it and detonate if he does
		bot_check_det_packs(bs);
	}
	else if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number && !bs->frame_Enemy_Vis && bs->
		plantTime < level.time &&
		!bs->doAttack && !bs->doAltAttack)
	{
		VectorSubtract(bs->origin, bs->hereWhenSpotted, a);

		if (bs->plantDecided > level.time || bs->frame_Enemy_Len < BOT_PLANT_DISTANCE * 2 && VectorLength(a) <
			BOT_PLANT_DISTANCE)
		{
			int det_select = 0;
			int mine_select = 0;
			mine_select = bot_select_choice_weapon(bs, WP_TRIP_MINE, 0);
			det_select = bot_select_choice_weapon(bs, WP_DET_PACK, 0);
			if (bs->cur_ps.hasDetPackPlanted)
			{
				det_select = 0;
			}

			if (bs->plantDecided > level.time && bs->forceWeaponSelect &&
				bs->cur_ps.weapon == bs->forceWeaponSelect)
			{
				bs->doAttack = 1;
				bs->plantDecided = 0;
				bs->plantTime = level.time + BOT_PLANT_INTERVAL;
				bs->plantContinue = level.time + 500;
				bs->beStill = level.time + 500;
			}
			else if (mine_select || det_select)
			{
				if (bot_surface_near(bs))
				{
					if (!mine_select)
					{
						//if no mines use detpacks, otherwise use mines
						mine_select = WP_DET_PACK;
					}
					else
					{
						mine_select = WP_TRIP_MINE;
					}

					det_select = bot_select_choice_weapon(bs, mine_select, 1);

					if (det_select && det_select != 2)
					{
						//We have it and it is now our weapon
						bs->plantDecided = level.time + 1000;
						bs->forceWeaponSelect = mine_select;
						return;
					}
					if (det_select == 2)
					{
						bs->forceWeaponSelect = mine_select;
						return;
					}
				}
			}
		}
	}
	else if (bs->plantContinue < level.time)
	{
		bs->forceWeaponSelect = 0;
	}

	if (level.gametype == GT_JEDIMASTER && !bs->cur_ps.isJediMaster && bs->jmState == -1 && gJMSaberEnt && gJMSaberEnt->
		inuse)
	{
		vec3_t saber_len;
		float f_saber_len = 0;

		VectorSubtract(bs->origin, gJMSaberEnt->r.currentOrigin, saber_len);
		f_saber_len = VectorLength(saber_len);

		if (f_saber_len < 256)
		{
			if (org_visible(bs->origin, gJMSaberEnt->r.currentOrigin, bs->client))
			{
				VectorCopy(gJMSaberEnt->r.currentOrigin, bs->goalPosition);
			}
		}
	}

	if (bs->beStill < level.time && !waiting_for_now(bs, bs->goalPosition) && !fj_halt)
	{
		VectorSubtract(bs->goalPosition, bs->origin, bs->goalMovedir);
		VectorNormalize(bs->goalMovedir);

		if (bs->jumpTime > level.time && bs->jDelay < level.time &&
			level.clients[bs->client].pers.cmd.upmove > 0)
		{
			bs->beStill = level.time + 200;
		}
		else
		{
			trap->EA_Move(bs->client, bs->goalMovedir, 5000);
		}

		if (meleestrafe && bs->meleeStrafeDisable < level.time)
		{
			strafe_tracing(bs);

			//StrafeTracing() can boost this level
			if (bs->meleeStrafeDisable < level.time)
			{
				if (bs->meleeStrafeDir)
				{
					trap->EA_MoveRight(bs->client);
				}
				else
				{
					trap->EA_MoveLeft(bs->client);
				}
			}
		}

		if (bot_trace_jump(bs, bs->goalPosition))
		{
			bs->jumpTime = level.time + 100;
		}
		else if (bot_trace_duck(bs, bs->goalPosition))
		{
			bs->duckTime = level.time + 100;
		}
#ifdef BOT_STRAFE_AVOIDANCE
		else
		{
			int strafe_around = bot_trace_strafe(bs, bs->goalPosition);

			if (strafe_around == STRAFEAROUND_RIGHT)
			{
				trap->EA_MoveRight(bs->client);
			}
			else if (strafe_around == STRAFEAROUND_LEFT)
			{
				trap->EA_MoveLeft(bs->client);
			}
		}
#endif
	}

#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->forceJumpChargeTime > level.time)
	{
		bs->jumpTime = 0;
	}
#endif

	if (bs->jumpPrep > level.time)
	{
		bs->forceJumpChargeTime = 0;
	}

	if (bs->forceJumpChargeTime > level.time)
	{
		bs->jumpHoldTime = (bs->forceJumpChargeTime - level.time) / 2 + level.time;
		bs->forceJumpChargeTime = 0;
	}

	if (bs->jumpHoldTime > level.time)
	{
		bs->jumpTime = bs->jumpHoldTime;
	}

	if (bs->jumpTime > level.time && bs->jDelay < level.time)
	{
		if (bs->jumpHoldTime > level.time)
		{
			trap->EA_Jump(bs->client);
			if (bs->wpCurrent)
			{
				if (bs->wpCurrent->origin[2] - bs->origin[2] < 64)
				{
					trap->EA_MoveForward(bs->client);
				}
			}
			else
			{
				trap->EA_MoveForward(bs->client);
			}
			if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
			}
		}
		else if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
		{
			trap->EA_Jump(bs->client);
		}
	}

	if (bs->duckTime > level.time)
	{
		trap->EA_Crouch(bs->client);
	}

	if (bs->dangerousObject && bs->dangerousObject->inuse && bs->dangerousObject->health > 0 &&
		bs->dangerousObject->takedamage && (!bs->frame_Enemy_Vis || !bs->currentEnemy) &&
		(bot_get_weapon_range(bs) == BWEAPONRANGE_MID || bot_get_weapon_range(bs) == BWEAPONRANGE_LONG) &&
		bs->cur_ps.weapon != WP_DET_PACK && bs->cur_ps.weapon != WP_TRIP_MINE &&
		!bs->shootGoal)
	{
		float dan_len;

		VectorSubtract(bs->dangerousObject->r.currentOrigin, bs->eye, a);

		dan_len = VectorLength(a);

		if (dan_len > 256)
		{
			vectoangles(a, a);
			VectorCopy(a, bs->goalAngles);

			if (Q_irand(1, 10) < 5)
			{
				bs->goalAngles[YAW] += Q_irand(0, 3);
				bs->goalAngles[PITCH] += Q_irand(0, 3);
			}
			else
			{
				bs->goalAngles[YAW] -= Q_irand(0, 3);
				bs->goalAngles[PITCH] -= Q_irand(0, 3);
			}

			if (in_field_of_vision(bs->viewangles, 30, a) &&
				entity_visible_box(bs->origin, NULL, NULL, bs->dangerousObject->r.currentOrigin, bs->client,
					bs->dangerousObject->s.number))
			{
				bs->doAttack = 1;
			}
		}
	}

	if (prim_firing(bs) ||
		alt_firing(bs))
	{
		friend_in_lof = check_for_friend_in_lof(bs);

		if (friend_in_lof)
		{
			if (prim_firing(bs))
			{
				keep_prim_from_firing(bs);
			}
			if (alt_firing(bs))
			{
				keep_alt_from_firing(bs);
			}
			if (use_the_force && forceHostile)
			{
				use_the_force = 0;
			}

			if (!use_the_force && friend_in_lof->client)
			{
				//we have a friend here and are not currently using force powers, see if we can help them out
				if (friend_in_lof->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.
					clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
					use_the_force = 1;
					forceHostile = 0;
				}
				else if (friend_in_lof->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower >
					forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
					use_the_force = 1;
					forceHostile = 0;
				}
			}
		}
	}
	else if (level.gametype >= GT_TEAM)
	{
		//still check for anyone to help..
		friend_in_lof = check_for_friend_in_lof(bs);

		if (!use_the_force && friend_in_lof)
		{
			if (friend_in_lof->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.
				clients
				[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
				use_the_force = 1;
				forceHostile = 0;
			}
			else if (friend_in_lof->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
				use_the_force = 1;
				forceHostile = 0;
			}
		}
	}

	if (bs->doAttack && bs->cur_ps.weapon == WP_DET_PACK &&
		bs->cur_ps.hasDetPackPlanted)
	{
		//maybe a bit hackish, but bots only want to plant one of these at any given time to avoid complications
		bs->doAttack = 0;
	}

	if (bs->doAttack && bs->cur_ps.weapon == WP_SABER &&
		bs->saberDefending && bs->currentEnemy && bs->currentEnemy->client &&
		bot_weapon_blockable(bs->currentEnemy->client->ps.weapon))
	{
		bs->doAttack = 0;
	}

	if (bs->cur_ps.saberLockTime > level.time && bs->saberLockDebounce < level.time)
	{
		if (rand() % 10 < 5)
		{
			bs->doAttack = 1;
		}
		else
		{
			bs->doAttack = 0;
		}
		bs->saberLockDebounce = level.time + 50;
	}

	if (bs->botChallengingTime > level.time)
	{
		bs->doAttack = 0;
		bs->doAltAttack = 0;
		bs->doBotKick = 0;
	}

	if (bs->cur_ps.weapon == WP_SABER &&
		bs->cur_ps.saberInFlight &&
		!bs->cur_ps.saberEntityNum)
	{
		//saber knocked away, keep trying to get it back
		bs->doAttack = 1;
		bs->doAltAttack = 0;
	}

	if (bs->doAttack)
	{
		trap->EA_Attack(bs->client);
	}
	else if (bs->doAltAttack)
	{
		trap->EA_Alt_Attack(bs->client);
	}

	if (use_the_force && forceHostile && bs->botChallengingTime > level.time)
	{
		use_the_force = qfalse;
	}

	if (use_the_force)
	{
#ifndef FORCEJUMP_INSTANTMETHOD
		if (bs->forceJumpChargeTime > level.time)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_LEVITATION;
			trap->EA_ForcePower(bs->client);
		}
		else
		{
#endif
			if (bot_forcepowers.integer && !g_forcePowerDisable.integer)
			{
				trap->EA_ForcePower(bs->client);
			}
#ifndef FORCEJUMP_INSTANTMETHOD
		}
#endif
	}

	move_toward_ideal_angles(bs);
}

void Enhanced_bot_ai(bot_state_t* bs)
{
	const int saberNum = 0;
	int doing_fallback = 0;
	int fj_halt;
	vec3_t a;
	vec3_t ang;
	vec3_t a_fo;
	float reaction;
	int meleestrafe = 0;
	int use_the_force = 0;
	int forceHostile = 0;
	gentity_t* friend_in_lof = 0;
	vec3_t pre_frame_g_angles;
	vec3_t move_dir;
	const saberInfo_t* saber1 = BG_MySaber(bs->client, 0);
	const saberInfo_t* saber2 = BG_MySaber(bs->client, 1);

	//Reset the action states
	bs->doAttack = qfalse;
	bs->doAltAttack = qfalse;
	bs->doSaberThrow = qfalse;
	bs->doBotKick = qfalse;
	bs->doWalk = qfalse;
	bs->virtualWeapon = bs->cur_ps.weapon;

	if (gDeactivated || g_entities[bs->client].client->tempSpectate > level.time)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		clear_route(bs->botRoute);
		VectorClear(bs->lastDestPosition);
		bs->wpSpecial = qfalse;

		//reset tactical stuff
		bs->tacticEntity = NULL;
		bs->objectiveType = 0;
		bs->MiscBotFlags = 0;
		return;
	}

	if (g_entities[bs->client].inuse &&
		g_entities[bs->client].client &&
		g_entities[bs->client].client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		return;
	}

#ifndef FINAL_BUILD
	if (bot_getinthecarrr.integer)
	{ //stupid vehicle debug, I tire of having to connect another client to test passengers.
		gentity_t* botEnt = &g_entities[bs->client];

		if (botEnt->inuse && botEnt->client && botEnt->client->ps.m_iVehicleNum)
		{ //in a vehicle, so...
			bs->noUseTime = level.time + 5000;

			if (bot_getinthecarrr.integer != 2)
			{
				trap->EA_MoveForward(bs->client);

				if (bot_getinthecarrr.integer == 3)
				{ //use alt fire
					trap->EA_Alt_Attack(bs->client);
				}
			}
		}
		else
		{ //find one, get in
			int i = 0;
			gentity_t* vehicle = NULL;
			//find the nearest, manned vehicle
			while (i < MAX_GENTITIES)
			{
				vehicle = &g_entities[i];

				if (vehicle->inuse && vehicle->client && vehicle->s.eType == ET_NPC &&
					vehicle->s.NPC_class == CLASS_VEHICLE && vehicle->m_pVehicle &&
					(vehicle->client->ps.m_iVehicleNum || bot_getinthecarrr.integer == 2))
				{ //ok, this is a vehicle, and it has a pilot/passengers
					break;
				}
				i++;
			}
			if (i != MAX_GENTITIES && vehicle)
			{ //broke before end so we must've found something
				vec3_t v;

				VectorSubtract(vehicle->client->ps.origin, bs->origin, v);
				VectorNormalize(v);
				vectoangles(v, bs->goalAngles);
				MoveTowardIdealAngles(bs);
				trap->EA_Move(bs->client, v, 5000.0f);

				if (bs->noUseTime < (level.time - 400))
				{
					bs->noUseTime = level.time + 500;
				}
			}
		}

		return;
	}
#endif

	if (bot_forgimmick.integer)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;

		if (bot_forgimmick.integer == 2)
		{
			//for debugging saber stuff, this is handy
			trap->EA_Attack(bs->client);
		}

		if (bot_forgimmick.integer == 3)
		{
			//for testing cpu usage moving around rmg terrain without AI
			vec3_t mdir;

			VectorSubtract(bs->origin, vec3_origin, mdir);
			VectorNormalize(mdir);
			trap->EA_Attack(bs->client);
			trap->EA_Move(bs->client, mdir, 5000);
		}

		if (bot_forgimmick.integer == 4)
		{
			//constantly move toward client 0
			if (g_entities[0].client && g_entities[0].inuse)
			{
				vec3_t mdir;

				VectorSubtract(g_entities[0].client->ps.origin, bs->origin, mdir);
				VectorNormalize(mdir);
				trap->EA_Move(bs->client, mdir, 5000);
			}
		}

		if (bs->forceMove_Forward)
		{
			if (bs->forceMove_Forward > 0)
			{
				trap->EA_MoveForward(bs->client);
			}
			else
			{
				trap->EA_MoveBack(bs->client);
			}
		}
		if (bs->forceMove_Right)
		{
			if (bs->forceMove_Right > 0)
			{
				trap->EA_MoveRight(bs->client);
			}
			else
			{
				trap->EA_MoveLeft(bs->client);
			}
		}
		if (bs->forceMove_Up)
		{
			trap->EA_Jump(bs->client);
		}
		return;
	}

	if (level.gametype == GT_SIEGE && level.time - level.startTime < 10000)
	{
		//make sure that the bots aren't all on the same team after map changes.
		select_best_siege_class(bs->client, qfalse);
	}

	if (bs->cur_ps.pm_type == PM_INTERMISSION
		|| g_entities[bs->client].client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		//in intermission
		//Mash the button to prevent the game from sticking on one level.
		if (level.gametype == GT_SIEGE)
		{
			//hack to get the bots to spawn into seige games after the game has started
			if (g_entities[bs->client].client->sess.siegeDesiredTeam != SIEGETEAM_TEAM1
				&& g_entities[bs->client].client->sess.siegeDesiredTeam != SIEGETEAM_TEAM2)
			{
				//we're not on a team, go onto the best team available.
				g_entities[bs->client].client->sess.siegeDesiredTeam = PickTeam(bs->client);
			}

			select_best_siege_class(bs->client, qtrue);
		}

		if (!(g_entities[bs->client].client->pers.cmd.buttons & BUTTON_ATTACK))
		{
			//only tap the button if it's not currently being pressed
			trap->EA_Attack(bs->client);
		}
		return;
	}

	if (!bs->lastDeadTime)
	{
		//just spawned in?
		bs->lastDeadTime = level.time;
		bs->MiscBotFlags = 0;
		bs->orderEntity = NULL;
		bs->ordererNum = bs->client;
		VectorClear(bs->DestPosition);
		bs->DestIgnore = -1;
	}

	if (g_entities[bs->client].health < 1 || g_entities[bs->client].client->ps.pm_type == PM_DEAD)
	{
		bs->lastDeadTime = level.time;

		if (!bs->deathActivitiesDone && bs->lastHurt && bs->lastHurt->client && bs->lastHurt->s.number != bs->client)
		{
			bot_death_notify(bs);
			if (pass_loved_one_check(bs, bs->lastHurt))
			{
				//CHAT: Died
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Died", 0);
			}
			else if (!pass_loved_one_check(bs, bs->lastHurt) &&
				botstates[bs->lastHurt->s.number] &&
				pass_loved_one_check(botstates[bs->lastHurt->s.number], &g_entities[bs->client]))
			{
				//killed by a bot that I love, but that does not love me
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "KilledOnPurposeByLove", 0);
			}

			bs->deathActivitiesDone = 1;
		}

		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpCamping = NULL;
		bs->wpCampingTo = NULL;
		bs->wpStoreDest = NULL;
		bs->wpDestIgnoreTime = 0;
		bs->wpDestSwitchTime = 0;
		bs->wpSeenTime = 0;
		bs->wpDirection = 0;
		VectorClear(bs->lastDestPosition);
		clear_route(bs->botRoute);
		bs->tacticEntity = NULL;
		bs->objectiveType = 0;
		bs->MiscBotFlags = 0;

		if (rand() % 10 < 5 &&
			(!bs->doChat || bs->chatTime < level.time))
		{
			trap->EA_Attack(bs->client);
			if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_STAFF
				&& g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_DUAL)
			{
				Cmd_SaberAttackCycle_f(&g_entities[bs->client]); // we died lets change the saber style
			}
		}

		return;
	}

	// -----------------------------------------------------
   // SABER STYLE AUTO-SELECTION (Modern, Safe, Rule-Aligned)
   // -----------------------------------------------------

	qboolean dualSabers = qfalse;
	qboolean staffSaber = qfalse;

	// Detect dual sabers
	if (saber2 && saber2->model[0])
		dualSabers = qtrue;

	// Detect staff saber (more than one blade)
	if (saber1 && saber1->numBlades > 1)
		staffSaber = qtrue;

	// Do not change styles during unstable states
	if (bs->cur_ps.saberLockTime > level.time ||
		bs->cur_ps.saberInFlight ||
		bs->forceJumping > level.time ||
		bs->botChallengingTime > level.time)
	{
		return;
	}

	// Cooldown to prevent spam
	if (bs->saberStyleDebounce > level.time)
		return;

	// -----------------------------------------------------
	// Dual Sabers → SS_DUAL
	// -----------------------------------------------------
	if (dualSabers)
	{
		if (bs->cur_ps.fd.saberAnimLevel != SS_DUAL)
		{
			Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
		}
		return;
	}

	// -----------------------------------------------------
	// Staff Saber → SS_STAFF
	// -----------------------------------------------------
	if (staffSaber)
	{
		if (bs->cur_ps.fd.saberAnimLevel != SS_STAFF)
		{
			Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
		}
		return;
	}

	// -----------------------------------------------------
	// Single Saber → ensure valid single-saber style
	// -----------------------------------------------------
	if (!dualSabers && !staffSaber)
	{
		int style = bs->cur_ps.fd.saberAnimLevel;

		if (style != SS_FAST &&
			style != SS_TAVION &&
			style != SS_MEDIUM &&
			style != SS_STRONG &&
			style != SS_DESANN)
		{
			Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
		}
	}

	bot_check_speak(&g_entities[bs->client], qtrue);

	if (PM_InLedgeMove(bs->cur_ps.legsAnim))
	{
		//we're in a ledge move, just pull up for now
		trap->EA_MoveForward(bs->client);
		return;
	}

	if (bs->cur_ps.saberLockTime > level.time)
	{
		//bot is in a saber lock
		//AI cheats by knowing their enemy's fp level, if they're low on fP, try to super break finish them.
		if (g_entities[bs->cur_ps.saberLockEnemy].client->ps.fd.forcePower < 50)
		{
			trap->EA_Attack(bs->client);
		}
		if (g_entities[bs->cur_ps.saberLockEnemy].client->ps.fd.blockPoints < BLOCKPOINTS_HALF)
		{
			trap->EA_Attack(bs->client);
		}
		return;
	}

	VectorCopy(bs->goalAngles, pre_frame_g_angles);

	bs->doAttack = 0;
	bs->doAltAttack = 0;
	advanced_scanfor_enemies(bs);

	// If carrying an objective, override everything
	if (carrying_cap_objective(bs))
	{
		bs->currentTactic = BOTORDER_OBJECTIVE;
		bs->objectiveType = OT_CAPTURE;
		return;
	}

	//determine which tactic we want to use.
	// If no explicit bot order, choose based on gametype or situation
	if (bs->botOrder == BOTORDER_NONE)
	{
		// If we already have a tactic, keep it
		if (bs->currentTactic)
		{
			// nothing to do
		}
		else
		{
			switch (level.gametype)
			{
			case GT_SIEGE:
				bs->currentTactic = BOTORDER_OBJECTIVE;
				break;

			case GT_CTF:
			case GT_CTY:
				determine_ctf_goal(bs);
				break;

			case GT_SINGLE_PLAYER:
			{
				gentity_t* player = find_closest_human_player(bs->origin, NPCTEAM_PLAYER);

				if (player)
				{
					bs->currentTactic = BOTORDER_DEFEND;
					bs->tacticEntity = player;
				}
				else
				{
					bs->currentTactic = BOTORDER_SEARCHANDDESTROY;
					bs->tacticEntity = NULL;
				}
				break;
			}

			case GT_JEDIMASTER:
				bs->currentTactic = BOTORDER_JEDIMASTER;
				break;

			default:
				if (bs->isSquadLeader)
					commander_bot_ai(bs);
				else
					bot_do_teamplay_ai(bs);
				break;
			}
		}
	}
	else
	{
		// We have a botOrder → use teamplay/commander logic
		if (bs->isSquadLeader)
			commander_bot_ai(bs);
		else
			bot_do_teamplay_ai(bs);
	}

	// If no current enemy, clear visibility flag
	if (!bs->currentEnemy)
	{
		bs->frame_Enemy_Vis = 0;
	}

	// Validate revenge enemy
	if (bs->revengeEnemy && bs->revengeEnemy->client)
	{
		const int conn = bs->revengeEnemy->client->pers.connected;
		if (conn != CON_CONNECTED && conn != CON_CONNECTING)
		{
			bs->revengeEnemy = NULL;
			bs->revengeHateLevel = 0;
		}
	}

	// Validate current enemy
	if (bs->currentEnemy && bs->currentEnemy->client)
	{
		const int conn = bs->currentEnemy->client->pers.connected;
		if (conn != CON_CONNECTED && conn != CON_CONNECTING)
		{
			bs->currentEnemy = NULL;
		}
	}

	fj_halt = 0;
	use_the_force = 0;
	forceHostile = 0;

#ifndef FORCEJUMP_INSTANTMETHOD
	// Charging a force jump overrides everything
	if (bs->forceJumpChargeTime > level.time)
	{
		use_the_force = 1;
		forceHostile = 0;
	}
#endif

	// Must have a visible enemy to consider offensive powers
#if !defined(FORCEJUMP_INSTANTMETHOD)
	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis &&
		bs->forceJumpChargeTime < level.time)
#else
	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis)
#endif
	{
		// Compute angle to enemy once
		vec3_t toEnemyAngles;
		vec3_t toEnemyVec;

		VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, toEnemyVec);
		vectoangles(toEnemyVec, toEnemyAngles);

		// -----------------------------
		// PRIORITY 1: PUSH (escape grip / emergency)
		// -----------------------------
		if (bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH) &&
			(bs->doForcePush > level.time ||
				bs->cur_ps.fd.forceGripBeingGripped > level.time) &&
			level.clients[bs->client].ps.fd.forcePower >
			forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH])
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
			use_the_force = 1;
			forceHostile = 1;
		}

		// -----------------------------
		// DARK SIDE LOGIC
		// -----------------------------
		else if (bs->cur_ps.fd.forceSide == FORCE_DARKSIDE)
		{
			// Maintain active grip
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) &&
				(bs->cur_ps.fd.forcePowersActive & (1 << FP_GRIP)) &&
				in_field_of_vision(bs->viewangles, 50, toEnemyAngles))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
				use_the_force = 1;
				forceHostile = 1;
			}
			// Lightning
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_LIGHTNING)) &&
				bs->frame_Enemy_Len < FORCE_LIGHTNING_RADIUS &&
				level.clients[bs->client].ps.fd.forcePower > 50 &&
				in_field_of_vision(bs->viewangles, 50, toEnemyAngles))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_LIGHTNING;
				use_the_force = 1;
				forceHostile = 1;
			}
			// Grip (new attempt)
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) &&
				bs->frame_Enemy_Len < MAX_GRIP_DISTANCE &&
				level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP] &&
				in_field_of_vision(bs->viewangles, 50, toEnemyAngles))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
				use_the_force = 1;
				forceHostile = 1;
			}
			// Rage (low health)
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_RAGE)) &&
				g_entities[bs->client].health < 25 &&
				level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_RAGE]][FP_RAGE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
				use_the_force = 1;
				forceHostile = 0;
			}
			// Rage (bowcaster special case)
			else if (bs->cur_ps.weapon == WP_BOWCASTER &&
				(bs->cur_ps.fd.forcePowersKnown & (1 << FP_RAGE)) &&
				g_entities[bs->client].health < 75 &&
				level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_RAGE]][FP_RAGE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
				use_the_force = 1;
				forceHostile = 0;
			}
			// Drain
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) &&
				bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE &&
				level.clients[bs->client].ps.fd.forcePower > 50 &&
				in_field_of_vision(bs->viewangles, 50, toEnemyAngles) &&
				bs->currentEnemy->client->ps.fd.forcePower > 10 &&
				bs->currentEnemy->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
				use_the_force = 1;
				forceHostile = 1;
			}
		}

		// -----------------------------
		// LIGHT SIDE LOGIC
		// -----------------------------
		else if (bs->cur_ps.fd.forceSide == FORCE_LIGHTSIDE)
		{
			// Absorb to escape grip
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) &&
				bs->cur_ps.fd.forceGripCripple &&
				level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
				use_the_force = 1;
				forceHostile = 0;
			}
			// Absorb lightning
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) &&
				bs->cur_ps.electrifyTime >= level.time &&
				level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
				use_the_force = 1;
				forceHostile = 0;
			}
			// Mind trick
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_TELEPATHY)) &&
				bs->frame_Enemy_Len < MAX_TRICK_DISTANCE &&
				level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY] &&
				in_field_of_vision(bs->viewangles, 50, toEnemyAngles) &&
				!(bs->currentEnemy->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TELEPATHY;
				use_the_force = 1;
				forceHostile = 1;
			}
			// Absorb (low health vs dark side)
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) &&
				g_entities[bs->client].health < 75 &&
				bs->currentEnemy->client->ps.fd.forceSide == FORCE_DARKSIDE &&
				level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
				use_the_force = 1;
				forceHostile = 0;
			}
			// Protect (low health)
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PROTECT)) &&
				g_entities[bs->client].health < 35 &&
				level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PROTECT]][FP_PROTECT])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PROTECT;
				use_the_force = 1;
				forceHostile = 0;
			}
		}

		// -----------------------------
		// LOST SABER LOGIC
		// -----------------------------
		else if (bs->cur_ps.saberInFlight && !bs->cur_ps.saberEntityNum)
		{
			const gentity_t* saberEnt =
				&g_entities[g_entities[bs->client].client->saberStoredIndex];

			if (saberEnt->s.pos.trType == TR_STATIONARY)
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SABERTHROW;
				use_the_force = 1;
				forceHostile = 0;
			}
		}

		// -----------------------------
		// NEUTRAL POWERS
		// -----------------------------
		if (!use_the_force)
		{
			// Push to escape grip
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) &&
				bs->cur_ps.fd.forceGripBeingGripped > level.time &&
				level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH] &&
				in_field_of_vision(bs->viewangles, 50, toEnemyAngles))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				use_the_force = 1;
				forceHostile = 1;
			}
			// Speed (low health)
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_SPEED)) &&
				g_entities[bs->client].health < 25 &&
				level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SPEED]][FP_SPEED])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SPEED;
				use_the_force = 1;
				forceHostile = 0;
			}
			// See (counter mind trick)
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_SEE)) &&
				bot_mind_tricked(bs->client, bs->currentEnemy->s.number) &&
				level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SEE]][FP_SEE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SEE;
				use_the_force = 1;
				forceHostile = 0;
			}
			// Push/Pull spam logic
			else
			{
				const qboolean doPull = (rand() % 10 < 5);

				if (bs->doForcePushPullSpamTime > level.time)
				{
					if (doPull &&
						(bs->cur_ps.fd.forcePowersKnown & (1 << FP_PULL)) &&
						bs->frame_Enemy_Len < 256 &&
						level.clients[bs->client].ps.fd.forcePower > 75 &&
						in_field_of_vision(bs->viewangles, 50, toEnemyAngles))
					{
						level.clients[bs->client].ps.fd.forcePowerSelected = FP_PULL;
						use_the_force = 1;
						forceHostile = 1;
					}
				}
				else
				{
					if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) &&
						bs->frame_Enemy_Len < 256 &&
						level.clients[bs->client].ps.fd.forcePower > 75 &&
						in_field_of_vision(bs->viewangles, 50, toEnemyAngles))
					{
						level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
						use_the_force = 1;
						forceHostile = 1;
					}
				}

				bs->doForcePushPullSpamTime = level.time + 500;
			}
		}
	}

	// ------------------------------------------------------------
	// NON-HOSTILE POWERS (only if no offensive power was chosen)
	// ------------------------------------------------------------
	if (!use_the_force)
	{
		const int fpLevelHeal = bs->cur_ps.fd.forcePowerLevel[FP_HEAL];
		const int fpCurrent = level.clients[bs->client].ps.fd.forcePower;
		const int fpNeeded = forcePowerNeeded[fpLevelHeal][FP_HEAL];
		const int health = g_entities[bs->client].health;

		const qboolean canHeal =
			(bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL)) &&
			fpLevelHeal > FORCE_LEVEL_1 &&
			fpCurrent > fpNeeded &&
			health < 50;

		if (canHeal)
		{
			// Heal normally
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			use_the_force = 1;
			forceHostile = 0;
		}
		else if (canHeal &&
			!bs->currentEnemy &&
			bs->isCamping > level.time)
		{
			// Heal while meditating during camping
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			use_the_force = 1;
			forceHostile = 0;
		}
	}

	// ------------------------------------------------------------
	// VALIDATE HOSTILE FORCE POWER
	// ------------------------------------------------------------
	if (use_the_force && forceHostile)
	{
		if (!bs->currentEnemy ||
			!bs->currentEnemy->client ||
			!ForcePowerUsableOn(&g_entities[bs->client],
				bs->currentEnemy,
				level.clients[bs->client].ps.fd.forcePowerSelected))
		{
			use_the_force = 0;
			forceHostile = 0;
		}
	}

	doing_fallback = 0;

	bs->deathActivitiesDone = 0;

	if (bot_use_inventory_item(bs))
	{
		if (rand() % 10 < 5)
		{
			trap->EA_Use(bs->client);
		}
	}

	if (g_AllowBotBuyItem.integer)
	{
		if (g_entities[bs->client].health <= 50)
		{
			bot_buy_item(bs, "health");
		}
		else if (bs->cur_ps.stats[STAT_ARMOR] <= 50)
		{
			bot_buy_item(bs, "shield");
		}
		else
		{
			switch (rand() % 35)
			{
			case 5: bot_buy_item(bs, "seeker");
			case 10: bot_buy_item(bs, "blaster");
			case 15: bot_buy_item(bs, "launcher");
			case 16: bot_buy_item(bs, "concussion");
			case 20: bot_buy_item(bs, "bowcaster");
			default:
				bot_buy_item(bs, "sentry");
			}
		}
	}

	if (bs->cur_ps.ammo[weaponData[bs->cur_ps.weapon].ammoIndex] < weaponData[bs->cur_ps.weapon].energyPerShot)
	{
		if (g_AllowBotBuyItem.integer)
		{
			if (bot_buy_item(bs, "ammo"))
			{
				return;
			}
		}
		else
		{
			if (bot_try_another_weapon(bs))
			{
				return;
			}
		}
	}
	else
	{
		int sel_result = 0;
		if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number &&
			bs->frame_Enemy_Vis && bs->forceWeaponSelect)
		{
			bs->forceWeaponSelect = 0;
		}

		if (bs->plantContinue > level.time)
		{
			bs->doAttack = 1;
			bs->destinationGrabTime = 0;
		}

		if (!bs->forceWeaponSelect && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
		{
			bs->forceWeaponSelect = WP_DET_PACK;
		}

		if (bs->forceWeaponSelect)
		{
			sel_result = bot_select_choice_weapon(bs, bs->forceWeaponSelect, 1);
		}

		if (sel_result)
		{
			if (sel_result == 2)
			{
				//newly selected
				return;
			}
		}
		else if (bot_select_ideal_weapon(bs))
		{
			return;
		}
	}

	reaction = bs->skills.reflex / bs->settings.skill;

	if (reaction < 0)
	{
		reaction = 0;
	}
	if (reaction > 2000)
	{
		reaction = 2000;
	}

	if (!bs->currentEnemy)
	{
		bs->timeToReact = level.time + reaction;
	}

	if (bs->cur_ps.weapon == WP_DET_PACK && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
	{
		bs->doAltAttack = 1;
	}

	if (bs->wpCamping)
	{
		if (bs->isCamping < level.time)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}

		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}
	}

	if (bs->wpCurrent &&
		(bs->wpSeenTime < level.time || bs->wpTravelTime < level.time))
	{
		bs->wpCurrent = NULL;
	}

	if (bs->currentEnemy)
	{
		if (bs->enemySeenTime < level.time ||
			!pass_standard_enemy_checks(bs, bs->currentEnemy))
		{
			if (bs->revengeEnemy == bs->currentEnemy &&
				bs->currentEnemy->health < 1 &&
				bs->lastAttacked && bs->lastAttacked == bs->currentEnemy)
			{
				bs->chatObject = bs->revengeEnemy;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "KilledHatedOne", 1);
				bs->revengeEnemy = NULL;
				bs->revengeHateLevel = 0;
			}
			else if (bs->currentEnemy->health < 1 && pass_loved_one_check(bs, bs->currentEnemy) &&
				bs->lastAttacked && bs->lastAttacked == bs->currentEnemy)
			{
				bs->chatObject = bs->currentEnemy;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Killed", 0);
			}

			bs->currentEnemy = NULL;
		}
	}

	if (bot_honorableduelacceptance.integer)
	{
		if (bs->currentEnemy && bs->currentEnemy->client &&
			g_privateDuel.integer &&
			bs->frame_Enemy_Vis &&
			bs->frame_Enemy_Len < 400)

		{
			vec3_t e_ang_vec;

			VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, e_ang_vec);

			if (in_field_of_vision(bs->viewangles, 100, e_ang_vec))
			{
				if (bs->currentEnemy->client->ps.duelIndex == bs->client &&
					bs->currentEnemy->client->ps.duelTime > level.time &&
					!bs->cur_ps.duelInProgress)
				{
					Cmd_EngageDuel_f(&g_entities[bs->client]);
				}

				bs->doAttack = 0;
				bs->doAltAttack = 0;
				bs->doBotKick = 0;
				bs->botChallengingTime = level.time + 100;
			}
		}
	}

	if (!bs->wpCurrent)
	{
		int wp;
		wp = get_nearest_visible_wp(bs->origin, bs->client);

		if (wp != -1)
		{
			bs->wpCurrent = gWPArray[wp];
			bs->wpSeenTime = level.time + 1500;
			bs->wpTravelTime = level.time + 10000; //never take more than 10 seconds to travel to a waypoint
		}
	}

	if (bs->enemySeenTime < level.time || !bs->frame_Enemy_Vis || !bs->currentEnemy ||
		bs->currentEnemy)
	{
		int enemy;
		enemy = scan_for_enemies(bs);

		if (enemy != -1)
		{
			bs->currentEnemy = &g_entities[enemy];
			bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
		}
	}

	if (!bs->squadLeader && !bs->isSquadLeader)
	{
		bot_scan_for_leader(bs);
	}

	if (!bs->squadLeader && bs->squadCannotLead < level.time)
	{
		//if still no leader after scanning, then become a squad leader
		bs->isSquadLeader = 1;
	}

	if (bs->isSquadLeader && bs->squadLeader)
	{
		//we don't follow anyone if we are a leader
		bs->squadLeader = NULL;
	}

	//ESTABLISH VISIBILITIES AND DISTANCES FOR THE WHOLE FRAME HERE
	if (bs->wpCurrent)
	{
		int vis_result = 0;
		if (RMG.integer)
		{
			//this is somewhat hacky, but in RMG we don't really care about vertical placement because points are scattered across only the terrain.
			vec3_t vec_b, vec_c;

			vec_b[0] = bs->origin[0];
			vec_b[1] = bs->origin[1];
			vec_b[2] = bs->origin[2];

			vec_c[0] = bs->wpCurrent->origin[0];
			vec_c[1] = bs->wpCurrent->origin[1];
			vec_c[2] = vec_b[2];

			VectorSubtract(vec_c, vec_b, a);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
		}
		bs->frame_Waypoint_Len = VectorLength(a);

		vis_result = wp_org_visible(&g_entities[bs->client], bs->origin, bs->wpCurrent->origin, bs->client);

		if (vis_result == 2)
		{
			bs->frame_Waypoint_Vis = 0;
			bs->wpSeenTime = 0;
			bs->wpDestination = NULL;
			bs->wpDestIgnoreTime = level.time + 5000;

			if (bs->wpDirection)
			{
				bs->wpDirection = 0;
			}
			else
			{
				bs->wpDirection = 1;
			}
		}
		else if (vis_result)
		{
			bs->frame_Waypoint_Vis = 1;
		}
		else
		{
			bs->frame_Waypoint_Vis = 0;
		}
	}

	if (bs->currentEnemy)
	{
		vec3_t eorg;
		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, eorg);
			eorg[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->s.origin, eorg);
		}

		VectorSubtract(eorg, bs->eye, a);
		bs->frame_Enemy_Len = VectorLength(a);

		if (org_visible(bs->eye, eorg, bs->client))
		{
			bs->frame_Enemy_Vis = 1;
			VectorCopy(eorg, bs->lastEnemySpotted);
			VectorCopy(bs->origin, bs->hereWhenSpotted);
			bs->lastVisibleEnemyIndex = bs->currentEnemy->s.number;
			bs->hitSpotted = 0;
		}
		else
		{
			bs->frame_Enemy_Vis = 0;
		}
	}
	else
	{
		bs->lastVisibleEnemyIndex = ENTITYNUM_NONE;
	}
	//END

	if (bs->frame_Enemy_Vis)
	{
		bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
	}

	if (bs->wpCurrent)
	{
		int goal_wp_index;
		int wp_touch_dist = BOT_WPTOUCH_DISTANCE;
		wp_constant_routine(bs);

		if (!bs->wpCurrent)
		{
			//WPConstantRoutine has the ability to nullify the waypoint if it fails certain checks, so..
			return;
		}

		if (bs->wpCurrent->flags & WPFLAG_WAITFORFUNC)
		{
			if (!check_for_func(bs->wpCurrent->origin, -1))
			{
				bs->beStill = level.time + 500; //no func brush under.. wait
			}
		}
		if (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
		{
			if (check_for_func(bs->wpCurrent->origin, -1))
			{
				bs->beStill = level.time + 500; //func brush under.. wait
			}
		}

		if (bs->frame_Waypoint_Vis || bs->wpCurrent->flags & WPFLAG_NOVIS)
		{
			if (RMG.integer)
			{
				bs->wpSeenTime = level.time + 5000;
				//if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
			else
			{
				bs->wpSeenTime = level.time + 1500;
				//if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
		}
		VectorCopy(bs->wpCurrent->origin, bs->goalPosition);
		if (bs->wpDirection)
		{
			goal_wp_index = bs->wpCurrent->index - 1;
		}
		else
		{
			goal_wp_index = bs->wpCurrent->index + 1;
		}

		if (bs->wpCamping)
		{
			VectorSubtract(bs->wpCampingTo->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);

			VectorSubtract(bs->origin, bs->wpCamping->origin, a);
			if (VectorLength(a) < 64)
			{
				VectorCopy(bs->wpCamping->origin, bs->goalPosition);
				bs->beStill = level.time + 1000;

				if (!bs->campStanding)
				{
					bs->duckTime = level.time + 1000;
				}
			}
		}
		else if (gWPArray[goal_wp_index] && gWPArray[goal_wp_index]->inuse &&
			!(gLevelFlags & LEVELFLAG_NOPOINTPREDICTION))
		{
			VectorSubtract(gWPArray[goal_wp_index]->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}

		if (bs->destinationGrabTime < level.time)
		{
			get_ideal_destination(bs);
		}

		if (bs->wpCurrent && bs->wpDestination)
		{
			if (total_trail_distance(bs->wpCurrent->index, bs->wpDestination->index) == -1)
			{
				bs->wpDestination = NULL;
				bs->destinationGrabTime = level.time + 10000;
			}
		}

		if (RMG.integer)
		{
			if (bs->frame_Waypoint_Vis)
			{
				if (bs->wpCurrent && !bs->wpCurrent->flags)
				{
					wp_touch_dist *= 3;
				}
			}
		}

		if (bs->frame_Waypoint_Len < wp_touch_dist || RMG.integer && bs->frame_Waypoint_Len < wp_touch_dist * 2)
		{
			int desired_index;
			WPTouchRoutine(bs);

			if (!bs->wpDirection)
			{
				desired_index = bs->wpCurrent->index + 1;
			}
			else
			{
				desired_index = bs->wpCurrent->index - 1;
			}

			if (gWPArray[desired_index] &&
				gWPArray[desired_index]->inuse &&
				desired_index < gWPNum &&
				desired_index >= 0 &&
				pass_way_check(bs, desired_index))
			{
				bs->wpCurrent = gWPArray[desired_index];
			}
			else
			{
				if (bs->wpDestination)
				{
					bs->wpDestination = NULL;
					bs->destinationGrabTime = level.time + 10000;
				}

				if (bs->wpDirection)
				{
					bs->wpDirection = 0;
				}
				else
				{
					bs->wpDirection = 1;
				}
			}
		}
	}
	else //We can't find a waypoint, going to need a fallback routine.
	{
		if (bs->cur_ps.weapon == WP_SABER)
		{
			doing_fallback = saber_bot_fallback_navigation(bs);
		}
		else
		{
			if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
			{
				// Jetpacker.. Jetpack ON
				bs->jumpTime = level.time + 1500;
				bs->jDelay = 0;
				bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
				bs->cur_ps.eFlags |= EF_JETPACK_FLAMING; //going up
				bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
			}
			doing_fallback = gunner_bot_fallback_navigation(bs);
		}
	}

	// Jumping logic (clean, rule-compliant)
	if (bs->currentEnemy && bs->entityNum < MAX_CLIENTS)
	{
		if (g_entities[bs->entityNum].health > 0 &&
			bs->currentEnemy->health > 0)
		{
			vec3_t dir = { 0, 0, 0 };
			float xy;
			qboolean will_fall = qfalse;

			// Fall prediction
			if (next_bot_fallcheck[bs->entityNum] < level.time)
			{
				vec3_t fwd;
				will_fall = check_fall_by_vectors(bs->origin, fwd, &g_entities[bs->entityNum]);
				next_bot_fallcheck[bs->entityNum] = level.time + 50;
				bot_will_fall[bs->entityNum] = will_fall;
			}
			else
			{
				will_fall = bot_will_fall[bs->entityNum];
			}

			// Horizontal distance
			VectorSubtract(bs->currentEnemy->r.currentOrigin, bs->origin, dir);
			dir[2] = 0;
			xy = VectorNormalize(dir);

			// Decide if we should jump
			if (bot_should_jump_to_enemy(bs, xy, will_fall))
			{
				bs->BOTjumpState = JS_FACING;
				ai_mod_jump(bs);
				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->BOTjumpState >= JS_CROUCHING)
			{
				// Continue any jumps already in progress
				ai_mod_jump(bs);
			}
		}
	}

	if (!VectorCompare(vec3_origin, move_dir))
	{
		trap->EA_Move(bs->client, move_dir, 5000);
	}

	if (RMG.integer)
	{
		//for RMG if the bot sticks around an area too long, jump around randomly some to spread to a new area (horrible hacky method)
		vec3_t v_sub_dif;

		VectorSubtract(bs->origin, bs->lastSignificantAreaChange, v_sub_dif);
		if (VectorLength(v_sub_dif) > 1500)
		{
			VectorCopy(bs->origin, bs->lastSignificantAreaChange);
			bs->lastSignificantChangeTime = level.time + 20000;
		}

		if (bs->lastSignificantChangeTime < level.time)
		{
			bs->iHaveNoIdeaWhereIAmGoing = level.time + 17000;
		}
	}

	if (bs->iHaveNoIdeaWhereIAmGoing > level.time && !bs->currentEnemy)
	{
		// Keep facing what we were facing last frame
		VectorCopy(pre_frame_g_angles, bs->goalAngles);

		// Reset waypoint logic
		bs->wpCurrent = NULL;
		bs->wpSwitchTime = level.time + 150;

		if (bs->cur_ps.weapon == WP_SABER)
		{
			// Saber bots: calm fallback, no forced jumps
			doing_fallback = saber_bot_fallback_navigation(bs);
			bs->BOTjumpState = JS_WAITING;
		}
		else
		{
			// Gunner bots: prefer jetpack if available, but don't fake jump timers
			if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
			{
				bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
				bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			}

			doing_fallback = gunner_bot_fallback_navigation(bs);
		}

		// Do NOT force jump timers here anymore
		bs->jumpTime = 0;
		bs->jumpHoldTime = 0;
		bs->jDelay = 0;

		// Shorter, sane debounce
		bs->lastSignificantChangeTime = level.time + 3000;
	}

	if (bs->wpCurrent && RMG.integer)
	{
		qboolean do_j = qfalse;

		if (bs->wpCurrent->origin[2] - 192 > bs->origin[2])
		{
			do_j = qtrue;
		}
		else if (bs->wpTravelTime - level.time < 5000 && bs->wpCurrent->origin[2] - 64 > bs->origin[2])
		{
			do_j = qtrue;
		}
		else if (bs->wpTravelTime - level.time < 7000 && bs->wpCurrent->flags & WPFLAG_RED_FLAG)
		{
			if (level.time - bs->jumpTime > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if (bs->wpTravelTime - level.time < 7000 && bs->wpCurrent->flags & WPFLAG_BLUE_FLAG)
		{
			if (level.time - bs->jumpTime > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if (bs->wpCurrent->index > 0)
		{
			if (bs->wpTravelTime - level.time < 7000)
			{
				if (gWPArray[bs->wpCurrent->index - 1]->flags & WPFLAG_RED_FLAG ||
					gWPArray[bs->wpCurrent->index - 1]->flags & WPFLAG_BLUE_FLAG)
				{
					if (level.time - bs->jumpTime > 200)
					{
						bs->jumpTime = level.time + 100;
						bs->jumpHoldTime = level.time + 100;
						bs->jDelay = 0;
					}
				}
			}
		}

		if (do_j)
		{
			bs->jumpTime = level.time + 1500;
			bs->jumpHoldTime = level.time + 1500;
			bs->jDelay = 0;
		}
	}

	if (doing_fallback)
	{
		bs->doingFallback = qtrue;
	}
	else
	{
		bs->doingFallback = qfalse;
	}

	if (bs->timeToReact < level.time && bs->currentEnemy && bs->enemySeenTime > level.time + (ENEMY_FORGET_MS -
		ENEMY_FORGET_MS * 0.2))
	{
		vec3_t headlevel;
		if (bs->frame_Enemy_Vis)
		{
			combat_bot_ai(bs);
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
		{
			//keep charging in case we see him again before we lose track of him
			bs->doAltAttack = 1;
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING)
		{
			//keep charging in case we see him again before we lose track of him
			bs->doAttack = 1;
		}

		if (bs->destinationGrabTime > level.time + 100)
		{
			bs->destinationGrabTime = level.time + 100;
			//assures that we will continue staying within a general area of where we want to be in a combat situation
		}

		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
			headlevel[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
		}

		if (!bs->frame_Enemy_Vis)
		{
			if (org_visible(bs->eye, bs->lastEnemySpotted, -1))
			{
				VectorCopy(bs->lastEnemySpotted, headlevel);
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);

				if (bs->cur_ps.weapon == WP_FLECHETTE &&
					bs->cur_ps.weaponstate == WEAPON_READY &&
					bs->currentEnemy && bs->currentEnemy->client)
				{
					float m_len;
					m_len = VectorLength(a) > 128;
					if (m_len > 128 && m_len < 1024)
					{
						VectorSubtract(bs->currentEnemy->client->ps.origin, bs->lastEnemySpotted, a);

						if (VectorLength(a) < 300)
						{
							bs->doAltAttack = 1;
						}
					}
				}
			}
		}
		else
		{
			float b_lead_amount;
			b_lead_amount = bot_weapon_can_lead(bs);
			if (bs->skills.accuracy / bs->settings.skill <= 8 &&
				b_lead_amount)
			{
				bot_aim_leading(bs, headlevel, b_lead_amount);
			}
			else
			{
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);
			}

			bot_aim_offset_goal_angles(bs);
		}
	}

	if (bs->cur_ps.saberInFlight)
	{
		bs->saberThrowTime = level.time + Q_irand(4000, 10000);
	}

	if (bs->currentEnemy)
	{
		const int weapRange = bot_get_weapon_range(bs);

		// -----------------------------
		// SABER RANGE
		// -----------------------------
		if (weapRange == BWEAPONRANGE_SABER)
		{
			int saber_range = SABER_ATTACK_RANGE;

			vec3_t toEnemy;
			VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, toEnemy);
			vectoangles(toEnemy, toEnemy);

			// Randomized "power" window for style biasing
			if (bs->saberPowerTime < level.time)
			{
				bs->saberPower = (Q_irand(1, 10) <= 5);
				bs->saberPowerTime = level.time + Q_irand(3000, 15000);
			}

			// Style selection (don’t touch staff/dual)
			if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_STAFF &&
				g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_DUAL)
			{
				const int enemyBP = bs->currentEnemy->client->ps.fd.blockPoints;
				const int myOffense = g_entities[bs->client].client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
				const int curStyle = g_entities[bs->client].client->ps.fd.saberAnimLevel;

				if (enemyBP > BLOCKPOINTS_FULL && myOffense > 2)
				{
					// High BP enemy, we have strong available
					if (bs->saberPower &&
						curStyle != SS_STRONG &&
						curStyle != SS_DESANN)
					{
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
				else if (enemyBP > BLOCKPOINTS_HALF && myOffense > 1)
				{
					// Mid BP → medium
					if (curStyle != SS_MEDIUM)
					{
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
				else
				{
					// Low BP → fast
					if (curStyle != SS_FAST &&
						curStyle != SS_TAVION)
					{
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
			}

			if (level.gametype == GT_SINGLE_PLAYER)
			{
				saber_range *= 3;
			}

			// Core saber combat
			if (bs->frame_Enemy_Len <= saber_range)
			{
				// Walk in saber combat
				bs->doWalk = qtrue;

				Enhanced_saber_combat_handling(bs);

				if (bs->frame_Enemy_Len < 80.0f)
				{
					meleestrafe = 1;
				}
			}
			// Saber throw (mid‑range, not spammy)
			else if (bs->saberThrowTime < level.time &&
				!bs->cur_ps.saberInFlight &&
				(bs->cur_ps.fd.forcePowersKnown & (1 << FP_SABERTHROW)) &&
				in_field_of_vision(bs->viewangles, 30, toEnemy) &&
				bs->frame_Enemy_Len > 512.0f &&
				bs->frame_Enemy_Len < BOT_SABER_THROW_RANGE)
			{
				bs->doAltAttack = 1;
				bs->doAttack = 0;
			}
			else if (bs->cur_ps.saberInFlight &&
				bs->frame_Enemy_Len > 300.0f &&
				bs->frame_Enemy_Len < BOT_SABER_THROW_RANGE)
			{
				bs->doAltAttack = 1;
				bs->doAttack = 0;
			}
		}
		// -----------------------------
		// MELEE RANGE
		// -----------------------------
		else if (weapRange == BWEAPONRANGE_MELEE)
		{
			if (bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE)
			{
				melee_combat_handling(bs);
				meleestrafe = 1;
			}
		}
	}

	if (doing_fallback && bs->currentEnemy) //just stand and fire if we have no idea where we are
	{
		VectorCopy(bs->origin, bs->goalPosition);
	}

	// ---------------------------------------------
// FORCE JUMP HALT CHECK
// ---------------------------------------------
	if (bs->forceJumping > level.time)
	{
		vec3_t horizDiff = { 0 };
		vec3_t origin2D = { bs->origin[0], bs->origin[1], 0 };
		vec3_t goal2D = { bs->goalPosition[0], bs->goalPosition[1], 0 };

		VectorSubtract(origin2D, goal2D, horizDiff);

		if (VectorLength(horizDiff) < 32.0f)
		{
			fj_halt = 1;
		}
	}

	// ---------------------------------------------
	// FALL‑SAFETY SYSTEM
	// ---------------------------------------------
	qboolean longFall =
		(bs->cur_ps.lastOnGround + 300 < level.time) &&
		(!bs->cur_ps.fd.forceJumpZStart ||
			bs->origin[2] < bs->cur_ps.fd.forceJumpZStart);

	if (longFall)
	{
		float speed = VectorLength(bs->cur_ps.velocity);

		// Clean, correct logic:
		// High fall speed OR extremely high velocity
		qboolean dangerousFall =
			(speed >= (100.0f + g_entities[bs->client].health)) ||
			(speed >= 700.0f);

		if (dangerousFall)
		{
			// Prefer saber or melee for roll landing
			if (bs->virtualWeapon != WP_SABER &&
				bs->virtualWeapon != WP_MELEE)
			{
				// Try saber first
				if (!bot_select_choice_weapon(bs, WP_SABER, 1))
				{
					// fallback to melee
					bot_select_choice_weapon(bs, WP_MELEE, 1);
				}
			}

			// Only roll if we actually have a roll‑capable weapon
			if (bs->virtualWeapon == WP_SABER ||
				bs->virtualWeapon == WP_MELEE)
			{
				bs->duckTime = level.time + 300;

				// If not moving, push forward to avoid stiff landing
				if (!bs->lastucmd.forwardmove &&
					!bs->lastucmd.rightmove)
				{
					trap->EA_MoveForward(bs->client);
				}
			}
		}
	}

	if (bs->doChat && bs->chatTime > level.time && (!bs->currentEnemy || !bs->frame_Enemy_Vis))
	{
		return;
	}
	if (bs->doChat && bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		bs->doChat = 0; //do we want to keep the bot waiting to chat until after the enemy is gone?
		bs->chatTeam = 0;
	}
	else if (bs->doChat && bs->chatTime <= level.time)
	{
		if (bs->chatTeam)
		{
			trap->EA_SayTeam(bs->client, bs->currentChat);
			bs->chatTeam = 0;
		}
		else
		{
			trap->EA_Say(bs->client, bs->currentChat);
		}
		if (bs->doChat == 2)
		{
			bot_reply_greetings(bs);
		}
		bs->doChat = 0;
	}

	ctf_flag_movement(bs);

	if (bs->shootGoal &&
		bs->shootGoal->health > 0 && bs->shootGoal->takedamage)
	{
		vec3_t dif;
		dif[0] = (bs->shootGoal->r.absmax[0] + bs->shootGoal->r.absmin[0]) / 2;
		dif[1] = (bs->shootGoal->r.absmax[1] + bs->shootGoal->r.absmin[1]) / 2;
		dif[2] = (bs->shootGoal->r.absmax[2] + bs->shootGoal->r.absmin[2]) / 2;

		if (!bs->currentEnemy || bs->frame_Enemy_Len > 256)
		{
			//if someone is close then don't stop shooting them for this
			VectorSubtract(dif, bs->eye, a);
			vectoangles(a, a);
			VectorCopy(a, bs->goalAngles);

			if (in_field_of_vision(bs->viewangles, 30, a) &&
				entity_visible_box(bs->origin, NULL, NULL, dif, bs->client, bs->shootGoal->s.number))
			{
				bs->doAttack = 1;
			}
		}
	}

	if (bs->cur_ps.hasDetPackPlanted)
	{
		//check if our enemy gets near it and detonate if he does
		bot_check_det_packs(bs);
	}
	else if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number && !bs->frame_Enemy_Vis && bs->
		plantTime < level.time &&
		!bs->doAttack && !bs->doAltAttack)
	{
		VectorSubtract(bs->origin, bs->hereWhenSpotted, a);

		if (bs->plantDecided > level.time || bs->frame_Enemy_Len < BOT_PLANT_DISTANCE * 2 && VectorLength(a) <
			BOT_PLANT_DISTANCE)
		{
			int det_select = 0;
			int mine_select = 0;
			mine_select = bot_select_choice_weapon(bs, WP_TRIP_MINE, 0);
			det_select = bot_select_choice_weapon(bs, WP_DET_PACK, 0);
			if (bs->cur_ps.hasDetPackPlanted)
			{
				det_select = 0;
			}

			if (bs->plantDecided > level.time && bs->forceWeaponSelect &&
				bs->cur_ps.weapon == bs->forceWeaponSelect)
			{
				bs->doAttack = 1;
				bs->plantDecided = 0;
				bs->plantTime = level.time + BOT_PLANT_INTERVAL;
				bs->plantContinue = level.time + 500;
				bs->beStill = level.time + 500;
			}
			else if (mine_select || det_select)
			{
				if (bot_surface_near(bs))
				{
					if (!mine_select)
					{
						//if no mines use detpacks, otherwise use mines
						mine_select = WP_DET_PACK;
					}
					else
					{
						mine_select = WP_TRIP_MINE;
					}

					det_select = bot_select_choice_weapon(bs, mine_select, 1);

					if (det_select && det_select != 2)
					{
						//We have it and it is now our weapon
						bs->plantDecided = level.time + 1000;
						bs->forceWeaponSelect = mine_select;
						return;
					}
					if (det_select == 2)
					{
						bs->forceWeaponSelect = mine_select;
						return;
					}
				}
			}
		}
	}
	else if (bs->plantContinue < level.time)
	{
		bs->forceWeaponSelect = 0;
	}

	if (level.gametype == GT_JEDIMASTER &&
		!bs->cur_ps.isJediMaster &&
		bs->jmState == -1 &&
		gJMSaberEnt && gJMSaberEnt->inuse)
	{
		vec3_t diff = { 0 };
		VectorSubtract(bs->origin, gJMSaberEnt->r.currentOrigin, diff);

		float dist = VectorLength(diff);

		// Only care if close enough to make a move
		if (dist < 256.0f)
		{
			// Visibility check
			if (org_visible(bs->origin, gJMSaberEnt->r.currentOrigin, bs->client))
			{
				// Safe copy of goal position
				VectorCopy(gJMSaberEnt->r.currentOrigin, bs->goalPosition);

				// Encourage movement toward the saber
				bs->DestIgnore = gJMSaberEnt->s.number;
			}
			else
			{
				// If not visible, gently nudge toward it anyway
				// (prevents bots freezing when saber is behind a crate)
				vec3_t dir = { 0 };
				VectorSubtract(gJMSaberEnt->r.currentOrigin, bs->origin, dir);

				if (VectorNormalize(dir) > 0.001f)
				{
					VectorMA(bs->origin, 64.0f, dir, bs->goalPosition);
				}
			}
		}
	}

	if (bs->beStill < level.time &&
		!waiting_for_now(bs, bs->goalPosition) &&
		!fj_halt)
	{
		// ---------------------------------------------
		// SAFE GOAL MOVEMENT VECTOR
		// ---------------------------------------------
		vec3_t dir = { 0 };
		VectorSubtract(bs->goalPosition, bs->origin, dir);

		float len = VectorLength(dir);
		if (len > 0.001f)
		{
			VectorScale(dir, 1.0f / len, bs->goalMovedir);
		}
		else
		{
			// No movement needed
			VectorClear(bs->goalMovedir);
		}

		// ---------------------------------------------
		// JUMP PAUSE LOGIC
		// ---------------------------------------------
		if (bs->jumpTime > level.time &&
			bs->jDelay < level.time &&
			level.clients[bs->client].pers.cmd.upmove > 0)
		{
			bs->beStill = level.time + 200;
		}
		else if (VectorLength(bs->goalMovedir) > 0.001f)
		{
			// Saber bots walk, others run
			int speed = (bs->cur_ps.weapon == WP_SABER) ? 2000 : 5000;
			trap->EA_Move(bs->client, bs->goalMovedir, speed);
		}

		// ---------------------------------------------
		// MELEE STRAFING
		// ---------------------------------------------
		if (meleestrafe && bs->meleeStrafeDisable < level.time)
		{
			strafe_tracing(bs);

			if (bs->meleeStrafeDisable < level.time)
			{
				if (bs->meleeStrafeDir)
					trap->EA_MoveRight(bs->client);
				else
					trap->EA_MoveLeft(bs->client);
			}
		}

		// ---------------------------------------------
		// JUMP / DUCK TRIGGERS
		// ---------------------------------------------
		if (bot_trace_jump(bs, bs->goalPosition))
		{
			bs->jumpTime = level.time + 100;
		}
		else if (bot_trace_duck(bs, bs->goalPosition))
		{
			bs->duckTime = level.time + 100;
		}

#ifdef BOT_STRAFE_AVOIDANCE
		// ---------------------------------------------
		// STRAFE AROUND OBSTACLES
		// ---------------------------------------------
		int around = bot_trace_strafe(bs, bs->goalPosition);

		if (around == STRAFEAROUND_RIGHT)
			trap->EA_MoveRight(bs->client);
		else if (around == STRAFEAROUND_LEFT)
			trap->EA_MoveLeft(bs->client);
#endif
	}

	// -----------------------------------------------------
	// JETPACK OVERRIDE (Rule #2)
	// Jetpack bots should not use ground jump logic
	// -----------------------------------------------------
	if (bs->cur_ps.eFlags & EF_JETPACK_ACTIVE)
	{
		return;
	}

	// -----------------------------------------------------
	// FORCE JUMP CHARGE CANCEL (Rule #6)
	// -----------------------------------------------------
#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->forceJumpChargeTime > level.time)
	{
		// While charging force jump, normal jump is disabled
		bs->jumpTime = 0;
	}
#endif

	// -----------------------------------------------------
	// PREP CANCELS FORCE JUMP (Rule #6)
	// -----------------------------------------------------
	if (bs->jumpPrep > level.time)
	{
		bs->forceJumpChargeTime = 0;
	}

	// -----------------------------------------------------
	// FORCE JUMP CHARGE → HOLD CONVERSION (Rule #6)
	// -----------------------------------------------------
	if (bs->forceJumpChargeTime > level.time)
	{
		bs->jumpHoldTime =
			((bs->forceJumpChargeTime - level.time) * 0.5f) + level.time;

		bs->forceJumpChargeTime = 0;
	}

	// -----------------------------------------------------
	// HOLD → JUMP WINDOW (Rule #6)
	// -----------------------------------------------------
	if (bs->jumpHoldTime > level.time)
	{
		bs->jumpTime = bs->jumpHoldTime;
	}

	// -----------------------------------------------------
	// EXECUTE JUMP (Rules #1, #4, #6, #7)
	// -----------------------------------------------------
	if (bs->jumpTime > level.time &&
		bs->jDelay < level.time &&
		!fj_halt)
	{
		// Saber bots walk, not sprint (Rule #1)
		if (bs->cur_ps.weapon == WP_SABER)
		{
			bs->doWalk = qtrue;
		}

		// Held jump (force jump)
		if (bs->jumpHoldTime > level.time)
		{
			trap->EA_Jump(bs->client);

			// Move forward only if it helps reach the waypoint
			if (bs->wpCurrent &&
				bs->wpCurrent->origin[2] - bs->origin[2] < 64)
			{
				trap->EA_MoveForward(bs->client);
			}
			else if (!bs->wpCurrent)
			{
				trap->EA_MoveForward(bs->client);
			}

			// If airborne, keep jump held
			if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
			}
		}
		else
		{
			// Normal jump (not held)
			if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
			{
				trap->EA_Jump(bs->client);
			}
		}
	}

	if (bs->duckTime > level.time)
	{
		trap->EA_Crouch(bs->client);
	}

	if (bs->dangerousObject &&
		bs->dangerousObject->inuse &&
		bs->dangerousObject->health > 0 &&
		bs->dangerousObject->takedamage &&
		(!bs->frame_Enemy_Vis || !bs->currentEnemy) &&
		(bot_get_weapon_range(bs) == BWEAPONRANGE_MID ||
			bot_get_weapon_range(bs) == BWEAPONRANGE_LONG) &&
		bs->cur_ps.weapon != WP_DET_PACK &&
		bs->cur_ps.weapon != WP_TRIP_MINE &&
		!bs->shootGoal)
	{
		// Saber bots should NOT stand still and shoot explosives
		if (bs->cur_ps.weapon == WP_SABER)
			return;

		// Jetpack bots should dodge upward, not shoot
		if (bs->cur_ps.eFlags & EF_JETPACK_ACTIVE)
			return;

		vec3_t diff = { 0 };
		VectorSubtract(bs->dangerousObject->r.currentOrigin, bs->eye, diff);

		float dist = VectorLength(diff);

		// Only react if far enough to safely shoot
		if (dist > 256.0f)
		{
			// Safe angle conversion
			if (dist < 0.001f)
				VectorSet(diff, 1, 0, 0);

			vectoangles(diff, diff);

			// Apply small jitter safely
			bs->goalAngles[YAW] = diff[YAW] + Q_irand(-3, 3);
			bs->goalAngles[PITCH] = diff[PITCH] + Q_irand(-3, 3);

			// Clamp angles to avoid NaNs or wrap issues
			AngleClamp(bs->goalAngles);

			// Visibility check
			if (in_field_of_vision(bs->viewangles, 30, diff) &&
				entity_visible_box(bs->origin, NULL, NULL,
					bs->dangerousObject->r.currentOrigin,
					bs->client,
					bs->dangerousObject->s.number))
			{
				bs->doAttack = 1;
			}
			else
			{
				// Move sideways to get a better angle
				if (Q_irand(0, 1))
					trap->EA_MoveRight(bs->client);
				else
					trap->EA_MoveLeft(bs->client);
			}
		}
		else
		{
			// Too close to shoot → dodge backwards
			vec3_t away = { 0 };
			VectorSubtract(bs->origin, bs->dangerousObject->r.currentOrigin, away);

			if (VectorNormalize(away) > 0.001f)
				trap->EA_Move(bs->client, away, 5000);
		}
	}

	if (prim_firing(bs) ||
		alt_firing(bs))
	{
		friend_in_lof = check_for_friend_in_lof(bs);

		if (friend_in_lof)
		{
			if (prim_firing(bs))
			{
				keep_prim_from_firing(bs);
			}
			if (alt_firing(bs))
			{
				keep_alt_from_firing(bs);
			}
			if (use_the_force && forceHostile)
			{
				use_the_force = 0;
			}

			if (!use_the_force && friend_in_lof->client)
			{
				//we have a friend here and are not currently using force powers, see if we can help them out
				if (friend_in_lof->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.
					clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
					use_the_force = 1;
					forceHostile = 0;
				}
				else if (friend_in_lof->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower >
					forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
					use_the_force = 1;
					forceHostile = 0;
				}
			}
		}
	}
	else if (level.gametype >= GT_TEAM)
	{
		//still check for anyone to help..
		friend_in_lof = check_for_friend_in_lof(bs);

		if (!use_the_force && friend_in_lof)
		{
			if (friend_in_lof->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.
				clients
				[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
				use_the_force = 1;
				forceHostile = 0;
			}
			else if (friend_in_lof->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
				use_the_force = 1;
				forceHostile = 0;
			}
		}
	}

	if (bs->doAttack && bs->cur_ps.weapon == WP_DET_PACK &&
		bs->cur_ps.hasDetPackPlanted)
	{
		//maybe a bit hackish, but bots only want to plant one of these at any given time to avoid complications
		bs->doAttack = 0;
	}

	// -----------------------------------------------------
// SABER DEFENSE OVERRIDE
// If defending and enemy has a blockable weapon, do NOT attack
// -----------------------------------------------------
	if (bs->doAttack &&
		bs->cur_ps.weapon == WP_SABER &&
		bs->saberDefending &&
		bs->currentEnemy && bs->currentEnemy->client &&
		bot_weapon_blockable(bs->currentEnemy->client->ps.weapon))
	{
		bs->doAttack = 0;
	}

	// -----------------------------------------------------
	// SABER LOCK RANDOMIZATION
	// -----------------------------------------------------
	if (bs->cur_ps.saberLockTime > level.time &&
		bs->saberLockDebounce < level.time)
	{
		bs->doAttack = (Q_irand(0, 9) < 5);
		bs->saberLockDebounce = level.time + 50;
	}

	// -----------------------------------------------------
	// CHALLENGE MODE DISABLES ALL ATTACKS
	// -----------------------------------------------------
	if (bs->botChallengingTime > level.time)
	{
		bs->doAttack = 0;
		bs->doAltAttack = 0;
		bs->doBotKick = 0;
	}

	// -----------------------------------------------------
	// SABER KNOCKED AWAY → TRY TO RETRIEVE
	// -----------------------------------------------------
	if (bs->cur_ps.weapon == WP_SABER &&
		bs->cur_ps.saberInFlight &&
		!bs->cur_ps.saberEntityNum)
	{
		// Attack to pull saber back
		bs->doAttack = 1;
		bs->doAltAttack = 0;
	}

	// -----------------------------------------------------
	// EXECUTE ATTACKS
	// -----------------------------------------------------
	if (bs->doAttack)
	{
		trap->EA_Attack(bs->client);
	}
	else if (bs->doAltAttack)
	{
		trap->EA_Alt_Attack(bs->client);
	}

	// -----------------------------------------------------
	// FORCE POWER ACTIVATION (Rule‑aligned)
	// -----------------------------------------------------
	if (use_the_force && forceHostile &&
		bs->botChallengingTime > level.time)
	{
		use_the_force = qfalse;
	}

	if (use_the_force)
	{
		// Jetpack bots should NOT use ground force powers
		if (bs->cur_ps.eFlags & EF_JETPACK_ACTIVE)
			return;

#ifndef FORCEJUMP_INSTANTMETHOD
		// Force jump charge → levitation
		if (bs->forceJumpChargeTime > level.time)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_LEVITATION;
			trap->EA_ForcePower(bs->client);
		}
		else
		{
#endif
			// Normal force power usage
			if (bot_forcepowers.integer && !g_forcePowerDisable.integer)
			{
				trap->EA_ForcePower(bs->client);
			}
#ifndef FORCEJUMP_INSTANTMETHOD
		}
#endif
	}

	move_toward_ideal_angles(bs);

}

static void movement_command(bot_state_t* bs, const int command, vec3_t move_dir)
{
	if (!command)
	{
		//don't need to do anything
		return;
	}
	if (command == 1)
	{
		//Force Jump
		bs->jumpTime = level.time + 100;
		return;
	}
	if (command == 2)
	{
		//crouch
		bs->duckTime = level.time + 100;
		return;
	}
	//can't move!
	VectorCopy(vec3_origin, move_dir);
}

static void adjust_move_direction(const bot_state_t* bs, vec3_t move_dir, const int quad)
{
	vec3_t fwd, right;
	vec3_t addvect;

	AngleVectors(bs->cur_ps.viewangles, fwd, right, NULL);
	fwd[2] = 0;
	right[2] = 0;

	VectorNormalize(fwd);
	VectorNormalize(right);

	switch (quad)
	{
	case 0:
		VectorCopy(fwd, addvect);
		break;
	case 1:
		VectorAdd(fwd, right, addvect);
		VectorNormalize(addvect);
		break;
	case 2:
		VectorCopy(right, addvect);
		break;
	case 3:
		VectorScale(fwd, -1, fwd);
		VectorAdd(fwd, right, addvect);
		VectorNormalize(addvect);
		break;
	case 4:
		VectorScale(fwd, -1, addvect);
		break;
	case 5:
		VectorScale(fwd, -1, fwd);
		VectorScale(right, -1, right);
		VectorAdd(fwd, right, addvect);
		VectorNormalize(addvect);
		break;
	case 6:
		VectorScale(right, -1, addvect);
		break;
	case 7:
		VectorScale(right, -1, right);
		VectorAdd(fwd, right, addvect);
		VectorNormalize(addvect);
		break;
	default:
		return;
	}

	VectorCopy(addvect, move_dir);
	VectorNormalize(move_dir);
}

static int adjust_quad(const int quad)
{
	int dir = quad;
	while (dir > 7)
	{
		//shift
		dir -= 8;
	}
	while (dir < 0)
	{
		//shift
		dir += 8;
	}

	return dir;
}

static int find_movement_quad(const playerState_t* ps, vec3_t move_dir)
{
	vec3_t viewfwd, viewright;
	vec3_t move;

	AngleVectors(ps->viewangles, viewfwd, viewright, NULL);

	VectorCopy(move_dir, move);

	viewfwd[2] = 0;
	viewright[2] = 0;
	move[2] = 0;

	VectorNormalize(viewfwd);
	VectorNormalize(viewright);
	VectorNormalize(move);

	const float x = DotProduct(viewright, move);
	const float y = DotProduct(viewfwd, move);

	if (x > .8)
	{
		//right
		return 2;
	}
	if (x < -0.8)
	{
		//left
		return 6;
	}
	if (x > .2)
	{
		//right forward/back
		if (y < 0)
		{
			//right back
			return 3;
		}
		//right forward
		return 1;
	}
	if (x < -0.2)
	{
		//left forward/back
		if (y < 0)
		{
			//left back
			return 5;
		}
		//left forward
		return 7;
	}
	//forward/back
	if (y < 0)
	{
		//back
		return 4;
	}
	//forward
	return 0;
}

static int trace_jump_crouch_fall(const bot_state_t* bs, vec3_t move_dir, const int target_num, vec3_t hit_normal)
{
	vec3_t mins;
	vec3_t maxs;
	vec3_t traceto_mod;
	vec3_t tracefrom_mod;
	vec3_t save_normal;
	trace_t tr;
	int move_command = -1;

	VectorClear(save_normal);

	//set the mins/maxs for the standard obstruction checks.
	VectorCopy(g_entities[bs->client].r.maxs, maxs);
	VectorCopy(g_entities[bs->client].r.mins, mins);

	//boost up the trace box as much as we can normally step up
	mins[2] += STEPSIZE;

	//Ok, check for obstructions then.
	traceto_mod[0] = bs->origin[0] + move_dir[0] * 20;
	traceto_mod[1] = bs->origin[1] + move_dir[1] * 20;
	traceto_mod[2] = bs->origin[2] + move_dir[2] * 20;

	//obstruction trace
	trap->Trace(&tr, bs->origin, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction == 1 //trace is clear
		|| tr.entityNum == target_num //is our ignore target
		|| bs->currentEnemy && bs->currentEnemy->s.number == tr.entityNum) //is our current enemy
	{
		//nothing blocking our path
		move_command = 0;
	}
	else if (tr.entityNum == ENTITYNUM_WORLD)
	{
		//world object, check to see if we can walk on it.
		if (tr.plane.normal[2] >= 0.7f)
		{
			//you're probably moving up a steep ledge that's still walkable
			move_command = 0;
		}
	}
	//check to see if this is another player.  If so, we should be able to jump over them easily.
	//RAFIXME - add force power/force jump skill check?
	else if (tr.entityNum < MAX_CLIENTS
		//not a bot or a bot that isn't jumping.
		&& (!botstates[tr.entityNum] || !botstates[tr.entityNum]->inuse
			|| botstates[tr.entityNum]->jumpTime < level.time)
		&& bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] >= FORCE_LEVEL_1)
	{
		//another player who isn't our objective and isn't our current enemy.  Hop over them.  Don't hop
		//over bots who are already hopping.
		move_command = 1;
	}

	if (move_command == -1)
	{
		//our initial path is blocked. Try other methods to get around it.
		//Save the normal of the object so we can move around it if we can't jump/duck around it.
		VectorCopy(tr.plane.normal, save_normal);

		//Check to see if we can successfully hop over this obsticle.
		VectorCopy(bs->origin, tracefrom_mod);

		//RAFIXME - make this based on Force jump skill level/force power availible.
		tracefrom_mod[2] += 20;
		traceto_mod[2] += 20;

		trap->Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

		if (tr.fraction == 1 //trace is clear
			|| tr.entityNum == target_num //is our ignore target
			|| bs->currentEnemy && bs->currentEnemy->s.number == tr.entityNum) //is our current enemy
		{
			//the hop check was clean.
			move_command = 1;
		}
		//check the slope of the thing blocking us
		else if (tr.entityNum == ENTITYNUM_WORLD)
		{
			//world object
			if (tr.plane.normal[2] >= 0.7f)
			{
				//you could hop to this, which is a steep ledge that's still walkable
				move_command = 1;
			}
		}

		if (move_command == -1)
		{
			//our hop would be blocked by something.  let's try crawling under obsticle.
			//just move the traceto_mod down from the hop trace position.  This is faster
			//than redoing it from scratch.
			traceto_mod[2] -= 20;

			maxs[2] = CROUCH_MAXS_2; //set our trace box to be the size of a crouched player.

			trap->Trace(&tr, bs->origin, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

			if (tr.fraction == 1 //trace is clear
				|| tr.entityNum == target_num //is our ignore target
				|| bs->currentEnemy && bs->currentEnemy->s.number == tr.entityNum) //is our current enemy
			{
				//we can duck under this object.
				move_command = 2;
			}
			//check the slope of the thing blocking us
			else if (tr.entityNum == ENTITYNUM_WORLD)
			{
				//world object
				if (tr.plane.normal[2] >= 0.7f)
				{
					//you could hop to this, which is a steep ledge that's still walkable
					move_command = 2;
				}
			}
		}
	}

	if (move_command != -1)
	{
		//we found a way around our current obsticle, check to make sure we're not going to go off a cliff.
		traceto_mod[0] = bs->origin[0] + move_dir[0] * 45;
		traceto_mod[1] = bs->origin[1] + move_dir[1] * 45;
		traceto_mod[2] = bs->origin[2] + move_dir[2] * 45;

		VectorCopy(traceto_mod, tracefrom_mod);

		//check for 50+ feet drops
		traceto_mod[2] -= 532;

		trap->Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);
		if (tr.fraction == 1 && !tr.startsolid)
		{
			//CLIFF!
			move_command = -1;
		}

		if (bs->jumpTime < level.time)
		{
			const int contents = trap->PointContents(tr.endpos, -1);
			if (contents & (CONTENTS_SLIME | CONTENTS_LAVA))
			{
				//the fall point is inside something we don't want to move into
				move_command = -1;
			}
		}
	}

	if (move_command == -1)
	{
		//couldn't find a way to move in this direction.  Save the normal vector so we can use it to move around
		//this object.  Note, we even do this when we can hop/crawl around something but the fall check fails
		//so we can follow the railing or whatever this is.
		VectorCopy(save_normal, hit_normal);
	}
	return move_command;
}

static qboolean try_move_around_obsticle(bot_state_t* bs, vec3_t move_dir, const int target_num, vec3_t hit_normal,
	const int try_num,
	const qboolean check_both_ways)
{
	//attempt to move around objects.

	if (try_num > 3)
	{
		//ok, don't get stuff in a loop
		return qfalse;
	}

	if (!VectorCompare(hit_normal, vec3_origin))
	{
		vec3_t cross;

		//find the perpendicular vector to the normal.
		PerpendicularVector(cross, hit_normal);
		cross[2] = 0; //flatten the cross vector to 2d.
		VectorNormalize(cross);

		//determine initial movement direction.
		if (bs->evadeTime > level.time)
		{
			//already going a set direction, keep going that direction.
			if (bs->evadeDir > 3)
			{
				//going "left".  switch directions.
				VectorScale(cross, -1, cross);
			}
		}
		else
		{
			//otherwise, try to move in the direction that takes us in the direction we're
			//trying to move.
			const float dot = DotProduct(cross, move_dir);
			if (dot < 0)
			{
				//going in the wrong initial direction, switch!
				VectorScale(cross, -1, cross);
			}
		}

		VectorCopy(cross, move_dir);
		int movecom = trace_jump_crouch_fall(bs, move_dir, target_num, hit_normal);

		if (movecom != -1)
		{
			movement_command(bs, movecom, move_dir);
			return qtrue;
		}

		if (!VectorCompare(hit_normal, vec3_origin))
		{
			//hit another surface while trying to trace along this one, try to move along
			//it instead.
			if (try_move_around_obsticle(bs, move_dir, target_num, hit_normal, try_num + 1, qfalse))
			{
				//set the evade timer because this is often where we can get stuck if
				//tracing the wall sends us in some weird direction.
				bs->evadeTime = level.time + 10000;
				return qtrue;
			}
		}

		if (check_both_ways)
		{
			//try the other direction
			VectorScale(cross, -1, cross);

			//toggle the evadeDir since we want to continue moving this direction.
			if (bs->evadeDir > 3)
			{
				//was left, try right
				bs->evadeDir = 0;
			}
			else
			{
				//was right, try left
				bs->evadeDir = 7;
			}

			VectorCopy(cross, move_dir);
			movecom = trace_jump_crouch_fall(bs, move_dir, target_num, hit_normal);

			if (movecom != -1)
			{
				movement_command(bs, movecom, move_dir);
				//set the evade timer because this is often where we can get stuck if
				//tracing the wall sends us in some weird direction.
				bs->evadeTime = level.time + 10000;
				return qtrue;
			}

			//try recursively dealing with this.
			return try_move_around_obsticle(bs, move_dir, target_num, hit_normal, try_num + 1, qfalse);
		}
	}

	return qfalse;
}

void trace_move(bot_state_t* bs, vec3_t move_dir, const int target_num)
{
	vec3_t dir = {0, 0, 0};
	vec3_t hit_normal;
	int i = 7;
	int quad;
	VectorClear(hit_normal);
	int movecom = trace_jump_crouch_fall(bs, move_dir, target_num, hit_normal);

	VectorCopy(move_dir, dir);

	if (movecom != -1)
	{
		movement_command(bs, movecom, move_dir);
		return;
	}

	//try moving around the edge of the blocking object if that's the problem.
	if (try_move_around_obsticle(bs, dir, target_num, hit_normal, 0, qtrue))
	{
		//found a way.
		VectorCopy(dir, move_dir);
		return;
	}

	//restore the original moveDir incase our obsticle code choked.
	VectorCopy(move_dir, dir);

	if (bs->evadeTime > level.time)
	{
		//try the current evade direction to prevent spazing
		adjust_move_direction(bs, dir, bs->evadeDir);
		movecom = trace_jump_crouch_fall(bs, dir, target_num, hit_normal);
		if (movecom != -1)
		{
			movement_command(bs, movecom, dir);
			VectorCopy(dir, move_dir);
			bs->evadeTime = level.time + 500;
			return;
		}
		i--;
	}

	//Since our default direction didn't work we need to switch melee strafe directions if
	//we are melee strafing.
	//0 = no strafe
	//1 = strafe right
	//2 = strafe left
	if (bs->meleeStrafeTime > level.time)
	{
		bs->meleeStrafeDir = Q_irand(0, 2);
		bs->meleeStrafeTime = level.time + Q_irand(500, 1800);
	}

	const int fwdstrafe = find_movement_quad(&bs->cur_ps, move_dir);

	if (Q_irand(0, 1))
	{
		//try strafing left
		quad = fwdstrafe - 2;
	}
	else
	{
		quad = fwdstrafe + 2;
	}

	quad = adjust_quad(quad);

	//reset Dir to original moveDir
	VectorCopy(move_dir, dir);

	//shift movedir for quad
	adjust_move_direction(bs, dir, quad);

	movecom = trace_jump_crouch_fall(bs, dir, target_num, hit_normal);

	if (movecom != -1)
	{
		movement_command(bs, movecom, dir);
		VectorCopy(dir, move_dir);
		bs->evadeDir = quad;
		bs->evadeTime = level.time + 100;
		return;
	}
	i--;

	//no luck, try the other full strafe direction
	quad += 4;
	quad = adjust_quad(quad);

	//reset Dir to original moveDir
	VectorCopy(move_dir, dir);

	//shift movedir for quad
	adjust_move_direction(bs, dir, quad);

	movecom = trace_jump_crouch_fall(bs, dir, target_num, hit_normal);

	if (movecom != -1)
	{
		movement_command(bs, movecom, dir);
		VectorCopy(dir, move_dir);
		bs->evadeDir = quad;
		bs->evadeTime = level.time + 100;
		return;
	}
	i--;

	//still no dice
	for (; i > 0; i--)
	{
		quad++;
		quad = adjust_quad(quad);

		if (quad == fwdstrafe || quad == adjust_quad(fwdstrafe - 2) || quad == adjust_quad(fwdstrafe + 2)
			|| bs->evadeTime > level.time && quad == bs->evadeDir)
		{
			//Already did those directions
			continue;
		}

		VectorCopy(move_dir, dir);

		//shift movedir for quad
		adjust_move_direction(bs, dir, quad);
		movecom = trace_jump_crouch_fall(bs, dir, target_num, hit_normal);
		if (movecom != -1)
		{
			//find a good direction
			movement_command(bs, movecom, dir);
			VectorCopy(dir, move_dir);
			bs->evadeDir = quad;
			bs->evadeTime = level.time + 100;
			return;
		}
	}
}

extern void npc_conversation_animation();

static void bot_check_speak(gentity_t* self, const qboolean moving)
{
	char filename[256];
	char npc_sound_dir[256];
	fileHandle_t f;

	if (self->bot_check_speach_time > level.time) return; // not yet...

	self->bot_check_speach_time = level.time + 5000 + irand(0, 15000);

	strcpy(npc_sound_dir, self->client->botSoundDir);

	if (botstates[self->s.number]->currentEnemy && botstates[self->s.number]->enemySeenTime >= level.time)
	{
		// Have enemy...
		const int rand_choice = irand(0, 50);

		switch (rand_choice)
		{
		case 0:
			strcpy(filename, va("sound/chars/%s/misc/anger1.mp3", npc_sound_dir));
			break;
		case 1:
			strcpy(filename, va("sound/chars/%s/misc/anger2.mp3", npc_sound_dir));
			break;
		case 2:
			strcpy(filename, va("sound/chars/%s/misc/anger3.mp3", npc_sound_dir));
			break;
		case 3:
			strcpy(filename, va("sound/chars/%s/misc/chase1.mp3", npc_sound_dir));
			break;
		case 4:
			strcpy(filename, va("sound/chars/%s/misc/chase2.mp3", npc_sound_dir));
			break;
		case 5:
			strcpy(filename, va("sound/chars/%s/misc/chase3.mp3", npc_sound_dir));
			break;
		case 6:
			strcpy(filename, va("sound/chars/%s/misc/combat1.mp3", npc_sound_dir));
			break;
		case 7:
			strcpy(filename, va("sound/chars/%s/misc/combat2.mp3", npc_sound_dir));
			break;
		case 8:
			strcpy(filename, va("sound/chars/%s/misc/combat3.mp3", npc_sound_dir));
			break;
		case 9:
			strcpy(filename, va("sound/chars/%s/misc/confuse1.mp3", npc_sound_dir));
			break;
		case 10:
			strcpy(filename, va("sound/chars/%s/misc/confuse2.mp3", npc_sound_dir));
			break;
		case 11:
			strcpy(filename, va("sound/chars/%s/misc/confuse3.mp3", npc_sound_dir));
			break;
		case 12:
			strcpy(filename, va("sound/chars/%s/misc/cover1.mp3", npc_sound_dir));
			break;
		case 13:
			strcpy(filename, va("sound/chars/%s/misc/cover2.mp3", npc_sound_dir));
			break;
		case 14:
			strcpy(filename, va("sound/chars/%s/misc/cover3.mp3", npc_sound_dir));
			break;
		case 15:
			strcpy(filename, va("sound/chars/%s/misc/cover4.mp3", npc_sound_dir));
			break;
		case 16:
			strcpy(filename, va("sound/chars/%s/misc/cover5.mp3", npc_sound_dir));
			break;
		case 17:
			strcpy(filename, va("sound/chars/%s/misc/deflect1.mp3", npc_sound_dir));
			break;
		case 18:
			strcpy(filename, va("sound/chars/%s/misc/deflect2.mp3", npc_sound_dir));
			break;
		case 19:
			strcpy(filename, va("sound/chars/%s/misc/deflect3.mp3", npc_sound_dir));
			break;
		case 20:
			strcpy(filename, va("sound/chars/%s/misc/detected1.mp3", npc_sound_dir));
			break;
		case 21:
			strcpy(filename, va("sound/chars/%s/misc/detected2.mp3", npc_sound_dir));
			break;
		case 22:
			strcpy(filename, va("sound/chars/%s/misc/detected3.mp3", npc_sound_dir));
			break;
		case 23:
			strcpy(filename, va("sound/chars/%s/misc/detected4.mp3", npc_sound_dir));
			break;
		case 24:
			strcpy(filename, va("sound/chars/%s/misc/detected5.mp3", npc_sound_dir));
			break;
		case 25:
			strcpy(filename, va("sound/chars/%s/misc/escaping1.mp3", npc_sound_dir));
			break;
		case 26:
			strcpy(filename, va("sound/chars/%s/misc/escaping2.mp3", npc_sound_dir));
			break;
		case 27:
			strcpy(filename, va("sound/chars/%s/misc/escaping3.mp3", npc_sound_dir));
			break;
		case 28:
			strcpy(filename, va("sound/chars/%s/misc/giveup1.mp3", npc_sound_dir));
			break;
		case 29:
			strcpy(filename, va("sound/chars/%s/misc/giveup2.mp3", npc_sound_dir));
			break;
		case 30:
			strcpy(filename, va("sound/chars/%s/misc/giveup3.mp3", npc_sound_dir));
			break;
		case 31:
			strcpy(filename, va("sound/chars/%s/misc/giveup4.mp3", npc_sound_dir));
			break;
		case 32:
			strcpy(filename, va("sound/chars/%s/misc/gloat1.mp3", npc_sound_dir));
			break;
		case 33:
			strcpy(filename, va("sound/chars/%s/misc/gloat2.mp3", npc_sound_dir));
			break;
		case 34:
			strcpy(filename, va("sound/chars/%s/misc/gloat3.mp3", npc_sound_dir));
			break;
		case 35:
			strcpy(filename, va("sound/chars/%s/misc/jchase1.mp3", npc_sound_dir));
			break;
		case 36:
			strcpy(filename, va("sound/chars/%s/misc/jchase2.mp3", npc_sound_dir));
			break;
		case 37:
			strcpy(filename, va("sound/chars/%s/misc/jchase3.mp3", npc_sound_dir));
			break;
		case 38:
			strcpy(filename, va("sound/chars/%s/misc/jdetected1.mp3", npc_sound_dir));
			break;
		case 39:
			strcpy(filename, va("sound/chars/%s/misc/jdetected2.mp3", npc_sound_dir));
			break;
		case 40:
			strcpy(filename, va("sound/chars/%s/misc/jdetected3.mp3", npc_sound_dir));
			break;
		case 41:
			strcpy(filename, va("sound/chars/%s/misc/jlost1.mp3", npc_sound_dir));
			break;
		case 42:
			strcpy(filename, va("sound/chars/%s/misc/jlost2.mp3", npc_sound_dir));
			break;
		case 43:
			strcpy(filename, va("sound/chars/%s/misc/jlost3.mp3", npc_sound_dir));
			break;
		case 44:
			strcpy(filename, va("sound/chars/%s/misc/outflank1.mp3", npc_sound_dir));
			break;
		case 45:
			strcpy(filename, va("sound/chars/%s/misc/outflank2.mp3", npc_sound_dir));
			break;
		case 46:
			strcpy(filename, va("sound/chars/%s/misc/outflank3.mp3", npc_sound_dir));
			break;
		case 47:
			strcpy(filename, va("sound/chars/%s/misc/taunt.mp3", npc_sound_dir));
			break;
		case 48:
			strcpy(filename, va("sound/chars/%s/misc/taunt1.mp3", npc_sound_dir));
			break;
		case 49:
			strcpy(filename, va("sound/chars/%s/misc/taunt2.mp3", npc_sound_dir));
			break;
		default:
			strcpy(filename, va("sound/chars/%s/misc/taunt3.mp3", npc_sound_dir));
			break;
		}
	}
	else
	{
		// No enemy...
		const int rand_choice = irand(0, 10);

		switch (rand_choice)
		{
		case 0:
			strcpy(filename, va("sound/chars/%s/misc/sight1.mp3", npc_sound_dir));
			break;
		case 1:
			strcpy(filename, va("sound/chars/%s/misc/sight2.mp3", npc_sound_dir));
			break;
		case 2:
			strcpy(filename, va("sound/chars/%s/misc/sight3.mp3", npc_sound_dir));
			break;
		case 3:
			strcpy(filename, va("sound/chars/%s/misc/look1.mp3", npc_sound_dir));
			break;
		case 4:
			strcpy(filename, va("sound/chars/%s/misc/look2.mp3", npc_sound_dir));
			break;
		case 5:
			strcpy(filename, va("sound/chars/%s/misc/look3.mp3", npc_sound_dir));
			break;
		case 6:
			strcpy(filename, va("sound/chars/%s/misc/suspicious1.mp3", npc_sound_dir));
			break;
		case 7:
			strcpy(filename, va("sound/chars/%s/misc/suspicious2.mp3", npc_sound_dir));
			break;
		case 8:
			strcpy(filename, va("sound/chars/%s/misc/suspicious3.mp3", npc_sound_dir));
			break;
		case 9:
			strcpy(filename, va("sound/chars/%s/misc/suspicious4.mp3", npc_sound_dir));
			break;
		default:
			strcpy(filename, va("sound/chars/%s/misc/suspicious5.mp3", npc_sound_dir));
			break;
		}
	}

	trap->FS_Open(filename, &f, FS_READ);

	if (!f)
	{
		trap->FS_Close(f);
		return;
	}

	trap->FS_Close(f);

	trap->Print("Bot sound file [%s] played.\n", filename);

	// Play a taunt/etc...
	G_SoundOnEnt(self, CHAN_VOICE_ATTEN, filename);

	if (!moving)
	{
		npc_conversation_animation();
	}
}

//Behavior to move to the given DestPosition
//strafe = do some strafing while moving to this location
static void bot_moveto(bot_state_t* bs, const qboolean strafe)
{
	qboolean recalcroute = qfalse;
	qboolean findwp = qfalse;
	int badwp = -2;

	if (!bs->wpCurrent)
	{
		////ok, we just did something other than wp navigation.  find the closest wp.
		findwp = qtrue;
	}
	else if (bs->wpSeenTime < level.time)
	{
		//lost track of the waypoint
		findwp = qtrue;
		badwp = bs->wpCurrent->index;
		bs->wpDestination = NULL;
		recalcroute = qtrue;
	}
	else if (bs->wpTravelTime < level.time)
	{
		//spent too much time traveling to this point or lost sight for too long.
		//recheck everything
		findwp = qtrue;
		badwp = bs->wpCurrent->index;

		bs->wpDestination = NULL;
		recalcroute = qtrue;
	}
	//Check to make sure we didn't get knocked way off course.
	else if (!bs->wpSpecial)
	{
		const float distthen = Distance(bs->wpCurrentLoc, bs->wpCurrent->origin);
		const float distnow = Distance(bs->wpCurrent->origin, bs->origin);
		if (2 * distthen < distnow)
		{
			//we're pretty far off the path, check to make sure we didn't get knocked way off course.
			findwp = qtrue;
		}
	}

	if (!VectorCompare(bs->DestPosition, bs->lastDestPosition) || !bs->wpDestination)
	{
		//The goal position has moved from last frame.  make sure it's not closer to a different
		//destination WP
		int destwp = get_nearest_visible_wpsje(bs, bs->DestPosition, bs->client, badwp);

		if (destwp == -1)
		{
			//ok, don't have a wappoint that can see that point...ok, go to the closest wp and
			//and try from there.
			destwp = get_nearest_wp(bs, bs->DestPosition, badwp);

			if (destwp == -1)
			{
				//crap, this map has no wps.  try just autonaving it then
				bot_move(bs, bs->DestPosition, qfalse, strafe);
				return;
			}
		}

		if (!bs->wpDestination || bs->wpDestination->index != destwp)
		{
			bs->wpDestination = gWPArray[destwp];
			recalcroute = qtrue;
		}
	}

	bot_check_speak(&g_entities[bs->client], qtrue);

	if (findwp)
	{
		int wp = get_nearest_visible_wpsje(bs, bs->origin, bs->client, badwp);
		if (wp == -1)
		{
			//can't find a visible
			wp = get_nearest_wp(bs, bs->origin, badwp);
			if (wp == -1)
			{
				//no waypoints
				bot_move(bs, bs->DestPosition, qfalse, strafe);
				return;
			}
		}

		//got a waypoint, lock on and move towards it
		bs->wpCurrent = gWPArray[wp];
		reset_wp_timers(bs);
		VectorCopy(bs->origin, bs->wpCurrentLoc);
		if (!recalcroute && find_on_route(bs->wpCurrent->index, bs->botRoute) == -1)
		{
			//recalc route
			recalcroute = qtrue;
		}
	}

	if (recalcroute)
	{
		if (find_ideal_pathto_wp(bs, bs->wpCurrent->index, bs->wpDestination->index, badwp, bs->botRoute) == -1)
		{
			//can't get to destination wp from current wp, wing it
			bs->wpCurrent = NULL;
			clear_route(bs->botRoute);
			bot_move(bs, bs->DestPosition, qfalse, strafe);
			return;
		}
		//set wp timers
		reset_wp_timers(bs);
	}

	//travelling to a waypoint
	if ((bs->wpCurrent->index != bs->wpDestination->index || !bs->wpTouchedDest) && Distance(
		bs->origin, bs->wpCurrent->origin) < BOT_WPTOUCH_DISTANCE
		&& !bs->wpSpecial)
	{
		wp_touch(bs);
	}

	//if you're closer to your bs->DestPosition than you are to your next waypoint, just
	//move to your bs->DestPosition.  This is to prevent the bots from backstepping when
	//very close to their target
	if (!bs->wpSpecial && (Distance(bs->origin, bs->wpCurrent->origin) > Distance(bs->origin, bs->DestPosition)
		//closer to our destination than the next waypoint
		|| bs->wpCurrent->index == bs->wpDestination->index && bs->wpTouchedDest))
		//We've touched our final waypoint and should head towards the destination
	{
		//move to DestPosition
		bot_move(bs, bs->DestPosition, qfalse, strafe);
	}
	else
	{
		//move to next waypoint
		bot_move(bs, bs->wpCurrent->origin, qtrue, strafe);
	}
}

void bot_behave_defend_basic(bot_state_t* bs, vec3_t defpoint)
{
	const float dist = Distance(bs->origin, defpoint);

	if (bs->currentEnemy)
	{
		//see an enemy
		bot_behave_attack_basic(bs, bs->currentEnemy);
		if (dist > 500)
		{
			//attack move back into the defend range
			bot_move(bs, defpoint, qfalse, qfalse);
		}
		else if (dist > 500 * .9)
		{
			//nearing max distance hold here and attack
			trap->EA_Move(bs->client, vec3_origin, 0);
		}
		else
		{
			//just attack them
		}
	}
	else
	{
		//don't see an enemy
		if (dont_block_allies(bs))
		{
		}
		else if (dist < 200)
		{
			//just stand around and wait
		}
		else
		{
			//move closer to defend target
			bot_move(bs, defpoint, qfalse, qfalse);
		}
	}
}

void bot_behave_attack_move(bot_state_t* bs)
{
	vec3_t view_dir;
	vec3_t ang;
	vec3_t enemy_origin;

	if (!bs->frame_Enemy_Vis && bs->enemySeenTime < level.time)
	{
		//lost track of enemy
		bs->currentEnemy = NULL;
		return;
	}

	FindOrigin(bs->currentEnemy, enemy_origin);

	const float range = TargetDistance(bs, bs->currentEnemy, enemy_origin);

	//move towards DestPosition
	if (VectorLength(bs->velocity) <= 16)
	{
		bot_moveto(bs, qtrue);
	}
	else
	{
		bot_moveto(bs, qfalse);
	}

	if (bs->wpSpecial)
	{
		//in special wp move, don't do interrupt it.
		return;
	}

	//adjust angle for target leading.
	const float leadamount = bot_weapon_can_lead(bs);

	bot_aim_leading(bs, enemy_origin, leadamount);

	//set viewangle
	VectorSubtract(enemy_origin, bs->eye, view_dir);

	vectoangles(view_dir, ang);
	VectorCopy(ang, bs->goalAngles);

	if (bs->frame_Enemy_Vis && bs->cur_ps.weapon == bs->virtualWeapon && range < MaximumAttackDistance[bs->
		virtualWeapon]
		&& range > MinimumAttackDistance[bs->virtualWeapon]
		&& (in_field_of_vision(bs->viewangles, 30, ang)
			|| bs->virtualWeapon == WP_SABER && in_field_of_vision(bs->viewangles, 100, ang)))
	{
		//don't attack unless you're inside your AttackDistance band and actually pointing at your enemy.
		if (bs->virtualWeapon != WP_SABER && bs->cur_ps.BlasterAttackChainCount >= MISHAPLEVEL_HEAVY)
		{
			//don't shoot like a retard if you're not going to hit anything
			return;
		}

		//This is to prevent the bots from attackmoving with the saber @ 500 meters. :)
		trap->EA_Attack(bs->client);

		if (bs->virtualWeapon == WP_SABER)
		{
			//only walk while attacking with the saber.
			bs->doWalk = qtrue;
		}
	}
}

void bot_behave_attack(bot_state_t* bs)
{
	const int desiredweap = favorite_weapon(bs, bs->currentEnemy, qtrue, qtrue, 0);

	if (bs->frame_Enemy_Len > MaximumAttackDistance[desiredweap])
	{
		//this should be an attack while moving function but for now we'll just use moveto
		vec3_t enemyOrigin;
		FindOrigin(bs->currentEnemy, enemyOrigin);
		VectorCopy(enemyOrigin, bs->DestPosition);
		bs->DestIgnore = bs->currentEnemy->s.number;
		bot_behave_attack_move(bs);
		return;
	}

	//we're going to go get in close so null out the wpCurrent so it will update when we're
	//done.
	bs->wpCurrent = NULL;

	//use basic attack
	bot_behave_attack_basic(bs, bs->currentEnemy);
}

int bot_weapon_detpack(bot_state_t* bs, const gentity_t* target)
{
	gentity_t* dp = NULL;
	const gentity_t* bestDet = NULL;
	vec3_t targ_origin;
	float best_distance = 9999;

	FindOrigin(target, targ_origin);

	while ((dp = G_Find(dp, FOFS(classname), "detpack")) != NULL)
	{
		if (dp && dp->parent && dp->parent->s.number == bs->client)
		{
			const float tempDist = Distance(targ_origin, dp->s.pos.trBase);
			if (tempDist < best_distance)
			{
				best_distance = tempDist;
				bestDet = dp;
			}
		}
	}

	if (!bestDet || best_distance > 500)
	{
		return qfalse;
	}

	//check to see if the bot knows that the det is near the target.

	//found the closest det to the target and it is in blow distance.
	if (WP_DET_PACK != bs->cur_ps.weapon)
	{
		//need to switch to desired weapon
		bot_select_choice_weapon(bs, WP_DET_PACK, qtrue);
	}
	else
	{
		//blast it!
		bs->doAltAttack = 1;
	}

	return qtrue;
}


//----- Start Enhaned logic code -----//



qboolean BotOrderJetPack(bot_state_t* bs)
{
	if (!bs->currentEnemy)
		return qfalse;

	// Reset forced movement each frame
	bs->forceMove_Forward = 0;
	bs->forceMove_Right = 0;
	bs->forceMove_Up = 0;

	// Always fly when enemy is present
	bs->forceMove_Up = 1;

	float dist = bs->frame_Enemy_Len;

	if (dist < 256)
	{
		// Back away while flying
		bs->forceMove_Forward = -1;
	}
	else
	{
		// Strafe around enemy in 3D
		bs->forceMove_Right = (rand() % 2) ? 1 : -1;
		bs->forceMove_Forward = 1;
	}

	// Attack with current weapon
	bs->doAttack = qtrue;

	// Jetpack overrides ground movement
	bs->doBotKick = qfalse;
	bs->doJump = qfalse;
	bs->doWalk = qfalse;

	return qtrue;
}

static int BotSelectChoiceWeapon(bot_state_t* bs, int weapon, int doselection)
{ //if !doselection then bot will only check if he has the specified weapon and return 1 (yes) or 0 (no)
	int i;
	int hasit = 0;

	i = 0;

	while (i < WP_NUM_WEAPONS)
	{
		if (bs->cur_ps.ammo[weaponData[i].ammoIndex] >= weaponData[i].energyPerShot &&
			i == weapon &&
			(bs->cur_ps.stats[STAT_WEAPONS] & (1 << i)))
		{
			hasit = 1;
			break;
		}

		i++;
	}

	if (hasit && bs->cur_ps.weapon != weapon && doselection && bs->virtualWeapon != weapon)
	{
		bs->virtualWeapon = weapon;
		bot_select_weapon(bs->client, weapon);
		return 2;
	}

	if (hasit)
	{
		return 1;
	}

	return 0;
}

qboolean BotOrderSaberDuel(bot_state_t* bs)
{
	gentity_t* self = &g_entities[bs->client];

	if (!bs->currentEnemy || !bs->tacticEntity || !bs->tacticEntity->client)
		return qfalse;

	// Reset forced movement each frame
	bs->forceMove_Forward = 0;
	bs->forceMove_Right = 0;
	bs->forceMove_Up = 0;

	// Walk-only rule
	bs->doWalk = qtrue;
	bs->doJump = qfalse;
	bs->doBotKick = qfalse;

	float dist = bs->frame_Enemy_Len;

	// Movement logic
	if (dist > 140)
	{
		bs->forceMove_Forward = 1;
	}
	else
	{
		bs->forceMove_Right = (rand() % 2) ? 1 : -1;
	}

	// Attack when in range
	if (dist < 150)
	{
		bs->doAttack = qtrue;
	}

	// Tactic entity dead ? end tactic
	if (bs->tacticEntity->health < 1)
	{
		bs->currentTactic = BOTORDER_NONE;
		bs->tacticEntity = NULL;

		if (bs->botOrder == BOTORDER_SABERDUELCHALLENGE)
		{
			bs->botOrder = BOTORDER_NONE;
			bs->orderEntity = NULL;
		}

		return qtrue;
	}

	// Not in duel yet ? approach and challenge
	if (!self->client->ps.duelInProgress)
	{
		float d2 = DistanceHorizontalSquared(bs->origin,
			bs->tacticEntity->client->ps.origin);

		if (d2 > SABERDUELCHALLENGEDIST * SABERDUELCHALLENGEDIST)
		{
			// Move toward target
			VectorCopy(bs->tacticEntity->client->ps.origin, bs->DestPosition);
			bs->DestIgnore = bs->tacticEntity->s.number;
			bot_moveto(bs, qfalse);
		}
		else
		{
			// Face target
			vec3_t viewDir, ang;
			VectorSubtract(bs->tacticEntity->client->ps.origin, bs->eye, viewDir);
			vectoangles(viewDir, ang);
			VectorCopy(ang, bs->goalAngles);

			// Ensure saber is out
			if (bs->cur_ps.weapon != WP_SABER && bs->virtualWeapon != WP_SABER)
			{
				bot_select_choice_weapon(bs, WP_SABER, 1);
			}
			else
			{
				// Send challenge (debounced)
				if (!(bs->MiscBotFlags & BOTFLAG_SABERCHALLENGED) ||
					bs->miscBotFlagsTimer <= level.time)
				{
					Cmd_EngageDuel_f(self);
					bs->MiscBotFlags |= BOTFLAG_SABERCHALLENGED;
					bs->miscBotFlagsTimer = level.time + Q_irand(5000, 10000);
				}
			}
		}

		return qtrue;
	}

	// Already in duel ? fight normally
	if (bs->tacticEntity->s.number == bs->currentEnemy->s.number)
	{
		enemy_visual_update(bs);
	}

	// Reset challenge flag
	bs->miscBotFlagsTimer = level.time;
	bs->MiscBotFlags &= ~BOTFLAG_SABERCHALLENGED;

	return qtrue;
}

//Scans for the given wp on the Close List and returns it's CloseList position.
//Returns -1 if not found.
static int FindCloseList(int wpNum)
{
	int i;
	for (i = 0; i < MAX_WPARRAY_SIZE && CloseList[i].wpNum != -1; i++)
	{
		if (CloseList[i].wpNum == wpNum)
		{
			return i;
		}
	}
	return -1;
}

//Remove the first element from the OpenList.
static void RemoveFirstOpenList(void)
{
	int i = 0;
	for (i = 1; i < MAX_WPARRAY_SIZE + 1 && OpenList[i].wpNum != -1; i++)
	{
	}

	i--;

	if (OpenList[1].wpNum == OpenList[i].wpNum)
	{//the first slot is the only thing on the list. blank it.
		OpenList[1].f = -1;
		OpenList[1].g = -1;
		OpenList[1].h = -1;
		OpenList[1].pNum = -1;
		OpenList[1].wpNum = -1;
		return;
	}

	//shift last entry to start
	OpenList[1].f = OpenList[i].f;
	OpenList[1].g = OpenList[i].g;
	OpenList[1].h = OpenList[i].h;
	OpenList[1].pNum = OpenList[i].pNum;
	OpenList[1].wpNum = OpenList[i].wpNum;

	OpenList[i].f = -1;
	OpenList[i].g = -1;
	OpenList[i].h = -1;
	OpenList[i].pNum = -1;
	OpenList[i].wpNum = -1;

	while ((OpenList[i].f >= OpenList[i * 2].f && OpenList[i * 2].wpNum != -1)
		|| (OpenList[i].f >= OpenList[i * 2 + 1].f && OpenList[i * 2 + 1].wpNum != -1))
	{
		if ((OpenList[i * 2].f < OpenList[i * 2 + 1].f) || OpenList[i * 2 + 1].wpNum == -1)
		{
			float ftemp = OpenList[i * 2].f;
			float gtemp = OpenList[i * 2].g;
			float htemp = OpenList[i * 2].h;
			int pNumtemp = OpenList[i * 2].pNum;
			int wptemp = OpenList[i * 2].wpNum;

			OpenList[i * 2].f = OpenList[i].f;
			OpenList[i * 2].g = OpenList[i].g;
			OpenList[i * 2].h = OpenList[i].h;
			OpenList[i * 2].pNum = OpenList[i].pNum;
			OpenList[i * 2].wpNum = OpenList[i].wpNum;

			OpenList[i].f = ftemp;
			OpenList[i].g = gtemp;
			OpenList[i].h = htemp;
			OpenList[i].pNum = pNumtemp;
			OpenList[i].wpNum = wptemp;

			i = i * 2;
		}
		else if (OpenList[i * 2 + 1].wpNum != -1)
		{
			float ftemp = OpenList[i * 2 + 1].f;
			float gtemp = OpenList[i * 2 + 1].g;
			float htemp = OpenList[i * 2 + 1].h;
			int pNumtemp = OpenList[i * 2 + 1].pNum;
			int wptemp = OpenList[i * 2 + 1].wpNum;

			OpenList[i * 2 + 1].f = OpenList[i].f;
			OpenList[i * 2 + 1].g = OpenList[i].g;
			OpenList[i * 2 + 1].h = OpenList[i].h;
			OpenList[i * 2 + 1].pNum = OpenList[i].pNum;
			OpenList[i * 2 + 1].wpNum = OpenList[i].wpNum;

			OpenList[i].f = ftemp;
			OpenList[i].g = gtemp;
			OpenList[i].h = htemp;
			OpenList[i].pNum = pNumtemp;
			OpenList[i].wpNum = wptemp;

			i = i * 2 + 1;
		}
		else
		{
			G_Printf("Something went wrong in RemoveFirstOpenList().\n");
			return;
		}
	}
	//sorting complete
	return;
}

static qboolean OpenListEmpty(void)
{
	//since we're using a binary heap, in theory, if the first slot is empty, the heap
	//is empty.
	if (OpenList[1].wpNum != -1)
	{
		return qfalse;
	}

	return qtrue;
}

//Scans for the given wp on the Open List and returns it's OpenList position.
//Returns -1 if not found.
static int FindOpenList(int wpNum)
{
	int i;
	for (i = 1; i < MAX_WPARRAY_SIZE + 1 && OpenList[i].wpNum != -1; i++)
	{
		if (OpenList[i].wpNum == wpNum)
		{
			return i;
		}
	}
	return -1;
}

//Adds a given OpenList wp to the closed list
static void AddCloseList(int openListpos)
{
	int i;
	if (OpenList[openListpos].wpNum != -1)
	{
		for (i = 0; i < MAX_WPARRAY_SIZE; i++)
		{
			if (CloseList[i].wpNum == -1)
			{//open slot, fill it.  heheh.
				CloseList[i].f = OpenList[openListpos].f;
				CloseList[i].g = OpenList[openListpos].g;
				CloseList[i].h = OpenList[openListpos].h;
				CloseList[i].pNum = OpenList[openListpos].pNum;
				CloseList[i].wpNum = OpenList[openListpos].wpNum;
				return;
			}
		}
		return;
	}
	return;
}

static void ClearRoute(int Route[MAX_WPARRAY_SIZE])
{
	int i;
	for (i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		Route[i] = -1;
	}
}

static void AddtoRoute(int wpNum, int Route[MAX_WPARRAY_SIZE])
{
	int i;
	for (i = 0; i < MAX_WPARRAY_SIZE && Route[i] != -1; i++)
	{
	}

	if (Route[i] == -1 && i < MAX_WPARRAY_SIZE)
	{//found the first empty slot
		while (i > 0)
		{
			Route[i] = Route[i - 1];
			i--;
		}
	}
	else
	{
		return;
	}
	if (i == 0)
	{
		Route[0] = wpNum;
	}
}

//find a given wpNum on the given route and return it's address.  return -1 if not on route.
//use wpNum = -1 to find the last wp on route.
static int FindOnRoute(int wpNum, int Route[MAX_WPARRAY_SIZE])
{
	int i;
	for (i = 0; i < MAX_WPARRAY_SIZE && Route[i] != wpNum; i++)
	{
	}

	//Special find end route command stuff
	if (wpNum == -1)
	{
		i--;
		if (Route[i] != -1)
		{//found it
			return i;
		}

		//otherwise, this is a empty route list
		return -1;
	}

	if (wpNum == Route[i])
	{//Success!
		return i;
	}

	//Couldn't find it
	return -1;
}

//Copy Route
static void CopyRoute(bot_route_t routesource, bot_route_t routedest)
{
	int i;
	for (i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		routedest[i] = routesource[i];
	}
}

static float RandFloat(float min, float max)
{
	return ((rand() * (max - min)) / (float)RAND_MAX) + min;
}

static float RouteRandomize(bot_state_t* bs, float DestDist)
{//this function randomizes the h value (distance to target location) to make the
	//bots take a random path instead of always taking the shortest route.
	//This should vary based on situation to prevent the bots from taking weird routes
	//for inapproprate situations.
	if (!carrying_cap_objective(bs))
	{//trying to capture something.  Fairly random paths to mix up the defending team.
		return DestDist * RandFloat(.5, 1.5);
	}

	//return shortest distance.
	return DestDist;
}

//Add this wpNum to the open list.
static void AddOpenList(bot_state_t* bs, int wpNum, int parent, int end)
{
	int i;
	float f;
	float g;
	float h;

	if (gWPArray[wpNum]->flags & WPFLAG_REDONLY
		&& g_entities[bs->client].client->sess.sessionTeam != TEAM_RED)
	{//red only wp, can't use
		return;
	}

	if (gWPArray[wpNum]->flags & WPFLAG_BLUEONLY
		&& g_entities[bs->client].client->sess.sessionTeam != TEAM_BLUE)
	{//blue only wp, can't use
		return;
	}

	if (parent != -1 && gWPArray[wpNum]->flags & WPFLAG_JUMP)
	{
		if (force_jump_needed(gWPArray[parent]->origin, gWPArray[wpNum]->origin) > bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION])
		{//can't make this jump with our level of Force Jump
			return;
		}
	}

	if (parent == -1)
	{//This is the start point, just use zero for g
		g = 0;
	}
	else if (wpNum == parent + 1)
	{
		if (gWPArray[wpNum]->flags & WPFLAG_ONEWAY_BACK)
		{//can't go down this one way
			return;
		}
		i = FindCloseList(parent);
		if (i != -1)
		{
			g = CloseList[i].g + gWPArray[parent]->disttonext;
		}
		else
		{
			G_Printf("Couldn't find parent on CloseList in AddOpenList().  This shouldn't happen.\n");
			return;
		}
	}
	else if (wpNum == parent - 1)
	{
		if (gWPArray[wpNum]->flags & WPFLAG_ONEWAY_FWD)
		{//can't go down this one way
			return;
		}
		i = FindCloseList(parent);
		if (i != -1)
		{
			g = CloseList[i].g + gWPArray[wpNum]->disttonext;
		}
		else
		{
			G_Printf("Couldn't find parent on CloseList in AddOpenList().  This shouldn't happen.\n");
			return;
		}
	}
	else
	{//nonsequencal parent/wpNum
		//don't try to go thru oneways when you're doing neighbor moves
		if ((gWPArray[wpNum]->flags & WPFLAG_ONEWAY_FWD) || (gWPArray[wpNum]->flags & WPFLAG_ONEWAY_BACK))
		{
			return;
		}

		i = FindCloseList(parent);
		if (i != -1)
		{
			g = OpenList[i].g + Distance(gWPArray[wpNum]->origin, gWPArray[parent]->origin);
		}
		else
		{
			G_Printf("Couldn't find parent on CloseList in AddOpenList().  This shouldn't happen.\n");
			return;
		}
	}

	//Find first open slot or the slot that this wpNum already occupies.
	for (i = 1; i < MAX_WPARRAY_SIZE + 1 && OpenList[i].wpNum != -1; i++)
	{
		if (OpenList[i].wpNum == wpNum)
		{
			break;
		}
	}

	h = Distance(gWPArray[wpNum]->origin, gWPArray[end]->origin);

	//add a bit of a random factor to h to make the bots take a variety of paths.
	h = RouteRandomize(bs, h);

	f = g + h;
	OpenList[i].f = f;
	OpenList[i].g = g;
	OpenList[i].h = h;
	OpenList[i].pNum = parent;
	OpenList[i].wpNum = wpNum;

	while (OpenList[i].f <= OpenList[i / 2].f && i != 1)
	{//binary parent has higher f than child
		float ftemp = OpenList[i / 2].f;
		float gtemp = OpenList[i / 2].g;
		float htemp = OpenList[i / 2].h;
		int pNumtemp = OpenList[i / 2].pNum;
		int wptemp = OpenList[i / 2].wpNum;

		OpenList[i / 2].f = OpenList[i].f;
		OpenList[i / 2].g = OpenList[i].g;
		OpenList[i / 2].h = OpenList[i].h;
		OpenList[i / 2].pNum = OpenList[i].pNum;
		OpenList[i / 2].wpNum = OpenList[i].wpNum;

		OpenList[i].f = ftemp;
		OpenList[i].g = gtemp;
		OpenList[i].h = htemp;
		OpenList[i].pNum = pNumtemp;
		OpenList[i].wpNum = wptemp;

		i = i / 2;
	}
	return;
}

//Find the ideal (shortest) route between the start wp and the end wp
//badwp is for situations where you need to recalc a path when you dynamically discover
//that a wp is bad (door locked, blocked, etc).
//doRoute = actually set botRoute
static float FindIdealPathtoWP(bot_state_t* bs, int start, int end, int badwp, bot_route_t Route)
{
	int i;
	float dist = -1;

	if (bs->PathFindDebounce > level.time)
	{//currently debouncing the path finding to prevent a massive overload of AI thinking
		//in weird situations, like when the target near a waypoint where the bot can't
		//get to (like in an area where you need a specific force power to get to).
		return -1;
	}

	if (start == end)
	{
		ClearRoute(Route);
		AddtoRoute(end, Route);
		return 0;
	}

	//reset node lists
	for (i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		OpenList[i].wpNum = -1;
		OpenList[i].f = -1;
		OpenList[i].g = -1;
		OpenList[i].h = -1;
		OpenList[i].pNum = -1;
	}

	for (i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		CloseList[i].wpNum = -1;
		CloseList[i].f = -1;
		CloseList[i].g = -1;
		CloseList[i].h = -1;
		CloseList[i].pNum = -1;
	}

	AddOpenList(bs, start, -1, end);

	while (!OpenListEmpty() && FindOpenList(end) == -1)
	{//open list not empty
		//we're using a binary pile so the first slot always has the lowest f score
		AddCloseList(1);
		i = OpenList[1].wpNum;
		RemoveFirstOpenList();

		//Add surrounding nodes
		if (gWPArray[i + 1] && gWPArray[i + 1]->inuse)
		{
			if (gWPArray[i]->disttonext < MAX_NODE_DIS
				&& FindCloseList(i + 1) == -1 && i + 1 != badwp)
			{//Add next sequencial node
				AddOpenList(bs, i + 1, i, end);
			}
		}

		if (i > 0)
		{
			if (gWPArray[i - 1]->disttonext < MAX_NODE_DIS && gWPArray[i - 1]->inuse
				&& FindCloseList(i - 1) == -1 && i - 1 != badwp)
			{//Add previous sequencial node
				AddOpenList(bs, i - 1, i, end);
			}
		}

		if (gWPArray[i]->neighbornum)
		{//add all the wp's neighbors to the list
			int x;
			for (x = 0; x < gWPArray[i]->neighbornum; x++)
			{
				if (x != badwp && FindCloseList(gWPArray[i]->neighbors[x].num) == -1)
				{
					AddOpenList(bs, gWPArray[i]->neighbors[x].num, i, end);
				}
			}
		}
	}

	i = FindOpenList(end);

	if (i != -1)
	{//we have a valid route to the end point
		ClearRoute(Route);
		AddtoRoute(end, Route);
		dist = OpenList[i].g;
		i = OpenList[i].pNum;
		i = FindCloseList(i);
		while (i != -1)
		{
			AddtoRoute(CloseList[i].wpNum, Route);
			i = FindCloseList(CloseList[i].pNum);
		}
		//only have the debouncer when we fail to find a route.
		bs->PathFindDebounce = level.time;
		return dist;
	}
	bs->PathFindDebounce = level.time + 3000;  //try again in 3 seconds.

	return -1;
}

static void ResetWPTimers(bot_state_t* bs)
{
	if ((bs->wpCurrent->flags & WPFLAG_WAITFORFUNC)
		|| (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
		|| (bs->wpCurrent->flags & WPFLAG_DESTROY_FUNCBREAK)
		|| (bs->wpCurrent->flags & WPFLAG_FORCEPUSH)
		|| (bs->wpCurrent->flags & WPFLAG_FORCEPULL))
	{//it's an elevator or something waypoint time needs to be longer.
		bs->wpSeenTime = level.time + 30000;
		bs->wpTravelTime = level.time + 30000;
	}
	else if (bs->wpCurrent->flags & WPFLAG_NOVIS)
	{
		//10 sec
		bs->wpSeenTime = level.time + 10000;
		bs->wpTravelTime = level.time + 10000;
	}
	else
	{
		//3 sec visual time
		bs->wpSeenTime = level.time + 3000;

		//10 sec move time
		bs->wpTravelTime = level.time + 10000;
	}

	if (bs->wpCurrent->index == bs->wpDestination->index)
	{//just touched our destination node
		bs->wpTouchedDest = qtrue;
	}
	else
	{//reset the final node touched flag
		bs->wpTouchedDest = qfalse;
	}

	bs->AltRouteCheck = qfalse;
	bs->DontSpamPushPull = 0;
}

// Simple helper: move directly toward DestPosition (no waypoint logic).
// This does NOT call EA_Move itself; it just sets goalPosition/goalMovedir.
static void BotMoveDirect(bot_state_t* bs, qboolean strafe)
{
	vec3_t dir = {0, 0, 0};

	// Set where we ultimately want to go this frame
	VectorCopy(bs->DestPosition, bs->goalPosition);

	// Compute movement direction
	VectorSubtract(bs->goalPosition, bs->origin, dir);
	if (VectorNormalize(dir) == 0.0f)
	{
		// Already at (or extremely close to) DestPosition
		VectorClear(bs->goalMovedir);
		return;
	}

	VectorCopy(dir, bs->goalMovedir);

	// If you later want strafe behavior, you can hook it here
	// (e.g. set flags or adjust goalMovedir slightly).
	(void)strafe;
}

//just like GetNearestVisibleWP except without visiblity checks
static int GetNearestWP(bot_state_t* bs, vec3_t org, int badwp)
{
	int i;
	float bestdist;
	float flLen;
	int bestindex;
	vec3_t a;

	if (RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		//We're not doing traces!
		bestdist = 99999;
	}
	bestindex = -1;

	for (i = 0; i < gWPNum; i++)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
		{
			if (bs)
			{//check to make sure that this bot's team can use this waypoint
				if (gWPArray[i]->flags & WPFLAG_REDONLY
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_RED)
				{//red only wp, can't use
					continue;
				}

				if (gWPArray[i]->flags & WPFLAG_BLUEONLY
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_BLUE)
				{//blue only wp, can't use
					continue;
				}
			}

			VectorSubtract(org, gWPArray[i]->origin, a);
			flLen = VectorLength(a);

			if (gWPArray[i]->flags & WPFLAG_WAITFORFUNC
				|| (gWPArray[i]->flags & WPFLAG_NOMOVEFUNC)
				|| (gWPArray[i]->flags & WPFLAG_DESTROY_FUNCBREAK)
				|| (gWPArray[i]->flags & WPFLAG_FORCEPUSH)
				|| (gWPArray[i]->flags & WPFLAG_FORCEPULL))
			{//boost the distance for these waypoints so that we will try to avoid using them
				//if at all possible
				flLen = flLen + 500;
			}

			if (flLen < bestdist)
			{
				bestdist = flLen;
				bestindex = i;
			}
		}
	}

	return bestindex;
}

qboolean BotOrderFlank(bot_state_t* bs)
{
	if (!bs->currentEnemy)
		return qfalse;

	// Reset forced movement
	bs->forceMove_Forward = 0;
	bs->forceMove_Right = 0;
	bs->forceMove_Up = 0;

	float dist = bs->frame_Enemy_Len;

	// Maintain medium range
	if (dist < 300)
		bs->forceMove_Forward = -1;   // back up
	else
		bs->forceMove_Forward = 1;    // advance

	// Flanking movement
	bs->forceMove_Right = (rand() % 2) ? 1 : -1;

	// Fire weapon
	bs->doAttack = qtrue;

	// Occasional dodge roll (non-saber only)
	if (bs->cur_ps.weapon != WP_SABER &&
		bs->frame_Enemy_Vis &&
		(rand() % 100) < 10)
	{
		bs->doBotKick = qtrue;
	}
	else
	{
		bs->doBotKick = qfalse;
	}

	bs->doJump = qfalse;

	return qtrue;
}

//get out of the way of allies if they're close
qboolean DontBlockAllies(bot_state_t* bs)
{
	int i;
	for (i = 0; i < level.maxclients; i++)
	{
		if (i != bs->client //not the bot
			&& g_entities[i].inuse && g_entities[i].client //valid player
			&& g_entities[i].client->pers.connected == CON_CONNECTED  //who is connected
			&& !(g_entities[i].s.eFlags & EF_DEAD) //and isn't dead
			&& g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.sessionTeam  //is on our team
			&& Distance(g_entities[i].client->ps.origin, bs->origin) < BLOCKALLIESDISTANCE)  //and we're too close to them.
		{//on your team and too close
			vec3_t moveDir, DestOrigin;
			VectorSubtract(bs->origin, g_entities[i].client->ps.origin, moveDir);
			VectorAdd(bs->origin, moveDir, DestOrigin);
			bot_move(bs, DestOrigin, qfalse, qfalse);
			return qtrue;
		}
	}
	return qfalse;
}

//Attack an enemy or track after them if you loss them.
static void BotTrackAttack(bot_state_t* bs)
{
	vec_t distance = Distance(bs->origin, bs->lastEnemySpotted);
	if (bs->frame_Enemy_Vis || bs->enemySeenTime > level.time)
	{//attack!
		bs->botBehave = BBEHAVE_ATTACK;
		VectorClear(bs->DestPosition);
		bs->DestIgnore = -1;
		return;
	}
	else if (distance < BOT_WEAPTOUCH_DISTANCE)
	{//do a visual scan
		if (bs->doVisualScan && bs->VisualScanTime < level.time)
		{//no dice.
			bs->doVisualScan = qfalse;
			bs->currentEnemy = NULL;
			bs->botBehave = BBEHAVE_STILL;
			return;
		}
		else
		{//try looking around for 5 seconds
			VectorCopy(bs->lastEnemyAngles, bs->VisualScanDir);
			if (!bs->doVisualScan)
			{
				bs->doVisualScan = qtrue;
				bs->VisualScanTime = level.time + 5000;
			}
			bs->botBehave = BBEHAVE_VISUALSCAN;
			return;
		}
	}
	else
	{//lost him, go to the last seen location and see if we can find them
		bs->botBehave = BBEHAVE_MOVETO;
		VectorCopy(bs->lastEnemySpotted, bs->DestPosition);
		bs->DestIgnore = bs->currentEnemy->s.number;
		return;
	}
}

//defend given entity from attack
qboolean BotDefend(bot_state_t* bs, gentity_t* defendEnt)
{
	vec3_t defendOrigin;
	float dist;

	FindOrigin(defendEnt, defendOrigin);

	if (strcmp(defendEnt->classname, "func_breakable") == 0
		&& defendEnt->paintarget
		&& strcmp(defendEnt->paintarget, "shieldgen_underattack") == 0)
	{//dirty hack to get the bots to attack the shield generator on siege_hoth
		VectorSet(defendOrigin, -369, 858, -231);
	}

	dist = Distance(bs->origin, defendOrigin);

	if (bs->currentEnemy)
	{//see an enemy
		if (dist > DEFEND_MAXDISTANCE)
		{//attack move back into the defend range
			VectorCopy(defendOrigin, bs->DestPosition);
			bs->DestIgnore = defendEnt->s.number;
			bot_behave_attack_move(bs);
		}
		else
		{//just attack them
			VectorClear(bs->DestPosition);
			bs->DestIgnore = -1;
			BotTrackAttack(bs);
		}
	}
	else
	{//don't see an enemy
		if (DontBlockAllies(bs))
		{
		}
		else if (dist < DEFEND_MINDISTANCE)
		{//just stand around and wait
			VectorClear(bs->DestPosition);
			bs->DestIgnore = -1;
			bs->botBehave = BBEHAVE_STILL;
		}
		else
		{//move closer to defend target
			VectorCopy(defendOrigin, bs->DestPosition);
			bs->DestIgnore = defendEnt->s.number;
			bs->botBehave = BBEHAVE_MOVETO;
		}
	}
	return qtrue;
}

static gentity_t* FindGoalPointEnt(bot_state_t* bs)
{//Find the goalpoint entity for this capture objective point
	if (level.gametype == GT_SIEGE)
	{
		return G_Find(NULL, FOFS(targetname), bs->tacticEntity->goaltarget);
	}
	else
	{//Capture the flag
		char* c;
		if (g_entities[bs->client].client->sess.sessionTeam == TEAM_RED)
		{
			c = "team_CTF_redflag";
		}
		else
		{
			c = "team_CTF_blueflag";
		}
		return G_Find(NULL, FOFS(classname), c);
	}
}

static int FlagColorforObjective(bot_state_t* bs)
{
	if (g_entities[bs->client].client->sess.sessionTeam == TEAM_RED)
	{
		if (bs->objectiveType == OT_CAPTURE)
		{
			return PW_BLUEFLAG;
		}
		else
		{
			return PW_REDFLAG;
		}
	}
	else
	{
		if (bs->objectiveType == OT_CAPTURE)
		{
			return PW_REDFLAG;
		}
		else
		{
			return PW_BLUEFLAG;
		}
	}
}

static qboolean CapObjectiveIsCarried(bot_state_t* bs)
{//check to see if the current objective capture item is being carried
	if (level.gametype == GT_SIEGE)
	{
		if (bs->tacticEntity->genericValue2)
			return qtrue;
	}
	else
	{//capture the flag types
		int flagpr, i;
		gentity_t* carrier;

		//Set which flag powerup we're looking for
		flagpr = FlagColorforObjective(bs);

		// check for carrier on desired flag
		for (i = 0; i < level.maxclients; i++)
		{
			carrier = g_entities + i;
			if (carrier->inuse && carrier->client->ps.powerups[flagpr])
				return qtrue;
		}
	}

	return qfalse;
}

static qboolean CarryingCapObjective(bot_state_t* bs)
{//Carrying the Capture Objective?
	if (level.gametype == GT_SIEGE)
	{
		if (bs->tacticEntity && bs->client == bs->tacticEntity->genericValue8)
			return qtrue;
	}
	else
	{
		if (g_entities[bs->client].client->ps.powerups[PW_REDFLAG]
			|| g_entities[bs->client].client->ps.powerups[PW_BLUEFLAG])
			return qtrue;
	}
	return qfalse;
}

//Find the favorite weapon for this range.
static int FindWeaponforRange(bot_state_t* bs, float range)
{
	int bestweap = -1;
	int bestfav = -1;
	int weapdist;
	int i;

	//try to find the fav weapon for this attack range
	for (i = 0; i < WP_NUM_WEAPONS; i++)
	{
		if (bs->cur_ps.ammo[weaponData[i].ammoIndex] < weaponData[i].energyPerShot
			|| !(bs->cur_ps.stats[STAT_WEAPONS] & (1 << i)))
		{//check to see if we have this weapon or enough ammo for it.
			continue;
		}

		if (i == WP_SABER)
		{//hack to prevent the player from switching away from the saber when close to
			//target
			weapdist = 300;
		}
		else
		{
			weapdist = MaximumAttackDistance[i];
			//weapdist = IdealAttackDistance[i] * 1.1;
		}

		if (range < weapdist && bs->botWeaponWeights[i] > bestfav)
		{
			bestweap = i;
			bestfav = bs->botWeaponWeights[i];
		}
	}

	return bestweap;
}

//should we be "leading" our aim with this weapon? And if
//so, by how much?
static float BotWeaponCanLead(bot_state_t* bs)
{
	int weap = bs->cur_ps.weapon;

	if (weap == WP_BRYAR_PISTOL)
	{
		return 0.5;
	}
	if (weap == WP_BLASTER)
	{
		return 0.35;
	}
	if (weap == WP_BOWCASTER)
	{
		return 0.5;
	}
	if (weap == WP_REPEATER)
	{
		return 0.45;
	}
	if (weap == WP_THERMAL)
	{
		return 0.5;
	}
	if (weap == WP_DEMP2)
	{
		return 0.35;
	}
	if (weap == WP_ROCKET_LAUNCHER)
	{
		return 0.7;
	}

	return 0;
}

//attack/fire at currentEnemy while moving towards DestPosition
static void BotBehave_AttackMove(bot_state_t* bs)
{
	vec3_t viewDir;
	vec3_t ang;
	vec3_t enemyOrigin;

	//switch to an approprate weapon
	int desiredweap;
	float range;

	float leadamount; //lead amount

	if (!bs->frame_Enemy_Vis && bs->enemySeenTime < level.time)
	{//lost track of enemy
		bs->currentEnemy = NULL;
		return;
	}

	FindOrigin(bs->currentEnemy, enemyOrigin);

	range = TargetDistance(bs, bs->currentEnemy, enemyOrigin);

	desiredweap = FindWeaponforRange(bs, range);

	if (desiredweap != bs->virtualWeapon && desiredweap != -1)
	{//need to switch to desired weapon otherwise stay with what you go
		BotSelectChoiceWeapon(bs, desiredweap, qtrue);
	}

	//move towards DestPosition
	bot_move(bs, qfalse, qfalse, qfalse);

	if (bs->wpSpecial)
	{//in special wp move, don't do interrupt it.
		return;
	}

	//adjust angle for target leading.
	leadamount = BotWeaponCanLead(bs);

	bot_aim_leading(bs, enemyOrigin, leadamount);

	//set viewangle
	VectorSubtract(enemyOrigin, bs->eye, viewDir);

	vectoangles(viewDir, ang);
	VectorCopy(ang, bs->goalAngles);

	if (bs->frame_Enemy_Vis && bs->cur_ps.weapon == bs->virtualWeapon && range < MaximumAttackDistance[bs->virtualWeapon]
		&& range > MinimumAttackDistance[bs->virtualWeapon]
		//if(bs->cur_ps.weapon == bs->virtualWeapon && range <= IdealAttackDistance[bs->virtualWeapon] * 1.1
		&& (in_field_of_vision(bs->viewangles, 30, ang)
			|| (bs->virtualWeapon == WP_SABER && in_field_of_vision(bs->viewangles, 100, ang))))
	{//don't attack unless you're inside your AttackDistance band and actually pointing at your enemy.
		//This is to prevent the bots from attackmoving with the saber @ 500 meters. :)
		trap_EA_Attack(bs->client);
		if (bs->virtualWeapon == WP_SABER)
		{//only walk while attacking with the saber.
			bs->doWalk = qtrue;
		}
	}
}

static void BotGrabNearByItems(bot_state_t* bs)
{//go around and pick up nearby items that we don't have.
	gentity_t* ent = NULL;
	gentity_t* closestEnt = NULL;
	float		closestDist = 9999;
	vec3_t		closestOrigin;
	int			i;

	//find the closest item we'd like to pick up
	for (i = MAX_CLIENTS; i < level.num_entities; i++)
	{
		vec3_t	entOrigin;
		float	entDist;
		ent = &g_entities[i];

		if (ent->s.eType != ET_ITEM || !BG_CanItemBeGrabbed(level.gametype, &ent->s, &bs->cur_ps))
		{//not something we can pick up.
			continue;
		}

		if (bg_itemlist[ent->s.modelindex].giType == IT_TEAM)
		{//ignore team items, like flags.
			continue;
		}

		if ((ent->s.eFlags & EF_ITEMPLACEHOLDER) || (ent->s.eFlags & EF_NODRAW))
		{//item has been picked up already
			continue;
		}

		FindOrigin(ent, entOrigin);

		entDist = Distance(bs->origin, entOrigin);

		if (entDist < closestDist)
		{
			closestDist = entDist;
			closestEnt = ent;
			VectorCopy(entOrigin, closestOrigin);
		}
	}

	if (closestEnt)
	{//found a nearby object that we can pick up.
		VectorCopy(closestOrigin, bs->DestPosition);
		bs->DestIgnore = closestEnt->s.number;
		if (bs->currentEnemy)
		{//have a local enemy, attackmove
			BotBehave_AttackMove(bs);
		}
		else
		{//normal move
			bs->botBehave = BBEHAVE_MOVETO;
		}
	}
}

static gentity_t* CapObjectiveCarrier(bot_state_t* bs)
{//Returns the gentity for the current carrier of the capture objective
	if (level.gametype == GT_SIEGE)
	{
		return &g_entities[bs->tacticEntity->genericValue8];
	}
	else
	{
		int flagpr, i;
		gentity_t* carrier;

		//Set which flag powerup we're looking for
		flagpr = FlagColorforObjective(bs);

		// find attacker's team's flag carrier
		for (i = 0; i < level.maxclients; i++)
		{
			carrier = g_entities + i;
			if (carrier->inuse && carrier->client->ps.powerups[flagpr])
				return carrier;
		}
	}

	return NULL;
}

extern teamgame_t teamgame;
static void objectiveType_Capture(bot_state_t* bs)
{
	if (!bs->tacticEntity)
	{//This is bad
		G_Printf("This is bad, we've lost out entity in objectiveType_Capture.\n");
		return;
	}

	if (CapObjectiveIsCarried(bs))
	{//objective already being carried
		if (CarryingCapObjective(bs))
		{//I'm carrying the flag.
			//find the goaltarget
			gentity_t* goal = NULL;
			goal = FindGoalPointEnt(bs);
			if (goal && !(bs->MiscBotFlags & BOTFLAG_REACHEDCAPTUREPOINT))
			{//found goal position and we haven't already visited the goal point
				vec3_t goalorigin;
				FindOrigin(goal, goalorigin);
				if (level.gametype != GT_SIEGE && DistanceHorizontal(goalorigin, bs->origin) < BOTAI_CAPTUREDISTANCE)
				{//we've touched the goal point and haven't captured the objective.
					//This means that our team's flag isn't there. Flip the waiting
					//for flag behavior
					bs->MiscBotFlags |= BOTFLAG_REACHEDCAPTUREPOINT;
				}

				VectorCopy(goalorigin, bs->DestPosition);
				bs->DestIgnore = goal->s.number;
				if (bs->currentEnemy)
				{
					bot_behave_attack_move(bs);
				}
				else
				{
					bs->botBehave = BBEHAVE_MOVETO;
				}
			}
			else
			{//we've already visited our capture point or the capture point isn't valid.
				//Do our "waiting for flag return" behavior here.

				//check to see if our flag has been returned.
				if (g_entities[bs->client].client->sess.sessionTeam == TEAM_RED)
				{
					if (teamgame.redStatus == FLAG_ATBASE)
					{
						bs->MiscBotFlags &= ~BOTFLAG_REACHEDCAPTUREPOINT;
						return;
					}
				}
				else
				{
					if (teamgame.blueStatus == FLAG_ATBASE)
					{
						bs->MiscBotFlags &= ~BOTFLAG_REACHEDCAPTUREPOINT;
						return;
					}
				}

				//ok, flag hasn't returned, wonder around and grab items in the area
				BotGrabNearByItems(bs);
			}
			return;
		}
		else
		{//someone else is covering the flag, cover them
			BotDefend(bs, CapObjectiveCarrier(bs));
			return;
		}
	}
	else
	{//not being carried
		//get the flag!
		vec3_t origin;
		FindOrigin(bs->tacticEntity, origin);
		VectorCopy(origin, bs->DestPosition);
		bs->DestIgnore = bs->tacticEntity->s.number;
		if (bs->currentEnemy)
		{
			bot_behave_attack_move(bs);
		}
		else
		{
			bs->botBehave = BBEHAVE_MOVETO;
		}
		return;
	}
}

extern gentity_t* droppedBlueFlag;
extern gentity_t* droppedRedFlag;
static gentity_t* FindFlag(bot_state_t* bs)
{//find the flag item entity for this bot's objective entity
	if (FlagColorforObjective(bs) == PW_BLUEFLAG)
	{//blue flag
		return droppedBlueFlag;
	}
	else
	{
		return droppedRedFlag;
	}

	//bad flag?!
	return NULL;
}
static void objectiveType_Attack(bot_state_t* bs, gentity_t* target);
//Prevent this objective from getting captured.
static void objectiveType_DefendCapture(bot_state_t* bs)
{
	if (!CapObjectiveIsCarried(bs))
	{
		if (level.gametype == GT_SIEGE)
		{
			//turns out that you don't normally recap capturable siege items.
			BotDefend(bs, bs->tacticEntity);
		}
		else
		{//flag of some sort, touch it if it is dropped.  Otherwise, just defend it.
			gentity_t* flag = FindFlag(bs);
			if (flag && flag->flags & FL_DROPPED_ITEM)
			{//dropped, touch it
				vec3_t origin;
				FindOrigin(flag, origin);
				VectorCopy(origin, bs->DestPosition);
				bs->DestIgnore = flag->s.number;
				if (bs->currentEnemy)
				{
					bot_behave_attack_move(bs);
				}
				else
				{
					bs->botBehave = BBEHAVE_MOVETO;
				}
			}
			else
			{//objective at homebase, defend it
				BotDefend(bs, bs->tacticEntity);
			}
		}
	}
	else
	{//object has been taken, attack the carrier
		objectiveType_Attack(bs, CapObjectiveCarrier(bs));
	}
}

extern qboolean InFOV2(vec3_t origin, const gentity_t* from, int hFOV, int vFOV);
static gentity_t* ClosestItemforWeapon(bot_state_t* bs, weapon_t weapon)
{//returns the closest gentity item (perminate) for a given ammo type.
	float		bestDist = 9999999;
	gentity_t* bestItem = NULL;

	gitem_t* currentItemType;
	gentity_t* currentObject = NULL;
	float		currentDist;

	//look for the ammo type for this weapon
	currentItemType = BG_FindItemForWeapon(weapon);

	while ((currentObject = G_Find(currentObject, FOFS(classname),
		currentItemType->classname)) != NULL)
	{//scan thru the map entities until we find an gentity of this ammo item
		if (!BG_CanItemBeGrabbed(level.gametype, &currentObject->s, &bs->cur_ps))
		{//we can't pick up this item right now.
			continue;
		}

		currentDist = DistanceSquared(bs->origin, currentObject->r.currentOrigin);

		if (currentDist < bestDist)
		{//this item is closer than the current best ammo object
			if ((currentObject->flags & FL_DROPPED_ITEM)
				|| (currentObject->s.eFlags & EF_ITEMPLACEHOLDER))
			{//we're going to need a visibility check to see if we know about the status of this
				//object
				if (bot_pvs_check(bs->eye, currentObject->r.currentOrigin)
					&& InFOV2(currentObject->r.currentOrigin, &g_entities[bs->client], 100, 100)
					&& org_visible(bs->eye, currentObject->r.currentOrigin, bs->client))
				{//we can see the item
					if (currentObject->s.eFlags & EF_ITEMPLACEHOLDER)
					{//the weapon item isn't currently there, ignore.
						continue;
					}
				}
				else
				{//can't see the item
					if (currentObject->flags & FL_DROPPED_ITEM)
					{//If we can't see the item, we won't know about this dropped item.
						continue;
					}
				}
			}

			bestDist = currentDist;
			bestItem = currentObject;
		}
	}

	if (bestItem)
	{//we've found an object for this ammo type, go for it.
		return bestItem;
	}
	else
	{
		return NULL;
	}
}

static gentity_t* WantWeapon(bot_state_t* bs, qboolean setOrder, int numOfChecks)
{//This function checks to see if the bot wishes to go  particular weapon that it doesn't have.
	int favWeapon;
	int ignoreWeapons = 0;  //weapons we've already checked.
	int counter;

	if (level.gametype == GT_SIEGE
		|| level.gametype == GT_DUEL
		|| level.gametype == GT_POWERDUEL)
	{//you can't pick up new weapons in these gametypes
		return NULL;
	}

	for (counter = 0; counter < numOfChecks; counter++)
	{//keep checking until we run out of fav weapons we want to check.
		//check to see if we have our favorite weapon.
		favWeapon = favorite_weapon(bs, bs->currentEnemy, qfalse, qfalse, ignoreWeapons);

		if (!(bs->cur_ps.stats[STAT_WEAPONS] & (1 << favWeapon)))
		{//We don't have the weapon we want.
			gentity_t* item = ClosestItemforWeapon(bs, favWeapon);
			if (item)
			{
				if (setOrder)
				{//we want to switch our current tactic.
					bs->currentTactic = BOTORDER_RESUPPLY;
					bs->tacticEntity = item;
				}
				return item;
			}
		}

		//no dice, add to ignore list and retry if we want to.
		ignoreWeapons |= (1 << favWeapon);
	}

	return NULL;
}

extern gitem_t* BG_FindItemForAmmo(const ammo_t ammo);
static gentity_t* ClosestItemforAmmo(bot_state_t* bs, int ammo)
{//returns the closest gentity item (perminate) for a given ammo type.
	float		bestDist = 9999999;
	gentity_t* bestItem = NULL;

	gitem_t* currentItemType;
	gentity_t* currentObject = NULL;
	float		currentDist;

	//look for the ammo type for this weapon
	currentItemType = BG_FindItemForAmmo((ammo_t)ammo);

	while ((currentObject = G_Find(currentObject, FOFS(classname),
		currentItemType->classname)) != NULL)
	{//scan thru the map entities until we find an gentity of this ammo item
		if (!BG_CanItemBeGrabbed(level.gametype, &currentObject->s, &bs->cur_ps))
		{//we can't pick up this item.
			continue;
		}

		currentDist = DistanceSquared(bs->origin, currentObject->r.currentOrigin);

		if (currentDist < bestDist)
		{//this item is closer than the current best ammo object
			if ((currentObject->s.eFlags & EF_NODRAW) || (currentObject->flags & FL_DROPPED_ITEM))
			{//we're going to need a visibility check to see if we know about the status of this
				//object
				if (bot_pvs_check(bs->eye, currentObject->r.currentOrigin)
					&& InFOV2(currentObject->r.currentOrigin, &g_entities[bs->client], 100, 100)
					&& org_visible(bs->eye, currentObject->r.currentOrigin, bs->client))
				{//we can see the item
					if (currentObject->s.eFlags & EF_NODRAW)
					{//the ammo item isn't currently there, ignore.
						continue;
					}
				}
				else
				{//can't see the item
					if (currentObject->flags & FL_DROPPED_ITEM)
					{//If we can't see the item, we won't know about this dropped item.
						continue;
					}
				}
			}

			bestDist = currentDist;
			bestItem = currentObject;
		}
	}

	//try looking for those ammo despensor things
	currentObject = NULL;
	while ((currentObject = G_Find(currentObject, FOFS(classname),
		"misc_ammo_floor_unit")) != NULL)
	{//scan thru the map entities until we find an gentity of this ammo item
		currentDist = DistanceSquared(bs->origin, currentObject->r.currentOrigin);

		if (currentDist < bestDist)
		{//this item is closer than the current best ammo object
			bestDist = currentDist;
			bestItem = currentObject;
		}
	}

	if (bestItem)
	{//we've found an object for this ammo type, go for it.
		return bestItem;
	}
	else
	{
		return NULL;
	}
}

static gentity_t* WantAmmo(bot_state_t* bs, qboolean setOrder, int numOfChecks)
{//This function checks to see if the bot wishes to go for ammo for a particular weapon.
	//Returns true if we've decided to go after ammo
	//setOrder - sets wheither or not we should switch our current tactic if we want ammo.
	int ignoreWeapons = 0;  //weapons we've already checked.
	int counter;
	int favWeapon;
	int ammoMax;

	for (counter = 0; counter < numOfChecks; counter++)
	{//keep checking until we run out of fav weapons we want to check.
		//check our ammo on our current favorite weapon
		favWeapon = favorite_weapon(bs, bs->currentEnemy, qtrue, qfalse, ignoreWeapons);
		ammoMax = ammoData[weaponData[favWeapon].ammoIndex].max;

		if (weaponData[favWeapon].ammoIndex == AMMO_ROCKETS && level.gametype == GT_SIEGE)
		{//hack for the lower rocket amount in Siege
			ammoMax = 10;
		}

		if (weaponData[favWeapon].ammoIndex != AMMO_NONE
			&& (bs->cur_ps.ammo[weaponData[favWeapon].ammoIndex]
				< ammoMax * DESIREDAMMOLEVEL))
		{//we have less ammo than we'd like, check to see if there's ammo for this weapon on
			//this map.
			gentity_t* item = ClosestItemforAmmo(bs, weaponData[favWeapon].ammoIndex);
			if (item)
			{
				if (setOrder)
				{//we want to switch our current tactic.
					bs->currentTactic = BOTORDER_RESUPPLY;
					bs->tacticEntity = item;
				}
				return item;
			}
		}

		//no dice, add to ignore list and retry if we want to.
		ignoreWeapons |= (1 << favWeapon);
	}

	return NULL;
}

qboolean BotResupply(bot_state_t* bs, gentity_t* tacticEnt)
{//this behavior makes the bot resupply by picking up or using the tacticEntity
	if (tacticEnt)
	{//we have a valid item to use/pickup
		float dist;

		if (((tacticEnt->s.eFlags & EF_NODRAW)
			|| (tacticEnt->s.eFlags & EF_ITEMPLACEHOLDER))
			&& bot_pvs_check(bs->eye, tacticEnt->r.currentOrigin)
			&& InFOV2(tacticEnt->r.currentOrigin, &g_entities[bs->client], 100, 100)
			&& org_visible(bs->eye, tacticEnt->r.currentOrigin, bs->client))
		{//This object has been picked up since we last saw it and we can see that it's
			//been taken.  Tactic complete then.
			if (bs->currentTactic == BOTORDER_RESUPPLY)
			{//this is the current tactic, cancel orders
				bs->currentTactic = BOTORDER_NONE;
				bs->tacticEntity = NULL;
			}

			//clear out the nav destination stuff.
			VectorClear(bs->DestPosition);
			bs->DestIgnore = -1;

			if (bs->botOrder == BOTORDER_RESUPPLY)
			{//order completed
				bs->botOrder = BOTORDER_NONE;
				bs->orderEntity = NULL;
			}
			return qfalse;
		}

		dist = DistanceSquared(tacticEnt->r.currentOrigin, bs->origin);

		if (dist < (40 * 40))
		{//we're touching the item
			if (tacticEnt->r.svFlags & SVF_PLAYER_USABLE)
			{//we have to use this item to get ammo from it.
				if (tacticEnt->count && WantAmmo(bs, qfalse, TAB_FAVWEAPCARELEVEL_MAX))
				{//the ammo system isn't empty and we still want ammo, so keep taking ammo.
					vec3_t viewDir, ang;
					VectorSubtract(tacticEnt->r.currentOrigin, bs->eye, viewDir);
					vectoangles(viewDir, ang);
					VectorCopy(ang, bs->goalAngles);

					if (in_field_of_vision(bs->viewangles, 5, ang))
					{//looking at dispensor, press the use button now.
						bs->useTime = level.time + 100;
					}
					return qfalse;
				}
			}

			if (bs->currentTactic == BOTORDER_RESUPPLY)
			{//this is the current tactic, cancel orders
				//picked up our item or used it up, stop this tactic and order
				bs->currentTactic = BOTORDER_NONE;
				bs->tacticEntity = NULL;
			}

			//clear out destination data
			VectorClear(bs->DestPosition);
			bs->DestIgnore = -1;

			if (bs->botOrder == BOTORDER_RESUPPLY)
			{//order completed
				bs->botOrder = BOTORDER_NONE;
				bs->orderEntity = NULL;
			}
		}
		else
		{//keep moving towards object.
			VectorCopy(tacticEnt->r.currentOrigin, bs->DestPosition);
			bs->DestIgnore = tacticEnt->s.number;

			if (bs->currentEnemy)
			{
				bot_behave_attack_move(bs);
			}
			else
			{
				bs->botBehave = BBEHAVE_MOVETO;
			}
		}
	}
	return qtrue;
}

qboolean BotSearchAndDestroy(bot_state_t* bs)
{
	if (!bs->currentEnemy && (VectorCompare(bs->DestPosition, vec3_origin) || DistanceHorizontal(bs->origin, bs->DestPosition) < BOT_WEAPTOUCH_DISTANCE))
	{//hmmm, noone in the area and we're not already going somewhere
		//Check to see if we need some weapons or ammo
		gentity_t* desiredPickup = WantWeapon(bs, qfalse, TAB_FAVWEAPCARELEVEL_MAX);
		if (desiredPickup)
		{//want weapon, going for it.
			BotResupply(bs, desiredPickup);
			return qfalse;
		}

		desiredPickup = WantAmmo(bs, qfalse, TAB_FAVWEAPCARELEVEL_MAX);

		if (desiredPickup)
		{//want ammo, going for it. behavior is set in
			BotResupply(bs, desiredPickup);
			return qfalse;
		}
		else
		{//let's just randomly go to a spawnpoint
			gentity_t* spawnpoint;
			vec3_t temp;
			spawnpoint = SelectSpawnPoint(vec3_origin, temp, temp, level.clients[bs->client].sess.sessionTeam, qtrue);
			if (spawnpoint)
			{
				VectorCopy(spawnpoint->s.origin, bs->DestPosition);
				bs->DestIgnore = -1;
				bs->botBehave = BBEHAVE_MOVETO;
				return qfalse;
			}
			else
			{//that's not good
				bs->botBehave = BBEHAVE_STILL;
				VectorClear(bs->DestPosition);
				bs->DestIgnore = -1;
				return qfalse;
			}
		}
	}
	else if (!bs->currentEnemy && !VectorCompare(bs->DestPosition, vec3_origin))
	{//moving towards a weapon or spawnpoint
		bs->botBehave = BBEHAVE_MOVETO;
		return qfalse;
	}
	else
	{//have an enemy and can see him
		if (bs->currentEnemy != bs->tacticEntity)
		{//attacking someone other that your target
			//This should probably be some sort of attack move or something
			BotTrackAttack(bs);
			return qfalse;
		}
		else
		{
			BotTrackAttack(bs);
			return qfalse;
		}
	}
	return qtrue;
}

//determines the trigger entity and type for a seige objective
//attacker = objective attacker or defender? This is used for the recursion stuff.
//use 0 to have it be determined by the side of the info_siege_objective
static gentity_t* DetermineObjectiveType(int team, int objective, int* type, gentity_t* obj, int attacker)
{
	gentity_t* test = NULL;

	if (level.gametype != GT_SIEGE)
	{//find the flag for this objective type
		char* c;
		if (*type == OT_CAPTURE)
		{
			if (team == TEAM_RED)
			{
				c = "team_CTF_blueflag";
			}
			else
			{
				c = "team_CTF_redflag";
			}
		}
		else if (*type == OT_DEFENDCAPTURE)
		{
			if (team == TEAM_RED)
			{
				c = "team_CTF_redflag";
			}
			else
			{
				c = "team_CTF_blueflag";
			}
		}
		else
		{
			G_Printf("DetermineObjectiveType() Error: Bad ObjectiveType Given for CTF flag find.\n");
			return NULL;
		}
		test = G_Find(test, FOFS(classname), c);
		return test;
	}

	if (!obj)
	{//don't already have obj entity to scan from, find it.
		while ((obj = G_Find(obj, FOFS(classname), "info_siege_objective")) != NULL)
		{
			if (objective == obj->objective)
			{//found it
				if (!attacker)
				{//this should always be true
					if (obj->side == team)
					{//we get points from this objective, we're the attacker.
						attacker = 1;
					}
					else
					{//we're the defender
						attacker = 2;
					}
				}
				break;
			}
		}
	}

	if (!obj)
	{//hmmm, couldn't find the thing.  That's not good.
		*type = OT_NONE;
		return NULL;
	}

	//let's try back tracking and figuring out how this trigger is triggered

	//try scanning thru the target triggers first.
	while ((test = G_Find(test, FOFS(target), obj->targetname)) != NULL)
	{
		if (test->flags & FL_INACTIVE)
		{//this entity isn't active, ignore it
			continue;
		}
		else if (strcmp(test->classname, "func_breakable") == 0
			|| (strcmp(test->classname, "NPC") == 0))
		{//Destroyable objective or NPC
			if (attacker == 1)
			{//attack
				*type = OT_ATTACK;
				return test;
			}
			else if (attacker == 2)
			{//Defend this target
				*type = OT_DEFEND;
				return test;
			}
			else
			{
				G_Printf("Bad attacker state for func_breakable objective in DetermineObjectiveType().\n");
				return test;
			}
			break;
		}
		else if ((strcmp(test->classname, "trigger_multiple") == 0)
			|| (strcmp(test->classname, "target_relay") == 0)
			|| (strcmp(test->classname, "target_counter") == 0)
			|| (strcmp(test->classname, "func_usable") == 0)
			|| (strcmp(test->classname, "trigger_once") == 0))
		{//ok, you can't do something directly to a trigger_multiple or a target_relay
			//scan for whatever links to this relay
			gentity_t* triggerer = DetermineObjectiveType(team, objective, type, test, attacker);
			if (triggerer)
			{//success!
				return triggerer;
			}
			else if ((!strcmp(test->classname, "func_usable") && (test->spawnflags & 64)) || //useable by player
				(!strcmp(test->classname, "trigger_multiple") && (test->spawnflags & 4)) || //need to press the use button to work
				(!strcmp(test->classname, "trigger_once")))
			{//ok, so they aren't linked to anything, try using them directly then
				if (test->NPC_targetname)
				{//vehicle objective
					if (attacker == 1)
					{//attack
						*type = OT_VEHICLE;
						return test;
					}
					else if (attacker == 2)
					{//destroy the vehicle
						gentity_t* vehicle = NULL;
						//Find the vehicle
						while ((vehicle = G_Find(vehicle, FOFS(script_targetname), test->NPC_targetname)) != NULL)
						{
							if (vehicle->inuse && vehicle->client && vehicle->s.eType == ET_NPC &&
								vehicle->s.NPC_class == CLASS_VEHICLE && vehicle->m_pVehicle)
							{
								break;
							}
						}

						if (!vehicle)
						{//can't find the vehicle?!
							*type = OT_WAIT;
							return NULL;
						}

						test = vehicle;
						*type = OT_ATTACK;
						return test;
					}
					else
					{
						G_Printf("Bad attacker state for vehicle trigger_once objective in DetermineObjectiveType().\n");
						return test;
					}
				}
				else
				{
					if (attacker == 1)
					{//attack
						*type = OT_TOUCH;
						return test;
					}
					else if (attacker == 2)
					{//Defend this target
						*type = OT_DEFEND;
						return test;
					}
					else
					{
						G_Printf("Bad attacker state for func_usable objective in DetermineObjectiveType().\n");
						return test;
					}
				}
				break;
			}
		}
	}

	test = NULL;

	//ok, see obj is triggered by the goaltarget of a capturable misc_siege_item
	while ((test = G_Find(test, FOFS(goaltarget), obj->targetname)) != NULL)
	{
		if (strcmp(test->classname, "misc_siege_item") == 0)
		{//Destroyable objective
			if (attacker == 1)
			{//attack
				*type = OT_CAPTURE;
				return test;
			}
			else if (attacker == 2)
			{//Defend this target
				*type = OT_DEFENDCAPTURE;
				return test;
			}
			else
			{
				G_Printf("Bad attacker state for misc_siege_item objective in DetermineObjectiveType().\n");
				return test;
			}
			break;
		}
	}

	test = NULL;

	//ok, see obj is triggered by the target3 (delivery target) of a capturable misc_siege_item
	while ((test = G_Find(test, FOFS(target3), obj->targetname)) != NULL)
	{
		if (strcmp(test->classname, "misc_siege_item") == 0)
		{//capturable objective
			if (attacker == 1)
			{//attack
				*type = OT_CAPTURE;
				return test;
			}
			else if (attacker == 2)
			{//Defend this target
				*type = OT_DEFENDCAPTURE;
				return test;
			}
			else
			{
				G_Printf("Bad attacker state for misc_siege_item (target3) objective in DetermineObjectiveType().\n");
				return test;
			}
			break;
		}
	}

	test = NULL;

	//check for a destroyable misc_siege_item that triggers this objective
	while ((test = G_Find(test, FOFS(target4), obj->targetname)) != NULL)
	{
		if (strcmp(test->classname, "misc_siege_item") == 0)
		{
			if (attacker == 1)
			{//attack
				*type = OT_ATTACK;
				return test;
			}
			else if (attacker == 2)
			{//Defend this target
				*type = OT_DEFEND;
				return test;
			}
			else
			{
				G_Printf("Bad attacker state for misc_siege_item (target4) objective in DetermineObjectiveType().\n");
				return test;
			}
			break;
		}
	}

	//no dice
	*type = OT_NONE;
	return NULL;
}

static void BotBehave_Attack(bot_state_t* bs)
{
	int desiredweap = favorite_weapon(bs, bs->currentEnemy, qtrue, qtrue, 0);

	if (bs->frame_Enemy_Len > MaximumAttackDistance[desiredweap])
		//if( bs->frame_Enemy_Len > IdealAttackDistance[desiredweap] * 1.1)
	{//this should be an attack while moving function but for now we'll just use moveto
		vec3_t enemyOrigin;
		FindOrigin(bs->currentEnemy, enemyOrigin);
		VectorCopy(enemyOrigin, bs->DestPosition);
		bs->DestIgnore = bs->currentEnemy->s.number;
		BotBehave_AttackMove(bs);
		return;
	}

	//determine which weapon you want to use
	if (desiredweap != bs->virtualWeapon)
	{//need to switch to desired weapon
		BotSelectChoiceWeapon(bs, desiredweap, qtrue);
	}

	//we're going to go get in close so null out the wpCurrent so it will update when we're
	//done.
	bs->wpCurrent = NULL;

	//use basic attack
	bot_behave_attack_basic(bs, bs->currentEnemy);
}

//use/touch the given objective
static void objectiveType_Touch(bot_state_t* bs)
{
	vec3_t objOrigin;

	FindOrigin(bs->tacticEntity, objOrigin);

	if (!G_PointInBounds(bs->origin, bs->tacticEntity->r.absmin, bs->tacticEntity->r.absmax))
	{//move closer
		VectorCopy(objOrigin, bs->DestPosition);
		bs->DestIgnore = bs->tacticEntity->s.number;
		if (bs->currentEnemy)
		{//have a local enemy, attackmove
			BotBehave_Attack(bs);
		}
		else
		{//normal move
			bs->botBehave = BBEHAVE_MOVETO;
		}
	}
	else
	{//in range hold down use
		bs->useTime = level.time + 100;
		if (bs->tacticEntity->spawnflags & 2 /*FACING*/)
		{//you have to face in the direction of the trigger to have it work
			vec3_t ang;
			vectoangles(bs->tacticEntity->movedir, ang);
			VectorCopy(ang, bs->goalAngles);
		}
	}
}

extern char gObjectiveCfgStr[1024];
static qboolean ObjectiveStillActive(int objective)
{//Is the given Objective for the given team still active?
	int i = 0;
	int objectiveNum = 0;

	if (objective <= 0)
	{//bad objective number
		return qfalse;
	}

	while (gObjectiveCfgStr[i])
	{
		if (gObjectiveCfgStr[i] == '|')
		{ //switch over to team2, this is the next section
			objectiveNum = 0;
		}
		else if (gObjectiveCfgStr[i] == '-')
		{
			objectiveNum++;
			i++;
			if (gObjectiveCfgStr[i] == '0' && objectiveNum == objective)
			{//tactic still active.
				return qtrue;
			}
		}
		i++;
	}

	return qfalse;
}

//See if bot is mindtricked by the client in question
static int BotMindTricked(int botClient, int enemyClient)
{
	forcedata_t* fd;

	if (!g_entities[enemyClient].client)
	{
		return 0;
	}

	fd = &g_entities[enemyClient].client->ps.fd;

	if (!fd)
	{
		return 0;
	}

	if (botClient > 47)
	{
		if (fd->forceMindtrickTargetIndex4 & (1 << (botClient - 48)))
		{
			return 1;
		}
	}
	else if (botClient > 31)
	{
		if (fd->forceMindtrickTargetIndex3 & (1 << (botClient - 32)))
		{
			return 1;
		}
	}
	else if (botClient > 15)
	{
		if (fd->forceMindtrickTargetIndex2 & (1 << (botClient - 16)))
		{
			return 1;
		}
	}
	else
	{
		if (fd->forceMindtrickTargetIndex & (1 << botClient))
		{
			return 1;
		}
	}

	return 0;
}

//perform cheap "hearing" checks based on the event catching
//system
static int BotCanHear(bot_state_t* bs, gentity_t* en, float endist)
{
	float minlen;

	if (!en || !en->client)
	{
		return 0;
	}

	if (en && en->client && en->client->ps.otherSoundTime > level.time)
	{ //they made a noise in recent time
		minlen = en->client->ps.otherSoundLen;
		goto checkStep;
	}

	if (en && en->client && en->client->ps.footstepTime > level.time)
	{ //they made a footstep
		minlen = 256;
		goto checkStep;
	}

	if (gBotEventTracker[en->s.number].eventTime < level.time)
	{ //no recent events to check
		return 0;
	}

	switch (gBotEventTracker[en->s.number].events[gBotEventTracker[en->s.number].eventSequence & (MAX_PS_EVENTS - 1)])
	{ //did the last event contain a sound?
	case EV_GLOBAL_SOUND:
		minlen = 256;
		break;
	case EV_FIRE_WEAPON:
	case EV_ALTFIRE:
	case EV_SABER_ATTACK:
		minlen = 512;
		break;
	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:
	case EV_FOOTSTEP:
	case EV_FOOTSTEP_METAL:
	case EV_FOOTWADE:
		minlen = 256;
		break;
	case EV_JUMP:
	case EV_ROLL:
		minlen = 256;
		break;
	default:
		minlen = 999999;
		break;
	}
checkStep:
	if (BotMindTricked(bs->client, en->s.number))
	{ //if mindtricked by this person, cut down on the minlen so they can't "hear" as well
		minlen /= 4;
	}

	if (endist <= minlen)
	{ //we heard it
		return 1;
	}

	return 0;
}

//Find a class with a heavyweapon
//index = which one of the classes with heavy weapons do you return first?  the first one?
//second one?  etc.
//saber:	qtrue = look for class with saber
//			qfalse = look for class with general heavy weapon
//returns the basic class enum
static int FindHeavyWeaponClass(int team, int index, qboolean saber)
{
	int i;
	int NumHeavyWeapClasses = 0;
	siegeTeam_t* stm;

	stm = BG_SiegeFindThemeForTeam(team);
	if (!stm)
	{
		return -1;
	}

	// Loop through all the classes for this team
	for (i = 0; i < stm->numClasses; i++)
	{
		if (!saber)
		{
			if (have_heavy_weapon(stm->classes[i]->weapons))
			{
				if (index == NumHeavyWeapClasses)
				{
					return stm->classes[i]->playerClass;
				}
				NumHeavyWeapClasses++;
			}
		}
		else
		{//look for saber
			if (stm->classes[i]->weapons & (1 << WP_SABER))
			{
				if (index == NumHeavyWeapClasses)
				{
					return stm->classes[i]->playerClass;
				}
				NumHeavyWeapClasses++;
			}
		}
	}

	//no heavy weapons/saber carrying units at this index
	return -1;
}

//Find the number of players useing this basic class on team.
static int NumberofSiegeBasicClass(int team, int BaseClass)
{
	int i = 0;
	siegeClass_t* holdClass = BG_GetClassOnBaseClass(team, BaseClass, 0);
	int NumPlayers = 0;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t* ent = &g_entities[i];
		if (ent && ent->client && ent->client->pers.connected == CON_CONNECTED
			&& ent->client->sess.siegeClass && ent->client->sess.sessionTeam == team)
		{
			if (strcmp(ent->client->sess.siegeClass, holdClass->name) == 0)
			{
				NumPlayers++;
			}
		}
	}
	return NumPlayers;
}

//Should we switch classes to destroy this breakable or just call for help?
//saber = saber only destroyable?
static void ShouldSwitchSiegeClasses(bot_state_t* bs, qboolean saber)
{
	int i = 0;
	int x;
	int classNum;

	classNum = FindHeavyWeaponClass(g_entities[bs->client].client->sess.siegeDesiredTeam, i, saber);
	while (classNum != -1)
	{
		x = NumberofSiegeBasicClass(g_entities[bs->client].client->sess.siegeDesiredTeam, classNum);
		if (x)
		{//request assistance for this class since we already have someone
			//playing that class
			request_siege_assistance(bs, SPC_DEMOLITIONIST);
			return;
		}

		//ok, noone is using that class check for the next
		//indexed heavy weapon class
		i++;
		classNum = FindHeavyWeaponClass(g_entities[bs->client].client->sess.siegeDesiredTeam, i, saber);
	}

	//ok, noone else is using a siege class with a heavyweapon.  Switch to
	//one ourselves
	i--;
	classNum = FindHeavyWeaponClass(g_entities[bs->client].client->sess.siegeDesiredTeam, i, saber);

	if (classNum == -1)
	{//what the?!
		G_Printf("couldn't find a siege class with a heavy weapon in ShouldSwitchSiegeClasses().\n");
	}
	else
	{//switch to this class
		siegeClass_t* holdClass = BG_GetClassOnBaseClass(g_entities[bs->client].client->sess.siegeDesiredTeam, classNum, 0);
		trap_EA_Command(bs->client, va("siegeclass \"%s\"\n", holdClass->name));
	}
}

//find angles/viewangles for entity
static void FindAngles(gentity_t* ent, vec3_t angles)
{
	if (ent->client)
	{//player
		VectorCopy(ent->client->ps.viewangles, angles);
	}
	else
	{//other stuff
		VectorCopy(ent->s.angles, angles);
	}
}

//Basically this is the tactical order for attacking an object with a known location
static void objectiveType_Attack(bot_state_t* bs, gentity_t* target)
{
	vec3_t objOrigin;
	vec3_t a;
	trace_t tr;
	float dist;

	FindOrigin(target, objOrigin);

	//Do visual check to target
	VectorSubtract(objOrigin, bs->eye, a);
	dist = TargetDistance(bs, target, objOrigin);
	vectoangles(a, a);

	trap->Trace(&tr, bs->eye, NULL, NULL, objOrigin, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (((tr.entityNum == target->s.number || tr.fraction == 1)
		&& (in_field_of_vision(bs->viewangles, 90, a) || bs->cur_ps.groundEntityNum == target->s.number)
		&& !BotMindTricked(bs->client, target->s.number))
		|| BotCanHear(bs, target, dist) || dist < 100)
	{//we see the objective, go for it.
		int desiredweap = favorite_weapon(bs, target, qtrue, qtrue, 0);
		if ((target->flags & FL_DMG_BY_HEAVY_WEAP_ONLY) && !is_heavy_weapon(bs, desiredweap))
		{//we currently don't have a heavy weap that we can use to destroy this target
			if (have_heavy_weapon(bs->cur_ps.stats[STAT_WEAPONS]))
			{//we have a weapon that could destroy this target but we don't have ammo
				BotDefend(bs, target);
			}
			else if (level.gametype == GT_SIEGE)
			{//ok, check to see if we should switch classes if noone else can blast this
				ShouldSwitchSiegeClasses(bs, qfalse);
				BotDefend(bs, target);
			}
			else
			{//go hunting for a weapon that can destroy this object
				//RAFIXME:  Add this code
				BotDefend(bs, target);
			}
		}
		else if ((target->flags & FL_DMG_BY_SABER_ONLY) && !(bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_SABER)))
		{//This is only damaged by sabers and we don't have a saber
			ShouldSwitchSiegeClasses(bs, qtrue);
			BotDefend(bs, target);
		}
		else
		{//cleared to attack
			bs->frame_Enemy_Len = dist;
			bs->frame_Enemy_Vis = 1;
			bs->currentEnemy = target;
			VectorCopy(objOrigin, bs->lastEnemySpotted);
			FindAngles(target, bs->lastEnemyAngles);
			bs->enemySeenTime = level.time + BOT_VISUALLOSETRACKTIME;
			BotBehave_Attack(bs);
		}
		return;
	}
	else if (bs->currentEnemy == target)
	{//can't see the target so null it out so we can find other enemies.
		bs->currentEnemy = NULL;
		bs->frame_Enemy_Vis = 0;
		bs->frame_Enemy_Len = 0;
	}

	if (strcmp(target->classname, "func_breakable") == 0
		&& target->paintarget
		&& strcmp(target->paintarget, "shieldgen_underattack") == 0)
	{//dirty hack to get the bots to attack the shield generator on siege_hoth
		vec3_t temp;
		VectorSet(temp, -369, 858, -231);
		if (Distance(bs->origin, temp) < DEFEND_MAXDISTANCE)
		{//automatically see target.
			if (!(bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_DEMP2))
				&& !(bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_ROCKET_LAUNCHER))
				&& !(bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_CONCUSSION))
				&& !(bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_REPEATER)))
			{//we currently don't have a heavy weap that can reach this target
				BotDefend(bs, target);
			}
			else
			{//cleared to attack
				bs->frame_Enemy_Len = dist;
				bs->frame_Enemy_Vis = 1;
				bs->currentEnemy = target;
				VectorCopy(objOrigin, bs->lastEnemySpotted);
				FindAngles(target, bs->lastEnemyAngles);
				bs->enemySeenTime = level.time + BOT_VISUALLOSETRACKTIME;
				BotBehave_Attack(bs);
			}
			return;
		}
		VectorCopy(temp, objOrigin);
	}

	//ok, we can't see the objective, move towards its location
	VectorCopy(objOrigin, bs->DestPosition);
	bs->DestIgnore = target->s.number;
	if (bs->currentEnemy)
	{//have a local enemy, attackmove
		BotBehave_AttackMove(bs);
	}
	else
	{//normal move
		bs->botBehave = BBEHAVE_MOVETO;
	}
}

static int FindValidObjective(int objective)
{
	int x = 0;

	//since the game only ever does 6 objectives
	if (objective == -1)
	{
		objective = Q_irand(1, 6);
	}

	//we assume that the round over check is done before this point
	while (!ObjectiveStillActive(objective))
	{
		objective--;
		if (objective < 1)
		{
			objective = 6;
		}
	}

	//depandancy checking
	for (x = 0; x < MAX_OBJECTIVEDEPENDANCY; x++)
	{
		if (ObjectiveDependancy[objective - 1][x])
		{//dependancy
			if (ObjectiveStillActive(ObjectiveDependancy[objective - 1][x]))
			{//a prereq objective hasn't been completed, do that first
				return FindValidObjective(ObjectiveDependancy[objective - 1][x]);
			}
		}
	}

	return objective;
}

//vehicle
static void objectiveType_Vehicle(bot_state_t* bs)
{
	gentity_t* vehicle = NULL;
	gentity_t* botEnt = &g_entities[bs->client];

	//find the vehicle that must trigger this trigger.
	while ((vehicle = G_Find(vehicle, FOFS(script_targetname), bs->tacticEntity->NPC_targetname)) != NULL)
	{
		if (vehicle->inuse && vehicle->client && vehicle->s.eType == ET_NPC &&
			vehicle->s.NPC_class == CLASS_VEHICLE && vehicle->m_pVehicle)
		{
			break;
		}
	}

	if (!vehicle)
	{//can't find the vehicle?!
		return;
	}

	if (botEnt->inuse && botEnt->client
		&& botEnt->client->ps.m_iVehicleNum == vehicle->s.number)
	{//in the vehicle
		//move towards trigger point
		vec3_t objOrigin;
		FindOrigin(bs->tacticEntity, objOrigin);

		bs->noUseTime = +level.time + 5000;

		bs->DestIgnore = bs->tacticEntity->s.number;
		bot_move(bs, objOrigin, qfalse, qfalse);
	}
	else if (vehicle->client->ps.m_iVehicleNum)
	{//vehicle already occuped, cover it.
		BotDefend(bs, vehicle);
	}
	else
	{//go to the vehicle!
		//hack!
		vec3_t vehOrigin;
		FindOrigin(vehicle, vehOrigin);

		//bs->useTime = level.time + 100;

		bs->botBehave = BBEHAVE_MOVETO;
		VectorCopy(vehOrigin, bs->DestPosition);
		bs->DestIgnore = vehicle->s.number;
	}
}

extern qboolean gSiegeRoundEnded;
qboolean BotObjective(bot_state_t* bs)
{
	//make sure the objective is still valid
	if (level.gametype != GT_SIEGE)
	{
	}
	else if (gSiegeRoundEnded)
	{//round over, don't do anything
		return qfalse;
	}
	else if (bs->tacticObjective <= 0 ||
		!ObjectiveStillActive(bs->tacticObjective))
	{//objective lost/completed. switch to first active objective
		//G_Printf("Bot switched objectives due to objective being lost/won.\n");
		bs->tacticObjective = FindValidObjective(-1);
		bs->objectiveType = 0;
		if (bs->tacticObjective == -1)
		{//end of map
			return qfalse;
		}
	}

	if (!bs->objectiveType || !bs->tacticEntity
		|| strcmp(bs->tacticEntity->classname, "freed") == 0)
	{//don't have objective entity type, don't have tacticEntity, or the tacticEntity you had
		//was killed/freed
		bs->tacticEntity = DetermineObjectiveType(g_entities[bs->client].client->sess.sessionTeam,
			bs->tacticObjective, &bs->objectiveType, NULL, 0);
	}

	if (bs->objectiveType == OT_ATTACK)
	{
		//attack tactical code
		objectiveType_Attack(bs, bs->tacticEntity);
	}
	else if (bs->objectiveType == OT_DEFEND)
	{//defend tactical code
		BotDefend(bs, bs->tacticEntity);
	}
	else if (bs->objectiveType == OT_CAPTURE)
	{//capture tactical code
		objectiveType_Capture(bs);
	}
	else if (bs->objectiveType == OT_DEFENDCAPTURE)
	{//defend capture tactical
		objectiveType_DefendCapture(bs);
	}
	else if (bs->objectiveType == OT_TOUCH)
	{//touch tactical
		objectiveType_Touch(bs);
	}
	else if (bs->objectiveType == OT_VEHICLE)
	{//vehicle techical
		objectiveType_Vehicle(bs);
	}
	else if (bs->objectiveType == OT_WAIT)
	{//just run around and attack people, since we're waiting for the objective to become valid.
		BotSearchAndDestroy(bs);
	}
	else
	{
		G_Printf("Bad/Unknown ObjectiveType in BotObjective.\n");
	}
	return qtrue;
}




//----- End Enhaned logic code -----//


int gUpdateVars = 0;

/*
==================
BotAIStartFrame
==================
*/
int bot_ai_startframe(const int time)
{
	int i;
	int thinktime;
	static int local_time;
	static int lastbotthink_time;

	if (gUpdateVars < level.time)
	{
		trap->Cvar_Update(&bot_pvstype);
		trap->Cvar_Update(&bot_camp);
		trap->Cvar_Update(&bot_attachments);
		trap->Cvar_Update(&bot_forgimmick);
		trap->Cvar_Update(&bot_honorableduelacceptance);
#ifndef FINAL_BUILD
		trap->Cvar_Update(&bot_getinthecarrr);
#endif
		trap->Cvar_Update(&bot_fps);

		gUpdateVars = level.time + 1000;
	}

	G_CheckBotSpawn();

	//rww - addl bot frame functions
	if (gBotEdit)
	{
		trap->Cvar_Update(&bot_wp_info);
		BotWaypointRender();
	}

	update_event_tracker();
	//end rww

	//cap the bot think time
	//if the bot think time changed we should reschedule the bots
	if (BOT_THINK_TIME != lastbotthink_time)
	{
		lastbotthink_time = BOT_THINK_TIME;
		bot_schedule_bot_think();
	}

	const int elapsed_time = time - local_time;
	local_time = time;

	if (elapsed_time > BOT_THINK_TIME) thinktime = elapsed_time;
	else thinktime = BOT_THINK_TIME;

	// execute scheduled bot AI
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!botstates[i] || !botstates[i]->inuse)
		{
			continue;
		}
		botstates[i]->botthink_residual += elapsed_time;

		if (botstates[i]->botthink_residual >= thinktime)
		{
			botstates[i]->botthink_residual -= thinktime;

			if (g_entities[i].client->pers.connected == CON_CONNECTED)
			{
				bot_ai(i, (float)thinktime / 1000);
			}
		}
	}

	// execute bot user commands every frame
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!botstates[i] || !botstates[i]->inuse)
		{
			continue;
		}
		if (g_entities[i].client->pers.connected != CON_CONNECTED)
		{
			continue;
		}

		bot_update_input(botstates[i], time, elapsed_time);
		trap->BotUserCommand(botstates[i]->client, &botstates[i]->lastucmd);
	}

	return qtrue;
}

/*
==============
BotAISetup
==============
*/
int bot_ai_setup(const int restart)
{
	//rww - new bot cvars..
	trap->Cvar_Register(&bot_forcepowers, "bot_forcepowers", "1", CVAR_CHEAT);
	trap->Cvar_Register(&bot_forgimmick, "bot_forgimmick", "0", CVAR_CHEAT);
	trap->Cvar_Register(&bot_honorableduelacceptance, "bot_honorableduelacceptance", "1", CVAR_CHEAT);
	trap->Cvar_Register(&bot_pvstype, "bot_pvstype", "1", CVAR_CHEAT);
#ifndef FINAL_BUILD
	trap->Cvar_Register(&bot_getinthecarrr, "bot_getinthecarrr", "0", 0);
#endif

#ifdef _DEBUG
	trap->Cvar_Register(&bot_nogoals, "bot_nogoals", "0", CVAR_CHEAT);
	trap->Cvar_Register(&bot_debugmessages, "bot_debugmessages", "0", CVAR_CHEAT);
#endif

	trap->Cvar_Register(&bot_attachments, "bot_attachments", "1", 0);
	trap->Cvar_Register(&bot_camp, "bot_camp", "1", 0);

	trap->Cvar_Register(&bot_wp_info, "bot_wp_info", "1", 0);
	trap->Cvar_Register(&bot_wp_edit, "bot_wp_edit", "0", CVAR_CHEAT);
	trap->Cvar_Register(&bot_wp_clearweight, "bot_wp_clearweight", "1", 0);
	trap->Cvar_Register(&bot_wp_distconnect, "bot_wp_distconnect", "1", 0);
	trap->Cvar_Register(&bot_wp_visconnect, "bot_wp_visconnect", "1", 0);
	trap->Cvar_Register(&bot_fps, "bot_fps", "20", CVAR_ARCHIVE);

	trap->Cvar_Update(&bot_forcepowers);
	//end rww

	//if the game is restarted for a tournament
	if (restart)
	{
		return qtrue;
	}

	//initialize the bot states
	memset(botstates, 0, sizeof botstates);

	if (!trap->BotLibSetup())
	{
		return qfalse; //wts?!
	}

	return qtrue;
}

/*
==============
BotAIShutdown
==============
*/
int bot_ai_shutdown(const int restart)
{
	//if the game is restarted for a tournament
	if (restart)
	{
		//shutdown all the bots in the botlib
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (botstates[i] && botstates[i]->inuse)
			{
				BotAIShutdownClient(botstates[i]->client);
			}
		}
		//don't shutdown the bot library
	}
	else
	{
		trap->BotLibShutdown();
	}
	return qtrue;
}