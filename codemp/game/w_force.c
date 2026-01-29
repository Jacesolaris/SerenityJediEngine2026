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

#include "b_local.h"
#include "w_saber.h"
#include "ai_main.h"
#include <qcommon\q_shared.h>
#include <qcommon\q_math.h>

#define METROID_JUMP 1

//NEEDED FOR MIND-TRICK on NPCS=========================================================
extern void NPC_PlayConfusionSound(gentity_t* self);
extern void NPC_Jedi_PlayConfusionSound(const gentity_t* self);
extern void NPC_UseResponse(gentity_t* self, const gentity_t* user, qboolean useWhenDone);
//NEEDED FOR MIND-TRICK on NPCS=========================================================
extern void Jedi_Decloak(gentity_t* self);
extern qboolean walk_check(const gentity_t* self);
extern float manual_forceblocking(const gentity_t* defender);
extern qboolean BG_FullBodyTauntAnim(int anim);
extern qboolean PM_SaberInBrokenParry(int move);
extern qboolean BG_InSlowBounce(const playerState_t* ps);
extern qboolean PM_FaceProtectAnim(int anim);
extern void G_KnockOver(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength, const qboolean breakSaberLock);
extern qboolean PM_InKnockDown(const playerState_t* ps);
extern qboolean PM_SaberInTransitionAny(int move);
extern qboolean PM_Saberinstab(int move);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean PM_RestAnim(int anim);
extern void BG_ReduceSaberMishapLevel(playerState_t* ps);
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength, const qboolean breakSaberLock);
extern qboolean PM_SaberInParry(int move);
extern void PM_AddFatigue(playerState_t* ps, int fatigue);
extern void Boba_FlyStart(gentity_t* self);
extern qboolean in_camera;
extern qboolean PM_RunningAnim(int anim);
extern void g_reflect_missile_auto(const gentity_t* ent, gentity_t* missile, vec3_t forward);
extern void g_reflect_missile_bot(const gentity_t* ent, gentity_t* missile, vec3_t forward);
extern void G_SoundOnEnt(gentity_t* ent, soundChannel_t channel, const char* sound_path);
extern saberInfo_t* BG_MySaber(int clientNum, int saberNum);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
extern qboolean PM_SaberCanInterruptMove(int move, int anim);
extern void Boba_FireWristMissile(gentity_t* self, int whichMissile);
extern void Boba_EndWristMissile(const gentity_t* self, int which_missile);
extern qboolean PM_RollingAnim(int anim);
extern void Jedi_PlayBlockedPushSound(const gentity_t* self);
extern bot_state_t* botstates[MAX_CLIENTS];
extern void Touch_Button(gentity_t* ent, gentity_t* other, trace_t* trace);
extern void player_Freeze(const gentity_t* self);
extern void Player_CheckFreeze(const gentity_t* self);
extern qboolean manual_saberblocking(const gentity_t* defender);
extern qboolean PM_ForceUsingSaberAnim(int anim);
extern qboolean PM_SaberInSpecial(int move);
extern qboolean BG_SabersOff(const playerState_t* ps);
extern qboolean BG_FullBodyEmoteAnim(int anim);
extern qboolean BG_FullBodyCowerAnim(int anim);
extern qboolean BG_FullBodyCowerstartAnim(int anim);
extern qboolean BG_IsAlreadyinTauntAnim(int anim);
qboolean jedi_dodge_evasion(gentity_t* self, const gentity_t* shooter, trace_t* tr, int hit_loc);
extern qboolean PM_SaberInReturn(int move);
extern qboolean PM_SuperBreakLoseAnim(int anim);
extern qboolean PM_SuperBreakWinAnim(int anim);
extern qboolean PM_InWallHoldMove(int anim);
extern int PM_InGrappleMove(int anim);
extern void WP_BlockPointsDrain(const gentity_t* self, int fatigue);
extern void PM_AddBlockFatigue(playerState_t* ps, int fatigue);
extern void G_StasisMissile(gentity_t* ent, gentity_t* missile, vec3_t forward);
extern qboolean PM_InSlopeAnim(int anim);
extern qboolean pm_saber_innonblockable_attack(int anim);
extern qboolean PM_InForceGetUp(const playerState_t* ps);
extern qboolean PM_KnockDownAnim(int anim);
extern qboolean PM_InGetUp(const playerState_t* ps);

void G_LetGoOfLedge(const gentity_t* ent)
{
	ent->client->ps.pm_flags &= ~PMF_STUCK_TO_WALL;
	ent->client->ps.legsTimer = 0;
	ent->client->ps.torsoTimer = 0;
}

int speedLoopSound = 0;
int rageLoopSound = 0;
int protectLoopSound = 0;
int absorbLoopSound = 0;
int seeLoopSound = 0;
int ysalamiriLoopSound = 0;

#define FORCE_VELOCITY_DAMAGE 0

#define FORCE_SPEED_DURATION_FORCE_LEVEL_1 1500.0f
#define FORCE_SPEED_DURATION_FORCE_LEVEL_2 2000.0f
#define FORCE_SPEED_DURATION_FORCE_LEVEL_3 10000.0f

#define FORCE_RAGE_DURATION 10000.0f

int ForceShootDrain(gentity_t* self);

gentity_t* G_PreDefSound(vec3_t org, const int pdSound)
{
	gentity_t* te = G_TempEntity(org, EV_PREDEFSOUND);
	te->s.eventParm = pdSound;
	VectorCopy(org, te->s.origin);

	return te;
}

qboolean CheckPushItem(const gentity_t* ent)
{
	if (!ent->item)
		return qfalse;

	if (ent->item->giType == IT_AMMO ||
		ent->item->giType == IT_HEALTH ||
		ent->item->giType == IT_ARMOR ||
		ent->item->giType == IT_HOLDABLE)
	{
		return qtrue; // these don't have placeholders
	}

	if (ent->item->giType == IT_WEAPON
		|| g_pushitems.integer == 2 && ent->item->giType == IT_TEAM || ent->item->giType == IT_POWERUP)
	{
		// check for if dropped item
		if (ent->r.svFlags & EF_DROPPEDWEAPON)
		{
			return qtrue; // these don't have placeholders
		}
		if (ent->flags & FL_DROPPED_ITEM)
		{
			return qtrue; // these don't have placeholders
		}
	}

	return qfalse;
}

const int forcePowerMinRank[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS] = //0 == neutral
{
	{
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
		10, //FP_HEAL,//instant
		0, //FP_LEVITATION,//hold/duration
		0, //FP_SPEED,//duration
		0, //FP_PUSH,//hold/duration
		0, //FP_PULL,//hold/duration
		10, //FP_TELEPATHY,//instant
		15, //FP_GRIP,//hold/duration
		10, //FP_LIGHTNING,//hold/duration
		15, //FP_RAGE,//duration
		15, //FP_PROTECT,//duration
		15, //FP_ABSORB,//duration
		10, //FP_TEAM_HEAL,//instant
		10, //FP_TEAM_FORCE,//instant
		10, //FP_DRAIN,//hold/duration
		5, //FP_SEE,//duration
		0, //FP_SABER_OFFENSE,
		0, //FP_SABER_DEFENSE,
		0 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		10, //FP_HEAL,//instant
		0, //FP_LEVITATION,//hold/duration
		0, //FP_SPEED,//duration
		0, //FP_PUSH,//hold/duration
		0, //FP_PULL,//hold/duration
		10, //FP_TELEPATHY,//instant
		15, //FP_GRIP,//hold/duration
		10, //FP_LIGHTNING,//hold/duration
		15, //FP_RAGE,//duration
		15, //FP_PROTECT,//duration
		15, //FP_ABSORB,//duration
		10, //FP_TEAM_HEAL,//instant
		10, //FP_TEAM_FORCE,//instant
		10, //FP_DRAIN,//hold/duration
		5, //FP_SEE,//duration
		5, //FP_SABER_OFFENSE,
		5, //FP_SABER_DEFENSE,
		5 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		10, //FP_HEAL,//instant
		0, //FP_LEVITATION,//hold/duration
		0, //FP_SPEED,//duration
		0, //FP_PUSH,//hold/duration
		0, //FP_PULL,//hold/duration
		10, //FP_TELEPATHY,//instant
		15, //FP_GRIP,//hold/duration
		10, //FP_LIGHTNING,//hold/duration
		15, //FP_RAGE,//duration
		15, //FP_PROTECT,//duration
		15, //FP_ABSORB,//duration
		10, //FP_TEAM_HEAL,//instant
		10, //FP_TEAM_FORCE,//instant
		10, //FP_DRAIN,//hold/duration
		5, //FP_SEE,//duration
		10, //FP_SABER_OFFENSE,
		10, //FP_SABER_DEFENSE,
		10 //FP_SABERTHROW,
		//NUM_FORCE_POWERS
	}
};

const int mindTrickTime[NUM_FORCE_POWER_LEVELS] =
{
	0, //none
	5000,
	10000,
	15000
};

float forcePushCone[NUM_FORCE_POWER_LEVELS] =
{
	1.0f, //none
	1.0f,
	0.8f,
	0.6f
};

float forcePullCone[NUM_FORCE_POWER_LEVELS] =
{
	1.0f, //none
	1.0f,
	1.0f,
	0.8f
};

float forceSpeedValue[NUM_FORCE_POWER_LEVELS] =
{
	1.0f, //none
	0.75f,
	0.5f,
	0.25f
};

#define SK_DP_FORFORCE		.5f	//determines the number of DP points players get for each skill point dedicated to Force Powers.

#define SK_DP_FORMERC		1/6.0f	//determines the number of DP points get for each skill point dedicated to gunner/merc skills.

void DetermineDodgeMax(const gentity_t* ent)
{
	//sets the maximum number of dodge points this player should have.  This is based on their skill point allocation.
	int i;
	int skillCount;
	float dodgeMax = 0;

	assert(ent && ent->client);

	if (ent->client->ps.isJediMaster)
	{
		//jedi masters have much more DP and don't actually have skills.
		ent->client->ps.stats[STAT_MAX_DODGE] = 100;
		return;
	}
	if (ent->s.number < MAX_CLIENTS)
	{
		//players get a initial DP bonus.
		dodgeMax = 50;
	}

	//force powers
	for (i = 0; i < NUM_FORCE_POWERS; i++)
	{
		if (ent->client->ps.fd.forcePowerLevel[i])
		{
			//has points in this skill
			for (skillCount = FORCE_LEVEL_1; skillCount <= ent->client->ps.fd.forcePowerLevel[i]; skillCount++)
			{
				dodgeMax += bgForcePowerCost[i][skillCount] * SK_DP_FORFORCE;
			}
		}
	}

	//additional skills
	for (i = 0; i < NUM_SKILLS; i++)
	{
		if (ent->client->skillLevel[i])
		{
			//has points in this skill
			for (skillCount = FORCE_LEVEL_1; skillCount <= ent->client->skillLevel[i]; skillCount++)
			{
				if (i >= SK_BLUESTYLE && i <= SK_STAFFSTYLE)
				{
					//styles count as force powers
					dodgeMax += bgForcePowerCost[i + NUM_FORCE_POWERS][skillCount] * SK_DP_FORFORCE;
				}
				else
				{
					dodgeMax += bgForcePowerCost[i + NUM_FORCE_POWERS][skillCount] * SK_DP_FORMERC;
				}
			}
		}
	}

	ent->client->ps.stats[STAT_MAX_DODGE] = (int)dodgeMax;
}

void WP_InitForcePowers(const gentity_t* ent)
{
	int i, last_fp_known = -1;
	qboolean warn_client, did_event = qfalse;

	char userinfo[MAX_INFO_STRING], forcePowers[DEFAULT_FORCEPOWERS_LEN + 1], readBuf[DEFAULT_FORCEPOWERS_LEN + 1];

	// if server has no max rank, default to max (50)
	if (g_maxForceRank.integer <= 0 || g_maxForceRank.integer >= NUM_FORCE_MASTERY_LEVELS)
	{
		// prevent user from being dumb
		trap->Cvar_Set("g_maxForceRank", va("%i", FORCE_MASTERY_JEDI_MASTER));
		trap->Cvar_Update(&g_maxForceRank);
	}

	if (!ent || !ent->client)
		return;

	ent->client->ps.fd.saberAnimLevel = ent->client->sess.saberLevel;

	if (ent->client->ps.fd.saberAnimLevel < FORCE_LEVEL_1 || ent->client->ps.fd.saberAnimLevel > FORCE_LEVEL_3)
		ent->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;

	// so that the client configstring is already modified with this when we need it
	if (!speedLoopSound)
		speedLoopSound = G_SoundIndex("sound/weapons/force/speedloop.wav");
	if (!rageLoopSound)
		rageLoopSound = G_SoundIndex("sound/weapons/force/rageloop.wav");
	if (!absorbLoopSound)
		absorbLoopSound = G_SoundIndex("sound/weapons/force/absorbloop.wav");
	if (!protectLoopSound)
		protectLoopSound = G_SoundIndex("sound/weapons/force/protectloop.wav");
	if (!seeLoopSound)
		seeLoopSound = G_SoundIndex("sound/weapons/force/seeloop.wav");
	if (!ysalamiriLoopSound)
		ysalamiriLoopSound = G_SoundIndex("sound/player/nullifyloop.wav");

	if (ent->s.eType == ET_NPC)
		return;

	for (i = 0; i < NUM_FORCE_POWERS; i++)
	{
		ent->client->ps.fd.forcePowerLevel[i] = 0;
		ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
	}

	ent->client->ps.fd.forcePowerSelected = -1;
	ent->client->ps.fd.forceSide = 0;

	// if in siege, then use the powers for this class, and skip all this nonsense.
	if (level.gametype == GT_SIEGE && ent->client->siegeClass != -1)
	{
		for (i = 0; i < NUM_FORCE_POWERS; i++)
		{
			ent->client->ps.fd.forcePowerLevel[i] = bgSiegeClasses[ent->client->siegeClass].forcePowerLevels[i];
			if (!ent->client->ps.fd.forcePowerLevel[i])
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
			else
				ent->client->ps.fd.forcePowersKnown |= 1 << i;
		}

		// bring up the class selection menu
		if (!ent->client->sess.setForce)
			trap->SendServerCommand(ent - g_entities, "scl");
		ent->client->sess.setForce = qtrue;
		DetermineDodgeMax(ent);

		return;
	}

	//rwwFIXMEFIXME: Temp
	if (ent->s.eType == ET_NPC && ent->s.number >= MAX_CLIENTS)
		Q_strncpyz(userinfo, "forcepowers\\7-1-333003000313003120", sizeof userinfo);
	else
		trap->GetUserinfo(ent->s.number, userinfo, sizeof userinfo);

	Q_strncpyz(forcePowers, Info_ValueForKey(userinfo, "forcepowers"), sizeof forcePowers);

	if (strlen(forcePowers) != DEFAULT_FORCEPOWERS_LEN)
	{
		Q_strncpyz(forcePowers, DEFAULT_FORCEPOWERS, sizeof forcePowers);
		trap->SendServerCommand(ent - g_entities,
			"print \"" S_COLOR_RED "Invalid forcepowers string, setting default\n\"");
	}

	//if it's a bot just copy the info directly from its personality
	if (ent->r.svFlags & SVF_BOT && botstates[ent->s.number])
		Q_strncpyz(forcePowers, botstates[ent->s.number]->forceinfo, sizeof forcePowers);

	if (g_forceBasedTeams.integer)
	{
		if (ent->client->sess.sessionTeam == TEAM_RED)
			warn_client = !BG_LegalizedForcePowers(forcePowers, sizeof forcePowers, g_maxForceRank.integer,
				HasSetSaberOnly(), FORCE_DARKSIDE, level.gametype,
				g_forcePowerDisable.integer);
		else if (ent->client->sess.sessionTeam == TEAM_BLUE)
			warn_client = !BG_LegalizedForcePowers(forcePowers, sizeof forcePowers, g_maxForceRank.integer,
				HasSetSaberOnly(), FORCE_LIGHTSIDE, level.gametype,
				g_forcePowerDisable.integer);
		else
			warn_client = !BG_LegalizedForcePowers(forcePowers, sizeof forcePowers, g_maxForceRank.integer,
				HasSetSaberOnly(), 0, level.gametype, g_forcePowerDisable.integer);
	}
	else
		warn_client = !BG_LegalizedForcePowers(forcePowers, sizeof forcePowers, g_maxForceRank.integer,
			HasSetSaberOnly(), 0, level.gametype, g_forcePowerDisable.integer);

	//rww - parse through the string manually and eat out all the appropriate data
	i = 0;
	int i_r = 0;
	while (forcePowers[i] && forcePowers[i] != '-')
	{
		readBuf[i_r] = forcePowers[i];
		i_r++;
		i++;
	}
	readBuf[i_r] = 0;
	//THE RANK
	ent->client->ps.fd.forceRank = atoi(readBuf);
	i++;

	i_r = 0;
	while (forcePowers[i] && forcePowers[i] != '-')
	{
		readBuf[i_r] = forcePowers[i];
		i_r++;
		i++;
	}
	readBuf[i_r] = 0;
	//THE SIDE
	ent->client->ps.fd.forceSide = atoi(readBuf);
	i++;

	if (level.gametype != GT_SIEGE && ent->r.svFlags & SVF_BOT && ent->client->ps.weapon == WP_SABER && botstates[ent
		->s.number])
	{
		// hmm..I'm going to cheat here.
		const int oldI = i;
		i_r = 0;

		while (forcePowers[i] && forcePowers[i] != '\n' && i_r < NUM_FORCE_POWERS)
		{
			if (ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
			{
				if (i_r == FP_ABSORB)
				{
					forcePowers[i] = '3';
				}
				if (botstates[ent->s.number]->settings.skill >= 4)
				{
					// cheat and give them more stuff
					if (i_r == FP_HEAL)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_PROTECT)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_TELEPATHY)
					{
						forcePowers[i] = '3';
					}
				}
			}
			else if (ent->client->ps.fd.forceSide == FORCE_DARKSIDE)
			{
				if (botstates[ent->s.number]->settings.skill >= 4)
				{
					if (i_r == FP_GRIP)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_LIGHTNING)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_RAGE)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_DRAIN)
					{
						forcePowers[i] = '3';
					}
				}
			}

			if (i_r == FP_PUSH)
			{
				forcePowers[i] = '3';
			}
			else if (i_r == FP_PULL)
			{
				forcePowers[i] = '3';
			}
			else if (i_r == FP_SABER_DEFENSE)
			{
				forcePowers[i] = '3';
			}
			else if (i_r == FP_SPEED)
			{
				forcePowers[i] = '1';
			}

			i++;
			i_r++;
		}
		i = oldI;
	}
	else if (level.gametype != GT_SIEGE && level.gametype == GT_TEAM && ent->r.svFlags & SVF_BOT && ent->client->ps.
		weapon == WP_SABER && botstates[ent->s.number])
	{
		// hmm..I'm going to cheat here.
		const int oldI = i;
		i_r = 0;

		while (forcePowers[i] && forcePowers[i] != '\n' && i_r < NUM_FORCE_POWERS)
		{
			if (ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
			{
				if (i_r == FP_ABSORB)
				{
					forcePowers[i] = '3';
				}
				if (botstates[ent->s.number]->settings.skill >= 4)
				{
					// cheat and give them more stuff
					if (i_r == FP_HEAL)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_PROTECT)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_TELEPATHY)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_TEAM_HEAL)
					{
						forcePowers[i] = '3';
					}
				}
			}
			else if (ent->client->ps.fd.forceSide == FORCE_DARKSIDE)
			{
				if (botstates[ent->s.number]->settings.skill >= 4)
				{
					if (i_r == FP_GRIP)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_LIGHTNING)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_RAGE)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_DRAIN)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_TEAM_FORCE)
					{
						forcePowers[i] = '3';
					}
				}
			}

			if (i_r == FP_PUSH)
			{
				forcePowers[i] = '3';
			}
			else if (i_r == FP_PULL)
			{
				forcePowers[i] = '3';
			}
			else if (i_r == FP_SABER_DEFENSE)
			{
				forcePowers[i] = '3';
			}
			else if (i_r == FP_SPEED)
			{
				forcePowers[i] = '1';
			}

			i++;
			i_r++;
		}
		i = oldI;
	}

	i_r = 0;
	while (forcePowers[i] && forcePowers[i] != '\n' &&
		i_r < NUM_FORCE_POWERS)
	{
		readBuf[0] = forcePowers[i];
		readBuf[1] = 0;

		ent->client->ps.fd.forcePowerLevel[i_r] = atoi(readBuf);
		if (ent->client->ps.fd.forcePowerLevel[i_r])
		{
			ent->client->ps.fd.forcePowersKnown |= 1 << i_r;
		}
		else
		{
			ent->client->ps.fd.forcePowersKnown &= ~(1 << i_r);
		}
		i++;
		i_r++;
	}
	//THE POWERS

	if (ent->s.eType != ET_NPC)
	{
		if (HasSetSaberOnly())
		{
			gentity_t* te = G_TempEntity(vec3_origin, EV_SET_FREE_SABER);
			te->r.svFlags |= SVF_BROADCAST;
			te->s.eventParm = 1;
		}
		else
		{
			gentity_t* te = G_TempEntity(vec3_origin, EV_SET_FREE_SABER);
			te->r.svFlags |= SVF_BROADCAST;
			te->s.eventParm = 0;
		}

		if (g_forcePowerDisable.integer)
		{
			gentity_t* te = G_TempEntity(vec3_origin, EV_SET_FORCE_DISABLE);
			te->r.svFlags |= SVF_BROADCAST;
			te->s.eventParm = 1;
		}
		else
		{
			gentity_t* te = G_TempEntity(vec3_origin, EV_SET_FORCE_DISABLE);
			te->r.svFlags |= SVF_BROADCAST;
			te->s.eventParm = 0;
		}
	}

	if (ent->s.eType == ET_NPC || ent->r.svFlags & SVF_BOT)
		ent->client->sess.setForce = qtrue;
	else if (level.gametype == GT_SIEGE)
	{
		if (!ent->client->sess.setForce)
		{
			ent->client->sess.setForce = qtrue;
			// bring up the class selection menu
			trap->SendServerCommand(ent - g_entities, "scl");
		}
	}
	else
	{
		if (warn_client || !ent->client->sess.setForce)
		{
			// the client's rank is too high for the server and has been autocapped, so tell them
			if (level.gametype != GT_HOLOCRON && level.gametype != GT_JEDIMASTER)
			{
				did_event = qtrue;

				if (!(ent->r.svFlags & SVF_BOT) && ent->s.eType != ET_NPC)
				{
					if (!g_teamAutoJoin.integer)
					{
						// make them a spectator so they can set their powerups up without being bothered.
						ent->client->sess.sessionTeam = TEAM_SPECTATOR;
						ent->client->sess.spectatorState = SPECTATOR_FREE;
						ent->client->sess.spectatorClient = 0;

						ent->client->pers.teamState.state = TEAM_BEGIN;
						trap->SendServerCommand(ent - g_entities, "spc"); // Fire up the profile menu
					}
				}

				// event isn't very reliable, I made it a string. This way I can send it to just one client also,
				//	as opposed to making a broadcast event.
				trap->SendServerCommand(ent->s.number,
					va("nfr %i %i %i", g_maxForceRank.integer, 1, ent->client->sess.sessionTeam));
				// arg1 is new max rank, arg2 is non-0 if force menu should be shown, arg3 is the current team
			}
			ent->client->sess.setForce = qtrue;
		}

		if (!did_event)
			trap->SendServerCommand(ent->s.number,
				va("nfr %i %i %i", g_maxForceRank.integer, 0, ent->client->sess.sessionTeam));

		// the server has one or more force powers disabled and the client is using them in his config
	}

	for (i = 0; i < NUM_FORCE_POWERS; i++)
	{
		if (ent->client->ps.fd.forcePowersKnown & 1 << i && !ent->client->ps.fd.forcePowerLevel[i])
			ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
		else if (i != FP_LEVITATION && i != FP_SABER_OFFENSE && i != FP_SABER_DEFENSE && i != FP_SABERTHROW)
			last_fp_known = i;
	}

	if (ent->client->ps.fd.forcePowersKnown & ent->client->sess.selectedFP)
		ent->client->ps.fd.forcePowerSelected = ent->client->sess.selectedFP;

	if (!(ent->client->ps.fd.forcePowersKnown & 1 << ent->client->ps.fd.forcePowerSelected))
	{
		if (last_fp_known != -1)
			ent->client->ps.fd.forcePowerSelected = last_fp_known;
		else
			ent->client->ps.fd.forcePowerSelected = 0;
	}

	for (/*i=0*/; i < NUM_FORCE_POWERS; i++)
		ent->client->ps.fd.forcePowerBaseLevel[i] = ent->client->ps.fd.forcePowerLevel[i];
	ent->client->ps.fd.forceUsingAdded = 0;
	DetermineDodgeMax(ent);
}

void WP_SpawnInitForcePowers(gentity_t* ent)
{
	ent->client->ps.saberAttackChainCount = MISHAPLEVEL_NONE;

	ent->client->ps.BlasterAttackChainCount = BLASTERMISHAPLEVEL_NONE;

	int i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & 1 << i)
		{
			WP_ForcePowerStop(ent, i);
		}

		i++;
	}

	ent->client->ps.fd.forceDeactivateAll = 0;

	ent->client->ps.fd.blockPoints = ent->client->ps.fd.blockPointsMax = BLOCK_POINTS_MAX;
	ent->client->ps.fd.forcePower = ent->client->ps.fd.forcePowerMax = FORCE_POWER_MAX;
	ent->client->ps.fd.forcePowerRegenDebounceTime = level.time;
	ent->client->ps.fd.BlockPointsRegenDebounceTime = level.time;
	ent->client->ps.fd.forceGripentity_num = ENTITYNUM_NONE;
	ent->client->ps.fd.forceMindtrickTargetIndex = 0;
	ent->client->ps.fd.forceMindtrickTargetIndex2 = 0;
	ent->client->ps.fd.forceMindtrickTargetIndex3 = 0;
	ent->client->ps.fd.forceMindtrickTargetIndex4 = 0;

	if (!ent->client->ps.fd.BlockPointRegenRate)
	{
		ent->client->ps.fd.BlockPointRegenRate = 100;
	}

	ent->client->ps.holocronBits = 0;

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		ent->client->ps.holocronsCarried[i] = 0;
		i++;
	}

	if (level.gametype == GT_HOLOCRON)
	{
		i = 0;
		while (i < NUM_FORCE_POWERS)
		{
			ent->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_0;
			i++;
		}

		if (HasSetSaberOnly())
		{
			if (ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] < FORCE_LEVEL_1)
			{
				ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_1;
			}
			if (ent->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1)
			{
				ent->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_1;
			}
		}
	}

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		ent->client->ps.fd.forcePowerDebounce[i] = 0;
		ent->client->ps.fd.forcePowerDuration[i] = 0;

		i++;
	}

	ent->client->ps.fd.forceJumpZStart = 0;
	ent->client->ps.fd.forceJumpCharge = 0;
	ent->client->ps.fd.forceJumpSound = 0;
	ent->client->ps.fd.forceGripDamageDebounceTime = 0;
	ent->client->ps.fd.forceGripBeingGripped = 0;
	ent->client->ps.fd.forceGripCripple = 0;
	ent->client->ps.fd.forceGripUseTime = 0;
	ent->client->ps.fd.forceGripSoundTime = 0;
	ent->client->ps.fd.forceGripStarted = 0;
	ent->client->ps.fd.forceHealTime = 0;
	ent->client->ps.fd.forceHealAmount = 0;
	ent->client->ps.fd.forceRageRecoveryTime = 0;
	ent->client->ps.fd.forceSpeedRecoveryTime = 0;
	ent->client->ps.fd.forceDrainEntNum = ENTITYNUM_NONE;
	ent->client->ps.fd.forceDrainTime = 0;

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersKnown & 1 << i &&
			!ent->client->ps.fd.forcePowerLevel[i])
		{
			//make sure all known powers are cleared if we have level 0 in them
			ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
		}

		i++;
	}

	if (level.gametype == GT_SIEGE &&
		ent->client->siegeClass != -1)
	{
		//Then use the powers for this class.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			ent->client->ps.fd.forcePowerLevel[i] = bgSiegeClasses[ent->client->siegeClass].forcePowerLevels[i];

			if (!ent->client->ps.fd.forcePowerLevel[i])
			{
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}
			else
			{
				ent->client->ps.fd.forcePowersKnown |= 1 << i;
			}
			i++;
		}
	}
}

static qboolean is_merc(const gentity_t* ent)
{
	if (!ent->client)
	{
		return qfalse;
	}

	if (ent->client->skillLevel[SK_JETPACK])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_PISTOL])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_BLASTER])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_THERMAL])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_ROCKET])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_BACTA])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_FLAMETHROWER])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_BOWCASTER])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_FORCEFIELD])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_CLOAK])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_SEEKER])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_SENTRY])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_DETPACK])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_REPEATER])
	{
		return qtrue;
	}
	if (ent->client->skillLevel[SK_DISRUPTOR])
	{
		return qtrue;
	}

	return qfalse;
}

extern qboolean BG_InKnockDown(int anim); //bg_pmove.c

int ForcePowerUsableOn(const gentity_t* attacker, const gentity_t* other, const forcePowers_t forcePower)
{
	if (other->client && (other->client->ps.inAirAnim || other->client->ps.groundEntityNum == ENTITYNUM_NONE))
	{
		return 1;
	}

	if (other && other->client && BG_HasYsalamiri(level.gametype, &other->client->ps))
	{
		return 0;
	}

	if (attacker && attacker->client && !BG_CanUseFPNow(level.gametype, &attacker->client->ps, level.time, forcePower))
	{
		return 0;
	}

	if (is_merc(other) && other->client->ps.fd.forcePowerLevel[FP_SEE] >= FORCE_LEVEL_1)
	{
		return 1;
	}

	//Dueling fighters cannot use force powers on others, with the exception of force push when locked with each other
	if (attacker && attacker->client && attacker->client->ps.duelInProgress)
	{
		return 0;
	}

	if (other && other->client && other->client->ps.duelInProgress)
	{
		return 0;
	}

	if (other && other->client && (other->client->ps.fd.saberAnimLevel == SS_DESANN && other->client->ps.
		saberFatigueChainCount >=
		MISHAPLEVEL_HEAVY))
	{
		return 0;
	}

	if (forcePower == FP_TELEPATHY && other->client)
	{
		switch (other->client->ps.fd.forcePowerLevel[FP_ABSORB])
		{
		case FORCE_LEVEL_5:
		case FORCE_LEVEL_4:
		case FORCE_LEVEL_3:
			return 0;
		case FORCE_LEVEL_2:
			if (!walk_check(other) && PM_RunningAnim(other->client->ps.legsAnim))
				return 0;
			break;

		case FORCE_LEVEL_1:
			if (!walk_check(other))
				return 0;
			break;
		default:;
		}
	}
	else if (forcePower == FP_GRIP && other->client)
	{
		switch (other->client->ps.fd.forcePowerLevel[FP_ABSORB])
		{
		case FORCE_LEVEL_1: //Can only block if walking
			if (!walk_check(other))
				return 1;
			break;
		case FORCE_LEVEL_2: //Can block if walking or running
			if (other->client->ps.inAirAnim || other->client->ps.groundEntityNum == ENTITYNUM_NONE)
				return 1;
			break;

		case FORCE_LEVEL_3:
		case FORCE_LEVEL_5:
		case FORCE_LEVEL_4:
			return 0;

		default:
			return 1;
		}
	}

	if (other && other->client && forcePower == FP_PUSH)
	{
		if (other->client->ps.stats[STAT_HEALTH] <= 0 || other->client->ps.eFlags & EF_DEAD)
		{
			return 0;
		}
	}
	else if (other && other->client && forcePower == FP_PULL)
	{
		if (g_AllowKnockDownPull.integer == 0)
		{
			if (BG_InKnockDown(other->client->ps.legsAnim))
			{
				return 0;
			}
		}
		if (other->client->ps.stats[STAT_HEALTH] <= 0 || other->client->ps.eFlags & EF_DEAD)
		{
			return 0;
		}
	}

	if (other && other->client && other->s.eType == ET_NPC &&
		other->s.NPC_class == CLASS_VEHICLE)
	{
		//can't use the force on vehicles.. except lightning
		if (forcePower == FP_LIGHTNING)
		{
			return 1;
		}
		return 0;
	}

	if (other && other->client && other->s.eType == ET_NPC &&
		level.gametype == GT_SIEGE)
	{
		//can't use powers at all on npc's normally in siege...
		return 0;
	}

	return 1;
}

