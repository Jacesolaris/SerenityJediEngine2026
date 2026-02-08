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

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "w_saber.h"

#ifdef _GAME
#include "g_local.h"
extern stringID_table_t animTable[MAX_ANIMATIONS + 1];
extern stringID_table_t saber_moveTable[];
#endif

extern qboolean BG_SabersOff(const playerState_t* ps);
saberInfo_t* BG_MySaber(int clientNum, int saberNum);
extern qboolean PM_SaberInDamageMove(int move);
void PM_AddFatigue(playerState_t* ps, int fatigue);
extern qboolean PM_InCartwheel(int anim);
extern qboolean PM_SaberInDeflect(int move);
extern void PM_AddEventWithParm(int new_event, int parm);
extern qboolean PM_CrouchingAnim(int anim);
extern qboolean PM_MeleeblockHoldAnim(int anim);
extern qboolean PM_MeleeblockAnim(int anim);
extern qboolean PM_BoltBlockingAnim(int anim);
extern qboolean PM_SaberInSpecial(int move);
saber_moveName_t PM_SaberLungeAttackMove(qboolean noSpecials);
extern qboolean ValidAnimFileIndex(int index);
extern qboolean PM_InOnGroundAnims(const playerState_t* ps);
extern qboolean PM_LockedAnim(int anim);
extern qboolean PM_SaberInReturn(int move);
qboolean PM_WalkingAnim(int anim);
qboolean PM_SwimmingAnim(int anim);
extern saber_moveName_t PM_SaberBounceForAttack(int move);
qboolean PM_SuperBreakLoseAnim(int anim);
qboolean PM_SuperBreakWinAnim(int anim);
extern bgEntity_t* pm_entSelf;
extern saber_moveName_t PM_KnockawayForParry(int move);
extern qboolean PM_SaberInbackblock(int move);
extern qboolean PM_BlockAnim(int anim);
extern qboolean PM_BlockHoldAnim(int anim);
extern qboolean PM_BlockDualAnim(int anim);
extern qboolean PM_BlockHoldDualAnim(int anim);
extern qboolean PM_BlockStaffAnim(int anim);
extern qboolean PM_BlockHoldStaffAnim(int anim);
extern qboolean PM_SaberInTransition(int move);
extern qboolean PM_SaberInTransitionAny(int move);
extern qboolean PM_InSlopeAnim(int anim);
extern qboolean PM_RunningAnim(int anim);
extern qboolean PM_SaberInAttackPure(int move);
extern saber_moveName_t pm_broken_parry_for_attack(int move);
extern qboolean PM_BounceAnim(int anim);
extern qboolean PM_SaberReturnAnim(int anim);
extern saber_moveName_t pm_block_the_attack(int move);
void PM_AddBlockFatigue(playerState_t* ps, int fatigue);
extern qboolean PM_IsInBlockingAnim(int move);
extern qboolean PM_SaberDoDamageAnim(int anim);
extern qboolean in_camera;
extern qboolean PM_Can_Do_Kill_Move(void);
extern qboolean PM_SaberInMassiveBounce(int anim);
int Next_Kill_Attack_Move_Check[MAX_CLIENTS]; // Next special move check.
extern vmCvar_t g_attackskill;
extern vmCvar_t bot_thinklevel;
saber_moveName_t PM_DoAI_Fake(const int curmove);
qboolean PM_Can_Do_Kill_Lunge(void);
qboolean PM_Can_Do_Kill_Lunge_back(void);
int PM_SaberBackflipAttackMove(void);
saber_moveName_t PM_NPC_Force_Leap_Attack(void);

int PM_irand_timesync(const int val1, const int val2)
{
	int i = val1 - 1 + Q_random(&pm->cmd.serverTime) * (val2 - val1) + 1;
	if (i < val1)
	{
		i = val1;
	}
	if (i > val2)
	{
		i = val2;
	}

	return i;
}

void WP_ForcePowerDrain(playerState_t* ps, const forcePowers_t force_power, const int override_amt)
{
	//take away the power
	int drain = override_amt;

	if (!drain)
	{
		if (ps->fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_1 &&
			ps->fd.forcePowersActive & 1 << FP_SPEED)
		{
			drain = forcePowerNeededlevel1[ps->fd.forcePowerLevel[force_power]][force_power];
		}
		else if (ps->fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_2 &&
			ps->fd.forcePowersActive & 1 << FP_SPEED)
		{
			drain = forcePowerNeededlevel2[ps->fd.forcePowerLevel[force_power]][force_power];
		}
		else if (ps->fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_3 &&
			ps->fd.forcePowersActive & 1 << FP_SPEED)
		{
			drain = forcePowerNeeded[ps->fd.forcePowerLevel[force_power]][force_power];
		}
		else
		{
			drain = forcePowerNeeded[ps->fd.forcePowerLevel[force_power]][force_power];
		}
	}

	if (!drain)
	{
		return;
	}

	ps->fd.forcePower -= drain;

	if (ps->fd.forcePower < 0)
	{
		ps->fd.forcePower = 0;
	}

	if (ps->fd.forcePower <= ps->fd.forcePowerMax * FATIGUEDTHRESHHOLD)
	{
		//Pop the Fatigued flag
		ps->userInt3 |= 1 << FLAG_FATIGUED;
	}
}

void BG_ForcePowerKill(playerState_t* ps)
{
	if (ps->fd.forcePower < 5)
	{
		ps->fd.forcePower = 5;
	}
	if (ps->fd.forcePower <= ps->fd.forcePowerMax * FATIGUEDTHRESHHOLD)
	{
		ps->userInt3 |= 1 << FLAG_FATIGUED;
	}
	if (pm->ps->fd.forcePowersActive & 1 << FP_SPEED)
	{
		pm->ps->fd.forcePowersActive &= ~(1 << FP_SPEED);
	}
}

qboolean BG_EnoughForcePowerForMove(const int cost, const qboolean play_sound)
{
	if (pm->ps->fd.forcePower < cost)
	{
		if (play_sound)
		{
			PM_AddEvent(EV_NOAMMO);
		}
		return qfalse;
	}

	return qtrue;
}

// Silly, but I'm replacing these macros so they are shorter!
#define AFLAG_IDLE	(SETANIM_FLAG_NORMAL)
#define AFLAG_ACTIVE (SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS)
#define AFLAG_WAIT (SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS)
#define AFLAG_FINISH (SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS)

