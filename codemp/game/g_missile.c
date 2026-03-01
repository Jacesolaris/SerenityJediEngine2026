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

/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///
///																																///
///																																///
///													SERENITY JEDI ENGINE														///
///										          LIGHTSABER COMBAT SYSTEM													    ///
///																																///
///						      System designed by Serenity and modded by JaceSolaris. (c) 2023 SJE   		                    ///
///								    https://www.moddb.com/mods/movie-duels											///
///																																///
/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///

#include "g_local.h"
#include "w_saber.h"
#include "qcommon/q_shared.h"
#include "bg_vehicles.h"
#include "b_public.h"
#include <qcommon\q_math.h>
#include "bg_weapons.h"
#include "bg_public.h"
#include "g_public.h"
#include "anims.h"
#include "surfaceflags.h"
#include <string.h>
#include <qcommon\q_string.h>
#include "teams.h"
#include <qcommon\q_platform.h>
#include <qcommon\q_color.h>

#define	MISSILE_PRESTEP_TIME	50

#define TASER_DAMAGE			20

extern void laserTrapStick(gentity_t* ent, vec3_t endpos, vec3_t normal);
extern void Jedi_Decloak(gentity_t* self);
extern qboolean FighterIsLanded(const Vehicle_t* p_veh, const playerState_t* parent_ps);
extern void PM_AddBlockFatigue(playerState_t* ps, int fatigue);
extern float VectorDistance(vec3_t v1, vec3_t v2);
qboolean PM_SaberInStart(int move);
extern qboolean PM_SaberInReturn(int move);
extern qboolean wp_saber_block_non_random_missile(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
extern int wp_saber_must_bolt_block(gentity_t* self, const gentity_t* atk, qboolean check_b_box_block, vec3_t point,
	int rSaberNum,
	int rBladeNum);
void wp_flechette_alt_blow(gentity_t* ent);
void wp_stasis_missile_blow(gentity_t* ent);
extern qboolean G_DoDodge(gentity_t* self, gentity_t* shooter, vec3_t dmg_origin, int hit_loc, int* dmg, int mod);
extern qboolean WP_DoingForcedAnimationForForcePowers(const gentity_t* self);
extern qboolean PM_RunningAnim(int anim);
vec3_t g_crosshairWorldCoord = { 0, 0, 0 };
extern qboolean PM_SaberInParry(int move);
extern qboolean PM_SaberInReflect(int move);
extern qboolean PM_SaberInIdle(int move);
extern qboolean PM_SaberInAttack(int move);
extern qboolean PM_SaberInTransitionAny(int move);
extern qboolean pm_saber_in_special_attack(int anim);
extern void CalcEntitySpot(const gentity_t* ent, spot_t spot, vec3_t point);
extern qboolean pm_walking_or_standing(const gentity_t* self);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
extern float manual_npc_saberblocking(const gentity_t* defender);
extern qboolean WP_BrokenBoltBlockKnockBack(gentity_t* victim);
extern qboolean WP_SaberBlockBolt(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
void wp_handle_bolt_block(gentity_t* bolt, gentity_t* blocker, trace_t* trace, vec3_t fwd);
extern int WP_SaberBoltBlockCost(gentity_t* defender, const gentity_t* attacker);
extern void WP_BlockPointsDrain(const gentity_t* self, int fatigue);
extern void G_KnockOver(gentity_t* self, const gentity_t* attacker, const vec3_t push_dir, float strength,
	qboolean breakSaberLock);
extern float manual_running_and_saberblocking(const gentity_t* defender);
extern qboolean WP_SaberFatiguedParryDirection(gentity_t* self, vec3_t hitloc, qboolean missileBlock);

static float vector_bolt_distance(vec3_t v1, vec3_t v2)
{
	//returns the distance between the two points.
	vec3_t dir;

	VectorSubtract(v2, v1, dir);
	return VectorLength(dir);
}

/*
 * Safety helper: validate entity index before indexing g_entities.
 * Prevents crashes if an entity's ownerNum is invalid/out of range.
 */
static gentity_t* G_GetEntitySafe(int idx)
{
	if (idx >= 0 && idx < MAX_GENTITIES)
	{
		return &g_entities[idx];
	}
	return NULL;
}

/*
================
G_ReflectMissile

  Reflect the missile roughly back at it's owner
================
*/

//////////////////// Boltblock new ////////////////////////////////

static void g_manual_block_missile(const gentity_t* ent, gentity_t* missile, vec3_t forward)
{
	// Safety: validate pointers (bug fix)
	if (!ent || !missile)
	{
		return;
	}

	vec3_t bounce_dir;

	// Save original speed
	const float speed = VectorNormalize(missile->s.pos.trDelta);

	if (ent->client)
	{
		vec3_t missile_dir;

		// Use blocker view direction
		AngleVectors(ent->client->ps.viewangles, missile_dir, NULL, NULL);

		// Project missile_dir onto forward
		VectorCopy(missile_dir, bounce_dir);
		VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
		VectorNormalize(bounce_dir);
	}
	else
	{
		// Fallback: use forward direction
		VectorCopy(forward, bounce_dir);
		VectorNormalize(bounce_dir);
	}

	// Add random slop
	for (int i = 0; i < 3; i++)
	{
		bounce_dir[i] += Q_flrand(-1.0f, 1.0f);
	}

	VectorNormalize(bounce_dir);

	// Reapply original speed
	VectorScale(bounce_dir, speed, missile->s.pos.trDelta);

	// Update trajectory
	missile->s.pos.trTime = level.time;
	VectorCopy(missile->r.currentOrigin, missile->s.pos.trBase);

	// Transfer ownership (except sabers and G2 parts)
	if (missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART)
	{
		missile->r.ownerNum = ent->s.number;
	}

	// Stop homing on rockets
	if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{
		missile->think = NULL;
		missile->nextthink = 0;
	}
}

static void g_missile_bouncedoff_saber(const gentity_t* ent, gentity_t* missile, vec3_t forward)
{
	// Safety: validate pointers (bug fix)
	if (!ent || !missile)
	{
		return;
	}

	vec3_t bounce_dir;

	// Save the original speed
	const float speed = VectorNormalize(missile->s.pos.trDelta);

	if (ent->client)
	{
		vec3_t missile_dir;

		// Use the blocker’s view direction
		AngleVectors(ent->client->ps.viewangles, missile_dir, NULL, NULL);

		// Project missile_dir onto forward
		VectorCopy(missile_dir, bounce_dir);
		VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
		VectorNormalize(bounce_dir);
	}
	else
	{
		// Fallback: use forward direction
		VectorCopy(forward, bounce_dir);
		VectorNormalize(bounce_dir);
	}

	// Add heavy random slop (±6)
	for (int i = 0; i < 3; i++)
	{
		bounce_dir[i] += Q_flrand(-6.0f, 6.0f);
	}

	VectorNormalize(bounce_dir);

	// Reapply original speed
	VectorScale(bounce_dir, speed, missile->s.pos.trDelta);

	// Update trajectory
	missile->s.pos.trTime = level.time;
	VectorCopy(missile->r.currentOrigin, missile->s.pos.trBase);

	// Transfer ownership (except sabers and G2 parts)
	if (missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART)
	{
		missile->r.ownerNum = ent->s.number;
	}

	// Stop homing on rockets
	if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{
		missile->think = NULL;
		missile->nextthink = 0;
	}
}



static void g_deflect_missile_to_attacker(const gentity_t* ent, gentity_t* missile, vec3_t forward)
{
	// Safety: validate pointers (prevents crashes if caller passes NULL)
	if (!ent || !missile)
	{
		return;
	}

	vec3_t bounce_dir;

	// Save original speed (VectorNormalize returns length and normalizes the vector)
	const float speed = VectorNormalize(missile->s.pos.trDelta);

	if (ent->client)
	{
		vec3_t missile_dir;
		// Use the blocker's view direction for aiming the deflection
		AngleVectors(ent->client->ps.viewangles, missile_dir, NULL, NULL);

		VectorCopy(missile_dir, bounce_dir);
		const float proj = DotProduct(forward, missile_dir);
		VectorScale(bounce_dir, proj, bounce_dir);
		VectorNormalize(bounce_dir);
	}
	else
	{
		// Fallback: use provided forward vector
		VectorCopy(forward, bounce_dir);
		VectorNormalize(bounce_dir);
	}

	// Add some random slop to the deflection
	const float slop = 1.0f;
	for (int i = 0; i < 3; i++)
	{
		bounce_dir[i] += Q_flrand(-slop, slop);
	}

	VectorNormalize(bounce_dir);

	// Reapply original speed and update trajectory
	VectorScale(bounce_dir, speed, missile->s.pos.trDelta);
	missile->s.pos.trTime = level.time; // move a bit on the very first frame
	VectorCopy(missile->r.currentOrigin, missile->s.pos.trBase);

	// Transfer ownership (except sabers and G2 parts)
	if (missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART)
	{
		missile->r.ownerNum = ent->s.number;
	}

	// Stop homing on rockets
	if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{
		missile->think = NULL;
		missile->nextthink = 0;
	}
}

static void g_reflect_missile_to_attacker(const gentity_t* ent, gentity_t* missile, vec3_t forward)
{
	vec3_t bounce_dir;

	// Is the blocker the original owner?
	const qboolean is_owner = (missile->r.ownerNum == ent->s.number);

	// Preserve original speed
	float speed = VectorNormalize(missile->s.pos.trDelta);

	//
	// Determine base bounce direction
	//
	if (!is_owner &&
		missile->s.weapon != WP_SABER &&
		missile->s.weapon != G2_MODEL_PART)
	{
		// Bounce back toward the original shooter (safe lookup)
		gentity_t* origOwner = G_GetEntitySafe(missile->r.ownerNum);
		if (origOwner)
		{
			VectorSubtract(origOwner->r.currentOrigin,
				missile->r.currentOrigin,
				bounce_dir);
			VectorNormalize(bounce_dir);
		}
		else
		{
			// Fallback: push forward
			VectorCopy(forward, bounce_dir);
			VectorNormalize(bounce_dir);
		}
	}
	else if (is_owner)
	{
		// Push the missile away from the owner, with extra speed
		vec3_t missile_dir;

		speed *= 1.5f;

		VectorSubtract(missile->r.currentOrigin,
			ent->r.currentOrigin,
			missile_dir);

		// Project current velocity onto missile_dir
		VectorCopy(missile->s.pos.trDelta, bounce_dir);
		VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
		VectorNormalize(bounce_dir);
	}
	else
	{
		// Default: push away from the defender
		vec3_t missile_dir;

		VectorSubtract(ent->r.currentOrigin,
			missile->r.currentOrigin,
			missile_dir);

		VectorCopy(missile->s.pos.trDelta, bounce_dir);
		VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
		VectorNormalize(bounce_dir);
	}

	//
	// Add slop based on saber state
	//
	if (!PM_SaberInIdle(ent->client->ps.saber_move))
	{
		float min_slop, max_slop;

		if (PM_SaberInAttack(ent->client->ps.saber_move) ||
			PM_SaberInTransitionAny(ent->client->ps.saber_move) ||
			pm_saber_in_special_attack(ent->client->ps.torsoAnim) ||
			ent->client->ps.fd.blockPoints < BLOCKPOINTS_KNOCKAWAY)
		{
			// Moderately wild
			min_slop = -0.3f;
			max_slop = 0.3f;
		}
		else
		{
			// Mildly wild
			min_slop = -0.1f;
			max_slop = 0.1f;
		}

		for (int i = 0; i < 3; i++)
		{
			bounce_dir[i] += Q_flrand(min_slop, max_slop);
		}
	}

	//
	// Apply final velocity
	//
	VectorNormalize(bounce_dir);
	VectorScale(bounce_dir, speed, missile->s.pos.trDelta);

	missile->s.pos.trTime = level.time;
	VectorCopy(missile->r.currentOrigin, missile->s.pos.trBase);

	//
	// Transfer ownership unless it's a saber or model part
	//
	if (missile->s.weapon != WP_SABER &&
		missile->s.weapon != G2_MODEL_PART)
	{
		missile->r.ownerNum = ent->s.number;
	}

	//
	// Stop homing rockets
	//
	if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{
		missile->think = NULL;
		missile->nextthink = 0;
	}
}

void g_reflect_missile_auto(const gentity_t* ent, gentity_t* missile, vec3_t forward)
{
	vec3_t bounce_dir;

	// Is the blocker the original owner?
	const qboolean is_owner = (missile->r.ownerNum == ent->s.number);

	// Preserve original speed
	float speed = VectorNormalize(missile->s.pos.trDelta);

	//
	// Determine base bounce direction
	//
	if (!is_owner &&
		missile->s.weapon != WP_SABER &&
		missile->s.weapon != G2_MODEL_PART)
	{
		// Bounce back toward the original shooter (safe lookup)
		gentity_t* origOwner = G_GetEntitySafe(missile->r.ownerNum);
		if (origOwner)
		{
			VectorSubtract(origOwner->r.currentOrigin,
				missile->r.currentOrigin,
				bounce_dir);
			VectorNormalize(bounce_dir);
		}
		else
		{
			// Fallback: push forward
			VectorCopy(forward, bounce_dir);
			VectorNormalize(bounce_dir);
		}
	}
	else if (is_owner)
	{
		// Push the missile away from the owner, with extra speed
		vec3_t missile_dir;

		speed *= 1.5f;

		VectorSubtract(missile->r.currentOrigin,
			ent->r.currentOrigin,
			missile_dir);

		// Project current velocity onto missile_dir
		VectorCopy(missile->s.pos.trDelta, bounce_dir);
		VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
		VectorNormalize(bounce_dir);
	}
	else
	{
		// Default: push away from the defender
		vec3_t missile_dir;

		VectorSubtract(ent->r.currentOrigin,
			missile->r.currentOrigin,
			missile_dir);

		VectorCopy(missile->s.pos.trDelta, bounce_dir);
		VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
		VectorNormalize(bounce_dir);
	}

	//
	// Add small random slop (±0.2)
	//
	for (int i = 0; i < 3; i++)
	{
		bounce_dir[i] += Q_flrand(-0.2f, 0.2f);
	}

	//
	// Apply final velocity
	//
	VectorNormalize(bounce_dir);
	VectorScale(bounce_dir, speed, missile->s.pos.trDelta);

	missile->s.pos.trTime = level.time;
	VectorCopy(missile->r.currentOrigin, missile->s.pos.trBase);

	//
	// Transfer ownership unless it's a saber or model part
	//
	if (missile->s.weapon != WP_SABER &&
		missile->s.weapon != G2_MODEL_PART)
	{
		missile->r.ownerNum = ent->s.number;
	}

	//
	// Stop homing rockets
	//
	if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{
		missile->think = NULL;
		missile->nextthink = 0;
	}
}

static qhandle_t stasisLoopSound = 0;
gentity_t* tgt_list[MAX_GENTITIES];

void G_StasisMissile(gentity_t* ent, gentity_t* missile, vec3_t forward)
{
	vec3_t bounce_dir = { 0 };
	vec3_t dir;
	static qboolean registered = qfalse;

	//
	// ⭐ 1. UNFREEZE CHECK — after 10 seconds, restore full speed
	//
	if (missile->s.userFloat1 > 0 && level.time >= missile->s.userFloat1)
	{
		// Get direction from current velocity
		VectorNormalize2(missile->s.pos.trDelta, dir);

		// Full speed (normalized)
		VectorScale(dir, 1.0f, missile->s.pos.trDelta);

		// Reset trajectory so movement resumes
		missile->s.pos.trTime = level.time;
		VectorCopy(missile->r.currentOrigin, missile->s.pos.trBase);

		missile->s.userFloat1 = 0; // clear flag
		return;
	}

	//
	// Register looping stasis sound once
	//
	if (!registered)
	{
		stasisLoopSound = G_SoundIndex("sound/effects/blaster_stasis_loop.wav");
		registered = qtrue;
	}

	//
	// ⭐ 2. STASIS START — set unfreeze timer and extend lifetime
	//
	if (missile->s.userFloat1 == 0)
	{
		missile->s.userFloat1 = level.time + 10000;      // unfreeze after 10 seconds

		missile->nextthink = level.time + 20000;         // explode after 20 seconds
		missile->think = wp_stasis_missile_blow;         // your custom blow effect
	}

	//
	// ⭐ 3. Compute direction once
	//
	VectorNormalize2(missile->s.pos.trDelta, dir);

	//
	// ⭐ 4. Slow-motion speed during stasis
	//
	float slowSpeed = 1.0f / 200.0f;   // your slow-motion factor
	float fullSpeed = 1.0f;            // normalized full speed

	//
	// ⭐ 5. Apply correct speed depending on timer
	//
	if (level.time < missile->s.userFloat1)
	{
		// Still in stasis → slow speed
		VectorScale(dir, slowSpeed, missile->s.pos.trDelta);
	}
	else
	{
		// After 10 seconds → full speed
		VectorScale(dir, fullSpeed, missile->s.pos.trDelta);
	}

	//
	// Looping stasis sound
	//
	missile->s.loopSound = stasisLoopSound;

	//
	// Update trajectory base
	//
	missile->s.pos.trTime = level.time;
	VectorCopy(missile->r.currentOrigin, missile->s.pos.trBase);

	//
	// Transfer ownership
	//
	if (missile->s.weapon != WP_SABER &&
		missile->s.weapon != G2_MODEL_PART)
	{
		missile->r.ownerNum = ent->s.number;
	}

	//
	// ⭐ 6. Rocket/Thermal proximity logic preserved
	//
	if (missile->s.weapon == WP_ROCKET_LAUNCHER ||
		missile->s.weapon == WP_THERMAL)
	{
		qboolean blow = qfalse;

		if (ent->delay > level.time)
		{
			const int count = G_RadiusList(ent->r.currentOrigin,
				200,
				ent,
				qtrue,
				tgt_list);

			for (int i = 0; i < count; i++)
			{
				gentity_t* tgt = tgt_list[i];

				if (tgt->client &&
					tgt->health > 0 &&
					ent->activator &&
					tgt->s.number != ent->activator->s.number)
				{
					blow = qtrue;
					break;
				}
			}
		}
		else
		{
			blow = qtrue;
		}

		if (blow)
		{
			missile->think = wp_flechette_alt_blow;
			missile->nextthink = 0;
		}
	}
}

void g_reflect_missile_bot(const gentity_t* ent, gentity_t* missile, vec3_t forward)
{
	vec3_t bounce_dir;
	int isowner = 0;

	if (missile->r.ownerNum == ent->s.number)
	{
		//the original owner is bouncing the missile, so don't try to bounce it back at him
		isowner = 1;
	}

	//save the original speed
	float speed = VectorNormalize(missile->s.pos.trDelta);
	/* Use safe owner lookup; the original code checked the address of g_entities which
	 * is always true and could read out-of-range owner indices. */
	gentity_t* origOwner = G_GetEntitySafe(missile->r.ownerNum);
	if (origOwner && missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART && !isowner)
	{
		//bounce back at them if you can
		VectorSubtract(origOwner->r.currentOrigin, missile->r.currentOrigin, bounce_dir);
		VectorNormalize(bounce_dir);
	}
	else if (isowner)
	{
		//in this case, actually push the missile away from me, and since we're giving boost to our own missile by pushing it, up the velocity
		vec3_t missile_dir;

		speed *= 1.5;

		VectorSubtract(missile->r.currentOrigin, ent->r.currentOrigin, missile_dir);
		VectorCopy(missile->s.pos.trDelta, bounce_dir);
		VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
		VectorNormalize(bounce_dir);
	}
	else
	{
		vec3_t missile_dir;

		VectorSubtract(ent->r.currentOrigin, missile->r.currentOrigin, missile_dir);
		VectorCopy(missile->s.pos.trDelta, bounce_dir);
		VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
		VectorNormalize(bounce_dir);
	}
	for (int i = 0; i < 3; i++)
	{
		bounce_dir[i] += Q_flrand(-0.2f, 0.2f);
	}

	VectorNormalize(bounce_dir);
	VectorScale(bounce_dir, speed, missile->s.pos.trDelta);
	missile->s.pos.trTime = level.time; // move a bit on the very first frame
	VectorCopy(missile->r.currentOrigin, missile->s.pos.trBase);
	if (missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART)
	{
		//you are mine, now!
		missile->r.ownerNum = ent->s.number;
	}
	if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{
		//stop homing
		missile->think = 0;
		missile->nextthink = 0;
	}
}

/*
================
G_BounceMissile

================
*/
static void G_BounceMissile(gentity_t* ent, trace_t* trace)
{
	vec3_t velocity;

	// reflect the velocity on the trace plane
	const int hit_time = level.previousTime + (level.time - level.previousTime) * trace->fraction;
	BG_EvaluateTrajectoryDelta(&ent->s.pos, hit_time, velocity);
	const float dot = DotProduct(velocity, trace->plane.normal);
	VectorMA(velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta);

	if (ent->flags & FL_BOUNCE_SHRAPNEL)
	{
		VectorScale(ent->s.pos.trDelta, 0.25f, ent->s.pos.trDelta);
		ent->s.pos.trType = TR_GRAVITY;

		// check for stop
		if (trace->plane.normal[2] > 0.7 && ent->s.pos.trDelta[2] < 40)
			//this can happen even on very slightly sloped walls, so changed it from > 0 to > 0.7
		{
			G_SetOrigin(ent, trace->endpos);
			ent->nextthink = level.time + 100;
			return;
		}
	}
	else if (ent->flags & FL_BOUNCE_HALF)
	{
		VectorScale(ent->s.pos.trDelta, 0.65f, ent->s.pos.trDelta);
		// check for stop
		if (trace->plane.normal[2] > 0.2 && VectorLength(ent->s.pos.trDelta) < 40)
		{
			G_SetOrigin(ent, trace->endpos);
			return;
		}
	}

	if (ent->s.weapon == WP_THERMAL)
	{
		//slight hack for hit sound
		G_Sound(ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/thermal/bounce%i.wav", Q_irand(1, 2))));
	}
	else if (ent->s.weapon == WP_SABER)
	{
		G_Sound(ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/saber/bounce%i.wav", Q_irand(1, 3))));
	}
	else if (ent->s.weapon == G2_MODEL_PART)
	{
		//Limb bounce sound?
	}

	VectorAdd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;

	if (ent->bounceCount != -5)
	{
		ent->bounceCount--;
	}
}

/*
================
g_explode_missile

Explode a missile without an impact
================
*/
void g_explode_missile(gentity_t* ent)
{
	vec3_t dir = { 0 };
	vec3_t origin;

	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);
	SnapVector(origin);
	G_SetOrigin(ent, origin);

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(dir));

	ent->freeAfterEvent = qtrue;

	ent->takedamage = qfalse;
	// splash damage
	if (ent->splashDamage)
	{
		if (ent->s.eType == ET_MISSILE //missile
			&& ent->s.eFlags & EF_JETPACK_ACTIVE //vehicle missile
			&& ent->r.ownerNum < MAX_CLIENTS) //valid client owner
		{
			//set my parent to my owner for purposes of damage credit...
			ent->parent = &g_entities[ent->r.ownerNum];
		}
		if (g_radius_damage(ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent,
			ent, ent->splashMethodOfDeath))
		{
			if (ent->parent)
			{
				g_entities[ent->parent->s.number].client->accuracy_hits++;
			}
			else if (ent->activator)
			{
				g_entities[ent->activator->s.number].client->accuracy_hits++;
			}
		}
	}

	trap->LinkEntity((sharedEntity_t*)ent);
}

static void g_run_stuck_missile(gentity_t* ent)
{
	if (ent->takedamage)
	{
		if (ent->s.groundEntityNum >= 0 && ent->s.groundEntityNum < ENTITYNUM_WORLD)
		{
			gentity_t* other = &g_entities[ent->s.groundEntityNum];

			if (!VectorCompare(vec3_origin, other->s.pos.trDelta) && other->s.pos.trType != TR_STATIONARY ||
				!VectorCompare(vec3_origin, other->s.apos.trDelta) && other->s.apos.trType != TR_STATIONARY)
			{
				//thing I stuck to is moving or rotating now, kill me
				G_Damage(ent, other, other, NULL, NULL, 99999, 0, MOD_CRUSH);
				return;
			}
		}
	}
	// check think function
	G_RunThink(ent);
}

/*
================
g_bounce_projectile
================
*/
void g_bounce_projectile(vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout)
{
	vec3_t v, newv;

	VectorSubtract(impact, start, v);
	const float dot = DotProduct(v, dir);
	VectorMA(v, -2 * dot, dir, newv);

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}

//-----------------------------------------------------------------------------
gentity_t* CreateMissile(vec3_t org, vec3_t dir, const float vel, const int life, gentity_t* owner, const qboolean alt_fire)
{
	gentity_t* missile = G_Spawn();

	missile->nextthink = level.time + life;
	missile->think = G_FreeEntity;
	missile->s.eType = ET_MISSILE;
	missile->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	missile->parent = owner;
	missile->r.ownerNum = owner->s.number;
	//lmo tag owner info into state for duel Nox
	missile->s.otherentity_num = owner->s.number;

	if (alt_fire)
	{
		missile->s.eFlags |= EF_ALT_FIRING;
	}

	missile->s.pos.trType = TR_LINEAR;
	missile->s.pos.trTime = level.time; // - MISSILE_PRESTEP_TIME;	// NOTE This is a Quake 3 addition over JK2
	missile->targetEnt = NULL;

	SnapVector(org);
	VectorCopy(org, missile->s.pos.trBase);
	VectorScale(dir, vel, missile->s.pos.trDelta);
	VectorCopy(org, missile->r.currentOrigin);
	SnapVector(missile->s.pos.trDelta);

	return missile;
}

static void g_missile_bounce_effect(gentity_t* ent, vec3_t org, vec3_t dir, const qboolean hit_world)
{
	switch (ent->s.weapon)
	{
	case WP_BOWCASTER:
		if (hit_world)
		{
			G_PlayEffectID(G_EffectIndex("bowcaster/bounce_wall"), org, dir);
		}
		else
		{
			G_PlayEffectID(G_EffectIndex("bowcaster/deflect"), ent->r.currentOrigin, dir);
		}
		break;
	case WP_BLASTER:
	case WP_BRYAR_PISTOL:
	case WP_BRYAR_OLD:
		G_PlayEffectID(G_EffectIndex("blaster/deflect"), ent->r.currentOrigin, dir);
		break;
	case WP_REPEATER:
		G_PlayEffectID(G_EffectIndex("repeater/deflectblock"), ent->r.currentOrigin, dir);
		break;
	default:
	{
		gentity_t* te = G_TempEntity(org, EV_GRENADE_BOUNCE);
		VectorCopy(org, te->s.origin);
		VectorCopy(dir, te->s.angles);
		te->s.eventParm = 0;
		te->s.weapon = 0; //saberNum
		te->s.legsAnim = 0; //blade_num
	}
	break;
	}
}

void g_missile_reflect_effect(gentity_t* ent, vec3_t dir)
{
	switch (ent->s.weapon)
	{
	case WP_BOWCASTER:
		G_PlayEffectID(G_EffectIndex("bowcaster/deflect"), ent->r.currentOrigin, dir);
		break;
	case WP_BLASTER:
	case WP_BRYAR_PISTOL:
	case WP_BRYAR_OLD:
		G_PlayEffectID(G_EffectIndex("blaster/deflect"), ent->r.currentOrigin, dir);
		break;
	case WP_REPEATER:
		G_PlayEffectID(G_EffectIndex("repeater/deflectblock"), ent->r.currentOrigin, dir);
		break;
	default:
		G_PlayEffectID(G_EffectIndex("blaster/deflect"), ent->r.currentOrigin, dir);
		break;
	}

	if (!(ent->r.svFlags & SVF_BOT))
	{
		CGCam_BlockShakeMP(ent->s.origin, ent, 0.45f, 100);
	}
}

static void G_MissileBounceBeskarEffect(gentity_t* ent, vec3_t org, vec3_t dir, const qboolean hit_world)
{
	G_PlayEffectID(G_EffectIndex("blaster/beskar_impact"), ent->r.currentOrigin, dir);
}

static void G_MissileAddAlerts(gentity_t* ent)
{
	//Add the event
	if (ent->s.weapon == WP_THERMAL && (ent->delay - level.time < 2000 || ent->s.pos.trType == TR_INTERPOLATE))
	{
		//a thermal about to explode or rolling
		if (ent->delay - level.time < 500)
		{
			//half a second before it explodes!
			AddSoundEvent(ent->owner, ent->r.currentOrigin, ent->splashRadius * 2, AEL_DANGER_GREAT, qfalse, qtrue);
			AddSightEvent(ent->owner, ent->r.currentOrigin, ent->splashRadius * 2, AEL_DANGER_GREAT, 20);
		}
		else
		{
			//2 seconds until it explodes or it's rolling
			AddSoundEvent(ent->owner, ent->r.currentOrigin, ent->splashRadius * 2, AEL_DANGER, qfalse, qtrue);
			AddSightEvent(ent->owner, ent->r.currentOrigin, ent->splashRadius * 2, AEL_DANGER, 20);
		}
	}
	else
	{
		AddSoundEvent(ent->owner, ent->r.currentOrigin, 128, AEL_DISCOVERED, qfalse, qtrue);
		AddSightEvent(ent->owner, ent->r.currentOrigin, 256, AEL_DISCOVERED, 40);
	}
}

/*
================
G_MissileImpact
================
*/
qboolean G_MissileImpact(gentity_t* ent, trace_t* trace)
{
	vec3_t   fwd;
	qboolean hit_client = qfalse;
	qboolean is_knocked_saber = qfalse;
	int      missile_dmg;

	gentity_t* other = &g_entities[trace->entityNum];

	// Initial bounce flag (note: mostly overridden by specific logic below)
	qboolean bounce =
		((!other->takedamage && (ent->flags & (FL_BOUNCE | FL_BOUNCE_HALF))) ||
			((trace->surfaceFlags & SURF_FORCEFIELD) || (other->flags & FL_SHIELDED)));

	//
	// Generic bounce for grenades / bouncing missiles
	//
	if ((!other->takedamage || ent->s.weapon == WP_THERMAL) &&
		(ent->bounceCount > 0 || ent->bounceCount == -5) &&
		(ent->flags & (FL_BOUNCE | FL_BOUNCE_HALF)))
	{
		G_BounceMissile(ent, trace);

		if (ent->s.weapon == WP_SABER)
		{
			// Intentionally no event here (commented out in original)
			// G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);
		}
		else
		{
			G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);
		}

		return qtrue;
	}

	//
	// Knocked‑away saber handling
	//
	if (ent->neverFree && ent->s.weapon == WP_SABER && (ent->flags & FL_BOUNCE_HALF))
	{
		// This is a knocked‑away saber
		if ((ent->bounceCount > 0 || ent->bounceCount == -5))
		{
			G_BounceMissile(ent, trace);

			if (ent->bounceCount > 0 && ent->bounceCount < 3)
			{
				G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);
			}

			return qtrue;
		}

		is_knocked_saber = qtrue;
	}

	//
	// Forcefield bounce (non‑splash, non‑radius, limited bounces)
	//
	if ((trace->surfaceFlags & SURF_FORCEFIELD) &&
		!ent->splashDamage &&
		!ent->splashRadius &&
		(ent->bounceCount > 0 || ent->bounceCount == -5))
	{
		G_BounceMissile(ent, trace);

		if (ent->bounceCount < 1)
		{
			ent->flags &= ~FL_BOUNCE_SHRAPNEL;
		}

		return qtrue;
	}

	//
	// Duel protection: projectiles cannot affect non‑duel opponents
	//
	if ((other->r.contents & CONTENTS_LIGHTSABER) && !is_knocked_saber)
	{
		const gentity_t* other_owner = G_GetEntitySafe(other->r.ownerNum);

		if (other_owner &&
			other_owner->takedamage &&
			other_owner->client &&
			other_owner->client->ps.duelInProgress &&
			other_owner->client->ps.duelIndex != ent->r.ownerNum)
		{
			goto killProj;
		}
	}
	else if (!is_knocked_saber)
	{
		if (other->takedamage &&
			other->client &&
			other->client->ps.duelInProgress &&
			other->client->ps.duelIndex != ent->r.ownerNum)
		{
			goto killProj;
		}
	}

	//
	// Beskar / Boba special bounce flags
	//
	qboolean beskar =
		((other->flags & FL_DINDJARIN) &&
			!ent->splashDamage &&
			!ent->splashRadius &&
			ent->methodOfDeath != MOD_SABER &&
			ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_ROCKET &&
			ent->methodOfDeath != MOD_ROCKET_SPLASH &&
			ent->methodOfDeath != MOD_CONC_ALT &&
			ent->methodOfDeath != MOD_THERMAL &&
			ent->methodOfDeath != MOD_THERMAL_SPLASH &&
			ent->methodOfDeath != MOD_DEMP2 &&
			ent->methodOfDeath != MOD_DEMP2_ALT &&
			ent->methodOfDeath != MOD_SEEKER &&
			ent->methodOfDeath != MOD_CONC &&
			!Q_irand(0, 1));

	qboolean boba_fett =
		((other->flags & FL_BOBAFETT) &&
			!ent->splashDamage &&
			!ent->splashRadius &&
			ent->methodOfDeath != MOD_SABER &&
			ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_ROCKET &&
			ent->methodOfDeath != MOD_ROCKET_SPLASH &&
			ent->methodOfDeath != MOD_CONC_ALT &&
			ent->methodOfDeath != MOD_THERMAL &&
			ent->methodOfDeath != MOD_THERMAL_SPLASH &&
			ent->methodOfDeath != MOD_DEMP2 &&
			ent->methodOfDeath != MOD_DEMP2_ALT &&
			ent->methodOfDeath != MOD_SEEKER &&
			ent->methodOfDeath != MOD_CONC);

	//
	// Heavy‑weapon missiles: never bounce, no beskar/boba tricks
	//
	if (ent->dflags & DAMAGE_HEAVY_WEAP_CLASS)
	{
		bounce = qfalse;
		beskar = qfalse;
		boba_fett = qfalse;
	}

	//
	// Targets that only take damage from heavy weapons
	//
	if (other->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)
	{
		if (ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_ROCKET &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_ROCKET_HOMING &&
			ent->methodOfDeath != MOD_THERMAL &&
			ent->methodOfDeath != MOD_THERMAL_SPLASH &&
			ent->methodOfDeath != MOD_TRIP_MINE_SPLASH &&
			ent->methodOfDeath != MOD_TIMED_MINE_SPLASH &&
			ent->methodOfDeath != MOD_DET_PACK_SPLASH &&
			ent->methodOfDeath != MOD_VEHICLE &&
			ent->methodOfDeath != MOD_CONC &&
			ent->methodOfDeath != MOD_CONC_ALT &&
			ent->methodOfDeath != MOD_SABER &&
			ent->methodOfDeath != MOD_TURBLAST &&
			ent->methodOfDeath != MOD_TARGET_LASER)
		{
			if (trace)
			{
				VectorCopy(trace->plane.normal, fwd);
			}
			else
			{
				AngleVectors(other->r.currentAngles, fwd, NULL, NULL);
			}

			g_manual_block_missile(other, ent, fwd);
			g_missile_bounce_effect(ent, ent->r.currentOrigin, fwd, (trace->entityNum == ENTITYNUM_WORLD));
			return qtrue;
		}
	}

	//
	// Shielded targets (non‑heavy, non‑explosive types)
	//
	if ((other->flags & FL_SHIELDED) &&
		ent->s.weapon != WP_ROCKET_LAUNCHER &&
		ent->s.weapon != WP_THERMAL &&
		ent->s.weapon != WP_TRIP_MINE &&
		ent->s.weapon != WP_DET_PACK &&
		ent->s.weapon != WP_EMPLACED_GUN &&
		ent->methodOfDeath != MOD_REPEATER_ALT &&
		ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
		ent->methodOfDeath != MOD_TURBLAST &&
		ent->methodOfDeath != MOD_VEHICLE &&
		ent->methodOfDeath != MOD_CONC &&
		ent->methodOfDeath != MOD_CONC_ALT &&
		!(ent->dflags & DAMAGE_HEAVY_WEAP_CLASS))
	{
		if (other->client)
		{
			AngleVectors(other->client->ps.viewangles, fwd, NULL, NULL);
		}
		else
		{
			AngleVectors(other->r.currentAngles, fwd, NULL, NULL);
		}

		g_manual_block_missile(other, ent, fwd);
		g_missile_bounce_effect(ent, ent->r.currentOrigin, fwd, (trace->entityNum == ENTITYNUM_WORLD));
		return qtrue;
	}

	//
// Beskar / Boba Fett bounce handling
//
	if (beskar || boba_fett)
	{
		bounce = qfalse;

		// Handle bounce count
		if (ent->bounceCount)
		{
			if (!--ent->bounceCount)
			{
				ent->flags &= ~(FL_BOUNCE | FL_BOUNCE_HALF);
			}
		}

		G_BounceMissile(ent, trace);

		if (other->client)
		{
			G_SetAnim(other, NULL, SETANIM_TORSO,
				Q_irand(BOTH_PAIN1, BOTH_PAIN3),
				SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD,
				0);
		}

		if (ent->owner)
		{
			G_MissileAddAlerts(ent);
		}

		G_MissileBounceBeskarEffect(ent,
			ent->r.currentOrigin,
			fwd,
			(trace->entityNum == ENTITYNUM_WORLD));

		return qfalse;
	}

	//
	// Sphere shield bounce (PW_SPHERESHIELDED)
	//
	if (other && other->client && other->client->ps.powerups[PW_SPHERESHIELDED])
	{
		bounce = qfalse;

		if (ent->bounceCount)
		{
			if (!--ent->bounceCount)
			{
				ent->flags &= ~(FL_BOUNCE | FL_BOUNCE_HALF);
			}
		}

		G_BounceMissile(ent, trace);

		G_MissileBounceBeskarEffect(ent,
			ent->r.currentOrigin,
			fwd,
			(trace->entityNum == ENTITYNUM_WORLD));

		return qfalse;
	}

	//
	// Saber bolt block (primary entry point)
	//
	if (wp_saber_must_bolt_block(other,
		ent,
		qfalse,
		trace->endpos,
		-1,
		-1) &&
		!WP_DoingForcedAnimationForForcePowers(other))
	{
		// Reset weaponTime if not attacking OR if using force powers
		if (other->client &&
			(!PM_SaberInAttack(other->client->ps.saber_move) ||
				(pm->cmd.buttons & (BUTTON_FORCEPOWER |
					BUTTON_FORCEGRIP |
					BUTTON_DASH |
					BUTTON_FORCE_LIGHTNING))))
		{
			other->client->ps.weaponTime = 0;
		}

		wp_handle_bolt_block(ent, other, trace, fwd);

		if (other->owner && other->owner->client)
		{
			other->owner->client->ps.saberEventFlags |= SEF_DEFLECTED;
		}

		return qtrue;
	}

	//
	// Fallback: hit the saber geometry (CONTENTS_LIGHTSABER)
	//
	if ((other->r.contents & CONTENTS_LIGHTSABER) && !is_knocked_saber)
	{
		gentity_t* saber_owner = &g_entities[other->r.ownerNum];

		if (saber_owner->takedamage &&
			saber_owner->client &&
			ent->s.weapon != WP_ROCKET_LAUNCHER &&
			ent->s.weapon != WP_THERMAL &&
			ent->s.weapon != WP_TRIP_MINE &&
			ent->s.weapon != WP_DET_PACK &&
			ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_CONC &&
			ent->methodOfDeath != MOD_CONC_ALT)
		{
			if (saber_owner->client &&
				(!PM_SaberInAttack(saber_owner->client->ps.saber_move) ||
					(pm->cmd.buttons & (BUTTON_FORCEPOWER |
						BUTTON_FORCEGRIP |
						BUTTON_DASH |
						BUTTON_FORCE_LIGHTNING))) &&
				!WP_DoingForcedAnimationForForcePowers(other))
			{
				saber_owner->client->ps.weaponTime = 0;
			}

			wp_handle_bolt_block(ent, saber_owner, trace, fwd);

			if (saber_owner->client)
			{
				saber_owner->client->ps.saberEventFlags |= SEF_DEFLECTED;
			}

			return qtrue;
		}
	}

	//
	// Missile sticking (trip mines, etc.)
	//
	if (!other->takedamage &&
		(ent->s.eFlags & EF_MISSILE_STICK) &&
		ent->s.weapon != WP_SABER)
	{
		laserTrapStick(ent, trace->endpos, trace->plane.normal);
		G_AddEvent(ent, EV_MISSILE_STICK, 0);
		return qtrue;
	}

	//
	// Impact damage (direct hit)
	//
	if (other->takedamage && !is_knocked_saber)
	{
		missile_dmg = ent->damage;

		// Dodge check (player dodge roll)
		if (G_DoDodge(other,
			&g_entities[other->r.ownerNum],
			trace->endpos,
			-1,
			&missile_dmg,
			ent->methodOfDeath))
		{
			return qfalse; // missile continues
		}

		if (missile_dmg)
		{
			vec3_t velocity;
			qboolean did_dmg = qfalse;

			gentity_t* ownerEnt = G_GetEntitySafe(ent->r.ownerNum);
			if (ownerEnt && ownerEnt->client && LogAccuracyHit(other, ownerEnt))
			{
				ownerEnt->client->accuracy_hits++;
				hit_client = qtrue;
			}

			BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity);

			if (VectorLength(velocity) == 0)
			{
				velocity[2] = 1; // stepped on a grenade
			}

			//
			// Weapon‑specific damage rules
			//
			if (ent->s.weapon == WP_BOWCASTER ||
				ent->s.weapon == WP_FLECHETTE ||
				ent->s.weapon == WP_ROCKET_LAUNCHER)
			{
				if (ent->s.weapon == WP_FLECHETTE &&
					(ent->s.eFlags & EF_ALT_FIRING))
				{
					if (ent->think == wp_flechette_alt_blow)
					{
						ent->think(ent);
					}
				}
				else
				{
					G_Damage(other,
						ent,
						&g_entities[ent->r.ownerNum],
						velocity,
						ent->r.currentOrigin,
						missile_dmg,
						DAMAGE_HALF_ABSORB,
						ent->methodOfDeath);
					did_dmg = qtrue;
				}
			}
			else
			{
				gentity_t* owner = G_GetEntitySafe(ent->r.ownerNum);
				float distance = owner ? VectorDistance(owner->r.currentOrigin,
					other->r.currentOrigin) : 999999.0f;

				if (distance <= 100.0f)
				{
					G_Damage(other, ent, owner,
						velocity, ent->r.currentOrigin,
						missile_dmg * 2, 0,
						ent->methodOfDeath);
				}
				else if (distance <= 300.0f)
				{
					G_Damage(other, ent, owner,
						velocity, ent->r.currentOrigin,
						missile_dmg * 1.5f, 0,
						ent->methodOfDeath);
				}
				else
				{
					G_Damage(other, ent,
						(owner ? owner : ent),
						velocity, ent->r.currentOrigin,
						missile_dmg, 0,
						ent->methodOfDeath);
				}

				did_dmg = qtrue;
			}

			//
			// Droid electrify feedback
			//
			if (did_dmg && other->client)
			{
				class_t npc_class = other->client->NPC_class;
				bclass_t bot_class = other->client->pers.botclass;

				qboolean is_droid =
					(npc_class == CLASS_SEEKER ||
						npc_class == CLASS_PROBE ||
						npc_class == CLASS_MOUSE ||
						npc_class == CLASS_SBD ||
						npc_class == CLASS_BATTLEDROID ||
						npc_class == CLASS_DROIDEKA ||
						npc_class == CLASS_GONK ||
						npc_class == CLASS_R2D2 ||
						npc_class == CLASS_R5D2 ||
						npc_class == CLASS_REMOTE ||
						npc_class == CLASS_MARK1 ||
						npc_class == CLASS_MARK2 ||
						npc_class == CLASS_PROTOCOL ||
						npc_class == CLASS_INTERROGATOR ||
						npc_class == CLASS_ATST ||
						npc_class == CLASS_SENTRY);

				qboolean is_bot_droid =
					(bot_class == BCLASS_BATTLEDROID ||
						bot_class == BCLASS_DROIDEKA ||
						bot_class == BCLASS_SBD ||
						bot_class == BCLASS_REMOTE ||
						bot_class == BCLASS_R2D2 ||
						bot_class == BCLASS_R5D2 ||
						bot_class == BCLASS_PROTOCOL);

				if (is_droid || is_bot_droid)
				{
					if (other->client->ps.electrifyTime < level.time + 100)
					{
						other->client->ps.electrifyTime =
							level.time + Q_irand(1500, 2000);
					}
				}
			}
		}

		//
		// DEMP2 special effects (vehicles, cloak)
		//
		if (ent->s.weapon == WP_DEMP2)
		{
			if (other->client &&
				other->client->NPC_class == CLASS_VEHICLE)
			{
				if (other->m_pVehicle &&
					other->m_pVehicle->m_pVehicleInfo &&
					(other->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
						(other->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER &&
							ent->classname &&
							Q_stricmp("vehicle_proj", ent->classname) == 0)) &&
					!FighterIsLanded(other->m_pVehicle, &other->client->ps) &&
					!(other->spawnflags & 2))
				{
					if (other->client->ps.electrifyTime > level.time)
					{
						other->client->ps.electrifyTime += Q_irand(200, 500);

						if (other->client->ps.electrifyTime >
							level.time + 4000)
						{
							other->client->ps.electrifyTime =
								level.time + 4000;
						}
					}
					else
					{
						other->client->ps.electrifyTime =
							level.time + Q_irand(200, 500);
					}
				}
			}
			else if (other->client &&
				other->client->ps.powerups[PW_CLOAKED])
			{
				Jedi_Decloak(other);

				if (ent->methodOfDeath == MOD_DEMP2_ALT)
				{
					other->client->cloakToggleTime = Q3_INFINITE;
				}
				else
				{
					other->client->cloakToggleTime =
						level.time + Q_irand(3000, 10000);
				}
			}
		}
	}