qboolean WP_ForcePowerAvailable(const gentity_t* self, const forcePowers_t forcePower, const int overrideAmt)
{
	const int drain = overrideAmt
		? overrideAmt
		: forcePowerNeeded[self->client->ps.fd.forcePowerLevel[forcePower]][forcePower];

	if (self->client->ps.fd.forcePowersActive & 1 << forcePower)
	{
		//we're probably going to deactivate it..
		return qtrue;
	}
	if (forcePower == FP_LEVITATION)
	{
		return qtrue;
	}
	if (!drain)
	{
		return qtrue;
	}
	if ((forcePower == FP_DRAIN || forcePower == FP_LIGHTNING) &&
		self->client->ps.fd.forcePower >= 25)
	{
		//it's ok then, drain/lightning are actually duration
		return qtrue;
	}
	if (self->client->ps.fd.forcePower < drain)
	{
		return qfalse;
	}
	return qtrue;
}

static qboolean class_is_gunner(const gentity_t* self)
{
	switch (self->client->ps.weapon)
	{
	case WP_STUN_BATON:
	case WP_BRYAR_PISTOL:
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
	case WP_EMPLACED_GUN:
	case WP_TURRET:
		// Is Gunner...
		return qtrue;
	default:
		// NOT Gunner...
		break;
	}

	return qfalse;
}

qboolean WP_ForcePowerUsable(const gentity_t* self, const forcePowers_t forcePower)
{
	if (BG_HasYsalamiri(level.gametype, &self->client->ps))
	{
		return qfalse;
	}

	if (self->health <= 0 || self->client->ps.stats[STAT_HEALTH] <= 0 ||
		self->client->ps.eFlags & EF_DEAD)
	{
		return qfalse;
	}

	if (self->client->ps.pm_flags & PMF_FOLLOW)
	{
		//specs can't use powers through people
		return qfalse;
	}
	if (self->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return qfalse;
	}
	if (self->client->tempSpectate >= level.time)
	{
		return qfalse;
	}

	if (!BG_CanUseFPNow(level.gametype, &self->client->ps, level.time, forcePower))
	{
		return qfalse;
	}

	if (!(self->client->ps.fd.forcePowersKnown & 1 << forcePower))
	{
		//don't know this power
		return qfalse;
	}

	if (self->client->ps.fd.forcePowersActive & 1 << forcePower)
	{
		//already using this power
		if (forcePower != FP_LEVITATION)
		{
			return qfalse;
		}
	}

	if (forcePower == FP_LEVITATION && self->client->fjDidJump)
	{
		return qfalse;
	}

	if (!self->client->ps.fd.forcePowerLevel[forcePower])
	{
		return qfalse;
	}

	if (self->client->ps.pm_flags & PMF_STUCK_TO_WALL)
	{
		//no offensive force powers when stuck to wall
		switch (forcePower)
		{
		case FP_GRIP:
		case FP_LIGHTNING:
		case FP_DRAIN:
		case FP_SABER_OFFENSE:
		case FP_SABER_DEFENSE:
		case FP_SABERTHROW:
			return qfalse;
		default:
			break;
		}
	}

	if (!self->client->ps.saberHolstered)
	{
		if (self->client->saber[0].saberFlags & SFL_TWO_HANDED)
		{
			if (g_saberRestrictForce.integer)
			{
				switch (forcePower)
				{
				case FP_PUSH:
				case FP_PULL:
				case FP_TELEPATHY:
				case FP_GRIP:
				case FP_LIGHTNING:
				case FP_DRAIN:
					return qfalse;
				default:
					break;
				}
			}
		}

		if (self->client->saber[0].saberFlags & SFL_TWO_HANDED
			|| self->client->saber[0].model[0])
		{
			//this saber requires the use of two hands OR our other hand is using an active saber too
			if (self->client->saber[0].forceRestrictions & 1 << forcePower)
			{
				//this power is verboten when using this saber
				return qfalse;
			}
		}

		if (self->client->saber[0].model[0])
		{
			//both sabers on
			if (g_saberRestrictForce.integer)
			{
				switch (forcePower)
				{
				case FP_PUSH:
				case FP_PULL:
				case FP_TELEPATHY:
				case FP_GRIP:
				case FP_LIGHTNING:
				case FP_DRAIN:
					return qfalse;
				default:
					break;
				}
			}
			if (self->client->saber[1].forceRestrictions & 1 << forcePower)
			{
				//this power is verboten when using this saber
				return qfalse;
			}
		}
	}
	return WP_ForcePowerAvailable(self, forcePower, 0); // OVERRIDEFIXME
}

int wp_absorb_conversion(const gentity_t* attacked, const int atd_abs_level, const int at_power,
	const int at_power_level, const int at_force_spent)
{
	if (at_power != FP_DRAIN &&
		at_power != FP_GRIP &&
		at_power != FP_PUSH &&
		at_power != FP_PULL)
	{
		//Only these powers can be absorbed
		return -1;
	}

	if (!atd_abs_level)
	{
		//looks like attacker doesn't have any absorb power
		return -1;
	}

	if (!(attacked->client->ps.fd.forcePowersActive & 1 << FP_ABSORB))
	{
		//absorb is not active
		return -1;
	}

	//Subtract absorb power level from the offensive force power
	int get_level = at_power_level;
	get_level -= atd_abs_level;

	if (get_level < 0)
	{
		get_level = 0;
	}

	//let the attacker absorb an amount of force used in this attack based on his level of absorb
	int add_tot = at_force_spent / 3 * attacked->client->ps.fd.forcePowerLevel[FP_ABSORB];

	if (add_tot < 1 && at_force_spent >= 1)
	{
		add_tot = 1;
	}
	attacked->client->ps.fd.forcePower += add_tot;
	if (attacked->client->ps.fd.forcePower > attacked->client->ps.fd.forcePowerMax)
	{
		attacked->client->ps.fd.forcePower = attacked->client->ps.fd.forcePowerMax;
	}

	//play sound indicating that attack was absorbed
	if (attacked->client->forcePowerSoundDebounce < level.time)
	{
		gentity_t* ab_sound = G_PreDefSound(attacked->client->ps.origin, PDSOUND_ABSORBHIT);
		ab_sound->s.trickedentindex = attacked->s.number;

		attacked->client->forcePowerSoundDebounce = level.time + 400;
	}

	return get_level;
}

void wp_force_power_regenerate(const gentity_t* self, const int override_amt)
{
	//called on a regular interval to regenerate force power.
	if (!self->client)
	{
		return;
	}

	if (override_amt)
	{
		//custom regen amount
		self->client->ps.fd.forcePower += override_amt;
	}
	else
	{
		//otherwise, just 1
		self->client->ps.fd.forcePower++;
	}

	if (self->client->ps.fd.forcePower > self->client->ps.fd.forcePowerMax)
	{
		//cap it off at the max (default 100)
		self->client->ps.fd.forcePower = self->client->ps.fd.forcePowerMax;
	}
}

void wp_block_points_regenerate(const gentity_t* self, const int override_amt)
{
	const qboolean is_holding_block_button = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking

	if (!is_holding_block_button)
	{
		if (self->client->ps.fd.blockPoints < BLOCK_POINTS_MAX)
		{
			if (override_amt)
			{
				self->client->ps.fd.blockPoints += override_amt;
			}
			else
			{
				self->client->ps.fd.blockPoints++;
			}
			if (self->client->ps.fd.blockPoints > BLOCK_POINTS_MAX)
			{
				self->client->ps.fd.blockPoints = BLOCK_POINTS_MAX;
			}
		}
	}
}

void wp_block_points_regenerate_over_ride(const gentity_t* self, const int override_amt)
{
	if (self->client->ps.fd.blockPoints < BLOCK_POINTS_MAX)
	{
		if (override_amt)
		{
			self->client->ps.fd.blockPoints += override_amt;
		}
		else
		{
			self->client->ps.fd.blockPoints++;
		}
		if (self->client->ps.fd.blockPoints > BLOCK_POINTS_MAX)
		{
			self->client->ps.fd.blockPoints = BLOCK_POINTS_MAX;
		}
	}
}

void WP_ForcePowerStart(const gentity_t* self, const forcePowers_t forcePower, int overrideAmt)
{
	//activate the given force power
	int duration = 0;
	qboolean hearable = qfalse;
	float hearDist = 0;

	if (!WP_ForcePowerAvailable(self, forcePower, overrideAmt))
	{
		return;
	}

	//hearable and hearDist are merely for the benefit of bots, and not related to if a sound is actually played.
	//If duration is set, the force power will assume to be timer-based.
	switch ((int)forcePower)
	{
	case FP_HEAL:
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= 1 << forcePower;
		break;
	case FP_LEVITATION:
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= 1 << forcePower;
		break;
	case FP_SPEED:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[forcePower] == FORCE_LEVEL_1)
		{
			duration = FORCE_SPEED_DURATION_FORCE_LEVEL_1;
		}
		else if (self->client->ps.fd.forcePowerLevel[forcePower] == FORCE_LEVEL_2)
		{
			duration = FORCE_SPEED_DURATION_FORCE_LEVEL_2;
		}
		else if (self->client->ps.fd.forcePowerLevel[forcePower] == FORCE_LEVEL_3)
		{
			duration = FORCE_SPEED_DURATION_FORCE_LEVEL_3;
		}
		else //shouldn't get here
		{
			break;
		}

		if (overrideAmt)
		{
			duration = overrideAmt;
		}

		self->client->ps.fd.forcePowersActive |= 1 << forcePower;
		break;
	case FP_PUSH:
		hearable = qtrue;
		hearDist = 256;
		break;
	case FP_PULL:
		hearable = qtrue;
		hearDist = 256;
		break;
	case FP_TELEPATHY:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[forcePower] == FORCE_LEVEL_1)
		{
			duration = 20000;
		}
		else if (self->client->ps.fd.forcePowerLevel[forcePower] == FORCE_LEVEL_2)
		{
			duration = 25000;
		}
		else if (self->client->ps.fd.forcePowerLevel[forcePower] == FORCE_LEVEL_3)
		{
			duration = 30000;
		}
		else //shouldn't get here
		{
			break;
		}

		self->client->ps.fd.forcePowersActive |= 1 << forcePower;
		break;
	case FP_GRIP:
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= 1 << forcePower;
		self->client->ps.powerups[PW_DISINT_4] = level.time + 60000;
		break;
	case FP_LIGHTNING:
		hearable = qtrue;
		hearDist = 512;
		duration = overrideAmt;
		overrideAmt = 0;
		self->client->ps.fd.forcePowersActive |= 1 << forcePower;
		self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_LIGHTNING];
		break;
	case FP_RAGE:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[forcePower] == FORCE_LEVEL_1)
		{
			duration = 8000;
		}
		else if (self->client->ps.fd.forcePowerLevel[forcePower] == FORCE_LEVEL_2)
		{
			duration = 14000;
		}
		else if (self->client->ps.fd.forcePowerLevel[forcePower] == FORCE_LEVEL_3)
		{
			duration = 20000;
		}
		else //shouldn't get here
		{
			break;
		}

		self->client->ps.fd.forcePowersActive |= 1 << forcePower;
		break;
	case FP_PROTECT:
		hearable = qtrue;
		hearDist = 256;
		duration = 20000;
		self->client->ps.fd.forcePowersActive |= 1 << forcePower;
		break;
	case FP_ABSORB:
		hearable = qtrue;
		hearDist = 256;
		duration = 20000;
		self->client->ps.fd.forcePowersActive |= 1 << forcePower;
		break;
	case FP_TEAM_HEAL:
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= 1 << forcePower;
		break;
	case FP_TEAM_FORCE:
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= 1 << forcePower;
		break;
	case FP_DRAIN:
		hearable = qtrue;
		hearDist = 256;
		duration = overrideAmt;
		overrideAmt = 0;
		self->client->ps.fd.forcePowersActive |= 1 << forcePower;
		break;
	case FP_SEE:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[forcePower] == FORCE_LEVEL_1)
		{
			duration = 10000;
		}
		else if (self->client->ps.fd.forcePowerLevel[forcePower] == FORCE_LEVEL_2)
		{
			duration = 20000;
		}
		else if (self->client->ps.fd.forcePowerLevel[forcePower] == FORCE_LEVEL_3)
		{
			duration = 30000;
		}
		else //shouldn't get here
		{
			break;
		}

		self->client->ps.fd.forcePowersActive |= 1 << forcePower;
		break;
	case FP_SABER_OFFENSE:
		break;
	case FP_SABER_DEFENSE:
		break;
	case FP_SABERTHROW:
		break;
	default:
		break;
	}

	if (duration)
	{
		self->client->ps.fd.forcePowerDuration[forcePower] = level.time + duration;
	}
	else
	{
		self->client->ps.fd.forcePowerDuration[forcePower] = 0;
	}

	if (hearable)
	{
		self->client->ps.otherSoundLen = hearDist;
		self->client->ps.otherSoundTime = level.time + 100;
	}

	self->client->ps.fd.forcePowerDebounce[forcePower] = 0;

	if ((int)forcePower == FP_SPEED && overrideAmt)
	{
		WP_ForcePowerDrain(&self->client->ps, forcePower, overrideAmt * 0.025);
	}
	else if ((int)forcePower != FP_GRIP && (int)forcePower != FP_DRAIN)
	{
		//grip and drain drain as damage is done
		WP_ForcePowerDrain(&self->client->ps, forcePower, overrideAmt);
	}
}

void ForceHeal(gentity_t* self)
{
	if (self->health <= 0 || self->client->ps.stats[STAT_MAX_HEALTH] <= self->health)
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_HEAL))
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->painDebounceTime > level.time || self->client->ps.weaponTime && self->client->ps.weapon != WP_NONE)
	{
		//can't initiate a heal while taking pain or attacking
		return;
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}

	if (self->health >= self->client->ps.stats[STAT_MAX_HEALTH])
	{
		// Shield Heal skill. Done when player has full HP
		if (self->client->ps.stats[STAT_ARMOR] < 100)
		{
			self->client->ps.stats[STAT_ARMOR] += 25;

			if (self->client->ps.stats[STAT_ARMOR] > 100)
			{
				self->client->ps.stats[STAT_ARMOR] = 100;
			}

			WP_ForcePowerDrain(&self->client->ps, FP_HEAL, 0);

			G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/player/pickupshield.wav"));
		}
		return;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_3)
	{
		self->health += 25; //This was 50, but that angered the Balance God.

		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
		}
		WP_ForcePowerDrain(&self->client->ps, FP_HEAL, 0);
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_2)
	{
		self->health += 10;

		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
		}
		WP_ForcePowerDrain(&self->client->ps, FP_HEAL, 0);
	}
	else
	{
		self->health += 5;

		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
		}
		WP_ForcePowerDrain(&self->client->ps, FP_HEAL, 0);
	}

	if (self->client->ps.fd.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_2)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_FORCEHEAL_START, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		self->client->ps.saber_move = self->client->ps.saberBounceMove = LS_READY;
		self->client->ps.saberBlocked = BLOCKED_NONE;
	}
	else
	{
		//just a quick gesture
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_FORCE_ABSORB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		self->client->ps.saber_move = self->client->ps.saberBounceMove = LS_READY;
		self->client->ps.saberBlocked = BLOCKED_NONE;
	}

	/*if (self->client->ps.fd.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_2)
	{
		G_Sound(self, CHAN_ITEM, G_SoundIndex("sound/weapons/force/heal.wav"));
	}
	else
	{
		G_Sound(self, CHAN_ITEM, G_SoundIndex("sound/player/injecthealth.mp3"));
	}*/
	G_Sound(self, CHAN_ITEM, G_SoundIndex("sound/weapons/force/heal.wav"));

	G_PlayBoltedEffect(G_EffectIndex("force/heal2.efx"), self, "thoracic");
}

static void wp_add_to_client_bitflags(gentity_t* ent, const int entNum)
{
	if (!ent)
	{
		return;
	}

	if (entNum > 47)
	{
		ent->s.trickedentindex4 |= (1 << (entNum - 48));
	}
	else if (entNum > 31)
	{
		ent->s.trickedentindex3 |= (1 << (entNum - 32));
	}
	else if (entNum > 15)
	{
		ent->s.trickedentindex2 |= (1 << (entNum - 16));
	}
	else
	{
		ent->s.trickedentindex |= (1 << entNum);
	}
}

void ForceTeamHeal(const gentity_t* self)
{
	float radius = 256;
	int i = 0;
	int numpl = 0;
	int pl[MAX_CLIENTS];
	int healthadd;
	gentity_t* te = NULL;

	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_TEAM_HEAL))
	{
		return;
	}

	if (self->client->ps.fd.forcePowerDebounce[FP_TEAM_HEAL] >= level.time)
	{
		return;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_2)
	{
		radius *= 1.5;
	}
	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_3)
	{
		radius *= 2;
	}

	while (i < MAX_CLIENTS)
	{
		const gentity_t* ent = &g_entities[i];

		if (ent && ent->client && self != ent && OnSameTeam(self, ent) && ent->client->ps.stats[STAT_HEALTH] < ent->
			client->ps.stats[STAT_MAX_HEALTH] && ent->client->ps.stats[STAT_HEALTH] > 0 && ForcePowerUsableOn(
				self, ent, FP_TEAM_HEAL) &&
			trap->InPVS(self->client->ps.origin, ent->client->ps.origin))
		{
			vec3_t a;
			VectorSubtract(self->client->ps.origin, ent->client->ps.origin, a);

			if (VectorLength(a) <= radius)
			{
				pl[numpl] = i;
				numpl++;
			}
		}

		i++;
	}

	if (numpl < 1)
	{
		return;
	}

	if (numpl == 1)
	{
		healthadd = 50;
	}
	else if (numpl == 2)
	{
		healthadd = 33;
	}
	else
	{
		healthadd = 25;
	}

	self->client->ps.fd.forcePowerDebounce[FP_TEAM_HEAL] = level.time + 2000;
	i = 0;

	while (i < numpl)
	{
		if (g_entities[pl[i]].client->ps.stats[STAT_HEALTH] > 0 &&
			g_entities[pl[i]].health > 0)
		{
			g_entities[pl[i]].client->ps.stats[STAT_HEALTH] += healthadd;
			if (g_entities[pl[i]].client->ps.stats[STAT_HEALTH] > g_entities[pl[i]].client->ps.stats[STAT_MAX_HEALTH])
			{
				g_entities[pl[i]].client->ps.stats[STAT_HEALTH] = g_entities[pl[i]].client->ps.stats[STAT_MAX_HEALTH];
			}

			g_entities[pl[i]].health = g_entities[pl[i]].client->ps.stats[STAT_HEALTH];

			//At this point we know we got one, so add him into the collective event client bitflag
			if (!te)
			{
				te = G_TempEntity(self->client->ps.origin, EV_TEAM_POWER);
				te->s.eventParm = 1; //eventParm 1 is heal, eventParm 2 is force regen

				//since we had an extra check above, do the drain now because we got at least one guy
				WP_ForcePowerDrain(&self->client->ps, FP_TEAM_HEAL,
					forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL]);
			}

			wp_add_to_client_bitflags(te, pl[i]);
			//Now cramming it all into one event.. doing this many g_sound events at once was a Bad Thing.
		}
		i++;
	}
}

void ForceTeamForceReplenish(const gentity_t* self)
{
	float radius = 256;
	int i = 0;
	int numpl = 0;
	int pl[MAX_CLIENTS];
	int poweradd;
	gentity_t* te = NULL;

	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_TEAM_FORCE))
	{
		return;
	}

	if (self->client->ps.fd.forcePowerDebounce[FP_TEAM_FORCE] >= level.time)
	{
		return;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_2)
	{
		radius *= 1.5;
	}
	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_3)
	{
		radius *= 2;
	}

	while (i < MAX_CLIENTS)
	{
		const gentity_t* ent = &g_entities[i];

		if (ent && ent->client && self != ent && OnSameTeam(self, ent) && ent->client->ps.fd.forcePower < 100 &&
			ForcePowerUsableOn(self, ent, FP_TEAM_FORCE) &&
			trap->InPVS(self->client->ps.origin, ent->client->ps.origin))
		{
			vec3_t a;
			VectorSubtract(self->client->ps.origin, ent->client->ps.origin, a);

			if (VectorLength(a) <= radius)
			{
				pl[numpl] = i;
				numpl++;
			}
		}

		i++;
	}

	if (numpl < 1)
	{
		return;
	}

	if (numpl == 1)
	{
		poweradd = 50;
	}
	else if (numpl == 2)
	{
		poweradd = 33;
	}
	else
	{
		poweradd = 25;
	}
	self->client->ps.fd.forcePowerDebounce[FP_TEAM_FORCE] = level.time + 2000;

	WP_ForcePowerDrain(&self->client->ps, FP_TEAM_FORCE,
		forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE]);

	i = 0;

	while (i < numpl)
	{
		g_entities[pl[i]].client->ps.fd.forcePower += poweradd;
		if (g_entities[pl[i]].client->ps.fd.forcePower > g_entities[pl[i]].client->ps.fd.forcePowerMax)
		{
			g_entities[pl[i]].client->ps.fd.forcePower = g_entities[pl[i]].client->ps.fd.forcePowerMax;
		}

		//At this point we know we got one, so add him into the collective event client bitflag
		if (!te)
		{
			te = G_TempEntity(self->client->ps.origin, EV_TEAM_POWER);
			te->s.eventParm = 2; //eventParm 1 is heal, eventParm 2 is force regen
		}

		wp_add_to_client_bitflags(te, pl[i]);
		//Now cramming it all into one event.. doing this many g_sound events at once was a Bad Thing.

		i++;
	}
}

static qboolean IsHybrid(const gentity_t* ent)
{
	qboolean jedi = qfalse, merc = qfalse;

	if (ent->client->ps.fd.forcePowersKnown & 1 << FP_SEE)
	{
		jedi = qtrue;
	}

	if (ent->client->skillLevel[SK_JETPACK])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_PISTOL])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_BLASTER])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_THERMAL])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_ROCKET])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_BACTA])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_FLAMETHROWER])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_BOWCASTER])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_FORCEFIELD])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_CLOAK])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_SEEKER])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_SENTRY])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_DETPACK])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_REPEATER])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_DISRUPTOR])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_ACROBATICS])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_TUSKEN_RIFLE])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_GRENADE])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_SMOKEGRENADE])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_FLASHGRENADE])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_CRYOBAN])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_EMP])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_VEHICLE])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_EOPIE])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_TAUNTAUN])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_SWOOP])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_DROIDEKA])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_RANCOR])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_CONCUSSION])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_ELECTROPULSE])
	{
		merc = qtrue;
	}

	if (jedi && merc)
	{
		return qtrue;
	}

	return qfalse;
}

static qboolean WP_CounterForce(const gentity_t* attacker, const gentity_t* defender, const int attackPower)
{
	if (BG_IsUsingHeavyWeap(&defender->client->ps))
	{
		//can't block force powers while using heavy weapons
		return qfalse;
	}

	if (!(defender->client->ps.fd.forcePowersKnown & 1 << attackPower) && !(defender->client->ps.fd.forcePowersKnown &
		1 << FP_ABSORB))
	{
		//doesn't have absorb or same power as the attack power.
		return qfalse;
	}

	//determine ability difference
	int abilityDef = attacker->client->ps.fd.forcePowerLevel[attackPower] - defender->client->ps.fd.forcePowerLevel[
		attackPower];

	if (abilityDef > attacker->client->ps.fd.forcePowerLevel[attackPower] - defender->client->ps.fd.forcePowerLevel[
		FP_ABSORB])
	{
		//defender's absorb ability is stronger than their attackPower ability, use that instead.
		abilityDef = attacker->client->ps.fd.forcePowerLevel[attackPower] - defender->client->ps.fd.forcePowerLevel[
			FP_ABSORB];
	}

	if (abilityDef >= 2)
	{
		//defender is largely weaker than the attacker (2 levels)
		if (!walk_check(defender) || defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			//can't block much stronger Force power while running or in mid-air
			return qfalse;
		}
	}
	else if (abilityDef >= 1)
	{
		//defender is slightly weaker than their attacker
		if (defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			return qfalse;
		}
	}

	if (PM_SaberInBrokenParry(defender->client->ps.saber_move))
	{
		//can't block while stunned
		return qfalse;
	}

	if (BG_InSlowBounce(&defender->client->ps) && defender->client->ps.userInt3 & 1 << FLAG_OLDSLOWBOUNCE)
	{
		//can't block lightning while in the heavier slow bounces.
		return qfalse;
	}

	if (defender->client->ps.fd.blockPoints <= BLOCKPOINTS_HALF)
	{
		//can't block if we're too off balance.
		return qfalse;
	}

	if (defender->client->ps.saberFatigueChainCount >= MISHAPLEVEL_HEAVY)
	{
		//can't block if we're too off balance.
		return qfalse;
	}
	if (defender->client->ps.saberFatigueChainCount >= MISHAPLEVEL_LIGHT
		&& attacker->client->ps.fd.saberAnimLevel == SS_DESANN)
	{
		//can't block if we're too off balance and they are using Juyo's perk
		return qfalse;
	}
	if (defender->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		//can block force while using forceHandExtend.
		return qtrue;
	}

	if (!(defender->client->buttons & BUTTON_BLOCK))
	{
		if (defender->r.svFlags & SVF_BOT || defender->s.eType == ET_NPC)
		{
			return qtrue;
		}
		return qfalse;
	}

	if (IsHybrid(defender))
	{
		defender->client->ps.userInt3 |= 1 << FLAG_BLOCKING;
		defender->client->ps.ManualBlockingTime = level.time + 1000;
	}
	return qtrue;
}

void ForceGrip(const gentity_t* self)
{
	trace_t tr;
	vec3_t tfrom, tto, fwd;

	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->client->ps.userInt3 & 1 << FLAG_PREBLOCK)
	{
		return;
	}

	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saber_move) || !(self->client->ps.userInt3
		& 1 << FLAG_PREBLOCK)))
	{
		return;
	}

	if (self->client->ps.fd.forceGripUseTime > level.time)
	{
		return;
	}

	if (self->r.svFlags & SVF_BOT && self->client->ps.weapon == WP_SABER) //npc force use limit
	{
		if (self->client->ps.fd.blockPoints < 75 || self->client->ps.fd.forcePower < 75)
		{
			return;
		}
	}

	if (!WP_ForcePowerUsable(self, FP_GRIP))
	{
		return;
	}

	VectorCopy(self->client->ps.origin, tfrom);
	tfrom[2] += self->client->ps.viewheight;
	AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
	tto[0] = tfrom[0] + fwd[0] * MAX_GRIP_DISTANCE;
	tto[1] = tfrom[1] + fwd[1] * MAX_GRIP_DISTANCE;
	tto[2] = tfrom[2] + fwd[2] * MAX_GRIP_DISTANCE;

	trap->Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction != 1.0 &&
		tr.entityNum != ENTITYNUM_NONE &&
		g_entities[tr.entityNum].client &&
		!g_entities[tr.entityNum].client->ps.fd.forceGripCripple &&
		g_entities[tr.entityNum].client->ps.fd.forceGripBeingGripped < level.time &&
		ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_GRIP) &&
		!WP_CounterForce(self, &g_entities[tr.entityNum], FP_GRIP) &&
		(g_friendlyFire.integer || !OnSameTeam(self, &g_entities[tr.entityNum])))
		//don't grip someone who's still crippled
	{
		if (g_entities[tr.entityNum].s.number < MAX_CLIENTS && g_entities[tr.entityNum].client->ps.m_iVehicleNum)
		{
			//a player on a vehicle
			const gentity_t* vehEnt = &g_entities[g_entities[tr.entityNum].client->ps.m_iVehicleNum];
			if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
			{
				if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
					vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
				{
					//push the guy off
					vehEnt->m_pVehicle->m_pVehicleInfo->Eject(vehEnt->m_pVehicle,
						(bgEntity_t*)&g_entities[tr.entityNum], qfalse);
				}
			}
		}
		self->client->ps.fd.forceGripentity_num = tr.entityNum;
		g_entities[tr.entityNum].client->ps.fd.forceGripStarted = level.time;
		self->client->ps.fd.forceGripDamageDebounceTime = 0;

		self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
		self->client->ps.forceHandExtendTime = level.time + 5000;
	}
	else
	{
		self->client->ps.fd.forceGripentity_num = ENTITYNUM_NONE;
	}
}

int IsPressingDashButton(const gentity_t* self)
{
	if (PM_RunningAnim(self->client->ps.legsAnim)
		&& !PM_SaberInAttack(self->client->ps.saber_move)
		&& self->client->pers.cmd.upmove == 0
		&& !self->client->hookhasbeenfired
		&& (!(self->client->buttons & BUTTON_KICK))
		&& (!(self->client->buttons & BUTTON_USE))
		&& self->client->buttons & BUTTON_DASH
		&& self->client->ps.pm_flags & PMF_DASH_HELD)
	{
		return qtrue;
	}
	return qfalse;
}

int IsPressingKickButton(const gentity_t* self)
{
	if ((!(self->client->buttons & BUTTON_DASH))
		&& (self->client->buttons & BUTTON_KICK && self->client->ps.pm_flags & PMF_KICK_HELD))
	{
		return qtrue;
	}
	return qfalse;
}

static void WP_DebounceForceDeactivateTime(const gentity_t* self)
{
	if (self && self->client)
	{
		if (self->client->ps.fd.forcePowersActive & 1 << FP_SPEED
			|| self->client->ps.fd.forcePowersActive & 1 << FP_PROTECT
			|| self->client->ps.fd.forcePowersActive & 1 << FP_ABSORB
			|| self->client->ps.fd.forcePowersActive & 1 << FP_RAGE
			|| self->client->ps.fd.forcePowersActive & 1 << FP_SEE)
		{
			//already running another power that can be manually, stopped don't debounce so long
			self->client->ps.forceAllowDeactivateTime = level.time + 500;
		}
		else
		{
			//not running one of the interrupt able powers
			self->client->ps.forceAllowDeactivateTime = level.time + 1500;
		}
	}
}

void ForceSpeed(gentity_t* self, const int forceDuration)
{
	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (BG_InKnockDown(self->client->ps.legsAnim) || PM_InKnockDown(&self->client->ps))
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		self->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
	{
		WP_ForcePowerStop(self, FP_SPEED);
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_SPEED))
	{
		return;
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		return;
	}

	if (self->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
	{
		//it's already turned on.  turn it off.
		WP_ForcePowerStop(self, FP_SPEED);
		return;
	}

	if (self->client->holdingObjectiveItem >= MAX_CLIENTS
		&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD)
	{
		//holding Siege item
		if (g_entities[self->client->holdingObjectiveItem].genericValue15)
		{
			//disables force powers
			return;
		}
	}

	WP_DebounceForceDeactivateTime(self);
	WP_ForcePowerStart(self, FP_SPEED, forceDuration);

	if (self->client->ps.fd.forcePowerLevel[FP_SPEED] < FORCE_LEVEL_3)
	{
		//short burst
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/speed.mp3"));
	}
	else
	{
		//holding it
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/speedloop.wav"));
	}
	G_PlayBoltedEffect(G_EffectIndex("misc/breath.efx"), self, "*head_front");
}

static void ForceDashAnim(gentity_t* self)
{
	const int setAnimOverride = SETANIM_AFLAG_PACE;

	if (self->client->pers.cmd.rightmove > 0)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_HOP_R, setAnimOverride, 0);
	}
	else if (self->client->pers.cmd.rightmove < 0)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_HOP_L, setAnimOverride, 0);
	}
	else if (self->client->pers.cmd.forwardmove < 0)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_HOP_B, setAnimOverride, 0);
	}
	else
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_HOP_F, setAnimOverride, 0);
	}
}

void ForceDashAnimDash(gentity_t* self)
{
	const int setAnimOverride = SETANIM_AFLAG_PACE;

	if (self->client->pers.cmd.rightmove > 0)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_DASH_R, setAnimOverride, 0);
	}
	else if (self->client->pers.cmd.rightmove < 0)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_DASH_L, setAnimOverride, 0);
	}
	else if (self->client->pers.cmd.forwardmove < 0)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_DASH_B, setAnimOverride, 0);
	}
	else
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_DASH_F, setAnimOverride, 0);
	}
}