//FIXME: add the alternate anims for each style?
saber_moveData_t saber_moveData[LS_MOVE_MAX] = {
	//							NB:randomized
	// name			anim(do all styles?)startQ	endQ	setanimflag		blend,	blocking	chain_idle		chain_attack	trailLen
	{"None", BOTH_STAND1, Q_R, Q_R, AFLAG_IDLE, 350, BLK_NO, LS_NONE, LS_NONE, 0}, // LS_NONE		= 0,

	// General movements with saber
	{"Ready", BOTH_STAND2, Q_R, Q_R, AFLAG_IDLE, 350, BLK_WIDE, LS_READY, LS_S_R2L, 0}, // LS_READY,
	{"Draw", BOTH_STAND1TO2, Q_R, Q_R, AFLAG_FINISH, 350, BLK_NO, LS_READY, LS_S_R2L, 0}, // LS_DRAW,
	{"Draw2", BOTH_SABER_IGNITION, Q_R, Q_R, AFLAG_FINISH, 350, BLK_NO, LS_READY, LS_S_R2L, 0}, // LS_DRAW2,
	{"Draw3", BOTH_GRIEVOUS_SABERON, Q_R, Q_R, AFLAG_FINISH, 350, BLK_NO, LS_READY, LS_S_R2L, 0}, // LS_DRAW3,
	{"Putaway", BOTH_STAND2TO1, Q_R, Q_R, AFLAG_FINISH, 350, BLK_NO, LS_READY, LS_S_R2L, 0}, // LS_PUTAWAY,

	// Attacks
	//UL2LR
	{"TL2BR Att", BOTH_A1_TL_BR, Q_TL, Q_BR, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_R_TL2BR, LS_R_TL2BR, 200}, // LS_A_TL2BR
	//SLASH LEFT
	{"L2R Att", BOTH_A1__L__R, Q_L, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_R_L2R, LS_R_L2R, 200}, // LS_A_L2R
	//LL2UR
	{"BL2TR Att", BOTH_A1_BL_TR, Q_BL, Q_TR, AFLAG_ACTIVE, 50, BLK_TIGHT, LS_R_BL2TR, LS_R_BL2TR, 200}, // LS_A_BL2TR
	//LR2UL
	{"BR2TL Att", BOTH_A1_BR_TL, Q_BR, Q_TL, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_R_BR2TL, LS_R_BR2TL, 200}, // LS_A_BR2TL
	//SLASH RIGHT
	{"R2L Att", BOTH_A1__R__L, Q_R, Q_L, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_R_R2L, LS_R_R2L, 200}, // LS_A_R2L
	//UR2LL
	{"TR2BL Att", BOTH_A1_TR_BL, Q_TR, Q_BL, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_R_TR2BL, LS_R_TR2BL, 200}, // LS_A_TR2BL
	//SLASH DOWN
	{"T2B Att", BOTH_A1_T__B_, Q_T, Q_B, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_R_T2B, LS_R_T2B, 200}, // LS_A_T2B
	//special attacks
	{"Back Stab", BOTH_A2_STABBACK1, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_A_BACKSTAB
	{"Back Stab_b", BOTH_A2_STABBACK1B, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},
	// LS_A_BACKSTAB_B
	{"Back Att", BOTH_ATTACK_BACK, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_A_BACK
	{"CR Back Att", BOTH_CROUCHATTACKBACK1, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},
	// LS_A_BACK_CR
	{"RollStab", BOTH_ROLL_STAB, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_ROLL_STAB
	{"Lunge Att", BOTH_LUNGE2_B__T_, Q_B, Q_T, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_A_LUNGE
	{"Jump Att", BOTH_FORCELEAP2_T__B_, Q_T, Q_B, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},
	// LS_A_JUMP_T__B_
	{"Palp Att", BOTH_FORCELEAP_PALP, Q_T, Q_B, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},
	// LS_A_JUMP_PALP_
	{"Flip Stab", BOTH_JUMPFLIPSTABDOWN, Q_R, Q_T, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_T1_T___R, 200},
	// LS_A_FLIP_STAB
	{"Flip Slash", BOTH_JUMPFLIPSLASHDOWN1, Q_L, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_T1__R_T_, 200},
	// LS_A_FLIP_SLASH
	{"DualJump Atk", BOTH_JUMPATTACK6, Q_R, Q_BL, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_T1_BL_TR, 200},
	// LS_JUMPATTACK_DUAL
	{"DualJump Atkgrie", BOTH_GRIEVOUS_LUNGE, Q_R, Q_BL, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_T1_BL_TR, 200},
	// LS_GRIEVOUS_LUNGE

	{"DualJumpAtkL_A", BOTH_ARIAL_LEFT, Q_R, Q_TL, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_A_TL2BR, 200},
	// LS_JUMPATTACK_ARIAL_LEFT
	{"DualJumpAtkR_A", BOTH_ARIAL_RIGHT, Q_R, Q_TR, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_A_TR2BL, 200},
	// LS_JUMPATTACK_ARIAL_RIGHT

	{"DualJumpAtkL_A", BOTH_CARTWHEEL_LEFT, Q_R, Q_TL, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_T1_TL_BR, 200},
	// LS_JUMPATTACK_CART_LEFT
	{"DualJumpAtkR_A", BOTH_CARTWHEEL_RIGHT, Q_R, Q_TR, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_T1_TR_BL, 200},
	// LS_JUMPATTACK_CART_RIGHT

	{"DualJumpAtkLStaff", BOTH_BUTTERFLY_FL1, Q_R, Q_L, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_T1__L__R, 200},
	// LS_JUMPATTACK_STAFF_LEFT
	{"DualJumpAtkRStaff", BOTH_BUTTERFLY_FR1, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_T1__R__L, 200},
	// LS_JUMPATTACK_STAFF_RIGHT

	{"ButterflyLeft", BOTH_BUTTERFLY_LEFT, Q_R, Q_L, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_T1__L__R, 200},
	// LS_BUTTERFLY_LEFT
	{"ButterflyRight", BOTH_BUTTERFLY_RIGHT, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_T1__R__L, 200},
	// LS_BUTTERFLY_RIGHT

	{"BkFlip Atk", BOTH_JUMPATTACK7, Q_B, Q_T, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_T1_T___R, 200},
	// LS_A_BACKFLIP_ATK
	{"DualSpinAtk", BOTH_SPINATTACK6, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},
	// LS_SPINATTACK_DUAL
	{"DualSpinAtkgrie", BOTH_SPINATTACKGRIEVOUS, Q_R, Q_R,AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},
	// LS_SPINATTACK_GRIEV
	{"StfSpinAtk", BOTH_SPINATTACK7, Q_L, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_SPINATTACK
	{"LngLeapAtk", BOTH_FORCELONGLEAP_ATTACK, Q_R, Q_L, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},	// LS_LEAP_ATTACK
	{"LngLeapAtk2", BOTH_FORCELONGLEAP_ATTACK2, Q_R, Q_L, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},	// LS_LEAP_ATTACK2
	{"SwoopAtkR", BOTH_VS_ATR_S, Q_R, Q_T, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},	// LS_SWOOP_ATTACK_RIGHT
	{"SwoopAtkL", BOTH_VS_ATL_S, Q_L, Q_T, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},
	// LS_SWOOP_ATTACK_LEFT
	{"TauntaunAtkR", BOTH_VT_ATR_S, Q_R, Q_T, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},
	// LS_TAUNTAUN_ATTACK_RIGHT
	{"TauntaunAtkL", BOTH_VT_ATL_S, Q_L, Q_T, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},
	// LS_TAUNTAUN_ATTACK_LEFT
	{"StfKickFwd", BOTH_A7_KICK_F, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200}, // LS_KICK_F
	{"StfKickFwd2", BOTH_A7_KICK_F2, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200}, // LS_KICK_F2
	{"StfKickBack", BOTH_A7_KICK_B, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200}, // LS_KICK_B
	{"StfKickBack2", BOTH_A7_KICK_B2, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200}, // LS_KICK_B2
	{"StfKickBack3", BOTH_A7_KICK_B3, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200}, // LS_KICK_B3
	{"StfKickRight", BOTH_A7_KICK_R, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200}, // LS_KICK_R
	{"StfslapRight", BOTH_A7_SLAP_R, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200}, // LS_SLAP_R
	{"StfKickLeft", BOTH_A7_KICK_L, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200}, // LS_KICK_L
	{"StfslapLeft", BOTH_A7_SLAP_L, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200}, // LS_SLAP_L
	{"StfKickSpin", BOTH_A7_KICK_S, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_NO, LS_READY, LS_S_R2L, 200}, // LS_KICK_S
	{"StfKickBkFwd", BOTH_A7_KICK_BF, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_NO, LS_READY, LS_S_R2L, 200}, // LS_KICK_BF
	{"StfKickSplit", BOTH_A7_KICK_RL, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_NO, LS_READY, LS_S_R2L, 200}, // LS_KICK_RL
	{"StfKickFwdAir", BOTH_A7_KICK_F_AIR, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200},
	// LS_KICK_F_AIR
	{"StfKickFwdAir2", BOTH_A7_KICK_F_AIR2, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200},
	// LS_KICK_F_AIR2
	{"StfKickBackAir", BOTH_A7_KICK_B_AIR, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200},
	// LS_KICK_B_AIR
	{"StfKickRightAir", BOTH_A7_KICK_R_AIR, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200},
	// LS_KICK_R_AIR
	{"StfKickLeftAir", BOTH_A7_KICK_L_AIR, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200},
	// LS_KICK_L_AIR
	{"StabDown", BOTH_STABDOWN, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200}, // LS_STABDOWN
	{"StabDownbhd", BOTH_STABDOWN_BACKHAND, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200},
	// LS_STABDOWN_BACKHAND
	{"StabDownStf", BOTH_STABDOWN_STAFF, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200},
	// LS_STABDOWN_STAFF
	{"StabDownDual", BOTH_STABDOWN_DUAL, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_S_R2L, 200},
	// LS_STABDOWN_DUAL
	{"dualspinprot", BOTH_A6_SABERPROTECT, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 500},
	// LS_DUAL_SPIN_PROTECT
	{"dualspinprotgrie", BOTH_GRIEVOUS_PROTECT, Q_R, Q_R,AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 500},
	// LS_DUAL_SPIN_PROTECT_GRIE
	{"StfSoulCal", BOTH_A7_SOULCAL, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 500},
	// LS_STAFF_SOULCAL
	{"specialyoda", BOTH_YODA_SPECIAL, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 2000},
	// LS_YODA_SPECIAL
	{"specialfast", BOTH_A1_SPECIAL, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 2000}, // LS_A1_SPECIAL
	{"specialmed", BOTH_A2_SPECIAL, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 2000}, // LS_A2_SPECIAL
	{"specialstr", BOTH_A3_SPECIAL, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 2000}, // LS_A3_SPECIAL
	{"specialtav", BOTH_A4_SPECIAL, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 2000}, // LS_A4_SPECIAL
	{"specialdes", BOTH_A5_SPECIAL, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 2000}, // LS_A5_SPECIAL
	{"specialgri", BOTH_GRIEVOUS_SPIN, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 2000},
	// LS_GRIEVOUS_SPECIAL
	{"upsidedwnatk", BOTH_FLIP_ATTACK7, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},
	// LS_UPSIDE_DOWN_ATTACK
	{"pullatkstab", BOTH_PULL_IMPALE_STAB, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},
	// LS_PULL_ATTACK_STAB
	{"pullatkswing", BOTH_PULL_IMPALE_SWING, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},
	// LS_PULL_ATTACK_SWING
	{"AloraSpinAtk", BOTH_ALORA_SPIN_SLASH, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200},
	// LS_SPINATTACK_ALORA
	{"Dual FB Atk", BOTH_A6_FB, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_DUAL_FB
	{"Dual LR Atk", BOTH_A6_LR, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_DUAL_LR
	{"StfHiltBash", BOTH_A7_HILT, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_HILT_BASH
	{"StfsmackRight", BOTH_SMACK_R, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_SMACK_R
	{"StfsmackLeft", BOTH_SMACK_L, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_SMACK_L

	//starts
	{"TL2BR St", BOTH_S1_S1_TL, Q_R, Q_TL, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_A_TL2BR, LS_A_TL2BR, 200}, // LS_S_TL2BR
	{"L2R St", BOTH_S1_S1__L, Q_R, Q_L, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_A_L2R, LS_A_L2R, 200}, // LS_S_L2R
	{"BL2TR St", BOTH_S1_S1_BL, Q_R, Q_BL, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_A_BL2TR, LS_A_BL2TR, 200}, // LS_S_BL2TR
	{"BR2TL St", BOTH_S1_S1_BR, Q_R, Q_BR, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_A_BR2TL, LS_A_BR2TL, 200}, // LS_S_BR2TL
	{"R2L St", BOTH_S1_S1__R, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_A_R2L, LS_A_R2L, 200}, // LS_S_R2L
	{"TR2BL St", BOTH_S1_S1_TR, Q_R, Q_TR, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_A_TR2BL, LS_A_TR2BL, 200}, // LS_S_TR2BL
	{"T2B St", BOTH_S1_S1_T_, Q_R, Q_T, AFLAG_ACTIVE, 100, BLK_TIGHT, LS_A_T2B, LS_A_T2B, 200}, // LS_S_T2B

	//returns
	{"TL2BR Ret", BOTH_R1_BR_S1, Q_BR, Q_R, AFLAG_FINISH, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_R_TL2BR
	{"L2R Ret", BOTH_R1__R_S1, Q_R, Q_R, AFLAG_FINISH, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_R_L2R
	{"BL2TR Ret", BOTH_R1_TR_S1, Q_TR, Q_R, AFLAG_FINISH, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_R_BL2TR
	{"BR2TL Ret", BOTH_R1_TL_S1, Q_TL, Q_R, AFLAG_FINISH, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_R_BR2TL
	{"R2L Ret", BOTH_R1__L_S1, Q_L, Q_R, AFLAG_FINISH, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_R_R2L
	{"TR2BL Ret", BOTH_R1_BL_S1, Q_BL, Q_R, AFLAG_FINISH, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_R_TR2BL
	{"T2B Ret", BOTH_R1_B__S1, Q_B, Q_R, AFLAG_FINISH, 100, BLK_TIGHT, LS_READY, LS_READY, 200}, // LS_R_T2B

	//Transitions
	{"BR2R Trans", BOTH_T1_BR__R, Q_BR, Q_R, AFLAG_ACTIVE, 100, BLK_NO, LS_R_L2R, LS_A_R2L, 150},
	//# Fast arc bottom right to right
	{"BR2TR Trans", BOTH_T1_BR_TR, Q_BR, Q_TR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_A_TR2BL, 150},
	//# Fast arc bottom right to top right		(use: BOTH_T1_TR_BR)
	{"BR2T Trans", BOTH_T1_BR_T_, Q_BR, Q_T, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_A_T2B, 150},
	//# Fast arc bottom right to top			(use: BOTH_T1_T__BR)
	{"BR2TL Trans", BOTH_T1_BR_TL, Q_BR, Q_TL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BR2TL, LS_A_TL2BR, 150},
	//# Fast weak spin bottom right to top left
	{"BR2L Trans", BOTH_T1_BR__L, Q_BR, Q_L, AFLAG_ACTIVE, 100, BLK_NO, LS_R_R2L, LS_A_L2R, 150},
	//# Fast weak spin bottom right to left
	{"BR2BL Trans", BOTH_T1_BR_BL, Q_BR, Q_BL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TR2BL, LS_A_BL2TR, 150},
	//# Fast weak spin bottom right to bottom left
	{"R2BR Trans", BOTH_T1__R_BR, Q_R, Q_BR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TL2BR, LS_A_BR2TL, 150},
	//# Fast arc right to bottom right			(use: BOTH_T1_BR__R)
	{"R2TR Trans", BOTH_T1__R_TR, Q_R, Q_TR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_A_TR2BL, 150},
	//# Fast arc right to top right
	{"R2T Trans", BOTH_T1__R_T_, Q_R, Q_T, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_A_T2B, 150},
	//# Fast ar right to top				(use: BOTH_T1_T___R)
	{"R2TL Trans", BOTH_T1__R_TL, Q_R, Q_TL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BR2TL, LS_A_TL2BR, 150},
	//# Fast arc right to top left
	{"R2L Trans", BOTH_T1__R__L, Q_R, Q_L, AFLAG_ACTIVE, 100, BLK_NO, LS_R_R2L, LS_A_L2R, 150},
	//# Fast weak spin right to left
	{"R2BL Trans", BOTH_T1__R_BL, Q_R, Q_BL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TR2BL, LS_A_BL2TR, 150},
	//# Fast weak spin right to bottom left
	{"TR2BR Trans", BOTH_T1_TR_BR, Q_TR, Q_BR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TL2BR, LS_A_BR2TL, 150},
	//# Fast arc top right to bottom right
	{"TR2R Trans", BOTH_T1_TR__R, Q_TR, Q_R, AFLAG_ACTIVE, 100, BLK_NO, LS_R_L2R, LS_A_R2L, 150},
	//# Fast arc top right to right			(use: BOTH_T1__R_TR)
	{"TR2T Trans", BOTH_T1_TR_T_, Q_TR, Q_T, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_A_T2B, 150},
	//# Fast arc top right to top				(use: BOTH_T1_T__TR)
	{"TR2TL Trans", BOTH_T1_TR_TL, Q_TR, Q_TL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BR2TL, LS_A_TL2BR, 150},
	//# Fast arc top right to top left
	{"TR2L Trans", BOTH_T1_TR__L, Q_TR, Q_L, AFLAG_ACTIVE, 100, BLK_NO, LS_R_R2L, LS_A_L2R, 150},
	//# Fast arc top right to left
	{"TR2BL Trans", BOTH_T1_TR_BL, Q_TR, Q_BL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TR2BL, LS_A_BL2TR, 150},
	//# Fast weak spin top right to bottom left
	{"T2BR Trans", BOTH_T1_T__BR, Q_T, Q_BR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TL2BR, LS_A_BR2TL, 150},
	//# Fast arc top to bottom right
	{"T2R Trans", BOTH_T1_T___R, Q_T, Q_R, AFLAG_ACTIVE, 100, BLK_NO, LS_R_L2R, LS_A_R2L, 150},
	//# Fast arc top to right
	{"T2TR Trans", BOTH_T1_T__TR, Q_T, Q_TR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_A_TR2BL, 150},
	//# Fast arc top to top right
	{"T2TL Trans", BOTH_T1_T__TL, Q_T, Q_TL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BR2TL, LS_A_TL2BR, 150},
	//# Fast arc top to top left
	{"T2L Trans", BOTH_T1_T___L, Q_T, Q_L, AFLAG_ACTIVE, 100, BLK_NO, LS_R_R2L, LS_A_L2R, 150}, //# Fast arc top to left
	{"T2BL Trans", BOTH_T1_T__BL, Q_T, Q_BL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TR2BL, LS_A_BL2TR, 150},
	//# Fast arc top to bottom left
	{"TL2BR Trans", BOTH_T1_TL_BR, Q_TL, Q_BR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TL2BR, LS_A_BR2TL, 150},
	//# Fast weak spin top left to bottom right
	{"TL2R Trans", BOTH_T1_TL__R, Q_TL, Q_R, AFLAG_ACTIVE, 100, BLK_NO, LS_R_L2R, LS_A_R2L, 150},
	//# Fast arc top left to right			(use: BOTH_T1__R_TL)
	{"TL2TR Trans", BOTH_T1_TL_TR, Q_TL, Q_TR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_A_TR2BL, 150},
	//# Fast arc top left to top right			(use: BOTH_T1_TR_TL)
	{"TL2T Trans", BOTH_T1_TL_T_, Q_TL, Q_T, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_A_T2B, 150},
	//# Fast arc top left to top				(use: BOTH_T1_T__TL)
	{"TL2L Trans", BOTH_T1_TL__L, Q_TL, Q_L, AFLAG_ACTIVE, 100, BLK_NO, LS_R_R2L, LS_A_L2R, 150},
	//# Fast arc top left to left				(use: BOTH_T1__L_TL)
	{"TL2BL Trans", BOTH_T1_TL_BL, Q_TL, Q_BL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TR2BL, LS_A_BL2TR, 150},
	//# Fast arc top left to bottom left
	{"L2BR Trans", BOTH_T1__L_BR, Q_L, Q_BR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TL2BR, LS_A_BR2TL, 150},
	//# Fast weak spin left to bottom right
	{"L2R Trans", BOTH_T1__L__R, Q_L, Q_R, AFLAG_ACTIVE, 100, BLK_NO, LS_R_L2R, LS_A_R2L, 150},
	//# Fast weak spin left to right
	{"L2TR Trans", BOTH_T1__L_TR, Q_L, Q_TR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_A_TR2BL, 150},
	//# Fast arc left to top right			(use: BOTH_T1_TR__L)
	{"L2T Trans", BOTH_T1__L_T_, Q_L, Q_T, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_A_T2B, 150},
	//# Fast arc left to top				(use: BOTH_T1_T___L)
	{"L2TL Trans", BOTH_T1__L_TL, Q_L, Q_TL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BR2TL, LS_A_TL2BR, 150},
	//# Fast arc left to top left
	{"L2BL Trans", BOTH_T1__L_BL, Q_L, Q_BL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TR2BL, LS_A_BL2TR, 150},
	//# Fast arc left to bottom left			(use: BOTH_T1_BL__L)
	{"BL2BR Trans", BOTH_T1_BL_BR, Q_BL, Q_BR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TL2BR, LS_A_BR2TL, 150},
	//# Fast weak spin bottom left to bottom right
	{"BL2R Trans", BOTH_T1_BL__R, Q_BL, Q_R, AFLAG_ACTIVE, 100, BLK_NO, LS_R_L2R, LS_A_R2L, 150},
	//# Fast weak spin bottom left to right
	{"BL2TR Trans", BOTH_T1_BL_TR, Q_BL, Q_TR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_A_TR2BL, 150},
	//# Fast weak spin bottom left to top right
	{"BL2T Trans", BOTH_T1_BL_T_, Q_BL, Q_T, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_A_T2B, 150},
	//# Fast arc bottom left to top			(use: BOTH_T1_T__BL)
	{"BL2TL Trans", BOTH_T1_BL_TL, Q_BL, Q_TL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BR2TL, LS_A_TL2BR, 150},
	//# Fast arc bottom left to top left		(use: BOTH_T1_TL_BL)
	{"BL2L Trans", BOTH_T1_BL__L, Q_BL, Q_L, AFLAG_ACTIVE, 100, BLK_NO, LS_R_R2L, LS_A_L2R, 150},
	//# Fast arc bottom left to left

	//Bounces
	{"Bounce BR", BOTH_B1_BR___, Q_BR, Q_BR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TL2BR, LS_T1_BR_TR, 150},
	{"Bounce R", BOTH_B1__R___, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_NO, LS_R_L2R, LS_T1__R__L, 150},
	{"Bounce TR", BOTH_B1_TR___, Q_TR, Q_TR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_T1_TR_TL, 150},
	{"Bounce T", BOTH_B1_T____, Q_T, Q_T, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_T1_T__BL, 150},
	{"Bounce TL", BOTH_B1_TL___, Q_TL, Q_TL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BR2TL, LS_T1_TL_TR, 150},
	{"Bounce L", BOTH_B1__L___, Q_L, Q_L, AFLAG_ACTIVE, 100, BLK_NO, LS_R_R2L, LS_T1__L__R, 150},
	{"Bounce BL", BOTH_B1_BL___, Q_BL, Q_BL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TR2BL, LS_T1_BL_TR, 150},

	//Deflected attacks (like bounces, but slide off enemy saber, not straight back)
	{"Deflect BR", BOTH_D1_BR___, Q_BR, Q_BR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TL2BR, LS_T1_BR_TR, 150},
	{"Deflect R", BOTH_D1__R___, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_NO, LS_R_L2R, LS_T1__R__L, 150},
	{"Deflect TR", BOTH_D1_TR___, Q_TR, Q_TR, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_T1_TR_TL, 150},
	{"Deflect T", BOTH_B1_T____, Q_T, Q_T, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_T1_T__BL, 150},
	{"Deflect TL", BOTH_D1_TL___, Q_TL, Q_TL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BR2TL, LS_T1_TL_TR, 150},
	{"Deflect L", BOTH_D1__L___, Q_L, Q_L, AFLAG_ACTIVE, 100, BLK_NO, LS_R_R2L, LS_T1__L__R, 150},
	{"Deflect BL", BOTH_D1_BL___, Q_BL, Q_BL, AFLAG_ACTIVE, 100, BLK_NO, LS_R_TR2BL, LS_T1_BL_TR, 150},
	{"Deflect B", BOTH_D1_B____, Q_B, Q_B, AFLAG_ACTIVE, 100, BLK_NO, LS_R_BL2TR, LS_T1_T__BL, 150},

	//Reflected attacks
	{"Reflected BR", BOTH_V1_BR_S1, Q_BR, Q_BR, AFLAG_ACTIVE, 100, BLK_NO, LS_READY, LS_READY, 150}, //	LS_V1_BR
	{"Reflected R", BOTH_V1__R_S1, Q_R, Q_R, AFLAG_ACTIVE, 100, BLK_NO, LS_READY, LS_READY, 150}, //	LS_V1__R
	{"Reflected TR", BOTH_V1_TR_S1, Q_TR, Q_TR, AFLAG_ACTIVE, 100, BLK_NO, LS_READY, LS_READY, 150}, //	LS_V1_TR
	{"Reflected T", BOTH_V1_T__S1, Q_T, Q_T, AFLAG_ACTIVE, 100, BLK_NO, LS_READY, LS_READY, 150}, //	LS_V1_T_
	{"Reflected TL", BOTH_V1_TL_S1, Q_TL, Q_TL, AFLAG_ACTIVE, 100, BLK_NO, LS_READY, LS_READY, 150}, //	LS_V1_TL
	{"Reflected L", BOTH_V1__L_S1, Q_L, Q_L, AFLAG_ACTIVE, 100, BLK_NO, LS_READY, LS_READY, 150}, //	LS_V1__L
	{"Reflected BL", BOTH_V1_BL_S1, Q_BL, Q_BL, AFLAG_ACTIVE, 100, BLK_NO, LS_READY, LS_READY, 150}, //	LS_V1_BL
	{"Reflected B", BOTH_V1_B__S1, Q_B, Q_B, AFLAG_ACTIVE, 100, BLK_NO, LS_READY, LS_READY, 150}, //	LS_V1_B_

	// Broken parries
	{"BParry Top", BOTH_H1_S1_T_, Q_T, Q_B, AFLAG_ACTIVE, 50, BLK_NO, LS_READY, LS_READY, 150}, // LS_PARRY_UP,
	{"BParry UR", BOTH_H1_S1_TR, Q_TR, Q_BL, AFLAG_ACTIVE, 50, BLK_NO, LS_READY, LS_READY, 150}, // LS_PARRY_UR,
	{"BParry UL", BOTH_H1_S1_TL, Q_TL, Q_BR, AFLAG_ACTIVE, 50, BLK_NO, LS_READY, LS_READY, 150}, // LS_PARRY_UL,
	{"BParry LR", BOTH_H1_S1_BR, Q_BL, Q_TR, AFLAG_ACTIVE, 50, BLK_NO, LS_READY, LS_READY, 150}, // LS_PARRY_LR,
	{"BParry Bot", BOTH_H1_S1_B_, Q_B, Q_T, AFLAG_ACTIVE, 50, BLK_NO, LS_READY, LS_READY, 150}, // LS_PARRY_LR
	{"BParry LL", BOTH_H1_S1_BL, Q_BR, Q_TL, AFLAG_ACTIVE, 50, BLK_NO, LS_READY, LS_READY, 150}, // LS_PARRY_LL

	// Knockaways
	{"Knock Top", BOTH_K1_S1_T_, Q_R, Q_T, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_T1_T__BR, 150}, // LS_PARRY_UP,
	{"Knock UR", BOTH_K1_S1_TR, Q_R, Q_TR, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_T1_TR__R, 150}, // LS_PARRY_UR,
	{"Knock UL", BOTH_K1_S1_TL, Q_R, Q_TL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BR2TL, LS_T1_TL__L, 150}, // LS_PARRY_UL,
	{"Knock LR", BOTH_K1_S1_BR, Q_R, Q_BL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_TL2BR, LS_T1_BL_TL, 150}, // LS_PARRY_LR,
	{"Knock LL", BOTH_K1_S1_BL, Q_R, Q_BR, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_TR2BL, LS_T1_BR_TR, 150}, // LS_PARRY_LL

	// Parry
	{"Parry Top", BOTH_P1_S1_T_, Q_R, Q_T, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_T2B, 150}, // LS_PARRY_UP,
	{"Parry Front", BOTH_P1_S1_T1_, Q_R, Q_T, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_T2B, 150}, // LS_PARRY_FRONT,
	{"Parry Walk", BOTH_PARRY_WALK, Q_R, Q_T, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_T2B, 150}, // LS_PARRY_WALK,
	{"Parry Walk_dual", BOTH_PARRY_WALK_DUAL, Q_R, Q_T, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_T2B, 150},
	// LS_PARRY_WALK_DUAL,
	{"Parry Walk_staff", BOTH_PARRY_WALK_STAFF, Q_R, Q_T, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_T2B, 150},
	// LS_PARRY_WALK_STAFF,
	{"Parry UR", BOTH_P1_S1_TR, Q_R, Q_TL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_TR2BL, 150}, // LS_PARRY_UR,
	{"Parry UL", BOTH_P1_S1_TL, Q_R, Q_TR, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BR2TL, LS_A_TL2BR, 150}, // LS_PARRY_UL,
	{"Parry LR", BOTH_P1_S1_BR, Q_R, Q_BR, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_TL2BR, LS_A_BR2TL, 150}, // LS_PARRY_LR,
	{"Parry LL", BOTH_P1_S1_BL, Q_R, Q_BL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_TR2BL, LS_A_BL2TR, 150}, // LS_PARRY_LL

	// Reflecting a missile
	{"Reflect Top", BOTH_P1_S1_T_, Q_R, Q_T, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_T2B, 300}, // LS_REFLECT_UP,
	{"Reflect Front", BOTH_P1_S1_T1_, Q_R, Q_T, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_T2B, 300},
	// LS_REFLECT_FRONT,
	{"Parry UR", BOTH_P1_S1_TR, Q_R, Q_TL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_TR2BL, 150}, // LS_PARRY_UR,
	{"Parry UL", BOTH_P1_S1_TL, Q_R, Q_TR, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BR2TL, LS_A_TL2BR, 150}, // LS_PARRY_UL,
	{"Parry LR", BOTH_P1_S1_BR, Q_R, Q_BR, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_TL2BR, LS_A_BR2TL, 150}, // LS_PARRY_LR,
	{"Parry LL", BOTH_P1_S1_BL, Q_R, Q_BL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_TR2BL, LS_A_BL2TR, 150}, // LS_PARRY_LL

	{"Reflect Attack_L", BOTH_BLOCKATTACK_LEFT, Q_R, Q_TR, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BR2TL, LS_A_TL2BR, 150},
	// LS_REFLECT_ATTACK_LEFT
	{"Reflect Attack_R", BOTH_BLOCKATTACK_RIGHT, Q_R, Q_TL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_TR2BL, 150},
	// LS_REFLECT_ATTACK_RIGHT,
	{"Reflect Attack_F", BOTH_P7_S7_T_, Q_R, Q_TL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_TR2BL, 150},
	// LS_REFLECT_ATTACK_FRONT,

	{"Block Full_R", BOTH_K1_S1_TR_ALT, Q_R, Q_TL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_TR2BL, 150},
	// LS_BLOCK_FULL_RIGHT,
	{"Block Full_L", BOTH_K1_S1_TL_ALT, Q_R, Q_TL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_TR2BL, 150},
	// LS_BLOCK_FULL_LEFT,

	{"Block Perfect_R", BOTH_K1_S1_TR_PB, Q_R, Q_TL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_TR2BL, 150},
	// LS_PERFECTBLOCK_RIGHT,
	{"Block Perfect_L", BOTH_K1_S1_TL_PB, Q_R, Q_TL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_TR2BL, 150},
	// LS_PERFECTBLOCK_LEFT,

	{"Knock Full_R", BOTH_K1_S1_TR_OLD, Q_R, Q_TL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_TR2BL, 150},
	// LS_KNOCK_RIGHT,
	{"Knock Full_L", BOTH_K1_S1_TL_OLD, Q_R, Q_TL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR, LS_A_TR2BL, 150},
	// LS_KNOCK_LEFT,
};

saber_moveName_t transitionMove[Q_NUM_QUADS][Q_NUM_QUADS] =
{
	{
		LS_NONE, //Can't transition to same pos!
		LS_T1_BR__R, //40
		LS_T1_BR_TR,
		LS_T1_BR_T_,
		LS_T1_BR_TL,
		LS_T1_BR__L,
		LS_T1_BR_BL,
		LS_NONE //No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1__R_BR, //46
		LS_NONE, //Can't transition to same pos!
		LS_T1__R_TR,
		LS_T1__R_T_,
		LS_T1__R_TL,
		LS_T1__R__L,
		LS_T1__R_BL,
		LS_NONE //No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_TR_BR, //52
		LS_T1_TR__R,
		LS_NONE, //Can't transition to same pos!
		LS_T1_TR_T_,
		LS_T1_TR_TL,
		LS_T1_TR__L,
		LS_T1_TR_BL,
		LS_NONE //No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_T__BR, //58
		LS_T1_T___R,
		LS_T1_T__TR,
		LS_NONE, //Can't transition to same pos!
		LS_T1_T__TL,
		LS_T1_T___L,
		LS_T1_T__BL,
		LS_NONE //No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_TL_BR, //64
		LS_T1_TL__R,
		LS_T1_TL_TR,
		LS_T1_TL_T_,
		LS_NONE, //Can't transition to same pos!
		LS_T1_TL__L,
		LS_T1_TL_BL,
		LS_NONE //No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1__L_BR, //70
		LS_T1__L__R,
		LS_T1__L_TR,
		LS_T1__L_T_,
		LS_T1__L_TL,
		LS_NONE, //Can't transition to same pos!
		LS_T1__L_BL,
		LS_NONE //No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_BL_BR, //76
		LS_T1_BL__R,
		LS_T1_BL_TR,
		LS_T1_BL_T_,
		LS_T1_BL_TL,
		LS_T1_BL__L,
		LS_NONE, //Can't transition to same pos!
		LS_NONE //No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_BL_BR, //NOTE: there are no transitions from bottom, so re-use the bottom right transitions
		LS_T1_BR__R,
		LS_T1_BR_TR,
		LS_T1_BR_T_,
		LS_T1_BR_TL,
		LS_T1_BR__L,
		LS_T1_BR_BL,
		LS_NONE //No transitions to bottom, and no anims start there, so shouldn't need any
	}
};

saber_moveName_t PM_NPCSaberAttackFromQuad(const int quad)
{
	saber_moveName_t auto_move = LS_NONE;

	if (auto_move != LS_NONE && PM_SaberInSpecial(auto_move))
	{
		return auto_move;
	}

	//pick another one
	saber_moveName_t newmove = LS_NONE;

	// Default quadrant‑based selection
	switch (quad)
	{
	case Q_T:
		return Q_irand(0, 1) ? LS_A_T2B : LS_A_TR2BL;

	case Q_TR:
		switch (Q_irand(0, 2))
		{
		case 0: return LS_A_R2L;
		case 1: return LS_A_TR2BL;
		default: return LS_T1_TR_BR;
		}

	case Q_TL:
		switch (Q_irand(0, 2))
		{
		case 0: return LS_A_L2R;
		case 1: return LS_A_TL2BR;
		default: return LS_T1_TL_BL;
		}

	case Q_BR:
		switch (Q_irand(0, 2))
		{
		case 0: return LS_A_BR2TL;
		case 1: return LS_T1_BR_TR;
		default: return LS_A_R2L;
		}

	case Q_BL:
		switch (Q_irand(0, 2))
		{
		case 0: return LS_A_BL2TR;
		case 1: return LS_T1_BL_TL;
		default: return LS_A_L2R;
		}

	case Q_L:
		switch (Q_irand(0, 2))
		{
		case 0: return LS_A_L2R;
		case 1: return LS_T1__L_T_;
		default: return LS_A_R2L;
		}

	case Q_R:
		switch (Q_irand(0, 2))
		{
		case 0: return LS_A_R2L;
		case 1: return LS_T1__R_T_;
		default: return LS_A_L2R;
		}

	case Q_B:
		return PM_SaberLungeAttackMove(qtrue);

	default:
		return LS_NONE;
	}


#ifdef _GAME
	if (bot_thinklevel.integer >= 1 &&
		BG_EnoughForcePowerForMove(SABER_KATA_ATTACK_POWER,qfalse) &&
		!pm->ps->fd.forcePowersActive &&
		!in_camera)
	{
		// Some special stuff.
		if (Next_Kill_Attack_Move_Check[pm->ps->clientNum] <= level.time &&
			g_attackskill.integer >= 0)
		{
			int check_val = 0; // Times 500 for next check interval.

			//
			// BACKSTAB / BACK-LUNGE LOGIC
			//
			if (PM_Can_Do_Kill_Lunge_back())
			{
				if ((pm->ps->pm_flags & PMF_DUCKED) || pm->cmd.upmove < 0)
				{
					newmove = LS_A_BACK_CR;
				}
				else
				{
					int choice = rand() % 4;   // FIXED: %4 so all 4 branches reachable

					switch (choice)
					{
					case 0:
						newmove = LS_A_BACK;
						break;

					case 1:
						newmove = PM_SaberBackflipAttackMove();
						break;

					case 2:
						newmove = LS_A_BACKFLIP_ATK;
						break;

					case 3:
					default:
						newmove = LS_A_BACKSTAB;
						break;
					}
				}
			}

			//
			// LUNGE / SPECIAL ATTACK LOGIC
			//
			else if (PM_Can_Do_Kill_Lunge())
			{
				if (pm->ps->pm_flags & PMF_DUCKED)
				{
					newmove = PM_SaberLungeAttackMove(qtrue);

					if (d_attackinfo.integer)
					{
						Com_Printf(S_COLOR_MAGENTA "Next_Kill_Attack_Move_Check 0\n");
					}
				}
				else
				{
					const int choice = rand() % 9;

					switch (choice)
					{
					case 0:
						newmove = PM_NPC_Force_Leap_Attack();
						if (d_attackinfo.integer)
							Com_Printf(S_COLOR_MAGENTA "Next_Kill_Attack_Move_Check 1\n");
						break;

					case 1:
						newmove = PM_DoAI_Fake(qtrue);
						if (d_attackinfo.integer)
							Com_Printf(S_COLOR_MAGENTA "Next_Kill_Attack_Move_Check 2\n");
						break;

					case 2:
						newmove = LS_A1_SPECIAL;
						if (d_attackinfo.integer)
							Com_Printf(S_COLOR_MAGENTA "Next_Kill_Attack_Move_Check 3\n");
						break;

					case 3:
						newmove = LS_A2_SPECIAL;
						if (d_attackinfo.integer)
							Com_Printf(S_COLOR_MAGENTA "Next_Kill_Attack_Move_Check 4\n");
						break;

					case 4:
						newmove = LS_A3_SPECIAL;
						if (d_attackinfo.integer)
							Com_Printf(S_COLOR_MAGENTA "Next_Kill_Attack_Move_Check 5\n");
						break;

					case 5:
						newmove = LS_A4_SPECIAL;
						if (d_attackinfo.integer)
							Com_Printf(S_COLOR_MAGENTA "Next_Kill_Attack_Move_Check 6\n");
						break;

					case 6:
						newmove = LS_A5_SPECIAL;
						if (d_attackinfo.integer)
							Com_Printf(S_COLOR_MAGENTA "Next_Kill_Attack_Move_Check 7\n");
						break;

					case 7:
						if (pm->ps->fd.saberAnimLevel == SS_DUAL)
						{
							newmove = LS_DUAL_SPIN_PROTECT;
						}
						else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
						{
							newmove = LS_STAFF_SOULCAL;
						}
						else
						{
							newmove = LS_A_JUMP_T__B_;
						}

						if (d_attackinfo.integer)
							Com_Printf(S_COLOR_MAGENTA "Next_Kill_Attack_Move_Check 8\n");
						break;

					case 8:
						if (pm->ps->fd.saberAnimLevel == SS_DUAL)
						{
							newmove = LS_JUMPATTACK_DUAL;
						}
						else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
						{
							newmove = LS_JUMPATTACK_STAFF_RIGHT;
						}
						else
						{
							newmove = LS_SPINATTACK;
						}

						if (d_attackinfo.integer)
							Com_Printf(S_COLOR_MAGENTA "Next_Kill_Attack_Move_Check 9\n");
						break;

					default:
						newmove = PM_SaberLungeAttackMove(qtrue);
						if (d_attackinfo.integer)
							Com_Printf(S_COLOR_MAGENTA "Next_Kill_Attack_Move_Check 10\n");
						break;
					}
				}

				pm->ps->weaponstate = WEAPON_FIRING;
				WP_ForcePowerDrain(pm->ps, FP_SABER_OFFENSE, SABER_ALT_ATTACK_POWER_FB);
			}

			//
			// COOLDOWN UPDATE
			//
			check_val = g_attackskill.integer;

			if (check_val <= 0)
			{
				check_val = 1;
			}

			Next_Kill_Attack_Move_Check[pm->ps->clientNum] =
				level.time + (90000 / check_val); // 20 secs / g_attackskill
		}
	}
#endif

	return newmove;
}

saber_moveName_t PM_AttackMoveForQuad(const int quad)
{
	switch (quad)
	{
	case Q_B:
	case Q_BR:
		return LS_A_BR2TL;
	case Q_R:
		return LS_A_R2L;
	case Q_TR:
		return LS_A_TR2BL;
	case Q_T:
		return LS_A_T2B;
	case Q_TL:
		return LS_A_TL2BR;
	case Q_L:
		return LS_A_L2R;
	case Q_BL:
		return LS_A_BL2TR;
	default:;
	}
	return LS_NONE;
}

qboolean PM_SaberKataDone(int curmove, int newmove);
int PM_ReturnforQuad(int quad);

saber_moveName_t PM_SaberAnimTransitionMove(const saber_moveName_t curmove, const saber_moveName_t newmove)
{
	int retmove = newmove;

	if (curmove == LS_READY)
	{
		//just standing there
		switch (newmove)
		{
		case LS_A_TL2BR:
		case LS_A_L2R:
		case LS_A_BL2TR:
		case LS_A_BR2TL:
		case LS_A_R2L:
		case LS_A_TR2BL:
		case LS_A_T2B:
			//transition is the start
			retmove = LS_S_TL2BR + (newmove - LS_A_TL2BR);
			break;
		default:
			break;
		}
	}
	else
	{
		switch (newmove)
		{
			//transitioning to ready pose
		case LS_READY:
			switch (curmove)
			{
				//transitioning from an attack
			case LS_A_TL2BR:
			case LS_A_L2R:
			case LS_A_BL2TR:
			case LS_A_BR2TL:
			case LS_A_R2L:
			case LS_A_TR2BL:
			case LS_A_T2B:
				//transition is the return
				retmove = LS_R_TL2BR + (newmove - LS_A_TL2BR);
				break;
			case LS_B1_BR:
			case LS_B1__R:
			case LS_B1_TR:
			case LS_B1_T_:
			case LS_B1_TL:
			case LS_B1__L:
			case LS_B1_BL:
				//transitioning from a parry/reflection/knockaway/broken parry
			case LS_PARRY_UP:
			case LS_PARRY_FRONT:
			case LS_PARRY_WALK:
			case LS_PARRY_WALK_DUAL:
			case LS_PARRY_WALK_STAFF:
			case LS_PARRY_UR:
			case LS_PARRY_UL:
			case LS_PARRY_LR:
			case LS_PARRY_LL:
			case LS_REFLECT_UP:
			case LS_REFLECT_FRONT:
			case LS_REFLECT_UR:
			case LS_REFLECT_UL:
			case LS_REFLECT_LR:
			case LS_REFLECT_LL:
			case LS_REFLECT_ATTACK_LEFT:
			case LS_REFLECT_ATTACK_RIGHT:
			case LS_REFLECT_ATTACK_FRONT:
			case LS_BLOCK_FULL_RIGHT:
			case LS_BLOCK_FULL_LEFT:
			case LS_PERFECTBLOCK_RIGHT:
			case LS_PERFECTBLOCK_LEFT:
			case LS_KNOCK_RIGHT:
			case LS_KNOCK_LEFT:
			case LS_K1_T_:
			case LS_K1_TR:
			case LS_K1_TL:
			case LS_K1_BR:
			case LS_K1_BL:
			case LS_V1_BR:
			case LS_V1__R:
			case LS_V1_TR:
			case LS_V1_T_:
			case LS_V1_TL:
			case LS_V1__L:
			case LS_V1_BL:
			case LS_V1_B_:
			case LS_H1_T_:
			case LS_H1_TR:
			case LS_H1_TL:
			case LS_H1_BR:
			case LS_H1_BL:
				retmove = PM_ReturnforQuad(saber_moveData[curmove].endQuad);
				break;
			default:;
			}
			break;
			//transitioning to an attack
		case LS_A_TL2BR:
		case LS_A_L2R:
		case LS_A_BL2TR:
		case LS_A_BR2TL:
		case LS_A_R2L:
		case LS_A_TR2BL:
		case LS_A_T2B:
			if (newmove == curmove)
			{
				if (PM_SaberKataDone(curmove, newmove))
				{
					//done with this kata, must return to ready before attack again
					retmove = LS_R_TL2BR + (newmove - LS_A_TL2BR);
				}
				else
				{
					//okay to chain to another attack
					retmove = transitionMove[saber_moveData[curmove].endQuad][saber_moveData[newmove].startQuad];
				}
			}
			else if (saber_moveData[curmove].endQuad == saber_moveData[newmove].startQuad)
			{
				//new move starts from same quadrant
				retmove = newmove;
			}
			else
			{
				switch (curmove)
				{
					//transitioning from an attack
				case LS_A_TL2BR:
				case LS_A_L2R:
				case LS_A_BL2TR:
				case LS_A_BR2TL:
				case LS_A_R2L:
				case LS_A_TR2BL:
				case LS_A_T2B:
				case LS_D1_BR:
				case LS_D1__R:
				case LS_D1_TR:
				case LS_D1_T_:
				case LS_D1_TL:
				case LS_D1__L:
				case LS_D1_BL:
				case LS_D1_B_:
					retmove = transitionMove[saber_moveData[curmove].endQuad][saber_moveData[newmove].startQuad];
					break;
					//transitioning from a return
				case LS_R_TL2BR:
				case LS_R_L2R:
				case LS_R_BL2TR:
				case LS_R_BR2TL:
				case LS_R_R2L:
				case LS_R_TR2BL:
				case LS_R_T2B:
					//transition is the start
					retmove = LS_S_TL2BR + (newmove - LS_A_TL2BR);
					break;
					//transitioning from a bounce
					//bounces should transition to transitions before attacks.
				case LS_B1_BR:
				case LS_B1__R:
				case LS_B1_TR:
				case LS_B1_T_:
				case LS_B1_TL:
				case LS_B1__L:
				case LS_B1_BL:
					//transitioning from a parry/reflection/knockaway/broken parry
				case LS_PARRY_UP:
				case LS_PARRY_FRONT:
				case LS_PARRY_WALK:
				case LS_PARRY_WALK_DUAL:
				case LS_PARRY_WALK_STAFF:
				case LS_PARRY_UR:
				case LS_PARRY_UL:
				case LS_PARRY_LR:
				case LS_PARRY_LL:
				case LS_REFLECT_UP:
				case LS_REFLECT_FRONT:
				case LS_REFLECT_UR:
				case LS_REFLECT_UL:
				case LS_REFLECT_LR:
				case LS_REFLECT_LL:
				case LS_REFLECT_ATTACK_LEFT:
				case LS_REFLECT_ATTACK_RIGHT:
				case LS_REFLECT_ATTACK_FRONT:
				case LS_BLOCK_FULL_RIGHT:
				case LS_BLOCK_FULL_LEFT:
				case LS_PERFECTBLOCK_RIGHT:
				case LS_PERFECTBLOCK_LEFT:
				case LS_KNOCK_RIGHT:
				case LS_KNOCK_LEFT:
				case LS_K1_T_:
				case LS_K1_TR:
				case LS_K1_TL:
				case LS_K1_BR:
				case LS_K1_BL:
				case LS_V1_BR:
				case LS_V1__R:
				case LS_V1_TR:
				case LS_V1_T_:
				case LS_V1_TL:
				case LS_V1__L:
				case LS_V1_BL:
				case LS_V1_B_:
				case LS_H1_T_:
				case LS_H1_TR:
				case LS_H1_TL:
				case LS_H1_BR:
				case LS_H1_BL:
					retmove = transitionMove[saber_moveData[curmove].endQuad][saber_moveData[newmove].startQuad];
					break;
					//NB: transitioning from transitions is fine
				default:
					break;
				}
			}
			break;
			//transitioning to any other anim is not supported
		default:
			break;
		}
	}

	if (retmove == LS_NONE)
	{
		return newmove;
	}

	return retmove;
}

extern qboolean BG_InKnockDown(int anim);

static saber_moveName_t PM_CheckStabDown(void)
{
	vec3_t face_fwd, facing_angles;
	vec3_t fwd;
	const bgEntity_t* ent = NULL;
	trace_t tr;
	const vec3_t trmins = { -15, -15, -15 };
	const vec3_t trmaxs = { 15, 15, 15 };

	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	const saberInfo_t* saber2 = BG_MySaber(pm->ps->clientNum, 1);

	if (saber1
		&& saber1->saberFlags & SFL_NO_STABDOWN)
	{
		return LS_NONE;
	}
	if (saber2
		&& saber2->saberFlags & SFL_NO_STABDOWN)
	{
		return LS_NONE;
	}

	if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
	{
		//sorry must be on ground!
		return LS_NONE;
	}
	if (pm->ps->clientNum < MAX_CLIENTS)
	{
		//player
		pm->ps->velocity[2] = 0;
		pm->cmd.upmove = 0;
	}

	VectorSet(facing_angles, 0, pm->ps->viewangles[YAW], 0);
	AngleVectors(facing_angles, face_fwd, NULL, NULL);

	//FIXME: need to only move forward until we bump into our target...?
	VectorMA(pm->ps->origin, 164.0f, face_fwd, fwd);

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, fwd, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.entityNum < ENTITYNUM_WORLD)
	{
		ent = PM_BGEntForNum(tr.entityNum);
	}

	if (ent &&
		(ent->s.eType == ET_PLAYER || ent->s.eType == ET_NPC) &&
		BG_InKnockDown(ent->s.legsAnim))
	{
		//guy is on the ground below me, do a top-down attack
		if (pm->ps->fd.saberAnimLevel == SS_DUAL)
		{
			return LS_STABDOWN_DUAL;
		}
		if (pm->ps->fd.saberAnimLevel == SS_STAFF)
		{
			if (saber1 && (saber1->type == SABER_BACKHAND
				|| saber1->type == SABER_ASBACKHAND)) //saber backhand
			{
				return LS_STABDOWN_BACKHAND;
			}
			return LS_STABDOWN_STAFF;
		}
		return LS_STABDOWN;
	}
	return LS_NONE;
}

static int PM_saber_moveQuadrantForMovement(const usercmd_t* ucmd)
{
	if (ucmd->rightmove > 0)
	{
		//moving right
		if (ucmd->forwardmove > 0)
		{
			//forward right = TL2BR slash
			return Q_TL;
		}
		if (ucmd->forwardmove < 0)
		{
			//backward right = BL2TR uppercut
			return Q_BL;
		}
		//just right is a left slice
		return Q_L;
	}
	if (ucmd->rightmove < 0)
	{
		//moving left
		if (ucmd->forwardmove > 0)
		{
			//forward left = TR2BL slash
			return Q_TR;
		}
		if (ucmd->forwardmove < 0)
		{
			//backward left = BR2TL uppercut
			return Q_BR;
		}
		//just left is a right slice
		return Q_R;
	}
	//not moving left or right
	if (ucmd->forwardmove > 0)
	{
		//forward= T2B slash
		return Q_T;
	}
	if (ucmd->forwardmove < 0)
	{
		//backward= T2B slash	//or B2T uppercut?
		return Q_T;
	}
	//Not moving at all
	return Q_R;
}

//===================================================================
qboolean PM_SaberInBounce(const int move)
{
	if (move >= LS_B1_BR && move <= LS_B1_BL)
	{
		return qtrue;
	}
	if (move >= LS_D1_BR && move <= LS_D1_BL)
	{
		return qtrue;
	}
	return qfalse;
}

int saber_moveTransitionAngle[Q_NUM_QUADS][Q_NUM_QUADS] =
{
	{
		0, //Q_BR,Q_BR,
		45, //Q_BR,Q_R,
		90, //Q_BR,Q_TR,
		135, //Q_BR,Q_T,
		180, //Q_BR,Q_TL,
		215, //Q_BR,Q_L,
		270, //Q_BR,Q_BL,
		45, //Q_BR,Q_B,
	},
	{
		45, //Q_R,Q_BR,
		0, //Q_R,Q_R,
		45, //Q_R,Q_TR,
		90, //Q_R,Q_T,
		135, //Q_R,Q_TL,
		180, //Q_R,Q_L,
		215, //Q_R,Q_BL,
		90, //Q_R,Q_B,
	},
	{
		90, //Q_TR,Q_BR,
		45, //Q_TR,Q_R,
		0, //Q_TR,Q_TR,
		45, //Q_TR,Q_T,
		90, //Q_TR,Q_TL,
		135, //Q_TR,Q_L,
		180, //Q_TR,Q_BL,
		135, //Q_TR,Q_B,
	},
	{
		135, //Q_T,Q_BR,
		90, //Q_T,Q_R,
		45, //Q_T,Q_TR,
		0, //Q_T,Q_T,
		45, //Q_T,Q_TL,
		90, //Q_T,Q_L,
		135, //Q_T,Q_BL,
		180, //Q_T,Q_B,
	},
	{
		180, //Q_TL,Q_BR,
		135, //Q_TL,Q_R,
		90, //Q_TL,Q_TR,
		45, //Q_TL,Q_T,
		0, //Q_TL,Q_TL,
		45, //Q_TL,Q_L,
		90, //Q_TL,Q_BL,
		135, //Q_TL,Q_B,
	},
	{
		215, //Q_L,Q_BR,
		180, //Q_L,Q_R,
		135, //Q_L,Q_TR,
		90, //Q_L,Q_T,
		45, //Q_L,Q_TL,
		0, //Q_L,Q_L,
		45, //Q_L,Q_BL,
		90, //Q_L,Q_B,
	},
	{
		270, //Q_BL,Q_BR,
		215, //Q_BL,Q_R,
		180, //Q_BL,Q_TR,
		135, //Q_BL,Q_T,
		90, //Q_BL,Q_TL,
		45, //Q_BL,Q_L,
		0, //Q_BL,Q_BL,
		45, //Q_BL,Q_B,
	},
	{
		45, //Q_B,Q_BR,
		90, //Q_B,Q_R,
		135, //Q_B,Q_TR,
		180, //Q_B,Q_T,
		135, //Q_B,Q_TL,
		90, //Q_B,Q_L,
		45, //Q_B,Q_BL,
		0 //Q_B,Q_B,
	}
};

static int PM_SaberAttackChainAngle(const int move1, const int move2)
{
	if (move1 == -1 || move2 == -1)
	{
		return -1;
	}
	return saber_moveTransitionAngle[saber_moveData[move1].endQuad][saber_moveData[move2].startQuad];
}

qboolean PM_SaberKataDone(const int curmove, const int newmove)
{
	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	if (pm->ps->m_iVehicleNum)
	{
		//never continue kata on vehicle
		if (pm->ps->saberAttackChainCount > MISHAPLEVEL_NONE)
		{
			return qtrue;
		}
	}

	if (pm->ps->fd.forceRageRecoveryTime > pm->cmd.serverTime)
	{
		//rage recovery, only 1 swing at a time (tired)
		if (pm->ps->saberAttackChainCount > MISHAPLEVEL_NONE)
		{
			//swung once
			return qtrue;
		}
		//allow one attack
		return qfalse;
	}
	if (pm->ps->fd.forcePowersActive & 1 << FP_RAGE)
	{
		//infinite chaining when raged
		return qfalse;
	}
	if (saber1[0].maxChain == -1)
	{
		return qfalse;
	}
	if (saber1[0].maxChain != 0)
	{
		if (pm->ps->saberAttackChainCount >= saber1[0].maxChain)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (pm->ps->fd.saberAnimLevel == SS_DESANN || pm->ps->fd.saberAnimLevel == SS_TAVION)
	{
		//desann and tavion can link up as many attacks as they want
		return qfalse;
	}
	if (pm->ps->fd.saberAnimLevel == SS_STAFF)
	{
		return qfalse;
	}
	if (pm->ps->fd.saberAnimLevel == SS_DUAL)
	{
		return qfalse;
	}
	if (pm->ps->fd.saberAnimLevel == FORCE_LEVEL_3)
	{
		if (curmove == LS_NONE || newmove == LS_NONE)
		{
			if (pm->ps->fd.saberAnimLevel >= FORCE_LEVEL_3 && pm->ps->saberAttackChainCount > Q_irand(0, 1))
			{
				return qtrue;
			}
		}
		else if (pm->ps->saberAttackChainCount > Q_irand(2, 3))
		{
			return qtrue;
		}
		else if (pm->ps->saberAttackChainCount > 0)
		{
			const int chain_angle = PM_SaberAttackChainAngle(curmove, newmove);
			if (chain_angle < 135 || chain_angle > 215)
			{
				//if trying to chain to a move that doesn't continue the momentum
				return qtrue;
			}
			if (chain_angle == 180)
			{
				//continues the momentum perfectly, allow it to chain 66% of the time
				if (pm->ps->saberAttackChainCount > 1)
				{
					return qtrue;
				}
			}
			else
			{
				//would continue the movement somewhat, 50% chance of continuing
				if (pm->ps->saberAttackChainCount > 2)
				{
					return qtrue;
				}
			}
		}
	}
	else
	{
		if ((pm->ps->fd.saberAnimLevel == FORCE_LEVEL_2 || pm->ps->fd.saberAnimLevel == SS_DUAL)
			&& pm->ps->saberAttackChainCount > Q_irand(2, 5))
		{
			return qtrue;
		}
	}
	return qfalse;
}

static void PM_SetAnimFrame(playerState_t* gent, const int frame)
{
	gent->saberLockFrame = frame;
}

static int PM_SaberLockWinAnim(const qboolean victory, const qboolean super_break)
{
	int win_anim = -1;
	switch (pm->ps->torsoAnim)
	{
	case BOTH_BF2LOCK:
		if (super_break)
		{
			win_anim = BOTH_LK_S_S_T_SB_1_W;
		}
		else if (!victory)
		{
			win_anim = BOTH_BF1BREAK;
		}
		else
		{
			pm->ps->saber_move = LS_A_T2B;
			win_anim = BOTH_A3_T__B_;
		}
		break;
	case BOTH_BF1LOCK:
		if (super_break)
		{
			win_anim = BOTH_LK_S_S_T_SB_1_W;
		}
		else if (!victory)
		{
			win_anim = BOTH_KNOCKDOWN4;
		}
		else
		{
			pm->ps->saber_move = LS_K1_T_;
			win_anim = BOTH_K1_S1_T_;
		}
		break;
	case BOTH_CWCIRCLELOCK:
		if (super_break)
		{
			win_anim = BOTH_LK_S_S_S_SB_1_W;
		}
		else if (!victory)
		{
			pm->ps->saber_move = pm->ps->saberBounceMove = LS_V1_BL;
			pm->ps->saberBlocked = BLOCKED_PARRY_BROKEN;
			win_anim = BOTH_V1_BL_S1;
		}
		else
		{
			win_anim = BOTH_CWCIRCLEBREAK;
		}
		break;
	case BOTH_CCWCIRCLELOCK:
		if (super_break)
		{
			win_anim = BOTH_LK_S_S_S_SB_1_W;
		}
		else if (!victory)
		{
			pm->ps->saber_move = pm->ps->saberBounceMove = LS_V1_BR;
			pm->ps->saberBlocked = BLOCKED_PARRY_BROKEN;
			win_anim = BOTH_V1_BR_S1;
		}
		else
		{
			win_anim = BOTH_CCWCIRCLEBREAK;
		}
		break;
	default:
		//must be using new system:
		break;
	}
	if (win_anim != -1)
	{
		PM_SetAnim(SETANIM_BOTH, win_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		pm->ps->weaponTime = pm->ps->torsoTimer;
		pm->ps->saberBlocked = BLOCKED_NONE;
		pm->ps->weaponstate = WEAPON_FIRING;
	}
	return win_anim;
}

#ifdef _GAME
extern void NPC_SetAnim(gentity_t* ent, int setAnimParts, int anim, int setAnimFlags);
extern gentity_t g_entities[];
#elif defined(_CGAME)
#include "cgame/cg_local.h"
#endif

static int PM_SaberLockLoseAnim(playerState_t* genemy, const qboolean victory, const qboolean super_break)
{
	int lose_anim = -1;

	switch (genemy->torsoAnim)
	{
	case BOTH_BF2LOCK:
		if (super_break)
		{
			lose_anim = BOTH_LK_S_S_T_SB_1_L;
		}
		else if (!victory)
		{
			lose_anim = BOTH_BF1BREAK;
		}
		else
		{
			if (!victory)
			{
				//no-one won
				genemy->saber_move = LS_K1_T_;
				lose_anim = BOTH_K1_S1_T_;
			}
			else
			{
				//FIXME: this anim needs to transition back to ready when done
				lose_anim = BOTH_BF1BREAK;
			}
		}
		break;
	case BOTH_BF1LOCK:
		if (super_break)
		{
			lose_anim = BOTH_LK_S_S_T_SB_1_L;
		}
		else if (!victory)
		{
			lose_anim = BOTH_KNOCKDOWN4;
		}
		else
		{
			if (!victory)
			{
				//no-one won
				genemy->saber_move = LS_A_T2B;
				lose_anim = BOTH_A3_T__B_;
			}
			else
			{
				lose_anim = BOTH_KNOCKDOWN4;
			}
		}
		break;
	case BOTH_CWCIRCLELOCK:
		if (super_break)
		{
			lose_anim = BOTH_LK_S_S_S_SB_1_L;
		}
		else if (!victory)
		{
			genemy->saber_move = genemy->saberBounceMove = LS_V1_BL;
			genemy->saberBlocked = BLOCKED_PARRY_BROKEN;
			lose_anim = BOTH_V1_BL_S1;
		}
		else
		{
			if (!victory)
			{
				//no-one won
				lose_anim = BOTH_CCWCIRCLEBREAK;
			}
			else
			{
				genemy->saber_move = genemy->saberBounceMove = LS_V1_BL;
				genemy->saberBlocked = BLOCKED_PARRY_BROKEN;
				lose_anim = BOTH_V1_BL_S1;
			}
		}
		break;
	case BOTH_CCWCIRCLELOCK:
		if (super_break)
		{
			lose_anim = BOTH_LK_S_S_S_SB_1_L;
		}
		else if (!victory)
		{
			genemy->saber_move = genemy->saberBounceMove = LS_V1_BR;
			genemy->saberBlocked = BLOCKED_PARRY_BROKEN;
			lose_anim = BOTH_V1_BR_S1;
		}
		else
		{
			if (!victory)
			{
				//no-one won
				lose_anim = BOTH_CWCIRCLEBREAK;
			}
			else
			{
				genemy->saber_move = genemy->saberBounceMove = LS_V1_BR;
				genemy->saberBlocked = BLOCKED_PARRY_BROKEN;
				lose_anim = BOTH_V1_BR_S1;
			}
		}
		break;
	default:;
	}
	if (lose_anim != -1)
	{
#ifdef _GAME
		NPC_SetAnim(&g_entities[genemy->clientNum], SETANIM_BOTH, lose_anim,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		genemy->weaponTime = genemy->torsoTimer; // + 250;
#endif
		genemy->saberBlocked = BLOCKED_NONE;
		genemy->weaponstate = WEAPON_READY;
	}
	return lose_anim;
}

static int PM_SaberLockResultAnim(playerState_t* duelist, const qboolean super_break, const qboolean won)
{
	int base_anim = duelist->torsoAnim;
	switch (base_anim)
	{
	case BOTH_LK_S_S_S_L_2: //lock if I'm using single vs. a single and other initiated
		base_anim = BOTH_LK_S_S_S_L_1;
		break;
	case BOTH_LK_S_S_T_L_2: //lock if I'm using single vs. a single and other initiated
		base_anim = BOTH_LK_S_S_T_L_1;
		break;
	case BOTH_LK_DL_DL_S_L_2: //lock if I'm using dual vs. dual and other initiated
		base_anim = BOTH_LK_DL_DL_S_L_1;
		break;
	case BOTH_LK_DL_DL_T_L_2: //lock if I'm using dual vs. dual and other initiated
		base_anim = BOTH_LK_DL_DL_T_L_1;
		break;
	case BOTH_LK_ST_ST_S_L_2: //lock if I'm using staff vs. a staff and other initiated
		base_anim = BOTH_LK_ST_ST_S_L_1;
		break;
	case BOTH_LK_ST_ST_T_L_2: //lock if I'm using staff vs. a staff and other initiated
		base_anim = BOTH_LK_ST_ST_T_L_1;
		break;
	default:;
	}
	//what kind of break?
	if (!super_break)
	{
		base_anim -= 2;
	}
	else if (super_break)
	{
		base_anim += 1;
	}
	else
	{
		//WTF?  Not a valid result
		return -1;
	}
	//win or lose?
	if (won)
	{
		base_anim += 1;
	}

	//play the anim and hold it
#ifdef _GAME
	//server-side: set it on the other guy, too
	if (duelist->clientNum == pm->ps->clientNum)
	{
		//me
		PM_SetAnim(SETANIM_BOTH, base_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	else
	{
		//other guy
		NPC_SetAnim(&g_entities[duelist->clientNum], SETANIM_BOTH, base_anim,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
#else
	PM_SetAnim(SETANIM_BOTH, base_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
#endif

	if (super_break
		&& !won)
	{
		//if you lose a superbreak, you're defenseless
#ifdef _GAME
		if (1)
#else
		if (duelist->clientNum == pm->ps->clientNum)
#endif
		{
			//set saber move to none
			duelist->saber_move = LS_NONE;
			//Hold the anim a little longer than it is
			duelist->torsoTimer += 250;
		}
	}

#ifdef _GAME
	if (1)
#else
	if (duelist->clientNum == pm->ps->clientNum)
#endif
	{
		//no attacking during this anim
		duelist->weaponTime = duelist->torsoTimer;
		duelist->saberBlocked = BLOCKED_NONE;
	}
	return base_anim;
}

static void PM_SaberLockBreak(playerState_t* genemy, const qboolean victory, const int strength)
{
	const qboolean super_break = strength + pm->cmd.buttons & BUTTON_ATTACK;

	pm->ps->userInt3 &= ~(1 << FLAG_SABERLOCK_ATTACKER);
	genemy->userInt3 &= ~(1 << FLAG_SABERLOCK_ATTACKER);

	int win_anim = PM_SaberLockWinAnim(victory, super_break);
	int loseAnim = PM_SaberLockLoseAnim(genemy, victory, super_break);

	if (win_anim != -1)
	{//a single vs. single break
		loseAnim = PM_SaberLockLoseAnim(genemy, victory, super_break);
	}
	else
	{//must be a saberlock that's not between single and single..
		win_anim = PM_SaberLockResultAnim(pm->ps, super_break, qtrue);
		pm->ps->weaponstate = WEAPON_FIRING;

		loseAnim = PM_SaberLockResultAnim(genemy, super_break, qfalse);
		genemy->weaponstate = WEAPON_READY;
	}

	if (victory)
	{//someone lost the lock, so punish them by knocking them down
		if (!super_break)
		{//if we're not in a superbreak, force the loser to mishap.
			pm->checkDuelLoss = genemy->clientNum + 1;
		}
	}
	else
	{//If no one lost, then shove each player away from the other
		vec3_t opp_dir;

		const int newstrength = 10;

		VectorSubtract(genemy->origin, pm->ps->origin, opp_dir);
		VectorNormalize(opp_dir);
		genemy->velocity[0] = opp_dir[0] * (newstrength * 40);
		genemy->velocity[1] = opp_dir[1] * (newstrength * 40);
		genemy->velocity[2] = 0;

		VectorSubtract(pm->ps->origin, genemy->origin, opp_dir);
		VectorNormalize(opp_dir);
		pm->ps->velocity[0] = opp_dir[0] * (newstrength * 40);
		pm->ps->velocity[1] = opp_dir[1] * (newstrength * 40);
		pm->ps->velocity[2] = 0;
	}

	pm->ps->saberLockTime = genemy->saberLockTime = 0;
	pm->ps->saberLockFrame = genemy->saberLockFrame = 0;
	pm->ps->saberLockEnemy = genemy->saberLockEnemy = 0;

	if (!victory)
	{
		//no-one won
		BG_AddPredictableEventToPlayerstate(EV_JUMP, 0, genemy);
	}
	else
	{
		if (PM_irand_timesync(0, 1))
		{
			BG_AddPredictableEventToPlayerstate(EV_JUMP, PM_irand_timesync(0, 75), genemy);
		}
	}
}

qboolean BG_CheckIncrementLockAnim(const int anim, const int win_or_lose)
{
	qboolean increment = qfalse;

	switch (anim)
	{
		//increment to win:
	case BOTH_LK_DL_DL_S_L_1: //lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_DL_S_L_2: //lock if I'm using dual vs. dual and other initiated
	case BOTH_LK_DL_DL_T_L_1: //lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_DL_T_L_2: //lock if I'm using dual vs. dual and other initiated
	case BOTH_LK_DL_S_S_L_1: //lock if I'm using dual vs. a single
	case BOTH_LK_DL_S_T_L_1: //lock if I'm using dual vs. a single
	case BOTH_LK_DL_ST_S_L_1: //lock if I'm using dual vs. a staff
	case BOTH_LK_DL_ST_T_L_1: //lock if I'm using dual vs. a staff
	case BOTH_LK_S_S_S_L_1: //lock if I'm using single vs. a single and I initiated
	case BOTH_LK_S_S_T_L_2: //lock if I'm using single vs. a single and other initiated
	case BOTH_LK_ST_S_S_L_1: //lock if I'm using staff vs. a single
	case BOTH_LK_ST_S_T_L_1: //lock if I'm using staff vs. a single
	case BOTH_LK_ST_ST_T_L_1: //lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_ST_T_L_2: //lock if I'm using staff vs. a staff and other initiated
		if (win_or_lose == SABER_LOCK_WIN)
		{
			increment = qtrue;
		}
		else
		{
			increment = qfalse;
		}
		break;

		//decrement to win:
	case BOTH_LK_S_DL_S_L_1: //lock if I'm using single vs. a dual
	case BOTH_LK_S_DL_T_L_1: //lock if I'm using single vs. a dual
	case BOTH_LK_S_S_S_L_2: //lock if I'm using single vs. a single and other initiated
	case BOTH_LK_S_S_T_L_1: //lock if I'm using single vs. a single and I initiated
	case BOTH_LK_S_ST_S_L_1: //lock if I'm using single vs. a staff
	case BOTH_LK_S_ST_T_L_1: //lock if I'm using single vs. a staff
	case BOTH_LK_ST_DL_S_L_1: //lock if I'm using staff vs. dual
	case BOTH_LK_ST_DL_T_L_1: //lock if I'm using staff vs. dual
	case BOTH_LK_ST_ST_S_L_1: //lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_ST_S_L_2: //lock if I'm using staff vs. a staff and other initiated
		if (win_or_lose == SABER_LOCK_WIN)
		{
			increment = qfalse;
		}
		else
		{
			increment = qtrue;
		}
		break;
	default:
		break;
	}
	return increment;
}

static void PM_SaberLocked(void)
{
	const bgEntity_t* e_genemy = PM_BGEntForNum(pm->ps->saberLockEnemy);

	if (!e_genemy)
	{
		return;
	}

	playerState_t* genemy = e_genemy->playerState;
	if (!genemy)
	{
		return;
	}

	PM_AddEventWithParm(EV_SABERLOCK, pm->ps->saberLockEnemy);

	if (pm->ps->saberLockFrame &&
		genemy->saberLockFrame &&
		PM_InSaberLock(pm->ps->torsoAnim) &&
		PM_InSaberLock(genemy->torsoAnim))
	{
		pm->ps->torsoTimer = 0;
		pm->ps->weaponTime = 0;
		genemy->torsoTimer = 0;
		genemy->weaponTime = 0;

		const float dist = DistanceSquared(pm->ps->origin, genemy->origin);

		if (dist < 64 || dist > 6400)
		{
			//between 8 and 80 from each other
			PM_SaberLockBreak(genemy, qfalse, 0);
#ifdef _GAME
			gentity_t* self = &g_entities[pm->ps->clientNum];
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberlockend.mp3"));
#endif
			return;
		}

#ifdef _GAME
		if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT)
		{
			if (pm->ps->saberLockAdvance)
			{
				int remaining;
				int cur_frame;
				const int strength = 1;

				pm->ps->saberLockAdvance = qfalse;

				const animation_t* anim = &pm->animations[pm->ps->torsoAnim];

				const float currentFrame = pm->ps->saberLockFrame;

				//advance/decrement my frame number
				if (BG_InSaberLockOld(pm->ps->torsoAnim))
				{
					//old locks
					if (pm->ps->torsoAnim == BOTH_CCWCIRCLELOCK ||
						pm->ps->torsoAnim == BOTH_BF2LOCK)
					{
						cur_frame = floor(currentFrame) - strength;
						//drop my frame one
						if (cur_frame <= anim->firstFrame)
						{
							//I won!  Break out
							PM_SaberLockBreak(genemy, qtrue, strength);
							gentity_t* self = &g_entities[pm->ps->clientNum];
							G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberlockend.mp3"));
							return;
						}
						PM_SetAnimFrame(pm->ps, cur_frame);
						remaining = cur_frame - anim->firstFrame;
					}
					else
					{
						cur_frame = ceil(currentFrame) + strength;
						//advance my frame one
						if (cur_frame >= anim->firstFrame + anim->numFrames)
						{
							//I won!  Break out
							PM_SaberLockBreak(genemy, qtrue, strength);
							gentity_t* self = &g_entities[pm->ps->clientNum];
							G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberlockend.mp3"));
							return;
						}
						PM_SetAnimFrame(pm->ps, cur_frame);
						remaining = anim->firstFrame + anim->numFrames - cur_frame;
					}
				}
				else
				{
					//new locks
					if (BG_CheckIncrementLockAnim(pm->ps->torsoAnim, SABER_LOCK_WIN))
					{
						cur_frame = ceil(currentFrame) + strength;
						//advance my frame one
						if (cur_frame >= anim->firstFrame + anim->numFrames)
						{
							//I won!  Break out
							PM_SaberLockBreak(genemy, qtrue, strength);
							gentity_t* self = &g_entities[pm->ps->clientNum];
							G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberlockend.mp3"));
							return;
						}
						PM_SetAnimFrame(pm->ps, cur_frame);
						remaining = anim->firstFrame + anim->numFrames - cur_frame;
					}
					else
					{
						cur_frame = floor(currentFrame) - strength;
						//drop my frame one
						if (cur_frame <= anim->firstFrame)
						{
							//I won!  Break out
							PM_SaberLockBreak(genemy, qtrue, strength);
							gentity_t* self = &g_entities[pm->ps->clientNum];
							G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberlockend.mp3"));
							return;
						}
						PM_SetAnimFrame(pm->ps, cur_frame);
						remaining = cur_frame - anim->firstFrame;
					}
				}
				//advance/decrement enemy frame number
				anim = &pm->animations[genemy->torsoAnim];

				if (BG_InSaberLockOld(genemy->torsoAnim))
				{
					if (genemy->torsoAnim == BOTH_CWCIRCLELOCK ||
						genemy->torsoAnim == BOTH_BF1LOCK)
					{
						if (!PM_irand_timesync(0, 4))
						{
							BG_AddPredictableEventToPlayerstate(Q_irand(EV_PUSHED1, EV_PUSHED3), floor((float)80 / 100 * 100.0f), genemy);
						}
						PM_SetAnimFrame(genemy, anim->firstFrame + remaining);
					}
					else
					{
						PM_SetAnimFrame(genemy, anim->firstFrame + anim->numFrames - remaining);
					}
				}
				else
				{
					//new locks
					if (BG_CheckIncrementLockAnim(genemy->torsoAnim, SABER_LOCK_LOSE))
					{
						if (!PM_irand_timesync(0, 4))
						{
							BG_AddPredictableEventToPlayerstate(EV_PAIN, floor((float)80 / 100 * 100.0f), genemy);
						}
						PM_SetAnimFrame(genemy, anim->firstFrame + anim->numFrames - remaining);
					}
					else
					{
						PM_SetAnimFrame(genemy, anim->firstFrame + remaining);
					}
				}
			}
		}
		else
#endif
		{
			if (pm->ps->saberLockAdvance && pm->cmd.buttons & BUTTON_ATTACK && pm->ps->communicatingflags & 1 << CF_SABERLOCK_ADVANCE)
			{
				//holding attack
				if (!(pm->ps->pm_flags & PMF_ATTACK_HELD))
				{
					int remaining;
					int cur_frame;
					const int strength = Q_irand(1, pm->ps->fd.forcePowerLevel[FP_SABER_OFFENSE]);

					pm->ps->saberLockAdvance = qfalse;

					const animation_t* anim = &pm->animations[pm->ps->torsoAnim];

					const float currentFrame = pm->ps->saberLockFrame;

					//advance/decrement my frame number
					if (BG_InSaberLockOld(pm->ps->torsoAnim))
					{
						//old locks
						if (pm->ps->torsoAnim == BOTH_CCWCIRCLELOCK ||
							pm->ps->torsoAnim == BOTH_BF2LOCK)
						{
							cur_frame = floor(currentFrame) - strength;
							//drop my frame one
							if (cur_frame <= anim->firstFrame)
							{
								//I won!  Break out
								PM_SaberLockBreak(genemy, qtrue, strength);
#ifdef _GAME
								gentity_t* self = &g_entities[pm->ps->clientNum];
								G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberlockend.mp3"));
#endif
								return;
							}
							PM_SetAnimFrame(pm->ps, cur_frame);
							remaining = cur_frame - anim->firstFrame;
						}
						else
						{
							cur_frame = ceil(currentFrame) + strength;
							//advance my frame one
							if (cur_frame >= anim->firstFrame + anim->numFrames)
							{
								//I won!  Break out
								PM_SaberLockBreak(genemy, qtrue, strength);
#ifdef _GAME
								gentity_t* self = &g_entities[pm->ps->clientNum];
								G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberlockend.mp3"));
#endif
								return;
							}
							PM_SetAnimFrame(pm->ps, cur_frame);
							remaining = anim->firstFrame + anim->numFrames - cur_frame;
						}
					}
					else
					{
						//new locks
						if (BG_CheckIncrementLockAnim(pm->ps->torsoAnim, SABER_LOCK_WIN))
						{
							cur_frame = ceil(currentFrame) + strength;
							//advance my frame one
							if (cur_frame >= anim->firstFrame + anim->numFrames)
							{
								//I won!  Break out
								PM_SaberLockBreak(genemy, qtrue, strength);
#ifdef _GAME
								gentity_t* self = &g_entities[pm->ps->clientNum];
								G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberlockend.mp3"));
#endif
								return;
							}
							PM_SetAnimFrame(pm->ps, cur_frame);
							remaining = anim->firstFrame + anim->numFrames - cur_frame;
						}
						else
						{
							cur_frame = floor(currentFrame) - strength;
							//drop my frame one
							if (cur_frame <= anim->firstFrame)
							{
								//I won!  Break out
								PM_SaberLockBreak(genemy, qtrue, strength);
#ifdef _GAME
								gentity_t* self = &g_entities[pm->ps->clientNum];
								G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberlockend.mp3"));
#endif
								return;
							}
							PM_SetAnimFrame(pm->ps, cur_frame);
							remaining = cur_frame - anim->firstFrame;
						}
					}
					//advance/decrement enemy frame number
					anim = &pm->animations[genemy->torsoAnim];

					if (BG_InSaberLockOld(genemy->torsoAnim))
					{
						if (genemy->torsoAnim == BOTH_CWCIRCLELOCK ||
							genemy->torsoAnim == BOTH_BF1LOCK)
						{
							if (!PM_irand_timesync(0, 4))
							{
								BG_AddPredictableEventToPlayerstate(Q_irand(EV_PUSHED1, EV_PUSHED3), floor((float)80 / 100 * 100.0f), genemy);
							}
							PM_SetAnimFrame(genemy, anim->firstFrame + remaining);
						}
						else
						{
							PM_SetAnimFrame(genemy, anim->firstFrame + anim->numFrames - remaining);
						}
					}
					else
					{
						//new locks
						if (BG_CheckIncrementLockAnim(genemy->torsoAnim, SABER_LOCK_LOSE))
						{
							if (!PM_irand_timesync(0, 4))
							{
								BG_AddPredictableEventToPlayerstate(EV_PAIN, floor((float)80 / 100 * 100.0f), genemy);
							}
							PM_SetAnimFrame(genemy, anim->firstFrame + anim->numFrames - remaining);
						}
						else
						{
							PM_SetAnimFrame(genemy, anim->firstFrame + remaining);
						}
					}
				}
			}
		}
	}
	else
	{
		//something broke us out of it
		PM_SaberLockBreak(genemy, qfalse, 0);
#ifdef _GAME
		gentity_t* self = &g_entities[pm->ps->clientNum];
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberlockend.mp3"));
#endif
	}
}

qboolean PM_SaberInBrokenParry(const int move)
{
	if (move == 139 || move == 133)
	{
		return qfalse;
	}
	if (move >= LS_V1_BR && move <= LS_V1_B_)
	{
		return qtrue;
	}
	if (move >= LS_H1_T_ && move <= LS_H1_BL)
	{
		return qtrue;
	}
	return qfalse;
}

saber_moveName_t PM_BrokenParryForParry(const int move)
{
	switch (move)
	{
	case LS_PARRY_UP:
	case LS_PARRY_FRONT:
	case LS_PARRY_WALK:
	case LS_PARRY_WALK_DUAL:
	case LS_PARRY_WALK_STAFF:
	{
		if (Q_irand(0, 1))
		{
			return LS_H1_B_;
		}
		return LS_H1_T_;
	}
	case LS_PARRY_UR:
		return LS_H1_TR;
	case LS_PARRY_UL:
		return LS_H1_TL;
	case LS_PARRY_LR:
		return LS_H1_BR;
	case LS_PARRY_LL:
		return LS_H1_BL;
	case LS_READY:
		return LS_H1_B_;
	default:;
	}
	return LS_NONE;
}

#define BACK_STAB_DISTANCE 128
static qboolean PM_CanBackstab(void)
{
	trace_t tr;
	vec3_t flat_ang;
	vec3_t fwd, back;
	const vec3_t trmins = { -15, -15, -8 };
	const vec3_t trmaxs = { 15, 15, 8 };

	VectorCopy(pm->ps->viewangles, flat_ang);
	flat_ang[PITCH] = 0;

	AngleVectors(flat_ang, fwd, 0, 0);

	back[0] = pm->ps->origin[0] - fwd[0] * BACK_STAB_DISTANCE;
	back[1] = pm->ps->origin[1] - fwd[1] * BACK_STAB_DISTANCE;
	back[2] = pm->ps->origin[2] - fwd[2] * BACK_STAB_DISTANCE;

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, back, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.fraction != 1.0 && tr.entityNum >= 0 && tr.entityNum < ENTITYNUM_NONE)
	{
		const bgEntity_t* bgEnt = PM_BGEntForNum(tr.entityNum);

		if (bgEnt && (bgEnt->s.eType == ET_PLAYER || bgEnt->s.eType == ET_NPC))
		{
			return qtrue;
		}
	}

	return qfalse;
}

saber_moveName_t PM_SaberFlipOverAttackMove(void)
{
	vec3_t fwdAngles, jumpFwd;

	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	const saberInfo_t* saber2 = BG_MySaber(pm->ps->clientNum, 1);

	//see if we have an overridden (or cancelled) lunge move
	if (saber1
		&& saber1->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber1->jumpAtkFwdMove != LS_NONE)
		{
			return saber1->jumpAtkFwdMove;
		}
	}
	if (saber2
		&& saber2->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber2->jumpAtkFwdMove != LS_NONE)
		{
			return saber2->jumpAtkFwdMove;
		}
	}
	//no overrides, cancelled?
	if (saber1
		&& saber1->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B; //LS_NONE;
	}
	if (saber2
		&& saber2->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B; //LS_NONE;
	}

	if (!PM_Can_Do_Kill_Move())
	{
		return LS_NONE;
	}
	//just do it
	VectorCopy(pm->ps->viewangles, fwdAngles);
	fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
	AngleVectors(fwdAngles, jumpFwd, NULL, NULL);

	if (pm->ps->fd.saberAnimLevel == SS_FAST || pm->ps->fd.saberAnimLevel == SS_TAVION)
	{
		VectorScale(jumpFwd, 200, pm->ps->velocity); //was 50
	}
	else
	{
		VectorScale(jumpFwd, 150, pm->ps->velocity); //was 50
	}

	pm->ps->velocity[2] = 400;

	PM_SetForceJumpZStart(pm->ps->origin[2]); //so we don't take damage if we land at same height

	PM_AddEvent(EV_JUMP);
	pm->ps->fd.forceJumpSound = 1;
	pm->cmd.upmove = 0;
	WP_ForcePowerDrain(pm->ps, FP_SABER_OFFENSE, SABER_ALT_ATTACK_POWER_FB);

	if (pm->ps->fd.saberAnimLevel == SS_FAST || pm->ps->fd.saberAnimLevel == SS_TAVION)
	{
		return LS_A_FLIP_STAB;
	}
	return LS_A_FLIP_SLASH;
}

int PM_SaberBackflipAttackMove(void)
{
	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	const saberInfo_t* saber2 = BG_MySaber(pm->ps->clientNum, 1);

	//see if we have an overridden (or cancelled) lunge move
	if (saber1
		&& saber1->jumpAtkBackMove != LS_INVALID)
	{
		if (saber1->jumpAtkBackMove != LS_NONE)
		{
			return (saber_moveName_t)saber1->jumpAtkBackMove;
		}
	}
	if (saber2
		&& saber2->jumpAtkBackMove != LS_INVALID)
	{
		if (saber2->jumpAtkBackMove != LS_NONE)
		{
			return (saber_moveName_t)saber2->jumpAtkBackMove;
		}
	}
	//no overrides, cancelled?
	if (saber1
		&& saber1->jumpAtkBackMove == LS_NONE)
	{
		return LS_A_T2B;
	}
	if (saber2
		&& saber2->jumpAtkBackMove == LS_NONE)
	{
		return LS_A_T2B;
	}
	//just do it
	pm->cmd.upmove = 127;
	pm->ps->velocity[2] = 500;
	return LS_A_BACKFLIP_ATK;
}

static int PM_SaberDualJumpAttackMove(void)
{
	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);

	pm->cmd.upmove = 0; //no jump just yet

	if (saber1 && saber1->type == SABER_GRIE4
		|| saber1 && saber1->type == SABER_GRIE
		|| saber1 && saber1->type == SABER_PALP
		&& pm->ps->fd.saberAnimLevel == SS_DUAL)
	{
		return LS_GRIEVOUS_LUNGE;
	}
	return LS_JUMPATTACK_DUAL;
}

saber_moveName_t PM_SaberLungeAttackMove(const qboolean noSpecials)
{
	vec3_t fwdAngles, jumpFwd;

	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	const saberInfo_t* saber2 = BG_MySaber(pm->ps->clientNum, 1);

	WP_ForcePowerDrain(pm->ps, FP_SABER_OFFENSE, SABER_ALT_ATTACK_POWER_FB);

	//see if we have an overridden (or cancelled) lunge move
	if (saber1
		&& saber1->lungeAtkMove != LS_INVALID)
	{
		if (saber1->lungeAtkMove != LS_NONE)
		{
			return saber1->lungeAtkMove;
		}
	}
	if (saber2
		&& saber2->lungeAtkMove != LS_INVALID)
	{
		if (saber2->lungeAtkMove != LS_NONE)
		{
			return saber2->lungeAtkMove;
		}
	}
	//no overrides, cancelled?
	if (saber1
		&& saber1->lungeAtkMove == LS_NONE)
	{
		return LS_A_T2B; //LS_NONE;
	}
	if (saber2
		&& saber2->lungeAtkMove == LS_NONE)
	{
		return LS_A_T2B; //LS_NONE;
	}
	//just do it
	saber1 = BG_MySaber(pm->ps->clientNum, 0);

	if (pm->ps->fd.saberAnimLevel == SS_FAST)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		return LS_A_LUNGE;
	}
	if (pm->ps->fd.saberAnimLevel == SS_MEDIUM)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		return LS_A_LUNGE;
	}
	if (pm->ps->fd.saberAnimLevel == SS_STRONG)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		if (saber1 && saber1->type == SABER_YODA || saber1 && saber1->type == SABER_PALP)
		{
			return LS_A_JUMP_PALP_;
		}
		return LS_A_JUMP_T__B_;
	}
	if (pm->ps->fd.saberAnimLevel == SS_DESANN)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		if (saber1 && saber1->type == SABER_YODA || saber1 && saber1->type == SABER_PALP)
		{
			return LS_A_JUMP_PALP_;
		}
		return LS_A_JUMP_T__B_;
	}
	if (pm->ps->fd.saberAnimLevel == SS_TAVION)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);

		if (pm->ps->fd.forcePower < BLOCKPOINTS_KNOCKAWAY)
		{
			return LS_A_LUNGE;
		}
		else
		{
			return LS_PULL_ATTACK_STAB;
		}
	}
	if (pm->ps->fd.saberAnimLevel == SS_DUAL)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		if (saber1 && saber1->type == SABER_GRIE)
		{
			return LS_GRIEVOUS_SPECIAL;
		}
		if (saber1 && saber1->type == SABER_GRIE4)
		{
			return LS_SPINATTACK_GRIEV;
		}
		return LS_SPINATTACK_DUAL;
	}
	if (!noSpecials && pm->ps->fd.saberAnimLevel == SS_STAFF)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		return LS_SPINATTACK;
	}
	if (!noSpecials && pm->ps->fd.saberAnimLevel == SS_STAFF && saber1->numBlades < 2)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		return LS_A_LUNGE;
	}
	if (!noSpecials && (!saber2 || saber2->numBlades == 0))
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		return LS_A_LUNGE;
	}
	if (!noSpecials)
	{
		if (saber1 && saber1->type == SABER_GRIE)
		{
			return LS_GRIEVOUS_SPECIAL;
		}
		if (saber1 && saber1->type == SABER_GRIE4)
		{
			return LS_SPINATTACK_GRIEV;
		}
		return LS_SPINATTACK_DUAL;
	}
	return LS_A_T2B;
}

#define SPECIAL_ATTACK_DISTANCE 128
qboolean PM_Can_Do_Kill_Lunge(void)
{
	trace_t tr;
	vec3_t flatAng;
	vec3_t fwd, back;
	const vec3_t trmins = { -15, -15, -8 };
	const vec3_t trmaxs = { 15, 15, 8 };

	VectorCopy(pm->ps->viewangles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	back[0] = pm->ps->origin[0] + fwd[0] * SPECIAL_ATTACK_DISTANCE;
	back[1] = pm->ps->origin[1] + fwd[1] * SPECIAL_ATTACK_DISTANCE;
	back[2] = pm->ps->origin[2] + fwd[2] * SPECIAL_ATTACK_DISTANCE;

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, back, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.fraction != 1.0 && tr.entityNum >= 0 && tr.entityNum < MAX_CLIENTS)
	{
		//We don't have real entity access here so we can't do an in depth check. But if it's a client, I guess that's reason enough to attack
		return qtrue;
	}

	return qfalse;
}

qboolean PM_Can_Do_Kill_Lunge_back(void)
{
	trace_t tr;
	vec3_t flatAng;
	vec3_t fwd, back;
	vec3_t trmins = { -15, -15, -8 };
	vec3_t trmaxs = { 15, 15, 8 };

	VectorCopy(pm->ps->viewangles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	back[0] = pm->ps->origin[0] - fwd[0] * SPECIAL_ATTACK_DISTANCE;
	back[1] = pm->ps->origin[1] - fwd[1] * SPECIAL_ATTACK_DISTANCE;
	back[2] = pm->ps->origin[2] - fwd[2] * SPECIAL_ATTACK_DISTANCE;

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, back, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.fraction != 1.0 && tr.entityNum >= 0 && (tr.entityNum < MAX_CLIENTS))
	{ //We don't have real entity access here so we can't do an indepth check. But if it's a client and it's behind us, I guess that's reason enough to stab backward
		return qtrue;
	}

	return qfalse;
}

static saber_moveName_t PM_SaberJumpAttackMove2(void)
{
	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	const saberInfo_t* saber2 = BG_MySaber(pm->ps->clientNum, 1);

	//see if we have an overridden (or cancelled) lunge move
	if (saber1
		&& saber1->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber1->jumpAtkFwdMove != LS_NONE)
		{
			return saber1->jumpAtkFwdMove;
		}
	}
	if (saber2
		&& saber2->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber2->jumpAtkFwdMove != LS_NONE)
		{
			return saber2->jumpAtkFwdMove;
		}
	}
	//no overrides, cancelled?
	if (saber1
		&& saber1->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B;
	}
	if (saber2
		&& saber2->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B;
	}
	//just do it
	if (pm->ps->fd.saberAnimLevel == SS_DUAL)
	{
		return PM_SaberDualJumpAttackMove();
	}
	if (PM_irand_timesync(0, 1))
	{
		return LS_JUMPATTACK_STAFF_LEFT;
	}
	return LS_JUMPATTACK_STAFF_RIGHT;
}

saber_moveName_t PM_SaberJumpForwardAttackMove(void)
{
	vec3_t fwdAngles, jumpFwd;

	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	const saberInfo_t* saber2 = BG_MySaber(pm->ps->clientNum, 1);

	//see if we have an overridden (or cancelled) lunge move
	if (saber1
		&& saber1->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber1->jumpAtkFwdMove != LS_NONE)
		{
			return saber1->jumpAtkFwdMove;
		}
	}
	if (saber2
		&& saber2->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber2->jumpAtkFwdMove != LS_NONE)
		{
			return saber2->jumpAtkFwdMove;
		}
	}
	//no overrides, cancelled?
	if (saber1
		&& saber1->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B; //LS_NONE;
	}
	if (saber2
		&& saber2->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B; //LS_NONE;
	}
	//just do it
	VectorCopy(pm->ps->viewangles, fwdAngles);
	fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
	AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
	VectorScale(jumpFwd, 300, pm->ps->velocity);
	pm->ps->velocity[2] = 280;
	PM_SetForceJumpZStart(pm->ps->origin[2]); //so we don't take damage if we land at same height

	PM_AddEvent(EV_JUMP);
	pm->ps->fd.forceJumpSound = 1;
	pm->cmd.upmove = 0;
	saber1 = BG_MySaber(pm->ps->clientNum, 0);

	if (saber1 && saber1->type == SABER_YODA || saber1 && saber1->type == SABER_PALP)
	{
		return LS_A_JUMP_PALP_;
	}
	return LS_A_JUMP_T__B_;
}

saber_moveName_t PM_NPC_Force_Leap_Attack(void)
{
	vec3_t fwdAngles, jumpFwd;

	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	const saberInfo_t* saber2 = BG_MySaber(pm->ps->clientNum, 1);

	//see if we have an overridden (or cancelled) lunge move
	if (saber1
		&& saber1->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber1->jumpAtkFwdMove != LS_NONE)
		{
			return saber1->jumpAtkFwdMove;
		}
	}
	if (saber2
		&& saber2->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber2->jumpAtkFwdMove != LS_NONE)
		{
			return saber2->jumpAtkFwdMove;
		}
	}
	//no overrides, cancelled?
	if (saber1
		&& saber1->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B; //LS_NONE;
	}
	if (saber2
		&& saber2->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B; //LS_NONE;
	}

	if (pm->ps->fd.saberAnimLevel == SS_DUAL || pm->ps->fd.saberAnimLevel == SS_STAFF)
	{
		pm->cmd.upmove = 0; //no jump just yet

		if (pm->ps->fd.saberAnimLevel == SS_STAFF)
		{
			if (Q_irand(0, 1))
			{
				return LS_JUMPATTACK_STAFF_LEFT;
			}
			return LS_JUMPATTACK_STAFF_RIGHT;
		}
		return LS_JUMPATTACK_DUAL;
	}
	else if (pm->ps->fd.saberAnimLevel == SS_FAST || pm->ps->fd.saberAnimLevel == SS_TAVION || pm->ps->saberAnimLevel == SS_MEDIUM)
	{
		//just do it
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity); //was 50
		pm->ps->velocity[2] = 200;

		PM_SetForceJumpZStart(pm->ps->origin[2]); //so we don't take damage if we land at same height

		PM_AddEvent(EV_JUMP);
		pm->ps->fd.forceJumpSound = 1;
		pm->cmd.upmove = 0;
		if (pm->ps->saberAnimLevel == SS_MEDIUM)
		{
			return LS_A_FLIP_SLASH;
		}
		else
		{
			return LS_A_FLIP_STAB;
		}
	}
	else
	{
		//just do it
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		pm->ps->velocity[2] = 180;
		PM_SetForceJumpZStart(pm->ps->origin[2]);//so we don't take damage if we land at same height

		PM_AddEvent(EV_JUMP);
		pm->ps->fd.forceJumpSound = 1;
		pm->cmd.upmove = 0;

		return LS_A_JUMP_T__B_;
	}
}

float PM_GroundDistance(void)
{
	trace_t tr;
	vec3_t down;

	VectorCopy(pm->ps->origin, down);

	down[2] -= 4096;

	pm->trace(&tr, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, MASK_SOLID);

	VectorSubtract(pm->ps->origin, tr.endpos, down);

	return VectorLength(down);
}

float PM_WalkableGroundDistance(void)
{
	trace_t tr;
	vec3_t down;

	VectorCopy(pm->ps->origin, down);

	down[2] -= 4096;

	pm->trace(&tr, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, MASK_SOLID);

	if (tr.plane.normal[2] < MIN_WALK_NORMAL)
	{
		//can't stand on this plane
		return 4096;
	}

	VectorSubtract(pm->ps->origin, tr.endpos, down);

	return VectorLength(down);
}

static qboolean PM_CanDoDualDoubleAttacks(void)
{
	if (pm->ps->weapon == WP_SABER)
	{
		const saberInfo_t* saber = BG_MySaber(pm->ps->clientNum, 0);
		if (saber
			&& saber->saberFlags & SFL_NO_MIRROR_ATTACKS)
		{
			return qfalse;
		}
		saber = BG_MySaber(pm->ps->clientNum, 1);
		if (saber
			&& saber->saberFlags & SFL_NO_MIRROR_ATTACKS)
		{
			return qfalse;
		}
	}
	if (pm_saber_in_special_attack(pm->ps->torsoAnim) ||
		pm_saber_in_special_attack(pm->ps->legsAnim))
	{
		return qfalse;
	}
	return qtrue;
}

qboolean G_CheckEnemyPresence(const int dir, const float radius)
{
	//anyone in this dir?
	vec3_t angles;
	vec3_t checkDir = { 0.0f };
	vec3_t interDir;
	vec3_t tTo;
	vec3_t tMins, tMaxs;
	trace_t tr;
	const float tSize = 12.0f;
	//sp uses a bbox ent list check, but.. that's not so easy/fast to
	//do in predicted code. So I'll just do a single box trace in the proper direction,
	//and take whatever is first hit.

	VectorSet(tMins, -tSize, -tSize, -tSize);
	VectorSet(tMaxs, tSize, tSize, tSize);

	VectorCopy(pm->ps->viewangles, angles);
	angles[PITCH] = 0.0f;

	switch (dir)
	{
	case DIR_RIGHT:
		AngleVectors(angles, NULL, checkDir, NULL);
		break;
	case DIR_LEFT:
		AngleVectors(angles, NULL, checkDir, NULL);
		VectorScale(checkDir, -1, checkDir);
		break;
	case DIR_FRONT:
		AngleVectors(angles, checkDir, NULL, NULL);
		break;
	case DIR_BACK:
		AngleVectors(angles, checkDir, NULL, NULL);
		VectorScale(checkDir, -1, checkDir);
		break;
	case DIR_LF:
		AngleVectors(angles, checkDir, interDir, NULL);
		VectorScale(interDir, -1, interDir);
		VectorAdd(checkDir, interDir, checkDir);
		VectorNormalize(checkDir);
		break;
	case DIR_LB:
		AngleVectors(angles, checkDir, interDir, NULL);
		VectorAdd(checkDir, interDir, checkDir);
		VectorNormalize(checkDir);
		VectorScale(checkDir, -1, checkDir);
		break;
	case DIR_RF:
		AngleVectors(angles, checkDir, interDir, NULL);
		VectorAdd(checkDir, interDir, checkDir);
		VectorNormalize(checkDir);
		break;
	case DIR_RB:
		AngleVectors(angles, checkDir, interDir, NULL);
		VectorScale(checkDir, -1, checkDir);
		VectorAdd(checkDir, interDir, checkDir);
		VectorNormalize(checkDir);
		break;
	default:;
	}

	VectorMA(pm->ps->origin, radius, checkDir, tTo);
	pm->trace(&tr, pm->ps->origin, tMins, tMaxs, tTo, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.fraction != 1.0f && tr.entityNum < ENTITYNUM_WORLD)
	{
		//let's see who we hit
		const bgEntity_t* bgEnt = PM_BGEntForNum(tr.entityNum);

		if (bgEnt &&
			(bgEnt->s.eType == ET_PLAYER || bgEnt->s.eType == ET_NPC))
		{
			//this guy can be considered an "enemy"... if he is on the same team, oh well. can't bg-check that (without a whole lot of hassle).
			return qtrue;
		}
	}

	//no one in the trace
	return qfalse;
}

saber_moveName_t PM_CheckPullAttack(void)
{
	const qboolean isPlayer = (pm->ps->clientNum < MAX_CLIENTS) ? qtrue : qfalse;

	//add pull attack swing,
	if (!(pm->cmd.buttons & BUTTON_ATTACK))
	{
		return LS_NONE;
	}

	if ((pm->ps->saber_move == LS_READY || PM_SaberInReturn(pm->ps->saber_move) || PM_SaberInReflect(pm->ps->saber_move))
		//ready
		&& pm->ps->groundEntityNum != ENTITYNUM_NONE
		&& pm->ps->fd.saberAnimLevel >= SS_FAST
		&& pm->ps->fd.saberAnimLevel <= SS_STRONG
		&& pm->ps->powerups[PW_DISINT_4] > pm->cmd.serverTime
		&& !(pm->ps->fd.forcePowersActive & 1 << FP_PULL)
		&& pm->ps->powerups[PW_PULL] > pm->cmd.serverTime
		&& pm->cmd.buttons & BUTTON_ATTACK
		&& BG_EnoughForcePowerForMove(FATIGUE_JUMPATTACK, isPlayer))
	{
		const qboolean doMove = qtrue;

		saber_moveName_t pullAttackMove;
		if (pm->ps->fd.saberAnimLevel == SS_FAST)
		{
			pullAttackMove = LS_PULL_ATTACK_STAB;
		}
		else
		{
			pullAttackMove = LS_PULL_ATTACK_SWING;
		}

		if (doMove)
		{
			WP_ForcePowerDrain(pm->ps, FP_PULL, FATIGUE_JUMPATTACK);
			return pullAttackMove;
		}
	}
	return LS_NONE;
}

static qboolean PM_InSecondarySaberStyle(void)
{
	if (pm->ps->fd.saber_anim_levelBase == SS_STAFF || pm->ps->fd.saber_anim_levelBase == SS_DUAL)
	{
		if (pm->ps->fd.saberAnimLevel != pm->ps->fd.saber_anim_levelBase)
		{
			return qtrue;
		}
	}
	return qfalse;
}

static saber_moveName_t PM_CheckPlayerAttackFromParry(const int curmove)
{
	if (curmove >= LS_PARRY_UP && curmove <= LS_REFLECT_LL)
	{
		//in a parry
		switch (saber_moveData[curmove].endQuad)
		{
		case Q_T:
			return LS_A_T2B;
		case Q_TR:
			return LS_A_TR2BL;
		case Q_TL:
			return LS_A_TL2BR;
		case Q_BR:
			return LS_A_BR2TL;
		case Q_BL:
			return LS_A_BL2TR;
		default:;
		}
	}
	return LS_NONE;
}

static saber_moveName_t PM_SaberAttackForMovement(const saber_moveName_t curmove)
{
	saber_moveName_t newmove = LS_NONE;
	const qboolean noSpecials = PM_InSecondarySaberStyle() || PM_InCartwheel(pm->ps->legsAnim);
	qboolean allowCartwheels = qtrue;
	saber_moveName_t overrideJumpRightAttackMove = LS_INVALID;
	saber_moveName_t overrideJumpLeftAttackMove = LS_INVALID;

	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	const saberInfo_t* saber2 = BG_MySaber(pm->ps->clientNum, 1);
	const qboolean isPlayer = (pm->ps->clientNum < MAX_CLIENTS) ? qtrue : qfalse;

	if (pm->ps->weapon == WP_SABER)
	{
		if (saber1
			&& saber1->jumpAtkRightMove != LS_INVALID)
		{
			if (saber1->jumpAtkRightMove != LS_NONE)
			{
				//actually overriding
				overrideJumpRightAttackMove = (saber_moveName_t)saber1->jumpAtkRightMove;
			}
			else if (saber2
				&& saber2->jumpAtkRightMove > LS_NONE)
			{
				//would be cancelling it, but check the second saber, too
				overrideJumpRightAttackMove = (saber_moveName_t)saber2->jumpAtkRightMove;
			}
			else
			{
				//nope, just cancel it
				overrideJumpRightAttackMove = LS_NONE;
			}
		}
		else if (saber2
			&& saber2->jumpAtkRightMove != LS_INVALID)
		{
			//first saber not overridden, check second
			overrideJumpRightAttackMove = (saber_moveName_t)saber2->jumpAtkRightMove;
		}

		if (saber1
			&& saber1->jumpAtkLeftMove != LS_INVALID)
		{
			if (saber1->jumpAtkLeftMove != LS_NONE)
			{
				//actually overriding
				overrideJumpLeftAttackMove = (saber_moveName_t)saber1->jumpAtkLeftMove;
			}
			else if (saber2
				&& saber2->jumpAtkLeftMove > LS_NONE)
			{
				//would be cancelling it, but check the second saber, too
				overrideJumpLeftAttackMove = (saber_moveName_t)saber2->jumpAtkLeftMove;
			}
			else
			{
				//nope, just cancel it
				overrideJumpLeftAttackMove = LS_NONE;
			}
		}
		else if (saber2
			&& saber2->jumpAtkLeftMove != LS_INVALID)
		{
			//first saber not overridden, check second
			overrideJumpLeftAttackMove = (saber_moveName_t)saber1->jumpAtkLeftMove;
		}

		if (saber1
			&& saber1->saberFlags & SFL_NO_CARTWHEELS)
		{
			allowCartwheels = qfalse;
		}
		if (saber2
			&& saber2->saberFlags & SFL_NO_CARTWHEELS)
		{
			allowCartwheels = qfalse;
		}
	}

	if (pm->cmd.rightmove > 0)
	{
		//moving right
		if (!noSpecials
			&& overrideJumpRightAttackMove != LS_NONE
			&& pm->ps->velocity[2] > 20.0f
			&& (pm->cmd.buttons & BUTTON_ATTACK && !(pm->cmd.buttons & BUTTON_BLOCK)) //hitting attack
			&& PM_GroundDistance() < 70.0f //not too high above ground
			&& (pm->cmd.upmove > 0 || pm->ps->pm_flags & PMF_JUMP_HELD) //focus-holding player
			&& BG_EnoughForcePowerForMove(SABER_ALT_ATTACK_POWER_LR, isPlayer)) //have enough power
		{
			//cartwheel right
			WP_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_LR);

			if (overrideJumpRightAttackMove != LS_INVALID)
			{
				//overridden with another move
				return overrideJumpRightAttackMove;
			}
			vec3_t right, fwdAngles;

			VectorSet(fwdAngles, 0.0f, pm->ps->viewangles[YAW], 0.0f);

			AngleVectors(fwdAngles, NULL, right, NULL);
			pm->ps->velocity[0] = pm->ps->velocity[1] = 0.0f;
			VectorMA(pm->ps->velocity, 190.0f, right, pm->ps->velocity);

			if (pm->ps->fd.saberAnimLevel == SS_STAFF)
			{
				newmove = LS_BUTTERFLY_RIGHT;
				pm->ps->velocity[2] = 350.0f;
			}
			else if (allowCartwheels)
			{
				PM_AddEvent(EV_JUMP);
				pm->ps->velocity[2] = 300.0f;
				if (1)
				{
					newmove = LS_JUMPATTACK_ARIAL_RIGHT;
				}
			}
		}
		else if (pm->cmd.forwardmove > 0)
		{
			//forward right = TL2BR slash
			newmove = LS_A_TL2BR;
		}
		else if (pm->cmd.forwardmove < 0)
		{
			//backward right = BL2TR uppercut
			newmove = LS_A_BL2TR;
		}
		else
		{
			//just right is a left slice
			newmove = LS_A_L2R;
		}
	}
	else if (pm->cmd.rightmove < 0)
	{
		//moving left
		if (!noSpecials
			&& overrideJumpLeftAttackMove != LS_NONE
			&& pm->ps->velocity[2] > 20.0f
			&& (pm->cmd.buttons & BUTTON_ATTACK && !(pm->cmd.buttons & BUTTON_BLOCK)) //hitting attack
			&& PM_GroundDistance() < 70.0f //not too high above ground
			&& (pm->cmd.upmove > 0 || pm->ps->pm_flags & PMF_JUMP_HELD) //focus-holding player
			&& BG_EnoughForcePowerForMove(SABER_ALT_ATTACK_POWER_LR, isPlayer)) //have enough power
		{
			//cartwheel left
			WP_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_LR);

			if (overrideJumpLeftAttackMove != LS_INVALID)
			{
				//overridden with another move
				return overrideJumpLeftAttackMove;
			}
			vec3_t right, fwdAngles;

			VectorSet(fwdAngles, 0.0f, pm->ps->viewangles[YAW], 0.0f);
			AngleVectors(fwdAngles, NULL, right, NULL);
			pm->ps->velocity[0] = pm->ps->velocity[1] = 0.0f;
			VectorMA(pm->ps->velocity, -190.0f, right, pm->ps->velocity);
			if (pm->ps->fd.saberAnimLevel == SS_STAFF)
			{
				newmove = LS_BUTTERFLY_LEFT;
				pm->ps->velocity[2] = 250.0f;
			}
			else if (allowCartwheels)
			{
				PM_AddEvent(EV_JUMP);
				pm->ps->velocity[2] = 350.0f;

				if (1)
				{
					newmove = LS_JUMPATTACK_ARIAL_LEFT;
				}
			}
		}
		else if (pm->cmd.forwardmove > 0)
		{
			//forward left = TR2BL slash
			newmove = LS_A_TR2BL;
		}
		else if (pm->cmd.forwardmove < 0)
		{
			//backward left = BR2TL uppercut
			newmove = LS_A_BR2TL;
		}
		else
		{
			//just left is a right slice
			newmove = LS_A_R2L;
		}
	}
	else
	{
		//not moving left or right
		if (pm->cmd.forwardmove > 0)
		{
			//forward= T2B slash
			if (!noSpecials &&
				(pm->ps->fd.saberAnimLevel == SS_DUAL || pm->ps->fd.saberAnimLevel == SS_STAFF) &&
				pm->ps->fd.forceRageRecoveryTime < pm->cmd.serverTime &&
				(pm->ps->groundEntityNum != ENTITYNUM_NONE || PM_GroundDistance() <= 40) &&
				pm->ps->velocity[2] >= 0 &&
				(pm->cmd.upmove > 0 || pm->ps->pm_flags & PMF_JUMP_HELD) &&
				!PM_SaberInTransitionAny(pm->ps->saber_move) &&
				!PM_SaberInAttack(pm->ps->saber_move) &&
				pm->ps->weaponTime <= 0 &&
				pm->ps->forceHandExtend == HANDEXTEND_NONE &&
				(pm->cmd.buttons & BUTTON_ATTACK && !(pm->cmd.buttons & BUTTON_BLOCK)) &&
				BG_EnoughForcePowerForMove(FATIGUE_JUMPATTACK, isPlayer))
			{
				//DUAL/STAFF JUMP ATTACK
				newmove = PM_SaberJumpAttackMove2();
				if (newmove != LS_A_T2B && newmove != LS_NONE)
				{
					WP_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_JUMPATTACK);
				}
			}
			else if (!noSpecials &&
				(pm->ps->fd.saberAnimLevel == SS_FAST
					|| pm->ps->fd.saberAnimLevel == SS_MEDIUM
					|| pm->ps->fd.saberAnimLevel == SS_TAVION) &&
				pm->ps->velocity[2] > 100 &&
				PM_GroundDistance() < 32 &&
				!PM_InSpecialJump(pm->ps->legsAnim) &&
				!pm_saber_in_special_attack(pm->ps->torsoAnim) &&
				BG_EnoughForcePowerForMove(FATIGUE_JUMPATTACK, isPlayer))
			{
				//FLIP AND DOWNWARD ATTACK
				newmove = PM_SaberFlipOverAttackMove();
				if (newmove != LS_A_T2B && newmove != LS_NONE)
				{
					WP_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_JUMPATTACK);
				}
			}
			else if (!noSpecials &&
				(pm->ps->fd.saberAnimLevel == SS_STRONG
					|| pm->ps->fd.saberAnimLevel == SS_DESANN) &&
				pm->ps->velocity[2] > 100 &&
				PM_GroundDistance() < 32 &&
				!PM_InSpecialJump(pm->ps->legsAnim) &&
				!pm_saber_in_special_attack(pm->ps->torsoAnim) &&
				PM_Can_Do_Kill_Move())
			{
				//DFA
				newmove = PM_SaberJumpForwardAttackMove();

				WP_ForcePowerDrain(pm->ps, FP_SABER_OFFENSE, SABER_ALT_ATTACK_POWER_FB);
			}
			else if (pm->ps->groundEntityNum != ENTITYNUM_NONE &&
				pm->ps->pm_flags & PMF_DUCKED &&
				pm->ps->weaponTime <= 0 &&
				!pm_saber_in_special_attack(pm->ps->torsoAnim) &&
				PM_Can_Do_Kill_Move())
			{
				newmove = PM_SaberLungeAttackMove(noSpecials);
			}
			else if (!noSpecials)
			{
				const saber_moveName_t stabDownMove = PM_CheckStabDown();
				if (stabDownMove != LS_NONE
					&& BG_EnoughForcePowerForMove(FATIGUE_GROUNDATTACK, isPlayer))
				{
					newmove = stabDownMove;
					WP_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_GROUNDATTACK);
				}
				else
				{
					newmove = LS_A_T2B;
				}
			}
		}
		else if (pm->cmd.forwardmove < 0)
		{
			//backward= T2B slash//B2T uppercut?
			if (!noSpecials &&
				pm->ps->fd.saberAnimLevel == SS_STAFF &&
				pm->ps->fd.forceRageRecoveryTime < pm->cmd.serverTime &&
				pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 &&
				(pm->ps->groundEntityNum != ENTITYNUM_NONE || PM_GroundDistance() <= 40) &&
				pm->ps->velocity[2] >= 0 &&
				(pm->cmd.upmove > 0 || pm->ps->pm_flags & PMF_JUMP_HELD) &&
				!PM_SaberInTransitionAny(pm->ps->saber_move) &&
				!PM_SaberInAttack(pm->ps->saber_move) &&
				pm->ps->weaponTime <= 0 &&
				pm->ps->forceHandExtend == HANDEXTEND_NONE &&
				pm->cmd.buttons & BUTTON_ATTACK)
			{
				//BACKFLIP ATTACK
				newmove = PM_SaberBackflipAttackMove();
			}
			else if (PM_CanBackstab() && !pm_saber_in_special_attack(pm->ps->torsoAnim))
			{
				//BACKSTAB (attack varies by level)
				if (pm->ps->fd.saberAnimLevel >= FORCE_LEVEL_2 && pm->ps->fd.saberAnimLevel != SS_STAFF)
				{
					//medium and higher attacks
					if (pm->ps->pm_flags & PMF_DUCKED || pm->cmd.upmove < 0)
					{
						newmove = LS_A_BACK_CR;
					}
					else
					{
						newmove = LS_A_BACK;
					}
				}
				else
				{
					//weak attack
					if (pm->ps->pm_flags & PMF_DUCKED || pm->cmd.upmove < 0)
					{
						newmove = LS_A_BACK_CR;
					}
					else
					{
						if (saber1 && (saber1->type == SABER_BACKHAND
							|| saber1->type == SABER_ASBACKHAND)) //saber backhand
						{
							newmove = LS_A_BACKSTAB_B;
						}
						else
						{
							newmove = LS_A_BACKSTAB;
						}
					}
				}
			}
			else
			{
				newmove = LS_A_T2B;
			}
		}
		else
		{
			//not moving in any direction
			if (PM_SaberInParry(curmove) || PM_SaberInBrokenParry(curmove))
			{
				//parries, return to the start position if a direction isn't given.
				newmove = LS_READY;
			}
			else if (PM_SaberInBounce(curmove) || PM_SaberInMassiveBounce(pm->ps->torsoAnim))
			{
				//bounces, parries, etc return to the start position if a direction isn't given.
#ifdef _GAME
				if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT)
				{
					newmove = LS_READY;
				}
				else
#endif
				{
					//player uses chain-attack
					newmove = saber_moveData[curmove].chain_attack;
				}
				if (PM_SaberKataDone(curmove, newmove))
				{
					return saber_moveData[curmove].chain_idle;
				}
				return newmove;
			}
			else if (PM_SaberInKnockaway(curmove))
			{//bounces should go to their default attack if you don't specify a direction but are attacking
#ifdef _GAME
				qboolean  bot = (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT);
				qboolean  npc = (pm_entSelf->s.eType == ET_NPC);

				if (bot || npc && Q_irand(0, 1)) // 50% chance
				{
					newmove = PM_NPCSaberAttackFromQuad(saber_moveData[curmove].endQuad);
				}
				else
#endif
				{
					if (pm->ps->fd.saberAnimLevel == SS_FAST || pm->ps->fd.saberAnimLevel == SS_TAVION)
					{
						//player is in fast attacks, so come right back down from the same spot
						newmove = PM_AttackMoveForQuad(saber_moveData[curmove].endQuad);
					}
					else
					{
						//use a transition to wrap to another attack from a different dir
						newmove = saber_moveData[curmove].chain_attack;
					}
				}
				if (PM_SaberKataDone(curmove, newmove))
				{
					return saber_moveData[curmove].chain_idle;
				}
				return newmove;
			}
			else if (curmove == LS_A_FLIP_STAB
				|| curmove == LS_A_FLIP_SLASH
				|| curmove >= LS_PARRY_UP && curmove <= LS_REFLECT_LL)
			{
				//Not moving at all, not too busy to attack
				//checked all special attacks, if we're in a parry, attack from that move
				const saber_moveName_t parryAttackMove = PM_CheckPlayerAttackFromParry(curmove);

				if (parryAttackMove != LS_NONE)
				{
					return parryAttackMove;
				}
				//check regular attacks
				if (fabs(pm->ps->viewangles[0]) > 30)
				{
					//looking far up or far down uses the top to bottom attack, presuming you want a vertical attack
					return LS_A_T2B;
				}
				//for now, just pick a random attack
				return Q_irand(LS_A_TL2BR, LS_A_T2B);
			}
			else if (curmove == LS_READY)
			{
				return Q_irand(LS_A_TL2BR, LS_A_T2B);
			}
		}
	}

	if (pm->ps->fd.saberAnimLevel == SS_DUAL)
	{
		if ((newmove == LS_A_R2L || newmove == LS_S_R2L
			|| newmove == LS_A_L2R || newmove == LS_S_L2R)
			&& PM_CanDoDualDoubleAttacks()
			&& G_CheckEnemyPresence(DIR_RIGHT, 100.0f)
			&& G_CheckEnemyPresence(DIR_LEFT, 100.0f))
		{
			//enemy both on left and right
			newmove = LS_DUAL_LR;
			//probably already moved, but...
			pm->cmd.rightmove = 0;
		}
		else if ((newmove == LS_A_T2B || newmove == LS_S_T2B
			|| newmove == LS_A_BACK || newmove == LS_A_BACK_CR)
			&& PM_CanDoDualDoubleAttacks()
			&& G_CheckEnemyPresence(DIR_FRONT, 100.0f)
			&& G_CheckEnemyPresence(DIR_BACK, 100.0f))
		{
			//enemy both in front and back
			newmove = LS_DUAL_FB;
			//probably already moved, but...
			pm->cmd.forwardmove = 0;
		}
	}

	return newmove;
}

