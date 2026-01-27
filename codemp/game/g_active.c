/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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
#include "bg_saga.h"

extern void player_Cloak(gentity_t* self);
extern void Jedi_Cloak(gentity_t* self);
extern void Jedi_Decloak(gentity_t* self);
extern int IsPressingKickButton(const gentity_t* self);
qboolean PM_SaberInTransition(int move);
qboolean PM_SaberInStart(int move);
qboolean PM_SaberInReturn(int move);
qboolean saberCheckKnockdown_DuelLoss(gentity_t* saberent, gentity_t* saberOwner, const gentity_t* other);
extern void BG_ReduceSaberMishapLevel(playerState_t* ps);
extern qboolean G_ValidSaberStyle(const gentity_t* ent, int saber_style);
extern qboolean BG_SprintAnim(int anim);
extern void Weapon_GrapplingHook_Fire(gentity_t* ent);
extern void G_SoundOnEnt(gentity_t* ent, soundChannel_t channel, const char* sound_path);
extern qboolean BG_InSlowBounce(const playerState_t* ps);
extern qboolean PM_SaberInBrokenParry(int move);
extern qboolean PM_InKnockDown(const playerState_t* ps);
extern void Player_CheckBurn(const gentity_t* self);
extern void Player_CheckFreeze(const gentity_t* self);
extern void bg_reduce_blaster_mishap_level_advanced(playerState_t* ps);
extern qboolean manual_saberblocking(const gentity_t* defender);
extern float manual_running_and_saberblocking(const gentity_t* defender);
extern qboolean manual_meleeblocking(const gentity_t* defender);
extern qboolean manual_melee_dodging(const gentity_t* defender);
extern qboolean PM_SaberInAttackPure(int move);
extern int IsPressingDashButton(const gentity_t* self);
extern qboolean PM_StandingAnim(int anim);
extern qboolean PM_InKnockDownOnly(int anim);
extern qboolean PM_SaberInTransitionAny(int move);
extern qboolean PM_SaberInMassiveBounce(int anim);
extern qboolean PM_Saberinstab(int move);
extern float manual_npc_saberblocking(const gentity_t* defender);
extern float manual_npc_kick_absorbing(const gentity_t* defender);
extern qboolean PM_WalkingAnim(int anim);
extern qboolean PM_RunningAnim(int anim);
extern qboolean BG_IsAlreadyinTauntAnim(int anim);
qboolean WP_SaberStyleValidForSaber(const saberInfo_t* saber1, const saberInfo_t* saber2, const int saberHolstered, const int saberAnimLevel);
extern qboolean PM_SaberInBashedAnim(int anim);
extern qboolean saberKnockOutOfHand(gentity_t* saberent, gentity_t* saber_owner, vec3_t velocity);
extern qboolean BG_SaberSprintAnim(int anim);
extern qboolean BG_WeaponSprintAnim(int anim);
extern void Sphereshield_On(gentity_t* self);
extern void Sphereshield_Off(gentity_t* self);

static void P_SetTwitchInfo(gclient_t* client)
{
	client->ps.painTime = level.time;
	client->ps.painDirection ^= 1;
}

qboolean G_ControlledByPlayer(const gentity_t* self)
{
	if (self && self->NPC && self->NPC->controlledTime > level.time)
	{
		//being controlled
		const gentity_t* controller = &g_entities[0];
		if (controller->client && controller->client->ps.viewEntity == self->s.number)
		{
			//we're the player's viewEntity
			return qtrue;
		}
	}
	return qfalse;
}

