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

extern void Boba_FireDecide(void);

void Seeker_Strafe(void);

#define VELOCITY_DECAY		0.7f

#define	MIN_MELEE_RANGE		320
#define	MIN_MELEE_RANGE_SQR	( MIN_MELEE_RANGE * MIN_MELEE_RANGE )

#define MIN_DISTANCE		80
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define SEEKER_STRAFE_VEL	100
#define SEEKER_STRAFE_DIS	200
#define SEEKER_UPWARD_PUSH	32

#define SEEKER_FORWARD_BASE_SPEED	10
#define SEEKER_FORWARD_MULTIPLIER	2

#define SEEKER_SEEK_RADIUS			1024

//------------------------------------
void NPC_Seeker_Precache(void)
{
	G_SoundIndex("sound/chars/seeker/misc/fire.wav");
	G_SoundIndex("sound/chars/seeker/misc/hiss.wav");
	G_EffectIndex("env/small_explode");
}

//------------------------------------
void NPC_Seeker_Pain(gentity_t* self, gentity_t* attacker, const int damage)
{
	if (!(self->NPC->aiFlags & NPCAI_CUSTOM_GRAVITY))
	{
		G_Damage(self, NULL, NULL, vec3_origin, vec3_origin, 999, 0, MOD_FALLING);
	}

	//if we die, remove the control from our owner
	if (self->health < 0 && self->activator && self->activator->client)
	{
		self->activator->client->remote = NULL;
		self->activator->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SEEKER);
	}

	SaveNPCGlobals();
	SetNPCGlobals(self);
	Seeker_Strafe();
	RestoreNPCGlobals();
	NPC_Pain(self, attacker, damage);
}

extern float Q_flrand(float min, float max);
//------------------------------------
static void Seeker_MaintainHeight(void)
{
	float dif;

	// Update our angles regardless
	NPC_UpdateAngles(qtrue, qtrue);

	// If we have an enemy, we should try to hover at or a little below enemy eye level
	if (NPCS.NPC->enemy)
	{
		if (TIMER_Done(NPCS.NPC, "heightChange"))
		{
			TIMER_Set(NPCS.NPC, "heightChange", Q_irand(1000, 3000));

			// Find the height difference
			dif = NPCS.NPC->enemy->r.currentOrigin[2] + Q_flrand(NPCS.NPC->enemy->r.maxs[2] / 2,
				NPCS.NPC->enemy->r.maxs[2] + 8) - NPCS.NPC->r.
				currentOrigin[2];

			float dif_factor = 1.0f;
			if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
			{
				if (TIMER_Done(NPCS.NPC, "flameTime"))
				{
					dif_factor = 10.0f;
				}
			}

			// cap to prevent dramatic height shifts
			if (fabs(dif) > 2 * dif_factor)
			{
				if (fabs(dif) > 24 * dif_factor)
				{
					dif = dif < 0 ? -24 * dif_factor : 24 * dif_factor;
				}

				NPCS.NPC->client->ps.velocity[2] = (NPCS.NPC->client->ps.velocity[2] + dif) / 2;
			}
			if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
			{
				NPCS.NPC->client->ps.velocity[2] *= Q_flrand(0.85f, 3.0f);
			}
		}
	}
	else
	{
		const gentity_t* goal;

		if (NPCS.NPCInfo->goalEntity) // Is there a goal?
		{
			goal = NPCS.NPCInfo->goalEntity;
		}
		else
		{
			goal = NPCS.NPCInfo->lastGoalEntity;
		}
		if (goal)
		{
			dif = goal->r.currentOrigin[2] - NPCS.NPC->r.currentOrigin[2];

			if (fabs(dif) > 24)
			{
				NPCS.ucmd.upmove = NPCS.ucmd.upmove < 0 ? -4 : 4;
			}
			else
			{
				if (NPCS.NPC->client->ps.velocity[2])
				{
					NPCS.NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

					if (fabs(NPCS.NPC->client->ps.velocity[2]) < 2)
					{
						NPCS.NPC->client->ps.velocity[2] = 0;
					}
				}
			}
		}
	}

	// Apply friction
	if (NPCS.NPC->client->ps.velocity[0])
	{
		NPCS.NPC->client->ps.velocity[0] *= VELOCITY_DECAY;

		if (fabs(NPCS.NPC->client->ps.velocity[0]) < 1)
		{
			NPCS.NPC->client->ps.velocity[0] = 0;
		}
	}

	if (NPCS.NPC->client->ps.velocity[1])
	{
		NPCS.NPC->client->ps.velocity[1] *= VELOCITY_DECAY;

		if (fabs(NPCS.NPC->client->ps.velocity[1]) < 1)
		{
			NPCS.NPC->client->ps.velocity[1] = 0;
		}
	}
}