static qboolean PM_CheckUpsideDownAttack(void)
{
	if (pm->ps->saber_move != LS_READY)
	{
		return qfalse;
	}
	if (!(pm->cmd.buttons & BUTTON_ATTACK))
	{
		return qfalse;
	}
	if (pm->ps->fd.saberAnimLevel < SS_FAST || pm->ps->fd.saberAnimLevel > SS_STRONG)
	{
		return qfalse;
	}
	if (pm->ps->clientNum >= MAX_CLIENTS)
	{
		//FIXME: check ranks?
		return qfalse;
	}

	switch (pm->ps->legsAnim)
	{
	case BOTH_WALL_RUN_RIGHT_FLIP:
	case BOTH_WALL_RUN_LEFT_FLIP:
	case BOTH_WALL_FLIP_RIGHT:
	case BOTH_WALL_FLIP_LEFT:
	case BOTH_FLIP_BACK1:
	case BOTH_FLIP_BACK2:
	case BOTH_FLIP_BACK3:
	case BOTH_WALL_FLIP_BACK1:
	case BOTH_ALORA_FLIP_B:
	case BOTH_FORCEWALLRUNFLIP_END:
	{
		const float anim_length = PM_AnimLength((animNumber_t)pm->ps->legsAnim);
		const float elapsed_time = anim_length - pm->ps->legsTimer;
		const float mid_point = anim_length / 2.0f;
		if (elapsed_time < mid_point - 100.0f
			|| elapsed_time > mid_point + 100.0f)
		{
			//only a 200ms window (in middle of anim) of opportunity to do this move in these anims
			return qfalse;
		}
	}
	case BOTH_FLIP_HOLD7:
		pm->ps->pm_flags |= PMF_SLOW_MO_FALL;
		PM_SetSaberMove(LS_UPSIDE_DOWN_ATTACK);
		return qtrue;
	default:;
	}
	return qfalse;
}

