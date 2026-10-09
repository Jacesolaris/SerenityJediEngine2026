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
///						      System designed by Serenity and modded by JaceSolaris. (c) 2026 SJE   		                    ///
///								    https://www.moddb.com/mods/serenityjediengine-20											///
///																																///
/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///

#include "g_local.h"
#include "b_local.h"
#include "wp_saber.h"
#include <qcommon\q_math.h>
#include <qcommon\q_shared.h>
#include "bg_public.h"
#include <qcommon/q_color.h>
#include "g_shared.h"
#include <qcommon\q_platform.h>

//////////Defines////////////////
extern qboolean PM_KickingAnim(int anim);
extern qboolean PM_SaberInNonIdleDamageMove(const playerState_t* ps);
extern qboolean InFront(vec3_t spot, vec3_t from, vec3_t from_angles, float thresh_hold = 0.0f);
extern qboolean PM_SaberInKnockaway(int move);
extern qboolean PM_SaberInBounce(const int move);
extern qboolean BG_InSlowBounce(const playerState_t* ps);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
extern qboolean PM_InKnockDown(const playerState_t* ps);
extern qboolean PM_SaberInTransitionAny(int move);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
extern qboolean npc_is_dark_jedi(const gentity_t* self);
extern saberMoveName_t pm_broken_parry_for_attack(int move);
extern qboolean PM_InGetUp(const playerState_t* ps);
extern qboolean PM_InForceGetUp(const playerState_t* ps);
extern qboolean PM_SaberInParry(int move);
extern saberMoveName_t PM_KnockawayForParry(int move);
extern int G_KnockawayForParry(int move);
extern qboolean WP_SaberLose(gentity_t* self, vec3_t throw_dir);
extern cvar_t* g_saberAutoBlocking;
extern qboolean WP_SabersCheckLock(gentity_t* ent1, gentity_t* ent2);
extern qboolean WalkCheck(const gentity_t* self);
extern qboolean PM_SuperBreakWinAnim(int anim);
extern void PM_AddFatigue(playerState_t* ps, int fatigue);
extern qboolean WP_SaberBlockNonRandom(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
extern saberMoveName_t PM_BrokenParryForParry(int move);
extern void PM_AddBlockFatigue(playerState_t* ps, int fatigue);
extern void G_Stagger(gentity_t* hitEnt);
extern void g_fatigue_bp_knockaway(gentity_t* blocker);
extern void G_StaggerAttacker(gentity_t* atk);
extern void G_BounceAttacker(gentity_t* atk);
extern void WP_BlockPointsRegenerate(const gentity_t* self, const int override_amt);
extern saberMoveName_t PM_SaberBounceForAttack(int move);
extern qboolean PM_SaberInnonblockableAttack(int anim);
extern qboolean PM_SaberInSpecialAttack(int anim);
extern qboolean PM_SaberInKata(saberMoveName_t saberMove);
extern void WP_SaberClearDamageForEntNum(gentity_t* attacker, const int entityNum, const int saberNum, const int bladeNum);
extern cvar_t* d_slowmoaction;
extern void G_StartStasisEffect(const gentity_t* ent, int me_flags = 0, int length = 1000, float time_scale = 0.0f,
	int spin_time = 0);
extern void CGCam_BlockShakeSP(float intensity, int duration);
extern int G_GetParryForBlock(int block);
extern qboolean WP_SaberDisarmed(gentity_t* self, vec3_t throw_dir);
extern void WP_BlockPointsRegenerate_over_ride(const gentity_t* self, const int override_amt);
// Saber Blocks
extern qboolean WP_SaberMBlock(gentity_t* victim, gentity_t* attacker, int saberNum, int bladeNum);
extern qboolean WP_SaberParry(gentity_t* blocker, gentity_t* attacker, int saberNum, int bladeNum);
extern qboolean WP_SaberBlockedBounceBlock(gentity_t* blocker, gentity_t* attacker, int saberNum, int bladeNum);
extern qboolean WP_SaberFatiguedParry(gentity_t* blocker, gentity_t* attacker, int saberNum, int bladeNum);
void sab_beh_animate_heavy_slow_bounce_attacker(gentity_t* attacker);
extern cvar_t* g_DebugSaberCombat;
extern qboolean PM_InSaberLock(int anim);
extern void g_do_m_block_response(const gentity_t* speaker_npc_self);
extern qboolean Rosh_BeingHealed(const gentity_t* self);
extern qboolean G_InCinematicSaberAnim(const gentity_t* self);
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

void SabBeh_SaberShouldBeDisarmedBlocker(gentity_t* blocker, const int saberNum)
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
	vec3_t p_angles;
	vec3_t p_right;
	vec3_t parrier_move = { 0 };
	vec3_t hit_pos;
	vec3_t hit_flat = { 0 };

	// MP uses 0.0f tolerance
	const qboolean in_front_of_me =
		InFront(attacker->client->ps.origin,
			blocker->client->ps.origin,
			blocker->client->ps.viewangles,
			0.0f);

	// ------------------------------------------------------------
	// Manual block requirement (players only)
	// ------------------------------------------------------------
	if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
	{
		if (!((blocker->client->ps.ManualBlockingFlags & (1 << MBF_HOLDINGBLOCK)) != 0))
			return qfalse;
	}

	// Cannot block from behind
	if (!in_front_of_me)
		return qfalse;

	// Already in knockaway → allow continued parry
	if (PM_SaberInKnockaway(blocker->client->ps.saberMove))
		return qtrue;

	// Cannot parry while kicking
	if (PM_KickingAnim(blocker->client->ps.legsAnim))
		return qfalse;

	// Cannot parry while transitioning or bouncing
	if (PM_SaberInNonIdleDamageMove(&blocker->client->ps)
		|| PM_SaberInBounce(blocker->client->ps.saberMove)
		|| BG_InSlowBounce(&blocker->client->ps))
		return qfalse;

	// Cannot parry while ducked
	if (blocker->client->ps.pm_flags & PMF_DUCKED)
		return qfalse;

	// Cannot parry while knocked down
	if (PM_InKnockDown(&blocker->client->ps))
		return qfalse;

	// Held block too long → too slow to parry.
	// ManualblockStartTime is the time the block started (0 = not holding, e.g. NPCs), not a duration:
	// comparing it with 3000 made every accurate block fail once a map had run 3 seconds.
	if (blocker->client->ps.ManualblockStartTime > 0
		&& level.time - blocker->client->ps.ManualblockStartTime >= 3000)
		return qfalse;

	// ------------------------------------------------------------
	// Directional parry correctness
	// ------------------------------------------------------------

	// Vector from blocker to hit location
	VectorSubtract(hit_loc, blocker->client->ps.origin, hit_pos);

	// Blocker's right vector (yaw only)
	VectorSet(p_angles, 0, blocker->client->ps.viewangles[YAW], 0);
	AngleVectors(p_angles, NULL, p_right, NULL);

	// Flatten hit into blocker's local plane
	hit_flat[0] = 0;
	hit_flat[1] = DotProduct(p_right, hit_pos);

	// MP uses fixed -10 vertical offset
	hit_flat[2] = hit_pos[2] - 10;

	VectorNormalize(hit_flat);

	// Player's intended parry direction
	parrier_move[0] = 0;
	parrier_move[1] = blocker->client->pers.cmd.rightmove;
	parrier_move[2] = -blocker->client->pers.cmd.forwardmove;

	VectorNormalize(parrier_move);

	// ------------------------------------------------------------
	// Style-based threshold
	// ------------------------------------------------------------
	float threshold = 0.40f; // default

	switch (blocker->client->ps.saberAnimLevel)
	{
	case SS_FAST:
		threshold = 0.55f;
		break;

	case SS_STRONG:
		threshold = 0.35f;
		break;

	default:
		threshold = 0.40f;
		break;
	}

	const float block_dot = DotProduct(hit_flat, parrier_move);

	if (block_dot >= threshold)
		return qtrue;

	// ------------------------------------------------------------
	// NPC fallback (MP logic)
	// ------------------------------------------------------------
	if (blocker->NPC && !G_ControlledByPlayer(blocker))
	{
		if (NPC_PARRYRATE * g_spskill->integer > Q_irand(0, 999))
			return qtrue;
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
				// 75% chance to disarm, 25% chance to stagger
				const int roll = Q_irand(0, 3);  // values: 0,1,2,3

				if (roll == 0)
				{
					// 25% chance
					G_Stagger(blocker);

					if (d_blockinfo->integer || g_DebugSaberCombat->integer)
					{
						gi.Printf(S_COLOR_YELLOW "NPC blocker staggering\n");
					}
				}
				else
				{
					// 75% chance
					SabBeh_SaberShouldBeDisarmedBlocker(blocker, saberNum);
					WP_BlockPointsRegenerate_over_ride(blocker, BLOCKPOINTS_FATIGUE);

					if (d_blockinfo->integer || g_DebugSaberCombat->integer)
					{
						gi.Printf(S_COLOR_RED "NPC blocker lost his saber\n");
					}
				}
			}
			else
			{
				SabBeh_SaberShouldBeDisarmedBlocker(blocker, saberNum);
				if (d_blockinfo->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf(S_COLOR_RED"Player blocker lost his saber\n");
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
		attacker->client->ps.saberBounceMove = LS_D1_BR + (saberMoveData[attacker->client->ps.saberMove].startQuad - Q_BR);
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
	const qboolean m_blocking = blocker->client->ps.ManualBlockingFlags & 1 << MBF_PERFECTBLOCKING ? qtrue : qfalse;
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
	if (!WalkCheck(self))
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
	else if (PM_SaberInKata(static_cast<saberMoveName_t>(attacker->client->ps.saberMove)))
	{
		sab_beh_add_balance(attacker, MPCOST_PARRIED);
		//set otherOwner
		sab_beh_add_balance(blocker, -MPCOST_PARRIED);

		if (blocker->client->ps.blockPoints < BLOCKPOINTS_TEN)
		{
			//Low points = bad blocks
			SabBeh_SaberShouldBeDisarmedBlocker(blocker, saberNum);
			WP_BlockPointsRegenerate_over_ride(blocker, BLOCKPOINTS_FATIGUE);
		}
		else
		{
			//Low points = bad blocks
			G_Stagger(blocker);
			PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_TEN);
		}
	}
	else if (PM_SaberInKata(static_cast<saberMoveName_t>(blocker->client->ps.saberMove)))
	{
		sab_beh_add_balance(attacker, -MPCOST_PARRIED);
		//set otherOwner
		sab_beh_add_balance(blocker, MPCOST_PARRIED);

		if (attacker->client->ps.blockPoints < BLOCKPOINTS_TEN)
		{
			//Low points = bad blocks
			sab_beh_saber_should_be_disarmed_attacker(attacker, saberNum);
			WP_BlockPointsRegenerate_over_ride(attacker, BLOCKPOINTS_FATIGUE);
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

qboolean sab_beh_attack_vs_block(gentity_t* attacker, gentity_t* blocker, const int saberNum, const int bladeNum, vec3_t hit_loc)
{
	//if the attack is blocked -(Im the attacker)
	const qboolean accurate_parry = g_accurate_blocking(blocker, attacker, hit_loc); // Perfect Normal Blocking
	const qboolean blocking = ((blocker->client->ps.ManualBlockingFlags & 1 << MBF_HOLDINGBLOCK) != 0) ? qtrue : qfalse;	//Normal Blocking (just holding block button)
	const qboolean m_blocking = ((blocker->client->ps.ManualBlockingFlags & 1 << MBF_PERFECTBLOCKING) != 0) ? qtrue : qfalse; //perfect Blocking (Timed Block)
	const qboolean is_holding_block_button_and_attack = ((blocker->client->ps.ManualBlockingFlags & 1 << MBF_HOLDINGBLOCKANDATTACK) != 0) ? qtrue : qfalse; //Active Blocking (Holding Block button + Attack button)
	const qboolean npc_blocking = ((blocker->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING) != 0) ? qtrue : qfalse; //(Npc Blocking function)

	const qboolean atkfake = ((attacker->client->ps.userInt3 & 1 << FLAG_ATTACKFAKE) != 0) ? qtrue : qfalse;

	if (PM_SaberInnonblockableAttack(attacker->client->ps.torsoAnim))
	{
		//perfect Blocking
		if (m_blocking) // A perfectly timed block
		{
			sab_beh_saber_should_be_disarmed_attacker(attacker, saberNum);
			//just so attacker knows that he was blocked
			attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
			//since it was parried, take away any damage done
			WP_SaberClearDamageForEntNum(attacker, blocker->s.number, saberNum, bladeNum);
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
	else if (PM_SaberInNonIdleDamageMove(&blocker->client->ps))
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
				if (PM_SaberInnonblockableAttack(blocker->client->ps.torsoAnim))
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
	int saberNum, int bladeNum)
{
	blocker->client->ps.saberEventFlags |= SEF_PARRIED;
	attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
	WP_SaberClearDamageForEntNum(attacker, blocker->s.number, saberNum, bladeNum);
}

static inline void SB_DebugPrint(const char* msg, gentity_t* blocker)
{
	if ((d_blockinfo->integer || g_DebugSaberCombat->integer) &&
		(blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker)))
	{
		gi.Printf("%s%s\n", S_COLOR_CYAN, msg);
	}
}

void WP_SaberWearOnBlock(gentity_t* blocker, const gentity_t* attacker);

qboolean sab_beh_block_vs_attack(gentity_t* blocker, gentity_t* attacker, const int saberNum, const int bladeNum, vec3_t hit_loc)
{
	// Early exits
	if (!blocker || !blocker->client || !attacker) {
		return qfalse;
	}

	if (Rosh_BeingHealed(blocker)) {
		return qfalse;
	}

	if (G_InCinematicSaberAnim(blocker)) {
		return qfalse;
	}

	if (PM_SuperBreakLoseAnim(blocker->client->ps.torsoAnim) ||
		PM_SuperBreakWinAnim(blocker->client->ps.torsoAnim)) {
		return qfalse;
	}
	WP_SaberWearOnBlock(blocker, attacker); // a breakable staff wears down
	//-(Im the blocker)
	const qboolean accurate_parry = g_accurate_blocking(blocker, attacker, hit_loc); // Perfect Normal Blocking
	const qboolean blocking = ((blocker->client->ps.ManualBlockingFlags & 1 << MBF_HOLDINGBLOCK) != 0) ? qtrue : qfalse;	//Normal Blocking
	const qboolean m_blocking = ((blocker->client->ps.ManualBlockingFlags & 1 << MBF_PERFECTBLOCKING) != 0) ? qtrue : qfalse;	//perfect Blocking
	const qboolean is_holding_block_button_and_attack = ((blocker->client->ps.ManualBlockingFlags & 1 << MBF_HOLDINGBLOCKANDATTACK) != 0) ? qtrue : qfalse;	//Active Blocking
	const qboolean npc_blocking = ((blocker->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING) != 0) ? qtrue : qfalse;	//Active NPC Blocking

	// ============================================================
	// UNBLOCKABLE ATTACK BRANCH
	// ============================================================
	if (!PM_SaberInnonblockableAttack(attacker->client->ps.torsoAnim))
	{ //if the attack is blockable, then check for accurate parry
		if (blocker->client->ps.blockPoints <= BLOCKPOINTS_FATIGUE) // blocker has less than 20BP
		{//Low points = bad blocks
			if (blocker->client->ps.blockPoints <= BLOCKPOINTS_TEN) // blocker has less than 10BP
			{//Low points = bad blocks
				//Low points = bad blocks
				if (blocker->NPC && !G_ControlledByPlayer(blocker)) //NPC only
				{
					sab_beh_add_mishap_blocker(blocker, saberNum);
				}
				else
				{
					SabBeh_SaberShouldBeDisarmedBlocker(blocker, saberNum);
				}

				// The blocker recovers a little if he still holds his saber (a disarm already gives the disarmed
				// fighter these points). This gave them to the attacker whenever the attacker was an NPC.
				if (!blocker->client->ps.saberInFlight)
				{
					WP_BlockPointsRegenerate(blocker, BLOCKPOINTS_FATIGUE);
				}

				if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && blocker->s.number < MAX_CLIENTS ||
					G_ControlledByPlayer(blocker))
				{
					gi.Printf(S_COLOR_CYAN"Blocker was disarmed with very low bp, recharge bp 20bp\n");
				}

				//just so blocker knows that he has parried the attacker
				blocker->client->ps.saberEventFlags |= SEF_PARRIED;
				//just so attacker knows that he was blocked
				attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
				WP_SaberClearDamageForEntNum(attacker, blocker->s.number, saberNum, bladeNum);
			}
			else
			{
				//Low points = bad blocks
				g_fatigue_bp_knockaway(blocker);

				PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_DANGER);

				if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker)))
				{
					gi.Printf(S_COLOR_CYAN"Blocker stagger drain 4 bp\n");
				}

				//just so blocker knows that he has parried the attacker
				blocker->client->ps.saberEventFlags |= SEF_PARRIED;
				//just so attacker knows that he was blocked
				attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
				//since it was parried, take away any damage done
				WP_SaberClearDamageForEntNum(attacker, blocker->s.number, saberNum, bladeNum);
			}
		}
		else
		{
			//just block it //jacesolaris
			if (is_holding_block_button_and_attack) //Holding Block Button + attack button
			{//Active Blocking
				//perfect Blocking
				if (m_blocking) // A perfectly timed block
				{//Parry the attack
					WP_SaberMBlock(blocker, attacker, saberNum, bladeNum);

					if (attacker->client->ps.saberFatigueChainCount >= MISHAPLEVEL_THIRTEEN)
					{
						sab_beh_add_mishap_attacker(attacker, blocker, saberNum);
					}
					else
					{
						sab_beh_animate_heavy_slow_bounce_attacker(attacker);
						attacker->client->ps.userInt3 |= 1 << FLAG_MBLOCKBOUNCE;
					}

					blocker->client->ps.userInt3 |= 1 << FLAG_PERFECTBLOCK;

					if (blocker->NPC && !G_ControlledByPlayer(blocker)) //NPC only
					{
						g_do_m_block_response(blocker);
					}

					if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
					{
						if (d_slowmoaction->integer)
						{
							G_StartStasisEffect(blocker, MEF_NO_SPIN, 200, 0.3f, 0);
						}
						CGCam_BlockShakeSP(0.45f, 100);
					}

					G_Sound(blocker, G_SoundIndex(va("sound/weapons/saber/saber_perfectblock%d.mp3", Q_irand(1, 3))));
					//G_PlayEffect("saber_goodparry", hit_loc);

					if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && blocker->s.number < MAX_CLIENTS
						|| G_ControlledByPlayer(blocker))
					{
						gi.Printf(S_COLOR_CYAN"Blocker Perfect blocked reward 15\n");
					}

					//just so blocker knows that he has parried the attacker
					blocker->client->ps.saberEventFlags |= SEF_PARRIED;
					//just so attacker knows that he was blocked
					attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
					//since it was parried, take away any damage done
					WP_SaberClearDamageForEntNum(attacker, blocker->s.number, saberNum, bladeNum);

					WP_BlockPointsRegenerate_over_ride(blocker, BLOCKPOINTS_FIFTEEN);//SAC Reward blocker
					sab_beh_add_balance(blocker, -MPCOST_MBLOCKED); //SAC Reward blocker

					if (attacker->NPC && !G_ControlledByPlayer(attacker)) //NPC only
					{
						PM_AddBlockFatigue(&attacker->client->ps, BLOCKPOINTS_FATIGUE); //BP Punish Attacker
						sab_beh_add_balance(attacker, BLOCKPOINTS_TEN); //SAC Punish attacker
					}
					else
					{
						PM_AddBlockFatigue(&attacker->client->ps, BLOCKPOINTS_TEN); //BP Punish Attacker
						sab_beh_add_balance(attacker, MPCOST_MBLOCKED); //SAC Punish attacker
					}
				}
				else
				{
					//Spamming block + attack buttons
					if (blocker->client->ps.blockPoints <= BLOCKPOINTS_HALF)
					{
						WP_SaberFatiguedParry(blocker, attacker, saberNum, bladeNum);
					}
					else if (attacker->client->ps.saberAnimLevel == SS_DESANN || attacker->client->ps.saberAnimLevel == SS_STRONG)
					{
						WP_SaberFatiguedParry(blocker, attacker, saberNum, bladeNum);
					}
					else
					{
						WP_SaberParry(blocker, attacker, saberNum, bladeNum);
					}

					if (attacker->NPC && !G_ControlledByPlayer(attacker)) //NPC only
					{
						sab_beh_add_balance(attacker, MPCOST_MBLOCKED); //SAC Punish attacker
					}

					PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_FIVE);

					if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
					{
						CGCam_BlockShakeSP(0.45f, 100);
					}

					if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && blocker->s.number < MAX_CLIENTS
						|| G_ControlledByPlayer(blocker))
					{
						gi.Printf(S_COLOR_CYAN"Blocker Spamming block + attack cost 5\n");
					}

					//just so blocker knows that he has parried the attacker
					blocker->client->ps.saberEventFlags |= SEF_PARRIED;
					//just so attacker knows that he was blocked
					attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
					//since it was parried, take away any damage done
					WP_SaberClearDamageForEntNum(attacker, blocker->s.number, saberNum, bladeNum);
				}
			}
			else if (blocking && !is_holding_block_button_and_attack) //Holding block button only (spamming block)
			{
				if (blocker->client->ps.blockPoints <= BLOCKPOINTS_HALF)
				{
					WP_SaberFatiguedParry(blocker, attacker, saberNum, bladeNum);
				}
				else if (attacker->client->ps.saberAnimLevel == SS_DESANN ||
					attacker->client->ps.saberAnimLevel == SS_STRONG)
				{
					WP_SaberFatiguedParry(blocker, attacker, saberNum, bladeNum);
				}
				else
				{
					WP_SaberBlockedBounceBlock(blocker, attacker, saberNum, bladeNum);
				}

				if (blocker->NPC && !G_ControlledByPlayer(blocker)) //NPC only
				{
					//
				}
				else
				{
					PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_FIVE);
				}

				if (attacker->NPC && !G_ControlledByPlayer(attacker)) //NPC only
				{
					if (attacker->client->ps.blockPoints <= BLOCKPOINTS_FATIGUE)
					{
						WP_BlockPointsRegenerate(attacker, BLOCKPOINTS_FATIGUE);
					}
					else
					{
						WP_BlockPointsRegenerate(attacker, BLOCKPOINTS_TEN);
					}
				}

				if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
				{
					CGCam_BlockShakeSP(0.45f, 100);
				}
				if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && blocker->s.number < MAX_CLIENTS ||
					G_ControlledByPlayer(blocker))
				{
					gi.Printf(S_COLOR_CYAN"Blocker Holding block button only (spamming block) cost 5\n");
				}

				//just so blocker knows that he has parried the attacker
				blocker->client->ps.saberEventFlags |= SEF_PARRIED;
				//just so attacker knows that he was blocked
				attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
				//since it was parried, take away any damage done
				WP_SaberClearDamageForEntNum(attacker, blocker->s.number, saberNum, bladeNum);
			}
			else if ((accurate_parry || npc_blocking)) //Other types and npc,s
			{
				if (attacker->client->ps.saberAnimLevel == SS_DESANN || attacker->client->ps.saberAnimLevel == SS_STRONG)
				{
					WP_SaberFatiguedParry(blocker, attacker, saberNum, bladeNum);
				}
				else if (blocker->client->ps.blockPoints <= BLOCKPOINTS_MISSILE)
				{
					if (blocker->client->ps.blockPoints <= BLOCKPOINTS_FOURTY)
					{
						WP_SaberFatiguedParry(blocker, attacker, saberNum, bladeNum);

						if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && (blocker->NPC && !
							G_ControlledByPlayer(blocker)))
						{
							gi.Printf(S_COLOR_CYAN"NPC Fatigued Parry\n");
						}

						PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_FAIL);
					}
					else
					{
						WP_SaberParry(blocker, attacker, saberNum, bladeNum);

						if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && (blocker->NPC && !
							G_ControlledByPlayer(blocker)))
						{
							gi.Printf(S_COLOR_CYAN"NPC normal Parry\n");
						}

						PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_THREE);
					}
				}
				else
				{ //good parry
					WP_SaberMBlock(blocker, attacker, saberNum, bladeNum);

					if (blocker->NPC && !G_ControlledByPlayer(blocker)) //NPC only
					{
						g_do_m_block_response(blocker);
						PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_TEN);
					}
					else
					{
						PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_THREE);
					}

					if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && (blocker->NPC && !
						G_ControlledByPlayer(blocker)))
					{
						gi.Printf(S_COLOR_CYAN"NPC good Parry\n");
					}
				}

				G_Sound(blocker, G_SoundIndex(va("sound/weapons/saber/saber_goodparry%d.mp3", Q_irand(1, 3))));

				//just so blocker knows that he has parried the attacker
				blocker->client->ps.saberEventFlags |= SEF_PARRIED;
				//just so attacker knows that he was blocked
				attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
				//since it was parried, take away any damage done
				WP_SaberClearDamageForEntNum(attacker, blocker->s.number, saberNum, bladeNum);
			}
			else
			{
				sab_beh_add_mishap_blocker(blocker, saberNum);

				if (blocker->NPC && !G_ControlledByPlayer(blocker)) //NPC only
				{
					//
				}
				else
				{
					PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_TEN);
				}
				if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && blocker->s.number < MAX_CLIENTS ||
					G_ControlledByPlayer(blocker))
				{
					gi.Printf(S_COLOR_CYAN"Blocker Not holding block drain 10\n");
				}
			}
		}
	}
	else
	{//if the attack is unblockable, then only perfect block can save the day
		//perfect Blocking
		if (m_blocking) // A perfectly timed block
		{
			if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
			{
				if (d_slowmoaction->integer)
				{
					G_StartStasisEffect(blocker, MEF_NO_SPIN, 200, 0.3f, 0);
				}
				CGCam_BlockShakeSP(0.45f, 100);
			}

			G_Sound(blocker, G_SoundIndex(va("sound/weapons/saber/saber_perfectblock%d.mp3", Q_irand(1, 3))));

			if (d_blockinfo->integer && blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
			{
				gi.Printf(S_COLOR_CYAN"Blocker Perfect blocked an Unblockable attack reward 15\n");
			}

			blocker->client->ps.saberEventFlags |= SEF_PARRIED;
			attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
			WP_SaberClearDamageForEntNum(attacker, blocker->s.number, saberNum, bladeNum);

			WP_BlockPointsRegenerate_over_ride(blocker, BLOCKPOINTS_FIFTEEN);
			PM_AddBlockFatigue(&attacker->client->ps, BLOCKPOINTS_TEN); //BP Punish Attacker
			sab_beh_add_balance(blocker, -MPCOST_MBLOCKED); //SAC Reward blocker
			sab_beh_add_balance(attacker, MPCOST_MBLOCKED); //SAC Punish attacker
		}
		else
		{
			//This must be Unblockable
			if (blocker->client->ps.blockPoints < BLOCKPOINTS_TEN)
			{
				//Low points = bad blocks
				SabBeh_SaberShouldBeDisarmedBlocker(blocker, saberNum);
				WP_BlockPointsRegenerate_over_ride(blocker, BLOCKPOINTS_FATIGUE);
			}
			else
			{
				g_fatigue_bp_knockaway(blocker);
				PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_TEN);
			}
			if (d_blockinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_MAGENTA"Blocker can not block Unblockable\n");
			}
			blocker->client->ps.saberEventFlags &= ~SEF_PARRIED;
		}
	}
	return qtrue;
}