//------------------------------------
void Seeker_Strafe(void)
{
	int side;
	vec3_t end, right;
	trace_t tr;

	if (Q_flrand(0.0f, 1.0f) > 0.7f || !NPCS.NPC->enemy || !NPCS.NPC->enemy->client)
	{
		// Do a regular style strafe
		AngleVectors(NPCS.NPC->client->renderInfo.eyeAngles, NULL, right, NULL);

		// Pick a random strafe direction, then check to see if doing a strafe would be
		//	reasonably valid
		side = rand() & 1 ? -1 : 1;
		VectorMA(NPCS.NPC->r.currentOrigin, SEEKER_STRAFE_DIS * side, right, end);

		trap->Trace(&tr, NPCS.NPC->r.currentOrigin, NULL, NULL, end, NPCS.NPC->s.number, MASK_SOLID, qfalse, 0, 0);

		// Close enough
		if (tr.fraction > 0.9f)
		{
			float vel = SEEKER_STRAFE_VEL;
			float upPush = SEEKER_UPWARD_PUSH;
			if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT)
			{
				G_Sound(NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/chars/seeker/misc/hiss"));
			}
			else
			{
				vel *= 3.0f;
				upPush *= 4.0f;
			}
			VectorMA(NPCS.NPC->client->ps.velocity, vel * side, right, NPCS.NPC->client->ps.velocity);
			// Add a slight upward push
			NPCS.NPC->client->ps.velocity[2] += upPush;

			NPCS.NPCInfo->standTime = level.time + 1000 + Q_flrand(0.0f, 1.0f) * 500;
		}
	}
	else
	{
		vec3_t dir;
		// Do a strafe to try and keep on the side of their enemy
		AngleVectors(NPCS.NPC->enemy->client->renderInfo.eyeAngles, dir, right, NULL);

		// Pick a random side
		side = rand() & 1 ? -1 : 1;
		float stDis = SEEKER_STRAFE_DIS;
		if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
		{
			stDis *= 2.0f;
		}
		VectorMA(NPCS.NPC->enemy->r.currentOrigin, stDis * side, right, end);

		// then add a very small bit of random in front of/behind the player action
		VectorMA(end, Q_flrand(-1.0f, 1.0f) * 25, dir, end);

		trap->Trace(&tr, NPCS.NPC->r.currentOrigin, NULL, NULL, end, NPCS.NPC->s.number, MASK_SOLID, qfalse, 0, 0);

		// Close enough
		if (tr.fraction > 0.9f)
		{
			VectorSubtract(tr.endpos, NPCS.NPC->r.currentOrigin, dir);
			dir[2] *= 0.25; // do less upward change
			const float dis = VectorNormalize(dir);

			// Try to move the desired enemy side
			VectorMA(NPCS.NPC->client->ps.velocity, dis, dir, NPCS.NPC->client->ps.velocity);

			float upPush = SEEKER_UPWARD_PUSH;
			if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT)
			{
				G_Sound(NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/chars/seeker/misc/hiss"));
			}
			else
			{
				upPush *= 4.0f;
			}

			// Add a slight upward push
			NPCS.NPC->client->ps.velocity[2] += upPush;

			NPCS.NPCInfo->standTime = level.time + 2500 + Q_flrand(0.0f, 1.0f) * 500;
		}
	}
}

//------------------------------------
static void Seeker_Hunt(const qboolean visible, const qboolean advance)
{
	vec3_t forward;

	NPC_FaceEnemy(qtrue);

	if (NPCS.NPC->genericValue3)
	{
		if (TIMER_Done(NPCS.NPC, "seekerAlert"))
		{
			TIMER_Set(NPCS.NPC, "seekerAlert", level.time + 900);
			G_Sound(NPCS.NPC, CHAN_AUTO, NPCS.NPC->genericValue3);
		}
	}

	// If we're not supposed to stand still, pursue the player
	if (NPCS.NPCInfo->standTime < level.time)
	{
		// Only strafe when we can see the player
		if (visible)
		{
			Seeker_Strafe();
			return;
		}
	}

	// If we don't want to advance, stop here
	if (advance == qfalse)
	{
		return;
	}

	// Only try and navigate if the player is visible
	if (visible == qfalse)
	{
		// Move towards our goal
		NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
		NPCS.NPCInfo->goalRadius = 24;
		NPC_MoveToGoal(qtrue);
		return;
	}
	VectorSubtract(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, forward);

	const float speed = SEEKER_FORWARD_BASE_SPEED + SEEKER_FORWARD_MULTIPLIER * g_npcspskill.integer;
	VectorMA(NPCS.NPC->client->ps.velocity, speed, forward, NPCS.NPC->client->ps.velocity);
}