static void ForceSpeedDash(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//can't dash in mid-air
		return;
	}

	if (self->watertype == CONTENTS_WATER)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (PM_SaberInAttack(self->client->ps.saber_move))
	{
		return;
	}

	if (PM_kick_move(self->client->ps.saber_move))
	{
		return;
	}

	if (PM_InSlopeAnim(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->client->ps.fd.forcePowerDebounce[FP_SPEED] > level.time)
	{
		//stops it while using it and also after using it, up to 3 second delay
		return;
	}

	if (self->client->ps.fd.forceSpeedRecoveryTime >= level.time)
	{
		return;
	}

	if (BG_InKnockDown(self->client->ps.legsAnim) || PM_InKnockDown(&self->client->ps))
	{
		return;
	}

	if (self->client->NPC_class == CLASS_DROIDEKA ||
		self->client->NPC_class == CLASS_VEHICLE ||
		self->client->NPC_class == CLASS_SBD ||
		self->client->NPC_class == CLASS_OBJECT ||
		self->client->NPC_class == CLASS_ASSASSIN_DROID)
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time && self->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
	{
		WP_ForcePowerStop(self, FP_SPEED);
		return;
	}

	if (self->client->ps.fd.forcePowersActive & 1 << FP_SPEED) //If using speed at same time just in case
	{
		if (PM_RunningAnim(self->client->ps.legsAnim))
		{
			ForceDashAnim(self);
			WP_ForcePowerStop(self, FP_SPEED);
		}
		else
		{
			return;
		}
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		return;
	}

	if (!IsPressingDashButton(self))
	{
		//it's already turned on.  turn it off.
		return;
	}

	if (!(self->client->ps.communicatingflags & 1 << DASHING))
	{
		return;
	}

	if (!(self->client->ps.pm_flags & PMF_DASH_HELD))
	{
		return;
	}

	if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		vec3_t dir;

		AngleVectors(self->client->ps.viewangles, dir, NULL, NULL);
		self->client->ps.velocity[0] = self->client->ps.velocity[0] * 4;
		self->client->ps.velocity[1] = self->client->ps.velocity[1] * 4;

		ForceDashAnimDash(self);
	}
	else if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_FORCEINAIR1, SETANIM_AFLAG_PACE, 0);
	}

	G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/dash.mp3"));
	G_PlayBoltedEffect(G_EffectIndex("misc/breath.efx"), self, "*head_front");
}

void ForceSeeing(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		self->client->ps.fd.forcePowersActive & 1 << FP_SEE)
	{
		WP_ForcePowerStop(self, FP_SEE);
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_SEE))
	{
		return;
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart(self, FP_SEE, 0);

	if (self->client->ps.saberLockTime < level.time && !PM_InKnockDown(&self->client->ps))
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_FORCE_DRAIN_HOLD,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		self->client->ps.weaponTime = self->client->ps.torsoTimer;
		self->client->ps.saber_move = self->client->ps.saberBounceMove = LS_READY;
		self->client->ps.saberBlocked = BLOCKED_NONE;
	}

	G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/see.wav"));
	G_Sound(self, TRACK_CHANNEL_5, seeLoopSound);
}

void ForceProtect(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		self->client->ps.fd.forcePowersActive & 1 << FP_PROTECT)
	{
		WP_ForcePowerStop(self, FP_PROTECT);
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_PROTECT))
	{
		return;
	}

	// Make sure to turn off Force Rage.
	if (self->client->ps.fd.forcePowersActive & 1 << FP_RAGE)
	{
		WP_ForcePowerStop(self, FP_RAGE);
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart(self, FP_PROTECT, 0);

	if (self->client->ps.saberLockTime < level.time && !PM_InKnockDown(&self->client->ps))
	{
		if (self->client->ps.fd.forcePowerLevel[FP_PROTECT] > FORCE_LEVEL_1)
		{
			//level 2 only does it on torso (can keep running)
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_FORCE_PROTECT_FAST,
				SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		}
		else
		{
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_FORCE_PROTECT,
				SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		}
		self->client->ps.weaponTime = self->client->ps.torsoTimer;
		self->client->ps.saber_move = self->client->ps.saberBounceMove = LS_READY;
		self->client->ps.saberBlocked = BLOCKED_NONE;
	}

	G_PreDefSound(self->client->ps.origin, PDSOUND_PROTECT);
	G_Sound(self, TRACK_CHANNEL_3, protectLoopSound);
}

void ForceAbsorb(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		self->client->ps.fd.forcePowersActive & 1 << FP_ABSORB)
	{
		WP_ForcePowerStop(self, FP_ABSORB);
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_ABSORB))
	{
		return;
	}

	// Make sure to turn off Force Rage and Force Protection.
	if (self->client->ps.fd.forcePowersActive & 1 << FP_RAGE)
	{
		WP_ForcePowerStop(self, FP_RAGE);
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart(self, FP_ABSORB, 0);

	if (self->client->ps.saberLockTime < level.time && !PM_InKnockDown(&self->client->ps))
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_FORCE_ABSORB,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		self->client->ps.weaponTime = self->client->ps.torsoTimer;
		self->client->ps.saber_move = self->client->ps.saberBounceMove = LS_READY;
		self->client->ps.saberBlocked = BLOCKED_NONE;
	}

	G_PreDefSound(self->client->ps.origin, PDSOUND_ABSORB);
	G_Sound(self, TRACK_CHANNEL_3, absorbLoopSound);
}

void ForceRage(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		self->client->ps.fd.forcePowersActive & 1 << FP_RAGE)
	{
		WP_ForcePowerStop(self, FP_RAGE);
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_RAGE))
	{
		return;
	}

	if (self->client->ps.fd.forceRageRecoveryTime >= level.time)
	{
		return;
	}

	if (self->r.svFlags & SVF_BOT && self->client->ps.weapon == WP_SABER) //npc force use limit
	{
		if (self->client->ps.fd.blockPoints < 75 || self->client->ps.fd.forcePower < 75)
		{
			return;
		}
	}

	if (self->s.number < MAX_CLIENTS
		&& self->health < 25)
	{
		//have to have at least 25 health to start it
		return;
	}

	if (self->health < 10)
	{
		return;
	}

	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.fd.forcePowersActive & 1 << FP_PROTECT)
	{
		WP_ForcePowerStop(self, FP_PROTECT);
	}
	if (self->client->ps.fd.forcePowersActive & 1 << FP_ABSORB)
	{
		WP_ForcePowerStop(self, FP_ABSORB);
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart(self, FP_RAGE, 0);

	if (self->client->ps.saberLockTime < level.time && !PM_InKnockDown(&self->client->ps))
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_FORCE_RAGE,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		self->client->ps.weaponTime = self->client->ps.torsoTimer;
		self->client->ps.saber_move = self->client->ps.saberBounceMove = LS_READY;
		self->client->ps.saberBlocked = BLOCKED_NONE;
	}

	if (self->client->pers.botclass == BCLASS_CHEWIE
		|| self->client->pers.botclass == BCLASS_WOOKIE
		|| self->client->pers.botclass == BCLASS_WOOKIEMELEE || self->client->NPC_class == CLASS_WOOKIE)
	{
		G_SoundOnEnt(self, CHAN_VOICE, "sound/chars/chewbacca/misc/death2.mp3");
	}
	else
	{
		G_SoundOnEnt(self, CHAN_VOICE, "sound/weapons/force/rage.mp3");
	}

	G_Sound(self, TRACK_CHANNEL_4, G_SoundIndex("sound/weapons/force/rage.mp3"));
	G_Sound(self, TRACK_CHANNEL_3, rageLoopSound);

	G_PlayBoltedEffect(G_EffectIndex("misc/breathSith.efx"), self, "*head_front");
}

static qboolean ForceLightningCheckattack(const gentity_t* self)
{
	if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
		|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_OLD
		|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_NEW
		|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
		|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
		|| self->client->ps.torsoAnim == BOTH_FORCELIGHTNING_START
		|| self->client->ps.torsoAnim == BOTH_FORCELIGHTNING_HOLD
		|| self->client->ps.torsoAnim == BOTH_FORCELIGHTNING)
	{
		return qtrue;
	}
	return qfalse;
}

void ForceLightning(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}
	if (BG_InKnockDown(self->client->ps.legsAnim) || self->client->ps.leanofs || PM_InKnockDown(&self->client->ps))
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}
	if (!self->s.number && in_camera)
	{
		//can't force lightning when zoomed in or in cinematic
		return;
	}

	if (self->r.svFlags & SVF_BOT && self->client->ps.weapon == WP_SABER) //npc force use limit
	{
		if (self->client->ps.weapon == WP_SABER && self->client->ps.fd.blockPoints < 75 || self->client->ps.fd.
			forcePower < 75 || !WP_ForcePowerUsable(self, FP_LIGHTNING))
		{
			return;
		}
	}
	else
	{
		if (self->client->ps.fd.forcePower < 25 || !WP_ForcePowerUsable(self, FP_LIGHTNING))
		{
			return;
		}
	}

	if (self->client->ps.fd.forcePowerDebounce[FP_LIGHTNING] > level.time)
	{
		//stops it while using it and also after using it, up to 3 second delay
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}
	if (self->client->ps.userInt3 & 1 << FLAG_PREBLOCK)
	{
		return;
	}
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saber_move) || !(self->client->ps.userInt3
		& 1 << FLAG_PREBLOCK)))
	{
		return;
	}
	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.fd.forcePowersActive & 1 << FP_PROTECT)
	{
		WP_ForcePowerStop(self, FP_PROTECT);
	}
	if (self->client->ps.fd.forcePowersActive & 1 << FP_ABSORB)
	{
		WP_ForcePowerStop(self, FP_ABSORB);
	}
	if (self->client->ps.fd.forcePowersActive & 1 << FP_SEE)
	{
		WP_ForcePowerStop(self, FP_SEE);
	}

	BG_ClearRocketLock(&self->client->ps);

	//Shoot lightning from hand
	//using grip anim now, to extend the burst time
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 20000;

	G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/lightning2.wav");

	if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] < FORCE_LEVEL_2)
	{
		//short burst
		//G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/lightning3.mp3");
	}
	else
	{
		//holding it
		self->s.loopSound = G_SoundIndex("sound/weapons/force/lightning.mp3");
	}

	WP_ForcePowerStart(self, FP_LIGHTNING, 500);
}

static qboolean melee_block_lightning_counter_force(gentity_t* attacker, const gentity_t* defender, int attackPower)
{
	//generically checks to see if the defender is able to block an attack from this attacker
	if (!manual_forceblocking(defender))
	{
		return qfalse;
	}

	if (!walk_check(defender) || defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//can't block  Force power while running or in mid-air
		return qfalse;
	}
	if (!(defender->client->ps.fd.forcePowersKnown & 1 << FP_ABSORB))
	{
		//doesn't have absorb
		return qfalse;
	}

	return qtrue;
}

static qboolean melee_block_lightning(gentity_t* attacker, gentity_t* defender)
{
	const qboolean melee_light_block = qtrue;

	if (defender->client->ps.weapon == WP_SABER
		|| defender->client->ps.saberHolstered == 2
		|| defender->client->ps.saberInFlight)
	{
		return qfalse;
	}

	if (!melee_block_lightning_counter_force(attacker, defender, FP_LIGHTNING))
	{
		return qfalse;
	}

	if (!defender || !defender->client || !attacker || !attacker->client)
	{
		return qfalse;
	}

	if (!in_front(attacker->client->ps.origin, defender->client->ps.origin, defender->client->ps.viewangles, -0.7f))
		//can't block behind us while hand blocking.
	{
		//not facing the lightning attacker
		return qfalse;
	}
	//determine the cost to block the lightning
	const int fp_hand_block_cost = Q_irand(1, 3);

	if (defender->client->ps.fd.forcePower < 20)
	{
		return qfalse;
	}

	if (melee_light_block)
	{
		G_SetAnim(defender, &defender->client->pers.cmd, SETANIM_TORSO, BOTH_FORCE_2HANDEDLIGHTNING,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		defender->client->ps.weaponTime = Q_irand(300, 600);
		G_PlayBoltedEffect(G_EffectIndex("force/HBlockLightning.efx"), defender, "*r_hand");
		WP_ForcePowerDrain(&defender->client->ps, FP_ABSORB, fp_hand_block_cost);
		G_PlayBoltedEffect(G_EffectIndex("force/HBlockLightning.efx"), defender, "*l_hand");
	}
	return qtrue;
}

static qboolean saber_block_lightning(const gentity_t* attacker, const gentity_t* defender)
{
	//defender is attempting to block lightning.  Try to do it.
	const qboolean is_holding_block_button_and_attack = defender->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK
		? qtrue
		: qfalse; //Active Blocking
	int fp_block_cost;
	const qboolean saber_light_block = qtrue;

	if (!manual_saberblocking(defender))
	{
		return qfalse;
	}

	if (!defender || !defender->client || !attacker || !attacker->client)
	{
		return qfalse;
	}

	if (!in_front(attacker->client->ps.origin, defender->client->ps.origin, defender->client->ps.viewangles, -0.7f))
		//can't block behind us while hand blocking.
	{
		//not facing the lightning attacker
		return qfalse;
	}

	//determine the cost to block the lightning
	if (defender->client && defender->r.svFlags & SVF_BOT)
	{
		fp_block_cost = Q_irand(1, 3);
	}
	else
	{
		fp_block_cost = 1;
	}

	if (defender->client->ps.fd.forcePower < 10)
	{
		return qfalse;
	}

	if (saber_light_block)
	{
		if (is_holding_block_button_and_attack)
		{
			PM_AddBlockFatigue(&defender->client->ps, fp_block_cost);
		}
		else
		{
			WP_ForcePowerDrain(&defender->client->ps, FP_ABSORB, fp_block_cost);
		}
	}
	return qtrue;
}

static void force_lightning_damage(gentity_t* self, gentity_t* traceEnt, vec3_t dir, const float dist, const float dot,
	vec3_t impact_point)
{
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if (traceEnt && traceEnt->takedamage)
	{
		if (!traceEnt->client && traceEnt->s.eType == ET_NPC)
		{
			if (traceEnt->s.genericenemyindex < level.time)
			{
				traceEnt->s.genericenemyindex = level.time + 2000;
			}
		}
		if (traceEnt->client)
		{
			if (traceEnt->client->noLightningTime >= level.time)
			{
				//give them power and don't hurt them.
				traceEnt->client->ps.fd.forcePower++;
				if (traceEnt->client->ps.fd.forcePower > traceEnt->client->ps.fd.forcePowerMax)
				{
					traceEnt->client->ps.fd.forcePower = traceEnt->client->ps.fd.forcePowerMax;
				}
				return;
			}
			if (ForcePowerUsableOn(self, traceEnt, FP_LIGHTNING))
			{
				int dmg = 1;

				if (self->client->NPC_class == CLASS_REBORN && self->client->ps.weapon == WP_NONE)
				{
					//Cultist: looks fancy, but does less damage
				}
				else
				{
					if (dist < 100)
					{
						dmg += 2;
					}
					else if (dist < 200)
					{
						dmg += 1;
					}
					if (dot > 0.9f)
					{
						dmg += 2;
					}
					else if (dot > 0.7f)
					{
						dmg += 1;
					}
				}

				if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_OLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_NEW
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{
					dmg *= 2;
				}
				else
				{
					dmg = Q_irand(1, 3);
				}
				const qboolean saber_lightning_blocked = saber_block_lightning(self, traceEnt);
				const qboolean hand_lightning_blocked = melee_block_lightning(self, traceEnt);

				if (dmg && !saber_lightning_blocked && !hand_lightning_blocked)
				{
					if (traceEnt->s.weapon == WP_SABER)
					{
						G_Damage(traceEnt, self, self, dir, impact_point, dmg, DAMAGE_LIGHNING_KNOCKBACK,
							MOD_FORCE_LIGHTNING);
					}
					else
					{
						G_Damage(traceEnt, self, self, dir, impact_point, dmg, DAMAGE_NO_KNOCKBACK,
							MOD_FORCE_LIGHTNING);
					}

					if (traceEnt->s.weapon != WP_EMPLACED_GUN)
					{
						if (traceEnt
							&& traceEnt->health <= 35 && !class_is_gunner(traceEnt))
						{
							traceEnt->client->stunDamage = 9;
							traceEnt->client->stunTime = level.time + 1000;

							gentity_t* tent = G_TempEntity(traceEnt->r.currentOrigin, EV_STUNNED);
							tent->s.owner = traceEnt->s.number;
						}

						if (traceEnt->client->ps.stats[STAT_HEALTH] <= 0 && class_is_gunner(traceEnt))
						{
							VectorClear(traceEnt->client->ps.lastHitLoc);
							VectorClear(traceEnt->client->ps.velocity);

							traceEnt->client->ps.eFlags |= EF_DISINTEGRATION;
							traceEnt->r.contents = 0;

							traceEnt->think = G_FreeEntity;
							traceEnt->nextthink = level.time;
						}

						if (PM_RunningAnim(traceEnt->client->ps.legsAnim) && traceEnt->client->ps.stats[STAT_HEALTH] > 1)
						{
							G_KnockOver(traceEnt, self, dir, 25, qtrue);
						}
						else if (traceEnt->client->ps.groundEntityNum == ENTITYNUM_NONE && traceEnt->client->ps.stats[STAT_HEALTH] > 1)
						{
							g_throw(traceEnt, dir, 2);
							G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_BOTH, Q_irand(BOTH_SLAPDOWNRIGHT, BOTH_SLAPDOWNLEFT), SETANIM_AFLAG_PACE, 0);
						}
						else
						{
							if (!PM_RunningAnim(traceEnt->client->ps.legsAnim)
								&& !PM_InKnockDown(&traceEnt->client->ps)
								&& traceEnt->client->ps.stats[STAT_HEALTH] > 1)
							{
								if (traceEnt->client->ps.stats[STAT_HEALTH] < 75)
								{
									G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_TORSO, BOTH_COWER1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
								}
								else if (traceEnt->client->ps.stats[STAT_HEALTH] < 50)
								{
									G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_TORSO, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
								}
								else
								{
									G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_TORSO, BOTH_FACEPROTECT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
								}
							}
							else
							{
								if (traceEnt->client->ps.stats[STAT_HEALTH] < 2 && class_is_gunner(traceEnt))
								{
									vec3_t defaultDir;
									VectorSet(defaultDir, 0, 0, 1);
									G_PlayEffectID(G_EffectIndex("force/Lightningkill.efx"), traceEnt->r.currentOrigin, defaultDir);
								}
							}
						}
					}
				}

				if (traceEnt->client)
				{
					if (!Q_irand(0, 2))
					{
						G_Sound(traceEnt, CHAN_BODY,
							G_SoundIndex(va("sound/weapons/force/lightninghit%d.mp3", Q_irand(1, 3))));
					}

					if (traceEnt->client && traceEnt->client->ps.powerups[PW_CLOAKED])
					{
						//disable cloak temporarily
						Jedi_Decloak(traceEnt);
						traceEnt->client->cloakToggleTime = level.time + Q_irand(3000, 10000);
					}

					const class_t npc_class = traceEnt->client->NPC_class;
					const bclass_t botclass = traceEnt->client->pers.botclass;

					if (traceEnt->health <= 0 || (botclass == BCLASS_SEEKER || botclass == BCLASS_SBD ||
						botclass == BCLASS_MANDOLORIAN || botclass == BCLASS_MANDOLORIAN1 || botclass ==
						BCLASS_MANDOLORIAN2 ||
						botclass == BCLASS_BOBAFETT || botclass == BCLASS_BATTLEDROID || botclass == BCLASS_DROIDEKA))
					{
						traceEnt->client->ps.electrifyTime = level.time + 4000;
					}
					else if (traceEnt->health <= 0 || (npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE ||
						npc_class == CLASS_MOUSE || npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class ==
						CLASS_REMOTE ||
						npc_class == CLASS_R5D2 || npc_class == CLASS_PROTOCOL || npc_class == CLASS_MARK1 ||
						npc_class == CLASS_SBD || npc_class == CLASS_BATTLEDROID || npc_class == CLASS_DROIDEKA ||
						npc_class == CLASS_MARK2 || npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST ||
						npc_class == CLASS_SENTRY))
					{
						// special droid only behaviors
						traceEnt->client->ps.electrifyTime = level.time + 4000;
					}
					else if (traceEnt->client->ps.electrifyTime < level.time + 400 && !saber_lightning_blocked && !
						hand_lightning_blocked)
					{
						//only update every 400ms to reduce bandwidth usage (as it is passing a 32-bit time value)
						traceEnt->client->ps.electrifyTime = level.time + 3000;
					}

					if (traceEnt->client
						&& traceEnt->health > 0
						&& traceEnt->NPC
						&& traceEnt->client->ps.weapon == WP_SABER)
					{
						if (manual_saberblocking(traceEnt)
							&& traceEnt->client->ps.fd.forcePower > 20
							&& !traceEnt->client->ps.saberHolstered
							&& !traceEnt->client->ps.saberInFlight //saber in hand
							&& InFOV3(self->r.currentOrigin, traceEnt->r.currentOrigin,
								traceEnt->client->ps.viewangles,
								20, 35) //I'm in front of them
							&& !PM_InKnockDown(&traceEnt->client->ps))
						{
							//saber can block lightning
							const int rsaber_num = 0;
							const int rblade_num = 0;
							traceEnt->client->saber[rsaber_num].blade[rblade_num].storageTime = level.time;
							if (saber_lightning_blocked && !hand_lightning_blocked)
							{
								const float chance_of_fizz = flrand(0.0f, 1.0f);
								vec3_t ang = { 0, 0, 0 };
								ang[0] = flrand(0, 360);
								ang[1] = flrand(0, 360);
								ang[2] = flrand(0, 360);
								if (chance_of_fizz > 0)
								{
									vec3_t end2;
									VectorMA(traceEnt->client->saber[rsaber_num].blade[rblade_num].muzzlePoint,
										traceEnt->client->saber[rsaber_num].blade[rblade_num].lengthMax *
										flrand(0, 1),
										traceEnt->client->saber[rsaber_num].blade[rblade_num].muzzleDir, end2);
									if (traceEnt->client->ps.fd.blockPoints > 50)
									{
										G_PlayEffectID(G_EffectIndex("saber/saber_Lightninghit.efx"), end2, ang);
									}
									else
									{
										G_PlayEffectID(G_EffectIndex("saber/fizz.efx"), end2, ang);
									}
								}
								switch (traceEnt->client->ps.fd.saberAnimLevel)
								{
								case SS_DUAL:
									G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_T_,
										SETANIM_AFLAG_PACE, 0);
									break;
								case SS_STAFF:
									G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_T_,
										SETANIM_AFLAG_PACE, 0);
									break;
								case SS_FAST:
								case SS_TAVION:
								case SS_STRONG:
								case SS_DESANN:
								case SS_MEDIUM:
									G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_T_,
										SETANIM_AFLAG_PACE, 0);
									break;
								case SS_NONE:
								default:
									G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_T_,
										SETANIM_AFLAG_PACE, 0);
									break;
								}
								traceEnt->client->ps.weaponTime = Q_irand(300, 600);
							}
							else
							{
								dmg = 1;
							}
						}
					}
					else if (traceEnt->client->ps.weapon == WP_SABER)
					{
						//Serenitysabersystems saber can block lightning
						const int rsaber_num = 0;
						const int rblade_num = 0;
						traceEnt->client->saber[rsaber_num].blade[rblade_num].storageTime = level.time;
						if (saber_lightning_blocked && !hand_lightning_blocked)
						{
							const float chance_of_fizz = flrand(0.0f, 1.0f);
							vec3_t ang = { 0, 0, 0 };
							ang[0] = flrand(0, 360);
							ang[1] = flrand(0, 360);
							ang[2] = flrand(0, 360);
							if (chance_of_fizz > 0)
							{
								vec3_t end2;
								VectorMA(traceEnt->client->saber[rsaber_num].blade[rblade_num].muzzlePoint,
									traceEnt->client->saber[rsaber_num].blade[rblade_num].lengthMax * flrand(0, 1),
									traceEnt->client->saber[rsaber_num].blade[rblade_num].muzzleDir, end2);
								if (traceEnt->client->ps.fd.blockPoints > 50)
								{
									G_PlayEffectID(G_EffectIndex("saber/saber_Lightninghit.efx"), end2, ang);
								}
								else
								{
									G_PlayEffectID(G_EffectIndex("saber/fizz.efx"), end2, ang);
								}
							}
							switch (traceEnt->client->ps.fd.saberAnimLevel)
							{
							case SS_DUAL:
								G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_TORSO, BOTH_P6_S6_T_,
									SETANIM_AFLAG_PACE, 0);
								break;
							case SS_STAFF:
								G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_TORSO, BOTH_P7_S7_T_,
									SETANIM_AFLAG_PACE, 0);
								break;
							case SS_FAST:
							case SS_TAVION:
							case SS_STRONG:
							case SS_DESANN:
							case SS_MEDIUM:
								G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_T_,
									SETANIM_AFLAG_PACE, 0);
								break;
							case SS_NONE:
							default:
								G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_TORSO, BOTH_P1_S1_T_,
									SETANIM_AFLAG_PACE, 0);
								break;
							}
							traceEnt->client->ps.weaponTime = Q_irand(300, 600);
						}
						else
						{
							dmg = 1;
						}
					}
					else if (traceEnt->client->ps.weapon == WP_NONE
						|| traceEnt->client->ps.weapon == WP_MELEE
						&& traceEnt->client->ps.fd.forcePower > 20
						&& in_front(self->client->ps.origin, traceEnt->client->ps.origin,
							traceEnt->client->ps.viewangles, -0.7f))
					{
						if (hand_lightning_blocked && !saber_lightning_blocked)
						{
							dmg = 0;
						}
						else
						{
							dmg = 1;
						}
					}
					else if (traceEnt && traceEnt->client && traceEnt->client->NPC_class == CLASS_GALAK)
					{
						if (traceEnt->client->ps.powerups[PW_GALAK_SHIELD])
						{
							//has shield up
							dmg = 0;
						}
					}
				}
			}
		}
	}
}

static void force_shoot_lightning(gentity_t* self)
{
	trace_t tr;
	vec3_t forward;
	gentity_t* traceEnt;

	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}
	AngleVectors(self->client->ps.viewangles, forward, NULL, NULL);
	VectorNormalize(forward);

	if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2)
	{
		vec3_t center;
		vec3_t mins;
		vec3_t maxs;
		vec3_t v;
		const float radius = FORCE_LIGHTNING_RADIUS;
		float dot;
		gentity_t* entity_list[MAX_GENTITIES];
		int iEntityList[MAX_GENTITIES];
		int i;

		VectorCopy(self->client->ps.origin, center);
		for (i = 0; i < 3; i++)
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
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
			vec3_t size;
			vec3_t ent_org;
			vec3_t dir;
			traceEnt = entity_list[e];

			if (!traceEnt)
				continue;
			if (traceEnt == self)
				continue;
			if (traceEnt->r.ownerNum == self->s.number && traceEnt->s.weapon != WP_THERMAL) //can push your own thermals
				continue;
			if (!traceEnt->inuse)
				continue;
			if (!traceEnt->takedamage)
				continue;
			//if (traceEnt->health <= 0)//no torturing corpses
			//	continue;
			if (!g_friendlyFire.integer && OnSameTeam(self, traceEnt))
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for (i = 0; i < 3; i++)
			{
				if (center[i] < traceEnt->r.absmin[i])
				{
					v[i] = traceEnt->r.absmin[i] - center[i];
				}
				else if (center[i] > traceEnt->r.absmax[i])
				{
					v[i] = center[i] - traceEnt->r.absmax[i];
				}
				else
				{
					v[i] = 0;
				}
			}

			VectorSubtract(traceEnt->r.absmax, traceEnt->r.absmin, size);
			VectorMA(traceEnt->r.absmin, 0.5, size, ent_org);

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract(ent_org, center, dir);
			VectorNormalize(dir);
			if ((dot = DotProduct(dir, forward)) < 0.5)
				continue;

			//must be close enough
			const float dist = VectorLength(v);
			if (dist >= radius)
			{
				continue;
			}

			//in PVS?
			if (!traceEnt->r.bmodel && !trap->InPVS(ent_org, self->client->ps.origin))
			{
				//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT,
				qfalse, 0, 0);
			if (tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number)
			{
				//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
			force_lightning_damage(self, traceEnt, dir, dist, dot, ent_org);
		}
	}
	else
	{
		vec3_t end;
		VectorMA(self->client->ps.origin, 2048, forward, end);

		trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, qfalse, 0,
			0);
		if (tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
		{
			return;
		}

		traceEnt = &g_entities[tr.entityNum];
		force_lightning_damage(self, traceEnt, forward, 0, 0, tr.endpos);
	}
}

void ForceDrain(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

	if (self->client->ps.fd.forcePower < 25 || !WP_ForcePowerUsable(self, FP_DRAIN))
	{
		return;
	}
	if (self->client->ps.fd.forcePowerDebounce[FP_DRAIN] > level.time)
	{
		//stops it while using it and also after using it, up to 3 second delay
		return;
	}
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 20000;

	G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/drain.wav"));

	WP_ForcePowerStart(self, FP_DRAIN, 500);
}

qboolean Jedi_DrainReaction(gentity_t* self);

static void ForceDrainDamage(gentity_t* self, gentity_t* traceEnt, vec3_t dir, vec3_t impact_point)
{
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if (traceEnt && traceEnt->takedamage)
	{
		if (traceEnt->client && (!OnSameTeam(self, traceEnt) || g_friendlyFire.integer) && self->client->ps.fd.
			forceDrainTime < level.time && traceEnt->client->ps.fd.forcePower)
		{
			//an enemy or object
			if (!traceEnt->client && traceEnt->s.eType == ET_NPC)
			{
				//g2animent
				if (traceEnt->s.genericenemyindex < level.time)
				{
					traceEnt->s.genericenemyindex = level.time + 2000;
				}
			}
			if (ForcePowerUsableOn(self, traceEnt, FP_DRAIN))
			{
				int modPowerLevel = -1;
				int dmg = 0;
				if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_1)
				{
					dmg = 2; //because it's one-shot
				}
				else if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_2)
				{
					dmg = 3;
				}
				else if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_3)
				{
					dmg = 4;
				}

				if (traceEnt->client)
				{
					modPowerLevel = wp_absorb_conversion(traceEnt, traceEnt->client->ps.fd.forcePowerLevel[FP_ABSORB],
						FP_DRAIN, self->client->ps.fd.forcePowerLevel[FP_DRAIN], 1);
				}

				if (modPowerLevel != -1)
				{
					if (!modPowerLevel)
					{
						dmg = 0;
					}
					else if (modPowerLevel == 1)
					{
						dmg = 1;
					}
					else if (modPowerLevel == 2)
					{
						dmg = 2;
					}
				}

				if (dmg)
				{
					traceEnt->client->ps.fd.forcePower -= dmg;
					Jedi_DrainReaction(traceEnt);
				}
				if (traceEnt->client->ps.fd.forcePower < 0)
				{
					traceEnt->client->ps.fd.forcePower = 0;
				}

				if (self->client->ps.stats[STAT_HEALTH] < self->client->ps.stats[STAT_MAX_HEALTH] &&
					self->health > 0 && self->client->ps.stats[STAT_HEALTH] > 0)
				{
					self->health += dmg;
					if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
					{
						self->health = self->client->ps.stats[STAT_MAX_HEALTH];
					}
					self->client->ps.stats[STAT_HEALTH] = self->health;
				}

				traceEnt->client->ps.fd.forcePowerRegenDebounceTime = level.time + 800;
				//don't let the client being drained get force power back right away

				if (traceEnt->client->forcePowerSoundDebounce < level.time)
				{
					gentity_t* tent = G_TempEntity(impact_point, EV_FORCE_DRAINED);
					tent->s.eventParm = DirToByte(dir);
					tent->s.owner = traceEnt->s.number;

					traceEnt->client->forcePowerSoundDebounce = level.time + 400;
				}
			}
		}
	}
}

extern qboolean G_BoxInBounds(vec3_t point, vec3_t mins, vec3_t maxs, vec3_t boundsMins, vec3_t boundsMaxs);

