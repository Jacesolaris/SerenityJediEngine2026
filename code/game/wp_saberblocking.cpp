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
#include "b_local.h"
#include "wp_saber.h"
#include "g_vehicles.h"
#include "../qcommon/tri_coll_test.h"
#include "../cgame/cg_local.h"
#include <qcommon\q_math.h>
#include <qcommon\q_shared.h>
#include "bg_public.h"

//////////Defines////////////////
extern qboolean PM_KickingAnim(int anim);
extern qboolean BG_SaberInNonIdleDamageMove(const playerState_t* ps);
extern qboolean in_front(vec3_t spot, vec3_t from, vec3_t from_angles, float thresh_hold = 0.0f);
extern qboolean PM_SaberInKnockaway(int move);
extern qboolean PM_SaberInBounce(int move);
extern qboolean BG_InSlowBounce(const playerState_t* ps);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
extern qboolean PM_InKnockDown(const playerState_t* ps);
extern qboolean PM_SaberInTransitionAny(int move);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
extern qboolean npc_is_dark_jedi(const gentity_t* self);
extern saber_moveName_t pm_broken_parry_for_attack(int move);
extern qboolean PM_InGetUp(const playerState_t* ps);
extern qboolean PM_InForceGetUp(const playerState_t* ps);
extern qboolean PM_SaberInParry(int move);
extern saber_moveName_t PM_KnockawayForParry(int move);
extern int G_KnockawayForParry(int move);
extern qboolean WP_SaberLose(gentity_t* self, vec3_t throw_dir);
extern cvar_t* g_saberAutoBlocking;
extern qboolean WP_SabersCheckLock(gentity_t* ent1, gentity_t* ent2);
extern qboolean walk_check(const gentity_t* self);
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength, const qboolean breakSaberLock);
extern qboolean PM_SuperBreakWinAnim(int anim);
extern void PM_AddFatigue(playerState_t* ps, int fatigue);
extern qboolean WP_SaberBlockNonRandom(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
extern saber_moveName_t PM_BrokenParryForParry(int move);
extern void PM_AddBlockFatigue(playerState_t* ps, int fatigue);
extern void G_Stagger(gentity_t* hit_ent);
extern void g_fatigue_bp_knockaway(gentity_t* blocker);
extern void G_StaggerAttacker(gentity_t* atk);
extern void G_BounceAttacker(gentity_t* atk);
extern void wp_block_points_regenerate(const gentity_t* self, int override_amt);
extern saber_moveName_t PM_SaberBounceForAttack(int move);
extern void WP_SaberDrop(const gentity_t* self, gentity_t* saber);
extern qboolean pm_saber_innonblockable_attack(int anim);
extern qboolean pm_saber_in_special_attack(int anim);
extern qboolean PM_SaberInKata(saber_moveName_t saber_move);
extern void wp_saber_clear_damage_for_ent_num(gentity_t* attacker, int entityNum, int saberNum, int blade_num);
extern cvar_t* d_slowmoaction;
extern void G_StartStasisEffect(const gentity_t* ent, int me_flags = 0, int length = 1000, float time_scale = 0.0f,
	int spin_time = 0);
extern void CGCam_BlockShakeSP(float intensity, int duration);
extern int G_GetParryForBlock(int block);
extern qboolean WP_SaberDisarmed(gentity_t* self, vec3_t throw_dir);
extern void wp_block_points_regenerate_over_ride(const gentity_t* self, int override_amt);
// Saber Blocks
extern qboolean WP_SaberMBlock(gentity_t* victim, gentity_t* attacker, int saberNum, int blade_num);
extern qboolean WP_SaberParry(gentity_t* blocker, gentity_t* attacker, int saberNum, int blade_num);
extern qboolean WP_SaberBlockedBounceBlock(gentity_t* blocker, gentity_t* attacker, int saberNum, int blade_num);
extern qboolean WP_SaberFatiguedParry(gentity_t* blocker, gentity_t* attacker, int saberNum, int blade_num);
void sab_beh_animate_heavy_slow_bounce_attacker(gentity_t* attacker);
extern cvar_t* g_DebugSaberCombat;
extern qboolean PM_InSaberLock(int anim);
extern void g_do_m_block_response(const gentity_t* speaker_npc_self);
extern qboolean Rosh_BeingHealed(const gentity_t* self);
extern qboolean g_in_cinematic_saber_anim(const gentity_t* self);
extern qboolean PM_SuperBreakLoseAnim(int anim);
///////////Defines////////////////

//////////Actions////////////////

static void sab_beh_saber_should_be_disarmed_attacker(gentity_t* attacker, const int saberNum)
{
	if (saberNum == 0)
	{
		//can only lose right-hand saber for now
		if (!(attacker->client->ps.saber[saberNum].saberFlags & SFL_NOT_DISARMABLE))
		{
			//knocked the saber right out of his hand!
			vec3_t throw_dir = { 0, 0, 350 };

			G_Stagger(attacker);

			WP_SaberDisarmed(attacker, throw_dir);
		}
	}
}

void sab_beh_saber_should_be_disarmed_blocker(gentity_t* blocker, const int saberNum)
{
	if (saberNum == 0)
	{
		//can only lose right-hand saber for now
		if (!(blocker->client->ps.saber[saberNum].saberFlags & SFL_NOT_DISARMABLE))
		{
			//knocked the saber right out of his hand!
			vec3_t throw_dir = { 0, 0, 350 };

			G_Stagger(blocker);

			WP_SaberDisarmed(blocker, throw_dir);
		}
	}
}

qboolean g_accurate_blocking(const gentity_t* blocker, const gentity_t* attacker, vec3_t hit_loc)
{
	//determines if self (who is blocking) is actively blocking (parrying)
	vec3_t p_angles;
	vec3_t p_right;
	vec3_t parrier_move{};
	vec3_t hit_pos;
	vec3_t hit_flat{}; //flatten 2D version of the hitPos.
	const qboolean in_front_of_me = in_front(attacker->client->ps.origin, blocker->client->ps.origin, blocker->client->ps.viewangles, 0.0f);

	if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
	{
		if (!(blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
		{
			return qfalse;
		}
	}

	if (!in_front_of_me)
	{
		//can't parry attacks to the rear.
		return qfalse;
	}
	if (PM_SaberInKnockaway(blocker->client->ps.saber_move))
	{
		//already in parry move, continue parrying anything that hits us as long as
		//the attacker is in the same general area that we're facing.
		return qtrue;
	}

	if (PM_KickingAnim(blocker->client->ps.legsAnim))
	{
		//can't parry in kick.
		return qfalse;
	}

	if (BG_SaberInNonIdleDamageMove(&blocker->client->ps)
		|| PM_SaberInBounce(blocker->client->ps.saber_move) || BG_InSlowBounce(&blocker->client->ps))
	{
		//can't parry if we're transitioning into a block from an attack state.
		return qfalse;
	}

	if (blocker->client->ps.pm_flags & PMF_DUCKED)
	{
		//can't parry while ducked or running
		return qfalse;
	}

	if (PM_InKnockDown(&blocker->client->ps))
	{
		//can't block while knocked down or getting up from knockdown, or we are staggered.
		return qfalse;
	}

	if (blocker->client->ps.ManualblockStartTime >= 3000) //3 sec
	{
		//cant perfect parry if your too slow
		return qfalse;
	}

	//set up flatten version of the location of the incoming attack in orientation
	//to the player.
	VectorSubtract(hit_loc, blocker->client->ps.origin, hit_pos);
	VectorSet(p_angles, 0, blocker->client->ps.viewangles[YAW], 0);
	AngleVectors(p_angles, nullptr, p_right, nullptr);
	hit_flat[0] = 0;
	hit_flat[1] = DotProduct(p_right, hit_pos);

	//just bump the hit pos down for the blocking since the average left/right slice happens at about origin +10
	hit_flat[2] = hit_pos[2] - 10;
	VectorNormalize(hit_flat);

	//set up the vector for the direction the player is trying to parry in.
	parrier_move[0] = 0;
	parrier_move[1] = blocker->client->pers.cmd.rightmove;
	parrier_move[2] = -blocker->client->pers.cmd.forwardmove;
	VectorNormalize(parrier_move);

	const float block_dot = DotProduct(hit_flat, parrier_move);

	if (block_dot >= 0.4f)
	{
		//player successfully blocked in the right direction to do a full parry.
		return qtrue;
	}
	//player didn't parry in the correct direction, do blockPoints punishment
	if (blocker->NPC && !G_ControlledByPlayer(blocker))
	{
		//bots just randomly parry to make up for them not intelligently parrying.
		if (NPC_PARRYRATE * g_spskill->integer > Q_irand(0, 999))
		{
			return qtrue;
		}
	}
	return qfalse;
}

static void sab_beh_add_mishap_attacker(gentity_t* attacker, const gentity_t* blocker, const int saberNum)
{
	if (attacker->client->ps.blockPoints <= MISHAPLEVEL_NONE)
	{
		attacker->client->ps.blockPoints = MISHAPLEVEL_NONE;
	}
	else if (attacker->client->ps.saberFatigueChainCount <= MISHAPLEVEL_NONE)
	{
		attacker->client->ps.saberFatigueChainCount = MISHAPLEVEL_NONE;
	}
	else
	{
		//overflowing causes a full mishap.
		const int rand_num = Q_irand(0, 2);

		switch (rand_num)
		{
		case 0:
			if (blocker->NPC && !G_ControlledByPlayer(blocker)) //NPC only
			{
				if (!Q_irand(0, 4))
				{
					//20% chance
					sab_beh_animate_heavy_slow_bounce_attacker(attacker);
					if (d_attackinfo->integer || g_DebugSaberCombat->integer && (attacker->NPC && !G_ControlledByPlayer(attacker)))
					{
						gi.Printf(S_COLOR_YELLOW"NPC Attacker staggering\n");
					}
				}
				else
				{
					sab_beh_saber_should_be_disarmed_attacker(attacker, saberNum);
					if (d_attackinfo->integer || g_DebugSaberCombat->integer && (attacker->NPC && !G_ControlledByPlayer(attacker)))
					{
						gi.Printf(S_COLOR_RED"NPC Attacker lost his saber\n");
					}
				}
			}
			else
			{
				sab_beh_saber_should_be_disarmed_attacker(attacker, saberNum);
				if (d_attackinfo->integer || g_DebugSaberCombat->integer && (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker)))
				{
					gi.Printf(S_COLOR_RED"Player Attacker lost his saber\n");
				}
			}
			break;
		case 1:
			sab_beh_animate_heavy_slow_bounce_attacker(attacker);
			if (d_attackinfo->integer || g_DebugSaberCombat->integer && (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker)))
			{
				gi.Printf(S_COLOR_RED"Player Attacker staggering\n");
			}
			break;
		default:;
		}
	}
}

static void sab_beh_add_mishap_Fake_attacker(gentity_t* attacker, const gentity_t* blocker, const int saberNum)
{
	if (attacker->client->ps.blockPoints <= MISHAPLEVEL_NONE)
	{
		attacker->client->ps.blockPoints = MISHAPLEVEL_NONE;
	}
	else if (attacker->client->ps.saberFatigueChainCount <= MISHAPLEVEL_NONE)
	{
		attacker->client->ps.saberFatigueChainCount = MISHAPLEVEL_NONE;
	}
	else
	{
		//overflowing causes a full mishap.
		const int rand_num = Q_irand(0, 2);

		switch (rand_num)
		{
		case 0:
			if (blocker->NPC && !G_ControlledByPlayer(blocker)) //NPC only
			{
				if (!Q_irand(0, 4))
				{
					//20% chance
					sab_beh_saber_should_be_disarmed_attacker(attacker, saberNum);
					if (d_attackinfo->integer || g_DebugSaberCombat->integer && (attacker->NPC && !G_ControlledByPlayer(attacker)))
					{
						gi.Printf(S_COLOR_RED"NPC Attacker lost his saber\n");
					}
				}
				else
				{
					sab_beh_animate_heavy_slow_bounce_attacker(attacker);
					if (d_attackinfo->integer || g_DebugSaberCombat->integer && (attacker->NPC && !G_ControlledByPlayer(attacker)))
					{
						gi.Printf(S_COLOR_YELLOW"NPC Attacker staggering\n");
					}
				}
			}
			else
			{
				sab_beh_saber_should_be_disarmed_attacker(attacker, saberNum);
				if (d_attackinfo->integer || g_DebugSaberCombat->integer && (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker)))
				{
					gi.Printf(S_COLOR_RED"Player Attacker lost his saber\n");
				}
			}
			break;
		case 1:
			sab_beh_animate_heavy_slow_bounce_attacker(attacker);
			if (d_attackinfo->integer || g_DebugSaberCombat->integer && (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker)))
			{
				gi.Printf(S_COLOR_RED"Player Attacker staggering\n");
			}
			break;
		default:;
		}
	}
}