killProj:

	//
	// Grapple / Hook projectile
	//
	if (strcmp(ent->classname, "hook") == 0)
	{
		vec3_t v = { 0 };
		gentity_t* nent = G_Spawn();

		if (other->takedamage || other->client || other->s.eType == ET_MOVER)
		{
			G_PlayEffectID(G_EffectIndex("blaster/flesh_impact"),
				trace->endpos, trace->plane.normal);

			if (other->takedamage && other->client)
			{
				vec3_t dummy = { 0 };
				G_Damage(other, ent, ent,
					dummy, ent->r.currentOrigin,
					TASER_DAMAGE,
					DAMAGE_NO_KNOCKBACK,
					MOD_CRUSH);
			}

			nent->s.otherentity_num2 = other->s.number;
			ent->enemy = other;

			// Center of the target
			v[0] = other->r.currentOrigin[0] + (other->r.mins[0] + other->r.maxs[0]) * 0.5f;
			v[1] = other->r.currentOrigin[1] + (other->r.mins[1] + other->r.maxs[1]) * 0.5f;
			v[2] = other->r.currentOrigin[2] + (other->r.mins[2] + other->r.maxs[2]) * 0.5f;

			SnapVectorTowards(v, ent->s.pos.trBase);
		}
		else
		{
			VectorCopy(trace->endpos, v);
			G_PlayEffectID(G_EffectIndex("impacts/droid_impact1"),
				trace->endpos, trace->plane.normal);
			ent->enemy = NULL;
		}

		SnapVectorTowards(v, ent->s.pos.trBase);

		nent->freeAfterEvent = qtrue;
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GRAPPLE;

		G_SetOrigin(ent, v);
		G_SetOrigin(nent, v);

		ent->think = Weapon_HookThink;
		ent->nextthink = level.time + FRAMETIME;

		if (!other->takedamage)
		{
			ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
			VectorCopy(ent->r.currentOrigin,
				ent->parent->client->ps.lastHitLoc);
		}

		trap->LinkEntity((sharedEntity_t*)ent);
		trap->LinkEntity((sharedEntity_t*)nent);

		return qfalse;
	}

	//
	// Stun projectile
	//
	if (strcmp(ent->classname, "stun") == 0)
	{
		vec3_t v = { 0 };
		gentity_t* nent = G_Spawn();

		if (other->takedamage || other->client || other->s.eType == ET_MOVER)
		{
			G_PlayEffectID(G_EffectIndex("stunBaton/flesh_impact"),
				trace->endpos, trace->plane.normal);

			nent->s.otherentity_num2 = other->s.number;
			ent->enemy = other;

			if (other->takedamage && other->client)
			{
				other->client->stunDamage = 40;
				other->client->stunTime = level.time + 1000;

				if (other->client->ps.electrifyTime < level.time + 100)
				{
					other->client->ps.electrifyTime = level.time + 4000;
				}

				vec3_t dummy = { 0 };
				G_Damage(other, ent, ent,
					dummy, ent->r.currentOrigin,
					TASER_DAMAGE,
					DAMAGE_NO_KNOCKBACK,
					MOD_STUN_BATON);

				if (other->client->ps.stats[STAT_HEALTH] <= 0)
				{
					VectorClear(other->client->ps.lastHitLoc);
					VectorClear(other->client->ps.velocity);

					other->client->ps.eFlags |= EF_DISINTEGRATION;
					other->r.contents = 0;

					other->think = G_FreeEntity;
					other->nextthink = level.time;
				}
				else
				{
					G_KnockOver(other, nent, v, 25, qtrue);
				}
			}
		}
		else
		{
			VectorCopy(trace->endpos, v);
			G_PlayEffectID(G_EffectIndex("impacts/droid_impact1"),
				trace->endpos, trace->plane.normal);
			ent->enemy = NULL;
		}

		nent->freeAfterEvent = qtrue;
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GENERAL;

		G_SetOrigin(ent, v);
		G_SetOrigin(nent, v);

		trap->LinkEntity((sharedEntity_t*)ent);
		trap->LinkEntity((sharedEntity_t*)nent);

		return qfalse;
	}

	//
	// Impact events (hit, metal, miss)
	//
	if (other->takedamage && other->client && !is_knocked_saber)
	{
		G_AddEvent(ent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
		ent->s.otherentity_num = other->s.number;
	}
	else if (trace->surfaceFlags & SURF_METALSTEPS)
	{
		G_AddEvent(ent, EV_MISSILE_MISS_METAL, DirToByte(trace->plane.normal));
	}
	else if (ent->s.weapon != G2_MODEL_PART && !is_knocked_saber)
	{
		G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(trace->plane.normal));
	}

	//
	// Convert missile to impact entity
	//
	if (!is_knocked_saber)
	{
		ent->freeAfterEvent = qtrue;
		ent->s.eType = ET_GENERAL;
	}

	SnapVectorTowards(trace->endpos, ent->s.pos.trBase);
	G_SetOrigin(ent, trace->endpos);

	ent->takedamage = qfalse;

	//
	// Splash damage
	//
	if (ent->splashDamage)
	{
		if (g_radius_damage(trace->endpos,
			ent->parent,
			ent->splashDamage,
			ent->splashRadius,
			other,
			ent,
			ent->splashMethodOfDeath))
		{
			gentity_t* splashOwner = G_GetEntitySafe(ent->r.ownerNum);
			if (!hit_client && splashOwner && splashOwner->client)
			{
				splashOwner->client->accuracy_hits++;
			}
		}
	}

	//
	// G2 model parts free themselves
	//
	if (ent->s.weapon == G2_MODEL_PART)
	{
		ent->freeAfterEvent = qfalse;
	}

	trap->LinkEntity((sharedEntity_t*)ent);

	return qtrue;
}