//-----------------------------------------------------------------------------
static void FP_TraceSetStart(const gentity_t* ent, vec3_t start, vec3_t mins, vec3_t maxs)
//-----------------------------------------------------------------------------
{
	//make sure our start point isn't on the other side of a wall
	trace_t tr;
	vec3_t entMins;
	vec3_t entMaxs;

	VectorAdd(ent->r.currentOrigin, ent->r.mins, entMins);
	VectorAdd(ent->r.currentOrigin, ent->r.maxs, entMaxs);

	if (G_BoxInBounds(start, mins, maxs, entMins, entMaxs))
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

#define FORCE_DESTRUCTION_SIZE			3
#define	DESTRUCTION_SPLASH_DAMAGE		10
#define	DESTRUCTION_SPLASH_RADIUS		160
#define	DESTRUCTION_DAMAGE				20
#define	DESTRUCTION_VELOCITY			1200
#define	DESTRUCTION_RANGE				150

//---------------------------------------------------------
static void WP_FireDestruction(gentity_t* ent, const int force_level)
//---------------------------------------------------------
{
	vec3_t start, forward;
	const int damage = DESTRUCTION_DAMAGE;
	float vel = DESTRUCTION_VELOCITY;

	if (force_level == FORCE_LEVEL_2)
	{
		vel *= 1.5f;
	}
	else if (force_level == FORCE_LEVEL_3)
	{
		vel *= 2.0f;
	}

	AngleVectors(ent->client->ps.viewangles, forward, NULL, NULL);
	VectorNormalize(forward);

	VectorCopy(ent->client->renderInfo.eyePoint, start);
	FP_TraceSetStart(ent, start, vec3_origin, vec3_origin);
	//make sure our start point isn't on the other side of a wall

	gentity_t* missile = CreateMissile(start, forward, vel, 10000, ent, qfalse);

	missile->classname = "rocket_proj";
	missile->s.weapon = WP_CONCUSSION;
	missile->s.powerups |= 1 << PW_FORCE_PROJECTILE;
	missile->mass = 10;

	// Make it easier to hit things
	VectorSet(missile->r.maxs, FORCE_DESTRUCTION_SIZE, FORCE_DESTRUCTION_SIZE, FORCE_DESTRUCTION_SIZE);
	VectorScale(missile->r.maxs, -1, missile->r.mins);

	missile->damage = damage * (1.0f + force_level) / 2.0f;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;

	missile->methodOfDeath = MOD_CONC;
	missile->splashMethodOfDeath = MOD_CONC;

	missile->clipmask = MASK_SHOT;
	missile->splashDamage = DESTRUCTION_SPLASH_DAMAGE * (1.0f + force_level) / 2.0f;
	missile->splashRadius = DESTRUCTION_SPLASH_RADIUS * (1.0f + force_level) / 2.0f;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

int ForceShootDrain(gentity_t* self)
{
	trace_t tr;
	vec3_t forward;
	int numDrained = 0;

	if (self->health <= 0)
	{
		return 0;
	}

	if (self->r.svFlags & SVF_BOT && self->client->ps.weapon == WP_SABER) //npc force use limit
	{
		if (self->client->ps.fd.blockPoints < 50 || self->client->ps.fd.forcePower < 50)
		{
			return 0;
		}
	}

	AngleVectors(self->client->ps.viewangles, forward, NULL, NULL);
	VectorNormalize(forward);

	if (self->client->ps.fd.forcePowerDebounce[FP_DRAIN] <= level.time)
	{
		gentity_t* traceEnt;
		if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2) // level 3 only
		{
			vec3_t center;
			vec3_t mins;
			vec3_t maxs;
			vec3_t v;
			const float radius = MAX_DRAIN_DISTANCE;
			gentity_t* entity_list[MAX_GENTITIES];
			int iEntityList[MAX_GENTITIES];
			int i;

			VectorCopy(self->client->ps.origin, center);
			for (i = 0; i < 3; i++)
			{
				mins[i] = center[i] - radius;
				maxs[i] = center[i] + radius;
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
				vec3_t size;
				vec3_t ent_org;
				vec3_t dir;
				float dot;
				traceEnt = entity_list[e];

				if (!traceEnt)
					continue;
				if (traceEnt == self)
					continue;
				if (!traceEnt->inuse)
					continue;
				if (!traceEnt->takedamage)
					continue;
				if (traceEnt->health <= 0) //no torturing corpses
					continue;
				if (!traceEnt->client)
					continue;
				if (!traceEnt->client->ps.fd.forcePower)
					continue;
				if (OnSameTeam(self, traceEnt) && !g_friendlyFire.integer)
					continue;
				//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
				// find the distance from the edge of the bounding box
				for (i = 0; i < 3; i++)
				{
					if (center[i] < traceEnt->r.absmin[i])
					{
						v[i] = traceEnt->r.absmin[i] - center[i];
					}
					else if (center[i] > traceEnt->r.absmax[i])
					{
						v[i] = center[i] - traceEnt->r.absmax[i];
					}
					else
					{
						v[i] = 0;
					}
				}

				VectorSubtract(traceEnt->r.absmax, traceEnt->r.absmin, size);
				VectorMA(traceEnt->r.absmin, 0.5, size, ent_org);

				//see if they're in front of me
				//must be within the forward cone
				VectorSubtract(ent_org, center, dir);
				VectorNormalize(dir);
				const float dist = VectorLength(v);

				if ((dot = DotProduct(dir, forward)) < 0.5)
				{
					continue;
				}

				//must be close enough
				if (dist >= radius)
				{
					continue;
				}

				//in PVS?
				if (!traceEnt->r.bmodel && !trap->InPVS(ent_org, self->client->ps.origin))
				{
					//must be in PVS
					continue;
				}

				//Now check and see if we can actually hit it
				trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT,
					qfalse, 0, 0);

				if (tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number)
				{
					//must have clear LOS
					continue;
				}

				// ok, we are within the radius, add us to the incoming list
				ForceDrainDamage(self, traceEnt, dir, ent_org);
				numDrained = 1;
			}
		}
		else
		{
			vec3_t end;
			//trace-line
			VectorMA(self->client->ps.origin, 2048, forward, end);

			trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, qfalse,
				0, 0);
			if (tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid || !g_entities[tr.
				entityNum].client || !g_entities[tr.entityNum].inuse)
			{
				return 0;
			}

			traceEnt = &g_entities[tr.entityNum];

			ForceDrainDamage(self, traceEnt, forward, tr.endpos);
			numDrained = 1;
		}
	}

	self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_DRAIN] + FORCE_LEVEL_3;

	WP_ForcePowerDrain(&self->client->ps, FP_DRAIN, 5); //used to be 1, but this did, too, anger the God of Balance.

	self->client->ps.fd.forcePowerRegenDebounceTime = level.time + 500;

	return numDrained;
}

static int ForceShootDestruction(gentity_t* self)
{
	trace_t tr;
	vec3_t forward;
	int numDrained = 0;

	if (self->health <= 0)
	{
		return 0;
	}

	if (self->client->ps.fd.forcePower < 85)
	{
		return 0;
	}

	AngleVectors(self->client->ps.viewangles, forward, NULL, NULL);
	VectorNormalize(forward);

	if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2)
	{
		vec3_t center;
		vec3_t mins;
		vec3_t maxs;
		vec3_t v;
		const float radius = MAX_DRAIN_DISTANCE;
		gentity_t* entity_list[MAX_GENTITIES];
		int iEntityList[MAX_GENTITIES];
		int i;

		VectorCopy(self->client->ps.origin, center);
		for (i = 0; i < 3; i++)
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
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
			vec3_t size;
			vec3_t ent_org;
			vec3_t dir;
			float dot;
			const gentity_t* traceEnt = entity_list[e];

			if (!traceEnt)
				continue;
			if (traceEnt == self)
				continue;
			if (!traceEnt->inuse)
				continue;
			if (!traceEnt->takedamage)
				continue;
			if (traceEnt->health <= 0) //no torturing corpses
				continue;
			if (!traceEnt->client)
				continue;
			if (!traceEnt->client->ps.fd.forcePower)
				continue;
			if (OnSameTeam(self, traceEnt) && !g_friendlyFire.integer)
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for (i = 0; i < 3; i++)
			{
				if (center[i] < traceEnt->r.absmin[i])
				{
					v[i] = traceEnt->r.absmin[i] - center[i];
				}
				else if (center[i] > traceEnt->r.absmax[i])
				{
					v[i] = center[i] - traceEnt->r.absmax[i];
				}
				else
				{
					v[i] = 0;
				}
			}

			VectorSubtract(traceEnt->r.absmax, traceEnt->r.absmin, size);
			VectorMA(traceEnt->r.absmin, 0.5, size, ent_org);

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract(ent_org, center, dir);
			VectorNormalize(dir);
			if ((dot = DotProduct(dir, forward)) < 0.5)
				continue;

			//must be close enough
			const float dist = VectorLength(v);
			if (dist >= radius)
			{
				continue;
			}

			//in PVS?
			if (!traceEnt->r.bmodel && !trap->InPVS(ent_org, self->client->ps.origin))
			{
				//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT,
				qfalse, 0, 0);
			if (tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number)
			{
				//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
			if (dist >= DESTRUCTION_RANGE)
			{
				WP_FireDestruction(self, self->client->ps.fd.forcePowerLevel[FP_DRAIN]);
				G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/destruction.mp3"));
				WP_ForcePowerDrain(&self->client->ps, FP_DRAIN, 50);
			}
			numDrained = 1;
		}
	}

	self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_DRAIN] + FORCE_LEVEL_3;

	self->client->ps.fd.forcePowerRegenDebounceTime = level.time + 500;

	return numDrained;
}

static void ForceJumpCharge(gentity_t* self, usercmd_t* ucmd)
{
	//I guess this is unused now. Was used for the "charge" jump type.
	const float forceJumpChargeInterval = forceJumpStrength[0] / (FORCE_JUMP_CHARGE_TIME / FRAMETIME);

	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (!self->client->ps.fd.forceJumpCharge && self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		return;
	}

	if (self->client->ps.fd.forcePower < forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]][
		FP_LEVITATION])
	{
		G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);
		return;
	}

	if (!self->client->ps.fd.forceJumpCharge)
	{
		self->client->ps.fd.forceJumpAddTime = 0;
	}

	if (self->client->ps.fd.forceJumpAddTime >= level.time)
	{
		return;
	}

	//need to play sound
	if (!self->client->ps.fd.forceJumpCharge)
	{
		G_Sound(self, TRACK_CHANNEL_1, G_SoundIndex("sound/weapons/force/jumpbuild.wav"));
	}

	//Increment
	if (self->client->ps.fd.forceJumpAddTime < level.time)
	{
		self->client->ps.fd.forceJumpCharge += forceJumpChargeInterval * 50;
		self->client->ps.fd.forceJumpAddTime = level.time + 500;
	}

	//clamp to max strength for current level
	if (self->client->ps.fd.forceJumpCharge > forceJumpStrength[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]])
	{
		self->client->ps.fd.forceJumpCharge = forceJumpStrength[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]];
		G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);
	}

	//clamp to max available force power
	if (self->client->ps.fd.forceJumpCharge / forceJumpChargeInterval / (FORCE_JUMP_CHARGE_TIME / FRAMETIME) *
		forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]][FP_LEVITATION] > self->client->ps.fd.
		forcePower)
	{
		//can't use more than you have
		G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);
		self->client->ps.fd.forceJumpCharge = self->client->ps.fd.forcePower * forceJumpChargeInterval / (
			FORCE_JUMP_CHARGE_TIME / FRAMETIME);
	}
}

int WP_GetVelocityForForceJump(gentity_t* self, vec3_t jumpVel, const usercmd_t* ucmd)
{
	float pushFwd = 0, pushRt = 0;
	vec3_t view, forward, right;
	VectorCopy(self->client->ps.viewangles, view);
	view[0] = 0;
	AngleVectors(view, forward, right, NULL);
	if (ucmd->forwardmove && ucmd->rightmove)
	{
		if (ucmd->forwardmove > 0)
		{
			pushFwd = 50;
		}
		else
		{
			pushFwd = -50;
		}
		if (ucmd->rightmove > 0)
		{
			pushRt = 50;
		}
		else
		{
			pushRt = -50;
		}
	}
	else if (ucmd->forwardmove || ucmd->rightmove)
	{
		if (ucmd->forwardmove > 0)
		{
			pushFwd = 100;
		}
		else if (ucmd->forwardmove < 0)
		{
			pushFwd = -100;
		}
		else if (ucmd->rightmove > 0)
		{
			pushRt = 100;
		}
		else if (ucmd->rightmove < 0)
		{
			pushRt = -100;
		}
	}

	G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);

	if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
	{
		//short burst
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/jumpsmall.mp3"));
	}
	else
	{
		//holding it
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
	}

	if (self->client->ps.fd.forceJumpCharge < JUMP_VELOCITY + 40)
	{
		//give him at least a tiny boost from just a tap
		self->client->ps.fd.forceJumpCharge = JUMP_VELOCITY + 400;
	}

	if (self->client->ps.velocity[2] < -30)
	{
		//so that we can get a good boost when force jumping in a fall
		self->client->ps.velocity[2] = -30;
	}

	VectorMA(self->client->ps.velocity, pushFwd, forward, jumpVel);
	VectorMA(self->client->ps.velocity, pushRt, right, jumpVel);
	jumpVel[2] += self->client->ps.fd.forceJumpCharge;
	if (pushFwd > 0 && self->client->ps.fd.forceJumpCharge > 200)
	{
		return FJ_FORWARD;
	}
	if (pushFwd < 0 && self->client->ps.fd.forceJumpCharge > 200)
	{
		return FJ_BACKWARD;
	}
	if (pushRt > 0 && self->client->ps.fd.forceJumpCharge > 200)
	{
		return FJ_RIGHT;
	}
	if (pushRt < 0 && self->client->ps.fd.forceJumpCharge > 200)
	{
		return FJ_LEFT;
	}
	return FJ_UP;
}

void ForceJump(gentity_t* self, const usercmd_t* ucmd)
{
	vec3_t jumpVel;

	if (self->client->ps.fd.forcePowerDuration[FP_LEVITATION] > level.time)
	{
		return;
	}
	if (!WP_ForcePowerUsable(self, FP_LEVITATION))
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->s.groundEntityNum == ENTITYNUM_NONE)
	{
		return;
	}
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->pers.botclass == BCLASS_MANDOLORIAN
		|| self->client->pers.botclass == BCLASS_BOBAFETT
		|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
		|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
	{
		Boba_FlyStart(self);
	}

	self->client->fjDidJump = qtrue;

	const float forceJumpChargeInterval = forceJumpStrength[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]] / (
		FORCE_JUMP_CHARGE_TIME / FRAMETIME);

	WP_GetVelocityForForceJump(self, jumpVel, ucmd);

	//FIXME: sound effect
	self->client->ps.fd.forceJumpZStart = self->client->ps.origin[2]; //remember this for when we land
	VectorCopy(jumpVel, self->client->ps.velocity);

	WP_ForcePowerStart(self, FP_LEVITATION,
		self->client->ps.fd.forceJumpCharge / forceJumpChargeInterval / (FORCE_JUMP_CHARGE_TIME /
			FRAMETIME) * forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]][
				FP_LEVITATION]);
	self->client->ps.fd.forceJumpCharge = 0;
	self->client->ps.forceJumpFlip = qtrue;

	// We've just jumped, we're not gonna be on the ground in the following frames.
	// This makes sure npc's wont trigger the ForceJump function multiple times before detecting that they have left the ground.
	self->client->ps.groundEntityNum = ENTITYNUM_NONE;
}

static void WP_AddAsMindtricked(forcedata_t* fd, const int entNum)
{
	if (!fd)
	{
		return;
	}

	if (entNum > 47)
	{
		fd->forceMindtrickTargetIndex4 |= (1 << (entNum - 48));
	}
	else if (entNum > 31)
	{
		fd->forceMindtrickTargetIndex3 |= (1 << (entNum - 32));
	}
	else if (entNum > 15)
	{
		fd->forceMindtrickTargetIndex2 |= (1 << (entNum - 16));
	}
	else
	{
		fd->forceMindtrickTargetIndex |= (1 << entNum);
	}
}

static qboolean ForceTelepathyCheckDirectNPCTarget(gentity_t* self, trace_t* tr, qboolean* tookPower)
{
	qboolean targetLive = qfalse;
	vec3_t tfrom, tto, fwd;
	const float radius = MAX_TRICK_DISTANCE;

	//Check for a direct usage on NPCs first
	VectorCopy(self->client->ps.origin, tfrom);
	tfrom[2] += self->client->ps.viewheight;
	AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
	tto[0] = tfrom[0] + fwd[0] * radius / 2;
	tto[1] = tfrom[1] + fwd[1] * radius / 2;
	tto[2] = tfrom[2] + fwd[2] * radius / 2;

	trap->Trace(tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr->entityNum == ENTITYNUM_NONE
		|| tr->fraction == 1.0f
		|| tr->allsolid
		|| tr->startsolid)
	{
		return qfalse;
	}

	gentity_t* traceEnt = &g_entities[tr->entityNum];

	if (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE)
	{
		return qfalse;
	}

	if (traceEnt && traceEnt->client)
	{
		switch (traceEnt->client->NPC_class)
		{
		case CLASS_GALAKMECH: //cant grip him, he's in armor
		case CLASS_ATST: //much too big to grip!
			//no droids either
		case CLASS_PROBE:
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_PROTOCOL:
		case CLASS_BOBAFETT:
		case CLASS_RANCOR:
			break;
		default:
			targetLive = qtrue;
			break;
		}
	}

	if (traceEnt->s.number < MAX_CLIENTS)
	{
		//a regular client
		return qfalse;
	}

	if (targetLive && traceEnt->NPC)
	{
		//hit an organic non-player
		vec3_t eyeDir;
		if (G_ActivateBehavior(traceEnt, BSET_MINDTRICK))
		{
			//activated a script on him
			//FIXME: do the visual sparkles effect on their heads, still?
			WP_ForcePowerStart(self, FP_TELEPATHY, 0);
		}
		else if (self->NPC && traceEnt->client->playerTeam != self->client->playerTeam
			|| !self->NPC && traceEnt->client->playerTeam != (npcteam_t)self->client->sess.sessionTeam)
		{
			//an enemy
			const int override = 0;
			if (traceEnt->NPC->scriptFlags & SCF_NO_MIND_TRICK)
			{
			}
			else if (traceEnt->s.weapon != WP_SABER && traceEnt->client->NPC_class != CLASS_REBORN)
			{
				//haha!  Jedi aren't easily confused!
				if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_2)
				{
					//turn them to our side
					//if mind trick 3 and aiming at an enemy need more force power
					if (traceEnt->s.weapon != WP_NONE)
					{
						//don't charm people who aren't capable of fighting... like ugnaughts and droids
						int newPlayerTeam, newEnemyTeam;

						if (traceEnt->enemy)
						{
							G_ClearEnemy(traceEnt);
						}
						if (traceEnt->NPC)
						{
							//traceEnt->NPC->tempBehavior = BS_FOLLOW_LEADER;
							traceEnt->client->leader = self;
						}
						//FIXME: maybe pick an enemy right here?
						if (self->NPC)
						{
							//NPC
							newPlayerTeam = self->client->playerTeam;
							newEnemyTeam = self->client->enemyTeam;
						}
						else
						{
							//client/bot
							if (self->client->sess.sessionTeam == TEAM_BLUE)
							{
								//rebel
								newPlayerTeam = NPCTEAM_PLAYER;
								newEnemyTeam = NPCTEAM_ENEMY;
							}
							else if (self->client->sess.sessionTeam == TEAM_RED)
							{
								//imperial
								newPlayerTeam = NPCTEAM_ENEMY;
								newEnemyTeam = NPCTEAM_PLAYER;
							}
							else
							{
								//neutral - wasn't attack anyone
								newPlayerTeam = NPCTEAM_NEUTRAL;
								newEnemyTeam = NPCTEAM_NEUTRAL;
							}
						}
						//store these for retrieval later
						traceEnt->genericValue1 = traceEnt->client->playerTeam;
						traceEnt->genericValue2 = traceEnt->client->enemyTeam;
						traceEnt->genericValue3 = traceEnt->s.teamowner;
						//set the new values
						traceEnt->client->playerTeam = newPlayerTeam;
						traceEnt->client->enemyTeam = newEnemyTeam;
						traceEnt->s.teamowner = newPlayerTeam;
						traceEnt->NPC->charmedTime = level.time + mindTrickTime[self->client->ps.fd.forcePowerLevel[
							FP_TELEPATHY]];

						G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_TORSO, BOTH_SONICPAIN_HOLD,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD, 0);
					}
				}
				else
				{
					//just confuse them
					//somehow confuse them?  Set don't fire to true for a while?  Drop their aggression?  Maybe just take their enemy away and don't let them pick one up for a while unless shot?
					traceEnt->NPC->confusionTime = level.time + mindTrickTime[self->client->ps.fd.forcePowerLevel[
						FP_TELEPATHY]]; //confused for about 10 seconds

					G_SetAnim(traceEnt, &traceEnt->client->pers.cmd, SETANIM_TORSO, BOTH_SONICPAIN_HOLD,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD, 0);

					NPC_PlayConfusionSound(traceEnt);
					if (traceEnt->enemy)
					{
						G_ClearEnemy(traceEnt);
					}
				}
			}
			else
			{
				NPC_Jedi_PlayConfusionSound(traceEnt);
			}
			WP_ForcePowerStart(self, FP_TELEPATHY, override);
		}
		else if (traceEnt->client->playerTeam == self->client->playerTeam)
		{
			//an ally
			//maybe just have him look at you?  Respond?  Take your enemy?
			if (traceEnt->client->ps.pm_type < PM_DEAD && traceEnt->NPC != NULL && !(traceEnt->NPC->scriptFlags &
				SCF_NO_RESPONSE))
			{
				NPC_UseResponse(traceEnt, self, qfalse);
				WP_ForcePowerStart(self, FP_TELEPATHY, 1);
			}
		} //NOTE: no effect on TEAM_NEUTRAL?
		AngleVectors(traceEnt->client->renderInfo.eyeAngles, eyeDir, NULL, NULL);
		VectorNormalize(eyeDir);
		G_PlayEffectID(G_EffectIndex("force/force_touch"), traceEnt->client->renderInfo.eyePoint, eyeDir);
	}
	else
	{
		if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_1 && tr->fraction * 2048 > 64)
		{
			//don't create a diversion less than 64 from you of if at power level 1
			//use distraction anim instead
			G_PlayEffectID(G_EffectIndex("force/force_touch"), tr->endpos, tr->plane.normal);
			AddSoundEvent(self, tr->endpos, 512, AEL_SUSPICIOUS, qtrue, qtrue);
			AddSightEvent(self, tr->endpos, 512, AEL_SUSPICIOUS, 50);
			WP_ForcePowerStart(self, FP_TELEPATHY, 0);
			*tookPower = qtrue;
		}
	}
	self->client->ps.saberBlocked = BLOCKED_NONE;
	self->client->ps.weaponTime = 1000;
	return qtrue;
}

const int STASIS_TIME = 5000;
const int STASISJEDI_TIME = 2500;

void ForceTelepathy(gentity_t* self)
{
	trace_t tr;
	vec3_t tto;
	vec3_t mins, maxs, fwdangles, forward, right, center;
	float vision_arc = 0;
	float radius = MAX_TRICK_DISTANCE;
	qboolean took_power = qfalse;

	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (self->client->ps.weapon == WP_TURRET)
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

	if (self->client->ps.powerups[PW_REDFLAG] ||
		self->client->ps.powerups[PW_BLUEFLAG])
	{
		//can't mind trick while carrying the flag
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		self->client->ps.fd.forcePowersActive & 1 << FP_TELEPATHY)
	{
		WP_ForcePowerStop(self, FP_TELEPATHY);
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_TELEPATHY))
	{
		return;
	}

	BG_ClearRocketLock(&self->client->ps);

	if (ForceTelepathyCheckDirectNPCTarget(self, &tr, &took_power))
	{
		//hit an NPC directly
		self->client->ps.forceAllowDeactivateTime = level.time + 1500;
		G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav"));
		self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
		self->client->ps.forceHandExtendTime = level.time + 1000;
		return;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_2)
	{
		vision_arc = 180;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_3)
	{
		vision_arc = 360;
		radius = MAX_TRICK_DISTANCE * 2.0f;
	}

	VectorCopy(self->client->ps.viewangles, fwdangles);
	AngleVectors(fwdangles, forward, right, NULL);
	VectorCopy(self->client->ps.origin, center);

	for (int i = 0; i < 3; i++)
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_1)
	{
		if (tr.fraction != 1.0 &&
			tr.entityNum != ENTITYNUM_NONE &&
			g_entities[tr.entityNum].inuse &&
			g_entities[tr.entityNum].client &&
			g_entities[tr.entityNum].client->pers.connected &&
			g_entities[tr.entityNum].client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			WP_AddAsMindtricked(&self->client->ps.fd, tr.entityNum);
			if (!took_power)
			{
				WP_ForcePowerStart(self, FP_TELEPATHY, 0);
			}

			G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav"));

			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 1000;

			return;
		}
		return;
	}
	int entity_list[MAX_GENTITIES];
	int e = 0;
	qboolean gotatleastone = qfalse;
	vec3_t dir = { 0, 0, 1 };

	const int num_listed_entities = trap->EntitiesInBox(mins, maxs, entity_list, MAX_GENTITIES);

	while (e < num_listed_entities)
	{
		gentity_t* ent = &g_entities[entity_list[e]];

		if (ent)
		{
			vec3_t a;
			vec3_t thispush_org;
			//not in the arc, don't consider it
			if (ent->client)
			{
				VectorCopy(ent->client->ps.origin, thispush_org);
			}
			else
			{
				VectorCopy(ent->s.pos.trBase, thispush_org);
			}
			VectorCopy(self->client->ps.origin, tto);
			tto[2] += self->client->ps.viewheight;
			VectorSubtract(thispush_org, tto, a);
			vectoangles(a, a);

			if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_2 && ent->s.eType == ET_MISSILE && ent->s.eType != TR_STATIONARY)
			{
				vec3_t dir2_me;
				VectorSubtract(self->r.currentOrigin, ent->r.currentOrigin, dir2_me);
				const float missilemovement = DotProduct(ent->s.pos.trDelta, dir2_me);

				if (missilemovement >= 0)
				{
					G_StasisMissile(self, ent, forward);
				}
			}
			if (!ent->client)
			{
				entity_list[e] = ENTITYNUM_NONE;
			}
			else if (!in_field_of_vision(self->client->ps.viewangles, vision_arc, a))
			{
				//only bother with arc rules if the victim is a client
				entity_list[e] = ENTITYNUM_NONE;
			}
			else if (!ForcePowerUsableOn(self, ent, FP_TELEPATHY))
			{
				entity_list[e] = ENTITYNUM_NONE;
			}
			else if (OnSameTeam(self, ent))
			{
				entity_list[e] = ENTITYNUM_NONE;
			}
		}
		ent = &g_entities[entity_list[e]];

		if (ent && ent != self && ent->client)
		{
			gotatleastone = qtrue;
			WP_AddAsMindtricked(&self->client->ps.fd, ent->s.number);

			if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_2 && ent->s.weapon != WP_SABER)
			{
				ent->client->frozenTime = level.time + STASIS_TIME;
				ent->client->ps.userInt3 |= 1 << FLAG_FROZEN;
				ent->client->ps.userInt1 |= LOCK_UP;
				ent->client->ps.userInt1 |= LOCK_DOWN;
				ent->client->ps.userInt1 |= LOCK_RIGHT;
				ent->client->ps.userInt1 |= LOCK_LEFT;
				ent->client->viewLockTime = level.time + STASIS_TIME;
				ent->client->ps.legsTimer = ent->client->ps.torsoTimer = level.time + STASIS_TIME;
				G_SetAnim(ent, NULL, SETANIM_BOTH, WeaponReadyAnim[ent->client->ps.weapon],
					SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 500);
				G_AddEvent(ent, EV_STASIS, DirToByte(dir));
				G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/stasis.wav"));
				ent->client->ps.saber_move = ent->client->ps.saberBounceMove = LS_READY;
				//don't finish whatever saber anim you may have been in
				ent->client->ps.saberBlocked = BLOCKED_NONE;
				player_Freeze(ent);
			}
			else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_2 && ent->s.weapon == WP_SABER &&
				ent->client->ps.fd.forcePower <= 75)
			{
				ent->client->frozenTime = level.time + STASISJEDI_TIME;
				ent->client->ps.userInt3 |= 1 << FLAG_FROZEN;
				ent->client->ps.userInt1 |= LOCK_UP;
				ent->client->ps.userInt1 |= LOCK_DOWN;
				ent->client->ps.userInt1 |= LOCK_RIGHT;
				ent->client->ps.userInt1 |= LOCK_LEFT;
				ent->client->viewLockTime = level.time + STASISJEDI_TIME;
				ent->client->ps.legsTimer = ent->client->ps.torsoTimer = level.time + STASISJEDI_TIME;
				G_SetAnim(ent, NULL, SETANIM_BOTH, WeaponReadyAnim[ent->client->ps.weapon],
					SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 300);
				G_AddEvent(ent, EV_STASIS, DirToByte(dir));
				G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/stasis.wav"));
				ent->client->ps.saber_move = ent->client->ps.saberBounceMove = LS_READY;
				//don't finish whatever saber anim you may have been in
				ent->client->ps.saberBlocked = BLOCKED_NONE;
				player_Freeze(ent);
			}
			else
			{
				if (ent->client->ps.userInt3 |= 1 << FLAG_FROZEN)
				{
					ent->client->ps.userInt3 &= ~(1 << FLAG_FROZEN);
				}
				Player_CheckFreeze(ent);
				G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_TORSO, BOTH_SONICPAIN_HOLD,
					SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD, 0);
			}
		}
		e++;
	}

	if (gotatleastone)
	{
		self->client->ps.forceAllowDeactivateTime = level.time + 1500;

		if (!took_power)
		{
			WP_ForcePowerStart(self, FP_TELEPATHY, 0);
		}

		G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav"));

		self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
		self->client->ps.forceHandExtendTime = level.time + 1000;
	}
}

void GEntity_UseFunc(gentity_t* self, gentity_t* other, gentity_t* activator)
{
	GlobalUse(self, other, activator);
}

qboolean PM_KnockDownAnimExtended(int anim);
extern qboolean PM_StaggerAnim(int anim);
extern qboolean PM_InKnockDown(const playerState_t* ps);