/////////Functions//////////////

/////////////////////// 20233 new build ////////////////////////////////

/*
-------------------------
Breakable saber staffs (saber wear)

A saber with brokenSaber1 in its .sab (Academy_staff_1, dual_1) wears down in a fight and breaks into its
brokenSaber1 (right hand) / brokenSaber2 (left hand; "none": one half). Wear 0-100 in ps.stats[STAT_SABER_WEAR]
(the HUD tints the saber style icon by it). g_saberBreaking: 0 off, 1 NPCs only, 2 everyone. Boss NPCs never break.
Wear: a hit on the hilt 20; blocking a heavy attack (strong / desann / tavion style, a special, unblockable) 4, medium
2, fast 1; a perfect block 0. x2 with no block points, x2 the attacker's Sith sword, x1.5 the attacker's saber
damageScale > 1, x1.5 the attacker's Force Rage. Recovers 1 a second after 8 s without wear, but once under 25 left
(cracked, wear > 75) never back above 25. At 100 it breaks at the end of the swing: sparks, the pieces, a stagger -
or from a heavy hit a knockdown and the right-hand half knocked away (Force pull / pick it up for dual again). Saber
damage is halved for 1.5 s after a break.
-------------------------
*/
#define SABERWEAR_BREAK			100
#define SABERWEAR_BREAK_HEAVY	101
#define SABERWEAR_CRACKED		75
#define SABERWEAR_RECOVER_DELAY	8000

