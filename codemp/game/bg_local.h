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

#pragma once

// bg_local.h -- local definitions for the bg (both games) files

#define	MIN_WALK_NORMAL	0.7f		// can't walk on very steep slopes

#define	TIMER_LAND		130
#define	TIMER_GESTURE	(34*66+50)

#define	OVERCLIP		1.001f

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server
typedef struct
{
	vec3_t forward, right, up;
	float frametime;

	int msec;

	qboolean walking;
	qboolean groundPlane;
	trace_t groundTrace;

	float impactSpeed;

	vec3_t previous_origin;
	vec3_t previous_velocity;
	int previous_waterlevel;
} pml_t;

extern pml_t pml;

// movement parameters

extern int c_pmove;

extern int forcePowerNeeded[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS];

extern int forcePowerNeededlevel1[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS];

extern int forcePowerNeededlevel2[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS];

//PM anim utility functions:
qboolean PM_SaberInParry(int move);
qboolean PM_SaberInKnockaway(int move);
qboolean PM_SaberInReflect(int move);
qboolean PM_SaberInStart(int move);
qboolean PM_InSaberAnim(int anim);
qboolean PM_InKnockDown(const playerState_t* ps);
qboolean PM_PainAnim(int anim);
qboolean PM_JumpingAnim(int anim);
qboolean PM_LandingAnim(int anim);
qboolean PM_SpinningAnim(int anim);
qboolean PM_InOnGroundAnim(const int anim);
qboolean PM_InRollComplete(const playerState_t* ps, int anim);

int PM_AnimLength(const animNumber_t anim);

int PM_ReadyPoseForsaber_anim_level(void);
int PM_IdlePoseForsaber_anim_level(void);
int PM_ReadyPoseForsaber_anim_levelBOT(void);
int PM_ReadyPoseForsaber_anim_levelDucked(void);
int PM_BlockingPoseForsaber_anim_levelSingle(void);
int PM_BlockingPoseForsaber_anim_levelDual(void);
int PM_BlockingPoseForsaber_anim_levelStaff(void);

float PM_GroundDistance(void);

saber_moveName_t PM_SaberFlipOverAttackMove(void);
saber_moveName_t PM_SaberJumpForwardAttackMove(void);

void PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce);
void PM_AddTouchEnt(int entityNum);
void PM_AddEvent(int new_event);

qboolean PM_SlideMove(qboolean gravity);
void PM_StepSlideMove(qboolean gravity);

void PM_StartTorsoAnim(int anim);
void PM_ContinueLegsAnim(int anim);
void PM_ForceLegsAnim(int anim);

void PM_BeginWeaponChange(int weapon);
void PM_FinishWeaponChange(void);

void PM_SetAnim(int setAnimParts, int anim, int setAnimFlags);

void PM_WeaponLightsaber(void);
void PM_SetSaberMove(saber_moveName_t new_move);

void PM_SetForceJumpZStart(float value);

void BG_CycleInven(playerState_t* ps, int direction);

qboolean PM_DoKick(void);

qboolean PM_DoSlap(void);
