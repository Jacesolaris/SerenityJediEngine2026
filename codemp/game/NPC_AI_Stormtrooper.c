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

extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
extern void AI_GroupUpdateSquadstates(AIGroupInfo_t* group, const gentity_t* member, int new_squad_state);
extern qboolean AI_GroupContainsEntNum(const AIGroupInfo_t* group, int entNum);
extern void AI_GroupUpdateEnemyLastSeen(AIGroupInfo_t* group, vec3_t spot);
extern void AI_GroupUpdateClearShotTime(AIGroupInfo_t* group);
extern void NPC_TempLookTarget(const gentity_t* self, int lookEntNum, int minLookTime, int maxLookTime);
extern qboolean G_ExpandPointToBBox(vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask);
extern void ChangeWeapon(const gentity_t* ent, int new_weapon);
extern void NPC_CheckGetNewWeapon(void);
extern int GetTime(int lastTime);
extern void NPC_AimAdjust(int change);
extern qboolean FlyingCreature(const gentity_t* ent);
extern void NPC_EvasionSaber(void);
extern qboolean RT_Flying(const gentity_t* self);
extern float NPC_EnemyRangeFromBolt(int boltIndex);
extern qboolean in_camera;
extern void NPC_CheckEvasion(void);
extern qboolean PM_InKnockDown(const playerState_t* ps);
extern qboolean NPC_CanUseAdvancedFighting();

#define	MAX_VIEW_DIST		1024
#define MAX_VIEW_SPEED		250
#define	MAX_LIGHT_INTENSITY 255
#define	MIN_LIGHT_THRESHOLD	0.1
#define	ST_MIN_LIGHT_THRESHOLD 30
#define	ST_MAX_LIGHT_THRESHOLD 180
#define	DISTANCE_THRESHOLD	0.075f
#define	MIN_TURN_AROUND_DIST_SQ	(10000)	//(100 squared) don't stop running backwards
//if your goal is less than 100 away

#define	DISTANCE_SCALE		0.35f	//These first three get your base detection rating, ideally add up to 1
#define	FOV_SCALE			0.40f	//
#define	LIGHT_SCALE			0.25f	//

#define	SPEED_SCALE			0.25f	//These next two are bonuses
#define	TURNING_SCALE		0.25f	//

#define	REALIZE_THRESHOLD	0.6f
#define CAUTIOUS_THRESHOLD	( REALIZE_THRESHOLD * 0.75 )

qboolean NPC_CheckPlayerTeamStealth(void);

static qboolean enemyLOS;
static qboolean enemyCS;
static qboolean enemyInFOV;
static qboolean hitAlly;
static qboolean faceEnemy;
static qboolean move;
static qboolean shoot;
static float enemyDist;
static vec3_t impactPos;

int groupSpeechDebounceTime[TEAM_NUM_TEAMS]; //used to stop several group AI from speaking all at once

void NPC_Saboteur_Precache(void)
{
	G_SoundIndex("sound/chars/shadowtrooper/cloak.wav");
	G_SoundIndex("sound/chars/shadowtrooper/decloak.wav");
}

extern void G_SoundOnEnt(gentity_t* ent, soundChannel_t channel, const char* sound_path);

void Saboteur_Decloak(gentity_t* self, const int uncloak_time)
{
	//decloak this Saboteur
	if (self && self->client)
	{
		if (self->client->ps.powerups[PW_CLOAKED] && TIMER_Done(self, "decloakwait"))
		{
			//Uncloak
			self->client->ps.powerups[PW_CLOAKED] = 0;
			G_SoundOnEnt(self, CHAN_ITEM, "sound/chars/shadowtrooper/decloak.wav");
			TIMER_Set(self, "nocloak", uncloak_time);
		}
	}
}

void Saboteur_Cloak(gentity_t* self)
{
	//cloak a Saboteur NPC
	if (self && self->client && self->NPC)
	{
		//FIXME: need to have this timer set once first?
		if (TIMER_Done(self, "nocloak"))
		{
			//not sitting around waiting to cloak again
			if (!self->client->ps.powerups[PW_CLOAKED])
			{
				//cloak
				self->client->ps.powerups[PW_CLOAKED] = Q3_INFINITE;
				G_SoundOnEnt(self, CHAN_ITEM, "sound/chars/shadowtrooper/cloak.wav");
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_PROTECT_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
		}
	}
}

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_UNDERFIRE,
	LSTATE_INVESTIGATE,
};

void ST_AggressionAdjust(const gentity_t* self, const int change)
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
		if (self->client->NPC_class == CLASS_IMPERIAL ||
			self->client->NPC_class == CLASS_RODIAN ||
			self->client->NPC_class == CLASS_TRANDOSHAN ||
			self->client->NPC_class == CLASS_TUSKEN ||
			self->client->NPC_class == CLASS_WEEQUAY ||
			self->client->NPC_class == CLASS_STORMTROOPER ||
			self->client->NPC_class == CLASS_CLONETROOPER ||
			self->client->NPC_class == CLASS_STORMCOMMANDO)
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

void ST_ClearTimers(const gentity_t* ent)
{
	TIMER_Set(ent, "chatter", 0);
	TIMER_Set(ent, "duck", 0);
	TIMER_Set(ent, "stand", 0);
	TIMER_Set(ent, "shuffleTime", 0);
	TIMER_Set(ent, "sleepTime", 0);
	TIMER_Set(ent, "enemyLastVisible", 0);
	TIMER_Set(ent, "roamTime", 0);
	TIMER_Set(ent, "hideTime", 0);
	TIMER_Set(ent, "attackDelay", 0); //FIXME: Slant for difficulty levels
	TIMER_Set(ent, "stick", 0);
	TIMER_Set(ent, "scoutTime", 0);
	TIMER_Set(ent, "flee", 0);
	TIMER_Set(ent, "interrogating", 0);
	TIMER_Set(ent, "verifyCP", 0);
	TIMER_Set(ent, "strafeRight", 0);
	TIMER_Set(ent, "strafeLeft", 0);
	TIMER_Set(ent, "smackTime", 0);
}

enum
{
	SPEECH_CHASE,
	SPEECH_CONFUSED,
	SPEECH_COVER,
	SPEECH_DETECTED,
	SPEECH_GIVEUP,
	SPEECH_LOOK,
	SPEECH_LOST,
	SPEECH_OUTFLANK,
	SPEECH_ESCAPING,
	SPEECH_SIGHT,
	SPEECH_SOUND,
	SPEECH_SUSPICIOUS,
	SPEECH_YELL,
	SPEECH_PUSHED
};