/*
================
g_run_missile
================
*/
extern int g_real_trace(
	gentity_t* attacker,
	trace_t* tr,
	vec3_t start,
	vec3_t mins,
	vec3_t maxs,
	vec3_t end,
	const int pass_entity_num,
	const int contentmask,
	const int rSaberNum,
	const int rBladeNum
);

void g_run_missile(gentity_t* ent)
{
	vec3_t origin, ground_spot;
	trace_t tr;
	int passent;
	qboolean is_knocked_saber = qfalse;

	if (ent->neverFree && ent->s.weapon == WP_SABER && ent->flags & FL_BOUNCE_HALF)
	{
		is_knocked_saber = qtrue;
		if (!(ent->s.eFlags & EF_MISSILE_STICK))
		{
			//only go into gravity mode if we're not stuck to something
			ent->s.pos.trType = TR_GRAVITY;
		}
	}

	// get current position
	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);

	// if this missile bounced off an invulnerability sphere
	if (ent->targetEnt)
	{
		passent = ent->targetEnt->s.number;
	}
	else
	{
		// ignore interactions with the missile owner
		if (ent->r.svFlags & SVF_OWNERNOTSHARED
			&& ent->s.eFlags & EF_JETPACK_ACTIVE)
		{
			//A vehicle missile that should be solid to its owner
			//I don't care about hitting my owner
			passent = ent->s.number;
		}
		else
		{
			passent = ent->r.ownerNum;
		}
	}

	// trace a line from the previous position to the current position
	g_real_trace(ent, &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask, -1, -1);

	if (tr.startsolid || tr.allsolid)
	{
		// make sure the tr.entityNum is set to the entity we're stuck in
	}
	else
	{
		VectorCopy(tr.endpos, ent->r.currentOrigin);
	}

	if (ent->passThroughNum && tr.entityNum == ent->passThroughNum - 1)
	{
		VectorCopy(origin, ent->r.currentOrigin);
		trap->LinkEntity((sharedEntity_t*)ent);
		goto passthrough;
	}

	trap->LinkEntity((sharedEntity_t*)ent);

	if (ent->s.weapon == G2_MODEL_PART && !ent->bounceCount)
	{
		vec3_t lower_org;
		trace_t tr_g;

		VectorCopy(ent->r.currentOrigin, lower_org);
		lower_org[2] -= 1;
		trap->Trace(&tr_g, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, lower_org, passent, ent->clipmask, qfalse, 0,
			0);

		VectorCopy(tr_g.endpos, ground_spot);

		if (!tr_g.startsolid && !tr_g.allsolid && tr_g.entityNum == ENTITYNUM_WORLD)
		{
			ent->s.groundEntityNum = tr_g.entityNum;
		}
		else
		{
			ent->s.groundEntityNum = ENTITYNUM_NONE;
		}
	}

	if (tr.fraction != 1)
	{
		// never explode or bounce on sky
		if (tr.surfaceFlags & SURF_NOIMPACT)
		{
			// If grapple, reset owner
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent)
			{
				ent->parent->client->hook = NULL;
				ent->parent->client->hookhasbeenfired = qfalse;
				ent->parent->client->fireHeld = qfalse;
			}
			else if (ent->parent && ent->parent->client && ent->parent->client->stun == ent)
			{
				ent->parent->client->stun = NULL;
				ent->parent->client->stunhasbeenfired = qfalse;
				ent->parent->client->stunHeld = qfalse;
			}
			else if (ent->s.weapon == WP_SABER && ent->isSaberEntity || is_knocked_saber)
			{
				G_RunThink(ent);
				return;
			}
			else if (ent->s.weapon != G2_MODEL_PART)
			{
				G_FreeEntity(ent);
				return;
			}
		}

