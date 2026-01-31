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

#include "g_local.h"
#include "bg_local.h"
#include "w_saber.h"
#include "ai_main.h"

#define SABER_BOX_SIZE 16.0f
#define SABER_BIG_BOX_SIZE 16.0f
extern bot_state_t* botstates[MAX_CLIENTS];
extern qboolean in_front(vec3_t spot, vec3_t from, vec3_t from_angles, float thresh_hold);
extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);
extern void G_BlockLine(vec3_t start, vec3_t end, int color, int time);
extern float VectorDistance(vec3_t v1, vec3_t v2);
qboolean G_FindClosestPointOnLineSegment(const vec3_t start, const vec3_t end, const vec3_t from, vec3_t result);
extern qboolean G_GetHitLocFromSurfName(gentity_t* ent, const char* surfName, int* hit_loc, vec3_t point, vec3_t dir,
	vec3_t blade_dir, int mod);
extern int G_GetHitLocation(const gentity_t* target, vec3_t ppoint);
int saberSpinSound = 0;
extern saber_moveName_t PM_SaberBounceForAttack(int move);
qboolean PM_SaberInTransition(int move);
qboolean PM_SaberInDeflect(int move);
qboolean PM_SaberInBrokenParry(int move);
qboolean PM_SaberInBounce(int move);
qboolean BG_SaberInReturn(int move);
qboolean BG_InKnockDownOnGround(const playerState_t* ps);
extern qboolean BG_InKnockDown(int anim);
qboolean BG_StabDownAnim(int anim);
qboolean BG_SabersOff(const playerState_t* ps);
qboolean PM_SaberInTransitionAny(int move);
extern qboolean PM_SaberInAttackPure(int move);
qboolean WP_SaberBladeUseSecondBladeStyle(const saberInfo_t* saber, int blade_num);
qboolean WP_SaberBladeDoTransitionDamage(const saberInfo_t* saber, int blade_num);
void WP_SaberAddG2Model(gentity_t* saberent, const char* saber_model, qhandle_t saber_skin);
void WP_SaberRemoveG2Model(gentity_t* saberent);
extern qboolean BG_SaberInNonIdleDamageMove(const playerState_t* ps, int anim_index);
qboolean walk_check(const gentity_t* self);
qboolean saberKnockOutOfHand(gentity_t* saberent, gentity_t* saber_owner, vec3_t velocity);
extern qboolean PM_SuperBreakWinAnim(int anim);
extern stringID_table_t saber_moveTable[];
extern stringID_table_t animTable[MAX_ANIMATIONS + 1];
qboolean WP_SaberBlockNonRandom(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
qboolean wp_saber_block_non_random_missile(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
extern qboolean g_accurate_blocking(const gentity_t* blocker, const gentity_t* attacker, vec3_t hit_loc);
extern void PM_AddFatigue(playerState_t* ps, int fatigue);
extern qboolean PM_WalkingAnim(int anim);
extern qboolean PM_StandingAnim(int anim);
extern saber_moveName_t PM_BrokenParryForParry(int move);
extern saber_moveName_t pm_broken_parry_for_attack(int move);
extern saber_moveName_t PM_KnockawayForParry(int move);
extern saber_moveName_t PM_KnockawayForParryOld(int move);
qboolean G_HeavyMelee(const gentity_t* attacker);
qboolean PM_RunningAnim(int anim);
extern void Boba_FlyStart(gentity_t* self);
extern qboolean in_camera;
extern qboolean PM_InGetUp(const playerState_t* ps);
extern qboolean PM_InRoll(const playerState_t* ps);
extern qboolean jedi_dodge_evasion(gentity_t* self, const gentity_t* shooter, trace_t* tr, int hit_loc);
extern qboolean PM_CrouchAnim(int anim);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
extern void AddFatigueMeleeBonus(const gentity_t* attacker, const gentity_t* victim);
extern qboolean npc_is_dark_jedi(const gentity_t* self);
extern qboolean npc_is_light_jedi(const gentity_t* self);
extern qboolean PM_SaberInMassiveBounce(int anim);
extern qboolean PM_InForceGetUp(const playerState_t* ps);
extern void sab_beh_animate_slow_bounce_blocker(gentity_t* self);
extern void NPC_SetPainEvent(gentity_t* self);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
extern saber_moveName_t pm_block_the_attack(int move);
extern int g_block_the_attack(int move);
void WP_BlockPointsDrain(const gentity_t* self, int fatigue);
extern int Jedi_ReCalcParryTime(const gentity_t* self, evasionType_t evasion_type);
extern qboolean pm_saber_innonblockable_attack(int anim);
extern qboolean NPC_IsAlive(const gentity_t* self, const gentity_t* npc);
//////////////////////////////////////////////////
extern qboolean sab_beh_attack_vs_block(gentity_t* attacker, gentity_t* blocker, int saberNum, int blade_num, vec3_t hit_loc);
//////////////////////////////////////////////////
extern saber_moveName_t PM_AnimateOldKnockBack(int move);
extern int G_AnimateOldKnockBack(int move);
extern qboolean BG_IsAlreadyinTauntAnim(int anim);
extern qboolean PM_SaberInDamageMove(int move);
extern qboolean PM_SaberDoDamageAnim(int anim);
extern qboolean BG_SaberInTransitionDamageMove(const playerState_t* ps);
extern qboolean PM_InSlowBounce(const playerState_t* ps);
void DebounceSaberImpact(const gentity_t* self, const gentity_t* other_saberer, int rsaber_num, int rblade_num, int sabimpactentity_num);
extern qboolean BG_InFlipBack(int anim);
extern qboolean PM_SaberInBashedAnim(int anim);
extern qboolean PM_SaberInReturn(int move);
extern qboolean PM_CrouchingAnim(int anim);
int PlayerCanAbsorbKick(const gentity_t* defender, const vec3_t push_dir);
int BotCanAbsorbKick(const gentity_t* defender, const vec3_t push_dir);
extern qboolean PM_Dyinganim(const playerState_t* ps);
extern int SabBeh_AnimateMassiveDualSlowBounce(int anim);
extern int SabBeh_AnimateMassiveStaffSlowBounce(int anim);
extern qboolean PM_SaberInFullDamageMove(const playerState_t* ps, int anim_index);
extern void G_ClearEnemy(gentity_t* self);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern void wp_block_points_regenerate(const gentity_t* self, int override_amt);
extern void PM_AddBlockFatigue(playerState_t* ps, int fatigue);
qboolean WP_SaberBouncedSaberDirection(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
qboolean WP_SaberFatiguedParryDirection(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
extern void wp_block_points_regenerate_over_ride(const gentity_t* self, int override_amt);
extern qboolean BG_FullBodyTauntAnim(int anim);
extern int PM_InGrappleMove(int anim);
extern qboolean PM_SaberInKillMove(int move);
extern qboolean PM_WalkingOrRunningAnim(int anim);
extern qboolean PM_RestAnim(int anim);
extern qboolean sab_beh_block_vs_attack(gentity_t* blocker, gentity_t* attacker, int saberNum, int blade_num, vec3_t hit_loc);
extern qboolean BG_HopAnim(int anim);
extern void wp_force_power_regenerate(const gentity_t* self, int override_amt);
extern qboolean PM_SaberInOverHeadSlash(saber_moveName_t saber_move);
extern qboolean PM_SaberInBackAttack(saber_moveName_t saber_move);
qboolean WP_DoingForcedAnimationForForcePowers(const gentity_t* self);
void thrownSaberTouch(gentity_t* saberent, gentity_t* other, const trace_t* trace);
int WP_SaberCanBlockThrownSaber(gentity_t* self, vec3_t point, qboolean projectile);
void G_Beskar_Attack_Bounce(const gentity_t* self, gentity_t* other);

float VectorBlockDistance(vec3_t v1, vec3_t v2)
{
	//returns the distance between the two points.
	vec3_t dir;

	VectorSubtract(v2, v1, dir);
	return VectorLength(dir);
}

int G_GetParryForBlock(const int block)
{
	switch (block)
	{
	case BLOCKED_UPPER_RIGHT:
		return LS_PARRY_UR;
	case BLOCKED_UPPER_RIGHT_PROJ:
		return LS_K1_TR;
	case BLOCKED_UPPER_LEFT:
		return LS_PARRY_UL;
	case BLOCKED_UPPER_LEFT_PROJ:
		return LS_K1_TL;
	case BLOCKED_LOWER_RIGHT:
		return LS_PARRY_LR;
	case BLOCKED_LOWER_RIGHT_PROJ:
		return LS_REFLECT_LR;
	case BLOCKED_LOWER_LEFT:
		return LS_PARRY_LL;
	case BLOCKED_LOWER_LEFT_PROJ:
		return LS_REFLECT_LL;
	case BLOCKED_TOP:
		return LS_PARRY_UP;
	case BLOCKED_TOP_PROJ:
		return LS_REFLECT_UP;
	case BLOCKED_FRONT:
		return LS_PARRY_FRONT;
	case BLOCKED_FRONT_PROJ:
		return LS_REFLECT_FRONT;
	default:
		break;
	}

	return LS_NONE;
}

void G_Stagger(gentity_t* hit_ent)
{
	if (PM_InGetUp(&hit_ent->client->ps) || PM_InForceGetUp(&hit_ent->client->ps))
	{
		return;
	}

	const int anim_choice = irand(0, 6);
	// this could possibly be based on animation done when the clash happend, but this should do for now.
	int use_anim;

	switch (anim_choice)
	{
	default:
	case 0:
		use_anim = BOTH_BASHED1;
		break;
	case 1:
		use_anim = BOTH_H1_S1_T_;
		break;
	case 2:
		use_anim = BOTH_H1_S1_TR;
		break;
	case 3:
		use_anim = BOTH_H1_S1_TL;
		break;
	case 4:
		use_anim = BOTH_H1_S1_BL;
		break;
	case 5:
		use_anim = BOTH_H1_S1_B_;
		break;
	case 6:
		use_anim = BOTH_H1_S1_BR;
		break;
	}

	G_SetAnim(hit_ent, &hit_ent->client->pers.cmd, SETANIM_TORSO, use_anim, SETANIM_AFLAG_PACE, 0);

	if (PM_SaberInMassiveBounce(hit_ent->client->ps.torsoAnim))
	{
		hit_ent->client->ps.saber_move = LS_NONE;
		hit_ent->client->ps.saberBlocked = BLOCKED_NONE;
		hit_ent->client->ps.weaponTime = hit_ent->client->ps.torsoTimer;
		hit_ent->client->MassiveBounceAnimTime = hit_ent->client->ps.torsoTimer + level.time;
	}

	if (hit_ent->client->ps.fd.saberAnimLevel == SS_DUAL)
	{
		SabBeh_AnimateMassiveDualSlowBounce(use_anim);
	}
	else if (hit_ent->client->ps.fd.saberAnimLevel == SS_STAFF)
	{
		SabBeh_AnimateMassiveStaffSlowBounce(use_anim);
	}
}

void g_fatigue_bp_knockaway(gentity_t* blocker)
{
	if (PM_InGetUp(&blocker->client->ps) || PM_InForceGetUp(&blocker->client->ps))
	{
		return;
	}

	const int anim_choice = irand(0, 5);

	if (blocker->client->ps.fd.saberAnimLevel == SS_DUAL)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE, 0);
			break;
		case 1:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K6_S6_TR, SETANIM_AFLAG_PACE, 0);
			break;
		case 2:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K6_S6_TL, SETANIM_AFLAG_PACE, 0);
			break;
		case 3:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K6_S6_BL, SETANIM_AFLAG_PACE, 0);
			break;
		case 4:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K6_S6_B_, SETANIM_AFLAG_PACE, 0);
			break;
		case 5:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K6_S6_BR, SETANIM_AFLAG_PACE, 0);
			break;
		}
	}
	else if (blocker->client->ps.fd.saberAnimLevel == SS_STAFF)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE, 0);
			break;
		case 1:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K7_S7_TR, SETANIM_AFLAG_PACE, 0);
			break;
		case 2:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K7_S7_TL, SETANIM_AFLAG_PACE, 0);
			break;
		case 3:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K7_S7_BL, SETANIM_AFLAG_PACE, 0);
			break;
		case 4:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K7_S7_B_, SETANIM_AFLAG_PACE, 0);
			break;
		case 5:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K7_S7_BR, SETANIM_AFLAG_PACE, 0);
			break;
		}
	}
	else
	{
		switch (anim_choice)
		{
		default:
		case 0:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_T_, SETANIM_AFLAG_PACE, 0);
			break;
		case 1:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_TR_OLD, SETANIM_AFLAG_PACE, 0);
			break;
		case 2:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_TL_OLD, SETANIM_AFLAG_PACE, 0);
			break;
		case 3:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_BL, SETANIM_AFLAG_PACE, 0);
			break;
		case 4:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_B_, SETANIM_AFLAG_PACE, 0);
			break;
		case 5:
			G_SetAnim(blocker, &blocker->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_BR, SETANIM_AFLAG_PACE, 0);
			break;
		}
	}

	if (PM_SaberInMassiveBounce(blocker->client->ps.torsoAnim))
	{
		blocker->client->ps.saber_move = LS_NONE;
		blocker->client->ps.saberBlocked = BLOCKED_NONE;
		blocker->client->ps.weaponTime = blocker->client->ps.torsoTimer;
		blocker->client->MassiveBounceAnimTime = blocker->client->ps.torsoTimer + level.time;
	}
	else
	{
		if (!in_camera)
		{
			blocker->client->ps.saber_move = LS_READY;
		}
	}
}

// Attack stagger
void G_StaggerAttacker(gentity_t* atk)
{
	if (PM_InGetUp(&atk->client->ps) || PM_InForceGetUp(&atk->client->ps))
	{
		return;
	}

	const int anim_choice = irand(0, 7);

	if (atk->client->ps.fd.saberAnimLevel == SS_DUAL)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V6_BR_S6, SETANIM_AFLAG_PACE, 0);
			break;
		case 1:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V6__R_S6, SETANIM_AFLAG_PACE, 0);
			break;
		case 2:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V6_TR_S6, SETANIM_AFLAG_PACE, 0);
			break;
		case 3:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V6_T__S6, SETANIM_AFLAG_PACE, 0);
			break;
		case 4:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V6_TL_S6, SETANIM_AFLAG_PACE, 0);
			break;
		case 5:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V6__L_S6, SETANIM_AFLAG_PACE, 0);
			break;
		case 6:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V6_BL_S6, SETANIM_AFLAG_PACE, 0);
			break;
		case 7:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V6_B__S6, SETANIM_AFLAG_PACE, 0);
			break;
		}
	}
	else if (atk->client->ps.fd.saberAnimLevel == SS_STAFF)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V7_BR_S7, SETANIM_AFLAG_PACE, 0);
			break;
		case 1:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V7__R_S7, SETANIM_AFLAG_PACE, 0);
			break;
		case 2:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V7_TR_S7, SETANIM_AFLAG_PACE, 0);
			break;
		case 3:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V7_T__S7, SETANIM_AFLAG_PACE, 0);
			break;
		case 4:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V7_TL_S7, SETANIM_AFLAG_PACE, 0);
			break;
		case 5:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V7__L_S7, SETANIM_AFLAG_PACE, 0);
			break;
		case 6:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V7_BL_S7, SETANIM_AFLAG_PACE, 0);
			break;
		case 7:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V7_B__S7, SETANIM_AFLAG_PACE, 0);
			break;
		}
	}
	else
	{
		switch (anim_choice)
		{
		default:
		case 0:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V1_BR_S1, SETANIM_AFLAG_PACE, 0);
			break;
		case 1:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V1__R_S1, SETANIM_AFLAG_PACE, 0);
			break;
		case 2:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V1_TR_S1, SETANIM_AFLAG_PACE, 0);
			break;
		case 3:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V1_T__S1, SETANIM_AFLAG_PACE, 0);
			break;
		case 4:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V1_TL_S1, SETANIM_AFLAG_PACE, 0);
			break;
		case 5:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V1__L_S1, SETANIM_AFLAG_PACE, 0);
			break;
		case 6:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V1_BL_S1, SETANIM_AFLAG_PACE, 0);
			break;
		case 7:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_V1_B__S1, SETANIM_AFLAG_PACE, 0);
			break;
		}
	}

	if (PM_SaberInMassiveBounce(atk->client->ps.torsoAnim))
	{
		atk->client->ps.saber_move = LS_NONE;
		atk->client->ps.saberBlocked = BLOCKED_NONE;
		atk->client->ps.weaponTime = atk->client->ps.torsoTimer;
		atk->client->MassiveBounceAnimTime = atk->client->ps.torsoTimer + level.time;
	}
	else
	{
		if (!in_camera)
		{
			atk->client->ps.saber_move = LS_READY;
		}
	}
}

// Attack bounce
void G_BounceAttacker(gentity_t* atk)
{
	if (PM_InGetUp(&atk->client->ps) || PM_InForceGetUp(&atk->client->ps))
	{
		return;
	}

	const int anim_choice = irand(0, 6);

	if (atk->client->ps.fd.saberAnimLevel == SS_DUAL)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B6_BL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 1:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B6_BR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 2:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B6_TL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 3:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B6_TR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 4:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B6_T____, SETANIM_AFLAG_PACE, 0);
			break;
		case 5:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B6__L___, SETANIM_AFLAG_PACE, 0);
			break;
		case 6:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B6__R___, SETANIM_AFLAG_PACE, 0);
			break;
		}
	}
	else if (atk->client->ps.fd.saberAnimLevel == SS_STAFF)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B7_BL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 1:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B7_BR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 2:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B7_TL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 3:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B7_TR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 4:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B7_T____, SETANIM_AFLAG_PACE, 0);
			break;
		case 5:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B7__L___, SETANIM_AFLAG_PACE, 0);
			break;
		case 6:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B7__R___, SETANIM_AFLAG_PACE, 0);
			break;
		}
	}
	else if (atk->client->ps.fd.saberAnimLevel == SS_FAST)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B1_BL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 1:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B1_BR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 2:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B1_TL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 3:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B1_TR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 4:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B1_T____, SETANIM_AFLAG_PACE, 0);
			break;
		case 5:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B1__L___, SETANIM_AFLAG_PACE, 0);
			break;
		case 6:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B1__R___, SETANIM_AFLAG_PACE, 0);
			break;
		}
	}
	else if (atk->client->ps.fd.saberAnimLevel == SS_MEDIUM)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B2_BL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 1:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B2_BR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 2:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B2_TL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 3:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B2_TR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 4:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B2_T____, SETANIM_AFLAG_PACE, 0);
			break;
		case 5:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B2__L___, SETANIM_AFLAG_PACE, 0);
			break;
		case 6:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B2__R___, SETANIM_AFLAG_PACE, 0);
			break;
		}
	}
	else if (atk->client->ps.fd.saberAnimLevel == SS_STRONG)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B3_BL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 1:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B3_BR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 2:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B3_TL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 3:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B3_TR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 4:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B3_T____, SETANIM_AFLAG_PACE, 0);
			break;
		case 5:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B3__L___, SETANIM_AFLAG_PACE, 0);
			break;
		case 6:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B3__R___, SETANIM_AFLAG_PACE, 0);
			break;
		}
	}
	else if (atk->client->ps.fd.saberAnimLevel == SS_DESANN)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B4_BL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 1:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B4_BR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 2:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B4_TL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 3:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B4_TR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 4:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B4_T____, SETANIM_AFLAG_PACE, 0);
			break;
		case 5:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B4__L___, SETANIM_AFLAG_PACE, 0);
			break;
		case 6:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B4__R___, SETANIM_AFLAG_PACE, 0);
			break;
		}
	}
	else if (atk->client->ps.fd.saberAnimLevel == SS_TAVION)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B5_BL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 1:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B5_BR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 2:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B5_TL___, SETANIM_AFLAG_PACE, 0);
			break;
		case 3:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B5_TR___, SETANIM_AFLAG_PACE, 0);
			break;
		case 4:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B5_T____, SETANIM_AFLAG_PACE, 0);
			break;
		case 5:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B5__L___, SETANIM_AFLAG_PACE, 0);
			break;
		case 6:
			G_SetAnim(atk, &atk->client->pers.cmd, SETANIM_TORSO, BOTH_B5__R___, SETANIM_AFLAG_PACE, 0);
			break;
		}
	}

	if (PM_SaberInMassiveBounce(atk->client->ps.torsoAnim))
	{
		atk->client->ps.saber_move = LS_NONE;
		atk->client->ps.saberBlocked = BLOCKED_NONE;
		atk->client->ps.weaponTime = atk->client->ps.torsoTimer;
		atk->client->MassiveBounceAnimTime = atk->client->ps.torsoTimer + level.time;
	}
	else
	{
		if (!in_camera)
		{
			atk->client->ps.saber_move = LS_READY;
		}
	}
}

void G_Stumble(gentity_t* hit_ent)
{
	if (PM_SaberInBashedAnim(hit_ent->client->ps.torsoAnim))
	{
		return;
	}

	if (PM_InGetUp(&hit_ent->client->ps) || PM_InForceGetUp(&hit_ent->client->ps))
	{
		return;
	}

	const int anim_choice = irand(0, 6);
	// this could possibly be based on animation done when the clash happend, but this should do for now.
	int use_anim;

	switch (anim_choice)
	{
	default:
	case 0:
		use_anim = BOTH_BASHED1;
		break;
	case 1:
		use_anim = BOTH_PAIN3;
		break;
	case 2:
		use_anim = BOTH_PAIN2;
		break;
	case 3:
		use_anim = BOTH_PAIN15;
		break;
	case 4:
		use_anim = BOTH_PAIN12;
		break;
	case 5:
		use_anim = BOTH_PAIN5;
		break;
	case 6:
		use_anim = BOTH_PAIN7;
		break;
	}

	G_SetAnim(hit_ent, &hit_ent->client->pers.cmd, SETANIM_TORSO, use_anim, SETANIM_AFLAG_PACE, 0);

	if (PM_SaberInBashedAnim(hit_ent->client->ps.torsoAnim))
	{
		hit_ent->client->ps.saber_move = LS_NONE;
		hit_ent->client->ps.saberBlocked = BLOCKED_NONE;
		hit_ent->client->ps.weaponTime = hit_ent->client->ps.torsoTimer;
		hit_ent->client->MassiveBounceAnimTime = hit_ent->client->ps.torsoTimer + level.time;
	}
}

void bg_reduce_blaster_mishap_level_advanced(playerState_t* ps)
{
	//reduces a player's mishap meter by one level
	if (ps->BlasterAttackChainCount > BLASTERMISHAPLEVEL_MIN)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_NONE;
	}
	else
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_NONE;
	}
}

void BG_ReduceSaberMishapLevel(playerState_t* ps)
{
	//reduces a player's mishap meter by one level
	if (ps->saberFatigueChainCount >= MISHAPLEVEL_MAX)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_FULL;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_FULL)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_THIRTEEN;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_THIRTEEN)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_HUDFLASH;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_HUDFLASH)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_ELEVEN;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_ELEVEN)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_TEN;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_TEN)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_NINE;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_NINE)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_HEAVY;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_HEAVY)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_SEVEN;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_SEVEN)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_SIX;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_SIX)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_LIGHT;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_LIGHT)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_FOUR;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_FOUR)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_THREE;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_THREE)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_TWO;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_TWO)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_MIN;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_MIN)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_NONE;
	}
	else
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_NONE;
	}
}

//#ifdef DEBUG_SABER_BOX
static void G_DebugBoxLines(vec3_t mins, vec3_t maxs, const int duration)
{
	vec3_t start;
	vec3_t end;

	const float x = maxs[0] - mins[0];
	const float y = maxs[1] - mins[1];

	// top of box
	VectorCopy(maxs, start);
	VectorCopy(maxs, end);
	start[0] -= x;
	G_TestLine(start, end, 0x00000ff, duration);
	end[0] = start[0];
	end[1] -= y;
	G_TestLine(start, end, 0x00000ff, duration);
	start[1] = end[1];
	start[0] += x;
	G_TestLine(start, end, 0x00000ff, duration);
	G_TestLine(start, maxs, 0x00000ff, duration);
	// bottom of box
	VectorCopy(mins, start);
	VectorCopy(mins, end);
	start[0] += x;
	G_TestLine(start, end, 0x00000ff, duration);
	end[0] = start[0];
	end[1] += y;
	G_TestLine(start, end, 0x00000ff, duration);
	start[1] = end[1];
	start[0] -= x;
	G_TestLine(start, end, 0x00000ff, duration);
	G_TestLine(start, mins, 0x00000ff, duration);
}

static void G_DebugBlockBoxLines(vec3_t mins, vec3_t maxs, const int duration)
{
	vec3_t start;
	vec3_t end;

	const float x = maxs[0] - mins[0];
	const float y = maxs[1] - mins[1];

	// top of box
	VectorCopy(maxs, start);
	VectorCopy(maxs, end);
	start[0] -= x;
	G_BlockLine(start, end, 0x00000ff, duration);
	end[0] = start[0];
	end[1] -= y;
	G_BlockLine(start, end, 0x00000ff, duration);
	start[1] = end[1];
	start[0] += x;
	G_BlockLine(start, end, 0x00000ff, duration);
	G_BlockLine(start, maxs, 0x00000ff, duration);
	// bottom of box
	VectorCopy(mins, start);
	VectorCopy(mins, end);
	start[0] += x;
	G_BlockLine(start, end, 0x00000ff, duration);
	end[0] = start[0];
	end[1] += y;
	G_BlockLine(start, end, 0x00000ff, duration);
	start[1] = end[1];
	start[0] -= x;
	G_BlockLine(start, end, 0x00000ff, duration);
	G_BlockLine(start, mins, 0x00000ff, duration);
}

//#endif

//general check for performing certain attacks against others
qboolean G_CanBeEnemy(const gentity_t* self, const gentity_t* enemy)
{
	//ptrs!
	if (!self->inuse || !enemy->inuse || !self->client || !enemy->client)
		return qfalse;

	if (level.gametype < GT_TEAM)
		return qtrue;

	if (g_friendlyFire.integer)
		return qtrue;

	if (OnSameTeam(self, enemy))
		return qfalse;

	return qtrue;
}

//This function gets the attack power which is used to decide broken parries,
//knockaways, and numerous other things. It is not directly related to the
//actual amount of damage done, however. -rww
static QINLINE int g_saber_attack_power(gentity_t* ent, const qboolean attacking)
{
	assert(ent && ent->client);

	int base_level = ent->client->ps.fd.saberAnimLevel;

	//Give "medium" strength for the two special stances.
	if (base_level == SS_DUAL)
	{
		base_level = 2;
	}
	else if (base_level == SS_STAFF)
	{
		base_level = 2;
	}

	if (attacking)
	{
		//the attacker gets a boost to help penetrate defense.
		//General boost up so the individual levels make a bigger difference.
		base_level *= 2;

		base_level++;

		//Get the "speed" of the swing, roughly, and add more power
		//to the attack based on it.
		if (ent->client->lastSaberStorageTime >= level.time - 50 &&
			ent->client->olderIsValid)
		{
			vec3_t v_sub;
			int tolerance_amt;

			//We want different "tolerance" levels for adding in the distance of the last swing
			//to the base power level depending on which stance we are using. Otherwise fast
			//would have more advantage than it should since the animations are all much faster.
			switch (ent->client->ps.fd.saberAnimLevel)
			{
			case SS_STRONG:
				tolerance_amt = 8;
				break;
			case SS_MEDIUM:
				tolerance_amt = 16;
				break;
			case SS_FAST:
				tolerance_amt = 24;
				break;
			default: //dual, staff, etc.
				tolerance_amt = 16;
				break;
			}

			VectorSubtract(ent->client->lastSaberBase_Always, ent->client->olderSaberBase, v_sub);
			int swing_dist = (int)VectorLength(v_sub);

			while (swing_dist > 0)
			{
				//I would like to do something more clever. But I suppose this works, at least for now.
				base_level++;
				swing_dist -= tolerance_amt;
			}
		}

#ifndef FINAL_BUILD
		if (g_saberDebugPrint.integer > 1)
		{
			Com_Printf("Client %i: ATT STR: %i\n", ent->s.number, baseLevel);
		}
#endif
	}

	if (ent->client->ps.brokenLimbs & 1 << BROKENLIMB_RARM || ent->client->ps.brokenLimbs & 1 <<
		BROKENLIMB_LARM)
	{
		//We're very weak when one of our arms is broken
		base_level *= 0.3;
	}

	//Cap at reasonable values now.
	if (base_level < 1)
	{
		base_level = 1;
	}
	else if (base_level > 16)
	{
		base_level = 16;
	}

	if (level.gametype == GT_POWERDUEL &&
		ent->client->sess.duelTeam == DUELTEAM_LONE)
	{
		//get more power then
		return base_level * 2;
	}
	if (attacking && level.gametype == GT_SIEGE)
	{
		//in siege, saber battles should be quicker and more biased toward the attacker
		return base_level * 3;
	}

	return base_level;
}

void WP_DeactivateSaber(gentity_t* self)
{
	const qboolean blocking = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking
	const qboolean is_holding_block_button_and_attack = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
	//Active Blocking

	if (!self || !self->client)
	{
		return;
	}

	if (blocking || is_holding_block_button_and_attack)
	{
		return;
	}
	self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
	self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
	self->client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
	self->client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCBLOCKING);
	//keep my saber off!
	if (!self->client->ps.saberHolstered)
	{
		self->client->ps.saberHolstered = 2;

		//Doesn't matter ATM
		if (self->client->saber[0].soundOff)
		{
			G_Sound(self, CHAN_WEAPON, self->client->saber[0].soundOff);
		}

		if (self->client->saber[1].soundOff &&
			self->client->saber[1].model[0])
		{
			G_Sound(self, CHAN_WEAPON, self->client->saber[1].soundOff);
		}
	}
}

void WP_ActivateSaber(gentity_t* self)
{
	if (!self || !self->client)
	{
		return;
	}

	if (self->NPC &&
		self->client->ps.forceHandExtend == HANDEXTEND_JEDITAUNT &&
		self->client->ps.forceHandExtendTime - level.time > 200)
	{
		//if we're an NPC and in the middle of a taunt then stop it
		self->client->ps.forceHandExtend = HANDEXTEND_NONE;
		self->client->ps.forceHandExtendTime = 0;
	}
	else if (self->client->ps.fd.forceGripCripple)
	{
		//can't activate saber while being gripped
		return;
	}

	if (self->client->ps.saberHolstered == 2 && self->watertype != CONTENTS_WATER)
	{
		self->client->ps.saberHolstered = 0;
		if (self->client->saber[0].soundOn)
		{
			G_Sound(self, CHAN_WEAPON, self->client->saber[0].soundOn);
		}

		if (self->client->saber[1].soundOn)
		{
			G_Sound(self, CHAN_WEAPON, self->client->saber[1].soundOn);
		}
	}
}

#define PROPER_THROWN_VALUE 999 //Ah, well..

void SaberUpdateSelf(gentity_t* ent)
{
	if (ent->r.ownerNum == ENTITYNUM_NONE)
	{
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	if (!g_entities[ent->r.ownerNum].inuse ||
		!g_entities[ent->r.ownerNum].client/* ||
										   g_entities[ent->r.ownerNum].client->sess.sessionTeam == TEAM_SPECTATOR*/)
	{
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	if (g_entities[ent->r.ownerNum].client->ps.saberInFlight && g_entities[ent->r.ownerNum].health > 0)
	{
		//let The Master take care of us now (we'll get treated like a missile until we return)
		ent->nextthink = level.time;
		ent->genericValue5 = PROPER_THROWN_VALUE;
		return;
	}

	ent->genericValue5 = 0;

	if (g_entities[ent->r.ownerNum].client->ps.weapon != WP_SABER ||
		g_entities[ent->r.ownerNum].client->ps.pm_flags & PMF_FOLLOW ||
		//RWW ADDED 7-19-03 BEGIN
		g_entities[ent->r.ownerNum].client->sess.sessionTeam == TEAM_SPECTATOR ||
		g_entities[ent->r.ownerNum].client->tempSpectate >= level.time ||
		//RWW ADDED 7-19-03 END
		g_entities[ent->r.ownerNum].health < 1 ||
		BG_SabersOff(&g_entities[ent->r.ownerNum].client->ps) ||
		!g_entities[ent->r.ownerNum].client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] && g_entities[ent->r.ownerNum].s.
		eType != ET_NPC)
	{
		//owner is not using saber, spectating, dead, saber holstered, or has no attack level
		ent->r.contents = 0;
		ent->clipmask = 0;
	}
	else
	{
		//Standard contents (saber is active)
		//#ifdef DEBUG_SABER_BOX
		if ((d_saberInfo.integer || g_DebugSaberCombat.integer)
			&& !PM_SaberInAttack(g_entities[ent->r.ownerNum].client->ps.saber_move))
		{
			vec3_t dbg_mins;
			vec3_t dbg_maxs;

			VectorAdd(ent->r.currentOrigin, ent->r.mins, dbg_mins);
			VectorAdd(ent->r.currentOrigin, ent->r.maxs, dbg_maxs);

			G_DebugBlockBoxLines(dbg_mins, dbg_maxs, 10.0f / (float)sv_fps.integer * 100);
		}
		if (ent->r.contents != CONTENTS_LIGHTSABER)
		{
			if (level.time - g_entities[ent->r.ownerNum].client->lastSaberStorageTime <= 200)
			{
				//Only go back to solid once we're sure our owner has updated recently
				ent->r.contents = CONTENTS_LIGHTSABER;
				ent->clipmask = MASK_PLAYERSOLID | CONTENTS_LIGHTSABER;
			}
		}
		else
		{
			ent->r.contents = CONTENTS_LIGHTSABER;
			ent->clipmask = MASK_PLAYERSOLID | CONTENTS_LIGHTSABER;
		}
	}

	trap->LinkEntity((sharedEntity_t*)ent);

	ent->nextthink = level.time;
}

static void SaberGotHit(const gentity_t* self, gentity_t* other, trace_t* trace)
{
	const gentity_t* own = &g_entities[self->r.ownerNum];

	if (!own || !own->client)
	{
	}

	//Do something here..? Was handling projectiles here, but instead they're now handled in their own functions.
}

qboolean PM_SuperBreakLoseAnim(int anim);

static QINLINE void SetSaberBoxSize(gentity_t* saberent)
{
	gentity_t* owner = NULL;
	int i;
	int j = 0;
	int k = 0;
	qboolean dual_sabers = qfalse;
	qboolean always_block[MAX_SABERS][MAX_BLADES];
	qboolean force_block = qfalse;

	assert(saberent && saberent->inuse);

	if (saberent->r.ownerNum < MAX_CLIENTS && saberent->r.ownerNum >= 0)
	{
		owner = &g_entities[saberent->r.ownerNum];
	}
	else if (saberent->r.ownerNum >= 0 && saberent->r.ownerNum < ENTITYNUM_WORLD &&
		g_entities[saberent->r.ownerNum].s.eType == ET_NPC)
	{
		owner = &g_entities[saberent->r.ownerNum];
	}

	if (!owner || !owner->inuse || !owner->client)
	{
		if (level.gametype == GT_SINGLE_PLAYER)
		{
		}
		else
		{
			assert(!"Saber with no owner?");
		}
		return;
	}

	if (owner->client->saber[1].model[0])
	{
		dual_sabers = qtrue;
	}

	if (PM_SaberInBrokenParry(owner->client->ps.saber_move) || PM_SuperBreakLoseAnim(owner->client->ps.torsoAnim))
	{
		//let swings go right through when we're in this state
		for (i = 0; i < MAX_SABERS; i++)
		{
			if (i > 0 && !dual_sabers)
			{
				//not using a second saber, set it to not blocking
				for (j = 0; j < MAX_BLADES; j++)
				{
					always_block[i][j] = qfalse;
				}
			}
			else
			{
				if (owner->client->saber[i].saberFlags2 & SFL2_ALWAYS_BLOCK)
				{
					for (j = 0; j < owner->client->saber[i].numBlades; j++)
					{
						always_block[i][j] = qtrue;
						force_block = qtrue;
					}
				}
				if (owner->client->saber[i].bladeStyle2Start > 0)
				{
					for (j = owner->client->saber[i].bladeStyle2Start; j < owner->client->saber[i].numBlades; j++)
					{
						if (owner->client->saber[i].saberFlags2 & SFL2_ALWAYS_BLOCK2)
						{
							always_block[i][j] = qtrue;
							force_block = qtrue;
						}
						else
						{
							always_block[i][j] = qfalse;
						}
					}
				}
			}
		}
		if (!force_block)
		{
			//no sabers/blades to FORCE to be on, so turn off blocking altogether
			VectorSet(saberent->r.mins, 0, 0, 0);
			VectorSet(saberent->r.maxs, 0, 0, 0);
#ifndef FINAL_BUILD
			if (g_saberDebugPrint.integer > 1)
			{
				Com_Printf("Client %i in broken parry, saber box 0\n", owner->s.number);
			}
#endif
			return;
		}
	}

	if (level.time - owner->client->lastSaberStorageTime > 200)
	{
		//it's been too long since we got a reliable point storage, so use the defaults and leave.
		VectorSet(saberent->r.mins, -SABER_BOX_SIZE, -SABER_BOX_SIZE, -SABER_BOX_SIZE);
		VectorSet(saberent->r.maxs, SABER_BIG_BOX_SIZE, SABER_BIG_BOX_SIZE, SABER_BIG_BOX_SIZE);
		return;
	}
	if (owner->client->saber && level.time - owner->client->saber[j].blade[k].storageTime > 100)
	{
		//it's been too long since we got a reliable point storage, so use the defaults and leave.
		VectorSet(saberent->r.mins, -SABER_BOX_SIZE, -SABER_BOX_SIZE, -SABER_BOX_SIZE);
		VectorSet(saberent->r.maxs, SABER_BIG_BOX_SIZE, SABER_BIG_BOX_SIZE, SABER_BIG_BOX_SIZE);
		return;
	}

	if (dual_sabers
		|| owner->client->saber[0].numBlades > 1)
	{
		//dual sabers or multi-blade saber
		if (owner->client->ps.saberHolstered > 1)
		{
			//entirely off
			//no blocking at all
			VectorSet(saberent->r.mins, 0, 0, 0);
			VectorSet(saberent->r.maxs, 0, 0, 0);
			return;
		}
	}
	else
	{
		//single saber
		if (owner->client->ps.saberHolstered)
		{
			//off
			//no blocking at all
			VectorSet(saberent->r.mins, 0, 0, 0);
			VectorSet(saberent->r.maxs, 0, 0, 0);
			return;
		}
	}
	//Start out at the saber origin, then go through all the blades and push out the extents
	//for each blade, then set the box relative to the origin.
	VectorCopy(saberent->r.currentOrigin, saberent->r.mins);
	VectorCopy(saberent->r.currentOrigin, saberent->r.maxs);

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < MAX_SABERS; j++)
		{
			if (!owner->client->saber[j].model[0])
			{
				break;
			}
			if (dual_sabers
				&& owner->client->ps.saberHolstered == 1
				&& j == 1)
			{
				//this mother is holstered, get outta here.
				j++;
				continue;
			}
			for (k = 0; k < owner->client->saber[j].numBlades; k++)
			{
				vec3_t saber_tip;
				vec3_t saber_org;
				if (k > 0)
				{
					//not the first blade
					if (!dual_sabers)
					{
						//using a single saber
						if (owner->client->saber[j].numBlades > 1)
						{
							//with multiple blades
							if (owner->client->ps.saberHolstered == 1)
							{
								//all blades after the first one are off
								break;
							}
						}
					}
				}
				if (force_block)
				{
					//only do blocking with blades that are marked to block
					if (!always_block[j][k])
					{
						//this blade shouldn't be blocking
						continue;
					}
				}
				VectorCopy(owner->client->saber[j].blade[k].muzzlePoint, saber_org);
				VectorMA(owner->client->saber[j].blade[k].muzzlePoint, owner->client->saber[j].blade[k].lengthMax,
					owner->client->saber[j].blade[k].muzzleDir, saber_tip);

				if (saber_org[i] < saberent->r.mins[i])
				{
					saberent->r.mins[i] = saber_org[i];
				}
				if (saber_tip[i] < saberent->r.mins[i])
				{
					saberent->r.mins[i] = saber_tip[i];
				}

				if (saber_org[i] > saberent->r.maxs[i])
				{
					saberent->r.maxs[i] = saber_org[i];
				}
				if (saber_tip[i] > saberent->r.maxs[i])
				{
					saberent->r.maxs[i] = saber_tip[i];
				}
			}
		}
	}

	VectorSubtract(saberent->r.mins, saberent->r.currentOrigin, saberent->r.mins);
	VectorSubtract(saberent->r.maxs, saberent->r.currentOrigin, saberent->r.maxs);
}

void wp_saber_init_blade_data(const gentity_t* ent)
{
	gentity_t* saberent = NULL;
	int i = 0;

	while (i < level.num_entities)
	{
		//make sure there are no other saber entities floating around that think they belong to this client.
		gentity_t* check_ent = &g_entities[i];

		if (check_ent->inuse && check_ent->neverFree &&
			check_ent->r.ownerNum == ent->s.number &&
			check_ent->classname && check_ent->classname[0] &&
			!Q_stricmp(check_ent->classname, "lightsaber"))
		{
			if (saberent)
			{
				//already have one
				check_ent->neverFree = qfalse;
				check_ent->think = G_FreeEntity;
				check_ent->nextthink = level.time;
			}
			else
			{
				//hmm.. well then, take it as my own.
				//free the bitch but don't issue a kg2 to avoid overflowing clients.
				check_ent->s.modelGhoul2 = 0;
				G_FreeEntity(check_ent);

				//now init it manually and reuse this ent slot.
				G_InitGentity(check_ent);
				saberent = check_ent;
			}
		}

		i++;
	}

	//We do not want the client to have any real knowledge of the entity whatsoever. It will only
	//ever be used on the server.
	if (!saberent)
	{
		//ok, make one then
		saberent = G_Spawn();
	}
	ent->client->ps.saberEntityNum = ent->client->saberStoredIndex = saberent->s.number;
	saberent->classname = "lightsaber";

	saberent->neverFree = qtrue; //the saber being removed would be a terrible thing.

	saberent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	saberent->r.ownerNum = ent->s.number;

	saberent->clipmask = MASK_PLAYERSOLID | CONTENTS_LIGHTSABER;
	saberent->r.contents = CONTENTS_LIGHTSABER;

	SetSaberBoxSize(saberent);

	saberent->mass = 10;

	saberent->s.eFlags |= EF_NODRAW;
	saberent->r.svFlags |= SVF_NOCLIENT;

	saberent->s.modelGhoul2 = 1;
	//should we happen to be removed (we belong to an NPC and he is removed) then
	//we want to attempt to remove our g2 instance on the client in case we had one.

	saberent->touch = SaberGotHit;

	saberent->think = SaberUpdateSelf;
	saberent->genericValue5 = 0;
	saberent->nextthink = level.time + 50;

	saberSpinSound = G_SoundIndex("sound/weapons/saber/saberspin.wav");
}

#define LOOK_DEFAULT_SPEED	0.15f
#define LOOK_TALKING_SPEED	0.15f

static QINLINE qboolean G_CheckLookTarget(const gentity_t* ent, vec3_t look_angles, float* looking_speed)
{
	if (ent->s.eType == ET_NPC &&
		ent->s.m_iVehicleNum &&
		ent->s.NPC_class != CLASS_VEHICLE)
	{
		//an NPC bolted to a vehicle should just look around randomly
		if (TIMER_Done(ent, "lookAround"))
		{
			ent->NPC->shootAngles[YAW] = flrand(0, 360);
			TIMER_Set(ent, "lookAround", Q_irand(500, 3000));
		}
		VectorSet(look_angles, 0, ent->NPC->shootAngles[YAW], 0);
		return qtrue;
	}
	//Now calc head angle to lookTarget, if any
	if (ent->client->renderInfo.lookTarget >= 0 && ent->client->renderInfo.lookTarget < ENTITYNUM_WORLD)
	{
		vec3_t look_dir, look_org, eye_org;

		if (ent->client->renderInfo.lookMode == LM_ENT)
		{
			const gentity_t* look_cent = &g_entities[ent->client->renderInfo.lookTarget];
			if (look_cent)
			{
				if (look_cent != ent->enemy)
				{
					//We turn heads faster than headbob speed, but not as fast as if watching an enemy
					*looking_speed = LOOK_DEFAULT_SPEED;
				}
				if (look_cent->client)
				{
					VectorCopy(look_cent->client->renderInfo.eyePoint, look_org);
				}
				else if (look_cent->inuse && !VectorCompare(look_cent->r.currentOrigin, vec3_origin))
				{
					VectorCopy(look_cent->r.currentOrigin, look_org);
				}
				else
				{
					//at origin of world
					return qfalse;
				}
				//Look in dir of lookTarget
			}
		}
		else if (ent->client->renderInfo.lookMode == LM_INTEREST && ent->client->renderInfo.lookTarget > -1 && ent->
			client->renderInfo.lookTarget < MAX_INTEREST_POINTS)
		{
			VectorCopy(level.interestPoints[ent->client->renderInfo.lookTarget].origin, look_org);
		}
		else
		{
			return qfalse;
		}

		VectorCopy(ent->client->renderInfo.eyePoint, eye_org);

		VectorSubtract(look_org, eye_org, look_dir);

		vectoangles(look_dir, look_angles);

		for (int i = 0; i < 3; i++)
		{
			look_angles[i] = AngleNormalize180(look_angles[i]);
			ent->client->renderInfo.eyeAngles[i] = AngleNormalize180(ent->client->renderInfo.eyeAngles[i]);
		}
		AnglesSubtract(look_angles, ent->client->renderInfo.eyeAngles, look_angles);
		return qtrue;
	}

	return qfalse;
}

//rww - attempted "port" of the SP version which is completely client-side and
//uses illegal gentity access. I am trying to keep this from being too
//bandwidth-intensive.
//This is primarily droid stuff I guess, I'm going to try to handle all humanoid
//NPC stuff in with the actual player stuff if possible.
void NPC_SetBoneAngles(gentity_t* ent, const char* bone, vec3_t angles);

static QINLINE void G_G2NPCAngles(gentity_t* ent, matrix3_t legs, vec3_t angles)
{
	if (ent->client)
	{
		if (ent->client->NPC_class == CLASS_PROBE
			|| ent->client->NPC_class == CLASS_R2D2
			|| ent->client->NPC_class == CLASS_R5D2
			|| ent->client->NPC_class == CLASS_ATST)
		{
			vec3_t trailing_legs_angles;
			vec3_t look_angles;
			vec3_t view_angles;
			const char* cranium_bone = "cranium";
			if (ent->s.eType == ET_NPC &&
				ent->s.m_iVehicleNum &&
				ent->s.NPC_class != CLASS_VEHICLE)
			{
				//an NPC bolted to a vehicle should use the full angles
				VectorCopy(ent->r.currentAngles, angles);
			}
			else
			{
				VectorCopy(ent->client->ps.viewangles, angles);
				angles[PITCH] = 0;
			}

			VectorCopy(ent->client->ps.viewangles, view_angles);
			//			viewAngles[YAW] = viewAngles[ROLL] = 0;
			view_angles[PITCH] *= 0.5;
			VectorCopy(view_angles, look_angles);

			look_angles[1] = 0;

			if (ent->client->NPC_class == CLASS_ATST)
			{
				const char* thoracic_bone = "thoracic";
				//body pitch
				NPC_SetBoneAngles(ent, thoracic_bone, look_angles);
			}

			VectorCopy(view_angles, look_angles);

			if (ent && ent->client && ent->client->NPC_class == CLASS_ATST)
			{
				AnglesToAxis(trailing_legs_angles, legs);
			}
			else
			{
				//FIXME: this needs to properly set the legs.yawing field so we don't erroneously play the turning anim, but we do play it when turning in place
			}
			{
				float looking_speed = 0.3f;
				const qboolean looking = G_CheckLookTarget(ent, look_angles, &looking_speed);
				look_angles[PITCH] = look_angles[ROLL] = 0; //droids can't pitch or roll their heads
				if (looking)
				{
					//want to keep doing this lerp behavior for a full second after stopped looking (so don't snap)
					ent->client->renderInfo.lookingDebounceTime = level.time + 1000;
				}
			}

			if (ent && ent->client)
			{
				if (ent->client->renderInfo.lookingDebounceTime > level.time)
				{
					//adjust for current body orientation
					vec3_t old_look_angles;

					look_angles[YAW] -= 0;
					look_angles[YAW] = AngleNormalize180(look_angles[YAW]);

					//slowly lerp to this new value
					//Remember last headAngles
					VectorCopy(ent->client->renderInfo.lastHeadAngles, old_look_angles);
					if (VectorCompare(old_look_angles, look_angles) == qfalse)
					{
						look_angles[YAW] = old_look_angles[YAW] + (look_angles[YAW] - old_look_angles[YAW]) * 0.4f;
					}
					//Remember current lookAngles next time
					VectorCopy(look_angles, ent->client->renderInfo.lastHeadAngles);
				}
				else
				{
					//Remember current lookAngles next time
					VectorCopy(look_angles, ent->client->renderInfo.lastHeadAngles);
				}
			}
			if (ent && ent->client && ent->client->NPC_class == CLASS_ATST)
			{
				VectorCopy(ent->client->ps.viewangles, look_angles);
				look_angles[0] = look_angles[2] = 0;
				look_angles[YAW] -= trailing_legs_angles[YAW];
			}
			else
			{
				look_angles[PITCH] = look_angles[ROLL] = 0;
				look_angles[YAW] -= ent->client->ps.viewangles[YAW];
			}

			NPC_SetBoneAngles(ent, cranium_bone, look_angles);
		}
		else
		{
			//return;
		}
	}
}

static QINLINE void G_G2PlayerAngles(gentity_t* ent, matrix3_t legs, vec3_t legs_angles)
{
	qboolean t_pitching = qfalse,
		t_yawing = qfalse,
		l_yawing = qfalse;
	float t_yaw_angle = ent->client->ps.viewangles[YAW],
		t_pitch_angle = 0,
		l_yaw_angle = ent->client->ps.viewangles[YAW];

	const int ci_legs = ent->client->ps.legsAnim;
	const int ci_torso = ent->client->ps.torsoAnim;

	vec3_t lerp_org, lerp_ang;

	if (ent->s.eType == ET_NPC && ent->client)
	{
		//sort of hacky, but it saves a pretty big load off the server
		int i = 0;

		//If no real clients are in the same PVS then don't do any of this stuff, no one can see him anyway!
		while (i < MAX_CLIENTS)
		{
			const gentity_t* clEnt = &g_entities[i];

			if (clEnt && clEnt->inuse && clEnt->client &&
				trap->InPVS(clEnt->client->ps.origin, ent->client->ps.origin))
			{
				//this client can see him
				break;
			}

			i++;
		}

		if (i == MAX_CLIENTS)
		{
			//no one can see him, just return
			return;
		}
	}

	VectorCopy(ent->client->ps.origin, lerp_org);
	VectorCopy(ent->client->ps.viewangles, lerp_ang);

	if (ent->localAnimIndex <= 1)
	{
		vec3_t tur_angles;
		//don't do these things on non-humanoids
		vec3_t look_angles;
		const entityState_t* emplaced = NULL;

		if (ent->client->ps.hasLookTarget)
		{
			VectorSubtract(g_entities[ent->client->ps.lookTarget].r.currentOrigin, ent->client->ps.origin, look_angles);
			vectoangles(look_angles, look_angles);
			ent->client->lookTime = level.time + 1000;
		}
		else
		{
			VectorCopy(ent->client->ps.origin, look_angles);
		}
		look_angles[PITCH] = 0;

		if (ent->client->ps.emplacedIndex)
		{
			emplaced = &g_entities[ent->client->ps.emplacedIndex].s;
		}

		BG_G2PlayerAngles(ent->ghoul2, ent->client->renderInfo.motionBolt, &ent->s, level.time, lerp_org, lerp_ang,
			legs,
			legs_angles, &t_yawing, &t_pitching, &l_yawing, &t_yaw_angle, &t_pitch_angle, &l_yaw_angle,
			FRAMETIME,
			tur_angles,
			ent->modelScale, ci_legs, ci_torso, &ent->client->corrTime, look_angles,
			ent->client->lastHeadAngles,
			ent->client->lookTime, emplaced, NULL, ent->client->ps.ManualBlockingFlags);

		if (ent->client->ps.heldByClient && ent->client->ps.heldByClient <= MAX_CLIENTS)
		{
			//then put our arm in this client's hand
			//is index+1 because index 0 is valid.
			const int held_by_index = ent->client->ps.heldByClient - 1;
			gentity_t* other = &g_entities[held_by_index];
			int l_hand_bolt;

			if (other && other->inuse && other->client && other->ghoul2)
			{
				l_hand_bolt = trap->G2API_AddBolt(other->ghoul2, 0, "*l_hand");
			}
			else
			{
				//they left the game, perhaps?
				ent->client->ps.heldByClient = 0;
				return;
			}

			if (l_hand_bolt)
			{
				mdxaBone_t boltMatrix;
				vec3_t bolt_org;
				vec3_t t_angles;

				VectorCopy(other->client->ps.viewangles, t_angles);
				t_angles[PITCH] = t_angles[ROLL] = 0;

				trap->G2API_GetBoltMatrix(other->ghoul2, 0, l_hand_bolt, &boltMatrix, t_angles,
					other->client->ps.origin,
					level.time, 0, other->modelScale);
				bolt_org[0] = boltMatrix.matrix[0][3];
				bolt_org[1] = boltMatrix.matrix[1][3];
				bolt_org[2] = boltMatrix.matrix[2][3];

				BG_IK_MoveArm(ent->ghoul2, l_hand_bolt, level.time, &ent->s, ent->client->ps.torsoAnim/*BOTH_DEAD1*/,
					bolt_org, &ent->client->ikStatus,
					ent->client->ps.origin, ent->client->ps.viewangles, ent->modelScale, 500, qfalse);
			}
		}
		else if (ent->client->ikStatus)
		{
			//make sure we aren't IKing if we don't have anyone to hold onto us.
			int l_hand_bolt = 0;

			if (ent && ent->inuse)
			{
				if (ent->client && ent->ghoul2)
				{
					l_hand_bolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*l_hand");
				}
				else
				{
					//This shouldn't happen, but just in case it does, we'll have a failsafe.
					ent->client->ikStatus = qfalse;
				}
			}

			if (l_hand_bolt)
			{
				BG_IK_MoveArm(ent->ghoul2, l_hand_bolt, level.time, &ent->s,
					ent->client->ps.torsoAnim/*BOTH_DEAD1*/, vec3_origin, &ent->client->ikStatus,
					ent->client->ps.origin, ent->client->ps.viewangles, ent->modelScale, 500, qtrue);
			}
		}
	}
	else if (ent->m_pVehicle && ent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)
	{
		vec3_t look_angles;

		VectorCopy(ent->client->ps.viewangles, legs_angles);
		legs_angles[PITCH] = 0;
		AnglesToAxis(legs_angles, legs);

		VectorCopy(ent->client->ps.viewangles, look_angles);
		look_angles[YAW] = look_angles[ROLL] = 0;

		BG_G2ATSTAngles(ent->ghoul2, level.time, look_angles);
	}
	else if (ent->NPC)
	{
		//an NPC not using a humanoid skeleton, do special angle stuff.
		if (ent->s.eType == ET_NPC &&
			ent->s.NPC_class == CLASS_VEHICLE &&
			ent->m_pVehicle &&
			ent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
		{
			//fighters actually want to take pitch and roll into account for the axial angles
			VectorCopy(ent->client->ps.viewangles, legs_angles);
			AnglesToAxis(legs_angles, legs);
		}
		else
		{
			G_G2NPCAngles(ent, legs, legs_angles);
		}
	}
}

qboolean SaberAttacking(const gentity_t* self)
{
	if (PM_SaberInParry(self->client->ps.saber_move))
	{
		return qfalse;
	}
	if (PM_SaberInBrokenParry(self->client->ps.saber_move))
	{
		return qfalse;
	}
	if (PM_SaberInDeflect(self->client->ps.saber_move))
	{
		return qfalse;
	}
	if (PM_SaberInBounce(self->client->ps.saber_move))
	{
		return qfalse;
	}
	if (PM_SaberInKnockaway(self->client->ps.saber_move))
	{
		return qfalse;
	}

	if (PM_SaberInAttack(self->client->ps.saber_move))
	{
		if (self->client->ps.weaponstate == WEAPON_FIRING && self->client->ps.saberBlocked == BLOCKED_ATK_BOUNCE)
			// edit was BLOCKED_NONE
		{
			//if we're firing and not blocking, then we're attacking.
			return qtrue;
		}
	}

	if (PM_SaberInSpecial(self->client->ps.saber_move))
	{
		return qtrue;
	}

	return qfalse;
}

typedef enum
{
	LOCK_FIRST = 0,
	LOCK_TOP = LOCK_FIRST,
	LOCK_DIAG_TR,
	LOCK_DIAG_TL,
	LOCK_DIAG_BR,
	LOCK_DIAG_BL,
	LOCK_R,
	LOCK_L,
	LOCK_RANDOM,
	LOCK_KYLE_GRAB1,
	LOCK_KYLE_GRAB2,
	LOCK_KYLE_GRAB3,
	LOCK_FORCE_DRAIN
} saberslock_mode_t;

#define LOCK_IDEAL_DIST_TOP 32.0f
#define LOCK_IDEAL_DIST_CIRCLE 48.0f

static int G_SaberLockAnim(const int attacker_saber_style, const int defender_saber_style, const int top_or_side,
	const int lock_or_break_or_super_break,
	const int win_or_lose)
{
	int base_anim = -1;
	if (lock_or_break_or_super_break == SABER_LOCK_LOCK)
	{
		//special case: if we're using the same style and locking
		if (attacker_saber_style == defender_saber_style
			|| attacker_saber_style >= SS_FAST && attacker_saber_style <= SS_TAVION && defender_saber_style >= SS_FAST
			&&
			defender_saber_style <= SS_TAVION)
		{
			//using same style
			if (win_or_lose == SABER_LOCK_LOSE)
			{
				//you want the defender's stance...
				switch (defender_saber_style)
				{
				case SS_DUAL:
					if (top_or_side == SABER_LOCK_TOP)
					{
						base_anim = BOTH_LK_DL_DL_T_L_2;
					}
					else
					{
						base_anim = BOTH_LK_DL_DL_S_L_2;
					}
					break;
				case SS_STAFF:
					if (top_or_side == SABER_LOCK_TOP)
					{
						base_anim = BOTH_LK_ST_ST_T_L_2;
					}
					else
					{
						base_anim = BOTH_LK_ST_ST_S_L_2;
					}
					break;
				default:
					if (top_or_side == SABER_LOCK_TOP)
					{
						base_anim = BOTH_LK_S_S_T_L_2;
					}
					else
					{
						base_anim = BOTH_LK_S_S_S_L_2;
					}
					break;
				}
			}
		}
	}
	if (base_anim == -1)
	{
		switch (attacker_saber_style)
		{
		case SS_DUAL:
			switch (defender_saber_style)
			{
			case SS_DUAL:
				base_anim = BOTH_LK_DL_DL_S_B_1_L;
				break;
			case SS_STAFF:
				base_anim = BOTH_LK_DL_ST_S_B_1_L;
				break;
			default: //single
				base_anim = BOTH_LK_DL_S_S_B_1_L;
				break;
			}
			break;
		case SS_STAFF:
			switch (defender_saber_style)
			{
			case SS_DUAL:
				base_anim = BOTH_LK_ST_DL_S_B_1_L;
				break;
			case SS_STAFF:
				base_anim = BOTH_LK_ST_ST_S_B_1_L;
				break;
			default: //single
				base_anim = BOTH_LK_ST_S_S_B_1_L;
				break;
			}
			break;
		default: //single
			switch (defender_saber_style)
			{
			case SS_DUAL:
				base_anim = BOTH_LK_S_DL_S_B_1_L;
				break;
			case SS_STAFF:
				base_anim = BOTH_LK_S_ST_S_B_1_L;
				break;
			default: //single
				base_anim = BOTH_LK_S_S_S_B_1_L;
				break;
			}
			break;
		}
		//side lock or top lock?
		if (top_or_side == SABER_LOCK_TOP)
		{
			base_anim += 5;
		}
		//lock, break or superbreak?
		if (lock_or_break_or_super_break == SABER_LOCK_LOCK)
		{
			base_anim += 2;
		}
		else
		{
			//a break or superbreak
			if (lock_or_break_or_super_break == SABER_LOCK_SUPER_BREAK)
			{
				base_anim += 3;
			}
			//winner or loser?
			if (win_or_lose == SABER_LOCK_WIN)
			{
				base_anim += 1;
			}
		}
	}
	return base_anim;
}

extern qboolean BG_CheckIncrementLockAnim(int anim, int win_or_lose); //bg_saber.c
#define LOCK_IDEAL_DIST_JKA 46.0f//all of the new saberlocks are 46.08 from each other because Richard Lico is da MAN

static QINLINE qboolean WP_SabersCheckLock2(gentity_t* attacker, gentity_t* defender, saberslock_mode_t lockMode)
{
	int attAnim, defAnim = 0;
	float attStart = 0.5f, defStart = 0.5f;
	float ideal_dist = 48.0f;
	vec3_t att_angles, def_angles, def_dir;
	vec3_t new_org;
	vec3_t att_dir;
	float diff = 0;
	trace_t trace;

	//MATCH ANIMS

	if (lockMode == LOCK_KYLE_GRAB1
		|| lockMode == LOCK_KYLE_GRAB2
		|| lockMode == LOCK_KYLE_GRAB3)
	{
		float num_spins = 1.0f;
		ideal_dist = 46.0f; //42.0f;
		attStart = defStart = 0.0f;

		switch (lockMode)
		{
		default:
		case LOCK_KYLE_GRAB1:
			attAnim = BOTH_KYLE_PA_1;
			defAnim = BOTH_PLAYER_PA_1;
			num_spins = 2.0f;
			break;
		case LOCK_KYLE_GRAB2:
			attAnim = BOTH_KYLE_PA_3;
			defAnim = BOTH_PLAYER_PA_3;
			num_spins = 1.0f;
			break;
		case LOCK_KYLE_GRAB3:
			attAnim = BOTH_KYLE_PA_2;
			defAnim = BOTH_PLAYER_PA_2;
			num_spins = 3.0f;
			break;
		}
	}
	else if (lockMode == LOCK_FORCE_DRAIN)
	{
		ideal_dist = 46.0f; //42.0f;
		attStart = defStart = 0.0f;

		attAnim = BOTH_FORCE_DRAIN_GRAB_START;
		defAnim = BOTH_FORCE_DRAIN_GRABBED;
	}
	else
	{
		const int index_start = Q_irand(1, 5);

		if (lockMode == LOCK_RANDOM)
		{
			lockMode = (saberslock_mode_t)Q_irand(LOCK_FIRST, LOCK_RANDOM - 1);
		}
		if (attacker->client->ps.fd.saberAnimLevel >= SS_FAST
			&& attacker->client->ps.fd.saberAnimLevel <= SS_TAVION
			&& defender->client->ps.fd.saberAnimLevel >= SS_FAST
			&& defender->client->ps.fd.saberAnimLevel <= SS_TAVION)
		{
			switch (lockMode)
			{
			case LOCK_TOP:
				attAnim = BOTH_BF2LOCK;
				defAnim = BOTH_BF1LOCK;
				attStart = defStart = 0.5f;
				ideal_dist = LOCK_IDEAL_DIST_TOP;
				break;
			case LOCK_DIAG_TR:
				attAnim = BOTH_CCWCIRCLELOCK;
				defAnim = BOTH_CWCIRCLELOCK;
				attStart = defStart = 0.5f;
				ideal_dist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_DIAG_TL:
				attAnim = BOTH_CWCIRCLELOCK;
				defAnim = BOTH_CCWCIRCLELOCK;
				attStart = defStart = 0.5f;
				ideal_dist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_DIAG_BR:
				attAnim = BOTH_CWCIRCLELOCK;
				defAnim = BOTH_CCWCIRCLELOCK;
				attStart = defStart = 0.15f;
				ideal_dist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_DIAG_BL:
				attAnim = BOTH_CCWCIRCLELOCK;
				defAnim = BOTH_CWCIRCLELOCK;
				attStart = defStart = 0.15f;
				ideal_dist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_R:
				attAnim = BOTH_CCWCIRCLELOCK;
				defAnim = BOTH_CWCIRCLELOCK;
				attStart = defStart = 0.25f;
				ideal_dist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_L:
				attAnim = BOTH_CWCIRCLELOCK;
				defAnim = BOTH_CCWCIRCLELOCK;
				attStart = defStart = 0.25f;
				ideal_dist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			default:
				return qfalse;
				break;
			}
			G_Sound(attacker, CHAN_AUTO, G_SoundIndex(va("sound/weapons/saber/saber_locking_start%d.mp3", index_start)));
		}
		else
		{
			//use the new system
			ideal_dist = LOCK_IDEAL_DIST_JKA;
			//all of the new saberlocks are 46.08 from each other because Richard Lico is da MAN
			if (lockMode == LOCK_TOP)
			{
				//top lock
				attAnim = G_SaberLockAnim(attacker->client->ps.fd.saberAnimLevel,
					defender->client->ps.fd.saberAnimLevel, SABER_LOCK_TOP, SABER_LOCK_LOCK,
					SABER_LOCK_WIN);
				defAnim = G_SaberLockAnim(defender->client->ps.fd.saberAnimLevel,
					attacker->client->ps.fd.saberAnimLevel, SABER_LOCK_TOP, SABER_LOCK_LOCK,
					SABER_LOCK_LOSE);
				attStart = defStart = 0.5f;
			}
			else
			{
				//side lock
				switch (lockMode)
				{
				case LOCK_DIAG_TR:
					attAnim = G_SaberLockAnim(attacker->client->ps.fd.saberAnimLevel,
						defender->client->ps.fd.saberAnimLevel, SABER_LOCK_SIDE,
						SABER_LOCK_LOCK,
						SABER_LOCK_WIN);
					defAnim = G_SaberLockAnim(defender->client->ps.fd.saberAnimLevel,
						attacker->client->ps.fd.saberAnimLevel, SABER_LOCK_SIDE,
						SABER_LOCK_LOCK,
						SABER_LOCK_LOSE);
					attStart = defStart = 0.5f;
					break;
				case LOCK_DIAG_TL:
					attAnim = G_SaberLockAnim(attacker->client->ps.fd.saberAnimLevel,
						defender->client->ps.fd.saberAnimLevel, SABER_LOCK_SIDE,
						SABER_LOCK_LOCK,
						SABER_LOCK_LOSE);
					defAnim = G_SaberLockAnim(defender->client->ps.fd.saberAnimLevel,
						attacker->client->ps.fd.saberAnimLevel, SABER_LOCK_SIDE,
						SABER_LOCK_LOCK,
						SABER_LOCK_WIN);
					attStart = defStart = 0.5f;
					break;
				case LOCK_DIAG_BR:
					attAnim = G_SaberLockAnim(attacker->client->ps.fd.saberAnimLevel,
						defender->client->ps.fd.saberAnimLevel, SABER_LOCK_SIDE,
						SABER_LOCK_LOCK,
						SABER_LOCK_WIN);
					defAnim = G_SaberLockAnim(defender->client->ps.fd.saberAnimLevel,
						attacker->client->ps.fd.saberAnimLevel, SABER_LOCK_SIDE,
						SABER_LOCK_LOCK,
						SABER_LOCK_LOSE);
					if (BG_CheckIncrementLockAnim(attAnim, SABER_LOCK_WIN))
					{
						attStart = 0.85f; //move to end of anim
					}
					else
					{
						attStart = 0.15f; //start at beginning of anim
					}
					if (BG_CheckIncrementLockAnim(defAnim, SABER_LOCK_LOSE))
					{
						defStart = 0.85f; //start at end of anim
					}
					else
					{
						defStart = 0.15f; //start at beginning of anim
					}
					break;
				case LOCK_DIAG_BL:
					attAnim = G_SaberLockAnim(attacker->client->ps.fd.saberAnimLevel,
						defender->client->ps.fd.saberAnimLevel, SABER_LOCK_SIDE,
						SABER_LOCK_LOCK,
						SABER_LOCK_LOSE);
					defAnim = G_SaberLockAnim(defender->client->ps.fd.saberAnimLevel,
						attacker->client->ps.fd.saberAnimLevel, SABER_LOCK_SIDE,
						SABER_LOCK_LOCK,
						SABER_LOCK_WIN);
					if (BG_CheckIncrementLockAnim(attAnim, SABER_LOCK_WIN))
					{
						attStart = 0.85f; //move to end of anim
					}
					else
					{
						attStart = 0.15f; //start at beginning of anim
					}
					if (BG_CheckIncrementLockAnim(defAnim, SABER_LOCK_LOSE))
					{
						defStart = 0.85f; //start at end of anim
					}
					else
					{
						defStart = 0.15f; //start at beginning of anim
					}
					break;
				case LOCK_R:
					attAnim = G_SaberLockAnim(attacker->client->ps.fd.saberAnimLevel,
						defender->client->ps.fd.saberAnimLevel, SABER_LOCK_SIDE,
						SABER_LOCK_LOCK,
						SABER_LOCK_LOSE);
					defAnim = G_SaberLockAnim(defender->client->ps.fd.saberAnimLevel,
						attacker->client->ps.fd.saberAnimLevel, SABER_LOCK_SIDE,
						SABER_LOCK_LOCK,
						SABER_LOCK_WIN);
					if (BG_CheckIncrementLockAnim(attAnim, SABER_LOCK_WIN))
					{
						attStart = 0.75f; //move to end of anim
					}
					else
					{
						attStart = 0.25f; //start at beginning of anim
					}
					if (BG_CheckIncrementLockAnim(defAnim, SABER_LOCK_LOSE))
					{
						defStart = 0.75f; //start at end of anim
					}
					else
					{
						defStart = 0.25f; //start at beginning of anim
					}
					break;
				case LOCK_L:
					attAnim = G_SaberLockAnim(attacker->client->ps.fd.saberAnimLevel,
						defender->client->ps.fd.saberAnimLevel, SABER_LOCK_SIDE,
						SABER_LOCK_LOCK,
						SABER_LOCK_WIN);
					defAnim = G_SaberLockAnim(defender->client->ps.fd.saberAnimLevel,
						attacker->client->ps.fd.saberAnimLevel, SABER_LOCK_SIDE,
						SABER_LOCK_LOCK,
						SABER_LOCK_LOSE);
					//attacker starts with advantage
					if (BG_CheckIncrementLockAnim(attAnim, SABER_LOCK_WIN))
					{
						attStart = 0.75f; //move to end of anim
					}
					else
					{
						attStart = 0.25f; //start at beginning of anim
					}
					if (BG_CheckIncrementLockAnim(defAnim, SABER_LOCK_LOSE))
					{
						defStart = 0.75f; //start at end of anim
					}
					else
					{
						defStart = 0.25f; //start at beginning of anim
					}
					break;
				default:
					return qfalse;
				}
			}
			G_Sound(attacker, CHAN_AUTO, G_SoundIndex(va("sound/weapons/saber/saber_locking_start%d.mp3", index_start)));
		}
	}

	G_SetAnim(attacker, NULL, SETANIM_BOTH, attAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);

	attacker->client->ps.saberLockFrame = bgAllAnims[attacker->localAnimIndex].anims[attAnim].firstFrame + bgAllAnims[attacker->localAnimIndex].anims[attAnim].numFrames * attStart;

	G_SetAnim(defender, NULL, SETANIM_BOTH, defAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);

	defender->client->ps.saberLockFrame = bgAllAnims[defender->localAnimIndex].anims[defAnim].firstFrame + bgAllAnims[defender->localAnimIndex].anims[defAnim].numFrames * defStart;

	attacker->client->ps.saberLockHits = 0;
	defender->client->ps.saberLockHits = 0;

	attacker->client->ps.saberLockAdvance = qfalse;
	defender->client->ps.saberLockAdvance = qfalse;

	VectorClear(attacker->client->ps.velocity);
	VectorClear(defender->client->ps.velocity);
	attacker->client->ps.saberLockTime = defender->client->ps.saberLockTime = level.time + 10000;
	attacker->client->ps.saberLockEnemy = defender->s.number;
	defender->client->ps.saberLockEnemy = attacker->s.number;
	attacker->client->ps.weaponTime = defender->client->ps.weaponTime = Q_irand(1000, 3000);
	//delay 1 to 3 seconds before pushing
	attacker->client->ps.userInt3 &= ~(1 << FLAG_ATTACKFAKE);
	defender->client->ps.userInt3 &= ~(1 << FLAG_ATTACKFAKE);

	VectorSubtract(defender->r.currentOrigin, attacker->r.currentOrigin, def_dir);
	VectorCopy(attacker->client->ps.viewangles, att_angles);
	att_angles[YAW] = vectoyaw(def_dir);
	SetClientViewAngle(attacker, att_angles);
	def_angles[PITCH] = att_angles[PITCH] * -1;
	def_angles[YAW] = AngleNormalize180(att_angles[YAW] + 180);
	def_angles[ROLL] = 0;
	SetClientViewAngle(defender, def_angles);

	//MATCH POSITIONS
	diff = VectorNormalize(def_dir) - ideal_dist; //diff will be the total error in dist
	//try to move attacker half the diff towards the defender
	VectorMA(attacker->r.currentOrigin, diff * 0.5f, def_dir, new_org);

	trap->Trace(&trace, attacker->r.currentOrigin, attacker->r.mins, attacker->r.maxs, new_org, attacker->s.number,
		attacker->clipmask, qfalse, 0, 0);
	if (!trace.startsolid && !trace.allsolid)
	{
		G_SetOrigin(attacker, trace.endpos);
		if (attacker->client)
		{
			VectorCopy(trace.endpos, attacker->client->ps.origin);
		}
		trap->LinkEntity((sharedEntity_t*)attacker);
	}
	//now get the defender's dist and do it for him too
	VectorSubtract(attacker->r.currentOrigin, defender->r.currentOrigin, att_dir);
	diff = VectorNormalize(att_dir) - ideal_dist; //diff will be the total error in dist
	//try to move defender all of the remaining diff towards the attacker
	VectorMA(defender->r.currentOrigin, diff, att_dir, new_org);
	trap->Trace(&trace, defender->r.currentOrigin, defender->r.mins, defender->r.maxs, new_org, defender->s.number,
		defender->clipmask, qfalse, 0, 0);
	if (!trace.startsolid && !trace.allsolid)
	{
		if (defender->client)
		{
			VectorCopy(trace.endpos, defender->client->ps.origin);
		}
		G_SetOrigin(defender, trace.endpos);
		trap->LinkEntity((sharedEntity_t*)defender);
	}

	//DONE!
	return qtrue;
}

extern saber_moveData_t saber_moveData[LS_MOVE_MAX];

qboolean WP_SabersCheckLock(gentity_t* ent1, gentity_t* ent2)
{
	qboolean lock_quad;

	if (g_debugSaberLocks.integer)
	{
		WP_SabersCheckLock2(ent1, ent2, LOCK_RANDOM);
		return qtrue;
	}

	if (!g_saberLocking.integer)
	{
		return qfalse;
	}

	if (!ent1->client || !ent2->client)
	{
		return qfalse;
	}

	if (ent1->s.eType == ET_NPC ||
		ent2->s.eType == ET_NPC)
	{
		//if either ents is NPC, then never let an NPC lock with someone on the same playerTeam
		if (ent1->client->playerTeam == ent2->client->playerTeam)
		{
			return qfalse;
		}
	}

	if (!ent1->client->ps.saberEntityNum ||
		!ent2->client->ps.saberEntityNum ||
		ent1->client->ps.saberInFlight ||
		ent2->client->ps.saberInFlight)
	{
		//can't get in lock if one of them has had the saber knocked out of his hand
		return qfalse;
	}

	if (fabs(ent1->r.currentOrigin[2] - ent2->r.currentOrigin[2]) > 16)
	{
		return qfalse;
	}

	if (ent1->client->ps.groundEntityNum == ENTITYNUM_NONE ||
		ent2->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		return qfalse;
	}

	const float dist = DistanceSquared(ent1->r.currentOrigin, ent2->r.currentOrigin);

	if (dist < 64 || dist > 6400)
	{
		//between 8 and 80 from each other
		return qfalse;
	}

	if (PM_InSpecialJump(ent1->client->ps.legsAnim))
	{
		return qfalse;
	}
	if (PM_InSpecialJump(ent2->client->ps.legsAnim))
	{
		return qfalse;
	}

	if (BG_InRoll(&ent1->client->ps, ent1->client->ps.legsAnim))
	{
		return qfalse;
	}
	if (BG_InRoll(&ent2->client->ps, ent2->client->ps.legsAnim))
	{
		return qfalse;
	}

	if (ent1->client->ps.forceHandExtend != HANDEXTEND_NONE ||
		ent2->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return qfalse;
	}

	if (ent1->client->ps.pm_flags & PMF_DUCKED ||
		ent2->client->ps.pm_flags & PMF_DUCKED)
	{
		return qfalse;
	}

	if (ent1->client->saber[0].saberFlags & SFL_NOT_LOCKABLE
		|| ent2->client->saber[0].saberFlags & SFL_NOT_LOCKABLE)
	{
		return qfalse;
	}
	if (ent1->client->saber[1].model
		&& ent1->client->saber[1].model[0]
		&& !ent1->client->ps.saberHolstered
		&& ent1->client->saber[1].saberFlags & SFL_NOT_LOCKABLE)
	{
		return qfalse;
	}
	if (ent2->client->saber[1].model
		&& ent2->client->saber[1].model[0]
		&& !ent2->client->ps.saberHolstered
		&& ent2->client->saber[1].saberFlags & SFL_NOT_LOCKABLE)
	{
		return qfalse;
	}

	//don't allow saberlocks while a player is in a slow bounce.  This was allowing players to spam attack fakes/saberlocks
	//and never let their opponent get out of a slow bounce.
	if (PM_InSlowBounce(&ent1->client->ps) || PM_InSlowBounce(&ent2->client->ps))
	{
		return qfalse;
	}

	if (!in_front(ent1->client->ps.origin, ent2->client->ps.origin, ent2->client->ps.viewangles, 0.4f))
	{
		return qfalse;
	}
	if (!in_front(ent2->client->ps.origin, ent1->client->ps.origin, ent1->client->ps.viewangles, 0.4f))
	{
		return qfalse;
	}

	if (PM_SaberInParry(ent1->client->ps.saber_move))
	{
		//use the endquad of the move
		lock_quad = saber_moveData[ent1->client->ps.saber_move].endQuad;
	}
	else
	{
		//use the startquad of the move
		lock_quad = saber_moveData[ent1->client->ps.saber_move].startQuad;
	}

	switch (lock_quad)
	{
	case (qboolean)Q_BR:
		return WP_SabersCheckLock2(ent1, ent2, LOCK_DIAG_BR);
	case (qboolean)Q_R:
		return WP_SabersCheckLock2(ent1, ent2, LOCK_R);
	case (qboolean)Q_TR:
		return WP_SabersCheckLock2(ent1, ent2, LOCK_DIAG_TR);
	case (qboolean)Q_T:
		return WP_SabersCheckLock2(ent1, ent2, LOCK_TOP);
	case (qboolean)Q_TL:
		return WP_SabersCheckLock2(ent1, ent2, LOCK_DIAG_TL);
	case (qboolean)Q_L:
		return WP_SabersCheckLock2(ent1, ent2, LOCK_L);
	case (qboolean)Q_BL:
		return WP_SabersCheckLock2(ent1, ent2, LOCK_DIAG_BL);
	case (qboolean)Q_B:
		return WP_SabersCheckLock2(ent1, ent2, LOCK_TOP);
	default:
		//this shouldn't happen.  just wing it
		return qfalse;
	}
}

extern qboolean g_standard_humanoid(gentity_t* self);

void G_SaberBounce(const gentity_t* attacker, gentity_t* victim)
{
	if (victim->health <= 20)
	{
		return;
	}

	if (pm_saber_innonblockable_attack(attacker->client->ps.torsoAnim))
	{
		return;
	}

	if (attacker->client->ps.saberFatigueChainCount < MISHAPLEVEL_HUDFLASH)
	{
		return;
	}

	if (!g_standard_humanoid(victim))
	{
		return;
	}

	if (attacker->r.svFlags & SVF_BOT)
	{
		return;
	}

	if (attacker->client->ps.saberBlocked == BLOCKED_NONE)
	{
		if (!pm_saber_in_special_attack(attacker->client->ps.torsoAnim))
		{
			if (SaberAttacking(attacker))
			{
				// Saber is in attack, use bounce for this attack.
				attacker->client->ps.saber_move = PM_SaberBounceForAttack(attacker->client->ps.saber_move);
				attacker->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
			}
			else
			{
				// Saber is in defense, use defensive bounce.
				attacker->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			}
		}
	}
}

extern int pm_saber_deflection_for_quad(int quad);

extern stringID_table_t animTable[MAX_ANIMATIONS + 1];

int G_KnockawayForParry(const int move)
{
	switch (move)
	{
	case LS_REFLECT_ATTACK_FRONT:
	case LS_PARRY_FRONT:
	case LS_PARRY_WALK:
	case LS_PARRY_WALK_DUAL:
	case LS_PARRY_WALK_STAFF:
	case LS_PARRY_UP:
		return LS_K1_T_; //push up
	case LS_REFLECT_ATTACK_RIGHT:
	case LS_PARRY_UR:
	default:
		return LS_K1_TR; //push up, slightly to right
	case LS_REFLECT_ATTACK_LEFT:
	case LS_PARRY_UL:
		return LS_K1_TL; //push up and to left
	case LS_PARRY_LR:
		return LS_K1_BR; //push down and to left
	case LS_PARRY_LL:
		return LS_K1_BL; //push down and to right
	}
}

#define SABER_NO_DAMAGE 0
#define SABER_BLOCKING_DAMAGE 0.5
#define SABER_NONATTACK_DAMAGE 1
#define SABER_TRANSITION_DAMAGE 2

static float calc_trace_fraction(vec3_t start, vec3_t end, vec3_t endpos)
{
	const float fulldist = VectorDistance(start, end);
	const float dist = VectorDistance(start, endpos);

	if (fulldist > 0)
	{
		if (dist > 0)
		{
			return dist / fulldist;
		}
		return 0;
	}
	//I'm going to let it return 1 when the EndPos = End = Start
	return 1;
}

static QINLINE qboolean G_G2TraceCollide(trace_t* tr, vec3_t last_valid_start, vec3_t last_valid_end,
	vec3_t trace_mins,
	vec3_t trace_maxs)
{
	//Hit the ent with the normal trace, try the collision trace.
	G2Trace_t g2_trace;
	int t_n = 0;
	float f_radius = 0;

	if (!d_saberGhoul2Collision.integer)
	{
		return qfalse;
	}

	if (!g_entities[tr->entityNum].inuse)
	{
		//don't do perpoly on corpses.
		return qfalse;
	}

	if (trace_mins[0] ||
		trace_mins[1] ||
		trace_mins[2] ||
		trace_maxs[0] ||
		trace_maxs[1] ||
		trace_maxs[2])
	{
		f_radius = (trace_maxs[0] - trace_mins[0]) / 2.0f;
	}

	memset(&g2_trace, 0, sizeof g2_trace);

	while (t_n < MAX_G2_COLLISIONS)
	{
		g2_trace[t_n].mEntityNum = -1;
		t_n++;
	}
	gentity_t* g2_hit = &g_entities[tr->entityNum];

	if (g2_hit && g2_hit->inuse && g2_hit->ghoul2)
	{
		vec3_t angles;
		vec3_t g2_hit_origin;

		angles[ROLL] = angles[PITCH] = 0;

		if (g2_hit->client)
		{
			VectorCopy(g2_hit->client->ps.origin, g2_hit_origin);
			angles[YAW] = g2_hit->client->ps.viewangles[YAW];
		}
		else
		{
			VectorCopy(g2_hit->r.currentOrigin, g2_hit_origin);
			angles[YAW] = g2_hit->r.currentAngles[YAW];
		}

		if (com_optvehtrace.integer &&
			g2_hit->s.eType == ET_NPC &&
			g2_hit->s.NPC_class == CLASS_VEHICLE &&
			g2_hit->m_pVehicle)
		{
			trap->G2API_CollisionDetectCache(g2_trace, g2_hit->ghoul2, angles, g2_hit_origin, level.time,
				g2_hit->s.number,
				last_valid_start, last_valid_end, g2_hit->modelScale, 0,
				g_g2TraceLod.integer,
				f_radius);
		}
		else
		{
			trap->G2API_CollisionDetect(g2_trace, g2_hit->ghoul2, angles, g2_hit_origin, level.time, g2_hit->s.number,
				last_valid_start, last_valid_end, g2_hit->modelScale, 0, g_g2TraceLod.integer,
				f_radius);
		}

		if (g2_trace[0].mEntityNum != g2_hit->s.number)
		{
			tr->fraction = 1.0f;
			tr->entityNum = ENTITYNUM_NONE;
			tr->startsolid = 0;
			tr->allsolid = 0;
			return qfalse;
		}
		//The ghoul2 trace result matches, so copy the collision position into the trace endpos and send it back.
		VectorCopy(g2_trace[0].mCollisionPosition, tr->endpos);
		VectorCopy(g2_trace[0].mCollisionNormal, tr->plane.normal);
		tr->fraction = calc_trace_fraction(last_valid_start, last_valid_end, tr->endpos);

		if (g2_hit->client)
		{
			g2_hit->client->g2LastSurfaceHit = g2_trace[0].mSurfaceIndex;
			g2_hit->client->g2LastSurfaceTime = level.time;
			g2_hit->client->g2LastSurfaceModel = g2_trace[0].mModelindex;
		}
		return qtrue;
	}

	return qfalse;
}

qboolean saberCheckKnockdown_Thrown(gentity_t* saberent, gentity_t* saberOwner, const gentity_t* other);
qboolean saberCheckKnockdown_Smashed(gentity_t* saberent, gentity_t* saberOwner, const gentity_t* other, int damage);
qboolean saberCheckKnockdown_BrokenParry(gentity_t* saberent, gentity_t* saberOwner, gentity_t* other);

typedef struct saber_face_s
{
	vec3_t v1;
	vec3_t v2;
	vec3_t v3;
} saber_face_t;

//build faces around blade for collision checking -rww
static QINLINE void g_build_saber_faces(vec3_t base, vec3_t tip, const float radius, vec3_t fwd, vec3_t right,
	int* f_num,
	saber_face_t** f_list)
{
	static saber_face_t faces[12];
	int i = 0;
	const float* d1 = NULL, * d2 = NULL;
	vec3_t inv_fwd;
	vec3_t inv_right;

	VectorCopy(fwd, inv_fwd);
	VectorInverse(inv_fwd);
	VectorCopy(right, inv_right);
	VectorInverse(inv_right);

	while (i < 8)
	{
		//yeah, this part is kind of a hack, but eh
		if (i < 2)
		{
			//"left" surface
			d1 = &fwd[0];
			d2 = &inv_right[0];
		}
		else if (i < 4)
		{
			//"right" surface
			d1 = &fwd[0];
			d2 = &right[0];
		}
		else if (i < 6)
		{
			//"front" surface
			d1 = &right[0];
			d2 = &fwd[0];
		}
		else if (i < 8)
		{
			//"back" surface
			d1 = &right[0];
			d2 = &inv_fwd[0];
		}

		//first triangle for this surface
		VectorMA(base, radius / 3.0f, d1, faces[i].v1);
		VectorMA(faces[i].v1, radius / 3.0f, d2, faces[i].v1);

		VectorMA(tip, radius / 3.0f, d1, faces[i].v2);
		VectorMA(faces[i].v2, radius / 3.0f, d2, faces[i].v2);

		VectorMA(tip, -radius / 3.0f, d1, faces[i].v3);
		VectorMA(faces[i].v3, radius / 3.0f, d2, faces[i].v3);

		i++;

		//second triangle for this surface
		VectorMA(tip, -radius / 3.0f, d1, faces[i].v1);
		VectorMA(faces[i].v1, radius / 3.0f, d2, faces[i].v1);

		VectorMA(base, radius / 3.0f, d1, faces[i].v2);
		VectorMA(faces[i].v2, radius / 3.0f, d2, faces[i].v2);

		VectorMA(base, -radius / 3.0f, d1, faces[i].v3);
		VectorMA(faces[i].v3, radius / 3.0f, d2, faces[i].v3);

		i++;
	}

	//top surface
	//face 1
	VectorMA(tip, radius / 3.0f, fwd, faces[i].v1);
	VectorMA(faces[i].v1, -radius / 3.0f, right, faces[i].v1);

	VectorMA(tip, radius / 3.0f, fwd, faces[i].v2);
	VectorMA(faces[i].v2, radius / 3.0f, right, faces[i].v2);

	VectorMA(tip, -radius / 3.0f, fwd, faces[i].v3);
	VectorMA(faces[i].v3, -radius / 3.0f, right, faces[i].v3);

	i++;

	//face 2
	VectorMA(tip, radius / 3.0f, fwd, faces[i].v1);
	VectorMA(faces[i].v1, radius / 3.0f, right, faces[i].v1);

	VectorMA(tip, -radius / 3.0f, fwd, faces[i].v2);
	VectorMA(faces[i].v2, -radius / 3.0f, right, faces[i].v2);

	VectorMA(tip, -radius / 3.0f, fwd, faces[i].v3);
	VectorMA(faces[i].v3, radius / 3.0f, right, faces[i].v3);

	i++;

	//bottom surface
	//face 1
	VectorMA(base, radius / 3.0f, fwd, faces[i].v1);
	VectorMA(faces[i].v1, -radius / 3.0f, right, faces[i].v1);

	VectorMA(base, radius / 3.0f, fwd, faces[i].v2);
	VectorMA(faces[i].v2, radius / 3.0f, right, faces[i].v2);

	VectorMA(base, -radius / 3.0f, fwd, faces[i].v3);
	VectorMA(faces[i].v3, -radius / 3.0f, right, faces[i].v3);

	i++;

	//face 2
	VectorMA(base, radius / 3.0f, fwd, faces[i].v1);
	VectorMA(faces[i].v1, radius / 3.0f, right, faces[i].v1);

	VectorMA(base, -radius / 3.0f, fwd, faces[i].v2);
	VectorMA(faces[i].v2, -radius / 3.0f, right, faces[i].v2);

	VectorMA(base, -radius / 3.0f, fwd, faces[i].v3);
	VectorMA(faces[i].v3, radius / 3.0f, right, faces[i].v3);

	i++;

	//yeah.. always going to be 12 I suppose.
	*f_num = i;
	*f_list = &faces[0];
}

//collision utility function -rww
static QINLINE void g_sab_col_calc_plane_eq(vec3_t x, vec3_t y, vec3_t z, float* plane_eq)
{
	plane_eq[0] = x[1] * (y[2] - z[2]) + y[1] * (z[2] - x[2]) + z[1] * (x[2] - y[2]);
	plane_eq[1] = x[2] * (y[0] - z[0]) + y[2] * (z[0] - x[0]) + z[2] * (x[0] - y[0]);
	plane_eq[2] = x[0] * (y[1] - z[1]) + y[0] * (z[1] - x[1]) + z[0] * (x[1] - y[1]);
	plane_eq[3] = -(x[0] * (y[1] * z[2] - z[1] * y[2]) + y[0] * (z[1] * x[2] - x[1] * z[2]) + z[0] * (x[1] * y[2] - y[1]
		* x[2]));
}

//collision utility function -rww
static QINLINE int g_sab_col_point_relative_to_plane(vec3_t pos, float* side, const float* plane_eq)
{
	*side = plane_eq[0] * pos[0] + plane_eq[1] * pos[1] + plane_eq[2] * pos[2] + plane_eq[3];

	if (*side > 0.0f)
	{
		return 1;
	}
	if (*side < 0.0f)
	{
		return -1;
	}

	return 0;
}

//do actual collision check using generated saber "faces"
static QINLINE qboolean g_saber_face_collision_check(const int f_num, saber_face_t* f_list, vec3_t atk_start,
	vec3_t atk_end,
	vec3_t atk_mins, vec3_t atk_maxs, vec3_t impact_point)
{
	static float side, side2, dist;
	static vec3_t dir;
	int i = 0;

	if (VectorCompare(atk_mins, vec3_origin) && VectorCompare(atk_maxs, vec3_origin))
	{
		VectorSet(atk_mins, -1.0f, -1.0f, -1.0f);
		VectorSet(atk_maxs, 1.0f, 1.0f, 1.0f);
	}

	VectorSubtract(atk_end, atk_start, dir);

	while (i < f_num)
	{
		static float plane_eq[4];
		g_sab_col_calc_plane_eq(f_list->v1, f_list->v2, f_list->v3, plane_eq);

		if (g_sab_col_point_relative_to_plane(atk_start, &side, plane_eq) !=
			g_sab_col_point_relative_to_plane(atk_end, &side2, plane_eq))
		{
			static vec3_t point;
			//start/end points intersect with the plane
			static vec3_t extruded;
			static vec3_t min_point, max_point;
			static vec3_t plane_normal;
			static int facing;

			VectorCopy(&plane_eq[0], plane_normal);
			side2 = plane_normal[0] * dir[0] + plane_normal[1] * dir[1] + plane_normal[2] * dir[2];

			dist = side / side2;
			VectorMA(atk_start, -dist, dir, point);

			VectorAdd(point, atk_mins, min_point);
			VectorAdd(point, atk_maxs, max_point);

			//point is now the point at which we intersect on the plane.
			//see if that point is within the edges of the face.
			VectorMA(f_list->v1, -2.0f, plane_normal, extruded);
			g_sab_col_calc_plane_eq(f_list->v1, f_list->v2, extruded, plane_eq);
			facing = g_sab_col_point_relative_to_plane(point, &side, plane_eq);

			if (facing < 0)
			{
				//not intersecting.. let's try with the mins/maxs and see if they intersect on the edge plane
				facing = g_sab_col_point_relative_to_plane(min_point, &side, plane_eq);
				if (facing < 0)
				{
					facing = g_sab_col_point_relative_to_plane(max_point, &side, plane_eq);
				}
			}

			if (facing >= 0)
			{
				//first edge is facing...
				VectorMA(f_list->v2, -2.0f, plane_normal, extruded);
				g_sab_col_calc_plane_eq(f_list->v2, f_list->v3, extruded, plane_eq);
				facing = g_sab_col_point_relative_to_plane(point, &side, plane_eq);

				if (facing < 0)
				{
					//not intersecting.. let's try with the mins/maxs and see if they intersect on the edge plane
					facing = g_sab_col_point_relative_to_plane(min_point, &side, plane_eq);
					if (facing < 0)
					{
						facing = g_sab_col_point_relative_to_plane(max_point, &side, plane_eq);
					}
				}

				if (facing >= 0)
				{
					//second edge is facing...
					VectorMA(f_list->v3, -2.0f, plane_normal, extruded);
					g_sab_col_calc_plane_eq(f_list->v3, f_list->v1, extruded, plane_eq);
					facing = g_sab_col_point_relative_to_plane(point, &side, plane_eq);

					if (facing < 0)
					{
						//not intersecting.. let's try with the mins/maxs and see if they intersect on the edge plane
						facing = g_sab_col_point_relative_to_plane(min_point, &side, plane_eq);
						if (facing < 0)
						{
							facing = g_sab_col_point_relative_to_plane(max_point, &side, plane_eq);
						}
					}

					if (facing >= 0)
					{
						//third edge is facing.. success
						VectorCopy(point, impact_point);
						return qtrue;
					}
				}
			}
		}

		i++;
		f_list++;
	}

	//did not hit anything
	return qfalse;
}

//Copies all the important data from one trace_t to another.  Please note that this doesn't transfer ALL
//of the trace_t data.
static void trace_copy(const trace_t* a, trace_t* b)
{
	b->allsolid = a->allsolid;
	b->contents = a->contents;
	VectorCopy(a->endpos, b->endpos);
	b->entityNum = a->entityNum;
	b->fraction = a->fraction;
	//This is the only thing that's ever really used from the plane data.
	VectorCopy(a->plane.normal, b->plane.normal);
	b->startsolid = a->startsolid;
	b->surfaceFlags = a->surfaceFlags;
}

//Reset the trace to be "blank".
static QINLINE void trace_clear(trace_t* tr, vec3_t end)
{
	tr->fraction = 1;
	VectorCopy(end, tr->endpos);
	tr->entityNum = ENTITYNUM_NONE;
}

//check for collision of 2 blades -rww
qboolean WP_SaberIsOff(const gentity_t* self, int saberNum);
qboolean WP_BladeIsOff(const gentity_t* self, int saberNum, int blade_num);

static QINLINE qboolean g_saber_collide(gentity_t* atk, const gentity_t* def, vec3_t atk_start, vec3_t atk_end,
	vec3_t atk_mins,
	vec3_t atk_maxs, trace_t* tr)
{
	static int i, j;

	if (!def->inuse || !def->client)
	{
		//must have 2 clients and a valid saber entity
		trace_clear(tr, atk_end);
		return qfalse;
	}

	if (def->client->ps.saberHolstered == 2)
	{
		//no sabers on.
		trace_clear(tr, atk_end);
		return qfalse;
	}

	i = 0;
	while (i < MAX_SABERS)
	{
		j = 0;

		if (WP_SaberIsOff(def, i))
		{
			//saber is off and can't be used.
			i++;
			continue;
		}

		if (def->client->saber[i].model && def->client->saber[i].model[0])
		{
			int f_num;
			saber_face_t* f_list;

			//go through each blade on the defender's sabers
			while (j < def->client->saber[i].numBlades)
			{
				const bladeInfo_t* blade = &def->client->saber[i].blade[j];

				if (WP_BladeIsOff(def, i, j))
				{
					//this particular blade is turned off.
					j++;
					continue;
				}

				if (level.time - blade->storageTime < 200)
				{
					vec3_t tip;
					vec3_t base;
					vec3_t right;
					vec3_t fwd;
					vec3_t v;
					//recently updated
					//first get base and tip of blade
					VectorCopy(blade->muzzlePoint, base);
					VectorMA(base, blade->lengthMax, blade->muzzleDir, tip);

					//Now get relative angles between the points
					VectorSubtract(tip, base, v);
					vectoangles(v, v);
					AngleVectors(v, NULL, right, fwd);

					//now build collision faces for this blade
					g_build_saber_faces(base, tip, blade->radius * 4.0f, fwd, right, &f_num, &f_list);
					if (f_num > 0)
					{
#if 0
						if (atk->inuse && atk->client && atk->s.number == 0)
						{
							int x = 0;
							saberFace_t* l = fList;
							while (x < fNum)
							{
								G_TestLine(fList->v1, fList->v2, 0x0000ff, 100);
								G_TestLine(fList->v2, fList->v3, 0x0000ff, 100);
								G_TestLine(fList->v3, fList->v1, 0x0000ff, 100);

								fList++;
								x++;
							}
							fList = l;
						}
#endif

						if (g_saber_face_collision_check(f_num, f_list, atk_start, atk_end, atk_mins, atk_maxs,
							tr->endpos))
						{
							//collided
							//determine the plane of impact for the viewlocking stuff.
							vec3_t result;

							tr->fraction = calc_trace_fraction(atk_start, atk_end, tr->endpos);

							G_FindClosestPointOnLineSegment(base, tip, tr->endpos, result);
							VectorSubtract(tr->endpos, result, result);
							VectorCopy(result, tr->plane.normal);
							if (atk && atk->client)
							{
								atk->client->lastSaberCollided = i;
								atk->client->lastBladeCollided = j;
							}
							return qtrue;
						}
					}
				}
				j++;
			}
		}
		i++;
	}

	trace_clear(tr, atk_end);
	return qfalse;
}

int BasicWeaponBlockCosts[MOD_MAX] =
{
	-1, //MOD_UNKNOWN,
	-1, //MOD_STUN_BATON,
	-1, //MOD_MELEE,
	20, //MOD_SABER,
	20, //MOD_BRYAR_PISTOL,
	10, //MOD_BRYAR_PISTOL_ALT,
	20, //MOD_BLASTER,
	20, //MOD_TURBLAST,
	20, //MOD_DISRUPTOR,
	20, //MOD_DISRUPTOR_SPLASH,
	20, //MOD_DISRUPTOR_SNIPER,
	20, //MOD_BOWCASTER,
	20, //MOD_REPEATER,
	20, //MOD_REPEATER_ALT,
	20, //MOD_REPEATER_ALT_SPLASH,
	20, //MOD_DEMP2,
	20, //MOD_DEMP2_ALT,
	20, //MOD_FLECHETTE,
	20, //MOD_FLECHETTE_ALT_SPLASH,
	20, //MOD_ROCKET,
	20, //MOD_ROCKET_SPLASH,
	20, //MOD_ROCKET_HOMING,
	20, //MOD_ROCKET_HOMING_SPLASH,
	20, //MOD_THERMAL,
	20, //MOD_THERMAL_SPLASH,
	20, //MOD_TRIP_MINE_SPLASH,
	20, //MOD_TIMED_MINE_SPLASH,
	20, //MOD_DET_PACK_SPLASH,
	-1, //MOD_VEHICLE,
	20, //MOD_CONC,
	20, //MOD_CONC_ALT,
	-1, //MOD_FORCE_DARK,
	20, //MOD_SENTRY,
	-1, //MOD_WATER,
	-1, //MOD_SLIME,
	-1, //MOD_LAVA,
	-1, //MOD_CRUSH,
	-1, //MOD_TELEFRAG,
	-1, //MOD_FALLING,
	-1, //MOD_SUICIDE,
	-1, //MOD_TARGET_LASER,
	-1, //MOD_TRIGGER_HURT,
	-1, //MOD_TEAM_CHANGE,
	-1, //MOD_COLLISION,
	-1, //MOD_VEH_EXPLOSION,
	20, //MOD_SEEKER,
	20, //MOD_ELECTROCUTE,
	-1, //MOD_SPECTATE,
	20, //MOD_HEADSHOT,
	20, //MOD_BODYSHOT,
	20, //MOD_FORCE_LIGHTNING,
	-1, //MOD_BURNING,
	-1, //MOD_GAS,

	//MOD_MAX
};

static int BasicSaberBlockCost(const int attackerStyle)
{
	//returns the basic saber block cost of blocking an attack from the given saber style.
	switch (attackerStyle)
	{
	case SS_DUAL:
		return 13;
	case SS_STAFF:
		return 13;
	case SS_TAVION:
		return 14;
	case SS_FAST:
		return 12;
	case SS_MEDIUM:
		return 15;
	case SS_DESANN:
		return 16;
	case SS_STRONG:
		return 17;
	default:
		return 0;
	}
}

static qboolean IsMoving(const gentity_t* ent)
{
	if (!ent || !ent->client)
		return qfalse;

	if (ent->client->pers.cmd.upmove == 0 && ent->client->pers.cmd.forwardmove == 0 &&
		ent->client->pers.cmd.rightmove == 0)
		return qfalse;

	return qtrue;
}

int WP_SaberBlockCost(gentity_t* defender, const gentity_t* attacker, vec3_t hit_loc)
{
	//returns the cost to block this attack for this attacker/defender combo.
	float saber_block_cost;
	//===========================
	// Determine Base Block Cost
	//===========================

	if (!attacker //don't have attacker
		|| !attacker->client //attacker isn't a NPC/player
		|| attacker->client->ps.weapon != WP_SABER) //or the player that is attacking isn't using a saber
	{
		//standard bolt block!
		saber_block_cost = DODGE_BOLTBLOCK;

		if (attacker && attacker->client && attacker->activator && attacker->activator->s.weapon == WP_BRYAR_PISTOL)
		{
			saber_block_cost = 4;
		}
		if (attacker->client && attacker->client->ps.weapon != WP_FLECHETTE)
		{
			saber_block_cost += 2;
		}

		if (defender->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
		{
			saber_block_cost += 10; //Using force speed
		}
		if (PM_RunningAnim(defender->client->ps.legsAnim))
		{
			saber_block_cost += 5; //Running
		}
		if (walk_check(defender) && IsMoving(defender))
		{
			saber_block_cost++; //Walking
		}
		if (defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			saber_block_cost += 10;
		}

		if (attacker->activator && attacker->activator->s.weapon == WP_BRYAR_PISTOL
			|| attacker->activator && attacker->activator->s.weapon == WP_BRYAR_OLD
			|| attacker->activator && attacker->activator->s.weapon == WP_REPEATER
			|| attacker->activator && attacker->activator->s.weapon == WP_BOWCASTER
			|| attacker->activator && attacker->activator->s.weapon == WP_DISRUPTOR
			|| attacker->activator && attacker->activator->s.weapon == WP_EMPLACED_GUN
			|| attacker->activator && attacker->activator->s.weapon == WP_FLECHETTE)
		{
			if (attacker->activator->s.weapon == WP_FLECHETTE)
			{
				const float distance = VectorBlockDistance(attacker->activator->r.currentOrigin,
					defender->r.currentOrigin);

				if (walk_check(defender))
				{
					saber_block_cost = 2;
				}
				else
				{
					saber_block_cost = 4;
				}

				if (defender->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] >= FORCE_LEVEL_3 && defender->client->ps.
					fd.saberAnimLevel != SS_MEDIUM)
				{
					saber_block_cost++;
				}

				if (distance >= 200.0f)
				{
					saber_block_cost += 1.5;
				}
				saber_block_cost += 1;
			}
			else if (attacker->activator->s.weapon != WP_BOWCASTER)
			{
				const float distance = VectorBlockDistance(attacker->activator->r.currentOrigin,
					defender->r.currentOrigin);

				if (distance <= 125.0f)
				{
					saber_block_cost = DODGE_BOLTBLOCK * 3;
				}
				else if (distance <= 300.0f)
				{
					saber_block_cost = DODGE_BOLTBLOCK * 2;
				}
				else
				{
					saber_block_cost = DODGE_BOLTBLOCK;
				}

				if (attacker->activator->s.weapon == WP_REPEATER)
				{
					saber_block_cost = DODGE_REPEATERBLOCK;
				}
			}
			else if (attacker->activator->s.weapon == WP_DISRUPTOR)
			{
				saber_block_cost = 10;
			}
			else if (attacker->activator->s.weapon == WP_DEMP2)
			{
				saber_block_cost = DODGE_TUSKENBLOCK * 3;
			}
			else
			{
				saber_block_cost = 5;
			}

			if (PM_SaberInAttack(defender->client->ps.saber_move) || PM_SaberInStart(defender->client->ps.saber_move))
			{
				if (attacker->activator->s.weapon == WP_FLECHETTE)
				{
					saber_block_cost = saber_block_cost * 1.5;
				}
				else
				{
					saber_block_cost = saber_block_cost * 2;
				}
			}

			if (attacker->activator->s.weapon == WP_BOWCASTER)
			{
				saber_block_cost = 3;
			}

			if (defender->client->ps.fd.saberAnimLevel == SS_FAST)
			{
				saber_block_cost--;
			}
		}
		else if (attacker->activator && attacker->activator->s.weapon == WP_BLASTER)
		{
			saber_block_cost = 2;
		}
		else
		{
			saber_block_cost = DODGE_BOLTBLOCK;
		}
	}
	else if (attacker->client->ps.saber_move == LS_A_LUNGE
		|| attacker->client->ps.saber_move == LS_SPINATTACK
		|| attacker->client->ps.saber_move == LS_SPINATTACK_DUAL
		|| attacker->client->ps.saber_move == LS_SPINATTACK_GRIEV
		|| attacker->client->ps.saber_move == LS_GRIEVOUS_SPECIAL)
	{
		//lunge attacks
		saber_block_cost = .75 * BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else if (attacker->client->ps.saber_move == LS_ROLL_STAB)
	{
		//roll stab
		saber_block_cost = 2 * BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else if (attacker->client->ps.saber_move == LS_A_JUMP_T__B_)
	{
		//DFA moves
		saber_block_cost = 4 * BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else if (attacker->client->ps.saber_move == LS_A_JUMP_PALP_)
	{
		//DFA moves
		saber_block_cost = 4 * BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else if (attacker->client->ps.saber_move == LS_A_FLIP_STAB
		|| attacker->client->ps.saber_move == LS_A_FLIP_SLASH)
	{
		//flip stabs do more DP
		saber_block_cost = 2 * BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else
	{
		//"normal" swing moves
		if (attacker->client->ps.userInt3 & 1 << FLAG_ATTACKFAKE)
		{
			//attacker is in an attack fake
			if (attacker->client->ps.fd.saberAnimLevel == SS_STRONG
				&& !g_accurate_blocking(defender, attacker, hit_loc))
			{
				//Red does additional DP damage with attack fakes if they aren't parried.
				saber_block_cost = BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel) * 1.35;
			}
			else
			{
				saber_block_cost = BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel) * 1.25;
			}
		}
		else
		{
			//normal saber block
			saber_block_cost = BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
		}

		//add running damage bonus to normal swings but don't apply if the defender is slowbouncing
		if (!walk_check(attacker)
			&& !(defender->client->ps.userInt3 & 1 << FLAG_SLOWBOUNCE)
			&& !(defender->client->ps.userInt3 & 1 << FLAG_OLDSLOWBOUNCE))
		{
			if (attacker->client->ps.fd.saberAnimLevel == SS_DUAL)
			{
				saber_block_cost *= 3.0;
			}
			else
			{
				saber_block_cost *= 1.5;
			}
		}
	}

	//======================
	// Block Cost Modifiers
	//======================

	if (attacker && attacker->client)
	{
		//attacker is a player so he must have just hit you with a saber blow.
		if (g_accurate_blocking(defender, attacker, hit_loc))
		{
			//parried this attack, cost is less
			if (defender->client->ps.fd.saberAnimLevel == SS_FAST)
			{
				//blue parries cheaper
				saber_block_cost = saber_block_cost / 3.25;
			}
			else
			{
				saber_block_cost = saber_block_cost / 3;
			}
		}

		if (!in_front(attacker->client->ps.origin, defender->client->ps.origin, defender->client->ps.viewangles, -0.7f))
		{
			//player is behind us, costs more to block
			//staffs back block at normal cost.
			if (defender->client->ps.fd.saberAnimLevel == SS_STAFF && defender->client->ps.fd.forcePowerLevel[
				FP_SABER_DEFENSE] == FORCE_LEVEL_3)
			{
				// Having both staff and defense 3 allow no extra back hit damage
				saber_block_cost *= 1;
			}
			else if (defender->client->ps.fd.saberAnimLevel != SS_STAFF && defender->client->ps.fd.forcePowerLevel[
				FP_SABER_DEFENSE] == FORCE_LEVEL_2)
			{
				//level 2 defense lowers back damage more
				saber_block_cost *= 1.50;
			}
			else if (defender->client->ps.fd.saberAnimLevel != SS_STAFF && defender->client->ps.fd.forcePowerLevel[
				FP_SABER_DEFENSE] == FORCE_LEVEL_1)
			{
				//level 1 defense lowers back damage a bit
				saber_block_cost *= 1.75;
			}
			else
			{
				saber_block_cost *= 2;
			}
		}

		//clamp to body dodge cost since it wouldn't be fair to cost more than that.
		if (saber_block_cost > BasicWeaponBlockCosts[MOD_SABER])
		{
			saber_block_cost = BasicWeaponBlockCosts[MOD_SABER];
		}
	}
	if (PM_SaberInBrokenParry(defender->client->ps.saber_move))
	{
		//we're stunned/stumbling, increase DP cost
		saber_block_cost *= 1.5;
	}

	if (PM_KickingAnim(defender->client->ps.legsAnim))
	{
		//kicking
		saber_block_cost *= 1.5;
	}

	if (!walk_check(defender))
	{
		if (defender->NPC)
		{
			saber_block_cost *= 1.0;
		}
		else
		{
			saber_block_cost *= 2.5;
		}
	}
	if (defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//in mid-air
		if (defender->client->ps.fd.saberAnimLevel == SS_DUAL) //Ataru's other perk much less cost for air hit
		{
			saber_block_cost *= .5;
		}
		else
		{
			saber_block_cost *= 2;
		}
	}
	if (defender->client->ps.ManualblockStartTime > level.time)
	{
		//attempting to block something too soon after a saber bolt block
		saber_block_cost *= 2;
	}
	return (int)saber_block_cost;
}

int WP_SaberBoltBlockCost(gentity_t* defender, const gentity_t* attacker)
{
	//returns the cost to block this attack for this attacker/defender combo.
	float saber_block_cost;
	//===========================
	// Determine Base Block Cost
	//===========================

	if (!attacker //don't have attacker
		|| !attacker->client //attacker isn't a NPC/player
		|| attacker->client->ps.weapon != WP_SABER) //or the player that is attacking isn't using a saber
	{
		//standard bolt block!
		saber_block_cost = DODGE_BOLTBLOCK;

		if (attacker && attacker->client && attacker->activator && attacker->activator->s.weapon == WP_BRYAR_PISTOL)
		{
			saber_block_cost = 4;
		}
		if (attacker->client && attacker->client->ps.weapon != WP_FLECHETTE)
		{
			saber_block_cost += 2;
		}

		if (defender->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
		{
			saber_block_cost += 10; //Using force speed
		}
		if (PM_RunningAnim(defender->client->ps.legsAnim))
		{
			saber_block_cost += 5; //Running
		}
		if (walk_check(defender) && IsMoving(defender))
		{
			saber_block_cost++; //Walking
		}
		if (defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			saber_block_cost += 10;
		}

		if (attacker->activator && attacker->activator->s.weapon == WP_BRYAR_PISTOL
			|| attacker->activator && attacker->activator->s.weapon == WP_BRYAR_OLD
			|| attacker->activator && attacker->activator->s.weapon == WP_REPEATER
			|| attacker->activator && attacker->activator->s.weapon == WP_BOWCASTER
			|| attacker->activator && attacker->activator->s.weapon == WP_DISRUPTOR
			|| attacker->activator && attacker->activator->s.weapon == WP_EMPLACED_GUN
			|| attacker->activator && attacker->activator->s.weapon == WP_FLECHETTE)
		{
			if (attacker->activator->s.weapon == WP_FLECHETTE)
			{
				const float distance = VectorBlockDistance(attacker->activator->r.currentOrigin,
					defender->r.currentOrigin);

				if (walk_check(defender))
				{
					saber_block_cost = 2;
				}
				else
				{
					saber_block_cost = 4;
				}

				if (defender->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] >= FORCE_LEVEL_3 && defender->client->ps.
					fd.saberAnimLevel != SS_MEDIUM)
				{
					saber_block_cost++;
				}

				if (distance >= 200.0f)
				{
					saber_block_cost += 1.5;
				}
				saber_block_cost += 1;
			}
			else if (attacker->activator->s.weapon != WP_BOWCASTER)
			{
				const float distance = VectorBlockDistance(attacker->activator->r.currentOrigin,
					defender->r.currentOrigin);

				if (distance <= 125.0f)
				{
					saber_block_cost = DODGE_BOLTBLOCK * 3;
				}
				else if (distance <= 300.0f)
				{
					saber_block_cost = DODGE_BOLTBLOCK * 2;
				}
				else
				{
					saber_block_cost = DODGE_BOLTBLOCK;
				}

				if (attacker->activator->s.weapon == WP_REPEATER)
				{
					saber_block_cost = DODGE_REPEATERBLOCK;
				}
			}
			else if (attacker->activator->s.weapon == WP_DISRUPTOR)
			{
				saber_block_cost = 10;
			}
			else if (attacker->activator->s.weapon == WP_DEMP2)
			{
				saber_block_cost = DODGE_TUSKENBLOCK * 3;
			}
			else
			{
				saber_block_cost = 5;
			}

			if (PM_SaberInAttack(defender->client->ps.saber_move) || PM_SaberInStart(defender->client->ps.saber_move))
			{
				if (attacker->activator->s.weapon == WP_FLECHETTE)
				{
					saber_block_cost = saber_block_cost * 1.5;
				}
				else
				{
					saber_block_cost = saber_block_cost * 2;
				}
			}

			if (attacker->activator->s.weapon == WP_BOWCASTER)
			{
				saber_block_cost = 3;
			}

			if (defender->client->ps.fd.saberAnimLevel == SS_FAST)
			{
				saber_block_cost--;
			}
		}
		else if (attacker->activator && attacker->activator->s.weapon == WP_BLASTER)
		{
			saber_block_cost = 2;
		}
		else
		{
			saber_block_cost = DODGE_BOLTBLOCK;
		}
	}
	else if (attacker->client->ps.saber_move == LS_A_LUNGE
		|| attacker->client->ps.saber_move == LS_SPINATTACK
		|| attacker->client->ps.saber_move == LS_SPINATTACK_DUAL
		|| attacker->client->ps.saber_move == LS_SPINATTACK_GRIEV
		|| attacker->client->ps.saber_move == LS_GRIEVOUS_SPECIAL)
	{
		//lunge attacks
		saber_block_cost = .75 * BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else if (attacker->client->ps.saber_move == LS_ROLL_STAB)
	{
		//roll stab
		saber_block_cost = 2 * BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else if (attacker->client->ps.saber_move == LS_A_JUMP_T__B_)
	{
		//DFA moves
		saber_block_cost = 4 * BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else if (attacker->client->ps.saber_move == LS_A_JUMP_PALP_)
	{
		//DFA moves
		saber_block_cost = 4 * BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else if (attacker->client->ps.saber_move == LS_A_FLIP_STAB
		|| attacker->client->ps.saber_move == LS_A_FLIP_SLASH)
	{
		//flip stabs do more DP
		saber_block_cost = 2 * BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else
	{
		//"normal" swing moves
		if (attacker->client->ps.userInt3 & 1 << FLAG_ATTACKFAKE)
		{
			//attacker is in an attack fake
			saber_block_cost = BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel) * 1.25;
		}
		else
		{
			//normal saber block
			saber_block_cost = BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
		}

		//add running damage bonus to normal swings but don't apply if the defender is slowbouncing
		if (!walk_check(attacker)
			&& !(defender->client->ps.userInt3 & 1 << FLAG_SLOWBOUNCE)
			&& !(defender->client->ps.userInt3 & 1 << FLAG_OLDSLOWBOUNCE))
		{
			if (attacker->client->ps.fd.saberAnimLevel == SS_DUAL)
			{
				saber_block_cost *= 3.0;
			}
			else
			{
				saber_block_cost *= 1.5;
			}
		}
	}

	//======================
	// Block Cost Modifiers
	//======================

	if (attacker && attacker->client)
	{
		//attacker is a player so he must have just hit you with a saber blow.
		if (!in_front(attacker->client->ps.origin, defender->client->ps.origin, defender->client->ps.viewangles, -0.7f))
		{
			//player is behind us, costs more to block
			//staffs back block at normal cost.
			if (defender->client->ps.fd.saberAnimLevel == SS_STAFF && defender->client->ps.fd.forcePowerLevel[
				FP_SABER_DEFENSE] == FORCE_LEVEL_3)
			{
				// Having both staff and defense 3 allow no extra back hit damage
				saber_block_cost *= 1;
			}
			else if (defender->client->ps.fd.saberAnimLevel != SS_STAFF && defender->client->ps.fd.forcePowerLevel[
				FP_SABER_DEFENSE] == FORCE_LEVEL_2)
			{
				//level 2 defense lowers back damage more
				saber_block_cost *= 1.50;
			}
			else if (defender->client->ps.fd.saberAnimLevel != SS_STAFF && defender->client->ps.fd.forcePowerLevel[
				FP_SABER_DEFENSE] == FORCE_LEVEL_1)
			{
				//level 1 defense lowers back damage a bit
				saber_block_cost *= 1.75;
			}
			else
			{
				saber_block_cost *= 2;
			}
		}

		//clamp to body dodge cost since it wouldn't be fair to cost more than that.
		if (saber_block_cost > BasicWeaponBlockCosts[MOD_SABER])
		{
			saber_block_cost = BasicWeaponBlockCosts[MOD_SABER];
		}
	}
	if (PM_SaberInBrokenParry(defender->client->ps.saber_move))
	{
		//we're stunned/stumbling, increase DP cost
		saber_block_cost *= 1.5;
	}

	if (PM_KickingAnim(defender->client->ps.legsAnim))
	{
		//kicking
		saber_block_cost *= 1.5;
	}

	if (!walk_check(defender))
	{
		if (defender->NPC)
		{
			saber_block_cost *= 1.0;
		}
		else
		{
			saber_block_cost *= 2.5;
		}
	}
	if (defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//in mid-air
		if (defender->client->ps.fd.saberAnimLevel == SS_DUAL) //Ataru's other perk much less cost for air hit
		{
			saber_block_cost *= .5;
		}
		else
		{
			saber_block_cost *= 2;
		}
	}
	if (defender->client->ps.ManualblockStartTime > level.time)
	{
		//attempting to block something too soon after a saber bolt block
		saber_block_cost *= 2;
	}
	return (int)saber_block_cost;
}

qboolean wp_using_dual_saber_as_primary(const playerState_t* ps);

int wp_saber_must_block(gentity_t* self, const gentity_t* atk, const qboolean check_b_box_block, vec3_t point,
	const int rSaberNum, const int rBladeNum)
{
	if (!self || !self->client || !atk)
	{
		return 0;
	}

	if (atk && atk->s.eType == ET_MISSILE &&
		(atk->s.weapon == WP_ROCKET_LAUNCHER ||
			atk->s.weapon == WP_THERMAL ||
			atk->s.weapon == WP_TRIP_MINE ||
			atk->s.weapon == WP_DET_PACK ||
			atk->methodOfDeath == MOD_REPEATER_ALT ||
			atk->methodOfDeath == MOD_CONC ||
			atk->methodOfDeath == MOD_CONC_ALT ||
			atk->methodOfDeath == MOD_FLECHETTE_ALT_SPLASH))
	{
		//can't block this stuff with a saber
		return 0;
	}

	if (self && self->client && self->client->MassiveBounceAnimTime > level.time)
	{
		return 0;
	}

	if (!(self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
	{
		if (self->r.svFlags & SVF_BOT)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			if (self->client->ps.weapon == WP_SABER
				&& !BG_SabersOff(&self->client->ps)
				&& !self->client->ps.saberInFlight)
			{
				return 1;
			}
			return 0;
		}
		return 0;
	}

	if (PM_InGrappleMove(self->client->ps.torsoAnim))
	{
		//you can't block while doing a melee move.
		return 0;
	}

	if (PM_SaberInBrokenParry(self->client->ps.saber_move))
	{
		//you've been stunned from a broken parry
		return 0;
	}

	if (PM_kick_move(self->client->ps.saber_move))
	{
		return 0;
	}

	if (BG_FullBodyTauntAnim(self->client->ps.torsoAnim))
	{
		return 0;
	}

	if (self->client->ps.weapon != WP_SABER
		|| self->client->ps.weapon == WP_NONE
		|| self->client->ps.weapon == WP_MELEE) //saber not here
	{
		return 0;
	}

	if (self->client->ps.weapon == WP_SABER && self->client->ps.saberInFlight) //saber not here
	{
		//our saber is currently dropped or in flight.
		return 0;
	}

	if (!self->client->ps.saberEntityNum || self->client->ps.saberInFlight)
	{
		//our saber is currently dropped or in flight.
		if (!wp_using_dual_saber_as_primary(&self->client->ps))
		{
			//don't have a saber to block with
			return 0;
		}
	}

	if (self->client->ps.weaponstate == WEAPON_RAISING)
	{
		if (self->r.svFlags & SVF_BOT)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			if (self->client->ps.weapon == WP_SABER
				&& !BG_SabersOff(&self->client->ps)
				&& !self->client->ps.saberInFlight)
			{
				return 1;
			}
			return 0;
		}
		return 0;
	}

	if (!walk_check(self) && self->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
	{
		//can't block while running in force speed.
		return 0;
	}

	if (PM_InKnockDown(&self->client->ps))
	{
		//can't block while knocked down or getting up from knockdown.
		return 0;
	}

	if (atk && atk->client && atk->client->ps.weapon == WP_SABER)
	{
		//player is attacking with saber
		if (!BG_SaberInNonIdleDamageMove(&atk->client->ps, atk->localAnimIndex))
		{
			//saber attacker isn't in a real damaging move
			if (self->r.svFlags & SVF_BOT)
			{
				//bots just randomly parry to make up for them not intelligently parrying.
				if (self->client->ps.weapon == WP_SABER && !BG_SabersOff(&self->client->ps) && !self->client->ps.
					saberInFlight)
				{
					return 1;
				}
				return 0;
			}
			return 0;
		}

		if ((atk->client->ps.saber_move == LS_A_LUNGE
			|| atk->client->ps.saber_move == LS_SPINATTACK
			|| atk->client->ps.saber_move == LS_SPINATTACK_DUAL
			|| atk->client->ps.saber_move == LS_SPINATTACK_GRIEV
			|| atk->client->ps.saber_move == LS_GRIEVOUS_SPECIAL)
			&& self->client->ps.userInt3 & 1 << FLAG_FATIGUED)
		{
			//saber attacker, we can't block lunge attacks while fatigued.
			return 0;
		}

		if (PM_SuperBreakWinAnim(atk->client->ps.torsoAnim) && self->client->ps.fd.blockPoints < BLOCKPOINTS_THIRTY)
		{
			//can't block super breaks when in critical fp.
			return 0;
		}

		if (!walk_check(self)
			&& (!in_front(atk->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, -0.7f)
				|| PM_SaberInAttack(self->client->ps.saber_move)
				|| PM_SaberInStart(self->client->ps.saber_move)))
		{
			//can't block saber swings while running and hit from behind or in swing.
			if (self->r.svFlags & SVF_BOT)
			{
				//bots just randomly parry to make up for them not intelligently parrying.
				if (self->client->ps.weapon == WP_SABER && !BG_SabersOff(&self->client->ps) && !self->client->ps.
					saberInFlight)
				{
					return 1;
				}
				return 0;
			}
			return 0;
		}
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		if (self->r.svFlags & SVF_BOT)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			if (self->client->ps.weapon == WP_SABER
				&& !BG_SabersOff(&self->client->ps)
				&& !self->client->ps.saberInFlight)
			{
				return 1;
			}
			return 0;
		}
		return 0;
	}

	//check to see if we have the force to do this.
	if (self->client->ps.fd.blockPoints < WP_SaberBlockCost(self, atk, point))
	{
		if (self->r.svFlags & SVF_BOT)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			if (self->client->ps.weapon == WP_SABER && !BG_SabersOff(&self->client->ps) && !self->client->ps.
				saberInFlight)
			{
				return 1;
			}
			return 0;
		}
		return 0;
	}

	// allow for blocking behind our backs
	if (!in_front(point, self->client->ps.origin, self->client->ps.viewangles, -0.7f))
	{
		return 1;
	}

	if (!check_b_box_block)
	{
		//don't do the additional checkBBoxBlock checks.  As such, we're safe to saber block.
		return 1;
	}

	if (atk && atk->client && atk->client->ps.weapon == WP_SABER && PM_SuperBreakWinAnim(atk->client->ps.torsoAnim))
	{
		//never box block saberlock super break wins, it looks weird.
		return 0;
	}

	if (VectorCompare(point, vec3_origin))
	{
		//no hit position given, can't do blade movement check.
		return 0;
	}

	if (atk && atk->client && rSaberNum != -1 && rBladeNum != -1)
	{
		vec3_t saber_move_dir;
		vec3_t dir_to_body;
		vec3_t body_max;
		vec3_t body_min;
		vec3_t closest_body_point;
		//player attacker, if they are here they're using their saber to attack.
		//Check to make sure that we only block the blade if it is moving towards the player

		//create a line seqment thru the center of the player.
		VectorCopy(self->client->ps.origin, body_min);
		VectorCopy(self->client->ps.origin, body_max);

		body_max[2] += self->r.maxs[2];
		body_min[2] -= self->r.mins[2];

		//find dirToBody
		G_FindClosestPointOnLineSegment(body_min, body_max, point, closest_body_point);

		VectorSubtract(closest_body_point, point, dir_to_body);

		//find current saber movement direction of the attacker
		VectorSubtract(atk->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint,
			atk->client->saber[rSaberNum].blade[rBladeNum].muzzlePointOld, saber_move_dir);

		if (DotProduct(dir_to_body, saber_move_dir) < 0)
		{
			//saber is moving away from defender
			return 0;
		}
	}

	return 1;
}

extern qboolean PM_InLedgeMove(int anim);

int wp_player_must_dodge(const gentity_t* self, const gentity_t* shooter)
{
	if (in_camera)
	{
		return qfalse;
	}

	if (!self || !self->client || self->health <= 0)
	{
		return qfalse;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return qfalse;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//can't dodge in mid-air
		return qfalse;
	}

	if (!(self->client->ps.fd.forcePowersKnown & 1 << FP_SABER_DEFENSE))
	{
		//doesn't have saber defense
		return qfalse;
	}

	if (self->client->ps.forceHandExtend == HANDEXTEND_CHOKE
		|| PM_InGrappleMove(self->client->ps.torsoAnim) > 1)
	{
		//in some effect that stops me from moving on my own
		return qfalse;
	}

	if (BG_IsAlreadyinTauntAnim(self->client->ps.torsoAnim))
	{
		if (BG_HopAnim(self->client->ps.legsAnim) //in dodge hop
			|| BG_InRoll(&self->client->ps, self->client->ps.legsAnim)
			&& self->client->ps.userInt3 & 1 << FLAG_DODGEROLL)
		{
			//already doing a dodge
			return qtrue;
		}
		return qfalse;
	}

	if (PM_InKnockDown(&self->client->ps) ||
		(BG_InRoll(&self->client->ps, self->client->ps.legsAnim) ||
			BG_InKnockDown(self->client->ps.legsAnim)))
	{
		//can't Dodge while knocked down or getting up from knockdown.
		return qfalse;
	}

	if (self->client->ps.fd.forcePower < FATIGUE_DODGE)
	{
		//Not enough dodge
		return qfalse;
	}

	if (self->r.svFlags & SVF_BOT
		&& (self->client->pers.botclass == BCLASS_SBD ||
			self->client->pers.botclass == BCLASS_BATTLEDROID ||
			self->client->pers.botclass == BCLASS_DROIDEKA ||
			self->client->pers.botclass == BCLASS_PROTOCOL ||
			self->client->pers.botclass == BCLASS_JAWA))
	{
		// don't get Dodge.
		return qfalse;
	}

	if (self->client->ps.weaponTime > 0 || self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		//in some effect that stops me from moving on my own
		return qfalse;
	}

	if (self->client->ps.weapon == WP_TURRET
		|| self->client->ps.weapon == WP_EMPLACED_GUN
		|| self->client->ps.weapon == WP_BRYAR_OLD)
	{
		return qfalse;
	}

	//check for private duel conditions
	if (shooter && shooter->client)
	{
		if (shooter->client->ps.duelInProgress && shooter->client->ps.duelIndex != self->s.number)
		{
			//enemy client is in duel with someone else.
			return qfalse;
		}

		if (self->client->ps.duelInProgress && self->client->ps.duelIndex != shooter->s.number)
		{
			//we're in a duel with someone else.
			return qfalse;
		}
	}

	if (self->NPC
		&& (self->client->NPC_class == CLASS_STORMTROOPER ||
			self->client->NPC_class == CLASS_STORMCOMMANDO ||
			self->client->NPC_class == CLASS_SWAMPTROOPER ||
			self->client->NPC_class == CLASS_CLONETROOPER ||
			self->client->NPC_class == CLASS_IMPWORKER ||
			self->client->NPC_class == CLASS_IMPERIAL ||
			self->client->NPC_class == CLASS_BATTLEDROID ||
			self->client->NPC_class == CLASS_SBD ||
			self->client->NPC_class == CLASS_SABER_DROID ||
			self->client->NPC_class == CLASS_SBD ||
			self->client->NPC_class == CLASS_ASSASSIN_DROID ||
			self->client->NPC_class == CLASS_GONK ||
			self->client->NPC_class == CLASS_MOUSE ||
			self->client->NPC_class == CLASS_PROBE ||
			self->client->NPC_class == CLASS_PROTOCOL ||
			self->client->NPC_class == CLASS_R2D2 ||
			self->client->NPC_class == CLASS_R5D2 ||
			self->client->NPC_class == CLASS_SEEKER ||
			self->client->NPC_class == CLASS_INTERROGATOR))
	{
		// don't get Dodge.
		return qfalse;
	}

	if (!walk_check(self)
		|| PM_SaberInAttack(self->client->ps.saber_move)
		|| PM_SaberInStart(self->client->ps.saber_move))
	{
		if (self->NPC)
		{
			//can't Dodge saber swings while running or in swing.
			return qtrue;
		}
		return qfalse;
	}

	if (self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK)
	{
		return qfalse;
	}

	return qtrue;
}

int wp_saber_must_bolt_block(gentity_t* self, const gentity_t* atk, const qboolean check_b_box_block, vec3_t point,
	const int rSaberNum, const int rBladeNum)
{
	if (!self || !self->client || !atk)
	{
		return 0;
	}

	if (atk && atk->s.eType == ET_MISSILE &&
		(atk->s.weapon == WP_ROCKET_LAUNCHER ||
			atk->s.weapon == WP_THERMAL ||
			atk->s.weapon == WP_TRIP_MINE ||
			atk->s.weapon == WP_DET_PACK ||
			atk->methodOfDeath == MOD_REPEATER_ALT ||
			atk->methodOfDeath == MOD_CONC ||
			atk->methodOfDeath == MOD_CONC_ALT ||
			atk->methodOfDeath == MOD_FLECHETTE_ALT_SPLASH))
	{
		//can't block this stuff with a saber
		return 0;
	}

	if (self && self->client && self->client->MassiveBounceAnimTime > level.time)
	{
		return 0;
	}

	if (!(self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
	{
		if (self->r.svFlags & SVF_BOT)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			if (self->client->ps.weapon == WP_SABER && !BG_SabersOff(&self->client->ps) && !self->client->ps.
				saberInFlight)
			{
				return 1;
			}
			return 0;
		}
		return 0;
	}

	if (PM_SaberInBrokenParry(self->client->ps.saber_move))
	{
		//you've been stunned from a broken parry
		return 0;
	}

	if (PM_InGrappleMove(self->client->ps.torsoAnim))
	{
		//you can't block while doing a melee move.
		return 0;
	}

	if (PM_kick_move(self->client->ps.saber_move))
	{
		return 0;
	}

	if (self->client->ps.weapon != WP_SABER
		|| self->client->ps.weapon == WP_NONE
		|| self->client->ps.weapon == WP_MELEE) //saber not here
	{
		return 0;
	}

	if (self->client->ps.weapon == WP_SABER && self->client->ps.saberInFlight) //saber not here
	{
		//our saber is currently dropped or in flight.
		return 0;
	}

	if (self->client->ps.weaponstate == WEAPON_RAISING)
	{
		if (self->r.svFlags & SVF_BOT)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			if (self->client->ps.weapon == WP_SABER && !BG_SabersOff(&self->client->ps) && !self->client->ps.
				saberInFlight)
			{
				return 1;
			}
			return 0;
		}
		return 0;
	}

	if (!walk_check(self) && self->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
	{
		//can't block while running in force speed.
		return 0;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		// can't block in a stagger animation
		return 0;
	}

	if (PM_InKnockDown(&self->client->ps))
	{
		//can't block while knocked down or getting up from knockdown.
		return 0;
	}

	if (atk && atk->client && atk->client->ps.weapon == WP_SABER)
	{
		//player is attacking with saber
		if (!BG_SaberInNonIdleDamageMove(&atk->client->ps, atk->localAnimIndex))
		{
			//saber attacker isn't in a real damaging move
			if (self->r.svFlags & SVF_BOT)
			{
				//bots just randomly parry to make up for them not intelligently parrying.
				if (self->client->ps.weapon == WP_SABER && !BG_SabersOff(&self->client->ps) && !self->client->ps.
					saberInFlight)
				{
					return 1;
				}
				return 0;
			}
			return 0;
		}

		if ((atk->client->ps.saber_move == LS_A_LUNGE
			|| atk->client->ps.saber_move == LS_SPINATTACK
			|| atk->client->ps.saber_move == LS_SPINATTACK_DUAL
			|| atk->client->ps.saber_move == LS_SPINATTACK_GRIEV
			|| atk->client->ps.saber_move == LS_GRIEVOUS_SPECIAL)
			&& self->client->ps.userInt3 & 1 << FLAG_FATIGUED)
		{
			//saber attacker, we can't block lunge attacks while fatigued.
			return 0;
		}

		if (PM_SuperBreakWinAnim(atk->client->ps.torsoAnim) && self->client->ps.fd.blockPoints < BLOCKPOINTS_THIRTY)
		{
			//can't block super breaks when in critical fp.
			return 0;
		}

		if (!walk_check(self)
			&& (!in_front(atk->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, -0.7f)
				|| PM_SaberInAttack(self->client->ps.saber_move)
				|| PM_SaberInStart(self->client->ps.saber_move)))
		{
			//can't block saber swings while running and hit from behind or in swing.
			if (self->r.svFlags & SVF_BOT)
			{
				//bots just randomly parry to make up for them not intelligently parrying.
				if (self->client->ps.weapon == WP_SABER && !BG_SabersOff(&self->client->ps) && !self->client->ps.
					saberInFlight)
				{
					return 1;
				}
				return 0;
			}
			return 0;
		}
	}

	//check to see if we have the BLOCKPOINTS to do this.
	if (self->client->ps.fd.blockPoints < WP_SaberBlockCost(self, atk, point))
	{
		if (self->r.svFlags & SVF_BOT)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			if (self->client->ps.weapon == WP_SABER && !BG_SabersOff(&self->client->ps) && !self->client->ps.
				saberInFlight)
			{
				return 1;
			}
			return 0;
		}
		return 0;
	}

	// allow for blocking behind our backs
	if (!in_front(point, self->client->ps.origin, self->client->ps.viewangles, -0.7f))
	{
		return 1;
	}

	if (!check_b_box_block)
	{
		//don't do the additional checkBBoxBlock checks.  As such, we're safe to saber block.
		return 1;
	}

	if (atk && atk->client && atk->client->ps.weapon == WP_SABER && PM_SuperBreakWinAnim(atk->client->ps.torsoAnim))
	{
		//never box block saberlock super break wins, it looks weird.
		return 0;
	}

	if (VectorCompare(point, vec3_origin))
	{
		//no hit position given, can't do blade movement check.
		return 0;
	}

	if (atk && atk->client && rSaberNum != -1 && rBladeNum != -1)
	{
		vec3_t body_min;
		vec3_t body_max;
		vec3_t closest_body_point;
		vec3_t dir_to_body;
		vec3_t saber_move_dir;
		//player attacker, if they are here they're using their saber to attack.
		//Check to make sure that we only block the blade if it is moving towards the player

		//create a line seqment thru the center of the player.
		VectorCopy(self->client->ps.origin, body_min);
		VectorCopy(self->client->ps.origin, body_max);

		body_max[2] += self->r.maxs[2];
		body_min[2] -= self->r.mins[2];

		//find dirToBody
		G_FindClosestPointOnLineSegment(body_min, body_max, point, closest_body_point);

		VectorSubtract(closest_body_point, point, dir_to_body);

		//find current saber movement direction of the attacker
		VectorSubtract(atk->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint,
			atk->client->saber[rSaberNum].blade[rBladeNum].muzzlePointOld, saber_move_dir);

		if (DotProduct(dir_to_body, saber_move_dir) < 0)
		{
			//saber is moving away from defender
			return 0;
		}
	}

	return 1;
}

//Number of objects that a RealTrace can passthru when the ghoul2 trace fails.
#define MAX_REAL_PASSTHRU 8

//struct for saving the
typedef struct content_s
{
	int content;
	int entNum;
} content_t;

content_t real_trace_content[MAX_REAL_PASSTHRU];

#define REALTRACEDATADEFAULT	-2

static void init_real_trace_content(void)
{
	for (int i = 0; i < MAX_REAL_PASSTHRU; i++)
	{
		real_trace_content[i].content = REALTRACEDATADEFAULT;
		real_trace_content[i].entNum = REALTRACEDATADEFAULT;
	}
}

//returns true on success
static qboolean add_real_trace_content(const int entityNum)
{
	if (entityNum == ENTITYNUM_WORLD || entityNum == ENTITYNUM_NONE)
	{
		//can't blank out the world.  Give an error.
		return qtrue;
	}

	for (int i = 0; i < MAX_REAL_PASSTHRU; i++)
	{
		if (real_trace_content[i].content == REALTRACEDATADEFAULT && real_trace_content[i].entNum ==
			REALTRACEDATADEFAULT)
		{
			//found an empty slot.  Use it.
			//Stored Data
			real_trace_content[i].entNum = entityNum;
			real_trace_content[i].content = g_entities[entityNum].r.contents;

			//Blank it.
			g_entities[entityNum].r.contents = 0;
			return qtrue;
		}
	}

	//All slots already used.
	return qfalse;
}

//Restored all the entities that have been blanked out in the RealTrace
static void restore_real_trace_content(void)
{
	for (int i = 0; i < MAX_REAL_PASSTHRU; i++)
	{
		if (real_trace_content[i].entNum != REALTRACEDATADEFAULT)
		{
			if (real_trace_content[i].content != REALTRACEDATADEFAULT)
			{
				g_entities[real_trace_content[i].entNum].r.contents = real_trace_content[i].content;

				//Let's clean things out to be sure.
				real_trace_content[i].entNum = REALTRACEDATADEFAULT;
				real_trace_content[i].content = REALTRACEDATADEFAULT;
			}
			else
			{
				//
			}
		}
		else
		{
			//This data slot is blank.  This should mean that the rest are empty as well.
			break;
		}
	}
}

#define REALTRACE_MISS				0 //didn't hit anything
#define REALTRACE_HIT				1 //hit object normally
#define REALTRACE_SABERBLOCKHIT		2 //hit a player
#define REALTRACE_HIT_WORLD			3 //hit world

static QINLINE int finish_real_trace(trace_t* results, const trace_t* closest_trace, vec3_t end)
{
	//this function reverts the real trace content removals and finish up the realtrace
	//restore all the entities we blanked out.

	// Hit some entity...
	const gentity_t* current_ent = &g_entities[closest_trace->entityNum];

	restore_real_trace_content();

	if (VectorCompare(closest_trace->endpos, end))
	{
		//No hit. Make sure that tr is correct.
		trace_clear(results, end);
		return REALTRACE_MISS;
	}
	if (closest_trace->entityNum == ENTITYNUM_WORLD)
	{
		// Hit the world...
		trace_copy(closest_trace, results);
		return REALTRACE_HIT_WORLD;
	}
	if (current_ent && current_ent->inuse && current_ent->client)
	{
		// Hit a client...
		trace_copy(closest_trace, results);
		return REALTRACE_SABERBLOCKHIT;
	}

	trace_copy(closest_trace, results);
	return REALTRACE_HIT;
}

int g_real_trace(gentity_t* attacker, trace_t* tr, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end,
	const int pass_entity_num,
	const int contentmask, const int rSaberNum, const int rBladeNum)
{
	//the current start position of the traces.
	//This is advanced to the edge of each bound box after each saber/ghoul2 entity is processed.
	vec3_t current_start;
	trace_t closest_trace; //this is the trace struct of the closest successful trace.
	float closest_fraction = 1.1f;
	const qboolean atk_is_saberer = attacker && attacker->client && attacker->client->ps.weapon == WP_SABER
		? qtrue
		: qfalse;
	init_real_trace_content();

	if (atk_is_saberer)
	{
		//attacker is using a saber to attack us, blank out their saber/blade data so we have a fresh start for this trace.
		attacker->client->lastSaberCollided = -1;
		attacker->client->lastBladeCollided = -1;
	}

	//make the default closestTrace be nothing
	trace_clear(&closest_trace, end);

	VectorCopy(start, current_start);

	for (int misses = 0; misses < MAX_REAL_PASSTHRU; misses++)
	{
		vec3_t current_end_pos;

		//Fire a standard trace and see what we find.
		trap->Trace(tr, current_start, mins, maxs, end, pass_entity_num, contentmask, qfalse, 0, 0);

		//save the point where we hit.  This is either our end point or the point where we hit our next bounding box.
		VectorCopy(tr->endpos, current_end_pos);

		//also save the storedentity_num since the internal traces normally blank out the trace_t if they fail.
		const int current_entity_num = tr->entityNum;

		if (tr->startsolid)
		{
			//make sure that tr->endpos is at the start point as it should be for startsolid.
			VectorCopy(current_start, tr->endpos);
		}

		if (tr->entityNum == ENTITYNUM_NONE)
		{
			//We've run out of things to hit so we're done.
			if (!VectorCompare(start, current_start))
			{
				//didn't do trace with original start point.  Recalculate the real fraction before we do our comparision.
				tr->fraction = calc_trace_fraction(start, end, tr->endpos);
			}

			if (tr->fraction < closest_fraction)
			{
				//this is the closest hit, make it so.
				trace_copy(tr, &closest_trace);
			}
			return finish_real_trace(tr, &closest_trace, end);
		}

		//set up a pointer to the entity we hit.
		gentity_t* current_ent = &g_entities[tr->entityNum];

		if (current_ent->inuse && current_ent->client)
		{
			//initial trace hit a humanoid
			if (attacker && wp_saber_must_block(current_ent, attacker, qtrue, tr->endpos, rSaberNum, rBladeNum))
			{
				//hit victim is willing to bbox block with their jedi saber abilities.  Can only do this if we have data on the attacker.
				if (!VectorCompare(start, current_start))
				{
					//didn't do trace with original start point.  Recalculate the real fraction before we do our comparision.
					tr->fraction = calc_trace_fraction(start, end, tr->endpos);
				}

				if (tr->fraction < closest_fraction)
				{
					//this is the closest known hit object for this trace, so go ahead and count the bbox block as the closest impact.
					restore_real_trace_content();

					//act like the saber was hit instead of us.
					tr->entityNum = current_ent->client->saberStoredIndex;
					return REALTRACE_SABERBLOCKHIT;
				}
				//something else ghoul2 related was already hit and was closer, skip to end of function
				return finish_real_trace(tr, &closest_trace, end);
			}

			//ok, no bbox block this time.  So, try a ghoul2 trace then.
			G_G2TraceCollide(tr, current_start, end, mins, maxs);
		}
		else if (current_ent->r.contents & CONTENTS_LIGHTSABER &&
			current_ent->r.contents != -1 &&
			current_ent->inuse)
		{
			//hit a lightsaber, do the appropriate collision detection checks.
			const gentity_t* saber_owner = &g_entities[current_ent->r.ownerNum];

			g_saber_collide(atk_is_saberer ? attacker : NULL, saber_owner, current_start, end, mins, maxs, tr);
		}
		else if (tr->entityNum < ENTITYNUM_WORLD)
		{
			if (current_ent->inuse && current_ent->ghoul2)
			{
				//hit a non-client entity with a g2 instance
				G_G2TraceCollide(tr, current_start, end, mins, maxs);
			}
			else
			{
				//this object doesn't have a ghoul2 or saber internal trace.
				if (!VectorCompare(start, current_start))
				{
					//didn't do trace with original start point.  Recalculate the real fraction before we do our comparision.
					tr->fraction = calc_trace_fraction(start, end, tr->endpos);
				}

				//As such, it's the last trace on our layered trace.
				if (tr->fraction < closest_fraction)
				{
					//this is the closest hit, make it so.
					trace_copy(tr, &closest_trace);
				}
				return finish_real_trace(tr, &closest_trace, end);
			}
		}
		else
		{
			//world hit.  We either hit something closer or this is the final trace of our layered tracing.
			if (!VectorCompare(start, current_start))
			{
				//didn't do trace with original start point.  Recalculate the real fraction before we do our comparision.
				tr->fraction = calc_trace_fraction(start, end, tr->endpos);
			}

			if (tr->fraction < closest_fraction)
			{
				//this is the closest hit, make it so.
				trace_copy(tr, &closest_trace);
			}
			return finish_real_trace(tr, &closest_trace, end);
		}

		//ok, we're just completed an internal ghoul2 or saber internal trace on an entity.
		//At this point, we need to make this the closest impact if it was and continue scanning.
		//We do this since this ghoul2/saber entities have true impact positions that aren't the same as their bounding box
		//exterior impact position.  As such, another entity could be slightly inside that bounding box but have a closest
		//actual impact position.
		if (!VectorCompare(start, current_start))
		{
			//didn't do trace with original start point.  Recalculate the real fraction before we do our comparision.
			tr->fraction = calc_trace_fraction(start, end, tr->endpos);
		}

		if (tr->fraction < closest_fraction)
		{
			//current impact was the closest impact.
			trace_copy(tr, &closest_trace);
			closest_fraction = tr->fraction;
		}

		//remove the last hit entity from the trace and try again.
		if (!add_real_trace_content(current_entity_num))
		{
			//crap!  The data structure is full.  We're done.
			break;
		}

		//move our start trace point up to the point where we hit the bbox for the last ghoul2/saber object.
		VectorCopy(current_end_pos, current_start);
	}

	return finish_real_trace(tr, &closest_trace, end);
}

static float wp_saber_blade_length(const saberInfo_t* saber)
{
	float len = 0.0f;
	for (int i = 0; i < saber->numBlades; i++)
	{
		if (saber->blade[i].lengthMax > len)
		{
			len = saber->blade[i].lengthMax;
		}
	}
	return len;
}

float wp_saber_length(const gentity_t* ent)
{
	//return largest length
	if (!ent || !ent->client)
	{
		return 0.0f;
	}
	float best_len = 0.0f;
	for (int i = 0; i < MAX_SABERS; i++)
	{
		const float len = wp_saber_blade_length(&ent->client->saber[i]);
		if (len > best_len)
		{
			best_len = len;
		}
	}
	return best_len;
}

int wp_debug_saber_colour(const saber_colors_t saber_color)
{
	switch ((int)saber_color)
	{
	case SABER_RED:
		return 0x000000ff;
	case SABER_ORANGE:
		return 0x000088ff;
	case SABER_YELLOW:
		return 0x0000ffff;
	case SABER_GREEN:
		return 0x0000ff00;
	case SABER_BLUE:
		return 0x00ff0000;
	case SABER_PURPLE:
		return 0x00ff00ff;
	case SABER_LIME:
		return 0x0000ff00;
	default:
		return 0x00ffffff; //white
	}
}

extern saberInfo_t* BG_MySaber(int clientNum, int saberNum);

#define MAX_SABER_VICTIMS 8192
static int victimentity_num[MAX_SABER_VICTIMS];
static qboolean victimHitEffectDone[MAX_SABER_VICTIMS];
static float totalDmg[MAX_SABER_VICTIMS];
static vec3_t dmgDir[MAX_SABER_VICTIMS];
static vec3_t dmgSpot[MAX_SABER_VICTIMS];
static qboolean dismemberDmg[MAX_SABER_VICTIMS];
static int saberKnockbackFlags[MAX_SABER_VICTIMS];
static int numVictims = 0;

static void WP_SaberClearDamage(void)
{
	for (int ven = 0; ven < MAX_SABER_VICTIMS; ven++)
	{
		victimentity_num[ven] = ENTITYNUM_NONE;
	}
	memset(victimHitEffectDone, 0, sizeof victimHitEffectDone);
	memset(totalDmg, 0, sizeof totalDmg);
	memset(dmgDir, 0, sizeof dmgDir);
	memset(dmgSpot, 0, sizeof dmgSpot);
	memset(dismemberDmg, 0, sizeof dismemberDmg);
	memset(saberKnockbackFlags, 0, sizeof saberKnockbackFlags);
	numVictims = 0;
}

static void wp_saber_specific_do_hit(const gentity_t* self, const int saberNum, const int blade_num, const gentity_t* victim, vec3_t impactpoint, const int dmg)
{
	qboolean is_droid = qfalse;

	if (victim->client)
	{
		const class_t npc_class = victim->client->NPC_class;

		if (npc_class == CLASS_SEEKER
			|| npc_class == CLASS_PROBE
			|| npc_class == CLASS_MOUSE
			|| npc_class == CLASS_SBD
			|| npc_class == CLASS_BATTLEDROID
			|| npc_class == CLASS_DROIDEKA
			|| npc_class == CLASS_GONK
			|| npc_class == CLASS_R2D2
			|| npc_class == CLASS_R5D2
			|| npc_class == CLASS_REMOTE
			|| npc_class == CLASS_MARK1
			|| npc_class == CLASS_MARK2
			|| npc_class == CLASS_PROTOCOL
			|| npc_class == CLASS_INTERROGATOR
			|| npc_class == CLASS_ATST
			|| npc_class == CLASS_SENTRY)
		{
			// special droid only behaviors
			is_droid = qtrue;
		}
	}

	gentity_t* te = G_TempEntity(impactpoint, EV_SABER_HIT);

	if (te)
	{
		te->s.otherentity_num = victim->s.number;
		te->s.otherentity_num2 = self->s.number;
		te->s.weapon = saberNum;
		te->s.legsAnim = blade_num;

		VectorCopy(impactpoint, te->s.origin);
		VectorScale(impactpoint, -1, te->s.angles);

		if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
		{
			//don't let it play with no direction
			te->s.angles[1] = 1;
		}

		if (!is_droid && (victim->client || victim->s.eType == ET_NPC || victim->s.eType == ET_BODY))
		{
			if (dmg < 5)
			{
				te->s.eventParm = 3;
			}
			else if (dmg < 20)
			{
				te->s.eventParm = 2;
			}
			else
			{
				te->s.eventParm = 1;
			}
		}
		else
		{
			if (!WP_SaberBladeUseSecondBladeStyle(&self->client->saber[saberNum], blade_num)
				&& self->client->saber[saberNum].saberFlags2 & SFL2_NO_CLASH_FLARE)
			{
				//don't do clash flare
			}
			else if (WP_SaberBladeUseSecondBladeStyle(&self->client->saber[saberNum], blade_num)
				&& self->client->saber[saberNum].saberFlags2 & SFL2_NO_CLASH_FLARE2)
			{
				//don't do clash flare
			}
			else
			{
				if (dmg > SABER_NONATTACK_DAMAGE)
				{
					//I suppose I could tie this into the saberblock event, but I'm tired of adding flags to that thing.
					gentity_t* te_s = G_TempEntity(te->s.origin, EV_SABER_CLASHFLARE);
					VectorCopy(te->s.origin, te_s->s.origin);
				}
				te->s.eventParm = 0;
			}
		}
	}
}

extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength, const qboolean breakSaberLock);
static qboolean saberDoClashEffect = qfalse;
static vec3_t saberClashPos = { 0 };
static vec3_t saberClashNorm = { 0 };
static int saberClashEventParm = 1;
static int saberClashOther = -1; //the clientNum for the other player involved in the saber clash.
static QINLINE void G_SetViewLock(const gentity_t* self, vec3_t impact_pos, vec3_t impact_normal);
static QINLINE void G_SetViewLockDebounce(const gentity_t* self);

static void WP_SaberDoClash(const gentity_t* self, const int saberNum, const int blade_num)
{
	if (saberDoClashEffect)
	{
		gentity_t* te = G_TempEntity(saberClashPos, EV_SABER_BLOCK);
		VectorCopy(saberClashPos, te->s.origin);
		VectorCopy(saberClashNorm, te->s.angles);
		te->s.eventParm = saberClashEventParm;
		te->s.otherentity_num2 = self->s.number;
		te->s.weapon = saberNum;
		te->s.legsAnim = blade_num;

		if (saberClashOther != -1 && PM_SaberInParry(g_entities[saberClashOther].client->ps.saber_move))
		{
			const gentity_t* other_owner = &g_entities[saberClashOther];

			G_SetViewLock(self, saberClashPos, saberClashNorm);
			G_SetViewLockDebounce(self);

			G_SetViewLock(other_owner, saberClashPos, saberClashNorm);
			G_SetViewLockDebounce(other_owner);
		}
	}
	saberDoClashEffect = qfalse;
}

static void WP_SaberBounceSound(gentity_t* ent, const int saberNum, const int blade_num)
{
	if (!ent || !ent->client)
	{
		return;
	}
	const int index = Q_irand(1, 64);
	if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->saber[saberNum], blade_num)
		&& ent->client->saber[saberNum].bounceSound[0])
	{
		G_Sound(ent, CHAN_AUTO, ent->client->saber[saberNum].bounceSound[Q_irand(0, 2)]);
	}
	else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->saber[saberNum], blade_num)
		&& ent->client->saber[saberNum].bounce2Sound[0])
	{
		G_Sound(ent, CHAN_AUTO, ent->client->saber[saberNum].bounce2Sound[Q_irand(0, 2)]);
	}

	else if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->saber[saberNum], blade_num)
		&& ent->client->saber[saberNum].blockSound[0])
	{
		G_Sound(ent, CHAN_AUTO, ent->client->saber[saberNum].blockSound[Q_irand(0, 2)]);
	}
	else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->saber[saberNum], blade_num)
		&& ent->client->saber[saberNum].block2Sound[0])
	{
		G_Sound(ent, CHAN_AUTO, ent->client->saber[saberNum].block2Sound[Q_irand(0, 2)]);
	}
	else
	{
		G_Sound(ent, CHAN_AUTO, G_SoundIndex(va("sound/weapons/saber/saberbounce%d.wav", index)));
	}
}

static void WP_SaberBounceOnWallSound(gentity_t* ent, const int saberNum, const int blade_num)
{
	if (!ent || !ent->client)
	{
		return;
	}
	const int index = Q_irand(1, 90);
	const int classicindex = Q_irand(1, 30);

	if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->saber[saberNum], blade_num)
		&& ent->client->saber[saberNum].bounceSound[0])
	{
		G_Sound(ent, CHAN_AUTO, ent->client->saber[saberNum].bounceSound[Q_irand(0, 2)]);
	}
	else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->saber[saberNum], blade_num)
		&& ent->client->saber[saberNum].bounce2Sound[0])
	{
		G_Sound(ent, CHAN_AUTO, ent->client->saber[saberNum].bounce2Sound[Q_irand(0, 2)]);
	}

	else if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->saber[saberNum], blade_num)
		&& ent->client->saber[saberNum].blockSound[0])
	{
		G_Sound(ent, CHAN_AUTO, ent->client->saber[saberNum].blockSound[Q_irand(0, 2)]);
	}
	else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->saber[saberNum], blade_num)
		&& ent->client->saber[saberNum].block2Sound[0])
	{
		G_Sound(ent, CHAN_AUTO, ent->client->saber[saberNum].block2Sound[Q_irand(0, 2)]);
	}
	else
	{
		if (ent->client->saber[saberNum].type == SABER_SINGLE_CLASSIC)
		{
			G_Sound(ent, CHAN_AUTO, G_SoundIndex(va("sound/weapons/saber/classicblock%d.mp3", classicindex)));
		}
		else
		{
			G_Sound(ent, CHAN_AUTO, G_SoundIndex(va("sound/weapons/saber/saberblock%d.mp3", index)));
		}
	}
}

static QINLINE void G_SetViewLockDebounce(const gentity_t* self)
{
	if (!walk_check(self))
	{
		//running pauses you longer
		self->client->viewLockTime = level.time + 500;
	}
	else if (PM_SaberInParry(G_GetParryForBlock(self->client->ps.saberBlocked)) //normal block (not a parry)
		|| !PM_SaberInKnockaway(self->client->ps.saber_move) //didn't parry
		&& self->client->ps.stats[STAT_HEALTH] < self->client->ps.stats[STAT_MAX_HEALTH] * .50)
	{
		//normal block or attacked with less than %50HP
		self->client->viewLockTime = level.time + 300;
	}
	else
	{
		self->client->viewLockTime = level.time;
	}
}

static QINLINE void G_SetViewLock(const gentity_t* self, vec3_t impact_pos, vec3_t impact_normal)
{
	//Sets the view/movement lock flags based on the given information
	vec3_t cross;
	vec3_t length;
	vec3_t forward;
	vec3_t right;
	vec3_t up;

	if (!self || !self->client)
	{
		return;
	}

	//Since this is the only function that sets/unsets these flags.  We need to clear them
	//all before messing with them.  Otherwise we end up with all the flags piling up until
	//they are cleared.
	self->client->ps.userInt1 = 0;

	if (VectorCompare(impact_normal, vec3_origin)
		|| self->client->ps.saberInFlight)
	{
		//bad impact surface normal or our saber is in flight
		return;
	}

	//find the impact point in terms of the player origin
	VectorSubtract(impact_pos, self->client->ps.origin, length);

	//Check for very low hits.  If it's very close to the lower bbox edge just skip view/move locking.
	//This is to prevent issues with many of the saber moves touching the ground naturally.
	if (length[2] < .8 * self->r.mins[2])
	{
		return;
	}

	CrossProduct(length, impact_normal, cross);

	if (cross[0] > 0)
	{
		self->client->ps.userInt1 |= LOCK_UP;
	}
	else if (cross[0] < 0)
	{
		self->client->ps.userInt1 |= LOCK_DOWN;
	}

	if (cross[2] > 0)
	{
		self->client->ps.userInt1 |= LOCK_RIGHT;
	}
	else if (cross[2] < 0)
	{
		self->client->ps.userInt1 |= LOCK_LEFT;
	}

	//Movement lock

	//Forward
	AngleVectors(self->client->ps.viewangles, forward, right, up);
	if (DotProduct(length, forward) < 0)
	{
		//lock backwards
		self->client->ps.userInt1 |= LOCK_MOVEBACK;
	}
	else if (DotProduct(length, forward) > 0)
	{
		//lock forwards
		self->client->ps.userInt1 |= LOCK_MOVEFORWARD;
	}

	if (DotProduct(length, right) < 0)
	{
		self->client->ps.userInt1 |= LOCK_MOVELEFT;
	}
	else if (DotProduct(length, right) > 0)
	{
		self->client->ps.userInt1 |= LOCK_MOVERIGHT;
	}

	if (DotProduct(length, up) > 0)
	{
		self->client->ps.userInt1 |= LOCK_MOVEDOWN;
	}
	else if (DotProduct(length, up) < 0)
	{
		self->client->ps.userInt1 |= LOCK_MOVEUP;
	}
}

void AnimateStun(gentity_t* self, gentity_t* inflictor, vec3_t impact)
{
	//place self into a stunned state.
	if (self->client->ps.weapon != WP_SABER)
	{
		//knock them down instead
		G_Knockdown(self, inflictor, vec3_origin, 50, qtrue);
	}
	else if (!PM_SaberInBrokenParry(self->client->ps.saber_move) && !PM_InKnockDown(&self->client->ps))
	{
		if (!PM_SaberInParry(G_GetParryForBlock(self->client->ps.saberBlocked)))
		{
			//not already in a parry position, get one
			WP_SaberBlockNonRandom(self, impact, qfalse);
		}

		self->client->ps.saber_move = PM_BrokenParryForParry(G_GetParryForBlock(self->client->ps.saberBlocked));
		self->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;

		//make pain noise
		if (npc_is_dark_jedi(self))
		{
			// Do taunt/anger...
			const int call_out = Q_irand(0, 3);

			switch (call_out)
			{
			case 0:
			default:
				G_AddVoiceEvent(self, Q_irand(EV_TAUNT1, EV_TAUNT3), 3000);
				break;
			case 1:
				G_AddVoiceEvent(self, Q_irand(EV_ANGER1, EV_ANGER1), 3000);
				break;
			case 2:
				G_AddVoiceEvent(self, Q_irand(EV_COMBAT1, EV_COMBAT3), 3000);
				break;
			case 3:
				G_AddVoiceEvent(self, Q_irand(EV_JCHASE1, EV_JCHASE3), 3000);
				break;
			}
		}
		else if (npc_is_light_jedi(self))
		{
			// Do taunt...
			const int call_out = Q_irand(0, 3);

			switch (call_out)
			{
			case 0:
			default:
				G_AddVoiceEvent(self, Q_irand(EV_TAUNT1, EV_TAUNT3), 3000);
				break;
			case 1:
				G_AddVoiceEvent(self, Q_irand(EV_ANGER1, EV_ANGER1), 3000);
				break;
			case 2:
				G_AddVoiceEvent(self, Q_irand(EV_COMBAT1, EV_COMBAT3), 3000);
				break;
			case 3:
				G_AddVoiceEvent(self, Q_irand(EV_JCHASE1, EV_JCHASE3), 3000);
				break;
			}
		}
		else
		{
			// Do taunt/anger...
			const int call_out = Q_irand(0, 3);

			switch (call_out)
			{
			case 0:
			default:
				G_AddVoiceEvent(self, Q_irand(EV_TAUNT1, EV_TAUNT3), 3000);
				break;
			case 1:
				G_AddVoiceEvent(self, Q_irand(EV_ANGER1, EV_ANGER1), 3000);
				break;
			case 2:
				G_AddVoiceEvent(self, Q_irand(EV_COMBAT1, EV_COMBAT3), 3000);
				break;
			case 3:
				G_AddVoiceEvent(self, Q_irand(EV_JCHASE1, EV_JCHASE3), 3000);
				break;
			}
		}
	}
}

qboolean WP_BrokenBoltBlockKnockBack(gentity_t* victim)
{
	if (!victim || !victim->client)
	{
		return qfalse;
	}

	if (victim->s.weapon == WP_SABER && !BG_SabersOff(&victim->client->ps) && victim->client->ps.fd.blockPoints <=
		BLOCKPOINTS_TEN)
	{
		//knock their asses down!
		G_Stagger(victim);

		vec3_t throw_dir = { 0, 0, 350 };

		saberKnockOutOfHand(&g_entities[victim->client->ps.saberEntityNum], victim, throw_dir);

		G_AddEvent(victim, EV_PAIN, victim->health);
		return qtrue;
	}
	G_Stagger(victim);
	return qtrue;
}

//Check to see if the player is actually walking or just standing
qboolean walk_check(const gentity_t* self)
{
	if (PM_RunningAnim(self->client->ps.legsAnim))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean pm_walking_or_standing(const gentity_t* self)
{
	//mainly for checking if we can guard well
	if (PM_WalkingAnim(self->client->ps.legsAnim)
		|| PM_StandingAnim(self->client->ps.legsAnim)
		|| PM_CrouchAnim(self->client->ps.legsAnim))
	{
		return qtrue;
	}
	return qfalse;
}

static void DoNormalDodge(gentity_t* self, const int dodge_anim)
{
	//have self go into the given dodgeAnim
	//Our own happy way of forcing an anim:
	self->client->ps.forceHandExtend = HANDEXTEND_DODGE;
	self->client->ps.forceDodgeAnim = dodge_anim;
	self->client->ps.forceHandExtendTime = level.time + 300;
	self->client->ps.weaponTime = 300;
	self->client->ps.saber_move = LS_NONE;

	G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/melee/swing4.wav"));
}

static qboolean DodgeRollCheck(const gentity_t* self, const int dodge_anim, vec3_t forward, vec3_t right)
{
	//checks to see if there's a cliff in the direction that dodgeAnim would travel.
	vec3_t mins, maxs, traceto_mod, tracefrom_mod, move_dir;
	trace_t tr;
	int dodge_distance = 200; //how far away do we do the scans.

	//determine which direction we're traveling in.
	if (dodge_anim == BOTH_ROLL_F || dodge_anim == BOTH_ROLL_F1 || dodge_anim == BOTH_ROLL_F2 || dodge_anim ==
		BOTH_HOP_F)
	{
		//forward rolls
		VectorCopy(forward, move_dir);
	}
	else if (dodge_anim == BOTH_ROLL_B || dodge_anim == BOTH_HOP_B)
	{
		//backward rolls
		VectorCopy(forward, move_dir);
		VectorScale(move_dir, -1, move_dir);
	}
	else if (dodge_anim == BOTH_GETUP_BROLL_R
		|| dodge_anim == BOTH_GETUP_FROLL_R
		|| dodge_anim == BOTH_ROLL_R
		|| dodge_anim == BOTH_HOP_R)
	{
		//right rolls
		VectorCopy(right, move_dir);
	}
	else if (dodge_anim == BOTH_GETUP_BROLL_L
		|| dodge_anim == BOTH_GETUP_FROLL_L
		|| dodge_anim == BOTH_ROLL_L
		|| dodge_anim == BOTH_HOP_L)
	{
		//left rolls
		VectorCopy(right, move_dir);
		VectorScale(move_dir, -1, move_dir);
	}
	else
	{
		return qfalse;
	}

	if (dodge_anim == BOTH_HOP_L || dodge_anim == BOTH_HOP_R)
	{
		//left/right hops are shorter
		dodge_distance = 120;
	}

	//set up the trace positions
	VectorCopy(self->client->ps.origin, tracefrom_mod);
	VectorMA(tracefrom_mod, dodge_distance, move_dir, traceto_mod);

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -10; //was -18, changed because bots were hopping over the flag stands
	//on ctf1
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	//check for solids/or players in the way.
	trap->Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, self->client->ps.clientNum, MASK_PLAYERSOLID, qfalse, 0,
		0);

	if (tr.fraction != 1 || tr.startsolid)
	{
		//something is in the way.
		return qfalse;
	}

	VectorCopy(traceto_mod, tracefrom_mod);

	//check for 20+ feet drops
	traceto_mod[2] -= 200;

	trap->Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, self->client->ps.clientNum, MASK_SOLID, qfalse, 0, 0);
	if (tr.fraction == 1 && !tr.startsolid)
	{
		//CLIFF!
		return qfalse;
	}

	const int contents = trap->PointContents(tr.endpos, -1);
	if (contents & (CONTENTS_SLIME | CONTENTS_LAVA))
	{
		//the fall point is inside something we don't want to fall to
		return qfalse;
	}

	return qtrue;
}

extern qboolean PM_InKnockDown(const playerState_t* ps);
//Returns qfalse if hit effects/damage is still suppose to be applied.
qboolean G_DoDodge(gentity_t* self, gentity_t* shooter, vec3_t dmg_origin, int hit_loc, int* dmg, const int mod)
{
	int dodge_anim = -1;
	int dpcost = BasicWeaponBlockCosts[mod];
	const int saved_dmg = *dmg;
	qboolean no_action = qfalse;

	if (in_camera)
	{
		return qfalse;
	}

	if (self->NPC &&
		(self->client->NPC_class == CLASS_STORMTROOPER
			|| self->client->NPC_class == CLASS_STORMCOMMANDO
			|| self->client->NPC_class == CLASS_SWAMPTROOPER
			|| self->client->NPC_class == CLASS_CLONETROOPER
			|| self->client->NPC_class == CLASS_IMPWORKER
			|| self->client->NPC_class == CLASS_IMPERIAL
			|| self->client->NPC_class == CLASS_SABER_DROID
			|| self->client->NPC_class == CLASS_ASSASSIN_DROID
			|| self->client->NPC_class == CLASS_GONK
			|| self->client->NPC_class == CLASS_MOUSE
			|| self->client->NPC_class == CLASS_PROBE
			|| self->client->NPC_class == CLASS_PROTOCOL
			|| self->client->NPC_class == CLASS_R2D2
			|| self->client->NPC_class == CLASS_R5D2
			|| self->client->NPC_class == CLASS_SEEKER
			|| self->client->NPC_class == CLASS_INTERROGATOR))
	{
		// don't get Dodge.
		return qfalse;
	}

	if (dpcost == -1)
	{
		//can't dodge this.
		return qfalse;
	}

	if (!self || !self->client || !self->inuse || self->health <= 0)
	{
		return qfalse;
	}

	if (self->client->ps.stats[STAT_DODGE] < 30)
	{
		//Not enough dodge
		return qfalse;
	}

	//check for private duel conditions
	if (shooter && shooter->client)
	{
		if (shooter->client->ps.duelInProgress && shooter->client->ps.duelIndex != self->s.number)
		{
			//enemy client is in duel with someone else.
			return qfalse;
		}

		if (self->client->ps.duelInProgress && self->client->ps.duelIndex != shooter->s.number)
		{
			//we're in a duel with someone else.
			return qfalse;
		}
	}

	if (BG_HopAnim(self->client->ps.legsAnim) //in dodge hop
		|| BG_InRoll(&self->client->ps, self->client->ps.legsAnim)
		&& self->client->ps.userInt3 & 1 << FLAG_DODGEROLL //in dodge roll
		|| mod != MOD_SABER && self->client->ps.forceHandExtend == HANDEXTEND_DODGE)
	{
		//already doing a dodge
		return qtrue;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE
		&& mod != MOD_REPEATER_ALT_SPLASH
		&& mod != MOD_FLECHETTE_ALT_SPLASH
		&& mod != MOD_ROCKET_SPLASH
		&& mod != MOD_ROCKET_HOMING_SPLASH
		&& mod != MOD_THERMAL_SPLASH
		&& mod != MOD_TRIP_MINE_SPLASH
		&& mod != MOD_TIMED_MINE_SPLASH
		&& mod != MOD_DET_PACK_SPLASH)
	{
		//can't dodge direct fire in mid-air
		return qfalse;
	}

	if (PM_InKnockDown(&self->client->ps)
		|| BG_InRoll(&self->client->ps, self->client->ps.legsAnim)
		|| BG_InKnockDown(self->client->ps.legsAnim))
	{
		//can't Dodge while knocked down or getting up from knockdown.
		return qfalse;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//can't dodge in mid-air
		return qfalse;
	}

	if (self->client->ps.forceHandExtend == HANDEXTEND_CHOKE
		|| PM_InGrappleMove(self->client->ps.torsoAnim) > 1)
	{
		//in some effect that stops me from moving on my own
		return qfalse;
	}

	if (mod == MOD_MELEE)
	{
		//don't dodge melee attacks for now.
		return qfalse;
	}

	if (BG_IsAlreadyinTauntAnim(self->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (self->client->ps.weapon == WP_SABER)
	{
		return qfalse; //dont do if we are already blocking
	}

	if (self->client->ps.weapon == WP_SABER && self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK)
	{
		return qfalse; //dont do if we are already blocking
	}

	if (self->client->ps.weapon == WP_SABER && PM_CrouchingAnim(self->client->ps.legsAnim))
	{
		return qfalse; //dont do if we are already blocking
	}

	if (self->client->ps.weapon == WP_SABER && PM_SaberInAttack(self->client->ps.saber_move))
	{
		return qfalse; //dont do if we are already blocking
	}

	if (self->r.svFlags & SVF_BOT
		&& self->client->ps.weapon == WP_SABER
		&& self->client->ps.fd.forcePower < FATIGUE_DODGEINGBOT)
		// if your not a mando or jedi or low FP then you cant dodge
	{
		return qfalse;
	}

	if (self->r.svFlags & SVF_BOT
		&& (self->client->ps.weapon != WP_SABER &&
			self->client->pers.botclass != BCLASS_MANDOLORIAN &&
			self->client->pers.botclass != BCLASS_MANDOLORIAN1 &&
			self->client->pers.botclass != BCLASS_MANDOLORIAN2 &&
			self->client->pers.botclass != BCLASS_BOBAFETT)
		|| self->client->ps.fd.forcePower < FATIGUE_DODGEINGBOT)
		// if your not a mando or jedi or low FP then you cant dodge
	{
		return qfalse;
	}

	if (self->client->ps.fd.forcePower < FATIGUE_DODGE && !(self->r.svFlags & SVF_BOT))
	{
		//Not enough dodge
		return qfalse;
	}

	if (mod == MOD_SABER && shooter && shooter->client)
	{
		//special saber moves have special effects.
		if (shooter->client->ps.saber_move == LS_A_LUNGE
			|| shooter->client->ps.saber_move == LS_SPINATTACK
			|| shooter->client->ps.saber_move == LS_SPINATTACK_DUAL
			|| shooter->client->ps.saber_move == LS_SPINATTACK_GRIEV)
		{
			//attacker is doing lunge special
			if (self->client->ps.userInt3 & 1 << FLAG_FATIGUED)
			{
				//can't dodge a lunge special while fatigued
				return qfalse;
			}
		}

		if (PM_SuperBreakWinAnim(shooter->client->ps.torsoAnim) && self->client->ps.fd.forcePower < 50)
		{
			//can't block super breaks if we're low on DP.
			return qfalse;
		}

		if (!walk_check(self)
			|| PM_SaberInAttack(self->client->ps.saber_move)
			|| PM_SaberInStart(self->client->ps.saber_move))
		{
			if (self->NPC)
			{
				//can't Dodge saber swings while running or in swing.
				return qtrue;
			}
			return qfalse;
		}
	}

	if (*dmg <= DODGE_MINDAM && mod != MOD_REPEATER)
	{
		dpcost = (int)(dpcost * ((float)*dmg / DODGE_MINDAM));
		no_action = qtrue;
	}
	else if (self->client->ps.forceHandExtend == HANDEXTEND_DODGE)
	{
		//you're already dodging but you got hit again.
		dpcost = 0;
	}

	if (mod == MOD_DISRUPTOR_SNIPER && shooter && shooter->client)
	{
		int damage = 0;

		if (shooter->genericValue6 <= 10)
		{
			damage = 10;
		}
		else if (shooter->genericValue6 <= 25)
		{
			damage = 20;
		}
		else if (shooter->genericValue6 < 59)
		{
			damage = 50;
		}
		else if (shooter->genericValue6 == 60)
		{
			damage = 100;
		}

		if (self->client->ps.stats[STAT_DODGE] > damage)
		{
			dpcost = damage;
		}
		else
		{
			return qfalse;
		}
	}

	if (dpcost < self->client->ps.stats[STAT_DODGE])
	{
		G_DodgeDrain(self, shooter, dpcost);
	}
	else if (dpcost != 0 && self->client->ps.stats[STAT_DODGE])
	{
		//still have enough DP for a partial dodge.
		//Scale damage as is appropriate
		*dmg = (int)(*dmg * (self->client->ps.stats[STAT_DODGE] / (float)dpcost));
		G_DodgeDrain(self, shooter, self->client->ps.stats[STAT_DODGE]);
	}
	else
	{
		//not enough DP left
		return qfalse;
	}

	if (self->r.svFlags & SVF_BOT) //bots only get 1
	{
		PM_AddFatigue(&self->client->ps, FATIGUE_DODGEINGBOT);
	}
	else
	{
		PM_AddFatigue(&self->client->ps, FATIGUE_DODGEING);
	}

	if (no_action)
	{
		return qtrue;
	}

	if (mod == MOD_REPEATER_ALT_SPLASH
		|| mod == MOD_FLECHETTE_ALT_SPLASH
		|| mod == MOD_ROCKET_SPLASH
		|| mod == MOD_ROCKET_HOMING_SPLASH
		|| mod == MOD_THERMAL_SPLASH
		|| mod == MOD_TRIP_MINE_SPLASH
		|| mod == MOD_TIMED_MINE_SPLASH
		|| mod == MOD_DET_PACK_SPLASH
		|| mod == MOD_ROCKET)
	{
		//splash damage dodge, dodged by throwing oneself away from the blast into a knockdown
		vec3_t blow_back_dir;
		int blow_back_power = saved_dmg;
		VectorSubtract(self->client->ps.origin, dmg_origin, blow_back_dir);
		VectorNormalize(blow_back_dir);

		if (blow_back_power > 1000)
		{
			//clamp blow back power level
			blow_back_power = 1000;
		}
		blow_back_power *= 2;
		g_throw(self, blow_back_dir, blow_back_power);
		G_Knockdown(self, shooter, blow_back_dir, 600, qtrue);
		return qtrue;
	}

	/*===========================================================================
	doing a positional dodge for direct hit damage (like sabers or blaster bolts)
	===========================================================================*/
	if (hit_loc == -1)
	{
		//Use the last surface impact data as the hit location
		if (d_saberGhoul2Collision.integer && self->client
			&& self->client->g2LastSurfaceModel == G2MODEL_PLAYER
			&& self->client->g2LastSurfaceTime == level.time)
		{
			char hit_surface[MAX_QPATH];

			trap->G2API_GetSurfaceName(self->ghoul2, self->client->g2LastSurfaceHit, 0, hit_surface);

			if (hit_surface[0])
			{
				G_GetHitLocFromSurfName(self, hit_surface, &hit_loc, dmg_origin, vec3_origin, vec3_origin, MOD_SABER);
			}
		}
		else
		{
			//ok, that didn't work.  Try the old math way.
			hit_loc = G_GetHitLocation(self, dmg_origin);
		}
	}

	switch (hit_loc)
	{
	case HL_NONE:
		return qfalse;

	case HL_FOOT_RT:
	case HL_FOOT_LT:
		dodge_anim = Q_irand(BOTH_HOP_L, BOTH_HOP_R);
		break;
	case HL_LEG_RT:
	case HL_LEG_LT:
		if (self->client->pers.botclass == BCLASS_MANDOLORIAN
			|| self->client->pers.botclass == BCLASS_BOBAFETT
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			self->client->jetPackOn = qtrue;
			self->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			self->client->ps.eFlags |= EF_JETPACK_FLAMING;
			self->client->ps.eFlags |= EF3_JETPACK_HOVER;
			Boba_FlyStart(self);
			self->client->ps.fd.forceJumpCharge = 280;
			self->client->jetPackTime = level.time + 30000;
		}
		else
		{
			dodge_anim = Q_irand(BOTH_HOP_L, BOTH_HOP_R);
		}
		break;

	case HL_BACK_RT:
		dodge_anim = BOTH_DODGE_FL;
		break;
	case HL_CHEST_RT:
		dodge_anim = BOTH_DODGE_FR;
		break;
	case HL_BACK_LT:
		dodge_anim = BOTH_DODGE_FR;
		break;
	case HL_CHEST_LT:
		dodge_anim = BOTH_DODGE_FR;
		break;
	case HL_BACK:
		if (self->client->pers.botclass == BCLASS_MANDOLORIAN
			|| self->client->pers.botclass == BCLASS_BOBAFETT
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			self->client->jetPackOn = qtrue;
			self->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			self->client->ps.eFlags |= EF_JETPACK_FLAMING;
			self->client->ps.eFlags |= EF3_JETPACK_HOVER;
			Boba_FlyStart(self);
			self->client->ps.fd.forceJumpCharge = 280;
			self->client->jetPackTime = (self->client->jetPackTime + level.time) / 2 + 10000;
		}
		else
		{
			dodge_anim = Q_irand(BOTH_DODGE_FL, BOTH_DODGE_FR);
		}
		break;
	case HL_CHEST:
		if (self->client->pers.botclass == BCLASS_MANDOLORIAN
			|| self->client->pers.botclass == BCLASS_BOBAFETT
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			self->client->jetPackOn = qtrue;
			self->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			self->client->ps.eFlags |= EF_JETPACK_FLAMING;
			self->client->ps.eFlags |= EF3_JETPACK_HOVER;
			Boba_FlyStart(self);
			self->client->ps.fd.forceJumpCharge = 280;
			self->client->jetPackTime = (self->client->jetPackTime + level.time) / 2 + 10000;
		}
		else
		{
			dodge_anim = BOTH_DODGE_B;
		}
		break;
	case HL_WAIST:
		if (self->client->pers.botclass == BCLASS_MANDOLORIAN
			|| self->client->pers.botclass == BCLASS_BOBAFETT
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			self->client->jetPackOn = qtrue;
			self->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			self->client->ps.eFlags |= EF_JETPACK_FLAMING;
			self->client->ps.eFlags |= EF3_JETPACK_HOVER;
			Boba_FlyStart(self);
			self->client->ps.fd.forceJumpCharge = 280;
			self->client->jetPackTime = (self->client->jetPackTime + level.time) / 2 + 10000;
		}
		else
		{
			dodge_anim = Q_irand(BOTH_DODGE_L, BOTH_DODGE_R);
		}
		break;
	case HL_ARM_RT:
	case HL_HAND_RT:
		dodge_anim = BOTH_DODGE_L;
		break;
	case HL_ARM_LT:
	case HL_HAND_LT:
		dodge_anim = BOTH_DODGE_R;
		break;
	case HL_HEAD:
		dodge_anim = BOTH_CROUCHDODGE;
		break;
	default:
		return qfalse;
	}

	if (dodge_anim != -1)
	{
		if (self->client->ps.forceHandExtend != HANDEXTEND_DODGE && !PM_InKnockDown(&self->client->ps))
		{
			//do a simple dodge
			DoNormalDodge(self, dodge_anim);
		}
		else
		{
			//can't just do a simple dodge
			int rolled;
			vec3_t tangles;
			vec3_t forward;
			vec3_t right;
			vec3_t impactpoint;
			int x;
			int fall_check_max;

			VectorSubtract(dmg_origin, self->client->ps.origin, impactpoint);
			VectorNormalize(impactpoint);

			VectorSet(tangles, 0, self->client->ps.viewangles[YAW], 0);

			AngleVectors(tangles, forward, right, NULL);

			const float fdot = DotProduct(impactpoint, forward);
			const float rdot = DotProduct(impactpoint, right);

			if (PM_InKnockDown(&self->client->ps))
			{
				//ground dodge roll
				fall_check_max = 2;
				if (rdot < 0)
				{
					//Right
					if (self->client->ps.legsAnim == BOTH_KNOCKDOWN3
						|| self->client->ps.legsAnim == BOTH_KNOCKDOWN5
						|| self->client->ps.legsAnim == BOTH_LK_DL_ST_T_SB_1_L)
					{
						rolled = BOTH_GETUP_FROLL_R;
					}
					else
					{
						rolled = BOTH_GETUP_BROLL_R;
					}
				}
				else
				{
					//left
					if (self->client->ps.legsAnim == BOTH_KNOCKDOWN3
						|| self->client->ps.legsAnim == BOTH_KNOCKDOWN5
						|| self->client->ps.legsAnim == BOTH_LK_DL_ST_T_SB_1_L)
					{
						rolled = BOTH_GETUP_FROLL_L;
					}
					else
					{
						rolled = BOTH_GETUP_BROLL_L;
					}
				}
			}
			else
			{
				//normal Dodge rolls.
				fall_check_max = 4;
				if (fabs(fdot) > fabs(rdot))
				{
					//Forward/Back
					if (fdot < 0)
					{
						//Forward
						rolled = BOTH_HOP_F;
					}
					else
					{
						//Back
						rolled = BOTH_HOP_B;
					}
				}
				else
				{
					//Right/Left
					if (rdot < 0)
					{
						//Right
						rolled = BOTH_HOP_R;
					}
					else
					{
						//Left
						rolled = BOTH_HOP_L;
					}
				}
			}

			for (x = 0; x < fall_check_max; x++)
			{
				//check for a valid rolling direction..namely someplace where we
				//won't roll off cliffs
				if (DodgeRollCheck(self, rolled, forward, right))
				{
					//passed check
					break;
				}

				//otherwise, rotate to try the next possible direction
				//reset to start of possible moves if we're at the end of the list
				//for the perspective roll type.
				if (rolled == BOTH_GETUP_BROLL_R)
				{
					//there's only two possible evasions in this mode.
					rolled = BOTH_GETUP_BROLL_L;
				}
				else if (rolled == BOTH_GETUP_BROLL_L)
				{
					//there's only two possible evasions in this mode.
					rolled = BOTH_GETUP_BROLL_R;
				}
				else if (rolled == BOTH_GETUP_FROLL_R)
				{
					//there's only two possible evasions in this mode.
					rolled = BOTH_GETUP_FROLL_L;
				}
				else if (rolled == BOTH_GETUP_FROLL_L)
				{
					//there's only two possible evasions in this mode.
					rolled = BOTH_GETUP_FROLL_R;
				}
				else if (rolled == BOTH_ROLL_R)
				{
					//reset to start of the possible normal roll directions
					rolled = BOTH_ROLL_F;
				}
				else if (rolled == BOTH_HOP_R)
				{
					//reset to the start of possible hop directions
					rolled = BOTH_HOP_F;
				}
				else
				{
					//just advance the roll move
					rolled++;
				}
			}

			if (x == fall_check_max)
			{
				//we don't have a valid position to dodge roll to. just do a normal dodge
				DoNormalDodge(self, dodge_anim);
			}
			else
			{
				//ok, we can do the dodge hops/rolls
				if (PM_InKnockDown(&self->client->ps)
					|| BG_HopAnim(rolled))
				{
					//ground dodge roll
					G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, rolled,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
				}
				else
				{
					//normal dodge roll
					self->client->ps.legsTimer = 0;
					self->client->ps.legsAnim = 0;
					G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, rolled,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 150);
					G_AddEvent(self, EV_ROLL, 0);
					self->r.maxs[2] = self->client->ps.crouchheight;
					self->client->ps.viewheight = DEFAULT_VIEWHEIGHT;
					self->client->ps.pm_flags &= ~PMF_DUCKED;
					self->client->ps.pm_flags |= PMF_ROLLING;
				}

				//set the dodge roll flag
				self->client->ps.userInt3 |= 1 << FLAG_DODGEROLL;

				//set weapontime
				self->client->ps.weaponTime = self->client->ps.torsoTimer;

				//clear out the old dodge move.
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
				self->client->ps.forceDodgeAnim = 0;
				self->client->ps.forceHandExtendTime = 0;
				self->client->ps.saber_move = LS_NONE;
				G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/melee/swing4.wav"));
			}
		}
		return qtrue;
	}
	return qfalse;
}

static qboolean G_DoSaberDodge(gentity_t* dodger, gentity_t* attacker, vec3_t dmg_origin, int hit_loc, int* dmg, const int mod)
{
	int dodge_anim = -1;
	int dpcost = BasicWeaponBlockCosts[MOD_SABER];
	const int saved_dmg = *dmg;
	qboolean no_action = qfalse;

	if (in_camera)
	{
		return qfalse;
	}

	if (dodger->NPC &&
		(dodger->client->NPC_class == CLASS_STORMTROOPER
			|| dodger->client->NPC_class == CLASS_STORMCOMMANDO
			|| dodger->client->NPC_class == CLASS_SWAMPTROOPER
			|| dodger->client->NPC_class == CLASS_CLONETROOPER
			|| dodger->client->NPC_class == CLASS_IMPWORKER
			|| dodger->client->NPC_class == CLASS_IMPERIAL
			|| dodger->client->NPC_class == CLASS_SABER_DROID
			|| dodger->client->NPC_class == CLASS_ASSASSIN_DROID
			|| dodger->client->NPC_class == CLASS_GONK
			|| dodger->client->NPC_class == CLASS_MOUSE
			|| dodger->client->NPC_class == CLASS_PROBE
			|| dodger->client->NPC_class == CLASS_PROTOCOL
			|| dodger->client->NPC_class == CLASS_R2D2
			|| dodger->client->NPC_class == CLASS_R5D2
			|| dodger->client->NPC_class == CLASS_SEEKER
			|| dodger->client->NPC_class == CLASS_INTERROGATOR))
	{
		// don't get Dodge.
		return qfalse;
	}

	if (dpcost == -1)
	{
		//can't dodge this.
		return qfalse;
	}

	if (!dodger || !dodger->client || !dodger->inuse || dodger->health <= 0)
	{
		return qfalse;
	}

	//check for private duel conditions
	if (attacker && attacker->client)
	{
		if (attacker->client->ps.duelInProgress && attacker->client->ps.duelIndex != dodger->s.number)
		{
			//enemy client is in duel with someone else.
			return qfalse;
		}

		if (dodger->client->ps.duelInProgress && dodger->client->ps.duelIndex != attacker->s.number)
		{
			//we're in a duel with someone else.
			return qfalse;
		}
	}

	if (BG_HopAnim(dodger->client->ps.legsAnim) //in dodge hop
		|| BG_InRoll(&dodger->client->ps, dodger->client->ps.legsAnim)
		&& dodger->client->ps.userInt3 & 1 << FLAG_DODGEROLL //in dodge roll
		|| mod != MOD_SABER && dodger->client->ps.forceHandExtend == HANDEXTEND_DODGE)
	{
		//already doing a dodge
		return qtrue;
	}

	if (dodger->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//can't dodge direct fire in mid-air
		return qfalse;
	}

	if (PM_InKnockDown(&dodger->client->ps)
		|| BG_InRoll(&dodger->client->ps, dodger->client->ps.legsAnim)
		|| BG_InKnockDown(dodger->client->ps.legsAnim))
	{
		//can't Dodge while knocked down or getting up from knockdown.
		return qfalse;
	}

	if (dodger->client->ps.forceHandExtend == HANDEXTEND_CHOKE
		|| PM_InGrappleMove(dodger->client->ps.torsoAnim) > 1)
	{
		//in some effect that stops me from moving on my own
		return qfalse;
	}

	if (BG_IsAlreadyinTauntAnim(dodger->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (dodger->client->ps.legsAnim == BOTH_MEDITATE || dodger->client->ps.legsAnim == BOTH_MEDITATE_SABER)
	{//can't dodge while meditating.
		return qfalse;
	}

	if (dodger->client->ps.weapon == WP_SABER && dodger->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK)
	{
		return qfalse; //dont do if we are already blocking
	}

	if (dodger->client->ps.weapon == WP_SABER && PM_CrouchingAnim(dodger->client->ps.legsAnim))
	{
		return qfalse; //dont do if we are already blocking
	}

	if (!(dodger->client->pers.cmd.buttons & BUTTON_USE) && !(dodger->r.svFlags & SVF_BOT))
	{
		return qfalse;
	}

	if (dodger->r.svFlags & SVF_BOT //A bot
		&& dodger->client->ps.weapon == WP_SABER // with a saber
		&& (dodger->client->ps.fd.blockPoints <= FATIGUE_DODGEINGBOT ||
			dodger->client->ps.fd.forcePower <= FATIGUE_DODGEINGBOT ||
			dodger->client->ps.saberFatigueChainCount >= MISHAPLEVEL_HEAVY))// if your low then you cant dodge
	{
		return qfalse;
	}

	if ((dodger->r.svFlags & SVF_BOT
		&& dodger->client->ps.weapon != WP_SABER
		&& dodger->client->pers.botclass != BCLASS_MANDOLORIAN
		&& dodger->client->pers.botclass != BCLASS_MANDOLORIAN1
		&& dodger->client->pers.botclass != BCLASS_MANDOLORIAN2
		&& dodger->client->pers.botclass != BCLASS_BOBAFETT))// if your not a mando or jedi then you cant dodge
	{
		return qfalse;
	}

	if (dodger->r.svFlags & SVF_BOT
		&& dodger->client->pers.botclass == BCLASS_MANDOLORIAN
		&& dodger->client->pers.botclass == BCLASS_MANDOLORIAN1
		&& dodger->client->pers.botclass == BCLASS_MANDOLORIAN2
		&& dodger->client->pers.botclass == BCLASS_BOBAFETT && dodger->client->ps.fd.forcePower <= FATIGUE_DODGEINGBOT)// if your a mando low FP then you cant dodge
	{
		return qfalse;
	}

	if (dodger->client->ps.fd.forcePower < dpcost + FATIGUE_DODGEING && !(dodger->r.svFlags & SVF_BOT)) //player
	{
		//Not enough dodge
		return qfalse;
	}

	if (mod == MOD_SABER && attacker && attacker->client)
	{
		//special saber moves have special effects.
		if (attacker->client->ps.saber_move == LS_A_LUNGE
			|| attacker->client->ps.saber_move == LS_SPINATTACK
			|| attacker->client->ps.saber_move == LS_SPINATTACK_DUAL
			|| attacker->client->ps.saber_move == LS_SPINATTACK_GRIEV)
		{
			//attacker is doing lunge special
			if (dodger->client->ps.userInt3 & 1 << FLAG_FATIGUED)
			{
				//can't dodge a lunge special while fatigued
				return qfalse;
			}
		}

		if (PM_SuperBreakWinAnim(attacker->client->ps.torsoAnim) && dodger->client->ps.fd.forcePower < 50)
		{
			//can't block super breaks if we're low on DP.
			return qfalse;
		}

		if (!walk_check(dodger)
			|| PM_SaberInAttack(dodger->client->ps.saber_move)
			|| PM_SaberInStart(dodger->client->ps.saber_move))
		{
			return qfalse;
		}
	}

	if (*dmg <= DODGE_MINDAM && mod == MOD_SABER)
	{
		dpcost = (int)(dpcost * ((float)*dmg / DODGE_MINDAM));
		no_action = qtrue;
	}
	else if (dodger->client->ps.forceHandExtend == HANDEXTEND_DODGE)
	{
		//you're already dodging but you got hit again.
		dpcost = 0;
	}

	if (dodger->r.svFlags & SVF_BOT) //bots only get 1
	{
		PM_AddFatigue(&dodger->client->ps, FATIGUE_DODGEINGBOT);
	}
	else
	{
		PM_AddFatigue(&dodger->client->ps, dpcost);
	}

	if (no_action)
	{
		return qtrue;
	}

	if (mod == MOD_REPEATER_ALT_SPLASH
		|| mod == MOD_FLECHETTE_ALT_SPLASH
		|| mod == MOD_ROCKET_SPLASH
		|| mod == MOD_ROCKET_HOMING_SPLASH
		|| mod == MOD_THERMAL_SPLASH
		|| mod == MOD_TRIP_MINE_SPLASH
		|| mod == MOD_TIMED_MINE_SPLASH
		|| mod == MOD_DET_PACK_SPLASH
		|| mod == MOD_ROCKET)
	{
		//splash damage dodge, dodged by throwing oneself away from the blast into a knockdown
		vec3_t blow_back_dir;
		int blow_back_power = saved_dmg;
		VectorSubtract(dodger->client->ps.origin, dmg_origin, blow_back_dir);
		VectorNormalize(blow_back_dir);

		if (blow_back_power > 1000)
		{
			//clamp blow back power level
			blow_back_power = 1000;
		}
		blow_back_power *= 2;
		g_throw(dodger, blow_back_dir, blow_back_power);
		G_Knockdown(dodger, attacker, blow_back_dir, 600, qtrue);
		return qtrue;
	}

	/*===========================================================================
	doing a positional dodge for direct hit damage (like sabers or blaster bolts)
	===========================================================================*/
	if (hit_loc == -1)
	{
		//Use the last surface impact data as the hit location
		if (d_saberGhoul2Collision.integer && dodger->client
			&& dodger->client->g2LastSurfaceModel == G2MODEL_PLAYER
			&& dodger->client->g2LastSurfaceTime == level.time)
		{
			char hit_surface[MAX_QPATH];

			trap->G2API_GetSurfaceName(dodger->ghoul2, dodger->client->g2LastSurfaceHit, 0, hit_surface);

			if (hit_surface[0])
			{
				G_GetHitLocFromSurfName(dodger, hit_surface, &hit_loc, dmg_origin, vec3_origin, vec3_origin, MOD_SABER);
			}
		}
		else
		{
			//ok, that didn't work.  Try the old math way.
			hit_loc = G_GetHitLocation(dodger, dmg_origin);
		}
	}

	switch (hit_loc)
	{
	case HL_NONE:
		return qfalse;

	case HL_FOOT_RT:
	case HL_FOOT_LT:
		dodge_anim = Q_irand(BOTH_HOP_L, BOTH_HOP_R);
		break;
	case HL_LEG_RT:
	case HL_LEG_LT:
		if (dodger->client->pers.botclass == BCLASS_MANDOLORIAN
			|| dodger->client->pers.botclass == BCLASS_BOBAFETT
			|| dodger->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| dodger->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			dodger->client->jetPackOn = qtrue;
			dodger->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			dodger->client->ps.eFlags |= EF_JETPACK_FLAMING;
			dodger->client->ps.eFlags |= EF3_JETPACK_HOVER;
			Boba_FlyStart(dodger);
			dodger->client->ps.fd.forceJumpCharge = 280;
			dodger->client->jetPackTime = level.time + 30000;
		}
		else
		{
			dodge_anim = Q_irand(BOTH_HOP_L, BOTH_HOP_R);
		}
		break;

	case HL_BACK_RT:
		dodge_anim = BOTH_DODGE_FL;
		break;
	case HL_CHEST_RT:
		dodge_anim = BOTH_DODGE_FR;
		break;
	case HL_BACK_LT:
		dodge_anim = BOTH_DODGE_FR;
		break;
	case HL_CHEST_LT:
		dodge_anim = BOTH_DODGE_FR;
		break;
	case HL_BACK:
		if (dodger->client->pers.botclass == BCLASS_MANDOLORIAN
			|| dodger->client->pers.botclass == BCLASS_BOBAFETT
			|| dodger->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| dodger->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			dodger->client->jetPackOn = qtrue;
			dodger->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			dodger->client->ps.eFlags |= EF_JETPACK_FLAMING;
			dodger->client->ps.eFlags |= EF3_JETPACK_HOVER;
			Boba_FlyStart(dodger);
			dodger->client->ps.fd.forceJumpCharge = 280;
			dodger->client->jetPackTime = (dodger->client->jetPackTime + level.time) / 2 + 10000;
		}
		else
		{
			dodge_anim = Q_irand(BOTH_DODGE_FL, BOTH_DODGE_FR);
		}
		break;
	case HL_CHEST:
		if (dodger->client->pers.botclass == BCLASS_MANDOLORIAN
			|| dodger->client->pers.botclass == BCLASS_BOBAFETT
			|| dodger->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| dodger->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			dodger->client->jetPackOn = qtrue;
			dodger->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			dodger->client->ps.eFlags |= EF_JETPACK_FLAMING;
			dodger->client->ps.eFlags |= EF3_JETPACK_HOVER;
			Boba_FlyStart(dodger);
			dodger->client->ps.fd.forceJumpCharge = 280;
			dodger->client->jetPackTime = (dodger->client->jetPackTime + level.time) / 2 + 10000;
		}
		else
		{
			dodge_anim = BOTH_DODGE_B;
		}
		break;
	case HL_WAIST:
		if (dodger->client->pers.botclass == BCLASS_MANDOLORIAN
			|| dodger->client->pers.botclass == BCLASS_BOBAFETT
			|| dodger->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| dodger->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			dodger->client->jetPackOn = qtrue;
			dodger->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			dodger->client->ps.eFlags |= EF_JETPACK_FLAMING;
			dodger->client->ps.eFlags |= EF3_JETPACK_HOVER;
			Boba_FlyStart(dodger);
			dodger->client->ps.fd.forceJumpCharge = 280;
			dodger->client->jetPackTime = (dodger->client->jetPackTime + level.time) / 2 + 10000;
		}
		else
		{
			dodge_anim = Q_irand(BOTH_DODGE_L, BOTH_DODGE_R);
		}
		break;
	case HL_ARM_RT:
	case HL_HAND_RT:
		dodge_anim = BOTH_DODGE_L;
		break;
	case HL_ARM_LT:
	case HL_HAND_LT:
		dodge_anim = BOTH_DODGE_R;
		break;
	case HL_HEAD:
		dodge_anim = BOTH_CROUCHDODGE;
		break;
	default:
		return qfalse;
	}

	if (dodge_anim != -1)
	{
		if (dodger->client->ps.forceHandExtend != HANDEXTEND_DODGE && !PM_InKnockDown(&dodger->client->ps))
		{
			//do a simple dodge
			DoNormalDodge(dodger, dodge_anim);
		}
		else
		{
			//can't just do a simple dodge
			int rolled;
			vec3_t tangles;
			vec3_t forward;
			vec3_t right;
			vec3_t impactpoint;
			int x;
			int fall_check_max;

			VectorSubtract(dmg_origin, dodger->client->ps.origin, impactpoint);
			VectorNormalize(impactpoint);

			VectorSet(tangles, 0, dodger->client->ps.viewangles[YAW], 0);

			AngleVectors(tangles, forward, right, NULL);

			const float fdot = DotProduct(impactpoint, forward);
			const float rdot = DotProduct(impactpoint, right);

			if (PM_InKnockDown(&dodger->client->ps))
			{
				//ground dodge roll
				fall_check_max = 2;
				if (rdot < 0)
				{
					//Right
					if (dodger->client->ps.legsAnim == BOTH_KNOCKDOWN3
						|| dodger->client->ps.legsAnim == BOTH_KNOCKDOWN5
						|| dodger->client->ps.legsAnim == BOTH_LK_DL_ST_T_SB_1_L)
					{
						rolled = BOTH_GETUP_FROLL_R;
					}
					else
					{
						rolled = BOTH_GETUP_BROLL_R;
					}
				}
				else
				{
					//left
					if (dodger->client->ps.legsAnim == BOTH_KNOCKDOWN3
						|| dodger->client->ps.legsAnim == BOTH_KNOCKDOWN5
						|| dodger->client->ps.legsAnim == BOTH_LK_DL_ST_T_SB_1_L)
					{
						rolled = BOTH_GETUP_FROLL_L;
					}
					else
					{
						rolled = BOTH_GETUP_BROLL_L;
					}
				}
			}
			else
			{
				//normal Dodge rolls.
				fall_check_max = 4;
				if (fabs(fdot) > fabs(rdot))
				{
					//Forward/Back
					if (fdot < 0)
					{
						//Forward
						rolled = BOTH_HOP_F;
					}
					else
					{
						//Back
						rolled = BOTH_HOP_B;
					}
				}
				else
				{
					//Right/Left
					if (rdot < 0)
					{
						//Right
						rolled = BOTH_HOP_R;
					}
					else
					{
						//Left
						rolled = BOTH_HOP_L;
					}
				}
			}

			for (x = 0; x < fall_check_max; x++)
			{
				//check for a valid rolling direction..namely someplace where we
				//won't roll off cliffs
				if (DodgeRollCheck(dodger, rolled, forward, right))
				{
					//passed check
					break;
				}

				//otherwise, rotate to try the next possible direction
				//reset to start of possible moves if we're at the end of the list
				//for the perspective roll type.
				if (rolled == BOTH_GETUP_BROLL_R)
				{
					//there's only two possible evasions in this mode.
					rolled = BOTH_GETUP_BROLL_L;
				}
				else if (rolled == BOTH_GETUP_BROLL_L)
				{
					//there's only two possible evasions in this mode.
					rolled = BOTH_GETUP_BROLL_R;
				}
				else if (rolled == BOTH_GETUP_FROLL_R)
				{
					//there's only two possible evasions in this mode.
					rolled = BOTH_GETUP_FROLL_L;
				}
				else if (rolled == BOTH_GETUP_FROLL_L)
				{
					//there's only two possible evasions in this mode.
					rolled = BOTH_GETUP_FROLL_R;
				}
				else if (rolled == BOTH_ROLL_R)
				{
					//reset to start of the possible normal roll directions
					rolled = BOTH_ROLL_F;
				}
				else if (rolled == BOTH_HOP_R)
				{
					//reset to the start of possible hop directions
					rolled = BOTH_HOP_F;
				}
				else
				{
					//just advance the roll move
					rolled++;
				}
			}

			if (x == fall_check_max)
			{
				//we don't have a valid position to dodge roll to. just do a normal dodge
				DoNormalDodge(dodger, dodge_anim);
			}
			else
			{
				//ok, we can do the dodge hops/rolls
				if (PM_InKnockDown(&dodger->client->ps)
					|| BG_HopAnim(rolled))
				{
					//ground dodge roll
					G_SetAnim(dodger, &dodger->client->pers.cmd, SETANIM_BOTH, rolled,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
				}
				else
				{
					//normal dodge roll
					dodger->client->ps.legsTimer = 0;
					dodger->client->ps.legsAnim = 0;
					G_SetAnim(dodger, &dodger->client->pers.cmd, SETANIM_BOTH, rolled,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 150);
					G_AddEvent(dodger, EV_ROLL, 0);
					dodger->r.maxs[2] = dodger->client->ps.crouchheight;
					dodger->client->ps.viewheight = DEFAULT_VIEWHEIGHT;
					dodger->client->ps.pm_flags &= ~PMF_DUCKED;
					dodger->client->ps.pm_flags |= PMF_ROLLING;
				}

				//set the dodge roll flag
				dodger->client->ps.userInt3 |= 1 << FLAG_DODGEROLL;

				//set weapontime
				dodger->client->ps.weaponTime = dodger->client->ps.torsoTimer;

				//clear out the old dodge move.
				dodger->client->ps.forceHandExtend = HANDEXTEND_NONE;
				dodger->client->ps.forceDodgeAnim = 0;
				dodger->client->ps.forceHandExtendTime = 0;
				dodger->client->ps.saber_move = LS_NONE;
				G_Sound(dodger, CHAN_BODY, G_SoundIndex("sound/weapons/melee/swing4.wav"));
			}
		}
		return qtrue;
	}
	return qfalse;
}

static int hit_loc[MAX_SABER_VICTIMS];
static vec3_t saberHitLocation;
static qboolean hitDismember[MAX_SABER_VICTIMS];
static int hitDismemberLoc[MAX_SABER_VICTIMS];

void wp_saber_clear_damage_for_ent_num(gentity_t* attacker, const int entityNum, const int saberNum,
	const int blade_num)
{
	float knock_back_scale = 0.0f;
	if (attacker && attacker->client)
	{
		if (!WP_SaberBladeUseSecondBladeStyle(&attacker->client->saber[saberNum], blade_num)
			&& attacker->client->saber[saberNum].knockbackScale > 0.0f)
		{
			knock_back_scale = attacker->client->saber[saberNum].knockbackScale;
		}
		else if (WP_SaberBladeUseSecondBladeStyle(&attacker->client->saber[saberNum], blade_num)
			&& attacker->client->saber[saberNum].knockbackScale2 > 0.0f)
		{
			knock_back_scale = attacker->client->saber[saberNum].knockbackScale2;
		}
	}

	for (int i = 0; i < numVictims; i++)
	{
		if (victimentity_num[i] == entityNum)
		{
			//hold on a sec, let's still do any accumulated knockback
			if (knock_back_scale)
			{
				gentity_t* victim = &g_entities[victimentity_num[i]];
				if (victim && victim->client)
				{
					vec3_t center, dir_to_center;
					float knock_down_thresh_hold;
					const float knockback = knock_back_scale * totalDmg[i] * 0.5f;

					VectorAdd(victim->r.absmin, victim->r.absmax, center);
					VectorScale(center, 0.5, center);
					VectorSubtract(victim->r.currentOrigin, saberHitLocation, dir_to_center);
					VectorNormalize(dir_to_center);
					g_throw(victim, dir_to_center, knockback);
					if (victim->client->ps.groundEntityNum != ENTITYNUM_NONE
						&& dir_to_center[2] <= 0)
					{
						//hit downward on someone who is standing on firm ground, so more likely to knock them down
						knock_down_thresh_hold = Q_irand(25, 50);
					}
					else
					{
						knock_down_thresh_hold = Q_irand(75, 125);
					}

					if (knockback > knock_down_thresh_hold)
					{
						G_Knockdown(victim, attacker, dir_to_center, 350, qtrue);
					}
				}
			}
			//now clear everything
			totalDmg[i] = 0; //no damage
			hit_loc[i] = HL_NONE;
			hitDismemberLoc[i] = HL_NONE;
			hitDismember[i] = qfalse;
			victimentity_num[i] = ENTITYNUM_NONE; //like we never hit him
		}
	}
}

static QINLINE qboolean CheckSaberDamage(gentity_t* self, const int rSaberNum, const int rBladeNum,
	vec3_t saber_start, vec3_t saber_end, const int trMask)
{
	static trace_t tr;
	static vec3_t dir;
	static vec3_t saber_tr_mins, saber_tr_maxs;
	static int self_saber_level;
	int dmg;
	qboolean idle_damage = qfalse;
	qboolean did_hit = qfalse;
	qboolean passthru = qfalse;
	int sabimpactdebounce;
	int sabimpactentity_num;
	qboolean saber_hit_wall = qfalse;
	gentity_t* blocker = NULL;

	float saber_box_size = d_saberBoxTraceSize.value;
	const float hilt_radius = self->client->saber[rSaberNum].blade[rBladeNum].radius * 1.2f;
	const qboolean saber_in_kill_move = PM_SaberInKillMove(self->client->ps.saber_move);

	const qboolean self_is_holding_block_button = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking
	const qboolean self_active_blocking = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK
		? qtrue
		: qfalse; //Active Blocking
	const qboolean self_m_blocking = self->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING ? qtrue : qfalse;
	//Perfect Blocking

	if (BG_SabersOff(&self->client->ps) || !self->client->ps.saberEntityNum && self->client->ps.fd.saber_anim_levelBase
		!= SS_DUAL)
	{
		// register as a hit so we don't do a lot of interpolation.
		self->client->ps.saberBlocked = BLOCKED_NONE;
		return qtrue;
	}

	if (!self->s.number)
	{
		//player never uses these
		if (PM_SaberInAttack(self->client->ps.saber_move))
		{
			//make sure code knows player is attacking and not doing any kind of block
			self->client->ps.saberEventFlags &= ~SEF_PARRIED;
			self->client->ps.saberEventFlags &= ~SEF_DEFLECTED;
		}
		self->client->ps.saberEventFlags &= ~SEF_EVENTS;
	}

	self_saber_level = g_saber_attack_power(self, SaberAttacking(self));

	/////////////////////////////SABERBLADE BOX SIZE///////////////////////////////////////
	if (self->client->ps.weaponTime <= 0) //
	{
		//
		VectorClear(saber_tr_mins); //
		VectorClear(saber_tr_maxs); //
	} //
	else if (self_is_holding_block_button || self_active_blocking || self_m_blocking || //
		PM_SaberInFullDamageMove(&self->client->ps, self->localAnimIndex)) //
	{
		//Setting things up so the game always does realistic box traces for the sabers.    //
		saber_box_size += (d_saberBoxTraceSize.value + hilt_radius * 0.5f) * 3.0f; //
		//
		VectorSet(saber_tr_mins, -saber_box_size, -saber_box_size, -saber_box_size); //
		VectorSet(saber_tr_maxs, saber_box_size * 3, saber_box_size * 3, saber_box_size * 4); //
	} //
	else //
	{
		//
		saber_box_size += self->client->saber[rSaberNum].blade[rBladeNum].radius * 0.5f; //
		//
		VectorSet(saber_tr_mins, -saber_box_size, -saber_box_size, -saber_box_size); //
		VectorSet(saber_tr_maxs, saber_box_size * 3, saber_box_size * 3, saber_box_size * 3); //
	} //
	/////////////////////////////SABERBLADE BOX SIZE///////////////////////////////////////

	const int real_trace_result = g_real_trace(self, &tr, saber_start, saber_tr_mins, saber_tr_maxs, saber_end,
		self->s.number, trMask, rSaberNum, rBladeNum);

	if (real_trace_result == REALTRACE_MISS || real_trace_result == REALTRACE_HIT_WORLD)
	{
		self->client->ps.saberBlocked = BLOCKED_NONE;
		return qtrue;
	}

	if (tr.fraction == 1 && !tr.startsolid)
	{
		return qfalse;
	}

	if (self->client->ps.saberInFlight && rSaberNum == 0 && self->client->ps.saberAttackWound < level.time)
	{
		dmg = SABER_NORHITDAMAGE;
	}
	else if (PM_SaberInFullDamageMove(&self->client->ps, self->localAnimIndex))
	{
		//full damage moves
		if (g_saberRealisticCombat.integer)
		{
			if (g_saberRealisticCombat.integer > 1)
			{
				if (g_saberRealisticCombat.integer > 2)
				{
					if (PM_kick_move(self->client->ps.saber_move)
						|| PM_KickingAnim(self->client->ps.legsAnim)
						|| PM_KickingAnim(self->client->ps.torsoAnim))
					{
						//use no saber damage for kick moves.
						dmg = SABER_NO_DAMAGE;
					}
					else if (saber_in_kill_move)
					{
						dmg = SABER_MAXHITDAMAGE;
					}
					else if (self->client->ps.saber_move == LS_PULL_ATTACK_STAB)
					{
						dmg = SABER_NORHITDAMAGE;
					}
					else
					{
						if (g_saberdebug.integer)
						{
							dmg = SABER_DEBUGTDAMAGE;
						}
						else
						{
							dmg = SABER_MAXHITDAMAGE;
						}
					}
				}
				else
				{
					if (PM_kick_move(self->client->ps.saber_move)
						|| PM_KickingAnim(self->client->ps.legsAnim)
						|| PM_KickingAnim(self->client->ps.torsoAnim))
					{
						//use no saber damage for kick moves.
						dmg = SABER_NO_DAMAGE;
					}
					else if (saber_in_kill_move)
					{
						dmg = SABER_MAXHITDAMAGE;
					}
					else if (self->client->ps.saber_move == LS_PULL_ATTACK_STAB)
					{
						dmg = SABER_NORHITDAMAGE;
					}
					else
					{
						if (g_saberdebug.integer)
						{
							dmg = SABER_DEBUGTDAMAGE;
						}
						else
						{
							dmg = SABER_SJEHITDAMAGE;
						}
					}
				}
			}
			else
			{
				if (PM_kick_move(self->client->ps.saber_move)
					|| PM_KickingAnim(self->client->ps.legsAnim)
					|| PM_KickingAnim(self->client->ps.torsoAnim))
				{
					//use no saber damage for kick moves.
					dmg = SABER_NO_DAMAGE;
				}
				else if (saber_in_kill_move)
				{
					dmg = SABER_MAXHITDAMAGE;
				}
				else if (self->client->ps.saber_move == LS_PULL_ATTACK_STAB)
				{
					dmg = SABER_NORHITDAMAGE;
				}
				else
				{
					if (g_saberdebug.integer)
					{
						dmg = SABER_DEBUGTDAMAGE;
					}
					else
					{
						dmg = SABER_NORHITDAMAGE;
					}
				}
			}
		}
		else
		{
			if (PM_kick_move(self->client->ps.saber_move)
				|| PM_KickingAnim(self->client->ps.legsAnim)
				|| PM_KickingAnim(self->client->ps.torsoAnim))
			{
				//use no saber damage for kick moves.
				dmg = SABER_NO_DAMAGE;
			}
			else if (saber_in_kill_move)
			{
				dmg = SABER_MAXHITDAMAGE;
			}
			else if (self->client->ps.saber_move == LS_PULL_ATTACK_STAB)
			{
				dmg = SABER_NORHITDAMAGE;
			}
			else
			{
				if (g_saberdebug.integer)
				{
					dmg = SABER_DEBUGTDAMAGE;
				}
				else
				{
					dmg = SABER_MINHITDAMAGE;
				}
			}
		}
	}
	else if (BG_SaberInTransitionDamageMove(&self->client->ps))
	{
		//use idle damage for transition moves.
		dmg = SABER_NONATTACK_DAMAGE;
		idle_damage = qtrue;
	}
	else if ((self_is_holding_block_button || self_active_blocking || self_m_blocking || self->client->ps.saberManualBlockingTime > level.time) && !(self->r.svFlags & SVF_BOT))
	{
		//Dont do damage if holding block.
		dmg = SABER_NO_DAMAGE;
		idle_damage = qtrue;
	}
	else
	{
		//idle saber damage
		dmg = SABER_NONATTACK_DAMAGE;
		idle_damage = qtrue;
	}

	if (!dmg)
	{
		if (tr.entityNum < MAX_CLIENTS ||
			g_entities[tr.entityNum].inuse && g_entities[tr.entityNum].r.contents & CONTENTS_LIGHTSABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (dmg > SABER_NONATTACK_DAMAGE)
	{
		dmg *= g_saberDamageScale.value;

		//see if this specific saber has a damage scale
		if (!WP_SaberBladeUseSecondBladeStyle(&self->client->saber[rSaberNum], rBladeNum)
			&& self->client->saber[rSaberNum].damageScale != 1.0f)
		{
			dmg = ceil((float)dmg * self->client->saber[rSaberNum].damageScale);
		}
		else if (WP_SaberBladeUseSecondBladeStyle(&self->client->saber[rSaberNum], rBladeNum)
			&& self->client->saber[rSaberNum].damageScale2 != 1.0f)
		{
			dmg = ceil((float)dmg * self->client->saber[rSaberNum].damageScale2);
		}

		if (self->client->ps.brokenLimbs & 1 << BROKENLIMB_RARM ||
			self->client->ps.brokenLimbs & 1 << BROKENLIMB_LARM)
		{
			//weaken it if an arm is broken
			dmg *= 0.3;
			if (dmg <= SABER_NONATTACK_DAMAGE)
			{
				dmg = SABER_NONATTACK_DAMAGE + 1;
			}
		}
	}

	if (dmg > SABER_NONATTACK_DAMAGE && self->client->ps.isJediMaster)
	{
		//give the Jedi Master more saber attack power
		dmg *= 2;
	}

	if (dmg > SABER_NONATTACK_DAMAGE && level.gametype == GT_SIEGE &&
		self->client->siegeClass != -1 && bgSiegeClasses[self->client->siegeClass].classflags & 1 <<
		CFL_MORESABERDMG)
	{
		//this class is flagged to do extra saber damage. I guess 2x will do for now.
		dmg *= 2;
	}

	VectorSubtract(saber_end, saber_start, dir);
	VectorNormalize(dir);

	if (tr.entityNum == ENTITYNUM_WORLD || g_entities[tr.entityNum].s.eType == ET_TERRAIN)
	{
		//register this as a wall hit for jedi AI
		self->client->ps.saberEventFlags |= SEF_HITWALL;
		saber_hit_wall = qtrue;
	}

	if (g_entities[tr.entityNum].takedamage &&
		(g_entities[tr.entityNum].health > 0 || !(g_entities[tr.entityNum].s.eFlags & EF_DISINTEGRATION)) &&
		tr.entityNum != self->s.number &&
		g_entities[tr.entityNum].inuse)
	{
		//attack has hit something that had health and takes damage
		blocker = &g_entities[tr.entityNum];

		if (g_entities[tr.entityNum].client &&
			g_entities[tr.entityNum].client->ps.duelInProgress &&
			g_entities[tr.entityNum].client->ps.duelIndex != self->s.number)
		{
			return qfalse;
		}

		if (g_entities[tr.entityNum].client &&
			self->client->ps.duelInProgress &&
			self->client->ps.duelIndex != g_entities[tr.entityNum].s.number)
		{
			return qfalse;
		}

		if (wp_saber_must_block(blocker, self, qfalse, tr.endpos, -1, -1))
		{
			if ((d_blockinfo.integer || g_DebugSaberCombat.integer) && !(blocker->r.svFlags & SVF_BOT))
			{
				Com_Printf(S_COLOR_CYAN"Players Blocking starts here 1\n");
			}
			//hit victim is able to block, block!
			did_hit = qfalse;

			//make me parry	-(Im the blocker)
			sab_beh_block_vs_attack(blocker, self, rSaberNum, rBladeNum, tr.endpos);

			//make me bounce -(Im the attacker)
			sab_beh_attack_vs_block(self, blocker, rSaberNum, rBladeNum, tr.endpos);
		}
		else
		{
			did_hit = qtrue;
			blocker = NULL;
		}
	}
	else if (g_entities[tr.entityNum].r.contents & CONTENTS_LIGHTSABER &&
		g_entities[tr.entityNum].r.contents != -1 &&
		g_entities[tr.entityNum].inuse)
	{
		//hit a saber blade
		blocker = &g_entities[g_entities[tr.entityNum].r.ownerNum];

		if (!blocker->inuse || !blocker->client)
		{
			//Bad defender saber owner state
			return qfalse;
		}

		if (blocker->client->ps.duelInProgress &&
			blocker->client->ps.duelIndex != self->s.number)
		{
			return qfalse;
		}

		if (self->client->ps.duelInProgress &&
			self->client->ps.duelIndex != blocker->s.number)
		{
			return qfalse;
		}

		if (blocker->client->ps.saberInFlight && !wp_using_dual_saber_as_primary(&blocker->client->ps))
		{
			//Hit a thrown saber, deactivate it.
			saberCheckKnockdown_Smashed(&g_entities[tr.entityNum], blocker, self, dmg);
			blocker = NULL;
		}
		else if (real_trace_result == REALTRACE_SABERBLOCKHIT || real_trace_result == REALTRACE_HIT)
		{
			//this is actually a faked lightsaber hit to make the bounding box saber blocking work.
			//As such, we know that the player can block, set the appropriate block position for this attack.
			if (wp_saber_must_block(blocker, self, qfalse, tr.endpos, -1, -1))
			{
				if ((d_blockinfo.integer || g_DebugSaberCombat.integer) && !(blocker->r.svFlags & SVF_BOT))
				{
					Com_Printf(S_COLOR_CYAN"REALTRACE Blocking starts here 1\n");
				}
				//hit victim is able to block, block!
				did_hit = qfalse;

				//make me parry	-(Im the blocker)
				sab_beh_block_vs_attack(blocker, self, rSaberNum, rBladeNum, tr.endpos);

				//make me bounce -(Im the attacker)
				sab_beh_attack_vs_block(self, blocker, rSaberNum, rBladeNum, tr.endpos);
			}
			else
			{
				did_hit = qtrue;
				blocker = NULL;
			}
		}
	}
	else if (real_trace_result == REALTRACE_HIT_WORLD)
	{
		// Hit the world, we only want efx...
		did_hit = qtrue;
		blocker = NULL;
	}
	else if (real_trace_result == REALTRACE_MISS)
	{
		// Hit nothing...
		did_hit = qfalse;
		blocker = NULL;
	}

	//saber impact debounce stuff
	if (idle_damage)
	{
		sabimpactdebounce = g_saberDmgDelay_Idle.integer;
	}
	else
	{
		sabimpactdebounce = g_saberDmgDelay_Wound.integer;
	}

	if (blocker)
	{
		sabimpactentity_num = blocker->client->ps.saberEntityNum;
	}
	else
	{
		sabimpactentity_num = tr.entityNum;
	}

	if (self->client->sabimpact[rSaberNum][rBladeNum].entityNum == sabimpactentity_num
		&& level.time - self->client->sabimpact[rSaberNum][rBladeNum].Debounce < sabimpactdebounce)
	{
		//the impact debounce for this entity isn't up yet.
		if (self->client->sabimpact[rSaberNum][rBladeNum].blade_num != -1
			|| self->client->sabimpact[rSaberNum][rBladeNum].saberNum != -1)
		{
			//the last impact was a saber on this entity
			if (blocker)
			{
				if (self->client->sabimpact[rSaberNum][rBladeNum].blade_num == self->client->lastBladeCollided
					&& self->client->sabimpact[rSaberNum][rBladeNum].saberNum == self->client->lastSaberCollided)
				{
					return qtrue;
				}
			}
		}
		else
		{
			//last impact was this saber.
			return qtrue;
		}
	}
	//end saber impact debounce stuff

	if (blocker)
	{
		//Do the saber clash effects here now.
		saberDoClashEffect = qtrue;
		VectorCopy(tr.endpos, saberClashPos);
		VectorCopy(tr.plane.normal, saberClashNorm);
		saberClashEventParm = 1;
		if (!idle_damage
			|| BG_SaberInNonIdleDamageMove(&blocker->client->ps, blocker->localAnimIndex))
		{
			//only do view locks if one player or the other is in an attack move
			saberClashOther = blocker->s.number;
		}
		else
		{
			//make the saberClashOther be invalid
			saberClashOther = -1;
		}
	}

	if (did_hit && (!OnSameTeam(self, &g_entities[tr.entityNum]) || g_friendlySaber.integer))
	{
		//deal damage
		//damage the thing we hit
		int dflags = 0;

		gentity_t* victim = &g_entities[tr.entityNum];
		const int index = Q_irand(1, 3);

		if (dmg <= SABER_NONATTACK_DAMAGE || (G_DoSaberDodge(victim, self, tr.endpos, -1, &dmg, MOD_SABER)))
		{
			self->client->ps.saberIdleWound = level.time + 350;

			//add saber impact debounce
			DebounceSaberImpact(self, blocker, rSaberNum, rBladeNum, sabimpactentity_num);
			return qtrue;
		}

		if (victim->flags & FL_SABERDAMAGE_RESIST && (!Q_irand(0, 1)))
		{
			dflags |= DAMAGE_NO_DAMAGE;
			G_Beskar_Attack_Bounce(self, victim);
			G_Sound(victim, CHAN_BODY, G_SoundIndex(va("sound/weapons/impacts/beskar_impact%d.mp3", index)));
		}
		//hit!
		//determine if this saber blade does dismemberment or not.
		if (!WP_SaberBladeUseSecondBladeStyle(&self->client->saber[rSaberNum], rBladeNum)
			&& !(self->client->saber[rSaberNum].saberFlags2 & SFL2_NO_DISMEMBERMENT))
		{
			dflags |= DAMAGE_NO_DISMEMBER;
		}
		if (WP_SaberBladeUseSecondBladeStyle(&self->client->saber[rSaberNum], rBladeNum)
			&& !(self->client->saber[rSaberNum].saberFlags2 & SFL2_NO_DISMEMBERMENT2))
		{
			dflags |= DAMAGE_NO_DISMEMBER;
		}

		if (!WP_SaberBladeUseSecondBladeStyle(&self->client->saber[rSaberNum], rBladeNum)
			&& self->client->saber[rSaberNum].knockbackScale > 0.0f)
		{
			if (rSaberNum < 1)
			{
				dflags |= DAMAGE_SABER_KNOCKBACK1;
			}
			else
			{
				dflags |= DAMAGE_SABER_KNOCKBACK2;
			}
		}

		if (WP_SaberBladeUseSecondBladeStyle(&self->client->saber[rSaberNum], rBladeNum)
			&& self->client->saber[rSaberNum].knockbackScale > 0.0f)
		{
			if (rSaberNum < 1)
			{
				dflags |= DAMAGE_SABER_KNOCKBACK1_B2;
			}
			else
			{
				dflags |= DAMAGE_SABER_KNOCKBACK2_B2;
			}
		}

		//I don't see the purpose of wall damage scaling...but oh well. :)
		if (!victim->client)
		{
			float damage = dmg;
			damage *= g_saberWallDamageScale.value;
			dmg = (int)damage;
		}

		if (dmg >= SABER_TRANSITION_DAMAGE)
		{
			if (!(self->r.svFlags & SVF_BOT))
			{
				if (victim->health >= 1)
				{
					CGCam_BlockShakeMP(self->s.origin, self, 0.25f, 100);

					G_SaberBounce(self, victim);
				}
			}
		}

		//We need the final damage total to know if we need to bounce the saber back or not.
		G_Damage(victim, self, self, dir, tr.endpos, dmg, dflags, MOD_SABER);
		wp_saber_specific_do_hit(self, rSaberNum, rBladeNum, victim, tr.endpos, dmg);

		if (victim->health <= 0)
		{
			//The attack killed your opponent, don't bounce the saber back to prevent nasty passthru.
			passthru = qtrue;
		}

		if (victim->client)
		{
			//Let jedi AI know if it hit an enemy
			if (self->enemy && self->enemy == victim)
			{
				self->client->ps.saberEventFlags |= SEF_HITENEMY;
			}
			else
			{
				self->client->ps.saberEventFlags |= SEF_HITOBJECT;
			}
		}
	}

	if (self->client->ps.saberInFlight && !wp_using_dual_saber_as_primary(&self->client->ps))
	{
		//saber in flight and it has hit something.  deactivate it.
		saberCheckKnockdown_Smashed(&g_entities[self->client->ps.saberEntityNum], self, blocker, dmg);
	}
	else
	{
		//Didn't go into a special animation, so do a saber lock/bounce/deflection/etc that's appropriate
		if (blocker && blocker->inuse && blocker->client)
		{
			//saberlock test
			if (dmg > SABER_NONATTACK_DAMAGE ||
				BG_SaberInNonIdleDamageMove(&blocker->client->ps, blocker->localAnimIndex))
			{
				const int lock_factor = g_saberLockFactor.integer;

				if (Q_irand(1, 20) < lock_factor)
				{
					if (WP_SabersCheckLock(self, blocker))
					{
						self->client->ps.userInt3 |= 1 << FLAG_SABERLOCK_ATTACKER;
						self->client->ps.saberBlocked = BLOCKED_NONE;
						blocker->client->ps.saberBlocked = BLOCKED_NONE;
						//add saber impact debounce
						DebounceSaberImpact(self, blocker, rSaberNum, rBladeNum, sabimpactentity_num);
						return qtrue;
					}
				}
			}
		}

		//ok, that didn't happen.  Just bounce the sucker then.
		if (!passthru)
		{
			if (saber_hit_wall)
			{
				if (self->client->saber[rSaberNum].saberFlags & SFL_BOUNCE_ON_WALLS
					&& (PM_SaberInAttackPure(self->client->ps.saber_move) //only in a normal attack anim
						|| self->client->ps.saber_move == LS_A_JUMP_T__B_ || self->client->ps.saber_move ==
						LS_A_JUMP_PALP_)) //or in the strong jump-fwd-attack "death from above" move
				{
					//do bounce sound & force feedback
					WP_SaberBounceOnWallSound(self, rSaberNum, rBladeNum);
					self->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
					self->client->ps.saberBounceMove = LS_D1_BR + (saber_moveData[self->client->ps.saber_move].startQuad -
						Q_BR);
				}
				else if (self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK &&
					!PM_SaberInAttackPure(self->client->ps.saber_move) &&
					!PM_CrouchAnim(self->client->ps.legsAnim) &&
					!PM_WalkingAnim(self->client->ps.legsAnim) &&
					!PM_RunningAnim(self->client->ps.legsAnim) &&
					self->client->buttons & BUTTON_WALKING &&
					!(self->r.svFlags & SVF_BOT))
				{
					//reflect from wall
					//do bounce sound & force feedback
					self->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
					self->client->ps.saberBounceMove = LS_D1_BR + (saber_moveData[self->client->ps.saber_move].startQuad -
						Q_BR);
				}
			}
			else
			{
				if (!idle_damage)
				{
					if (self->client->ps.saberLockTime >= level.time
						//in a saberlock otherOwner wasn't in an attack and didn't block the blow.
						|| blocker && blocker->inuse && blocker->client
						&& !BG_SaberInNonIdleDamageMove(&blocker->client->ps, blocker->localAnimIndex)
						//they aren't in an attack swing
						&& blocker->client->ps.saberBlocked == BLOCKED_NONE) //and they didn't block the attack.
					{
						//don't bounce!
					}
					else
					{
						self->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
					}
				}
				else
				{
					//don't bounce!
				}
			}
		}
		else
		{
			//don't bounce!
		}
	}

	//defender animations
	if (blocker && blocker->inuse && blocker->client)
	{
		if (BG_SaberInNonIdleDamageMove(&blocker->client->ps, blocker->localAnimIndex)
			//otherOwner was doing a damaging move
			&& blocker->client->ps.saberLockTime < level.time
			//otherOwner isn't in a saberlock (this can change mid-impact)
			&& (self->client->ps.saberBlocked != BLOCKED_NONE //self reacted to this impact.
				|| !idle_damage)) //or self was in an attack move (and probably misshaped from this impact)
		{
			blocker->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
		}
	}

	//add saber impact debounce
	DebounceSaberImpact(self, blocker, rSaberNum, rBladeNum, sabimpactentity_num);
	return qtrue;
}

qboolean PM_SaberInTransitionAny(int move);
qboolean WP_ForcePowerUsable(const gentity_t* self, forcePowers_t forcePower);
qboolean InFOV3(vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV);
qboolean Jedi_WaitingAmbush(const gentity_t* self);
void Jedi_Ambush(gentity_t* self);
evasionType_t jedi_saber_block_go(gentity_t* self, usercmd_t* cmd, vec3_t p_hitloc, vec3_t phit_dir,
	const gentity_t* incoming,
	float dist);
void NPC_SetLookTarget(const gentity_t* self, int entNum, int clear_time);
int blockedfor_quad(int quad);
int invert_quad(int quad);

void wp_saber_start_missile_block_check(gentity_t* self, usercmd_t* ucmd)
{
	qboolean swing_block;
	qboolean closest_swing_block = qfalse; //default setting makes the compiler happy.
	int swing_block_quad = Q_T;
	int closest_swing_quad = Q_T;
	float dist;
	gentity_t* ent, * incoming = NULL;
	int entity_list[MAX_GENTITIES];
	int num_listed_entities;
	int i, e;
	vec3_t mins, maxs;
	float closest_dist, radius = 256;
	vec3_t forward, fwdangles = { 0 };
	trace_t trace;
	vec3_t trace_to, ent_dir;
	float look_t_dist = -1;
	gentity_t* look_t = NULL;
	qboolean do_full_routine = qtrue;

	//keep this updated even if we don't get below
	if (!(self->client->ps.eFlags2 & EF2_HELD_BY_MONSTER))
	{
		//lookTarget is set by and to the monster that's holding you, no other operations can change that
		self->client->ps.hasLookTarget = qfalse;
	}

	if (self->client->ps.weapon != WP_SABER) //saber not here
	{
		return;
	}
	else if (self->client->ps.weapon == WP_SABER && BG_SabersOff(&self->client->ps))
	{
		//saber not currently in use or available, attempt to use our hands instead.
		do_full_routine = qfalse;
	}
	else if (self->client->ps.weapon == WP_SABER && self->client->ps.saberInFlight)
	{
		//saber not currently in use or available, attempt to use our hands instead.
		do_full_routine = qfalse;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1)
	{
		//you have not the SKILLZ
		do_full_routine = qfalse;
	}

	if (!(self->r.svFlags & SVF_BOT) && !(self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
	{
		return;
	}

	if (!walk_check(self)
		&& (PM_SaberInAttack(self->client->ps.saber_move)
			|| PM_SaberInStart(self->client->ps.saber_move)))
	{
		//this was put in to help bolts stop swings a bit. I dont know why it helps but it does :p
		do_full_routine = qfalse;
	}

	else if (self->client->ps.fd.forcePowersActive & 1 << FP_LIGHTNING)
	{
		//can't block while zapping
		do_full_routine = qfalse;
	}
	else if (self->client->ps.fd.forcePowersActive & 1 << FP_DRAIN)
	{
		//can't block while draining
		do_full_routine = qfalse;
	}
	else if (self->client->ps.fd.forcePowersActive & 1 << FP_PUSH)
	{
		//can't block while shoving
		do_full_routine = qfalse;
	}
	else if (self->client->ps.fd.forcePowersActive & 1 << FP_GRIP)
	{
		//can't block while gripping (FIXME: or should it break the grip?  Pain should break the grip, I think...)
		do_full_routine = qfalse;
	}

	//you should be able to update block positioning if you're already in a block.
	if (self->client->ps.weaponTime > 0 && !PM_SaberInParry(self->client->ps.saber_move))
	{
		//don't autoblock while busy with stuff
		return;
	}

	if (self->client->saber[0].saberFlags & SFL_NOT_ACTIVE_BLOCKING)
	{
		//can't actively block with this saber type
		return;
	}

	if (self->health <= 0)
	{
		//dead don't try to block (NOTE: actual deflection happens in missile code)
		return;
	}

	if (PM_InKnockDown(&self->client->ps))
	{
		//can't block when knocked down
		return;
	}

	if (BG_SabersOff(&self->client->ps) && self->client->NPC_class != CLASS_BOBAFETT
		|| self->client->pers.botclass != BCLASS_BOBAFETT
		|| self->client->pers.botclass != BCLASS_MANDOLORIAN1
		|| self->client->pers.botclass != BCLASS_MANDOLORIAN2)
	{
		if (self->s.eType != ET_NPC)
		{
			//player doesn't auto-activate
			do_full_routine = qfalse;
		}
	}

	if (self->s.eType == ET_PLAYER)
	{
		//don't do this if already attacking!
		if (ucmd->buttons & BUTTON_ATTACK
			|| PM_SaberInAttack(self->client->ps.saber_move)
			|| pm_saber_in_special_attack(self->client->ps.torsoAnim)
			|| PM_SaberInTransitionAny(self->client->ps.saber_move))
		{
			do_full_routine = qfalse;
		}
	}

	//leaving it now.
	if (self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] > level.time)
	{
		//can't block while gripping (FIXME: or should it break the grip?  Pain should break the grip, I think...)
		do_full_routine = qfalse;
	}

	fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors(fwdangles, forward, NULL, NULL);

	for (i = 0; i < 3; i++)
	{
		mins[i] = self->r.currentOrigin[i] - radius;
		maxs[i] = self->r.currentOrigin[i] + radius;
	}

	num_listed_entities = trap->EntitiesInBox(mins, maxs, entity_list, MAX_GENTITIES);

	closest_dist = radius;

	for (e = 0; e < num_listed_entities; e++)
	{
		float dot1;
		vec3_t dir;
		ent = &g_entities[entity_list[e]];
		swing_block = qfalse;

		if (ent == self)
			continue;

		//as long as we're here I'm going to get a look target too, I guess. -rww
		if (self->s.eType == ET_PLAYER &&
			ent->client &&
			(ent->s.eType == ET_NPC || ent->s.eType == ET_PLAYER) &&
			!OnSameTeam(ent, self) &&
			ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
			!(ent->client->ps.pm_flags & PMF_FOLLOW) &&
			(ent->s.eType != ET_NPC || ent->s.NPC_class != CLASS_VEHICLE) && //don't look at vehicle NPCs
			ent->health > 0)
		{
			//seems like a valid enemy to look at.
			vec3_t vec_sub;
			float vec_len;

			VectorSubtract(self->client->ps.origin, ent->client->ps.origin, vec_sub);
			vec_len = VectorLength(vec_sub);

			if (look_t_dist == -1 || vec_len < look_t_dist)
			{
				trace_t tr;
				vec3_t my_eyes;

				VectorCopy(self->client->ps.origin, my_eyes);
				my_eyes[2] += self->client->ps.viewheight;

				trap->Trace(&tr, my_eyes, NULL, NULL, ent->client->ps.origin, self->s.number, MASK_PLAYERSOLID, qfalse,
					0, 0);

				if (tr.fraction == 1.0f || tr.entityNum == ent->s.number)
				{
					//we have a clear line of sight to him, so it's all good.
					look_t = ent;
					look_t_dist = vec_len;
				}
			}
		}

		if (ent->r.ownerNum == self->s.number)
			continue;
		if (!ent->inuse)
			continue;
		if (ent->s.eType != ET_MISSILE && !(ent->s.eFlags & EF_MISSILE_STICK))
		{
			//not a normal projectile
			gentity_t* p_owner;

			if (ent->r.ownerNum < 0 || ent->r.ownerNum >= ENTITYNUM_WORLD)
			{
				//not going to be a client then.
				continue;
			}

			p_owner = &g_entities[ent->r.ownerNum];

			if (!p_owner->inuse || !p_owner->client)
			{
				continue; //not valid cl owner
			}

			if (!p_owner->client->ps.saberEntityNum ||
				p_owner->client->ps.saberEntityNum != ent->s.number)
			{
				//the saber is knocked away and/or not flying actively, or this ent is not the cl's saber ent at all
				continue;
			}
			//allow the blocking of normal saber swings
			if (!p_owner->client->ps.saberInFlight)
			{
				//active saber blade, treat differently.
				swing_block = qtrue;
				if (BG_SaberInNonIdleDamageMove(&p_owner->client->ps, p_owner->localAnimIndex))
				{
					//attacking
					swing_block_quad = invert_quad(saber_moveData[p_owner->client->ps.saber_move].startQuad);
				}
				else if (PM_SaberInStart(p_owner->client->ps.saber_move)
					|| PM_SaberInTransition(p_owner->client->ps.saber_move))
				{
					//preparing to attack
					swing_block_quad = invert_quad(saber_moveData[p_owner->client->ps.saber_move].endQuad);
				}
				else
				{
					//not attacking
					continue;
				}
			}

			//If we get here then it's ok to be treated as a thrown saber, I guess.
		}
		else
		{
			if (ent->s.pos.trType == TR_STATIONARY && self->s.eType == ET_PLAYER)
			{
				//nothing you can do with a stationary missile if you're the player
				continue;
			}
		}

		//see if they're in front of me
		VectorSubtract(ent->r.currentOrigin, self->r.currentOrigin, dir);
		dist = VectorNormalize(dir);

		if (dist > 150 && swing_block)
		{
			//don't block swings that are too far away.
			continue;
		}

		//handle detpacks, proximity mines and tripmines
		if (ent->s.weapon == WP_THERMAL)
		{
			//thermal detonator!
			if (dist < ent->splashRadius && !OnSameTeam(&g_entities[ent->r.ownerNum], self))
			{
				if (dist < ent->splashRadius &&
					ent->nextthink < level.time + 600 &&
					ent->count &&
					self->client->ps.groundEntityNum != ENTITYNUM_NONE &&
					(ent->s.pos.trType == TR_STATIONARY ||
						ent->s.pos.trType == TR_INTERPOLATE ||
						(dot1 = DotProduct(dir, forward)) < SABER_REFLECT_MISSILE_CONE))
				{
					//TD is close enough to hurt me, I'm on the ground and the thing is at rest or behind me and about to blow up, or I don't have force-push so force-jump!
					if (self->client->pers.botclass == BCLASS_BOBAFETT
						|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
						|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
					{
						//jump out of the way
						self->client->ps.fd.forceJumpCharge = 480;
						PM_AddFatigue(&self->client->ps, FORCE_LONGJUMP_POWER);
					}
				}
				else if (self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK
					&& self->client->pers.botclass != BCLASS_BOBAFETT
					&& self->client->pers.botclass != BCLASS_MANDOLORIAN1
					&& self->client->pers.botclass != BCLASS_MANDOLORIAN2)
				{
					ForceThrow(self, qfalse);
					PM_AddFatigue(&self->client->ps, FORCE_DEFLECT_PUSH);
				}
				else if (self->r.svFlags & SVF_BOT && self->client->ps.fd.forcePowerLevel[FP_PUSH] > 1)
				{
					ForceThrow(self, qfalse);
					PM_AddFatigue(&self->client->ps, FORCE_DEFLECT_PUSH);
				}
			}
			continue;
		}
		if (ent->splashDamage && ent->splashRadius && !(ent->s.powerups & 1 << PW_FORCE_PROJECTILE))
		{
			//exploding missile
			if (ent->s.pos.trType == TR_STATIONARY && ent->s.eFlags & EF_MISSILE_STICK)
			{
				//a placed explosive like a tripmine or detpack
				if (InFOV3(ent->r.currentOrigin, self->client->renderInfo.eyePoint, self->client->ps.viewangles, 90,
					90))
				{
					//in front of me
					if (G_ClearLOS4(self, ent))
					{
						//can see it
						vec3_t throw_dir;
						//make the gesture
						if (self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK
							&& self->client->pers.botclass != BCLASS_BOBAFETT
							&& self->client->pers.botclass != BCLASS_MANDOLORIAN1
							&& self->client->pers.botclass != BCLASS_MANDOLORIAN2)
						{
							ForceThrow(self, qfalse);
							PM_AddFatigue(&self->client->ps, FORCE_DEFLECT_PUSH);
						}
						//take it off the wall and toss it
						ent->s.pos.trType = TR_GRAVITY;
						ent->s.eType = ET_MISSILE;
						ent->s.eFlags &= ~EF_MISSILE_STICK;
						ent->flags |= FL_BOUNCE_HALF;
						AngleVectors(ent->r.currentAngles, throw_dir, NULL, NULL);
						VectorMA(ent->r.currentOrigin, ent->r.maxs[0] + 4, throw_dir, ent->r.currentOrigin);
						VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
						VectorScale(throw_dir, 300, ent->s.pos.trDelta);
						ent->s.pos.trDelta[2] += 150;
						VectorMA(ent->s.pos.trDelta, 800, dir, ent->s.pos.trDelta);
						ent->s.pos.trTime = level.time; // move a bit on the very first frame
						VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
						ent->r.ownerNum = self->s.number;
						// make it explode, but with less damage
						ent->splashDamage /= 3;
						ent->splashRadius /= 3;
						ent->nextthink = level.time + Q_irand(500, 3000);
					}
				}
			}
			else if (dist < ent->splashRadius
				&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
				&& DotProduct(dir, forward) < SABER_REFLECT_MISSILE_CONE)
			{
				//try to evade it
				if (self->client->pers.botclass == BCLASS_BOBAFETT
					|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
					|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
				{
					//jump out of the way
					self->client->ps.fd.forceJumpCharge = 480;
					PM_AddFatigue(&self->client->ps, FORCE_LONGJUMP_POWER);
				}
			}
			else if (self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK
				&& self->client->pers.botclass != BCLASS_BOBAFETT
				&& self->client->pers.botclass != BCLASS_MANDOLORIAN1
				&& self->client->pers.botclass != BCLASS_MANDOLORIAN2)
			{
				if (!self->s.number && self->client->ps.fd.forcePowerLevel[FP_PUSH] == 1 && dist >= 192)
				{
					//player with push 1 has to wait until it's closer otherwise the push misses
				}
				else
				{
					ForceThrow(self, qfalse);
					PM_AddFatigue(&self->client->ps, FORCE_DEFLECT_PUSH);
				}
			}
			else if (self->r.svFlags & SVF_BOT && self->client->ps.fd.forcePowerLevel[FP_PUSH] > 1)
			{
				ForceThrow(self, qfalse);
				PM_AddFatigue(&self->client->ps, FORCE_DEFLECT_PUSH);
			}
			//otherwise, can't block it, so we're screwed
			continue;
		}

		if (!do_full_routine)
		{
			//don't care about the rest then
			continue;
		}

		if (ent->s.weapon != WP_SABER)
		{
			//only block shots coming from behind
			if ((dot1 = DotProduct(dir, forward)) < SABER_REFLECT_MISSILE_CONE)
				continue;
		}

		//see if they're heading towards me
		if (!swing_block)
		{
			float dot2;
			vec3_t missile_dir;
			VectorCopy(ent->s.pos.trDelta, missile_dir);
			VectorNormalize(missile_dir);
			if ((dot2 = DotProduct(dir, missile_dir)) > 0)
				continue;
		}

		if (dist < closest_dist)
		{
			VectorCopy(self->r.currentOrigin, trace_to);
			trace_to[2] = self->r.absmax[2] - 4;
			trap->Trace(&trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, trace_to, ent->s.number, ent->clipmask,
				qfalse, 0, 0);
			if (trace.allsolid || trace.startsolid || trace.fraction < 1.0f && trace.entityNum != self->s.number &&
				trace.entityNum != self->client->ps.saberEntityNum)
			{
				//okay, try one more check
				VectorNormalize2(ent->s.pos.trDelta, ent_dir);
				VectorMA(ent->r.currentOrigin, radius, ent_dir, trace_to);
				trap->Trace(&trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, trace_to, ent->s.number,
					ent->clipmask, qfalse, 0, 0);
				if (trace.allsolid || trace.startsolid || trace.fraction < 1.0f && trace.entityNum != self->s.number &&
					trace.entityNum != self->client->ps.saberEntityNum)
				{
					//can't hit me, ignore it
					continue;
				}
			}
			if (self->s.eType == ET_NPC)
			{
				//An NPC
				if (self->NPC && !self->enemy && ent->r.ownerNum != ENTITYNUM_NONE)
				{
					gentity_t* owner = &g_entities[ent->r.ownerNum];
					if (owner->health >= 0 && (!owner->client || owner->client->playerTeam != self->client->playerTeam))
					{
						G_SetEnemy(self, owner);
					}
				}
			}
			//FIXME: if NPC, predict the intersection between my current velocity/path and the missile's, see if it intersects my bounding box (+/-saberLength?), don't try to deflect unless it does?
			closest_dist = dist;
			incoming = ent;
			closest_swing_block = swing_block;
			closest_swing_quad = swing_block_quad;
		}
	}

	if (self->s.eType == ET_NPC && self->localAnimIndex <= 1)
	{
		//humanoid NPCs don't set angles based on server angles for looking, unlike other NPCs
		if (self->client && self->client->renderInfo.lookTarget < ENTITYNUM_WORLD)
		{
			look_t = &g_entities[self->client->renderInfo.lookTarget];
		}
	}

	if (look_t)
	{
		//we got a look target at some point so we'll assign it then.
		if (!(self->client->ps.eFlags2 & EF2_HELD_BY_MONSTER))
		{
			//lookTarget is set by and to the monster that's holding you, no other operations can change that
			self->client->ps.hasLookTarget = qtrue;
			self->client->ps.lookTarget = look_t->s.number;
		}
	}

	if (!do_full_routine)
	{
		//then we're done now
		return;
	}

	if (incoming)
	{
		if (self->NPC)
		{
			if (Jedi_WaitingAmbush(self))
			{
				Jedi_Ambush(self);
			}
			if (self->client->NPC_class == CLASS_BOBAFETT
				|| self->client->pers.botclass == BCLASS_BOBAFETT
				|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
				|| self->client->pers.botclass == BCLASS_MANDOLORIAN2
				&& self->client->ps.eFlags2 & EF2_FLYING //moveType == MT_FLYSWIM
				&& incoming->methodOfDeath != MOD_ROCKET_HOMING)
			{
				//a hovering Boba Fett, not a tracking rocket
				if (!Q_irand(0, 1))
				{
					//strafe
					self->NPC->standTime = 0;
					self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + Q_irand(1000, 2000);
				}
				if (!Q_irand(0, 1))
				{
					//go up/down
					TIMER_Set(self, "heightChange", Q_irand(1000, 3000));
					self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + Q_irand(1000, 2000);
				}
			}
			else if (jedi_saber_block_go(self, &self->NPC->last_ucmd, NULL, NULL, incoming, 0.0f) != EVASION_NONE)
			{
				//make sure to turn on your saber if it's not on
				if (self->client->NPC_class != CLASS_BOBAFETT
					&& self->client->NPC_class != CLASS_ROCKETTROOPER
					&& self->client->NPC_class != CLASS_REBORN
					&& self->client->pers.botclass != BCLASS_BOBAFETT
					&& self->client->pers.botclass != BCLASS_MANDOLORIAN1
					&& self->client->pers.botclass != BCLASS_MANDOLORIAN2)
				{
					if (self->s.weapon == WP_SABER)
					{
						WP_ActivateSaber(self);
					}
				}
			}
		}
		else //player
		{
			gentity_t* blocker = &g_entities[incoming->r.ownerNum];

			if (self->client && self->client->ps.saberHolstered == 2)
			{
				WP_ActivateSaber(self);
			}
			if (closest_swing_block && blocker->health > 0)
			{
				blocker->client->ps.saberBlocked = blockedfor_quad(closest_swing_quad);
				blocker->client->ps.userInt3 |= 1 << FLAG_PREBLOCK;
			}
			else if (blocker->health > 0 && (blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK || blocker->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING))
			{
				wp_saber_block_non_random_missile(blocker, incoming->r.currentOrigin, qtrue);
			}
			else
			{
				vec3_t diff, start, end;
				VectorSubtract(incoming->r.currentOrigin, self->r.currentOrigin, diff);
				float scale = VectorLength(diff);
				VectorNormalize2(incoming->s.pos.trDelta, ent_dir);
				VectorMA(incoming->r.currentOrigin, scale, ent_dir, start);
				VectorCopy(self->r.currentOrigin, end);
				end[2] += self->maxs[2] * 0.75f;
				trap->Trace(&trace, start, incoming->mins, incoming->maxs, end, incoming->s.number, MASK_SHOT, qfalse, 0, 0);

				jedi_dodge_evasion(self, incoming->owner, &trace, HL_NONE);
			}
			if (incoming->owner && incoming->owner->client && (!self->enemy || self->enemy->s.weapon != WP_SABER))
			{
				self->enemy = incoming->owner;
				NPC_SetLookTarget(self, incoming->owner->s.number, level.time + 1000);
			}
		}
	}
}

#define MIN_SABER_SLICE_DISTANCE 50

#define MIN_SABER_SLICE_RETURN_DISTANCE 30

#define SABER_THROWN_HIT_DAMAGE 30
#define SABER_THROWN_RETURN_HIT_DAMAGE 5

static QINLINE qboolean CheckThrownSaberDamaged(gentity_t* saberent, gentity_t* saber_owner, gentity_t* ent,
	const int dist,
	const int returning, const qboolean no_d_check)
{
	vec3_t vecsub;
	float veclen;
	gentity_t* te;

	if (!saber_owner || !saber_owner->client)
	{
		return qfalse;
	}

	if (saber_owner->client->ps.saberAttackWound > level.time)
	{
		return qfalse;
	}

	if (ent && ent->client && ent->inuse && ent->s.number != saber_owner->s.number &&
		ent->health > 0 && ent->takedamage &&
		trap->InPVS(ent->client->ps.origin, saberent->r.currentOrigin) &&
		ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
		(ent->client->pers.connected || ent->s.eType == ET_NPC))
	{
		//hit a client
		if (ent->inuse && ent->client &&
			ent->client->ps.duelInProgress &&
			ent->client->ps.duelIndex != saber_owner->s.number)
		{
			return qfalse;
		}

		if (ent->inuse && ent->client &&
			saber_owner->client->ps.duelInProgress &&
			saber_owner->client->ps.duelIndex != ent->s.number)
		{
			return qfalse;
		}

		VectorSubtract(saberent->r.currentOrigin, ent->client->ps.origin, vecsub);
		veclen = VectorLength(vecsub);

		if (veclen < dist)
		{
			//within range
			trace_t tr;

			trap->Trace(&tr, saberent->r.currentOrigin, NULL, NULL, ent->client->ps.origin, saberent->s.number, MASK_SHOT, qfalse, 0, 0);

			if (tr.fraction == 1 || tr.entityNum == ent->s.number)
			{
				//Slice them
				if (WP_SaberCanBlockThrownSaber(ent, tr.endpos, qtrue))
				{
					//they blocked it
					te = G_TempEntity(tr.endpos, EV_SABER_BLOCK);
					VectorCopy(tr.endpos, te->s.origin);
					VectorCopy(tr.plane.normal, te->s.angles);
					if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
					{
						te->s.angles[1] = 1;
					}
					te->s.eventParm = 1;
					te->s.weapon = 0; //saberNum
					te->s.legsAnim = 0; //blade_num

					if (saberCheckKnockdown_Thrown(saberent, saber_owner, &g_entities[tr.entityNum]))
					{
						//it was knocked out of the air
						return qfalse;
					}

					if (!returning)
					{
						//return to owner if blocked
						thrownSaberTouch(saberent, saberent, NULL);
					}

					saber_owner->client->ps.saberAttackWound = level.time + 500;
					return qfalse;
				}
				else
				{ //a good hit
					vec3_t dir;
					int dflags = 0;

					VectorSubtract(tr.endpos, saberent->r.currentOrigin, dir);
					VectorNormalize(dir);

					if (!dir[0] && !dir[1] && !dir[2])
					{
						dir[1] = 1;
					}

					if (saber_owner->client->saber[0].saberFlags2 & SFL2_NO_DISMEMBERMENT)
					{
						dflags |= DAMAGE_NO_DISMEMBER;
					}

					if (saber_owner->client->saber[0].knockbackScale > 0.0f)
					{
						dflags |= DAMAGE_SABER_KNOCKBACK1;
					}

					if (saber_owner->client->ps.isJediMaster)
					{
						//2x damage for the Jedi Master
						G_Damage(ent, saber_owner, saber_owner, dir, tr.endpos, saberent->damage * 2, dflags, MOD_SABER);
					}
					else
					{
						G_Damage(ent, saber_owner, saber_owner, dir, tr.endpos, saberent->damage, dflags, MOD_SABER);
					}

					te = G_TempEntity(tr.endpos, EV_SABER_HIT);
					te->s.otherentity_num = ent->s.number;
					te->s.otherentity_num2 = saber_owner->s.number;
					te->s.weapon = 0; //saberNum
					te->s.legsAnim = 0; //blade_num
					VectorCopy(tr.endpos, te->s.origin);
					VectorCopy(tr.plane.normal, te->s.angles);
					if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
					{
						te->s.angles[1] = 1;
					}

					te->s.eventParm = 1;

					if (!returning)
					{
						//return to owner if blocked
						thrownSaberTouch(saberent, saberent, NULL);
					}
				}

				saber_owner->client->ps.saberAttackWound = level.time + 500;
			}
		}
	}
	else if (ent && !ent->client && ent->inuse && ent->takedamage && ent->health > 0 && ent->s.number != saber_owner->s.number
		&& ent->s.number != saberent->s.number && (no_d_check || trap->InPVS(ent->r.currentOrigin, saberent->r.currentOrigin)))
	{
		//hit a non-client
		if (no_d_check)
		{
			veclen = 0;
		}
		else
		{
			VectorSubtract(saberent->r.currentOrigin, ent->r.currentOrigin, vecsub);
			veclen = VectorLength(vecsub);
		}

		if (veclen < dist)
		{
			trace_t tr;
			vec3_t ent_origin;

			if (ent->s.eType == ET_MOVER)
			{
				VectorSubtract(ent->r.absmax, ent->r.absmin, ent_origin);
				VectorMA(ent->r.absmin, 0.5, ent_origin, ent_origin);
				VectorAdd(ent->r.absmin, ent->r.absmax, ent_origin);
				VectorScale(ent_origin, 0.5f, ent_origin);
			}
			else
			{
				VectorCopy(ent->r.currentOrigin, ent_origin);
			}

			trap->Trace(&tr, saberent->r.currentOrigin, NULL, NULL, ent_origin, saberent->s.number, MASK_SHOT, qfalse, 0, 0);

			if (tr.fraction == 1 || tr.entityNum == ent->s.number)
			{
				vec3_t dir;
				int dflags = 0;

				VectorSubtract(tr.endpos, ent_origin, dir);
				VectorNormalize(dir);

				if (saber_owner->client->saber[0].saberFlags2 & SFL2_NO_DISMEMBERMENT)
				{
					dflags |= DAMAGE_NO_DISMEMBER;
				}
				if (saber_owner->client->saber[0].knockbackScale > 0.0f)
				{
					dflags |= DAMAGE_SABER_KNOCKBACK1;
				}

				if (ent->s.eType == ET_NPC)
				{
					G_Damage(ent, saber_owner, saber_owner, dir, tr.endpos, 40, dflags, MOD_SABER);
				}
				else
				{
					G_Damage(ent, saber_owner, saber_owner, dir, tr.endpos, 5, dflags, MOD_SABER);
				}

				te = G_TempEntity(tr.endpos, EV_SABER_HIT);
				te->s.otherentity_num = ENTITYNUM_NONE; //don't do this for throw damage
				te->s.otherentity_num2 = saber_owner->s.number;
				//actually, do send this, though - for the overridden per-saber hit effects/sounds
				te->s.weapon = 0; //saberNum
				te->s.legsAnim = 0; //blade_num
				VectorCopy(tr.endpos, te->s.origin);
				VectorCopy(tr.plane.normal, te->s.angles);
				if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
				{
					te->s.angles[1] = 1;
				}

				if (ent->s.eType == ET_MOVER)
				{
					if (saber_owner
						&& saber_owner->client
						&& saber_owner->client->saber[0].saberFlags2 & SFL2_NO_CLASH_FLARE)
					{
						//don't do clash flare - NOTE: assumes same is true for both sabers if using dual sabers!
						G_FreeEntity(te); //kind of a waste, but...
					}
					else
					{
						//I suppose I could tie this into the saberblock event, but I'm tired of adding flags to that thing.
						gentity_t* te_s = G_TempEntity(te->s.origin, EV_SABER_CLASHFLARE);
						VectorCopy(te->s.origin, te_s->s.origin);

						te->s.eventParm = 0;
					}
				}
				else
				{
					te->s.eventParm = 1;
				}

				if (!returning)
				{
					//return to owner if blocked
					thrownSaberTouch(saberent, saberent, NULL);
				}

				saber_owner->client->ps.saberAttackWound = level.time + 500;
			}
		}
	}

	return qtrue;
}

#define THROWN_SABER_COMP

static QINLINE void saberMoveBack(gentity_t* ent)
{
	vec3_t origin, old_org;

	ent->s.pos.trType = TR_LINEAR;

	VectorCopy(ent->r.currentOrigin, old_org);
	// get current position
	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);
	//Get current angles?
	BG_EvaluateTrajectory(&ent->s.apos, level.time, ent->r.currentAngles);

	VectorCopy(origin, ent->r.currentOrigin);
}

static void SaberBounceSound(gentity_t* self, gentity_t* other, trace_t* trace)
{
	VectorCopy(self->r.currentAngles, self->s.apos.trBase);
	self->s.apos.trBase[PITCH] = 90;
}

static void DeadSaberThink(gentity_t* saberent)
{
	if (saberent->s.owner != ENTITYNUM_NONE && g_entities[saberent->s.owner].s.eFlags & EF_DEAD)
	{
		saberent->s.owner = ENTITYNUM_NONE;
	}
	if (saberent->speed < level.time)
	{
		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	G_RunObject(saberent);
}

static void MakeDeadSaber(const gentity_t* ent)
{
	//spawn a "dead" saber entity here so it looks like the saber fell out of the air.
	//This entity will remove itself after a very short time period.
	vec3_t startorg;
	vec3_t startang;
	const gentity_t* owner;
	//trace stuct used for determining if it's safe to spawn at current location
	trace_t tr;

	if (level.gametype == GT_JEDIMASTER)
	{
		return;
	}

	gentity_t* saberent = G_Spawn();

	VectorCopy(ent->r.currentOrigin, startorg);
	VectorCopy(ent->r.currentAngles, startang);

	saberent->classname = "deadsaber";

	saberent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	saberent->r.ownerNum = ent->s.number;

	saberent->clipmask = MASK_PLAYERSOLID;
	saberent->r.contents = CONTENTS_TRIGGER; //0;

	VectorSet(saberent->r.mins, -3.0f, -3.0f, -1.5f);
	VectorSet(saberent->r.maxs, 3.0f, 3.0f, 1.5f);

	saberent->touch = SaberBounceSound;

	saberent->think = DeadSaberThink;
	saberent->nextthink = level.time;

	trap->Trace(&tr, startorg, saberent->r.mins, saberent->r.maxs, startorg, saberent->s.number, saberent->clipmask,
		qfalse, 0, 0);
	if (tr.startsolid || tr.fraction != 1)
	{
		//bad position, try popping our origin up a bit
		startorg[2] += 20;
		trap->Trace(&tr, startorg, saberent->r.mins, saberent->r.maxs, startorg, saberent->s.number, saberent->clipmask,
			qfalse, 0, 0);
		if (tr.startsolid || tr.fraction != 1)
		{
			//still no luck, try using our owner's origin
			owner = &g_entities[ent->r.ownerNum];
			if (owner->inuse && owner->client)
			{
				G_SetOrigin(saberent, owner->client->ps.origin);
			}
		}
	}

	VectorCopy(startorg, saberent->s.pos.trBase);
	VectorCopy(startang, saberent->s.apos.trBase);

	VectorCopy(startorg, saberent->s.origin);
	VectorCopy(startang, saberent->s.angles);

	VectorCopy(startorg, saberent->r.currentOrigin);
	VectorCopy(startang, saberent->r.currentAngles);

	saberent->s.apos.trType = TR_GRAVITY;
	saberent->s.apos.trDelta[0] = Q_irand(200, 800);
	saberent->s.apos.trDelta[1] = Q_irand(200, 800);
	saberent->s.apos.trDelta[2] = Q_irand(200, 800);
	saberent->s.apos.trTime = level.time - 50;

	saberent->s.pos.trType = TR_GRAVITY;
	saberent->s.pos.trTime = level.time - 50;
	saberent->flags = FL_BOUNCE_HALF;
	if (ent->r.ownerNum >= 0 && ent->r.ownerNum < ENTITYNUM_WORLD)
	{
		owner = &g_entities[ent->r.ownerNum];

		if (owner->inuse && owner->client &&
			owner->client->saber[0].model[0])
		{
			WP_SaberAddG2Model(saberent, owner->client->saber[0].model, owner->client->saber[0].skin);
			saberent->s.owner = owner->s.number;
		}
		else
		{
			G_FreeEntity(saberent);
			return;
		}
	}

	saberent->s.modelGhoul2 = 1;
	saberent->s.g2radius = 20;

	saberent->s.eType = ET_MISSILE;
	saberent->s.weapon = WP_SABER;

	saberent->speed = level.time + 4000;

	saberent->bounceCount = 12;

	//fall off in the direction the real saber was headed
	VectorCopy(ent->s.pos.trDelta, saberent->s.pos.trDelta);

	saberMoveBack(saberent);
	saberent->s.pos.trType = TR_GRAVITY;

	trap->LinkEntity((sharedEntity_t*)saberent);
}

#define MAX_LEAVE_TIME 2000
#define MAX_BOT_LEAVE_TIME 1750
#define MAX_PLAYER_LEAVE_TIME 1500
#define MIN_LEAVE_TIME 1250

void saberReactivate(gentity_t* saberent, gentity_t* saber_owner);
void saberBackToOwner(gentity_t* saberent);

static void DownedSaberThink(gentity_t* saberent)
{
	qboolean not_disowned = qfalse;
	qboolean pull_back = qfalse;

	saberent->nextthink = level.time;

	if (saberent->r.ownerNum == ENTITYNUM_NONE)
	{
		MakeDeadSaber(saberent);

		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	gentity_t* saber_own = &g_entities[saberent->r.ownerNum];

	if (!saber_own ||
		!saber_own->inuse ||
		!saber_own->client ||
		saber_own->client->sess.sessionTeam == TEAM_SPECTATOR ||
		saber_own->client->ps.pm_flags & PMF_FOLLOW)
	{
		MakeDeadSaber(saberent);

		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	if (saber_own->client->ps.saberEntityNum)
	{
		if (saber_own->client->ps.saberEntityNum == saberent->s.number)
		{
			//owner shouldn't have this set if we're thinking in here. Must've fallen off a cliff and instantly respawned or something.
			not_disowned = qtrue;
		}
		else
		{
			//This should never happen, but just in case..
			assert(!"ULTRA BAD THING");
			MakeDeadSaber(saberent);

			saberent->think = G_FreeEntity;
			saberent->nextthink = level.time;
			return;
		}
	}

	if (not_disowned || saber_own->health < 1 || !saber_own->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
	{
		//He's dead, just go back to our normal saber status
		saber_own->client->ps.saberEntityNum = saber_own->client->saberStoredIndex;

#ifdef _DEBUG
		if (saber_own->client->saberStoredIndex != saberent->s.number)
		{
			//I'm paranoid.
			saberent->think = G_FreeEntity;
			saberent->nextthink = level.time;
			return;
		}
#endif

		saberReactivate(saberent, saber_own);

		if (saber_own->health < 1)
		{
			saber_own->client->ps.saberInFlight = qfalse;
			MakeDeadSaber(saberent);
		}

		saberent->touch = SaberGotHit;
		saberent->think = SaberUpdateSelf;
		saberent->genericValue5 = 0;
		saberent->nextthink = level.time;

		saberent->r.svFlags |= SVF_NOCLIENT;
		saberent->s.loopSound = 0;
		saberent->s.loopIsSoundset = qfalse;

		if (saber_own->health > 0)
		{
			//only set this if he's alive. If dead we want to reflect the lack of saber on the corpse, as he died with his saber out.
			saber_own->client->ps.saberInFlight = qfalse;
			WP_SaberRemoveG2Model(saberent);
		}
		saber_own->client->ps.saberEntityState = 0;
		saber_own->client->ps.saberThrowDelay = level.time + 500;
		saber_own->client->ps.saberCanThrow = qfalse;

		return;
	}

	if ((level.time - saber_own->client->saberKnockedTime > MAX_PLAYER_LEAVE_TIME && saber_own->client->pers.cmd.buttons & BUTTON_ATTACK) ||
		(saberent->s.pos.trType == TR_STATIONARY &&
			(saber_own->client->pers.cmd.buttons & BUTTON_ATTACK || saber_own->client->ps.forceHandExtend == HANDEXTEND_SABERPULL))
		&& !(saber_own->r.svFlags & SVF_BOT))
	{
		//we want to pull the saber back.
		pull_back = qtrue;
	}
	else if (saber_own->r.svFlags & SVF_BOT)
	{
		if (saber_own->client->ps.fd.saberAnimLevel != SS_DUAL && level.time - saber_own->client->saberKnockedTime > MAX_BOT_LEAVE_TIME)
		{
			//we want to pull the saber back.
			pull_back = qtrue;
		}
		else
		{
			//we want to pull the saber back.
			pull_back = qtrue;
		}
	}
	else if (level.time - saber_own->client->saberKnockedTime > MAX_LEAVE_TIME)
	{
		//Been sitting around for too long, go back no matter what he wants.
		pull_back = qtrue;
	}

	if (pull_back)
	{
		//Get going back to the owner.
		saber_own->client->ps.saberEntityNum = saber_own->client->saberStoredIndex;

#ifdef _DEBUG
		if (saber_own->client->saberStoredIndex != saberent->s.number)
		{
			//I'm paranoid.
			saber_own->client->saberStoredIndex = saber_own->client->ps.saberEntityNum = saberent->s.number;
		}
#endif
		saberReactivate(saberent, saber_own);

		saberent->touch = SaberGotHit;

		saberent->think = saberBackToOwner;
		saberent->speed = 0;
		saberent->genericValue5 = 0;
		saberent->nextthink = level.time;

		saberent->r.contents = CONTENTS_LIGHTSABER;

		G_Sound(saber_own, CHAN_BODY, G_SoundIndex("sound/weapons/force/pull.wav"));
		//reset the angle physics so we don't have don't have weird spinning on return
		VectorCopy(saberent->r.currentAngles, saberent->s.apos.trBase);
		VectorClear(saberent->s.apos.trDelta);
		saberent->s.apos.trType = TR_STATIONARY;

		return;
	}

	G_RunObject(saberent);
	saberent->nextthink = level.time;
}

static void DrownedSaberTouch(gentity_t* self, gentity_t* other, trace_t* trace)
{
	//similar to SaberBounceSound but the saber's owners can also pick up their saber by crouching or rolling over it
	VectorCopy(self->r.currentAngles, self->s.apos.trBase);
	self->s.apos.trBase[PITCH] = 90;

	//be able to pick up a dead saber by crouching/rolling over it while on the ground or by catching it mid-air.
	if (other->s.number != self->r.ownerNum || !other || !other->client)
	{
		//not our owner, or our owner is bad, ignore touch
		return;
	}

	if (self->s.pos.trType == TR_STATIONARY //saber isn't bouncing around.
		//and in a roll or crouching
		&& (BG_InRoll(&other->client->ps, other->client->ps.legsAnim) || PM_CrouchAnim(other->client->ps.legsAnim)))
	{
		other->client->ps.saberEntityNum = other->client->saberStoredIndex;

#ifdef _DEBUG
		if (other->client->saberStoredIndex != self->s.number)
		{
			//I'm paranoid.
			assert(!"Bad saber index!!!");
		}
#endif
		saberReactivate(self, other);

		self->r.contents = CONTENTS_LIGHTSABER;

		G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/saber/saber_catch.mp3"));

		other->client->ps.saberInFlight = qfalse;
		other->client->ps.saberEntityState = 0;
		other->client->ps.saberCanThrow = qfalse;
		other->client->ps.saberThrowDelay = level.time + 300;

		if (other->client->ps.forceHandExtend == HANDEXTEND_SABERPULL)
		{
			//stop holding hand out if we still are.
			other->client->ps.forceHandExtend = HANDEXTEND_NONE;
			other->client->ps.forceHandExtendTime = level.time;
		}

		self->touch = SaberGotHit;

		self->think = SaberUpdateSelf;
		self->genericValue5 = 0;
		self->nextthink = level.time + 50;
		WP_SaberRemoveG2Model(self);

		//auto reactive the blade
		other->client->ps.saberHolstered = 0;

		if (other->client->saber[0].soundOn)
		{
			//make activation noise if we have one.
			G_Sound(other, CHAN_WEAPON, other->client->saber[0].soundOn);
		}
	}
}

void saberReactivate(gentity_t* saberent, gentity_t* saber_owner)
{
	saberent->s.saberInFlight = qtrue;

	saberent->s.apos.trType = TR_LINEAR;
	saberent->s.apos.trDelta[0] = 600;
	saberent->s.apos.trDelta[1] = 0;
	saberent->s.apos.trDelta[2] = 0;

	saberent->s.pos.trType = TR_LINEAR;
	saberent->s.eType = ET_GENERAL;
	saberent->s.eFlags = 0;
	saberent->s.modelGhoul2 = 127;

	saberent->parent = saber_owner;

	saberent->genericValue5 = 0;

	SetSaberBoxSize(saberent);

	saberent->touch = thrownSaberTouch;

	saberent->s.weapon = WP_SABER;

	saber_owner->client->ps.saberEntityState = 1;

	trap->LinkEntity((sharedEntity_t*)saberent);
}

#define SABER_BOTRETRIEVE_DELAY 600
#define SABER_RETRIEVE_DELAY 3000 //3 seconds for now. This will leave you nice and open if you lose your saber.

static void saberKnockDown(gentity_t* saberent, gentity_t* saber_owner, const gentity_t* other)
{
	trace_t tr;

	saber_owner->client->ps.saberEntityNum = 0; //still stored in client->saberStoredIndex
	saber_owner->client->saberKnockedTime = level.time + SABER_RETRIEVE_DELAY;

	saberent->clipmask = MASK_SOLID;

	if (saber_owner->client->ps.fd.saberAnimLevel != SS_DUAL)
		saberent->r.contents = CONTENTS_TRIGGER; //0;

	VectorSet(saberent->r.mins, -3.0f, -3.0f, -1.5f);
	VectorSet(saberent->r.maxs, 3.0f, 3.0f, 1.5f);

	//perform a trace before attempting to spawn at currently location.
	//unfortunately, it's a fairly regular occurance that current saber location
	//(normally at the player's right hand) could result in the saber being stuck
	//in the the map and then freaking out.
	trap->Trace(&tr, saberent->r.currentOrigin, saberent->r.mins, saberent->r.maxs, saberent->r.currentOrigin,
		saberent->s.number, saberent->clipmask, qfalse, 0, 0);
	if (tr.startsolid || tr.fraction != 1)
	{
		//bad position, try popping our origin up a bit
		saberent->r.currentOrigin[2] += 20;
		G_SetOrigin(saberent, saberent->r.currentOrigin);
		trap->Trace(&tr, saberent->r.currentOrigin, saberent->r.mins, saberent->r.maxs, saberent->r.currentOrigin,
			saberent->s.number, saberent->clipmask, qfalse, 0, 0);
		if (tr.startsolid || tr.fraction != 1)
		{
			//still no luck, try using our owner's origin
			G_SetOrigin(saberent, saber_owner->client->ps.origin);
		}
	}

	saberent->s.apos.trType = TR_GRAVITY;
	saberent->s.apos.trDelta[0] = Q_irand(200, 800);
	saberent->s.apos.trDelta[1] = Q_irand(200, 800);
	saberent->s.apos.trDelta[2] = Q_irand(200, 800);
	saberent->s.apos.trTime = level.time - 50;

	saberent->s.pos.trType = TR_GRAVITY;
	saberent->s.pos.trTime = level.time - 50;
	saberent->flags |= FL_BOUNCE_HALF;

	WP_SaberAddG2Model(saberent, saber_owner->client->saber[0].model, saber_owner->client->saber[0].skin);

	saberent->s.modelGhoul2 = 1;
	saberent->s.g2radius = 20;

	saberent->s.eType = ET_MISSILE;
	saberent->s.weapon = WP_SABER;

	saberent->speed = level.time + 4000;

	saberent->bounceCount = -5; //8;

	saberent->s.owner = saber_owner->s.number;
	saberent->s.pos.trType = TR_GRAVITY;

	saberent->s.loopSound = 0; //kill this in case it was spinning.
	saberent->s.loopIsSoundset = qfalse;

	saberent->r.svFlags &= ~SVF_NOCLIENT; //make sure the client is getting updates on where it is and such.

	saberent->touch = DrownedSaberTouch;
	saberent->think = DownedSaberThink;
	saberent->nextthink = level.time;

	if (saber_owner != other)
	{
		//if someone knocked it out of the air and it wasn't turned off, go in the direction they were facing.
		if (other->inuse && other->client)
		{
			vec3_t other_fwd;
			const float deflect_speed = 200;

			AngleVectors(other->client->ps.viewangles, other_fwd, 0, 0);

			saberent->s.pos.trDelta[0] = other_fwd[0] * deflect_speed;
			saberent->s.pos.trDelta[1] = other_fwd[1] * deflect_speed;
			saberent->s.pos.trDelta[2] = other_fwd[2] * deflect_speed;
		}
	}

	trap->LinkEntity((sharedEntity_t*)saberent);

	if (saber_owner->client->saber[0].soundOff)
	{
		G_Sound(saberent, CHAN_BODY, saber_owner->client->saber[0].soundOff);
	}

	if (saber_owner->client->saber[1].soundOff &&
		saber_owner->client->saber[1].model[0])
	{
		G_Sound(saber_owner, CHAN_BODY, saber_owner->client->saber[1].soundOff);
	}
	if (saber_owner->client->ps.fd.saberAnimLevel == SS_DUAL)
	{
		//only switch off one blade if player is in the dual styley.
		saber_owner->client->ps.saberHolstered = 1;
	}
	else
	{
		saber_owner->client->ps.saberHolstered = 2;
	}
}

//sort of a silly macro I guess. But if I change anything in here I'll probably want it to be everywhere.
#define SABERINVALID (!saberent || !saberOwner || !other || !saberent->inuse || !saberOwner->inuse || !other->inuse || !saberOwner->client || !other->client || !saberOwner->client->ps.saberEntityNum || saberOwner->client->ps.saberLockTime > (level.time-100))

void WP_SaberRemoveG2Model(gentity_t* saberent)
{
	if (saberent->ghoul2)
	{
		trap->G2API_RemoveGhoul2Models(&saberent->ghoul2);
	}
}

void WP_SaberAddG2Model(gentity_t* saberent, const char* saber_model, const qhandle_t saber_skin)
{
	WP_SaberRemoveG2Model(saberent);
	if (saber_model && saber_model[0])
	{
		saberent->s.modelIndex = G_model_index(saber_model);
	}
	else
	{
		saberent->s.modelIndex = G_model_index(DEFAULT_SABER_MODEL);
	}
	//FIXME: use customSkin?
	trap->G2API_InitGhoul2Model(&saberent->ghoul2, saber_model, saberent->s.modelIndex, saber_skin, 0, 0, 0);
}

//Make the saber go flying directly out of the owner's hand in the specified direction
qboolean saberKnockOutOfHand(gentity_t* saberent, gentity_t* saber_owner, vec3_t velocity)
{
	if (!saberent || !saber_owner ||
		!saberent->inuse || !saber_owner->inuse ||
		!saber_owner->client)
	{
		return qfalse;
	}

	if (!saber_owner->client->ps.saberEntityNum)
	{
		//already gone
		return qfalse;
	}

	if (level.time - saber_owner->client->lastSaberStorageTime > 50)
	{
		//must have a reasonably updated saber base pos
		return qfalse;
	}

	if (saber_owner->client->ps.saberLockTime > level.time - 100)
	{
		return qfalse;
	}
	if (saber_owner->client->saber[0].saberFlags & SFL_NOT_DISARMABLE)
	{
		return qfalse;
	}

	saber_owner->client->ps.saberInFlight = qtrue;
	saber_owner->client->ps.saberEntityState = 1;

	saberent->s.saberInFlight = qfalse; //qtrue;

	saberent->s.pos.trType = TR_LINEAR;
	saberent->s.eType = ET_GENERAL;
	saberent->s.eFlags = 0;

	WP_SaberAddG2Model(saberent, saber_owner->client->saber[0].model, saber_owner->client->saber[0].skin);

	saberent->s.modelGhoul2 = 127;

	saberent->parent = saber_owner;

	saberent->damage = SABER_THROWN_HIT_DAMAGE;
	saberent->methodOfDeath = MOD_SABER;
	saberent->splashMethodOfDeath = MOD_SABER;
	saberent->s.solid = 2;
	saberent->r.contents = CONTENTS_LIGHTSABER;

	saberent->genericValue5 = 0;

	VectorSet(saberent->r.mins, -24.0f, -24.0f, -8.0f);
	VectorSet(saberent->r.maxs, 24.0f, 24.0f, 8.0f);

	saberent->s.genericenemyindex = saber_owner->s.number + 1024;
	saberent->s.weapon = WP_SABER;

	saberent->genericValue5 = 0;

	G_SetOrigin(saberent, saber_owner->client->lastSaberBase_Always); //use this as opposed to the right hand bolt,
	//because I don't want to risk reconstructing the skel again to get it here. And it isn't worth storing.
	saberKnockDown(saberent, saber_owner, saber_owner);
	VectorCopy(velocity, saberent->s.pos.trDelta); //override the velocity on the knocked away saber.

	return qtrue;
}

//Called at the result of a circle lock duel - the loser gets his saber tossed away and is put into a reflected attack anim
qboolean saberCheckKnockdown_DuelLoss(gentity_t* saberent, gentity_t* saberOwner, const gentity_t* other)
{
	vec3_t dif;
	qboolean valid_momentum = qtrue;
	int disarm_chance = 1;

	if (SABERINVALID)
	{
		return qfalse;
	}

	VectorClear(dif);

	if (!other->client->olderIsValid || level.time - other->client->lastSaberStorageTime >= 200)
	{
		//see if the spots are valid
		valid_momentum = qfalse;
	}

	if (valid_momentum)
	{
		//Get the difference
		VectorSubtract(other->client->lastSaberBase_Always, other->client->olderSaberBase, dif);
		float total_distance = VectorNormalize(dif);

		if (!total_distance)
		{
			//fine, try our own
			if (!saberOwner->client->olderIsValid || level.time - saberOwner->client->lastSaberStorageTime >= 200)
			{
				valid_momentum = qfalse;
			}

			if (valid_momentum)
			{
				VectorSubtract(saberOwner->client->lastSaberBase_Always, saberOwner->client->olderSaberBase, dif);
				total_distance = VectorNormalize(dif);
			}
		}

		if (valid_momentum)
		{
			if (!total_distance)
			{
				//try the difference between the two blades
				VectorSubtract(saberOwner->client->lastSaberBase_Always, other->client->lastSaberBase_Always, dif);
				total_distance = VectorNormalize(dif);
			}

			if (total_distance)
			{
				const float distScale = 6.5f;
				//if we still have no difference somehow, just let it fall to the ground when the time comes.
				if (total_distance < 20)
				{
					total_distance = 20;
				}
				VectorScale(dif, total_distance * distScale, dif);
			}
		}
	}

	saberOwner->client->ps.saber_move = LS_V1_BL;

	//Ideally check which lock it was exactly and use the proper anim (same goes for the attacker)
	saberOwner->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;

	if (other && other->client)
	{
		disarm_chance += other->client->saber[0].disarmBonus;
		if (other->client->saber[1].model[0]
			&& !other->client->ps.saberHolstered)
		{
			disarm_chance += other->client->saber[1].disarmBonus;
		}
	}
	if (Q_irand(0, disarm_chance))
	{
		return saberKnockOutOfHand(saberent, saberOwner, dif);
	}
	return qfalse;
}

//Knock the saber out of the hands of saberOwner using the net momentum between saberOwner and others net momentum
qboolean ButterFingers(gentity_t* saberent, gentity_t* saber_owner, const gentity_t* other, const trace_t* tr)
{
	vec3_t svelocity, ovelocity;
	vec3_t sswing, oswing;
	vec3_t dir;

	VectorClear(svelocity);
	VectorClear(ovelocity);
	VectorClear(sswing);
	VectorClear(oswing);

	if (!saber_owner->client->olderIsValid || level.time - saber_owner->client->lastSaberStorageTime >= 200
		|| !saber_owner->client->ps.saberEntityNum || saber_owner->client->ps.saberInFlight)
	{
		//old or bad saberOwner data or you don't have a saber in your hand.  We're kind of screwed so just return.
		return qfalse;
	}
	VectorSubtract(saber_owner->client->lastSaberBase_Always, saber_owner->client->olderSaberBase, sswing);
	VectorAdd(saber_owner->client->ps.velocity, sswing, sswing);

	if (other && other->client && other->inuse)
	{
		if (other->client->olderIsValid && level.time - other->client->lastSaberStorageTime >= 200
			&& other->client->ps.saberEntityNum && !other->client->ps.saberInFlight)
		{
			VectorSubtract(other->client->lastSaberBase_Always, other->client->olderSaberBase, oswing);
			VectorCopy(other->client->ps.velocity, ovelocity);
			VectorAdd(ovelocity, oswing, oswing);
		}
		else if (other->client->ps.velocity)
		{
			VectorCopy(other->client->ps.velocity, oswing);
		}
	}
	else
	{
		//No useable client data....ok  Let's try just bouncing off of the impact surface then.
		//scale things back a bit for realism.
		VectorScale(sswing, SABER_ELASTIC_RATIO, sswing);

		if (DotProduct(tr->plane.normal, sswing) > 0)
		{
			//weird impact as the saber is moving away from the impact plane.  Oh well.
			VectorScale(tr->plane.normal, -VectorLength(sswing), oswing);
		}
		else
		{
			VectorScale(tr->plane.normal, VectorLength(sswing), oswing);
		}
	}

	if (DotProduct(sswing, oswing) > 0)
	{
		VectorSubtract(oswing, sswing, dir);
	}
	else
	{
		VectorAdd(oswing, sswing, dir);
	}
	//don't pull it back on the next frame
	if (level.time - saber_owner->client->saberKnockedTime <= MAX_LEAVE_TIME)
	{
		saber_owner->client->buttons &= ~BUTTON_ATTACK;
	}

	return saberKnockOutOfHand(saberent, saber_owner, dir);
}

//Called when we want to try knocking the saber out of the owner's hand upon them going into a broken parry.
//Also called on reflected attacks.
qboolean saberCheckKnockdown_BrokenParry(gentity_t* saberent, gentity_t* saberOwner, gentity_t* other)
{
	qboolean doKnock = qfalse;

	if (SABERINVALID)
	{
		return qfalse;
	}

	//Neither gets an advantage based on attack state, when it comes to knocking
	//saber out of hand.
	const int my_attack = g_saber_attack_power(saberOwner, qfalse);
	const int other_attack = g_saber_attack_power(other, qfalse);

	if (!other->client->olderIsValid || level.time - other->client->lastSaberStorageTime >= 200)
	{
		//if we don't know which way to throw the saber based on momentum between saber positions, just don't throw it
		return qfalse;
	}

	//only knock the saber out of the hand if they're in a stronger stance I suppose. Makes strong more advantageous.
	if (other_attack > my_attack + 1 && Q_irand(1, 10) <= 7)
	{
		//This would be, say, strong stance against light stance.
		doKnock = qtrue;
	}
	else if (other_attack > my_attack && Q_irand(1, 10) <= 3)
	{
		//Strong vs. medium, medium vs. light
		doKnock = qtrue;
	}

	if (doKnock)
	{
		int disarm_chance = 1;
		vec3_t dif;
		const float dist_scale = 6.5f;

		VectorSubtract(other->client->lastSaberBase_Always, other->client->olderSaberBase, dif);
		float total_distance = VectorNormalize(dif);

		if (!total_distance)
		{
			//fine, try our own
			if (!saberOwner->client->olderIsValid || level.time - saberOwner->client->lastSaberStorageTime >= 200)
			{
				//if we don't know which way to throw the saber based on momentum between saber positions, just don't throw it
				return qfalse;
			}

			VectorSubtract(saberOwner->client->lastSaberBase_Always, saberOwner->client->olderSaberBase, dif);
			total_distance = VectorNormalize(dif);
		}

		if (!total_distance)
		{
			//...forget it then.
			return qfalse;
		}

		if (total_distance < 20)
		{
			total_distance = 20;
		}
		VectorScale(dif, total_distance * dist_scale, dif);

		if (other && other->client)
		{
			disarm_chance += other->client->saber[0].disarmBonus;
			if (other->client->saber[1].model[0]
				&& !other->client->ps.saberHolstered)
			{
				disarm_chance += other->client->saber[1].disarmBonus;
			}
		}
		if (Q_irand(0, disarm_chance))
		{
			return saberKnockOutOfHand(saberent, saberOwner, dif);
		}
	}

	return qfalse;
}

extern qboolean BG_InExtraDefenseSaberMove(int move);

//Called upon an enemy actually slashing into a thrown saber
qboolean saberCheckKnockdown_Smashed(gentity_t* saberent, gentity_t* saberOwner, const gentity_t* other,
	const int damage)
{
	if (SABERINVALID)
	{
		return qfalse;
	}

	if (!saberOwner->client->ps.saberInFlight)
	{
		//can only do this if the saber is already actually in flight
		return qfalse;
	}

	if (other
		&& other->inuse
		&& other->client
		&& BG_InExtraDefenseSaberMove(other->client->ps.saber_move))
	{
		//make sure the blow was strong enough
		saberKnockDown(saberent, saberOwner, other);
		return qtrue;
	}

	if (damage > 10)
	{
		//make sure the blow was strong enough
		saberKnockDown(saberent, saberOwner, other);
		return qtrue;
	}
	return qtrue;
}

//Called upon blocking a thrown saber. If the throw level compared to the blocker's defense level
//is inferior, or equal and a random factor is met, then the saber will be tossed to the ground.
qboolean saberCheckKnockdown_Thrown(gentity_t* saberent, gentity_t* saberOwner, const gentity_t* other)
{
	qboolean toss_it = qfalse;

	if (SABERINVALID)
	{
		return qfalse;
	}

	const int defen_level = other->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE];
	const int throw_level = saberOwner->client->ps.fd.forcePowerLevel[FP_SABERTHROW];

	if (defen_level > throw_level)
	{
		toss_it = qtrue;
	}
	else if (defen_level == throw_level && Q_irand(1, 10) <= 4)
	{
		toss_it = qtrue;
	}

	if (toss_it)
	{
		saberKnockDown(saberent, saberOwner, other);
		return qtrue;
	}

	return qfalse;
}

void saberBackToOwner(gentity_t* saberent)
{
	gentity_t* saber_owner = &g_entities[saberent->r.ownerNum];
	vec3_t dir;

	if (saberent->r.ownerNum == ENTITYNUM_NONE)
	{
		MakeDeadSaber(saberent);

		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	if (!saber_owner->inuse ||
		!saber_owner->client ||
		saber_owner->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		MakeDeadSaber(saberent);

		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	if (saber_owner->health < 1 || !saber_owner->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
	{
		//He's dead, just go back to our normal saber status
		saberent->touch = SaberGotHit;
		saberent->think = SaberUpdateSelf;
		saberent->genericValue5 = 0;
		saberent->nextthink = level.time;

		if (saber_owner->client &&
			saber_owner->client->saber[0].soundOff)
		{
			G_Sound(saberent, CHAN_AUTO, saber_owner->client->saber[0].soundOff);
		}
		MakeDeadSaber(saberent);

		saberent->r.svFlags |= SVF_NOCLIENT;
		saberent->r.contents = CONTENTS_LIGHTSABER;
		SetSaberBoxSize(saberent);
		saberent->s.loopSound = 0;
		saberent->s.loopIsSoundset = qfalse;
		WP_SaberRemoveG2Model(saberent);

		saber_owner->client->ps.saberInFlight = qfalse;
		saber_owner->client->ps.saberEntityState = 0;
		saber_owner->client->ps.saberThrowDelay = level.time + 500;
		saber_owner->client->ps.saberCanThrow = qfalse;

		return;
	}

	//make sure this is set alright
	assert(saber_owner->client->ps.saberEntityNum == saberent->s.number ||
		saber_owner->client->saberStoredIndex == saberent->s.number);
	saber_owner->client->ps.saberEntityNum = saberent->s.number;

	saberent->r.contents = CONTENTS_LIGHTSABER;

	VectorSubtract(saberent->pos1, saberent->r.currentOrigin, dir);

	const float owner_len = VectorLength(dir);

	if (saberent->speed < level.time)
	{
		float base_speed;

		VectorNormalize(dir);

		saberMoveBack(saberent);
		VectorCopy(saberent->r.currentOrigin, saberent->s.pos.trBase);

		if (saber_owner->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3)
		{
			//allow players with high saber throw rank to control the return speed of the saber
			base_speed = 900;

			saberent->speed = level.time; // + 200;

			//Gradually slow down as it approaches, so it looks smoother coming into the hand.
			if (owner_len < 64)
			{
				VectorScale(dir, base_speed - 200, saberent->s.pos.trDelta);
			}
			else if (owner_len < 128)
			{
				VectorScale(dir, base_speed - 150, saberent->s.pos.trDelta);
			}
			else if (owner_len < 256)
			{
				VectorScale(dir, base_speed - 100, saberent->s.pos.trDelta);
			}
			else
			{
				VectorScale(dir, base_speed, saberent->s.pos.trDelta);
			}
		}
		else
		{
			base_speed = 700;

			saberent->speed = level.time + 50;

			//Gradually slow down as it approaches, so it looks smoother coming into the hand.
			if (owner_len < 64)
			{
				VectorScale(dir, base_speed - 200, saberent->s.pos.trDelta);
			}
			else if (owner_len < 128)
			{
				VectorScale(dir, base_speed - 150, saberent->s.pos.trDelta);
			}
			else if (owner_len < 256)
			{
				VectorScale(dir, base_speed - 100, saberent->s.pos.trDelta);
			}
			else
			{
				VectorScale(dir, base_speed, saberent->s.pos.trDelta);
			}
		}

		saberent->s.pos.trTime = level.time;
	}

	//I'm just doing this now. I don't really like the spin on the way back. And it does weird stuff with the new saber-knocked-away code.
	if (saber_owner->client->ps.saberEntityNum == saberent->s.number)
	{
		if (!(saber_owner->client->saber[0].saberFlags & SFL_RETURN_DAMAGE)
			|| saber_owner->client->ps.saberHolstered)
		{
			saberent->s.saberInFlight = qfalse;
		}
		saberent->s.loopSound = saber_owner->client->saber[0].soundLoop;
		saberent->s.loopIsSoundset = qfalse;

		if (saber_owner->client->ps.forceHandExtend != HANDEXTEND_SABERPULL && owner_len <= 180)
		{
			saber_owner->client->ps.forceHandExtend = HANDEXTEND_SABERPULL;
		}

		if (owner_len <= 32)
		{
			G_Sound(saberent, CHAN_AUTO, G_SoundIndex("sound/weapons/saber/saber_catch.mp3"));

			saber_owner->client->ps.saberInFlight = qfalse;
			saber_owner->client->ps.saberEntityState = 0;
			saber_owner->client->ps.saberCanThrow = qfalse;
			saber_owner->client->ps.saberThrowDelay = level.time + 300;

			if (saber_owner->client->ps.forceHandExtend == HANDEXTEND_SABERPULL)
			{
				//stop holding hand out if we still are.
				saber_owner->client->ps.forceHandExtend = HANDEXTEND_NONE;
				saber_owner->client->ps.forceHandExtendTime = level.time;
			}

			if (saber_owner->r.svFlags & SVF_BOT) //NPC only
			{
				if (saber_owner->client->ps.saberFatigueChainCount >= MISHAPLEVEL_TEN)
				{
					saber_owner->client->ps.saberFatigueChainCount = MISHAPLEVEL_LIGHT;
				}
				wp_block_points_regenerate(saber_owner, BLOCKPOINTS_TWENTYFIVE);
				wp_force_power_regenerate(saber_owner, BLOCKPOINTS_TWENTYFIVE);
			}
			else
			{
				if (saber_owner->client->ps.saberFatigueChainCount >= MISHAPLEVEL_HUDFLASH)
				{
					saber_owner->client->ps.saberFatigueChainCount = MISHAPLEVEL_LIGHT;
				}
			}

			saberent->touch = SaberGotHit;

			saberent->think = SaberUpdateSelf;
			saberent->genericValue5 = 0;
			saberent->nextthink = level.time + 50;
			WP_SaberRemoveG2Model(saberent);
			saber_owner->client->ps.saberHolstered = 0;

			if (saber_owner->client->saber[0].soundOn)
			{
				//make activation noise if we have one.
				G_Sound(saber_owner, CHAN_WEAPON, saber_owner->client->saber[0].soundOn);
			}

			return;
		}

		saberMoveBack(saberent);
	}

	saberent->nextthink = level.time;
}

void thrownSaberBallistics(gentity_t* saberEnt, const gentity_t* saber_own, qboolean stuck);

void thrownSaberTouch(gentity_t* saberent, gentity_t* other, const trace_t* trace)
{
	gentity_t* hit_ent = other;
	gentity_t* saber_own = &g_entities[saberent->r.ownerNum];

	if (other && other->s.number == saberent->r.ownerNum)
	{
		return;
	}
	if (other
		&& other->s.number == ENTITYNUM_WORLD //hit solid object.
		&& saber_own->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3)
	{
		//hit something we can stick to and we threw the saber hard enough to stick.
		vec3_t saber_fwd;
		AngleVectors(saberent->r.currentAngles, saber_fwd, NULL, NULL);

		if (DotProduct(saber_fwd, trace->plane.normal) > 0)
		{
			//hit straight enough on to stick to the surface!
			thrownSaberBallistics(saberent, saber_own, qtrue);
			return;
		}
	}

	if (other && other->r.ownerNum < MAX_CLIENTS &&
		other->r.contents & CONTENTS_LIGHTSABER &&
		g_entities[other->r.ownerNum].client &&
		g_entities[other->r.ownerNum].inuse)
	{
		hit_ent = &g_entities[other->r.ownerNum];
	}

	//we'll skip the dist check, since we don't really care about that (we just hit it physically)
	CheckThrownSaberDamaged(saberent, &g_entities[saberent->r.ownerNum], hit_ent, 256, 0, qtrue);
	VectorCopy(saberent->r.currentOrigin, saberent->s.pos.trBase);
	VectorCopy(saberent->r.currentAngles, saberent->s.apos.trBase);

	if (saber_own->r.svFlags & SVF_BOT && level.time - saber_own->client->saberKnockedTime > SABER_BOTRETRIEVE_DELAY)
	{
		saberReactivate(saberent, saber_own);

		saberent->touch = SaberGotHit;
		saberent->think = saberBackToOwner;
		saberent->speed = 0;
		saberent->genericValue5 = 0;
		saberent->nextthink = level.time;

		saberent->r.contents = CONTENTS_LIGHTSABER;
	}
	else
	{
		saberKnockDown(saberent, saber_own, saber_own);
	}
}

#define SABER_MAX_THROW_DISTANCE 1000

static void saberFirstThrown(gentity_t* saberent)
{
	vec3_t v_sub;
	gentity_t* saber_own = &g_entities[saberent->r.ownerNum];

	if (saberent->r.ownerNum == ENTITYNUM_NONE)
	{
		MakeDeadSaber(saberent);

		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	if (!saber_own ||
		!saber_own->inuse ||
		!saber_own->client ||
		saber_own->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		MakeDeadSaber(saberent);

		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	if (saber_own->health < 1 || !saber_own->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
	{
		//He's dead, just go back to our normal saber status
		saberent->touch = SaberGotHit;
		saberent->think = SaberUpdateSelf;
		saberent->genericValue5 = 0;
		saberent->nextthink = level.time;

		if (saber_own->client &&
			saber_own->client->saber[0].soundOff)
		{
			G_Sound(saberent, CHAN_AUTO, saber_own->client->saber[0].soundOff);
		}
		MakeDeadSaber(saberent);

		saberent->r.svFlags |= SVF_NOCLIENT;
		saberent->r.contents = CONTENTS_LIGHTSABER;
		SetSaberBoxSize(saberent);
		saberent->s.loopSound = 0;
		saberent->s.loopIsSoundset = qfalse;
		WP_SaberRemoveG2Model(saberent);

		saber_own->client->ps.saberInFlight = qfalse;
		saber_own->client->ps.saberEntityState = 0;
		saber_own->client->ps.saberThrowDelay = level.time + 500;
		saber_own->client->ps.saberCanThrow = qfalse;

		return;
	}

	if (BG_HasYsalamiri(level.gametype, &saber_own->client->ps))
	{
		thrownSaberBallistics(saberent, saber_own, qfalse);
		goto runMin;
	}

	if (!BG_CanUseFPNow(level.gametype, &saber_own->client->ps, level.time, FP_SABERTHROW))
	{
		thrownSaberBallistics(saberent, saber_own, qfalse);
		goto runMin;
	}

	VectorSubtract(saber_own->client->ps.origin, saberent->r.currentOrigin, v_sub);
	const float v_len = VectorLength(v_sub);

	if (v_len >= 300 && saber_own->client->ps.fd.forcePowerLevel[FP_SABERTHROW] == FORCE_LEVEL_1)
	{
		thrownSaberBallistics(saberent, saber_own, qfalse);
		goto runMin;
	}

	if (v_len >= SABER_MAX_THROW_DISTANCE * saber_own->client->ps.fd.forcePowerLevel[FP_SABERTHROW])
	{
		thrownSaberBallistics(saberent, saber_own, qfalse);
		goto runMin;
	}

	if (!(saber_own->client->pers.cmd.buttons & BUTTON_ALT_ATTACK))
	{
		//we want to pull the saber back.
		if (saber_own->r.svFlags & SVF_BOT)
		{
			saberent->s.eFlags &= ~EF_MISSILE_STICK;

			saberReactivate(saberent, saber_own);

			saberent->touch = SaberGotHit;

			if (level.time - saber_own->client->saberKnockedTime > SABER_BOTRETRIEVE_DELAY)
			{
				saberent->think = saberBackToOwner;
			}
			else
			{
				G_RunObject(saberent);
				thrownSaberBallistics(saberent, saber_own, qfalse);
			}
			saberent->speed = 0;
			saberent->genericValue5 = 0;
			saberent->nextthink = level.time;

			saberent->r.contents = CONTENTS_LIGHTSABER;
		}
		else
		{
			G_RunObject(saberent);
			thrownSaberBallistics(saberent, saber_own, qfalse);
		}
	}

	if (saber_own->client->ps.fd.forcePowerDebounce[FP_SABERTHROW] < level.time
		&& saber_own->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3)
	{
		WP_ForcePowerDrain(&saber_own->client->ps, FP_SABERTHROW, 2);
		if (saber_own->client->ps.fd.forcePower < 1)
		{
			WP_ForcePowerStop(saber_own, FP_SABERTHROW);
		}

		saber_own->client->ps.fd.forcePowerDebounce[FP_SABERTHROW] = level.time + 1000;
	}

	// we don't want to have any physical control over the thrown saber's path. basejka code.
	//bots need this!
	if (saber_own->r.svFlags & SVF_BOT && saberent->speed < level.time)
	{
		vec3_t fwd, trace_from, trace_to, dir;
		trace_t tr;

		AngleVectors(saber_own->client->ps.viewangles, fwd, 0, 0);

		VectorCopy(saber_own->client->ps.origin, trace_from);
		trace_from[2] += saber_own->client->ps.viewheight;

		VectorCopy(trace_from, trace_to);
		if (saber_own->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3)
		{
			trace_to[0] += fwd[0] * 300;
			trace_to[1] += fwd[1] * 300;
			trace_to[2] += fwd[2] * 300;
		}
		else
		{
			//
		}

		if (level.time > SABER_BOTRETRIEVE_DELAY)
		{
			saberMoveBack(saberent);
		}

		VectorCopy(saberent->r.currentOrigin, saberent->s.pos.trBase);

		if (saber_own->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3)
		{
			//if highest saber throw rank, we can direct the saber toward players directly by looking at them
			trap->Trace(&tr, trace_from, NULL, NULL, trace_to, saber_own->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);
		}
		else
		{
			trap->Trace(&tr, trace_from, NULL, NULL, trace_to, saber_own->s.number, MASK_SOLID, qfalse, 0, 0);
		}

		VectorSubtract(tr.endpos, saberent->r.currentOrigin, dir);

		VectorNormalize(dir);

		VectorScale(dir, 500, saberent->s.pos.trDelta);
		saberent->s.pos.trTime = level.time;

		if (saber_own->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3)
		{
			//we'll treat them to a quicker update rate if their throw rank is high enough
			saberent->speed = level.time + 300;
		}
		else
		{
			saberent->speed = level.time + 400;
		}
	}
runMin:
	G_RunObject(saberent);
}

void UpdateClientRenderBolts(gentity_t* self, vec3_t render_origin, vec3_t render_angles)
{
	mdxaBone_t boltMatrix;
	renderInfo_t* ri = &self->client->renderInfo;

	if (!self->ghoul2)
	{
		VectorCopy(self->client->ps.origin, ri->headPoint);
		VectorCopy(self->client->ps.origin, ri->handRPoint);
		VectorCopy(self->client->ps.origin, ri->handLPoint);
		VectorCopy(self->client->ps.origin, ri->torsoPoint);
		VectorCopy(self->client->ps.origin, ri->crotchPoint);
		VectorCopy(self->client->ps.origin, ri->footRPoint);
		VectorCopy(self->client->ps.origin, ri->footLPoint);
	}
	else
	{
		//head
		trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->headBolt, &boltMatrix, render_angles, render_origin, level.time,
			NULL, self->modelScale);
		ri->headPoint[0] = boltMatrix.matrix[0][3];
		ri->headPoint[1] = boltMatrix.matrix[1][3];
		ri->headPoint[2] = boltMatrix.matrix[2][3];

		//right hand
		trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->handRBolt, &boltMatrix, render_angles, render_origin,
			level.time,
			NULL, self->modelScale);
		ri->handRPoint[0] = boltMatrix.matrix[0][3];
		ri->handRPoint[1] = boltMatrix.matrix[1][3];
		ri->handRPoint[2] = boltMatrix.matrix[2][3];

		//left hand
		trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->handLBolt, &boltMatrix, render_angles, render_origin,
			level.time,
			NULL, self->modelScale);
		ri->handLPoint[0] = boltMatrix.matrix[0][3];
		ri->handLPoint[1] = boltMatrix.matrix[1][3];
		ri->handLPoint[2] = boltMatrix.matrix[2][3];

		//chest
		trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->torsoBolt, &boltMatrix, render_angles, render_origin,
			level.time,
			NULL, self->modelScale);
		ri->torsoPoint[0] = boltMatrix.matrix[0][3];
		ri->torsoPoint[1] = boltMatrix.matrix[1][3];
		ri->torsoPoint[2] = boltMatrix.matrix[2][3];

		//crotch
		trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->crotchBolt, &boltMatrix, render_angles, render_origin,
			level.time,
			NULL, self->modelScale);
		ri->crotchPoint[0] = boltMatrix.matrix[0][3];
		ri->crotchPoint[1] = boltMatrix.matrix[1][3];
		ri->crotchPoint[2] = boltMatrix.matrix[2][3];

		//right foot
		trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->footRBolt, &boltMatrix, render_angles, render_origin,
			level.time,
			NULL, self->modelScale);
		ri->footRPoint[0] = boltMatrix.matrix[0][3];
		ri->footRPoint[1] = boltMatrix.matrix[1][3];
		ri->footRPoint[2] = boltMatrix.matrix[2][3];

		//left foot
		trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->footLBolt, &boltMatrix, render_angles, render_origin,
			level.time,
			NULL, self->modelScale);
		ri->footLPoint[0] = boltMatrix.matrix[0][3];
		ri->footLPoint[1] = boltMatrix.matrix[1][3];
		ri->footLPoint[2] = boltMatrix.matrix[2][3];
	}

	self->client->renderInfo.boltValidityTime = level.time;
}

static void UpdateClientRenderinfo(gentity_t* self, vec3_t render_origin, vec3_t render_angles)
{
	renderInfo_t* ri = &self->client->renderInfo;
	if (ri->mPCalcTime < level.time)
	{
		//We're just going to give rough estimates on most of this stuff,
		//it's not like most of it matters.

#if 0 //#if 0'd since it's a waste setting all this to 0 each frame.
		//Should you wish to make any of this valid then feel free to do so.
		ri->headYawRangeLeft = ri->headYawRangeRight = ri->headPitchRangeUp = ri->headPitchRangeDown = 0;
		ri->torsoYawRangeLeft = ri->torsoYawRangeRight = ri->torsoPitchRangeUp = ri->torsoPitchRangeDown = 0;

		ri->torsoFpsMod = ri->legsFpsMod = 0;

		VectorClear(ri->customRGB);
		ri->customAlpha = 0;
		ri->renderFlags = 0;
		ri->lockYaw = 0;

		VectorClear(ri->headAngles);
		VectorClear(ri->torsoAngles);

		//VectorClear(ri->eyeAngles);

		ri->legsYaw = 0;
#endif

		if (self->ghoul2 &&
			self->ghoul2 != ri->lastG2)
		{
			//the g2 instance changed, so update all the bolts.
			//rwwFIXMEFIXME: Base on skeleton used? Assuming humanoid currently.
			ri->lastG2 = self->ghoul2;

			if (self->localAnimIndex <= 1)
			{
				ri->headBolt = trap->G2API_AddBolt(self->ghoul2, 0, "*head_eyes");
				ri->handRBolt = trap->G2API_AddBolt(self->ghoul2, 0, "*r_hand");
				ri->handLBolt = trap->G2API_AddBolt(self->ghoul2, 0, "*l_hand");
				ri->torsoBolt = trap->G2API_AddBolt(self->ghoul2, 0, "thoracic");
				ri->crotchBolt = trap->G2API_AddBolt(self->ghoul2, 0, "pelvis");
				ri->footRBolt = trap->G2API_AddBolt(self->ghoul2, 0, "*r_leg_foot");
				ri->footLBolt = trap->G2API_AddBolt(self->ghoul2, 0, "*l_leg_foot");
				ri->motionBolt = trap->G2API_AddBolt(self->ghoul2, 0, "Motion");
			}
			else
			{
				ri->headBolt = -1;
				ri->handRBolt = -1;
				ri->handLBolt = -1;
				ri->torsoBolt = -1;
				ri->crotchBolt = -1;
				ri->footRBolt = -1;
				ri->footLBolt = -1;
				ri->motionBolt = -1;
			}

			ri->lastG2 = self->ghoul2;
		}

		VectorCopy(self->client->ps.viewangles, self->client->renderInfo.eyeAngles);

		//we'll just say the legs/torso are whatever the first frame of our current anim is.
		ri->torsoFrame = bgAllAnims[self->localAnimIndex].anims[self->client->ps.torsoAnim].firstFrame;
		ri->legsFrame = bgAllAnims[self->localAnimIndex].anims[self->client->ps.legsAnim].firstFrame;
		if (g_debugServerSkel.integer)
		{
			//Alright, I was doing this, but it's just too slow to do every frame.
			//From now on if we want this data to be valid we're going to have to make a verify call for it before
			//accessing it. I'm only doing this now if we want to debug the server skel by drawing lines from bolt
			//positions every frame.
			mdxaBone_t boltMatrix;

			if (!self->ghoul2)
			{
				VectorCopy(self->client->ps.origin, ri->headPoint);
				VectorCopy(self->client->ps.origin, ri->handRPoint);
				VectorCopy(self->client->ps.origin, ri->handLPoint);
				VectorCopy(self->client->ps.origin, ri->torsoPoint);
				VectorCopy(self->client->ps.origin, ri->crotchPoint);
				VectorCopy(self->client->ps.origin, ri->footRPoint);
				VectorCopy(self->client->ps.origin, ri->footLPoint);
			}
			else
			{
				//head
				trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->headBolt, &boltMatrix, render_angles, render_origin,
					level.time, NULL, self->modelScale);
				ri->headPoint[0] = boltMatrix.matrix[0][3];
				ri->headPoint[1] = boltMatrix.matrix[1][3];
				ri->headPoint[2] = boltMatrix.matrix[2][3];

				//right hand
				trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->handRBolt, &boltMatrix, render_angles, render_origin,
					level.time, NULL, self->modelScale);
				ri->handRPoint[0] = boltMatrix.matrix[0][3];
				ri->handRPoint[1] = boltMatrix.matrix[1][3];
				ri->handRPoint[2] = boltMatrix.matrix[2][3];

				//left hand
				trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->handLBolt, &boltMatrix, render_angles, render_origin,
					level.time, NULL, self->modelScale);
				ri->handLPoint[0] = boltMatrix.matrix[0][3];
				ri->handLPoint[1] = boltMatrix.matrix[1][3];
				ri->handLPoint[2] = boltMatrix.matrix[2][3];

				//chest
				trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->torsoBolt, &boltMatrix, render_angles, render_origin,
					level.time, NULL, self->modelScale);
				ri->torsoPoint[0] = boltMatrix.matrix[0][3];
				ri->torsoPoint[1] = boltMatrix.matrix[1][3];
				ri->torsoPoint[2] = boltMatrix.matrix[2][3];

				//crotch
				trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->crotchBolt, &boltMatrix, render_angles, render_origin,
					level.time, NULL, self->modelScale);
				ri->crotchPoint[0] = boltMatrix.matrix[0][3];
				ri->crotchPoint[1] = boltMatrix.matrix[1][3];
				ri->crotchPoint[2] = boltMatrix.matrix[2][3];

				//right foot
				trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->footRBolt, &boltMatrix, render_angles, render_origin,
					level.time, NULL, self->modelScale);
				ri->footRPoint[0] = boltMatrix.matrix[0][3];
				ri->footRPoint[1] = boltMatrix.matrix[1][3];
				ri->footRPoint[2] = boltMatrix.matrix[2][3];

				//left foot
				trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->footLBolt, &boltMatrix, render_angles, render_origin,
					level.time, NULL, self->modelScale);
				ri->footLPoint[0] = boltMatrix.matrix[0][3];
				ri->footLPoint[1] = boltMatrix.matrix[1][3];
				ri->footLPoint[2] = boltMatrix.matrix[2][3];
			}

			//Now draw the skel for debug
			G_TestLine(ri->headPoint, ri->torsoPoint, 0x000000ff, 50);
			G_TestLine(ri->torsoPoint, ri->handRPoint, 0x000000ff, 50);
			G_TestLine(ri->torsoPoint, ri->handLPoint, 0x000000ff, 50);
			G_TestLine(ri->torsoPoint, ri->crotchPoint, 0x000000ff, 50);
			G_TestLine(ri->crotchPoint, ri->footRPoint, 0x000000ff, 50);
			G_TestLine(ri->crotchPoint, ri->footLPoint, 0x000000ff, 50);
		}

		//muzzle point calc (we are going to be cheap here)
		VectorCopy(ri->muzzlePoint, ri->muzzlePointOld);
		VectorCopy(self->client->ps.origin, ri->muzzlePoint);
		VectorCopy(ri->muzzleDir, ri->muzzleDirOld);
		AngleVectors(self->client->ps.viewangles, ri->muzzleDir, 0, 0);
		ri->mPCalcTime = level.time;

		VectorCopy(self->client->ps.origin, ri->eyePoint);
		ri->eyePoint[2] += self->client->ps.viewheight;
	}
}

#define STAFF_KICK_RANGE 16
extern void G_GetBoltPosition(gentity_t* self, int boltIndex, vec3_t pos, int modelIndex); //NPC_utils.c

extern qboolean BG_InKnockDown(int anim);

static void WP_HolsterSaberAndPlayOffSounds(gentity_t* hit_ent)
{
	if (!hit_ent || !hit_ent->client)
	{
		return;
	}

	if (!hit_ent->client->ps.saberHolstered)
	{
		if (hit_ent->client->saber[0].soundOff)
		{
			G_Sound(hit_ent, CHAN_WEAPON, hit_ent->client->saber[0].soundOff);
		}
		if (hit_ent->client->saber[1].soundOff)
		{
			G_Sound(hit_ent, CHAN_WEAPON, hit_ent->client->saber[1].soundOff);
		}
		hit_ent->client->ps.saberHolstered = 2;
	}
}

static void WP_KnockdownAndDrain(gentity_t* hit_ent, const gentity_t* pusher, int knockAnimMin, int knockAnimMax, qboolean addFatigueBonus, qboolean useAbsorbDrain)
{
	if (!hit_ent || !hit_ent->client)
	{
		return;
	}

	G_SetAnim(hit_ent, &hit_ent->client->pers.cmd,
		SETANIM_BOTH,
		(knockAnimMin == knockAnimMax) ? knockAnimMin : Q_irand(knockAnimMin, knockAnimMax),
		SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD,
		0);

	if (useAbsorbDrain)
	{
		WP_BlockPointsDrain(hit_ent, FATIGUE_BP_ABSORB);
	}
	else
	{
		WP_BlockPointsDrain(hit_ent, FATIGUE_BLOCKPOINTDRAIN);
	}

	if (addFatigueBonus && pusher)
	{
		AddFatigueMeleeBonus(pusher, hit_ent);
	}

	G_Sound(hit_ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
	WP_HolsterSaberAndPlayOffSounds(hit_ent);
}

static qboolean WP_AbsorbKick(gentity_t* hit_ent, const gentity_t* pusher, const vec3_t push_dir)
{
	if (!hit_ent || !hit_ent->client || !pusher || !pusher->client)
	{
		return qfalse;
	}

	const qboolean is_holding_block_button_and_attack =
		(hit_ent->client->ps.ManualBlockingFlags & (1 << HOLDINGBLOCKANDATTACK)) ? qtrue : qfalse;
	const qboolean is_holding_block_button =
		(hit_ent->client->ps.ManualBlockingFlags & (1 << HOLDINGBLOCK)) ? qtrue : qfalse;
	const qboolean npc_blocking =
		(hit_ent->client->ps.ManualBlockingFlags & (1 << MBF_NPCKICKBLOCK)) ? qtrue : qfalse;

	const qboolean isPlayer = (qboolean)((hit_ent->r.svFlags & SVF_BOT) == 0);
	const qboolean isBot = (qboolean)((hit_ent->r.svFlags & SVF_BOT) != 0);

	// ============================================================
	// 1. Player absorb (manual block)
	// ============================================================
	if (PlayerCanAbsorbKick(hit_ent, push_dir) &&
		(is_holding_block_button || is_holding_block_button_and_attack) &&
		isPlayer)
	{
		if (hit_ent->client->ps.fd.blockPoints > 50)
		{
			G_Stagger(hit_ent);
		}
		else
		{
			G_Stumble(hit_ent);
		}

		WP_BlockPointsDrain(hit_ent, FATIGUE_BP_ABSORB);
		G_Sound(hit_ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
		return qtrue;
	}

	// ============================================================
	// 2. Bot / NPC absorb
	// ============================================================
	if (BotCanAbsorbKick(hit_ent, push_dir) && npc_blocking && isBot)
	{
		if (hit_ent->client->ps.fd.blockPoints > 50 && hit_ent->client->ps.fd.forcePower > 50)
		{
			G_Stagger(hit_ent);
			WP_BlockPointsDrain(hit_ent, FATIGUE_BP_ABSORB);
			WP_DeactivateSaber(hit_ent);
		}
		else
		{
			G_SetAnim(hit_ent, &hit_ent->client->pers.cmd,
				SETANIM_BOTH,
				Q_irand(BOTH_FLIP_BACK1, BOTH_FLIP_BACK2),
				SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD,
				0);
			WP_BlockPointsDrain(hit_ent, FATIGUE_BP_ABSORB);
		}

		G_Sound(hit_ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
		return qtrue;
	}

	// ============================================================
	// 3. Non‑absorb: actual hits
	// ============================================================

	// Uppercut / melee stance
	if (pusher->client->ps.torsoAnim == BOTH_MELEEUP ||
		pusher->client->ps.torsoAnim == MELEE_STANCE_HOLD_T ||
		pusher->client->ps.torsoAnim == MELEE_STANCE_HOLD_LT ||
		pusher->client->ps.torsoAnim == MELEE_STANCE_HOLD_RT ||
		pusher->client->ps.torsoAnim == MELEE_STANCE_HOLD_BR ||
		pusher->client->ps.torsoAnim == MELEE_STANCE_HOLD_BL ||
		pusher->client->ps.torsoAnim == MELEE_STANCE_HOLD_B ||
		pusher->client->ps.torsoAnim == MELEE_STANCE_T ||
		pusher->client->ps.torsoAnim == MELEE_STANCE_LT ||
		pusher->client->ps.torsoAnim == MELEE_STANCE_RT ||
		pusher->client->ps.torsoAnim == MELEE_STANCE_BR ||
		pusher->client->ps.torsoAnim == MELEE_STANCE_BL ||
		pusher->client->ps.torsoAnim == MELEE_STANCE_B)
	{
		WP_KnockdownAndDrain(hit_ent, pusher, BOTH_KNOCKDOWN1, BOTH_KNOCKDOWN2, qtrue, qfalse);
		return qtrue;
	}

	// Wookie slap / Tusken lunge
	if (pusher->client->ps.torsoAnim == BOTH_WOOKIE_SLAP ||
		pusher->client->ps.torsoAnim == BOTH_TUSKENLUNGE1)
	{
		WP_KnockdownAndDrain(hit_ent, pusher, BOTH_SLAPDOWNRIGHT, BOTH_SLAPDOWNLEFT, qtrue, qfalse);
		return qtrue;
	}

	// Front kick
	if (pusher->client->ps.legsAnim == BOTH_A7_KICK_F ||
		pusher->client->ps.legsAnim == BOTH_A7_KICK_F2 ||
		pusher->client->ps.torsoAnim == BOTH_TUSKENATTACK1 ||
		pusher->client->ps.torsoAnim == BOTH_TUSKENATTACK2 ||
		pusher->client->ps.torsoAnim == BOTH_TUSKENATTACK3)
	{
		WP_KnockdownAndDrain(hit_ent, pusher, BOTH_KNOCKDOWN1, BOTH_KNOCKDOWN2, qtrue, qfalse);

		if (pusher->client->ps.legsAnim == BOTH_A7_KICK_F2)
		{
			G_Sound(hit_ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/melee/swing%d", Q_irand(1, 4))));
		}
		else if (pusher->client->ps.torsoAnim == BOTH_TUSKENATTACK1 ||
			pusher->client->ps.torsoAnim == BOTH_TUSKENATTACK2 ||
			pusher->client->ps.torsoAnim == BOTH_TUSKENATTACK3)
		{
			G_Sound(hit_ent, CHAN_BODY, G_SoundIndex("sound/movers/objects/saber_slam"));
		}

		return qtrue;
	}

	// Roundhouse
	if (pusher->client->ps.legsAnim == BOTH_A7_KICK_B)
	{
		WP_KnockdownAndDrain(hit_ent, pusher, BOTH_KNOCKDOWN1, BOTH_KNOCKDOWN5, qtrue, qfalse);
		return qtrue;
	}

	// Backkick variants
	if (pusher->client->ps.legsAnim == BOTH_A7_KICK_B2)
	{
		WP_KnockdownAndDrain(hit_ent, pusher, BOTH_KNOCKDOWN5, BOTH_KNOCKDOWN5, qtrue, qfalse);
		return qtrue;
	}

	if (pusher->client->ps.legsAnim == BOTH_A7_KICK_B3)
	{
		WP_KnockdownAndDrain(hit_ent, pusher, BOTH_KNOCKDOWN4, BOTH_KNOCKDOWN4, qtrue, qfalse);
		return qtrue;
	}

	// Side kicks
	if (pusher->client->ps.legsAnim == BOTH_A7_KICK_R ||
		pusher->client->ps.legsAnim == BOTH_A7_KICK_L)
	{
		WP_KnockdownAndDrain(hit_ent, pusher, BOTH_KNOCKDOWN2, BOTH_KNOCKDOWN2, qtrue, qfalse);
		return qtrue;
	}

	// Right slap
	if (pusher->client->ps.torsoAnim == BOTH_A7_SLAP_R ||
		pusher->client->ps.torsoAnim == BOTH_SMACK_R)
	{
		WP_KnockdownAndDrain(hit_ent, pusher, BOTH_SLAPDOWNRIGHT, BOTH_SLAPDOWNRIGHT, qtrue, qfalse);
		return qtrue;
	}

	// Left slap
	if (pusher->client->ps.torsoAnim == BOTH_A7_SLAP_L ||
		pusher->client->ps.torsoAnim == BOTH_SMACK_L)
	{
		WP_KnockdownAndDrain(hit_ent, pusher, BOTH_SLAPDOWNLEFT, BOTH_SLAPDOWNLEFT, qtrue, qfalse);
		return qtrue;
	}

	// Hilt bash
	if (pusher->client->ps.torsoAnim == BOTH_A7_HILT)
	{
		G_SetAnim(hit_ent, &hit_ent->client->pers.cmd,
			SETANIM_BOTH,
			BOTH_BASHED1,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD,
			0);

		WP_BlockPointsDrain(hit_ent, FATIGUE_BLOCKPOINTDRAIN);
		AddFatigueMeleeBonus(pusher, hit_ent);
		G_Sound(hit_ent, CHAN_BODY, G_SoundIndex("sound/movers/objects/saber_slam"));

		return qtrue;
	}

	// Default: generic knockdown
	WP_KnockdownAndDrain(hit_ent, pusher, BOTH_KNOCKDOWN1, BOTH_KNOCKDOWN2, qfalse, qtrue);
	return qtrue;
}

extern void G_ThrownDeathAnimForDeathAnim(gentity_t* hit_ent, vec3_t impactPoint);

extern void g_kick_throw(gentity_t* targ, const vec3_t new_dir, float push);

static gentity_t* G_KickTrace(gentity_t* ent, vec3_t kick_dir, const float kick_dist, vec3_t kick_end, const int kick_damage,
	const float kick_push,
	const qboolean do_sound_on_walls)
{
	vec3_t trace_org, trace_end;
	const vec3_t kick_maxs = { 4, 4, 4 };
	const vec3_t kick_mins = { -2, -2, -2 };
	trace_t trace;
	gentity_t* hit_ent = NULL;

	if (kick_end && !VectorCompare(kick_end, vec3_origin))
	{
		//they passed us the end point of the trace, just use that
		//this makes the trace flat
		VectorSet(trace_org, ent->r.currentOrigin[0], ent->r.currentOrigin[1], kick_end[2]);
		VectorCopy(kick_end, trace_end);
	}
	else
	{
		//extrude
		VectorSet(trace_org, ent->r.currentOrigin[0], ent->r.currentOrigin[1],
			ent->r.currentOrigin[2] + ent->maxs[2] * 0.5f);
		VectorMA(trace_org, kick_dist, kick_dir, trace_end);
	}

	if (d_saberKickTweak.integer)
	{
		trap->Trace(&trace, trace_org, kick_mins, kick_maxs, trace_end, ent->s.number, MASK_SHOT, qfalse,
			G2TRFLAG_DOGHOULTRACE | G2TRFLAG_GETSURFINDEX | G2TRFLAG_THICK | G2TRFLAG_HITCORPSES,
			g_g2TraceLod.integer);
	}
	else
	{
		trap->Trace(&trace, trace_org, kick_mins, kick_maxs, trace_end, ent->s.number, MASK_SHOT, qfalse, 0, 0);
	}

	if (trace.fraction < 1.0f || trace.startsolid && trace.entityNum < ENTITYNUM_NONE)
	{
		if (ent->client->jediKickTime > level.time)
		{
			if (trace.entityNum == ent->client->jediKickIndex)
			{
				//we are hitting the same ent we last hit in this same anim, don't hit it again
				return NULL;
			}
			TIMER_Remove(ent, "kickSoundDebounce");
		}
		ent->client->jediKickIndex = trace.entityNum;
		ent->client->jediKickTime = level.time + ent->client->ps.legsTimer;

		hit_ent = &g_entities[trace.entityNum];

		if (hit_ent->inuse)
		{
			//we hit an entity
			if (hit_ent->client)
			{
				if (!(hit_ent->client->ps.pm_flags & PMF_TIME_KNOCKBACK)
					&& TIMER_Done(hit_ent, "kickedDebounce") && G_CanBeEnemy(ent, hit_ent))
				{
					if (PM_InKnockDown(&hit_ent->client->ps) && !PM_InGetUp(&hit_ent->client->ps))
					{
						//don't hit people who are knocked down or being knocked down (okay to hit people getting up, though)
						return NULL;
					}
					if (PM_InRoll(&hit_ent->client->ps))
					{
						//can't hit people who are rolling
						return NULL;
					}
					if (PM_Dyinganim(&hit_ent->client->ps))
					{
						//don't hit people who are in a death anim
						return NULL;
					}
					//don't hit same ent more than once per kick
					if (hit_ent->takedamage)
					{
						//hurt it
						if (!in_camera && WP_AbsorbKick(hit_ent, ent, kick_dir))
						{
							//but the lucky devil absorbed it by backflipping
							//toss them back a bit and make sure that the kicker gets the kill if the player falls off a cliff or something.
							if (hit_ent->client)
							{
								hit_ent->client->ps.otherKiller = ent->s.number;
								hit_ent->client->ps.otherKillerDebounceTime = level.time + 10000;
								hit_ent->client->ps.otherKillerTime = level.time + 10000;
								hit_ent->client->otherKillerMOD = MOD_MELEE;
								hit_ent->client->otherKillerVehWeapon = 0;
								hit_ent->client->otherKillerWeaponType = WP_NONE;
								hit_ent->client->ps.saber_move = LS_READY;
							}
							g_kick_throw(hit_ent, kick_dir, 70);
							return hit_ent;
						}
						G_Damage(hit_ent, ent, ent, kick_dir, trace.endpos, kick_damage * 0.2f, DAMAGE_NO_KNOCKBACK,
							MOD_MELEE);
					}
					//do kick hit sound and impact effect
					if (TIMER_Done(ent, "kickSoundDebounce"))
					{
						if (ent->client->ps.torsoAnim == BOTH_A7_HILT)
						{
							G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/movers/objects/saber_slam"));
						}
						else
						{
							vec3_t fx_org, fx_dir;
							VectorCopy(kick_dir, fx_dir);
							VectorMA(trace.endpos, Q_flrand(5.0f, 10.0f), fx_dir, fx_org);
							VectorScale(fx_dir, -1, fx_dir);
							G_PlayEffectID(G_EffectIndex("melee/kick_impact.efx"), fx_org, fx_dir);
							G_Sound(ent, CHAN_AUTO, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
						}
						TIMER_Set(ent, "kickSoundDebounce", 1000);
					}
					TIMER_Set(hit_ent, "kickedDebounce", 500);

					if (ent->client->ps.torsoAnim == BOTH_A7_HILT ||
						ent->client->ps.torsoAnim == BOTH_SMACK_L ||
						ent->client->ps.torsoAnim == BOTH_SMACK_R ||
						ent->client->ps.torsoAnim == BOTH_A7_SLAP_R ||
						ent->client->ps.torsoAnim == BOTH_A7_SLAP_L)
					{
						//hit in head
						if (hit_ent->health > 0)
						{
							//knock down
							if (kick_push >= 150.0f && !Q_irand(0, 1))
							{
								//knock them down
								if (!(hit_ent->flags & FL_NO_KNOCKBACK))
								{
									g_kick_throw(hit_ent, kick_dir, 80);
								}
								G_Knockdown(hit_ent, ent, kick_dir, 80, qtrue);
							}
							else
							{
								//force them to play a pain anim
								if (hit_ent->s.number < MAX_CLIENTS)
								{
									NPC_SetPainEvent(hit_ent);
								}
								else
								{
									NPC_SetPainEvent(hit_ent);
									//GEntity_PainFunc(hit_ent, ent, ent, hit_ent->r.currentOrigin, 0, MOD_MELEE);
								}
							}
							//just so we don't hit him again...
							hit_ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
							hit_ent->client->ps.pm_time = 100;
						}
						else
						{
							if (!(hit_ent->flags & FL_NO_KNOCKBACK))
							{
								g_kick_throw(hit_ent, kick_dir, 80);
							}
							//see if we should play a better looking death on them
							G_ThrownDeathAnimForDeathAnim(hit_ent, trace.endpos);
						}
					}
					else if (ent->client->ps.legsAnim == BOTH_GETUP_BROLL_B
						|| ent->client->ps.legsAnim == BOTH_GETUP_BROLL_F
						|| ent->client->ps.legsAnim == BOTH_GETUP_FROLL_B
						|| ent->client->ps.legsAnim == BOTH_GETUP_FROLL_F)
					{
						if (hit_ent->health > 0)
						{
							//knock down
							if (hit_ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								//he's in the air?  Send him flying back
								if (!(hit_ent->flags & FL_NO_KNOCKBACK))
								{
									g_kick_throw(hit_ent, kick_dir, 80);
								}
							}
							else
							{
								//just so we don't hit him again...
								hit_ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
								hit_ent->client->ps.pm_time = 100;
							}
							//knock them down
							G_Knockdown(hit_ent, ent, kick_dir, 80, qtrue);
						}
						else
						{
							if (!(hit_ent->flags & FL_NO_KNOCKBACK))
							{
								g_kick_throw(hit_ent, kick_dir, 80);
							}
							//see if we should play a better looking death on them
							G_ThrownDeathAnimForDeathAnim(hit_ent, trace.endpos);
						}
					}
					else if (hit_ent->health <= 0)
					{
						//we kicked a dead guy
						if (!(hit_ent->flags & FL_NO_KNOCKBACK))
						{
							g_kick_throw(hit_ent, kick_dir, kick_push * 2);
						}
						G_ThrownDeathAnimForDeathAnim(hit_ent, trace.endpos);
					}
					else
					{
						if (!(hit_ent->flags & FL_NO_KNOCKBACK))
						{
							g_kick_throw(hit_ent, kick_dir, 80);
						}
						if ((hit_ent->client->ps.saberFatigueChainCount >= MISHAPLEVEL_HEAVY ||
							hit_ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
							&& !(hit_ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
						{
							//knockdown
							if (hit_ent->client->ps.fd.saberAnimLevel == SS_STAFF)
							{
								sab_beh_animate_slow_bounce_blocker(hit_ent);
							}
							else
							{
								if (kick_push >= 150.0f && !Q_irand(0, 2))
								{
									G_Knockdown(hit_ent, ent, kick_dir, 90, qtrue);
								}
								else
								{
									G_Knockdown(hit_ent, ent, kick_dir, 80, qtrue);
								}
							}
						}
						else if (ent->client->ps.fd.saberAnimLevel == SS_DESANN
							&& (hit_ent->client->ps.saberFatigueChainCount >= MISHAPLEVEL_HEAVY ||
								hit_ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
							&& !(hit_ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
						{
							//knockdown
							if (kick_push >= 150.0f && !Q_irand(0, 2))
							{
								G_Knockdown(hit_ent, ent, kick_dir, 90, qtrue);
							}
							else
							{
								G_Knockdown(hit_ent, ent, kick_dir, 80, qtrue);
							}
						}
						else if (kick_push >= 150.0f && !Q_irand(0, 2))
						{
							G_Knockdown(hit_ent, ent, kick_dir, 90, qtrue);
						}
						else
						{
							//stumble
							AnimateStun(hit_ent, ent, trace.endpos);
						}
					}
				}
			}
			else
			{
				if (do_sound_on_walls)
				{
					//do kick hit sound and impact effect
					if (TIMER_Done(ent, "kickSoundDebounce"))
					{
						if (ent->client->ps.torsoAnim == BOTH_A7_HILT)
						{
							G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/movers/objects/saber_slam"));
						}
						else
						{
							G_Sound(ent, CHAN_AUTO, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
							//G_PlayEffect(G_EffectIndex("melee/kick_impact.efx"), trace.endpos, trace.plane.normal);
						}
						TIMER_Set(ent, "kickSoundDebounce", 1000);
					}
				}
			}
		}
	}
	return hit_ent;
}

static void G_PunchSomeMofos(gentity_t* ent)
{
	trace_t tr;
	const renderInfo_t* ri = &ent->client->renderInfo;
	int boltIndex;

	if (ent->client->ps.torsoAnim == BOTH_MELEE1 ||
		ent->client->ps.torsoAnim == BOTH_MELEE3 ||
		ent->client->ps.torsoAnim == BOTH_MELEE_L ||
		ent->client->ps.torsoAnim == BOTH_MELEE_R)
	{
		boltIndex = ri->handLBolt;
	}
	else
	{
		boltIndex = ri->handRBolt;
	}

	if (boltIndex != -1)
	{
		vec3_t hand;
		vec3_t maxs;
		vec3_t mins;
		//safety check, this shouldn't ever not be valid
		G_GetBoltPosition(ent, boltIndex, hand, 0);

		//Set bbox size
		VectorSet(maxs, 6, 6, 6);
		VectorScale(maxs, -1, mins);

		trap->Trace(&tr, ent->client->ps.origin, mins, maxs, hand, ent->s.number, MASK_SHOT, qfalse,
			G2TRFLAG_DOGHOULTRACE | G2TRFLAG_GETSURFINDEX | G2TRFLAG_THICK | G2TRFLAG_HITCORPSES,
			g_g2TraceLod.integer);

		if (tr.fraction != 1)
		{
			//hit something
			vec3_t forward;
			gentity_t* tr_ent = &g_entities[tr.entityNum];

			VectorSubtract(hand, ent->client->ps.origin, forward);
			VectorNormalize(forward);

			//Prevent punches from hitting the same entity more than once
			if (ent->client->jediKickTime > level.time)
			{
				if (tr.entityNum == ent->client->jediKickIndex)
				{
					//we are hitting the same ent we last hit in this same anim, don't hit it again
					return;
				}
			}
			ent->client->jediKickIndex = tr.entityNum;
			ent->client->jediKickTime = level.time + ent->client->ps.torsoTimer;

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

				tr_ent->client->ps.otherKiller = ent->s.number;
				tr_ent->client->ps.otherKillerDebounceTime = level.time + 10000;
				tr_ent->client->ps.otherKillerTime = level.time + 10000;
				tr_ent->client->otherKillerMOD = MOD_MELEE;
				tr_ent->client->otherKillerVehWeapon = 0;
				tr_ent->client->otherKillerWeaponType = WP_NONE;
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
}

static void G_KickSomeMofos(gentity_t* ent)
{
	vec3_t kick_dir, kick_end, kick_dir2, kick_end2, fwd_angs;
	float anim_length = BG_AnimLength(ent->localAnimIndex, ent->client->ps.legsAnim);
	float elapsed_time = anim_length - ent->client->ps.legsTimer;
	float remaining_time = anim_length - elapsed_time;
	float kick_dist = ent->r.maxs[0] * 1.5f + (m_nerf.integer & 1 << EOC_STAFFKICK
		? STAFF_KICK_RANGE
		: 2.25 * STAFF_KICK_RANGE) + 8.0f; //fudge factor of 8
	float kick_dist2 = kick_dist;
	int kick_damage = Q_irand(3, 8);
	int kick_damage2 = Q_irand(3, 8);
	int kick_push = flrand(50.0f, 100.0f);
	int kick_push2 = flrand(50.0f, 100.0f);
	qboolean do_kick = qfalse;
	qboolean do_kick2 = qfalse;
	renderInfo_t* ri = &ent->client->renderInfo;
	qboolean kick_sound_on_walls = qfalse;

	VectorSet(kick_dir, 0.0f, 0.0f, 0.0f);
	VectorSet(kick_end, 0.0f, 0.0f, 0.0f);
	VectorSet(kick_dir2, 0.0f, 0.0f, 0.0f);
	VectorSet(kick_end2, 0.0f, 0.0f, 0.0f);
	VectorSet(fwd_angs, 0.0f, ent->client->ps.viewangles[YAW], 0.0f);

	if (ent->client->ps.torsoAnim == BOTH_A7_HILT)
	{
		if (elapsed_time >= 250 && remaining_time >= 250)
		{
			//front
			do_kick = qtrue;
			if (ri->handRBolt != -1)
			{
				//actually trace to a bolt
				G_GetBoltPosition(ent, ri->handRBolt, kick_end, 0);
				VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
				kick_dir[2] = 0; //ah, flatten it, I guess...
				VectorNormalize(kick_dir);
				//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
				VectorMA(kick_end, 8.0f, kick_dir, kick_end);
			}
			else
			{
				//guess
				AngleVectors(fwd_angs, kick_dir, NULL, NULL);
			}
		}
	}
	else
	{
		switch (ent->client->ps.legsAnim)
		{
		case BOTH_A7_SOULCAL:
			kick_push = flrand(150.0f, 250.0f);
			if (elapsed_time >= 1400 && elapsed_time <= 1500)
			{
				//right leg
				do_kick = qtrue;
				if (ri->footRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
			}
			break;
		case BOTH_ARIAL_F1:
			if (elapsed_time >= 550 && elapsed_time <= 1000)
			{
				//right leg
				do_kick = qtrue;
				if (ri->footRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
			}
			if (elapsed_time >= 800 && elapsed_time <= 1200)
			{
				//left leg
				do_kick2 = qtrue;
				if (ri->footLBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footLBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir2[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir2);
					VectorMA(kick_end2, 8.0f, kick_dir2, kick_end2);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir2, NULL, NULL);
				}
			}
			break;
		case BOTH_ARIAL_LEFT:
		case BOTH_ARIAL_RIGHT:
			if (elapsed_time >= 200 && elapsed_time <= 600)
			{
				//lead leg
				int foot_bolt = ent->client->ps.legsAnim == BOTH_ARIAL_LEFT ? ri->footLBolt : ri->footRBolt;
				//mirrored anims
				do_kick = qtrue;
				if (foot_bolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, foot_bolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
			}
			if (elapsed_time >= 400 && elapsed_time <= 850)
			{
				//trailing leg
				int foot_bolt = ent->client->ps.legsAnim == BOTH_ARIAL_LEFT ? ri->footLBolt : ri->footRBolt;
				//mirrored anims
				do_kick2 = qtrue;
				if (foot_bolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, foot_bolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir2);
					VectorMA(kick_end, 8.0f, kick_dir2, kick_end2);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
			}
			break;
		case BOTH_CARTWHEEL_LEFT:
		case BOTH_CARTWHEEL_RIGHT:
			if (elapsed_time >= 200 && elapsed_time <= 600)
			{
				//lead leg
				int foot_bolt = ent->client->ps.legsAnim == BOTH_CARTWHEEL_LEFT ? ri->footLBolt : ri->footRBolt;
				//mirrored anims
				do_kick = qtrue;
				if (foot_bolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, foot_bolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
			}
			if (elapsed_time >= 350 && elapsed_time <= 650)
			{
				//trailing leg
				int foot_bolt = ent->client->ps.legsAnim == BOTH_CARTWHEEL_LEFT ? ri->footLBolt : ri->footRBolt;
				//mirrored anims
				do_kick2 = qtrue;
				if (foot_bolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, foot_bolt, kick_end2, 0);
					VectorSubtract(kick_end2, ent->r.currentOrigin, kick_dir2);
					kick_dir2[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir2);
					VectorMA(kick_end2, 8.0f, kick_dir2, kick_end2);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir2, NULL, NULL);
				}
			}
			break;
		case BOTH_JUMPATTACK7:
			if (elapsed_time >= 300 && elapsed_time <= 900)
			{
				//right knee
				do_kick = qtrue;
				if (ri->footRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
			}
			if (elapsed_time >= 600 && elapsed_time <= 900)
			{
				//left leg
				do_kick2 = qtrue;
				if (ri->footLBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footLBolt, kick_end2, 0);
					VectorSubtract(kick_end2, ent->r.currentOrigin, kick_dir2);
					kick_dir2[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir2);
					VectorMA(kick_end2, 8.0f, kick_dir2, kick_end2);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir2, NULL, NULL);
				}
			}
			break;
		case BOTH_BUTTERFLY_FL1:
		case BOTH_BUTTERFLY_FR1:
			if (elapsed_time >= 950 && elapsed_time <= 1300)
			{
				//lead leg
				int foot_bolt = ent->client->ps.legsAnim == BOTH_BUTTERFLY_FL1 ? ri->footLBolt : ri->footRBolt;
				//mirrored anims
				do_kick = qtrue;
				if (foot_bolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, foot_bolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
			}
			if (elapsed_time >= 1150 && elapsed_time <= 1600)
			{
				//trailing leg
				int foot_bolt = ent->client->ps.legsAnim == BOTH_BUTTERFLY_FL1 ? ri->footLBolt : ri->footRBolt;
				//mirrored anims
				do_kick2 = qtrue;
				if (foot_bolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, foot_bolt, kick_end2, 0);
					VectorSubtract(kick_end2, ent->r.currentOrigin, kick_dir2);
					kick_dir2[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir2);
					VectorMA(kick_end2, 8.0f, kick_dir2, kick_end2);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir2, NULL, NULL);
				}
			}
			break;
		case BOTH_BUTTERFLY_LEFT:
		case BOTH_BUTTERFLY_RIGHT:
			if (elapsed_time >= 100 && elapsed_time <= 450
				|| elapsed_time >= 1100 && elapsed_time <= 1350)
			{
				//lead leg
				int foot_bolt = ent->client->ps.legsAnim == BOTH_BUTTERFLY_LEFT ? ri->footLBolt : ri->footRBolt;
				//mirrored anims
				do_kick = qtrue;
				if (foot_bolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, foot_bolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
			}
			if (elapsed_time >= 900 && elapsed_time <= 1600)
			{
				//trailing leg
				int foot_bolt = ent->client->ps.legsAnim == BOTH_BUTTERFLY_LEFT ? ri->footLBolt : ri->footRBolt;
				//mirrored anims
				do_kick2 = qtrue;
				if (foot_bolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, foot_bolt, kick_end2, 0);
					VectorSubtract(kick_end2, ent->r.currentOrigin, kick_dir2);
					kick_dir2[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir2);
					VectorMA(kick_end2, 8.0f, kick_dir2, kick_end2);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir2, NULL, NULL);
				}
			}
			break;
		case BOTH_GETUP_BROLL_B:
		case BOTH_GETUP_BROLL_F:
		case BOTH_GETUP_FROLL_B:
		case BOTH_GETUP_FROLL_F:
			kick_push = flrand(150.0f, 250.0f); //Q_flrand( 75.0f, 125.0f );
			if (elapsed_time >= 250 && remaining_time >= 250)
			{
				//front
				do_kick = qtrue;
				if (ri->footRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
			}
			break;
		case BOTH_A7_KICK_F_AIR2:
		case BOTH_A7_KICK_F_AIR:
			kick_push = flrand(150.0f, 250.0f);
			kick_sound_on_walls = qtrue;
			if (elapsed_time >= 100 && remaining_time >= 500)
			{
				//front
				do_kick = qtrue;
				if (ri->footRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
			}
			break;
		case BOTH_A7_KICK_F:
		case BOTH_A7_KICK_F2:
			kick_sound_on_walls = qtrue;
			if (elapsed_time >= 250 && remaining_time >= 250)
			{
				//front
				do_kick = qtrue;
				if (ri->footRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
			}
			break;
		case BOTH_A7_KICK_B_AIR:
			kick_push = flrand(150.0f, 250.0f); //Q_flrand( 75.0f, 125.0f );
			kick_sound_on_walls = qtrue;
			if (elapsed_time >= 100 && remaining_time >= 400)
			{
				//back
				do_kick = qtrue;
				if (ri->footRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
					VectorScale(kick_dir, -1, kick_dir);
				}
			}
			break;
		case BOTH_A7_KICK_B:
		case BOTH_A7_KICK_B3:
		case BOTH_A7_KICK_B2:
			kick_sound_on_walls = qtrue;
			if (elapsed_time >= 250 && remaining_time >= 250)
			{
				//back
				do_kick = qtrue;
				if (ri->footRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
					VectorScale(kick_dir, -1, kick_dir);
				}
			}
			break;
		case BOTH_A7_KICK_R_AIR:
			kick_push = flrand(150.0f, 250.0f);
			kick_sound_on_walls = qtrue;
			if (elapsed_time >= 150 && remaining_time >= 300)
			{
				//left
				do_kick = qtrue;
				if (ri->footRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, NULL, kick_dir, NULL);
					VectorScale(kick_dir, -1, kick_dir);
				}
			}
			break;
		case BOTH_A7_KICK_R:
			kick_sound_on_walls = qtrue;
			//FIXME: push right?
			if (elapsed_time >= 250 && remaining_time >= 250)
			{
				//right
				do_kick = qtrue;
				if (ri->footRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, NULL, kick_dir, NULL);
				}
			}
			break;
		case BOTH_A7_SLAP_R:
			if (level.framenum & 1)
			{
				//back
				int handBolt = ent->client->ps.legsAnim == BOTH_A7_SLAP_R ? ri->handRBolt : ri->handLBolt;
				//mirrored anims
				do_kick = qtrue;
				kick_dist = 80;
				if (handBolt != -1) //changed to accomodate teh new hand hand anim
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, handBolt, kick_end, 0); //changed to accomodate teh new hand hand anim
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 20.0f, kick_dir, kick_end);
				}
			}
			else if (ri->handLBolt != -1) //for hand slap
			{
				//actually trace to a bolt
				if (elapsed_time >= 250 && remaining_time >= 250)
				{
					//right, though either would do
					do_kick = qtrue;
					kick_dist = 80;
					G_GetBoltPosition(ent, ri->handLBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0;
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 20.0f, kick_dir, kick_end);
				}
			}
			else if (ri->elbowLBolt != -1) //for hand slap
			{
				//actually trace to a bolt
				if (elapsed_time >= 250 && remaining_time >= 250)
				{
					//right, though either would do
					do_kick = qtrue;
					kick_dist = 80;
					G_GetBoltPosition(ent, ri->elbowLBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
					VectorMA(kick_end, 20.0f, kick_dir, kick_end);
				}
			}
			break;
		case BOTH_KYLE_GRAB: //right slap
		case BOTH_KYLE_MISS: //right slap
		case BOTH_KYLE_PA_1: //right slap
		case BOTH_KYLE_PA_2: //right slap
		case BOTH_KYLE_PA_3: //right slap
		case MELEE_STANCE_HOLD_T: //right slap
		case MELEE_STANCE_HOLD_RT: //right slap
		case MELEE_STANCE_HOLD_BR:
		case MELEE_STANCE_T: //right slap
		case MELEE_STANCE_RT: //right slap
		case MELEE_STANCE_BR:
		case BOTH_MELEE_R://right slap
		case BOTH_MELEE2://right slap
		case BOTH_MELEE4://right slap
		case BOTH_MELEEUP://right slap
		case BOTH_TUSKENLUNGE1:
		case BOTH_TUSKENATTACK2:
		case BOTH_WOOKIE_SLAP:
		case BOTH_FJSS_TL_BR:
		case BOTH_SMACK_R:
			kick_sound_on_walls = qtrue;
			//FIXME: push right?
			if (level.framenum & 1)
			{
				//right
				do_kick = qtrue;
				if (ri->handRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->handRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 20.0f, kick_dir, kick_end);
				}
				else if (ri->elbowRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->elbowRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 20.0f, kick_dir, kick_end);
				}
				else if (ri->handLBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->handLBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 20.0f, kick_dir, kick_end);
				}
				else if (ri->elbowLBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->elbowLBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 20.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, NULL, kick_dir, NULL);
				}
			}
			break;
		case BOTH_A7_KICK_L_AIR:
			kick_push = flrand(150.0f, 250.0f); //Q_flrand( 75.0f, 125.0f );
			kick_sound_on_walls = qtrue;
			if (elapsed_time >= 150 && remaining_time >= 300)
			{
				//left
				do_kick = qtrue;
				if (ri->footLBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footLBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, NULL, kick_dir, NULL);
					VectorScale(kick_dir, -1, kick_dir);
				}
			}
			break;
		case BOTH_A7_KICK_L:
			kick_sound_on_walls = qtrue;
			//FIXME: push left?
			if (elapsed_time >= 250 && remaining_time >= 250)
			{
				//left
				do_kick = qtrue;
				if (ri->footLBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footLBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, NULL, kick_dir, NULL);
					VectorScale(kick_dir, -1, kick_dir);
				}
			}
			break;
		case BOTH_A7_SLAP_L:
			if (level.framenum & 1)
			{
				//back
				int handBolt = ent->client->ps.legsAnim == BOTH_A7_SLAP_L ? ri->handLBolt : ri->handRBolt;
				//mirrored anims
				do_kick = qtrue;
				kick_dist = 80;
				if (handBolt != -1) //changed to accomodate teh new hand hand anim
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, handBolt, kick_end, 0); //changed to accomodate teh new hand hand anim
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 20.0f, kick_dir, kick_end);
				}
			}
			else if (ri->handLBolt != -1) //for hand slap
			{
				//actually trace to a bolt
				if (elapsed_time >= 250 && remaining_time >= 250)
				{
					//right, though either would do
					do_kick = qtrue;
					kick_dist = 80;
					G_GetBoltPosition(ent, ri->handLBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
					VectorMA(kick_end, 20.0f, kick_dir, kick_end);
				}
			}
			else if (ri->elbowLBolt != -1) //for hand slap
			{
				//actually trace to a bolt
				if (elapsed_time >= 250 && remaining_time >= 250)
				{
					//right, though either would do
					do_kick = qtrue;
					kick_dist = 80;
					G_GetBoltPosition(ent, ri->elbowLBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
					VectorMA(kick_end, 20.0f, kick_dir, kick_end);
				}
			}
			break;
		case MELEE_STANCE_HOLD_LT: //left slap
		case MELEE_STANCE_HOLD_BL: //left slap
		case MELEE_STANCE_HOLD_B: //left slap
		case MELEE_STANCE_LT: //left slap
		case MELEE_STANCE_BL: //left slap
		case MELEE_STANCE_B: //left slap
		case BOTH_MELEE_L://left slap
		case BOTH_MELEE1://left slap
		case BOTH_MELEE3://left slap
		case BOTH_TUSKENATTACK1:
		case BOTH_TUSKENATTACK3:
		case BOTH_FJSS_TR_BL:
		case BOTH_SMACK_L:
			kick_sound_on_walls = qtrue;
			//FIXME: push left?
			if (level.framenum & 1)
			{
				//right
				do_kick = qtrue;
				if (ri->handLBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->handLBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 20.0f, kick_dir, kick_end);
				}
				else if (ri->handRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->handRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					VectorMA(kick_end, 20.0f, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, NULL, kick_dir, NULL);
					VectorScale(kick_dir, -1, kick_dir);
				}
			}
			break;
		case BOTH_A7_KICK_S:
			kick_push = flrand(150.0f, 250.0f); //Q_flrand( 75.0f, 125.0f );
			if (ri->footRBolt != -1)
			{
				//actually trace to a bolt
				if (elapsed_time >= 550
					&& elapsed_time <= 1050)
				{
					do_kick = qtrue;
					G_GetBoltPosition(ent, ri->footRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
					VectorMA(kick_end, 8.0f, kick_dir, kick_end);
				}
			}
			else
			{
				//guess
				if (elapsed_time >= 400 && elapsed_time < 500)
				{
					//front
					do_kick = qtrue;
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
				else if (elapsed_time >= 500 && elapsed_time < 600)
				{
					//front-right?
					do_kick = qtrue;
					fwd_angs[YAW] += 45;
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
				else if (elapsed_time >= 600 && elapsed_time < 700)
				{
					//right
					do_kick = qtrue;
					AngleVectors(fwd_angs, NULL, kick_dir, NULL);
				}
				else if (elapsed_time >= 700 && elapsed_time < 800)
				{
					//back-right?
					do_kick = qtrue;
					fwd_angs[YAW] += 45;
					AngleVectors(fwd_angs, NULL, kick_dir, NULL);
				}
				else if (elapsed_time >= 800 && elapsed_time < 900)
				{
					//back
					do_kick = qtrue;
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
					VectorScale(kick_dir, -1, kick_dir);
				}
				else if (elapsed_time >= 900 && elapsed_time < 1000)
				{
					//back-left?
					do_kick = qtrue;
					fwd_angs[YAW] += 45;
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
				else if (elapsed_time >= 1000 && elapsed_time < 1100)
				{
					//left
					do_kick = qtrue;
					AngleVectors(fwd_angs, NULL, kick_dir, NULL);
					VectorScale(kick_dir, -1, kick_dir);
				}
				else if (elapsed_time >= 1100 && elapsed_time < 1200)
				{
					//front-left?
					do_kick = qtrue;
					fwd_angs[YAW] += 45;
					AngleVectors(fwd_angs, NULL, kick_dir, NULL);
					VectorScale(kick_dir, -1, kick_dir);
				}
			}
			break;
		case BOTH_A7_KICK_BF:
			kick_push = flrand(150.0f, 250.0f); //Q_flrand( 75.0f, 125.0f );
			if (elapsed_time < 1500)
			{
				//auto-aim!
				//overridAngles = PM_AdjustAnglesForBFKick(ent, ucmd, fwdAngs, qboolean(elapsedTime<850)) ? qtrue : overridAngles;
			}
			if (ri->footRBolt != -1)
			{
				//actually trace to a bolt
				if (elapsed_time >= 750 && elapsed_time < 850
					|| elapsed_time >= 1400 && elapsed_time < 1500)
				{
					//right, though either would do
					do_kick = qtrue;
					G_GetBoltPosition(ent, ri->footRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
					VectorMA(kick_end, 8, kick_dir, kick_end);
				}
			}
			else
			{
				//guess
				if (elapsed_time >= 250 && elapsed_time < 350)
				{
					//front
					do_kick = qtrue;
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
				}
				else if (elapsed_time >= 350 && elapsed_time < 450)
				{
					//back
					do_kick = qtrue;
					AngleVectors(fwd_angs, kick_dir, NULL, NULL);
					VectorScale(kick_dir, -1, kick_dir);
				}
			}
			break;
		case BOTH_A7_KICK_RL:
			kick_sound_on_walls = qtrue;
			kick_push = flrand(150.0f, 250.0f);
			if (elapsed_time >= 250 && elapsed_time < 350)
			{
				//right
				do_kick = qtrue;
				if (ri->footRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footRBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
					VectorMA(kick_end, 8, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, NULL, kick_dir, NULL);
				}
			}
			else if (elapsed_time >= 350 && elapsed_time < 450)
			{
				//left
				do_kick = qtrue;
				if (ri->footLBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ri->footLBolt, kick_end, 0);
					VectorSubtract(kick_end, ent->r.currentOrigin, kick_dir);
					kick_dir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kick_dir);
					//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
					VectorMA(kick_end, 8, kick_dir, kick_end);
				}
				else
				{
					//guess
					AngleVectors(fwd_angs, NULL, kick_dir, NULL);
					VectorScale(kick_dir, -1, kick_dir);
				}
			}
			break;
		default:;
		}
	}

	if (do_kick)
	{
		G_KickTrace(ent, kick_dir, kick_dist, kick_end, kick_damage, kick_push, kick_sound_on_walls);
	}
	if (do_kick2)
	{
		G_KickTrace(ent, kick_dir2, kick_dist2, kick_end2, kick_damage2, kick_push2, kick_sound_on_walls);
	}
}

static QINLINE qboolean G_PrettyCloseIGuess(float a, const float b, const float tolerance)
{
	if (a - b < tolerance &&
		a - b > -tolerance)
	{
		return qtrue;
	}

	return qfalse;
}

static void G_GrabSomeMofos(gentity_t* self)
{
	renderInfo_t* ri = &self->client->renderInfo;
	mdxaBone_t boltMatrix;
	vec3_t flatAng;
	vec3_t pos;
	vec3_t grabMins, grabMaxs;
	trace_t trace;

	if (!self->ghoul2 || ri->handRBolt == -1)
	{ //no good
		return;
	}

	VectorSet(flatAng, 0.0f, self->client->ps.viewangles[1], 0.0f);
	trap->G2API_GetBoltMatrix(self->ghoul2, 0, ri->handRBolt, &boltMatrix, flatAng, self->client->ps.origin, level.time, NULL, self->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, pos);

	VectorSet(grabMins, -4.0f, -4.0f, -4.0f);
	VectorSet(grabMaxs, 4.0f, 4.0f, 4.0f);

	//trace from my origin to my hand, if we hit anyone then get 'em
	trap->Trace(&trace, self->client->ps.origin, grabMins, grabMaxs, pos, self->s.number, MASK_SHOT, qfalse, G2TRFLAG_DOGHOULTRACE | G2TRFLAG_GETSURFINDEX | G2TRFLAG_THICK | G2TRFLAG_HITCORPSES, g_g2TraceLod.integer);

	if (trace.fraction != 1.0f && trace.entityNum < ENTITYNUM_WORLD)
	{
		gentity_t* grabbed = &g_entities[trace.entityNum];

		if (grabbed->inuse && (grabbed->s.eType == ET_PLAYER || grabbed->s.eType == ET_NPC) &&
			grabbed->client && grabbed->health > 0 &&
			G_CanBeEnemy(self, grabbed) &&
			G_PrettyCloseIGuess(grabbed->client->ps.origin[2], self->client->ps.origin[2], 4.0f) &&
			(!PM_InGrappleMove(grabbed->client->ps.torsoAnim) || grabbed->client->ps.torsoAnim == BOTH_KYLE_GRAB) &&
			(!PM_InGrappleMove(grabbed->client->ps.legsAnim) || grabbed->client->ps.legsAnim == BOTH_KYLE_GRAB))
		{ //grabbed an active player/npc
			int tortureAnim = -1;
			int correspondingAnim = -1;

			if (self->client->pers.cmd.forwardmove > 0)
			{ //punch grab
				tortureAnim = BOTH_KYLE_PA_1;
				correspondingAnim = BOTH_PLAYER_PA_1;
			}
			else if (self->client->pers.cmd.forwardmove < 0)
			{ //knee-throw
				tortureAnim = BOTH_KYLE_PA_2;
				correspondingAnim = BOTH_PLAYER_PA_2;
			}
			else
			{ //head lock
				tortureAnim = BOTH_KYLE_PA_3;
				correspondingAnim = BOTH_PLAYER_PA_3;
			}

			if (tortureAnim == -1 || correspondingAnim == -1)
			{
				if (self->client->ps.torsoTimer < 300 && !self->client->grappleState)
				{ //you failed to grab anyone, play the "failed to grab" anim
					G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
					if (self->client->ps.torsoAnim == BOTH_KYLE_MISS)
					{ //providing the anim set succeeded..
						self->client->ps.weaponTime = self->client->ps.torsoTimer;
					}
				}
				return;
			}

			self->client->grappleIndex = grabbed->s.number;
			self->client->grappleState = 1;

			grabbed->client->grappleIndex = self->s.number;
			grabbed->client->grappleState = 20;

			//time to crack some heads
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, tortureAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
			if (self->client->ps.torsoAnim == tortureAnim)
			{ //providing the anim set succeeded..
				self->client->ps.weaponTime = self->client->ps.torsoTimer;
			}

			G_SetAnim(grabbed, &grabbed->client->pers.cmd, SETANIM_BOTH, correspondingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
			if (grabbed->client->ps.torsoAnim == correspondingAnim)
			{ //providing the anim set succeeded..
				if (grabbed->client->ps.weapon == WP_SABER)
				{ //turn it off
					if (!grabbed->client->ps.saberHolstered)
					{
						grabbed->client->ps.saberHolstered = 2;
						if (grabbed->client->saber[0].soundOff)
						{
							G_Sound(grabbed, CHAN_AUTO, grabbed->client->saber[0].soundOff);
						}
						if (grabbed->client->saber[1].soundOff &&
							grabbed->client->saber[1].model[0])
						{
							G_Sound(grabbed, CHAN_AUTO, grabbed->client->saber[1].soundOff);
						}
					}
				}
				if (grabbed->client->ps.torsoTimer < self->client->ps.torsoTimer)
				{ //make sure they stay in the anim at least as long as the grabber
					grabbed->client->ps.torsoTimer = self->client->ps.torsoTimer;
				}
				grabbed->client->ps.weaponTime = grabbed->client->ps.torsoTimer;
			}
		}
	}

	if (self->client->ps.torsoTimer < 300 && !self->client->grappleState)
	{ //you failed to grab anyone, play the "failed to grab" anim
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		if (self->client->ps.torsoAnim == BOTH_KYLE_MISS)
		{ //providing the anim set succeeded..
			self->client->ps.weaponTime = self->client->ps.torsoTimer;
		}
	}
}

void WP_SaberPositionUpdate(gentity_t* self, usercmd_t* ucmd)
{
	//rww - keep the saber position as updated as possible on the server so that we can try to do realistic-looking contact stuff
	//Note that this function also does the majority of working in maintaining the server g2 client instance (updating angles/anims/etc)
	gentity_t* mySaber = NULL;
	mdxaBone_t boltMatrix;
	vec3_t properAngles, properOrigin;
	vec3_t boltAngles, boltOrigin;
	vec3_t end;
	vec3_t legAxis[3];
	vec3_t rawAngles;
	int returnAfterUpdate = 0;
	float animSpeedScale = 1.0f;
	int saberNum;
	qboolean clientOverride;
	gentity_t* vehEnt = NULL;
	float fVSpeed = 0;
	vec3_t addVel;

	const qboolean self_is_holding_block_button = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;

#ifdef _DEBUG
	if (g_disableServerG2.integer)
	{
		return;
	}
#endif

	if (self && self->inuse && self->client)
	{
		if (self->client->saberCycleQueue)
		{
			self->client->ps.fd.saberDrawAnimLevel = self->client->saberCycleQueue;
		}
		else
		{
			self->client->ps.fd.saberDrawAnimLevel = self->client->ps.fd.saberAnimLevel;
		}
	}

	if (self &&
		self->inuse &&
		self->client &&
		self->client->saberCycleQueue &&
		(self->client->ps.weaponTime <= 0 || self->health < 1))
	{
		//we cycled attack levels while we were busy, so update now that we aren't (even if that means we're dead)
		self->client->ps.fd.saberAnimLevel = self->client->saberCycleQueue;
		self->client->saberCycleQueue = 0;
	}

	if (!self ||
		!self->inuse ||
		!self->client ||
		!self->ghoul2 ||
		!g2SaberInstance)
	{
		return;
	}

	if (ucmd->buttons & BUTTON_THERMALTHROW)
	{
		//player wants to snap throw a grenade
		if (self->client->ps.weaponTime <= 0 //not currently using a weapon
			&& self->client->ps.stats[STAT_WEAPONS] & 1 << WP_THERMAL && self->client->ps.ammo[AMMO_THERMAL] > 0)
			//have a thermal
		{
			//throw!
			self->s.weapon = WP_THERMAL; //temp switch weapons so we can toss it.
			self->client->ps.weaponChargeTime = level.time - 450; //throw at medium power
			FireWeapon(self, qfalse);
			self->s.weapon = self->client->ps.weapon; //restore weapon
			self->client->ps.weaponTime = weaponData[WP_THERMAL].fireTime;
			self->client->ps.ammo[AMMO_THERMAL]--;
			G_SetAnim(self, NULL, SETANIM_TORSO, BOTH_MELEE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		}
	}

	if (PM_PunchAnim(self->client->ps.torsoAnim))
	{
		G_PunchSomeMofos(self);
	}
	else if (PM_KickingAnim(self->client->ps.legsAnim))
	{
		//do some kick traces and stuff if we're in the appropriate anim
		G_KickSomeMofos(self);
	}
	else if (self->client->ps.torsoAnim == BOTH_KYLE_GRAB)
	{//try to grab someone
		G_GrabSomeMofos(self);
	}
	else if (self->client->grappleState)
	{
		gentity_t* grappler = &g_entities[self->client->grappleIndex];

		if (!grappler->inuse || !grappler->client || grappler->client->grappleIndex != self->s.number ||
			!PM_InGrappleMove(grappler->client->ps.torsoAnim) || !PM_InGrappleMove(grappler->client->ps.legsAnim) ||
			!PM_InGrappleMove(self->client->ps.torsoAnim) || !PM_InGrappleMove(self->client->ps.legsAnim) ||
			!self->client->grappleState || !grappler->client->grappleState ||
			grappler->health < 1 || self->health < 1 ||
			!G_PrettyCloseIGuess(self->client->ps.origin[2], grappler->client->ps.origin[2], 4.0f))
		{
			self->client->grappleState = 0;

			//Special case for BOTH_KYLE_PA_3 This don't look correct otherwise.
			if (self->client->ps.torsoAnim == BOTH_KYLE_PA_3 && (self->client->ps.torsoTimer > 100 ||
				self->client->ps.legsTimer > 100))
			{
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_OVERRIDE, 0);
				self->client->ps.weaponTime = self->client->ps.torsoTimer = 0;
			}
			else if ((PM_InGrappleMove(self->client->ps.torsoAnim) && self->client->ps.torsoTimer > 100) ||
				(PM_InGrappleMove(self->client->ps.legsAnim) && self->client->ps.legsTimer > 100))
			{ //if they're pretty far from finishing the anim then shove them into another anim
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
				if (self->client->ps.torsoAnim == BOTH_KYLE_MISS)
				{ //providing the anim set succeeded..
					self->client->ps.weaponTime = self->client->ps.torsoTimer;
				}
			}
		}
		else
		{
			vec3_t grapAng;

			VectorSubtract(grappler->client->ps.origin, self->client->ps.origin, grapAng);

			if (VectorLength(grapAng) > 64.0f)
			{ //too far away, break it off
				if ((PM_InGrappleMove(self->client->ps.torsoAnim) && self->client->ps.torsoTimer > 100) ||
					(PM_InGrappleMove(self->client->ps.legsAnim) && self->client->ps.legsTimer > 100))
				{
					self->client->grappleState = 0;

					G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
					if (self->client->ps.torsoAnim == BOTH_KYLE_MISS)
					{ //providing the anim set succeeded..
						self->client->ps.weaponTime = self->client->ps.torsoTimer;
					}
				}
			}
			else
			{
				vectoangles(grapAng, grapAng);
				SetClientViewAngle(self, grapAng);

				if (self->client->grappleState >= 20)
				{ //grapplee
					//try to position myself at the correct distance from my grappler
					float idealDist;
					vec3_t gFwd, idealSpot;
					trace_t trace;

					if (grappler->client->ps.torsoAnim == BOTH_KYLE_PA_1)
					{ //grab punch
						idealDist = 46.0f;
					}
					else if (grappler->client->ps.torsoAnim == BOTH_KYLE_PA_2)
					{ //knee-throw
						idealDist = 46.0f;
					}
					else
					{ //head lock
						idealDist = 46.0f;
					}

					AngleVectors(grappler->client->ps.viewangles, gFwd, 0, 0);
					VectorMA(grappler->client->ps.origin, idealDist, gFwd, idealSpot);

					trap->Trace(&trace, self->client->ps.origin, self->r.mins, self->r.maxs, idealSpot, self->s.number, self->clipmask, qfalse, 0, 0);
					if (!trace.startsolid && !trace.allsolid && trace.fraction == 1.0f)
					{ //go there
						G_SetOrigin(self, idealSpot);
						VectorCopy(idealSpot, self->client->ps.origin);
					}
				}
				else if (self->client->grappleState >= 1)
				{ //grappler
					if (grappler->client->ps.weapon == WP_SABER)
					{ //make sure their saber is shut off
						if (!grappler->client->ps.saberHolstered)
						{
							grappler->client->ps.saberHolstered = 2;
							if (grappler->client->saber[0].soundOff)
							{
								G_Sound(grappler, CHAN_AUTO, grappler->client->saber[0].soundOff);
							}
							if (grappler->client->saber[1].soundOff &&
								grappler->client->saber[1].model[0])
							{
								G_Sound(grappler, CHAN_AUTO, grappler->client->saber[1].soundOff);
							}
						}
					}

					//check for smashy events
					if (self->client->ps.torsoAnim == BOTH_KYLE_PA_1)
					{ //grab punch
						if (self->client->grappleState == 1)
						{ //smack
							if (self->client->ps.torsoTimer < 3400)
							{
								int grapplerAnim = grappler->client->ps.torsoAnim;
								int grapplerTime = grappler->client->ps.torsoTimer;

								G_Damage(grappler, self, self, NULL, self->client->ps.origin, 10, 0, MOD_MELEE);
								//G_Sound( grappler, CHAN_AUTO, G_SoundIndex( va( "sound/weapons/melee/punch%d", Q_irand( 1, 4 ) ) ) );

								//it might try to put them into a pain anim or something, so override it back again
								if (grappler->health > 0)
								{
									grappler->client->ps.torsoAnim = grapplerAnim;
									grappler->client->ps.torsoTimer = grapplerTime;
									grappler->client->ps.legsAnim = grapplerAnim;
									grappler->client->ps.legsTimer = grapplerTime;
									grappler->client->ps.weaponTime = grapplerTime;
								}
								self->client->grappleState++;
							}
						}
						else if (self->client->grappleState == 2)
						{ //smack!
							if (self->client->ps.torsoTimer < 2550)
							{
								int grapplerAnim = grappler->client->ps.torsoAnim;
								int grapplerTime = grappler->client->ps.torsoTimer;

								G_Damage(grappler, self, self, NULL, self->client->ps.origin, 10, 0, MOD_MELEE);
								//G_Sound( grappler, CHAN_AUTO, G_SoundIndex( va( "sound/weapons/melee/punch%d", Q_irand( 1, 4 ) ) ) );

								//it might try to put them into a pain anim or something, so override it back again
								if (grappler->health > 0)
								{
									grappler->client->ps.torsoAnim = grapplerAnim;
									grappler->client->ps.torsoTimer = grapplerTime;
									grappler->client->ps.legsAnim = grapplerAnim;
									grappler->client->ps.legsTimer = grapplerTime;
									grappler->client->ps.weaponTime = grapplerTime;
								}
								self->client->grappleState++;
							}
						}
						else
						{ //SMACK!
							if (self->client->ps.torsoTimer < 1300)
							{
								vec3_t tossDir;

								G_Damage(grappler, self, self, NULL, self->client->ps.origin, 30, 0, MOD_MELEE);

								self->client->grappleState = 0;

								VectorSubtract(grappler->client->ps.origin, self->client->ps.origin, tossDir);
								VectorNormalize(tossDir);
								VectorScale(tossDir, 250.0f, tossDir);
								tossDir[2] = 200.0f;

								VectorAdd(grappler->client->ps.velocity, tossDir, grappler->client->ps.velocity);

								if (grappler->health > 0)
								{ //if still alive knock them down

									//grappler->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
									//No more Yeeting them across the map
									G_Knockdown(grappler, self, tossDir, 100, qtrue);
									G_SetAnim(grappler, &grappler->client->pers.cmd, SETANIM_BOTH, BOTH_KNOCKDOWN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);

									grappler->client->ps.forceHandExtendTime = level.time + 500;

									//Count as kill for attacker if the other player falls to his death.
									grappler->client->ps.otherKiller = self->s.number;
									grappler->client->ps.otherKillerTime = level.time + 8000;
									grappler->client->ps.otherKillerDebounceTime = level.time + 100;
								}
							}
						}
					}
					else if (self->client->ps.torsoAnim == BOTH_KYLE_PA_2)
					{ //knee throw
						if (self->client->grappleState == 1)
						{ //knee to the face
							if (self->client->ps.torsoTimer < 3200)
							{
								int grapplerAnim = grappler->client->ps.torsoAnim;
								int grapplerTime = grappler->client->ps.torsoTimer;

								G_Damage(grappler, self, self, NULL, self->client->ps.origin, 20, 0, MOD_MELEE);

								//it might try to put them into a pain anim or something, so override it back again
								if (grappler->health > 0)
								{
									grappler->client->ps.torsoAnim = grapplerAnim;
									grappler->client->ps.torsoTimer = grapplerTime;
									grappler->client->ps.legsAnim = grapplerAnim;
									grappler->client->ps.legsTimer = grapplerTime;
									grappler->client->ps.weaponTime = grapplerTime;
								}
								self->client->grappleState++;
							}
						}
						else if (self->client->grappleState == 2)
						{ //smashed on the ground
							if (self->client->ps.torsoTimer < 2000)
							{
								//don't do damage on this one, it would look very freaky if they died
								G_EntitySound(grappler, CHAN_VOICE, G_SoundIndex("*pain100.wav"));
								self->client->grappleState++;
							}
						}
						else
						{ //and another smash
							if (self->client->ps.torsoTimer < 1000)
							{
								G_Damage(grappler, self, self, NULL, self->client->ps.origin, 30, 0, MOD_MELEE);

								//it might try to put them into a pain anim or something, so override it back again
								if (grappler->health > 0)
								{
									grappler->client->ps.torsoTimer = 500;
									SetClientViewAngle(grappler, grapAng);
									G_SetAnim(grappler, &grappler->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
									grappler->client->grappleState = 0;
								}
								else
								{ //override death anim
									SetClientViewAngle(grappler, grapAng);
									grappler->client->ps.torsoAnim = BOTH_DEADFLOP1;
									grappler->client->ps.legsAnim = BOTH_DEADFLOP1;
								}

								self->client->grappleState = 0;
							}
						}
					}
					else if (self->client->ps.torsoAnim == BOTH_KYLE_PA_3)
					{//Head Lock
						if ((self->client->grappleState == 1 && self->client->ps.torsoTimer < 5034) ||
							(self->client->grappleState == 2 && self->client->ps.torsoTimer < 3965))
						{//choke noises
							self->client->grappleState++;
						}
						else if (self->client->grappleState == 3)
						{ //throw to ground
							if (self->client->ps.torsoTimer < 2379)
							{
								int grapplerAnim = grappler->client->ps.torsoAnim;
								int grapplerTime = grappler->client->ps.torsoTimer;

								G_Damage(grappler, self, self, NULL, self->client->ps.origin, 50, 0, MOD_MELEE);

								//it might try to put them into a pain anim or something, so override it back again
								if (grappler->health > 0)
								{
									grappler->client->ps.torsoAnim = grapplerAnim;
									grappler->client->ps.torsoTimer = grapplerTime;
									grappler->client->ps.legsAnim = grapplerAnim;
									grappler->client->ps.legsTimer = grapplerTime;
									grappler->client->ps.weaponTime = grapplerTime;
								}
								else
								{//he bought it.  Make sure it looks right.
									//set the correct exit player angle
									SetClientViewAngle(grappler, grapAng);

									grappler->client->ps.torsoAnim = BOTH_DEADFLOP1;
									grappler->client->ps.legsAnim = BOTH_DEADFLOP1;
								}
								self->client->grappleState++;
							}
						}
						else if (self->client->grappleState == 4)
						{
							if (self->client->ps.torsoTimer < 758)
							{
								vec3_t tossDir;

								VectorSubtract(grappler->client->ps.origin, self->client->ps.origin, tossDir);
								VectorNormalize(tossDir);
								VectorScale(tossDir, 250.0f, tossDir);
								tossDir[2] = 200.0f;

								VectorAdd(grappler->client->ps.velocity, tossDir, grappler->client->ps.velocity);

								if (grappler->health > 0)
								{//racc - knock this mofo down
									grappler->client->ps.torsoTimer = 500;
									//grappler->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
									//No more Yeeting them across the map
									G_Knockdown(grappler, self, tossDir, 200, qtrue);
									G_SetAnim(grappler, &grappler->client->pers.cmd, SETANIM_BOTH, BOTH_KNOCKDOWN3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);

									grappler->client->ps.forceHandExtendTime = level.time + 500;
									grappler->client->grappleState = 0;

									//Count as kill for attacker if the other player falls to his death.
									grappler->client->ps.otherKiller = self->s.number;
									grappler->client->ps.otherKillerTime = level.time + 8000;
									grappler->client->ps.otherKillerDebounceTime = level.time + 100;
								}
							}
						}
					}
					else
					{ //?
					}
				}
			}
		}
	}

	//If this is a listen server (client+server running on same machine),
	//then lets try to steal the skeleton/etc data off the client instance
	//for this entity to save us processing time.
	clientOverride = trap->G2API_OverrideServer(self->ghoul2);

	saberNum = self->client->ps.saberEntityNum;

	if (!saberNum)
	{
		saberNum = self->client->saberStoredIndex;
	}

	if (!saberNum)
	{
		returnAfterUpdate = 1;
		goto nextStep;
	}

	mySaber = &g_entities[saberNum];

	if (self->health < 1)
	{
		//we don't want to waste precious CPU time calculating saber positions for corpses. But we want to avoid the saber ent position lagging on spawn, so..
		//I guess it's good to keep the position updated even when contents are 0
		if (mySaber && (mySaber->r.contents & CONTENTS_LIGHTSABER || mySaber->r.contents == 0) && !self->client->ps.saberInFlight)
		{
			//Since we haven't got a bolt position, place it on top of the player origin.
			VectorCopy(self->client->ps.origin, mySaber->r.currentOrigin);
		}
	}

	if (PM_SuperBreakWinAnim(self->client->ps.torsoAnim))
	{
		self->client->ps.weaponstate = WEAPON_FIRING;
	}
	if (self->client->ps.weapon != WP_SABER ||
		self->client->ps.weaponstate == WEAPON_RAISING ||
		self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->health < 1)
	{
		if (!self->client->ps.saberInFlight)
		{
			returnAfterUpdate = 1;
		}
	}

	if (self->client->ps.saberThrowDelay < level.time)
	{
		if (self->client->saber[0].saberFlags & SFL_NOT_THROWABLE)
		{
			//cant throw it normally!
			if (self->client->saber[0].saberFlags & SFL_SINGLE_BLADE_THROWABLE)
			{
				//but can throw it if only have 1 blade on
				if (self->client->saber[0].numBlades > 1
					&& self->client->ps.saberHolstered == 1)
				{
					//have multiple blades and only one blade on
					self->client->ps.saberCanThrow = qtrue; //qfalse;
					//huh? want to be able to throw then right?
				}
				else
				{
					//multiple blades on, can't throw
					self->client->ps.saberCanThrow = qfalse;
				}
			}
			else
			{
				//never can throw it
				self->client->ps.saberCanThrow = qfalse;
			}
		}
		else
		{
			//can throw it!
			self->client->ps.saberCanThrow = qtrue;
		}
	}
nextStep:
	if (self->client->ps.fd.forcePowersActive & 1 << FP_RAGE)
	{
		//animSpeedScale = 2;
	}

	// factor in different lightsaber moves here
	if (self->client->ps.saber_move >= LS_A_TL2BR &&
		self->client->ps.saber_move <= LS_A_BACKSTAB)
	{
		animSpeedScale = 0.8;
	}

	VectorCopy(self->client->ps.origin, properOrigin);

	//try to predict the origin based on velocity so it's more like what the client is seeing
	VectorCopy(self->client->ps.velocity, addVel);
	VectorNormalize(addVel);

	if (self->client->ps.velocity[0] < 0)
	{
		fVSpeed += (-self->client->ps.velocity[0]);
	}
	else
	{
		fVSpeed += self->client->ps.velocity[0];
	}
	if (self->client->ps.velocity[1] < 0)
	{
		fVSpeed += (-self->client->ps.velocity[1]);
	}
	else
	{
		fVSpeed += self->client->ps.velocity[1];
	}
	if (self->client->ps.velocity[2] < 0)
	{
		fVSpeed += (-self->client->ps.velocity[2]);
	}
	else
	{
		fVSpeed += self->client->ps.velocity[2];
	}

	fVSpeed *= 1.6f / sv_fps.value;

	//Cap it off at reasonable values so the saber box doesn't go flying ahead of us or
	//something if we get a big speed boost from something.
	if (fVSpeed > 70)
	{
		fVSpeed = 70;
	}
	if (fVSpeed < -70)
	{
		fVSpeed = -70;
	}

	properOrigin[0] += addVel[0] * fVSpeed;
	properOrigin[1] += addVel[1] * fVSpeed;
	properOrigin[2] += addVel[2] * fVSpeed;

	properAngles[0] = 0;
	if (self->s.number < MAX_CLIENTS && self->client->ps.m_iVehicleNum)
	{
		vehEnt = &g_entities[self->client->ps.m_iVehicleNum];
		if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
		{
			properAngles[1] = vehEnt->m_pVehicle->m_vOrientation[YAW];
		}
		else
		{
			properAngles[1] = self->client->ps.viewangles[YAW];
			vehEnt = NULL;
		}
	}
	else
	{
		properAngles[1] = self->client->ps.viewangles[YAW];
	}
	properAngles[2] = 0;

	AnglesToAxis(properAngles, legAxis);

	UpdateClientRenderinfo(self, properOrigin, properAngles);

	if (!clientOverride)
	{ //if we get the client instance we don't need to do this
		G_G2PlayerAngles(self, legAxis, properAngles);
	}

	if (vehEnt)
	{
		properAngles[1] = vehEnt->m_pVehicle->m_vOrientation[YAW];
	}

	if (returnAfterUpdate && saberNum)
	{ //We don't even need to do GetBoltMatrix if we're only in here to keep the g2 server instance in sync
		//but keep our saber entity in sync too, just copy it over our origin.

		//I guess it's good to keep the position updated even when contents are 0
		if (mySaber && ((mySaber->r.contents & CONTENTS_LIGHTSABER) || mySaber->r.contents == 0) && !self->client->ps.saberInFlight)
		{ //Since we haven't got a bolt position, place it on top of the player origin.
			VectorCopy(self->client->ps.origin, mySaber->r.currentOrigin);
		}

		goto finalUpdate;
	}

	if (returnAfterUpdate)
	{
		goto finalUpdate;
	}

	//We'll get data for blade 0 first no matter what it is and stick them into
	//the constant ("_Always") values. Later we will handle going through each blade.
	trap->G2API_GetBoltMatrix(self->ghoul2, 1, 0, &boltMatrix, properAngles, properOrigin, level.time, NULL, self->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrigin);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, boltAngles);

	//immediately store these values so we don't have to recalculate this again
	if (self->client->lastSaberStorageTime && level.time - self->client->lastSaberStorageTime < 200)
	{
		//alright
		VectorCopy(self->client->lastSaberBase_Always, self->client->olderSaberBase);
		self->client->olderIsValid = qtrue;
	}
	else
	{
		self->client->olderIsValid = qfalse;
	}

	VectorCopy(boltOrigin, self->client->lastSaberBase_Always);
	VectorCopy(boltAngles, self->client->lastSaberDir_Always);
	self->client->lastSaberStorageTime = level.time;

	VectorCopy(boltAngles, rawAngles);

	VectorMA(boltOrigin, self->client->saber[0].blade[0].lengthMax, boltAngles, end);

	if (self->client->ps.saberEntityNum)
	{
		//I guess it's good to keep the position updated even when contents are 0
		if (mySaber && (mySaber->r.contents & CONTENTS_LIGHTSABER || mySaber->r.contents == 0) && !self->client->ps.saberInFlight)
		{
			//place it roughly in the middle of the saber..
			VectorMA(boltOrigin, self->client->saber[0].blade[0].lengthMax, boltAngles, mySaber->r.currentOrigin);
		}
	}

	boltAngles[YAW] = self->client->ps.viewangles[YAW];

	if (self->client->ps.saberInFlight)
	{
		//do the thrown-saber stuff
		gentity_t* saberent = &g_entities[saberNum];

		if (saberent)
		{
			if (!self->client->ps.saberEntityState && self->client->ps.saberEntityNum)
			{
				vec3_t startorg, startang, dir;

				VectorCopy(boltOrigin, saberent->r.currentOrigin);

				VectorCopy(boltOrigin, startorg);
				VectorCopy(boltAngles, startang);

				startang[0] = 90;
				//Instead of this we'll sort of fake it and slowly tilt it down on the client via
				//a per frame method (which doesn't actually affect where or how the saber hits)

				saberent->r.svFlags &= ~SVF_NOCLIENT;
				VectorCopy(startorg, saberent->s.pos.trBase);
				VectorCopy(startang, saberent->s.apos.trBase);

				VectorCopy(startorg, saberent->s.origin);
				VectorCopy(startang, saberent->s.angles);

				saberent->s.saberInFlight = qtrue;

				if (self->client->ps.fd.saberAnimLevel == SS_STAFF)
				{
					saberent->s.apos.trType = TR_LINEAR;
					saberent->s.apos.trDelta[0] = 0;
					saberent->s.apos.trDelta[1] = 800;
					saberent->s.apos.trDelta[2] = 0;
				}
				else
				{
					saberent->s.apos.trType = TR_LINEAR;;
					saberent->s.apos.trDelta[0] = 600;
					saberent->s.apos.trDelta[1] = 0;
					saberent->s.apos.trDelta[2] = 0;
				}

				saberent->s.pos.trType = TR_LINEAR;
				saberent->s.eType = ET_GENERAL;
				saberent->s.eFlags = 0;

				WP_SaberAddG2Model(saberent, self->client->saber[0].model, self->client->saber[0].skin);

				saberent->s.modelGhoul2 = 127;

				saberent->parent = self;

				self->client->ps.saberEntityState = 1;

				//Projectile stuff:
				AngleVectors(self->client->ps.viewangles, dir, NULL, NULL);

				saberent->nextthink = level.time + FRAMETIME;
				saberent->think = saberFirstThrown;

				saberent->damage = SABER_THROWN_HIT_DAMAGE;
				saberent->methodOfDeath = MOD_SABER;
				saberent->splashMethodOfDeath = MOD_SABER;
				saberent->s.solid = 2;
				saberent->r.contents = CONTENTS_LIGHTSABER;

				saberent->genericValue5 = 0;

				VectorSet(saberent->r.mins, SABERMINS_X, SABERMINS_Y, SABERMINS_Z);
				VectorSet(saberent->r.maxs, SABERMAXS_X, SABERMAXS_Y, SABERMAXS_Z);

				saberent->s.genericenemyindex = self->s.number + 1024;

				saberent->touch = thrownSaberTouch;

				saberent->s.weapon = WP_SABER;

				VectorScale(dir, 400, saberent->s.pos.trDelta);
				saberent->s.pos.trTime = level.time;

				if (self->client->saber[0].spinSound)
				{
					saberent->s.loopSound = self->client->saber[0].spinSound;
				}
				else
				{
					saberent->s.loopSound = saberSpinSound;
				}
				saberent->s.loopIsSoundset = qfalse;

				self->client->dangerTime = level.time;
				self->client->ps.eFlags &= ~EF_INVULNERABLE;
				self->client->invulnerableTimer = 0;

				trap->LinkEntity((sharedEntity_t*)saberent);
			}
			else if (self->client->ps.saberEntityNum) //only do this stuff if your saber is active and has not been knocked out of the air.
			{
				VectorCopy(boltOrigin, saberent->pos1);
				trap->LinkEntity((sharedEntity_t*)saberent);

				if (saberent->genericValue5 == PROPER_THROWN_VALUE)
				{
					//return to the owner now, this is a bad state to be in for here..
					saberent->genericValue5 = 0;
					saberent->think = SaberUpdateSelf;
					saberent->nextthink = level.time;
					WP_SaberRemoveG2Model(saberent);

					self->client->ps.saberInFlight = qfalse;
					self->client->ps.saberEntityState = 0;
					self->client->ps.saberThrowDelay = level.time + 500;
					self->client->ps.saberCanThrow = qfalse;
				}
			}
		}
	}

	if (!BG_SabersOff(&self->client->ps))
	{
		int rSaberNum = 0;
		int rBladeNum = 0;
		gentity_t* saberent = &g_entities[saberNum];

		if (!self->client->ps.saberInFlight && saberent)
		{
			saberent->r.svFlags |= SVF_NOCLIENT;
			saberent->r.contents = CONTENTS_LIGHTSABER;
			SetSaberBoxSize(saberent);
			saberent->s.loopSound = 0;
			saberent->s.loopIsSoundset = qfalse;
		}

		if (self->client->ps.saberLockTime > level.time && self->client->ps.saberEntityNum)
		{
			while (rSaberNum < MAX_SABERS)
			{
				rBladeNum = 0;
				while (rBladeNum < self->client->saber[rSaberNum].numBlades)
				{
					//Don't bother updating the bolt for each blade for this, it's just a very rough fallback method for during saberlocks
					VectorCopy(boltOrigin, self->client->saber[rSaberNum].blade[rBladeNum].trail.base);
					VectorCopy(end, self->client->saber[rSaberNum].blade[rBladeNum].trail.tip);
					self->client->saber[rSaberNum].blade[rBladeNum].trail.lastTime = level.time;

					rBladeNum++;
				}

				rSaberNum++;
			}
			self->client->hasCurrentPosition = qtrue;

			self->client->ps.saberBlocked = BLOCKED_NONE;

			goto finalUpdate;
		}

		//reset it in case we used it for cycling before
		rSaberNum = rBladeNum = 0;

		if (self->client->ps.saberInFlight)
		{
			//if saber is thrown then only do the standard stuff for the left hand saber
			if (!self->client->ps.saberEntityNum)
			{
				//however, if saber is not in flight but rather knocked away, our left saber is off, and thus we may do nothing.
				rSaberNum = 1; //was 2?
			}
			else
			{
				//thrown saber still in flight, so do damage
				rSaberNum = 0; //was 1?
			}
		}
		saberDoClashEffect = qfalse;

		//Now cycle through each saber and each blade on the saber and do damage traces.
		while (rSaberNum < MAX_SABERS)
		{
			if (!self->client->saber[rSaberNum].model[0])
			{
				rSaberNum++;
				continue;
			}

			//for now I'm keeping a broken right arm swingable, it will just look and act damaged
			//but still be useable

			if (rSaberNum == 1 && self->client->ps.brokenLimbs & 1 << BROKENLIMB_LARM)
			{
				//don't to saber 1 if the left arm is broken
				break;
			}
			if (rSaberNum > 0
				&& self->client->saber[1].model
				&& self->client->saber[1].model[0]
				&& self->client->ps.saberHolstered == 1
				&& (!self->client->ps.saberInFlight || self->client->ps.saberEntityNum))
			{
				//don't to saber 2 if it's off
				break;
			}
			rBladeNum = 0;
			while (rBladeNum < self->client->saber[rSaberNum].numBlades)
			{
				//update muzzle data for the blade
				VectorCopy(self->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, self->client->saber[rSaberNum].blade[rBladeNum].muzzlePointOld);
				VectorCopy(self->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, self->client->saber[rSaberNum].blade[rBladeNum].muzzleDirOld);

				if (!(g_Enhanced_saberTweaks.integer & SABERTWEAK_TWOBLADEDEFLECTFIX)
					&& rBladeNum > 0 //more than one blade
					&& !self->client->saber[1].model[0] //not using dual blades
					&& self->client->saber[rSaberNum].numBlades > 1 //using a multi-bladed saber
					&& self->client->ps.saberHolstered == 1) //
				{
					//don't to extra blades if they're off
					break;
				}
				//get the new data
				//then update the bolt pos/dir. rblade_num corresponds to the bolt index because blade bolts are added in order.
				if (rSaberNum == 0 && self->client->ps.saberInFlight)
				{
					if (!self->client->ps.saberEntityNum)
					{
						//dropped it... shouldn't get here, but...
						rSaberNum++;
						rBladeNum = 0;
						continue;
					}
					gentity_t* saberEnt = &g_entities[self->client->ps.saberEntityNum];
					vec3_t saber_org, saber_angles;
					if (!saberEnt
						|| !saberEnt->inuse
						|| !saberEnt->ghoul2)
					{
						//wtf?
						rSaberNum++;
						rBladeNum = 0;
						continue;
					}
					if (saberent->s.saberInFlight)
					{
						//spinning
						BG_EvaluateTrajectory(&saberEnt->s.pos, level.time + 50, saber_org);
						BG_EvaluateTrajectory(&saberEnt->s.apos, level.time + 50, saber_angles);
					}
					else
					{
						//coming right back
						vec3_t saber_dir;
						BG_EvaluateTrajectory(&saberEnt->s.pos, level.time, saber_org);
						VectorSubtract(self->r.currentOrigin, saber_org, saber_dir);
						vectoangles(saber_dir, saber_angles);
					}
					trap->G2API_GetBoltMatrix(saberEnt->ghoul2, 0, rBladeNum, &boltMatrix, saber_angles, saber_org, level.time, NULL, self->modelScale);
					BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, self->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint);
					BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, self->client->saber[rSaberNum].blade[rBladeNum].muzzleDir);
					VectorCopy(self->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, boltOrigin);
					VectorMA(boltOrigin, self->client->saber[rSaberNum].blade[rBladeNum].lengthMax, self->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end);
				}
				else
				{
					trap->G2API_GetBoltMatrix(self->ghoul2, rSaberNum + 1, rBladeNum, &boltMatrix, properAngles, properOrigin, level.time, NULL, self->modelScale);
					BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, self->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint);
					BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, self->client->saber[rSaberNum].blade[rBladeNum].muzzleDir);
					VectorCopy(self->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, boltOrigin);
					VectorMA(boltOrigin, self->client->saber[rSaberNum].blade[rBladeNum].lengthMax, self->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end);
				}

				self->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;

				if (self->client->hasCurrentPosition && d_saberInterpolate.integer == 1)
				{
					if (self->client->ps.weaponTime <= 0)
					{
						//don't bother doing the extra stuff unless actually attacking. This is in attempt to save CPU.
						CheckSaberDamage(self, rSaberNum, rBladeNum, boltOrigin, end, MASK_PLAYERSOLID | CONTENTS_LIGHTSABER | MASK_SHOT);
					}
					else if (d_saberInterpolate.integer == 1)
					{
						int trMask = CONTENTS_LIGHTSABER | CONTENTS_BODY;
						int s_n = 0;
						qboolean gotHit = qfalse;
						qboolean clientUnlinked[MAX_CLIENTS];
						qboolean skipSaberTrace = qfalse;

						if (!g_saberTraceSaberFirst.integer)
						{
							skipSaberTrace = qtrue;
						}
						else if (g_saberTraceSaberFirst.integer >= 2 &&
							level.gametype != GT_DUEL &&
							level.gametype != GT_POWERDUEL &&
							!self->client->ps.duelInProgress)
						{
							//if value is >= 2, and not in a duel, skip
							skipSaberTrace = qtrue;
						}

						if (skipSaberTrace)
						{
							//skip the saber-contents-only trace and get right to the full trace
							trMask = MASK_PLAYERSOLID | CONTENTS_LIGHTSABER | MASK_SHOT;
						}
						else
						{
							while (s_n < MAX_CLIENTS)
							{
								if (g_entities[s_n].inuse && g_entities[s_n].client && g_entities[s_n].r.linked &&
									g_entities[s_n].health > 0 && g_entities[s_n].r.contents & CONTENTS_BODY)
								{
									//Take this mask off before the saber trace, because we want to hit the saber first
									g_entities[s_n].r.contents &= ~CONTENTS_BODY;
									clientUnlinked[s_n] = qtrue;
								}
								else
								{
									clientUnlinked[s_n] = qfalse;
								}
								s_n++;
							}
						}

						while (!gotHit)
						{
							if (!CheckSaberDamage(self, rSaberNum, rBladeNum, boltOrigin, end, trMask))
							{
								vec3_t oldSaberStart;
								vec3_t oldSaberEnd;
								vec3_t saberAngleNow;
								vec3_t saberAngleBefore;
								float delta_x;
								float delta_y;
								float delta_z;

								if (level.time - self->client->saber[rSaberNum].blade[rBladeNum].trail.lastTime > 100)
								{
									//no valid last pos, use current
									VectorCopy(boltOrigin, oldSaberStart);
									VectorCopy(end, oldSaberEnd);
								}
								else
								{
									//trace from last pos
									VectorCopy(self->client->saber[rSaberNum].blade[rBladeNum].trail.base, oldSaberStart);
									VectorCopy(self->client->saber[rSaberNum].blade[rBladeNum].trail.tip, oldSaberEnd);
								}

								VectorSubtract(oldSaberEnd, oldSaberStart, saberAngleBefore);
								vectoangles(saberAngleBefore, saberAngleBefore);

								VectorSubtract(end, boltOrigin, saberAngleNow);
								vectoangles(saberAngleNow, saberAngleNow);

								delta_x = AngleDelta(saberAngleBefore[0], saberAngleNow[0]);
								delta_y = AngleDelta(saberAngleBefore[1], saberAngleNow[1]);
								delta_z = AngleDelta(saberAngleBefore[2], saberAngleNow[2]);

								if ((delta_x != 0 || delta_y != 0 || delta_z != 0) && delta_x < 180 && delta_y < 180 &&
									delta_z < 180 && (PM_SaberInAttack(self->client->ps.saber_move) ||
										PM_SaberInTransition(self->client->ps.saber_move)))
								{
									vec3_t saber_sub_base;
									vec3_t saber_mid_end;
									vec3_t saber_mid_point;
									vec3_t saber_mid_angle;
									vec3_t saber_mid_dir;
									//don't go beyond here if we aren't attacking/transitioning or the angle is too large.
									//and don't bother if the angle is the same
									saber_mid_angle[0] = saberAngleBefore[0] + delta_x / 2;
									saber_mid_angle[1] = saberAngleBefore[1] + delta_y / 2;
									saber_mid_angle[2] = saberAngleBefore[2] + delta_z / 2;

									//Now that I have the angle, I'll just say the base for it is the difference between the two start
									//points (even though that's quite possibly completely false)
									VectorSubtract(boltOrigin, oldSaberStart, saber_sub_base);
									saber_mid_point[0] = boltOrigin[0] + saber_sub_base[0] * 0.5;
									saber_mid_point[1] = boltOrigin[1] + saber_sub_base[1] * 0.5;
									saber_mid_point[2] = boltOrigin[2] + saber_sub_base[2] * 0.5;

									AngleVectors(saber_mid_angle, saber_mid_dir, 0, 0);
									saber_mid_end[0] = saber_mid_point[0] + saber_mid_dir[0] * self->client->saber[rSaberNum].blade[rBladeNum].lengthMax;
									saber_mid_end[1] = saber_mid_point[1] + saber_mid_dir[1] * self->client->saber[rSaberNum].blade[rBladeNum].lengthMax;
									saber_mid_end[2] = saber_mid_point[2] + saber_mid_dir[2] * self->client->saber[rSaberNum].blade[rBladeNum].lengthMax;

									//I'll just trace straight out and not even trace between positions to save speed.
									if (CheckSaberDamage(self, rSaberNum, rBladeNum, saber_mid_point, saber_mid_end, trMask))
									{
										gotHit = qtrue;
									}
								}
							}
							else
							{
								gotHit = qtrue;
							}

							if (g_saberTraceSaberFirst.integer)
							{
								s_n = 0;
								while (s_n < MAX_CLIENTS)
								{
									if (clientUnlinked[s_n])
									{
										//Make clients clip properly again.
										if (g_entities[s_n].inuse && g_entities[s_n].health > 0)
										{
											g_entities[s_n].r.contents |= CONTENTS_BODY;
										}
									}
									s_n++;
								}
							}

							if (!gotHit)
							{
								if (trMask != (MASK_PLAYERSOLID | CONTENTS_LIGHTSABER | MASK_SHOT))
								{
									trMask = MASK_PLAYERSOLID | CONTENTS_LIGHTSABER | MASK_SHOT;
								}
								else
								{
									gotHit = qtrue; //break out of the loop
								}
							}
						}
					}
					else if (d_saberInterpolate.integer) //anything but 0 or 1, use the old plain method.
					{
						if (!CheckSaberDamage(self, rSaberNum, rBladeNum, boltOrigin, end, MASK_PLAYERSOLID | CONTENTS_LIGHTSABER | MASK_SHOT))
						{
							CheckSaberDamage(self, rSaberNum, rBladeNum, boltOrigin, end, MASK_PLAYERSOLID | CONTENTS_LIGHTSABER | MASK_SHOT);
						}
					}
				}
				else if (self->client->hasCurrentPosition && d_saberInterpolate.integer == 2)
				{
					//Super duper interplotation system
					if (level.time - self->client->saber[rSaberNum].blade[rBladeNum].trail.lastTime < 100 &&
						(PM_SaberInFullDamageMove(&self->client->ps, self->localAnimIndex) || self_is_holding_block_button))
					{
						vec3_t olddir;
						float dist = (d_saberBoxTraceSize.value + self->client->saber[rSaberNum].blade[rBladeNum].radius) * 0.5f;
						VectorSubtract(self->client->saber[rSaberNum].blade[rBladeNum].trail.tip, self->client->saber[rSaberNum].blade[rBladeNum].trail.base, olddir);
						VectorNormalize(olddir);

						//start off by firing a trace down the old saber position to see if it's still inside something.
						if (CheckSaberDamage(self, rSaberNum, rBladeNum, self->client->saber[rSaberNum].blade[rBladeNum].trail.base, self->client->saber[rSaberNum].blade[rBladeNum].trail.tip, MASK_PLAYERSOLID | CONTENTS_LIGHTSABER | MASK_SHOT))
						{
							//saber was still in something at it's previous position, just check damage there.
						}
						else
						{
							//fire a series of traces thru the space the saber moved thru where it moved during the last frame.
							//This is done linearly so it's not going to 100% accurately reflect the normally curved movement
							//of the saber.
							while (dist < self->client->saber[rSaberNum].blade[rBladeNum].lengthMax)
							{
								vec3_t startpos;
								vec3_t endpos;
								//set new blade position
								VectorMA(boltOrigin, dist, self->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, endpos);

								//set old blade position
								VectorMA(self->client->saber[rSaberNum].blade[rBladeNum].trail.base, dist, olddir, startpos);

								if (CheckSaberDamage(self, rSaberNum, rBladeNum, startpos, endpos, MASK_PLAYERSOLID | CONTENTS_LIGHTSABER | MASK_SHOT))
								{
									//saber hit something, that's good enough for us!
									break;
								}

								//didn't hit anything, slide down the blade a bit and try again.
								dist += d_saberBoxTraceSize.value + self->client->saber[rSaberNum].blade[rBladeNum].radius;
							}
						}
					}
					else
					{
						//out of date blade position data or not in full damage move,
						//just do a single ghoul2 trace to keep the CPU useage down.
						CheckSaberDamage(self, rSaberNum, rBladeNum, boltOrigin, end, MASK_PLAYERSOLID | CONTENTS_LIGHTSABER | MASK_SHOT);
					}
				}
				else
				{
					CheckSaberDamage(self, rSaberNum, rBladeNum, boltOrigin, end, MASK_PLAYERSOLID | CONTENTS_LIGHTSABER | MASK_SHOT);
				}

				VectorCopy(boltOrigin, self->client->saber[rSaberNum].blade[rBladeNum].trail.base);
				VectorCopy(end, self->client->saber[rSaberNum].blade[rBladeNum].trail.tip);
				self->client->saber[rSaberNum].blade[rBladeNum].trail.lastTime = level.time;
				self->client->hasCurrentPosition = qtrue;

				//do hit effects
				WP_SaberDoClash(self, rSaberNum, rBladeNum);

				rBladeNum++;
			}

			rSaberNum++;
		}

		if (mySaber && mySaber->inuse)
		{
			trap->LinkEntity((sharedEntity_t*)mySaber);
		}

		if (!self->client->ps.saberInFlight)
		{
			self->client->ps.saberEntityState = 0;
		}
	}
finalUpdate:

	//Update the previous frame viewangle storage.
	VectorCopy(self->client->ps.viewangles, self->client->prevviewangle);
	self->client->prevviewtime = level.time;

	if (self->client->viewLockTime < level.time)
	{
		self->client->ps.userInt1 = 0;
	}

	if (clientOverride)
	{
		//if we get the client instance we don't even need to bother setting anims and stuff
		return;
	}

	G_UpdateClientAnims(self, animSpeedScale);
}

int WP_MissileBlockForBlock(const int saber_block)
{
	switch (saber_block)
	{
	case BLOCKED_UPPER_RIGHT:
		return BLOCKED_UPPER_RIGHT_PROJ;
	case BLOCKED_UPPER_LEFT:
		return BLOCKED_UPPER_LEFT_PROJ;
	case BLOCKED_LOWER_RIGHT:
		return BLOCKED_LOWER_RIGHT_PROJ;
	case BLOCKED_LOWER_LEFT:
		return BLOCKED_LOWER_LEFT_PROJ;
	case BLOCKED_TOP:
		return BLOCKED_TOP_PROJ;
	case BLOCKED_FRONT:
		return BLOCKED_FRONT_PROJ;
	case BLOCKED_BACK:
		return BLOCKED_BACK;
	default:;
	}
	return saber_block;
}

static float wp_block_force_chance(const gentity_t* defender)
{
	float block_factor = 1.0f;

	if (defender->r.svFlags & SVF_BOT || defender->s.eType == ET_NPC)
	{
		//bots just randomly parry to make up for them not intelligently parrying.
		if (defender->client->ps.fd.forcePower <= BLOCKPOINTS_HALF)
		{
			switch (defender->client->ps.fd.forcePowerLevel[FP_ABSORB])
			{
				// These actually make more sense to be separate from SABER blocking arcs.
			case FORCE_LEVEL_3:
				block_factor = 0.6f;
				break;
			case FORCE_LEVEL_2:
				block_factor = 0.5f;
				break;
			case FORCE_LEVEL_1:
				block_factor = 0.4f;
				break;
			default:
				block_factor = 0.1f;
				break;
			}
		}
		else
		{
			switch (defender->client->ps.fd.forcePowerLevel[FP_ABSORB])
			{
				// These actually make more sense to be separate from SABER blocking arcs.
			case FORCE_LEVEL_3:
				block_factor = 0.9f;
				break;
			case FORCE_LEVEL_2:
				block_factor = 0.8f;
				break;
			case FORCE_LEVEL_1:
				block_factor = 0.7f;
				break;
			default:
				block_factor = 0.1f;
				break;
			}
		}
	}

	return block_factor;
}

float manual_forceblocking(const gentity_t* defender)
{
	const float block_factor = wp_block_force_chance(defender);

	if (defender->client->ps.fd.forcePower <= BLOCKPOINTS_FATIGUE)
	{
		return qfalse;
	}

	if (!(defender->client->ps.fd.forcePowersKnown & 1 << FP_ABSORB))
	{
		//doesn't have absorb
		return qfalse;
	}

	if (defender->health <= 1
		|| PM_InKnockDown(&defender->client->ps)
		|| BG_InRoll(&defender->client->ps, defender->client->ps.legsAnim)
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim)
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim)
		|| PM_InSpecialJump(defender->client->ps.torsoAnim)
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE
		|| !walk_check(defender)
		|| in_camera)
	{
		return qfalse;
	}

	if (!(defender->client->buttons & BUTTON_BLOCK))
	{
		if (defender->r.svFlags & SVF_BOT)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			return block_factor;
		}

		if (defender->s.eType == ET_NPC)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			return block_factor;
		}

		if (defender->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(defender))
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			return block_factor;
		}
		return qfalse;
	}
	return qtrue;
}

qboolean Block_Button_Held(const gentity_t* defender)
{
	if (defender->client->ps.pm_flags & PMF_BLOCK_HELD)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean manual_saberblocking(const gentity_t* defender)
{
	if (defender->r.svFlags & SVF_BOT && defender->client->ps.weapon != WP_SABER)
	{
		return qfalse;
	}

	if (PM_RestAnim(defender->client->ps.legsAnim))
	{
		return qfalse;
	}

	if (!(defender->client->ps.fd.forcePowersKnown & 1 << FP_SABER_DEFENSE))
	{
		//doesn't have saber defense
		return qfalse;
	}

	if (defender->client->ps.weapon != WP_SABER
		|| defender->client->ps.weapon == WP_NONE
		|| defender->client->ps.weapon == WP_MELEE) //saber not here
	{
		return qfalse;
	}

	if (defender->client->ps.weapon == WP_SABER
		&& defender->client->ps.saberInFlight)
	{
		//saber not currently in use or available.
		return qfalse;
	}

	if (defender->health <= 1
		|| PM_InKnockDown(&defender->client->ps)
		|| BG_InFlipBack(defender->client->ps.torsoAnim)
		|| BG_InRoll(&defender->client->ps, defender->client->ps.legsAnim)
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim)
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim)
		|| PM_InSpecialJump(defender->client->ps.torsoAnim)
		|| PM_SaberInBounce(defender->client->ps.saber_move)
		|| PM_SaberInKnockaway(defender->client->ps.saber_move)
		|| PM_SaberInBrokenParry(defender->client->ps.saber_move)
		|| PM_SaberInMassiveBounce(defender->client->ps.saber_move)
		|| PM_SaberInBashedAnim(defender->client->ps.saber_move)
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE
		|| defender->client->ps.fd.blockPoints < BLOCKPOINTS_FIVE
		|| defender->client->ps.fd.forcePower < BLOCKPOINTS_FIVE)
	{
		return qfalse;
	}

	if (defender->client->buttons & BUTTON_ALT_ATTACK ||
		defender->client->buttons & BUTTON_FORCE_LIGHTNING ||
		defender->client->buttons & BUTTON_FORCEPOWER ||
		defender->client->buttons & BUTTON_FORCE_DRAIN ||
		defender->client->buttons & BUTTON_FORCEGRIP ||
		defender->client->buttons & BUTTON_DASH)
	{
		return qfalse;
	}

	if (SaberAttacking(defender))
	{
		if (defender->r.svFlags & SVF_BOT)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			return qtrue;
		}
		return qfalse;
	}

	if (!(defender->client->buttons & BUTTON_BLOCK))
	{
		if (defender->r.svFlags & SVF_BOT)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			return qtrue;
		}
		return qfalse;
	}
	return qtrue;
}

float manual_running_and_saberblocking(const gentity_t* defender)
{
	if (defender->r.svFlags & SVF_BOT && defender->client->ps.weapon != WP_SABER)
	{
		return qfalse;
	}

	if (defender->client->buttons & BUTTON_ATTACK)
	{
		return qfalse;
	}

	if (defender->client->buttons & BUTTON_WALKING)
	{
		return qfalse;
	}

	if (!(defender->client->ps.fd.forcePowersKnown & 1 << FP_SABER_DEFENSE))
	{
		//doesn't have saber defense
		return qfalse;
	}

	if (defender->health <= 1
		|| PM_InKnockDown(&defender->client->ps)
		|| BG_InRoll(&defender->client->ps, defender->client->ps.legsAnim)
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim)
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim)
		|| PM_InSpecialJump(defender->client->ps.torsoAnim)
		|| PM_SaberInBounce(defender->client->ps.saber_move)
		|| PM_SaberInKnockaway(defender->client->ps.saber_move)
		|| PM_SaberInMassiveBounce(defender->client->ps.torsoAnim)
		|| PM_SaberInBashedAnim(defender->client->ps.torsoAnim)
		|| PM_SaberInBrokenParry(defender->client->ps.saber_move)
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE
		|| defender->client->ps.fd.blockPoints < BLOCKPOINTS_FIVE
		|| defender->client->ps.fd.forcePower < BLOCKPOINTS_FIVE)
	{
		return qfalse;
	}

	if (defender->client->ps.weapon != WP_SABER
		|| defender->client->ps.weapon == WP_NONE
		|| defender->client->ps.weapon == WP_MELEE) //saber not here
	{
		return qfalse;
	}

	if (defender->client->ps.weapon == WP_SABER
		&& defender->client->ps.saberInFlight)
	{
		//saber not currently in use or available.
		return qfalse;
	}

	if (defender->client->buttons & BUTTON_ALT_ATTACK ||
		defender->client->buttons & BUTTON_FORCE_LIGHTNING ||
		defender->client->buttons & BUTTON_FORCEPOWER ||
		defender->client->buttons & BUTTON_FORCE_DRAIN ||
		defender->client->buttons & BUTTON_FORCEGRIP ||
		defender->client->buttons & BUTTON_DASH)
	{
		return qfalse;
	}

	if (SaberAttacking(defender) && defender->r.svFlags & SVF_BOT)
	{
		//bots just randomly parry to make up for them not intelligently parrying.
		return qtrue;
	}

	if (!(defender->client->buttons & BUTTON_BLOCK))
	{
		if (defender->r.svFlags & SVF_BOT)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			return qtrue;
		}
		return qfalse;
	}
	return qtrue;
}

qboolean manual_meleeblocking(const gentity_t* defender) //Is this guy blocking or not?
{
	if (defender->client->ps.weapon == WP_MELEE
		&& defender->client->buttons & BUTTON_WALKING
		&& defender->client->buttons & BUTTON_BLOCK
		&& !PM_kick_move(defender->client->ps.saber_move)
		&& !PM_KickingAnim(defender->client->ps.torsoAnim)
		&& !PM_KickingAnim(defender->client->ps.legsAnim)
		&& !PM_InRoll(&defender->client->ps)
		&& !PM_InKnockDown(&defender->client->ps)
		&& !(defender->client->ps.pm_flags & PMF_DUCKED))
	{
		return qtrue;
	}
	return qfalse;
}

qboolean manual_melee_dodging(const gentity_t* defender) //Is this guy dodgeing or not?
{
	if (defender->client->pers.botclass == BCLASS_BOBAFETT || defender->client->pers.botclass == BCLASS_MANDOLORIAN || defender->
		client->pers.botclass == BCLASS_MANDOLORIAN1 || defender->client->pers.botclass == BCLASS_MANDOLORIAN2)
	{
		return qfalse;
	}
	if (!PM_WalkingOrRunningAnim(defender->client->ps.legsAnim)
		&& defender->client->buttons & BUTTON_USE
		&& !(defender->client->buttons & BUTTON_WALKING)
		&& !(defender->client->buttons & BUTTON_BLOCK)
		&& !PM_kick_move(defender->client->ps.saber_move)
		&& !PM_KickingAnim(defender->client->ps.torsoAnim)
		&& !PM_KickingAnim(defender->client->ps.legsAnim)
		&& !PM_InRoll(&defender->client->ps)
		&& !PM_InKnockDown(&defender->client->ps)
		&& !(defender->client->ps.pm_flags & PMF_DUCKED))
	{
		return qtrue;
	}
	return qfalse;
}

float manual_npc_saberblocking(const gentity_t* defender)
{
	if (!(defender->r.svFlags & SVF_BOT))
	{
		return qfalse;
	}

	if (defender->r.svFlags & SVF_BOT && defender->client->ps.weapon != WP_SABER)
	{
		return qfalse;
	}

	if (BG_IsAlreadyinTauntAnim(defender->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (PM_SaberInKata(defender->client->ps.saber_move))
	{
		return qfalse;
	}

	if (defender->health <= 1
		|| PM_InKnockDown(&defender->client->ps)
		|| BG_InRoll(&defender->client->ps, defender->client->ps.legsAnim)
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim)
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim)
		|| PM_InSpecialJump(defender->client->ps.torsoAnim)
		|| PM_SaberInBounce(defender->client->ps.saber_move)
		|| PM_SaberInKnockaway(defender->client->ps.saber_move)
		|| PM_SaberInMassiveBounce(defender->client->ps.torsoAnim)
		|| PM_SaberInBashedAnim(defender->client->ps.torsoAnim)
		|| PM_SaberInBrokenParry(defender->client->ps.saber_move)
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE
		|| defender->client->ps.fd.blockPoints < BLOCKPOINTS_FIVE
		|| defender->client->ps.fd.forcePower < BLOCKPOINTS_FIVE)
	{
		return qfalse;
	}

	if (defender->client->ps.weapon != WP_SABER
		|| defender->client->ps.weapon == WP_NONE
		|| defender->client->ps.weapon == WP_MELEE) //saber not here
	{
		return qfalse;
	}

	if (defender->client->ps.weapon == WP_SABER
		&& defender->client->ps.saberInFlight)
	{
		//saber not currently in use or available.
		return qfalse;
	}

	if (defender->client->ps.weapon == WP_SABER && defender->client->ps.saberHolstered)
	{
		//saber not currently in use or available.
		return qfalse;
	}

	if (PM_SaberInMassiveBounce(defender->client->ps.torsoAnim) || PM_SaberInBashedAnim(defender->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (SaberAttacking(defender) && defender->client->ps.saberFatigueChainCount < MISHAPLEVEL_HUDFLASH)
	{
		//bots just randomly parry to make up for them not intelligently parrying.
		return qtrue;
	}

	return qtrue;
}

int PlayerCanAbsorbKick(const gentity_t* defender, const vec3_t push_dir) //Can the player absorb a kick
{
	vec3_t p_l_angles, p_l_fwd;

	const qboolean is_holding_block_button = defender->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking

	if (!defender || !defender->client)
	{
		//non-humanoids can't absorb kicks.
		return qfalse;
	}
	if (!is_holding_block_button) // Must be holding Block button
	{
		// Not doing any blocking can't absorb kicks.
		return qfalse;
	}

	if (!walk_check(defender))
	{
		// runners cant absorb kick
		return qfalse;
	}
	// Cant absorb kick if
	if (defender->health <= 1 // low health (so dead people dont do it)
		|| BG_InKnockDown(defender->client->ps.legsAnim) // In a Knockdown (legs)
		|| BG_InKnockDown(defender->client->ps.torsoAnim) // In a Knockdown (torso)
		|| PM_InRoll(&defender->client->ps) // In a roll
		|| BG_InFlipBack(defender->client->ps.torsoAnim) // Flipping back
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim) // lost a saber lock
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim) // won a saber lock
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim) // A special saber attack
		|| PM_InSpecialJump(defender->client->ps.torsoAnim) // A Force jump
		|| PM_SaberInBounce(defender->client->ps.saber_move) // Saber is bouncing
		|| PM_SaberInKnockaway(defender->client->ps.saber_move) // Saber is being knocked away
		|| PM_SaberInBrokenParry(defender->client->ps.saber_move) // Your parry got smashed open
		|| PM_kick_move(defender->client->ps.saber_move) // If you are doing a kick / melee / slap
		|| SaberAttacking(defender) // you are saber attacking
		|| PM_InGrappleMove(defender->client->ps.torsoAnim) // Trying to grab
		|| defender->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1
		// No force saber deference (Gunners cant do it at all)
		|| defender->client->ps.eFlags2 & EF2_FLYING // Bobafett flying cant do it
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE // Your in the air (jumping).
		|| defender->client->ps.fd.blockPoints < FATIGUE_DODGEING // Less than 35 Block points
		|| defender->client->ps.fd.forcePower < FATIGUE_DODGEING // Less than 35 Force points
		|| defender->client->ps.saberFatigueChainCount >= MISHAPLEVEL_TEN) // Your saber fatigued
	{
		return qfalse;
	}

	VectorSet(p_l_angles, 0, defender->client->ps.viewangles[YAW], 0);
	AngleVectors(p_l_angles, p_l_fwd, NULL, NULL);

	if (DotProduct(p_l_fwd, push_dir) > 0.2f)
	{
		//not hit in the front, can't absorb kick.
		return qfalse;
	}

	return qtrue; // If all that stuff above is clear then you can convert a knockdown in to a stagger
}

int BotCanAbsorbKick(const gentity_t* defender, const vec3_t push_dir) //Can the player absorb a kick
{
	vec3_t p_l_angles, p_l_fwd;

	if (!defender || !defender->client)
	{
		//non-humanoids can't absorb kicks.
		return qfalse;
	}

	if (!walk_check(defender))
	{
		// runners cant absorb kick
		return qfalse;
	}
	// Cant absorb kick if
	if (defender->health <= 1 // low health (so dead people dont do it)
		|| BG_InKnockDown(defender->client->ps.legsAnim) // In a Knockdown (legs)
		|| BG_InKnockDown(defender->client->ps.torsoAnim) // In a Knockdown (torso)
		|| PM_InRoll(&defender->client->ps) // In a roll
		|| BG_InFlipBack(defender->client->ps.torsoAnim) // Flipping back
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim) // lost a saber lock
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim) // won a saber lock
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim) // A special saber attack
		|| PM_InSpecialJump(defender->client->ps.torsoAnim) // A Force jump
		|| PM_SaberInBounce(defender->client->ps.saber_move) // Saber is bouncing
		|| PM_SaberInKnockaway(defender->client->ps.saber_move) // Saber is being knocked away
		|| PM_SaberInBrokenParry(defender->client->ps.saber_move) // Your parry got smashed open
		|| PM_kick_move(defender->client->ps.saber_move) // If you are doing a kick / melee / slap
		|| SaberAttacking(defender) // you are saber attacking
		|| PM_InGrappleMove(defender->client->ps.torsoAnim) // Trying to grab
		|| defender->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1
		// No force saber deference (Gunners cant do it at all)
		|| defender->client->ps.eFlags2 & EF2_FLYING // Bobafett flying cant do it
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE // Your in the air (jumping).
		|| defender->client->ps.fd.blockPoints < FATIGUE_DODGEING // Less than 35 Block points
		|| defender->client->ps.fd.forcePower < FATIGUE_DODGEING // Less than 35 Force points
		|| defender->client->ps.saberFatigueChainCount >= MISHAPLEVEL_TEN) // Your saber fatigued
	{
		return qfalse;
	}

	VectorSet(p_l_angles, 0, defender->client->ps.viewangles[YAW], 0);
	AngleVectors(p_l_angles, p_l_fwd, NULL, NULL);

	if (DotProduct(p_l_fwd, push_dir) > 0.2f)
	{
		//not hit in the front, can't absorb kick.
		return qfalse;
	}

	return qtrue; // If all that stuff above is clear then you can convert a knockdown in to a stagger
}

float manual_npc_kick_absorbing(const gentity_t* defender)
{
	if (!(defender->r.svFlags & SVF_BOT))
	{
		return qfalse;
	}

	if (defender->client->ps.saberFatigueChainCount > MISHAPLEVEL_TEN && (defender->client->ps.fd.forcePower <=
		BLOCKPOINTS_HALF || defender->client->ps.fd.blockPoints <= BLOCKPOINTS_HALF))
	{
		return qfalse;
	}

	if (defender->health <= 1
		|| PM_InKnockDown(&defender->client->ps)
		|| BG_InRoll(&defender->client->ps, defender->client->ps.legsAnim)
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim)
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim)
		|| PM_InSpecialJump(defender->client->ps.torsoAnim)
		|| PM_SaberInBounce(defender->client->ps.saber_move)
		|| PM_SaberInReturn(defender->client->ps.saber_move)
		|| PM_SaberInKnockaway(defender->client->ps.saber_move)
		|| PM_SaberInBrokenParry(defender->client->ps.saber_move)
		|| PM_SaberInMassiveBounce(defender->client->ps.saber_move)
		|| PM_SaberInBashedAnim(defender->client->ps.saber_move)
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE
		|| defender->client->ps.fd.blockPoints < BLOCKPOINTS_THIRTY
		|| defender->client->ps.fd.forcePower < BLOCKPOINTS_THIRTY
		|| defender->client->ps.userInt3 & 1 << FLAG_FATIGUED)
	{
		return qfalse;
	}

	if (defender->client->ps.weapon != WP_SABER
		|| defender->client->ps.weapon == WP_NONE
		|| defender->client->ps.weapon == WP_MELEE) //saber not here
	{
		return qfalse;
	}

	if (defender->client->ps.weapon == WP_SABER
		&& defender->client->ps.saberInFlight)
	{
		//saber not currently in use or available.
		return qfalse;
	}

	if (defender->client->ps.weapon == WP_SABER
		&& defender->client->ps.saberHolstered)
	{
		//saber not currently in use or available.
		return qfalse;
	}

	if (!walk_check(defender))
	{
		//can't block while running.
		return qfalse;
	}

	if (defender->r.svFlags & SVF_BOT)
	{
		return qtrue;
	}

	return qtrue;
}

extern qboolean BG_FullBodyEmoteAnim(int anim);
extern qboolean BG_FullBodyCowerstartAnim(int anim);

qboolean IsSurrendering(const gentity_t* self)
{
	if (self->r.svFlags & SVF_BOT)
	{
		return qfalse;
	}

	if (self->client->ps.weapon != WP_SABER)
	{
		return qfalse;
	}

	if (!BG_FullBodyEmoteAnim(self->client->ps.torsoAnim))
	{
		// Ignore players surrendering
		return qfalse;
	}

	return qtrue;
}

qboolean IsCowering(const gentity_t* self)
{
	if (self->r.svFlags & SVF_BOT)
	{
		return qfalse;
	}

	if (!BG_FullBodyCowerstartAnim(self->client->ps.torsoAnim))
	{
		// Ignore players surrendering
		return qfalse;
	}

	return qtrue;
}

extern qboolean BG_FullBodyRespectAnim(int anim);

qboolean IsRespecting(const gentity_t* self)
{
	if (self->r.svFlags & SVF_BOT)
	{
		return qfalse;
	}

	if (!BG_FullBodyRespectAnim(self->client->ps.torsoAnim))
	{
		// Ignore players surrendering
		return qfalse;
	}

	return qtrue;
}

extern qboolean BG_AnimRequiresResponce(int anim);

qboolean IsAnimRequiresResponce(const gentity_t* self)
{
	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
	{
		return qfalse;
	}

	if (!BG_AnimRequiresResponce(self->client->ps.torsoAnim))
	{
		// Ignore players surrendering
		return qfalse;
	}

	return qtrue;
}

qboolean WP_SaberMBlockDirection(gentity_t* self, vec3_t hitloc, const qboolean missileBlock)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	vec3_t cl_eye;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.7f);

	VectorCopy(self->client->ps.origin, cl_eye);
	cl_eye[2] += self->client->ps.viewheight;
	VectorSubtract(hitloc, cl_eye, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, NULL, right, NULL);
	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - cl_eye[2];

	if (self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING)
	{
		//don't block
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (PM_SaberInAttack(self->client->ps.saber_move) ||
		PM_SuperBreakLoseAnim(self->client->ps.torsoAnim) ||
		PM_SuperBreakWinAnim(self->client->ps.torsoAnim) ||
		PM_SaberInBrokenParry(self->client->ps.saber_move) ||
		PM_SaberInKnockaway(self->client->ps.saber_move) ||
		BG_InRoll(&self->client->ps, self->client->ps.legsAnim))
	{
		//don't block
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		// can't block in a stagger animation
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (self->client->ps.fd.blockPoints <= BLOCKPOINTS_FAIL || self->client->ps.fd.forcePower <= BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (!inFront && self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.fd.saberAnimLevel)
		{
		case SS_STAFF:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		case SS_DUAL:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		default:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		}
		self->client->ps.saberBlocked = BLOCKED_BACK;
		self->client->ps.weaponTime = Q_irand(300, 600);
	}
	else if (zdiff > 0)
	{
		if (rightdot > 0.3)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B7_TR___, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B6_TR___, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_TR_ALT, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B7_TL___, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B6_TL___, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_TL_ALT, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_T_, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else if (zdiff > -20)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block
		}
		if (rightdot > 0.1)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B7_TR___, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B6_TR___, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_TR_ALT, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.1)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B7_TL___, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B6_TL___, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_TL_ALT, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_T_, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
				//BOTTOM RIGHT
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B7_BR___, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B6_BR___, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B1_BR___, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
				//BOTTOM LEFT
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B7_BL___, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B6_BL___, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_B1_BL___, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}

	if (missileBlock)
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
		self->client->ps.weaponTime = Q_irand(300, 600);
	}

	if (self->r.svFlags & SVF_BOT && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parryReCalcTime = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

qboolean WP_SaberBlockNonRandom(gentity_t* self, vec3_t hitloc, const qboolean missileBlock)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	vec3_t cl_eye;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.7f);

	VectorCopy(self->client->ps.origin, cl_eye);
	cl_eye[2] += self->client->ps.viewheight;
	VectorSubtract(hitloc, cl_eye, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, NULL, right, NULL);
	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - cl_eye[2];

	if (self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING)
	{
		//don't block
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (PM_SaberInAttack(self->client->ps.saber_move) ||
		PM_SuperBreakLoseAnim(self->client->ps.torsoAnim) ||
		PM_SuperBreakWinAnim(self->client->ps.torsoAnim) ||
		PM_SaberInBrokenParry(self->client->ps.saber_move) ||
		PM_SaberInKnockaway(self->client->ps.saber_move) ||
		BG_InRoll(&self->client->ps, self->client->ps.legsAnim))
	{
		//don't block
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		// can't block in a stagger animation
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (self->client->ps.fd.blockPoints <= BLOCKPOINTS_FAIL || self->client->ps.fd.forcePower <= BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (!inFront && self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.fd.saberAnimLevel)
		{
		case SS_STAFF:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		case SS_DUAL:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		default:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		}
		self->client->ps.saberBlocked = BLOCKED_BACK;
		self->client->ps.weaponTime = Q_irand(300, 600);
	}
	else if (zdiff > 0)
	{
		if (rightdot > 0.3)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_TR, SETANIM_AFLAG_PACE, 0);
				self->client->ps.weaponTime = Q_irand(300, 600);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_TR, SETANIM_AFLAG_PACE, 0);
				self->client->ps.weaponTime = Q_irand(300, 600);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_TR, SETANIM_AFLAG_PACE, 0);
				self->client->ps.weaponTime = Q_irand(300, 600);
				break;
			}
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_TL, SETANIM_AFLAG_PACE, 0);
				self->client->ps.weaponTime = Q_irand(300, 600);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_TL, SETANIM_AFLAG_PACE, 0);
				self->client->ps.weaponTime = Q_irand(300, 600);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_TL, SETANIM_AFLAG_PACE, 0);
				self->client->ps.weaponTime = Q_irand(300, 600);
				break;
			}
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else if (zdiff > -20)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block, so we need to duck
		}
		if (rightdot > 0.1)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_TR, SETANIM_AFLAG_PACE, 0);
				self->client->ps.weaponTime = Q_irand(300, 600);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_TR, SETANIM_AFLAG_PACE, 0);
				self->client->ps.weaponTime = Q_irand(300, 600);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_TR, SETANIM_AFLAG_PACE, 0);
				self->client->ps.weaponTime = Q_irand(300, 600);
				break;
			}
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_TL, SETANIM_AFLAG_PACE, 0);
				self->client->ps.weaponTime = Q_irand(300, 600);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_TL, SETANIM_AFLAG_PACE, 0);
				self->client->ps.weaponTime = Q_irand(300, 600);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_TL, SETANIM_AFLAG_PACE, 0);
				self->client->ps.weaponTime = Q_irand(300, 600);
				break;
			}
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_BR, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_BR, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_BR, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_BL, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_BL, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_BL, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}

	if (missileBlock)
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
		self->client->ps.weaponTime = Q_irand(300, 600);
	}

	if (self->r.svFlags & SVF_BOT && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parryReCalcTime = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

qboolean WP_SaberBouncedSaberDirection(gentity_t* self, vec3_t hitloc, const qboolean missileBlock)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	vec3_t cl_eye;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.7f);

	VectorCopy(self->client->ps.origin, cl_eye);
	cl_eye[2] += self->client->ps.viewheight;
	VectorSubtract(hitloc, cl_eye, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, NULL, right, NULL);
	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - cl_eye[2];

	if (self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING)
	{
		//don't block
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (PM_SaberInAttack(self->client->ps.saber_move) ||
		PM_SuperBreakLoseAnim(self->client->ps.torsoAnim) ||
		PM_SuperBreakWinAnim(self->client->ps.torsoAnim) ||
		PM_SaberInBrokenParry(self->client->ps.saber_move) ||
		PM_SaberInKnockaway(self->client->ps.saber_move) ||
		BG_InRoll(&self->client->ps, self->client->ps.legsAnim))
	{
		//don't block
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		// can't block in a stagger animation
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (self->client->ps.fd.blockPoints <= BLOCKPOINTS_FAIL || self->client->ps.fd.forcePower <= BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (!inFront && self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.fd.saberAnimLevel)
		{
		case SS_STAFF:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		case SS_DUAL:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		default:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		}
		self->client->ps.saberBlocked = BLOCKED_BACK;
		self->client->ps.weaponTime = Q_irand(300, 600);
	}
	else if (zdiff > 0)
	{
		if (rightdot > 0.3)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R7_TR_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R6_TR_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R1_TR_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R7_TL_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R6_TL_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R1_TL_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_T_, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else if (zdiff > -20)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block
		}
		if (rightdot > 0.1)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R7_TR_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R6_TR_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R1_TR_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.1)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R7_TL_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R6_TL_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R1_TL_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K1_S1_T_, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_BR, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_BR, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_BR, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_BL, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_BL, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_BL, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}

	if (missileBlock)
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
		self->client->ps.weaponTime = Q_irand(300, 600);
	}

	if (self->r.svFlags & SVF_BOT && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parryReCalcTime = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

qboolean WP_SaberFatiguedParryDirection(gentity_t* self, vec3_t hitloc, const qboolean missileBlock)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	vec3_t cl_eye;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.7f);

	VectorCopy(self->client->ps.origin, cl_eye);
	cl_eye[2] += self->client->ps.viewheight;
	VectorSubtract(hitloc, cl_eye, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, NULL, right, NULL);
	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - cl_eye[2];

	if (self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING)
	{
		//don't block
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (PM_SaberInAttack(self->client->ps.saber_move) ||
		PM_SuperBreakLoseAnim(self->client->ps.torsoAnim) ||
		PM_SuperBreakWinAnim(self->client->ps.torsoAnim) ||
		PM_SaberInBrokenParry(self->client->ps.saber_move) ||
		PM_SaberInKnockaway(self->client->ps.saber_move) ||
		BG_InRoll(&self->client->ps, self->client->ps.legsAnim))
	{
		//don't block
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		// can't block in a stagger animation
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (self->client->ps.fd.blockPoints <= BLOCKPOINTS_FAIL || self->client->ps.fd.forcePower <= BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (!inFront && self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.fd.saberAnimLevel)
		{
		case SS_STAFF:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		case SS_DUAL:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		default:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		}
		self->client->ps.saberBlocked = BLOCKED_BACK;
		self->client->ps.weaponTime = Q_irand(300, 600);
	}
	else if (zdiff > 0)
	{
		if (rightdot > 0.3)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V7_TR_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V6_TR_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V1_TR_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V7_TL_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V6_TL_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V1_TL_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V7_T__S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V6_T__S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V1_T__S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else if (zdiff > -20)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block
		}
		if (rightdot > 0.1)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V7__R_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V6__R_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V1__R_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.1)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V7__L_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V6__L_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V1_TL_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V7_T__S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V6_T__S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V1_T__S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V7_BR_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V6_BR_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V1_BR_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V7_BL_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V6_BL_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_V1_BL_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}

	if (missileBlock)
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
		self->client->ps.weaponTime = Q_irand(300, 600);
	}

	if (self->r.svFlags & SVF_BOT && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parryReCalcTime = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

//missile blocks

qboolean wp_saber_block_non_random_missile(gentity_t* self, vec3_t hitloc, const qboolean missileBlock)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	vec3_t cl_eye;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.7f);

	VectorCopy(self->client->ps.origin, cl_eye);
	cl_eye[2] += self->client->ps.viewheight;
	VectorSubtract(hitloc, cl_eye, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, NULL, right, NULL);
	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - cl_eye[2];

	if (self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING)
	{
		//don't block
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (PM_SaberInAttack(self->client->ps.saber_move) ||
		PM_SuperBreakLoseAnim(self->client->ps.torsoAnim) ||
		PM_SuperBreakWinAnim(self->client->ps.torsoAnim) ||
		PM_SaberInBrokenParry(self->client->ps.saber_move) ||
		PM_SaberInKnockaway(self->client->ps.saber_move) ||
		BG_InRoll(&self->client->ps, self->client->ps.legsAnim))
	{
		//don't block
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		// can't block in a stagger animation
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (self->client->ps.fd.blockPoints <= BLOCKPOINTS_FAIL || self->client->ps.fd.forcePower <= BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (!inFront && self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.fd.saberAnimLevel)
		{
		case SS_STAFF:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		case SS_DUAL:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		default:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		}
		self->client->ps.saberBlocked = BLOCKED_BACK;
		self->client->ps.weaponTime = Q_irand(300, 600);
	}
	else if (zdiff > 0)
	{
		if (rightdot > 0.3)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_TR, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_TR, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_TR, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_TL, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_TL, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_TL, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else if (zdiff > -20)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block
		}
		if (rightdot > 0.1)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_TR, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_TR, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_TR, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.1)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_TL, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_TL, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_TL, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_BR, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_BR, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_BR, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_BL, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_BL, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_BL, SETANIM_AFLAG_PACE, 0);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}

	if (missileBlock)
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
		self->client->ps.weaponTime = Q_irand(300, 600);
	}

	if (self->r.svFlags & SVF_BOT && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parryReCalcTime = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

qboolean WP_SaberBlockBolt(gentity_t* self, vec3_t hitloc, const qboolean missileBlock)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	vec3_t cl_eye;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.7f);

	VectorCopy(self->client->ps.origin, cl_eye);
	cl_eye[2] += self->client->ps.viewheight;
	VectorSubtract(hitloc, cl_eye, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, NULL, right, NULL);
	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - cl_eye[2];

	if (self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING)
	{
		//don't block
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (PM_SaberInAttack(self->client->ps.saber_move) ||
		PM_SuperBreakLoseAnim(self->client->ps.torsoAnim) ||
		PM_SuperBreakWinAnim(self->client->ps.torsoAnim) ||
		PM_SaberInBrokenParry(self->client->ps.saber_move) ||
		PM_SaberInKnockaway(self->client->ps.saber_move) ||
		BG_InRoll(&self->client->ps, self->client->ps.legsAnim))
	{
		//don't block
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		// can't block in a stagger animation
		if (self->health > 0
			&& self->r.svFlags & SVF_BOT
			&& self->client->ps.weapon == WP_SABER)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (self->client->ps.fd.blockPoints <= BLOCKPOINTS_FAIL || self->client->ps.fd.forcePower <= BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (!inFront && self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.fd.saberAnimLevel)
		{
		case SS_STAFF:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		case SS_DUAL:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		default:
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_B1_, SETANIM_AFLAG_PACE, 0);
			break;
		}
		self->client->ps.saberBlocked = BLOCKED_BACK;
		self->client->ps.weaponTime = Q_irand(300, 600);
	}
	else if (zdiff > 0)
	{
		if (rightdot > 0.3)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R7_TR_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R6_TR_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R1_TR_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R7_TL_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R6_TL_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R1_TL_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_SABER_BLOCKBOLT, SETANIM_AFLAG_PACE, 0);
				break;
			}
		}
	}
	else if (zdiff > -20)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block
		}
		if (rightdot > 0.1)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R7_TR_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R6_TR_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R1_TR_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
		}
		else if (rightdot < -0.1)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R7_TL_S7, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R6_TL_S6, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_R1_TL_S1, SETANIM_AFLAG_PACE, 0);
				break;
			}
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_SABER_BLOCKBOLT, SETANIM_AFLAG_PACE, 0);
				break;
			}
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_BR, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_BR, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_BR, SETANIM_AFLAG_PACE, 0);
				break;
			}
		}
		else
		{
			switch (self->client->ps.fd.saberAnimLevel)
			{
			case SS_STAFF:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_BL, SETANIM_AFLAG_PACE, 0);
				break;
			case SS_DUAL:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_BL, SETANIM_AFLAG_PACE, 0);
				break;
			default:
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_BL, SETANIM_AFLAG_PACE, 0);
				break;
			}
		}
	}

	if (missileBlock)
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
		self->client->ps.weaponTime = Q_irand(300, 600);
	}

	if (self->r.svFlags & SVF_BOT && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parryReCalcTime = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

extern float Q_clamp(float min, float value, float max);

int WP_SaberCanBlockThrownSaber(gentity_t* self, vec3_t point, qboolean projectile)
{
	if (!self || !self->client || !point)
	{
		return 0;
	}

	if (!(self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
	{
		if (self->r.svFlags & SVF_BOT)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			if (self->client->ps.weapon == WP_SABER && !BG_SabersOff(&self->client->ps) && !self->client->ps.saberInFlight)
			{
				return 1;
			}
			return 0;
		}
		return 0;
	}

	if (PM_SaberInBrokenParry(self->client->ps.saber_move))
	{
		return 0;
	}

	if (!self->client->ps.saberEntityNum)
	{ //saber is knocked away
		return 0;
	}

	if (BG_SabersOff(&self->client->ps))
	{
		return 0;
	}

	if (self->client->ps.weapon != WP_SABER)
	{
		return 0;
	}

	if (self->client->ps.saberInFlight)
	{
		return 0;
	}

	if (projectile)
	{
		wp_saber_block_non_random_missile(self, point, projectile);
	}
	return 1;
}

qboolean HasSetSaberOnly(void)
{
	int i = 0;
	int w_disable;

	if (level.gametype == GT_JEDIMASTER)
	{
		//set to 0
		return qfalse;
	}

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		w_disable = g_duelWeaponDisable.integer;
	}
	else
	{
		w_disable = g_weaponDisable.integer;
	}

	while (i < WP_NUM_WEAPONS)
	{
		if (!(w_disable & 1 << i) &&
			i != WP_SABER && i != WP_NONE)
		{
			return qfalse;
		}

		i++;
	}

	return qtrue;
}

//additional kick code used by NPCs to choice kick moves.
extern float G_GroundDistance(const gentity_t* self);
extern qboolean G_CheckEnemyPresence(int dir, float radius);

saber_moveName_t G_PickAutoKick(gentity_t* self, const gentity_t* enemy)
{
	vec3_t v_fwd;
	vec3_t v_rt;
	vec3_t enemy_dir;
	vec3_t fwd_angs;
	saber_moveName_t kick_move = LS_NONE;
	if (!self || !self->client)
	{
		return LS_NONE;
	}
	if (!enemy)
	{
		return LS_NONE;
	}
	/*if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		return LS_NONE;
	}*/
	VectorSet(fwd_angs, 0, self->client->ps.viewangles[YAW], 0);
	VectorSubtract(enemy->r.currentOrigin, self->r.currentOrigin, enemy_dir);
	VectorNormalize(enemy_dir); //not necessary, I guess, but doesn't happen often
	AngleVectors(fwd_angs, v_fwd, v_rt, NULL);
	const float f_dot = DotProduct(enemy_dir, v_fwd);
	const float r_dot = DotProduct(enemy_dir, v_rt);
	if (fabs(r_dot) > 0.5f && fabs(f_dot) < 0.5f)
	{
		//generally to one side
		if (r_dot > 0)
		{
			//kick right
			if (self->client->ps.weapon == WP_SABER && !BG_SabersOff(&self->client->ps))
			{
				kick_move = LS_SLAP_R;
			}
			else
			{
				kick_move = LS_KICK_R;
			}
		}
		else
		{
			//kick left
			if (self->client->ps.weapon == WP_SABER && !BG_SabersOff(&self->client->ps))
			{
				kick_move = LS_SLAP_L;
			}
			else
			{
				kick_move = LS_KICK_L;
			}
		}
	}
	else if (fabs(f_dot) > 0.5f && fabs(r_dot) < 0.5f)
	{
		//generally in front or behind us
		if (f_dot > 0)
		{
			//kick fwd
			if (self->client->ps.groundEntityNum != ENTITYNUM_NONE && self->client->ps.weapon == WP_SABER && !
				BG_SabersOff(&self->client->ps))
			{
				kick_move = LS_KICK_F2;
			}
			else
			{
				kick_move = LS_KICK_F;
			}
		}
		else
		{
			//kick back
			if (PM_CrouchingAnim(self->client->ps.legsAnim))
			{
				kick_move = LS_KICK_B3;
			}
			else
			{
				kick_move = LS_KICK_B;
			}
		}
	}
	else
	{
		//diagonal to us, kick would miss
	}
	if (kick_move != LS_NONE)
	{
		//have a valid one to do
		if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			//if in air, convert kick to an in-air kick
			const float g_dist = G_GroundDistance(self);
			//let's only allow air kicks if a certain distance from the ground
			//it's silly to be able to do them right as you land.
			//also looks wrong to transition from a non-complete flip anim...
			if ((!PM_FlippingAnim(self->client->ps.legsAnim) || self->client->ps.legsTimer <= 0) &&
				g_dist > 64.0f && //strict minimum
				g_dist > -self->client->ps.velocity[2] - 64.0f)
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
					kick_move = LS_NONE;
					break;
				}
			}
			else
			{
				//leave it as a normal kick unless we're too high up
				if (g_dist > 128.0f || self->client->ps.velocity[2] >= 0)
				{
					//off ground, but too close to ground
					kick_move = LS_NONE;
				}
			}
		}
	}
	if (kick_move != LS_NONE)
	{
		//we have a kick_move, do it!
		const int kick_anim = saber_moveData[kick_move].animToUse;
		if (kick_anim != -1)
		{
			G_SetAnim(self, NULL, SETANIM_BOTH, kick_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
			self->client->ps.weaponTime = self->client->ps.legsTimer;
			self->client->ps.saber_move = kick_move;
		}
	}
	return kick_move;
}

static qboolean G_EnemyInKickRange(const gentity_t* self, const gentity_t* enemy)
{
	//is the enemy within kick range?
	if (!self || !enemy)
	{
		return qfalse;
	}
	if (fabs(self->r.currentOrigin[2] - enemy->r.currentOrigin[2]) < 32)
	{
		//generally at same height
		if (DistanceHorizontal(self->r.currentOrigin, enemy->r.currentOrigin) <= STAFF_KICK_RANGE + 8.0f + self->r.
			maxs[0] * 1.5f + enemy->r.maxs[0] * 1.5f)
		{
			//within kicking range!
			return qtrue;
		}
	}
	return qfalse;
}

extern qboolean BG_InKnockDown(int anim);

qboolean G_CanKickEntity(const gentity_t* self, const gentity_t* target)
{
	//can we kick the given target?
	if (target && target->client
		&& !BG_InKnockDown(target->client->ps.legsAnim)
		&& G_EnemyInKickRange(self, target))
	{
		return qtrue;
	}
	return qfalse;
}

static void SaberBallisticsTouch(gentity_t* saberent, const gentity_t* other, trace_t* trace)
{
	//touch function for sabers in ballistics mode
	gentity_t* saber_own = &g_entities[saberent->r.ownerNum];

	if (other && other->s.number == saberent->r.ownerNum)
	{
		//racc - we hit our owner, just ignore it.
		return;
	}

	//knock the saber down after impact.
	saberKnockDown(saberent, saber_own, other);
}

//This is the bounce count used for ballistic sabers.  We watch this value to know
//when to make the switch to a simple dropped saber.
#define BALLISTICSABER_BOUNCECOUNT 10

static void SaberBallisticsThink(gentity_t* saberEnt)
{
	//think function for sabers in ballistics mode
	//G_RunObject(saberEnt);

	saberEnt->nextthink = level.time;

	if (saberEnt->s.eFlags & EF_MISSILE_STICK)
	{
		vec3_t dir;
		gentity_t* saber_owner = &g_entities[saberEnt->r.ownerNum];

		//racc - Apparently pos1 is the hand bolt.
		VectorSubtract(saber_owner->r.currentOrigin, saberEnt->r.currentOrigin, dir);

		const float owner_len = VectorLength(dir);

		if (owner_len <= 32)
		{
			//racc - picked up the saber.
			G_Sound(saberEnt, CHAN_AUTO, G_SoundIndex("sound/weapons/saber/saber_catch.mp3"));
			G_SetAnim(saber_owner, NULL, SETANIM_TORSO, BOTH_STAND1TO2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);

			saberEnt->s.eFlags &= ~EF_MISSILE_STICK;
			saberReactivate(saberEnt, saber_owner);

			saberEnt->speed = 0;
			saberEnt->genericValue5 = 0;
			saberEnt->nextthink = level.time;

			saberEnt->r.contents = CONTENTS_LIGHTSABER;

			saber_owner->client->ps.saberInFlight = qfalse;
			saber_owner->client->ps.saberEntityState = 0;
			saber_owner->client->ps.saberCanThrow = qfalse;
			saber_owner->client->ps.saberThrowDelay = level.time + 300;

			saberEnt->touch = SaberGotHit;

			saberEnt->think = SaberUpdateSelf;
			saberEnt->genericValue5 = 0;
			saberEnt->nextthink = level.time + 50;
			WP_SaberRemoveG2Model(saberEnt);

			saber_owner->client->ps.saberEntityNum = saber_owner->client->saberStoredIndex;
			return;
		}
		if (saber_owner->client->pers.cmd.buttons & BUTTON_ATTACK
			&& level.time - saber_owner->client->saberKnockedTime > MIN_LEAVE_TIME
			&& !(saber_owner->client->pers.cmd.buttons & BUTTON_BLOCK)
			&& !(saber_owner->r.svFlags & SVF_BOT))
		{
			//we want to pull the saber back.
			saberEnt->s.eFlags &= ~EF_MISSILE_STICK;

			saberReactivate(saberEnt, saber_owner);

			saberEnt->touch = SaberGotHit;

			saberEnt->think = saberBackToOwner;
			saberEnt->speed = 0;
			saberEnt->genericValue5 = 0;
			saberEnt->nextthink = level.time;

			saberEnt->r.contents = CONTENTS_LIGHTSABER;
		}
		else if (level.time - saber_owner->client->saberKnockedTime > MAX_LEAVE_TIME)
		{
			//We left it too long.  Just have it turn off and fall to the ground.
			VectorClear(saberEnt->s.pos.trDelta);
			VectorClear(saberEnt->s.apos.trDelta);
			saberEnt->speed = 0;
			saberKnockDown(saberEnt, saber_owner, saber_owner);
		}
	}
	else
	{
		//flying thru the air
		if (saberEnt->bounceCount != BALLISTICSABER_BOUNCECOUNT)
		{
			//we've hit an object and bounced off it, go to dead saber mode
			gentity_t* saberOwn = &g_entities[saberEnt->r.ownerNum];
			saberKnockDown(saberEnt, saberOwn, saberOwn);
		}
		else
		{
			G_RunObject(saberEnt);
		}
	}
}

void thrownSaberBallistics(gentity_t* saberEnt, const gentity_t* saber_own, const qboolean stuck)
{
	//this function converts the saber from thrown saber that's being held on course by the force into a saber that's just ballastically moving.
	const saberInfo_t* saber1 = BG_MySaber(saber_own->clientNum, 0);

	if (stuck)
	{
		//lightsaber is stuck in something, just hang there.
		VectorClear(saberEnt->s.pos.trDelta);
		VectorClear(saberEnt->s.apos.trDelta);

		//set the sticky eflag
		saberEnt->s.eFlags = EF_MISSILE_STICK;

		//don't move at all.
		saberEnt->s.pos.trType = TR_STATIONARY;
		saberEnt->s.apos.trType = TR_STATIONARY;

		//set the force retrieve timer so we'll know when to pull it out
		//of the object with the force/turn off saber.
		if (saber_own->r.svFlags & SVF_BOT)
		{
			saber_own->client->saberKnockedTime = level.time + SABER_BOTRETRIEVE_DELAY;
		}
		else
		{
			saber_own->client->saberKnockedTime = level.time + SABER_RETRIEVE_DELAY;
		}

		//no more loop sound!
		saberEnt->s.loopSound = saber_own->client->saber[0].soundLoop;
		saberEnt->s.loopIsSoundset = qfalse;

		//don't actually bounce on impact with walls.
		saberEnt->bounceCount = 0;
	}
	else
	{
		//otherwise, just move by normal ballistic physics
		//spin just like we were in our saber throw.

		if (saber1 && saber1->type == SABER_VADER)
		{
			saberEnt->s.apos.trType = TR_LINEAR;
			saberEnt->s.apos.trDelta[0] = 0;
			saberEnt->s.apos.trDelta[1] = 600;
			saberEnt->s.apos.trDelta[2] = 0;
		}
		else
		{
			if (saber_own->client->ps.fd.saberAnimLevel == SS_DESANN || saber_own->client->ps.fd.saberAnimLevel ==
				SS_TAVION)
			{
				saberEnt->s.apos.trType = TR_LINEAR;
				saberEnt->s.apos.trDelta[0] = 0;
				saberEnt->s.apos.trDelta[1] = 600;
				saberEnt->s.apos.trDelta[2] = 0;
			}
			else if (saber_own->client->ps.fd.saberAnimLevel == SS_STAFF)
			{
				saberEnt->s.apos.trType = TR_LINEAR;
				saberEnt->s.apos.trDelta[0] = 0;
				saberEnt->s.apos.trDelta[1] = 1200;
				saberEnt->s.apos.trDelta[2] = 0;
			}
			else if (saber_own->client->ps.fd.saberAnimLevel == SS_FAST)
			{
				saberEnt->s.apos.trType = TR_LINEAR;
				saberEnt->s.apos.trDelta[0] = 1200;
				saberEnt->s.apos.trDelta[1] = 0;
				saberEnt->s.apos.trDelta[2] = 0;
			}
			else
			{
				saberEnt->s.apos.trType = TR_LINEAR;
				saberEnt->s.apos.trDelta[0] = 600;
				saberEnt->s.apos.trDelta[1] = 0;
				saberEnt->s.apos.trDelta[2] = 0;
			}
		}

		//but now gravity has an effect.
		saberEnt->s.pos.trType = TR_GRAVITY;

		//clear the entity flags
		saberEnt->s.eFlags = 0;

		saberEnt->bounceCount = BALLISTICSABER_BOUNCECOUNT;
	}

	//set up for saber style bouncing
	saberEnt->flags |= FL_BOUNCE_HALF;

	//set transect timers and initial positions
	saberEnt->s.apos.trTime = level.time;
	VectorCopy(saberEnt->r.currentAngles, saberEnt->s.apos.trBase);

	saberEnt->s.pos.trTime = level.time;
	VectorCopy(saberEnt->r.currentOrigin, saberEnt->s.pos.trBase);

	//let the player know that they've lost control of the saber.
	saber_own->client->ps.saberEntityNum = 0;

	//set the appropriate function pointer stuff
	saberEnt->think = SaberBallisticsThink;
	saberEnt->touch = SaberBallisticsTouch;
	saberEnt->nextthink = level.time + FRAMETIME;

	trap->LinkEntity((sharedEntity_t*)saberEnt);

	//add the saber model to our gentity ghoul2 instance
	WP_SaberAddG2Model(saberEnt, saber_own->client->saber[0].model,
		saber_own->client->saber[0].skin);

	saberEnt->s.modelGhoul2 = 1;
	saberEnt->s.g2radius = 20;

	saberEnt->s.eType = ET_MISSILE;
	saberEnt->s.weapon = WP_SABER;
}

void DebounceSaberImpact(const gentity_t* self, const gentity_t* other_saberer, const int rsaber_num,
	const int rblade_num, const int sabimpactentity_num)
{
	//this function adds the necessary denounces for saber impacts so that we can do
	//consistant damage to players

	//add basic saber impact debounce
	self->client->sabimpact[rsaber_num][rblade_num].entityNum = sabimpactentity_num;
	self->client->sabimpact[rsaber_num][rblade_num].Debounce = level.time;

	if (other_saberer)
	{
		//we hit an enemy saber so update our sabimpactdebounce data with that info for us and the enemy.
		self->client->sabimpact[rsaber_num][rblade_num].saberNum = self->client->lastSaberCollided;
		self->client->sabimpact[rsaber_num][rblade_num].blade_num = self->client->lastBladeCollided;

		//Also add this impact to the otherowner so he doesn't do do his behavior rolls twice.
		other_saberer->client->sabimpact[self->client->lastSaberCollided][self->client->lastBladeCollided].entityNum =
			self->client->ps.saberEntityNum;
		other_saberer->client->sabimpact[self->client->lastSaberCollided][self->client->lastBladeCollided].Debounce =
			level.time;
		other_saberer->client->sabimpact[self->client->lastSaberCollided][self->client->lastBladeCollided].saberNum =
			rsaber_num;
		other_saberer->client->sabimpact[self->client->lastSaberCollided][self->client->lastBladeCollided].blade_num =
			rblade_num;
	}
	else
	{
		//blank out the saber blade impact stuff since we didn't hit another guy's saber
		self->client->sabimpact[rsaber_num][rblade_num].saberNum = -1;
		self->client->sabimpact[rsaber_num][rblade_num].blade_num = -1;
	}
}

qboolean WP_SaberIsOff(const gentity_t* self, const int saberNum)
{
	//this function checks to see if a given saber is off.
	switch (self->client->ps.saberHolstered)
	{
	case 0:
		//all sabers on
		return qfalse;

	case 1:
		//one saber off, one saber on, depends on situation.
	{
		//one saber off, one saber on, depends on situation.
		if (self->client->ps.fd.saberAnimLevel == SS_DUAL && self->client->ps.saberInFlight && !self->client->ps.
			saberEntityNum)
		{
			//special case where the secondary blade is lit instead of the primary
			if (saberNum == 0)
			{
				//primary is off
				return qtrue;
			}
			//secondary is on
			return qfalse;
		}
		//the normal case
		if (saberNum == 0)
		{
			//primary is on
			return qfalse;
		}
		//secondary is off
		return qtrue;
	}
	case 2:
		//all sabers off
		return qtrue;
	default:
		return qtrue;
	}
}

qboolean WP_BladeIsOff(const gentity_t* self, const int saberNum, const int blade_num)
{
	//checks to see if a given saber blade is supposed to be off.  This function does not check to see if the
	//saber or saber blade actually exists.

	//We have this function to account for the special cases with dual sabers where one saber has been dropped.

	if (saberNum > 0)
	{
		//secondary sabers are all on/all off.
		if (WP_SaberIsOff(self, saberNum))
		{
			//blades are all off
			return qtrue;
		}
		//blades are all on
		return qfalse;
	}
	//primary blade
	//based on number of blades on saber
	if (WP_SaberIsOff(self, saberNum)) //This function accounts for the weird saber throw situations.
	{
		//saber is off, all blades are off
		return qtrue;
	}
	//saber is on, secondary blades status is based on saberHolstered value.
	if (blade_num > 0 && self->client->ps.saberHolstered == 1)
	{
		//secondaries are off
		return qtrue;
	}
	//blade is on.
	return qfalse;
}

qboolean wp_using_dual_saber_as_primary(const playerState_t* ps)
{
	//indicates that the player is in the very special case of using their dual saber
	//as their primary saber when their primary saber is dropped.
	if (ps->fd.saberAnimLevel == SS_DUAL
		&& ps->saberInFlight
		&& ps->saberHolstered)
	{
		return qtrue;
	}

	return qfalse;
}

qboolean WP_DoingForcedAnimationForForcePowers(const gentity_t* self)
{
	if (!self->client)
	{
		return qfalse;
	}
	if (self->client->ps.legsAnim == BOTH_FORCE_ABSORB_START ||
		self->client->ps.legsAnim == BOTH_FORCE_ABSORB_END ||
		self->client->ps.legsAnim == BOTH_FORCE_ABSORB ||
		self->client->ps.torsoAnim == BOTH_FORCE_RAGE ||
		self->client->ps.legsAnim == BOTH_FORCE_PROTECT)
	{
		return qtrue;
	}
	return qfalse;
}

void player_Burn(const gentity_t* self);
void player_StopBurn(const gentity_t* self);

void Player_CheckBurn(const gentity_t* self)
{
	if (self->health <= 0 || self->painDebounceTime < level.time)
	{
		player_StopBurn(self);
	}
	else if (PM_InRoll(&self->client->ps))
	{
		player_StopBurn(self);
	}
	else if (TIMER_Done(self, "BurnDebounce"))
	{
		player_StopBurn(self);
	}
	else if (self->client->ps.saberEventFlags & SEF_INWATER)
	{
		player_StopBurn(self);
	}
	else
	{
		player_StopBurn(self);
	}
}

void player_Burn(const gentity_t* self)
{
	if ((self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self)) && self && self->client)
	{
		if (!(self->client->ps.PlayerEffectFlags & 1 << PEF_BURNING))
		{
			self->client->ps.PlayerEffectFlags |= 1 << PEF_BURNING;
		}
	}

	TIMER_Set(self, "BurnDebounce", 1000);
}

void player_StopBurn(const gentity_t* self)
{
	if (self && self->client)
	{
		if (self->client->ps.PlayerEffectFlags & 1 << PEF_BURNING)
		{
			//Unburn
			self->client->ps.PlayerEffectFlags &= ~(1 << PEF_BURNING);
		}
		else
		{
			//Unburn
			self->client->ps.PlayerEffectFlags &= ~(1 << PEF_BURNING);
		}
	}
}

void player_Freeze(const gentity_t* self);

static void player_StopFreeze(const gentity_t* self)
{
	if (self && self->client)
	{
		if (self->client->ps.PlayerEffectFlags & 1 << PEF_FREEZING)
		{
			//UnFreeze
			self->client->ps.PlayerEffectFlags &= ~(1 << PEF_FREEZING);
		}
		else
		{
			//UnFreeze
			self->client->ps.PlayerEffectFlags &= ~(1 << PEF_FREEZING);
		}
	}
}

void Player_CheckFreeze(const gentity_t* self)
{
	if (self && self->client)
	{
		if (self->health <= 0 || self->painDebounceTime < level.time)
		{
			player_StopFreeze(self);
		}
		else if (PM_InRoll(&self->client->ps))
		{
			player_StopFreeze(self);
		}
		else if (TIMER_Done(self, "FreezeDebounce"))
		{
			player_StopFreeze(self);
		}
		else
		{
			player_StopFreeze(self);
		}
	}
}

void player_Freeze(const gentity_t* self)
{
	if ((self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self)) && self && self->client)
	{
		if (!(self->client->ps.PlayerEffectFlags & 1 << PEF_FREEZING))
		{
			self->client->ps.PlayerEffectFlags |= 1 << PEF_FREEZING;
		}
		else
		{
			player_StopFreeze(self);
		}
	}

	TIMER_Set(self, "FreezeDebounce", 1000);
}

void WP_BlockPointsDrain(const gentity_t* self, const int fatigue)
{
	if (self->client->ps.fd.blockPoints > fatigue)
	{
		self->client->ps.fd.blockPoints -= fatigue;
	}
	else
	{
		self->client->ps.fd.blockPoints = 0;
	}

	if (self->client->ps.fd.blockPoints < 0)
	{
		self->client->ps.fd.blockPoints = 0;
	}

	if (self->client->ps.fd.blockPoints <= BLOCKPOINTS_HALF)
	{
		//Pop the Fatigued flag
		self->client->ps.userInt3 |= 1 << FLAG_BLOCKDRAINED;
	}
}

void G_Beskar_Attack_Bounce(const gentity_t* self, gentity_t* other)
{
	if (self->client->ps.saberBlocked == BLOCKED_NONE)
	{
		if (!pm_saber_in_special_attack(self->client->ps.torsoAnim))
		{
			if (SaberAttacking(self))
			{
				// Saber is in attack, use bounce for this attack.
				self->client->ps.saberBounceMove = PM_SaberBounceForAttack(self->client->ps.saber_move);
				self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
			}
			else
			{
				// Saber is in defense, use defensive bounce.
				self->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			}
		}
	}
}