static void ST_Speech(const gentity_t* self, const int speech_type, const float fail_chance)
{
	if (Q_flrand(0.0f, 1.0f) < fail_chance)
	{
		return;
	}

	if (fail_chance >= 0)
	{
		//a negative failChance makes it always talk
		if (self->NPC->group)
		{
			//group AI speech debounce timer
			if (self->NPC->group->speechDebounceTime > level.time)
			{
				return;
			}
		}
		else if (!TIMER_Done(self, "chatter"))
		{
			//personal timer
			return;
		}
		else if (groupSpeechDebounceTime[self->client->playerTeam] > level.time)
		{
			//for those not in group AI
			return;
		}
	}

	if (self->NPC->group)
	{
		//So they don't all speak at once...
		self->NPC->group->speechDebounceTime = level.time + Q_irand(2000, 4000);
	}
	else
	{
		TIMER_Set(self, "chatter", Q_irand(2000, 4000));
	}
	groupSpeechDebounceTime[self->client->playerTeam] = level.time + Q_irand(2000, 4000);

	if (self->NPC->blockedSpeechDebounceTime > level.time)
	{
		return;
	}

	switch (speech_type)
	{
	case SPEECH_CHASE:
		G_AddVoiceEvent(self, Q_irand(EV_CHASE1, EV_CHASE3), 2000);
		break;
	case SPEECH_CONFUSED:
		G_AddVoiceEvent(self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000);
		break;
	case SPEECH_COVER:
		G_AddVoiceEvent(self, Q_irand(EV_COVER1, EV_COVER5), 2000);
		break;
	case SPEECH_DETECTED:
		G_AddVoiceEvent(self, Q_irand(EV_DETECTED1, EV_DETECTED5), 2000);
		break;
	case SPEECH_GIVEUP:
		G_AddVoiceEvent(self, Q_irand(EV_GIVEUP1, EV_GIVEUP4), 2000);
		break;
	case SPEECH_LOOK:
		G_AddVoiceEvent(self, Q_irand(EV_LOOK1, EV_LOOK2), 2000);
		break;
	case SPEECH_LOST:
		G_AddVoiceEvent(self, EV_LOST1, 2000);
		break;
	case SPEECH_OUTFLANK:
		G_AddVoiceEvent(self, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
		break;
	case SPEECH_ESCAPING:
		G_AddVoiceEvent(self, Q_irand(EV_ESCAPING1, EV_ESCAPING3), 2000);
		break;
	case SPEECH_SIGHT:
		G_AddVoiceEvent(self, Q_irand(EV_SIGHT1, EV_SIGHT3), 2000);
		break;
	case SPEECH_SOUND:
		G_AddVoiceEvent(self, Q_irand(EV_SOUND1, EV_SOUND3), 2000);
		break;
	case SPEECH_SUSPICIOUS:
		G_AddVoiceEvent(self, Q_irand(EV_SUSPICIOUS1, EV_SUSPICIOUS5), 2000);
		break;
	case SPEECH_YELL:
		G_AddVoiceEvent(self, Q_irand(EV_ANGER1, EV_ANGER3), 2000);
		break;
	case SPEECH_PUSHED:
		G_AddVoiceEvent(self, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
		break;
	default:
		break;
	}

	self->NPC->blockedSpeechDebounceTime = level.time + 2000;
}

void ST_MarkToCover(const gentity_t* self)
{
	if (!self || !self->NPC)
	{
		return;
	}
	self->NPC->localState = LSTATE_UNDERFIRE;
	TIMER_Set(self, "attackDelay", Q_irand(500, 2500));
	ST_AggressionAdjust(self, -3);
	if (self->NPC->group && self->NPC->group->numGroup > 1)
	{
		ST_Speech(self, SPEECH_COVER, 0); //FIXME: flee sound?
	}
}

void ST_StartFlee(gentity_t* self, gentity_t* enemy, vec3_t danger_point, const int danger_level, const int min_time,
	const int max_time)
{
	if (!self || !self->NPC)
	{
		return;
	}
	G_StartFlee(self, enemy, danger_point, danger_level, min_time, max_time);
	if (self->NPC->group && self->NPC->group->numGroup > 1)
	{
		ST_Speech(self, SPEECH_COVER, 0); //FIXME: flee sound?
	}
}

/*
-------------------------
NPC_ST_Pain
-------------------------
*/

void NPC_ST_Pain(gentity_t* self, gentity_t* attacker, const int damage)
{
	self->NPC->localState = LSTATE_UNDERFIRE;

	TIMER_Set(self, "duck", -1);
	TIMER_Set(self, "hideTime", -1);
	TIMER_Set(self, "stand", 2000);

	NPC_Pain(self, attacker, damage);

	if (!damage && self->health > 0)
	{
		//FIXME: better way to know I was pushed
		G_AddVoiceEvent(self, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
	}
}

/*
-------------------------
ST_HoldPosition
-------------------------
*/

static void ST_HoldPosition(void)
{
	if (NPCS.NPCInfo->squadState == SQUAD_RETREAT)
	{
		TIMER_Set(NPCS.NPC, "flee", -level.time);
	}
	TIMER_Set(NPCS.NPC, "verifyCP", Q_irand(1000, 3000)); //don't look for another one for a few seconds
	NPC_FreeCombatPoint(NPCS.NPCInfo->combatPoint, qtrue);

	if (!trap->ICARUS_TaskIDPending((sharedEntity_t*)NPCS.NPC, TID_MOVE_NAV))
	{
		//don't have a script waiting for me to get to my point, okay to stop trying and stand
		AI_GroupUpdateSquadstates(NPCS.NPCInfo->group, NPCS.NPC, SQUAD_STAND_AND_SHOOT);
		NPCS.NPCInfo->goalEntity = NULL;
	}
}

static void NPC_ST_SayMovementSpeech(void)
{
	if (!NPCS.NPCInfo->movementSpeech)
	{
		return;
	}
	if (NPCS.NPCInfo->group &&
		NPCS.NPCInfo->group->commander &&
		NPCS.NPCInfo->group->commander->client &&
		NPCS.NPCInfo->group->commander->client->NPC_class == CLASS_IMPERIAL &&
		!Q_irand(0, 3))
	{
		//imperial (commander) gives the order
		ST_Speech(NPCS.NPCInfo->group->commander, NPCS.NPCInfo->movementSpeech, NPCS.NPCInfo->movementSpeechChance);
	}
	else
	{
		//really don't want to say this unless we can actually get there...
		ST_Speech(NPCS.NPC, NPCS.NPCInfo->movementSpeech, NPCS.NPCInfo->movementSpeechChance);
	}

	NPCS.NPCInfo->movementSpeech = 0;
	NPCS.NPCInfo->movementSpeechChance = 0.0f;
}

static void NPC_ST_StoreMovementSpeech(const int speech, const float chance)
{
	NPCS.NPCInfo->movementSpeech = speech;
	NPCS.NPCInfo->movementSpeechChance = chance;
}

extern qboolean PM_InOnGroundAnim(const int anim);

static qboolean Melee_CanDoGrab(void)
{
	if (NPCS.NPC->client->NPC_class == CLASS_STORMTROOPER || NPCS.NPC->client->NPC_class == CLASS_CLONETROOPER)
	{
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

static void Melee_GrabEnemy(void)
{
	TIMER_Set(NPCS.NPC, "grabEnemyDebounce", NPCS.NPC->client->ps.torsoTimer + Q_irand(4000, 20000));
}

static void Melee_TryGrab(void)
{
	NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	NPCS.NPC->client->ps.torsoTimer += 200;
	NPCS.NPC->client->ps.weaponTime = NPCS.NPC->client->ps.torsoTimer;
	VectorClear(NPCS.NPC->client->ps.velocity);
	VectorClear(NPCS.NPC->client->ps.moveDir);
	NPCS.ucmd.rightmove = NPCS.ucmd.forwardmove = NPCS.ucmd.upmove = 0;
	NPCS.NPC->painDebounceTime = level.time + NPCS.NPC->client->ps.torsoTimer;
	//WTF?
	NPCS.NPC->client->ps.saberHolstered = 2;
}

/*
-------------------------
ST_Move
-------------------------
*/
void ST_TransferMoveGoal(const gentity_t* self, const gentity_t* other);

static qboolean ST_Move(void)
{
	navInfo_t info;

	NPCS.NPCInfo->combatMove = qtrue; //always move straight toward our goal

	const qboolean moved = NPC_MoveToGoal(qtrue);

	//Get the move info
	NAV_GetLastMove(&info);

	//FIXME: if we bump into another one of our guys and can't get around him, just stop!
	//If we hit our target, then stop and fire!
	if (info.flags & NIF_COLLISION)
	{
		if (info.blocker == NPCS.NPC->enemy)
		{
			ST_HoldPosition();
		}
	}

	//If our move failed, then reset
	if (moved == qfalse)
	{
		//FIXME: if we're going to a combat point, need to pick a different one
		if (!trap->ICARUS_TaskIDPending((sharedEntity_t*)NPCS.NPC, TID_MOVE_NAV))
		{
			//can't transfer movegoal or stop when a script we're running is waiting to complete
			if (info.blocker && info.blocker->NPC && NPCS.NPCInfo->group != NULL && info.blocker->NPC->group == NPCS.
				NPCInfo->group) //(NPCInfo->aiFlags&NPCAI_BLOCKED) && NPCInfo->group != NULL )
			{
				//dammit, something is in our way

				for (int j = 0; j < NPCS.NPCInfo->group->numGroup; j++)
				{
					if (NPCS.NPCInfo->group->member[j].number == NPCS.NPCInfo->blockingEntNum)
					{
						//we're being blocked by one of our own, pass our goal onto them and I'll stand still
						ST_TransferMoveGoal(NPCS.NPC, &g_entities[NPCS.NPCInfo->group->member[j].number]);
						break;
					}
				}
			}

			ST_HoldPosition();
		}
	}
	else
	{
		//First time you successfully move, say what it is you're doing
		NPC_ST_SayMovementSpeech();
	}

	return moved;
}

/*
-------------------------
NPC_ST_SleepShuffle
-------------------------
*/

static void NPC_ST_SleepShuffle(void)
{
	//Play an awake script if we have one
	if (G_ActivateBehavior(NPCS.NPC, BSET_AWAKE))
	{
		return;
	}

	//Automate some movement and noise
	if (TIMER_Done(NPCS.NPC, "shuffleTime"))
	{
		TIMER_Set(NPCS.NPC, "shuffleTime", 4000);
		TIMER_Set(NPCS.NPC, "sleepTime", 2000);
		return;
	}

	//They made another noise while we were stirring, see if we can see them
	if (TIMER_Done(NPCS.NPC, "sleepTime"))
	{
		NPC_CheckPlayerTeamStealth();
		TIMER_Set(NPCS.NPC, "sleepTime", 2000);
	}
}

/*
-------------------------
NPC_ST_Sleep
-------------------------
*/

void NPC_BSST_Sleep(void)
{
	const int alert_event = NPC_CheckAlertEvents(qfalse, qtrue, -1, qfalse, AEL_MINOR);
	//only check sounds since we're alseep!

	//There is an event we heard
	if (alert_event >= 0)
	{
		//See if it was enough to wake us up
		if (level.alertEvents[alert_event].level == AEL_DISCOVERED && NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			gentity_t* closestPlayer = FindClosestPlayer(NPCS.NPC->client->ps.origin, NPCS.NPC->client->enemyTeam);
			if (closestPlayer)
			{
				G_SetEnemy(NPCS.NPC, closestPlayer);
				return;
			}
		}

		//Otherwise just stir a bit
		NPC_ST_SleepShuffle();
	}
}

/*
-------------------------
NPC_CheckEnemyStealth
-------------------------
*/

static qboolean NPC_CheckEnemyStealth(gentity_t* target)
{
	float minDist = 40; //any closer than 40 and we definitely notice

	//In case we aquired one some other way
	if (NPCS.NPC->enemy != NULL)
		return qtrue;

	//Ignore notarget
	if (target->flags & FL_NOTARGET)
		return qfalse;

	if (target->health <= 0)
	{
		return qfalse;
	}

	if (target->client->ps.weapon == WP_SABER && !target->client->ps.saberHolstered && !target->client->ps.
		saberInFlight)
	{
		//if target has saber in hand and activated, we wake up even sooner even if not facing him
		minDist = 100;
	}

	float target_dist = DistanceSquared(target->r.currentOrigin, NPCS.NPC->r.currentOrigin);

	//If the target is this close, then wake up regardless
	if (!(target->client->ps.pm_flags & PMF_DUCKED)
		&& NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES
		&& target_dist < minDist * minDist)
	{
		G_SetEnemy(NPCS.NPC, target);
		NPCS.NPCInfo->enemyLastSeenTime = level.time;
		TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(500, 1500));
		return qtrue;
	}

	const float maxViewDist = NPCS.NPCInfo->stats.visrange;

	if (target_dist > maxViewDist * maxViewDist)
	{
		//out of possible visRange
		return qfalse;
	}

	//Check FOV first
	if (InFOV(target, NPCS.NPC, NPCS.NPCInfo->stats.hfov, NPCS.NPCInfo->stats.vfov) == qfalse)
		return qfalse;

	//clearLOS = ( target->client->ps.leanofs ) ? NPC_ClearLOS5( target->client->renderInfo.eyePoint ) : NPC_ClearLOS4( target );
	const qboolean clearLOS = NPC_ClearLOS4(target);

	//Now check for clear line of vision
	if (clearLOS)
	{
		vec3_t targ_org;
		float realize, cautious;

		if (target->client->NPC_class == CLASS_ATST)
		{
			//can't miss 'em!
			G_SetEnemy(NPCS.NPC, target);
			TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(500, 1500));
			return qtrue;
		}
		VectorSet(targ_org, target->r.currentOrigin[0], target->r.currentOrigin[1],
			target->r.currentOrigin[2] + target->r.maxs[2] - 4);
		float hAngle_perc = NPC_GetHFOVPercentage(targ_org, NPCS.NPC->client->renderInfo.eyePoint,
			NPCS.NPC->client->renderInfo.eyeAngles, NPCS.NPCInfo->stats.hfov);
		float vAngle_perc = NPC_GetVFOVPercentage(targ_org, NPCS.NPC->client->renderInfo.eyePoint,
			NPCS.NPC->client->renderInfo.eyeAngles, NPCS.NPCInfo->stats.vfov);

		//Scale them vertically some, and horizontally pretty harshly
		vAngle_perc *= vAngle_perc; //( vAngle_perc * vAngle_perc );
		hAngle_perc *= hAngle_perc * hAngle_perc;

		//Cap our vertical vision severely
		//if ( vAngle_perc <= 0.3f ) // was 0.5f
		//	return qfalse;

		//Assess the player's current status
		target_dist = Distance(target->r.currentOrigin, NPCS.NPC->r.currentOrigin);

		const float target_speed = VectorLength(target->client->ps.velocity);
		const int target_crouching = target->client->pers.cmd.upmove < 0;
		const float dist_rating = target_dist / maxViewDist;
		float speed_rating = target_speed / MAX_VIEW_SPEED;
		const float turning_rating = 5.0f;
		//AngleDelta( target->client->ps.viewangles[PITCH], target->lastAngles[PITCH] )/180.0f + AngleDelta( target->client->ps.viewangles[YAW], target->lastAngles[YAW] )/180.0f;
		const float light_level = 255 / MAX_LIGHT_INTENSITY; //( target->lightLevel / MAX_LIGHT_INTENSITY );
		const float FOV_perc = 1.0f - (hAngle_perc + vAngle_perc) * 0.5f; //FIXME: Dunno about the average...
		float vis_rating = 0.0f;

		//Too dark

		//Too close?
		if (dist_rating < DISTANCE_THRESHOLD)
		{
			G_SetEnemy(NPCS.NPC, target);
			TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(500, 1500));
			return qtrue;
		}

		//Out of range
		if (dist_rating > 1.0f)
			return qfalse;

		//Cap our speed checks
		if (speed_rating > 1.0f)
			speed_rating = 1.0f;

		//Calculate the distance, fov and light influences
		//...Visibilty linearly wanes over distance
		const float dist_influence = DISTANCE_SCALE * (1.0f - dist_rating);
		//...As the percentage out of the FOV increases, straight perception suffers on an exponential scale
		const float fov_influence = FOV_SCALE * (1.0f - FOV_perc);
		//...Lack of light hides, abundance of light exposes
		const float light_influence = (light_level - 0.5f) * LIGHT_SCALE;

		//Calculate our base rating
		float target_rating = dist_influence + fov_influence + light_influence;

		//Now award any final bonuses to this number
		const int contents = trap->PointContents(targ_org, target->s.number);
		if (contents & CONTENTS_WATER)
		{
			const int myContents = trap->PointContents(NPCS.NPC->client->renderInfo.eyePoint, NPCS.NPC->s.number);
			if (!(myContents & CONTENTS_WATER))
			{
				//I'm not in water
				if (NPCS.NPC->client->NPC_class == CLASS_SWAMPTROOPER)
				{
					//these guys can see in in/through water pretty well
					vis_rating = 0.10f; //10% bonus
				}
				else
				{
					vis_rating = 0.35f; //35% bonus
				}
			}
			else
			{
				//else, if we're both in water
				if (NPCS.NPC->client->NPC_class == CLASS_SWAMPTROOPER)
				{
					//I can see him just fine
				}
				else
				{
					vis_rating = 0.15f; //15% bonus
				}
			}
		}
		else
		{
			//not in water
			if (contents & CONTENTS_FOG)
			{
				vis_rating = 0.15f; //15% bonus
			}
		}

		target_rating *= 1.0f - vis_rating;

		//...Motion draws the eye quickly
		target_rating += speed_rating * SPEED_SCALE;
		target_rating += turning_rating * TURNING_SCALE;
		//FIXME: check to see if they're animating, too?  But can we do something as simple as frame != oldframe?

		//...Smaller targets are harder to indentify
		if (target_crouching)
		{
			target_rating *= 0.9f; //10% bonus
		}

		//If he's violated the threshold, then realize him
		//float difficulty_scale = 1.0f + (2.0f-g_npcspskill.value);//if playing on easy, 20% harder to be seen...?
		if (NPCS.NPC->client->NPC_class == CLASS_SWAMPTROOPER)
		{
			//swamptroopers can see much better
			realize = (float)CAUTIOUS_THRESHOLD/**difficulty_scale*/;
			cautious = (float)CAUTIOUS_THRESHOLD * 0.75f/**difficulty_scale*/;
		}
		else
		{
			realize = REALIZE_THRESHOLD
				/**difficulty_scale*/;
			cautious = (float)CAUTIOUS_THRESHOLD * 0.75f/**difficulty_scale*/;
		}

		if (target_rating > realize && NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			G_SetEnemy(NPCS.NPC, target);
			NPCS.NPCInfo->enemyLastSeenTime = level.time;
			TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(500, 1500));
			return qtrue;
		}

		//If he's above the caution threshold, then realize him in a few seconds unless he moves to cover
		if (target_rating > cautious && !(NPCS.NPCInfo->scriptFlags & SCF_IGNORE_ALERTS))
		{
			//FIXME: ambushing guys should never talk
			if (TIMER_Done(NPCS.NPC, "enemyLastVisible"))
			{
				//If we haven't already, start the counter
				const int lookTime = Q_irand(4500, 8500);
				//NPCInfo->timeEnemyLastVisible = level.time + 2000;
				TIMER_Set(NPCS.NPC, "enemyLastVisible", lookTime);
				ST_Speech(NPCS.NPC, SPEECH_SIGHT, 0);
				NPC_TempLookTarget(NPCS.NPC, target->s.number, lookTime, lookTime);
				//FIXME: set desired yaw and pitch towards this guy?
			}
			else if (TIMER_Get(NPCS.NPC, "enemyLastVisible") <= level.time + 500 && NPCS.NPCInfo->scriptFlags &
				SCF_LOOK_FOR_ENEMIES) //FIXME: Is this reliable?
			{
				if (NPCS.NPCInfo->rank < RANK_LT && !Q_irand(0, 2))
				{
					const int interrogateTime = Q_irand(2000, 4000);
					ST_Speech(NPCS.NPC, SPEECH_SUSPICIOUS, 0);
					TIMER_Set(NPCS.NPC, "interrogating", interrogateTime);
					G_SetEnemy(NPCS.NPC, target);
					NPCS.NPCInfo->enemyLastSeenTime = level.time;
					TIMER_Set(NPCS.NPC, "attackDelay", interrogateTime);
					TIMER_Set(NPCS.NPC, "stand", interrogateTime);
				}
				else
				{
					G_SetEnemy(NPCS.NPC, target);
					NPCS.NPCInfo->enemyLastSeenTime = level.time;
					//FIXME: ambush guys (like those popping out of water) shouldn't delay...
					TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(500, 1500));
					TIMER_Set(NPCS.NPC, "stand", Q_irand(500, 2500));
				}
				return qtrue;
			}

			return qfalse;
		}
	}

	return qfalse;
}