static qboolean playeris_resisting_force_throw(const gentity_t* player, gentity_t* attacker)
{
	if (player->health <= 0)
	{
		return qfalse;
	}

	if (player->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1)
	{
		return qfalse;
	}

	if (!player->client)
	{
		return qfalse;
	}

	if (player->client->ps.fd.forceRageRecoveryTime >= level.time)
	{
		return qfalse;
	}

	//wasn't trying to grip/drain anyone
	if (player->client->ps.torsoAnim == BOTH_FORCEGRIP_HOLD ||
		player->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START ||
		player->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_HOLD)
	{
		return qfalse;
	}

	//on the ground
	if (player->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		return qfalse;
	}

	//not knocked down already
	if (PM_InKnockDown(&player->client->ps))
	{
		return qfalse;
	}

	if (PM_KnockDownAnimExtended(player->client->ps.legsAnim) || PM_KnockDownAnimExtended(player->client->ps.torsoAnim)
		|| PM_StaggerAnim(player->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (PM_SaberInKata(player->client->ps.saber_move) || PM_InKataAnim(player->client->ps.torsoAnim))
	{
		//don't throw saber when in special attack (alt+attack)
		return qfalse;
	}

	//not involved in a saberLock
	if (player->client->ps.saberLockTime >= level.time)
	{
		return qfalse;
	}

	//not attacking or otherwise busy
	if (player->client->ps.weaponTime >= level.time)
	{
		return qfalse;
	}

	if (!WP_ForcePowerUsable(player, FP_ABSORB))
	{
		return qfalse;
	}

	if (manual_forceblocking(player))
	{
		// player was pushing, or player's force push/pull is high enough to try to stop me
		if (in_front(attacker->r.currentOrigin, player->client->renderInfo.eyePoint, player->client->ps.viewangles,
			0.3f))
		{
			//I'm in front of player
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean ShouldPlayerResistForceThrow(const gentity_t* self, const gentity_t* thrower, const qboolean pull)
{
	int power_use;

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return 0;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return 0;
	}

	if (self->health <= 0)
	{
		return 0;
	}

	if (self->client->ps.powerups[PW_DISINT_4] > level.time)
	{
		return 0;
	}

	if (PM_KnockDownAnimExtended(self->client->ps.legsAnim) || PM_KnockDownAnimExtended(self->client->ps.torsoAnim))
	{
		return 0;
	}

	if (PM_SaberInKata(self->client->ps.saber_move))
	{
		//don't throw saber when in special attack (alt+attack)
		return 0;
	}

	if (!WP_CounterForce(thrower, self, pull ? FP_PULL : FP_PUSH))
	{
		//wasn't able to counter due to generic counter issue
		return 0;
	}

	if (!WP_ForcePowerUsable(self, FP_ABSORB))
	{
		return 0;
	}

	if (!in_front(thrower->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.3f))
	{
		//not facing the attacker
		return 0;
	}

	if (self->client->ps.weaponstate == WEAPON_CHARGING ||
		self->client->ps.weaponstate == WEAPON_CHARGING_ALT)
	{
		//don't auto defend when charging a weapon
		return 0;
	}

	if (level.gametype == GT_SIEGE &&
		pull &&
		thrower && thrower->client)
	{
		//in siege, pull will affect people if they are not facing you, so they can't run away so much
		vec3_t d;

		VectorSubtract(thrower->client->ps.origin, self->client->ps.origin, d);
		vectoangles(d, d);

		const float a = AngleSubtract(d[YAW], self->client->ps.viewangles[YAW]);

		if (a > 60.0f || a < -60.0f)
		{
			//if facing more than 60 degrees away they cannot defend
			return 0;
		}
	}

	if (pull)
	{
		power_use = FP_PULL;
	}
	else
	{
		power_use = FP_PUSH;
	}

	if (!WP_ForcePowerUsable(self, power_use))
	{
		return 0;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//you cannot counter a push/pull if you're in the air
		return 0;
	}

	self->client->ps.powerups[PW_MEDITATE] = level.time + self->client->ps.torsoTimer + 3000;
	return 1;
}

qboolean G_InGetUpAnim(const playerState_t* ps)
{
	switch (ps->legsAnim)
	{
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

	switch (ps->torsoAnim)
	{
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

void G_LetGoOfWall(const gentity_t* ent)
{
	if (!ent || !ent->client)
	{
		return;
	}
	ent->client->ps.pm_flags &= ~PMF_STUCK_TO_WALL;
	if (PM_InReboundJump(ent->client->ps.legsAnim)
		|| PM_InReboundHold(ent->client->ps.legsAnim))
	{
		ent->client->ps.legsTimer = 0;
	}
	if (PM_InReboundJump(ent->client->ps.torsoAnim)
		|| PM_InReboundHold(ent->client->ps.torsoAnim))
	{
		ent->client->ps.torsoTimer = 0;
	}
}

float forcePushPullRadius[NUM_FORCE_POWER_LEVELS] =
{
	0, //none
	384, //256,
	448, //384,
	512
};

void WP_ResistForcePush(gentity_t* self, const gentity_t* pusher, const qboolean no_penalty)
{
	int parts;
	qboolean runningResist = qfalse;

	if (!self || self->health <= 0 || !self->client || !pusher || !pusher->client)
	{
		return;
	}

	if (PM_InKnockDown(&self->client->ps))
	{
		return;
	}

	if (PM_KnockDownAnimExtended(self->client->ps.legsAnim) || PM_KnockDownAnimExtended(self->client->ps.torsoAnim) ||
		PM_StaggerAnim(self->client->ps.torsoAnim))
	{
		return;
	}

	if (PM_SaberInKata(self->client->ps.saber_move) || PM_InKataAnim(self->client->ps.torsoAnim))
	{
		//don't throw saber when in special attack (alt+attack)
		return;
	}

	if (!PM_SaberCanInterruptMove(self->client->ps.saber_move, self->client->ps.torsoAnim))
	{
		//can't interrupt my current torso anim/sabermove with this, so ignore it entirely!
		return;
	}

	if ((!self->s.number
		|| self->NPC && self->NPC->aiFlags & NPCAI_BOSS_CHARACTER
		|| self->client && self->client->NPC_class == CLASS_SHADOWTROOPER)
		&& (VectorLengthSquared(self->client->ps.velocity) > 10000 || self->client->ps.fd.forcePowerLevel[FP_PUSH] >=
			FORCE_LEVEL_3 || self->client->ps.fd.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3))
	{
		runningResist = qtrue;
	}

	if (!runningResist
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
		&& !PM_SpinningSaberAnim(self->client->ps.legsAnim)
		&& !PM_FlippingAnim(self->client->ps.legsAnim)
		&& !PM_RollingAnim(self->client->ps.legsAnim)
		&& !PM_InKnockDown(&self->client->ps)
		&& !PM_CrouchAnim(self->client->ps.legsAnim))
	{
		//if on a surface and not in a spin or flip, play full body resist
		parts = SETANIM_BOTH;
	}
	else
	{
		//play resist just in torso
		parts = SETANIM_TORSO;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_2)
	{
		G_SetAnim(self, &self->client->pers.cmd, parts, BOTH_RESISTPUSH, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
	}
	else if (self->client->saber[0].type == SABER_YODA)
	{
		G_SetAnim(self, &self->client->pers.cmd, parts, BOTH_YODA_RESISTFORCE,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
	}
	else
	{
		G_SetAnim(self, &self->client->pers.cmd, parts, BOTH_RESISTPUSH2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
	}
	self->client->ps.powerups[PW_MEDITATE] = level.time + self->client->ps.torsoTimer + 1000;

	if (!no_penalty)
	{
		char buf[128];

		trap->Cvar_VariableStringBuffer("timescale", buf, sizeof buf);

		const float t_f_val = atof(buf);

		if (!runningResist)
		{
			VectorClear(self->client->ps.velocity);
			self->client->ps.weaponTime = 1000;
			if (self->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
			{
				self->client->ps.weaponTime = floor(self->client->ps.weaponTime * t_f_val);
			}
			self->client->ps.pm_time = self->client->ps.weaponTime;
			self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			//play the full body push effect on me
			self->forcePushTime = level.time + 600; // let the push effect last for 600 ms
		}
		else
		{
			self->client->ps.weaponTime = 600;
			if (self->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
			{
				self->client->ps.weaponTime = floor(self->client->ps.weaponTime * t_f_val);
			}
		}
	}
	//play my force push effect on my hand
	self->client->ps.powerups[PW_DISINT_4] = level.time + self->client->ps.torsoTimer + 500;
	self->client->ps.powerups[PW_PULL] = 0;
	Jedi_PlayBlockedPushSound(self);
}

static void RepulseDamage(gentity_t* self, gentity_t* enemy, vec3_t location, const int damageLevel)
{
	switch (damageLevel)
	{
	case FORCE_LEVEL_1:
		G_Damage(enemy, self, self, NULL, location, 20, DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK,
			MOD_DISRUPTOR_SNIPER);
		break;
	case FORCE_LEVEL_2:
		G_Damage(enemy, self, self, NULL, location, 40, DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK,
			MOD_DISRUPTOR_SNIPER);
		break;
	case FORCE_LEVEL_3:
		G_Damage(enemy, self, self, NULL, location, 60, DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK,
			MOD_DISRUPTOR_SNIPER);
		break;
	default:
		break;
	}

	if (enemy->client->ps.stats[STAT_HEALTH] <= 0)
	{
		VectorClear(enemy->client->ps.lastHitLoc);
		VectorClear(enemy->client->ps.velocity);

		enemy->client->ps.eFlags |= EF_DISINTEGRATION;
		enemy->r.contents = 0;

		enemy->think = G_FreeEntity;
		enemy->nextthink = level.time;
	}
}

static void PushDamage(gentity_t* self, gentity_t* enemy, vec3_t location, const int damageLevel)
{
	switch (damageLevel)
	{
	case FORCE_LEVEL_1:
		G_Damage(enemy, self, self, NULL, location, 10, DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK, MOD_UNKNOWN);
		break;
	case FORCE_LEVEL_2:
		G_Damage(enemy, self, self, NULL, location, 15, DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK, MOD_UNKNOWN);
		break;
	case FORCE_LEVEL_3:
		G_Damage(enemy, self, self, NULL, location, 20, DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK, MOD_UNKNOWN);
		break;
	default:
		break;
	}
}

void ForceThrow(gentity_t* self, qboolean pull)
{
	//shove things in front of you away
	float dist;
	gentity_t* ent;
	int entity_list[MAX_GENTITIES];
	gentity_t* push_target[MAX_GENTITIES];
	int num_listed_entities;
	vec3_t mins, maxs;
	vec3_t v;
	int i, e;
	int ent_count = 0;
	int radius = 1024; //since it's view-based now. //350;
	int power_level;
	int vision_arc;
	int push_power;
	vec3_t center, ent_org, size, forward, right, dir, fwdangles = { 0 };
	float dot1;
	float cone;
	trace_t tr;
	vec3_t thispush_org;
	vec3_t tfrom, tto, fwd;
	int power_use = 0;
	qboolean i_grip = qfalse;
	int damage_level = FORCE_LEVEL_0;

	saberInfo_t* saber1 = BG_MySaber(self->clientNum, 0);

	vision_arc = 0;

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE && (self->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN
		|| !G_InGetUpAnim(&self->client->ps)) && !(self->client->ps.fd.forcePowersActive & 1 << FP_GRIP))
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->health <= 0)
	{
		return;
	}

	if (PM_SaberInKata(self->client->ps.saber_move))
	{
		return;
	}

	if (pm_saber_innonblockable_attack(self->client->ps.torsoAnim))
	{
		return;
	}

	if (PM_InGetUp(&self->client->ps)
		|| PM_InForceGetUp(&self->client->ps)
		|| PM_InKnockDown(&self->client->ps)
		|| PM_KnockDownAnim(self->client->ps.legsAnim)
		|| PM_KnockDownAnim(self->client->ps.torsoAnim)
		|| PM_StaggerAnim(self->client->ps.torsoAnim))
	{
		return;
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		if (pull || self->client->ps.fd.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3)
		{
			//this can be a way to break out
			return;
		}
		//else, I'm breaking my half of the saberlock
		self->client->ps.saberLockTime = 0;
		self->client->ps.saberLockEnemy = ENTITYNUM_NONE;
	}

	if (pull)
	{
		power_use = FP_PULL;
		cone = forcePullCone[self->client->ps.fd.forcePowerLevel[FP_PULL]];
	}
	else
	{
		power_use = FP_PUSH;
		cone = forcePushCone[self->client->ps.fd.forcePowerLevel[FP_PUSH]];
	}

	if (!WP_ForcePowerUsable(self, power_use))
	{
		return;
	}

	if (self->r.svFlags & SVF_BOT && self->client->ps.weapon == WP_SABER) //npc force use limit
	{
		if (self->client->ps.fd.blockPoints < 75 || self->client->ps.fd.forcePower < 75)
		{
			return;
		}
	}

	if (self->client->ps.fd.forcePowersActive & 1 << FP_GRIP)
	{
		WP_ForcePowerStop(self, FP_GRIP);
		i_grip = qtrue;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE && self->client->ps.fd.forcePowerLevel[FP_PUSH] >
		FORCE_LEVEL_2
		&& (self->s.weapon == WP_MELEE ||
			self->s.weapon == WP_NONE ||
			self->s.weapon == WP_SABER && self->client->ps.saberHolstered))
	{
		damage_level = FORCE_LEVEL_3;
	}
	else if (self->client->ps.groundEntityNum == ENTITYNUM_NONE && self->client->ps.fd.forcePowerLevel[FP_PUSH] <
		FORCE_LEVEL_3
		&& (self->s.weapon == WP_MELEE ||
			self->s.weapon == WP_NONE ||
			self->s.weapon == WP_SABER && self->client->ps.saberHolstered))
	{
		damage_level = FORCE_LEVEL_2;
	}
	else
	{
		damage_level = FORCE_LEVEL_1;
	}

	if (!pull && self->client->ps.saberLockTime > level.time && self->client->ps.saberLockFrame)
	{
		if (saber1 && saber1
			->
			type == SABER_UNSTABLE //saber kylo
			|| saber1 && saber1->type == SABER_STAFF_UNSTABLE
			|| saber1 && saber1->type == SABER_STAFF_MAUL
			|| saber1 && saber1->type == SABER_BACKHAND
			|| saber1 && saber1->type == SABER_ASBACKHAND
			|| saber1 && saber1->type == SABER_ANAKIN
			|| saber1 && saber1->type == SABER_PALP
			|| saber1 && saber1->type == SABER_DOOKU
			|| saber1 && saber1->type == SABER_YODA
			) //saber yoda
		{
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/push.mp3"));
		}
		else
		{
			if (self->client->ps.fd.forcePower < 50)
			{
				G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pushyoda.mp3"));
			}
			else
			{
				G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pushlow.mp3"));
			}
		}
		self->client->ps.powerups[PW_DISINT_4] = level.time + 1500;

		self->client->ps.saberLockHits += self->client->ps.fd.forcePowerLevel[FP_PUSH] * 2;

		WP_ForcePowerStart(self, FP_PUSH, 0);
		return;
	}

	if (self->client->ps.legsAnim == BOTH_KNOCKDOWN3
		|| self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F1 && self->client->ps.torsoTimer > 400
		|| self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F2 && self->client->ps.torsoTimer > 900
		|| self->client->ps.torsoAnim == BOTH_GETUP3 && self->client->ps.torsoTimer > 500
		|| self->client->ps.torsoAnim == BOTH_GETUP4 && self->client->ps.torsoTimer > 300
		|| self->client->ps.torsoAnim == BOTH_GETUP5 && self->client->ps.torsoTimer > 500)
	{
		//we're face-down, so we'd only be force-push/pulling the floor
		return;
	}

	WP_ForcePowerStart(self, power_use, 0);

	//make sure this plays and that you cannot press fire for about 1 second after this
	if (pull)
	{
		//RACC - force pull
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pull.wav"));
		if (self->client->ps.forceHandExtend == HANDEXTEND_NONE)
		{
			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPULL;
			self->client->ps.forceHandExtendTime = level.time + 600;
		}
		self->client->ps.powerups[PW_DISINT_4] = self->client->ps.forceHandExtendTime + 200;
		self->client->ps.powerups[PW_PULL] = self->client->ps.powerups[PW_DISINT_4];
	}
	else
	{
		//RACC - Force Push
		if (self->client->ps.weapon == WP_MELEE ||
			self->client->ps.weapon == WP_NONE ||
			self->client->ps.weapon == WP_SABER && self->client->ps.saberHolstered)
		{
			//2-handed PUSH
			if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/repulsepush.wav"));
			}
			else
			{
				G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pushhard.mp3"));
			}
		}
		else if (saber1 && saber1
			->
			type == SABER_YODA
			) //saber yoda
		{
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pushyoda.mp3"));
		}
		else
			if (saber1 && saber1
				->
				type == SABER_UNSTABLE //saber kylo
				|| saber1 && saber1->type == SABER_STAFF_UNSTABLE
				|| saber1 && saber1->type == SABER_STAFF_MAUL
				|| saber1 && saber1->type == SABER_BACKHAND
				|| saber1 && saber1->type == SABER_ASBACKHAND
				|| saber1 && saber1->type == SABER_ANAKIN
				|| saber1 && saber1->type == SABER_PALP
				|| saber1 && saber1->type == SABER_DOOKU
				) //saber yoda
			{
				G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/push.mp3"));
			}
			else
				if (self->client->ps.fd.forcePower < 30 || PM_InKnockDown(&self->client->ps))
				{
					G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pushyoda.mp3"));
				}
				else
				{
					G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pushlow.mp3"));
				}
		if (self->client->ps.forceHandExtend == HANDEXTEND_NONE)
		{
			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 650;
		}
		else if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN && G_InGetUpAnim(&self->client->ps))
		{
			//RACC - Force Pushed while in get up animation.
			if (self->client->ps.forceDodgeAnim > 4)
			{
				self->client->ps.forceDodgeAnim -= 8;
			}
			self->client->ps.forceDodgeAnim += 8;
			//special case, play push on upper torso, but keep playing current knockdown anim on legs
		}
		self->client->ps.powerups[PW_DISINT_4] = level.time + 1100;
		self->client->ps.powerups[PW_PULL] = 0;
	}

	VectorCopy(self->client->ps.viewangles, fwdangles);
	AngleVectors(fwdangles, forward, right, NULL);
	VectorCopy(self->client->ps.origin, center);

	for (i = 0; i < 3; i++)
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	if (pull)
	{
		power_level = self->client->ps.fd.forcePowerLevel[FP_PULL];
		push_power = 256 * self->client->ps.fd.forcePowerLevel[FP_PULL];
	}
	else
	{
		power_level = self->client->ps.fd.forcePowerLevel[FP_PUSH];
		push_power = 256 * self->client->ps.fd.forcePowerLevel[FP_PUSH];
	}

	if (!power_level)
	{
		//Shouldn't have made it here..
		return;
	}

	if (power_level == FORCE_LEVEL_2)
	{
		vision_arc = 60;
	}
	else if (power_level == FORCE_LEVEL_3)
	{
		vision_arc = 180;
	}

	if (power_level == FORCE_LEVEL_1)
	{
		//can only push/pull targeted things at level 1
		VectorCopy(self->client->ps.origin, tfrom);
		tfrom[2] += self->client->ps.viewheight;
		AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
		tto[0] = tfrom[0] + fwd[0] * radius / 2;
		tto[1] = tfrom[1] + fwd[1] * radius / 2;
		tto[2] = tfrom[2] + fwd[2] * radius / 2;

		trap->Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID | CONTENTS_TRIGGER, qfalse, 0, 0);

		if (tr.fraction != 1.0 &&
			tr.entityNum != ENTITYNUM_NONE)
		{
			if (!g_entities[tr.entityNum].client && g_entities[tr.entityNum].s.eType == ET_NPC)
			{
				//g2animent
				if (g_entities[tr.entityNum].s.genericenemyindex < level.time)
				{
					g_entities[tr.entityNum].s.genericenemyindex = level.time + 2000;
				}
			}
			num_listed_entities = 0;
			entity_list[num_listed_entities] = tr.entityNum;

			if (pull)
			{
				if (!ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_PULL))
				{
					return;
				}
			}
			else
			{
				if (!ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_PUSH))
				{
					return;
				}
			}

			// if a CONTENTS_TRIGGER was traced, must be an item
			if (g_entities[tr.entityNum].r.contents == CONTENTS_TRIGGER && g_entities[tr.entityNum].s.eType != ET_ITEM)
				return;

			num_listed_entities++;
		}
		else
		{
			//didn't get anything, so just
			return;
		}

		//now check for brush based force moveable objects.
		{
			int i1;
			int func_num;
			int func_list[MAX_GENTITIES];
			gentity_t* funcEnt = NULL;

			func_num = trap->EntitiesInBox(mins, maxs, func_list, MAX_GENTITIES);

			if (num_listed_entities <= 0)
				return;

			for (i1 = 0; i1 < func_num; i1++)
			{
				funcEnt = &g_entities[func_list[i1]];
				if (!funcEnt->client && funcEnt->s.number != tr.entityNum)
				{
					//we have one, add it to the actual push list.
					entity_list[num_listed_entities] = funcEnt->s.number;
					num_listed_entities++;
				}
			}
		}
	}
	else
	{
		gentity_t* aiming_at;
		num_listed_entities = trap->EntitiesInBox(mins, maxs, entity_list, MAX_GENTITIES);

		e = 0;

		while (e < num_listed_entities)
		{
			ent = &g_entities[entity_list[e]];

			if (!ent->client && ent->s.eType == ET_NPC)
			{
				//g2animent
				if (ent->s.genericenemyindex < level.time)
				{
					ent->s.genericenemyindex = level.time + 2000;
				}
			}
			if (Q_stricmp(ent->classname, "sentryGun") == 0)
			{
				VectorCopy(self->client->ps.origin, tfrom);
				tfrom[2] += self->client->ps.viewheight;
				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				tto[0] = tfrom[0] + fwd[0] * radius / 2;
				tto[1] = tfrom[1] + fwd[1] * radius / 2;
				tto[2] = tfrom[2] + fwd[2] * radius / 2;

				trap->Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

				if (tr.fraction != 1.0 &&
					tr.entityNum != ENTITYNUM_NONE)
				{
					if (!g_entities[tr.entityNum].client && g_entities[tr.entityNum].s.eType == ET_NPC)
					{
						//g2animent
						if (g_entities[tr.entityNum].s.genericenemyindex < level.time)
						{
							g_entities[tr.entityNum].s.genericenemyindex = level.time + 2000;
						}
					}
					aiming_at = &g_entities[tr.entityNum];
					if (ent == aiming_at) //Looking at the sentry...so deactivate for 10 seconds
						ent->nextthink = level.time + 5000;
				}
			}

			if (ent)
			{
				if (ent->client)
				{
					VectorCopy(ent->client->ps.origin, thispush_org);
				}
				else
				{
					VectorCopy(ent->s.pos.trBase, thispush_org);
				}
			}

			if (ent)
			{
				vec3_t a;
				//not in the arc, don't consider it
				VectorCopy(self->client->ps.origin, tto);
				tto[2] += self->client->ps.viewheight;
				VectorSubtract(thispush_org, tto, a);
				vectoangles(a, a);

				if (ent->client && !in_field_of_vision(self->client->ps.viewangles, vision_arc, a) &&
					ForcePowerUsableOn(self, ent, power_use))
				{
					//only bother with arc rules if the victim is a client
					entity_list[e] = ENTITYNUM_NONE;
				}
				else if (ent->client)
				{
					if (pull)
					{
						if (!ForcePowerUsableOn(self, ent, FP_PULL))
						{
							entity_list[e] = ENTITYNUM_NONE;
						}
					}
					else
					{
						if (!ForcePowerUsableOn(self, ent, FP_PUSH))
						{
							entity_list[e] = ENTITYNUM_NONE;
						}
					}
				}
			}
			e++;
		}
	}

	//REPULSE ########################################################################## IN THE AIR PUSH
	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE
		&& (self->s.weapon == WP_MELEE || self->s.weapon == WP_NONE || self->s.weapon == WP_SABER && BG_SabersOff(
			&self->client->ps)))
	{
		if (pull)
		{
			for (e = 0; e < num_listed_entities; e++)
			{
				if (entity_list[e] != ENTITYNUM_NONE &&
					entity_list[e] >= 0 &&
					entity_list[e] < MAX_GENTITIES)
				{
					ent = &g_entities[entity_list[e]];
				}
				else
				{
					ent = NULL;
				}

				if (!ent)
					continue;
				if (ent == self)
					continue;
				if (ent->client && OnSameTeam(ent, self))
				{
					continue;
				}
				if (!ent->inuse)
					continue;
				if (ent->s.eType != ET_MISSILE)
				{
					if (ent->s.eType != ET_ITEM)
					{
						if (Q_stricmp("func_button", ent->classname) == 0)
						{
							//we might push it
							if (pull || !(ent->spawnflags & SPF_BUTTON_FPUSHABLE))
							{
								//not force-pushable, never pullable
								continue;
							}
						}
						else
						{
							if (ent->s.eFlags & EF_NODRAW)
							{
								continue;
							}
							if (!ent->client)
							{
								if (Q_stricmp("lightsaber", ent->classname) != 0)
								{
									//not a lightsaber
									if (Q_stricmp("func_door", ent->classname) != 0 || !(ent->spawnflags & 2))
									{
										//not a force-usable door
										if (Q_stricmp("func_static", ent->classname) != 0 || !(ent->spawnflags & 1
											/*F_PUSH*/) && !(ent->spawnflags & 2))
										{
											//not a force-usable func_static
											if (Q_stricmp("limb", ent->classname))
											{
												//not a limb
												continue;
											}
										}
									}
									else if (ent->moverState != MOVER_POS1 && ent->moverState != MOVER_POS2)
									{
										//not at rest
										continue;
									}
								}
							}
							else if (ent->client->NPC_class == CLASS_GALAKMECH
								|| ent->client->NPC_class == CLASS_ATST
								|| ent->client->NPC_class == CLASS_RANCOR)
							{
								//can't push ATST or Galak or Rancor
								continue;
							}
						}
					}
				}
				else
				{
					if (ent->s.pos.trType == TR_STATIONARY && ent->s.eFlags & EF_MISSILE_STICK)
					{
						//can't force-push/pull stuck missiles (detpacks, tripmines)
						continue;
					}
					if (ent->s.pos.trType == TR_STATIONARY && ent->s.weapon != WP_THERMAL)
					{
						//only thermal detonators can be pushed once stopped
						continue;
					}
				}

				//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
				// find the distance from the edge of the bounding box
				for (i = 0; i < 3; i++)
				{
					if (center[i] < ent->r.absmin[i])
					{
						v[i] = ent->r.absmin[i] - center[i];
					}
					else if (center[i] > ent->r.absmax[i])
					{
						v[i] = center[i] - ent->r.absmax[i];
					}
					else
					{
						v[i] = 0;
					}
				}

				VectorSubtract(ent->r.absmax, ent->r.absmin, size);
				VectorMA(ent->r.absmin, 0.5, size, ent_org);

				VectorSubtract(ent_org, center, dir);
				VectorNormalize(dir);
				if ((dot1 = DotProduct(dir, forward)) < 0.6)
					continue;

				dist = VectorLength(v);

				//Now check and see if we can actually deflect it
				//method1
				//if within a certain range, deflect it
				if (dist >= radius)
				{
					continue;
				}

				//in PVS?
				if (!ent->r.bmodel && !trap->InPVS(ent_org, self->client->ps.origin))
				{
					//must be in PVS
					continue;
				}

				//really should have a clear LOS to this thing...
				trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT,
					qfalse, 0, 0);
				if (tr.fraction < 1.0f && tr.entityNum != ent->s.number)
				{
					//must have clear LOS
					//try from eyes too before you give up
					vec3_t eye_point;
					VectorCopy(self->client->ps.origin, eye_point);
					eye_point[2] += self->client->ps.viewheight;
					trap->Trace(&tr, eye_point, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT, qfalse, 0,
						0);

					if (tr.fraction < 1.0f && tr.entityNum != ent->s.number)
					{
						continue;
					}
				}

				// ok, we are within the radius, add us to the incoming list
				push_target[ent_count] = ent;
				ent_count++;
			}
		}
		else
		{
			for (e = 0; e < num_listed_entities; e++)
			{
				if (entity_list[e] != ENTITYNUM_NONE &&
					entity_list[e] >= 0 &&
					entity_list[e] < MAX_GENTITIES)
				{
					ent = &g_entities[entity_list[e]];
				}
				else
				{
					ent = NULL;
				}

				if (!ent)
					continue;
				if (ent == self)
					continue;
				if (ent->client && OnSameTeam(ent, self))
				{
					continue;
				}
				if (!ent->inuse)
					continue;
				if (ent->s.eType != ET_MISSILE)
				{
					if (g_pushitems.integer || ent->s.eType != ET_ITEM)
					{
						if (Q_stricmp("func_button", ent->classname) == 0)
						{
							//we might push it
							if (pull || !(ent->spawnflags & SPF_BUTTON_FPUSHABLE))
							{
								//not force-pushable, never pullable
								continue;
							}
						}
						else
						{
							if (ent->s.eFlags & EF_NODRAW)
							{
								continue;
							}
							if (!ent->client)
							{
								if (!g_pushitems.integer)
								{
									if (Q_stricmp("lightsaber", ent->classname) != 0)
									{
										//not a lightsaber
										if (Q_stricmp("func_door", ent->classname) != 0 || !(ent->spawnflags & 2))
										{
											//not a force-usable door
											if (Q_stricmp("func_static", ent->classname) != 0 || !(ent->spawnflags & 1)
												&& !(ent->spawnflags & 2))
											{
												//not a force-usable func_static
												if (Q_stricmp("limb", ent->classname))
												{
													//not a limb
													continue;
												}
											}
										}
										else if (ent->moverState != MOVER_POS1 && ent->moverState != MOVER_POS2)
										{
											//not at rest
											continue;
										}
									}
								}
							}
							else if (ent->client->NPC_class == CLASS_GALAKMECH
								|| ent->client->NPC_class == CLASS_ATST
								|| ent->client->NPC_class == CLASS_SBD
								|| ent->client->NPC_class == CLASS_RANCOR
								|| ent->client->NPC_class == CLASS_WAMPA
								|| ent->client->NPC_class == CLASS_SAND_CREATURE)
							{
								//can't push ATST or Galak or Rancor
								continue;
							}
						}
					}
				}
				else
				{
					//RACC - Missiles
					if (ent->s.pos.trType == TR_STATIONARY && ent->s.eFlags & EF_MISSILE_STICK)
					{
						//can't force-push/pull stuck missiles (detpacks, tripmines)
						continue;
					}
					if (ent->s.pos.trType == TR_STATIONARY && ent->s.weapon != WP_THERMAL)
					{
						//only thermal detonators can be pushed once stopped
						continue;
					}
				}

				//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
				// find the distance from the edge of the bounding box
				for (i = 0; i < 3; i++)
				{
					if (center[i] < ent->r.absmin[i])
					{
						v[i] = ent->r.absmin[i] - center[i];
					}
					else if (center[i] > ent->r.absmax[i])
					{
						v[i] = center[i] - ent->r.absmax[i];
					}
					else
					{
						v[i] = 0;
					}
				}

				VectorSubtract(ent->r.absmax, ent->r.absmin, size);
				VectorMA(ent->r.absmin, 0.5, size, ent_org);

				VectorSubtract(ent_org, center, dir);
				VectorNormalize(dir);

				if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					if (cone < 1.0f)
					{
						//must be within the forward cone
						if (ent->client && !pull
							&& ent->client->ps.fd.forceGripentity_num == self->s.number)
						{
							//this is the guy that's force-gripping me, use a wider cone regardless of force power level
							if ((dot1 = DotProduct(dir, forward)) < cone - 0.3f)
								continue;
						}
						else if (ent->s.eType == ET_MISSILE)
						{
							//missiles are easier to force-push, never require direct trace (FIXME: maybe also items and general physics objects)
							if ((dot1 = DotProduct(dir, forward)) < cone - 0.3f)
								continue;
						}
						else if ((dot1 = DotProduct(dir, forward)) < cone)
						{
							continue;
						}
					}
					else if (ent->s.eType == ET_MISSILE)
					{
						//a missile and we're at force level 1... just use a small cone, but not ridiculously small
						if ((dot1 = DotProduct(dir, forward)) < 0.75f)
						{
							continue;
						}
					}
				}

				dist = VectorLength(v);

				if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					//if within a certain range, deflect it
					if (ent->s.eType == ET_MISSILE && cone >= 1.0f)
					{
						//smaller radius on missile checks at force push 1
						if (dist >= 192)
						{
							continue;
						}
					}
					else if (dist >= radius)
					{
						continue;
					}
				}
				else if (dist >= radius)
				{
					continue;
				}

				if (!ent->r.bmodel && !trap->InPVS(ent_org, self->client->ps.origin))
				{
					//must be in PVS
					continue;
				}

				//really should have a clear LOS to this thing...
				trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT,
					qfalse, 0, 0);
				if (tr.fraction < 1.0f && tr.entityNum != ent->s.number)
				{
					//must have clear LOS
					//try from eyes too before you give up
					vec3_t eye_point;
					VectorCopy(self->client->ps.origin, eye_point);
					eye_point[2] += self->client->ps.viewheight;
					trap->Trace(&tr, eye_point, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT, qfalse, 0,
						0);

					if (tr.fraction < 1.0f && tr.entityNum != ent->s.number)
					{
						continue;
					}
				}

				// ok, we are within the radius, add us to the incoming list
				push_target[ent_count] = ent;
				ent_count++;
			}
		}
	}
	else // ON THE GROUND ################################## NORMAL PUSH
	{
		for (e = 0; e < num_listed_entities; e++)
		{
			if (entity_list[e] != ENTITYNUM_NONE &&
				entity_list[e] >= 0 &&
				entity_list[e] < MAX_GENTITIES)
			{
				ent = &g_entities[entity_list[e]];
			}
			else
			{
				ent = NULL;
			}

			if (!ent)
				continue;
			if (ent == self)
				continue;
			if (ent->client && OnSameTeam(ent, self))
			{
				continue;
			}
			if (!ent->inuse)
				continue;
			if (ent->s.eType != ET_MISSILE)
			{
				if (ent->s.eType != ET_ITEM)
				{
					if (Q_stricmp("func_button", ent->classname) == 0)
					{
						//we might push it
						if (pull || !(ent->spawnflags & SPF_BUTTON_FPUSHABLE))
						{
							//not force-pushable, never pullable
							continue;
						}
					}
					else
					{
						if (ent->s.eFlags & EF_NODRAW)
						{
							continue;
						}
						if (!ent->client)
						{
							if (Q_stricmp("lightsaber", ent->classname) != 0)
							{
								//not a lightsaber
								if (Q_stricmp("func_door", ent->classname) != 0 || !(ent->spawnflags & 2))
								{
									//not a force-usable door
									if (Q_stricmp("func_static", ent->classname) != 0 || !(ent->spawnflags & 1) && !(
										ent->spawnflags & 2))
									{
										//not a force-usable func_static
										if (Q_stricmp("limb", ent->classname))
										{
											//not a limb
											continue;
										}
									}
								}
								else if (ent->moverState != MOVER_POS1 && ent->moverState != MOVER_POS2)
								{
									//not at rest
									continue;
								}
							}
						}
						else if (ent->client->NPC_class == CLASS_GALAKMECH
							|| ent->client->NPC_class == CLASS_ATST
							|| ent->client->NPC_class == CLASS_RANCOR)
						{
							//can't push ATST or Galak or Rancor
							continue;
						}
					}
				}
			}
			else
			{
				if (ent->s.pos.trType == TR_STATIONARY && ent->s.eFlags & EF_MISSILE_STICK)
				{
					//can't force-push/pull stuck missiles (detpacks, tripmines)
					continue;
				}
				if (ent->s.pos.trType == TR_STATIONARY && ent->s.weapon != WP_THERMAL)
				{
					//only thermal detonators can be pushed once stopped
					continue;
				}
			}

			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for (i = 0; i < 3; i++)
			{
				if (center[i] < ent->r.absmin[i])
				{
					v[i] = ent->r.absmin[i] - center[i];
				}
				else if (center[i] > ent->r.absmax[i])
				{
					v[i] = center[i] - ent->r.absmax[i];
				}
				else
				{
					v[i] = 0;
				}
			}

			VectorSubtract(ent->r.absmax, ent->r.absmin, size);
			VectorMA(ent->r.absmin, 0.5, size, ent_org);

			VectorSubtract(ent_org, center, dir);
			VectorNormalize(dir);
			if ((dot1 = DotProduct(dir, forward)) < 0.6)
				continue;

			dist = VectorLength(v);

			//Now check and see if we can actually deflect it
			//method1
			//if within a certain range, deflect it
			if (dist >= radius)
			{
				continue;
			}

			//in PVS?
			if (!ent->r.bmodel && !trap->InPVS(ent_org, self->client->ps.origin))
			{
				//must be in PVS
				continue;
			}

			//really should have a clear LOS to this thing...
			trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT,
				qfalse, 0, 0);
			if (tr.fraction < 1.0f && tr.entityNum != ent->s.number)
			{
				//must have clear LOS
				//try from eyes too before you give up
				vec3_t eye_point;
				VectorCopy(self->client->ps.origin, eye_point);
				eye_point[2] += self->client->ps.viewheight;
				trap->Trace(&tr, eye_point, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT, qfalse, 0, 0);

				if (tr.fraction < 1.0f && tr.entityNum != ent->s.number)
				{
					continue;
				}
			}

			// ok, we are within the radius, add us to the incoming list
			push_target[ent_count] = ent;
			ent_count++;
		}
	}

	if (ent_count)
	{
		int x;
		int push_power_mod;
		//method1:
		for (x = 0; x < ent_count; x++)
		{
			int mod_power_level = power_level;

			push_power = 256 * mod_power_level;

			if (push_target[x]->client)
			{
				VectorCopy(push_target[x]->client->ps.origin, thispush_org);
			}
			else
			{
				VectorCopy(push_target[x]->s.origin, thispush_org);
			}

			if (push_target[x]->client)
			{
				vec3_t push_dir;
				qboolean was_wall_grabbing = qfalse;
				qboolean can_pull_weapon = qtrue;
				float dir_len = 0;

				if (push_target[x]->client->ps.pm_flags & PMF_STUCK_TO_WALL)
				{
					//no resistance if stuck to wall
					was_wall_grabbing = qtrue;

					if (PM_InLedgeMove(push_target[x]->client->ps.legsAnim))
					{
						G_LetGoOfLedge(push_target[x]);
					}
					else
					{
						G_LetGoOfWall(push_target[x]);
					}
				}

				push_power_mod = push_power;

				//switched to more logical wasWallGrabbing toggle.
				if (!was_wall_grabbing && ShouldPlayerResistForceThrow(push_target[x], self, pull) ||
					playeris_resisting_force_throw(push_target[x], self)
					&& push_target[x]->client->ps.saberFatigueChainCount < MISHAPLEVEL_HEAVY)
				{
					//racc - player blocked the throw.
					if (pull)
					{
						G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pull.wav"));
						push_target[x]->client->ps.forceHandExtendTime = level.time + 600;
					}
					else
					{
						if (self->client->ps.weapon == WP_MELEE ||
							self->client->ps.weapon == WP_NONE ||
							self->client->ps.weapon == WP_SABER && self->client->ps.saberHolstered)
						{
							//2-handed PUSH
							if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/repulsepush.wav"));
							}
							else
							{
								G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushhard.mp3"));
							}
						}
						else if (saber1 && saber1->type == SABER_YODA) //saber yoda
						{
							G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushyoda.mp3"));
						}
						else
							if (saber1 && saber1->type == SABER_UNSTABLE //saber kylo
								|| saber1 && saber1->type == SABER_STAFF_UNSTABLE
								|| saber1 && saber1->type == SABER_STAFF_MAUL
								|| saber1 && saber1->type == SABER_BACKHAND
								|| saber1 && saber1->type == SABER_ASBACKHAND
								|| saber1 && saber1->type == SABER_ANAKIN
								|| saber1 && saber1->type == SABER_PALP
								|| saber1 && saber1->type == SABER_DOOKU) //saber yoda
							{
								G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/push.mp3"));
							}
							else
							{
								if (self->client->ps.fd.forcePower < 30 || PM_InKnockDown(&self->client->ps))
								{
									G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushyoda.mp3"));
								}
								else
								{
									G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushlow.mp3"));
								}
							}
						push_target[x]->client->ps.forceHandExtendTime = level.time + 650;
					}
					push_target[x]->client->ps.powerups[PW_DISINT_4] = push_target[x]->client->ps.forceHandExtendTime + 200;

					if (pull)
					{
						push_target[x]->client->ps.powerups[PW_PULL] = push_target[x]->client->ps.powerups[PW_DISINT_4];
					}
					else
					{
						push_target[x]->client->ps.powerups[PW_PULL] = 0;
					}

					//Make a counter-throw effect
					WP_ResistForcePush(push_target[x], self, qfalse);
					continue;
				}

				//shove them
				if (pull)
				{
					VectorSubtract(self->client->ps.origin, thispush_org, push_dir);

					if (push_target[x]->client && VectorLength(push_dir) <= 256)
					{
						if (!OnSameTeam(self, push_target[x]) && push_target[x]->client->ps.saberFatigueChainCount <
							MISHAPLEVEL_HEAVY)
						{
							can_pull_weapon = qfalse;
						}
						if (!OnSameTeam(self, push_target[x]) && can_pull_weapon)
						{
							// pull the weapon out of the player's hand.
							VectorCopy(self->client->ps.origin, tfrom);
							tfrom[2] += self->client->ps.viewheight;
							AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
							tto[0] = tfrom[0] + fwd[0] * radius / 2;
							tto[1] = tfrom[1] + fwd[1] * radius / 2;
							tto[2] = tfrom[2] + fwd[2] * radius / 2;

							trap->Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

							if (tr.fraction != 1.0
								&& tr.entityNum == push_target[x]->s.number)
							{
								vec3_t vecnorm;
								vec3_t uorg;
								VectorCopy(self->client->ps.origin, uorg);
								uorg[2] += 64;

								VectorSubtract(uorg, thispush_org, vecnorm);
								VectorNormalize(vecnorm);

								TossClientWeapon(push_target[x], vecnorm, 500);
							}
						}
					}
				}
				else
				{
					VectorSubtract(thispush_org, self->client->ps.origin, push_dir);
				}

				if (!dir_len)
				{
					dir_len = VectorLength(push_dir);
				}

				VectorNormalize(push_dir);

				if (push_target[x]->client)
				{
					//escape a force grip if we're in one
					if (self->client->ps.fd.forceGripBeingGripped > level.time)
					{
						//force the enemy to stop gripping me if I managed to push him
						if (push_target[x]->client->ps.fd.forceGripentity_num == self->s.number)
						{
							WP_ForcePowerStop(push_target[x], FP_GRIP);
							self->client->ps.fd.forceGripBeingGripped = 0;
							push_target[x]->client->ps.fd.forceGripUseTime = level.time + 1000;
							//since we just broke out of it..
						}
					}

					push_target[x]->client->ps.otherKiller = self->s.number;
					push_target[x]->client->ps.otherKillerTime = level.time + 5000;
					push_target[x]->client->ps.otherKillerDebounceTime = level.time + 100;
					push_target[x]->client->otherKillerMOD = MOD_UNKNOWN;
					push_target[x]->client->otherKillerVehWeapon = 0;
					push_target[x]->client->otherKillerWeaponType = WP_NONE;

					push_power_mod -= dir_len * 0.7;

					if (push_power_mod < 100)
					{
						push_power_mod = 100;
					}

					if (push_power_mod > 250)
					{
						//got pushed hard, get knockdown or knocked off a animals or speeders if riding one.
						if (BG_KnockDownable(&push_target[x]->client->ps) && dir_len <= 128.0f)
						{
							//can only do a knockdown if running
							G_Knockdown(push_target[x], self, push_dir, 300, qtrue);
							G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushed.mp3"));

							if (self->client->ps.weapon == WP_MELEE || self->client->ps.weapon == WP_NONE && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE
								|| self->client->ps.weapon == WP_SABER && self->client->ps.saberHolstered && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								RepulseDamage(self, push_target[x], tr.endpos, damage_level);
							}
							else
							{
								if (pull)
								{
									//
								}
								else
								{
									PushDamage(self, push_target[x], tr.endpos, damage_level);
								}
							}
						}
						else if (push_target[x]->client->ps.m_iVehicleNum && dir_len <= 128.0f)
						{
							//a player on a vehicle
							gentity_t* vehEnt = &g_entities[push_target[x]->client->ps.m_iVehicleNum];

							if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
							{
								if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
									vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
								{
									//push the guy off
									vehEnt->m_pVehicle->m_pVehicleInfo->Eject(
										vehEnt->m_pVehicle, (bgEntity_t*)push_target[x], qfalse);
								}
							}
						}
					}
					//full body push effect
					push_target[x]->client->pushEffectTime = level.time + 600;

					if (!pull)
					{
						if (walk_check(push_target[x])
							|| push_target[x]->client->ps.fd.blockPoints >= BLOCKPOINTS_HALF && push_target[x]->client->
							pers.cmd.buttons & BUTTON_BLOCK
							|| push_target[x]->client->ps.BlasterAttackChainCount <= BLASTERMISHAPLEVEL_HEAVYER && !
							BG_IsUsingHeavyWeap(&push_target[x]->client->ps)
							&& in_front(push_target[x]->client->ps.origin, self->client->ps.origin,
								self->client->ps.viewangles, -0.7f))
						{
							WP_ResistForcePush(push_target[x], self, qfalse);
							push_target[x]->client->ps.saber_move = push_target[x]->client->ps.saberBounceMove = LS_READY;
							//don't finish whatever saber anim you may have been in
							push_target[x]->client->ps.saberBlocked = BLOCKED_NONE;
						}
						else if (PM_SaberInBrokenParry(push_target[x]->client->ps.saber_move) && dir_len <= 64.0f)
						{
							//do a knockdown if fairly close
							G_Knockdown(push_target[x], self, push_dir, 300, qtrue);
							G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushed.mp3"));
							push_power_mod = 200;

							if (self->client->ps.weapon == WP_MELEE || self->client->ps.weapon == WP_NONE && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE
								|| self->client->ps.weapon == WP_SABER && self->client->ps.saberHolstered && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								RepulseDamage(self, push_target[x], tr.endpos, damage_level);
							}
							else
							{
								if (pull)
								{
									//
								}
								else
								{
									PushDamage(self, push_target[x], tr.endpos, damage_level);
								}
							}
						}
						else if (BG_KnockDownable(&push_target[x]->client->ps) && dir_len <= 128.0f && !walk_check(
							push_target[x]))
						{
							//can only do a knockdown if fairly close
							G_Knockdown(push_target[x], self, push_dir, 300, qtrue);
							G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushed.mp3"));
							push_power_mod = 250;

							if (self->client->ps.weapon == WP_MELEE || self->client->ps.weapon == WP_NONE && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE
								|| self->client->ps.weapon == WP_SABER && self->client->ps.saberHolstered && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								RepulseDamage(self, push_target[x], tr.endpos, damage_level);
							}
							else
							{
								if (pull)
								{
									//
								}
								else
								{
									PushDamage(self, push_target[x], tr.endpos, damage_level);
								}
							}
						}
						else if (!in_front(push_target[x]->r.currentOrigin, self->r.currentOrigin,
							self->client->ps.viewangles, 0.3f) && !walk_check(push_target[x]))
						{
							G_Knockdown(push_target[x], self, push_dir, 300, qtrue);
							G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushed.mp3"));
							push_power_mod = 200;

							if (self->client->ps.weapon == WP_MELEE || self->client->ps.weapon == WP_NONE && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE
								|| self->client->ps.weapon == WP_SABER && self->client->ps.saberHolstered && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								RepulseDamage(self, push_target[x], tr.endpos, damage_level);
							}
							else
							{
								if (pull)
								{
									//
								}
								else
								{
									PushDamage(self, push_target[x], tr.endpos, damage_level);
								}
							}
						}
						else
						{
							G_Knockdown(push_target[x], self, push_dir, 300, qtrue);
							push_power_mod = 200;
							G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushed.mp3"));

							if (self->client->ps.weapon == WP_MELEE || self->client->ps.weapon == WP_NONE && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE
								|| self->client->ps.weapon == WP_SABER && self->client->ps.saberHolstered && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								RepulseDamage(self, push_target[x], tr.endpos, damage_level);
							}
							else
							{
								if (pull)
								{
									//
								}
								else
								{
									PushDamage(self, push_target[x], tr.endpos, damage_level);
								}
							}
						}
					}
					else if (pull)
					{
						if (walk_check(push_target[x])
							|| push_target[x]->client->ps.fd.blockPoints >= BLOCKPOINTS_HALF && push_target[x]->client->
							pers.cmd.buttons & BUTTON_BLOCK
							|| push_target[x]->client->ps.BlasterAttackChainCount <= BLASTERMISHAPLEVEL_HEAVYER && !
							BG_IsUsingHeavyWeap(&push_target[x]->client->ps)
							&& in_front(push_target[x]->client->ps.origin, self->client->ps.origin,
								self->client->ps.viewangles, -0.7f))
						{
							WP_ResistForcePush(push_target[x], self, qfalse);
							push_target[x]->client->ps.saber_move = push_target[x]->client->ps.saberBounceMove = LS_READY;
							//don't finish whatever saber anim you may have been in
							push_target[x]->client->ps.saberBlocked = BLOCKED_NONE;
						}
						else if (PM_SaberInBrokenParry(push_target[x]->client->ps.saber_move) && dir_len <= 64.0f)
						{
							//do a knockdown if fairly close
							G_Knockdown(push_target[x], self, push_dir, 300, qtrue);
							push_power_mod = 200;
							G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushed.mp3"));

							if (self->client->ps.weapon == WP_MELEE || self->client->ps.weapon == WP_NONE && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE
								|| self->client->ps.weapon == WP_SABER && self->client->ps.saberHolstered && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								RepulseDamage(self, push_target[x], tr.endpos, damage_level);
							}
							else
							{
								if (pull)
								{
									//
								}
								else
								{
									PushDamage(self, push_target[x], tr.endpos, damage_level);
								}
							}
						}
						else if (BG_KnockDownable(&push_target[x]->client->ps) && dir_len <= 128.0f && !walk_check(
							push_target[x]))
						{
							//can only do a knockdown if fairly close
							G_Knockdown(push_target[x], self, push_dir, 300, qtrue);
							push_power_mod = 250;
							G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushed.mp3"));

							if (self->client->ps.weapon == WP_MELEE || self->client->ps.weapon == WP_NONE && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE
								|| self->client->ps.weapon == WP_SABER && self->client->ps.saberHolstered && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								RepulseDamage(self, push_target[x], tr.endpos, damage_level);
							}
							else
							{
								if (pull)
								{
									//
								}
								else
								{
									PushDamage(self, push_target[x], tr.endpos, damage_level);
								}
							}
						}
						else if (!in_front(push_target[x]->r.currentOrigin, self->r.currentOrigin,
							self->client->ps.viewangles, 0.3f) && !walk_check(push_target[x]))
						{
							G_Knockdown(push_target[x], self, push_dir, 300, qtrue);
							push_power_mod = 200;
							G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushed.mp3"));

							if (self->client->ps.weapon == WP_MELEE || self->client->ps.weapon == WP_NONE && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE
								|| self->client->ps.weapon == WP_SABER && self->client->ps.saberHolstered && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								RepulseDamage(self, push_target[x], tr.endpos, damage_level);
							}
							else
							{
								if (pull)
								{
									//
								}
								else
								{
									PushDamage(self, push_target[x], tr.endpos, damage_level);
								}
							}
						}
						else
						{
							G_Knockdown(push_target[x], self, push_dir, 300, qtrue);
							push_power_mod = 200;
							G_Sound(push_target[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushed.mp3"));

							if (self->client->ps.weapon == WP_MELEE || self->client->ps.weapon == WP_NONE && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE
								|| self->client->ps.weapon == WP_SABER && self->client->ps.saberHolstered && self->
								client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								RepulseDamage(self, push_target[x], tr.endpos, damage_level);
							}
							else
							{
								if (pull)
								{
									//
								}
								else
								{
									PushDamage(self, push_target[x], tr.endpos, damage_level);
								}
							}
						}
					}

					if (i_grip)
					{
						push_power_mod *= 4;
					}

					push_target[x]->client->ps.velocity[0] = push_dir[0] * push_power_mod;
					push_target[x]->client->ps.velocity[1] = push_dir[1] * push_power_mod;

					if ((int)push_target[x]->client->ps.velocity[2] == 0)
					{
						//if not going anywhere vertically, boost them up a bit
						push_target[x]->client->ps.velocity[2] = push_dir[2] * push_power_mod + 200;

						if (push_target[x]->client->ps.velocity[2] < 128)
						{
							push_target[x]->client->ps.velocity[2] = 128;
						}
					}
					else
					{
						push_target[x]->client->ps.velocity[2] = push_dir[2] * push_power_mod;
					}
				}
			}
			else if (push_target[x]->s.eType == ET_MISSILE && push_target[x]->s.pos.trType != TR_STATIONARY && (push_target[x]
				->s.pos.trType != TR_INTERPOLATE || push_target[x]->s.weapon != WP_THERMAL))
				//rolling and stationary thermal detonators are dealt with below
			{
				if (pull)
				{
					//deflect rather than reflect?
				}
				else
				{
					if (self->r.svFlags & SVF_BOT)
					{
						g_reflect_missile_bot(self, push_target[x], forward);
					}
					else
					{
						g_reflect_missile_auto(self, push_target[x], forward);
					}
				}
			}
			else if (g_pushitems.integer && CheckPushItem(push_target[x]))
			{
				if (push_target[x]->item->giType == IT_TEAM)
				{
					push_target[x]->nextthink = level.time + CTF_FLAG_RETURN_TIME;
					push_target[x]->think = ResetItem; //incase it falls off a cliff
				}
				else
				{
					push_target[x]->nextthink = level.time + 30000;
					push_target[x]->think = ResetItem; //incase it falls off a cliff
				}

				if (pull)
				{
					//pull the item
					push_target[x]->s.pos.trType = TR_GRAVITY;
					push_target[x]->s.apos.trType = TR_GRAVITY;
					VectorScale(forward, -650.0f, push_target[x]->s.pos.trDelta);
					VectorScale(forward, -650.0f, push_target[x]->s.apos.trDelta);
					push_target[x]->s.pos.trTime = level.time; // move a bit on the very first frame
					push_target[x]->s.apos.trTime = level.time; // move a bit on the very first frame
					VectorCopy(push_target[x]->r.currentOrigin, push_target[x]->s.pos.trBase);
					VectorCopy(push_target[x]->r.currentOrigin, push_target[x]->s.apos.trBase);
					push_target[x]->s.eFlags = FL_BOUNCE_HALF;
				}
				else
				{
					push_target[x]->s.pos.trType = TR_GRAVITY;
					push_target[x]->s.apos.trType = TR_GRAVITY;
					VectorScale(forward, 650.0f, push_target[x]->s.pos.trDelta);
					VectorScale(forward, 650.0f, push_target[x]->s.apos.trDelta);
					push_target[x]->s.pos.trTime = level.time; // move a bit on the very first frame
					push_target[x]->s.apos.trTime = level.time; // move a bit on the very first frame
					VectorCopy(push_target[x]->r.currentOrigin, push_target[x]->s.pos.trBase);
					VectorCopy(push_target[x]->r.currentOrigin, push_target[x]->s.apos.trBase);
					push_target[x]->s.eFlags = FL_BOUNCE_HALF;
				}
			}
			else if (!Q_stricmp("func_static", push_target[x]->classname))
			{
				//force-usable func_static
				if (!pull && push_target[x]->spawnflags & 1/*F_PUSH*/)
				{
					GEntity_UseFunc(push_target[x], self, self);
				}
				else if (pull && push_target[x]->spawnflags & 2/*F_PULL*/)
				{
					GEntity_UseFunc(push_target[x], self, self);
				}
			}
			else if (!Q_stricmp("func_door", push_target[x]->classname) && push_target[x]->spawnflags & 2)
			{
				vec3_t end;
				//push/pull the door
				vec3_t pos1, pos2;
				vec3_t trFrom;

				VectorCopy(self->client->ps.origin, trFrom);
				trFrom[2] += self->client->ps.viewheight;

				AngleVectors(self->client->ps.viewangles, forward, NULL, NULL);
				VectorNormalize(forward);
				VectorMA(trFrom, radius, forward, end);
				trap->Trace(&tr, trFrom, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, qfalse, 0, 0);
				if (tr.entityNum != push_target[x]->s.number || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
				{
					//must be pointing right at it
					continue;
				}

				if (VectorCompare(vec3_origin, push_target[x]->s.origin))
				{
					//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
					VectorSubtract(push_target[x]->r.absmax, push_target[x]->r.absmin, size);
					VectorMA(push_target[x]->r.absmin, 0.5, size, center);
					if (push_target[x]->spawnflags & 1 && push_target[x]->moverState == MOVER_POS1)
					{
						//if at pos1 and started open, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
						VectorSubtract(center, push_target[x]->pos1, center);
					}
					else if (!(push_target[x]->spawnflags & 1) && push_target[x]->moverState == MOVER_POS2)
					{
						//if at pos2, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
						VectorSubtract(center, push_target[x]->pos2, center);
					}
					VectorAdd(center, push_target[x]->pos1, pos1);
					VectorAdd(center, push_target[x]->pos2, pos2);
				}
				else
				{
					//actually has an origin, pos1 and pos2 are absolute
					VectorCopy(push_target[x]->r.currentOrigin, center);
					VectorCopy(push_target[x]->pos1, pos1);
					VectorCopy(push_target[x]->pos2, pos2);
				}

				if (Distance(pos1, trFrom) < Distance(pos2, trFrom))
				{
					//pos1 is closer
					if (push_target[x]->moverState == MOVER_POS1)
					{
						//at the closest pos
						if (pull)
						{
							//trying to pull, but already at closest point, so screw it
							continue;
						}
					}
					else if (push_target[x]->moverState == MOVER_POS2)
					{
						//at farthest pos
						if (!pull)
						{
							//trying to push, but already at farthest point, so screw it
							continue;
						}
					}
				}
				else
				{
					//pos2 is closer
					if (push_target[x]->moverState == MOVER_POS1)
					{
						//at the farthest pos
						if (!pull)
						{
							//trying to push, but already at farthest point, so screw it
							continue;
						}
					}
					else if (push_target[x]->moverState == MOVER_POS2)
					{
						//at closest pos
						if (pull)
						{
							//trying to pull, but already at closest point, so screw it
							continue;
						}
					}
				}
				GEntity_UseFunc(push_target[x], self, self);
			}
			else if (Q_stricmp("func_button", push_target[x]->classname) == 0)
			{
				//pretend you pushed it
				Touch_Button(push_target[x], self, NULL);
			}
		}
	}

	//attempt to break any leftover grips
	//if we're still in a current grip that wasn't broken by the push, it will still remain
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if (self->client->ps.fd.forceGripBeingGripped > level.time)
	{
		self->client->ps.fd.forceGripBeingGripped = 0;
	}
}

void WP_ForcePowerStop(gentity_t* self, const forcePowers_t forcePower)
{
	const int was_active = self->client->ps.fd.forcePowersActive;

	self->client->ps.fd.forcePowersActive &= ~(1 << forcePower);

	switch ((int)forcePower)
	{
	case FP_HEAL:
		self->client->ps.fd.forceHealAmount = 0;
		self->client->ps.fd.forceHealTime = 0;
		break;
	case FP_LEVITATION:
		break;
	case FP_SPEED:
		if (was_active & 1 << forcePower)
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_2 - 50], CHAN_VOICE);
		}
		if (self->client->ps.fd.forcePowerLevel[forcePower] < FORCE_LEVEL_2)
		{
			self->client->ps.fd.forceSpeedRecoveryTime = level.time + 1500; //recover for 1.5 seconds
		}
		else
		{
			self->client->ps.fd.forceSpeedRecoveryTime = level.time + 1000; //recover for 1 seconds
		}
		break;
	case FP_PUSH:
		break;
	case FP_PULL:
		break;
	case FP_TELEPATHY:
		if (was_active & 1 << forcePower)
		{
			G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distractstop.wav"));
		}
		self->client->ps.fd.forceMindtrickTargetIndex = 0;
		self->client->ps.fd.forceMindtrickTargetIndex2 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex3 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex4 = 0;
		break;
	case FP_SEE:
		if (was_active & 1 << FP_SEE)
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_5 - 50], CHAN_VOICE);
		}
		break;
	case FP_GRIP:
		self->client->ps.fd.forceGripUseTime = level.time + 3000;
		if (self->client->ps.fd.forcePowerLevel[forcePower] > FORCE_LEVEL_1 &&
			g_entities[self->client->ps.fd.forceGripentity_num].client &&
			g_entities[self->client->ps.fd.forceGripentity_num].health > 0 &&
			g_entities[self->client->ps.fd.forceGripentity_num].inuse &&
			level.time - g_entities[self->client->ps.fd.forceGripentity_num].client->ps.fd.forceGripStarted > 500)
		{
			//if we had our throat crushed in for more than half a second, gasp for air when we're let go
			if (was_active & 1 << forcePower)
			{
				G_EntitySound(&g_entities[self->client->ps.fd.forceGripentity_num], CHAN_VOICE,
					G_SoundIndex("*gasp.wav"));
			}
		}

		if (g_entities[self->client->ps.fd.forceGripentity_num].client &&
			g_entities[self->client->ps.fd.forceGripentity_num].inuse)
		{
			g_entities[self->client->ps.fd.forceGripentity_num].client->ps.forceGripChangeMovetype = PM_NORMAL;
		}

		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0;
		}

		self->client->ps.fd.forceGripentity_num = ENTITYNUM_NONE;

		self->client->ps.powerups[PW_DISINT_4] = 0;
		break;
	case FP_LIGHTNING:
		if (self->client->ps.fd.forcePowerLevel[forcePower] < FORCE_LEVEL_2)
		{
			//don't do it again for 3 seconds, minimum... FIXME: this should be automatic once regeneration is slower (normal)
			self->client->ps.fd.forcePowerDebounce[forcePower] = level.time + 3000;
		}
		else
		{
			self->client->ps.fd.forcePowerDebounce[forcePower] = level.time + 1500;
		}
		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0; //reset hand position
		}

		self->client->ps.activeForcePass = 0;
		break;
	case FP_RAGE:
		if (self->client->ps.fd.forcePowerLevel[forcePower] < FORCE_LEVEL_2)
		{
			self->client->ps.fd.forceRageRecoveryTime = level.time + 10000; //recover for 10 seconds
		}
		else
		{
			self->client->ps.fd.forceRageRecoveryTime = level.time + 5000; //recover for 5 seconds
		}

		if (was_active & 1 << forcePower)
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3 - 50], CHAN_VOICE);
		}
		break;
	case FP_ABSORB:
		if (was_active & 1 << forcePower)
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3 - 50], CHAN_VOICE);
		}
		break;
	case FP_PROTECT:
		if (was_active & 1 << forcePower)
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3 - 50], CHAN_VOICE);
		}
		break;
	case FP_DRAIN:
		if (self->client->ps.fd.forcePowerLevel[forcePower] < FORCE_LEVEL_2)
		{
			//don't do it again for 3 seconds, minimum...
			self->client->ps.fd.forcePowerDebounce[forcePower] = level.time + 3000;
		}
		else
		{
			self->client->ps.fd.forcePowerDebounce[forcePower] = level.time + 1500;
		}

		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0; //reset hand position
		}

		self->client->ps.activeForcePass = 0;
	default:
		break;
	}
}