static void PM_TryAirKick(const saber_moveName_t kick_move)
{
	if (pm->ps->groundEntityNum < ENTITYNUM_NONE)
	{
		//just do it
		PM_SetSaberMove(kick_move);
	}
	else
	{
		const float gDist = PM_GroundDistance();
		//let's only allow air kicks if a certain distance from the ground
		//it's silly to be able to do them right as you land.
		//also looks wrong to transition from a non-complete flip anim...
		if ((!PM_FlippingAnim(pm->ps->legsAnim) || pm->ps->legsTimer <= 0) &&
			gDist > 64.0f && //strict minimum
			gDist > -pm->ps->velocity[2] - 64.0f)
			//make sure we are high to ground relative to downward velocity as well
		{
			PM_SetSaberMove(kick_move);
		}
		else
		{
			//leave it as a normal kick unless we're too high up
			if (gDist > 128.0f || pm->ps->velocity[2] >= 0)
			{
				//off ground, but too close to ground
			}
			else
			{
				//high close enough to ground to do a normal kick, convert it
				switch (kick_move)
				{
				case LS_KICK_F_AIR:
					PM_SetSaberMove(LS_KICK_F);
					break;
				case LS_KICK_F_AIR2:
					PM_SetSaberMove(LS_KICK_F2);
					break;
				case LS_KICK_B_AIR:
					PM_SetSaberMove(LS_KICK_B);
					break;
				case LS_KICK_R_AIR:
					PM_SetSaberMove(LS_KICK_R);
					break;
				case LS_KICK_L_AIR:
					PM_SetSaberMove(LS_KICK_L);
					break;
				default:
					break;
				}
			}
		}
	}
}