extern cvar_t* g_saberBreaking;
extern qboolean WP_SaberDoBreak(gentity_t* ent);
extern qboolean WP_SaberLose(gentity_t* self, vec3_t throw_dir);
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength, qboolean breakSaberLock);
extern void G_Stagger(gentity_t* hitEnt);
extern qboolean PM_SaberInStart(int move);
extern qboolean PM_SaberInTransition(int move);
extern qboolean PM_SaberInAttack(int move);

static qboolean WP_SaberWearBoss(const gentity_t* ent)
{
	if (ent->s.number < MAX_CLIENTS)
	{
		return qfalse;
	}
	switch (ent->client->NPC_class)
	{
	case CLASS_DESANN:
	case CLASS_TAVION:
	case CLASS_LUKE:
	case CLASS_KYLE:
	case CLASS_ALORA:
	case CLASS_VADER:
	case CLASS_YODA:
		return qtrue;
	default:
		return qfalse;
	}
}

// this one's saber can wear down and break
static qboolean WP_SaberBreakable(const gentity_t* ent)
{
	if (!ent || !ent->client || ent->health <= 0 || !g_saberBreaking || g_saberBreaking->integer <= 0)
	{
		return qfalse;
	}
	if (g_saberBreaking->integer == 1 && ent->s.number < MAX_CLIENTS)
	{
		return qfalse;
	}
	if (ent->client->ps.weapon != WP_SABER || ent->client->ps.dualSabers)
	{
		return qfalse;
	}
	if (!ent->client->ps.saber[0].brokenSaber1 || !ent->client->ps.saber[0].brokenSaber1[0]
		|| !Q_stricmp("none", ent->client->ps.saber[0].brokenSaber1))
	{
		return qfalse;
	}
	return WP_SaberWearBoss(ent) ? qfalse : qtrue;
}

