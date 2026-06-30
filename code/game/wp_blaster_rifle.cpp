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

#include "g_local.h"
#include "b_local.h"
#include "wp_saber.h"
#include "w_local.h"

extern qboolean WalkCheck(const gentity_t* self);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
extern qboolean PM_RunningAnim(int anim);
extern qboolean PM_WalkingAnim(int anim);
extern qboolean NPC_IsMando(const gentity_t* self);

//---------------
//	Blaster
//---------------

//---------------------------------------------------------
void WP_FireBlasterMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire)
//---------------------------------------------------------
{
	int velocity = BLASTER_VELOCITY;
	int damage = alt_fire ? weaponData[WP_BLASTER].altDamage : weaponData[WP_BLASTER].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent && ent->client && ent->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(ent) && !NPC_IsMando(ent)) //not controlled by player and not a mando of any kind
		{
			if (g_spskill->integer < 2)
			{
				velocity *= BLASTER_NPC_VEL_CUT;
			}
			else
			{
				velocity *= BLASTER_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, alt_fire);

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_BLASTER;

	// If an enemy is shooting at us, lower the velocity so you have a chance to evade
	if (ent && ent->client && ent->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(ent) && !NPC_IsMando(ent)) //not controlled by player and not a mando of any kind
	{
		if (g_spskill->integer == 0)
		{
			damage = BLASTER_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = BLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BLASTER_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	if (alt_fire)
	{
		missile->methodOfDeath = MOD_BLASTER_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BLASTER;
	}
	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireBlaster(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	// SAFETY: ent or ent->client may be NULL in edge cases
	if (ent == NULL || ent->client == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireBlaster: NULL ent or ent->client\n");
		return;
	}

	vec3_t dir;
	vec3_t angs;

	// Convert forward vector to angles
	vectoangles(forward_vec, angs);

	// ---------------------------------------------------------------------
	// AIM / SPREAD LOGIC
	// ---------------------------------------------------------------------
	if (ent->client->NPC_class == CLASS_VEHICLE)
	{
		// Vehicles: no inherent aim screw up
	}
	else if (NPC_IsNotHavingEnoughForceSight(ent) == qtrue)
	{
		const qboolean is_player_or_controlled =
			((ent->s.number < MAX_CLIENTS) || (G_ControlledByPlayer(ent) == qtrue))
			? qtrue : qfalse;

		if (alt_fire == qtrue)
		{
			if (is_player_or_controlled == qtrue)
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim) == qtrue)
				{
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
					{
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
					}
				}
			}
			else
			{
				// NPC alt-fire spread
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else // PRIMARY FIRE
		{
			if (is_player_or_controlled == qtrue)
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim) == qtrue)
				{
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF)
					{
						angs[PITCH] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
					}
					else
					{
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{
				// NPC primary-fire spread
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
		}
	}

	// Convert final angles back to direction vector
	AngleVectors(angs, dir, NULL, NULL);

	// Fire the missile
	WP_FireBlasterMissile(ent, muzzle, dir, alt_fire);
}


//---------------------------------------------------------
static void WP_FireJangoWristMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire)
//---------------------------------------------------------
{
	int velocity = CLONECOMMANDO_VELOCITY;
	int damage = alt_fire ? weaponData[WP_WRIST_BLASTER].altDamage : weaponData[WP_WRIST_BLASTER].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent && ent->client && ent->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(ent) && !NPC_IsMando(ent)) //not controlled by player and not a mando of any kind
		{
			if (g_spskill->integer < 2)
			{
				velocity *= BLASTER_NPC_VEL_CUT;
			}
			else
			{
				velocity *= BLASTER_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, alt_fire);

	missile->classname = "clone_proj";
	missile->s.weapon = WP_WRIST_BLASTER;

	// If an enemy is shooting at us, lower the velocity so you have a chance to evade
	if (ent && ent->client && ent->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(ent) && !NPC_IsMando(ent)) //not controlled by player and not a mando of any kind
	{
		if (g_spskill->integer == 0)
		{
			damage = BLASTER_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = BLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BLASTER_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	if (alt_fire)
	{
		missile->methodOfDeath = MOD_BLASTER_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BLASTER;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireWristPistol(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	// SAFETY: ent or ent->client may be NULL in edge cases
	if (ent == NULL || ent->client == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireWristPistol: NULL ent or ent->client\n");
		return;
	}

	vec3_t dir;
	vec3_t angs;

	// Convert forward vector to angles
	vectoangles(forward_vec, angs);

	// ---------------------------------------------------------------------
	// AIM / SPREAD LOGIC
	// ---------------------------------------------------------------------
	if (ent->client->NPC_class == CLASS_VEHICLE)
	{
		// Vehicles: no inherent aim screw up
	}
	else if (NPC_IsNotHavingEnoughForceSight(ent) == qtrue)
	{
		const qboolean is_player_or_controlled =
			((ent->s.number < MAX_CLIENTS) || (G_ControlledByPlayer(ent) == qtrue))
			? qtrue : qfalse;

		if (alt_fire == qtrue)
		{
			if (is_player_or_controlled == qtrue)
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim) == qtrue)
				{
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
					{
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
					}
				}
			}
			else
			{
				// NPC alt-fire spread
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else // PRIMARY FIRE
		{
			if (is_player_or_controlled == qtrue)
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim) == qtrue)
				{
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF)
					{
						angs[PITCH] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
					}
					else
					{
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{
				// NPC primary-fire spread
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
		}
	}

	// Convert final angles back to direction vector
	AngleVectors(angs, dir, NULL, NULL);

	// Fire the missile
	WP_FireJangoWristMissile(ent, muzzle, dir, alt_fire);
}


//////// DROIDEKA ////////

//---------------------------------------------------------
static void WP_FireDroidekaDualPistolMissileDuals(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire)
//---------------------------------------------------------
{
	// SAFETY: ent or ent->client may be NULL in edge cases
	if (ent == NULL || ent->client == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireDroidekaDualPistolMissileDuals: NULL ent or ent->client\n");
		return;
	}

	int velocity = BLASTER_VELOCITY;
	int damage = (alt_fire == qtrue)
		? weaponData[WP_DROIDEKA].altDamage
		: weaponData[WP_DROIDEKA].damage;

	// Vehicle logic
	if (ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// Enemy NPC velocity reduction (evade chance)
		const qboolean is_enemy_npc =
			((ent->s.number >= MAX_CLIENTS) &&
				(G_ControlledByPlayer(ent) == qfalse) &&
				(NPC_IsMando(ent) == qfalse))
			? qtrue : qfalse;

		if (is_enemy_npc == qtrue)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= BLASTER_NPC_VEL_CUT;
			}
			else
			{
				velocity *= BLASTER_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, alt_fire);

	if (missile == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireDroidekaDualPistolMissileDuals: CreateMissile returned NULL\n");
		return;
	}

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_DROIDEKA;

	// Enemy NPC damage scaling
	const qboolean is_enemy_npc =
		((ent->s.number >= MAX_CLIENTS) &&
			(G_ControlledByPlayer(ent) == qfalse) &&
			(NPC_IsMando(ent) == qfalse))
		? qtrue : qfalse;

	if (is_enemy_npc == qtrue)
	{
		if (g_spskill->integer == 0)
		{
			damage = BLASTER_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = BLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BLASTER_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->methodOfDeath = (alt_fire == qtrue) ? MOD_BLASTER_ALT : MOD_BLASTER;

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->bounceCount = 8;

	// Alternate muzzle FX toggle
	ent->fxID = ~ent->fxID;

	// Dual pistols: toggle muzzle point
	if (ent->weaponModel[1] > 0)
	{
		ent->count = (ent->count != 0) ? 0 : 1;
	}
}


//---------------------------------------------------------
static void WP_FireDroidekaDualPistolMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire)
//---------------------------------------------------------
{
	// SAFETY: ent or ent->client may be NULL in edge cases
	if (ent == NULL || ent->client == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireDroidekaDualPistolMissile: NULL ent or ent->client\n");
		return;
	}

	int velocity = BLASTER_VELOCITY;
	int damage = (alt_fire == qtrue)
		? weaponData[WP_DROIDEKA].altDamage
		: weaponData[WP_DROIDEKA].damage;

	// Vehicle logic
	if (ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// Enemy NPC velocity reduction (evade chance)
		const qboolean is_enemy_npc =
			((ent->s.number >= MAX_CLIENTS) &&
				(G_ControlledByPlayer(ent) == qfalse) &&
				(NPC_IsMando(ent) == qfalse))
			? qtrue : qfalse;

		if (is_enemy_npc == qtrue)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= BLASTER_NPC_VEL_CUT;
			}
			else
			{
				velocity *= BLASTER_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, alt_fire);

	if (missile == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireDroidekaDualPistolMissile: CreateMissile returned NULL\n");
		return;
	}

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_DROIDEKA;

	// Enemy NPC damage scaling
	const qboolean is_enemy_npc =
		((ent->s.number >= MAX_CLIENTS) &&
			(G_ControlledByPlayer(ent) == qfalse) &&
			(NPC_IsMando(ent) == qfalse))
		? qtrue : qfalse;

	if (is_enemy_npc == qtrue)
	{
		if (g_spskill->integer == 0)
		{
			damage = BLASTER_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = BLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BLASTER_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->methodOfDeath = (alt_fire == qtrue) ? MOD_BLASTER_ALT : MOD_BLASTER;

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->bounceCount = 8;

	// Alternate muzzle FX toggle
	ent->fxID = ~ent->fxID;

	// Dual pistols: toggle muzzle point
	if (ent->weaponModel[1] > 0)
	{
		ent->count = (ent->count != 0) ? 0 : 1;
	}
}


//---------------------------------------------------------
void WP_FireDroidekaDualPistol(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forward_vec, angs);

	if (ent->client && ent->client->ps.BlasterAttackChainCount <= BLASTERMISHAPLEVEL_HEAVYER)
	{
		//no inherent aim screw up
	}
	else
	{//force sight 2+ gives perfect aim
		vectoangles(forward_vec, angs);

		if (alt_fire)
		{
			// add some slop to the alt-fire direction for NPC,s
			angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
		}
		else
		{
			// add some slop to the fire direction for NPC,s
			angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
		}

		AngleVectors(angs, forward_vec, nullptr, nullptr);
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	WP_FireDroidekaDualPistolMissile(ent, muzzle, dir, alt_fire);
}

//---------------------------------------------------------
void WP_FireDroidekaFPPistolDuals(gentity_t* ent, const qboolean alt_fire, const qboolean second_pistol)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forward_vec, angs);

	if (ent->client && ent->client->ps.BlasterAttackChainCount <= BLASTERMISHAPLEVEL_HEAVYER)
	{
		//no inherent aim screw up
	}
	else
	{//force sight 2+ gives perfect aim
		vectoangles(forward_vec, angs);

		if (alt_fire)
		{
			// add some slop to the alt-fire direction for NPC,s
			angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
		}
		else
		{
			// add some slop to the fire direction for NPC,s
			angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
		}

		AngleVectors(angs, forward_vec, nullptr, nullptr);
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	if (second_pistol)
	{
		WP_FireDroidekaDualPistolMissileDuals(ent, muzzle2, dir, alt_fire);
	}
	else
	{
		WP_FireDroidekaDualPistolMissileDuals(ent, muzzle, dir, alt_fire);
	}
}