/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the playerState_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
static void P_DamageFeedback(gentity_t* player)
{
	gclient_t* client = player->client;
	if (client->ps.pm_type == PM_DEAD || client->tempSpectate >= level.time)
	{
		return;
	}

	// total points of damage shot at the player this frame
	float count = client->damage_blood + client->damage_armor;
	if (count == 0)
	{
		return; // didn't take any damage
	}

	if (count > 255)
	{
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if (client->damage_fromWorld)
	{
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	}
	else
	{
		vec3_t angles;
		vectoangles(client->damage_from, angles);
		client->ps.damagePitch = angles[PITCH] / 360.0 * 256;
		client->ps.damageYaw = angles[YAW] / 360.0 * 256;

		//cap them since we can't send negative values in here across the net
		if (client->ps.damagePitch < 0)
		{
			client->ps.damagePitch = 0;
		}
		if (client->ps.damageYaw < 0)
		{
			client->ps.damageYaw = 0;
		}
	}

	// play an appropriate pain sound
	if (level.time > player->pain_debounce_time && !(player->flags & FL_GODMODE) && !(player->s.eFlags & EF_DEAD) &&
		player->client->tempSpectate < level.time)
	{
		// don't do more than two pain sounds a second
		// nmckenzie: also don't make him loud and whiny if he's only getting nicked.
		if (level.time - client->ps.painTime < 1500 || count < 10)
		{
			return;
		}
		P_SetTwitchInfo(client);
		player->pain_debounce_time = level.time + 700;

		G_AddEvent(player, EV_PAIN, player->health);
		client->ps.damageEvent++;

		if (client->damage_armor && !client->damage_blood)
		{
			client->ps.damageType = 1; //pure shields
		}
		else if (client->damage_armor)
		{
			client->ps.damageType = 2; //shields and health
		}
		else
		{
			client->ps.damageType = 0; //pure health
		}
	}

	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_knockback = 0;
}

/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
extern qboolean G_PointInBounds(vec3_t point, vec3_t mins, vec3_t maxs);
extern void NPC_SetAnim(gentity_t* ent, int setAnimParts, int anim, int setAnimFlags);
extern void WP_ForcePowerStart(const gentity_t* self, forcePowers_t forcePower, int overrideAmt);

static void P_WorldEffects(gentity_t* ent)
{
	if (ent->client->noclip)
	{
		ent->client->airOutTime = level.time + 12000; // don't need air
		return;
	}

	const int waterlevel = ent->waterlevel;

	const qboolean envirosuit = ent->client->ps.powerups[PW_BATTLESUIT] > level.time;

	//
	// check for drowning
	//
	if (waterlevel == 3)
	{
		// envirosuit give air
		if (envirosuit)
		{
			ent->client->airOutTime = level.time + 10000;
		}

		// if out of air, start drowning
		if (ent->client->NPC_class == CLASS_SWAMPTROOPER)
		{
			//they have air tanks
			ent->client->airOutTime = level.time + 12000; // don't need air
			ent->damage = 2;
		}
		else if (ent->client->airOutTime < level.time)
		{
			// drown!
			ent->client->airOutTime += 1000;
			if (ent->health > 0 && ent->client->tempSpectate < level.time)
			{
				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
					ent->damage = 15;

				// play a gurp sound instead of a normal pain sound
				if (ent->health <= ent->damage)
				{
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp1.wav"));
				}
				else if (rand() & 1)
				{
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp1.wav"));
				}
				else
				{
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp2.wav"));
				}

				// don't play a normal pain sound
				ent->pain_debounce_time = level.time + 200;

				G_Damage(ent, NULL, NULL, NULL, NULL, ent->damage, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	}
	else
	{
		ent->client->airOutTime = level.time + 12000;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (waterlevel && ent->watertype & (CONTENTS_LAVA | CONTENTS_SLIME))
	{
		if (ent->health > 0 && ent->client->tempSpectate < level.time && ent->pain_debounce_time <= level.time)
		{
			if (envirosuit)
			{
				G_AddEvent(ent, EV_POWERUP_BATTLESUIT, 0);
			}
			else
			{
				if (ent->watertype & CONTENTS_LAVA)
				{
					G_Damage(ent, NULL, NULL, NULL, NULL, 30 * waterlevel, 0, MOD_LAVA);
				}

				if (ent->watertype & CONTENTS_SLIME)
				{
					G_Damage(ent, NULL, NULL, NULL, NULL, 10 * waterlevel, 0, MOD_SLIME);
				}
			}
		}
	}

	// Poisoned?
	if (ent->client->poisonDamage && ent->client->tempSpectate < level.time && ent->client->poisonTime < level.time)
	{
		ent->client->poisonDamage -= 4;
		ent->client->poisonTime = level.time + 1000;
		G_Damage(ent, NULL, NULL, 0, 0, 2, DAMAGE_NO_KNOCKBACK | DAMAGE_NO_ARMOR, MOD_UNKNOWN); //FIXME: MOD_POISON?

		if (ent->client->poisonDamage < 0)
		{
			ent->client->poisonDamage = 0;
		}
	}

	// stunned?
	if (ent->client->stunDamage && ent->client->tempSpectate < level.time && ent->client->stunTime < level.time)
	{
		ent->client->stunDamage -= 2;
		ent->client->stunTime = level.time + 1000;
		G_Damage(ent, NULL, NULL, 0, 0, 2, DAMAGE_NO_KNOCKBACK | DAMAGE_NO_ARMOR, MOD_UNKNOWN);

		if (ent->client->stunDamage < 0)
		{
			ent->client->stunDamage = 0;
		}
	}

	if (ent->client->AmputateDamage && ent->client->tempSpectate < level.time && ent->client->AmputateTime < level.time)
	{
		ent->client->AmputateDamage -= 2;
		ent->client->AmputateTime = level.time + 1000;

		if (ent->s.number && ent->health > 100)
		{
			G_Damage(ent, NULL, NULL, 0, 0, 200, DAMAGE_NO_KNOCKBACK | DAMAGE_NO_ARMOR, MOD_UNKNOWN);
		}
		else
		{
			G_Damage(ent, NULL, NULL, 0, 0, 25, DAMAGE_NO_KNOCKBACK | DAMAGE_NO_ARMOR, MOD_UNKNOWN);
		}

		if (ent->client->AmputateDamage < 0)
		{
			ent->client->AmputateDamage = 0;
		}
	}

	//in space?
	if (ent->client->inSpaceIndex && ent->client->inSpaceIndex != ENTITYNUM_NONE)
	{
		//we're in space, check for suffocating and for exiting
		gentity_t* spacetrigger = &g_entities[ent->client->inSpaceIndex];

		if (!spacetrigger->inuse ||
			!G_PointInBounds(ent->client->ps.origin, spacetrigger->r.absmin, spacetrigger->r.absmax))
		{
			//no longer in space then I suppose
			ent->client->inSpaceIndex = 0;
		}
		else
		{
			//check for suffocation
			if (ent->client->inSpaceSuffocation < level.time)
			{
				//suffocate!
				if (ent->health > 0)
				{
					//if they're still alive..
					G_Damage(ent, spacetrigger, spacetrigger, NULL, ent->client->ps.origin, Q_irand(20, 40),
						DAMAGE_NO_ARMOR, MOD_SUICIDE);

					if (ent->health > 0)
					{
						//did that last one kill them?
						//play the choking sound
						G_SoundOnEnt(ent, CHAN_VOICE, va("*choke%d.wav", Q_irand(1, 3)));

						if (ent->health > 70)
						{
							//make them grasp their throat
							NPC_SetAnim(ent, SETANIM_BOTH, BOTH_CHOKE4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else if (ent->health > 50)
						{
							//make them grasp their throat
							NPC_SetAnim(ent, SETANIM_BOTH, BOTH_CHOKE3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
							if (ent->client->NPC_class == CLASS_OBJECT)
							{
								//
							}
							else
							{
								G_SoundOnEnt(ent, CHAN_BODY, "sound/weapons/force/grip.mp3");
							}
						}
						else
						{
							//make them grasp their throat
							NPC_SetAnim(ent, SETANIM_BOTH, BOTH_CHOKE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}

				ent->client->inSpaceSuffocation = level.time + Q_irand(1000, 2000);
			}
		}
	}
}

//==============================================================
extern void G_ApplyKnockback(gentity_t* targ, vec3_t new_dir, float knockback);
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength,
	qboolean break_saber_lock);

void G_GetMassAndVelocityForEnt(const gentity_t* ent, float* mass, vec3_t velocity)
{
	if (ent->client)
	{
		VectorCopy(ent->client->ps.velocity, velocity);
		if (!ent->mass)
		{
			*mass = 10;
		}
		else
		{
			*mass = ent->mass;
		}
	}
	else
	{
		VectorCopy(ent->s.pos.trDelta, velocity);
		if (ent->s.pos.trType == TR_GRAVITY)
		{
			velocity[2] -= 0.25f * g_gravity.value;
		}
		if (!ent->mass)
		{
			*mass = 1;
		}
		else if (ent->mass <= 10)
		{
			*mass = 10;
		}
		else
		{
			*mass = ent->mass; ///10;
		}
	}
}

void DoImpact(gentity_t* self, gentity_t* other, const qboolean damageSelf)
{
	float my_mass;
	vec3_t velocity;

	G_GetMassAndVelocityForEnt(self, &my_mass, velocity);

	float magnitude = VectorLength(velocity) * my_mass / 50;

	if (!self->client || self->client->ps.lastOnGround + 300 < level.time || self->client->ps.lastOnGround + 100 < level
		.time)
	{
		float force = 0;
		vec3_t defaultDir;

		if (other->material == MAT_GLASS
			|| other->material == MAT_GLASS_METAL
			|| other->material == MAT_GRATE1
			|| other->flags & FL_BBRUSH && other->spawnflags & 8
			|| other->r.svFlags & SVF_GLASS_BRUSH)
		{
			//glass and thin breakable brushes (axially aligned only, unfortunately) take more impact damage
			magnitude *= 2;
		}

		VectorSet(defaultDir, 0, 0, 1);

		if (self->client && PM_InKnockDown(&self->client->ps) && magnitude >= 120)
		{
			//FORCE-SMACKED into something
			if (TIMER_Done(self, "impactEffect"))
			{
				vec3_t right;
				vec3_t effectPos;
				AngleVectors(self->r.currentAngles, NULL, right, NULL);
				VectorMA(self->r.currentOrigin, 10, right, effectPos);
				effectPos[2] -= 15;
				G_PlayEffectID(G_EffectIndex("env/impact_dustonly"), effectPos, defaultDir);
				G_Sound(self, CHAN_AUTO, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
				TIMER_Set(self, "impactEffect", 1000);
			}
		}

		//damage them
		if (magnitude >= 100 && other->s.number < ENTITYNUM_WORLD)
		{
			vec3_t dir2;
			vec3_t dir1;
			VectorCopy(velocity, dir1);
			VectorNormalize(dir1);
			if (VectorCompare(other->r.currentOrigin, vec3_origin))
			{
				//a brush with no origin
				VectorCopy(dir1, dir2);
			}
			else
			{
				VectorSubtract(other->r.currentOrigin, self->r.currentOrigin, dir2);
				VectorNormalize(dir2);
			}

			const float dot = DotProduct(dir1, dir2);

			if (dot >= 0.2)
			{
				force = dot;
			}
			else
			{
				force = 0;
			}

			force *= magnitude / 50;

			const int cont = trap->PointContents(other->r.absmax, other->s.number);

			if (cont & CONTENTS_WATER)
			{
				force *= 0.33333f;
			}

			if (self->NPC && other->s.number == ENTITYNUM_WORLD)
			{
				//NPCs take less damage
				force *= 0.5f;
			}

			if (self->s.number >= MAX_CLIENTS && self->client && PM_InKnockDown(&self->client->ps))
			{
				force *= 10;
			}

			if (force >= 1 && other->s.number >= MAX_CLIENTS || force >= 10)
			{
				if (other->r.svFlags & SVF_GLASS_BRUSH)
				{
					other->splashRadius = (self->maxs[0] - self->mins[0]) / 4.0f;
				}

				if (self->forcePushTime > level.time - 1000 //was force pushed/pulled in the last 1600 milliseconds
					&& self->forcePuller == other->s.number) //hit the person who pushed/pulled me
				{
					//ignore the impact
				}
				else if (other->takedamage)
				{
					if (!self->client || other->s.number < MAX_CLIENTS || !other->client)
					{
						//aw, fuck it, clients no longer take impact damage from other clients, unless you're the player
						if (other->client //he's a client
							&& self->client //I'm a client
							&& other->client->ps.fd.forceGripentity_num == self->s.number) //he's force-gripping me
						{
							//don't damage the other guy if he's gripping me
						}
						else
						{
							G_Damage(other, self, self, velocity, self->r.currentOrigin, force, DAMAGE_NO_ARMOR,
								MOD_CRUSH);
						}
					}
					else
					{
						//Hmm, maybe knockdown?
						g_throw(other, dir2, force);
					}
					if (other->health > 0)
					{
						//still alive?
						if (other->client)
						{
							if (self->client)
							{
								if (PM_InKnockDown(&self->client->ps))
								{
									G_Knockdown(other, self, dir2, Q_irand(200, 400), qtrue);
								}
							}
							else if (self->forcePuller != ENTITYNUM_NONE
								&& g_entities[self->forcePuller].client
								&& self->mass > Q_irand(50, 100))
							{
								G_Knockdown(other, &g_entities[self->forcePuller], dir2, Q_irand(200, 400), qtrue);
							}
						}
					}
				}
				else
				{
					//Hmm, maybe knockdown?
					G_ApplyKnockback(other, dir2, force);
				}
			}
		}

		if (damageSelf && self->takedamage && !(self->flags & FL_NO_IMPACT_DMG))
		{
			if (self->client && self->client->ps.fd.forceJumpZStart)
			{
				//we were force-jumping
				if (self->r.currentOrigin[2] >= self->client->ps.fd.forceJumpZStart)
				{
					//we landed at same height or higher than we landed
					magnitude = 0;
				}
				else
				{
					magnitude = (self->client->ps.fd.forceJumpZStart - self->r.currentOrigin[2]) / 3;
				}
			}

			if ((magnitude >= 100 + self->health
				&& self->s.number >= MAX_CLIENTS
				&& self->s.weapon != WP_SABER)
				|| (magnitude >= 700))
			{//health here is used to simulate structural integrity
				if ((self->s.weapon == WP_SABER)
					&& self->client
					&& self->client->ps.groundEntityNum < ENTITYNUM_NONE
					&& magnitude < 1000)
				{//players and jedi take less impact damage
					//allow for some lenience on high falls
					magnitude /= 2;
				}
				//drop it some (magic number... sigh)
				magnitude /= 40;
				//If damage other, subtract half of that damage off of own injury
				if (other->material != MAT_GLASS)
				{
					//take off only a little because we broke architecture (not including glass), that should hurt
					magnitude = magnitude - force / 8;
				}
				else
				{
					//take off half damage we did to it
					magnitude = magnitude - force / 2;
				}
				if (magnitude >= 1)
				{
					G_Damage(self, NULL, NULL, NULL, self->r.currentOrigin, magnitude / 2, DAMAGE_NO_ARMOR, MOD_FALLING);
				}
			}
		}
	}
}

static void DoImpactPlayer(gentity_t* self, gentity_t* other, const qboolean damageSelf) //mp
{
	float my_mass;
	vec3_t velocity;
	qboolean easyBreakBrush = qtrue;

	if (self->client)
	{
		VectorCopy(self->client->ps.velocity, velocity);
		if (!self->mass)
		{
			my_mass = 10;
		}
		else
		{
			my_mass = self->mass;
		}
	}
	else
	{
		VectorCopy(self->s.pos.trDelta, velocity);
		if (self->s.pos.trType == TR_GRAVITY)
		{
			velocity[2] -= 0.25f * g_gravity.value;
		}
		if (!self->mass)
		{
			my_mass = 1;
		}
		else if (self->mass <= 10)
		{
			my_mass = 10;
		}
		else
		{
			my_mass = self->mass; ///10;
		}
	}

	float magnitude = VectorLength(velocity) * my_mass / 10;

	if (other->material == MAT_GLASS
		|| other->material == MAT_GLASS_METAL
		|| other->material == MAT_GRATE1
		|| other->flags & FL_BBRUSH && other->spawnflags & 8 /*THIN*/
		|| other->r.svFlags & SVF_GLASS_BRUSH)
	{
		easyBreakBrush = qtrue;
	}

	if (!self->client || self->client->ps.lastOnGround + 300 < level.time || self->client->ps.lastOnGround + 100 < level
		.time && easyBreakBrush)
	{
		float force = 0;

		if (easyBreakBrush)
			magnitude *= 2;

		//damage them
		if (magnitude >= 100 && other->s.number < ENTITYNUM_WORLD)
		{
			vec3_t dir2;
			vec3_t dir1;
			VectorCopy(velocity, dir1);
			VectorNormalize(dir1);
			if (VectorCompare(other->r.currentOrigin, vec3_origin))
			{
				//a brush with no origin
				VectorCopy(dir1, dir2);
			}
			else
			{
				VectorSubtract(other->r.currentOrigin, self->r.currentOrigin, dir2);
				VectorNormalize(dir2);
			}

			const float dot = DotProduct(dir1, dir2);

			if (dot >= 0.2)
			{
				force = dot;
			}
			else
			{
				force = 0;
			}

			force *= magnitude / 50;

			const int cont = trap->PointContents(other->r.absmax, other->s.number);
			if (cont & CONTENTS_WATER)
			{
				force /= 3; //water absorbs 2/3 velocity
			}

			if (force >= 1 && other->s.number >= MAX_CLIENTS || force >= 10)
			{
				if (other->r.svFlags & SVF_GLASS_BRUSH)
				{
					other->splashRadius = (self->r.maxs[0] - self->r.mins[0]) / 4.0f;
				}
				if (other->takedamage)
				{
					G_Damage(other, self, self, velocity, self->r.currentOrigin, force, DAMAGE_NO_ARMOR, MOD_CRUSH);
				}
				else
				{
					G_ApplyKnockback(other, dir2, force);
				}
			}
		}

		if (damageSelf && self->takedamage)
		{
			//FIXME: more lenient falling damage, especially for when driving a vehicle
			if (self->client && self->client->ps.fd.forceJumpZStart)
			{
				//we were force-jumping
				if (self->r.currentOrigin[2] >= self->client->ps.fd.forceJumpZStart)
				{
					//we landed at same height or higher than we landed
					magnitude = 0;
				}
				else
				{
					//FIXME: take off some of it, at least?
					magnitude = (self->client->ps.fd.forceJumpZStart - self->r.currentOrigin[2]) / 3;
				}
			}
			if (magnitude >= 100 + self->health && self->s.number >= MAX_CLIENTS && self->s.weapon != WP_SABER ||
				magnitude >= 700)
			{
				if (self->s.weapon == WP_SABER && self->client && self->client->ps.groundEntityNum < ENTITYNUM_NONE &&
					magnitude < 1000)
				{
					//players and jedi take less impact damage
					//allow for some lenience on high falls
					magnitude /= 2;
				}
				magnitude /= 40;
				magnitude = magnitude - force / 2; //If damage other, subtract half of that damage off of own injury
				if (magnitude >= 1)
				{
					G_Damage(self, NULL, NULL, NULL, self->r.currentOrigin, magnitude / 2, DAMAGE_NO_ARMOR,
						MOD_FALLING);
				}
			}
		}
	}
}

void Client_CheckImpactBBrush(gentity_t* self, gentity_t* other)
{
	if (!other || !other->inuse)
	{
		return;
	}
	if (!self || !self->inuse || !self->client ||
		self->client->tempSpectate >= level.time ||
		self->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		//hmm.. let's not let spectators ram into breakables.
		return;
	}

	if (other->material == MAT_GLASS
		|| other->material == MAT_GLASS_METAL
		|| other->material == MAT_GRATE1
		|| other->flags & FL_BBRUSH && other->spawnflags & 8 /*THIN*/
		|| other->flags & FL_BBRUSH && other->health <= 10
		|| other->r.svFlags & SVF_GLASS_BRUSH)
	{
		//clients only do impact damage against easy-break breakables
		DoImpactPlayer(self, other, qfalse);
	}
}

/*
===============
G_SetClientSound
===============
*/
static void G_SetClientSound(gentity_t* ent)
{
	if (ent->client && ent->client->isHacking)
	{
		//loop hacking sound
		ent->client->ps.loopSound = level.snd_hack;
		ent->s.loopIsSoundset = qfalse;
	}
	else if (ent->client && ent->client->isMedHealed > level.time)
	{
		//loop healing sound
		ent->client->ps.loopSound = level.snd_medHealed;
		ent->s.loopIsSoundset = qfalse;
	}
	else if (ent->client && ent->client->isMedSupplied > level.time)
	{
		//loop supplying sound
		ent->client->ps.loopSound = level.snd_medSupplied;
		ent->s.loopIsSoundset = qfalse;
	}
	else if (ent->client && ent->waterlevel && ent->watertype & (CONTENTS_LAVA | CONTENTS_SLIME))
	{
		ent->client->ps.loopSound = level.snd_fry;
		ent->s.loopIsSoundset = qfalse;
	}
	else if (ent->client)
	{
		ent->client->ps.loopSound = 0;
		ent->s.loopIsSoundset = qfalse;
	}
	else
	{
		ent->s.loopSound = 0;
		ent->s.loopIsSoundset = qfalse;
	}
}

//==============================================================

/*
==============
ClientImpacts
==============
*/
static void ClientImpacts(gentity_t* ent, const pmove_t* pmove)
{
	int j;
	trace_t trace = { 0 };

	for (int i = 0; i < pmove->numtouch; i++)
	{
		for (j = 0; j < i; j++)
		{
			if (pmove->touchents[j] == pmove->touchents[i])
			{
				break;
			}
		}
		if (j != i)
		{
			continue; // duplicated
		}
		gentity_t* other = &g_entities[pmove->touchents[i]];

		if (ent->r.svFlags & SVF_BOT && ent->touch)
		{
			ent->touch(ent, other, &trace);
		}

		if (!other->touch)
		{
			continue;
		}

		other->touch(other, ent, &trace);
	}
}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void G_TouchTriggers(gentity_t* ent)
{
	int touch[MAX_GENTITIES];
	trace_t trace;
	vec3_t mins, maxs;
	static vec3_t range = { 40, 40, 52 };

	if (!ent->client)
	{
		return;
	}

	// dead clients don't activate triggers!
	if (ent->client->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	VectorSubtract(ent->client->ps.origin, range, mins);
	VectorAdd(ent->client->ps.origin, range, maxs);

	const int num = trap->EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	// can't use ent->r.absmin, because that has a one unit pad
	VectorAdd(ent->client->ps.origin, ent->r.mins, mins);
	VectorAdd(ent->client->ps.origin, ent->r.maxs, maxs);

	for (int i = 0; i < num; i++)
	{
		gentity_t* hit = &g_entities[touch[i]];

		if (!hit->touch && !ent->touch)
		{
			continue;
		}
		if (!(hit->r.contents & CONTENTS_TRIGGER))
		{
			continue;
		}

		// ignore most entities if a spectator
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			if (hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
				hit->touch != Touch_DoorTrigger)
			{
				continue;
			}
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if (hit->s.eType == ET_ITEM)
		{
			if (!BG_PlayerTouchesItem(&ent->client->ps, &hit->s, level.time))
			{
				continue;
			}
		}
		else
		{
			if (!trap->EntityContact(mins, maxs, (sharedEntity_t*)hit, qfalse))
			{
				continue;
			}
		}

		memset(&trace, 0, sizeof trace);

		if (hit->touch)
		{
			hit->touch(hit, ent, &trace);
		}

		if (ent->r.svFlags & SVF_BOT && ent->touch)
		{
			ent->touch(ent, hit, &trace);
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if (ent->client->ps.jumppad_frame != ent->client->ps.pmove_framecount)
	{
		ent->client->ps.jumppad_frame = 0;
		ent->client->ps.jumppad_ent = 0;
	}
}

/*
============
G_MoverTouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void g_mover_touch_push_triggers(gentity_t* ent, vec3_t old_org)
{
	trace_t trace;
	vec3_t dir, size;
	const vec3_t range = { 40, 40, 52 };

	// non-moving movers don't hit triggers!
	if (!VectorLengthSquared(ent->s.pos.trDelta))
	{
		return;
	}

	VectorSubtract(ent->r.mins, ent->r.maxs, size);
	float stepSize = VectorLength(size);
	if (stepSize < 1)
	{
		stepSize = 1;
	}

	VectorSubtract(ent->r.currentOrigin, old_org, dir);
	const float dist = VectorNormalize(dir);
	for (float step = 0; step <= dist; step += stepSize)
	{
		vec3_t checkSpot;
		vec3_t maxs;
		vec3_t mins;
		int touch[MAX_GENTITIES];
		VectorMA(ent->r.currentOrigin, step, dir, checkSpot);
		VectorSubtract(checkSpot, range, mins);
		VectorAdd(checkSpot, range, maxs);

		const int num = trap->EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

		// can't use ent->r.absmin, because that has a one unit pad
		VectorAdd(checkSpot, ent->r.mins, mins);
		VectorAdd(checkSpot, ent->r.maxs, maxs);

		for (int i = 0; i < num; i++)
		{
			gentity_t* hit = &g_entities[touch[i]];

			if (hit->s.eType != ET_PUSH_TRIGGER)
			{
				continue;
			}

			if (hit->touch == NULL)
			{
				continue;
			}

			if (!(hit->r.contents & CONTENTS_TRIGGER))
			{
				continue;
			}

			if (!trap->EntityContact(mins, maxs, (sharedEntity_t*)hit, qfalse))
			{
				continue;
			}

			memset(&trace, 0, sizeof trace);

			if (hit->touch != NULL)
			{
				hit->touch(hit, ent, &trace);
			}
		}
	}
}

static void SV_PMTrace(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
	const int pass_entity_num, const int content_mask)
{
	trap->Trace(results, start, mins, maxs, end, pass_entity_num, content_mask, qfalse, 0, 10);
}

/*
=================
SpectatorThink
=================
*/
static void SpectatorThink(gentity_t* ent, const usercmd_t* ucmd)
{
	pmove_t pmove;

	gclient_t* client = ent->client;

	if (client->sess.spectatorState != SPECTATOR_FOLLOW)
	{
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed = 400; // faster than normal
		client->ps.basespeed = 400;

		//hmm, shouldn't have an anim if you're a spectator, make sure
		//it gets cleared.
		client->ps.legsAnim = 0;
		client->ps.legsTimer = 0;
		client->ps.torsoAnim = 0;
		client->ps.torsoTimer = 0;

		// set up for pmove
		memset(&pmove, 0, sizeof pmove);
		pmove.ps = &client->ps;
		pmove.cmd = *ucmd;
		pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY; // spectators can fly through bodies
		pmove.trace = SV_PMTrace;
		pmove.pointcontents = trap->PointContents;

		pmove.noSpecMove = g_noSpecMove.integer;

		pmove.animations = NULL;
		pmove.nonHumanoid = qfalse;

		//Set up bg entity data
		pmove.baseEnt = (bgEntity_t*)g_entities;
		pmove.entSize = sizeof(gentity_t);

		// perform a pmove
		Pmove(&pmove);
		// save results of pmove
		VectorCopy(client->ps.origin, ent->s.origin);

		if (ent->client->tempSpectate < level.time)
		{
			G_TouchTriggers(ent);
		}
		trap->UnlinkEntity((sharedEntity_t*)ent);
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

	if (client->tempSpectate < level.time)
	{
		// attack button cycles through spectators
		if (client->buttons & BUTTON_ATTACK && !(client->oldbuttons & BUTTON_ATTACK))
			Cmd_FollowCycle_f(ent, 1);

		else if (client->sess.spectatorState == SPECTATOR_FOLLOW && client->buttons & BUTTON_ALT_ATTACK && !(client->
			oldbuttons & BUTTON_ALT_ATTACK))
			Cmd_FollowCycle_f(ent, -1);

		if (client->sess.spectatorState == SPECTATOR_FOLLOW && ucmd->upmove > 0)
		{
			//jump now removes you from follow mode
			StopFollowing(ent);
		}
	}
}

/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
static qboolean ClientInactivityTimer(gclient_t* client)
{
	if (!g_inactivity.integer)
	{
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	}
	else if (client->pers.cmd.forwardmove ||
		client->pers.cmd.rightmove ||
		client->pers.cmd.upmove ||
		client->pers.cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK))
	{
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = qfalse;
	}
	else if (!client->pers.localClient)
	{
		if (level.time > client->inactivityTime)
		{
			trap->DropClient(client - level.clients, "Dropped due to inactivity");
			return qfalse;
		}
		if (level.time > client->inactivityTime - 10000 && !client->inactivityWarning)
		{
			client->inactivityWarning = qtrue;
			trap->SendServerCommand(client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"");
		}
	}
	return qtrue;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
extern void WP_SaberFatigueRegenerate(int override_amt);
extern void WP_BlasterFatigueRegenerate(int override_amt);
extern void G_Rename_Player(gentity_t* player, const char* newname);
extern char* PickName(void);

static void ClientTimerActions(gentity_t* ent, const int msec)
{
	gclient_t* client = ent->client;
	client->timeResidual += msec;
	int clientNum = ent - g_entities;

	while (client->timeResidual >= 1000)
	{
		client->timeResidual -= 1000;

		// if out of trip mines or thermals, remove them from weapon selection
		if (client->ps.ammo[AMMO_THERMAL] == 0)
		{
			client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_THERMAL);
		}

		if (client->ps.ammo[AMMO_TRIPMINE] == 0)
		{
			client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_TRIP_MINE);
		}

		if (!(ent->r.svFlags & SVF_BOT) &&
			!PM_SaberInAttack(ent->client->ps.saber_move)
			&& !(ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK)
			&& !ent->client->poisonTime
			&& !ent->client->stunTime
			&& !ent->client->AmputateTime
			&& ent->client->NPC_class != CLASS_GALAKMECH
			&& ent->client->ps.fd.forceRageRecoveryTime < level.time
			&& !(ent->client->ps.fd.forcePowersActive & 1 << FP_RAGE))
		{
			//you heal 1 hp every 1 second.
			if (ent->client->ps.fd.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_2)
			{
				if (ent->health < client->ps.stats[STAT_MAX_HEALTH])
				{
					ent->health++;
				}
			}

			if (ent->client->ps.fd.forcePowerLevel[FP_PROTECT] > FORCE_LEVEL_2)
			{
				if (client->ps.stats[STAT_ARMOR] < client->ps.stats[STAT_MAX_HEALTH])
				{
					client->ps.stats[STAT_ARMOR]++;
				}
			}
		}

		// count down armor when over max
		if (client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH])
		{
			client->ps.stats[STAT_ARMOR]--;
		}

		// count down health when over max
		if (ent->health > client->ps.stats[STAT_MAX_HEALTH])
		{
			ent->health--;
		}

		if (!(ent->client->buttons & BUTTON_ATTACK) && !(ent->client->buttons & BUTTON_ALT_ATTACK))
		{
			if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_MIN && ent->client->ps.weaponTime < 1)
			{
				if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_ELEVEN)
				{
					WP_BlasterFatigueRegenerate(4);
				}
				else
				{
					WP_BlasterFatigueRegenerate(1);
				}
			}
		}

		if (ent->client->ps.fd.forcePowersActive & 1 << FP_SEE)
		{
			bg_reduce_blaster_mishap_level_advanced(&ent->client->ps);
		}

		if (ent->client->ps.saberFatigueChainCount > MISHAPLEVEL_NONE
			&& (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)) // player
			&& !BG_InSlowBounce(&ent->client->ps)
			&& !PM_SaberInBrokenParry(ent->client->ps.saber_move)
			&& !PM_SaberInAttackPure(ent->client->ps.saber_move)
			&& !PM_SaberInAttack(ent->client->ps.saber_move)
			&& !PM_SaberInTransitionAny(ent->client->ps.saber_move)
			&& !PM_InKnockDown(&ent->client->ps)
			&& ent->client->ps.saberLockTime < level.time
			&& ent->client->ps.saberBlockingTime < level.time
			&& ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			if (!(ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
			{
				WP_SaberFatigueRegenerate(1);
			}
		}
		else if (ent->client->ps.saberFatigueChainCount > MISHAPLEVEL_NONE
			&& ent->r.svFlags & SVF_BOT //npc
			&& !BG_InSlowBounce(&ent->client->ps)
			&& !PM_SaberInBrokenParry(ent->client->ps.saber_move)
			&& !PM_SaberInAttackPure(ent->client->ps.saber_move)
			&& !PM_SaberInAttack(ent->client->ps.saber_move)
			&& !PM_SaberInTransitionAny(ent->client->ps.saber_move)
			&& !PM_InKnockDown(&ent->client->ps)
			&& ent->client->ps.saberLockTime < level.time
			&& ent->client->ps.saberBlockingTime < level.time
			&& ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			WP_SaberFatigueRegenerate(1);
		}

		if (ent->r.svFlags & SVF_BOT
			&& ent->client->ps.fd.blockPoints < BLOCK_POINTS_MAX)
		{
			ent->client->ps.fd.blockPoints++;
		}

		Player_CheckBurn(ent);
		Player_CheckFreeze(ent);

		if (client->pers.padawantimer >= 3 && client->pers.isapadawan == 1 && g_noPadawanNames.integer != 0)
		{
			trap->SendServerCommand(ent - g_entities, va("cp \"The Name ^1padawan ^7is not allowed here!\nPlease change it in ^3%d seconds,\nor your name will be changed.\n\"", client->pers.padawantimer));
			client->pers.padawantimer--;
		}

		if ((client->pers.padawantimer == 2 || client->pers.padawantimer == 1) && client->pers.isapadawan == 1 && g_noPadawanNames.integer != 0)
		{
			client->pers.isapadawan = 0;
			G_Rename_Player(&g_entities[clientNum], PickName());
			trap->SendServerCommand(ent - g_entities, va("cp \"^1Padawan names are not allowed, you have been ^1forcefully renamed.\n\""));
		}
	}
}

/*
====================
ClientIntermissionThink
====================
*/
static void ClientIntermissionThink(gclient_t* client)
{
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;
	if (client->buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE) & (client->oldbuttons ^ client->buttons))
	{
		// this used to be an ^1 but once a player says ready, it should stick
		client->readyToExit = qtrue;
	}
}

static void G_VehicleAttachDroidUnit(gentity_t* vehEnt)
{
	if (vehEnt && vehEnt->m_pVehicle && vehEnt->m_pVehicle->m_pDroidUnit != NULL)
	{
		gentity_t* droidEnt = (gentity_t*)vehEnt->m_pVehicle->m_pDroidUnit;
		mdxaBone_t boltMatrix;
		vec3_t fwd;

		trap->G2API_GetBoltMatrix(vehEnt->ghoul2, 0, vehEnt->m_pVehicle->m_iDroidUnitTag, &boltMatrix,
			vehEnt->r.currentAngles, vehEnt->r.currentOrigin, level.time,
			NULL, vehEnt->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, droidEnt->r.currentOrigin);
		BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, fwd);
		vectoangles(fwd, droidEnt->r.currentAngles);

		if (droidEnt->client)
		{
			VectorCopy(droidEnt->r.currentAngles, droidEnt->client->ps.viewangles);
			VectorCopy(droidEnt->r.currentOrigin, droidEnt->client->ps.origin);
		}

		G_SetOrigin(droidEnt, droidEnt->r.currentOrigin);
		trap->LinkEntity((sharedEntity_t*)droidEnt);

		if (droidEnt->NPC)
		{
			NPC_SetAnim(droidEnt, SETANIM_BOTH, BOTH_STAND2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
}

//called gameside only from pmove code (convenience)
extern qboolean BG_SabersOff(const playerState_t* ps);
extern qboolean PM_ReloadAnim(int anim);

void G_CheapWeaponFire(const int entNum, const int ev)
{
	gentity_t* ent = &g_entities[entNum];

	if (!ent->inuse || !ent->client)
	{
		return;
	}

	switch (ev)
	{
	case EV_FIRE_WEAPON:
		if (PM_ReloadAnim(ent->client->ps.torsoAnim))
		{
			return;
		}

		if (ent && ent->client && ent->client->frozenTime > level.time)
		{
			return; //this entity is mind-tricking the current client, so don't render it
		}

		if (ent->m_pVehicle && ent->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER &&
			ent->client && ent->client->ps.m_iVehicleNum)
		{
			//a speeder with a pilot
			const gentity_t* rider = &g_entities[ent->client->ps.m_iVehicleNum - 1];
			if (rider->inuse && rider->client)
			{
				//pilot is valid...
				if (rider->client->ps.weapon != WP_MELEE &&
					(rider->client->ps.weapon != WP_SABER || !BG_SabersOff(&rider->client->ps)))
				{
					//can only attack on speeder when using melee or when saber is holstered
					break;
				}
			}
		}

		FireWeapon(ent, qfalse);
		ent->client->dangerTime = level.time;
		ent->client->ps.eFlags &= ~EF_INVULNERABLE;
		ent->client->invulnerableTimer = 0;
		break;
	case EV_ALTFIRE:
		if (PM_ReloadAnim(ent->client->ps.torsoAnim))
		{
			return;
		}

		if (ent && ent->client && ent->client->frozenTime > level.time)
		{
			return; //this entity is mind-tricking the current client, so don't render it
		}
		FireWeapon(ent, qtrue);
		ent->client->dangerTime = level.time;
		ent->client->ps.eFlags &= ~EF_INVULNERABLE;
		ent->client->invulnerableTimer = 0;
		break;
	default:;
	}
}

/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/

static void ClientEvents(gentity_t* ent, int old_event_sequence)
{
	int damage;

	const gclient_t* Client = ent->client;

	if (old_event_sequence < Client->ps.eventSequence - MAX_PS_EVENTS)
	{
		old_event_sequence = Client->ps.eventSequence - MAX_PS_EVENTS;
	}
	for (int i = old_event_sequence; i < Client->ps.eventSequence; i++)
	{
		vec3_t dir;
		const int event = Client->ps.events[i & MAX_PS_EVENTS - 1];

		switch (event)
		{
		case EV_FALL:
		{
			const int delta = Client->ps.eventParms[i & MAX_PS_EVENTS - 1];
			qboolean knockDownage = qfalse;

			if (Client && Client->ps.fallingToDeath)
			{
				break;
			}

			if (ent->s.eType != ET_PLAYER)
			{
				break; // not in the player model
			}

			if (dmflags.integer & DF_NO_FALLING)
			{
				break;
			}

			if (Client && PM_InKnockDownOnly(Client->ps.legsAnim))
			{
				if (delta <= 14)
				{
					break;
				}
				knockDownage = qtrue;
			}
			else
			{
				if (delta <= 44)
				{
					break;
				}
			}

			if (knockDownage)
			{
				damage = delta * 1;
				//you suffer for falling unprepared. A lot. Makes throws and things useful, and more realistic I suppose.
			}
			else
			{
				if (level.gametype == GT_SIEGE &&
					delta > 60)
				{
					//longer falls hurt more
					damage = delta * 1; //good enough for now, I guess
				}
				else
				{
					//damage = delta * 0.16; //good enough for now, I guess
					damage = delta * 1; //good enough for now, I guess
				}
			}

			VectorSet(dir, 0, 0, 1);

			ent->pain_debounce_time = level.time + 200; // no normal pain sound
			G_Damage(ent, NULL, NULL, NULL, NULL, damage, DAMAGE_NO_ARMOR, MOD_FALLING);

			if (ent->health < 1)
			{
				G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/player/fallsplat.wav"));
			}
		}
		break;
		case EV_ROLL:
		{
			const int delta = Client->ps.eventParms[i & MAX_PS_EVENTS - 1];
			qboolean knock_downage = qfalse;

			if (Client && Client->ps.fallingToDeath)
			{
				break;
			}

			if (ent->s.eType != ET_PLAYER)
			{
				break; // not in the player model
			}

			if (dmflags.integer & DF_NO_FALLING)
			{
				break;
			}

			if (PM_InKnockDownOnly(Client->ps.legsAnim))
			{
				if (delta <= 14)
				{
					break;
				}
				knock_downage = qtrue;
			}
			else
			{
				if (delta <= 44)
				{
					break;
				}
			}

			if (knock_downage)
			{
				damage = delta * 1;
				//you suffer for falling unprepared. A lot. Makes throws and things useful, and more realistic I suppose.
			}
			else
			{
				if (level.gametype == GT_SIEGE &&
					delta > 60)
				{
					//longer falls hurt more
					damage = delta * 1; //good enough for now, I guess
				}
				else
				{
					damage = delta * 0.16; //good enough for now, I guess
				}
			}

			VectorSet(dir, 0, 0, 1);

			ent->pain_debounce_time = level.time + 200; // no normal pain sound
			G_Damage(ent, NULL, NULL, NULL, NULL, damage, DAMAGE_NO_ARMOR, MOD_FALLING);

			if (ent->health < 1)
			{
				G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/player/fallsplat.wav"));
			}
		}
		break;
		case EV_FIRE_WEAPON:
			if (PM_ReloadAnim(Client->ps.torsoAnim))
			{
				return;
			}

			if (ent && ent->client && ent->client->frozenTime > level.time)
			{
				return; //this entity is mind-tricking the current client, so don't render it
			}
			FireWeapon(ent, qfalse);
			ent->client->dangerTime = level.time;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;

		case EV_ALTFIRE:
			if (PM_ReloadAnim(Client->ps.torsoAnim))
			{
				return;
			}

			if (ent && ent->client && ent->client->frozenTime > level.time)
			{
				return; //this entity is mind-tricking the current client, so don't render it
			}
			FireWeapon(ent, qtrue);
			ent->client->dangerTime = level.time;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;

		case EV_SABER_ATTACK:
			ent->client->dangerTime = level.time;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;

			//rww - Note that these must be in the same order (ITEM#-wise) as they are in holdable_t
		case EV_USE_ITEM1: //seeker droid        HI_SEEKER
			ItemUse_Seeker(ent);
			break;
		case EV_USE_ITEM2: //shield              HI_SHIELD
			ItemUse_Shield(ent);
			break;
		case EV_USE_ITEM3: //medpack             HI_MEDPAC
			ItemUse_MedPack(ent);
			break;
		case EV_USE_ITEM4: //big medpack         HI_MEDPAC_BIG
			ItemUse_MedPack_Big(ent);
			break;
		case EV_USE_ITEM5: //binoculars          HI_BINOCULARS
			ItemUse_Binoculars(ent);
			break;
		case EV_USE_ITEM6: //sentry gun          HI_SENTRY_GUN
			ItemUse_Sentry(ent);
			break;
		case EV_USE_ITEM7: //jetpack             HI_JETPACK
			ItemUse_Jetpack(ent);
			break;
		case EV_USE_ITEM8: //health disp         HI_HEALTHDISP
			ItemUse_UseDisp(ent, HI_HEALTHDISP);
			break;
		case EV_USE_ITEM9: //ammo disp           HI_AMMODISP
			ItemUse_UseDisp(ent, HI_AMMODISP);
			break;
		case EV_USE_ITEM10: //eweb               HI_EWEB
			ItemUse_UseEWeb(ent);
			break;
		case EV_USE_ITEM11: //cloak              HI_CLOAK
			ItemUse_UseCloak(ent);
			break;
		case EV_USE_ITEM12: //flamethrower       HI_FLAMETHROWER
			ItemUse_FlameThrower(ent);
			break;
		case EV_USE_ITEM13: //swoop              HI_SWOOP
			ItemUse_Swoop(ent);
			break;
		case EV_USE_ITEM14: //Decca              HI_DROIDEKA
			ItemUse_Decca(ent);
			break;
		case EV_USE_ITEM15: //sphereshield       HI_SPHERESHIELD
			ItemUse_UseSphereshield(ent);
			break;
		case EV_USE_ITEM16: //Grapple hook       HI_GRAPPLE
			//
			break;
		default:
			break;
		}
	}
}

void G_ThrownDeathAnimForDeathAnim(gentity_t* hit_ent, vec3_t impactPoint)
{
	//racc - sets an alternate "being thrown" death animation based on current death animation.
	int anim = -1;
	if (!hit_ent || !hit_ent->client)
	{
		return;
	}
	switch (hit_ent->client->ps.legsAnim)
	{
	case BOTH_DEATH9: //fall to knees, fall over
	case BOTH_DEATH10: //fall to knees, fall over
	case BOTH_DEATH11: //fall to knees, fall over
	case BOTH_DEATH13: //stumble back, fall over
	case BOTH_DEATH17: //jerky fall to knees, fall over
	case BOTH_DEATH18: //grab gut, fall to knees, fall over
	case BOTH_DEATH19: //grab gut, fall to knees, fall over
	case BOTH_DEATH20: //grab shoulder, fall forward
	case BOTH_DEATH21: //grab shoulder, fall forward
	case BOTH_DEATH3: //knee collapse, twist & fall forward
	case BOTH_DEATH7: //knee collapse, twist & fall forward
	{
		vec3_t dir2Impact, fwdAngles, facing;
		VectorSubtract(impactPoint, hit_ent->r.currentOrigin, dir2Impact);
		dir2Impact[2] = 0;
		VectorNormalize(dir2Impact);
		VectorSet(fwdAngles, 0, hit_ent->client->ps.viewangles[YAW], 0);
		AngleVectors(fwdAngles, facing, NULL, NULL);
		const float dot = DotProduct(facing, dir2Impact); //-1 = hit in front, 0 = hit on side, 1 = hit in back

		if (dot > 0.5f)
		{
			//kicked in chest, fly backward
			switch (Q_irand(0, 4))
			{
				//FIXME: don't start at beginning of anim?
			case 0:
				anim = BOTH_DEATH1; //thrown backwards
				break;
			case 1:
				anim = BOTH_DEATH2; //fall backwards
				break;
			case 2:
				anim = BOTH_DEATH15; //roll over backwards
				break;
			case 3:
				anim = BOTH_DEATH22; //fast fall back
				break;
			case 4:
				anim = BOTH_DEATH23; //fast fall back
				break;
			default:;
			}
		}
		else if (dot < -0.5f)
		{
			//kicked in back, fly forward
			switch (Q_irand(0, 5))
			{
				//FIXME: don't start at beginning of anim?
			case 0:
				anim = BOTH_DEATH14;
				break;
			case 1:
				anim = BOTH_DEATH24;
				break;
			case 2:
				anim = BOTH_DEATH25;
				break;
			case 3:
				anim = BOTH_DEATH4; //thrown forwards
				break;
			case 4:
				anim = BOTH_DEATH5; //thrown forwards
				break;
			case 5:
				anim = BOTH_DEATH16; //thrown forwards
				break;
			default:;
			}
		}
		else
		{
			//hit on side, spin
			switch (Q_irand(0, 2))
			{
				//FIXME: don't start at beginning of anim?
			case 0:
				anim = BOTH_DEATH12;
				break;
			case 1:
				anim = BOTH_DEATH14;
				break;
			case 2:
				anim = BOTH_DEATH15;
				break;
			case 3:
				anim = BOTH_DEATH6;
				break;
			case 4:
				anim = BOTH_DEATH8;
				break;
			default:;
			}
		}
	}
	break;
	default:;
	}
	if (anim != -1)
	{
		NPC_SetAnim(hit_ent, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
}

/*
==============
SendPendingPredictableEvents
==============
*/
static void SendPendingPredictableEvents(playerState_t* ps)
{
	// if there are still events pending
	if (ps->entityEventSequence < ps->eventSequence)
	{
		// create a temporary entity for this event which is sent to everyone
		// except the client who generated the event
		const int seq = ps->entityEventSequence & MAX_PS_EVENTS - 1;
		const int event = ps->events[seq] | (ps->entityEventSequence & 3) << 8;
		// set external event to zero before calling BG_PlayerStateToEntityState
		const int extEvent = ps->externalEvent;
		ps->externalEvent = 0;
		// create temporary entity for event
		gentity_t* t = G_TempEntity(ps->origin, event);
		const int number = t->s.number;
		BG_PlayerStateToEntityState(ps, &t->s, qtrue);
		t->s.number = number;
		t->s.eType = ET_EVENTS + event;
		t->s.eFlags |= EF_PLAYER_EVENT;
		t->s.otherentity_num = ps->clientNum;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_NOTSINGLECLIENT;
		t->r.singleClient = ps->clientNum;
		// set back external event
		ps->externalEvent = extEvent;
	}
}

static const float maxJediMasterDistance = 2500.0f * 2500.0f; // x^2, optimisation
static const float maxJediMasterFOV = 100.0f;
static const float maxForceSightDistance = Square(1500.0f) * 1500.0f; // x^2, optimisation
static const float maxForceSightFOV = 100.0f;

static void G_UpdateClientBroadcasts(gentity_t* self)
{
	int i;
	gentity_t* other;

	// we are always sent to ourselves
	// we are always sent to other clients if we are in their PVS
	// if we are not in their PVS, we must set the broadcastClients bit field
	// if we do not wish to be sent to any particular entity, we must set the broadcastClients bit field and the
	//	SVF_BROADCASTCLIENTS bit flag
	self->r.broadcastClients[0] = 0u;
	self->r.broadcastClients[1] = 0u;

	for (i = 0, other = g_entities; i < MAX_CLIENTS; i++, other++)
	{
		qboolean send = qfalse;
		vec3_t angles;

		if (!other->inuse || other->client->pers.connected != CON_CONNECTED)
		{
			// no need to compute visibility for non-connected clients
			continue;
		}

		if (other == self)
		{
			// we are always sent to ourselves anyway, this is purely an optimisation
			continue;
		}

		VectorSubtract(self->client->ps.origin, other->client->ps.origin, angles);
		const float dist = VectorLengthSquared(angles);
		vectoangles(angles, angles);

		// broadcast jedi master to everyone if we are in distance/field of view
		if (level.gametype == GT_JEDIMASTER && self->client->ps.isJediMaster)
		{
			if (dist < maxJediMasterDistance
				&& in_field_of_vision(other->client->ps.viewangles, maxJediMasterFOV, angles))
			{
				send = qtrue;
			}
		}

		// broadcast this client to everyone using force sight if we are in distance/field of view
		if (other->client->ps.fd.forcePowersActive & 1 << FP_SEE)
		{
			if (dist < maxForceSightDistance
				&& in_field_of_vision(other->client->ps.viewangles, maxForceSightFOV, angles))
			{
				send = qtrue;
			}
		}

		if (send)
		{
			Q_AddToBitflags(self->r.broadcastClients, i, 32);
		}
	}

	trap->LinkEntity((sharedEntity_t*)self);
}

static void G_AddPushVecToUcmd(const gentity_t* self, usercmd_t* ucmd)
{
	vec3_t forward, right, moveDir;

	if (!self->client)
	{
		return;
	}
	const float pushSpeed = VectorLengthSquared(self->client->pushVec);
	if (!pushSpeed)
	{
		//not being pushed
		return;
	}

	AngleVectors(self->client->ps.viewangles, forward, right, NULL);
	VectorScale(forward, ucmd->forwardmove / 127.0f * self->client->ps.speed, moveDir);
	VectorMA(moveDir, ucmd->rightmove / 127.0f * self->client->ps.speed, right, moveDir);
	//moveDir is now our intended move velocity

	VectorAdd(moveDir, self->client->pushVec, moveDir);
	self->client->ps.speed = VectorNormalize(moveDir);
	//moveDir is now our intended move velocity plus our push Vector

	const float fMove = 127.0 * DotProduct(forward, moveDir);
	const float rMove = 127.0 * DotProduct(right, moveDir);
	ucmd->forwardmove = floor(fMove); //If in the same dir , will be positive
	ucmd->rightmove = floor(rMove); //If in the same dir , will be positive

	if (self->client->pushVecTime < level.time)
	{
		VectorClear(self->client->pushVec);
	}
}

static qboolean G_ActionButtonPressed(const int buttons)
{
	if (buttons & BUTTON_ATTACK)
	{
		return qtrue;
	}
	if (buttons & BUTTON_USE_HOLDABLE)
	{
		return qtrue;
	}
	if (buttons & BUTTON_GESTURE)
	{
		return qtrue;
	}
	if (buttons & BUTTON_GLOAT)
	{
		return qtrue;
	}
	if (buttons & BUTTON_RESPECT)
	{
		return qtrue;
	}
	if (buttons & BUTTON_FLOURISH)
	{
		return qtrue;
	}
	if (buttons & BUTTON_USE)
	{
		return qtrue;
	}
	if (buttons & BUTTON_FORCEGRIP)
	{
		return qtrue;
	}
	if (buttons & BUTTON_ALT_ATTACK)
	{
		return qtrue;
	}
	if (buttons & BUTTON_FORCEPOWER)
	{
		return qtrue;
	}
	if (buttons & BUTTON_FORCE_LIGHTNING)
	{
		return qtrue;
	}
	if (buttons & BUTTON_FORCE_DRAIN)
	{
		return qtrue;
	}
	if (buttons & BUTTON_KICK)
	{
		return qtrue;
	}
	if (buttons & BUTTON_BLOCK)
	{
		return qtrue;
	}
	if (buttons & BUTTON_BOTUSE)
	{
		return qtrue;
	}
	if (buttons & BUTTON_DASH)
	{
		return qtrue;
	}

	return qfalse;
}

static void G_CheckClientIdleSabers(gentity_t* ent, usercmd_t* ucmd)
{
	vec3_t viewChange;

	if (!ent || !ent->client || ent->health <= 0 || ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

	if (ent && ent->client->ps.torsoAnim == BOTH_WIND)
	{
		return;
	}

	int buttons = ucmd->buttons;

	if (ent->r.svFlags & SVF_BOT)
	{
		//they press use all the time..
		buttons &= ~BUTTON_BOTUSE;
	}
	const qboolean action_pressed = G_ActionButtonPressed(buttons);

	VectorSubtract(ent->client->ps.viewangles, ent->client->idleViewAngles, viewChange);

	if (!VectorCompare(vec3_origin, ent->client->ps.velocity)
		|| action_pressed || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove
		|| !PM_StandingAnim(ent->client->ps.legsAnim)
		|| ent->health + ent->client->ps.stats[STAT_ARMOR] != ent->client->idleHealth
		|| VectorLength(viewChange) > 10
		|| ent->client->ps.legsTimer > 0
		|| ent->client->ps.torsoTimer > 0
		|| ent->client->ps.weaponTime > 0
		|| ent->client->ps.weaponstate == WEAPON_CHARGING
		|| ent->client->ps.weaponstate == WEAPON_CHARGING_ALT
		|| ent->client->ps.zoomMode
		|| ent->client->ps.weaponstate != WEAPON_READY && ent->client->ps.weapon != WP_SABER
		|| ent->client->ps.forceHandExtend != HANDEXTEND_NONE
		|| ent->client->ps.saberBlocked != BLOCKED_NONE
		|| ent->client->ps.saberBlocking >= level.time
		|| ent->client->ps.weapon == WP_MELEE
		|| ent->client->ps.weapon != ent->client->pers.cmd.weapon && ent->s.eType != ET_NPC)
	{
		//FIXME: also check for turning?

		if (!VectorCompare(vec3_origin, ent->client->ps.velocity)
			|| action_pressed || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove
			|| ent->health + ent->client->ps.stats[STAT_ARMOR] != ent->client->idleHealth
			|| ent->client->ps.zoomMode
			|| ent->client->ps.weaponstate != WEAPON_READY && ent->client->ps.weapon != WP_SABER
			|| ent->client->ps.weaponTime > 0 && ent->client->ps.weapon == WP_SABER
			|| ent->client->ps.weaponstate == WEAPON_CHARGING
			|| ent->client->ps.weaponstate == WEAPON_CHARGING_ALT
			|| ent->client->ps.forceHandExtend != HANDEXTEND_NONE
			|| ent->client->ps.saberBlocked != BLOCKED_NONE
			|| ent->client->ps.saberBlocking >= level.time
			//|| ent->client->ps.weapon == WP_MELEE
			|| ent->client->ps.weapon != ent->client->pers.cmd.weapon && ent->s.eType != ET_NPC)
		{
			qboolean broke_out;
			//if in an idle, break out
			switch (ent->client->ps.legsAnim)
			{
			case BOTH_STAND1IDLE1:
			case BOTH_STAND2IDLE1:
			case BOTH_STAND2IDLE2:
			case BOTH_STAND3IDLE1:
			case BOTH_STAND5IDLE1:
			case BOTH_MENUIDLE1:
				ent->client->ps.legsTimer = 0;
				broke_out = qtrue;
				break;
			default:;
			}
			switch (ent->client->ps.torsoAnim)
			{
			case BOTH_STAND1IDLE1:
			case BOTH_STAND2IDLE1:
			case BOTH_STAND2IDLE2:
			case BOTH_STAND3IDLE1:
			case BOTH_STAND5IDLE1:
			case BOTH_MENUIDLE1:
				ent->client->ps.torsoTimer = 0;
				break;
			default:;
			}
		}
		//
		ent->client->idleHealth = ent->health + ent->client->ps.stats[STAT_ARMOR];
		VectorCopy(ent->client->ps.viewangles, ent->client->idleViewAngles);
		if (ent->client->idleTime < level.time)
		{
			ent->client->idleTime = level.time;
		}
	}
	else if (level.time - ent->client->idleTime > 2000)
	{
		//been idle for 5 seconds
		int idleAnim = -1;

		switch (ent->client->ps.legsAnim)
		{
		case BOTH_STAND1:
		case BOTH_STAND6:
			idleAnim = BOTH_STAND1IDLE1;
			break;
		case BOTH_STAND2: //single-saber, medium style
			idleAnim = BOTH_STAND2IDLE2;
			break;
		case BOTH_SABERFAST_STANCE: //single-saber, fast style
		case BOTH_SABERSLOW_STANCE: //single-saber, strong style
		case BOTH_SABERSTAFF_STANCE: //saber staff style
		case BOTH_SABERDUAL_STANCE: //dual saber style
		case BOTH_SABERDUAL_STANCE_GRIEVOUS: //dual saber style
		case BOTH_SABERTAVION_STANCE: //tavion saberstyle
		case BOTH_SABERDESANN_STANCE: //desann saber style
		case BOTH_SABERSTANCE_STANCE: //similar to stand2
		case BOTH_SABERYODA_STANCE: //YODA saber style
		case BOTH_SABERBACKHAND_STANCE: //BACKHAND saber style
		case BOTH_SABEROBI_STANCE: //obiwan
		case BOTH_SABEREADY_STANCE: //ready
		case BOTH_SABER_REY_STANCE: //rey
			idleAnim = Q_irand(BOTH_STAND2IDLE1, BOTH_STAND2IDLE2);
			break;
		case BOTH_STAND3:
			idleAnim = BOTH_STAND3IDLE1;
			break;
		case BOTH_STAND5:
		case BOTH_SABERSTANCE_STANCE_ALT: //alt idle
		case BOTH_SABERSTAFF_STANCE_IDLE: //saber staff style
		case BOTH_SABERSTAFF_STANCE_ALT: //alt staff
		case BOTH_SABERDUAL_STANCE_IDLE: //dual saber style
		case BOTH_SABERDUAL_STANCE_ALT: //alt dual
			idleAnim = BOTH_STAND5IDLE1;
			break;
		case BOTH_MENUIDLE1:
			idleAnim = BOTH_MENUIDLE1;
			break;
		case TORSO_WEAPONREADY1:
			idleAnim = TORSO_WEAPONIDLE1;
			break;
		case TORSO_WEAPONREADY2:
			idleAnim = TORSO_WEAPONIDLE2;
			break;
		case TORSO_WEAPONREADY3:
			idleAnim = TORSO_WEAPONIDLE3;
			break;
		case TORSO_WEAPONREADY4:
			idleAnim = TORSO_WEAPONIDLE4;
			break;
		case TORSO_WEAPONREADY10:
			idleAnim = TORSO_WEAPONIDLE10;
			break;
		default:;
		}
		if (idleAnim != -1 && idleAnim > 0 && idleAnim < MAX_ANIMATIONS)
		{
			G_SetAnim(ent, ucmd, SETANIM_BOTH, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		}
	}
}

static qboolean GunisMelee(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_MELEE:
	case WP_STUN_BATON:
		return qtrue;
	default:;
	}
	return qfalse;
}

static qboolean GunisShort(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_BRYAR_PISTOL:
	case WP_BRYAR_OLD:
		return qtrue;
	default:;
	}
	return qfalse;
}

static qboolean GunisLong(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_BLASTER:
	case WP_BOWCASTER:
	case WP_REPEATER:
	case WP_DEMP2:
	case WP_FLECHETTE:
		return qtrue;
	default:;
	}
	return qfalse;
}

static qboolean GunisMassive(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_DISRUPTOR:
	case WP_CONCUSSION:
	case WP_ROCKET_LAUNCHER:
		return qtrue;
	default:;
	}
	return qfalse;
}

static qboolean GunisBomb(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_THERMAL:
	case WP_TRIP_MINE:
	case WP_DET_PACK:
		return qtrue;
	default:;
	}
	return qfalse;
}

static void G_CheckClientIdleGuns(gentity_t* ent, usercmd_t* ucmd)
{
	vec3_t viewChange;

	if (!ent || !ent->client || ent->health <= 0 || ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

	if (ent && ent->client->ps.torsoAnim == BOTH_WIND)
	{
		return;
	}

	int buttons = ucmd->buttons;

	if (ent->r.svFlags & SVF_BOT)
	{
		//they press use all the time..
		buttons &= ~BUTTON_BOTUSE;
	}
	const qboolean actionPressed = G_ActionButtonPressed(buttons);

	VectorSubtract(ent->client->ps.viewangles, ent->client->idleViewAngles, viewChange);

	if (!VectorCompare(vec3_origin, ent->client->ps.velocity)
		|| actionPressed || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove
		|| !PM_StandingAnim(ent->client->ps.legsAnim)
		|| ent->health + ent->client->ps.stats[STAT_ARMOR] != ent->client->idleHealth
		|| VectorLength(viewChange) > 10
		|| ent->client->ps.legsTimer > 0
		|| ent->client->ps.torsoTimer > 0
		|| ent->client->ps.weaponTime > 0
		|| ent->client->ps.weaponstate == WEAPON_CHARGING
		|| ent->client->ps.weaponstate == WEAPON_CHARGING_ALT
		|| ent->client->ps.zoomMode
		|| ent->client->ps.weaponstate != WEAPON_READY && ent->client->ps.weapon != WP_SABER
		|| ent->client->ps.forceHandExtend != HANDEXTEND_NONE
		|| ent->client->ps.saberBlocked != BLOCKED_NONE
		|| ent->client->ps.saberBlocking >= level.time
		|| ent->client->ps.weapon == WP_MELEE
		|| ent->client->ps.weapon != ent->client->pers.cmd.weapon && ent->s.eType != ET_NPC)
	{
		//FIXME: also check for turning?

		if (!VectorCompare(vec3_origin, ent->client->ps.velocity)
			|| actionPressed || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove
			|| ent->health + ent->client->ps.stats[STAT_ARMOR] != ent->client->idleHealth
			|| ent->client->ps.zoomMode
			|| ent->client->ps.weaponstate != WEAPON_READY && ent->client->ps.weapon != WP_SABER
			|| ent->client->ps.weaponTime > 0 && ent->client->ps.weapon == WP_SABER
			|| ent->client->ps.weaponstate == WEAPON_CHARGING
			|| ent->client->ps.weaponstate == WEAPON_CHARGING_ALT
			|| ent->client->ps.forceHandExtend != HANDEXTEND_NONE
			|| ent->client->ps.saberBlocked != BLOCKED_NONE
			|| ent->client->ps.saberBlocking >= level.time
			|| ent->client->ps.weapon != ent->client->pers.cmd.weapon && ent->s.eType != ET_NPC)
		{
			qboolean broke_out;
			//if in an idle, break out
			switch (ent->client->ps.legsAnim)
			{
			case BOTH_STAND1IDLE1:
			case BOTH_STAND2IDLE1:
			case BOTH_STAND2IDLE2:
			case BOTH_STAND3IDLE1:
			case BOTH_STAND5IDLE1:
			case BOTH_STAND9IDLE1:
			case BOTH_MENUIDLE1:
			case TORSO_WEAPONIDLE2:
			case TORSO_WEAPONIDLE3:
			case TORSO_WEAPONIDLE4:
				ent->client->ps.legsTimer = 0;
				broke_out = qtrue;
				break;
			default:;
			}
			switch (ent->client->ps.torsoAnim)
			{
			case BOTH_STAND1IDLE1:
			case BOTH_STAND2IDLE1:
			case BOTH_STAND2IDLE2:
			case BOTH_STAND3IDLE1:
			case BOTH_STAND5IDLE1:
			case BOTH_STAND9IDLE1:
			case BOTH_MENUIDLE1:
			case TORSO_WEAPONIDLE2:
			case TORSO_WEAPONIDLE3:
			case TORSO_WEAPONIDLE4:
				ent->client->ps.torsoTimer = 0;
				//brokeOut = qtrue;
				break;
			default:;
			}
		}
		//
		ent->client->idleHealth = ent->health + ent->client->ps.stats[STAT_ARMOR];
		VectorCopy(ent->client->ps.viewangles, ent->client->idleViewAngles);
		if (ent->client->idleTime < level.time)
		{
			ent->client->idleTime = level.time;
		}
	}
	else if (GunisMelee(ent) && level.time - ent->client->idleTime > 5000)
	{
		//been idle for 2 seconds

		const int idleAnim = BOTH_STAND1IDLE1;

		if (idleAnim < MAX_ANIMATIONS)
		{
			G_SetAnim(ent, ucmd, SETANIM_BOTH, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		}
	}
	else if (GunisShort(ent) && level.time - ent->client->idleTime > 5000)
	{
		//been idle for 2 seconds

		const int idleAnim = BOTH_STAND9IDLE1;

		if (idleAnim < MAX_ANIMATIONS)
		{
			G_SetAnim(ent, ucmd, SETANIM_TORSO, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		}
	}
	else if (GunisLong(ent) && level.time - ent->client->idleTime > 5000)
	{
		//been idle for 2 seconds

		const int idleAnim = BOTH_STAND3IDLE1;

		if (idleAnim < MAX_ANIMATIONS)
		{
			G_SetAnim(ent, ucmd, SETANIM_TORSO, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		}
	}
	else if (GunisMassive(ent) && level.time - ent->client->idleTime > 5000)
	{
		//been idle for 2 seconds

		const int idleAnim = BOTH_STAND3IDLE1;

		if (idleAnim < MAX_ANIMATIONS)
		{
			G_SetAnim(ent, ucmd, SETANIM_TORSO, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		}
	}
	else if (GunisBomb(ent) && level.time - ent->client->idleTime > 3000)
	{
		//been idle for 2 seconds

		const int idleAnim = BOTH_STAND9IDLE1;

		if (idleAnim < MAX_ANIMATIONS)
		{
			G_SetAnim(ent, ucmd, SETANIM_BOTH, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		}
	}
	else
	{
		if (level.time - ent->client->idleTime > 5000)
		{
			//been idle for 2 seconds

			const int idleAnim = BOTH_STAND1IDLE1;

			if (idleAnim < MAX_ANIMATIONS)
			{
				G_SetAnim(ent, ucmd, SETANIM_BOTH, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
			}
		}
	}
}

static void NPC_Accelerate(const gentity_t* ent, const qboolean fullWalkAcc, const qboolean fullRunAcc)
{
	if (!ent->client || !ent->NPC)
	{
		return;
	}

	if (!ent->NPC->stats.acceleration)
	{
		//No acceleration means just start and stop
		ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
	}
	else if (ent->NPC->desiredSpeed <= ent->NPC->stats.walkSpeed)
	{
		//Only accelerate if at walkSpeeds
		if (ent->NPC->desiredSpeed > ent->NPC->currentSpeed + ent->NPC->stats.acceleration)
		{
			ent->NPC->currentSpeed += ent->NPC->stats.acceleration;
		}
		else if (ent->NPC->desiredSpeed > ent->NPC->currentSpeed)
		{
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
		else if (fullWalkAcc && ent->NPC->desiredSpeed < ent->NPC->currentSpeed - ent->NPC->stats.acceleration)
		{
			//decelerate even when walking
			ent->NPC->currentSpeed -= ent->NPC->stats.acceleration;
		}
		else if (ent->NPC->desiredSpeed < ent->NPC->currentSpeed)
		{
			//stop on a dime
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
	}
	else
	{
		//Only decelerate if at runSpeeds
		if (fullRunAcc && ent->NPC->desiredSpeed > ent->NPC->currentSpeed + ent->NPC->stats.acceleration)
		{
			//Accelerate to runspeed
			ent->NPC->currentSpeed += ent->NPC->stats.acceleration;
		}
		else if (ent->NPC->desiredSpeed > ent->NPC->currentSpeed)
		{
			//accelerate instantly
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
		else if (fullRunAcc && ent->NPC->desiredSpeed < ent->NPC->currentSpeed - ent->NPC->stats.acceleration)
		{
			ent->NPC->currentSpeed -= ent->NPC->stats.acceleration;
		}
		else if (ent->NPC->desiredSpeed < ent->NPC->currentSpeed)
		{
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
	}
}

/*
-------------------------
NPC_GetWalkSpeed
-------------------------
*/

static int NPC_GetWalkSpeed(const gentity_t* ent)
{
	int walk_speed;

	if (ent->client == NULL || ent->NPC == NULL)
		return 0;

	switch (ent->client->playerTeam)
	{
	case NPCTEAM_PLAYER: //To shutup compiler, will add entries later (this is stub code)
	default:
		walk_speed = ent->NPC->stats.walkSpeed;
		break;
	}

	return walk_speed;
}

/*
-------------------------
NPC_GetRunSpeed
-------------------------
*/
static int NPC_GetRunSpeed(const gentity_t* ent)
{
	int run_speed;

	if (ent->client == NULL || ent->NPC == NULL)
		return 0;

	// team no longer indicates species/race.  Use NPC_class to adjust speed for specific npc types
	switch (ent->client->NPC_class)
	{
	case CLASS_PROBE: // droid cases here to shut-up compiler
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_PROTOCOL:
	case CLASS_ATST: // hmm, not really your average droid
	case CLASS_MOUSE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
		run_speed = ent->NPC->stats.runSpeed;
		break;

	default:
		run_speed = ent->NPC->stats.runSpeed * 1.3f; //rww - seems to slow in MP for some reason.
		break;
	}

	return run_speed;
}

//Seems like a slightly less than ideal method for this, could it be done on the client?
extern qboolean FlyingCreature(const gentity_t* ent);

static void G_CheckMovingLoopingSounds(gentity_t* ent, const usercmd_t* ucmd)
{
	if (ent->client)
	{
		if (ent->NPC && !VectorCompare(vec3_origin, ent->client->ps.moveDir) //moving using moveDir
			|| ucmd->forwardmove || ucmd->rightmove //moving using ucmds
			|| ucmd->upmove && FlyingCreature(ent) //flier using ucmds to move
			|| FlyingCreature(ent) && !VectorCompare(vec3_origin, ent->client->ps.velocity) && ent->health > 0)
			//flier using velocity to move
		{
			switch (ent->client->NPC_class)
			{
			case CLASS_R2D2:
				ent->s.loopSound = G_SoundIndex("sound/chars/r2d2/misc/r2_move_lp.wav");
				break;
			case CLASS_R5D2:
				ent->s.loopSound = G_SoundIndex("sound/chars/r2d2/misc/r2_move_lp2.wav");
				break;
			case CLASS_MARK2:
				ent->s.loopSound = G_SoundIndex("sound/chars/mark2/misc/mark2_move_lp");
				break;
			case CLASS_MOUSE:
				ent->s.loopSound = G_SoundIndex("sound/chars/mouse/misc/mouse_lp");
				break;
			case CLASS_PROBE:
				ent->s.loopSound = G_SoundIndex("sound/chars/probe/misc/probedroidloop");
			default:
				break;
			}
		}
		else
		{
			//not moving under your own control, stop loopSound
			if (ent->client->NPC_class == CLASS_R2D2 || ent->client->NPC_class == CLASS_R5D2
				|| ent->client->NPC_class == CLASS_MARK2 || ent->client->NPC_class == CLASS_MOUSE
				|| ent->client->NPC_class == CLASS_PROBE)
			{
				ent->s.loopSound = 0;
			}
		}
	}
}

static void G_HeldByMonster(gentity_t* ent, usercmd_t* ucmd)
{
	if (ent
		&& ent->client
		&& ent->client->ps.hasLookTarget)
		//NOTE: lookTarget is an entity number, so this presumes that client 0 is NOT a Rancor...
	{
		gentity_t* monster = &g_entities[ent->client->ps.lookTarget];
		if (monster && monster->client)
		{
			//take the monster's waypoint as your own
			ent->waypoint = monster->waypoint;
			if (monster->s.NPC_class == CLASS_RANCOR)
			{
				//only possibility right now, may add Wampa and Sand Creature later
				BG_AttachToRancor(monster->ghoul2, //ghoul2 info
					monster->r.currentAngles[YAW],
					monster->r.currentOrigin,
					level.time,
					NULL,
					monster->modelScale,
					monster->client->ps.eFlags2 & EF2_GENERIC_NPC_FLAG,
					ent->client->ps.origin,
					ent->client->ps.viewangles,
					NULL);
			}
			else if (monster->s.NPC_class == CLASS_SAND_CREATURE)
			{
				BG_AttachToSandCreature(monster->ghoul2, //ghoul2 info
					monster->r.currentAngles[YAW],
					monster->r.currentOrigin,
					level.time,
					NULL,
					monster->modelScale,
					ent->client->ps.origin,
					ent->client->ps.viewangles,
					NULL);
			}
			VectorClear(ent->client->ps.velocity);
			G_SetOrigin(ent, ent->client->ps.origin);
			SetClientViewAngle(ent, ent->client->ps.viewangles);
			G_SetAngles(ent, ent->client->ps.viewangles);
			trap->LinkEntity((sharedEntity_t*)ent); //redundant?
		}
	}
	// don't allow movement, weapon switching, and most kinds of button presses
	ucmd->forwardmove = 0;
	ucmd->rightmove = 0;
	ucmd->upmove = 0;
}

typedef enum tauntTypes_e
{
	TAUNT_TAUNT = 0,
	TAUNT_BOW,
	TAUNT_MEDITATE,
	TAUNT_FLOURISH,
	TAUNT_GLOAT,
	TAUNT_SURRENDER,
	TAUNT_RELOAD
} tauntTypes_t;

qboolean IsHoldingReloadableGun(const gentity_t* ent);
extern saberInfo_t* BG_MySaber(int clientNum, int saberNum);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean PM_LungeAnim(int anim);
extern qboolean PM_RollingAnim(int anim);
extern qboolean Block_Button_Held(const gentity_t* defender);
void WP_ReloadGun(gentity_t* ent);
void CancelReload(gentity_t* ent);
extern qboolean PM_RestAnim(int anim);

void G_SetTauntAnim(gentity_t* ent, int taunt)
{
	const saberInfo_t* saber1 = BG_MySaber(ent->clientNum, 0);
	const qboolean is_holding_block_button = ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;//Normal Blocking

	// dead clients dont get to spam taunt
	if (ent->client->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (ent->painDebounceTime > level.time)
	{
		return;
	}

	if (ent->client->ps.emplacedIndex)
	{
		//on an emplaced gun
		return;
	}

	if (BG_IsAlreadyinTauntAnim(ent->client->ps.torsoAnim))
	{
		return;
	}

	if (taunt != TAUNT_RELOAD)
	{
		if (PM_CrouchAnim(ent->client->ps.legsAnim) ||
			PM_CrouchAnim(ent->client->ps.torsoAnim) ||
			PM_RestAnim(ent->client->ps.legsAnim) ||
			PM_RestAnim(ent->client->ps.torsoAnim))
		{
			return;
		}
	}

	if (is_holding_block_button || Block_Button_Held(ent))
	{
		return;
	}

	if (manual_saberblocking(ent))
	{
		return;
	}

	if (ent->client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING)
	{
		return;
	}

	if (ent->client->ps.m_iVehicleNum)
	{
		//in a vehicle like at-st
		const gentity_t* veh = &g_entities[ent->client->ps.m_iVehicleNum];

		if (veh->m_pVehicle && veh->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)
			return;

		if (veh->m_pVehicle && veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
			return;

		if (taunt == TAUNT_FLOURISH || taunt == TAUNT_GLOAT)
		{
			taunt = TAUNT_TAUNT;
		}
		if (taunt == TAUNT_MEDITATE || taunt == TAUNT_BOW || taunt == TAUNT_SURRENDER || taunt == TAUNT_RELOAD)
		{
			return;
		}
	}

	// fix: rocket lock bug
	BG_ClearRocketLock(&ent->client->ps);

	if (ent->client->ps.weaponTime <= 0 && ent->client->ps.saberLockTime < level.time)
	{
		int anim = -1;

		switch (taunt)
		{
		case TAUNT_TAUNT:
			G_AddEvent(ent, EV_TAUNT, taunt);
			if (ent->client->ps.weapon != WP_SABER) //MP
			{
				if (ent->client->pers.botclass == BCLASS_VADER || ent->client->pers.botclass == BCLASS_DESANN)
				{
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					if (ent->client->ps.weapon == WP_DISRUPTOR)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
			}
			else if (ent->client->saber[0].tauntAnim != -1)
			{
				anim = ent->client->saber[0].tauntAnim;
			}
			else if (ent->client->saber[1].model[0]
				&& ent->client->saber[1].tauntAnim != -1)
			{
				anim = ent->client->saber[1].tauntAnim;
			}
			else
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					switch (ent->client->ps.fd.saberAnimLevel)
					{
					case SS_FAST:
					case SS_TAVION:
						if (ent->client->ps.saberHolstered == 1
							&& ent->client->saber[1].model[0])
						{
							//turn off second saber
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOff);
						}
						else if (ent->client->ps.saberHolstered == 0)
						{
							//turn off first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOff);
						}
						ent->client->ps.saberHolstered = 2;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_GESTURE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_MEDIUM:
						if (saber1 && saber1->type == SABER_OBIWAN) //saber kylo
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_OBI, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_STRONG:
					case SS_DESANN:
						if (saber1 && saber1->type == SABER_VADER || ent->client->pers.botclass == BCLASS_VADER) //saber kylo
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_DUAL:
						if (ent->client->ps.saberHolstered == 1 && ent->client->saber[1].model[0])
						{
							//turn on second saber
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOn);
						}
						else if (ent->client->ps.saberHolstered == 2)
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						if (ent->client->saber[0].type == SABER_GRIE || ent->client->saber[0].type == SABER_GRIE4)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_DUAL_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_STAFF:
						if (ent->client->ps.saberHolstered > 0)
						{
							//turn on all blades
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					default:;
					}
				}
				else
				{
					switch (ent->client->ps.fd.saberAnimLevel)
					{
					case SS_FAST:
					case SS_TAVION:
						if (ent->client->ps.saberHolstered == 1
							&& ent->client->saber[1].model[0])
						{
							//turn off second saber
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOff);
						}
						else if (ent->client->ps.saberHolstered == 0)
						{
							//turn off first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOff);
						}
						ent->client->ps.saberHolstered = 2;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_GESTURE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_MEDIUM:
						if (saber1 && saber1->type == SABER_OBIWAN) //saber kylo
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_OBI,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_STRONG:
					case SS_DESANN:
						if (saber1 && saber1->type == SABER_VADER || ent->client->pers.botclass == BCLASS_VADER) //saber kylo
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_DUAL:
						if (ent->client->ps.saberHolstered == 1 && ent->client->saber[1].model[0])
						{
							//turn on second saber
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOn);
						}
						else if (ent->client->ps.saberHolstered == 2)
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						if (ent->client->saber[0].type == SABER_GRIE || ent->client->saber[0].type == SABER_GRIE4)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_DUAL_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_STAFF:
						if (ent->client->ps.saberHolstered > 0)
						{
							//turn on all blades
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					default:;
					}
				}
			}
			break;
		case TAUNT_BOW:
			if (ent->client->ps.weapon != WP_SABER) //MP
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->pers.botclass == BCLASS_VADER || ent->client->pers.botclass == BCLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_BOW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
				else
				{
					if (ent->client->pers.botclass == BCLASS_VADER || ent->client->pers.botclass == BCLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_BOW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
			}
			else if (ent->client->saber[0].bowAnim != -1)
			{
				anim = ent->client->saber[0].bowAnim;
			}
			else if (ent->client->saber[1].model[0]
				&& ent->client->saber[1].bowAnim != -1)
			{
				anim = ent->client->saber[1].bowAnim;
			}
			else
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->ps.saberHolstered == 1
						&& ent->client->saber[1].model[0])
					{
						//turn off second saber
						if (ent->client->ps.weapon == WP_SABER)
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOff);
					}
					else if (ent->client->ps.saberHolstered == 0)
					{
						//turn off first
						if (ent->client->ps.weapon == WP_SABER)
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOff);
					}
					ent->client->ps.saberHolstered = 2;
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_BOW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					if (ent->client->ps.saberHolstered == 1
						&& ent->client->saber[1].model[0])
					{
						//turn off second saber
						if (ent->client->ps.weapon == WP_SABER)
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOff);
					}
					else if (ent->client->ps.saberHolstered == 0)
					{
						//turn off first
						if (ent->client->ps.weapon == WP_SABER)
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOff);
					}
					ent->client->ps.saberHolstered = 2;
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_BOW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}
			break;
		case TAUNT_MEDITATE:
			//G_AddEvent(ent, EV_TAUNT, taunt);
			if (ent->client->ps.weapon != WP_SABER) //MP
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->pers.botclass == BCLASS_VADER || ent->client->pers.botclass == BCLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
				else
				{
					if (ent->client->pers.botclass == BCLASS_VADER || ent->client->pers.botclass == BCLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						anim = BOTH_MEDITATE1;
					}
				}
			}
			else if (ent->client->saber[0].meditateAnim != -1)
			{
				anim = ent->client->saber[0].meditateAnim;
			}
			else if (ent->client->saber[1].model[0]
				&& ent->client->saber[1].meditateAnim != -1)
			{
				anim = ent->client->saber[1].meditateAnim;
			}
			else
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					if (ent->client->ps.saberHolstered == 1
						&& ent->client->saber[1].model[0])
					{
						//turn off second saber
						G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOff);
					}
					else if (ent->client->ps.saberHolstered == 0)
					{
						//turn off first
						G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOff);
					}
					ent->client->ps.saberHolstered = 2;
					anim = BOTH_MEDITATE;
				}
			}
			break;
		case TAUNT_FLOURISH:
			G_AddEvent(ent, EV_TAUNT, taunt);
			if (ent->client->ps.weapon != WP_SABER) //MP
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->pers.botclass == BCLASS_VADER || ent->client->pers.botclass == BCLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL2,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
				else
				{
					if (ent->client->pers.botclass == BCLASS_VADER || ent->client->pers.botclass == BCLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL2,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
			}
			else if (ent->client->saber[0].flourishAnim != -1)
			{
				anim = ent->client->saber[0].flourishAnim;
			}
			else if (ent->client->saber[1].model[0]
				&& ent->client->saber[1].flourishAnim != -1)
			{
				anim = ent->client->saber[1].flourishAnim;
			}
			else if (ent->client->saber[1].model[0]
				&& ent->client->saber[1].flourishAnim != -1)
			{
				anim = ent->client->saber[1].flourishAnim;
			}
			else
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					switch (ent->client->ps.fd.saberAnimLevel)
					{
					case SS_FAST:
					case SS_TAVION:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_MEDIUM:
						if (saber1 && saber1->type == SABER_OBIWAN) //saber kylo
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_OBI, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_MEDIUM, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_STRONG:
					case SS_DESANN:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_STRONG, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_DUAL:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_DUAL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_STAFF:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_STAFF, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					default:;
					}
				}
				else
				{
					switch (ent->client->ps.fd.saberAnimLevel)
					{
					case SS_FAST:
					case SS_TAVION:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_MEDIUM:
						if (saber1 && saber1->type == SABER_OBIWAN) //saber kylo
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_OBI, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_MEDIUM, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_STRONG:
					case SS_DESANN:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_STRONG, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_DUAL:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_DUAL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_STAFF:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_STAFF, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					default:;
					}
				}
				if (ent->client->ps.saberHolstered == 1
					&& ent->client->saber[1].model[0])
				{
					//turn on second saber
					G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOn);
				}
				else if (ent->client->ps.saberHolstered == 2)
				{
					//turn on first
					G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
				}
				ent->client->ps.saberHolstered = 0;
			}
			break;
		case TAUNT_GLOAT:
			G_AddEvent(ent, EV_TAUNT, taunt);
			if (ent->client->ps.weapon != WP_SABER) //MP
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->pers.botclass == BCLASS_VADER || ent->client->pers.botclass == BCLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL3,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
				else
				{
					if (ent->client->pers.botclass == BCLASS_VADER || ent->client->pers.botclass == BCLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL3,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
			}
			else if (ent->client->saber[0].gloatAnim != -1)
			{
				anim = ent->client->saber[0].gloatAnim;
			}
			else if (ent->client->saber[1].model[0]
				&& ent->client->saber[1].gloatAnim != -1)
			{
				anim = ent->client->saber[1].gloatAnim;
			}
			else
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					switch (ent->client->ps.fd.saberAnimLevel)
					{
					case SS_FAST:
					case SS_TAVION:
						if (ent->client->ps.saberHolstered)
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_MEDIUM:
						if (ent->client->ps.saberHolstered)
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_MEDIUM, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_STRONG:
					case SS_DESANN:
						if (ent->client->ps.saberHolstered)
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_STRONG, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_DUAL:
						if (ent->client->ps.saberHolstered == 1
							&& ent->client->saber[1].model[0])
						{
							//turn on second saber
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOn);
						}
						else if (ent->client->ps.saberHolstered == 2)
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_DUAL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_STAFF:
						if (ent->client->ps.saberHolstered)
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_STAFF, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					default:;
					}
				}
				else
				{
					switch (ent->client->ps.fd.saberAnimLevel)
					{
					case SS_FAST:
					case SS_TAVION:
						if (ent->client->ps.saberHolstered)
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_MEDIUM:
						if (ent->client->ps.saberHolstered)
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_MEDIUM, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_STRONG:
					case SS_DESANN:
						if (ent->client->ps.saberHolstered)
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_STRONG, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_DUAL:
						if (ent->client->ps.saberHolstered == 1
							&& ent->client->saber[1].model[0])
						{
							//turn on second saber
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOn);
						}
						else if (ent->client->ps.saberHolstered == 2)
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_DUAL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_STAFF:
						if (ent->client->ps.saberHolstered)
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
						}
						ent->client->ps.saberHolstered = 0;
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_STAFF, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					default:;
					}
				}
			}
			break;
		case TAUNT_SURRENDER:
			G_AddEvent(ent, EV_TAUNT, taunt);
			if (ent->client->ps.weapon != WP_SABER) //MP
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->pers.botclass == BCLASS_VADER || ent->client->pers.botclass == BCLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL4,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
				else
				{
					if (ent->client->pers.botclass == BCLASS_VADER || ent->client->pers.botclass == BCLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.torsoAnim != BOTH_COWER1 && ent->client->ps.torsoAnim != BOTH_COWER1_START)
						{
							anim = BOTH_COWER1_START;
						}
						else if (ent->client->ps.torsoAnim == BOTH_COWER1_START)
						{
							anim = BOTH_COWER1;
						}
					}
				}
			}
			else if (ent->client->saber[0].surrenderAnim != -1)
			{
				anim = ent->client->saber[0].surrenderAnim;
			}
			else if (ent->client->saber[1].model[0]
				&& ent->client->saber[1].surrenderAnim != -1)
			{
				anim = ent->client->saber[1].surrenderAnim;
			}
			else
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->pers.botclass == BCLASS_VADER || ent->client->pers.botclass == BCLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
				else
				{
					if (ent->client->pers.botclass == BCLASS_VADER || ent->client->pers.botclass == BCLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.saberHolstered == 1
							&& ent->client->saber[1].model[0])
						{
							//turn off second saber
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOff);
						}
						else if (ent->client->ps.saberHolstered == 0)
						{
							//turn off first
							G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOff);
						}
						ent->client->ps.saberHolstered = 2;
						anim = PLAYER_SURRENDER_START;
					}
				}
			}
			break;
		case TAUNT_RELOAD:
			if (IsHoldingReloadableGun(ent)) //MP
			{
				if (ent->reloadTime > 0)
				{
					CancelReload(ent);
				}
				else
				{
					WP_ReloadGun(ent);
				}
				break;
			}
			if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
			{
				//TORSO ONLY
				switch (ent->client->ps.fd.saberAnimLevel)
				{
				case SS_FAST:
				case SS_TAVION:
					if (ent->client->ps.saberHolstered == 1
						&& ent->client->saber[1].model[0])
					{
						//turn off second saber
						G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOff);
					}
					else if (ent->client->ps.saberHolstered == 0)
					{
						//turn off first
						G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOff);
					}
					ent->client->ps.saberHolstered = 2;
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_GESTURE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_MEDIUM:
					if (saber1 && saber1->type == SABER_OBIWAN) //saber kylo
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_OBI,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					break;
				case SS_STRONG:
				case SS_DESANN:
					if (saber1 && saber1->type == SABER_VADER || ent->client->pers.botclass == BCLASS_VADER) //saber kylo
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					break;
				case SS_DUAL:
					if (ent->client->ps.saberHolstered == 1 && ent->client->saber[1].model[0])
					{
						//turn on second saber
						G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOn);
					}
					else if (ent->client->ps.saberHolstered == 2)
					{
						//turn on first
						G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
					}
					ent->client->ps.saberHolstered = 0;
					if (ent->client->saber[0].type == SABER_GRIE || ent->client->saber[0].type == SABER_GRIE4)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_DUAL_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					break;
				case SS_STAFF:
					if (ent->client->ps.saberHolstered > 0)
					{
						//turn on all blades
						G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
					}
					ent->client->ps.saberHolstered = 0;
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				default:;
				}
			}
			else
			{
				switch (ent->client->ps.fd.saberAnimLevel)
				{
				case SS_FAST:
				case SS_TAVION:
					if (ent->client->ps.saberHolstered == 1
						&& ent->client->saber[1].model[0])
					{
						//turn off second saber
						G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOff);
					}
					else if (ent->client->ps.saberHolstered == 0)
					{
						//turn off first
						G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOff);
					}
					ent->client->ps.saberHolstered = 2;
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_GESTURE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_MEDIUM:
					if (saber1 && saber1->type == SABER_OBIWAN) //saber kylo
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_OBI,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					break;
				case SS_STRONG:
				case SS_DESANN:
					if (saber1 && saber1->type == SABER_VADER || ent->client->pers.botclass == BCLASS_VADER) //saber kylo
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					break;
				case SS_DUAL:
					if (ent->client->ps.saberHolstered == 1 && ent->client->saber[1].model[0])
					{
						//turn on second saber
						G_Sound(ent, CHAN_WEAPON, ent->client->saber[1].soundOn);
					}
					else if (ent->client->ps.saberHolstered == 2)
					{
						//turn on first
						G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
					}
					ent->client->ps.saberHolstered = 0;
					if (ent->client->saber[0].type == SABER_GRIE || ent->client->saber[0].type == SABER_GRIE4)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_DUAL_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					break;
				case SS_STAFF:
					if (ent->client->ps.saberHolstered > 0)
					{
						//turn on all blades
						G_Sound(ent, CHAN_WEAPON, ent->client->saber[0].soundOn);
					}
					ent->client->ps.saberHolstered = 0;
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				default:;
				}
			}
			break;
		default:;
		}

		if (anim != -1)
		{
			if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				int parts = SETANIM_TORSO;

				if (anim != BOTH_ENGAGETAUNT)
				{
					parts = SETANIM_BOTH;
					VectorClear(ent->client->ps.velocity);
				}
				NPC_SetAnim(ent, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
		}
	}
}