static void WP_SaberWearAdd(gentity_t* victim, const gentity_t* attacker, float amount, const qboolean heavy)
{
	if (!WP_SaberBreakable(victim))
	{
		return;
	}
	playerState_t* ps = &victim->client->ps;
	if (ps->saberInFlight || ps->saberLockTime > level.time)
	{
		return;
	}
	if (attacker && attacker->client)
	{
		if (attacker->client->ps.saber[0].type == SABER_SITH_SWORD)
		{
			amount *= 2.0f;
		}
		if (attacker->client->ps.saber[0].damageScale > 1.0f)
		{
			amount *= 1.5f;
		}
		if (attacker->client->ps.forcePowersActive & 1 << FP_RAGE)
		{
			amount *= 1.5f;
		}
	}
	if (ps->blockPoints <= BLOCKPOINTS_FATIGUE)
	{
		amount *= 2.0f;
	}
	int wear = ps->stats[STAT_SABER_WEAR];
	if (wear < SABERWEAR_BREAK)
	{
		wear += static_cast<int>(ceilf(amount));
	}
	if (wear >= SABERWEAR_BREAK)
	{
		wear = heavy || wear == SABERWEAR_BREAK_HEAVY ? SABERWEAR_BREAK_HEAVY : SABERWEAR_BREAK;
	}
	ps->stats[STAT_SABER_WEAR] = wear;
	ps->stats[STAT_SABER_WEAR_TIMER] = SABERWEAR_RECOVER_DELAY;
}