#if 0 //will get stomped with missile impact event...
		if (ent->s.weapon > WP_NONE && ent->s.weapon < WP_NUM_WEAPONS &&
			(tr.entityNum < MAX_CLIENTS || g_entities[tr.entityNum].s.eType == ET_NPC))
		{ //player or NPC, try making a mark on him
		  //ok, let's try adding it to the missile ent instead
			G_AddEvent(ent, EV_GHOUL2_MARK, 0);

			//copy current pos to s.origin, and current projected to origin2
			VectorCopy(ent->r.currentOrigin, ent->s.origin);
			BG_EvaluateTrajectory(&ent->s.pos, level.time, ent->s.origin2);

			//the index for whoever we are hitting
			ent->s.otherentity_num = tr.entityNum;

			if (VectorCompare(ent->s.origin, ent->s.origin2))
			{
				ent->s.origin2[2] += 2.0f; //whatever, at least it won't mess up.
			}
		}
#else
		if (ent->s.weapon > WP_NONE && ent->s.weapon < WP_NUM_WEAPONS &&
			(tr.entityNum < MAX_CLIENTS || g_entities[tr.entityNum].s.eType == ET_NPC))
		{
			//player or NPC, try making a mark on him
			//copy current pos to s.origin, and current projected to origin2
			VectorCopy(ent->r.currentOrigin, ent->s.origin);
			BG_EvaluateTrajectory(&ent->s.pos, level.time, ent->s.origin2);

			if (VectorCompare(ent->s.origin, ent->s.origin2))
			{
				ent->s.origin2[2] += 2.0f; //whatever, at least it won't mess up.
			}
		}