qboolean NPC_CheckPlayerTeamStealth(void)
{
	for (int i = 0; i < ENTITYNUM_WORLD; i++)
	{
		gentity_t* enemy = &g_entities[i];

		if (!enemy->inuse)
		{
			continue;
		}

		if (enemy && enemy->client && NPC_ValidEnemy(enemy) && enemy->client->playerTeam == NPCS.NPC->client->enemyTeam)
		{
			if (NPC_CheckEnemyStealth(enemy)) //Change this pointer to assess other entities
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

extern float Q_flrand(float min, float max);

static qboolean NPC_CheckEnemiesInSpotlight(void)
{
	//racc = check for enemies in our immediate eyesight?
	int entity_list[MAX_GENTITIES];
	gentity_t* suspect = NULL;
	int i;
	vec3_t mins, maxs;

	for (i = 0; i < 3; i++)
	{
		mins[i] = NPCS.NPC->client->renderInfo.eyePoint[i] - NPCS.NPC->speed;
		maxs[i] = NPCS.NPC->client->renderInfo.eyePoint[i] + NPCS.NPC->speed;
	}

	const int num_listed_entities = trap->EntitiesInBox(mins, maxs, entity_list, MAX_GENTITIES);

	for (i = 0; i < num_listed_entities; i++)
	{
		gentity_t* enemy = &g_entities[entity_list[i]];

		if (!enemy->inuse)
			continue;

		if (enemy && enemy->client && NPC_ValidEnemy(enemy) && enemy->client->playerTeam == NPCS.NPC->client->enemyTeam)
		{
			//valid ent & client, valid enemy, on the target team
			//check to see if they're in my FOV
			if (InFOV3(enemy->r.currentOrigin, NPCS.NPC->client->renderInfo.eyePoint,
				NPCS.NPC->client->renderInfo.eyeAngles, NPCS.NPCInfo->stats.hfov, NPCS.NPCInfo->stats.vfov))
			{
				//in my cone
				//check to see that they're close enough
				if (DistanceSquared(NPCS.NPC->client->renderInfo.eyePoint, enemy->r.currentOrigin) - 256 <= NPCS.NPC->
					speed * NPCS.NPC->speed)
				{
					//within range
					//check to see if we have a clear trace to them
					if (G_ClearLOS4(NPCS.NPC, enemy))
					{
						//clear LOS
						G_SetEnemy(NPCS.NPC, enemy);
						TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(500, 1500));
						return qtrue;
					}
				}
			}
			if (InFOV3(enemy->r.currentOrigin, NPCS.NPC->client->renderInfo.eyePoint,
				NPCS.NPC->client->renderInfo.eyeAngles, 90, NPCS.NPCInfo->stats.vfov * 3))
			{
				//one to look at if we don't get an enemy
				if (G_ClearLOS4(NPCS.NPC, enemy))
				{
					//clear LOS
					if (suspect == NULL || DistanceSquared(NPCS.NPC->client->renderInfo.eyePoint,
						enemy->r.currentOrigin) < DistanceSquared(
							NPCS.NPC->client->renderInfo.eyePoint, suspect->r.currentOrigin))
					{
						//remember him
						suspect = enemy;
					}
				}
			}
		}
	}
	if (suspect && Q_flrand(0, NPCS.NPCInfo->stats.visrange * NPCS.NPCInfo->stats.visrange) > DistanceSquared(
		NPCS.NPC->client->renderInfo.eyePoint, suspect->r.currentOrigin))
	{
		//hey!  who's that?
		if (TIMER_Done(NPCS.NPC, "enemyLastVisible"))
		{
			//If we haven't already, start the counter
			const int lookTime = Q_irand(4500, 8500);
			//NPCInfo->timeEnemyLastVisible = level.time + 2000;
			TIMER_Set(NPCS.NPC, "enemyLastVisible", lookTime);
			ST_Speech(NPCS.NPC, SPEECH_SIGHT, 0);
			NPC_FacePosition(suspect->r.currentOrigin, qtrue);
		}
		else if (TIMER_Get(NPCS.NPC, "enemyLastVisible") <= level.time + 500
			&& NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES) //FIXME: Is this reliable?
		{
			if (!Q_irand(0, 2))
			{
				const int interrogateTime = Q_irand(2000, 4000);
				ST_Speech(NPCS.NPC, SPEECH_SUSPICIOUS, 0);
				TIMER_Set(NPCS.NPC, "interrogating", interrogateTime);
				NPC_FacePosition(suspect->r.currentOrigin, qtrue);
			}
		}
	}
	return qfalse;
}

/*
-------------------------
NPC_ST_InvestigateEvent
-------------------------
*/

#define	MAX_CHECK_THRESHOLD	1

static qboolean NPC_ST_InvestigateEvent(const int eventID, const qboolean extraSuspicious)
{
	//If they've given themselves away, just take them as an enemy
	if (NPCS.NPCInfo->confusionTime < level.time)
	{
		if (level.alertEvents[eventID].level == AEL_DISCOVERED && NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			if (!level.alertEvents[eventID].owner ||
				!level.alertEvents[eventID].owner->client ||
				level.alertEvents[eventID].owner->health <= 0 ||
				level.alertEvents[eventID].owner->client->playerTeam != NPCS.NPC->client->enemyTeam)
			{
				//not an enemy
				return qfalse;
			}
			//FIXME: what if can't actually see enemy, don't know where he is... should we make them just become very alert and start looking for him?  Or just let combat AI handle this... (act as if you lost him)
			ST_Speech(NPCS.NPC, SPEECH_CHASE, 0);
			G_SetEnemy(NPCS.NPC, level.alertEvents[eventID].owner);
			NPCS.NPCInfo->enemyLastSeenTime = level.time;
			TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(500, 1500));
			if (level.alertEvents[eventID].type == AET_SOUND)
			{
				//heard him, didn't see him, stick for a bit
				TIMER_Set(NPCS.NPC, "roamTime", Q_irand(500, 2500));
			}
			return qtrue;
		}
	}

	//don't look at the same alert twice
	if (level.alertEvents[eventID].ID == NPCS.NPCInfo->lastAlertID)
	{
		return qfalse;
	}
	NPCS.NPCInfo->lastAlertID = level.alertEvents[eventID].ID;

	if (level.alertEvents[eventID].type == AET_SIGHT)
	{
		//sight alert, check the light level
		if (level.alertEvents[eventID].light < Q_irand(ST_MIN_LIGHT_THRESHOLD, ST_MAX_LIGHT_THRESHOLD))
		{
			//below my threshhold of potentially seeing
			return qfalse;
		}
	}

	//Save the position for movement (if necessary)
	VectorCopy(level.alertEvents[eventID].position, NPCS.NPCInfo->investigateGoal);

	//First awareness of it
	NPCS.NPCInfo->investigateCount += extraSuspicious ? 2 : 1;

	//Clamp the value
	if (NPCS.NPCInfo->investigateCount > 4)
		NPCS.NPCInfo->investigateCount = 4;

	//See if we should walk over and investigate
	if (level.alertEvents[eventID].level > AEL_MINOR && NPCS.NPCInfo->investigateCount > 1 && NPCS.NPCInfo->scriptFlags
		& SCF_CHASE_ENEMIES)
	{
		//make it so they can walk right to this point and look at it rather than having to use combatPoints
		if (G_ExpandPointToBBox(NPCS.NPCInfo->investigateGoal, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, NPCS.NPC->s.number,
			NPCS.NPC->clipmask & ~CONTENTS_BODY | CONTENTS_BOTCLIP))
		{
			//we were able to move the investigateGoal to a point in which our bbox would fit
			//drop the goal to the ground so we can get at it
			vec3_t end;
			trace_t trace;
			VectorCopy(NPCS.NPCInfo->investigateGoal, end);
			end[2] -= 512; //FIXME: not always right?  What if it's even higher, somehow?
			trap->Trace(&trace, NPCS.NPCInfo->investigateGoal, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, end, ENTITYNUM_NONE,
				NPCS.NPC->clipmask & ~CONTENTS_BODY | CONTENTS_BOTCLIP, qfalse, 0, 0);
			if (trace.fraction >= 1.0f)
			{
				//too high to even bother
				//FIXME: look at them???
			}
			else
			{
				VectorCopy(trace.endpos, NPCS.NPCInfo->investigateGoal);
				NPC_SetMoveGoal(NPCS.NPC, NPCS.NPCInfo->investigateGoal, 16, qtrue, -1, NULL);
				NPCS.NPCInfo->localState = LSTATE_INVESTIGATE;
			}
		}
		else
		{
			const int id = NPC_FindCombatPoint(NPCS.NPCInfo->investigateGoal, NPCS.NPCInfo->investigateGoal,
				NPCS.NPCInfo->investigateGoal, CP_INVESTIGATE | CP_HAS_ROUTE, 0, -1);

			if (id != -1)
			{
				NPC_SetMoveGoal(NPCS.NPC, level.combatPoints[id].origin, 16, qtrue, id, NULL);
				NPCS.NPCInfo->localState = LSTATE_INVESTIGATE;
			}
		}
		//Say something
		//FIXME: only if have others in group... these should be responses?
		if (NPCS.NPCInfo->investigateDebounceTime + NPCS.NPCInfo->pauseTime > level.time)
		{
			//was already investigating
			if (NPCS.NPCInfo->group &&
				NPCS.NPCInfo->group->commander &&
				NPCS.NPCInfo->group->commander->client &&
				NPCS.NPCInfo->group->commander->client->NPC_class == CLASS_IMPERIAL &&
				!Q_irand(0, 3))
			{
				ST_Speech(NPCS.NPCInfo->group->commander, SPEECH_LOOK, 0); //FIXME: "I'll go check it out" type sounds
			}
			else
			{
				ST_Speech(NPCS.NPC, SPEECH_LOOK, 0); //FIXME: "I'll go check it out" type sounds
			}
		}
		else
		{
			if (level.alertEvents[eventID].type == AET_SIGHT)
			{
				ST_Speech(NPCS.NPC, SPEECH_SIGHT, 0);
			}
			else if (level.alertEvents[eventID].type == AET_SOUND)
			{
				ST_Speech(NPCS.NPC, SPEECH_SOUND, 0);
			}
		}
		//Setup the debounce info
		NPCS.NPCInfo->investigateDebounceTime = NPCS.NPCInfo->investigateCount * 5000;
		NPCS.NPCInfo->investigateSoundDebounceTime = level.time + 2000;
		NPCS.NPCInfo->pauseTime = level.time;
	}
	else
	{
		//just look?
		//Say something
		if (level.alertEvents[eventID].type == AET_SIGHT)
		{
			ST_Speech(NPCS.NPC, SPEECH_SIGHT, 0);
		}
		else if (level.alertEvents[eventID].type == AET_SOUND)
		{
			ST_Speech(NPCS.NPC, SPEECH_SOUND, 0);
		}
		//Setup the debounce info
		NPCS.NPCInfo->investigateDebounceTime = NPCS.NPCInfo->investigateCount * 1000;
		NPCS.NPCInfo->investigateSoundDebounceTime = level.time + 1000;
		NPCS.NPCInfo->pauseTime = level.time;
		VectorCopy(level.alertEvents[eventID].position, NPCS.NPCInfo->investigateGoal);

		if (NPCS.NPC->client->NPC_class == CLASS_ROCKETTROOPER
			&& !RT_Flying(NPCS.NPC))
		{
			NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}

	if (level.alertEvents[eventID].level >= AEL_DANGER)
	{
		NPCS.NPCInfo->investigateDebounceTime = Q_irand(500, 2500);
	}

	//Start investigating
	NPCS.NPCInfo->tempBehavior = BS_INVESTIGATE;
	return qtrue;
}

/*
-------------------------
ST_OffsetLook
-------------------------
*/

static void ST_OffsetLook(const float offset, vec3_t out)
{
	vec3_t angles, forward, temp;

	GetAnglesForDirection(NPCS.NPC->r.currentOrigin, NPCS.NPCInfo->investigateGoal, angles);
	angles[YAW] += offset;
	AngleVectors(angles, forward, NULL, NULL);
	VectorMA(NPCS.NPC->r.currentOrigin, 64, forward, out);

	CalcEntitySpot(NPCS.NPC, SPOT_HEAD, temp);
	out[2] = temp[2];
}

/*
-------------------------
ST_LookAround
-------------------------
*/

static void ST_LookAround(void)
{
	vec3_t lookPos;
	const float perc = (float)(level.time - NPCS.NPCInfo->pauseTime) / (float)NPCS.NPCInfo->investigateDebounceTime;

	//Keep looking at the spot
	if (perc < 0.25)
	{
		VectorCopy(NPCS.NPCInfo->investigateGoal, lookPos);
	}
	else if (perc < 0.5f) //Look up but straight ahead
	{
		ST_OffsetLook(0.0f, lookPos);
	}
	else if (perc < 0.75f) //Look right
	{
		ST_OffsetLook(45.0f, lookPos);
	}
	else //Look left
	{
		ST_OffsetLook(-45.0f, lookPos);
	}

	NPC_FacePosition(lookPos, qtrue);
}

/*
-------------------------
NPC_BSST_Investigate
-------------------------
*/

void NPC_BSST_Investigate(void)
{
	//get group- mainly for group speech debouncing, but may use for group scouting/investigating AI, too
	AI_GetGroup(NPCS.NPC);

	if (NPCS.NPCInfo->scriptFlags & SCF_FIRE_WEAPON)
	{
		WeaponThink();
	}

	if (NPCS.NPCInfo->confusionTime < level.time)
	{
		if (NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			//Look for an enemy
			if (NPC_CheckPlayerTeamStealth())
			{
				//NPCInfo->behaviorState	= BS_HUNT_AND_KILL;//should be auto now
				ST_Speech(NPCS.NPC, SPEECH_DETECTED, 0);
				NPCS.NPCInfo->tempBehavior = BS_DEFAULT;
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}

	if (!(NPCS.NPCInfo->scriptFlags & SCF_IGNORE_ALERTS))
	{
		const int alert_event = NPC_CheckAlertEvents(qtrue, qtrue, NPCS.NPCInfo->lastAlertID, qfalse, AEL_MINOR);

		//There is an event to look at
		if (alert_event >= 0)
		{
			if (NPCS.NPCInfo->confusionTime < level.time)
			{
				if (NPC_CheckForDanger(alert_event))
				{
					//running like hell
					ST_Speech(NPCS.NPC, SPEECH_COVER, 0); //FIXME: flee sound?
					return;
				}
			}

			if (level.alertEvents[alert_event].ID != NPCS.NPCInfo->lastAlertID)
			{
				NPC_ST_InvestigateEvent(alert_event, qtrue);
			}
		}
	}

	//If we're done looking, then just return to what we were doing
	if (NPCS.NPCInfo->investigateDebounceTime + NPCS.NPCInfo->pauseTime < level.time)
	{
		NPCS.NPCInfo->tempBehavior = BS_DEFAULT;
		NPCS.NPCInfo->goalEntity = UpdateGoal();

		NPC_UpdateAngles(qtrue, qtrue);
		//Say something
		ST_Speech(NPCS.NPC, SPEECH_GIVEUP, 0);
		return;
	}

	//FIXME: else, look for new alerts

	//See if we're searching for the noise's origin
	if (NPCS.NPCInfo->localState == LSTATE_INVESTIGATE && NPCS.NPCInfo->goalEntity != NULL)
	{
		//See if we're there
		if (NAV_HitNavGoal(NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs,
			NPCS.NPCInfo->goalEntity->r.currentOrigin, 32, FlyingCreature(NPCS.NPC)) == qfalse)
		{
			NPCS.ucmd.buttons |= BUTTON_WALKING;

			//Try and move there
			if (NPC_MoveToGoal(qtrue))
			{
				//Bump our times
				NPCS.NPCInfo->investigateDebounceTime = NPCS.NPCInfo->investigateCount * 5000;
				NPCS.NPCInfo->pauseTime = level.time;

				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
		ST_Speech(NPCS.NPC, SPEECH_LOOK, 0.33f);
		NPCS.NPCInfo->localState = LSTATE_NONE;
	}

	//Look around
	ST_LookAround();
}

/*
-------------------------
NPC_BSST_Patrol
-------------------------
*/

void NPC_BSST_Patrol(void)
{
	//FIXME: pick up on bodies of dead buddies?
	//Not a scriptflag, but...
	if (NPCS.NPC->client->NPC_class == CLASS_ROCKETTROOPER && NPCS.NPC->client->ps.eFlags & EF_SPOTLIGHT)
	{
		//using spotlight search mode
		vec3_t eyeFwd, end;
		const vec3_t maxs = { 2, 2, 2 };
		const vec3_t mins = { -2, -2, -2 };
		trace_t trace;
		AngleVectors(NPCS.NPC->client->renderInfo.eyeAngles, eyeFwd, NULL, NULL);
		VectorMA(NPCS.NPC->client->renderInfo.eyePoint, NPCS.NPCInfo->stats.visrange, eyeFwd, end);
		//get server-side trace impact point
		trap->Trace(&trace, NPCS.NPC->client->renderInfo.eyePoint, mins, maxs, end, NPCS.NPC->s.number,
			MASK_OPAQUE | CONTENTS_BODY | CONTENTS_CORPSE, qfalse, 0, 0);
		NPCS.NPC->speed = trace.fraction * NPCS.NPCInfo->stats.visrange;
		if (NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			//racc - looking for entities.
			//FIXME: do a FOV cone check, then a trace
			if (trace.entityNum < ENTITYNUM_WORLD)
			{
				//hit something
				//try cheap check first
				gentity_t* enemy = &g_entities[trace.entityNum];
				if (enemy && enemy->client && NPC_ValidEnemy(enemy) && enemy->client->playerTeam == NPCS.NPC->client->
					enemyTeam)
				{
					//racc - found someone.
					G_SetEnemy(NPCS.NPC, enemy);
					TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(500, 1500));
					NPC_UpdateAngles(qtrue, qtrue);
					return;
				}
			}
			if (NPC_CheckEnemiesInSpotlight())
			{
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}
	else
	{
		//get group- mainly for group speech debouncing, but may use for group scouting/investigating AI, too
		AI_GetGroup(NPCS.NPC);

		if (NPCS.NPCInfo->confusionTime < level.time)
		{
			//Look for any enemies
			if (NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
			{
				if (NPC_CheckPlayerTeamStealth())
				{
					NPC_UpdateAngles(qtrue, qtrue);
					return;
				}
			}
		}
	}

	if (!(NPCS.NPCInfo->scriptFlags & SCF_IGNORE_ALERTS))
	{
		const int alert_event = NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_MINOR);

		//There is an event to look at
		if (alert_event >= 0)
		{
			if (NPC_CheckForDanger(alert_event))
			{
				//going to run?
				ST_Speech(NPCS.NPC, SPEECH_COVER, 0);
				return;
			}
			if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
			{
				if (!level.alertEvents[alert_event].owner ||
					!level.alertEvents[alert_event].owner->client ||
					level.alertEvents[alert_event].owner->health <= 0 ||
					level.alertEvents[alert_event].owner->client->playerTeam != NPCS.NPC->client->enemyTeam)
				{
					//not an enemy
					return;
				}
				ST_Speech(NPCS.NPC, SPEECH_CHASE, 0);
				G_SetEnemy(NPCS.NPC, level.alertEvents[alert_event].owner);
				NPCS.NPCInfo->enemyLastSeenTime = level.time;
				TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(500, 1500));
				return;
			}
			if (NPC_ST_InvestigateEvent(alert_event, qfalse))
			{
				//actually going to investigate it
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}

	//If we have somewhere to go, then do that
	if (UpdateGoal())
	{
		NPCS.ucmd.buttons |= BUTTON_WALKING;
		NPC_MoveToGoal(qtrue);
	}
	else
	{
		if (TIMER_Done(NPCS.NPC, "enemyLastVisible"))
		{
			//nothing suspicious, look around
			if (!Q_irand(0, 30))
			{
				NPCS.NPCInfo->desiredYaw = NPCS.NPC->s.angles[1] + Q_irand(-90, 90);
			}
			if (!Q_irand(0, 30))
			{
				NPCS.NPCInfo->desiredPitch = Q_irand(-20, 20);
			}
		}
	}

	NPC_UpdateAngles(qtrue, qtrue);
	//TEMP hack for Imperial stand anim
	if (NPCS.NPC->client->NPC_class == CLASS_IMPERIAL
		|| NPCS.NPC->client->NPC_class == CLASS_IMPWORKER)
	{
		//hack
		if (NPCS.NPC->client->ps.weapon != WP_CONCUSSION)
		{
			//not Rax
			if (NPCS.ucmd.forwardmove || NPCS.ucmd.rightmove || NPCS.ucmd.upmove)
			{
				//moving
				if (!NPCS.NPC->client->ps.torsoTimer || NPCS.NPC->client->ps.torsoAnim == BOTH_STAND4)
				{
					if (NPCS.ucmd.buttons & BUTTON_WALKING && !(NPCS.NPCInfo->scriptFlags & SCF_RUNNING))
					{
						//not running, only set upper anim
						//  No longer overrides scripted anims
						NPC_SetAnim(NPCS.NPC, SETANIM_TORSO, BOTH_STAND4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						NPCS.NPC->client->ps.torsoTimer = 200;
					}
				}
			}
			else
			{
				//standing still, set both torso and legs anim
				//  No longer overrides scripted anims
				if ((!NPCS.NPC->client->ps.torsoTimer || NPCS.NPC->client->ps.torsoAnim == BOTH_STAND4) &&
					(!NPCS.NPC->client->ps.legsTimer || NPCS.NPC->client->ps.legsAnim == BOTH_STAND4))
				{
					NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_STAND4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					NPCS.NPC->client->ps.torsoTimer = NPCS.NPC->client->ps.legsTimer = 200;
				}
			}
			//FIXME: this is a disgusting hack that is supposed to make the Imperials start with their weapon holstered- need a better way
			if (NPCS.NPC->client->ps.weapon != WP_NONE)
			{
				ChangeWeapon(NPCS.NPC, WP_NONE);
				NPCS.NPC->client->ps.weapon = WP_NONE;
				NPCS.NPC->client->ps.weaponstate = WEAPON_READY;
			}
		}
	}
}

/*
-------------------------
ST_CheckMoveState
-------------------------
*/

static void ST_CheckMoveState(void)
{
	if (trap->ICARUS_TaskIDPending((sharedEntity_t*)NPCS.NPC, TID_MOVE_NAV))
	{
		//moving toward a goal that a script is waiting on, so don't stop for anything!
		move = qtrue;
	}
	else if (NPCS.NPC->client->NPC_class == CLASS_ROCKETTROOPER
		&& NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//no squad stuff
		return;
	}
	else if (NPCS.NPC->NPC->scriptFlags & SCF_NO_GROUPS)
	{
		move = qtrue;
	}
	//See if we're a scout
	else if (NPCS.NPCInfo->squadState == SQUAD_SCOUT)
	{
		//If we're supposed to stay put, then stand there and fire
		if (TIMER_Done(NPCS.NPC, "stick") == qfalse)
		{
			move = qfalse;
			return;
		}

		//Otherwise, if we can see our target, just shoot
		if (enemyLOS)
		{
			if (enemyCS)
			{
				//if we're going after our enemy, we can stop now
				if (NPCS.NPCInfo->goalEntity == NPCS.NPC->enemy)
				{
					AI_GroupUpdateSquadstates(NPCS.NPCInfo->group, NPCS.NPC, SQUAD_STAND_AND_SHOOT);
					move = qfalse;
					return;
				}
			}
		}
		else
		{
			//Move to find our target
			faceEnemy = qfalse;
		}
	}
	//See if we're running away
	else if (NPCS.NPCInfo->squadState == SQUAD_RETREAT)
	{
		if (NPCS.NPCInfo->goalEntity)
		{
			faceEnemy = qfalse;
		}
		else
		{
			//um, lost our goal?  Just stand and shoot, then
			NPCS.NPCInfo->squadState = SQUAD_STAND_AND_SHOOT;
		}
	}
	//see if we're heading to some other combatPoint
	else if (NPCS.NPCInfo->squadState == SQUAD_TRANSITION)
	{
		//ucmd.buttons |= BUTTON_CAREFUL;
		if (!NPCS.NPCInfo->goalEntity)
		{
			//um, lost our goal?  Just stand and shoot, then
			NPCS.NPCInfo->squadState = SQUAD_STAND_AND_SHOOT;
		}
	}
	//see if we're at point, duck and fire
	else if (NPCS.NPCInfo->squadState == SQUAD_POINT)
	{
		if (TIMER_Done(NPCS.NPC, "stick"))
		{
			AI_GroupUpdateSquadstates(NPCS.NPCInfo->group, NPCS.NPC, SQUAD_STAND_AND_SHOOT);
			return;
		}

		move = qfalse;
		return;
	}
	//see if we're just standing around
	else if (NPCS.NPCInfo->squadState == SQUAD_STAND_AND_SHOOT)
	{
		//from this squadState we can transition to others?
		move = qfalse;
		return;
	}
	//see if we're hiding
	else if (NPCS.NPCInfo->squadState == SQUAD_COVER)
	{
		//Should we duck?
		move = qfalse;
		return;
	}
	//see if we're just standing around
	else if (NPCS.NPCInfo->squadState == SQUAD_IDLE)
	{
		if (!NPCS.NPCInfo->goalEntity)
		{
			move = qfalse;
			return;
		}
	}
	//??
	else
	{
		//invalid squadState!
	}

	//See if we're moving towards a goal, not the enemy
	if (NPCS.NPCInfo->goalEntity != NPCS.NPC->enemy && NPCS.NPCInfo->goalEntity != NULL)
	{
		//Did we make it?
		if (NAV_HitNavGoal(NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs,
			NPCS.NPCInfo->goalEntity->r.currentOrigin, 16, FlyingCreature(NPCS.NPC)) ||
			enemyLOS && NPCS.NPCInfo->aiFlags & NPCAI_STOP_AT_LOS &&
			!trap->ICARUS_TaskIDPending((sharedEntity_t*)NPCS.NPC, TID_MOVE_NAV))
		{
			//either hit our navgoal or our navgoal was not a crucial (scripted) one (maybe a combat point) and we're scouting and found our enemy
			int newSquadState = SQUAD_STAND_AND_SHOOT;
			//we got where we wanted to go, set timers based on why we were running
			switch (NPCS.NPCInfo->squadState)
			{
			case SQUAD_RETREAT: //was running away
				//done fleeing, obviously
				TIMER_Set(NPCS.NPC, "duck", (NPCS.NPC->client->pers.maxHealth - NPCS.NPC->health) * 100);
				TIMER_Set(NPCS.NPC, "hideTime", Q_irand(3000, 7000));
				TIMER_Set(NPCS.NPC, "flee", -level.time);
				newSquadState = SQUAD_COVER;
				break;
			case SQUAD_TRANSITION: //was heading for a combat point
				TIMER_Set(NPCS.NPC, "hideTime", Q_irand(2000, 4000));
				break;
			case SQUAD_SCOUT: //was running after player
				break;
			default:
				break;
			}
			AI_GroupUpdateSquadstates(NPCS.NPCInfo->group, NPCS.NPC, newSquadState);
			NPC_ReachedGoal();
			//don't attack right away
			TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(250, 500)); //FIXME: Slant for difficulty levels
			//don't do something else just yet

			// THIS IS THE ONE TRUE PLACE WHERE ROAM TIME IS SET
			TIMER_Set(NPCS.NPC, "roamTime", Q_irand(8000, 15000));
			if (Q_irand(0, 3) == 0)
			{
				TIMER_Set(NPCS.NPC, "duck", Q_irand(5000, 10000)); // just reached our goal, chance of ducking now
			}
			return;
		}

		//keep going, hold of roamTimer until we get there
		TIMER_Set(NPCS.NPC, "roamTime", Q_irand(8000, 9000));
	}
}

static void ST_ResolveBlockedShot(const int hit)
{
	int stuck_time;

	//figure out how long we intend to stand here, max
	if (TIMER_Get(NPCS.NPC, "roamTime") > TIMER_Get(NPCS.NPC, "stick"))
	{
		stuck_time = TIMER_Get(NPCS.NPC, "roamTime") - level.time;
	}
	else
	{
		stuck_time = TIMER_Get(NPCS.NPC, "stick") - level.time;
	}

	if (TIMER_Done(NPCS.NPC, "duck"))
	{
		//we're not ducking
		if (AI_GroupContainsEntNum(NPCS.NPCInfo->group, hit))
		{
			const gentity_t* member = &g_entities[hit];
			if (TIMER_Done(member, "duck"))
			{
				//they aren't ducking
				if (TIMER_Done(member, "stand"))
				{
					//they're not being forced to stand
					//tell them to duck at least as long as I'm not moving
					TIMER_Set(member, "duck", stuck_time);
					return;
				}
			}
		}
	}
	else
	{
		//maybe we should stand
		if (TIMER_Done(NPCS.NPC, "stand"))
		{
			//stand for as long as we'll be here
			TIMER_Set(NPCS.NPC, "stand", stuck_time);
			return;
		}
	}
	//Hmm, can't resolve this by telling them to duck or telling me to stand
	//We need to move!
	TIMER_Set(NPCS.NPC, "roamTime", -1);
	TIMER_Set(NPCS.NPC, "stick", -1);
	TIMER_Set(NPCS.NPC, "duck", -1);
	TIMER_Set(NPCS.NPC, "attakDelay", Q_irand(1000, 3000));
}

/*
-------------------------
ST_CheckFireState
-------------------------
*/

static void ST_CheckFireState(void)
{
	if (enemyCS)
	{
		//if have a clear shot, always try
		return;
	}

	if (NPCS.NPCInfo->squadState == SQUAD_RETREAT || NPCS.NPCInfo->squadState == SQUAD_TRANSITION || NPCS.NPCInfo->
		squadState == SQUAD_SCOUT)
	{
		//runners never try to fire at the last pos
		return;
	}

	if (!VectorCompare(NPCS.NPC->client->ps.velocity, vec3_origin))
	{
		//if moving at all, don't do this
		return;
	}

	//See if we should continue to fire on their last position
	//!TIMER_Done( NPC, "stick" ) ||
	if (!hitAlly //we're not going to hit an ally
		&& enemyInFOV //enemy is in our FOV //FIXME: or we don't have a clear LOS?
		&& NPCS.NPCInfo->enemyLastSeenTime > 0 //we've seen the enemy
		&& NPCS.NPCInfo->group //have a group
		&& (NPCS.NPCInfo->group->numState[SQUAD_RETREAT] > 0 || NPCS.NPCInfo->group->numState[SQUAD_TRANSITION] > 0 ||
			NPCS.NPCInfo->group->numState[SQUAD_SCOUT] > 0)) //laying down covering fire
	{
		if (level.time - NPCS.NPCInfo->enemyLastSeenTime < 10000 && //we have seem the enemy in the last 10 seconds
			(!NPCS.NPCInfo->group || level.time - NPCS.NPCInfo->group->lastSeenEnemyTime < 10000))
			//we are not in a group or the group has seen the enemy in the last 10 seconds
		{
			if (!Q_irand(0, 10))
			{
				//Fire on the last known position
				vec3_t muzzle;
				qboolean too_close = qfalse;
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
					trap->Trace(&tr, muzzle, vec3_origin, vec3_origin, end, NPCS.NPC->s.number, MASK_SHOT, qfalse, 0,
						0);
					VectorCopy(tr.endpos, impactPos);
				}

				//see if impact would be too close to me
				float dist_threshold = 16384/*128*128*/; //default
				switch (NPCS.NPC->s.weapon)
				{
				case WP_ROCKET_LAUNCHER:
				case WP_FLECHETTE:
				case WP_THERMAL:
				case WP_TRIP_MINE:
				case WP_DET_PACK:
					dist_threshold = 65536/*256*256*/;
					break;
				case WP_REPEATER:
					if (NPCS.NPCInfo->scriptFlags & SCF_altFire)
					{
						dist_threshold = 65536/*256*256*/;
					}
					break;
				case WP_CONCUSSION:
					if (!(NPCS.NPCInfo->scriptFlags & SCF_altFire))
					{
						dist_threshold = 65536/*256*256*/;
					}
					break;
				default:
					break;
				}

				float dist = DistanceSquared(impactPos, muzzle);

				if (dist < dist_threshold)
				{
					//impact would be too close to me
					too_close = qtrue;
				}
				else if (level.time - NPCS.NPCInfo->enemyLastSeenTime > 5000 ||
					NPCS.NPCInfo->group && level.time - NPCS.NPCInfo->group->lastSeenEnemyTime > 5000)
				{
					//we've haven't seen them in the last 5 seconds
					//see if it's too far from where he is
					dist_threshold = 65536/*256*256*/; //default
					switch (NPCS.NPC->s.weapon)
					{
					case WP_ROCKET_LAUNCHER:
					case WP_FLECHETTE:
					case WP_THERMAL:
					case WP_TRIP_MINE:
					case WP_DET_PACK:
						dist_threshold = 262144/*512*512*/;
						break;
					case WP_REPEATER:
						if (NPCS.NPCInfo->scriptFlags & SCF_altFire)
						{
							dist_threshold = 262144/*512*512*/;
						}
						break;
					case WP_CONCUSSION:
						if (!(NPCS.NPCInfo->scriptFlags & SCF_altFire))
						{
							dist_threshold = 262144/*512*512*/;
						}
						break;
					default:
						break;
					}
					dist = DistanceSquared(impactPos, NPCS.NPCInfo->enemyLastSeenLocation);
					if (dist > dist_threshold)
					{
						//impact would be too far from enemy
						tooFar = qtrue;
					}
				}

				if (!too_close && !tooFar)
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
					faceEnemy = qfalse;
				}
			}
		}
	}
}

static void ST_TrackEnemy(const gentity_t* self, vec3_t enemy_pos)
{
	//clear timers
	TIMER_Set(self, "attackDelay", Q_irand(1000, 2000));
	TIMER_Set(self, "stick", Q_irand(500, 1500));
	TIMER_Set(self, "stand", -1);
	TIMER_Set(self, "scoutTime", TIMER_Get(self, "stick") - level.time + Q_irand(5000, 10000));
	//leave my combat point
	NPC_FreeCombatPoint(self->NPC->combatPoint, qfalse);
	//go after his last seen pos
	NPC_SetMoveGoal(self, enemy_pos, 100.0f, qfalse, -1, NULL);
	if (Q_irand(0, 3) == 0)
	{
		NPCS.NPCInfo->aiFlags |= NPCAI_STOP_AT_LOS;
	}
}

static int ST_ApproachEnemy(const gentity_t* self)
{
	TIMER_Set(self, "attackDelay", Q_irand(250, 500));
	TIMER_Set(self, "stick", Q_irand(1000, 2000));
	TIMER_Set(self, "stand", -1);
	TIMER_Set(self, "scoutTime", TIMER_Get(self, "stick") - level.time + Q_irand(5000, 10000));
	//leave my combat point
	NPC_FreeCombatPoint(self->NPC->combatPoint, qfalse);
	//return the relevant combat point flags
	return CP_CLEAR | CP_CLOSEST;
}

static void ST_HuntEnemy(const gentity_t* self)
{
	//TIMER_Set( NPC, "attackDelay", Q_irand( 250, 500 ) );//Disabled this for now, guys who couldn't hunt would never attack
	//TIMER_Set( NPC, "duck", -1 );
	TIMER_Set(self, "stick", Q_irand(250, 1000));
	TIMER_Set(self, "stand", -1);
	TIMER_Set(self, "scoutTime", TIMER_Get(self, "stick") - level.time + Q_irand(5000, 10000));
	//leave my combat point
	NPC_FreeCombatPoint(NPCS.NPCInfo->combatPoint, qfalse);
	//go directly after the enemy
	if (NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)
	{
		self->NPC->goalEntity = NPCS.NPC->enemy;
	}
}

static void ST_TransferTimers(const gentity_t* self, const gentity_t* other)
{
	TIMER_Set(other, "attackDelay", TIMER_Get(self, "attackDelay") - level.time);
	TIMER_Set(other, "duck", TIMER_Get(self, "duck") - level.time);
	TIMER_Set(other, "stick", TIMER_Get(self, "stick") - level.time);
	TIMER_Set(other, "scoutTime", TIMER_Get(self, "scoutTime") - level.time);
	TIMER_Set(other, "roamTime", TIMER_Get(self, "roamTime") - level.time);
	TIMER_Set(other, "stand", TIMER_Get(self, "stand") - level.time);
	TIMER_Set(self, "attackDelay", -1);
	TIMER_Set(self, "duck", -1);
	TIMER_Set(self, "stick", -1);
	TIMER_Set(self, "scoutTime", -1);
	TIMER_Set(self, "roamTime", -1);
	TIMER_Set(self, "stand", -1);
}

void ST_TransferMoveGoal(const gentity_t* self, const gentity_t* other)
{
	if (trap->ICARUS_TaskIDPending((sharedEntity_t*)self, TID_MOVE_NAV))
	{
		//can't transfer movegoal when a script we're running is waiting to complete
		return;
	}
	if (self->NPC->combatPoint != -1)
	{
		//I've got a combatPoint I'm going to, give it to him
		self->NPC->lastFailedCombatPoint = other->NPC->combatPoint = self->NPC->combatPoint;
		self->NPC->combatPoint = -1;
	}
	else
	{
		//I must be going for a goal, give that to him instead
		if (self->NPC->goalEntity == self->NPC->tempGoal)
		{
			NPC_SetMoveGoal(other, self->NPC->tempGoal->r.currentOrigin, self->NPC->goalRadius,
				self->NPC->tempGoal->flags & FL_NAVGOAL ? qtrue : qfalse, -1, NULL);
		}
		else
		{
			other->NPC->goalEntity = self->NPC->goalEntity;
		}
	}
	//give him my squadstate
	AI_GroupUpdateSquadstates(self->NPC->group, other, NPCS.NPCInfo->squadState);

	//give him my timers and clear mine
	ST_TransferTimers(self, other);

	//now make me stand around for a second or two at least
	AI_GroupUpdateSquadstates(self->NPC->group, self, SQUAD_STAND_AND_SHOOT);
	TIMER_Set(self, "stand", Q_irand(1000, 3000));
}

static int ST_GetCPFlags(void)
{
	int cpFlags = 0;
	if (NPCS.NPC && NPCS.NPCInfo->group)
	{
		if (NPCS.NPC == NPCS.NPCInfo->group->commander && NPCS.NPC->client->NPC_class == CLASS_IMPERIAL)
		{
			//imperials hang back and give orders
			if (NPCS.NPCInfo->group->numGroup > 1 && Q_irand(-3, NPCS.NPCInfo->group->numGroup) > 1)
			{
				//FIXME: make sure he;s giving orders with these lines
				if (Q_irand(0, 1))
				{
					ST_Speech(NPCS.NPC, SPEECH_CHASE, 0.5);
				}
				else
				{
					ST_Speech(NPCS.NPC, SPEECH_YELL, 0.5);
				}
			}
			cpFlags = CP_CLEAR | CP_COVER | CP_AVOID | CP_SAFE | CP_RETREAT;
		}
		else if (NPCS.NPCInfo->group->morale < 0)
		{
			//hide
			cpFlags = CP_COVER | CP_AVOID | CP_SAFE | CP_RETREAT;

			if (NPCS.NPC->client->NPC_class == CLASS_SABOTEUR && !Q_irand(0, 3))
			{
				Saboteur_Cloak(NPCS.NPC);
			}
		}
		else if (NPCS.NPCInfo->group->morale < NPCS.NPCInfo->group->numGroup)
		{
			//morale is low for our size
			const int moraleDrop = NPCS.NPCInfo->group->numGroup - NPCS.NPCInfo->group->morale;
			if (moraleDrop < -6)
			{
				//flee (no clear shot needed)
				cpFlags = CP_FLEE | CP_RETREAT | CP_COVER | CP_AVOID | CP_SAFE;
			}
			else if (moraleDrop < -3)
			{
				//retreat (no clear shot needed)
				cpFlags = CP_RETREAT | CP_COVER | CP_AVOID | CP_SAFE;
			}
			else if (moraleDrop < 0)
			{
				//cover (no clear shot needed)
				cpFlags = CP_COVER | CP_AVOID | CP_SAFE;
			}
		}
		else
		{
			const int moraleBoost = NPCS.NPCInfo->group->morale - NPCS.NPCInfo->group->numGroup;
			if (moraleBoost > 20)
			{
				//charge to any one and outflank (no cover needed)
				cpFlags = CP_CLEAR | CP_FLANK | CP_APPROACH_ENEMY;
			}
			else if (moraleBoost > 15)
			{
				//charge to closest one (no cover needed)
				cpFlags = CP_CLEAR | CP_CLOSEST | CP_APPROACH_ENEMY;

				if (NPCS.NPC->client->NPC_class == CLASS_SABOTEUR && !Q_irand(0, 3))
				{
					Saboteur_Decloak(NPCS.NPC, 2000);
				}
			}
			else if (moraleBoost > 10)
			{
				//charge closer (no cover needed)
				cpFlags = CP_CLEAR | CP_APPROACH_ENEMY;

				if (NPCS.NPC->client->NPC_class == CLASS_SABOTEUR && !Q_irand(0, 6))
				{
					Saboteur_Decloak(NPCS.NPC, 2000);
				}
			}
		}
	}
	if (!cpFlags)
	{
		//at some medium level of morale
		switch (Q_irand(0, 3))
		{
		case 0: //just take the nearest one
			cpFlags = CP_CLEAR | CP_COVER | CP_NEAREST;
			break;
		case 1: //take one closer to the enemy
			cpFlags = CP_CLEAR | CP_COVER | CP_APPROACH_ENEMY;
			break;
		case 2: //take the one closest to the enemy
			cpFlags = CP_CLEAR | CP_COVER | CP_CLOSEST | CP_APPROACH_ENEMY;
			break;
		case 3: //take the one on the other side of the enemy
			cpFlags = CP_CLEAR | CP_COVER | CP_FLANK | CP_APPROACH_ENEMY;
			break;
		default:;
		}
	}
	if (NPCS.NPC && NPCS.NPCInfo->scriptFlags & SCF_USE_CP_NEAREST)
	{
		cpFlags &= ~(CP_FLANK | CP_APPROACH_ENEMY | CP_CLOSEST);
		cpFlags |= CP_NEAREST;
	}
	return cpFlags;
}

/*
-------------------------
ST_Commander

  Make decisions about who should go where, etc.

FIXME: leader (group-decision-making) AI?
FIXME: need alternate routes!
FIXME: more group voice interaction
FIXME: work in pairs?

-------------------------
*/
extern qboolean AI_RefreshGroup(AIGroupInfo_t* group);

static void ST_Commander(void)
{
	int i, j;
	int cp, cpFlags_org, cpFlags;
	AIGroupInfo_t* group = NPCS.NPCInfo->group;
	gentity_t* member; //, *buddy;
	qboolean runner = qfalse;
	qboolean enemyLost = qfalse;
	qboolean enemyProtected = qfalse;
	qboolean scouting = qfalse;
	int squadState;
	int curMemberNum, lastMemberNum;
	float avoidDist;

	group->processed = qtrue;

	if (group->enemy == NULL || group->enemy->client == NULL)
	{
		//hmm, no enemy...?!
		return;
	}

	SaveNPCGlobals();

	if (group->lastSeenEnemyTime < level.time - 180000)
	{
		//dissolve the group
		ST_Speech(NPCS.NPC, SPEECH_LOST, 0.0f);
		group->enemy->waypoint = NAV_FindClosestWaypointForEnt(group->enemy, WAYPOINT_NONE);
		for (i = 0; i < group->numGroup; i++)
		{
			member = &g_entities[group->member[i].number];
			SetNPCGlobals(member);
			if (trap->ICARUS_TaskIDPending((sharedEntity_t*)NPCS.NPC, TID_MOVE_NAV))
			{
				//running somewhere that a script requires us to go, don't break from that
				continue;
			}
			if (!(NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES))
			{
				//not allowed to move on my own
				continue;
			}
			//Lost enemy for three minutes?  go into search mode?
			G_ClearEnemy(NPCS.NPC);
			NPCS.NPC->waypoint = NAV_FindClosestWaypointForEnt(NPCS.NPC, group->enemy->waypoint);
			if (NPCS.NPC->waypoint == WAYPOINT_NONE)
			{
				NPCS.NPCInfo->behaviorState = BS_DEFAULT; //BS_PATROL;
			}
			else if (group->enemy->waypoint == WAYPOINT_NONE || trap->Nav_GetPathCost(
				NPCS.NPC->waypoint, group->enemy->waypoint) >= Q3_INFINITE)
			{
				NPC_BSSearchStart(NPCS.NPC->waypoint, BS_SEARCH);
			}
			else
			{
				NPC_BSSearchStart(group->enemy->waypoint, BS_SEARCH);
			}
		}
		group->enemy = NULL;
		RestoreNPCGlobals();
		return;
	}

	//see if anyone is running
	if (group->numState[SQUAD_SCOUT] > 0 ||
		group->numState[SQUAD_TRANSITION] > 0 ||
		group->numState[SQUAD_RETREAT] > 0)
	{
		//someone is running
		runner = qtrue;
	}

	if (/*!runner &&*/ group->lastSeenEnemyTime > level.time - 32000 && group->lastSeenEnemyTime < level.time - 30000)
	{
		//no-one has seen the enemy for 30 seconds// and no-one is running after him
		if (group->commander && !Q_irand(0, 1))
		{
			ST_Speech(group->commander, SPEECH_ESCAPING, 0.0f);
		}
		else
		{
			ST_Speech(NPCS.NPC, SPEECH_ESCAPING, 0.0f);
		}
		//don't say this again
		NPCS.NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
	}

	if (group->lastSeenEnemyTime < level.time - 7000)
	{
		//no-one has seen the enemy for at least 10 seconds!  Should send a scout
		enemyLost = qtrue;
	}

	if (group->lastClearShotTime < level.time - 5000)
	{
		//no-one has had a clear shot for 5 seconds!
		enemyProtected = qtrue;
	}

	//Go through the list:

	//Everyone should try to get to a combat point if possible
	if (d_asynchronousGroupAI.integer)
	{
		//do one member a turn
		group->activeMemberNum++;
		if (group->activeMemberNum >= group->numGroup)
		{
			group->activeMemberNum = 0;
		}
		curMemberNum = group->activeMemberNum;
		lastMemberNum = curMemberNum + 1;
	}
	else
	{
		curMemberNum = 0;
		lastMemberNum = group->numGroup;
	}
	for (i = curMemberNum; i < lastMemberNum; i++)
	{
		//reset combat point flags
		cp = -1;
		cpFlags = 0;
		squadState = SQUAD_IDLE;
		avoidDist = 0;
		scouting = qfalse;

		//get the next guy
		member = &g_entities[group->member[i].number];
		if (!member->enemy)
		{
			//don't include guys that aren't angry
			continue;
		}
		SetNPCGlobals(member);

		if (!TIMER_Done(NPCS.NPC, "flee"))
		{
			//running away
			continue;
		}

		if (trap->ICARUS_TaskIDPending((sharedEntity_t*)NPCS.NPC, TID_MOVE_NAV))
		{
			//running somewhere that a script requires us to go
			continue;
		}

		if (NPCS.NPC->s.weapon == WP_NONE
			&& NPCS.NPCInfo->goalEntity
			&& NPCS.NPCInfo->goalEntity == NPCS.NPCInfo->tempGoal
			&& NPCS.NPCInfo->goalEntity->enemy
			&& NPCS.NPCInfo->goalEntity->enemy->s.eType == ET_ITEM)
		{
			//running to pick up a gun, don't do other logic
			continue;
		}

		//see if this member should start running (only if have no officer... FIXME: should always run from AEL_DANGER_GREAT?)
		if (!group->commander || group->commander->NPC && group->commander->NPC->rank < RANK_ENSIGN)
		{
			if (NPC_CheckForDanger(NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_DANGER)))
			{
				//going to run
				ST_Speech(NPCS.NPC, SPEECH_COVER, 0);
				continue;
			}
		}

		if (!(NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES))
		{
			//not allowed to do combat-movement
			continue;
		}

		if (NPCS.NPCInfo->squadState != SQUAD_RETREAT)
		{
			//not already retreating
			if (NPCS.NPC->client->ps.weapon == WP_NONE)
			{
				//weaponless, should be hiding
				if (NPCS.NPCInfo->goalEntity == NULL || NPCS.NPCInfo->goalEntity->enemy == NULL || NPCS.NPCInfo->
					goalEntity->enemy->s.eType != ET_ITEM)
				{
					//not running after a pickup
					if (TIMER_Done(NPCS.NPC, "hideTime") || DistanceSquared(
						group->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin) < 65536 && NPC_ClearLOS4(
							NPCS.NPC->enemy))
					{
						//done hiding or enemy near and can see us
						//er, start another flee I guess?
						NPC_StartFlee(NPCS.NPC->enemy, NPCS.NPC->enemy->r.currentOrigin, AEL_DANGER_GREAT, 5000, 10000);
					} //else, just hang here
				}
				continue;
			}
			if (TIMER_Done(NPCS.NPC, "roamTime") && TIMER_Done(NPCS.NPC, "hideTime") && NPCS.NPC->health > 10 && !trap->
				InPVS(group->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin))
			{
				//cant even see enemy
				//better go after him
				cpFlags |= CP_CLEAR | CP_COVER;
			}
			else if (NPCS.NPCInfo->localState == LSTATE_UNDERFIRE)
			{
				//we've been shot
				switch (group->enemy->client->ps.weapon)
				{
				case WP_SABER:
					if (DistanceSquared(group->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin) < 65536) //256 squared
					{
						cpFlags |= CP_AVOID_ENEMY | CP_COVER | CP_AVOID | CP_RETREAT;
						if (!group->commander || group->commander->NPC->rank < RANK_ENSIGN)
						{
							squadState = SQUAD_RETREAT;
						}
						avoidDist = 256;
					}
					break;
				default:
				case WP_BLASTER:
					cpFlags |= CP_COVER;
					break;
				}
				if (NPCS.NPC->health <= 10)
				{
					if (!group->commander || group->commander->NPC && group->commander->NPC->rank < RANK_ENSIGN)
					{
						cpFlags |= CP_FLEE | CP_AVOID | CP_RETREAT;
						squadState = SQUAD_RETREAT;
					}
				}
			}
			else
			{
				//not hit, see if there are other reasons we should run
				if (trap->InPVS(NPCS.NPC->r.currentOrigin, group->enemy->r.currentOrigin))
				{
					//in the same room as enemy
					if (NPCS.NPC->client->ps.weapon == WP_ROCKET_LAUNCHER &&
						DistanceSquared(group->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin) <
						MIN_ROCKET_DIST_SQUARED &&
						NPCS.NPCInfo->squadState != SQUAD_TRANSITION)
					{
						//too close for me to fire my weapon and I'm not already on the move
						cpFlags |= CP_AVOID_ENEMY | CP_CLEAR | CP_AVOID;
						avoidDist = 256;
					}
					else
					{
						switch (group->enemy->client->ps.weapon)
						{
						case WP_SABER:
							//if ( group->enemy->client->ps.SaberLength() > 0 )
							if (!group->enemy->client->ps.saberHolstered)
							{
								if (DistanceSquared(group->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin) < 65536)
								{
									if (TIMER_Done(NPCS.NPC, "hideTime"))
									{
										if (NPCS.NPCInfo->squadState != SQUAD_TRANSITION)
										{
											//not already moving: FIXME: we need to see if where we're going is good now?
											cpFlags |= CP_AVOID_ENEMY | CP_CLEAR | CP_AVOID;
											avoidDist = 256;
										}
									}
								}
							}
						default:
							break;
						}
					}
				}
			}
		}

		if (!cpFlags)
		{
			//okay, we have no new enemy-driven reason to run... let's use tactics now
			if (runner && NPCS.NPCInfo->combatPoint != -1)
			{
				//someone is running and we have a combat point already
				if (NPCS.NPCInfo->squadState != SQUAD_SCOUT &&
					NPCS.NPCInfo->squadState != SQUAD_TRANSITION &&
					NPCS.NPCInfo->squadState != SQUAD_RETREAT)
				{
					//it's not us
					if (TIMER_Done(NPCS.NPC, "verifyCP") && DistanceSquared(
						NPCS.NPC->r.currentOrigin, level.combatPoints[NPCS.NPCInfo->combatPoint].origin) > 64 * 64)
					{
						//1 - 3 seconds have passed since you chose a CP, see if you're there since, for some reason, you've stopped running...
						//uh, WTF, we're not on our combat point?
						//er, try again, I guess?
						cp = NPCS.NPCInfo->combatPoint;
						cpFlags |= ST_GetCPFlags();
					}
					else
					{
						//cover them
						//stop ducking
						TIMER_Set(NPCS.NPC, "duck", -1);
						//start shooting
						TIMER_Set(NPCS.NPC, "attackDelay", -1);
						//AI should take care of the rest - fire at enemy
					}
				}
				else
				{
					//we're running
					//see if we're blocked
					if (NPCS.NPCInfo->aiFlags & NPCAI_BLOCKED)
					{
						//dammit, something is in our way
						//see if it's one of ours
						for (j = 0; j < group->numGroup; j++)
						{
							if (group->member[j].number == NPCS.NPCInfo->blockingEntNum)
							{
								//we're being blocked by one of our own, pass our goal onto them and I'll stand still
								ST_TransferMoveGoal(NPCS.NPC, &g_entities[group->member[j].number]);
								break;
							}
						}
					}
					//we don't need to do anything else
					continue;
				}
			}
			else
			{
				//okay no-one is running, use some tactics
				if (NPCS.NPCInfo->combatPoint != -1)
				{
					//we have a combat point we're supposed to be running to
					if (NPCS.NPCInfo->squadState != SQUAD_SCOUT &&
						NPCS.NPCInfo->squadState != SQUAD_TRANSITION &&
						NPCS.NPCInfo->squadState != SQUAD_RETREAT)
					{
						//but we're not running
						if (TIMER_Done(NPCS.NPC, "verifyCP"))
						{
							//1 - 3 seconds have passed since you chose a CP, see if you're there since, for some reason, you've stopped running...
							if (DistanceSquared(NPCS.NPC->r.currentOrigin,
								level.combatPoints[NPCS.NPCInfo->combatPoint].origin) > 64 * 64)
							{
								//uh, WTF, we're not on our combat point?
								//er, try again, I guess?
								cp = NPCS.NPCInfo->combatPoint;
								cpFlags |= ST_GetCPFlags();
							}
						}
					}
				}
				if (enemyLost)
				{
					//if no-one has seen the enemy for a while, send a scout
					//ask where he went
					if (group->numState[SQUAD_SCOUT] <= 0)
					{
						//	scouting = qtrue;
						NPC_ST_StoreMovementSpeech(SPEECH_CHASE, 0.0f);
					}
					//Since no-one else has done this, I should be the closest one, so go after him...
					ST_TrackEnemy(NPCS.NPC, group->enemyLastSeenPos);
					//set me into scout mode
					AI_GroupUpdateSquadstates(group, NPCS.NPC, SQUAD_SCOUT);
					//we're not using a cp, so we need to set runner to true right here
					runner = qtrue;
				}
				else if (enemyProtected)
				{
					//if no-one has a clear shot at the enemy, someone should go after him
					//FIXME: if I'm in an area where no safe combat points have a clear shot at me, they don't come after me... they should anyway, though after some extra hesitation.
					//ALSO: seem to give up when behind an area portal?
					//since no-one else here has done this, I should be the closest one
					if (TIMER_Done(NPCS.NPC, "roamTime") && !Q_irand(0, group->numGroup))
					{
						//only do this if we're ready to move again and we feel like it
						cpFlags |= ST_ApproachEnemy(NPCS.NPC);
						//set me into scout mode
						AI_GroupUpdateSquadstates(group, NPCS.NPC, SQUAD_SCOUT);
					}
				}
				else
				{
					//group can see and has been shooting at the enemy
					//see if we should do something fancy?

					{
						//we're ready to move
						if (NPCS.NPCInfo->combatPoint == -1)
						{
							//we're not on a combat point
							if (1) //!Q_irand( 0, 2 ) )
							{
								//we should go for a combat point
								cpFlags |= ST_GetCPFlags();
							}
						}
						else if (TIMER_Done(NPCS.NPC, "roamTime"))
						{
							//we are already on a combat point
							if (i == 0)
							{
								//we're the closest
								if (group->morale - group->numGroup > 0 && !Q_irand(0, 4))
								{
									//try to outflank him
									cpFlags |= CP_CLEAR | CP_COVER | CP_FLANK | CP_APPROACH_ENEMY;
								}
								else if (group->morale - group->numGroup < 0)
								{
									//better move!
									cpFlags |= ST_GetCPFlags();
								}
								else
								{
									//If we're point, then get down
									TIMER_Set(NPCS.NPC, "roamTime", Q_irand(2000, 5000));
									TIMER_Set(NPCS.NPC, "stick", Q_irand(2000, 5000));
									//FIXME: what if we can't shoot from a ducked pos?
									TIMER_Set(NPCS.NPC, "duck", Q_irand(3000, 4000));
									AI_GroupUpdateSquadstates(group, NPCS.NPC, SQUAD_POINT);
								}
							}
							else if (i == group->numGroup - 1)
							{
								//farthest from the enemy
								if (group->morale - group->numGroup < 0)
								{
									//low morale, just hang here
									TIMER_Set(NPCS.NPC, "roamTime", Q_irand(2000, 5000));
									TIMER_Set(NPCS.NPC, "stick", Q_irand(2000, 5000));
								}
								else if (group->morale - group->numGroup > 0)
								{
									//try to move in on the enemy
									cpFlags |= ST_ApproachEnemy(NPCS.NPC);
									//set me into scout mode
									AI_GroupUpdateSquadstates(group, NPCS.NPC, SQUAD_SCOUT);
								}
								else
								{
									//use normal decision making process
									cpFlags |= ST_GetCPFlags();
								}
							}
							else
							{
								//someone in-between
								if (group->morale - group->numGroup < 0 || !Q_irand(0, 4))
								{
									//do something
									cpFlags |= ST_GetCPFlags();
								}
								else
								{
									TIMER_Set(NPCS.NPC, "stick", Q_irand(2000, 4000));
									TIMER_Set(NPCS.NPC, "roamTime", Q_irand(2000, 4000));
								}
							}
						}
					}
					if (!cpFlags)
					{
						//still not moving
						//see if we should do other fun stuff
						if (NPCS.NPC->attackDebounceTime < level.time - 2000)
						{
							//we, personally, haven't shot for 2 seconds
							//maybe yell at the enemy?
							ST_Speech(NPCS.NPC, SPEECH_LOOK, 0.9f);
						}
						//toy with ducking
						if (TIMER_Done(NPCS.NPC, "duck"))
						{
							//not ducking
							if (TIMER_Done(NPCS.NPC, "stand"))
							{
								//don't have to keep standing
								if (NPCS.NPCInfo->combatPoint == -1 || level.combatPoints[NPCS.NPCInfo->combatPoint].
									flags & CPF_DUCK)
								{
									//okay to duck here
									if (!Q_irand(0, 3))
									{
										TIMER_Set(NPCS.NPC, "duck", Q_irand(1000, 3000));
									}
								}
							}
						}
						//FIXME: what about CPF_LEAN?
					}
				}
			}
		}

		if (enemyLost)
		{
			//can nav to it
			ST_TrackEnemy(NPCS.NPC, NPCS.NPC->enemy->r.currentOrigin);
		}

		if (!NPCS.NPC->enemy)
		{
			continue;
		}

		// Check To See We Have A Clear Shot To The Enemy Every Couple Seconds
		//---------------------------------------------------------------------
		if (TIMER_Done(NPCS.NPC, "checkGrenadeTooCloseDebouncer"))
		{
			int i1, e;
			vec3_t mins;
			vec3_t maxs;
			int num_listed_entities;
			qboolean fled = qfalse;
			gentity_t* ent;
			int entity_list[MAX_GENTITIES];

			TIMER_Set(NPCS.NPC, "checkGrenadeTooCloseDebouncer", Q_irand(300, 600));

			for (i1 = 0; i1 < 3; i1++)
			{
				mins[i1] = NPCS.NPC->r.currentOrigin[i1] - 200;
				maxs[i1] = NPCS.NPC->r.currentOrigin[i1] + 200;
			}

			num_listed_entities = trap->EntitiesInBox(mins, maxs, entity_list, MAX_GENTITIES);

			for (e = 0; e < num_listed_entities; e++)
			{
				ent = &g_entities[entity_list[e]];

				if (ent == NPCS.NPC)
					continue;
				if (ent->parent == NPCS.NPC)
					continue;
				if (!ent->inuse)
					continue;
				if (ent->s.eType == ET_MISSILE)
				{
					if (ent->s.weapon == WP_THERMAL)
					{
						//a thermal
						//RAFIXME - impliment has_bounced flag?
						if (!ent->parent || !OnSameTeam(ent->parent, NPCS.NPC))
						{
							//bounced and an enemy thermal
							ST_Speech(NPCS.NPC, SPEECH_COVER, 0); //FIXME: flee sound?
							NPC_StartFlee(NPCS.NPC->enemy, ent->r.currentOrigin, AEL_DANGER_GREAT, 1000, 2000);
							fled = qtrue;
							TIMER_Set(NPCS.NPC, "checkGrenadeTooCloseDebouncer", Q_irand(2000, 4000));
							break;
						}
					}
				}
			}
			if (fled)
			{
				continue;
			}
		}

		// Check To See We Have A Clear Shot To The Enemy Every Couple Seconds
		//---------------------------------------------------------------------
		if (TIMER_Done(NPCS.NPC, "checkEnemyVisDebouncer"))
		{
			TIMER_Set(NPCS.NPC, "checkEnemyVisDebouncer", Q_irand(3000, 7000));
			if (!NPC_ClearLOS4(NPCS.NPC->enemy))
			{
				cpFlags |= CP_CLEAR | CP_COVER; // NOPE, Can't See The Enemy, So Find A New Combat Point
			}
		}

		// Check To See If The Enemy Is Too Close For Comfort
		//----------------------------------------------------
		if (NPCS.NPC->client->NPC_class != CLASS_ASSASSIN_DROID)
		{
			if (TIMER_Done(NPCS.NPC, "checkEnemyTooCloseDebouncer"))
			{
				float distThreshold;
				TIMER_Set(NPCS.NPC, "checkEnemyTooCloseDebouncer", Q_irand(1000, 6000));

				distThreshold = 16384/*128*128*/; //default
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
				case WP_CONCUSSION:
					if (!(NPCS.NPCInfo->scriptFlags & SCF_altFire))
					{
						distThreshold = 65536/*256*256*/;
					}
					break;
				default:
					break;
				}

				if (DistanceSquared(group->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin) < distThreshold)
				{
					cpFlags |= CP_CLEAR | CP_COVER;
				}
			}
		}

		//clear the local state
		NPCS.NPCInfo->localState = LSTATE_NONE;

		if (NPCS.NPCInfo->scriptFlags & SCF_USE_CP_NEAREST)
		{
			cpFlags &= ~(CP_FLANK | CP_APPROACH_ENEMY | CP_CLOSEST);
			cpFlags |= CP_NEAREST;
		}
		//Assign combat points
		if (cpFlags)
		{
			//we want to run to a combat point
			if (group->enemy->client->ps.weapon == WP_SABER && !group->enemy->client->ps.saberHolstered)
			{
				//we obviously want to avoid the enemy if he has a saber
				cpFlags |= CP_AVOID_ENEMY;
				avoidDist = 256;
			}

			//remember what we *wanted* to do...
			cpFlags_org = cpFlags;

			//now get a combat point
			if (cp == -1)
			{
				//may have had sone set above
				cp = NPC_FindCombatPoint(NPCS.NPC->r.currentOrigin, NPCS.NPC->r.currentOrigin,
					group->enemy->r.currentOrigin, cpFlags | CP_HAS_ROUTE, avoidDist,
					NPCS.NPCInfo->lastFailedCombatPoint);
			}
			while (cp == -1 && cpFlags != CP_ANY)
			{
				//start "OR"ing out certain flags to see if we can find *any* point
				if (cpFlags & CP_INVESTIGATE)
				{
					//don't need to investigate
					cpFlags &= ~CP_INVESTIGATE;
				}
				else if (cpFlags & CP_SQUAD)
				{
					//don't need to stick to squads
					cpFlags &= ~CP_SQUAD;
				}
				else if (cpFlags & CP_DUCK)
				{
					//don't need to duck
					cpFlags &= ~CP_DUCK;
				}
				else if (cpFlags & CP_NEAREST)
				{
					//don't need closest one to me
					cpFlags &= ~CP_NEAREST;
				}
				else if (cpFlags & CP_FLANK)
				{
					//don't need to flank enemy
					cpFlags &= ~CP_FLANK;
				}
				else if (cpFlags & CP_SAFE)
				{
					//don't need one that hasn't been shot at recently
					cpFlags &= ~CP_SAFE;
				}
				else if (cpFlags & CP_CLOSEST)
				{
					//don't need to get closest to enemy
					cpFlags &= ~CP_CLOSEST;
					//but let's try to approach at least
					cpFlags |= CP_APPROACH_ENEMY;
				}
				else if (cpFlags & CP_APPROACH_ENEMY)
				{
					//don't need to approach enemy
					cpFlags &= ~CP_APPROACH_ENEMY;
				}
				else if (cpFlags & CP_COVER)
				{
					//don't need cover
					cpFlags &= ~CP_COVER;
					//but let's pick one that makes us duck
					cpFlags |= CP_DUCK;
				}
				else if (cpFlags & CP_CLEAR)
				{
					//don't need a clear shot to enemy
					cpFlags &= ~CP_CLEAR;
				}
				else if (cpFlags & CP_AVOID_ENEMY)
				{
					//don't need to avoid enemy
					cpFlags &= ~CP_AVOID_ENEMY;
				}
				else if (cpFlags & CP_RETREAT)
				{
					//don't need to retreat
					cpFlags &= ~CP_RETREAT;
				}
				else if (cpFlags & CP_FLEE)
				{
					//don't need to flee
					cpFlags &= ~CP_FLEE;
					//but at least avoid enemy and pick one that gives cover
					cpFlags |= CP_COVER | CP_AVOID_ENEMY;
				}
				else if (cpFlags & CP_AVOID)
				{
					//okay, even pick one right by me
					cpFlags &= ~CP_AVOID;
				}
				else
				{
					cpFlags = CP_ANY;
				}
				//now try again
				cp = NPC_FindCombatPoint(NPCS.NPC->r.currentOrigin, NPCS.NPC->r.currentOrigin,
					group->enemy->r.currentOrigin, cpFlags | CP_HAS_ROUTE, avoidDist, -1);
			}
			//see if we got a valid one
			if (cp != -1)
			{
				//found a combat point
				//let others know that someone is now running
				runner = qtrue;
				//don't change course again until we get to where we're going
				TIMER_Set(NPCS.NPC, "roamTime", Q3_INFINITE);
				TIMER_Set(NPCS.NPC, "verifyCP", Q_irand(1000, 3000));
				//don't make sure you're in your CP for 1 - 3 seconds
				NPC_SetCombatPoint(cp);
				NPC_SetMoveGoal(NPCS.NPC, level.combatPoints[cp].origin, 8, qtrue, cp, NULL);
				//okay, try a move right now to see if we can even get there

				//if ( ST_Move() )
				{
					//we actually can get to it, so okay to say you're going there.
					//set us up so others know we're on the move
					if (squadState != SQUAD_IDLE)
					{
						AI_GroupUpdateSquadstates(group, NPCS.NPC, squadState);
					}
					else if (cpFlags & CP_FLEE)
					{
						//outright running for your life
						AI_GroupUpdateSquadstates(group, NPCS.NPC, SQUAD_RETREAT);
					}
					else
					{
						//any other kind of transition between combat points
						AI_GroupUpdateSquadstates(group, NPCS.NPC, SQUAD_TRANSITION);
					}

					//unless we're trying to flee, walk slowly
					if (!(cpFlags_org & CP_FLEE))
					{
						//ucmd.buttons |= BUTTON_CAREFUL;
					}

					if (cpFlags & CP_FLANK)
					{
						if (group->numGroup > 1)
						{
							NPC_ST_StoreMovementSpeech(SPEECH_OUTFLANK, -1);
						}
					}
					else
					{
						//okay, let's cheat
						if (group->numGroup > 1)
						{
							float dot = 1.0f;
							if (!Q_irand(0, 3))
							{
								//25% of the time, see if we're flanking the enemy
								vec3_t eDir2Me, eDir2CP;

								VectorSubtract(NPCS.NPC->r.currentOrigin, group->enemy->r.currentOrigin, eDir2Me);
								VectorNormalize(eDir2Me);

								VectorSubtract(level.combatPoints[NPCS.NPCInfo->combatPoint].origin,
									group->enemy->r.currentOrigin, eDir2CP);
								VectorNormalize(eDir2CP);

								dot = DotProduct(eDir2Me, eDir2CP);
							}

							if (dot < 0.4)
							{
								//flanking!
								NPC_ST_StoreMovementSpeech(SPEECH_OUTFLANK, -1);
							}
							else if (!Q_irand(0, 10))
							{
								//regular movement
								NPC_ST_StoreMovementSpeech(SPEECH_YELL, 0.2f); //was SPEECH_COVER
							}
						}
					}
				}
			}
			else if (NPCS.NPCInfo->squadState == SQUAD_SCOUT)
			{
				//we couldn't find a combatPoint by the player, so just go after him directly
				ST_HuntEnemy(NPCS.NPC);
				//set me into scout mode
				AI_GroupUpdateSquadstates(group, NPCS.NPC, SQUAD_SCOUT);
				//AI should take care of rest
			}
		}
	}

	RestoreNPCGlobals();
}

extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength,
	qboolean break_saber_lock);

static void Noghri_StickTrace(void)
{
	//code for noghi stick attacks

	if (!NPCS.NPC->ghoul2
		|| NPCS.NPC->client->weaponGhoul2[0])
	{
		//bad ghoul2 models
		return;
	}

	const int boltIndex = trap->G2API_AddBolt(NPCS.NPC->client->weaponGhoul2[0], 0, "*weapon");
	if (boltIndex != -1)
	{
		const int curTime = level.time;
		qboolean hit = qfalse;
		int lastHit = ENTITYNUM_NONE;
		for (int time = curTime - 25; time <= curTime + 25 && !hit; time += 25)
		{
			mdxaBone_t boltMatrix;
			vec3_t tip, dir, base, angles;
			vec3_t mins, maxs;
			trace_t trace;

			VectorSet(angles, 0, NPCS.NPC->r.currentAngles[YAW], 0);
			VectorSet(mins, -2, -2, -2);
			VectorSet(maxs, 2, 2, 2);

			trap->G2API_GetBoltMatrix(NPCS.NPC->ghoul2, 1,
				boltIndex,
				&boltMatrix, angles, NPCS.NPC->r.currentOrigin, time,
				NULL, NPCS.NPC->modelScale);
			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, base);
			BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Y, dir);
			VectorMA(base, 48, dir, tip);
			trap->Trace(&trace, base, mins, maxs, tip, NPCS.NPC->s.number, MASK_SHOT, qfalse, 0, 0);
			if (trace.fraction < 1.0f && trace.entityNum != lastHit)
			{
				//hit something
				gentity_t* traceEnt = &g_entities[trace.entityNum];
				if (traceEnt->takedamage
					&& (!traceEnt->client || traceEnt == NPCS.NPC->enemy || traceEnt->client->NPC_class != NPCS.NPC->
						client->NPC_class))
				{
					//smack
					const int dmg = Q_irand(12, 20); //FIXME: base on skill!
					G_Sound(traceEnt, CHAN_AUTO,
						G_SoundIndex(va("sound/weapons/tusken_staff/stickhit%d.wav", Q_irand(1, 4))));
					G_Damage(traceEnt, NPCS.NPC, NPCS.NPC, vec3_origin, trace.endpos, dmg, DAMAGE_NO_KNOCKBACK,
						MOD_MELEE);
					if (traceEnt->health > 0 && dmg > 17)
					{
						//do pain on enemy
						G_Knockdown(traceEnt, NPCS.NPC, dir, 300, qtrue);
					}
					lastHit = trace.entityNum;
					hit = qtrue;
				}
			}
		}
	}
}

/*
-------------------------
NPC_BSST_Attack
-------------------------
*/

extern qboolean PM_CrouchAnim(int anim);
#define MELEE_DIST_SQUARED 6400

void NPC_BSST_Attack(void)
{
	vec3_t enemyDir, shootDir;

	//Don't do anything if we're hurt
	if (NPCS.NPC->painDebounceTime > level.time)
	{
		NPC_UpdateAngles(qtrue, qtrue);

		if (NPCS.NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB)
		{
			//see if we grabbed enemy
			if (NPCS.NPC->client->ps.torsoTimer <= 200)
			{
				if (Melee_CanDoGrab()
					&& NPC_EnemyRangeFromBolt(0) <= 88.0f)
				{
					//grab him!
					Melee_GrabEnemy();
					return;
				}
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				NPCS.NPC->client->ps.weaponTime = NPCS.NPC->client->ps.torsoTimer;
				return;
			}
		}
		else
		{
			ST_Speech(NPCS.NPC, SPEECH_COVER, 0);

			NPC_CheckEvasion();
		}
		return;
	}
	if (!in_camera && (TIMER_Done(NPCS.NPC, "flee") && NPC_CheckForDanger(
		NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_DANGER))))
	{
		ST_Speech(NPCS.NPC, SPEECH_COVER, 0);

		NPC_CheckEvasion();
	}

	//If we don't have an enemy, just idle
	if (NPC_CheckEnemyExt(qfalse) == qfalse)
	{
		NPCS.NPC->enemy = NULL;
		if (NPCS.NPC->client->playerTeam == NPCTEAM_PLAYER)
		{
			NPC_BSPatrol();
		}
		else
		{
			NPC_BSST_Patrol(); //FIXME: or patrol?
		}
		return;
	}

	// now if this timer is done and the enemy is no longer in our line of sight, try to get a new enemy later
	if (TIMER_Done(NPCS.NPC, "sje_check_enemy"))
	{
		TIMER_Set(NPCS.NPC, "sje_check_enemy", Q_irand(5000, 10000));

		if (NPCS.NPC->enemy && !NPC_ClearLOS4(NPCS.NPC->enemy) && NPCS.NPC->health > 0)
		{
			// if enemy cant be seen, try getting one later
			if (NPCS.NPC->client)
			{
				NPCS.NPC->enemy = NULL;
				if (NPCS.NPC->client->playerTeam == NPCTEAM_PLAYER)
				{
					NPC_BSPatrol();
				}
				else
				{
					NPC_BSST_Patrol();
				}
				return;
			}
			if (NPCS.NPC->client)
			{
				// guardians have a different way to find enemies. He tries to find the quest player and his allies

				for (int sje_it = 0; sje_it < level.maxclients; sje_it++)
				{
					gentity_t* allied_player = &g_entities[sje_it];

					if (allied_player && allied_player->client &&
						NPC_ClearLOS4(allied_player))
					{
						// the quest player or one of his allies. If one of them is in line of sight, choose him as enemy
						NPCS.NPC->enemy = allied_player;
					}
				}
			}
		}
	}

	//Get our group info
	if (TIMER_Done(NPCS.NPC, "interrogating"))
	{
		AI_GetGroup(NPCS.NPC); //, 45, 512, NPC->enemy );
	}
	else
	{
		ST_Speech(NPCS.NPC, SPEECH_YELL, 0);
	}

	if (NPCS.NPCInfo->group)
	{
		//I belong to a squad of guys - we should *always* have a group
		if (!NPCS.NPCInfo->group->processed)
		{
			//I'm the first ent in my group, I'll make the command decisions
#if	AI_TIMERS
			int	startTime = GetTime(0);
#endif//	AI_TIMERS
			ST_Commander();
#if	AI_TIMERS
			int commTime = GetTime(startTime);
			if (commTime > 20)
			{
				trap->Printf(S_COLOR_RED"ERROR: Commander time: %d\n", commTime);
			}
			else if (commTime > 10)
			{
				trap->Printf(S_COLOR_YELLOW"WARNING: Commander time: %d\n", commTime);
			}
			else if (commTime > 2)
			{
				trap->Printf(S_COLOR_GREEN"Commander time: %d\n", commTime);
			}
#endif//	AI_TIMERS
		}
	}
	else if (enemyDist < 8192)
	{
		// Too close for comfort, start moving away from us
		TIMER_Set(NPCS.NPC, "duck", -1);
		NPC_StartFlee(NPCS.NPC->enemy, NPCS.NPC->enemy->r.currentOrigin, AEL_DANGER, 1000, 3000);
		ST_Speech(NPCS.NPC, SPEECH_OUTFLANK, 0.3f);
	}
	else if (enemyDist <= -16)
	{
		//we're too damn close!
		if (!Q_irand(0, 30)
			&& Melee_CanDoGrab())
		{
			Melee_TryGrab();
			return;
		}
		NPC_StartFlee(NPCS.NPC->enemy, NPCS.NPC->enemy->r.currentOrigin, AEL_DANGER, 1000, 3000);
	}
	else if (enemyDist < 32768)
	{
		//too close, so switch to primary fire
		if (NPCS.NPC->client->ps.weapon == WP_BLASTER)
		{
			if (NPCS.NPCInfo->scriptFlags & SCF_altFire)
			{
				//use primary fire
				trace_t trace;
				trap->Trace(&trace, NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.mins, NPCS.NPC->r.maxs,
					NPCS.NPC->r.currentOrigin, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0);
				if (!trace.allsolid && !trace.startsolid && (trace.fraction == 1.0 || trace.entityNum == NPCS.NPC->s.
					number))
				{
					//he can get right to me
					NPCS.NPCInfo->scriptFlags &= ~SCF_altFire;
					TIMER_Set(NPCS.NPC, "attackDelay", -1);
					// If we move into position right after he fires, he can sit there and charge his weapon and look dumb
					NPC_ChangeWeapon(WP_BLASTER);
					ST_Speech(NPCS.NPC, SPEECH_COVER, 0);
					NPC_UpdateAngles(qtrue, qtrue);
					return;
				}
			}
		}
	}
	else if (enemyDist > 65536 && TIMER_Done(NPCS.NPC, "flee"))
	{
		// do NOT switch into sniper mode if we are running...
		if (NPCS.NPC->client->ps.weapon == WP_BLASTER)
		{
			//sniping... should be assumed
			if (!(NPCS.NPCInfo->scriptFlags & SCF_altFire))
			{
				//use primary fire
				NPCS.NPCInfo->scriptFlags |= SCF_altFire;
				//reset fire-timing variables
				NPC_ChangeWeapon(WP_BLASTER);
				ST_Speech(NPCS.NPC, SPEECH_COVER, 0);
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}
	else if (enemyDist <= 0)
	{
		//we're within striking range
		//if we are attacking, see if we should stop
		if (NPCS.NPCInfo->stats.aggression < 4)
		{
			//back off and defend
			if (!Q_irand(0, 30)
				&& Melee_CanDoGrab())
			{
				Melee_TryGrab();
				return;
			}
			NPC_StartFlee(NPCS.NPC->enemy, NPCS.NPC->enemy->r.currentOrigin, AEL_DANGER, 1000, 3000);
		}
	}
	else if (TIMER_Done(NPCS.NPC, "flee") && NPC_CheckForDanger(
		NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_DANGER)))
	{
		//not already fleeing, and going to run
		ST_Speech(NPCS.NPC, SPEECH_COVER, 0);
		NPC_UpdateAngles(qtrue, qtrue);
		return;
	}

	if (!NPCS.NPC->enemy)
	{
		//WTF?  somehow we lost our enemy?
		NPC_BSST_Patrol(); //FIXME: or patrol?
		return;
	}

	if (NPCS.NPCInfo->goalEntity && NPCS.NPCInfo->goalEntity != NPCS.NPC->enemy)
	{
		NPCS.NPCInfo->goalEntity = UpdateGoal();
	}

	enemyLOS = enemyCS = enemyInFOV = qfalse;
	move = qtrue;
	faceEnemy = qfalse;
	shoot = qfalse;
	hitAlly = qfalse;
	VectorClear(impactPos);
	enemyDist = DistanceSquared(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin);

	VectorSubtract(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, enemyDir);
	VectorNormalize(enemyDir);
	AngleVectors(NPCS.NPC->client->ps.viewangles, shootDir, NULL, NULL);
	const float dot = DotProduct(enemyDir, shootDir);
	if (dot > 0.5f || enemyDist * (1.0f - dot) < 10000)
	{
		//enemy is in front of me or they're very close and not behind me
		enemyInFOV = qtrue;
	}

	if (enemyDist < MIN_ROCKET_DIST_SQUARED) //128
	{
		//enemy within 128
		if ((NPCS.NPC->client->ps.weapon == WP_FLECHETTE || NPCS.NPC->client->ps.weapon == WP_REPEATER) &&
			NPCS.NPCInfo->scriptFlags & SCF_altFire)
		{
			//shooting an explosive, but enemy too close, switch to primary fire
			NPCS.NPCInfo->scriptFlags &= ~SCF_altFire;
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
	if (NPC_ClearLOS4(NPCS.NPC->enemy))
	{
		AI_GroupUpdateEnemyLastSeen(NPCS.NPCInfo->group, NPCS.NPC->enemy->r.currentOrigin);
		NPCS.NPCInfo->enemyLastSeenTime = level.time;
		enemyLOS = qtrue;

		if (NPCS.NPC->client->ps.weapon == WP_NONE)
		{
			enemyCS = qfalse; //not true, but should stop us from firing
			NPC_AimAdjust(-1); //adjust aim worse longer we have no weapon
		}
		else
		{
			if (enemyDist < MIN_ROCKET_DIST_SQUARED &&
				(NPCS.NPC->client->ps.weapon == WP_ROCKET_LAUNCHER
					|| NPCS.NPC->client->ps.weapon == WP_CONCUSSION && !(NPCS.NPCInfo->scriptFlags & SCF_altFire)
					|| NPCS.NPC->client->ps.weapon == WP_FLECHETTE && NPCS.NPCInfo->scriptFlags & SCF_altFire))
			{
				enemyCS = qfalse; //not true, but should stop us from firing
				hitAlly = qtrue; //us!
			}
			else if (enemyInFOV)
			{
				//if enemy is FOV, go ahead and check for shooting
				const int hit = NPC_ShotEntity(NPCS.NPC->enemy, impactPos);
				const gentity_t* hit_ent = &g_entities[hit];

				if (hit == NPCS.NPC->enemy->s.number
					|| hit_ent && hit_ent->client && hit_ent->client->playerTeam == NPCS.NPC->client->enemyTeam
					|| hit_ent && hit_ent->takedamage && (hit_ent->r.svFlags & SVF_GLASS_BRUSH || hit_ent->health < 40 ||
						NPCS.NPC->s.weapon == WP_EMPLACED_GUN))
				{
					//can hit enemy or enemy ally or will hit glass or other minor breakable (or in emplaced gun), so shoot anyway
					AI_GroupUpdateClearShotTime(NPCS.NPCInfo->group);
					enemyCS = qtrue;
					NPC_AimAdjust(2); //adjust aim better longer we have clear shot at enemy
					VectorCopy(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPCInfo->enemyLastSeenLocation);
				}
				else
				{
					//Hmm, have to get around this bastard
					NPC_AimAdjust(1); //adjust aim better longer we can see enemy
					ST_ResolveBlockedShot(hit);
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
		faceEnemy = qtrue;
		NPC_AimAdjust(-1); //adjust aim worse longer we cannot see enemy
	}

	if (NPCS.NPC->client->ps.weapon == WP_NONE)
	{
		faceEnemy = qfalse;
		shoot = qfalse;
	}
	else if (NPCS.NPC->client->ps.weapon == WP_MELEE)
	{
		faceEnemy = qtrue;
		shoot = qtrue;
	}
	else
	{
		if (enemyLOS)
		{
			//FIXME: no need to face enemy if we're moving to some other goal and he's too far away to shoot?
			faceEnemy = qtrue;
		}
		if (enemyCS)
		{
			shoot = qtrue;
		}
	}

	//Check for movement to take care of
	ST_CheckMoveState();

	//See if we should override shooting decision with any special considerations
	ST_CheckFireState();

	if (faceEnemy)
	{
		//face the enemy
		NPC_FaceEnemy(qtrue);
	}

	if (!(NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES))
	{
		//not supposed to chase my enemies
		if (NPCS.NPCInfo->goalEntity == NPCS.NPC->enemy)
		{
			//goal is my entity, so don't move
			move = qfalse;
		}
	}
	else if (NPCS.NPC->NPC->scriptFlags & SCF_NO_GROUPS)
	{
		NPCS.NPCInfo->goalEntity = enemyLOS ? 0 : NPCS.NPC->enemy;
	}

	if (NPCS.NPC->client->ps.weaponTime > 0 && NPCS.NPC->s.weapon == WP_ROCKET_LAUNCHER)
	{
		move = qfalse;
	}

	if (!NPCS.ucmd.rightmove)
	{
		//only if not already strafing for some strange reason...?
		if (!TIMER_Done(NPCS.NPC, "strafeLeft"))
		{
			NPCS.ucmd.rightmove = -127;
			//re-check the duck as we might want to be rolling
			VectorClear(NPCS.NPC->client->ps.moveDir);
			move = qfalse;
		}
		else if (!TIMER_Done(NPCS.NPC, "strafeRight"))
		{
			NPCS.ucmd.rightmove = 127;
			VectorClear(NPCS.NPC->client->ps.moveDir);
			move = qfalse;
		}
	}

	if (NPCS.NPC->client->ps.legsAnim == BOTH_GUARD_LOOKAROUND1)
	{
		//don't move when doing silly look around thing
		move = qfalse;
	}
	if (move)
	{
		//move toward goal
		if (NPCS.NPCInfo->goalEntity)
		{
			move = ST_Move();

			if ((NPCS.NPC->client->NPC_class != CLASS_ROCKETTROOPER ||
				NPCS.NPC->s.weapon != WP_ROCKET_LAUNCHER || enemyDist < MIN_ROCKET_DIST_SQUARED)
				//rockettroopers who use rocket launchers turn around and run if you get too close (closer than 128)
				&& NPCS.ucmd.forwardmove <= -32)
			{
				//moving backwards at least 45 degrees
				if (NPCS.NPCInfo->goalEntity
					&& DistanceSquared(NPCS.NPCInfo->goalEntity->r.currentOrigin, NPCS.NPC->r.currentOrigin) >
					MIN_TURN_AROUND_DIST_SQ)
				{
					//don't stop running backwards if your goal is less than 100 away
					if (TIMER_Done(NPCS.NPC, "runBackwardsDebounce"))
					{
						//not already waiting for next run backwards
						if (!TIMER_Exists(NPCS.NPC, "runningBackwards"))
						{
							//start running backwards
							TIMER_Set(NPCS.NPC, "runningBackwards", Q_irand(500, 1000));
						}
						else if (TIMER_Done2(NPCS.NPC, "runningBackwards", qtrue))
						{
							//done running backwards
							TIMER_Set(NPCS.NPC, "runBackwardsDebounce", Q_irand(3000, 5000));
						}
					}
				}
			}
			else
			{
				//not running backwards
				//TIMER_Remove( NPC, "runningBackwards" );
			}
		}
		else
		{
			move = qfalse;
		}
	}

	if (!move)
	{
		if (NPCS.NPC->client->NPC_class != CLASS_ASSASSIN_DROID)
		{
			//droids don't duck.
			if (!TIMER_Done(NPCS.NPC, "duck"))
			{
				NPCS.ucmd.upmove = -127;
			}
		}
	}
	else
	{
		//stop ducking!
		TIMER_Set(NPCS.NPC, "duck", -1);
	}
	if (NPCS.NPC->client->NPC_class == CLASS_NOGHRI
		&& NPCS.NPC->client->playerTeam == NPCTEAM_ENEMY
		&& !PM_InKnockDown(&NPCS.NPC->client->ps)) //not knocked down ) )
	{
		if (NPCS.NPC->client->ps.torsoAnim == BOTH_MELEE_L
			|| NPCS.NPC->client->ps.torsoAnim == BOTH_MELEEUP
			|| NPCS.NPC->client->ps.torsoAnim == BOTH_MELEE_R)
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
					smack_dir[2] += 30;
					VectorNormalize(smack_dir);
					//hurt them
					G_Sound(NPCS.NPC->enemy, CHAN_AUTO, G_SoundIndex("sound/chars/noghri/misc/taunt.mp3"));
					G_Damage(NPCS.NPC->enemy, NPCS.NPC, NPCS.NPC, smack_dir, NPCS.NPC->r.currentOrigin,
						(g_npcspskill.integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);
					if (NPCS.NPC->client->ps.torsoAnim == BOTH_MELEE_L)
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
						g_throw(NPCS.NPC->enemy, smack_dir, 50);
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, knockAnim,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (NPCS.NPC->client->ps.torsoAnim == BOTH_MELEE_R)
					{
						g_throw(NPCS.NPC->enemy, smack_dir, 65);
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, BOTH_KNOCKDOWN5,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						//uppercut
						//throw them
						g_throw(NPCS.NPC->enemy, smack_dir, 80);
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

	//sbd slap
	if (NPCS.NPC->client->NPC_class == CLASS_NOGHRI
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
				//animate me
				int swing_anim;
				if (NPCS.NPC->health > 70)
				{
					swing_anim = Q_irand(BOTH_MELEE_L, BOTH_MELEE_R); //smackdown or uppercut
				}
				else
				{
					swing_anim = BOTH_MELEEUP; //smackdown or uppercut
				}
				G_Sound(NPCS.NPC->enemy, CHAN_AUTO, G_SoundIndex("sound/weapons/melee/punch2.mp3"));
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.torsoTimer + Q_irand(3500, 6500));
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPCS.NPC, "smackTime", 300);
				NPCS.NPCInfo->blockedDebounceTime = 0;
			}
		}
	}
	//==
	if (NPCS.NPC->client->NPC_class == CLASS_IMPERIAL ||
		NPCS.NPC->client->NPC_class == CLASS_REELO ||
		NPCS.NPC->client->NPC_class == CLASS_SABOTEUR
		&& NPCS.NPC->client->playerTeam == NPCTEAM_ENEMY
		&& !NPCS.NPC->client->ps.weaponTime
		&& !PM_InKnockDown(&NPCS.NPC->client->ps))
	{
		if (NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_F
			|| NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_F2
			|| NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_B2
			|| NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_B)
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
					smack_dir[2] += 30;
					VectorNormalize(smack_dir);
					//hurt them
					G_Sound(NPCS.NPC->enemy, CHAN_AUTO, G_SoundIndex("sound/chars/%s/misc/taunt0%d.mp3"));
					G_Damage(NPCS.NPC->enemy, NPCS.NPC, NPCS.NPC, smack_dir, NPCS.NPC->r.currentOrigin,
						(g_npcspskill.integer + 1) * Q_irand(1, 3), DAMAGE_NO_KNOCKBACK, MOD_MELEE);
					if (NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_F)
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
						g_throw(NPCS.NPC->enemy, smack_dir, 25);
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, knockAnim,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_F2)
					{
						g_throw(NPCS.NPC->enemy, smack_dir, 30);
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
	if (NPCS.NPC->client->NPC_class == CLASS_IMPERIAL ||
		NPCS.NPC->client->NPC_class == CLASS_REELO ||
		NPCS.NPC->client->NPC_class == CLASS_SABOTEUR
		&& NPCS.NPC->client->playerTeam == NPCTEAM_ENEMY
		&& !NPCS.NPC->client->ps.weaponTime
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
				//animate me
				const int swing_anim = Q_irand(BOTH_A7_KICK_F, BOTH_A7_KICK_F2);
				G_Sound(NPCS.NPC->enemy, CHAN_AUTO, G_SoundIndex("sound/weapons/melee/kick2.mp3"));
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.torsoTimer + Q_irand(2500, 4500));
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
				//animate me
				const int swing_anim = Q_irand(BOTH_A7_KICK_B2, BOTH_A7_KICK_B);
				G_Sound(NPCS.NPC->enemy, CHAN_AUTO, G_SoundIndex("sound/weapons/melee/kick1.mp3"));
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.torsoTimer + Q_irand(2500, 4500));
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPCS.NPC, "smackTime", 300);
				NPCS.NPCInfo->blockedDebounceTime = 0;
			}
		}
	}
	//===================================================
	if (NPCS.NPC->client->NPC_class == CLASS_TRANDOSHAN ||
		NPCS.NPC->client->NPC_class == CLASS_WEEQUAY ||
		NPCS.NPC->client->NPC_class == CLASS_RODIAN
		&& NPCS.NPC->client->playerTeam == NPCTEAM_ENEMY
		&& !NPCS.NPC->client->ps.weaponTime
		&& !PM_InKnockDown(&NPCS.NPC->client->ps))
	{
		if (NPCS.NPC->client->ps.torsoAnim == BOTH_MELEE_L
			|| NPCS.NPC->client->ps.torsoAnim == BOTH_MELEE_R
			|| NPCS.NPC->client->ps.torsoAnim == BOTH_MELEEUP
			|| NPCS.NPC->client->ps.torsoAnim == BOTH_MELEE3)
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
					smack_dir[2] += 10;
					VectorNormalize(smack_dir);
					//hurt them
					G_Damage(NPCS.NPC->enemy, NPCS.NPC, NPCS.NPC, smack_dir, NPCS.NPC->r.currentOrigin,
						(g_npcspskill.integer + 1) * 1, DAMAGE_NO_KNOCKBACK, MOD_MELEE);

					if (NPCS.NPC->client->ps.torsoAnim == BOTH_MELEE_R)
					{
						//smackdown
						int knockAnim = Q_irand(BOTH_PAIN1, BOTH_PAIN2);
						if (PM_CrouchAnim(NPCS.NPC->enemy->client->ps.legsAnim))
						{
							//knockdown from crouch
							knockAnim = BOTH_PAIN2;
						}
						//throw them
						smack_dir[2] = 1;
						VectorNormalize(smack_dir);
						g_throw(NPCS.NPC->enemy, smack_dir, 10);
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, knockAnim,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (NPCS.NPC->client->ps.torsoAnim == BOTH_MELEE_L)
					{
						g_throw(NPCS.NPC->enemy, smack_dir, 10);
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, BOTH_PAIN3,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (NPCS.NPC->client->ps.torsoAnim == BOTH_MELEEUP)
					{
						g_throw(NPCS.NPC->enemy, smack_dir, 10);
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, BOTH_PAIN4,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						//uppercut
						//throw them
						g_throw(NPCS.NPC->enemy, smack_dir, 10);
						//make them backflip
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, BOTH_PAIN1,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					//done with the damage
					NPCS.NPCInfo->blockedDebounceTime = 1;
				}
			}
		}
	}

	//slap
	if (NPCS.NPC->client->NPC_class == CLASS_TRANDOSHAN ||
		NPCS.NPC->client->NPC_class == CLASS_WEEQUAY ||
		NPCS.NPC->client->NPC_class == CLASS_RODIAN
		&& NPCS.NPC->client->playerTeam == NPCTEAM_ENEMY
		&& !NPCS.NPC->client->ps.weaponTime
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
				//animate me
				int swing_anim;
				if (NPCS.NPC->health > 50)
				{
					swing_anim = Q_irand(BOTH_MELEEUP, BOTH_MELEE3); //kick
				}
				else
				{
					swing_anim = Q_irand(BOTH_MELEE_R, BOTH_MELEE_L); //kick
				}
				G_Sound(NPCS.NPC->enemy, CHAN_AUTO, G_SoundIndex("sound/weapons/melee/kick2.mp3"));
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.torsoTimer + Q_irand(2500, 4500));
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPCS.NPC, "smackTime", 300);
				NPCS.NPCInfo->blockedDebounceTime = 0;
			}
		}
	}

	if (NPCS.NPC->client->NPC_class == CLASS_BESPIN_COP ||
		NPCS.NPC->client->NPC_class == CLASS_JAN ||
		NPCS.NPC->client->NPC_class == CLASS_PRISONER ||
		NPCS.NPC->client->NPC_class == CLASS_REBEL ||
		NPCS.NPC->client->NPC_class == CLASS_WOOKIE
		&& NPCS.NPC->client->playerTeam == NPCTEAM_PLAYER
		&& !PM_InKnockDown(&NPCS.NPC->client->ps)) //not knocked down ) )
	{
		if (NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_F
			|| NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_F2
			|| NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_B2
			|| NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_B)
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
					smack_dir[2] += 30;
					VectorNormalize(smack_dir);
					//hurt them
					G_Sound(NPCS.NPC->enemy, CHAN_AUTO, G_SoundIndex("sound/weapons/melee/kick2.mp3"));
					G_Damage(NPCS.NPC->enemy, NPCS.NPC, NPCS.NPC, smack_dir, NPCS.NPC->r.currentOrigin,
						(g_npcspskill.integer + 1) * Q_irand(5, 10), DAMAGE_NO_KNOCKBACK, MOD_MELEE);
					if (NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_F)
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
						g_throw(NPCS.NPC->enemy, smack_dir, 50);
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, knockAnim,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (NPCS.NPC->client->ps.torsoAnim == BOTH_A7_KICK_F2)
					{
						g_throw(NPCS.NPC->enemy, smack_dir, 65);
						NPC_SetAnim(NPCS.NPC->enemy, SETANIM_BOTH, BOTH_KNOCKDOWN5,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						//uppercut
						//throw them
						g_throw(NPCS.NPC->enemy, smack_dir, 80);
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
	if (NPCS.NPC->client->NPC_class == CLASS_BESPIN_COP ||
		NPCS.NPC->client->NPC_class == CLASS_JAN ||
		NPCS.NPC->client->NPC_class == CLASS_PRISONER ||
		NPCS.NPC->client->NPC_class == CLASS_REBEL ||
		NPCS.NPC->client->NPC_class == CLASS_WOOKIE
		&& NPCS.NPC->client->playerTeam == NPCTEAM_PLAYER
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
				//animate me
				const int swing_anim = Q_irand(BOTH_A7_KICK_F, BOTH_A7_KICK_F2);
				G_Sound(NPCS.NPC->enemy, CHAN_AUTO, G_SoundIndex("sound/weapons/melee/kick3.mp3"));
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.torsoTimer + Q_irand(1000, 3000));
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
				//animate me
				const int swing_anim = Q_irand(BOTH_A7_KICK_B2, BOTH_A7_KICK_B);
				G_Sound(NPCS.NPC->enemy, CHAN_AUTO, G_SoundIndex("sound/weapons/melee/kick1.mp3"));
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.torsoTimer + Q_irand(1000, 3000));
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPCS.NPC, "smackTime", 300);
				NPCS.NPCInfo->blockedDebounceTime = 0;
			}
		}
	}

	if (NPCS.NPC->client->NPC_class == CLASS_REBORN && NPCS.NPC->s.weapon != WP_SABER || //using a gun
		NPCS.NPC->client->NPC_class == CLASS_IMPERIAL ||
		NPCS.NPC->client->NPC_class == CLASS_REELO ||
		NPCS.NPC->client->NPC_class == CLASS_SABOTEUR ||
		NPCS.NPC->client->NPC_class == CLASS_TRANDOSHAN ||
		NPCS.NPC->client->NPC_class == CLASS_WEEQUAY ||
		NPCS.NPC->client->NPC_class == CLASS_GRAN ||
		NPCS.NPC->client->NPC_class == CLASS_RODIAN ||
		NPCS.NPC->client->NPC_class == CLASS_PRISONER
		&& NPCS.NPC->enemy->s.weapon == WP_SABER) //fighting a saber-user
	{
		//see if we need to avoid their saber
		NPC_EvasionSaber();
	}

	if (!TIMER_Done(NPCS.NPC, "runBackwardsDebounce"))
	{
		//running away
		faceEnemy = qfalse;
	}

	//FIXME: check scf_face_move_dir here?

	if (!faceEnemy)
	{
		//we want to face in the dir we're running
		if (!move)
		{
			//if we haven't moved, we should look in the direction we last looked?
			VectorCopy(NPCS.NPC->client->ps.viewangles, NPCS.NPCInfo->lastPathAngles);
		}
		NPCS.NPCInfo->desiredYaw = NPCS.NPCInfo->lastPathAngles[YAW];
		NPCS.NPCInfo->desiredPitch = 0;
		NPC_UpdateAngles(qtrue, qtrue);
		if (move)
		{
			//don't run away and shoot
			shoot = qfalse;
		}
	}

	if (NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE)
	{
		shoot = qfalse;
	}

	if (NPCS.NPC->enemy && NPCS.NPC->enemy->enemy && NPCS.NPC->enemy->enemy->client)
	{
		if (NPCS.NPC->enemy->s.weapon == WP_SABER && NPCS.NPC->enemy->enemy->s.weapon == WP_SABER &&
			NPCS.NPC->enemy->enemy->client->playerTeam == NPCS.NPC->client->playerTeam)
		{
			//don't shoot at an enemy jedi who is fighting another jedi, for fear of injuring one or causing rogue blaster deflections (a la Obi Wan/Vader duel at end of ANH)
			shoot = qfalse;
		}
	}

	if (NPCS.NPC->client->ps.weaponTime > 0)
	{
		if (NPCS.NPC->client->NPC_class == CLASS_SABOTEUR)
		{
			//decloak if we're firing.
			Saboteur_Decloak(NPCS.NPC, 2000);
		}
		if (NPCS.NPC->s.weapon == WP_ROCKET_LAUNCHER
			|| NPCS.NPC->s.weapon == WP_CONCUSSION && !(NPCS.NPCInfo->scriptFlags & SCF_altFire))
		{
			if (!enemyLOS || !enemyCS)
			{
				//cancel it
				NPCS.NPC->client->ps.weaponTime = 0;
			}
			else
			{
				//delay our next attempt
				TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(3000, 5000));
			}
		}
	}
	else if (shoot)
	{
		//try to shoot if it's time
		if (NPCS.NPC->client->NPC_class == CLASS_SABOTEUR)
		{
			//decloak if firing.
			Saboteur_Decloak(NPCS.NPC, 2000);
		}
		if (TIMER_Done(NPCS.NPC, "attackDelay"))
		{
			if (!(NPCS.NPCInfo->scriptFlags & SCF_FIRE_WEAPON)) // we've already fired, no need to do it again here
			{
				WeaponThink();
			}
			if (NPCS.NPC->s.weapon == WP_ROCKET_LAUNCHER)
			{
				if (NPCS.ucmd.buttons & BUTTON_ATTACK
					&& !move
					&& g_npcspskill.integer > 1
					&& !Q_irand(0, 3))
				{
					//every now and then, shoot a homing rocket
					NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
					NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
					NPCS.NPC->client->ps.weaponTime = Q_irand(1000, 2500);
				}
			}
		}
	}
	else if (NPCS.NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB)
	{
		//see if we grabbed enemy
		if (NPCS.NPC->client->ps.torsoTimer <= 200)
		{
			if (Melee_CanDoGrab()
				&& NPC_EnemyRangeFromBolt(0) <= 72.0f)
			{
				//grab him!
				Melee_GrabEnemy();
				return;
			}
			NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			NPCS.NPC->client->ps.weaponTime = NPCS.NPC->client->ps.torsoTimer;
		}
	}
	else
	{
		if (NPCS.NPC->attackDebounceTime < level.time)
		{
			if (NPCS.NPC->client->NPC_class == CLASS_SABOTEUR)
			{
				Saboteur_Cloak(NPCS.NPC);
			}
		}
	}
}