int PM_CheckKick(void)
{
	int kick_move = -1;

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
		if (pm->cmd.rightmove)
		{
			//kick to side
			if (pm->cmd.rightmove > 0)
			{
				//kick right
				if (pm->ps->groundEntityNum == ENTITYNUM_NONE || pm->cmd.upmove > 0)
				{
					PM_TryAirKick(LS_KICK_R_AIR);
				}
				else
				{
					if (pm->ps->weapon == WP_SABER && !BG_SabersOff(pm->ps))
					{
						kick_move = LS_SLAP_R;
					}
					else
					{
						kick_move = LS_KICK_R;
					}
				}
			}
			else
			{
				//kick left
				if (pm->ps->groundEntityNum == ENTITYNUM_NONE || pm->cmd.upmove > 0)
				{
					PM_TryAirKick(LS_KICK_L_AIR);
				}
				else
				{
					if (pm->ps->weapon == WP_SABER && !BG_SabersOff(pm->ps))
					{
						kick_move = LS_SLAP_L;
					}
					else
					{
						kick_move = LS_KICK_L;
					}
				}
			}
			pm->cmd.rightmove = 0;
		}
		else if (pm->cmd.forwardmove)
		{
			//kick front/back
			if (pm->cmd.forwardmove > 0)
			{
				//kick fwd
				if (pm->ps->groundEntityNum == ENTITYNUM_NONE || pm->cmd.upmove > 0)
				{
					PM_TryAirKick(LS_KICK_F_AIR);
				}
				else if (pm->ps->groundEntityNum != ENTITYNUM_NONE && pm->ps->weapon == WP_SABER && !
					BG_SabersOff(pm->ps))
				{
					kick_move = LS_KICK_F2;
				}
				else
				{
					kick_move = LS_KICK_F;
				}
			}
			else if (pm->ps->groundEntityNum == ENTITYNUM_NONE || pm->cmd.upmove > 0)
			{
				PM_TryAirKick(LS_KICK_B_AIR);
			}
			else
			{
				//kick back
				if (PM_CrouchingAnim(pm->ps->legsAnim))
				{
					kick_move = LS_KICK_B3;
				}
				else
				{
					kick_move = LS_KICK_B2;
				}
			}
			pm->cmd.forwardmove = 0;
		}
		else
		{
		}
	}

	return kick_move;
}

int PM_MeleeMoveForConditions(void)
{
	int kick_move = -1;

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
		if (pm->cmd.rightmove)
		{
			//kick to side
			if (pm->cmd.rightmove > 0)
			{
				//kick right
				if (pm->ps->groundEntityNum == ENTITYNUM_NONE || pm->cmd.upmove > 0)
				{
					PM_TryAirKick(LS_KICK_R_AIR);
				}
				else
				{
					if (pm->ps->weapon == WP_SABER && !BG_SabersOff(pm->ps))
					{
						kick_move = LS_SLAP_R;
					}
					else
					{
						kick_move = LS_KICK_R;
					}
				}
			}
			else
			{
				//kick left
				if (pm->ps->groundEntityNum == ENTITYNUM_NONE || pm->cmd.upmove > 0)
				{
					PM_TryAirKick(LS_KICK_L_AIR);
				}
				else
				{
					if (pm->ps->weapon == WP_SABER && !BG_SabersOff(pm->ps))
					{
						kick_move = LS_SLAP_L;
					}
					else
					{
						kick_move = LS_KICK_L;
					}
				}
			}
			pm->cmd.rightmove = 0;
		}
		else if (pm->cmd.forwardmove)
		{
			//kick front/back
			if (pm->cmd.forwardmove > 0)
			{
				//kick fwd
				if (pm->ps->groundEntityNum == ENTITYNUM_NONE || pm->cmd.upmove > 0)
				{
					PM_TryAirKick(LS_KICK_F_AIR);
				}
				else if (pm->ps->groundEntityNum != ENTITYNUM_NONE && pm->ps->weapon == WP_SABER && !
					BG_SabersOff(pm->ps))
				{
					kick_move = LS_KICK_F2;
				}
				else
				{
					kick_move = LS_KICK_F;
				}
			}
			else if (pm->ps->groundEntityNum == ENTITYNUM_NONE || pm->cmd.upmove > 0)
			{
				PM_TryAirKick(LS_KICK_B_AIR);
			}
			else
			{
				//kick back
				if (PM_CrouchingAnim(pm->ps->legsAnim))
				{
					kick_move = LS_KICK_B3;
				}
				else
				{
					kick_move = LS_KICK_B2;
				}
			}
			pm->cmd.forwardmove = 0;
		}
		else
		{
		}
	}

	return kick_move;
}

qboolean PM_InSlopeAnim(int anim);
qboolean PM_RunningAnim(int anim);

static qboolean PM_saber_moveOkayForKata(void)
{
	if (pm->ps->saber_move == LS_READY
		|| PM_SaberInReflect(pm->ps->saber_move)
		|| PM_SaberInStart(pm->ps->saber_move))
	{
		return qtrue;
	}
	return qfalse;
}

static qboolean PM_CanDoKata(void)
{
	const qboolean isPlayer = (pm->ps->clientNum < MAX_CLIENTS) ? qtrue : qfalse;

	const qboolean is_holding_block_button = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Holding Block Button
	const qboolean is_holding_block_button_and_attack = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
	//Active Blocking
	const qboolean walking_blocking = pm->ps->ManualBlockingFlags & 1 << MBF_BLOCKWALKING ? qtrue : qfalse;
	//Walking Blocking

	if (is_holding_block_button || is_holding_block_button_and_attack || walking_blocking)
	{
		return qfalse;
	}

	if (!pm->ps->saberInFlight //not throwing saber
		&& PM_saber_moveOkayForKata()
		&& !PM_SaberInKata(pm->ps->saber_move)
		&& !PM_InKataAnim(pm->ps->legsAnim)
		&& !PM_InKataAnim(pm->ps->torsoAnim)
		&& pm->ps->groundEntityNum != ENTITYNUM_NONE //not in the air
		&& pm->cmd.buttons & BUTTON_ATTACK //pressing attack
		&& pm->cmd.buttons & BUTTON_ALT_ATTACK //pressing alt attack
		&& !pm->cmd.forwardmove //not moving f/b
		&& !pm->cmd.rightmove //not moving r/l
		&& pm->cmd.upmove <= 0 //not jumping...?
		&& BG_EnoughForcePowerForMove(SABER_KATA_ATTACK_POWER, isPlayer)) // have enough power
	{
		return qtrue;
	}
	return qfalse;
}

static qboolean PM_SaberThrowable(void)
{
	const saberInfo_t* saber = BG_MySaber(pm->ps->clientNum, 0);
	if (!saber)
	{
		//this is bad, just drop out.
		return qfalse;
	}

	if (!(saber->saberFlags & SFL_NOT_THROWABLE))
	{
		//yes, this saber is always throwable
		return qtrue;
	}

	//saber is not normally throwable
	if (saber->saberFlags & SFL_SINGLE_BLADE_THROWABLE)
	{
		//it is throwable if only one blade is on
		if (saber->numBlades > 1)
		{
			//it has more than one blade
			int i = 0;
			int numBladesActive = 0;
			for (; i < saber->numBlades; i++)
			{
				if (saber->blade[i].active)
				{
					numBladesActive++;
				}
			}
			if (numBladesActive == 1)
			{
				//only 1 blade is on
				return qtrue;
			}
		}
	}
	//nope, can't throw it
	return qfalse;
}

static qboolean PM_CheckAltKickAttack(void)
{
	if ((pm->cmd.buttons & BUTTON_ALT_ATTACK || pm->cmd.buttons & BUTTON_KICK)
		&& (!PM_FlippingAnim(pm->ps->legsAnim) || pm->ps->legsTimer <= 250) && !(pm->cmd.buttons & BUTTON_DASH))
	{
		return qtrue;
	}
	return qfalse;
}

int bg_parryDebounce[NUM_FORCE_POWER_LEVELS] =
{
	500, //if don't even have defense, can't use defense!
	300,
	150,
	50
};

static qboolean PM_SaberPowerCheck(void)
{
	const qboolean isPlayer = (pm->ps->clientNum < MAX_CLIENTS) ? qtrue : qfalse;

	if (pm->ps->saberInFlight)
	{
		//so we don't keep doing stupid force out thing while guiding saber.
		if (pm->ps->fd.forcePower > forcePowerNeeded[pm->ps->fd.forcePowerLevel[FP_SABERTHROW]][FP_SABERTHROW])
		{
			return qtrue;
		}
	}
	else
	{
		return BG_EnoughForcePowerForMove(forcePowerNeeded[pm->ps->fd.forcePowerLevel[FP_SABERTHROW]][FP_SABERTHROW], isPlayer);
	}

	return qfalse;
}

static qboolean PM_CanDoRollStab(void)
{
	if (pm->ps->weapon == WP_SABER && !(pm->ps->userInt3 & 1 << FLAG_DODGEROLL))
	{
		const saberInfo_t* saber = BG_MySaber(pm->ps->clientNum, 0);
		if (saber
			&& saber->saberFlags & SFL_NO_ROLL_STAB)
		{
			return qfalse;
		}
		saber = BG_MySaber(pm->ps->clientNum, 1);
		if (saber
			&& saber->saberFlags & SFL_NO_ROLL_STAB)
		{
			return qfalse;
		}
	}
	else
	{
		return qfalse;
	}
	return qtrue;
}

qboolean PM_Can_Do_Kill_Move(void)
{
	const qboolean isPlayer = (pm->ps->clientNum < MAX_CLIENTS) ? qtrue : qfalse;

	if (!pm->ps->saberInFlight //not throwing saber
		&& pm->cmd.buttons & BUTTON_ATTACK //pressing attack
		&& pm->cmd.forwardmove >= 0 //not moving back (used to be !pm->cmd.forwardmove)
		&& !pm->cmd.rightmove //not moving r/l
		&& BG_EnoughForcePowerForMove(SABER_ALT_ATTACK_POWER_FB, isPlayer)) // have enough power
	{
		return qtrue;
	}
	return qfalse;
}

static qboolean InSaberDelayAnimation(const int move)
{
	if (move >= 665 && move <= 669
		|| move >= 690 && move <= 694
		|| move >= 715 && move <= 719)
	{
		return qtrue;
	}
	return qfalse;
}