static void sab_beh_add_mishap_blocker(gentity_t* blocker, const int saberNum)
{
	if (blocker->client->ps.blockPoints <= MISHAPLEVEL_NONE)
	{
		blocker->client->ps.blockPoints = MISHAPLEVEL_NONE;
	}
	else if (blocker->client->ps.saberFatigueChainCount <= MISHAPLEVEL_NONE)
	{
		blocker->client->ps.saberFatigueChainCount = MISHAPLEVEL_NONE;
	}
	else
	{
		//overflowing causes a full mishap.
		const int rand_num = Q_irand(0, 2);

		switch (rand_num)
		{
		case 0:
			G_Stagger(blocker);
			if (d_blockinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_RED"blocker staggering\n");
			}
			break;
		case 1:
			if (blocker->NPC && !G_ControlledByPlayer(blocker)) //NPC only
			{
				if (!Q_irand(0, 4))
				{
					//20% chance
					G_Stagger(blocker);
					if (d_blockinfo->integer || g_DebugSaberCombat->integer)
					{
						gi.Printf(S_COLOR_RED"NPC blocker staggering\n");
					}
				}
				else
				{
					sab_beh_saber_should_be_disarmed_blocker(blocker, saberNum);
					wp_block_points_regenerate_over_ride(blocker, BLOCKPOINTS_FATIGUE);
					if (d_blockinfo->integer || g_DebugSaberCombat->integer)
					{
						gi.Printf(S_COLOR_RED"NPC blocker lost his saber\n");
					}
				}
			}
			else
			{
				sab_beh_saber_should_be_disarmed_blocker(blocker, saberNum);
				if (d_blockinfo->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf(S_COLOR_RED"blocker lost his saber\n");
				}
			}
			break;
		default:;
		}
	}
}

