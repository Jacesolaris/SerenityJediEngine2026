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

extern qboolean walk_check(const gentity_t* self);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
extern qboolean PM_RunningAnim(int anim);
extern qboolean PM_WalkingAnim(int anim);

//---------------------------------------------------------
void WP_FireBryarPistol(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t start;
	int damage = !alt_fire ? weaponData[WP_BLASTER_PISTOL].damage : weaponData[WP_BLASTER_PISTOL].altDamage;

	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (NPC_IsNotHavingEnoughForceSight(ent))
	{//force sight 2+ gives perfect aim
		vec3_t angs;

		vectoangles(forward_vec, angs);

		if (alt_fire)
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{// add some slop to the alt-fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{// add some slop to the fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
		}

		AngleVectors(angs, forward_vec, nullptr, nullptr);
	}

	WP_MissileTargetHint(ent, start, forward_vec);

	gentity_t* missile = CreateMissile(start, forward_vec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

	missile->classname = "bryar_proj";

	if (ent->s.weapon == WP_BRYAR_PISTOL
		|| ent->s.weapon == WP_JAWA)
	{
		//*SIGH*... I hate our weapon system...
		missile->s.weapon = ent->s.weapon;
	}
	else
	{
		missile->s.weapon = WP_BLASTER_PISTOL;
	}

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
		missile->methodOfDeath = MOD_BRYAR_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BRYAR;
	}

	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if (ent->weaponModel[1] > 0)
	{
		//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
		ent->count = ent->count ? 0 : 1;
	}
}

//---------------------------------------------------------
void WP_FireBryarPistolDuals(gentity_t* ent, const qboolean alt_fire, const qboolean second_pistol)
//---------------------------------------------------------
{
	vec3_t start;
	int damage = !alt_fire ? weaponData[WP_BLASTER_PISTOL].damage : weaponData[WP_BLASTER_PISTOL].altDamage;

	if (second_pistol)
	{
		VectorCopy(muzzle2, start);
	}
	else
	{
		VectorCopy(muzzle, start);
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (NPC_IsNotHavingEnoughForceSight(ent))
	{//force sight 2+ gives perfect aim
		vec3_t angs;

		vectoangles(forward_vec, angs);

		if (alt_fire)
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{// add some slop to the alt-fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{// add some slop to the fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
		}

		AngleVectors(angs, forward_vec, nullptr, nullptr);
	}

	WP_MissileTargetHint(ent, start, forward_vec);

	gentity_t* missile = CreateMissile(start, forward_vec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

	missile->classname = "bryar_proj";

	if (ent->s.weapon == WP_BRYAR_PISTOL
		|| ent->s.weapon == WP_JAWA)
	{
		//*SIGH*... I hate our weapon system...
		missile->s.weapon = ent->s.weapon;
	}
	else
	{
		missile->s.weapon = WP_BLASTER_PISTOL;
	}

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
		missile->methodOfDeath = MOD_BRYAR_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BRYAR;
	}

	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if (ent->weaponModel[1] > 0)
	{
		//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
		ent->count = ent->count ? 0 : 1;
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
	vec3_t dir, angs;

	vectoangles(forward_vec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (NPC_IsNotHavingEnoughForceSight(ent))
	{//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
					}
				}
			}
			else
			{// add some slop to the alt-fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{// add some slop to the fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
		}
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	WP_FireBryarsbdMissile(ent, muzzle, dir, alt_fire);
}

//---------------------------------------------------------
void WP_FireJawaPistol(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t start;
	int damage = !alt_fire ? weaponData[WP_JAWA].damage : weaponData[WP_JAWA].altDamage;

	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (NPC_IsNotHavingEnoughForceSight(ent))
	{//force sight 2+ gives perfect aim
		vec3_t angs;

		vectoangles(forward_vec, angs);

		if (alt_fire)
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{// add some slop to the alt-fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{// add some slop to the fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
		}

		AngleVectors(angs, forward_vec, nullptr, nullptr);
	}

	WP_MissileTargetHint(ent, start, forward_vec);

	gentity_t* missile = CreateMissile(start, forward_vec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

	missile->classname = "bryar_proj";
	if (ent->s.weapon == WP_BLASTER_PISTOL
		|| ent->s.weapon == WP_SBD_PISTOL
		|| ent->s.weapon == WP_BRYAR_PISTOL)
	{
		//*SIGH*... I hate our weapon system...
		missile->s.weapon = ent->s.weapon;
	}
	else
	{
		missile->s.weapon = WP_JAWA;
	}

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
		missile->methodOfDeath = MOD_BRYAR_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BRYAR_ALT;
	}

	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if (ent->weaponModel[1] > 0)
	{
		//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
		ent->count = ent->count ? 0 : 1;
	}
}

//---------------------------------------------------------
void WP_FireBryarPistolold(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t start;
	int damage = !alt_fire ? weaponData[WP_BRYAR_PISTOL].damage : weaponData[WP_BRYAR_PISTOL].altDamage;

	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (NPC_IsNotHavingEnoughForceSight(ent))
	{//force sight 2+ gives perfect aim
		vec3_t angs;

		vectoangles(forward_vec, angs);

		if (alt_fire)
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{// add some slop to the alt-fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{// add some slop to the fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
		}

		AngleVectors(angs, forward_vec, nullptr, nullptr);
	}

	WP_MissileTargetHint(ent, start, forward_vec);

	gentity_t* missile = CreateMissile(start, forward_vec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

	missile->classname = "bryar_proj";
	if (ent->s.weapon == WP_BLASTER_PISTOL
		|| ent->s.weapon == WP_JAWA)
	{
		//*SIGH*... I hate our weapon system...
		missile->s.weapon = ent->s.weapon;
	}
	else
	{
		missile->s.weapon = WP_BRYAR_PISTOL;
	}

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

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if (ent->weaponModel[1] > 0)
	{
		//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
		ent->count = ent->count ? 0 : 1;
	}
}