extern qboolean Boba_Flying(const gentity_t* self);
extern void Boba_FlyStop(gentity_t* self);

static void DoGripAction(gentity_t* self, const forcePowers_t forcePower)
{
	trace_t tr;
	vec3_t a;

	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	gentity_t* gripEnt = &g_entities[self->client->ps.fd.forceGripentity_num];

	if (!gripEnt || !gripEnt->client || !gripEnt->inuse || gripEnt->health < 1 || !ForcePowerUsableOn(
		self, gripEnt, FP_GRIP))
	{
		WP_ForcePowerStop(self, forcePower);
		self->client->ps.fd.forceGripentity_num = ENTITYNUM_NONE;

		if (gripEnt && gripEnt->client && gripEnt->inuse)
		{
			gripEnt->client->ps.forceGripChangeMovetype = PM_NORMAL;
		}
		return;
	}

	VectorSubtract(gripEnt->client->ps.origin, self->client->ps.origin, a);

	trap->Trace(&tr, self->client->ps.origin, NULL, NULL, gripEnt->client->ps.origin, self->s.number, MASK_PLAYERSOLID,
		qfalse, 0, 0);

	int grip_level = wp_absorb_conversion(gripEnt, gripEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], FP_GRIP,
		self->client->ps.fd.forcePowerLevel[FP_GRIP],
		forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP]);

	if (grip_level == -1)
	{
		grip_level = self->client->ps.fd.forcePowerLevel[FP_GRIP];
	}

	if (!grip_level)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}

	if (VectorLength(a) > MAX_GRIP_DISTANCE)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}

	if (!in_front(gripEnt->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.9f) &&
		grip_level < FORCE_LEVEL_3)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}

	if (tr.fraction != 1.0f &&
		tr.entityNum != gripEnt->s.number /*&&
		gripLevel < FORCE_LEVEL_3*/)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}

	if (self->client->ps.fd.forcePowerDebounce[FP_GRIP] < level.time)
	{
		//2 damage per second while choking, resulting in 10 damage total (not including The Squeeze<tm>)
		self->client->ps.fd.forcePowerDebounce[FP_GRIP] = level.time + 1000;
		G_Damage(gripEnt, self, self, NULL, NULL, 2, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);
	}

	Jetpack_Off(gripEnt); //make sure the guy being gripped has his jetpack off.

	if (Boba_Flying(gripEnt))
	{
		Boba_FlyStop(gripEnt);
	}

	if (grip_level == FORCE_LEVEL_1)
	{
		gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

		if (level.time - gripEnt->client->ps.fd.forceGripStarted > 5000)
		{
			WP_ForcePowerStop(self, forcePower);
		}
		return;
	}

	if (grip_level == FORCE_LEVEL_2)
	{
		gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

		if (gripEnt->client->ps.forceGripMoveInterval < level.time)
		{
			gripEnt->client->ps.velocity[2] = 30;

			gripEnt->client->ps.forceGripMoveInterval = level.time + 300;
			//only update velocity every 300ms, so as to avoid heavy bandwidth usage
		}

		gripEnt->client->ps.otherKiller = self->s.number;
		gripEnt->client->ps.otherKillerTime = level.time + 5000;
		gripEnt->client->ps.otherKillerDebounceTime = level.time + 100;
		gripEnt->client->otherKillerMOD = MOD_UNKNOWN;
		gripEnt->client->otherKillerVehWeapon = 0;
		gripEnt->client->otherKillerWeaponType = WP_NONE;

		gripEnt->client->ps.forceGripChangeMovetype = PM_FLOAT;

		if (level.time - gripEnt->client->ps.fd.forceGripStarted > 3000 && !self->client->ps.fd.
			forceGripDamageDebounceTime)
		{
			//if we managed to lift him into the air for 2 seconds, give him a crack
			self->client->ps.fd.forceGripDamageDebounceTime = 1;
			G_Damage(gripEnt, self, self, NULL, NULL, 20, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);

			//Must play custom sounds on the actual entity. Don't use G_Sound (it creates a temp entity for the sound)
			G_EntitySound(gripEnt, CHAN_VOICE, G_SoundIndex(va("*choke%d.wav", Q_irand(1, 3))));

			gripEnt->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
			gripEnt->client->ps.forceHandExtendTime = level.time + 2000;

			if (gripEnt->client->ps.fd.forcePowersActive & 1 << FP_GRIP)
			{
				//choking, so don't let him keep gripping himself
				WP_ForcePowerStop(gripEnt, FP_GRIP);
			}
		}
		else if (level.time - gripEnt->client->ps.fd.forceGripStarted > 4000)
		{
			WP_ForcePowerStop(self, forcePower);
		}
		return;
	}

	if (grip_level == FORCE_LEVEL_3)
	{
		gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

		gripEnt->client->ps.otherKiller = self->s.number;
		gripEnt->client->ps.otherKillerTime = level.time + 5000;
		gripEnt->client->ps.otherKillerDebounceTime = level.time + 100;
		gripEnt->client->otherKillerMOD = MOD_UNKNOWN;
		gripEnt->client->otherKillerVehWeapon = 0;
		gripEnt->client->otherKillerWeaponType = WP_NONE;

		gripEnt->client->ps.forceGripChangeMovetype = PM_FLOAT;

		if (gripEnt->client->ps.forceGripMoveInterval < level.time)
		{
			vec3_t nvel;
			vec3_t start_o;
			vec3_t fwd_o;
			vec3_t fwd;

			VectorCopy(gripEnt->client->ps.origin, start_o);
			AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
			fwd_o[0] = self->client->ps.origin[0] + fwd[0] * 128;
			fwd_o[1] = self->client->ps.origin[1] + fwd[1] * 128;
			fwd_o[2] = self->client->ps.origin[2] + fwd[2] * 128;
			fwd_o[2] += 16;
			VectorSubtract(fwd_o, start_o, nvel);

			const float nv_len = VectorLength(nvel);

			if (nv_len < 16)
			{
				//within x units of desired spot
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0] * 8;
				gripEnt->client->ps.velocity[1] = nvel[1] * 8;
				gripEnt->client->ps.velocity[2] = nvel[2] * 8;
			}
			else if (nv_len < 64)
			{
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0] * 128;
				gripEnt->client->ps.velocity[1] = nvel[1] * 128;
				gripEnt->client->ps.velocity[2] = nvel[2] * 128;
			}
			else if (nv_len < 128)
			{
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0] * 256;
				gripEnt->client->ps.velocity[1] = nvel[1] * 256;
				gripEnt->client->ps.velocity[2] = nvel[2] * 256;
			}
			else if (nv_len < 200)
			{
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0] * 512;
				gripEnt->client->ps.velocity[1] = nvel[1] * 512;
				gripEnt->client->ps.velocity[2] = nvel[2] * 512;
			}
			else
			{
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0] * 700;
				gripEnt->client->ps.velocity[1] = nvel[1] * 700;
				gripEnt->client->ps.velocity[2] = nvel[2] * 700;
			}

			gripEnt->client->ps.forceGripMoveInterval = level.time + 300;
			//only update velocity every 300ms, so as to avoid heavy bandwidth usage
		}

		if (level.time - gripEnt->client->ps.fd.forceGripStarted > 3000 && !self->client->ps.fd.
			forceGripDamageDebounceTime)
		{
			//if we managed to lift him into the air for 2 seconds, give him a crack
			self->client->ps.fd.forceGripDamageDebounceTime = 1;
			G_Damage(gripEnt, self, self, NULL, NULL, 40, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);

			//Must play custom sounds on the actual entity. Don't use G_Sound (it creates a temp entity for the sound)
			G_EntitySound(gripEnt, CHAN_VOICE, G_SoundIndex(va("*choke%d.wav", Q_irand(1, 3))));

			gripEnt->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
			gripEnt->client->ps.forceHandExtendTime = level.time + 2000;

			if (gripEnt->client->ps.fd.forcePowersActive & 1 << FP_GRIP)
			{
				//choking, so don't let him keep gripping himself
				WP_ForcePowerStop(gripEnt, FP_GRIP);
			}
		}
		else if (level.time - gripEnt->client->ps.fd.forceGripStarted > 4000)
		{
			WP_ForcePowerStop(self, forcePower);
		}
	}
}