// the blocker's saber takes the attack (sab_beh_block_vs_attack)
void WP_SaberWearOnBlock(gentity_t* blocker, const gentity_t* attacker)
{
	if (!attacker || !attacker->client || !WP_SaberBreakable(blocker))
	{
		return;
	}
	const playerState_t* bps = &blocker->client->ps;
	const playerState_t* aps = &attacker->client->ps;
	const qboolean unblockable = PM_SaberInnonblockableAttack(aps->torsoAnim);

	if (!unblockable
		&& bps->ManualBlockingFlags & 1 << MBF_PERFECTBLOCKING
		&& bps->ManualBlockingFlags & 1 << MBF_HOLDINGBLOCKANDATTACK
		&& bps->blockPoints > BLOCKPOINTS_FATIGUE)
	{
		return; // a perfect block: no wear
	}
	float amount;
	qboolean heavy = qfalse;
	if (unblockable || PM_SaberInSpecialAttack(aps->torsoAnim))
	{
		amount = 4.0f;
		heavy = qtrue;
	}
	else
	{
		switch (aps->saberAnimLevel)
		{
		case SS_STRONG:
		case SS_DESANN:
		case SS_TAVION:
			amount = 4.0f;
			heavy = qtrue;
			break;
		case SS_MEDIUM:
			amount = 2.0f;
			break;
		default:
			amount = 1.0f;
			break;
		}
	}
	WP_SaberWearAdd(blocker, attacker, amount, heavy);
}