//------------------------------------
static qboolean Seeker_Fire(void)
{
#if 1

	vec3_t dir, enemy_org, muzzle;
	trace_t tr;
	const int randomnum = Q_irand(1, 5);

	if (randomnum == 1)
	{
		CalcEntitySpot(NPCS.NPC->enemy, SPOT_HEAD, enemy_org);
	}
	else if (randomnum == 2)
	{
		CalcEntitySpot(NPCS.NPC->enemy, SPOT_WEAPON, enemy_org);
	}
	else if (randomnum == 3)
	{
		CalcEntitySpot(NPCS.NPC->enemy, SPOT_ORIGIN, enemy_org);
	}
	else if (randomnum == 4)
	{
		CalcEntitySpot(NPCS.NPC->enemy, SPOT_CHEST, enemy_org);
	}
	else
	{
		CalcEntitySpot(NPCS.NPC->enemy, SPOT_LEGS, enemy_org);
	}

	enemy_org[0] += Q_irand(2, 15);
	enemy_org[1] += Q_irand(2, 15);

	//calculate everything based on our model offset
	VectorCopy(NPCS.NPC->r.currentOrigin, muzzle);
	//correct for our model offset
	muzzle[2] -= 22;
	muzzle[2] -= randomnum;

	VectorSubtract(enemy_org, NPCS.NPC->r.currentOrigin, dir);
	VectorNormalize(dir);

	// move a bit forward in the direction we shall shoot in so that the bolt doesn't poke out the other side of the seeker
	VectorMA(muzzle, 15, dir, muzzle);

	trap->Trace(&tr, muzzle, NULL, NULL, enemy_org, NPCS.NPC->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);
	//tr.fraction wont be 1.0f, since we hit our enemy.
	if (tr.entityNum != NPCS.NPC->enemy->s.number)
		return qfalse;

	gentity_t* missile = CreateMissile(muzzle, dir, 1000, 10000, NPCS.NPC, qfalse);

	G_PlayEffectID(G_EffectIndex("blaster/muzzle_flash"), muzzle, dir);

	missile->classname = "blaster";
	missile->s.weapon = WP_BLASTER;

	missile->damage = NPCS.NPC->damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_SEEKER;
	missile->clipmask = MASK_SHOT;

	return qtrue;

#else
	vec3_t		dir, enemy_org, muzzle;
	gentity_t* missile;

	CalcEntitySpot(NPCS.NPC->enemy, SPOT_HEAD, enemy_org);
	VectorSubtract(enemy_org, NPCS.NPC->r.currentOrigin, dir);
	VectorNormalize(dir);

	// move a bit forward in the direction we shall shoot in so that the bolt doesn't poke out the other side of the seeker
	VectorMA(NPCS.NPC->r.currentOrigin, 15, dir, muzzle);

	missile = CreateMissile(muzzle, dir, 1000, 10000, NPCS.NPC, qfalse);

	G_PlayEffectID(G_EffectIndex("blaster/muzzle_flash"), NPCS.NPC->r.currentOrigin, dir);

	missile->classname = "blaster";
	missile->s.weapon = WP_BLASTER;

	missile->damage = 5;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_SEEKER;
	missile->clipmask = MASK_SHOT;

#endif
}

//------------------------------------
static void Seeker_Ranged(const qboolean visible, const qboolean advance)
{
	if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT)
	{
		if (NPCS.NPC->count > 0 || NPCS.NPC->count == -1)
		{
			if (TIMER_Done(NPCS.NPC, "attackDelay")) // Attack?
			{
				TIMER_Set(NPCS.NPC, "attackDelay", Q_irand(NPCS.NPC->genericValue1, NPCS.NPC->genericValue2));
				Seeker_Fire();
				if (NPCS.NPC->count != -1)
					NPCS.NPC->count--;
			}
		}
		else
		{
			G_Damage(NPCS.NPC, NPCS.NPC, NPCS.NPC, NULL, NULL, NPCS.NPC->health, 0, MOD_UNKNOWN);
		}
	}

	if (NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)
	{
		Seeker_Hunt(visible, advance);
	}
}