static qboolean G_IsMindTricked(const forcedata_t* fd, const int client)
{
	int checkIn;
	int sub = 0;

	if (!fd)
	{
		return qfalse;
	}

	const int trickIndex1 = fd->forceMindtrickTargetIndex;
	const int trickIndex2 = fd->forceMindtrickTargetIndex2;
	const int trickIndex3 = fd->forceMindtrickTargetIndex3;
	const int trickIndex4 = fd->forceMindtrickTargetIndex4;

	if (client > 47)
	{
		checkIn = trickIndex4;
		sub = 48;
	}
	else if (client > 31)
	{
		checkIn = trickIndex3;
		sub = 32;
	}
	else if (client > 15)
	{
		checkIn = trickIndex2;
		sub = 16;
	}
	else
	{
		checkIn = trickIndex1;
	}

	if (checkIn & (1 << (client - sub)))
	{
		return qtrue;
	}

	return qfalse;
}

static void RemoveTrickedEnt(forcedata_t* fd, const int client)
{
	if (!fd)
	{
		return;
	}

	if (client > 47)
	{
		fd->forceMindtrickTargetIndex4 &= ~(1 << (client - 48));
	}
	else if (client > 31)
	{
		fd->forceMindtrickTargetIndex3 &= ~(1 << (client - 32));
	}
	else if (client > 15)
	{
		fd->forceMindtrickTargetIndex2 &= ~(1 << (client - 16));
	}
	else
	{
		fd->forceMindtrickTargetIndex &= ~(1 << client);
	}
}

extern int g_LastFrameTime;
extern int g_TimeSinceLastFrame;

static void WP_UpdateMindtrickEnts(gentity_t* self)
{
	int i = 0;

	while (i < MAX_CLIENTS)
	{
		if (G_IsMindTricked(&self->client->ps.fd, i))
		{
			const gentity_t* ent = &g_entities[i];

			if (!ent || !ent->client || !ent->inuse || ent->health < 1 ||
				ent->client->ps.fd.forcePowersActive & 1 << FP_SEE)
			{
				RemoveTrickedEnt(&self->client->ps.fd, i);
			}
			else if (level.time - self->client->dangerTime < g_TimeSinceLastFrame * 4)
			{
				//Untrick this entity if the tricker (self) fires while in his fov
				if (trap->InPVS(ent->client->ps.origin, self->client->ps.origin) &&
					org_visible(ent->client->ps.origin, self->client->ps.origin, ent->s.number))
				{
					RemoveTrickedEnt(&self->client->ps.fd, i);
				}
			}
			else if (BG_HasYsalamiri(level.gametype, &ent->client->ps))
			{
				RemoveTrickedEnt(&self->client->ps.fd, i);
			}
		}

		i++;
	}

	if (!self->client->ps.fd.forceMindtrickTargetIndex &&
		!self->client->ps.fd.forceMindtrickTargetIndex2 &&
		!self->client->ps.fd.forceMindtrickTargetIndex3 &&
		!self->client->ps.fd.forceMindtrickTargetIndex4)
	{
		//everyone who we had tricked is no longer tricked, so stop the power
		WP_ForcePowerStop(self, FP_TELEPATHY);
	}
	else if (self->client->ps.powerups[PW_REDFLAG] ||
		self->client->ps.powerups[PW_BLUEFLAG])
	{
		WP_ForcePowerStop(self, FP_TELEPATHY);
	}
}

#define FORCE_DEBOUNCE_TIME 50 // sv_fps 20 = 50msec frametime, basejka balance/timing

static int FlamethrowerDebounceTime = 0;
#define FLAMETHROWERDEBOUNCE		50

static void wp_force_power_run(gentity_t* self, const forcePowers_t forcePower, const usercmd_t* cmd)
{
	int soundduration;

	switch ((int)forcePower)
	{
	case FP_HEAL:
		if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_1)
		{
			if (self->client->ps.velocity[0] || self->client->ps.velocity[1] || self->client->ps.velocity[2])
			{
				WP_ForcePowerStop(self, forcePower);
				break;
			}
		}

		if (self->health < 1 || self->client->ps.stats[STAT_HEALTH] < 1)
		{
			WP_ForcePowerStop(self, forcePower);
			break;
		}

		if (self->client->ps.fd.forceHealTime > level.time)
		{
			break;
		}
		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{
			//rww - we might start out over max_health and we don't want force heal taking us down to 100 or whatever max_health is
			WP_ForcePowerStop(self, forcePower);
			break;
		}
		self->client->ps.fd.forceHealTime = level.time + 1000;
		self->health++;
		self->client->ps.fd.forceHealAmount++;

		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH]) // Past max health
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
			WP_ForcePowerStop(self, forcePower);
		}

		if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_1 && self->client->ps.fd.forceHealAmount >= 25
			||
			self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_2 && self->client->ps.fd.forceHealAmount >=
			33)
		{
			WP_ForcePowerStop(self, forcePower);
		}
		break;
	case FP_SPEED:
		//This is handled in PM_WalkMove and PM_StepSlideMove
		if (self->client->holdingObjectiveItem >= MAX_CLIENTS
			&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD)
		{
			if (g_entities[self->client->holdingObjectiveItem].genericValue15)
			{
				//disables force powers
				WP_ForcePowerStop(self, forcePower);
			}
		}
		break;
	case FP_GRIP:
		if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		{
			WP_ForcePowerStop(self, FP_GRIP);
			break;
		}

		if (self->client->ps.fd.forcePowerDebounce[FP_PULL] < level.time)
		{
			//This is sort of not ideal. Using the debounce value reserved for pull for this because pull doesn't need it.
			WP_ForcePowerDrain(&self->client->ps, forcePower, 1);
			self->client->ps.fd.forcePowerDebounce[FP_PULL] = level.time + 100;
		}

		if (self->client->ps.fd.forcePower < 1)
		{
			WP_ForcePowerStop(self, FP_GRIP);
			break;
		}

		DoGripAction(self, forcePower);
		break;
	case FP_LEVITATION:
		if (self->client->ps.groundEntityNum != ENTITYNUM_NONE && !self->client->ps.fd.forceJumpZStart ||
			PM_InLedgeMove(self->client->ps.legsAnim) || PM_InWallHoldMove(self->client->ps.legsAnim))
		{
			//done with jump
			WP_ForcePowerStop(self, forcePower);
		}
		break;
	case FP_RAGE:
		if (self->health < 1)
		{
			WP_ForcePowerStop(self, forcePower);
			break;
		}
		if (self->client->ps.forceRageDrainTime < level.time)
		{
			int addTime = 400;

			self->health -= 2;

			if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_1)
			{
				addTime = 150;
			}
			else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_2)
			{
				addTime = 300;
			}
			else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_3)
			{
				addTime = 450;
			}
			self->client->ps.forceRageDrainTime = level.time + addTime;
		}

		if (self->health < 1)
		{
			self->health = 1;
			WP_ForcePowerStop(self, forcePower);
		}

		self->client->ps.stats[STAT_HEALTH] = self->health;

		soundduration = ceil(FORCE_RAGE_DURATION * forceSpeedValue[self->client->ps.fd.forcePowerLevel[FP_RAGE] - 1]);

		if (soundduration)
		{
			self->s.loopSound = G_SoundIndex("sound/weapons/force/rageloop.wav");
		}

		break;
	case FP_DRAIN:
		if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		{
			WP_ForcePowerStop(self, forcePower);
			break;
		}

		if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1)
		{
			//higher than level 1
			if (cmd->buttons & BUTTON_FORCE_DRAIN || cmd->buttons & BUTTON_FORCEPOWER && self->client->ps.fd.
				forcePowerSelected == FP_DRAIN)
			{
				//holding it keeps it going
				self->client->ps.fd.forcePowerDuration[FP_DRAIN] = level.time + 500;
			}
		}
		// OVERRIDEFIXME
		if (!WP_ForcePowerAvailable(self, forcePower, 0) || self->client->ps.fd.forcePowerDuration[FP_DRAIN] < level.
			time ||
			self->client->ps.fd.forcePower < 25)
		{
			WP_ForcePowerStop(self, forcePower);
		}
		else
		{
			while (self->client->force.drainDebounce < level.time)
			{
				ForceShootDrain(self);
				self->client->force.drainDebounce += FORCE_DEBOUNCE_TIME;
			}

			if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2)
			{
				ForceShootDestruction(self);
			}
		}
		break;
	case FP_LIGHTNING:
		if (!in_camera && self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		{
			//Animation for hand extend doesn't end with hand out, so we have to limit lightning intervals by animation intervals (once hand starts to go in in animation, lightning should stop)
			WP_ForcePowerStop(self, forcePower);
			break;
		}

		if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1)
		{
			//higher than level 1
			if (cmd->buttons & BUTTON_FORCE_LIGHTNING || cmd->buttons & BUTTON_FORCEPOWER && self->client->ps.fd.
				forcePowerSelected == FP_LIGHTNING)
			{
				//holding it keeps it going
				self->client->ps.fd.forcePowerDuration[FP_LIGHTNING] = level.time + 500;
			}
		}

		if (!WP_ForcePowerAvailable(self, forcePower, 0) || self->client->ps.fd.forcePower < 25)
		{
			WP_ForcePowerStop(self, forcePower);
		}
		else
		{
			while (self->client->force.lightningDebounce < level.time)
			{
				force_shoot_lightning(self);
				WP_ForcePowerDrain(&self->client->ps, forcePower, 0);

				self->client->force.lightningDebounce += FORCE_DEBOUNCE_TIME;
			}
		}
		break;
	case FP_TELEPATHY:
		if (self->client->holdingObjectiveItem >= MAX_CLIENTS
			&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD
			&& g_entities[self->client->holdingObjectiveItem].genericValue15)
		{
			//if force hindered can't mindtrick whilst carrying a siege item
			WP_ForcePowerStop(self, FP_TELEPATHY);
		}
		else
		{
			WP_UpdateMindtrickEnts(self);
		}
		break;
	case FP_SABER_OFFENSE:
		break;
	case FP_SABER_DEFENSE:
		break;
	case FP_SABERTHROW:
		break;
	case FP_PROTECT:
		if (self->client->ps.fd.forcePowerDebounce[forcePower] < level.time)
		{
			WP_ForcePowerDrain(&self->client->ps, forcePower, 1);
			if (self->client->ps.fd.forcePower < 1)
			{
				WP_ForcePowerStop(self, forcePower);
			}

			self->client->ps.fd.forcePowerDebounce[forcePower] = level.time + 300;
		}
		break;
	case FP_ABSORB:
		if (self->client->ps.fd.forcePowerDebounce[forcePower] < level.time)
		{
			WP_ForcePowerDrain(&self->client->ps, forcePower, 1);
			if (self->client->ps.fd.forcePower < 1)
			{
				WP_ForcePowerStop(self, forcePower);
			}

			self->client->ps.fd.forcePowerDebounce[forcePower] = level.time + 600;
		}
		break;
	default:
		break;
	}
}

static int WP_DoSpecificPower(gentity_t* self, const usercmd_t* ucmd, const forcePowers_t forcepower)
{
	int powerSucceeded = 1;

	// OVERRIDEFIXME
	if (!WP_ForcePowerAvailable(self, forcepower, 0))
	{
		return 0;
	}

	switch (forcepower)
	{
	case FP_HEAL:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{
			//need to release before we can use nonhold powers again
			break;
		}
		ForceHeal(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_LEVITATION:
		//if leave the ground by some other means, cancel the force jump so we don't suddenly jump when we land.

		if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			self->client->ps.fd.forceJumpCharge = 0;
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);
			//This only happens if the groundEntityNum == ENTITYNUM_NONE when the button is actually released
		}
		else
		{
			//still on ground, so jump
			ForceJump(self, ucmd);
		}
		break;
	case FP_SPEED:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{
			//need to release before we can use nonhold powers again
			break;
		}
		ForceSpeed(self, 0);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_GRIP:
		if (self->client->ps.fd.forceGripentity_num == ENTITYNUM_NONE)
		{
			ForceGrip(self);
		}

		if (self->client->ps.fd.forceGripentity_num != ENTITYNUM_NONE)
		{
			if (!(self->client->ps.fd.forcePowersActive & 1 << FP_GRIP))
			{
				WP_ForcePowerStart(self, FP_GRIP, 0);
				WP_ForcePowerDrain(&self->client->ps, FP_GRIP, GRIP_DRAIN_AMOUNT);
			}
		}
		else
		{
			powerSucceeded = 0;
		}
		break;
	case FP_LIGHTNING:
		ForceLightning(self);
		break;
	case FP_PUSH:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease && !(self->r.svFlags & SVF_BOT))
		{
			//need to release before we can use nonhold powers again
			break;
		}
		ForceThrow(self, qfalse);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_PULL:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{
			//need to release before we can use nonhold powers again
			break;
		}
		ForceThrow(self, qtrue);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_TELEPATHY:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{
			//need to release before we can use nonhold powers again
			break;
		}
		ForceTelepathy(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_RAGE:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{
			//need to release before we can use nonhold powers again
			break;
		}
		ForceRage(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_PROTECT:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{
			//need to release before we can use nonhold powers again
			break;
		}
		ForceProtect(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_ABSORB:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{
			//need to release before we can use nonhold powers again
			break;
		}
		ForceAbsorb(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_TEAM_HEAL:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{
			//need to release before we can use nonhold powers again
			break;
		}
		ForceTeamHeal(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_TEAM_FORCE:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{
			//need to release before we can use nonhold powers again
			break;
		}
		ForceTeamForceReplenish(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_DRAIN:
		ForceDrain(self);
		break;
	case FP_SEE:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{
			//need to release before we can use nonhold powers again
			break;
		}
		ForceSeeing(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_SABER_OFFENSE:
		break;
	case FP_SABER_DEFENSE:
		break;
	case FP_SABERTHROW:
		break;
	default:
		break;
	}

	return powerSucceeded;
}

static void FindGenericEnemyIndex(const gentity_t* self)
{
	//Find another client that would be considered a threat.
	int i = 0;
	const gentity_t* besten = NULL;
	float blen = 99999999.9f;

	while (i < MAX_CLIENTS)
	{
		const gentity_t* ent = &g_entities[i];

		if (ent && ent->client && ent->s.number != self->s.number && ent->health > 0 && !OnSameTeam(self, ent) && ent->
			client->ps.pm_type != PM_INTERMISSION && ent->client->ps.pm_type != PM_SPECTATOR)
		{
			vec3_t a;
			VectorSubtract(ent->client->ps.origin, self->client->ps.origin, a);
			const float tlen = VectorLength(a);

			if (tlen < blen &&
				in_front(ent->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.8f) &&
				org_visible(self->client->ps.origin, ent->client->ps.origin, self->s.number))
			{
				blen = tlen;
				besten = ent;
			}
		}

		i++;
	}

	if (!besten)
	{
		return;
	}

	self->client->ps.genericEnemyIndex = besten->s.number;
}

static void SeekerDroneUpdate(gentity_t* self)
{
	vec3_t org, elevated, dir, a;
	float angle;
	trace_t tr;

	if (!(self->client->ps.eFlags & EF_SEEKERDRONE))
	{
		self->client->ps.genericEnemyIndex = -1;
		return;
	}

	if (self->health < 1)
	{
		VectorCopy(self->client->ps.origin, elevated);
		elevated[2] += 40;

		angle = (level.time / 12 & 255) * (M_PI * 2) / 255; //magical numbers make magic happen
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, org);

		a[ROLL] = 0;
		a[YAW] = 0;
		a[PITCH] = 1;

		G_PlayEffect(EFFECT_SPARK_EXPLOSION, org, a);

		self->client->ps.eFlags &= ~EF_SEEKERDRONE;
		self->client->ps.genericEnemyIndex = -1;

		return;
	}

	if (self->client->ps.droneExistTime >= level.time &&
		self->client->ps.droneExistTime < level.time + 5000)
	{
		self->client->ps.genericEnemyIndex = 1024 + self->client->ps.droneExistTime;
		if (self->client->ps.droneFireTime < level.time)
		{
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/laser_trap/warning.wav"));
			self->client->ps.droneFireTime = level.time + 100;
		}
		return;
	}
	if (self->client->ps.droneExistTime < level.time)
	{
		VectorCopy(self->client->ps.origin, elevated);
		elevated[2] += 40;

		float prefig = (self->client->ps.droneExistTime - level.time) / 80;

		if (prefig > 55)
		{
			prefig = 55;
		}
		else if (prefig < 1)
		{
			prefig = 1;
		}

		elevated[2] -= 55 - prefig;

		angle = (level.time / 12 & 255) * (M_PI * 2) / 255; //magical numbers make magic happen
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, org);

		a[ROLL] = 0;
		a[YAW] = 0;
		a[PITCH] = 1;

		G_PlayEffect(EFFECT_SPARK_EXPLOSION, org, a);

		self->client->ps.eFlags &= ~EF_SEEKERDRONE;
		self->client->ps.genericEnemyIndex = -1;

		return;
	}

	if (self->client->ps.genericEnemyIndex == -1)
	{
		self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
	}

	if (self->client->ps.genericEnemyIndex == ENTITYNUM_NONE || self->client->ps.genericEnemyIndex == -1)
	{
		FindGenericEnemyIndex(self);
	}

	if (self->client->ps.genericEnemyIndex != ENTITYNUM_NONE && self->client->ps.genericEnemyIndex != -1)
	{
		gentity_t* en = &g_entities[self->client->ps.genericEnemyIndex];

		VectorCopy(self->client->ps.origin, elevated);
		elevated[2] += 40;

		angle = (level.time / 12 & 255) * (M_PI * 2) / 255; //magical numbers make magic happen
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, org);

		//org is now where the thing should be client-side because it uses the same time-based offset
		if (self->client->ps.droneFireTime < level.time)
		{
			trap->Trace(&tr, org, NULL, NULL, en->client->ps.origin, -1, MASK_SOLID, qfalse, 0, 0);

			if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
			{
				vec3_t endir;
				VectorSubtract(en->client->ps.origin, org, endir);
				VectorNormalize(endir);

				WP_FireGenericBlasterMissile(self, org, endir, 0, 15, 2000, MOD_BLASTER);
				G_SoundAtLoc(org, CHAN_WEAPON, G_SoundIndex("sound/weapons/bryar/fire.wav"));

				self->client->ps.droneFireTime = level.time + Q_irand(400, 700);
			}
		}

		if (!en || !en->client)
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else if (en->s.number == self->s.number)
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else if (en->health < 1)
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else if (OnSameTeam(self, en))
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else
		{
			if (!in_front(en->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.8f))
			{
				self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
			}
			else if (!org_visible(self->client->ps.origin, en->client->ps.origin, self->s.number))
			{
				self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
			}
		}
	}
}

static void HolocronUpdate(gentity_t* self)
{
	//keep holocron status updated in holocron mode
	int i = 0;
	int noHRank = 0;

	if (noHRank < FORCE_LEVEL_0)
	{
		noHRank = FORCE_LEVEL_0;
	}
	if (noHRank > FORCE_LEVEL_3)
	{
		noHRank = FORCE_LEVEL_3;
	}

	trap->Cvar_Update(&g_maxHolocronCarry);

	while (i < NUM_FORCE_POWERS)
	{
		if (self->client->ps.holocronsCarried[i])
		{
			//carrying it, make sure we have the power
			self->client->ps.holocronBits |= 1 << i;
			self->client->ps.fd.forcePowersKnown |= 1 << i;
			self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;
		}
		else
		{
			//otherwise, make sure the power is cleared from us
			self->client->ps.fd.forcePowerLevel[i] = 0;
			if (self->client->ps.holocronBits & 1 << i)
			{
				self->client->ps.holocronBits -= 1 << i;
			}

			if (self->client->ps.fd.forcePowersKnown & 1 << i && i != FP_LEVITATION && i != FP_SABER_OFFENSE)
			{
				self->client->ps.fd.forcePowersKnown -= 1 << i;
			}

			if (self->client->ps.fd.forcePowersActive & 1 << i && i != FP_LEVITATION && i != FP_SABER_OFFENSE)
			{
				WP_ForcePowerStop(self, i);
			}

			if (i == FP_LEVITATION)
			{
				if (noHRank >= FORCE_LEVEL_1)
				{
					self->client->ps.fd.forcePowerLevel[i] = noHRank;
				}
				else
				{
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_1;
				}
			}
			else if (i == FP_SABER_OFFENSE)
			{
				self->client->ps.fd.forcePowersKnown |= 1 << i;

				if (noHRank >= FORCE_LEVEL_1)
				{
					self->client->ps.fd.forcePowerLevel[i] = noHRank;
				}
				else
				{
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_1;
				}

				//make sure that the player's saber stance is reset so they can't continue to use that stance when they don't have the skill for it anymore.
				if (self->client->saber[0].model[0] && self->client->saber[1].model[0])
				{
					//dual
					self->client->ps.fd.saber_anim_levelBase = self->client->ps.fd.saberAnimLevel = self->client->ps.fd.
						saberDrawAnimLevel = SS_DUAL;
				}
				else if (self->client->saber[0].saberFlags & SFL_TWO_HANDED)
				{
					//staff
					self->client->ps.fd.saberAnimLevel = self->client->ps.fd.saberDrawAnimLevel = SS_STAFF;
				}
				else
				{
					self->client->ps.fd.saber_anim_levelBase = self->client->ps.fd.saberAnimLevel = self->client->ps.fd.
						saberDrawAnimLevel = SS_MEDIUM;
				}
			}
			else
			{
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_0;
			}
		}

		i++;
	}

	if (HasSetSaberOnly())
	{
		//if saberonly, we get these powers no matter what (still need the holocrons for level 3)
		if (self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] < FORCE_LEVEL_1)
		{
			self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_1;
		}
		if (self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1)
		{
			self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_1;
		}
	}
}

static void JediMasterUpdate(gentity_t* self)
{
	//keep jedi master status updated for JM gametype
	int i = 0;

	trap->Cvar_Update(&g_maxHolocronCarry);

	while (i < NUM_FORCE_POWERS)
	{
		if (self->client->ps.isJediMaster)
		{
			self->client->ps.fd.forcePowersKnown |= 1 << i;
			self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;

			if (i == FP_TEAM_HEAL || i == FP_TEAM_FORCE ||
				i == FP_DRAIN || i == FP_ABSORB)
			{
				//team powers are useless in JM, absorb is too because no one else has powers to absorb. Drain is just
				//relatively useless in comparison, because its main intent is not to heal, but rather to cripple others
				//by draining their force at the same time. And no one needs force in JM except the JM himself.
				self->client->ps.fd.forcePowersKnown &= ~(1 << i);
				self->client->ps.fd.forcePowerLevel[i] = 0;
			}

			if (i == FP_TELEPATHY)
			{
				//this decision was made because level 3 mindtrick allows the JM to just hide too much, and no one else has force
				//sight to counteract it. Since the JM himself is the focus of gameplay in this mode, having him hidden for large
				//durations is indeed a bad thing.
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_2;
			}
		}
		else
		{
			if (self->client->ps.fd.forcePowersKnown & 1 << i && i != FP_LEVITATION)
			{
				self->client->ps.fd.forcePowersKnown -= 1 << i;
			}

			if (self->client->ps.fd.forcePowersActive & 1 << i && i != FP_LEVITATION)
			{
				WP_ForcePowerStop(self, i);
			}

			if (i == FP_LEVITATION)
			{
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_1;
			}
			else
			{
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_0;
			}
		}

		i++;
	}
}

qboolean WP_HasForcePowers(const playerState_t* ps)
{
	if (ps)
	{
		for (int i = 0; i < NUM_FORCE_POWERS; i++)
		{
			if (i == FP_LEVITATION)
			{
				if (ps->fd.forcePowerLevel[i] > FORCE_LEVEL_1)
				{
					return qtrue;
				}
			}
			else if (ps->fd.forcePowerLevel[i] > FORCE_LEVEL_0)
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

//try a special roll getup move
static qboolean G_SpecialRollGetup(gentity_t* self)
{
	//fixme: currently no knockdown will actually land you on your front... so froll's are pretty useless at the moment.
	qboolean rolled = qfalse;

	if ( /*!self->client->pers.cmd.upmove &&*/
		self->client->pers.cmd.rightmove > 0 &&
		!self->client->pers.cmd.forwardmove)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP_BROLL_R,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if ( /*!self->client->pers.cmd.upmove &&*/
		self->client->pers.cmd.rightmove < 0 &&
		!self->client->pers.cmd.forwardmove)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP_BROLL_L,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if ( /*self->client->pers.cmd.upmove > 0 &&*/
		!self->client->pers.cmd.rightmove &&
		self->client->pers.cmd.forwardmove > 0)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP_BROLL_F,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if ( /*self->client->pers.cmd.upmove > 0 &&*/
		!self->client->pers.cmd.rightmove &&
		self->client->pers.cmd.forwardmove < 0)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP_BROLL_B,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if (self->client->pers.cmd.upmove)
	{
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
		self->client->ps.forceDodgeAnim = 2;
		self->client->ps.forceHandExtendTime = level.time + 500;
	}

	if (rolled)
	{
		G_EntitySound(self, CHAN_VOICE, G_SoundIndex("*jump1.wav"));
	}

	return rolled;
}

extern qboolean PM_SaberInBrokenParry(int move);
extern qboolean PM_InKnockDown(const playerState_t* ps);
void Flamethrower_Fire(gentity_t* self);

void WP_ForcePowersUpdate(gentity_t* self, usercmd_t* ucmd)
{
	qboolean using_force = qfalse;
	int i;
	int prepower = 0;

	//see if any force powers are running
	if (!self)
	{
		return;
	}

	if (!self->client)
	{
		return;
	}

	if (self->client->ps.pm_flags & PMF_FOLLOW)
	{
		//not a "real" game client, it's a spectator following someone
		return;
	}
	if (self->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return;
	}

	//The stance in relation to power level is no longer applicable with the crazy new akimbo/staff stances.
	if (!self->client->ps.fd.saberAnimLevel)
	{
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
	}

	if (level.gametype != GT_SIEGE)
	{
		if (!(self->client->ps.fd.forcePowersKnown & 1 << FP_LEVITATION))
		{
			self->client->ps.fd.forcePowersKnown |= 1 << FP_LEVITATION;
		}

		if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_1)
		{
			self->client->ps.fd.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_1;
		}
	}

	if (self->client->ps.fd.forcePowerSelected < 0 || self->client->ps.fd.forcePowerSelected >= NUM_FORCE_POWERS)
	{
		//bad
		self->client->ps.fd.forcePowerSelected = 0;
	}
	if (self->client->ps.fd.forcePower <= self->client->ps.fd.forcePowerMax * FATIGUEDTHRESHHOLD)
	{
		//Pop the Fatigued flag
		self->client->ps.userInt3 |= 1 << FLAG_FATIGUED;
	}

	if ((self->client->sess.selectedFP != self->client->ps.fd.forcePowerSelected ||
		self->client->sess.saberLevel != self->client->ps.fd.saberAnimLevel) &&
		!(self->r.svFlags & SVF_BOT))
	{
		if (self->client->sess.updateUITime < level.time)
		{
			//a bit hackish, but we don't want the client to flood with userinfo updates if they rapidly cycle
			//through their force powers or saber attack levels

			self->client->sess.selectedFP = self->client->ps.fd.forcePowerSelected;
			self->client->sess.saberLevel = self->client->ps.fd.saberAnimLevel;
		}
	}

	if (!g_LastFrameTime)
	{
		g_LastFrameTime = level.time;
	}

	if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN)
	{
		self->client->ps.zoomFov = 0;
		self->client->ps.zoomMode = 0;
		self->client->ps.zoomLocked = qfalse;
		self->client->ps.zoomTime = 0;
	}

	if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN &&
		self->client->ps.forceHandExtendTime >= level.time)
	{
		self->client->ps.saber_move = 0;
		self->client->ps.saberBlocking = 0;
		self->client->ps.saberBlocked = 0;
		self->client->ps.weaponTime = 0;
		self->client->ps.weaponstate = WEAPON_READY;
	}
	else if (self->client->ps.forceHandExtend != HANDEXTEND_NONE &&
		self->client->ps.forceHandExtendTime < level.time)
	{
		if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN &&
			!self->client->ps.forceDodgeAnim)
		{
			if (self->health < 1 || self->client->ps.eFlags & EF_DEAD)
			{
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
			else if (G_SpecialRollGetup(self))
			{
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
			else
			{
				//hmm.. ok.. no more getting up on your own, you've gotta push something, unless..
				if (level.time - self->client->ps.forceHandExtendTime > 4000)
				{
					//4 seconds elapsed, I guess they're too dumb to push something to get up!
					if (self->client->pers.cmd.upmove &&
						self->client->ps.fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1)
					{
						//force getup
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
						self->client->ps.forceDodgeAnim = 2;
						self->client->ps.forceHandExtendTime = level.time + 500;
					}
					else if (self->client->ps.quickerGetup)
					{
						G_EntitySound(self, CHAN_VOICE, G_SoundIndex("*jump1.wav"));
						self->client->ps.forceDodgeAnim = 3;
						self->client->ps.forceHandExtendTime = level.time + 500;
						self->client->ps.velocity[2] = 300;
					}
					else
					{
						self->client->ps.forceDodgeAnim = 1;
						self->client->ps.forceHandExtendTime = level.time + 1000;
					}
				}
			}
			self->client->ps.quickerGetup = qfalse;
		}
		else if (self->client->ps.forceHandExtend == HANDEXTEND_POSTTHROWN)
		{
			if (self->health < 1 || self->client->ps.eFlags & EF_DEAD)
			{
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
			else if (self->client->ps.groundEntityNum != ENTITYNUM_NONE && !self->client->ps.forceDodgeAnim)
			{
				self->client->ps.forceDodgeAnim = 1;
				self->client->ps.forceHandExtendTime = level.time + 1000;
				G_EntitySound(self, CHAN_VOICE, G_SoundIndex("*jump1.wav"));
				self->client->ps.velocity[2] = 100;
			}
			else if (!self->client->ps.forceDodgeAnim)
			{
				self->client->ps.forceHandExtendTime = level.time + 100;
			}
			else
			{
				self->client->ps.forceHandExtend = HANDEXTEND_WEAPONREADY;
			}
		}
		else if (self->client->ps.forceHandExtend == HANDEXTEND_DODGE)
		{
			//don't do the HANDEXTEND_WEAPONREADY since it screws up our saber block code.
			self->client->ps.forceHandExtend = HANDEXTEND_NONE;
		}
		else
		{
			self->client->ps.forceHandExtend = HANDEXTEND_WEAPONREADY;
		}
	}

	if (level.gametype == GT_HOLOCRON)
	{
		HolocronUpdate(self);
	}
	if (level.gametype == GT_JEDIMASTER)
	{
		JediMasterUpdate(self);
	}

	SeekerDroneUpdate(self);

	if (self->client->ps.powerups[PW_FORCE_BOON])
	{
		prepower = self->client->ps.fd.forcePower;
	}

	if (self && self->client && (BG_HasYsalamiri(level.gametype, &self->client->ps) ||
		self->client->ps.fd.forceDeactivateAll || self->client->tempSpectate >= level.time))
	{
		//has ysalamiri->. or we want to forcefully stop all his active powers
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			if (self->client->ps.fd.forcePowersActive & 1 << i && i != FP_LEVITATION)
			{
				WP_ForcePowerStop(self, i);
			}

			i++;
		}

		if (self->client->tempSpectate >= level.time)
		{
			self->client->ps.fd.forcePower = 100;
			self->client->ps.fd.forceRageRecoveryTime = 0;
			self->client->ps.fd.forceSpeedRecoveryTime = 0;
		}

		self->client->ps.fd.forceDeactivateAll = 0;

		if (self->client->ps.fd.forceJumpCharge)
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);
			self->client->ps.fd.forceJumpCharge = 0;
		}
	}
	else
	{
		//otherwise just do a check through them all to see if they need to be stopped for any reason.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			if (self->client->ps.fd.forcePowersActive & 1 << i && i != FP_LEVITATION &&
				!BG_CanUseFPNow(level.gametype, &self->client->ps, level.time, i))
			{
				WP_ForcePowerStop(self, i);
			}

			i++;
		}
	}

	//i = 0;

	if (self->client->ps.powerups[PW_FORCE_ENLIGHTENED_LIGHT] || self->client->ps.powerups[PW_FORCE_ENLIGHTENED_DARK])
	{
		//enlightenment
		if (!self->client->ps.fd.forceUsingAdded)
		{
			i = 0;

			while (i < NUM_FORCE_POWERS)
			{
				self->client->ps.fd.forcePowerBaseLevel[i] = self->client->ps.fd.forcePowerLevel[i];

				if (!force_power_dark_light[i] ||
					self->client->ps.fd.forceSide == force_power_dark_light[i])
				{
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;
					self->client->ps.fd.forcePowersKnown |= 1 << i;
				}

				i++;
			}

			self->client->ps.fd.forceUsingAdded = 1;
		}
	}
	else if (self->client->ps.fd.forceUsingAdded)
	{
		//we don't have enlightenment but we're still using enlightened powers, so clear them back to how they should be.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			self->client->ps.fd.forcePowerLevel[i] = self->client->ps.fd.forcePowerBaseLevel[i];
			if (!self->client->ps.fd.forcePowerLevel[i])
			{
				if (self->client->ps.fd.forcePowersActive & 1 << i)
				{
					WP_ForcePowerStop(self, i);
				}
				self->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}

			i++;
		}

		self->client->ps.fd.forceUsingAdded = 0;
	}

	//i = 0;

	if (!(self->client->ps.fd.forcePowersActive & 1 << FP_TELEPATHY))
	{
		//clear the mindtrick index values
		self->client->ps.fd.forceMindtrickTargetIndex = 0;
		self->client->ps.fd.forceMindtrickTargetIndex2 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex3 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex4 = 0;
	}

	if (self->health < 1)
	{
		self->client->ps.fd.forceGripBeingGripped = 0;
	}

	if (self->client->ps.fd.forceGripBeingGripped > level.time)
	{
		self->client->ps.fd.forceGripCripple = 1;

		//keep the saber off during this period
		if (self->client->ps.weapon == WP_SABER && !self->client->ps.saberHolstered)
		{
			Cmd_ToggleSaber_f(self);
		}
	}
	else
	{
		self->client->ps.fd.forceGripCripple = 0;
	}

	if (self->client->ps.fd.forceJumpSound)
	{
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
		self->client->ps.fd.forceJumpSound = 0;
	}

	if (self->client->ps.fd.forceGripCripple)
	{
		if (self->client->ps.fd.forceGripSoundTime < level.time)
		{
			G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEGRIP);
			self->client->ps.fd.forceGripSoundTime = level.time + 1000;
		}
	}

	if (self->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
	{
		self->client->ps.powerups[PW_SPEED] = level.time + 100;
	}

	if (self->health <= 0)
	{
		//if dead, deactivate any active force powers
		for (i = 0; i < NUM_FORCE_POWERS; i++)
		{
			if (self->client->ps.fd.forcePowerDuration[i] || self->client->ps.fd.forcePowersActive & 1 << i)
			{
				WP_ForcePowerStop(self, i);
				self->client->ps.fd.forcePowerDuration[i] = 0;
			}
		}
		goto powersetcheck;
	}

	if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		self->client->fjDidJump = qfalse;
	}

	if (self->client->ps.fd.forceJumpCharge && self->client->ps.groundEntityNum == ENTITYNUM_NONE && self->client->
		fjDidJump)
	{
		//this was for the "charge" jump method... I guess
		if (ucmd->upmove < 10 && (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected !=
			FP_LEVITATION))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);
			self->client->ps.fd.forceJumpCharge = 0;
		}
	}