// a saber hit on the hilt (G_GetHitLocFromSurfName): qtrue when it was a breakable saber's hilt
qboolean WP_SaberWearHiltHit(gentity_t* ent, const char* surfName, const saberType_t saberType)
{
	if (!surfName || (Q_stricmpn("w_", surfName, 2) && Q_stricmpn("saber", surfName, 5) && Q_stricmp("cylinder01", surfName)))
	{
		return qfalse;
	}
	if (!WP_SaberBreakable(ent))
	{
		return qfalse;
	}
	WP_SaberWearAdd(ent, nullptr, saberType == SABER_SITH_SWORD ? 40.0f : 20.0f, qtrue);
	return qtrue;
}

static void WP_SaberWearBreak(gentity_t* ent, const qboolean heavy)
{
	gclient_t* client = ent->client;
	vec3_t fwd, push;

	client->ps.stats[STAT_SABER_WEAR] = 0;
	client->ps.stats[STAT_SABER_WEAR_TIMER] = 0;

	G_PlayEffect("sparks/spark_explosion", client->renderInfo.handRPoint);
	G_Sound(ent, G_SoundIndex("sound/weapons/saber/saberoffquick.mp3"));

	if (!WP_SaberDoBreak(ent))
	{
		return;
	}
	client->ps.stats[STAT_SABER_BREAK_SAFE] = level.time + 1500;

	AngleVectors(client->ps.viewangles, fwd, nullptr, nullptr);
	fwd[2] = 0;
	VectorNormalize(fwd);
	VectorScale(fwd, -1.0f, push);
	if (client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		if (heavy)
		{
			// knocked down, the right-hand half knocked away
			G_Knockdown(ent, ent, push, 300, qtrue);
			if (client->ps.dualSabers)
			{
				WP_SaberLose(ent, nullptr);
			}
		}
		else
		{
			VectorMA(client->ps.velocity, 150.0f, push, client->ps.velocity);
			G_Stagger(ent);
		}
	}
	else
	{
		VectorMA(client->ps.velocity, heavy ? 300.0f : 150.0f, push, client->ps.velocity);
	}
	if (ent->s.number < MAX_CLIENTS)
	{
		gi.SendServerCommand(ent->s.number, "cp \"Your saber staff broke!\"");
	}
}