//------------------------------------
static void Seeker_Attack(void)
{
	// Always keep a good height off the ground
	Seeker_MaintainHeight();

	// Rate our distance to the target, and our visibilty
	const float distance = DistanceHorizontalSquared(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin);
	const qboolean visible = NPC_ClearLOS4(NPCS.NPC->enemy);
	qboolean advance = distance > MIN_DISTANCE_SQR;

	//dont shoot at dead people
	if (!NPCS.NPC->enemy->inuse || NPCS.NPC->enemy->health <= 0)
	{
		NPCS.NPC->enemy = NULL;
		return;
	}

	if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
	{
		advance = (qboolean)(distance > 200.0f * 200.0f);
	}

	// If we cannot see our target, move to see it
	if (visible == qfalse)
	{
		if (NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)
		{
			Seeker_Hunt(visible, advance);
			return;
		}
		NPCS.NPC->enemy = NULL;
		if (NPCS.NPC->client->leader)
			NPCS.NPCInfo->goalEntity = NPCS.NPC->client->leader;
		return;
	}

	Seeker_Ranged(visible, advance);
}

//------------------------------------
static void Seeker_FindEnemy(void)
{
	float bestDis = SEEKER_SEEK_RADIUS * SEEKER_SEEK_RADIUS + 1;
	vec3_t mins, maxs;
	int entity_list[MAX_GENTITIES];
	gentity_t* best = NULL;
	float closestDist = SEEKER_SEEK_RADIUS * SEEKER_SEEK_RADIUS + 1;

	if (NPCS.NPC->activator && NPCS.NPC->activator->client->ps.weapon == WP_SABER && !NPCS.NPC->activator->client->ps.
		saberHolstered)
	{
		NPCS.NPC->enemy = NULL;
		return;
	}

	VectorSet(maxs, SEEKER_SEEK_RADIUS, SEEKER_SEEK_RADIUS, SEEKER_SEEK_RADIUS);
	VectorScale(maxs, -1, mins);
	//without this, the seekers are just scanning in terms of the world coordinates instead of around themselves, which is bad.
	VectorAdd(maxs, NPCS.NPC->r.currentOrigin, maxs);
	VectorAdd(mins, NPCS.NPC->r.currentOrigin, mins);

	const int num_found = trap->EntitiesInBox(mins, maxs, entity_list, MAX_GENTITIES);

	for (int i = 0; i < num_found; i++)
	{
		gentity_t* ent = &g_entities[entity_list[i]];

		if (ent->s.number == NPCS.NPC->s.number
			|| !ent->client
			|| ent->health <= 0
			|| !ent->inuse)
		{
			continue;
		}

		if (OnSameTeam(NPCS.NPC->activator, ent))
		{
			//our owner is on the same team as this entity, don't target them.
			continue;
		}

		//dont attack our owner
		if (NPCS.NPC->activator == ent)
			continue;

		if (ent->s.NPC_class == CLASS_VEHICLE)
			continue;

		if (ent->client->playerTeam == NPCS.NPC->client->playerTeam || ent->client->playerTeam == NPCTEAM_NEUTRAL)
			// don't attack same team or bots
		{
			continue;
		}

		const float dis = DistanceHorizontalSquared(NPCS.NPC->r.currentOrigin, ent->r.currentOrigin);

		if (dis <= closestDist)
			closestDist = dis;

		// try to find the closest visible one
		if (!NPC_ClearLOS4(ent))
		{
			continue;
		}

		if (dis <= bestDis)
		{
			bestDis = dis;
			best = ent;
		}
	}

	if (best)
	{
		if (!NPCS.NPC->enemy)
		{
			// used to offset seekers around a circle so they don't occupy the same spot.  This is not a fool-proof method.
			NPCS.NPC->random = Q_flrand(0.0f, 1.0f) * 6.3f; // roughly 2pi
			NPCS.NPC->enemy = best;
		}
	}
	//positive radius, check with los in mind
	if (NPCS.NPC->radius > 0)
	{
		if (best && bestDis <= NPCS.NPC->radius)
			NPCS.NPC->fly_sound_debounce_time = level.time + (int)floor(2500.0f * (bestDis / NPCS.NPC->radius)) + 500;
		else
			NPCS.NPC->fly_sound_debounce_time = -1;
	}
	//negitive radius, check only closest without los
	else if (NPCS.NPC->radius < 0)
	{
		if (closestDist <= -NPCS.NPC->radius)
			NPCS.NPC->fly_sound_debounce_time = level.time + (int)floor(2500.0f * (closestDist / -NPCS.NPC->radius)) +
			500;
		else
			NPCS.NPC->fly_sound_debounce_time = -1;
	}
}