static qboolean ClientCinematicThink(gclient_t* client, const usercmd_t* ucmd);
extern qboolean in_camera;
extern qboolean player_locked;
extern char cinematicSkipScript[1024];
extern qboolean skippingCutscene;
extern qboolean inGameCinematic;

void G_SetsaberdownorAnim(gentity_t* ent)
{
	if (ent->client->ps.saberLockTime >= level.time)
	{
		return;
	}
	if (ent->client->ps.weapon == WP_SABER)
	{
		if (ent->client && ent->client->ps.weaponTime < 1 && ent->watertype != CONTENTS_WATER)
		{
			if (ent->client->ps.saberHolstered == 2)
			{
				ent->client->ps.saberHolstered = 0;

				if (ent->client->saber[0].soundOn)
				{
					G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOn);
				}
				if (ent->client->saber[1].soundOn)
				{
					G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOn);
				}
			}
			else
			{
				ent->client->ps.saberHolstered = 2;
				if (ent->client->saber[0].soundOff)
				{
					G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
				}
				if (ent->client->saber[1].soundOff &&
					ent->client->saber[1].model[0])
				{
					G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
				}
				ent->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
				ent->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
				//prevent anything from being done for 400ms after holster
				ent->client->ps.weaponTime = 400;
			}
		}
	}
	else
	{
		if (IsHoldingReloadableGun(ent)) //mp
		{
			if (ent->reloadTime > 0)
			{
				CancelReload(ent);
			}
			else
			{
				WP_ReloadGun(ent);
			}
		}
		else
		{
			G_SetTauntAnim(ent, TAUNT_TAUNT);
		}
	}
}

