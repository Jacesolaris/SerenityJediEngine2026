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

// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

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

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "ghoul2/G2.h"

#ifdef _GAME
#include "g_local.h"
#elif _CGAME
#include "cgame/cg_local.h"
#elif UI_BUILD
#include "ui/ui_local.h"
#endif

#define MAX_WEAPON_CHARGE_TIME 5000

int GROUND_TIME[MAX_GENTITIES];

#ifdef _GAME
extern void G_CheapWeaponFire(int entNum, int ev);
extern qboolean TryGrapple(gentity_t* ent); //g_cmds.c
extern qboolean g_standard_humanoid(gentity_t* self);
#endif // _GAME

extern qboolean BG_FullBodyTauntAnim(int anim);
extern float PM_WalkableGroundDistance(void);
extern qboolean PM_GroundSlideOkay(float z_normal);
extern saberInfo_t* BG_MySaber(int clientNum, int saberNum);
extern qboolean PM_DodgeAnim(int anim);
extern qboolean PM_DodgeHoldAnim(int anim);
extern qboolean PM_BlockAnim(int anim);
extern qboolean PM_BlockHoldAnim(int anim);
extern qboolean PM_BlockDualAnim(int anim);
extern qboolean PM_BlockHoldDualAnim(int anim);
extern qboolean PM_BlockStaffAnim(int anim);
extern qboolean PM_BlockHoldStaffAnim(int anim);
qboolean PM_SaberWalkAnim(int anim);
extern void PM_SetMeleeBlock(void);
extern qboolean PM_MeleeblockAnim(int anim);
extern qboolean PM_ForceAnim(int anim);
extern qboolean BG_SprintAnim(int anim);
qboolean PM_CrouchAnim(int anim);
extern qboolean in_camera;
extern qboolean PM_InRoll(const playerState_t* ps);
extern qboolean PM_InAirKickingAnim(int anim);
extern qboolean PM_InCartwheel(int anim);
extern qboolean PM_InButterfly(int anim);
extern vmCvar_t bg_fighterAltControl;
extern qboolean PM_SaberStanceAnim(int anim);
extern qboolean PM_SaberDrawPutawayAnim(int anim);
extern qboolean PM_ForceJumpingAnim(int anim);
extern void PM_AddBlockFatigue(playerState_t* ps, int fatigue);
extern qboolean BG_FullBodyEmoteAnim(int anim);
extern qboolean BG_FullBodyCowerAnim(int anim);
qboolean PM_StandingidleAnim(int anim);
extern qboolean PM_BoltBlockingAnim(int anim);
extern qboolean PM_SaberInBounce(int move);
extern qboolean PM_InSlapDown(const playerState_t* ps);
qboolean PM_RollingAnim(int anim);
extern qboolean PM_MeleeblockHoldAnim(int anim);
extern int PM_InGrappleMove(int anim);
extern qboolean PM_SaberInMassiveBounce(int anim);
extern qboolean PM_InRollIgnoreTimer(const playerState_t* ps);

pmove_t* pm;
pml_t pml;

bgEntity_t* pm_entSelf = NULL;
bgEntity_t* pm_entVeh = NULL;

qboolean gPMDoSlowFall = qfalse;

qboolean pm_cancelOutZoom = qfalse;

// movement parameters
float pm_stopspeed = 100.0f;
float pm_duckScale = 0.50f;
float pm_swimScale = 0.50f;
float pm_wadeScale = 0.70f;
float pm_ladderScale = 0.7f;
float pm_Laddeeraccelerate = 1.0f;

float pm_vehicleaccelerate = 36.0f;
float pm_accelerate = 10.0f;
float pm_airaccelerate = 1.0f;
float pm_wateraccelerate = 4.0f;
float pm_flyaccelerate = 8.0f;

float pm_friction = 6.0f;
float pm_waterfriction = 1.0f;
float pm_spectatorfriction = 5.0f;

int c_pmove = 0;

float forceSpeedLevels[4] =
{
	1, //rank 0?
	1.25,
	1.5,
	1.75
};

int forcePowerNeeded[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS] =
{
	{
		//nothing should be usable at rank 0..
		999, //FP_HEAL,//instant
		999, //FP_LEVITATION,//hold/duration
		999, //FP_SPEED,//duration
		999, //FP_PUSH,//hold/duration
		999, //FP_PULL,//hold/duration
		999, //FP_TELEPATHY,//instant
		999, //FP_GRIP,//hold/duration
		999, //FP_LIGHTNING,//hold/duration
		999, //FP_RAGE,//duration
		999, //FP_PROTECT,//duration
		999, //FP_ABSORB,//duration
		999, //FP_TEAM_HEAL,//instant
		999, //FP_TEAM_FORCE,//instant
		999, //FP_DRAIN,//hold/duration
		999, //FP_SEE,//duration
		999, //FP_SABER_OFFENSE,
		999, //FP_SABER_DEFENSE,
		999 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		65, //FP_HEAL,//instant //was 25, but that was way too little
		10, //FP_LEVITATION,//hold/duration
		10, //FP_SPEED,//duration
		20, //FP_PUSH,//hold/duration
		20, //FP_PULL,//hold/duration
		20, //FP_TELEPATHY,//instant
		30, //FP_GRIP,//hold/duration
		1, //FP_LIGHTNING,//hold/duration
		50, //FP_RAGE,//duration
		50, //FP_PROTECT,//duration
		50, //FP_ABSORB,//duration
		50, //FP_TEAM_HEAL,//instant
		50, //FP_TEAM_FORCE,//instant
		20, //FP_DRAIN,//hold/duration
		20, //FP_SEE,//duration
		0, //FP_SABER_OFFENSE,
		2, //FP_SABER_DEFENSE,
		20 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		60, //FP_HEAL,//instant
		10, //FP_LEVITATION,//hold/duration
		20, //FP_SPEED,//duration
		20, //FP_PUSH,//hold/duration
		20, //FP_PULL,//hold/duration
		20, //FP_TELEPATHY,//instant
		30, //FP_GRIP,//hold/duration
		1, //FP_LIGHTNING,//hold/duration
		50, //FP_RAGE,//duration
		25, //FP_PROTECT,//duration
		25, //FP_ABSORB,//duration
		33, //FP_TEAM_HEAL,//instant
		33, //FP_TEAM_FORCE,//instant
		20, //FP_DRAIN,//hold/duration
		20, //FP_SEE,//duration
		0, //FP_SABER_OFFENSE,
		1, //FP_SABER_DEFENSE,
		20 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		50, //FP_HEAL,//instant //You get 5 points of health.. for 50 force points!
		10, //FP_LEVITATION,//hold/duration
		50, //FP_SPEED,//duration
		20, //FP_PUSH,//hold/duration
		20, //FP_PULL,//hold/duration
		20, //FP_TELEPATHY,//instant
		60, //FP_GRIP,//hold/duration
		1, //FP_LIGHTNING,//hold/duration
		50, //FP_RAGE,//duration
		10, //FP_PROTECT,//duration
		10, //FP_ABSORB,//duration
		25, //FP_TEAM_HEAL,//instant
		25, //FP_TEAM_FORCE,//instant
		20, //FP_DRAIN,//hold/duration
		20, //FP_SEE,//duration
		0, //FP_SABER_OFFENSE,
		0, //FP_SABER_DEFENSE,
		20 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	}
};

int forcePowerNeededlevel1[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS] =
{
	{
		//nothing should be usable at rank 0..
		999, //FP_HEAL,//instant
		999, //FP_LEVITATION,//hold/duration
		999, //FP_SPEED,//duration
		999, //FP_PUSH,//hold/duration
		999, //FP_PULL,//hold/duration
		999, //FP_TELEPATHY,//instant
		999, //FP_GRIP,//hold/duration
		999, //FP_LIGHTNING,//hold/duration
		999, //FP_RAGE,//duration
		999, //FP_PROTECT,//duration
		999, //FP_ABSORB,//duration
		999, //FP_TEAM_HEAL,//instant
		999, //FP_TEAM_FORCE,//instant
		999, //FP_DRAIN,//hold/duration
		999, //FP_SEE,//duration
		999, //FP_SABER_OFFENSE,
		999, //FP_SABER_DEFENSE,
		999 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		65, //FP_HEAL,//instant //was 25, but that was way too little
		10, //FP_LEVITATION,//hold/duration
		10, //FP_SPEED,//duration
		20, //FP_PUSH,//hold/duration
		20, //FP_PULL,//hold/duration
		20, //FP_TELEPATHY,//instant
		30, //FP_GRIP,//hold/duration
		1, //FP_LIGHTNING,//hold/duration
		50, //FP_RAGE,//duration
		50, //FP_PROTECT,//duration
		50, //FP_ABSORB,//duration
		50, //FP_TEAM_HEAL,//instant
		50, //FP_TEAM_FORCE,//instant
		20, //FP_DRAIN,//hold/duration
		20, //FP_SEE,//duration
		0, //FP_SABER_OFFENSE,
		2, //FP_SABER_DEFENSE,
		20 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		60, //FP_HEAL,//instant
		10, //FP_LEVITATION,//hold/duration
		20, //FP_SPEED,//duration
		20, //FP_PUSH,//hold/duration
		20, //FP_PULL,//hold/duration
		20, //FP_TELEPATHY,//instant
		30, //FP_GRIP,//hold/duration
		1, //FP_LIGHTNING,//hold/duration
		50, //FP_RAGE,//duration
		25, //FP_PROTECT,//duration
		25, //FP_ABSORB,//duration
		33, //FP_TEAM_HEAL,//instant
		33, //FP_TEAM_FORCE,//instant
		20, //FP_DRAIN,//hold/duration
		20, //FP_SEE,//duration
		0, //FP_SABER_OFFENSE,
		1, //FP_SABER_DEFENSE,
		20 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		50, //FP_HEAL,//instant //You get 5 points of health.. for 50 force points!
		10, //FP_LEVITATION,//hold/duration
		50, //FP_SPEED,//duration
		20, //FP_PUSH,//hold/duration
		20, //FP_PULL,//hold/duration
		20, //FP_TELEPATHY,//instant
		60, //FP_GRIP,//hold/duration
		1, //FP_LIGHTNING,//hold/duration
		50, //FP_RAGE,//duration
		10, //FP_PROTECT,//duration
		10, //FP_ABSORB,//duration
		25, //FP_TEAM_HEAL,//instant
		25, //FP_TEAM_FORCE,//instant
		20, //FP_DRAIN,//hold/duration
		20, //FP_SEE,//duration
		0, //FP_SABER_OFFENSE,
		0, //FP_SABER_DEFENSE,
		20 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	}
};

int forcePowerNeededlevel2[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS] =
{
	{
		//nothing should be usable at rank 0..
		999, //FP_HEAL,//instant
		999, //FP_LEVITATION,//hold/duration
		999, //FP_SPEED,//duration
		999, //FP_PUSH,//hold/duration
		999, //FP_PULL,//hold/duration
		999, //FP_TELEPATHY,//instant
		999, //FP_GRIP,//hold/duration
		999, //FP_LIGHTNING,//hold/duration
		999, //FP_RAGE,//duration
		999, //FP_PROTECT,//duration
		999, //FP_ABSORB,//duration
		999, //FP_TEAM_HEAL,//instant
		999, //FP_TEAM_FORCE,//instant
		999, //FP_DRAIN,//hold/duration
		999, //FP_SEE,//duration
		999, //FP_SABER_OFFENSE,
		999, //FP_SABER_DEFENSE,
		999 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		65, //FP_HEAL,//instant //was 25, but that was way too little
		10, //FP_LEVITATION,//hold/duration
		10, //FP_SPEED,//duration
		20, //FP_PUSH,//hold/duration
		20, //FP_PULL,//hold/duration
		20, //FP_TELEPATHY,//instant
		30, //FP_GRIP,//hold/duration
		1, //FP_LIGHTNING,//hold/duration
		50, //FP_RAGE,//duration
		50, //FP_PROTECT,//duration
		50, //FP_ABSORB,//duration
		50, //FP_TEAM_HEAL,//instant
		50, //FP_TEAM_FORCE,//instant
		20, //FP_DRAIN,//hold/duration
		20, //FP_SEE,//duration
		0, //FP_SABER_OFFENSE,
		2, //FP_SABER_DEFENSE,
		20 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		60, //FP_HEAL,//instant
		10, //FP_LEVITATION,//hold/duration
		20, //FP_SPEED,//duration
		20, //FP_PUSH,//hold/duration
		20, //FP_PULL,//hold/duration
		20, //FP_TELEPATHY,//instant
		30, //FP_GRIP,//hold/duration
		1, //FP_LIGHTNING,//hold/duration
		50, //FP_RAGE,//duration
		25, //FP_PROTECT,//duration
		25, //FP_ABSORB,//duration
		33, //FP_TEAM_HEAL,//instant
		33, //FP_TEAM_FORCE,//instant
		20, //FP_DRAIN,//hold/duration
		20, //FP_SEE,//duration
		0, //FP_SABER_OFFENSE,
		1, //FP_SABER_DEFENSE,
		20 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		50, //FP_HEAL,//instant //You get 5 points of health.. for 50 force points!
		10, //FP_LEVITATION,//hold/duration
		50, //FP_SPEED,//duration
		20, //FP_PUSH,//hold/duration
		20, //FP_PULL,//hold/duration
		20, //FP_TELEPATHY,//instant
		60, //FP_GRIP,//hold/duration
		1, //FP_LIGHTNING,//hold/duration
		50, //FP_RAGE,//duration
		10, //FP_PROTECT,//duration
		10, //FP_ABSORB,//duration
		25, //FP_TEAM_HEAL,//instant
		25, //FP_TEAM_FORCE,//instant
		20, //FP_DRAIN,//hold/duration
		20, //FP_SEE,//duration
		0, //FP_SABER_OFFENSE,
		0, //FP_SABER_DEFENSE,
		20 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	}
};

float forceJumpHeight[NUM_FORCE_POWER_LEVELS] =
{
	32, //normal jump (+stepheight+crouchdiff = 66)
	96, //(+stepheight+crouchdiff = 130)
	192, //(+stepheight+crouchdiff = 226)
	384 //(+stepheight+crouchdiff = 418)
};

float forceJumpStrength[NUM_FORCE_POWER_LEVELS] =
{
	JUMP_VELOCITY, //normal jump
	420,
	590,
	840
};

//rww - Get a pointer to the bgEntity by the index
bgEntity_t* PM_BGEntForNum(const int num)
{
	if (!pm)
	{
		assert(!"You cannot call PM_BGEntForNum outside of pm functions!");
		return NULL;
	}

	if (!pm->baseEnt)
	{
		assert(!"Base entity address not set");
		return NULL;
	}

	if (!pm->entSize)
	{
		assert(!"sizeof(ent) is 0, impossible (not set?)");
		return NULL;
	}

	assert(num >= 0 && num < MAX_GENTITIES);

	bgEntity_t* ent = (bgEntity_t*)((byte*)pm->baseEnt + pm->entSize * num);

	return ent;
}

qboolean PM_WalkingAnim(int anim);
qboolean PM_RunningAnim(int anim);

static qboolean PM_CanSetWeaponReadyAnim(void)
{
	if (pm->ps->pm_type != PM_JETPACK
		&& pm->ps->weaponstate != WEAPON_FIRING
		&& (!pm->cmd.forwardmove && !pm->cmd.rightmove
			|| (pm->ps->groundEntityNum == ENTITYNUM_NONE
				|| pm->cmd.buttons & BUTTON_WALKING && pm->cmd.forwardmove > 0)
			&& (PM_WalkingAnim(pm->ps->torsoAnim)
				|| PM_RunningAnim(pm->ps->torsoAnim))))
	{
		return qtrue;
	}

	return qfalse;
}

qboolean BG_SabersOff(const playerState_t* ps)
{
	if (!ps->saberHolstered)
	{
		return qfalse;
	}
	if (ps->fd.saber_anim_levelBase == SS_DUAL
		|| ps->fd.saber_anim_levelBase == SS_STAFF)
	{
		if (ps->saberHolstered < 2)
		{
			return qfalse;
		}
	}
	return qtrue;
}

qboolean BG_KnockDownable(const playerState_t* ps)
{
	if (!ps)
	{
		//just for safety
		return qfalse;
	}

	if (ps->m_iVehicleNum)
	{
		//riding a vehicle, don't knock me down
		return qfalse;
	}

	if (ps->emplacedIndex)
	{
		//using emplaced gun or eweb, can't be knocked down
		return qfalse;
	}

	//ok, I guess?
	return qtrue;
}

//hacky assumption check, assume any client non-humanoid is a rocket trooper
static QINLINE qboolean PM_IsRocketTrooper(void)
{
	return qfalse;
}

static animNumber_t QINLINE PM_GetWeaponReadyAnim(void)
{
	if (pm->cmd.buttons & BUTTON_WALKING && pm->cmd.buttons & BUTTON_BLOCK)
	{
		if (pm->ps->eFlags & EF3_DUAL_WEAPONS && pm->ps->weapon == WP_BRYAR_PISTOL)
		{
			return WeaponAimingAnim2[pm->ps->weapon];
		}
		return WeaponAimingAnim[pm->ps->weapon];
	}
	if (pm->ps->eFlags & EF3_DUAL_WEAPONS && pm->ps->weapon == WP_BRYAR_PISTOL)
	{
		return WeaponReadyAnim2[pm->ps->weapon];
	}
	return WeaponReadyAnim[pm->ps->weapon];
}

int PM_ReadyPoseForsaber_anim_levelBOT(void);

int PM_ReadyPoseForsaber_anim_level(void)
{
	int anim = BOTH_STAND2;
	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	const saberInfo_t* saber2 = BG_MySaber(pm->ps->clientNum, 1);

	if (!pm->ps->saberEntityNum)
	{
		//lost it
		return BOTH_STAND1;
	}

	if (BG_SabersOff(pm->ps))
	{
		return BOTH_STAND1;
	}

	if (PM_BoltBlockingAnim(pm->ps->torsoAnim)
		|| PM_SaberInBounce(pm->ps->saber_move))
	{
		//
	}
	else
	{
		if (saber1
			&& saber1->readyAnim != -1)
		{
			return saber1->readyAnim;
		}

		if (saber2
			&& saber2->readyAnim != -1)
		{
			return saber2->readyAnim;
		}

		if (saber1
			&& saber2
			&& !pm->ps->saberHolstered)
		{
			//dual sabers, both on
			return BOTH_SABERDUAL_STANCE;
		}
		switch (pm->ps->fd.saberAnimLevel)
		{
		case SS_DUAL:
			anim = BOTH_SABERDUAL_STANCE;
			break;
		case SS_STAFF:
			anim = BOTH_SABERSTAFF_STANCE;
			break;
		case SS_FAST:
			anim = BOTH_SABERFAST_STANCE;
			break;
		case SS_TAVION:
			anim = BOTH_SABERTAVION_STANCE;
			break;
		case SS_STRONG:
			anim = BOTH_SABERSLOW_STANCE;
			break;
		case SS_DESANN:
			anim = BOTH_SABERDESANN_STANCE;
			break;
		case SS_MEDIUM:
			anim = BOTH_STAND2;
			break;
		case SS_NONE:
		default:
			anim = BOTH_STAND2;
			break;
		}
	}
	return anim;
}

int PM_IdlePoseForsaber_anim_level(void)
{
	int anim = BOTH_STAND2;

	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);

	const qboolean is_holding_block_button = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;	//Holding Block Button

	const qboolean is_holding_block_button_and_attack = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse; //Holding Block and attack Buttons

	if (!pm->ps->saberEntityNum)
	{
		//lost it
		return BOTH_STAND1;
	}

	if (BG_SabersOff(pm->ps))
	{
		return BOTH_STAND1;
	}

	if (pm->ps->fd.blockPoints <= BLOCKPOINTS_FAIL)
	{
		return BOTH_STAND1;
	}

#ifdef _GAME
	if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
	{
		anim = PM_ReadyPoseForsaber_anim_levelBOT();
	}
#endif
	if (PM_BoltBlockingAnim(pm->ps->torsoAnim)
		|| PM_SaberInBounce(pm->ps->saber_move))
	{
		//
	}
	else
	{
		if (pm->ps->pm_flags & PMF_DUCKED)
		{
			anim = PM_ReadyPoseForsaber_anim_levelDucked();
		}
		else
		{
			if (pm->cmd.buttons & BUTTON_WALKING &&
				PM_SaberWalkAnim(pm->ps->legsAnim) &&
				!is_holding_block_button &&
				pm->cmd.forwardmove > 0 ||
				pm->cmd.rightmove > 0 ||
				pm->cmd.rightmove < 0)
			{
				if (pm->ps->fd.saberAnimLevel == SS_DUAL)
				{
					return BOTH_WALK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_STAFF)
				{
					return BOTH_WALK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_TAVION)
				{
					return BOTH_WALK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_DESANN)
				{
					return BOTH_WALK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_STRONG)
				{
					return BOTH_WALK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_MEDIUM)
				{
					return BOTH_WALK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_FAST)
				{
					return BOTH_WALK1;
				}
				return BOTH_WALK1;
			}
			if (pm->cmd.buttons & BUTTON_WALKING &&
				PM_SaberWalkAnim(pm->ps->legsAnim) &&
				!is_holding_block_button &&
				pm->cmd.forwardmove > 0)
			{
				if (pm->ps->fd.saberAnimLevel == SS_DUAL)
				{
					return BOTH_WALK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_STAFF)
				{
					return BOTH_WALK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_TAVION)
				{
					return BOTH_WALK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_DESANN)
				{
					return BOTH_WALK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_STRONG)
				{
					return BOTH_WALK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_MEDIUM)
				{
					return BOTH_WALK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_FAST)
				{
					return BOTH_WALK1;
				}
				return BOTH_WALK1;
			}
			if (pm->cmd.buttons & BUTTON_WALKING &&
				PM_SaberWalkAnim(pm->ps->legsAnim) &&
				!is_holding_block_button &&
				pm->cmd.forwardmove < 0 ||
				pm->cmd.rightmove > 0 ||
				pm->cmd.rightmove < 0)
			{
				if (pm->ps->fd.saberAnimLevel == SS_DUAL)
				{
					return BOTH_WALKBACK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_STAFF)
				{
					return BOTH_WALKBACK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_TAVION)
				{
					return BOTH_WALKBACK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_DESANN)
				{
					return BOTH_WALKBACK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_STRONG)
				{
					return BOTH_WALKBACK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_MEDIUM)
				{
					return BOTH_WALKBACK1;
				}
				if (pm->ps->fd.saberAnimLevel == SS_FAST)
				{
					return BOTH_WALKBACK1;
				}
				return BOTH_WALKBACK1;
			}

			switch (pm->ps->fd.saberAnimLevel)
			{
			case SS_DUAL:
				if (pm->ps->fd.blockPoints < BLOCKPOINTS_FULL)
				{
					anim = BOTH_SABERDUAL_STANCE_ALT;
				}
				else
				{
					//simple saber attack
					if (is_holding_block_button)
					{
						if (saber1 && saber1->type == SABER_GRIE)
						{
							return BOTH_SABERDUAL_STANCE_GRIEVOUS;
						}
						if (saber1 && saber1->type == SABER_GRIE4)
						{
							anim = BOTH_SABERDUAL_STANCE_GRIEVOUS;
						}
						else
						{
							if (is_holding_block_button_and_attack)
							{
								anim = BOTH_SABERDUAL_STANCE_ALT;
							}
							else
							{
								anim = BOTH_SABERDUAL_STANCE;
							}
						}
					}
					else
					{
						anim = BOTH_SABERDUAL_STANCE_IDLE;
					}
				}
				break;
			case SS_STAFF:
				if (pm->ps->fd.blockPoints < BLOCKPOINTS_FULL)
				{
					anim = BOTH_SABERSTAFF_STANCE_ALT;
				}
				else
				{
					if (is_holding_block_button)
					{
						if (saber1 && (saber1->type == SABER_BACKHAND
							|| saber1->type == SABER_ASBACKHAND)) //saber backhand
						{
							anim = BOTH_SABERBACKHAND_STANCE;
						}
						else
						{
							if (is_holding_block_button_and_attack)
							{
								anim = BOTH_SABERSTAFF_STANCE_ALT;
							}
							else
							{
								anim = BOTH_SABERSTAFF_STANCE;
							}
						}
					}
					else
					{
						anim = BOTH_SABERSTAFF_STANCE_IDLE;
					}
				}
				break;
			case SS_FAST:
				if (is_holding_block_button)
				{
					if (saber1 && saber1->type == SABER_YODA) //yoda
					{
						anim = BOTH_SABERYODA_STANCE;
					}
					else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
					{
						anim = BOTH_SABEROBI_STANCE;
					}
					else if (saber1 && saber1->type == SABER_REY) //saber ray
					{
						anim = BOTH_SABER_REY_STANCE;
					}
					else if (saber1 && saber1->type == SABER_DOOKU) //dooku
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else if (saber1 && saber1->type == SABER_WINDU) //saber windu
					{
						anim = BOTH_SABEREADY_STANCE;
					}
					else
					{
						if (is_holding_block_button_and_attack)
						{
							anim = BOTH_SABEREADY_STANCE;
						}
						else
						{
							anim = BOTH_SABERFAST_STANCE;
						}
					}
				}
				else
				{
					if (pm->ps->fd.blockPoints < BLOCKPOINTS_FULL)
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else
					{
						anim = BOTH_STAND1;
					}
				}
				break;
			case SS_MEDIUM:
				if (is_holding_block_button)
				{
					if (saber1 && saber1->type == SABER_YODA) //yoda
					{
						anim = BOTH_SABERYODA_STANCE;
					}
					else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
					{
						anim = BOTH_SABEROBI_STANCE;
					}
					else if (saber1 && saber1->type == SABER_REY) //saber ray
					{
						anim = BOTH_SABER_REY_STANCE;
					}
					else if (saber1 && saber1->type == SABER_DOOKU) //dooku
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else if (saber1 && saber1->type == SABER_WINDU) //saber windu
					{
						anim = BOTH_SABEREADY_STANCE;
					}
					else
					{
						if (is_holding_block_button_and_attack)
						{
							anim = BOTH_SABEREADY_STANCE;
						}
						else
						{
							anim = BOTH_STAND2;
						}
					}
				}
				else
				{
					if (pm->ps->fd.blockPoints < BLOCKPOINTS_FULL)
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else
					{
						anim = BOTH_STAND1;
					}
				}
				break;
			case SS_STRONG:
				if (is_holding_block_button)
				{
					if (saber1 && saber1->type == SABER_YODA) //yoda
					{
						anim = BOTH_SABERYODA_STANCE;
					}
					else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
					{
						anim = BOTH_SABEROBI_STANCE;
					}
					else if (saber1 && saber1->type == SABER_REY) //saber ray
					{
						anim = BOTH_SABER_REY_STANCE;
					}
					else if (saber1 && saber1->type == SABER_DOOKU) //dooku
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else if (saber1 && saber1->type == SABER_WINDU) //saber windu
					{
						anim = BOTH_SABEREADY_STANCE;
					}
					else
					{
						if (is_holding_block_button_and_attack)
						{
							anim = BOTH_SABEREADY_STANCE;
						}
						else
						{
							anim = BOTH_SABERSLOW_STANCE;
						}
					}
				}
				else
				{
					if (pm->ps->fd.blockPoints < BLOCKPOINTS_FULL)
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else
					{
						anim = BOTH_STAND1;
					}
				}
				break;
			case SS_TAVION:
				if (is_holding_block_button)
				{
					if (saber1 && saber1->type == SABER_YODA) //yoda
					{
						anim = BOTH_SABERYODA_STANCE;
					}
					else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
					{
						anim = BOTH_SABEROBI_STANCE;
					}
					else if (saber1 && saber1->type == SABER_REY) //saber ray
					{
						anim = BOTH_SABER_REY_STANCE;
					}
					else if (saber1 && saber1->type == SABER_DOOKU) //dooku
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else if (saber1 && saber1->type == SABER_WINDU) //saber windu
					{
						anim = BOTH_SABEREADY_STANCE;
					}
					else
					{
						if (is_holding_block_button_and_attack)
						{
							anim = BOTH_SABEREADY_STANCE;
						}
						else
						{
							anim = BOTH_SABERTAVION_STANCE;
						}
					}
				}
				else
				{
					if (pm->ps->fd.blockPoints < BLOCKPOINTS_FULL)
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else
					{
						anim = BOTH_STAND1;
					}
				}
				break;
			case SS_DESANN:
				if (is_holding_block_button)
				{
					if (saber1 && saber1->type == SABER_YODA) //yoda
					{
						anim = BOTH_SABERYODA_STANCE;
					}
					else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
					{
						anim = BOTH_SABEROBI_STANCE;
					}
					else if (saber1 && saber1->type == SABER_REY) //saber ray
					{
						anim = BOTH_SABER_REY_STANCE;
					}
					else if (saber1 && saber1->type == SABER_DOOKU) //dooku
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else if (saber1 && saber1->type == SABER_WINDU) //saber windu
					{
						anim = BOTH_SABEREADY_STANCE;
					}
					else
					{
						if (is_holding_block_button_and_attack)
						{
							anim = BOTH_SABEREADY_STANCE;
						}
						else
						{
							anim = BOTH_SABERDESANN_STANCE;
						}
					}
				}
				else
				{
					if (pm->ps->fd.blockPoints < BLOCKPOINTS_FULL)
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else
					{
						anim = BOTH_STAND1;
					}
				}
				break;
			case SS_NONE:
			default:
				if (is_holding_block_button)
				{
					if (saber1 && saber1->type == SABER_YODA) //yoda
					{
						anim = BOTH_SABERYODA_STANCE;
					}
					else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
					{
						anim = BOTH_SABEROBI_STANCE;
					}
					else if (saber1 && saber1->type == SABER_REY) //saber ray
					{
						anim = BOTH_SABER_REY_STANCE;
					}
					else if (saber1 && saber1->type == SABER_DOOKU) //dooku
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else if (saber1 && saber1->type == SABER_WINDU) //saber windu
					{
						anim = BOTH_SABEREADY_STANCE;
					}
					else
					{
						if (is_holding_block_button_and_attack)
						{
							anim = BOTH_SABEREADY_STANCE;
						}
						else
						{
							anim = BOTH_STAND2;
						}
					}
				}
				else
				{
					if (pm->ps->fd.blockPoints < BLOCKPOINTS_FULL)
					{
						anim = BOTH_SABERSTANCE_STANCE_ALT;
					}
					else
					{
						anim = BOTH_STAND1;
					}
				}
				break;
			}
		}
	}
	return anim;
}

int PM_ReadyPoseForsaber_anim_levelBOT(void)
{
	int anim;

	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	const saberInfo_t* saber2 = BG_MySaber(pm->ps->clientNum, 1);

	if (!pm->ps->saberEntityNum)
	{
		//lost it
		return BOTH_STAND1;
	}

	if (BG_SabersOff(pm->ps))
	{
		return BOTH_STAND1;
	}

	if (saber1
		&& saber1->readyAnim != -1)
	{
		return saber1->readyAnim;
	}

	if (saber2
		&& saber2->readyAnim != -1)
	{
		return saber2->readyAnim;
	}

	if (saber1
		&& saber2
		&& !pm->ps->saberHolstered)
	{
		//dual sabers, both on
		if (saber1 && saber1->type == SABER_GRIE)
		{
			return BOTH_SABERDUAL_STANCE_GRIEVOUS;
		}
		if (saber1 && saber1->type == SABER_GRIE4)
		{
			return BOTH_SABERDUAL_STANCE_GRIEVOUS;
		}
		return BOTH_SABERDUAL_STANCE;
	}

	switch (pm->ps->fd.saberAnimLevel)
	{
	case SS_DUAL:
		if (saber1 && saber1->type == SABER_YODA)
		{
			anim = BOTH_SABERYODA_STANCE;
		}
		else if (saber1 && (saber1->type == SABER_BACKHAND
			|| saber1->type == SABER_ASBACKHAND)) //saber backhand
		{
			anim = BOTH_SABERBACKHAND_STANCE;
		}
		else if (saber1 && saber1->type == SABER_DOOKU)
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
		{
			anim = BOTH_SABEROBI_STANCE;
		}
		else if (saber1 && saber1->type == SABER_REY) //saber backhand
		{
			anim = BOTH_SABER_REY_STANCE;
		}
		else if (saber1 && saber1->type == SABER_GRIE) //saber
		{
			anim = BOTH_SABERDUAL_STANCE_GRIEVOUS;
		}
		else if (saber1 && saber1->type == SABER_GRIE4) //saber
		{
			anim = BOTH_SABERDUAL_STANCE_GRIEVOUS;
		}
		else
		{
			if (pm->ps->fd.blockPoints < BLOCKPOINTS_FULL)
			{
				anim = BOTH_SABERDUAL_STANCE_ALT;
			}
			else
			{
				anim = BOTH_SABERDUAL_STANCE;
			}
		}
		break;
	case SS_STAFF:
		if (saber1 && saber1->type == SABER_YODA)
		{
			anim = BOTH_SABERYODA_STANCE;
		}
		else if (saber1 && (saber1->type == SABER_BACKHAND
			|| saber1->type == SABER_ASBACKHAND)) //saber backhand
		{
			anim = BOTH_SABERBACKHAND_STANCE;
		}
		else if (saber1 && saber1->type == SABER_DOOKU)
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
		{
			anim = BOTH_SABEROBI_STANCE;
		}
		else if (saber1 && saber1->type == SABER_REY) //saber backhand
		{
			anim = BOTH_SABER_REY_STANCE;
		}
		else
		{
			if (pm->ps->fd.blockPoints < BLOCKPOINTS_FULL)
			{
				anim = BOTH_SABERSTAFF_STANCE_ALT;
			}
			else
			{
				anim = BOTH_SABERSTAFF_STANCE;
			}
		}
		break;
	case SS_FAST:
		if (saber1 && saber1->type == SABER_YODA)
		{
			anim = BOTH_SABERYODA_STANCE;
		}
		else if (saber1 && (saber1->type == SABER_BACKHAND
			|| saber1->type == SABER_ASBACKHAND)) //saber backhand
		{
			anim = BOTH_SABERBACKHAND_STANCE;
		}
		else if (saber1 && saber1->type == SABER_DOOKU)
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
		{
			anim = BOTH_SABEROBI_STANCE;
		}
		else if (saber1 && saber1->type == SABER_REY) //saber backhand
		{
			anim = BOTH_SABER_REY_STANCE;
		}
		else
		{
			anim = BOTH_SABERFAST_STANCE;
		}
		break;
	case SS_TAVION:
		if (saber1 && saber1->type == SABER_YODA)
		{
			anim = BOTH_SABERYODA_STANCE;
		}
		else if (saber1 && (saber1->type == SABER_BACKHAND
			|| saber1->type == SABER_ASBACKHAND)) //saber backhand
		{
			anim = BOTH_SABERBACKHAND_STANCE;
		}
		else if (saber1 && saber1->type == SABER_DOOKU)
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
		{
			anim = BOTH_SABEROBI_STANCE;
		}
		else if (saber1 && saber1->type == SABER_REY) //saber backhand
		{
			anim = BOTH_SABER_REY_STANCE;
		}
		else
		{
			anim = BOTH_SABERTAVION_STANCE;
		}
		break;
	case SS_STRONG:
		if (saber1 && saber1->type == SABER_YODA)
		{
			anim = BOTH_SABERYODA_STANCE;
		}
		else if (saber1 && (saber1->type == SABER_BACKHAND
			|| saber1->type == SABER_ASBACKHAND)) //saber backhand
		{
			anim = BOTH_SABERBACKHAND_STANCE;
		}
		else if (saber1 && saber1->type == SABER_DOOKU)
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
		{
			anim = BOTH_SABEROBI_STANCE;
		}
		else if (saber1 && saber1->type == SABER_REY) //saber backhand
		{
			anim = BOTH_SABER_REY_STANCE;
		}
		else
		{
			anim = BOTH_SABERSLOW_STANCE;
		}
		break;
	case SS_DESANN:
		if (saber1 && saber1->type == SABER_YODA)
		{
			anim = BOTH_SABERYODA_STANCE;
		}
		else if (saber1 && (saber1->type == SABER_BACKHAND
			|| saber1->type == SABER_ASBACKHAND)) //saber backhand
		{
			anim = BOTH_SABERBACKHAND_STANCE;
		}
		else if (saber1 && saber1->type == SABER_DOOKU)
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
		{
			anim = BOTH_SABEROBI_STANCE;
		}
		else if (saber1 && saber1->type == SABER_REY) //saber backhand
		{
			anim = BOTH_SABER_REY_STANCE;
		}
		else
		{
			anim = BOTH_SABERDESANN_STANCE;
		}
		break;
	case SS_MEDIUM:
		if (saber1 && saber1->type == SABER_YODA)
		{
			anim = BOTH_SABERYODA_STANCE;
		}
		else if (saber1 && (saber1->type == SABER_BACKHAND
			|| saber1->type == SABER_ASBACKHAND)) //saber backhand
		{
			anim = BOTH_SABERBACKHAND_STANCE;
		}
		else if (saber1 && saber1->type == SABER_DOOKU)
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
		{
			anim = BOTH_SABEREADY_STANCE;
		}
		else if (saber1 && saber1->type == SABER_WINDU) //saber obi
		{
			anim = BOTH_SABEREADY_STANCE;
		}
		else
		{
			anim = BOTH_SABERSTANCE_STANCE;
		}
		break;
	case SS_NONE:
	default:
		if (saber1 && (saber1->type == SABER_BACKHAND
			|| saber1->type == SABER_ASBACKHAND)) //saber backhand
		{
			anim = BOTH_SABERBACKHAND_STANCE;
		}
		else if (saber1 && saber1->type == SABER_YODA)
		{
			anim = BOTH_SABERYODA_STANCE;
		}
		else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_DOOKU)
		{
			anim = BOTH_SABERSTANCE_STANCE_ALT;
		}
		else if (saber1 && saber1->type == SABER_OBIWAN) //saber obi
		{
			anim = BOTH_SABEROBI_STANCE;
		}
		else if (saber1 && saber1->type == SABER_REY) //saber backhand
		{
			anim = BOTH_SABER_REY_STANCE;
		}
		else
		{
			anim = BOTH_SABERSTANCE_STANCE;
		}

		break;
	}
	return anim;
}

int PM_ReadyPoseForsaber_anim_levelDucked(void)
{
	int anim;

	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	const saberInfo_t* saber2 = BG_MySaber(pm->ps->clientNum, 1);

	if (!pm->ps->saberEntityNum)
	{
		//lost it
		return BOTH_STAND1;
	}

	if (BG_SabersOff(pm->ps))
	{
		return BOTH_STAND1;
	}

	if (saber1
		&& saber1->readyAnim != -1)
	{
		return saber1->readyAnim;
	}

	if (saber2
		&& saber2->readyAnim != -1)
	{
		return saber2->readyAnim;
	}

	if (saber1
		&& saber2
		&& !pm->ps->saberHolstered)
	{
		//dual sabers, both on
		if (saber1 && saber1->type == SABER_GRIE)
		{
			return BOTH_SABERDUAL_STANCE_GRIEVOUS;
		}
		if (saber1 && saber1->type == SABER_GRIE4)
		{
			return BOTH_SABERDUAL_STANCE_GRIEVOUS;
		}
		return BOTH_SABERDUALCROUCH;
	}

	switch (pm->ps->fd.saberAnimLevel)
	{
	case SS_DUAL:
		anim = BOTH_SABERDUALCROUCH;
		break;
	case SS_STAFF:
		anim = BOTH_SABERSTAFFCROUCH;
		break;
	case SS_FAST:
		anim = BOTH_SABERSINGLECROUCH;
		break;
	case SS_TAVION:
		anim = BOTH_SABERSINGLECROUCH;
		break;
	case SS_STRONG:
		anim = BOTH_SABERSINGLECROUCH;
		break;
	case SS_MEDIUM:
		anim = BOTH_SABERSINGLECROUCH;
		break;
	case SS_DESANN:
		anim = BOTH_SABERSINGLECROUCH;
		break;
	case SS_NONE:
	default:
		anim = BOTH_SABERSINGLECROUCH;
		break;
	}
	return anim;
}

int PM_BlockingPoseForsaber_anim_levelSingle(void)
{
	int anim = PM_ReadyPoseForsaber_anim_level();

	const qboolean is_holding_block_button_and_attack = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;//Active Blocking

	const signed char forwardmove = pm->cmd.forwardmove;
	const signed char rightmove = pm->cmd.rightmove;

	if (pm->ps->fd.blockPoints < BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (PM_InKnockDown(pm->ps) || PM_InSlapDown(pm->ps) || PM_InRoll(pm->ps))
	{
		return qfalse;
	}

	if (PM_BoltBlockingAnim(pm->ps->torsoAnim)
		|| PM_SaberInBounce(pm->ps->saber_move))
	{
		//
	}
	else
	{
#ifdef _GAME
		if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
		{
			anim = PM_ReadyPoseForsaber_anim_levelBOT();
		}
		else if (pm->cmd.upmove <= 0)
#endif
		{
			if (forwardmove < 0)
			{
				//walking backwards
				if (rightmove < 0)
				{
					//back left
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P1_S1_TL;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
				else if (rightmove > 0)
				{
					//Back right
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P1_S1_TR;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
				else
				{
					//straight back
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P1_S1_T_;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
			}
			else if (forwardmove > 0)
			{
				//walking forwards
				if (rightmove < 0)
				{
					//forwards left
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P1_S1_BL;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
				else if (rightmove > 0)
				{
					//forwards right
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P1_S1_BR;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
				else
				{
					//Top Block
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P1_S1_T_;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
			}
			else
			{
				//STANDING STILL
				if (rightmove < 0)
				{
					//left block
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P1_S1_TL;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
				else if (rightmove > 0)
				{
					//right block
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P1_S1_TR;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
				else
				{
					if (is_holding_block_button_and_attack)
					{
						if (pm->cmd.buttons & BUTTON_WALKING)
						{
							anim = BOTH_P1_S1_T_;
						}
						else
						{
							anim = BOTH_SABEREADY_STANCE;
						}
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
			}
		}
	}
	return anim;
}

int PM_BlockingPoseForsaber_anim_levelDual(void)
{
	//Sets your saber block position based on your current movement commands.
	int anim = PM_ReadyPoseForsaber_anim_level();
	const qboolean is_holding_block_button_and_attack = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
	//Active Blocking

	const signed char forwardmove = pm->cmd.forwardmove;
	const signed char rightmove = pm->cmd.rightmove;

	if (pm->ps->fd.blockPoints < BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (PM_InKnockDown(pm->ps) || PM_InSlapDown(pm->ps) || PM_InRoll(pm->ps))
	{
		return qfalse;
	}

	if (PM_BoltBlockingAnim(pm->ps->torsoAnim)
		|| PM_SaberInBounce(pm->ps->saber_move))
	{
		//
	}
	else if (!pm->ps->fd.saberAnimLevel == SS_DUAL)
	{
		anim = PM_BlockingPoseForsaber_anim_levelSingle();
	}
	else
	{
#ifdef _GAME
		if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
		{
			anim = PM_ReadyPoseForsaber_anim_levelBOT();
		}
		else if (pm->cmd.upmove <= 0)
#endif
		{
			if (forwardmove < 0)
			{
				//walking backwards
				if (rightmove < 0)
				{
					//back left
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P6_S6_TL;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
				else if (rightmove > 0)
				{
					//Back right
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P6_S6_TR;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
				else
				{
					//straight back
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P6_S6_T_;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
			}
			else if (forwardmove > 0)
			{
				//walking forwards
				if (rightmove < 0)
				{
					//forwards left
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P6_S6_BL;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
				else if (rightmove > 0)
				{
					//forwards right
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P6_S6_BR;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
				else
				{
					//Top Block
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P6_S6_T_;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
			}
			else
			{
				//STANDING STILL
				if (rightmove < 0)
				{
					//left block
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P6_S6_TL;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
				else if (rightmove > 0)
				{
					//right block
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P6_S6_TR;
					}
					else
					{
						anim = PM_ReadyPoseForsaber_anim_level();
					}
				}
				else
				{
					if (is_holding_block_button_and_attack)
					{
						if (pm->cmd.buttons & BUTTON_WALKING)
						{
							anim = BOTH_P6_S6_T_;
						}
						else
						{
							anim = BOTH_SABERDUAL_STANCE_ALT;
						}
					}
					else
					{
						if (pm->ps->pm_flags & PMF_DUCKED)
						{
							anim = PM_ReadyPoseForsaber_anim_levelDucked();
						}
						else
						{
							anim = PM_ReadyPoseForsaber_anim_level();
						}
					}
				}
			}
		}
	}
	return anim;
}

int PM_BlockingPoseForsaber_anim_levelStaff(void)
{
	//Sets your saber block position based on your current movement commands.
	int anim = PM_ReadyPoseForsaber_anim_level();
	const qboolean is_holding_block_button_and_attack = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
	//Active Blocking

	const signed char forwardmove = pm->cmd.forwardmove;
	const signed char rightmove = pm->cmd.rightmove;

	if (pm->ps->fd.blockPoints < BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (PM_InKnockDown(pm->ps) || PM_InSlapDown(pm->ps) || PM_InRoll(pm->ps))
	{
		return qfalse;
	}

	if (PM_BoltBlockingAnim(pm->ps->torsoAnim)
		|| PM_SaberInBounce(pm->ps->saber_move))
	{
		//
	}
	else if (!pm->ps->fd.saberAnimLevel == SS_STAFF)
	{
		anim = PM_BlockingPoseForsaber_anim_levelSingle();
	}
	else
	{
#ifdef _GAME
		if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
		{
			anim = PM_ReadyPoseForsaber_anim_levelBOT();
		}
		else if (pm->cmd.upmove <= 0)
#endif
		{
			if (forwardmove < 0)
			{
				//walking backwards
				if (rightmove < 0)
				{
					//back left
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P7_S7_TL;
					}
					else
					{
						anim = BOTH_SABERSTAFF_STANCE;
					}
				}
				else if (rightmove > 0)
				{
					//Back right
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P7_S7_TR;
					}
					else
					{
						anim = BOTH_SABERSTAFF_STANCE;
					}
				}
				else
				{
					//straight back
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P7_S7_T_;
					}
					else
					{
						anim = BOTH_SABERSTAFF_STANCE;
					}
				}
			}
			else if (forwardmove > 0)
			{
				//walking forwards
				if (rightmove < 0)
				{
					//forwards left
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P7_S7_BL;
					}
					else
					{
						anim = BOTH_SABERSTAFF_STANCE;
					}
				}
				else if (rightmove > 0)
				{
					//forwards right
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P7_S7_BR;
					}
					else
					{
						anim = BOTH_SABERSTAFF_STANCE;
					}
				}
				else
				{
					//Top Block
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P7_S7_T_;
					}
					else
					{
						anim = BOTH_SABERSTAFF_STANCE;
					}
				}
			}
			else
			{
				//STANDING STILL
				if (rightmove < 0)
				{
					//left block
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P7_S7_TL;
					}
					else
					{
						anim = BOTH_SABERSTAFF_STANCE;
					}
				}
				else if (rightmove > 0)
				{
					//right block
					if (is_holding_block_button_and_attack)
					{
						anim = BOTH_P7_S7_TR;
					}
					else
					{
						anim = BOTH_SABERSTAFF_STANCE;
					}
				}
				else
				{
					if (is_holding_block_button_and_attack)
					{
						if (pm->cmd.buttons & BUTTON_WALKING)
						{
							anim = BOTH_P7_S7_T_;
						}
						else
						{
							anim = BOTH_SABERSTAFF_STANCE_ALT;
						}
					}
					else
					{
						if (pm->ps->pm_flags & PMF_DUCKED)
						{
							anim = PM_ReadyPoseForsaber_anim_levelDucked();
						}
						else
						{
							anim = BOTH_SABERSTAFF_STANCE;
						}
					}
				}
			}
		}
	}
	return anim;
}

static qboolean PM_DoSlowFall(void)
{
	if ((pm->ps->legsAnim == BOTH_WALL_RUN_RIGHT || pm->ps->legsAnim == BOTH_WALL_RUN_LEFT) && pm->ps->legsTimer > 500)
	{
		return qtrue;
	}

	return qfalse;
}

//begin vehicle functions crudely ported from sp -rww
/*
====================================================================
void pitch_roll_for_slope (edict_t *forwhom, vec3_t *slope, vec3_t storeAngles )

MG

This will adjust the pitch and roll of a monster to match
a given slope - if a non-'0 0 0' slope is passed, it will
use that value, otherwise it will use the ground underneath
the monster.  If it doesn't find a surface, it does nothinh\g
and returns.
====================================================================
*/

static void PM_pitch_roll_for_slope(const bgEntity_t* forwhom, vec3_t pass_slope, vec3_t storeAngles)
{
	vec3_t slope;
	vec3_t nvf, ovf, ovr, new_angles = { 0, 0, 0 };

	//if we don't have a slope, get one
	if (!pass_slope || VectorCompare(vec3_origin, pass_slope))
	{
		vec3_t endspot;
		vec3_t startspot;
		trace_t trace;

		VectorCopy(pm->ps->origin, startspot);
		startspot[2] += pm->mins[2] + 4;
		VectorCopy(startspot, endspot);
		endspot[2] -= 300;
		pm->trace(&trace, pm->ps->origin, vec3_origin, vec3_origin, endspot, forwhom->s.number, MASK_SOLID);

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

	if (forwhom->s.NPC_class == CLASS_VEHICLE)
	{
		//special code for vehicles
		const Vehicle_t* p_veh = forwhom->m_pVehicle;
		vec3_t temp_angles;

		temp_angles[PITCH] = temp_angles[ROLL] = 0;
		temp_angles[YAW] = p_veh->m_vOrientation[YAW];
		AngleVectors(temp_angles, ovf, ovr, NULL);
	}
	else
	{
		AngleVectors(pm->ps->viewangles, ovf, ovr, NULL);
	}

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

	if (storeAngles)
	{
		storeAngles[PITCH] = dot * pitch;
		storeAngles[ROLL] = (1 - Q_fabs(dot)) * pitch * mod;
	}
	else
	{
		pm->ps->viewangles[PITCH] = dot * pitch;
		pm->ps->viewangles[ROLL] = (1 - Q_fabs(dot)) * pitch * mod;
		const float oldmins2 = pm->mins[2];
		pm->mins[2] = -24 + 12 * fabs(pm->ps->viewangles[PITCH]) / 180.0f;
		//FIXME: if it gets bigger, move up
		if (oldmins2 > pm->mins[2])
		{
			//our minimum_mins is now lower, need to move up
			//FIXME: trace?
			pm->ps->origin[2] += oldmins2 - pm->mins[2];
		}
	}
}

#define		FLY_NONE	0
#define		FLY_NORMAL	1
#define		FLY_VEHICLE	2
#define		FLY_HOVER	3
static int pm_flying = FLY_NONE;

static void PM_SetSpecialMoveValues(void)
{
	if (pm->ps->clientNum < MAX_CLIENTS)
	{
		//we know that real players aren't vehs
		pm_flying = FLY_NONE;
		return;
	}

	//default until we decide otherwise
	pm_flying = FLY_NONE;

	const bgEntity_t* pEnt = pm_entSelf;

	if (pEnt)
	{
		if (pm->ps->eFlags2 & EF2_FLYING)
		{
			pm_flying = FLY_NORMAL;
		}
		else if (pEnt->s.NPC_class == CLASS_VEHICLE)
		{
			if (pEnt->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
			{
				pm_flying = FLY_VEHICLE;
			}
			else if (pEnt->m_pVehicle->m_pVehicleInfo->hoverHeight > 0)
			{
				pm_flying = FLY_HOVER;
			}
		}
	}
}

#ifdef _GAME

//extern qboolean JET_Flying(gentity_t *self);
//
//qboolean PM_RocketeersAvoidDangerousFalls(void)
//{
//	bgEntity_t *pEnt;
//
//	pEnt = pm_entSelf;
//
//	if (pEnt->s.NPC_class == == CLASS_BOBAFETT || pEnt->s.NPC_class == == CLASS_ROCKETTROOPER)
//	{
//		if (JET_Flying(pEnt))
//		{
//			if (pEnt->s.NPC_class == == CLASS_BOBAFETT)
//			{
//				pEnt->jetPackTime = level.time + 2000;
//			}
//			else
//			{
//				pEnt->jetPackTime = Q3_INFINITE;
//			}
//		}
//		else
//		{
//			TIMER_Set(pEnt, "jetRecharge", 0);
//			JET_FlyStart(pEnt);
//		}
//		return qtrue;
//	}
//	return qfalse;
//}
extern void G_SoundOnEnt(gentity_t* ent, soundChannel_t channel, const char* sound_path);

static void PM_FallToDeath(gentity_t* self)
{
	if (!self)
	{
		return;
	}

	/*if ( PM_RocketeersAvoidDangerousFalls() )
	{
		return;
	}*/

	// If this is a vehicle, more precisely an animal...
	if (self->client->NPC_class == CLASS_VEHICLE && self->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
	{
		Vehicle_t* p_veh = self->m_pVehicle;
		p_veh->m_pVehicleInfo->EjectAll(p_veh);
	}
	else
	{
		if (BG_HasAnimation(self->localAnimIndex, BOTH_FALLDEATH1))
		{
			PM_SetAnim(SETANIM_LEGS, BOTH_FALLDEATH1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		else
		{
			PM_SetAnim(SETANIM_LEGS, BOTH_DEATH1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}

		G_SoundOnEnt(self, CHAN_VOICE, "*falling1.wav");
	}

	if (self->NPC)
	{
		self->NPC->aiFlags |= NPCAI_DIE_ON_IMPACT;
		self->NPC->nextBStateThink = Q3_INFINITE;
	}
}
#endif

static void PM_SetVehicleAngles(vec3_t normal)
{
	const bgEntity_t* pEnt = pm_entSelf;
	vec3_t vAngles;
	float pitchBias;

	if (!pEnt || pEnt->s.NPC_class != CLASS_VEHICLE)
	{
		return;
	}

	const Vehicle_t* p_veh = pEnt->m_pVehicle;

	//float	curVehicleBankingSpeed;
	float vehicleBankingSpeed = p_veh->m_pVehicleInfo->bankingSpeed * 32.0f * pml.frametime; //0.25f

	if (vehicleBankingSpeed <= 0
		|| p_veh->m_pVehicleInfo->pitchLimit == 0 && p_veh->m_pVehicleInfo->rollLimit == 0)
	{
		//don't bother, this vehicle doesn't bank
		return;
	}

	if (p_veh->m_pVehicleInfo->type == VH_FIGHTER)
	{
		pitchBias = 0.0f;
	}
	else
	{
		//FIXME: gravity does not matter in SPACE!!!
		//center of gravity affects pitch in air/water (FIXME: what about roll?)
		pitchBias = 90.0f * p_veh->m_pVehicleInfo->centerOfGravity[0];
		//if centerOfGravity is all the way back (-1.0f), vehicle pitches up 90 degrees when in air
	}

	VectorClear(vAngles);
	if (pm->waterlevel > 0)
	{
		//in water
		//view pitch has some influence when in water
		//FIXME: take center of gravity into account?
		vAngles[PITCH] += (pm->ps->viewangles[PITCH] - vAngles[PITCH]) * 0.75f + pitchBias * 0.5;
	}
	else if (normal)
	{
		//have a valid surface below me
		PM_pitch_roll_for_slope(pEnt, normal, vAngles);
		if (pml.groundTrace.contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
		{
			//on water
			//view pitch has some influence when on a fluid surface
			//FIXME: take center of gravity into account
			vAngles[PITCH] += (pm->ps->viewangles[PITCH] - vAngles[PITCH]) * 0.5f + pitchBias * 0.5f;
		}
	}
	else
	{
		//in air, let pitch match view...?
		//FIXME: take center of gravity into account
		vAngles[PITCH] = pm->ps->viewangles[PITCH] * 0.5f + pitchBias;
		//don't bank so fast when in the air
		vehicleBankingSpeed *= 0.125f * pml.frametime;
	}
	//NOTE: if angles are flat and we're moving through air (not on ground),
	//		then pitch/bank?
	if (p_veh->m_pVehicleInfo->rollLimit > 0)
	{
		//roll when banking
		vec3_t velocity;
		VectorCopy(pm->ps->velocity, velocity);
		velocity[2] = 0.0f;
		float speed = VectorNormalize(velocity);
		if (speed > 32.0f || speed < -32.0f)
		{
			vec3_t rt, tempVAngles;

			// Magic number fun!  Speed is used for banking, so modulate the speed by a sine wave
			//FIXME: this banks too early
			speed *= sin((150 + pml.frametime) * 0.003);

			// Clamp to prevent harsh rolling
			if (speed > 60)
				speed = 60;

			VectorCopy(p_veh->m_vOrientation, tempVAngles);
			tempVAngles[ROLL] = 0;
			AngleVectors(tempVAngles, NULL, rt, NULL);
			const float dp = DotProduct(velocity, rt);
			const float side = speed * dp;
			vAngles[ROLL] -= side;
		}
	}

	//cap
	if (p_veh->m_pVehicleInfo->pitchLimit != -1)
	{
		if (vAngles[PITCH] > p_veh->m_pVehicleInfo->pitchLimit)
		{
			vAngles[PITCH] = p_veh->m_pVehicleInfo->pitchLimit;
		}
		else if (vAngles[PITCH] < -p_veh->m_pVehicleInfo->pitchLimit)
		{
			vAngles[PITCH] = -p_veh->m_pVehicleInfo->pitchLimit;
		}
	}

	if (vAngles[ROLL] > p_veh->m_pVehicleInfo->rollLimit)
	{
		vAngles[ROLL] = p_veh->m_pVehicleInfo->rollLimit;
	}
	else if (vAngles[ROLL] < -p_veh->m_pVehicleInfo->rollLimit)
	{
		vAngles[ROLL] = -p_veh->m_pVehicleInfo->rollLimit;
	}

	//do it
	for (int i = 0; i < 3; i++)
	{
		if (i == YAW)
		{
			//yawing done elsewhere
			continue;
		}
		if (p_veh->m_vOrientation[i] >= vAngles[i] + vehicleBankingSpeed)
		{
			p_veh->m_vOrientation[i] -= vehicleBankingSpeed;
		}
		else if (p_veh->m_vOrientation[i] <= vAngles[i] - vehicleBankingSpeed)
		{
			p_veh->m_vOrientation[i] += vehicleBankingSpeed;
		}
		else
		{
			p_veh->m_vOrientation[i] = vAngles[i];
		}
	}
}

void BG_VehicleTurnRateForSpeed(const Vehicle_t* p_veh, const float speed, float* mPitchOverride, float* mYawOverride)
{
	if (p_veh && p_veh->m_pVehicleInfo)
	{
		float speedFrac = 1.0f;
		if (p_veh->m_pVehicleInfo->speedDependantTurning)
		{
			if (p_veh->m_LandTrace.fraction >= 1.0f
				|| p_veh->m_LandTrace.plane.normal[2] < MIN_LANDING_SLOPE)
			{
				speedFrac = speed / (p_veh->m_pVehicleInfo->speedMax * 0.75f);
				if (speedFrac < 0.25f)
				{
					speedFrac = 0.25f;
				}
				else if (speedFrac > 1.0f)
				{
					speedFrac = 1.0f;
				}
			}
		}
		if (p_veh->m_pVehicleInfo->mousePitch)
		{
			*mPitchOverride = p_veh->m_pVehicleInfo->mousePitch * speedFrac;
		}
		if (p_veh->m_pVehicleInfo->mouseYaw)
		{
			*mYawOverride = p_veh->m_pVehicleInfo->mouseYaw * speedFrac;
		}
	}
}

// Following couple things don't belong in the DLL namespace!
#ifdef _GAME
#if !defined(MACOS_X) && !defined(__GCC__) && !defined(__GNUC__)
typedef struct gentity_s gentity_t;
#endif
gentity_t* G_PlayEffectID(int fxID, vec3_t org, vec3_t ang);
#endif

static void PM_GroundTraceMissed(void);

static void PM_HoverTrace(void)
{
	vec3_t vAng;
	vec3_t fxAxis[3];

	bgEntity_t* pEnt = pm_entSelf;
	if (!pEnt || pEnt->s.NPC_class != CLASS_VEHICLE)
	{
		return;
	}

	Vehicle_t* p_veh = pEnt->m_pVehicle;
	const float hoverHeight = p_veh->m_pVehicleInfo->hoverHeight;
	trace_t* trace = &pml.groundTrace;

	pml.groundPlane = qfalse;

	//relativeWaterLevel = (pm->ps->waterheight - (pm->ps->origin[2]+pm->minimum_mins[2]));
	const float relativeWaterLevel = pm->waterlevel; //I.. guess this works
	if (pm->waterlevel && relativeWaterLevel >= 0)
	{
		//in water
		if (p_veh->m_pVehicleInfo->bouyancy <= 0.0f)
		{
			//sink like a rock
		}
		else
		{
			//rise up
			const float floatHeight = p_veh->m_pVehicleInfo->bouyancy * ((pm->maxs[2] - pm->mins[2]) * 0.5f) -
				hoverHeight * 0.5f; //1.0f should make you float half-in, half-out of water
			if (relativeWaterLevel > floatHeight)
			{
				//too low, should rise up
				pm->ps->velocity[2] += (relativeWaterLevel - floatHeight) * p_veh->m_fTimeModifier;
			}
		}
		if (pm->waterlevel <= 1)
		{
			//part of us is sticking out of water
			if (fabs(pm->ps->velocity[0]) + fabs(pm->ps->velocity[1]) > 100)
			{
				//moving at a decent speed
				if (Q_irand(pml.frametime, 100) >= 50)
				{
					//splash
					vec3_t wakeOrg;

					vAng[PITCH] = vAng[ROLL] = 0;
					vAng[YAW] = p_veh->m_vOrientation[YAW];
					AngleVectors(vAng, fxAxis[2], fxAxis[1], fxAxis[0]);
					VectorCopy(pm->ps->origin, wakeOrg);
					//wakeOrg[2] = pm->ps->waterheight;
					if (pm->waterlevel >= 2)
					{
						wakeOrg[2] = pm->ps->origin[2] + 16;
					}
					else
					{
						wakeOrg[2] = pm->ps->origin[2];
					}
#ifdef _GAME //yeah, this is kind of crappy and makes no use of prediction whatsoever
					if (p_veh->m_pVehicleInfo->iWakeFX)
					{
						G_AddEvent((gentity_t*)pEnt, EV_PLAY_EFFECT_ID, p_veh->m_pVehicleInfo->iWakeFX);
					}
#endif
				}
			}
		}
	}
	else
	{
		vec3_t point;
		const float minNormal = p_veh->m_pVehicleInfo->maxSlope;

		point[0] = pm->ps->origin[0];
		point[1] = pm->ps->origin[1];
		point[2] = pm->ps->origin[2] - hoverHeight;

		//FIXME: check for water, too?  If over water, go slower and make wave effect
		//		If *in* water, go really slow and use bouyancy stat to determine how far below surface to float

		//NOTE: if bouyancy is 2.0f or higher, you float over water like it's solid ground.
		//		if it's 1.0f, you sink halfway into water.  If it's 0, you sink...
		int traceContents = pm->tracemask;
		if (p_veh->m_pVehicleInfo->bouyancy >= 2.0f)
		{
			//sit on water
			traceContents |= CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA;
		}
		pm->trace(trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, traceContents);
		if (trace->plane.normal[0] > 0.5f || trace->plane.normal[0] < -0.5f ||
			trace->plane.normal[1] > 0.5f || trace->plane.normal[1] < -0.5f)
		{
			//steep slanted hill, don't go up it.
			float d = fabs(trace->plane.normal[0]);
			const float e = fabs(trace->plane.normal[1]);
			if (e > d)
			{
				d = e;
			}
			pm->ps->velocity[2] = -300.0f * d;
		}
		else if (trace->plane.normal[2] >= minNormal)
		{
			//not a steep slope, so push us up
			if (trace->fraction < 1.0f)
			{
				//push up off ground
				const float hoverForce = p_veh->m_pVehicleInfo->hoverStrength;
				if (trace->fraction > 0.5f)
				{
					pm->ps->velocity[2] += (1.0f - trace->fraction) * hoverForce * p_veh->m_fTimeModifier;
				}
				else
				{
					pm->ps->velocity[2] += (0.5f - trace->fraction * trace->fraction) * hoverForce * 2.0f * p_veh->
						m_fTimeModifier;
				}
				if (trace->contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
				{
					//hovering on water, make a spash if moving
					if (fabs(pm->ps->velocity[0]) + fabs(pm->ps->velocity[1]) > 100)
					{
						//moving at a decent speed
						if (Q_irand(pml.frametime, 100) >= 50)
						{
							//splash
							vAng[PITCH] = vAng[ROLL] = 0;
							vAng[YAW] = p_veh->m_vOrientation[YAW];
							AngleVectors(vAng, fxAxis[2], fxAxis[1], fxAxis[0]);
#ifdef _GAME
							if (p_veh->m_pVehicleInfo->iWakeFX)
							{
								G_PlayEffectID(p_veh->m_pVehicleInfo->iWakeFX, trace->endpos, fxAxis[0]);
							}
#endif
						}
					}
				}
				pml.groundPlane = qtrue;
			}
		}
	}
	if (pml.groundPlane)
	{
		PM_SetVehicleAngles(pml.groundTrace.plane.normal);
		// We're on the ground.
		p_veh->m_ulFlags &= ~VEH_FLYING;

		p_veh->m_vAngularVelocity = 0.0f;
	}
	else
	{
		PM_SetVehicleAngles(NULL);
		// We're flying in the air.
		p_veh->m_ulFlags |= VEH_FLYING;
		//groundTrace

		if (p_veh->m_vAngularVelocity == 0.0f)
		{
			p_veh->m_vAngularVelocity = p_veh->m_vOrientation[YAW] - p_veh->m_vPrevOrientation[YAW];
			if (p_veh->m_vAngularVelocity < -15.0f)
			{
				p_veh->m_vAngularVelocity = -15.0f;
			}
			if (p_veh->m_vAngularVelocity > 15.0f)
			{
				p_veh->m_vAngularVelocity = 15.0f;
			}
		}
		if (p_veh->m_vAngularVelocity > 0.0f)
		{
			p_veh->m_vAngularVelocity -= pml.frametime;
			if (p_veh->m_vAngularVelocity < 0.0f)
			{
				p_veh->m_vAngularVelocity = 0.0f;
			}
		}
		else if (p_veh->m_vAngularVelocity < 0.0f)
		{
			p_veh->m_vAngularVelocity += pml.frametime;
			if (p_veh->m_vAngularVelocity > 0.0f)
			{
				p_veh->m_vAngularVelocity = 0.0f;
			}
		}
	}
	PM_GroundTraceMissed();
}

//end vehicle functions crudely ported from sp -rww

/*
=============
PM_SetWaterLevelAtPoint
=============
*/
#ifdef _GAME
static void PM_SetWaterLevelAtPoint(vec3_t org, int* waterlevel, int* watertype)
{
	vec3_t point;

	//
	// get waterlevel, accounting for ducking
	//
	*waterlevel = 0;
	*watertype = 0;

	point[0] = org[0];
	point[1] = org[1];
	point[2] = org[2] + DEFAULT_MINS_2 + 1;
	int cont = pm->pointcontents(point, pm->ps->clientNum);

	if (cont & (MASK_WATER | CONTENTS_LADDER))
	{
		const int sample2 = pm->ps->viewheight - DEFAULT_MINS_2;
		const int sample1 = sample2 / 2;

		*watertype = cont;
		*waterlevel = 1;
		point[2] = org[2] + DEFAULT_MINS_2 + sample1;
		cont = pm->pointcontents(point, pm->ps->clientNum);
		if (cont & (MASK_WATER | CONTENTS_LADDER))
		{
			*waterlevel = 2;
			point[2] = org[2] + DEFAULT_MINS_2 + sample2;
			cont = pm->pointcontents(point, pm->ps->clientNum);
			if (cont & (MASK_WATER | CONTENTS_LADDER))
			{
				*waterlevel = 3;
			}
		}
	}
}
#endif

/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent(const int new_event)
{
	BG_AddPredictableEventToPlayerstate(new_event, 0, pm->ps);
}

void PM_AddEventWithParm(const int new_event, const int parm)
{
	BG_AddPredictableEventToPlayerstate(new_event, parm, pm->ps);
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt(const int entityNum)
{
	if (entityNum == ENTITYNUM_WORLD)
	{
		return;
	}
	if (pm->numtouch >= MAXTOUCH)
	{
		return;
	}

	// see if it is already added
	for (int i = 0; i < pm->numtouch; i++)
	{
		if (pm->touchents[i] == entityNum)
		{
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch++] = entityNum;
}

/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, const float overbounce)
{
	if (pm->ps->pm_flags & PMF_STUCK_TO_WALL)
	{
		//no sliding!
		VectorCopy(in, out);
		return;
	}
	const float oldInZ = in[2];

	float backoff = DotProduct(in, normal);

	if (backoff < 0)
	{
		backoff *= overbounce;
	}
	else
	{
		backoff /= overbounce;
	}

	for (int i = 0; i < 3; i++)
	{
		const float change = normal[i] * backoff;
		out[i] = in[i] - change;
	}
	if (pm->stepSlideFix)
	{
		if (pm->ps->clientNum < MAX_CLIENTS //normal player
			&& pm->ps->groundEntityNum != ENTITYNUM_NONE //on the ground
			&& normal[2] < MIN_WALK_NORMAL) //sliding against a steep slope
		{
			//if walking on the ground, don't slide up slopes that are too steep to walk on
			out[2] = oldInZ;
		}
	}
}

/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction(void)
{
	vec3_t vec;
	float control;
	float friction = pm_friction;
	const bgEntity_t* pEnt = NULL;

	float* vel = pm->ps->velocity;

	VectorCopy(vel, vec);
	if (pml.walking)
	{
		vec[2] = 0; // ignore slope movement
	}

	const float speed = VectorLength(vec);
	if (speed < 1)
	{
		vel[0] = 0;
		vel[1] = 0; // allow sinking underwater
		if (pm->ps->pm_type == PM_SPECTATOR)
		{
			vel[2] = 0;
		}
		return;
	}

	float drop = 0;

	if (pm->ps->clientNum >= MAX_CLIENTS)
	{
		pEnt = pm_entSelf;
	}

	// apply ground friction, even if on ladder
	if (pm_flying != FLY_VEHICLE &&
		pEnt &&
		pEnt->s.NPC_class == CLASS_VEHICLE &&
		pEnt->m_pVehicle &&
		pEnt->m_pVehicle->m_pVehicleInfo->type != VH_ANIMAL &&
		pEnt->m_pVehicle->m_pVehicleInfo->type != VH_WALKER &&
		pEnt->m_pVehicle->m_pVehicleInfo->friction)
	{
		const float friction1 = pEnt->m_pVehicle->m_pVehicleInfo->friction;
		if (!(pm->ps->pm_flags & PMF_TIME_KNOCKBACK) && !(pm->ps->pm_flags & PMF_TIME_NOFRICTION))
		{
			control = speed < pm_stopspeed ? pm_stopspeed : speed;
			drop += control * friction1 * pml.frametime;
		}
	}
	else if (pm_flying != FLY_NORMAL && pm_flying != FLY_VEHICLE)
	{
		// apply ground friction
		if (pm->waterlevel <= 1)
		{
			if (pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK))
			{
				// if getting knocked back, no friction
				if (!(pm->ps->pm_flags & PMF_TIME_KNOCKBACK) && !(pm->ps->pm_flags & PMF_TIME_NOFRICTION))
				{
					if (pm->ps->legsAnim == BOTH_FORCELONGLEAP_START
						|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK
						|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK2
						|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_LAND
						|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_LAND2)
					{
						//super forward jump
						if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
						{
							//not in air
							if (pm->cmd.forwardmove < 0)
							{
								//trying to hold back some
								friction *= 0.5f; //0.25f;
							}
							else
							{
								//free slide
								friction *= 0.2f; //0.1f;
							}
							pm->cmd.forwardmove = pm->cmd.rightmove = 0;
							if (pml.groundPlane && pm->ps->legsAnim == BOTH_FORCELONGLEAP_LAND || pm->ps->legsAnim ==
								BOTH_FORCELONGLEAP_LAND2)
							{
#ifdef _GAME
								G_PlayEffect(EFFECT_LANDING_SAND, pml.groundTrace.endpos, pml.groundTrace.plane.normal);
#endif
							}
						}
					}
					control = speed < pm_stopspeed ? pm_stopspeed : speed;
					drop += control * pm_friction * pml.frametime;
				}
			}
		}
	}
	else if (pm->ps->clientNum < MAX_CLIENTS && pm->ps->eFlags2 & EF2_FLYING)
	{
		drop += speed * pm_waterfriction * pml.frametime;
	}

	if (pm_flying == FLY_VEHICLE)
	{
		if (!(pm->ps->pm_flags & PMF_TIME_KNOCKBACK) && !(pm->ps->pm_flags & PMF_TIME_NOFRICTION))
		{
			control = speed < pm_stopspeed ? pm_stopspeed : speed;
			drop += control * pm_friction * pml.frametime;
		}
	}

	// apply water friction even if just wading
	if (pm->waterlevel)
	{
		drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;
	}
	// If on a client then there is no friction
	else if (pm->ps->groundEntityNum < MAX_CLIENTS)
	{
		drop = 0;
	}

	if (pm->ps->pm_type == PM_SPECTATOR || pm->ps->pm_type == PM_FLOAT)
	{
		if (pm->ps->pm_type == PM_FLOAT)
		{
			//almost no friction while floating
			drop += speed * 0.1 * pml.frametime;
		}
		else
		{
			drop += speed * pm_spectatorfriction * pml.frametime;
		}
	}

	// scale the velocity
	float newspeed = speed - drop;
	if (newspeed < 0)
	{
		newspeed = 0;
	}
	newspeed /= speed;

	VectorScale(vel, newspeed, vel);
}

/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate(vec3_t wishdir, const float wishspeed, const float accel)
{
	if (pm->gametype != GT_SIEGE
		|| pm->ps->m_iVehicleNum
		|| pm->ps->clientNum >= MAX_CLIENTS
		|| pm->ps->pm_type != PM_NORMAL)
	{
		float accelspeed;

		const float currentspeed = DotProduct(pm->ps->velocity, wishdir);
		const float addspeed = wishspeed - currentspeed;
		if (addspeed <= 0 && pm->ps->clientNum < MAX_CLIENTS)
		{
			return;
		}

		if (addspeed < 0)
		{
			accelspeed = -accel * pml.frametime * wishspeed;
			if (accelspeed < addspeed)
			{
				accelspeed = addspeed;
			}
		}
		else
		{
			accelspeed = accel * pml.frametime * wishspeed;
			if (accelspeed > addspeed)
			{
				accelspeed = addspeed;
			}
		}

		for (int i = 0; i < 3; i++)
		{
			pm->ps->velocity[i] += accelspeed * wishdir[i];
		}
	}
	else
	{
		//use the proper way for siege
		vec3_t wish_velocity;
		vec3_t push_dir;

		VectorScale(wishdir, wishspeed, wish_velocity);
		VectorSubtract(wish_velocity, pm->ps->velocity, push_dir);
		const float push_len = VectorNormalize(push_dir);

		float canPush = accel * pml.frametime * wishspeed;
		if (canPush > push_len)
		{
			canPush = push_len;
		}

		VectorMA(pm->ps->velocity, canPush, push_dir, pm->ps->velocity);
	}
}

/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale(const usercmd_t* cmd)
{
	const int umove = 0; //cmd->upmove;
	//don't factor upmove into scaling speed

	int max = abs(cmd->forwardmove);
	if (abs(cmd->rightmove) > max)
	{
		max = abs(cmd->rightmove);
	}
	if (abs(umove) > max)
	{
		max = abs(umove);
	}
	if (!max)
	{
		return 0;
	}

	const float total = sqrt(
		(float)(cmd->forwardmove * cmd->forwardmove + cmd->rightmove * cmd->rightmove + umove * umove));
	const float scale = pm->ps->speed * max / (127.0 * total);

	return scale;
}

/*
================
PM_SetMovementDir

Determine the rotation of the legs reletive
to the facing dir
================
*/
static void PM_SetMovementDir(void)
{
	if (pm->cmd.forwardmove || pm->cmd.rightmove)
	{
		if (pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0)
		{
			pm->ps->movementDir = 0;
		}
		else if (pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0)
		{
			pm->ps->movementDir = 1;
		}
		else if (pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0)
		{
			pm->ps->movementDir = 2;
		}
		else if (pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0)
		{
			pm->ps->movementDir = 3;
		}
		else if (pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0)
		{
			pm->ps->movementDir = 4;
		}
		else if (pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0)
		{
			pm->ps->movementDir = 5;
		}
		else if (pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0)
		{
			pm->ps->movementDir = 6;
		}
		else if (pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0)
		{
			pm->ps->movementDir = 7;
		}
	}
	else
	{
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if (pm->ps->movementDir == 2)
		{
			pm->ps->movementDir = 1;
		}
		else if (pm->ps->movementDir == 6)
		{
			pm->ps->movementDir = 7;
		}
	}
}

#define METROID_JUMP 1

qboolean PM_InReboundJump(const int anim)
{
	switch (anim)
	{
	case BOTH_FORCEWALLREBOUND_FORWARD:
	case BOTH_FORCEWALLREBOUND_LEFT:
	case BOTH_FORCEWALLREBOUND_BACK:
	case BOTH_FORCEWALLREBOUND_RIGHT:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean PM_InReboundHold(const int anim)
{
	switch (anim)
	{
	case BOTH_FORCEWALLHOLD_FORWARD:
	case BOTH_FORCEWALLHOLD_LEFT:
	case BOTH_FORCEWALLHOLD_BACK:
	case BOTH_FORCEWALLHOLD_RIGHT:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean PM_InReboundRelease(const int anim)
{
	switch (anim)
	{
	case BOTH_FORCEWALLRELEASE_FORWARD:
	case BOTH_FORCEWALLRELEASE_LEFT:
	case BOTH_FORCEWALLRELEASE_BACK:
	case BOTH_FORCEWALLRELEASE_RIGHT:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean PM_InBackFlip(const int anim)
{
	switch (anim)
	{
	case BOTH_FLIP_BACK1:
	case BOTH_FLIP_BACK2:
	case BOTH_FLIP_BACK3:
	case BOTH_ALORA_FLIP_B:
		return qtrue;
	default:;
	}
	return qfalse;
}

static qboolean PM_ForceJumpingUp(void)
{
	if (!(pm->ps->fd.forcePowersActive & 1 << FP_LEVITATION) && pm->ps->fd.forceJumpCharge)
	{
		//already jumped and let go
		return qfalse;
	}

	if (PM_InSpecialJump(pm->ps->legsAnim))
	{
		return qfalse;
	}

	if (PM_SaberInSpecial(pm->ps->saber_move))
	{
		return qfalse;
	}

	if (pm_saber_in_special_attack(pm->ps->legsAnim))
	{
		return qfalse;
	}

	if (BG_HasYsalamiri(pm->gametype, pm->ps))
	{
		return qfalse;
	}

	if (!BG_CanUseFPNow(pm->gametype, pm->ps, pm->cmd.serverTime, FP_LEVITATION))
	{
		return qfalse;
	}

	if (pm->ps->groundEntityNum == ENTITYNUM_NONE && //in air
		pm->ps->pm_flags & PMF_JUMP_HELD && //jumped
		pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 && //force-jump capable
		pm->ps->velocity[2] > 0) //going up
	{
		return qtrue;
	}
	return qfalse;
}

static void PM_JumpForDir(void)
{
	int anim;
	if (pm->cmd.forwardmove > 0)
	{
		if (pm->ps->weapon == WP_SABER && !pm->ps->saberHolstered) //saber out
		{
			anim = BOTH_JUMP2;
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}
		else
		{
			anim = BOTH_JUMP1;
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}
	}
	else if (pm->cmd.forwardmove < 0)
	{
		anim = BOTH_JUMPBACK1;
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}
	else if (pm->cmd.rightmove > 0)
	{
		anim = BOTH_JUMPRIGHT1;
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else if (pm->cmd.rightmove < 0)
	{
		anim = BOTH_JUMPLEFT1;
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else
	{
		if (pm->ps->weapon == WP_SABER && !pm->ps->saberHolstered) //saber out
		{
			anim = BOTH_JUMP2;
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}
		else
		{
			anim = BOTH_JUMP1;
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}
	}
	if (!BG_InDeathAnim(pm->ps->legsAnim))
	{
		PM_SetAnim(SETANIM_LEGS, anim, SETANIM_FLAG_OVERRIDE);
	}
}

static void PM_SetPMViewAngle(playerState_t* ps, vec3_t angle, const usercmd_t* ucmd)
{
	for (int i = 0; i < 3; i++)
	{
		const int cmdAngle = ANGLE2SHORT(angle[i]);
		ps->delta_angles[i] = cmdAngle - ucmd->angles[i];
	}
	VectorCopy(angle, ps->viewangles);
}

static qboolean PM_AdjustAngleForWallRun(playerState_t* ps, usercmd_t* ucmd, const qboolean doMove)
{
	if ((ps->legsAnim == BOTH_WALL_RUN_RIGHT || ps->legsAnim == BOTH_WALL_RUN_LEFT) && ps->legsTimer > 500)
	{
		//wall-running and not at end of anim
		//stick to wall, if there is one
		vec3_t fwd, rt, traceTo, mins, maxs, fwdAngles;
		trace_t trace;
		float dist, yawAdjust;

		VectorSet(mins, -15, -15, 0);
		VectorSet(maxs, 15, 15, 24);
		VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);

		AngleVectors(fwdAngles, fwd, rt, NULL);
		if (ps->legsAnim == BOTH_WALL_RUN_RIGHT)
		{
			dist = 128;
			yawAdjust = -90;
		}
		else
		{
			dist = -128;
			yawAdjust = 90;
		}
		VectorMA(ps->origin, dist, rt, traceTo);

		pm->trace(&trace, ps->origin, mins, maxs, traceTo, ps->clientNum, MASK_PLAYERSOLID);

		if (trace.fraction < 1.0f
			&& (trace.plane.normal[2] >= 0.0f && trace.plane.normal[2] <= 0.4f))
			//&& ent->client->ps.groundEntityNum == ENTITYNUM_NONE )
		{
			trace_t trace2;
			vec3_t traceTo2;
			vec3_t wallRunFwd, wallRunAngles;

			VectorClear(wallRunAngles);
			wallRunAngles[YAW] = vectoyaw(trace.plane.normal) + yawAdjust;
			AngleVectors(wallRunAngles, wallRunFwd, NULL, NULL);

			VectorMA(pm->ps->origin, 32, wallRunFwd, traceTo2);
			pm->trace(&trace2, pm->ps->origin, mins, maxs, traceTo2, pm->ps->clientNum, MASK_PLAYERSOLID);
			if (trace2.fraction < 1.0f && DotProduct(trace2.plane.normal, wallRunFwd) <= -0.999f)
			{
				//wall we can't run on in front of us
				trace.fraction = 1.0f; //just a way to get it to kick us off the wall below
			}
		}

		if (trace.fraction < 1.0f
			&& (trace.plane.normal[2] >= 0.0f && trace.plane.normal[2] <= 0.4f/*MAX_WALL_RUN_Z_NORMAL*/))
		{
			//still a wall there
			if (ps->legsAnim == BOTH_WALL_RUN_RIGHT)
			{
				ucmd->rightmove = 127;
			}
			else
			{
				ucmd->rightmove = -127;
			}
			if (ucmd->upmove < 0)
			{
				ucmd->upmove = 0;
			}
			//make me face perpendicular to the wall
			ps->viewangles[YAW] = vectoyaw(trace.plane.normal) + yawAdjust;

			PM_SetPMViewAngle(ps, ps->viewangles, ucmd);

			ucmd->angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - ps->delta_angles[YAW];
			if (doMove)
			{
				//push me forward
				const float zVel = ps->velocity[2];
				if (ps->legsTimer > 500)
				{
					//not at end of anim yet
					float speed = 175;
					if (ucmd->forwardmove < 0)
					{
						//slower
						speed = 100;
					}
					else if (ucmd->forwardmove > 0)
					{
						speed = 250; //running speed
					}
					VectorScale(fwd, speed, ps->velocity);
				}
				ps->velocity[2] = zVel; //preserve z velocity
				//pull me toward the wall, too
				VectorMA(ps->velocity, dist, rt, ps->velocity);
			}
			ucmd->forwardmove = 0;
			return qtrue;
		}
		if (doMove)
		{
			//stop it
			if (ps->legsAnim == BOTH_WALL_RUN_RIGHT)
			{
				PM_SetAnim(SETANIM_BOTH, BOTH_WALL_RUN_RIGHT_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else if (ps->legsAnim == BOTH_WALL_RUN_LEFT)
			{
				PM_SetAnim(SETANIM_BOTH, BOTH_WALL_RUN_LEFT_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
		}
	}

	return qfalse;
}

static qboolean PM_AdjustAnglesForWallRunUpFlipAlt(const usercmd_t* ucmd)
{
	PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, ucmd);
	return qtrue;
}

static qboolean PM_AdjustAngleForWallRunUp(playerState_t* ps, usercmd_t* ucmd, const qboolean doMove)
{
	if (ps->legsAnim == BOTH_FORCEWALLRUNFLIP_START)
	{
		//wall-running up
		//stick to wall, if there is one
		vec3_t fwd, traceTo, mins, maxs, fwdAngles;
		trace_t trace;
		const float dist = 128;

		VectorSet(mins, -15, -15, 0);
		VectorSet(maxs, 15, 15, 24);
		VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);

		AngleVectors(fwdAngles, fwd, NULL, NULL);
		VectorMA(ps->origin, dist, fwd, traceTo);
		pm->trace(&trace, ps->origin, mins, maxs, traceTo, ps->clientNum, MASK_PLAYERSOLID);
		if (trace.fraction > 0.5f)
		{
			//hmm, some room, see if there's a floor right here
			trace_t trace2;
			vec3_t top, bottom;

			VectorCopy(trace.endpos, top);
			top[2] += pm->mins[2] * -1 + 4.0f;
			VectorCopy(top, bottom);
			bottom[2] -= 64.0f;
			pm->trace(&trace2, top, pm->mins, pm->maxs, bottom, ps->clientNum, MASK_PLAYERSOLID);
			if (!trace2.allsolid
				&& !trace2.startsolid
				&& trace2.fraction < 1.0f
				&& trace2.plane.normal[2] > 0.7f) //slope we can stand on
			{
				//cool, do the alt-flip and land on whetever it is we just scaled up
				VectorScale(fwd, 100, pm->ps->velocity);
				pm->ps->velocity[2] += 400;
				PM_SetAnim(SETANIM_BOTH, BOTH_FORCEWALLRUNFLIP_ALT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->pm_flags |= PMF_JUMP_HELD;
				PM_AddEvent(EV_JUMP);
				ucmd->upmove = 0;
				return qfalse;
			}
		}

		if ( //ucmd->upmove <= 0 &&
			ps->legsTimer > 0 &&
			ucmd->forwardmove > 0 &&
			trace.fraction < 1.0f &&
			(trace.plane.normal[2] >= 0.0f && trace.plane.normal[2] <= 0.4f/*MAX_WALL_RUN_Z_NORMAL*/))
		{
			//still a vertical wall there
			//make sure there's not a ceiling above us!
			trace_t trace2;
			VectorCopy(ps->origin, traceTo);
			traceTo[2] += 64;
			pm->trace(&trace2, ps->origin, mins, maxs, traceTo, ps->clientNum, MASK_PLAYERSOLID);
			if (trace2.fraction < 1.0f)
			{
				//will hit a ceiling, so force jump-off right now
				//NOTE: hits any entity or clip brush in the way, too, not just architecture!
			}
			else
			{
				//all clear, keep going
				//FIXME: don't pull around 90 turns
				//FIXME: simulate stepping up steps here, somehow?
				ucmd->forwardmove = 127;
				if (ucmd->upmove < 0)
				{
					ucmd->upmove = 0;
				}
				//make me face the wall
				ps->viewangles[YAW] = vectoyaw(trace.plane.normal) + 180;
				PM_SetPMViewAngle(ps, ps->viewangles, ucmd);
				ucmd->angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - ps->delta_angles[YAW];
				//if ( ent->s.number || !player_locked )
				if (1) //aslkfhsakf
				{
					if (doMove)
					{
						//pull me toward the wall
						VectorScale(trace.plane.normal, -dist * trace.fraction, ps->velocity);
						//push me up
						if (ps->legsTimer > 200)
						{
							//not at end of anim yet
							const float speed = 300;
							ps->velocity[2] = speed; //preserve z velocity
						}
					}
				}
				ucmd->forwardmove = 0;
				return qtrue;
			}
		}
		//failed!
		if (doMove)
		{
			//stop it
			VectorScale(fwd, -300.0f, ps->velocity);
			ps->velocity[2] += 200;
			PM_SetAnim(SETANIM_BOTH, BOTH_FORCEWALLRUNFLIP_END, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			ps->pm_flags |= PMF_JUMP_HELD;
			PM_AddEvent(EV_JUMP);
			ucmd->upmove = 0;
		}
	}
	return qfalse;
}

#define	FORCE_LONG_LEAP_SPEED	300.0f//300
#define	JUMP_OFF_WALL_SPEED	200.0f
//nice...
static float BG_ForceWallJumpStrength(void)
{
	return forceJumpStrength[FORCE_LEVEL_3] / 2.5f;
}

static qboolean PM_AdjustAngleForWallJump(playerState_t* ps, usercmd_t* ucmd, const qboolean doMove)
{
	if (PM_InLedgeMove(ps->legsAnim))
	{
		//Ledge movin'  Let the ledge move function handle it.
		return qfalse;
	}
	if ((PM_InReboundJump(ps->legsAnim) || PM_InReboundHold(ps->legsAnim))
		&& (PM_InReboundJump(ps->torsoAnim) || PM_InReboundHold(ps->torsoAnim))
		|| pm->ps->pm_flags & PMF_STUCK_TO_WALL)
	{
		//hugging wall, getting ready to jump off
		//stick to wall, if there is one
		vec3_t checkDir, traceTo, mins, maxs, fwdAngles;
		trace_t trace;
		const float dist = 128.0f;
		float yaw_adjust;

		VectorSet(mins, pm->mins[0], pm->mins[1], 0);
		VectorSet(maxs, pm->maxs[0], pm->maxs[1], 24);
		VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);

		switch (ps->legsAnim)
		{
		case BOTH_FORCEWALLREBOUND_RIGHT:
		case BOTH_FORCEWALLHOLD_RIGHT:
			AngleVectors(fwdAngles, NULL, checkDir, NULL);
			yaw_adjust = -90;
			break;
		case BOTH_FORCEWALLREBOUND_LEFT:
		case BOTH_FORCEWALLHOLD_LEFT:
			AngleVectors(fwdAngles, NULL, checkDir, NULL);
			VectorScale(checkDir, -1, checkDir);
			yaw_adjust = 90;
			break;
		case BOTH_FORCEWALLREBOUND_FORWARD:
		case BOTH_FORCEWALLHOLD_FORWARD:
			AngleVectors(fwdAngles, checkDir, NULL, NULL);
			yaw_adjust = 180;
			break;
		case BOTH_FORCEWALLREBOUND_BACK:
		case BOTH_FORCEWALLHOLD_BACK:
			AngleVectors(fwdAngles, checkDir, NULL, NULL);
			VectorScale(checkDir, -1, checkDir);
			yaw_adjust = 0;
			break;
		default:
			//WTF???
			pm->ps->pm_flags &= ~PMF_STUCK_TO_WALL;
			return qfalse;
		}
		if (pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1) //must have force jump to hold walls
		{
			//uber-skillz
			if (ucmd->upmove > 0)
			{
				//hold on until you let go manually
				if (PM_InReboundHold(ps->legsAnim))
				{
					//keep holding
					if (ps->legsTimer < 150)
					{
						ps->legsTimer = 150;
					}
				}
				else
				{
					//if got to hold part of anim, play hold anim
					if (ps->legsTimer <= 300)
					{
						ps->saberHolstered = 2;
						PM_SetAnim(
							SETANIM_BOTH, BOTH_FORCEWALLRELEASE_FORWARD + (ps->legsAnim - BOTH_FORCEWALLHOLD_FORWARD),
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						ps->legsTimer = ps->torsoTimer = 150;
					}
				}
			}
		}
		VectorMA(ps->origin, dist, checkDir, traceTo);
		pm->trace(&trace, ps->origin, mins, maxs, traceTo, ps->clientNum, MASK_PLAYERSOLID);
		if (ps->legsTimer > 100 &&
			trace.fraction < 1.0f &&
			fabs(trace.plane.normal[2]) <= 0.2f)
		{
			//still a vertical wall there
			if (ucmd->upmove < 0)
			{
				ucmd->upmove = 0;
			}
			//align me to the wall
			ps->viewangles[YAW] = vectoyaw(trace.plane.normal) + yaw_adjust;
			PM_SetPMViewAngle(ps, ps->viewangles, ucmd);
			ucmd->angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - ps->delta_angles[YAW];
			if (1)
			{
				if (doMove)
				{
					//pull me toward the wall
					VectorScale(trace.plane.normal, -128.0f, ps->velocity);
				}
			}
			ucmd->upmove = 0;
			ps->pm_flags |= PMF_STUCK_TO_WALL;
			return qtrue;
		}
		if (doMove
			&& ps->pm_flags & PMF_STUCK_TO_WALL)
		{
			//jump off
			//push off of it!
			ps->pm_flags &= ~PMF_STUCK_TO_WALL;
			ps->velocity[0] = ps->velocity[1] = 0;
			VectorScale(checkDir, -JUMP_OFF_WALL_SPEED, ps->velocity);
			ps->velocity[2] = BG_ForceWallJumpStrength();
			ps->pm_flags |= PMF_JUMP_HELD;
			ps->fd.forceJumpSound = 1; //this is a stupid thing, i should fix it.
			if (ps->origin[2] < ps->fd.forceJumpZStart)
			{
				ps->fd.forceJumpZStart = ps->origin[2];
			}
			//FIXME do I need this?

			WP_ForcePowerDrain(ps, FP_LEVITATION, 10);
			//no control for half a second
			ps->pm_flags |= PMF_TIME_KNOCKBACK;
			ps->pm_time = 500;
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->upmove = 127;

			if (PM_InReboundHold(ps->legsAnim))
			{
				//if was in hold pose, release now
				PM_SetAnim(SETANIM_BOTH, BOTH_FORCEWALLRELEASE_FORWARD + (ps->legsAnim - BOTH_FORCEWALLHOLD_FORWARD),
					SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else
			{
				if (pm->ps->weapon == WP_SABER)
				{
					PM_SetAnim(SETANIM_LEGS, BOTH_FORCEJUMP2,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
				}
				else
				{
					PM_SetAnim(SETANIM_LEGS, BOTH_FORCEJUMP1,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
				}
			}
		}
	}
	ps->pm_flags &= ~PMF_STUCK_TO_WALL;
	return qfalse;
}

//The height level at which you grab ledges.  In terms of player origin
#define LEDGEGRABMAXHEIGHT		70
#define LEDGEGRABHEIGHT			52.4
#define LEDGEVERTOFFSET			LEDGEGRABHEIGHT
#define LEDGEGRABMINHEIGHT		40

//max distance you can be from the ledge for ledge grabbing to work
#define LEDGEGRABDISTANCE		40

//min distance you can be from the ledge for ledge grab to work
#define LEDGEGRABMINDISTANCE	22

//distance at which the animation grabs the ledge
#define LEDGEHOROFFSET			22.3

//lets go of a ledge
static void BG_LetGoofLedge(playerState_t* ps)
{
	ps->pm_flags &= ~PMF_STUCK_TO_WALL;
	ps->torsoTimer = 0;
	ps->legsTimer = 0;
}

static void PM_SetVelocityforLedgeMove(playerState_t* ps, const int anim)
{
	vec3_t fwdAngles, moveDir;
	const float animationpoint = BG_GetLegsAnimPoint(ps, pm_entSelf->localAnimIndex);

	switch (anim)
	{
	case BOTH_LEDGE_GRAB:
	case BOTH_LEDGE_HOLD:
		VectorClear(ps->velocity);
		return;
	case BOTH_LEDGE_LEFT:
		if (animationpoint > .333 && animationpoint < .666)
		{
			VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);
			AngleVectors(fwdAngles, NULL, moveDir, NULL);
			VectorScale(moveDir, -30, moveDir);
			VectorCopy(moveDir, ps->velocity);
		}
		else
		{
			VectorClear(ps->velocity);
		}
		break;
	case BOTH_LEDGE_RIGHT:
		if (animationpoint > .333 && animationpoint < .666)
		{
			VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);
			AngleVectors(fwdAngles, NULL, moveDir, NULL);
			VectorScale(moveDir, 30, moveDir);
			VectorCopy(moveDir, ps->velocity);
		}
		else
		{
			VectorClear(ps->velocity);
		}
		break;
	case BOTH_LEDGE_MERCPULL:
		if (animationpoint > .8 && animationpoint < .925)
		{
			ps->velocity[0] = 0;
			ps->velocity[1] = 0;
			ps->velocity[2] = 154;
		}
		else if (animationpoint > .7 && animationpoint < .75)
		{
			ps->velocity[0] = 0;
			ps->velocity[1] = 0;
			ps->velocity[2] = 26;
		}
		else if (animationpoint > .375 && animationpoint < .7)
		{
			ps->velocity[0] = 0;
			ps->velocity[1] = 0;
			ps->velocity[2] = 140;
		}
		else if (animationpoint < .375)
		{
			VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);
			AngleVectors(fwdAngles, moveDir, NULL, NULL);
			VectorScale(moveDir, 140, moveDir);
			VectorCopy(moveDir, ps->velocity);
		}
		else
		{
			VectorClear(ps->velocity);
		}
		break;
	default:
		VectorClear(ps->velocity);
	}
}

static qboolean LedgeGrabableEntity(const int entityNum)
{
	//indicates if the given entity is an entity that can be ledgegrabbed.
	const bgEntity_t* ent = PM_BGEntForNum(entityNum);

	switch (ent->s.eType)
	{
	case ET_PLAYER:
	case ET_ITEM:
	case ET_MISSILE:
	case ET_SPECIAL:
	case ET_HOLOCRON:
	case ET_NPC:
		return qfalse;
	default:
		return qtrue;
	}
}

//Switch to this animation and keep repeating this animation while updating its timers
void PM_AdjustAngleForWallGrab(playerState_t* ps, usercmd_t* ucmd)
{
	if (ps->pm_flags & PMF_STUCK_TO_WALL && PM_InLedgeMove(ps->legsAnim))
	{
		//still holding onto the ledge stick our view to the wall angles
		if (ps->legsAnim != BOTH_LEDGE_MERCPULL)
		{
			vec3_t traceTo, traceFrom, fwd, fwdAngles;
			trace_t trace;

			VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);
			AngleVectors(fwdAngles, fwd, NULL, NULL);
			VectorNormalize(fwd);

			VectorCopy(ps->origin, traceFrom);
			traceFrom[2] += LEDGEGRABHEIGHT - 1;

			VectorMA(traceFrom, LEDGEGRABDISTANCE, fwd, traceTo);

			pm->trace(&trace, traceFrom, NULL, NULL, traceTo, ps->clientNum, pm->tracemask);

			if (trace.fraction == 1 || !LedgeGrabableEntity(trace.entityNum) || pm->ps->clientNum >= MAX_CLIENTS
				|| pm->cmd.buttons & BUTTON_FORCEPOWER
				|| pm->cmd.buttons & BUTTON_FORCE_LIGHTNING
				|| pm->cmd.buttons & BUTTON_FORCE_DRAIN
				|| pm->cmd.buttons & BUTTON_FORCEGRIP
				|| pm->cmd.buttons & BUTTON_BLOCK
				|| pm->cmd.buttons & BUTTON_KICK
				|| pm->cmd.buttons & BUTTON_DASH)
			{
				//that's not good, we lost the ledge so let go.
				BG_LetGoofLedge(ps);
				return;
			}

			//lock the view viewangles
			ps->viewangles[YAW] = vectoyaw(trace.plane.normal) + 180;
			PM_SetPMViewAngle(ps, ps->viewangles, ucmd);
			ucmd->angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - ps->delta_angles[YAW];
		}
		else
		{
			//lock viewangles
			PM_SetPMViewAngle(ps, ps->viewangles, ucmd);
			ucmd->angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - ps->delta_angles[YAW];
		}

		if (ps->legsTimer <= 50)
		{
			//Try switching to idle
			if (ps->legsAnim == BOTH_LEDGE_MERCPULL)
			{
				//pull up done, bail.
				ps->pm_flags &= ~PMF_STUCK_TO_WALL;
			}
			else
			{
				PM_SetAnim(SETANIM_BOTH, BOTH_LEDGE_HOLD, SETANIM_FLAG_OVERRIDE);
				ps->torsoTimer = 500;
				ps->legsTimer = 500;
				//hold weapontime so people can't do attacks while in ledgegrab
				ps->weaponTime = ps->legsTimer;
			}
		}
		else if (ps->legsAnim == BOTH_LEDGE_HOLD)
		{
			if (ucmd->rightmove)
			{
				//trying to move left/right
				if (ucmd->rightmove < 0)
				{
					//shimmy left
					PM_SetAnim(SETANIM_BOTH, BOTH_LEDGE_LEFT, AFLAG_LEDGE);
					//hold weapontime so people can't do attacks while in ledgegrab
					ps->weaponTime = ps->legsTimer;
				}
				else
				{
					//shimmy right
					PM_SetAnim(SETANIM_BOTH, BOTH_LEDGE_RIGHT, AFLAG_LEDGE);
					//hold weapontime so people can't do attacks while in ledgegrab
					ps->weaponTime = ps->legsTimer;
				}
			}
			else if (ucmd->forwardmove < 0 || pm->ps->clientNum >= MAX_CLIENTS
				|| pm->cmd.buttons & BUTTON_FORCEPOWER
				|| pm->cmd.buttons & BUTTON_FORCE_LIGHTNING
				|| pm->cmd.buttons & BUTTON_FORCE_DRAIN
				|| pm->cmd.buttons & BUTTON_FORCEGRIP
				|| pm->cmd.buttons & BUTTON_BLOCK
				|| pm->cmd.buttons & BUTTON_KICK
				|| pm->cmd.buttons & BUTTON_DASH)
			{
				//letting go
				BG_LetGoofLedge(ps);
			}
			else if (ucmd->forwardmove > 0)
			{
				//Pull up
				PM_SetAnim(SETANIM_BOTH, BOTH_LEDGE_MERCPULL,
					SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS);
				//hold weapontime so people can't do attacks while in ledgegrab
				ps->weaponTime = ps->legsTimer;
			}
			else
			{
				//keep holding on
				ps->torsoTimer = 500;
				ps->legsTimer = 500;
				//hold weapontime so people can't do attacks while in ledgegrab
				ps->weaponTime = ps->legsTimer;
			}
		}

		//set movement velocity
		PM_SetVelocityforLedgeMove(ps, ps->legsAnim);

		//clear movement commands to prevent movement
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		ucmd->forwardmove = 0;
	}
}

extern qboolean PM_InForceGetUp(const playerState_t* ps);
int G_MinGetUpTime(playerState_t* ps);

static qboolean PM_AdjustAnglesForKnockdown(playerState_t* ps, usercmd_t* ucmd)
{
	if (PM_InKnockDown(ps))
	{
		//being knocked down or getting up, can't do anything!
		if (!PM_InForceGetUp(ps) && (ps->legsTimer > G_MinGetUpTime(ps)
			|| ps->clientNum >= MAX_CLIENTS
			|| ps->legsAnim == BOTH_GETUP1
			|| ps->legsAnim == BOTH_GETUP2
			|| ps->legsAnim == BOTH_GETUP3
			|| ps->legsAnim == BOTH_GETUP4
			|| ps->legsAnim == BOTH_GETUP5))
		{
			//can't get up yet
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
		}

		if (ps->clientNum >= MAX_CLIENTS)
		{
			VectorClear(ps->moveDir);
		}

		if (!PM_InForceGetUp(ps) || (ps->legsTimer > 800 || pm->ps->weapon != WP_SABER) &&
			bg_get_torso_anim_point(ps, pm_entSelf->localAnimIndex) < .9f && ps->stats[STAT_HEALTH] > 0)
		{
			//can only attack if you've started a force-getup and are using the saber
			ucmd->buttons = 0;
		}
		//allow players to continue to move unless they're actually on lying on the ground.
		if (!PM_InForceGetUp(ps))
		{
			//can't turn unless in a force getup
			if (ps->viewEntity <= 0 || ps->viewEntity >= ENTITYNUM_WORLD)
			{
				//don't clamp angles when looking through a viewEntity
				PM_SetPMViewAngle(ps, ps->viewangles, ucmd);
			}
			ucmd->angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - ps->delta_angles[PITCH];
			ucmd->angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - ps->delta_angles[YAW];
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean PM_AdjustAnglesForLongJump(playerState_t* ps, usercmd_t* ucmd)
{
	if (ps->legsAnim == BOTH_FORCELONGLEAP_START
		|| ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK
		|| ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK2
		|| ps->legsAnim == BOTH_FORCELONGLEAP_LAND)
	{
		//can't turn
		if (ps->viewEntity <= 0 || ps->viewEntity >= ENTITYNUM_WORLD)
		{
			//don't clamp angles when looking through a viewEntity
			PM_SetPMViewAngle(ps, ps->viewangles, ucmd);
		}
		ucmd->angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - ps->delta_angles[PITCH];
		ucmd->angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - ps->delta_angles[YAW];
		return qtrue;
	}
	return qfalse;
}

float G_ForceWallJumpStrength(void)
{
	return forceJumpStrength[FORCE_LEVEL_3] / 2.5f;
}

//Set the height for when a force jump was started. If it's 0, nuge it up (slight hack to prevent holding jump over slopes)
void PM_SetForceJumpZStart(const float value)
{
	pm->ps->fd.forceJumpZStart = value;
	if (!pm->ps->fd.forceJumpZStart)
	{
		pm->ps->fd.forceJumpZStart -= 0.1f;
	}
}

float forceJumpHeightMax[NUM_FORCE_POWER_LEVELS] =
{
	66, //normal jump (32+stepheight(18)+crouchdiff(24) = 74)
	130, //(96+stepheight(18)+crouchdiff(24) = 138)
	226, //(192+stepheight(18)+crouchdiff(24) = 234)
	418 //(384+stepheight(18)+crouchdiff(24) = 426)
};

static qboolean PM_PredictJumpSafe()
{
	return qtrue;
}

static void PM_GrabWallForJump(const int anim)
{
	//NOTE!!! assumes an appropriate anim is being passed in!!!
	PM_SetAnim(SETANIM_BOTH, anim, SETANIM_FLAG_RESTART | SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	PM_AddEvent(EV_JUMP); //make sound for grab
	pm->ps->pm_flags |= PMF_STUCK_TO_WALL;
}

qboolean PM_CheckGrabWall(const trace_t* trace)
{
	if (!pm->ps)
	{
		return qfalse;
	}
	if (pm->ps->stats[STAT_HEALTH] <= 0)
	{
		//must be alive
		return qfalse;
	}
	if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
	{
		//must be in air
		return qfalse;
	}
	if (trace->plane.normal[2] != 0)
	{
		//must be a flat wall
		return qfalse;
	}
	if (!trace->plane.normal[0] && !trace->plane.normal[1])
	{
		//invalid normal
		return qfalse;
	}
	if (trace->contents & (CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP))
	{
		//can't jump off of clip brushes
		return qfalse;
	}
	if (pm->ps->fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_1)
	{
		//must have at least FJ 1
		return qfalse;
	}
	if (pm->ps->clientNum < MAX_CLIENTS
		&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_1)
	{
		//player must have force jump 3
		return qfalse;
	}

	if (pm->ps->clientNum < MAX_CLIENTS)
	{
		//player
		//only if we were in a longjump
		if (pm->ps->legsAnim != BOTH_FORCELONGLEAP_START
			&& pm->ps->legsAnim != BOTH_FORCELONGLEAP_ATTACK
			&& pm->ps->legsAnim != BOTH_FORCELONGLEAP_ATTACK2)
		{
			return qfalse;
		}
		//hit a flat wall during our long jump, see if we should grab it
		vec3_t moveDir;
		VectorCopy(pm->ps->velocity, moveDir);
		VectorNormalize(moveDir);
		if (DotProduct(moveDir, trace->plane.normal) > -0.65f)
		{
			//not enough of a direct impact, just slide off
			return qfalse;
		}
		if (fabs(trace->plane.normal[2]) > MAX_WALL_GRAB_SLOPE)
		{
			return qfalse;
		}
		VectorClear(pm->ps->velocity);
		PM_GrabWallForJump(BOTH_FORCEWALLREBOUND_FORWARD);
		return qtrue;
	}
	//NPCs
	if (PM_InReboundJump(pm->ps->legsAnim))
	{
		//already in a rebound!
		return qfalse;
	}

	if (pm->ps->legsAnim != BOTH_FORCELONGLEAP_START
		&& pm->ps->legsAnim != BOTH_FORCELONGLEAP_ATTACK
		&& pm->ps->legsAnim != BOTH_FORCELONGLEAP_ATTACK2)
	{
		//not in a long-jump
		vec3_t enemyDir;
		enemyDir[2] = 0;
		VectorNormalize(enemyDir);
		if (DotProduct(enemyDir, trace->plane.normal) < 0.65f)
		{
			//jumping off this wall would not launch me in the general direction of my enemy
			return qfalse;
		}
	}
	//hit a flat wall during our long jump, see if we should grab it
	vec3_t moveDir;
	VectorCopy(pm->ps->velocity, moveDir);
	VectorNormalize(moveDir);
	if (DotProduct(moveDir, trace->plane.normal) > -0.65f)
	{
		//not enough of a direct impact, just slide off
		return qfalse;
	}

	//Okay, now see if jumping off this thing would send us into a do not enter brush
	if (!PM_PredictJumpSafe())
	{
		//we would hit a do not enter brush, so don't grab the wall
		return qfalse;
	}

	//grab it!
	//Pick the proper anim
	int anim;
	vec3_t facingAngles, wallDir, fwdDir, rtDir;
	wallDir[2] = 0;
	VectorNormalize(wallDir);
	VectorSet(facingAngles, 0, pm->ps->viewangles[YAW], 0);
	AngleVectors(facingAngles, fwdDir, rtDir, NULL);
	const float fDot = DotProduct(fwdDir, wallDir);
	if (fabs(fDot) >= 0.5f)
	{
		//hit a wall in front/behind
		if (fDot > 0.0f)
		{
			//in front
			anim = BOTH_FORCEWALLREBOUND_FORWARD;
		}
		else
		{
			//behind
			anim = BOTH_FORCEWALLREBOUND_BACK;
		}
	}
	else if (DotProduct(rtDir, wallDir) > 0)
	{
		//hit a wall on the right
		anim = BOTH_FORCEWALLREBOUND_RIGHT;
	}
	else
	{
		//hit a wall on the left
		anim = BOTH_FORCEWALLREBOUND_LEFT;
	}
	VectorClear(pm->ps->velocity);
	//FIXME: stop slidemove!
	PM_GrabWallForJump(anim);
	return qtrue;
}

//The FP cost of Force Fall
#define FM_FORCEFALL			10

//the velocity to which Force Fall activates and tries to slow you to.
#define FORCEFALLVELOCITY		-250

//Rate at which the player brakes
int ForceFallBrakeRate[NUM_FORCE_POWER_LEVELS] =
{
	0, //Can't brake with zero Force Jump skills
	60,
	80,
	100,
};

//time between Force Fall braking actions.
#define FORCEFALLDEBOUNCE		100

static qboolean PM_CanForceFall()
{
	return !BG_InRoll(pm->ps, pm->ps->legsAnim) // not rolling
		&& !PM_InKnockDown(pm->ps) // not knocked down
		&& !BG_InDeathAnim(pm->ps->legsAnim) // not dead
		&& !pm_saber_in_special_attack(pm->ps->torsoAnim) // not doing special attack
		&& !PM_SaberInAttack(pm->ps->saber_move) // not attacking
		&& BG_CanUseFPNow(pm->gametype, pm->ps, pm->cmd.serverTime, FP_HEAL) // can use force power
		&& !(pm->ps->pm_flags & PMF_JUMP_HELD) // have to have released jump since last press
		&& pm->cmd.upmove > 10 // pressing the jump button
		&& pm->ps->velocity[2] < FORCEFALLVELOCITY // falling
		&& pm->ps->groundEntityNum == ENTITYNUM_NONE // in the air
		&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 //have force jump level 2 or above
		&& pm->ps->fd.forcePower > FM_FORCEFALL // have atleast 5 force power points
		&& pm->waterlevel < 2 // above water level
		&& pm->ps->gravity > 0; // not in zero-g
}

static qboolean PM_InForceFall()
{
	const int FFDebounce = pm->ps->fd.forcePowerDebounce[FP_LEVITATION] - pm->ps->fd.forcePowerLevel[FP_LEVITATION] *
		100;

	// can player force fall?
	if (PM_CanForceFall())
	{
		PM_SetAnim(SETANIM_BOTH, BOTH_FORCEINAIR1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

		// reduce falling velocity to a safe speed at set intervals
		if (FFDebounce + FORCEFALLDEBOUNCE < pm->cmd.serverTime)
		{
			if (pm->ps->velocity[2] < FORCEFALLVELOCITY)
			{
				if (FORCEFALLVELOCITY - pm->ps->velocity[2] < ForceFallBrakeRate[pm->ps->fd.forcePowerLevel[
					FP_LEVITATION]])
				{
					pm->ps->velocity[2] = FORCEFALLVELOCITY;
				}
				else
				{
					pm->ps->velocity[2] += ForceFallBrakeRate[pm->ps->fd.forcePowerLevel[FP_LEVITATION]];
				}
			}
		}

		pm->ps->powerups[PW_INVINCIBLE] = pm->cmd.serverTime + 100;

		// is it time to reduce the players force power
		if (pm->ps->fd.forcePowerDebounce[FP_LEVITATION] < pm->cmd.serverTime)
		{
			int force_mana_modifier = 0;
			// reduced the use of force power for duel and power duel matches
			if (pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_2)
			{
				force_mana_modifier = -4;
			}

			WP_ForcePowerDrain(pm->ps, FP_HEAL, FM_FORCEFALL + force_mana_modifier);

			// removes force power at a rate of 0.1 secs * force jump level
			pm->ps->fd.forcePowerDebounce[FP_LEVITATION] = pm->cmd.serverTime + pm->ps->fd.forcePowerLevel[
				FP_LEVITATION] * 100;
		}

		// player is force falling
		return qtrue;
	}

	// player is not force falling
	return qfalse;
}

static qboolean PM_Is_A_Dash_Anim(const int anim)
{
	switch (anim)
	{
	case BOTH_DASH_R:
	case BOTH_DASH_L:
	case BOTH_DASH_B:
	case BOTH_DASH_F:
	case BOTH_HOP_R:
	case BOTH_HOP_L:
	case BOTH_HOP_B:
	case BOTH_HOP_F:
		return qtrue;
	default:;
	}
	return qfalse;
}

/*
=============
PM_CheckJump
=============
*/
extern qboolean BG_EnoughForcePowerForMove(int cost);
extern void PM_AddFatigue(playerState_t* ps, int fatigue);

static qboolean pm_check_jump(void)
{
	qboolean allowFlips = qtrue;

	saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	saberInfo_t* saber2 = BG_MySaber(pm->ps->clientNum, 1);

	if (pm->ps->clientNum >= MAX_CLIENTS)
	{
		bgEntity_t* pEnt = pm_entSelf;

		if (pEnt->s.eType == ET_NPC &&
			pEnt->s.NPC_class == CLASS_VEHICLE)
		{
			//no!
			return qfalse;
		}
	}

	if (pm->ps->forceHandExtend == HANDEXTEND_KNOCKDOWN ||
		pm->ps->forceHandExtend == HANDEXTEND_PRETHROWN ||
		pm->ps->forceHandExtend == HANDEXTEND_POSTTHROWN)
	{
		return qfalse;
	}

	if (PM_InKnockDown(pm->ps))
	{
		return qfalse;
	}

	if (pm->ps->pm_type == PM_JETPACK)
	{
		//there's no actual jumping while we jetpack
		return qfalse;
	}

	//Don't allow jump until all buttons are up
	if (pm->ps->pm_flags & PMF_RESPAWNED)
	{
		return qfalse;
	}

	if (PM_InKnockDown(pm->ps) || BG_InRoll(pm->ps, pm->ps->legsAnim))
	{
		//in knockdown
		return qfalse;
	}

	if (PM_InForceFall())
	{
		return qfalse;
	}

	if (pm->ps->communicatingflags & 1 << DASHING || PM_Is_A_Dash_Anim(pm->ps->torsoAnim))
	{
		return qfalse;
	}

	if (pm->ps->weapon == WP_SABER)
	{
		if (saber1
			&& saber1->saberFlags & SFL_NO_FLIPS)
		{
			allowFlips = qfalse;
		}
		if (saber2
			&& saber2->saberFlags & SFL_NO_FLIPS)
		{
			allowFlips = qfalse;
		}
	}

	if (pm->ps->groundEntityNum != ENTITYNUM_NONE || pm->ps->origin[2] < pm->ps->fd.forceJumpZStart)
	{
		pm->ps->fd.forcePowersActive &= ~(1 << FP_LEVITATION);
	}

	if (pm->cmd.buttons & BUTTON_GRAPPLE)
	{
		// turn off force levitation if grapple is used
		// also remove protection from a fall
		pm->ps->fd.forcePowersActive &= ~(1 << FP_LEVITATION);
		pm->ps->fd.forceJumpZStart = 0;
	}

	if (pm->ps->fd.forcePowersActive & 1 << FP_LEVITATION)
	{
		//Force jump is already active.. continue draining power appropriately until we land.
		if (pm->ps->fd.forcePowerDebounce[FP_LEVITATION] < pm->cmd.serverTime)
		{
			if (pm->gametype == GT_DUEL
				|| pm->gametype == GT_POWERDUEL)
			{
				//jump takes less power
				WP_ForcePowerDrain(pm->ps, FP_LEVITATION, 1);
			}
			else
			{
				WP_ForcePowerDrain(pm->ps, FP_LEVITATION, 5);
			}
			if (pm->ps->fd.forcePowerLevel[FP_LEVITATION] >= FORCE_LEVEL_2)
			{
				pm->ps->fd.forcePowerDebounce[FP_LEVITATION] = pm->cmd.serverTime + 300;
			}
			else
			{
				pm->ps->fd.forcePowerDebounce[FP_LEVITATION] = pm->cmd.serverTime + 200;
			}
		}
	}

	if (pm->ps->forceJumpFlip)
	{
		//Forced jump anim
		int anim = BOTH_FORCEINAIR1;
		int parts = SETANIM_BOTH;
		if (allowFlips)
		{
			if (pm->cmd.forwardmove > 0)
			{
				anim = BOTH_FLIP_F;
			}
			else if (pm->cmd.forwardmove < 0)
			{
				anim = BOTH_FLIP_B;
			}
			else if (pm->cmd.rightmove > 0)
			{
				anim = BOTH_FLIP_R;
			}
			else if (pm->cmd.rightmove < 0)
			{
				anim = BOTH_FLIP_L;
			}
		}
		else
		{
			if (pm->cmd.forwardmove > 0)
			{
				anim = BOTH_FORCEINAIR1;
			}
			else if (pm->cmd.forwardmove < 0)
			{
				anim = BOTH_FORCEINAIRBACK1;
			}
			else if (pm->cmd.rightmove > 0)
			{
				anim = BOTH_FORCEINAIRRIGHT1;
			}
			else if (pm->cmd.rightmove < 0)
			{
				anim = BOTH_FORCEINAIRLEFT1;
			}
		}
		if (pm->ps->weaponTime)
		{
			//FIXME: really only care if we're in a saber attack anim...
			parts = SETANIM_LEGS;
		}

		PM_SetAnim(parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		pm->ps->forceJumpFlip = qfalse;
		return qtrue;
	}
#if METROID_JUMP
	if (pm->cmd.buttons & BUTTON_GRAPPLE)
	{
		//  turn off force levitation if grapple is used
		// also remove protection from a fall
		pm->ps->fd.forcePowersActive &= ~(1 << FP_LEVITATION);
		pm->ps->fd.forceJumpZStart = 0;
	}

	if (pm_entSelf->s.botclass == BCLASS_BOBAFETT ||
		pm_entSelf->s.botclass == BCLASS_MANDOLORIAN ||
		pm_entSelf->s.botclass == BCLASS_MANDOLORIAN1 ||
		pm_entSelf->s.botclass == BCLASS_MANDOLORIAN2)
	{
		//player playing as boba fett
		if (pm->cmd.upmove > 0 && pm->ps->jetpackFuel > 10)
		{
			//turn on/go up
			if (pm->ps->groundEntityNum == ENTITYNUM_NONE && !(pm->ps->pm_flags & PMF_JUMP_HELD))
			{
				//double-tap - must activate while in air
#ifdef _GAME
				ItemUse_Jetpack(&g_entities[pm->ps->clientNum]);
#endif
			}
		}
		else if (pm->cmd.upmove < 0)
		{
			//turn it off (or should we just go down)?
		}
	}
	else if (pm->waterlevel < 3)
	{
		if (pm->ps->gravity > 0)
		{
			//can't do this in zero-G
			if (pm->ps->legsAnim == BOTH_FORCELONGLEAP_START
				|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK
				|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK2
				|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_LAND
				|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_LAND2)
			{
				//in the middle of a force long-jump
				if ((pm->ps->legsAnim == BOTH_FORCELONGLEAP_START
					|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK
					|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK2)
					&& pm->ps->legsTimer > 0)
				{
					float oldZVel;
					//in the air
					vec3_t jFwdAngs, jFwdVec;
					VectorSet(jFwdAngs, 0, pm->ps->viewangles[YAW], 0);
					AngleVectors(jFwdAngs, jFwdVec, NULL, NULL);
					oldZVel = pm->ps->velocity[2];
					if (pm->ps->legsTimer > 150 && oldZVel < 0)
					{
						oldZVel = 0;
					}
					VectorScale(jFwdVec, FORCE_LONG_LEAP_SPEED, pm->ps->velocity);
					pm->ps->velocity[2] = oldZVel;
					pm->ps->pm_flags |= PMF_JUMP_HELD;
					pm->ps->pm_flags |= PMF_JUMPING | PMF_SLOW_MO_FALL;
					pm->ps->fd.forcePowersActive |= 1 << FP_LEVITATION;
					return qtrue;
				}
				//landing-slide
				if (pm->ps->legsAnim == BOTH_FORCELONGLEAP_START
					|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK
					|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK2)
				{
					//still in anim, but it's run out
					pm->ps->fd.forcePowersActive |= 1 << FP_LEVITATION;
					if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
					{
						//still in air?
						//hold it for another 50ms
					}
				}
				else
				{
					//in land-slide anim
					//FIXME: force some forward movement?  Less if holding back?
				}
				if (pm->ps->groundEntityNum == ENTITYNUM_NONE //still in air
					&& pm->ps->origin[2] < pm->ps->fd.forceJumpZStart) //dropped below original jump start
				{
					//slow down
					pm->ps->velocity[0] *= 0.65f;
					pm->ps->velocity[1] *= 0.65f;
					if ((pm->ps->velocity[0] + pm->ps->velocity[1]) * 0.5f <= 10.0f)
					{
						//falling straight down
						PM_SetAnim(SETANIM_BOTH, BOTH_FORCEINAIR1, SETANIM_FLAG_OVERRIDE);
					}
				}
				return qfalse;
			}
			if (pm->cmd.upmove > 0
				&& pm->ps->weapon == WP_SABER
				&& pm->ps->fd.forcePowerLevel[FP_SPEED] >= FORCE_LEVEL_3 //force speed 1 or better
				&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] >= FORCE_LEVEL_3 //force jump 1 or better
				&& pm->ps->fd.forcePower >= FORCE_LONGJUMP_POWER //this costs 20 force to do
				&& pm->ps->fd.forcePowersActive & 1 << FP_SPEED //force-speed is on
				&& pm->cmd.forwardmove > 0 //pushing forward
				&& !pm->cmd.rightmove //not strafing
				&& pm->ps->groundEntityNum != ENTITYNUM_NONE //not in mid-air
				&& !(pm->ps->pm_flags & PMF_JUMP_HELD)
				&& pm->ps->fd.forcePowerDebounce[FP_SPEED] <= 500
				//have to have just started the force speed within the last half second
				&& pm->ps)
			{
				vec3_t fwdAngles;
				vec3_t jumpFwd;
				//start a force long-jump!
				if (pm->cmd.buttons & BUTTON_ATTACK)
				{
					//only 1 attack you can do from this anim
					if (pm->ps->saberHolstered == 2)
					{
						pm->ps->saberHolstered = 0;
						PM_AddEvent(EV_SABER_UNHOLSTER);
					}
					PM_SetAnim(SETANIM_BOTH, BOTH_FORCELONGLEAP_ATTACK2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					if (pm->ps->saberHolstered == 2)
					{
						pm->ps->saberHolstered = 0;
						PM_AddEvent(EV_SABER_UNHOLSTER);
					}
					PM_SetAnim(SETANIM_BOTH, BOTH_FORCELONGLEAP_ATTACK, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);
				AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
				VectorScale(jumpFwd, FORCE_LONG_LEAP_SPEED, pm->ps->velocity); //speed
				pm->ps->velocity[2] = 240;
				pml.groundPlane = qfalse;
				pml.walking = qfalse;
				pm->ps->groundEntityNum = ENTITYNUM_NONE;
				pm->ps->fd.forceJumpZStart = pm->ps->origin[2];
				pm->ps->pm_flags |= PMF_JUMP_HELD;
				pm->ps->pm_flags |= PMF_JUMPING | PMF_SLOW_MO_FALL;
				//start force jump
				pm->ps->fd.forcePowersActive |= 1 << FP_LEVITATION;
				pm->cmd.upmove = 0;
				// keep track of force jump stat
				pm->ps->fd.forceJumpSound = 1;
				BG_ForcePowerKill(pm->ps);
				WP_ForcePowerDrain(pm->ps, FP_LEVITATION, FORCE_LONGJUMP_POWER);
				return qtrue;
			}
			if (PM_InCartwheel(pm->ps->legsAnim)
				|| PM_InButterfly(pm->ps->legsAnim))
			{
				//can't keep jumping up in cartwheels, ariels and butterflies
			}
			else if (PM_ForceJumpingUp() && pm->ps->pm_flags & PMF_JUMP_HELD)
			{
				//holding jump in air
				float curHeight = pm->ps->origin[2] - pm->ps->fd.forceJumpZStart;
				//check for max force jump level and cap off & cut z vel
				if ((curHeight <= forceJumpHeight[0] || //still below minimum jump height
					pm->ps->fd.forcePower && pm->cmd.upmove >= 10) &&
					////still have force power available and still trying to jump up
					curHeight < forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] &&
					pm->ps->fd.forceJumpZStart) //still below maximum jump height
				{
					//can still go up
					if (curHeight > forceJumpHeight[0])
					{
						//passed normal jump height  *2?
						if (!(pm->ps->fd.forcePowersActive & 1 << FP_LEVITATION)) //haven't started forcejump yet
						{
							//start force jump
							pm->ps->fd.forcePowersActive |= 1 << FP_LEVITATION;
							pm->ps->fd.forceJumpSound = 1;
							//play flip
							if (PM_InAirKickingAnim(pm->ps->legsAnim) && pm->ps->legsTimer)
							{
								//still in kick
							}
							else if ((pm->cmd.forwardmove || pm->cmd.rightmove) && //pushing in a dir
								pm->ps->legsAnim != BOTH_FLIP_F && //not already flipping
								pm->ps->legsAnim != BOTH_FLIP_F2 &&
								pm->ps->legsAnim != BOTH_FLIP_B &&
								pm->ps->legsAnim != BOTH_FLIP_R &&
								pm->ps->legsAnim != BOTH_FLIP_L &&
								pm->ps->legsAnim != BOTH_ALORA_FLIP_1_MD2 &&
								pm->ps->legsAnim != BOTH_ALORA_FLIP_2_MD2 &&
								pm->ps->legsAnim != BOTH_ALORA_FLIP_3_MD2 &&
								pm->ps->legsAnim != BOTH_ALORA_FLIP_1 &&
								pm->ps->legsAnim != BOTH_ALORA_FLIP_2 &&
								pm->ps->legsAnim != BOTH_ALORA_FLIP_3
								&& allowFlips)
							{
								int anim = BOTH_FORCEINAIR1;
								int parts = SETANIM_BOTH;

								if (pm->cmd.forwardmove > 0)
								{
									if (pm_entSelf->s.NPC_class == CLASS_ALORA && Q_irand(0, 3))
									{
										anim = Q_irand(BOTH_ALORA_FLIP_1, BOTH_ALORA_FLIP_3);
									}
									else
									{
										anim = BOTH_FLIP_F;
									}
								}
								else if (pm->cmd.forwardmove < 0)
								{
									anim = BOTH_FLIP_B;
								}
								else if (pm->cmd.rightmove > 0)
								{
									anim = BOTH_FLIP_R;
								}
								else if (pm->cmd.rightmove < 0)
								{
									anim = BOTH_FLIP_L;
								}
								if (pm->ps->weaponTime)
								{
									parts = SETANIM_LEGS;
								}

								PM_SetAnim(parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
							}
							else if (pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1)
							{
								vec3_t facingFwd, facingRight, facingAngles;
								int anim = -1;
								float dotR, dotF;

								VectorSet(facingAngles, 0, pm->ps->viewangles[YAW], 0);

								AngleVectors(facingAngles, facingFwd, facingRight, NULL);
								dotR = DotProduct(facingRight, pm->ps->velocity);
								dotF = DotProduct(facingFwd, pm->ps->velocity);

								if (fabs(dotR) > fabs(dotF) * 1.5)
								{
									if (dotR > 150)
									{
										anim = BOTH_FORCEJUMPRIGHT1;
									}
									else if (dotR < -150)
									{
										anim = BOTH_FORCEJUMPLEFT1;
									}
								}
								else
								{
									if (dotF > 150)
									{
										if (pm->ps->weapon == WP_SABER) //saber out
										{
											anim = BOTH_FORCEJUMP2;
										}
										else
										{
											anim = BOTH_FORCEJUMP1;
										}
									}
									else if (dotF < -150)
									{
										anim = BOTH_FORCEJUMPBACK1;
									}
								}
								if (anim != -1)
								{
									int parts = SETANIM_BOTH;
									if (pm->ps->weaponTime)
									{
										//FIXME: really only care if we're in a saber attack anim...
										parts = SETANIM_LEGS;
									}

									PM_SetAnim(parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
								}
							}
						}
						else
						{
							//jump is already active (the anim has started)
							if (pm->ps->legsTimer < 1)
							{
								//not in the middle of a legsAnim
								int anim = pm->ps->legsAnim;
								int newAnim = -1;
								switch (anim)
								{
								case BOTH_FORCEJUMP1:
								case BOTH_FORCEJUMP2:
									newAnim = BOTH_FORCELAND1;
									break;
								case BOTH_FORCEJUMPBACK1:
									newAnim = BOTH_FORCELANDBACK1;
									break;
								case BOTH_FORCEJUMPLEFT1:
									newAnim = BOTH_FORCELANDLEFT1;
									break;
								case BOTH_FORCEJUMPRIGHT1:
									newAnim = BOTH_FORCELANDRIGHT1;
									break;
								default:;
								}
								if (newAnim != -1)
								{
									int parts = SETANIM_BOTH;
									if (pm->ps->weaponTime)
									{
										parts = SETANIM_LEGS;
									}

									PM_SetAnim(parts, newAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
								}
							}
						}
					}

					//need to scale this down, start with height velocity (based on max force jump height) and scale down to regular jump vel
					pm->ps->velocity[2] = (forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] - curHeight) /
						forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] * forceJumpStrength[pm->ps->fd.
						forcePowerLevel[FP_LEVITATION]]; //JUMP_VELOCITY;
					pm->ps->velocity[2] /= 10;
					pm->ps->velocity[2] += JUMP_VELOCITY;
					pm->ps->pm_flags |= PMF_JUMP_HELD;
				}
				else if (curHeight > forceJumpHeight[0] && curHeight < forceJumpHeight[pm->ps->fd.forcePowerLevel[
					FP_LEVITATION]] - forceJumpHeight[0])
				{
					//still have some headroom, don't totally stop it
					if (pm->ps->velocity[2] > JUMP_VELOCITY)
					{
						pm->ps->velocity[2] = JUMP_VELOCITY;
					}
				}
				else
				{
					pm->ps->velocity[2] = 0;
				}
				pm->cmd.upmove = 0;
				return qfalse;
			}
			else if (pm->cmd.upmove > 0 && pm->ps->velocity[2] < -10)
			{
				//can't keep jumping, turn on jetpack?
#ifdef _GAME
				if (g_entities[pm->ps->clientNum].client && pm->ps->stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK
					&& !g_entities[pm->ps->clientNum].client->jetPackOn && pm->ps->jetpackFuel > 10)
				{
					ItemUse_Jetpack(&g_entities[pm->ps->clientNum]);
				}
#endif
			}
		}
	}

#endif

	//Not jumping
	if (pm->cmd.upmove < 10 && pm->ps->groundEntityNum != ENTITYNUM_NONE)
	{
		return qfalse;
	}

	// must wait for jump to be released
	if (pm->ps->pm_flags & PMF_JUMP_HELD)
	{
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	if (pm->ps->gravity <= 0)
	{
		//in low grav, you push in the dir you're facing as long as there is something behind you to shove off of
		vec3_t forward, back;
		trace_t trace;

		AngleVectors(pm->ps->viewangles, forward, NULL, NULL);
		VectorMA(pm->ps->origin, -8, forward, back);
		pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, back, pm->ps->clientNum, pm->tracemask);

		if (trace.fraction <= 1.0f)
		{
			if (pm->ps->weapon == WP_SABER)
			{
				VectorMA(pm->ps->velocity, JUMP_VELOCITY * 2, forward, pm->ps->velocity);
				PM_SetAnim(SETANIM_LEGS, BOTH_FORCEJUMP2,
					SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
			}
			else
			{
				VectorMA(pm->ps->velocity, JUMP_VELOCITY * 2, forward, pm->ps->velocity);
				PM_SetAnim(SETANIM_LEGS, BOTH_FORCEJUMP1,
					SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
			}
		} //else no surf close enough to push off of
		pm->cmd.upmove = 0;
	}
	else if (pm->cmd.upmove > 0 && pm->waterlevel < 2 &&
		pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 &&
		!(pm->ps->pm_flags & PMF_JUMP_HELD) &&
		!PM_IsRocketTrooper() &&
		!BG_HasYsalamiri(pm->gametype, pm->ps) &&
		BG_CanUseFPNow(pm->gametype, pm->ps, pm->cmd.serverTime, FP_LEVITATION))
	{
		qboolean allowWallRuns = qtrue;
		qboolean allowWallFlips = qtrue;
		qboolean allowWallGrabs = qtrue;

		if (pm->ps->weapon == WP_SABER)
		{
			if (saber1
				&& saber1

				->
				saberFlags & SFL_NO_WALL_RUNS
				)
			{
				allowWallRuns = qfalse;
			}
			if (saber2
				&& saber2

				->
				saberFlags & SFL_NO_WALL_RUNS
				)
			{
				allowWallRuns = qfalse;
			}
			if (saber1
				&& saber1

				->
				saberFlags & SFL_NO_WALL_FLIPS
				)
			{
				allowWallFlips = qfalse;
			}
			if (saber2
				&& saber2

				->
				saberFlags & SFL_NO_WALL_FLIPS
				)
			{
				allowWallFlips = qfalse;
			}
			if (saber1
				&& saber1

				->
				saberFlags & SFL_NO_FLIPS
				)
			{
				allowFlips = qfalse;
			}
			if (saber2
				&& saber2

				->
				saberFlags & SFL_NO_FLIPS
				)
			{
				allowFlips = qfalse;
			}
			if (saber1
				&& saber1

				->
				saberFlags & SFL_NO_WALL_GRAB
				)
			{
				allowWallGrabs = qfalse;
			}
			if (saber2
				&& saber2

				->
				saberFlags & SFL_NO_WALL_GRAB
				)
			{
				allowWallGrabs = qfalse;
			}
		}

		if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
		{
			//on the ground
			//check for left-wall and right-wall special jumps
			int anim = -1;
			float vertPush = 0;
			if (pm->cmd.rightmove > 0 && pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1)
			{
				//strafing right
				if (pm->cmd.forwardmove > 0)
				{
					//wall-run
					if (allowWallRuns)
					{
						vertPush = forceJumpStrength[FORCE_LEVEL_2] / 2.0f;
						anim = BOTH_WALL_RUN_RIGHT;
					}
				}
				else if (pm->cmd.forwardmove == 0)
				{
					//wall-flip
					if (allowWallFlips)
					{
						vertPush = forceJumpStrength[FORCE_LEVEL_2] / 2.25f;
						anim = BOTH_WALL_FLIP_RIGHT;
					}
				}
			}
			else if (pm->cmd.rightmove < 0 && pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1)
			{
				//strafing left
				if (pm->cmd.forwardmove > 0)
				{
					//wall-run
					if (allowWallRuns)
					{
						vertPush = forceJumpStrength[FORCE_LEVEL_2] / 2.0f;
						anim = BOTH_WALL_RUN_LEFT;
					}
				}
				else if (pm->cmd.forwardmove == 0)
				{
					//wall-flip
					if (allowWallFlips)
					{
						vertPush = forceJumpStrength[FORCE_LEVEL_2] / 2.25f;
						anim = BOTH_WALL_FLIP_LEFT;
					}
				}
			}
			else if (pm->cmd.forwardmove < 0 && !(pm->cmd.buttons & BUTTON_ATTACK))
			{
				//backflip
				if (allowFlips && BG_EnoughForcePowerForMove(FATIGUE_BACKFLIP_ATARU)
					&& saber1 && !saber2 && pm->ps->fd.saberAnimLevel == SS_DUAL)
				{
					vertPush = JUMP_VELOCITY;
					anim = BOTH_FLIP_BACK1;
					PM_AddFatigue(pm->ps, FATIGUE_BACKFLIP_ATARU);
				}
				else if (allowFlips && BG_EnoughForcePowerForMove(FATIGUE_BACKFLIP))
				{
					vertPush = JUMP_VELOCITY;
					anim = BOTH_FLIP_BACK1;
					PM_AddFatigue(pm->ps, FATIGUE_BACKFLIP);
				}
			}

			vertPush += 128; //give them an extra shove

			if (anim != -1)
			{
				vec3_t fwd, right, traceto, mins, maxs, fwd_angles;
				vec3_t idealNormal = { 0 }, wallNormal = { 0 };
				trace_t trace;
				qboolean doTrace = qfalse;
				int contents = MASK_SOLID;
				int playercontents = MASK_PLAYERSOLID;

				VectorSet(mins, pm->mins[0], pm->mins[1], 0);
				VectorSet(maxs, pm->maxs[0], pm->maxs[1], 24);
				VectorSet(fwd_angles, 0, pm->ps->viewangles[YAW], 0);

				memset(&trace, 0, sizeof trace); //to shut the compiler up

				AngleVectors(fwd_angles, fwd, right, NULL);

				//trace-check for a wall, if necc.
				switch (anim)
				{
				case BOTH_WALL_FLIP_LEFT:
					//NOTE: purposely falls through to next case!
				case BOTH_WALL_RUN_LEFT:
					doTrace = qtrue;
					VectorMA(pm->ps->origin, -16, right, traceto);
					break;

				case BOTH_WALL_FLIP_RIGHT:
					//NOTE: purposely falls through to next case!
				case BOTH_WALL_RUN_RIGHT:
					doTrace = qtrue;
					VectorMA(pm->ps->origin, 16, right, traceto);
					break;

				case BOTH_WALL_FLIP_BACK1:
					doTrace = qtrue;
					VectorMA(pm->ps->origin, 16, fwd, traceto);
					break;
				default:;
				}

				if (doTrace)
				{
					if (pm->ps->userInt2 == 0)
					{
						pm->trace(&trace, pm->ps->origin, mins, maxs, traceto, pm->ps->clientNum, contents);
					}
					else
					{
						pm->trace(&trace, pm->ps->origin, mins, maxs, traceto, pm->ps->clientNum, playercontents);
					}

					VectorCopy(trace.plane.normal, wallNormal);
					VectorNormalize(wallNormal);
					VectorSubtract(pm->ps->origin, traceto, idealNormal);
					VectorNormalize(idealNormal);
				}

				if (!doTrace || trace.fraction < 1.0f && (trace.entityNum < MAX_CLIENTS || DotProduct(
					wallNormal, idealNormal) > 0.7))
				{
					//there is a wall there.. or hit a client
					if (anim != BOTH_WALL_RUN_LEFT
						&& anim != BOTH_WALL_RUN_RIGHT
						&& anim != BOTH_FORCEWALLRUNFLIP_START
						|| wallNormal[2] >= 0.0f && wallNormal[2] <= 0.4f/*MAX_WALL_RUN_Z_NORMAL*/)
					{
						//wall-runs can only run on perfectly flat walls, sorry.
						int parts;
						//move me to side
						if (anim == BOTH_WALL_FLIP_LEFT)
						{
							pm->ps->velocity[0] = pm->ps->velocity[1] = 0;
							VectorMA(pm->ps->velocity, 150, right, pm->ps->velocity);
						}
						else if (anim == BOTH_WALL_FLIP_RIGHT)
						{
							pm->ps->velocity[0] = pm->ps->velocity[1] = 0;
							VectorMA(pm->ps->velocity, -150, right, pm->ps->velocity);
						}
						else if (anim == BOTH_FLIP_BACK1
							|| anim == BOTH_FLIP_BACK2
							|| anim == BOTH_FLIP_BACK3
							|| anim == BOTH_WALL_FLIP_BACK1)
						{
							pm->ps->velocity[0] = pm->ps->velocity[1] = 0;
							VectorMA(pm->ps->velocity, -150, fwd, pm->ps->velocity);
						}

						if (pm->ps->userInt2 == 1)
						{
							if (doTrace && anim != BOTH_WALL_RUN_LEFT && anim != BOTH_WALL_RUN_RIGHT)
							{
								if (trace.entityNum < MAX_CLIENTS)
								{
									pm->ps->forceKickFlip = trace.entityNum + 1;
									//let the server know that this person gets kicked by this client
								}
							}
						}

						//up
						if (vertPush)
						{
							pm->ps->velocity[2] = vertPush;
							pm->ps->fd.forcePowersActive |= 1 << FP_LEVITATION;
						}
						//animate me
						parts = SETANIM_LEGS;
						if (anim == BOTH_BUTTERFLY_LEFT)
						{
							parts = SETANIM_BOTH;
							pm->cmd.buttons &= ~BUTTON_ATTACK;
							pm->ps->saber_move = LS_NONE;
						}
						else if (!pm->ps->weaponTime) //not firing
						{
							parts = SETANIM_BOTH;
						}
						PM_SetAnim(parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (anim == BOTH_BUTTERFLY_LEFT)
						{
							pm->ps->weaponTime = pm->ps->torsoTimer;
						}
						PM_SetForceJumpZStart(pm->ps->origin[2]); //so we don't take damage if we land at same height
						pm->ps->pm_flags |= PMF_JUMP_HELD;
						pm->cmd.upmove = 0;
						pm->ps->fd.forceJumpSound = 1;
					}
				}
			}
		}
		else
		{
			//in the air
			int legsAnim = pm->ps->legsAnim;

			if (legsAnim == BOTH_WALL_RUN_LEFT || legsAnim == BOTH_WALL_RUN_RIGHT)
			{
				//running on a wall
				vec3_t right, traceto, mins, maxs, fwd_angles;
				trace_t trace;
				int anim = -1;

				VectorSet(mins, pm->mins[0], pm->mins[0], 0);
				VectorSet(maxs, pm->maxs[0], pm->maxs[0], 24);
				VectorSet(fwd_angles, 0, pm->ps->viewangles[YAW], 0);

				AngleVectors(fwd_angles, NULL, right, NULL);

				if (legsAnim == BOTH_WALL_RUN_LEFT)
				{
					if (pm->ps->legsTimer > 400)
					{
						//not at the end of the anim
						float anim_len = PM_AnimLength((animNumber_t)BOTH_WALL_RUN_LEFT);
						if (pm->ps->legsTimer < anim_len - 400)
						{
							//not at start of anim
							VectorMA(pm->ps->origin, -16, right, traceto);
							anim = BOTH_WALL_RUN_LEFT_FLIP;
						}
					}
				}
				else if (legsAnim == BOTH_WALL_RUN_RIGHT)
				{
					if (pm->ps->legsTimer > 400)
					{
						//not at the end of the anim
						float anim_len = PM_AnimLength((animNumber_t)BOTH_WALL_RUN_RIGHT);
						if (pm->ps->legsTimer < anim_len - 400)
						{
							//not at start of anim
							VectorMA(pm->ps->origin, 16, right, traceto);
							anim = BOTH_WALL_RUN_RIGHT_FLIP;
						}
					}
				}
				if (anim != -1)
				{
					pm->trace(&trace, pm->ps->origin, mins, maxs, traceto, pm->ps->clientNum,
						CONTENTS_SOLID | CONTENTS_BODY);
					if (trace.fraction < 1.0f)
					{
						//flip off wall
						int parts = 0;

						if (anim == BOTH_WALL_RUN_LEFT_FLIP)
						{
							pm->ps->velocity[0] *= 0.5f;
							pm->ps->velocity[1] *= 0.5f;
							VectorMA(pm->ps->velocity, 150, right, pm->ps->velocity);
						}
						else if (anim == BOTH_WALL_RUN_RIGHT_FLIP)
						{
							pm->ps->velocity[0] *= 0.5f;
							pm->ps->velocity[1] *= 0.5f;
							VectorMA(pm->ps->velocity, -150, right, pm->ps->velocity);
						}
						parts = SETANIM_LEGS;
						if (!pm->ps->weaponTime) //not firing
						{
							parts = SETANIM_BOTH;
						}
						PM_SetAnim(parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						pm->cmd.upmove = 0;
					}
				}
				if (pm->cmd.upmove != 0)
				{
					//jump failed, so don't try to do normal jump code, just return
					return qfalse;
				}
			}
			//NEW JKA
			else if (pm->ps->legsAnim == BOTH_FORCEWALLRUNFLIP_START)
			{
				vec3_t fwd, traceto, mins, maxs, fwd_angles;
				trace_t trace;
				int anim = -1;
				float anim_len;

				VectorSet(mins, pm->mins[0], pm->mins[0], 0.0f);
				VectorSet(maxs, pm->maxs[0], pm->maxs[0], 24.0f);
				//hmm, did you mean [1] and [1]?
				VectorSet(fwd_angles, 0, pm->ps->viewangles[YAW], 0.0f);
				AngleVectors(fwd_angles, fwd, NULL, NULL);

				assert(pm_entSelf); //null pm_entSelf would be a Bad Thing<tm>
				anim_len = BG_AnimLength(pm_entSelf->localAnimIndex, BOTH_FORCEWALLRUNFLIP_START);
				if (pm->ps->legsTimer < anim_len - 400)
				{
					//not at start of anim
					VectorMA(pm->ps->origin, 16, fwd, traceto);
					anim = BOTH_FORCEWALLRUNFLIP_END;
				}
				if (anim != -1)
				{
					pm->trace(&trace, pm->ps->origin, mins, maxs, traceto, pm->ps->clientNum,
						CONTENTS_SOLID | CONTENTS_BODY);
					if (trace.fraction < 1.0f)
					{
						//flip off wall
						int parts = SETANIM_LEGS;

						pm->ps->velocity[0] *= 0.5f;
						pm->ps->velocity[1] *= 0.5f;
						VectorMA(pm->ps->velocity, -300, fwd, pm->ps->velocity);
						pm->ps->velocity[2] += 200;
						if (!pm->ps->weaponTime) //not firing
						{
							//not attacking, set anim on both
							parts = SETANIM_BOTH;
						}
						PM_SetAnim(parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						pm->cmd.upmove = 0;
						PM_AddEvent(EV_JUMP);
					}
				}
				if (pm->cmd.upmove != 0)
				{
					//jump failed, so don't try to do normal jump code, just return
					return qfalse;
				}
			}
			else if (pm->cmd.forwardmove > 0 //pushing forward
				&& pm->ps->fd.forceRageRecoveryTime < pm->cmd.serverTime //not in a force Rage recovery period
				&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1
				&& PM_WalkableGroundDistance() <= 80
				//unfortunately we do not have a happy ground timer like SP (this would use up more bandwidth if we wanted prediction workign right), so we'll just use the actual ground distance.
				&& (pm->ps->legsAnim == BOTH_JUMP1 || pm->ps->legsAnim == BOTH_JUMP2 || pm->ps->legsAnim ==
					BOTH_INAIR1)) //not in a flip or spin or anything
			{
				//run up wall, flip backwards
				vec3_t fwd, traceto, mins, maxs, fwd_angles;
				trace_t trace;
				vec3_t idealNormal;

				VectorSet(mins, pm->mins[0], pm->mins[1], pm->mins[2]);
				VectorSet(maxs, pm->maxs[0], pm->maxs[1], pm->maxs[2]);
				VectorSet(fwd_angles, 0, pm->ps->viewangles[YAW], 0);

				AngleVectors(fwd_angles, fwd, NULL, NULL);
				VectorMA(pm->ps->origin, 32, fwd, traceto);

				pm->trace(&trace, pm->ps->origin, mins, maxs, traceto, pm->ps->clientNum, MASK_PLAYERSOLID);
				//FIXME: clip brushes too?
				VectorSubtract(pm->ps->origin, traceto, idealNormal);
				VectorNormalize(idealNormal);

				if (pm->ps->userInt2 == 1 && pm->ps->velocity[2] > 200 && !PM_InSpecialJump(pm->ps->legsAnim))
				{
					if (trace.fraction < 1.0f && trace.entityNum < MAX_CLIENTS)
					{
						int parts = SETANIM_LEGS;

						pm->ps->velocity[0] = pm->ps->velocity[1] = 0;
						VectorMA(pm->ps->velocity, -150, fwd, pm->ps->velocity);
						pm->ps->velocity[2] += 128;

						if (!pm->ps->weaponTime) //not firing
						{
							parts = SETANIM_BOTH;
						}
						PM_SetAnim(parts, BOTH_WALL_FLIP_BACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

						pm->ps->legsTimer -= 600;
						//I force this anim to play to the end to prevent landing on your head and suddenly flipping over.
						//It is a bit too long at the end though, so I'll just shorten it.

						PM_SetForceJumpZStart(pm->ps->origin[2]); //so we don't take damage if we land at same height
						pm->cmd.upmove = 0;
						pm->ps->fd.forceJumpSound = 1;
						WP_ForcePowerDrain(pm->ps, FP_LEVITATION, 5);

						if (trace.entityNum < MAX_CLIENTS)
						{
							pm->ps->forceKickFlip = trace.entityNum + 1;
							//let the server know that this person gets kicked by this client
						}
					}
				}

				if (allowWallRuns)
				{
					//FIXME: have to be moving... make sure it's opposite the wall... or at least forward?
					int wallWalkAnim = BOTH_WALL_FLIP_BACK1;
					int parts = SETANIM_LEGS;
					int contents = MASK_PLAYERSOLID;
					if (pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_2)
					{
						wallWalkAnim = BOTH_FORCEWALLRUNFLIP_START;
						parts = SETANIM_BOTH;
					}
					else
					{
						if (!pm->ps->weaponTime) //not firing
						{
							parts = SETANIM_BOTH;
						}
					}
					if (1) //sure, we have it! Because I SAID SO.
					{
						vec3_t forward, trace_to, minimum_mins, maximum_maxs, angles;
						trace_t trace_s;
						vec3_t ideal_normal;
						bgEntity_t* traceEnt;

						VectorSet(minimum_mins, pm->mins[0], pm->mins[1], 0.0f);
						VectorSet(maximum_maxs, pm->maxs[0], pm->maxs[1], 24.0f);
						VectorSet(angles, 0, pm->ps->viewangles[YAW], 0.0f);

						AngleVectors(angles, forward, NULL, NULL);
						VectorMA(pm->ps->origin, 32, forward, trace_to);

						pm->trace(&trace_s, pm->ps->origin, minimum_mins, maximum_maxs, trace_to, pm->ps->clientNum,
							contents);
						//FIXME: clip brushes too?
						VectorSubtract(pm->ps->origin, trace_to, ideal_normal);
						VectorNormalize(ideal_normal);

						traceEnt = PM_BGEntForNum(trace_s.entityNum);

						if (trace_s.fraction < 1.0f
							&& (trace_s.entityNum < ENTITYNUM_WORLD && traceEnt && traceEnt->s.solid != SOLID_BMODEL ||
								DotProduct(trace_s.plane.normal, ideal_normal) > 0.7))
						{
							//there is a wall there
							pm->ps->velocity[0] = pm->ps->velocity[1] = 0;
							if (wallWalkAnim == BOTH_FORCEWALLRUNFLIP_START)
							{
								pm->ps->velocity[2] = forceJumpStrength[FORCE_LEVEL_3] / 2.0f;
							}
							else
							{
								VectorMA(pm->ps->velocity, -150, forward, pm->ps->velocity);
								pm->ps->velocity[2] += 150.0f;
							}
							//animate me
							PM_SetAnim(parts, wallWalkAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
							PM_SetForceJumpZStart(pm->ps->origin[2]);
							//so we don't take damage if we land at same height
							pm->cmd.upmove = 0;
							pm->ps->fd.forceJumpSound = 1;
							WP_ForcePowerDrain(pm->ps, FP_LEVITATION, 5);

							if (pm->ps->userInt2 == 1)
							{
								if (traceEnt && (traceEnt->s.eType == ET_PLAYER || traceEnt->s.eType == ET_NPC))
								{
									//kick that thang!
									pm->ps->forceKickFlip = traceEnt->s.number + 1;
								}
							}
							pm->cmd.rightmove = pm->cmd.forwardmove = 0;
						}
					}
				}
			}
			else if ((!PM_InSpecialJump(legsAnim) //not in a special jump anim
				|| PM_InReboundJump(legsAnim) //we're already in a rebound
				|| PM_InBackFlip(legsAnim)) //a backflip (needed so you can jump off a wall behind you)
				&& pm->ps->velocity[2] > -1200 //not falling down very fast
				&& !(pm->ps->pm_flags & PMF_JUMP_HELD) //have to have released jump since last press
				&& (pm->cmd.forwardmove || pm->cmd.rightmove) //pushing in a direction
				&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 //level 1 jump or better
				&& BG_CanUseFPNow(pm->gametype, pm->ps, pm->cmd.serverTime, FP_LEVITATION)
				&& pm->ps->origin[2] - pm->ps->fd.forceJumpZStart < forceJumpHeightMax[FORCE_LEVEL_3] -
				BG_ForceWallJumpStrength() / 2.0f)
				//can fit at least one more wall jump in (yes, using "magic numbers"... for now)
			{
				//see if we're pushing at a wall and jump off it if so
				if (allowWallGrabs)
				{
					vec3_t checkDir, mins, maxs, fwd_angles;
					trace_t trace;
					int anim = -1;

					VectorSet(mins, pm->mins[0], pm->mins[1], 0.0f);
					VectorSet(maxs, pm->maxs[0], pm->maxs[1], 24.0f);
					VectorSet(fwd_angles, 0, pm->ps->viewangles[YAW], 0.0f);

					if (pm->cmd.rightmove)
					{
						if (pm->cmd.rightmove > 0)
						{
							anim = BOTH_FORCEWALLREBOUND_RIGHT;
							AngleVectors(fwd_angles, NULL, checkDir, NULL);
						}
						else if (pm->cmd.rightmove < 0)
						{
							anim = BOTH_FORCEWALLREBOUND_LEFT;
							AngleVectors(fwd_angles, NULL, checkDir, NULL);
							VectorScale(checkDir, -1, checkDir);
						}
					}
					else if (pm->cmd.forwardmove > 0)
					{
						anim = BOTH_FORCEWALLREBOUND_FORWARD;
						AngleVectors(fwd_angles, checkDir, NULL, NULL);
					}
					else if (pm->cmd.forwardmove < 0)
					{
						anim = BOTH_FORCEWALLREBOUND_BACK;
						AngleVectors(fwd_angles, checkDir, NULL, NULL);
						VectorScale(checkDir, -1, checkDir);
					}
					if (anim != -1)
					{
						vec3_t idealNormal;
						vec3_t traceto;
						//trace in the dir we're pushing in and see if there's a vertical wall there
						bgEntity_t* traceEnt;

						VectorMA(pm->ps->origin, 8, checkDir, traceto);
						pm->trace(&trace, pm->ps->origin, mins, maxs, traceto, pm->ps->clientNum, CONTENTS_SOLID);
						//FIXME: clip brushes too?
						VectorSubtract(pm->ps->origin, traceto, idealNormal);
						VectorNormalize(idealNormal);
						traceEnt = PM_BGEntForNum(trace.entityNum);
						if (trace.fraction < 1.0f
							&& fabs(trace.plane.normal[2]) <= 0.2f /*MAX_WALL_GRAB_SLOPE*/
							&& (trace.entityNum < ENTITYNUM_WORLD && traceEnt && traceEnt->s.solid != SOLID_BMODEL ||
								DotProduct(trace.plane.normal, idealNormal) > 0.7))
						{
							//there is a wall there
							float dot = DotProduct(pm->ps->velocity, trace.plane.normal);
							if (dot < 1.0f)
							{
								//can't be heading *away* from the wall!
								//grab it!
								PM_GrabWallForJump(anim);
							}
						}
					}
				}
			}
			else
			{
				//FIXME: if in a butterfly, kick people away?
			}
			//END NEW JKA
		}
	}

	if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
	{
		if (!(pm->waterlevel == 1 && pm->cmd.upmove > 0 && pm->ps->fd.forcePowerLevel[FP_LEVITATION] == FORCE_LEVEL_3 &&
			pm->ps->fd.forcePower >= 5))
		{
			return qfalse;
		}
		// jumping out of water uses some force
		pm->ps->fd.forcePower -= 5;
	}

	if (pm->ps->fd.forcePower < FATIGUE_JUMP)
	{
		//too tired to jump
		return qfalse;
	}

	PM_AddFatigue(pm->ps, FATIGUE_JUMP);

	if (pm->cmd.upmove > 0)
	{
		//no special jumps
		pm->ps->velocity[2] = JUMP_VELOCITY;
#ifdef _GAME
		if (g_entities[pm->ps->clientNum].client->skillLevel[SK_ACROBATICS])
			pm->ps->velocity[2] += 155;
#endif
		PM_SetForceJumpZStart(pm->ps->origin[2]); //so we don't take damage if we land at same height
		pm->ps->pm_flags |= PMF_JUMP_HELD;
	}

	//Jumping
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	PM_SetForceJumpZStart(pm->ps->origin[2]);

	PM_AddEvent(EV_JUMP);

	//Set the animations
	if (pm->ps->gravity > 0 && !PM_InSpecialJump(pm->ps->legsAnim))
	{
		PM_JumpForDir();
	}

	return qtrue;
}

static qboolean LedgeTrace(trace_t* trace, vec3_t dir, float* lerpup, float* lerpfwd, float* lerpyaw)
{
	//scan for for a ledge in the given direction
	vec3_t traceTo, traceFrom, wallangles;
	VectorMA(pm->ps->origin, LEDGEGRABDISTANCE, dir, traceTo);
	VectorCopy(pm->ps->origin, traceFrom);

	traceFrom[2] += LEDGEGRABMINHEIGHT;
	traceTo[2] += LEDGEGRABMINHEIGHT;

	pm->trace(trace, traceFrom, NULL, NULL, traceTo, pm->ps->clientNum, pm->tracemask);

	if (trace->fraction < 1 && LedgeGrabableEntity(trace->entityNum))
	{
		//hit a wall, pop into the wall and fire down to find top of wall
		VectorMA(trace->endpos, 0.5, dir, traceTo);

		VectorCopy(traceTo, traceFrom);

		traceFrom[2] += LEDGEGRABMAXHEIGHT - LEDGEGRABMINHEIGHT;

		pm->trace(trace, traceFrom, NULL, NULL, traceTo, pm->ps->clientNum, pm->tracemask);

		if (trace->fraction == 1.0 || trace->startsolid || !LedgeGrabableEntity(trace->entityNum))
		{
			return qfalse;
		}
	}

	//check to make sure we found a good top surface and go from there
	vectoangles(trace->plane.normal, wallangles);
	if (wallangles[PITCH] > -45)
	{
		//no ledge or the ledge is too steep
		return qfalse;
	}
	VectorCopy(trace->endpos, traceTo);
	*lerpup = trace->endpos[2] - pm->ps->origin[2] - LEDGEVERTOFFSET;

	VectorCopy(pm->ps->origin, traceFrom);
	traceTo[2] -= 1;

	traceFrom[2] = traceTo[2];

	pm->trace(trace, traceFrom, NULL, NULL, traceTo, pm->ps->clientNum, pm->tracemask);

	vectoangles(trace->plane.normal, wallangles);
	if (trace->fraction == 1.0
		|| wallangles[PITCH] > 20 || wallangles[PITCH] < -20
		|| !LedgeGrabableEntity(trace->entityNum))
	{
		//no ledge or too steep of a ledge
		return qfalse;
	}

	*lerpfwd = Distance(trace->endpos, traceFrom) - LEDGEHOROFFSET;
	*lerpyaw = vectoyaw(trace->plane.normal) + 180;
	return qtrue;
}

//check for ledge grab

static qboolean PM_IsGunner(void)
{
	switch (pm->ps->weapon)
	{
	case WP_BRYAR_OLD:
	case WP_BLASTER:
	case WP_DISRUPTOR:
	case WP_BOWCASTER:
	case WP_REPEATER:
	case WP_DEMP2:
	case WP_FLECHETTE:
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
	case WP_TRIP_MINE:
	case WP_DET_PACK:
	case WP_CONCUSSION:
	case WP_STUN_BATON:
	case WP_BRYAR_PISTOL:
		return qtrue;
	default:;
	}
	return qfalse;
}

void PM_CheckGrab(void)
{
	vec3_t checkDir, traceTo, fwdAngles;
	trace_t trace;
	float lerpup = 0;
	float lerpfwd = 0;
	float lerpyaw = 0;
	qboolean skip_Cmdtrace = qfalse;

	if (pm->ps->groundEntityNum != ENTITYNUM_NONE && pm->ps->inAirAnim)
	{
		//not in the air don't attempt a ledge grab
		return;
	}

	if (pm->ps->pm_type == PM_JETPACK)
	{
		//don't do ledgegrab checks while using the jetpack
		return;
	}

	if (pm->watertype & CONTENTS_LADDER)
	{
		//no
		return;
	}

	if (pm->waterlevel > 1)
	{
		//no
		return;
	}

	if (PM_InLedgeMove(pm->ps->legsAnim) || pm->ps->pm_type == PM_SPECTATOR
		|| PM_InSpecialJump(pm->ps->legsAnim)
		|| PM_CrouchAnim(pm->ps->legsAnim)
		|| pm->ps->pm_flags & PMF_DUCKED)
	{
		//already on ledge, a spectator, or in a special jump
		return;
	}

	if (pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCK ||
		pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
		pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
	{
		return;
	}

	if (pm->ps->velocity[0] == 0)
	{
		//Not moving forward
		return;
	}

#ifdef _GAME
	if (pm_entSelf->s.eType == ET_NPC)
	{
		return;
	}
#endif

	if (g_AllowLedgeGrab.integer == 0)
	{
		//Not allowed
		return;
	}

	//try looking in front of us first
	VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0.0f);
	AngleVectors(fwdAngles, checkDir, NULL, NULL);

	if (!VectorCompare(pm->ps->velocity, vec3_origin))
	{
		//player is moving
		if (LedgeTrace(&trace, checkDir, &lerpup, &lerpfwd, &lerpyaw))
		{
			skip_Cmdtrace = qtrue;
		}
	}

	if (!skip_Cmdtrace)
	{
		//no luck finding a ledge to grab based on movement.  Try looking for a ledge based on where the player is
		//TRYING to go.
		if (!pm->cmd.rightmove && !pm->cmd.forwardmove)
		{
			//no dice abort
			return;
		}
		if (pm->cmd.rightmove)
		{
			if (pm->cmd.rightmove > 0)
			{
				AngleVectors(fwdAngles, NULL, checkDir, NULL);
				VectorNormalize(checkDir);
			}
			else if (pm->cmd.rightmove < 0)
			{
				AngleVectors(fwdAngles, NULL, checkDir, NULL);
				VectorScale(checkDir, -1, checkDir);
				VectorNormalize(checkDir);
			}
		}
		else if (pm->cmd.forwardmove > 0)
		{
			//already tried this direction.
			return;
		}
		else if (pm->cmd.forwardmove < 0)
		{
			AngleVectors(fwdAngles, checkDir, NULL, NULL);
			VectorScale(checkDir, -1, checkDir);
			VectorNormalize(checkDir);
		}

		if (!LedgeTrace(&trace, checkDir, &lerpup, &lerpfwd, &lerpyaw))
		{
			//no dice
			return;
		}
	}

	VectorCopy(pm->ps->origin, traceTo);
	VectorMA(pm->ps->origin, lerpfwd, checkDir, traceTo);
	traceTo[2] += lerpup;

	//check to see if we can actually latch to that position.
	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, traceTo, pm->ps->clientNum, MASK_PLAYERSOLID);
	if (trace.fraction != 1 || trace.startsolid)
	{
		return;
	}

	//turn to face wall
	pm->ps->viewangles[YAW] = lerpyaw;
	PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
	pm->cmd.angles[YAW] = ANGLE2SHORT(pm->ps->viewangles[YAW]) - pm->ps->delta_angles[YAW];
	pm->ps->weaponTime = 0;
	pm->ps->saber_move = 0;
	pm->cmd.upmove = 0;
	//We are clear to latch to the wall
	if (pm->ps->weapon == WP_SABER && !pm->ps->saberHolstered)
	{
		pm->ps->saberHolstered = 2;
#ifdef _GAME
		gentity_t* self = &g_entities[pm->ps->clientNum];
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberoff.mp3"));
#endif
	}
	else
	{
		if (PM_IsGunner())
		{
#ifdef _GAME
			gentity_t* self = &g_entities[pm->ps->clientNum];
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/change.wav"));
#endif
		}
	}
	VectorCopy(trace.endpos, pm->ps->origin);
	VectorCopy(vec3_origin, pm->ps->velocity);
	PM_GrabWallForJump(BOTH_LEDGE_GRAB);
	pm->ps->weaponTime = pm->ps->legsTimer;
}

/*
=============
PM_CheckWaterJump
=============
*/
static qboolean PM_CheckWaterJump(void)
{
	vec3_t spot;
	vec3_t flatforward;

	if (pm->ps->pm_time)
	{
		return qfalse;
	}

	if (pm->cmd.forwardmove <= 0 && pm->cmd.upmove <= 0)
	{
		//they must not want to get out?
		return qfalse;
	}

	// check for water jump
	if (pm->waterlevel != 2)
	{
		return qfalse;
	}

	if (pm->watertype & CONTENTS_LADDER)
	{
		if (pm->ps->velocity[2] <= 0)
			return qfalse;
	}

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize(flatforward);

	VectorMA(pm->ps->origin, 30, flatforward, spot);
	spot[2] += 24;
	int cont = pm->pointcontents(spot, pm->ps->clientNum);
	if (!(cont & CONTENTS_SOLID))
	{
		return qfalse;
	}

	spot[2] += 16;
	cont = pm->pointcontents(spot, pm->ps->clientNum);
	if (cont & (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_BODY))
	{
		return qfalse;
	}

	// jump out of water
	VectorScale(pml.forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350 + (pm->waterlevel - pm->ps->origin[2]) * 2;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

//============================================================================

/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove(void)
{
	// waterjump has no control, but falls

	PM_StepSlideMove(qtrue);

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if (pm->ps->velocity[2] < 0)
	{
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove(void)
{
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float scale;

	if (PM_CheckWaterJump())
	{
		PM_WaterJumpMove();
		return;
	}
#if 0
	// jump = head for surface
	if (pm->cmd.upmove >= 10) {
		if (pm->ps->velocity[2] > -300) {
			if (pm->watertype == CONTENTS_WATER) {
				pm->ps->velocity[2] = 100;
			}
			else if (pm->watertype == CONTENTS_SLIME) {
				pm->ps->velocity[2] = 80;
			}
			else {
				pm->ps->velocity[2] = 50;
			}
		}
	}
#endif
	PM_Friction();

	scale = PM_CmdScale(&pm->cmd);
	//
	// user intentions
	//
	if (!scale)
	{
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60; // sink towards bottom
	}
	else
	{
		for (int i = 0; i < 3; i++)
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (pm->watertype & CONTENTS_LADDER) //ladder
	{
		if (wishspeed > pm->ps->speed * pm_ladderScale)
		{
			wishspeed = pm->ps->speed * pm_ladderScale;
		}
		PM_Accelerate(wishdir, wishspeed, pm_Laddeeraccelerate);
	}
	else
	{
		if (pm->ps->gravity < 0)
		{
			//float up
			pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
		}

		if (wishspeed > pm->ps->speed * pm_swimScale)
		{
			wishspeed = pm->ps->speed * pm_swimScale;
		}

		PM_Accelerate(wishdir, wishspeed, pm_wateraccelerate);
	}

	// make sure we can go up slopes easily under water
	if (pml.groundPlane && DotProduct(pm->ps->velocity, pml.groundTrace.plane.normal) < 0)
	{
		const float vel = VectorLength(pm->ps->velocity);
		// slide along the ground plane
		PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,
			pm->ps->velocity, OVERCLIP);

		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	PM_SlideMove(qfalse);

	if (pm->ps->weapon == WP_SABER && !pm->ps->saberHolstered)
	{
		pm->ps->saberHolstered = 2;
#ifdef _GAME
		gentity_t* self = &g_entities[pm->ps->clientNum];
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberoff.mp3"));
#endif
	}
	else
	{
		if (PM_IsGunner())
		{
#ifdef _GAME
			gentity_t* self = &g_entities[pm->ps->clientNum];
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/change.wav"));
#endif
		}
	}
}

static void PM_LadderMove(void)
{
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float scale;

	if (PM_CheckWaterJump())
	{
		PM_WaterJumpMove();
		return;
	}
#if 0
	// jump = head for surface
	if (pm->cmd.upmove >= 10) {
		if (pm->ps->velocity[2] > -300) {
			if (pm->watertype == CONTENTS_WATER) {
				pm->ps->velocity[2] = 100;
			}
			else if (pm->watertype == CONTENTS_SLIME) {
				pm->ps->velocity[2] = 80;
			}
			else {
				pm->ps->velocity[2] = 50;
			}
		}
	}
#endif
	PM_Friction();

	scale = PM_CmdScale(&pm->cmd);
	//
	// user intentions
	//
	if (!scale)
	{
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60; // sink towards bottom
	}
	else
	{
		for (int i = 0; i < 3; i++)
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (pm->watertype & CONTENTS_LADDER) //ladder
	{
		if (wishspeed > pm->ps->speed * pm_ladderScale)
		{
			wishspeed = pm->ps->speed * pm_ladderScale;
		}
		PM_Accelerate(wishdir, wishspeed, pm_Laddeeraccelerate);
	}
	else
	{
		if (pm->ps->gravity < 0)
		{
			//float up
			pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
		}

		if (wishspeed > pm->ps->speed * pm_swimScale)
		{
			wishspeed = pm->ps->speed * pm_swimScale;
		}

		PM_Accelerate(wishdir, wishspeed, pm_wateraccelerate);
	}

	// make sure we can go up slopes easily under water
	if (pml.groundPlane && DotProduct(pm->ps->velocity, pml.groundTrace.plane.normal) < 0)
	{
		const float vel = VectorLength(pm->ps->velocity);
		// slide along the ground plane
		PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,
			pm->ps->velocity, OVERCLIP);

		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	PM_SlideMove(qfalse);

	if (pm->ps->weapon == WP_SABER && !pm->ps->saberHolstered)
	{
		pm->ps->saberHolstered = 2;
#ifdef _GAME
		gentity_t* self = &g_entities[pm->ps->clientNum];
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberoff.mp3"));
#endif
	}
	else
	{
		if (PM_IsGunner())
		{
#ifdef _GAME
			gentity_t* self = &g_entities[pm->ps->clientNum];
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/change.wav"));
#endif
		}
	}
}

/*
===================
PM_FlyVehicleMove

===================
*/
static void PM_FlyVehicleMove(void)
{
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;

	// We don't use these here because we pre-calculate the movedir in the vehicle update anyways, and if
	// you leave this, you get strange motion during boarding (the player can move the vehicle).
	//fmove = pm->cmd.forwardmove;
	//smove = pm->cmd.rightmove;

	// normal slowdown
	if (pm->ps->gravity && pm->ps->velocity[2] < 0 && pm->ps->groundEntityNum == ENTITYNUM_NONE)
	{
		//falling
		const float zVel = pm->ps->velocity[2];
		PM_Friction();
		pm->ps->velocity[2] = zVel;
	}
	else
	{
		PM_Friction();
		if (pm->ps->velocity[2] < 0 && pm->ps->groundEntityNum != ENTITYNUM_NONE)
		{
			pm->ps->velocity[2] = 0; // ignore slope movement
		}
	}

	const float scale = PM_CmdScale(&pm->cmd);

	// Get The WishVel And WishSpeed
	//-------------------------------
	if (pm->ps->clientNum >= MAX_CLIENTS)
	{
		//NPC
		// If The UCmds Were Set, But Never Converted Into A MoveDir, Then Make The WishDir From UCmds
		//--------------------------------------------------------------------------------------------
		{
			wishspeed = pm->ps->speed;
			VectorScale(pm->ps->moveDir, pm->ps->speed, wishvel);
			VectorCopy(pm->ps->moveDir, wishdir);
		}
	}
	else
	{
		for (int i = 0; i < 3; i++)
		{
			const float smove = 0.0f;
			const float fmove = 0.0f;
			wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
		}
		// when going up or down slopes the wish velocity should Not be zero
		//	wishvel[2] = 0;

		VectorCopy(wishvel, wishdir);
		wishspeed = VectorNormalize(wishdir);
		wishspeed *= scale;
	}

	// Handle negative speed.
	if (wishspeed < 0)
	{
		wishspeed = wishspeed * -1.0f;
		VectorScale(wishvel, -1.0f, wishvel);
		VectorScale(wishdir, -1.0f, wishdir);
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	PM_Accelerate(wishdir, wishspeed, 100);

	PM_StepSlideMove(qtrue);
}

/*
===================
PM_GrappleMove

===================
*/
static void PM_GrappleMove(void)
{
	vec3_t vel, v;
	const int anim = BOTH_FORCEINAIR1;

	VectorScale(pml.forward, -16, v);
	VectorAdd(pm->ps->lastHitLoc, v, v);
	VectorSubtract(v, pm->ps->origin, vel);
	const float vlen = VectorLength(vel);
	VectorNormalize(vel);

	if (pm->cmd.buttons & BUTTON_ATTACK || pm->cmd.buttons & BUTTON_ALT_ATTACK)
	{
		return;
	}

	PM_SetAnim(SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLDLESS);

	if (pm->ps->userFloat1 == 0)
	{
		if (vlen <= 100)
		{
			VectorScale(vel, 10 * vlen, vel);
		}
		else
		{
			VectorScale(vel, 800, vel);
		}

		VectorCopy(vel, pm->ps->velocity);
		pml.groundPlane = qfalse;
	}
	else
	{
		if (vlen > pm->ps->userFloat1 && !(PM_InSpecialJump(pm->ps->torsoAnim) || pm->ps->pm_flags &
			PMF_STUCK_TO_WALL))
		{
			VectorSubtract(pm->ps->lastHitLoc, pm->ps->origin, vel);
			VectorNormalize(vel);
			VectorMA(pm->ps->origin, vlen - pm->ps->userFloat1, vel, pm->ps->origin);
			VectorScale(vel, vlen - pm->ps->userFloat1, vel);
			VectorAdd(pm->ps->velocity, vel, pm->ps->velocity);
			pm->ps->velocity[2] = 0;
		}
	}
}

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove(void)
{
	vec3_t wishvel;
	vec3_t wishdir;

	// normal slowdown
	PM_Friction();

	float scale = PM_CmdScale(&pm->cmd);

	if (pm->ps->pm_type == PM_SPECTATOR && pm->cmd.buttons & BUTTON_ALT_ATTACK)
	{
		//turbo boost
		scale *= 10;
	}

	//
	// user intentions
	//
	if (!scale)
	{
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = pm->ps->speed * (pm->cmd.upmove / 127.0f);
	}
	else
	{
		for (int i = 0; i < 3; i++)
		{
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;
		}

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy(wishvel, wishdir);
	const float wishspeed = VectorNormalize(wishdir);

	PM_Accelerate(wishdir, wishspeed, pm_flyaccelerate);

	PM_StepSlideMove(qfalse);
}

/*
===================
PM_AirMove

===================
*/
static void PM_AirMove(void)
{
	int i;
	vec3_t wishvel;
	vec3_t wishdir;
	float wishspeed;
	usercmd_t cmd;
	Vehicle_t* p_veh = NULL;

	if (pm->ps->clientNum >= MAX_CLIENTS)
	{
		const bgEntity_t* pEnt = pm_entSelf;

		if (pEnt && pEnt->s.NPC_class == CLASS_VEHICLE)
		{
			p_veh = pEnt->m_pVehicle;
		}
	}

	float jumpMultiplier;
	if (pm->ps->weapon == WP_MELEE)
	{
		jumpMultiplier = 5.0;
	}
	else
	{
		jumpMultiplier = 1.0;
	}

	if (pm->ps->pm_type != PM_SPECTATOR)
	{
#if METROID_JUMP
		pm_check_jump();
#else
		if (pm->ps->fd.forceJumpZStart &&
			pm->ps->forceJumpFlip)
		{
			PM_CheckJump();
		}
#endif
		PM_CheckGrab();
	}
	PM_Friction();

	float fmove = pm->cmd.forwardmove;
	float smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	float scale = PM_CmdScale(&cmd);

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);

	if (p_veh && p_veh->m_pVehicleInfo->hoverHeight > 0)
	{
		//in a hovering vehicle, have air control
		if (1)
		{
			wishspeed = pm->ps->speed;
			VectorScale(pm->ps->moveDir, pm->ps->speed, wishvel);
			VectorCopy(pm->ps->moveDir, wishdir);
			scale = 1.0f;
		}
#if 0
		else
		{
			float controlMod = 1.0f;
			if (pml.groundPlane)
			{//on a slope of some kind, shouldn't have much control and should slide a lot
				controlMod = pml.groundTrace.plane.normal[2];
			}

			vec3_t	vfwd, vrt;
			vec3_t	vAngles;

			VectorCopy(p_veh->m_vOrientation, vAngles);
			vAngles[ROLL] = 0;//since we're a hovercraft, we really don't want to stafe up into the air if we're banking
			AngleVectors(vAngles, vfwd, vrt, NULL);

			float speed = pm->ps->speed;
			float strafeSpeed = 0;

			if (fmove < 0)
			{//going backwards
				if (speed < 0)
				{//speed is negative, but since our command is reverse, make speed positive
					speed = fabs(speed);
				}
				else if (speed > 0)
				{//trying to move back but speed is still positive, so keep moving forward (we'll slow down eventually)
					speed = 0;
				}
			}

			if (pm->ps->clientNum < MAX_CLIENTS)
			{//do normal adding to wishvel
				VectorScale(vfwd, speed * controlMod * (fmove / 127.0f), wishvel);
				//just add strafing
				if (p_veh->m_pVehicleInfo->strafePerc)
				{//we can strafe
					if (smove)
					{//trying to strafe
						float minSpeed = p_veh->m_pVehicleInfo->speedMax * 0.5f * p_veh->m_pVehicleInfo->strafePerc;
						strafeSpeed = fabs(DotProduct(pm->ps->velocity, vfwd)) * p_veh->m_pVehicleInfo->strafePerc;
						if (strafeSpeed < minSpeed)
						{
							strafeSpeed = minSpeed;
						}
						strafeSpeed *= controlMod * ((float)(smove)) / 127.0f;
						if (strafeSpeed < 0)
						{//pm_accelerate does not understand negative numbers
							strafeSpeed *= -1;
							VectorScale(vrt, -1, vrt);
						}
						//now just add it to actual velocity
						PM_Accelerate(vrt, strafeSpeed, p_veh->m_pVehicleInfo->traction);
					}
				}
			}
			else
			{
				if (p_veh->m_pVehicleInfo->strafePerc)
				{//we can strafe
					if (pm->ps->clientNum)
					{//alternate control scheme: can strafe
						if (smove)
						{
							strafeSpeed = ((float)(smove)) / 127.0f;
						}
					}
				}
				//strafing takes away from forward speed
				VectorScale(vfwd, (fmove / 127.0f) * (1.0f - p_veh->m_pVehicleInfo->strafePerc), wishvel);
				if (strafeSpeed)
				{
					VectorMA(wishvel, strafeSpeed * p_veh->m_pVehicleInfo->strafePerc, vrt, wishvel);
				}
				VectorNormalize(wishvel);
				VectorScale(wishvel, speed * controlMod, wishvel);
			}
		}
#endif
	}
	else if (gPMDoSlowFall)
	{
		//no air-control
		VectorClear(wishvel);
	}
	else if (pm->ps->pm_type == PM_JETPACK)
	{
		//reduced air control while not jetting
		if (!scale)
		{
			//just up/down movement
			wishvel[0] = 0;
			wishvel[1] = 0;
			wishvel[2] = pm->ps->speed * (pm->cmd.upmove / 127.0f) * 1.5;
		}
		else
		{
			//some x/y movement
			for (i = 0; i < 3; i++)
			{
				if (i == 2)
				{
					wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove
						* 1.6;
				}
				else
				{
					wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove
						* 1.2;
				}
			}

			wishvel[2] += scale * pm->cmd.upmove;
		}

		VectorScale(wishvel, 1.5f, wishvel);
	}
	else
	{
		for (i = 0; i < 2; i++)
		{
			wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
		}
		wishvel[2] = 0;
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (pm->ps->pm_type != PM_JETPACK || scale)
	{
		wishspeed *= scale;
	}

	float accelerate = pm_airaccelerate;
	if (p_veh && p_veh->m_pVehicleInfo->type == VH_SPEEDER)
	{
		//speeders have more control in air
		//in mid-air
		accelerate = p_veh->m_pVehicleInfo->traction;
		if (pml.groundPlane)
		{
			//on a slope of some kind, shouldn't have much control and should slide a lot
			accelerate *= 0.5f;
		}
	}
	// not on ground, so little effect on velocity
	PM_Accelerate(wishdir, wishspeed, accelerate);

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if (pml.groundPlane)
	{
		if (!(pm->ps->pm_flags & PMF_STUCK_TO_WALL))
		{
			//don't slide when stuck to a wall
			if (PM_GroundSlideOkay(pml.groundTrace.plane.normal[2]))
			{
				PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP);
			}
		}
	}

	if (pm->ps->clientNum < MAX_CLIENTS
		&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0
		&& pm->ps->fd.forceJumpZStart
		&& pm->ps->velocity[2] > 0)
	{
		//I am force jumping and I'm not holding the button anymore
		const float curHeight = pm->ps->origin[2] - pm->ps->fd.forceJumpZStart + pm->ps->velocity[2] * pml.frametime;
		float maxJumpHeight = forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] * jumpMultiplier;

		if (pm->ps->weapon == WP_MELEE)
		{
			maxJumpHeight *= 30;
		}
		if (curHeight >= maxJumpHeight)
		{
			//reached top, cut velocity
			pm->ps->velocity[2] = 0;
		}
	}

	if (pm->ps->pm_flags & PMF_STUCK_TO_WALL)
	{
		//no grav when stuck to wall
		PM_StepSlideMove(qfalse);
	}
	else
	{
		PM_StepSlideMove(qtrue);
	}
}

extern qboolean PM_Bobaspecialanim(int anim);

static void PM_JetPackAnim(void)
{
	int anim = BOTH_FORCEJUMP1;

	if (!PM_Bobaspecialanim(pm->ps->torsoAnim))
	{
		if (pm->cmd.rightmove > 0)
		{
			if (pm->ps->fd.forcePowersActive & 1 << FP_LIGHTNING
				|| pm->ps->fd.forcePowersActive & 1 << FP_GRIP
				|| (pm->cmd.buttons & BUTTON_FORCEPOWER
					|| pm->cmd.buttons & BUTTON_FORCE_LIGHTNING
					|| pm->cmd.buttons & BUTTON_FORCEGRIP))
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_FLAMETHROWER, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else
			{
				anim = BOTH_FORCEJUMPRIGHT1;
			}
		}
		else if (pm->cmd.rightmove < 0)
		{
			if (pm->ps->fd.forcePowersActive & 1 << FP_LIGHTNING
				|| pm->ps->fd.forcePowersActive & 1 << FP_GRIP
				|| (pm->cmd.buttons & BUTTON_FORCEPOWER
					|| pm->cmd.buttons & BUTTON_FORCE_LIGHTNING
					|| pm->cmd.buttons & BUTTON_FORCEGRIP))
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_FLAMETHROWER, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else
			{
				anim = BOTH_FORCEJUMPLEFT1;
			}
		}
		else if (pm->cmd.forwardmove > 0)
		{
			if (pm->ps->weapon == WP_SABER) //saber out
			{
				if (pm->ps->fd.forcePowersActive & 1 << FP_LIGHTNING
					|| pm->ps->fd.forcePowersActive & 1 << FP_GRIP
					|| (pm->cmd.buttons & BUTTON_FORCEPOWER
						|| pm->cmd.buttons & BUTTON_FORCE_LIGHTNING
						|| pm->cmd.buttons & BUTTON_FORCEGRIP))
				{
					PM_SetAnim(SETANIM_TORSO, BOTH_FLAMETHROWER, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					anim = BOTH_FORCEJUMP1;
				}
			}
			else
			{
				if (pm->ps->fd.forcePowersActive & 1 << FP_LIGHTNING
					|| pm->ps->fd.forcePowersActive & 1 << FP_GRIP
					|| (pm->cmd.buttons & BUTTON_FORCEPOWER
						|| pm->cmd.buttons & BUTTON_FORCE_LIGHTNING
						|| pm->cmd.buttons & BUTTON_FORCEGRIP))
				{
					PM_SetAnim(SETANIM_TORSO, BOTH_FLAMETHROWER, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					anim = BOTH_FORCEJUMP1;
				}
			}
		}
		else if (pm->cmd.forwardmove < 0)
		{
			if (pm->ps->fd.forcePowersActive & 1 << FP_LIGHTNING
				|| pm->ps->fd.forcePowersActive & 1 << FP_GRIP
				|| (pm->cmd.buttons & BUTTON_FORCEPOWER
					|| pm->cmd.buttons & BUTTON_FORCE_LIGHTNING
					|| pm->cmd.buttons & BUTTON_FORCEGRIP))
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_FLAMETHROWER, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else
			{
				anim = BOTH_INAIRBACK1;
			}
		}
		else
		{
			if (pm->ps->fd.forcePowersActive & 1 << FP_LIGHTNING
				|| pm->ps->fd.forcePowersActive & 1 << FP_GRIP
				|| (pm->cmd.buttons & BUTTON_FORCEPOWER
					|| pm->cmd.buttons & BUTTON_FORCE_LIGHTNING
					|| pm->cmd.buttons & BUTTON_FORCEGRIP))
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_FLAMETHROWER, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else
			{
				anim = BOTH_FORCEJUMP1;
			}
		}
		int parts = SETANIM_BOTH;

		if (pm->ps->weaponTime)
		{
			parts = SETANIM_LEGS;
		}

		PM_SetAnim(parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
}

static int PM_ForceJumpAnimForJumpAnim(int anim)
{
	switch (anim)
	{
	case BOTH_JUMP1: //# Jump - wind-up and leave ground
		if (pm->ps->weapon == WP_SABER) //saber out
		{
			anim = BOTH_FORCEJUMP2; //# Jump - wind-up and leave ground
		}
		else
		{
			anim = BOTH_FORCEJUMP1; //# Jump - wind-up and leave ground
		}
		break;
	case BOTH_JUMP2: //# Jump - wind-up and leave ground
		if (pm->ps->weapon == WP_SABER) //saber out
		{
			anim = BOTH_FORCEJUMP2; //# Jump - wind-up and leave ground
		}
		else
		{
			anim = BOTH_FORCEJUMP1; //# Jump - wind-up and leave ground
		}
		break;
	case BOTH_INAIR1: //# In air loop (from jump)
		anim = BOTH_FORCEINAIR1; //# In air loop (from jump)
		break;
	case BOTH_LAND1: //# Landing (from in air loop)
		anim = BOTH_FORCELAND1; //# Landing (from in air loop)
		break;
	case BOTH_JUMPBACK1: //# Jump backwards - wind-up and leave ground
		anim = BOTH_FORCEJUMPBACK1; //# Jump backwards - wind-up and leave ground
		break;
	case BOTH_INAIRBACK1: //# In air loop (from jump back)
		anim = BOTH_FORCEINAIRBACK1; //# In air loop (from jump back)
		break;
	case BOTH_LANDBACK1: //# Landing backwards(from in air loop)
		anim = BOTH_FORCELANDBACK1; //# Landing backwards(from in air loop)
		break;
	case BOTH_JUMPLEFT1: //# Jump left - wind-up and leave ground
		anim = BOTH_FORCEJUMPLEFT1; //# Jump left - wind-up and leave ground
		break;
	case BOTH_INAIRLEFT1: //# In air loop (from jump left)
		anim = BOTH_FORCEINAIRLEFT1; //# In air loop (from jump left)
		break;
	case BOTH_LANDLEFT1: //# Landing left(from in air loop)
		anim = BOTH_FORCELANDLEFT1; //# Landing left(from in air loop)
		break;
	case BOTH_JUMPRIGHT1: //# Jump right - wind-up and leave ground
		anim = BOTH_FORCEJUMPRIGHT1; //# Jump right - wind-up and leave ground
		break;
	case BOTH_INAIRRIGHT1: //# In air loop (from jump right)
		anim = BOTH_FORCEINAIRRIGHT1; //# In air loop (from jump right)
		break;
	case BOTH_LANDRIGHT1: //# Landing right(from in air loop)
		anim = BOTH_FORCELANDRIGHT1; //# Landing right(from in air loop)
		break;
	default:;
	}
	return anim;
}

qboolean BG_AllowThirdPersonSpecialMove(const playerState_t* ps)
{
	return (ps->weapon == WP_SABER || ps->weapon == WP_MELEE) && !pm->ps->zoomMode;
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove(void)
{
	int i;
	vec3_t wishvel;
	vec3_t wishdir;
	float wishspeed = 0.0f;
	usercmd_t cmd;
	float accelerate;
	qboolean npcMovement = qfalse;

	if (pm->waterlevel > 2 && DotProduct(pml.forward, pml.groundTrace.plane.normal) > 0)
	{
		if (pm->watertype & CONTENTS_LADDER)
		{
			// begin climbing
			PM_LadderMove();
		}
		else
		{
			// begin swimming
			PM_WaterMove();
		}
		return;
	}

	if (pm->ps->pm_type != PM_SPECTATOR)
	{
		if (pm_check_jump())
		{
			// jumped away
			if (pm->waterlevel > 1)
			{
				if (pm->watertype & CONTENTS_LADDER)
				{
					// begin climbing
					PM_LadderMove();
				}
				else
				{
					// begin swimming
					PM_WaterMove();
				}
			}
			else
			{
				PM_AirMove();
			}
			return;
		}
	}

	PM_Friction();

	const float fmove = pm->cmd.forwardmove;
	const float smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	const float scale = PM_CmdScale(&cmd);

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity(pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP);
	PM_ClipVelocity(pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP);
	//
	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);

	// Get The WishVel And WishSpeed
	//-------------------------------
	if (pm->ps->clientNum >= MAX_CLIENTS && !VectorCompare(pm->ps->moveDir, vec3_origin))
	{
		//NPC
		const bgEntity_t* pEnt = pm_entSelf;

		if (pEnt && pEnt->s.NPC_class == CLASS_VEHICLE)
		{
			// If The UCmds Were Set, But Never Converted Into A MoveDir, Then Make The WishDir From UCmds
			//--------------------------------------------------------------------------------------------
			if ((fmove != 0.0f || smove != 0.0f) && VectorCompare(pm->ps->moveDir, vec3_origin))
			{
				//trap->Printf("Generating MoveDir\n");
				for (i = 0; i < 3; i++)
				{
					wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
				}

				VectorCopy(wishvel, wishdir);
				wishspeed = VectorNormalize(wishdir);
				wishspeed *= scale;
			}
			// Otherwise, Use The Move Dir
			//-----------------------------
			else
			{
				//wishspeed = pm->ps->speed;
				VectorScale(pm->ps->moveDir, pm->ps->speed, wishvel);
				VectorCopy(wishvel, wishdir);
				wishspeed = VectorNormalize(wishdir);
			}

			npcMovement = qtrue;
		}
	}

	if (!npcMovement)
	{
		for (i = 0; i < 3; i++)
		{
			wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
		}
		// when going up or down slopes the wish velocity should Not be zero

		VectorCopy(wishvel, wishdir);
		wishspeed = VectorNormalize(wishdir);
		wishspeed *= scale;
	}

	// clamp the speed lower if ducking
	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		if (wishspeed > pm->ps->speed * pm_duckScale)
		{
			wishspeed = pm->ps->speed * pm_duckScale;
		}
	}
	else if (pm->ps->pm_flags & PMF_ROLLING && !BG_InRoll(pm->ps, pm->ps->legsAnim) &&
		!PM_InRollComplete(pm->ps, pm->ps->legsAnim))
	{
		if (wishspeed > pm->ps->speed * pm_duckScale)
		{
			wishspeed = pm->ps->speed * pm_duckScale;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if (pm->waterlevel)
	{
		float waterScale = pm->waterlevel / 3.0;
		waterScale = 1.0 - (1.0 - pm_swimScale) * waterScale;
		if (wishspeed > pm->ps->speed * waterScale)
		{
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if (pm_flying == FLY_HOVER)
	{
		accelerate = pm_vehicleaccelerate;
	}
	else if (pm->ps->pm_flags & PMF_TIME_KNOCKBACK || pm->ps->pm_flags & PMF_TIME_NOFRICTION)
	{
		accelerate = pm_airaccelerate;
	}
	else if (pml.groundTrace.surfaceFlags & SURF_SLICK)
	{
		if (!(pm->cmd.buttons & BUTTON_WALKING))
		{
			accelerate = 0.1f;

			if (!PM_SpinningSaberAnim(pm->ps->legsAnim)
				&& !PM_FlippingAnim(pm->ps->legsAnim)
				&& !PM_RollingAnim(pm->ps->legsAnim)
				&& !PM_InKnockDown(pm->ps))
			{
				int knock_anim; //default knockdown
				if (Q_irand(0, 3) > 2)
				{
					knock_anim = BOTH_KNOCKDOWN1;
				}
				else
				{
					knock_anim = BOTH_KNOCKDOWN4;
				}

				PM_SetAnim(SETANIM_BOTH, knock_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
				pm->ps->legsTimer += 1000;
				pm->ps->torsoTimer += 1000;

				if (PM_InOnGroundAnim(pm->ps->legsAnim) || PM_InKnockDown(pm->ps))
				{
					pm->ps->legsTimer += 1000;
					pm->ps->torsoTimer += 1000;
				}
			}
		}
		else
		{
			accelerate = pm_airaccelerate;
		}
	}
	else
	{
		accelerate = pm_accelerate;

		// Wind Affects Acceleration
		//===================================================
		/*if (wishspeed > 0.0f && pm->gent && !pml.walking)
		{
			if (gi.WE_GetWindGusting(pm->gent->currentOrigin))
			{
				vec3_t	windDir;
				if (gi.WE_GetWindVector(windDir, pm->gent->currentOrigin))
				{
					if (gi.WE_IsOutside(pm->gent->currentOrigin))
					{
						VectorScale(windDir, -1.0f, windDir);
						accelerate *= (1.0f - (DotProduct(wishdir, windDir) * 0.55f));
					}
				}
			}
		}*/
	}

	PM_Accelerate(wishdir, wishspeed, accelerate);

	if (pml.groundTrace.surfaceFlags & SURF_SLICK || pm->ps->pm_flags & PMF_TIME_KNOCKBACK || pm->ps->pm_flags &
		PMF_TIME_NOFRICTION)
	{
		if (pm->ps->gravity >= 0 && pm->ps->groundEntityNum != ENTITYNUM_NONE && !VectorLengthSquared(pm->ps->velocity)
			&& pml.groundTrace.plane.normal[2] == 1.0)
		{
			//on ground and not moving and on level ground, no reason to do stupid fucking gravity with the clipvelocity!!!!
		}
		else
		{
			//if (!(pm->ps->eFlags & EF_FORCE_GRIPPED) && !(pm->ps->eFlags & EF_FORCE_DRAINED) && !(pm->ps->eFlags & EF_FORCE_GRABBED))
			//{
			pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
			//}
		}
	}

	const float vel = VectorLength(pm->ps->velocity);

	// slide along the ground plane
	PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,
		pm->ps->velocity, OVERCLIP);

	// don't decrease velocity when going up or down a slope
	VectorNormalize(pm->ps->velocity);
	VectorScale(pm->ps->velocity, vel, pm->ps->velocity);

	// don't do anything if standing still
	if (!pm->ps->velocity[0] && !pm->ps->velocity[1])
	{
		return;
	}

	if (pm->ps->gravity <= 0)
	{
		//need to apply gravity since we're going to float up from ground
		PM_StepSlideMove(qtrue);
	}
	else
	{
		PM_StepSlideMove(qfalse);
	}
}

/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove(void)
{
	if (!pml.walking)
	{
		return;
	}

	// extra friction

	float forward = VectorLength(pm->ps->velocity);
	forward -= 20;
	if (forward <= 0)
	{
		VectorClear(pm->ps->velocity);
	}
	else
	{
		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, forward, pm->ps->velocity);
	}
}

/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove(void)
{
	vec3_t wishvel;
	vec3_t wishdir;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	const float speed = VectorLength(pm->ps->velocity);
	if (speed < 1)
	{
		VectorCopy(vec3_origin, pm->ps->velocity);
	}
	else
	{
		float drop = 0;

		const float friction = pm_friction * 1.5; // extra friction
		const float control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control * friction * pml.frametime;

		// scale the velocity
		float newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale(pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	float scale = PM_CmdScale(&pm->cmd);
	if (pm->cmd.buttons & BUTTON_ATTACK)
	{
		//turbo boost
		scale *= 10;
	}
	if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
	{
		//turbo boost
		scale *= 10;
	}

	const float fmove = pm->cmd.forwardmove;
	const float smove = pm->cmd.rightmove;

	for (int i = 0; i < 3; i++)
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	wishvel[2] += pm->cmd.upmove;

	VectorCopy(wishvel, wishdir);
	float wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	PM_Accelerate(wishdir, wishspeed, pm_accelerate);

	// move
	VectorMA(pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

//============================================================================

#ifdef _GAME
static float PM_DamageForDelta(const int delta)
{
	float damage = delta;
	const gentity_t* self = &g_entities[pm->ps->clientNum];
	if (self->NPC)
	{
		if (pm->ps->weapon == WP_SABER || self->client && self->client->NPC_class == CLASS_REBORN)
		{
			//FIXME: for now Jedi take no falling damage, but really they should if pushed off?
			damage = 0;
		}
	}
	else if (pm->ps->clientNum < MAX_CLIENTS)
	{
		if (damage < 50)
		{
			if (damage > 24)
			{
				damage = damage - 25;
			}
		}
		else
		{
			damage *= 0.5f;
		}
	}
	return damage * 0.5f;
}

static void PM_CrashLandDamage(int damage)
{
	gentity_t* self = &g_entities[pm_entSelf->s.number];
	if (self)
	{
		if (self->NPC && self->NPC->aiFlags & NPCAI_DIE_ON_IMPACT)
		{
			damage = 1000;
		}
		else if (!(self->flags & FL_NO_IMPACT_DMG))
		{
			damage = PM_DamageForDelta(damage);
		}

		if (damage)
		{
			const int dflags = DAMAGE_NO_ARMOR;
			self->painDebounceTime = level.time + 200;
			G_Damage(self, NULL, NULL, NULL, NULL, damage, dflags, MOD_FALLING);
		}
	}
}

static float PM_CrashLandDelta(vec3_t prev_vel, int waterlevel)
{
	if (pm->waterlevel == 3)
	{
		return 0;
	}
	float delta = fabs(prev_vel[2]) / 10;

	if (pm->waterlevel == 2)
	{
		delta *= 0.25;
	}
	if (pm->waterlevel == 1)
	{
		delta *= 0.5;
	}

	return delta;
}
#endif

/*
================
PM_FootstepForSurface

Returns an event number appropriate for the groundsurface
================
*/
static int PM_FootstepForSurface(void)
{
	if (pml.groundTrace.surfaceFlags & SURF_NOSTEPS)
	{
		return 0;
	}
	return pml.groundTrace.surfaceFlags & MATERIAL_MASK;
}

extern qboolean PM_CanRollFromSoulCal(const playerState_t* ps);

static int pm_try_roll(void)
{
	trace_t trace;
	int anim = -1;
	vec3_t fwd, right, traceto, mins, maxs, fwdAngles;
	const float rollDist = 192; //was 64;

	if (PM_SaberInAttack(pm->ps->saber_move) || pm_saber_in_special_attack(pm->ps->torsoAnim)
		|| PM_SpinningSaberAnim(pm->ps->legsAnim)
		|| PM_SaberInStart(pm->ps->saber_move))
	{
		//attacking or spinning (or, if player, starting an attack)
		if (PM_CanRollFromSoulCal(pm->ps))
		{
			//hehe
		}
		else
		{
			return 0;
		}
	}

	if (PM_InRoll(pm->ps))
	{
		return qfalse;
	}

	if (PM_IsRocketTrooper())
	{
		//Not using saber, or can't use jump
		return 0;
	}

	if (pm->ps->weapon == WP_SABER)
	{
		const saberInfo_t* saber = BG_MySaber(pm->ps->clientNum, 0);
		if (saber
			&& saber->saberFlags & SFL_NO_ROLLS)
		{
			return 0;
		}
		saber = BG_MySaber(pm->ps->clientNum, 1);
		if (saber
			&& saber->saberFlags & SFL_NO_ROLLS)
		{
			return 0;
		}
	}

	VectorSet(mins, pm->mins[0], pm->mins[1], pm->mins[2] + STEPSIZE);
	VectorSet(maxs, pm->maxs[0], pm->maxs[1], pm->ps->crouchheight);
	VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);

	AngleVectors(fwdAngles, fwd, right, NULL);

#ifdef _GAME
	gentity_t* npc = &g_entities[pm->ps->clientNum];
	if (npc->npc_roll_start)
	{
		if (npc->npc_roll_direction == EVASION_ROLL_DIR_BACK)
		{
			// backward roll
			anim = BOTH_ROLL_B;
			VectorMA(pm->ps->origin, -rollDist, fwd, traceto);
		}
		else if (npc->npc_roll_direction == EVASION_ROLL_DIR_RIGHT)
		{
			//right
			anim = BOTH_ROLL_R;
			VectorMA(pm->ps->origin, rollDist, right, traceto);
		}
		else if (npc->npc_roll_direction == EVASION_ROLL_DIR_LEFT)
		{
			//left
			anim = BOTH_ROLL_L;
			VectorMA(pm->ps->origin, -rollDist, right, traceto);
		}
		// No matter what, re-initialize the roll flag, so we dont end up with npcs rolling all around the map :)
		npc->npc_roll_start = qfalse;
	}
#endif //_GAME

	if (pm->cmd.forwardmove)
	{
		if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
		{
			anim = BOTH_ROLL_B;
			VectorMA(pm->ps->origin, -rollDist, fwd, traceto);
		}
		else
		{
			if (pm->ps->weapon == WP_SABER)
			{
				anim = BOTH_ROLL_F;
				VectorMA(pm->ps->origin, rollDist, fwd, traceto);
			}
			else if (pm->ps->weapon == WP_MELEE)
			{
				anim = BOTH_ROLL_F2;
				VectorMA(pm->ps->origin, rollDist, fwd, traceto);
			}
			else
			{
				//GUNNERS WILL DO THIS
				anim = BOTH_ROLL_F1;
				VectorMA(pm->ps->origin, rollDist, fwd, traceto);
			}
		}
	}
	else if (pm->cmd.rightmove > 0)
	{
		anim = BOTH_ROLL_R;
		VectorMA(pm->ps->origin, rollDist, right, traceto);
	}
	else if (pm->cmd.rightmove < 0)
	{
		anim = BOTH_ROLL_L;
		VectorMA(pm->ps->origin, -rollDist, right, traceto);
	}
	else
	{
		//???
	}

	if (anim != -1)
	{
		//We want to roll. Perform a trace to see if we can, and if so, send us into one.
		pm->trace(&trace, pm->ps->origin, mins, maxs, traceto, pm->ps->clientNum, CONTENTS_SOLID);
		if (trace.fraction >= 1.0f)
		{
			pm->ps->saber_move = LS_NONE;
			return anim;
		}
	}
	return 0;
}

#ifdef _GAME
static void PM_CrashLandEffect(void)
{
	if (pm->waterlevel)
	{
		return;
	}
	const float delta = fabs(pml.previous_velocity[2]) / 10;
	if (delta >= 30)
	{
		vec3_t bottom;
		int effectID = -1;
		const int material = pml.groundTrace.surfaceFlags & MATERIAL_MASK;
		VectorSet(bottom, pm->ps->origin[0], pm->ps->origin[1], pm->ps->origin[2] + pm->mins[2] + 1);
		switch (material)
		{
		case MATERIAL_MUD:
			effectID = EFFECT_LANDING_MUD;
			break;
		case MATERIAL_SAND:
			effectID = EFFECT_LANDING_SAND;
			break;
		case MATERIAL_DIRT:
			effectID = EFFECT_LANDING_DIRT;
			break;
		case MATERIAL_SNOW:
			effectID = EFFECT_LANDING_SNOW;
			break;
		case MATERIAL_GRAVEL:
			effectID = EFFECT_LANDING_GRAVEL;
			break;
		case MATERIAL_LAVA:
			effectID = EFFECT_LANDING_SAND;
			break;
		default:;
		}

		if (effectID != -1)
		{
			G_PlayEffect(effectID, bottom, pml.groundTrace.plane.normal);
		}
	}
}
#endif
/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand(void)
{
	float delta;
	float dist;
	float vel, acc;
	float t;
	float a, b, c, den;
	qboolean didRoll = qfalse;

	//falling to death NPCs pavement smear.
#ifdef _GAME
	if (g_entities[pm->ps->clientNum].NPC && g_entities[pm->ps->clientNum].NPC->aiFlags & NPCAI_DIE_ON_IMPACT)
	{
		//have to do death on impact if we are falling to our death, FIXME: should we avoid any additional damage this func?
		PM_CrashLandDamage(1000);
	}
#endif

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den = b * b - 4 * a * c;
	if (den < 0)
	{
		pm->ps->inAirAnim = qfalse;
		return;
	}
	t = (-b - sqrt(den)) / (2 * a);

	delta = vel + t * acc;
	delta = delta * delta * 0.0001;

#ifdef _GAME
	PM_CrashLandEffect();
#endif
	// ducking while falling doubles damage
	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		delta *= 2;
	}

	if (pm->ps->legsAnim == BOTH_A7_KICK_F_AIR ||
		pm->ps->legsAnim == BOTH_A7_KICK_B_AIR ||
		pm->ps->legsAnim == BOTH_A7_KICK_R_AIR ||
		pm->ps->legsAnim == BOTH_A7_KICK_L_AIR)
	{
		int landAnim = -1;
		switch (pm->ps->legsAnim)
		{
		case BOTH_A7_KICK_F_AIR:
			landAnim = BOTH_FORCELAND1;
			break;
		case BOTH_A7_KICK_B_AIR:
			landAnim = BOTH_FORCELANDBACK1;
			break;
		case BOTH_A7_KICK_R_AIR:
			landAnim = BOTH_FORCELANDRIGHT1;
			break;
		case BOTH_A7_KICK_L_AIR:
			landAnim = BOTH_FORCELANDLEFT1;
			break;
		default:;
		}
		if (landAnim != -1)
		{
			if (pm->ps->torsoAnim == pm->ps->legsAnim)
			{
				PM_SetAnim(SETANIM_BOTH, landAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else
			{
				PM_SetAnim(SETANIM_LEGS, landAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
		}
	}
	else if (pm->ps->legsAnim == BOTH_FORCEJUMPLEFT1 ||
		pm->ps->legsAnim == BOTH_FORCEJUMPRIGHT1 ||
		pm->ps->legsAnim == BOTH_FORCEJUMPBACK1 ||
		pm->ps->legsAnim == BOTH_FORCEJUMP1 ||
		pm->ps->legsAnim == BOTH_FORCEJUMP2)
	{
		int fjAnim;
		switch (pm->ps->legsAnim)
		{
		case BOTH_FORCEJUMPLEFT1:
			fjAnim = BOTH_LANDLEFT1;
			break;
		case BOTH_FORCEJUMPRIGHT1:
			fjAnim = BOTH_LANDRIGHT1;
			break;
		case BOTH_FORCEJUMPBACK1:
			fjAnim = BOTH_LANDBACK1;
			break;
		default:
			fjAnim = BOTH_LAND1;
			break;
		}
		PM_SetAnim(SETANIM_BOTH, fjAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	// decide which landing animation to use
	else if (!BG_InRoll(pm->ps, pm->ps->legsAnim) && pm->ps->inAirAnim && !pm->ps->m_iVehicleNum)
	{
		//only play a land animation if we transitioned into an in-air animation while off the ground
		if (!PM_SaberInSpecial(pm->ps->saber_move))
		{
			if (pm->ps->pm_flags & PMF_BACKWARDS_JUMP)
			{
				PM_ForceLegsAnim(BOTH_LANDBACK1);
			}
			else
			{
				PM_ForceLegsAnim(BOTH_LAND1);
			}
		}
	}

	if (pm->ps->weapon != WP_SABER && pm->ps->weapon != WP_MELEE && !PM_IsRocketTrooper())
	{
		//saber handles its own anims
		//This will push us back into our weaponready stance from the land anim.
		if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1)
		{
			PM_StartTorsoAnim(TORSO_WEAPONREADY4);
		}
		else if (pm_entSelf && pm_entSelf->s.botclass == BCLASS_SBD)
		{
			PM_StartTorsoAnim(SBD_WEAPON_STANDING);
		}
		else if (pm->ps->weapon == WP_BRYAR_OLD)
		{
			PM_StartTorsoAnim(SBD_WEAPON_STANDING);
		}
		else
		{
			if (pm->ps->weapon == WP_EMPLACED_GUN)
			{
				PM_StartTorsoAnim(BOTH_GUNSIT1);
			}
			else
			{
				if (PM_CanSetWeaponReadyAnim())
				{
					PM_StartTorsoAnim(PM_GetWeaponReadyAnim());
				}
			}
		}
	}

	if (!PM_InSpecialJump(pm->ps->legsAnim) ||
		pm->ps->legsTimer < 1 ||
		pm->ps->legsAnim == BOTH_WALL_RUN_LEFT ||
		pm->ps->legsAnim == BOTH_WALL_RUN_RIGHT)
	{
		//Only set the timer if we're in an anim that can be interrupted (this would not be, say, a flip)
		if (!BG_InRoll(pm->ps, pm->ps->legsAnim) && pm->ps->inAirAnim)
		{
			if (!PM_SaberInSpecial(pm->ps->saber_move) || pm->ps->weapon != WP_SABER)
			{
				if (pm->ps->legsAnim != BOTH_FORCELAND1 && pm->ps->legsAnim != BOTH_FORCELANDBACK1 &&
					pm->ps->legsAnim != BOTH_FORCELANDRIGHT1 && pm->ps->legsAnim != BOTH_FORCELANDLEFT1)
				{
					//don't override if we have started a force land
					pm->ps->legsTimer = TIMER_LAND;
				}
			}
		}
	}

	pm->ps->inAirAnim = qfalse;

	if (pm->ps->m_iVehicleNum)
	{
		//don't do fall stuff while on a vehicle
		return;
	}

	// never take falling damage if completely underwater
	if (pm->waterlevel == 3)
	{
		return;
	}

	// reduce falling damage if there is standing water
	if (pm->waterlevel == 2)
	{
		delta *= 0.25;
	}
	if (pm->waterlevel == 1)
	{
		delta *= 0.5;
	}

	if (delta < 1)
	{
		//sight/sound event for soft landing
#ifdef _GAME
		AddSoundEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 32, AEL_MINOR, qfalse, qtrue);
#endif
		return;
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		if (delta >= 2 && !PM_InOnGroundAnim(pm->ps->legsAnim) && !PM_InKnockDown(pm->ps) && !BG_InRoll(
			pm->ps, pm->ps->legsAnim) &&
			pm->ps->forceHandExtend == HANDEXTEND_NONE)
		{
			//roll!
			int anim = pm_try_roll();

			if (PM_InRollComplete(pm->ps, pm->ps->legsAnim))
			{
				anim = 0;
				pm->ps->legsTimer = 0;
				pm->ps->legsAnim = 0;
				PM_SetAnim(SETANIM_BOTH, BOTH_LAND1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->legsTimer = TIMER_LAND;
			}

			if (anim)
			{
				//absorb some impact
				pm->ps->legsTimer = 0;
				delta /= 3;
				// /= 2 just cancels out the above delta *= 2 when landing while crouched, the roll itself should absorb a little damage
				pm->ps->legsAnim = 0;
				if (pm->ps->torsoAnim == BOTH_A7_SOULCAL)
				{
					//get out of it on torso
					pm->ps->torsoTimer = 0;
				}
				PM_SetAnim(SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				didRoll = qtrue;
			}
		}
	}

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if (!(pml.groundTrace.surfaceFlags & SURF_NODAMAGE))
	{
		if (delta > 7)
		{
			int delta_send = (int)delta;

			if (delta_send > 600)
			{
				//will never need to know any value above this
				delta_send = 600;
			}

			if (pm->ps->fd.forceJumpZStart)
			{
				if ((int)pm->ps->origin[2] >= (int)pm->ps->fd.forceJumpZStart)
				{
					//was force jumping, landed on higher or same level as when force jump was started
					if (delta_send > 8)
					{
						delta_send = 8;
					}
				}
				else
				{
					if (delta_send > 8)
					{
						const int dif = (int)pm->ps->fd.forceJumpZStart - (int)pm->ps->origin[2];
						int dmgLess = forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] - dif;

						if (dmgLess < 0)
						{
							dmgLess = 0;
						}

						delta_send -= dmgLess * 0.3;

						if (delta_send < 8)
						{
							delta_send = 8;
						}
					}
				}
			}

			delta_send /= 2; //half it
			delta_send *= 3;

			if (didRoll)
			{
				//Add the appropriate event..
				PM_AddEventWithParm(EV_ROLL, delta_send);
			}
			else
			{
				PM_AddEventWithParm(EV_FALL, delta_send);
			}
		}
		else
		{
			if (didRoll)
			{
				PM_AddEventWithParm(EV_ROLL, 0);
			}
			else
			{
				PM_AddEventWithParm(EV_FOOTSTEP, PM_FootstepForSurface());
			}
		}
	}

	if (PM_InForceFall())
	{
		return;
	}

	VectorClear(pm->ps->velocity);
	// start footstep cycle over
	pm->ps->bobCycle = 0;
}

/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid(trace_t* trace)
{
	vec3_t point;

	if (pm->debugLevel)
	{
		Com_Printf("%i:allsolid\n", c_pmove);
	}

	// jitter around
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			for (int k = -1; k <= 1; k++)
			{
				VectorCopy(pm->ps->origin, point);
				point[0] += (float)i;
				point[1] += (float)j;
				point[2] += (float)k;
				pm->trace(trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
				if (!trace->allsolid)
				{
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace(trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}

/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
qboolean PM_KnockDownAnimExtended(int anim);
#ifdef _GAME
extern void NPC_RemoveBody(gentity_t* self);
extern qboolean FlyingCreature(gentity_t* ent);
#endif

static void PM_GroundTraceMissed(void)
{
	trace_t trace;
	vec3_t point;

	//while still holding your throat.
	if (pm->ps->pm_type == PM_FLOAT)
	{
		//we're assuming this is because you're being choked
		const int parts = SETANIM_LEGS;

		if (pm->ps->stats[STAT_HEALTH] > 50)
		{
			PM_SetAnim(parts, BOTH_CHOKE4, SETANIM_FLAG_OVERRIDE);
		}
		else if (pm->ps->stats[STAT_HEALTH] > 30)
		{
			PM_SetAnim(parts, BOTH_CHOKE3, SETANIM_FLAG_OVERRIDE);
		}
		else
		{
			PM_SetAnim(parts, BOTH_CHOKE1, SETANIM_FLAG_OVERRIDE);
		}
	}
	else if (pm->ps->pm_type == PM_JETPACK)
	{
		//jetpacking
	}
	//If the anim is choke3, act like we just went into the air because we aren't in a float
	else if (pm->ps->groundEntityNum != ENTITYNUM_NONE || pm->ps->legsAnim == BOTH_CHOKE3 || pm->ps->legsAnim ==
		BOTH_CHOKE4)
	{
		// we just transitioned into freefall
		if (pm->debugLevel)
		{
			Com_Printf("%i:lift\n", c_pmove);
		}
		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy(pm->ps->origin, point);
		point[2] -= 64;

		pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		if (trace.fraction == 1.0 || pm->ps->pm_type == PM_FLOAT)
		{
			if (pm->ps->velocity[2] <= 0 && !(pm->ps->pm_flags & PMF_JUMP_HELD))
			{
				PM_SetAnim(SETANIM_LEGS, BOTH_INAIR1, 0);
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			}
			else if (pm->cmd.forwardmove >= 0)
			{
				PM_SetAnim(SETANIM_LEGS, BOTH_JUMP1, SETANIM_FLAG_OVERRIDE);
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			}
			else
			{
				PM_SetAnim(SETANIM_LEGS, BOTH_JUMPBACK1, SETANIM_FLAG_OVERRIDE);
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}

			pm->ps->inAirAnim = qtrue;
		}
	}
	else if (pm->ps->legsAnim == BOTH_FORCELONGLEAP_START
		|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK
		|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK2
		|| pm->ps->legsAnim == BOTH_FORCEWALLRUNFLIP_START
		|| pm->ps->legsAnim == BOTH_FLIP_LAND)
	{
		//let it stay on this anim
	}
	else if (PM_KnockDownAnimExtended(pm->ps->legsAnim))
	{
		//no in-air anims when in knockdown anim
	}
	//handle NPCs falling to their deaths.
#ifdef _GAME
	else if (pm->ps->clientNum >= MAX_CLIENTS
		&& g_entities[pm->ps->clientNum].NPC
		&& g_entities[pm->ps->clientNum].client
		&& g_entities[pm->ps->clientNum].client->NPC_class != CLASS_DESANN) //desann never falls to his death
	{
		gentity_t* self = &g_entities[pm->ps->clientNum];
		if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
		{
			if (pm->ps->stats[STAT_HEALTH] > 0
				&& !(self->NPC->aiFlags & NPCAI_DIE_ON_IMPACT)
				&& self->NPC->jumpState != JS_JUMPING
				&& self->NPC->jumpState != JS_LANDING
				&& !(self->NPC->scriptFlags & SCF_NO_FALLTODEATH)
				&& self->NPC->behaviorState != BS_JUMP) //not being scripted to jump
			{
				if (level.time - self->client->respawnTime > 2000 //been in the world for at least 2 seconds
					&& (!self->NPC->timeOfDeath || level.time - self->NPC->timeOfDeath < 1000)
					&& self->think != NPC_RemoveBody
					//Have to do this now because timeOfDeath is used by NPC_RemoveBody to debounce removal checks
					&& !(self->client->ps.fd.forcePowersActive & 1 << FP_LEVITATION))
				{
					if (!FlyingCreature(self) && g_gravity.value > 0)
					{
						if (!(self->flags & FL_UNDYING) && !(self->flags & FL_GODMODE))
						{
							if (1)
							{
								if (!pm->ps->fd.forceJumpZStart || pm->ps->fd.forceJumpZStart > pm->ps->origin[2])
								{
									vec3_t vel;

									VectorCopy(pm->ps->velocity, vel);
									float speed = VectorLength(vel);
									if (!speed)
									{
										//damn divide by zero
										speed = 1;
									}
									float time = 400 / speed;
									vel[2] -= 0.5 * time * pm->ps->gravity;
									speed = VectorLength(vel);
									if (!speed)
									{
										//damn divide by zero
										speed = 1;
									}
									time = 400 / speed;
									VectorScale(vel, time, vel);
									VectorAdd(pm->ps->origin, vel, point);

									pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum,
										pm->tracemask);

									if (trace.contents & CONTENTS_LAVA)
									{
										//got out of it
									}
									else if (!trace.allsolid && !trace.startsolid && pm->ps->origin[2] - trace.endpos[2]
										>= 128) //>=128 so we don't die on steps!
									{
										if (trace.fraction == 1.0)
										{
											//didn't hit, we're probably going to die
											if (pm->ps->velocity[2] < 0 && pm->ps->origin[2] - point[2] > 256)
											{
												//going down, into a bottomless pit, apparently
												PM_FallToDeath(self);
											}
										}
										else if (trace.entityNum < ENTITYNUM_NONE
											&& pm->ps->weapon != WP_SABER
											&& (!self || !self->client || self->client->NPC_class != CLASS_BOBAFETT
												&& self->client->NPC_class != CLASS_REBORN
												&& self->client->NPC_class != CLASS_ROCKETTROOPER))
										{
											//Jedi don't scream and die if they're heading for a hard impact
											if (trace.entityNum == ENTITYNUM_WORLD)
											{
												//hit architecture, find impact force
												float dmg;

												VectorCopy(pm->ps->velocity, vel);
												time = Distance(trace.endpos, pm->ps->origin) / VectorLength(vel);
												vel[2] -= 0.5 * time * pm->ps->gravity;

												if (trace.plane.normal[2] > 0.5)
												{
													//use falling damage
													int waterlevel, junk;
													PM_SetWaterLevelAtPoint(trace.endpos, &waterlevel, &junk);
													dmg = PM_CrashLandDelta(vel, waterlevel);
													if (dmg >= 30)
													{
														//there is a minimum fall threshhold
														dmg = PM_DamageForDelta(dmg);
													}
													else
													{
														dmg = 0;
													}
												}
												else
												{
													//use impact damage
													//guestimate
													if (self->client && self->client->ps.fd.forceJumpZStart)
													{
														//we were force-jumping
														if (self->r.currentOrigin[2] >= self->client->ps.fd.
															forceJumpZStart)
														{
															//we landed at same height or higher than we landed
															dmg = 0;
														}
														else
														{
															//FIXME: take off some of it, at least?
															dmg = (self->client->ps.fd.forceJumpZStart - self->r.
																currentOrigin[2]) / 3;
														}
													}
													dmg = 10 * VectorLength(pm->ps->velocity);
													if (pm->ps->clientNum < MAX_CLIENTS)
													{
														dmg /= 2;
													}
													dmg *= 0.01875f; //magic number
												}
												float maxDmg = pm->ps->stats[STAT_HEALTH] > 20
													? pm->ps->stats[STAT_HEALTH]
													: 20;
												//a fall that would do less than 20 points of damage should never make us scream to our deaths
												if (self && self->client && self->client->NPC_class == CLASS_HOWLER)
												{
													//howlers can survive long falls, despire their health
													maxDmg *= 5;
												}
												if (dmg >= pm->ps->stats[STAT_HEALTH]) //armor?
												{
													PM_FallToDeath(self);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
#endif
	else if (!pm->ps->inAirAnim)
	{
		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy(pm->ps->origin, point);
		point[2] -= 64;

		pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		if (trace.fraction == 1.0 || pm->ps->pm_type == PM_FLOAT)
		{
			pm->ps->inAirAnim = qtrue;
		}
	}

	if (PM_InRollComplete(pm->ps, pm->ps->legsAnim))
	{
		//Client won't catch an animation restart because it only checks frame against incoming frame, so if you roll when you land after rolling
		//off of something it won't replay the roll anim unless we switch it off in the air. This fixes that.
		PM_SetAnim(SETANIM_BOTH, BOTH_INAIR1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		pm->ps->inAirAnim = qtrue;
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}

#ifdef _GAME
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength,
	qboolean breakSaberLock);
#endif

static qboolean BG_InDFA()
{
	if (pm->ps->saber_move == LS_A_JUMP_T__B_)
	{
		return qtrue;
	}
	if (pm->ps->saber_move == LS_A_JUMP_PALP_)
	{
		return qtrue;
	}

	if (pm->ps->saber_move == LS_LEAP_ATTACK)
	{
		return qtrue;
	}

	if (pm->ps->saber_move == LS_LEAP_ATTACK2)
	{
		return qtrue;
	}

	if (pm->ps->torsoAnim == saber_moveData[16].animToUse)
	{
		return qtrue;
	}

	if (pm->ps->torsoAnim == 1252)
	{
		return qtrue;
	}

	return qfalse;
}

#ifdef _GAME
static qboolean G_InDFA(const gentity_t* ent)
{
	if (!ent || !ent->client) return qfalse;

	if (ent->client->ps.saber_move == LS_A_JUMP_T__B_
		|| ent->client->ps.saber_move == LS_A_JUMP_PALP_
		|| ent->client->ps.saber_move == LS_A_FLIP_STAB
		|| ent->client->ps.saber_move == LS_LEAP_ATTACK
		|| ent->client->ps.saber_move == LS_LEAP_ATTACK2
		|| ent->client->ps.saber_move == LS_A_FLIP_SLASH
		|| ent->client->ps.saber_move == LS_R_BL2TR)
	{
		return qtrue;
	}

	if (ent->client->ps.torsoAnim == saber_moveData[16].animToUse)
	{
		return qtrue;
	}

	if (ent->client->ps.torsoAnim == 1252)
	{
		return qtrue;
	}

	return qfalse;
}

#endif

/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace(void)
{
	vec3_t point;
	trace_t trace;
	float minNormal = MIN_WALK_NORMAL;

	if (pm->ps->clientNum >= MAX_CLIENTS)
	{
		const bgEntity_t* pEnt = pm_entSelf;

		if (pEnt && pEnt->s.NPC_class == CLASS_VEHICLE)
		{
			minNormal = pEnt->m_pVehicle->m_pVehicleInfo->maxSlope;
		}
	}

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;

	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if (trace.allsolid)
	{
		if (!PM_CorrectAllSolid(&trace))
			return;
	}

	if (pm->ps->pm_type == PM_FLOAT)
	{
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// if the trace didn't hit anything, we are in free fall
	if (trace.fraction == 1.0)
	{
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if ((pm->ps->velocity[2] > 0 && pm->ps->pm_flags & PMF_TIME_KNOCKBACK || pm->ps->velocity[2] > 100) &&
		DotProduct(pm->ps->velocity, trace.plane.normal) > 10)
	{
		if (pm->debugLevel)
		{
			Com_Printf("%i:kickoff\n", c_pmove);
		}
		// go into jump animation
		if (PM_FlippingAnim(pm->ps->legsAnim))
		{
			//we're flipping
		}
		else if (PM_InSpecialJump(pm->ps->legsAnim))
		{
			//special jumps
		}
		else if (PM_InKnockDown(pm->ps))
		{
			//in knockdown
		}
		else if (PM_InRoll(pm->ps))
		{
			//in knockdown
		}
		else if (PM_KickingAnim(pm->ps->legsAnim))
		{
			//in kick
		}
		else
		{
			PM_JumpForDir();
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// slopes that are too steep will not be considered onground
	if (trace.plane.normal[2] < minNormal)
	{
		if (pm->debugLevel)
		{
			Com_Printf("%i:steep\n", c_pmove);
		}
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

#ifdef _GAME
	if (pm->ps->pm_type == PM_JETPACK && (g_entities[pm->ps->clientNum].client || pm->cmd.buttons & BUTTON_USE))
	{
		//turn off jetpack if we touch the ground.
		Jetpack_Off(&g_entities[pm->ps->clientNum]);
	}
#endif

	// hitting solid ground will end a waterjump
	if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
	{
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
	{
		// just hit the ground
		if (pm->debugLevel)
		{
			Com_Printf("%i:Land\n", c_pmove);
		}

		PM_CrashLand();

#ifdef _GAME
		if (trace.entityNum != ENTITYNUM_WORLD
			&& Q_stricmp(g_entities[pm->ps->clientNum].NPC_type, "seeker")
			&& g_entities[pm->ps->clientNum].s.NPC_class != CLASS_VEHICLE
			&& !G_InDFA(&g_entities[pm->ps->clientNum]))
		{
			gentity_t* trEnt = &g_entities[trace.entityNum];

			if (trEnt->inuse && trEnt->client && !trEnt->item
				&& trEnt->s.NPC_class != CLASS_SEEKER && trEnt->s.NPC_class != CLASS_VEHICLE && !G_InDFA(trEnt))
			{
				vec3_t pushdir;
				//Player?
				trEnt->client->ps.legsAnim = trEnt->client->ps.torsoAnim = 0;
				g_entities[pm->ps->clientNum].client->ps.legsAnim = g_entities[pm->ps->clientNum].client->ps.torsoAnim
					=
					0;
				VectorSubtract(g_entities[pm->ps->clientNum].client->ps.origin, trEnt->client->ps.origin, pushdir);
				G_Knockdown(trEnt, &g_entities[pm->ps->clientNum], pushdir, 0, qfalse);
				G_Knockdown(&g_entities[pm->ps->clientNum], trEnt, pushdir, 5, qfalse);
				g_throw(&g_entities[pm->ps->clientNum], pushdir, 5);
				g_entities[pm->ps->clientNum].client->ps.velocity[2] = 10;
			}
		}
		if (pm->ps->clientNum < MAX_CLIENTS &&
			!pm->ps->m_iVehicleNum &&
			trace.entityNum < ENTITYNUM_WORLD &&
			trace.entityNum >= MAX_CLIENTS &&
			!pm->ps->zoomMode &&
			pm_entSelf)
		{
			//check if we landed on a vehicle
			const gentity_t* trEnt = &g_entities[trace.entityNum];
			if (trEnt->inuse && trEnt->client &&
				trEnt->s.eType == ET_NPC &&
				trEnt->s.NPC_class == CLASS_VEHICLE &&
				!trEnt->client->ps.m_iVehicleNum &&
				trEnt->m_pVehicle &&
				trEnt->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER)
			{
				//it's a vehicle alright, let's board it.. if it's not an atst or ship
				if (!PM_SaberInSpecial(pm->ps->saber_move) &&
					pm->ps->forceHandExtend == HANDEXTEND_NONE &&
					pm->ps->weaponTime <= 0)
				{
					const gentity_t* servEnt = (gentity_t*)pm_entSelf;
					if (level.gametype < GT_TEAM ||
						!trEnt->alliedTeam ||
						trEnt->alliedTeam == servEnt->client->sess.sessionTeam)
					{
						//not belonging to a team, or client is on same team
						trEnt->m_pVehicle->m_pVehicleInfo->Board(trEnt->m_pVehicle, pm_entSelf);
					}
				}
			}
		}
#endif

		// don't do landing time if we were just going down a slope
		if (pml.previous_velocity[2] < -200)
		{
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;
	pm->ps->lastOnGround = pm->cmd.serverTime;

	if (!pm->ps->clientNum)
	{
		//if a player, clear the jumping "flag" so can't double-jump
		pm->ps->fd.forceJumpCharge = 0;
	}

	PM_AddTouchEnt(trace.entityNum);
}

/*
=============
PM_SetWaterLevel
=============
*/
static void PM_SetWaterLevel(void)
{
	vec3_t point;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;
	int cont = pm->pointcontents(point, pm->ps->clientNum);

	if (cont & (MASK_WATER | CONTENTS_LADDER))
	{
		const int sample2 = pm->ps->viewheight - MINS_Z;
		const int sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		cont = pm->pointcontents(point, pm->ps->clientNum);
		if (cont & MASK_WATER)
		{
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			cont = pm->pointcontents(point, pm->ps->clientNum);
			if (cont & MASK_WATER)
			{
				pm->waterlevel = 3;
			}
		}
	}
}

//void PM_SetWaterHeight(void)
//{
//	pm->ps->waterHeightLevel = WHL_NONE;
//	if (pm->waterlevel < 1)
//	{
//		pm->ps->waterheight = pm->ps->origin[2] + DEFAULT_MINS_2 - 4;
//		return;
//	}
//	trace_t trace;
//	vec3_t	top, bottom;
//
//	VectorCopy(pm->ps->origin, top);
//	VectorCopy(pm->ps->origin, bottom);
//	top[2] += pm->gent->client->standheight;
//	bottom[2] += DEFAULT_MINS_2;
//
//	gi.trace(&trace, top, pm->minimum_mins, pm->maximum_maxs, bottom, pm->ps->clientNum, (CONTENTS_WATER | CONTENTS_SLIME), G2_NOCOLLIDE, 0);
//
//	if (trace.startsolid)
//	{//under water
//		pm->ps->waterheight = top[2] + 4;
//	}
//	else if (trace.fraction < 1.0f)
//	{//partially in and partially out of water
//		pm->ps->waterheight = trace.endpos[2] + pm->minimum_mins[2];
//	}
//	else if (trace.contents & (CONTENTS_WATER | CONTENTS_SLIME))
//	{//water is above me
//		pm->ps->waterheight = top[2] + 4;
//	}
//	else
//	{//water is below me
//		pm->ps->waterheight = bottom[2] - 4;
//	}
//	float distFromEyes = (pm->ps->origin[2] + pm->gent->client->standheight) - pm->ps->waterheight;
//
//	if (distFromEyes < 0)
//	{
//		pm->ps->waterHeightLevel = WHL_UNDER;
//	}
//	else if (distFromEyes < 6)
//	{
//		pm->ps->waterHeightLevel = WHL_HEAD;
//	}
//	else if (distFromEyes < 18)
//	{
//		pm->ps->waterHeightLevel = WHL_SHOULDERS;
//	}
//	else if (distFromEyes < pm->gent->client->standheight - 8)
//	{//at least 8 above origin
//		pm->ps->waterHeightLevel = WHL_TORSO;
//	}
//	else
//	{
//		float distFromOrg = pm->ps->origin[2] - pm->ps->waterheight;
//		if (distFromOrg < 6)
//		{
//			pm->ps->waterHeightLevel = WHL_WAIST;
//		}
//		else if (distFromOrg < 16)
//		{
//			pm->ps->waterHeightLevel = WHL_KNEES;
//		}
//		else if (distFromOrg > fabs(pm->minimum_mins[2]))
//		{
//			pm->ps->waterHeightLevel = WHL_NONE;
//		}
//		else
//		{
//			pm->ps->waterHeightLevel = WHL_ANKLES;
//		}
//	}
//}

static qboolean PM_CheckDualForwardJumpDuck(void)
{
	qboolean resized = qfalse;
	if (pm->ps->legsAnim == BOTH_JUMPATTACK6)
	{
		//dynamically reduce bounding box to let character sail over heads of enemies
		if (pm->ps->legsTimer >= 1450
			&& PM_AnimLength((animNumber_t)BOTH_JUMPATTACK6) - pm->ps->legsTimer >= 400
			|| pm->ps->legsTimer >= 400
			&& PM_AnimLength((animNumber_t)BOTH_JUMPATTACK6) - pm->ps->legsTimer >= 1100)
		{
			//in a part of the anim that we're pretty much sideways in, raise up the minimum_mins
			pm->mins[2] = 0;
			pm->ps->pm_flags |= PMF_FIX_MINS;
			resized = qtrue;
		}
	}
	return resized;
}

static void PM_CheckFixMins(void)
{
	if (pm->ps->legsAnim == BOTH_JUMPATTACK6)
	{
		//dynamically reduce bounding box to let character sail over heads of enemies
		if (pm->ps->legsTimer >= 1450
			&& PM_AnimLength((animNumber_t)BOTH_JUMPATTACK6) - pm->ps->legsTimer >= 400
			|| pm->ps->legsTimer >= 400
			&& PM_AnimLength((animNumber_t)BOTH_JUMPATTACK6) - pm->ps->legsTimer >= 1100)
		{
			//in a part of the anim that we're pretty much sideways in, raise up the minimum_mins
			pm->mins[2] = 0;
			pm->ps->pm_flags |= PMF_FIX_MINS;
		}
	}
	else
	{
		if (pm->ps->pm_flags & PMF_FIX_MINS)
		{
			//drop the minimum_mins back down
			//do a trace to make sure it's okay
			trace_t trace;
			vec3_t end, curMins, curMaxs;

			VectorSet(end, pm->ps->origin[0], pm->ps->origin[1], pm->ps->origin[2] + MINS_Z);
			VectorSet(curMins, pm->mins[0], pm->mins[1], 0);
			VectorSet(curMaxs, pm->maxs[0], pm->maxs[1], pm->ps->standheight);

			pm->trace(&trace, pm->ps->origin, curMins, curMaxs, end, pm->ps->clientNum, pm->tracemask);
			if (!trace.allsolid && !trace.startsolid)
			{
				//should never start in solid
				if (trace.fraction >= 1.0f)
				{
					//all clear
					//drop the bottom of my bbox back down
					pm->mins[2] = MINS_Z;
					pm->ps->pm_flags &= ~PMF_FIX_MINS;
				}
				else
				{
					//move me up so the bottom of my bbox will be where the trace ended, at least
					//need to trace up, too
					const float updist = (1.0f - trace.fraction) * -MINS_Z;
					end[2] = pm->ps->origin[2] + updist;
					pm->trace(&trace, pm->ps->origin, curMins, curMaxs, end, pm->ps->clientNum, pm->tracemask);
					if (!trace.allsolid && !trace.startsolid)
					{
						//should never start in solid
						if (trace.fraction >= 1.0f)
						{
							//all clear
							//move me up
							pm->ps->origin[2] += updist;
							//drop the bottom of my bbox back down
							pm->mins[2] = MINS_Z;
							pm->ps->pm_flags &= ~PMF_FIX_MINS;
						}
						else
						{
							//crap, no room to expand, so just crouch us
							if (pm->ps->legsAnim != BOTH_JUMPATTACK6 && pm->ps->legsAnim != BOTH_GRIEVOUS_LUNGE
								|| pm->ps->legsTimer <= 200)
							{
								//at the end of the anim, and we can't leave ourselves like this
								//so drop the maximum_maxs, put the minimum_mins back and move us up
								pm->maxs[2] += MINS_Z;
								pm->ps->origin[2] -= MINS_Z;
								pm->mins[2] = MINS_Z;
								//this way we'll be in a crouch when we're done
								if (pm->ps->legsAnim == BOTH_JUMPATTACK6 || pm->ps->legsAnim == BOTH_GRIEVOUS_LUNGE)
								{
									pm->ps->legsTimer = pm->ps->torsoTimer = 0;
								}
								pm->ps->pm_flags |= PMF_DUCKED;
								pm->ps->pm_flags &= ~PMF_FIX_MINS;
							}
						}
					} //crap, stuck
				}
			} //crap, stuck!
		}
	}
}

static qboolean PM_CanStand(void)
{
	qboolean canStand = qtrue;
	trace_t trace;

	const vec3_t lineMins = { -5.0f, -5.0f, -2.5f };
	const vec3_t lineMaxs = { 5.0f, 5.0f, 0.0f };

	for (float x = pm->mins[0] + 5.0f; canStand && x <= pm->maxs[0] - 5.0f; x += 10.0f)
	{
		for (float y = pm->mins[1] + 5.0f; y <= pm->maxs[1] - 5.0f; y += 10.0f)
		{
			vec3_t start, end; //
			VectorSet(start, x, y, pm->maxs[2]);
			VectorSet(end, x, y, pm->ps->standheight);

			VectorAdd(start, pm->ps->origin, start);
			VectorAdd(end, pm->ps->origin, end);

			pm->trace(&trace, start, lineMins, lineMaxs, end, pm->ps->clientNum, pm->tracemask);
			if (trace.allsolid || trace.fraction < 1.0f)
			{
				canStand = qfalse;
				break;
			}
		}
	}

	return canStand;
}

/*
==============
PM_CheckDuck

Sets minimum_mins, maximum_maxs, and pm->ps->viewheight
==============
*/
qboolean PM_GettingUpFromKnockDown(float standheight, float crouchheight);

static void PM_CheckDuck(void)
{
	if (pm->ps->m_iVehicleNum > 0 && pm->ps->m_iVehicleNum < ENTITYNUM_NONE)
	{
		//riding a vehicle or are a vehicle
		pm->ps->pm_flags &= ~PMF_DUCKED;
		pm->ps->pm_flags &= ~PMF_ROLLING;

		if (pm->ps->clientNum >= MAX_CLIENTS)
		{
			return;
		}

		if (pm_entVeh && pm_entVeh->m_pVehicle &&
			(pm_entVeh->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
				pm_entVeh->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL))
		{
			trace_t solidTr;

			pm->mins[0] = -16;
			pm->mins[1] = -16;
			pm->mins[2] = MINS_Z;

			pm->maxs[0] = 16;
			pm->maxs[1] = 16;
			pm->maxs[2] = pm->ps->standheight;
			pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

			pm->trace(&solidTr, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->m_iVehicleNum,
				pm->tracemask);
			if (solidTr.startsolid || solidTr.allsolid || solidTr.fraction != 1.0f)
			{
				//whoops, can't fit here. Down to 0!
				VectorClear(pm->mins);
				VectorClear(pm->maxs);
#ifdef _GAME
				{
					const gentity_t* me = &g_entities[pm->ps->clientNum];
					if (me->inuse && me->client)
					{
						//yeah, this is a really terrible hack.
						me->client->solidHack = level.time + 200;
					}
				}
#endif
			}
		}
	}
	else
	{
		if (pm->ps->clientNum < MAX_CLIENTS)
		{
			if (pm->ps->pm_flags & PMF_DUCKED)
			{
				pm->maxs[2] = pm->ps->crouchheight;
				pm->maxs[1] = 8;
				pm->maxs[0] = 8;
				pm->mins[1] = -8;
				pm->mins[0] = -8;
			}
			else if (!(pm->ps->pm_flags & PMF_DUCKED))
			{
				pm->maxs[2] = pm->ps->standheight - 8;
				pm->maxs[1] = 8;
				pm->maxs[0] = 8;
				pm->mins[1] = -8;
				pm->mins[0] = -8;
			}
		}

		if (PM_CheckDualForwardJumpDuck())
		{
			//special anim resizing us
		}
		else
		{
			PM_CheckFixMins();

			if (!pm->mins[2])
			{
				pm->mins[2] = MINS_Z;
			}
		}

		if (pm->ps->pm_type == PM_DEAD && pm->ps->clientNum < MAX_CLIENTS)
		{
			pm->maxs[2] = -8;
			pm->ps->viewheight = DEAD_VIEWHEIGHT;
			return;
		}

		if (BG_InRoll(pm->ps, pm->ps->legsAnim) && !PM_KickingAnim(pm->ps->legsAnim))
		{
			pm->maxs[2] = pm->ps->crouchheight; //CROUCH_MAXS_2;
			pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
			pm->ps->pm_flags &= ~PMF_DUCKED;
			pm->ps->pm_flags |= PMF_ROLLING;
			return;
		}
		if (pm->ps->pm_flags & PMF_ROLLING)
		{
			if (PM_CanStand())
			{
				pm->maxs[2] = pm->ps->standheight;
				pm->ps->userInt3 &= ~(1 << FLAG_DODGEROLL);
				pm->ps->pm_flags &= ~PMF_ROLLING;
			}
		}
		else if (PM_GettingUpFromKnockDown(pm->ps->standheight, pm->ps->crouchheight))
		{
			//we're attempting to get up from a knockdown.
			pm->ps->viewheight = pm->ps->crouchheight + STANDARD_VIEWHEIGHT_OFFSET;
			return;
		}
		else if (PM_InKnockDown(pm->ps))
		{
			//forced crouch
			pm->maxs[2] = pm->ps->crouchheight;
			pm->ps->viewheight = pm->ps->crouchheight + STANDARD_VIEWHEIGHT_OFFSET;
			pm->ps->pm_flags |= PMF_DUCKED;
			return;
		}
		else if (pm->cmd.upmove < 0 ||
			pm->ps->forceHandExtend == HANDEXTEND_KNOCKDOWN ||
			pm->ps->forceHandExtend == HANDEXTEND_PRETHROWN ||
			pm->ps->forceHandExtend == HANDEXTEND_POSTTHROWN)
		{
			// duck
			pm->ps->pm_flags |= PMF_DUCKED;
		}
		else
		{
			// stand up if possible
			if (pm->ps->pm_flags & PMF_DUCKED)
			{
				if (PM_CanStand())
				{
					pm->maxs[2] = pm->ps->standheight;
					pm->ps->pm_flags &= ~PMF_DUCKED;
				}
			}
		}
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		pm->maxs[2] = pm->ps->crouchheight; //CROUCH_MAXS_2;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
	}
	else if (pm->ps->pm_flags & PMF_ROLLING)
	{
		pm->maxs[2] = pm->ps->crouchheight; //CROUCH_MAXS_2;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}
	else
	{
		pm->maxs[2] = pm->ps->standheight; //DEFAULT_MAXS_2;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}
}

//===================================================================

/*
==============
PM_Use

Generates a use event
==============
*/
#define USE_DELAY 2000

static void PM_Use(void)
{
	if (pm->ps->useTime > 0)
		pm->ps->useTime -= 100; //pm->cmd.msec;

	if (pm->ps->useTime > 0)
	{
		return;
	}

	if (!(pm->cmd.buttons & BUTTON_USE))
	{
		pm->useEvent = 0;
		pm->ps->useTime = 0;
		return;
	}

	pm->useEvent = EV_USE;
	pm->ps->useTime = USE_DELAY;
}

qboolean PM_SaberWalkAnim(const int anim)
{
	switch (anim)
	{
	case BOTH_WALK1:
	case BOTH_WALK2:
	case BOTH_WALK2B: //# Normal walk with saber
	case BOTH_WALK_STAFF: //# Normal walk with staff
	case BOTH_WALK_DUAL: //# Normal walk with staff
	case BOTH_WALKBACK1: //# Walk1 backwards
	case BOTH_WALKBACK2: //# Walk2 backwards
	case BOTH_WALKBACK_STAFF: //# Walk backwards with staff
	case BOTH_WALKBACK_DUAL: //# Walk backwards with dual
	case BOTH_VADERWALK1:
	case BOTH_VADERWALK2:
	case BOTH_MENUIDLE1:
	case BOTH_PARRY_WALK:
	case BOTH_PARRY_WALK_DUAL:
	case BOTH_PARRY_WALK_STAFF:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean PM_WindAnim(const int anim)
{
	switch (anim)
	{
	case BOTH_WIND:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean PM_WalkingAnim(const int anim)
{
	switch (anim)
	{
	case BOTH_WALK1: //# Normal walk
	case BOTH_WALK1TALKCOMM1:
	case BOTH_WALK2: //# Normal walk with saber
	case BOTH_WALK2B: //# Normal walk with saber
	case BOTH_WALK3:
	case BOTH_WALK4:
	case BOTH_WALK5: //# Tavion taunting Kyle (cin 22)
	case BOTH_WALK6: //# Slow walk for Luke (cin 12)
	case BOTH_WALK7: //# Fast walk
	case BOTH_WALK8: //# pistolwalk
	case BOTH_WALK9: //# riflewalk
	case BOTH_WALK10: //# grenadewalk
	case BOTH_WALK_STAFF: //# Normal walk with staff
	case BOTH_WALK_DUAL: //# Normal walk with staff
	case BOTH_WALKBACK1: //# Walk1 backwards
	case BOTH_WALKBACK2: //# Walk2 backwards
	case BOTH_WALKBACK_STAFF: //# Walk backwards with staff
	case BOTH_WALKBACK_DUAL: //# Walk backwards with dual
	case SBD_WALKBACK_NORMAL:
	case SBD_WALKBACK_WEAPON:
	case SBD_WALK_WEAPON:
	case SBD_WALK_NORMAL:
	case BOTH_VADERWALK1:
	case BOTH_VADERWALK2:
	case BOTH_MENUIDLE1:
	case BOTH_PARRY_WALK:
	case BOTH_PARRY_WALK_DUAL:
	case BOTH_PARRY_WALK_STAFF:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean PM_RunningAnim(const int anim)
{
	switch (anim)
	{
	case BOTH_RUN1:
	case BOTH_RUN2:
	case BOTH_RUN3:
	case BOTH_RUN3_MP:
	case BOTH_RUN4:
	case BOTH_RUN5:
	case BOTH_RUN6:
	case BOTH_RUN7:
	case BOTH_RUN8:
	case BOTH_RUN9:
	case BOTH_RUN10:
	case BOTH_SPRINT:
	case BOTH_SPRINT_SABER:
	case BOTH_SPRINT_MP:
	case BOTH_SPRINT_SABER_MP:
	case SBD_RUNBACK_NORMAL:
	case SBD_RUNING_WEAPON:
	case SBD_RUNBACK_WEAPON:
	case BOTH_RUN_STAFF:
	case BOTH_RUN_DUAL:
	case BOTH_RUNBACK1:
	case BOTH_RUNBACK2:
	case BOTH_RUNBACK_STAFF:
	case BOTH_RUNBACK_DUAL:
	case BOTH_RUN1START: //# Start into full run1
	case BOTH_RUN1STOP: //# Stop from full run1
	case BOTH_RUNSTRAFE_LEFT1: //# Sidestep left: should loop
	case BOTH_RUNSTRAFE_RIGHT1: //# Sidestep right: should loop
	case BOTH_RUNINJURED1:
	case BOTH_VADERRUN1:
	case BOTH_VADERRUN2:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean PM_SwimmingAnim(const int anim)
{
	switch (anim)
	{
	case BOTH_SWIM_IDLE1: //# Swimming Idle 1
	case BOTH_SWIMFORWARD: //# Swim forward loop
	case BOTH_SWIMBACKWARD: //# Swim backward loop
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean PM_RollingAnim(const int anim)
{
	switch (anim)
	{
	case BOTH_ROLL_F: //# Roll forward
	case BOTH_ROLL_F1:
	case BOTH_ROLL_F2:
	case BOTH_ROLL_B: //# Roll backward
	case BOTH_ROLL_L: //# Roll left
	case BOTH_ROLL_R: //# Roll right
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean PM_WalkingOrRunningAnim(const int anim)
{
	//mainly for checking if we can guard well
	if (PM_WalkingAnim(anim)
		|| PM_RunningAnim(anim)
		|| PM_RollingAnim(anim)
		|| PM_SaberWalkAnim(anim))
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_StandingidleAnim(const int anim)
{
	//NOTE: does not check idles or special (cinematic) stands
	switch (anim)
	{
	case BOTH_STANDMELEE:
	case BOTH_SABERFAST_STANCE: //single-saber, fast style
	case BOTH_SABERSLOW_STANCE: //single-saber, strong style
	case BOTH_SABERSTAFF_STANCE: //saber staff style
	case BOTH_SABERSTAFF_STANCE_IDLE: //saber staff style
	case BOTH_SABERDUAL_STANCE: //dual saber style
	case BOTH_SABERDUAL_STANCE_GRIEVOUS: //dual saber style
	case BOTH_SABERDUAL_STANCE_IDLE: //dual saber style
	case BOTH_SABERTAVION_STANCE: //tavion saberstyle
	case BOTH_SABERDESANN_STANCE: //desann saber style
	case BOTH_SABERSTANCE_STANCE: //similar to stand2
	case BOTH_SABERYODA_STANCE: //YODA saber style
	case BOTH_SABERBACKHAND_STANCE: //BACKHAND saber style
	case BOTH_SABERDUAL_STANCE_ALT: //alt dual
	case BOTH_SABERSTAFF_STANCE_ALT: //alt staff
	case BOTH_SABERSTANCE_STANCE_ALT: //alt idle
	case BOTH_SABEROBI_STANCE: //obiwan
	case BOTH_SABEREADY_STANCE: //ready
	case BOTH_SABER_REY_STANCE: //rey
	case BOTH_STAND1:
	case BOTH_STAND1IDLE1:
	case BOTH_STAND2:
	case BOTH_STAND2IDLE1:
	case BOTH_STAND2IDLE2:
	case BOTH_STAND3:
	case BOTH_STAND3IDLE1:
	case BOTH_STAND4:
	case BOTH_STAND5:
	case BOTH_STAND5IDLE1:
	case BOTH_ATTACK3:
	case BOTH_ATTACK5:
	case BOTH_ATTACK6:
	case BOTH_MENUIDLE1:
	case TORSO_WEAPONIDLE2:
	case TORSO_WEAPONIDLE3:
	case TORSO_WEAPONIDLE4:
		return qtrue;
	default:;
	}
	return qfalse;
}

static void PM_AnglesForSlope(const float yaw, const vec3_t slope, vec3_t angles)
{
	vec3_t nvf, ovf, ovr, new_angles;

	VectorSet(angles, 0, yaw, 0);
	AngleVectors(angles, ovf, ovr, NULL);

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

	angles[YAW] = 0;
	angles[PITCH] = dot * pitch;
	angles[ROLL] = (1 - Q_fabs(dot)) * pitch * mod;
}

static void PM_FootSlopeTrace(float* p_diff, float* p_interval)
{
	vec3_t footLOrg, footROrg, footLBot, footRBot;
	vec3_t footLPoint, footRPoint;
	vec3_t footMins, footMaxs;
	vec3_t footLSlope, footRSlope;

	trace_t trace;

	mdxaBone_t boltMatrix;
	vec3_t G2Angles;

	VectorSet(G2Angles, 0, pm->ps->viewangles[YAW], 0);

	const float interval = 4; //?

	trap->G2API_GetBoltMatrix(pm->ghoul2, 0, pm->g2Bolts_LFoot, &boltMatrix, G2Angles, pm->ps->origin,
		pm->cmd.serverTime, NULL, pm->modelScale);
	footLPoint[0] = boltMatrix.matrix[0][3];
	footLPoint[1] = boltMatrix.matrix[1][3];
	footLPoint[2] = boltMatrix.matrix[2][3];

	trap->G2API_GetBoltMatrix(pm->ghoul2, 0, pm->g2Bolts_RFoot, &boltMatrix, G2Angles, pm->ps->origin,
		pm->cmd.serverTime, NULL, pm->modelScale);
	footRPoint[0] = boltMatrix.matrix[0][3];
	footRPoint[1] = boltMatrix.matrix[1][3];
	footRPoint[2] = boltMatrix.matrix[2][3];

	//get these on the cgame and store it, save ourselves a ghoul2 construct skel call
	VectorCopy(footLPoint, footLOrg);
	VectorCopy(footRPoint, footROrg);

	//step 2: adjust foot tag z height to bottom of bbox+1
	footLOrg[2] = pm->ps->origin[2] + pm->mins[2] + 1;
	footROrg[2] = pm->ps->origin[2] + pm->mins[2] + 1;
	VectorSet(footLBot, footLOrg[0], footLOrg[1], footLOrg[2] - interval * 10);
	VectorSet(footRBot, footROrg[0], footROrg[1], footROrg[2] - interval * 10);

	//step 3: trace down from each, find difference
	VectorSet(footMins, -3, -3, 0);
	VectorSet(footMaxs, 3, 3, 1);

	pm->trace(&trace, footLOrg, footMins, footMaxs, footLBot, pm->ps->clientNum, pm->tracemask);
	VectorCopy(trace.endpos, footLBot);
	VectorCopy(trace.plane.normal, footLSlope);

	pm->trace(&trace, footROrg, footMins, footMaxs, footRBot, pm->ps->clientNum, pm->tracemask);
	VectorCopy(trace.endpos, footRBot);
	VectorCopy(trace.plane.normal, footRSlope);

	const float diff = footLBot[2] - footRBot[2];

	if (p_diff != NULL)
	{
		*p_diff = diff;
	}
	if (p_interval != NULL)
	{
		*p_interval = interval;
	}
}

qboolean PM_InSlopeAnim(const int anim)
{
	switch (anim)
	{
	case LEGS_LEFTUP1: //# On a slope with left foot 4 higher than right
	case LEGS_LEFTUP2: //# On a slope with left foot 8 higher than right
	case LEGS_LEFTUP3: //# On a slope with left foot 12 higher than right
	case LEGS_LEFTUP4: //# On a slope with left foot 16 higher than right
	case LEGS_LEFTUP5: //# On a slope with left foot 20 higher than right
	case LEGS_RIGHTUP1: //# On a slope with RIGHT foot 4 higher than left
	case LEGS_RIGHTUP2: //# On a slope with RIGHT foot 8 higher than left
	case LEGS_RIGHTUP3: //# On a slope with RIGHT foot 12 higher than left
	case LEGS_RIGHTUP4: //# On a slope with RIGHT foot 16 higher than left
	case LEGS_RIGHTUP5: //# On a slope with RIGHT foot 20 higher than left
	case LEGS_S1_LUP1:
	case LEGS_S1_LUP2:
	case LEGS_S1_LUP3:
	case LEGS_S1_LUP4:
	case LEGS_S1_LUP5:
	case LEGS_S1_RUP1:
	case LEGS_S1_RUP2:
	case LEGS_S1_RUP3:
	case LEGS_S1_RUP4:
	case LEGS_S1_RUP5:
	case LEGS_S3_LUP1:
	case LEGS_S3_LUP2:
	case LEGS_S3_LUP3:
	case LEGS_S3_LUP4:
	case LEGS_S3_LUP5:
	case LEGS_S3_RUP1:
	case LEGS_S3_RUP2:
	case LEGS_S3_RUP3:
	case LEGS_S3_RUP4:
	case LEGS_S3_RUP5:
	case LEGS_S4_LUP1:
	case LEGS_S4_LUP2:
	case LEGS_S4_LUP3:
	case LEGS_S4_LUP4:
	case LEGS_S4_LUP5:
	case LEGS_S4_RUP1:
	case LEGS_S4_RUP2:
	case LEGS_S4_RUP3:
	case LEGS_S4_RUP4:
	case LEGS_S4_RUP5:
	case LEGS_S5_LUP1:
	case LEGS_S5_LUP2:
	case LEGS_S5_LUP3:
	case LEGS_S5_LUP4:
	case LEGS_S5_LUP5:
	case LEGS_S5_RUP1:
	case LEGS_S5_RUP2:
	case LEGS_S5_RUP3:
	case LEGS_S5_RUP4:
	case LEGS_S5_RUP5:
	case LEGS_S6_LUP1:
	case LEGS_S6_LUP2:
	case LEGS_S6_LUP3:
	case LEGS_S6_LUP4:
	case LEGS_S6_LUP5:
	case LEGS_S6_RUP1:
	case LEGS_S6_RUP2:
	case LEGS_S6_RUP3:
	case LEGS_S6_RUP4:
	case LEGS_S6_RUP5:
	case LEGS_S7_LUP1:
	case LEGS_S7_LUP2:
	case LEGS_S7_LUP3:
	case LEGS_S7_LUP4:
	case LEGS_S7_LUP5:
	case LEGS_S7_RUP1:
	case LEGS_S7_RUP2:
	case LEGS_S7_RUP3:
	case LEGS_S7_RUP4:
	case LEGS_S7_RUP5:
		return qtrue;
	default:;
	}
	return qfalse;
}

#define	SLOPE_RECALC_INT 100

static qboolean PM_AdjustStandAnimForSlope(void)
{
	float diff;
	float interval;
	int destAnim;
	const bgEntity_t* pEnt = pm_entSelf;
#define SLOPERECALCVAR pm->ps->slopeRecalcTime //this is purely convenience

	if (!pm->ghoul2)
	{
		//probably just changed models and not quite in sync yet
		return qfalse;
	}

	if (pm->g2Bolts_LFoot == -1 || pm->g2Bolts_RFoot == -1)
	{
		//need these bolts!
		return qfalse;
	}

	//step 1: find the 2 foot tags
	PM_FootSlopeTrace(&diff, &interval);

	//step 4: based on difference, choose one of the left/right slope-match intervals
	if (diff >= interval * 5)
	{
		destAnim = LEGS_LEFTUP5;
	}
	else if (diff >= interval * 4)
	{
		destAnim = LEGS_LEFTUP4;
	}
	else if (diff >= interval * 3)
	{
		destAnim = LEGS_LEFTUP3;
	}
	else if (diff >= interval * 2)
	{
		destAnim = LEGS_LEFTUP2;
	}
	else if (diff >= interval)
	{
		destAnim = LEGS_LEFTUP1;
	}
	else if (diff <= interval * -5)
	{
		destAnim = LEGS_RIGHTUP5;
	}
	else if (diff <= interval * -4)
	{
		destAnim = LEGS_RIGHTUP4;
	}
	else if (diff <= interval * -3)
	{
		destAnim = LEGS_RIGHTUP3;
	}
	else if (diff <= interval * -2)
	{
		destAnim = LEGS_RIGHTUP2;
	}
	else if (diff <= interval * -1)
	{
		destAnim = LEGS_RIGHTUP1;
	}
	else
	{
		return qfalse;
	}

	int legsAnim = pm->ps->legsAnim;
	//adjust for current legs anim

	if (pEnt->s.NPC_class != CLASS_ATST)
	{
		//adjust for current legs anim
		switch (legsAnim)
		{
		case BOTH_STAND1:
		case LEGS_S1_LUP1:
		case LEGS_S1_LUP2:
		case LEGS_S1_LUP3:
		case LEGS_S1_LUP4:
		case LEGS_S1_LUP5:
		case LEGS_S1_RUP1:
		case LEGS_S1_RUP2:
		case LEGS_S1_RUP3:
		case LEGS_S1_RUP4:
		case LEGS_S1_RUP5:
			destAnim = LEGS_S1_LUP1 + (destAnim - LEGS_LEFTUP1);
			break;
		case BOTH_STAND2:
		case BOTH_SABERFAST_STANCE:
		case BOTH_SABERSLOW_STANCE:
		case BOTH_SABERTAVION_STANCE:
		case BOTH_SABERDESANN_STANCE:
		case BOTH_SABERSTANCE_STANCE:
		case BOTH_SABERYODA_STANCE: //YODA saber style
		case BOTH_SABERBACKHAND_STANCE: //BACKHAND saber style
		case BOTH_SABERSTANCE_STANCE_ALT: //alt idle
		case BOTH_SABEROBI_STANCE: //obiwan
		case BOTH_SABEREADY_STANCE: //ready
		case BOTH_SABER_REY_STANCE: //rey
		case BOTH_CROUCH1IDLE:
		case BOTH_CROUCH1:
		case LEGS_LEFTUP1: //# On a slope with left foot 4 higher than right
		case LEGS_LEFTUP2: //# On a slope with left foot 8 higher than right
		case LEGS_LEFTUP3: //# On a slope with left foot 12 higher than right
		case LEGS_LEFTUP4: //# On a slope with left foot 16 higher than right
		case LEGS_LEFTUP5: //# On a slope with left foot 20 higher than right
		case LEGS_RIGHTUP1: //# On a slope with RIGHT foot 4 higher than left
		case LEGS_RIGHTUP2: //# On a slope with RIGHT foot 8 higher than left
		case LEGS_RIGHTUP3: //# On a slope with RIGHT foot 12 higher than left
		case LEGS_RIGHTUP4: //# On a slope with RIGHT foot 16 higher than left
		case LEGS_RIGHTUP5: //# On a slope with RIGHT foot 20 higher than left
			//fine
			break;
		case BOTH_STAND3:
		case LEGS_S3_LUP1:
		case LEGS_S3_LUP2:
		case LEGS_S3_LUP3:
		case LEGS_S3_LUP4:
		case LEGS_S3_LUP5:
		case LEGS_S3_RUP1:
		case LEGS_S3_RUP2:
		case LEGS_S3_RUP3:
		case LEGS_S3_RUP4:
		case LEGS_S3_RUP5:
			destAnim = LEGS_S3_LUP1 + (destAnim - LEGS_LEFTUP1);
			break;
		case BOTH_STAND4:
		case LEGS_S4_LUP1:
		case LEGS_S4_LUP2:
		case LEGS_S4_LUP3:
		case LEGS_S4_LUP4:
		case LEGS_S4_LUP5:
		case LEGS_S4_RUP1:
		case LEGS_S4_RUP2:
		case LEGS_S4_RUP3:
		case LEGS_S4_RUP4:
		case LEGS_S4_RUP5:
			destAnim = LEGS_S4_LUP1 + (destAnim - LEGS_LEFTUP1);
			break;
		case BOTH_STAND5:
		case LEGS_S5_LUP1:
		case LEGS_S5_LUP2:
		case LEGS_S5_LUP3:
		case LEGS_S5_LUP4:
		case LEGS_S5_LUP5:
		case LEGS_S5_RUP1:
		case LEGS_S5_RUP2:
		case LEGS_S5_RUP3:
		case LEGS_S5_RUP4:
		case LEGS_S5_RUP5:
			destAnim = LEGS_S5_LUP1 + (destAnim - LEGS_LEFTUP1);
			break;
		case BOTH_SABERDUAL_STANCE_ALT: //alt dual
		case BOTH_SABERDUAL_STANCE:
		case BOTH_SABERDUAL_STANCE_GRIEVOUS: //dual saber style
		case BOTH_SABERDUAL_STANCE_IDLE:
		case LEGS_S6_LUP1:
		case LEGS_S6_LUP2:
		case LEGS_S6_LUP3:
		case LEGS_S6_LUP4:
		case LEGS_S6_LUP5:
		case LEGS_S6_RUP1:
		case LEGS_S6_RUP2:
		case LEGS_S6_RUP3:
		case LEGS_S6_RUP4:
		case LEGS_S6_RUP5:
			destAnim = LEGS_S6_LUP1 + (destAnim - LEGS_LEFTUP1);
			break;
		case BOTH_SABERSTAFF_STANCE_ALT: //alt staff
		case BOTH_SABERSTAFF_STANCE:
		case BOTH_SABERSTAFF_STANCE_IDLE:
		case LEGS_S7_LUP1:
		case LEGS_S7_LUP2:
		case LEGS_S7_LUP3:
		case LEGS_S7_LUP4:
		case LEGS_S7_LUP5:
		case LEGS_S7_RUP1:
		case LEGS_S7_RUP2:
		case LEGS_S7_RUP3:
		case LEGS_S7_RUP4:
		case LEGS_S7_RUP5:
			destAnim = LEGS_S7_LUP1 + (destAnim - LEGS_LEFTUP1);
			break;
		case BOTH_STAND6:
		default:
			return qfalse;
		}
	}

	//step 5: based on the chosen interval and the current legsAnim, pick the correct anim
	//step 6: increment/decrement to the dest anim, not instant
	if (legsAnim >= LEGS_LEFTUP1 && legsAnim <= LEGS_LEFTUP5
		|| legsAnim >= LEGS_S1_LUP1 && legsAnim <= LEGS_S1_LUP5
		|| legsAnim >= LEGS_S3_LUP1 && legsAnim <= LEGS_S3_LUP5
		|| legsAnim >= LEGS_S4_LUP1 && legsAnim <= LEGS_S4_LUP5
		|| legsAnim >= LEGS_S5_LUP1 && legsAnim <= LEGS_S5_LUP5
		|| legsAnim >= LEGS_S6_LUP1 && legsAnim <= LEGS_S6_LUP5
		|| legsAnim >= LEGS_S7_LUP1 && legsAnim <= LEGS_S7_LUP5)
	{
		//already in left-side up
		if (destAnim > legsAnim && SLOPERECALCVAR < pm->cmd.serverTime)
		{
			legsAnim++;
			SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
		}
		else if (destAnim < legsAnim && SLOPERECALCVAR < pm->cmd.serverTime)
		{
			legsAnim--;
			SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
		}
		else
		{
			legsAnim = destAnim;
		}

		destAnim = legsAnim;
	}
	else if (legsAnim >= LEGS_RIGHTUP1 && legsAnim <= LEGS_RIGHTUP5
		|| legsAnim >= LEGS_S1_RUP1 && legsAnim <= LEGS_S1_RUP5
		|| legsAnim >= LEGS_S3_RUP1 && legsAnim <= LEGS_S3_RUP5
		|| legsAnim >= LEGS_S4_RUP1 && legsAnim <= LEGS_S4_RUP5
		|| legsAnim >= LEGS_S5_RUP1 && legsAnim <= LEGS_S5_RUP5
		|| legsAnim >= LEGS_S6_RUP1 && legsAnim <= LEGS_S6_RUP5
		|| legsAnim >= LEGS_S7_RUP1 && legsAnim <= LEGS_S7_RUP5)
	{
		//already in right-side up
		if (destAnim > legsAnim && SLOPERECALCVAR < pm->cmd.serverTime)
		{
			legsAnim++;
			SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
		}
		else if (destAnim < legsAnim && SLOPERECALCVAR < pm->cmd.serverTime)
		{
			legsAnim--;
			SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
		}
		else
		{
			legsAnim = destAnim;
		}

		destAnim = legsAnim;
	}
	else
	{
		//in a stand of some sort?
		if (pEnt->s.NPC_class == CLASS_ATST)
		{
			if (legsAnim == BOTH_STAND1 || legsAnim == BOTH_STAND2 || legsAnim == BOTH_CROUCH1IDLE)
			{
				if (destAnim >= LEGS_LEFTUP1 && destAnim <= LEGS_LEFTUP5)
				{
					//going into left side up
					destAnim = LEGS_LEFTUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else if (destAnim >= LEGS_RIGHTUP1 && destAnim <= LEGS_RIGHTUP5)
				{
					//going into right side up
					destAnim = LEGS_RIGHTUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else
				{
					//will never get here
					return qfalse;
				}
			}
		}
		else
		{
			switch (legsAnim)
			{
			case BOTH_STAND1:
			case TORSO_WEAPONREADY1:
			case TORSO_WEAPONREADY2:
			case TORSO_WEAPONREADY3:
			case TORSO_WEAPONREADY10:

				if (destAnim >= LEGS_S1_LUP1 && destAnim <= LEGS_S1_LUP5)
				{
					//going into left side up
					destAnim = LEGS_S1_LUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else if (destAnim >= LEGS_S1_RUP1 && destAnim <= LEGS_S1_RUP5)
				{
					//going into right side up
					destAnim = LEGS_S1_RUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else
				{
					//will never get here
					return qfalse;
				}
				break;
			case BOTH_STAND2: //single-saber, medium style
			case BOTH_SABERFAST_STANCE: //single-saber, fast style
			case BOTH_SABERSLOW_STANCE: //single-saber, strong style
			case BOTH_SABERTAVION_STANCE: //tavion saberstyle
			case BOTH_SABERDESANN_STANCE: //desann saber style
			case BOTH_SABERSTANCE_STANCE: //similar to stand2
			case BOTH_SABERYODA_STANCE: //YODA saber style
			case BOTH_SABERBACKHAND_STANCE: //BACKHAND saber style
			case BOTH_SABERSTANCE_STANCE_ALT: //alt idle
			case BOTH_SABEROBI_STANCE: //obiwan
			case BOTH_SABEREADY_STANCE: //ready
			case BOTH_SABER_REY_STANCE: //rey
			case BOTH_CROUCH1IDLE:
				if (destAnim >= LEGS_LEFTUP1 && destAnim <= LEGS_LEFTUP5)
				{
					//going into left side up
					destAnim = LEGS_LEFTUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else if (destAnim >= LEGS_RIGHTUP1 && destAnim <= LEGS_RIGHTUP5)
				{
					//going into right side up
					destAnim = LEGS_RIGHTUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else
				{
					//will never get here
					return qfalse;
				}
				break;
			case BOTH_STAND3:
				if (destAnim >= LEGS_S3_LUP1 && destAnim <= LEGS_S3_LUP5)
				{
					//going into left side up
					destAnim = LEGS_S3_LUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else if (destAnim >= LEGS_S3_RUP1 && destAnim <= LEGS_S3_RUP5)
				{
					//going into right side up
					destAnim = LEGS_S3_RUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else
				{
					//will never get here
					return qfalse;
				}
				break;
			case BOTH_STAND4:
				if (destAnim >= LEGS_S4_LUP1 && destAnim <= LEGS_S4_LUP5)
				{
					//going into left side up
					destAnim = LEGS_S4_LUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else if (destAnim >= LEGS_S4_RUP1 && destAnim <= LEGS_S4_RUP5)
				{
					//going into right side up
					destAnim = LEGS_S4_RUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else
				{
					//will never get here
					return qfalse;
				}
				break;
			case BOTH_STAND5:
				if (destAnim >= LEGS_S5_LUP1 && destAnim <= LEGS_S5_LUP5)
				{
					//going into left side up
					destAnim = LEGS_S5_LUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else if (destAnim >= LEGS_S5_RUP1 && destAnim <= LEGS_S5_RUP5)
				{
					//going into right side up
					destAnim = LEGS_S5_RUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else
				{
					//will never get here
					return qfalse;
				}
				break;
			case BOTH_SABERDUAL_STANCE_ALT: //alt dual
			case BOTH_SABERDUAL_STANCE:
			case BOTH_SABERDUAL_STANCE_GRIEVOUS: //dual saber style
			case BOTH_SABERDUAL_STANCE_IDLE:
				if (destAnim >= LEGS_S6_LUP1 && destAnim <= LEGS_S6_LUP5)
				{
					//going into left side up
					destAnim = LEGS_S6_LUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else if (destAnim >= LEGS_S6_RUP1 && destAnim <= LEGS_S6_RUP5)
				{
					//going into right side up
					destAnim = LEGS_S6_RUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else
				{
					//will never get here
					return qfalse;
				}
				break;
			case BOTH_SABERSTAFF_STANCE_ALT: //alt staff
			case BOTH_SABERSTAFF_STANCE:
			case BOTH_SABERSTAFF_STANCE_IDLE:
				if (destAnim >= LEGS_S7_LUP1 && destAnim <= LEGS_S7_LUP5)
				{
					//going into left side up
					destAnim = LEGS_S7_LUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else if (destAnim >= LEGS_S7_RUP1 && destAnim <= LEGS_S7_RUP5)
				{
					//going into right side up
					destAnim = LEGS_S7_RUP1;
					SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
				}
				else
				{
					//will never get here
					return qfalse;
				}
				break;
			case BOTH_STAND6:
			default:
				return qfalse;
			}
		}
	}
	//step 7: set the anim
	PM_ContinueLegsAnim(destAnim);

	return qtrue;
}

extern int WeaponReadyLegsAnim[WP_NUM_WEAPONS];

//rww - slowly back out of slope leg anims, to prevent skipping between slope anims and general jittering
static int PM_LegsSlopeBackTransition(const int desiredAnim)
{
	const int anim = pm->ps->legsAnim;
	int resultingAnim = desiredAnim;

	switch (anim)
	{
	case LEGS_LEFTUP2: //# On a slope with left foot 8 higher than right
	case LEGS_LEFTUP3: //# On a slope with left foot 12 higher than right
	case LEGS_LEFTUP4: //# On a slope with left foot 16 higher than right
	case LEGS_LEFTUP5: //# On a slope with left foot 20 higher than right
	case LEGS_RIGHTUP2: //# On a slope with RIGHT foot 8 higher than left
	case LEGS_RIGHTUP3: //# On a slope with RIGHT foot 12 higher than left
	case LEGS_RIGHTUP4: //# On a slope with RIGHT foot 16 higher than left
	case LEGS_RIGHTUP5: //# On a slope with RIGHT foot 20 higher than left
	case LEGS_S1_LUP2:
	case LEGS_S1_LUP3:
	case LEGS_S1_LUP4:
	case LEGS_S1_LUP5:
	case LEGS_S1_RUP2:
	case LEGS_S1_RUP3:
	case LEGS_S1_RUP4:
	case LEGS_S1_RUP5:
	case LEGS_S3_LUP2:
	case LEGS_S3_LUP3:
	case LEGS_S3_LUP4:
	case LEGS_S3_LUP5:
	case LEGS_S3_RUP2:
	case LEGS_S3_RUP3:
	case LEGS_S3_RUP4:
	case LEGS_S3_RUP5:
	case LEGS_S4_LUP2:
	case LEGS_S4_LUP3:
	case LEGS_S4_LUP4:
	case LEGS_S4_LUP5:
	case LEGS_S4_RUP2:
	case LEGS_S4_RUP3:
	case LEGS_S4_RUP4:
	case LEGS_S4_RUP5:
	case LEGS_S5_LUP2:
	case LEGS_S5_LUP3:
	case LEGS_S5_LUP4:
	case LEGS_S5_LUP5:
	case LEGS_S5_RUP2:
	case LEGS_S5_RUP3:
	case LEGS_S5_RUP4:
	case LEGS_S5_RUP5:
		if (pm->ps->slopeRecalcTime < pm->cmd.serverTime)
		{
			resultingAnim = anim - 1;
			pm->ps->slopeRecalcTime = pm->cmd.serverTime + 8; //SLOPE_RECALC_INT;
		}
		else
		{
			resultingAnim = anim;
		}
		VectorClear(pm->ps->velocity);
		break;
	default:;
	}

	return resultingAnim;
}

static void PM_LadderAnim(void)
{
	const int legsAnim = pm->ps->legsAnim;
	//FIXME: no start or stop anims
	if (pm->cmd.forwardmove || pm->cmd.rightmove || pm->cmd.upmove)
	{
		PM_SetAnim(SETANIM_LEGS, BOTH_LADDER_UP1, SETANIM_FLAG_NORMAL);
	}
	else
	{
		//stopping
		if (legsAnim == BOTH_SWIMFORWARD)
		{
			//I was swimming
			if (!pm->ps->legsTimer)
			{
				PM_SetAnim(SETANIM_LEGS, BOTH_LADDER_IDLE, SETANIM_FLAG_NORMAL);
			}
		}
		else
		{
			//idle
			if (!(pm->ps->pm_flags & PMF_DUCKED) && pm->cmd.upmove >= 0)
			{
				//not crouching
				PM_SetAnim(SETANIM_LEGS, BOTH_LADDER_IDLE, SETANIM_FLAG_NORMAL);
			}
		}
	}
}

static void PM_SwimFloatAnim(void)
{
	const int legsAnim = pm->ps->legsAnim;
	pm->xyspeed = sqrt(pm->ps->velocity[0] * pm->ps->velocity[0] + pm->ps->velocity[1] * pm->ps->velocity[1]);

	if (pm->ps->weapon == WP_SABER && !pm->ps->saberHolstered)
	{
		pm->ps->saberHolstered = 2;
#ifdef _GAME
		gentity_t* self = &g_entities[pm->ps->clientNum];
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberoff.mp3"));
#endif
	}
	else
	{
		if (PM_IsGunner())
		{
#ifdef _GAME
			gentity_t* self = &g_entities[pm->ps->clientNum];
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/change.wav"));
#endif
		}
	}

	if (pm->watertype & CONTENTS_LADDER)
	{
		PM_LadderAnim();
	}
	else
	{
		if (pm->cmd.forwardmove || pm->cmd.rightmove || pm->cmd.upmove || pm->xyspeed > 60)
		{
			PM_SetAnim(SETANIM_LEGS, BOTH_SWIMFORWARD, SETANIM_FLAG_NORMAL);
		}
		else
		{
			//stopping
			if (legsAnim == BOTH_SWIMFORWARD)
			{
				//I was swimming
				if (!pm->ps->legsTimer)
				{
					PM_SetAnim(SETANIM_LEGS, BOTH_SWIM_IDLE1, SETANIM_FLAG_NORMAL);
				}
			}
			else
			{
				//idle
				if (!(pm->ps->pm_flags & PMF_DUCKED) && pm->cmd.upmove >= 0)
				{
					//not crouching
					PM_SetAnim(SETANIM_LEGS, BOTH_SWIM_IDLE1, SETANIM_FLAG_NORMAL);
				}
			}
		}
	}
}

/*
===============
PM_Footsteps
===============
*/

extern qboolean PM_SaberInBrokenParry(int move);

static void PM_Footsteps(void)
{
	float bobmove;
	int old;
	int setAnimFlags = 0;

	const qboolean is_holding_block_button = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Holding Block Button

	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);

	if (PM_SpinningSaberAnim(pm->ps->legsAnim) && pm->ps->legsTimer)
	{
		//spinning
		return;
	}
	if (PM_InKnockDown(pm->ps) || BG_InRoll(pm->ps, pm->ps->legsAnim))
	{
		//don't interrupt knockdowns with footstep stuff.
		return;
	}

	//trying to move laterally
	if (pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf && pm_entSelf->s.NPC_class == CLASS_RANCOR
		//does this catch NPCs, too?
		|| pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf && pm_entSelf->s.NPC_class == CLASS_WAMPA)
		//does this catch NPCs, too?
	{
		//atst, Rancor & Wampa, only override turn anims on legs (no torso)
		if (pm->ps->legsAnim == BOTH_TURN_LEFT1 || pm->ps->legsAnim == BOTH_TURN_RIGHT1)
		{
			//moving overrides turning
			setAnimFlags |= SETANIM_FLAG_OVERRIDE;
		}
	}
	else
	{
		//all other NPCs...
		if (PM_InSaberAnim(pm->ps->legsAnim)
			&& !PM_SpinningSaberAnim(pm->ps->legsAnim)
			&& !PM_SaberInBrokenParry(pm->ps->saber_move)
			|| PM_SaberStanceAnim(pm->ps->legsAnim)
			|| PM_SaberDrawPutawayAnim(pm->ps->legsAnim)
			|| pm->ps->legsAnim == BOTH_BUTTON_HOLD
			|| pm->ps->legsAnim == BOTH_BUTTON_RELEASE
			|| pm->ps->legsAnim == BOTH_THERMAL_READY
			|| pm->ps->legsAnim == BOTH_THERMAL_THROW
			|| pm->ps->legsAnim == BOTH_ATTACK10
			|| pm->ps->legsAnim == BOTH_BUTTON_HOLD
			|| pm->ps->legsAnim == BOTH_BUTTON_RELEASE
			|| pm->ps->legsAnim == BOTH_THERMAL_READY
			|| pm->ps->legsAnim == BOTH_THERMAL_THROW
			|| pm->ps->legsAnim == BOTH_ATTACK10
			|| PM_LandingAnim(pm->ps->legsAnim)
			|| PM_PainAnim(pm->ps->legsAnim)
			|| PM_ForceAnim(pm->ps->legsAnim))
		{
			//legs are in a saber anim, and not spinning, be sure to override it
			setAnimFlags |= SETANIM_FLAG_OVERRIDE;
		}
	}

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = sqrt(pm->ps->velocity[0] * pm->ps->velocity[0] + pm->ps->velocity[1] * pm->ps->velocity[1]);

	if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
	{
		// airborne leaves position in cycle intact, but doesn't advance
		if (pm->waterlevel > 1)
		{
			if (pm->ps->weapon == WP_SABER && !pm->ps->saberHolstered)
			{
				pm->ps->saberHolstered = 2;
#ifdef _GAME
				gentity_t* self = &g_entities[pm->ps->clientNum];
				G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberoff.mp3"));
#endif
			}
			else
			{
				if (PM_IsGunner())
				{
#ifdef _GAME
					gentity_t* self = &g_entities[pm->ps->clientNum];
					G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/change.wav"));
#endif
				}
			}
			pm->cmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCEPOWER | BUTTON_DASH);

			if (pm->ps->pm_flags & PMF_LADDER || pm->watertype & CONTENTS_LADDER)
			{
				// on ladder
				PM_LadderAnim();
			}
			else
			{
				PM_SwimFloatAnim();
			}
		}
		return;
	}
	// if not trying to move
	else if (!pm->cmd.forwardmove && !pm->cmd.rightmove || pm->ps->speed == 0)
	{
		if (pm->xyspeed < 5)
		{
			pm->ps->bobCycle = 0; // start at beginning of cycle again
			if (pm->ps->clientNum >= MAX_CLIENTS &&
				pm_entSelf &&
				pm_entSelf->s.NPC_class == CLASS_RANCOR)
			{
				if (pm->ps->eFlags2 & EF2_USE_ALT_ANIM)
				{
					//holding someone
					PM_ContinueLegsAnim(BOTH_STAND4);
				}
				else if (pm->ps->eFlags2 & EF2_ALERTED)
				{
					//have an enemy or have had one since we spawned
					PM_ContinueLegsAnim(BOTH_STAND2);
				}
				else
				{
					//just stand there
					PM_ContinueLegsAnim(BOTH_STAND1);
				}
			}
			else if (pm->ps->clientNum >= MAX_CLIENTS &&
				pm_entSelf &&
				pm_entSelf->s.NPC_class == CLASS_WAMPA)
			{
				if (pm->ps->eFlags2 & EF2_USE_ALT_ANIM)
				{
					//holding a victim
					PM_ContinueLegsAnim(BOTH_STAND2);
				}
				else
				{
					//not holding a victim
					PM_ContinueLegsAnim(BOTH_STAND1);
				}
			}
			else if (pm->ps->pm_flags & PMF_DUCKED || pm->ps->pm_flags & PMF_ROLLING)
			{
				if (pm->ps->legsAnim != BOTH_CROUCH1IDLE)
				{
					PM_SetAnim(SETANIM_LEGS, BOTH_CROUCH1IDLE, setAnimFlags);
				}
				else
				{
					PM_ContinueLegsAnim(BOTH_CROUCH1IDLE);
				}
			}
			else
			{
				if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1)
				{
					PM_ContinueLegsAnim(TORSO_WEAPONREADY4);
				}
				else if (pm_entSelf && pm_entSelf->s.botclass == BCLASS_SBD)
				{
					PM_ContinueLegsAnim(SBD_WEAPON_STANDING);
				}
				else if (pm->ps->weapon == WP_BRYAR_OLD)
				{
					PM_ContinueLegsAnim(SBD_WEAPON_STANDING);
				}
				else
				{
					if (pm->ps->weapon == WP_SABER && BG_SabersOff(pm->ps))
					{
						if (!PM_AdjustStandAnimForSlope())
						{
							PM_ContinueLegsAnim(PM_LegsSlopeBackTransition(BOTH_STAND1));
						}
					}
					else
					{
						if (pm->ps->weapon != WP_SABER || !PM_AdjustStandAnimForSlope())
						{
							if (pm->ps->weapon == WP_SABER)
							{
#ifdef _GAME
								if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
								{
									// Some special bot stuff.
									PM_ContinueLegsAnim(PM_LegsSlopeBackTransition(PM_ReadyPoseForsaber_anim_levelBOT()));
								}
								else
#endif
								{
									if (is_holding_block_button && pm->cmd.buttons & BUTTON_WALKING)
									{
										if (pm->ps->fd.saberAnimLevel == SS_DUAL)
										{
											PM_ContinueLegsAnim(
												PM_LegsSlopeBackTransition(PM_BlockingPoseForsaber_anim_levelDual()));
										}
										else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
										{
											PM_ContinueLegsAnim(
												PM_LegsSlopeBackTransition(PM_BlockingPoseForsaber_anim_levelStaff()));
										}
										else
										{
											PM_ContinueLegsAnim(
												PM_LegsSlopeBackTransition(PM_BlockingPoseForsaber_anim_levelSingle()));
										}
									}
									else
									{
										PM_ContinueLegsAnim(PM_LegsSlopeBackTransition(PM_IdlePoseForsaber_anim_level()));
									}
								}
							}
							else
							{
								PM_ContinueLegsAnim(PM_LegsSlopeBackTransition(WeaponReadyLegsAnim[pm->ps->weapon]));
							}
						}
					}
				}
			}
		}
		if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
			pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
		{
			pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
			pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
			g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
		}
		return;
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		int rolled = 0;

		bobmove = 0.5; // ducked characters bob much faster

		if (!PM_InOnGroundAnim(pm->ps->legsAnim) //not on the ground
			&& (!PM_InRollIgnoreTimer(pm->ps) || (!pm->ps->legsTimer && pm->cmd.upmove < 0)))//not in a roll (or you just finished one and you're still holding crouch)
		{
			if ((PM_RunningAnim(pm->ps->legsAnim)
				|| pm->ps->legsAnim == BOTH_FORCEHEAL_START
				|| PM_CanRollFromSoulCal(pm->ps)
				|| pm->cmd.buttons & BUTTON_DASH) &&
				!BG_InRoll(pm->ps, pm->ps->legsAnim))
			{
				//roll!
				rolled = pm_try_roll();
			}

			if (!rolled)
			{
				//if the roll failed or didn't attempt, do standard crouching anim stuff.
				if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
				{
					if (pm->ps->legsAnim != BOTH_CROUCH1WALKBACK)
					{
						PM_SetAnim(SETANIM_LEGS, BOTH_CROUCH1WALKBACK, setAnimFlags);
					}
					else
					{
						PM_ContinueLegsAnim(BOTH_CROUCH1WALKBACK);
					}
				}
				else
				{
					if (pm->ps->legsAnim != BOTH_CROUCH1WALK)
					{
						PM_SetAnim(SETANIM_LEGS, BOTH_CROUCH1WALK, setAnimFlags);
					}
					else
					{
						PM_ContinueLegsAnim(BOTH_CROUCH1WALK);
					}
				}
#ifdef _GAME
				if (!Q_irand(0, 19))
				{
					//5% chance of making an alert
					AddSoundEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 16, AEL_MINOR, qtrue, qtrue);
				}
#endif
			}
			else
			{
				//otherwise send us into the roll
				pm->ps->legsTimer = 0;
				pm->ps->legsAnim = 0;
				PM_SetAnim(SETANIM_BOTH, rolled, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				PM_AddEventWithParm(EV_ROLL, 0);
				pm->maxs[2] = pm->ps->crouchheight; //CROUCH_MAXS_2;
				pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
				pm->ps->pm_flags &= ~PMF_DUCKED;
				pm->ps->pm_flags |= PMF_ROLLING;
#ifdef _GAME
				AddSoundEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 128, AEL_MINOR, qtrue, qtrue);
#endif
			}

			if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
				pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
			{
				pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
				pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
				g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
			}
		}
	}
	else if (pm->ps->pm_flags & PMF_ROLLING && !BG_InRoll(pm->ps, pm->ps->legsAnim) &&
		!PM_InRollComplete(pm->ps, pm->ps->legsAnim))
	{
		bobmove = 0.5; // ducked characters bob much faster

		if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
		{
			if (pm->ps->legsAnim != BOTH_CROUCH1WALKBACK)
			{
				PM_SetAnim(SETANIM_LEGS, BOTH_CROUCH1WALKBACK, setAnimFlags);
			}
			else
			{
				PM_ContinueLegsAnim(BOTH_CROUCH1WALKBACK);
			}
		}
		else
		{
			if (pm->ps->legsAnim != BOTH_CROUCH1WALK)
			{
				PM_SetAnim(SETANIM_LEGS, BOTH_CROUCH1WALK, setAnimFlags);
			}
			else
			{
				PM_ContinueLegsAnim(BOTH_CROUCH1WALK);
			}
		}
	}
	else
	{
		int desiredAnim = -1;

		if ((pm->ps->legsAnim == BOTH_FORCELAND1 ||
			pm->ps->legsAnim == BOTH_FORCELANDBACK1 ||
			pm->ps->legsAnim == BOTH_FORCELANDRIGHT1 ||
			pm->ps->legsAnim == BOTH_FORCELANDLEFT1) &&
			pm->ps->legsTimer > 0)
		{
			//let it finish first
			bobmove = 0.2f;
		}
		else if (!(pm->cmd.buttons & BUTTON_WALKING))
		{
			//running
			bobmove = 0.4f; // faster speeds bob faster
			if (pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf && pm_entSelf->s.NPC_class == CLASS_WAMPA)
			{
				if (pm->ps->eFlags2 & EF2_USE_ALT_ANIM)
				{
					//full on run, on all fours
					desiredAnim = BOTH_RUN1;
				}
				else
				{
					//regular, upright run
					desiredAnim = BOTH_RUN2;
				}
			}
			else if (pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf && pm_entSelf->s.NPC_class == CLASS_RANCOR)
			{
				//no run anims
				if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
				{
					desiredAnim = BOTH_WALKBACK1;
				}
				else
				{
					desiredAnim = BOTH_WALK1;
				}
			}
			else if (pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf && pm_entSelf->s.NPC_class == CLASS_JAWA)
			{
				// Jawa has a special run animation :D
				desiredAnim = BOTH_RUN4;
				bobmove = 0.2f;
			}
			else if (pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf && pm_entSelf->s.NPC_class == CLASS_VADER)
			{
				// has a special run animation :D
				desiredAnim = BOTH_VADERRUN1;
			}
			else if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
			{
				if (pm->ps->weapon != WP_SABER)
				{
					if (pm->ps->weapon == WP_BRYAR_OLD || pm_entSelf && pm_entSelf->s.botclass == BCLASS_SBD)
					{
						desiredAnim = SBD_RUNBACK_WEAPON;
					}
					else
					{
						desiredAnim = BOTH_RUNBACK1;
					}
				}
				else
				{
					switch (pm->ps->fd.saberAnimLevel)
					{
					case SS_STAFF:
						if (pm->ps->saberHolstered > 1)
						{
							//saber off
							desiredAnim = BOTH_RUNBACK1;
						}
						else
						{
							desiredAnim = BOTH_RUNBACK2;
						}
						break;
					case SS_DUAL:
						if (pm->ps->saberHolstered > 1)
						{
							//sabers off
							desiredAnim = BOTH_RUNBACK1;
						}
						else
						{
							desiredAnim = BOTH_RUNBACK2;
						}
						break;
					case SS_FAST:
					case SS_MEDIUM:
					case SS_STRONG:
					case SS_DESANN:
					case SS_TAVION:
						if (pm->ps->saberHolstered)
						{
							//saber off
							desiredAnim = BOTH_RUNBACK1;
						}
						else
						{
							desiredAnim = BOTH_RUNBACK2;
						}
						break;
					default:
						if (pm->ps->saberHolstered)
						{
							//saber off
							desiredAnim = BOTH_RUNBACK1;
						}
						else
						{
							desiredAnim = BOTH_RUNBACK2;
						}
						break;
					}
				}

				if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
					pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
				{
					pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
					pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
					g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
				}
			}
			else
			{
				// add gun run anims somehow
				if (pm->ps->weapon != WP_SABER)
				{
					if (pm->ps->weapon == WP_BRYAR_OLD || pm_entSelf->s.botclass == BCLASS_SBD)
					{
						if (!pm->ps->weaponTime) //not firing
						{
							PM_SetAnim(SETANIM_BOTH, SBD_RUNING_WEAPON, SETANIM_FLAG_NORMAL);
						}
						else
						{
							desiredAnim = SBD_RUNING_WEAPON;
						}

						if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
							pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
						{
							pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
							pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
							g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
						}
					}
					else if (pm->ps->weapon == WP_BRYAR_PISTOL)
					{
						if (pm->ps->eFlags & EF3_DUAL_WEAPONS)
						{
							if (!pm->ps->weaponTime) //not firing
							{
								if (pm->ps->stats[STAT_HEALTH] <= 70 && pm->ps->stats[STAT_HEALTH] >= 40)
								{
									if (pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf &&
										(pm_entSelf->s.NPC_class == CLASS_STORMTROOPER ||
											pm_entSelf && pm_entSelf->s.NPC_class == CLASS_STORMCOMMANDO ||
											pm_entSelf && pm_entSelf->s.NPC_class == CLASS_CLONETROOPER))
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN3, SETANIM_FLAG_NORMAL);
									}
									else
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN8, SETANIM_FLAG_NORMAL);
									}
								}
								else if (pm->ps->stats[STAT_HEALTH] <= 40)
								{
									if (pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf &&
										(pm_entSelf->s.NPC_class == CLASS_STORMTROOPER ||
											pm_entSelf && pm_entSelf->s.NPC_class == CLASS_STORMCOMMANDO ||
											pm_entSelf && pm_entSelf->s.NPC_class == CLASS_CLONETROOPER))
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN3, SETANIM_FLAG_NORMAL);
									}
									else
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN9, SETANIM_FLAG_NORMAL);
									}
								}
								else
								{
									if (pm->cmd.buttons & BUTTON_BLOCK && pm->ps->sprintFuel > 15)
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_SPRINT_MP, SETANIM_FLAG_NORMAL);

										if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING)
										{
											pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
										}
										if (!(pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING))
										{
											pm->ps->PlayerEffectFlags |= 1 << PEF_WEAPONSPRINTING;
#ifdef _GAME
											g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
											if (pm->ps->sprintFuel < 17) // single sprint here
											{
												pm->ps->sprintFuel -= 10;
											}
#endif
										}
									}
									else
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN2, SETANIM_FLAG_NORMAL);

										if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
											pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
										{
											pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
											pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
											g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
										}
									}
								}
							}
							else
							{
								if (pm->cmd.buttons & BUTTON_BLOCK && pm->ps->sprintFuel > 15)
								{
									desiredAnim = BOTH_SPRINT_MP;

									if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING)
									{
										pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
									}
									if (!(pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING))
									{
										pm->ps->PlayerEffectFlags |= 1 << PEF_WEAPONSPRINTING;
#ifdef _GAME
										g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
										if (pm->ps->sprintFuel < 17) // single sprint here
										{
											pm->ps->sprintFuel -= 10;
										}
#endif
									}
								}
								else
								{
									desiredAnim = BOTH_RUN2;

									if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
										pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
									{
										pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
										pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
										g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
									}
								}
							}
						}
						else
						{
							if (!pm->ps->weaponTime) //not firing
							{
								if (pm->ps->stats[STAT_HEALTH] <= 70 && pm->ps->stats[STAT_HEALTH] >= 40)
								{
									if (pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf &&
										(pm_entSelf->s.NPC_class == CLASS_STORMTROOPER ||
											pm_entSelf && pm_entSelf->s.NPC_class == CLASS_STORMCOMMANDO ||
											pm_entSelf && pm_entSelf->s.NPC_class == CLASS_CLONETROOPER))
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN3, SETANIM_FLAG_NORMAL);
									}
									else
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN8, SETANIM_FLAG_NORMAL);
									}
								}
								else if (pm->ps->stats[STAT_HEALTH] <= 40)
								{
									if (pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf &&
										(pm_entSelf->s.NPC_class == CLASS_STORMTROOPER ||
											pm_entSelf && pm_entSelf->s.NPC_class == CLASS_STORMCOMMANDO ||
											pm_entSelf && pm_entSelf->s.NPC_class == CLASS_CLONETROOPER))
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN3, SETANIM_FLAG_NORMAL);
									}
									else
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN9, SETANIM_FLAG_NORMAL);
									}
								}
								else
								{
									if (pm->cmd.buttons & BUTTON_BLOCK && pm->ps->sprintFuel > 15)
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_SPRINT_MP, SETANIM_FLAG_NORMAL);

										if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING)
										{
											pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
										}
										if (!(pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING))
										{
											pm->ps->PlayerEffectFlags |= 1 << PEF_WEAPONSPRINTING;
#ifdef _GAME
											g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
											if (pm->ps->sprintFuel < 17) // single sprint here
											{
												pm->ps->sprintFuel -= 10;
											}
#endif
										}
									}
									else
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN5, SETANIM_FLAG_NORMAL);

										if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
											pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
										{
											pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
											pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
											g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
										}
									}
								}
							}
							else
							{
								if (pm->cmd.buttons & BUTTON_BLOCK && pm->ps->sprintFuel > 15)
								{
									desiredAnim = BOTH_SPRINT_MP;

									if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING)
									{
										pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
									}
									if (!(pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING))
									{
										pm->ps->PlayerEffectFlags |= 1 << PEF_WEAPONSPRINTING;
#ifdef _GAME
										g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
										if (pm->ps->sprintFuel < 17) // single sprint here
										{
											pm->ps->sprintFuel -= 10;
										}
#endif
									}
								}
								else
								{
									desiredAnim = BOTH_RUN5;

									if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
										pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
									{
										pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
										pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
										g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
									}
								}
							}
						}
					}
					else if (pm->ps->weapon == WP_BOWCASTER ||
						pm->ps->weapon == WP_FLECHETTE ||
						pm->ps->weapon == WP_DISRUPTOR ||
						pm->ps->weapon == WP_DEMP2 ||
						pm->ps->weapon == WP_REPEATER ||
						pm->ps->weapon == WP_BLASTER || pm->ps->weapon == WP_STUN_BATON)
					{
						if (!pm->ps->weaponTime) //not firing
						{
							if (pm->ps->stats[STAT_HEALTH] <= 70 && pm->ps->stats[STAT_HEALTH] >= 40)
							{
								if (pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf &&
									(pm_entSelf->s.NPC_class == CLASS_STORMTROOPER ||
										pm_entSelf && pm_entSelf->s.NPC_class == CLASS_STORMCOMMANDO ||
										pm_entSelf && pm_entSelf->s.NPC_class == CLASS_CLONETROOPER))
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_RUN3_MP, SETANIM_FLAG_NORMAL);
								}
								else
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, SETANIM_FLAG_NORMAL);
								}
							}
							else if (pm->ps->stats[STAT_HEALTH] <= 40)
							{
								if (pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf &&
									(pm_entSelf->s.NPC_class == CLASS_STORMTROOPER ||
										pm_entSelf && pm_entSelf->s.NPC_class == CLASS_STORMCOMMANDO ||
										pm_entSelf && pm_entSelf->s.NPC_class == CLASS_CLONETROOPER))
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_RUN3_MP, SETANIM_FLAG_NORMAL);
								}
								else
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_RUN8, SETANIM_FLAG_NORMAL);
								}
							}
							else
							{
								if (pm->cmd.buttons & BUTTON_BLOCK && pm->ps->sprintFuel > 15)
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_RUN3_MP, SETANIM_FLAG_NORMAL);

									if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING)
									{
										pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
									}
									if (!(pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING))
									{
										pm->ps->PlayerEffectFlags |= 1 << PEF_WEAPONSPRINTING;
#ifdef _GAME
										g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
										if (pm->ps->sprintFuel < 17) // single sprint here
										{
											pm->ps->sprintFuel -= 10;
										}
#endif
									}
								}
								else
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_RUN3, SETANIM_FLAG_NORMAL);

									if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
										pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
									{
										pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
										pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
										g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
									}
								}
							}
						}
						else
						{
							if (pm->cmd.buttons & BUTTON_BLOCK && pm->ps->sprintFuel > 15)
							{
								desiredAnim = BOTH_RUN3_MP;

								if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING)
								{
									pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
								}
								if (!(pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING))
								{
									pm->ps->PlayerEffectFlags |= 1 << PEF_WEAPONSPRINTING;
#ifdef _GAME
									g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
									if (pm->ps->sprintFuel < 17) // single sprint here
									{
										pm->ps->sprintFuel -= 10;
									}
#endif
								}
							}
							else
							{
								desiredAnim = BOTH_RUN3;

								if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
									pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
								{
									pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
									pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
									g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
								}
							}
						}
					}
					else if (pm->ps->weapon == WP_THERMAL ||
						pm->ps->weapon == WP_DET_PACK ||
						pm->ps->weapon == WP_TRIP_MINE)
					{
						if (!pm->ps->weaponTime) //not firing
						{
							if (pm->ps->stats[STAT_HEALTH] <= 70 && pm->ps->stats[STAT_HEALTH] >= 40)
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, SETANIM_FLAG_NORMAL);
							}
							else if (pm->ps->stats[STAT_HEALTH] <= 40)
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_RUN8, SETANIM_FLAG_NORMAL);
							}
							else
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_RUN6, SETANIM_FLAG_NORMAL);
							}
						}
						else
						{
							desiredAnim = BOTH_RUN6;
						}

						if (pm->cmd.buttons & BUTTON_BLOCK && pm->ps->sprintFuel > 15)
						{
							if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING)
							{
								pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
							}
							if (!(pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING))
							{
								pm->ps->PlayerEffectFlags |= 1 << PEF_WEAPONSPRINTING;
#ifdef _GAME
								g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
								if (pm->ps->sprintFuel < 17) // single sprint here
								{
									pm->ps->sprintFuel -= 10;
								}
#endif
							}
						}
						else
						{
							if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
								pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
							{
								pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
								pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
								g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
							}
						}
					}
					else if (pm->ps->weapon == WP_CONCUSSION || pm->ps->weapon == WP_ROCKET_LAUNCHER)
					{
						if (!pm->ps->weaponTime) //not firing
						{
							if (pm->ps->stats[STAT_HEALTH] <= 70 && pm->ps->stats[STAT_HEALTH] >= 40)
							{
								if (pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf &&
									(pm_entSelf->s.NPC_class == CLASS_STORMTROOPER ||
										pm_entSelf && pm_entSelf->s.NPC_class == CLASS_STORMCOMMANDO ||
										pm_entSelf && pm_entSelf->s.NPC_class == CLASS_CLONETROOPER))
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_RUN3_MP, SETANIM_FLAG_NORMAL);
								}
								else
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, SETANIM_FLAG_NORMAL);
								}
							}
							else if (pm->ps->stats[STAT_HEALTH] <= 40)
							{
								if (pm->ps->clientNum >= MAX_CLIENTS && pm_entSelf &&
									(pm_entSelf->s.NPC_class == CLASS_STORMTROOPER ||
										pm_entSelf && pm_entSelf->s.NPC_class == CLASS_STORMCOMMANDO ||
										pm_entSelf && pm_entSelf->s.NPC_class == CLASS_CLONETROOPER))
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_RUN3_MP, SETANIM_FLAG_NORMAL);
								}
								else
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_RUN8, SETANIM_FLAG_NORMAL);
								}
							}
							else
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, SETANIM_FLAG_NORMAL);
							}
						}
						else
						{
							desiredAnim = BOTH_RUN7;
						}

						if (pm->cmd.buttons & BUTTON_BLOCK && pm->ps->sprintFuel > 15)
						{
							if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING)
							{
								pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
							}
							if (!(pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING))
							{
								pm->ps->PlayerEffectFlags |= 1 << PEF_WEAPONSPRINTING;
#ifdef _GAME
								g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
								if (pm->ps->sprintFuel < 17) // single sprint here
								{
									pm->ps->sprintFuel -= 10;
								}
#endif
							}
						}
						else
						{
							if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
								pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
							{
								pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
								pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
								g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
							}
						}
					}
					else if (pm->ps && pm_entSelf
						&& pm_entSelf->s.botclass == BCLASS_WOOKIEMELEE || pm_entSelf->s.botclass == BCLASS_CHEWIE)
					{
						if (!pm->ps->weaponTime) //not firing
						{
							if (pm->ps->stats[STAT_HEALTH] <= 70 && pm->ps->stats[STAT_HEALTH] >= 40)
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, SETANIM_FLAG_NORMAL);
							}
							else if (pm->ps->stats[STAT_HEALTH] <= 40)
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_RUN8, SETANIM_FLAG_NORMAL);
							}
							else
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_RUN6, SETANIM_FLAG_NORMAL);
							}
						}
						else
						{
							desiredAnim = BOTH_RUN6;
						}

						if (pm->cmd.buttons & BUTTON_BLOCK && pm->ps->sprintFuel > 15)
						{
							if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING)
							{
								pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
							}
							if (!(pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING))
							{
								pm->ps->PlayerEffectFlags |= 1 << PEF_WEAPONSPRINTING;
#ifdef _GAME
								g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
								if (pm->ps->sprintFuel < 17) // single sprint here
								{
									pm->ps->sprintFuel -= 10;
								}
#endif
							}
						}
						else
						{
							if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
								pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
							{
								pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
								pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
								g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
							}
						}
					}
					else if (pm->ps->fd.forcePowersActive & 1 << FP_RAGE)
					{
						//in force rage
						if (!pm->ps->weaponTime) //not firing
						{
							if (pm->ps->stats[STAT_HEALTH] <= 70 && pm->ps->stats[STAT_HEALTH] >= 40)
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, SETANIM_FLAG_NORMAL);
							}
							else if (pm->ps->stats[STAT_HEALTH] <= 40)
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_RUN8, SETANIM_FLAG_NORMAL);
							}
							else
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, SETANIM_FLAG_NORMAL);
							}
						}
						else
						{
							desiredAnim = BOTH_RUN7;
						}
						if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
							pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
						{
							pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
							pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
							g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
						}
					}
					else if (pm->ps->fd.forcePowersActive & 1 << FP_SPEED && pm->ps->weapon == WP_MELEE)
					{
						if (!pm->ps->weaponTime) //not firing
						{
							PM_SetAnim(SETANIM_BOTH, BOTH_SPRINT, SETANIM_FLAG_NORMAL);
						}
						else
						{
							desiredAnim = BOTH_SPRINT;
						}
					}
					else if (pm->ps->weapon == WP_MELEE)
					{
						if (pm->ps->stats[STAT_HEALTH] <= 70 && pm->ps->stats[STAT_HEALTH] >= 40)
						{
							PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, SETANIM_FLAG_NORMAL);
						}
						else if (pm->ps->stats[STAT_HEALTH] <= 40)
						{
							PM_SetAnim(SETANIM_BOTH, BOTH_RUN8, SETANIM_FLAG_NORMAL);
						}
						else if (pm_entSelf->s.botclass == BCLASS_SBD)
						{
							desiredAnim = SBD_RUNING_WEAPON;
						}
						else
						{
							if (pm->cmd.buttons & BUTTON_BLOCK && pm->ps->sprintFuel > 15)
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_SPRINT_SABER_MP, SETANIM_FLAG_NORMAL);

								if (!(pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING))
								{
									pm->ps->PlayerEffectFlags |= 1 << PEF_SPRINTING;
#ifdef _GAME
									g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
									if (pm->ps->sprintFuel < 17) // single sprint here
									{
										pm->ps->sprintFuel -= 10;
									}
#endif
								}
							}
							else
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_RUN1, SETANIM_FLAG_NORMAL);

								if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
									pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
								{
									pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
									pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
									g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
								}
							}
						}
					}
					else
					{
						if (!pm->ps->weaponTime) //not firing
						{
							PM_SetAnim(SETANIM_BOTH, BOTH_RUN1, SETANIM_FLAG_NORMAL);
						}
						else
						{
							desiredAnim = BOTH_RUN1;
						}
					}
				} ////////////////////////////////////////////// saber running anims ///////////////////////////////////////////
				else
				{
					if (pm->ps->weapon == WP_SABER)
					{
						if (!BG_SabersOff(pm->ps)) //Saber active
						{
							if (pm->ps->fd.saberAnimLevel == SS_STAFF)
							{
								if (pm->ps->fd.forcePowersActive & 1 << FP_SPEED)
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_SPRINT, setAnimFlags);
								}
								else
								{
									if (pm->ps->stats[STAT_HEALTH] <= 70 && pm->ps->stats[STAT_HEALTH] >= 40)
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN9, setAnimFlags);
									}
									else if (pm->ps->stats[STAT_HEALTH] <= 40)
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN8, setAnimFlags);
									}
									else
									{
										if (is_holding_block_button && pm->ps->sprintFuel > 15) // staff sprint here
										{
											PM_SetAnim(SETANIM_BOTH, BOTH_RUN_STAFF, setAnimFlags);

											if (!(pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING))
											{
												pm->ps->PlayerEffectFlags |= 1 << PEF_SPRINTING;
#ifdef _GAME
												g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
												if (pm->ps->sprintFuel < 17) // single sprint here
												{
													pm->ps->sprintFuel -= 10;
												}
#endif
											}
										}
										else
										{
											PM_SetAnim(SETANIM_BOTH, BOTH_RUN1, setAnimFlags);

											if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
												pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
											{
												pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
												pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
												g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
											}
										}
									}
								}
							}
							else if (pm->ps->fd.saberAnimLevel == SS_DUAL)
							{
								if (pm->ps->fd.forcePowersActive & 1 << FP_SPEED)
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_SPRINT, setAnimFlags);

									if (!(pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING))
									{
										pm->ps->PlayerEffectFlags |= 1 << PEF_SPRINTING;
#ifdef _GAME
										g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
										if (pm->ps->sprintFuel < 17) // single sprint here
										{
											pm->ps->sprintFuel -= 10;
										}
#endif
									}
								}
								else
								{
									if (pm->ps->stats[STAT_HEALTH] <= 70 && pm->ps->stats[STAT_HEALTH] >= 40)
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, setAnimFlags);
									}
									else if (pm->ps->stats[STAT_HEALTH] <= 40)
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN8, setAnimFlags);
									}
									else
									{
										if (is_holding_block_button && pm->ps->sprintFuel > 15) //dual sprint here
										{
											if (saber1 && saber1->type == SABER_GRIE)
											{
												PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, setAnimFlags);
											}
											else if (saber1 && saber1->type == SABER_GRIE4)
											{
												PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, setAnimFlags);
											}
											else
											{
												PM_SetAnim(SETANIM_BOTH, BOTH_RUN_DUAL, setAnimFlags);
											}

											if (!(pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING))
											{
												pm->ps->PlayerEffectFlags |= 1 << PEF_SPRINTING;
#ifdef _GAME
												g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
												if (pm->ps->sprintFuel < 17) // single sprint here
												{
													pm->ps->sprintFuel -= 10;
												}
#endif
											}
										}
										else
										{
											PM_SetAnim(SETANIM_BOTH, BOTH_RUN1, setAnimFlags);

											if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
												pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
											{
												pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
												pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
												g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
											}
										}
									}
								}
							}
							else if (pm->ps->fd.saberAnimLevel == SS_FAST
								|| pm->ps->fd.saberAnimLevel == SS_MEDIUM
								|| pm->ps->fd.saberAnimLevel == SS_STRONG
								|| pm->ps->fd.saberAnimLevel == SS_DESANN
								|| pm->ps->fd.saberAnimLevel == SS_TAVION)
							{
								if (pm->ps->fd.forcePowersActive & 1 << FP_SPEED)
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_SPRINT, setAnimFlags);

									if (!(pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING))
									{
										pm->ps->PlayerEffectFlags |= 1 << PEF_SPRINTING;
#ifdef _GAME
										g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
										if (pm->ps->sprintFuel < 17) // single sprint here
										{
											pm->ps->sprintFuel -= 10;
										}
#endif
									}
								}
								else
								{
									if (pm->ps->stats[STAT_HEALTH] <= 70 && pm->ps->stats[STAT_HEALTH] >= 40)
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN9, setAnimFlags);
									}
									else if (pm->ps->stats[STAT_HEALTH] <= 40)
									{
										PM_SetAnim(SETANIM_BOTH, BOTH_RUN8, setAnimFlags);
									}
									else
									{
										if (is_holding_block_button && pm->ps->sprintFuel > 15) // single sprint here
										{
											if (saber1 && (saber1->type == SABER_BACKHAND
												|| saber1->type == SABER_ASBACKHAND)) //saber backhand
											{
												PM_SetAnim(SETANIM_BOTH, BOTH_RUN_STAFF, setAnimFlags);
											}
											else if (saber1 && saber1->type == SABER_YODA) //saber yoda
											{
												PM_SetAnim(SETANIM_BOTH, BOTH_RUN10, setAnimFlags);
											}
											else if (saber1 && saber1->type == SABER_GRIE) //saber kylo
											{
												PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, setAnimFlags);
											}
											else if (saber1 && saber1->type == SABER_GRIE4) //saber kylo
											{
												PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, setAnimFlags);
											}
											else
											{
												PM_SetAnim(SETANIM_BOTH, BOTH_SPRINT_SABER_MP, setAnimFlags);
											}

											if (!(pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING))
											{
												pm->ps->PlayerEffectFlags |= 1 << PEF_SPRINTING;
#ifdef _GAME
												g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
												if (pm->ps->sprintFuel < 17) // single sprint here
												{
													pm->ps->sprintFuel -= 10;
												}
#endif
											}
										}
										else
										{
											PM_SetAnim(SETANIM_BOTH, BOTH_RUN1, setAnimFlags);

											if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
												pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
											{
												pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
												pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
												g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
											}
										}
									}
								}
							}
						}
						else // holding saber but its off
						{
							if (pm->ps->stats[STAT_HEALTH] <= 70 && pm->ps->stats[STAT_HEALTH] >= 40)
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_RUN7, SETANIM_FLAG_NORMAL);
							}
							else if (pm->ps->stats[STAT_HEALTH] <= 40)
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_RUN8, SETANIM_FLAG_NORMAL);
							}
							else if (pm_entSelf->s.botclass == BCLASS_SBD)
							{
								desiredAnim = SBD_RUNING_WEAPON;
							}
							else
							{
								if (pm->cmd.buttons & BUTTON_BLOCK && pm->ps->sprintFuel > 15)
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_SPRINT_SABER_MP, SETANIM_FLAG_NORMAL);

									if (!(pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING))
									{
										pm->ps->PlayerEffectFlags |= 1 << PEF_SPRINTING;
#ifdef _GAME
										g_entities[pm->ps->clientNum].client->IsSprinting = qtrue;
										if (pm->ps->sprintFuel < 17) // single sprint here
										{
											pm->ps->sprintFuel -= 10;
										}
#endif
									}
								}
								else
								{
									PM_SetAnim(SETANIM_BOTH, BOTH_RUN1, SETANIM_FLAG_NORMAL);

									if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
										pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
									{
										pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
										pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
										g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
									}
								}
							}
						}
					} //////////////////////////////// end saber running anims /////////////////////////////////
				}
			}
		}
		else
		{
			bobmove = 0.2f; // walking bobs slow

			if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
			{
				if (pm->ps->weapon != WP_SABER)
				{
					if (pm->ps->weapon == WP_BRYAR_OLD || pm_entSelf && pm_entSelf->s.botclass == BCLASS_SBD)
					{
						desiredAnim = SBD_WALKBACK_NORMAL;
					}
					else
					{
						desiredAnim = BOTH_WALKBACK1;
					}
				}
				else
				{
					switch (pm->ps->fd.saberAnimLevel)
					{
					case SS_STAFF:
						if (pm->ps->saberHolstered > 1)
						{
							desiredAnim = BOTH_WALKBACK1;
						}
						else if (pm->ps->saberHolstered)
						{
							desiredAnim = BOTH_WALKBACK1;
						}
						else
						{
							desiredAnim = BOTH_WALKBACK1;
						}
						break;
					case SS_DUAL:
						if (pm->ps->saberHolstered > 1)
						{
							desiredAnim = BOTH_WALKBACK1;
						}
						else if (pm->ps->saberHolstered && (!pm->ps->saberInFlight || pm->ps->saberEntityNum))
						{
							desiredAnim = BOTH_WALKBACK1;
						}
						else
						{
							desiredAnim = BOTH_WALKBACK1;
						}
						break;
					case SS_FAST:
					case SS_MEDIUM:
					case SS_STRONG:
					case SS_DESANN:
					case SS_TAVION:
						if (pm->ps->saberHolstered > 1)
						{
							desiredAnim = BOTH_WALKBACK1;
						}
						else if (pm->ps->saberHolstered && (!pm->ps->saberInFlight || pm->ps->saberEntityNum))
						{
							desiredAnim = BOTH_WALKBACK1;
						}
						else
						{
							desiredAnim = BOTH_WALKBACK1;
						}
						break;
					default:
						if (pm->ps->saberHolstered)
						{
							desiredAnim = BOTH_WALKBACK1;
						}
						else
						{
							desiredAnim = BOTH_WALKBACK1;
						}
						break;
					}
				}
				if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
					pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
				{
					pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
					pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
					g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
				}
			}
			else
			{
				if (pm->ps->weapon == WP_MELEE)
				{
					if (pm_entSelf->s.botclass == BCLASS_SBD)
					{
						desiredAnim = SBD_WALK_NORMAL;
					}
					else
					{
						desiredAnim = BOTH_WALK1;
					}
				}
				else if (pm->ps->weapon == WP_BRYAR_OLD || pm_entSelf->s.botclass == BCLASS_SBD)
				{
					if (!pm->ps->weaponTime) //not firing
					{
						PM_SetAnim(SETANIM_BOTH, SBD_WALK_WEAPON, SETANIM_FLAG_NORMAL);
					}
					else
					{
						desiredAnim = SBD_WALK_WEAPON;
					}
				}
				else if (BG_SabersOff(pm->ps))
				{
					desiredAnim = BOTH_WALK1;
				}
				else if (pm->ps->weapon != WP_SABER)
				{
					// add gun walk animas somehow
					if (pm->ps->weapon == WP_BRYAR_OLD || pm_entSelf->s.botclass == BCLASS_SBD)
					{
						if (!pm->ps->weaponTime) //not firing
						{
							PM_SetAnim(SETANIM_BOTH, SBD_WALK_NORMAL, SETANIM_FLAG_NORMAL);
						}
						else
						{
							desiredAnim = SBD_WALK_NORMAL;
						}
					}
					else if (pm->ps->weapon == WP_BRYAR_PISTOL)
					{
						if (pm->ps->eFlags & EF3_DUAL_WEAPONS)
						{
							if (!pm->ps->weaponTime) //not firing
							{
								desiredAnim = BOTH_WALK1;
							}
							else
							{
								desiredAnim = BOTH_WALK1;
							}
						}
						else
						{
							if (!pm->ps->weaponTime) //not firing
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_WALK8, SETANIM_FLAG_NORMAL);
							}
							else
							{
								desiredAnim = BOTH_WALK8;
							}
						}
					}
					else if (pm->ps && pm->ps->weapon == WP_BOWCASTER ||
						pm->ps->weapon == WP_FLECHETTE ||
						pm->ps->weapon == WP_DISRUPTOR ||
						pm->ps->weapon == WP_DEMP2 ||
						pm->ps->weapon == WP_REPEATER ||
						pm->ps->weapon == WP_CONCUSSION ||
						pm->ps->weapon == WP_ROCKET_LAUNCHER ||
						pm->ps->weapon == WP_BLASTER)
					{
						if (pm_entSelf->s.NPC_class == CLASS_ASSASSIN_DROID)
						{
							desiredAnim = BOTH_WALK2;
						}
						else
						{
							if (!pm->ps->weaponTime) //not firing
							{
								PM_SetAnim(SETANIM_BOTH, BOTH_WALK9, SETANIM_FLAG_NORMAL);
							}
							else
							{
								desiredAnim = BOTH_WALK2;
							}
						}
					}
					else if (pm->ps->weapon == WP_THERMAL ||
						pm->ps->weapon == WP_DET_PACK ||
						pm->ps->weapon == WP_TRIP_MINE)
					{
						if (!pm->ps->weaponTime) //not firing
						{
							desiredAnim = BOTH_WALK1;
						}
						else
						{
							desiredAnim = BOTH_WALK1;
						}
					}
					else
					{
						desiredAnim = BOTH_WALK1;
					}
				}
				else
				{
					switch (pm->ps->fd.saberAnimLevel)
					{
					case SS_STAFF:
						if (pm->ps->saberHolstered > 1)
						{
							if (pm->cmd.buttons & BUTTON_BLOCK)
							{
								desiredAnim = BOTH_WALK2;
							}
							else
							{
#ifdef _GAME
								if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
								{
									// Some special bot stuff.
									desiredAnim = BOTH_WALK2;
								}
								else
								{
									desiredAnim = BOTH_WALK1;
								}
#endif
							}
						}
						else if (pm->ps->saberHolstered)
						{
							if (pm->cmd.buttons & BUTTON_BLOCK)
							{
								desiredAnim = BOTH_WALK2;
							}
							else
							{
#ifdef _GAME
								if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
								{
									// Some special bot stuff.
									desiredAnim = BOTH_WALK2;
								}
								else
								{
									desiredAnim = BOTH_WALK1;
								}
#endif
							}
						}
						else
						{
							desiredAnim = BOTH_WALK_STAFF;
						}
						break;
					case SS_DUAL:
						if (pm->ps->saberHolstered > 1)
						{
							if (pm->cmd.buttons & BUTTON_BLOCK)
							{
								desiredAnim = BOTH_WALK2;
							}
							else
							{
#ifdef _GAME
								if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
								{
									// Some special bot stuff.
									desiredAnim = BOTH_WALK2;
								}
								else
								{
									desiredAnim = BOTH_WALK1;
								}
#endif
							}
						}
						else if (pm->ps->saberHolstered && (!pm->ps->saberInFlight || pm->ps->saberEntityNum))
						{
							desiredAnim = BOTH_WALK1;
						}
						else
						{
							desiredAnim = BOTH_WALK1;
						}
						break;
					case SS_FAST:
					case SS_MEDIUM:
					case SS_STRONG:
					case SS_DESANN:
					case SS_TAVION:
						if (pm->ps->saberHolstered)
						{
							if (pm->cmd.buttons & BUTTON_BLOCK)
							{
								desiredAnim = BOTH_WALK2;
							}
							else
							{
#ifdef _GAME
								if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
								{
									// Some special bot stuff.
									desiredAnim = BOTH_WALK2;
								}
								else
								{
									desiredAnim = BOTH_WALK1;
								}
#endif
							}
						}
						else
						{
							if (pm->cmd.buttons & BUTTON_BLOCK)
							{
								desiredAnim = BOTH_WALK2;
							}
							else
							{
#ifdef _GAME
								if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
								{
									// Some special bot stuff.
									desiredAnim = BOTH_WALK2;
								}
								else
								{
									desiredAnim = BOTH_WALK1;
								}
#endif
							}
						}
						break;
					default:
						if (pm->ps->saberHolstered)
						{
							desiredAnim = BOTH_WALK1;
						}
						else
						{
							if (pm->cmd.buttons & BUTTON_BLOCK)
							{
								desiredAnim = BOTH_WALK2;
							}
							else
							{
#ifdef _GAME
								if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
								{
									// Some special bot stuff.
									desiredAnim = BOTH_WALK2;
								}
								else
								{
									desiredAnim = BOTH_WALK1;
								}
#endif
							}
						}
						break;
					}
				}
				if (pm->ps->PlayerEffectFlags & 1 << PEF_SPRINTING ||
					pm->ps->PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
				{
					pm->ps->PlayerEffectFlags &= ~(1 << PEF_SPRINTING);
					pm->ps->PlayerEffectFlags &= ~(1 << PEF_WEAPONSPRINTING);
#ifdef _GAME
					g_entities[pm->ps->clientNum].client->IsSprinting = qfalse;
#endif
				}
			}
#ifdef _GAME
			if (!Q_irand(0, 9))
			{
				//10% chance of a small alert, mainly for the sand_creature
				AddSoundEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 16, AEL_MINOR, qtrue, qtrue);
			}
#endif
		}

		if (desiredAnim != -1)
		{
			const int ires = PM_LegsSlopeBackTransition(desiredAnim);

			if (pm->ps->legsAnim != desiredAnim && ires == desiredAnim)
			{
				PM_SetAnim(SETANIM_LEGS, desiredAnim, setAnimFlags);
			}
			else
			{
				PM_ContinueLegsAnim(ires);
			}
		}
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)(old + bobmove * pml.msec) & 255;

	// if we just crossed a cycle boundary, play an appropriate footstep event
	if ((old + 64 ^ pm->ps->bobCycle + 64) & 128)
	{
		pm->ps->footstepTime = pm->cmd.serverTime + 300;
		//footstep sound event
#ifdef _GAME

		qboolean footstep = qfalse;

		if (pm->waterlevel == 0 && footstep && !pm->noFootsteps)
		{
			// on ground will only play sounds if running
			vec3_t bottom;

			VectorCopy(pm->ps->origin, bottom);
			bottom[2] += pm->mins[2];
			{
				AddSoundEvent(&g_entities[pm->ps->clientNum], bottom, 256, AEL_MINOR, qtrue, qtrue);
			}
		}
#endif
		if (pm->waterlevel == 1)
		{
			// splashing
			PM_AddEvent(EV_FOOTSPLASH);
			//sound/sight events for foot splashing
#ifdef _GAME
			AddSoundEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 384, AEL_SUSPICIOUS, qfalse, qtrue);
			//was bottom
			AddSightEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 512, AEL_MINOR, 0.0f);
#endif
		}
		else if (pm->waterlevel == 2)
		{
			// wading / swimming at surface
			PM_AddEvent(EV_SWIM);
			//sound/sight events for wading
#ifdef _GAME
			AddSoundEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 256, AEL_MINOR, qfalse, qtrue);
			AddSightEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 512, AEL_SUSPICIOUS, 0);
#endif
		}
		else if (pm->waterlevel == 3)
		{
			// no sound when completely underwater
		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents(void)
{
	qboolean impact_splash = qfalse;

	if (pm->watertype & CONTENTS_LADDER) //fake water for ladder
	{
		return;
	}

	// if just entered a water volume, play a sound

	if (!pml.previous_waterlevel && pm->waterlevel)
	{
		if (pm->watertype & CONTENTS_LAVA)
		{
			PM_AddEvent(EV_LAVA_TOUCH);
		}
		else
		{
			PM_AddEvent(EV_WATER_TOUCH);
		}

		if (VectorLengthSquared(pm->ps->velocity) > 40000)
		{
			impact_splash = qtrue;
		}
		//sight/sound event for water touching
#ifdef _GAME
		AddSoundEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 384, AEL_SUSPICIOUS, qfalse, qfalse);
		AddSightEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 512, AEL_SUSPICIOUS, 0);
#endif
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (pml.previous_waterlevel && !pm->waterlevel)
	{
		if (pm->watertype & CONTENTS_LAVA)
		{
			PM_AddEvent(EV_LAVA_LEAVE);
		}
		else
		{
			PM_AddEvent(EV_WATER_LEAVE);
		}
		if (VectorLengthSquared(pm->ps->velocity) > 40000)
		{
			impact_splash = qtrue;
		}
		//sight/sound event for leaving water
#ifdef _GAME
		AddSoundEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 384, AEL_SUSPICIOUS, qfalse, qfalse);
		AddSightEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 512, AEL_SUSPICIOUS, 0);
#endif
	}

	if (impact_splash)
	{
		trace_t tr;
		vec3_t start, end;

		VectorCopy(pm->ps->origin, start);
		VectorCopy(pm->ps->origin, end);

		start[2] += 10;
		end[2] -= 40;

		pm->trace(&tr, start, vec3_origin, vec3_origin, end, pm->ps->clientNum, MASK_WATER);

		if (tr.fraction < 1.0f)
		{
			if (tr.contents & CONTENTS_LAVA)
			{
#ifdef _GAME
				G_PlayEffect(EFFECT_LAVA_SPLASH, tr.endpos, tr.plane.normal);
#endif
			}
			else if (tr.contents & CONTENTS_SLIME)
			{
#ifdef _GAME
				G_PlayEffect(EFFECT_ACID_SPLASH, tr.endpos, tr.plane.normal);
#endif
			}
			else //must be water
			{
#ifdef _GAME
				G_PlayEffect(EFFECT_WATER_SPLASH, tr.endpos, tr.plane.normal);
#endif
			}
		}
	}

	//
	// check for head just going under water
	//
	if (pml.previous_waterlevel != 3 && pm->waterlevel == 3)
	{
		if (pm->watertype & CONTENTS_LAVA)
		{
			PM_AddEvent(EV_LAVA_UNDER);
		}
		else
		{
			PM_AddEvent(EV_WATER_UNDER);
		}

		if (pm->ps->weapon == WP_SABER && !pm->ps->saberHolstered) //saber out
		{
			pm->ps->saberHolstered = 2;
		}
		//sight/sound event for head just going under water.
#ifdef _GAME
		AddSoundEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 256, AEL_MINOR, qfalse, qfalse);
		AddSightEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 384, AEL_MINOR, 0);
#endif
	}

	//
	// check for head just coming out of water
	//
	if (pml.previous_waterlevel == 3 && pm->waterlevel != 3)
	{
		if (pm->ps->stats[STAT_HEALTH] <= 70)
		{
			//only do this if we were drowning or about to start drowning
			PM_AddEvent(EV_WATER_CLEAR);
		}
		else
		{
			if (pm->watertype & CONTENTS_LAVA)
			{
				PM_AddEvent(EV_LAVA_LEAVE);
			}
			else
			{
				PM_AddEvent(EV_WATER_LEAVE);
			}
		}
		//sight/sound event for head just coming of water.
#ifdef _GAME
		AddSoundEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 256, AEL_MINOR, qfalse, qfalse);
		AddSightEvent(&g_entities[pm->ps->clientNum], pm->ps->origin, 384, AEL_SUSPICIOUS, 0);
#endif
	}
}

void BG_ClearRocketLock(playerState_t* ps)
{
	if (ps)
	{
		ps->rocketLockIndex = ENTITYNUM_NONE;
		ps->rocketLastValidTime = 0;
		ps->rocketLockTime = -1;
		ps->rocketTargetTime = 0;
	}
}

/*
===============
PM_BeginWeaponChange
===============
*/
void PM_BeginWeaponChange(const int weapon)
{
	const qboolean is_holding_block_button = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Holding Block Button
	const qboolean is_holding_block_button_and_attack = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
	//Active Blocking

	if (weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS)
	{
		return;
	}

	if (!(pm->ps->stats[STAT_WEAPONS] & 1 << weapon))
	{
		return;
	}

	if (pm->ps->weaponstate == WEAPON_DROPPING)
	{
		return;
	}

	if (pm->ps->weapon == WP_SABER && (is_holding_block_button || is_holding_block_button_and_attack))
	{
		return;
	}

	if (pm->ps->weapon == WP_BRYAR_PISTOL)
	{
		//Changing weaps, remove dual weaps
		pm->ps->eFlags &= ~EF3_DUAL_WEAPONS;
	}

	// turn of any kind of zooming when weapon switching.
	if (pm->ps->zoomMode)
	{
		pm->ps->zoomMode = 0;
		pm->ps->zoomTime = pm->ps->commandTime;
	}

	PM_AddEventWithParm(EV_CHANGE_WEAPON, weapon);
	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += 200;
	PM_SetAnim(SETANIM_TORSO, TORSO_DROPWEAP1, SETANIM_FLAG_OVERRIDE);

	BG_ClearRocketLock(pm->ps);
}

/*
===============
PM_FinishWeaponChange
===============
*/
void PM_FinishWeaponChange(void)
{
	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);

	int weapon = pm->cmd.weapon;
	if (weapon < WP_NONE || weapon >= WP_NUM_WEAPONS)
	{
		weapon = WP_NONE;
	}

	if (!(pm->ps->stats[STAT_WEAPONS] & 1 << weapon))
	{
		weapon = WP_NONE;
	}

	if (weapon == WP_BRYAR_PISTOL && (pm_entSelf->s.botclass == BCLASS_BOBAFETT ||
		pm_entSelf->s.botclass == BCLASS_MANDOLORIAN ||
		pm_entSelf->s.botclass == BCLASS_MANDOLORIAN1 ||
		pm_entSelf->s.botclass == BCLASS_MANDOLORIAN2))
	{
		//player playing as boba fett
		pm->ps->eFlags |= EF3_DUAL_WEAPONS;
	}
#ifdef _GAME
	else if (weapon == WP_BRYAR_PISTOL && g_entities[pm->ps->clientNum].client->skillLevel[SK_PISTOL] >= FORCE_LEVEL_3)
	{
		//Changed weaps, add dual weaps
		pm->ps->eFlags |= EF3_DUAL_WEAPONS;
	}
#endif

	if (weapon == WP_SABER)
	{
		if (!pm->ps->saberEntityNum && pm->ps->saberInFlight)
		{
			//our saber is currently dropped.  Don't allow the dropped blade to be activated.
			if (pm->ps->fd.saberAnimLevel == SS_DUAL)
			{
				//holding second saber, activate it.
				pm->ps->saberHolstered = 1;
				PM_SetSaberMove(LS_DRAW);
			}
			else
			{
				//not holding any sabers, make sure all our blades are all off.
				pm->ps->saberHolstered = 2;
			}
		}
		else if (!pm->ps->saberInFlight && !BG_FullBodyEmoteAnim(pm->ps->torsoAnim))
		{
			//have saber(s)
#ifdef _GAME
			if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
			{
				// Some special bot stuff.
				PM_SetSaberMove(LS_DRAW);
			}
			else
#endif
			{
				if (PM_RunningAnim(pm->ps->legsAnim) || pm->ps->groundEntityNum == ENTITYNUM_NONE || in_camera)
				{
					PM_SetSaberMove(LS_DRAW);
				}
				else if (PM_WalkingAnim(pm->ps->legsAnim))
				{
					PM_SetAnim(SETANIM_TORSO, BOTH_SABER_IGNITION_JFA, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					if (saber1 && (saber1->type == SABER_BACKHAND
						|| saber1->type == SABER_ASBACKHAND)) //saber backhand
					{
						PM_SetAnim(SETANIM_TORSO, BOTH_SABER_BACKHAND_IGNITION,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (saber1 && saber1->type == SABER_YODA) //saber yoda
					{
						PM_SetAnim(SETANIM_TORSO, BOTH_SABER_IGNITION_JFA, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (saber1 && saber1->type == SABER_DOOKU) //saber dooku
					{
						PM_SetAnim(SETANIM_TORSO, BOTH_DOOKU_SMALLDRAW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (saber1 && saber1->type == SABER_UNSTABLE) //saber kylo
					{
						PM_SetAnim(SETANIM_TORSO, BOTH_SABERSTANCE_STANCE_ALT,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (saber1 && saber1->type == SABER_OBIWAN) //saber kylo
					{
						PM_SetAnim(SETANIM_TORSO, BOTH_SHOWOFF_OBI, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (saber1 && saber1->type == SABER_SFX || saber1 && saber1->type == SABER_REY)
						//saber backhand
					{
						PM_SetAnim(SETANIM_TORSO, BOTH_SABER_IGNITION_JFA, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (saber1 && saber1->type == SABER_GRIE || saber1 && saber1->type == SABER_GRIE4)
						//saber GRIEVOUS
					{
						PM_SetSaberMove(LS_DRAW3);
					}
					else
					{
						PM_SetSaberMove(LS_DRAW2);
					}
				}
			}
		}
	}
	else
	{
		PM_SetAnim(SETANIM_TORSO, TORSO_RAISEWEAP1, SETANIM_FLAG_OVERRIDE);
	}
	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	if (pm->ps->weapon == WP_MELEE)
	{
		pm->ps->weaponTime += 500;
	}
	else
	{
		pm->ps->weaponTime += 250;
	}
}

#ifdef _GAME
extern void WP_GetVehicleCamPos(gentity_t* ent, gentity_t* pilot, vec3_t camPos);
#else
extern void CG_GetVehicleCamPos(vec3_t camPos);
#endif
#define MAX_XHAIR_DIST_ACCURACY	20000.0f

int BG_VehTraceFromCamPos(trace_t* cam_trace, const bgEntity_t* bg_ent, const vec3_t ent_org, const vec3_t shot_start,
	const vec3_t end, vec3_t new_end, vec3_t shot_dir, const float best_dist)
{
	//NOTE: this MUST stay up to date with the method used in CG_ScanForCrosshairEntity (where it checks the doExtraVehTraceFromViewPos bool)
	vec3_t viewDir2End, extraEnd, camPos;
	float minAutoAimDist;

#ifdef _GAME
	WP_GetVehicleCamPos((gentity_t*)bg_ent, (gentity_t*)bg_ent->m_pVehicle->m_pPilot, camPos);
#else
	CG_GetVehicleCamPos(camPos);
#endif

	minAutoAimDist = Distance(ent_org, camPos) + bg_ent->m_pVehicle->m_pVehicleInfo->length / 2.0f + 200.0f;

	VectorCopy(end, new_end);
	VectorSubtract(end, camPos, viewDir2End);
	VectorNormalize(viewDir2End);
	VectorMA(camPos, MAX_XHAIR_DIST_ACCURACY, viewDir2End, extraEnd);

	pm->trace(cam_trace, camPos, vec3_origin, vec3_origin, extraEnd, bg_ent->s.number, CONTENTS_SOLID | CONTENTS_BODY);

	if (!cam_trace->allsolid
		&& !cam_trace->startsolid
		&& cam_trace->fraction < 1.0f
		&& cam_trace->fraction * MAX_XHAIR_DIST_ACCURACY > minAutoAimDist
		&& cam_trace->fraction * MAX_XHAIR_DIST_ACCURACY - Distance(ent_org, camPos) < best_dist)
	{
		//this trace hit *something* that's closer than the thing the main trace hit, so use this result instead
		VectorCopy(cam_trace->endpos, new_end);
		VectorSubtract(new_end, shot_start, shot_dir);
		VectorNormalize(shot_dir);
		return cam_trace->entityNum + 1;
	}
	return 0;
}

static void PM_RocketLock(const float lockDist, const qboolean vehicleLock)
{
	// Not really a charge weapon, but we still want to delay fire until the button comes up so that we can
	//	implement our alt-fire locking stuff
	vec3_t ang;
	trace_t tr;

	vec3_t muzzlePoint, forward, right, up;

	if (vehicleLock)
	{
		AngleVectors(pm->ps->viewangles, forward, right, up);
		VectorCopy(pm->ps->origin, muzzlePoint);
		VectorMA(muzzlePoint, lockDist, forward, ang);
	}
	else
	{
		vec3_t muzzleOffPoint;
		AngleVectors(pm->ps->viewangles, forward, right, up);

		AngleVectors(pm->ps->viewangles, ang, NULL, NULL);

		VectorCopy(pm->ps->origin, muzzlePoint);
		VectorCopy(WP_muzzlePoint[WP_ROCKET_LAUNCHER], muzzleOffPoint);

		VectorMA(muzzlePoint, muzzleOffPoint[0], forward, muzzlePoint);
		VectorMA(muzzlePoint, muzzleOffPoint[1], right, muzzlePoint);
		muzzlePoint[2] += pm->ps->viewheight + muzzleOffPoint[2];
		ang[0] = muzzlePoint[0] + ang[0] * lockDist;
		ang[1] = muzzlePoint[1] + ang[1] * lockDist;
		ang[2] = muzzlePoint[2] + ang[2] * lockDist;
	}

	pm->trace(&tr, muzzlePoint, NULL, NULL, ang, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (vehicleLock)
	{
		//vehicles also do a trace from the camera point if the main one misses
		if (tr.fraction >= 1.0f)
		{
			trace_t camTrace;
			vec3_t newEnd, shotDir;
			if (BG_VehTraceFromCamPos(&camTrace, PM_BGEntForNum(pm->ps->clientNum), pm->ps->origin, muzzlePoint,
				tr.endpos, newEnd, shotDir, tr.fraction * lockDist))
			{
				memcpy(&tr, &camTrace, sizeof tr);
			}
		}
	}

	if (tr.fraction != 1 && tr.entityNum < ENTITYNUM_NONE && tr.entityNum != pm->ps->clientNum)
	{
		const bgEntity_t* bgEnt = PM_BGEntForNum(tr.entityNum);
		if (bgEnt && bgEnt->s.powerups & PW_CLOAKED)
		{
			pm->ps->rocketLockIndex = ENTITYNUM_NONE;
			pm->ps->rocketLockTime = 0;
		}
		else if (bgEnt && (bgEnt->s.eType == ET_PLAYER || bgEnt->s.eType == ET_NPC))
		{
			if (pm->ps->rocketLockIndex == ENTITYNUM_NONE)
			{
				pm->ps->rocketLockIndex = tr.entityNum;
				pm->ps->rocketLockTime = pm->cmd.serverTime;
			}
			else if (pm->ps->rocketLockIndex != tr.entityNum && pm->ps->rocketTargetTime < pm->cmd.serverTime)
			{
				pm->ps->rocketLockIndex = tr.entityNum;
				pm->ps->rocketLockTime = pm->cmd.serverTime;
			}
			else if (pm->ps->rocketLockIndex == tr.entityNum)
			{
				if (pm->ps->rocketLockTime == -1)
				{
					pm->ps->rocketLockTime = pm->ps->rocketLastValidTime;
				}
			}

			if (pm->ps->rocketLockIndex == tr.entityNum)
			{
				pm->ps->rocketTargetTime = pm->cmd.serverTime + 500;
			}
		}
		else if (!vehicleLock)
		{
			if (pm->ps->rocketTargetTime < pm->cmd.serverTime)
			{
				pm->ps->rocketLockIndex = ENTITYNUM_NONE;
				pm->ps->rocketLockTime = 0;
			}
		}
	}
	else if (pm->ps->rocketTargetTime < pm->cmd.serverTime)
	{
		pm->ps->rocketLockIndex = ENTITYNUM_NONE;
		pm->ps->rocketLockTime = 0;
	}
	else
	{
		if (pm->ps->rocketLockTime != -1)
		{
			pm->ps->rocketLastValidTime = pm->ps->rocketLockTime;
		}
		pm->ps->rocketLockTime = -1;
	}
}

//---------------------------------------
static qboolean PM_DoChargedWeapons(const qboolean vehicleRocketLock, const bgEntity_t* veh)
//---------------------------------------
{
	qboolean charging = qfalse,
		alt_fire = qfalse;

	if (vehicleRocketLock)
	{
		if (pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK))
		{
			//actually charging
			if (veh
				&& veh->m_pVehicle)
			{
				//just make sure we have this veh info
				if (pm->cmd.buttons & BUTTON_ATTACK
					&& g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID].fHoming
					&& pm->ps->ammo[0] >= g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID].
					iAmmoPerShot
					||
					pm->cmd.buttons & BUTTON_ALT_ATTACK
					&& g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID].fHoming
					&& pm->ps->ammo[1] >= g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID].
					iAmmoPerShot)
				{
					//pressing the appropriate fire button for the lock-on/charging weapon
					PM_RocketLock(16384, qtrue);
					charging = qtrue;
				}
				if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
				{
					alt_fire = qtrue;
				}
			}
		}
		//else, let go and should fire now
	}
	else
	{
		// If you want your weapon to be a charging weapon, just set this bit up
		switch (pm->ps->weapon)
		{
			//------------------
		case WP_BRYAR_PISTOL:

			// alt-fire charges the weapon
			if (1)
			{
				if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
				{
					charging = qtrue;
					alt_fire = qtrue;
				}
			}
			break;

		case WP_CONCUSSION:
			if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
			{
				alt_fire = qtrue;
			}
			break;

		case WP_BRYAR_OLD:

			// alt-fire charges the weapon
			if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
			{
				charging = qtrue;
				alt_fire = qtrue;
			}
			break;

			//------------------
		case WP_BOWCASTER:

			// primary fire charges the weapon
			if (pm->cmd.buttons & BUTTON_ATTACK)
			{
				charging = qtrue;
			}
			break;

			//------------------
		case WP_ROCKET_LAUNCHER:
			if (pm->cmd.buttons & BUTTON_ALT_ATTACK
				&& pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] >= weaponData[pm->ps->weapon].altEnergyPerShot)
			{
				PM_RocketLock(2048, qfalse);
				charging = qtrue;
				alt_fire = qtrue;
			}
			break;

			//------------------
		case WP_THERMAL:

			if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
			{
				alt_fire = qtrue; // override default of not being an alt-fire
				charging = qtrue;
			}
			else if (pm->cmd.buttons & BUTTON_ATTACK)
			{
				charging = qtrue;
			}
			break;

		case WP_DEMP2:
			if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
			{
				alt_fire = qtrue; // override default of not being an alt-fire
				charging = qtrue;
			}
			break;

		case WP_DISRUPTOR:
			if (pm->cmd.buttons & BUTTON_ATTACK &&
				pm->ps->zoomMode == 1 &&
				pm->ps->zoomLocked)
			{
				if (!pm->cmd.forwardmove &&
					!pm->cmd.rightmove &&
					pm->cmd.upmove <= 0)
				{
					charging = qtrue;
					alt_fire = qtrue;
				}
				else
				{
					charging = qfalse;
					alt_fire = qfalse;
				}
			}

			if (pm->ps->zoomMode != 1 &&
				pm->ps->weaponstate == WEAPON_CHARGING_ALT)
			{
				pm->ps->weaponstate = WEAPON_READY;
				charging = qfalse;
				alt_fire = qfalse;
			}
		default:;
		}
		// end switch
	}

	// set up the appropriate weapon state based on the button that's down.
	//	Note that we ALWAYS return if charging is set ( meaning the buttons are still down )
	if (charging)
	{
		if (alt_fire)
		{
			if (pm->ps->weaponstate != WEAPON_CHARGING_ALT)
			{
				// charge isn't started, so do it now
				pm->ps->weaponstate = WEAPON_CHARGING_ALT;
				pm->ps->weaponChargeTime = pm->cmd.serverTime;
				pm->ps->weaponChargeSubtractTime = pm->cmd.serverTime + weaponData[pm->ps->weapon].altChargeSubTime;

#ifdef _DEBUG
				//	Com_Printf("Starting charge\n");
#endif
				assert(pm->ps->weapon > WP_NONE);
				BG_AddPredictableEventToPlayerstate(EV_WEAPON_CHARGE_ALT, pm->ps->weapon, pm->ps);
			}

			if (vehicleRocketLock)
			{
				//check vehicle ammo
				if (veh && pm->ps->ammo[1] < g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID].
					iAmmoPerShot)
				{
					pm->ps->weaponstate = WEAPON_CHARGING_ALT;
					goto rest;
				}
			}
			else if (pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < weaponData[pm->ps->weapon].altChargeSub +
				weaponData[pm->ps->weapon].altEnergyPerShot)
			{
				pm->ps->weaponstate = WEAPON_CHARGING_ALT;

				goto rest;
			}
			else if (pm->cmd.serverTime - pm->ps->weaponChargeTime < weaponData[pm->ps->weapon].altMaxCharge)
			{
				if (pm->ps->weaponChargeSubtractTime < pm->cmd.serverTime)
				{
					pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] -= weaponData[pm->ps->weapon].altChargeSub;
					pm->ps->weaponChargeSubtractTime = pm->cmd.serverTime + weaponData[pm->ps->weapon].altChargeSubTime;
				}
			}
		}
		else
		{
			if (pm->ps->weaponstate != WEAPON_CHARGING)
			{
				// charge isn't started, so do it now
				pm->ps->weaponstate = WEAPON_CHARGING;
				pm->ps->weaponChargeTime = pm->cmd.serverTime;
				pm->ps->weaponChargeSubtractTime = pm->cmd.serverTime + weaponData[pm->ps->weapon].chargeSubTime;

#ifdef _DEBUG
				//	Com_Printf("Starting charge\n");
#endif
				BG_AddPredictableEventToPlayerstate(EV_WEAPON_CHARGE, pm->ps->weapon, pm->ps);
			}

			if (vehicleRocketLock)
			{
				if (veh && pm->ps->ammo[0] < g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID].
					iAmmoPerShot)
				{
					//check vehicle ammo
					pm->ps->weaponstate = WEAPON_CHARGING;
					goto rest;
				}
			}
			else if (pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < weaponData[pm->ps->weapon].chargeSub +
				weaponData[pm->ps->weapon].energyPerShot)
			{
				pm->ps->weaponstate = WEAPON_CHARGING;

				goto rest;
			}
			else if (pm->cmd.serverTime - pm->ps->weaponChargeTime < weaponData[pm->ps->weapon].maxCharge)
			{
				if (pm->ps->weaponChargeSubtractTime < pm->cmd.serverTime)
				{
					pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] -= weaponData[pm->ps->weapon].chargeSub;
					pm->ps->weaponChargeSubtractTime = pm->cmd.serverTime + weaponData[pm->ps->weapon].chargeSubTime;
				}
			}
		}

		return qtrue; // short-circuit rest of weapon code
	}
rest:
	// Only charging weapons should be able to set these states...so....
	//	let's see which fire mode we need to set up now that the buttons are up
	if (pm->ps->weaponstate == WEAPON_CHARGING)
	{
		// weapon has a charge, so let us do an attack
#ifdef _DEBUG
		//	Com_Printf("Firing.  Charge time=%d\n", pm->cmd.serverTime - pm->ps->weaponChargeTime);
#endif
		if (pm->ps->weapon == WP_BOWCASTER)
		{
			PM_StartTorsoAnim(WeaponAltAttackAnim[pm->ps->weapon]);
		}
		// dumb, but since we shoot a charged weapon on button-up, we need to repress this button for now
		pm->cmd.buttons |= BUTTON_ATTACK;
		pm->ps->eFlags |= EF_FIRING;
	}
	else if (pm->ps->weaponstate == WEAPON_CHARGING_ALT)
	{
		// weapon has a charge, so let us do an alt-attack
#ifdef _DEBUG
		//	Com_Printf("Firing.  Charge time=%d\n", pm->cmd.serverTime - pm->ps->weaponChargeTime);
#endif

		// dumb, but since we shoot a charged weapon on button-up, we need to repress this button for now
		pm->cmd.buttons |= BUTTON_ALT_ATTACK;
		pm->ps->eFlags |= EF_FIRING | EF_ALT_FIRING;
	}

	return qfalse; // continue with the rest of the weapon code
}

static int PM_ItemUsable(const playerState_t* ps, int forcedUse)
{
	vec3_t fwd, fwdorg, dest;
	vec3_t yawonly;
	vec3_t mins, maxs;
	vec3_t trtest;
	trace_t tr;

	if (ps->m_iVehicleNum)
	{
		return 0;
	}

	if (ps->pm_flags & PMF_USE_ITEM_HELD && bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_FLAMETHROWER)
	{
		//force to let go first
		return 0;
	}

	if (ps->duelInProgress)
	{
		//not allowed to use holdables while in a private duel.
		return 0;
	}

	if (!forcedUse)
	{
		forcedUse = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	}

	if (!BG_IsItemSelectable(forcedUse))
	{
		return 0;
	}

	switch (forcedUse)
	{
	case HI_SEEKER:
		if (ps->eFlags & EF_SEEKERDRONE)
		{
			PM_AddEventWithParm(EV_ITEMUSEFAIL, SEEKER_ALREADYDEPLOYED);
			return 0;
		}
		return 1;
	case HI_SHIELD:
		mins[0] = -8;
		mins[1] = -8;
		mins[2] = 0;

		maxs[0] = 8;
		maxs[1] = 8;
		maxs[2] = 8;

		AngleVectors(ps->viewangles, fwd, NULL, NULL);
		fwd[2] = 0;
		VectorMA(ps->origin, 64, fwd, dest);
		pm->trace(&tr, ps->origin, mins, maxs, dest, ps->clientNum, MASK_SHOT);
		if (tr.fraction > 0.9 && !tr.startsolid && !tr.allsolid)
		{
			vec3_t pos;
			VectorCopy(tr.endpos, pos);
			VectorSet(dest, pos[0], pos[1], pos[2] - 4096);
			pm->trace(&tr, pos, mins, maxs, dest, ps->clientNum, MASK_SOLID);
			if (!tr.startsolid && !tr.allsolid)
			{
				return 1;
			}
		}
		PM_AddEventWithParm(EV_ITEMUSEFAIL, SHIELD_NOROOM);
		return 0;
	case HI_MEDPAC:
	case HI_MEDPAC_BIG:
		if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
		{
			return 0;
		}
		if (ps->stats[STAT_HEALTH] <= 0 ||
			ps->eFlags & EF_DEAD)
		{
			return 0;
		}

		return 1;
	case HI_BINOCULARS:
		return 1;
	case HI_SENTRY_GUN:
		if (ps->fd.sentryDeployed)
		{
			PM_AddEventWithParm(EV_ITEMUSEFAIL, SENTRY_ALREADYPLACED);
			return 0;
		}

		yawonly[ROLL] = 0;
		yawonly[PITCH] = 0;
		yawonly[YAW] = ps->viewangles[YAW];

		VectorSet(mins, -8, -8, 0);
		VectorSet(maxs, 8, 8, 24);

		AngleVectors(yawonly, fwd, NULL, NULL);

		fwdorg[0] = ps->origin[0] + fwd[0] * 64;
		fwdorg[1] = ps->origin[1] + fwd[1] * 64;
		fwdorg[2] = ps->origin[2] + fwd[2] * 64;

		trtest[0] = fwdorg[0] + fwd[0] * 16;
		trtest[1] = fwdorg[1] + fwd[1] * 16;
		trtest[2] = fwdorg[2] + fwd[2] * 16;

		pm->trace(&tr, ps->origin, mins, maxs, trtest, ps->clientNum, MASK_PLAYERSOLID);

		if (tr.fraction != 1 && tr.entityNum != ps->clientNum || tr.startsolid || tr.allsolid)
		{
			PM_AddEventWithParm(EV_ITEMUSEFAIL, SENTRY_NOROOM);
			return 0;
		}

		return 1;
	case HI_JETPACK: //done dont show
		return 1;
	case HI_HEALTHDISP:
		return 1;
	case HI_AMMODISP:
		return 1;
	case HI_EWEB:
		return 1;
	case HI_CLOAK: //check for stuff here?
		return 1;
	case HI_FLAMETHROWER: //check for stuff here?
		if (pm->ps->jetpackFuel < 15)
		{
			//not enough fuel to fire the weapon.
			return 0;
		}
		if (PM_InGrappleMove(pm->ps->torsoAnim))
		{
			//In grapple
			return 0;
		}
		return 1;
	case HI_SWOOP:
		return 1;
	case HI_DROIDEKA:
		return 1;
	case HI_SPHERESHIELD: //check for stuff here?
		return 1;
	case HI_GRAPPLE://done dont show
		return 1;
	default:
		return 1;
	}
}

//cheesy vehicle weapon hackery
static qboolean PM_CanSetWeaponAnims(void)
{
	return qtrue;
}

//perform player anim overrides while on vehicle.
extern int PM_irand_timesync(int val1, int val2);

static void PM_VehicleWeaponAnimate(void)
{
	const bgEntity_t* veh = pm_entVeh;
	int iFlags = 0, Anim = -1;

	if (!veh ||
		!veh->m_pVehicle ||
		!veh->m_pVehicle->m_pPilot ||
		!veh->m_pVehicle->m_pPilot->playerState ||
		pm->ps->clientNum != veh->m_pVehicle->m_pPilot->playerState->clientNum)
	{
		//make sure the vehicle exists, and its pilot is this player
		return;
	}

	const Vehicle_t* p_veh = veh->m_pVehicle;

	if (p_veh->m_pVehicleInfo->type == VH_WALKER ||
		p_veh->m_pVehicleInfo->type == VH_FIGHTER)
	{
		//slightly hacky I guess, but whatever.
		return;
	}
backAgain:
	// If they're firing, play the right fire animation.
	if (pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK))
	{
		iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;

		switch (pm->ps->weapon)
		{
		case WP_SABER:
			if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
			{
				//don't do anything.. I guess.
				pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
				goto backAgain;
			}
			// If we're already in an attack animation, leave (let it continue).
			if (pm->ps->torsoTimer <= 0)
			{
				//we'll be starting a new attack
				PM_AddEvent(EV_SABER_ATTACK);
			}

			//just set it to something so we have a proper trail. This is a stupid
			//hack (much like the rest of this function)
			pm->ps->saber_move = LS_R_TL2BR;

			if (pm->ps->torsoTimer > 0 && (pm->ps->torsoAnim == BOTH_VS_ATR_S ||
				pm->ps->torsoAnim == BOTH_VS_ATL_S))
			{
				return;
			}

			// Start the attack.
			if (pm->cmd.rightmove > 0) //right side attack
			{
				Anim = BOTH_VS_ATR_S;
			}
			else if (pm->cmd.rightmove < 0) //left-side attack
			{
				Anim = BOTH_VS_ATL_S;
			}
			else //random
			{
				if (!PM_irand_timesync(0, 1))
				{
					Anim = BOTH_VS_ATR_S;
				}
				else
				{
					Anim = BOTH_VS_ATL_S;
				}
			}

			if (pm->ps->torsoTimer <= 0)
			{
				//restart the anim if we are already in it (and finished)
				iFlags |= SETANIM_FLAG_RESTART;
			}
			break;

		case WP_BLASTER:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK3)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		case WP_STUN_BATON:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK3)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		case WP_BRYAR_PISTOL:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK3)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		case WP_DISRUPTOR:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK3)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		case WP_BOWCASTER:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK3)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		case WP_REPEATER:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK4)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		case WP_FLECHETTE:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK3)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		case WP_ROCKET_LAUNCHER:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK3)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		case WP_CONCUSSION:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK3)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		case WP_DEMP2:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK3)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		case WP_THERMAL:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK3)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		case WP_TRIP_MINE:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK3)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		case WP_DET_PACK:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK3)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		case WP_BRYAR_OLD:
			// Override the shoot anim.
			if (pm->ps->torsoAnim == BOTH_ATTACK3)
			{
				if (pm->cmd.rightmove > 0) //right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if (pm->cmd.rightmove < 0) //left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else //frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		default:
			Anim = BOTH_VS_IDLE;
			break;
		}
	}
	else if (veh->playerState && veh->playerState->speed < 0 &&
		p_veh->m_pVehicleInfo->type == VH_ANIMAL)
	{
		//tauntaun is going backwards
		Anim = BOTH_VT_WALK_REV;
	}
	else if (veh->playerState && veh->playerState->speed < 0 &&
		p_veh->m_pVehicleInfo->type == VH_SPEEDER)
	{
		//speeder is going backwards
		Anim = BOTH_VS_REV;
	}
	// They're not firing so play the Idle for the weapon.
	else
	{
		iFlags = SETANIM_FLAG_NORMAL;

		switch (pm->ps->weapon)
		{
		case WP_SABER:
			if (BG_SabersOff(pm->ps))
			{
				//saber holstered, normal idle
				Anim = BOTH_VS_IDLE;
			}
			// In the Air.
			//else if ( p_veh->m_ulFlags & VEH_FLYING )
			else
			{
				Anim = BOTH_VS_IDLE_SR;
			}
			break;

		case WP_BLASTER:
			// In the Air.
			//if ( p_veh->m_ulFlags & VEH_FLYING )
		{
			Anim = BOTH_VS_IDLE_G;
		}
		break;

		default:
			Anim = BOTH_VS_IDLE;
			break;
		}
	}

	if (Anim != -1)
	{
		//override it
		if (p_veh->m_pVehicleInfo->type == VH_ANIMAL)
		{
			//agh.. remap anims for the tauntaun
			switch (Anim)
			{
			case BOTH_VS_IDLE:
				if (veh->playerState && veh->playerState->speed > 0)
				{
					if (veh->playerState->speed > p_veh->m_pVehicleInfo->speedMax)
					{
						//turbo
						Anim = BOTH_VT_TURBO;
					}
					else
					{
						Anim = BOTH_VT_RUN_FWD;
					}
				}
				else
				{
					Anim = BOTH_VT_IDLE;
				}
				break;
			case BOTH_VS_ATR_S:
				Anim = BOTH_VT_ATR_S;
				break;
			case BOTH_VS_ATL_S:
				Anim = BOTH_VT_ATL_S;
				break;
			case BOTH_VS_ATR_G:
				Anim = BOTH_VT_ATR_G;
				break;
			case BOTH_VS_ATL_G:
				Anim = BOTH_VT_ATL_G;
				break;
			case BOTH_VS_ATF_G:
				Anim = BOTH_VT_ATF_G;
				break;
			case BOTH_VS_IDLE_SL:
				Anim = BOTH_VT_IDLE_S;
				break;
			case BOTH_VS_IDLE_SR:
				Anim = BOTH_VT_IDLE_S;
				break;
			case BOTH_VS_IDLE_G:
				Anim = BOTH_VT_IDLE_G;
				break;

				//should not happen for tauntaun:
			case BOTH_VS_AIR_G:
			case BOTH_VS_LAND_SL:
			case BOTH_VS_LAND_SR:
			case BOTH_VS_LAND_G:
				return;
			default:
				break;
			}
		}

		PM_SetAnim(SETANIM_BOTH, Anim, iFlags);
	}
}

/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
extern int PM_MeleeMoveForConditions(void);
extern int PM_CheckKick(void);

static void PM_Weapon(void)
{
	int addTime;
	int amount;
	int killAfterItem = 0;
	bgEntity_t* veh = NULL;
	qboolean vehicleRocketLock = qfalse;

	const qboolean is_holding_block_button = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Holding Block Button

#if 0
#ifdef _GAME
	if (pm->ps->clientNum >= MAX_CLIENTS &&
		pm->ps->weapon == WP_NONE &&
		pm->cmd.weapon == WP_NONE &&
		pm_entSelf)
	{ //npc with no weapon
		gentity_t* gent = (gentity_t*)pm_entSelf;
		if (gent->inuse && gent->client &&
			!gent->localAnimIndex)
		{ //humanoid
			pm->ps->torsoAnim = pm->ps->legsAnim;
			pm->ps->torsoTimer = pm->ps->legsTimer;
			return;
		}
	}
#endif
#endif //0

	if (!pm->ps->emplacedIndex &&
		pm->ps->weapon == WP_EMPLACED_GUN)
	{
		//oh no!
		int i = 0;
		int weap = -1;

		while (i < WP_NUM_WEAPONS)
		{
			if (pm->ps->stats[STAT_WEAPONS] & 1 << i && i != WP_NONE)
			{
				//this one's good
				weap = i;
				break;
			}
			i++;
		}

		if (weap != -1)
		{
			pm->cmd.weapon = weap;
			pm->ps->weapon = weap;
			return;
		}
	}

	if (pm_entSelf && pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&& pm->ps->m_iVehicleNum)
	{
		//riding a vehicle
		if (((veh = pm_entVeh)) &&
			(veh->m_pVehicle && (veh->m_pVehicle->m_pVehicleInfo->type == VH_WALKER || veh->m_pVehicle->m_pVehicleInfo->
				type == VH_FIGHTER)))
		{
			//riding a walker/fighter
			//keep saber off, do no weapon stuff at all!
			pm->ps->saberHolstered = 2;
#ifdef _GAME
			pm->cmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK);
#else
			if (g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID].fHoming
				|| g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID].fHoming)
			{
				//our vehicle uses a rocket launcher, so do the normal checks
				vehicleRocketLock = qtrue;
				pm->cmd.buttons &= ~BUTTON_ATTACK;
			}
			else
			{
				pm->cmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK);
			}
#endif
		}
	}

	if (pm->ps->weapon != WP_DISRUPTOR //not using disruptor
		&& pm->ps->weapon != WP_ROCKET_LAUNCHER //not using rocket launcher
		&& pm->ps->weapon != WP_THERMAL //not using thermals
		&& !pm->ps->m_iVehicleNum) //not a vehicle or in a vehicle
	{
		//check for exceeding max charge time if not using disruptor or rocket launcher or thermals
		if (pm->ps->weaponstate == WEAPON_CHARGING_ALT)
		{
			int time_dif = pm->cmd.serverTime - pm->ps->weaponChargeTime;

			if (time_dif > MAX_WEAPON_CHARGE_TIME)
			{
				pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			}
		}

		if (pm->ps->weaponstate == WEAPON_CHARGING)
		{
			int time_dif = pm->cmd.serverTime - pm->ps->weaponChargeTime;

			if (time_dif > MAX_WEAPON_CHARGE_TIME)
			{
				pm->cmd.buttons &= ~BUTTON_ATTACK;
			}
		}
	}
	//we handle the flame thrower item useage seperately to allow it to be held down and used continously (ignores PMF_USE_ITEM_HELD)
	//it's also processed before the handextend stuff so we can continue to use it even when the player's already in the
	//handextend animation for the flamethrower.
	if (pm->cmd.buttons & BUTTON_USE_HOLDABLE && bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag == HI_FLAMETHROWER)
	{
		if (pm_entSelf && pm_entSelf->s.NPC_class != CLASS_VEHICLE && pm->ps->m_iVehicleNum)
		{
			//riding a vehicle, can't use holdable items, this button operates as the weapon link/unlink toggle
		}
		else if (!PM_ItemUsable(pm->ps, 0))
		{
		}
		else
		{
			//use flamethrower
			PM_AddEvent(EV_USE_ITEM0 + bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag);
		}
	}

	if (pm->ps->forceHandExtend == HANDEXTEND_WEAPONREADY && PM_CanSetWeaponAnims())
	{
		//reset into weapon stance
		if (pm->ps->weapon != WP_SABER && pm->ps->weapon != WP_MELEE && !PM_IsRocketTrooper())
		{
			//saber handles its own anims
			if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1)
			{
				PM_StartTorsoAnim(TORSO_RAISEWEAP1);
			}
			else if (pm_entSelf && pm_entSelf->s.botclass == BCLASS_SBD)
			{
				PM_StartTorsoAnim(SBD_WEAPON_OUT_STANDING);
			}
			else if (pm->ps->weapon == WP_BRYAR_OLD)
			{
				PM_StartTorsoAnim(SBD_WEAPON_OUT_STANDING);
			}
			else
			{
				if (pm->ps->weapon == WP_EMPLACED_GUN)
				{
					PM_StartTorsoAnim(BOTH_GUNSIT1);
				}
				else
				{
					PM_StartTorsoAnim(TORSO_RAISEWEAP1);
				}
			}
		}

		//we now go into a weapon raise anim after every force hand extend.
		//this is so that my holster-view-weapon-when-hand-extend stuff works.
		pm->ps->weaponstate = WEAPON_RAISING;
		pm->ps->weaponTime += 250;

		pm->ps->forceHandExtend = HANDEXTEND_NONE;
	}
	else if (pm->ps->forceHandExtend != HANDEXTEND_NONE)
	{
		//nothing else should be allowed to happen during this time, including weapon fire
		int desiredAnim = 0;
		qboolean seperateOnTorso = qfalse;
		qboolean playFullBody = qfalse;
		int desiredOnTorso = 0;

		switch (pm->ps->forceHandExtend)
		{
		case HANDEXTEND_FORCEPUSH:
			if (pm->ps->weapon == WP_MELEE ||
				pm->ps->weapon == WP_NONE ||
				pm->ps->weapon == WP_SABER && pm->ps->saberHolstered)
			{
				//2-handed PUSH
				if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
				{
					desiredAnim = BOTH_SUPERPUSH;

					pm->ps->powerups[PW_INVINCIBLE] = pm->cmd.serverTime + pm->ps->torsoTimer + 2000;
				}
				else
				{
					desiredAnim = BOTH_2HANDPUSH;

					pm->ps->powerups[PW_FORCE_PUSH_RHAND] = pm->cmd.serverTime + pm->ps->torsoTimer + 1000;
				}
			}
			else
			{
				desiredAnim = BOTH_FORCEPUSH;

				pm->ps->powerups[PW_FORCE_PUSH] = pm->cmd.serverTime + pm->ps->torsoTimer + 1000;
			}
			break;
		case HANDEXTEND_FORCEPULL:
			desiredAnim = BOTH_FORCEPULL;
			break;
		case HANDEXTEND_FORCE_HOLD:
			if (pm->ps->fd.forcePowersActive & 1 << FP_GRIP)
			{
				//gripping
				if (pm->ps->weapon == WP_MELEE ||
					pm->ps->weapon == WP_NONE ||
					pm->ps->weapon == WP_SABER && pm->ps->saberHolstered)
				{
					//2-handed
					desiredAnim = BOTH_FORCEGRIP_HOLD;
				}
				else
				{
					desiredAnim = BOTH_FORCEGRIP_OLD;
				}
			}
			else if (pm->ps->fd.forcePowersActive & 1 << FP_LIGHTNING)
			{
				//lightning
				if (pm->ps->weapon == WP_MELEE ||
					pm->ps->weapon == WP_NONE ||
					pm->ps->weapon == WP_SABER && pm->ps->saberHolstered)
				{
					//2-handed lightning
					desiredAnim = BOTH_FORCE_2HANDEDLIGHTNING_HOLD;
				}
				else
				{
					desiredAnim = BOTH_FORCELIGHTNING_HOLD;
				}
			}
			else if (pm->ps->fd.forcePowersActive & 1 << FP_DRAIN)
			{
				//draining
				if (pm->ps->weapon == WP_MELEE ||
					pm->ps->weapon == WP_NONE ||
					pm->ps->weapon == WP_SABER && pm->ps->saberHolstered)
				{
					//2-handed draining
					desiredAnim = BOTH_FORCE_2HANDEDLIGHTNING_NEW;
				}
				else
				{
					desiredAnim = BOTH_FORCE_DRAIN_HOLD;
				}
			}
			else
			{
				//???
				desiredAnim = BOTH_FORCELIGHTNING_HOLD;
			}
			break;
		case HANDEXTEND_SABERPULL:

			switch (pm->ps->fd.saberAnimLevel)
			{
			case SS_FAST:
			case SS_TAVION:
			case SS_MEDIUM:
			case SS_STRONG:
			case SS_DESANN:
			case SS_DUAL:
				desiredAnim = BOTH_SABERPULL;
				break;
			case SS_STAFF:
				desiredAnim = BOTH_SABERPULL_ALT;
				break;
			default:;
			}
			break;
		case HANDEXTEND_CHOKE:
			if (pm->ps->stats[STAT_HEALTH] > 50)
			{
				desiredAnim = BOTH_CHOKE4;
			}
			else if (pm->ps->stats[STAT_HEALTH] > 30)
			{
				desiredAnim = BOTH_CHOKE3;
			}
			else
			{
				desiredAnim = BOTH_CHOKE1;
			}
			break;
		case HANDEXTEND_DODGE:
			desiredAnim = pm->ps->forceDodgeAnim;
			break;
		case HANDEXTEND_KNOCKDOWN:
			if (pm->ps->forceDodgeAnim)
			{
				if (pm->ps->forceDodgeAnim > 4)
				{
					//this means that we want to play a sepereate anim on the torso
					int originalDAnim = pm->ps->forceDodgeAnim - 8; //-8 is the original legs anim
					if (originalDAnim == 2)
					{
						desiredAnim = BOTH_FORCE_GETUP_B1;
					}
					else if (originalDAnim == 3)
					{
						desiredAnim = BOTH_FORCE_GETUP_B3;
					}
					else
					{
						desiredAnim = BOTH_GETUP1;
					}

					//now specify the torso anim
					seperateOnTorso = qtrue;
					desiredOnTorso = BOTH_FORCEPUSH;
				}
				else if (pm->ps->forceDodgeAnim == 2)
				{
					desiredAnim = BOTH_FORCE_GETUP_B1;
				}
				else if (pm->ps->forceDodgeAnim == 3)
				{
					desiredAnim = BOTH_FORCE_GETUP_B3;
				}
				else
				{
					desiredAnim = BOTH_GETUP1;
				}
			}
			else
			{
				//Allow different animation for the headlock knockdown
				if (pm->ps->torsoAnim == BOTH_PLAYER_PA_3_FLY &&
					pm->ps->legsAnim == BOTH_PLAYER_PA_3_FLY)
				{
					desiredAnim = BOTH_PLAYER_PA_3_FLY;
				}
				else
				{
					desiredAnim = BOTH_KNOCKDOWN1;
				}
			}
			break;
		case HANDEXTEND_DUELCHALLENGE:
			desiredAnim = BOTH_ENGAGETAUNT;
			break;
		case HANDEXTEND_TAUNT:
			if (pm->ps->weapon == WP_DISRUPTOR)
			{
				//2-handed PUSH
				desiredAnim = BOTH_TUSKENTAUNT1;
			}
			else
			{
				desiredAnim = BOTH_ENGAGETAUNT;
			}
			break;
		case HANDEXTEND_PRETHROW:
			desiredAnim = BOTH_A3_TL_BR;
			playFullBody = qtrue;
			break;
		case HANDEXTEND_POSTTHROW:
			desiredAnim = BOTH_D3_TL___;
			playFullBody = qtrue;
			break;
		case HANDEXTEND_PRETHROWN:
			desiredAnim = BOTH_KNEES1;
			playFullBody = qtrue;
			break;
		case HANDEXTEND_POSTTHROWN:
			if (pm->ps->forceDodgeAnim)
			{
				desiredAnim = BOTH_FORCE_GETUP_F2;
			}
			else
			{
				desiredAnim = BOTH_KNOCKDOWN5;
			}
			playFullBody = qtrue;
			break;
		case HANDEXTEND_DRAGGING:
			desiredAnim = BOTH_B1_BL___;
			break;
		case HANDEXTEND_JEDITAUNT:
			if (pm->ps->weapon == WP_DISRUPTOR)
			{
				//2-handed PUSH
				desiredAnim = BOTH_TUSKENTAUNT1;
			}
			else
			{
				desiredAnim = BOTH_GESTURE1;
			}
			break;
		case HANDEXTEND_FLAMETHROWER_HOLD:
			desiredAnim = BOTH_FLAMETHROWER;
			break;
		default:
			if (pm->ps->weapon == WP_MELEE ||
				pm->ps->weapon == WP_NONE ||
				pm->ps->weapon == WP_SABER && pm->ps->saberHolstered)
			{
				//2-handed PUSH
				if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
				{
					desiredAnim = BOTH_SUPERPUSH;
				}
				else
				{
					desiredAnim = BOTH_2HANDPUSH;
				}
			}
			else
			{
				desiredAnim = BOTH_FORCEPUSH;
			}
			break;
		}

		if (!seperateOnTorso)
		{
			//of seperateOnTorso, handle it after setting the legs
			PM_SetAnim(SETANIM_TORSO, desiredAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->torsoTimer = 1;
		}

		if (playFullBody)
		{
			//sorry if all these exceptions are getting confusing. This one just means play on both legs and torso.
			PM_SetAnim(SETANIM_BOTH, desiredAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer = pm->ps->torsoTimer = 1;
		}
		else if (pm->ps->forceHandExtend == HANDEXTEND_DODGE || pm->ps->forceHandExtend == HANDEXTEND_KNOCKDOWN ||
			pm->ps->forceHandExtend == HANDEXTEND_CHOKE && pm->ps->groundEntityNum == ENTITYNUM_NONE)
		{
			//special case, play dodge anim on whole body, choke anim too if off ground
			if (seperateOnTorso)
			{
				PM_SetAnim(SETANIM_LEGS, desiredAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->legsTimer = 1;

				PM_SetAnim(SETANIM_TORSO, desiredOnTorso, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer = 1;
			}
			else
			{
				PM_SetAnim(SETANIM_LEGS, desiredAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->legsTimer = 1;
			}
		}
		//reset the saber_move so we don't hang at the end of the handextend if we were in a saber move.
		pm->ps->saber_move = LS_READY;

		if (pm->ps->fd.forcePowersActive & 1 << FP_GRIP)
			PM_WeaponLightsaber();

		return;
	}

	if (PM_InSpecialJump(pm->ps->legsAnim) ||
		BG_InRoll(pm->ps, pm->ps->legsAnim) ||
		PM_InRollComplete(pm->ps, pm->ps->legsAnim))
	{
		if (pm->ps->weaponTime < pm->ps->legsTimer)
		{
			pm->ps->weaponTime = pm->ps->legsTimer;
		}
	}

	if (pm->ps->duelInProgress)
	{
		pm->cmd.weapon = WP_SABER;
		pm->ps->weapon = WP_SABER;

		if (pm->ps->duelTime >= pm->cmd.serverTime)
		{
			pm->cmd.upmove = 0;
			pm->cmd.forwardmove = 0;
			pm->cmd.rightmove = 0;
			pm->cmd.buttons &= ~BUTTON_ATTACK;
			pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			pm->cmd.buttons &= ~BUTTON_GRAPPLE;
			pm->cmd.buttons &= ~BUTTON_KICK;
		}
	}

	if (pm->ps->weapon == WP_SABER && pm->ps->saber_move != LS_READY && pm->ps->saber_move != LS_NONE)
	{
		pm->cmd.weapon = WP_SABER; //don't allow switching out mid-attack
	}

	if (pm->ps->weapon == WP_SABER)
	{
		//rww - we still need the item stuff, so we won't return immediately
		PM_WeaponLightsaber();
		killAfterItem = 1;
	}
	else if (pm->ps->weapon != WP_EMPLACED_GUN)
	{
		pm->ps->saberHolstered = 0;
	}

	if (PM_CanSetWeaponAnims())
	{
		if (pm->ps->weapon == WP_THERMAL ||
			pm->ps->weapon == WP_TRIP_MINE ||
			pm->ps->weapon == WP_DET_PACK)
		{
			if (pm->ps->weapon == WP_THERMAL)
			{
				if (pm->ps->torsoAnim == WeaponAttackAnim[pm->ps->weapon] &&
					pm->ps->weaponTime - 200 <= 0)
				{
					PM_StartTorsoAnim(WeaponReadyAnim[pm->ps->weapon]);
				}
			}
			else
			{
				if (pm->ps->torsoAnim == WeaponAttackAnim[pm->ps->weapon] &&
					pm->ps->weaponTime - 700 <= 0)
				{
					PM_StartTorsoAnim(WeaponReadyAnim[pm->ps->weapon]);
				}
			}
		}
	}

	// don't allow attack until all buttons are up
	if (pm->ps->pm_flags & PMF_RESPAWNED)
	{
		return;
	}

	// ignore if spectator
	if (pm->ps->clientNum < MAX_CLIENTS && pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	// check for dead player
	if (pm->ps->stats[STAT_HEALTH] <= 0)
	{
		pm->ps->weapon = WP_NONE;
		return;
	}

	// check for item using
	if (pm->cmd.buttons & BUTTON_USE_HOLDABLE)
	{
		// fix: rocket lock bug, one of many...
		BG_ClearRocketLock(pm->ps);

		if (!(pm->ps->pm_flags & PMF_USE_ITEM_HELD))
		{
			if (!pm->ps->stats[STAT_HOLDABLE_ITEM])
			{
				return;
			}

			if (bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag == HI_FLAMETHROWER)
			{
				//flame thrower is handled earlier, just terminate out
				return;
			}

			if (!PM_ItemUsable(pm->ps, 0))
			{
				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
				return;
			}
			// These things never get used up or run out
			if (pm->ps->stats[STAT_HOLDABLE_ITEMS] & 1 << bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag)
			{
				if (//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_SEEKER &&
					//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_SHIELD &&
					//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_MEDPAC &&
					//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_MEDPAC_BIG &&
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_BINOCULARS &&
					//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_SENTRY_GUN &&
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_JETPACK &&    //done dont show
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_HEALTHDISP &&
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_AMMODISP &&
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_EWEB &&
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_CLOAK &&
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_FLAMETHROWER &&
					//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_SWOOP &&
					//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_DROIDEKA &&
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_SPHERESHIELD &&     //done dont show
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_GRAPPLE)
				{
					//never use up the binoculars or jetpack or dispensers or cloak or ...
					pm->ps->stats[STAT_HOLDABLE_ITEMS] -= 1 << bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag;
				}
			}
			else
			{
				return; //this should not happen...
			}

			pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
			PM_AddEvent(EV_USE_ITEM0 + bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag);

			if (//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_SEEKER &&
				//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_SHIELD &&
				//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_MEDPAC &&
				//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_MEDPAC_BIG &&
				bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_BINOCULARS &&
				//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_SENTRY_GUN &&
				bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_JETPACK &&    //done dont show
				bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_HEALTHDISP &&
				bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_AMMODISP &&
				bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_EWEB &&
				bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_CLOAK &&
				bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_FLAMETHROWER &&
				//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_SWOOP &&
				//bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_DROIDEKA &&
				bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_SPHERESHIELD &&     //done dont show
				bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_GRAPPLE)
			{
				pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
				BG_CycleInven(pm->ps, 1);
			}
			return;
		}
	}
	else
	{
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	}

	if (killAfterItem)
	{
		return;
	}

	// make weapon function
	if (pm->ps->weaponTime > 0)
	{
		pm->ps->weaponTime -= pml.msec;
	}
	else
	{
		if (pm->ps->ManualBlockingFlags & 1 << MBF_MELEEBLOCK)
		{
			//set up the block position
			PM_SetMeleeBlock();
		}
		else if (pm->cmd.buttons & BUTTON_KICK && pm->ps->communicatingflags & 1 << KICKING)
		{
			//ok, try a kick I guess.//allow them to do the kick now!
			if (!PM_SaberInBounce(pm->ps->saber_move)
				&& !PM_SaberInKnockaway(pm->ps->saber_move)
				&& !PM_SaberInBrokenParry(pm->ps->saber_move)
				&& !PM_kick_move(pm->ps->saber_move)
				&& !PM_KickingAnim(pm->ps->torsoAnim)
				&& !PM_KickingAnim(pm->ps->legsAnim)
				&& !BG_InRoll(pm->ps, pm->ps->legsAnim)
				&& !PM_InKnockDown(pm->ps)) //not already in a kick
			{
				//player kicks
				int kick_move = PM_MeleeMoveForConditions();

				if (kick_move != -1)
				{
					if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
					{
						//if in air, convert kick to an in-air kick
						float gDist = PM_GroundDistance();

						if ((!PM_FlippingAnim(pm->ps->legsAnim) || pm->ps->legsTimer <= 0) &&
							gDist > 64.0f && //strict minimum
							gDist > -pm->ps->velocity[2] - 64.0f)
						{
							switch (kick_move)
							{
							case LS_KICK_F:
								kick_move = LS_KICK_F_AIR;
								break;
							case LS_KICK_F2:
								kick_move = LS_KICK_F_AIR2;
								break;
							case LS_KICK_B:
								kick_move = LS_KICK_B_AIR;
								break;
							case LS_KICK_B2:
								kick_move = LS_KICK_B_AIR;
								break;
							case LS_KICK_B3:
								kick_move = LS_KICK_B_AIR;
								break;
							case LS_SLAP_R:
								kick_move = LS_KICK_R_AIR;
								break;
							case LS_SMACK_R:
								kick_move = LS_KICK_R_AIR;
								break;
							case LS_KICK_R:
								kick_move = LS_KICK_R_AIR;
								break;
							case LS_SLAP_L:
								kick_move = LS_KICK_L_AIR;
								break;
							case LS_SMACK_L:
								kick_move = LS_KICK_L_AIR;
								break;
							case LS_KICK_L:
								kick_move = LS_KICK_L_AIR;
								break;
							default: //oh well, can't do any other kick move while in-air
								kick_move = -1;
								break;
							}
						}
						else
						{
							//off ground, but too close to ground
							kick_move = -1;
						}
					}
				}
				if (kick_move != -1)
				{
					int kickAnim = saber_moveData[kick_move].animToUse;

					if (kickAnim != -1)
					{
						PM_SetAnim(SETANIM_BOTH, kickAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (pm->ps->legsAnim == kickAnim)
						{
							pm->ps->weaponTime = pm->ps->legsTimer;
							return;
						}
					}
				}
			}
			//if got here then no move to do so put torso into leg idle or whatever
			if (pm->ps->torsoAnim != pm->ps->legsAnim)
			{
				PM_SetAnim(SETANIM_BOTH, pm->ps->legsAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			pm->ps->weaponTime = 0;
			return;
		}
	}

	if (pm->ps->isJediMaster && pm->ps->emplacedIndex)
	{
		pm->ps->emplacedIndex = 0;
		pm->ps->saberHolstered = 0;
	}

	if (pm->ps->duelInProgress && pm->ps->emplacedIndex)
	{
		pm->ps->emplacedIndex = 0;
		pm->ps->saberHolstered = 0;
	}

	if (pm->ps->weapon == WP_EMPLACED_GUN && pm->ps->emplacedIndex)
	{
		pm->cmd.weapon = WP_EMPLACED_GUN; //No switch for you!
		PM_StartTorsoAnim(BOTH_GUNSIT1);
	}

	if (pm->ps->isJediMaster || pm->ps->duelInProgress || pm->ps->trueJedi)
	{
		pm->cmd.weapon = WP_SABER;
		pm->ps->weapon = WP_SABER;

		if (pm->ps->isJediMaster || pm->ps->trueJedi)
		{
			pm->ps->stats[STAT_WEAPONS] = 1 << WP_SABER;
		}
	}

	amount = weaponData[pm->ps->weapon].energyPerShot;

	// take an ammo away if not infinite
	if (pm->ps->weapon != WP_NONE && pm->ps->weapon == pm->cmd.weapon &&
		(pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING))
	{
		if (pm->ps->clientNum < MAX_CLIENTS && pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] != -1)
		{
			if (pm->ps->eFlags & EF3_DUAL_WEAPONS)
			{
				if (pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < weaponData[pm->ps->weapon].energyPerShot * 2
					&&
					pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < weaponData[pm->ps->weapon].altEnergyPerShot *
					2)
				{
					//the weapon is out of ammo essentially because it cannot fire primary or secondary, so do the switch
					//regardless of if the player is attacking or not

					if (pm->ps->weapon == WP_BRYAR_PISTOL)
					{
						PM_AddEventWithParm(EV_NOAMMO, WP_NUM_WEAPONS + pm->ps->weapon);

						if (pm->ps->weaponTime < 500)
						{
							pm->ps->weaponTime += 500;
						}
					}
					else
					{
						if (pm->ps->weaponTime < 50)
						{
							pm->ps->weaponTime += 50;
						}
					}
					return;
				}
			}
			else
			{
				// enough energy to fire this weapon?
				if (pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < weaponData[pm->ps->weapon].energyPerShot &&
					pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < weaponData[pm->ps->weapon].altEnergyPerShot)
				{
					//the weapon is out of ammo essentially because it cannot fire primary or secondary, so do the switch
					//regardless of if the player is attacking or not
					PM_AddEventWithParm(EV_NOAMMO, WP_NUM_WEAPONS + pm->ps->weapon);

					if (pm->ps->weaponTime < 500)
					{
						pm->ps->weaponTime += 500;
					}
					return;
				}

				if (pm->ps->weapon == WP_DET_PACK && !pm->ps->hasDetPackPlanted && pm->ps->ammo[weaponData[pm->ps->
					weapon].ammoIndex] < 1)
				{
					PM_AddEventWithParm(EV_NOAMMO, WP_NUM_WEAPONS + pm->ps->weapon);

					if (pm->ps->weaponTime < 500)
					{
						pm->ps->weaponTime += 500;
					}
					return;
				}
			}
		}
	}

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if (pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING && !(pm->ps->ManualBlockingFlags & 1 <<
		HOLDINGBLOCK))
	{
		if (pm->ps->weapon != pm->cmd.weapon)
		{
			PM_BeginWeaponChange(pm->cmd.weapon);
		}
	}

	if (pm->ps->weaponTime > 0 && pm->ps->weapon != WP_FLECHETTE)
	{
		return;
	}
	if (pm->ps->weapon == WP_FLECHETTE)
	{
		if (!(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK)) && pm->ps->weaponTime > 50000)
		{
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
			return;
		}
		if (pm->ps->weaponTime > 0)
		{
			return;
		}
	}

	if (pm->ps->weapon == WP_DISRUPTOR &&
		pm->ps->zoomMode == 1)
	{
		if (pm_cancelOutZoom)
		{
			pm->ps->zoomMode = 0;
			pm->ps->zoomFov = 0;
			pm->ps->zoomLocked = qfalse;
			pm->ps->zoomLockTime = 0;
			PM_AddEvent(EV_DISRUPTOR_ZOOMSOUND);
			return;
		}

		if (pm->cmd.forwardmove ||
			pm->cmd.rightmove ||
			pm->cmd.upmove > 0)
		{
			return;
		}
	}

	// change weapon if time
	if (pm->ps->weaponstate == WEAPON_DROPPING)
	{
		PM_FinishWeaponChange();
		return;
	}

	if (pm->ps->weaponstate == WEAPON_RAISING)
	{
		pm->ps->weaponstate = WEAPON_READY;
		if (PM_CanSetWeaponAnims())
		{
			if (pm->ps->weapon == WP_SABER)
			{
#ifdef _GAME
				if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
				{
					// Some special bot stuff.
					PM_StartTorsoAnim(PM_ReadyPoseForsaber_anim_levelBOT());
				}
				else
#endif
				{
					if (is_holding_block_button && pm->cmd.buttons & BUTTON_WALKING)
					{
						if (pm->ps->fd.saberAnimLevel == SS_DUAL)
						{
							PM_StartTorsoAnim(PM_BlockingPoseForsaber_anim_levelDual());
						}
						else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
						{
							PM_StartTorsoAnim(PM_BlockingPoseForsaber_anim_levelStaff());
						}
						else
						{
							PM_StartTorsoAnim(PM_BlockingPoseForsaber_anim_levelSingle());
						}
					}
					else
					{
						PM_StartTorsoAnim(PM_IdlePoseForsaber_anim_level());
					}
				}
			}
			else if (pm->ps->weapon == WP_MELEE || PM_IsRocketTrooper())
			{
				PM_StartTorsoAnim(pm->ps->legsAnim);
			}
			else
			{
				if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1)
				{
					PM_StartTorsoAnim(TORSO_WEAPONREADY4);
				}
				else if (pm_entSelf && pm_entSelf->s.botclass == BCLASS_SBD)
				{
					PM_StartTorsoAnim(SBD_WEAPON_STANDING);
				}
				else if (pm->ps->weapon == WP_BRYAR_OLD)
				{
					PM_StartTorsoAnim(SBD_WEAPON_STANDING);
				}
				else
				{
					if (pm->ps->weapon == WP_EMPLACED_GUN)
					{
						PM_StartTorsoAnim(BOTH_GUNSIT1);
					}
					else
					{
						if (PM_CanSetWeaponReadyAnim())
						{
							PM_StartTorsoAnim(PM_GetWeaponReadyAnim());
						}
					}
				}
			}
		}
		return;
	}

	if (PM_CanSetWeaponAnims() &&
		!PM_IsRocketTrooper() &&
		pm->ps->weaponstate == WEAPON_READY && pm->ps->weaponTime <= 0 &&
		(pm->ps->weapon >= WP_BRYAR_PISTOL || pm->ps->weapon == WP_STUN_BATON) &&
		pm->ps->torsoTimer <= 0 &&
		pm->ps->torsoAnim != PM_GetWeaponReadyAnim() &&
		pm->ps->torsoAnim != TORSO_WEAPONIDLE3 &&
		pm->ps->weapon != WP_EMPLACED_GUN)
	{
		if (PM_CanSetWeaponReadyAnim())
		{
			PM_StartTorsoAnim(PM_GetWeaponReadyAnim());
		}
	}
	else if (PM_CanSetWeaponAnims() &&
		pm->ps->weapon == WP_MELEE)
	{
		if (pm->ps->weaponTime <= 0 &&
			pm->ps->forceHandExtend == HANDEXTEND_NONE)
		{
			int desTAnim = pm->ps->legsAnim;

			if (desTAnim == BOTH_STAND1 ||
				desTAnim == BOTH_STAND2)
			{
				//remap the standard standing anims for melee stance
				desTAnim = BOTH_STAND6;
			}

			if (!(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK)))
			{
				//don't do this while holding attack
				if (pm->ps->torsoAnim != desTAnim)
				{
					PM_StartTorsoAnim(desTAnim);
				}
			}
		}
	}
	else if (PM_CanSetWeaponAnims() && PM_IsRocketTrooper())
	{
		int desTAnim = pm->ps->legsAnim;

		if (!(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK)))
		{
			//don't do this while holding attack
			if (pm->ps->torsoAnim != desTAnim)
			{
				PM_StartTorsoAnim(desTAnim);
			}
		}
	}

	if ((pm->ps->torsoAnim == TORSO_WEAPONREADY4 ||
		pm->ps->torsoAnim == BOTH_ATTACK4) &&
		(pm->ps->weapon != WP_DISRUPTOR || pm->ps->zoomMode != 1))
	{
		if (pm->ps->weapon == WP_EMPLACED_GUN)
		{
			PM_StartTorsoAnim(BOTH_GUNSIT1);
		}
		else if (PM_CanSetWeaponAnims())
		{
			if (PM_CanSetWeaponReadyAnim())
			{
				PM_StartTorsoAnim(PM_GetWeaponReadyAnim());
			}
		}
	}
	else if (pm->ps->torsoAnim != TORSO_WEAPONREADY4 &&
		pm->ps->torsoAnim != BOTH_ATTACK4 &&
		PM_CanSetWeaponAnims() &&
		(pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1))
	{
		PM_StartTorsoAnim(TORSO_WEAPONREADY4);
	}

	if (pm->ps->clientNum >= MAX_CLIENTS &&
		pm_entSelf &&
		pm_entSelf->s.NPC_class == CLASS_VEHICLE)
	{
		//we are a vehicle
		veh = pm_entSelf;
	}
	if (veh
		&& veh->m_pVehicle)
	{
		if (g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID].fHoming
			|| g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID].fHoming)
		{
			//don't clear the rocket locking ever?
			vehicleRocketLock = qtrue;
		}
	}

	if (!vehicleRocketLock)
	{
		if (pm->ps->weapon != WP_ROCKET_LAUNCHER)
		{
			if (pm_entSelf && pm_entSelf->s.NPC_class != CLASS_VEHICLE && pm->ps->m_iVehicleNum)
			{
				//riding a vehicle, the vehicle will tell me my rocketlock stuff...
			}
			else
			{
				pm->ps->rocketLockIndex = ENTITYNUM_NONE;
				pm->ps->rocketLockTime = 0;
				pm->ps->rocketTargetTime = 0;
			}
		}
	}

	if (PM_DoChargedWeapons(vehicleRocketLock, veh))
	{
		// In some cases the charged weapon code may want us to short circuit the rest of the firing code
		return;
	}

	// check for fire
	if (!(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK)))
	{
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	if (pm->ps->weapon == WP_EMPLACED_GUN)
	{
		addTime = weaponData[pm->ps->weapon].fireTime;
		pm->ps->weaponTime += addTime;
		if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
		{
			PM_AddEvent(EV_ALTFIRE);
		}
		else
		{
			PM_AddEvent(EV_FIRE_WEAPON);
		}
		return;
	}
	if (pm->ps->m_iVehicleNum && pm_entSelf && pm_entSelf->s.NPC_class == CLASS_VEHICLE)
	{
		//a vehicle NPC that has a pilot
		pm->ps->weaponstate = WEAPON_FIRING;
		pm->ps->weaponTime += 100;
#ifdef _GAME //hack, only do it game-side. vehicle weapons don't really need predicting I suppose.
		if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
		{
			G_CheapWeaponFire(pm->ps->clientNum, EV_ALTFIRE);
		}
		else
		{
			G_CheapWeaponFire(pm->ps->clientNum, EV_FIRE_WEAPON);
		}
#endif
		return;
	}

	if (pm->ps->weapon == WP_DISRUPTOR &&
		pm->cmd.buttons & BUTTON_ALT_ATTACK &&
		!pm->ps->zoomLocked)
	{
		return;
	}

	if (pm->ps->weapon == WP_DISRUPTOR &&
		pm->cmd.buttons & BUTTON_ALT_ATTACK &&
		pm->ps->zoomMode == 2)
	{
		//can't use disruptor secondary while zoomed binoculars
		return;
	}
	//special fire animation overrides for NPCs
	if (pm_entSelf && pm_entSelf->s.NPC_class == CLASS_ROCKETTROOPER)
	{
		if (pm->ps->eFlags2 & EF2_FLYING)
		{
			PM_StartTorsoAnim(BOTH_ATTACK2);
		}
		else
		{
			PM_StartTorsoAnim(BOTH_ATTACK1);
		}
	}
	else if (pm_entSelf && pm_entSelf->s.NPC_class == CLASS_ASSASSIN_DROID)
	{
		// Crouched Attack
		if (PM_CrouchAnim(pm->ps->legsAnim))
		{
			PM_StartTorsoAnim(BOTH_ATTACK2);
		}
		// Standing Attack
		//-----------------
		else
		{
			PM_StartTorsoAnim(BOTH_ATTACK3);
		}
	}
	else if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1)
	{
		PM_StartTorsoAnim(BOTH_ATTACK4);
	}
	else if (pm_entSelf && pm_entSelf->s.botclass == BCLASS_SBD)
	{
		PM_StartTorsoAnim(SBD_WEAPON_OUT_STANDING);
	}
	else if (pm->ps->weapon == WP_BRYAR_OLD)
	{
		PM_StartTorsoAnim(SBD_WEAPON_OUT_STANDING);
	}
	else if (pm->ps->weapon == WP_MELEE)
	{
		if (pm->ps->ManualBlockingFlags & 1 << MBF_MELEEBLOCK)
		{
			PM_SetMeleeBlock();
		}
		//special anims for standard melee attacks
		if (!pm->ps->m_iVehicleNum)
		{
			//if riding a vehicle don't do this stuff at all
			if (pm->cmd.buttons & BUTTON_ATTACK && pm->cmd.buttons & BUTTON_ALT_ATTACK)
			{
				//ok, grapple time
#if 0 //eh, I want to try turning the saber off, but can't do that reliably for prediction..
				qboolean icandoit = qtrue;

				if (pm->ps->weaponTime > 0)
				{ //weapon busy
					icandoit = qfalse;
				}
				if (pm->ps->forceHandExtend != HANDEXTEND_NONE)
				{ //force power or knockdown or something
					icandoit = qfalse;
				}
				if (pm->ps->weapon != WP_SABER && pm->ps->weapon != WP_MELEE)
				{
					icandoit = qfalse;
				}

				if (icandoit)
				{
					PM_SetAnim(SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					if (pm->ps->torsoAnim == BOTH_KYLE_GRAB)
					{ //providing the anim set succeeded..
						pm->ps->torsoTimer += 500; //make the hand stick out a little longer than it normally would
						if (pm->ps->legsAnim == pm->ps->torsoAnim)
						{
							pm->ps->legsTimer = pm->ps->torsoTimer;
						}
						pm->ps->weaponTime = pm->ps->torsoTimer;
						return;
					}
				}
#else
#ifdef _GAME
				if (pm_entSelf)
				{
					if (TryGrapple((gentity_t*)pm_entSelf))
					{
						return;
					}
				}
#else
				return;
#endif
#endif
			}
			if (pm->cmd.buttons & BUTTON_ALT_ATTACK && pm->ps->weapon == WP_MELEE)
			{
				//kicks
				if (!PM_KickingAnim(pm->ps->torsoAnim) &&
					!PM_KickingAnim(pm->ps->legsAnim) &&
					!BG_InRoll(pm->ps, pm->ps->legsAnim) &&
					pm->ps->communicatingflags & 1 << KICKING)
				{
					int kick_move = PM_CheckKick();
					if (kick_move == LS_HILT_BASH)
					{
						//yeah.. no hilt to bash with!
						kick_move = LS_KICK_F;
					}

					if (kick_move != -1)
					{
						if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
						{
							//if in air, convert kick to an in-air kick
							float gDist = PM_GroundDistance();
							//let's only allow air kicks if a certain distance from the ground
							//it's silly to be able to do them right as you land.
							//also looks wrong to transition from a non-complete flip anim...
							if ((!PM_FlippingAnim(pm->ps->legsAnim) || pm->ps->legsTimer <= 0) &&
								gDist > 64.0f && //strict minimum
								gDist > -pm->ps->velocity[2] - 64.0f)
								//make sure we are high to ground relative to downward velocity as well
							{
								switch (kick_move)
								{
								case LS_KICK_F:
									kick_move = LS_KICK_F_AIR;
									break;
								case LS_KICK_F2:
									kick_move = LS_KICK_F_AIR2;
									break;
								case LS_KICK_B:
									kick_move = LS_KICK_B_AIR;
									break;
								case LS_KICK_B2:
									kick_move = LS_KICK_B_AIR;
									break;
								case LS_KICK_B3:
									kick_move = LS_KICK_B_AIR;
									break;
								case LS_SLAP_R:
									kick_move = LS_KICK_R_AIR;
									break;
								case LS_SMACK_R:
									kick_move = LS_KICK_R_AIR;
									break;
								case LS_KICK_R:
									kick_move = LS_KICK_R_AIR;
									break;
								case LS_SLAP_L:
									kick_move = LS_KICK_L_AIR;
									break;
								case LS_SMACK_L:
									kick_move = LS_KICK_L_AIR;
									break;
								case LS_KICK_L:
									kick_move = LS_KICK_L_AIR;
									break;
								default: //oh well, can't do any other kick move while in-air
									kick_move = -1;
									break;
								}
							}
							else
							{
								//off ground, but too close to ground
								kick_move = -1;
							}
						}
					}

					if (kick_move != -1)
					{
						int kickAnim = saber_moveData[kick_move].animToUse;

						if (kickAnim != -1)
						{
							PM_SetAnim(SETANIM_BOTH, kickAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
							if (pm->ps->legsAnim == kickAnim)
							{
								pm->ps->weaponTime = pm->ps->legsTimer;
								return;
							}
						}
					}
				}

				//if got here then no move to do so put torso into leg idle or whatever
				if (pm->ps->torsoAnim != pm->ps->legsAnim)
				{
					PM_SetAnim(SETANIM_BOTH, pm->ps->legsAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				pm->ps->weaponTime = 0;
				return;
			}
			if (pm->cmd.buttons & BUTTON_KICK && pm->ps->communicatingflags & 1 << KICKING)
			{
				//kicks
				if (!PM_SaberInBounce(pm->ps->saber_move)
					&& !PM_SaberInKnockaway(pm->ps->saber_move)
					&& !PM_SaberInBrokenParry(pm->ps->saber_move)
					&& !PM_kick_move(pm->ps->saber_move)
					&& !PM_KickingAnim(pm->ps->torsoAnim)
					&& !PM_KickingAnim(pm->ps->legsAnim)
					&& !BG_InRoll(pm->ps, pm->ps->legsAnim)
					&& !PM_InKnockDown(pm->ps)) //not already in a kick
				{
					//player kicks
					int kick_move = PM_MeleeMoveForConditions();

					if (kick_move == LS_HILT_BASH)
					{
						//yeah.. no hilt to bash with!
						kick_move = LS_KICK_F2;
					}

					if (kick_move != -1)
					{
						if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
						{
							//if in air, convert kick to an in-air kick
							float gDist = PM_GroundDistance();

							if ((!PM_FlippingAnim(pm->ps->legsAnim) || pm->ps->legsTimer <= 0) &&
								gDist > 64.0f && //strict minimum
								gDist > -pm->ps->velocity[2] - 64.0f)
								//make sure we are high to ground relative to downward velocity as well
							{
								switch (kick_move)
								{
								case LS_KICK_F:
									kick_move = LS_KICK_F_AIR;
									break;
								case LS_KICK_F2:
									kick_move = LS_KICK_F_AIR2;
									break;
								case LS_KICK_B:
									kick_move = LS_KICK_B_AIR;
									break;
								case LS_KICK_B2:
									kick_move = LS_KICK_B_AIR;
									break;
								case LS_KICK_B3:
									kick_move = LS_KICK_B_AIR;
									break;
								case LS_SLAP_R:
									kick_move = LS_KICK_R_AIR;
									break;
								case LS_SMACK_R:
									kick_move = LS_KICK_R_AIR;
									break;
								case LS_KICK_R:
									kick_move = LS_KICK_R_AIR;
									break;
								case LS_SLAP_L:
									kick_move = LS_KICK_L_AIR;
									break;
								case LS_SMACK_L:
									kick_move = LS_KICK_L_AIR;
									break;
								case LS_KICK_L:
									kick_move = LS_KICK_L_AIR;
									break;
								default: //oh well, can't do any other kick move while in-air
									kick_move = -1;
									break;
								}
							}
							else
							{
								//off ground, but too close to ground
								kick_move = -1;
							}
						}
					}

					if (kick_move != -1)
					{
						int kickAnim = saber_moveData[kick_move].animToUse;

						if (kickAnim != -1)
						{
							PM_SetAnim(SETANIM_BOTH, kickAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
							if (pm->ps->legsAnim == kickAnim)
							{
								pm->ps->weaponTime = pm->ps->legsTimer;
								return;
							}
						}
					}
				}

				//if got here then no move to do so put torso into leg idle or whatever
				if (pm->ps->torsoAnim != pm->ps->legsAnim)
				{
					PM_SetAnim(SETANIM_BOTH, pm->ps->legsAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				pm->ps->weaponTime = 0;
				return;
			}
			//just punch
			int desTAnim = BOTH_MELEE1;

			if (pm->ps->torsoAnim == BOTH_MELEE1)
			{
				desTAnim = BOTH_MELEE2;
			}
			PM_StartTorsoAnim(desTAnim);

			if (pm->ps->torsoAnim == desTAnim)
			{
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
		}
	}
	else if (pm->ps->weapon == WP_BRYAR_PISTOL)
	{
		if (pm->ps->eFlags & EF3_DUAL_WEAPONS)
		{
			PM_StartTorsoAnim(WeaponAttackAnim2[pm->ps->weapon]);
		}
		else
		{
			PM_StartTorsoAnim(WeaponAttackAnim[pm->ps->weapon]);
		}
	}
	else
	{
		if (pm->ps->eFlags & EF3_DUAL_WEAPONS && pm->ps->weapon == WP_BRYAR_PISTOL)
		{
			PM_StartTorsoAnim(WeaponAttackAnim2[pm->ps->weapon]);
		}
		else
		{
			if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
			{
				PM_StartTorsoAnim(WeaponAltAttackAnim[pm->ps->weapon]);
			}
			else
			{
				PM_StartTorsoAnim(WeaponAttackAnim[pm->ps->weapon]);
			}
		}
	}

	if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
	{
		amount = weaponData[pm->ps->weapon].altEnergyPerShot;
	}
	else
	{
		amount = weaponData[pm->ps->weapon].energyPerShot;
	}

	if (pm->ps->eFlags & EF3_DUAL_WEAPONS && pm->ps->weapon == WP_BRYAR_PISTOL)
	{
		amount *= 2;
	}
	pm->ps->weaponstate = WEAPON_FIRING;

	// take an ammo away if not infinite
	if (pm->ps->clientNum < MAX_CLIENTS && pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] != -1)
	{
		// enough energy to fire this weapon?
		if (pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] - amount >= 0)
		{
			pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] -= amount;
		}
		else // Not enough energy
		{
			// Switch weapons
			if (pm->ps->weapon != WP_DET_PACK || !pm->ps->hasDetPackPlanted)
			{
				PM_AddEventWithParm(EV_NOAMMO, WP_NUM_WEAPONS + pm->ps->weapon);
				if (pm->ps->weaponTime < 500)
				{
					pm->ps->weaponTime += 500;
				}
			}
			return;
		}
	}

	if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
	{
		if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode != 1)
		{
			PM_AddEvent(EV_FIRE_WEAPON);
			addTime = weaponData[pm->ps->weapon].fireTime;
		}
		else
		{
			if (pm->ps->weapon != WP_MELEE || !pm->ps->m_iVehicleNum)
			{
				//do not fire melee events at all when on vehicle
				PM_AddEvent(EV_ALTFIRE);
			}
			addTime = weaponData[pm->ps->weapon].altFireTime;
		}
	}
	else
	{
		if (pm->ps->weapon != WP_MELEE || !pm->ps->m_iVehicleNum)
		{
			//do not fire melee events at all when on vehicle
			PM_AddEvent(EV_FIRE_WEAPON);
		}

		addTime = weaponData[pm->ps->weapon].fireTime;

#ifdef _GAME
		if (1)
		{
			gentity_t* ent = &g_entities[pm->ps->clientNum];

			if (ent->client->ps.weapon == WP_BLASTER && ent->client->skillLevel[SK_BLASTERRATEOFFIREUPGRADE] >
				FORCE_LEVEL_0)
			{
				addTime = 350;
			}
		}
#endif

		if (pm->gametype == GT_SIEGE && pm->ps->weapon == WP_DET_PACK)
		{
			// were far too spammy before?  So says Rick.
			addTime *= 2;
		}
	}
	pm->ps->weaponTime += addTime;
	pm->ps->lastShotTime = 3000; //so we know when the last time we fired our gun is
}

/*
================
PM_Animate
================
*/

extern qboolean BG_IsAlreadyinTauntAnim(int anim);
static void PM_BotGesture(void)
{
	if (BG_IsAlreadyinTauntAnim(pm->ps->torsoAnim))
	{
		return;
	}

#ifdef _GAME
	if (!(g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT))
	{
		return;
	}
#endif

	if (pm->ps->stats[STAT_HEALTH] <= 1)
	{
		return;
	}

	if (PM_SaberInAttack(pm->ps->saber_move) || pm->ps->saberLockTime >= pm->cmd.serverTime)
	{
		return;
	}

	if (!pm->ps->m_iVehicleNum)
	{
		if (pm->cmd.buttons & BUTTON_GESTURE)
		{
			PM_AddEvent(EV_TAUNT);

			if (pm->ps->weapon != WP_SABER) //MP
			{
				if (pm_entSelf->s.botclass == BCLASS_VADER || pm_entSelf->s.botclass == BCLASS_DESANN)
				{
					PM_SetAnim(SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					if (pm->ps->weapon == WP_DISRUPTOR)
					{
						PM_SetAnim(SETANIM_TORSO, BOTH_TUSKENTAUNT1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						PM_SetAnim(SETANIM_TORSO, TORSO_HANDSIGNAL4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
			}
			else if (pm->ps->weapon == WP_MELEE) //MP
			{
				if (pm_entSelf->s.botclass == BCLASS_VADER || pm_entSelf->s.botclass == BCLASS_DESANN)
				{
					PM_SetAnim(SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					PM_SetAnim(SETANIM_TORSO, TORSO_HANDSIGNAL4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}
			else
			{
				switch (pm->ps->fd.saberAnimLevel)
				{
				case SS_FAST:
				case SS_TAVION:
				case SS_MEDIUM:
				case SS_STRONG:
				case SS_DESANN:
					PM_SetAnim(SETANIM_TORSO, BOTH_ENGAGETAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_DUAL:
					pm->ps->saberHolstered = 0;
					PM_SetAnim(SETANIM_TORSO, BOTH_DUAL_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_STAFF:
					pm->ps->saberHolstered = 0;
					PM_SetAnim(SETANIM_TORSO, BOTH_STAFF_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				default:;
				}
			}

			pm->ps->forceHandExtendTime = pm->cmd.serverTime + 1000;
		}
		else if (pm->cmd.buttons & BUTTON_RESPECT)
		{
			PM_AddEvent(EV_TAUNT);

			if (pm->ps->weapon != WP_SABER) //MP
			{
				if (pm_entSelf->s.botclass == BCLASS_VADER || pm_entSelf->s.botclass == BCLASS_DESANN)
				{
					PM_SetAnim(SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					if (pm->ps->weapon == WP_DISRUPTOR)
					{
						PM_SetAnim(SETANIM_TORSO, BOTH_TUSKENTAUNT1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						PM_SetAnim(SETANIM_TORSO, BOTH_BOW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
			}
			else if (pm->ps->weapon == WP_MELEE) //MP
			{
				if (pm_entSelf->s.botclass == BCLASS_VADER || pm_entSelf->s.botclass == BCLASS_DESANN)
				{
					PM_SetAnim(SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					PM_SetAnim(SETANIM_TORSO, TORSO_HANDSIGNAL4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}
			else
			{
				pm->ps->saberHolstered = 2;
				if (pm_entSelf->s.botclass == BCLASS_VADER || pm_entSelf->s.botclass == BCLASS_DESANN)
				{
					PM_SetAnim(SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					PM_SetAnim(SETANIM_TORSO, BOTH_BOW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}

			pm->ps->forceHandExtendTime = pm->cmd.serverTime + 1000;
		}
		else if (pm->cmd.buttons & BUTTON_FLOURISH)
		{
			PM_AddEvent(EV_TAUNT);

			if (pm->ps->weapon != WP_SABER) //MP
			{
				if (pm_entSelf->s.botclass == BCLASS_VADER || pm_entSelf->s.botclass == BCLASS_DESANN)
				{
					PM_SetAnim(SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					if (pm->ps->weapon == WP_DISRUPTOR)
					{
						PM_SetAnim(SETANIM_TORSO, BOTH_TUSKENTAUNT1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						PM_SetAnim(SETANIM_TORSO, TORSO_HANDSIGNAL3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
			}
			else if (pm->ps->weapon == WP_MELEE) //MP
			{
				if (pm_entSelf->s.botclass == BCLASS_VADER || pm_entSelf->s.botclass == BCLASS_DESANN)
				{
					PM_SetAnim(SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					PM_SetAnim(SETANIM_TORSO, TORSO_HANDSIGNAL4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}
			else
			{
				switch (pm->ps->fd.saberAnimLevel)
				{
				case SS_FAST:
				case SS_TAVION:
					PM_SetAnim(SETANIM_TORSO, BOTH_VICTORY_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_MEDIUM:
					PM_SetAnim(SETANIM_TORSO, BOTH_VICTORY_MEDIUM, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_STRONG:
				case SS_DESANN:
					pm->ps->saberHolstered = 0;
					PM_SetAnim(SETANIM_TORSO, BOTH_VICTORY_STRONG, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_DUAL:
					pm->ps->saberHolstered = 0;
					PM_SetAnim(SETANIM_TORSO, BOTH_VICTORY_DUAL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_STAFF:
					pm->ps->saberHolstered = 0;
					PM_SetAnim(SETANIM_TORSO, BOTH_VICTORY_STAFF, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				default:;
				}
			}

			pm->ps->forceHandExtendTime = pm->cmd.serverTime + 1000;
		}
		else if (pm->cmd.buttons & BUTTON_GLOAT)
		{
			PM_AddEvent(EV_TAUNT);

			if (pm->ps->weapon != WP_SABER) //MP
			{
				if (pm_entSelf->s.botclass == BCLASS_VADER || pm_entSelf->s.botclass == BCLASS_DESANN)
				{
					PM_SetAnim(SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					if (pm->ps->weapon == WP_DISRUPTOR)
					{
						PM_SetAnim(SETANIM_TORSO, BOTH_TUSKENTAUNT1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						PM_SetAnim(SETANIM_TORSO, TORSO_HANDSIGNAL2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
			}
			else if (pm->ps->weapon == WP_MELEE) //MP
			{
				if (pm_entSelf->s.botclass == BCLASS_VADER || pm_entSelf->s.botclass == BCLASS_DESANN)
				{
					PM_SetAnim(SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					PM_SetAnim(SETANIM_TORSO, TORSO_HANDSIGNAL4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}
			else
			{
				switch (pm->ps->fd.saberAnimLevel)
				{
				case SS_FAST:
				case SS_TAVION:
					PM_SetAnim(SETANIM_TORSO, BOTH_SHOWOFF_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_MEDIUM:
					PM_SetAnim(SETANIM_TORSO, BOTH_SHOWOFF_MEDIUM, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_STRONG:
				case SS_DESANN:
					PM_SetAnim(SETANIM_TORSO, BOTH_SHOWOFF_STRONG, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_DUAL:
					PM_SetAnim(SETANIM_TORSO, BOTH_SHOWOFF_DUAL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_STAFF:
					PM_SetAnim(SETANIM_TORSO, BOTH_SHOWOFF_STAFF, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				default:;
				}
			}

			pm->ps->forceHandExtendTime = pm->cmd.serverTime + 1000;
		}
	}
}

/*
================
PM_DropTimers
================
*/
static void PM_DropTimers(void)
{
	// drop misc timing counter
	if (pm->ps->pm_time)
	{
		if (pml.msec >= pm->ps->pm_time)
		{
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		}
		else
		{
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if (pm->ps->legsTimer > 0)
	{
		pm->ps->legsTimer -= pml.msec;
		if (pm->ps->legsTimer < 0)
		{
			pm->ps->legsTimer = 0;
		}
	}

	if (pm->ps->torsoTimer > 0)
	{
		pm->ps->torsoTimer -= pml.msec;
		if (pm->ps->torsoTimer < 0)
		{
			pm->ps->torsoTimer = 0;
		}
	}
}

// Following function is stateless (at the moment). And hoisting it out
// of the namespace here is easier than fixing all the places it's used,
// which includes files that are also compiled in SP. We do need to make
// sure we only get one copy in the linker, though.

qboolean BG_UnrestrainedPitchRoll(const playerState_t* ps, const Vehicle_t* p_veh)
{
	if (bg_fighterAltControl.integer
		&& ps->clientNum < MAX_CLIENTS //real client
		&& ps->m_iVehicleNum //in a vehicle
		&& p_veh //valid vehicle data pointer
		&& p_veh->m_pVehicleInfo //valid vehicle info
		&& p_veh->m_pVehicleInfo->type == VH_FIGHTER) //fighter
	{
		//can roll and pitch without limitation!
		return qtrue;
	}
	return qfalse;
}

qboolean G_OkayToLean(const playerState_t* ps, const usercmd_t* uscmd, const qboolean interruptOkay)
{
	if (ps->clientNum < MAX_CLIENTS //player
		&& ps->groundEntityNum != ENTITYNUM_NONE //on ground
		&& (interruptOkay //okay to interrupt a lean
			&& !PM_CrouchAnim(ps->legsAnim)
			&& PM_DodgeAnim(ps->torsoAnim)
			|| PM_BlockAnim(ps->torsoAnim) || PM_BlockDualAnim(ps->torsoAnim) || PM_BlockStaffAnim(ps->torsoAnim)
			|| PM_MeleeblockAnim(ps->torsoAnim) //already leaning
			|| !ps->weaponTime //not attacking or being prevented from attacking
			&& !ps->legsTimer //not in any held legs anim
			&& !ps->torsoTimer) //not in any held torso anim
		&& !(uscmd->buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_LIGHTNING | BUTTON_FORCEPOWER |
			BUTTON_DASH | BUTTON_FORCE_DRAIN | BUTTON_FORCEGRIP)) //not trying to attack
		&& !(ps->ManualBlockingFlags & 1 << HOLDINGBLOCK)
		&& VectorCompare(ps->velocity, vec3_origin))
	{
		return qtrue;
	}
	return qfalse;
}

static qboolean G_OkayToDoStandingBlock(const playerState_t* ps, const usercmd_t* uscmd, const qboolean interruptOkay)
{
	if (ps->clientNum < MAX_CLIENTS //player
		&& ps->groundEntityNum != ENTITYNUM_NONE //on ground
		&& (interruptOkay //okay to interrupt a lean
			&& PM_DodgeAnim(ps->torsoAnim)
			|| PM_BlockAnim(ps->torsoAnim) || PM_BlockDualAnim(ps->torsoAnim) || PM_BlockStaffAnim(ps->torsoAnim)
			|| PM_MeleeblockAnim(ps->torsoAnim) //already leaning
			|| !ps->weaponTime //not attacking or being prevented from attacking
			&& !ps->legsTimer //not in any held legs anim
			&& !ps->torsoTimer) //not in any held torso anim
		&& !(uscmd->buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_LIGHTNING | BUTTON_FORCEPOWER |
			BUTTON_DASH | BUTTON_FORCE_DRAIN | BUTTON_FORCEGRIP)) //not trying to attack
		&& VectorCompare(ps->velocity, vec3_origin))
	{
		return qtrue;
	}
	return qfalse;
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated isntead of a full move
================
*/
extern qboolean in_camera;

void PM_UpdateViewAngles(int saberAnimLevel, playerState_t* ps, const usercmd_t* cmd)
{
	short temp;
	int i;
	vec3_t start, end, tmins, tmaxs, right;
	trace_t trace;

	saberInfo_t* saber1 = BG_MySaber(ps->clientNum, 0);

	if (ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION)
	{
		return; // no view changes at all
	}

	if (ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0)
	{
		return; // no view changes at all
	}

	//don't do any updating during cutscenes
	if (in_camera && ps->clientNum < MAX_CLIENTS)
	{
		return;
	}

	if (ps->userInt1)
	{
		short angle;
		//have some sort of lock in place
		if (ps->userInt1 & LOCK_UP)
		{
			temp = cmd->angles[PITCH] + ps->delta_angles[PITCH];
			angle = ANGLE2SHORT(ps->viewangles[PITCH]);

			if (temp < angle)
			{
				//cancel out the cmd angles with the delta_angles if the resulting sum
				//is in the banned direction
				ps->delta_angles[PITCH] = angle - cmd->angles[PITCH];
			}
		}

		if (ps->userInt1 & LOCK_DOWN)
		{
			temp = cmd->angles[PITCH] + ps->delta_angles[PITCH];
			angle = ANGLE2SHORT(ps->viewangles[PITCH]);

			if (temp > angle)
			{
				//cancel out the cmd angles with the delta_angles if the resulting sum
				//is in the banned direction
				ps->delta_angles[PITCH] = angle - cmd->angles[PITCH];
			}
		}

		if (ps->userInt1 & LOCK_RIGHT)
		{
			temp = cmd->angles[YAW] + ps->delta_angles[YAW];
			angle = ANGLE2SHORT(ps->viewangles[YAW]);

			if (temp < angle)
			{
				//cancel out the cmd angles with the delta_angles if the resulting sum
				//is in the banned direction
				ps->delta_angles[YAW] = angle - cmd->angles[YAW];
			}
		}

		if (ps->userInt1 & LOCK_LEFT)
		{
			temp = cmd->angles[YAW] + ps->delta_angles[YAW];
			angle = ANGLE2SHORT(ps->viewangles[YAW]);

			if (temp > angle)
			{
				//cancel out the cmd angles with the delta_angles if the resulting sum
				//is in the banned direction
				ps->delta_angles[YAW] = angle - cmd->angles[YAW];
			}
		}
	}

	// circularly clamp the angles with deltas
	for (i = 0; i < 3; i++)
	{
		temp = cmd->angles[i] + ps->delta_angles[i];
#ifdef VEH_CONTROL_SCHEME_4
		if (pm_entVeh
			&& pm_entVeh->m_pVehicle
			&& pm_entVeh->m_pVehicle->m_pVehicleInfo
			&& pm_entVeh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER
			&& (cmd->serverTime - pm_entVeh->playerState->hyperSpaceTime) >= HYPERSPACE_TIME)
		{//in a vehicle and not hyperspacing
			if (i == PITCH)
			{
				int pitchClamp = ANGLE2SHORT(AngleNormalize180(pm_entVeh->m_pVehicle->m_vPrevRiderViewAngles[PITCH] + 10.0f));
				// don't let the player look up or down more than 22.5 degrees
				if (temp > pitchClamp)
				{
					ps->delta_angles[i] = pitchClamp - cmd->angles[i];
					temp = pitchClamp;
				}
				else if (temp < -pitchClamp)
				{
					ps->delta_angles[i] = -pitchClamp - cmd->angles[i];
					temp = -pitchClamp;
				}
			}
			if (i == YAW)
			{
				int yawClamp = ANGLE2SHORT(AngleNormalize180(pm_entVeh->m_pVehicle->m_vPrevRiderViewAngles[YAW] + 10.0f));
				// don't let the player look left or right more than 22.5 degrees
				if (temp > yawClamp)
				{
					ps->delta_angles[i] = yawClamp - cmd->angles[i];
					temp = yawClamp;
				}
				else if (temp < -yawClamp)
				{
					ps->delta_angles[i] = -yawClamp - cmd->angles[i];
					temp = -yawClamp;
				}
			}
		}
#else //VEH_CONTROL_SCHEME_4
		if (pm_entVeh && BG_UnrestrainedPitchRoll(ps, pm_entVeh->m_pVehicle))
		{
			//in a fighter
			if (i == ROLL)
			{
				//get roll from vehicle
				ps->viewangles[ROLL] = pm_entVeh->playerState->viewangles[ROLL];
				continue;
			}
		}
#endif // VEH_CONTROL_SCHEME_4
		else
		{
			if (i == PITCH)
			{
				// don't let the player look up or down more than 90 degrees
				if (temp > 16000)
				{
					ps->delta_angles[i] = 16000 - cmd->angles[i];
					temp = 16000;
				}
				else if (temp < -16000)
				{
					ps->delta_angles[i] = -16000 - cmd->angles[i];
					temp = -16000;
				}
			}
		}
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}

	//manual dodge
	if (ps->ManualBlockingFlags & 1 << MBF_MELEEDODGE)
	{
		//check leaning
		if (G_OkayToLean(ps, &pm->cmd, qtrue) && (cmd->rightmove || cmd->forwardmove)) //pushing a direction
		{
			int anim = -1;
			if (cmd->rightmove > 0)
			{
				//lean right
				if (cmd->forwardmove > 0)
				{
					//lean forward right
					if (ps->torsoAnim == BOTH_DODGE_HOLD_FR)
					{
						anim = ps->torsoAnim;
					}
					else
					{
						anim = BOTH_DODGE_FR;
					}
				}
				else if (cmd->forwardmove < 0)
				{
					//lean backward right
					if (ps->torsoAnim == BOTH_DODGE_HOLD_BR)
					{
						anim = ps->torsoAnim;
					}
					else
					{
						anim = BOTH_DODGE_BR;
					}
				}
				else
				{
					//lean right
					if (ps->torsoAnim == BOTH_DODGE_HOLD_R)
					{
						anim = ps->torsoAnim;
					}
					else
					{
						anim = BOTH_DODGE_R;
					}
				}
			}
			else if (cmd->rightmove < 0)
			{
				//lean left
				if (cmd->forwardmove > 0)
				{
					//lean forward left
					if (ps->torsoAnim == BOTH_DODGE_HOLD_FL)
					{
						anim = ps->torsoAnim;
					}
					else
					{
						anim = BOTH_DODGE_FL;
					}
				}
				else if (cmd->forwardmove < 0)
				{
					//lean backward left
					if (ps->torsoAnim == BOTH_DODGE_HOLD_BL)
					{
						anim = ps->torsoAnim;
					}
					else
					{
						anim = BOTH_DODGE_BL;
					}
				}
				else
				{
					//lean left
					if (ps->torsoAnim == BOTH_DODGE_HOLD_L)
					{
						anim = ps->torsoAnim;
					}
					else
					{
						anim = BOTH_DODGE_L;
					}
				}
			}
			else
			{
				//not pressing either side
				if (cmd->forwardmove > 0)
				{
					//lean forward
					if (PM_DodgeAnim(ps->torsoAnim))
					{
						anim = ps->torsoAnim;
					}
					else if (Q_irand(0, 1))
					{
						anim = BOTH_DODGE_FL;
					}
					else
					{
						anim = BOTH_DODGE_FR;
					}
				}
				else if (cmd->forwardmove < 0)
				{
					//lean backward
					if (PM_DodgeAnim(ps->torsoAnim))
					{
						anim = ps->torsoAnim;
					}
					else if (Q_irand(0, 1))
					{
						anim = BOTH_DODGE_B;
					}
					else
					{
						anim = BOTH_DODGE_B;
					}
				}
			}
			if (anim != -1)
			{
				int extraHoldTime = 0;
				if (PM_DodgeAnim(ps->torsoAnim) && !PM_DodgeHoldAnim(ps->torsoAnim))
				{
					//already in a dodge
					//use the hold pose, don't start it all over again
					anim = BOTH_DODGE_HOLD_FL + (anim - BOTH_DODGE_FL);
					extraHoldTime = 300;
				}
				if (anim == ps->torsoAnim)
				{
					if (ps->torsoTimer < 200)
					{
						ps->torsoTimer = 200;
					}
				}
				else
				{
					PM_SetAnim(SETANIM_TORSO, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				if (extraHoldTime && ps->torsoTimer < extraHoldTime)
				{
					ps->torsoTimer += extraHoldTime;
				}
				if (ps->groundEntityNum != ENTITYNUM_NONE && !pm->cmd.upmove)
				{
					PM_SetAnim(SETANIM_LEGS, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					ps->legsTimer = ps->torsoTimer;
				}
				else
				{
					PM_SetAnim(SETANIM_LEGS, anim, SETANIM_FLAG_NORMAL);
				}
				ps->weaponTime = ps->torsoTimer;
				ps->leanStopDebounceTime = ceil((float)ps->torsoTimer / 50.0f); //20;
			}
		}
		else if (!ps->zoomMode && cmd->rightmove != 0 && !cmd->forwardmove && cmd->upmove <= 0)
		{
			//Only lean if holding use button, strafing and not moving forward or back and not jumping
			int leanofs = 0;
			vec3_t viewangles;

			if (cmd->rightmove > 0)
			{
				if (ps->leanofs <= 28)
				{
					leanofs = ps->leanofs + 4;
				}
				else
				{
					leanofs = 32;
				}
			}
			else
			{
				if (ps->leanofs >= -28)
				{
					leanofs = ps->leanofs - 4;
				}
				else
				{
					leanofs = -32;
				}
			}

			VectorCopy(ps->origin, start);
			start[2] += ps->viewheight;
			VectorCopy(ps->viewangles, viewangles);
			viewangles[ROLL] = 0;
			AngleVectors(ps->viewangles, NULL, right, NULL);
			VectorNormalize(right);
			right[2] = leanofs < 0 ? 0.25 : -0.25;
			VectorMA(start, leanofs, right, end);
			VectorSet(tmins, -8, -8, -4);
			VectorSet(tmaxs, 8, 8, 4);
			pm->trace(&trace, start, tmins, tmaxs, end, ps->clientNum, MASK_PLAYERSOLID);

			ps->leanofs = floor((float)leanofs * trace.fraction);

			ps->leanStopDebounceTime = 20;
		}
		else
		{
			if (cmd->forwardmove || cmd->upmove > 0)
			{
				if (ps->legsAnim == LEGS_LEAN_RIGHT1 ||
					ps->legsAnim == LEGS_LEAN_LEFT1)
				{
					ps->legsTimer = 0; //Force it to stop the anim
				}

				if (ps->leanofs > 0)
				{
					ps->leanofs -= 4;
					if (ps->leanofs < 0)
					{
						ps->leanofs = 0;
					}
				}
				else if (ps->leanofs < 0)
				{
					ps->leanofs += 4;
					if (ps->leanofs > 0)
					{
						ps->leanofs = 0;
					}
				}
			}
		}
	}
	else //BUTTON_USE
	{
		if (ps->leanofs > 0)
		{
			ps->leanofs -= 4;
			if (ps->leanofs < 0)
			{
				ps->leanofs = 0;
			}
		}
		else if (ps->leanofs < 0)
		{
			ps->leanofs += 4;
			if (ps->leanofs > 0)
			{
				ps->leanofs = 0;
			}
		}
	}

	//standing block
	if (ps->weapon == WP_SABER
		&& !BG_SabersOff(ps)
		&& !ps->saberInFlight
		&& !PM_kick_move(ps->saber_move)
		&& cmd->forwardmove >= 0
		&& !PM_WalkingOrRunningAnim(ps->legsAnim)
		&& !PM_WalkingOrRunningAnim(ps->torsoAnim)
		&& !(ps->pm_flags & PMF_DUCKED)
		&& ps->fd.blockPoints > BLOCKPOINTS_FAIL
		&& ps->fd.forcePower > BLOCKPOINTS_FAIL
		&& ps->fd.forcePowersKnown & 1 << FP_SABER_DEFENSE)
	{
		if (cmd->buttons & BUTTON_BLOCK && !(cmd->buttons & BUTTON_USE) && !(cmd->buttons & BUTTON_WALKING))
		{
			//check leaning
			int anim = -1;

			if (G_OkayToDoStandingBlock(ps, &pm->cmd, qtrue)) //pushing a direction
			{
				//third person lean
				if (cmd->upmove <= 0)
				{
					if (cmd->rightmove > 0)
					{
						//lean right
						if (saberAnimLevel == SS_DUAL)
						{
							if (ps->torsoAnim == BOTH_BLOCK_HOLD_R_DUAL)
							{
								anim = ps->torsoAnim;
							}
							else
							{
								anim = BOTH_BLOCK_R_DUAL;
							}
						}
						else if (saberAnimLevel == SS_STAFF)
						{
							if (saber1 && (saber1->type == SABER_BACKHAND
								|| saber1->type == SABER_ASBACKHAND)) //saber backhand
							{
								if (ps->torsoAnim == BOTH_BLOCK_HOLD_L_STAFF)
								{
									anim = ps->torsoAnim;
								}
								else
								{
									anim = BOTH_BLOCK_L_STAFF;
								}
							}
							else
							{
								if (ps->torsoAnim == BOTH_BLOCK_HOLD_R_STAFF)
								{
									anim = ps->torsoAnim;
								}
								else
								{
									anim = BOTH_BLOCK_R_STAFF;
								}
							}
						}
						else
						{
							if (ps->torsoAnim == BOTH_BLOCK_HOLD_R)
							{
								anim = ps->torsoAnim;
							}
							else
							{
								anim = BOTH_BLOCK_R;
							}
						}
					}
					else if (cmd->rightmove < 0)
					{
						//lean left
						if (saberAnimLevel == SS_DUAL)
						{
							if (ps->torsoAnim == BOTH_BLOCK_HOLD_L_DUAL)
							{
								anim = ps->torsoAnim;
							}
							else
							{
								anim = BOTH_BLOCK_L_DUAL;
							}
						}
						else if (saberAnimLevel == SS_STAFF)
						{
							if (saber1 && (saber1->type == SABER_BACKHAND
								|| saber1->type == SABER_ASBACKHAND)) //saber backhand
							{
								if (ps->torsoAnim == BOTH_BLOCK_HOLD_R_STAFF)
								{
									anim = ps->torsoAnim;
								}
								else
								{
									anim = BOTH_BLOCK_R_STAFF;
								}
							}
							else
							{
								if (ps->torsoAnim == BOTH_BLOCK_HOLD_L_STAFF)
								{
									anim = ps->torsoAnim;
								}
								else
								{
									anim = BOTH_BLOCK_L_STAFF;
								}
							}
						}
						else
						{
							if (ps->torsoAnim == BOTH_BLOCK_HOLD_L)
							{
								anim = pm->ps->torsoAnim;
							}
							else
							{
								anim = BOTH_BLOCK_L;
							}
						}
					}
				}
				else if (!cmd->forwardmove && !cmd->rightmove && cmd->buttons & BUTTON_ATTACK)
				{
					ps->saberBlocked = BLOCKED_TOP;
				}
				if (anim != -1)
				{
					int extraHoldTime = 0;

					if (saberAnimLevel == SS_DUAL)
					{
						if (PM_BlockDualAnim(ps->torsoAnim) && !PM_BlockHoldDualAnim(ps->torsoAnim))
						{
							//already in a dodge
							//use the hold pose, don't start it all over again
							anim = BOTH_BLOCK_HOLD_L_DUAL + (anim - BOTH_BLOCK_L_DUAL);
							extraHoldTime = 100;
						}
					}
					else if (saberAnimLevel == SS_STAFF)
					{
						if (PM_BlockStaffAnim(ps->torsoAnim) && !PM_BlockHoldStaffAnim(ps->torsoAnim))
						{
							//already in a dodge
							//use the hold pose, don't start it all over again
							anim = BOTH_BLOCK_HOLD_L_STAFF + (anim - BOTH_BLOCK_L_STAFF);
							extraHoldTime = 100;
						}
					}
					else
					{
						if (PM_BlockAnim(ps->torsoAnim) && !PM_BlockHoldAnim(ps->torsoAnim))
						{
							//already in a dodge
							//use the hold pose, don't start it all over again
							anim = BOTH_BLOCK_HOLD_L + (anim - BOTH_BLOCK_L);
							extraHoldTime = 100;
						}
					}
					if (anim == ps->torsoAnim)
					{
						if (ps->torsoTimer < 100)
						{
							ps->torsoTimer = 100;
						}
					}
					else
					{
						PM_SetAnim(SETANIM_TORSO, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					if (extraHoldTime && ps->torsoTimer < extraHoldTime)
					{
						ps->torsoTimer += extraHoldTime;
					}
					if (ps->groundEntityNum != ENTITYNUM_NONE && !pm->cmd.upmove)
					{
						PM_SetAnim(SETANIM_LEGS, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						ps->legsTimer = ps->torsoTimer;
					}
					else
					{
						PM_SetAnim(SETANIM_LEGS, anim, SETANIM_FLAG_NORMAL);
					}
					ps->weaponTime = ps->torsoTimer;
					ps->leanStopDebounceTime = ceil((float)ps->torsoTimer / 50.0f); //20;
				}
			}
			else if (!ps->zoomMode && cmd->rightmove != 0 && !cmd->forwardmove && cmd->upmove <= 0)
			{
				//Only lean if holding use button, strafing and not moving forward or back and not jumping
				int leanofs = 0;
				vec3_t viewangles;

				if (cmd->rightmove > 0)
				{
					if (ps->leanofs <= 28)
					{
						leanofs = ps->leanofs + 4;
					}
					else
					{
						leanofs = 32;
					}
				}
				else
				{
					if (ps->leanofs >= -28)
					{
						leanofs = ps->leanofs - 4;
					}
					else
					{
						leanofs = -32;
					}
				}

				VectorCopy(ps->origin, start);
				start[2] += ps->viewheight;
				VectorCopy(ps->viewangles, viewangles);
				viewangles[ROLL] = 0;
				AngleVectors(ps->viewangles, NULL, right, NULL);
				VectorNormalize(right);
				right[2] = leanofs < 0 ? 0.25 : -0.25;
				VectorMA(start, leanofs, right, end);
				VectorSet(tmins, -8, -8, -4);
				VectorSet(tmaxs, 8, 8, 4);
				pm->trace(&trace, start, tmins, tmaxs, end, ps->clientNum, MASK_PLAYERSOLID);

				ps->leanofs = floor((float)leanofs * trace.fraction);

				ps->leanStopDebounceTime = 20;
			}
			else
			{
				if (cmd->forwardmove || cmd->upmove > 0)
				{
					if (ps->legsAnim == LEGS_LEAN_RIGHT1 ||
						ps->legsAnim == LEGS_LEAN_LEFT1)
					{
						ps->legsTimer = 0; //Force it to stop the anim
					}

					if (ps->leanofs > 0)
					{
						ps->leanofs -= 4;
						if (ps->leanofs < 0)
						{
							ps->leanofs = 0;
						}
					}
					else if (ps->leanofs < 0)
					{
						ps->leanofs += 4;
						if (ps->leanofs > 0)
						{
							ps->leanofs = 0;
						}
					}
				}
			}
		}
		else //BUTTON_USE
		{
			if (ps->leanofs > 0)
			{
				ps->leanofs -= 4;
				if (ps->leanofs < 0)
				{
					ps->leanofs = 0;
				}
			}
			else if (ps->leanofs < 0)
			{
				ps->leanofs += 4;
				if (ps->leanofs > 0)
				{
					ps->leanofs = 0;
				}
			}
		}
	}

	if (ps->leanStopDebounceTime)
	{
		ps->leanStopDebounceTime -= 1;
		pm->cmd.rightmove = 0;
		pm->cmd.buttons &= ~BUTTON_USE;
		pm->cmd.buttons &= ~BUTTON_BLOCK;
	}
}

//-------------------------------------------
static void PM_AdjustAttackStates(pmove_t* pmove)
//-------------------------------------------
{
	int amount;

	if (pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&& pmove->ps->m_iVehicleNum)
	{
		//riding a vehicle
		const bgEntity_t* veh = pm_entVeh;
		if (veh &&
			(veh->m_pVehicle && (veh->m_pVehicle->m_pVehicleInfo->type == VH_WALKER || veh->m_pVehicle->m_pVehicleInfo->
				type == VH_FIGHTER)))
		{
			//riding a walker/fighter
			//not firing, ever
			pmove->ps->eFlags &= ~(EF_FIRING | EF_ALT_FIRING);
			return;
		}
	}
	// get ammo usage
	if (pmove->cmd.buttons & BUTTON_ALT_ATTACK)
	{
		amount = pmove->ps->ammo[weaponData[pmove->ps->weapon].ammoIndex] - weaponData[pmove->ps->weapon].
			altEnergyPerShot;
	}
	else
	{
		amount = pmove->ps->ammo[weaponData[pmove->ps->weapon].ammoIndex] - weaponData[pmove->ps->weapon].energyPerShot;
	}

	// disruptor alt-fire should toggle the zoom mode, but only bother doing this for the player?
	if (pmove->ps->weapon == WP_DISRUPTOR && pmove->ps->weaponstate == WEAPON_READY)
	{
		if (!(pmove->ps->eFlags & EF_ALT_FIRING) && pmove->cmd.buttons & BUTTON_ALT_ATTACK /*&&
			pmove->cmd.upmove <= 0 && !pmove->cmd.forwardmove && !pmove->cmd.rightmove*/)
		{
			// We just pressed the alt-fire key
			if (!pmove->ps->zoomMode && pmove->ps->pm_type != PM_DEAD)
			{
				// not already zooming, so do it now
				pmove->ps->zoomMode = 1;
				pmove->ps->zoomLocked = qfalse;
				pmove->ps->zoomFov = 80.0f; //cg_fov.value;
				pmove->ps->zoomLockTime = pmove->cmd.serverTime + 50;
				PM_AddEvent(EV_DISRUPTOR_ZOOMSOUND);
			}
			else if (pmove->ps->zoomMode == 1 && pmove->ps->zoomLockTime < pmove->cmd.serverTime)
			{
				//check for == 1 so we can't turn binoculars off with disruptor alt fire
				// already zooming, so must be wanting to turn it off
				pmove->ps->zoomMode = 0;
				pmove->ps->zoomTime = pmove->ps->commandTime;
				pmove->ps->zoomLocked = qfalse;
				PM_AddEvent(EV_DISRUPTOR_ZOOMSOUND);
				pmove->ps->weaponTime = 1000;
			}
		}
		else if (!(pmove->cmd.buttons & BUTTON_ALT_ATTACK) && pmove->ps->zoomLockTime < pmove->cmd.serverTime)
		{
			// Not pressing zoom any more
			if (pmove->ps->zoomMode)
			{
				if (pmove->ps->zoomMode == 1 && !pmove->ps->zoomLocked)
				{
					//approximate what level the client should be zoomed at based on how long zoom was held
					pmove->ps->zoomFov = (pmove->cmd.serverTime + 50 - pmove->ps->zoomLockTime) * 0.035f;
					if (pmove->ps->zoomFov > 50)
					{
						pmove->ps->zoomFov = 50;
					}
					if (pmove->ps->zoomFov < 1)
					{
						pmove->ps->zoomFov = 1;
					}
				}
				// were zooming in, so now lock the zoom
				pmove->ps->zoomLocked = qtrue;
			}
		}
		//This seemed like a good idea, but apparently it confuses people. So disabled for now.
		/*
		else if (!(pmove->ps->eFlags & EF_ALT_FIRING) && (pmove->cmd.buttons & BUTTON_ALT_ATTACK) &&
			(pmove->cmd.upmove > 0 || pmove->cmd.forwardmove || pmove->cmd.rightmove))
		{ //if you try to zoom while moving, just convert it into a primary attack
			pmove->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			pmove->cmd.buttons |= BUTTON_ATTACK;
		}
		*/

		/*
		if (pmove->cmd.upmove > 0 || pmove->cmd.forwardmove || pmove->cmd.rightmove)
		{
			if (pmove->ps->zoomMode == 1 && pmove->ps->zoomLockTime < pmove->cmd.serverTime)
			{ //check for == 1 so we can't turn binoculars off with disruptor alt fire
				pmove->ps->zoomMode = 0;
				pmove->ps->zoomTime = pmove->ps->commandTime;
				pmove->ps->zoomLocked = qfalse;
				PM_AddEvent(EV_DISRUPTOR_ZOOMSOUND);
			}
		}
		*/

		if (pmove->cmd.buttons & BUTTON_ATTACK)
		{
			// If we are zoomed, we should switch the ammo usage to the alt-fire, otherwise, we'll
			//	just use whatever ammo was selected from above
			if (pmove->ps->zoomMode)
			{
				amount = pmove->ps->ammo[weaponData[pmove->ps->weapon].ammoIndex] -
					weaponData[pmove->ps->weapon].altEnergyPerShot;
			}
		}
		else
		{
			// alt-fire button pressing doesn't use any ammo
			amount = 0;
		}
	}
	/*
	else if (pmove->ps->weapon == WP_DISRUPTOR) //still perform certain checks, even if the weapon is not ready
	{
		if (pmove->cmd.upmove > 0 || pmove->cmd.forwardmove || pmove->cmd.rightmove)
		{
			if (pmove->ps->zoomMode == 1 && pmove->ps->zoomLockTime < pmove->cmd.serverTime)
			{ //check for == 1 so we can't turn binoculars off with disruptor alt fire
				pmove->ps->zoomMode = 0;
				pmove->ps->zoomTime = pmove->ps->commandTime;
				pmove->ps->zoomLocked = qfalse;
				PM_AddEvent(EV_DISRUPTOR_ZOOMSOUND);
			}
		}
	}
	*/

	// set the firing flag for continuous beam weapons, saber will fire even if out of ammo
	if (!(pmove->ps->pm_flags & PMF_RESPAWNED) &&
		pmove->ps->pm_type != PM_INTERMISSION &&
		pmove->ps->pm_type != PM_NOCLIP &&
		pmove->cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK) &&
		(amount >= 0 || pmove->ps->weapon == WP_SABER))
	{
		if (pmove->cmd.buttons & BUTTON_ALT_ATTACK)
		{
			pmove->ps->eFlags |= EF_ALT_FIRING;
		}
		else
		{
			pmove->ps->eFlags &= ~EF_ALT_FIRING;
		}

		// This flag should always get set, even when alt-firing
		pmove->ps->eFlags |= EF_FIRING;
	}
	else
	{
		// Clear 'em out
		pmove->ps->eFlags &= ~(EF_FIRING | EF_ALT_FIRING);
	}

	// disruptor should convert a main fire to an alt-fire if the gun is currently zoomed
	if (pmove->ps->weapon == WP_DISRUPTOR)
	{
		if (pmove->cmd.buttons & BUTTON_ATTACK && pmove->ps->zoomMode == 1 && pmove->ps->zoomLocked)
		{
			// converting the main fire to an alt-fire
			pmove->cmd.buttons |= BUTTON_ALT_ATTACK;
			pmove->ps->eFlags |= EF_ALT_FIRING;
		}
		else if (pmove->cmd.buttons & BUTTON_ALT_ATTACK && pmove->ps->zoomMode == 1 && pmove->ps->zoomLocked)
		{
			pmove->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			pmove->ps->eFlags &= ~EF_ALT_FIRING;
		}
	}
}

static void PM_CmdForRoll(playerState_t* ps, const int anim, usercmd_t* p_Cmd)
{
#ifdef _GAME
	if (ps->userInt3 & 1 << FLAG_DODGEROLL)
	{
		//remove the FLAG_DODGEROLL at the end of the rolls
		const float animationpoint = BG_GetLegsAnimPoint(ps, pm_entSelf->localAnimIndex);

		if (animationpoint <= .1)
		{
			ps->userInt3 &= ~(1 << FLAG_DODGEROLL);
		}
	}
#endif
	switch (anim)
	{
	case BOTH_ROLL_F:
	case BOTH_ROLL_F1:
	case BOTH_ROLL_F2:
		p_Cmd->forwardmove = 127;
		p_Cmd->rightmove = 0;
		break;
	case BOTH_ROLL_B:
		p_Cmd->forwardmove = -127;
		p_Cmd->rightmove = 0;
		break;
	case BOTH_ROLL_R:
		p_Cmd->forwardmove = 0;
		p_Cmd->rightmove = 127;
		break;
	case BOTH_ROLL_L:
		p_Cmd->forwardmove = 0;
		p_Cmd->rightmove = -127;
		break;
	case BOTH_GETUP_BROLL_R:
		p_Cmd->forwardmove = 0;
		p_Cmd->rightmove = 48;
		break;

	case BOTH_GETUP_FROLL_R:
		if (ps->legsTimer <= 250)
		{
			//end of anim
			p_Cmd->forwardmove = p_Cmd->rightmove = 0;
		}
		else
		{
			p_Cmd->forwardmove = 0;
			p_Cmd->rightmove = 64;
		}
		break;

	case BOTH_GETUP_BROLL_L:
		p_Cmd->forwardmove = 0;
		p_Cmd->rightmove = -48;
		break;

	case BOTH_GETUP_FROLL_L:
		if (ps->legsTimer <= 250)
		{
			//end of anim
			p_Cmd->forwardmove = p_Cmd->rightmove = 0;
		}
		else
		{
			p_Cmd->forwardmove = 0;
			p_Cmd->rightmove = -64;
		}
		break;

	case BOTH_GETUP_BROLL_B:
		if (ps->torsoTimer <= 250)
		{
			//end of anim
			p_Cmd->forwardmove = p_Cmd->rightmove = 0;
		}
		else if (PM_AnimLength((animNumber_t)ps->legsAnim) - ps->torsoTimer < 350)
		{
			//beginning of anim
			p_Cmd->forwardmove = p_Cmd->rightmove = 0;
		}
		else
		{
			p_Cmd->forwardmove = -64;
			p_Cmd->rightmove = 0;
		}
		break;

	case BOTH_GETUP_FROLL_B:
		if (ps->torsoTimer <= 100)
		{
			//end of anim
			p_Cmd->forwardmove = p_Cmd->rightmove = 0;
		}
		else if (PM_AnimLength((animNumber_t)ps->legsAnim) - ps->torsoTimer < 200)
		{
			//beginning of anim
			p_Cmd->forwardmove = p_Cmd->rightmove = 0;
		}
		else
		{
			p_Cmd->forwardmove = -64;
			p_Cmd->rightmove = 0;
		}
		break;

	case BOTH_GETUP_BROLL_F:
		if (ps->torsoTimer <= 550)
		{
			//end of anim
			p_Cmd->forwardmove = p_Cmd->rightmove = 0;
		}
		else if (PM_AnimLength((animNumber_t)ps->legsAnim) - ps->torsoTimer < 150)
		{
			//beginning of anim
			p_Cmd->forwardmove = p_Cmd->rightmove = 0;
		}
		else
		{
			p_Cmd->forwardmove = 64;
			p_Cmd->rightmove = 0;
		}
		break;
	case BOTH_GETUP_FROLL_F:
		if (ps->torsoTimer <= 100)
		{
			//end of anim
			p_Cmd->forwardmove = p_Cmd->rightmove = 0;
		}
		else
		{
			p_Cmd->forwardmove = 64;
			p_Cmd->rightmove = 0;
		}
		break;
	case BOTH_LK_DL_ST_T_SB_1_L:
		//kicked backwards
		if (ps->legsTimer < 3050 //at least 10 frames in
			&& ps->legsTimer > 550) //at least 6 frames from end
		{
			//move backwards
			p_Cmd->forwardmove = -64;
			p_Cmd->rightmove = 0;
		}
		else
		{
			p_Cmd->forwardmove = p_Cmd->rightmove = 0;
		}
		break;
	default:;
	}
	p_Cmd->upmove = 0;
}

qboolean PM_SaberInTransition(int move);

static void BG_AdjustClientSpeed(playerState_t* ps, const usercmd_t* cmd, const int svTime)
{
	saberInfo_t* saber;

	if (ps->clientNum >= MAX_CLIENTS)
	{
		const bgEntity_t* bgEnt = pm_entSelf;

		if (bgEnt && bgEnt->s.NPC_class == CLASS_VEHICLE)
		{
			//vehicles manage their own speed
			return;
		}
	}

	//For prediction, always reset speed back to the last known server base speed
	//If we didn't do this, under lag we'd eventually dwindle speed down to 0 even though
	//that would not be the correct predicted value.
	ps->speed = ps->basespeed;

	if (ps->userInt1)
	{
		ps->speed = 0;
	}

	if (ps->forceHandExtend == HANDEXTEND_KNOCKDOWN ||
		ps->forceHandExtend == HANDEXTEND_PRETHROWN ||
		ps->forceHandExtend == HANDEXTEND_POSTTHROWN)
	{
		ps->speed = 0;
	}

	if (cmd->forwardmove < 0 && !(cmd->buttons & BUTTON_WALKING) && pm->ps->groundEntityNum != ENTITYNUM_NONE)
	{
		//running backwards is slower than running forwards (like SP)
		ps->speed *= 0.75f;
	}
	if (cmd->forwardmove < 0 && cmd->buttons & BUTTON_WALKING && pm->ps->groundEntityNum != ENTITYNUM_NONE)
	{
		//walking backwards also makes a player move a little slower
		if (pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCK)
		{
			ps->speed *= 0.75f + 0.2f;
		}
		else
		{
			ps->speed *= 0.70f;
		}
	}

	if (!cmd->forwardmove
		&& cmd->rightmove
		&& !(cmd->buttons & BUTTON_WALKING)
		&& pm->ps->groundEntityNum != ENTITYNUM_NONE)
	{
		//pure strafe running is slower.
		ps->speed *= 0.75f;
	}

	if (ps->fd.forcePowersActive & 1 << FP_GRIP)
	{
		ps->speed *= 0.4f;
	}

	if (ps->fd.forcePowersActive & 1 << FP_SPEED)
	{
		ps->speed *= 1.7f;
	}
	else if (ps->fd.forceRageRecoveryTime > svTime)
	{
		ps->speed *= 0.75f;
	}

	if (pm->ps->weapon == WP_DISRUPTOR &&
		pm->ps->zoomMode == 1 && pm->ps->zoomLockTime < pm->cmd.serverTime)
	{
		ps->speed *= 0.5f;
	}

	if (ps->fd.forceGripCripple && pm->ps->persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{
		if (ps->fd.forcePowersActive & 1 << FP_SPEED)
			ps->speed *= 0.8f;
		else
			ps->speed *= 0.2f;
	}

	if (pm->ps->stats[STAT_HEALTH] <= 25)
	{
		//move slower when low on health
		ps->speed *= 0.85f;
	}
	else if (pm->ps->stats[STAT_HEALTH] <= 40)
	{
		//move slower when low on health
		ps->speed *= 0.90f;
	}
	else if (PM_SaberInAttack(ps->saber_move) && cmd->forwardmove < 0)
	{
		//if running backwards while attacking, don't run as fast.
		switch (ps->fd.saberAnimLevel)
		{
		case SS_TAVION:
		case SS_FAST:
			ps->speed *= 0.85f;
			break;
		case SS_MEDIUM:
		case SS_DUAL:
		case SS_STAFF:
			ps->speed *= 0.70f;
			break;
		case SS_DESANN:
		case SS_STRONG:
			ps->speed *= 0.55f;
			break;
		default:
			break;
		}
	}
	else if (PM_SpinningSaberAnim(ps->legsAnim))
	{
		ps->speed *= 0.5f;
	}
	else if (ps->weapon == WP_SABER && PM_SaberInAttack(ps->saber_move))
	{
		//if attacking with saber while running, drop your speed
		switch (ps->fd.saberAnimLevel)
		{
		case SS_TAVION:
		case SS_FAST:
			ps->speed *= 0.80f;
			break;
		case SS_MEDIUM:
		case SS_DUAL:
		case SS_STAFF:
			ps->speed *= 0.85f;
			break;
		case SS_DESANN:
		case SS_STRONG:
			ps->speed *= 0.75f;
			break;
		default:
			break;
		}
	}
	else if (ps->weapon == WP_SABER && ps->fd.saberAnimLevel == FORCE_LEVEL_3 &&
		PM_SaberInTransition(ps->saber_move))
	{
		//Now, we want to even slow down in transitions for level 3 (since it has chains and stuff)
		if (cmd->forwardmove < 0)
		{
			ps->speed *= 0.4f;
		}
		else
		{
			ps->speed *= 0.6f;
		}
	}

#ifdef _GAME
	if (!(dmflags.integer & DF_JK2ROLL))
#else
	if (!(cgs.dmflags & DF_JK2ROLL))
#endif
	{
		if (BG_InRoll(ps, ps->legsAnim) && ps->speed > 50)
		{
			//can't roll unless you're able to move normally
#ifdef _GAME
			if (ps->legsAnim == BOTH_ROLL_B)
			{
				//backwards roll is pretty fast, should also be slower
				if (m_nerf.integer & 1 << EOC_ROLL)
				{
					if (ps->legsTimer > 800)
					{
						ps->speed = ps->legsTimer / 2.5;
					}
					else
					{
						ps->speed = ps->legsTimer / 6.0;
					}
				}
				else
				{
					ps->speed = ps->legsTimer / 4.5; // slow up the roll?
				}
			}
			else
			{
				if (m_nerf.integer & 1 << EOC_ROLL)
				{
					if (ps->legsTimer > 800)
					{
						ps->speed = ps->legsTimer / 1.5;
					}
					else
					{
						ps->speed = ps->legsTimer / 5.0;
					}
				}
				else
				{
					ps->speed = ps->legsTimer / 2.0; // slow up the roll?
				}
			}
#else
			if (ps->legsAnim == BOTH_ROLL_B)
			{
				//backwards roll is pretty fast, should also be slower
				if (cgs.m_nerf & 1 << EOC_ROLL)
				{
					if (ps->legsTimer > 800)
					{
						ps->speed = ps->legsTimer / 2.5;
					}
					else
					{
						ps->speed = ps->legsTimer / 6.0; //450;
					}
				}
				else
				{
					ps->speed = ps->legsTimer / 4.5; // slow up the roll?
				}
			}
			else
			{
				if (cgs.m_nerf & 1 << EOC_ROLL)
				{
					if (ps->legsTimer > 800)
					{
						ps->speed = ps->legsTimer / 1.5; //450;
					}
					else
					{
						ps->speed = ps->legsTimer / 5.0; //450;
					}
				}
				else
				{
					ps->speed = ps->legsTimer / 2.0; //450; // slow up the roll?
				}
			}
#endif
			if (ps->speed > 600)
			{
				ps->speed = 600;
			}
			//Automatically slow down as the roll ends.
		}
	}

	saber = BG_MySaber(ps->clientNum, 0);

	if (saber && saber->moveSpeedScale != 1.0f)
	{
		ps->speed *= saber->moveSpeedScale;
	}
	saber = BG_MySaber(ps->clientNum, 1);

	if (saber && saber->moveSpeedScale != 1.0f)
	{
		ps->speed *= saber->moveSpeedScale;
	}
}

static qboolean BG_InRollAnim(const entityState_t* cent)
{
	switch (cent->legsAnim)
	{
	case BOTH_ROLL_F:
	case BOTH_ROLL_F1:
	case BOTH_ROLL_F2:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean BG_InKnockDown(const int anim)
{
	switch (anim)
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
	case BOTH_SLAPDOWNRIGHT:
	case BOTH_SLAPDOWNLEFT:
		return qtrue;
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
	default:;
	}
	return qfalse;
}

static qboolean BG_InRollES(entityState_t* ps, const int anim)
{
	switch (anim)
	{
	case BOTH_ROLL_F:
	case BOTH_ROLL_F1:
	case BOTH_ROLL_F2:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		return qtrue;
	default:;
	}
	return qfalse;
}

void BG_IK_MoveArm(void* ghoul2, const int lHandBolt, const int time, const entityState_t* ent, const int basePose,
	vec3_t desiredPos, qboolean* ikInProgress, vec3_t origin, vec3_t angles, vec3_t scale, const int blendTime, const qboolean forceHalt)
{
	mdxaBone_t l_hand_matrix;

	if (!ghoul2)
	{
		return;
	}

	assert(bgHumanoidAnimations[basePose].firstFrame > 0);

	if (!*ikInProgress && !forceHalt)
	{
		const int basepose_anim = basePose;
		sharedSetBoneIKStateParams_t ikP;

		//restrict the shoulder joint
		//VectorSet(ikP.pcjMins,-50.0f,-80.0f,-15.0f);
		//VectorSet(ikP.pcjMaxs,15.0f,40.0f,15.0f);

		//for now, leaving it unrestricted, but restricting elbow joint.
		//This lets us break the arm however we want in order to fling people
		//in throws, and doesn't look bad.
		VectorSet(ikP.pcjMins, 0, 0, 0);
		VectorSet(ikP.pcjMaxs, 0, 0, 0);

		//give the info on our entity.
		ikP.blendTime = blendTime;
		VectorCopy(origin, ikP.origin);
		VectorCopy(angles, ikP.angles);
		ikP.angles[PITCH] = 0;
		ikP.pcjOverrides = 0;
		ikP.radius = 10.0f;
		VectorCopy(scale, ikP.scale);

		//base pose frames for the limb
		ikP.startFrame = bgHumanoidAnimations[basepose_anim].firstFrame + bgHumanoidAnimations[basepose_anim].numFrames;
		ikP.endFrame = bgHumanoidAnimations[basepose_anim].firstFrame + bgHumanoidAnimations[basepose_anim].numFrames;

		ikP.forceAnimOnBone = qfalse; //let it use existing anim if it's the same as this one.

		//we want to call with a null bone name first. This will init all of the
		//ik system stuff on the g2 instance, because we need ragdoll effectors
		//in order for our pcj's to know how to angle properly.
		if (!trap->G2API_SetBoneIKState(ghoul2, time, NULL, IKS_DYNAMIC, &ikP))
		{
			assert(!"Failed to init IK system for g2 instance!");
		}

		//Now, create our IK bone state.
		if (trap->G2API_SetBoneIKState(ghoul2, time, "lhumerus", IKS_DYNAMIC, &ikP))
		{
			//restrict the elbow joint
			VectorSet(ikP.pcjMins, -90.0f, -20.0f, -20.0f);
			VectorSet(ikP.pcjMaxs, 30.0f, 20.0f, -20.0f);

			if (trap->G2API_SetBoneIKState(ghoul2, time, "lradius", IKS_DYNAMIC, &ikP))
			{
				//everything went alright.
				*ikInProgress = qtrue;
			}
		}
	}

	if (*ikInProgress && !forceHalt)
	{
		vec3_t torg;
		vec3_t lHand;
		//actively update our ik state.
		sharedIKMoveParams_t ikM;
		sharedRagDollUpdateParams_t tuParms;
		vec3_t tAngles;

		//set the argument struct up
		VectorCopy(desiredPos, ikM.desiredOrigin); //we want the bone to move here.. if possible

		VectorCopy(angles, tAngles);
		tAngles[PITCH] = tAngles[ROLL] = 0;

		trap->G2API_GetBoltMatrix(ghoul2, 0, lHandBolt, &l_hand_matrix, tAngles, origin, time, 0, scale);

		//Get the point position from the matrix.
		lHand[0] = l_hand_matrix.matrix[0][3];
		lHand[1] = l_hand_matrix.matrix[1][3];
		lHand[2] = l_hand_matrix.matrix[2][3];

		VectorSubtract(lHand, desiredPos, torg);
		const float distToDest = VectorLength(torg);

		//closer we are, more we want to keep updated.
		//if we're far away we don't want to be too fast or we'll start twitching all over.
		if (distToDest < 2)
		{
			//however if we're this close we want very precise movement
			ikM.movementSpeed = 0.4f;
		}
		else if (distToDest < 16)
		{
			ikM.movementSpeed = 0.9f; //8.0f;
		}
		else if (distToDest < 32)
		{
			ikM.movementSpeed = 0.8f; //4.0f;
		}
		else if (distToDest < 64)
		{
			ikM.movementSpeed = 0.7f; //2.0f;
		}
		else
		{
			ikM.movementSpeed = 0.6f;
		}
		VectorCopy(origin, ikM.origin); //our position in the world.

		ikM.boneName[0] = 0;
		if (trap->G2API_IKMove(ghoul2, time, &ikM))
		{
			//now do the standard model animate stuff with ragdoll update params.
			VectorCopy(angles, tuParms.angles);
			tuParms.angles[PITCH] = 0;

			VectorCopy(origin, tuParms.position);
			VectorCopy(scale, tuParms.scale);

			tuParms.me = ent->number;
			VectorClear(tuParms.velocity);

			trap->G2API_AnimateG2Models(ghoul2, time, &tuParms);
		}
		else
		{
			*ikInProgress = qfalse;
		}
	}
	else if (*ikInProgress)
	{
		//kill it
		float cFrame, animSpeed;
		int sFrame, eFrame, flags;

		trap->G2API_SetBoneIKState(ghoul2, time, "lhumerus", IKS_NONE, NULL);
		trap->G2API_SetBoneIKState(ghoul2, time, "lradius", IKS_NONE, NULL);

		//then reset the angles/anims on these PCJs
		trap->G2API_SetBoneAngles(ghoul2, 0, "lhumerus", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
			NEGATIVE_Z, NULL, 0, time);
		trap->G2API_SetBoneAngles(ghoul2, 0, "lradius", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
			NEGATIVE_Z, NULL, 0, time);

		//Get the anim/frames that the pelvis is on exactly, and match the left arm back up with them again.
		trap->G2API_GetBoneAnim(ghoul2, "pelvis", time, &cFrame, &sFrame, &eFrame, &flags, &animSpeed, 0, 0);
		trap->G2API_SetBoneAnim(ghoul2, 0, "lhumerus", sFrame, eFrame, flags, animSpeed, time, sFrame, 300);
		trap->G2API_SetBoneAnim(ghoul2, 0, "lradius", sFrame, eFrame, flags, animSpeed, time, sFrame, 300);

		//And finally, get rid of all the ik state effector data by calling with null bone name (similar to how we init it).
		trap->G2API_SetBoneIKState(ghoul2, time, NULL, IKS_NONE, NULL);

		*ikInProgress = qfalse;
	}
}

//used to make sure NPCs with weird bone structures get they skeletons used correctly
static qboolean BG_ClassHasBadBones(const int NPC_class)
{
	switch (NPC_class)
	{
	case CLASS_WAMPA:
	case CLASS_ROCKETTROOPER:
	case CLASS_SABER_DROID:
	case CLASS_HAZARD_TROOPER:
	case CLASS_ASSASSIN_DROID:
	case CLASS_RANCOR:
		return qtrue;
	default:;
	}
	return qfalse;
}

//used to set the proper orientations for the funky NPC class bone structures.
static void BG_BoneOrientationsForClass(const int NPC_class, const char* boneName, int* oUp, int* oRt, int* oFwd)
{
	//defaults
	*oUp = POSITIVE_X;
	*oRt = NEGATIVE_Y;
	*oFwd = NEGATIVE_Z;
	//switch off class
	switch (NPC_class)
	{
	case CLASS_RANCOR:
		if (Q_stricmp("pelvis", boneName) == 0
			|| Q_stricmp("lower_lumbar", boneName) == 0
			|| Q_stricmp("upper_lumbar", boneName) == 0)
		{
			//only these 3 bones on them are wrong
			*oUp = NEGATIVE_X;
			*oRt = POSITIVE_Y;
			*oFwd = POSITIVE_Z;
		}
		break;
	case CLASS_ROCKETTROOPER:
	case CLASS_HAZARD_TROOPER:
		if (Q_stricmp("pelvis", boneName) == 0)
		{
			//child of root
			//actual, when differences with root are accounted for:
			*oUp = POSITIVE_Z;
			*oRt = NEGATIVE_X;
			*oFwd = NEGATIVE_Y;
		}
		else
		{
			//all the rest are the same, children of root (not pelvis)
			*oUp = NEGATIVE_X;
			*oRt = POSITIVE_Y;
			*oFwd = POSITIVE_Z;
		}
		break;
	case CLASS_SABER_DROID:
		if (Q_stricmp("pelvis", boneName) == 0
			|| Q_stricmp("thoracic", boneName) == 0)
		{
			*oUp = NEGATIVE_X;
			*oRt = NEGATIVE_Z;
			*oFwd = NEGATIVE_Y;
		}
		else
		{
			*oUp = NEGATIVE_X; //POSITIVE_X;
			*oRt = POSITIVE_Y;
			*oFwd = POSITIVE_Z;
		}
		break;
	case CLASS_WAMPA:
		if (Q_stricmp("pelvis", boneName) == 0)
		{
			*oUp = NEGATIVE_X;
			*oRt = POSITIVE_Y;
			*oFwd = NEGATIVE_Z;
		}
		else
		{
			//kinda worked
			*oUp = NEGATIVE_X;
			*oRt = POSITIVE_Y;
			*oFwd = POSITIVE_Z;
		}
		break;
	case CLASS_ASSASSIN_DROID:
		if (Q_stricmp("pelvis", boneName) == 0
			|| Q_stricmp("lower_lumbar", boneName) == 0
			|| Q_stricmp("upper_lumbar", boneName) == 0)
		{
			//only these 3 bones on them are wrong
			*oUp = NEGATIVE_X;
			*oRt = POSITIVE_Y;
			*oFwd = POSITIVE_Z;
		}
		break;
	default:;
	}
}

//Adjust the head/neck desired angles
static void BG_UpdateLookAngles(const int lookingDebounceTime, vec3_t lastHeadAngles, const int time, vec3_t lookAngles,
	const float lookSpeed,
	const float minPitch, const float maxPitch, const float minYaw, const float maxYaw,
	const float minRoll, const float maxRoll)
{
	if (lookingDebounceTime > time)
	{
		static int ang;
		static vec3_t lookAnglesDiff;
		static vec3_t oldLookAngles;
		//clamp so don't get "Exorcist" effect
		if (lookAngles[PITCH] > maxPitch)
		{
			lookAngles[PITCH] = maxPitch;
		}
		else if (lookAngles[PITCH] < minPitch)
		{
			lookAngles[PITCH] = minPitch;
		}
		if (lookAngles[YAW] > maxYaw)
		{
			lookAngles[YAW] = maxYaw;
		}
		else if (lookAngles[YAW] < minYaw)
		{
			lookAngles[YAW] = minYaw;
		}
		if (lookAngles[ROLL] > maxRoll)
		{
			lookAngles[ROLL] = maxRoll;
		}
		else if (lookAngles[ROLL] < minRoll)
		{
			lookAngles[ROLL] = minRoll;
		}

		//slowly lerp to this new value
		//Remember last headAngles
		VectorCopy(lastHeadAngles, oldLookAngles);
		VectorSubtract(lookAngles, oldLookAngles, lookAnglesDiff);

		for (ang = 0; ang < 3; ang++)
		{
			lookAnglesDiff[ang] = AngleNormalize180(lookAnglesDiff[ang]);
		}

		if (VectorLengthSquared(lookAnglesDiff))
		{
			static const float fFrameInter = 0.1f;
			lookAngles[PITCH] = AngleNormalize180(
				oldLookAngles[PITCH] + lookAnglesDiff[PITCH] * fFrameInter * lookSpeed);
			lookAngles[YAW] = AngleNormalize180(oldLookAngles[YAW] + lookAnglesDiff[YAW] * fFrameInter * lookSpeed);
			lookAngles[ROLL] =
				AngleNormalize180(oldLookAngles[ROLL] + lookAnglesDiff[ROLL] * fFrameInter * lookSpeed);
		}
	}
	//Remember current lookAngles next time
	VectorCopy(lookAngles, lastHeadAngles);
}

//for setting visual look (headturn) angles
static void BG_G2ClientNeckAngles(void* ghoul2, const int time, const vec3_t lookAngles, vec3_t headAngles,
	vec3_t neckAngles,
	vec3_t thoracicAngles, vec3_t headClampMinAngles, vec3_t headClampMaxAngles,
	const entityState_t* cent)
{
	vec3_t lA;

	if (cent->NPC_class == CLASS_HAZARD_TROOPER)
	{
		//don't use upper bones
		return;
	}

	VectorCopy(lookAngles, lA);
	//clamp the headangles (which should now be relative to the cervical (neck) angles
	if (lA[PITCH] < headClampMinAngles[PITCH])
	{
		lA[PITCH] = headClampMinAngles[PITCH];
	}
	else if (lA[PITCH] > headClampMaxAngles[PITCH])
	{
		lA[PITCH] = headClampMaxAngles[PITCH];
	}

	if (lA[YAW] < headClampMinAngles[YAW])
	{
		lA[YAW] = headClampMinAngles[YAW];
	}
	else if (lA[YAW] > headClampMaxAngles[YAW])
	{
		lA[YAW] = headClampMaxAngles[YAW];
	}

	if (lA[ROLL] < headClampMinAngles[ROLL])
	{
		lA[ROLL] = headClampMinAngles[ROLL];
	}
	else if (lA[ROLL] > headClampMaxAngles[ROLL])
	{
		lA[ROLL] = headClampMaxAngles[ROLL];
	}

	if (cent->NPC_class == CLASS_ASSASSIN_DROID)
	{
		//each bone has only 1 axis of rotation!
		//thoracic only pitches, split with cervical
		if (thoracicAngles[PITCH])
		{
			//already been set above, blend them
			thoracicAngles[PITCH] = (thoracicAngles[PITCH] + lA[PITCH] * 0.5f) * 0.5f;
		}
		else
		{
			thoracicAngles[PITCH] = lA[PITCH] * 0.5f;
		}
		thoracicAngles[YAW] = thoracicAngles[ROLL] = 0.0f;
		//cervical only pitches, split with thoracis
		neckAngles[PITCH] = lA[PITCH] * 0.5f;
		neckAngles[YAW] = 0.0f;
		neckAngles[ROLL] = 0.0f;
		//cranium only yaws
		headAngles[PITCH] = 0.0f;
		headAngles[YAW] = lA[YAW];
		headAngles[ROLL] = 0.0f;
		//no bones roll
	}
	else if (cent->NPC_class == CLASS_SABER_DROID)
	{
		//each bone has only 1 axis of rotation!
		//no thoracic
		VectorClear(thoracicAngles);
		//cervical only yaws
		neckAngles[PITCH] = 0.0f;
		neckAngles[YAW] = lA[YAW];
		neckAngles[ROLL] = 0.0f;
		//cranium only pitches
		headAngles[PITCH] = lA[PITCH];
		headAngles[YAW] = 0.0f;
		headAngles[ROLL] = 0.0f;
		//none of the bones roll
	}
	else
	{
		//normal humaniod
		if (PM_InLedgeMove(cent->legsAnim))
		{
			//lock arm parent bone to animation
			thoracicAngles[PITCH] = 0;
		}
		else if (thoracicAngles[PITCH])
		{
			//already been set above, blend them
			thoracicAngles[PITCH] = (thoracicAngles[PITCH] + lA[PITCH] * 0.4) * 0.5f;
		}
		else
		{
			thoracicAngles[PITCH] = lA[PITCH] * 0.4;
		}
		if (thoracicAngles[YAW])
		{
			//already been set above, blend them
			thoracicAngles[YAW] = (thoracicAngles[YAW] + lA[YAW] * 0.1) * 0.5f;
		}
		else
		{
			thoracicAngles[YAW] = lA[YAW] * 0.1;
		}
		if (thoracicAngles[ROLL])
		{
			//already been set above, blend them
			thoracicAngles[ROLL] = (thoracicAngles[ROLL] + lA[ROLL] * 0.1) * 0.5f;
		}
		else
		{
			thoracicAngles[ROLL] = lA[ROLL] * 0.1;
		}

		if (PM_InLedgeMove(cent->legsAnim))
		{
			//lock the neckAngles to prevent the head from acting weird
			VectorClear(neckAngles);
			VectorClear(headAngles);
		}
		else
		{
			neckAngles[PITCH] = lA[PITCH] * 0.2f;
			neckAngles[YAW] = lA[YAW] * 0.3f;
			neckAngles[ROLL] = lA[ROLL] * 0.3f;

			headAngles[PITCH] = lA[PITCH] * 0.4;
			headAngles[YAW] = lA[YAW] * 0.6;
			headAngles[ROLL] = lA[ROLL] * 0.6;
		}
	}

	if (BG_ClassHasBadBones(cent->NPC_class))
	{
		int oUp, oRt, oFwd;
		if (cent->NPC_class != CLASS_RANCOR)
		{
			//Rancor doesn't use cranium and cervical
			BG_BoneOrientationsForClass(cent->NPC_class, "cranium", &oUp, &oRt, &oFwd);
			trap->G2API_SetBoneAngles(ghoul2, 0, "cranium", headAngles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, 0, 0,
				time);
			BG_BoneOrientationsForClass(cent->NPC_class, "cervical", &oUp, &oRt, &oFwd);
			trap->G2API_SetBoneAngles(ghoul2, 0, "cervical", neckAngles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, 0, 0,
				time);
		}
		if (cent->NPC_class != CLASS_SABER_DROID)
		{
			//saber droid doesn't use thoracic
			if (cent->NPC_class == CLASS_RANCOR)
			{
				thoracicAngles[YAW] = thoracicAngles[ROLL] = 0.0f;
			}
			BG_BoneOrientationsForClass(cent->NPC_class, "thoracic", &oUp, &oRt, &oFwd);
			trap->G2API_SetBoneAngles(ghoul2, 0, "thoracic", thoracicAngles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, 0, 0,
				time);
		}
	}
	else
	{
		//normal humanoid
		trap->G2API_SetBoneAngles(ghoul2, 0, "cranium", headAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
			NEGATIVE_Z, 0, 0, time);
		trap->G2API_SetBoneAngles(ghoul2, 0, "cervical", neckAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
			NEGATIVE_Z, 0, 0, time);
		trap->G2API_SetBoneAngles(ghoul2, 0, "thoracic", thoracicAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
			NEGATIVE_Z, 0, 0, time);
	}
}

//rww - Finally decided to convert all this stuff to BG form.
static void BG_G2ClientSpineAngles(void* ghoul2, const int motionBolt, vec3_t cent_lerpOrigin, vec3_t cent_lerpAngles,
	entityState_t* cent,
	int time, vec3_t viewAngles, int ciLegs, int ciTorso, const vec3_t angles,
	vec3_t thoracicAngles,
	vec3_t ulAngles, vec3_t llAngles, vec3_t modelScale, float* tPitchAngle,
	float* tYawAngle, int* corrTime, const int NPC_class)
{
	qboolean doCorr = qfalse;

	viewAngles[YAW] = AngleDelta(cent_lerpAngles[YAW], angles[YAW]);

	if (NPC_class == CLASS_SABER_DROID)
	{
		//don't use lower bones
		VectorClear(thoracicAngles);
		VectorClear(ulAngles);
		VectorClear(llAngles);
		return;
	}

#if 1
	if (!PM_FlippingAnim(cent->legsAnim) &&
		!PM_SpinningSaberAnim(cent->legsAnim) &&
		!PM_SpinningSaberAnim(cent->torsoAnim) &&
		!PM_InSpecialJump(cent->legsAnim) &&
		!PM_InSpecialJump(cent->torsoAnim) &&
		!BG_InDeathAnim(cent->legsAnim) &&
		!BG_InDeathAnim(cent->torsoAnim) &&
		!BG_InRollES(cent, cent->legsAnim) &&
		!BG_InRollAnim(cent) &&
		!PM_SaberInSpecial(cent->saber_move) &&
		!pm_saber_in_special_attack(cent->torsoAnim) &&
		!pm_saber_in_special_attack(cent->legsAnim) &&

		!BG_InKnockDown(cent->torsoAnim) &&
		!BG_InKnockDown(cent->legsAnim) &&
		!BG_InKnockDown(ciTorso) &&
		!BG_InKnockDown(ciLegs) &&

		!PM_FlippingAnim(ciLegs) &&
		!PM_SpinningSaberAnim(ciLegs) &&
		!PM_SpinningSaberAnim(ciTorso) &&
		!PM_InSpecialJump(ciLegs) &&
		!PM_InSpecialJump(ciTorso) &&
		!BG_InDeathAnim(ciLegs) &&
		!BG_InDeathAnim(ciTorso) &&
		!pm_saber_in_special_attack(ciTorso) &&
		!pm_saber_in_special_attack(ciLegs) &&

		!(cent->eFlags & EF_DEAD) &&
		cent->legsAnim != cent->torsoAnim &&
		ciLegs != ciTorso &&
		!BG_ClassHasBadBones(NPC_class) &&
		//these guys' bones are so fucked up we shouldn't even bother with this motion bone comp...
		!cent->m_iVehicleNum)
	{
		doCorr = qtrue;
	}
#else
	if (((!PM_FlippingAnim(cent->legsAnim)
		&& !PM_SpinningSaberAnim(cent->legsAnim)
		&& !PM_SpinningSaberAnim(cent->torsoAnim)
		&& (cent->legsAnim) != (cent->torsoAnim)) //NOTE: presumes your legs & torso are on the same frame, though they *should* be because PM_SetAnimFinal tries to keep them in synch
		||
		(!PM_FlippingAnim(ciLegs)
			&& !PM_SpinningSaberAnim(ciLegs)
			&& !PM_SpinningSaberAnim(ciTorso)
			&& (ciLegs) != (ciTorso)))
		||
		ciLegs != cent->legsAnim
		||
		ciTorso != cent->torsoAnim)
	{
		doCorr = qtrue;
		*corrTime = time + 1000; //continue correcting for a second after to smooth things out. SP doesn't need this for whatever reason but I can't find a way around it.
	}
	else if (*corrTime >= time)
	{
		if (!PM_FlippingAnim(cent->legsAnim)
			&& !PM_SpinningSaberAnim(cent->legsAnim)
			&& !PM_SpinningSaberAnim(cent->torsoAnim)
			&& !PM_FlippingAnim(ciLegs)
			&& !PM_SpinningSaberAnim(ciLegs)
			&& !PM_SpinningSaberAnim(ciTorso))
		{
			doCorr = qtrue;
		}
	}
#endif

	if (doCorr)
	{
		//FIXME: no need to do this if legs and torso on are same frame
		//adjust for motion offset
		mdxaBone_t boltMatrix;
		vec3_t motionFwd, motionAngles;
		vec3_t motionRt, tempAng;

		trap->G2API_GetBoltMatrix_NoRecNoRot(ghoul2, 0, motionBolt, &boltMatrix, vec3_origin, cent_lerpOrigin, time, 0,
			modelScale);
		motionFwd[0] = -boltMatrix.matrix[0][1];
		motionFwd[1] = -boltMatrix.matrix[1][1];
		motionFwd[2] = -boltMatrix.matrix[2][1];

		vectoangles(motionFwd, motionAngles);

		motionRt[0] = -boltMatrix.matrix[0][0];
		motionRt[1] = -boltMatrix.matrix[1][0];
		motionRt[2] = -boltMatrix.matrix[2][0];

		vectoangles(motionRt, tempAng);
		motionAngles[ROLL] = -tempAng[PITCH];

		for (int ang = 0; ang < 3; ang++)
		{
			viewAngles[ang] = AngleNormalize180(viewAngles[ang] - AngleNormalize180(motionAngles[ang]));
		}
	}

	//distribute the angles differently up the spine
	//added in different bone handling support for some of NPCs
	if (NPC_class == CLASS_HAZARD_TROOPER)
	{
		//only uses lower_lumbar and upper_lumbar to look around
		VectorClear(thoracicAngles);
		ulAngles[PITCH] = viewAngles[PITCH] * 0.50f;
		llAngles[PITCH] = viewAngles[PITCH] * 0.50f;

		ulAngles[YAW] = viewAngles[YAW] * 0.45f;
		llAngles[YAW] = viewAngles[YAW] * 0.55f;

		ulAngles[ROLL] = viewAngles[ROLL] * 0.45f;
		llAngles[ROLL] = viewAngles[ROLL] * 0.55f;
	}
	else if (NPC_class == CLASS_ASSASSIN_DROID)
	{
		//each bone has only 1 axis of rotation!
		//upper lumbar does not pitch
		thoracicAngles[PITCH] = viewAngles[PITCH] * 0.40f;
		ulAngles[PITCH] = 0.0f;
		llAngles[PITCH] = viewAngles[PITCH] * 0.60f;
		//only upper lumbar yaws
		thoracicAngles[YAW] = 0.0f;
		ulAngles[YAW] = viewAngles[YAW];
		llAngles[YAW] = 0.0f;
		//no bone is capable of rolling
		thoracicAngles[ROLL] = 0.0f;
		ulAngles[ROLL] = 0.0f;
		llAngles[ROLL] = 0.0f;
	}
	else
	{
		//use all 3 bones (normal humanoid models)
		if (PM_InLedgeMove(cent->legsAnim))
		{
			//lock spine to animation
			thoracicAngles[PITCH] = 0;
			llAngles[PITCH] = 0;
			ulAngles[PITCH] = 0;
		}
		else
		{
			thoracicAngles[PITCH] = viewAngles[PITCH] * 0.20f;
			llAngles[PITCH] = viewAngles[PITCH] * 0.40f;
			ulAngles[PITCH] = viewAngles[PITCH] * 0.40f;
		}
		thoracicAngles[YAW] = viewAngles[YAW] * 0.20f;
		ulAngles[YAW] = viewAngles[YAW] * 0.35f;
		llAngles[YAW] = viewAngles[YAW] * 0.45f;

		thoracicAngles[ROLL] = viewAngles[ROLL] * 0.20f;
		ulAngles[ROLL] = viewAngles[ROLL] * 0.35f;
		llAngles[ROLL] = viewAngles[ROLL] * 0.45f;
	}
}

/*
==================
CG_SwingAngles
==================
*/
static float BG_SwingAngles(const float destination, const float swingTolerance, const float clampTolerance,
	const float speed, float* angle, qboolean* swinging, const int frametime)
{
	float swing;
	float move;

	if (!*swinging)
	{
		// see if a swing should be started
		swing = AngleSubtract(*angle, destination);
		if (swing > swingTolerance || swing < -swingTolerance)
		{
			*swinging = qtrue;
		}
	}

	if (!*swinging)
	{
		return 0;
	}

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract(destination, *angle);
	float scale = fabs(swing);
	if (scale < swingTolerance * 0.5)
	{
		scale = 0.5;
	}
	else if (scale < swingTolerance)
	{
		scale = 1.0;
	}
	else
	{
		scale = 2.0;
	}

	// swing towards the destination angle
	if (swing >= 0)
	{
		move = frametime * scale * speed;
		if (move >= swing)
		{
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}
	else if (swing < 0)
	{
		move = frametime * scale * -speed;
		if (move <= swing)
		{
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}

	// clamp to no more than tolerance
	swing = AngleSubtract(destination, *angle);
	if (swing > clampTolerance)
	{
		*angle = AngleMod(destination - (clampTolerance - 1));
	}
	else if (swing < -clampTolerance)
	{
		*angle = AngleMod(destination + (clampTolerance - 1));
	}

	return swing;
}

//I apologize for this function
static qboolean BG_InRoll2(const entityState_t* es)
{
	switch (es->legsAnim)
	{
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
	case BOTH_ROLL_F:
	case BOTH_ROLL_F1:
	case BOTH_ROLL_F2:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		return qtrue;
	default:;
	}
	return qfalse;
}

extern qboolean PM_SaberLockBreakAnim(int anim); //bg_panimate.c
void BG_G2PlayerAngles(void* ghoul2, const int motionBolt, entityState_t* cent, int time, vec3_t cent_lerpOrigin,
	vec3_t cent_lerpAngles, matrix3_t legs, vec3_t legsAngles, qboolean* tYawing,
	qboolean* tPitching, qboolean* lYawing, float* tYawAngle, float* tPitchAngle,
	float* lYawAngle, const int frametime, vec3_t turAngles, vec3_t modelScale, const int ciLegs,
	const int ciTorso, int* corrTime, vec3_t lookAngles, vec3_t lastHeadAngles, const int lookTime,
	const entityState_t* emplaced, int* crazySmoothFactor, int ManualBlockingFlags)
{
	static int dir;
	static int i;
	static float dif;
	static float dest;
	static float speed; //, speed_dif, speed_desired;
	static const float lookSpeed = 1.5f;
#ifdef BONE_BASED_LEG_ANGLES
	static float		legBoneYaw;
#endif
	static vec3_t eyeAngles;
	static vec3_t neckAngles;
	static vec3_t velocity;
	static vec3_t torsoAngles, headAngles;
	static vec3_t velPos, velAng;
	static vec3_t ulAngles, llAngles, viewAngles, angles, thoracicAngles = { 0, 0, 0 };
	static vec3_t headClampMinAngles = { -25, -55, -10 }, headClampMaxAngles = { 50, 50, 10 };

	if (cent->m_iVehicleNum || cent->forceFrame || PM_SaberLockBreakAnim(cent->legsAnim) || PM_SaberLockBreakAnim(
		cent->torsoAnim))
	{
		//a vehicle or riding a vehicle - in either case we don't need to be in here
		vec3_t forcedAngles;

		VectorClear(forcedAngles);
		forcedAngles[YAW] = cent_lerpAngles[YAW];
		forcedAngles[ROLL] = cent_lerpAngles[ROLL];
		AnglesToAxis(forcedAngles, legs);
		VectorCopy(forcedAngles, legsAngles);
		VectorCopy(legsAngles, turAngles);

		if (cent->number < MAX_CLIENTS)
		{
			trap->G2API_SetBoneAngles(ghoul2, 0, "lower_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X,
				NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			trap->G2API_SetBoneAngles(ghoul2, 0, "upper_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X,
				NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			trap->G2API_SetBoneAngles(ghoul2, 0, "cranium", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
				NEGATIVE_Z, 0, 0, time);
			trap->G2API_SetBoneAngles(ghoul2, 0, "thoracic", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
				NEGATIVE_Z, 0, 0, time);
			trap->G2API_SetBoneAngles(ghoul2, 0, "cervical", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
				NEGATIVE_Z, 0, 0, time);
		}
		return;
	}

	if (time + 2000 < *corrTime)
	{
		*corrTime = 0;
	}

	VectorCopy(cent_lerpAngles, headAngles);
	headAngles[YAW] = AngleMod(headAngles[YAW]);
	VectorClear(legsAngles);
	VectorClear(torsoAngles);
	// --------- yaw -------------

	// allow yaw to drift a bit
	if (cent->legsAnim != BOTH_STAND1 ||
		(cent->torsoAnim != WeaponReadyAnim[cent->weapon] && !(cent->eFlags & EF3_DUAL_WEAPONS) ||
			cent->torsoAnim != WeaponReadyAnim2[cent->weapon] && cent->eFlags & EF3_DUAL_WEAPONS))
	{
		// if not standing still, always point all in the same direction
		//cent->pe.torso.yawing = qtrue;	// always center
		*tYawing = qtrue;
		//cent->pe.torso.pitching = qtrue;	// always center
		*tPitching = qtrue;
		//cent->pe.legs.yawing = qtrue;	// always center
		*lYawing = qtrue;
	}

	// adjust legs for movement dir
	if (cent->eFlags & EF_DEAD)
	{
		// don't let dead bodies twitch
		dir = 0;
	}
	else
	{
		dir = cent->angles2[YAW];
		if (dir < 0 || dir > 7)
		{
			Com_Error(ERR_DROP, "Bad player movement angle (%i)", dir);
		}
	}

	torsoAngles[YAW] = headAngles[YAW];

	//for now, turn torso instantly and let the legs swing to follow
	*tYawAngle = torsoAngles[YAW];

	// --------- pitch -------------

	VectorCopy(cent->pos.trDelta, velocity);

	if (BG_InRoll2(cent))
	{
		//don't affect angles based on vel then
		VectorClear(velocity);
	}
	else if (cent->weapon == WP_SABER &&
		PM_SaberInSpecial(cent->saber_move))
	{
		VectorClear(velocity);
	}

	speed = VectorNormalize(velocity);

	if (!speed)
	{
		torsoAngles[YAW] = headAngles[YAW];
	}

	// only show a fraction of the pitch angle in the torso
	if (headAngles[PITCH] > 180)
	{
		dest = (-360 + headAngles[PITCH]) * 0.75;
	}
	else
	{
		dest = headAngles[PITCH] * 0.75;
	}

	if (cent->m_iVehicleNum)
	{
		//swing instantly on vehicles
		*tPitchAngle = dest;
	}
	else
	{
		BG_SwingAngles(dest, 15, 30, 0.1f, tPitchAngle, tPitching, frametime);
	}
	torsoAngles[PITCH] = *tPitchAngle;

	// --------- roll -------------

	if (speed)
	{
		matrix3_t axis;

		speed *= 0.05f;

		AnglesToAxis(legsAngles, axis);
		float side = speed * DotProduct(velocity, axis[1]);
		legsAngles[ROLL] -= side;

		side = speed * DotProduct(velocity, axis[0]);
		legsAngles[PITCH] += side;
	}

	//legsAngles[YAW] = headAngles[YAW] + (movementOffsets[ dir ]*speed_dif);

	//rww - crazy velocity-based leg angle calculation
	legsAngles[YAW] = headAngles[YAW];
	velPos[0] = cent_lerpOrigin[0] + velocity[0];
	velPos[1] = cent_lerpOrigin[1] + velocity[1];
	velPos[2] = cent_lerpOrigin[2]; // + velocity[2];

	if (cent->groundEntityNum == ENTITYNUM_NONE ||
		cent->forceFrame ||
		cent->weapon == WP_EMPLACED_GUN && emplaced)
	{
		//off the ground, no direction-based leg angles (same if in saberlock)
		VectorCopy(cent_lerpOrigin, velPos);
	}

	VectorSubtract(cent_lerpOrigin, velPos, velAng);

	if (!VectorCompare(velAng, vec3_origin))
	{
		float degrees_positive;
		float degrees_negative;
		int adddir;
		vectoangles(velAng, velAng);

		if (velAng[YAW] <= legsAngles[YAW])
		{
			degrees_negative = legsAngles[YAW] - velAng[YAW];
			degrees_positive = 360 - legsAngles[YAW] + velAng[YAW];
		}
		else
		{
			degrees_negative = legsAngles[YAW] + (360 - velAng[YAW]);
			degrees_positive = velAng[YAW] - legsAngles[YAW];
		}

		if (degrees_negative < degrees_positive)
		{
			dif = degrees_negative;
			adddir = 0;
		}
		else
		{
			dif = degrees_positive;
			adddir = 1;
		}

		if (dif > 90)
		{
			dif = 180 - dif;
		}

		if (dif > 60)
		{
			dif = 60;
		}

		//Slight hack for when playing is running backward
		if (dir == 3 || dir == 5)
		{
			dif = -dif;
		}

		if (adddir)
		{
			legsAngles[YAW] -= dif;
		}
		else
		{
			legsAngles[YAW] += dif;
		}
	}

	if (cent->m_iVehicleNum)
	{
		//swing instantly on vehicles
		*lYawAngle = legsAngles[YAW];
	}
	else
	{
		BG_SwingAngles(legsAngles[YAW], /*40*/0, 90, 0.65f, lYawAngle, lYawing, frametime);
	}
	legsAngles[YAW] = *lYawAngle;

	legsAngles[ROLL] = 0;
	torsoAngles[ROLL] = 0;

	// pull the angles back out of the hierarchial chain
	AnglesSubtract(headAngles, torsoAngles, headAngles);
	AnglesSubtract(torsoAngles, legsAngles, torsoAngles);

	legsAngles[PITCH] = 0;

	if (cent->heldByClient)
	{
		//keep the base angles clear when doing the IK stuff, it doesn't compensate for it.
		//rwwFIXMEFIXME: Store leg angles off and add them to all the fed in angles for G2 functions?
		VectorClear(legsAngles);
		legsAngles[YAW] = cent_lerpAngles[YAW];
	}

#ifdef BONE_BASED_LEG_ANGLES
	legBoneYaw = legsAngles[YAW];
	VectorClear(legsAngles);
	legsAngles[YAW] = cent_lerpAngles[YAW];
#endif

	VectorCopy(legsAngles, turAngles);

	AnglesToAxis(legsAngles, legs);

	VectorCopy(cent_lerpAngles, viewAngles);
	viewAngles[YAW] = viewAngles[ROLL] = 0;
	viewAngles[PITCH] *= 0.5;

	VectorSet(angles, 0, legsAngles[1], 0);

	angles[0] = legsAngles[0];
	if (angles[0] > 30)
	{
		angles[0] = 30;
	}
	else if (angles[0] < -30)
	{
		angles[0] = -30;
	}

	if (cent->weapon == WP_EMPLACED_GUN &&
		emplaced)
	{
		//if using an emplaced gun, then we want to make sure we're angled to "hold" it right
		vec3_t facingAngles;

		VectorSubtract(emplaced->pos.trBase, cent_lerpOrigin, facingAngles);
		vectoangles(facingAngles, facingAngles);

		if (emplaced->weapon == WP_NONE)
		{
			//e-web
			VectorCopy(facingAngles, legsAngles);
			AnglesToAxis(legsAngles, legs);
		}
		else
		{
			//misc emplaced
			const float emplacedDif = AngleSubtract(cent_lerpAngles[YAW], facingAngles[YAW]);

			VectorSet(facingAngles, -16.0f, -emplacedDif, 0.0f);

			if (cent->legsAnim == BOTH_STRAFE_LEFT1 || cent->legsAnim == BOTH_STRAFE_RIGHT1)
			{
				//try to adjust so it doesn't look wrong
				if (crazySmoothFactor)
				{
					//want to smooth a lot during this because it chops around and looks like ass
					*crazySmoothFactor = time + 1000;
				}

				BG_G2ClientSpineAngles(ghoul2, motionBolt, cent_lerpOrigin, cent_lerpAngles, cent, time, viewAngles,
					ciLegs, ciTorso, angles, thoracicAngles, ulAngles, llAngles, modelScale,
					tPitchAngle, tYawAngle, corrTime, cent->NPC_class);
				trap->G2API_SetBoneAngles(ghoul2, 0, "lower_lumbar", llAngles, BONE_ANGLES_POSTMULT, POSITIVE_X,
					NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
				trap->G2API_SetBoneAngles(ghoul2, 0, "upper_lumbar", ulAngles, BONE_ANGLES_POSTMULT, POSITIVE_X,
					NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
				trap->G2API_SetBoneAngles(ghoul2, 0, "cranium", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X,
					NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);

				VectorAdd(facingAngles, thoracicAngles, facingAngles);

				if (cent->legsAnim == BOTH_STRAFE_LEFT1)
				{
					//this one needs some further correction
					facingAngles[YAW] -= 32.0f;
				}
			}
			else
			{
				trap->G2API_SetBoneAngles(ghoul2, 0, "cranium", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X,
					NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			}

			VectorScale(facingAngles, 0.6f, facingAngles);
			trap->G2API_SetBoneAngles(ghoul2, 0, "lower_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X,
				NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			VectorScale(facingAngles, 0.8f, facingAngles);
			trap->G2API_SetBoneAngles(ghoul2, 0, "upper_lumbar", facingAngles, BONE_ANGLES_POSTMULT, POSITIVE_X,
				NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			VectorScale(facingAngles, 0.8f, facingAngles);
			trap->G2API_SetBoneAngles(ghoul2, 0, "thoracic", facingAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
				NEGATIVE_Z, 0, 0, time);

			//Now we want the head angled toward where we are facing
			VectorSet(facingAngles, 0.0f, dif, 0.0f);
			VectorScale(facingAngles, 0.6f, facingAngles);
			trap->G2API_SetBoneAngles(ghoul2, 0, "cervical", facingAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
				NEGATIVE_Z, 0, 0, time);

			return; //don't have to bother with the rest then
		}
	}

	BG_G2ClientSpineAngles(ghoul2, motionBolt, cent_lerpOrigin, cent_lerpAngles, cent, time,
		viewAngles, ciLegs, ciTorso, angles, thoracicAngles, ulAngles, llAngles, modelScale,
		tPitchAngle, tYawAngle, corrTime, cent->NPC_class);

	VectorCopy(cent_lerpAngles, eyeAngles);

	for (i = 0; i < 3; i++)
	{
		lookAngles[i] = AngleNormalize180(lookAngles[i]);
		eyeAngles[i] = AngleNormalize180(eyeAngles[i]);
	}
	AnglesSubtract(lookAngles, eyeAngles, lookAngles);

	BG_UpdateLookAngles(lookTime, lastHeadAngles, time, lookAngles, lookSpeed, -50.0f, 50.0f, -70.0f, 70.0f, -30.0f,
		30.0f);

	BG_G2ClientNeckAngles(ghoul2, time, lookAngles, headAngles, neckAngles, thoracicAngles, headClampMinAngles,
		headClampMaxAngles, cent);

#ifdef BONE_BASED_LEG_ANGLES
	{
		vec3_t bLAngles;
		VectorClear(bLAngles);
		bLAngles[ROLL] = AngleNormalize180((legBoneYaw - cent_lerpAngles[YAW]));
		strap_G2API_SetBoneAngles(ghoul2, 0, "model_root", bLAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);

		if (!llAngles[YAW])
		{
			llAngles[YAW] -= bLAngles[ROLL];
		}
	}
#endif

	if (BG_ClassHasBadBones(cent->NPC_class))
	{
		int oUp, oRt, oFwd;
		if (cent->NPC_class == CLASS_RANCOR)
		{
			llAngles[YAW] = llAngles[ROLL] = 0.0f;
			ulAngles[YAW] = ulAngles[ROLL] = 0.0f;
		}
		BG_BoneOrientationsForClass(cent->NPC_class, "upper_lumbar", &oUp, &oRt, &oFwd);
		trap->G2API_SetBoneAngles(ghoul2, 0, "lower_lumbar", ulAngles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, 0, 0,
			time);
		BG_BoneOrientationsForClass(cent->NPC_class, "lower_lumbar", &oUp, &oRt, &oFwd);
		trap->G2API_SetBoneAngles(ghoul2, 0, "upper_lumbar", llAngles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, 0, 0,
			time);
	}
	else
	{
		trap->G2API_SetBoneAngles(ghoul2, 0, "lower_lumbar", llAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
			NEGATIVE_Z, 0, 0, time);
		trap->G2API_SetBoneAngles(ghoul2, 0, "upper_lumbar", ulAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
			NEGATIVE_Z, 0, 0, time);
	}
	//trap->G2API_SetBoneAngles(ghoul2, 0, "thoracic", thoracicAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
}

void BG_G2ATSTAngles(void* ghoul2, const int time, vec3_t cent_lerpAngles)
{
	trap->G2API_SetBoneAngles(ghoul2, 0, "thoracic", cent_lerpAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
		NEGATIVE_Z, 0, 0, time);
}

static qboolean PM_AdjustAnglesForDualJumpAttack(playerState_t* ps, usercmd_t* ucmd)
{
	return qtrue;
}

static QINLINE void PM_CmdForSaberMoves(usercmd_t* ucmd)
{
	//DUAL FORWARD+JUMP+ATTACK
	if (pm->ps->legsAnim == BOTH_JUMPATTACK6 && pm->ps->saber_move == LS_JUMPATTACK_DUAL ||
		// (pm->ps->legsAnim == BOTH_GRIEVOUS_LUNGE   && pm->ps->saber_move == LS_GRIEVOUS_LUNGE) ||
		pm->ps->legsAnim == BOTH_BUTTERFLY_FL1 && pm->ps->saber_move == LS_JUMPATTACK_STAFF_LEFT ||
		pm->ps->legsAnim == BOTH_BUTTERFLY_FR1 && pm->ps->saber_move == LS_JUMPATTACK_STAFF_RIGHT ||
		pm->ps->legsAnim == BOTH_BUTTERFLY_RIGHT && pm->ps->saber_move == LS_BUTTERFLY_RIGHT ||
		pm->ps->legsAnim == BOTH_BUTTERFLY_LEFT && pm->ps->saber_move == LS_BUTTERFLY_LEFT)
	{
		int aLen = PM_AnimLength((animNumber_t)BOTH_JUMPATTACK6);

		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;

		if (pm->ps->legsAnim == BOTH_JUMPATTACK6 /*|| pm->ps->legsAnim == BOTH_GRIEVOUS_LUNGE*/)
		{
			//dual stance attack
			if (pm->ps->legsTimer >= 100 //not at end
				&& aLen - pm->ps->legsTimer >= 250) //not in beginning
			{
				//middle of anim
				//push forward
				ucmd->forwardmove = 127;
			}

			if (pm->ps->legsTimer >= 900 //not at end
				&& aLen - pm->ps->legsTimer >= 950 //not in beginning
				|| pm->ps->legsTimer >= 1600
				&& aLen - pm->ps->legsTimer >= 400) //not in beginning
			{
				//one of the two jumps
				if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
				{
					//still on ground?
					if (pm->ps->groundEntityNum >= MAX_CLIENTS)
					{
						//jump!
						pm->ps->velocity[2] = 250; //400;
						pm->ps->fd.forceJumpZStart = pm->ps->origin[2];
						//so we don't take damage if we land at same height
						//pm->ps->pm_flags |= PMF_JUMPING;
						//FIXME: NPCs yell?
						PM_AddEvent(EV_JUMP);
					}
				}
				else
				{
					//FIXME: if this is the second jump, maybe we should just stop the anim?
				}
			}
		}
		else
		{
			//saberstaff attacks
			float lenMin = 1700.0f;
			float lenMax = 1800.0f;

			aLen = PM_AnimLength((animNumber_t)pm->ps->legsAnim);

			if (pm->ps->legsAnim == BOTH_BUTTERFLY_LEFT)
			{
				lenMin = 1200.0f;
				lenMax = 1400.0f;
			}

			//FIXME: don't slide off people/obstacles?
			if (pm->ps->legsAnim == BOTH_BUTTERFLY_RIGHT
				|| pm->ps->legsAnim == BOTH_BUTTERFLY_LEFT)
			{
				if (pm->ps->legsTimer > 450)
				{
					switch (pm->ps->legsAnim)
					{
					case BOTH_BUTTERFLY_LEFT:
						ucmd->rightmove = -127;
						break;
					case BOTH_BUTTERFLY_RIGHT:
						ucmd->rightmove = 127;
						break;
					default:
						break;
					}
				}
			}
			else
			{
				if (pm->ps->legsTimer >= 100 //not at end
					&& aLen - pm->ps->legsTimer >= 250) //not in beginning
				{
					//middle of anim
					//push forward
					ucmd->forwardmove = 127;
				}
			}

			if (pm->ps->legsTimer >= lenMin && pm->ps->legsTimer < lenMax)
			{
				//one of the two jumps
				if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
				{
					//still on ground?
					//jump!
					if (pm->ps->legsAnim == BOTH_BUTTERFLY_LEFT)
					{
						pm->ps->velocity[2] = 350;
					}
					else
					{
						pm->ps->velocity[2] = 250;
					}
					pm->ps->fd.forceJumpZStart = pm->ps->origin[2]; //so we don't take damage if we land at same height
					PM_AddEvent(EV_JUMP);
				}
				else
				{
					//FIXME: if this is the second jump, maybe we should just stop the anim?
				}
			}
		}

		if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
		{
			//can only turn when your feet hit the ground
			if (PM_AdjustAnglesForDualJumpAttack(pm->ps, ucmd))
			{
				PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, ucmd);
			}
		}
	}
	//STAFF BACK+JUMP+ATTACK
	else if (pm->ps->saber_move == LS_A_BACKFLIP_ATK &&
		pm->ps->legsAnim == BOTH_JUMPATTACK7)
	{
		const int aLen = PM_AnimLength((animNumber_t)BOTH_JUMPATTACK7);

		if (pm->ps->legsTimer > 800 //not at end
			&& aLen - pm->ps->legsTimer >= 400) //not in beginning
		{
			//middle of anim
			if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
			{
				//still on ground?
				vec3_t yawAngles, backDir;

				//push backwards some?
				VectorSet(yawAngles, 0, pm->ps->viewangles[YAW] + 180, 0);
				AngleVectors(yawAngles, backDir, 0, 0);
				VectorScale(backDir, 100, pm->ps->velocity);

				//jump!
				pm->ps->velocity[2] = 300;
				pm->ps->fd.forceJumpZStart = pm->ps->origin[2]; //so we don't take damage if we land at same height

				//FIXME: NPCs yell?
				PM_AddEvent(EV_JUMP);
				ucmd->upmove = 0; //clear any actual jump command
			}
		}
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
	}
	else if (PM_SaberInBrokenParry(pm->ps->saber_move))
	{
		//you can't move while stunned.
		switch (pm->ps->torsoAnim)
		{
		case BOTH_H1_S1_T_:
		case BOTH_H1_S1_TR:
		case BOTH_H1_S1_TL:
		case BOTH_H1_S1_BL:
		case BOTH_H1_S1_B_:
		case BOTH_H1_S1_BR:
			//slight backwards stumble
			if (bg_get_torso_anim_point(pm->ps, pm_entSelf->localAnimIndex) >= .5f)
			{
				//past the stumble part of the animation
				ucmd->forwardmove = -46;
			}
			else
			{
				ucmd->forwardmove = 0;
			}
			break;

		case BOTH_H6_S6_BL:
			//slight back hop
			ucmd->forwardmove = -30;
			break;

		case BOTH_H7_S7_T_:
		case BOTH_H7_S7_TR:
			//two small steps back
			ucmd->forwardmove = -30;
			break;

		default: //don't know this one.
			ucmd->forwardmove = 0;
			break;
		}

		ucmd->rightmove = 0;
		ucmd->upmove = 0;
	}
	else if (pm->ps->torsoAnim == BOTH_HOP_F)
	{
		ucmd->forwardmove = 127;
		ucmd->rightmove = 0;
	}
	else if (pm->ps->torsoAnim == BOTH_HOP_R)
	{
		ucmd->forwardmove = 0;
		ucmd->rightmove = 75;
	}
	else if (pm->ps->torsoAnim == BOTH_HOP_L)
	{
		ucmd->forwardmove = 0;
		ucmd->rightmove = -75;
	}
	else if (pm->ps->torsoAnim == BOTH_HOP_B)
	{
		ucmd->forwardmove = -127;
		ucmd->rightmove = 0;
	}
}

//constrain him based on the angles of his vehicle and the caps
static void PM_VehicleViewAngles(playerState_t* ps, const bgEntity_t* veh, const usercmd_t* ucmd)
{
	const Vehicle_t* p_veh = veh->m_pVehicle;
	qboolean setAngles = qfalse;
	vec3_t clampMin;
	vec3_t clampMax;
	int i;

	if (veh->m_pVehicle->m_pPilot
		&& veh->m_pVehicle->m_pPilot->s.number == ps->clientNum)
	{
		//set the pilot's viewangles to the vehicle's viewangles
#ifdef VEH_CONTROL_SCHEME_4
		if (1)
#else //VEH_CONTROL_SCHEME_4
		if (!BG_UnrestrainedPitchRoll(ps, veh->m_pVehicle))
#endif //VEH_CONTROL_SCHEME_4
		{
			//only if not if doing special free-roll/pitch control
			setAngles = qtrue;
			clampMin[PITCH] = -p_veh->m_pVehicleInfo->lookPitch;
			clampMax[PITCH] = p_veh->m_pVehicleInfo->lookPitch;
			clampMin[YAW] = clampMax[YAW] = 0;
			clampMin[ROLL] = clampMax[ROLL] = -1;
		}
	}
	else
	{
		//NOTE: passengers can look around freely, UNLESS they're controlling a turret!
		for (i = 0; i < MAX_VEHICLE_TURRETS; i++)
		{
			if (veh->m_pVehicle->m_pVehicleInfo->turret[i].passengerNum == ps->generic1)
			{
				//this turret is my station
				return;
			}
		}
	}
	if (setAngles)
	{
		for (i = 0; i < 3; i++)
		{
			//clamp viewangles
			if (clampMin[i] == -1 || clampMax[i] == -1)
			{
				//no clamp
			}
			else if (!clampMin[i] && !clampMax[i])
			{
				//no allowance
				//ps->viewangles[i] = veh->playerState->viewangles[i];
			}
			else
			{
				//allowance
				if (ps->viewangles[i] > clampMax[i])
				{
					ps->viewangles[i] = clampMax[i];
				}
				else if (ps->viewangles[i] < clampMin[i])
				{
					ps->viewangles[i] = clampMin[i];
				}
			}
		}

		PM_SetPMViewAngle(ps, ps->viewangles, ucmd);
	}
}

//see if a weapon is ok to use on a vehicle
static qboolean PM_WeaponOkOnVehicle(const int weapon)
{
	switch (weapon)
	{
	case WP_NONE:
	case WP_MELEE:
	case WP_SABER:
	case WP_BLASTER:
	case WP_STUN_BATON:
	case WP_BRYAR_PISTOL:
	case WP_DISRUPTOR:
	case WP_BOWCASTER:
	case WP_REPEATER:
	case WP_DEMP2:
	case WP_FLECHETTE:
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
	case WP_TRIP_MINE:
	case WP_DET_PACK:
	case WP_CONCUSSION:
	case WP_BRYAR_OLD:
		return qtrue;
	default:;
	}
	return qfalse;
}

extern void PM_CheckClearSaberBlock(void);

static void PM_CheckInVehicleSaberAttackAnim(void)
{
	//A bit of a hack, but makes the vehicle saber attacks act like any other saber attack...
	// make weapon function
	if (pm->ps->weaponTime > 0)
	{
		pm->ps->weaponTime -= pml.msec;
		if (pm->ps->weaponTime <= 0)
		{
			pm->ps->weaponTime = 0;
		}
	}

	PM_CheckClearSaberBlock();

	saber_moveName_t saber_move = LS_INVALID;
	switch (pm->ps->torsoAnim)
	{
	case BOTH_VS_ATR_S:
		saber_move = LS_SWOOP_ATTACK_RIGHT;
		break;
	case BOTH_VS_ATL_S:
		saber_move = LS_SWOOP_ATTACK_LEFT;
		break;
	case BOTH_VT_ATR_S:
		saber_move = LS_TAUNTAUN_ATTACK_RIGHT;
		break;
	case BOTH_VT_ATL_S:
		saber_move = LS_TAUNTAUN_ATTACK_LEFT;
		break;
	default:;
	}
	if (saber_move != LS_INVALID)
	{
		if (pm->ps->saber_move == saber_move)
		{
			//already playing it
			if (!pm->ps->torsoTimer)
			{
				//anim was done, set it back to ready
				PM_SetSaberMove(LS_READY);
				pm->ps->saber_move = LS_READY;
				pm->ps->weaponstate = WEAPON_IDLE;
				if (pm->cmd.buttons & BUTTON_ATTACK)
				{
					if (!pm->ps->weaponTime) //not firing
					{
						PM_SetSaberMove(saber_move);
						pm->ps->weaponstate = WEAPON_FIRING;
						pm->ps->weaponTime = pm->ps->torsoTimer;
					}
				}
			}
		}
		else if (pm->ps->torsoTimer
			&& !pm->ps->weaponTime)
		{
			PM_SetSaberMove(LS_READY);
			pm->ps->saber_move = LS_READY;
			pm->ps->weaponstate = WEAPON_IDLE;
			PM_SetSaberMove(saber_move);
			pm->ps->weaponstate = WEAPON_FIRING;
			pm->ps->weaponTime = pm->ps->torsoTimer;
		}
	}
	pm->ps->saberBlocking = saber_moveData[pm->ps->saber_move].blocking;
}

//do we have a weapon that's ok for using on the vehicle?
static int PM_GetOkWeaponForVehicle(void)
{
	int i = 0;

	while (i < WP_NUM_WEAPONS)
	{
		if (pm->ps->stats[STAT_WEAPONS] & 1 << i &&
			PM_WeaponOkOnVehicle(i))
		{
			//this one's good
			return i;
		}

		i++;
	}

	//oh dear!
	//assert(!"No valid veh weaps");
	return -1;
}

//force the vehicle to turn and travel to its forced destination point
static void PM_VehForcedTurning(bgEntity_t* veh)
{
	const bgEntity_t* dst = PM_BGEntForNum(veh->playerState->vehTurnaroundIndex);
	vec3_t dir;

	if (!veh || !veh->m_pVehicle)
	{
		return;
	}

	if (!dst)
	{
		//can't find dest ent?
		return;
	}

	pm->cmd.upmove = veh->m_pVehicle->m_ucmd.upmove = 127;
	pm->cmd.forwardmove = veh->m_pVehicle->m_ucmd.forwardmove = 0;
	pm->cmd.rightmove = veh->m_pVehicle->m_ucmd.rightmove = 0;

	VectorSubtract(dst->s.origin, veh->playerState->origin, dir);
	vectoangles(dir, dir);

	float yawD = AngleSubtract(pm->ps->viewangles[YAW], dir[YAW]);
	float pitchD = AngleSubtract(pm->ps->viewangles[PITCH], dir[PITCH]);

	yawD *= 0.6f * pml.frametime;
	pitchD *= 0.6f * pml.frametime;

#ifdef VEH_CONTROL_SCHEME_4
	veh->playerState->viewangles[YAW] = AngleSubtract(veh->playerState->viewangles[YAW], yawD);
	veh->playerState->viewangles[PITCH] = AngleSubtract(veh->playerState->viewangles[PITCH], pitchD);
	pm->ps->viewangles[YAW] = veh->playerState->viewangles[YAW];
	pm->ps->viewangles[PITCH] = 0;

	PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
	PM_SetPMViewAngle(veh->playerState, veh->playerState->viewangles, &pm->cmd);
	VectorClear(veh->m_pVehicle->m_vPrevRiderViewAngles);
	veh->m_pVehicle->m_vPrevRiderViewAngles[YAW] = AngleNormalize180(pm->ps->viewangles[YAW]);

#else //VEH_CONTROL_SCHEME_4

	pm->ps->viewangles[YAW] = AngleSubtract(pm->ps->viewangles[YAW], yawD);
	pm->ps->viewangles[PITCH] = AngleSubtract(pm->ps->viewangles[PITCH], pitchD);

	PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
#endif //VEH_CONTROL_SCHEME_4
}

#ifdef VEH_CONTROL_SCHEME_4
static void PM_VehFaceHyperspacePoint(bgEntity_t* veh)
{
	if (!veh || !veh->m_pVehicle)
	{
		return;
	}
	else
	{
		float timeFrac = ((float)(pm->cmd.serverTime - veh->playerState->hyperSpaceTime)) / HYPERSPACE_TIME;
		float	turnRate, aDelta;
		int		i, matchedAxes = 0;

		pm->cmd.upmove = veh->m_pVehicle->m_ucmd.upmove = 127;
		pm->cmd.forwardmove = veh->m_pVehicle->m_ucmd.forwardmove = 0;
		pm->cmd.rightmove = veh->m_pVehicle->m_ucmd.rightmove = 0;

		turnRate = (90.0f * pml.frametime);
		for (i = 0; i < 3; i++)
		{
			aDelta = AngleSubtract(veh->playerState->hyperSpaceAngles[i], veh->m_pVehicle->m_vOrientation[i]);
			if (fabs(aDelta) < turnRate)
			{//all is good
				veh->playerState->viewangles[i] = veh->playerState->hyperSpaceAngles[i];
				matchedAxes++;
			}
			else
			{
				aDelta = AngleSubtract(veh->playerState->hyperSpaceAngles[i], veh->playerState->viewangles[i]);
				if (fabs(aDelta) < turnRate)
				{
					veh->playerState->viewangles[i] = veh->playerState->hyperSpaceAngles[i];
				}
				else if (aDelta > 0)
				{
					if (i == YAW)
					{
						veh->playerState->viewangles[i] = AngleNormalize360(veh->playerState->viewangles[i] + turnRate);
					}
					else
					{
						veh->playerState->viewangles[i] = AngleNormalize180(veh->playerState->viewangles[i] + turnRate);
					}
				}
				else
				{
					if (i == YAW)
					{
						veh->playerState->viewangles[i] = AngleNormalize360(veh->playerState->viewangles[i] - turnRate);
					}
					else
					{
						veh->playerState->viewangles[i] = AngleNormalize180(veh->playerState->viewangles[i] - turnRate);
					}
				}
			}
		}

		pm->ps->viewangles[YAW] = veh->playerState->viewangles[YAW];
		pm->ps->viewangles[PITCH] = 0.0f;

		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
		PM_SetPMViewAngle(veh->playerState, veh->playerState->viewangles, &pm->cmd);
		VectorClear(veh->m_pVehicle->m_vPrevRiderViewAngles);
		veh->m_pVehicle->m_vPrevRiderViewAngles[YAW] = AngleNormalize180(pm->ps->viewangles[YAW]);

		if (timeFrac < HYPERSPACE_TELEPORT_FRAC)
		{//haven't gone through yet
			if (matchedAxes < 3)
			{//not facing the right dir yet
				//keep hyperspace time up to date
				veh->playerState->hyperSpaceTime += pml.msec;
			}
			else if (!(veh->playerState->eFlags2 & EF2_HYPERSPACE))
			{//flag us as ready to hyperspace!
				veh->playerState->eFlags2 |= EF2_HYPERSPACE;
			}
		}
	}
}

#else //VEH_CONTROL_SCHEME_4

static void PM_VehFaceHyperspacePoint(const bgEntity_t* veh)
{
	if (!veh || !veh->m_pVehicle)
	{
		return;
	}
	const float timeFrac = (float)(pm->cmd.serverTime - veh->playerState->hyperSpaceTime) / HYPERSPACE_TIME;
	int matchedAxes = 0;

	pm->cmd.upmove = veh->m_pVehicle->m_ucmd.upmove = 127;
	pm->cmd.forwardmove = veh->m_pVehicle->m_ucmd.forwardmove = 0;
	pm->cmd.rightmove = veh->m_pVehicle->m_ucmd.rightmove = 0;

	const float turnRate = 90.0f * pml.frametime;
	for (int i = 0; i < 3; i++)
	{
		float aDelta = AngleSubtract(veh->playerState->hyperSpaceAngles[i], veh->m_pVehicle->m_vOrientation[i]);
		if (fabs(aDelta) < turnRate)
		{
			//all is good
			pm->ps->viewangles[i] = veh->playerState->hyperSpaceAngles[i];
			matchedAxes++;
		}
		else
		{
			aDelta = AngleSubtract(veh->playerState->hyperSpaceAngles[i], pm->ps->viewangles[i]);
			if (fabs(aDelta) < turnRate)
			{
				pm->ps->viewangles[i] = veh->playerState->hyperSpaceAngles[i];
			}
			else if (aDelta > 0)
			{
				if (i == YAW)
				{
					pm->ps->viewangles[i] = AngleNormalize360(pm->ps->viewangles[i] + turnRate);
				}
				else
				{
					pm->ps->viewangles[i] = AngleNormalize180(pm->ps->viewangles[i] + turnRate);
				}
			}
			else
			{
				if (i == YAW)
				{
					pm->ps->viewangles[i] = AngleNormalize360(pm->ps->viewangles[i] - turnRate);
				}
				else
				{
					pm->ps->viewangles[i] = AngleNormalize180(pm->ps->viewangles[i] - turnRate);
				}
			}
		}
	}

	PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);

	if (timeFrac < HYPERSPACE_TELEPORT_FRAC)
	{
		//haven't gone through yet
		if (matchedAxes < 3)
		{
			//not facing the right dir yet
			//keep hyperspace time up to date
			veh->playerState->hyperSpaceTime += pml.msec;
		}
		else if (!(veh->playerState->eFlags2 & EF2_HYPERSPACE))
		{
			//flag us as ready to hyperspace!
			veh->playerState->eFlags2 |= EF2_HYPERSPACE;
		}
	}
}

#endif //VEH_CONTROL_SCHEME_4

void bg_vehicle_adjust_b_box_for_orientation(const Vehicle_t* veh, vec3_t origin, vec3_t mins, vec3_t maxs,
	const int clientNum, const int tracemask,
	void (*local_trace)(trace_t* results, const vec3_t start,
		const vec3_t minimum_mins, const vec3_t maximum_maxs,
		const vec3_t end, int pass_entity_num,
		int content_mask))
{
	if (!veh
		|| !veh->m_pVehicleInfo->length
		|| !veh->m_pVehicleInfo->width
		|| !veh->m_pVehicleInfo->height)
		//|| veh->m_LandTrace.fraction < 1.0f )
	{
		return;
	}
	if (veh->m_pVehicleInfo->type != VH_FIGHTER
		//&& veh->m_pVehicleInfo->type != VH_SPEEDER
		&& veh->m_pVehicleInfo->type != VH_FLIER)
	{
		//only those types of vehicles have dynamic bboxes, the rest just use a static bbox
		VectorSet(maxs, veh->m_pVehicleInfo->width / 2.0f, veh->m_pVehicleInfo->width / 2.0f,
			veh->m_pVehicleInfo->height + DEFAULT_MINS_2);
		VectorSet(mins, veh->m_pVehicleInfo->width / -2.0f, veh->m_pVehicleInfo->width / -2.0f, DEFAULT_MINS_2);
		return;
	}
	matrix3_t axis;
	vec3_t point[8], newMins, newMaxs;
	trace_t trace;

	AnglesToAxis(veh->m_vOrientation, axis);
	VectorMA(origin, veh->m_pVehicleInfo->length / 2.0f, axis[0], point[0]);
	VectorMA(origin, -veh->m_pVehicleInfo->length / 2.0f, axis[0], point[1]);
	//extrapolate each side up and down
	VectorMA(point[0], veh->m_pVehicleInfo->height / 2.0f, axis[2], point[0]);
	VectorMA(point[0], -veh->m_pVehicleInfo->height, axis[2], point[2]);
	VectorMA(point[1], veh->m_pVehicleInfo->height / 2.0f, axis[2], point[1]);
	VectorMA(point[1], -veh->m_pVehicleInfo->height, axis[2], point[3]);

	VectorMA(origin, veh->m_pVehicleInfo->width / 2.0f, axis[1], point[4]);
	VectorMA(origin, -veh->m_pVehicleInfo->width / 2.0f, axis[1], point[5]);
	//extrapolate each side up and down
	VectorMA(point[4], veh->m_pVehicleInfo->height / 2.0f, axis[2], point[4]);
	VectorMA(point[4], -veh->m_pVehicleInfo->height, axis[2], point[6]);
	VectorMA(point[5], veh->m_pVehicleInfo->height / 2.0f, axis[2], point[5]);
	VectorMA(point[5], -veh->m_pVehicleInfo->height, axis[2], point[7]);
	/*
		VectorMA( origin, veh->m_pVehicleInfo->height/2.0f, axis[2], point[4] );
		VectorMA( origin, -veh->m_pVehicleInfo->height/2.0f, axis[2], point[5] );
		*/
		//Now inflate a bbox around these points
	VectorCopy(origin, newMins);
	VectorCopy(origin, newMaxs);
	for (int cur_axis = 0; cur_axis < 3; cur_axis++)
	{
		for (int i = 0; i < 8; i++)
		{
			if (point[i][cur_axis] > newMaxs[cur_axis])
			{
				newMaxs[cur_axis] = point[i][cur_axis];
			}
			else if (point[i][cur_axis] < newMins[cur_axis])
			{
				newMins[cur_axis] = point[i][cur_axis];
			}
		}
	}
	VectorSubtract(newMins, origin, newMins);
	VectorSubtract(newMaxs, origin, newMaxs);
	//now see if that's a valid way to be
	if (local_trace)
	{
		local_trace(&trace, origin, newMins, newMaxs, origin, clientNum, tracemask);
	}
	else
	{
		//don't care about solid stuff then
		trace.startsolid = trace.allsolid = 0;
	}
	if (!trace.startsolid && !trace.allsolid)
	{
		//let's use it!
		VectorCopy(newMins, mins);
		VectorCopy(newMaxs, maxs);
	}
	//else: just use the last one, I guess...?
	//FIXME: make it as close as possible?  Or actually prevent the change in m_vOrientation?  Or push away from anything we hit?
}

/*
================
PmoveSingle

================
*/
extern int BG_EmplacedView(vec3_t base_angles, vec3_t angles, float* new_yaw, float constraint);
extern qboolean BG_FighterUpdate(Vehicle_t* p_veh, const usercmd_t* pUcmd, vec3_t trMins, vec3_t trMaxs, float gravity, void (*traceFunc)
	(trace_t* results, const vec3_t start, const vec3_t lmins, const vec3_t lmaxs, const vec3_t end, int pass_entity_num, int content_mask)); //FighterNPC.c

#define JETPACK_HOVER_HEIGHT	64

//#define _TESTING_VEH_PREDICTION

static void PM_MoveForKata(usercmd_t* ucmd)
{
	if (pm->ps->legsAnim == BOTH_A7_SOULCAL
		&& pm->ps->saber_move == LS_STAFF_SOULCAL)
	{
		//forward spinning staff attack
		ucmd->upmove = 0;

		if (PM_CanRollFromSoulCal(pm->ps))
		{
			ucmd->upmove = -127;
			ucmd->rightmove = 0;
			if (ucmd->forwardmove < 0)
			{
				ucmd->forwardmove = 0;
			}
		}
		else
		{
			ucmd->rightmove = 0;
			//FIXME: don't slide off people/obstacles?
			if (pm->ps->legsTimer >= 2750)
			{
				//not at end
				//push forward
				ucmd->forwardmove = 64;
			}
			else
			{
				ucmd->forwardmove = 0;
			}
		}
		if (pm->ps->legsTimer >= 2650
			&& pm->ps->legsTimer < 2850)
		{
			//the jump
			if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
			{
				//still on ground?
				//jump!
				pm->ps->velocity[2] = 250;
				pm->ps->fd.forceJumpZStart = pm->ps->origin[2]; //so we don't take damage if we land at same height
				PM_AddEvent(EV_JUMP);
			}
		}
	}
	else if (pm->ps->legsAnim == BOTH_A2_SPECIAL || pm->ps->legsAnim == BOTH_GRIEVOUS_SPIN)
	{
		//medium kata
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
		if (pm->ps->legsTimer < 2700 && pm->ps->legsTimer > 2300)
		{
			pm->cmd.forwardmove = 127;
		}
		else if (pm->ps->legsTimer < 900 && pm->ps->legsTimer > 500)
		{
			pm->cmd.forwardmove = 127;
		}
		else
		{
			pm->cmd.forwardmove = 0;
		}
	}
	else if (pm->ps->legsAnim == BOTH_A3_SPECIAL)
	{
		//strong kata
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
		if (pm->ps->legsTimer < 1700 && pm->ps->legsTimer > 1000)
		{
			pm->cmd.forwardmove = 127;
		}
		else
		{
			pm->cmd.forwardmove = 0;
		}
	}
	else if (pm->ps->legsAnim == BOTH_A4_SPECIAL)
	{
		//palpatine spin
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
		if (pm->ps->legsTimer < 1700 && pm->ps->legsTimer > 1000)
		{
			pm->cmd.forwardmove = 127;
		}
		else
		{
			pm->cmd.forwardmove = 0;
		}
	}
	else if (pm->ps->legsAnim == BOTH_A5_SPECIAL)
	{
		//palpatine spin
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
		if (pm->ps->legsTimer < 1700 && pm->ps->legsTimer > 1000)
		{
			pm->cmd.forwardmove = 127;
		}
		else
		{
			pm->cmd.forwardmove = 0;
		}
	}
	else
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}
}

extern qboolean PM_InAmputateMove(int anim);

static void PmoveSingle(pmove_t* pmove)
{
	qboolean stiffenedUp = qfalse;
	qboolean noAnimate = qfalse;
	int savedGravity = 0;

	pm = pmove;

	if (pm->cmd.buttons & BUTTON_ATTACK && pm->cmd.buttons & BUTTON_USE_HOLDABLE)
	{
		pm->cmd.buttons &= ~BUTTON_ATTACK;
		pm->cmd.buttons &= ~BUTTON_USE_HOLDABLE;
	}
	if (pm->cmd.buttons & BUTTON_ALT_ATTACK && pm->cmd.buttons & BUTTON_USE_HOLDABLE)
	{
		pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
		pm->cmd.buttons &= ~BUTTON_USE_HOLDABLE;
	}

	if (pm->ps->emplacedIndex)
	{
		if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
		{
			//hackerrific.
			pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			pm->cmd.buttons |= BUTTON_ATTACK;
		}
	}

	//set up these "global" bg ents
	pm_entSelf = PM_BGEntForNum(pm->ps->clientNum);
	if (pm->ps->m_iVehicleNum)
	{
		if (pm->ps->clientNum < MAX_CLIENTS)
		{
			//player riding vehicle
			pm_entVeh = PM_BGEntForNum(pm->ps->m_iVehicleNum);
		}
		else
		{
			//vehicle with player pilot
			pm_entVeh = PM_BGEntForNum(pm->ps->m_iVehicleNum - 1);
		}
	}
	else
	{
		//no vehicle ent
		pm_entVeh = NULL;
	}

	gPMDoSlowFall = PM_DoSlowFall();

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if (PM_IsRocketTrooper())
	{
		//kind of nasty, don't let them crouch or anything if nonhumanoid (probably a rockettrooper)
		if (pm->cmd.upmove < 0)
		{
			pm->cmd.upmove = 0;
		}
	}

	if (pm->ps->pm_type == PM_FLOAT)
	{
		//You get no control over where you go in grip movement
		stiffenedUp = qtrue;
	}
	else if (pm->ps->eFlags & EF_DISINTEGRATION)
	{
		stiffenedUp = qtrue;
	}
	else if (PM_SaberLockBreakAnim(pm->ps->legsAnim)
		|| PM_SaberLockBreakAnim(pm->ps->torsoAnim)
		|| pm->ps->saberLockTime >= pm->cmd.serverTime)
	{
		//can't move or turn
		stiffenedUp = qtrue;
		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
	}
	else if (pm->ps->saber_move == LS_A_BACK || pm->ps->saber_move == LS_A_BACK_CR ||
		pm->ps->saber_move == LS_A_BACKSTAB || pm->ps->saber_move == LS_A_BACKSTAB_B ||
		pm->ps->saber_move == LS_A_FLIP_STAB || pm->ps->saber_move == LS_A_FLIP_SLASH ||
		pm->ps->saber_move == LS_DUAL_LR || pm->ps->saber_move == LS_DUAL_FB)
	{
		if (pm->ps->fd.saberAnimLevel == SS_FAST || pm->ps->fd.saberAnimLevel == SS_TAVION)
		{
			if (pm->ps->legsAnim == BOTH_JUMPFLIPSTABDOWN)
			{ //flipover fast stance attack
				if (pm->ps->legsTimer < 1600 && pm->ps->legsTimer > 900)
				{
					pm->ps->viewangles[YAW] += pml.frametime * 180.0f;
					PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
				}
			}
		}
		else
		{
			if ((pm->ps->legsAnim == BOTH_JUMPFLIPSTABDOWN && pm->ps->legsTimer < 1600 && pm->ps->legsTimer > 1150)
				|| (pm->ps->legsAnim == BOTH_JUMPFLIPSLASHDOWN1 && pm->ps->legsTimer < 1600 && pm->ps->legsTimer > 900))
			{ //flipover medium stance attack
				pm->ps->viewangles[YAW] += pml.frametime * 180.0f;
				PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
			}
		}
		stiffenedUp = qtrue;
	}
	else if (pm->ps->legsAnim == BOTH_A2_STABBACK1 ||
		pm->ps->legsAnim == BOTH_A2_STABBACK1B ||
		pm->ps->legsAnim == BOTH_ATTACK_BACK ||
		pm->ps->legsAnim == BOTH_CROUCHATTACKBACK1 ||
		pm->ps->legsAnim == BOTH_FORCELEAP2_T__B_ ||
		pm->ps->legsAnim == BOTH_FORCELEAP_PALP ||
		pm->ps->legsAnim == BOTH_JUMPFLIPSTABDOWN ||
		pm->ps->legsAnim == BOTH_JUMPFLIPSLASHDOWN1)
	{
		stiffenedUp = qtrue;
	}
	else if (pm->ps->legsAnim == BOTH_ROLL_STAB)
	{
		stiffenedUp = qtrue;
		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
	}
	else if (pm->ps->heldByClient)
	{
		stiffenedUp = qtrue;
	}
	else if (PM_kick_move(pm->ps->saber_move) || PM_KickingAnim(pm->ps->legsAnim))
	{
		stiffenedUp = qtrue;
		if (pm->ps->legsTimer <= 0)
		{
			//done?  be immeditately ready to do an attack
			pm->ps->saber_move = LS_READY;
		}
	}
	else if (PM_InGrappleMove(pm->ps->torsoAnim))
	{
		stiffenedUp = qtrue;
		//PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
	}
	//else if (PM_MeleeblockHoldAnim(pm->ps->torsoAnim) || PM_MeleeblockAnim(pm->ps->torsoAnim))
	//{
	//	stiffenedUp = qtrue;
	//	//PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
	//}
	else if (pm->ps->saber_move == LS_STABDOWN_DUAL ||
		pm->ps->saber_move == LS_STABDOWN_STAFF ||
		pm->ps->saber_move == LS_STABDOWN_BACKHAND ||
		pm->ps->saber_move == LS_STABDOWN)
	{
		//FIXME: need to only move forward until we bump into our target...?
		if (pm->ps->legsTimer < 800)
		{
			//freeze movement near end of anim
			stiffenedUp = qtrue;
			PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
		}
		else
		{
			//force forward til then
			pm->cmd.rightmove = 0;
			pm->cmd.upmove = 0;
			pm->cmd.forwardmove = 64;
		}
	}
	else if (pm->ps->saber_move == LS_PULL_ATTACK_STAB || pm->ps->saber_move == LS_PULL_ATTACK_SWING)
	{
		stiffenedUp = qtrue;
	}
	else if (PM_SaberInKata(pm->ps->saber_move) ||
		PM_InKataAnim(pm->ps->torsoAnim) ||
		PM_InKataAnim(pm->ps->legsAnim))
	{
		PM_MoveForKata(&pm->cmd);
	}
	else if (BG_FullBodyTauntAnim(pm->ps->legsAnim))
	{
		if (pm->cmd.buttons & BUTTON_ATTACK
			|| pm->cmd.buttons & BUTTON_ALT_ATTACK
			|| pm->cmd.buttons & BUTTON_FORCEPOWER
			|| pm->cmd.buttons & BUTTON_FORCEGRIP
			|| pm->cmd.buttons & BUTTON_FORCE_LIGHTNING
			|| pm->cmd.buttons & BUTTON_FORCE_DRAIN
			|| pm->cmd.buttons & BUTTON_BLOCK
			|| pm->cmd.buttons & BUTTON_KICK
			|| pm->cmd.buttons & BUTTON_USE
			|| pm->cmd.buttons & BUTTON_DASH
			|| pm->cmd.rightmove
			|| pm->cmd.upmove
			|| pm->cmd.forwardmove)
		{
			//stop the anim
			if (pm->ps->legsAnim == BOTH_MEDITATE)
			{
				PM_SetAnim(SETANIM_BOTH, BOTH_MEDITATE_END, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else if (pm->ps->legsAnim == BOTH_MEDITATE1)
			{
				PM_SetAnim(SETANIM_BOTH, BOTH_MEDITATE_END1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else
			{
				pm->ps->legsTimer = pm->ps->torsoTimer = 0;
			}
		}
		else
		{
			if (pm->ps->legsAnim == BOTH_MEDITATE)
			{
				if (pm->ps->legsTimer < 100)
				{
					pm->ps->legsTimer = 100;
				}
			}
			if (pm->ps->legsAnim == BOTH_MEDITATE1)
			{
				if (pm->ps->legsTimer < 100)
				{
					pm->ps->legsTimer = 100;
				}
			}
			if (pm->ps->legsTimer > 0 || pm->ps->torsoTimer > 0)
			{
				stiffenedUp = qtrue;
				PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
				pm->cmd.rightmove = 0;
				pm->cmd.upmove = 0;
				pm->cmd.forwardmove = 0;
				pm->cmd.buttons = 0;
			}
		}
	}
	else if (pm->ps->legsAnim == BOTH_MEDITATE_END && pm->ps->legsTimer > 0)
	{
		stiffenedUp = qtrue;
		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
		pm->cmd.forwardmove = 0;
		pm->cmd.buttons = 0;
	}
	else if (pm->ps->legsAnim == BOTH_MEDITATE_END1 && pm->ps->legsTimer > 0)
	{
		stiffenedUp = qtrue;
		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
		pm->cmd.forwardmove = 0;
		pm->cmd.buttons = 0;
	}
	else if (pm->ps->legsAnim == BOTH_FORCELAND1 ||
		pm->ps->legsAnim == BOTH_FORCELANDBACK1 ||
		pm->ps->legsAnim == BOTH_FORCELANDRIGHT1 ||
		pm->ps->legsAnim == BOTH_FORCELANDLEFT1)
	{
		//can't move while in a force land
		stiffenedUp = qtrue;
	}
	//EMOTE SYSTEM
	else if (BG_FullBodyEmoteAnim(pm->ps->torsoAnim))
	{
		if (pm->cmd.buttons & BUTTON_ATTACK
			|| pm->cmd.buttons & BUTTON_ALT_ATTACK
			|| pm->cmd.buttons & BUTTON_FORCEPOWER
			|| pm->cmd.buttons & BUTTON_FORCEGRIP
			|| pm->cmd.buttons & BUTTON_FORCE_LIGHTNING
			|| pm->cmd.buttons & BUTTON_FORCE_DRAIN
			|| pm->cmd.buttons & BUTTON_BLOCK
			|| pm->cmd.buttons & BUTTON_KICK
			|| pm->cmd.buttons & BUTTON_USE
			|| pm->cmd.buttons & BUTTON_DASH
			|| pm->cmd.rightmove
			|| pm->cmd.upmove
			|| pm->cmd.forwardmove)
		{
			//stop the anim
			if (pm->ps->torsoAnim == PLAYER_SURRENDER_START)
			{
				PM_SetAnim(SETANIM_TORSO, PLAYER_SURRENDER_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else
			{
				pm->ps->legsTimer = pm->ps->torsoTimer = 0;
			}
		}
		else
		{
			if (pm->ps->torsoAnim == PLAYER_SURRENDER_START)
			{
				if (pm->ps->torsoTimer < 100)
				{
					pm->ps->legsTimer = 100;
				}
			}
			if (pm->ps->legsTimer > 0 || pm->ps->torsoTimer > 0)
			{
				stiffenedUp = qtrue;
				PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
				pm->cmd.rightmove = 0;
				pm->cmd.upmove = 0;
				pm->cmd.forwardmove = 0;
				pm->cmd.buttons = 0;
			}
		}
	}
	else if (pm->ps->torsoAnim == PLAYER_SURRENDER_STOP && pm->ps->torsoTimer > 0)
	{
		stiffenedUp = qtrue;
		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
		pm->cmd.forwardmove = 0;
		pm->cmd.buttons = 0;
	} //stiffenedup
	else if (!in_camera && (PM_SaberInAttack(pm->ps->saber_move)
		|| pm_saber_in_special_attack(pm->ps->torsoAnim)
		|| PM_SpinningSaberAnim(pm->ps->legsAnim)
		|| PM_SaberInStart(pm->ps->saber_move)
		|| PM_SaberInMassiveBounce(pm->ps->torsoAnim) && pm->ps->torsoTimer))
	{
		//attacking or spinning (or, if player, starting an attack)
#ifdef _GAME
		if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT)
		{
			stiffenedUp = qtrue;
		}
#endif
	}
	else if (BG_FullBodyCowerAnim(pm->ps->torsoAnim))
	{
		if (pm->cmd.buttons & BUTTON_ATTACK
			|| pm->cmd.buttons & BUTTON_ALT_ATTACK
			|| pm->cmd.buttons & BUTTON_FORCEPOWER
			|| pm->cmd.buttons & BUTTON_FORCEGRIP
			|| pm->cmd.buttons & BUTTON_FORCE_LIGHTNING
			|| pm->cmd.buttons & BUTTON_FORCE_DRAIN
			|| pm->cmd.buttons & BUTTON_BLOCK
			|| pm->cmd.buttons & BUTTON_KICK
			|| pm->cmd.buttons & BUTTON_USE
			|| pm->cmd.buttons & BUTTON_DASH
			|| pm->cmd.rightmove
			|| pm->cmd.upmove
			|| pm->cmd.forwardmove)
		{
			//stop the anim
			if (pm->ps->torsoAnim == BOTH_COWER1_START)
			{
				PM_SetAnim(SETANIM_BOTH, BOTH_COWER1_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else if (pm->ps->torsoAnim == BOTH_COWER1)
			{
				PM_SetAnim(SETANIM_BOTH, BOTH_COWER1_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else
			{
				pm->ps->legsTimer = pm->ps->torsoTimer = 0;
			}
		}
		else
		{
			if (pm->ps->torsoAnim == BOTH_COWER1)
			{
				if (pm->ps->torsoTimer < 100)
				{
					pm->ps->legsTimer = 100;
				}
			}
			if (pm->ps->torsoAnim == BOTH_COWER1_START)
			{
				if (pm->ps->torsoTimer < 100)
				{
					pm->ps->legsTimer = 100;
				}
			}
			if (pm->ps->legsTimer > 0 || pm->ps->torsoTimer > 0)
			{
				stiffenedUp = qtrue;
				PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
				pm->cmd.rightmove = 0;
				pm->cmd.upmove = 0;
				pm->cmd.forwardmove = 0;
				pm->cmd.buttons = 0;
			}
		}
	}
	else if (pm->ps->torsoAnim == BOTH_COWER1_STOP && pm->ps->torsoTimer > 0)
	{
		stiffenedUp = qtrue;
		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
		pm->cmd.forwardmove = 0;
		pm->cmd.buttons = 0;
	}
	// END EMOTE
	else if (pm->ps->saber_move == LS_GRIEVOUS_SPECIAL || pm->ps->legsAnim == BOTH_GRIEVOUS_SPIN)
	{
		stiffenedUp = qtrue;
	}
	else if (pm->ps->legsAnim == BOTH_GRIEVOUS_LUNGE)
	{
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
		if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
		{
			//hit the ground
			pm->cmd.forwardmove = 0;
		}
	}
	else if (PM_InLedgeMove(pm->ps->torsoAnim))
	{
		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
	}

	if (pm->ps->saber_move == LS_A_LUNGE)
	{
		//can't move during lunge
		pm->cmd.rightmove = pm->cmd.upmove = 0;
		if (pm->ps->legsTimer > 500)
		{
			pm->cmd.forwardmove = 127;
		}
		else
		{
			pm->cmd.forwardmove = 0;
		}
	}

	if (pm->ps->saber_move == LS_A_JUMP_T__B_ || pm->ps->saber_move == LS_A_JUMP_PALP_)
	{
		//can't move during leap
		if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
		{
			//hit the ground
			pm->cmd.forwardmove = 0;
		}
	}

	if (pm->ps->legsAnim == BOTH_KISSER1LOOP ||
		pm->ps->legsAnim == BOTH_KISSEE1LOOP)
	{
		stiffenedUp = qtrue;
	}

	if (pm->ps->torsoAnim == BOTH_CONSOLE1)
	{
		stiffenedUp = qtrue;
		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
	}

	if (pm->ps->emplacedIndex)
	{
		if (pm->cmd.forwardmove < 0 || PM_GroundDistance() > 32.0f)
		{
			pm->ps->emplacedIndex = 0;
			pm->ps->saberHolstered = 0;
		}
		else
		{
			stiffenedUp = qtrue;
		}
	}

	if (pm->ps->legsAnim == BOTH_DODGE_HOLD_BL ||
		pm->ps->legsAnim == BOTH_DODGE_HOLD_BR ||
		pm->ps->legsAnim == BOTH_DODGE_HOLD_FL ||
		pm->ps->legsAnim == BOTH_DODGE_HOLD_L ||
		pm->ps->legsAnim == BOTH_DODGE_HOLD_R ||
		pm->ps->legsAnim == BOTH_DODGE_HOLD_FR)
	{
		//can't move while in a dodge
		stiffenedUp = qtrue;
	} // manualdodge
	else if (pm->ps->legsAnim == BOTH_DODGE_BL ||
		pm->ps->legsAnim == BOTH_DODGE_BR ||
		pm->ps->legsAnim == BOTH_DODGE_FL ||
		pm->ps->legsAnim == BOTH_DODGE_L ||
		pm->ps->legsAnim == BOTH_DODGE_R ||
		pm->ps->legsAnim == BOTH_DODGE_FR)
	{
		//can't move while in a dodge
		stiffenedUp = qtrue;
	}
	else if (pm->ps->legsAnim == BOTH_DODGE_MANUAL_BL ||
		pm->ps->legsAnim == BOTH_DODGE_MANUAL_BR ||
		pm->ps->legsAnim == BOTH_DODGE_MANUAL_FL ||
		pm->ps->legsAnim == BOTH_DODGE_MANUAL_L ||
		pm->ps->legsAnim == BOTH_DODGE_MANUAL_R ||
		pm->ps->legsAnim == BOTH_CROUCHDODGE ||
		pm->ps->legsAnim == BOTH_DODGE_MANUAL_FR)
	{
		//can't move while in a dodge
		stiffenedUp = qtrue;
	} // manualdodge
	else if (pm->ps->legsAnim == BOTH_MANUAL_BL ||
		pm->ps->legsAnim == BOTH_MANUAL_BR ||
		pm->ps->legsAnim == BOTH_MANUAL_FL ||
		pm->ps->legsAnim == BOTH_MANUAL_L ||
		pm->ps->legsAnim == BOTH_MANUAL_R ||
		pm->ps->legsAnim == BOTH_DODGE_B ||
		pm->ps->legsAnim == BOTH_MANUAL_FR)
	{
		//can't move while in a dodge
		stiffenedUp = qtrue;
	}
	else if (pm->ps->legsAnim == MELEE_STANCE_HOLD_BL ||
		pm->ps->legsAnim == MELEE_STANCE_HOLD_BR ||
		pm->ps->legsAnim == MELEE_STANCE_HOLD_LT ||
		pm->ps->legsAnim == MELEE_STANCE_HOLD_RT ||
		pm->ps->legsAnim == MELEE_STANCE_HOLD_T ||
		pm->ps->legsAnim == MELEE_STANCE_HOLD_B)
	{ //can't move while in a dodge
		stiffenedUp = qtrue;
	}
	else if (pm->ps->legsAnim == MELEE_STANCE_BL ||
		pm->ps->legsAnim == MELEE_STANCE_BR ||
		pm->ps->legsAnim == MELEE_STANCE_LT ||
		pm->ps->legsAnim == MELEE_STANCE_RT ||
		pm->ps->legsAnim == MELEE_STANCE_T ||
		pm->ps->legsAnim == MELEE_STANCE_B)
	{ //can't move while in a dodge
		stiffenedUp = qtrue;
	}
	else if (pm->cmd.buttons & BUTTON_GRAPPLE)
	{
		stiffenedUp = qtrue;
	}
	else if (PM_InAmputateMove(pm->ps->legsAnim))
	{
		//can't move
		stiffenedUp = qtrue;
	}

	if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->weaponstate == WEAPON_CHARGING_ALT)
	{
		//not allowed to move while charging the disruptor
		if (pm->cmd.forwardmove ||
			pm->cmd.rightmove ||
			pm->cmd.upmove > 0)
		{
			//get out
			pm->ps->weaponstate = WEAPON_READY;
			pm->ps->weaponTime = 1000;
			PM_AddEventWithParm(EV_WEAPON_CHARGE, WP_DISRUPTOR); //cut the weapon charge sound
			pm->cmd.upmove = 0;
		}
	}
	else if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1)
	{
		//can't jump
		if (pm->cmd.upmove > 0)
		{
			pm->cmd.upmove = 0;
		}
	}

	if (stiffenedUp)
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if (pm->ps->fd.forceGripCripple)
	{
		//don't let attack or alt attack if being gripped I guess
		pm->cmd.buttons &= ~BUTTON_ATTACK;
		pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
		pm->cmd.buttons &= ~BUTTON_GRAPPLE;
	}

	if (pm->ps->userInt3 == qtrue)
	{
		pm->cmd.buttons &= ~BUTTON_ATTACK;
		pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
		pm->cmd.buttons &= ~BUTTON_GRAPPLE;
	}

	if (BG_InRoll(pm->ps, pm->ps->legsAnim))
	{
		//can't roll unless you're able to move normally
#ifdef _GAME
		if (!(dmflags.integer & DF_JK2ROLL))
#else
		if (!(cgs.dmflags & DF_JK2ROLL))
#endif
			PM_CmdForRoll(pm->ps, pm->ps->legsAnim, &pm->cmd);
	}

	PM_CmdForSaberMoves(&pm->cmd);

	BG_AdjustClientSpeed(pm->ps, &pm->cmd, pm->cmd.serverTime);

	if (pm->ps->stats[STAT_HEALTH] <= 0)
	{
		pm->tracemask &= ~CONTENTS_BODY; // corpses can fly through bodies
	}

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if (abs(pm->cmd.forwardmove) > 64 || abs(pm->cmd.rightmove) > 64)
	{
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

	// set the talk balloon flag
	if (pm->cmd.buttons & BUTTON_TALK)
	{
		pm->ps->eFlags |= EF_TALK;
	}
	else
	{
		pm->ps->eFlags &= ~EF_TALK;
	}

	pm_cancelOutZoom = qfalse;
	if (pm->ps->weapon == WP_DISRUPTOR &&
		pm->ps->zoomMode == 1)
	{
		if (pm->cmd.buttons & BUTTON_ALT_ATTACK &&
			!(pm->cmd.buttons & BUTTON_ATTACK) &&
			pm->ps->zoomLocked)
		{
			pm_cancelOutZoom = qtrue;
		}
	}
	// In certain situations, we may want to control which attack buttons are pressed and what kind of functionality
	//	is attached to them
	PM_AdjustAttackStates(pm);

	// clear the respawned flag if attack and use are cleared
	if (pm->ps->stats[STAT_HEALTH] > 0 &&
		!(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE)))
	{
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if (pmove->cmd.buttons & BUTTON_TALK)
	{
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset(&pml, 0, sizeof pml);

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if (pml.msec < 1)
	{
		pml.msec = 1;
	}
	else if (pml.msec > 200)
	{
		pml.msec = 200;
	}

	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy(pm->ps->origin, pml.previous_origin);

	// save old velocity for crashlanding
	VectorCopy(pm->ps->velocity, pml.previous_velocity);

	pml.frametime = pml.msec * 0.001;

	if (pm->ps->clientNum >= MAX_CLIENTS &&
		pm_entSelf &&
		pm_entSelf->s.NPC_class == CLASS_VEHICLE)
	{
		//we are a vehicle
		bgEntity_t* veh = pm_entSelf;
		assert(veh && veh->m_pVehicle);
		if (veh && veh->m_pVehicle)
		{
			veh->m_pVehicle->m_fTimeModifier = pml.frametime * 60.0f;
		}
	}
	else if (pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&& pm->ps->m_iVehicleNum)
	{
		bgEntity_t* veh = pm_entVeh;

		if (veh && veh->playerState &&
			pm->cmd.serverTime - veh->playerState->hyperSpaceTime < HYPERSPACE_TIME)
		{
			//going into hyperspace, turn to face the right angles
			PM_VehFaceHyperspacePoint(veh);
		}
		else if (veh && veh->playerState &&
			veh->playerState->vehTurnaroundIndex &&
			veh->playerState->vehTurnaroundTime > pm->cmd.serverTime)
		{
			//riding this vehicle, turn my view too
			PM_VehForcedTurning(veh);
		}
	}

	if (pm->ps->legsAnim == BOTH_FORCEWALLRUNFLIP_ALT &&
		pm->ps->legsTimer > 0)
	{
		vec3_t vFwd, fwdAng;
		VectorSet(fwdAng, 0.0f, pm->ps->viewangles[YAW], 0.0f);

		AngleVectors(fwdAng, vFwd, NULL, NULL);
		if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
		{
			const float savZ = pm->ps->velocity[2];
			VectorScale(vFwd, 100, pm->ps->velocity);
			pm->ps->velocity[2] = savZ;
		}
		pm->cmd.forwardmove = pm->cmd.rightmove = pm->cmd.upmove = 0;
		PM_AdjustAnglesForWallRunUpFlipAlt(&pm->cmd);
	}
	PM_AdjustAngleForWallJump(pm->ps, &pm->cmd, qtrue);
	PM_AdjustAngleForWallRunUp(pm->ps, &pm->cmd, qtrue);
	PM_AdjustAngleForWallRun(pm->ps, &pm->cmd, qtrue);
	PM_AdjustAnglesForKnockdown(pm->ps, &pm->cmd);
	PM_AdjustAngleForWallGrab(pm->ps, &pm->cmd);
	PM_AdjustAnglesForLongJump(pm->ps, &pm->cmd);

	if (pm->ps->saber_move == LS_A_LUNGE ||
		pm->ps->saber_move == LS_A_BACK_CR || pm->ps->saber_move == LS_A_BACK ||
		pm->ps->saber_move == LS_A_BACKSTAB || pm->ps->saber_move == LS_A_BACKSTAB_B)
	{
		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
	}

	if (pm->ps->legsAnim == BOTH_KISSER1LOOP ||
		pm->ps->legsAnim == BOTH_KISSEE1LOOP)
	{
		pm->ps->viewangles[PITCH] = 0;
		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
	}

	PM_SetSpecialMoveValues();

	// update the viewangles
	PM_UpdateViewAngles(pm->ps->fd.saberAnimLevel, pm->ps, &pm->cmd);

	AngleVectors(pm->ps->viewangles, pml.forward, pml.right, pml.up);

	if (pm->cmd.upmove < 10 && !(pm->ps->pm_flags & PMF_STUCK_TO_WALL))
	{
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if (pm->cmd.forwardmove < 0)
	{
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	}
	else if (pm->cmd.forwardmove > 0 || pm->cmd.forwardmove == 0 && pm->cmd.rightmove)
	{
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if (pm->ps->pm_type >= PM_DEAD)
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if (pm->ps->saberLockTime >= pm->cmd.serverTime)
	{
		pm->cmd.upmove = 0;
		pm->cmd.forwardmove = 0; //50;
		pm->cmd.rightmove = 0; //*= 0.1;
	}

	if (pm->ps->pm_type == PM_SPECTATOR)
	{
		PM_CheckDuck();
		if (!pm->noSpecMove)
		{
			PM_FlyMove();
		}
		PM_DropTimers();
		return;
	}

	if (pm->ps->pm_type == PM_NOCLIP)
	{
		if (pm->ps->clientNum < MAX_CLIENTS)
		{
			PM_NoclipMove();
			PM_DropTimers();
			return;
		}
	}

	if (pm->ps->pm_type == PM_FREEZE)
	{
		return; // no movement at all
	}

	if (pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_SPINTERMISSION)
	{
		return; // no movement at all
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set minimum_mins, maximum_maxs, and viewheight
	PM_CheckDuck();
	if (pm->ps->pm_type == PM_JETPACK)
	{
		savedGravity = pm->ps->gravity;

		if (pm->cmd.rightmove || pm->cmd.forwardmove || pm->cmd.upmove)
		{
			pm->ps->gravity = 0.0f;
		}
	}
	else if (gPMDoSlowFall)
	{
		savedGravity = pm->ps->gravity;
		pm->ps->gravity *= 0.5;
	}

	//if we're in jetpack mode then see if we should be jetting around
	if (pm->ps->pm_type == PM_JETPACK)
	{
		//flying around with jetpack
		PM_JetPackAnim();

		if (pm->ps->weapon == WP_SABER && PM_SpinningSaberAnim(pm->ps->legsAnim))
		{
			//make him stir around since he shouldn't have any real control when spinning
			pm->ps->velocity[0] += Q_irand(-100, 100);
			pm->ps->velocity[1] += Q_irand(-100, 100);
		}

		if (pm->cmd.upmove || pm->cmd.rightmove || pm->cmd.forwardmove)
		{
			pm->ps->eFlags |= EF_JETPACK_FLAMING; //going up
			pm->ps->eFlags |= EF_JETPACK_ACTIVE;
			pm->ps->eFlags |= EF3_JETPACK_HOVER;
		}
		else
		{
			pm->ps->eFlags &= ~EF_JETPACK_FLAMING; //idling
			pm->ps->eFlags |= EF_JETPACK_ACTIVE;
			pm->ps->eFlags &= ~EF3_JETPACK_HOVER;
		}
	}

	if (pm->ps->clientNum >= MAX_CLIENTS &&
		pm_entSelf && pm_entSelf->m_pVehicle)
	{
		//Now update our minimum_mins/maximum_maxs to match our m_vOrientation based on our length, width & height
		bg_vehicle_adjust_b_box_for_orientation(pm_entSelf->m_pVehicle, pm->ps->origin, pm->mins, pm->maxs,
			pm->ps->clientNum, pm->tracemask, pm->trace);
	}

	// set groundentity
	PM_GroundTrace();

	if (pm->ps->groundEntityNum == ENTITYNUM_WORLD)
	{
		GROUND_TIME[pm->ps->clientNum] = pm->cmd.serverTime;
	}

	if (pm_entSelf->s.botclass == BCLASS_BOBAFETT
		|| pm_entSelf->s.botclass == BCLASS_MANDOLORIAN
		|| pm_entSelf->s.botclass == BCLASS_MANDOLORIAN1
		|| pm_entSelf->s.botclass == BCLASS_MANDOLORIAN2)
	{
		if (pm->ps->eFlags & EF_DEAD || pm->ps->pm_type == PM_DEAD)
		{
			pm->ps->eFlags &= ~EF_JETPACK_ACTIVE;
			pm->ps->eFlags &= ~EF_JETPACK_FLAMING;
			pm->ps->pm_type = PM_DEAD;
		}
		else if (GROUND_TIME[pm->ps->clientNum] >= pm->cmd.serverTime - 500
			&& pm->cmd.upmove > 0)
		{
			// Have jetpack and jumping, but not activated yet. Make sure jetpack is not active...
			pm->ps->eFlags &= ~EF_JETPACK_ACTIVE;
			pm->ps->eFlags &= ~EF_JETPACK_FLAMING;
			pm->ps->pm_type = PM_NORMAL;
		}
		else if (pm->ps->eFlags & EF_JETPACK
			&& pm->ps->groundEntityNum != ENTITYNUM_WORLD
			&& pm->cmd.upmove > 0)
		{
			// Have jetpack and jumping, make sure jetpack is active...
			pm->ps->eFlags |= EF_JETPACK_ACTIVE;

			if (pm->ps->velocity[2] > 10)
			{
				// Also hit the afterburner...
				pm->ps->eFlags |= EF_JETPACK_FLAMING;
			}

			pm->ps->pm_type = PM_JETPACK;
		}
		else if (pm->ps->eFlags & EF_JETPACK
			&& pm->ps->pm_type == PM_JETPACK
			&& pm->ps->groundEntityNum != ENTITYNUM_WORLD
			&& pm->ps->velocity[2] < 0
			&& pm->cmd.upmove == 0)
		{
			// Hover at this height...
			pm->ps->eFlags |= EF3_JETPACK_HOVER;
			pm->ps->eFlags &= ~EF_JETPACK_ACTIVE;
			pm->ps->eFlags &= ~EF_JETPACK_FLAMING;
			pm->ps->pm_type = PM_JETPACK;
			pm->cmd.upmove = 150;
		}
		else if (GROUND_TIME[pm->ps->clientNum] >= pm->cmd.serverTime)
		{
			// On the ground. Make sure jetpack is deactivated...
			pm->ps->eFlags &= ~EF_JETPACK_ACTIVE;
			pm->ps->eFlags &= ~EF_JETPACK_FLAMING;
			pm->ps->eFlags &= ~EF3_JETPACK_HOVER;
			pm->ps->pm_type = PM_NORMAL;
		}
	}

	if (pm_flying == FLY_HOVER)
	{
		//never stick to the ground
		PM_HoverTrace();
	}

	if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
	{
		//on ground
		pm->ps->fd.forceJumpZStart = 0;
	}

	if (pm->ps->pm_type == PM_DEAD)
	{
		if (pm->ps->clientNum >= MAX_CLIENTS &&
			pm_entSelf &&
			pm_entSelf->s.NPC_class == CLASS_VEHICLE &&
			pm_entSelf->m_pVehicle->m_pVehicleInfo->type != VH_ANIMAL)
		{
			//vehicles don't use deadmove
		}
		else
		{
			PM_DeadMove();
		}
	}

	PM_DropTimers();

#ifdef _TESTING_VEH_PREDICTION
#ifndef _GAME
	{
		vec3_t blah;
		VectorMA(pm->ps->origin, 128.0f, pm->ps->moveDir, blah);
		CG_TestLine(pm->ps->origin, blah, 1, 0x0000ff, 1);

		VectorMA(pm->ps->origin, 1.0f, pm->ps->velocity, blah);
		CG_TestLine(pm->ps->origin, blah, 1, 0xff0000, 1);
	}
#endif
#endif

	if (pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&& pm->ps->m_iVehicleNum)
	{
		//a player riding a vehicle
		bgEntity_t* veh = pm_entVeh;

		if (veh && veh->m_pVehicle &&
			(veh->m_pVehicle->m_pVehicleInfo->type == VH_WALKER || veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER))
		{
			//*sigh*, until we get forced weapon-switching working?
			pm->cmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK);
			pm->ps->eFlags &= ~(EF_FIRING | EF_ALT_FIRING);
			//pm->cmd.weapon = pm->ps->weapon;
		}
	}

	if (!pm->ps->m_iVehicleNum &&
		pm_entSelf->s.NPC_class != CLASS_VEHICLE &&
		pm_entSelf->s.NPC_class != CLASS_RANCOR &&
		pm->ps->groundEntityNum < ENTITYNUM_WORLD &&
		pm->ps->groundEntityNum >= MAX_CLIENTS)
	{
		//I am a player client, not riding on a vehicle, and potentially standing on an NPC
		bgEntity_t* pEnt = PM_BGEntForNum(pm->ps->groundEntityNum);

		if (pEnt && pEnt->s.eType == ET_NPC &&
			pEnt->s.NPC_class != CLASS_VEHICLE) //don't bounce on vehicles
		{
			//this is actually an NPC, let's try to bounce of its head to make sure we can't just stand around on top of it.
			if (pm->ps->velocity[2] < 270)
			{
				//try forcing velocity up and also force him to jump
				pm->ps->velocity[2] = 270; //seems reasonable
				pm->cmd.upmove = 127;
			}
		}
#ifdef _GAME
		else if (!pm->ps->zoomMode &&
			pm_entSelf //I exist
			&& pEnt->m_pVehicle) //ent has a vehicle
		{
			const gentity_t* gEnt = (gentity_t*)pEnt;
			if (gEnt->client
				&& !gEnt->client->ps.m_iVehicleNum //vehicle is empty
				&& gEnt->spawnflags & 2) //SUSPENDED
			{
				//it's a vehicle, see if we should get in it
				//if land on an empty, suspended vehicle, get in it
				pEnt->m_pVehicle->m_pVehicleInfo->Board(pEnt->m_pVehicle, pm_entSelf);
			}
		}
#endif
	}

	if (pm->ps->clientNum >= MAX_CLIENTS &&
		pm_entSelf &&
		pm_entSelf->s.NPC_class == CLASS_VEHICLE)
	{
		//we are a vehicle
		bgEntity_t* veh = pm_entSelf;

		assert(veh && veh->playerState && veh->m_pVehicle && veh->s.number >= MAX_CLIENTS);

		if (veh->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER)
		{
			//kind of hacky, don't want to do this for flying vehicles
			veh->m_pVehicle->m_vOrientation[PITCH] = pm->ps->viewangles[PITCH];
		}

		if (!pm->ps->m_iVehicleNum)
		{
			//no one is driving, just update and get out
#ifdef _GAME
			veh->m_pVehicle->m_pVehicleInfo->Update(veh->m_pVehicle, &pm->cmd);
			veh->m_pVehicle->m_pVehicleInfo->Animate(veh->m_pVehicle);
#endif
		}
		else
		{
			bgEntity_t* self = pm_entVeh;
#ifdef _GAME
			int i = 0;
#endif

			assert(self && self->playerState && self->s.number < MAX_CLIENTS);

			if (pm->ps->pm_type == PM_DEAD &&
				veh->m_pVehicle->m_ulFlags & VEH_CRASHING)
			{
				veh->m_pVehicle->m_ulFlags &= ~VEH_CRASHING;
			}

			if (self->playerState->m_iVehicleNum)
			{
				//only do it if they still have a vehicle (didn't get ejected this update or something)
				PM_VehicleViewAngles(self->playerState, veh, &veh->m_pVehicle->m_ucmd);
			}

#ifdef _GAME
			veh->m_pVehicle->m_pVehicleInfo->Update(veh->m_pVehicle, &veh->m_pVehicle->m_ucmd);
			veh->m_pVehicle->m_pVehicleInfo->Animate(veh->m_pVehicle);

			veh->m_pVehicle->m_pVehicleInfo->UpdateRider(veh->m_pVehicle, self, &veh->m_pVehicle->m_ucmd);
			//update the passengers
			while (i < veh->m_pVehicle->m_iNumPassengers)
			{
				if (veh->m_pVehicle->m_ppPassengers[i])
				{
					const gentity_t* thePassenger = (gentity_t*)veh->m_pVehicle->m_ppPassengers[i];
					//yes, this is, in fact, ass.
					if (thePassenger->inuse && thePassenger->client)
					{
						veh->m_pVehicle->m_pVehicleInfo->UpdateRider(veh->m_pVehicle,
							veh->m_pVehicle->m_ppPassengers[i],
							&thePassenger->client->pers.cmd);
					}
				}
				i++;
			}
#else
			if (!veh->playerState->vehBoarding) //|| veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
			{
				if (veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
				{
					//client must explicitly call this for prediction
					BG_FighterUpdate(veh->m_pVehicle, &veh->m_pVehicle->m_ucmd, pm->mins, pm->maxs,
						self->playerState->gravity, pm->trace);
				}

				if (veh->m_pVehicle->m_iBoarding == 0)
				{
					vec3_t vRollAng;

					//make sure we are set as its pilot cgame side
					veh->m_pVehicle->m_pPilot = self;

					// Keep track of the old orientation.
					VectorCopy(veh->m_pVehicle->m_vOrientation, veh->m_pVehicle->m_vPrevOrientation);

					veh->m_pVehicle->m_pVehicleInfo->ProcessOrientCommands(veh->m_pVehicle);
					PM_SetPMViewAngle(veh->playerState, veh->m_pVehicle->m_vOrientation, &veh->m_pVehicle->m_ucmd);
					veh->m_pVehicle->m_pVehicleInfo->ProcessMoveCommands(veh->m_pVehicle);

					vRollAng[YAW] = self->playerState->viewangles[YAW];
					vRollAng[PITCH] = self->playerState->viewangles[PITCH];
					vRollAng[ROLL] = veh->m_pVehicle->m_vOrientation[ROLL];
					PM_SetPMViewAngle(self->playerState, vRollAng, &pm->cmd);

					// Setup the move direction.
					if (veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
					{
						AngleVectors(veh->m_pVehicle->m_vOrientation, veh->playerState->moveDir, NULL, NULL);
					}
					else
					{
						vec3_t vVehAngles;

						VectorSet(vVehAngles, 0, veh->m_pVehicle->m_vOrientation[YAW], 0);
						AngleVectors(vVehAngles, veh->playerState->moveDir, NULL, NULL);
					}
				}
			}
			else if (veh->playerState)
			{
				veh->playerState->speed = 0.0f;
				if (veh->m_pVehicle)
				{
					PM_SetPMViewAngle(self->playerState, veh->m_pVehicle->m_vOrientation, &pm->cmd);
					PM_SetPMViewAngle(veh->playerState, veh->m_pVehicle->m_vOrientation, &pm->cmd);
				}
			}
#endif
		}
		noAnimate = qtrue;
	}

	if (pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&& pm->ps->m_iVehicleNum)
	{
		//don't even run physics on a player if he's on a vehicle - he goes where the vehicle goes
	}
	else
	{
		//don't even run physics on a player if he's on a vehicle - he goes where the vehicle goes
		if (pm->ps->pm_type == PM_FLOAT
			|| pm_flying == FLY_NORMAL)
		{
			PM_FlyMove();
		}
		else if (pm_flying == FLY_VEHICLE)
		{
			PM_FlyVehicleMove();
		}
		else
		{
			if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
			{
				PM_WaterJumpMove();
			}
			else if (pm->ps->pm_flags & PMF_GRAPPLE_PULL)
			{
				PM_GrappleMove();
				PM_AirMove();
			}
			else if (pm->waterlevel > 1)
			{
				if (pm->watertype & CONTENTS_LADDER)
				{
					// begin climbing
					PM_LadderMove();
				}
				else
				{
					// begin swimming
					PM_WaterMove();
				}
			}
			else if (pml.walking)
			{
				// walking on ground
				PM_WalkMove();
			}
			else
			{
				// airborne
				PM_AirMove();
			}
		}
	}

	if (!noAnimate)
	{
		PM_BotGesture();
	}

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();

	if (pm_flying == FLY_HOVER)
	{
		//never stick to the ground
		PM_HoverTrace();
	}

	PM_SetWaterLevel();

	if (pm->cmd.forcesel != (byte)-1 && pm->ps->fd.forcePowersKnown & 1 << pm->cmd.forcesel)
	{
		pm->ps->fd.forcePowerSelected = pm->cmd.forcesel;
	}
	if (pm->cmd.invensel != (byte)-1 && pm->ps->stats[STAT_HOLDABLE_ITEMS] & 1 << pm->cmd.invensel)
	{
		pm->ps->stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(pm->cmd.invensel, IT_HOLDABLE);
	}

	if (pm->ps->m_iVehicleNum
		&& pm->ps->clientNum < MAX_CLIENTS)
	{
		//a client riding a vehicle
		if (pm->ps->eFlags & EF_NODRAW)
		{
			//inside the vehicle, do nothing
		}
		else if (!PM_WeaponOkOnVehicle(pm->cmd.weapon) || !PM_WeaponOkOnVehicle(pm->ps->weapon))
		{
			//this weapon is not legal for the vehicle, force to our current one
			if (!PM_WeaponOkOnVehicle(pm->ps->weapon))
			{
				//uh-oh!
				const int weap = PM_GetOkWeaponForVehicle();

				if (weap != -1)
				{
					pm->cmd.weapon = weap;
					pm->ps->weapon = weap;
				}
			}
			else
			{
				pm->cmd.weapon = pm->ps->weapon;
			}
		}
	}

	if (!pm->ps->m_iVehicleNum //not a vehicle and not riding one
		|| pm_entSelf->s.NPC_class == CLASS_VEHICLE //you are a vehicle NPC
		|| !(pm->ps->eFlags & EF_NODRAW) && PM_WeaponOkOnVehicle(pm->cmd.weapon))
		//you're not inside the vehicle and the weapon you're holding can be used when riding this vehicle
	{
		//only run weapons if a valid weapon is selected
		// weapons
		PM_Weapon();
	}

	PM_Use();

	if (!pm->ps->m_iVehicleNum &&
		(pm->ps->clientNum < MAX_CLIENTS ||
			!pm_entSelf ||
			pm_entSelf->s.NPC_class != CLASS_VEHICLE))
	{
		//don't do this if we're on a vehicle, or we are one
		// footstep events / legs animations
		PM_Footsteps();
	}
	else
	{
		PM_CheckInVehicleSaberAttackAnim();
	}

	// entering / leaving water splashes
	PM_WaterEvents();

	// snap velocity to integer coordinates to save network bandwidth
	if (!pm->pmove_float)
		trap->SnapVector(pm->ps->velocity);

	if (pm->ps->pm_type == PM_JETPACK || gPMDoSlowFall)
	{
		pm->ps->gravity = savedGravity;
	}

	if ( //pm->ps->m_iVehicleNum &&
		pm->ps->clientNum >= MAX_CLIENTS &&
		pm_entSelf &&
		pm_entSelf->s.NPC_class == CLASS_VEHICLE)
	{
		bgEntity_t* veh = pm_entSelf;

		assert(veh->m_pVehicle);

		//this could be kind of "inefficient" because it's called after every passenger pmove too.
		//Maybe instead of AttachRiders we should have each rider call attach for himself?
		if (veh->m_pVehicle && veh->ghoul2)
		{
			veh->m_pVehicle->m_pVehicleInfo->AttachRiders(veh->m_pVehicle);
		}
	}

	if (pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&& pm->ps->m_iVehicleNum)
	{
		//riding a vehicle, see if we should do some anim overrides
		PM_VehicleWeaponAnimate();
	}
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove(pmove_t* pmove)
{
	pm = pmove;

	const int finalTime = pmove->cmd.serverTime;

	if (finalTime < pmove->ps->commandTime)
	{
		return; // should not happen
	}

	if (finalTime > pmove->ps->commandTime + 1000)
	{
		pmove->ps->commandTime = finalTime - 1000;
	}

	if (pmove->ps->fallingToDeath)
	{
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
		pmove->cmd.buttons = 0;
	}

	pmove->ps->pmove_framecount = pmove->ps->pmove_framecount + 1 & (1 << PS_PMOVEFRAMECOUNTBITS) - 1;

	// disable attacks when using grappling hook
	if (pmove->cmd.buttons & BUTTON_GRAPPLE)
	{
		pmove->cmd.buttons &= ~BUTTON_ATTACK;
		pmove->cmd.buttons &= ~BUTTON_ALT_ATTACK;
	}

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while (pmove->ps->commandTime != finalTime)
	{
		int msec = finalTime - pmove->ps->commandTime;

		if (pmove->pmove_fixed)
		{
			if (msec > pmove->pmove_msec)
			{
				msec = pmove->pmove_msec;
			}
		}
		else
		{
			if (msec > 66)
			{
				msec = 66;
			}
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;

		PmoveSingle(pmove);

		if (pmove->ps->pm_flags & PMF_JUMP_HELD)
		{
			pmove->cmd.upmove = 20;
		}
	}
	if (pm->cmd.buttons & BUTTON_ATTACK)
	{
		pm->ps->pm_flags |= PMF_ATTACK_HELD;
	}
	else
	{
		pm->ps->pm_flags &= ~PMF_ATTACK_HELD;
	}
	if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
	{
		pm->ps->pm_flags |= PMF_ALT_ATTACK_HELD;
	}
	else
	{
		pm->ps->pm_flags &= ~PMF_ALT_ATTACK_HELD;
	}
	if (pm->cmd.buttons & BUTTON_DASH && !(pm->cmd.buttons & BUTTON_KICK))
	{
		pm->ps->pm_flags |= PMF_DASH_HELD;
	}
	else
	{
		pm->ps->pm_flags &= ~PMF_DASH_HELD;
	}

	if (pm->cmd.buttons & BUTTON_BLOCK)
	{
		pm->ps->pm_flags |= PMF_BLOCK_HELD;
	}
	else
	{
		pm->ps->pm_flags &= ~PMF_BLOCK_HELD;
	}

	if (pm->cmd.buttons & BUTTON_KICK && !(pm->cmd.buttons & BUTTON_DASH))
	{
		pm->ps->pm_flags |= PMF_KICK_HELD;
	}
	else
	{
		pm->ps->pm_flags &= ~PMF_KICK_HELD;
	}

	if (pm->cmd.buttons & BUTTON_WALKING && pm->cmd.buttons & BUTTON_BLOCK && pm->cmd.buttons & BUTTON_ATTACK)
	{
		pm->ps->pm_flags |= PMF_ACCURATE_MISSILE_BLOCK_HELD;
	}
	else
	{
		pm->ps->pm_flags &= ~PMF_ACCURATE_MISSILE_BLOCK_HELD;
	}
}

int G_MinGetUpTime(playerState_t* ps)
{
	const bgEntity_t* pEnt = pm_entSelf;
	const int npcget_up_time = NPC_KNOCKDOWN_HOLD_EXTRA_TIME;

	if (ps->legsAnim == BOTH_PLAYER_PA_3_FLY
		|| ps->legsAnim == BOTH_LK_DL_ST_T_SB_1_L
		|| ps->legsAnim == BOTH_RELEASED)
	{
		return 200;
	}
	if (pEnt->s.NPC_class == CLASS_ALORA)
	{
		return 1000;
	}
	if (pEnt->s.NPC_class == CLASS_STORMTROOPER)
	{
		return npcget_up_time + 100;
	}
	if (pEnt->s.NPC_class == CLASS_CLONETROOPER)
	{
		return npcget_up_time + 100;
	}
	if (pEnt->s.NPC_class == CLASS_STORMCOMMANDO)
	{
		return npcget_up_time + 100;
	}

#ifdef _GAME
	if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT)
	{
		if (pm->ps->weapon == WP_SABER) //saber out
		{
			const int jedi_bot_get_up_time = BOT_KNOCKDOWN_HOLD_EXTRA_TIME;
			return jedi_bot_get_up_time + 400;
		}
		const int gunner_get_up_time = PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME;
		return gunner_get_up_time + 100;
	}
#endif
	if (ps->clientNum < MAX_CLIENTS)
	{
		const int get_up_time = PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME;

		if (ps->fd.forcePowerLevel[FP_LEVITATION] >= FORCE_LEVEL_3)
		{
			return get_up_time + 400;
		}
		if (ps->fd.forcePowerLevel[FP_LEVITATION] == FORCE_LEVEL_2)
		{
			return get_up_time + 200;
		}
		if (ps->fd.forcePowerLevel[FP_LEVITATION] <= FORCE_LEVEL_2)
		{
			return get_up_time + 100;
		}
		return get_up_time;
	}
	return 200;
}

qboolean PM_InAttackRoll(const int anim)
{
	//racc - anim in a melee attack roll.
	switch (anim)
	{
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
		return qtrue;
	default:;
	}
	return qfalse;
}

static qboolean PM_CheckRollSafety(const int anim, const float testDist)
{
	vec3_t forward;
	vec3_t right;
	vec3_t testPos;
	vec3_t angles;
	trace_t trace;
	int contents = CONTENTS_SOLID | CONTENTS_BOTCLIP;

	if (pm->ps->clientNum < MAX_CLIENTS)
	{
		//player
		contents |= CONTENTS_PLAYERCLIP;
	}
	else
	{
		//NPC
		contents |= CONTENTS_MONSTERCLIP;
	}
	if (PM_InAttackRoll(pm->ps->legsAnim))
	{
		//we don't care if people are in the way, we'll knock them down!
		contents &= ~CONTENTS_BODY;
	}
	angles[PITCH] = angles[ROLL] = 0;
	angles[YAW] = pm->ps->viewangles[YAW]; //Add ucmd.angles[YAW]?
	AngleVectors(angles, forward, right, NULL);

	switch (anim)
	{
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_R:
		VectorMA(pm->ps->origin, testDist, right, testPos);
		break;
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_FROLL_L:
		VectorMA(pm->ps->origin, -testDist, right, testPos);
		break;
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_FROLL_F:
		VectorMA(pm->ps->origin, testDist, forward, testPos);
		break;
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_FROLL_B:
		VectorMA(pm->ps->origin, -testDist, forward, testPos);
		break;
	default: //FIXME: add normal rolls?  Make generic for any forced-movement anim?
		return qtrue;
	}

	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, testPos, pm->ps->clientNum, contents);
	if (trace.fraction < 1.0f
		|| trace.allsolid
		|| trace.startsolid)
	{
		//inside something or will hit something
		return qfalse;
	}
	return qtrue;
}

extern qboolean BG_StabDownAnim(int anim);

qboolean PM_GoingToAttackDown(const playerState_t* ps)
{
	//racc - is the given ps in an animation that is about to attack the ground?
	if (BG_StabDownAnim(ps->torsoAnim) //stabbing downward
		|| ps->saber_move == LS_A_LUNGE //lunge
		|| ps->saber_move == LS_A_JUMP_T__B_ //death from above
		|| ps->saber_move == LS_A_JUMP_PALP_ //death from above
		|| ps->saber_move == LS_A_T2B //attacking top to bottom
		|| ps->saber_move == LS_S_T2B //starting at attack downward
		|| PM_SaberInTransition(ps->saber_move) && saber_moveData[ps->saber_move].endQuad == Q_T)
		//transitioning to a top to bottom attack
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_ForceUsingSaberAnim(const int anim)
{
	//saber/acrobatic anims that should prevent you from recharging force power while you're in them...
	switch (anim)
	{
	case BOTH_JUMPFLIPSLASHDOWN1:
	case BOTH_JUMPFLIPSTABDOWN:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_FORCELEAP_PALP:
	case BOTH_JUMPATTACK6:
	case BOTH_GRIEVOUS_LUNGE:
	case BOTH_JUMPATTACK7:
	case BOTH_FORCELONGLEAP_START:
	case BOTH_FORCELONGLEAP_ATTACK:
	case BOTH_FORCELONGLEAP_ATTACK2:
	case BOTH_FORCEWALLRUNFLIP_START:
	case BOTH_FORCEWALLRUNFLIP_END:
	case BOTH_FORCEWALLRUNFLIP_ALT:
	case BOTH_FORCEWALLREBOUND_FORWARD:
	case BOTH_FORCEWALLREBOUND_LEFT:
	case BOTH_FORCEWALLREBOUND_BACK:
	case BOTH_FORCEWALLREBOUND_RIGHT:
	case BOTH_FLIP_ATTACK7:
	case BOTH_FLIP_HOLD7:
	case BOTH_FLIP_LAND:
	case BOTH_PULL_IMPALE_STAB:
	case BOTH_PULL_IMPALE_SWING:
	case BOTH_A6_SABERPROTECT:
	case BOTH_A7_SOULCAL:
	case BOTH_A1_SPECIAL:
	case BOTH_A2_SPECIAL:
	case BOTH_A3_SPECIAL:
	case BOTH_A4_SPECIAL:
	case BOTH_A5_SPECIAL:
	case BOTH_GRIEVOUS_SPIN:
	case BOTH_GRIEVOUS_PROTECT:
	case BOTH_YODA_SPECIAL:
	case BOTH_ARIAL_LEFT:
	case BOTH_ARIAL_RIGHT:
	case BOTH_CARTWHEEL_LEFT:
	case BOTH_CARTWHEEL_RIGHT:
	case BOTH_FLIP_LEFT:
	case BOTH_FLIP_BACK1:
	case BOTH_FLIP_BACK2:
	case BOTH_FLIP_BACK3:
	case BOTH_ALORA_FLIP_B_MD2:
	case BOTH_ALORA_FLIP_B:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_WALL_RUN_RIGHT:
	case BOTH_WALL_RUN_RIGHT_FLIP:
	case BOTH_WALL_RUN_RIGHT_STOP:
	case BOTH_WALL_RUN_LEFT:
	case BOTH_WALL_RUN_LEFT_FLIP:
	case BOTH_WALL_RUN_LEFT_STOP:
	case BOTH_WALL_FLIP_RIGHT:
	case BOTH_WALL_FLIP_LEFT:
	case BOTH_FORCEJUMP1:
	case BOTH_FORCEJUMP2:
	case BOTH_FORCEINAIR1:
	case BOTH_FORCELAND1:
	case BOTH_FORCEJUMPBACK1:
	case BOTH_FORCEINAIRBACK1:
	case BOTH_FORCELANDBACK1:
	case BOTH_FORCEJUMPLEFT1:
	case BOTH_FORCEINAIRLEFT1:
	case BOTH_FORCELANDLEFT1:
	case BOTH_FORCEJUMPRIGHT1:
	case BOTH_FORCEINAIRRIGHT1:
	case BOTH_FORCELANDRIGHT1:
	case BOTH_FLIP_F:
	case BOTH_FLIP_B:
	case BOTH_FLIP_L:
	case BOTH_FLIP_R:
	case BOTH_ALORA_FLIP_1_MD2:
	case BOTH_ALORA_FLIP_2_MD2:
	case BOTH_ALORA_FLIP_3_MD2:
	case BOTH_ALORA_FLIP_1:
	case BOTH_ALORA_FLIP_2:
	case BOTH_ALORA_FLIP_3:
	case BOTH_DODGE_FL:
	case BOTH_DODGE_FR:
	case BOTH_DODGE_BL:
	case BOTH_DODGE_BR:
	case BOTH_DODGE_L:
	case BOTH_DODGE_R:
	case BOTH_DODGE_HOLD_FL:
	case BOTH_DODGE_HOLD_FR:
	case BOTH_DODGE_HOLD_BL:
	case BOTH_DODGE_HOLD_BR:
	case BOTH_DODGE_HOLD_L:
	case BOTH_DODGE_HOLD_R:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_FORCE_GETUP_B6:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
	case BOTH_WALL_FLIP_BACK1:
	case BOTH_WALL_FLIP_BACK2:
	case BOTH_SPIN1:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_DEFLECTSLASH__R__L_FIN:
	case BOTH_ARIAL_F1:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean PM_LockedAnim(int anim);

static qboolean PM_CrouchGetup(const float crouchheight)
{
	//racc - recover from our current knockdown state into a crouch.
	//This should work as long as we're in a known knockdown state.
	int anim = -1;
	pm->maxs[2] = crouchheight;
	pm->ps->viewheight = crouchheight + STANDARD_VIEWHEIGHT_OFFSET;

	switch (pm->ps->legsAnim)
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN4:
	case BOTH_RELEASED:
	case BOTH_PLAYER_PA_3_FLY:
		anim = BOTH_GETUP_CROUCH_B1;
		break;
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN5:
	case BOTH_SLAPDOWNRIGHT:
	case BOTH_SLAPDOWNLEFT:
	case BOTH_LK_DL_ST_T_SB_1_L:
		anim = BOTH_GETUP_CROUCH_F1;
		break;
	default:;
	}
	if (anim == -1)
	{
		//WTF? stay down?
		pm->ps->legsTimer = 100; //hold this anim for another 10th of a second
		return qfalse;
	}
	//get up into crouch anim
	if (PM_LockedAnim(pm->ps->torsoAnim))
	{
		//need to be able to override this anim
		pm->ps->torsoTimer = 0;
	}
	if (PM_LockedAnim(pm->ps->legsAnim))
	{
		//need to be able to override this anim
		pm->ps->legsTimer = 0;
	}
	PM_SetAnim(SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS);
	pm->ps->saber_move = pm->ps->saberBounceMove = LS_READY; //don't finish whatever saber anim you may have been in
	pm->ps->saberBlocked = BLOCKED_NONE;
	return qtrue;
}

extern qboolean PM_LockedAnim(int anim);

static qboolean PM_CheckRollGetup(void)
{
	//racc - try getting up from a knockdown by using a getup roll move.
#ifdef _GAME
	gentity_t* self = &g_entities[pm->ps->clientNum];
#endif
	if (pm->ps->legsAnim == BOTH_KNOCKDOWN1
		|| pm->ps->legsAnim == BOTH_KNOCKDOWN2
		|| pm->ps->legsAnim == BOTH_KNOCKDOWN3
		|| pm->ps->legsAnim == BOTH_KNOCKDOWN4
		|| pm->ps->legsAnim == BOTH_KNOCKDOWN5
		|| pm->ps->legsAnim == BOTH_SLAPDOWNRIGHT
		|| pm->ps->legsAnim == BOTH_SLAPDOWNLEFT
		|| pm->ps->legsAnim == BOTH_LK_DL_ST_T_SB_1_L
		|| pm->ps->legsAnim == BOTH_PLAYER_PA_3_FLY
		|| pm->ps->legsAnim == BOTH_RELEASED)
	{
		//lying on back or front
		if (pm->ps->clientNum < MAX_CLIENTS //player
			&& !(pm->ps->userInt3 & 1 << FLAG_FATIGUED) //can't do roll getups while fatigued.
			&& (pm->cmd.rightmove //pressing left or right
				|| pm->cmd.forwardmove && pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0)
			//or pressing fwd/back and have force jump.
#ifdef _GAME
			|| pm->ps->clientNum >= MAX_CLIENTS
			&& self->NPC //an NPC
			&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 //have at least force jump 1
			&& self->enemy //I have an enemy
			&& self->enemy->client //a client
			&& self->enemy->enemy == self //he's mad at me!
			&& (PM_GoingToAttackDown(&self->enemy->client->ps) || !Q_irand(0, 2))
			//he's attacking downward! (or we just feel like doing it this time)
			&& (self->client && self->client->NPC_class == CLASS_ALORA || Q_irand(0, RANK_CAPTAIN) < self->NPC->rank)
			//higher rank I am, more likely I am to roll away!
#endif
			)
		{
			//roll away!
			int anim;
			qboolean forceGetUp = qfalse;
			if (pm->cmd.forwardmove > 0)
			{
				if (pm->ps->legsAnim == BOTH_KNOCKDOWN3
					|| pm->ps->legsAnim == BOTH_KNOCKDOWN5
					|| pm->ps->legsAnim == BOTH_SLAPDOWNRIGHT
					|| pm->ps->legsAnim == BOTH_SLAPDOWNLEFT
					|| pm->ps->legsAnim == BOTH_LK_DL_ST_T_SB_1_L)
				{
					anim = BOTH_GETUP_FROLL_F;
				}
				else
				{
					anim = BOTH_GETUP_BROLL_F;
				}
				forceGetUp = qtrue;
			}
			else if (pm->cmd.forwardmove < 0)
			{
				if (pm->ps->legsAnim == BOTH_KNOCKDOWN3
					|| pm->ps->legsAnim == BOTH_KNOCKDOWN5
					|| pm->ps->legsAnim == BOTH_SLAPDOWNRIGHT
					|| pm->ps->legsAnim == BOTH_SLAPDOWNLEFT
					|| pm->ps->legsAnim == BOTH_LK_DL_ST_T_SB_1_L)
				{
					anim = BOTH_GETUP_FROLL_B;
				}
				else
				{
					anim = BOTH_GETUP_BROLL_B;
				}
				forceGetUp = qtrue;
			}
			else if (pm->cmd.rightmove > 0)
			{
				if (pm->ps->legsAnim == BOTH_KNOCKDOWN3
					|| pm->ps->legsAnim == BOTH_KNOCKDOWN5
					|| pm->ps->legsAnim == BOTH_SLAPDOWNRIGHT
					|| pm->ps->legsAnim == BOTH_SLAPDOWNLEFT
					|| pm->ps->legsAnim == BOTH_LK_DL_ST_T_SB_1_L)
				{
					anim = BOTH_GETUP_FROLL_R;
				}
				else
				{
					anim = BOTH_GETUP_BROLL_R;
				}
			}
			else if (pm->cmd.rightmove < 0)
			{
				if (pm->ps->legsAnim == BOTH_KNOCKDOWN3
					|| pm->ps->legsAnim == BOTH_KNOCKDOWN5
					|| pm->ps->legsAnim == BOTH_SLAPDOWNRIGHT
					|| pm->ps->legsAnim == BOTH_SLAPDOWNLEFT
					|| pm->ps->legsAnim == BOTH_LK_DL_ST_T_SB_1_L)
				{
					anim = BOTH_GETUP_FROLL_L;
				}
				else
				{
					anim = BOTH_GETUP_BROLL_L;
				}
			}
			else
			{
				//racc - If no move, then randomly select a roll move.  This only only works for NPCs.
				if (pm->ps->legsAnim == BOTH_KNOCKDOWN3
					|| pm->ps->legsAnim == BOTH_KNOCKDOWN5
					|| pm->ps->legsAnim == BOTH_SLAPDOWNRIGHT
					|| pm->ps->legsAnim == BOTH_SLAPDOWNLEFT
					|| pm->ps->legsAnim == BOTH_LK_DL_ST_T_SB_1_L)
				{
					//on your front
					anim = PM_irand_timesync(BOTH_GETUP_FROLL_B, BOTH_GETUP_FROLL_R);
				}
				else
				{
					anim = PM_irand_timesync(BOTH_GETUP_BROLL_B, BOTH_GETUP_BROLL_R);
				}
			}

			if (pm->ps->clientNum >= MAX_CLIENTS)
			{
				//racc - NPCs do roll safety checks to make sure they can safely roll in that direction.
				if (!PM_CheckRollSafety(anim, 64))
				{
					//oops, try other one
					if (pm->ps->legsAnim == BOTH_KNOCKDOWN3
						|| pm->ps->legsAnim == BOTH_KNOCKDOWN5
						|| pm->ps->legsAnim == BOTH_SLAPDOWNRIGHT
						|| pm->ps->legsAnim == BOTH_SLAPDOWNLEFT
						|| pm->ps->legsAnim == BOTH_LK_DL_ST_T_SB_1_L)
					{
						if (anim == BOTH_GETUP_FROLL_R)
						{
							anim = BOTH_GETUP_FROLL_L;
						}
						else if (anim == BOTH_GETUP_FROLL_F)
						{
							anim = BOTH_GETUP_FROLL_B;
						}
						else if (anim == BOTH_GETUP_FROLL_B)
						{
							anim = BOTH_GETUP_FROLL_F;
						}
						else
						{
							anim = BOTH_GETUP_FROLL_L;
						}
						if (!PM_CheckRollSafety(anim, 64))
						{
							//neither side is clear, screw it
							return qfalse;
						}
					}
					else
					{
						if (anim == BOTH_GETUP_BROLL_R)
						{
							anim = BOTH_GETUP_BROLL_L;
						}
						else if (anim == BOTH_GETUP_BROLL_F)
						{
							anim = BOTH_GETUP_BROLL_B;
						}
						else if (anim == BOTH_GETUP_FROLL_B)
						{
							anim = BOTH_GETUP_BROLL_F;
						}
						else
						{
							anim = BOTH_GETUP_BROLL_L;
						}
						if (!PM_CheckRollSafety(anim, 64))
						{
							//neither side is clear, screw it
							return qfalse;
						}
					}
				}
			}
			pm->cmd.rightmove = pm->cmd.forwardmove = 0;
			if (PM_LockedAnim(pm->ps->torsoAnim))
			{
				//need to be able to override this anim
				pm->ps->torsoTimer = 0;
			}
			if (PM_LockedAnim(pm->ps->legsAnim))
			{
				//need to be able to override this anim
				pm->ps->legsTimer = 0;
			}
			PM_SetAnim(SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS);
			pm->ps->weaponTime = pm->ps->torsoTimer - 300; //don't attack until near end of this anim
			pm->ps->saber_move = pm->ps->saberBounceMove = LS_READY;
			//don't finish whatever saber anim you may have been in
			pm->ps->saberBlocked = BLOCKED_NONE;
			if (forceGetUp)
			{
#ifdef _GAME
				if (self && self->client && self->client->playerTeam == NPCTEAM_ENEMY
					&& self->NPC && self->NPC->blockedSpeechDebounceTime < level.time
					&& !Q_irand(0, 1))
				{
					//racc - evil NPCs sometimes taunt when they use the force to jump up from a knockdown.
					PM_AddEvent(Q_irand(EV_COMBAT1, EV_COMBAT3));
					self->NPC->blockedSpeechDebounceTime = level.time + 1000;
				}
				if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
				{
					//short burst
					G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/jumpsmall.mp3"));
				}
				else
				{
					//holding it
					G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);
				}
#endif
				//launch off ground?
				pm->ps->weaponTime = 300; //just to make sure it's cleared
			}
			return qtrue;
		}
	}
	return qfalse;
}

qboolean PM_GettingUpFromKnockDown(const float standheight, const float crouchheight)
{
	const bgEntity_t* pEnt = pm_entSelf;

	const int legsAnim = pm->ps->legsAnim;

	if (legsAnim == BOTH_KNOCKDOWN1
		|| legsAnim == BOTH_KNOCKDOWN2
		|| legsAnim == BOTH_KNOCKDOWN3
		|| legsAnim == BOTH_KNOCKDOWN4
		|| legsAnim == BOTH_KNOCKDOWN5
		|| legsAnim == BOTH_SLAPDOWNRIGHT
		|| legsAnim == BOTH_SLAPDOWNLEFT
		|| legsAnim == BOTH_PLAYER_PA_3_FLY
		|| legsAnim == BOTH_LK_DL_ST_T_SB_1_L
		|| legsAnim == BOTH_RELEASED)
	{
		//in a knockdown
		const int minTimeLeft = G_MinGetUpTime(pm->ps);

		if (pm->ps->legsTimer <= minTimeLeft)
		{
			//if only a quarter of a second left, allow roll-aways
			if (PM_CheckRollGetup())
			{
				//racc - decided to use a getup roll.
				pm->cmd.rightmove = pm->cmd.forwardmove = 0;
				return qtrue;
			}
		}
#ifdef _GAME
		if (TIMER_Exists(&g_entities[pm->ps->clientNum], "noGetUpStraight"))
		{
			//racc - check for a npc don't-getup-right-now timer for this NPC.
			if (!TIMER_Done2(&g_entities[pm->ps->clientNum], "noGetUpStraight", qtrue))
			{
				//not allowed to do straight get-ups for another few seconds
				if (pm->ps->legsTimer <= minTimeLeft)
				{
					//hold it for a bit
					pm->ps->legsTimer = minTimeLeft + 1;
				}
			}
		}
#endif
		if (!pm->ps->legsTimer //our knockdown is over
			|| pm->ps->legsTimer <= minTimeLeft //or we're strong enough to get up earlier.
			&& (pm->cmd.upmove > 0 || pEnt->s.NPC_class == CLASS_ALORA)) //and we're trying to get up
		{
			//done with the knockdown - FIXME: somehow this is allowing an *instant* getup...???
			if (pm->cmd.upmove < 0)
			{
				return PM_CrouchGetup(crouchheight);
			}
			trace_t trace;
			// try to stand up
			pm->maxs[2] = standheight;
			pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask);

			if (!trace.allsolid)
			{
				//stand up
				int anim = BOTH_GETUP1;
				qboolean forceGetUp = qfalse;
				pm->maxs[2] = standheight;
				pm->ps->viewheight = standheight + STANDARD_VIEWHEIGHT_OFFSET;

				switch (pm->ps->legsAnim)
				{
				case BOTH_KNOCKDOWN1:
					if (pm->ps->clientNum && pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 || pm->ps->
						clientNum < MAX_CLIENTS && !(pm->ps->userInt3 & 1 << FLAG_FATIGUED)
						&& pm->cmd.upmove > 0
						&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0)
					{
						anim = PM_irand_timesync(BOTH_FORCE_GETUP_B1, BOTH_FORCE_GETUP_B6);
						forceGetUp = qtrue;
					}
					else
					{
						anim = BOTH_GETUP1;
					}
					break;
				case BOTH_KNOCKDOWN2:
				case BOTH_PLAYER_PA_3_FLY:
					if (pm->ps->clientNum && pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 || pm->ps->
						clientNum < MAX_CLIENTS && !(pm->ps->userInt3 & 1 << FLAG_FATIGUED)
						&& pm->cmd.upmove > 0
						&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0)
					{
						anim = PM_irand_timesync(BOTH_FORCE_GETUP_B1, BOTH_FORCE_GETUP_B6);
						//NOTE: BOTH_FORCE_GETUP_B5 takes soe steps forward at end
						forceGetUp = qtrue;
					}
					else
					{
						anim = BOTH_GETUP2;
					}
					break;
				case BOTH_KNOCKDOWN3:
					if (pm->ps->clientNum && pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 || pm->ps->
						clientNum < MAX_CLIENTS && !(pm->ps->userInt3 & 1 << FLAG_FATIGUED)
						&& pm->cmd.upmove > 0
						&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0)
					{
						anim = PM_irand_timesync(BOTH_FORCE_GETUP_F1, BOTH_FORCE_GETUP_F2);
						forceGetUp = qtrue;
					}
					else
					{
						anim = BOTH_GETUP3;
					}
					break;
				case BOTH_KNOCKDOWN4:
				case BOTH_RELEASED:
					if (pm->ps->clientNum && pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 || pm->ps->
						clientNum < MAX_CLIENTS && !(pm->ps->userInt3 & 1 << FLAG_FATIGUED)
						&& pm->cmd.upmove > 0
						&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0)
					{
						anim = PM_irand_timesync(BOTH_FORCE_GETUP_B1, BOTH_FORCE_GETUP_B6);
						//NOTE: BOTH_FORCE_GETUP_B5 takes soe steps forward at end
						forceGetUp = qtrue;
					}
					else
					{
						anim = BOTH_GETUP4;
					}
					break;
				case BOTH_KNOCKDOWN5:
				case BOTH_SLAPDOWNRIGHT:
				case BOTH_SLAPDOWNLEFT:
				case BOTH_LK_DL_ST_T_SB_1_L:
					if (pm->ps->clientNum && pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 || pm->ps->
						clientNum < MAX_CLIENTS && !(pm->ps->userInt3 & 1 << FLAG_FATIGUED)
						&& pm->cmd.upmove > 0
						&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0)
					{
						anim = PM_irand_timesync(BOTH_FORCE_GETUP_F1, BOTH_FORCE_GETUP_F2);
						forceGetUp = qtrue;
					}
					else
					{
						anim = BOTH_GETUP5;
					}
					break;
				default:;
				}
				if (forceGetUp)
				{
#ifdef _GAME
					gentity_t* self = &g_entities[pm->ps->clientNum];
					if (self && self->client && self->client->playerTeam == NPCTEAM_ENEMY
						&& self->NPC && self->NPC->blockedSpeechDebounceTime < level.time
						&& !Q_irand(0, 1))
					{
						//racc - enemy bots talk a little smack if they
						PM_AddEvent(Q_irand(EV_COMBAT1, EV_COMBAT3));
						self->NPC->blockedSpeechDebounceTime = level.time + 1000;
					}
					if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
					{
						//short burst
						G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/jumpsmall.mp3"));
					}
					else
					{
						//holding it
						G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);
					}
#endif
					//launch off ground?
					pm->ps->weaponTime = 300; //just to make sure it's cleared
				}
				if (PM_LockedAnim(pm->ps->torsoAnim))
				{
					//need to be able to override this anim
					pm->ps->torsoTimer = 0;
				}
				if (PM_LockedAnim(pm->ps->legsAnim))
				{
					//need to be able to override this anim
					pm->ps->legsTimer = 0;
				}
				PM_SetAnim(SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS);
				pm->ps->saber_move = pm->ps->saberBounceMove = LS_READY;
				//don't finish whatever saber anim you may have been in
				pm->ps->saberBlocked = BLOCKED_NONE;
				return qtrue;
			}
			return PM_CrouchGetup(crouchheight);
		}
		if (pm->ps->legsAnim == BOTH_LK_DL_ST_T_SB_1_L)
		{
			//racc - apprenently this move has a special cmd for it.
			PM_CmdForRoll(pm->ps, pm->ps->legsAnim, &pm->cmd);
		}
		else
		{
			pm->cmd.rightmove = pm->cmd.forwardmove = 0;
		}
	}
	return qfalse;
}