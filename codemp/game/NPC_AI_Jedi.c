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

#include "b_local.h"
#include "g_nav.h"
#include "anims.h"
#include "w_saber.h"
#include "bg_weapons.h"

extern qboolean BG_SabersOff(const playerState_t* ps);

extern void CG_DrawAlert(vec3_t origin, float rating);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
extern void ForceJump(gentity_t* self, const usercmd_t* ucmd);
extern qboolean PM_InRoll(const playerState_t* ps);
extern void WP_ResistForcePush(gentity_t* self, const gentity_t* pusher, qboolean no_penalty);
extern qboolean G_EntIsBreakable(int entityNum);

#define	MAX_VIEW_DIST		2048
#define MAX_VIEW_SPEED		100
#define	JEDI_MAX_LIGHT_INTENSITY 64
#define	JEDI_MIN_LIGHT_THRESHOLD 10
#define	JEDI_MAX_LIGHT_THRESHOLD 50

#define	DISTANCE_SCALE		0.25f
#define	SPEED_SCALE			0.25f
#define	FOV_SCALE			0.5f
#define	LIGHT_SCALE			0.25f

#define	REALIZE_THRESHOLD	0.6f
#define CAUTIOUS_THRESHOLD	( REALIZE_THRESHOLD * 0.3 )

#define	MAX_CHECK_THRESHOLD	1

extern void NPC_ClearLookTarget(const gentity_t* self);
extern void NPC_SetLookTarget(const gentity_t* self, int entNum, int clear_time);
extern void NPC_TempLookTarget(const gentity_t* self, int lookEntNum, int minLookTime, int maxLookTime);
extern qboolean G_ExpandPointToBBox(vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask);
extern void G_SoundOnEnt(gentity_t* ent, soundChannel_t channel, const char* sound_path);
void Boba_FireDecide(void);
extern void Player_CheckBurn(const gentity_t* self);
extern void player_Burn(const gentity_t* self);

extern gitem_t* BG_FindItemForAmmo(const ammo_t ammo);
extern void PM_AddFatigue(playerState_t* ps, int fatigue);

extern void ForceThrow(gentity_t* self, qboolean pull);
extern void ForceLightning(gentity_t* self);
extern void ForceHeal(gentity_t* self);
extern void ForceRage(gentity_t* self);
extern void ForceProtect(gentity_t* self);
extern void ForceAbsorb(gentity_t* self);
extern int WP_MissileBlockForBlock(int saber_block);
extern qboolean G_GetHitLocFromSurfName(gentity_t* ent, const char* surfName, int* hit_loc, vec3_t point, vec3_t dir,
	vec3_t blade_dir, int mod);
extern qboolean WP_ForcePowerUsable(const gentity_t* self, forcePowers_t forcePower);
extern qboolean WP_ForcePowerAvailable(const gentity_t* self, forcePowers_t forcePower, int overrideAmt);
extern void WP_ForcePowerStop(gentity_t* self, forcePowers_t forcePower);
extern void WP_DeactivateSaber(const gentity_t* self);
extern void WP_ActivateSaber(gentity_t* self);

extern qboolean PM_SaberInStart(int move);
extern qboolean pm_saber_in_special_attack(int anim);
extern qboolean PM_SaberInAttack(int move);
extern qboolean PM_SaberInBounce(int move);
extern qboolean PM_SaberInParry(int move);
extern qboolean PM_SaberInKnockaway(int move);
extern qboolean PM_SaberInBrokenParry(int move);
extern qboolean PM_SaberInDeflect(int move);
extern qboolean PM_SpinningSaberAnim(int anim);
extern qboolean PM_FlippingAnim(int anim);
extern qboolean PM_RollingAnim(int anim);
extern qboolean PM_InKnockDown(const playerState_t* ps);
extern qboolean BG_InRoll(const playerState_t* ps, int anim);
extern qboolean PM_CrouchAnim(int anim);
void NPC_CheckEvasion(void);
extern void ForceDashAnimDash(gentity_t* self);

extern qboolean NPC_SomeoneLookingAtMe(gentity_t* ent);

extern int WP_GetVelocityForForceJump(const gentity_t* self, vec3_t jump_vel, const usercmd_t* ucmd);

extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);

qboolean Jedi_WaitingAmbush(const gentity_t* self);
extern qboolean in_camera;

extern int bg_parryDebounce[];

static int jediSpeechDebounceTime[TEAM_NUM_TEAMS]; //used to stop several jedi from speaking all at once
//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_UNDERFIRE,
	LSTATE_INVESTIGATE,
};

enum
{
	BTS_NONE,

	// Attack
	//--------
	BTS_RIFLE,
	// Uses Jedi / Seeker Movement
	BTS_MISSILE,
	// Uses Jedi / Seeker Movement
	BTS_SNIPER,
	// Uses Special Movement Internal To This File
	BTS_FLAMETHROW,
	// Locked In Place
	// Waiting
	//---------
	BTS_AMBUSHWAIT,
	// Goto CP & Wait

	BTS_MAX
};

void npc_cultist_destroyer_precache(void)
{
	//precashe for cultist destroyer
	G_SoundIndex("sound/movers/objects/green_beam_lp2.wav");
	G_EffectIndex("explosions/droidexplosion1");
}

void npc_shadow_trooper_precache(void)
{
	register_item(BG_FindItemForAmmo(AMMO_FORCE));
	G_SoundIndex("sound/chars/shadowtrooper/cloak.wav");
	G_SoundIndex("sound/chars/shadowtrooper/decloak.wav");
}

void npc_rosh_dark_precache(void)
{
	//precashe for Rosh boss
	G_EffectIndex("force/kothos_recharge.efx");
	G_EffectIndex("force/kothos_beam.efx");
}

void Jedi_ClearTimers(const gentity_t* ent)
{
	TIMER_Set(ent, "roamTime", 0);
	TIMER_Set(ent, "chatter", 0);
	TIMER_Set(ent, "strafeLeft", 0);
	TIMER_Set(ent, "strafeRight", 0);
	TIMER_Set(ent, "noStrafe", 0);
	TIMER_Set(ent, "walking", 0);
	TIMER_Set(ent, "taunting", 0);
	TIMER_Set(ent, "parryTime", 0);
	TIMER_Set(ent, "parryReCalcTime", 0);
	TIMER_Set(ent, "forceJumpChasing", 0);
	TIMER_Set(ent, "jumpChaseDebounce", 0);
	TIMER_Set(ent, "moveforward", 0);
	TIMER_Set(ent, "moveback", 0);
	TIMER_Set(ent, "movenone", 0);
	TIMER_Set(ent, "moveright", 0);
	TIMER_Set(ent, "moveleft", 0);
	TIMER_Set(ent, "movecenter", 0);
	TIMER_Set(ent, "saberLevelDebounce", 0);
	TIMER_Set(ent, "noRetreat", 0);
	TIMER_Set(ent, "holdLightning", 0);
	TIMER_Set(ent, "gripping", 0);
	TIMER_Set(ent, "draining", 0);
	TIMER_Set(ent, "noturn", 0);
	TIMER_Set(ent, "specialEvasion", 0);
	TIMER_Set(ent, "blocking", 0);
	TIMER_Set(ent, "TalkTime", 0);
}

qboolean NPC_IsJedi(const gentity_t* self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_SITHLORD:
	case CLASS_DESANN:
	case CLASS_VADER:
	case CLASS_JEDI:
	case CLASS_KYLE:
	case CLASS_LUKE:
	case CLASS_MORGANKATARN:
	case CLASS_REBORN:
	case CLASS_SHADOWTROOPER:
	case CLASS_TAVION:
	case CLASS_ALORA:
	case CLASS_YODA:
		// Is Jedi...
		return qtrue;
	default:
		// NOT Jedi...
		break;
	}

	return qfalse;
}

qboolean npc_is_light_jedi(const gentity_t* self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_JEDI:
	case CLASS_KYLE:
	case CLASS_LUKE:
	case CLASS_MONMOTHA:
	case CLASS_MORGANKATARN:
	case CLASS_YODA:
		// Is Jedi...
		return qtrue;
	default:
		// NOT Jedi...
		break;
	}

	return qfalse;
}

qboolean npc_is_dark_jedi(const gentity_t* self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_ALORA:
	case CLASS_DESANN:
	case CLASS_REBORN:
	case CLASS_SHADOWTROOPER:
	case CLASS_TAVION:
	case CLASS_VADER:
	case CLASS_SITHLORD:
		// Is Jedi...
		return qtrue;
	default:
		// NOT Jedi...
		break;
	}

	return qfalse;
}

void Jedi_PlayBlockedPushSound(const gentity_t* self)
{
	if (self->s.number >= 0 && self->s.number < MAX_CLIENTS)
	{
		if (!npc_is_dark_jedi(self))
		{
			G_AddVoiceEvent(self, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
		}
		else
		{
			G_AddVoiceEvent(self, EV_PUSHFAIL, 3000);
		}
	}
	else if (self->health > 0 && self->NPC && self->NPC->blockedSpeechDebounceTime < level.time)
	{
		if (!npc_is_dark_jedi(self))
		{
			G_AddVoiceEvent(self, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
		}
		else
		{
			G_AddVoiceEvent(self, EV_PUSHFAIL, 3000);
		}
		self->NPC->blockedSpeechDebounceTime = level.time + 3000;
	}
}

void Jedi_PlayDeflectSound(const gentity_t* self)
{
	if (self->s.number >= 0 && self->s.number < MAX_CLIENTS)
	{
		if (!npc_is_dark_jedi(self))
		{
			G_AddVoiceEvent(self, Q_irand(EV_CHASE1, EV_CHASE3), 2000);
		}
		else
		{
			G_AddVoiceEvent(self, Q_irand(EV_DEFLECT1, EV_DEFLECT3), 3000);
		}
	}
	else if (self->health > 0 && self->NPC && self->NPC->blockedSpeechDebounceTime < level.time)
	{
		if (!npc_is_dark_jedi(self))
		{
			G_AddVoiceEvent(self, Q_irand(EV_CHASE1, EV_CHASE3), 2000);
		}
		else
		{
			G_AddVoiceEvent(self, Q_irand(EV_DEFLECT1, EV_DEFLECT3), 3000);
		}
		self->NPC->blockedSpeechDebounceTime = level.time + 3000;
	}
}

void NPC_Jedi_PlayConfusionSound(const gentity_t* self)
{
	if (self->health > 0)
	{
		if (self->client
			&& (self->client->NPC_class == CLASS_ALORA
				|| self->client->NPC_class == CLASS_TAVION
				|| self->client->NPC_class == CLASS_DESANN
				|| self->client->NPC_class == CLASS_SITHLORD
				|| self->client->NPC_class == CLASS_VADER))
		{
			G_AddVoiceEvent(self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000);
		}
		else if (Q_irand(0, 1))
		{
			if (!npc_is_dark_jedi(self))
			{
				G_AddVoiceEvent(self, Q_irand(EV_ESCAPING1, EV_ESCAPING3), 2000);
			}
			else
			{
				G_AddVoiceEvent(self, Q_irand(EV_TAUNT1, EV_TAUNT3), 2000);
			}
		}
		else
		{
			if (!npc_is_dark_jedi(self))
			{
				G_AddVoiceEvent(self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000);
			}
			else
			{
				G_AddVoiceEvent(self, Q_irand(EV_GLOAT1, EV_GLOAT3), 2000);
			}
		}
	}
}

qboolean Jedi_CultistDestroyer(const gentity_t* self)
{
	if (!self || !self->client)
	{
		return qfalse;
	}
	if (self->client->NPC_class == CLASS_REBORN &&
		self->s.weapon == WP_MELEE &&
		!Q_stricmp("cultist_destroyer", self->NPC_type))
	{
		return qtrue;
	}
	return qfalse;
}

void Boba_Precache(void)
{
	G_SoundIndex("sound/jetpack/ignite.wav");
	G_SoundIndex("sound/jetpack/idle.wav");
	G_SoundIndex("sound/jetpack/thrust.wav");
	G_SoundIndex("sound/chars/boba/bf_land.wav");
	G_SoundIndex("sound/weapons/boba/bf_flame.mp3");
	G_SoundIndex("sound/player/footsteps/boot1");
	G_SoundIndex("sound/player/footsteps/boot2");
	G_SoundIndex("sound/player/footsteps/boot3");
	G_SoundIndex("sound/player/footsteps/boot4");
	G_SoundIndex("sound/boba/jeton.wav");
	G_SoundIndex("sound/boba/jethover.wav");
	G_SoundIndex("sound/effects/combustfire.mp3");
	G_EffectIndex("boba/jet");
	G_EffectIndex("effects/flamethrower/flamethrower_mp.efx");
	G_EffectIndex("volumetric/black_smoke");
	G_EffectIndex("chunks/dustFall");
	G_EffectIndex("boba/fthrw");
	G_EffectIndex("flamethrower/flamethrower_mp");
	G_EffectIndex("flamethrower/flame_impact");
}

//===============================================================================================
//TAVION BOSS
//===============================================================================================
void NPC_TavionScepter_Precache(void)
{
	//precashe tavion boss scepter effects
	G_EffectIndex("scepter/beam_warmup.efx");
	G_EffectIndex("scepter/beam.efx");
	G_EffectIndex("scepter/slam_warmup.efx");
	G_EffectIndex("scepter/slam.efx");
	G_EffectIndex("scepter/impact.efx");
	G_SoundIndex("sound/weapons/scepter/loop.wav");
	G_SoundIndex("sound/weapons/scepter/slam_warmup.wav");
	G_SoundIndex("sound/weapons/scepter/beam_warmup.wav");
}

void NPC_TavionSithSword_Precache(void)
{
	//precashe tavion boss sithsword effects
	G_EffectIndex("scepter/recharge.efx");
	G_EffectIndex("scepter/invincibility.efx");
	G_EffectIndex("scepter/sword.efx");
	G_SoundIndex("sound/weapons/scepter/recharge.wav");
}

extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength, const qboolean breakSaberLock);

void Tavion_ScepterDamage(void)
{
	//Damage code for tavion's scepter weapon.
	if (!NPCS.NPC->ghoul2 || !NPCS.NPC->client->weaponGhoul2[1])
	{
		//sanity checks
		return;
	}

	if (NPCS.NPC->NPC->genericBolt1 != -1)
	{
		const int curTime = level.time;
		qboolean hit = qfalse;
		int lastHit = ENTITYNUM_NONE;

		for (int time = curTime - 25; time <= curTime + 25 && !hit; time += 25)
		{
			mdxaBone_t boltMatrix;
			vec3_t tip, dir, base, angles;
			trace_t trace;

			VectorSet(angles, 0, NPCS.NPC->r.currentAngles[YAW], 0);

			trap->G2API_GetBoltMatrix(NPCS.NPC->client->weaponGhoul2[1], 0,
				NPCS.NPC->NPC->genericBolt1,
				&boltMatrix, angles, NPCS.NPC->r.currentOrigin, time,
				NULL, NPCS.NPC->modelScale);
			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, base);
			BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, dir);
			VectorMA(base, 512, dir, tip);
			trap->Trace(&trace, base, vec3_origin, vec3_origin, tip, NPCS.NPC->s.number, MASK_SHOT, qfalse, 0, 0);
			if (trace.fraction < 1.0f)
			{
				//hit something
				gentity_t* traceEnt = &g_entities[trace.entityNum];

				//FIXME: too expensive!
				//if ( time == curTime )
				{
					//UGH
					G_PlayEffect(G_EffectIndex("scepter/impact.efx"), trace.endpos, trace.plane.normal);
				}

				if (traceEnt->takedamage
					&& trace.entityNum != lastHit
					&& (!traceEnt->client || traceEnt == NPCS.NPC->enemy || traceEnt->client->NPC_class != NPCS.NPC->
						client->NPC_class))
				{
					//smack
					const int dmg = Q_irand(10, 20) * (g_npcspskill.integer + 1); //NOTE: was 6-12
					G_Damage(traceEnt, NPCS.NPC, NPCS.NPC, vec3_origin, trace.endpos, dmg, DAMAGE_NO_KNOCKBACK,
						MOD_SABER); //MOD_MELEE );
					if (traceEnt->client)
					{
						if (!Q_irand(0, 2))
						{
							G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_CONFUSE1, EV_CONFUSE2), 10000);
						}
						else
						{
							G_AddVoiceEvent(NPCS.NPC, EV_JDETECTED3, 10000);
						}
						g_throw(traceEnt, dir, Q_flrand(50, 80));
						if (traceEnt->health > 0 && !Q_irand(0, 2)) //FIXME: base on skill!
						{
							//do pain on enemy
							G_Knockdown(traceEnt, NPCS.NPC, dir, 300, qtrue);
						}
					}
					hit = qtrue;
					lastHit = trace.entityNum;
				}
			}
		}
	}
}

void Tavion_ScepterSlam(void)
{
	if (!NPCS.NPC->ghoul2 || !NPCS.NPC->client->weaponGhoul2[1])
	{
		return;
	}

	const int boltIndex = trap->G2API_AddBolt(NPCS.NPC->client->weaponGhoul2[1], 0, "*weapon");
	if (boltIndex != -1)
	{
		mdxaBone_t boltMatrix;
		vec3_t handle, bottom, angles;
		trace_t trace;
		const float radius = 300.0f;
		const float halfRad = radius / 2;
		int i;
		vec3_t mins, maxs, entDir;
		int radius_ents[MAX_GENTITIES];

		VectorSet(angles, 0, NPCS.NPC->r.currentAngles[YAW], 0);

		trap->G2API_GetBoltMatrix(NPCS.NPC->ghoul2, 2,
			boltIndex,
			&boltMatrix, angles, NPCS.NPC->r.currentOrigin, level.time,
			NULL, NPCS.NPC->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, handle);
		VectorCopy(handle, bottom);
		bottom[2] -= 128.0f;

		trap->Trace(&trace, handle, vec3_origin, vec3_origin, bottom, NPCS.NPC->s.number, MASK_SHOT, qfalse, 0, 0);
		G_PlayEffect(G_EffectIndex("scepter/slam.efx"), trace.endpos, trace.plane.normal);

		//Setup the bbox to search in
		for (i = 0; i < 3; i++)
		{
			mins[i] = trace.endpos[i] - radius;
			maxs[i] = trace.endpos[i] + radius;
		}

		//Get the number of entities in a given space
		const int num_ents = trap->EntitiesInBox(mins, maxs, radius_ents, 128);

		for (i = 0; i < num_ents; i++)
		{
			gentity_t* radius_ent = &g_entities[radius_ents[i]];
			if (!radius_ent->inuse)
			{
				continue;
			}

			if (radius_ent->flags & FL_NO_KNOCKBACK)
			{
				//don't throw them back
				continue;
			}

			if (radius_ent == NPCS.NPC)
			{
				//Skip myself
				continue;
			}

			if (radius_ent->client == NULL)
			{
				//must be a client
				//RAFIXME: impliment
				if (G_EntIsBreakable(radius_ent->s.number))
				{
					//damage breakables within range, but not as much
					G_Damage(radius_ent, NPCS.NPC, NPCS.NPC, vec3_origin, radius_ent->r.currentOrigin, 100, 0,
						MOD_ROCKET_SPLASH);
				}
				continue;
			}

			if (radius_ent->client->ps.eFlags2 & EF2_HELD_BY_MONSTER)
			{
				//can't be one being held
				continue;
			}

			VectorSubtract(radius_ent->r.currentOrigin, trace.endpos, entDir);
			const float dist = VectorNormalize(entDir);
			if (dist <= radius)
			{
				if (dist < halfRad)
				{
					//close enough to do damage, too
					G_Damage(radius_ent, NPCS.NPC, NPCS.NPC, vec3_origin, radius_ent->r.currentOrigin, Q_irand(20, 30),
						DAMAGE_NO_KNOCKBACK, MOD_ROCKET_SPLASH);
				}
				if (radius_ent->client
					&& radius_ent->client->NPC_class != CLASS_RANCOR
					&& radius_ent->client->NPC_class != CLASS_ATST)
				{
					float throwStr = 0.0f;
					if (g_npcspskill.integer > 1)
					{
						throwStr = 10.0f + (radius - dist) / 2.0f;
						if (throwStr > 150.0f)
						{
							throwStr = 150.0f;
						}
					}
					else
					{
						throwStr = 10.0f + (radius - dist) / 4.0f;
						if (throwStr > 85.0f)
						{
							throwStr = 85.0f;
						}
					}
					entDir[2] += 0.1f;
					VectorNormalize(entDir);
					g_throw(radius_ent, entDir, throwStr);
					if (radius_ent->health > 0)
					{
						if (dist < halfRad
							|| radius_ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
						{
							//within range of my fist or within ground-shaking range and not in the air
							G_Knockdown(radius_ent, NPCS.NPC, vec3_origin, 500, qtrue);
						}
					}
				}
			}
		}
	}
}

void Tavion_StartScepterBeam(void)
{
	//Activate the scepter beam for the current NPC.
	//RAFIXME:  This probably needs to be moved to client side.
	mdxaBone_t boltMatrix;
	vec3_t dir, base, angles;

	VectorSet(angles, 0, NPCS.NPC->r.currentAngles[YAW], 0);

	trap->G2API_GetBoltMatrix(NPCS.NPC->client->weaponGhoul2[1], 0, NPCS.NPC->NPC->genericBolt1, &boltMatrix, angles,
		NPCS.NPC->r.currentOrigin, level.time, NULL, NPCS.NPC->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, base);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, dir);

	G_PlayEffect(G_EffectIndex("scepter/beam_warmup.efx"), base, dir);
	G_SoundOnEnt(NPCS.NPC, CHAN_ITEM, "sound/weapons/scepter/beam_warmup.wav");
	NPCS.NPC->client->ps.legsTimer = NPCS.NPC->client->ps.torsoTimer = 0;
	NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_SCEPTER_START, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	NPCS.NPC->client->ps.torsoTimer += 200;
	NPCS.NPC->painDebounceTime = level.time + NPCS.NPC->client->ps.torsoTimer;
	NPCS.NPC->client->ps.pm_time = NPCS.NPC->client->ps.torsoTimer;
	NPCS.NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	VectorClear(NPCS.NPC->client->ps.velocity);
	VectorClear(NPCS.NPC->client->ps.moveDir);
}

void Tavion_StartScepterSlam(void)
{
	mdxaBone_t boltMatrix;
	vec3_t dir, base, angles;

	VectorSet(angles, 0, NPCS.NPC->r.currentAngles[YAW], 0);

	trap->G2API_GetBoltMatrix(NPCS.NPC->client->weaponGhoul2[1], 0,
		NPCS.NPC->NPC->genericBolt1,
		&boltMatrix, angles, NPCS.NPC->r.currentOrigin, level.time,
		NULL, NPCS.NPC->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, base);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, dir);
	G_PlayEffect(G_EffectIndex("scepter/slam_warmup.efx"), base, dir);
	G_SoundOnEnt(NPCS.NPC, CHAN_ITEM, "sound/weapons/scepter/slam_warmup.wav");
	NPCS.NPC->client->ps.legsTimer = NPCS.NPC->client->ps.torsoTimer = 0;
	NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_TAVION_SCEPTERGROUND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	NPCS.NPC->painDebounceTime = level.time + NPCS.NPC->client->ps.torsoTimer;
	NPCS.NPC->client->ps.pm_time = NPCS.NPC->client->ps.torsoTimer;
	NPCS.NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	VectorClear(NPCS.NPC->client->ps.velocity);
	VectorClear(NPCS.NPC->client->ps.moveDir);
	NPCS.NPC->count = 0;
}

void Tavion_SithSwordRecharge(void)
{
	if (NPCS.NPC->client->ps.torsoAnim != BOTH_TAVION_SWORDPOWER
		&& NPCS.NPC->count
		&& TIMER_Done(NPCS.NPC, "rechargeDebounce")
		&& NPCS.NPC->client->weaponGhoul2[0])
	{
		mdxaBone_t boltMatrix;
		vec3_t dir, base, angles;

		VectorSet(angles, 0, NPCS.NPC->r.currentAngles[YAW], 0);

		NPCS.NPC->s.loopSound = G_SoundIndex("sound/weapons/scepter/recharge.wav");
		const int boltIndex = trap->G2API_AddBolt(NPCS.NPC->client->weaponGhoul2[0], 0, "*weapon");
		NPCS.NPC->client->ps.legsTimer = NPCS.NPC->client->ps.torsoTimer = 0;
		NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_TAVION_SWORDPOWER, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

		//RAFIXME:  This probably needs to be moved to client side.
		trap->G2API_GetBoltMatrix(NPCS.NPC->client->weaponGhoul2[0], 0,
			boltIndex,
			&boltMatrix, angles, NPCS.NPC->r.currentOrigin, level.time,
			NULL, NPCS.NPC->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, base);
		BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, dir);
		G_PlayEffect(G_EffectIndex("scepter/recharge.efx"), base, dir);
		NPCS.NPC->painDebounceTime = level.time + NPCS.NPC->client->ps.torsoTimer;
		NPCS.NPC->client->ps.pm_time = NPCS.NPC->client->ps.torsoTimer;
		NPCS.NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		VectorClear(NPCS.NPC->client->ps.velocity);
		VectorClear(NPCS.NPC->client->ps.moveDir);
		NPCS.NPC->client->ps.powerups[PW_INVINCIBLE] = level.time + NPCS.NPC->client->ps.torsoTimer + 10000;
		G_PlayEffect(G_EffectIndex("scepter/invincibility.efx"), NPCS.NPC->r.currentOrigin, NPCS.NPC->r.currentAngles);
		TIMER_Set(NPCS.NPC, "rechargeDebounce", NPCS.NPC->client->ps.torsoTimer + 10000 + Q_irand(10000, 20000));
		NPCS.NPC->count--;
		//now you have a chance of killing her
		NPCS.NPC->flags &= ~FL_UNDYING;
	}
}

extern void ChangeWeapon(const gentity_t* ent, int new_weapon);

void Boba_ChangeWeapon(const int wp)
{
	if (NPCS.NPC->s.weapon == wp)
	{
		return;
	}
	NPC_ChangeWeapon(wp);
	G_AddEvent(NPCS.NPC, EV_GENERAL_SOUND, G_SoundIndex("sound/weapons/change.wav"));
}

qboolean Boba_StopKnockdown(gentity_t* self, const gentity_t* pusher, const vec3_t push_dir,
	const qboolean force_knockdown)
	//forceKnockdown = qfalse
{
	vec3_t pDir, fwd, right, ang;

	if (self->client->NPC_class != CLASS_BOBAFETT
		|| self->client->pers.botclass != BCLASS_BOBAFETT
		|| self->client->pers.botclass != BCLASS_MANDOLORIAN1
		|| self->client->pers.botclass != BCLASS_MANDOLORIAN2)
	{
		return qfalse;
	}

	if (self->client->ps.eFlags2 & EF2_FLYING)
	{
		//can't knock me down when I'm flying
		return qtrue;
	}

	VectorSet(ang, 0, self->r.currentAngles[YAW], 0);
	const int strafeTime = Q_irand(1000, 2000);

	AngleVectors(ang, fwd, right, NULL);
	VectorNormalize2(push_dir, pDir);
	const float fDot = DotProduct(pDir, fwd);
	const float rDot = DotProduct(pDir, right);

	if (Q_irand(0, 2))
	{
		//flip or roll with it
		usercmd_t temp_Cmd;
		if (fDot >= 0.4f)
		{
			temp_Cmd.forwardmove = 127;
			TIMER_Set(self, "moveforward", strafeTime);
		}
		else if (fDot <= -0.4f)
		{
			temp_Cmd.forwardmove = -127;
			TIMER_Set(self, "moveback", strafeTime);
		}
		else if (rDot > 0)
		{
			temp_Cmd.rightmove = 127;
			TIMER_Set(self, "strafeRight", strafeTime);
			TIMER_Set(self, "strafeLeft", -1);
		}
		else
		{
			temp_Cmd.rightmove = -127;
			TIMER_Set(self, "strafeLeft", strafeTime);
			TIMER_Set(self, "strafeRight", -1);
		}
		G_AddEvent(self, EV_JUMP, 0);
		if (!Q_irand(0, 1))
		{
			//flip
			self->client->ps.fd.forceJumpCharge = 280; //FIXME: calc this intelligently?
			ForceJump(self, &temp_Cmd);
		}
		else
		{
			//roll
			TIMER_Set(self, "duck", strafeTime);
		}
		self->painDebounceTime = 0; //so we do something
	}
	else if (!Q_irand(0, 1) && force_knockdown)
	{
		//resist
		WP_ResistForcePush(self, pusher, qtrue);
	}
	else
	{
		//fall down
		return qfalse;
	}

	return qtrue;
}

void Boba_SetJetpackAnims(const gentity_t* self)
{
	vec3_t angs, mvnt;
	int forwardmove = 0, rightmove = 0;

	if (!self)
		return;

	if (!(self->client->ps.eFlags2 & EF2_FLYING))
		return;

	if (self->localAnimIndex != 0)
		return;

	if (self->client->ps.legsTimer > 0)
		return;

	if (VectorLengthSquared(NPCS.NPC->client->ps.velocity) < 2048)
		return;

	vectoangles(NPCS.NPC->client->ps.velocity, angs);
	angs[YAW] -= NPCS.NPC->client->ps.viewangles[YAW];
	AngleVectors(angs, mvnt, NULL, NULL);

	if (mvnt[0] > 0)
	{
		rightmove = 1;
	}
	else if (mvnt[0] < 0)
	{
		rightmove = -1;
	}

	if (mvnt[1] > 0)
	{
		forwardmove = 1;
	}
	else if (mvnt[1] < 0)
	{
		forwardmove = -1;
	}
}

qboolean Jedi_StopKnockdown(gentity_t* self, const vec3_t push_dir)
{
	//certain NPCs can avoid knockdowns, check for that.
	vec3_t pDir, fwd, right, ang;
	usercmd_t temp_Cmd;
	const int strafeTime = Q_irand(1000, 2000);

	if (self->s.number < MAX_CLIENTS || !self->NPC)
	{
		//only NPCs
		return qfalse;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_1)
	{
		//only force-users
		return qfalse;
	}

	if (self->client->ps.eFlags2 & EF2_FLYING)
	{
		//can't knock me down when I'm flying
		return qtrue;
	}

	if (self->NPC && self->NPC->aiFlags & NPCAI_BOSS_CHARACTER)
	{
		//bosses always get out of a knockdown
	}
	else if (Q_irand(0, RANK_CAPTAIN + 5) > self->NPC->rank)
	{
		//lower their rank, the more likely they are fall down
		return qfalse;
	}

	VectorSet(ang, 0, self->r.currentAngles[YAW], 0);

	AngleVectors(ang, fwd, right, NULL);
	VectorNormalize2(push_dir, pDir);
	const float fDot = DotProduct(pDir, fwd);
	const float rDot = DotProduct(pDir, right);

	//flip or roll with it
	if (fDot >= 0.4f)
	{
		temp_Cmd.forwardmove = 127;
		TIMER_Set(self, "moveforward", strafeTime);
	}
	else if (fDot <= -0.4f)
	{
		temp_Cmd.forwardmove = -127;
		TIMER_Set(self, "moveback", strafeTime);
	}
	else if (rDot > 0)
	{
		temp_Cmd.rightmove = 127;
		TIMER_Set(self, "strafeRight", strafeTime);
		TIMER_Set(self, "strafeLeft", -1);
	}
	else
	{
		temp_Cmd.rightmove = -127;
		TIMER_Set(self, "strafeLeft", strafeTime);
		TIMER_Set(self, "strafeRight", -1);
	}
	G_AddEvent(self, EV_JUMP, 0);
	if (!Q_irand(0, 1))
	{
		//flip
		self->client->ps.fd.forceJumpCharge = 280; //FIXME: calc this intelligently?
		ForceJump(self, &temp_Cmd);
	}
	else
	{
		//roll
		TIMER_Set(self, "duck", strafeTime);
	}
	self->painDebounceTime = 0; //so we do something

	return qtrue;
}

void Boba_FlyStart(gentity_t* self)
{
	//switch to seeker AI for a while
	if (TIMER_Done(self, "jetRecharge"))
	{
		self->client->ps.gravity = 0;
		if (self->NPC)
		{
			self->NPC->aiFlags |= NPCAI_CUSTOM_GRAVITY;
		}
		self->client->ps.eFlags2 |= EF2_FLYING; //moveType = MT_FLYSWIM;
		self->client->jetPackTime = level.time + Q_irand(3000, 10000);
		//take-off sound
		G_SoundOnEnt(self, CHAN_ITEM, "sound/chars/boba/bf_blast-off.mp3");
		//jet loop sound
		self->s.loopSound = G_SoundIndex("sound/chars/boba/jethover.wav");
		if (self->NPC)
		{
			self->count = Q3_INFINITE; // SEEKER shot ammo count
		}
	}
}

void Boba_ForceFlyStart(gentity_t* self, const int jetTime)
{
	//switch to seeker AI for a while
	self->client->ps.gravity = 0;
	if (self->NPC)
	{
		self->NPC->aiFlags |= NPCAI_CUSTOM_GRAVITY;
		self->NPC->scriptFlags &= ~SCF_CROUCHED;
		self->NPC->scriptFlags &= ~SCF_RUNNING;
		self->NPC->scriptFlags &= ~SCF_WALKING;
		self->NPC->scriptFlags &= ~SCF_FORCED_MARCH;
	}
	self->client->ps.eFlags2 |= EF2_FLYING;
	self->client->jetPackTime = level.time + jetTime;
	//take-off sound
	G_Sound(self, CHAN_ITEM, G_SoundIndex("sound/chars/boba/bf_blast-off.mp3"));
	if (self->NPC)
	{
		self->count = Q3_INFINITE; // SEEKER shot ammo count
	}
}

void Boba_FlyStop(gentity_t* self)
{
	self->client->ps.gravity = g_gravity.value;
	if (self->NPC)
	{
		self->NPC->aiFlags &= ~NPCAI_CUSTOM_GRAVITY;
	}
	self->client->ps.eFlags2 &= ~EF2_FLYING;
	self->client->jetPackTime = 0;
	//stop jet loop sound
	self->s.loopSound = 0;
	if (self->NPC)
	{
		self->count = 0; // SEEKER shot ammo count
		TIMER_Set(self, "jetRecharge", Q_irand(1000, 5000));
		TIMER_Set(self, "jumpChaseDebounce", Q_irand(500, 2000));
	}
}

qboolean Boba_Flying(const gentity_t* self)
{
	return self->client->ps.eFlags2 & EF2_FLYING;
}

void Boba_FlyThink(gentity_t* self)
{
	if (Boba_Flying(self))
		if (self->client->jetPackTime < level.time)
		{
			Boba_FlyStop(self);
		}
}

void Boba_SetBolts(const gentity_t* self)
{
	if (self && self->client)
	{
		self->client->renderInfo.handLBolt = trap->G2API_AddBolt(self->ghoul2, 0, "*l_hand");
		self->client->renderInfo.handRBolt = trap->G2API_AddBolt(self->ghoul2, 0, "*r_hand");
	}
}

float G_GroundDistance(const gentity_t* self);

void Boba_JetpackDecideDefault(void)
{
	const float gDist = G_GroundDistance(NPCS.NPC);

	if (!NPCS.NPC->client->playerLeader)
	{
		if (!NPCS.NPC->enemy)
		{
			Boba_FlyThink(NPCS.NPC);
		}
		return;
	}

	if (NPCS.NPC->client->playerLeader->client->jetPackOn && NPCS.NPCInfo->defaultBehavior == BS_FOLLOW_LEADER)
	{
		if (TIMER_Done(NPCS.NPC, "jetpackGoGo"))
		{
			const int r = Q_irand(4000, 6500);

			if (NPCS.NPC->NPC->shouldJetOn)
			{
				NPCS.ucmd.rightmove = NPCS.ucmd.forwardmove = 0;
				NPCS.ucmd.upmove = 127;
				Boba_ForceFlyStart(NPCS.NPC, r);
				NPCS.NPC->NPC->shouldJetOn = qfalse;
			}
			else
			{
				NPCS.NPC->client->jetPackTime = level.time + r;
			}

			TIMER_Set(NPCS.NPC, "jetpackGoGo", r - FRAMETIME * 2);
		}
	}
	else
	{
		if (NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE
			&& NPCS.NPC->client->ps.fd.forceJumpZStart
			&& !PM_FlippingAnim(NPCS.NPC->client->ps.legsAnim)
			&& !Q_irand(0, 3)
			&& !Boba_Flying(NPCS.NPC))
		{
			//take off
			Boba_FlyStart(NPCS.NPC);
		}
		else if (gDist > 512 && !Boba_Flying(NPCS.NPC))
		{
			Boba_ForceFlyStart(NPCS.NPC, Q_irand(2500, 5000));
		}
		Boba_FlyThink(NPCS.NPC);
	}
}

void Boba_JetpackDecideCombat(void)
{
	const float gDist = G_GroundDistance(NPCS.NPC);

	if (NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE
		&& NPCS.NPC->client->ps.fd.forceJumpZStart
		&& !PM_FlippingAnim(NPCS.NPC->client->ps.legsAnim)
		&& !Boba_Flying(NPCS.NPC))
	{
		//take off
		Boba_FlyStart(NPCS.NPC);
	}
	else if (gDist > 512 && !Boba_Flying(NPCS.NPC))
	{
		Boba_ForceFlyStart(NPCS.NPC, Q_irand(2000, 10000) * NPCS.NPCInfo->stats.move);
	}

	Boba_FlyThink(NPCS.NPC);
}

void Boba_ChooseWeapon(void)
{
	if (TIMER_Done(NPCS.NPC, "nextWeaponDelay"))
	{
		if (Q_irand(0, 1))
		{
			NPCS.NPCInfo->scriptFlags &= ~SCF_altFire;
		}
		else
		{
			NPCS.NPCInfo->scriptFlags |= SCF_altFire;
		}

		while (1)
		{
			const int nw = Q_irand(0, WP_NUM_WEAPONS - 1);

			if (HaveWeapon(nw))
			{
				const int hbw = NPC_HaveBetterWeapon(nw);
				if (hbw == 2 || hbw == 1 && Q_irand(0, 8))
					continue;

				Boba_ChangeWeapon(nw);
				break;
			}
		}

		TIMER_Set(NPCS.NPC, "nextWeaponDelay", Q_irand(5000, 12000));
	}
}

void Boba_FireFlameThrower(gentity_t* self)
{
	const int damage = Q_irand(8, 12);
	trace_t tr;
	mdxaBone_t boltMatrix;
	vec3_t start, end, dir;
	const vec3_t trace_maxs = { 4, 4, 4 };
	const vec3_t trace_mins = { -4, -4, -4 };

	trap->G2API_GetBoltMatrix(self->ghoul2, 0, self->client->renderInfo.handLBolt,
		&boltMatrix, self->r.currentAngles, self->r.currentOrigin, level.time,
		NULL, self->modelScale);

	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, start);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, dir);
	VectorMA(start, 128, dir, end);

	trap->Trace(&tr, start, trace_mins, trace_maxs, end, self->s.number, MASK_SHOT, qfalse, 0, 0);

	gentity_t* traceEnt = &g_entities[tr.entityNum];

	if (tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage)
	{
		G_Damage(traceEnt, self, self, dir, tr.endpos, damage,
			DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK | DAMAGE_IGNORE_TEAM, MOD_BURNING);
		g_throw(traceEnt, dir, 30);

		if (traceEnt->health > 0 && traceEnt->painDebounceTime > level.time)
		{
			g_throw(traceEnt, dir, 30);

			if (damage && traceEnt->client)
			{
				G_PlayBoltedEffect(G_EffectIndex("flamethrower/flame_impact"), traceEnt, "thoracic");

				if (!PM_InRoll(&traceEnt->client->ps)
					&& !PM_InKnockDown(&traceEnt->client->ps))
				{
					G_SetAnim(traceEnt, &self->client->pers.cmd, SETANIM_TORSO, BOTH_FACEPROTECT,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
				}
			}
			player_Burn(traceEnt);
		}
		else
		{
			Player_CheckBurn(traceEnt);
		}
	}
}

void Boba_StartFlameThrower(gentity_t* self)
{
	const int flameTime = 4000; //Q_irand( 1000, 3000 );
	mdxaBone_t boltMatrix;
	vec3_t org, dir;

	self->client->ps.torsoTimer = flameTime; //+1000;
	if (self->NPC)
	{
		TIMER_Set(self, "nextAttackDelay", flameTime);
		TIMER_Set(self, "walking", 0);
	}
	TIMER_Set(self, "flameTime", flameTime);
	G_SoundOnEnt(self, CHAN_WEAPON, "sound/effects/combustfire.mp3");

	trap->G2API_GetBoltMatrix(NPCS.NPC->ghoul2, 0, NPCS.NPC->client->renderInfo.handLBolt, &boltMatrix,
		NPCS.NPC->r.currentAngles,
		NPCS.NPC->r.currentOrigin, level.time, NULL, NPCS.NPC->modelScale);

	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, org);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, dir);

	G_PlayEffectID(G_EffectIndex("flamethrower/flamethrower_mp"), org, dir);
}

void Boba_DoFlameThrower(gentity_t* self)
{
	if (self->client->ps.jetpackFuel < 10)
	{
		return;
	}
	NPC_SetAnim(self, SETANIM_TORSO, BOTH_FLAMETHROWER, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	if (TIMER_Done(self, "nextAttackDelay") && TIMER_Done(self, "flameTime"))
	{
		Boba_StartFlameThrower(self);
	}
	Boba_FireFlameThrower(self);
}

void Boba_DoSniper(const gentity_t* self)
{
	if (TIMER_Done(self, "PickNewSniperPoint"))
	{
		TIMER_Set(self, "PickNewSniperPoint", Q_irand(15000, 25000));
		const int SniperPoint = NPC_FindCombatPoint(NPCS.NPC->r.currentOrigin, 0, NPCS.NPC->r.currentOrigin,
			CP_SNIPE | CP_CLEAR | CP_HAS_ROUTE | CP_TRYFAR | CP_HORZ_DIST_COLL,
			0, -1);
		if (SniperPoint != -1)
		{
			NPC_SetCombatPoint(SniperPoint);
			NPC_SetMoveGoal(NPCS.NPC, level.combatPoints[SniperPoint].origin, 16, qtrue, SniperPoint, NULL);
		}
	}

	if (Distance(NPCS.NPC->r.currentOrigin, level.combatPoints[NPCS.NPCInfo->combatPoint].origin) < 50.0f)
	{
		Boba_FireDecide();
	}

	NPC_FaceEnemy(qtrue);
	NPC_UpdateAngles(qtrue, qtrue);
}

#define	BOBA_FLAMEDURATION			3000

typedef struct wristWeapon_s
{
	int theMissile;
	int dummyForcePower;
	int whichWeapon;
	qboolean alt_fire;
	int maxShots;
	int animTimer;
	int animDelay;
	int fireAnim;
	qboolean fullyCharged;
	qboolean leftBolt;
	qboolean hold;
} wristWeapon_t;

wristWeapon_t missileStates[4] = {
	{
		BOBA_MISSILE_ROCKET, FP_FIRST, WP_ROCKET_LAUNCHER, qfalse, 1, BOBA_FLAMEDURATION, 150, BOTH_FLAMETHROWER,
		qfalse, qtrue, qtrue
	},
	{
		BOBA_MISSILE_LASER, FP_GRIP, WP_EMPLACED_GUN, qfalse, 5, BOBA_FLAMEDURATION, 150, BOTH_FLAMETHROWER, qfalse,
		qtrue, qtrue
	},
	{BOBA_MISSILE_DART, FP_FIRST, WP_DISRUPTOR, qfalse, 1, 1500, 200, BOTH_PULL_IMPALE_STAB, qfalse, qfalse, qtrue},
	{BOBA_MISSILE_VIBROBLADE, FP_DRAIN, WP_MELEE, qfalse, 1, 1000, 100, BOTH_PULL_IMPALE_STAB, qfalse, qfalse, qfalse}
};

extern void WP_FireWristMissile(gentity_t* ent, vec3_t start, vec3_t dir);

void Boba_FireWristMissile(gentity_t* self, const int whichMissile)
{
	static int shotsFired = 0; //only 5 shots allowed for wristlaser; only 1 for missile launcher or dart

	if (self->s.number >= MAX_CLIENTS)
	{
		return;
	}

	if (!self->client)
	{
		return;
	}

	if (self->health <= 0)
	{
		return;
	}

	if (in_camera)
	{
		return;
	}

	const int dummyForcePower = missileStates[whichMissile].dummyForcePower;

	if (!self->client->ps.fd.forcePowerDuration[dummyForcePower])
	{
		NPC_SetAnim(self, SETANIM_TORSO, missileStates[whichMissile].fireAnim,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		self->client->ps.torsoTimer = missileStates[whichMissile].animTimer;
		self->client->ps.fd.forcePowerDuration[dummyForcePower] = 1;
		shotsFired = 0;
	}

	if (self->client->ps.torsoTimer > missileStates[whichMissile].animTimer - missileStates[whichMissile].animDelay)
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

	if (missileStates[whichMissile].hold)
	{
		if (self->client->ps.torsoTimer < 150)
		{
			self->client->ps.fd.forcePowerDuration[dummyForcePower] = 150;
			return;
		}
	}
	else
	{
		if (self->client->ps.torsoTimer < 50)
		{
			self->client->ps.fd.forcePowerDuration[dummyForcePower] = 0;
			self->client->ps.torsoTimer = 0;
			return;
		}
	}

	const int oldWeapon = self->s.weapon;

	self->s.weapon = missileStates[whichMissile].whichWeapon;
	qboolean alt_fire = missileStates[whichMissile].alt_fire;
	if (missileStates[whichMissile].fullyCharged)
	{
		self->client->ps.weaponChargeTime = 0;
	}

	const int addTime = weaponData[self->s.weapon].fireTime;
	self->client->ps.weaponTime += addTime;
	self->client->ps.lastShotTime = level.time;

	mdxaBone_t boltMatrix;
	vec3_t muzzlePoint;
	vec3_t muzzleDir;

	//trap->G2API_GetBoltMatrix(self->ghoul2, 0, self->client->renderInfo.handLBolt, &boltMatrix, self->r.currentAngles, self->r.currentOrigin, level.time, NULL, self->modelScale);
	trap->G2API_GetBoltMatrix(self->ghoul2, 0,
		missileStates[whichMissile].leftBolt
		? self->genericBolt3
		: self->client->renderInfo.handLBolt, &boltMatrix, self->r.currentAngles,
		self->r.currentOrigin, level.time, NULL, self->modelScale);

	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, muzzlePoint);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, muzzleDir);
	// work the matrix axis stuff into the original axis and origins used.

	VectorCopy(muzzlePoint, self->client->renderInfo.muzzlePoint);
	VectorCopy(muzzleDir, self->client->renderInfo.muzzleDir);

	if (shotsFired >= missileStates[whichMissile].maxShots)
	{
		vec3_t ORIGIN = { 0, 0, 0 };
		G_PlayEffectID(G_EffectIndex("repeater/muzzle_smoke"), muzzlePoint, ORIGIN);
		self->client->ps.userInt3 &= ~(1 << FLAG_WRISTBLASTER);
		//play smoke
		return;
	}
	vec3_t ORIGIN = { 0, 0, 0 };
	G_PlayEffectID(G_EffectIndex("blaster/muzzle_flash"), muzzlePoint, ORIGIN);
	self->client->ps.userInt3 |= 1 << FLAG_WRISTBLASTER;

	WP_FireWristMissile(self, muzzlePoint, muzzleDir);
	shotsFired++;

	self->s.weapon = oldWeapon;
}

void Boba_EndWristMissile(const gentity_t* self, const int which_missile)
{
	if (missileStates[which_missile].hold)
	{
		self->client->ps.fd.forcePowerDuration[missileStates[which_missile].dummyForcePower] = 0;
		self->client->ps.torsoTimer = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////////////
#define		BOBA_FLAMEDURATION			3000
#define		BOBA_FLAMETHROWRANGE		128
#define		BOBA_FLAMETHROWSIZE			40
#define		BOBA_FLAMETHROWDAMAGEMIN	1//10
#define		BOBA_FLAMETHROWDAMAGEMAX	5//40
#define		BOBA_ROCKETRANGEMIN			300
#define		BOBA_ROCKETRANGEMAX			2000

void Boba_TacticsSelect()
{
	// Don't Change Tactics For A Little While
	//------------------------------------------
	TIMER_Set(NPCS.NPC, "Boba_TacticsSelect", Q_irand(8000, 15000));
	int nextState = NPCS.NPCInfo->localState;

	// Get Some Data That Will Help With The Selection Of The Next Tactic
	//--------------------------------------------------------------------
	const float enemyDistance = Distance(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin);

	// Enemy Is Really Close
	//-----------------------
	if (!NPCS.NPC->enemy->health > 0)
	{
		nextState = BTS_RIFLE;
	}
	else if (enemyDistance < BOBA_FLAMETHROWRANGE)
	{
		// If It's Been Long Enough Since Our Last Flame Blast, Try To Torch The Enemy
		//-----------------------------------------------------------------------------
		if (TIMER_Done(NPCS.NPC, "nextFlameDelay"))
		{
			nextState = BTS_FLAMETHROW;
		}

		// Otherwise, He's Probably Too Close, So Try To Get Clear Of Him
		//----------------------------------------------------------------
		else
		{
			nextState = BTS_RIFLE;
		}
	}

	// Recently Saw The Enemy, Time For Some Good Ole Fighten!
	//---------------------------------------------------------
	else if (level.time - NPCS.NPCInfo->enemyLastSeenTime < 10000)
	{
		// At First, Boba will prefer to use his blaster against the player, but
		//  the more times he is driven away (NPC->count), he will be less likely to
		//  choose the blaster, and more likely to go for the missile launcher
		nextState = !enemyDistance > BOBA_ROCKETRANGEMIN && enemyDistance < BOBA_ROCKETRANGEMAX ||
			Q_irand(0, NPCS.NPC->count) < 1
			? BTS_RIFLE
			: BTS_MISSILE;
	}

	// Hmmm...  Havn't Seen The Player In A While, We Might Want To Try Something Sneaky
	//-----------------------------------------------------------------------------------
	else
	{
		int SnipePointsNear = qfalse; // TODO
		const int AmbushPointNear = qfalse; // TODO

		if (Q_irand(0, NPCS.NPC->count) > 0)
		{
			const int SniperPoint = NPC_FindCombatPoint(NPCS.NPC->r.currentOrigin, 0, NPCS.NPC->r.currentOrigin,
				CP_SNIPE | CP_CLEAR | CP_HAS_ROUTE | CP_TRYFAR |
				CP_HORZ_DIST_COLL, 0, -1);
			if (SniperPoint != -1)
			{
				NPC_SetCombatPoint(SniperPoint);
				NPC_SetMoveGoal(NPCS.NPC, level.combatPoints[SniperPoint].origin, 8, qtrue, SniperPoint, NULL);
				TIMER_Set(NPCS.NPC, "PickNewSniperPoint", Q_irand(15000, 25000));
				SnipePointsNear = qtrue;
			}
		}

		if (SnipePointsNear && TIMER_Done(NPCS.NPC, "Boba_NoSniperTime"))
		{
			TIMER_Set(NPCS.NPC, "Boba_NoSniperTime", 120000); // Don't snipe again for a while
			TIMER_Set(NPCS.NPC, "Boba_TacticsSelect", Q_irand(35000, 45000)); // More patience here
			nextState = BTS_SNIPER;
		}
		else
		{
			nextState = !enemyDistance > BOBA_ROCKETRANGEMIN && enemyDistance < BOBA_ROCKETRANGEMAX ||
				Q_irand(0, NPCS.NPC->count) < 1
				? BTS_RIFLE
				: BTS_MISSILE;
		}
	}

	// The Next State Has Been Selected, Now Change Weapon If Necessary
	//------------------------------------------------------------------
	if (nextState != NPCS.NPCInfo->localState)
	{
		NPCS.NPCInfo->localState = nextState;
		switch (NPCS.NPCInfo->localState)
		{
		case BTS_FLAMETHROW:
			Boba_ChangeWeapon(WP_NONE);
			Boba_DoFlameThrower(NPCS.NPC);
			break;

		case BTS_RIFLE:
			Boba_ChangeWeapon(WP_BLASTER);
			break;

		case BTS_MISSILE:
			Boba_ChangeWeapon(WP_ROCKET_LAUNCHER);
			break;

		case BTS_SNIPER:
			Boba_ChangeWeapon(WP_DISRUPTOR);
			break;

		case BTS_AMBUSHWAIT:
			Boba_ChangeWeapon(WP_NONE);
			break;
		default:;
		}
	}
}

void Boba_DoAmbushWait(gentity_t* self)
{
}

void Boba_Tactics()
{
	if (!NPCS.NPC->enemy)
	{
		return;
	}

	// Think About Changing Tactics
	//------------------------------
	if (TIMER_Done(NPCS.NPC, "Boba_TacticsSelect"))
	{
		Boba_TacticsSelect();
	}

	// These Tactics Require Seeker & Jedi Movement
	//----------------------------------------------
	if (!NPCS.NPCInfo->localState ||
		NPCS.NPCInfo->localState == BTS_RIFLE ||
		NPCS.NPCInfo->localState == BTS_MISSILE)
	{
		return;
	}

	// Flame Thrower - Locked In Place
	//---------------------------------
	if (NPCS.NPCInfo->localState == BTS_FLAMETHROW)
	{
		Boba_DoFlameThrower(NPCS.NPC);
	}

	// Sniper - Move Around, And Take Shots
	//--------------------------------------
	else if (NPCS.NPCInfo->localState == BTS_SNIPER)
	{
		Boba_DoSniper(NPCS.NPC);
	}

	// Ambush Wait
	//------------
	else if (NPCS.NPCInfo->localState == BTS_AMBUSHWAIT)
	{
		Boba_DoAmbushWait(NPCS.NPC);
	}

	NPC_FacePosition(NPCS.NPC->enemy->r.currentOrigin, qtrue);
	NPC_UpdateAngles(qtrue, qtrue);
}

float G_GroundDistance(const gentity_t* self)
{
	if (!self)
	{
		//wtf?!!
		return Q3_INFINITE;
	}
	trace_t tr;
	vec3_t down;

	VectorCopy(self->r.currentOrigin, down);

	down[2] -= 4096;

	trap->Trace(&tr, self->client->ps.origin, self->r.mins, self->r.maxs, down, self->s.clientNum, MASK_SOLID, qfalse,
		0, 0);

	VectorSubtract(self->r.currentOrigin, tr.endpos, down);

	return VectorLength(down);
}

void Boba_FireDecide(void)
{
	qboolean enemyLOS = qfalse, enemyCS = qfalse, enemyInFOV = qfalse;
	qboolean shoot = qfalse, hitAlly = qfalse;
	vec3_t impactPos, enemyDir, shootDir;

	if (NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE
		&& NPCS.NPC->client->ps.fd.forceJumpZStart
		&& !PM_FlippingAnim(NPCS.NPC->client->ps.legsAnim)
		&& !Q_irand(0, 10))
	{
		//take off
		Boba_FlyStart(NPCS.NPC);
	}

	if (!NPCS.NPC->enemy)
	{
		return;
	}

	Boba_JetpackDecideCombat();

	if (NPCS.NPC->enemy->s.weapon == WP_SABER)
	{
		NPCS.NPCInfo->scriptFlags &= ~SCF_altFire;
		Boba_ChangeWeapon(WP_ROCKET_LAUNCHER);
	}
	else
	{
		if (NPCS.NPC->health < NPCS.NPC->client->pers.maxHealth * 0.5f)
		{
			NPCS.NPCInfo->scriptFlags |= SCF_altFire;
			Boba_ChangeWeapon(WP_BLASTER);
			NPCS.NPCInfo->burstMin = 3;
			NPCS.NPCInfo->burstMean = 12;
			NPCS.NPCInfo->burstMax = 20;
			NPCS.NPCInfo->burstSpacing = Q_irand(300, 750); //attack debounce
		}
		else
		{
			NPCS.NPCInfo->scriptFlags &= ~SCF_altFire;
			Boba_ChangeWeapon(WP_BLASTER);
		}
	}

	VectorClear(impactPos);
	const float enemyDist = DistanceSquared(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin);

	VectorSubtract(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, enemyDir);
	VectorNormalize(enemyDir);
	AngleVectors(NPCS.NPC->client->ps.viewangles, shootDir, NULL, NULL);
	const float dot = DotProduct(enemyDir, shootDir);
	if (dot > 0.5f || enemyDist * (1.0f - dot) < 10000)
	{
		//enemy is in front of me or they're very close and not behind me
		enemyInFOV = qtrue;
	}

	if (enemyDist < 128 * 128 && enemyInFOV || !TIMER_Done(NPCS.NPC, "flameTime"))
	{
		//flamethrower
		Boba_DoFlameThrower(NPCS.NPC);
		enemyCS = qfalse;
		shoot = qfalse;
		NPCS.NPCInfo->enemyLastSeenTime = level.time;
		NPCS.ucmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK);
	}
	else if (enemyDist < MIN_ROCKET_DIST_SQUARED) //128
	{
		//enemy within 128
		if ((NPCS.NPC->client->ps.weapon == WP_FLECHETTE || NPCS.NPC->client->ps.weapon == WP_REPEATER) &&
			NPCS.NPCInfo->scriptFlags & SCF_altFire)
		{
			//shooting an explosive, but enemy too close, switch to primary fire
			NPCS.NPCInfo->scriptFlags &= ~SCF_altFire;
			//FIXME: we can never go back to alt-fire this way since, after this, we don't know if we were initially supposed to use alt-fire or not...
		}
	}
	else if (enemyDist > 65536) //256 squared
	{
		if (NPCS.NPC->client->ps.weapon == WP_DISRUPTOR)
		{
			//sniping... should be assumed
			if (!(NPCS.NPCInfo->scriptFlags & SCF_altFire))
			{
				//use primary fire
				NPCS.NPCInfo->scriptFlags |= SCF_altFire;
				//reset fire-timing variables
				NPC_ChangeWeapon(WP_DISRUPTOR);
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}

	//can we see our target?
	if (TIMER_Done(NPCS.NPC, "nextAttackDelay") && TIMER_Done(NPCS.NPC, "flameTime"))
	{
		if (NPC_ClearLOS4(NPCS.NPC->enemy))
		{
			NPCS.NPCInfo->enemyLastSeenTime = level.time;
			enemyLOS = qtrue;

			if (NPCS.NPC->client->ps.weapon == WP_NONE)
			{
				enemyCS = qfalse; //not true, but should stop us from firing
			}
			else
			{
				//can we shoot our target?
				if ((NPCS.NPC->client->ps.weapon == WP_ROCKET_LAUNCHER || NPCS.NPC->client->ps.weapon == WP_FLECHETTE
					&& NPCS.NPCInfo->scriptFlags & SCF_altFire) && enemyDist < MIN_ROCKET_DIST_SQUARED) //128*128
				{
					enemyCS = qfalse; //not true, but should stop us from firing
					hitAlly = qtrue; //us!
					//FIXME: if too close, run away!
				}
				else if (enemyInFOV)
				{
					//if enemy is FOV, go ahead and check for shooting
					const int hit = NPC_ShotEntity(NPCS.NPC->enemy, impactPos);
					const gentity_t* hit_ent = &g_entities[hit];

					if (hit == NPCS.NPC->enemy->s.number
						|| hit_ent && hit_ent->client && hit_ent->client->playerTeam == NPCS.NPC->client->enemyTeam
						|| hit_ent && hit_ent->takedamage && (hit_ent->r.svFlags & SVF_GLASS_BRUSH || hit_ent->health <
							40 || NPCS.NPC->s.weapon == WP_EMPLACED_GUN))
					{
						//can hit enemy or enemy ally or will hit glass or other minor breakable (or in emplaced gun), so shoot anyway
						enemyCS = qtrue;
						//NPC_AimAdjust( 2 );//adjust aim better longer we have clear shot at enemy
						VectorCopy(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPCInfo->enemyLastSeenLocation);
					}
					else
					{
						//Hmm, have to get around this bastard
						//NPC_AimAdjust( 1 );//adjust aim better longer we can see enemy
						if (hit_ent && hit_ent->client && hit_ent->client->playerTeam == NPCS.NPC->client->playerTeam)
						{
							//would hit an ally, don't fire!!!
							hitAlly = qtrue;
						}
						else
						{
							//Check and see where our shot *would* hit... if it's not close to the enemy (within 256?), then don't fire
						}
					}
				}
				else
				{
					enemyCS = qfalse; //not true, but should stop us from firing
				}
			}
		}
		else if (trap->InPVS(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin))
		{
			NPCS.NPCInfo->enemyLastSeenTime = level.time;
		}

		if (NPCS.NPC->client->ps.weapon == WP_NONE)
		{
			shoot = qfalse;
		}
		else
		{
			if (enemyCS)
			{
				shoot = qtrue;
			}
		}

		if (!enemyCS)
		{
			//if have a clear shot, always try
			//See if we should continue to fire on their last position
			if (!hitAlly //we're not going to hit an ally
				&& enemyInFOV //enemy is in our FOV //FIXME: or we don't have a clear LOS?
				&& NPCS.NPCInfo->enemyLastSeenTime > 0) //we've seen the enemy
			{
				if (level.time - NPCS.NPCInfo->enemyLastSeenTime < 10000)
					//we have seem the enemy in the last 10 seconds
				{
					if (!Q_irand(0, 10))
					{
						//Fire on the last known position
						vec3_t muzzle;
						qboolean tooClose = qfalse;
						qboolean tooFar = qfalse;

						CalcEntitySpot(NPCS.NPC, SPOT_HEAD, muzzle);
						if (VectorCompare(impactPos, vec3_origin))
						{
							//never checked ShotEntity this frame, so must do a trace...
							trace_t tr;
							//vec3_t	mins = {-2,-2,-2}, maxs = {2,2,2};
							vec3_t forward, end;
							AngleVectors(NPCS.NPC->client->ps.viewangles, forward, NULL, NULL);
							VectorMA(muzzle, 8192, forward, end);
							trap->Trace(&tr, muzzle, vec3_origin, vec3_origin, end, NPCS.NPC->s.number, MASK_SHOT,
								qfalse, 0, 0);
							VectorCopy(tr.endpos, impactPos);
						}

						//see if impact would be too close to me
						float distThreshold = 16384/*128*128*/; //default
						switch (NPCS.NPC->s.weapon)
						{
						case WP_ROCKET_LAUNCHER:
						case WP_FLECHETTE:
						case WP_THERMAL:
						case WP_TRIP_MINE:
						case WP_DET_PACK:
							distThreshold = 65536/*256*256*/;
							break;
						case WP_REPEATER:
							if (NPCS.NPCInfo->scriptFlags & SCF_altFire)
							{
								distThreshold = 65536/*256*256*/;
							}
							break;
						default:
							break;
						}

						float dist = DistanceSquared(impactPos, muzzle);

						if (dist < distThreshold)
						{
							//impact would be too close to me
							tooClose = qtrue;
						}
						else if (level.time - NPCS.NPCInfo->enemyLastSeenTime > 5000 ||
							NPCS.NPCInfo->group && level.time - NPCS.NPCInfo->group->lastSeenEnemyTime > 5000)
						{
							//we've haven't seen them in the last 5 seconds
							//see if it's too far from where he is
							distThreshold = 65536/*256*256*/; //default
							switch (NPCS.NPC->s.weapon)
							{
							case WP_ROCKET_LAUNCHER:
							case WP_FLECHETTE:
							case WP_THERMAL:
							case WP_TRIP_MINE:
							case WP_DET_PACK:
								distThreshold = 262144/*512*512*/;
								break;
							case WP_REPEATER:
								if (NPCS.NPCInfo->scriptFlags & SCF_altFire)
								{
									distThreshold = 262144/*512*512*/;
								}
								break;
							default:
								break;
							}
							dist = DistanceSquared(impactPos, NPCS.NPCInfo->enemyLastSeenLocation);
							if (dist > distThreshold)
							{
								//impact would be too far from enemy
								tooFar = qtrue;
							}
						}

						if (!tooClose && !tooFar)
						{
							vec3_t angles;
							vec3_t dir;
							//okay too shoot at last pos
							VectorSubtract(NPCS.NPCInfo->enemyLastSeenLocation, muzzle, dir);
							VectorNormalize(dir);
							vectoangles(dir, angles);

							NPCS.NPCInfo->desiredYaw = angles[YAW];
							NPCS.NPCInfo->desiredPitch = angles[PITCH];

							shoot = qtrue;
						}
					}
				}
			}
		}

		//FIXME: don't shoot right away!
		if (NPCS.NPC->client->ps.weaponTime > 0)
		{
			if (NPCS.NPC->s.weapon == WP_ROCKET_LAUNCHER)
			{
				if (!enemyLOS || !enemyCS)
				{
					//cancel it
					NPCS.NPC->client->ps.weaponTime = 0;
				}
				else
				{
					//delay our next attempt
					TIMER_Set(NPCS.NPC, "nextAttackDelay", Q_irand(500, 1000));
				}
			}
		}
		else if (shoot)
		{
			//try to shoot if it's time
			if (TIMER_Done(NPCS.NPC, "nextAttackDelay"))
			{
				if (!(NPCS.NPCInfo->scriptFlags & SCF_FIRE_WEAPON)) // we've already fired, no need to do it again here
				{
					WeaponThink();
				}
				//NASTY
				if (NPCS.NPC->s.weapon == WP_ROCKET_LAUNCHER
					&& NPCS.ucmd.buttons & BUTTON_ATTACK
					&& !Q_irand(0, 3))
				{
					//every now and then, shoot a homing rocket
					NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
					NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
					NPCS.NPC->client->ps.weaponTime = Q_irand(500, 1500);
				}
			}
		}
	}
}

void player_Cloak(gentity_t* self)
{
	if (self)
	{
		self->flags |= FL_NOTARGET;
		if (self->client)
		{
			if (!self->client->ps.powerups[PW_CLOAKED])
			{
				//cloak
				self->client->ps.powerups[PW_CLOAKED] = Q3_INFINITE;
				G_Sound(self, CHAN_ITEM, G_SoundIndex("sound/chars/shadowtrooper/cloak.wav"));
				self->client->ps.cloakFuel -= 15;
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_PROTECT_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
		}
	}
}

void Jedi_Cloak(gentity_t* self)
{
	if (self)
	{
		self->flags |= FL_NOTARGET;
		if (self->client)
		{
			if (!self->client->ps.powerups[PW_CLOAKED])
			{
				//cloak
				self->client->ps.powerups[PW_CLOAKED] = Q3_INFINITE;
				G_Sound(self, CHAN_ITEM, G_SoundIndex("sound/chars/shadowtrooper/cloak.wav"));
				self->client->ps.cloakFuel -= 15;

				if (self->client->ps.saberHolstered > 1)
				{
					NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_PROTECT_FAST,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}
		}
	}
}

void Jedi_Decloak(gentity_t* self)
{
	if (self)
	{
		self->flags &= ~FL_NOTARGET;
		if (self->client)
		{
			if (self->client->ps.powerups[PW_CLOAKED])
			{
				//Uncloak
				self->client->ps.powerups[PW_CLOAKED] = 0;

				G_Sound(self, CHAN_ITEM, G_SoundIndex("sound/chars/shadowtrooper/decloak.wav"));

				if (!(self->r.svFlags & SVF_BOT))
				{
					NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_DRAIN_RELEASE,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}
		}
	}
}

void Sphereshield_On(gentity_t* self)
{//cloak this entity
	if (self && self->client)
	{
		if (!self->client->ps.powerups[PW_SPHERESHIELDED])
		{//cloak
			self->client->ps.powerups[PW_SPHERESHIELDED] = Q3_INFINITE;
			G_SoundOnEnt(self, CHAN_ITEM, "sound/barrier/barrier_on.mp3");
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_PROTECT_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
}

void Sphereshield_Off(gentity_t* self)
{//decloak this entity
	if (self && self->client)
	{
		if (self->client->ps.powerups[PW_SPHERESHIELDED])
		{//Uncloak
			self->client->ps.powerups[PW_SPHERESHIELDED] = 0;
			G_SoundOnEnt(self, CHAN_ITEM, "sound/barrier/barrier_off.mp3");
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_DRAIN_RELEASE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
}

static void Jedi_CheckCloak(void)
{
	if (NPCS.NPC
		&& NPCS.NPC->client
		&& NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER
		&& Q_stricmpn("shadowtrooper", NPCS.NPC->NPC_type, 13) == 0)
	{
		if ( // !NPCS.NPC->client->ps.saberHolstered ||
			NPCS.NPC->health <= 0 ||
			NPCS.NPC->client->ps.saberInFlight ||
			NPCS.NPC->client->ps.fd.forceGripBeingGripped > level.time ||
			NPCS.NPC->painDebounceTime > level.time)
		{
			//can't be cloaked if saber is on, or dead or saber in flight or taking pain or being gripped
			Jedi_Decloak(NPCS.NPC);
		}
		else if (NPCS.NPC->health > 0
			&& !NPCS.NPC->client->ps.saberInFlight
			&& !(NPCS.NPC->client->ps.fd.forceGripBeingGripped > level.time)
			&& NPCS.NPC->painDebounceTime < level.time)
		{
			//still alive, have saber in hand, not taking pain and not being gripped
			if (NPCS.NPC->r.svFlags & SVF_BOT && !(NPCS.NPC->client->ps.communicatingflags & 1 <<
				CLOAK_CHARGE_RESTRICTION))
			{
				if (!PM_SaberInAttack(NPCS.NPC->client->ps.saber_move))
				{
					//using cloak, drain force
					Jedi_Cloak(NPCS.NPC);
				}
			}
			else
			{
				player_Cloak(NPCS.NPC);
			}
		}
	}
}

/*
==========================================================================================
AGGRESSION
==========================================================================================
*/
static void Jedi_Aggression(const gentity_t* self, const int change)
{
	int upper_threshold, lower_threshold;

	self->NPC->stats.aggression += change;

	//FIXME: base this on initial NPC stats
	if (self->client->playerTeam == NPCTEAM_PLAYER)
	{
		//good guys are less aggressive
		upper_threshold = 10;
		lower_threshold = 5;
	}
	else
	{
		//bad guys are more aggressive
		if (npc_is_dark_jedi(self))
		{
			upper_threshold = 20;
			lower_threshold = 10;
		}
		else
		{
			upper_threshold = 15;
			lower_threshold = 5;
		}
	}

	if (self->NPC->stats.aggression > upper_threshold)
	{
		self->NPC->stats.aggression = upper_threshold;
	}
	else if (self->NPC->stats.aggression < lower_threshold)
	{
		self->NPC->stats.aggression = lower_threshold;
	}
}

static void Jedi_AggressionErosion(const int amt)
{
	if (TIMER_Done(NPCS.NPC, "roamTime"))
	{
		//the longer we're not alerted and have no enemy, the more our aggression goes down
		TIMER_Set(NPCS.NPC, "roamTime", Q_irand(2000, 5000));
		Jedi_Aggression(NPCS.NPC, amt);
	}

	if (NPCS.NPCInfo->stats.aggression < 4 || NPCS.NPCInfo->stats.aggression < 6 && NPCS.NPC->client->NPC_class ==
		CLASS_DESANN)
	{
		//turn off the saber
		WP_DeactivateSaber(NPCS.NPC);
	}

	if (NPCS.NPCInfo->stats.aggression < 4 || NPCS.NPCInfo->stats.aggression < 6 && NPCS.NPC->client->NPC_class ==
		CLASS_VADER)
	{
		//turn off the saber
		WP_DeactivateSaber(NPCS.NPC);
	}
}

void NPC_Jedi_RateNewEnemy(const gentity_t* self, const gentity_t* enemy)
{
	float health_aggression;
	float weapon_aggression;

	switch (enemy->s.weapon)
	{
	case WP_SABER:
		health_aggression = (float)self->health / 200.0f * 6.0f;
		weapon_aggression = 10; //go after him
		break;
	case WP_BLASTER:
		if (DistanceSquared(self->r.currentOrigin, enemy->r.currentOrigin) < 65536) //256 squared
		{
			health_aggression = (float)self->health / 200.0f * 8.0f;
			weapon_aggression = 10; //go after him
		}
		else
		{
			health_aggression = 8.0f - (float)self->health / 200.0f * 8.0f;
			weapon_aggression = 5; //hang back for a second
		}
		break;
	default:
		health_aggression = (float)self->health / 200.0f * 8.0f;
		weapon_aggression = 9; //approach
		break;
	}
	//Average these with current aggression
	const int new_aggression = ceil((health_aggression + weapon_aggression + (float)self->NPC->stats.aggression) / 3.0f);
	Jedi_Aggression(self, new_aggression - self->NPC->stats.aggression);

	//don't taunt right away
	TIMER_Set(self, "chatter", Q_irand(4000, 7000));
}

static void Jedi_Rage(void)
{
	Jedi_Aggression(NPCS.NPC, 10 - NPCS.NPCInfo->stats.aggression + Q_irand(-2, 2));
	TIMER_Set(NPCS.NPC, "roamTime", 0);
	TIMER_Set(NPCS.NPC, "chatter", 0);
	TIMER_Set(NPCS.NPC, "walking", 0);
	TIMER_Set(NPCS.NPC, "taunting", 0);
	TIMER_Set(NPCS.NPC, "jumpChaseDebounce", 0);
	TIMER_Set(NPCS.NPC, "movenone", 0);
	TIMER_Set(NPCS.NPC, "movecenter", 0);
	TIMER_Set(NPCS.NPC, "noturn", 0);
	ForceRage(NPCS.NPC);
}

void Jedi_RageStop(const gentity_t* self)
{
	if (self->NPC)
	{
		//calm down and back off
		TIMER_Set(self, "roamTime", 0);
		Jedi_Aggression(self, Q_irand(-5, 0));
	}
}

/*
==========================================================================================
SPEAKING
==========================================================================================
*/

static qboolean Jedi_BattleTaunt(const gentity_t* NPC)
{
	if (TIMER_Done(NPCS.NPC, "chatter")
		&& !Q_irand(0, 3)
		&& NPCS.NPCInfo->blockedSpeechDebounceTime < level.time
		&& jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] < level.time)
	{
		int event = -1;

		if (NPCS.NPC->enemy
			&& NPCS.NPC->enemy->client
			&& (NPCS.NPC->enemy->client->NPC_class == CLASS_RANCOR
				|| NPCS.NPC->enemy->client->NPC_class == CLASS_WAMPA
				|| NPCS.NPC->enemy->client->NPC_class == CLASS_SAND_CREATURE))
		{
			//never taunt these mindless creatures
		}
		else
		{
			if (NPCS.NPC->client->playerTeam == NPCTEAM_PLAYER
				&& NPCS.NPC->enemy && NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->NPC_class == CLASS_JEDI)
			{
				//a jedi fighting a jedi - training
				if (NPCS.NPC->client->NPC_class == CLASS_JEDI && NPCS.NPCInfo->rank == RANK_COMMANDER)
				{
					//only trainer taunts
					event = EV_TAUNT1;
				}
			}
			else
			{
				//reborn or a jedi fighting an enemy
				event = Q_irand(EV_TAUNT1, EV_TAUNT3);
			}
			if (event != -1)
			{
				if (!npc_is_dark_jedi(NPCS.NPC))
				{
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
				}
				else
				{
					G_AddVoiceEvent(NPCS.NPC, event, 3000);
				}
				jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->blockedSpeechDebounceTime = level.
					time + 6000;
				if (NPCS.NPCInfo->aiFlags & NPCAI_ROSH)
				{
					//Rosh taunts less often
					TIMER_Set(NPCS.NPC, "chatter", Q_irand(8000, 20000));
				}
				else
				{
					TIMER_Set(NPCS.NPC, "chatter", Q_irand(5000, 10000));
				}

				if (NPCS.NPC->enemy && NPCS.NPC->enemy->NPC
					&& NPCS.NPC->enemy->s.weapon == WP_SABER
					&& NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->NPC_class == CLASS_JEDI)
				{
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 2000);
				}
				return qtrue;
			}
		}
	}
	return qfalse;
}

/*
==========================================================================================
MOVEMENT
==========================================================================================
*/
static qboolean Jedi_ClearPathToSpot(vec3_t dest, const int impactEntNum)
{
	trace_t trace;
	vec3_t mins, end, dir;
	float drop;

	//Offset the step height
	VectorSet(mins, NPCS.NPC->r.mins[0], NPCS.NPC->r.mins[1], NPCS.NPC->r.mins[2] + STEPSIZE);

	trap->Trace(&trace, NPCS.NPC->r.currentOrigin, mins, NPCS.NPC->r.maxs, dest, NPCS.NPC->s.number, NPCS.NPC->clipmask,
		qfalse, 0, 0);

	//Do a simple check
	if (trace.allsolid || trace.startsolid)
	{
		//inside solid
		return qfalse;
	}

	if (trace.fraction < 1.0f)
	{
		//hit something
		if (impactEntNum != ENTITYNUM_NONE && trace.entityNum == impactEntNum)
		{
			//hit what we're going after
			return qtrue;
		}
		return qfalse;
	}

	//otherwise, clear path in a straight line.
	//Now at intervals of my size, go along the trace and trace down STEPSIZE to make sure there is a solid floor.
	VectorSubtract(dest, NPCS.NPC->r.currentOrigin, dir);
	const float dist = VectorNormalize(dir);
	if (dest[2] > NPCS.NPC->r.currentOrigin[2])
	{
		//going up, check for steps
		drop = STEPSIZE;
	}
	else
	{
		//going down or level, check for moderate drops
		drop = 64;
	}
	for (float i = NPCS.NPC->r.maxs[0] * 2; i < dist; i += NPCS.NPC->r.maxs[0] * 2)
	{
		vec3_t start;
		//FIXME: does this check the last spot, too?  We're assuming that should be okay since the enemy is there?
		VectorMA(NPCS.NPC->r.currentOrigin, i, dir, start);
		VectorCopy(start, end);
		end[2] -= drop;
		trap->Trace(&trace, start, mins, NPCS.NPC->r.maxs, end, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0,
			0); //NPC->r.mins?
		if (trace.fraction < 1.0f || trace.allsolid || trace.startsolid)
		{
			//good to go
			continue;
		}
		//no floor here! (or a long drop?)
		return qfalse;
	}
	//we made it!
	return qtrue;
}

qboolean NPC_MoveDirClear(const int forwardmove, const int rightmove, const qboolean reset)
{
	vec3_t forward, right, testPos, angles, mins;
	trace_t trace;
	float bottom_max = -STEPSIZE * 4 - 1;

	if (!forwardmove && !rightmove)
	{
		//not even moving
		return qtrue;
	}

	if (NPCS.ucmd.upmove > 0 || NPCS.NPC->client->ps.fd.forceJumpCharge)
	{
		//Going to jump
		return qtrue;
	}

	if (NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//in the air
		return qtrue;
	}

	//FIXME: to really do this right, we'd have to actually do a pmove to predict where we're
	//going to be... maybe this should be a flag and pmove handles it and sets a flag so AI knows
	//NEXT frame?  Or just incorporate current velocity, runspeed and possibly friction?
	VectorCopy(NPCS.NPC->r.mins, mins);
	mins[2] += STEPSIZE;
	angles[PITCH] = angles[ROLL] = 0;
	angles[YAW] = NPCS.NPC->client->ps.viewangles[YAW]; //Add ucmd.angles[YAW]?
	AngleVectors(angles, forward, right, NULL);
	const float fwdDist = (float)forwardmove / 2.0f;
	const float rtDist = (float)rightmove / 2.0f;
	VectorMA(NPCS.NPC->r.currentOrigin, fwdDist, forward, testPos);
	VectorMA(testPos, rtDist, right, testPos);
	trap->Trace(&trace, NPCS.NPC->r.currentOrigin, mins, NPCS.NPC->r.maxs, testPos, NPCS.NPC->s.number,
		NPCS.NPC->clipmask | CONTENTS_BOTCLIP, qfalse, 0, 0);
	if (trace.allsolid || trace.startsolid)
	{
		//hmm, trace started inside this brush... how do we decide if we should continue?
		//FIXME: what do we do if we start INSIDE a CONTENTS_BOTCLIP? Try the trace again without that in the clipmask?
		if (reset)
		{
			trace.fraction = 1.0f;
		}
		VectorCopy(testPos, trace.endpos);
	}
	if (trace.fraction < 0.6)
	{
		//Going to bump into something very close, don't move, just turn
		if (NPCS.NPC->enemy && trace.entityNum == NPCS.NPC->enemy->s.number || NPCS.NPCInfo->goalEntity && trace.
			entityNum == NPCS.NPCInfo->goalEntity->s.number)
		{
			//okay to bump into enemy or goal
			return qtrue;
		}
		if (reset)
		{
			//actually want to screw with the ucmd
			//Com_Printf( "%d avoiding walk into wall (entnum %d)\n", level.time, trace.entityNum );
			NPCS.ucmd.forwardmove = 0;
			NPCS.ucmd.rightmove = 0;
			VectorClear(NPCS.NPC->client->ps.moveDir);
		}
		return qfalse;
	}

	if (NPCS.NPCInfo->goalEntity)
	{
		qboolean enemy_in_air = qfalse;

		if (NPCS.NPC->NPC->goalEntity->client)
		{
			if (NPCS.NPC->NPC->goalEntity->client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				enemy_in_air = qtrue;
			}
		}

		if (NPCS.NPCInfo->goalEntity->r.currentOrigin[2] < NPCS.NPC->r.currentOrigin[2])
		{
			//goal is below me, okay to step off at least that far plus stepheight
			if (!enemy_in_air)
			{
				bottom_max += NPCS.NPCInfo->goalEntity->r.currentOrigin[2] - NPCS.NPC->r.currentOrigin[2];
			}
		}
	}
	VectorCopy(trace.endpos, testPos);
	testPos[2] += bottom_max;

	trap->Trace(&trace, trace.endpos, mins, NPCS.NPC->r.maxs, testPos, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse,
		0, 0);

	if (trace.allsolid || trace.startsolid)
	{
		//Not going off a cliff
		return qtrue;
	}

	if (trace.fraction < 1.0)
	{
		//Not going off a cliff
		return qtrue;
	}

	//going to fall at least bottom_max, don't move, just turn... is this bad, though?  What if we want them to drop off?
	if (reset)
	{
		//actually want to screw with the ucmd
		NPCS.ucmd.forwardmove *= -1.0; //= 0;
		NPCS.ucmd.rightmove *= -1.0; //= 0;
		VectorScale(NPCS.NPC->client->ps.moveDir, -1, NPCS.NPC->client->ps.moveDir);
	}
	return qfalse;
}

/*
-------------------------
Jedi_HoldPosition
-------------------------
*/

static void Jedi_HoldPosition(void)
{
	NPCS.NPCInfo->goalEntity = NULL;
}

/*
-------------------------
Jedi_Move
-------------------------
*/

static qboolean Jedi_Move(gentity_t* goal, const qboolean retreat)
{
	navInfo_t info;

	NPCS.NPCInfo->combatMove = qtrue;
	NPCS.NPCInfo->goalEntity = goal;

	const qboolean moved = NPC_MoveToGoal(qtrue);

	if (retreat)
	{
		NPCS.ucmd.forwardmove *= -1;
		NPCS.ucmd.rightmove *= -1;
		VectorScale(NPCS.NPC->client->ps.moveDir, -1, NPCS.NPC->client->ps.moveDir);
	}

	//Get the move info
	NAV_GetLastMove(&info);

	//If we hit our target, then stop and fire!
	if (info.flags & NIF_COLLISION && info.blocker == NPCS.NPC->enemy)
	{
		Jedi_HoldPosition();
	}

	if (!moved)
	{
		//can't move towards target, hold here.
		Jedi_HoldPosition();
	}
	return moved;
}

static qboolean Jedi_Hunt(void)
{
	//if we're at all interested in fighting, go after him
	if (NPCS.NPCInfo->stats.aggression > 1)
	{
		//approach enemy
		NPCS.NPCInfo->combatMove = qtrue;
		if (!(NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES))
		{
			NPC_UpdateAngles(qtrue, qtrue);
			return qtrue;
		}
		NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
		NPCS.NPCInfo->goalRadius = 40.0f;

		if (NPC_MoveToGoal(qfalse))
		{
			NPC_UpdateAngles(qtrue, qtrue);
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean Jedi_Track(void)
{
	if (!NPC_IsJedi(NPCS.NPC))
	{
		return qfalse;
	}

	if (NPCS.NPCInfo->stats.aggression > 1)
	{
		//approach enemy
		NPCS.NPCInfo->combatMove = qtrue;
		NPC_SetMoveGoal(NPCS.NPC, NPCS.NPCInfo->enemyLastSeenLocation, 16, qtrue, 0, NPCS.NPC->enemy);
		if (NPC_MoveToGoal(qfalse))
		{
			NPC_UpdateAngles(qtrue, qtrue);
			return qtrue;
		}
	}
	return qfalse;
}

extern qboolean PM_PainAnim(int anim);

static void Jedi_StartBackOff(void)
{
	//start to evade a spin attack.
	TIMER_Set(NPCS.NPC, "roamTime", -level.time);
	TIMER_Set(NPCS.NPC, "strafeLeft", -level.time);
	TIMER_Set(NPCS.NPC, "strafeRight", -level.time);
	TIMER_Set(NPCS.NPC, "walking", -level.time);
	TIMER_Set(NPCS.NPC, "moveforward", -level.time);
	TIMER_Set(NPCS.NPC, "movenone", -level.time);
	TIMER_Set(NPCS.NPC, "moveright", -level.time);
	TIMER_Set(NPCS.NPC, "moveleft", -level.time);
	TIMER_Set(NPCS.NPC, "movecenter", -level.time);
	TIMER_Set(NPCS.NPC, "moveback", 1000);
	NPCS.ucmd.forwardmove = -127;
	NPCS.ucmd.rightmove = 0;
	NPCS.ucmd.upmove = 0;
	if (d_JediAI.integer || g_DebugSaberCombat.integer)
	{
		Com_Printf("%s backing off from spin attack!\n", NPCS.NPC->NPC_type);
	}
	TIMER_Set(NPCS.NPC, "specialEvasion", 1000);
	TIMER_Set(NPCS.NPC, "noRetreat", -level.time);
	if (PM_PainAnim(NPCS.NPC->client->ps.legsAnim))
	{
		//override leg pain animation if running
		NPCS.NPC->client->ps.legsTimer = 0;
	}
	VectorClear(NPCS.NPC->client->ps.moveDir);
}

static qboolean Jedi_Retreat(void)
{
	if (!TIMER_Done(NPCS.NPC, "noRetreat"))
	{
		//don't actually move
		return qfalse;
	}
	return Jedi_Move(NPCS.NPC->enemy, qtrue);
}

static void Jedi_Advance(void)
{
	if (NPCS.NPCInfo->aiFlags & NPCAI_HEAL_ROSH)
	{
		return;
	}
	if (!NPCS.NPC->client->ps.saberInFlight)
	{
		WP_ActivateSaber(NPCS.NPC);
	}
	Jedi_Move(NPCS.NPC->enemy, qfalse);
}

extern qboolean WP_SaberCanTurnOffSomeBlades(const saberInfo_t* saber);
extern qboolean G_ValidSaberStyle(const gentity_t* ent, int saber_style);

static void Jedi_AdjustSaberAnimLevel(const gentity_t* self, int new_level)
{
	if (!self || !self->client)
	{
		return;
	}
	//override stance with special saber style if we're using special sabers
	if (self->client->saber[0].model[0] && self->client->saber[1].model[0]
		&& !G_ValidSaberStyle(self, SS_DUAL))
	{
		self->client->ps.fd.saberAnimLevel = SS_DUAL;
		return;
	}
	if (self->client->saber[0].numBlades > 1
		&& WP_SaberCanTurnOffSomeBlades(&self->client->saber[0])
		&& !G_ValidSaberStyle(self, SS_STAFF))
	{
		self->client->ps.fd.saberAnimLevel = SS_STAFF;
		return;
	}

	if (self->client->playerTeam == NPCTEAM_ENEMY)
	{
		if (!Q_stricmp("cultist_saber_all", self->NPC_type)
			|| !Q_stricmp("cultist_saber_all_throw", self->NPC_type)
			|| !Q_stricmp("md_maul_tcw", self->NPC_type)
			|| !Q_stricmp("md_maul_cyber_tcw", self->NPC_type))
		{
			//use any, regardless of rank, etc.
		}
		else if (!Q_stricmp("cultist_saber", self->NPC_type)
			|| !Q_stricmp("cultist_saber_throw", self->NPC_type))
		{
			//fast only
			self->client->ps.fd.saberAnimLevel = SS_FAST;
		}
		else if (!Q_stricmp("cultist_saber_med", self->NPC_type)
			|| !Q_stricmp("cultist_saber_med_throw", self->NPC_type))
		{
			//med only
			self->client->ps.fd.saberAnimLevel = SS_MEDIUM;
		}
		else if (!Q_stricmp("cultist_saber_strong", self->NPC_type)
			|| !Q_stricmp("cultist_saber_strong_throw", self->NPC_type))
		{
			//strong only
			self->client->ps.fd.saberAnimLevel = SS_STRONG;
		}
		else if (!Q_stricmp("md_grievous", self->NPC_type)
			|| !Q_stricmp("md_grievous4", self->NPC_type)
			|| !Q_stricmp("md_grievous_robed", self->NPC_type)
			|| !Q_stricmp("md_clone_assassin", self->NPC_type)
			|| !Q_stricmp("md_jango", self->NPC_type)
			|| !Q_stricmp("md_jango_geo", self->NPC_type)
			|| !Q_stricmp("md_ani_ep2_dual", self->NPC_type)
			|| !Q_stricmp("md_serra", self->NPC_type)
			|| !Q_stricmp("md_ahsoka_rebels", self->NPC_type)
			|| !Q_stricmp("md_ahsoka", self->NPC_type)
			|| !Q_stricmp("md_ahsoka_s7", self->NPC_type)
			|| !Q_stricmp("md_ventress", self->NPC_type)
			|| !Q_stricmp("md_ven_ns", self->NPC_type)
			|| !Q_stricmp("md_ven_bh", self->NPC_type)
			|| !Q_stricmp("md_ven_dg", self->NPC_type)
			|| !Q_stricmp("md_asharad", self->NPC_type)
			|| !Q_stricmp("md_asharad_tus", self->NPC_type)
			|| !Q_stricmp("boba_fett", self->NPC_type)
			|| !Q_stricmp("boba_fett_esb", self->NPC_type)
			|| !Q_stricmp("md_pguard4", self->NPC_type)
			|| !Q_stricmp("md_grie_egg", self->NPC_type)
			|| !Q_stricmp("md_grie3_egg", self->NPC_type)
			|| !Q_stricmp("md_grie4_egg", self->NPC_type)
			|| !Q_stricmp("md_fet_ga", self->NPC_type)
			|| !Q_stricmp("md_fet2_ga", self->NPC_type)
			|| !Q_stricmp("md_fet3_ga", self->NPC_type)
			|| !Q_stricmp("md_ven2_ga", self->NPC_type)
			|| !Q_stricmp("md_ket_jt", self->NPC_type)
			|| !Q_stricmp("md_fet_ka", self->NPC_type)
			|| !Q_stricmp("md_fet2_ka", self->NPC_type)
			|| !Q_stricmp("md_clo2_rt", self->NPC_type)
			|| !Q_stricmp("md_clo5_rt", self->NPC_type)
			|| !Q_stricmp("reborn_dual", self->NPC_type)
			|| !Q_stricmp("reborn_dual2", self->NPC_type)
			|| !Q_stricmp("alora_dual", self->NPC_type)
			|| !Q_stricmp("JediTrainer", self->NPC_type)
			|| !Q_stricmp("jedi_zf2", self->NPC_type)
			|| !Q_stricmp("RebornMasterDual", self->NPC_type)
			|| !Q_stricmp("Tavion_scepter", self->NPC_type)
			|| !Q_stricmp("md_jed6_jt", self->NPC_type)
			|| !Q_stricmp("md_jed11_jt", self->NPC_type)
			|| !Q_stricmp("md_jed13_jt", self->NPC_type)
			|| !Q_stricmp("md_jediknight2_jt", self->NPC_type)
			|| !Q_stricmp("md_jediveteran2_jt", self->NPC_type)
			|| !Q_stricmp("md_jediveteran3_jt", self->NPC_type)
			|| !Q_stricmp("md_serra_jt", self->NPC_type))
		{
			//dual only
			self->client->ps.fd.saberAnimLevel = SS_DUAL;
		}
		else if (!Q_stricmp("md_magnaguard", self->NPC_type)
			|| !Q_stricmp("md_inquisitor", self->NPC_type)
			|| !Q_stricmp("md_5thbrother", self->NPC_type)
			|| !Q_stricmp("md_7thsister", self->NPC_type)
			|| !Q_stricmp("md_8thbrother", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels2", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels3", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels4", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels5", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels6", self->NPC_type)
			|| !Q_stricmp("md_jbrute", self->NPC_type)
			|| !Q_stricmp("md_templeguard", self->NPC_type)
			|| !Q_stricmp("md_sidious_tcw", self->NPC_type)
			|| !Q_stricmp("md_maul_tcw_staff", self->NPC_type)
			|| !Q_stricmp("md_savage", self->NPC_type)
			|| !Q_stricmp("md_galen", self->NPC_type)
			|| !Q_stricmp("md_galen_jt", self->NPC_type)
			|| !Q_stricmp("md_galencjr", self->NPC_type)
			|| !Q_stricmp("md_starkiller", self->NPC_type)
			|| !Q_stricmp("md_sithstalker", self->NPC_type)
			|| !Q_stricmp("md_stk_lord", self->NPC_type)
			|| !Q_stricmp("md_stk_tat", self->NPC_type)
			|| !Q_stricmp("darthdesolous", self->NPC_type)
			|| !Q_stricmp("purge_trooper", self->NPC_type)
			|| !Q_stricmp("cal_kestis_staff", self->NPC_type)
			|| !Q_stricmp("md_2ndsister", self->NPC_type)
			|| !Q_stricmp("md_shadowguard", self->NPC_type)
			|| !Q_stricmp("md_ven_dual", self->NPC_type)
			|| !Q_stricmp("md_pguard5", self->NPC_type)
			|| !Q_stricmp("md_mau_dof", self->NPC_type)
			|| !Q_stricmp("md_mau2_dof", self->NPC_type)
			|| !Q_stricmp("md_mau3_dof", self->NPC_type)
			|| !Q_stricmp("md_mag_egg", self->NPC_type)
			|| !Q_stricmp("md_mag_ga", self->NPC_type)
			|| !Q_stricmp("md_ven_ga", self->NPC_type)
			|| !Q_stricmp("md_mau_luke", self->NPC_type)
			|| !Q_stricmp("darthphobos", self->NPC_type)
			|| !Q_stricmp("md_tus5_tc", self->NPC_type)
			|| !Q_stricmp("md_sta_tfu", self->NPC_type)
			|| !Q_stricmp("md_gua1_tfu", self->NPC_type)
			|| !Q_stricmp("md_sta_cs", self->NPC_type)
			|| !Q_stricmp("md_gua_am", self->NPC_type)
			|| !Q_stricmp("md_gua2_am", self->NPC_type)
			|| !Q_stricmp("md_mag_am", self->NPC_type)
			|| !Q_stricmp("md_mag2_am", self->NPC_type)
			|| !Q_stricmp("JediF", self->NPC_type)
			|| !Q_stricmp("JediMaster", self->NPC_type)
			|| !Q_stricmp("jedi_kdm1", self->NPC_type)
			|| !Q_stricmp("jedi_tf1", self->NPC_type)
			|| !Q_stricmp("reborn_staff", self->NPC_type)
			|| !Q_stricmp("reborn_staff2", self->NPC_type)
			|| !Q_stricmp("RebornMasterStaff", self->NPC_type)
			|| !Q_stricmp("md_jed7_jt", self->NPC_type)
			|| !Q_stricmp("md_jed12_jt", self->NPC_type)
			|| !Q_stricmp("md_jed14_jt", self->NPC_type)
			|| !Q_stricmp("md_jedimaster3_jt", self->NPC_type)
			|| !Q_stricmp("md_jedimaster5_jt", self->NPC_type)
			|| !Q_stricmp("md_guard_jt", self->NPC_type)
			|| !Q_stricmp("md_guardboss_jt", self->NPC_type)
			|| !Q_stricmp("md_jedibrute_jt", self->NPC_type)
			|| !Q_stricmp("md_maul", self->NPC_type)
			|| !Q_stricmp("md_maul_robed", self->NPC_type)
			|| !Q_stricmp("md_maul_hooded", self->NPC_type)
			|| !Q_stricmp("md_maul_wots", self->NPC_type))
		{
			//staff only
			self->client->ps.fd.saberAnimLevel = SS_STAFF;
		}
		else
		{
			//use any, regardless of rank, etc.
		}
	}
	else
	{
		if (!Q_stricmp("cultist_saber_all", self->NPC_type)
			|| !Q_stricmp("cultist_saber_all_throw", self->NPC_type)
			|| !Q_stricmp("md_maul_tcw", self->NPC_type)
			|| !Q_stricmp("md_maul_cyber_tcw", self->NPC_type))
		{
			//use any, regardless of rank, etc.
		}
		else if (!Q_stricmp("cultist_saber", self->NPC_type)
			|| !Q_stricmp("cultist_saber_throw", self->NPC_type))
		{
			//fast only
			self->client->ps.fd.saberAnimLevel = SS_FAST;
		}
		else if (!Q_stricmp("cultist_saber_med", self->NPC_type)
			|| !Q_stricmp("cultist_saber_med_throw", self->NPC_type))
		{
			//med only
			self->client->ps.fd.saberAnimLevel = SS_MEDIUM;
		}
		else if (!Q_stricmp("cultist_saber_strong", self->NPC_type)
			|| !Q_stricmp("cultist_saber_strong_throw", self->NPC_type))
		{
			//strong only
			self->client->ps.fd.saberAnimLevel = SS_STRONG;
		}
		else if (!Q_stricmp("md_grievous", self->NPC_type)
			|| !Q_stricmp("md_grievous4", self->NPC_type)
			|| !Q_stricmp("md_grievous_robed", self->NPC_type)
			|| !Q_stricmp("md_clone_assassin", self->NPC_type)
			|| !Q_stricmp("md_jango", self->NPC_type)
			|| !Q_stricmp("md_jango_geo", self->NPC_type)
			|| !Q_stricmp("md_ani_ep2_dual", self->NPC_type)
			|| !Q_stricmp("md_serra", self->NPC_type)
			|| !Q_stricmp("md_ahsoka_rebels", self->NPC_type)
			|| !Q_stricmp("md_ahsoka", self->NPC_type)
			|| !Q_stricmp("md_ahsoka_s7", self->NPC_type)
			|| !Q_stricmp("md_ventress", self->NPC_type)
			|| !Q_stricmp("md_ven_ns", self->NPC_type)
			|| !Q_stricmp("md_ven_bh", self->NPC_type)
			|| !Q_stricmp("md_ven_dg", self->NPC_type)
			|| !Q_stricmp("md_asharad", self->NPC_type)
			|| !Q_stricmp("md_asharad_tus", self->NPC_type)
			|| !Q_stricmp("boba_fett", self->NPC_type)
			|| !Q_stricmp("boba_fett_esb", self->NPC_type)
			|| !Q_stricmp("md_pguard4", self->NPC_type)
			|| !Q_stricmp("md_grie_egg", self->NPC_type)
			|| !Q_stricmp("md_grie3_egg", self->NPC_type)
			|| !Q_stricmp("md_grie4_egg", self->NPC_type)
			|| !Q_stricmp("md_fet_ga", self->NPC_type)
			|| !Q_stricmp("md_fet2_ga", self->NPC_type)
			|| !Q_stricmp("md_fet3_ga", self->NPC_type)
			|| !Q_stricmp("md_ven2_ga", self->NPC_type)
			|| !Q_stricmp("md_ket_jt", self->NPC_type)
			|| !Q_stricmp("md_fet_ka", self->NPC_type)
			|| !Q_stricmp("md_fet2_ka", self->NPC_type)
			|| !Q_stricmp("md_clo2_rt", self->NPC_type)
			|| !Q_stricmp("md_clo5_rt", self->NPC_type)
			|| !Q_stricmp("reborn_dual", self->NPC_type)
			|| !Q_stricmp("reborn_dual2", self->NPC_type)
			|| !Q_stricmp("alora_dual", self->NPC_type)
			|| !Q_stricmp("JediTrainer", self->NPC_type)
			|| !Q_stricmp("jedi_zf2", self->NPC_type)
			|| !Q_stricmp("RebornMasterDual", self->NPC_type)
			|| !Q_stricmp("Tavion_scepter", self->NPC_type)
			|| !Q_stricmp("md_jed6_jt", self->NPC_type)
			|| !Q_stricmp("md_jed11_jt", self->NPC_type)
			|| !Q_stricmp("md_jed13_jt", self->NPC_type)
			|| !Q_stricmp("md_jediknight2_jt", self->NPC_type)
			|| !Q_stricmp("md_jediveteran2_jt", self->NPC_type)
			|| !Q_stricmp("md_jediveteran3_jt", self->NPC_type)
			|| !Q_stricmp("md_serra_jt", self->NPC_type))
		{
			//dual only
			self->client->ps.fd.saberAnimLevel = SS_DUAL;
		}
		else if (!Q_stricmp("md_magnaguard", self->NPC_type)
			|| !Q_stricmp("md_inquisitor", self->NPC_type)
			|| !Q_stricmp("md_5thbrother", self->NPC_type)
			|| !Q_stricmp("md_7thsister", self->NPC_type)
			|| !Q_stricmp("md_8thbrother", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels2", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels3", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels4", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels5", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels6", self->NPC_type)
			|| !Q_stricmp("md_jbrute", self->NPC_type)
			|| !Q_stricmp("md_templeguard", self->NPC_type)
			|| !Q_stricmp("md_sidious_tcw", self->NPC_type)
			|| !Q_stricmp("md_maul_tcw_staff", self->NPC_type)
			|| !Q_stricmp("md_savage", self->NPC_type)
			|| !Q_stricmp("md_galen", self->NPC_type)
			|| !Q_stricmp("md_galen_jt", self->NPC_type)
			|| !Q_stricmp("md_galencjr", self->NPC_type)
			|| !Q_stricmp("md_starkiller", self->NPC_type)
			|| !Q_stricmp("md_sithstalker", self->NPC_type)
			|| !Q_stricmp("md_stk_lord", self->NPC_type)
			|| !Q_stricmp("md_stk_tat", self->NPC_type)
			|| !Q_stricmp("darthdesolous", self->NPC_type)
			|| !Q_stricmp("purge_trooper", self->NPC_type)
			|| !Q_stricmp("cal_kestis_staff", self->NPC_type)
			|| !Q_stricmp("md_2ndsister", self->NPC_type)
			|| !Q_stricmp("md_shadowguard", self->NPC_type)
			|| !Q_stricmp("md_ven_dual", self->NPC_type)
			|| !Q_stricmp("md_pguard5", self->NPC_type)
			|| !Q_stricmp("md_mau_dof", self->NPC_type)
			|| !Q_stricmp("md_mau2_dof", self->NPC_type)
			|| !Q_stricmp("md_mau3_dof", self->NPC_type)
			|| !Q_stricmp("md_mag_egg", self->NPC_type)
			|| !Q_stricmp("md_mag_ga", self->NPC_type)
			|| !Q_stricmp("md_ven_ga", self->NPC_type)
			|| !Q_stricmp("md_mau_luke", self->NPC_type)
			|| !Q_stricmp("darthphobos", self->NPC_type)
			|| !Q_stricmp("md_tus5_tc", self->NPC_type)
			|| !Q_stricmp("md_sta_tfu", self->NPC_type)
			|| !Q_stricmp("md_gua1_tfu", self->NPC_type)
			|| !Q_stricmp("md_sta_cs", self->NPC_type)
			|| !Q_stricmp("md_gua_am", self->NPC_type)
			|| !Q_stricmp("md_gua2_am", self->NPC_type)
			|| !Q_stricmp("md_mag_am", self->NPC_type)
			|| !Q_stricmp("md_mag2_am", self->NPC_type)
			|| !Q_stricmp("JediF", self->NPC_type)
			|| !Q_stricmp("JediMaster", self->NPC_type)
			|| !Q_stricmp("jedi_kdm1", self->NPC_type)
			|| !Q_stricmp("jedi_tf1", self->NPC_type)
			|| !Q_stricmp("reborn_staff", self->NPC_type)
			|| !Q_stricmp("reborn_staff2", self->NPC_type)
			|| !Q_stricmp("RebornMasterStaff", self->NPC_type)
			|| !Q_stricmp("md_jed7_jt", self->NPC_type)
			|| !Q_stricmp("md_jed12_jt", self->NPC_type)
			|| !Q_stricmp("md_jed14_jt", self->NPC_type)
			|| !Q_stricmp("md_jedimaster3_jt", self->NPC_type)
			|| !Q_stricmp("md_jedimaster5_jt", self->NPC_type)
			|| !Q_stricmp("md_guard_jt", self->NPC_type)
			|| !Q_stricmp("md_guardboss_jt", self->NPC_type)
			|| !Q_stricmp("md_jedibrute_jt", self->NPC_type)
			|| !Q_stricmp("md_maul", self->NPC_type)
			|| !Q_stricmp("md_maul_robed", self->NPC_type)
			|| !Q_stricmp("md_maul_hooded", self->NPC_type)
			|| !Q_stricmp("md_maul_wots", self->NPC_type))
		{
			//staff only
			self->client->ps.fd.saberAnimLevel = SS_STAFF;
		}
		else
		{
			//use any, regardless of rank, etc.
		}
	}
	if (!G_ValidSaberStyle(self, new_level))
	{
		for (int count = SS_FAST; count < SS_STAFF; count++)
		{
			new_level++;
			if (new_level > SS_STAFF)
			{
				new_level = SS_FAST;
			}

			if (G_ValidSaberStyle(self, new_level))
			{
				break;
			}
		}
	}

	//set stance
	self->client->ps.fd.saberAnimLevel = new_level;

	if (d_JediAI.integer || g_DebugSaberCombat.integer)
	{
		switch (self->client->ps.fd.saberAnimLevel)
		{
		case SS_FAST:
			Com_Printf(S_COLOR_GREEN"%s Saber Attack Set: fast\n", self->NPC_type);
			break;
		case SS_MEDIUM:
			Com_Printf(S_COLOR_YELLOW"%s Saber Attack Set: medium\n", self->NPC_type);
			break;
		case SS_STRONG:
			Com_Printf(S_COLOR_RED"%s Saber Attack Set: strong\n", self->NPC_type);
			break;
		case SS_DESANN:
			Com_Printf(S_COLOR_RED"%s Saber Attack Set: desann\n", self->NPC_type);
			break;
		case SS_TAVION:
			Com_Printf(S_COLOR_RED"%s Saber Attack Set: tavion\n", self->NPC_type);
			break;
		case SS_DUAL:
			Com_Printf(S_COLOR_CYAN"%s Saber Attack Set: dual\n", self->NPC_type);
			break;
		case SS_STAFF:
			Com_Printf(S_COLOR_ORANGE"%s Saber Attack Set: staff\n", self->NPC_type);
			break;
		default:;
		}
	}
}

static void Jedi_CheckDecreasesaber_anim_level(void)
{
	if (NPCS.NPC->client->ps.fd.saberAnimLevel == SS_DUAL
		|| NPCS.NPC->client->ps.fd.saberAnimLevel == SS_STAFF)
	{
		return;
	}
	if (!NPCS.NPC->client->ps.weaponTime && !(NPCS.ucmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK)))
	{
		//not attacking
		if (TIMER_Done(NPCS.NPC, "saberLevelDebounce") && !Q_irand(0, 10))
		{
			Jedi_AdjustSaberAnimLevel(NPCS.NPC, Q_irand(SS_FAST, SS_TAVION)); //random
			TIMER_Set(NPCS.NPC, "saberLevelDebounce", Q_irand(3000, 10000));
		}
	}
	else
	{
		TIMER_Set(NPCS.NPC, "saberLevelDebounce", Q_irand(1000, 5000));
	}
}

extern qboolean G_InGetUpAnim(const playerState_t* ps);

static qboolean Jedi_DecideKick(void)
{
	//check to see if we feel like kicking
	if (PM_InKnockDown(&NPCS.NPC->client->ps))
	{
		return qfalse;
	}
	if (BG_InRoll(&NPCS.NPC->client->ps, NPCS.NPC->client->ps.legsAnim))
	{
		return qfalse;
	}
	if (G_InGetUpAnim(&NPCS.NPC->client->ps))
	{
		return qfalse;
	}
	if (!NPCS.NPC->enemy || NPCS.NPC->enemy->s.number < MAX_CLIENTS && NPCS.NPC->enemy->health <= 0)
	{
		//have no enemy or enemy is a dead player
		return qfalse;
	}
	if (Q_irand(0, RANK_CAPTAIN + 5) > NPCS.NPCInfo->rank)
	{
		//low chance, based on rank
		return qfalse;
	}
	if (Q_irand(0, 10) > NPCS.NPCInfo->stats.aggression)
	{
		//the madder the better
		return qfalse;
	}
	if (!TIMER_Done(NPCS.NPC, "kickDebounce"))
	{
		//just did one
		return qfalse;
	}
	if (NPCS.NPC->client->ps.weapon == WP_SABER)
	{
		if (NPCS.NPC->client->saber[0].saberFlags & SFL_NO_KICKS)
		{
			//can't kick with this saber
			return qfalse;
		}
		if (NPCS.NPC->client->saber[1].model
			&& NPCS.NPC->client->saber[1].model[0]
			&& NPCS.NPC->client->saber[1].saberFlags & SFL_NO_KICKS)
		{
			//can't kick with this saber
			return qfalse;
		}
	}
	//go for it!
	return qtrue;
}

/*
==========================================================================================
KYLE GRAPPLE CODE
==========================================================================================
*/

void Kyle_GrabEnemy(void)
{
	TIMER_Set(NPCS.NPC, "grabEnemyDebounce", NPCS.NPC->client->ps.torsoTimer + Q_irand(4000, 20000));
}

void Kyle_TryGrab(void)
{
	//have the kyle boss NPC attempt a grab.
	NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	NPCS.NPC->client->ps.torsoTimer += 200;
	NPCS.NPC->client->ps.weaponTime = NPCS.NPC->client->ps.torsoTimer;
	NPCS.NPC->client->ps.saber_move = LS_READY;
	VectorClear(NPCS.NPC->client->ps.velocity);
	VectorClear(NPCS.NPC->client->ps.moveDir);
	NPCS.ucmd.rightmove = NPCS.ucmd.forwardmove = NPCS.ucmd.upmove = 0;
	NPCS.NPC->painDebounceTime = level.time + NPCS.NPC->client->ps.torsoTimer;
	NPCS.NPC->client->ps.saberHolstered = 2;
}

extern qboolean PM_InOnGroundAnim(const int anim);

qboolean Kyle_CanDoGrab(void)
{
	//Kyle can do grab on this enemy
	if (NPCS.NPC->client->NPC_class == CLASS_KYLE ||
		NPCS.NPC->client->NPC_class == CLASS_LUKE ||
		NPCS.NPC->client->NPC_class == CLASS_ALORA ||
		NPCS.NPC->client->NPC_class == CLASS_DESANN &&
		NPCS.NPC->spawnflags & 1)
	{
		//Boss Kyle
		if (NPCS.NPC->enemy && NPCS.NPC->enemy->client)
		{
			//have a valid enemy
			if (TIMER_Done(NPCS.NPC, "grabEnemyDebounce"))
			{
				//okay to grab again
				if (NPCS.NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
					&& NPCS.NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					//me and enemy are on ground
					if (!PM_InOnGroundAnim(NPCS.NPC->enemy->client->ps.legsAnim))
					{
						if ((NPCS.NPC->client->ps.weaponTime <= 200 || NPCS.NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB)
							&& !NPCS.NPC->client->ps.saberInFlight)
						{
							if (fabs(NPCS.NPC->enemy->r.currentOrigin[2] - NPCS.NPC->r.currentOrigin[2]) <= 8.0f)
							{
								//close to same level of ground
								if (DistanceSquared(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin) <=
									10000.0f)
								{
									return qtrue;
								}
							}
						}
					}
				}
			}
		}
	}
	return qfalse;
}

saber_moveName_t G_PickAutoMultiKick(gentity_t* self, qboolean allowSingles, qboolean storeMove)
{
	return LS_NONE;
}

extern qboolean G_CanKickEntity(const gentity_t* self, const gentity_t* target);
extern saber_moveName_t G_PickAutoKick(gentity_t* self, const gentity_t* enemy);
extern float NPC_EnemyRangeFromBolt(int boltIndex);
extern qboolean PM_SaberInTransition(int move);
extern void ForceDrain(gentity_t* self);
extern qboolean PM_CrouchAnim(int anim);
static qboolean shoot;
static float enemyDist;
#define MELEE_DIST_SQUARED 6400

static void Jedi_CombatDistance(const int enemy_dist)
{
	//FIXME: for many of these checks, what we really want is horizontal distance to enemy
	enemyDist = DistanceSquared(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin);

	if (Jedi_CultistDestroyer(NPCS.NPC))
	{
		//destroyers suicide charge
		Jedi_Advance();
		//always run, regardless of what navigation tells us to do!
		NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.runSpeed;
		NPCS.ucmd.buttons &= ~BUTTON_WALKING;
		return;
	}
	if (enemy_dist < 128
		&& NPCS.NPC->enemy
		&& NPCS.NPC->enemy->client
		&& (NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK6
			|| NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK7
			|| NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACKGRIEVOUS))
	{
		//whoa, back off!!! attempt to dodge spin attacks
		if (Q_irand(-3, NPCS.NPCInfo->rank) > RANK_CREWMAN)
		{
			Jedi_StartBackOff();
			return;
		}
	}
	//saber users must walk in combat
	if (enemy_dist < 170
		&& NPCS.NPC->enemy
		&& NPCS.NPC->client->ps.weapon == WP_SABER
		&& NPCS.NPC->enemy->s.weapon == WP_SABER
		&& NPCS.NPC->enemy->client)
	{
		if (enemy_dist <= 150 && (NPCS.NPC->client->ps.fd.saberAnimLevel == SS_DUAL || NPCS.NPC->client->ps.fd.
			saberAnimLevel == SS_STAFF))
		{
			NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.walkSpeed;
			NPCS.ucmd.buttons |= BUTTON_WALKING;
		}
		else if (enemy_dist <= 80)
		{
			NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.walkSpeed;
			NPCS.ucmd.buttons |= BUTTON_WALKING;
		}
		else
		{
			NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.runSpeed;
			NPCS.ucmd.buttons &= ~BUTTON_WALKING;
		}
	}

	if (TIMER_Done(NPCS.NPC, "blocking")
		&& (!PM_SaberInAttack(NPCS.NPC->client->ps.saber_move)
			&& !PM_SaberInStart(NPCS.NPC->client->ps.saber_move)
			&& !PM_SaberInBrokenParry(NPCS.NPC->client->ps.saber_move)
			&& !PM_InRoll(&NPCS.NPC->client->ps)
			&& !PM_InKnockDown(&NPCS.NPC->client->ps)
			&& !pm_saber_in_special_attack(NPCS.NPC->client->ps.torsoAnim)
			&& !NPCS.NPC->client->ps.weaponTime)
		&& enemy_dist < 80
		&& !(NPCS.ucmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK))
		&& NPCS.NPC->enemy
		&& NPCS.NPC->enemy->client
		&& NPCS.NPC->client->ps.weapon == WP_SABER
		&& NPCS.NPC->enemy->s.weapon == WP_SABER)
	{
		NPCS.ucmd.buttons |= BUTTON_BLOCK;

		if (TIMER_Done(NPCS.NPC, "TalkTime"))
		{
			if (npc_is_dark_jedi(NPCS.NPC))
			{
				// Do taunt/anger...
				const int call_out = Q_irand(0, 3);

				switch (call_out)
				{
				case 3:
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_JCHASE1, EV_JCHASE3), 5000 + Q_irand(0, 15000));
					break;
				case 2:
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 5000 + Q_irand(0, 15000));
					break;
				case 1:
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_ANGER1, EV_ANGER1), 5000 + Q_irand(0, 15000));
					break;
				default:
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 5000 + Q_irand(0, 15000));
					break;
				}
			}
			else if (npc_is_light_jedi(NPCS.NPC))
			{
				// Do taunt...
				const int call_out = Q_irand(0, 2);

				switch (call_out)
				{
				case 2:
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_JCHASE1, EV_JCHASE3), 5000 + Q_irand(0, 15000));
					break;
				case 1:
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 5000 + Q_irand(0, 15000));
					break;
				default:
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 5000 + Q_irand(0, 15000));
				}
			}
			else
			{
				// Do taunt/anger...
				const int call_out = Q_irand(0, 3);

				switch (call_out)
				{
				case 3:
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_JCHASE1, EV_JCHASE3), 5000 + Q_irand(0, 15000));
					break;
				case 2:
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 5000 + Q_irand(0, 15000));
					break;
				case 1:
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_ANGER1, EV_ANGER1), 5000 + Q_irand(0, 15000));
					break;
				default:
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 5000 + Q_irand(0, 15000));
					break;
				}
			}
			TIMER_Set(NPCS.NPC, "TalkTime", 5000);
		}
	}

	if (enemyDist < MELEE_DIST_SQUARED
		&& NPCS.NPC->enemy
		&& NPCS.NPC->enemy->client
		&& NPCS.NPC->client->ps.weapon == WP_SABER
		&& NPCS.NPC->enemy->s.weapon == WP_SABER)
	{
		NPCS.ucmd.buttons &= ~BUTTON_BLOCK;
		TIMER_Set(NPCS.NPC, "blocking", NPCS.NPC->client->ps.torsoTimer + Q_irand(8000, 12000));
	}

	//SLAP
	shoot = qfalse;

	if (NPCS.NPC->client->NPC_class == CLASS_KYLE ||
		NPCS.NPC->client->NPC_class == CLASS_JEDI ||
		NPCS.NPC->client->NPC_class == CLASS_YODA ||
		NPCS.NPC->client->NPC_class == CLASS_LUKE
		&& NPCS.NPC->client->playerTeam == NPCTEAM_PLAYER
		&& !(NPCS.ucmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCEPOWER | BUTTON_DASH | BUTTON_USE |
			BUTTON_BLOCK))
		&& !PM_InKnockDown(&NPCS.NPC->client->ps))
	{
		if (NPCS.NPC->client->ps.torsoAnim == BOTH_A7_SLAP_R ||
			NPCS.NPC->client->ps.torsoAnim == BOTH_A7_SLAP_L ||
			NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_B2 ||
			NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_B3)
		{
			shoot = qfalse;
			if (TIMER_Done(NPCS.NPC, "smackTime") && !NPCS.NPCInfo->blockedDebounceTime)
			{
				//time to smack
				//recheck enemyDist and InFront
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPCS.NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPCS.NPC->client->ps) //not knocked down
					&& in_front(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin,
						NPCS.NPC->client->ps.viewangles, 0.3f))
				{
					vec3_t smack_dir;
					VectorSubtract(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, smack_dir);
					smack_dir[2] += 20;
					VectorNormalize(smack_dir);
					//hurt them
					G_Sound(NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/chars/%s/misc/pain0%d"));
					G_Damage(NPCS.NPC->enemy, NPCS.NPC, NPCS.NPC, smack_dir, NPCS.NPC->r.currentOrigin,
						(g_npcspskill.integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);
					if (NPCS.NPC->client->ps.torsoAnim == BOTH_A7_SLAP_L)
					{
						//smackdown
						int knockAnim = BOTH_KNOCKDOWN1;
						if (PM_CrouchAnim(NPCS.NPC->enemy->client->ps.legsAnim))
						{
							//knockdown from crouch
							knockAnim = BOTH_KNOCKDOWN4;
						}
						//throw them
						smack_dir[2] = 1;
						VectorNormalize(smack_dir);
						g_throw(NPCS.NPC->enemy, smack_dir, 30);
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, knockAnim,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (NPCS.NPC->client->ps.torsoAnim == BOTH_A7_SLAP_R)
					{
						g_throw(NPCS.NPC->enemy, smack_dir, 40);
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, BOTH_KNOCKDOWN5,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						//uppercut
						//throw them
						g_throw(NPCS.NPC->enemy, smack_dir, 50);
						//make them backflip
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, BOTH_KNOCKDOWN2,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					//done with the damage
					NPCS.NPCInfo->blockedDebounceTime = 1;
				}
			}
		}
	}

	//slap
	if (NPCS.NPC->client->NPC_class == CLASS_KYLE ||
		NPCS.NPC->client->NPC_class == CLASS_JEDI ||
		NPCS.NPC->client->NPC_class == CLASS_YODA ||
		NPCS.NPC->client->NPC_class == CLASS_LUKE
		&& NPCS.NPC->client->playerTeam == NPCTEAM_PLAYER
		&& !(NPCS.ucmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_BLOCK))
		&& !PM_InKnockDown(&NPCS.NPC->client->ps))
	{
		if (enemyDist < MELEE_DIST_SQUARED
			&& !NPCS.NPC->client->ps.weaponTime //not firing
			&& !PM_InKnockDown(&NPCS.NPC->client->ps) //not knocked down
			&& in_front(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, NPCS.NPC->client->ps.viewangles,
				0.3f)) //within 80 and in front
		{
			//enemy within 80, if very close, use melee attack to slap away
			if (TIMER_Done(NPCS.NPC, "attackDelay"))
			{
				const int swing_anim = Q_irand(BOTH_A7_SLAP_L, BOTH_A7_SLAP_R);
				G_Sound(NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/weapons/melee/kick1.mp3"));
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.torsoTimer + Q_irand(10000, 15000));
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPCS.NPC, "smackTime", 300);
				NPCS.NPCInfo->blockedDebounceTime = 0;
			}
		}
		else if (enemyDist < MELEE_DIST_SQUARED
			&& !NPCS.NPC->client->ps.weaponTime //not firing
			&& !PM_InKnockDown(&NPCS.NPC->client->ps) //not knocked down
			&& !in_front(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, NPCS.NPC->client->ps.viewangles,
				-0.25f)) //within 80 and generally behind
		{
			//enemy within 80, if very close, use melee attack to slap away
			if (TIMER_Done(NPCS.NPC, "attackDelay"))
			{
				const int swing_anim = Q_irand(BOTH_A7_KICK_B2, BOTH_A7_KICK_B3);
				G_Sound(NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/weapons/melee/kick1.mp3"));
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.torsoTimer + Q_irand(10000, 15000));
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPCS.NPC, "smackTime", 300);
				NPCS.NPCInfo->blockedDebounceTime = 0;
			}
		}
	}

	if (NPCS.NPC->client->NPC_class == CLASS_DESANN ||
		NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER ||
		NPCS.NPC->client->NPC_class == CLASS_TAVION ||
		NPCS.NPC->client->NPC_class == CLASS_ALORA ||
		NPCS.NPC->client->NPC_class == CLASS_REBORN
		&& !(NPCS.ucmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_BLOCK))
		&& NPCS.NPC->client->playerTeam == NPCTEAM_ENEMY
		&& !PM_InKnockDown(&NPCS.NPC->client->ps))
	{
		if (NPCS.NPC->client->ps.torsoAnim == BOTH_A7_SLAP_R ||
			NPCS.NPC->client->ps.torsoAnim == BOTH_A7_SLAP_L ||
			NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_B2 ||
			NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_B)
		{
			shoot = qfalse;
			if (TIMER_Done(NPCS.NPC, "smackTime") && !NPCS.NPCInfo->blockedDebounceTime)
			{
				//time to smack
				//recheck enemyDist and InFront
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPCS.NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPCS.NPC->client->ps) //not knocked down
					&& in_front(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin,
						NPCS.NPC->client->ps.viewangles, 0.3f))
				{
					vec3_t smack_dir;
					VectorSubtract(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, smack_dir);
					smack_dir[2] += 20;
					VectorNormalize(smack_dir);
					//hurt them
					G_Sound(NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/chars/%s/misc/pain0%d"));
					G_Damage(NPCS.NPC->enemy, NPCS.NPC, NPCS.NPC, smack_dir, NPCS.NPC->r.currentOrigin,
						(g_npcspskill.integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);
					if (NPCS.NPC->client->ps.torsoAnim == BOTH_A7_SLAP_L)
					{
						//smackdown
						int knockAnim = BOTH_KNOCKDOWN1;
						if (PM_CrouchAnim(NPCS.NPC->enemy->client->ps.legsAnim))
						{
							//knockdown from crouch
							knockAnim = BOTH_KNOCKDOWN4;
						}
						//throw them
						smack_dir[2] = 1;
						VectorNormalize(smack_dir);
						g_throw(NPCS.NPC->enemy, smack_dir, 30);
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, knockAnim,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (NPCS.NPC->client->ps.torsoAnim == BOTH_A7_SLAP_R)
					{
						g_throw(NPCS.NPC->enemy, smack_dir, 40);
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, BOTH_KNOCKDOWN5,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						//uppercut
						//throw them
						g_throw(NPCS.NPC->enemy, smack_dir, 50);
						//make them backflip
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, BOTH_KNOCKDOWN2,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					//done with the damage
					NPCS.NPCInfo->blockedDebounceTime = 1;
				}
			}
		}
	}

	//slap
	if (NPCS.NPC->client->NPC_class == CLASS_DESANN ||
		NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER ||
		NPCS.NPC->client->NPC_class == CLASS_TAVION ||
		NPCS.NPC->client->NPC_class == CLASS_ALORA ||
		NPCS.NPC->client->NPC_class == CLASS_REBORN
		&& !(NPCS.ucmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_BLOCK))
		&& NPCS.NPC->client->playerTeam == NPCTEAM_ENEMY
		&& !PM_InKnockDown(&NPCS.NPC->client->ps))
	{
		if (enemyDist < MELEE_DIST_SQUARED
			&& !NPCS.NPC->client->ps.weaponTime //not firing
			&& !PM_InKnockDown(&NPCS.NPC->client->ps) //not knocked down
			&& in_front(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, NPCS.NPC->client->ps.viewangles,
				0.3f)) //within 80 and in front
		{
			//enemy within 80, if very close, use melee attack to slap away
			if (TIMER_Done(NPCS.NPC, "attackDelay"))
			{
				const int swing_anim = Q_irand(BOTH_A7_SLAP_L, BOTH_A7_SLAP_R);
				G_Sound(NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/weapons/melee/kick1.mp3"));
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.torsoTimer + Q_irand(15000, 20000));
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPCS.NPC, "smackTime", 300);
				NPCS.NPCInfo->blockedDebounceTime = 0;
			}
		}
		else if (enemyDist < MELEE_DIST_SQUARED
			&& !NPCS.NPC->client->ps.weaponTime //not firing
			&& !PM_InKnockDown(&NPCS.NPC->client->ps) //not knocked down
			&& !in_front(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, NPCS.NPC->client->ps.viewangles,
				-0.25f)) //within 80 and generally behind
		{
			//enemy within 80, if very close, use melee attack to slap away
			if (TIMER_Done(NPCS.NPC, "attackDelay"))
			{
				const int swing_anim = Q_irand(BOTH_A7_KICK_B2, BOTH_A7_KICK_B);
				G_Sound(NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/weapons/melee/kick1.mp3"));
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.torsoTimer + Q_irand(15000, 20000));
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPCS.NPC, "smackTime", 300);
				NPCS.NPCInfo->blockedDebounceTime = 0;
			}
		}
	}

	if (NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_GRIP &&
		NPCS.NPC->client->ps.fd.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1)
	{
		//when gripping, don't move
		return;
	}
	if (!TIMER_Done(NPCS.NPC, "gripping"))
	{
		//stopped gripping, clear timers just in case
		TIMER_Set(NPCS.NPC, "gripping", -level.time);
		TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(0, 1000));
	}

	if (NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_DRAIN &&
		NPCS.NPC->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1)
	{
		//when draining, don't move
		return;
	}
	if (!TIMER_Done(NPCS.NPC, "draining"))
	{
		//stopped draining, clear timers just in case
		TIMER_Set(NPCS.NPC, "draining", -level.time);
		TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(0, 1000));
	}

	if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT
		|| NPCS.NPC->client->pers.nextbotclass == BCLASS_BOBAFETT
		|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN1
		|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN2)
	{
		if (!TIMER_Done(NPCS.NPC, "flameTime"))
		{
			if (enemy_dist > 50)
			{
				Jedi_Advance();
			}
			else if (enemy_dist <= 0)
			{
				Jedi_Retreat();
			}
		}
		else if (enemy_dist < 200)
		{
			Jedi_Retreat();
		}
		else if (enemy_dist > 1024)
		{
			Jedi_Advance();
		}
	}
	else if (NPCS.NPC->client->ps.legsAnim == BOTH_ALORA_SPIN_THROW_MD2)
	{
		//don't move at all when alora is doing her spin throw attack.
	}
	else if (NPCS.NPC->client->ps.legsAnim == BOTH_ALORA_SPIN_THROW)
	{
		//don't move at all when alora is doing her spin throw attack.
	}
	else if (NPCS.NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB)
	{
		//see if we grabbed enemy
		if (NPCS.NPC->client->ps.torsoTimer <= 200)
		{
			if (Kyle_CanDoGrab()
				&& NPC_EnemyRangeFromBolt(0) <= 72.0f)
			{
				//grab him!
				Kyle_GrabEnemy();
				return;
			}
			//biffed it!
			NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			NPCS.NPC->client->ps.weaponTime = NPCS.NPC->client->ps.torsoTimer;
			return;
		}
		//else just sit here?
		return;
	}
	else if (NPCS.NPC->client->ps.saberInFlight &&
		!PM_SaberInBrokenParry(NPCS.NPC->client->ps.saber_move)
		&& NPCS.NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN)
	{
		//maintain distance
		if (enemy_dist < NPCS.NPC->client->ps.saberEntityDist)
		{
			Jedi_Retreat();
		}
		else if (enemy_dist > NPCS.NPC->client->ps.saberEntityDist && enemy_dist > 100)
		{
			Jedi_Advance();
		}
		if (NPCS.NPC->client->ps.weapon == WP_SABER //using saber
			&& NPCS.NPC->client->ps.saberEntityState == SES_LEAVING //not returning yet
			&& NPCS.NPC->client->ps.fd.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_1 //2nd or 3rd level lightsaber
			&& !(NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
			&& !(NPCS.NPC->client->ps.saberEventFlags & SEF_INWATER)) //saber not in water
		{
			//hold it out there
			NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
			NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
		}
	}
	else if (!TIMER_Done(NPCS.NPC, "taunting"))
	{
		if (enemy_dist <= 64)
		{
			//he's getting too close
			NPCS.ucmd.buttons &= ~BUTTON_ALT_ATTACK;
			NPCS.ucmd.buttons |= BUTTON_ATTACK;
			if (!NPCS.NPC->client->ps.saberInFlight)
			{
				WP_ActivateSaber(NPCS.NPC);
			}
			TIMER_Set(NPCS.NPC, "taunting", -level.time);
		}
		else if (NPCS.NPC->client->ps.forceHandExtend == HANDEXTEND_JEDITAUNT && NPCS.NPC->client->ps.
			forceHandExtendTime - level.time < 200)
		{
			//we're almost done with our special taunt
			if (!NPCS.NPC->client->ps.saberInFlight)
			{
				WP_ActivateSaber(NPCS.NPC);
			}
		}
	}
	else if (NPCS.NPC->client->ps.saberEventFlags & SEF_LOCK_WON)
	{
		//we won a saber lock, press the advantage
		if (enemy_dist > 0)
		{
			//get closer so we can hit!
			Jedi_Advance();
		}
		if (enemy_dist > 128)
		{
			//lost 'em
			NPCS.NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON;
		}
		if (NPCS.NPC->enemy->painDebounceTime + 2000 < level.time)
		{
			//the window of opportunity is gone
			NPCS.NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON;
		}
		//don't strafe?
		TIMER_Set(NPCS.NPC, "strafeLeft", -1);
		TIMER_Set(NPCS.NPC, "strafeRight", -1);
	}
	else if (NPCS.NPC->enemy->client
		&& NPCS.NPC->enemy->s.weapon == WP_SABER
		&& NPCS.NPC->enemy->client->ps.saberLockTime > level.time
		&& NPCS.NPC->client->ps.saberLockTime < level.time)
	{
		//enemy is in a saberLock and we are not
		if (enemy_dist < 64)
		{
			//FIXME: maybe just pick another enemy?
			Jedi_Retreat();
		}
	}
	else if (NPCS.NPC->enemy->s.weapon == WP_TURRET
		&& !Q_stricmp("PAS", NPCS.NPC->enemy->classname)
		&& NPCS.NPC->enemy->s.apos.trType == TR_STATIONARY)
	{
		int testlevel;
		if (enemy_dist > forcePushPullRadius[FORCE_LEVEL_1] - 16)
		{
			Jedi_Advance();
		}
		if (NPCS.NPC->client->ps.fd.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_1)
		{
			//
			testlevel = FORCE_LEVEL_1;
		}
		else
		{
			testlevel = NPCS.NPC->client->ps.fd.forcePowerLevel[FP_PUSH];
		}
		if (enemy_dist < forcePushPullRadius[testlevel] - 16)
		{
			//close enough to push
			if (in_front(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->client->renderInfo.eyePoint,
				NPCS.NPC->client->renderInfo.eyeAngles, 0.6f))
			{
				//knock it down
				//do the forcethrow call just for effect
				ForceThrow(NPCS.NPC, qfalse);
			}
		}
	}
	else if (enemy_dist <= 64
		&& (NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE
			|| (!Q_stricmp("T_yoda", NPCS.NPC->NPC_type)
				|| (!Q_stricmp("jedi_kdm1", NPCS.NPC->NPC_type)
					|| (!Q_stricmp("RebornBoss", NPCS.NPC->NPC_type)
						|| (!Q_stricmp("T_Palpatine_sith", NPCS.NPC->NPC_type)
							|| !Q_stricmp("Yoda", NPCS.NPC->NPC_type)
							&& !Q_irand(0, 10)))))))
	{
		//can't use saber and they're in striking range
		if (!Q_irand(0, 5) && in_front(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin,
			NPCS.NPC->client->ps.viewangles, 0.2f))
		{
			if ((NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE || NPCS.NPC->client->pers.maxHealth - NPCS.NPC->health >
				NPCS.NPC->client->pers.maxHealth * 0.25f) //lost over 1/4 of our health or not firing
				&& WP_ForcePowerUsable(NPCS.NPC, FP_DRAIN) //know how to drain and have enough power
				&& !Q_irand(0, 2))
			{
				//drain
				TIMER_Set(NPCS.NPC, "draining", 3000);
				TIMER_Set(NPCS.NPC, "attackDelay", 3000);
				Jedi_Advance();
				return;
			}
			//ok, we can't drain at the moment
			if (Jedi_DecideKick())
			{
				//let's try a kick
				if (G_PickAutoMultiKick(NPCS.NPC, qfalse, qtrue) != LS_NONE
					|| G_CanKickEntity(NPCS.NPC, NPCS.NPC->enemy) && G_PickAutoKick(NPCS.NPC, NPCS.NPC->enemy)
					!= LS_NONE)
				{
					//kicked!
					TIMER_Set(NPCS.NPC, "kickDebounce", Q_irand(3000, 10000));
					return;
				}
			}
			ForceThrow(NPCS.NPC, qfalse);
		}
		Jedi_Retreat();
	}
	else if (enemy_dist <= 64
		&& NPCS.NPC->client->pers.maxHealth - NPCS.NPC->health > NPCS.NPC->client->pers.maxHealth * 0.25f
		//lost over 1/4 of our health
		&& NPCS.NPC->client->ps.fd.forcePowersKnown & 1 << FP_DRAIN //know how to drain
		&& WP_ForcePowerAvailable(NPCS.NPC, FP_DRAIN, 20) //have enough power
		&& !Q_irand(0, 10)
		&& in_front(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, NPCS.NPC->client->ps.viewangles, 0.2f))
	{
		TIMER_Set(NPCS.NPC, "draining", 3000);
		TIMER_Set(NPCS.NPC, "attackDelay", 3000);
		Jedi_Advance();
		return;
	}
	else if (enemy_dist <= -16)
	{
		//we're too damn close!
		if (!Q_irand(0, 30) && Kyle_CanDoGrab())
		{
			//try to grapple
			Kyle_TryGrab();
			return;
		}
		//else if (NPCS.NPC->client->NPC_class == CLASS_TAVION
		//	&& (!Q_stricmp("tavion_scepter", NPCS.NPC->NPC_type))//WP_SCEPTER
		//	&& !Q_irand(0, 20))
		//{
		//	Tavion_StartScepterSlam();
		//	return;
		//}
		if (Jedi_DecideKick())
		{
			//let's try a kick
			if (G_PickAutoMultiKick(NPCS.NPC, qfalse, qtrue) != LS_NONE
				|| G_CanKickEntity(NPCS.NPC, NPCS.NPC->enemy) && G_PickAutoKick(NPCS.NPC, NPCS.NPC->enemy) !=
				LS_NONE)
			{
				//kicked!
				TIMER_Set(NPCS.NPC, "kickDebounce", Q_irand(3000, 10000));
				return;
			}
		}
		Jedi_Retreat();
	}
	else if (enemy_dist <= 0)
	{
		//we're within striking range
		if (NPCS.NPCInfo->stats.aggression < 4)
		{
			//back off and defend
			if (!Q_irand(0, 30)
				&& Kyle_CanDoGrab())
			{
				Kyle_TryGrab();
				return;
			}
			//else if (NPCS.NPC->client->NPC_class == CLASS_TAVION
			//	&& (!Q_stricmp("tavion_scepter", NPCS.NPC->NPC_type))//WP_SCEPTER
			//	&& !Q_irand(0, 20))
			//{
			//	Tavion_StartScepterSlam();
			//	return;
			//}
			if (Jedi_DecideKick())
			{
				//let's try a kick
				if (G_PickAutoMultiKick(NPCS.NPC, qfalse, qtrue) != LS_NONE
					|| G_CanKickEntity(NPCS.NPC, NPCS.NPC->enemy) && G_PickAutoKick(NPCS.NPC, NPCS.NPC->enemy)
					!= LS_NONE)
				{
					//kicked!
					TIMER_Set(NPCS.NPC, "kickDebounce", Q_irand(3000, 10000));
					return;
				}
			}
			Jedi_Retreat();
		}
	}
	else if (enemy_dist > 256)
	{
		//we're way out of range
		qboolean usedForce = qfalse;
		if (NPCS.NPCInfo->stats.aggression < Q_irand(0, 20)
			&& NPCS.NPC->health < NPCS.NPC->client->pers.maxHealth * 0.9f
			&& !Q_irand(0, 2))
		{
			//racc - we're out of rand so let's check for a special activity to do.
			if (NPCS.NPC->enemy
				&& NPCS.NPC->enemy->s.number < MAX_CLIENTS
				&& (NPCS.NPC->client->NPC_class != CLASS_KYLE && NPCS.NPC->client->NPC_class != CLASS_YODA)
				&& (NPCS.NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER
					|| NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER)
				&& Q_irand(0, 3 - g_npcspskill.integer))
			{
				//hmm, bosses should do this less against the player
			}
			else if (NPCS.NPC->client->saber[0].type == SABER_SITH_SWORD
				&& NPCS.NPC->client->weaponGhoul2[0])
			{
				//packing the sith sword.  we can use it to recharge our health.
				Tavion_SithSwordRecharge();
				usedForce = qtrue;
			}
			else if ((NPCS.NPC->client->ps.fd.forcePowersKnown & 1 << FP_HEAL) != 0
				&& (NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_HEAL) == 0
				&& Q_irand(0, 1))
			{
				ForceHeal(NPCS.NPC);
				usedForce = qtrue;

				if (NPC_IsJedi(NPCS.NPC))
				{
					// Do deflect taunt...
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_GLOAT1, EV_GLOAT3), 5000 + Q_irand(0, 15000));
				}
			}
			else if ((NPCS.NPC->client->ps.fd.forcePowersKnown & 1 << FP_TELEPATHY) != 0
				&& (NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_TELEPATHY) == 0
				&& !Q_irand(0, 2))
			{
				// npcs can now use Mind Trick
				ForceTelepathy(NPCS.NPC);
				usedForce = qtrue;
			}
			else if ((NPCS.NPC->client->ps.fd.forcePowersKnown & 1 << FP_PROTECT) != 0
				&& (NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_PROTECT) == 0
				&& Q_irand(0, 1))
			{
				ForceProtect(NPCS.NPC);
				usedForce = qtrue;

				if (NPC_IsJedi(NPCS.NPC))
				{
					// Do deflect taunt...
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_COVER1, EV_COVER5), 5000 + Q_irand(0, 15000));
				}
			}
			else if ((NPCS.NPC->client->ps.fd.forcePowersKnown & 1 << FP_ABSORB) != 0
				&& (NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_ABSORB) == 0
				&& Q_irand(0, 1))
			{
				ForceAbsorb(NPCS.NPC);
				usedForce = qtrue;

				if (NPC_IsJedi(NPCS.NPC))
				{
					// Do deflect taunt...
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_GIVEUP1, EV_GIVEUP4), 5000 + Q_irand(0, 15000));
				}
			}
			else if ((NPCS.NPC->client->ps.fd.forcePowersKnown & 1 << FP_RAGE) != 0
				&& (NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_RAGE) == 0
				&& Q_irand(0, 1))
			{
				Jedi_Rage();
				usedForce = qtrue;

				if (NPC_IsJedi(NPCS.NPC))
				{
					// Do deflect taunt...
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_ANGER1, EV_ANGER3), 5000 + Q_irand(0, 15000));
				}
			}
			//FIXME: what about things like mind tricks and force sight?
		}
		if (enemy_dist > 384)
		{
			//FIXME: check for enemy facing away and/or moving away
			if (!Q_irand(0, 10) && NPCS.NPCInfo->blockedSpeechDebounceTime < level.time && jediSpeechDebounceTime[NPCS.
				NPC->client->playerTeam] < level.time)
			{
				if (NPC_ClearLOS4(NPCS.NPC->enemy))
				{
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_JCHASE1, EV_JCHASE3), 3000);
				}
				jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->blockedSpeechDebounceTime = level.
					time + 3000;
			}
		}
		//Unless we're totally hiding, go after him
		if (NPCS.NPCInfo->stats.aggression > 0)
		{
			//approach enemy
			if (!usedForce)
			{
				if (NPCS.NPC->enemy
					&& NPCS.NPC->enemy->client
					&& (NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK6
						|| NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK7
						|| NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACKGRIEVOUS))
				{
					//stay put!  They're trying to slice us up!
				}
				else
				{
					Jedi_Advance();
				}
			}
		}
	}
	else if (enemy_dist > 50) //FIXME: not hardcoded- base on our reach (modelScale?) and saberLengthMax
	{
		//we're out of striking range and we are allowed to attack
		//first, check some tactical force power decisions
		if (NPCS.NPC->enemy && NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->ps.fd.forceGripBeingGripped > level.
			time)
		{
			//They're being gripped, rush them!
			if (NPCS.NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				//they're on the ground, so advance
				if (TIMER_Done(NPCS.NPC, "parryTime") || NPCS.NPCInfo->rank > RANK_LT)
				{
					//not parrying
					if (enemy_dist > 200 || !(NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE))
					{
						//far away or allowed to use saber
						Jedi_Advance();
					}
				}
			}
			if (NPCS.NPC->client->ps.fd.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2
				&& TIMER_Done(NPCS.NPC, "attackDelay") || (NPCS.NPCInfo->rank >= RANK_LT_COMM || WP_ForcePowerUsable(
					NPCS.NPC, FP_SABERTHROW))
				&& !Q_irand(0, 5)
				&& !(NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
				&& !(NPCS.NPC->client->ps.saberEventFlags & SEF_INWATER)) //saber not in water
			{
				//throw saber
				NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
				NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
			}
			TIMER_Set(NPCS.NPC, "attackDelay", 3000);
		}
		else if (NPCS.NPC->enemy && NPCS.NPC->enemy->client && //valid enemy
			NPCS.NPC->enemy->client->ps.saberInFlight && NPCS.NPC->enemy->client->ps.saberEntityNum &&
			//enemy throwing saber
			NPCS.NPC->client->ps.weaponTime <= 0 && //I'm not busy
			WP_ForcePowerAvailable(NPCS.NPC, FP_GRIP, 0) && //I can use the power
			!Q_irand(0, 10) && //don't do it all the time, averages to 1 check a second
			Q_irand(0, 6) < g_npcspskill.integer && //more likely on harder diff
			Q_irand(RANK_CIVILIAN, RANK_CAPTAIN) < NPCS.NPCInfo->rank //more likely against harder enemies
			&& InFOV3(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, NPCS.NPC->client->ps.viewangles, 20,
				30))
		{
			//They're throwing their saber, grip them!
			//taunt
			if (TIMER_Done(NPCS.NPC, "chatter") && jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] < level.time &&
				NPCS.NPCInfo->blockedSpeechDebounceTime < level.time)
			{
				if (!npc_is_dark_jedi(NPCS.NPC))
				{
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
				}
				else
				{
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 10000);
				}
				jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->blockedSpeechDebounceTime = level.
					time + 3000;
				if (NPCS.NPCInfo->aiFlags & NPCAI_ROSH)
				{
					TIMER_Set(NPCS.NPC, "chatter", 6000);
				}
				else
				{
					TIMER_Set(NPCS.NPC, "chatter", 3000);
				}
			}

			//grip
			TIMER_Set(NPCS.NPC, "gripping", 3000);
			TIMER_Set(NPCS.NPC, "attackDelay", 3000);
		}
		else
		{
			if (NPCS.NPC->enemy && NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->ps.fd.forcePowersActive & 1 <<
				FP_GRIP)
			{
				//They're choking someone, probably an ally, run at them and do some sort of attack
				if (NPCS.NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					//they're on the ground, so advance
					if (TIMER_Done(NPCS.NPC, "parryTime") || NPCS.NPCInfo->rank > RANK_LT)
					{
						//not parrying
						if (enemy_dist > 200 || !(NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE))
						{
							//far away or allowed to use saber
							Jedi_Advance();
						}
					}
				}
			}
			if (NPCS.NPC->client->NPC_class == CLASS_KYLE || NPCS.NPC->client->NPC_class == CLASS_YODA
				&& NPCS.NPC->spawnflags & 1
				&& (NPCS.NPC->enemy && NPCS.NPC->enemy->client && !NPCS.NPC->enemy->client->ps.saberInFlight)
				&& TIMER_Done(NPCS.NPC, "kyleTakesSaber")
				&& !Q_irand(0, 20))
			{
				//kyle tries to pull the player's saber out of their hands
				ForceThrow(NPCS.NPC, qtrue);
			}
			//else if (NPCS.NPC->client->NPC_class == CLASS_TAVION
			//	&& (!Q_stricmp("tavion_scepter", NPCS.NPC->NPC_type))//WP_SCEPTER
			//	&& !Q_irand(0, 20))
			//{
			//	Tavion_StartScepterBeam();
			//	return;
			//}
			else
			{
				int chanceScale = 0;
				if ((NPCS.NPC->client->NPC_class == CLASS_KYLE || NPCS.NPC->client->NPC_class == CLASS_YODA) && NPCS.
					NPC->spawnflags & 1)
				{
					chanceScale = 4;
				}
				else if (NPCS.NPC->enemy
					&& NPCS.NPC->enemy->s.number < MAX_CLIENTS
					&& (NPCS.NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER
						|| NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER))
				{
					//hmm, bosses do this less against player
					chanceScale = 8 - g_npcspskill.integer * 2;
				}

				if (NPCS.NPC->client->NPC_class == CLASS_DESANN)
				{
					chanceScale = 1;
				}
				else if (NPCS.NPCInfo->rank == RANK_ENSIGN)
				{
					chanceScale = 2;
				}
				else if (NPCS.NPCInfo->rank >= RANK_LT_JG)
				{
					chanceScale = 5;
				}
				if (chanceScale
					&& (enemy_dist > Q_irand(100, 200) || NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE || !Q_stricmp(
						"Yoda", NPCS.NPC->NPC_type) && !Q_irand(0, 3))
					&& enemy_dist < 500
					&& (Q_irand(0, chanceScale * 10) < 5 || NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->ps.
						weapon != WP_SABER && !Q_irand(0, chanceScale)))
				{
					//else, randomly try some kind of attack every now and then
					if ((NPCS.NPCInfo->rank == RANK_ENSIGN
						|| NPCS.NPCInfo->rank > RANK_LT_JG)
						&& (!Q_irand(0, 1) || NPCS.NPC->s.weapon != WP_SABER))
					{
						if (WP_ForcePowerUsable(NPCS.NPC, FP_PULL) && !Q_irand(0, 2))
						{
							//maybe strafe too?
							TIMER_Set(NPCS.NPC, "duck", enemy_dist * 3);
							if (Q_irand(0, 1))
							{
								NPCS.ucmd.buttons &= ~BUTTON_ALT_ATTACK;
								NPCS.ucmd.buttons |= BUTTON_ATTACK;
							}
						}
						//let lightning cultists spam the lightning.
						else if (WP_ForcePowerUsable(NPCS.NPC, FP_LIGHTNING)
							&& (NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE && Q_stricmp(
								"cultist_lightning", NPCS.NPC->NPC_type) || Q_irand(0, 1)))
						{
							ForceLightning(NPCS.NPC);
							if (NPCS.NPC->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1)
							{
								NPCS.NPC->client->ps.weaponTime = Q_irand(1000, 3000 + g_npcspskill.integer * 500);
								TIMER_Set(NPCS.NPC, "holdLightning", NPCS.NPC->client->ps.weaponTime);
							}
							TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.weaponTime);
						}
						else if (NPCS.NPC->health < NPCS.NPC->client->pers.maxHealth * 0.75f
							&& Q_irand(FORCE_LEVEL_0, NPCS.NPC->client->ps.fd.forcePowerLevel[FP_DRAIN]) > FORCE_LEVEL_1
							&& WP_ForcePowerAvailable(NPCS.NPC, FP_DRAIN, 0)
							&& Q_irand(0, 1))
						{
							NPCS.NPC->client->ps.weaponTime = Q_irand(1000, 3000 + g_npcspskill.integer * 500);
							TIMER_Set(NPCS.NPC, "draining", NPCS.NPC->client->ps.weaponTime);
							TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.weaponTime);
						}
						else if (WP_ForcePowerUsable(NPCS.NPC, FP_GRIP)
							&& NPCS.NPC->enemy
							&& InFOV3(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin,
								NPCS.NPC->client->ps.viewangles, 20, 30))
						{
							//taunt
							if (TIMER_Done(NPCS.NPC, "chatter") && jediSpeechDebounceTime[NPCS.NPC->client->playerTeam]
								< level.time && NPCS.NPCInfo->blockedSpeechDebounceTime < level.time)
							{
								if (!npc_is_dark_jedi(NPCS.NPC))
								{
									G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
								}
								else
								{
									G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 10000);
								}
								jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->
									blockedSpeechDebounceTime = level.time + 3000;
								if (NPCS.NPCInfo->aiFlags & NPCAI_ROSH)
								{
									//Rosh has a longer taunt?
									TIMER_Set(NPCS.NPC, "chatter", 6000);
								}
								else
								{
									TIMER_Set(NPCS.NPC, "chatter", 3000);
								}
							}

							//grip
							TIMER_Set(NPCS.NPC, "gripping", 3000);
							TIMER_Set(NPCS.NPC, "attackDelay", 3000);
						}
						else
						{
							if (WP_ForcePowerAvailable(NPCS.NPC, FP_SABERTHROW, 0)
								&& !(NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
								&& !(NPCS.NPC->client->ps.saberEventFlags & SEF_INWATER)) //saber not in water
							{
								//throw saber
								NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
								NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
							}
						}
					}
				}
				//see if we should advance now
				else if (NPCS.NPCInfo->stats.aggression > 5)
				{
					//approach enemy
					if (TIMER_Done(NPCS.NPC, "parryTime") || NPCS.NPCInfo->rank > RANK_LT)
					{
						//not parrying
						if (!NPCS.NPC->enemy->client || NPCS.NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
						{
							//they're on the ground, so advance
							if (enemy_dist > 200 || !(NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE))
							{
								//far away or allowed to use saber
								Jedi_Advance();
							}
						}
					}
				}
				else
				{
					//maintain this distance?
					//walk?
				}
			}
		}
	}
	else
	{
		//we're not close enough to attack, but not far enough away to be safe
		//racc - kyle tries to grab even when out of range?
		if (!Q_irand(0, 30)
			&& Kyle_CanDoGrab())
		{
			Kyle_TryGrab();
			return;
		}
		if (NPCS.NPCInfo->stats.aggression < 4)
		{
			//back off and defend
			if (Jedi_DecideKick())
			{
				//let's try a kick
				if (G_PickAutoMultiKick(NPCS.NPC, qfalse, qtrue) != LS_NONE
					|| G_CanKickEntity(NPCS.NPC, NPCS.NPC->enemy)
					&& G_PickAutoKick(NPCS.NPC, NPCS.NPC->enemy) != LS_NONE)
				{
					//kicked!
					TIMER_Set(NPCS.NPC, "kickDebounce", Q_irand(3000, 10000));
					return;
				}
			}
			Jedi_Retreat();
		}
		else if (NPCS.NPCInfo->stats.aggression > 5)
		{
			//try to get closer
			if (enemy_dist > 0 && !(NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE))
			{
				//we're allowed to use our lightsaber, get closer
				if (TIMER_Done(NPCS.NPC, "parryTime") || NPCS.NPCInfo->rank > RANK_LT)
				{
					//not parrying
					if (!NPCS.NPC->enemy->client || NPCS.NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{
						//they're on the ground, so advance
						Jedi_Advance();
					}
				}
			}
		}
		else
		{
			//agression is 4 or 5... somewhere in the middle
			//what do we do here?  Nothing?
			//Move forward and back?
		}
	}
	//if really really mad, rage!
	if (enemy_dist > 1024 && NPCS.NPCInfo->stats.aggression > Q_irand(5, 15)
		&& NPCS.NPC->health < NPCS.NPC->client->pers.maxHealth * 0.75f
		&& !Q_irand(0, 2))
	{
		if ((NPCS.NPC->client->ps.fd.forcePowersKnown & 1 << FP_RAGE) != 0
			&& (NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_RAGE) == 0)
		{
			Jedi_Rage();
		}
	}
}

static qboolean jedi_strafe(const int strafe_time_min, const int strafe_time_max, const int next_strafe_time_min,
	const int next_strafe_time_max,
	const qboolean walking)
{
	if (Jedi_CultistDestroyer(NPCS.NPC))
	{
		return qfalse;
	}
	if (NPCS.NPC->client->ps.saberEventFlags & SEF_LOCK_WON && NPCS.NPC->enemy && NPCS.NPC->enemy->painDebounceTime >
		level.time)
	{
		//don't strafe if pressing the advantage of winning a saberLock
		return qfalse;
	}
	if (TIMER_Done(NPCS.NPC, "strafeLeft") && TIMER_Done(NPCS.NPC, "strafeRight"))
	{
		qboolean strafed = qfalse;
		//TODO: make left/right choice a tactical decision rather than random:
		//		try to keep own back away from walls and ledges,
		//		try to keep enemy's back to a ledge or wall
		//		Maybe try to strafe toward designer-placed "safe spots" or "goals"?
		const int strafeTime = Q_irand(strafe_time_min, strafe_time_max);

		if (Q_irand(0, 1))
		{
			if (NPC_MoveDirClear(NPCS.ucmd.forwardmove, -127, qfalse))
			{
				TIMER_Set(NPCS.NPC, "strafeLeft", strafeTime);
				strafed = qtrue;
			}
			else if (NPC_MoveDirClear(NPCS.ucmd.forwardmove, 127, qfalse))
			{
				TIMER_Set(NPCS.NPC, "strafeRight", strafeTime);
				strafed = qtrue;
			}
		}
		else
		{
			if (NPC_MoveDirClear(NPCS.ucmd.forwardmove, 127, qfalse))
			{
				TIMER_Set(NPCS.NPC, "strafeRight", strafeTime);
				strafed = qtrue;
			}
			else if (NPC_MoveDirClear(NPCS.ucmd.forwardmove, -127, qfalse))
			{
				TIMER_Set(NPCS.NPC, "strafeLeft", strafeTime);
				strafed = qtrue;
			}
		}

		if (strafed)
		{
			TIMER_Set(NPCS.NPC, "noStrafe", strafeTime + Q_irand(next_strafe_time_min, next_strafe_time_max));
			if (walking)
			{
				//should be a slow strafe
				TIMER_Set(NPCS.NPC, "walking", strafeTime);
			}
			return qtrue;
		}
	}
	return qfalse;
}

extern int PM_AnimLength(const animNumber_t anim);

evasionType_t Jedi_CheckFlipEvasions(gentity_t* self, const float rightdot, float zdiff)
{
	if (self->NPC && self->NPC->scriptFlags & SCF_NO_ACROBATICS)
	{
		return EVASION_NONE;
	}
	if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO)
	{
		//boba can't flip
		return EVASION_NONE;
	}
	if (self->client
		&& (self->client->ps.fd.forceRageRecoveryTime > level.time || self->client->ps.fd.forcePowersActive & 1 <<
			FP_RAGE))
	{
		//no fancy dodges when raging
		return EVASION_NONE;
	}

	if (self->client->ps.legsAnim == BOTH_WALL_RUN_LEFT || self->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT)
	{
		//already running on a wall
		vec3_t right, fwdAngles;
		int anim = -1;

		VectorSet(fwdAngles, 0, self->client->ps.viewangles[YAW], 0);

		AngleVectors(fwdAngles, NULL, right, NULL);

		const float anim_length = PM_AnimLength((animNumber_t)self->client->ps.legsAnim);
		if (self->client->ps.legsAnim == BOTH_WALL_RUN_LEFT && rightdot < 0)
		{
			//I'm running on a wall to my left and the attack is on the left
			if (anim_length - self->client->ps.legsTimer > 400
				&& self->client->ps.legsTimer > 400)
			{
				//not at the beginning or end of the anim
				anim = BOTH_WALL_RUN_LEFT_FLIP;
			}
		}
		else if (self->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT && rightdot > 0)
		{
			//I'm running on a wall to my right and the attack is on the right
			if (anim_length - self->client->ps.legsTimer > 400
				&& self->client->ps.legsTimer > 400)
			{
				//not at the beginning or end of the anim
				anim = BOTH_WALL_RUN_RIGHT_FLIP;
			}
		}
		if (anim != -1)
		{
			//FIXME: check the direction we will flip towards for do-not-enter/walls/drops?
			//NOTE: we presume there is still a wall there!
			if (anim == BOTH_WALL_RUN_LEFT_FLIP)
			{
				self->client->ps.velocity[0] *= 0.5f;
				self->client->ps.velocity[1] *= 0.5f;
				VectorMA(self->client->ps.velocity, 150, right, self->client->ps.velocity);
			}
			else if (anim == BOTH_WALL_RUN_RIGHT_FLIP)
			{
				self->client->ps.velocity[0] *= 0.5f;
				self->client->ps.velocity[1] *= 0.5f;
				VectorMA(self->client->ps.velocity, -150, right, self->client->ps.velocity);
			}
			int parts = SETANIM_LEGS;
			if (!self->client->ps.weaponTime)
			{
				parts = SETANIM_BOTH;
			}
			NPC_SetAnim(self, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			//self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
			//rwwFIXMEFIXME: Add these pm flags?
			G_AddEvent(self, EV_JUMP, 0);
			return EVASION_OTHER;
		}
	}
	else if (self->client->NPC_class != CLASS_DESANN //desann doesn't do these kind of frilly acrobatics
		&& (self->NPC->rank == RANK_CREWMAN || self->NPC->rank >= RANK_LT)
		&& Q_irand(0, 1)
		&& !BG_InRoll(&self->client->ps, self->client->ps.legsAnim)
		&& !PM_InKnockDown(&self->client->ps)
		&& !pm_saber_in_special_attack(self->client->ps.torsoAnim))
	{
		vec3_t fwd, right, traceto, mins, maxs, fwdAngles;
		trace_t trace;
		int anim;
		float speed, checkDist;
		qboolean allowCartWheels = qtrue;
		qboolean allowWallFlips = qtrue;

		if (self->client->ps.weapon == WP_SABER)
		{
			if (self->client->saber[0].model[0]
				&& self->client->saber[0].saberFlags & SFL_NO_CARTWHEELS)
			{
				allowCartWheels = qfalse;
			}
			else if (self->client->saber[1].model[0]
				&& self->client->saber[1].saberFlags & SFL_NO_CARTWHEELS)
			{
				allowCartWheels = qfalse;
			}
			if (self->client->saber[0].model[0]
				&& self->client->saber[0].saberFlags & SFL_NO_WALL_FLIPS)
			{
				allowWallFlips = qfalse;
			}
			else if (self->client->saber[1].model[0]
				&& self->client->saber[1].saberFlags & SFL_NO_WALL_FLIPS)
			{
				allowWallFlips = qfalse;
			}
		}

		VectorSet(mins, self->r.mins[0], self->r.mins[1], 0);
		VectorSet(maxs, self->r.maxs[0], self->r.maxs[1], 24);
		VectorSet(fwdAngles, 0, self->client->ps.viewangles[YAW], 0);

		AngleVectors(fwdAngles, fwd, right, NULL);

		int parts = SETANIM_BOTH;

		if (PM_SaberInAttack(self->client->ps.saber_move)
			|| PM_SaberInStart(self->client->ps.saber_move))
		{
			parts = SETANIM_LEGS;
		}
		if (rightdot >= 0)
		{
			if (Q_irand(0, 1))
			{
				anim = BOTH_ARIAL_LEFT;
			}
			else
			{
				anim = BOTH_CARTWHEEL_LEFT;
			}
			checkDist = -128;
			speed = -200;
		}
		else
		{
			if (Q_irand(0, 1))
			{
				anim = BOTH_ARIAL_RIGHT;
			}
			else
			{
				anim = BOTH_CARTWHEEL_RIGHT;
			}
			checkDist = 128;
			speed = 200;
		}
		//trace in the dir that we want to go
		VectorMA(self->r.currentOrigin, checkDist, right, traceto);
		trap->Trace(&trace, self->r.currentOrigin, mins, maxs, traceto, self->s.number,
			CONTENTS_SOLID | CONTENTS_MONSTERCLIP | CONTENTS_BOTCLIP, qfalse, 0, 0);
		if (trace.fraction >= 1.0f && allowCartWheels)
		{
			//it's clear, let's do it
			//FIXME: check for drops?
			vec3_t jumpRt;

			NPC_SetAnim(self, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			self->client->ps.weaponTime = self->client->ps.legsTimer; //don't attack again until this anim is done
			VectorCopy(self->client->ps.viewangles, fwdAngles);
			fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
			//do the flip
			AngleVectors(fwdAngles, NULL, jumpRt, NULL);
			VectorScale(jumpRt, speed, self->client->ps.velocity);
			self->client->ps.fd.forceJumpCharge = 0; //so we don't play the force flip anim
			self->client->ps.velocity[2] = 200;
			self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];
			//so we don't take damage if we land at same height
			//self->client->ps.pm_flags |= PMF_JUMPING;
			if (self->client->NPC_class == CLASS_BOBAFETT
				|| self->client->NPC_class == CLASS_MANDO
				|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
				|| self->client->pers.nextbotclass == BCLASS_BOBAFETT
				|| self->client->pers.nextbotclass == BCLASS_MANDOLORIAN1
				|| self->client->pers.nextbotclass == BCLASS_MANDOLORIAN2)
			{
				G_AddEvent(self, EV_JUMP, 0);
			}
			else
			{
				if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
				{
					//short burst
					G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
				}
				else
				{
					//holding it
					G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jump.mp3");
				}
			}
			//ucmd.upmove = 0;
			return EVASION_CARTWHEEL;
		}
		if (!(trace.contents & CONTENTS_BOTCLIP))
		{
			//hit a wall, not a do-not-enter brush
			//FIXME: before we check any of these jump-type evasions, we should check for headroom, right?
			//Okay, see if we can flip *off* the wall and go the other way
			vec3_t idealNormal;

			VectorSubtract(self->r.currentOrigin, traceto, idealNormal);
			VectorNormalize(idealNormal);
			const gentity_t* traceEnt = &g_entities[trace.entityNum];
			if (trace.entityNum < ENTITYNUM_WORLD && traceEnt && traceEnt->s.solid != SOLID_BMODEL || DotProduct(
				trace.plane.normal, idealNormal) > 0.7f)
			{
				//it's a ent of some sort or it's a wall roughly facing us
				float bestCheckDist = 0;
				//hmm, see if we're moving forward
				if (DotProduct(self->client->ps.velocity, fwd) < 200)
				{
					//not running forward very fast
					//check to see if it's okay to move the other way
					if (trace.fraction * checkDist <= 32)
					{
						//wall on that side is close enough to wall-flip off of or wall-run on
						bestCheckDist = checkDist;
						checkDist *= -1.0f;
						VectorMA(self->r.currentOrigin, checkDist, right, traceto);
						//trace in the dir that we want to go
						trap->Trace(&trace, self->r.currentOrigin, mins, maxs, traceto, self->s.number,
							CONTENTS_SOLID | CONTENTS_MONSTERCLIP | CONTENTS_BOTCLIP, qfalse, 0, 0);
						if (trace.fraction >= 1.0f)
						{
							//it's clear, let's do it
							if (self->client->ps.weapon == WP_SABER)
							{
								if (self->client->saber[0].model
									&& self->client->saber[0].model[0]
									&& self->client->saber[0].saberFlags & SFL_NO_WALL_RUNS)
								{
									allowWallFlips = qfalse;
								}
								else if (self->client->saber[1].model
									&& self->client->saber[1].model[0]
									&& self->client->saber[1].saberFlags & SFL_NO_WALL_RUNS)
								{
									allowWallFlips = qfalse;
								}
							}
							if (allowWallFlips)
							{
								//okay to do wall-flips with this saber
								//FIXME: check for drops?
								//turn the cartwheel into a wallflip in the other dir
								if (rightdot > 0)
								{
									anim = BOTH_WALL_FLIP_LEFT;
									self->client->ps.velocity[0] = self->client->ps.velocity[1] = 0;
									VectorMA(self->client->ps.velocity, 150, right, self->client->ps.velocity);
								}
								else
								{
									anim = BOTH_WALL_FLIP_RIGHT;
									self->client->ps.velocity[0] = self->client->ps.velocity[1] = 0;
									VectorMA(self->client->ps.velocity, -150, right, self->client->ps.velocity);
								}
								self->client->ps.velocity[2] = forceJumpStrength[FORCE_LEVEL_2] / 2.25f;
								//animate me
								parts = SETANIM_LEGS;
								if (!self->client->ps.weaponTime)
								{
									parts = SETANIM_BOTH;
								}
								NPC_SetAnim(self, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
								self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];
								//so we don't take damage if we land at same height
								//self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
								if (self->client->NPC_class == CLASS_BOBAFETT
									|| self->client->NPC_class == CLASS_MANDO
									|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
									|| self->client->pers.nextbotclass == BCLASS_BOBAFETT
									|| self->client->pers.nextbotclass == BCLASS_MANDOLORIAN1
									|| self->client->pers.nextbotclass == BCLASS_MANDOLORIAN2)
								{
									G_AddEvent(self, EV_JUMP, 0);
								}
								else
								{
									if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
									{
										//short burst
										G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
									}
									else
									{
										//holding it
										G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jump.mp3");
									}
								}
								return EVASION_OTHER;
							}
						}
						else
						{
							//boxed in on both sides
							if (DotProduct(self->client->ps.velocity, fwd) < 0)
							{
								//moving backwards
								return EVASION_NONE;
							}
							if (trace.fraction * checkDist <= 32 && trace.fraction * checkDist < bestCheckDist)
							{
								bestCheckDist = checkDist;
							}
						}
					}
					else
					{
						//too far from that wall to flip or run off it, check other side
						checkDist *= -1.0f;
						VectorMA(self->r.currentOrigin, checkDist, right, traceto);
						//trace in the dir that we want to go
						trap->Trace(&trace, self->r.currentOrigin, mins, maxs, traceto, self->s.number,
							CONTENTS_SOLID | CONTENTS_MONSTERCLIP | CONTENTS_BOTCLIP, qfalse, 0, 0);
						if (trace.fraction * checkDist <= 32)
						{
							//wall on this side is close enough
							bestCheckDist = checkDist;
						}
						else
						{
							//neither side has a wall within 32
							return EVASION_NONE;
						}
					}
				}
				//Try wall run?
				if (bestCheckDist)
				{
					//one of the walls was close enough to wall-run on
					qboolean allowWallRuns = qtrue;
					if (self->client->ps.weapon == WP_SABER)
					{
						if (self->client->saber[0].model[0]
							&& self->client->saber[0].saberFlags & SFL_NO_WALL_RUNS)
						{
							allowWallRuns = qfalse;
						}
						else if (self->client->saber[1].model[0]
							&& self->client->saber[1].saberFlags & SFL_NO_WALL_RUNS)
						{
							allowWallRuns = qfalse;
						}
					}
					if (allowWallRuns)
					{
						//okay to do wallruns with this saber
						//FIXME: check for long enough wall and a drop at the end?
						if (bestCheckDist > 0)
						{
							//it was to the right
							anim = BOTH_WALL_RUN_RIGHT;
						}
						else
						{
							//it was to the left
							anim = BOTH_WALL_RUN_LEFT;
						}
						self->client->ps.velocity[2] = forceJumpStrength[FORCE_LEVEL_2] / 2.25f;
						//animate me
						parts = SETANIM_LEGS;
						if (!self->client->ps.weaponTime)
						{
							parts = SETANIM_BOTH;
						}
						NPC_SetAnim(self, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];
						//so we don't take damage if we land at same height
						//self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
						if (self->client->NPC_class == CLASS_BOBAFETT
							|| self->client->NPC_class == CLASS_MANDO
							|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
							|| self->client->pers.nextbotclass == BCLASS_BOBAFETT
							|| self->client->pers.nextbotclass == BCLASS_MANDOLORIAN1
							|| self->client->pers.nextbotclass == BCLASS_MANDOLORIAN2)
						{
							G_AddEvent(self, EV_JUMP, 0);
						}
						else
						{
							if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
							{
								//short burst
								G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
							}
							else
							{
								//holding it
								G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jump.mp3");
							}
						}
						return EVASION_OTHER;
					}
				}
				//else check for wall in front, do backflip off wall
			}
		}
	}
	return EVASION_NONE;
}

int Jedi_ReCalcParryTime(const gentity_t* self, const evasionType_t evasion_type)
{
	if (!self->client)
	{
		return 0;
	}
	if (!self->s.number)
	{
		//player blocks how he wants
	}
	else if (self->NPC)
	{
		int base_time;
		if (evasion_type == EVASION_DODGE)
		{
			base_time = self->client->ps.torsoTimer;
		}
		else if (evasion_type == EVASION_CARTWHEEL)
		{
			base_time = self->client->ps.torsoTimer;
		}
		else if (self->client->ps.saberInFlight)
		{
			base_time = Q_irand(1, 3) * 50;
		}
		else
		{
			switch (g_npcspskill.integer)
			{
			case 0:
				base_time = 500;
				break;
			case 1:
				base_time = 250;
				break;
			case 2:
			default:
				base_time = 100;
				break;
			}

			base_time = base_time += 400 * Q_irand(1, 2);

			if (evasion_type == EVASION_DUCK || evasion_type == EVASION_DUCK_PARRY)
			{
				base_time += 100;
			}
			else if (evasion_type == EVASION_JUMP || evasion_type == EVASION_JUMP_PARRY)
			{
				base_time += 50;
			}
			else if (evasion_type == EVASION_OTHER)
			{
				base_time += 100;
			}
			else if (evasion_type == EVASION_FJUMP)
			{
				base_time += 100;
			}
		}
		return base_time;
	}
	return 0;
}

qboolean Jedi_QuickReactions(gentity_t* self)
{
	if (NPC_IsJedi(self))
	{
		return qtrue;
	}
	return qfalse;
}

qboolean Jedi_SaberBusy(const gentity_t* self)
{
	if (self->client->ps.torsoTimer > 300
		&& (PM_SaberInAttack(self->client->ps.saber_move) && self->client->ps.fd.saberAnimLevel == FORCE_LEVEL_3
			|| PM_SpinningSaberAnim(self->client->ps.torsoAnim)
			|| pm_saber_in_special_attack(self->client->ps.torsoAnim)
			|| PM_SaberInBrokenParry(self->client->ps.saber_move)
			|| PM_FlippingAnim(self->client->ps.torsoAnim)
			|| PM_RollingAnim(self->client->ps.torsoAnim)))
	{
		//my saber is not in a parrying position
		return qtrue;
	}
	return qfalse;
}

extern qboolean PM_InRollIgnoreTimer(const playerState_t* ps);
extern qboolean PM_InAirKickingAnim(int anim);
extern qboolean BG_StabDownAnim(int anim);
extern qboolean PM_SuperBreakLoseAnim(int anim);
extern qboolean PM_SuperBreakWinAnim(int anim);

qboolean Jedi_InNoAIAnim(const gentity_t* self)
{
	//Don't block or do any AI processing during these animations.
	if (!self || !self->client)
	{
		//wtf???
		return qtrue;
	}

	if (self->NPC->rank >= RANK_COMMANDER)
	{
		//boss-level guys can multitask, the rest need to chill out during special moves
		return qfalse;
	}

	if (PM_KickingAnim(self->client->ps.legsAnim)
		|| BG_StabDownAnim(self->client->ps.legsAnim)
		|| PM_InAirKickingAnim(self->client->ps.legsAnim)
		|| PM_InRollIgnoreTimer(&self->client->ps)
		|| PM_SaberInKata(self->client->ps.saber_move)
		|| PM_SuperBreakWinAnim(self->client->ps.torsoAnim)
		|| PM_SuperBreakLoseAnim(self->client->ps.torsoAnim))
	{
		return qtrue;
	}

	switch (self->client->ps.legsAnim)
	{
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_FLIP_F:
	case BOTH_FLIP_B:
	case BOTH_FLIP_L:
	case BOTH_FLIP_R:
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
	case BOTH_FORCEWALLRUNFLIP_START:
	case BOTH_JUMPATTACK6:
	case BOTH_GRIEVOUS_LUNGE:
	case BOTH_JUMPATTACK7:
	case BOTH_JUMPFLIPSLASHDOWN1:
	case BOTH_JUMPFLIPSTABDOWN:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_FORCELEAP_PALP:
	case BOTH_ROLL_STAB:
	case BOTH_SPINATTACK6:
	case BOTH_SPINATTACK7:
	case BOTH_SPINATTACKGRIEVOUS:
	case BOTH_PULL_IMPALE_STAB:
	case BOTH_PULL_IMPALE_SWING:
	case BOTH_A6_FB:
	case BOTH_A6_LR:
	case BOTH_A7_HILT:
	case BOTH_SMACK_R:
	case BOTH_SMACK_L:
		return qtrue;
	default:;
	}
	return qfalse;
}

extern qboolean FlyingCreature(const gentity_t* ent);
extern qboolean NAV_DirSafe(const gentity_t* self, vec3_t dir, float dist);
extern qboolean NAV_MoveDirSafe(const gentity_t* self, const usercmd_t* cmd, float distScale);

void Jedi_CheckJumpEvasionSafety(const gentity_t* self, usercmd_t* cmd, const evasionType_t evasion_type)
{
	//checks to make sure that the non-flip evasion are safe to do.
	if (evasion_type != EVASION_OTHER //not a FlipEvasion, which does it's own safety checks
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		//on terra firma right now
		if (self->client->ps.velocity[2] > 0
			|| self->client->ps.fd.forceJumpCharge
			|| cmd->upmove > 0)
		{
			//going to jump
			if (!NAV_MoveDirSafe(self, cmd, self->client->ps.speed * 10.0f))
			{
				//we can't jump in the dir we're pushing in
				//cancel the evasion
				self->client->ps.velocity[2] = self->client->ps.fd.forceJumpCharge = 0;
				cmd->upmove = 0;
				if (d_JediAI.integer || g_DebugSaberCombat.integer)
				{
					Com_Printf(S_COLOR_RED"jump not safe, cancelling!");
				}
			}
			else if (self->client->ps.velocity[0] || self->client->ps.velocity[1])
			{
				//sliding
				vec3_t jumpDir;
				const float jumpDist = VectorNormalize2(self->client->ps.velocity, jumpDir);
				if (!NAV_DirSafe(self, jumpDir, jumpDist))
				{
					//this jump combined with our momentum would send us into a do not enter brush, so cancel it
					//cancel the evasion
					self->client->ps.velocity[2] = self->client->ps.fd.forceJumpCharge = 0;
					cmd->upmove = 0;
					if (d_JediAI.integer || g_DebugSaberCombat.integer)
					{
						Com_Printf(S_COLOR_RED"jump not safe, cancelling!\n");
					}
				}
			}
			if (d_JediAI.integer || g_DebugSaberCombat.integer)
			{
				Com_Printf(S_COLOR_GREEN"jump checked, is safe\n");
			}
		}
	}
}

/*
-------------------------
Jedi_SaberBlock

Pick proper block anim

FIXME: Based on difficulty level/enemy saber combat skill, make this decision-making more/less effective

NOTE: always blocking projectiles in this func!

-------------------------
*/
extern qboolean PM_LockedAnim(int anim);
extern qboolean G_FindClosestPointOnLineSegment(const vec3_t start, const vec3_t end, const vec3_t from, vec3_t result);

evasionType_t jedi_saber_block_go(gentity_t* self, usercmd_t* cmd, vec3_t p_hitloc, vec3_t phit_dir,
	const gentity_t* incoming,
	const float dist) //dist = 0.0f
{
	vec3_t hitloc, hitdir, diff, fwdangles = { 0, 0, 0 }, right;
	int duckChance = 0;
	int dodgeAnim = -1;
	const qboolean alwaysDodgeOrRoll = qfalse;
	qboolean doRoll = qfalse;
	qboolean saberBusy = qfalse, evaded = qfalse, doDodge = qfalse;
	evasionType_t evasionType = EVASION_NONE;

	if (!self || !self->client)
	{
		//sanity check
		return EVASION_NONE;
	}

	if (PM_LockedAnim(self->client->ps.torsoAnim)
		&& self->client->ps.torsoTimer)
	{
		//Never interrupt these...
		return EVASION_NONE;
	}
	if (PM_InSpecialJump(self->client->ps.legsAnim)
		&& pm_saber_in_special_attack(self->client->ps.torsoAnim))
	{
		return EVASION_NONE;
	}

	if (Jedi_InNoAIAnim(self))
	{
		return EVASION_NONE;
	}
	if (!incoming)
	{
		VectorCopy(p_hitloc, hitloc);
		VectorCopy(phit_dir, hitdir);

		if (self->client->ps.saberInFlight)
		{
			//DOH!  do non-saber evasion!
			saberBusy = qtrue;
		}
		else if (Jedi_QuickReactions(self))
		{
			//jedi trainer and tavion are must faster at parrying and can do it whenever they like
		}
		else
		{
			saberBusy = Jedi_SaberBusy(self);
		}
	}
	else
	{
		if (incoming->s.weapon == WP_SABER)
		{
			//flying lightsaber, face it!
		}
		VectorCopy(incoming->r.currentOrigin, hitloc);
		VectorNormalize2(incoming->s.pos.trDelta, hitdir);
	}

	VectorSubtract(hitloc, self->client->renderInfo.eyePoint, diff);
	diff[2] = 0;
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, NULL, right, NULL);

	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];

	//see if we can dodge if need-be
	if (dist > 16 && (Q_irand(0, 2) || saberBusy)
		|| self->client->ps.saberInFlight
		|| BG_SabersOff(&self->client->ps)
		|| self->client->NPC_class == CLASS_BOBAFETT
		|| self->client->pers.nextbotclass == BCLASS_BOBAFETT
		|| self->client->pers.nextbotclass == BCLASS_MANDOLORIAN1
		|| self->client->pers.nextbotclass == BCLASS_MANDOLORIAN2)
	{
		//either it will miss by a bit (and 25% chance) OR our saber is not in-hand OR saber is off
		if (self->NPC && (self->NPC->rank == RANK_CREWMAN || self->NPC->rank >= RANK_LT_JG))
		{
			//acrobat or fencer or above
			if (self->client->ps.groundEntityNum != ENTITYNUM_NONE && //on the ground
				!(self->client->ps.pm_flags & PMF_DUCKED) && cmd->upmove >= 0 && TIMER_Done(self, "duck") //not ducking
				&& !BG_InRoll(&self->client->ps, self->client->ps.legsAnim) //not rolling
				&& !PM_InKnockDown(&self->client->ps) //not knocked down
				&& (self->client->ps.saberInFlight ||
					self->client->NPC_class == CLASS_BOBAFETT
					|| self->client->pers.nextbotclass == BCLASS_BOBAFETT
					|| self->client->pers.nextbotclass == BCLASS_MANDOLORIAN1
					|| self->client->pers.nextbotclass == BCLASS_MANDOLORIAN2 ||
					!PM_SaberInAttack(self->client->ps.saber_move) //not attacking
					&& !PM_SaberInStart(self->client->ps.saber_move) //not starting an attack
					&& !PM_SpinningSaberAnim(self->client->ps.torsoAnim) //not in a saber spin
					&& !pm_saber_in_special_attack(self->client->ps.torsoAnim)))
			{
				//need to check all these because it overrides both torso and legs with the dodge
				doDodge = qtrue;
			}
		}
	}

	if ((self->client->NPC_class == CLASS_BOBAFETT //boba fett
		|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER)
		&& !Q_irand(0, 2))
	{
		doRoll = qtrue;
	}
	// Figure out what quadrant the block was in.
	if (d_JediAI.integer || g_DebugSaberCombat.integer)
	{
		Com_Printf("(%d) evading attack from height %4.2f, zdiff: %4.2f, rightdot: %4.2f\n", level.time,
			hitloc[2] - self->r.absmin[2], zdiff, rightdot);
	}

	if (zdiff >= -5)
	{
		if (incoming || !saberBusy)
		{
			if (rightdot > 12
				|| rightdot > 3 && zdiff < 5
				|| !incoming && fabs(hitdir[2]) < 0.25f) //was normalized, 0.3
			{
				//coming from right
				if (doDodge)
				{
					if (doRoll)
					{
						//roll!
						TIMER_Start(self, "duck", Q_irand(500, 1500));
						TIMER_Start(self, "strafeLeft", Q_irand(500, 1500));
						TIMER_Set(self, "strafeRight", 0);
						evasionType = EVASION_DUCK;
						evaded = qtrue;
					}
					else if (Q_irand(0, 1))
					{
						dodgeAnim = BOTH_DODGE_FL;
					}
					else
					{
						dodgeAnim = BOTH_DODGE_BL;
					}
					PM_AddFatigue(&self->client->ps, FATIGUE_DODGEING);
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
					evasionType = EVASION_PARRY;
					if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{
						if (zdiff > 5)
						{
							TIMER_Start(self, "duck", Q_irand(500, 1500));
							evasionType = EVASION_DUCK_PARRY;
							evaded = qtrue;
							if (d_JediAI.integer || g_DebugSaberCombat.integer)
							{
								Com_Printf("duck ");
							}
						}
						else
						{
							duckChance = 6;
						}
					}
				}
				if (d_JediAI.integer || g_DebugSaberCombat.integer)
				{
					Com_Printf("UR block\n");
				}
			}
			else if (rightdot < -12
				|| rightdot < -3 && zdiff < 5
				|| !incoming && fabs(hitdir[2]) < 0.25f)
			{
				//coming from left
				if (doDodge)
				{
					if (doRoll)
					{
						//roll!
						TIMER_Start(self, "duck", Q_irand(500, 1500));
						TIMER_Start(self, "strafeRight", Q_irand(500, 1500));
						TIMER_Set(self, "strafeLeft", 0);
						evasionType = EVASION_DUCK;
						evaded = qtrue;
					}
					else if (Q_irand(0, 1))
					{
						dodgeAnim = BOTH_DODGE_FR;
					}
					else
					{
						dodgeAnim = BOTH_DODGE_BR;
					}
					PM_AddFatigue(&self->client->ps, FATIGUE_DODGEING);
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
					evasionType = EVASION_PARRY;
					if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{
						if (zdiff > 5)
						{
							TIMER_Start(self, "duck", Q_irand(500, 1500));
							evasionType = EVASION_DUCK_PARRY;
							evaded = qtrue;
							if (d_JediAI.integer || g_DebugSaberCombat.integer)
							{
								Com_Printf("duck ");
							}
						}
						else
						{
							duckChance = 6;
						}
					}
				}
				if (d_JediAI.integer || g_DebugSaberCombat.integer)
				{
					Com_Printf("UL block\n");
				}
			}
			else
			{
				self->client->ps.saberBlocked = BLOCKED_TOP;
				evasionType = EVASION_PARRY;
				if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					duckChance = 4;
				}
				if (d_JediAI.integer || g_DebugSaberCombat.integer)
				{
					Com_Printf("TOP block\n");
				}
			}
			evaded = qtrue;
		}
		else
		{
			if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				TIMER_Start(self, "duck", Q_irand(500, 1500));
				evasionType = EVASION_DUCK;
				evaded = qtrue;
				if (d_JediAI.integer || g_DebugSaberCombat.integer)
				{
					Com_Printf("duck ");
				}
			}
		}
	}
	else if (zdiff > -22) //was-15 )
	{
		if (1)
		{
			//hmm, pretty low, but not low enough to use the low block, so we need to duck
			if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				TIMER_Start(self, "duck", Q_irand(500, 1500));
				evasionType = EVASION_DUCK;
				evaded = qtrue;
				if (d_JediAI.integer || g_DebugSaberCombat.integer)
				{
					Com_Printf("duck ");
				}
			}
			else
			{
				//in air!  Ducking does no good
			}
		}
		if (incoming || !saberBusy)
		{
			if (rightdot > 8 || rightdot > 3 && zdiff < -11) //was normalized, 0.2
			{
				if (doDodge)
				{
					if (doRoll)
					{
						//roll!
						TIMER_Start(self, "strafeLeft", Q_irand(500, 1500));
						TIMER_Set(self, "strafeRight", 0);
					}
					else
					{
						if (self->s.weapon == WP_SABER)
						{
							if (self->health > BLOCKPOINTS_HALF)
							{
								self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
							}
							else
							{
								AngleVectors(self->client->ps.viewangles, hitdir, NULL, NULL);
								self->client->ps.velocity[0] = self->client->ps.velocity[0] * 3;
								self->client->ps.velocity[1] = self->client->ps.velocity[1] * 3;
								self->client->pers.cmd.rightmove = -64;
								ForceDashAnimDash(self);
							}
						}
						else
						{
							dodgeAnim = BOTH_DODGE_L;
							PM_AddFatigue(&self->client->ps, FATIGUE_DODGEING);
						}
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
					if (evasionType == EVASION_DUCK)
					{
						evasionType = EVASION_DUCK_PARRY;
					}
					else
					{
						evasionType = EVASION_PARRY;
					}
				}
				if (d_JediAI.integer || g_DebugSaberCombat.integer)
				{
					Com_Printf("mid-UR block\n");
				}
			}
			else if (rightdot < -8 || rightdot < -3 && zdiff < -11) //was normalized, -0.2
			{
				if (doDodge)
				{
					if (doRoll)
					{
						//roll!
						TIMER_Start(self, "strafeLeft", Q_irand(500, 1500));
						TIMER_Set(self, "strafeRight", 0);
					}
					else
					{
						if (self->s.weapon == WP_SABER)
						{
							if (self->health > BLOCKPOINTS_HALF)
							{
								self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
							}
							else
							{
								AngleVectors(self->client->ps.viewangles, hitdir, NULL, NULL);
								self->client->ps.velocity[0] = self->client->ps.velocity[0] * 3;
								self->client->ps.velocity[1] = self->client->ps.velocity[1] * 3;
								self->client->pers.cmd.rightmove = 64;
								ForceDashAnimDash(self);
							}
						}
						else
						{
							dodgeAnim = BOTH_DODGE_R;
							PM_AddFatigue(&self->client->ps, FATIGUE_DODGEING);
						}
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
					if (evasionType == EVASION_DUCK)
					{
						evasionType = EVASION_DUCK_PARRY;
					}
					else
					{
						evasionType = EVASION_PARRY;
					}
				}
				if (d_JediAI.integer || g_DebugSaberCombat.integer)
				{
					Com_Printf("mid-UL block\n");
				}
			}
			else
			{
				self->client->ps.saberBlocked = BLOCKED_TOP;
				if (evasionType == EVASION_DUCK)
				{
					evasionType = EVASION_DUCK_PARRY;
				}
				else
				{
					evasionType = EVASION_PARRY;
				}
				if (d_JediAI.integer || g_DebugSaberCombat.integer)
				{
					Com_Printf("mid-TOP block\n");
				}
			}
			evaded = qtrue;
		}
	}
	else
	{
		if (saberBusy || zdiff < -36 && (zdiff < -44 || !Q_irand(0, 2)))
		{
			//jump!
			if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				//already in air, duck to pull up legs
				TIMER_Start(self, "duck", Q_irand(500, 1500));
				evasionType = EVASION_DUCK;
				evaded = qtrue;
				if (d_JediAI.integer || g_DebugSaberCombat.integer)
				{
					Com_Printf("legs up\n");
				}
				if (incoming || !saberBusy)
				{
					//since the jump may be cleared if not safe, set a lower block too
					if (rightdot >= 0)
					{
						self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
						evasionType = EVASION_DUCK_PARRY;
						if (d_JediAI.integer || g_DebugSaberCombat.integer)
						{
							Com_Printf("LR block\n");
						}
					}
					else
					{
						self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
						evasionType = EVASION_DUCK_PARRY;
						if (d_JediAI.integer || g_DebugSaberCombat.integer)
						{
							Com_Printf("LL block\n");
						}
					}
					evaded = qtrue;
				}
			}
			else
			{
				//gotta jump!
				if (self->NPC && (self->NPC->rank == RANK_CREWMAN || self->NPC->rank > RANK_LT_JG) &&
					(!Q_irand(0, 10) || !Q_irand(0, 2) && (cmd->forwardmove || cmd->rightmove)))
				{
					//superjump
					if (self->NPC
						&& !(self->NPC->scriptFlags & SCF_NO_ACROBATICS)
						&& self->client->ps.fd.forceRageRecoveryTime < level.time
						&& !(self->client->ps.fd.forcePowersActive & 1 << FP_RAGE)
						&& !PM_InKnockDown(&self->client->ps))
					{
						evasionType = EVASION_PARRY;
					}
				}
				else
				{
					//normal jump
					if (self->NPC
						&& !(self->NPC->scriptFlags & SCF_NO_ACROBATICS)
						&& self->client->ps.fd.forceRageRecoveryTime < level.time
						&& !(self->client->ps.fd.forcePowersActive & 1 << FP_RAGE))
					{
						if ((self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO) && !
							Q_irand(0, 1))
						{
							//flip!
							if (rightdot > 0)
							{
								TIMER_Start(self, "strafeLeft", Q_irand(500, 1500));
								TIMER_Set(self, "strafeRight", 0);
								TIMER_Set(self, "walking", 0);
							}
							else
							{
								TIMER_Start(self, "strafeRight", Q_irand(500, 1500));
								TIMER_Set(self, "strafeLeft", 0);
								TIMER_Set(self, "walking", 0);
							}
						}
						else
						{
							if (self == NPCS.NPC)
							{
								cmd->upmove = 127;
							}
							else
							{
								self->client->ps.velocity[2] = JUMP_VELOCITY;
							}
						}
						evasionType = EVASION_JUMP;
						evaded = qtrue;
						if (d_JediAI.integer || g_DebugSaberCombat.integer)
						{
							Com_Printf("jump + ");
						}
					}
					if (self->client->NPC_class == CLASS_ALORA
						|| self->client->NPC_class == CLASS_SHADOWTROOPER
						|| self->client->NPC_class == CLASS_TAVION)
					{
						if (!incoming
							&& self->client->ps.groundEntityNum < ENTITYNUM_NONE
							&& !Q_irand(0, 2))
						{
							if (!PM_SaberInAttack(self->client->ps.saber_move)
								&& !PM_SaberInStart(self->client->ps.saber_move)
								&& !BG_InRoll(&self->client->ps, self->client->ps.legsAnim)
								&& !PM_InKnockDown(&self->client->ps)
								&& !pm_saber_in_special_attack(self->client->ps.torsoAnim))
							{
								//do the butterfly!
								int butterflyAnim;
								if (self->client->NPC_class == CLASS_ALORA && !Q_irand(0, 2))
								{
									butterflyAnim = BOTH_ALORA_SPIN;
								}
								else if (Q_irand(0, 1))
								{
									butterflyAnim = BOTH_BUTTERFLY_LEFT;
								}
								else
								{
									butterflyAnim = BOTH_BUTTERFLY_RIGHT;
								}
								evasionType = EVASION_CARTWHEEL;
								NPC_SetAnim(self, SETANIM_BOTH, butterflyAnim,
									SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
								self->client->ps.velocity[2] = 225;
								self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];
								//so we don't take damage if we land at same height
								//self->client->ps.SaberActivateTrail( 300 );//FIXME: reset this when done!
								{
									if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
									{
										//short burst
										G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
									}
									else
									{
										//holding it
										G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jump.mp3");
									}
								}
								cmd->upmove = 0;
								saberBusy = qtrue;
								evaded = qtrue;
							}
						}
					}
				}
				if ((evasionType = Jedi_CheckFlipEvasions(self, rightdot, zdiff)) != EVASION_NONE)
				{
					//racc - if the flip is valid
					saberBusy = qtrue;
					evaded = qtrue;
				}
				else if (incoming || !saberBusy)
				{
					//racc - ok, let's try a saber block then.
					//since the jump may be cleared if not safe, set a lower block too
					if (rightdot >= 0)
					{
						self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
						if (evasionType == EVASION_JUMP)
						{
							evasionType = EVASION_JUMP_PARRY;
						}
						else if (evasionType == EVASION_NONE)
						{
							evasionType = EVASION_PARRY;
						}
						if (d_JediAI.integer || g_DebugSaberCombat.integer)
						{
							Com_Printf("LR block\n");
						}
					}
					else
					{
						self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
						if (evasionType == EVASION_JUMP)
						{
							evasionType = EVASION_JUMP_PARRY;
						}
						else if (evasionType == EVASION_NONE)
						{
							evasionType = EVASION_PARRY;
						}
						if (d_JediAI.integer || g_DebugSaberCombat.integer)
						{
							Com_Printf("LL block\n");
						}
					}
					evaded = qtrue;
				}
			}
		}
		else
		{
			if (incoming || !saberBusy)
			{
				if (rightdot >= 0)
				{
					self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
					evasionType = EVASION_PARRY;
					if (d_JediAI.integer || g_DebugSaberCombat.integer)
					{
						Com_Printf("LR block\n");
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
					evasionType = EVASION_PARRY;
					if (d_JediAI.integer || g_DebugSaberCombat.integer)
					{
						Com_Printf("LL block\n");
					}
				}
				if (incoming && incoming->s.weapon == WP_SABER)
				{
					//thrown saber!
					if (self->NPC && (self->NPC->rank == RANK_CREWMAN || self->NPC->rank > RANK_LT_JG) &&
						(!Q_irand(0, 10) || !Q_irand(0, 2) && (cmd->forwardmove || cmd->rightmove)))
					{
						//superjump
						//FIXME: check the jump, if can't, then block
						if (self->NPC
							&& !(self->NPC->scriptFlags & SCF_NO_ACROBATICS)
							&& self->client->ps.fd.forceRageRecoveryTime < level.time
							&& !(self->client->ps.fd.forcePowersActive & 1 << FP_RAGE)
							&& !PM_InKnockDown(&self->client->ps))
						{
							evasionType = EVASION_PARRY;
						}
					}
					else
					{
						//normal jump
						if (self->NPC
							&& !(self->NPC->scriptFlags & SCF_NO_ACROBATICS)
							&& self->client->ps.fd.forceRageRecoveryTime < level.time
							&& !(self->client->ps.fd.forcePowersActive & 1 << FP_RAGE))
						{
							if (self == NPCS.NPC)
							{
								cmd->upmove = 127;
							}
							else
							{
								self->client->ps.velocity[2] = JUMP_VELOCITY;
							}
							evasionType = EVASION_JUMP_PARRY;
							if (d_JediAI.integer || g_DebugSaberCombat.integer)
							{
								Com_Printf("jump + ");
							}
						}
					}
				}
				evaded = qtrue;
			}
		}
	}
	if (evasionType == EVASION_NONE)
	{
		return EVASION_NONE;
	}
	//=======================================================================================
	//see if it's okay to jump
	Jedi_CheckJumpEvasionSafety(self, cmd, evasionType);
	//=======================================================================================
	//stop taunting
	TIMER_Set(self, "taunting", 0);
	//stop gripping
	TIMER_Set(self, "gripping", -level.time);
	WP_ForcePowerStop(self, FP_GRIP);
	//stop draining
	TIMER_Set(self, "draining", -level.time);
	WP_ForcePowerStop(self, FP_DRAIN);

	if (dodgeAnim != -1)
	{
		//dodged
		evasionType = EVASION_DODGE;
		NPC_SetAnim(self, SETANIM_BOTH, dodgeAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		self->client->ps.weaponTime = self->client->ps.torsoTimer;
		//force them to stop moving in this case
		self->client->ps.pm_time = self->client->ps.torsoTimer;
		//FIXME: maybe make a sound?  Like a grunt?  EV_JUMP?
		self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		//dodged, not block
	}
	else
	{
		if (duckChance)
		{
			if (!Q_irand(0, duckChance))
			{
				TIMER_Start(self, "duck", Q_irand(500, 1500));
				if (evasionType == EVASION_PARRY)
				{
					evasionType = EVASION_DUCK_PARRY;
				}
				else
				{
					evasionType = EVASION_DUCK;
				}
			}
		}

		if (incoming)
		{
			self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	const int parryReCalcTime = Jedi_ReCalcParryTime(self, evasionType);
	if (self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
	{
		self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
	}
	return evasionType;
}

extern float wp_saber_length(const gentity_t* ent);

static evasionType_t Jedi_CheckEvadeSpecialAttacks(void)
{
	//evade special attacks
	if (!NPCS.NPC
		|| !NPCS.NPC->client)
	{
		return EVASION_NONE;
	}

	if (!NPCS.NPC->enemy
		|| NPCS.NPC->enemy->health <= 0
		|| !NPCS.NPC->enemy->client)
	{
		//don't keep blocking him once he's dead (or if not a client)
		return EVASION_NONE;
	}

	if (NPCS.NPC->enemy->s.number >= MAX_CLIENTS)
	{
		//only do these against player
		return EVASION_NONE;
	}

	if (!TIMER_Done(NPCS.NPC, "specialEvasion"))
	{
		//still evading from last time
		return EVASION_NONE;
	}

	if (NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK6
		|| NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK7
		|| NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACKGRIEVOUS)
	{
		//back away from these
		if (NPCS.NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER
			|| NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER
			|| NPCS.NPC->client->NPC_class == CLASS_ALORA
			|| Q_irand(0, NPCS.NPCInfo->rank) > RANK_LT_JG)
		{
			//see if we should back off
			if (in_front(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->enemy->r.currentAngles,
				0.0f))
			{
				//facing me
				float minSafeDistSq = NPCS.NPC->r.maxs[0] * 1.5f + NPCS.NPC->enemy->r.maxs[0] * 1.5f + wp_saber_length(
					NPCS.NPC->enemy) + 24.0f;
				minSafeDistSq *= minSafeDistSq;
				if (DistanceSquared(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin) < minSafeDistSq)
				{
					//back off!
					Jedi_StartBackOff();
					return EVASION_OTHER;
				}
			}
		}
	}
	else
	{
		//check some other attacks?
		//check roll-stab
		if (NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_STAB
			|| NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_F && (NPCS.NPCInfo->last_ucmd.buttons &
				BUTTON_ATTACK
				|| NPCS.NPC->enemy->client->ps.pm_flags & PMF_ATTACK_HELD))
		{
			//either already in a roll-stab or may go into one
			if (NPCS.NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER
				|| NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER
				|| NPCS.NPC->client->NPC_class == CLASS_ALORA
				|| Q_irand(-3, NPCS.NPCInfo->rank) > RANK_LT_JG)
			{
				//see if we should evade
				vec3_t yawOnlyAngles;
				VectorSet(yawOnlyAngles, 0, NPCS.NPC->enemy->r.currentAngles[YAW], 0);
				if (in_front(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin, yawOnlyAngles, 0.25f))
				{
					float minSafeDistSq = NPCS.NPC->r.maxs[0] * 1.5f + NPCS.NPC->enemy->r.maxs[0] * 1.5f +
						wp_saber_length(NPCS.NPC->enemy) + 24.0f;
					minSafeDistSq *= minSafeDistSq;
					const float distSq = DistanceSquared(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin);
					if (distSq < minSafeDistSq)
					{
						//evade!
						qboolean doJump = NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_STAB || distSq < 3000.0f;
						//not much time left, just jump!
						if (NPCS.NPCInfo->scriptFlags & SCF_NO_ACROBATICS
							|| !doJump)
						{
							//roll?
							vec3_t enemyRight, dir2Me;

							AngleVectors(yawOnlyAngles, NULL, enemyRight, NULL);
							VectorSubtract(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin, dir2Me);
							VectorNormalize(dir2Me);
							const float dot = DotProduct(enemyRight, dir2Me);

							NPCS.ucmd.forwardmove = 0;
							TIMER_Start(NPCS.NPC, "duck", Q_irand(500, 1500));
							NPCS.ucmd.upmove = -127;
							//NOTE: this *assumes* I'm facing him!
							if (dot > 0)
							{
								//I'm to his right
								if (!NPC_MoveDirClear(0, -127, qfalse))
								{
									//fuck, jump instead
									doJump = qtrue;
								}
								else
								{
									//roll to the left.
									TIMER_Start(NPCS.NPC, "strafeLeft", Q_irand(500, 1500));
									TIMER_Set(NPCS.NPC, "strafeRight", 0);
									NPCS.ucmd.rightmove = -127;
									if (d_JediAI.integer || g_DebugSaberCombat.integer)
									{
										Com_Printf("%s rolling left from roll-stab!\n", NPCS.NPC->NPC_type);
									}
									if (NPCS.NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
									{
										//fuck it, just force it
										NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_ROLL_L,
											SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
										G_AddEvent(NPCS.NPC, EV_ROLL, 0);
										NPCS.NPC->client->ps.saber_move = LS_NONE;
									}
								}
							}
							else
							{
								//I'm to his left
								if (!NPC_MoveDirClear(0, 127, qfalse))
								{
									//fuck, jump instead
									doJump = qtrue;
								}
								else
								{
									//roll to the right
									TIMER_Start(NPCS.NPC, "strafeRight", Q_irand(500, 1500));
									TIMER_Set(NPCS.NPC, "strafeLeft", 0);
									NPCS.ucmd.rightmove = 127;
									if (d_JediAI.integer || g_DebugSaberCombat.integer)
									{
										Com_Printf("%s rolling right from roll-stab!\n", NPCS.NPC->NPC_type);
									}
									if (NPCS.NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
									{
										//fuck it, just force it
										NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_ROLL_R,
											SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
										G_AddEvent(NPCS.NPC, EV_ROLL, 0);
										NPCS.NPC->client->ps.saber_move = LS_NONE;
									}
								}
							}
							if (!doJump)
							{
								TIMER_Set(NPCS.NPC, "specialEvasion", 3000);
								return EVASION_DUCK;
							}
						}
						//didn't roll, do jump
						if (NPCS.NPC->s.weapon != WP_SABER
							|| NPCS.NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER
							|| NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER
							|| NPCS.NPC->client->NPC_class == CLASS_ALORA
							|| Q_irand(-3, NPCS.NPCInfo->rank) > RANK_CREWMAN)
						{
							//superjump
							NPCS.NPC->client->ps.fd.forceJumpCharge = 320; //FIXME: calc this intelligently
							if (Q_irand(0, 2))
							{
								//make it a backflip
								NPCS.ucmd.forwardmove = -127;
								TIMER_Set(NPCS.NPC, "roamTime", -level.time);
								TIMER_Set(NPCS.NPC, "strafeLeft", -level.time);
								TIMER_Set(NPCS.NPC, "strafeRight", -level.time);
								TIMER_Set(NPCS.NPC, "walking", -level.time);
								TIMER_Set(NPCS.NPC, "moveforward", -level.time);
								TIMER_Set(NPCS.NPC, "movenone", -level.time);
								TIMER_Set(NPCS.NPC, "moveright", -level.time);
								TIMER_Set(NPCS.NPC, "moveleft", -level.time);
								TIMER_Set(NPCS.NPC, "movecenter", -level.time);
								TIMER_Set(NPCS.NPC, "moveback", Q_irand(500, 1000));
								if (d_JediAI.integer || g_DebugSaberCombat.integer)
								{
									Com_Printf("%s backflipping from roll-stab!\n", NPCS.NPC->NPC_type);
								}
							}
							else
							{
								if (d_JediAI.integer || g_DebugSaberCombat.integer)
								{
									Com_Printf("%s force-jumping over roll-stab!\n", NPCS.NPC->NPC_type);
								}
							}
							TIMER_Set(NPCS.NPC, "specialEvasion", 3000);
							return EVASION_FJUMP;
						}
						//normal jump
						NPCS.ucmd.upmove = 127;
						if (d_JediAI.integer || g_DebugSaberCombat.integer)
						{
							Com_Printf("%s jumping over roll-stab!\n", NPCS.NPC->NPC_type);
						}
						TIMER_Set(NPCS.NPC, "specialEvasion", 2000);
						return EVASION_JUMP;
					}
				}
			}
		}
	}
	return EVASION_NONE;
}

extern float ShortestLineSegBewteen2LineSegs(vec3_t start1, vec3_t end1, vec3_t start2, vec3_t end2, vec3_t close_pnt1,
	vec3_t close_pnt2);
extern int wp_debug_saber_colour(saber_colors_t saber_color);

static qboolean Jedi_SaberBlock(void)
{
	vec3_t hitloc, saberTipOld, saberTip, top, bottom, axisPoint, saberPoint, dir; //saberBase,
	vec3_t pointDir, baseDir, tipDir, saberMins, saberMaxs;
	float baseDirPerc, dist;
	float bestDist = Q3_INFINITE;
	int closestsaber_num = 0, closestblade_num = 0;
	trace_t tr;
	evasionType_t evasionType;

	if (!TIMER_Done(NPCS.NPC, "parryReCalcTime"))
	{
		//can't do our own re-think of which parry to use yet
		return qfalse;
	}

	if (NPCS.NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] > level.time)
	{
		//can't move the saber to another position yet
		return qfalse;
	}

	if (NPCS.NPC->enemy->health <= 0 || !NPCS.NPC->enemy->client)
	{
		//don't keep blocking him once he's dead (or if not a client)
		return qfalse;
	}

	if (Distance(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin) > 64
		|| !(NPCS.NPC->enemy->client->pers.cmd.buttons & BUTTON_ATTACK))
	{
		// : They were doing evasion WAY too far away... And when the enemy isn't even attacking...
		return qfalse;
	}

	VectorSet(saberMins, -4, -4, -4);
	VectorSet(saberMaxs, 4, 4, 4);

	for (int saberNum = 0; saberNum < MAX_SABERS; saberNum++)
	{
		for (int blade_num = 0; blade_num < NPCS.NPC->enemy->client->saber[saberNum].numBlades; blade_num++)
		{
			if (NPCS.NPC->enemy->client->saber[saberNum].type != SABER_NONE
				&& NPCS.NPC->enemy->client->saber[saberNum].blade[blade_num].length > 0)
			{
				//valid saber and this blade is on
				VectorMA(NPCS.NPC->enemy->client->saber[saberNum].blade[blade_num].muzzlePointOld,
					NPCS.NPC->enemy->client->saber[saberNum].blade[blade_num].length,
					NPCS.NPC->enemy->client->saber[saberNum].blade[blade_num].muzzleDirOld, saberTipOld);
				VectorMA(NPCS.NPC->enemy->client->saber[saberNum].blade[blade_num].muzzlePoint,
					NPCS.NPC->enemy->client->saber[saberNum].blade[blade_num].length,
					NPCS.NPC->enemy->client->saber[saberNum].blade[blade_num].muzzleDir, saberTip);

				VectorCopy(NPCS.NPC->r.currentOrigin, top);
				top[2] = NPCS.NPC->r.absmax[2];
				VectorCopy(NPCS.NPC->r.currentOrigin, bottom);
				bottom[2] = NPCS.NPC->r.absmin[2];

				dist = ShortestLineSegBewteen2LineSegs(
					NPCS.NPC->enemy->client->saber[saberNum].blade[blade_num].muzzlePoint, saberTip, bottom, top,
					saberPoint, axisPoint);
				if (dist < bestDist)
				{
					bestDist = dist;
					closestsaber_num = saberNum;
					closestblade_num = blade_num;
				}
			}
		}
	}

	if (bestDist > NPCS.NPC->r.maxs[0] * 5)
	{
		//sometimes he reacts when you're too far away to actually hit him
		if (d_JediAI.integer || g_DebugSaberCombat.integer)
		{
			Com_Printf(S_COLOR_RED"enemy saber dist: %4.2f\n", bestDist);
		}
		TIMER_Set(NPCS.NPC, "parryTime", -1);
		return qfalse;
	}

	dist = bestDist;

	if (d_JediAI.integer || g_DebugSaberCombat.integer)
	{
		Com_Printf(S_COLOR_GREEN"enemy saber dist: %4.2f\n", dist);
	}

	//now use the closest blade for my evasion check
	VectorMA(NPCS.NPC->enemy->client->saber[closestsaber_num].blade[closestblade_num].muzzlePointOld,
		NPCS.NPC->enemy->client->saber[closestsaber_num].blade[closestblade_num].length,
		NPCS.NPC->enemy->client->saber[closestsaber_num].blade[closestblade_num].muzzleDirOld, saberTipOld);
	VectorMA(NPCS.NPC->enemy->client->saber[closestsaber_num].blade[closestblade_num].muzzlePoint,
		NPCS.NPC->enemy->client->saber[closestsaber_num].blade[closestblade_num].length,
		NPCS.NPC->enemy->client->saber[closestsaber_num].blade[closestblade_num].muzzleDir, saberTip);

	VectorCopy(NPCS.NPC->r.currentOrigin, top);
	top[2] = NPCS.NPC->r.absmax[2];
	VectorCopy(NPCS.NPC->r.currentOrigin, bottom);
	bottom[2] = NPCS.NPC->r.absmin[2];

	dist = ShortestLineSegBewteen2LineSegs(
		NPCS.NPC->enemy->client->saber[closestsaber_num].blade[closestblade_num].muzzlePoint, saberTip, bottom, top,
		saberPoint, axisPoint);
	VectorSubtract(saberPoint, NPCS.NPC->enemy->client->saber[closestsaber_num].blade[closestblade_num].muzzlePoint,
		pointDir);
	const float pointDist = VectorLength(pointDir);

	if (NPCS.NPC->enemy->client->saber[closestsaber_num].blade[closestblade_num].length <= 0)
	{
		baseDirPerc = 0.5f;
	}
	else
	{
		baseDirPerc = pointDist / NPCS.NPC->enemy->client->saber[closestsaber_num].blade[closestblade_num].length;
	}

	VectorSubtract(NPCS.NPC->enemy->client->saber[closestsaber_num].blade[closestblade_num].muzzlePoint,
		NPCS.NPC->enemy->client->saber[closestsaber_num].blade[closestblade_num].muzzlePointOld, baseDir);
	VectorSubtract(saberTip, saberTipOld, tipDir);
	VectorScale(baseDir, baseDirPerc, baseDir);
	VectorMA(baseDir, 1.0f - baseDirPerc, tipDir, dir);
	VectorMA(saberPoint, 200, dir, hitloc);

	//get the actual point of impact
	trap->Trace(&tr, saberPoint, saberMins, saberMaxs, hitloc, NPCS.NPC->enemy->s.number, CONTENTS_BODY, qfalse, 0, 0);
	//, G2_RETURNONHIT, 10 );
	if (tr.allsolid || tr.startsolid || tr.fraction >= 1.0f)
	{
		vec3_t saberHitPoint;
		//estimate
		vec3_t dir2Me;
		VectorSubtract(axisPoint, saberPoint, dir2Me);
		dist = VectorNormalize(dir2Me);
		if (DotProduct(dir, dir2Me) < 0.2f)
		{
			//saber is not swinging in my direction
			TIMER_Set(NPCS.NPC, "parryTime", -1);
			return qfalse;
		}
		ShortestLineSegBewteen2LineSegs(saberPoint, hitloc, bottom, top, saberHitPoint, hitloc);
	}
	else
	{
		VectorCopy(tr.endpos, hitloc);
	}

	if (d_JediAI.integer || g_DebugSaberCombat.integer)
	{
		G_TestLine(saberPoint, hitloc, 0x0000ff, FRAMETIME);
	}

	if ((evasionType = jedi_saber_block_go(NPCS.NPC, &NPCS.ucmd, hitloc, dir, NULL, dist)) != EVASION_NONE)
	{
		//did some sort of evasion
		if (evasionType != EVASION_DODGE)
		{
			if (!NPCS.NPC->client->ps.saberInFlight)
			{
				//make sure saber is on
				WP_ActivateSaber(NPCS.NPC);
			}

			//debounce our parry recalc time
			const int parryReCalcTime = Jedi_ReCalcParryTime(NPCS.NPC, evasionType);
			TIMER_Set(NPCS.NPC, "parryReCalcTime", Q_irand(0, parryReCalcTime));

			//determine how long to hold this anim
			if (TIMER_Done(NPCS.NPC, "parryTime"))
			{
				TIMER_Set(NPCS.NPC, "parryTime", Q_irand(parryReCalcTime / 2, parryReCalcTime * 1.5));
			}
		}
		else
		{
			//dodged.
			int dodgeTime = NPCS.NPC->client->ps.torsoTimer;
			if (NPCS.NPCInfo->rank > RANK_LT_COMM && NPCS.NPC->client->NPC_class != CLASS_DESANN)
			{
				//higher-level guys can dodge faster
				dodgeTime -= 200;
			}
			TIMER_Set(NPCS.NPC, "parryReCalcTime", dodgeTime);
			TIMER_Set(NPCS.NPC, "parryTime", dodgeTime);
		}
	}
	if (evasionType != EVASION_DUCK_PARRY
		&& evasionType != EVASION_JUMP_PARRY
		&& evasionType != EVASION_JUMP
		&& evasionType != EVASION_DUCK
		&& evasionType != EVASION_FJUMP)
	{
		if (Jedi_CheckEvadeSpecialAttacks() != EVASION_NONE)
		{
			//got a new evasion!
			//see if it's okay to jump
			Jedi_CheckJumpEvasionSafety(NPCS.NPC, &NPCS.ucmd, evasionType);
		}
	}
	return qtrue;
}

extern qboolean NPC_CheckFallPositionOK(const gentity_t* NPC, vec3_t position);

qboolean Jedi_EvasionRoll(gentity_t* ai_ent)
{
	if (!ai_ent->enemy->client)
	{
		ai_ent->npc_roll_start = qfalse;
		return qfalse;
	}
	if (ai_ent->enemy->client
		&& ai_ent->enemy->s.weapon == WP_SABER
		&& ai_ent->enemy->client->ps.saberLockTime > level.time)
	{
		//don't try to block/evade an enemy who is in a saberLock
		ai_ent->npc_roll_start = qfalse;
		return qfalse;
	}
	if (ai_ent->client->ps.saberEventFlags & SEF_LOCK_WON && ai_ent->enemy->painDebounceTime > level.time)
	{
		//pressing the advantage of winning a saber lock
		ai_ent->npc_roll_start = qfalse;
		return qfalse;
	}

	if (ai_ent->npc_roll_time >= level.time)
	{
		// Already in a roll...
		ai_ent->npc_roll_start = qfalse;
		return qfalse;
	}

	// Init...
	ai_ent->npc_roll_direction = EVASION_ROLL_DIR_NONE;
	ai_ent->npc_roll_start = qfalse;

	qboolean canRollBack = qfalse;
	qboolean canRollLeft = qfalse;
	qboolean canRollRight = qfalse;

	trace_t tr;
	vec3_t fwd, right, up, start, end;
	AngleVectors(ai_ent->r.currentAngles, fwd, right, up);

	VectorSet(start, ai_ent->r.currentOrigin[0], ai_ent->r.currentOrigin[1], ai_ent->r.currentOrigin[2] + 24.0);
	VectorMA(start, -128, fwd, end);
	trap->Trace(&tr, start, NULL, NULL, end, ai_ent->s.number, MASK_NPCSOLID, qfalse, 0, 0);

	if (tr.fraction == 1.0 && !NPC_CheckFallPositionOK(ai_ent, end))
	{
		// We can roll back...
		canRollBack = qtrue;
	}

	VectorMA(start, -128, right, end);
	trap->Trace(&tr, start, NULL, NULL, end, ai_ent->s.number, MASK_NPCSOLID, qfalse, 0, 0);

	if (tr.fraction == 1.0 && !NPC_CheckFallPositionOK(ai_ent, end))
	{
		// We can roll back...
		canRollLeft = qtrue;
	}

	VectorMA(start, 128, right, end);
	trap->Trace(&tr, start, NULL, NULL, end, ai_ent->s.number, MASK_NPCSOLID, qfalse, 0, 0);

	if (tr.fraction == 1.0 && !NPC_CheckFallPositionOK(ai_ent, end))
	{
		// We can roll back...
		canRollRight = qtrue;
	}

	if (canRollBack && canRollLeft && canRollRight)
	{
		const int choice = Q_irand(0, 2);

		switch (choice)
		{
		case 2:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_RIGHT;
			break;
		case 1:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_LEFT;
			break;
		default:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_BACK;
			break;
		}

		return qtrue;
	}
	if (canRollBack && canRollLeft)
	{
		const int choice = Q_irand(0, 1);

		switch (choice)
		{
		case 1:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_LEFT;
			break;
		default:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_BACK;
			break;
		}

		return qtrue;
	}
	if (canRollBack && canRollRight)
	{
		const int choice = Q_irand(0, 1);

		switch (choice)
		{
		case 1:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_RIGHT;
			break;
		default:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_BACK;
			break;
		}

		return qtrue;
	}
	if (canRollLeft && canRollRight)
	{
		const int choice = Q_irand(0, 1);

		switch (choice)
		{
		case 1:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_RIGHT;
			break;
		default:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_LEFT;
			break;
		}

		return qtrue;
	}
	if (canRollLeft)
	{
		ai_ent->npc_roll_time = level.time + 5000;
		ai_ent->npc_roll_start = qtrue;
		ai_ent->npc_roll_direction = EVASION_ROLL_DIR_LEFT;
		return qtrue;
	}
	if (canRollRight)
	{
		ai_ent->npc_roll_time = level.time + 5000;
		ai_ent->npc_roll_start = qtrue;
		ai_ent->npc_roll_direction = EVASION_ROLL_DIR_RIGHT;
		return qtrue;
	}
	if (canRollBack)
	{
		ai_ent->npc_roll_time = level.time + 5000;
		ai_ent->npc_roll_start = qtrue;
		ai_ent->npc_roll_direction = EVASION_ROLL_DIR_BACK;
		return qtrue;
	}

	return qfalse;
}

/*
-------------------------
Jedi_EvasionSaber

defend if other is using saber and attacking me!
-------------------------
*/
static void Jedi_EvasionSaber(vec3_t enemy_movedir, const float enemy_dist, vec3_t enemy_dir)
{
	vec3_t dirEnemy2Me;
	int evasionChance = 30; //only step aside 30% if he's moving at me but not attacking
	qboolean enemy_attacking = qfalse;
	qboolean throwing_saber = qfalse;
	qboolean shooting_lightning = qfalse;

	if (!NPCS.NPC->enemy->client)
	{
		return;
	}
	if (NPCS.NPC->enemy->client
		&& NPCS.NPC->enemy->s.weapon == WP_SABER
		&& NPCS.NPC->enemy->client->ps.saberLockTime > level.time)
	{
		//don't try to block/evade an enemy who is in a saberLock
		return;
	}
	if (NPCS.NPC->client->ps.saberEventFlags & SEF_LOCK_WON && NPCS.NPC->enemy->painDebounceTime > level.time)
	{
		//pressing the advantage of winning a saber lock
		return;
	}

	if (NPCS.NPC->enemy->client->ps.saberInFlight && !TIMER_Done(NPCS.NPC, "taunting"))
	{
		//if he's throwing his saber, stop taunting
		TIMER_Set(NPCS.NPC, "taunting", -level.time);
		if (!NPCS.NPC->client->ps.saberInFlight)
		{
			WP_ActivateSaber(NPCS.NPC);
		}
	}

	if (TIMER_Done(NPCS.NPC, "parryTime"))
	{
		if (NPCS.NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
			NPCS.NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN)
		{
			//wasn't blocked myself
			NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}

	if (NPCS.NPC->enemy->client->ps.weaponTime && NPCS.NPC->enemy->client->ps.weaponstate == WEAPON_FIRING)
	{
		if ((!NPCS.NPC->client->ps.saberInFlight || //their saber isn't in the air
			NPCS.NPC->client->saber[1].model && NPCS.NPC->client->saber[1].model[0]
			&& NPCS.NPC->client->ps.saberHolstered != 2) //or they still have their second saber
			&& Jedi_SaberBlock())
		{
			//blocked/evaded
			return;
		}
	}
	else if (Jedi_CheckEvadeSpecialAttacks() != EVASION_NONE)
	{
		//succeeded at evading a special attack
		return;
	}

	VectorSubtract(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin, dirEnemy2Me);
	VectorNormalize(dirEnemy2Me);

	if (NPCS.NPC->enemy->client->ps.weaponTime && NPCS.NPC->enemy->client->ps.weaponstate == WEAPON_FIRING)
	{
		//enemy is attacking
		enemy_attacking = qtrue;
		evasionChance = 90;
	}

	if (NPCS.NPC->enemy->client->ps.fd.forcePowersActive & 1 << FP_LIGHTNING)
	{
		//enemy is shooting lightning
		enemy_attacking = qtrue;
		shooting_lightning = qtrue;
		evasionChance = 80;
	}

	if (NPCS.NPC->enemy->client->ps.saberInFlight
		&& NPCS.NPC->enemy->client->ps.saberEntityNum != ENTITYNUM_NONE
		&& NPCS.NPC->enemy->client->ps.saberEntityState != SES_RETURNING)
	{
		//enemy is shooting lightning
		enemy_attacking = qtrue;
		throwing_saber = qtrue;
	}

	if (Q_irand(0, 100) < evasionChance)
	{
		//check to see if he's coming at me
		float facingAmt;
		if (VectorCompare(enemy_movedir, vec3_origin) || shooting_lightning || throwing_saber)
		{
			//he's not moving (or he's using a ranged attack), see if he's facing me
			vec3_t enemy_fwd;
			AngleVectors(NPCS.NPC->enemy->client->ps.viewangles, enemy_fwd, NULL, NULL);
			facingAmt = DotProduct(enemy_fwd, dirEnemy2Me);
		}
		else
		{
			//he's moving
			facingAmt = DotProduct(enemy_movedir, dirEnemy2Me);
		}

		if (flrand(0.25, 1) < facingAmt)
		{
			//coming at/facing me!
			int whichDefense = 0;
			if (NPCS.NPC->client->ps.weaponTime || NPCS.NPC->client->ps.saberInFlight || NPCS.NPC->client->NPC_class ==
				CLASS_BOBAFETT
				|| NPCS.NPC->client->pers.nextbotclass == BCLASS_BOBAFETT
				|| NPCS.NPC->client->NPC_class == CLASS_REBORN && NPCS.NPC->s.weapon != WP_SABER
				|| NPCS.NPC->client->NPC_class == CLASS_ROCKETTROOPER
				|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN1
				|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN2)
			{
				//I'm attacking or recovering from a parry, can only try to strafe/jump right now
				if (Q_irand(0, 10) < NPCS.NPCInfo->stats.aggression)
				{
					return;
				}
				whichDefense = 100;
			}
			else
			{
				if (shooting_lightning)
				{
					//check for lightning attack
					//only valid defense is strafe and/or jump
					whichDefense = 100;
					if (NPCS.NPC->s.weapon == WP_SABER)
					{
						Jedi_SaberBlock();
					}
					else
					{
						//already chose one
					}
				}
				else if (throwing_saber)
				{
					vec3_t saberDir2Me;
					vec3_t saberMoveDir;
					const gentity_t* saber = &g_entities[NPCS.NPC->enemy->client->ps.saberEntityNum];
					VectorSubtract(NPCS.NPC->r.currentOrigin, saber->r.currentOrigin, saberDir2Me);
					const float saberDist = VectorNormalize(saberDir2Me);
					VectorCopy(saber->s.pos.trDelta, saberMoveDir);
					VectorNormalize(saberMoveDir);
					if (!Q_irand(0, 3))
					{
						//Com_Printf( "(%d) raise agg - enemy threw saber\n", level.time );
						Jedi_Aggression(NPCS.NPC, 1);
					}
					if (DotProduct(saberMoveDir, saberDir2Me) > 0.5)
					{
						//it's heading towards me
						if (saberDist < 100)
						{
							//it's close
							whichDefense = Q_irand(3, 6);
						}
						else if (saberDist < 200)
						{
							//got some time, yet, try pushing
							whichDefense = Q_irand(0, 8);
						}
					}
				}
				if (whichDefense)
				{
					if (NPCS.NPC->s.weapon == WP_SABER)
					{
						Jedi_SaberBlock();
					}
					else
					{
						//already chose one
					}
				}
				else if (enemy_dist > 80 || !enemy_attacking)
				{
					//he's pretty far, or not swinging, just strafe
					if (VectorCompare(enemy_movedir, vec3_origin))
					{
						//if he's not moving, not swinging and far enough away, no evasion necc.
						return;
					}
					if (Q_irand(0, 10) < NPCS.NPCInfo->stats.aggression)
					{
						return;
					}
					whichDefense = 100;
				}
				else
				{
					//he's getting close and swinging at me
					vec3_t fwd;
					//see if I'm facing him
					AngleVectors(NPCS.NPC->client->ps.viewangles, fwd, NULL, NULL);
					if (DotProduct(enemy_dir, fwd) < 0.5)
					{
						//I'm not really facing him, best option is to strafe
						whichDefense = Q_irand(5, 16);
					}
					else if (enemy_dist < 56)
					{
						//he's very close, maybe we should be more inclined to block or throw
						whichDefense = Q_irand(Q_min(NPCS.NPCInfo->stats.aggression, 24), 24);
					}
					else
					{
						whichDefense = Q_irand(2, 16);
					}
				}
			}

			if (whichDefense >= 4 && whichDefense <= 12)
			{
				//would try to block
				if (NPCS.NPC->client->ps.saberInFlight)
				{
					//can't, saber in not in hand, so fall back to strafe/jump
					whichDefense = 100;
				}
			}

			switch (whichDefense)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				//use jedi force push? or kick?
				if (Jedi_DecideKick() //let's try a kick
					&& (G_PickAutoMultiKick(NPCS.NPC, qfalse, qtrue) != LS_NONE
						|| G_CanKickEntity(NPCS.NPC, NPCS.NPC->enemy)
						&& G_PickAutoKick(NPCS.NPC, NPCS.NPC->enemy) != LS_NONE))
				{
					//kicked
					TIMER_Set(NPCS.NPC, "kickDebounce", Q_irand(3000, 10000));
				}
				else if ((NPCS.NPCInfo->rank == RANK_ENSIGN || NPCS.NPCInfo->rank > RANK_LT_JG) && TIMER_Done(
					NPCS.NPC, "parryTime"))
				{
					ForceThrow(NPCS.NPC, qfalse);
				}
				else if (!Jedi_SaberBlock())
				{
					Jedi_EvasionRoll(NPCS.NPC);
				}
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
				Jedi_SaberBlock();
				break;
			default:
				//Evade!
				if (!Q_irand(0, 5) || !jedi_strafe(300, 1000, 0, 1000, qfalse))
				{
					if (Jedi_DecideKick()
						&& G_CanKickEntity(NPCS.NPC, NPCS.NPC->enemy)
						&& G_PickAutoKick(NPCS.NPC, NPCS.NPC->enemy) != LS_NONE)
					{
						//kicked!
						TIMER_Set(NPCS.NPC, "kickDebounce", Q_irand(3000, 10000));
					}
					else if (shooting_lightning || throwing_saber || enemy_dist < 80)
					{
						//force-jump+forward - jump over the guy!
						if (shooting_lightning || !Q_irand(0, 2) && NPCS.NPCInfo->stats.aggression < 4 && TIMER_Done(
							NPCS.NPC, "parryTime"))
						{
							if ((NPCS.NPCInfo->rank == RANK_ENSIGN || NPCS.NPCInfo->rank > RANK_LT_JG) && !
								shooting_lightning && Q_irand(0, 2))
							{
								//FIXME: check forcePushRadius[NPC->client->ps.fd.forcePowerLevel[FP_PUSH]]
								ForceThrow(NPCS.NPC, qfalse);
							}
							else if ((NPCS.NPCInfo->rank == RANK_CREWMAN || NPCS.NPCInfo->rank > RANK_LT_JG)
								&& !(NPCS.NPCInfo->scriptFlags & SCF_NO_ACROBATICS)
								&& NPCS.NPC->client->ps.fd.forceRageRecoveryTime < level.time
								&& !(NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_RAGE)
								&& !PM_InKnockDown(&NPCS.NPC->client->ps)
								&& NPCS.NPC->s.weapon != WP_SABER)
							{
								if (NPCS.NPC->s.weapon != WP_SABER)
								{
									//i dont have a saber can only try to jump right now
									NPCS.NPC->client->ps.fd.forceJumpCharge = 480;
									TIMER_Set(NPCS.NPC, "jumpChaseDebounce", Q_irand(2000, 5000));

									if (Q_irand(0, 2))
									{
										NPCS.ucmd.forwardmove = 127;
										VectorClear(NPCS.NPC->client->ps.moveDir);
									}
									else
									{
										NPCS.ucmd.forwardmove = -127;
										VectorClear(NPCS.NPC->client->ps.moveDir);
									}
								}
								else
								{
									Jedi_SaberBlock();
								}
							}
							else
							{
								if (!Jedi_SaberBlock())
								{
									Jedi_EvasionRoll(NPCS.NPC);
								}
							}
						}
						else if (enemy_attacking)
						{
							Jedi_SaberBlock();
						}
					}
					else if (!Jedi_SaberBlock())
					{
						Jedi_EvasionRoll(NPCS.NPC);
					}
				}
				else if (!Jedi_SaberBlock())
				{
					Jedi_EvasionRoll(NPCS.NPC);
				}
				else
				{
					//strafed
					if (d_JediAI.integer || g_DebugSaberCombat.integer)
					{
						Com_Printf("def strafe\n");
					}
					if (!(NPCS.NPCInfo->scriptFlags & SCF_NO_ACROBATICS)
						&& NPCS.NPC->client->ps.fd.forceRageRecoveryTime < level.time
						&& !(NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_RAGE)
						&& (NPCS.NPCInfo->rank == RANK_CREWMAN || NPCS.NPCInfo->rank > RANK_LT_JG)
						&& !PM_InKnockDown(&NPCS.NPC->client->ps)
						&& !Q_irand(0, 5))
					{
						//FIXME: make this a function call?
						if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT
							|| NPCS.NPC->client->NPC_class == CLASS_REBORN && NPCS.NPC->s.weapon != WP_SABER
							|| NPCS.NPC->client->NPC_class == CLASS_ROCKETTROOPER
							|| NPCS.NPC->client->pers.nextbotclass == BCLASS_BOBAFETT
							|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN1
							|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN2)
						{
							NPCS.NPC->client->ps.fd.forceJumpCharge = 280; //FIXME: calc this intelligently?
						}
						else
						{
							if (NPCS.NPC->s.weapon == WP_SABER)
							{
								Jedi_SaberBlock();
							}
							else
							{
								NPCS.NPC->client->ps.fd.forceJumpCharge = 320;
							}
						}
						//Don't jump again for another 2 to 5 seconds
						TIMER_Set(NPCS.NPC, "jumpChaseDebounce", Q_irand(2000, 5000));
					}
					else if (!Jedi_SaberBlock())
					{
						Jedi_EvasionRoll(NPCS.NPC);
					}
				}
				break;
			}

			//turn off slow walking no matter what
			TIMER_Set(NPCS.NPC, "walking", -level.time);
			TIMER_Set(NPCS.NPC, "taunting", -level.time);
		}
	}
}

/*
==========================================================================================
INTERNAL AI ROUTINES
==========================================================================================
*/
gentity_t* Jedi_FindEnemyInCone(const gentity_t* self, gentity_t* fallback, const float min_dot)
{
	vec3_t forward, mins, maxs;
	gentity_t* enemy = fallback;
	int entity_list[MAX_GENTITIES];
	int e;
	trace_t tr;

	if (!self->client)
	{
		return enemy;
	}

	AngleVectors(self->client->ps.viewangles, forward, NULL, NULL);

	for (e = 0; e < 3; e++)
	{
		mins[e] = self->r.currentOrigin[e] - 1024;
		maxs[e] = self->r.currentOrigin[e] + 1024;
	}
	const int num_listed_entities = trap->EntitiesInBox(mins, maxs, entity_list, MAX_GENTITIES);

	for (e = 0; e < num_listed_entities; e++)
	{
		const float bestDist = Q3_INFINITE;
		vec3_t dir;
		gentity_t* check = &g_entities[entity_list[e]];
		if (check == self)
		{
			//me
			continue;
		}
		if (!check->inuse)
		{
			//freed
			continue;
		}
		if (!check->client)
		{
			//not a client - FIXME: what about turrets?
			continue;
		}
		if (check->client->playerTeam != self->client->enemyTeam)
		{
			//not an enemy - FIXME: what about turrets?
			continue;
		}
		if (check->health <= 0)
		{
			//dead
			continue;
		}

		if (!trap->InPVS(check->r.currentOrigin, self->r.currentOrigin))
		{
			//can't potentially see them
			continue;
		}

		VectorSubtract(check->r.currentOrigin, self->r.currentOrigin, dir);
		float dist = VectorNormalize(dir);

		if (DotProduct(dir, forward) < min_dot)
		{
			//not in front
			continue;
		}

		//really should have a clear LOS to this thing...
		trap->Trace(&tr, self->r.currentOrigin, vec3_origin, vec3_origin, check->r.currentOrigin, self->s.number,
			MASK_SHOT, qfalse, 0, 0);
		if (tr.fraction < 1.0f && tr.entityNum != check->s.number)
		{
			//must have clear shot
			continue;
		}

		if (dist < bestDist)
		{
			//closer than our last best one
			dist = bestDist;
			enemy = check;
		}
	}
	return enemy;
}

static qboolean enemy_in_striking_range = qfalse;

static void jedi_set_enemy_info(vec3_t enemy_dest, vec3_t enemy_dir, float* enemy_dist, vec3_t enemy_movedir,
	float* enemy_movespeed, const int prediction)
{
	if (!NPCS.NPC || !NPCS.NPC->enemy)
	{
		//no valid enemy
		return;
	}
	if (!NPCS.NPC->enemy->client)
	{
		VectorClear(enemy_movedir);
		*enemy_movespeed = 0;
		VectorCopy(NPCS.NPC->enemy->r.currentOrigin, enemy_dest);
		enemy_dest[2] += NPCS.NPC->enemy->r.mins[2] + 24; //get it's origin to a height I can work with
		VectorSubtract(enemy_dest, NPCS.NPC->r.currentOrigin, enemy_dir);
		//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
		*enemy_dist = VectorNormalize(enemy_dir); // - (NPC->client->ps.saberLengthMax + NPC->r.maxs[0]*1.5 + 16);
	}
	else
	{
		//see where enemy is headed
		VectorCopy(NPCS.NPC->enemy->client->ps.velocity, enemy_movedir);
		*enemy_movespeed = VectorNormalize(enemy_movedir);
		//figure out where he'll be, say, 3 frames from now
		VectorMA(NPCS.NPC->enemy->r.currentOrigin, *enemy_movespeed * 0.001 * prediction, enemy_movedir, enemy_dest);
		//figure out what dir the enemy's estimated position is from me and how far from the tip of my saber he is
		VectorSubtract(enemy_dest, NPCS.NPC->r.currentOrigin, enemy_dir); //NPC->client->renderInfo.muzzlePoint
		//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
		*enemy_dist = VectorNormalize(enemy_dir) - (NPCS.NPC->client->saber[0].blade[0].lengthMax + NPCS.NPC->r.maxs[0]
			* 1.5 + 16);
	}
	enemy_in_striking_range = qfalse;
	if (*enemy_dist <= 0.0f)
	{
		enemy_in_striking_range = qtrue;
	}
	else
	{
		//if he's too far away, see if he's at least facing us or coming towards us
		if (*enemy_dist <= 32.0f)
		{
			//has to be facing us
			vec3_t eAngles;
			VectorSet(eAngles, 0, NPCS.NPC->r.currentAngles[YAW], 0);
			if (InFOV3(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin, eAngles, 30, 90))
			{
				//in striking range
				enemy_in_striking_range = qtrue;
			}
		}
		if (*enemy_dist >= 64.0f)
		{
			//we have to be approaching each other
			float vDot = 1.0f;
			if (!VectorCompare(NPCS.NPC->client->ps.velocity, vec3_origin))
			{
				//I am moving, see if I'm moving toward the enemy
				vec3_t eDir;
				VectorSubtract(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, eDir);
				VectorNormalize(eDir);
				vDot = DotProduct(eDir, NPCS.NPC->client->ps.velocity);
			}
			else if (NPCS.NPC->enemy->client && !VectorCompare(NPCS.NPC->enemy->client->ps.velocity, vec3_origin))
			{
				//I'm not moving, but the enemy is, see if he's moving towards me
				vec3_t meDir;
				VectorSubtract(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin, meDir);
				VectorNormalize(meDir);
				vDot = DotProduct(meDir, NPCS.NPC->enemy->client->ps.velocity);
			}
			else
			{
				//neither of us is moving, below check will fail, so just return
				return;
			}
			if (vDot >= *enemy_dist)
			{
				//moving towards each other
				enemy_in_striking_range = qtrue;
			}
		}
	}
}

void NPC_EvasionSaber(void)
{
	//uses some cheap movement prediction to dodge saber strikes from the NPC's enemy
	if (NPCS.ucmd.upmove <= 0 //not jumping
		&& (!NPCS.ucmd.upmove || !NPCS.ucmd.rightmove)) //either just ducking or just strafing (i.e.: not rolling
	{
		//see if we need to avoid their saber
		vec3_t enemy_dir, enemy_movedir, enemy_dest;
		float enemy_dist, enemy_movespeed;
		//set enemy
		jedi_set_enemy_info(enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300);
		Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
	}
}

extern float WP_SpeedOfMissileForWeapon(int wp, qboolean alt_fire);

static void Jedi_FaceEnemy(const qboolean doPitch)
{
	vec3_t enemy_eyes, eyes, angles;

	if (NPCS.NPC == NULL)
		return;

	if (NPCS.NPC->enemy == NULL)
		return;

	if (NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_GRIP &&
		NPCS.NPC->client->ps.fd.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1)
	{
		//don't update?
		NPCS.NPCInfo->desiredPitch = NPCS.NPC->client->ps.viewangles[PITCH];
		NPCS.NPCInfo->desiredYaw = NPCS.NPC->client->ps.viewangles[YAW];
		return;
	}
	CalcEntitySpot(NPCS.NPC, SPOT_HEAD, eyes);

	CalcEntitySpot(NPCS.NPC->enemy, SPOT_HEAD, enemy_eyes);

	if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT || NPCS.NPC->client->NPC_class == CLASS_MANDO
		|| NPCS.NPC->client->pers.nextbotclass == BCLASS_BOBAFETT
		|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN1
		|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN2
		&& TIMER_Done(NPCS.NPC, "flameTime")
		&& NPCS.NPC->s.weapon != WP_NONE
		&& NPCS.NPC->s.weapon != WP_DISRUPTOR
		&& (NPCS.NPC->s.weapon != WP_ROCKET_LAUNCHER || !(NPCS.NPCInfo->scriptFlags & SCF_altFire))
		&& NPCS.NPC->s.weapon != WP_THERMAL
		&& NPCS.NPC->s.weapon != WP_TRIP_MINE
		&& NPCS.NPC->s.weapon != WP_DET_PACK
		&& NPCS.NPC->s.weapon != WP_STUN_BATON
		&& NPCS.NPC->s.weapon != WP_MELEE)
	{
		//boba leads his enemy
		if (NPCS.NPC->health < NPCS.NPC->client->pers.maxHealth * 0.5f)
		{
			//lead
			const float missileSpeed = WP_SpeedOfMissileForWeapon(NPCS.NPC->s.weapon,
				NPCS.NPCInfo->scriptFlags & SCF_altFire);
			if (missileSpeed)
			{
				float eDist = Distance(eyes, enemy_eyes);
				eDist /= missileSpeed; //How many seconds it will take to get to the enemy
				VectorMA(enemy_eyes, eDist * flrand(0.95f, 1.25f), NPCS.NPC->enemy->client->ps.velocity, enemy_eyes);
			}
		}
	}

	//Find the desired angles
	if (!NPCS.NPC->client->ps.saberInFlight
		&& (NPCS.NPC->client->ps.legsAnim == BOTH_A2_STABBACK1
			|| NPCS.NPC->client->ps.legsAnim == BOTH_A2_STABBACK1B
			|| NPCS.NPC->client->ps.legsAnim == BOTH_CROUCHATTACKBACK1
			|| NPCS.NPC->client->ps.legsAnim == BOTH_ATTACK_BACK))
	{
		//point *away*
		GetAnglesForDirection(enemy_eyes, eyes, angles);
	}
	else if (NPCS.NPC->client->ps.legsAnim == BOTH_A7_KICK_R)
	{
		//keep enemy to right
	}
	else if (NPCS.NPC->client->ps.legsAnim == BOTH_A7_KICK_L)
	{
		//keep enemy to left
	}
	else if (NPCS.NPC->client->ps.legsAnim == BOTH_A7_KICK_RL
		|| NPCS.NPC->client->ps.legsAnim == BOTH_A7_KICK_BF
		|| NPCS.NPC->client->ps.legsAnim == BOTH_A7_KICK_S)
	{
		//???
	}
	else
	{
		//point towards him
		GetAnglesForDirection(eyes, enemy_eyes, angles);
	}

	NPCS.NPCInfo->desiredYaw = AngleNormalize360(angles[YAW]);

	if (doPitch)
	{
		NPCS.NPCInfo->desiredPitch = AngleNormalize360(angles[PITCH]);
		if (NPCS.NPC->client->ps.saberInFlight)
		{
			//tilt down a little
			NPCS.NPCInfo->desiredPitch += 10;
		}
	}
}

static void Jedi_DebounceDirectionChanges(void)
{
	//FIXME: check these before making fwd/back & right/left decisions?
	//Time-debounce changes in forward/back dir
	if (NPCS.ucmd.forwardmove > 0)
	{
		if (!TIMER_Done(NPCS.NPC, "moveback") || !TIMER_Done(NPCS.NPC, "movenone"))
		{
			NPCS.ucmd.forwardmove = 0;
			//now we have to normalize the total movement again
			if (NPCS.ucmd.rightmove > 0)
			{
				NPCS.ucmd.rightmove = 127;
			}
			else if (NPCS.ucmd.rightmove < 0)
			{
				NPCS.ucmd.rightmove = -127;
			}
			VectorClear(NPCS.NPC->client->ps.moveDir);
			TIMER_Set(NPCS.NPC, "moveback", -level.time);
			if (TIMER_Done(NPCS.NPC, "movenone"))
			{
				TIMER_Set(NPCS.NPC, "movenone", Q_irand(1000, 2000));
			}
		}
		else if (TIMER_Done(NPCS.NPC, "moveforward"))
		{
			//FIXME: should be if it's zero
			if (TIMER_Done(NPCS.NPC, "lastmoveforward"))
			{
				const int holdDirTime = Q_irand(500, 2000);
				TIMER_Set(NPCS.NPC, "moveforward", holdDirTime);
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set(NPCS.NPC, "lastmoveforward", holdDirTime + Q_irand(1000, 2000));
			}
		}
		else
		{
			//NOTE: edge checking should stop me if this is bad... but what if it sends us colliding into the enemy?
			//if being forced to move forward, do a full-speed moveforward
			NPCS.ucmd.forwardmove = 127;
			VectorClear(NPCS.NPC->client->ps.moveDir);
		}
	}
	else if (NPCS.ucmd.forwardmove < 0)
	{
		if (!TIMER_Done(NPCS.NPC, "moveforward") || !TIMER_Done(NPCS.NPC, "movenone"))
		{
			NPCS.ucmd.forwardmove = 0;
			//now we have to normalize the total movement again
			if (NPCS.ucmd.rightmove > 0)
			{
				NPCS.ucmd.rightmove = 127;
			}
			else if (NPCS.ucmd.rightmove < 0)
			{
				NPCS.ucmd.rightmove = -127;
			}
			VectorClear(NPCS.NPC->client->ps.moveDir);
			TIMER_Set(NPCS.NPC, "moveforward", -level.time);
			if (TIMER_Done(NPCS.NPC, "movenone"))
			{
				TIMER_Set(NPCS.NPC, "movenone", Q_irand(1000, 2000));
			}
		}
		else if (TIMER_Done(NPCS.NPC, "moveback"))
		{
			//FIXME: should be if it's zero?
			if (TIMER_Done(NPCS.NPC, "lastmoveback"))
			{
				const int holdDirTime = Q_irand(500, 2000);
				TIMER_Set(NPCS.NPC, "moveback", holdDirTime);
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set(NPCS.NPC, "lastmoveback", holdDirTime + Q_irand(1000, 2000));
			}
		}
		else
		{
			//NOTE: edge checking should stop me if this is bad...
			//if being forced to move back, do a full-speed moveback
			NPCS.ucmd.forwardmove = -127;
			VectorClear(NPCS.NPC->client->ps.moveDir);
		}
	}
	else if (!TIMER_Done(NPCS.NPC, "moveforward"))
	{
		//NOTE: edge checking should stop me if this is bad... but what if it sends us colliding into the enemy?
		NPCS.ucmd.forwardmove = 127;
		VectorClear(NPCS.NPC->client->ps.moveDir);
	}
	else if (!TIMER_Done(NPCS.NPC, "moveback"))
	{
		//NOTE: edge checking should stop me if this is bad...
		NPCS.ucmd.forwardmove = -127;
		VectorClear(NPCS.NPC->client->ps.moveDir);
	}
	//Time-debounce changes in right/left dir
	if (NPCS.ucmd.rightmove > 0)
	{
		if (!TIMER_Done(NPCS.NPC, "moveleft") || !TIMER_Done(NPCS.NPC, "movecenter"))
		{
			NPCS.ucmd.rightmove = 0;
			//now we have to normalize the total movement again
			if (NPCS.ucmd.forwardmove > 0)
			{
				NPCS.ucmd.forwardmove = 127;
			}
			else if (NPCS.ucmd.forwardmove < 0)
			{
				NPCS.ucmd.forwardmove = -127;
			}
			VectorClear(NPCS.NPC->client->ps.moveDir);
			TIMER_Set(NPCS.NPC, "moveleft", -level.time);
			if (TIMER_Done(NPCS.NPC, "movecenter"))
			{
				TIMER_Set(NPCS.NPC, "movecenter", Q_irand(1000, 2000));
			}
		}
		else if (TIMER_Done(NPCS.NPC, "moveright"))
		{
			//FIXME: should be if it's zero?
			if (TIMER_Done(NPCS.NPC, "lastmoveright"))
			{
				const int holdDirTime = Q_irand(250, 1500);
				TIMER_Set(NPCS.NPC, "moveright", holdDirTime);
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set(NPCS.NPC, "lastmoveright", holdDirTime + Q_irand(1000, 2000));
			}
		}
		else
		{
			//NOTE: edge checking should stop me if this is bad...
			//if being forced to move back, do a full-speed moveright
			NPCS.ucmd.rightmove = 127;
			VectorClear(NPCS.NPC->client->ps.moveDir);
		}
	}
	else if (NPCS.ucmd.rightmove < 0)
	{
		if (!TIMER_Done(NPCS.NPC, "moveright") || !TIMER_Done(NPCS.NPC, "movecenter"))
		{
			NPCS.ucmd.rightmove = 0;
			//now we have to normalize the total movement again
			if (NPCS.ucmd.forwardmove > 0)
			{
				NPCS.ucmd.forwardmove = 127;
			}
			else if (NPCS.ucmd.forwardmove < 0)
			{
				NPCS.ucmd.forwardmove = -127;
			}
			VectorClear(NPCS.NPC->client->ps.moveDir);
			TIMER_Set(NPCS.NPC, "moveright", -level.time);
			if (TIMER_Done(NPCS.NPC, "movecenter"))
			{
				TIMER_Set(NPCS.NPC, "movecenter", Q_irand(1000, 2000));
			}
		}
		else if (TIMER_Done(NPCS.NPC, "moveleft"))
		{
			//FIXME: should be if it's zero?
			if (TIMER_Done(NPCS.NPC, "lastmoveleft"))
			{
				const int holdDirTime = Q_irand(250, 1500);
				TIMER_Set(NPCS.NPC, "moveleft", holdDirTime);
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set(NPCS.NPC, "lastmoveleft", holdDirTime + Q_irand(1000, 2000));
			}
		}
		else
		{
			//NOTE: edge checking should stop me if this is bad...
			//if being forced to move back, do a full-speed moveleft
			NPCS.ucmd.rightmove = -127;
			VectorClear(NPCS.NPC->client->ps.moveDir);
		}
	}
	else if (!TIMER_Done(NPCS.NPC, "moveright"))
	{
		//NOTE: edge checking should stop me if this is bad...
		NPCS.ucmd.rightmove = 127;
		VectorClear(NPCS.NPC->client->ps.moveDir);
	}
	else if (!TIMER_Done(NPCS.NPC, "moveleft"))
	{
		//NOTE: edge checking should stop me if this is bad...
		NPCS.ucmd.rightmove = -127;
		VectorClear(NPCS.NPC->client->ps.moveDir);
	}
}

static void Jedi_TimersApply(void)
{
	//use careful anim/slower movement if not already moving
	if (!NPCS.ucmd.forwardmove && !TIMER_Done(NPCS.NPC, "walking"))
	{
		NPCS.ucmd.buttons |= BUTTON_WALKING;
	}

	if (!TIMER_Done(NPCS.NPC, "taunting"))
	{
		NPCS.ucmd.buttons |= BUTTON_WALKING;
	}

	if (!NPCS.ucmd.rightmove)
	{
		//only if not already strafing
		//FIXME: if enemy behind me and turning to face enemy, don't strafe in that direction, too
		if (!TIMER_Done(NPCS.NPC, "strafeLeft"))
		{
			if (NPCS.NPCInfo->desiredYaw > NPCS.NPC->client->ps.viewangles[YAW] + 60)
			{
				//we want to turn left, don't apply the strafing
			}
			else
			{
				//go ahead and strafe left
				NPCS.ucmd.rightmove = -127;
				VectorClear(NPCS.NPC->client->ps.moveDir);
			}
		}
		else if (!TIMER_Done(NPCS.NPC, "strafeRight"))
		{
			if (NPCS.NPCInfo->desiredYaw < NPCS.NPC->client->ps.viewangles[YAW] - 60)
			{
				//we want to turn right, don't apply the strafing
			}
			else
			{
				//go ahead and strafe left
				NPCS.ucmd.rightmove = 127;
				VectorClear(NPCS.NPC->client->ps.moveDir);
			}
		}
	}

	Jedi_DebounceDirectionChanges();

	if (!TIMER_Done(NPCS.NPC, "gripping"))
	{
		//FIXME: what do we do if we ran out of power?  NPC's can't?
		//FIXME: don't keep turning to face enemy or we'll end up spinning around
		NPCS.ucmd.buttons |= BUTTON_FORCEGRIP;
	}

	if (!TIMER_Done(NPCS.NPC, "draining"))
	{
		//FIXME: what do we do if we ran out of power?  NPC's can't?
		//FIXME: don't keep turning to face enemy or we'll end up spinning around
		NPCS.ucmd.buttons |= BUTTON_FORCE_DRAIN;
	}

	if (!TIMER_Done(NPCS.NPC, "holdLightning"))
	{
		//hold down the lightning key
		NPCS.ucmd.buttons |= BUTTON_FORCE_LIGHTNING;
	}
}

static void Jedi_CombatTimersUpdate(const int enemy_dist)
{
	if (Jedi_CultistDestroyer(NPCS.NPC))
	{
		Jedi_Aggression(NPCS.NPC, 5);
		return;
	}
	if (TIMER_Done(NPCS.NPC, "roamTime"))
	{
		TIMER_Set(NPCS.NPC, "roamTime", Q_irand(2000, 5000));

		if (NPCS.NPC->enemy && NPCS.NPC->enemy->client)
		{
			switch (NPCS.NPC->enemy->client->ps.weapon)
			{
			case WP_SABER:
				//If enemy has a lightsaber, always close in
				if (BG_SabersOff(&NPCS.NPC->enemy->client->ps))
				{
					//fool!  Standing around unarmed, charge!
					//Com_Printf( "(%d) raise agg - enemy saber off\n", level.time );
					Jedi_Aggression(NPCS.NPC, 2);
				}
				else
				{
					//Com_Printf( "(%d) raise agg - enemy saber\n", level.time );
					Jedi_Aggression(NPCS.NPC, 1);
				}
				break;
			case WP_BLASTER:
			case WP_BRYAR_PISTOL:
			case WP_DISRUPTOR:
			case WP_BOWCASTER:
			case WP_REPEATER:
			case WP_DEMP2:
			case WP_FLECHETTE:
			case WP_ROCKET_LAUNCHER:
				//if he has a blaster, move in when:
				//They're not shooting at me
				if (NPCS.NPC->enemy->attackDebounceTime < level.time)
				{
					//does this apply to players?
					//Com_Printf( "(%d) raise agg - enemy not shooting ranged weap\n", level.time );
					Jedi_Aggression(NPCS.NPC, 1);
				}
				//He's closer than a dist that gives us time to deflect
				if (enemy_dist < 256)
				{
					//Com_Printf( "(%d) raise agg - enemy ranged weap- too close\n", level.time );
					Jedi_Aggression(NPCS.NPC, 1);
				}
				break;
			default:
				break;
			}
		}
	}

	if (TIMER_Done(NPCS.NPC, "noStrafe") && TIMER_Done(NPCS.NPC, "strafeLeft") && TIMER_Done(NPCS.NPC, "strafeRight"))
	{
		//FIXME: Maybe more likely to do this if aggression higher?  Or some other stat?
		if (!Q_irand(0, 4))
		{
			//start a strafe
			if (jedi_strafe(1000, 3000, 0, 4000, qtrue))
			{
				if (d_JediAI.integer || g_DebugSaberCombat.integer)
				{
					Com_Printf("off strafe\n");
				}
			}
		}
		else
		{
			//postpone any strafing for a while
			TIMER_Set(NPCS.NPC, "noStrafe", Q_irand(1000, 3000));
		}
	}

	if (NPCS.NPC->client->ps.saberEventFlags)
	{
		//some kind of saber combat event is still pending
		int newFlags = NPCS.NPC->client->ps.saberEventFlags;
		if (NPCS.NPC->client->ps.saberEventFlags & SEF_PARRIED)
		{
			//parried
			TIMER_Set(NPCS.NPC, "parryTime", -1);
			if (NPCS.NPC->enemy && (!NPCS.NPC->enemy->client || PM_SaberInKnockaway(
				NPCS.NPC->enemy->client->ps.saber_move)))
			{
				//advance!
				Jedi_Aggression(NPCS.NPC, 1); //get closer
				Jedi_AdjustSaberAnimLevel(NPCS.NPC, NPCS.NPC->client->ps.fd.saberAnimLevel - 1); //use a faster attack
			}
			else
			{
				if (!Q_irand(0, 1)) //FIXME: dependant on rank/diff?
				{
					//Com_Printf( "(%d) drop agg - we parried\n", level.time );
					Jedi_Aggression(NPCS.NPC, -1);
				}
				if (!Q_irand(0, 1))
				{
					Jedi_AdjustSaberAnimLevel(NPCS.NPC, NPCS.NPC->client->ps.fd.saberAnimLevel - 1);
				}
			}
			if (d_JediAI.integer || g_DebugSaberCombat.integer)
			{
				Com_Printf("(%d) PARRY: agg %d, no parry until %d\n", level.time, NPCS.NPCInfo->stats.aggression,
					level.time + 100);
			}
			newFlags &= ~SEF_PARRIED;
		}
		if (!NPCS.NPC->client->ps.weaponTime && NPCS.NPC->client->ps.saberEventFlags & SEF_HITENEMY) //hit enemy
		{
			//we hit our enemy last time we swung, drop our aggression
			if (!Q_irand(0, 1)) //FIXME: dependant on rank/diff?
			{
				//Com_Printf( "(%d) drop agg - we hit enemy\n", level.time );
				Jedi_Aggression(NPCS.NPC, -1);
				if (d_JediAI.integer || g_DebugSaberCombat.integer)
				{
					Com_Printf("(%d) HIT: agg %d\n", level.time, NPCS.NPCInfo->stats.aggression);
				}
				if (!Q_irand(0, 3)
					&& NPCS.NPCInfo->blockedSpeechDebounceTime < level.time
					&& jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] < level.time
					&& NPCS.NPC->painDebounceTime < level.time - 1000)
				{
					if (!npc_is_dark_jedi(NPCS.NPC))
					{
						G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_ANGER1, EV_ANGER3), 2000);
					}
					else
					{
						G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_GLOAT1, EV_GLOAT3), 10000);
					}

					jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->blockedSpeechDebounceTime =
						level.time + 3000;
				}
			}
			if (!Q_irand(0, 2))
			{
				Jedi_AdjustSaberAnimLevel(NPCS.NPC, NPCS.NPC->client->ps.fd.saberAnimLevel + 1);
			}
			newFlags &= ~SEF_HITENEMY;
		}
		if (NPCS.NPC->client->ps.saberEventFlags & SEF_BLOCKED)
		{
			//was blocked whilst attacking
			if (PM_SaberInBrokenParry(NPCS.NPC->client->ps.saber_move)
				|| NPCS.NPC->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN)
			{
				//Com_Printf( "(%d) drop agg - we were knock-blocked\n", level.time );
				if (NPCS.NPC->client->ps.saberInFlight)
				{
					//lost our saber, too!!!
					Jedi_Aggression(NPCS.NPC, -5); //really really really should back off!!!
				}
				else
				{
					Jedi_Aggression(NPCS.NPC, -2); //really should back off!
				}
				Jedi_AdjustSaberAnimLevel(NPCS.NPC, NPCS.NPC->client->ps.fd.saberAnimLevel + 1);
				//use a stronger attack
				if (d_JediAI.integer || g_DebugSaberCombat.integer)
				{
					Com_Printf("(%d) KNOCK-BLOCKED: agg %d\n", level.time, NPCS.NPCInfo->stats.aggression);
				}
			}
			else
			{
				if (!Q_irand(0, 2)) //FIXME: dependant on rank/diff?
				{
					//Com_Printf( "(%d) drop agg - we were blocked\n", level.time );
					Jedi_Aggression(NPCS.NPC, -1);
					if (d_JediAI.integer || g_DebugSaberCombat.integer)
					{
						Com_Printf("(%d) BLOCKED: agg %d\n", level.time, NPCS.NPCInfo->stats.aggression);
					}
				}
				if (!Q_irand(0, 1))
				{
					Jedi_AdjustSaberAnimLevel(NPCS.NPC, NPCS.NPC->client->ps.fd.saberAnimLevel + 1);
				}
			}
			newFlags &= ~SEF_BLOCKED;
			//FIXME: based on the type of parry the enemy is doing and my skill,
			//		choose an attack that is likely to get around the parry?
			//		right now that's generic in the saber animation code, auto-picks
			//		a next anim for me, but really should be AI-controlled.
		}
		if (NPCS.NPC->client->ps.saberEventFlags & SEF_DEFLECTED)
		{
			//deflected a shot
			newFlags &= ~SEF_DEFLECTED;
			if (!Q_irand(0, 3))
			{
				Jedi_AdjustSaberAnimLevel(NPCS.NPC, NPCS.NPC->client->ps.fd.saberAnimLevel - 1);
			}
		}
		if (NPCS.NPC->client->ps.saberEventFlags & SEF_HITWALL)
		{
			//hit a wall
			newFlags &= ~SEF_HITWALL;
		}
		if (NPCS.NPC->client->ps.saberEventFlags & SEF_HITOBJECT)
		{
			//hit some other damagable object
			if (!Q_irand(0, 3))
			{
				Jedi_AdjustSaberAnimLevel(NPCS.NPC, NPCS.NPC->client->ps.fd.saberAnimLevel - 1);
			}
			newFlags &= ~SEF_HITOBJECT;
		}
		NPCS.NPC->client->ps.saberEventFlags = newFlags;
	}
}

static void Jedi_CombatIdle(const int enemy_dist)
{
	if (!TIMER_Done(NPCS.NPC, "parryTime"))
	{
		return;
	}
	if (NPCS.NPC->client->ps.saberInFlight)
	{
		//don't do this idle stuff if throwing saber
		return;
	}
	if (NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_RAGE
		|| NPCS.NPC->client->ps.fd.forceRageRecoveryTime > level.time)
	{
		//never taunt while raging or recovering from rage
		return;
	}
	if (NPCS.NPC->client->saber[0].type == SABER_SITH_SWORD)
	{
		//never taunt when holding sith sword
		return;
	}
	//FIXME: make these distance numbers defines?
	if (enemy_dist >= 64)
	{
		//FIXME: only do this if standing still?
		//based on aggression, flaunt/taunt
		int chance = 20;
		if (NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER)
		{
			chance = 10;
		}
		//FIXME: possibly throw local objects at enemy?
		if (Q_irand(2, chance) < NPCS.NPCInfo->stats.aggression)
		{
			if (TIMER_Done(NPCS.NPC, "chatter") && NPCS.NPC->client->ps.forceHandExtend == HANDEXTEND_NONE)
			{
				//FIXME: add more taunt behaviors
				//FIXME: sometimes he turns it off, then turns it right back on again???
				if (enemy_dist > 200
					&& NPCS.NPC->client->NPC_class != CLASS_BOBAFETT
					&& (NPCS.NPC->client->NPC_class != CLASS_REBORN || NPCS.NPC->s.weapon == WP_SABER)
					&& NPCS.NPC->client->NPC_class != CLASS_ROCKETTROOPER
					|| NPCS.NPC->client->pers.nextbotclass != BCLASS_BOBAFETT
					|| NPCS.NPC->client->pers.nextbotclass != BCLASS_MANDOLORIAN1
					|| NPCS.NPC->client->pers.nextbotclass != BCLASS_MANDOLORIAN2
					&& !NPCS.NPC->client->ps.saberHolstered
					&& !Q_irand(0, 5))
				{
					//taunt even more, turn off the saber
					//some taunts leave the saber on, otherwise turn it off
					if (NPCS.NPC->client->ps.fd.saberAnimLevel != SS_STAFF
						&& NPCS.NPC->client->ps.fd.saberAnimLevel != SS_DUAL)
					{
						//those taunts leave saber on
						WP_DeactivateSaber(NPCS.NPC);
					}
					//Don't attack for a bit
					NPCS.NPCInfo->stats.aggression = 3;
					//FIXME: maybe start strafing?
					//debounce this
					if (NPCS.NPC->client->playerTeam != NPCTEAM_PLAYER && !Q_irand(0, 1))
					{
						NPCS.NPC->client->ps.forceHandExtend = HANDEXTEND_JEDITAUNT;
						NPCS.NPC->client->ps.forceHandExtendTime = level.time + 5000;

						TIMER_Set(NPCS.NPC, "chatter", Q_irand(5000, 10000));
						TIMER_Set(NPCS.NPC, "taunting", 5500);
					}
					else
					{
						Jedi_BattleTaunt(NPCS.NPC);
						TIMER_Set(NPCS.NPC, "taunting", Q_irand(5000, 10000));
					}
				}
				else if (Jedi_BattleTaunt(NPCS.NPC))
				{
					//FIXME: pick some anims
				}
			}
		}
	}
}

static qboolean Jedi_AttackDecide(const int enemy_dist)
{
	if (!TIMER_Done(NPCS.NPC, "allyJediDelay"))
	{
		//jedi allies hold off attacking non-saber using enemies.  Probably to give the
		//player more oppurtunity to battle.
		return qfalse;
	}
	// Begin fixed cultist_destroyer AI
	if (Jedi_CultistDestroyer(NPCS.NPC))
	{
		// destroyer
		if (enemy_dist <= 32)
		{
			//go boom!
			NPCS.NPC->flags |= FL_GODMODE;
			NPCS.NPC->takedamage = qfalse;

			NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_FORCE_RAGE, SETANIM_FLAG_HOLD | SETANIM_FLAG_OVERRIDE);
			NPCS.NPC->client->ps.fd.forcePowersActive |= 1 << FP_RAGE;
			NPCS.NPC->painDebounceTime = NPCS.NPC->useDebounceTime = level.time + NPCS.NPC->client->ps.torsoTimer;
			return qtrue;
		}
		return qfalse;
	}

	if (NPCS.NPC->enemy->client
		&& NPCS.NPC->enemy->s.weapon == WP_SABER
		&& NPCS.NPC->enemy->client->ps.saberLockTime > level.time
		&& NPCS.NPC->client->ps.saberLockTime < level.time)
	{
		//enemy is in a saberLock and we are not
		return qfalse;
	}

	if (NPCS.NPC->client->ps.saberEventFlags & SEF_LOCK_WON)
	{
		//we won a saber lock, press the advantage with an attack!
		int chance = 0;
		if (NPCS.NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER)
		{
			//desann and luke
			chance = 20;
		}
		else if (NPCS.NPC->client->NPC_class == CLASS_TAVION || NPCS.NPC->client->NPC_class == CLASS_ALORA)
		{
			//tavion
			chance = 10;
		}
		else if (NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER)
		{
			//shadowtrooper
			chance = 5;
		}
		else if (NPCS.NPC->client->NPC_class == CLASS_REBORN)
		{
			//fencer
			chance = 5;
		}
		else
		{
			chance = NPCS.NPCInfo->rank;
		}
		if (Q_irand(0, 30) < chance)
		{
			//based on skill with some randomness
			NPCS.NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON; //clear this now that we are using the opportunity
			TIMER_Set(NPCS.NPC, "noRetreat", Q_irand(500, 2000));
			NPCS.NPC->client->ps.weaponTime = NPCS.NPCInfo->shotTime = NPCS.NPC->attackDebounceTime = 0;
			NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
			WeaponThink();
			return qtrue;
		}
	}

	if (NPCS.NPC->client->NPC_class == CLASS_TAVION ||
		NPCS.NPC->client->NPC_class == CLASS_ALORA ||
		NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER ||
		NPCS.NPC->client->NPC_class == CLASS_REBORN ||
		NPCS.NPC->client->NPC_class == CLASS_JEDI)
	{
		//tavion, fencers, jedi trainer are all good at following up a parry with an attack
		if ((PM_SaberInParry(NPCS.NPC->client->ps.saber_move) || PM_SaberInKnockaway(NPCS.NPC->client->ps.saber_move))
			&& NPCS.NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN)
		{
			//try to attack straight from a parry
			NPCS.NPC->client->ps.weaponTime = NPCS.NPCInfo->shotTime = NPCS.NPC->attackDebounceTime = 0;
			NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
			Jedi_AdjustSaberAnimLevel(NPCS.NPC, SS_FAST); //try to follow-up with a quick attack
			WeaponThink();
			return qtrue;
		}
	}

	//try to hit them if we can
	if (!enemy_in_striking_range)
	{
		return qfalse;
	}

	if (!TIMER_Done(NPCS.NPC, "parryTime"))
	{
		return qfalse;
	}

	if (NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE)
	{
		//not allowed to attack
		return qfalse;
	}

	if (!(NPCS.ucmd.buttons & BUTTON_ATTACK) && !(NPCS.ucmd.buttons & BUTTON_ALT_ATTACK) && !(NPCS.ucmd.buttons &
		BUTTON_FORCEPOWER) && !(NPCS.ucmd.buttons & BUTTON_DASH))
	{
		//not already attacking
		if (CanShoot(NPCS.NPC->enemy, NPCS.NPC))
		{
			if (NPCS.NPC->s.weapon == WP_SABER)
			{
				//Try to attack
				NPC_FaceEnemy(qtrue);

				if (!PM_SaberInAttack(NPCS.NPC->client->ps.saber_move) && !Jedi_SaberBusy(NPCS.NPC))
				{
					if (NPCS.NPC->client->ps.fd.blockPoints < BLOCKPOINTS_HALF)
					{
						// Back away while attacking...
						const int rand = irand(0, 100);

						if (rand < 20)
						{
							NPCS.NPC->client->pers.cmd.rightmove = 64;
						}
						else if (rand < 40)
						{
							NPCS.NPC->client->pers.cmd.rightmove = -64;
						}

						WeaponThink();
					}
					else
					{
						const int rand = irand(0, 100);

						Jedi_Advance();

						if (rand < 20)
						{
							NPCS.NPC->client->pers.cmd.rightmove = 64;
						}
						else if (rand < 40)
						{
							NPCS.NPC->client->pers.cmd.rightmove = -64;
						}

						WeaponThink();

						if (rand > 95)
						{
							// Do a kata occasionally...
							NPCS.NPC->client->pers.cmd.buttons |= BUTTON_ATTACK;
							NPCS.NPC->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
						}
						else if (rand > 90)
						{
							// Do a lunge, etc occasionally...
							NPCS.NPC->client->pers.cmd.upmove = -127;
							NPCS.NPC->client->pers.cmd.buttons |= BUTTON_ATTACK;
						}
					}
				}
			}
			else
			{
				//Try to attack
				WeaponThink();
			}
		}
	}

	if (NPCS.ucmd.buttons & BUTTON_ATTACK && !NPC_Jumping())
	{
		//attacking
		if (!NPCS.ucmd.rightmove)
		{
			//not already strafing
			if (!Q_irand(0, 3))
			{
				//25% chance of doing this
				vec3_t right, dir2enemy;

				AngleVectors(NPCS.NPC->r.currentAngles, NULL, right, NULL);
				VectorSubtract(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentAngles, dir2enemy);
				if (DotProduct(right, dir2enemy) > 0)
				{
					//he's to my right, strafe left
					if (NPC_MoveDirClear(NPCS.ucmd.forwardmove, -127, qfalse))
					{
						NPCS.ucmd.rightmove = -127;
						VectorClear(NPCS.NPC->client->ps.moveDir);
					}
				}
				else
				{
					//he's to my left, strafe right
					if (NPC_MoveDirClear(NPCS.ucmd.forwardmove, 127, qfalse))
					{
						NPCS.ucmd.rightmove = 127;
						VectorClear(NPCS.NPC->client->ps.moveDir);
					}
				}
			}
		}
		return qtrue;
	}

	return qfalse;
}

#define	APEX_HEIGHT		200.0f
#define	PARA_WIDTH		(sqrt(APEX_HEIGHT)+sqrt(APEX_HEIGHT))
#define	JUMP_SPEED		200.0f

static qboolean Jedi_Jump(vec3_t dest, const int goal_ent_num)
{
	//FIXME: if land on enemy, knock him down & jump off again
	if (1)
	{
		float shot_speed = 300, best_impact_dist = Q3_INFINITE; //fireSpeed,
		vec3_t shot_vel, fail_case;
		trace_t trace;
		trajectory_t tr;
		int hit_count = 0;
		const int max_hits = 7;
		vec3_t bottom;

		while (hit_count < max_hits)
		{
			vec3_t target_dir;
			VectorSubtract(dest, NPCS.NPC->r.currentOrigin, target_dir);
			const float target_dist = VectorNormalize(target_dir);

			VectorScale(target_dir, shot_speed, shot_vel);
			float travel_time = target_dist / shot_speed;
			shot_vel[2] += travel_time * 0.5 * NPCS.NPC->client->ps.gravity;

			if (!hit_count)
			{
				//save the first one as the worst case scenario
				VectorCopy(shot_vel, fail_case);
			}

			if (1) //tracePath )
			{
				vec3_t last_pos;
				const int time_step = 500;
				//do a rough trace of the path
				qboolean blocked = qfalse;

				VectorCopy(NPCS.NPC->r.currentOrigin, tr.trBase);
				VectorCopy(shot_vel, tr.trDelta);
				tr.trType = TR_GRAVITY;
				tr.trTime = level.time;
				travel_time *= 1000.0f;
				VectorCopy(NPCS.NPC->r.currentOrigin, last_pos);

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
					if (test_pos[2] < last_pos[2])
					{
						//going down, ignore botclip
						trap->Trace(&trace, last_pos, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, test_pos, NPCS.NPC->s.number,
							NPCS.NPC->clipmask, qfalse, 0, 0);
					}
					else
					{
						//going up, check for botclip
						trap->Trace(&trace, last_pos, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, test_pos, NPCS.NPC->s.number,
							NPCS.NPC->clipmask | CONTENTS_BOTCLIP, qfalse, 0, 0);
					}

					if (trace.allsolid || trace.startsolid)
					{
						blocked = qtrue;
						break;
					}
					if (trace.fraction < 1.0f)
					{
						//hit something
						if (trace.entityNum == goal_ent_num)
						{
							//hit the enemy, that's perfect!
							//Hmm, don't want to land on him, though...
							break;
						}
						if (trace.contents & CONTENTS_BOTCLIP)
						{
							//hit a do-not-enter brush
							blocked = qtrue;
							break;
						}
						if (trace.plane.normal[2] > 0.7 && DistanceSquared(trace.endpos, dest) < 4096)
							//hit within 64 of desired location, should be okay
						{
							//close enough!
							break;
						}
						//FIXME: maybe find the extents of this brush and go above or below it on next try somehow?
						const float impactDist = DistanceSquared(trace.endpos, dest);
						if (impactDist < best_impact_dist)
						{
							best_impact_dist = impactDist;
							VectorCopy(shot_vel, fail_case);
						}
						blocked = qtrue;
						break;
					}
					if (elapsed_time == floor(travel_time))
					{
						//reached end, all clear
						if (trace.fraction >= 1.0f)
						{
							//hmm, make sure we'll land on the ground...
							//FIXME: do we care how far below ourselves or our dest we'll land?
							VectorCopy(trace.endpos, bottom);
							bottom[2] -= 128;
							trap->Trace(&trace, trace.endpos, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, bottom,
								NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0);
							if (trace.fraction >= 1.0f)
							{
								//would fall too far
								blocked = qtrue;
							}
						}
						break;
					}
					//all clear, try next slice
					VectorCopy(test_pos, last_pos);
				}
				if (blocked)
				{
					//hit something, adjust speed (which will change arc)
					hit_count++;
					shot_speed = 300 + (hit_count - 2) * 100; //from 100 to 900 (skipping 300)
					if (hit_count >= 2)
					{
						//skip 300 since that was the first value we tested
						shot_speed += 100;
					}
				}
				else
				{
					//made it!
					break;
				}
			}
		}

		if (hit_count >= max_hits)
		{
			VectorCopy(fail_case, NPCS.NPC->client->ps.velocity);
		}
		VectorCopy(shot_vel, NPCS.NPC->client->ps.velocity);
	}
	return qtrue;
}

static qboolean Jedi_TryJump(const gentity_t* goal)
{
	//FIXME: never does a simple short, regular jump...
	//FIXME: I need to be on ground too!
	if (NPCS.NPCInfo->scriptFlags & SCF_NO_ACROBATICS)
	{
		return qfalse;
	}
	if (TIMER_Done(NPCS.NPC, "jumpChaseDebounce"))
	{
		if (!goal->client || goal->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			if (!PM_InKnockDown(&NPCS.NPC->client->ps) && !BG_InRoll(&NPCS.NPC->client->ps,
				NPCS.NPC->client->ps.legsAnim))
			{
				//enemy is on terra firma
				vec3_t goal_diff;
				VectorSubtract(goal->r.currentOrigin, NPCS.NPC->r.currentOrigin, goal_diff);
				const float goal_z_diff = goal_diff[2];
				goal_diff[2] = 0;
				const float goal_xy_dist = VectorNormalize(goal_diff);
				if (goal_xy_dist < 550 && goal_z_diff > -400/*was -256*/)
					//for now, jedi don't take falling damage && (NPC->health > 20 || goal_z_diff > 0 ) && (NPC->health >= 100 || goal_z_diff > -128 ))//closer than @512
				{
					qboolean debounce = qfalse;
					if (NPCS.NPC->health < 150 && (NPCS.NPC->health < 30 && goal_z_diff < 0 || goal_z_diff < -128))
					{
						//don't jump, just walk off... doesn't help with ledges, though
						debounce = qtrue;
					}
					else if (goal_z_diff < 32 && goal_xy_dist < 200)
					{
						//what is their ideal jump height?
						NPCS.ucmd.upmove = 127;
						debounce = qtrue;
					}
					else
					{
						if (goal_z_diff > 0 || goal_xy_dist > 128)
						{
							//Fake a force-jump
							//Screw it, just do my own calc & throw
							vec3_t dest;
							VectorCopy(goal->r.currentOrigin, dest);
							if (goal == NPCS.NPC->enemy)
							{
								int sideTry = 0;
								while (sideTry < 10)
								{
									//FIXME: make it so it doesn't try the same spot again?
									trace_t trace;
									vec3_t bottom;

									if (Q_irand(0, 1))
									{
										dest[0] += NPCS.NPC->enemy->r.maxs[0] * 1.25;
									}
									else
									{
										dest[0] += NPCS.NPC->enemy->r.mins[0] * 1.25;
									}
									if (Q_irand(0, 1))
									{
										dest[1] += NPCS.NPC->enemy->r.maxs[1] * 1.25;
									}
									else
									{
										dest[1] += NPCS.NPC->enemy->r.mins[1] * 1.25;
									}
									VectorCopy(dest, bottom);
									bottom[2] -= 128;
									trap->Trace(&trace, dest, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, bottom,
										goal->s.number, NPCS.NPC->clipmask, qfalse, 0, 0);
									if (trace.fraction < 1.0f)
									{
										//hit floor, okay to land here
										break;
									}
									sideTry++;
								}
								if (sideTry >= 10)
								{
									//screw it, just jump right at him?
									VectorCopy(goal->r.currentOrigin, dest);
								}
							}
							if (Jedi_Jump(dest, goal->s.number))
							{
								{
									//FIXME: make this a function call
									int jumpAnim;
									//FIXME: this should be more intelligent, like the normal force jump anim logic
									if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT
										|| NPCS.NPC->client->pers.nextbotclass == BCLASS_BOBAFETT
										|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN1
										|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN2
										|| NPCS.NPCInfo->rank != RANK_CREWMAN && NPCS.NPCInfo->rank <= RANK_LT_JG)
									{
										//can't do acrobatics
										jumpAnim = BOTH_FORCEJUMP1;
									}
									else
									{
										jumpAnim = BOTH_FLIP_F;
									}
									NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, jumpAnim,
										SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
								}

								NPCS.NPC->client->ps.fd.forceJumpZStart = NPCS.NPC->r.currentOrigin[2];
								NPCS.NPC->client->ps.pm_flags |= PMF_JUMPING;

								NPCS.NPC->client->ps.weaponTime = NPCS.NPC->client->ps.torsoTimer;
								NPCS.NPC->client->ps.fd.forcePowersActive |= 1 << FP_LEVITATION;

								if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT
									|| NPCS.NPC->client->pers.nextbotclass == BCLASS_BOBAFETT
									|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN1
									|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN2)
								{
									G_SoundOnEnt(NPCS.NPC, CHAN_ITEM, "sound/boba/jeton.wav");
									NPCS.NPC->client->jetPackTime = level.time + Q_irand(1000, 3000);
								}
								else
								{
									if (NPCS.NPC->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
									{
										//short burst
										G_SoundOnEnt(NPCS.NPC, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
									}
									else
									{
										//holding it
										G_SoundOnEnt(NPCS.NPC, CHAN_BODY, "sound/weapons/force/jump.mp3");
									}
								}

								TIMER_Set(NPCS.NPC, "forceJumpChasing", Q_irand(2000, 3000));
								debounce = qtrue;
							}
						}
					}
					if (debounce)
					{
						//Don't jump again for another 2 to 5 seconds
						TIMER_Set(NPCS.NPC, "jumpChaseDebounce", Q_irand(2000, 5000));
						NPCS.ucmd.forwardmove = 127;
						VectorClear(NPCS.NPC->client->ps.moveDir);
						TIMER_Set(NPCS.NPC, "duck", -level.time);
						return qtrue;
					}
				}
			}
		}
	}
	return qfalse;
}

static qboolean Jedi_Jumping(const gentity_t* goal)
{
	if (!TIMER_Done(NPCS.NPC, "forceJumpChasing") && goal)
	{
		//force-jumping at the enemy
		if (NPCS.NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			//landed
			TIMER_Set(NPCS.NPC, "forceJumpChasing", 0);
		}
		else
		{
			NPC_FaceEntity(goal, qtrue);
			return qtrue;
		}
	}
	return qfalse;
}

extern void G_UcmdMoveForDir(const gentity_t* self, usercmd_t* cmd, vec3_t dir);

static void Jedi_CheckEnemyMovement(const float enemy_dist)
{
	if (!NPCS.NPC->enemy || !NPCS.NPC->enemy->client)
	{
		return;
	}

	if (!(NPCS.NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER))
	{
		if (PM_KickingAnim(NPCS.NPC->enemy->client->ps.legsAnim)
			&& NPCS.NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
			//FIXME: I'm relatively close to him
			&& (NPCS.NPC->enemy->client->ps.legsAnim == BOTH_A7_KICK_RL
				|| NPCS.NPC->enemy->client->ps.legsAnim == BOTH_A7_KICK_BF
				|| NPCS.NPC->enemy->client->ps.legsAnim == BOTH_A7_KICK_S
				|| NPCS.NPC->enemy->enemy && NPCS.NPC->enemy->enemy == NPCS.NPC))
		{
			//run into the kick!
			NPCS.ucmd.forwardmove = NPCS.ucmd.rightmove = NPCS.ucmd.upmove = 0;
			VectorClear(NPCS.NPC->client->ps.moveDir);
			Jedi_Advance();
		}
		else if (NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_A7_HILT
			|| NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_SMACK_R
			|| NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_SMACK_L
			&& NPCS.NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			//run into the hilt bash
			NPCS.ucmd.forwardmove = NPCS.ucmd.rightmove = NPCS.ucmd.upmove = 0;
			VectorClear(NPCS.NPC->client->ps.moveDir);
			Jedi_Advance();
		}
		else if ((NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_A6_FB
			|| NPCS.NPC->enemy->client->ps.torsoAnim == BOTH_A6_LR)
			&& NPCS.NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			//run into the attack
			//FIXME : only if on R/L or F/B?
			NPCS.ucmd.forwardmove = NPCS.ucmd.rightmove = NPCS.ucmd.upmove = 0;
			VectorClear(NPCS.NPC->client->ps.moveDir);
			Jedi_Advance();
		}
		else if (NPCS.NPC->enemy->enemy && NPCS.NPC->enemy->enemy == NPCS.NPC)
		{
			//enemy is mad at *me*
			if (NPCS.NPC->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSLASHDOWN1 ||
				NPCS.NPC->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSTABDOWN ||
				NPCS.NPC->enemy->client->ps.legsAnim == BOTH_FLIP_ATTACK7)
			{
				//enemy is flipping over me
				if (Q_irand(0, NPCS.NPCInfo->rank) < RANK_LT)
				{
					//be nice and stand still for him...
					NPCS.ucmd.forwardmove = NPCS.ucmd.rightmove = NPCS.ucmd.upmove = 0;
					VectorClear(NPCS.NPC->client->ps.moveDir);
					NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
					TIMER_Set(NPCS.NPC, "strafeLeft", -1);
					TIMER_Set(NPCS.NPC, "strafeRight", -1);
					TIMER_Set(NPCS.NPC, "noStrafe", Q_irand(500, 1000));
					TIMER_Set(NPCS.NPC, "movenone", Q_irand(500, 1000));
					TIMER_Set(NPCS.NPC, "movecenter", Q_irand(500, 1000));
				}
			}
			else if (NPCS.NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_BACK1
				|| NPCS.NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_RIGHT
				|| NPCS.NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_LEFT
				|| NPCS.NPC->enemy->client->ps.legsAnim == BOTH_WALL_RUN_LEFT_FLIP
				|| NPCS.NPC->enemy->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT_FLIP)
			{
				//he's flipping off a wall
				if (NPCS.NPC->enemy->client->ps.groundEntityNum == ENTITYNUM_NONE)
				{
					//still in air
					if (enemy_dist < 256)
					{
						//close
						if (Q_irand(0, NPCS.NPCInfo->rank) < RANK_LT)
						{
							//be nice and stand still for him...
							//stop current movement
							NPCS.ucmd.forwardmove = NPCS.ucmd.rightmove = NPCS.ucmd.upmove = 0;
							VectorClear(NPCS.NPC->client->ps.moveDir);
							NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
							TIMER_Set(NPCS.NPC, "strafeLeft", -1);
							TIMER_Set(NPCS.NPC, "strafeRight", -1);
							TIMER_Set(NPCS.NPC, "noStrafe", Q_irand(500, 1000));
							TIMER_Set(NPCS.NPC, "noturn", Q_irand(250, 500) * (3 - g_npcspskill.integer));

							vec3_t enemyFwd, dest, dir;

							VectorCopy(NPCS.NPC->enemy->client->ps.velocity, enemyFwd);
							VectorNormalize(enemyFwd);
							VectorMA(NPCS.NPC->enemy->r.currentOrigin, -64, enemyFwd, dest);
							VectorSubtract(dest, NPCS.NPC->r.currentOrigin, dir);
							if (VectorNormalize(dir) > 32)
							{
								G_UcmdMoveForDir(NPCS.NPC, &NPCS.ucmd, dir);
							}
							else
							{
								TIMER_Set(NPCS.NPC, "movenone", Q_irand(500, 1000));
								TIMER_Set(NPCS.NPC, "movecenter", Q_irand(500, 1000));
							}
						}
					}
				}
			}
			else if (NPCS.NPC->enemy->client->ps.legsAnim == BOTH_A2_STABBACK1 || NPCS.NPC->enemy->client->ps.legsAnim
				== BOTH_A2_STABBACK1B)
			{
				//he's stabbing backwards
				if (enemy_dist < 256 && enemy_dist > 64)
				{
					//close
					if (!in_front(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin,
						NPCS.NPC->enemy->r.currentAngles, 0.0f))
					{
						//behind him
						if (!Q_irand(0, NPCS.NPCInfo->rank))
						{
							//be nice and stand still for him...
							//stop current movement
							NPCS.ucmd.forwardmove = NPCS.ucmd.rightmove = NPCS.ucmd.upmove = 0;
							VectorClear(NPCS.NPC->client->ps.moveDir);
							NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
							TIMER_Set(NPCS.NPC, "strafeLeft", -1);
							TIMER_Set(NPCS.NPC, "strafeRight", -1);
							TIMER_Set(NPCS.NPC, "noStrafe", Q_irand(500, 1000));

							vec3_t enemyFwd, dest, dir;

							AngleVectors(NPCS.NPC->enemy->r.currentAngles, enemyFwd, NULL, NULL);
							VectorMA(NPCS.NPC->enemy->r.currentOrigin, -32, enemyFwd, dest);
							VectorSubtract(dest, NPCS.NPC->r.currentOrigin, dir);
							if (VectorNormalize(dir) > 64)
							{
								G_UcmdMoveForDir(NPCS.NPC, &NPCS.ucmd, dir);
							}
							else
							{
								TIMER_Set(NPCS.NPC, "movenone", Q_irand(500, 1000));
								TIMER_Set(NPCS.NPC, "movecenter", Q_irand(500, 1000));
							}
						}
					}
				}
			}
		}
	}
}

static void Jedi_CheckJumps(void)
{
	vec3_t jumpVel;
	trace_t trace;
	trajectory_t tr;
	vec3_t lastPos, bottom;

	if (NPCS.NPCInfo->scriptFlags & SCF_NO_ACROBATICS)
	{
		NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
		NPCS.ucmd.upmove = 0;
		return;
	}
	VectorClear(jumpVel);

	if (NPCS.NPC->client->ps.fd.forceJumpCharge)
	{
		//Com_Printf( "(%d) force jump\n", level.time );
		WP_GetVelocityForForceJump(NPCS.NPC, jumpVel, &NPCS.ucmd);
	}
	else if (NPCS.ucmd.upmove > 0)
	{
		//Com_Printf( "(%d) regular jump\n", level.time );
		VectorCopy(NPCS.NPC->client->ps.velocity, jumpVel);
		jumpVel[2] = JUMP_VELOCITY;
	}
	else
	{
		return;
	}

	//NOTE: for now, we clear ucmd.forwardmove & ucmd.rightmove while in air to avoid jumps going awry...
	if (!jumpVel[0] && !jumpVel[1]) //FIXME: && !ucmd.forwardmove && !ucmd.rightmove?
	{
		//we assume a jump straight up is safe
		//Com_Printf( "(%d) jump straight up is safe\n", level.time );
		return;
	}

	VectorCopy(NPCS.NPC->r.currentOrigin, tr.trBase);
	VectorCopy(jumpVel, tr.trDelta);
	tr.trType = TR_GRAVITY;
	tr.trTime = level.time;
	VectorCopy(NPCS.NPC->r.currentOrigin, lastPos);

	VectorClear(trace.endpos); //shut the compiler up

	//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
	for (int elapsedTime = 500; elapsedTime <= 4000; elapsedTime += 500)
	{
		vec3_t testPos;
		BG_EvaluateTrajectory(&tr, level.time + elapsedTime, testPos);
		//FIXME: account for PM_AirMove if ucmd.forwardmove and/or ucmd.rightmove is non-zero...
		if (testPos[2] < lastPos[2])
		{
			//going down, don't check for BOTCLIP
			trap->Trace(&trace, lastPos, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, testPos, NPCS.NPC->s.number,
				NPCS.NPC->clipmask, qfalse, 0, 0); //FIXME: include CONTENTS_BOTCLIP?
		}
		else
		{
			//going up, check for BOTCLIP
			trap->Trace(&trace, lastPos, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, testPos, NPCS.NPC->s.number,
				NPCS.NPC->clipmask | CONTENTS_BOTCLIP, qfalse, 0, 0);
		}
		if (trace.allsolid || trace.startsolid)
		{
			//WTF?
			//FIXME: what do we do when we start INSIDE the CONTENTS_BOTCLIP?  Do the trace again without that clipmask?
			goto jump_unsafe;
		}
		if (trace.fraction < 1.0f)
		{
			//hit something
			if (trace.contents & CONTENTS_BOTCLIP)
			{
				//hit a do-not-enter brush
				goto jump_unsafe;
			}
			//FIXME: trace through func_glass?
			break;
		}
		VectorCopy(testPos, lastPos);
	}
	//okay, reached end of jump, now trace down from here for a floor
	VectorCopy(trace.endpos, bottom);
	if (bottom[2] > NPCS.NPC->r.currentOrigin[2])
	{
		//only care about dist down from current height or lower
		bottom[2] = NPCS.NPC->r.currentOrigin[2];
	}
	else if (NPCS.NPC->r.currentOrigin[2] - bottom[2] > 400)
	{
		//whoa, long drop, don't do it!
		//probably no floor at end of jump, so don't jump
		goto jump_unsafe;
	}
	bottom[2] -= 128;
	trap->Trace(&trace, trace.endpos, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, bottom, NPCS.NPC->s.number,
		NPCS.NPC->clipmask, qfalse, 0, 0);
	if (trace.allsolid || trace.startsolid || trace.fraction < 1.0f)
	{
		//hit ground!
		if (trace.entityNum < ENTITYNUM_WORLD)
		{
			//landed on an ent
			const gentity_t* groundEnt = &g_entities[trace.entityNum];
			if (groundEnt->r.svFlags & SVF_GLASS_BRUSH)
			{
				//don't land on breakable glass!
				goto jump_unsafe;
			}
		}
		//Com_Printf( "(%d) jump is safe\n", level.time );
		return;
	}
jump_unsafe:
	//probably no floor at end of jump, so don't jump
	NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
	NPCS.ucmd.upmove = 0;
}

extern void RT_FireDecide(void);

void RT_CheckJump(void)
{
	int jumpEntNum = ENTITYNUM_NONE;
	vec3_t jumpPos = { 0, 0, 0 };

	if (!NPCS.NPCInfo->goalEntity)
	{
		if (NPCS.NPC->enemy)
		{
			//FIXME: debounce this?
			if (TIMER_Done(NPCS.NPC, "roamTime")
				&& Q_irand(0, 9))
			{
				//okay to try to find another spot to be
				int cpFlags = CP_CLEAR | CP_HAS_ROUTE; //must have a clear shot at enemy
				const float enemyDistSq = DistanceHorizontalSquared(NPCS.NPC->r.currentOrigin,
					NPCS.NPC->enemy->r.currentOrigin);
				//FIXME: base these ranges on weapon
				if (enemyDistSq > 2048 * 2048)
				{
					//hmm, close in?
					cpFlags |= CP_APPROACH_ENEMY;
				}
				else if (enemyDistSq < 256 * 256)
				{
					//back off!
					cpFlags |= CP_RETREAT;
				}
				int sendFlags = cpFlags;
				int cp = NPC_FindCombatPointRetry(NPCS.NPC->r.currentOrigin,
					NPCS.NPC->r.currentOrigin,
					NPCS.NPC->r.currentOrigin,
					&sendFlags,
					256,
					NPCS.NPCInfo->lastFailedCombatPoint);
				if (cp == -1)
				{
					//try again, no route needed since we can rocket-jump to it!
					cpFlags &= ~CP_HAS_ROUTE;
					cp = NPC_FindCombatPointRetry(NPCS.NPC->r.currentOrigin,
						NPCS.NPC->r.currentOrigin,
						NPCS.NPC->r.currentOrigin,
						&cpFlags,
						256,
						NPCS.NPCInfo->lastFailedCombatPoint);
				}
				if (cp != -1)
				{
					NPC_SetMoveGoal(NPCS.NPC, level.combatPoints[cp].origin, 8, qtrue, cp, NULL);
				}
				else
				{
					//FIXME: okay to do this if have good close-range weapon...
					//FIXME: should we really try to go right for him?!
					//NPCInfo->goalEntity = NPC->enemy;
					jumpEntNum = NPCS.NPC->enemy->s.number;
					VectorCopy(NPCS.NPC->enemy->r.currentOrigin, jumpPos);
					//return;
				}
				TIMER_Set(NPCS.NPC, "roamTime", Q_irand(3000, 12000));
			}
			else
			{
				//FIXME: okay to do this if have good close-range weapon...
				//FIXME: should we really try to go right for him?!
				//NPCInfo->goalEntity = NPC->enemy;
				jumpEntNum = NPCS.NPC->enemy->s.number;
				VectorCopy(NPCS.NPC->enemy->r.currentOrigin, jumpPos);
			}
		}
		else
		{
			return;
		}
	}
	else
	{
		jumpEntNum = NPCS.NPCInfo->goalEntity->s.number;
		VectorCopy(NPCS.NPCInfo->goalEntity->r.currentOrigin, jumpPos);
	}
	vec3_t vec2Goal;
	VectorSubtract(jumpPos, NPCS.NPC->r.currentOrigin, vec2Goal);
	if (fabs(vec2Goal[2]) < 32)
	{
		//not a big height diff, see how far it is
		vec2Goal[2] = 0;
		if (VectorLengthSquared(vec2Goal) < 256 * 256)
		{
			//too close!  Don't rocket-jump to it...
			return;
		}
	}
	//If we can't get straight at him
	if (!Jedi_ClearPathToSpot(jumpPos, jumpEntNum))
	{
		//hunt him down
		if ((NPC_ClearLOS4(NPCS.NPC->enemy) || NPCS.NPCInfo->enemyLastSeenTime > level.time - 500) &&
			NPC_FaceEnemy(qtrue))
		{
			if (Jedi_TryJump(NPCS.NPC->enemy))
			{
				//what about jumping to his enemyLastSeenLocation?
				return;
			}
		}

		if (Jedi_Hunt() && !(NPCS.NPCInfo->aiFlags & NPCAI_BLOCKED))
		{
			//can macro-navigate to him
		}
		//FIXME: try to find a waypoint that can see enemy, jump from there
		//if (STEER::HasBeenBlockedFor(NPC, 2000))
		//{//try to jump to the blockedTargetPosition
		//	if (NPC_TryJump(NPCS.NPCInfo->blockedTargetPosition))	// Rocket Trooper
		//	{//just do the jetpack effect for a litte bit
		//		RT_JetPackEffect(Q_irand(800, 1500));
		//	}
		//}
	}
}

static void Jedi_Combat(void)
{
	vec3_t enemy_dir, enemy_movedir, enemy_dest;
	float enemy_dist, enemy_movespeed;

	//See where enemy will be 300 ms from now
	jedi_set_enemy_info(enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300);

	if (NPC_Jumping())
	{
		//I'm in the middle of a jump, so just see if I should attack
		Jedi_AttackDecide(enemy_dist);
		return;
	}
	if (TIMER_Done(NPCS.NPC, "allyJediDelay"))
	{
		//racc - we're not holding back
		if (!(NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_GRIP) || NPCS.NPC->client->ps.fd.forcePowerLevel[
			FP_GRIP] < FORCE_LEVEL_2)
		{
			//not gripping
			//If we can't get straight at him
			if (!Jedi_ClearPathToSpot(enemy_dest, NPCS.NPC->enemy->s.number))
			{
				//hunt him down
				//Com_Printf( "No Clear Path\n" );
				if ((NPC_ClearLOS4(NPCS.NPC->enemy) || NPCS.NPCInfo->enemyLastSeenTime > level.time - 500) &&
					NPC_FaceEnemy(qtrue)) //( NPCInfo->rank == RANK_CREWMAN || NPCInfo->rank > RANK_LT_JG ) &&
				{
					if (NPCS.NPC->client && NPCS.NPC->client->NPC_class == CLASS_BOBAFETT || NPCS.NPC->client && NPCS.
						NPC->client->NPC_class == CLASS_MANDO)
					{
						Boba_FireDecide();
					}
				}

				//Check for evasion
				if (TIMER_Done(NPCS.NPC, "parryTime"))
				{
					//finished parrying
					if (NPCS.NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
						NPCS.NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN)
					{
						//wasn't blocked myself
						NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
					}
				}
				if (Jedi_Hunt() && !(NPCS.NPCInfo->aiFlags & NPCAI_BLOCKED))
				{
					//can macro-navigate to him
					if (enemy_dist < 384 && !Q_irand(0, 10) && NPCS.NPCInfo->blockedSpeechDebounceTime < level.time &&
						jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] < level.time && !NPC_ClearLOS4(
							NPCS.NPC->enemy))
					{
						if (!npc_is_dark_jedi(NPCS.NPC))
						{
							G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_GIVEUP1, EV_GIVEUP4), 2000);
						}
						else
						{
							G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_JLOST1, EV_JLOST3), 10000);
						}
						jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->blockedSpeechDebounceTime =
							level.time + 3000;
					}
					if (NPCS.NPC->client && NPCS.NPC->client->NPC_class == CLASS_BOBAFETT || NPCS.NPC->client && NPCS.
						NPC->client->NPC_class == CLASS_MANDO)
					{
						Boba_FireDecide();
					}
					return;
				}
				//FIXME: try to find a waypoint that can see enemy, jump from there
				if (NPCS.NPCInfo->aiFlags & NPCAI_BLOCKED)
				{
					//try to jump to the blockedDest
					gentity_t* tempGoal = G_Spawn(); //ugh, this is NOT good...?
					G_SetOrigin(tempGoal, NPCS.NPCInfo->blockedDest);
					trap->LinkEntity((sharedEntity_t*)tempGoal);
					if (Jedi_TryJump(tempGoal))
					{
						//going to jump to the dest
						G_FreeEntity(tempGoal);
						return;
					}
					G_FreeEntity(tempGoal);
				}
			}
		}
		//else, we can see him or we can't track him at all

		//every few seconds, decide if we should we advance or retreat?
		Jedi_CombatTimersUpdate(enemy_dist);
		//We call this even if lost enemy to keep him moving and to update the taunting behavior
		//maintain a distance from enemy appropriate for our aggression level
		Jedi_CombatDistance(enemy_dist);
	}

	if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT && NPCS.NPC->client->NPC_class != CLASS_MANDO)
	{
		//Update our seen enemy position
		if (!NPCS.NPC->enemy->client || NPCS.NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE && NPCS.NPC->
			client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			VectorCopy(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPCInfo->enemyLastSeenLocation);
		}
		NPCS.NPCInfo->enemyLastSeenTime = level.time;
	}

	//Turn to face the enemy
	if (TIMER_Done(NPCS.NPC, "noturn") && !NPC_Jumping())
	{
		Jedi_FaceEnemy(qtrue);
	}
	NPC_UpdateAngles(qtrue, qtrue);

	//Check for evasion
	if (TIMER_Done(NPCS.NPC, "parryTime"))
	{
		//finished parrying
		if (NPCS.NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
			NPCS.NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN)
		{
			//wasn't blocked myself
			NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}
	if (NPCS.NPC->enemy->s.weapon == WP_SABER)
	{
		Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
	}
	else
	{
		if (!in_camera)
		{
			NPC_CheckEvasion();
		}

		if (NPC_IsJedi(NPCS.NPC))
		{
			// Do deflect taunt...
			G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_DEFLECT1, EV_DEFLECT3), 5000 + Q_irand(0, 15000));
		}
	}

	//apply strafing/walking timers, etc.
	Jedi_TimersApply();

	if (TIMER_Done(NPCS.NPC, "allyJediDelay"))
	{
		//not holding back
		if ((!NPCS.NPC->client->ps.saberInFlight //saber in had
			|| NPCS.NPC->client->ps.fd.saberAnimLevel == SS_DUAL
			&& NPCS.NPC->client->ps.saberHolstered != 2) //or we have another saber in our hands
			&& (!(NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_GRIP)
				|| NPCS.NPC->client->ps.fd.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2))
		{
			//not throwing saber or using force grip
			//see if we can attack
			if (!Jedi_AttackDecide(enemy_dist))
			{
				//we're not attacking, decide what else to do
				Jedi_CombatIdle(enemy_dist);
			}
			else
			{
				//we are attacking
				TIMER_Set(NPCS.NPC, "taunting", -level.time);
			}
		}
		else
		{
		}

		if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
		{
			Boba_FireDecide();
		}
		else if (NPCS.NPC->client->NPC_class == CLASS_MANDO)
		{
			Boba_FireDecide();
		}
		else if (NPCS.NPC->client->NPC_class == CLASS_ROCKETTROOPER)
		{
			RT_FireDecide();
		}
	}
	//Check for certain enemy special moves
	Jedi_CheckEnemyMovement(enemy_dist);
	//Make sure that we don't jump off ledges over long drops
	Jedi_CheckJumps();
	//Just make sure we don't strafe into walls or off cliffs

	if (VectorCompare(NPCS.NPC->client->ps.moveDir, vec3_origin) //stomped the NAV system's moveDir
		&& !NPC_MoveDirClear(NPCS.ucmd.forwardmove, NPCS.ucmd.rightmove, qtrue)) //check ucmd-driven movement
	{
		//uh-oh, we are going to fall or hit something
		//reset the timers.
		TIMER_Set(NPCS.NPC, "strafeLeft", 0);
		TIMER_Set(NPCS.NPC, "strafeRight", 0);
	}
}

/*
==========================================================================================
EXTERNALLY CALLED BEHAVIOR STATES
==========================================================================================
*/

/*
-------------------------
NPC_Jedi_Pain
-------------------------
*/

void NPC_Jedi_Pain(gentity_t* self, gentity_t* attacker, const int damage)
{
	const int mod = gPainMOD;
	const gentity_t* other = attacker;
	vec3_t point;

	VectorCopy(gPainPoint, point);

	if (other->s.weapon == WP_SABER)
	{
		//back off
		TIMER_Set(self, "parryTime", -1);

		if (self->client->NPC_class == CLASS_DESANN
			|| !Q_stricmp("Yoda", self->NPC_type)
			|| !Q_stricmp("T_Palpatine_sith", self->NPC_type)
			|| !Q_stricmp("RebornBoss", self->NPC_type)
			|| !Q_stricmp("jedi_kdm1", self->NPC_type)
			|| !Q_stricmp("T_Yoda", self->NPC_type))
		{
			//less for Desann
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3 - g_npcspskill.integer) * 50;
		}
		else if (self->NPC->rank >= RANK_LT_JG)
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3 - g_npcspskill.integer) * 100;
			//300
		}
		else
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3 - g_npcspskill.integer) * 200;
			//500
		}
		if (!Q_irand(0, 3))
		{
			//ouch... maybe switch up which saber power level we're using
			Jedi_AdjustSaberAnimLevel(self, Q_irand(SS_FAST, SS_TAVION));
		}
		if (!Q_irand(0, 1))
		{
			Jedi_Aggression(self, -1);
		}
		if (d_JediAI.integer || g_DebugSaberCombat.integer)
		{
			Com_Printf("(%d) PAIN: agg %d, no parry until %d\n", level.time, self->NPC->stats.aggression,
				level.time + 500);
		}
		//for testing only
		// Figure out what quadrant the hit was in.
		if (d_JediAI.integer || g_DebugSaberCombat.integer)
		{
			vec3_t diff, fwdangles, right;

			VectorSubtract(point, self->client->renderInfo.eyePoint, diff);
			diff[2] = 0;
			fwdangles[1] = self->client->ps.viewangles[1];
			AngleVectors(fwdangles, NULL, right, NULL);
			const float rightdot = DotProduct(right, diff);
			const float zdiff = point[2] - self->client->renderInfo.eyePoint[2];

			Com_Printf("(%d) saber hit at height %4.2f, zdiff: %4.2f, rightdot: %4.2f\n", level.time,
				point[2] - self->r.absmin[2], zdiff, rightdot);
		}
	}
	else
	{
		//attack
		Jedi_Aggression(self, 1);
	}

	self->NPC->enemyCheckDebounceTime = 0;

	WP_ForcePowerStop(self, FP_GRIP);

	NPC_Pain(self, attacker, damage);

	if (!damage && self->health > 0)
	{
		//FIXME: better way to know I was pushed
		if (!npc_is_dark_jedi(self))
		{
			G_AddVoiceEvent(self, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
		}
		else
		{
			G_AddVoiceEvent(self, Q_irand(EV_ANGER1, EV_ANGER3), 2000);
		}
	}

	//drop me from the ceiling if I'm on it
	if (Jedi_WaitingAmbush(self))
	{
		self->client->noclip = qfalse;
	}
	if (self->client->ps.legsAnim == BOTH_CEILING_CLING)
	{
		NPC_SetAnim(self, SETANIM_LEGS, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	if (self->client->ps.torsoAnim == BOTH_CEILING_CLING)
	{
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}

	//check special defenses
	if (other
		&& other->client
		&& !OnSameTeam(self, other))
	{
		//hit by a client
		if (mod == MOD_FORCE_DARK
			|| mod == MOD_FORCE_LIGHTNING)
		{
			//see if we should turn on absorb
			if ((self->client->ps.fd.forcePowersKnown & 1 << FP_ABSORB) != 0
				&& (self->client->ps.fd.forcePowersActive & 1 << FP_ABSORB) == 0)
			{
				//know absorb and not already using it
				if (other->s.number >= MAX_CLIENTS //enemy is an NPC
					|| Q_irand(0, g_npcspskill.integer + 1)) //enemy is player
				{
					if (Q_irand(0, self->NPC->rank) > RANK_ENSIGN)
					{
						if (!Q_irand(0, 5))
						{
							ForceAbsorb(self);
						}
					}
				}
			}
		}
		else if (damage > Q_irand(5, 20))
		{
			//respectable amount of normal damage
			if ((self->client->ps.fd.forcePowersKnown & 1 << FP_PROTECT) != 0
				&& (self->client->ps.fd.forcePowersActive & 1 << FP_PROTECT) == 0)
			{
				//know protect and not already using it
				if (other->s.number >= MAX_CLIENTS //enemy is an NPC
					|| Q_irand(0, g_npcspskill.integer + 1)) //enemy is player
				{
					if (Q_irand(0, self->NPC->rank) > RANK_ENSIGN)
					{
						if (!Q_irand(0, 1))
						{
							if (other->s.number < MAX_CLIENTS
								&& (self->NPC->aiFlags & NPCAI_BOSS_CHARACTER
									|| self->client->NPC_class == CLASS_SHADOWTROOPER)
								&& Q_irand(0, 6 - g_npcspskill.integer))
							{
							}
							else
							{
								ForceProtect(self);
							}
						}
					}
				}
			}
		}
	}
}

qboolean Jedi_CheckDanger(void)
{
	const int alert_event = NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_MINOR);

	if (level.alertEvents[alert_event].level >= AEL_DANGER)
	{
		//run away!
		if (!level.alertEvents[alert_event].owner
			|| !level.alertEvents[alert_event].owner->client
			|| level.alertEvents[alert_event].owner != NPCS.NPC && level.alertEvents[alert_event].owner->client->
			playerTeam != NPCS.NPC->client->playerTeam)
		{
			//no owner
			return qfalse;
		}
		G_SetEnemy(NPCS.NPC, level.alertEvents[alert_event].owner);
		NPCS.NPCInfo->enemyLastSeenTime = level.time;
		TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(500, 2500));
		return qtrue;
	}
	return qfalse;
}

qboolean Jedi_CheckAmbushPlayer(void)
{
	float target_dist;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t* player = &g_entities[i];

		if (!player || !player->client)
		{
			continue;
		}

		if (!NPC_ValidEnemy(player))
		{
			continue;
		}

		if (NPCS.NPC->client->ps.powerups[PW_CLOAKED] || !NPC_SomeoneLookingAtMe(NPCS.NPC))
			//rwwFIXMEFIXME: Need to pay attention to who is under crosshair for each player or something.
		{
			//if I'm not cloaked and the player's crosshair is on me, I will wake up, otherwise do this stuff down here...
			if (!trap->InPVS(player->r.currentOrigin, NPCS.NPC->r.currentOrigin))
			{
				//must be in same room
				continue;
			}
			if (!NPCS.NPC->client->ps.powerups[PW_CLOAKED])
			{
				NPC_SetLookTarget(NPCS.NPC, 0, 0);
			}
			const float zDiff = NPCS.NPC->r.currentOrigin[2] - player->r.currentOrigin[2];
			if (zDiff <= 0 || zDiff > 512)
			{
				//never ambush if they're above me or way way below me
				continue;
			}

			//If the target is this close, then wake up regardless
			if ((target_dist = DistanceHorizontalSquared(player->r.currentOrigin, NPCS.NPC->r.currentOrigin)) > 4096)
			{
				//closer than 64 - always ambush
				if (target_dist > 147456)
				{
					//> 384, not close enough to ambush
					continue;
				}
				//Check FOV first
				if (NPCS.NPC->client->ps.powerups[PW_CLOAKED])
				{
					if (InFOV(player, NPCS.NPC, 30, 90) == qfalse)
					{
						continue;
					}
				}
				else
				{
					if (InFOV(player, NPCS.NPC, 45, 90) == qfalse)
					{
						continue;
					}
				}
			}

			if (!NPC_ClearLOS4(player))
			{
				continue;
			}
		}

		//Got him, return true;
		G_SetEnemy(NPCS.NPC, player);
		NPCS.NPCInfo->enemyLastSeenTime = level.time;
		TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(500, 2500));
		return qtrue;
	}

	//Didn't get anyone.
	return qfalse;
}

void Jedi_Ambush(gentity_t* self)
{
	self->client->noclip = qfalse;
	//	self->client->ps.pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
	NPC_SetAnim(self, SETANIM_BOTH, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	self->client->ps.weaponTime = self->client->ps.torsoTimer; //NPC->client->ps.torsoTimer; //what the?
	if (self->client->NPC_class != CLASS_BOBAFETT
		&& self->client->NPC_class != CLASS_ROCKETTROOPER)
	{
		WP_ActivateSaber(self);
	}
	Jedi_Decloak(self);
	G_AddVoiceEvent(self, Q_irand(EV_ANGER1, EV_ANGER3), 1000);
}

qboolean Jedi_WaitingAmbush(const gentity_t* self)
{
	if (self->spawnflags & JSF_AMBUSH && self->client->noclip)
	{
		return qtrue;
	}
	return qfalse;
}

/*
-------------------------
Jedi_Patrol
-------------------------
*/

static void Jedi_Patrol(void)
{
	NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;

	if (Jedi_WaitingAmbush(NPCS.NPC))
	{
		//hiding on the ceiling
		NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_CEILING_CLING, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		if (NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			//look for enemies
			if (Jedi_CheckAmbushPlayer() || Jedi_CheckDanger())
			{
				//found him!
				Jedi_Ambush(NPCS.NPC);
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}
	else if (NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
	{
		//look for enemies
		gentity_t* best_enemy = NULL;
		float best_enemy_dist = Q3_INFINITE;
		for (int i = 0; i < ENTITYNUM_WORLD; i++)
		{
			gentity_t* enemy = &g_entities[i];
			if (enemy && enemy->client && NPC_ValidEnemy(enemy))
			{
				if (trap->InPVS(NPCS.NPC->r.currentOrigin, enemy->r.currentOrigin))
				{
					//we could potentially see him
					const float enemy_dist = DistanceSquared(NPCS.NPC->r.currentOrigin, enemy->r.currentOrigin);
					if (enemy->s.eType == ET_PLAYER || enemy_dist < best_enemy_dist)
					{
						//if the enemy is close enough, or threw his saber, take him as the enemy
						//FIXME: what if he throws a thermal detonator?
						if (enemy_dist < 220 * 220 || NPCS.NPCInfo->investigateCount >= 3 && !NPCS.NPC->client->ps.
							saberHolstered)
						{
							G_SetEnemy(NPCS.NPC, enemy);
							NPCS.NPCInfo->stats.aggression = 3;
							break;
						}
						if (enemy->client->ps.saberInFlight && !enemy->client->ps.saberHolstered)
						{
							vec3_t saberDir2Me;
							vec3_t saberMoveDir;
							const gentity_t* saber = &g_entities[enemy->client->ps.saberEntityNum];
							VectorSubtract(NPCS.NPC->r.currentOrigin, saber->r.currentOrigin, saberDir2Me);
							const float saberDist = VectorNormalize(saberDir2Me);
							VectorCopy(saber->s.pos.trDelta, saberMoveDir);
							VectorNormalize(saberMoveDir);
							if (DotProduct(saberMoveDir, saberDir2Me) > 0.5)
							{
								//it's heading towards me
								if (saberDist < 200)
								{
									//incoming!
									G_SetEnemy(NPCS.NPC, enemy);
									//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be auto now
									NPCS.NPCInfo->stats.aggression = 3;
									break;
								}
							}
						}
						best_enemy_dist = enemy_dist;
						best_enemy = enemy;
					}
				}
			}
		}
		if (!NPCS.NPC->enemy)
		{
			//still not mad
			if (!best_enemy)
			{
				//Com_Printf( "(%d) drop agg - no enemy (patrol)\n", level.time );
				Jedi_AggressionErosion(-1);
			}
			else
			{
				//have one to consider
				if (NPC_ClearLOS4(best_enemy))
				{
					//we have a clear (of architecture) LOS to him
					if (NPCS.NPCInfo->aiFlags & NPCAI_NO_JEDI_DELAY)
					{
						//just get mad right away
						if (DistanceHorizontalSquared(NPCS.NPC->r.currentOrigin, best_enemy->r.currentOrigin) < 1024 *
							1024)
						{
							G_SetEnemy(NPCS.NPC, best_enemy);
							NPCS.NPCInfo->stats.aggression = 20;
						}
					}
					else if (best_enemy->s.number)
					{
						//just attack
						G_SetEnemy(NPCS.NPC, best_enemy);
						NPCS.NPCInfo->stats.aggression = 3;
					}
					else if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT && NPCS.NPC->client->NPC_class != CLASS_MANDO
						&& NPCS.NPC->client->pers.nextbotclass != BCLASS_BOBAFETT
						&& NPCS.NPC->client->pers.nextbotclass != BCLASS_MANDOLORIAN1
						&& NPCS.NPC->client->pers.nextbotclass != BCLASS_MANDOLORIAN2)
					{
						//the player, toy with him
						//get progressively more interested over time
						if (TIMER_Done(NPCS.NPC, "watchTime"))
						{
							//we want to pick him up in stages
							if (TIMER_Get(NPCS.NPC, "watchTime") == -1)
							{
								//this is the first time, we'll ignore him for a couple seconds
								TIMER_Set(NPCS.NPC, "watchTime", Q_irand(3000, 5000));
								goto finish;
							}
							//okay, we've ignored him, now start to notice him
							if (!NPCS.NPCInfo->investigateCount)
							{
								if (!npc_is_dark_jedi(NPCS.NPC))
								{
									G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_SUSPICIOUS1, EV_SUSPICIOUS5), 2000);
								}
								else
								{
									G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_JDETECTED1, EV_JDETECTED3), 10000);
								}
							}
							NPCS.NPCInfo->investigateCount++;
							TIMER_Set(NPCS.NPC, "watchTime", Q_irand(4000, 10000));
						}
						//while we're waiting, do what we need to do
						if (best_enemy_dist < 440 * 440 || NPCS.NPCInfo->investigateCount >= 2)
						{
							//stage three: keep facing him
							NPC_FaceEntity(best_enemy, qtrue);
							if (best_enemy_dist < 330 * 330)
							{
								//stage four: turn on the saber
								if (!NPCS.NPC->client->ps.saberInFlight)
								{
									WP_ActivateSaber(NPCS.NPC);
								}
							}
						}
						else if (best_enemy_dist < 550 * 550 || NPCS.NPCInfo->investigateCount == 1)
						{
							//stage two: stop and face him every now and then
							if (TIMER_Done(NPCS.NPC, "watchTime"))
							{
								NPC_FaceEntity(best_enemy, qtrue);
							}
						}
						else
						{
							//stage one: look at him.
							NPC_SetLookTarget(NPCS.NPC, best_enemy->s.number, 0);
						}
					}
				}
				else if (TIMER_Done(NPCS.NPC, "watchTime"))
				{
					//haven't seen him in a bit, clear the lookTarget
					NPC_ClearLookTarget(NPCS.NPC);
				}
			}
		}
	}
finish:
	//If we have somewhere to go, then do that
	if (UpdateGoal())
	{
		NPCS.ucmd.buttons |= BUTTON_WALKING;
		//Jedi_Move( NPCInfo->goalEntity );
		NPC_MoveToGoal(qtrue);
	}

	NPC_UpdateAngles(qtrue, qtrue);

	if (NPCS.NPC->enemy)
	{
		//just picked one up
		NPCS.NPCInfo->enemyCheckDebounceTime = level.time + Q_irand(3000, 10000);
	}
}

qboolean Jedi_CanPullBackSaber(const gentity_t* self)
{
	if (self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN && !TIMER_Done(self, "parryTime"))
	{
		return qfalse;
	}

	if (self->client->NPC_class == CLASS_SHADOWTROOPER
		|| self->client->NPC_class == CLASS_ALORA
		|| self->NPC && self->NPC->aiFlags & NPCAI_BOSS_CHARACTER)
	{
		return qtrue;
	}

	if (self->painDebounceTime > level.time)
	{
		return qfalse;
	}

	return qtrue;
}

/*
-------------------------
NPC_BSJedi_FollowLeader
-------------------------
*/
void NPC_BSJedi_FollowLeader(void)
{
	NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
	if (!NPCS.NPC->enemy)
	{
		Jedi_AggressionErosion(-1);
	}

	//did we drop our saber?  If so, go after it!
	if (NPCS.NPC->client->ps.saberInFlight)
	{
		//saber is not in hand
		if (NPCS.NPC->client->ps.saberEntityNum < ENTITYNUM_NONE && NPCS.NPC->client->ps.saberEntityNum > 0)
			//player is 0
		{
			//
			if (g_entities[NPCS.NPC->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY)
			{
				//fell to the ground, try to pick it up...
				if (Jedi_CanPullBackSaber(NPCS.NPC))
				{
					//FIXME: if it's on the ground and we just pulled it back to us, should we
					//		stand still for a bit to make sure it gets to us...?
					//		otherwise we could end up running away from it while it's on its
					//		way back to us and we could lose it again.
					NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
					NPCS.NPCInfo->goalEntity = &g_entities[NPCS.NPC->client->ps.saberEntityNum];
					NPCS.ucmd.buttons &= ~BUTTON_ALT_ATTACK;
					NPCS.ucmd.buttons |= BUTTON_ATTACK;
					if (NPCS.NPC->enemy && NPCS.NPC->enemy->health > 0)
					{
						//get our saber back NOW!
						if (!NPC_MoveToGoal(qtrue)) //Jedi_Move( NPCInfo->goalEntity, qfalse );
						{
							//can't nav to it, try jumping to it
							NPC_FaceEntity(NPCS.NPCInfo->goalEntity, qtrue);
							if (!Jedi_TryJump(NPCS.NPCInfo->goalEntity))
							{
								Jedi_Move(NPCS.NPCInfo->goalEntity, qfalse);
							}
						}
						NPC_UpdateAngles(qtrue, qtrue);
						return;
					}
				}
			}
		}
	}

	if (NPCS.NPCInfo->goalEntity)
	{
		trace_t trace;

		if (Jedi_Jumping(NPCS.NPCInfo->goalEntity))
		{
			//in mid-jump
			return;
		}

		if (!NAV_CheckAhead(NPCS.NPC, NPCS.NPCInfo->goalEntity->r.currentOrigin, &trace,
			NPCS.NPC->clipmask & ~CONTENTS_BODY | CONTENTS_BOTCLIP))
		{
			//can't get straight to him
			if (NPC_ClearLOS4(NPCS.NPCInfo->goalEntity) && NPC_FaceEntity(NPCS.NPCInfo->goalEntity, qtrue))
			{
				//no line of sight
				if (Jedi_TryJump(NPCS.NPCInfo->goalEntity))
				{
					//started a jump
					return;
				}
				Jedi_Move(NPCS.NPCInfo->goalEntity, qfalse);
				return;
			}
		}
		if (NPCS.NPCInfo->aiFlags & NPCAI_BLOCKED)
		{
			//try to jump to the blockedDest
			if (fabs(NPCS.NPCInfo->blockedDest[2] - NPCS.NPC->r.currentOrigin[2]) > 64)
			{
				gentity_t* tempGoal = G_Spawn(); //ugh, this is NOT good...?
				G_SetOrigin(tempGoal, NPCS.NPCInfo->blockedDest);
				trap->LinkEntity((sharedEntity_t*)tempGoal);
				TIMER_Set(NPCS.NPC, "jumpChaseDebounce", -1);
				if (Jedi_TryJump(tempGoal))
				{
					//going to jump to the dest
					G_FreeEntity(tempGoal);
					return;
				}
				G_FreeEntity(tempGoal);
			}
		}
	}
	//try normal movement
	NPC_BSFollowLeader();

	if (!NPCS.NPC->enemy &&
		NPCS.NPC->health < NPCS.NPC->client->pers.maxHealth &&
		(NPCS.NPC->client->ps.fd.forcePowersKnown & 1 << FP_HEAL) != 0 &&
		(NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_HEAL) == 0 &&
		TIMER_Done(NPCS.NPC, "FollowHealDebouncer"))
	{
		if (Q_irand(0, 3) == 0)
		{
			TIMER_Set(NPCS.NPC, "FollowHealDebouncer", Q_irand(12000, 18000));
			ForceHeal(NPCS.NPC);

			if (NPC_IsJedi(NPCS.NPC))
			{
				// Do deflect taunt...
				G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_GLOAT1, EV_GLOAT3), 5000 + Q_irand(0, 15000));
			}
		}
		else
		{
			TIMER_Set(NPCS.NPC, "FollowHealDebouncer", Q_irand(1000, 2000));
		}
	}
}

qboolean Jedi_CheckKataAttack(void)
{
	//decide if we want to do a kata and do it
	if (NPCS.NPCInfo->rank == RANK_CAPTAIN)
	{
		//only top-level guys and bosses do this
		if (NPCS.ucmd.buttons & BUTTON_ATTACK)
		{
			//attacking
			if (!(NPCS.ucmd.buttons & BUTTON_ALT_ATTACK))
			{
				//not already going to do a kata move somehow
				if (NPCS.NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					//on the ground
					if (NPCS.ucmd.upmove <= 0 && NPCS.NPC->client->ps.fd.forceJumpCharge <= 0)
					{
						//not going to try to jump
						if (Q_irand(0, g_npcspskill.integer + 1) //50% chance on easy, 66% on medium, 75% on hard
							&& !Q_irand(0, 9)) //10% chance overall
						{
							//base on skill level
							NPCS.ucmd.upmove = 0;
							VectorClear(NPCS.NPC->client->ps.moveDir);
							NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
							return qtrue;
						}
					}
				}
			}
		}
	}
	return qfalse;
}

/*
-------------------------
Jedi_Attack
-------------------------
*/

static void Jedi_Attack(void)
{
	//Don't do anything if we're in a pain anim
	if (NPCS.NPC->painDebounceTime > level.time)
	{
		if (Q_irand(0, 1))
		{
			Jedi_FaceEnemy(qtrue);
		}
		NPC_UpdateAngles(qtrue, qtrue);

		if (NPCS.NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB)
		{
			//see if we grabbed enemy
			if (NPCS.NPC->client->ps.torsoTimer <= 200)
			{
				if (Kyle_CanDoGrab()
					&& NPC_EnemyRangeFromBolt(0) < 88.0f)
				{
					//grab him!
					Kyle_GrabEnemy();
					return;
				}
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				NPCS.NPC->client->ps.weaponTime = NPCS.NPC->client->ps.torsoTimer;
				return;
			}
			//else just sit here?
		}
		return;
	}

	if (NPCS.NPC->client->ps.saberLockTime > level.time)
	{
		//FIXME: maybe if I'm losing I should try to force-push out of it?  Very rarely, though...
		if (NPCS.NPC->client->ps.fd.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2
			&& NPCS.NPC->client->ps.saberLockTime < level.time + 5000
			&& !Q_irand(0, 10))
		{
			ForceThrow(NPCS.NPC, qfalse);
		}
		//based on my skill, hit attack button every other to every several frames in order to push enemy back
		else
		{
			float chance;

			if (NPCS.NPC->client->NPC_class == CLASS_DESANN
				|| !Q_stricmp("T_Yoda", NPCS.NPC->NPC_type)
				|| !Q_stricmp("RebornBoss", NPCS.NPC->NPC_type)
				|| !Q_stricmp("jedi_kdm1", NPCS.NPC->NPC_type)
				|| !Q_stricmp("T_Palpatine_sith", NPCS.NPC->NPC_type)
				|| !Q_stricmp("Yoda", NPCS.NPC->NPC_type))
			{
				if (g_npcspskill.integer)
				{
					chance = 4.0f; //he pushes *hard*
				}
				else
				{
					chance = 3.0f; //he pushes *hard*
				}
			}
			else if (NPCS.NPC->client->NPC_class == CLASS_TAVION
				|| NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER
				|| NPCS.NPC->client->NPC_class == CLASS_ALORA
				|| NPCS.NPC->client->NPC_class == CLASS_KYLE && NPCS.NPC->spawnflags & 1)
			{
				chance = 2.0f + g_npcspskill.value; //from 2 to 4
			}
			else
			{
				//the escalation in difficulty is nice, here, but cap it so it doesn't get *impossible* on hard
				const float maxChance = (float)RANK_LT / 2.0f + 3.0f; //5?
				if (!g_npcspskill.value)
				{
					chance = (float)NPCS.NPCInfo->rank / 2.0f;
				}
				else
				{
					chance = (float)NPCS.NPCInfo->rank / 2.0f + 1.0f;
				}
				if (chance > maxChance)
				{
					chance = maxChance;
				}
			}
			if (NPCS.NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER)
			{
				chance += Q_irand(0, 2);
			}
			else if (NPCS.NPCInfo->aiFlags & NPCAI_SUBBOSS_CHARACTER)
			{
				chance += Q_irand(-1, 1);
			}
			if (flrand(-4.0f, chance) >= 0.0f && !(NPCS.NPC->client->ps.pm_flags & PMF_ATTACK_HELD))
			{
				NPCS.ucmd.buttons &= ~BUTTON_ALT_ATTACK;
				NPCS.ucmd.buttons |= BUTTON_ATTACK;
			}
		}
		NPC_UpdateAngles(qtrue, qtrue);
		return;
	}
	//did we drop our saber?  If so, go after it!
	if (NPCS.NPC->client->ps.saberInFlight)
	{
		//saber is not in hand
		if (!NPCS.NPC->client->ps.saberEntityNum && NPCS.NPC->client->saberStoredIndex)
		{
			//
			if (g_entities[NPCS.NPC->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY)
			{
				//fell to the ground, try to pick it up
				if (Jedi_CanPullBackSaber(NPCS.NPC))
				{
					NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
					NPCS.NPCInfo->goalEntity = &g_entities[NPCS.NPC->client->saberStoredIndex];
					NPCS.ucmd.buttons &= ~BUTTON_ALT_ATTACK;
					NPCS.ucmd.buttons |= BUTTON_ATTACK;
					if (NPCS.NPC->enemy && NPCS.NPC->enemy->health > 0)
					{
						//get our saber back NOW!
						Jedi_Move(NPCS.NPCInfo->goalEntity, qfalse);
						NPC_UpdateAngles(qtrue, qtrue);
						if (NPCS.NPC->enemy->s.weapon == WP_SABER)
						{
							//be sure to continue evasion
							vec3_t enemy_dir, enemy_movedir, enemy_dest;
							float enemy_dist, enemy_movespeed;
							jedi_set_enemy_info(enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed,
								300);
							Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
						}
						return;
					}
				}
				else
				{
					if (NPCS.NPC->enemy && NPCS.NPC->enemy->health > 0)
					{
						if (NPCS.NPC->enemy->s.weapon == WP_SABER)
						{
							//be sure to continue evasion
							vec3_t enemy_dir, enemy_movedir, enemy_dest;
							float enemy_dist, enemy_movespeed;
							jedi_set_enemy_info(enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed,
								300);
							Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
						}
					}
				}
			}
			if (NPCS.NPC->enemy && NPCS.NPC->enemy->health > 0)
			{
				if (NPCS.NPC->enemy->s.weapon == WP_SABER)
				{
					//be sure to continue evasion
					vec3_t enemy_dir, enemy_movedir, enemy_dest;
					float enemy_dist, enemy_movespeed;
					jedi_set_enemy_info(enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300);
					Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
				}
			}
		}
	}
	//see if our enemy was killed by us, gloat and turn off saber after cool down.
	//FIXME: don't do this if we have other enemies to fight...?
	if (NPCS.NPC->enemy)
	{
		if (NPCS.NPC->enemy->health <= 0
			&& NPCS.NPC->enemy->enemy == NPCS.NPC
			&& (NPCS.NPC->client->playerTeam != NPCTEAM_PLAYER ||
				NPCS.NPC->client->NPC_class == CLASS_KYLE
				&& NPCS.NPC->spawnflags & 1 && NPCS.NPC->s.number < MAX_CLIENTS))
		{
			//my enemy is dead and I killed him
			NPCS.NPCInfo->enemyCheckDebounceTime = 0; //keep looking for others

			if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT
				|| NPCS.NPC->client->NPC_class == CLASS_REBORN && NPCS.NPC->s.weapon != WP_SABER
				|| NPCS.NPC->client->NPC_class == CLASS_ROCKETTROOPER
				|| NPCS.NPC->client->pers.nextbotclass == BCLASS_BOBAFETT
				|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN1
				|| NPCS.NPC->client->pers.nextbotclass == BCLASS_MANDOLORIAN2)
			{
				if (NPCS.NPCInfo->walkDebounceTime < level.time && NPCS.NPCInfo->walkDebounceTime >= 0)
				{
					TIMER_Set(NPCS.NPC, "gloatTime", 10000);
					NPCS.NPCInfo->walkDebounceTime = -1;
				}
				if (!TIMER_Done(NPCS.NPC, "gloatTime"))
				{
					if (DistanceHorizontalSquared(NPCS.NPC->client->renderInfo.eyePoint,
						NPCS.NPC->enemy->r.currentOrigin) > 4096 && NPCS.NPCInfo->scriptFlags
						& SCF_CHASE_ENEMIES) //64 squared
					{
						NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
						Jedi_Move(NPCS.NPC->enemy, qfalse);
						NPCS.ucmd.buttons |= BUTTON_WALKING;
					}
					else
					{
						TIMER_Set(NPCS.NPC, "gloatTime", 0);
					}
				}
				else if (NPCS.NPCInfo->walkDebounceTime == -1)
				{
					NPCS.NPCInfo->walkDebounceTime = -2;
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_VICTORY1, EV_VICTORY3), 3000);
					jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = level.time + 3000;
					NPCS.NPCInfo->desiredPitch = 0;
					NPCS.NPCInfo->goalEntity = NULL;
				}
				Jedi_FaceEnemy(qtrue);
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
			if (!TIMER_Done(NPCS.NPC, "parryTime"))
			{
				TIMER_Set(NPCS.NPC, "parryTime", -1);
				NPCS.NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			}
			NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
			if (!NPCS.NPC->client->ps.saberHolstered && NPCS.NPC->client->ps.saberInFlight)
			{
				//saber is still on (or we're trying to pull it back), count down erosion and keep facing the enemy
				Jedi_AggressionErosion(-3);

				if (BG_SabersOff(&NPCS.NPC->client->ps) && !NPCS.NPC->client->ps.saberInFlight)
				{
					//turned off saber (in hand), gloat
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_VICTORY1, EV_VICTORY3), 3000);
					jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = level.time + 3000;
					NPCS.NPCInfo->desiredPitch = 0;
					NPCS.NPCInfo->goalEntity = NULL;
				}
				TIMER_Set(NPCS.NPC, "gloatTime", 10000);
			}
			if (!NPCS.NPC->client->ps.saberHolstered || NPCS.NPC->client->ps.saberInFlight || !TIMER_Done(
				NPCS.NPC, "gloatTime"))
			{
				//keep walking
				if (DistanceHorizontalSquared(NPCS.NPC->client->renderInfo.eyePoint, NPCS.NPC->enemy->r.currentOrigin) >
					4096 && NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES) //64 squared
				{
					NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
					Jedi_Move(NPCS.NPC->enemy, qfalse);
					NPCS.ucmd.buttons |= BUTTON_WALKING;
				}
				else
				{
					//got there
					if (NPCS.NPC->health < NPCS.NPC->client->pers.maxHealth)
					{
						//racc - add insult to injury by healing near their dead body
						if (NPCS.NPC->client->saber[0].type == SABER_SITH_SWORD
							&& NPCS.NPC->client->weaponGhoul2[0])
						{
							Tavion_SithSwordRecharge();
						}
						else if ((NPCS.NPC->client->ps.fd.forcePowersKnown & 1 << FP_HEAL) != 0
							&& (NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_HEAL) == 0)
						{
							ForceHeal(NPCS.NPC);

							if (NPC_IsJedi(NPCS.NPC))
							{
								// Do deflect taunt...
								G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_GLOAT1, EV_GLOAT3), 5000 + Q_irand(0, 15000));
							}
						}
					}
				}
				Jedi_FaceEnemy(qtrue);
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}

	//If we don't have an enemy, just idle
	if (NPCS.NPC->enemy->s.weapon == WP_TURRET && !Q_stricmp("PAS", NPCS.NPC->enemy->classname))
	{
		if (NPCS.NPC->enemy->count <= 0)
		{
			//it's out of ammo
			if (NPCS.NPC->enemy->activator && NPC_ValidEnemy(NPCS.NPC->enemy->activator))
			{
				gentity_t* turretOwner = NPCS.NPC->enemy->activator;
				G_ClearEnemy(NPCS.NPC);
				G_SetEnemy(NPCS.NPC, turretOwner);
			}
			else
			{
				G_ClearEnemy(NPCS.NPC);
			}
		}
	}
	if (NPCS.NPC->enemy->NPC
		&& NPCS.NPC->enemy->NPC->charmedTime > level.time)
	{
		//my enemy was charmed
		if (OnSameTeam(NPCS.NPC, NPCS.NPC->enemy))
		{
			//has been charmed to be on my team
			G_ClearEnemy(NPCS.NPC);
		}
	}
	if (NPCS.NPC->client->playerTeam == NPCTEAM_ENEMY
		&& NPCS.NPC->client->enemyTeam == NPCTEAM_PLAYER
		&& NPCS.NPC->enemy
		&& NPCS.NPC->enemy->client
		&& NPCS.NPC->enemy->client->playerTeam != NPCS.NPC->client->enemyTeam
		&& OnSameTeam(NPCS.NPC, NPCS.NPC->enemy)
		&& !(NPCS.NPC->NPC->aiFlags & NPCAI_LOCKEDENEMY))
	{
		//an evil jedi somehow got another evil NPC as an enemy, they were probably charmed and it's run out now
		if (!NPC_ValidEnemy(NPCS.NPC->enemy))
		{
			G_ClearEnemy(NPCS.NPC);
		}
	}

	NPC_CheckEnemy(qtrue, qtrue, qtrue);

	if (!NPCS.NPC->enemy)
	{
		NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
		if (NPCS.NPCInfo->tempBehavior == BS_HUNT_AND_KILL)
		{
			//lost him, go back to what we were doing before
			NPCS.NPCInfo->tempBehavior = BS_DEFAULT;
			NPC_UpdateAngles(qtrue, qtrue);
			return;
		}
		Jedi_Patrol(); //was calling Idle... why?
		return;
	}

	//always face enemy if have one
	NPCS.NPCInfo->combatMove = qtrue;

	//Track the player and kill them if possible
	Jedi_Combat();

	if (!(NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)
		|| NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_HEAL && NPCS.NPC->client->ps.fd.forcePowerLevel[
			FP_HEAL] < FORCE_LEVEL_2)
	{
		//this is really stupid, but okay...
		NPCS.ucmd.forwardmove = 0;
		NPCS.ucmd.rightmove = 0;
		if (NPCS.ucmd.upmove > 0)
		{
			NPCS.ucmd.upmove = 0;
		}
		NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
		VectorClear(NPCS.NPC->client->ps.moveDir);
	}

	//NOTE: for now, we clear ucmd.forwardmove & ucmd.rightmove while in air to avoid jumps going awry...
	if (NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//don't push while in air, throws off jumps!
		//FIXME: if we are in the air over a drop near a ledge, should we try to push back towards the ledge?
		NPCS.ucmd.forwardmove = 0;
		NPCS.ucmd.rightmove = 0;
		VectorClear(NPCS.NPC->client->ps.moveDir);
	}

	if (!TIMER_Done(NPCS.NPC, "duck"))
	{
		NPCS.ucmd.upmove = -127;
	}

	if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT
		&& (NPCS.NPC->client->NPC_class != CLASS_REBORN || NPCS.NPC->s.weapon == WP_SABER)
		&& NPCS.NPC->client->NPC_class != CLASS_ROCKETTROOPER
		|| NPCS.NPC->client->pers.nextbotclass != BCLASS_BOBAFETT
		|| NPCS.NPC->client->pers.nextbotclass != BCLASS_MANDOLORIAN1
		|| NPCS.NPC->client->pers.nextbotclass != BCLASS_MANDOLORIAN2)
	{
		if (PM_SaberInBrokenParry(NPCS.NPC->client->ps.saber_move) || NPCS.NPC->client->ps.saberBlocked ==
			BLOCKED_PARRY_BROKEN)
		{
			//just make sure they don't pull their saber to them if they're being blocked
			NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
		}
	}

	if (NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE //not allowed to attack
		|| NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_HEAL && NPCS.NPC->client->ps.fd.forcePowerLevel[
			FP_HEAL] < FORCE_LEVEL_3
		|| NPCS.NPC->client->ps.saberEventFlags & SEF_INWATER && !NPCS.NPC->client->ps.saberInFlight)
		//saber in water
	{
		NPCS.ucmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK);
	}

	if (NPCS.NPCInfo->scriptFlags & SCF_NO_ACROBATICS)
	{
		NPCS.ucmd.upmove = 0;
		NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
	}

	if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT
		&& (NPCS.NPC->client->NPC_class != CLASS_REBORN || NPCS.NPC->s.weapon == WP_SABER)
		&& NPCS.NPC->client->NPC_class != CLASS_ROCKETTROOPER
		|| NPCS.NPC->client->pers.nextbotclass != BCLASS_BOBAFETT
		|| NPCS.NPC->client->pers.nextbotclass != BCLASS_MANDOLORIAN1
		|| NPCS.NPC->client->pers.nextbotclass != BCLASS_MANDOLORIAN2)
	{
		Jedi_CheckDecreasesaber_anim_level();
	}

	if (NPCS.ucmd.buttons & BUTTON_ATTACK && NPCS.NPC->client->playerTeam == NPCTEAM_ENEMY)
	{
		if (Q_irand(0, NPCS.NPC->client->ps.fd.saberAnimLevel) > 0
			&& Q_irand(0, NPCS.NPC->client->pers.maxHealth + 10) > NPCS.NPC->health
			&& !Q_irand(0, 3))
		{
			//the more we're hurt and the stronger the attack we're using, the more likely we are to make a anger noise when we swing
			G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 1000);
		}
	}

	if (Jedi_CheckKataAttack())
	{
		//doing a kata attack
	}
	else
	{
		//check other special combat behavior
		if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT
			&& (NPCS.NPC->client->NPC_class != CLASS_REBORN || NPCS.NPC->s.weapon == WP_SABER)
			&& NPCS.NPC->client->NPC_class != CLASS_ROCKETTROOPER
			|| NPCS.NPC->client->pers.nextbotclass != BCLASS_BOBAFETT
			|| NPCS.NPC->client->pers.nextbotclass != BCLASS_MANDOLORIAN1
			|| NPCS.NPC->client->pers.nextbotclass != BCLASS_MANDOLORIAN2)
		{
			//saber wielders only.
			if (NPCS.NPC->client->NPC_class == CLASS_TAVION
				|| NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER
				|| NPCS.NPC->client->NPC_class == CLASS_ALORA
				|| g_npcspskill.integer && (NPCS.NPC->client->NPC_class == CLASS_DESANN || NPCS.NPCInfo->rank >=
					Q_irand(RANK_CREWMAN, RANK_CAPTAIN)))
			{
				//certain NPCS will kick in force speed if the player does...
				if (NPCS.NPC->enemy
					&& !NPCS.NPC->enemy->s.number
					&& NPCS.NPC->enemy->client
					&& NPCS.NPC->enemy->client->ps.fd.forcePowersActive & 1 << FP_SPEED
					&& !(NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_SPEED))
				{
					int chance = 0;
					switch (g_npcspskill.integer)
					{
					case 0:
						chance = 9;
					case 1:
						chance = 3;
					case 2:
						chance = 1;
						break;
					default:;
					}
					if (!Q_irand(0, chance))
					{
						ForceSpeed(NPCS.NPC, 0);
					}
				}
			}
		}
		//Sometimes Alora flips towards you instead of runs
		if (NPCS.NPC->client->NPC_class == CLASS_ALORA)
		{
			if (NPCS.ucmd.buttons & BUTTON_ALT_ATTACK)
			{
				//chance of doing a special dual saber throw
				if (NPCS.NPC->client->ps.fd.saberAnimLevel == SS_DUAL
					&& !NPCS.NPC->client->ps.saberInFlight)
				{
					//has dual sabers and haven't already tossed the saber.
					if (Distance(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin) >= 120)
					{
						NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_ALORA_SPIN_THROW,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						NPCS.NPC->client->ps.weaponTime = NPCS.NPC->client->ps.torsoTimer;
					}
				}
			}
			else if (NPCS.NPC->enemy
				&& NPCS.ucmd.forwardmove > 0
				&& fabs(NPCS.ucmd.rightmove) < 32
				&& !(NPCS.ucmd.buttons & BUTTON_WALKING)
				&& !(NPCS.ucmd.buttons & BUTTON_ATTACK)
				&& NPCS.NPC->client->ps.saber_move == LS_READY
				&& NPCS.NPC->client->ps.legsAnim == BOTH_RUN_DUAL)
			{
				//running at us, not attacking
				if (Distance(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin) > 80)
				{
					if (NPCS.NPC->client->ps.legsAnim == BOTH_FLIP_F
						|| NPCS.NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_1_MD2
						|| NPCS.NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_2_MD2
						|| NPCS.NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_3_MD2
						|| NPCS.NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_1
						|| NPCS.NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_2
						|| NPCS.NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_3)
					{
						if (NPCS.NPC->client->ps.legsTimer <= 200 && Q_irand(0, 2))
						{
							//go ahead and start anotther
							NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, Q_irand(BOTH_ALORA_FLIP_1, BOTH_ALORA_FLIP_3),
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
					else if (!Q_irand(0, 6))
					{
						NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, Q_irand(BOTH_ALORA_FLIP_1, BOTH_ALORA_FLIP_3),
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
			}
		}
	}

	if (VectorCompare(NPCS.NPC->client->ps.moveDir, vec3_origin)
		&& (NPCS.ucmd.forwardmove || NPCS.ucmd.rightmove))
	{
		//using ucmds to move this turn, not NAV
		if (NPCS.ucmd.buttons & BUTTON_WALKING)
		{
			//FIXME: NAV system screws with speed directly, so now I have to re-set it myself!
			NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.walkSpeed;
		}
		else
		{
			NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.runSpeed;
		}
	}
}

//added all SP code used for the Rosh boss fight
qboolean Rosh_BeingHealed(const gentity_t* self)
{
	//Is this Rosh being healed?
	if (self
		&& self->NPC
		&& self->client
		&& self->NPC->aiFlags & NPCAI_ROSH
		&& self->flags & FL_UNDYING
		&& (self->health == 1 //need healing
			|| self->flags & FL_GODMODE
			|| self->client->ps.powerups[PW_INVINCIBLE] > level.time))
	{
		return qtrue;
	}
	return qfalse;
}

qboolean Rosh_TwinPresent()
{
	//look for the healer twins for the Rosh boss.
	const gentity_t* found_twin = G_Find(NULL, FOFS(NPC_type), "DKothos");
	if (!found_twin
		|| found_twin->health < 0)
	{
		found_twin = G_Find(NULL, FOFS(NPC_type), "VKothos");
	}
	if (!found_twin
		|| found_twin->health < 0)
	{
		//oh well, both twins are dead...
		return qfalse;
	}
	return qtrue;
}

extern qboolean G_ClearLineOfSight(const vec3_t point1, const vec3_t point2, int ignore, int clipmask);

qboolean Kothos_HealRosh(void)
{
	//this NPC attempts to heal the heal target (the rosh boss normally).
	if (NPCS.NPC->client
		&& NPCS.NPC->client->leader
		&& NPCS.NPC->client->leader->client)
	{
		if (DistanceSquared(NPCS.NPC->client->leader->r.currentOrigin, NPCS.NPC->r.currentOrigin) <= 256 * 256
			&& G_ClearLineOfSight(NPCS.NPC->client->leader->client->renderInfo.eyePoint,
				NPCS.NPC->client->renderInfo.eyePoint, NPCS.NPC->s.number, MASK_OPAQUE))
		{
			NPC_SetAnim(NPCS.NPC, SETANIM_TORSO, BOTH_FORCE_2HANDEDLIGHTNING_HOLD,
				SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			NPCS.NPC->client->ps.torsoTimer = 1000;

			if (NPCS.NPC->ghoul2)
			{
				mdxaBone_t boltMatrix;
				vec3_t fxOrg, fx_dir, angles;

				VectorSet(angles, 0, NPCS.NPC->r.currentAngles[YAW], 0);

				trap->G2API_GetBoltMatrix(NPCS.NPC->ghoul2, 0,
					Q_irand(0, 1),
					&boltMatrix, angles, NPCS.NPC->r.currentOrigin, level.time,
					NULL, NPCS.NPC->modelScale);
				BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, fxOrg);
				VectorSubtract(NPCS.NPC->client->leader->r.currentOrigin, fxOrg, fx_dir);
				VectorNormalize(fx_dir);
				G_PlayEffect(G_EffectIndex("force/kothos_beam.efx"), fxOrg, fx_dir);
			}

			NPCS.NPC->client->leader->health += Q_irand(1 + g_npcspskill.integer * 2, 4 + g_npcspskill.integer * 3);
			//from 1-5 to 4-10
			if (NPCS.NPC->client->leader->client)
			{
				if (NPCS.NPC->client->leader->client->ps.legsAnim == BOTH_FORCEHEAL_START
					&& NPCS.NPC->client->leader->health >= NPCS.NPC->client->leader->client->pers.maxHealth)
				{
					//let him get up now
					NPC_SetAnim(NPCS.NPC->client->leader, SETANIM_BOTH, BOTH_FORCEHEAL_STOP,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					NPCS.NPC->client->leader->flags &= ~FL_GODMODE;
					NPCS.NPC->client->leader->client->ps.powerups[PW_INVINCIBLE] = level.time + NPCS.NPC->client->leader
						->client->ps.torsoTimer;
					NPCS.NPC->client->leader->NPC->ignorePain = qfalse;
					NPCS.NPC->client->leader->health = NPCS.NPC->client->leader->client->pers.maxHealth;
				}
				else
				{
					//start effect?
					NPCS.NPC->client->leader->flags |= FL_GODMODE;
					NPCS.NPC->client->leader->client->ps.powerups[PW_INVINCIBLE] = level.time + 500;
				}
			}
			//decrement
			NPCS.NPC->count--;
			if (!NPCS.NPC->count)
			{
				TIMER_Set(NPCS.NPC, "healRoshDebounce", Q_irand(5000, 10000));
				NPCS.NPC->count = 100;
			}
			//now protect me, too
			if (g_npcspskill.integer)
			{
				//not on easy
				NPCS.NPC->client->leader->flags |= FL_GODMODE;
				NPCS.NPC->client->ps.powerups[PW_INVINCIBLE] = level.time + 500;
			}
			return qtrue;
		}
	}
	return qfalse;
}

void Kothos_PowerRosh(void)
{
	if (NPCS.NPC->client
		&& NPCS.NPC->client->leader)
	{
		if (Distance(NPCS.NPC->client->leader->r.currentOrigin, NPCS.NPC->r.currentOrigin) <= 512.0f
			&& G_ClearLineOfSight(NPCS.NPC->client->leader->client->renderInfo.eyePoint,
				NPCS.NPC->client->renderInfo.eyePoint, NPCS.NPC->s.number, MASK_OPAQUE))
		{
			NPC_FaceEntity(NPCS.NPC->client->leader, qtrue);
			NPC_SetAnim(NPCS.NPC, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			NPCS.NPC->client->ps.torsoTimer = 500;
			if (NPCS.NPC->client->leader->client)
			{
				//hmm, give him some force?
				NPCS.NPC->client->leader->client->ps.fd.forcePower++;
			}
		}
	}
}

qboolean Kothos_Retreat(void)
{
	//racc - I think this is suppose to be the movement code for the Kothos healers when
	if (NPCS.NPCInfo->aiFlags & NPCAI_BLOCKED)
	{
		if (level.time - NPCS.NPCInfo->blockedDebounceTime > 1000)
		{
			return qfalse;
		}
	}
	return qtrue;
}

#define TWINS_DANGER_DIST_EASY (128.0f*128.0f)
#define TWINS_DANGER_DIST_MEDIUM (192.0f*192.0f)
#define TWINS_DANGER_DIST_HARD (256.0f*256.0f)

float Twins_DangerDist(void)
{
	//determines the distance at which the Kothos twins engage enemies.
	switch (g_npcspskill.integer)
	{
	case 0:
		return TWINS_DANGER_DIST_EASY;
	case 1:
		return TWINS_DANGER_DIST_MEDIUM;
	case 2:
	default:
		return TWINS_DANGER_DIST_HARD;
	}
}

extern void WP_Explode(gentity_t* self);

qboolean Jedi_InSpecialMove(void)
{
	if (NPCS.NPC->client->ps.torsoAnim == BOTH_KYLE_PA_1
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_KYLE_PA_2
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_KYLE_PA_3
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_1
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_2
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_3
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_END
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRABBED)
	{
		NPC_UpdateAngles(qtrue, qtrue);
		return qtrue;
	}

	if (Jedi_InNoAIAnim(NPCS.NPC))
	{
		//in special anims, don't do force powers or attacks, just face the enemy
		if (NPCS.NPC->enemy)
		{
			NPC_FaceEnemy(qtrue);
		}
		else
		{
			NPC_UpdateAngles(qtrue, qtrue);
		}
		return qtrue;
	}

	if (NPCS.NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_HOLD)
	{
		if (!TIMER_Done(NPCS.NPC, "draining"))
		{
			//FIXME: what do we do if we ran out of power?  NPC's can't?
			//FIXME: don't keep turning to face enemy or we'll end up spinning around
			NPCS.ucmd.buttons |= BUTTON_FORCE_DRAIN;
		}
		NPC_UpdateAngles(qtrue, qtrue);
		return qtrue;
	}

	if (NPCS.NPC->client->ps.torsoAnim == BOTH_TAVION_SWORDPOWER)
	{
		NPCS.NPC->health += Q_irand(1, 2);
		if (NPCS.NPC->health > NPCS.NPC->client->ps.stats[STAT_MAX_HEALTH])
		{
			NPCS.NPC->health = NPCS.NPC->client->ps.stats[STAT_MAX_HEALTH];
		}
		NPC_UpdateAngles(qtrue, qtrue);
		return qtrue;
	}

	if (NPCS.NPC->client->ps.torsoAnim == BOTH_SCEPTER_START)
	{
		//going into the scepter laser attack
		if (NPCS.NPC->client->ps.torsoTimer <= 100)
		{
			//go into the hold
			NPCS.NPC->s.loopSound = G_SoundIndex("sound/weapons/scepter/loop.wav");
			//G_StopEffect(G_EffectIndex("scepter/beam.efx"), NPCS.NPC->weaponModel[1], NPCS.NPC->genericBolt1, NPCS.NPC->s.number);
			NPCS.NPC->client->ps.legsTimer = NPCS.NPC->client->ps.torsoTimer = 0;
			NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_SCEPTER_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			NPCS.NPC->client->ps.torsoTimer += 200;
			NPCS.NPC->painDebounceTime = level.time + NPCS.NPC->client->ps.torsoTimer;
			NPCS.NPC->client->ps.pm_time = NPCS.NPC->client->ps.torsoTimer;
			NPCS.NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			VectorClear(NPCS.NPC->client->ps.velocity);
			VectorClear(NPCS.NPC->client->ps.moveDir);
		}
		if (NPCS.NPC->enemy)
		{
			NPC_FaceEnemy(qtrue);
		}
		else
		{
			NPC_UpdateAngles(qtrue, qtrue);
		}
		return qtrue;
	}
	if (NPCS.NPC->client->ps.torsoAnim == BOTH_SCEPTER_HOLD)
	{
		//in the scepter laser attack
		if (NPCS.NPC->client->ps.torsoTimer <= 100)
		{
			NPCS.NPC->s.loopSound = 0;
			//G_StopEffect(G_EffectIndex("scepter/beam.efx"), NPCS.NPC->weaponModel[1], NPCS.NPC->genericBolt1, NPC->s.number);
			NPCS.NPC->client->ps.legsTimer = NPCS.NPC->client->ps.torsoTimer = 0;
			NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_SCEPTER_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			NPCS.NPC->painDebounceTime = level.time + NPCS.NPC->client->ps.torsoTimer;
			NPCS.NPC->client->ps.pm_time = NPCS.NPC->client->ps.torsoTimer;
			NPCS.NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			VectorClear(NPCS.NPC->client->ps.velocity);
			VectorClear(NPCS.NPC->client->ps.moveDir);
		}
		else
		{
			Tavion_ScepterDamage();
		}
		if (NPCS.NPC->enemy)
		{
			NPC_FaceEnemy(qtrue);
		}
		else
		{
			NPC_UpdateAngles(qtrue, qtrue);
		}
		return qtrue;
	}
	if (NPCS.NPC->client->ps.torsoAnim == BOTH_SCEPTER_STOP)
	{
		//coming out of the scepter laser attack
		if (NPCS.NPC->enemy)
		{
			NPC_FaceEnemy(qtrue);
		}
		else
		{
			NPC_UpdateAngles(qtrue, qtrue);
		}
		return qtrue;
	}
	if (NPCS.NPC->client->ps.torsoAnim == BOTH_TAVION_SCEPTERGROUND)
	{
		//specter slamming
		if (NPCS.NPC->client->ps.torsoTimer <= 1200
			&& !NPCS.NPC->count)
		{
			Tavion_ScepterSlam();
			NPCS.NPC->count = 1;
		}
		NPC_UpdateAngles(qtrue, qtrue);
		return qtrue;
	}

	if (Jedi_CultistDestroyer(NPCS.NPC))
	{
		if (!NPCS.NPC->takedamage)
		{
			//ready to explode
			if (NPCS.NPC->useDebounceTime <= level.time)
			{
				//this should damage everyone - FIXME: except other destroyers?
				NPCS.NPC->client->playerTeam = NPCTEAM_FREE; //FIXME: will this destroy wampas, tusken & rancors?
				NPCS.NPC->splashDamage = 200; // rough match to SP
				NPCS.NPC->splashRadius = 512; // see above
				WP_Explode(NPCS.NPC);
				return qtrue;
			}
			if (NPCS.NPC->enemy)
			{
				NPC_FaceEnemy(qfalse);
			}
			return qtrue;
		}
	}

	if (NPCS.NPC->client->NPC_class == CLASS_REBORN)
	{
		//reborn!
		if (NPCS.NPCInfo->aiFlags & NPCAI_HEAL_ROSH)
		{
			//NPC heals Rosh
			if (!NPCS.NPC->client->leader)
			{
				//find Rosh
				NPCS.NPC->client->leader = G_Find(NULL, FOFS(NPC_type), "rosh_dark");
			}
			if (NPCS.NPC->client->leader)
			{
				//have our heal target
				qboolean helpingRosh = qfalse;
				NPCS.NPC->flags |= FL_LOCK_PLAYER_WEAPONS;
				NPCS.NPC->client->leader->flags |= FL_UNDYING;

				if (NPCS.NPC->client->leader->client)
				{
					NPCS.NPC->client->leader->client->ps.fd.forcePowersKnown |= FORCE_POWERS_ROSH_FROM_TWINS;
				}
				if (NPCS.NPC->client->leader->client->ps.legsAnim == BOTH_FORCEHEAL_START
					&& TIMER_Done(NPCS.NPC, "healRoshDebounce"))
				{
					//our heal target is needing a heal
					if (Kothos_HealRosh())
					{
						helpingRosh = qtrue;
					}
					else
					{
						//not able to heal, probably not close enough, try to follow him to
						//get closer
						NPC_BSJedi_FollowLeader();
						NPC_UpdateAngles(qtrue, qtrue);
						return qtrue;
					}
				}

				if (helpingRosh)
				{
					//if we're healing Rosh, quit using force powers and face him.
					WP_ForcePowerStop(NPCS.NPC, FP_LIGHTNING);
					WP_ForcePowerStop(NPCS.NPC, FP_DRAIN);
					WP_ForcePowerStop(NPCS.NPC, FP_GRIP);
					NPC_FaceEntity(NPCS.NPC->client->leader, qtrue);
					return qtrue;
				}
				if (NPCS.NPC->enemy && DistanceSquared(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin) <
					Twins_DangerDist())
				{
					//enemies to attack and close enough to our heal target.
					if (NPCS.NPC->enemy && Kothos_Retreat())
					{
						//not blocked in movement
						NPC_FaceEnemy(qtrue);
						if (TIMER_Done(NPCS.NPC, "attackDelay"))
						{
							//attack!
							if (NPCS.NPC->painDebounceTime > level.time //just get hurt
								|| NPCS.NPC->health < 100 && Q_irand(-20, (g_npcspskill.integer + 1) * 10) > 0
								//injured
								|| !Q_irand(0, 80 - g_npcspskill.integer * 20)) //just randomly
							{
								switch (Q_irand(0, 7 + g_npcspskill.integer)) //on easy: no lightning
								{
								case 0:
								case 1:
								case 2:
								case 3:
									//Push the bastard.
									ForceThrow(NPCS.NPC, qfalse);
									NPCS.NPC->client->ps.weaponTime = Q_irand(1000, 3000) + (2 - g_npcspskill.integer) *
										1000;
									if (NPCS.NPC->painDebounceTime <= level.time
										&& NPCS.NPC->health >= 100)
									{
										//have an attack delay if this attack isn't the result
										//of injury
										TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.weaponTime);
									}
									break;
								case 4:
								case 5:
									NPCS.NPC->client->ps.weaponTime = Q_irand(3000, 6000) + (2 - g_npcspskill.integer) *
										2000;
									TIMER_Set(NPCS.NPC, "draining", NPCS.NPC->client->ps.weaponTime);
									if (NPCS.NPC->painDebounceTime <= level.time
										&& NPCS.NPC->health >= 100)
									{
										//have an attack delay if this attack isn't the result
										//of injury
										TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.weaponTime);
									}
									break;
								case 6:
								case 7:
									//try to grip them
									if (NPCS.NPC->enemy && InFOV3(NPCS.NPC->enemy->r.currentOrigin,
										NPCS.NPC->r.currentOrigin,
										NPCS.NPC->client->ps.viewangles, 20, 30))
									{
										NPCS.NPC->client->ps.weaponTime = Q_irand(3000, 6000) + (2 - g_npcspskill.
											integer) * 2000;
										TIMER_Set(NPCS.NPC, "gripping", 3000);
										if (NPCS.NPC->painDebounceTime <= level.time
											&& NPCS.NPC->health >= 100)
										{
											//have an attack delay if this attack isn't the result
											//of injury
											TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.weaponTime);
										}
									}
									break;
								case 8:
								case 9:
								default:
									//fly their ass.
									ForceLightning(NPCS.NPC);
									if (NPCS.NPC->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1)
									{
										NPCS.NPC->client->ps.weaponTime = Q_irand(3000, 6000) + (2 - g_npcspskill.
											integer) * 2000;
										TIMER_Set(NPCS.NPC, "holdLightning", NPCS.NPC->client->ps.weaponTime);
									}
									if (NPCS.NPC->painDebounceTime <= level.time
										&& NPCS.NPC->health >= 100)
									{
										TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.weaponTime);
									}
									break;
								}
							}
						}
						else
						{
							NPCS.NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
						}
						Jedi_TimersApply();
						return qtrue;
					}
					NPCS.NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
				}
				else if (!G_ClearLOS4(NPCS.NPC, NPCS.NPC->client->leader)
					|| DistanceSquared(NPCS.NPC->r.currentOrigin, NPCS.NPC->client->leader->r.currentOrigin) > 512 *
					512)
				{
					//can't see Rosh or too far away, catch up with him
					if (!TIMER_Done(NPCS.NPC, "attackDelay"))
					{
						NPCS.NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
					}
					NPC_BSJedi_FollowLeader();
					NPC_UpdateAngles(qtrue, qtrue);
					return qtrue;
				}
				else
				{
					//no enemies and close to our heal target
					if (!TIMER_Done(NPCS.NPC, "attackDelay"))
					{
						NPCS.NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
					}

					NPC_FaceEnemy(qtrue);
					return qtrue;
				}
			}
			NPC_UpdateAngles(qtrue, qtrue);
		}
		else if (NPCS.NPCInfo->aiFlags & NPCAI_ROSH)
		{
			//I'm rosh!
			if (NPCS.NPC->flags & FL_UNDYING)
			{
				//Vil and/or Dasariah still around to heal me
				if (NPCS.NPC->health == 1 //need healing
					|| NPCS.NPC->flags & FL_GODMODE
					|| NPCS.NPC->client->ps.powerups[PW_INVINCIBLE] > level.time) //being healed
				{
					//FIXME: custom anims
					if (Rosh_TwinPresent())
					{
						//twins are still alive
						if (!NPCS.NPC->client->ps.weaponTime)
						{
							//not attacking
							if (NPCS.NPC->client->ps.legsAnim != BOTH_FORCEHEAL_START
								&& NPCS.NPC->client->ps.legsAnim != BOTH_FORCEHEAL_STOP)
							{
								//get down and wait for Vil or Dasariah to help us
								NPCS.NPC->client->ps.legsTimer = NPCS.NPC->client->ps.torsoTimer = 0;
								NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_FORCEHEAL_START,
									SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
								NPCS.NPC->client->ps.torsoTimer = NPCS.NPC->client->ps.legsTimer = -1;
								//hold the animation
								NPCS.NPC->client->ps.saberHolstered = 2; //turn saber off.
								NPCS.NPCInfo->ignorePain = qtrue;
							}
						}
						NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
						NPCS.NPC->client->ps.saber_move /* not sure what this is = NPC->client->ps.saberMoveNext*/ =
							LS_NONE;
						NPCS.NPC->painDebounceTime = level.time + 500;
						NPCS.NPC->client->ps.pm_time = 500;
						NPCS.NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
						VectorClear(NPCS.NPC->client->ps.velocity);
						VectorClear(NPCS.NPC->client->ps.moveDir);
						return qtrue;
					}
				}
			}
		}
	}

	if (PM_SuperBreakWinAnim(NPCS.NPC->client->ps.torsoAnim))
	{
		//striking down our saberlock victim.
		NPC_FaceEnemy(qtrue);
		if (NPCS.NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			//on the ground, don't move
			VectorClear(NPCS.NPC->client->ps.velocity);
		}
		VectorClear(NPCS.NPC->client->ps.moveDir);
		NPCS.ucmd.rightmove = NPCS.ucmd.forwardmove = NPCS.ucmd.upmove = 0;
		return qtrue;
	}
	return qfalse;
}

void NPC_CheckEvasion(void)
{
	vec3_t enemy_dir, enemy_movedir, enemy_dest;
	float enemy_dist, enemy_movespeed;

	if (in_camera)
	{
		return;
	}

	if (!NPCS.NPC->enemy || !NPCS.NPC->enemy->inuse || NPCS.NPC->enemy->NPC && NPCS.NPC->enemy->health <= 0)
	{
		return;
	}

	switch (NPCS.NPC->client->NPC_class)
	{
	case CLASS_BARTENDER:
	case CLASS_BESPIN_COP:
	case CLASS_CLAW:
	case CLASS_COMMANDO:
	case CLASS_GALAK:
	case CLASS_GRAN:
	case CLASS_IMPERIAL:
	case CLASS_IMPWORKER:
	case CLASS_JAN:
	case CLASS_LANDO:
	case CLASS_MONMOTHA:
	case CLASS_PRISONER:
	case CLASS_PROTOCOL:
	case CLASS_REBEL:
	case CLASS_REELO:
	case CLASS_RODIAN:
	case CLASS_TRANDOSHAN:
	case CLASS_WEEQUAY:
	case CLASS_BOBAFETT:
	case CLASS_MANDO:
	case CLASS_SABOTEUR:
	case CLASS_STORMTROOPER:
	case CLASS_STORMCOMMANDO:
	case CLASS_SWAMPTROOPER:
	case CLASS_CLONETROOPER:
	case CLASS_NOGHRI:
	case CLASS_UGNAUGHT:
	case CLASS_WOOKIE:
		// OK... EVADE AWAY!!!
		break;
	default:
		// NOT OK...
		return;
	}

	//See where enemy will be 300 ms from now
	jedi_set_enemy_info(enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300);

	if (NPCS.NPC->enemy->s.weapon == WP_SABER)
	{
		Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
	}
	else
	{
		//do we need to do any evasion for other kinds of enemies?
		if (NPCS.NPC->enemy->client)
		{
			vec3_t shotDir, ang;

			VectorSubtract(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin, shotDir);
			vectoangles(shotDir, ang);

			if (NPCS.NPC->enemy->client->ps.weaponstate == WEAPON_FIRING && in_field_of_vision(
				NPCS.NPC->enemy->client->ps.viewangles, 90, ang))
			{
				// They are shooting at us. Evade!!!
				if (NPCS.NPC->enemy->s.weapon == WP_SABER)
				{
					Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
				}
				else
				{
					NPC_StartFlee(NPCS.NPC->enemy, NPCS.NPC->enemy->r.currentOrigin, AEL_DANGER, 1000, 3000);
				}
			}
			else if (in_field_of_vision(NPCS.NPC->enemy->client->ps.viewangles, 60, ang))
			{
				// Randomly (when they are targetting us)... Evade!!!
				if (NPCS.NPC->enemy->s.weapon == WP_SABER)
				{
					Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
				}
				else
				{
					NPC_StartFlee(NPCS.NPC->enemy, NPCS.NPC->enemy->r.currentOrigin, AEL_DANGER, 1000, 3000);
				}
			}
			else
			{
				int entity_list[MAX_GENTITIES];

				vec3_t mins;
				vec3_t maxs;

				for (int e = 0; e < 3; e++)
				{
					mins[e] = NPCS.NPC->r.currentOrigin[e] - 256;
					maxs[e] = NPCS.NPC->r.currentOrigin[e] + 256;
				}
				const int num_ents = trap->EntitiesInBox(mins, maxs, entity_list, MAX_GENTITIES);

				for (int i = 0; i < num_ents; i++)
				{
					const gentity_t* missile = &g_entities[i];

					if (!missile) continue;
					if (!missile->inuse) continue;

					if (missile->s.eType == ET_MISSILE)
					{
						// Missile incoming!!! Evade!!!
						if (NPCS.NPC->enemy->s.weapon == WP_SABER)
						{
							Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
						}
						else
						{
							NPC_StartFlee(NPCS.NPC->enemy, NPCS.NPC->enemy->r.currentOrigin, AEL_DANGER, 1000, 3000);
						}
						return;
					}
				}
			}
		}
	}
}

extern void NPC_BSST_Patrol(void);
extern void NPC_BSSniper_Default(void);

void NPC_BSJedi_Default(void)
{
	if (Jedi_InSpecialMove())
	{
		return;
	}

	Jedi_CheckCloak();

	if (!NPCS.NPC->enemy)
	{
		//don't have an enemy, look for one
		if (!NPC_IsJedi(NPCS.NPC))
		{
			NPC_BSST_Patrol();
		}
		else
		{
			Jedi_Patrol();
		}
	}
	else //if ( NPC->enemy )
	{
		//have an enemy
		if (Jedi_WaitingAmbush(NPCS.NPC))
		{
			//we were still waiting to drop down - must have had enemy set on me outside my AI
			Jedi_Ambush(NPCS.NPC);
		}

		if (Jedi_CultistDestroyer(NPCS.NPC) && !NPCS.NPCInfo->charmedTime)
		{
			//destroyer
			//permanent effect
			NPCS.NPCInfo->charmedTime = Q3_INFINITE;
			NPCS.NPC->client->ps.fd.forcePowersActive |= 1 << FP_RAGE;
			NPCS.NPC->client->ps.fd.forcePowerDuration[FP_RAGE] = Q3_INFINITE;
			NPCS.NPC->s.loopSound = G_SoundIndex("sound/movers/objects/green_beam_lp2.wav");
		}

		if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
		{
			if (NPCS.NPC->enemy->enemy != NPCS.NPC && NPCS.NPC->health == NPCS.NPC->client->pers.maxHealth &&
				DistanceSquared(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin) > 800 * 800)
			{
				NPCS.NPCInfo->scriptFlags |= SCF_altFire;
				Boba_ChangeWeapon(WP_DISRUPTOR);
				NPC_BSSniper_Default();
				return;
			}
		}

		if (NPCS.NPC->enemy->s.weapon != WP_SABER)
		{
			// Normal non-jedi enemy
			if (TIMER_Done(NPCS.NPC, "heal")
				&& !Jedi_SaberBusy(NPCS.NPC)
				&& NPCS.NPC->client->ps.fd.forcePowerLevel[FP_HEAL] > 0
				&& NPCS.NPC->health > 0
				&& NPCS.NPC->client->ps.weaponTime <= 0
				&& NPCS.NPC->client->ps.fd.forcePower >= 25
				&& NPCS.NPC->client->ps.fd.forcePowerDebounce[FP_HEAL] <= level.time
				&& WP_ForcePowerUsable(NPCS.NPC, FP_HEAL)
				&& NPCS.NPC->health <= NPCS.NPC->client->ps.stats[STAT_MAX_HEALTH] / 3
				&& Q_irand(0, 2) == 2)
			{
				// Try to heal...
				ForceHeal(NPCS.NPC);
				TIMER_Set(NPCS.NPC, "heal", Q_irand(5000, 10000));

				if (NPC_IsJedi(NPCS.NPC))
				{
					// Do deflect taunt...
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_GLOAT1, EV_GLOAT3), 5000 + Q_irand(0, 15000));
				}
			}
			else
			{
				// Check for an evasion method...
				if (!in_camera)
				{
					NPC_CheckEvasion();
				}

				if (NPC_IsJedi(NPCS.NPC))
				{
					// Do deflect taunt...
					G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_DEFLECT1, EV_DEFLECT3), 5000 + Q_irand(0, 15000));
				}
			}

			Jedi_Attack();
		}
		else
		{
			// Jedi/Sith.
			if (TIMER_Done(NPCS.NPC, "TalkTime"))
			{
				if (npc_is_dark_jedi(NPCS.NPC))
				{
					// Do taunt/anger...
					const int call_out = Q_irand(0, 3);

					switch (call_out)
					{
					case 3:
						G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_JCHASE1, EV_JCHASE3), 5000 + Q_irand(0, 15000));
						break;
					case 2:
						G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 5000 + Q_irand(0, 15000));
						break;
					case 1:
						G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_ANGER1, EV_ANGER1), 5000 + Q_irand(0, 15000));
						break;
					default:
						G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 5000 + Q_irand(0, 15000));
						break;
					}
				}
				else if (npc_is_light_jedi(NPCS.NPC))
				{
					// Do taunt...
					const int call_out = Q_irand(0, 2);

					switch (call_out)
					{
					case 2:
						G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_JCHASE1, EV_JCHASE3), 5000 + Q_irand(0, 15000));
						break;
					case 1:
						G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 5000 + Q_irand(0, 15000));
						break;
					default:
						G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 5000 + Q_irand(0, 15000));
					}
				}
				else
				{
					// Do taunt/anger...
					const int call_out = Q_irand(0, 3);

					switch (call_out)
					{
					case 3:
						G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_JCHASE1, EV_JCHASE3), 5000 + Q_irand(0, 15000));
						break;
					case 2:
						G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 5000 + Q_irand(0, 15000));
						break;
					case 1:
						G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_ANGER1, EV_ANGER1), 5000 + Q_irand(0, 15000));
						break;
					default:
						G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 5000 + Q_irand(0, 15000));
						break;
					}
				}
				TIMER_Set(NPCS.NPC, "TalkTime", 5000);
			}

			Jedi_Attack();
		}

		//if we have multiple-jedi combat, probably need to keep checking (at certain debounce intervals) for a better (closer, more active) enemy and switch if needbe...
		if ((!NPCS.ucmd.buttons && !NPCS.NPC->client->ps.fd.forcePowersActive || NPCS.NPC->enemy && NPCS.NPC->enemy->
			health <= 0) && NPCS.NPCInfo->enemyCheckDebounceTime < level.time)
		{
			//not doing anything (or walking toward a vanquished enemy - fixme: always taunt the player?), not using force powers and it's time to look again
			//FIXME: build a list of all local enemies (since we have to find best anyway) for other AI factors- like when to use group attacks, determine when to change tactics, when surrounded, when blocked by another in the enemy group, etc.  Should we build this group list or let the enemies maintain their own list and we just access it?
			gentity_t* sav_enemy = NPCS.NPC->enemy; //FIXME: what about NPC->lastEnemy?

			NPCS.NPC->enemy = NULL;
			gentity_t* newEnemy = NPC_CheckEnemy(NPCS.NPCInfo->confusionTime < level.time, qfalse, qfalse);
			NPCS.NPC->enemy = sav_enemy;
			if (newEnemy && newEnemy != sav_enemy)
			{
				//picked up a new enemy!
				NPCS.NPC->lastEnemy = NPCS.NPC->enemy;
				G_SetEnemy(NPCS.NPC, newEnemy);
			}
			NPCS.NPCInfo->enemyCheckDebounceTime = level.time + Q_irand(1000, 3000);
		}
	}
	if (NPCS.NPC->client->saber[0].type == SABER_SITH_SWORD
		&& NPCS.NPC->client->weaponGhoul2[0])
	{
		//have sith sword, think about healing.
		if (NPCS.NPC->health < 100
			&& !Q_irand(0, 20))
		{
			Tavion_SithSwordRecharge();
		}
	}
}