////////Attacker Bounces//////////

void sab_beh_animate_heavy_slow_bounce_attacker(gentity_t* attacker)
{
	attacker->client->ps.userInt3 |= 1 << FLAG_SLOWBOUNCE;
	attacker->client->ps.userInt3 |= 1 << FLAG_OLDSLOWBOUNCE;
	G_StaggerAttacker(attacker);
}

static void sab_beh_animate_small_bounce(gentity_t* attacker)
{
	if (attacker->NPC && !G_ControlledByPlayer(attacker)) //NPC only
	{
		attacker->client->ps.userInt3 |= 1 << FLAG_SLOWBOUNCE;
		attacker->client->ps.userInt3 |= 1 << FLAG_OLDSLOWBOUNCE;
		G_BounceAttacker(attacker);
	}
	else
	{
		attacker->client->ps.userInt3 |= 1 << FLAG_SLOWBOUNCE;
		attacker->client->ps.saberBounceMove = LS_D1_BR + (saber_moveData[attacker->client->ps.saber_move].startQuad - Q_BR);
		attacker->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
	}
}

////////Blocker Bounces//////////

void sab_beh_animate_slow_bounce_blocker(gentity_t* blocker)
{
	blocker->client->ps.userInt3 |= 1 << FLAG_SLOWBOUNCE;
	blocker->client->ps.userInt3 |= 1 << FLAG_OLDSLOWBOUNCE;

	G_AddEvent(blocker, Q_irand(EV_PUSHED1, EV_PUSHED3), 0);

	blocker->client->ps.saberBounceMove = PM_BrokenParryForParry(G_GetParryForBlock(blocker->client->ps.saberBlocked));
	blocker->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
}

////////Bounces//////////