//// reload ////

static int ReloadTime(const gentity_t* ent)
{
	if (ent->client->ps.weapon == WP_FLECHETTE)
	{
		return 1000;
	}
	if (ent->client->ps.weapon == WP_ROCKET_LAUNCHER)
	{
		return 3000;
	}
	return 300;
}

static int PainTime(const gentity_t* ent)
{
	if (ent->client->ps.weapon == WP_FLECHETTE)
	{
		return 250;
	}
	if (ent->client->ps.weapon == WP_ROCKET_LAUNCHER)
	{
		return 500;
	}
	return 150;
}

int fire_deley_time()
{
	return 500;
}

qboolean IsHoldingReloadableGun(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_BRYAR_OLD:
	case WP_BLASTER:
	case WP_DISRUPTOR:
	case WP_BOWCASTER:
	case WP_REPEATER:
	case WP_DEMP2:
	case WP_FLECHETTE:
	case WP_ROCKET_LAUNCHER:
	case WP_CONCUSSION:
	case WP_BRYAR_PISTOL:
		return qtrue;
	default:;
	}
	return qfalse;
}

static int ClipSize(const int ammo, gentity_t* ent)
{
	switch (ammo)
	{
	case AMMO_BLASTER:
		return 20;
	case AMMO_POWERCELL:
		return 30;
	case AMMO_METAL_BOLTS:
		return 50;
	case AMMO_ROCKETS:
		return 2;
	default:;
	}
	return -1;
}

static int MagazineSize(const int ammo, gentity_t* ent)
{
	switch (ammo)
	{
	case AMMO_BLASTER:
		return 75;
	case AMMO_POWERCELL:
		return 75;
	case AMMO_METAL_BOLTS:
		return 100;
	case AMMO_ROCKETS:
		return 3;
	default:;
	}
	return -1;
}

void WP_ReloadGun(gentity_t* ent)
{
	if (ent->reloadCooldown > level.time)
	{
		return;
	}

	if (IsHoldingReloadableGun(ent))
	{
		if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_TWELVE)
		{
			NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RELOADFAIL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/reloadfail.mp3");
			G_SoundOnEnt(ent, CHAN_VOICE_ATTEN, "*pain25.wav");
			G_Damage(ent, NULL, NULL, NULL, ent->r.currentOrigin, 2, DAMAGE_NO_ARMOR, MOD_LAVA);
			ent->reloadTime = level.time + PainTime(ent);
		}
		else
		{
			if (ent->s.weapon == WP_BRYAR_OLD || ent->s.weapon == WP_BLASTER || ent->s.weapon == WP_BRYAR_PISTOL)
			{
				if (ent->client->ps.ammo[AMMO_BLASTER] < ClipSize(AMMO_BLASTER, ent))
				{
					ent->client->ps.ammo[AMMO_BLASTER] += MagazineSize(AMMO_BLASTER, ent);
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RELOAD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/reload.mp3");
					ent->reloadTime = level.time + ReloadTime(ent);
				}
				else
				{
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RECHARGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/recharge.mp3");
				}
			}
			else if (ent->s.weapon == WP_DISRUPTOR || ent->s.weapon == WP_BOWCASTER || ent->s.weapon == WP_DEMP2)
			{
				if (ent->client->ps.ammo[AMMO_POWERCELL] < ClipSize(AMMO_POWERCELL, ent))
				{
					ent->client->ps.ammo[AMMO_POWERCELL] += MagazineSize(AMMO_POWERCELL, ent);
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RELOAD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/reload.mp3");
					ent->reloadTime = level.time + ReloadTime(ent);
				}
				else
				{
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RECHARGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/recharge.mp3");
				}
			}
			else if (ent->s.weapon == WP_REPEATER || ent->s.weapon == WP_FLECHETTE || ent->s.weapon == WP_CONCUSSION)
			{
				if (ent->client->ps.ammo[AMMO_METAL_BOLTS] < ClipSize(AMMO_METAL_BOLTS, ent))
				{
					ent->client->ps.ammo[AMMO_METAL_BOLTS] += MagazineSize(AMMO_METAL_BOLTS, ent);
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RELOAD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/reload.mp3");
					ent->reloadTime = level.time + ReloadTime(ent);
				}
				else
				{
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RECHARGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/recharge.mp3");
				}
			}
			else if (ent->s.weapon == WP_ROCKET_LAUNCHER)
			{
				if (ent->client->ps.ammo[AMMO_ROCKETS] < ClipSize(AMMO_ROCKETS, ent))
				{
					ent->client->ps.ammo[AMMO_ROCKETS] += MagazineSize(AMMO_ROCKETS, ent);
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RELOAD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/reload.mp3");
					ent->reloadTime = level.time + ReloadTime(ent);
				}
				else
				{
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RECHARGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/recharge.mp3");
				}
			}
			else
			{
				ent->reloadTime = 0;
			}
			ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
			bg_reduce_blaster_mishap_level_advanced(&ent->client->ps);
		}
	}
}

void FireOverheatFail(gentity_t* ent)
{
	if (IsHoldingReloadableGun(ent))
	{
		NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RELOADFAIL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/reloadfail.mp3");
		G_SoundOnEnt(ent, CHAN_VOICE_ATTEN, "*pain25.wav");
		G_Damage(ent, NULL, NULL, NULL, ent->r.currentOrigin, 2, DAMAGE_NO_ARMOR, MOD_LAVA);
		ent->reloadTime = level.time + PainTime(ent);
	}
}

void CancelReload(gentity_t* ent)
{
	ent->reloadTime = 0;
	ent->reloadCooldown = level.time + 500;
}
////////////////////// reload