#endif

		if (!G_MissileImpact(ent, &tr))
		{
			//target dodged the damage.
			VectorCopy(origin, ent->r.currentOrigin);
			trap->LinkEntity((sharedEntity_t*)ent);
			return;
		}

		if (tr.entityNum == ent->s.otherentity_num)
		{
			//if the impact event other and the trace ent match then it's ok to do the g2 mark
			ent->s.trickedentindex = 1;
		}

		if (ent->s.eType != ET_MISSILE && ent->s.weapon != G2_MODEL_PART)
		{
			return; // exploded
		}
	}

passthrough:
	if (ent->s.pos.trType == TR_STATIONARY && ent->s.eFlags & EF_MISSILE_STICK)
	{
		//stuck missiles should check some special stuff
		g_run_stuck_missile(ent);
		return;
	}

	if (ent->s.weapon == G2_MODEL_PART)
	{
		if (ent->s.groundEntityNum == ENTITYNUM_WORLD)
		{
			ent->s.pos.trType = TR_LINEAR;
			VectorClear(ent->s.pos.trDelta);
			ent->s.pos.trTime = level.time;

			VectorCopy(ground_spot, ent->s.pos.trBase);
			VectorCopy(ground_spot, ent->r.currentOrigin);

			if (ent->s.apos.trType != TR_STATIONARY)
			{
				ent->s.apos.trType = TR_STATIONARY;
				ent->s.apos.trTime = level.time;

				ent->s.apos.trBase[ROLL] = 0;
				ent->s.apos.trBase[PITCH] = 0;
			}
		}
	}

	// check think function after bouncing
	G_RunThink(ent);
}