static qboolean sab_beh_attack_blocked(gentity_t* attacker, gentity_t* blocker, const int saberNum, const qboolean force_mishap)
{
	//if the attack is blocked -(Im the attacker)
	const qboolean m_blocking = blocker->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING ? qtrue : qfalse;
	//perfect Blocking (Timed Block)

	if (attacker->client->ps.saberFatigueChainCount >= MISHAPLEVEL_MAX)
	{
		//hard mishap.

		if (attacker->NPC && !G_ControlledByPlayer(attacker)) //NPC only
		{
			if (!Q_irand(0, 4))
			{
				//20% chance
				sab_beh_add_mishap_attacker(attacker, blocker, saberNum);
			}
			else
			{
				sab_beh_animate_heavy_slow_bounce_attacker(attacker);
			}
			if (d_attackinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_GREEN"Attacker npc is fatigued\n");
			}

			attacker->client->ps.saberFatigueChainCount = MISHAPLEVEL_MIN;
		}
		else
		{
			if (d_attackinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_GREEN"Attacker player is fatigued\n");
			}
			sab_beh_add_mishap_attacker(attacker, blocker, saberNum);
		}
		return qtrue;
	}
	if (attacker->client->ps.saberFatigueChainCount >= MISHAPLEVEL_HUDFLASH)
	{
		//slow bounce
		if (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker))
		{
			sab_beh_animate_heavy_slow_bounce_attacker(attacker);
		}
		else
		{
			sab_beh_animate_small_bounce(attacker);
		}

		if (attacker->NPC && !G_ControlledByPlayer(attacker)) //NPC only
		{
			attacker->client->ps.saberFatigueChainCount = MISHAPLEVEL_LIGHT;
		}

		if (d_attackinfo->integer || g_DebugSaberCombat->integer)
		{
			if (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker))
			{
				gi.Printf(S_COLOR_GREEN"player attack stagger\n");
			}
			else
			{
				gi.Printf(S_COLOR_GREEN"npc attack stagger\n");
			}
		}
		return qtrue;
	}
	if (attacker->client->ps.saberFatigueChainCount >= MISHAPLEVEL_LIGHT)
	{
		//slow bounce
		sab_beh_animate_small_bounce(attacker);

		if (d_attackinfo->integer || g_DebugSaberCombat->integer)
		{
			if (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker))
			{
				gi.Printf(S_COLOR_GREEN"player light blocked bounce\n");
			}
			else
			{
				gi.Printf(S_COLOR_GREEN"npc light blocked bounce\n");
			}
		}
		return qtrue;
	}
	if (force_mishap)
	{
		//two attacking sabers bouncing off each other
		sab_beh_animate_small_bounce(attacker);
		sab_beh_animate_small_bounce(blocker);

		if (d_attackinfo->integer || g_DebugSaberCombat->integer)
		{
			if (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker))
			{
				gi.Printf(S_COLOR_GREEN"player two attacking sabers bouncing off each other\n");
			}
			else
			{
				gi.Printf(S_COLOR_GREEN"npc two attacking sabers bouncing off each other\n");
			}
		}
		return qtrue;
	}
	if (!m_blocking)
	{
		if (d_attackinfo->integer || g_DebugSaberCombat->integer)
		{
			if (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker))
			{
				gi.Printf(S_COLOR_GREEN"player blocked bounce\n");
			}
			else
			{
				gi.Printf(S_COLOR_GREEN"npc blocked bounce\n");
			}
		}
		sab_beh_animate_small_bounce(attacker);
	}
	return qtrue;
}

static void sab_beh_add_balance(const gentity_t* self, int amount)
{
	if (!walk_check(self))
	{
		//running or moving very fast, can't balance as well
		if (amount > 0)
		{
			amount *= 2;
		}
		else
		{
			amount = amount * .5f;
		}
	}

	self->client->ps.saberFatigueChainCount += amount;

	if (self->client->ps.saberFatigueChainCount < MISHAPLEVEL_NONE)
	{
		self->client->ps.saberFatigueChainCount = MISHAPLEVEL_NONE;
	}
	else if (self->client->ps.saberFatigueChainCount >= MISHAPLEVEL_OVERLOAD)
	{
		self->client->ps.saberFatigueChainCount = MISHAPLEVEL_MAX;
	}
}

//////////Actions////////////////

/////////Functions//////////////

