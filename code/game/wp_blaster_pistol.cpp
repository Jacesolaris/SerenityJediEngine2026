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

//---------------
//	Bryar Pistol
//---------------

extern qboolean WalkCheck(const gentity_t* self);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
extern qboolean PM_RunningAnim(int anim);
extern qboolean PM_WalkingAnim(int anim);

//---------------------------------------------------------
void WP_FireBryarPistol(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	// SAFETY: ent or ent->client may be NULL in edge cases (NPCs, turrets, bad spawns)
	if (ent == NULL || ent->client == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireBryarPistol: NULL ent or ent->client\n");
		return;
	}

	vec3_t start;
	int damage = (alt_fire == qfalse) ? weaponData[WP_BLASTER_PISTOL].damage : weaponData[WP_BLASTER_PISTOL].altDamage;

	// Starting point of the shot
	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start);

	// ---------------------------------------------------------------------
	// AIM / SPREAD LOGIC
	// ---------------------------------------------------------------------
	if (ent->client->NPC_class == CLASS_VEHICLE)
	{
		// Vehicles: no inherent aim screw up
	}
	else if (NPC_IsNotHavingEnoughForceSight(ent) == qtrue)
	{
		// Force Sight 2+ gives perfect aim; below that we add spread
		vec3_t angs;
		vectoangles(forward_vec, angs);

		const qboolean is_player_or_controlled = ((ent->s.number < MAX_CLIENTS) || (G_ControlledByPlayer(ent) == qtrue)) ? qtrue : qfalse;

		if (alt_fire == qtrue)
		{
			if (is_player_or_controlled == qtrue)
			{
				if ((PM_CrouchAnim(ent->client->ps.legsAnim) == qtrue) || g_entities[ent->s.number].client->IsAiming == qtrue)
				{
					// Firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						// Running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						// Walking or somewhat fatigued
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{
						// Standing still
						angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
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
				if ((PM_CrouchAnim(ent->client->ps.legsAnim) == qtrue) || g_entities[ent->s.number].client->IsAiming == qtrue)
				{
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF && g_entities[ent->s.number].client->IsAiming == qfalse)
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

		AngleVectors(angs, forward_vec, NULL, NULL);
	}

	// Provide target hinting for homing / assist logic
	WP_MissileTargetHint(ent, start, forward_vec);

	// ---------------------------------------------------------------------
	// MISSILE CREATION
	// ---------------------------------------------------------------------
	gentity_t* missile = CreateMissile(start, forward_vec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

	if (missile == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireBryarPistol: CreateMissile returned NULL\n");
		return;
	}

	missile->classname = "bryar_proj";

	// Weapon identity: Bryar vs Blaster Pistol / Jawa
	if (ent->s.weapon == WP_BRYAR_PISTOL || ent->s.weapon == WP_JAWA)
	{
		missile->s.weapon = ent->s.weapon;
	}
	else
	{
		missile->s.weapon = WP_BLASTER_PISTOL;
	}

	// ---------------------------------------------------------------------
	// ALT-FIRE CHARGE LOGIC
	// ---------------------------------------------------------------------
	if (alt_fire == qtrue)
	{
		int count = (level.time - ent->client->ps.weaponChargeTime) / BRYAR_CHARGE_UNIT;

		if (count < 1) count = 1;
		else if (count > 5) count = 5;

		damage *= count;
		missile->count = count;
	}

	// ---------------------------------------------------------------------
	// DAMAGE / DEATH SETUP
	// ---------------------------------------------------------------------
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = (alt_fire == qtrue) ? MOD_BRYAR_ALT : MOD_BRYAR;

	missile->clipmask = MASK_SHOT;
	missile->bounceCount = 8;

	// Dual pistols: toggle muzzle point
	if (ent->weaponModel[1] > 0)
	{
		ent->count = (ent->count != 0) ? 0 : 1;
	}
}

//---------------------------------------------------------
void WP_FireBryarPistolDuals(gentity_t* ent, const qboolean alt_fire, const qboolean second_pistol)
//---------------------------------------------------------
{
	// SAFETY: ent or ent->client may be NULL in edge cases
	if (ent == NULL || ent->client == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireBryarPistolDuals: NULL ent or ent->client\n");
		return;
	}

	vec3_t start;
	int damage = (alt_fire == qfalse)
		? weaponData[WP_BLASTER_PISTOL].damage
		: weaponData[WP_BLASTER_PISTOL].altDamage;

	// Choose muzzle based on which pistol is firing
	if (second_pistol == qtrue)
	{
		VectorCopy(muzzle2, start);
	}
	else
	{
		VectorCopy(muzzle, start);
	}

	WP_TraceSetStart(ent, start);

	// AIM / SPREAD LOGIC
	if (ent->client->NPC_class == CLASS_VEHICLE)
	{
		// Vehicles: no inherent aim screw up
	}
	else if (NPC_IsNotHavingEnoughForceSight(ent) == qtrue)
	{
		vec3_t angs;
		vectoangles(forward_vec, angs);

		const qboolean is_player_or_controlled =
			((ent->s.number < MAX_CLIENTS) || (G_ControlledByPlayer(ent) == qtrue))
			? qtrue : qfalse;

		if (alt_fire == qtrue)
		{
			if (is_player_or_controlled == qtrue)
			{
				if ((PM_CrouchAnim(ent->client->ps.legsAnim) == qtrue) || g_entities[ent->s.number].client->IsAiming == qtrue)
				{
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{
						angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (is_player_or_controlled == qtrue)
			{
				if ((PM_CrouchAnim(ent->client->ps.legsAnim) == qtrue) || g_entities[ent->s.number].client->IsAiming == qtrue)
				{
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF && g_entities[ent->s.number].client->IsAiming == qfalse)
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
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
		}

		AngleVectors(angs, forward_vec, NULL, NULL);
	}

	WP_MissileTargetHint(ent, start, forward_vec);

	gentity_t* missile = CreateMissile(start, forward_vec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

	if (missile == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireBryarPistolDuals: CreateMissile returned NULL\n");
		return;
	}

	missile->classname = "bryar_proj";

	if (ent->s.weapon == WP_BRYAR_PISTOL || ent->s.weapon == WP_JAWA)
	{
		missile->s.weapon = ent->s.weapon;
	}
	else
	{
		missile->s.weapon = WP_BLASTER_PISTOL;
	}

	if (alt_fire == qtrue)
	{
		int count = (level.time - ent->client->ps.weaponChargeTime) / BRYAR_CHARGE_UNIT;

		if (count < 1)
		{
			count = 1;
		}
		else if (count > 5)
		{
			count = 5;
		}

		damage *= count;
		missile->count = count;
	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = (alt_fire == qtrue) ? MOD_BRYAR_ALT : MOD_BRYAR;

	missile->clipmask = MASK_SHOT;
	missile->bounceCount = 8;

	if (ent->weaponModel[1] > 0)
	{
		ent->count = (ent->count != 0) ? 0 : 1;
	}
}

//---------------
//	sbdBlaster
//---------------

//---------------------------------------------------------
static void WP_FireBryarsbdMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire)
//---------------------------------------------------------
{
	constexpr int velocity = BRYAR_PISTOL_VEL;
	int damage = alt_fire ? weaponData[WP_SBD_PISTOL].altDamage : weaponData[WP_SBD_PISTOL].damage;

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, alt_fire);

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_SBD_PISTOL;

	// Do the damages
	if (alt_fire)
	{
		int count = (level.time - ent->client->ps.weaponChargeTime) / BRYAR_CHARGE_UNIT;

		if (count < 1)
		{
			count = 1;
		}
		else if (count > 5)
		{
			count = 5;
		}

		damage *= count;
		missile->count = count; // this will get used in the projectile rendering code to make a beefier effect
	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	if (alt_fire)
	{
		missile->methodOfDeath = MOD_PISTOL_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_PISTOL;
	}

	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireBryarsbdPistol(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	// SAFETY: ent or ent->client may be NULL in edge cases
	if (ent == NULL || ent->client == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireBryarsbdPistol: NULL ent or ent->client\n");
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
				if (PM_CrouchAnim(ent->client->ps.legsAnim) == qtrue || g_entities[ent->s.number].client->IsAiming == qtrue)
				{
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY && g_entities[ent->s.number].client->IsAiming == qfalse)
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
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF && g_entities[ent->s.number].client->IsAiming == qfalse)
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
	WP_FireBryarsbdMissile(ent, muzzle, dir, alt_fire);
}

//---------------------------------------------------------
void WP_FireJawaPistol(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	// SAFETY: ent or ent->client may be NULL in edge cases
	if (ent == NULL || ent->client == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireJawaPistol: NULL ent or ent->client\n");
		return;
	}

	vec3_t start;
	int damage = (alt_fire == qfalse)
		? weaponData[WP_JAWA].damage
		: weaponData[WP_JAWA].altDamage;

	// Starting point of the shot
	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start);

	// AIM / SPREAD LOGIC
	if (ent->client->NPC_class == CLASS_VEHICLE)
	{
		// Vehicles: no inherent aim screw up
	}
	else if (NPC_IsNotHavingEnoughForceSight(ent) == qtrue)
	{
		vec3_t angs;
		vectoangles(forward_vec, angs);

		const qboolean is_player_or_controlled =
			((ent->s.number < MAX_CLIENTS) || (G_ControlledByPlayer(ent) == qtrue))
			? qtrue : qfalse;

		if (alt_fire == qtrue)
		{
			if (is_player_or_controlled == qtrue)
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim) == qtrue || g_entities[ent->s.number].client->IsAiming == qtrue)
				{
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{
						angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else
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
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF && g_entities[ent->s.number].client->IsAiming == qfalse)
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
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
		}

		AngleVectors(angs, forward_vec, NULL, NULL);
	}

	WP_MissileTargetHint(ent, start, forward_vec);

	gentity_t* missile = CreateMissile(start, forward_vec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

	if (missile == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireJawaPistol: CreateMissile returned NULL\n");
		return;
	}

	missile->classname = "bryar_proj";

	if (ent->s.weapon == WP_BLASTER_PISTOL ||
		ent->s.weapon == WP_SBD_PISTOL ||
		ent->s.weapon == WP_BRYAR_PISTOL)
	{
		missile->s.weapon = ent->s.weapon;
	}
	else
	{
		missile->s.weapon = WP_JAWA;
	}

	if (alt_fire == qtrue)
	{
		int count = (level.time - ent->client->ps.weaponChargeTime) / BRYAR_CHARGE_UNIT;

		if (count < 1)
		{
			count = 1;
		}
		else if (count > 5)
		{
			count = 5;
		}

		damage *= count;
		missile->count = count;
	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = (alt_fire == qtrue) ? MOD_BRYAR_ALT : MOD_BRYAR_ALT; // preserves current behaviour

	missile->clipmask = MASK_SHOT;
	missile->bounceCount = 8;

	if (ent->weaponModel[1] > 0)
	{
		ent->count = (ent->count != 0) ? 0 : 1;
	}
}

//---------------------------------------------------------
void WP_FireBryarPistolold(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	// SAFETY: ent or ent->client may be NULL in edge cases
	if (ent == NULL || ent->client == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireBryarPistolold: NULL ent or ent->client\n");
		return;
	}

	vec3_t start;
	int damage = (alt_fire == qfalse)
		? weaponData[WP_BRYAR_PISTOL].damage
		: weaponData[WP_BRYAR_PISTOL].altDamage;

	// Starting point of the shot
	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start);

	// AIM / SPREAD LOGIC
	if (ent->client->NPC_class == CLASS_VEHICLE)
	{
		// Vehicles: no inherent aim screw up
	}
	else if (NPC_IsNotHavingEnoughForceSight(ent) == qtrue)
	{
		vec3_t angs;
		vectoangles(forward_vec, angs);

		const qboolean is_player_or_controlled =
			((ent->s.number < MAX_CLIENTS) || (G_ControlledByPlayer(ent) == qtrue))
			? qtrue : qfalse;

		if (alt_fire == qtrue)
		{
			if (is_player_or_controlled == qtrue)
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim) == qtrue || g_entities[ent->s.number].client->IsAiming == qtrue)
				{
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{
						angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else
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
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN && g_entities[ent->s.number].client->IsAiming == qfalse)
					{
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
						ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF && g_entities[ent->s.number].client->IsAiming == qfalse)
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
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
		}

		AngleVectors(angs, forward_vec, NULL, NULL);
	}

	WP_MissileTargetHint(ent, start, forward_vec);

	gentity_t* missile = CreateMissile(start, forward_vec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

	if (missile == NULL)
	{
		Com_Printf(S_COLOR_YELLOW "WP_FireBryarPistolold: CreateMissile returned NULL\n");
		return;
	}

	missile->classname = "bryar_proj";

	if (ent->s.weapon == WP_BLASTER_PISTOL ||
		ent->s.weapon == WP_JAWA)
	{
		missile->s.weapon = ent->s.weapon;
	}
	else
	{
		missile->s.weapon = WP_BRYAR_PISTOL;
	}

	if (alt_fire == qtrue)
	{
		int count = (level.time - ent->client->ps.weaponChargeTime) / BRYAR_CHARGE_UNIT;

		if (count < 1)
		{
			count = 1;
		}
		else if (count > 5)
		{
			count = 5;
		}

		damage *= count;
		missile->count = count;
	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = (alt_fire == qtrue) ? MOD_PISTOL_ALT : MOD_PISTOL;

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->bounceCount = 8;

	if (ent->weaponModel[1] > 0)
	{
		ent->count = (ent->count != 0) ? 0 : 1;
	}
}