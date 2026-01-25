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

#pragma once

#define ARMOR_EFFECT_TIME	500

//saberEventFlags
#define	SEF_HITENEMY	0x1		//Hit the enemy
#define	SEF_HITOBJECT	0x2		//Hit some other object
#define	SEF_HITWALL		0x4		//Hit a wall
#define	SEF_PARRIED		0x8		//Parried a saber swipe
#define	SEF_DEFLECTED	0x10	//Deflected a missile or saberInFlight
#define	SEF_BLOCKED		0x20	//Was blocked by a parry
#define	SEF_EVENTS		(SEF_HITENEMY|SEF_HITOBJECT|SEF_HITWALL|SEF_PARRIED|SEF_DEFLECTED|SEF_BLOCKED)
#define	SEF_LOCKED		0x40	//Sabers locked with someone else
#define	SEF_INWATER		0x80	//Saber is in water
#define	SEF_LOCK_WON	0x100	//Won a saberLock
//saberEntityState
#define SES_LEAVING		1
#define SES_HOVERING	1//2
#define SES_RETURNING	1//3
//This is a hack because ATM the saberEntityState is only non-0 if out or 0 if in, and we
//at least want NPCs knowing when their saber is out regardless.

#define JSF_AMBUSH		16	//ambusher Jedi

#define DASH_DRAIN_AMOUNT 15

#define SABER_RADIUS_STANDARD	3.5f
#define	SABER_REFLECT_MISSILE_CONE	0.2f

#define	FORCE_POWER_MAX	100
#define MAX_GRIP_DISTANCE 256
#define MAX_TRICK_DISTANCE 512
#define FORCE_JUMP_CHARGE_TIME 6400//3000.0f
#define GRIP_DRAIN_AMOUNT 30
#define FORCE_LIGHTNING_RADIUS 512
#define MAX_DRAIN_DISTANCE 512

typedef enum forceJump_e
{
	FJ_FORWARD,
	FJ_BACKWARD,
	FJ_RIGHT,
	FJ_LEFT,
	FJ_UP
} forceJump_t;

typedef enum
{
	EVASION_NONE = 0,
	EVASION_PARRY,
	EVASION_DUCK_PARRY,
	EVASION_JUMP_PARRY,
	EVASION_DODGE,
	EVASION_JUMP,
	EVASION_DUCK,
	EVASION_FJUMP,
	EVASION_CARTWHEEL,
	EVASION_OTHER,
	NUM_EVASION_TYPES
} evasionType_t;

#define SABERMINS_X -3.0f//-24.0f
#define SABERMINS_Y -3.0f//-24.0f
#define SABERMINS_Z -3.0f//-8.0f
#define SABERMAXS_X 3.0f//24.0f
#define SABERMAXS_Y 3.0f//24.0f
#define SABERMAXS_Z 3.0f//8.0f
#define	SABER_MIN_THROW_DIST	80.0f

extern int forcePowerNeeded[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS];
extern float forceJumpHeight[NUM_FORCE_POWER_LEVELS];
extern float forceJumpStrength[NUM_FORCE_POWER_LEVELS];
extern float forcePushPullRadius[NUM_FORCE_POWER_LEVELS];

#define SABER_ELASTIC_RATIO          .5

#define SABER_MAXHITDAMAGE		     200
#define SABER_KILHITDAMAGE           150
#define SABER_HIGHITDAMAGE		     100
#define SABER_SJEHITDAMAGE           70
#define SABER_NORHITDAMAGE			 50
#define SABER_MINHITDAMAGE           35
#define SABER_DEBUGTDAMAGE           5

#define DODGE_SABERBLOCK		     15
#define DODGE_LOWDPBOOST		     10
#define DODGE_LOWFPBOOST		     5
#define BOT_PARRYRATE			     100
#define SABBEH_RUN_MODIFIER		     2
#define MELEE_SWING1_DAMAGE		     3
#define MELEE_SWING2_DAMAGE		     5
#define MELEE_SWING_WOOKIE_DAMAGE	 50
#define MELEE_SWING_EXTRA_DAMAGE	 15
#define DODGE_KICKCOST	             10

#define MPCOST_PARRIED				 1
#define MPCOST_PARRIED_ATTACKFAKE	 2
#define MPCOST_PARRYING				 -2
#define MPCOST_PARRYING_ATTACKFAKE	 -4