#ifndef METROID_JUMP
	else if ((ucmd->upmove > 10) && (self->client->ps.pm_flags & PMF_JUMP_HELD) && self->client->ps.groundTime && (level.time - self->client->ps.groundTime) > 150 && !BG_HasYsalamiri(level.gametype, &self->client->ps) && BG_CanUseFPNow(level.gametype, &self->client->ps, level.time, FP_LEVITATION))
	{//just charging up
		ForceJumpCharge(self, ucmd);
	}
	else if (ucmd->upmove < 10 && self->client->ps.groundEntityNum == ENTITYNUM_NONE && self->client->ps.fd.forceJumpCharge)
	{
		self->client->ps.pm_flags &= ~(PMF_JUMP_HELD);
	}
#endif

	if (!(self->client->ps.pm_flags & PMF_JUMP_HELD) && self->client->ps.fd.forceJumpCharge)
	{
		if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_LEVITATION)
		{
			WP_DoSpecificPower(self, ucmd, FP_LEVITATION);
		}
	}

	if (self->client->flameTime > level.time)
	{
		//flamethrower is active, flip active flamethrower flag
		if (self->client->ps.jetpackFuel < 15)
		{
			//not enough gas, turn it off.
			self->client->flameTime = 0;
			self->client->ps.PlayerEffectFlags &= ~(1 << PEF_FLAMING);
		}
		else
		{
			//fire flamethrower
			self->client->ps.PlayerEffectFlags |= 1 << PEF_FLAMING;
			self->client->ps.forceHandExtend = HANDEXTEND_FLAMETHROWER_HOLD;
			self->client->ps.forceHandExtendTime = level.time + 100;

			if (FlamethrowerDebounceTime == level.time //someone already advanced the timer this frame
				|| level.time - FlamethrowerDebounceTime >= FLAMETHROWERDEBOUNCE)
			{
				G_Sound(self, CHAN_WEAPON, G_SoundIndex("sound/effects/fireburst"));
				Flamethrower_Fire(self);
				FlamethrowerDebounceTime = level.time;

				if (!(self->r.svFlags & SVF_BOT))
				{
					self->client->ps.jetpackFuel -= FLAMETHROWER_FUELCOST;
				}
			}
		}
	}
	else
	{
		self->client->ps.PlayerEffectFlags &= ~(1 << PEF_FLAMING);
	}

	if (self->client->pers.botclass == BCLASS_MANDOLORIAN
		|| self->client->pers.botclass == BCLASS_BOBAFETT
		|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
		|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
	{
		//Boba Fett
		if (ucmd->buttons & BUTTON_FORCEGRIP)
		{
			//start wrist laser
			Boba_FireWristMissile(self, BOBA_MISSILE_LASER);
			return;
		}
		if (self->client->ps.fd.forcePowerDuration[FP_GRIP])
		{
			Boba_EndWristMissile(self, BOBA_MISSILE_LASER);
			return;
		}
	}
	else if (ucmd->buttons & BUTTON_FORCEGRIP)
	{
		//grip is one of the powers with its own button.. if it's held, call the specific grip power function.
		WP_DoSpecificPower(self, ucmd, FP_GRIP);
	}
	else
	{
		//see if we're using it generically.. if not, stop.
		if (self->client->ps.fd.forcePowersActive & 1 << FP_GRIP)
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_GRIP)
			{
				WP_ForcePowerStop(self, FP_GRIP);
			}
		}
	}

	if (self->client->ps.communicatingflags & 1 << DASHING)
	{//dash is one of the powers with its own button.. if it's held, call the specific dash power function.
		ForceSpeedDash(self);
	}

	if (ucmd->buttons & BUTTON_FORCE_LIGHTNING)
	{
		//lightning
		WP_DoSpecificPower(self, ucmd, FP_LIGHTNING);
	}
	else
	{
		//see if we're using it generically.. if not, stop.
		if (self->client->ps.fd.forcePowersActive & 1 << FP_LIGHTNING)
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_LIGHTNING)
			{
				WP_ForcePowerStop(self, FP_LIGHTNING);
			}
		}
	}

	if (ucmd->buttons & BUTTON_FORCE_DRAIN)
	{
		//drain
		WP_DoSpecificPower(self, ucmd, FP_DRAIN);
	}
	else
	{
		//see if we're using it generically.. if not, stop.
		if (self->client->ps.fd.forcePowersActive & 1 << FP_DRAIN)
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_DRAIN)
			{
				WP_ForcePowerStop(self, FP_DRAIN);
			}
		}
	}

	if (ucmd->buttons & BUTTON_FORCEPOWER &&
		BG_CanUseFPNow(level.gametype, &self->client->ps, level.time, self->client->ps.fd.forcePowerSelected))
	{
		if (self->client->ps.fd.forcePowerSelected == FP_LEVITATION)
			ForceJumpCharge(self, ucmd);
		else
		{
			WP_DoSpecificPower(self, ucmd, self->client->ps.fd.forcePowerSelected);
		}
	}
	else
	{
		self->client->ps.fd.forceButtonNeedRelease = 0;
	}

	for (i = 0; i < NUM_FORCE_POWERS; i++)
	{
		if (self->client->ps.fd.forcePowerDuration[i])
		{
			if (self->client->ps.fd.forcePowerDuration[i] < level.time)
			{
				if (self->client->ps.fd.forcePowersActive & 1 << i)
				{
					//turn it off
					WP_ForcePowerStop(self, i);
				}
				self->client->ps.fd.forcePowerDuration[i] = 0;
			}
		}
		if (self->client->ps.fd.forcePowersActive & 1 << i)
		{
			using_force = qtrue;
			wp_force_power_run(self, i, ucmd);
		}
	}

	if (!(self->client->ps.fd.forcePowersActive & 1 << FP_DRAIN))
	{
		self->client->force.drainDebounce = level.time;
	}
	if (!(self->client->ps.fd.forcePowersActive & 1 << FP_LIGHTNING))
	{
		self->client->force.lightningDebounce = level.time;
	}
	if (self->client->ps.saberInFlight)
	{
		//don't regen force power while throwing saber
		if (self->client->ps.saberEntityNum < ENTITYNUM_NONE && self->client->ps.saberEntityNum > 0) //player is 0
		{
			//
			if (&g_entities[self->client->ps.saberEntityNum] != NULL && g_entities[self->client->ps.saberEntityNum].s.
				pos.trType == TR_LINEAR)
			{
				//fell to the ground and we're trying to pull it back
				using_force = qtrue;
			}
		}
	}
	if (PM_ForceUsingSaberAnim(self->client->ps.torsoAnim))
	{
		using_force = qtrue;
	}

	const qboolean is_holding_block_button_and_attack = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
	const qboolean is_holding_block_button = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking

	if (!using_force
		&& !PM_InKnockDown(&self->client->ps)
		&& walk_check(self)
		&& self->client->ps.weaponTime <= 0
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		//when not using the force, regenerate at 1 point per half second
		while (self->client->ps.fd.forcePowerRegenDebounceTime < level.time)
		{
			if (level.gametype != GT_HOLOCRON || g_maxHolocronCarry.value)
			{
				if (self->client->ps.powerups[PW_FORCE_BOON])
				{
					wp_force_power_regenerate(self, 6);
				}
				else if (self->client->ps.isJediMaster && level.gametype == GT_JEDIMASTER)
				{
					wp_force_power_regenerate(self, 4); //jedi master regenerates 4 times as fast
				}
				else if (PM_RestAnim(self->client->ps.legsAnim))
				{
					wp_force_power_regenerate(self, 10);
					BG_ReduceSaberMishapLevel(&self->client->ps);
					self->client->ps.powerups[PW_MEDITATE] = level.time + self->client->ps.torsoTimer + 3000;
				}
				else if (PM_CrouchAnim(self->client->ps.legsAnim))
				{
					wp_force_power_regenerate(self, 8);
					BG_ReduceSaberMishapLevel(&self->client->ps);
				}
				else if (is_holding_block_button || is_holding_block_button_and_attack)
				{
					//regen half as fast
					self->client->ps.fd.forcePowerRegenDebounceTime += 2000; //1 point per 1 seconds.. super slow
					wp_force_power_regenerate(self, 1);
				}
				else if (self->client->ps.saberInFlight)
				{
					//regen half as fast
					self->client->ps.fd.forcePowerRegenDebounceTime += 2000; //1 point per 1 seconds.. super slow
					wp_force_power_regenerate(self, 1);
				}
				else if (PM_SaberInAttack(self->client->ps.saber_move)
					|| pm_saber_in_special_attack(self->client->ps.torsoAnim)
					|| PM_SpinningSaberAnim(self->client->ps.torsoAnim)
					|| PM_SaberInParry(self->client->ps.saber_move)
					|| PM_SaberInReturn(self->client->ps.saber_move))
				{
					//regen half as fast
					self->client->ps.fd.forcePowerRegenDebounceTime += 4000; //1 point per 1 seconds.. super slow
					wp_force_power_regenerate(self, 1);
				}
				else
				{
					wp_force_power_regenerate(self, 0);
				}
			}
			else
			{
				//regenerate based on the number of holocrons carried
				int holoregen = 0;
				int holo = 0;
				while (holo < NUM_FORCE_POWERS)
				{
					if (self->client->ps.holocronsCarried[holo])
						holoregen++;
					holo++;
				}

				wp_force_power_regenerate(self, holoregen);
			}

			if (level.gametype == GT_SIEGE)
			{
				if (self->client->holdingObjectiveItem && g_entities[self->client->holdingObjectiveItem].inuse &&
					g_entities[self->client->holdingObjectiveItem].genericValue15)
					self->client->ps.fd.forcePowerRegenDebounceTime += 7000; //1 point per 7 seconds.. super slow
				else if (self->client->siegeClass != -1 && bgSiegeClasses[self->client->siegeClass].classflags & 1 <<
					CFL_FASTFORCEREGEN)
					self->client->ps.fd.forcePowerRegenDebounceTime += Q_max(g_forceRegenTime.integer * 0.2, 1);
				//if this is siege and our player class has the fast force regen ability, then recharge with 1/5th the usual delay
				else
					self->client->ps.fd.forcePowerRegenDebounceTime += Q_max(g_forceRegenTime.integer, 1);
			}
			else
			{
				if (level.gametype == GT_POWERDUEL && self->client->sess.duelTeam == DUELTEAM_LONE)
				{
					if (duel_fraglimit.integer)
						self->client->ps.fd.forcePowerRegenDebounceTime += Q_max(
							g_forceRegenTime.integer * (0.6 + (.3 * (float)self->client->sess.wins / (float)
								duel_fraglimit.integer)), 1);
					else
						self->client->ps.fd.forcePowerRegenDebounceTime += Q_max(g_forceRegenTime.integer * 0.7, 1);
				}
				else
					self->client->ps.fd.forcePowerRegenDebounceTime += Q_max(g_forceRegenTime.integer, 1);
			}
		}
		if (self->client->ps.fd.forcePower > self->client->ps.fd.forcePowerMax * FATIGUEDTHRESHHOLD)
		{
			//You gained some FP back.  Cancel the Fatigue status.
			self->client->ps.userInt3 &= ~(1 << FLAG_FATIGUED);
		}
	}
	else
	{
		self->client->ps.fd.forcePowerRegenDebounceTime = level.time;
	}

	if (self->client->DodgeDebounce < level.time
		&& !BG_InSlowBounce(&self->client->ps) && !PM_SaberInBrokenParry(self->client->ps.saber_move)
		&& !PM_InKnockDown(&self->client->ps) && self->client->ps.forceHandExtend != HANDEXTEND_DODGE
		&& self->client->ps.saberLockTime < level.time //not in a saber lock.
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE //can't regen while in the air.
		&& walk_check(self))
	{
		if (self->client->ps.fd.forcePower > self->client->ps.fd.forcePowerMax * FATIGUEDTHRESHHOLD + 1
			&& self->client->ps.stats[STAT_DODGE] < self->client->ps.stats[STAT_MAX_DODGE])
		{
			//you have enough fatigue to transfer to Dodge
			if (self->client->ps.stats[STAT_MAX_DODGE] - self->client->ps.stats[STAT_DODGE] < DODGE_FATIGUE)
			{
				self->client->ps.stats[STAT_DODGE] = self->client->ps.stats[STAT_MAX_DODGE];
			}
			else
			{
				self->client->ps.stats[STAT_DODGE] += DODGE_FATIGUE;
			}
			self->client->ps.fd.forcePower--;
		}

		self->client->DodgeDebounce = level.time + g_dodgeRegenTime.integer;
	}

	if (self->client->MishapDebounce < level.time
		&& !BG_InSlowBounce(&self->client->ps) && !PM_SaberInBrokenParry(self->client->ps.saber_move)
		&& !PM_InKnockDown(&self->client->ps) && self->client->ps.forceHandExtend != HANDEXTEND_DODGE
		&& self->client->ps.saberLockTime < level.time //not in a saber lock.
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE) //can't regen while in the air.
	{
		if (self->client->ps.saberFatigueChainCount > MISHAPLEVEL_NONE)
		{
			self->client->ps.saberFatigueChainCount--;
		}

		if (self->client->ps.weapon == WP_SABER)
		{
			//saberer regens slower since they use MP differently
			if (self->client->ps.fd.saberAnimLevel == SS_MEDIUM)
			{
				//yellow style is more "centered" and recovers MP faster.
				self->client->MishapDebounce = level.time + g_mishapRegenTime.integer * .75;
			}
			else
			{
				self->client->MishapDebounce = level.time + g_mishapRegenTime.integer;
			}
		}
		else
		{
			//gunner regen faster
			self->client->MishapDebounce = level.time + g_mishapRegenTime.integer / 5;
		}
	}

powersetcheck:

	if (prepower && self->client->ps.fd.forcePower < prepower)
	{
		int dif = (prepower - self->client->ps.fd.forcePower) / 2;
		if (dif < 1)
		{
			dif = 1;
		}

		self->client->ps.fd.forcePower = prepower - dif;
	}
}

void WP_BlockPointsUpdate(const gentity_t* self)
{
	const qboolean is_holding_block_button = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking

	if (!(self->r.svFlags & SVF_BOT))
	{
		if (!PM_InKnockDown(&self->client->ps)
			&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
			&& walk_check(self)
			&& !PM_SaberInAttack(self->client->ps.saber_move)
			&& !pm_saber_in_special_attack(self->client->ps.torsoAnim)
			&& !PM_SpinningSaberAnim(self->client->ps.torsoAnim)
			&& !PM_SaberInParry(self->client->ps.saber_move)
			&& !PM_SaberInReturn(self->client->ps.saber_move)
			&& !PM_Saberinstab(self->client->ps.saber_move)
			&& !is_holding_block_button
			&& !(self->client->buttons & BUTTON_BLOCK))
		{
			//when not using the block, regenerate at 10 points per second
			if (self->client->ps.fd.BlockPointsRegenDebounceTime < level.time)
			{
				wp_block_points_regenerate(self, self->client->ps.fd.BlockPointRegenAmount);

				self->client->ps.fd.BlockPointsRegenDebounceTime = level.time + self->client->ps.fd.BlockPointRegenRate;

				if (PM_RestAnim(self->client->ps.legsAnim))
				{
					wp_block_points_regenerate(self, 4);
					self->client->ps.powerups[PW_MEDITATE] = level.time + self->client->ps.torsoTimer + 3000;
				}
				else if (PM_CrouchAnim(self->client->ps.legsAnim))
				{
					wp_block_points_regenerate(self, 2);
				}
				else
				{
					if (self->client->ps.fd.forceRageRecoveryTime >= level.time)
					{
						//regen half as fast
						self->client->ps.fd.BlockPointsRegenDebounceTime += self->client->ps.fd.BlockPointRegenRate;
					}
					else if (self->client->ps.fd.blockPoints > BLOCKPOINTS_MISSILE) //slows down for the last 25 points
					{
						//regen half as fast
						self->client->ps.fd.BlockPointsRegenDebounceTime += 2000;
					}
					else if (self->client->ps.saberInFlight) //slows down
					{
						//regen half as fast
						self->client->ps.fd.BlockPointsRegenDebounceTime += 2000;
					}
					else if (self->client->ps.weaponTime <= 0) //slows down
					{
						//regen half as fast
						self->client->ps.fd.BlockPointsRegenDebounceTime += 2000;
					}
				}
			}
		}
	}
	else
	{
		//when not using the block, regenerate at 10 points per second
		if (!PM_InKnockDown(&self->client->ps)
			&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
			//&& WalkCheck(self)
			&& !(PM_SaberInAttack(self->client->ps.saber_move)
				//|| PM_SaberInSpecialAttack(self->client->ps.torsoAnim)
				//|| PM_SpinningSaberAnim(self->client->ps.torsoAnim)
				//|| PM_SaberInParry(self->client->ps.saber_move)
				//|| PM_SaberInReturn(self->client->ps.saber_move)
				|| PM_Saberinstab(self->client->ps.saber_move)))
		{
			if (self->client->ps.fd.BlockPointsRegenDebounceTime < level.time)
			{
				wp_block_points_regenerate_over_ride(self, self->client->ps.fd.BlockPointRegenAmount);

				self->client->ps.fd.BlockPointsRegenDebounceTime = level.time + self->client->ps.fd.BlockPointRegenRate;

				if (self->client->ps.fd.forceRageRecoveryTime >= level.time)
				{
					//regen half as fast
					self->client->ps.fd.BlockPointsRegenDebounceTime += self->client->ps.fd.BlockPointRegenRate;
				}
				else if (self->client->ps.fd.blockPoints > BLOCKPOINTS_MISSILE) //slows down for the last 25 points
				{
					//regen half as fast
					self->client->ps.fd.BlockPointsRegenDebounceTime += 2000;
				}
				else if (self->client->ps.saberInFlight) //slows down
				{
					//regen half as fast
					self->client->ps.fd.BlockPointsRegenDebounceTime += 2000;
				}
				else if (self->client->ps.weaponTime <= 0) //slows down
				{
					//regen half as fast
					self->client->ps.fd.BlockPointsRegenDebounceTime += 2000;
				}
			}
		}
	}
}

qboolean Jedi_DrainReaction(gentity_t* self)
{
	if (self->health > 0 && !PM_InKnockDown(&self->client->ps)
		&& self->client->ps.torsoAnim != BOTH_FORCE_DRAIN_GRABBED)
	{
		//still alive
		if (self->client->ps.stats[STAT_HEALTH] < 50)
		{
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_SONICPAIN_HOLD, SETANIM_AFLAG_PACE, 0);
		}
		else
		{
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_TORSO, BOTH_WIND, SETANIM_AFLAG_PACE, 0);
		}

		self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
		self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
		self->client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
		self->client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCBLOCKING);
	}
	return qfalse;
}

qboolean jedi_dodge_evasion(gentity_t* self, const gentity_t* shooter, trace_t* tr, const int hit_loc)
{
	int dodge_anim = -1;

	if (in_camera)
	{
		return qfalse;
	}

	if (!self || !self->client || self->health <= 0)
	{
		return qfalse;
	}

	if (self->NPC
		&& (self->client->NPC_class == CLASS_STORMTROOPER ||
			self->client->NPC_class == CLASS_STORMCOMMANDO ||
			self->client->NPC_class == CLASS_SWAMPTROOPER ||
			self->client->NPC_class == CLASS_CLONETROOPER ||
			self->client->NPC_class == CLASS_IMPWORKER ||
			self->client->NPC_class == CLASS_IMPERIAL ||
			self->client->NPC_class == CLASS_SABER_DROID ||
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

	if (self->client->ps.weapon == WP_BRYAR_OLD)
	{
		return qfalse; //dont do
	}

	if (!g_forceDodge.integer)
	{
		return qfalse;
	}

	if (g_forceDodge.integer != 2)
	{
		if (!(self->client->ps.fd.forcePowersActive & 1 << FP_SEE))
		{
			return qfalse;
		}
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

	if (PM_InKnockDown(&self->client->ps) ||
		(BG_InRoll(&self->client->ps, self->client->ps.legsAnim) ||
			BG_InKnockDown(self->client->ps.legsAnim)))
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

	if (BG_IsAlreadyinTauntAnim(self->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (self->client->ps.weapon == WP_SABER)
	{
		return qfalse; //dont do THIS IS FOR GUNNERS ONLY NOW LIKE SP
	}

	if (PM_InKnockDown(&self->client->ps) ||
		(BG_InRoll(&self->client->ps, self->client->ps.legsAnim) ||
			BG_InKnockDown(self->client->ps.legsAnim)))
	{
		//can't Dodge while knocked down or getting up from knockdown.
		return qfalse;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//can't dodge in mid-air
		return qfalse;
	}

	if (self->r.svFlags & SVF_BOT
		&& (self->client->pers.botclass != BCLASS_MANDOLORIAN &&
			self->client->pers.botclass != BCLASS_MANDOLORIAN1 &&
			self->client->pers.botclass != BCLASS_MANDOLORIAN2 &&
			self->client->pers.botclass != BCLASS_BOBAFETT) &&
		self->client->ps.fd.forcePower < FATIGUE_DODGEINGBOT)
	{
		//must have enough force power
		return qfalse;
	}
	if (self->client->ps.fd.forcePower < FATIGUE_DODGE)
	{
		//Not enough dodge
		return qfalse;
	}

	if (self->r.svFlags & SVF_BOT
		&& (self->client->pers.botclass != BCLASS_MANDOLORIAN &&
			self->client->pers.botclass != BCLASS_MANDOLORIAN1 &&
			self->client->pers.botclass != BCLASS_MANDOLORIAN2 &&
			self->client->pers.botclass != BCLASS_BOBAFETT))
	{
		PM_AddFatigue(&self->client->ps, FATIGUE_DODGEINGBOT);
	}
	else
	{
		PM_AddFatigue(&self->client->ps, FATIGUE_DODGEING);
	}

	if (self->client->ps.weaponTime > 0 || self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		//in some effect that stops me from moving on my own
		return qfalse;
	}

	if (g_forceDodge.integer == 2)
	{
		if (Q_irand(1, 7) > self->client->ps.fd.forcePowerLevel[FP_SPEED])
		{
			//more likely to fail on lower force speed level
			return qfalse;
		}
		if (!WP_ForcePowerUsable(self, FP_SPEED))
		{
			//make sure we have it and have enough force power
			return qfalse;
		}
		if (self->client->ps.fd.forcePowersActive)
		{
			//for now just don't let us dodge if we're using a force power at all
			return qfalse;
		}
	}
	else
	{
		//We now dodge all the time, but only on level 3
		if (self->client->ps.fd.forcePowerLevel[FP_SEE] < FORCE_LEVEL_3)
		{
			//more likely to fail on lower force sight level
			return qfalse;
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
		//Our own happy way of forcing an anim:
		self->client->ps.forceHandExtend = HANDEXTEND_DODGE;
		self->client->ps.forceDodgeAnim = dodge_anim;
		self->client->ps.forceHandExtendTime = level.time + 300;
		self->client->ps.weaponTime = 300;
		self->client->ps.saber_move = LS_NONE;
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/melee/swing4.wav"));
		return qtrue;
	}
	return qfalse;
}

extern qboolean G_GetHitLocFromSurfName(gentity_t* ent, const char* surfName, int* hit_loc, vec3_t point, vec3_t dir,
	vec3_t blade_dir, int mod);
extern int G_GetHitLocation(const gentity_t* target, vec3_t ppoint);

qboolean jedi_disruptor_dodge_evasion(gentity_t* self, gentity_t* shooter, vec3_t dmg_origin, int hit_loc)
{
	int dodge_anim = -1;

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
		//Our own happy way of forcing an anim:
		self->client->ps.forceHandExtend = HANDEXTEND_DODGE;
		self->client->ps.forceDodgeAnim = dodge_anim;
		self->client->ps.forceHandExtendTime = level.time + 300;
		self->client->ps.weaponTime = 300;
		self->client->ps.saber_move = LS_NONE;
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/melee/swing4.wav"));
		return qtrue;
	}
	return qfalse;
}