//------------------------------------
static void Seeker_FollowPlayer(void)
{
	//hover around the closest player
#if 1
	float min_dist_sqr = MIN_DISTANCE_SQR;
	gentity_t* target;

	Seeker_MaintainHeight();

	if (NPCS.NPC->activator && NPCS.NPC->activator->client)
	{
		if (NPCS.NPC->activator->client->remote != NPCS.NPC || NPCS.NPC->activator->health <= 0)
		{
			//have us fall down and explode.
			NPCS.NPC->NPC->aiFlags |= NPCAI_CUSTOM_GRAVITY;
			return;
		}
		target = NPCS.NPCInfo->goalEntity;
		if (!target)
			target = NPCS.NPC->client->leader;
	}
	else
	{
		target = FindClosestPlayer(NPCS.NPC->r.currentOrigin, NPCS.NPC->client->playerTeam);
	}

	if (!target)
	{
		//in MP it's actually possible that there's no players on our team at the moment.
		return;
	}

	float dis = DistanceHorizontalSquared(NPCS.NPC->r.currentOrigin, target->r.currentOrigin);

	if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
	{
		if (TIMER_Done(NPCS.NPC, "flameTime"))
		{
			min_dist_sqr = 200 * 200;
		}
	}

	if (dis < min_dist_sqr)
	{
		vec3_t dir;
		vec3_t pt;
		// generally circle the player closely till we take an enemy..this is our target point
		if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
		{
			pt[0] = target->r.currentOrigin[0] + cos(level.time * 0.001f + NPCS.NPC->random) * 250;
			pt[1] = target->r.currentOrigin[1] + sin(level.time * 0.001f + NPCS.NPC->random) * 250;
			if (NPCS.NPC->client->jetPackTime < level.time)
			{
				pt[2] = target->r.currentOrigin[2] - 64;
			}
			else
			{
				pt[2] = target->r.currentOrigin[2] + 200;
			}
		}
		else
		{
			pt[0] = target->r.currentOrigin[0] + cos(level.time * 0.001f + NPCS.NPC->random) * 56;
			pt[1] = target->r.currentOrigin[1] + sin(level.time * 0.001f + NPCS.NPC->random) * 56;
			pt[2] = target->r.currentOrigin[2] + 40;
		}

		VectorSubtract(pt, NPCS.NPC->r.currentOrigin, dir);
		VectorMA(NPCS.NPC->client->ps.velocity, 0.8f, dir, NPCS.NPC->client->ps.velocity);
	}
	else
	{
		if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT)
		{
			if (TIMER_Done(NPCS.NPC, "seekerhiss"))
			{
				TIMER_Set(NPCS.NPC, "seekerhiss", 1000 + Q_flrand(0.0f, 1.0f) * 1000);
				G_Sound(NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/chars/seeker/misc/hiss"));
			}
		}

		// Hey come back!
		NPCS.NPCInfo->goalEntity = target;
		if (target == NPCS.NPC->enemy)
			NPCS.NPCInfo->goalRadius = 60;
		else
			NPCS.NPCInfo->goalRadius = 32;
		if (!NPC_MoveToGoal(qtrue))
		{
			//cant go there on our first try, abort.
			//this really isnt the best way... but if it cant reach the point, it will just sit there doing nothing.
			NPCS.NPCInfo->goalEntity = NPCS.NPC->client->leader;
			//stop chasing the enemy if we were told to, and return to the player
			NPCS.NPCInfo->scriptFlags &= ~SCF_CHASE_ENEMIES;
		}
	}

	//call this even if we do have an enemy, for enemy proximity detection
	if (NPCS.NPCInfo->enemyCheckDebounceTime < level.time)
	{
		// check twice a second to find a new enemy
		Seeker_FindEnemy();
		NPCS.NPCInfo->enemyCheckDebounceTime = level.time + 500;
	}

	//play our proximity beep
	if (NPCS.NPC->genericValue3 && NPCS.NPC->fly_sound_debounce_time > 0 && NPCS.NPC->fly_sound_debounce_time < level.
		time)
	{
		G_Sound(NPCS.NPC, CHAN_AUTO, NPCS.NPC->genericValue3);
		NPCS.NPC->fly_sound_debounce_time = -1;
	}

	NPC_UpdateAngles(qtrue, qtrue);

#else
	vec3_t	pt, dir;
	float	dis;
	float	minDistSqr = MIN_DISTANCE_SQR;
	gentity_t* closestPlayer = NULL;

	Seeker_MaintainHeight();

	closestPlayer = FindClosestPlayer(NPCS.NPC->r.currentOrigin, NPCS.NPC->client->playerTeam);

	if (!closestPlayer)
	{//in MP it's actually possible that there's no players on our team at the moment.
		return;
	}

	dis = DistanceHorizontalSquared(NPCS.NPC->r.currentOrigin, closestPlayer->r.currentOrigin);

	if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
	{
		if (TIMER_Done(NPCS.NPC, "flameTime"))
		{
			minDistSqr = 200 * 200;
		}
	}

	if (dis < minDistSqr)
	{
		// generally circle the player closely till we take an enemy..this is our target point
		if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
		{
			pt[0] = closestPlayer->r.currentOrigin[0] + cos(level.time * 0.001f + NPCS.NPC->random) * 250;
			pt[1] = closestPlayer->r.currentOrigin[1] + sin(level.time * 0.001f + NPCS.NPC->random) * 250;
			if (NPCS.NPC->client->jetPackTime < level.time)
			{
				pt[2] = closestPlayer->r.currentOrigin[2] - 64;
			}
			else
			{
				pt[2] = closestPlayer->r.currentOrigin[2] + 200;
			}
		}
		else
		{
			pt[0] = closestPlayer->r.currentOrigin[0] + cos(level.time * 0.001f + NPCS.NPC->random) * 56;
			pt[1] = closestPlayer->r.currentOrigin[1] + sin(level.time * 0.001f + NPCS.NPC->random) * 56;
			pt[2] = closestPlayer->r.currentOrigin[2] + 40;
		}

		VectorSubtract(pt, NPCS.NPC->r.currentOrigin, dir);
		VectorMA(NPCS.NPC->client->ps.velocity, 0.8f, dir, NPCS.NPC->client->ps.velocity);
	}
	else
	{
		if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT)
		{
			if (TIMER_Done(NPCS.NPC, "seekerhiss"))
			{
				TIMER_Set(NPCS.NPC, "seekerhiss", 1000 + random() * 1000);
				G_Sound(NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/chars/seeker/misc/hiss"));
			}
		}

		// Hey come back!
		NPCS.NPCInfo->goalEntity = closestPlayer;
		NPCS.NPCInfo->goalRadius = 32;
		NPCS.NPC_MoveToGoal(qtrue);
		NPCS.NPC->s.owner = closestPlayer->s.number;
	}

	if (NPCS.NPCInfo->enemyCheckDebounceTime < level.time)
	{
		// check twice a second to find a new enemy
		Seeker_FindEnemy();
		NPCS.NPCInfo->enemyCheckDebounceTime = level.time + 500;
	}

	NPC_UpdateAngles(qtrue, qtrue);
#endif
}