static qboolean sab_beh_attack_vs_attack(gentity_t* attacker, gentity_t* blocker, const int saberNum)
{
	//set the saber behavior for two attacking blades hitting each other
	const qboolean atkfake = attacker->client->ps.userInt3 & 1 << FLAG_ATTACKFAKE ? qtrue : qfalse;
	const qboolean otherfake = blocker->client->ps.userInt3 & 1 << FLAG_ATTACKFAKE ? qtrue : qfalse;

	if (atkfake && !otherfake)
	{
		//self is solo faking
		//set self
		sab_beh_add_balance(attacker, MPCOST_PARRIED);
		//set otherOwner

		if (WP_SabersCheckLock(attacker, blocker))
		{
			attacker->client->ps.userInt3 |= 1 << FLAG_SABERLOCK_ATTACKER;
			attacker->client->ps.saberBlocked = BLOCKED_NONE;
			blocker->client->ps.saberBlocked = BLOCKED_NONE;
		}
		sab_beh_add_balance(blocker, -MPCOST_PARRIED);
	}
	else if (!atkfake && otherfake)
	{
		//only otherOwner is faking
		//set self
		if (WP_SabersCheckLock(blocker, attacker))
		{
			attacker->client->ps.saberBlocked = BLOCKED_NONE;
			blocker->client->ps.userInt3 |= 1 << FLAG_SABERLOCK_ATTACKER;
			blocker->client->ps.saberBlocked = BLOCKED_NONE;
		}
		sab_beh_add_balance(attacker, -MPCOST_PARRIED);
		//set otherOwner
		sab_beh_add_balance(blocker, MPCOST_PARRIED);
	}
	else if (atkfake && otherfake)
	{
		//both faking
		//set self
		if (WP_SabersCheckLock(attacker, blocker))
		{
			attacker->client->ps.userInt3 |= 1 << FLAG_SABERLOCK_ATTACKER;
			attacker->client->ps.saberBlocked = BLOCKED_NONE;

			blocker->client->ps.userInt3 |= 1 << FLAG_SABERLOCK_ATTACKER;
			blocker->client->ps.saberBlocked = BLOCKED_NONE;
		}
		sab_beh_add_balance(attacker, MPCOST_PARRIED);
		//set otherOwner
		sab_beh_add_balance(blocker, MPCOST_PARRIED);
	}
	else if (PM_SaberInKata(static_cast<saber_moveName_t>(attacker->client->ps.saber_move)))
	{
		sab_beh_add_balance(attacker, MPCOST_PARRIED);
		//set otherOwner
		sab_beh_add_balance(blocker, -MPCOST_PARRIED);

		if (blocker->client->ps.blockPoints < BLOCKPOINTS_TEN)
		{
			//Low points = bad blocks
			sab_beh_saber_should_be_disarmed_blocker(blocker, saberNum);
			wp_block_points_regenerate_over_ride(blocker, BLOCKPOINTS_FATIGUE);
		}
		else
		{
			//Low points = bad blocks
			G_Stagger(blocker);
			PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_TEN);
		}
	}
	else if (PM_SaberInKata(static_cast<saber_moveName_t>(blocker->client->ps.saber_move)))
	{
		sab_beh_add_balance(attacker, -MPCOST_PARRIED);
		//set otherOwner
		sab_beh_add_balance(blocker, MPCOST_PARRIED);

		if (attacker->client->ps.blockPoints < BLOCKPOINTS_TEN)
		{
			//Low points = bad blocks
			sab_beh_saber_should_be_disarmed_attacker(attacker, saberNum);
			wp_block_points_regenerate_over_ride(attacker, BLOCKPOINTS_FATIGUE);
		}
		else
		{
			//Low points = bad blocks
			G_Stagger(attacker);
			PM_AddBlockFatigue(&attacker->client->ps, BLOCKPOINTS_TEN);
		}
	}
	else
	{
		//either both are faking or neither is faking.  Either way, it's canceled out
		//set self
		sab_beh_add_balance(attacker, MPCOST_PARRIED);
		//set otherOwner
		sab_beh_add_balance(blocker, MPCOST_PARRIED);

		sab_beh_attack_blocked(attacker, blocker, saberNum, qtrue);

		sab_beh_attack_blocked(blocker, attacker, saberNum, qtrue);
	}
	return qtrue;
}