// every client think: recovery, the cracked sparks, the break when it is due
void WP_SaberWearThink(gentity_t* ent, const int msec)
{
	if (!ent || !ent->client)
	{
		return;
	}
	playerState_t* ps = &ent->client->ps;
	if (!WP_SaberBreakable(ent))
	{
		ps->stats[STAT_SABER_WEAR] = 0;
		ps->stats[STAT_SABER_WEAR_TIMER] = 0;
		return;
	}
	int wear = ps->stats[STAT_SABER_WEAR];
	if (wear >= SABERWEAR_BREAK)
	{
		// at the end of the swing (not while it is thrown, holstered or locked)
		if (ps->saberInFlight || !ps->SaberActive() || ps->saberLockTime > level.time
			|| PM_SaberInStart(ps->saberMove) || PM_SaberInTransition(ps->saberMove) || PM_SaberInAttack(ps->saberMove))
		{
			return;
		}
		WP_SaberWearBreak(ent, wear == SABERWEAR_BREAK_HEAVY ? qtrue : qfalse);
		return;
	}
	if (wear > SABERWEAR_CRACKED && ps->SaberActive() && Q_irand(0, 1500) < msec)
	{
		G_PlayEffect("sparks/spark_nosnd", ent->client->renderInfo.handRPoint);
	}
	if (wear > 0)
	{
		ps->stats[STAT_SABER_WEAR_TIMER] -= msec;
		if (ps->stats[STAT_SABER_WEAR_TIMER] <= 0)
		{
			const int wear_floor = wear > SABERWEAR_CRACKED ? SABERWEAR_CRACKED + 1 : 0;
			if (wear > wear_floor)
			{
				ps->stats[STAT_SABER_WEAR] = wear - 1;
			}
			ps->stats[STAT_SABER_WEAR_TIMER] = 1000;
		}
	}
}