void NPC_BSST_Default(void)
{
	if (NPCS.NPCInfo->scriptFlags & SCF_FIRE_WEAPON)
	{
		WeaponThink();
	}

	if (Q_stricmp("saboteur", NPCS.NPC->NPC_type) == 0) //Makes saboteur's cloak
	{
		if (NPCS.NPC->health <= 0 ||
			NPCS.NPC->client->ps.fd.forceGripBeingGripped > level.time ||
			NPCS.NPC->painDebounceTime > level.time)
		{
			//taking pain or being gripped
			Saboteur_Decloak(NPCS.NPC, 2000);
		}
		else if (NPCS.NPC->health > 0
			&& NPCS.NPC->client->ps.fd.forceGripBeingGripped <= level.time
			&& NPCS.NPC->painDebounceTime < level.time)
		{
			//still alive, not taking pain and not being gripped
			Saboteur_Cloak(NPCS.NPC);
		}
	}

	if (!NPCS.NPC->enemy)
	{
		//don't have an enemy, look for one
		NPC_BSST_Patrol();
	}
	else
	{
		//have an enemy
		if (NPCS.NPC->enemy->client //enemy is a client
			&& (NPCS.NPC->enemy->client->NPC_class == CLASS_UGNAUGHT
				|| NPCS.NPC->enemy->client->NPC_class == CLASS_JAWA) //enemy is a lowly jawa or ugnaught
			&& NPCS.NPC->enemy->enemy != NPCS.NPC //enemy's enemy is not me
			&& (!NPCS.NPC->enemy->enemy || !NPCS.NPC->enemy->enemy->client
				|| NPCS.NPC->enemy->enemy->client->NPC_class != CLASS_RANCOR
				&& NPCS.NPC->enemy->enemy->client->NPC_class != CLASS_WAMPA))
			//enemy's enemy is not a client or is not a wampa or rancor (which is scarier than me)
		{
			//they should be scared of ME and no-one else
			G_SetEnemy(NPCS.NPC->enemy, NPCS.NPC);
		}
		else
		{
			//have an enemy
			if (NPC_CanUseAdvancedFighting())
			{
				NPC_BSJedi_Default();
			}
			else
			{
				NPC_CheckGetNewWeapon();
				NPC_BSST_Attack();
			}
		}
	}
}