qboolean sab_beh_attack_vs_block(gentity_t* attacker, gentity_t* blocker, const int saberNum, const int blade_num, vec3_t hit_loc)
{
	//if the attack is blocked -(Im the attacker)
	const qboolean accurate_parry = g_accurate_blocking(blocker, attacker, hit_loc); // Perfect Normal Blocking
	const qboolean blocking = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;	//Normal Blocking (just holding block button)
	const qboolean m_blocking = blocker->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING ? qtrue : qfalse; //perfect Blocking (Timed Block)
	const qboolean is_holding_block_button_and_attack = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse; //Active Blocking (Holding Block button + Attack button)
	const qboolean npc_blocking = blocker->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING ? qtrue : qfalse; //(Npc Blocking function)

	const qboolean atkfake = attacker->client->ps.userInt3 & 1 << FLAG_ATTACKFAKE ? qtrue : qfalse;

	if (pm_saber_innonblockable_attack(attacker->client->ps.torsoAnim))
	{
		//perfect Blocking
		if (m_blocking) // A perfectly timed block
		{
			sab_beh_saber_should_be_disarmed_attacker(attacker, saberNum);
			//just so attacker knows that he was blocked
			attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
			//since it was parried, take away any damage done
			wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saberNum, blade_num);
			PM_AddBlockFatigue(&attacker->client->ps, BLOCKPOINTS_TEN); //BP Punish Attacker
		}
		else
		{
			//This must be Unblockable
			if (d_attackinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_MAGENTA"Attacker must be Unblockable\n");
			}
			attacker->client->ps.saberEventFlags &= ~SEF_BLOCKED;
		}
	}
	else if (BG_SaberInNonIdleDamageMove(&blocker->client->ps))
	{
		//and blocker is attacking
		if (d_attackinfo->integer || g_DebugSaberCombat->integer)
		{
			gi.Printf(S_COLOR_YELLOW"Both Attacker and Blocker are now attacking\n");
		}

		sab_beh_attack_vs_attack(blocker, attacker, saberNum);
	}
	else if (PM_SuperBreakWinAnim(attacker->client->ps.torsoAnim))
	{
		//attacker was attempting a superbreak and he hit someone who could block the move, rail him for screwing up.
		sab_beh_add_balance(attacker, MPCOST_PARRIED);

		sab_beh_animate_heavy_slow_bounce_attacker(attacker);

		sab_beh_add_balance(blocker, -MPCOST_PARRIED);
		if (d_attackinfo->integer || g_DebugSaberCombat->integer)
		{
			gi.Printf(S_COLOR_YELLOW"Attacker Super break win / fail\n");
		}
	}
	else if (atkfake)
	{
		//attacker faked but it was blocked here
		if (m_blocking || npc_blocking)
		{
			//defender parried the attack fake.
			sab_beh_add_balance(attacker, MPCOST_PARRIED_ATTACKFAKE);

			if (npc_blocking)
			{
				attacker->client->ps.userInt3 |= 1 << FLAG_BLOCKED;
			}
			else
			{
				attacker->client->ps.userInt3 |= 1 << FLAG_PARRIED;
			}

			sab_beh_add_balance(blocker, MPCOST_PARRYING_ATTACKFAKE);
			sab_beh_add_mishap_Fake_attacker(attacker, blocker, saberNum);

			if ((d_attackinfo->integer || g_DebugSaberCombat->integer) && !PM_InSaberLock(attacker->client->ps.torsoAnim))
			{
				gi.Printf(S_COLOR_YELLOW"Attackers Attack Fake was P-Blocked\n");
			}
		}
		else
		{
			//otherwise, the defender stands a good chance of having his defensive broken.
			sab_beh_add_balance(attacker, -MPCOST_PARRIED);

			if (WP_SabersCheckLock(attacker, blocker))
			{
				attacker->client->ps.userInt3 |= 1 << FLAG_SABERLOCK_ATTACKER;
				attacker->client->ps.saberBlocked = BLOCKED_NONE;
				blocker->client->ps.saberBlocked = BLOCKED_NONE;
			}

			if (d_attackinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_YELLOW"Attacker forced a saberlock\n");
			}
		}
	}
	else
	{
		//standard attack.
		if (accurate_parry || blocking || m_blocking || is_holding_block_button_and_attack || npc_blocking) // All types of active blocking
		{
			if (m_blocking || is_holding_block_button_and_attack || npc_blocking)
			{
				if (npc_blocking && blocker->client->ps.blockPoints >= BLOCKPOINTS_MISSILE
					&& attacker->client->ps.saberFatigueChainCount >= MISHAPLEVEL_HUDFLASH
					&& !Q_irand(0, 4))
				{
					//20% chance
					sab_beh_animate_heavy_slow_bounce_attacker(attacker);
					attacker->client->ps.userInt3 |= 1 << FLAG_MBLOCKBOUNCE;
				}
				else
				{
					attacker->client->ps.userInt3 |= 1 << FLAG_BLOCKED;
				}

				if (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker))
				{
					CGCam_BlockShakeSP(0.45f, 100);
				}
			}
			else
			{
				attacker->client->ps.userInt3 |= 1 << FLAG_PARRIED;
			}

			if (!m_blocking)
			{
				sab_beh_attack_blocked(attacker, blocker, saberNum, qfalse);
			}

			sab_beh_add_balance(blocker, -MPCOST_PARRIED);

			if (d_attackinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_YELLOW"Attackers Attack was Blocked\n");
			}
		}
		else
		{
			//Backup in case i missed some

			if (!m_blocking)
			{
				if (pm_saber_innonblockable_attack(blocker->client->ps.torsoAnim))
				{
					sab_beh_animate_heavy_slow_bounce_attacker(attacker);

					sab_beh_add_balance(blocker, -MPCOST_PARRIED);
					if (d_attackinfo->integer || g_DebugSaberCombat->integer)
					{
						gi.Printf(S_COLOR_YELLOW"Attack an Unblockable attack\n");
					}
				}
				else
				{
					sab_beh_attack_blocked(attacker, blocker, saberNum, qtrue);

					G_Stagger(blocker);

					if (d_attackinfo->integer || g_DebugSaberCombat->integer)
					{
						gi.Printf(S_COLOR_ORANGE"Attacker All the rest of the types of contact\n");
					}
				}
			}
		}
	}
	return qtrue;
}

// Small helpers to remove repeated code
static inline void SB_ParrySuccess(gentity_t* blocker, gentity_t* attacker,
	int saberNum, int blade_num)
{
	blocker->client->ps.saberEventFlags |= SEF_PARRIED;
	attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
	wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saberNum, blade_num);
}

static inline void SB_DebugPrint(const char* msg, gentity_t* blocker)
{
	if ((d_blockinfo->integer || g_DebugSaberCombat->integer) &&
		(blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker)))
	{
		gi.Printf("%s%s\n", S_COLOR_CYAN, msg);
	}
}