//------------------------------------
static void Seeker_FollowOwner(void)
{
	gentity_t* owner = &g_entities[NPCS.NPC->s.owner];

	Seeker_MaintainHeight();

	if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
	{
		owner = NPCS.NPC->enemy;
	}
	if (!owner || owner == NPCS.NPC || !owner->client)
	{
		return;
	}

	const float dis = DistanceHorizontalSquared(NPCS.NPC->r.currentOrigin, owner->r.currentOrigin);

	float min_dist_sqr = MIN_DISTANCE_SQR;

	if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
	{
		if (TIMER_Done(NPCS.NPC, "flameTime"))
		{
			min_dist_sqr = 200 * 200;
		}
	}

	if (dis < min_dist_sqr)
	{
		vec3_t dir;
		vec3_t pt;
		// generally circle the player closely till we take an enemy..this is our target point
		if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
		{
			pt[0] = owner->r.currentOrigin[0] + cos(level.time * 0.001f + NPCS.NPC->random) * 250;
			pt[1] = owner->r.currentOrigin[1] + sin(level.time * 0.001f + NPCS.NPC->random) * 250;
			if (NPCS.NPC->client->jetPackTime < level.time)
			{
				pt[2] = NPCS.NPC->r.currentOrigin[2] - 64;
			}
			else
			{
				pt[2] = owner->r.currentOrigin[2] + 200;
			}
		}
		else
		{
			pt[0] = owner->r.currentOrigin[0] + cos(level.time * 0.001f + NPCS.NPC->random) * 56;
			pt[1] = owner->r.currentOrigin[1] + sin(level.time * 0.001f + NPCS.NPC->random) * 56;
			pt[2] = owner->r.currentOrigin[2] + 40;
		}

		VectorSubtract(pt, NPCS.NPC->r.currentOrigin, dir);
		VectorMA(NPCS.NPC->client->ps.velocity, 0.8f, dir, NPCS.NPC->client->ps.velocity);
	}
	else
	{
		if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT)
		{
			if (TIMER_Done(NPCS.NPC, "seekerhiss"))
			{
				TIMER_Set(NPCS.NPC, "seekerhiss", 1000 + Q_flrand(0.0f, 1.0f) * 1000);
				G_Sound(NPCS.NPC, CHAN_AUTO, G_SoundIndex("sound/chars/seeker/misc/hiss"));
			}
		}

		// Hey come back!
		NPCS.NPCInfo->goalEntity = owner;
		NPCS.NPCInfo->goalRadius = 32;
		NPC_MoveToGoal(qtrue);
		NPCS.NPC->parent = owner;
	}

	if (NPCS.NPCInfo->enemyCheckDebounceTime < level.time)
	{
		// check twice a second to find a new enemy
		Seeker_FindEnemy();
		NPCS.NPCInfo->enemyCheckDebounceTime = level.time + 500;
	}

	NPC_UpdateAngles(qtrue, qtrue);
}