void PM_SetMeleeBlock(void)
{
	int anim = -1;

	if (pm->ps->weapon != WP_MELEE)
	{
		return;
	}

	if (pm->cmd.rightmove || pm->cmd.forwardmove)  //pushing a direction
	{
		if (pm->cmd.rightmove > 0)
		{//lean right
			if (pm->cmd.forwardmove > 0)
			{//lean forward right
				if (pm->ps->torsoAnim == MELEE_STANCE_HOLD_RT)
				{
					anim = pm->ps->torsoAnim;
				}
				else
				{
					anim = MELEE_STANCE_RT;
				}
				pm->cmd.rightmove = 0;
				pm->cmd.forwardmove = 0;
			}
			else if (pm->cmd.forwardmove < 0)
			{//lean backward right
				if (pm->ps->torsoAnim == MELEE_STANCE_HOLD_BR)
				{
					anim = pm->ps->torsoAnim;
				}
				else
				{
					anim = MELEE_STANCE_BR;
				}
				pm->cmd.rightmove = 0;
				pm->cmd.forwardmove = 0;
			}
			else
			{//lean right
				if (pm->ps->torsoAnim == MELEE_STANCE_HOLD_RT)
				{
					anim = pm->ps->torsoAnim;
				}
				else
				{
					anim = MELEE_STANCE_RT;
				}
			}
			pm->cmd.rightmove = 0;
			pm->cmd.forwardmove = 0;
		}
		else if (pm->cmd.rightmove < 0)
		{//lean left
			if (pm->cmd.forwardmove > 0)
			{//lean forward left
				if (pm->ps->torsoAnim == MELEE_STANCE_HOLD_LT)
				{
					anim = pm->ps->torsoAnim;
				}
				else
				{
					anim = MELEE_STANCE_LT;
				}
				pm->cmd.rightmove = 0;
				pm->cmd.forwardmove = 0;
			}
			else if (pm->cmd.forwardmove < 0)
			{//lean backward left
				if (pm->ps->torsoAnim == MELEE_STANCE_HOLD_BL)
				{
					anim = pm->ps->torsoAnim;
				}
				else
				{
					anim = MELEE_STANCE_BL;
				}
				pm->cmd.rightmove = 0;
				pm->cmd.forwardmove = 0;
			}
			else
			{//lean left
				if (pm->ps->torsoAnim == MELEE_STANCE_HOLD_LT)
				{
					anim = pm->ps->torsoAnim;
				}
				else
				{
					anim = MELEE_STANCE_LT;
				}
			}
			pm->cmd.rightmove = 0;
			pm->cmd.forwardmove = 0;
		}
		else
		{//not pressing either side
			if (pm->cmd.forwardmove > 0)
			{//lean forward
				if (PM_MeleeblockAnim(pm->ps->torsoAnim))
				{
					anim = pm->ps->torsoAnim;
				}
				else if (Q_irand(0, 1))
				{
					anim = MELEE_STANCE_T;
				}
				else
				{
					anim = MELEE_STANCE_T;
				}
				pm->cmd.rightmove = 0;
				pm->cmd.forwardmove = 0;
			}
			else if (pm->cmd.forwardmove < 0)
			{//lean backward
				if (PM_MeleeblockAnim(pm->ps->torsoAnim))
				{
					anim = pm->ps->torsoAnim;
				}
				else if (Q_irand(0, 1))
				{
					anim = MELEE_STANCE_B;
				}
				else
				{
					anim = MELEE_STANCE_B;
				}
				pm->cmd.rightmove = 0;
				pm->cmd.forwardmove = 0;
			}
			pm->cmd.rightmove = 0;
			pm->cmd.forwardmove = 0;
		}
		if (anim != -1)
		{
			int extraHoldTime = 0;
			if (PM_MeleeblockAnim(pm->ps->torsoAnim) && !PM_MeleeblockHoldAnim(pm->ps->torsoAnim))
			{//use the hold pose, don't start it all over again
				anim = MELEE_STANCE_HOLD_LT + (anim - MELEE_STANCE_LT);
				extraHoldTime = 600;
			}
			if (anim == pm->ps->torsoAnim)
			{
				if (pm->ps->torsoTimer < 600)
				{
					pm->ps->torsoTimer = 600;
				}
			}
			else
			{
				PM_SetAnim(SETANIM_TORSO, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->cmd.forwardmove = 0;
				pm->cmd.rightmove = 0;
				pm->cmd.upmove = 0;
			}
			if (extraHoldTime && pm->ps->torsoTimer < extraHoldTime)
			{
				pm->ps->torsoTimer += extraHoldTime;
				pm->cmd.forwardmove = 0;
				pm->cmd.rightmove = 0;
				pm->cmd.upmove = 0;
			}
			if (pm->ps->groundEntityNum != ENTITYNUM_NONE && !pm->cmd.upmove)
			{
				PM_SetAnim(SETANIM_LEGS, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->legsTimer = pm->ps->torsoTimer;
				pm->cmd.forwardmove = 0;
				pm->cmd.rightmove = 0;
				pm->cmd.upmove = 0;
			}
			else
			{
				PM_SetAnim(SETANIM_LEGS, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->cmd.forwardmove = 0;
				pm->cmd.rightmove = 0;
				pm->cmd.upmove = 0;
			}
			pm->ps->weaponTime = pm->ps->torsoTimer;
			pm->cmd.forwardmove = 0;
			pm->cmd.rightmove = 0;
			pm->cmd.upmove = 0;
		}
	}
}

int PM_ReturnforQuad(const int quad)
{
	switch (quad)
	{
	case Q_BR:
		return LS_R_TL2BR;
	case Q_R:
		return LS_R_L2R;
	case Q_TR:
		return LS_R_BL2TR;
	case Q_T:
	{
		if (!Q_irand(0, 1))
		{
			return LS_R_BL2TR;
		}
		return LS_R_BR2TL;
	}
	case Q_TL:
		return LS_R_BR2TL;
	case Q_L:
		return LS_R_R2L;
	case Q_BL:
		return LS_R_TR2BL;
	case Q_B:
		return LS_R_T2B;
	default:
		return LS_READY;
	}
}

int blockedfor_quad(const int quad)
{
	//returns the saberBlocked direction for given quad.
	switch (quad)
	{
	case Q_BR:
		return BLOCKED_LOWER_RIGHT;
	case Q_R:
		return BLOCKED_UPPER_RIGHT;
	case Q_TR:
		return BLOCKED_UPPER_RIGHT;
	case Q_T:
		return BLOCKED_TOP;
	case Q_TL:
		return BLOCKED_UPPER_LEFT;
	case Q_L:
		return BLOCKED_UPPER_LEFT;
	case Q_BL:
		return BLOCKED_LOWER_LEFT;
	case Q_B:
		return BLOCKED_LOWER_LEFT;
	default:
		return BLOCKED_FRONT;
	}
}

static saber_moveName_t PM_DoFake(const int curmove)
{
	int newQuad = -1;

	if (pm->ps->userInt3 & 1 << FLAG_ATTACKFAKE)
	{
		//already attack faking, can't do another one until this one is over.
		return LS_NONE;
	}

	if (pm->cmd.rightmove > 0)
	{
		//moving right
		if (pm->cmd.forwardmove > 0)
		{
			//forward right = TL2BR slash
			newQuad = Q_TL;
		}
		else if (pm->cmd.forwardmove < 0)
		{
			//backward right = BL2TR uppercut
			newQuad = Q_BL;
		}
		else
		{
			//just right is a left slice
			newQuad = Q_L;
		}
	}
	else if (pm->cmd.rightmove < 0)
	{
		//moving left
		if (pm->cmd.forwardmove > 0)
		{
			//forward left = TR2BL slash
			newQuad = Q_TR;
		}
		else if (pm->cmd.forwardmove < 0)
		{
			//backward left = BR2TL uppercut
			newQuad = Q_BR;
		}
		else
		{
			//just left is a right slice
			newQuad = Q_R;
		}
	}
	else
	{
		//not moving left or right
		if (pm->cmd.forwardmove > 0)
		{
			//forward= T2B slash
			newQuad = Q_T;
		}
		else if (pm->cmd.forwardmove < 0)
		{
			//backward= T2B slash	//or B2T uppercut?
			newQuad = Q_T;
		}
		else
		{
			//Not moving at all
		}
	}

	if (newQuad == -1)
	{
		//assume that we're trying to fake in our current direction so we'll automatically fake
		//in the completely opposite direction.  This allows the player to do a fake while standing still.
		newQuad = saber_moveData[curmove].endQuad;
	}

	if (newQuad == saber_moveData[curmove].endQuad)
	{
		//player is attempting to do a fake move to the same quadrant
		//as such, fake to the completely opposite quad
		newQuad += 4;
		if (newQuad > Q_B)
		{
			//rotated past Q_B, shift back to get the proper quadrant
			newQuad -= Q_NUM_QUADS;
		}
	}

	if (newQuad == Q_B)
	{
		//attacks can't be launched from this quad, just randomly fake to the bottom left/right
		if (PM_irand_timesync(0, 9) <= 4)
		{
			newQuad = Q_BL;
		}
		else
		{
			newQuad = Q_BR;
		}
	}

	//add faking flag
	pm->ps->userInt3 |= 1 << FLAG_ATTACKFAKE;
	return transitionMove[saber_moveData[curmove].endQuad][newQuad];
}

saber_moveName_t PM_DoAI_Fake(const int curmove)
{
	int newQuad = -1;

	if (in_camera)
	{
		return LS_NONE;
	}

	if (pm->cmd.rightmove > 0)
	{
		//moving right
		if (pm->cmd.forwardmove > 0)
		{
			//forward right = TL2BR slash
			newQuad = Q_TL;
		}
		else if (pm->cmd.forwardmove < 0)
		{
			//backward right = BL2TR uppercut
			newQuad = Q_BL;
		}
		else
		{
			//just right is a left slice
			newQuad = Q_L;
		}
	}
	else if (pm->cmd.rightmove < 0)
	{
		//moving left
		if (pm->cmd.forwardmove > 0)
		{
			//forward left = TR2BL slash
			newQuad = Q_TR;
		}
		else if (pm->cmd.forwardmove < 0)
		{
			//backward left = BR2TL uppercut
			newQuad = Q_BR;
		}
		else
		{
			//just left is a right slice
			newQuad = Q_R;
		}
	}
	else
	{
		//not moving left or right
		if (pm->cmd.forwardmove > 0)
		{
			//forward= T2B slash
			newQuad = Q_T;
		}
		else if (pm->cmd.forwardmove < 0)
		{
			//backward= T2B slash	//or B2T uppercut?
			newQuad = Q_T;
		}
		else
		{
			//Not moving at all
		}
	}

	if (newQuad == -1)
	{
		//assume that we're trying to fake in our current direction so we'll automatically fake
		//in the completely opposite direction.  This allows the player to do a fake while standing still.
		newQuad = saber_moveData[curmove].endQuad;
	}

	if (newQuad == saber_moveData[curmove].endQuad)
	{
		//player is attempting to do a fake move to the same quadrant
		//as such, fake to the completely opposite quad
		newQuad += 4;
		if (newQuad > Q_B)
		{
			//rotated past Q_B, shift back to get the proper quadrant
			newQuad -= Q_NUM_QUADS;
		}
	}

	if (newQuad == Q_B)
	{
		//attacks can't be launched from this quad, just randomly fake to the bottom left/right
		if (PM_irand_timesync(0, 9) <= 4)
		{
			newQuad = Q_BL;
		}
		else
		{
			newQuad = Q_BR;
		}
	}

	//add faking flag
	pm->ps->userInt3 |= 1 << FLAG_ATTACKFAKE;
	return transitionMove[saber_moveData[curmove].endQuad][newQuad];
}

static void PM_SaberDroidWeapon(void)
{
	// make weapon function
	if (pm->ps->weaponTime > 0)
	{
		pm->ps->weaponTime -= pml.msec;
		if (pm->ps->weaponTime <= 0)
		{
			pm->ps->weaponTime = 0;
		}
	}

	// Now saberdroids react to a block action by the player's lightsaber.
	if (pm->ps->saberBlocked)
	{
		switch (pm->ps->saberBlocked)
		{
		case BLOCKED_PARRY_BROKEN:
			PM_SetAnim(SETANIM_BOTH, Q_irand(BOTH_PAIN1, BOTH_PAIN3), SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_ATK_BOUNCE:
			PM_SetAnim(SETANIM_BOTH, Q_irand(BOTH_PAIN1, BOTH_PAIN3), SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_UPPER_RIGHT:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_TR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_UPPER_RIGHT_PROJ:
			PM_SetAnim(SETANIM_BOTH, BOTH_BLOCKATTACK_RIGHT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_LOWER_RIGHT:
		case BLOCKED_LOWER_RIGHT_PROJ:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_BR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_UPPER_LEFT_PROJ:
			PM_SetAnim(SETANIM_BOTH, BOTH_BLOCKATTACK_LEFT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_UPPER_LEFT:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_TL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_LOWER_LEFT:
		case BLOCKED_LOWER_LEFT_PROJ:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_BL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_TOP:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_T_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_TOP_PROJ:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_T_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_FRONT:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_T1_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_FRONT_PROJ:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_T1_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_BLOCKATTACK_LEFT:
			PM_SetAnim(SETANIM_BOTH, BOTH_BLOCKATTACK_LEFT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_BLOCKATTACK_RIGHT:
			PM_SetAnim(SETANIM_BOTH, BOTH_BLOCKATTACK_RIGHT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_BLOCKATTACK_FRONT:
			PM_SetAnim(SETANIM_BOTH, BOTH_P7_S7_T_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_BACK:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_B1_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		default:
			pm->ps->saberBlocked = BLOCKED_NONE;
			break;
		}

		pm->ps->saberBlocked = BLOCKED_NONE;
		pm->ps->saber_move = LS_NONE;
		pm->ps->weaponstate = WEAPON_READY;
	}
}

void PM_CheckClearSaberBlock(void)
{
#ifdef _GAME
	if (!(g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC))
#endif
	{
		//player
		if (pm->ps->saberBlocked >= BLOCKED_FRONT && pm->ps->saberBlocked <= BLOCKED_FRONT_PROJ)
		{
			//blocking a projectile
			if (pm->ps->fd.forcePowerDebounce[FP_SABER_DEFENSE] < pm->cmd.serverTime)
			{
				//block is done or breaking out of it with an attack
				pm->ps->weaponTime = 0;
				pm->ps->saberBlocked = BLOCKED_NONE;
			}
			else if (pm->cmd.buttons & BUTTON_ATTACK && !(pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCK))
			{
				//block is done or breaking out of it with an attack
				pm->ps->weaponTime = 0;
				pm->ps->saberBlocked = BLOCKED_NONE;
			}
		}
	}
}

static qboolean PM_SaberBlocking(void)
{
	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);

	// Now we react to a block action by the player's lightsaber.
	if (pm->ps->saberBlocked)
	{
		qboolean wasAttackedByGun = qfalse;

		if (pm->ps->saber_move > LS_PUTAWAY && pm->ps->saber_move <= LS_A_BL2TR && pm->ps->saberBlocked !=
			BLOCKED_PARRY_BROKEN &&
			(pm->ps->saberBlocked < BLOCKED_FRONT || pm->ps->saberBlocked > BLOCKED_FRONT_PROJ))
		{
			//we parried another lightsaber while attacking, so treat it as a bounce
			pm->ps->saberBlocked = BLOCKED_ATK_BOUNCE;
		}
		else if ((pm->ps->fd.saberAnimLevel == SS_FAST ||
			pm->ps->fd.saberAnimLevel == SS_MEDIUM ||
			pm->ps->fd.saberAnimLevel == SS_STRONG ||
			pm->ps->fd.saberAnimLevel == SS_DESANN ||
			pm->ps->fd.saberAnimLevel == SS_TAVION) &&
			(pm->cmd.buttons & BUTTON_WALKING && pm->cmd.buttons & BUTTON_USE)
			&& pm->ps->saberBlocked >= BLOCKED_UPPER_RIGHT
			&& pm->ps->saberBlocked <= BLOCKED_FRONT
			&& !PM_SaberInKnockaway(pm->ps->saberBounceMove))
		{
			//if hitting use during a parry (not already a knockaway)
			pm->ps->saberBounceMove = pm_block_the_attack(pm->ps->saberBlocked);
		}
		else
		{
			if (pm->ps->saberBlocked >= BLOCKED_FRONT && pm->ps->saberBlocked <= BLOCKED_FRONT_PROJ)
			{
				//blocking a projectile
				if (pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK)
				{
					//trying to attack
					if (pm->ps->saber_move == LS_READY || PM_SaberInReflect(pm->ps->saber_move))
					{
						pm->ps->saberBlocked = BLOCKED_NONE;
						pm->ps->saberBounceMove = LS_NONE;
						pm->ps->weaponstate = WEAPON_READY;
						if (PM_SaberInReflect(pm->ps->saber_move) && pm->ps->weaponTime > 0)
						{
							pm->ps->weaponTime = 0;
						}
						return qfalse;
					}
				}
				else if (pm->cmd.buttons & BUTTON_ATTACK && !(pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCK))
				{
					//block is done or breaking out of it with an attack
					pm->ps->weaponTime = 0;
					pm->ps->saberBlocked = BLOCKED_NONE;
				}
			}
		}
		switch (pm->ps->saberBlocked)
		{
		case BLOCKED_BOUNCE_MOVE:
		{
			//act as a bounceMove and reset the saber_move instead of using a separate value for it
			pm->ps->torsoTimer = 0;
			PM_SetSaberMove(pm->ps->saber_move);
			pm->ps->weaponTime = pm->ps->torsoTimer;
			pm->ps->saberBlocked = 0;
		}
		break;
		case BLOCKED_PARRY_BROKEN:
			//whatever parry we were is in now broken, play the appropriate knocked-away anim
		{
			saber_moveName_t next_move;

			if (PM_SaberInBrokenParry(pm->ps->saber_move))
			{
				//already have one...?
				next_move = (saber_moveName_t)pm->ps->saberBounceMove;
			}
			else
			{
				next_move = PM_BrokenParryForParry(pm->ps->saber_move);
			}
			if (next_move != LS_NONE)
			{
				PM_SetSaberMove(next_move);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{
				//Maybe in a knockaway?
			}
		}
		break;
		case BLOCKED_ATK_BOUNCE:
			if (pm->ps->saber_move >= LS_T1_BR__R
				|| PM_SaberInBounce(pm->ps->saber_move)
				|| PM_SaberInReturn(pm->ps->saber_move)
				|| PM_SaberInMassiveBounce(pm->ps->torsoAnim))
			{
				//an actual bounce?  Other bounces before this are actually transitions?
				pm->ps->saberBlocked = BLOCKED_NONE;
			}
			else if (pm->ps->userInt3 & 1 << FLAG_SLOWBOUNCE
				&& !(pm->ps->userInt3 & 1 << FLAG_OLDSLOWBOUNCE)
				|| !PM_SaberInAttack(pm->ps->saber_move) && !PM_SaberInStart(pm->ps->saber_move))
			{
				//already in the bounce, go into an attack or transition to ready.. should never get here since can't be blocked in a bounce!
				int next_move;

				if (pm->cmd.buttons & BUTTON_ATTACK)
				{
					//transition to a new attack
#ifdef _GAME
					if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
					{
						// Some special bot stuff.
						next_move = saber_moveData[pm->ps->saber_move].chain_attack;
					}
					else
#endif
					{
						//player
						int newQuad = PM_saber_moveQuadrantForMovement(&pm->cmd);

						while (newQuad == saber_moveData[pm->ps->saber_move].startQuad)
						{
							//player is still in same attack quad, don't repeat that attack because it looks bad,
							newQuad = Q_irand(Q_BR, Q_BL);
						}
						next_move = transitionMove[saber_moveData[pm->ps->saber_move].startQuad][newQuad];
					}
				}
				else
				{
					//return to ready
#ifdef _GAME
					if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
					{
						// Some special bot stuff.
						next_move = saber_moveData[pm->ps->saber_move].chain_idle;
					}
					else
#endif
					{
						//player
						if (saber_moveData[pm->ps->saber_move].startQuad == Q_T)
						{
							next_move = LS_R_BL2TR;
						}
						else if (saber_moveData[pm->ps->saber_move].startQuad < Q_T)
						{
							next_move = LS_R_TL2BR + (saber_moveName_t)(saber_moveData[pm->ps->saber_move].startQuad -
								Q_BR);
						}
						else
						{
							next_move = LS_R_BR2TL + (saber_moveName_t)(saber_moveData[pm->ps->saber_move].startQuad -
								Q_TL);
						}
					}
				}
				PM_SetSaberMove(next_move);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{
				//start the bounce move
				saber_moveName_t bounceMove;
				if (pm->ps->saberBounceMove != LS_NONE)
				{
					bounceMove = (saber_moveName_t)pm->ps->saberBounceMove;
				}
				else
				{
					bounceMove = PM_SaberBounceForAttack(pm->ps->saber_move);
				}
				PM_SetSaberMove(bounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			break;
		case BLOCKED_UPPER_RIGHT:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove(pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1 && (saber1->type == SABER_BACKHAND
				|| saber1->type == SABER_ASBACKHAND))
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{
				PM_SetSaberMove(LS_PARRY_UR);
			}
			break;
		case BLOCKED_UPPER_RIGHT_PROJ:
			if (saber1 && saber1->type == SABER_DOOKU
				|| saber1 && saber1->type == SABER_ANAKIN
				|| saber1 && saber1->type == SABER_REY
				|| saber1 && saber1->type == SABER_PALP && pm->ps->userInt3 & 1 << FLAG_BLOCKDRAINED)
			{
				PM_SetSaberMove(LS_K1_TR);
			}
			else
			{
				PM_SetSaberMove(LS_REFLECT_UR);
			}
			wasAttackedByGun = qtrue;
			break;
		case BLOCKED_UPPER_LEFT:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove(pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1 && (saber1->type == SABER_BACKHAND
				|| saber1->type == SABER_ASBACKHAND))
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{
				PM_SetSaberMove(LS_PARRY_UL);
			}
			break;
		case BLOCKED_UPPER_LEFT_PROJ:
			if (saber1 && saber1->type == SABER_DOOKU
				|| saber1 && saber1->type == SABER_ANAKIN
				|| saber1 && saber1->type == SABER_REY
				|| saber1 && saber1->type == SABER_PALP && pm->ps->userInt3 & 1 << FLAG_BLOCKDRAINED)
			{
				PM_SetSaberMove(LS_K1_TL);
			}
			else
			{
				PM_SetSaberMove(LS_REFLECT_UL);
			}
			wasAttackedByGun = qtrue;
			break;
		case BLOCKED_LOWER_RIGHT:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove(pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{
				PM_SetSaberMove(LS_PARRY_LR);
			}
			break;
		case BLOCKED_LOWER_RIGHT_PROJ:
			PM_SetSaberMove(LS_REFLECT_LR);
			wasAttackedByGun = qtrue;
			break;
		case BLOCKED_LOWER_LEFT:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove(pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{
				PM_SetSaberMove(LS_PARRY_LL);
			}
			break;
		case BLOCKED_LOWER_LEFT_PROJ:
			PM_SetSaberMove(LS_REFLECT_LL);
			wasAttackedByGun = qtrue;
			break;
		case BLOCKED_TOP:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove(pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1 && (saber1->type == SABER_BACKHAND
				|| saber1->type == SABER_ASBACKHAND))
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{
				PM_SetSaberMove(LS_PARRY_UP);
			}
			break;
		case BLOCKED_TOP_PROJ:
			PM_SetSaberMove(LS_REFLECT_UP);
			wasAttackedByGun = qtrue;
			break;
		case BLOCKED_FRONT:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove(pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1 && (saber1->type == SABER_BACKHAND
				|| saber1->type == SABER_ASBACKHAND))
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1 && saber1->type == SABER_DOOKU
				|| saber1 && saber1->type == SABER_ANAKIN
				|| saber1 && saber1->type == SABER_REY
				|| saber1 && saber1->type == SABER_PALP && pm->ps->userInt3 & 1 << FLAG_BLOCKDRAINED)
			{
				PM_SetSaberMove(LS_REFLECT_ATTACK_FRONT);
			}
			else
			{
				if (pm->ps->fd.saberAnimLevel == SS_DUAL)
				{
					PM_SetSaberMove(LS_PARRY_UP);
				}
				else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
				{
					PM_SetSaberMove(LS_PARRY_UP);
				}
				else
				{
					PM_SetSaberMove(LS_PARRY_FRONT);
				}
			}
			break;
		case BLOCKED_FRONT_PROJ:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove(pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1 && (saber1->type == SABER_BACKHAND
				|| saber1->type == SABER_ASBACKHAND))
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1 && saber1->type == SABER_DOOKU
				|| saber1 && saber1->type == SABER_ANAKIN
				|| saber1 && saber1->type == SABER_REY
				|| saber1 && saber1->type == SABER_PALP)
			{
				PM_SetSaberMove(LS_REFLECT_ATTACK_FRONT);
			}
			else
			{
				PM_SetSaberMove(LS_REFLECT_FRONT);
			}
			wasAttackedByGun = qtrue;
			break;
		case BLOCKED_BLOCKATTACK_LEFT:
			PM_SetSaberMove(LS_REFLECT_ATTACK_LEFT);
			break;
		case BLOCKED_BLOCKATTACK_RIGHT:
			PM_SetSaberMove(LS_REFLECT_ATTACK_RIGHT);
			break;
		case BLOCKED_BLOCKATTACK_FRONT:
			PM_SetSaberMove(LS_REFLECT_ATTACK_FRONT);
			break;
		case BLOCKED_BACK:
			if (pm->ps->fd.saberAnimLevel == SS_DUAL)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_P6_S1_B1_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_P7_S1_B1_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_P1_S1_B1_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			break;
		default:
			pm->ps->saberBlocked = BLOCKED_NONE;
			break;
		}
		if (InSaberDelayAnimation(pm->ps->torsoAnim) && pm->cmd.buttons & BUTTON_ATTACK && wasAttackedByGun)
		{
			pm->ps->weaponTime += 700;
			pm->ps->torsoTimer += 1500;
			if (pm->ps->saberBlocked >= BLOCKED_UPPER_RIGHT && pm->ps->saberBlocked < BLOCKED_UPPER_RIGHT_PROJ)
			{
				//hold the parry for a bit
				if (pm->ps->torsoTimer < pm->ps->weaponTime)
				{
					pm->ps->torsoTimer = pm->ps->weaponTime;
				}
			}
		}

		//clear block
		pm->ps->saberBlocked = 0;

		// Charging is like a lead-up before attacking again.  This is an appropriate use, or we can create a new weaponstate for blocking
		pm->ps->saberBounceMove = LS_NONE;
		pm->ps->weaponstate = WEAPON_READY;

		// Done with block, so stop these active weapon branches.
		return qtrue;
	}
	return qfalse;
}

/*
=================
PM_WeaponLightsaber

Consults a chart to choose what to do with the lightsaber.
=================
*/
void PM_WeaponLightsaber(void)
{
	const qboolean delayed_fire = qfalse;
	int anim = -1;
	int newmove = LS_NONE;

	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);

	const qboolean is_holding_block_button = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Holding Block Button
	const qboolean is_holding_block_button_and_attack = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
	//Active Blocking
	const qboolean walking_blocking = pm->ps->ManualBlockingFlags & 1 << MBF_BLOCKWALKING ? qtrue : qfalse;
	//Walking Blocking

	const qboolean isPlayer = (pm->ps->clientNum < MAX_CLIENTS) ? qtrue : qfalse;

	qboolean check_only_weap = qfalse;

	if (pm_entSelf->s.NPC_class == CLASS_SABER_DROID)
	{
		//Saber droid does it's own attack logic
		PM_SaberDroidWeapon();
		return;
	}

	if (pm_entSelf->s.NPC_class == CLASS_SBD)
	{
		//Saber droid does it's own attack logic
		PM_SaberDroidWeapon();
		return;
	}

	// don't allow attack until all buttons are up
	if (pm->ps->pm_flags & PMF_RESPAWNED)
	{
		return;
	}

	if (!in_camera)
	{
		//preblocks can be interrupted
		if (PM_SaberInParry(pm->ps->saber_move) && pm->ps->userInt3 & 1 << FLAG_PREBLOCK // in a pre-block
			&& (pm->cmd.buttons & BUTTON_ALT_ATTACK || pm->cmd.buttons & BUTTON_ATTACK)) //and attempting an attack
		{
			//interrupting a preblock
			pm->ps->weaponTime = 0;
			pm->ps->torsoTimer = 0;
		}

#ifdef _GAME
		if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT)
		{
			if (!(pm->cmd.buttons & BUTTON_ATTACK)
				&& !PM_SaberInSpecial(pm->ps->saber_move)
				&& !PM_SaberInStart(pm->ps->saber_move)
				&& !PM_SaberInTransition(pm->ps->saber_move)
				&& !PM_SaberInAttack(pm->ps->saber_move)
				&& pm->ps->saber_move > LS_PUTAWAY)
			{
				// Always return to ready when attack is released...
				PM_SetSaberMove(LS_READY);
				return;
			}
		}
#endif
	}

	if (PM_InKnockDown(pm->ps) || BG_InRoll(pm->ps, pm->ps->legsAnim))
	{
		//in knockdown
		// make weapon function
		if (pm->ps->weaponTime > 0)
		{
			pm->ps->weaponTime -= pml.msec;
			if (pm->ps->weaponTime <= 0)
			{
				pm->ps->weaponTime = 0;
			}
		}
		if (pm->ps->legsAnim == BOTH_ROLL_F ||
			pm->ps->legsAnim == BOTH_ROLL_F1 ||
			pm->ps->legsAnim == BOTH_ROLL_F2 &&
			pm->ps->legsTimer <= 250)
		{
			if (pm->cmd.buttons & BUTTON_ATTACK)
			{
				if (BG_EnoughForcePowerForMove(SABER_KATA_ATTACK_POWER, isPlayer) && !pm->ps->saberInFlight && pm->watertype != CONTENTS_WATER)
				{
					if (PM_CanDoRollStab())
					{
						//make sure the saber is on for this move!
						if (pm->ps->saberHolstered == 2)
						{
							//all the way off
							pm->ps->saberHolstered = 0;
							PM_AddEvent(EV_SABER_UNHOLSTER);
						}
						PM_SetSaberMove(LS_ROLL_STAB);
						WP_ForcePowerDrain(pm->ps, FP_SABER_OFFENSE, SABER_KATA_ATTACK_POWER);
					}
				}
			}
		}
		return;
	}

	if (pm->ps->torsoAnim == BOTH_FORCELONGLEAP_LAND
		|| pm->ps->torsoAnim == BOTH_FORCELONGLEAP_LAND2
		|| pm->ps->torsoAnim == BOTH_FORCELONGLEAP_START && !(pm->cmd.buttons & BUTTON_ATTACK))
	{
		//if you're in the long-jump and you're not attacking (or are landing), you're not doing anything
		if (pm->ps->torsoTimer)
		{
			return;
		}
	}

	if (pm->ps->legsAnim == BOTH_FLIP_HOLD7
		&& !(pm->cmd.buttons & BUTTON_ATTACK))
	{
		//if you're in the upside-down attack hold, don't do anything unless you're attacking
		return;
	}

	if (pm->ps->saberLockTime > pm->cmd.serverTime)
	{
		pm->ps->saber_move = LS_NONE;
		PM_SaberLocked();
		return;
	}
	if (pm->ps->saberLockFrame)
	{
		if (pm->ps->saberLockEnemy < ENTITYNUM_NONE &&
			pm->ps->saberLockEnemy >= 0)
		{
			const bgEntity_t* bg_ent = PM_BGEntForNum(pm->ps->saberLockEnemy);

			if (bg_ent)
			{
				playerState_t* en = bg_ent->playerState;

				if (en)
				{
					PM_SaberLockBreak(en, qfalse, 0);
#ifdef _GAME
					gentity_t* self = &g_entities[pm->ps->clientNum];
					G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/saber/saberlockend.mp3"));
#endif
					return;
				}
			}
		}

		if (pm->ps->saberLockFrame)
		{
			pm->ps->torsoTimer = 0;
			PM_SetAnim(SETANIM_TORSO, BOTH_STAND1, SETANIM_FLAG_OVERRIDE);
			pm->ps->saberLockFrame = 0;
		}
	}

	if (PM_KickingAnim(pm->ps->legsAnim) ||
		PM_KickingAnim(pm->ps->torsoAnim))
	{
		if (pm->ps->legsTimer > 0)
		{
			//you're kicking, no interruptions
			return;
		}
		//done?  be immediately ready to do an attack
		pm->ps->saber_move = LS_READY;
		pm->ps->weaponTime = 0;
	}

	if (BG_SabersOff(pm->ps))
	{
		if (pm->ps->saber_move != LS_READY)
		{
			PM_SetSaberMove(LS_READY);
		}

		if (pm->ps->legsAnim != pm->ps->torsoAnim && !PM_InSlopeAnim(pm->ps->legsAnim) &&
			pm->ps->torsoTimer <= 0)
		{
			PM_SetAnim(SETANIM_TORSO, pm->ps->legsAnim, SETANIM_FLAG_OVERRIDE);
		}
		else if (PM_InSlopeAnim(pm->ps->legsAnim) && pm->ps->torsoTimer <= 0)
		{
#ifdef _GAME
			if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
			{
				// Some special bot stuff.
				PM_SetAnim(SETANIM_TORSO, PM_ReadyPoseForsaber_anim_levelBOT(), SETANIM_FLAG_OVERRIDE);
			}
			else
#endif
			{
				if (is_holding_block_button && pm->cmd.buttons & BUTTON_WALKING)
				{
					if (pm->ps->fd.saberAnimLevel == SS_DUAL)
					{
						PM_SetAnim(SETANIM_TORSO, PM_BlockingPoseForsaber_anim_levelDual(), SETANIM_FLAG_OVERRIDE);
					}
					else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
					{
						PM_SetAnim(SETANIM_TORSO, PM_BlockingPoseForsaber_anim_levelStaff(), SETANIM_FLAG_OVERRIDE);
					}
					else
					{
						PM_SetAnim(SETANIM_TORSO, PM_BlockingPoseForsaber_anim_levelSingle(), SETANIM_FLAG_OVERRIDE);
					}
				}
				else
				{
					PM_SetAnim(SETANIM_TORSO, PM_IdlePoseForsaber_anim_level(), SETANIM_FLAG_OVERRIDE);
				}
			}
		}

		if (pm->ps->weaponTime < 1 && pm->cmd.buttons & BUTTON_ATTACK && !pm->ps->saberInFlight && pm->watertype !=
			CONTENTS_WATER)
		{
			if (pm->ps->duelTime < pm->cmd.serverTime)
			{
				if (!pm->ps->m_iVehicleNum)
				{
					//don't let em unholster the saber by attacking while on vehicle
					pm->ps->saberHolstered = 0;
					PM_AddEvent(EV_SABER_UNHOLSTER);
				}
				else
				{
					pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
					pm->cmd.buttons &= ~BUTTON_ATTACK;
				}
			}
		}

		if (pm->ps->weaponTime > 0)
		{
			pm->ps->weaponTime -= pml.msec;
		}

		check_only_weap = qtrue;
		goto weapChecks;
	}

	if (!pm->ps->saberEntityNum && pm->ps->saberInFlight)
	{
		//this means our saber has been knocked away
		if (pm->ps->fd.saberAnimLevel == SS_DUAL)
		{
			if (pm->ps->saberHolstered > 1 || !pm->ps->saberHolstered)
			{
				pm->ps->saberHolstered = 1;
			}
		}
		else
		{
			pm->cmd.buttons &= ~BUTTON_ATTACK;
		}
		pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
	}

	if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
	{
		//might as well just check for a saber throw right here
		if (pm->ps->saberInFlight && pm->cmd.buttons & BUTTON_KICK && pm->ps->communicatingflags & 1 << KICKING)
		{
			//kick after doing a saberthrow,whalst saber is still being controlled
			if (!(pm->cmd.buttons & BUTTON_ATTACK)) //not trying to swing the saber
			{
				if ((pm->cmd.forwardmove || pm->cmd.rightmove) //trying to kick in a specific direction
					&& PM_CheckAltKickAttack()) //trying to do a kick
				{
					//allow them to do the kick now!
					int kick_move = PM_MeleeMoveForConditions();
					if (kick_move == LS_HILT_BASH)
					{
						//yeah.. no hilt to bash with!
						kick_move = LS_KICK_F2;
					}
					if (kick_move != -1)
					{
						pm->ps->weaponTime = 0;
						PM_SetSaberMove(kick_move);
						return;
					}
				}
			}
		}
		else if (!PM_SaberThrowable() && pm->cmd.buttons & BUTTON_ALT_ATTACK && pm->ps->communicatingflags & 1 << KICKING) //not trying to swing the saber
		{
			//kick instead of doing a throw
			if (PM_DoKick())
			{
				return;
			}
		}
		else if (!PM_SaberThrowable() && pm->cmd.buttons & BUTTON_KICK && pm->ps->communicatingflags & 1 << KICKING)
			//not trying to swing the saber
		{
			//kick instead of doing a throw
			if (PM_DoSlap())
			{
				return;
			}
		}
		else if (pm->ps->saberInFlight && pm->ps->saberEntityNum)
		{
			//saber is already in flight continue moving it with the force.
			switch (pm->ps->fd.saberAnimLevel)
			{
			case SS_FAST:
			case SS_TAVION:
			case SS_MEDIUM:
			case SS_STRONG:
			case SS_DESANN:
			case SS_DUAL:
				PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				break;
			case SS_STAFF:
				PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL_ALT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				break;
			default:;
			}
			pm->ps->torsoTimer = 1;
			return;
		}
		else if (pm->ps->weaponTime < 1
			&& pm->ps->saberCanThrow
			&& !BG_HasYsalamiri(pm->gametype, pm->ps) &&
			BG_CanUseFPNow(pm->gametype, pm->ps, pm->cmd.serverTime, FP_SABERTHROW) &&
			pm->ps->fd.forcePowerLevel[FP_SABERTHROW] > 0 &&
			PM_SaberPowerCheck())
		{
			trace_t sab_tr;
			vec3_t fwd, min_fwd, sab_mins, sab_maxs;

			VectorSet(sab_mins, SABERMINS_X, SABERMINS_Y, SABERMINS_Z);
			VectorSet(sab_maxs, SABERMAXS_X, SABERMAXS_Y, SABERMAXS_Z);

			AngleVectors(pm->ps->viewangles, fwd, NULL, NULL);
			VectorMA(pm->ps->origin, SABER_MIN_THROW_DIST, fwd, min_fwd);
#ifdef _GAME
			if (m_nerf.integer & 1 << EOC_CLOSETHROW)
#else
			if (cgs.m_nerf & 1 << EOC_CLOSETHROW)
#endif
			{
				pm->trace(&sab_tr, pm->ps->origin, sab_mins, sab_maxs, min_fwd, pm->ps->clientNum, MASK_PLAYERSOLID);

				if (sab_tr.allsolid || sab_tr.startsolid || sab_tr.fraction < 1.0f)
				{
					//not enough room to throw
				}
				else
				{
					//throw it
					//This will get set to false again once the saber makes it back to its owner game-side
					if (!pm->ps->saberInFlight)
					{
						pm->ps->fd.forcePower -= forcePowerNeeded[pm->ps->fd.forcePowerLevel[FP_SABERTHROW]][FP_SABERTHROW];
					}

					pm->ps->saberInFlight = qtrue;
				}
			}
			else
			{
				//always throw no matter how close
				if (!pm->ps->saberInFlight)
				{
					pm->ps->fd.forcePower -= forcePowerNeeded[pm->ps->fd.forcePowerLevel[FP_SABERTHROW]][FP_SABERTHROW];
				}

				pm->ps->saberInFlight = qtrue;

				switch (pm->ps->fd.saberAnimLevel)
				{
				case SS_FAST:
				case SS_TAVION:
				case SS_MEDIUM:
				case SS_STRONG:
				case SS_DESANN:
				case SS_DUAL:
					PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_STAFF:
					PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL_ALT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				default:;
				}
				pm->ps->torsoTimer = 1;
				return;
			}
		}
	}

	// check for dead player
	if (pm->ps->stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	// make weapon function
	if (pm->ps->weaponTime > 0)
	{
		//check for special pull move while busy
		const saber_moveName_t pullmove = PM_CheckPullAttack();
		if (pullmove != LS_NONE)
		{
			pm->ps->weaponTime = 0;
			pm->ps->torsoTimer = 0;
			pm->ps->legsTimer = 0;
			pm->ps->forceHandExtend = HANDEXTEND_NONE;
			pm->ps->weaponstate = WEAPON_READY;
			PM_SetSaberMove(pullmove);
			return;
		}

		pm->ps->weaponTime -= pml.msec;
	}
	else
	{
		pm->ps->weaponstate = WEAPON_READY;
	}

	if (pm->ps->saberEventFlags & SEF_INWATER || pm->waterlevel > 1 || pm->watertype == CONTENTS_WATER) //saber in water
	{
		pm->cmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCEPOWER | BUTTON_DASH);
	}

	PM_CheckClearSaberBlock();

	if (PM_LockedAnim(pm->ps->torsoAnim)
		&& pm->ps->torsoTimer)
	{
		//can't interrupt these anims ever
		return;
	}

	if (PM_SaberBlocking())
	{
		//busy blocking, don't do attacks
		return;
	}

weapChecks:

	// check for weapon change
	// can't change if weapon is firing, but can change again if lowering or raising
	if ((pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING) && pm->ps->weaponstate != WEAPON_CHARGING_ALT
		&& pm->ps->weaponstate != WEAPON_CHARGING && !(pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCK))
	{
		if (pm->ps->weapon != pm->cmd.weapon)
		{
			PM_BeginWeaponChange(pm->cmd.weapon);
		}
	}

	if (PM_CanDoKata())
	{
		const saber_moveName_t override_move = LS_INVALID;

		if (override_move == LS_INVALID)
		{
			switch (pm->ps->fd.saberAnimLevel)
			{
			case SS_FAST:
				if (saber1 && saber1->type == SABER_YODA)
				{
					PM_SetSaberMove(LS_YODA_SPECIAL);
				}
				else
				{
					PM_SetSaberMove(LS_A1_SPECIAL);
				}
				break;
			case SS_MEDIUM:
				PM_SetSaberMove(LS_A2_SPECIAL);
				break;
			case SS_STRONG:
				if (saber1 && saber1->type == SABER_PALP)
				{
					PM_SetSaberMove(LS_A_JUMP_PALP_);
				}
				else
				{
					PM_SetSaberMove(LS_A3_SPECIAL);
				}
				break;
			case SS_DESANN:
				if (saber1 && saber1->type == SABER_PALP)
				{
					PM_SetSaberMove(LS_A_JUMP_PALP_);
				}
				else
				{
					PM_SetSaberMove(LS_A5_SPECIAL);
				}
				break;
			case SS_TAVION:
				if (saber1 && saber1->type == SABER_YODA)
				{
					PM_SetSaberMove(LS_YODA_SPECIAL);
				}
				else
				{
					PM_SetSaberMove(LS_A4_SPECIAL);
				}
				break;
			case SS_DUAL:
				if (saber1 && saber1->type == SABER_GRIE)
				{
					PM_SetSaberMove(LS_DUAL_SPIN_PROTECT_GRIE);
				}
				else if (saber1 && saber1->type == SABER_GRIE4)
				{
					PM_SetSaberMove(LS_DUAL_SPIN_PROTECT_GRIE);
				}
				else if (saber1 && saber1->type == SABER_DAGGER)
				{
					PM_SetSaberMove(LS_STAFF_SOULCAL);
				}
				else if (saber1 && saber1->type == SABER_BACKHAND
					|| saber1->type == SABER_ASBACKHAND)
				{
					PM_SetSaberMove(LS_STAFF_SOULCAL);
				}
				else
				{
					PM_SetSaberMove(LS_DUAL_SPIN_PROTECT);
				}
				break;
			case SS_STAFF:
				PM_SetSaberMove(LS_STAFF_SOULCAL);
				break;
			default:;
			}
			pm->ps->weaponstate = WEAPON_FIRING;
			WP_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER);
		}
		if (override_move != LS_NONE)
		{
			//not cancelled
			return;
		}
	}

	if (pm->ps->weaponTime > 0)
	{
		return;
	}

	// *********************************************************
	// WEAPON_DROPPING
	// *********************************************************

	// change weapon if time
	if (pm->ps->weaponstate == WEAPON_DROPPING)
	{
		PM_FinishWeaponChange();
		return;
	}

	// *********************************************************
	// WEAPON_RAISING
	// *********************************************************

	if (pm->ps->weaponstate == WEAPON_RAISING)
	{
		//Just selected the weapon
		pm->ps->weaponstate = WEAPON_IDLE;

		if (pm->ps->legsAnim == BOTH_WALK1)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK1, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_MENUIDLE1)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_MENUIDLE1, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN1)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN1, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN2)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN2, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_SPRINT)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_SPRINT, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_SPRINT_MP)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_SPRINT_MP, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_SPRINT_SABER_MP)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_SPRINT_SABER_MP, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN3)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN3, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN3_MP)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN3_MP, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN4)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN4, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN5)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN5, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN6)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN6, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN7)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN7, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN8)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN8, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN9)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN9, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN10)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN10, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN_STAFF)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN_STAFF, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN_DUAL)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN_DUAL, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_VADERRUN1)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_VADERRUN1, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_VADERRUN2)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_VADERRUN2, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK2)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK2, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK5)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK5, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK6)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK6, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK7)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK7, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK8)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK8, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK9)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK9, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK10)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK10, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK_STAFF)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK_STAFF, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK_DUAL)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK_DUAL, SETANIM_FLAG_NORMAL);
		}
		else
		{
#ifdef _GAME
			if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
			{
				// Some special bot stuff.
				PM_SetAnim(SETANIM_TORSO, PM_ReadyPoseForsaber_anim_levelBOT(), SETANIM_FLAG_NORMAL);
			}
			else
#endif
			{
				if (is_holding_block_button && pm->cmd.buttons & BUTTON_WALKING)
				{
					if (pm->ps->fd.saberAnimLevel == SS_DUAL)
					{
						PM_SetAnim(SETANIM_TORSO, PM_BlockingPoseForsaber_anim_levelDual(), SETANIM_FLAG_NORMAL);
					}
					else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
					{
						PM_SetAnim(SETANIM_TORSO, PM_BlockingPoseForsaber_anim_levelStaff(), SETANIM_FLAG_NORMAL);
					}
					else
					{
						PM_SetAnim(SETANIM_TORSO, PM_BlockingPoseForsaber_anim_levelSingle(), SETANIM_FLAG_NORMAL);
					}
				}
				else
				{
					PM_SetAnim(SETANIM_TORSO, PM_IdlePoseForsaber_anim_level(), SETANIM_FLAG_NORMAL);
				}
			}
		}

		if (pm->ps->weaponstate == WEAPON_RAISING)
		{
			return;
		}
	}

	if (check_only_weap)
	{
		return;
	}

	// *********************************************************
	// Check for WEAPON ATTACK
	// *********************************************************
	if (pm->ps->saberInFlight && pm->ps->forceHandExtend != HANDEXTEND_SABERPULL
		&& pm->cmd.buttons & BUTTON_KICK && pm->ps->communicatingflags & 1 << KICKING)
	{
		//don't have our saber so we can punch instead.
		PM_DoSlap();
		return;
	}

	if (pm->cmd.buttons & BUTTON_KICK && pm->ps->communicatingflags & 1 << KICKING)
	{
		//ok, try a kick I guess.
		if (!PM_KickingAnim(pm->ps->torsoAnim) &&
			!PM_KickingAnim(pm->ps->legsAnim) &&
			!BG_InRoll(pm->ps, pm->ps->legsAnim))
		{
			int kick_move = PM_MeleeMoveForConditions();

			if (kick_move != -1)
			{
				if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
				{
					//if in air, convert kick to an in-air kick
					const float gDist = PM_GroundDistance();

					if ((!PM_FlippingAnim(pm->ps->legsAnim) || pm->ps->legsTimer <= 0) &&
						gDist > 64.0f &&
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
						//leave it as a normal kick unless we're too high up
						if (gDist > 128.0f || pm->ps->velocity[2] >= 0)
						{
							//off ground, but too close to ground
							kick_move = -1;
						}
					}
				}
			}
			if (kick_move != -1)
			{
				const int kick_anim = saber_moveData[kick_move].animToUse;

				if (kick_anim != -1)
				{
					PM_SetAnim(SETANIM_BOTH, kick_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					if (pm->ps->legsAnim == kick_anim)
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

	if (PM_CheckUpsideDownAttack())
	{
		return;
	}

	if (!delayed_fire)
	{
		int curmove;
		// Start with the current move, and cross index it with the current control states.
		if (pm->ps->saber_move > LS_NONE && pm->ps->saber_move < LS_MOVE_MAX)
		{
			curmove = (saber_moveName_t)pm->ps->saber_move;
		}
		else
		{
			curmove = LS_READY;
		}

		if (curmove == LS_A_JUMP_T__B_ || curmove == LS_A_JUMP_PALP_ || pm->ps->torsoAnim ==
			BOTH_FORCELEAP2_T__B_ || pm->ps->torsoAnim == BOTH_FORCELEAP_PALP)
		{
			//must transition back to ready from this anim
			newmove = LS_R_T2B;
		}
		// check for fire
		//This section dictates want happens when you quit holding down attack.
		else if (!(pm->cmd.buttons & BUTTON_ATTACK) || is_holding_block_button || walking_blocking || is_holding_block_button_and_attack)
		{
			//not attacking
			pm->ps->weaponTime = 0;

			if (pm->ps->weaponTime > 0)
			{
				//Still firing
				pm->ps->weaponstate = WEAPON_FIRING;
			}
			else if (pm->ps->weaponstate != WEAPON_READY)
			{
				pm->ps->weaponstate = WEAPON_IDLE;
			}

			//Check for finishing an anim if necc.
			if (curmove >= LS_S_TL2BR && curmove <= LS_S_T2B)
			{
				//started a swing, must continue from here
#ifdef _GAME
				if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT) //npc
				{
					//NPCs never do attack fakes, just follow thru with attack.
					newmove = LS_A_TL2BR + (curmove - LS_S_TL2BR);
				}
				else
#endif
				{
					//perform attack fake
					newmove = PM_ReturnforQuad(saber_moveData[curmove].endQuad);
					PM_AddBlockFatigue(pm->ps, FATIGUE_ATTACKFAKE);
				}
			}
			else if (curmove >= LS_A_TL2BR && curmove <= LS_A_T2B)
			{
				//finished an attack, must continue from here
				newmove = LS_R_TL2BR + (curmove - LS_A_TL2BR);
			}
			else if (PM_SaberInTransition(curmove))
			{
				//in a transition, must play sequential attack
#ifdef _GAME
				if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT) //npc
				{
					//NPCs never stop attacking mid-attack, just follow thru with attack.
					newmove = saber_moveData[curmove].chain_attack;
				}
				else
#endif
				{
					//exit out of transition without attacking
					newmove = PM_ReturnforQuad(saber_moveData[curmove].endQuad);
				}
			}
			else if (PM_SaberInBounce(curmove))
			{
				//in a bounce
#ifdef _GAME
				if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT) //npc
				{
					//NPCs must play sequential attack
					if (PM_SaberKataDone(LS_NONE, LS_NONE))
					{
						//done with this kata, must return to ready before attack again
						newmove = saber_moveData[curmove].chain_idle;
					}
					else
					{
						//okay to chain to another attack
						newmove = saber_moveData[curmove].chain_attack;
						//we assume they're attacking, even if they're not
						pm->ps->saberAttackChainCount++;

						if (pm->ps->saberFatigueChainCount < MISHAPLEVEL_MAX)
						{
							pm->ps->saberFatigueChainCount++;
						}
					}
				}
				else
#endif
				{
					//player gets his by directional control
					newmove = saber_moveData[curmove].chain_idle; //oops, not attacking, so don't chain
				}
			}
			else
			{
				PM_SetSaberMove(LS_READY);
				return;
			}
		}
		else if (pm->cmd.buttons & BUTTON_ATTACK && pm->cmd.buttons & BUTTON_USE)
		{
			//do some fancy faking stuff.
			if (pm->ps->weaponstate != WEAPON_READY)
			{
				pm->ps->weaponstate = WEAPON_IDLE;
			}
			//Check for finishing an anim if necc.
			if (curmove >= LS_S_TL2BR && curmove <= LS_S_T2B)
			{
				//allow the player to fake into another transition
				newmove = PM_DoFake(curmove);
				if (newmove == LS_NONE)
				{
					//no movement, just do the attack
					newmove = LS_A_TL2BR + (curmove - LS_S_TL2BR);
				}
			}
			else if (curmove >= LS_A_TL2BR && curmove <= LS_A_T2B)
			{
				//finished attack, let attack code handle the next step.
			}
			else if (PM_SaberInTransition(curmove))
			{
				//in a transition, must play sequential attack
				newmove = PM_DoFake(curmove);
				if (newmove == LS_NONE)
				{
					//no movement, just let the normal attack code handle it
					newmove = saber_moveData[curmove].chain_attack;
				}
			}
			else if (PM_SaberInBounce(curmove))
			{
				//in a bounce
			}
			else
			{
				//returning from a parry I think.
			}
		}

		// ***************************************************
		// Pressing attack, so we must look up the proper attack move.

		if (pm->ps->weaponTime > 0)
		{
			// Last attack is not yet complete.
			if (is_holding_block_button && pm->cmd.buttons & BUTTON_WALKING)
			{
				if (pm->ps->fd.saberAnimLevel == SS_DUAL)
				{
					PM_SetAnim(SETANIM_TORSO, PM_BlockingPoseForsaber_anim_levelDual(), SETANIM_FLAG_OVERRIDE);
					return;
				}
				if (pm->ps->fd.saberAnimLevel == SS_STAFF)
				{
					PM_SetAnim(SETANIM_TORSO, PM_BlockingPoseForsaber_anim_levelStaff(), SETANIM_FLAG_OVERRIDE);
					return;
				}
				PM_SetAnim(SETANIM_TORSO, PM_BlockingPoseForsaber_anim_levelSingle(), SETANIM_FLAG_OVERRIDE);
				return;
			}
			pm->ps->weaponstate = WEAPON_FIRING;
			return;
		}
		int both = qfalse;

		if (pm->ps->torsoAnim == BOTH_FORCELONGLEAP_ATTACK
			|| pm->ps->torsoAnim == BOTH_FORCELONGLEAP_ATTACK2
			|| pm->ps->torsoAnim == BOTH_FORCELONGLEAP_LAND
			|| pm->ps->torsoAnim == BOTH_FORCELONGLEAP_LAND2)
		{
			//can't attack in these anims
			return;
		}
		if (pm->cmd.buttons & BUTTON_ATTACK && pm->ps->torsoAnim == BOTH_FORCELONGLEAP_START)
		{
			//only 1 attack you can do from this anim
			if (pm->ps->saberHolstered == 2)
			{
				pm->ps->saberHolstered = 0;
				PM_AddEvent(EV_SABER_UNHOLSTER);
			}
			PM_SetSaberMove(LS_LEAP_ATTACK2);
			return;
		}
		if (pm->ps->torsoAnim == BOTH_FORCELONGLEAP_START)
		{
			//only 1 attack you can do from this anim
			if (pm->ps->torsoTimer >= 200 && pm->cmd.buttons & BUTTON_ATTACK)
			{
				//hit it early enough to do the attack
				PM_SetSaberMove(LS_LEAP_ATTACK);
			}
			return;
		}

		if (curmove >= LS_PARRY_UP && curmove <= LS_REFLECT_LL)
		{//from a parry or reflection, can go directly into an attack
			switch (saber_moveData[curmove].endQuad)
			{
			case Q_T:
				newmove = LS_A_T2B;
				break;
			case Q_TR:
				newmove = LS_A_TR2BL;
				break;
			case Q_TL:
				newmove = LS_A_TL2BR;
				break;
			case Q_BR:
				newmove = LS_A_BR2TL;
				break;
			case Q_BL:
				newmove = LS_A_BL2TR;
				break;
			default:;
				//shouldn't be a parry that ends at L, R or B
			}
		}

		if (newmove != LS_NONE)
		{
			//have a valid, final LS_ move picked, so skip finding the transition move and just get the anim
			anim = saber_moveData[newmove].animToUse;
		}

		if (anim == -1)
		{
			if (PM_SaberInTransition(curmove))
			{
				//in a transition, must play sequential attack
				newmove = saber_moveData[curmove].chain_attack;
			}
			else if (curmove >= LS_S_TL2BR && curmove <= LS_S_T2B)
			{
				//started a swing, must continue from here
				newmove = LS_A_TL2BR + (curmove - LS_S_TL2BR);
			}
			else if (PM_SaberInBrokenParry(curmove))
			{
				//broken parries must always return to ready
				newmove = LS_READY;
			}
			else if (PM_SaberInBounce(curmove) && pm->ps->userInt3 & 1 << FLAG_PARRIED)
			{
				//can't combo if we were parried.
				newmove = LS_READY;
			}
			else
			{//get attack move from movement command
#ifdef _GAME
				qboolean  bot = (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT);
				qboolean  npc = (pm_entSelf->s.eType == ET_NPC);

				if (bot || npc && Q_irand(0, 1)) // 50% chance
				{
					newmove = PM_NPCSaberAttackFromQuad(saber_moveData[curmove].endQuad);
				}
				else
#endif
				{
					newmove = PM_SaberAttackForMovement(curmove);

					if (pm->ps->saberFatigueChainCount < MISHAPLEVEL_TEN)
					{
						if ((PM_SaberInBounce(curmove) || PM_SaberInBrokenParry(curmove))
							&& saber_moveData[newmove].startQuad == saber_moveData[curmove].endQuad)
						{
							//this attack would be a repeat of the last (which was blocked), so don't actually use it, use the default chain attack for this bounce
							newmove = saber_moveData[curmove].chain_attack;
						}
					}
					else
					{
						if ((PM_SaberInBounce(curmove) || PM_SaberInParry(curmove))
							&& newmove >= LS_A_TL2BR && newmove <= LS_A_T2B)
						{
							//prevent similar attack directions to prevent lightning-like bounce attacks.
							if (saber_moveData[newmove].startQuad == saber_moveData[curmove].endQuad)
							{
								//can't attack in the same direction
								newmove = LS_READY;
							}
						}
					}
				}

				//starting a new attack, as such, remove the attack fake flag.
				pm->ps->userInt3 &= ~(1 << FLAG_ATTACKFAKE);

				if (PM_SaberKataDone(curmove, newmove))
				{
					//cannot chain this time
					newmove = saber_moveData[curmove].chain_idle;
				}
			}

			if (newmove != LS_NONE)
			{
				if (!PM_InCartwheel(pm->ps->legsAnim))
				{
					//don't do transitions when cartwheeling - could make you spin!
					newmove = PM_SaberAnimTransitionMove(curmove, newmove);
					anim = saber_moveData[newmove].animToUse;
				}
			}
		}

		if (anim == -1)
		{
			//not side-stepping, pick neutral anim
			if (PM_InCartwheel(pm->ps->legsAnim) && pm->ps->legsTimer > 100)
			{
				//if in the middle of a cartwheel, the chain attack is just a normal attack
				switch (pm->ps->legsAnim)
				{
				case BOTH_ARIAL_LEFT: //swing from l to r
				case BOTH_CARTWHEEL_LEFT:
					newmove = LS_A_L2R;
					break;
				case BOTH_ARIAL_RIGHT: //swing from r to l
				case BOTH_CARTWHEEL_RIGHT:
					newmove = LS_A_R2L;
					break;
				case BOTH_ARIAL_F1: //random l/r attack
					if (Q_irand(0, 1))
					{
						newmove = LS_A_L2R;
					}
					else
					{
						newmove = LS_A_R2L;
					}
					break;
				default:;
				}
			}
			else
			{
				newmove = saber_moveData[curmove].chain_attack;
			}
			if (newmove != LS_NONE)
			{
				anim = saber_moveData[newmove].animToUse;
			}

			if (!pm->cmd.forwardmove && !pm->cmd.rightmove && pm->cmd.upmove >= 0 && pm->ps->groundEntityNum !=
				ENTITYNUM_NONE)
			{
				//not moving at all, so set the anim on entire body
				both = qtrue;
			}
		}

		if (anim == -1)
		{
			switch (pm->ps->legsAnim)
			{
			case BOTH_WALK1:
			case BOTH_WALK1TALKCOMM1:
			case BOTH_WALK2:
			case BOTH_WALK2B:
			case BOTH_WALK3:
			case BOTH_WALK4:
			case BOTH_WALK5:
			case BOTH_WALK6:
			case BOTH_WALK7:
			case BOTH_WALK8: //# pistolwalk
			case BOTH_WALK9: //# rifle walk
			case BOTH_WALK10: //# grenade-walk
			case BOTH_WALK_STAFF:
			case BOTH_WALK_DUAL:
			case BOTH_WALKBACK1:
			case BOTH_WALKBACK2:
			case BOTH_WALKBACK_STAFF:
			case BOTH_WALKBACK_DUAL:
			case BOTH_VADERWALK1:
			case BOTH_VADERWALK2:
			case BOTH_SPRINT:
			case BOTH_SPRINT_MP:
			case BOTH_SPRINT_SABER_MP:
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
			case BOTH_RUN_STAFF:
			case BOTH_RUN_DUAL:
			case BOTH_RUNBACK1:
			case BOTH_RUNBACK2:
			case BOTH_RUNBACK_STAFF:
			case SBD_WALK_WEAPON:
			case SBD_WALK_NORMAL:
			case SBD_WALKBACK_NORMAL:
			case SBD_WALKBACK_WEAPON:
			case SBD_RUNBACK_NORMAL:
			case SBD_RUNING_WEAPON:
			case SBD_RUNBACK_WEAPON:
			case BOTH_RUNINJURED1:
			case BOTH_VADERRUN1:
			case BOTH_VADERRUN2:
				anim = pm->ps->legsAnim;
				break;
			default:;
#ifdef _GAME
				if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT)
				{
					anim = PM_ReadyPoseForsaber_anim_levelBOT();
				}
				else
#endif
				{
					if (is_holding_block_button && pm->cmd.buttons & BUTTON_WALKING)
					{
						if (pm->ps->fd.saberAnimLevel == SS_DUAL)
						{
							anim = PM_BlockingPoseForsaber_anim_levelDual();
						}
						else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
						{
							anim = PM_BlockingPoseForsaber_anim_levelStaff();
						}
						else
						{
							anim = PM_BlockingPoseForsaber_anim_levelSingle();
						}
					}
					else
					{
						anim = PM_IdlePoseForsaber_anim_level();
					}
				}
				break;
			}
			newmove = LS_READY;
		}

		if (pm->ps->saberHolstered == 2 && pm->watertype != CONTENTS_WATER)
		{
			//turn on the saber if it's not on
			pm->ps->saberHolstered = 0;
			PM_AddEvent(EV_SABER_UNHOLSTER);
		}

		PM_SetSaberMove(newmove);

		if (both && pm->ps->torsoAnim == anim)
		{
			PM_SetAnim(SETANIM_LEGS, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}

		if (!PM_InCartwheel(pm->ps->torsoAnim))
		{
			//can still attack during a cartwheel/arial
			//don't fire again until anim is done
			pm->ps->weaponTime = pm->ps->torsoTimer;
		}
	}

	// *********************************************************
	// WEAPON_FIRING
	// *********************************************************

	pm->ps->weaponstate = WEAPON_FIRING;

	if (pm->ps->weaponTime > 0 && is_holding_block_button && pm->cmd.buttons & BUTTON_WALKING)
	{
		if (pm->ps->fd.saberAnimLevel == SS_STAFF)
		{
			PM_SetAnim(SETANIM_TORSO, PM_BlockingPoseForsaber_anim_levelStaff(),
				SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		else if (pm->ps->fd.saberAnimLevel == SS_DUAL)
		{
			PM_SetAnim(SETANIM_TORSO, PM_BlockingPoseForsaber_anim_levelDual(),
				SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		else
		{
			PM_SetAnim(SETANIM_TORSO, PM_BlockingPoseForsaber_anim_levelSingle(),
				SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		PM_SetSaberMove(LS_READY);
	}

	int add_time = pm->ps->weaponTime;

	if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
	{
		if (!add_time)
		{
			add_time = weaponData[pm->ps->weapon].altFireTime;
		}
	}
	else
	{
		if (!add_time)
		{
			add_time = weaponData[pm->ps->weapon].fireTime;
		}
	}

	if (pm->ps->fd.forcePowersActive & 1 << FP_RAGE)
	{
		//addTime *= 0.75;
	}
	else if (pm->ps->fd.forceRageRecoveryTime > pm->cmd.serverTime)
	{
		//addTime *= 1.5;
	}

	if (!PM_InCartwheel(pm->ps->torsoAnim))
	{
		//can still attack during a cartwheel/arial
		pm->ps->weaponTime = add_time;
	}
}

//Add Fatigue to a player
void PM_AddFatigue(playerState_t* ps, const int fatigue)
{
	//For now, all saber attacks cost one FP.
	if (ps->fd.forcePower > fatigue)
	{
		ps->fd.forcePower -= fatigue;
	}
	else
	{
		//don't have enough so just completely drain FP then.
		ps->fd.forcePower = 0;
	}

	if (ps->fd.forcePower < BLOCKPOINTS_HALF)
	{
		ps->userInt3 |= 1 << FLAG_BLOCKDRAINED;
	}

	if (ps->fd.forcePower <= ps->fd.forcePowerMax * FATIGUEDTHRESHHOLD)
	{
		//Pop the Fatigued flag
		ps->userInt3 |= 1 << FLAG_FATIGUED;
	}
}

//Add Fatigue to a player
void PM_AddBlockFatigue(playerState_t* ps, const int fatigue)
{
	//For now, all saber attacks cost one BP.
	if (ps->fd.blockPoints > fatigue)
	{
		ps->fd.blockPoints -= fatigue;
	}
	else
	{
		//don't have enough so just completely drain FP then.
		ps->fd.blockPoints = 0;
	}

	if (ps->fd.blockPoints < BLOCKPOINTS_HALF)
	{
		ps->userInt3 |= 1 << FLAG_BLOCKDRAINED;
	}

	if (ps->fd.blockPoints <= ps->fd.blockPointsMax * FATIGUEDTHRESHHOLD)
	{
		//Pop the Fatigued flag
		ps->userInt3 |= 1 << FLAG_FATIGUED;
	}
}

static int Fatigue_SaberAttack()
{
	//returns the FP cost for a saber attack by this player.
	return FATIGUE_SABERATTACK;
}

//Add Fatigue for all the saber moves.
extern qboolean PM_KnockAwayStaffAndDuels(int move);

static void PM_NPCFatigue(playerState_t* ps, const int new_move)
{
	if (ps->saber_move != new_move)
	{
		//wasn't playing that attack before
		if (PM_KnockAwayStaffAndDuels(new_move))
		{
			//simple saber attack
			PM_AddBlockFatigue(ps, Fatigue_SaberAttack());
		}
		else if (PM_KnockawayForParry(new_move))
		{
			//simple saber attack
			PM_AddBlockFatigue(ps, Fatigue_SaberAttack());
		}
		else if (PM_SaberInbackblock(new_move))
		{
			//simple block
			PM_AddBlockFatigue(ps, Fatigue_SaberAttack());
		}
		else if (PM_IsInBlockingAnim(new_move))
		{
			//simple block
			PM_AddBlockFatigue(ps, Fatigue_SaberAttack());
		}
		else if (PM_SaberInTransition(new_move) && pm->ps->userInt3 & 1 << FLAG_ATTACKFAKE)
		{
			//attack fakes cost FP as well
			if (ps->fd.saberAnimLevel == SS_DUAL)
			{
				//dual sabers don't have transition/FP costs.
			}
			else
			{
				//single sabers
				PM_AddBlockFatigue(ps, Fatigue_SaberAttack());
			}
		}
	}
}

void PM_SaberFakeFlagUpdate(int new_move);
void PM_SaberPerfectBlockUpdate(int new_move);

void PM_SetJumped(const float height, const qboolean force)
{
	pm->ps->velocity[2] = height;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pm->ps->pm_flags |= PMF_JUMP_HELD;
	pm->ps->pm_flags |= PMF_JUMPING;
	pm->cmd.upmove = 0;

	if (force)
	{
		pm->ps->fd.forceJumpZStart = pm->ps->origin[2];
		pm->ps->pm_flags |= PMF_SLOW_MO_FALL;
		pm->ps->fd.forcePowersActive |= 1 << FP_LEVITATION;
	}
	else
	{
		PM_AddEvent(EV_JUMP);
	}
}

void WP_SaberFatigueRegenerate(const int override_amt)
{
	if (pm->ps->saberFatigueChainCount >= MISHAPLEVEL_NONE)
	{
		if (override_amt)
		{
			pm->ps->saberFatigueChainCount -= override_amt;
		}
		else
		{
			pm->ps->saberFatigueChainCount--;
		}
		if (pm->ps->saberFatigueChainCount > MISHAPLEVEL_MAX)
		{
			pm->ps->saberFatigueChainCount = MISHAPLEVEL_MAX;
		}
	}
}

void WP_BlasterFatigueRegenerate(const int override_amt)
{
	if (pm->ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_NONE)
	{
		if (override_amt)
		{
			pm->ps->BlasterAttackChainCount -= override_amt;
		}
		else
		{
			pm->ps->BlasterAttackChainCount--;
		}
		if (pm->ps->BlasterAttackChainCount > BLASTERMISHAPLEVEL_MAX)
		{
			pm->ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_MAX;
		}
	}
}

void PM_SetSaberMove(saber_moveName_t new_move)
{
	unsigned int setflags = saber_moveData[new_move].animSetFlags;
	int anim = saber_moveData[new_move].animToUse;
	int parts = SETANIM_TORSO;

	const saberInfo_t* saber1 = BG_MySaber(pm->ps->clientNum, 0);
	const saberInfo_t* saber2 = BG_MySaber(pm->ps->clientNum, 1);

	const qboolean is_holding_block_button = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Holding Block Button
	if (new_move == LS_READY || new_move == LS_A_FLIP_STAB || new_move == LS_A_FLIP_SLASH)
	{
		//finished with a kata (or in a special move) reset attack counter
		pm->ps->saberAttackChainCount = MISHAPLEVEL_NONE;
	}
	else if (PM_SaberInAttack(new_move))
	{
		//continuing with a kata, increment attack counter
		pm->ps->saberAttackChainCount++;

		if (pm->ps->saberFatigueChainCount < MISHAPLEVEL_MAX)
		{
			pm->ps->saberFatigueChainCount++;
		}
	}

	if (pm->ps->saberFatigueChainCount > MISHAPLEVEL_OVERLOAD)
	{
		//for the sake of being able to send the value over the net within a reasonable bit count
		pm->ps->saberFatigueChainCount = MISHAPLEVEL_MAX;
	}

	if (pm->ps->saberFatigueChainCount > MISHAPLEVEL_HUDFLASH)
	{
		pm->ps->userInt3 |= 1 << FLAG_ATTACKFATIGUE;
	}
	else
	{
		pm->ps->userInt3 &= ~(1 << FLAG_ATTACKFATIGUE);
	}

	if (pm->ps->saberFatigueChainCount > MISHAPLEVEL_LIGHT)
	{
		pm->ps->userInt3 |= 1 << FLAG_ATTACKFATIGUE;
	}
	else
	{
		pm->ps->userInt3 &= ~(1 << FLAG_ATTACKFATIGUE);
	}

	if (pm->ps->fd.blockPoints > BLOCK_POINTS_MAX)
	{
		//for the sake of being able to send the value over the net within a reasonable bit count
		pm->ps->fd.blockPoints = BLOCK_POINTS_MAX;
	}

	if (pm->ps->fd.blockPoints < BLOCK_POINTS_MIN)
	{
		//for the sake of being able to send the value over the net within a reasonable bit count
		pm->ps->fd.blockPoints = BLOCK_POINTS_MIN;
	}

	if (pm->ps->fd.blockPoints < BLOCKPOINTS_HALF)
	{
		pm->ps->userInt3 |= 1 << FLAG_BLOCKDRAINED;
	}
	else if (pm->ps->fd.blockPoints > BLOCKPOINTS_HALF)
	{
		//CANCEL THE BLOCK DRAINED FLAG
		pm->ps->userInt3 &= ~(1 << FLAG_BLOCKDRAINED);
	}

	if (pm->ps->fd.forcePower < BLOCKPOINTS_HALF)
	{
		pm->ps->userInt3 |= 1 << FLAG_BLOCKDRAINED;
	}
	else if (pm->ps->fd.forcePower > BLOCKPOINTS_HALF)
	{
		//CANCEL THE BLOCK DRAINED FLAG
		pm->ps->userInt3 &= ~(1 << FLAG_BLOCKDRAINED);
	}

	if (new_move == LS_DRAW || new_move == LS_DRAW2 || new_move == LS_DRAW3)
	{
		if (saber1
			&& saber1->drawAnim != -1)
		{
			anim = saber1->drawAnim;
		}
		else if (saber2
			&& saber2->drawAnim != -1)
		{
			anim = saber2->drawAnim;
		}
		else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
		{
			if (saber1 && (saber1->type == SABER_BACKHAND
				|| saber1->type == SABER_ASBACKHAND))
			{
				anim = BOTH_SABER_BACKHAND_IGNITION;
			}
			else if (saber1 && saber1->type == SABER_STAFF_MAUL)
			{
				anim = BOTH_S1_S7;
			}
			else
			{
				anim = BOTH_S1_S7;
			}
		}
		else if (pm->ps->fd.saberAnimLevel == SS_DUAL)
		{
			if (saber1 && saber1->type == SABER_GRIE || saber1 && saber1->type == SABER_GRIE4)
			{
				anim = BOTH_GRIEVOUS_SABERON;
			}
			else
			{
				anim = BOTH_S1_S6;
			}
		}
	}
	else if (new_move == LS_PUTAWAY)
	{
		if (saber1
			&& saber1->putawayAnim != -1)
		{
			anim = saber1->putawayAnim;
		}
		else if (saber2
			&& saber2->putawayAnim != -1)
		{
			anim = saber2->putawayAnim;
		}
		else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
		{
			anim = BOTH_S7_S1;
		}
		else if (pm->ps->fd.saberAnimLevel == SS_DUAL)
		{
			if (saber1 && saber1->type == SABER_GRIE || saber1 && saber1->type == SABER_GRIE4)
			{
				anim = BOTH_GRIEVOUS_SABERON;
			}
			else
			{
				anim = BOTH_S6_S1;
			}
		}
	}
	//different styles use different animations for the DFA move.
	else if (new_move == LS_A_JUMP_T__B_ && pm->ps->fd.saberAnimLevel == SS_DESANN)
	{
		anim = Q_irand(BOTH_FJSS_TR_BL, BOTH_FJSS_TL_BR);
	}
	else if (pm->ps->fd.saberAnimLevel == SS_STAFF && new_move >= LS_S_TL2BR && new_move < LS_REFLECT_LL)
	{
		//staff has an entirely new set of anims, besides special attacks
		if (new_move >= LS_V1_BR && new_move <= LS_REFLECT_LL)
		{
			//there aren't 1-7, just 1, 6 and 7, so just set it
			anim = BOTH_P7_S7_T_ + (anim - BOTH_P1_S1_T_); //shift it up to the proper set
		}
		else
		{
			//add the appropriate animLevel
			anim += (pm->ps->fd.saberAnimLevel - FORCE_LEVEL_1) * SABER_ANIM_GROUP_SIZE;
		}
	}
	else if (pm->ps->fd.saberAnimLevel == SS_DUAL && new_move >= LS_S_TL2BR && new_move < LS_REFLECT_LL)
	{
		//akimbo has an entirely new set of anims, besides special attacks
		if (new_move >= LS_V1_BR && new_move <= LS_REFLECT_LL)
		{
			//there aren't 1-7, just 1, 6 and 7, so just set it
			anim = BOTH_P6_S6_T_ + (anim - BOTH_P1_S1_T_); //shift it up to the proper set
		}
		else if ((new_move == LS_A_R2L || new_move == LS_S_R2L
			|| new_move == LS_A_L2R || new_move == LS_S_L2R)
			&& PM_CanDoDualDoubleAttacks()
			&& G_CheckEnemyPresence(DIR_RIGHT, 150.0f)
			&& G_CheckEnemyPresence(DIR_LEFT, 150.0f))
		{
			//enemy both on left and right
			new_move = LS_DUAL_LR;
			anim = saber_moveData[new_move].animToUse;
			//probably already moved, but...
			pm->cmd.rightmove = 0;
		}
		else if ((new_move == LS_A_T2B || new_move == LS_S_T2B
			|| new_move == LS_A_BACK || new_move == LS_A_BACK_CR)
			&& PM_CanDoDualDoubleAttacks()
			&& G_CheckEnemyPresence(DIR_FRONT, 150.0f)
			&& G_CheckEnemyPresence(DIR_BACK, 150.0f))
		{
			//enemy both in front and back
			new_move = LS_DUAL_FB;
			anim = saber_moveData[new_move].animToUse;
			//probably already moved, but...
			pm->cmd.forwardmove = 0;
		}
		else
		{
			//add the appropriate animLevel
			anim += (pm->ps->fd.saberAnimLevel - FORCE_LEVEL_1) * SABER_ANIM_GROUP_SIZE;
		}
	}
	else if (pm->ps->fd.saberAnimLevel > FORCE_LEVEL_1 &&
		!PM_SaberInIdle(new_move) && !PM_SaberInParry(new_move) && !PM_SaberInKnockaway(new_move) && !
		PM_SaberInBrokenParry(new_move) && !PM_SaberInReflect(new_move) && !PM_SaberInSpecial(new_move))
	{
		//readies, parries and reflections have only 1 level
		anim += (pm->ps->fd.saberAnimLevel - FORCE_LEVEL_1) * SABER_ANIM_GROUP_SIZE;
	}
	else if (new_move == LS_KICK_F_AIR
		|| new_move == LS_KICK_F_AIR2
		|| new_move == LS_KICK_B_AIR
		|| new_move == LS_KICK_R_AIR
		|| new_move == LS_KICK_L_AIR)
	{
		if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
		{
			PM_SetJumped(200, qtrue);
		}
	}

	if ((pm->ps->torsoAnim == anim || pm->ps->legsAnim == anim) && new_move > LS_PUTAWAY)
	{
		setflags |= SETANIM_FLAG_RESTART;
	}

	//saber torso anims should always be highest priority (4/12/02 - for special anims only)
	if (!pm->ps->m_iVehicleNum)
	{
		//if not riding a vehicle
		if (PM_SaberInSpecial(new_move))
		{
			setflags |= SETANIM_FLAG_OVERRIDE;
		}
	}
	if (PM_InSaberStandAnim(anim) || anim == BOTH_STAND1)
	{
		anim = pm->ps->legsAnim;

		if (anim >= BOTH_STAND1 && anim <= BOTH_STAND4TOATTACK2 ||
			anim >= TORSO_DROPWEAP1 && anim <= TORSO_WEAPONIDLE10)
		{
			//If standing then use the special saber stand anim
#ifdef _GAME
			if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
			{
				anim = PM_ReadyPoseForsaber_anim_levelBOT();
			}
			else
#endif
			{
				if (is_holding_block_button && pm->cmd.buttons & BUTTON_WALKING)
				{
					if (pm->ps->fd.saberAnimLevel == SS_DUAL)
					{
						anim = PM_BlockingPoseForsaber_anim_levelDual();
					}
					else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
					{
						anim = PM_BlockingPoseForsaber_anim_levelStaff();
					}
					else
					{
						anim = PM_BlockingPoseForsaber_anim_levelSingle();
					}
				}
				else
				{
					anim = PM_IdlePoseForsaber_anim_level();
				}
			}
		}

		if (pm->ps->pm_flags & PMF_DUCKED)
		{
			//Playing torso walk anims while crouched makes you look like a monkey
			if (is_holding_block_button && pm->cmd.buttons & BUTTON_WALKING)
			{
				if (pm->ps->fd.saberAnimLevel == SS_DUAL)
				{
					anim = PM_BlockingPoseForsaber_anim_levelDual();
				}
				else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
				{
					anim = PM_BlockingPoseForsaber_anim_levelStaff();
				}
				else
				{
					anim = PM_BlockingPoseForsaber_anim_levelSingle();
				}
			}
			else
			{
				anim = PM_ReadyPoseForsaber_anim_levelDucked();
			}
		}

		if (anim == BOTH_WALKBACK1 || anim == BOTH_WALKBACK2 || anim == BOTH_WALK1 || anim == BOTH_MENUIDLE1)
		{
			//normal stance when walking backward so saber doesn't look like it's cutting through leg
#ifdef _GAME
			if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
			{
				anim = PM_ReadyPoseForsaber_anim_levelBOT();
			}
			else
#endif
			{
				if (is_holding_block_button && pm->cmd.buttons & BUTTON_WALKING)
				{
					if (pm->ps->fd.saberAnimLevel == SS_DUAL)
					{
						anim = PM_BlockingPoseForsaber_anim_levelDual();
					}
					else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
					{
						anim = PM_BlockingPoseForsaber_anim_levelStaff();
					}
					else
					{
						anim = PM_BlockingPoseForsaber_anim_levelSingle();
					}
				}
				else
				{
					anim = PM_IdlePoseForsaber_anim_level();
				}
			}
		}

		if (PM_InSlopeAnim(anim))
		{
#ifdef _GAME
			if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
			{
				// Some special bot stuff.
				anim = PM_ReadyPoseForsaber_anim_levelBOT();
			}
			else
#endif
			{
				if (is_holding_block_button && pm->cmd.buttons & BUTTON_WALKING)
				{
					if (pm->ps->fd.saberAnimLevel == SS_DUAL)
					{
						anim = PM_BlockingPoseForsaber_anim_levelDual();
					}
					else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
					{
						anim = PM_BlockingPoseForsaber_anim_levelStaff();
					}
					else
					{
						anim = PM_BlockingPoseForsaber_anim_levelSingle();
					}
				}
				else
				{
					anim = PM_IdlePoseForsaber_anim_level();
				}
			}
		}

		parts = SETANIM_TORSO;
	}

	if (!pm->ps->m_iVehicleNum)
	{
		//if not riding a vehicle
		if (new_move == LS_JUMPATTACK_ARIAL_RIGHT ||
			new_move == LS_JUMPATTACK_ARIAL_LEFT)
		{
			//force only on legs
			parts = SETANIM_LEGS;
		}
		else if (new_move == LS_A_LUNGE
			|| new_move == LS_A_JUMP_T__B_
			|| new_move == LS_A_JUMP_PALP_
			|| new_move == LS_A_BACKSTAB
			|| new_move == LS_A_BACKSTAB_B
			|| new_move == LS_A_BACK
			|| new_move == LS_A_BACK_CR
			|| new_move == LS_ROLL_STAB
			|| new_move == LS_A_FLIP_STAB
			|| new_move == LS_A_FLIP_SLASH
			|| new_move == LS_JUMPATTACK_DUAL
			|| new_move == LS_GRIEVOUS_LUNGE
			|| new_move == LS_JUMPATTACK_ARIAL_LEFT
			|| new_move == LS_JUMPATTACK_ARIAL_RIGHT
			|| new_move == LS_JUMPATTACK_CART_LEFT
			|| new_move == LS_JUMPATTACK_CART_RIGHT
			|| new_move == LS_JUMPATTACK_STAFF_LEFT
			|| new_move == LS_JUMPATTACK_STAFF_RIGHT
			|| new_move == LS_BUTTERFLY_LEFT
			|| new_move == LS_BUTTERFLY_RIGHT
			|| new_move == LS_A_BACKFLIP_ATK
			|| new_move == LS_STABDOWN
			|| new_move == LS_STABDOWN_BACKHAND
			|| new_move == LS_STABDOWN_STAFF
			|| new_move == LS_STABDOWN_DUAL
			|| new_move == LS_DUAL_SPIN_PROTECT
			|| new_move == LS_DUAL_SPIN_PROTECT_GRIE
			|| new_move == LS_STAFF_SOULCAL
			|| new_move == LS_YODA_SPECIAL
			|| new_move == LS_A1_SPECIAL
			|| new_move == LS_A2_SPECIAL
			|| new_move == LS_A3_SPECIAL
			|| new_move == LS_A4_SPECIAL
			|| new_move == LS_A5_SPECIAL
			|| new_move == LS_GRIEVOUS_SPECIAL
			|| new_move == LS_UPSIDE_DOWN_ATTACK
			|| new_move == LS_PULL_ATTACK_STAB
			|| new_move == LS_PULL_ATTACK_SWING
			|| PM_SaberInBrokenParry(new_move)
			|| PM_kick_move(new_move))
		{
			parts = SETANIM_BOTH;
		}
		else if (PM_SpinningSaberAnim(anim))
		{
			//spins must be played on entire body
			parts = SETANIM_BOTH;
		}
		//coming out of a spin, force full body setting
		else if (PM_SpinningSaberAnim(pm->ps->legsAnim))
		{
			//spins must be played on entire body
			parts = SETANIM_BOTH;
			pm->ps->legsTimer = pm->ps->torsoTimer = 0;
		}
		else if (!pm->cmd.forwardmove && !pm->cmd.rightmove && !pm->cmd.upmove && !(pm->ps->pm_flags & PMF_DUCKED) ||
			is_holding_block_button && pm->cmd.buttons & BUTTON_WALKING)
		{
			//not trying to run, duck or jump
			if (is_holding_block_button && pm->cmd.buttons & BUTTON_WALKING
				&& !PM_SaberInParry(new_move)
				&& !PM_SaberInKnockaway(new_move)
				&& !PM_SaberInBrokenParry(new_move)
				&& !PM_SaberInReflect(new_move)
				&& !PM_SaberInSpecial(new_move))
			{
				parts = SETANIM_TORSO;
				if (pm->ps->fd.saberAnimLevel == SS_DUAL)
				{
					anim = PM_BlockingPoseForsaber_anim_levelDual();
				}
				else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
				{
					anim = PM_BlockingPoseForsaber_anim_levelStaff();
				}
				else
				{
					anim = PM_BlockingPoseForsaber_anim_levelSingle();
				}
			}
			else if (!PM_FlippingAnim(pm->ps->legsAnim)
				&& !BG_InRoll(pm->ps, pm->ps->legsAnim)
				&& !PM_InKnockDown(pm->ps)
				&& !PM_JumpingAnim(pm->ps->legsAnim)
				&& !PM_PainAnim(pm->ps->legsAnim)
				&& !PM_InSpecialJump(pm->ps->legsAnim)
				&& !PM_InSlopeAnim(pm->ps->legsAnim)
				&& pm->ps->groundEntityNum != ENTITYNUM_NONE
				&& !(pm->ps->pm_flags & PMF_DUCKED)
				&& new_move != LS_PUTAWAY)
			{
				parts = SETANIM_BOTH;
			}
			else if ((new_move == LS_SPINATTACK_DUAL || new_move == LS_SPINATTACK || new_move == LS_SPINATTACK_GRIEV || new_move == LS_GRIEVOUS_SPECIAL))
			{
				if (pm->ps->pm_flags & PMF_DUCKED)
				{
					parts = SETANIM_BOTH;
				}
				else
				{
					parts = SETANIM_TORSO;
				}
			}
		}

		PM_SetAnim(parts, anim, setflags);

		if (parts != SETANIM_LEGS &&
			(pm->ps->legsAnim == BOTH_ARIAL_LEFT ||
				pm->ps->legsAnim == BOTH_ARIAL_RIGHT))
		{
			if (pm->ps->legsTimer > pm->ps->torsoTimer)
			{
				pm->ps->legsTimer = pm->ps->torsoTimer;
			}
		}
	}

	if (pm->ps->torsoAnim == anim)
	{
		//successfully changed anims
		if (pm->ps->weapon == WP_SABER && !BG_SabersOff(pm->ps))
		{
#ifdef _GAME
			const qboolean is_holding_block_button_and_attack = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
			//Active Blocking

			if (!(g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT))
			{
			}
			else
#endif
			{
				PM_NPCFatigue(pm->ps, new_move); //drainblockpoints low cost
			}

			//update the flag
			PM_SaberFakeFlagUpdate(new_move);

			PM_SaberPerfectBlockUpdate(new_move);

			if (!PM_SaberInBounce(new_move) && !PM_SaberInReturn(new_move)) //or new move isn't slow bounce move
			{
				//switched away from a slow bounce move, remove the flags.
				pm->ps->userInt3 &= ~(1 << FLAG_SLOWBOUNCE);
				pm->ps->userInt3 &= ~(1 << FLAG_OLDSLOWBOUNCE);
				pm->ps->userInt3 &= ~(1 << FLAG_PARRIED);
				pm->ps->userInt3 &= ~(1 << FLAG_BLOCKING);
				pm->ps->userInt3 &= ~(1 << FLAG_BLOCKED);
			}

			if (!PM_SaberInMassiveBounce(pm->ps->torsoAnim))
			{
				//cancel out pre-block flag
				pm->ps->userInt3 &= ~(1 << FLAG_MBLOCKBOUNCE);
			}

			if (!PM_SaberInParry(new_move))
			{
				//cancel out pre-block flag
				pm->ps->userInt3 &= ~(1 << FLAG_PREBLOCK);
			}

			if (PM_SaberInAttack(new_move) || pm_saber_in_special_attack(anim))
			{
				if (pm->ps->saber_move != new_move)
				{
					//wasn't playing that attack before
					if (new_move != LS_KICK_F
						&& new_move != LS_KICK_F2
						&& new_move != LS_KICK_B
						&& new_move != LS_KICK_B2
						&& new_move != LS_KICK_B3
						&& new_move != LS_KICK_R
						&& new_move != LS_SLAP_R
						&& new_move != LS_KICK_L
						&& new_move != LS_SLAP_L
						&& new_move != LS_KICK_F_AIR
						&& new_move != LS_KICK_F_AIR2
						&& new_move != LS_KICK_B_AIR
						&& new_move != LS_KICK_R_AIR
						&& new_move != LS_KICK_L_AIR)
					{
						PM_AddEvent(EV_SABER_ATTACK);
					}

					if (pm->ps->brokenLimbs)
					{
						//randomly make pain sounds with a broken arm because we are suffering.
						int i_factor = -1;

						if (pm->ps->brokenLimbs & 1 << BROKENLIMB_RARM)
						{
							//You're using it more. So it hurts more.
							i_factor = 5;
						}
						else if (pm->ps->brokenLimbs & 1 << BROKENLIMB_LARM)
						{
							i_factor = 10;
						}

						if (i_factor != -1)
						{
							if (!PM_irand_timesync(0, i_factor))
							{
								BG_AddPredictableEventToPlayerstate(EV_PAIN, PM_irand_timesync(1, 100), pm->ps);
							}
						}
					}
				}
				else if (setflags & SETANIM_FLAG_RESTART && pm_saber_in_special_attack(anim))
				{
					//sigh, if restarted a special, then set the weaponTime *again*
					if (!PM_InCartwheel(pm->ps->torsoAnim))
					{
						//can still attack during a cartwheel/arial
						pm->ps->weaponTime = pm->ps->torsoTimer; //so we know our weapon is busy
					}
				}
			}
			else if (PM_SaberInStart(new_move))
			{
				const int damage_delay = 150;
				if (pm->ps->torsoTimer < damage_delay)
				{
					pm->ps->torsoTimer;
				}
			}

			if (PM_SaberInSpecial(new_move) &&
				pm->ps->weaponTime < pm->ps->torsoTimer)
			{
				//rww 01-02-03 - I think this will solve the issue of special attacks being interrupt able, hopefully without side effects
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
		}

		pm->ps->saber_move = new_move;
		pm->ps->saberBlocking = saber_moveData[new_move].blocking;

		pm->ps->torsoAnim = anim;

		if (pm->ps->clientNum == 0)
		{
			if (pm->ps->saberBlocked >= BLOCKED_UPPER_RIGHT_PROJ && pm->ps->saberBlocked <= BLOCKED_TOP_PROJ
				&& new_move >= LS_REFLECT_UP && new_move <= LS_REFLECT_LL)
			{
				//don't clear it when blocking projectiles
			}
			else
			{
				pm->ps->saberBlocked = BLOCKED_NONE;
			}
		}
		else if (pm->ps->saberBlocked <= BLOCKED_ATK_BOUNCE || !BG_SabersOff(pm->ps) || (new_move < LS_PARRY_UR ||
			new_move > LS_REFLECT_LL))
		{
			//NPCs only clear blocked if not blocking?
			pm->ps->saberBlocked = BLOCKED_NONE;
		}
	}
}

saberInfo_t* BG_MySaber(int clientNum, int saberNum)
{
	//returns a pointer to the requested saberNum
#ifdef _GAME
	const gentity_t* ent = &g_entities[clientNum];
	if (ent->inuse && ent->client)
	{
		if (!ent->client->saber[saberNum].model[0])
		{
			//don't have saber anymore!
			return NULL;
		}
		return &ent->client->saber[saberNum];
	}
#elif defined(_CGAME)
	clientInfo_t* ci = NULL;
	if (clientNum < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[clientNum];
	}
	else
	{
		const centity_t* cent = &cg_entities[clientNum];
		if (cent->npcClient)
		{
			ci = cent->npcClient;
		}
	}
	if (ci
		&& ci->infoValid)
	{
		if (!ci->saber[saberNum].model[0])
		{
			//don't have sabers anymore!
			return NULL;
		}
		return &ci->saber[saberNum];
	}
#endif

	return NULL;
}

qboolean PM_DoKick(void)
{
	//perform a kick.
	int kick_move = -1;

	const qboolean isPlayer = (pm->ps->clientNum < MAX_CLIENTS) ? qtrue : qfalse;

	if (!PM_KickingAnim(pm->ps->torsoAnim) &&
		!PM_KickingAnim(pm->ps->legsAnim) &&
		!BG_InRoll(pm->ps, pm->ps->legsAnim) &&
		pm->ps->weaponTime <= 0 &&
		pm->ps->communicatingflags & 1 << KICKING)
	{
		//player kicks
		kick_move = PM_CheckKick();
	}

	if (kick_move != -1)
	{
		if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
		{
			//if in air, convert kick to an in-air kick
			const float g_dist = PM_GroundDistance();
			//also looks wrong to transition from a non-complete flip anim...
			if ((!PM_FlippingAnim(pm->ps->legsAnim) || pm->ps->legsTimer <= 0) &&
				g_dist > 64.0f && //strict minimum
				g_dist > -pm->ps->velocity[2] - 64.0f)
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
				//leave it as a normal kick unless we're too high up
				if (g_dist > 128.0f || pm->ps->velocity[2] >= 0)
				{
					//off ground, but too close to ground
					kick_move = -1;
				}
			}
		}

		if (kick_move != -1 && BG_EnoughForcePowerForMove(FATIGUE_SABERATTACK, isPlayer))
		{
			PM_SetSaberMove(kick_move);
			return qtrue;
		}
	}

	return qfalse;
}

qboolean PM_DoSlap(void)
{
	//perform a kick.
	int kick_move = -1;

	const qboolean isPlayer = (pm->ps->clientNum < MAX_CLIENTS) ? qtrue : qfalse;

	if (!PM_SaberInBounce(pm->ps->saber_move)
		&& !PM_SaberInKnockaway(pm->ps->saber_move)
		&& !PM_SaberInBrokenParry(pm->ps->saber_move)
		&& !PM_kick_move(pm->ps->saber_move)
		&& !PM_KickingAnim(pm->ps->torsoAnim)
		&& !PM_KickingAnim(pm->ps->legsAnim)
		&& !BG_InRoll(pm->ps, pm->ps->legsAnim)
		&& !PM_InKnockDown(pm->ps)
		&& pm->ps->communicatingflags & 1 << KICKING) //not already in a kick
	{
		//player kicks
		kick_move = PM_MeleeMoveForConditions();
	}

	if (kick_move != -1)
	{
		if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
		{
			//if in air, convert kick to an in-air kick
			const float g_dist = PM_GroundDistance();
			//also looks wrong to transition from a non-complete flip anim...
			if ((!PM_FlippingAnim(pm->ps->legsAnim) || pm->ps->legsTimer <= 0) &&
				g_dist > 64.0f && //strict minimum
				g_dist > -pm->ps->velocity[2] - 64.0f)
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
				//leave it as a normal kick unless we're too high up
				if (g_dist > 128.0f || pm->ps->velocity[2] >= 0)
				{
					//off ground, but too close to ground
					kick_move = -1;
				}
			}
		}

		if (kick_move != -1 && BG_EnoughForcePowerForMove(FATIGUE_SABERATTACK, isPlayer))
		{
			PM_SetSaberMove(kick_move);
			return qtrue;
		}
	}

	return qfalse;
}

void PM_SaberFakeFlagUpdate(const int new_move)
{
	//checks to see if the attack fake flag needs to be removed.
	if (!PM_SaberInTransition(new_move) && !PM_SaberInStart(new_move) && !PM_SaberInAttackPure(new_move))
	{
		//not going into an attack move, clear the flag
		pm->ps->userInt3 &= ~(1 << FLAG_ATTACKFAKE);
	}
}

void PM_SaberPerfectBlockUpdate(const int new_move)
{
	const qboolean is_holding_block_button = pm->ps->ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;

	//checks to see if the flag needs to be removed.
	if ((!(is_holding_block_button)) || PM_SaberInBounce(new_move) || PM_SaberInMassiveBounce(pm->ps->torsoAnim) || PM_SaberInAttack(new_move))
	{
		pm->ps->userInt3 &= ~(1 << FLAG_PERFECTBLOCK);
	}
}

extern float bg_get_torso_anim_point(const playerState_t* ps, int anim_index);

qboolean PM_SaberInFullDamageMove(const playerState_t* ps, const int anim_index)
{
	//The player is attacking with a saber attack that does full damage
	const float torso_anim_point = bg_get_torso_anim_point(ps, anim_index);

	if (PM_SaberInAttack(ps->saber_move)
		|| PM_SaberInDamageMove(ps->saber_move)
		|| pm_saber_in_special_attack(ps->torsoAnim) //jacesolaris 2019 test for idle kill
		|| PM_SaberDoDamageAnim(ps->torsoAnim)
		&& !PM_kick_move(ps->saber_move)
		&& !PM_InSaberLock(ps->torsoAnim)
		|| PM_SuperBreakWinAnim(ps->torsoAnim))
	{
		//in attack animation
		if ((ps->saber_move == LS_A_FLIP_STAB || ps->saber_move == LS_A_FLIP_SLASH
			|| ps->saber_move == BOTH_JUMPFLIPSTABDOWN || ps->saber_move == BOTH_JUMPFLIPSLASHDOWN1)
			&& (torso_anim_point >= 0.30f && torso_anim_point <= 0.75f)) //assumes that the dude is
		{
			//flip attacks shouldn't do damage during the whole move.
			return qtrue;
		}

		if ((ps->saber_move == BOTH_ROLL_STAB
			|| ps->saber_move == LS_ROLL_STAB) && (torso_anim_point >= 0.30f && torso_anim_point <= 0.95f))
		{
			//don't do damage during the follow thru part of the roll stab.
			return qtrue;
		}

		if ((ps->saber_move == BOTH_STABDOWN || ps->saber_move == BOTH_STABDOWN_STAFF || ps->saber_move ==
			BOTH_STABDOWN_DUAL
			|| ps->saber_move == LS_STABDOWN || ps->saber_move == LS_STABDOWN_STAFF || ps->saber_move ==
			LS_STABDOWN_DUAL)
			&& (torso_anim_point >= 0.35f && torso_anim_point <= 0.95f))
		{
			//don't do damage during the follow thru part of the stab.
			return qtrue;
		}

		if (ps->saberBlocked == BLOCKED_NONE)
		{
			//and not attempting to do some sort of block animation
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean BG_SaberInPartialDamageMove(const playerState_t* ps, const int anim_index)
{
	//The player is attacking with a saber attack that does NO damage AT THIS POINT
	const float torso_anim_point = bg_get_torso_anim_point(ps, anim_index);

	if ((ps->saber_move == LS_A_FLIP_STAB || ps->saber_move == LS_A_FLIP_SLASH
		|| ps->saber_move == BOTH_JUMPFLIPSTABDOWN || ps->saber_move == BOTH_JUMPFLIPSLASHDOWN1)
		&& (torso_anim_point >= 0.30f && torso_anim_point <= 0.75f)) //assumes that the dude is
	{
		//flip attacks shouldn't do damage during the whole move.
		return qtrue;
	}

	if ((ps->saber_move == BOTH_ROLL_STAB
		|| ps->saber_move == LS_ROLL_STAB) && (torso_anim_point >= 0.30f && torso_anim_point <= 0.95f))
	{
		//don't do damage during the follow thru part of the roll stab.
		return qtrue;
	}

	if ((ps->saber_move == BOTH_STABDOWN || ps->saber_move == BOTH_STABDOWN_STAFF || ps->saber_move == BOTH_STABDOWN_DUAL
		|| ps->saber_move == LS_STABDOWN || ps->saber_move == LS_STABDOWN_STAFF || ps->saber_move == LS_STABDOWN_DUAL)
		&& (torso_anim_point >= 0.35f && torso_anim_point <= 0.95f))
	{
		//don't do damage during the follow thru part of the stab.
		return qtrue;
	}

	return qfalse;
}

qboolean BG_SaberInTransitionDamageMove(const playerState_t* ps)
{
	//player is in a saber move where it does transitional damage
	if (PM_SaberInTransition(ps->saber_move))
	{
		if (ps->saberBlocked == BLOCKED_NONE)
		{
			//and not attempting to do some sort of block animation
			return qtrue;
		}
	}
	return qfalse;
}

qboolean BG_SaberInNonIdleDamageMove(const playerState_t* ps, const int anim_index)
{
	//player is in a saber move that does something more than idle saber damage
	return PM_SaberInFullDamageMove(ps, anim_index);
}

qboolean BG_InSlowBounce(const playerState_t* ps)
{
	//checks for a bounce/return animation in combination with the slow bounce flag
	if (ps->userInt3 & 1 << FLAG_SLOWBOUNCE
		&& (PM_BounceAnim(ps->torsoAnim) || PM_SaberReturnAnim(ps->torsoAnim)))
	{
		//in slow bounce
		return qtrue;
	}
	if (PM_SaberInBounce(ps->saber_move))
	{
		//in slow bounce
		return qtrue;
	}
	return qfalse;
}

qboolean PM_InSlowBounce(const playerState_t* ps)
{
	//checks for a bounce/return animation in combination with the slow bounce flag
	if (ps->userInt3 & 1 << FLAG_SLOWBOUNCE
		&& (PM_BounceAnim(ps->torsoAnim)))
	{
		//in slow bounce
		return qtrue;
	}
	if (PM_SaberInBounce(ps->saber_move))
	{
		//in slow bounce
		return qtrue;
	}
	return qfalse;
}