qboolean sab_beh_block_vs_attack(gentity_t* blocker, gentity_t* attacker, const int saberNum, const int blade_num, vec3_t hit_loc)
{
	// Early exits
	if (!blocker || !blocker->client || !attacker) {
		return qfalse;
	}

	if (Rosh_BeingHealed(blocker)) {
		return qfalse;
	}

	if (g_in_cinematic_saber_anim(blocker)) {
		return qfalse;
	}

	if (PM_SuperBreakLoseAnim(blocker->client->ps.torsoAnim) ||
		PM_SuperBreakWinAnim(blocker->client->ps.torsoAnim)) {
		return qfalse;
	}

	// Precompute flags (Raven-style qboolean)
	const qboolean accurate_parry =
		(g_accurate_blocking(blocker, attacker, hit_loc)) ? qtrue : qfalse;

	const qboolean blocking =
		(blocker->client->ps.ManualBlockingFlags & (1 << HOLDINGBLOCK)) ? qtrue : qfalse;

	const qboolean m_blocking =
		(blocker->client->ps.ManualBlockingFlags & (1 << PERFECTBLOCKING)) ? qtrue : qfalse;

	const qboolean active_block =
		(blocker->client->ps.ManualBlockingFlags & (1 << HOLDINGBLOCKANDATTACK)) ? qtrue : qfalse;

	const qboolean npc_blocking =
		(blocker->client->ps.ManualBlockingFlags & (1 << MBF_NPCBLOCKING)) ? qtrue : qfalse;

	const qboolean unblockable =
		(pm_saber_innonblockable_attack(attacker->client->ps.torsoAnim)) ? qtrue : qfalse;

	int bp = blocker->client->ps.blockPoints;

	// ============================================================
	// UNBLOCKABLE ATTACK BRANCH
	// ============================================================
	if (unblockable) {
		if (m_blocking) {
			// Perfect block vs unblockable
			if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker)) {
				if (d_slowmoaction->integer) {
					G_StartStasisEffect(blocker, MEF_NO_SPIN, 200, 0.3f, 0);
				}
				CGCam_BlockShakeSP(0.45f, 100);
			}

			blocker->client->ps.userInt3 |= (1 << FLAG_PERFECTBLOCK);

			G_Sound(blocker, G_SoundIndex(va("sound/weapons/saber/saber_perfectblock%d.mp3",
				Q_irand(1, 3))));

			if ((d_blockinfo->integer || g_DebugSaberCombat->integer) &&
				(blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))) {
				gi.Printf(S_COLOR_MAGENTA"Blocker Perfect blocked an Unblockable attack reward 20\n");
			}

			blocker->client->ps.saberEventFlags |= SEF_PARRIED;

			wp_block_points_regenerate_over_ride(blocker, BLOCKPOINTS_FATIGUE);
			blocker->client->ps.saberFatigueChainCount = MISHAPLEVEL_NONE;

			return qtrue;
		}

		// Failed vs unblockable
		if (bp < BLOCKPOINTS_TEN) {
			sab_beh_saber_should_be_disarmed_blocker(blocker, saberNum);
			wp_block_points_regenerate_over_ride(blocker, BLOCKPOINTS_FATIGUE);
		}
		else {
			g_fatigue_bp_knockaway(blocker);
			PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_TEN);
		}

		if (d_blockinfo->integer || g_DebugSaberCombat->integer) {
			gi.Printf(S_COLOR_MAGENTA"Blocker can not block Unblockable\n");
		}

		blocker->client->ps.saberEventFlags &= ~SEF_PARRIED;
		return qtrue;
	}

	// ============================================================
	// VERY LOW BP BRANCH (≤ FATIGUE)
	// ============================================================
	if (bp <= BLOCKPOINTS_FATIGUE) {
		if (bp <= BLOCKPOINTS_TEN) {
			// Disarm / mishap
			if (blocker->NPC && !G_ControlledByPlayer(blocker)) {
				sab_beh_add_mishap_blocker(blocker, saberNum);
			}
			else {
				sab_beh_saber_should_be_disarmed_blocker(blocker, saberNum);
			}

			// Regenerate BP
			if (attacker->NPC && !G_ControlledByPlayer(attacker)) {
				wp_block_points_regenerate(attacker, BLOCKPOINTS_FATIGUE);
			}
			else if (!blocker->client->ps.saberInFlight) {
				wp_block_points_regenerate(blocker, BLOCKPOINTS_FATIGUE);
			}

			if ((d_blockinfo->integer || g_DebugSaberCombat->integer) &&
				(blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))) {
				gi.Printf(S_COLOR_CYAN"Blocker was disarmed with very low bp, recharge bp 20bp\n");
			}

			// Parry success
			blocker->client->ps.saberEventFlags |= SEF_PARRIED;
			attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
			wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saberNum, blade_num);
		}
		else {
			// Stagger / knockaway
			g_fatigue_bp_knockaway(blocker);
			PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_DANGER);

			if ((d_blockinfo->integer || g_DebugSaberCombat->integer) &&
				(blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))) {
				gi.Printf(S_COLOR_CYAN"Blocker stagger drain 4 bp\n");
			}

			// Parry success
			blocker->client->ps.saberEventFlags |= SEF_PARRIED;
			attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
			wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saberNum, blade_num);
		}

		return qtrue;
	}

	// ============================================================
	// ACTIVE BLOCKING (block + attack)
	// ============================================================
	if (active_block) {
		if (m_blocking) {
			// Perfect block
			WP_SaberMBlock(blocker, attacker, saberNum, blade_num);

			if (attacker->client->ps.saberFatigueChainCount >= MISHAPLEVEL_THIRTEEN) {
				sab_beh_add_mishap_attacker(attacker, blocker, saberNum);
			}
			else {
				sab_beh_animate_heavy_slow_bounce_attacker(attacker);
				attacker->client->ps.userInt3 |= (1 << FLAG_MBLOCKBOUNCE);
			}

			blocker->client->ps.userInt3 |= (1 << FLAG_PERFECTBLOCK);

			if (attacker->NPC && !G_ControlledByPlayer(attacker)) {
				g_do_m_block_response(attacker);
			}

			if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker)) {
				if (d_slowmoaction->integer) {
					G_StartStasisEffect(blocker, MEF_NO_SPIN, 200, 0.3f, 0);
				}
				CGCam_BlockShakeSP(0.45f, 100);
			}

			G_Sound(blocker, G_SoundIndex(va("sound/weapons/saber/saber_perfectblock%d.mp3",
				Q_irand(1, 3))));

			if ((d_blockinfo->integer || g_DebugSaberCombat->integer) &&
				(blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))) {
				gi.Printf(S_COLOR_CYAN"Blocker Perfect blocked reward 20\n");
			}

			// Parry success
			blocker->client->ps.saberEventFlags |= SEF_PARRIED;
			attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
			wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saberNum, blade_num);

			wp_block_points_regenerate_over_ride(blocker, BLOCKPOINTS_FATIGUE);
			blocker->client->ps.saberFatigueChainCount = MISHAPLEVEL_NONE;
			PM_AddBlockFatigue(&attacker->client->ps, BLOCKPOINTS_TEN);

			return qtrue;
		}

		// Spam block + attack
		if (bp <= BLOCKPOINTS_HALF ||
			attacker->client->ps.saberAnimLevel == SS_DESANN ||
			attacker->client->ps.saberAnimLevel == SS_STRONG) {
			WP_SaberFatiguedParry(blocker, attacker, saberNum, blade_num);
		}
		else {
			WP_SaberParry(blocker, attacker, saberNum, blade_num);
		}

		if (attacker->NPC && !G_ControlledByPlayer(attacker)) {
			PM_AddBlockFatigue(&attacker->client->ps, BLOCKPOINTS_THREE);
		}

		PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_FIVE);

		if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker)) {
			CGCam_BlockShakeSP(0.45f, 100);
		}

		if ((d_blockinfo->integer || g_DebugSaberCombat->integer) &&
			(blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))) {
			gi.Printf(S_COLOR_CYAN"Blocker Spamming block + attack cost 5\n");
		}

		// Parry success
		blocker->client->ps.saberEventFlags |= SEF_PARRIED;
		attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
		wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saberNum, blade_num);

		return qtrue;
	}

	// ============================================================
	// BLOCK ONLY (holding block, not attacking)
	// ============================================================
	if (blocking) {
		if (bp <= BLOCKPOINTS_HALF ||
			attacker->client->ps.saberAnimLevel == SS_DESANN ||
			attacker->client->ps.saberAnimLevel == SS_STRONG) {
			WP_SaberFatiguedParry(blocker, attacker, saberNum, blade_num);
		}
		else {
			WP_SaberBlockedBounceBlock(blocker, attacker, saberNum, blade_num);
		}

		if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker)) {
			CGCam_BlockShakeSP(0.45f, 100);
		}

		if (!(blocker->NPC && !G_ControlledByPlayer(blocker))) {
			PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_TEN);
		}

		if ((d_blockinfo->integer || g_DebugSaberCombat->integer) &&
			(blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))) {
			gi.Printf(S_COLOR_CYAN"Blocker Holding block button only (spamming block) cost 10\n");
		}

		// Parry success
		blocker->client->ps.saberEventFlags |= SEF_PARRIED;
		attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
		wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saberNum, blade_num);

		return qtrue;
	}

	// ============================================================
	// ACCURATE PARRY OR NPC AUTO-BLOCK
	// ============================================================
	if (accurate_parry || npc_blocking) {
		if (attacker->client->ps.saberAnimLevel == SS_DESANN ||
			attacker->client->ps.saberAnimLevel == SS_STRONG) {
			WP_SaberFatiguedParry(blocker, attacker, saberNum, blade_num);
		}
		else if (bp <= BLOCKPOINTS_MISSILE) {
			if (bp <= BLOCKPOINTS_FOURTY) {
				WP_SaberFatiguedParry(blocker, attacker, saberNum, blade_num);

				if ((d_blockinfo->integer || g_DebugSaberCombat->integer) &&
					(blocker->NPC && !G_ControlledByPlayer(blocker))) {
					gi.Printf(S_COLOR_CYAN"NPC Fatigued Parry\n");
				}

				PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_FAIL);
			}
			else {
				WP_SaberParry(blocker, attacker, saberNum, blade_num);

				if ((d_blockinfo->integer || g_DebugSaberCombat->integer) &&
					(blocker->NPC && !G_ControlledByPlayer(blocker))) {
					gi.Printf(S_COLOR_CYAN"NPC normal Parry\n");
				}

				PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_THREE);
			}
		}
		else {
			WP_SaberMBlock(blocker, attacker, saberNum, blade_num);

			if (blocker->NPC && !G_ControlledByPlayer(blocker)) {
				g_do_m_block_response(blocker);
			}

			if ((d_blockinfo->integer || g_DebugSaberCombat->integer) &&
				(blocker->NPC && !G_ControlledByPlayer(blocker))) {
				gi.Printf(S_COLOR_CYAN"NPC good Parry\n");
			}

			PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_THREE);
		}

		G_Sound(blocker, G_SoundIndex(va("sound/weapons/saber/saber_goodparry%d.mp3",
			Q_irand(1, 3))));

		if ((d_blockinfo->integer || g_DebugSaberCombat->integer) &&
			(blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))) {
			gi.Printf(S_COLOR_CYAN"Blocker Other types of block and npc,s\n");
		}

		// Parry success
		blocker->client->ps.saberEventFlags |= SEF_PARRIED;
		attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
		wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saberNum, blade_num);

		return qtrue;
	}

	// ============================================================
	// NOT BLOCKING AT ALL
	// ============================================================
	sab_beh_add_mishap_blocker(blocker, saberNum);

	if (!(blocker->NPC && !G_ControlledByPlayer(blocker))) {
		PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_TEN);
	}

	if ((d_blockinfo->integer || g_DebugSaberCombat->integer) &&
		(blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))) {
		gi.Printf(S_COLOR_CYAN"Blocker Not holding block drain 10\n");
	}

	return qtrue;
}

/////////Functions//////////////

/////////////////////// 20233 new build ////////////////////////////////