extern qboolean in_camera;
void NPC_BSST_Patrol(void);
//------------------------------------
void NPC_BSSeeker_Default(void)
{
	if (in_camera)
	{
		if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT && NPCS.NPC->client->NPC_class != CLASS_MANDO)
		{
			// cameras make me commit suicide....
			G_Damage(NPCS.NPC, NPCS.NPC, NPCS.NPC, NULL, NULL, 999, 0, MOD_UNKNOWN);
			return;
		}
	}

	if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT && NPCS.NPC->client->NPC_class != CLASS_MANDO && NPCS.NPC->
		activator->health <= 0)
	{
		G_Damage(NPCS.NPC, NPCS.NPC, NPCS.NPC, NULL, NULL, NPCS.NPC->health, 0, MOD_UNKNOWN);
		return;
	}

	if (NPCS.NPC->random == 0.0f)
	{
		// used to offset seekers around a circle so they don't occupy the same spot.  This is not a fool-proof method.
		NPCS.NPC->random = Q_flrand(0.0f, 1.0f) * 6.3f; // roughly 2pi
	}

	if (NPCS.NPC->enemy && NPCS.NPC->enemy->health && NPCS.NPC->enemy->inuse)
	{
		if (NPCS.NPC->client->NPC_class != CLASS_BOBAFETT && NPCS.NPC->client->NPC_class != CLASS_MANDO
			&& (NPCS.NPC->enemy == NPCS.NPC->activator || NPCS.NPC->enemy == NPCS.NPC->client->leader || !NPCS.NPC->
				enemy->inuse || NPCS.NPC->health <= 0 ||
				NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->NPC_class == CLASS_SEEKER))
		{
			//hacked to never take the player as an enemy, even if the player shoots at it
			NPCS.NPC->enemy = NULL;
		}
		else
		{
			Seeker_Attack();
			if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
			{
				Boba_FireDecide();
			}
			//still follow our target, be it a targeted enemy or our owner, but only if we aren't allowed to hunt down enemies.
			if (NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)
				return;
		}
	}
	else if (NPCS.NPC->client->NPC_class == CLASS_BOBAFETT)
	{
		//boba does a ST patrol if it doesn't have an enemy.
		NPC_BSST_Patrol();
		return;
	}

	// In all other cases, follow the player and look for enemies to take on
	Seeker_FollowPlayer();
}