//===========================grapplemod===============================
/*
=================
fire_grapple
=================
*/
gentity_t* fire_grapple(gentity_t* self, vec3_t start, vec3_t dir)
{
	VectorNormalize(dir);

	gentity_t* hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_MELEE;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_ELECTROCUTE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->targetEnt = NULL;
	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
	hook->s.otherentity_num = self->s.number; // use to match beam in client
	VectorCopy(start, hook->s.pos.trBase);
	VectorScale(dir, g_grapple_shoot_speed.integer, hook->s.pos.trDelta); // lmo scale speed!
	SnapVector(hook->s.pos.trDelta); // save net bandwidth
	VectorCopy(start, hook->r.currentOrigin);
	self->client->hook = hook;

	return hook;
}

gentity_t* fire_stun(gentity_t* self, vec3_t start, vec3_t dir)
{
	VectorNormalize(dir);

	gentity_t* stun = G_Spawn();
	stun->classname = "stun";
	stun->nextthink = level.time + 10000;
	stun->think = Weapon_StunFree;
	stun->s.eType = ET_MISSILE;
	stun->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	stun->s.weapon = WP_STUN_BATON;
	stun->r.ownerNum = self->s.number;
	stun->methodOfDeath = MOD_ELECTROCUTE;
	stun->clipmask = MASK_SHOT;
	stun->parent = self;
	stun->targetEnt = NULL;
	stun->s.pos.trType = TR_LINEAR;
	stun->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;
	stun->s.otherentity_num = self->s.number;
	VectorCopy(start, stun->s.pos.trBase);
	VectorScale(dir, 2000, stun->s.pos.trDelta);
	SnapVector(stun->s.pos.trDelta);
	VectorCopy(start, stun->r.currentOrigin);
	self->client->stun = stun;

	return stun;
}