static qboolean IsGunner(const gentity_t* ent)
{
	switch (ent->s.weapon)
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

static qboolean Bot_Is_Saber_Class(gentity_t* ent)
{
	// Evasion/Weapon Switching/etc...
	switch (ent->client->pers.botclass)
	{
	case BCLASS_ALORA:
	case BCLASS_CULTIST:
	case BCLASS_DESANN:
	case BCLASS_JEDI:
	case BCLASS_JEDIMASTER:
	case BCLASS_JEDITRAINER:
	case BCLASS_KYLE:
	case BCLASS_LUKE:
	case BCLASS_MORGANKATARN:
	case BCLASS_REBORN:
	case BCLASS_REBORN_TWIN:
	case BCLASS_REBORN_MASTER:
	case BCLASS_SABER_DROID:
	case BCLASS_SHADOWTROOPER:
	case BCLASS_SERENITY:
	case BCLASS_CADENCE:
	case BCLASS_YODA:
	case BCLASS_PADAWAN:
	case BCLASS_SITHLORD:
	case BCLASS_VADER:
	case BCLASS_SITH:
	case BCLASS_APPRENTICE:
	case BCLASS_JEDIKNIGHT1:
	case BCLASS_JEDIKNIGHT2:
	case BCLASS_JEDIKNIGHT3:
	case BCLASS_JEDICONSULAR1:
	case BCLASS_JEDICONSULAR2:
	case BCLASS_JEDICONSULAR3:
	case BCLASS_SITHWORRIOR1:
	case BCLASS_SITHWORRIOR2:
	case BCLASS_SITHWORRIOR3:
	case BCLASS_SITHINQUISITOR1:
	case BCLASS_SITHINQUISITOR2:
	case BCLASS_SITHINQUISITOR3:
	case BCLASS_DUELS:
	case BCLASS_GRIEVOUS:
	case BCLASS_STAFF:
	case BCLASS_STAFFDARK:
	case BCLASS_UNSTABLESABER:
	case BCLASS_OBIWAN:
		break;
	default:
		return qfalse;
	}
	return qtrue;
}

static qboolean Is_Oversized_Gunner(gentity_t* ent)
{
	gclient_t* client = ent->client;

	if ((client->pers.botmodelscale == BOTZIZE_TALL ||
		client->pers.botmodelscale == BOTZIZE_LARGE ||
		client->pers.botmodelscale == BOTZIZE_LARGER ||
		client->pers.botmodelscale == BOTZIZE_MASSIVE) && !Bot_Is_Saber_Class(ent))
	{
		return qtrue;
	}
	return qfalse;
}

static qboolean Is_Undersized_Gunner(gentity_t* ent)
{
	gclient_t* client = ent->client;

	if ((client->pers.botmodelscale == BOTZIZE_SMALL ||
		client->pers.botmodelscale == BOTZIZE_SMALLER ||
		client->pers.botmodelscale == BOTZIZE_SMALLEST) && !Bot_Is_Saber_Class(ent))
	{
		return qtrue;
	}
	return qfalse;
}

static qboolean Is_Undersized_Jedi(gentity_t* ent)
{
	gclient_t* client = ent->client;

	if ((client->pers.botmodelscale == BOTZIZE_SMALLER ||
		client->pers.botmodelscale == BOTZIZE_SMALLEST) && Bot_Is_Saber_Class(ent))
	{
		return qtrue;
	}
	return qfalse;
}

static void CG_BreathPuffsVader(gentity_t* ent)
{
	gclient_t* client = ent->client;

	if (ent->health < 1 || client->VaderBreathTime > level.time)
	{
		return;
	}

	if (ent->s.eFlags & EF_NODRAW)
	{
		return;
	}

	if (in_camera)
	{
		return;
	}

	if (PM_CrouchAnim(client->ps.legsAnim) ||
		PM_CrouchAnim(client->ps.torsoAnim) ||
		PM_LungeAnim(client->ps.torsoAnim) ||
		PM_RollingAnim(client->ps.legsAnim))
	{
		return;
	}

	if (client->pers.botclass == BCLASS_VADER || client->NPC_class == CLASS_VADER)
	{
		if (ent->health < 50)
		{
			if (ent->health < 20)
			{
				G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/chars/darthvader/breath3.mp3"));
			}
			else
			{
				G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/chars/darthvader/breath4.mp3"));
			}
		}
		else
		{
			if (client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING ||
				client->ps.PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
			{
				G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/chars/darthvader/vader_fast_breath.mp3"));
			}
			else
			{
				if (!Q_irand(0, 4))
				{
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/chars/darthvader/breath2.mp3"));
				}
				else
				{
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/chars/darthvader/breath2.mp3"));
				}
			}
		}
	}

	if (PM_SaberInAttack(client->ps.saber_move))
	{
		client->VaderBreathTime = level.time + 4000; // every 4 seconds.
	}
	else
	{
		if (PM_RunningAnim(ent->client->ps.legsAnim))
		{
			client->VaderBreathTime = level.time + 3000; // every 2 seconds.
		}
		else
		{
			client->VaderBreathTime = level.time + 6000; // every 6 seconds.
		}
	}
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
extern qboolean WP_SaberCanTurnOffSomeBlades(const saberInfo_t* saber);
extern qboolean player_locked;
extern qboolean g_standard_humanoid(gentity_t* self);
extern void Weapon_AltStun_Fire(gentity_t* ent);

static void ClientThink_real(gentity_t* ent)
{
	gclient_t* client;
	pmove_t pmove;
	int old_event_sequence;
	int msec;
	usercmd_t* ucmd;
	qboolean isNPC = qfalse;
	qboolean controlledByPlayer = qfalse;
	qboolean killJetFlags = qtrue;
	qboolean isFollowing;

	client = ent->client;

	qboolean HoldingStun = ent->client->ps.communicatingflags & 1 << STUNNING ? qtrue : qfalse;

	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	if (inGameCinematic)
	{
		//don't update the game world if an ROQ files is running.
		return;
	}

	if (ent->s.eType == ET_NPC)
	{
		isNPC = qtrue;
	}

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED && !isNPC)
	{
		return;
	}

	if (ent->client->ps.ManualMBlockingTime <= level.time && ent->client->ps.ManualMBlockingTime > 0)
	{
		ent->client->ps.userInt3 &= ~(1 << FLAG_BLOCKING);
		ent->client->ps.ManualMBlockingTime = 0;
	}
	if (ent->client->isHacking == -100 && ent->client->ps.hackingTime <= level.time)
	{
		ItemUse_Sentry2(ent);
		ent->client->isHacking = qfalse;
	}
	if (ent->sentryDeadThink <= level.time && ent->sentryDeadThink > 0)
	{
		ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SENTRY_GUN;
		ent->sentryDeadThink = 0;
	}
	if (ent->forceFieldThink <= level.time && ent->forceFieldThink > 0)
	{
		ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SHIELD;
		ent->forceFieldThink = 0;
	}

	// This code was moved here from clientThink to fix a problem with g_synchronousClients
	// being set to 1 when in vehicles.
	if (ent->s.number < MAX_CLIENTS && ent->client->ps.m_iVehicleNum)
	{
		//driving a vehicle
		if (g_entities[ent->client->ps.m_iVehicleNum].client)
		{
			gentity_t* veh = &g_entities[ent->client->ps.m_iVehicleNum];

			if (veh->m_pVehicle &&
				veh->m_pVehicle->m_pPilot == (bgEntity_t*)ent)
			{
				//only take input from the pilot...
				veh->client->ps.commandTime = ent->client->ps.commandTime;
				memcpy(&veh->m_pVehicle->m_ucmd, &ent->client->pers.cmd, sizeof(usercmd_t));
				if (veh->m_pVehicle->m_ucmd.buttons & BUTTON_TALK)
				{
					//forced input if "chat bubble" is up
					veh->m_pVehicle->m_ucmd.buttons = BUTTON_TALK;
					veh->m_pVehicle->m_ucmd.forwardmove = 0;
					veh->m_pVehicle->m_ucmd.rightmove = 0;
					veh->m_pVehicle->m_ucmd.upmove = 0;
				}
			}
		}
	}

	isFollowing = client->ps.pm_flags & PMF_FOLLOW ? qtrue : qfalse;

	if (!isFollowing)
	{
		if (level.gametype == GT_SIEGE &&
			client->siegeClass != -1 &&
			bgSiegeClasses[client->siegeClass].saberStance)
		{ //the class says we have to use this stance set.
			if (!(bgSiegeClasses[client->siegeClass].saberStance & (1 << client->ps.fd.saberAnimLevel)))
			{ //the current stance is not in the bitmask, so find the first one that is.
				int i = SS_FAST;

				while (i < SS_NUM_SABER_STYLES)
				{
					if (bgSiegeClasses[client->siegeClass].saberStance & (1 << i))
					{
						if (i == SS_DUAL
							&& client->ps.saberHolstered == 1)
						{//one saber should be off, adjust saberAnimLevel accordinly
							client->ps.fd.saber_anim_levelBase = i;
							client->ps.fd.saberAnimLevel = SS_FAST;
							client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevel;
						}
						else if (i == SS_STAFF
							&& client->ps.saberHolstered == 1
							&& client->saber[0].singleBladeStyle != SS_NONE)
						{//one saber or blade should be off, adjust saberAnimLevel accordinly
							client->ps.fd.saber_anim_levelBase = i;
							client->ps.fd.saberAnimLevel = client->saber[0].singleBladeStyle;
							client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevel;
						}
						else
						{
							client->ps.fd.saber_anim_levelBase = client->ps.fd.saberAnimLevel = i;
							client->ps.fd.saberDrawAnimLevel = i;
						}
						break;
					}

					i++;
				}
			}
		}
		else if (client->saber[0].model[0] && client->saber[1].model[0])
		{ //with two sabs always use akimbo style
			if (client->ps.saberHolstered == 1)
			{//one saber should be off, adjust saberAnimLevel accordinly
				client->ps.fd.saber_anim_levelBase = SS_DUAL;
				client->ps.fd.saberAnimLevel = SS_FAST;
				client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevel;
			}
			else
			{
				if (!WP_SaberStyleValidForSaber(&client->saber[0], &client->saber[1], client->ps.saberHolstered, client->ps.fd.saberAnimLevel))
				{//only use dual style if the style we're trying to use isn't valid
					client->ps.fd.saber_anim_levelBase = client->ps.fd.saberAnimLevel = SS_DUAL;
				}
				client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevel;
			}
		}
		else
		{
			if (client->saber[0].stylesLearned == (1 << SS_STAFF))
			{ //then *always* use the staff style
				client->ps.fd.saber_anim_levelBase = SS_STAFF;
			}
			if (client->ps.fd.saber_anim_levelBase == SS_STAFF)
			{//using staff style
				if (client->ps.saberHolstered == 1
					&& client->saber[0].singleBladeStyle != SS_NONE)
				{//one blade should be off, adjust saberAnimLevel accordinly
					client->ps.fd.saberAnimLevel = client->saber[0].singleBladeStyle;
					client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevel;
				}
				else
				{
					client->ps.fd.saberAnimLevel = SS_STAFF;
					client->ps.fd.saberDrawAnimLevel = client->ps.fd.saberAnimLevel;
				}
			}
		}
	}

	if (client && !isFollowing && client->ps.eFlags2 & EF2_HELD_BY_MONSTER)
	{
		G_HeldByMonster(ent, ucmd);
	}

	if (client && client->ps.clientNum < MAX_CLIENTS)
	{
		//player stuff
		if (in_camera)
		{
			// watch the code here, you MUST "return" within this IF(), *unless* you're stopping the cinematic skip.
			//skip cutscene code
			if (!skippingCutscene && (g_skipcutscenes.integer == 1
				|| g_skipcutscenes.integer == 2 || ClientCinematicThink(ent->client, &ent->client->pers.cmd)))
			{
				//order the system to accelerate the cutscene
				if (cinematicSkipScript[0])
				{
					//there's a skip script for this cutscene
					trap->ICARUS_RunScript((sharedEntity_t*)&g_entities[client->ps.clientNum], cinematicSkipScript);
					cinematicSkipScript[0] = 0;
				}

				skippingCutscene = qtrue;
				if (g_skipcutscenes.integer == 2 || g_skipcutscenes.integer == 4)
				{
					//use time accell
					trap->Cvar_Set("timescale", "100");
				}
				return;
			}
			if (skippingCutscene && (g_skipcutscenes.integer == 0
				|| (g_skipcutscenes.integer == 3 || g_skipcutscenes.integer == 4)
				&& ClientCinematicThink(ent->client, &ent->client->pers.cmd)))
			{
				//stopping a skip already in progress
				skippingCutscene = qfalse;
				trap->Cvar_Set("timescale", "1");
			}
		}

		if (player_locked)
		{
			//players' controls are locked, a scripting thingy
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->buttons = 0;
			ucmd->upmove = 0;
		}
	}

	if (g_VaderBreath.integer == 1)
	{
		CG_BreathPuffsVader(ent);
	}

	// sanity check the command time to prevent speedup cheating
	if (ucmd->serverTime > level.time + 200)
	{
		ucmd->serverTime = level.time + 200;
	}
	if (ucmd->serverTime < level.time - 1000)
	{
		ucmd->serverTime = level.time - 1000;
	}

	if (isNPC && ucmd->serverTime - client->ps.commandTime < 1)
	{
		ucmd->serverTime = client->ps.commandTime + 100;
	}

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if (msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW)
	{
		return;
	}

	if (msec > 200)
	{
		msec = 200;
	}

	if (pmove_msec.integer < 8)
	{
		trap->Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33)
	{
		trap->Cvar_Set("pmove_msec", "33");
	}

	if (pmove_fixed.integer || client->pers.pmoveFixed)
	{
		ucmd->serverTime = (ucmd->serverTime + pmove_msec.integer - 1) / pmove_msec.integer * pmove_msec.integer;
	}

	//
	// check for exiting intermission
	//
	if (level.intermissiontime)
	{
		if (ent->s.number < MAX_CLIENTS
			|| client->NPC_class == CLASS_VEHICLE)
		{
			//players and vehicles do nothing in intermissions
			ClientIntermissionThink(client);
			return;
		}
	}

	// spectators don't do much
	if (client->ps.clientNum < MAX_CLIENTS && (client->sess.sessionTeam == TEAM_SPECTATOR || client->tempSpectate >
		level.time))
	{
		if (client->sess.spectatorState == SPECTATOR_SCOREBOARD)
		{
			return;
		}
		SpectatorThink(ent, ucmd);
		return;
	}

	if (ent && ent->client && ent->client->ps.eFlags & EF_INVULNERABLE)
	{
		if (ent->client->invulnerableTimer <= level.time)
		{
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			if (ent->flags & FL_GODMODE)
			{
				ent->flags &= ~FL_GODMODE;
			}
		}
	}

	if (ent && ent->client && (ent->client->ps.eFlags & EF_TALK))
	{
		//#ifdef _GAME
				//if (g_chat_protection.integer == 1 && !ent->client->ps.duelInProgress && !ent->client->pers.isbeingpunished)
				//{
		ent->flags |= FL_GODMODE;
		//}
//#endif
	}
	else
	{
		//#ifdef _GAME
				//if (g_chat_protection.integer == 1 && !ent->client->ps.duelInProgress && !ent->client->pers.isbeingpunished)
				//{
		ent->flags &= ~FL_GODMODE;
		//}
//#endif
	}

	if (ent && ent->s.eType != ET_NPC)
	{
		// check for inactivity timer, but never drop the local client of a non-dedicated server
		if (!ClientInactivityTimer(client))
		{
			return;
		}
	}

	// clear the rewards if time
	/*if (level.time > client->rewardTime)
	{
		client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
	}*/

	//Check if we should have a fullbody push effect around the player
	if (client->pushEffectTime > level.time)
	{
		if (com_outcast.integer == 0) //JKA
		{
			client->ps.eFlags |= EF_BODYPUSH;
		}
		else if (com_outcast.integer == 1) //JKO
		{
			client->ps.eFlags |= EF_BODYPUSH;
		}
		else if (com_outcast.integer == 2) // NO EFFECT
		{
			//no effect
		}
		else // BACKUP
		{
			client->ps.eFlags |= EF_BODYPUSH;
		}
	}
	else if (client->pushEffectTime)
	{
		client->pushEffectTime = 0;
		client->ps.eFlags &= ~EF_BODYPUSH;
	}

	if (client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
	{
		client->ps.eFlags |= EF_JETPACK;
	}
	else
	{
		client->ps.eFlags &= ~EF_JETPACK;
	}

	if (client->noclip)
	{
		client->ps.pm_type = PM_NOCLIP;
	}
	else if (client->ps.eFlags & EF_DISINTEGRATION)
	{
		client->ps.pm_type = PM_NOCLIP;
	}
	else if (client->ps.stats[STAT_HEALTH] <= 0)
	{
		client->ps.pm_type = PM_DEAD;
	}
	else
	{
		if (client->ps.forceGripChangeMovetype)
		{
			client->ps.pm_type = client->ps.forceGripChangeMovetype;
		}
		else
		{
			if (client->jetPackOn)
			{
				client->ps.pm_type = PM_JETPACK;
				client->ps.eFlags |= EF_JETPACK_ACTIVE;
				killJetFlags = qfalse;
			}
			else
			{
				client->ps.pm_type = PM_NORMAL;
			}

			if (g_forcekickflip.integer >= 1)
			{
				if (client->ps.userInt2 != 1)
					client->ps.userInt2 = 1;
			}
		}
	}

	if (killJetFlags)
	{
		client->ps.eFlags &= ~EF_JETPACK_ACTIVE;
		client->ps.eFlags &= ~EF_JETPACK_FLAMING;
	}

#define	SLOWDOWN_DIST	128.0f
#define	MIN_NPC_SPEED	16.0f

	if (client->bodyGrabIndex != ENTITYNUM_NONE)
	{
		gentity_t* grabbed = &g_entities[client->bodyGrabIndex];

		if (!grabbed->inuse || grabbed->s.eType != ET_BODY ||
			grabbed->s.eFlags & EF_DISINTEGRATION ||
			grabbed->s.eFlags & EF_NODRAW)
		{
			if (grabbed->inuse && grabbed->s.eType == ET_BODY)
			{
				grabbed->s.ragAttach = 0;
			}
			client->bodyGrabIndex = ENTITYNUM_NONE;
		}
		else
		{
			mdxaBone_t rhMat;
			vec3_t rhOrg, tAng;
			vec3_t bodyDir;
			float bodyDist;

			ent->client->ps.forceHandExtend = HANDEXTEND_DRAGGING;

			if (ent->client->ps.forceHandExtendTime < level.time + 500)
			{
				ent->client->ps.forceHandExtendTime = level.time + 1000;
			}

			VectorSet(tAng, 0, ent->client->ps.viewangles[YAW], 0);
			trap->G2API_GetBoltMatrix(ent->ghoul2, 0, 0, &rhMat, tAng, ent->client->ps.origin, level.time,
				NULL, ent->modelScale); //0 is always going to be right hand bolt
			BG_GiveMeVectorFromMatrix(&rhMat, ORIGIN, rhOrg);

			VectorSubtract(rhOrg, grabbed->r.currentOrigin, bodyDir);
			bodyDist = VectorLength(bodyDir);

			if (bodyDist > 40.0f)
			{
				//can no longer reach
				grabbed->s.ragAttach = 0;
				client->bodyGrabIndex = ENTITYNUM_NONE;
			}
			else if (bodyDist > 24.0f)
			{
				bodyDir[2] = 0; //don't want it floating
				//VectorScale(bodyDir, 0.1f, bodyDir);
				VectorAdd(grabbed->epVelocity, bodyDir, grabbed->epVelocity);
				G_Sound(grabbed, CHAN_AUTO, G_SoundIndex("sound/player/roll1.wav"));
			}
		}
	}
	else if (client->ps.forceHandExtend == HANDEXTEND_DRAGGING)
	{
		ent->client->ps.forceHandExtend = HANDEXTEND_WEAPONREADY;
	}

	if (ent->NPC && ent->s.NPC_class != CLASS_VEHICLE) //vehicles manage their own speed
	{
		//FIXME: swoop should keep turning (and moving forward?) for a little bit?
		if (ent->NPC->combatMove == qfalse)
		{
			if (!(ucmd->buttons & BUTTON_USE))
			{
				//Not leaning
				qboolean Flying = ucmd->upmove && ent->client->ps.eFlags2 & EF2_FLYING;
				qboolean Climbing = ucmd->upmove && ent->watertype & CONTENTS_LADDER;

				//client->ps.friction = 6;

				if (ucmd->forwardmove || ucmd->rightmove || Flying)
				{
					//if ( ent->NPC->behaviorState != BS_FORMATION )
					{
						//In - Formation NPCs set thier desiredSpeed themselves
						if (ucmd->buttons & BUTTON_WALKING)
						{
							ent->NPC->desiredSpeed = NPC_GetWalkSpeed(ent);
						}
						else //running
						{
							ent->NPC->desiredSpeed = NPC_GetRunSpeed(ent);
						}

						if (ent->NPC->currentSpeed >= 80 && !controlledByPlayer)
						{
							//At higher speeds, need to slow down close to stuff
							//Slow down as you approach your goal
							if (ent->NPC->distToGoal < SLOWDOWN_DIST && !(ent->NPC->aiFlags & NPCAI_NO_SLOWDOWN)) //128
							{
								if (ent->NPC->desiredSpeed > MIN_NPC_SPEED)
								{
									float slowdownSpeed = (float)ent->NPC->desiredSpeed * ent->NPC->distToGoal /
										SLOWDOWN_DIST;

									ent->NPC->desiredSpeed = ceil(slowdownSpeed);
									if (ent->NPC->desiredSpeed < MIN_NPC_SPEED)
									{
										//don't slow down too much
										ent->NPC->desiredSpeed = MIN_NPC_SPEED;
									}
								}
							}
						}
					}
				}
				else if (Climbing)
				{
					ent->NPC->desiredSpeed = ent->NPC->stats.walkSpeed;
				}
				else
				{
					//We want to stop
					ent->NPC->desiredSpeed = 0;
				}

				NPC_Accelerate(ent, qfalse, qfalse);

				if (ent->NPC->currentSpeed <= 24 && ent->NPC->desiredSpeed < ent->NPC->currentSpeed)
				{
					//No-one walks this slow
					client->ps.speed = ent->NPC->currentSpeed = 0; //Full stop
					ucmd->forwardmove = 0;
					ucmd->rightmove = 0;
				}
				else
				{
					if (ent->NPC->currentSpeed <= ent->NPC->stats.walkSpeed)
					{
						//Play the walkanim
						ucmd->buttons |= BUTTON_WALKING;
					}
					else
					{
						ucmd->buttons &= ~BUTTON_WALKING;
					}

					if (ent->NPC->currentSpeed > 0)
					{
						//We should be moving
						if (Climbing || Flying)
						{
							if (!ucmd->upmove)
							{
								//We need to force them to take a couple more steps until stopped
								ucmd->upmove = ent->NPC->last_ucmd.upmove; //was last_upmove;
							}
						}
						else if (!ucmd->forwardmove && !ucmd->rightmove)
						{
							//We need to force them to take a couple more steps until stopped
							ucmd->forwardmove = ent->NPC->last_ucmd.forwardmove; //was last_forwardmove;
							ucmd->rightmove = ent->NPC->last_ucmd.rightmove; //was last_rightmove;
						}
					}

					client->ps.speed = ent->NPC->currentSpeed;

					if (1)
					{
						//Slow down on turns - don't orbit!!!
						float turndelta = 0;
						{
							turndelta = (180 - fabs(AngleDelta(ent->r.currentAngles[YAW], ent->NPC->desiredYaw))) / 180;
						}

						if (turndelta < 0.75f)
						{
							client->ps.speed = 0;
						}
						else if (ent->NPC->distToGoal < 100 && turndelta < 1.0)
						{
							//Turn is greater than 45 degrees or closer than 100 to goal
							client->ps.speed = floor(client->ps.speed * turndelta);
						}
					}
				}
			}
		}
		else
		{
			ent->NPC->desiredSpeed = ucmd->buttons & BUTTON_WALKING ? NPC_GetWalkSpeed(ent) : NPC_GetRunSpeed(ent);

			client->ps.speed = ent->NPC->desiredSpeed;
		}

		if (ucmd->buttons & BUTTON_WALKING && ent->s.eType != ET_NPC)
		{
			if (ucmd->forwardmove > 64)
			{
				ucmd->forwardmove = 64;
			}
			else if (ucmd->forwardmove < -64)
			{
				ucmd->forwardmove = -64;
			}

			if (ucmd->rightmove > 64)
			{
				ucmd->rightmove = 64;
			}
			else if (ucmd->rightmove < -64)
			{
				ucmd->rightmove = -64;
			}

			//ent->client->ps.speed = ent->client->ps.basespeed = NPC_GetRunSpeed( ent );
		}
		client->ps.basespeed = client->ps.speed;
	}
	else if (!client->ps.m_iVehicleNum &&
		(!ent->NPC || ent->s.NPC_class != CLASS_VEHICLE)) //if riding a vehicle it will manage our speed and such
	{
		// set speed
		client->ps.speed = g_speed.value;

		//Check for a siege class speed multiplier
		if (level.gametype == GT_SIEGE &&
			client->siegeClass != -1)
		{
			client->ps.speed *= bgSiegeClasses[client->siegeClass].speed;
		}
		if ((client->ps.stats[STAT_HEALTH] <= 25 && client->ps.stats[STAT_MAX_HEALTH] >= 100) ||
			(client->ps.stats[STAT_HEALTH] <= (client->ps.stats[STAT_MAX_HEALTH] / 3) && client->ps.stats[STAT_MAX_HEALTH] < 100))
		{
			//move slower when low on health
			client->ps.speed *= 0.85f;
		}
		else if ((client->ps.stats[STAT_HEALTH] <= 40 && client->ps.stats[STAT_MAX_HEALTH] >= 100) ||
			(client->ps.stats[STAT_HEALTH] <= (client->ps.stats[STAT_MAX_HEALTH] / 2) && client->ps.stats[STAT_MAX_HEALTH] < 100))
		{
			//move slower when low on health
			client->ps.speed *= 0.90f;
		}
		else if (client->bodyGrabIndex != ENTITYNUM_NONE)
		{
			//can't go nearly as fast when dragging a body around
			client->ps.speed *= 0.3f;
		}
		else if (BG_SprintAnim(ent->client->ps.legsAnim))
		{
			if (ent->client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING)
			{
				if (!(ent->r.svFlags & SVF_BOT))
				{
					if (client->pers.botclass == BCLASS_VADER)
					{
						client->ps.speed *= 1.15f;
					}
					else if (client->pers.botclass == BCLASS_YODA)
					{
						client->ps.speed *= 1.60f;
					}
					else
					{
						client->ps.speed *= 1.50f;
					}
				}
			}
		}
		else if (BG_SaberSprintAnim(ent->client->ps.legsAnim))
		{
			if (ent->client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING)
			{
				if (!(ent->r.svFlags & SVF_BOT))
				{
					if (client->pers.botclass == BCLASS_VADER)
					{
						client->ps.speed *= 1.15f;
					}
					else if (client->pers.botclass == BCLASS_YODA)
					{
						client->ps.speed *= 1.65f;
					}
					else
					{
						client->ps.speed *= 1.60f;
					}
				}
			}
		}
		else if (BG_WeaponSprintAnim(client->ps.legsAnim))
		{
			if (ent->client->ps.PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING)
			{
				if (!(ent->r.svFlags & SVF_BOT))
				{
					if (client->pers.botclass == BCLASS_VADER)
					{
						client->ps.speed *= 1.10f;
					}
					else if (client->pers.botclass == BCLASS_YODA)
					{
						client->ps.speed *= 1.60f;
					}
					else
					{
						client->ps.speed *= 1.30f;
					}
				}
			}
		}
		else if (client->ps.weapon == WP_BRYAR_PISTOL ||
			client->ps.weapon == WP_THERMAL ||
			client->ps.weapon == WP_DET_PACK ||
			client->ps.weapon == WP_TRIP_MINE)
		{
			client->ps.speed *= 0.85f;
		}

		client->ps.basespeed = client->ps.speed;
	}

	if (!ent->NPC || !(ent->NPC->aiFlags & NPCAI_CUSTOM_GRAVITY))
	{
		//use global gravity
		if (ent->NPC && ent->s.NPC_class == CLASS_VEHICLE &&
			ent->m_pVehicle && ent->m_pVehicle->m_pVehicleInfo->gravity)
		{
			//use custom veh gravity
			client->ps.gravity = ent->m_pVehicle->m_pVehicleInfo->gravity;
		}
		else
		{
			if (client->inSpaceIndex && client->inSpaceIndex != ENTITYNUM_NONE)
			{
				//in space, so no gravity...
				client->ps.gravity = 1.0f;
				if (ent->s.number < MAX_CLIENTS)
				{
					VectorScale(client->ps.velocity, 0.8f, client->ps.velocity);
				}
			}
			else
			{
				if (client->ps.eFlags2 & EF2_SHIP_DEATH)
				{
					//float there
					VectorClear(client->ps.velocity);
					client->ps.gravity = 1.0f;
				}
				else
				{
					client->ps.gravity = g_gravity.value;
				}
			}
		}
	}

	if (client->ps.duelInProgress)
	{
		gentity_t* duelAgainst = &g_entities[ent->client->ps.duelIndex];

		//Keep the time updated, so once this duel ends this player can't engage in a duel for another
		//10 seconds. This will give other people a chance to engage in duels in case this player wants
		//to engage again right after he's done fighting and someone else is waiting.
		ent->client->ps.fd.privateDuelTime = level.time + 10000;

		if (ent->client->ps.duelTime < level.time)
		{
			//Bring out the sabers
			if (ent->client->ps.weapon == WP_SABER
				&& ent->client->ps.saberHolstered
				&& ent->client->ps.duelTime)
			{
				ent->client->ps.saberHolstered = 0;

				if (ent->client->saber[0].soundOn)
				{
					G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOn);
				}
				if (ent->client->saber[1].soundOn)
				{
					G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOn);
				}

				G_AddEvent(ent, EV_PRIVATE_DUEL, 2);

				ent->client->ps.duelTime = 0;
			}

			if (duelAgainst
				&& duelAgainst->client
				&& duelAgainst->inuse
				&& duelAgainst->client->ps.weapon == WP_SABER
				&& duelAgainst->client->ps.saberHolstered
				&& duelAgainst->client->ps.duelTime)
			{
				duelAgainst->client->ps.saberHolstered = 0;

				if (duelAgainst->client->saber[0].soundOn)
				{
					G_Sound(duelAgainst, CHAN_AUTO, duelAgainst->client->saber[0].soundOn);
				}
				if (duelAgainst->client->saber[1].soundOn)
				{
					G_Sound(duelAgainst, CHAN_AUTO, duelAgainst->client->saber[1].soundOn);
				}

				G_AddEvent(duelAgainst, EV_PRIVATE_DUEL, 2);

				duelAgainst->client->ps.duelTime = 0;
			}
		}
		else
		{
			client->ps.speed = 0;
			client->ps.basespeed = 0;
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->upmove = 0;
		}

		if (!duelAgainst || !duelAgainst->client || !duelAgainst->inuse ||
			duelAgainst->client->ps.duelIndex != ent->s.number)
		{
			ent->client->ps.duelInProgress = 0;
			G_AddEvent(ent, EV_PRIVATE_DUEL, 0);
		}
		else if (duelAgainst->health < 1 || duelAgainst->client->ps.stats[STAT_HEALTH] < 1)
		{
			ent->client->ps.duelInProgress = 0;
			duelAgainst->client->ps.duelInProgress = 0;

			G_AddEvent(ent, EV_PRIVATE_DUEL, 0);
			G_AddEvent(duelAgainst, EV_PRIVATE_DUEL, 0);

			//Private duel announcements are now made globally because we only want one duel at a time.
			if (ent->health > 0 && ent->client->ps.stats[STAT_HEALTH] > 0)
			{
				trap->SendServerCommand(-1, va("cp \"%s %s %s!\n\"", ent->client->pers.netname,
					G_GetStringEdString("MP_SVGAME", "PLDUELWINNER"),
					duelAgainst->client->pers.netname));
				trap->SendServerCommand(-1, va("print \"%s ^7survived with ^5%d ^7health and ^5%d ^7shield\n\"",
					ent->client->pers.netname, ent->client->ps.stats[STAT_HEALTH],
					ent->client->ps.stats[STAT_ARMOR]));
			}
			else
			{
				//it was a draw, because we both managed to die in the same frame
				trap->SendServerCommand(-1, va("cp \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELTIE")));
			}

			//Winner gets full health.. providing he's still alive
			if (ent->health > 0 && ent->client->ps.stats[STAT_HEALTH] > 0)
			{
				if (ent->health < ent->client->ps.stats[STAT_MAX_HEALTH])
				{
					ent->client->ps.stats[STAT_HEALTH] = ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
				}

				if (ent->client->ps.stats[STAT_ARMOR] < ent->client->ps.stats[STAT_MAX_HEALTH])
				{
					ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];
				}

				if (g_spawnInvulnerability.integer)
				{
					if (!(ent->r.svFlags & SVF_BOT))
					{
						client->ps.powerups[PW_INVINCIBLE] = level.time + g_spawnInvulnerability.integer;
						ent->flags |= FL_GODMODE;
					}

					client->ps.eFlags |= EF_INVULNERABLE;
					client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
				}
			}
		}
		else
		{
			vec3_t vSub;
			float subLen = 0;

			VectorSubtract(ent->client->ps.origin, duelAgainst->client->ps.origin, vSub);
			subLen = VectorLength(vSub);

			if (g_remove_duel_radius.integer == 1)
			{
				if (subLen >= 1024)
				{
					ent->client->ps.duelInProgress = 0;
					duelAgainst->client->ps.duelInProgress = 0;

					G_AddEvent(ent, EV_PRIVATE_DUEL, 0);
					G_AddEvent(duelAgainst, EV_PRIVATE_DUEL, 0);

					trap->SendServerCommand(-1, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELSTOP")));
					trap->SendServerCommand(duelAgainst - g_entities,
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELSTOP")));
				}
			}
		}
	}

	// Activate the Blocking flags

	if (!(ent->r.svFlags & SVF_BOT) &&
		(ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)) &&
		client->ps.weapon == WP_SABER &&
		!PM_SaberInMassiveBounce(client->ps.torsoAnim) &&
		!PM_SaberInBashedAnim(client->ps.torsoAnim) &&
		!PM_Saberinstab(client->ps.saber_move))
	{
		if (manual_running_and_saberblocking(ent))
		{
			if (!(client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
			{
				client->ps.ManualBlockingFlags |= 1 << HOLDINGBLOCK;
				client->ps.userInt3 |= 1 << FLAG_BLOCKING;
				client->ps.ManualBlockingTime = level.time; //Blocking time 1 on
			}
		}
		else if (manual_saberblocking(ent))
		{
			if (!(client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
			{
				client->ps.ManualBlockingFlags |= 1 << HOLDINGBLOCK;
				client->ps.userInt3 |= 1 << FLAG_BLOCKING;
				client->ps.ManualBlockingTime = level.time; //Blocking time 1 on

				if (client->ps.weapon == WP_SABER && client->ps.saberHolstered && client->NPC_class != CLASS_YODA && !manual_running_and_saberblocking(ent)) // not yoda he can block lightning with a saber off in his hand
				{
					switch (client->ps.fd.saberAnimLevel)
					{
					case SS_FAST:
					case SS_TAVION:
					case SS_MEDIUM:
					case SS_STRONG:
					case SS_DESANN:
						if (client->ps.saberHolstered == 1 && client->saber[1].model[0])
						{
						}
						else if (client->saber[0].type == SABER_STAFF
							|| client->saber[0].type == SABER_BACKHAND
							|| client->saber[0].type == SABER_STAFF_UNSTABLE
							|| client->saber[0].type == SABER_STAFF_THIN
							|| client->saber[0].type == SABER_ASBACKHAND
							|| client->saber[0].type == SABER_STAFF_MAUL
							|| client->saber[0].type == SABER_ELECTROSTAFF)
						{
							if (client->ps.saberHolstered == 2)
							{
								//turn on all blades
								G_Sound(ent, CHAN_WEAPON, client->saber[0].soundOn);
								client->ps.saberHolstered = 0;
							}
						}
						else if (client->ps.saberHolstered > 0)
						{
							//turn on all blades
							G_Sound(ent, CHAN_WEAPON, client->saber[0].soundOn);
							client->ps.saberHolstered = 0;
						}
						break;
					case SS_DUAL:
						if (client->ps.saberHolstered == 1 && client->saber[1].model[0])
						{
							//turn on second saber
							G_Sound(ent, CHAN_WEAPON, client->saber[1].soundOn);
							client->ps.saberHolstered = 0;
						}
						else if (client->saber[0].type == SABER_STAFF
							|| client->saber[0].type == SABER_BACKHAND
							|| client->saber[0].type == SABER_STAFF_UNSTABLE
							|| client->saber[0].type == SABER_STAFF_THIN
							|| client->saber[0].type == SABER_ASBACKHAND
							|| client->saber[0].type == SABER_STAFF_MAUL
							|| client->saber[0].type == SABER_ELECTROSTAFF)
						{
							if (client->ps.saberHolstered == 2)
							{
								//turn on all blades
								G_Sound(ent, CHAN_WEAPON, client->saber[0].soundOn);
								client->ps.saberHolstered = 0;
							}
						}
						else if (client->ps.saberHolstered == 2)
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, client->saber[0].soundOn);
							client->ps.saberHolstered = 0;
						}
						break;
					case SS_STAFF:
						if (client->ps.saberHolstered == 1 && client->saber[1].model[0]) // right saber staff blade
						{
						}
						else if (client->ps.saberHolstered == 2 && client->saber[0].model[0])
						{
							//turn on first
							G_Sound(ent, CHAN_WEAPON, client->saber[0].soundOn);
							client->ps.saberHolstered = 0;
						}
						break;
					default:;
					}
				}
			}

			if (ucmd->buttons & BUTTON_ATTACK)
			{
				if (!(client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK))
				{
					client->ps.ManualBlockingFlags |= 1 << HOLDINGBLOCKANDATTACK;
					client->ps.ManualMBlockingTime = level.time;
				}
				ucmd->buttons &= ~BUTTON_ATTACK;

				if (g_npcspskill.integer == 0) //easy blocking
				{
					// started function
					if (client->ps.ManualblockStartTime <= 0) //fresh start
					{
						// They just pressed block. Mark the time...
						client->ps.ManualblockStartTime = level.time;

						if (!(client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING))
						{
							client->ps.ManualBlockingFlags |= 1 << PERFECTBLOCKING; // activate the function
						}
					}
					else
					{
						if (client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING && level.time - client->ps.
							ManualblockStartTime >= 500)
						{
							// Been holding block for too long....Turn off
							client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
						}
					}
				}
				else if (g_npcspskill.integer == 1) // medium blocking
				{
					if (client->ps.ManualblockStartTime <= 0) //fresh start
					{
						// They just pressed block. Mark the time...
						client->ps.ManualblockStartTime = level.time;

						if (!(client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING))
						{
							client->ps.ManualBlockingFlags |= 1 << PERFECTBLOCKING; // activate the function
						}
					}
					else
					{
						if (client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING && level.time - client->ps.
							ManualblockStartTime >= 200)
						{
							// Been holding block for too long....Turn off
							client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
						}
					}
				}
				else // hard blocking
				{
					if (client->ps.ManualblockStartTime <= 0) //fresh start
					{
						// They just pressed block. Mark the time...
						client->ps.ManualblockStartTime = level.time;

						if (!(client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING))
						{
							client->ps.ManualBlockingFlags |= 1 << PERFECTBLOCKING; // activate the function
						}
					}
					else
					{
						if (client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING && level.time - client->ps.
							ManualblockStartTime >= 80)
						{
							// Been holding block for too long....Turn off
							client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
						}
					}
				}

				if (client->ps.pm_flags & PMF_ACCURATE_MISSILE_BLOCK_HELD)
				{
					// started function
					if (client->ps.BoltblockStartTime <= 0) //fresh start
					{
						// They just pressed block. Mark the time...
						client->ps.BoltblockStartTime = level.time; //start the timer

						if (!(client->ps.ManualBlockingFlags & 1 << MBF_ACCURATEMISSILEBLOCKING))
						{
							client->ps.ManualBlockingFlags |= 1 << MBF_ACCURATEMISSILEBLOCKING; // activate the function
						}
					}
					else if (client->ps.ManualBlockingFlags & 1 << MBF_ACCURATEMISSILEBLOCKING && level.time - client->
						ps.BoltblockStartTime >= 3000)
					{
						// Been holding block for too long....let go.
						client->ps.ManualBlockingFlags &= ~(1 << MBF_ACCURATEMISSILEBLOCKING);
					}
				}
				else
				{
					// not doing it so reset
					client->ps.BoltblockStartTime = 0;
					client->ps.ManualBlockingFlags &= ~(1 << MBF_ACCURATEMISSILEBLOCKING);
				}
			}
			else
			{
				// No longer pressed, but we still need to make sure they are not spamming.
				client->ps.ManualblockStartTime = 0;
				client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
				client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
				client->ps.ManualBlockingFlags &= ~(1 << MBF_ACCURATEMISSILEBLOCKING);
			}
		}
		else
		{
			client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
			client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
			client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
			client->ps.ManualBlockingFlags &= ~(1 << MBF_ACCURATEMISSILEBLOCKING);
			client->ps.userInt3 &= ~(1 << FLAG_BLOCKING);
			client->ps.ManualBlockingTime = 0; //Blocking time 1 on
			client->ps.ManualMBlockingTime = 0;
		}
	}

	if (manual_meleeblocking(ent))
	{
		if (client->ps.MeleeblockStartTime <= 0 && level.time - client->ps.MeleeblockLastStartTime >= 1300)
		{
			// They just pressed block. Mark the time... 3000 wait between allowed presses.
			client->ps.MeleeblockStartTime = level.time; //Blocking 2
			client->ps.MeleeblockLastStartTime = level.time; //Blocking 3

			if (!(client->ps.ManualBlockingFlags & 1 << MBF_MELEEBLOCK))
			{
				client->ps.ManualBlockingFlags |= 1 << MBF_MELEEBLOCK;
			}
		}
		else
		{
			if (level.time - client->ps.MeleeblockStartTime >= 220) //Blocking 3
			{
				// When block was pressed, wait 200 before letting go of block.
				client->ps.MeleeblockStartTime = 0; //Blocking 2
				client->ps.ManualBlockingFlags &= ~(1 << MBF_MELEEBLOCK);
			}
		}
	}
	else
	{
		client->ps.ManualBlockingFlags &= ~(1 << MBF_MELEEBLOCK);
	}

	if (manual_melee_dodging(ent))
	{
		if (client->ps.DodgeStartTime <= 0 && level.time - client->ps.DodgeLastStartTime >= 1000)
		{
			// They just pressed block. Mark the time... 3000 wait between allowed presses.
			client->ps.DodgeStartTime = level.time; //Blocking 2
			client->ps.DodgeLastStartTime = level.time; //Blocking 3

			if (!(client->ps.ManualBlockingFlags & 1 << MBF_MELEEDODGE))
			{
				client->ps.ManualBlockingFlags |= 1 << MBF_MELEEDODGE;
			}
		}
		else
		{
			if (level.time - client->ps.DodgeStartTime >= 800) //Blocking 3
			{
				// When block was pressed, wait 200 before letting go of block.
				client->ps.DodgeStartTime = 0; //Blocking 2
				client->ps.ManualBlockingFlags &= ~(1 << MBF_MELEEDODGE);
			}
		}
	}
	else
	{
		client->ps.ManualBlockingFlags &= ~(1 << MBF_MELEEDODGE);
	}

	if (ent->r.svFlags & SVF_BOT &&
		client->ps.weapon == WP_SABER &&
		!BG_SabersOff(&client->ps) &&
		!PM_SaberInMassiveBounce(client->ps.torsoAnim) &&
		!PM_SaberInBashedAnim(client->ps.torsoAnim) &&
		!PM_Saberinstab(client->ps.saber_move))
	{
		if (manual_npc_saberblocking(ent))
		{
			if (!(client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING))
			{
				client->ps.ManualBlockingFlags |= 1 << MBF_NPCBLOCKING;
			}
		}
		else
		{
			client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCBLOCKING);
		}
		if (manual_npc_kick_absorbing(ent))
		{
			if (!(client->ps.ManualBlockingFlags & 1 << MBF_NPCKICKBLOCK))
			{
				client->ps.ManualBlockingFlags |= 1 << MBF_NPCKICKBLOCK;
			}
		}
		else
		{
			client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCKICKBLOCK);
		}
	}

	if (ent->r.svFlags & SVF_BOT)
	{
		if (client && client->cloak_is_charging)
		{
			if (client->ps.cloakCharcgestartTime <= 0 && level.time - client->ps.cloakchargelaststartTime >= 5000)
			{
				// They just started charging cloak lets restrict a restart until its full
				// to stop the bots spamming it
				client->ps.cloakCharcgestartTime = level.time;
				client->ps.cloakchargelaststartTime = level.time;

				if (!(client->ps.communicatingflags & 1 << CLOAK_CHARGE_RESTRICTION))
				{
					client->ps.communicatingflags |= 1 << CLOAK_CHARGE_RESTRICTION;
				}
			}
			else
			{
				if (level.time - client->ps.cloakCharcgestartTime >= 6000)
				{
					// When kick was pressed, wait 3000 before letting go.
					client->ps.cloakCharcgestartTime = 0;
					client->ps.communicatingflags &= ~(1 << CLOAK_CHARGE_RESTRICTION);
				}
			}
		}
		else
		{
			client->ps.communicatingflags &= ~(1 << CLOAK_CHARGE_RESTRICTION);
		}
	}

	// Activate the surrenderingTime flags
	if (!(ent->r.svFlags & SVF_BOT))
	{
		if (IsPressingDashButton(ent)
			&& !PM_kick_move(ent->client->ps.saber_move)
			&& !PM_SaberInAttack(ent->client->ps.saber_move))
		{
			if (client->Dash_Count <= 2)
			{
				if (client->ps.dashstartTime <= 0 && level.time - client->ps.dashlaststartTime >= 100)
				{
					// They just pressed dash. Mark the time... 3000 wait between allowed dash.
					client->ps.dashstartTime = level.time;
					client->ps.dashlaststartTime = level.time;
					client->Dash_Count++;

					if (!(client->ps.communicatingflags & 1 << DASHING))
					{
						client->ps.communicatingflags |= 1 << DASHING;
					}
				}
				else
				{
					if (level.time - client->ps.dashlaststartTime >= 10)
					{
						// When dash was pressed, wait 3000 before letting go of dash.
						client->ps.dashstartTime = 0;
						client->ps.communicatingflags &= ~(1 << DASHING);
					}
				}
			}
			else
			{
				if (client->ps.dashstartTime <= 0 && level.time - client->ps.dashlaststartTime >= 2500)
				{
					// They just pressed dash. Mark the time... 8000 wait between allowed dash.
					client->ps.dashstartTime = level.time;
					client->ps.dashlaststartTime = level.time;

					if (!(client->ps.communicatingflags & 1 << DASHING))
					{
						client->ps.communicatingflags |= 1 << DASHING;
					}
				}
				else
				{
					if (level.time - client->ps.dashlaststartTime >= 2500)
					{
						// When dash was pressed, wait 3000 before letting go of dash.
						client->ps.dashstartTime = 0;
						client->Dash_Count = 0;
						client->ps.communicatingflags &= ~(1 << DASHING);
					}
				}
			}
		}
		else if (IsPressingKickButton(ent))
		{
			if (client->ps.kickstartTime <= 0 && level.time - client->ps.kicklaststartTime >= 1500)
			{
				// They just pressed kick. Mark the time... 5000 wait between allowed kicks.
				client->ps.kickstartTime = level.time;
				client->ps.kicklaststartTime = level.time;

				if (!(client->ps.communicatingflags & 1 << KICKING))
				{
					client->ps.communicatingflags |= 1 << KICKING;
				}
			}
			else
			{
				if (level.time - client->ps.kicklaststartTime >= 500)
				{
					// When kick was pressed, wait 3000 before letting go.
					client->ps.kickstartTime = 0;
					client->ps.communicatingflags &= ~(1 << KICKING);
				}
			}
		}
		else if (client->ps.weapon == WP_STUN_BATON && ucmd->buttons & BUTTON_ALT_ATTACK)
		{
			client->ps.grappletimeplayer = level.time;

			if (client->ps.grapplestartTime <= 0 && level.time - client->ps.grapplelaststartTime >= 3000)
			{
				// They just pressed grapple. Mark the time... 3000 wait between allowed grapple.
				client->ps.grapplestartTime = level.time;
				client->ps.grapplelaststartTime = level.time;

				if (!(client->ps.communicatingflags & 1 << STUNNING))
				{
					client->ps.communicatingflags |= 1 << STUNNING;
				}
			}
			else
			{
				if (level.time - client->ps.grapplestartTime >= 1500)
				{
					// When grapple was pressed, wait 1500 before letting go of grapple.
					client->ps.grapplestartTime = 0;
					client->ps.communicatingflags &= ~(1 << STUNNING);
				}
			}
		}
		else if (Is_Oversized_Gunner(ent))
		{
			if (!(client->ps.communicatingflags & 1 << OVERSIZEDGUNNER))
			{
				client->ps.communicatingflags |= 1 << OVERSIZEDGUNNER;
			}

			if ((client->ps.saberLockTime > level.time) && (ucmd->rightmove > 0 || ucmd->forwardmove > 0))
			{//just in case i missed a saber user
				if (!(client->ps.communicatingflags & 1 << CF_SABERLOCK_ADVANCE))
				{
					client->ps.communicatingflags |= 1 << CF_SABERLOCK_ADVANCE;
				}
			}
			else
			{
				client->ps.communicatingflags &= ~(1 << CF_SABERLOCK_ADVANCE);
			}
		}
		else if (Is_Undersized_Jedi(ent))
		{
			if (!(client->ps.communicatingflags & 1 << UNDERSIZEDJEDI))
			{
				client->ps.communicatingflags |= 1 << UNDERSIZEDJEDI;
			}

			if ((client->ps.saberLockTime > level.time) && (ucmd->rightmove > 0 || ucmd->forwardmove > 0))
			{ //just in case i missed a saber user
				if (!(client->ps.communicatingflags & 1 << CF_SABERLOCK_ADVANCE))
				{
					client->ps.communicatingflags |= 1 << CF_SABERLOCK_ADVANCE;
				}
			}
			else
			{
				client->ps.communicatingflags &= ~(1 << CF_SABERLOCK_ADVANCE);
			}
		}
		else if (Is_Undersized_Gunner(ent))
		{
			if (!(client->ps.communicatingflags & 1 << UNDERSIZEDGUNNER))
			{
				client->ps.communicatingflags |= 1 << UNDERSIZEDGUNNER;
			}

			if ((client->ps.saberLockTime > level.time) && (ucmd->rightmove > 0 || ucmd->forwardmove > 0))
			{ //just in case i missed a saber user
				if (!(client->ps.communicatingflags & 1 << CF_SABERLOCK_ADVANCE))
				{
					client->ps.communicatingflags |= 1 << CF_SABERLOCK_ADVANCE;
				}
			}
			else
			{
				client->ps.communicatingflags &= ~(1 << CF_SABERLOCK_ADVANCE);
			}
		}
		else if (client->ps.saberLockTime > level.time && !Is_Undersized_Gunner(ent) && !Is_Oversized_Gunner(ent))
		{
			if (!(client->ps.communicatingflags & 1 << CF_SABERLOCKING))
			{
				client->ps.communicatingflags |= 1 << CF_SABERLOCKING;
			}

			if (ucmd->rightmove > 0 || ucmd->forwardmove > 0)
			{
				if (!(client->ps.communicatingflags & 1 << CF_SABERLOCK_ADVANCE))
				{
					client->ps.communicatingflags |= 1 << CF_SABERLOCK_ADVANCE;
				}
			}
			else
			{
				client->ps.communicatingflags &= ~(1 << CF_SABERLOCK_ADVANCE);
			}
		}
		else
		{
			client->ps.respectingtime = 0;
			client->ps.gesturingtime = 0;
			client->ps.surrendertimeplayer = 0;
			client->ps.communicatingflags &= ~(1 << CF_SABERLOCK_ADVANCE);
			client->ps.communicatingflags &= ~(1 << CF_SABERLOCKING);
			client->ps.communicatingflags &= ~(1 << SURRENDERING);
			client->ps.communicatingflags &= ~(1 << RESPECTING);
			client->ps.communicatingflags &= ~(1 << GESTURING);
			client->ps.communicatingflags &= ~(1 << DASHING);
			client->ps.communicatingflags &= ~(1 << KICKING);
			client->ps.communicatingflags &= ~(1 << UNDERSIZEDJEDI);
			client->ps.communicatingflags &= ~(1 << OVERSIZEDGUNNER);
			client->ps.communicatingflags &= ~(1 << UNDERSIZEDGUNNER);
			if (client->ps.weapon != WP_STUN_BATON ||
				(client->ps.communicatingflags |= client->ps.grapplestartTime >= 3000))
			{
				client->ps.communicatingflags &= ~(1 << STUNNING);
			}
		}
	}

	if (client->doingThrow > level.time)
	{
		gentity_t* throwee = &g_entities[ent->client->throwingIndex];

		if (!throwee->inuse || !throwee->client || throwee->health < 1 ||
			throwee->client->sess.sessionTeam == TEAM_SPECTATOR ||
			throwee->client->ps.pm_flags & PMF_FOLLOW ||
			throwee->client->throwingIndex != ent->s.number)
		{
			client->doingThrow = 0;
			client->ps.forceHandExtend = HANDEXTEND_NONE;

			if (throwee->inuse && throwee->client)
			{
				throwee->client->ps.heldByClient = 0;
				throwee->client->beingThrown = 0;

				if (throwee->client->ps.forceHandExtend != HANDEXTEND_POSTTHROWN)
				{
					throwee->client->ps.forceHandExtend = HANDEXTEND_NONE;
				}
			}
		}
	}

	if (ucmd->buttons & BUTTON_BLOCK)
	{
		//blocking with saber
		client->ps.saberManualBlockingTime = level.time + FRAMETIME;
	}

	if (client->beingThrown > level.time)
	{
		gentity_t* thrower = &g_entities[ent->client->throwingIndex];

		if (!thrower->inuse || !thrower->client || thrower->health < 1 ||
			thrower->client->sess.sessionTeam == TEAM_SPECTATOR ||
			thrower->client->ps.pm_flags & PMF_FOLLOW ||
			thrower->client->throwingIndex != ent->s.number)
		{
			client->ps.heldByClient = 0;
			client->beingThrown = 0;

			if (client->ps.forceHandExtend != HANDEXTEND_POSTTHROWN)
			{
				client->ps.forceHandExtend = HANDEXTEND_NONE;
			}

			if (thrower->inuse && thrower->client)
			{
				thrower->client->doingThrow = 0;
				thrower->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
		}
		else if (thrower->inuse && thrower->client && thrower->ghoul2 &&
			trap->G2API_HaveWeGhoul2Models(thrower->ghoul2))
		{
#if 0
			int lHandBolt = trap->G2API_AddBolt(thrower->ghoul2, 0, "*l_hand");
			int pelBolt = trap->G2API_AddBolt(thrower->ghoul2, 0, "pelvis");

			if (lHandBolt != -1 && pelBolt != -1)
#endif
			{
				float pDif = 40.0f;
				vec3_t bolt_org, pBoltOrg;
				vec3_t tAngles;
				vec3_t vDif;
				vec3_t entDir, otherAngles;
				vec3_t fwd, right;

				//Always look at the thrower.
				VectorSubtract(thrower->client->ps.origin, ent->client->ps.origin, entDir);
				VectorCopy(ent->client->ps.viewangles, otherAngles);
				otherAngles[YAW] = vectoyaw(entDir);
				SetClientViewAngle(ent, otherAngles);

				VectorCopy(thrower->client->ps.viewangles, tAngles);
				tAngles[PITCH] = tAngles[ROLL] = 0;

				//Get the direction between the pelvis and position of the hand
#if 0
				mdxaBone_t boltMatrix, pBoltMatrix;

				trap->G2API_GetBoltMatrix(thrower->ghoul2, 0, lHandBolt, &boltMatrix, tAngles, thrower->client->ps.origin, level.time, 0, thrower->modelScale);
				bolt_org[0] = boltMatrix.matrix[0][3];
				bolt_org[1] = boltMatrix.matrix[1][3];
				bolt_org[2] = boltMatrix.matrix[2][3];

				trap->G2API_GetBoltMatrix(thrower->ghoul2, 0, pelBolt, &pBoltMatrix, tAngles, thrower->client->ps.origin, level.time, 0, thrower->modelScale);
				pBoltOrg[0] = pBoltMatrix.matrix[0][3];
				pBoltOrg[1] = pBoltMatrix.matrix[1][3];
				pBoltOrg[2] = pBoltMatrix.matrix[2][3];
#else //above tends to not work once in a while, for various reasons I suppose.
				VectorCopy(thrower->client->ps.origin, pBoltOrg);
				AngleVectors(tAngles, fwd, right, 0);
				bolt_org[0] = pBoltOrg[0] + fwd[0] * 8 + right[0] * pDif;
				bolt_org[1] = pBoltOrg[1] + fwd[1] * 8 + right[1] * pDif;
				bolt_org[2] = pBoltOrg[2];
#endif

				VectorSubtract(ent->client->ps.origin, bolt_org, vDif);
				if (VectorLength(vDif) > 32.0f && thrower->client->doingThrow - level.time < 4500)
				{
					//the hand is too far away, and can no longer hold onto us, so escape.
					ent->client->ps.heldByClient = 0;
					ent->client->beingThrown = 0;
					thrower->client->doingThrow = 0;

					thrower->client->ps.forceHandExtend = HANDEXTEND_NONE;
					G_EntitySound(thrower, CHAN_VOICE, G_SoundIndex("*pain25.wav"));

					ent->client->ps.forceDodgeAnim = 2;
					ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
					ent->client->ps.forceHandExtendTime = level.time + 500;
					ent->client->ps.velocity[2] = 400;
					if (ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
					{
						//short burst
						G_Sound(ent, CHAN_BODY, G_SoundIndex("sound/weapons/force/jumpsmall.mp3"));
					}
					else
					{
						//holding it
						G_PreDefSound(ent->client->ps.origin, PDSOUND_FORCEJUMP);
					}
				}
				else if (client->beingThrown - level.time < 4000)
				{
					//step into the next part of the throw, and go flying back
					float vScale = 400.0f;
					ent->client->ps.forceHandExtend = HANDEXTEND_POSTTHROWN;
					ent->client->ps.forceHandExtendTime = level.time + 1200;
					ent->client->ps.forceDodgeAnim = 0;

					thrower->client->ps.forceHandExtend = HANDEXTEND_POSTTHROW;
					thrower->client->ps.forceHandExtendTime = level.time + 200;

					ent->client->ps.heldByClient = 0;

					ent->client->ps.heldByClient = 0;
					ent->client->beingThrown = 0;
					thrower->client->doingThrow = 0;

					AngleVectors(thrower->client->ps.viewangles, vDif, 0, 0);
					ent->client->ps.velocity[0] = vDif[0] * vScale;
					ent->client->ps.velocity[1] = vDif[1] * vScale;
					ent->client->ps.velocity[2] = 400;

					G_EntitySound(ent, CHAN_VOICE, G_SoundIndex("*pain100.wav"));
					G_EntitySound(thrower, CHAN_VOICE, G_SoundIndex("*jump1.wav"));

					//Set the thrower as the "other killer", so if we die from fall/impact damage he is credited.
					ent->client->ps.otherKiller = thrower->s.number;
					ent->client->ps.otherKillerTime = level.time + 8000;
					ent->client->ps.otherKillerDebounceTime = level.time + 100;
					ent->client->otherKillerMOD = MOD_FALLING;
					ent->client->otherKillerVehWeapon = 0;
					ent->client->otherKillerWeaponType = WP_NONE;
				}
				else
				{
					//see if we can move to be next to the hand.. if it's not clear, break the throw.
					vec3_t intendedOrigin;
					trace_t tr;
					trace_t tr2;

					VectorSubtract(bolt_org, pBoltOrg, vDif);
					VectorNormalize(vDif);

					VectorClear(ent->client->ps.velocity);
					intendedOrigin[0] = pBoltOrg[0] + vDif[0] * pDif;
					intendedOrigin[1] = pBoltOrg[1] + vDif[1] * pDif;
					intendedOrigin[2] = thrower->client->ps.origin[2];

					trap->Trace(&tr, intendedOrigin, ent->r.mins, ent->r.maxs, intendedOrigin, ent->s.number,
						ent->clipmask, qfalse, 0, 0);
					trap->Trace(&tr2, ent->client->ps.origin, ent->r.mins, ent->r.maxs, intendedOrigin, ent->s.number,
						CONTENTS_SOLID, qfalse, 0, 0);

					if (tr.fraction == 1.0 && !tr.startsolid && tr2.fraction == 1.0 && !tr2.startsolid)
					{
						VectorCopy(intendedOrigin, ent->client->ps.origin);

						if (client->beingThrown - level.time < 4800)
						{
							ent->client->ps.heldByClient = thrower->s.number + 1;
						}
					}
					else
					{
						//if the guy can't be put here then it's time to break the throw off.
						ent->client->ps.heldByClient = 0;
						ent->client->beingThrown = 0;
						thrower->client->doingThrow = 0;

						thrower->client->ps.forceHandExtend = HANDEXTEND_NONE;
						G_EntitySound(thrower, CHAN_VOICE, G_SoundIndex("*pain25.wav"));

						ent->client->ps.forceDodgeAnim = 2;
						ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
						ent->client->ps.forceHandExtendTime = level.time + 500;
						ent->client->ps.velocity[2] = 400;
						if (ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
						{
							//short burst
							G_Sound(ent, CHAN_BODY, G_SoundIndex("sound/weapons/force/jumpsmall.mp3"));
						}
						else
						{
							//holding it
							G_PreDefSound(ent->client->ps.origin, PDSOUND_FORCEJUMP);
						}
					}
				}
			}
		}
	}
	else if (ent->client->ps.heldByClient)
	{
		ent->client->ps.heldByClient = 0;
	}

	// set up for pmove
	old_event_sequence = client->ps.eventSequence;

	memset(&pmove, 0, sizeof pmove);

	if (client && ent->flags & FL_FORCE_GESTURE)
	{
		ent->flags &= ~FL_FORCE_GESTURE;
		ent->client->pers.cmd.buttons |= BUTTON_GESTURE;
	}

	if (ent->client && ent->client->ps.fallingToDeath &&
		level.time - FALL_FADE_TIME > ent->client->ps.fallingToDeath)
	{
		//die!
		if (ent->health > 0)
		{
			gentity_t* otherKiller = ent;
			if (ent->client->ps.otherKillerTime > level.time &&
				ent->client->ps.otherKiller != ENTITYNUM_NONE)
			{
				otherKiller = &g_entities[ent->client->ps.otherKiller];

				if (!otherKiller->inuse)
				{
					otherKiller = ent;
				}
			}
			G_Damage(ent, otherKiller, otherKiller, NULL, ent->client->ps.origin, 9999, DAMAGE_NO_PROTECTION,
				MOD_FALLING);

			G_MuteSound(ent->s.number, CHAN_VOICE); //stop screaming, because you are dead!
		}
	}

	if (ent->client->ps.otherKillerTime > level.time &&
		ent->client->ps.groundEntityNum != ENTITYNUM_NONE &&
		ent->client->ps.otherKillerDebounceTime < level.time)
	{
		ent->client->ps.otherKillerTime = 0;
		ent->client->ps.otherKiller = ENTITYNUM_NONE;
	}
	else if (ent->client->ps.otherKillerTime > level.time &&
		ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		if (ent->client->ps.otherKillerDebounceTime < level.time + 100)
		{
			ent->client->ps.otherKillerDebounceTime = level.time + 100;
		}
	}

	//NOTE: can't put USE here *before* PMove!!
	if (ent->client->ps.useDelay > level.time
		&& ent->client->ps.m_iVehicleNum)
	{
		//when in a vehicle, debounce the use...
		ucmd->buttons &= ~BUTTON_USE;
		ucmd->buttons &= ~BUTTON_BOTUSE;
	}

	//FIXME: need to do this before check to avoid walls and cliffs (or just cliffs?)
	G_AddPushVecToUcmd(ent, ucmd);

	//play/stop any looping sounds tied to controlled movement
	G_CheckMovingLoopingSounds(ent, ucmd);

	pmove.ps = &client->ps;
	pmove.cmd = *ucmd;
	if (pmove.ps->pm_type == PM_DEAD)
	{
		pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else if (ent->r.svFlags & SVF_BOT)
	{
		pmove.tracemask = MASK_PLAYERSOLID | CONTENTS_MONSTERCLIP;
	}
	else
	{
		pmove.tracemask = MASK_PLAYERSOLID;
	}
	pmove.trace = SV_PMTrace;
	pmove.pointcontents = trap->PointContents;
	pmove.debugLevel = g_debugMove.integer;
	pmove.noFootsteps = (dmflags.integer & DF_NO_FOOTSTEPS) > 0;

	pmove.pmove_fixed = pmove_fixed.integer | client->pers.pmoveFixed;
	pmove.pmove_msec = pmove_msec.integer;
	pmove.pmove_float = pmove_float.integer;

	pmove.animations = bgAllAnims[ent->localAnimIndex].anims; //NULL;

	//rww - bgghoul2
	pmove.ghoul2 = NULL;

#ifdef _DEBUG
	if (g_disableServerG2.integer)
	{
	}
	else
#endif
		if (ent->ghoul2)
		{
			if (ent->localAnimIndex > 1)
			{
				//if it isn't humanoid then we will be having none of this.
				pmove.ghoul2 = NULL;
			}
			else
			{
				pmove.ghoul2 = ent->ghoul2;
				pmove.g2Bolts_LFoot = trap->G2API_AddBolt(ent->ghoul2, 0, "*l_leg_foot");
				pmove.g2Bolts_RFoot = trap->G2API_AddBolt(ent->ghoul2, 0, "*r_leg_foot");
			}
		}

	//point the saber data to the right place
#if 0
	k = 0;
	while (k < MAX_SABERS)
	{
		if (ent->client->saber[k].model[0])
		{
			pm.saber[k] = &ent->client->saber[k];
		}
		else
		{
			pm.saber[k] = NULL;
		}
		k++;
	}
#endif

	//I'll just do this every frame in case the scale changes in realtime (don't need to update the g2 inst for that)
	VectorCopy(ent->modelScale, pmove.modelScale);
	//rww end bgghoul2

	pmove.gametype = level.gametype;
	pmove.stepSlideFix = g_stepSlideFix.integer;

	pmove.noSpecMove = g_noSpecMove.integer;

	pmove.nonHumanoid = ent->localAnimIndex > 0;

	VectorCopy(client->ps.origin, client->oldOrigin);

	//Set up bg entity data
	pmove.baseEnt = (bgEntity_t*)g_entities;
	pmove.entSize = sizeof(gentity_t);

	if (ent->client->ps.saberLockTime > level.time)
	{
		gentity_t* blockOpp = &g_entities[ent->client->ps.saberLockEnemy];

		if (blockOpp && blockOpp->inuse && blockOpp->client)
		{
			vec3_t lockDir, lockAng;

			VectorSubtract(blockOpp->r.currentOrigin, ent->r.currentOrigin, lockDir);
			vectoangles(lockDir, lockAng);
			SetClientViewAngle(ent, lockAng);
		}

		if (ent->client->pers.cmd.buttons & BUTTON_BLOCK && ent->client->pers.cmd.buttons & BUTTON_ALT_ATTACK && ent->client->pers.cmd.forwardmove < 0)
		{//breaking out of the saberlock!
			ent->client->ps.saberLockFrame = 0;
		}
		else if (ent->client->ps.saberLockHitCheckTime < level.time)
		{//have moved to next frame since last lock push
			int strength = 0;

			if (ent->r.svFlags & SVF_BOT)
			{
				ent->client->ps.saberLockHitCheckTime = level.time + 25; //check for AI pushes much slower.

				if (ent->client->ps.saberLockHitIncrementTime < level.time)
				{//have moved to next frame since last saberlock attack button press
					ent->client->ps.saberLockHitIncrementTime = level.time + 150;//so we don't register an attack key press more than once per server frame

					if (blockOpp && blockOpp->r.svFlags & SVF_BOT)
					{
						switch (ent->client->ps.fd.saberAnimLevel)
						{
						case SS_FAST:
							strength = Q_irand(1, 2);
							break;
						case SS_MEDIUM:
						case SS_TAVION:
						case SS_DUAL:
						case SS_STAFF:
							strength = Q_irand(2, 3);
							break;
						case SS_STRONG:
						case SS_DESANN:
							strength = Q_irand(3, 4); 
							break;
						}
					}
					else 
					{
						if (ent->client->pers.botclass == BCLASS_DESANN || ent->client->pers.botclass == BCLASS_LUKE)
						{
							strength = Q_irand(2, 3);
						}
						else
						{
							strength = Q_irand(1, 2);
						}
					}

					ent->client->ps.saberLockHits += strength;
				}

				if (ent->client->ps.saberLockHits > 0)
				{
					if (!ent->client->ps.saberLockAdvance)
					{
						ent->client->ps.saberLockHits--;
					}
					ent->client->ps.saberLockAdvance = qtrue;
				}
			}
			else
			{
				ent->client->ps.saberLockHitCheckTime = level.time + 25;//so we don't push more than once per server frame

				if (ent->client->ps.communicatingflags & 1 << CF_SABERLOCK_ADVANCE)
				{
					if (ent->client->ps.saberLockHitIncrementTime < level.time)
					{//have moved to next frame since last saberlock attack button press
						ent->client->ps.saberLockHitIncrementTime = level.time + 50;//so we don't register an attack key press more than once per server frame

						strength += ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] + Q_irand(1, g_attackskill.integer) + Q_irand(0, ent->client->buttons & BUTTON_ATTACK);

						ent->client->ps.saberLockHits += strength;
					}
				}
				else
				{
					if (ent->client->ps.saberLockHitIncrementTime < level.time)
					{//have moved to next frame since last saberlock attack button press
						ent->client->ps.saberLockHitIncrementTime = level.time + 50;//so we don't register an attack key press more than once per server frame

						strength = Q_irand(0, ent->client->buttons & BUTTON_ATTACK);

						ent->client->ps.saberLockHits += strength;
					}
				}

				if (ent->client->ps.saberLockHits > 0 && ent->client->ps.communicatingflags & 1 << CF_SABERLOCK_ADVANCE)
				{
					if (!ent->client->ps.saberLockAdvance)
					{
						ent->client->ps.saberLockHits--;
					}
					ent->client->ps.saberLockAdvance = qtrue;
				}
			}
		}
	}
	else
	{
		ent->client->ps.saberLockFrame = 0;
		//check for taunt
		if (pmove.cmd.generic_cmd == GENCMD_ENGAGE_DUEL && (level.gametype == GT_DUEL || level.gametype ==
			GT_POWERDUEL))
		{
			//already in a duel, make it a taunt command
			pmove.cmd.buttons |= BUTTON_GESTURE;
		}

		if (pmove.cmd.generic_cmd == GENCMD_TAUNT)
		{
			//already in a duel, make it a taunt command
			pmove.cmd.buttons |= BUTTON_GESTURE;
		}

		if (pmove.cmd.generic_cmd == GENCMD_GLOAT)
		{
			//already in a duel, make it a taunt command
			pmove.cmd.buttons |= BUTTON_GLOAT;
		}

		if (pmove.cmd.generic_cmd == GENCMD_BOW)
		{
			//already in a duel, make it a taunt command
			pmove.cmd.buttons |= BUTTON_RESPECT;
		}

		if (pmove.cmd.generic_cmd == GENCMD_FLOURISH)
		{
			//already in a duel, make it a taunt command
			pmove.cmd.buttons |= BUTTON_FLOURISH;
		}
	}

	if (ent->s.number >= MAX_CLIENTS)
	{
		VectorCopy(ent->r.mins, pmove.mins);
		VectorCopy(ent->r.maxs, pmove.maxs);
#if 1
		if (ent->s.NPC_class == CLASS_VEHICLE &&
			ent->m_pVehicle)
		{
			if (ent->m_pVehicle->m_pPilot)
			{
				//vehicles want to use their last pilot ucmd I guess
				if (level.time - ent->m_pVehicle->m_ucmd.serverTime > 2000)
				{
					//Previous owner disconnected, maybe
					ent->m_pVehicle->m_ucmd.serverTime = level.time;
					ent->client->ps.commandTime = level.time - 100;
					msec = 100;
				}

				memcpy(&pmove.cmd, &ent->m_pVehicle->m_ucmd, sizeof(usercmd_t));

				//no veh can strafe
				pmove.cmd.rightmove = 0;
				//no crouching or jumping!
				pmove.cmd.upmove = 0;

				//NOTE: button presses were getting lost!
				assert(g_entities[ent->m_pVehicle->m_pPilot->s.number].client);
				pmove.cmd.buttons = g_entities[ent->m_pVehicle->m_pPilot->s.number].client->pers.cmd.buttons & (
					BUTTON_ATTACK | BUTTON_ALT_ATTACK);
			}
			if (ent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)
			{
				if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					//ATST crushes anything underneath it
					gentity_t* under = &g_entities[ent->client->ps.groundEntityNum];
					if (under && under->health && under->takedamage)
					{
						vec3_t down = { 0, 0, -1 };
						//FIXME: we'll be doing traces down from each foot, so we'll have a real impact origin
						G_Damage(under, ent, ent, down, under->r.currentOrigin, 100, 0, MOD_CRUSH);
					}
				}
			}
		}
#endif
	}

	Pmove(&pmove);

	if (ent->client->solidHack)
	{
		if (ent->client->solidHack > level.time)
		{
			//whee!
			ent->r.contents = 0;
		}
		else
		{
			ent->r.contents = CONTENTS_BODY;
			ent->client->solidHack = 0;
		}
	}

	if (ent->NPC)
	{
		VectorCopy(ent->client->ps.viewangles, ent->r.currentAngles);
	}

	if (pmove.checkDuelLoss)
	{
		if (pmove.checkDuelLoss > 0 && (pmove.checkDuelLoss <= MAX_CLIENTS || (pmove.checkDuelLoss < (MAX_GENTITIES - 1) && g_entities[pmove.checkDuelLoss - 1].s.eType == ET_NPC)))
		{
			gentity_t* clientLost = &g_entities[pmove.checkDuelLoss - 1];

			if (clientLost && clientLost->inuse && clientLost->client)
			{
				saberKnockOutOfHand(&g_entities[clientLost->client->ps.saberEntityNum], clientLost, vec3_origin);
			}
		}

		pmove.checkDuelLoss = 0;
	}

	if (ent->client->ps.groundEntityNum < ENTITYNUM_WORLD)
	{
		//standing on an ent
		gentity_t* groundEnt = &g_entities[ent->client->ps.groundEntityNum];
		if (groundEnt
			&& groundEnt->s.eType == ET_NPC
			&& groundEnt->s.NPC_class == CLASS_VEHICLE
			&& groundEnt->inuse
			&& groundEnt->health > 0
			&& groundEnt->m_pVehicle)
		{
			//standing on a valid, living vehicle
			if (!groundEnt->client->ps.speed
				&& groundEnt->m_pVehicle->m_ucmd.upmove > 0)
			{
				//a vehicle that's trying to take off!
				//just kill me
				vec3_t up = { 0, 0, 1 };
				G_Damage(ent, NULL, NULL, up, ent->r.currentOrigin, 9999999, DAMAGE_NO_PROTECTION, MOD_CRUSH);
				return;
			}
		}
	}

	if (pmove.cmd.generic_cmd &&
		(pmove.cmd.generic_cmd != ent->client->lastGenCmd || ent->client->lastGenCmdTime < level.time))
	{
		ent->client->lastGenCmd = pmove.cmd.generic_cmd;

		if (pmove.cmd.generic_cmd == GENCMD_SABERATTACKCYCLE)
		{
			//saber style toggling debounces debounces faster than normal debouncable commands.
			ent->client->lastGenCmdTime = level.time + 100;
		}
		else if (pmove.cmd.generic_cmd != GENCMD_FORCE_THROW &&
			pmove.cmd.generic_cmd != GENCMD_FORCE_PULL &&
			pmove.cmd.generic_cmd != GENCMD_FORCE_SPEED)
		{
			//these are the only two where you wouldn't care about a delay between
			ent->client->lastGenCmdTime = level.time + 300; //default 100ms debounce between issuing the same command.
		}

		switch (pmove.cmd.generic_cmd)
		{
		case 0:
			break;
		case GENCMD_SABERSWITCH:
			Cmd_ToggleSaber_f(ent);
			break;
		case GENCMD_ENGAGE_DUEL:
			if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
			{
				G_SetTauntAnim(ent, TAUNT_TAUNT);
			}
			else
			{
				Cmd_EngageDuel_f(ent);
			}
			break;
		case GENCMD_FORCE_HEAL:
			ForceHeal(ent);
			break;
		case GENCMD_FORCE_SPEED:
			ForceSpeed(ent, 0);
			break;
		case GENCMD_FORCE_THROW:
			ForceThrow(ent, qfalse);
			break;
		case GENCMD_FORCE_PULL:
			ForceThrow(ent, qtrue);
			break;
		case GENCMD_FORCE_DISTRACT:
			ForceTelepathy(ent);
			break;
		case GENCMD_FORCE_RAGE:
			ForceRage(ent);
			break;
		case GENCMD_FORCE_PROTECT:
			ForceProtect(ent);
			break;
		case GENCMD_FORCE_ABSORB:
			ForceAbsorb(ent);
			break;
		case GENCMD_FORCE_HEALOTHER:
			ForceTeamHeal(ent);
			break;
		case GENCMD_FORCE_FORCEPOWEROTHER:
			ForceTeamForceReplenish(ent);
			break;
		case GENCMD_FORCE_SEEING:
			ForceSeeing(ent);
			break;

			/////////////////////////////////////////////////////////////////////////////////////////
		case GENCMD_USE_SEEKER: //HI_SEEKER
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_SEEKER &&
				G_ItemUsable(&ent->client->ps, HI_SEEKER))
			{
				ItemUse_Seeker(ent);
				G_AddEvent(ent, EV_USE_ITEM0 + HI_SEEKER, 0);
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SEEKER); // TAKE AWAY AFTER USE ?
			}
			break;
		case GENCMD_USE_SHIELD: //HI_SHIELD
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_SHIELD &&
				G_ItemUsable(&ent->client->ps, HI_SHIELD))
			{
				ItemUse_Shield(ent);
				G_AddEvent(ent, EV_USE_ITEM0 + HI_SHIELD, 0);
			}
			break;
		case GENCMD_USE_MEDPAC: //HI_MEDPAC
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_MEDPAC &&
				G_ItemUsable(&ent->client->ps, HI_MEDPAC))
			{
				ItemUse_MedPack(ent);
				G_AddEvent(ent, EV_USE_ITEM0 + HI_MEDPAC, 0);
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_MEDPAC); // TAKE AWAY AFTER USE ?
			}
			break;
		case GENCMD_USE_MEDPAC_BIG:  //HI_MEDPAC_BIG
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_MEDPAC_BIG &&
				G_ItemUsable(&ent->client->ps, HI_MEDPAC_BIG))
			{
				ItemUse_MedPack_Big(ent);
				G_AddEvent(ent, EV_USE_ITEM0 + HI_MEDPAC_BIG, 0);
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_MEDPAC_BIG); // TAKE AWAY AFTER USE ?
			}
			break;
		case GENCMD_USE_ELECTROBINOCULARS: //HI_BINOCULARS
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_BINOCULARS &&
				G_ItemUsable(&ent->client->ps, HI_BINOCULARS))
			{
				ItemUse_Binoculars(ent);
				if (ent->client->ps.zoomMode == 0)
				{
					G_AddEvent(ent, EV_USE_ITEM0 + HI_BINOCULARS, 1);
				}
				else
				{
					G_AddEvent(ent, EV_USE_ITEM0 + HI_BINOCULARS, 2);
				}
			}
			break;
		case GENCMD_USE_SENTRY: //HI_SENTRY_GUN
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_SENTRY_GUN &&
				G_ItemUsable(&ent->client->ps, HI_SENTRY_GUN))
			{
				ItemUse_Sentry(ent);
				G_AddEvent(ent, EV_USE_ITEM0 + HI_SENTRY_GUN, 0);
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SENTRY_GUN); // TAKE AWAY AFTER USE ?
			}
			break;
		case GENCMD_USE_JETPACK: //HI_JETPACK
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK &&
				G_ItemUsable(&ent->client->ps, HI_JETPACK))
			{
				ItemUse_Jetpack(ent);
				G_AddEvent(ent, EV_USE_ITEM0 + HI_JETPACK, 0);
			}
			break;

		case GENCMD_USE_HEALTHDISP: // HI_HEALTHDISP
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_HEALTHDISP &&
				G_ItemUsable(&ent->client->ps, HI_HEALTHDISP))
			{
				ItemUse_UseDisp(ent, HI_HEALTHDISP);
				G_AddEvent(ent, EV_USE_ITEM0 + HI_HEALTHDISP, 0);
			}
			break;
		case GENCMD_USE_AMMODISP: //HI_AMMODISP
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_AMMODISP &&
				G_ItemUsable(&ent->client->ps, HI_AMMODISP))
			{
				ItemUse_UseDisp(ent, HI_AMMODISP);
				G_AddEvent(ent, EV_USE_ITEM0 + HI_AMMODISP, 0);
			}
			break;
		case GENCMD_USE_EWEB: //HI_EWEB
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_EWEB &&
				G_ItemUsable(&ent->client->ps, HI_EWEB))
			{
				ItemUse_UseEWeb(ent);
				G_AddEvent(ent, EV_USE_ITEM0 + HI_EWEB, 0);
			}
			break;
		case GENCMD_USE_CLOAK: //HI_CLOAK
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_CLOAK &&
				G_ItemUsable(&ent->client->ps, HI_CLOAK))
			{
				if (ent->client->ps.powerups[PW_CLOAKED])
				{
					//decloak
					Jedi_Decloak(ent);
				}
				else
				{
					//cloak
					if (ent->r.svFlags & SVF_BOT)
					{
						if (ent->health > 0
							&& ent->painDebounceTime < level.time
							&& !ent->client->ps.saberInFlight
							&& !PM_SaberInAttack(ent->client->ps.saber_move)
							&& ent->client->ps.fd.forceGripBeingGripped <= level.time
							&& !(ent->client->ps.communicatingflags & 1 << CLOAK_CHARGE_RESTRICTION))
						{
							Jedi_Cloak(ent);
						}
					}
					else
					{
						player_Cloak(ent);
					}
				}
			}
			break;
		case GENCMD_FLAMETHROWER://HI_FLAMETHROWER
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_FLAMETHROWER &&
				G_ItemUsable(&ent->client->ps, HI_FLAMETHROWER))
			{
				ItemUse_FlameThrower(ent);
				G_AddEvent(ent, EV_USE_ITEM0 + HI_FLAMETHROWER, 0);
			}
			break;

		case GENCMD_USE_SWOOP: //HI_SWOOP
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_SWOOP &&
				G_ItemUsable(&ent->client->ps, HI_SWOOP))
			{
				ItemUse_Swoop(ent);
				G_AddEvent(ent, EV_USE_ITEM0 + HI_SWOOP, 0);
			}
			break;
		case GENCMD_USE_DECCA: //HI_DROIDEKA
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_DROIDEKA &&
				G_ItemUsable(&ent->client->ps, HI_DROIDEKA))
			{
				ItemUse_Decca(ent);
				G_AddEvent(ent, EV_USE_ITEM0 + HI_DROIDEKA, 0);
			}
			break;

		case GENCMD_USE_SPHERESHIELD: //HI_SPHERESHIELD
			if ((ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SPHERESHIELD)) &&
				G_ItemUsable(&ent->client->ps, HI_SPHERESHIELD))
			{
				if (ent->client->ps.powerups[PW_SPHERESHIELDED])
				{//decloak
					Sphereshield_Off(ent);
				}
				else
				{//cloak
					Sphereshield_On(ent);
				}
			}
			break;

		case GENCMD_USE_GRAPPLE: //HI_GRAPPLE
			if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_GRAPPLE &&
				G_ItemUsable(&ent->client->ps, HI_GRAPPLE))
			{
				//
				G_AddEvent(ent, EV_USE_ITEM0 + HI_GRAPPLE, 0);
			}
			break;
			//////////////////////////////////////////////////////////////////////////////////////////////////

		case GENCMD_SABERATTACKCYCLE:
			Cmd_SaberAttackCycle_f(ent);
			break;
		case GENCMD_TAUNT:
			G_SetTauntAnim(ent, TAUNT_TAUNT);
			break;
		case GENCMD_BOW:
			G_SetTauntAnim(ent, TAUNT_BOW);
			break;
		case GENCMD_MEDITATE:
			G_SetTauntAnim(ent, TAUNT_MEDITATE);
			break;
		case GENCMD_FLOURISH:
			G_SetTauntAnim(ent, TAUNT_FLOURISH);
			break;
		case GENCMD_GLOAT:
			G_SetTauntAnim(ent, TAUNT_GLOAT);
			break;
		case GENCMD_SURRENDER:
			G_SetTauntAnim(ent, TAUNT_SURRENDER);
			break;
		default:
			break;
		}
	}

	// save results of pmove
	if (ent->client->ps.eventSequence != old_event_sequence)
	{
		ent->eventTime = level.time;
	}
	if (g_smoothClients.integer)
	{
		BG_PlayerStateToEntityStateExtraPolate(&ent->client->ps, &ent->s, ent->client->ps.commandTime, qfalse);
		//rww - 12-03-02 - Don't snap the origin of players! It screws prediction all up.
	}
	else
	{
		BG_PlayerStateToEntityState(&ent->client->ps, &ent->s, qfalse);
	}

	if (isNPC)
	{
		ent->s.eType = ET_NPC;
	}

	SendPendingPredictableEvents(&ent->client->ps);

	if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_GRAPPLE &&
		ent->client->ps.weapon == WP_MELEE &&
		ent->client->skillLevel[SK_GRAPPLE] >= FORCE_LEVEL_1)
	{
		if (ucmd->buttons & BUTTON_GRAPPLE && !(ucmd->buttons & BUTTON_WALKING) && !
			PM_RunningAnim(ent->client->ps.legsAnim) && !PM_WalkingAnim(ent->client->ps.legsAnim)
			&& ent->client->ps.pm_type != PM_DEAD
			&& !ent->client->hookhasbeenfired)
		{
			if (ent->client && ent->client->hookDebounceTime > level.time)
			{
				if (ent->client->hook)
				{
					Weapon_HookFree(ent->client->hook);
				}
			}
			else
			{
				if (ent->client->ps.eFlags2 & EF2_FLYING || ent->s.groundEntityNum == ENTITYNUM_NONE || PM_CrouchAnim(
					ent->client->ps.legsAnim))
				{
					//Boba_FireWristMissile(ent, BOBA_MISSILE_VIBROBLADE);
				}
				else
				{
					Weapon_GrapplingHook_Fire(ent);
					ent->client->hookhasbeenfired = qtrue;
					NPC_SetAnim(ent, SETANIM_BOTH, BOTH_PULL_IMPALE_STAB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_SoundOnEnt(ent, CHAN_ITEM, "sound/weapons/grapple/hookfire.wav");
					ent->client->hookDebounceTime = level.time + 0;
				}
			}
		}
		else if (client->hook && (client->fireHeld == qfalse ||
			!(ucmd->buttons & BUTTON_GRAPPLE) ||
			ucmd->buttons & BUTTON_USE ||
			ucmd->buttons & BUTTON_BLOCK ||
			PM_RunningAnim(ent->client->ps.legsAnim) ||
			PM_WalkingAnim(ent->client->ps.legsAnim) ||
			ent->client->ps.pm_type == PM_DEAD))
		{
			Weapon_HookFree(client->hook);
		}
	}
	else if (level.gametype == GT_SINGLE_PLAYER && ent->client->ps.weapon == WP_MELEE)
	{
		if (ucmd->buttons & BUTTON_GRAPPLE && !(ucmd->buttons & BUTTON_WALKING) && !
			PM_RunningAnim(ent->client->ps.legsAnim) && !PM_WalkingAnim(ent->client->ps.legsAnim)
			&& ent->client->ps.pm_type != PM_DEAD
			&& !ent->client->hookhasbeenfired)
		{
			if (ent->client && ent->client->hookDebounceTime > level.time)
			{
				if (ent->client->hook)
				{
					Weapon_HookFree(ent->client->hook);
				}
			}
			else
			{
				if (ent->client->ps.eFlags2 & EF2_FLYING || ent->s.groundEntityNum == ENTITYNUM_NONE || PM_CrouchAnim(
					ent->client->ps.legsAnim))
				{
					//Boba_FireWristMissile(ent, BOBA_MISSILE_VIBROBLADE);
				}
				else
				{
					Weapon_GrapplingHook_Fire(ent);
					ent->client->hookhasbeenfired = qtrue;
					NPC_SetAnim(ent, SETANIM_BOTH, BOTH_PULL_IMPALE_STAB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_SoundOnEnt(ent, CHAN_ITEM, "sound/weapons/grapple/hookfire.wav");
					ent->client->hookDebounceTime = level.time + 0;
				}
			}
		}
		else if (client->hook && (client->fireHeld == qfalse ||
			!(ucmd->buttons & BUTTON_GRAPPLE) ||
			ucmd->buttons & BUTTON_USE ||
			ucmd->buttons & BUTTON_BLOCK ||
			PM_RunningAnim(ent->client->ps.legsAnim) ||
			PM_WalkingAnim(ent->client->ps.legsAnim) ||
			ent->client->ps.pm_type == PM_DEAD))
		{
			Weapon_HookFree(client->hook);
		}
	}
	else if (ent->client->ps.weapon == WP_STUN_BATON && HoldingStun)
	{
		if (ent->client->ps.pm_type != PM_DEAD && !ent->client->stunhasbeenfired)
		{
			if (ent->client && ent->client->stunDebounceTime > level.time)
			{
				if (client->stun)
				{
					Weapon_StunFree(client->stun);
				}
			}
			else
			{
				Weapon_AltStun_Fire(ent);
				ent->client->stunhasbeenfired = qtrue;
				G_SoundOnEnt(ent, CHAN_ITEM, "sound/weapons/noghri/fire.mp3");
				ent->client->stunDebounceTime = level.time + 0;
			}
		}
		else if (client->stun && (client->stunHeld == qfalse ||
			!HoldingStun ||
			ucmd->buttons & BUTTON_USE ||
			ucmd->buttons & BUTTON_BLOCK ||
			ent->client->ps.pm_type == PM_DEAD))
		{
			Weapon_StunFree(client->stun);
		}
	}
	else
	{
		if (client->hook && (client->fireHeld == qfalse ||
			!(ucmd->buttons & BUTTON_GRAPPLE) ||
			ucmd->buttons & BUTTON_USE ||
			ent->client->ps.pm_type == PM_DEAD))
		{
			Weapon_HookFree(client->hook);
		}

		if (client->stun && (client->stunHeld == qfalse ||
			!HoldingStun ||
			ucmd->buttons & BUTTON_USE ||
			ucmd->buttons & BUTTON_BLOCK ||
			ent->client->ps.pm_type == PM_DEAD))
		{
			Weapon_StunFree(client->stun);
		}

		if (!(ent->client->ps.eFlags & EF_FIRING))
		{
			client->fireHeld = qfalse;
			client->stunHeld = qfalse;
		}
	}

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy(ent->s.pos.trBase, ent->r.currentOrigin);

	if (ent->s.eType != ET_NPC ||
		ent->s.NPC_class != CLASS_VEHICLE ||
		!ent->m_pVehicle ||
		!ent->m_pVehicle->m_iRemovedSurfaces)
	{
		//let vehicles that are getting broken apart do their own crazy sizing stuff
		VectorCopy(pmove.mins, ent->r.mins);
		VectorCopy(pmove.maxs, ent->r.maxs);
	}

	ent->waterlevel = pmove.waterlevel;
	ent->watertype = pmove.watertype;

	// execute client events
	ClientEvents(ent, old_event_sequence);

	if (pmove.useEvent)
	{
		//TODO: Use
		//		TryUse( ent );
	}
	if (ent->client->pers.cmd.buttons & BUTTON_USE && ent->client->ps.useDelay < level.time)
	{
		TryUse(ent);
		ent->client->ps.useDelay = level.time + 100;
	}
	if (ent->client->pers.cmd.buttons & BUTTON_BOTUSE && ent->client->ps.useDelay < level.time)
	{
		TryUse(ent);
		ent->client->ps.useDelay = level.time + 100;
	}

	// link entity now, after any personal teleporters have been used
	trap->LinkEntity((sharedEntity_t*)ent);
	if (!ent->client->noclip)
	{
		G_TouchTriggers(ent);
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy(ent->client->ps.origin, ent->r.currentOrigin);

	// touch other objects
	ClientImpacts(ent, &pmove);

	// save results of triggers and client events
	if (ent->client->ps.eventSequence != old_event_sequence)
	{
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// Did we kick someone in our pmove sequence?
	if (client->ps.forceKickFlip && !(client->buttons & BUTTON_GRAPPLE))
	{
		gentity_t* face_kicked = &g_entities[client->ps.forceKickFlip - 1];

		if (face_kicked && face_kicked->client && (!OnSameTeam(ent, face_kicked) || g_friendlyFire.integer) &&
			(!face_kicked->client->ps.duelInProgress || face_kicked->client->ps.duelIndex == ent->s.number) &&
			(!ent->client->ps.duelInProgress || ent->client->ps.duelIndex == face_kicked->s.number))
		{
			if (face_kicked && face_kicked->client && face_kicked->health && face_kicked->takedamage)
			{
				//push them away and do pain
				vec3_t oppDir;
				int strength = (int)VectorNormalize2(client->ps.velocity, oppDir);

				strength *= 0.05;

				VectorScale(oppDir, -1, oppDir);

				if (ent->client->ps.fd.forceGripCripple)
				{
					G_Damage(face_kicked, ent, ent, oppDir, client->ps.origin, 5, DAMAGE_NO_ARMOR, MOD_MELEE);
				}
				else
				{
					G_Damage(face_kicked, ent, ent, oppDir, client->ps.origin, strength, DAMAGE_NO_ARMOR, MOD_MELEE);
				}

				//WE NEED TO RELEASE THE WALL!!!!
				if (face_kicked->client->ps.pm_flags & PMF_STUCK_TO_WALL)
				{
					face_kicked->client->ps.pm_flags &= ~PMF_STUCK_TO_WALL;
				}

				if (face_kicked->client->ps.weapon != WP_SABER ||
					face_kicked->client->ps.fd.saberAnimLevel != FORCE_LEVEL_3 ||
					!PM_SaberInAttack(face_kicked->client->ps.saber_move) && !
					PM_SaberInStart(face_kicked->client->ps.saber_move) && !
					PM_SaberInReturn(face_kicked->client->ps.saber_move) && !PM_SaberInTransition(
						face_kicked->client->ps.saber_move))
				{
					if (face_kicked->health > 0 &&
						face_kicked->client->ps.stats[STAT_HEALTH] > 0 &&
						face_kicked->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN)
					{
						if (BG_KnockDownable(&face_kicked->client->ps) && Q_irand(1, 10) <= 3)
						{
							//only actually knock over sometimes, but always do velocity hit
							face_kicked->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
							face_kicked->client->ps.forceHandExtendTime = level.time + 1100;
							face_kicked->client->ps.forceDodgeAnim = 0;
							//this toggles between 1 and 0, when it's 1 we should play the get up anim
						}

						face_kicked->client->ps.otherKiller = ent->s.number;
						face_kicked->client->ps.otherKillerTime = level.time + 5000;
						face_kicked->client->ps.otherKillerDebounceTime = level.time + 100;
						face_kicked->client->otherKillerMOD = MOD_MELEE;
						face_kicked->client->otherKillerVehWeapon = 0;
						face_kicked->client->otherKillerWeaponType = WP_NONE;

						face_kicked->client->ps.velocity[0] = oppDir[0] * (strength * 40);
						face_kicked->client->ps.velocity[1] = oppDir[1] * (strength * 40);
						face_kicked->client->ps.velocity[2] = 200;
					}
				}

				G_Sound(face_kicked, CHAN_AUTO, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
			}
		}

		client->ps.forceKickFlip = 0;
	}

	// check for respawning
	if (client->ps.stats[STAT_HEALTH] <= 0
		&& !(client->ps.eFlags2 & EF2_HELD_BY_MONSTER) //can't respawn while being eaten
		&& ent->s.eType != ET_NPC)
	{
		// wait for the attack button to be pressed
		if (level.time > client->respawnTime && !gDoSlowMoDuel)
		{
			// forcerespawn is to prevent users from waiting out powerups
			int forceRes = g_forceRespawn.integer;

			if (level.gametype == GT_POWERDUEL)
			{
				forceRes = 1;
			}
			else if (level.gametype == GT_SIEGE && g_siegeRespawn.integer)
			{
				//wave respawning on
				forceRes = 1;
			}
			else if ((level.gametype == GT_FFA ||
				level.gametype == GT_TEAM ||
				level.gametype == GT_CTF) &&
				g_ffaRespawnTimer.integer)
			{
				//wave respawning on
				forceRes = 1;
			}

			if (forceRes > 0 &&
				level.time - client->respawnTime > forceRes * 1000)
			{
				ClientRespawn(ent);
				return;
			}

			// pressing attack or use is the normal respawn method
			if (ucmd->buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE))
			{
				ClientRespawn(ent);
			}
		}
		else if (gDoSlowMoDuel)
		{
			client->respawnTime = level.time + 1000;
		}
		return;
	}

	// perform once-a-second actions
	ClientTimerActions(ent, msec);

	G_UpdateClientBroadcasts(ent);

	//try some idle anims on ent if getting no input and not moving for some time
	if (ent->client->ps.weapon == WP_SABER)
	{
		G_CheckClientIdleSabers(ent, ucmd);
	}
	else
	{
		G_CheckClientIdleGuns(ent, ucmd);
	}

	// This code was moved here from clientThink to fix a problem with g_synchronousClients
	// being set to 1 when in vehicles.
	if (ent->s.number < MAX_CLIENTS && ent->client->ps.m_iVehicleNum)
	{
		//driving a vehicle
		//run it
		if (g_entities[ent->client->ps.m_iVehicleNum].inuse && g_entities[ent->client->ps.m_iVehicleNum].client)
		{
			ClientThink(ent->client->ps.m_iVehicleNum, &g_entities[ent->client->ps.m_iVehicleNum].m_pVehicle->m_ucmd);
		}
		else
		{
			//vehicle no longer valid?
			ent->client->ps.m_iVehicleNum = 0;
		}
	}
}

/*
==================
G_CheckClientTimeouts

Checks whether a client has exceded any timeouts and act accordingly
==================
*/
void G_CheckClientTimeouts(gentity_t* ent)
{
	// Only timeout supported right now is the timeout to spectator mode
	if (!g_timeouttospec.integer)
	{
		return;
	}

	// Already a spectator, no need to boot them to spectator
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return;
	}

	// See how long its been since a command was received by the client and if its
	// longer than the timeout to spectator then force this client into spectator mode
	if (level.time - ent->client->pers.cmd.serverTime > g_timeouttospec.integer * 1000)
	{
		SetTeam(ent, "spectator");
	}
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink(const int clientNum, const usercmd_t* ucmd)
{
	gentity_t* ent = g_entities + clientNum;
	if (clientNum < MAX_CLIENTS)
	{
		trap->GetUsercmd(clientNum, &ent->client->pers.cmd);
	}

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->client->lastCmdTime = level.time;

	if (ucmd)
	{
		ent->client->pers.cmd = *ucmd;
	}

	if (ent->s.eType == ET_PLAYER)
	{
		if (ent->client->ps.pm_flags & PMF_DUCKED)
		{
			ent->r.maxs[2] = ent->client->ps.crouchheight;
			ent->r.maxs[1] = 8;
			ent->r.maxs[0] = 8;
			ent->r.mins[1] = -8;
			ent->r.mins[0] = -8;
		}
		else if (!(ent->client->ps.pm_flags & PMF_DUCKED))
		{
			ent->r.maxs[2] = ent->client->ps.standheight - 8;
			ent->r.maxs[1] = 8;
			ent->r.maxs[0] = 8;
			ent->r.mins[1] = -8;
			ent->r.mins[0] = -8;
		}
	}

	if (!(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer)
	{
		ClientThink_real(ent);
	}
	else if (clientNum >= MAX_CLIENTS)
	{
		ClientThink_real(ent);
	}
}

void G_RunClient(gentity_t* ent)
{
	// force client updates if they're not sending packets at roughly 4hz
	if (!(ent->r.svFlags & SVF_BOT) && g_forceClientUpdateRate.integer && ent->client->lastCmdTime < level.time -
		g_forceClientUpdateRate.integer)
	{
		trap->GetUsercmd(ent - g_entities, &ent->client->pers.cmd);

		ent->client->lastCmdTime = level.time;

		// fill with seemingly valid data
		ent->client->pers.cmd.serverTime = level.time;
		ent->client->pers.cmd.buttons = 0;
		ent->client->pers.cmd.forwardmove = ent->client->pers.cmd.rightmove = ent->client->pers.cmd.upmove = 0;

		ClientThink_real(ent);
		return;
	}

	if (!(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer)
	{
		return;
	}
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink_real(ent);
}

/*
==================
SpectatorClientEndFrame

==================
*/
static void SpectatorClientEndFrame(const gentity_t* ent)
{
	if (ent->s.eType == ET_NPC)
	{
		assert(0);
		return;
	}

	// if we are doing a chase cam or a remote view, grab the latest info
	if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW)
	{
		int clientNum = ent->client->sess.spectatorClient;

		// team follow1 and team follow2 go to whatever clients are playing
		if (clientNum == -1)
		{
			clientNum = level.follow1;
		}
		else if (clientNum == -2)
		{
			clientNum = level.follow2;
		}
		if (clientNum >= 0)
		{
			const gclient_t* cl = &level.clients[clientNum];
			if (cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR)
			{
				ent->client->ps.eFlags = cl->ps.eFlags;
				ent->client->ps = cl->ps;
				ent->client->ps.pm_flags |= PMF_FOLLOW;
				return;
			}
			// drop them to free spectators unless they are dedicated camera followers
			if (ent->client->sess.spectatorClient >= 0)
			{
				ent->client->sess.spectatorState = SPECTATOR_FREE;
				ClientBegin(ent->client - level.clients, qtrue);
			}
		}
	}

	if (ent->client->sess.spectatorState == SPECTATOR_SCOREBOARD)
	{
		ent->client->ps.pm_flags |= PMF_SCOREBOARD;
	}
	else
	{
		ent->client->ps.pm_flags &= ~PMF_SCOREBOARD;
	}
}

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame(gentity_t* ent)
{
	qboolean isNPC = qfalse;

	if (ent->s.eType == ET_NPC)
	{
		isNPC = qtrue;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		SpectatorClientEndFrame(ent);
		return;
	}

	// turn off any expired powerups
	for (int i = 0; i < MAX_POWERUPS; i++)
	{
		if (ent->client->ps.powerups[i] < level.time)
		{
			ent->client->ps.powerups[i] = 0;
		}
	}

	// save network bandwidth
#if 0
	if (!g_synchronousClients->integer && (ent->client->ps.pm_type == PM_NORMAL || ent->client->ps.pm_type == PM_JETPACK || ent->client->ps.pm_type == PM_FLOAT)) {
		// FIXME: this must change eventually for non-sync demo recording
		VectorClear(ent->client->ps.viewangles);
	}
#endif

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if (level.intermissiontime)
	{
		if (ent->s.number < MAX_CLIENTS || ent->client->NPC_class == CLASS_VEHICLE)
		{
			//players and vehicles do nothing in intermissions
			return;
		}
	}

	// burn from lava, etc
	P_WorldEffects(ent);

	// apply all the damage taken this frame
	P_DamageFeedback(ent);

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if (level.time - ent->client->lastCmdTime > 1000)
		ent->client->ps.eFlags |= EF_CONNECTION;
	else
		ent->client->ps.eFlags &= ~EF_CONNECTION;

	ent->client->ps.stats[STAT_HEALTH] = ent->health; // FIXME: get rid of ent->health...

	G_SetClientSound(ent);

	// set the latest infor
	if (g_smoothClients.integer)
	{
		BG_PlayerStateToEntityStateExtraPolate(&ent->client->ps, &ent->s, ent->client->ps.commandTime, qfalse);
		//rww - 12-03-02 - Don't snap the origin of players! It screws prediction all up.
	}
	else
	{
		BG_PlayerStateToEntityState(&ent->client->ps, &ent->s, qfalse);
	}

	if (isNPC)
	{
		ent->s.eType = ET_NPC;
	}

	SendPendingPredictableEvents(&ent->client->ps);
}

/*
====================
ClientIntermissionThink
====================
*/
static qboolean ClientCinematicThink(gclient_t* client, const usercmd_t* ucmd)
{
	client->ps.eFlags &= ~EF_FIRING;

	// swap button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	if (client->buttons & BUTTON_USE & (client->oldbuttons ^ client->buttons))
	{
		return qtrue;
	}
	return qfalse;
}

qboolean NPC_IsNotHavingEnoughForceSight(const gentity_t* self)
{
	if (self->client && (!(self->client->ps.fd.forcePowersActive & 1 << FP_SEE))) //sight is not on OR
	{//force sight 2+ gives perfect aim
		return qtrue;
	}
	if (self->client && (self->client->ps.fd.forcePowersActive & 1 << FP_SEE && self->client->ps.fd.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2)) //sight is on but lower than 3
	{//force sight 2+ gives perfect aim
		return qtrue;
	}

	return qfalse;
}