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
///								    https://www.moddb.com/mods/serenityjediengine-20											///
///																																///
/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///

// g_weapon.c
// perform the server side effects of a weapon firing

#include "g_local.h"
#include "botlib/be_aas.h"
#include "qcommon/q_shared.h"

static vec3_t forward, vright, up;
static vec3_t muzzle;
static vec3_t muzzle2;

extern qboolean PM_InKnockDown(const playerState_t* ps);
qboolean PM_RunningAnim(int anim);
qboolean PM_WalkingAnim(int anim);
float Distance2(const vec3_t p1, const vec3_t p2);
extern qboolean walk_check(const gentity_t* self);
extern qboolean PM_CrouchAnim(int anim);
const int FROZEN_TIME = 5000;

extern qboolean WP_DoingForcedAnimationForForcePowers(const gentity_t* self);
extern int wp_saber_must_bolt_block(gentity_t* self, const gentity_t* atk, qboolean check_b_box_block, vec3_t point,
	int rSaberNum, int rBladeNum);
extern int wp_player_must_dodge(const gentity_t* self, const gentity_t* shooter);
extern qboolean WP_SaberBlockBolt(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
extern void g_missile_reflect_effect(const gentity_t* ent, vec3_t dir);
extern void WP_ForcePowerDrain(playerState_t* ps, forcePowers_t force_power, int override_amt);
extern void Sphereshield_Off(gentity_t* self);

#define RUNNING_SPREAD			1.7f
#define WALKING_SPREAD			1.4f

// SDB Pistol
//--------
//spread for the sbd pistol.
#define SBD_PISTOL_DAMAGE			10

// Bryar Pistol
//--------
#define BRYAR_PISTOL_VEL			2000
#define BRYAR_PISTOL_DAMAGE			15
#define BRYAR_CHARGE_UNIT			200.0f	// bryar charging gives us one more unit every 200ms--if you change this, you'll have to do the same in bg_pmove

#define BRYAR_ALT_SIZE				1.0f

// E11 Blaster
//---------
#define BLASTER_MAIN_SPREAD			0.5f
#define BLASTER_ALT_SPREAD			1.0f
#define BLASTER_NPC_SPREAD			0.25f
#define BLASTER_VELOCITY			2300
#define BLASTER_NPC_VEL_CUT			0.5f
#define BLASTER_NPC_HARD_VEL_CUT	0.7f
#define BLASTER_DAMAGE				25
#define	BLASTER_NPC_DAMAGE_EASY		8
#define	BLASTER_NPC_DAMAGE_NORMAL	20
#define	BLASTER_NPC_DAMAGE_HARD		35
#define	SJE_BLASTER_NPC_DAMAGE_HARD		50

// Tenloss Disruptor
//----------
#define DISRUPTOR_MAIN_DAMAGE			30 //40
#define DISRUPTOR_MAIN_DAMAGE_SIEGE		50
#define DISRUPTOR_NPC_MAIN_DAMAGE_CUT	0.25f

#define DISRUPTOR_ALT_DAMAGE			900
#define DISRUPTOR_NPC_ALT_DAMAGE_CUT	0.2f
#define DISRUPTOR_ALT_TRACES			3		// can go through a max of 3 damageable(sp?) entities
#define DISRUPTOR_CHARGE_UNIT			50.0f	// distruptor charging gives us one more unit every 50ms--if you change this, you'll have to do the same in bg_pmove

#define DISRUPTOR_SHOT_SIZE				2		//disruptor shot size.  Was originally 0

// Wookiee Bowcaster
//----------
#define	BOWCASTER_DAMAGE			30
#define	BOWCASTER_VELOCITY			1600
#define BOWCASTER_SPLASH_DAMAGE		0
#define BOWCASTER_SPLASH_RADIUS		0
#define BOWCASTER_SIZE				2

#define BOWCASTER_ALT_SPREAD		0.5f
#define BOWCASTER_VEL_RANGE			0.3f
#define BOWCASTER_CHARGE_UNIT		200.0f	// bowcaster charging gives us one more unit every 200ms--if you change this, you'll have to do the same in bg_pmove

// Heavy Repeater
//----------
#define REPEATER_SPREAD				0.3f
#define REPEATER_NPC_SPREAD			0.7f
#define	REPEATER_DAMAGE				8
#define	REPEATER_VELOCITY			2000

#define REPEATER_ALT_SIZE				3	// half of bbox size
#define	REPEATER_ALT_DAMAGE				60
#define REPEATER_ALT_SPLASH_DAMAGE		60
#define REPEATER_ALT_SPLASH_RADIUS		128
#define REPEATER_ALT_SPLASH_RAD_SIEGE	80
#define	REPEATER_ALT_VELOCITY			1100

// DEMP2
//----------
#define	DEMP2_DAMAGE				35
#define	DEMP2_VELOCITY				2000
#define	DEMP2_SIZE					2		// half of bbox size

#define DEMP2_ALT_DAMAGE			8 //12		// does 12, 36, 84 at each of the 3 charge levels.
#define DEMP2_CHARGE_UNIT			700.0f	// demp2 charging gives us one more unit every 700ms--if you change this, you'll have to do the same in bg_weapons

#define DEMP2_ALT_RANGE				4096
#define DEMP2_ALT_SPLASHRADIUS		256

// Golan Arms Flechette
//---------
#define FLECHETTE_SHOTS				5
#define FLECHETTE_SPREAD			0.5f
#define FLECHETTE_DAMAGE			5
#define FLECHETTE_VEL				3100
#define FLECHETTE_SIZE				1
#define FLECHETTE_MINE_RADIUS_CHECK	256
#define FLECHETTE_ALT_DAMAGE		60
#define FLECHETTE_ALT_SPLASH_DAM	60
#define FLECHETTE_ALT_SPLASH_RAD	128

// Personal Rocket Launcher
//---------
#define	ROCKET_VELOCITY				1200
#define	ROCKET_DAMAGE				800
#define	ROCKET_SPLASH_DAMAGE		350
#define	ROCKET_SPLASH_RADIUS		256
#define ROCKET_SIZE					3
#define ROCKET_ALT_THINK_TIME		100

// Concussion Rifle
//---------
//primary
//man, this thing is too absurdly powerful. having to
//slash the values way down from sp.
#define	CONC_VELOCITY				1500
#define	CONC_DAMAGE					150
#define CONC_NPC_SPREAD				0.5f
#define	CONC_NPC_DAMAGE_EASY		40
#define	CONC_NPC_DAMAGE_NORMAL		80
#define	CONC_NPC_DAMAGE_HARD		100
#define	CONC_SPLASH_DAMAGE			50
#define	CONC_SPLASH_RADIUS			300
//alt
#define CONC_ALT_DAMAGE				225
#define CONC_ALT_NPC_DAMAGE_EASY	20
#define CONC_ALT_NPC_DAMAGE_MEDIUM	35
#define CONC_ALT_NPC_DAMAGE_HARD	50

// Stun Baton
//--------------
#define STUN_BATON_DAMAGE			20
#define STUN_BATON_ALT_DAMAGE		20
#define STUN_BATON_RANGE			8

// Melee
//--------------
#define MELEE_SWING1_DAMAGE			3
#define MELEE_SWING2_DAMAGE			5
#define MELEE_RANGE					8
#define MELEE_SWING_WOOKIE_DAMAGE	50
#define MELEE_SWING_EXTRA_DAMAGE	 15

// ATST Main Gun
//--------------
#define ATST_MAIN_VEL				4000	//
#define ATST_MAIN_DAMAGE			25		//
#define ATST_MAIN_SIZE				3		// make it easier to hit things

// ATST Side Gun
//---------------
#define ATST_SIDE_MAIN_DAMAGE				75
#define ATST_SIDE_MAIN_VELOCITY				1300
#define ATST_SIDE_MAIN_NPC_DAMAGE_EASY		30
#define ATST_SIDE_MAIN_NPC_DAMAGE_NORMAL	40
#define ATST_SIDE_MAIN_NPC_DAMAGE_HARD		50
#define ATST_SIDE_MAIN_SIZE					4
#define ATST_SIDE_MAIN_SPLASH_DAMAGE		10	// yeah, pretty small, either zero out or make it worth having?
#define ATST_SIDE_MAIN_SPLASH_RADIUS		16	// yeah, pretty small, either zero out or make it worth having?

#define ATST_SIDE_ALT_VELOCITY				1100
#define ATST_SIDE_ALT_NPC_VELOCITY			600
#define ATST_SIDE_ALT_DAMAGE				130

#define ATST_SIDE_ROCKET_NPC_DAMAGE_EASY	30
#define ATST_SIDE_ROCKET_NPC_DAMAGE_NORMAL	50
#define ATST_SIDE_ROCKET_NPC_DAMAGE_HARD	90

#define	ATST_SIDE_ALT_SPLASH_DAMAGE			130
#define	ATST_SIDE_ALT_SPLASH_RADIUS			200
#define ATST_SIDE_ALT_ROCKET_SIZE			5
#define ATST_SIDE_ALT_ROCKET_SPLASH_SCALE	0.5f	// scales splash for NPC's

extern qboolean G_BoxInBounds(vec3_t point, vec3_t mins, vec3_t maxs, vec3_t boundsMins, vec3_t boundsMaxs);
extern qboolean G_HeavyMelee(const gentity_t* attacker);
extern void Jedi_Decloak(gentity_t* self);

static void WP_FireEmplaced(gentity_t* ent, qboolean alt_fire);

void laserTrapStick(gentity_t* ent, vec3_t endpos, vec3_t normal);

static void touch_NULL(gentity_t* ent, gentity_t* other, trace_t* trace)
{
}

void laserTrapExplode(gentity_t* self);
void RocketDie(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int mod);

extern vmCvar_t g_vehAutoAimLead;

//We should really organize weapon data into tables or parse from the ext data so we have accurate info for this,
float WP_SpeedOfMissileForWeapon(int wp, qboolean alt_fire)
{
	return 500;
}

//-----------------------------------------------------------------------------
static void W_TraceSetStart(const gentity_t* ent, vec3_t start, vec3_t mins, vec3_t maxs)
//-----------------------------------------------------------------------------
{
	//make sure our start point isn't on the other side of a wall
	trace_t tr;
	vec3_t ent_mins;
	vec3_t ent_maxs;
	vec3_t eye_point;

	VectorAdd(ent->r.currentOrigin, ent->r.mins, ent_mins);
	VectorAdd(ent->r.currentOrigin, ent->r.maxs, ent_maxs);

	if (G_BoxInBounds(start, mins, maxs, ent_mins, ent_maxs))
	{
		return;
	}

	if (!ent->client)
	{
		return;
	}

	VectorCopy(ent->s.pos.trBase, eye_point);
	eye_point[2] += ent->client->ps.viewheight;

	trap->Trace(&tr, eye_point, mins, maxs, start, ent->s.number, MASK_SOLID | CONTENTS_SHOTCLIP, qfalse, 0, 0);

	if (tr.startsolid || tr.allsolid)
	{
		return;
	}

	if (tr.fraction < 1.0f)
	{
		VectorCopy(tr.endpos, start);
	}
}

/*
----------------------------------------------
	PLAYER WEAPONS
----------------------------------------------
*/

/*
======================================================================

BRYAR PISTOL

======================================================================
*/

//----------------------------------------------
static void WP_FireBryarPistol(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	int damage = BRYAR_PISTOL_DAMAGE;

	gentity_t* missile = CreateMissile(muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);
	gentity_t* missile2 = CreateMissile(muzzle2, forward, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

	if (ent->client->ps.eFlags & EF3_DUAL_WEAPONS)
	{
		missile2->classname = "bryar_proj";
		missile2->s.weapon = WP_BRYAR_PISTOL;
	}

	if (alt_fire)
	{
		int count = (level.time - ent->client->ps.weaponChargeTime) / BRYAR_CHARGE_UNIT;

		if (count < 1)
		{
			count = 1;
		}
		else if (count > BRYAR_MAX_CHARGE)
		{
			count = BRYAR_MAX_CHARGE;
		}

		damage = BRYAR_PISTOL_ALT_DPDAMAGE + (float)count / BRYAR_MAX_CHARGE * (BRYAR_PISTOL_ALT_DPMAXDAMAGE -
			BRYAR_PISTOL_ALT_DPDAMAGE);

		missile->s.generic1 = count; // The missile will then render according to the charge level.

		float box_size = BRYAR_ALT_SIZE * (count * 0.5);

		VectorSet(missile->r.maxs, box_size, box_size, box_size);
		VectorSet(missile->r.mins, -box_size, -box_size, -box_size);

		if (ent->client->ps.eFlags & EF3_DUAL_WEAPONS)
		{
			count = (level.time - ent->client->ps.weaponChargeTime) / BRYAR_CHARGE_UNIT;

			if (count < 1)
			{
				count = 1;
			}
			else if (count > BRYAR_MAX_CHARGE)
			{
				count = BRYAR_MAX_CHARGE;
			}

			damage = BRYAR_PISTOL_ALT_DPDAMAGE + (float)count / BRYAR_MAX_CHARGE * (BRYAR_PISTOL_ALT_DPMAXDAMAGE -
				BRYAR_PISTOL_ALT_DPDAMAGE);

			missile2->s.generic1 = count;

			box_size = BRYAR_ALT_SIZE * (count * 0.5);

			VectorSet(missile2->r.maxs, box_size, box_size, box_size);
			VectorSet(missile2->r.mins, -box_size, -box_size, -box_size);
		}
	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;

	if (alt_fire)
	{
		missile->methodOfDeath = MOD_BRYAR_PISTOL_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BRYAR_PISTOL;
	}
	missile->clipmask = MASK_SHOT;
	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if (ent->client->ps.eFlags & EF3_DUAL_WEAPONS)
	{
		missile2->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;

		if (alt_fire)
		{
			missile->methodOfDeath = MOD_BRYAR_PISTOL_ALT;
		}
		else
		{
			missile->methodOfDeath = MOD_BRYAR_PISTOL;
		}
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}
}

//----------------------------------------------
static void WP_FireBryarPistolold(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	int damage = SBD_PISTOL_DAMAGE;

	gentity_t* missile = CreateMissile(muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_OLD;

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

		if (count > 1)
		{
			damage *= count * 1.7;
		}
		else
		{
			damage *= count * 1.5;
		}

		missile->s.generic1 = count; // The missile will then render according to the charge level.

		const float box_size = BRYAR_ALT_SIZE * (count * 0.5);

		VectorSet(missile->r.maxs, box_size, box_size, box_size);
		VectorSet(missile->r.mins, -box_size, -box_size, -box_size);
	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	if (alt_fire)
	{
		missile->methodOfDeath = MOD_BOWCASTER;
	}
	else
	{
		missile->methodOfDeath = MOD_BRYAR_PISTOL;
	}
	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

/*
======================================================================

GENERIC

======================================================================
*/

//---------------------------------------------------------
void WP_FireTurretMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire, const int damage,
	const int velocity, const int mod,
	const gentity_t* ignore)
	//---------------------------------------------------------
{
	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, alt_fire);

	missile->classname = "generic_proj";
	missile->s.weapon = WP_TURRET;

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->methodOfDeath = mod;
	missile->clipmask = MASK_SHOT;

	if (ignore)
	{
		missile->passThroughNum = ignore->s.number + 1;
	}

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//-----------------------------------------------------------------------------
void WP_Explode(gentity_t* self)
//-----------------------------------------------------------------------------
{
	gentity_t* attacker = self;
	vec3_t forward_vec = { 0, 0, 1 };

	// stop chain reaction runaway loops
	self->takedamage = qfalse;

	self->s.loopSound = 0;

	//	VectorCopy( self->currentOrigin, self->s.pos.trBase );
	if (!self->client)
	{
		AngleVectors(self->s.angles, forward_vec, NULL, NULL);
	}

	if (self->s.owner && self->s.owner != 1023)
	{
		attacker = &g_entities[self->s.owner];
	}
	else if (self->activator)
	{
		attacker = self->activator;
	}
	else if (self->client)
	{
		attacker = self;
	}

	if (self->splashDamage > 0 && self->splashRadius > 0)
	{
		g_radius_damage(self->r.currentOrigin, attacker, self->splashDamage, self->splashRadius,
			0/*don't ignore attacker*/, self, MOD_UNKNOWN);
	}

	if (self->target)
	{
		G_UseTargets(self, attacker);
	}

	G_SetOrigin(self, self->r.currentOrigin);

	self->nextthink = level.time + 50;
	self->think = G_FreeEntity;
}

//Currently only the seeker drone uses this, but it might be useful for other things as well.

//---------------------------------------------------------
void WP_FireGenericBlasterMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire, const int damage,
	const int velocity,
	const int mod)
	//---------------------------------------------------------
{
	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, alt_fire);

	missile->classname = "generic_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->methodOfDeath = mod;
	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

/*
======================================================================

BLASTER

======================================================================
*/

//---------------------------------------------------------
void WP_FireBlasterMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire)
//---------------------------------------------------------
{
	const int velocity = BLASTER_VELOCITY;
	const int damage = BLASTER_DAMAGE;

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, alt_fire);

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_BLASTER;

	if (ent->s.eType == ET_NPC || ent->r.svFlags & SVF_BOT)
	{
		if (g_npcspskill.integer == 0)
		{
			missile->damage = BLASTER_NPC_DAMAGE_EASY;
		}
		else if (g_npcspskill.integer == 1)
		{
			missile->damage = BLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			missile->damage = BLASTER_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->methodOfDeath = MOD_BLASTER;
	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireTurboLaserMissile(gentity_t* ent, vec3_t start, vec3_t dir)
//---------------------------------------------------------
{
	const int velocity = ent->mass; //FIXME: externalize

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, qfalse);

	//use a custom shot effect
	missile->s.otherentity_num2 = ent->genericValue14;
	//use a custom impact effect
	missile->s.emplacedOwner = ent->genericValue15;

	missile->classname = "turbo_proj";
	missile->s.weapon = WP_TURRET;

	missile->damage = ent->damage; //FIXME: externalize
	missile->splashDamage = ent->splashDamage; //FIXME: externalize
	missile->splashRadius = ent->splashRadius; //FIXME: externalize
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->methodOfDeath = MOD_TARGET_LASER; //MOD_TURBLAST; //count as a heavy weap
	missile->splashMethodOfDeath = MOD_TARGET_LASER; //MOD_TURBLAST;// ?SPLASH;
	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//set veh as cgame side owner for purpose of fx overrides
	missile->s.owner = ent->s.number;

	//don't let them last forever
	missile->think = G_FreeEntity;
	missile->nextthink = level.time + 5000; //at 20000 speed, that should be more than enough
}

//---------------------------------------------------------
static void WP_FireEmplacedMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire, gentity_t* ignore)
//---------------------------------------------------------
{
	const int velocity = BLASTER_VELOCITY;
	const int damage = BLASTER_DAMAGE;

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, alt_fire);

	missile->classname = "emplaced_gun_proj";
	missile->s.weapon = WP_TURRET; //WP_EMPLACED_GUN;

	missile->activator = ignore;

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK | DAMAGE_HEAVY_WEAP_CLASS;
	missile->methodOfDeath = MOD_VEHICLE;
	missile->clipmask = MASK_SHOT;

	if (ignore)
	{
		missile->passThroughNum = ignore->s.number + 1;
	}

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireWristMissile(gentity_t* ent, vec3_t start, vec3_t dir)
//---------------------------------------------------------
{
	const int velocity = BLASTER_VELOCITY;
	const int damage = BLASTER_DAMAGE;

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, qfalse);

	missile->classname = "emplaced_gun_proj";
	missile->s.weapon = WP_BLASTER;

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK | DAMAGE_HEAVY_WEAP_CLASS;
	missile->methodOfDeath = MOD_VEHICLE;
	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
static void WP_FireWrist(gentity_t* ent)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forward, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (NPC_IsNotHavingEnoughForceSight(ent))
	{//force sight 2+ gives perfect aim
		if (!(ent->r.svFlags & SVF_BOT))
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

	AngleVectors(angs, dir, NULL, NULL);

	WP_FireWristMissile(ent, muzzle, dir);
}

//---------------------------------------------------------
void WP_FireBlaster(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forward, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (NPC_IsNotHavingEnoughForceSight(ent))
	{//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			if (!(ent->r.svFlags & SVF_BOT))
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
			if (!(ent->r.svFlags & SVF_BOT))
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

	AngleVectors(angs, dir, NULL, NULL);

	WP_FireBlasterMissile(ent, muzzle, dir, alt_fire);
}

/*
======================================================================

DISRUPTOR

======================================================================
*/
extern int WP_SaberBlockCost(gentity_t* defender, const gentity_t* attacker, vec3_t hit_loc);
//---------------------------------------------------------
static void WP_DisruptorMainFire(gentity_t* ent)
//---------------------------------------------------------
{
	int damage = DISRUPTOR_MAIN_DAMAGE;
	qboolean render_impact = qtrue;
	vec3_t start, end;
	trace_t tr;
	gentity_t* traceEnt;
	const float shot_range = 8192;

	const vec3_t shot_maxs = { DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE };
	const vec3_t shot_mins = { -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE };

	if (level.gametype == GT_SIEGE)
	{
		damage = DISRUPTOR_MAIN_DAMAGE_SIEGE;
	}

	memset(&tr, 0, sizeof tr); //to shut the compiler up

	VectorCopy(ent->client->ps.origin, start);
	start[2] += ent->client->ps.viewheight; //By eyes

	VectorMA(start, shot_range, forward, end);

	int ignore = ent->s.number;
	const int traces = 0;
	while (traces < 10)
	{
		//need to loop this in case we hit a Jedi who dodges the shot
		if (d_projectileGhoul2Collision.integer)
		{
			trap->Trace(&tr, start, shot_mins, shot_maxs, end, ignore, MASK_SHOT, qfalse,
				G2TRFLAG_DOGHOULTRACE | G2TRFLAG_GETSURFINDEX | G2TRFLAG_THICK | G2TRFLAG_HITCORPSES,
				g_g2TraceLod.integer);
		}
		else
		{
			trap->Trace(&tr, start, shot_mins, shot_maxs, end, ignore, MASK_SHOT, qfalse, 0, 0);
		}

		traceEnt = &g_entities[tr.entityNum];

		if (d_projectileGhoul2Collision.integer && traceEnt->inuse && traceEnt->client)
		{
			//g2 collision checks -rww
			if (traceEnt->inuse && traceEnt->client && traceEnt->ghoul2)
			{
				//since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				traceEnt->client->g2LastSurfaceHit = tr.surfaceFlags;
				traceEnt->client->g2LastSurfaceTime = level.time;
				traceEnt->client->g2LastSurfaceModel = G2MODEL_PLAYER;
			}

			if (traceEnt->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}

		if (traceEnt && traceEnt->client && traceEnt->client->ps.duelInProgress &&
			traceEnt->client->ps.duelIndex != ent->s.number)
		{
			ignore = tr.entityNum;
			VectorCopy(tr.endpos, start);
			continue;
		}

		if (traceEnt)
		{
			if (wp_saber_must_bolt_block(traceEnt, ent, qfalse, tr.endpos, -1, -1) && !
				WP_DoingForcedAnimationForForcePowers(traceEnt))
			{
				//players can block or dodge disruptor shots.
				g_missile_reflect_effect(traceEnt, tr.plane.normal);
				WP_ForcePowerDrain(&traceEnt->client->ps, FP_SABER_DEFENSE,
					WP_SaberBlockCost(traceEnt, ent, tr.endpos));

				//force player into a projective block move.
				if (d_combatinfo.integer || g_DebugSaberCombat.integer && !(traceEnt->r.svFlags & SVF_BOT))
				{
					Com_Printf(S_COLOR_ORANGE"should be blocking now\n");
				}
				WP_SaberBlockBolt(traceEnt, tr.endpos, qtrue);

				ignore = tr.entityNum;
				VectorCopy(tr.endpos, start);
				continue;
			}

			if (wp_player_must_dodge(traceEnt, ent) && !
				WP_DoingForcedAnimationForForcePowers(traceEnt))
			{
				//players can block or dodge disruptor shots.
				if (traceEnt->r.svFlags & SVF_BOT)
				{
					WP_ForcePowerDrain(&traceEnt->client->ps, FP_SABER_DEFENSE, FATIGUE_DODGEINGBOT);
				}
				else
				{
					WP_ForcePowerDrain(&traceEnt->client->ps, FP_SABER_DEFENSE, FATIGUE_DODGEING);
				}
				if (d_combatinfo.integer || g_DebugSaberCombat.integer && !(traceEnt->r.svFlags & SVF_BOT))
				{
					Com_Printf(S_COLOR_ORANGE"should be dodging now\n");
				}

				//force player into a projective block move.
				jedi_disruptor_dodge_evasion(traceEnt, ent, tr.endpos, -1);

				ignore = tr.entityNum;
				VectorCopy(tr.endpos, start);
				continue;
			}
		}

		if (traceEnt->flags & FL_SHIELDED)
		{
			//stopped cold
			return;
		}
		//a Jedi is not dodging this shot
		break;
	}

	if (tr.surfaceFlags & SURF_NOIMPACT)
	{
		render_impact = qfalse;
	}

	// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
	gentity_t* tent = G_TempEntity(tr.endpos, EV_DISRUPTOR_MAIN_SHOT);
	VectorCopy(muzzle, tent->s.origin2);
	tent->s.eventParm = ent->s.number;

	traceEnt = &g_entities[tr.entityNum];

	if (render_impact)
	{
		if (tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage)
		{
			if (traceEnt->client && LogAccuracyHit(traceEnt, ent))
			{
				ent->client->accuracy_hits++;
			}

			if (traceEnt && traceEnt->client && traceEnt->client->NPC_class == CLASS_GALAKMECH)
			{
				//hehe
				G_Damage(traceEnt, ent, ent, forward, tr.endpos, 3, DAMAGE_DEATH_KNOCKBACK, MOD_DISRUPTOR);
			}
			else
			{
				G_Damage(traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NORMAL, MOD_DISRUPTOR);
			}

			tent = G_TempEntity(tr.endpos, EV_DISRUPTOR_HIT);
			tent->s.eventParm = DirToByte(tr.plane.normal);
			if (traceEnt->client)
			{
				tent->s.weapon = 1;
			}
		}
		else
		{
			// Hmmm, maybe don't make any marks on things that could break
			tent = G_TempEntity(tr.endpos, EV_DISRUPTOR_SNIPER_MISS);
			tent->s.eventParm = DirToByte(tr.plane.normal);
			tent->s.weapon = 1;
		}
	}
}

static qboolean G_CanDisruptify(const gentity_t* ent)
{
	if (!ent || !ent->inuse || !ent->client || ent->s.eType != ET_NPC ||
		ent->s.NPC_class != CLASS_VEHICLE || !ent->m_pVehicle)
	{
		//not vehicle
		return qtrue;
	}

	if (ent->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
	{
		//animal is only type that can be disintegrated
		return qtrue;
	}

	//don't do it to any other veh
	return qfalse;
}

static int DetermineDisruptorCharge(const gentity_t* ent)
{
	//returns the current charge level of the disruptor.
	//WARNING: This function doesn't check ent to see if it is using a disruptor or if it is in alt-fire mode.
	int count;
	const int max_count = DISRUPTOR_MAX_CHARGE;
	if (ent->client)
	{
		count = (level.time - ent->client->ps.weaponChargeTime) / DISRUPTOR_CHARGE_UNIT;
	}
	else
	{
		count = 100 / DISRUPTOR_CHARGE_UNIT;
	}

	count *= 2;

	if (count < 1)
	{
		count = 1;
	}
	else if (count >= max_count)
	{
		count = max_count;
	}

	return count;
}

//---------------------------------------------------------
static void WP_DisruptorAltFire(gentity_t* ent)
//---------------------------------------------------------
{
	qboolean render_impact = qtrue;
	vec3_t start;
	vec3_t muzzle2;
	trace_t tr;
	int traces = DISRUPTOR_ALT_TRACES;
	qboolean full_charge = qfalse;

	const vec3_t shot_maxs = { DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE };
	const vec3_t shot_mins = { -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE };

	int damage = DISRUPTOR_ALT_DAMAGE - 30;

	VectorCopy(muzzle, muzzle2); // making a backup copy

	if (ent->client)
	{
		VectorCopy(ent->client->ps.origin, start);
		start[2] += ent->client->ps.viewheight; //By eyes
	}
	else
	{
		VectorCopy(ent->r.currentOrigin, start);
		start[2] += 24;
	}

	//moved into DetermineDisruptorCharge so we can use it for Dodge cost calcs
	const int count = DetermineDisruptorCharge(ent);

	if (count >= DISRUPTOR_MAX_CHARGE)
	{
		full_charge = qtrue;
	}

	// more powerful charges go through more things
	if (count < 10)
	{
		traces = 1;
	}
	else if (count < 20)
	{
		traces = 2;
	}

	//ent->s.generic1=count;
	ent->genericValue6 = count;

	damage += count;

	int ignore = ent->s.number;

	for (int i = 0; i < traces; i++)
	{
		const float shot_range = 8192.0f;
		vec3_t end;
		VectorMA(start, shot_range, forward, end);

		if (d_projectileGhoul2Collision.integer)
		{
			trap->Trace(&tr, start, shot_mins, shot_maxs, end, ignore, MASK_SHOT, qfalse,
				G2TRFLAG_DOGHOULTRACE | G2TRFLAG_GETSURFINDEX | G2TRFLAG_THICK | G2TRFLAG_HITCORPSES,
				g_g2TraceLod.integer);
		}
		else
		{
			trap->Trace(&tr, start, shot_mins, shot_maxs, end, ignore, MASK_SHOT, qfalse, 0, 0);
		}

		gentity_t* traceEnt = &g_entities[tr.entityNum];

		if (d_projectileGhoul2Collision.integer && traceEnt->inuse && traceEnt->client)
		{
			//g2 collision checks -rww
			if (traceEnt->inuse && traceEnt->client && traceEnt->ghoul2)
			{
				//since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				traceEnt->client->g2LastSurfaceHit = tr.surfaceFlags;
				traceEnt->client->g2LastSurfaceTime = level.time;
				traceEnt->client->g2LastSurfaceModel = G2MODEL_PLAYER;
			}

			if (traceEnt->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}

		if (tr.surfaceFlags & SURF_NOIMPACT)
		{
			render_impact = qfalse;
		}

		if (traceEnt && traceEnt->client && traceEnt->client->ps.duelInProgress &&
			traceEnt->client->ps.duelIndex != ent->s.number)
		{
			ignore = tr.entityNum;
			VectorCopy(tr.endpos, start);
			continue;
		}

		if (traceEnt)
		{
			if (wp_saber_must_bolt_block(traceEnt, ent, qfalse, tr.endpos, -1, -1) && !
				WP_DoingForcedAnimationForForcePowers(traceEnt))
			{
				//players can block or dodge disruptor shots.
				g_missile_reflect_effect(traceEnt, tr.plane.normal);
				WP_ForcePowerDrain(&traceEnt->client->ps, FP_SABER_DEFENSE,
					WP_SaberBlockCost(traceEnt, ent, tr.endpos));

				//force player into a projective block move.
				if (d_combatinfo.integer || g_DebugSaberCombat.integer && !(traceEnt->r.svFlags & SVF_BOT))
				{
					Com_Printf(S_COLOR_ORANGE"should be blocking now\n");
				}
				WP_SaberBlockBolt(traceEnt, tr.endpos, qtrue);

				ignore = tr.entityNum;
				VectorCopy(tr.endpos, start);
				continue;
			}

			if (wp_player_must_dodge(traceEnt, ent) && !
				WP_DoingForcedAnimationForForcePowers(traceEnt))
			{
				//players can block or dodge disruptor shots.
				if (traceEnt->r.svFlags & SVF_BOT)
				{
					WP_ForcePowerDrain(&traceEnt->client->ps, FP_SABER_DEFENSE, FATIGUE_DODGEINGBOT);
				}
				else
				{
					WP_ForcePowerDrain(&traceEnt->client->ps, FP_SABER_DEFENSE, FATIGUE_DODGEING);
				}
				if (d_combatinfo.integer || g_DebugSaberCombat.integer && !(traceEnt->r.svFlags & SVF_BOT))
				{
					Com_Printf(S_COLOR_ORANGE"should be dodging now\n");
				}

				//force player into a projective block move.
				jedi_disruptor_dodge_evasion(traceEnt, ent, tr.endpos, -1);

				ignore = tr.entityNum;
				VectorCopy(tr.endpos, start);
				continue;
			}
		}

		// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
		gentity_t* tent = G_TempEntity(tr.endpos, EV_DISRUPTOR_SNIPER_SHOT);
		VectorCopy(muzzle, tent->s.origin2);
		tent->s.shouldtarget = full_charge;
		tent->s.eventParm = ent->s.number;

		// If the beam hits a skybox, etc. it would look foolish to add impact effects
		if (render_impact)
		{
			if (traceEnt->takedamage && traceEnt->client)
			{
				tent->s.otherentity_num = traceEnt->s.number;

				// Create a simple impact type mark
				tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
				tent->s.eventParm = DirToByte(tr.plane.normal);
				tent->s.eFlags |= EF_ALT_FIRING;

				if (LogAccuracyHit(traceEnt, ent))
				{
					if (ent->client)
					{
						ent->client->accuracy_hits++;
					}
				}
			}
			else
			{
				if (traceEnt->r.svFlags & SVF_GLASS_BRUSH
					|| traceEnt->takedamage
					|| traceEnt->s.eType == ET_MOVER)
				{
					if (traceEnt->takedamage)
					{
						G_Damage(traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NO_KNOCKBACK,
							MOD_DISRUPTOR_SNIPER);

						tent = G_TempEntity(tr.endpos, EV_DISRUPTOR_HIT);
						tent->s.eventParm = DirToByte(tr.plane.normal);
					}
				}
				else
				{
					// Hmmm, maybe don't make any marks on things that could break
					tent = G_TempEntity(tr.endpos, EV_DISRUPTOR_SNIPER_MISS);
					tent->s.eventParm = DirToByte(tr.plane.normal);
				}
				break; // and don't try any more traces
			}

			if (traceEnt->flags & FL_SHIELDED)
			{
				//stops us cold
				break;
			}

			if (traceEnt->takedamage)
			{
				vec3_t pre_ang;
				const int pre_health = traceEnt->health;
				int pre_legs = 0;
				int pre_torso = 0;

				if (traceEnt->client)
				{
					pre_legs = traceEnt->client->ps.legsAnim;
					pre_torso = traceEnt->client->ps.torsoAnim;
					VectorCopy(traceEnt->client->ps.viewangles, pre_ang);
				}

				G_Damage(traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_DISRUPTOR_SNIPER);

				if (traceEnt->client && pre_health > 0 && traceEnt->health <= 0 && full_charge &&
					G_CanDisruptify(traceEnt))
				{
					//was killed by a fully charged sniper shot, so disintegrate
					VectorCopy(pre_ang, traceEnt->client->ps.viewangles);

					traceEnt->client->ps.eFlags |= EF_DISINTEGRATION;
					VectorCopy(tr.endpos, traceEnt->client->ps.lastHitLoc);

					traceEnt->client->ps.legsAnim = pre_legs;
					traceEnt->client->ps.torsoAnim = pre_torso;

					traceEnt->r.contents = 0;

					VectorClear(traceEnt->client->ps.velocity);
				}

				tent = G_TempEntity(tr.endpos, EV_DISRUPTOR_HIT);
				tent->s.eventParm = DirToByte(tr.plane.normal);
				if (traceEnt->client)
				{
					tent->s.weapon = 1;
				}
			}
		}
		else // not rendering impact, must be a skybox or other similar thing?
		{
			break; // don't try anymore traces
		}

		// Get ready for an attempt to trace through another person
		VectorCopy(tr.endpos, muzzle);
		VectorCopy(tr.endpos, start);
		ignore = tr.entityNum;
	}
}

//---------------------------------------------------------
static void WP_FireDisruptor(gentity_t* ent, qboolean alt_fire)
//---------------------------------------------------------
{
	if (!ent || !ent->client || ent->client->ps.zoomMode != 1)
	{
		//do not ever let it do the alt fire when not zoomed
		alt_fire = qfalse;
	}

	if (ent && ent->s.eType == ET_NPC && !ent->client)
	{
		//special case for animents
		WP_DisruptorAltFire(ent);
		return;
	}

	if (alt_fire)
	{
		WP_DisruptorAltFire(ent);
	}
	else
	{
		WP_DisruptorMainFire(ent);
	}
}

/*
======================================================================

BOWCASTER

======================================================================
*/

static void WP_BowcasterAltFire(gentity_t* ent)
{
	const int damage = BOWCASTER_DAMAGE;

	gentity_t* missile = CreateMissile(muzzle, forward, BOWCASTER_VELOCITY, 10000, ent, qfalse);

	missile->classname = "bowcaster_proj";
	missile->s.weapon = WP_BOWCASTER;

	if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_HALF)
	{
		G_SetAnim(ent, NULL, SETANIM_BOTH, BOTH_H1_S1_TR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
	}

	VectorSet(missile->r.maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE);
	VectorScale(missile->r.maxs, -1, missile->r.mins);

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->methodOfDeath = MOD_BOWCASTER;
	missile->clipmask = MASK_SHOT;
	missile->splashDamage = BOWCASTER_SPLASH_DAMAGE;
	missile->splashRadius = BOWCASTER_SPLASH_RADIUS;

	missile->flags |= FL_BOUNCE;
	missile->bounceCount = 3;
}

//---------------------------------------------------------
static void WP_BowcasterMainFire(gentity_t* ent)
//---------------------------------------------------------
{
	int damage, count;
	vec3_t angs;

	if (!ent->client)
	{
		count = 1;
	}
	else
	{
		count = (level.time - ent->client->ps.weaponChargeTime) / BOWCASTER_CHARGE_UNIT;
	}

	if (count < 1)
	{
		count = 1;
	}
	else if (count > 5)
	{
		count = 5;
	}

	if (!(count & 1))
	{
		// if we aren't odd, knock us down a level
		count--;
	}

	//scale the damage down based on how many are about to be fired
	if (count <= 1)
	{
		damage = 50;
	}
	else if (count == 2)
	{
		damage = 45;
	}
	else if (count == 3)
	{
		damage = 40;
	}
	else if (count == 4)
	{
		damage = 35;
	}
	else
	{
		damage = 30;
	}

	for (int i = 0; i < count; i++)
	{
		vec3_t dir;
		// create a range of different velocities
		const float vel = BOWCASTER_VELOCITY * (Q_flrand(-1.0f, 1.0f) * BOWCASTER_VEL_RANGE + 1.0f);

		vectoangles(forward, angs);

		if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
		{
			//no inherent aim screw up
		}
		else if (NPC_IsNotHavingEnoughForceSight(ent))
		{//force sight 2+ gives perfect aim
			if (!(ent->r.svFlags & SVF_BOT))
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

		AngleVectors(angs, dir, NULL, NULL);

		gentity_t* missile = CreateMissile(muzzle, dir, vel, 10000, ent, qtrue);

		missile->classname = "bowcaster_alt_proj";
		missile->s.weapon = WP_BOWCASTER;

		VectorSet(missile->r.maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE);
		VectorScale(missile->r.maxs, -1, missile->r.mins);

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
		missile->methodOfDeath = MOD_BOWCASTER;
		missile->clipmask = MASK_SHOT;

		// we don't want it to bounce
		missile->bounceCount = 0;
	}
}

//---------------------------------------------------------
static void WP_FireBowcaster(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	if (alt_fire)
	{
		WP_BowcasterAltFire(ent);
	}
	else
	{
		WP_BowcasterMainFire(ent);
	}
}

/*
======================================================================

REPEATER

======================================================================
*/

//---------------------------------------------------------
static void WP_RepeaterMainFire(gentity_t* ent, vec3_t dir)
//---------------------------------------------------------
{
	const int damage = REPEATER_DAMAGE;

	gentity_t* missile = CreateMissile(muzzle, dir, REPEATER_VELOCITY, 10000, ent, qfalse);

	missile->classname = "repeater_proj";
	missile->s.weapon = WP_REPEATER;

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_REPEATER;
	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
static void WP_RepeaterAltFire(gentity_t* ent)
//---------------------------------------------------------
{
	const int damage = REPEATER_ALT_DAMAGE;

	gentity_t* missile = CreateMissile(muzzle, forward, REPEATER_ALT_VELOCITY, 10000, ent, qtrue);

	missile->classname = "repeater_alt_proj";
	missile->s.weapon = WP_REPEATER;

	if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_HALF)
	{
		G_SetAnim(ent, NULL, SETANIM_BOTH, BOTH_H1_S1_TR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
	}

	VectorSet(missile->r.maxs, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE);
	VectorScale(missile->r.maxs, -1, missile->r.mins);
	missile->s.pos.trType = TR_GRAVITY;
	missile->s.pos.trDelta[2] += 40.0f; //give a slight boost in the upward direction
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->methodOfDeath = MOD_REPEATER_ALT;
	missile->splashMethodOfDeath = MOD_REPEATER_ALT_SPLASH;
	missile->clipmask = MASK_SHOT;
	missile->splashDamage = REPEATER_ALT_SPLASH_DAMAGE;
	if (level.gametype == GT_SIEGE)
		// we've been having problems with this being too hyper-potent because of it's radius
	{
		missile->splashRadius = REPEATER_ALT_SPLASH_RAD_SIEGE;
	}
	else
	{
		missile->splashRadius = REPEATER_ALT_SPLASH_RADIUS;
	}

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
static void WP_FireRepeater(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t angs;

	vectoangles(forward, angs);

	if (alt_fire)
	{
		WP_RepeaterAltFire(ent);
	}
	else
	{
		vec3_t dir;

		if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
		{
			//no inherent aim screw up
		}
		else if (NPC_IsNotHavingEnoughForceSight(ent))
		{//force sight 2+ gives perfect aim
			if (!(ent->r.svFlags & SVF_BOT))
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

		AngleVectors(angs, dir, NULL, NULL);

		WP_RepeaterMainFire(ent, dir);
	}
}

/*
======================================================================

DEMP2

======================================================================
*/

static void WP_DEMP2_MainFire(gentity_t* ent)
{
	const int damage = DEMP2_DAMAGE;

	gentity_t* missile = CreateMissile(muzzle, forward, DEMP2_VELOCITY, 10000, ent, qfalse);

	missile->classname = "demp2_proj";
	missile->s.weapon = WP_DEMP2;

	VectorSet(missile->r.maxs, DEMP2_SIZE, DEMP2_SIZE, DEMP2_SIZE);
	VectorScale(missile->r.maxs, -1, missile->r.mins);
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_DEMP2;
	missile->clipmask = MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

static gentity_t* ent_list[MAX_GENTITIES];

void DEMP2_AltRadiusDamage(gentity_t* ent)
{
	float frac = (level.time - ent->genericValue5) / 800.0f;
	int iEntityList[MAX_GENTITIES];
	gentity_t* entity_list[MAX_GENTITIES];
	gentity_t* my_owner = NULL;
	int i;
	vec3_t mins, maxs;
	vec3_t v, dir;

	if (ent->r.ownerNum >= 0 &&
		ent->r.ownerNum < MAX_GENTITIES)
	{
		my_owner = &g_entities[ent->r.ownerNum];
	}

	if (!my_owner || !my_owner->inuse || !my_owner->client)
	{
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	frac *= frac * frac;
	// yes, this is completely ridiculous...but it causes the shell to grow slowly then "explode" at the end

	float radius = frac * 200.0f;
	// 200 is max radius...the model is aprox. 100 units tall...the fx draw code mults. this by 2.

	float fact = ent->count * 0.6;

	if (fact < 1)
	{
		fact = 1;
	}

	radius *= fact;

	for (i = 0; i < 3; i++)
	{
		mins[i] = ent->r.currentOrigin[i] - radius;
		maxs[i] = ent->r.currentOrigin[i] + radius;
	}

	const int num_listed_entities = trap->EntitiesInBox(mins, maxs, iEntityList, MAX_GENTITIES);

	i = 0;
	while (i < num_listed_entities)
	{
		entity_list[i] = &g_entities[iEntityList[i]];
		i++;
	}

	for (int e = 0; e < num_listed_entities; e++)
	{
		gentity_t* gent = entity_list[e];

		if (!gent || !gent->takedamage || !gent->r.contents)
		{
			continue;
		}

		// find the distance from the edge of the bounding box
		for (i = 0; i < 3; i++)
		{
			if (ent->r.currentOrigin[i] < gent->r.absmin[i])
			{
				v[i] = gent->r.absmin[i] - ent->r.currentOrigin[i];
			}
			else if (ent->r.currentOrigin[i] > gent->r.absmax[i])
			{
				v[i] = ent->r.currentOrigin[i] - gent->r.absmax[i];
			}
			else
			{
				v[i] = 0;
			}
		}

		// shape is an ellipsoid, so cut vertical distance in half`
		v[2] *= 0.5f;

		const float dist = VectorLength(v);

		if (dist >= radius)
		{
			// shockwave hasn't hit them yet
			continue;
		}

		if (dist + 16 * ent->count < ent->genericValue6)
		{
			// shockwave has already hit this thing...
			continue;
		}

		VectorCopy(gent->r.currentOrigin, v);
		VectorSubtract(v, ent->r.currentOrigin, dir);

		// push the center of mass higher than the origin so players get knocked into the air more
		dir[2] += 12;

		if (gent != my_owner)
		{
			G_Damage(gent, my_owner, my_owner, dir, ent->r.currentOrigin, ent->damage, DAMAGE_DEATH_KNOCKBACK,
				ent->splashMethodOfDeath);
			if (gent->takedamage
				&& gent->client)
			{
				if (gent->client->ps.electrifyTime < level.time)
				{
					//electrocution effect
					if (gent->s.eType == ET_NPC && gent->s.NPC_class == CLASS_VEHICLE &&
						gent->m_pVehicle && (gent->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER || gent->m_pVehicle->
							m_pVehicleInfo->type == VH_WALKER))
					{
						//do some extra stuff to speeders/walkers
						gent->client->ps.electrifyTime = level.time + Q_irand(3000, 4000);
					}
					else if (gent->s.NPC_class != CLASS_VEHICLE
						|| gent->m_pVehicle && gent->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER)
					{
						//don't do this to fighters
						gent->client->ps.electrifyTime = level.time + Q_irand(300, 800);
					}
				}
				if (gent->client->ps.powerups[PW_CLOAKED])
				{
					//disable cloak temporarily
					Jedi_Decloak(gent);
					gent->client->cloakToggleTime = level.time + Q_irand(3000, 10000);
				}
				if (gent->client->ps.powerups[PW_SPHERESHIELDED])
				{//disable cloak temporarily
					Sphereshield_Off(gent);
					gent->client->cloakToggleTime = level.time + Q_irand(3000, 10000);
				}
			}
		}
	}

	// store the last fraction so that next time around we can test against those things that fall between that last point and where the current shockwave edge is
	ent->genericValue6 = radius;

	if (frac < 1.0f)
	{
		// shock is still happening so continue letting it expand
		ent->nextthink = level.time + 50;
	}
	else
	{
		//don't just leave the entity around
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
	}
}

//---------------------------------------------------------
void DEMP2_AltDetonate(gentity_t* ent)
//---------------------------------------------------------
{
	G_SetOrigin(ent, ent->r.currentOrigin);
	if (!ent->pos1[0] && !ent->pos1[1] && !ent->pos1[2])
	{
		//don't play effect with a 0'd out directional vector
		ent->pos1[1] = 1;
	}
	//Let's just save ourself some bandwidth and play both the effect and sphere spawn in 1 event
	gentity_t* ef_ent = G_PlayEffect(EFFECT_EXPLOSION_DEMP2ALT, ent->r.currentOrigin, ent->pos1);

	if (ef_ent)
	{
		ef_ent->s.weapon = ent->count * 2;
	}

	ent->genericValue5 = level.time;
	ent->genericValue6 = 0;
	ent->nextthink = level.time + 50;
	ent->think = DEMP2_AltRadiusDamage;
	ent->s.eType = ET_GENERAL; // make us a missile no longer
}

//---------------------------------------------------------
static void WP_DEMP2_AltFire(gentity_t* ent)
//---------------------------------------------------------
{
	int damage = DEMP2_ALT_DAMAGE;
	vec3_t start, end;
	trace_t tr;

	VectorCopy(muzzle, start);

	VectorMA(start, DEMP2_ALT_RANGE, forward, end);

	if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_HALF)
	{
		G_SetAnim(ent, NULL, SETANIM_BOTH, BOTH_H1_S1_TR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
	}

	int count = (level.time - ent->client->ps.weaponChargeTime) / DEMP2_CHARGE_UNIT;

	const int origcount = count;

	if (count < 1)
	{
		count = 1;
	}
	else if (count > 3)
	{
		count = 3;
	}

	float fact = count * 0.8;
	if (fact < 1)
	{
		fact = 1;
	}
	damage *= fact;

	if (!origcount)
	{
		//this was just a tap-fire
		damage = 1;
	}

	trap->Trace(&tr, start, NULL, NULL, end, ent->s.number, MASK_SHOT, qfalse, 0, 0);

	gentity_t* missile = G_Spawn();
	G_SetOrigin(missile, tr.endpos);
	//In SP the impact actually travels as a missile based on the trace fraction, but we're
	//just going to be instant. -rww

	VectorCopy(tr.plane.normal, missile->pos1);

	missile->count = count;

	missile->classname = "demp2_alt_proj";
	missile->s.weapon = WP_DEMP2;

	missile->think = DEMP2_AltDetonate;
	missile->nextthink = level.time;

	missile->splashDamage = missile->damage = damage;
	missile->splashMethodOfDeath = missile->methodOfDeath = MOD_DEMP2;
	missile->splashRadius = DEMP2_ALT_SPLASHRADIUS;

	missile->r.ownerNum = ent->s.number;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->clipmask = MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

//---------------------------------------------------------
static void WP_FireDEMP2(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	if (alt_fire)
	{
		WP_DEMP2_AltFire(ent);
	}
	else
	{
		WP_DEMP2_MainFire(ent);
	}
}

/*
======================================================================

FLECHETTE

======================================================================
*/

//---------------------------------------------------------
static void WP_FlechetteMainFire(gentity_t* ent)
//---------------------------------------------------------
{
	vec3_t angs;

	for (int i = 0; i < FLECHETTE_SHOTS; i++)
	{
		vec3_t fwd;
		vectoangles(forward, angs);

		// add some slop to the alt-fire direction
		if (i != 0)
		{
			//do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += Q_flrand(-1.0f, 1.0f) * FLECHETTE_SPREAD;
			angs[YAW] += Q_flrand(-1.0f, 1.0f) * FLECHETTE_SPREAD;
		}
		else
		{
			if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
			{
				//no inherent aim screw up
			}
			else if (NPC_IsNotHavingEnoughForceSight(ent))
			{//force sight 2+ gives perfect aim
				if (!(ent->r.svFlags & SVF_BOT))
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

		AngleVectors(angs, fwd, NULL, NULL);

		gentity_t* missile = CreateMissile(muzzle, fwd, FLECHETTE_VEL, 10000, ent, qfalse);

		missile->classname = "flech_proj";
		missile->s.weapon = WP_FLECHETTE;

		VectorSet(missile->r.maxs, FLECHETTE_SIZE, FLECHETTE_SIZE, FLECHETTE_SIZE);
		VectorScale(missile->r.maxs, -1, missile->r.mins);

		missile->damage = FLECHETTE_DAMAGE;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
		missile->methodOfDeath = MOD_FLECHETTE;
		missile->clipmask = MASK_SHOT;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5, 8);

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}

//---------------------------------------------------------
void prox_mine_think(gentity_t* ent)
//---------------------------------------------------------
{
	qboolean blow = qfalse;

	// if it isn't time to auto-explode, do a small proximity check
	if (ent->delay > level.time)
	{
		const int count = G_RadiusList(ent->r.currentOrigin, FLECHETTE_MINE_RADIUS_CHECK, ent, qtrue, ent_list);

		for (int i = 0; i < count; i++)
		{
			if (ent_list[i]->client && ent_list[i]->health > 0 && ent->activator && ent_list[i]->s.number != ent->
				activator->s.number)
			{
				blow = qtrue;
				break;
			}
		}
	}
	else
	{
		// well, we must die now
		blow = qtrue;
	}

	if (blow)
	{
		ent->think = laserTrapExplode;
		ent->nextthink = level.time + 200;
	}
	else
	{
		// we probably don't need to do this thinking logic very often...maybe this is fast enough?
		ent->nextthink = level.time + 500;
	}
}

//-----------------------------------------------------------------------------
static void WP_TraceSetStart(const gentity_t* ent, vec3_t start, vec3_t mins, vec3_t maxs)
//-----------------------------------------------------------------------------
{
	//make sure our start point isn't on the other side of a wall
	trace_t tr;
	vec3_t ent_mins;
	vec3_t ent_maxs;

	VectorAdd(ent->r.currentOrigin, ent->r.mins, ent_mins);
	VectorAdd(ent->r.currentOrigin, ent->r.maxs, ent_maxs);

	if (G_BoxInBounds(start, mins, maxs, ent_mins, ent_maxs))
	{
		return;
	}

	if (!ent->client)
	{
		return;
	}

	trap->Trace(&tr, ent->client->ps.origin, mins, maxs, start, ent->s.number, MASK_SOLID | CONTENTS_SHOTCLIP, qfalse,
		0, 0);

	if (tr.startsolid || tr.allsolid)
	{
		return;
	}

	if (tr.fraction < 1.0f)
	{
		VectorCopy(tr.endpos, start);
	}
}

static void WP_ExplosiveDie(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int mod)
{
	laserTrapExplode(self);
}

//----------------------------------------------
void wp_flechette_alt_blow(gentity_t* ent)
//----------------------------------------------
{
	ent->s.pos.trDelta[0] = 1;
	ent->s.pos.trDelta[1] = 0;
	ent->s.pos.trDelta[2] = 0;

	laserTrapExplode(ent);
}

//------------------------------------------------------------------------------
static void WP_CreateFlechetteBouncyThing(vec3_t start, vec3_t fwd, gentity_t* self)
//------------------------------------------------------------------------------
{
	gentity_t* missile = CreateMissile(start, fwd, 700 + Q_flrand(0.0f, 1.0f) * 700,
		1500 + Q_flrand(0.0f, 1.0f) * 2000,
		self, qtrue);

	missile->think = wp_flechette_alt_blow;

	missile->activator = self;

	missile->s.weapon = WP_FLECHETTE;
	missile->classname = "flech_alt";
	missile->mass = 4;

	// How 'bout we give this thing a size...
	VectorSet(missile->r.mins, -3.0f, -3.0f, -3.0f);
	VectorSet(missile->r.maxs, 3.0f, 3.0f, 3.0f);
	missile->clipmask = MASK_SHOT;

	missile->touch = touch_NULL;

	// normal ones bounce, alt ones explode on impact
	missile->s.pos.trType = TR_GRAVITY;

	missile->flags |= FL_BOUNCE_HALF;
	missile->s.eFlags |= EF_ALT_FIRING;

	missile->bounceCount = 50;

	missile->damage = FLECHETTE_ALT_DAMAGE;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->splashDamage = FLECHETTE_ALT_SPLASH_DAM;
	missile->splashRadius = FLECHETTE_ALT_SPLASH_RAD;

	missile->r.svFlags = SVF_USE_CURRENT_ORIGIN;

	missile->methodOfDeath = MOD_FLECHETTE_ALT_SPLASH;
	missile->splashMethodOfDeath = MOD_FLECHETTE_ALT_SPLASH;

	VectorCopy(start, missile->pos2);
}

//---------------------------------------------------------
static void WP_FlechetteAltFire(gentity_t* ent)
//---------------------------------------------------------
{
	vec3_t dir, start, angs;

	vectoangles(forward, angs);
	VectorCopy(muzzle, start);

	WP_TraceSetStart(ent, start, vec3_origin, vec3_origin);
	//make sure our start point isn't on the other side of a wall

	if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_HALF)
	{
		G_SetAnim(ent, NULL, SETANIM_BOTH, BOTH_H1_S1_TR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
	}

	for (int i = 0; i < 2; i++)
	{
		vec3_t fwd;
		VectorCopy(angs, dir);

		dir[PITCH] -= Q_flrand(0.0f, 1.0f) * 4 + 8; // make it fly upwards
		dir[YAW] += Q_flrand(-1.0f, 1.0f) * 2;
		AngleVectors(dir, fwd, NULL, NULL);

		WP_CreateFlechetteBouncyThing(start, fwd, ent);
	}
}

//---------------------------------------------------------
static void WP_FireFlechette(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	if (alt_fire)
	{
		WP_FlechetteAltFire(ent);
	}
	else
	{
		WP_FlechetteMainFire(ent);
	}
}

/*
======================================================================

ROCKET LAUNCHER

======================================================================
*/

//---------------------------------------------------------
void rocketThink(gentity_t* ent)
//---------------------------------------------------------
{
	const vec3_t rup = { 0, 0, 1 };
	float vel = ent->spawnflags & 1 ? ent->speed : ROCKET_VELOCITY;

	if (ent->genericValue1 && ent->genericValue1 < level.time)
	{
		//time's up, we're done, remove us
		if (ent->genericValue2)
		{
			//explode when die
			RocketDie(ent, &g_entities[ent->r.ownerNum], &g_entities[ent->r.ownerNum], 0, MOD_UNKNOWN);
		}
		else
		{
			//just remove when die
			G_FreeEntity(ent);
		}
		return;
	}
	if (!ent->enemy
		|| !ent->enemy->client
		|| ent->enemy->health <= 0
		|| ent->enemy->client->ps.powerups[PW_CLOAKED])
	{
		//no enemy or enemy not a client or enemy dead or enemy cloaked
		if (!ent->genericValue1)
		{
			//doesn't have its own self-kill time
			ent->nextthink = level.time + 10000;
			ent->think = G_FreeEntity;
		}
		return;
	}

	if (ent->spawnflags & 1)
	{
		//vehicle rocket
		if (ent->enemy->client && ent->enemy->client->NPC_class == CLASS_VEHICLE)
		{
			//tracking another vehicle
			if (ent->enemy->client->ps.speed + 4000 > vel)
			{
				vel = ent->enemy->client->ps.speed + 4000;
			}
		}
	}

	if (ent->enemy && ent->enemy->inuse)
	{
		vec3_t org;
		vec3_t targetdir;
		vec3_t newdir;
		const float new_dir_mult = ent->angle ? ent->angle * 2.0f : 1.0f;
		const float old_dir_mult = ent->angle ? (1.0f - ent->angle) * 2.0f : 1.0f;

		VectorCopy(ent->enemy->r.currentOrigin, org);
		org[2] += (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5f;

		VectorSubtract(org, ent->r.currentOrigin, targetdir);
		VectorNormalize(targetdir);

		// Now the rocket can't do a 180 in space, so we'll limit the turn to about 45 degrees.
		const float dot = DotProduct(targetdir, ent->movedir);
		if (ent->spawnflags & 1)
		{
			//vehicle rocket
			if (ent->radius > -1.0f)
			{
				//can lose the lock if DotProduct drops below this number
				if (dot < ent->radius)
				{
					return;
				}
			}
		}

		// a dot of 1.0 means right-on-target.
		if (dot < 0.0f)
		{
			vec3_t right;
			// Go in the direction opposite, start a 180.
			CrossProduct(ent->movedir, rup, right);
			const float dot2 = DotProduct(targetdir, right);

			if (dot2 > 0)
			{
				// Turn 45 degrees right.
				VectorMA(ent->movedir, 0.4f * new_dir_mult, right, newdir);
			}
			else
			{
				// Turn 45 degrees left.
				VectorMA(ent->movedir, -0.4f * new_dir_mult, right, newdir);
			}

			// Yeah we've adjusted horizontally, but let's split the difference vertically, so we kinda try to move towards it.
			newdir[2] = (targetdir[2] * new_dir_mult + ent->movedir[2] * old_dir_mult) * 0.5;

			// let's also slow down a lot
			vel *= 0.5f;
		}
		else if (dot < 0.70f)
		{
			// Still a bit off, so we turn a bit softer
			VectorMA(ent->movedir, 0.5f * new_dir_mult, targetdir, newdir);
		}
		else
		{
			// getting close, so turn a bit harder
			VectorMA(ent->movedir, 0.9f * new_dir_mult, targetdir, newdir);
		}

		// add crazy drunkenness
		for (int i = 0; i < 3; i++)
		{
			newdir[i] += Q_flrand(-1.0f, 1.0f) * ent->random * 0.25f;
		}

		// decay the randomness
		ent->random *= 0.9f;

		if (ent->enemy->client
			&& ent->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			//tracking a client who's on the ground, aim at the floor...?
			// Try to crash into the ground if we get close enough to do splash damage
			const float dis = Distance(ent->r.currentOrigin, org);

			if (dis < 128)
			{
				// the closer we get, the more we push the rocket down, heh heh.
				newdir[2] -= (1.0f - dis / 128.0f) * 0.6f;
			}
		}

		VectorNormalize(newdir);

		VectorScale(newdir, vel * 0.5f, ent->s.pos.trDelta);
		VectorCopy(newdir, ent->movedir);
		SnapVector(ent->s.pos.trDelta); // save net bandwidth
		VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
		ent->s.pos.trTime = level.time;
	}

	ent->nextthink = level.time + ROCKET_ALT_THINK_TIME; // Nothing at all spectacular happened, continue.
}

extern void g_explode_missile(gentity_t* ent);

void RocketDie(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int mod)
{
	self->die = 0;
	self->r.contents = 0;

	g_explode_missile(self);

	self->think = G_FreeEntity;
	self->nextthink = level.time;
}

//---------------------------------------------------------
static void WP_FireRocket(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	const int damage = ROCKET_DAMAGE;
	int vel = ROCKET_VELOCITY;

	if (alt_fire)
	{
		vel *= 0.5f;

		//Shove us backwards for half a second
		VectorMA(ent->client->ps.velocity, -200, forward, ent->client->ps.velocity);
		ent->client->ps.groundEntityNum = ENTITYNUM_NONE;

		if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_HALF)
		{
			G_SetAnim(ent, NULL, SETANIM_BOTH, BOTH_H1_S1_TR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		}
	}

	gentity_t* missile = CreateMissile(muzzle, forward, vel, 30000, ent, alt_fire);

	if (ent->client && ent->client->ps.rocketLockIndex != ENTITYNUM_NONE)
	{
		const float lock_time_interval = (level.gametype == GT_SIEGE ? 2400.0f : 1200.0f) / 16.0f;
		float r_time = ent->client->ps.rocketLockTime;

		if (r_time == -1)
		{
			r_time = ent->client->ps.rocketLastValidTime;
		}
		int dif = (level.time - r_time) / lock_time_interval;

		if (dif < 0)
		{
			dif = 0;
		}

		//It's 10 even though it locks client-side at 8, because we want them to have a sturdy lock first, and because there's a slight difference in time between server and client
		if (dif >= 10 && r_time != -1)
		{
			missile->enemy = &g_entities[ent->client->ps.rocketLockIndex];

			if (missile->enemy && missile->enemy->client && missile->enemy->health > 0 && !OnSameTeam(
				ent, missile->enemy))
			{
				//if enemy became invalid, died, or is on the same team, then don't seek it
				missile->angle = 0.5f;
				missile->think = rocketThink;
				missile->nextthink = level.time + ROCKET_ALT_THINK_TIME;
			}
		}

		ent->client->ps.rocketLockIndex = ENTITYNUM_NONE;
		ent->client->ps.rocketLockTime = 0;
		ent->client->ps.rocketTargetTime = 0;
	}

	missile->classname = "rocket_proj";
	missile->s.weapon = WP_ROCKET_LAUNCHER;

	// Make it easier to hit things
	VectorSet(missile->r.maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE);
	VectorScale(missile->r.maxs, -1, missile->r.mins);

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	if (alt_fire)
	{
		missile->methodOfDeath = MOD_ROCKET_HOMING;
		missile->splashMethodOfDeath = MOD_ROCKET_HOMING_SPLASH;
	}
	else
	{
		missile->methodOfDeath = MOD_ROCKET;
		missile->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	}
	//===testing being able to shoot rockets out of the air==================================
	missile->health = 10;
	missile->takedamage = qtrue;
	missile->r.contents = MASK_SHOT;
	missile->die = RocketDie;
	//===testing being able to shoot rockets out of the air==================================

	missile->clipmask = MASK_SHOT;
	missile->splashDamage = ROCKET_SPLASH_DAMAGE;
	missile->splashRadius = ROCKET_SPLASH_RADIUS;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

/*
======================================================================

THERMAL DETONATOR

======================================================================
*/

#define TD_DAMAGE			500
#define TD_SPLASH_RAD		300
#define TD_SPLASH_DAM		250
//#define TD_VELOCITY			900
#define TD_MIN_CHARGE		0.15f
#define TD_TIME				2000

#define TD_ALT_DAMAGE		500
#define TD_ALT_SPLASH_RAD	256
#define TD_ALT_SPLASH_DAM	250
#define TD_ALT_VELOCITY		600
#define TD_ALT_MIN_CHARGE	0.15f
#define TD_ALT_TIME			3000

#define GRENADE_SPLASH_RAD	500

void thermalThinkStandard(gentity_t* ent);

//---------------------------------------------------------
void thermalDetonatorExplode(gentity_t* ent)
//---------------------------------------------------------
{
	if (!ent->count)
	{
		G_Sound(ent, CHAN_WEAPON, G_SoundIndex("sound/weapons/thermal/warning.wav"));
		ent->count = 1;
		ent->genericValue5 = level.time + 500;

		ent->think = thermalThinkStandard;

		ent->nextthink = level.time;
		ent->r.svFlags |= SVF_BROADCAST; //so everyone hears/sees the explosion?
	}
	else
	{
		vec3_t origin;
		vec3_t dir = { 0, 0, 1 };

		BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);
		origin[2] += 8;
		SnapVector(origin);
		G_SetOrigin(ent, origin);

		ent->s.eType = ET_GENERAL;
		G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(dir));
		ent->freeAfterEvent = qtrue;

		if (g_radius_damage(ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent, ent,
			ent->splashMethodOfDeath))
		{
			g_entities[ent->r.ownerNum].client->accuracy_hits++;
		}

		trap->LinkEntity((sharedEntity_t*)ent);
	}
}

static void WP_GrenadeBlow(gentity_t* self)
{
	if (Q_stricmp(self->classname, "cryoban_grenade") == 0
		|| Q_stricmp(self->classname, "flash_grenade") == 0)
	{
		vec3_t dir = { 0, 0, 1 };
		int entitys[1024];
		vec3_t mins, maxs, v;
		int i;
		for (i = 0; i < 3; i++)
		{
			mins[i] = self->r.currentOrigin[i] - GRENADE_SPLASH_RAD / 2;
			maxs[i] = self->r.currentOrigin[i] + GRENADE_SPLASH_RAD / 2;
		}
		const int num = trap->EntitiesInBox(mins, maxs, entitys, MAX_GENTITIES);
		for (i = 0; i < num; i++)
		{
			gentity_t* ent = &g_entities[entitys[i]];
			if (ent == self)
				continue;
			if (!ent->takedamage)
				continue;
			if (!ent->inuse || !ent->client)
				continue;

			// find the distance from the edge of the bounding box
			for (int e = 0; e < 3; e++)
			{
				if (self->r.currentOrigin[e] < ent->r.absmin[e])
				{
					v[e] = ent->r.absmin[e] - self->r.currentOrigin[e];
				}
				else if (self->r.currentOrigin[e] > ent->r.absmax[e])
				{
					v[e] = self->r.currentOrigin[e] - ent->r.absmax[e];
				}
				else
				{
					v[e] = 0;
				}
			}

			const int dist = VectorLength(v);

			if (dist >= GRENADE_SPLASH_RAD)
			{
				continue;
			}

			if (Q_stricmp(self->classname, "cryoban_grenade") == 0)
			{
				G_AddEvent(ent, EV_CRYOBAN, DirToByte(dir));
			}
			if (Q_stricmp(self->classname, "cryoban_grenade") == 0)
			{
				ent->client->frozenTime = level.time + FROZEN_TIME;
				ent->client->ps.userInt3 |= 1 << FLAG_FROZEN;
				ent->client->ps.userInt1 |= LOCK_UP;
				ent->client->ps.userInt1 |= LOCK_DOWN;
				ent->client->ps.userInt1 |= LOCK_RIGHT;
				ent->client->ps.userInt1 |= LOCK_LEFT;
				ent->client->viewLockTime = level.time + FROZEN_TIME;
				ent->client->ps.legsTimer = ent->client->ps.torsoTimer = level.time + FROZEN_TIME;
				G_SetAnim(ent, NULL, SETANIM_BOTH, WeaponReadyAnim[ent->client->ps.weapon],
					SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 300);
			}
			else if (Q_stricmp(self->classname, "flash_grenade") == 0)
			{
				G_AddEvent(ent, EV_FLASHGRENADE, DirToByte(dir));
			}
		}
		self->s.eType = ET_GENERAL;

		if (Q_stricmp(self->classname, "flash_grenade") == 0)
		{
			G_AddEvent(self, EV_FLASHGRENADE, DirToByte(dir));
		}
		else if (Q_stricmp(self->classname, "cryoban_grenade") == 0)
		{
			G_AddEvent(self, EV_CRYOBAN_EXPLODE, DirToByte(dir));
		}
		self->freeAfterEvent = qtrue;
	}
	else if (Q_stricmp(self->classname, "smoke_grenade") == 0)
	{
		vec3_t dir = { 0, 0, 1 };
		G_AddEvent(self, EV_SMOKEGRENADE, DirToByte(dir));
		self->freeAfterEvent = qtrue;
	}
	else
	{
		self->think = G_FreeEntity;
		self->nextthink = level.time;
		if (self->client->ps.userInt3 |= 1 << FLAG_FROZEN)
		{
			self->client->ps.userInt3 &= ~(1 << FLAG_FROZEN);
		}
	}
}

void thermalThinkStandard(gentity_t* ent)
{
	if (ent->genericValue5 < level.time)
	{
		ent->think = thermalDetonatorExplode;
		ent->nextthink = level.time;
		return;
	}
	G_RunObject(ent);
	ent->nextthink = level.time;
}

static void WP_GrenadeThink(gentity_t* ent)
{
	if (ent->genericValue5 < level.time)
	{
		ent->think = WP_GrenadeBlow;
		ent->nextthink = level.time;
		return;
	}
	G_RunObject(ent);
	ent->nextthink = level.time;
}

//---------------------------------------------------------
gentity_t* WP_FireThermalDetonator(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, start;
	float chargeAmount = 1.0f; // default of full charge

	VectorCopy(forward, dir);
	VectorCopy(muzzle, start);

	gentity_t* bolt = G_Spawn();

	bolt->physicsObject = qtrue;

	if (ent->client->skillLevel[SK_SMOKEGRENADE])
	{
		bolt->classname = "smoke_grenade";
		bolt->think = WP_GrenadeThink;
	}
	else if (ent->client->skillLevel[SK_FLASHGRENADE])
	{
		bolt->classname = "flash_grenade";
		bolt->think = WP_GrenadeThink;
	}
	else if (ent->client->skillLevel[SK_CRYOBAN])
	{
		bolt->classname = "cryoban_grenade";
		bolt->think = WP_GrenadeThink;
	}
	else
	{
		bolt->classname = "thermal_detonator";
		bolt->think = thermalThinkStandard;
	}

	bolt->nextthink = level.time;
	bolt->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet(bolt->r.mins, -3.0f, -3.0f, -3.0f);
	VectorSet(bolt->r.maxs, 3.0f, 3.0f, 3.0f);
	bolt->clipmask = MASK_SHOT;

	W_TraceSetStart(ent, start, bolt->r.mins, bolt->r.maxs);
	//make sure our start point isn't on the other side of a wall

	if (ent->client)
	{
		chargeAmount = level.time - ent->client->ps.weaponChargeTime;
	}

	// get charge amount
	chargeAmount = chargeAmount / (float)TD_VELOCITY;

	if (chargeAmount > 1.0f)
	{
		chargeAmount = 1.0f;
	}
	else if (chargeAmount < TD_MIN_CHARGE)
	{
		chargeAmount = TD_MIN_CHARGE;
	}

	// normal ones bounce, alt ones explode on impact
	bolt->genericValue5 = level.time + TD_TIME; // How long 'til she blows
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->parent = ent;
	bolt->r.ownerNum = ent->s.number;
	VectorScale(dir, TD_VELOCITY * chargeAmount, bolt->s.pos.trDelta);

	if (ent->health >= 0)
	{
		bolt->s.pos.trDelta[2] += 120;
	}

	if (!alt_fire)
	{
		bolt->flags |= FL_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex("sound/weapons/thermal/thermloop.wav");
	bolt->s.loopIsSoundset = qfalse;

	bolt->damage = TD_DAMAGE;
	bolt->dflags = 0;
	bolt->splashDamage = TD_SPLASH_DAM;
	bolt->splashRadius = TD_SPLASH_RAD;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;

	bolt->methodOfDeath = MOD_THERMAL;
	bolt->splashMethodOfDeath = MOD_THERMAL_SPLASH;

	bolt->s.pos.trTime = level.time; // move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	SnapVector(bolt->s.pos.trDelta); // save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	VectorCopy(start, bolt->pos2);

	bolt->bounceCount = -5;

	return bolt;
}

gentity_t* WP_DropThermal(gentity_t* ent)
{
	AngleVectors(ent->client->ps.viewangles, forward, vright, up);
	return WP_FireThermalDetonator(ent, qfalse);
}

//---------------------------------------------------------
qboolean WP_LobFire(const gentity_t* self, vec3_t start, vec3_t target, vec3_t mins, vec3_t maxs, const int clipmask,
	vec3_t velocity, const qboolean trace_path, const int ignore_ent_num, const int enemy_num,
	float min_speed, float max_speed, float ideal_speed, const qboolean must_hit)
	//---------------------------------------------------------
{
	const float speedInc = 100;
	//for the galak mech NPC
	float best_impact_dist = Q3_INFINITE; //fireSpeed,
	vec3_t shot_vel, fail_case;
	trace_t trace;
	trajectory_t tr;
	int hit_count = 0;
	const int max_hits = 7;

	if (!ideal_speed)
	{
		ideal_speed = 300;
	}
	else if (ideal_speed < speedInc)
	{
		ideal_speed = speedInc;
	}
	float shot_speed = ideal_speed;
	const int skip_num = (ideal_speed - speedInc) / speedInc;

	if (!min_speed)
	{
		min_speed = 100;
	}
	if (!max_speed)
	{
		max_speed = 900;
	}
	while (hit_count < max_hits)
	{
		vec3_t target_dir;
		VectorSubtract(target, start, target_dir);
		const float target_dist = VectorNormalize(target_dir);

		VectorScale(target_dir, shot_speed, shot_vel);
		float travel_time = target_dist / shot_speed;
		shot_vel[2] += travel_time * 0.5 * g_gravity.value;

		if (!hit_count)
		{
			//save the first (ideal) one as the failCase (fallback value)
			if (!must_hit)
			{
				//default is fine as a return value
				VectorCopy(shot_vel, fail_case);
			}
		}

		if (trace_path)
		{
			vec3_t last_pos;
			const int time_step = 500;
			//do a rough trace of the path
			qboolean blocked = qfalse;

			VectorCopy(start, tr.trBase);
			VectorCopy(shot_vel, tr.trDelta);
			tr.trType = TR_GRAVITY;
			tr.trTime = level.time;
			travel_time *= 1000.0f;
			VectorCopy(start, last_pos);

			//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
			for (int elapsed_time = time_step; elapsed_time < floor(travel_time) + time_step; elapsed_time += time_step)
			{
				vec3_t test_pos;
				if ((float)elapsed_time > travel_time)
				{
					//cap it
					elapsed_time = floor(travel_time);
				}
				BG_EvaluateTrajectory(&tr, level.time + elapsed_time, test_pos);
				trap->Trace(&trace, last_pos, mins, maxs, test_pos, ignore_ent_num, clipmask, qfalse, 0, 0);

				if (trace.allsolid || trace.startsolid)
				{
					blocked = qtrue;
					break;
				}
				if (trace.fraction < 1.0f)
				{
					//hit something
					if (trace.entityNum == enemy_num)
					{
						//hit the enemy, that's perfect!
						break;
					}
					if (trace.plane.normal[2] > 0.7 && DistanceSquared(trace.endpos, target) < 4096)
						//hit within 64 of desired location, should be okay
					{
						//close enough!
						break;
					}
					//FIXME: maybe find the extents of this brush and go above or below it on next try somehow?
					const float impact_dist = DistanceSquared(trace.endpos, target);
					if (impact_dist < best_impact_dist)
					{
						best_impact_dist = impact_dist;
						VectorCopy(shot_vel, fail_case);
					}
					blocked = qtrue;
					//see if we should store this as the failCase
					if (trace.entityNum < ENTITYNUM_WORLD)
					{
						//hit an ent
						const gentity_t* traceEnt = &g_entities[trace.entityNum];
						if (traceEnt && traceEnt->takedamage && !OnSameTeam(self, traceEnt))
						{
							//hit something breakable, so that's okay
							//we haven't found a clear shot yet so use this as the failcase
							VectorCopy(shot_vel, fail_case);
						}
					}
					break;
				}
				if (elapsed_time == floor(travel_time))
				{
					//reached end, all clear
					break;
				}
				//all clear, try next slice
				VectorCopy(test_pos, last_pos);
			}
			if (blocked)
			{
				//hit something, adjust speed (which will change arc)
				hit_count++;
				shot_speed = ideal_speed + (hit_count - skip_num) * speedInc; //from min to max (skipping ideal)
				if (hit_count >= skip_num)
				{
					//skip ideal since that was the first value we tested
					shot_speed += speedInc;
				}
			}
			else
			{
				//made it!
				break;
			}
		}
		else
		{
			//no need to check the path, go with first calc
			break;
		}
	}

	if (hit_count >= max_hits)
	{
		//NOTE: worst case scenario, use the one that impacted closest to the target (or just use the first try...?)
		VectorCopy(fail_case, velocity);
		return qfalse;
	}
	VectorCopy(shot_vel, velocity);
	return qtrue;
}

/*
======================================================================

LASER TRAP / TRIP MINE

======================================================================
*/
#define LT_DAMAGE			100
#define LT_SPLASH_RAD		256.0f
#define LT_SPLASH_DAM		105
#define LT_VELOCITY			900.0f
#define LT_SIZE				1.5f
#define LT_ALT_TIME			2000
#define	LT_ACTIVATION_DELAY	1000
#define	LT_DELAY_TIME		50

void laserTrapExplode(gentity_t* self)
{
	vec3_t v;
	self->takedamage = qfalse;

	if (self->activator)
	{
		g_radius_damage(self->r.currentOrigin, self->activator, self->splashDamage, self->splashRadius, self, self,
			MOD_TRIP_MINE_SPLASH/*MOD_LT_SPLASH*/);
	}

	if (self->s.weapon != WP_FLECHETTE)
	{
		G_AddEvent(self, EV_MISSILE_MISS, 0);
	}

	VectorCopy(self->s.pos.trDelta, v);
	//Explode outward from the surface

	if (self->s.time == -2)
	{
		v[0] = 0;
		v[1] = 0;
		v[2] = 0;
	}

	if (self->s.weapon == WP_FLECHETTE)
	{
		G_PlayEffect(EFFECT_EXPLOSION_FLECHETTE, self->r.currentOrigin, v);
	}
	else
	{
		G_PlayEffect(EFFECT_EXPLOSION_TRIPMINE, self->r.currentOrigin, v);
	}

	self->think = G_FreeEntity;
	self->nextthink = level.time;
}

static void laserTrapDelayedExplode(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int means_of_death)
{
	self->enemy = attacker;
	self->think = laserTrapExplode;
	self->nextthink = level.time + FRAMETIME;
	self->takedamage = qfalse;
	if (attacker && attacker->s.number < MAX_CLIENTS)
	{
		//less damage when shot by player
		self->splashDamage /= 3;
		self->splashRadius /= 3;
	}
}

void touchLaserTrap(gentity_t* ent, const gentity_t* other, trace_t* trace)
{
	if (other && other->s.number < ENTITYNUM_WORLD)
	{
		//just explode if we hit any entity. This way we don't have things happening like tripmines floating
		//in the air after getting stuck to a moving door
		if (ent->activator != other)
		{
			ent->touch = 0;
			ent->nextthink = level.time + FRAMETIME;
			ent->think = laserTrapExplode;
			VectorCopy(trace->plane.normal, ent->s.pos.trDelta);
		}
	}
	else
	{
		ent->touch = 0;
		if (trace->entityNum != ENTITYNUM_NONE)
		{
			ent->enemy = &g_entities[trace->entityNum];
		}
		laserTrapStick(ent, trace->endpos, trace->plane.normal);
	}
}

static void proxMineThink(gentity_t* ent)
{
	int i = 0;
	const gentity_t* owner = NULL;

	if (ent->r.ownerNum < ENTITYNUM_WORLD)
	{
		owner = &g_entities[ent->r.ownerNum];
	}

	ent->nextthink = level.time;

	if (ent->genericValue15 < level.time ||
		!owner ||
		!owner->inuse ||
		!owner->client ||
		owner->client->pers.connected != CON_CONNECTED)
	{
		//time to die!
		ent->think = laserTrapExplode;
		return;
	}

	while (i < MAX_CLIENTS)
	{
		//eh, just check for clients, don't care about anyone else...
		const gentity_t* cl = &g_entities[i];

		if (cl->inuse && cl->client && cl->client->pers.connected == CON_CONNECTED &&
			owner != cl && cl->client->sess.sessionTeam != TEAM_SPECTATOR &&
			cl->client->tempSpectate < level.time && cl->health > 0)
		{
			if (!OnSameTeam(owner, cl) || g_friendlyFire.integer)
			{
				//not on the same team, or friendly fire is enabled
				vec3_t v;

				VectorSubtract(ent->r.currentOrigin, cl->client->ps.origin, v);
				if (VectorLength(v) < ent->splashRadius / 2.0f)
				{
					ent->think = laserTrapExplode;
					return;
				}
			}
		}
		i++;
	}
}

void laserTrapThink(gentity_t* ent)
{
	vec3_t end;
	trace_t tr;

	//just relink it every think
	trap->LinkEntity((sharedEntity_t*)ent);

	//turn on the beam effect
	if (!(ent->s.eFlags & EF_FIRING))
	{
		//arm me
		G_Sound(ent, CHAN_WEAPON, G_SoundIndex("sound/weapons/laser_trap/warning.wav"));
		ent->s.eFlags |= EF_FIRING;
	}
	ent->think = laserTrapThink;
	ent->nextthink = level.time + FRAMETIME;

	// Find the main impact point
	VectorMA(ent->s.pos.trBase, 1024, ent->movedir, end);
	trap->Trace(&tr, ent->r.currentOrigin, NULL, NULL, end, ent->s.number, MASK_SHOT, qfalse, 0, 0);

	const gentity_t* traceEnt = &g_entities[tr.entityNum];

	ent->s.time = -1; //let all clients know to draw a beam from this guy

	if (traceEnt->client || tr.startsolid)
	{
		//go boom
		ent->touch = 0;
		ent->nextthink = level.time + LT_DELAY_TIME;
		ent->think = laserTrapExplode;
	}
}

void laserTrapStick(gentity_t* ent, vec3_t endpos, vec3_t normal)
{
	G_SetOrigin(ent, endpos);
	VectorCopy(normal, ent->pos1);

	VectorClear(ent->s.apos.trDelta);
	// This will orient the object to face in the direction of the normal
	VectorCopy(normal, ent->s.pos.trDelta);
	//VectorScale( normal, -1, ent->s.pos.trDelta );
	ent->s.pos.trTime = level.time;

	//This does nothing, cg_missile makes assumptions about direction of travel controlling angles
	vectoangles(normal, ent->s.apos.trBase);
	VectorClear(ent->s.apos.trDelta);
	ent->s.apos.trType = TR_STATIONARY;
	VectorCopy(ent->s.apos.trBase, ent->s.angles);
	VectorCopy(ent->s.angles, ent->r.currentAngles);

	G_Sound(ent, CHAN_WEAPON, G_SoundIndex("sound/weapons/laser_trap/stick.wav"));
	if (ent->count)
	{
		//a tripwire
		//add draw line flag
		VectorCopy(normal, ent->movedir);
		ent->think = laserTrapThink;
		ent->nextthink = level.time + LT_ACTIVATION_DELAY; //delay the activation
		ent->touch = touch_NULL;
		//make it shootable
		ent->takedamage = qtrue;
		ent->health = 5;
		ent->die = laserTrapDelayedExplode;

		//shove the box through the wall
		VectorSet(ent->r.mins, -LT_SIZE * 2, -LT_SIZE * 2, -LT_SIZE * 2);
		VectorSet(ent->r.maxs, LT_SIZE * 2, LT_SIZE * 2, LT_SIZE * 2);

		//so that the owner can blow it up with projectiles
		ent->r.svFlags |= SVF_OWNERNOTSHARED;
	}
	else
	{
		ent->touch = touchLaserTrap;
		ent->think = proxMineThink; //laserTrapExplode;
		ent->genericValue15 = level.time + 30000; //auto-explode after 30 seconds.
		ent->nextthink = level.time + LT_ALT_TIME; // How long 'til she blows

		//make it shootable
		ent->takedamage = qtrue;
		ent->health = 5;
		ent->die = laserTrapDelayedExplode;

		//shove the box through the wall
		VectorSet(ent->r.mins, -LT_SIZE * 2, -LT_SIZE * 2, -LT_SIZE * 2);
		VectorSet(ent->r.maxs, LT_SIZE * 2, LT_SIZE * 2, LT_SIZE * 2);

		//so that the owner can blow it up with projectiles
		ent->r.svFlags |= SVF_OWNERNOTSHARED;

		if (!(ent->s.eFlags & EF_FIRING))
		{
			//arm me
			G_Sound(ent, CHAN_WEAPON, G_SoundIndex("sound/weapons/laser_trap/warning.wav"));
			ent->s.eFlags |= EF_FIRING;
			ent->s.time = -1;
			ent->s.bolt2 = 1;
		}
	}
}

static void TrapThink(gentity_t* ent)
{
	//laser trap think
	ent->nextthink = level.time + 50;
	G_RunObject(ent);
}

void CreateLaserTrap(gentity_t* laser_trap, vec3_t start, gentity_t* owner)
{
	//create a laser trap entity
	laser_trap->classname = "laserTrap";
	laser_trap->flags |= FL_BOUNCE_HALF;
	laser_trap->s.eFlags |= EF_MISSILE_STICK;
	laser_trap->splashDamage = LT_SPLASH_DAM;
	laser_trap->splashRadius = LT_SPLASH_RAD;
	laser_trap->damage = LT_DAMAGE;
	laser_trap->methodOfDeath = MOD_TRIP_MINE_SPLASH;
	laser_trap->splashMethodOfDeath = MOD_TRIP_MINE_SPLASH;
	laser_trap->s.eType = ET_GENERAL;
	laser_trap->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	laser_trap->s.weapon = WP_TRIP_MINE;
	laser_trap->s.pos.trType = TR_GRAVITY;
	laser_trap->r.contents = MASK_SHOT;
	laser_trap->parent = owner;
	laser_trap->activator = owner;
	laser_trap->r.ownerNum = owner->s.number;
	VectorSet(laser_trap->r.mins, -LT_SIZE, -LT_SIZE, -LT_SIZE);
	VectorSet(laser_trap->r.maxs, LT_SIZE, LT_SIZE, LT_SIZE);
	laser_trap->clipmask = MASK_SHOT;
	laser_trap->s.solid = 2;
	laser_trap->s.modelIndex = G_model_index("models/weapons2/laser_trap/laser_trap_w.glm");
	laser_trap->s.modelGhoul2 = 1;
	laser_trap->s.g2radius = 40;

	laser_trap->s.genericenemyindex = owner->s.number + MAX_GENTITIES;

	laser_trap->health = 1;

	laser_trap->s.time = 0;

	laser_trap->s.pos.trTime = level.time; // move a bit on the very first frame
	VectorCopy(start, laser_trap->s.pos.trBase);
	SnapVector(laser_trap->s.pos.trBase); // save net bandwidth

	SnapVector(laser_trap->s.pos.trDelta); // save net bandwidth
	VectorCopy(start, laser_trap->r.currentOrigin);

	laser_trap->s.apos.trType = TR_GRAVITY;
	laser_trap->s.apos.trTime = level.time;
	laser_trap->s.apos.trBase[YAW] = rand() % 360;
	laser_trap->s.apos.trBase[PITCH] = rand() % 360;
	laser_trap->s.apos.trBase[ROLL] = rand() % 360;

	if (rand() % 10 < 5)
	{
		laser_trap->s.apos.trBase[YAW] = -laser_trap->s.apos.trBase[YAW];
	}

	VectorCopy(start, laser_trap->pos2);
	laser_trap->touch = touchLaserTrap;
	laser_trap->think = TrapThink;
	laser_trap->nextthink = level.time + 50;
}

void WP_PlaceLaserTrap(gentity_t* ent, const qboolean alt_fire)
{
	gentity_t* found = NULL;
	vec3_t dir, start;
	int trapcount = 0;
	int found_laser_traps[MAX_GENTITIES];

	found_laser_traps[0] = ENTITYNUM_NONE;

	VectorCopy(forward, dir);
	VectorCopy(muzzle, start);

	gentity_t* laserTrap = G_Spawn();

	//limit to 10 placed at any one time
	//see how many there are now
	while ((found = G_Find(found, FOFS(classname), "laserTrap")) != NULL)
	{
		if (found->parent != ent)
		{
			continue;
		}
		found_laser_traps[trapcount++] = found->s.number;
	}
	//now remove first ones we find until there are only 9 left
	found = NULL;
	const int trapcount_org = trapcount;
	int lowestTimeStamp = level.time;
	while (trapcount > 9)
	{
		int removeMe = -1;
		for (int i = 0; i < trapcount_org; i++)
		{
			if (found_laser_traps[i] == ENTITYNUM_NONE)
			{
				continue;
			}
			found = &g_entities[found_laser_traps[i]];
			if (laserTrap && found->setTime < lowestTimeStamp)
			{
				removeMe = i;
				lowestTimeStamp = found->setTime;
			}
		}
		if (removeMe != -1)
		{
			//remove it... or blow it?
			if (&g_entities[found_laser_traps[removeMe]] == NULL)
			{
				break;
			}
			G_FreeEntity(&g_entities[found_laser_traps[removeMe]]);
			found_laser_traps[removeMe] = ENTITYNUM_NONE;
			trapcount--;
		}
		else
		{
			break;
		}
	}

	//now make the new one
	CreateLaserTrap(laserTrap, start, ent);

	//set player-created-specific fields
	laserTrap->setTime = level.time; //remember when we placed it

	if (!alt_fire)
	{
		//tripwire
		laserTrap->count = 1;
	}

	//move it
	laserTrap->s.pos.trType = TR_GRAVITY;

	if (alt_fire)
	{
		VectorScale(dir, 512, laserTrap->s.pos.trDelta);
	}
	else
	{
		VectorScale(dir, 256, laserTrap->s.pos.trDelta);
	}

	trap->LinkEntity((sharedEntity_t*)laserTrap);
}

/*
======================================================================

DET PACK

======================================================================
*/
static void VectorNPos(vec3_t in, vec3_t out)
{
	if (in[0] < 0) { out[0] = -in[0]; }
	else { out[0] = in[0]; }
	if (in[1] < 0) { out[1] = -in[1]; }
	else { out[1] = in[1]; }
	if (in[2] < 0) { out[2] = -in[2]; }
	else { out[2] = in[2]; }
}

void DetPackBlow(gentity_t* self);

void charge_stick(gentity_t* self, gentity_t* other, trace_t* trace)
{
	if (other
		&& other->flags & FL_BBRUSH
		&& other->s.pos.trType == TR_STATIONARY
		&& other->s.apos.trType == TR_STATIONARY)
	{
		//a perfectly still breakable brush, let us attach directly to it!
		self->targetEnt = other; //remember them when we blow up
	}
	else if (other
		&& other->s.number < ENTITYNUM_WORLD
		&& other->s.eType == ET_MOVER
		&& trace->plane.normal[2] > 0)
	{
		//stick to it?
		self->s.groundEntityNum = other->s.number;
	}
	else if (other && other->s.number < ENTITYNUM_WORLD &&
		(other->client || !other->s.weapon))
	{
		//hit another entity that is not stickable, "bounce" off
		vec3_t v_nor, t_n;

		VectorCopy(trace->plane.normal, v_nor);
		VectorNormalize(v_nor);
		VectorNPos(self->s.pos.trDelta, t_n);
		self->s.pos.trDelta[0] += v_nor[0] * (t_n[0] * ((float)Q_irand(1, 10) * 0.1));
		self->s.pos.trDelta[1] += v_nor[1] * (t_n[1] * ((float)Q_irand(1, 10) * 0.1));
		self->s.pos.trDelta[2] += v_nor[2] * (t_n[2] * ((float)Q_irand(1, 10) * 0.1));

		vectoangles(v_nor, self->s.angles);
		vectoangles(v_nor, self->s.apos.trBase);
		self->touch = charge_stick;
		return;
	}
	else if (other && other->s.number < ENTITYNUM_WORLD)
	{
		//hit an entity that we just want to explode on (probably another projectile or something)
		vec3_t v;

		self->touch = 0;
		self->think = 0;
		self->nextthink = 0;

		self->takedamage = qfalse;

		VectorClear(self->s.apos.trDelta);
		self->s.apos.trType = TR_STATIONARY;

		g_radius_damage(self->r.currentOrigin, self->parent, self->splashDamage, self->splashRadius, self, self,
			MOD_DET_PACK_SPLASH);
		VectorCopy(trace->plane.normal, v);
		VectorCopy(v, self->pos2);
		self->count = -1;
		G_PlayEffect(EFFECT_EXPLOSION_DETPACK, self->r.currentOrigin, v);

		self->think = G_FreeEntity;
		self->nextthink = level.time;
		return;
	}

	//if we get here I guess we hit hte world so we can stick to it

	// This requires a bit of explaining.
	// When you suicide, all of the detpacks you have placed (either on a wall, or still falling in the air) will have
	//	their ent->think() set to DetPackBlow and ent->nextthink will be between 100 <-> 300
	// If your detpacks land on a surface (i.e. charge_stick gets called) within that 100<->300 ms then ent->think()
	//	will be overwritten (set to DetpackBlow) and ent->nextthink will be 30000
	// The end result is your detpacks won't explode, but will be stuck to the wall for 30 seconds without being able to
	//	detonate them (or shoot them)
	// The fix Sil came up with is to check the think() function in charge_stick, and only overwrite it if they haven't
	//	been primed to detonate

	if (self->think == G_RunObject)
	{
		self->touch = 0;
		self->think = DetPackBlow;
		self->nextthink = level.time + 30000;
	}

	VectorClear(self->s.apos.trDelta);
	self->s.apos.trType = TR_STATIONARY;

	self->s.pos.trType = TR_STATIONARY;
	VectorCopy(self->r.currentOrigin, self->s.origin);
	VectorCopy(self->r.currentOrigin, self->s.pos.trBase);
	VectorClear(self->s.pos.trDelta);

	VectorClear(self->s.apos.trDelta);

	VectorNormalize(trace->plane.normal);

	vectoangles(trace->plane.normal, self->s.angles);
	VectorCopy(self->s.angles, self->r.currentAngles);
	VectorCopy(self->s.angles, self->s.apos.trBase);

	VectorCopy(trace->plane.normal, self->pos2);
	self->count = -1;

	G_Sound(self, CHAN_WEAPON, G_SoundIndex("sound/weapons/detpack/stick.wav"));

	gentity_t* tent = G_TempEntity(self->r.currentOrigin, EV_MISSILE_MISS);
	tent->s.weapon = 0;
	tent->parent = self;
	tent->r.ownerNum = self->s.number;

	//so that the owner can blow it up with projectiles
	self->r.svFlags |= SVF_OWNERNOTSHARED;
}

void DetPackBlow(gentity_t* self)
{
	vec3_t v;

	self->pain = 0;
	self->die = 0;
	self->takedamage = qfalse;

	if (self->targetEnt)
	{
		//we were attached to something, do *direct* damage to it!
		G_Damage(self->targetEnt, self, &g_entities[self->r.ownerNum], v, self->r.currentOrigin, self->damage, 0,
			MOD_DET_PACK_SPLASH);
	}
	g_radius_damage(self->r.currentOrigin, self->parent, self->splashDamage, self->splashRadius, self, self,
		MOD_DET_PACK_SPLASH);
	v[0] = 0;
	v[1] = 0;
	v[2] = 1;

	if (self->count == -1)
	{
		VectorCopy(self->pos2, v);
	}

	G_PlayEffect(EFFECT_EXPLOSION_DETPACK, self->r.currentOrigin, v);

	self->think = G_FreeEntity;
	self->nextthink = level.time;
}

static void DetPackPain(gentity_t* self, gentity_t* attacker, int damage)
{
	self->think = DetPackBlow;
	self->nextthink = level.time + Q_irand(50, 100);
	self->takedamage = qfalse;
}

static void DetPackDie(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int mod)
{
	self->think = DetPackBlow;
	self->nextthink = level.time + Q_irand(50, 100);
	self->takedamage = qfalse;
}

void drop_charge(gentity_t* self, vec3_t start, vec3_t dir)
{
	VectorNormalize(dir);

	gentity_t* bolt = G_Spawn();
	bolt->classname = "detpack";
	bolt->nextthink = level.time + FRAMETIME;
	bolt->think = G_RunObject;
	bolt->s.eType = ET_GENERAL;
	bolt->s.g2radius = 100;
	bolt->s.modelGhoul2 = 1;
	bolt->s.modelIndex = G_model_index("models/weapons2/detpack/det_pack_proj.glm");

	bolt->parent = self;
	bolt->r.ownerNum = self->s.number;
	bolt->damage = 100;
	bolt->splashDamage = 200;
	bolt->splashRadius = 200;
	bolt->methodOfDeath = MOD_DET_PACK_SPLASH;
	bolt->splashMethodOfDeath = MOD_DET_PACK_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->s.solid = 2;
	bolt->r.contents = MASK_SHOT;
	bolt->touch = charge_stick;

	bolt->physicsObject = qtrue;

	bolt->s.genericenemyindex = self->s.number + MAX_GENTITIES;
	//rww - so client prediction knows we own this and won't hit it

	VectorSet(bolt->r.mins, -2, -2, -2);
	VectorSet(bolt->r.maxs, 2, 2, 2);

	bolt->health = 1;
	bolt->takedamage = qtrue;
	bolt->pain = DetPackPain;
	bolt->die = DetPackDie;

	bolt->s.weapon = WP_DET_PACK;

	bolt->setTime = level.time;

	G_SetOrigin(bolt, start);
	bolt->s.pos.trType = TR_GRAVITY;
	VectorCopy(start, bolt->s.pos.trBase);
	VectorScale(dir, 300, bolt->s.pos.trDelta);
	bolt->s.pos.trTime = level.time;

	bolt->s.apos.trType = TR_GRAVITY;
	bolt->s.apos.trTime = level.time;
	bolt->s.apos.trBase[YAW] = rand() % 360;
	bolt->s.apos.trBase[PITCH] = rand() % 360;
	bolt->s.apos.trBase[ROLL] = rand() % 360;

	if (rand() % 10 < 5)
	{
		bolt->s.apos.trBase[YAW] = -bolt->s.apos.trBase[YAW];
	}

	vectoangles(dir, bolt->s.angles);
	VectorCopy(bolt->s.angles, bolt->s.apos.trBase);
	VectorSet(bolt->s.apos.trDelta, 300, 0, 0);
	bolt->s.apos.trTime = level.time;

	trap->LinkEntity((sharedEntity_t*)bolt);
	AddSoundEvent(NULL, bolt->r.currentOrigin, 128, AEL_MINOR, qtrue, qfalse);
	AddSightEvent(NULL, bolt->r.currentOrigin, 128, AEL_SUSPICIOUS, 10);
}

void BlowDetpacks(const gentity_t* ent)
{
	gentity_t* found = NULL;

	if (ent->client->ps.hasDetPackPlanted)
	{
		while ((found = G_Find(found, FOFS(classname), "detpack")) != NULL)
		{
			//loop through all ents and blow the crap out of them!
			if (found->parent == ent)
			{
				VectorCopy(found->r.currentOrigin, found->s.origin);
				found->think = DetPackBlow;
				found->nextthink = level.time + 100 + Q_flrand(0.0f, 1.0f) * 200;
				G_Sound(found, CHAN_BODY, G_SoundIndex("sound/weapons/detpack/warning.wav"));
				AddSoundEvent(NULL, found->r.currentOrigin, found->splashRadius * 2, AEL_DANGER, qfalse, qtrue);
				AddSightEvent(NULL, found->r.currentOrigin, found->splashRadius * 2, AEL_DISCOVERED, 100);
			}
		}
		ent->client->ps.hasDetPackPlanted = qfalse;
	}
}

void RemoveDetpacks(const gentity_t* ent)
{
	gentity_t* found = NULL;

	if (ent->client->ps.hasDetPackPlanted)
	{
		while ((found = G_Find(found, FOFS(classname), "detpack")) != NULL)
		{
			//loop through all ents and blow the crap out of them!
			if (found->parent == ent)
			{
				VectorCopy(found->r.currentOrigin, found->s.origin);
				found->think = G_FreeEntity;
				found->nextthink = level.time;
			}
		}
		ent->client->ps.hasDetPackPlanted = qfalse;
	}
}

static qboolean CheatsOn(void)
{
	if (!sv_cheats.integer)
	{
		return qfalse;
	}
	return qtrue;
}

static void WP_DropDetPack(gentity_t* ent, const qboolean alt_fire)
{
	gentity_t* found = NULL;
	int trapcount = 0;
	int found_det_packs[MAX_GENTITIES] = { ENTITYNUM_NONE };

	if (!ent || !ent->client)
	{
		return;
	}

	//limit to 10 placed at any one time
	//see how many there are now
	while ((found = G_Find(found, FOFS(classname), "detpack")) != NULL)
	{
		if (found->parent != ent)
		{
			continue;
		}
		found_det_packs[trapcount++] = found->s.number;
	}
	//now remove first ones we find until there are only 9 left
	found = NULL;
	const int trapcount_org = trapcount;
	int lowestTimeStamp = level.time;
	while (trapcount > 9)
	{
		int removeMe = -1;
		for (int i = 0; i < trapcount_org; i++)
		{
			if (found_det_packs[i] == ENTITYNUM_NONE)
			{
				continue;
			}
			found = &g_entities[found_det_packs[i]];
			if (found->setTime < lowestTimeStamp)
			{
				removeMe = i;
				lowestTimeStamp = found->setTime;
			}
		}
		if (removeMe != -1)
		{
			//remove it... or blow it?
			if (&g_entities[found_det_packs[removeMe]] == NULL)
			{
				break;
			}
			if (!CheatsOn())
			{
				//Let them have unlimited if cheats are enabled
				G_FreeEntity(&g_entities[found_det_packs[removeMe]]);
			}
			found_det_packs[removeMe] = ENTITYNUM_NONE;
			trapcount--;
		}
		else
		{
			break;
		}
	}

	if (alt_fire)
	{
		BlowDetpacks(ent);
	}
	else
	{
		AngleVectors(ent->client->ps.viewangles, forward, vright, up);

		calcmuzzlePoint(ent, forward, vright, muzzle);

		VectorNormalize(forward);
		VectorMA(muzzle, -4, forward, muzzle);
		drop_charge(ent, muzzle, forward);

		ent->client->ps.hasDetPackPlanted = qtrue;
	}
}

static void WP_FireConcussionAlt(gentity_t* ent)
{
	//a rail-gun-like beam
	const int traces = DISRUPTOR_ALT_TRACES;
	qboolean render_impact = qtrue;
	vec3_t start;
	vec3_t dir;
	trace_t tr;
	qboolean hit_dodged = qfalse;
	vec3_t shot_mins, shot_maxs;

	//Shove us backwards for half a second
	VectorMA(ent->client->ps.velocity, -200, forward, ent->client->ps.velocity);
	ent->client->ps.groundEntityNum = ENTITYNUM_NONE;

	if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_HALF)
	{
		G_SetAnim(ent, NULL, SETANIM_BOTH, BOTH_H1_S1_TR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
	}
	else
	{
		G_SetAnim(ent, NULL, SETANIM_LEGS, BOTH_H1_S1_TR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
	}

	if (ent->client->ps.pm_flags & PMF_DUCKED)
	{
		//hunkered down
		ent->client->ps.pm_time = 100;
	}
	else
	{
		ent->client->ps.pm_time = 250;
	}

	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start, vec3_origin, vec3_origin);

	int skip = ent->s.number;

	//Make it a little easier to hit guys at long range
	VectorSet(shot_mins, -1, -1, -1);
	VectorSet(shot_maxs, 1, 1, 1);

	for (int i = 0; i < traces; i++)
	{
		const float shot_range = 8192.0f;
		vec3_t end;
		VectorMA(start, shot_range, forward, end);

		if (d_projectileGhoul2Collision.integer)
		{
			trap->Trace(&tr, start, shot_mins, shot_maxs, end, skip, MASK_SHOT, qfalse,
				G2TRFLAG_DOGHOULTRACE | G2TRFLAG_GETSURFINDEX | G2TRFLAG_HITCORPSES, g_g2TraceLod.integer);
		}
		else
		{
			trap->Trace(&tr, start, shot_mins, shot_maxs, end, skip, MASK_SHOT, qfalse, 0, 0);
		}

		gentity_t* traceEnt = &g_entities[tr.entityNum];

		if (d_projectileGhoul2Collision.integer && traceEnt->inuse && traceEnt->client)
		{
			//g2 collision checks -rww
			if (traceEnt->inuse && traceEnt->client && traceEnt->ghoul2)
			{
				//since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				traceEnt->client->g2LastSurfaceHit = tr.surfaceFlags;
				traceEnt->client->g2LastSurfaceTime = level.time;
			}

			if (traceEnt->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}
		if (tr.surfaceFlags & SURF_NOIMPACT)
		{
			render_impact = qfalse;
		}

		if (tr.entityNum == ent->s.number)
		{
			VectorCopy(tr.endpos, start);
			skip = tr.entityNum;
#ifdef _DEBUG
			Com_Printf("BAD! Concussion gun shot somehow traced back and hit the owner!\n");
#endif
			continue;
		}

		if (tr.fraction >= 1.0f)
		{
			// draw the beam but don't do anything else
			break;
		}

		if (traceEnt)
		{
			if (wp_player_must_dodge(traceEnt, ent) && !
				WP_DoingForcedAnimationForForcePowers(traceEnt))
			{
				//players can block or dodge disruptor shots.
				if (traceEnt->r.svFlags & SVF_BOT)
				{
					WP_ForcePowerDrain(&traceEnt->client->ps, FP_SABER_DEFENSE, FATIGUE_DODGEINGBOT);
				}
				else
				{
					WP_ForcePowerDrain(&traceEnt->client->ps, FP_SABER_DEFENSE, FATIGUE_DODGEING);
				}
				if (d_combatinfo.integer || g_DebugSaberCombat.integer && !(traceEnt->r.svFlags & SVF_BOT))
				{
					Com_Printf(S_COLOR_ORANGE"should be dodging now\n");
				}

				//force player into a projective block move.
				jedi_disruptor_dodge_evasion(traceEnt, ent, tr.endpos, -1);

				skip = tr.entityNum;
				VectorCopy(tr.endpos, start);
				continue;
			}
		}
		if (!hit_dodged)
		{
			if (render_impact)
			{
				if (tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage
					|| !Q_stricmp(traceEnt->classname, "misc_model_breakable")
					|| traceEnt->s.eType == ET_MOVER)
				{
					const int damage = CONC_ALT_DAMAGE;
					if (traceEnt->client && LogAccuracyHit(traceEnt, ent))
					{
						//NOTE: hitting multiple ents can still get you over 100% accuracy
						ent->client->accuracy_hits++;
					}

					const qboolean no_knock_back = traceEnt->flags & FL_NO_KNOCKBACK;
					//will be set if they die, I want to know if it was on *before* they died
					if (traceEnt && traceEnt->client && traceEnt->client->NPC_class == CLASS_GALAKMECH)
					{
						//hehe
						G_Damage(traceEnt, ent, ent, forward, tr.endpos, 10, DAMAGE_NO_KNOCKBACK | DAMAGE_NO_HIT_LOC,
							MOD_CONC_ALT);
						break;
					}
					G_Damage(traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NO_KNOCKBACK | DAMAGE_NO_HIT_LOC,
						MOD_CONC_ALT);

					//do knockback and knockdown manually
					if (traceEnt->client)
					{
						//only if we hit a client
						vec3_t push_dir;
						VectorCopy(forward, push_dir);
						if (push_dir[2] < 0.2f)
						{
							push_dir[2] = 0.2f;
						} //hmm, re-normalize?  nah...

						if (traceEnt->health > 0)
						{
							//alive
							//if ( G_HasKnockdownAnims( traceEnt ) )
							if (!no_knock_back && !traceEnt->localAnimIndex && traceEnt->client->ps.forceHandExtend !=
								HANDEXTEND_KNOCKDOWN &&
								BG_KnockDownable(&traceEnt->client->ps)) //just check for humanoids..
							{
								//knock-downable
								vec3_t pl_p_dif;

								//cap it and stuff, base the strength and whether or not we can knockdown on the distance
								//from the shooter to the target
								VectorSubtract(traceEnt->client->ps.origin, ent->client->ps.origin, pl_p_dif);
								float p_str = 500.0f - VectorLength(pl_p_dif);
								if (p_str < 150.0f)
								{
									p_str = 150.0f;
								}
								if (p_str > 200.0f)
								{
									traceEnt->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
									traceEnt->client->ps.forceHandExtendTime = level.time + 1100;
									traceEnt->client->ps.forceDodgeAnim = 0;
									//this toggles between 1 and 0, when it's 1 we should play the get up anim
								}
								traceEnt->client->ps.otherKiller = ent->s.number;
								traceEnt->client->ps.otherKillerTime = level.time + 5000;
								traceEnt->client->ps.otherKillerDebounceTime = level.time + 100;
								traceEnt->client->otherKillerMOD = MOD_UNKNOWN;
								traceEnt->client->otherKillerVehWeapon = 0;
								traceEnt->client->otherKillerWeaponType = WP_NONE;

								traceEnt->client->ps.velocity[0] += push_dir[0] * p_str;
								traceEnt->client->ps.velocity[1] += push_dir[1] * p_str;
								traceEnt->client->ps.velocity[2] = p_str;
							}
						}
					}

					if (traceEnt->s.eType == ET_MOVER)
					{
						//stop the traces on any mover
						break;
					}
				}
				else
				{
					break;
					// hit solid, but doesn't take damage, so stop the shot...we _could_ allow it to shoot through walls, might be cool?
				}
			}
			else // not rendering impact, must be a skybox or other similar thing?
			{
				break; // don't try anymore traces
			}
		}
		// Get ready for an attempt to trace through another person
		//VectorCopy( tr.endpos, muzzle2 );
		VectorCopy(tr.endpos, start);
		skip = tr.entityNum;
		hit_dodged = qfalse;
	}

	// now go along the trail and make sight events
	VectorSubtract(tr.endpos, muzzle, dir);

	//let's pack all this junk into a single tempent, and send it off.
	gentity_t* tent = G_TempEntity(tr.endpos, EV_CONC_ALT_IMPACT);
	tent->s.eventParm = DirToByte(tr.plane.normal);
	tent->s.owner = ent->s.number;
	VectorCopy(dir, tent->s.angles);
	VectorCopy(muzzle, tent->s.origin2);
	VectorCopy(forward, tent->s.angles2);

#if 0 //yuck
	//FIXME: if shoot *really* close to someone, the alert could be way out of their FOV
	for (dist = 0; dist < shotDist; dist += 64)
	{
		//FIXME: on a really long shot, this could make a LOT of alerts in one frame...
		VectorMA(muzzle, dist, dir, spot);
		AddSightEvent(ent, spot, 256, AEL_DISCOVERED, 50);
		//FIXME: creates *way* too many effects, make it one effect somehow?
		G_PlayEffectID(G_EffectIndex("concussion/alt_ring"), spot, actualAngles);
	}
	//FIXME: spawn a temp ent that continuously spawns sight alerts here?  And 1 sound alert to draw their attention?
	VectorMA(start, shotDist - 4, forward, spot);
	AddSightEvent(ent, spot, 256, AEL_DISCOVERED, 50);

	G_PlayEffectID(G_EffectIndex("concussion/altmuzzle_flash"), muzzle, forward);
#endif
}

static void WP_FireConcussion(gentity_t* ent)
{
	//a fast rocket-like projectile
	vec3_t start;
	const int damage = CONC_DAMAGE;
	const float vel = CONC_VELOCITY;

	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start, vec3_origin, vec3_origin);
	//make sure our start point isn't on the other side of a wall

	gentity_t* missile = CreateMissile(start, forward, vel, 10000, ent, qfalse);

	missile->classname = "conc_proj";
	missile->s.weapon = WP_CONCUSSION;
	missile->mass = 10;

	// Make it easier to hit things
	VectorSet(missile->r.maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE);
	VectorScale(missile->r.maxs, -1, missile->r.mins);

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;

	missile->methodOfDeath = MOD_CONC;
	missile->splashMethodOfDeath = MOD_CONC;

	missile->clipmask = MASK_SHOT;
	missile->splashDamage = CONC_SPLASH_DAMAGE;
	missile->splashRadius = CONC_SPLASH_RADIUS;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

//---------------------------------------------------------
// FireStunBaton
//---------------------------------------------------------
void WP_FireStunBaton(gentity_t* ent, const qboolean alt_fire)
{
	if (alt_fire)
	{
		//
	}
	else
	{
		trace_t tr;
		vec3_t mins, maxs, end;
		vec3_t muzzle_stun;

		if (!ent->client)
		{
			VectorCopy(ent->r.currentOrigin, muzzle_stun);
			muzzle_stun[2] += 8;
		}
		else
		{
			VectorCopy(ent->client->ps.origin, muzzle_stun);
			muzzle_stun[2] += ent->client->ps.viewheight - 6;
		}

		VectorMA(muzzle_stun, 20.0f, forward, muzzle_stun);
		VectorMA(muzzle_stun, 4.0f, vright, muzzle_stun);

		VectorMA(muzzle_stun, STUN_BATON_RANGE, forward, end);

		VectorSet(maxs, 6, 6, 6);
		VectorScale(maxs, -1, mins);

		trap->Trace(&tr, muzzle_stun, mins, maxs, end, ent->s.number, MASK_SHOT, qfalse, 0, 0);

		if (tr.entityNum >= ENTITYNUM_WORLD)
		{
			return;
		}

		gentity_t* tr_ent = &g_entities[tr.entityNum];

		if (tr_ent && tr_ent->takedamage && tr_ent->client)
		{
			//see if either party is involved in a duel
			if (tr_ent->client->ps.duelInProgress &&
				tr_ent->client->ps.duelIndex != ent->s.number)
			{
				return;
			}

			if (ent->client &&
				ent->client->ps.duelInProgress &&
				ent->client->ps.duelIndex != tr_ent->s.number)
			{
				return;
			}
		}

		if (tr_ent && tr_ent->takedamage)
		{
			G_PlayEffect(EFFECT_STUNHIT, tr.endpos, tr.plane.normal);

			G_Sound(tr_ent, CHAN_WEAPON, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
			G_Damage(tr_ent, ent, ent, forward, tr.endpos, STUN_BATON_DAMAGE, DAMAGE_NO_KNOCKBACK | DAMAGE_HALF_ABSORB,
				MOD_STUN_BATON);

			if (tr_ent->client)
			{
				//if it's a player then use the shock effect
				if (tr_ent->client->NPC_class == CLASS_VEHICLE)
				{
					//not on vehicles
					if (!tr_ent->m_pVehicle
						|| tr_ent->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL
						|| tr_ent->m_pVehicle->m_pVehicleInfo->type == VH_FLIER)
					{
						//can zap animals
						tr_ent->client->ps.electrifyTime = level.time + Q_irand(3000, 4000);
					}
				}
				else
				{
					tr_ent->client->ps.electrifyTime = level.time + 700;
				}
			}
		}
	}
}

//---------------------------------------------------------
// FireMelee
//---------------------------------------------------------
static void WP_FireMelee(gentity_t* ent, qboolean alt_fire)
{
	trace_t tr;
	vec3_t mins, maxs, end;
	vec3_t muzzle_punch;

	if (ent->client && ent->client->ps.torsoAnim == BOTH_MELEE2
		|| ent->client && ent->client->ps.torsoAnim == BOTH_MELEE4
		|| ent->client && ent->client->ps.torsoAnim == BOTH_MELEEUP
		|| ent->client && ent->client->ps.torsoAnim == BOTH_WOOKIE_SLAP)
	{
		//right
		if (ent->client->ps.brokenLimbs & 1 << BROKENLIMB_RARM)
		{
			return;
		}
	}
	else
	{
		//left
		if (ent->client && ent->client->ps.brokenLimbs & 1 << BROKENLIMB_LARM)
		{
			return;
		}
	}

	if (!ent->client)
	{
		VectorCopy(ent->r.currentOrigin, muzzle_punch);
		muzzle_punch[2] += 8;
	}
	else
	{
		VectorCopy(ent->client->ps.origin, muzzle_punch);
		muzzle_punch[2] += ent->client->ps.viewheight - 6;
	}

	VectorMA(muzzle_punch, 20.0f, forward, muzzle_punch);
	VectorMA(muzzle_punch, 4.0f, vright, muzzle_punch);

	VectorMA(muzzle_punch, MELEE_RANGE, forward, end);

	VectorSet(maxs, 6, 6, 6);
	VectorScale(maxs, -1, mins);

	trap->Trace(&tr, muzzle_punch, mins, maxs, end, ent->s.number, MASK_SHOT, qfalse, 0, 0);

	if (tr.entityNum != ENTITYNUM_NONE)
	{
		//hit something
		gentity_t* tr_ent = &g_entities[tr.entityNum];

		G_Sound(ent, CHAN_AUTO, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));

		if (tr_ent->takedamage && tr_ent->client)
		{
			//special duel checks
			if (tr_ent->client->ps.duelInProgress &&
				tr_ent->client->ps.duelIndex != ent->s.number)
			{
				return;
			}

			if (ent->client &&
				ent->client->ps.duelInProgress &&
				ent->client->ps.duelIndex != tr_ent->s.number)
			{
				return;
			}
		}

		if (tr_ent->takedamage)
		{
			//damage them, do more damage if we're in the second right hook
			int dmg = MELEE_SWING1_DAMAGE;

			if (ent->client && ent->client->ps.torsoAnim == BOTH_MELEE2)
			{
				//do a tad bit more damage on the second swing
				dmg = MELEE_SWING2_DAMAGE;
			}
			if (ent->client->pers.botclass == BCLASS_GRAN)
			{
				//do a tad bit more damage IF WOOKIE CLASS // SERENITY
				dmg = MELEE_SWING_EXTRA_DAMAGE;
			}
			if (ent->client->pers.botclass == BCLASS_CHEWIE)
			{
				//do a tad bit more damage IF WOOKIE CLASS // SERENITY
				dmg = MELEE_SWING_WOOKIE_DAMAGE;
			}
			if (ent->client->pers.botclass == BCLASS_WOOKIE)
			{
				//do a tad bit more damage IF WOOKIE CLASS // SERENITY
				dmg = MELEE_SWING_WOOKIE_DAMAGE;
			}
			if (ent->client->pers.botclass == BCLASS_WOOKIEMELEE)
			{
				//do a tad bit more damage IF WOOKIE CLASS // SERENITY
				dmg = MELEE_SWING_WOOKIE_DAMAGE;
			}
			if (ent->client->pers.botclass == BCLASS_SBD)
			{
				//do a tad bit more damage IF WOOKIE CLASS // SERENITY
				dmg = MELEE_SWING_EXTRA_DAMAGE;
			}
			if (ent->client->ps.torsoAnim == BOTH_WOOKIE_SLAP)
			{
				//do a tad bit more damage IF WOOKIE CLASS // SERENITY
				dmg = MELEE_SWING_WOOKIE_DAMAGE;
			}
			if (ent->client->ps.torsoAnim == BOTH_MELEEUP)
			{
				//do a tad bit more damage IF WOOKIE CLASS // SERENITY
				dmg = MELEE_SWING_EXTRA_DAMAGE;
			}

			if (G_HeavyMelee(ent))
			{
				//2x damage for heavy melee class
				dmg *= 2;
			}

			G_Damage(tr_ent, ent, ent, forward, tr.endpos, dmg, DAMAGE_NO_ARMOR, MOD_MELEE);
		}
	}
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

/*
======================================================================

GRAPPLING HOOK

======================================================================
*/
extern gentity_t* fire_grapple(gentity_t* self, vec3_t start, vec3_t dir);
extern gentity_t* fire_stun(gentity_t* self, vec3_t start, vec3_t dir);

void Weapon_GrapplingHook_Fire(gentity_t* ent)
{
	vec3_t right;

	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	calcmuzzlePoint(ent, forward, right, muzzle);
	if (!ent->client->fireHeld && !ent->client->hook)
	{
		fire_grapple(ent, muzzle, forward);
	}
	ent->client->fireHeld = qtrue;
}

void Weapon_AltStun_Fire(gentity_t* ent)
{
	vec3_t forward, right, vup;

	AngleVectors(ent->client->ps.viewangles, forward, right, vup);
	calcmuzzlePoint(ent, forward, right, muzzle);
	if (!ent->client->stunHeld && !ent->client->stun)
	{
		fire_stun(ent, muzzle, forward);
	}
	ent->client->stunHeld = qtrue;
}

void Weapon_HookFree(gentity_t* ent)
{
	ent->parent->client->fireHeld = qfalse;
	ent->parent->client->hookhasbeenfired = qfalse;
	ent->parent->client->hook = NULL;
	ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	G_FreeEntity(ent);
}

void Weapon_StunFree(gentity_t* ent)
{
	ent->parent->client->stunHeld = qfalse;
	ent->parent->client->stunhasbeenfired = qfalse;
	ent->parent->client->stun = NULL;
	ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	G_FreeEntity(ent);
}

void Weapon_HookThink(gentity_t* ent)
{
	if (ent->enemy)
	{
		vec3_t v, oldorigin;

		VectorCopy(ent->r.currentOrigin, oldorigin);
		v[0] = ent->enemy->r.currentOrigin[0] + (ent->enemy->r.mins[0] + ent->enemy->r.maxs[0]) * 0.5;
		v[1] = ent->enemy->r.currentOrigin[1] + (ent->enemy->r.mins[1] + ent->enemy->r.maxs[1]) * 0.5;
		v[2] = ent->enemy->r.currentOrigin[2] + (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5;
		SnapVectorTowards(v, oldorigin); // save net bandwidth

		G_SetOrigin(ent, v);
		ent->nextthink = level.time + 50;
	}

	VectorCopy(ent->r.currentOrigin, ent->parent->client->ps.lastHitLoc);
}

void Weapon_StunThink(gentity_t* ent)
{
	if (ent->enemy)
	{
		vec3_t v, oldorigin;

		VectorCopy(ent->r.currentOrigin, oldorigin);

		G_SetOrigin(ent, v);
		ent->nextthink = level.time + 50; //continue to think if attached to an enemy!
	}

	VectorCopy(ent->r.currentOrigin, ent->parent->client->ps.lastHitLoc);
}

/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/
void SnapVectorTowards(vec3_t v, vec3_t to)
{
	for (int i = 0; i < 3; i++)
	{
		if (to[i] <= v[i])
		{
			v[i] = floorf(v[i]);
		}
		else
		{
			v[i] = ceilf(v[i]);
		}
	}
}

//======================================================================

/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit(const gentity_t* target, const gentity_t* attacker)
{
	if (!target->takedamage)
	{
		return qfalse;
	}

	if (target == attacker)
	{
		return qfalse;
	}

	if (!target->client)
	{
		return qfalse;
	}

	if (!attacker)
	{
		return qfalse;
	}

	if (!attacker->client)
	{
		return qfalse;
	}

	if (target->client->ps.stats[STAT_HEALTH] <= 0)
	{
		return qfalse;
	}

	if (OnSameTeam(target, attacker))
	{
		return qfalse;
	}

	return qtrue;
}

/*
===============
calcmuzzlePoint

set muzzle location relative to pivoting eye
rwwFIXMEFIXME: Since ghoul2 models are on server and properly updated now,
it may be reasonable to base muzzle point off actual weapon bolt point.
The down side would be that it does not necessarily look alright from a
first person perspective.
===============
*/
void calcmuzzlePoint(const gentity_t* ent, const vec3_t in_forward, const vec3_t in_right,
	vec3_t muzzlePoint)
{
	vec3_t muzzle_off_point;

	const int weapontype = ent->s.weapon;
	VectorCopy(ent->s.pos.trBase, muzzlePoint);

	VectorCopy(WP_muzzlePoint[weapontype], muzzle_off_point);

	if (weapontype > WP_NONE && weapontype < WP_NUM_WEAPONS)
	{
		// Use the table to generate the muzzlePoint;
		{
			// Crouching.  Use the add-to-Z method to adjust vertically.
			VectorMA(muzzlePoint, muzzle_off_point[0], in_forward, muzzlePoint);
			VectorMA(muzzlePoint, muzzle_off_point[1], in_right, muzzlePoint);
			muzzlePoint[2] += ent->client->ps.viewheight + muzzle_off_point[2];
		}
	}

	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector(muzzlePoint);
}

static void calcmuzzlePoint2(const gentity_t* ent, vec3_t forward, vec3_t right, vec3_t muzzlePoint)
{
	vec3_t muzzle_off_point;

	const int weapontype = ent->s.weapon;
	VectorCopy(ent->s.pos.trBase, muzzlePoint);

	VectorCopy(WP_muzzlePoint2[weapontype], muzzle_off_point);

	if (weapontype > WP_NONE && weapontype < WP_NUM_WEAPONS)
	{
		// Use the table to generate the muzzlePoint;
		{
			// Crouching.  Use the add-to-Z method to adjust vertically.
			VectorMA(muzzlePoint, muzzle_off_point[0], forward, muzzlePoint);
			VectorMA(muzzlePoint, muzzle_off_point[1], right, muzzlePoint);
			muzzlePoint[2] += ent->client->ps.viewheight + muzzle_off_point[2];
		}
	}

	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector(muzzlePoint);
}

extern qboolean G_MissileImpact(gentity_t* ent, trace_t* trace);

static void WP_TouchVehMissile(gentity_t* ent, const gentity_t* other, const trace_t* trace)
{
	trace_t myTrace;
	memcpy(&myTrace, trace, sizeof myTrace);
	if (other)
	{
		myTrace.entityNum = other->s.number;
	}
	G_MissileImpact(ent, &myTrace);
}

void WP_CalcVehMuzzle(gentity_t* ent, const int muzzle_num)
{
	Vehicle_t* p_veh = ent->m_pVehicle;
	mdxaBone_t boltMatrix;
	vec3_t veh_angles;

	assert(p_veh);

	if (p_veh->m_iMuzzleTime[muzzle_num] == level.time)
	{
		//already done for this frame, don't need to do it again
		return;
	}
	//Uh... how about we set this, hunh...?  :)
	p_veh->m_iMuzzleTime[muzzle_num] = level.time;

	VectorCopy(ent->client->ps.viewangles, veh_angles);
	if (p_veh->m_pVehicleInfo
		&& (p_veh->m_pVehicleInfo->type == VH_ANIMAL
			|| p_veh->m_pVehicleInfo->type == VH_WALKER
			|| p_veh->m_pVehicleInfo->type == VH_SPEEDER))
	{
		veh_angles[PITCH] = veh_angles[ROLL] = 0;
	}

	trap->G2API_GetBoltMatrix_NoRecNoRot(ent->ghoul2, 0, p_veh->m_iMuzzleTag[muzzle_num], &boltMatrix, veh_angles,
		ent->client->ps.origin, level.time, NULL, ent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, p_veh->m_vMuzzlePos[muzzle_num]);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, p_veh->m_vMuzzleDir[muzzle_num]);
}

static void WP_VehWeapSetSolidToOwner(gentity_t* self)
{
	self->r.svFlags |= SVF_OWNERNOTSHARED;
	if (self->genericValue1)
	{
		//expire after a time
		if (self->genericValue2)
		{
			//blow up when your lifetime is up
			self->think = g_explode_missile; //FIXME: custom func?
		}
		else
		{
			//just remove yourself
			self->think = G_FreeEntity; //FIXME: custom func?
		}
		self->nextthink = level.time + self->genericValue1;
	}
}

#define VEH_HOMING_MISSILE_THINK_TIME		100

gentity_t* WP_FireVehicleWeapon(gentity_t* ent, vec3_t start, vec3_t dir, const vehWeaponInfo_t* veh_weapon,
	const qboolean alt_fire,
	const qboolean is_turret_weap)
{
	gentity_t* missile = NULL;

	//FIXME: add some randomness...?  Inherent inaccuracy stat of weapon?  Pilot skill?
	if (!veh_weapon)
	{
		//invalid vehicle weapon
		return NULL;
	}
	if (veh_weapon->bIsProjectile)
	{
		//projectile entity
		vec3_t mins, maxs;

		VectorSet(maxs, veh_weapon->fWidth / 2.0f, veh_weapon->fWidth / 2.0f, veh_weapon->fHeight / 2.0f);
		VectorScale(maxs, -1, mins);

		//make sure our start point isn't on the other side of a wall
		WP_TraceSetStart(ent, start, mins, maxs);

		//FIXME: CUSTOM MODEL?
		//QUERY: alt_fire true or not?  Does it matter?
		missile = CreateMissile(start, dir, veh_weapon->fSpeed, 10000, ent, qfalse);

		missile->classname = "vehicle_proj";

		missile->s.genericenemyindex = ent->s.number + MAX_GENTITIES;
		missile->damage = veh_weapon->iDamage;
		missile->splashDamage = veh_weapon->iSplashDamage;
		missile->splashRadius = veh_weapon->fSplashRadius;

		//FIXME: externalize some of these properties?
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
		missile->clipmask = MASK_SHOT;
		//Maybe by checking flags...?
		if (veh_weapon->bSaberBlockable)
		{
			missile->clipmask |= CONTENTS_LIGHTSABER;
		}
		// Make it easier to hit things
		VectorCopy(mins, missile->r.mins);
		VectorCopy(maxs, missile->r.maxs);
		//some slightly different stuff for things with bboxes
		if (veh_weapon->fWidth || veh_weapon->fHeight)
		{
			//we assume it's a rocket-like thing
			missile->s.weapon = WP_ROCKET_LAUNCHER; //does this really matter?
			missile->methodOfDeath = MOD_VEHICLE; //MOD_ROCKET;
			missile->splashMethodOfDeath = MOD_VEHICLE; //MOD_ROCKET;// ?SPLASH;

			// we don't want it to ever bounce
			missile->bounceCount = 0;

			missile->mass = 10;
		}
		else
		{
			//a blaster-laser-like thing
			missile->s.weapon = WP_BLASTER; //does this really matter?
			missile->methodOfDeath = MOD_VEHICLE; //count as a heavy weap
			missile->splashMethodOfDeath = MOD_VEHICLE; // ?SPLASH;
			// we don't want it to bounce forever
			missile->bounceCount = 8;
		}

		if (veh_weapon->bHasGravity)
		{
			//TESTME: is this all we need to do?
			missile->s.weapon = WP_THERMAL; //does this really matter?
			missile->s.pos.trType = TR_GRAVITY;
		}

		if (veh_weapon->bIonWeapon)
		{
			//so it disables ship shields and sends them out of control
			missile->s.weapon = WP_DEMP2;
		}

		if (veh_weapon->iHealth)
		{
			//the missile can take damage
		}

		//pilot should own this projectile on server if we have a pilot
		if (ent->m_pVehicle && ent->m_pVehicle->m_pPilot)
		{
			//owned by vehicle pilot
			missile->r.ownerNum = ent->m_pVehicle->m_pPilot->s.number;
		}
		else
		{
			//owned by vehicle?
			missile->r.ownerNum = ent->s.number;
		}

		//set veh as cgame side owner for purpose of fx overrides
		missile->s.owner = ent->s.number;
		if (alt_fire)
		{
			//use the second weapon's iShotFX
			missile->s.eFlags |= EF_ALT_FIRING;
		}
		if (is_turret_weap)
		{
			//look for the turret weapon info on cgame side, not vehicle weapon info
			missile->s.weapon = WP_TURRET;
		}
		if (veh_weapon->iLifeTime)
		{
			//expire after a time
			if (veh_weapon->bExplodeOnExpire)
			{
				//blow up when your lifetime is up
				missile->think = g_explode_missile; //FIXME: custom func?
			}
			else
			{
				//just remove yourself
				missile->think = G_FreeEntity; //FIXME: custom func?
			}
			missile->nextthink = level.time + veh_weapon->iLifeTime;
		}
		missile->s.otherentity_num2 = veh_weapon - &g_vehWeaponInfo[0];
		missile->s.eFlags |= EF_JETPACK_ACTIVE;
		//homing
		if (veh_weapon->fHoming)
		{
			//homing missile
			if (ent->client && ent->client->ps.rocketLockIndex != ENTITYNUM_NONE)
			{
				int dif;
				float r_time = ent->client->ps.rocketLockTime;

				if (r_time == -1)
				{
					r_time = ent->client->ps.rocketLastValidTime;
				}

				if (!veh_weapon->iLockOnTime)
				{
					//no minimum lock-on time
					dif = 10; //guaranteed lock-on
				}
				else
				{
					const float lock_time_interval = veh_weapon->iLockOnTime / 16.0f;
					dif = (level.time - r_time) / lock_time_interval;
				}

				if (dif < 0)
				{
					dif = 0;
				}

				//It's 10 even though it locks client-side at 8, because we want them to have a sturdy lock first, and because there's a slight difference in time between server and client
				if (dif >= 10 && r_time != -1)
				{
					missile->enemy = &g_entities[ent->client->ps.rocketLockIndex];

					if (missile->enemy && missile->enemy->client && missile->enemy->health > 0 && !OnSameTeam(
						ent, missile->enemy))
					{
						//if enemy became invalid, died, or is on the same team, then don't seek it
						missile->spawnflags |= 1; //just to let it know it should be faster...
						missile->speed = veh_weapon->fSpeed;
						missile->angle = veh_weapon->fHoming;
						missile->radius = veh_weapon->fHomingFOV;
						//crap, if we have a lifetime, need to store that somewhere else on ent and have rocketThink func check it every frame...
						if (veh_weapon->iLifeTime)
						{
							//expire after a time
							missile->genericValue1 = level.time + veh_weapon->iLifeTime;
							missile->genericValue2 = (int)veh_weapon->bExplodeOnExpire;
						}
						//now go ahead and use the rocketThink func
						missile->think = rocketThink; //FIXME: custom func?
						missile->nextthink = level.time + VEH_HOMING_MISSILE_THINK_TIME;
						missile->s.eFlags |= EF_RADAROBJECT; //FIXME: externalize
						if (missile->enemy->s.NPC_class == CLASS_VEHICLE)
						{
							//let vehicle know we've locked on to them
							missile->s.otherentity_num = missile->enemy->s.number;
						}
					}
				}

				VectorCopy(dir, missile->movedir);
				missile->random = 1.0f; //FIXME: externalize?
			}
		}
		if (!veh_weapon->fSpeed)
		{
			//a mine or something?
			if (veh_weapon->iHealth)
			{
				//the missile can take damage
				missile->health = veh_weapon->iHealth;
				missile->takedamage = qtrue;
				missile->r.contents = MASK_SHOT;
				missile->die = RocketDie;
			}
			//only do damage when someone touches us
			missile->s.weapon = WP_THERMAL; //does this really matter?
			G_SetOrigin(missile, start);
			missile->touch = WP_TouchVehMissile;
			missile->s.eFlags |= EF_RADAROBJECT; //FIXME: externalize
			//crap, if we have a lifetime, need to store that somewhere else on ent and have rocketThink func check it every frame...
			if (veh_weapon->iLifeTime)
			{
				//expire after a time
				missile->genericValue1 = veh_weapon->iLifeTime;
				missile->genericValue2 = (int)veh_weapon->bExplodeOnExpire;
			}
			//now go ahead and use the setsolidtoowner func
			missile->think = WP_VehWeapSetSolidToOwner;
			missile->nextthink = level.time + 3000;
		}
	}
	else
	{
		//traceline
		//FIXME: implement
	}

	return missile;
}

//custom routine to not waste tempents horribly -rww
void G_VehMuzzleFireFX(const gentity_t* ent, gentity_t* broadcaster, const int muzzles_fired)
{
	const Vehicle_t* p_veh = ent->m_pVehicle;
	gentity_t* b;

	if (!p_veh)
	{
		return;
	}

	if (!broadcaster)
	{
		//oh well. We will WASTE A TEMPENT.
		b = G_TempEntity(ent->client->ps.origin, EV_VEH_FIRE);
	}
	else
	{
		//joy
		b = broadcaster;
	}

	//this guy owns it
	b->s.owner = ent->s.number;

	//this is the bitfield of all muzzles fired this time
	//NOTE: just need MAX_VEHICLE_MUZZLES bits for this... should be cool since it's currently 12 and we're sending it in 16 bits
	b->s.trickedentindex = muzzles_fired;

	if (broadcaster)
	{
		//add the event
		G_AddEvent(b, EV_VEH_FIRE, 0);
	}
}

static void G_EstimateCamPos(vec3_t view_angles, vec3_t camera_focus_loc, const float viewheight, const float third_person_range,
	const float third_person_horz_offset, const float vert_offset, const float pitch_offset,
	const int ignore_ent_num, vec3_t cam_pos)
{
	const int MASK_CAMERACLIP = MASK_SOLID | CONTENTS_PLAYERCLIP;
	const float CAMERA_SIZE = 4;
	vec3_t cameramins;
	vec3_t cameramaxs;
	vec3_t camera_focus_angles, camerafwd, cameraup;
	vec3_t camera_ideal_target, camera_cur_target;
	vec3_t camera_ideal_loc, camera_cur_loc;
	vec3_t diff;
	vec3_t cam_angles;
	trace_t trace;

	VectorSet(cameramins, -CAMERA_SIZE, -CAMERA_SIZE, -CAMERA_SIZE);
	VectorSet(cameramaxs, CAMERA_SIZE, CAMERA_SIZE, CAMERA_SIZE);

	VectorCopy(view_angles, camera_focus_angles);
	camera_focus_angles[PITCH] += pitch_offset;
	if (!bg_fighterAltControl.integer)
	{
		//clamp view pitch
		camera_focus_angles[PITCH] = AngleNormalize180(camera_focus_angles[PITCH]);
		if (camera_focus_angles[PITCH] > 80.0)
		{
			camera_focus_angles[PITCH] = 80.0;
		}
		else if (camera_focus_angles[PITCH] < -80.0)
		{
			camera_focus_angles[PITCH] = -80.0;
		}
	}
	AngleVectors(camera_focus_angles, camerafwd, NULL, cameraup);

	camera_focus_loc[2] += viewheight;

	VectorCopy(camera_focus_loc, camera_ideal_target);
	camera_ideal_target[2] += vert_offset;

	//NOTE: on cgame, this uses the thirdpersontargetdamp value, we ignore that here
	VectorCopy(camera_ideal_target, camera_cur_target);
	trap->Trace(&trace, camera_focus_loc, cameramins, cameramaxs, camera_cur_target, ignore_ent_num, MASK_CAMERACLIP, qfalse,
		0, 0);
	if (trace.fraction < 1.0)
	{
		VectorCopy(trace.endpos, camera_cur_target);
	}

	VectorMA(camera_ideal_target, -third_person_range, camerafwd, camera_ideal_loc);
	//NOTE: on cgame, this uses the thirdpersoncameradamp value, we ignore that here
	VectorCopy(camera_ideal_loc, camera_cur_loc);
	trap->Trace(&trace, camera_cur_target, cameramins, cameramaxs, camera_cur_loc, ignore_ent_num, MASK_CAMERACLIP, qfalse, 0,
		0);
	if (trace.fraction < 1.0)
	{
		VectorCopy(trace.endpos, camera_cur_loc);
	}

	VectorSubtract(camera_cur_target, camera_cur_loc, diff);
	{
		const float dist = VectorNormalize(diff);
		//under normal circumstances, should never be 0.00000 and so on.
		if (!dist || (diff[0] == 0 || diff[1] == 0))
		{
			//must be hitting something, need some value to calc angles, so use cam forward
			VectorCopy(camerafwd, diff);
		}
	}

	vectoangles(diff, cam_angles);

	if (third_person_horz_offset != 0.0f)
	{
		matrix3_t viewaxis;
		AnglesToAxis(cam_angles, viewaxis);
		VectorMA(camera_cur_loc, third_person_horz_offset, viewaxis[1], camera_cur_loc);
	}

	VectorCopy(camera_cur_loc, cam_pos);
}

void WP_GetVehicleCamPos(const gentity_t* ent, const gentity_t* pilot, vec3_t cam_pos)
{
	float third_person_horz_offset = ent->m_pVehicle->m_pVehicleInfo->cameraHorzOffset;
	float third_person_range = ent->m_pVehicle->m_pVehicleInfo->cameraRange;
	float pitch_offset = ent->m_pVehicle->m_pVehicleInfo->cameraPitchOffset;
	float vert_offset = ent->m_pVehicle->m_pVehicleInfo->cameraVertOffset;

	if (ent->client->ps.hackingTime)
	{
		third_person_horz_offset += (float)ent->client->ps.hackingTime / MAX_STRAFE_TIME * -80.0f;
		third_person_range += fabs((float)ent->client->ps.hackingTime / MAX_STRAFE_TIME) * 100.0f;
	}

	if (ent->m_pVehicle->m_pVehicleInfo->cameraPitchDependantVertOffset)
	{
		if (pilot->client->ps.viewangles[PITCH] > 0)
		{
			vert_offset = 130 + pilot->client->ps.viewangles[PITCH] * -10;
			if (vert_offset < -170)
			{
				vert_offset = -170;
			}
		}
		else if (pilot->client->ps.viewangles[PITCH] < 0)
		{
			vert_offset = 130 + pilot->client->ps.viewangles[PITCH] * -5;
			if (vert_offset > 130)
			{
				vert_offset = 130;
			}
		}
		else
		{
			vert_offset = 30;
		}
		if (pilot->client->ps.viewangles[PITCH] > 0)
		{
			pitch_offset = pilot->client->ps.viewangles[PITCH] * -0.75;
		}
		else if (pilot->client->ps.viewangles[PITCH] < 0)
		{
			pitch_offset = pilot->client->ps.viewangles[PITCH] * -0.75;
		}
		else
		{
			pitch_offset = 0;
		}
	}

	//Control Scheme 3 Method:
	G_EstimateCamPos(ent->client->ps.viewangles, pilot->client->ps.origin, pilot->client->ps.viewheight,
		third_person_range,
		third_person_horz_offset, vert_offset, pitch_offset,
		pilot->s.number, cam_pos);
}

static void WP_VehLeadCrosshairVeh(const gentity_t* camtrace_ent, vec3_t new_end, const vec3_t dir, const vec3_t shot_start,
	vec3_t shot_dir)
{
	if (g_vehAutoAimLead.integer)
	{
		if (camtrace_ent
			&& camtrace_ent->client
			&& camtrace_ent->client->NPC_class == CLASS_VEHICLE)
		{
			//if the crosshair is on a vehicle, lead it
			const float distAdjust = DotProduct(camtrace_ent->client->ps.velocity, dir);
			if (distAdjust > 500 || DistanceSquared(camtrace_ent->client->ps.origin, shot_start) > 7000000)
			{
				vec3_t predShotDir;
				vec3_t predPos;
				//moving away from me at a decent speed and/or more than @2600 units away from me
				VectorMA(new_end, distAdjust, dir, predPos);
				VectorSubtract(predPos, shot_start, predShotDir);
				VectorNormalize(predShotDir);
				const float dot = DotProduct(predShotDir, shot_dir);
				if (dot >= 0.75f)
				{
					//if the new aim vector is no more than 23 degrees off the original one, go ahead and adjust the aim
					VectorCopy(predPos, new_end);
				}
			}
		}
	}
	VectorSubtract(new_end, shot_start, shot_dir);
	VectorNormalize(shot_dir);
}

#define MAX_XHAIR_DIST_ACCURACY	20000.0f
extern float g_cullDistance;
extern int BG_VehTraceFromCamPos(trace_t* cam_trace, const bgEntity_t* bg_ent, const vec3_t ent_org, const vec3_t shot_start,
	const vec3_t end, vec3_t new_end, vec3_t shot_dir, float best_dist);

static qboolean WP_VehCheckTraceFromCamPos(gentity_t* ent, const vec3_t shot_start, vec3_t shot_dir)
{
	if (!ent
		|| !ent->m_pVehicle
		|| !ent->m_pVehicle->m_pVehicleInfo
		|| !ent->m_pVehicle->m_pPilot //not being driven
		|| !((gentity_t*)ent->m_pVehicle->m_pPilot)->client //not being driven by a client...?!!!
		|| ent->m_pVehicle->m_pPilot->s.number >= MAX_CLIENTS)
		//being driven, but not by a real client, no need to worry about crosshair
	{
		return qfalse;
	}
	if (ent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER && g_cullDistance > MAX_XHAIR_DIST_ACCURACY
		|| ent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)
	{
		//FIRST: simulate the normal crosshair trace from the center of the veh straight forward
		trace_t trace;
		vec3_t dir, start, end;
		if (ent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)
		{
			//for some reason, the walker always draws the crosshair out from from the first muzzle point
			AngleVectors(ent->client->ps.viewangles, dir, NULL, NULL);
			VectorCopy(ent->r.currentOrigin, start);
			start[2] += ent->m_pVehicle->m_pVehicleInfo->height - DEFAULT_MINS_2 - 48;
		}
		else
		{
			vec3_t ang;
			if (ent->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER)
			{
				VectorSet(ang, 0.0f, ent->m_pVehicle->m_vOrientation[1], 0.0f);
			}
			else
			{
				VectorCopy(ent->m_pVehicle->m_vOrientation, ang);
			}
			AngleVectors(ang, dir, NULL, NULL);
			VectorCopy(ent->r.currentOrigin, start);
		}
		VectorMA(start, g_cullDistance, dir, end);
		trap->Trace(&trace, start, vec3_origin, vec3_origin, end, ent->s.number, CONTENTS_SOLID | CONTENTS_BODY, qfalse,
			0, 0);

		if (ent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)
		{
			//just use the result of that one trace since walkers don't do the extra trace
			VectorSubtract(trace.endpos, shot_start, shot_dir);
			VectorNormalize(shot_dir);
			return qtrue;
		}
		//NOW do the trace from the camPos and compare with above trace
		trace_t extra_trace;
		vec3_t new_end;
		const int camtrace_ent_num = BG_VehTraceFromCamPos(&extra_trace, (bgEntity_t*)ent, ent->r.currentOrigin, shot_start,
			end, new_end, shot_dir, trace.fraction * g_cullDistance);
		if (camtrace_ent_num)
		{
			WP_VehLeadCrosshairVeh(&g_entities[camtrace_ent_num - 1], new_end, dir, shot_start, shot_dir);
			return qtrue;
		}
	}
	return qfalse;
}

//---------------------------------------------------------
static void FireVehicleWeapon(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	Vehicle_t* p_veh = ent->m_pVehicle;
	int muzzles_fired = 0;
	gentity_t* missile = NULL;
	const vehWeaponInfo_t* veh_weapon = NULL;
	qboolean clear_rocket_lock_entity = qfalse;

	if (!p_veh)
	{
		return;
	}

	if (p_veh->m_iRemovedSurfaces)
	{
		//can't fire when the thing is breaking apart
		return;
	}

	if (p_veh->m_pVehicleInfo->type == VH_WALKER &&
		ent->client->ps.electrifyTime > level.time)
	{
		//don't fire while being electrocuted
		return;
	}

	// If this is not the alternate fire, fire a normal blaster shot...
	if (p_veh->m_pVehicleInfo &&
		(p_veh->m_pVehicleInfo->type != VH_FIGHTER || p_veh->m_ulFlags & VEH_WINGSOPEN))
		// NOTE: Wings open also denotes that it has already launched.
	{
		//fighters can only fire when wings are open
		int weapon_num;
		qboolean linked_firing = qfalse;

		if (!alt_fire)
		{
			weapon_num = 0;
		}
		else
		{
			weapon_num = 1;
		}

		const int vehweapon_index = p_veh->m_pVehicleInfo->weapon[weapon_num].ID;

		if (p_veh->weaponStatus[weapon_num].ammo <= 0)
		{
			//no ammo for this weapon
			if (p_veh->m_pPilot && p_veh->m_pPilot->s.number < MAX_CLIENTS)
			{
				// let the client know he's out of ammo
				//but only if one of the vehicle muzzles is actually ready to fire this weapon
				for (int i = 0; i < MAX_VEHICLE_MUZZLES; i++)
				{
					if (p_veh->m_pVehicleInfo->weapMuzzle[i] != vehweapon_index)
					{
						//this muzzle doesn't match the weapon we're trying to use
						continue;
					}
					if (p_veh->m_iMuzzleTag[i] != -1
						&& p_veh->m_iMuzzleWait[i] < level.time)
					{
						//this one would have fired, send the no ammo message
						G_AddEvent((gentity_t*)p_veh->m_pPilot, EV_NOAMMO, weapon_num);
						break;
					}
				}
			}
			return;
		}

		const int delay = p_veh->m_pVehicleInfo->weapon[weapon_num].delay;
		const qboolean aim_correct = p_veh->m_pVehicleInfo->weapon[weapon_num].aimCorrect;
		if (p_veh->m_pVehicleInfo->weapon[weapon_num].linkable == 2 //always linked
			|| p_veh->m_pVehicleInfo->weapon[weapon_num].linkable == 1 //optionally linkable
			&& p_veh->weaponStatus[weapon_num].linked) //linked
		{
			//we're linking the primary or alternate weapons, so we'll do *all* the muzzles
			linked_firing = qtrue;
		}

		if (vehweapon_index <= VEH_WEAPON_BASE || vehweapon_index >= MAX_VEH_WEAPONS)
		{
			//invalid vehicle weapon
			return;
		}
		int i, numMuzzles = 0, numMuzzlesReady = 0, cumulativeDelay = 0, cumulativeAmmo = 0;
		qboolean sentAmmoWarning = qfalse;

		veh_weapon = &g_vehWeaponInfo[vehweapon_index];

		if (p_veh->m_pVehicleInfo->weapon[weapon_num].linkable == 2)
		{
			//always linked weapons don't accumulate delay, just use specified delay
			cumulativeDelay = delay;
		}
		//find out how many we've got for this weapon
		for (i = 0; i < MAX_VEHICLE_MUZZLES; i++)
		{
			if (p_veh->m_pVehicleInfo->weapMuzzle[i] != vehweapon_index)
			{
				//this muzzle doesn't match the weapon we're trying to use
				continue;
			}
			if (p_veh->m_iMuzzleTag[i] != -1 && p_veh->m_iMuzzleWait[i] < level.time)
			{
				numMuzzlesReady++;
			}
			if (p_veh->m_pVehicleInfo->weapMuzzle[p_veh->weaponStatus[weapon_num].nextMuzzle] != vehweapon_index)
			{
				//Our designated next muzzle for this weapon isn't valid for this weapon (happens when ships fire for the first time)
				//set the next to this one
				p_veh->weaponStatus[weapon_num].nextMuzzle = i;
			}
			if (linked_firing)
			{
				cumulativeAmmo += veh_weapon->iAmmoPerShot;
				if (p_veh->m_pVehicleInfo->weapon[weapon_num].linkable != 2)
				{
					//always linked weapons don't accumulate delay, just use specified delay
					cumulativeDelay += delay;
				}
			}
			numMuzzles++;
		}

		if (linked_firing)
		{
			//firing all muzzles at once
			if (numMuzzlesReady != numMuzzles)
			{
				//can't fire all linked muzzles yet
				return;
			}
			//can fire all linked muzzles, check ammo
			if (p_veh->weaponStatus[weapon_num].ammo < cumulativeAmmo)
			{
				//can't fire, not enough ammo
				if (p_veh->m_pPilot && p_veh->m_pPilot->s.number < MAX_CLIENTS)
				{
					// let the client know he's out of ammo
					G_AddEvent((gentity_t*)p_veh->m_pPilot, EV_NOAMMO, weapon_num);
				}
				return;
			}
		}

		for (i = 0; i < MAX_VEHICLE_MUZZLES; i++)
		{
			if (p_veh->m_pVehicleInfo->weapMuzzle[i] != vehweapon_index)
			{
				//this muzzle doesn't match the weapon we're trying to use
				continue;
			}
			if (!linked_firing
				&& i != p_veh->weaponStatus[weapon_num].nextMuzzle)
			{
				//we're only firing one muzzle and this isn't it
				continue;
			}

			// Fire this muzzle.
			if (p_veh->m_iMuzzleTag[i] != -1 && p_veh->m_iMuzzleWait[i] < level.time)
			{
				if (p_veh->weaponStatus[weapon_num].ammo < veh_weapon->iAmmoPerShot)
				{
					//out of ammo!
					if (!sentAmmoWarning)
					{
						sentAmmoWarning = qtrue;
						if (p_veh->m_pPilot && p_veh->m_pPilot->s.number < MAX_CLIENTS)
						{
							// let the client know he's out of ammo
							G_AddEvent((gentity_t*)p_veh->m_pPilot, EV_NOAMMO, weapon_num);
						}
					}
				}
				else
				{
					vec3_t dir;
					vec3_t start;
					//have enough ammo to shoot
					//do the firing
					WP_CalcVehMuzzle(ent, i);
					VectorCopy(p_veh->m_vMuzzlePos[i], start);
					VectorCopy(p_veh->m_vMuzzleDir[i], dir);
					if (WP_VehCheckTraceFromCamPos(ent, start, dir))
					{
						//auto-aim at whatever crosshair would be over from camera's point of view (if closer)
					}
					else if (aim_correct)
					{
						//auto-aim the missile at the crosshair if there's anything there
						trace_t trace;
						vec3_t end;
						vec3_t ang;
						vec3_t fixed_dir;

						if (p_veh->m_pVehicleInfo->type == VH_SPEEDER)
						{
							VectorSet(ang, 0.0f, p_veh->m_vOrientation[1], 0.0f);
						}
						else
						{
							VectorCopy(p_veh->m_vOrientation, ang);
						}
						AngleVectors(ang, fixed_dir, NULL, NULL);
						VectorMA(ent->r.currentOrigin, 32768, fixed_dir, end);
						//VectorMA( ent->r.currentOrigin, 8192, dir, end );
						trap->Trace(&trace, ent->r.currentOrigin, vec3_origin, vec3_origin, end, ent->s.number,
							MASK_SHOT, qfalse, 0, 0);
						if (trace.fraction < 1.0f && !trace.allsolid && !trace.startsolid)
						{
							vec3_t new_end;
							VectorCopy(trace.endpos, new_end);
							WP_VehLeadCrosshairVeh(&g_entities[trace.entityNum], new_end, fixed_dir, start, dir);
						}
					}

					//play the weapon's muzzle effect if we have one
					//NOTE: just need MAX_VEHICLE_MUZZLES bits for this... should be cool since it's currently 12 and we're sending it in 16 bits
					muzzles_fired |= 1 << i;

					missile = WP_FireVehicleWeapon(ent, start, dir, veh_weapon, alt_fire, qfalse);
					if (veh_weapon->fHoming)
					{
						//clear the rocket lock entity *after* all muzzles have fired
						clear_rocket_lock_entity = qtrue;
					}
				}

				if (linked_firing)
				{
					//we're linking the weapon, so continue on and fire all appropriate muzzles
					continue;
				}
				//else just firing one
				//take the ammo, set the next muzzle and set the delay on it
				if (numMuzzles > 1)
				{
					//more than one, look for it
					int nextMuzzle = p_veh->weaponStatus[weapon_num].nextMuzzle;
					while (1)
					{
						nextMuzzle++;
						if (nextMuzzle >= MAX_VEHICLE_MUZZLES)
						{
							nextMuzzle = 0;
						}
						if (nextMuzzle == p_veh->weaponStatus[weapon_num].nextMuzzle)
						{
							//WTF?  Wrapped without finding another valid one!
							break;
						}
						if (p_veh->m_pVehicleInfo->weapMuzzle[nextMuzzle] == vehweapon_index)
						{
							//this is the next muzzle for this weapon
							p_veh->weaponStatus[weapon_num].nextMuzzle = nextMuzzle;
							break;
						}
					}
				} //else, just stay on the one we just fired
				//set the delay on the next muzzle
				p_veh->m_iMuzzleWait[p_veh->weaponStatus[weapon_num].nextMuzzle] = level.time + delay;
				//take away the ammo
				p_veh->weaponStatus[weapon_num].ammo -= veh_weapon->iAmmoPerShot;
				//NOTE: in order to send the vehicle's ammo info to the client, we copy the ammo into the first 2 ammo slots on the vehicle NPC's client->ps.ammo array
				if (p_veh->m_pParentEntity && ((gentity_t*)p_veh->m_pParentEntity)->client)
				{
					((gentity_t*)p_veh->m_pParentEntity)->client->ps.ammo[weapon_num] = p_veh->weaponStatus[weapon_num].
						ammo;
				}
				//done!
				//we'll get in here again next frame and try the next muzzle...
				//return;
				goto tryFire;
			}
		}
		//we went through all the muzzles, so apply the cumulative delay and ammo cost
		if (cumulativeAmmo)
		{
			//taking ammo one shot at a time
			//take the ammo
			p_veh->weaponStatus[weapon_num].ammo -= cumulativeAmmo;
			//NOTE: in order to send the vehicle's ammo info to the client, we copy the ammo into the first 2 ammo slots on the vehicle NPC's client->ps.ammo array
			if (p_veh->m_pParentEntity && ((gentity_t*)p_veh->m_pParentEntity)->client)
			{
				((gentity_t*)p_veh->m_pParentEntity)->client->ps.ammo[weapon_num] = p_veh->weaponStatus[weapon_num].ammo;
			}
		}
		if (cumulativeDelay)
		{
			//we linked muzzles so we need to apply the cumulative delay now, to each of the linked muzzles
			for (i = 0; i < MAX_VEHICLE_MUZZLES; i++)
			{
				if (p_veh->m_pVehicleInfo->weapMuzzle[i] != vehweapon_index)
				{
					//this muzzle doesn't match the weapon we're trying to use
					continue;
				}
				//apply the cumulative delay
				p_veh->m_iMuzzleWait[i] = level.time + cumulativeDelay;
			}
		}
	}

tryFire:
	if (clear_rocket_lock_entity)
	{
		//hmm, should probably clear that anytime any weapon fires?
		ent->client->ps.rocketLockIndex = ENTITYNUM_NONE;
		ent->client->ps.rocketLockTime = 0;
		ent->client->ps.rocketTargetTime = 0;
	}

	if (veh_weapon && muzzles_fired > 0)
	{
		G_VehMuzzleFireFX(ent, missile, muzzles_fired);
	}
}

static void G_AddBlasterAttackChainCount(const gentity_t* ent, int amount)
{
	if (ent->r.svFlags & SVF_BOT)
	{
		return;
	}

	if (!walk_check(ent))
	{
		if (amount > 0)
		{
			amount *= 2;
		}
		else
		{
			amount = amount * .5f;
		}
	}

	ent->client->ps.BlasterAttackChainCount += amount;

	if (ent->client->ps.BlasterAttackChainCount < BLASTERMISHAPLEVEL_NONE)
	{
		ent->client->ps.BlasterAttackChainCount = BLASTERMISHAPLEVEL_NONE;
	}
	else if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_MAX)
	{
		ent->client->ps.BlasterAttackChainCount = BLASTERMISHAPLEVEL_MAX;
	}
}

/*
===============
FireWeapon
===============
*/
int BG_EmplacedView(vec3_t base_angles, vec3_t angles, float* new_yaw, float constraint);

static qboolean doesnot_drain_mishap(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_STUN_BATON:
	case WP_MELEE:
	case WP_SABER:
	case WP_THERMAL:
	case WP_TRIP_MINE:
	case WP_DET_PACK:
	case WP_EMPLACED_GUN:
	case WP_TURRET:
		return qtrue;
	default:;
	}
	return qfalse;
}

extern void FireOverheatFail(gentity_t* ent);
extern qboolean PM_ReloadAnim(int anim);
extern qboolean PM_WeponRestAnim(int anim);

void FireWeapon(gentity_t* ent, const qboolean alt_fire)
{
	float alert = 256;

	if (PM_InKnockDown(&ent->client->ps))
	{
		return;
	}

	// track shots taken for accuracy tracking. melee weapons are not tracked.
	if (ent->s.weapon != WP_SABER && ent->s.weapon != WP_STUN_BATON && ent->s.weapon != WP_MELEE)
	{
		if (ent->s.weapon == WP_FLECHETTE)
		{
			ent->client->accuracy_shots += FLECHETTE_SHOTS;
		}
		else
		{
			ent->client->accuracy_shots++;
		}
	}

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		FireVehicleWeapon(ent, alt_fire);
		return;
	}

	if (ent && ent->client && ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FOURTEEN)
	{
		if (!(ent->r.svFlags & SVF_BOT))
		{
			FireOverheatFail(ent);
			return;
		}
	}

	if (ent && ent->client && ent->client->frozenTime > level.time)
	{
		return; //this entity is mind-tricking the current client, so don't render it
	}

	if (ent && ent->client && PM_ReloadAnim(ent->client->ps.torsoAnim))
	{
		return;
	}

	if (ent && ent->client && PM_WeponRestAnim(ent->client->ps.torsoAnim))
	{
		return;
	}

	// set aiming directions
	if (ent && ent->client && ent->s.weapon == WP_EMPLACED_GUN && ent->client->ps.emplacedIndex)
	{
		//if using emplaced then base muzzle point off of gun position/angles
		gentity_t* emp = &g_entities[ent->client->ps.emplacedIndex];

		if (emp->inuse)
		{
			float yaw;
			vec3_t view_ang_cap;

			VectorCopy(ent->client->ps.viewangles, view_ang_cap);
			if (view_ang_cap[PITCH] > 40)
			{
				view_ang_cap[PITCH] = 40;
			}

			const int override = BG_EmplacedView(ent->client->ps.viewangles, emp->s.angles, &yaw,
				emp->s.origin2[0]);

			if (override)
			{
				view_ang_cap[YAW] = yaw;
			}

			AngleVectors(view_ang_cap, forward, vright, up);
		}
		else
		{
			AngleVectors(ent->client->ps.viewangles, forward, vright, up);
		}
	}
	else if (ent->s.number < MAX_CLIENTS &&
		ent->client->ps.m_iVehicleNum && ent->s.weapon == WP_BLASTER)
	{
		//riding a vehicle...with blaster selected
		vec3_t veh_turn_angles;
		const gentity_t* vehEnt = &g_entities[ent->client->ps.m_iVehicleNum];

		if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
		{
			VectorCopy(vehEnt->m_pVehicle->m_vOrientation, veh_turn_angles);
			veh_turn_angles[PITCH] = ent->client->ps.viewangles[PITCH];
		}
		else
		{
			VectorCopy(ent->client->ps.viewangles, veh_turn_angles);
		}
		if (ent->client->pers.cmd.rightmove > 0)
		{
			//shooting to right
			veh_turn_angles[YAW] -= 90.0f;
		}
		else if (ent->client->pers.cmd.rightmove < 0)
		{
			//shooting to left
			veh_turn_angles[YAW] += 90.0f;
		}

		AngleVectors(veh_turn_angles, forward, vright, up);
	}
	else
	{
		AngleVectors(ent->client->ps.viewangles, forward, vright, up);
	}

	calcmuzzlePoint(ent, forward, vright, muzzle);
	if (ent->client->ps.eFlags & EF3_DUAL_WEAPONS)
	{
		calcmuzzlePoint2(ent, forward, vright, muzzle2);
	}

	if (!doesnot_drain_mishap(ent) && ent->client->ps.BlasterAttackChainCount <= BLASTERMISHAPLEVEL_FULL)
	{
		if (ent->s.weapon == WP_REPEATER)
		{
			ent->client->cloneFired++;

			if (ent->client->cloneFired == 2)
			{
				if (ent->client->pers.cmd.forwardmove == 0 && ent->client->pers.cmd.rightmove == 0)
				{
					G_AddBlasterAttackChainCount(ent, BLASTERMISHAPLEVEL_MIN);
				}
				else
				{
					G_AddBlasterAttackChainCount(ent, BLASTERMISHAPLEVEL_TWO);
				}

				ent->client->cloneFired = 0;
			}
		}
		else if (ent->s.weapon == WP_DISRUPTOR)
		{
			G_AddBlasterAttackChainCount(ent, BLASTERMISHAPLEVEL_THREE);
		}
		else
		{
			ent->client->BoltsFired++;

			if (ent->client->BoltsFired == 2)
			{
				if (ent->client->pers.cmd.forwardmove == 0 && ent->client->pers.cmd.rightmove == 0)
				{
					G_AddBlasterAttackChainCount(ent, BLASTERMISHAPLEVEL_MIN);
				}
				else
				{
					G_AddBlasterAttackChainCount(ent, Q_irand(BLASTERMISHAPLEVEL_MIN, BLASTERMISHAPLEVEL_TWO));
					// 1 was not enough
				}

				ent->client->BoltsFired = 0;
			}
		}
	}

	// fire the specific weapon
	switch (ent->s.weapon)
	{
	case WP_STUN_BATON:
		WP_FireStunBaton(ent, alt_fire);
		break;

	case WP_MELEE:
		alert = 0;
		WP_FireMelee(ent, alt_fire);
		break;

	case WP_SABER:
		break;

	case WP_BRYAR_PISTOL:
		WP_FireBryarPistol(ent, alt_fire);
		break;

	case WP_CONCUSSION:
		if (alt_fire)
		{
			WP_FireConcussionAlt(ent);
		}
		else
		{
			WP_FireConcussion(ent);
		}
		break;

	case WP_BRYAR_OLD:
		WP_FireBryarPistolold(ent, alt_fire);
		break;

	case WP_BLASTER:
		WP_FireBlaster(ent, alt_fire);
		break;

	case WP_DISRUPTOR:
		alert = 50;
		WP_FireDisruptor(ent, alt_fire);
		break;

	case WP_BOWCASTER:
		WP_FireBowcaster(ent, alt_fire);
		break;

	case WP_REPEATER:
		WP_FireRepeater(ent, alt_fire);
		break;

	case WP_DEMP2:
		WP_FireDEMP2(ent, alt_fire);
		break;

	case WP_FLECHETTE:
		WP_FireFlechette(ent, alt_fire);
		break;

	case WP_ROCKET_LAUNCHER:
		WP_FireRocket(ent, alt_fire);
		break;

	case WP_THERMAL:
		WP_FireThermalDetonator(ent, alt_fire);
		break;

	case WP_TRIP_MINE:
		alert = 0;
		WP_PlaceLaserTrap(ent, alt_fire);
		break;

	case WP_DET_PACK:
		alert = 0;
		WP_DropDetPack(ent, alt_fire);
		break;

	case WP_EMPLACED_GUN:
		if (ent->client && ent->client->ewebIndex)
		{
			//specially handled by the e-web itself
			break;
		}
		WP_FireEmplaced(ent, alt_fire);
		break;
	default:
		//			assert(!"unknown weapon fire");
		break;
	}
	// We should probably just use this as a default behavior, in special cases, just set alert to false.
	if (alert > 0)
	{
		if (ent->client->ps.groundEntityNum == ENTITYNUM_WORLD //FIXME: check for sand contents type?
			&& ent->s.weapon != WP_STUN_BATON
			&& ent->s.weapon != WP_MELEE
			//&& ent->s.weapon != WP_TUSKEN_STAFF //RAFIXME - Impliment?
			&& ent->s.weapon != WP_THERMAL
			&& ent->s.weapon != WP_TRIP_MINE
			&& ent->s.weapon != WP_DET_PACK)
		{
			//the vibration of the shot carries through your feet into the ground
			AddSoundEvent(ent, muzzle, alert, AEL_DISCOVERED, qfalse, qtrue);
		}
		else
		{
			//an in-air alert
			AddSoundEvent(ent, muzzle, alert, AEL_DISCOVERED, qfalse, qfalse);
		}
		AddSightEvent(ent, muzzle, alert * 2, AEL_DISCOVERED, 20);
	}

	G_LogWeaponFire(ent->s.number, ent->s.weapon);
}

//---------------------------------------------------------
static void WP_FireEmplaced(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs, gunpoint;
	vec3_t right;
	int side;

	if (!ent->client)
	{
		return;
	}

	if (!ent->client->ps.emplacedIndex)
	{
		//shouldn't be using WP_EMPLACED_GUN if we aren't on an emplaced weapon
		return;
	}

	gentity_t* gun = &g_entities[ent->client->ps.emplacedIndex];

	if (!gun->inuse || gun->health <= 0)
	{
		//gun was removed or killed, although we should never hit this check because we should have been forced off it already
		return;
	}

	VectorCopy(gun->s.origin, gunpoint);
	gunpoint[2] += 46;

	AngleVectors(ent->client->ps.viewangles, NULL, right, NULL);

	if (gun->genericValue10)
	{
		//fire out of the right cannon side
		VectorMA(gunpoint, 10.0f, right, gunpoint);
		side = 0;
	}
	else
	{
		//the left
		VectorMA(gunpoint, -10.0f, right, gunpoint);
		side = 1;
	}

	gun->genericValue10 = side;
	G_AddEvent(gun, EV_FIRE_WEAPON, side);

	vectoangles(forward, angs);

	AngleVectors(angs, dir, NULL, NULL);

	WP_FireEmplacedMissile(gun, gunpoint, dir, alt_fire, ent);
}

#define EMPLACED_CANRESPAWN 1

//----------------------------------------------------------

/*QUAKED emplaced_gun (0 0 1) (-30 -20 8) (30 20 60) CANRESPAWN

 count - if CANRESPAWN spawnflag, decides how long it is before gun respawns (in ms)
 constraint - number of degrees gun is constrained from base angles on each side (default 60.0)

 showhealth - set to 1 to show health bar on this entity when crosshair is over it

  teamowner - crosshair shows green for this team, red for opposite team
	0 - none
	1 - red
	2 - blue

  alliedTeam - team that can use this
	0 - any
	1 - red
	2 - blue

  teamnodmg - team that turret does not take damage from or do damage to
	0 - none
	1 - red
	2 - blue
*/

//----------------------------------------------------------
extern qboolean TryHeal(gentity_t* ent, gentity_t* target); //g_utils.c
void emplaced_gun_use(gentity_t* self, gentity_t* other)
{
	vec3_t fwd1, fwd2;
	gentity_t* activator = other;
	const float zoffset = 50;
	vec3_t angles_to_owner;
	vec3_t v_len;

	if (self->health <= 0)
	{
		//gun is destroyed
		return;
	}

	if (self->activator)
	{
		//someone is already using me
		return;
	}

	if (!activator->client)
	{
		return;
	}

	if (activator->client->ps.emplacedTime > level.time)
	{
		//last use attempt still too recent
		return;
	}

	if (activator->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		//don't use if busy doing something else
		return;
	}

	if (activator->client->ps.origin[2] > self->s.origin[2] + zoffset - 8)
	{
		//can't use it from the top
		return;
	}

	if (activator->client->ps.pm_flags & PMF_DUCKED)
	{
		//must be standing
		return;
	}

	if (activator->client->ps.isJediMaster)
	{
		//jm can't use weapons
		return;
	}

	VectorSubtract(self->s.origin, activator->client->ps.origin, v_len);
	const float own_len = VectorLength(v_len);

	if (own_len > 64.0f)
	{
		//must be within 64 units of the gun to use at all
		return;
	}

	// Let's get some direction vectors for the user
	AngleVectors(activator->client->ps.viewangles, fwd1, NULL, NULL);

	// Get the guns direction vector
	AngleVectors(self->pos1, fwd2, NULL, NULL);

	float dot = DotProduct(fwd1, fwd2);

	// Must be reasonably facing the way the gun points ( 110 degrees or so ), otherwise we don't allow to use it.
	if (dot < -0.2f)
	{
		goto tryHeal;
	}

	VectorSubtract(self->s.origin, activator->client->ps.origin, fwd1);
	VectorNormalize(fwd1);

	dot = DotProduct(fwd1, fwd2);

	//check the positioning in relation to the gun as well
	if (dot < 0.6f)
	{
		goto tryHeal;
	}

	self->genericValue1 = 1;

	const int old_weapon = activator->s.weapon;

	// swap the users weapon with the emplaced gun
	activator->client->ps.weapon = self->s.weapon;
	activator->client->ps.weaponstate = WEAPON_READY;
	activator->client->ps.stats[STAT_WEAPONS] |= 1 << WP_EMPLACED_GUN;

	activator->client->ps.emplacedIndex = self->s.number;

	self->s.emplacedOwner = activator->s.number;
	self->s.activeForcePass = NUM_FORCE_POWERS + 1;

	// the gun will track which weapon we used to have
	self->s.weapon = old_weapon;

	//user's new owner becomes the gun ent
	activator->r.ownerNum = self->s.number;
	self->activator = activator;

	VectorSubtract(self->r.currentOrigin, activator->client->ps.origin, angles_to_owner);
	vectoangles(angles_to_owner, angles_to_owner);
	return;

tryHeal: //well, not in the right dir, try healing it instead...
	TryHeal(activator, self);
}

static void emplaced_gun_realuse(gentity_t* self, gentity_t* other, gentity_t* activator)
{
	emplaced_gun_use(self, other);
}

//----------------------------------------------------------
void emplaced_gun_pain(gentity_t* self, gentity_t* attacker, int damage)
{
	self->s.health = self->health;

	if (self->health <= 0)
	{
		//death effect.. for now taken care of on cgame
	}
	else
	{
		//if we have a pain behavior set then use it I guess
		G_ActivateBehavior(self, BSET_PAIN);
	}
}

#define EMPLACED_GUN_HEALTH 800

//----------------------------------------------------------
static void emplaced_gun_update(gentity_t* self)
{
	vec3_t puff_angle;
	float own_len = 0;

	if (self->health < 1 && !self->genericValue5)
	{
		//we are dead, set our respawn delay if we have one
		if (self->spawnflags & EMPLACED_CANRESPAWN)
		{
			self->genericValue5 = level.time + 4000 + self->count;
		}
	}
	else if (self->health < 1 && self->genericValue5 < level.time)
	{
		//we are dead, see if it's time to respawn
		self->s.time = 0;
		self->genericValue4 = 0;
		self->genericValue3 = 0;
		self->health = EMPLACED_GUN_HEALTH * 0.4;
		self->s.health = self->health;
	}

	if (self->genericValue4 && self->genericValue4 < 2 && self->s.time < level.time)
	{
		//we have finished our warning (red flashing) effect, it's time to finish dying
		vec3_t expl_org;

		VectorSet(puff_angle, 0, 0, 1);

		VectorCopy(self->r.currentOrigin, expl_org);
		expl_org[2] += 16;

		//just use the detpack explosion effect
		G_PlayEffect(EFFECT_EXPLOSION_DETPACK, expl_org, puff_angle);

		self->genericValue3 = level.time + Q_irand(2500, 3500);

		g_radius_damage(self->r.currentOrigin, self, self->splashDamage, self->splashRadius, self, NULL, MOD_UNKNOWN);

		self->s.time = -1;

		self->genericValue4 = 2;
	}

	if (self->genericValue3 > level.time)
	{
		//see if we are freshly dead and should be smoking
		if (self->genericValue2 < level.time)
		{
			vec3_t smoke_org;
			//is it time yet to spawn another smoke puff?
			VectorSet(puff_angle, 0, 0, 1);
			VectorCopy(self->r.currentOrigin, smoke_org);

			smoke_org[2] += 60;

			G_PlayEffect(EFFECT_SMOKE, smoke_org, puff_angle);
			self->genericValue2 = level.time + Q_irand(250, 400);
		}
	}

	if (self->activator && self->activator->client && self->activator->inuse)
	{
		//handle updating current user
		vec3_t v_len;
		VectorSubtract(self->s.origin, self->activator->client->ps.origin, v_len);
		own_len = VectorLength(v_len);

		if (!(self->activator->client->pers.cmd.buttons & BUTTON_USE) && self->genericValue1)
		{
			self->genericValue1 = 0;
		}

		if (self->activator->client->pers.cmd.buttons & BUTTON_USE && !self->genericValue1)
		{
			self->activator->client->ps.emplacedIndex = 0;
			self->activator->client->ps.saberHolstered = 0;
			self->nextthink = level.time + 50;
			return;
		}
	}

	if (self->activator && self->activator->client &&
		(!self->activator->inuse || self->activator->client->ps.emplacedIndex != self->s.number || self->genericValue4
			|| own_len > 64))
	{
		//get the user off of me then
		self->activator->client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_EMPLACED_GUN);
		self->activator->client->ps.ammo[weaponData[WP_EMPLACED_GUN].ammoIndex] = 0;

		const int old_weap = self->activator->client->ps.weapon;
		self->activator->client->ps.weapon = self->s.weapon;
		self->s.weapon = old_weap;
		self->activator->r.ownerNum = ENTITYNUM_NONE;
		self->activator->client->ps.emplacedTime = level.time + 1000;
		self->activator->client->ps.emplacedIndex = 0;
		self->activator->client->ps.saberHolstered = 0;
		self->activator = NULL;

		self->s.activeForcePass = 0;
	}
	else if (self->activator && self->activator->client)
	{
		//make sure the user is using the emplaced gun weapon
		self->activator->client->ps.weapon = WP_EMPLACED_GUN;
		self->activator->client->ps.weaponstate = WEAPON_READY;
		if (self->activator->client->ps.ammo[weaponData[WP_EMPLACED_GUN].ammoIndex] < ammoData[weaponData[
			WP_EMPLACED_GUN].ammoIndex].max)
		{
			self->activator->client->ps.ammo[weaponData[WP_EMPLACED_GUN].ammoIndex]++;
		}
	}
	self->nextthink = level.time + 50;
}

//----------------------------------------------------------
static void emplaced_gun_die(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int mod)
{
	//set us up to flash and then explode
	if (self->genericValue4)
	{
		return;
	}

	self->genericValue4 = 1;

	self->s.time = level.time + 3000;

	self->genericValue5 = 0;
}

void SP_emplaced_gun(gentity_t* ent)
{
	const char* name = "models/map_objects/mp/turret_chair.glm";
	vec3_t down;
	trace_t tr;

	//make sure our assets are precached
	register_item(BG_FindItemForWeapon(WP_EMPLACED_GUN));

	ent->r.contents = CONTENTS_SOLID;
	ent->s.solid = SOLID_BBOX;

	ent->genericValue5 = 0;

	VectorSet(ent->r.mins, -30, -20, 8);
	VectorSet(ent->r.maxs, 30, 20, 60);

	VectorCopy(ent->s.origin, down);

	down[2] -= 1024;

	trap->Trace(&tr, ent->s.origin, ent->r.mins, ent->r.maxs, down, ent->s.number, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction != 1 && !tr.allsolid && !tr.startsolid)
	{
		VectorCopy(tr.endpos, ent->s.origin);
	}

	ent->spawnflags |= 4; // deadsolid

	ent->health = EMPLACED_GUN_HEALTH;

	if (ent->spawnflags & EMPLACED_CANRESPAWN)
	{
		//make it somewhat easier to kill if it can respawn
		ent->health *= 0.4;
	}

	ent->maxHealth = ent->health;
	G_ScaleNetHealth(ent);

	ent->genericValue4 = 0;

	ent->takedamage = qtrue;
	ent->pain = emplaced_gun_pain;
	ent->die = emplaced_gun_die;

	// being caught in this thing when it blows would be really bad.
	ent->splashDamage = 80;
	ent->splashRadius = 128;

	// amount of ammo that this little poochie has
	G_SpawnInt("count", "600", &ent->count);

	G_SpawnFloat("constraint", "60", &ent->s.origin2[0]);

	ent->s.modelIndex = G_model_index((char*)name);
	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 110;

	//so the cgame knows for sure that we're an emplaced weapon
	ent->s.weapon = WP_EMPLACED_GUN;

	G_SetOrigin(ent, ent->s.origin);

	// store base angles for later
	VectorCopy(ent->s.angles, ent->pos1);
	VectorCopy(ent->s.angles, ent->r.currentAngles);
	VectorCopy(ent->s.angles, ent->s.apos.trBase);

	ent->think = emplaced_gun_update;
	ent->nextthink = level.time + 50;

	ent->use = emplaced_gun_realuse;

	ent->r.svFlags |= SVF_PLAYER_USABLE;

	ent->s.pos.trType = TR_STATIONARY;

	ent->s.owner = MAX_CLIENTS + 1;
	ent->s.shouldtarget = qtrue;

	trap->LinkEntity((sharedEntity_t*)ent);
}

void SP_emplaced_eweb(gentity_t* ent)
{
	const char* name = "models/map_objects/hoth/eweb_model.glm";
	vec3_t down;
	trace_t tr;

	//make sure our assets are precached
	register_item(BG_FindItemForWeapon(WP_EMPLACED_GUN));

	ent->r.contents = CONTENTS_SOLID;
	ent->s.solid = SOLID_BBOX;

	ent->genericValue5 = 0;

	VectorSet(ent->r.mins, -30, -20, 8);
	VectorSet(ent->r.maxs, 30, 20, 60);

	VectorCopy(ent->s.origin, down);

	down[2] -= 1024;

	trap->Trace(&tr, ent->s.origin, ent->r.mins, ent->r.maxs, down, ent->s.number, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction != 1 && !tr.allsolid && !tr.startsolid)
	{
		VectorCopy(tr.endpos, ent->s.origin);
	}

	ent->spawnflags |= 4; // deadsolid

	ent->health = EMPLACED_GUN_HEALTH;

	if (ent->spawnflags & EMPLACED_CANRESPAWN)
	{
		//make it somewhat easier to kill if it can respawn
		ent->health *= 0.4;
	}

	ent->maxHealth = ent->health;
	G_ScaleNetHealth(ent);

	ent->genericValue4 = 0;

	ent->takedamage = qtrue;
	ent->pain = emplaced_gun_pain;
	ent->die = emplaced_gun_die;

	// being caught in this thing when it blows would be really bad.
	ent->splashDamage = 80;
	ent->splashRadius = 128;

	// amount of ammo that this little poochie has
	G_SpawnInt("count", "600", &ent->count);

	G_SpawnFloat("constraint", "60", &ent->s.origin2[0]);

	ent->s.modelIndex = G_model_index((char*)name);
	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 110;

	//so the cgame knows for sure that we're an emplaced weapon
	ent->s.weapon = WP_EMPLACED_GUN;

	G_SetOrigin(ent, ent->s.origin);

	// store base angles for later
	VectorCopy(ent->s.angles, ent->pos1);
	VectorCopy(ent->s.angles, ent->r.currentAngles);
	VectorCopy(ent->s.angles, ent->s.apos.trBase);

	ent->think = emplaced_gun_update;
	ent->nextthink = level.time + 50;

	ent->use = emplaced_gun_realuse;

	ent->r.svFlags |= SVF_PLAYER_USABLE;

	ent->s.pos.trType = TR_STATIONARY;

	ent->s.owner = MAX_CLIENTS + 1;
	ent->s.shouldtarget = qtrue;

	trap->LinkEntity((sharedEntity_t*)ent);
}