//=============================================================================

static int ReflectionLevel(const gentity_t* player)
{
	// Determine reflection level.

	// Manual blocking flag (safe bit test)
	const qboolean manual_blocking =
		(player->client->ps.ManualBlockingFlags & (1 << HOLDINGBLOCK)) ? qtrue : qfalse;

	// NPCs may block manually depending on AI logic
	const int npc_is_blocking = manual_npc_saberblocking(player);

	if (manual_blocking || npc_is_blocking)
	{
		// Manual reflection: bounce toward crosshair / attacker
		return FORCE_LEVEL_3;
	}

	// Default: weak deflection
	return FORCE_LEVEL_1;
}

void wp_handle_bolt_block(gentity_t* bolt, gentity_t* blocker, trace_t* trace, vec3_t fwd)
{
	// Safety: validate pointers (bug fix – previously could crash on NULL)
	if (!bolt || !blocker || !blocker->client || !trace)
	{
		return;
	}

	// Handles all the behavior needed to saber block a blaster bolt.
	const int   other_def_level = ReflectionLevel(blocker);
	float slop_factor =
		(float)(MISHAP_MAXINACCURACY - 6) *
		(float)(FORCE_LEVEL_3 - blocker->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE]) /
		(float)FORCE_LEVEL_3;

	gentity_t* prev_owner = G_GetEntitySafe(bolt->r.ownerNum);
	const float distance = prev_owner ? vector_bolt_distance(blocker->r.currentOrigin, prev_owner->r.currentOrigin) : 99999.0f;

	const qboolean manual_proj_blocking =
		(blocker->client->ps.ManualBlockingFlags & (1 << HOLDINGBLOCKANDATTACK)) ? qtrue : qfalse;
	const qboolean accurate_missile_block =
		(blocker->client->ps.ManualBlockingFlags & (1 << MBF_ACCURATEMISSILEBLOCKING)) ? qtrue : qfalse;

	const int manual_run_blocking = manual_running_and_saberblocking(blocker);
	const int npc_is_blocking = manual_npc_saberblocking(blocker);

	// Visual effect for the bolt being blocked
	g_missile_reflect_effect(blocker, trace->plane.normal);

	// Base forward direction from blocker view
	AngleVectors(blocker->client->ps.viewangles, fwd, NULL, NULL);

	//
	// Low defense level: random deflect away, heavy fatigue
	//
	if (other_def_level <= FORCE_LEVEL_1)
	{
		const int punish = BLOCKPOINTS_TEN;

		// Only randomly deflect away the bolt
		g_missile_bouncedoff_saber(blocker, bolt, fwd);

		if ((d_blockinfo.integer || g_DebugSaberCombat.integer) &&
			!(blocker->r.svFlags & SVF_BOT))
		{
			Com_Printf(S_COLOR_YELLOW "only randomly deflect away the bolt\n");
		}

		WP_BrokenBoltBlockKnockBack(blocker);
		PM_AddBlockFatigue(&blocker->client->ps, punish);
	}
	else
	{
		float block_points_used;

		//
		// Very close range: send to enemy
		//
		if (distance < 80.0f)
		{
			g_deflect_missile_to_attacker(blocker, bolt, fwd);

			if ((d_blockinfo.integer || g_DebugSaberCombat.integer) &&
				!(blocker->r.svFlags & SVF_BOT))
			{
				Com_Printf(S_COLOR_YELLOW "GOES TO ENEMY\n");
			}

			if (blocker->client->ps.fd.blockPoints <= BLOCKPOINTS_THIRTY)
			{
				// Low points = bad blocks
				if (blocker->client->ps.fd.blockPoints <= BLOCKPOINTS_FATIGUE)
				{
					// Very low points = broken block
					WP_BrokenBoltBlockKnockBack(blocker);
					blocker->client->ps.saberBlocked = BLOCKED_NONE;
					blocker->client->ps.saber_move = LS_NONE;
				}
				else
				{
					WP_SaberFatiguedParryDirection(blocker, bolt->r.currentOrigin, qtrue);
				}
			}
			else
			{
				WP_SaberBlockBolt(blocker, bolt->r.currentOrigin, qtrue);
			}

			// Block‑point cost
			if (accurate_missile_block)
			{
				block_points_used = 2.0f;
			}
			else
			{
				block_points_used = WP_SaberBoltBlockCost(blocker, bolt);
			}

			if (blocker->client->ps.fd.blockPoints < block_points_used)
			{
				blocker->client->ps.fd.blockPoints = 0;
			}
			else
			{
				WP_BlockPointsDrain(blocker, block_points_used);
			}
		}
		//
		// Manual / running / NPC blocking: send to enemy
		//
		else if (manual_proj_blocking || manual_run_blocking || npc_is_blocking)
		{
			g_reflect_missile_to_attacker(blocker, bolt, fwd);

			if ((d_blockinfo.integer || g_DebugSaberCombat.integer) &&
				!(blocker->r.svFlags & SVF_BOT))
			{
				Com_Printf(S_COLOR_YELLOW "GOES TO ENEMY\n");
			}

			if (blocker->client->ps.fd.blockPoints <= BLOCKPOINTS_THIRTY)
			{
				if (blocker->client->ps.fd.blockPoints <= BLOCKPOINTS_FATIGUE)
				{
					WP_BrokenBoltBlockKnockBack(blocker);
					blocker->client->ps.saberBlocked = BLOCKED_NONE;
					blocker->client->ps.saber_move = LS_NONE;
				}
				else
				{
					WP_SaberFatiguedParryDirection(blocker, bolt->r.currentOrigin, qtrue);
				}
			}
			else
			{
				WP_SaberBlockBolt(blocker, bolt->r.currentOrigin, qtrue);
			}

			if (accurate_missile_block)
			{
				block_points_used = 2.0f;
			}
			else
			{
				block_points_used = WP_SaberBoltBlockCost(blocker, bolt);
			}

			if (blocker->client->ps.fd.blockPoints < block_points_used)
			{
				blocker->client->ps.fd.blockPoints = 0;
			}
			else
			{
				WP_BlockPointsDrain(blocker, block_points_used);
			}
		}
		//
		// Default: send toward crosshair (with slop)
		//
		else
		{
			vec3_t angs;
			vec3_t bounce_dir;

			if ((d_blockinfo.integer || g_DebugSaberCombat.integer) &&
				!(blocker->r.svFlags & SVF_BOT))
			{
				Com_Printf(S_COLOR_YELLOW "GOES TO CROSSHAIR\n");
			}

			if (level.time - blocker->client->ps.ManualblockStartTime < 3000)
			{
				// Good: early manual block, minimal slop
				vectoangles(fwd, angs);
				AngleVectors(angs, fwd, NULL, NULL);
			}
			else if (blocker->client->pers.cmd.forwardmove >= 0)
			{
				// Bad if moving forward: more slop
				slop_factor += Q_irand(1, 5);
				vectoangles(fwd, angs);
				angs[PITCH] += flrand(-slop_factor, slop_factor);
				angs[YAW] += flrand(-slop_factor, slop_factor);
				AngleVectors(angs, fwd, NULL, NULL);
			}
			else
			{
				// Average after 3 seconds: moderate slop
				slop_factor += Q_irand(1, 3);
				vectoangles(fwd, angs);
				angs[PITCH] += flrand(-slop_factor, slop_factor);
				angs[YAW] += flrand(-slop_factor, slop_factor);
				AngleVectors(angs, fwd, NULL, NULL);
			}

			if (blocker->client->ps.fd.blockPoints <= BLOCKPOINTS_THIRTY)
			{
				if (blocker->client->ps.fd.blockPoints <= BLOCKPOINTS_FATIGUE)
				{
					WP_BrokenBoltBlockKnockBack(blocker);
					blocker->client->ps.saberBlocked = BLOCKED_NONE;
					blocker->client->ps.saber_move = LS_NONE;
				}
				else
				{
					WP_SaberFatiguedParryDirection(blocker, bolt->r.currentOrigin, qtrue);
				}
			}
			else
			{
				wp_saber_block_non_random_missile(blocker, bolt->r.currentOrigin, qtrue);
			}

			if (accurate_missile_block)
			{
				block_points_used = 2.0f;
			}
			else
			{
				block_points_used = WP_SaberBoltBlockCost(blocker, bolt);
			}

			if (blocker->client->ps.fd.blockPoints < block_points_used)
			{
				blocker->client->ps.fd.blockPoints = 0;
			}
			else
			{
				WP_BlockPointsDrain(blocker, block_points_used);
			}

			// Save the original speed
			const float speed = VectorNormalize(bolt->s.pos.trDelta);

			VectorCopy(fwd, bounce_dir);
			VectorScale(bounce_dir, speed, bolt->s.pos.trDelta);

			bolt->s.pos.trTime = level.time;
			VectorCopy(bolt->r.currentOrigin, bolt->s.pos.trBase);

			if (bolt->s.weapon != WP_SABER && bolt->s.weapon != G2_MODEL_PART)
			{
				// You are mine now!
				bolt->r.ownerNum = blocker->s.number;
			}

			if (bolt->s.weapon == WP_ROCKET_LAUNCHER)
			{
				// Stop homing
				bolt->think = NULL;
				bolt->nextthink = 0;
			}
		}
	}

	// For Jedi AI
	blocker->client->ps.saberEventFlags |= SEF_DEFLECTED;

	// Remember original owner for damage credit (may be NULL)
	bolt->activator = prev_owner;

	// Manual missile‑blocking cooldown
	blocker->client->ps.ManualMBlockingTime =
		level.time + (600 - blocker->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] * 200);
}