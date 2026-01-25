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

//NPC_combat.cpp
#include "b_local.h"
#include "g_nav.h"
#include "bg_public.h"
#include "g_public.h"

extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
extern void G_SetEnemy(gentity_t* self, gentity_t* enemy);
extern qboolean NPC_CheckLookTarget(const gentity_t* self);
extern void NPC_ClearLookTarget(const gentity_t* self);
extern void NPC_Jedi_RateNewEnemy(const gentity_t* self, const gentity_t* enemy);
extern int NAV_FindClosestWaypointForPoint2(vec3_t point);
extern int NAV_GetNearestNode(gentity_t* self, int lastNode);
extern qboolean PM_DroidMelee(int npc_class);
extern qboolean PM_ReloadAnim(int anim);
void ChangeWeapon(const gentity_t* ent, int new_weapon);
qboolean G_ValidEnemy(const gentity_t* self, const gentity_t* enemy);

void G_ClearEnemy(gentity_t* self)
{
	NPC_CheckLookTarget(self);

	if (self->enemy)
	{
		//don't lose locked enemies
		if (G_ValidEnemy(self, self->enemy) && self->NPC->aiFlags & NPCAI_LOCKEDENEMY)
		{
			return;
		}
		if (self->client && self->client->renderInfo.lookTarget == self->enemy->s.number)
		{
			NPC_ClearLookTarget(self);
		}

		if (self->NPC && self->enemy == self->NPC->goalEntity)
		{
			self->NPC->goalEntity = NULL;
		}
		//FIXME: set last enemy?
	}

	self->enemy = NULL;
}

/*
-------------------------
NPC_AngerAlert
-------------------------
*/

#define	ANGER_ALERT_RADIUS			512
#define	ANGER_ALERT_SOUND_RADIUS	256

void G_AngerAlert(const gentity_t* self)
{
	if (self && self->NPC && self->NPC->scriptFlags & SCF_NO_GROUPS)
	{
		//I'm not a team playa...
		return;
	}
	if (!TIMER_Done(self, "interrogating"))
	{
		//I'm interrogating, don't wake everyone else up yet... FIXME: this may never wake everyone else up, though!
		return;
	}
	//FIXME: hmm.... with all the other new alerts now, is this still neccesary or even a good idea...?
	G_AlertTeam(self, self->enemy, ANGER_ALERT_RADIUS, ANGER_ALERT_SOUND_RADIUS);
}

/*
-------------------------
G_TeamEnemy
-------------------------
*/

qboolean G_TeamEnemy(const gentity_t* self)
{
	//FIXME: Probably a better way to do this, is a linked list of your teammates already available?

	if (!self->client || self->client->playerTeam == NPCTEAM_FREE)
	{
		return qfalse;
	}
	if (self && self->NPC && self->NPC->scriptFlags & SCF_NO_GROUPS)
	{
		//I'm not a team playa...
		return qfalse;
	}

	for (int i = 1; i < level.num_entities; i++)
	{
		const gentity_t* ent = &g_entities[i];

		if (ent == self)
		{
			continue;
		}

		if (ent->health <= 0)
		{
			continue;
		}

		if (!ent->client)
		{
			continue;
		}

		if (ent->client && ent->client->playerTeam != self->client->playerTeam)
		{
			//ent is not on my team
			continue;
		}

		if (ent->enemy)
		{
			//they have an enemy
			if (!ent->enemy->client || ent->enemy->client->playerTeam != self->client->playerTeam)
			{
				//the ent's enemy is either a normal ent or is a player/NPC that is not on my team
				return qtrue;
			}
		}
	}

	return qfalse;
}

static qboolean G_CheckSaberAllyAttackDelay(const gentity_t* self, const gentity_t* enemy)
{
	//sets a delay for attacking non-saber attackers when we're a saber user
	//assisting our human ally.
	if (!self || !self->enemy)
	{
		return qfalse;
	}
	if (self->NPC
		&& self->client->leader
		&& self->client->leader->client
		&& self->client->leader->client->ps.clientNum < MAX_CLIENTS
		&& self->enemy
		&& self->enemy->s.weapon != WP_SABER
		&& self->s.weapon == WP_SABER)
	{
		//assisting the player and I'm using a saber and my enemy is not
		TIMER_Set(self, "allyJediDelay", -level.time);
		//use the distance to the enemy to determine how long to delay
		const float distance = Distance(enemy->r.currentOrigin, self->r.currentOrigin);
		if (distance < 256)
		{
			return qtrue;
		}
		int delay;
		if (distance > 2048)
		{
			//the farther they are, the shorter the delay
			delay = 5000 - floor(distance); //(6-g_spskill->integer));
			if (delay < 500)
			{
				delay = 500;
			}
		}
		else
		{
			//the close they are, the shorter the delay
			delay = floor(distance * 4); //(6-g_spskill->integer));
			if (delay > 5000)
			{
				delay = 5000;
			}
		}
		TIMER_Set(self, "allyJediDelay", delay);

		return qtrue;
	}
	return qfalse;
}

static void G_AttackDelay(const gentity_t* self, const gentity_t* enemy)
{
	if (enemy && self->client && self->NPC)
	{
		//delay their attack based on how far away they're facing from enemy
		vec3_t fwd, dir;

		VectorSubtract(self->client->renderInfo.eyePoint, enemy->r.currentOrigin, dir); //purposely backwards
		VectorNormalize(dir);
		AngleVectors(self->client->renderInfo.eyeAngles, fwd, NULL, NULL);
		//dir[2] = fwd[2] = 0;//ignore z diff?

		int attDelay = (4 - g_npcspskill.integer) * 500; //initial: from 1000ms delay on hard to 2000ms delay on easy
		if (self->client->playerTeam == NPCTEAM_PLAYER)
		{
			//invert
			attDelay = 2000 - attDelay;
		}
		attDelay += floor((DotProduct(fwd, dir) + 1.0f) * 2000.0f); //add up to 4000ms delay if they're facing away

		//FIXME: should distance matter, too?

		//Now modify the delay based on NPC_class, weapon, and team
		//NOTE: attDelay should be somewhere between 1000 to 6000 milliseconds
		switch (self->client->NPC_class)
		{
		case CLASS_IMPERIAL: //they give orders and hang back
			attDelay += Q_irand(500, 1500);
			break;
		case CLASS_STORMTROOPER: //stormtroopers shoot sooner
		case CLASS_STORMCOMMANDO: //stormtroopers shoot sooner
			if (self->NPC->rank >= RANK_LT)
			{
				//officers shoot even sooner
				attDelay -= Q_irand(500, 1500);
			}
			else
			{
				//normal stormtroopers don't have as fast reflexes as officers
				attDelay -= Q_irand(0, 1000);
			}
			break;
		case CLASS_CLONETROOPER: //stormtroopers shoot sooner
			if (self->NPC->rank >= RANK_LT)
			{
				//officers shoot even sooner
				attDelay -= Q_irand(500, 1500);
			}
			else
			{
				//normal stormtroopers don't have as fast reflexes as officers
				attDelay -= Q_irand(0, 1000);
			}
			break;
		case CLASS_SWAMPTROOPER: //shoot very quickly?  What about guys in water?
			attDelay -= Q_irand(1000, 2000);
			break;
		case CLASS_IMPWORKER: //they panic, don't fire right away
			attDelay += Q_irand(1000, 2500);
			break;
		case CLASS_TRANDOSHAN:
			attDelay -= Q_irand(500, 1500);
			break;
		case CLASS_JAN:
		case CLASS_SBD:
		case CLASS_DROIDEKA:
		case CLASS_LANDO:
		case CLASS_PRISONER:
		case CLASS_REBEL:
		case CLASS_WOOKIE:
			attDelay -= Q_irand(500, 1500);
			break;
		case CLASS_GALAKMECH:
		case CLASS_ATST:
			attDelay -= Q_irand(1000, 2000);
			break;
		case CLASS_REELO:
		case CLASS_UGNAUGHT:
		case CLASS_JAWA:
			return;
		case CLASS_MINEMONSTER:
		case CLASS_MURJJ:
			return;
		case CLASS_INTERROGATOR:
		case CLASS_PROBE:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_SENTRY:
			return;
		case CLASS_REMOTE:
		case CLASS_SEEKER:
			return;
			/*
				case CLASS_GRAN:
				case CLASS_RODIAN:
				case CLASS_WEEQUAY:
					break;
				case CLASS_JEDI:
				case CLASS_SHADOWTROOPER:
				case CLASS_TAVION:
				case CLASS_REBORN:
				case CLASS_LUKE:
				case CLASS_DESANN:
					break;
				*/
		default:
			break;
		}

		switch (self->s.weapon)
		{
		case WP_NONE:
		case WP_SABER:
			return;
		case WP_BRYAR_PISTOL:
			break;
		case WP_BLASTER:
			if (self->NPC->scriptFlags & SCF_altFire)
			{
				//rapid-fire blasters
				attDelay += Q_irand(0, 500);
			}
			else
			{
				//regular blaster
				attDelay -= Q_irand(0, 500);
			}
			break;
		case WP_BOWCASTER:
			attDelay += Q_irand(0, 500);
			break;
		case WP_REPEATER:
			if (!(self->NPC->scriptFlags & SCF_altFire))
			{
				//rapid-fire blasters
				attDelay += Q_irand(0, 500);
			}
			break;
		case WP_FLECHETTE:
			attDelay += Q_irand(500, 1500);
			break;
		case WP_ROCKET_LAUNCHER:
			attDelay += Q_irand(500, 1500);
			break;
		case WP_CONCUSSION:
			attDelay += Q_irand(500, 1500);
			break;
			//		case WP_BLASTER_PISTOL:	// apparently some enemy only version of the blaster
			//			attDelay -= Q_irand( 500, 1500 );
			//			break;
			//rwwFIXMEFIXME: Have this weapon for NPCs?
		case WP_DISRUPTOR: //sniper's don't delay?
			return;
		case WP_THERMAL: //grenade-throwing has a built-in delay
			return;
		case WP_MELEE: // Any ol' melee attack
			return;
		case WP_EMPLACED_GUN:
			return;
		case WP_TURRET: // turret guns
			return;
		default:;
			/*
					case WP_DEMP2:
						break;
					case WP_TRIP_MINE:
						break;
					case WP_DET_PACK:
						break;
					case WP_STUN_BATON:
						break;
					case WP_ATST_MAIN:
						break;
					case WP_ATST_SIDE:
						break;
					case WP_TIE_FIGHTER:
						break;
					case WP_RAPID_FIRE_CONC:
						break;
					*/
		}

		if (self->client->playerTeam == NPCTEAM_PLAYER)
		{
			//clamp it
			if (attDelay > 2000)
			{
				attDelay = 2000;
			}
		}

		//don't shoot right away
		if (attDelay > 4000 + (2 - g_npcspskill.integer) * 3000)
		{
			attDelay = 4000 + (2 - g_npcspskill.integer) * 3000;
		}
		TIMER_Set(self, "attackDelay", attDelay); //Q_irand( 1500, 4500 ) );
		//don't move right away either
		if (attDelay > 4000)
		{
			attDelay = 4000 - Q_irand(500, 1500);
		}
		else
		{
			attDelay -= Q_irand(500, 1500);
		}

		TIMER_Set(self, "roamTime", attDelay); //was Q_irand( 1000, 3500 );
	}
}

static void G_ForceSaberOn(gentity_t* ent)
{
	if (ent->client->ps.saberInFlight)
	{
		//alright, can't turn it on now in any case, so forget it.
		return;
	}

	if (!ent->client->ps.saberHolstered)
	{
		//it's already on!
		return;
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		//This probably should never happen. But if it does we'll just return without complaining.
		return;
	}

	//Well then, turn it on.
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

/*
-------------------------
G_SetEnemy
-------------------------
*/
void G_AimSet(const gentity_t* self, int aim);
extern void Saboteur_Cloak(gentity_t* self);
extern gentity_t* G_CheckControlledTurretEnemy(const gentity_t* self, gentity_t* enemy, qboolean validate);
extern void G_SoundOnEnt(gentity_t* ent, soundChannel_t channel, const char* sound_path);

void G_SetEnemy(gentity_t* self, gentity_t* enemy)
{
	int event = 0;

	//Must be valid
	if (enemy == NULL)
		return;

	//Must be valid
	if (enemy->inuse == 0)
	{
		return;
	}

	enemy = G_CheckControlledTurretEnemy(self, enemy, qtrue);
	if (!enemy)
	{
		return;
	}

	//Don't take the enemy if in notarget
	if (enemy->flags & FL_NOTARGET)
		return;

	if (!self->NPC)
	{
		self->enemy = enemy;
		return;
	}

	if (self->NPC->confusionTime > level.time)
	{
		//can't pick up enemies if confused
		return;
	}

	//#ifdef _DEBUG
	//	if ( self->s.number >= MAX_CLIENTS )
	//	{
	//		assert( enemy != self );
	//	}
	//#endif// _DEBUG

	if (self->client && self->NPC && enemy->client)
	{
		if (enemy->client->playerTeam == self->client->playerTeam)
		{
			//Probably a damn script!
			if (self->NPC->charmedTime > level.time)
			{
				//Probably a damn script!
				return;
			}

			// Fix TEAM_ENEMY NPCs trying to target the player who is on their team
			if (self->client->playerTeam != NPCTEAM_FREE && self->client->playerTeam != NPCTEAM_SOLO)
			{
				return;
			}
		}
	}

	if (self->NPC && self->client && self->client->ps.weapon == WP_SABER)
	{
		//when get new enemy, set a base aggression based on what that enemy is using, how far they are, etc.
		NPC_Jedi_RateNewEnemy(self, enemy);
	}

	//NOTE: this is not necessarily true!
	//self->NPC->enemyLastSeenTime = level.time;

	if (self->enemy == NULL)
	{
		//TEMP HACK: turn on our saber
		if (self->health > 0)
		{
			G_ForceSaberOn(self);
		}

		//FIXME: Have to do this to prevent alert cascading
		G_ClearEnemy(self);
		self->enemy = enemy;

		if (self->client && self->client->NPC_class == CLASS_SABOTEUR)
		{
			//saboteurs cloak before initially attacking.
			Saboteur_Cloak(NPCS.NPC); // Cloak
			TIMER_Set(self, "decloakwait", 3000); // Wait 3 sec before decloak and attack
		}

		//Special case- if player is being hunted by his own people, set their enemy team correctly
		if (self->client->playerTeam == NPCTEAM_PLAYER
			&& enemy->s.number < MAX_CLIENTS
			&& enemy->client && enemy->client->playerTeam == NPCTEAM_PLAYER)
		{
			//make the player "evil" so that everyone goes after him
			enemy->client->enemyTeam = (npcteam_t)TEAM_FREE;
			enemy->client->playerTeam = (npcteam_t)TEAM_FREE;
		}

		//If have an anger script, run that instead of yelling
		if (G_ActivateBehavior(self, BSET_ANGER))
		{
		}
		else if (self->client
			&& self->client->NPC_class == CLASS_KYLE
			&& self->client->leader
			&& self->client->leader->client->ps.clientNum < MAX_CLIENTS
			&& !TIMER_Done(self, "kyleAngerSoundDebounce"))
		{
			//Kyle ally NPC doesn't yell if you have an enemy more than once every five seconds
		}
		else if (self->client && enemy->client && self->client->playerTeam != enemy->client->playerTeam)
		{
			//	 Basically, you're first one to notice enemies
			if (self->client->pushEffectTime < level.time) // not currently being pushed
			{
				//say something if we're not getting pushed.
				if (!G_TeamEnemy(self) && self->client->NPC_class != CLASS_BOBAFETT)
				{
					//team did not have an enemy previously and we're not Boba Fett
					if (self->NPC
						&& self->client->playerTeam == NPCTEAM_PLAYER
						&& enemy->s.number < MAX_CLIENTS
						&& Q_stricmp("Jedi2", self->NPC_type) == 0)
					{
						//jedi2 use different sounds
						switch (Q_irand(0, 2))
						{
						case 0:
							G_SoundOnEnt(self, CHAN_VOICE, "sound/chars/jedi2/28je2008.wav");
							break;
						case 1:
							G_SoundOnEnt(self, CHAN_VOICE, "sound/chars/jedi2/28je2009.wav");
							break;
						case 2:
							G_SoundOnEnt(self, CHAN_VOICE, "sound/chars/jedi2/28je2012.wav");
							break;
						default:;
						}
						self->NPC->blockedSpeechDebounceTime = level.time + 2000;
					}
					else
					{
						if (Q_irand(0, 1))
						{
							//hell, we're loading them, might as well use them!
							event = Q_irand(EV_CHASE1, EV_CHASE3);
						}
						else
						{
							event = Q_irand(EV_ANGER1, EV_ANGER3);
						}
					}
				}
			}

			if (event)
			{
				//yell
				if (self->client
					&& self->client->NPC_class == CLASS_KYLE
					&& self->client->leader
					&& self->client->leader->client->ps.clientNum < MAX_CLIENTS)
				{
					//don't yell that you have an enemy more than once every 4-8 seconds
					TIMER_Set(self, "kyleAngerSoundDebounce", Q_irand(4000, 8000));
				}
				G_AddVoiceEvent(self, event, 2000);
			}
		}

		if (self->s.weapon == WP_BLASTER || self->s.weapon == WP_REPEATER ||
			self->s.weapon == WP_THERMAL
			|| self->s.weapon == WP_BOWCASTER)
		{
			//When first get mad, aim is bad
			if (self->client->playerTeam == NPCTEAM_PLAYER)
			{
				G_AimSet(self, Q_irand(self->NPC->stats.aim - 5 * g_npcspskill.integer,
					self->NPC->stats.aim - g_npcspskill.integer));
			}
			else
			{
				int minErr = 3;
				int maxErr = 12;
				if (self->client->NPC_class == CLASS_IMPWORKER)
				{
					minErr = 15;
					maxErr = 30;
				}
				else if (self->client->NPC_class == CLASS_STORMTROOPER && self->NPC && self->NPC->rank <=
					RANK_CREWMAN)
				{
					minErr = 5;
					maxErr = 15;
				}
				else if (self->client->NPC_class == CLASS_CLONETROOPER && self->NPC && self->NPC->rank <=
					RANK_CREWMAN)
				{
					minErr = 5;
					maxErr = 15;
				}
				else if (self->client->NPC_class == CLASS_STORMCOMMANDO && self->NPC && self->NPC->rank <=
					RANK_CREWMAN)
				{
					minErr = 5;
					maxErr = 15;
				}
				else if (self->client->NPC_class == CLASS_SBD && self->NPC && self->NPC->rank <= RANK_CREWMAN)
				{
					minErr = 5;
					maxErr = 15;
				}

				G_AimSet(self, Q_irand(self->NPC->stats.aim - maxErr * (3 - g_npcspskill.integer),
					self->NPC->stats.aim - minErr * (3 - g_npcspskill.integer)));
			}
		}

		//Alert anyone else in the area
		if (Q_stricmp("STCommander", self->NPC_type) != 0 && Q_stricmp("ImpCommander", self->NPC_type) != 0)
		{
			//special enemies exception
			if (self->client->ps.fd.forceGripBeingGripped < level.time)
			{
				//gripped people can't call for help
				G_AngerAlert(self);
			}
		}
		if (!G_CheckSaberAllyAttackDelay(self, enemy))
		{
			//not a saber ally holding back
			//Stormtroopers don't fire right away!
			G_AttackDelay(self, enemy);
		}

		//FIXME: this is a disgusting hack that is supposed to make the Imperials start with their weapon holstered- need a better way
		if (self->client->ps.weapon == WP_NONE && !Q_strncmp(self->NPC_type, "imp", 3) && !(self->NPC->
			scriptFlags &
			SCF_FORCED_MARCH))
		{
			if (self->client->ps.stats[STAT_WEAPONS] & 1 << WP_BLASTER)
			{
				ChangeWeapon(self, WP_BLASTER);
				self->client->ps.weapon = WP_BLASTER;
				self->client->ps.weaponstate = WEAPON_READY;
			}
			else if (self->client->ps.stats[STAT_WEAPONS] & 1 << WP_BRYAR_PISTOL)
			{
				ChangeWeapon(self, WP_BRYAR_PISTOL);
				self->client->ps.weapon = WP_BRYAR_PISTOL;
				self->client->ps.weaponstate = WEAPON_READY;
			}
		}
		return;
	}

	//Otherwise, just picking up another enemy

	if (event)
	{
		G_AddVoiceEvent(self, event, 2000);
	}

	//Take the enemy
	G_ClearEnemy(self);
	self->enemy = enemy;
}

void ChangeWeapon(const gentity_t* ent, int new_weapon)
{
	if (!ent || !ent->client || !ent->NPC)
	{
		return;
	}

	if (PM_ReloadAnim(ent->client->ps.torsoAnim))
	{
		return;
	}

	if (new_weapon < 0)
	{
		new_weapon = WP_NONE;
	}

	ent->client->ps.weapon = new_weapon;
	ent->client->pers.cmd.weapon = new_weapon;
	ent->NPC->shotTime = 0;
	ent->NPC->burstCount = 0;
	ent->NPC->attackHold = 0;
	ent->NPC->currentAmmo = ent->client->ps.ammo[weaponData[new_weapon].ammoIndex];

	switch (new_weapon)
	{
	case WP_BRYAR_PISTOL: //prifle
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		ent->NPC->burstSpacing = 1000; //attackdebounce
		break;

	case WP_BRYAR_OLD:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		if (g_npcspskill.integer == 0)
			ent->NPC->burstSpacing = 1000; //attack debounce
		else if (g_npcspskill.integer == 1)
			ent->NPC->burstSpacing = 750; //attack debounce
		else
			ent->NPC->burstSpacing = 500; //attack debounce
		break;

	case WP_SABER:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		ent->NPC->burstSpacing = 500; //attackdebounce
		break;

	case WP_DISRUPTOR:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		if (ent->NPC->scriptFlags & SCF_altFire)
		{
			switch (g_npcspskill.integer)
			{
			case 0:
				ent->NPC->burstSpacing = 2500; //attackdebounce
				break;
			case 1:
				ent->NPC->burstSpacing = 2000; //attackdebounce
				break;
			case 2:
				ent->NPC->burstSpacing = 1500; //attackdebounce
				break;
			default:;
			}
		}
		else
		{
			ent->NPC->burstSpacing = 1000; //attackdebounce
		}
		break;

	case WP_BOWCASTER:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		if (g_npcspskill.integer == 0)
			ent->NPC->burstSpacing = 1000; //attack debounce
		else if (g_npcspskill.integer == 1)
			ent->NPC->burstSpacing = 750; //attack debounce
		else
			ent->NPC->burstSpacing = 500; //attack debounce
		break;

	case WP_REPEATER:
		if (ent->NPC->scriptFlags & SCF_altFire)
		{
			ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
			ent->NPC->burstSpacing = 2000; //attackdebounce
		}
		else
		{
			ent->NPC->aiFlags |= NPCAI_BURST_WEAPON;
			ent->NPC->burstMin = 3;
			ent->NPC->burstMean = 6;
			ent->NPC->burstMax = 10;
			if (g_npcspskill.integer == 0)
				ent->NPC->burstSpacing = 1500; //attack debounce
			else if (g_npcspskill.integer == 1)
				ent->NPC->burstSpacing = 1000; //attack debounce
			else
				ent->NPC->burstSpacing = 500; //attack debounce
		}
		break;

	case WP_DEMP2:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		ent->NPC->burstSpacing = 1000; //attackdebounce
		break;

	case WP_FLECHETTE:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		if (ent->NPC->scriptFlags & SCF_altFire)
		{
			ent->NPC->burstSpacing = 2000; //attackdebounce
		}
		else
		{
			ent->NPC->burstSpacing = 1000; //attackdebounce
		}
		break;

	case WP_ROCKET_LAUNCHER:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		//	ent->NPC->burstSpacing = 2500;//attackdebounce
		if (g_npcspskill.integer == 0)
			ent->NPC->burstSpacing = 2500; //attack debounce
		else if (g_npcspskill.integer == 1)
			ent->NPC->burstSpacing = 2000; //attack debounce
		else
			ent->NPC->burstSpacing = 1500; //attack debounce
		break;
	case WP_CONCUSSION:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		if (ent->NPC->scriptFlags & SCF_altFire)
		{
			//beam
			ent->NPC->burstSpacing = 1200; //attackdebounce
		}
		else
		{
			//rocket
			if (g_npcspskill.integer == 0)
				ent->NPC->burstSpacing = 2300; //attack debounce
			else if (g_npcspskill.integer == 1)
				ent->NPC->burstSpacing = 1800; //attack debounce
			else
				ent->NPC->burstSpacing = 1200; //attack debounce
		}
		break;

	case WP_THERMAL:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		//	ent->NPC->burstSpacing = 3000;//attackdebounce
		if (g_npcspskill.integer == 0)
			ent->NPC->burstSpacing = 4500; //attack debounce
		else if (g_npcspskill.integer == 1)
			ent->NPC->burstSpacing = 3000; //attack debounce
		else
			ent->NPC->burstSpacing = 2000; //attack debounce
		break;

		/*
		case WP_SABER:
			ent->NPC->aiFlags |= NPCAI_BURST_WEAPON;
			ent->NPC->burstMin = 5;//0.5 sec
			ent->NPC->burstMean = 10;//1 second
			ent->NPC->burstMax = 20;//3 seconds
			ent->NPC->burstSpacing = 2000;//2 seconds
			ent->NPC->attackHold = 1000;//Hold attack button for a 1-second burst
			break;
		*/

	case WP_BLASTER:
		if (ent->NPC->scriptFlags & SCF_altFire)
		{
			ent->NPC->aiFlags |= NPCAI_BURST_WEAPON;
			ent->NPC->burstMin = 3;
			ent->NPC->burstMean = 3;
			ent->NPC->burstMax = 3;
			if (g_npcspskill.integer == 0)
				ent->NPC->burstSpacing = 1500; //attack debounce
			else if (g_npcspskill.integer == 1)
				ent->NPC->burstSpacing = 1000; //attack debounce
			else
				ent->NPC->burstSpacing = 500; //attack debounce
		}
		else
		{
			ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
			if (g_npcspskill.integer == 0)
				ent->NPC->burstSpacing = 1000; //attack debounce
			else if (g_npcspskill.integer == 1)
				ent->NPC->burstSpacing = 750; //attack debounce
			else
				ent->NPC->burstSpacing = 500; //attack debounce
			//	ent->NPC->burstSpacing = 1000;//attackdebounce
		}
		break;

	case WP_MELEE:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		ent->NPC->burstSpacing = 1000; //attackdebounce
		break;

		/*
		case WP_ATST_MAIN:
		case WP_ATST_SIDE:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		//	ent->NPC->burstSpacing = 1000;//attackdebounce
			if ( g_npcspskill.integer == 0 )
				ent->NPC->burstSpacing = 1000;//attack debounce
			else if ( g_npcspskill.integer == 1 )
				ent->NPC->burstSpacing = 750;//attack debounce
			else
				ent->NPC->burstSpacing = 500;//attack debounce
		break;
		*/
		//rwwFIXMEFIXME: support for atst weaps

	case WP_EMPLACED_GUN:
		//FIXME: give some designer-control over this?
		if (ent->client && ent->client->NPC_class == CLASS_REELO)
		{
			ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
			ent->NPC->burstSpacing = 1000; //attack debounce
		}
		else
		{
			ent->NPC->aiFlags |= NPCAI_BURST_WEAPON;
			ent->NPC->burstMin = 2; // 3 shots, really
			ent->NPC->burstMean = 2;
			ent->NPC->burstMax = 2;

			if (ent->parent)
				// if we have an owner, it should be the chair at this point...so query the chair for its shot debounce times, etc.
			{
				if (g_npcspskill.integer == 0)
				{
					ent->NPC->burstSpacing = ent->parent->wait + 400; //attack debounce
					ent->NPC->burstMin = ent->NPC->burstMax = 1; // two shots
				}
				else if (g_npcspskill.integer == 1)
				{
					ent->NPC->burstSpacing = ent->parent->wait + 200; //attack debounce
				}
				else
				{
					ent->NPC->burstSpacing = ent->parent->wait; //attack debounce
				}
			}
			else
			{
				if (g_npcspskill.integer == 0)
				{
					ent->NPC->burstSpacing = 1200; //attack debounce
					ent->NPC->burstMin = ent->NPC->burstMax = 1; // two shots
				}
				else if (g_npcspskill.integer == 1)
				{
					ent->NPC->burstSpacing = 1000; //attack debounce
				}
				else
				{
					ent->NPC->burstSpacing = 800; //attack debounce
				}
			}
		}
		break;

	default:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		break;
	}
}

void NPC_ChangeWeapon(const int new_weapon)
{
	if (new_weapon != NPCS.NPC->client->ps.weapon)
	{
		G_AddEvent(NPCS.NPC, EV_GENERAL_SOUND, G_SoundIndex("sound/weapons/change.wav"));
	}
	ChangeWeapon(NPCS.NPC, new_weapon);
}

/*
void NPC_ApplyWeaponFireDelay(void)
How long, if at all, in msec the actual fire should delay from the time the attack was started
*/
void NPC_ApplyWeaponFireDelay(void)
{
	if (NPCS.NPC->attackDebounceTime > level.time)
	{
		//Just fired, if attacking again, must be a burst fire, so don't add delay
		return;
	}

	switch (NPCS.client->ps.weapon)
	{
		/*
	case WP_BOT_LASER:
		NPCInfo->burstCount = 0;
		client->ps.weaponTime = 500;
		break;
		*/ //rwwFIXMEFIXME: support for this

	case WP_THERMAL:
		if (NPCS.client->ps.clientNum)
		{
			NPCS.client->ps.weaponTime = 700;
		}
		break;
	case WP_MELEE:
		NPCS.client->ps.weaponTime = 300;
		break;

	default:
		NPCS.client->ps.weaponTime = 0;
		break;
	}
}

/*
-------------------------
ShootThink
-------------------------
*/
void ShootThink(void)
{
	int delay;

	NPCS.ucmd.buttons |= BUTTON_ATTACK;

	NPCS.NPCInfo->currentAmmo = NPCS.client->ps.ammo[weaponData[NPCS.client->ps.weapon].ammoIndex];
	// checkme

	NPC_ApplyWeaponFireDelay();

	if (NPCS.NPCInfo->aiFlags & NPCAI_BURST_WEAPON)
	{
		if (!NPCS.NPCInfo->burstCount)
		{
			NPCS.NPCInfo->burstCount = Q_irand(NPCS.NPCInfo->burstMin, NPCS.NPCInfo->burstMax);
			delay = 0;
		}
		else
		{
			NPCS.NPCInfo->burstCount--;
			if (NPCS.NPCInfo->burstCount == 0)
			{
				delay = NPCS.NPCInfo->burstSpacing + Q_irand(-150, 150);
			}
			else
			{
				delay = 0;
			}
		}

		if (!delay)
		{
			// HACK: dirty little emplaced bits, but is done because it would otherwise require some sort of new variable...
			if (NPCS.client->ps.weapon == WP_EMPLACED_GUN)
			{
				if (NPCS.NPC->parent) // try and get the debounce values from the chair if we can
				{
					if (g_npcspskill.integer == 0)
					{
						delay = NPCS.NPC->parent->random + 150;
					}
					else if (g_npcspskill.integer == 1)
					{
						delay = NPCS.NPC->parent->random + 100;
					}
					else
					{
						delay = NPCS.NPC->parent->random;
					}
				}
				else
				{
					if (g_npcspskill.integer == 0)
					{
						delay = 350;
					}
					else if (g_npcspskill.integer == 1)
					{
						delay = 300;
					}
					else
					{
						delay = 200;
					}
				}
			}
		}
	}
	else
	{
		delay = NPCS.NPCInfo->burstSpacing + Q_irand(-150, 150);
	}

	NPCS.NPCInfo->shotTime = level.time + delay;
	NPCS.NPC->attackDebounceTime = level.time + NPC_AttackDebounceForWeapon();
}

void WeaponThink()
{
	NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
	if (NPCS.client->ps.weaponstate == WEAPON_RAISING || NPCS.client->ps.weaponstate == WEAPON_DROPPING)
	{
		NPCS.ucmd.weapon = NPCS.client->ps.weapon;
		return;
	}

	// can't shoot while shield is up
	/*if (NPCS.NPC->flags&FL_SHIELDED && NPCS.NPC->client->NPC_class == CLASS_ASSASSIN_DROID)
	{
		return;
	}*/

	// Can't Fire While Cloaked
	if (NPCS.NPC->client &&
		NPCS.NPC->client->ps.powerups[PW_CLOAKED])
	{
		return;
	}

	if (NPCS.client->ps.weapon == WP_NONE)
	{
		return;
	}

	if (NPCS.client->ps.weaponstate != WEAPON_READY && NPCS.client->ps.weaponstate != WEAPON_FIRING &&
		NPCS.
		client->
		ps.weaponstate != WEAPON_IDLE)
	{
		return;
	}

	if (level.time < NPCS.NPCInfo->shotTime)
	{
		return;
	}

	//MCG - Begin
	//For now, no-one runs out of ammo
	if (NPCS.NPC->client->ps.ammo[weaponData[NPCS.client->ps.weapon].ammoIndex] < weaponData[NPCS.client
		->ps.
		weapon]
		.energyPerShot)
	{
		Add_Ammo(NPCS.NPC, NPCS.client->ps.weapon,
			weaponData[NPCS.client->ps.weapon].energyPerShot * 10);
	}
	else if (NPCS.NPC->client->ps.ammo[weaponData[NPCS.client->ps.weapon].ammoIndex] < weaponData[NPCS.
		client->
		ps.
		weapon].altEnergyPerShot)
	{
		Add_Ammo(NPCS.NPC, NPCS.client->ps.weapon,
			weaponData[NPCS.client->ps.weapon].altEnergyPerShot * 5);
	}

	NPCS.ucmd.weapon = NPCS.client->ps.weapon;
	ShootThink();
}

/*
HaveWeapon
*/

qboolean HaveWeapon(const int weapon)
{
	return NPCS.client->ps.stats[STAT_WEAPONS] & 1 << weapon;
}

int NPC_HaveBetterWeapon(const int weapon)
{
	if (weapon != WP_NONE && weapon != WP_MELEE && weapon != WP_STUN_BATON)
		return 0;

	for (int i = WP_SABER; i < WP_NUM_WEAPONS; i++)
	{
		if (HaveWeapon(i))
		{
			if (weapon == WP_NONE)
				return 2;
			return 1;
		}
	}

	return 0;
}

qboolean EntIsGlass(const gentity_t* check)
{
	if (check->classname
		&& (!Q_stricmp("func_breakable", check->classname) || !
			Q_stricmp("func_glass", check->classname))
		&& check->count == 1 && check->health <= 100)
	{
		return qtrue;
	}

	return qfalse;
}

qboolean ShotThroughGlass(trace_t* tr, const gentity_t* target, vec3_t spot, const int mask)
{
	const gentity_t* hit = &g_entities[tr->entityNum];
	if (hit != target && EntIsGlass(hit))
	{
		//ok to shoot through breakable glass
		const int skip = hit->s.number;
		vec3_t muzzle;

		VectorCopy(tr->endpos, muzzle);
		trap->Trace(tr, muzzle, NULL, NULL, spot, skip, mask, qfalse, 0, 0);
		return qtrue;
	}

	return qfalse;
}

/*
CanShoot
determine if NPC can directly target enemy

this function does not check teams, invulnerability, notarget, etc....

Added: If can't shoot center, try head, if not, see if it's close enough to try anyway.
*/
qboolean CanShoot(const gentity_t* ent, const gentity_t* shooter)
{
	trace_t tr;
	vec3_t muzzle;
	vec3_t spot, diff;

	CalcEntitySpot(shooter, SPOT_WEAPON, muzzle);
	CalcEntitySpot(ent, SPOT_ORIGIN, spot);
	//FIXME preferred target locations for some weapons (feet for R/L)

	trap->Trace(&tr, muzzle, NULL, NULL, spot, shooter->s.number, MASK_SHOT, qfalse, 0, 0);
	const gentity_t* traceEnt = &g_entities[tr.entityNum];

	// point blank, baby!
	if (tr.startsolid && shooter->NPC && shooter->NPC->touchedByPlayer)
	{
		traceEnt = shooter->NPC->touchedByPlayer;
	}

	if (ShotThroughGlass(&tr, ent, spot, MASK_SHOT))
	{
		traceEnt = &g_entities[tr.entityNum];
	}

	// shot is dead on
	if (traceEnt == ent)
	{
		return qtrue;
	}
	//MCG - Begin
	//ok, can't hit them in center, try their head
	CalcEntitySpot(ent, SPOT_HEAD, spot);
	trap->Trace(&tr, muzzle, NULL, NULL, spot, shooter->s.number, MASK_SHOT, qfalse, 0, 0);
	traceEnt = &g_entities[tr.entityNum];
	if (traceEnt == ent)
	{
		return qtrue;
	}

	//Actually, we should just check to fire in dir we're facing and if it's close enough,
	//and we didn't hit someone on our own team, shoot
	VectorSubtract(spot, tr.endpos, diff);
	if (VectorLength(diff) < Q_flrand(0.0f, 1.0f) * 32)
	{
		return qtrue;
	}
	//MCG - End
	// shot would hit a non-client
	if (!traceEnt->client)
	{
		return qfalse;
	}

	// shot is blocked by another player

	// he's already dead, so go ahead
	if (traceEnt->health <= 0)
	{
		return qtrue;
	}

	// don't deliberately shoot a teammate
	if (traceEnt->client && traceEnt->client->playerTeam == shooter->client->playerTeam)
	{
		return qfalse;
	}

	// he's just in the wrong place, go ahead
	return qtrue;
}

/*
void NPC_CheckPossibleEnemy( gentity_t *other, visibility_t vis )

Added: hacks for scripted NPCs
*/
void NPC_CheckPossibleEnemy(gentity_t* other, const visibility_t vis)
{
	// is he is already our enemy?
	if (other == NPCS.NPC->enemy)
		return;

	if (other->flags & FL_NOTARGET)
		return;

	// we already have an enemy and this guy is in our FOV, see if this guy would be better
	if (NPCS.NPC->enemy && vis == VIS_FOV)
	{
		if (NPCS.NPCInfo->enemyLastSeenTime - level.time < 2000)
		{
			return;
		}
		if (NPCS.enemyVisibility == VIS_UNKNOWN)
		{
			NPCS.enemyVisibility = NPC_CheckVisibility(NPCS.NPC->enemy, CHECK_360 | CHECK_FOV);
		}
		if (NPCS.enemyVisibility == VIS_FOV)
		{
			return;
		}
	}

	if (!NPCS.NPC->enemy)
	{
		//only take an enemy if you don't have one yet
		G_SetEnemy(NPCS.NPC, other);
	}

	if (vis == VIS_FOV)
	{
		NPCS.NPCInfo->enemyLastSeenTime = level.time;
		VectorCopy(other->r.currentOrigin, NPCS.NPCInfo->enemyLastSeenLocation);
		NPCS.NPCInfo->enemyLastHeardTime = 0;
		VectorClear(NPCS.NPCInfo->enemyLastHeardLocation);
	}
	else
	{
		NPCS.NPCInfo->enemyLastSeenTime = 0;
		VectorClear(NPCS.NPCInfo->enemyLastSeenLocation);
		NPCS.NPCInfo->enemyLastHeardTime = level.time;
		VectorCopy(other->r.currentOrigin, NPCS.NPCInfo->enemyLastHeardLocation);
	}
}

//==========================================
//MCG Added functions:
//==========================================

/*
int NPC_AttackDebounceForWeapon (void)

DOES NOT control how fast you can fire
Only makes you keep your weapon up after you fire

*/
int NPC_AttackDebounceForWeapon(void)
{
	switch (NPCS.NPC->client->ps.weapon)
	{
	case WP_SABER:
	{
		if ((NPCS.NPC->client->NPC_class == CLASS_KYLE || NPCS.NPC->client->NPC_class == CLASS_YODA)
			&& NPCS.NPC->spawnflags & 1)
		{
			return Q_irand(1500, 5000);
		}
		return 0;
	}
	default:
		return NPCS.NPCInfo->burstSpacing;
	}
}

//FIXME: need a mindist for explosive weapons
float NPC_MaxDistSquaredForWeapon(void)
{
	if (NPCS.NPCInfo->stats.shootDistance > 0)
	{
		//overrides default weapon dist
		return NPCS.NPCInfo->stats.shootDistance * NPCS.NPCInfo->stats.shootDistance;
	}

	switch (NPCS.NPC->s.weapon)
	{
	case WP_BLASTER: //scav rifle
		return 1024 * 1024; //should be shorter?

	case WP_BRYAR_PISTOL: //prifle
		return 1024 * 1024;

		/*
		case WP_BLASTER_PISTOL://prifle
			return 1024 * 1024;
			break;
			*/

	case WP_DISRUPTOR: //disruptor
	{
		if (NPCS.NPCInfo->scriptFlags & SCF_altFire)
		{
			return 4096 * 4096;
		}
		return 1024 * 1024;
	}
	/*
			case WP_SABER:
				return 1024 * 1024;
				break;
		*/
	case WP_SABER:
	{
		if (NPCS.NPC->client && NPCS.NPC->client->saber[0].blade[0].lengthMax)
		{
			//FIXME: account for whether enemy and I are heading towards each other!
			return (NPCS.NPC->client->saber[0].blade[0].lengthMax + NPCS.NPC->r.maxs[0] * 1.5) * (NPCS.NPC->
				client->
				saber[0].blade[0].lengthMax + NPCS.NPC->r.maxs[0] * 1.5);
		}
		return 48 * 48;
	}

	default:
		return 1024 * 1024; //was 0
	}
}

//This replaces ValidEnemy.
qboolean G_ValidEnemy(const gentity_t* self, const gentity_t* enemy)
{
	//Must be a valid pointer
	if (enemy == NULL)
		return qfalse;

	//Must not be me
	if (enemy == self)
		return qfalse;

	//Must not be deleted
	if (enemy->inuse == qfalse)
		return qfalse;

	//Must be alive
	if (enemy->health <= 0)
		return qfalse;

	//In case they're in notarget mode
	if (enemy->flags & FL_NOTARGET)
		return qfalse;

	//Must be an NPC
	if (enemy->client == NULL)
	{
		if (enemy->s.eType != ET_NPC)
		{
			//still potentially valid
			if (enemy->alliedTeam == self->client->playerTeam)
			{
				return qfalse;
			}
			return qtrue;
		}
		return qfalse;
	}
	if (enemy->client && enemy->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		//don't go after spectators
		return qfalse;
	}
	if (enemy->client && self->client->leader == enemy)
	{
		return qfalse;
	}

	if (enemy->client->playerTeam == NPCTEAM_FREE && enemy->s.number < MAX_CLIENTS)
	{
		//An evil player, everyone attacks him
		return qtrue;
	}

	if (self->client->playerTeam == NPCTEAM_SOLO || enemy->client->playerTeam == NPCTEAM_SOLO)
	{
		//	A new team which will attack anyone and everyone.
		return qtrue;
	}

	//Can't be on the same team
	if (enemy->client->playerTeam == self->client->playerTeam)
	{
		return qfalse;
	}

	if (enemy->client->playerTeam == self->client->enemyTeam //simplest case: they're on my enemy team
		|| self->client->enemyTeam == NPCTEAM_FREE && enemy->client->NPC_class != self->client->
		NPC_class
		//I get mad at anyone and this guy isn't the same class as me
		|| enemy->client->NPC_class == CLASS_WAMPA && enemy->enemy //a rampaging wampa
		|| enemy->client->NPC_class == CLASS_RANCOR && enemy->enemy //a rampaging rancor
		|| enemy->client->playerTeam == NPCTEAM_FREE && enemy->client->enemyTeam == NPCTEAM_FREE &&
		enemy->enemy
		&& enemy->enemy->client && (enemy->enemy->client->playerTeam == self->client->playerTeam
			|| enemy->enemy->client->playerTeam != NPCTEAM_ENEMY && self->client->playerTeam ==
			NPCTEAM_PLAYER))
	{
		return qtrue;
	}
	//all other cases = false?
	return qfalse;
}

/*
-------------------------
ValidEnemy
-------------------------
*/

qboolean NPC_ValidEnemy(const gentity_t* ent)
{
	return G_ValidEnemy(NPCS.NPC, ent);
}

qboolean ValidEnemy(const gentity_t* ent)
{
	return G_ValidEnemy(NPCS.NPC, ent);
}

qboolean NPC_EnemyTooFar(const gentity_t* enemy, float dist, const qboolean toShoot)
{
	if (!toShoot)
	{
		//Not trying to actually press fire button with this check
		if (NPCS.NPC->client->ps.weapon == WP_SABER)
		{
			//Just have to get to him
			return qfalse;
		}
	}

	if (!dist)
	{
		vec3_t vec;
		VectorSubtract(NPCS.NPC->r.currentOrigin, enemy->r.currentOrigin, vec);
		dist = VectorLengthSquared(vec);
	}

	if (dist > NPC_MaxDistSquaredForWeapon())
		return qtrue;

	return qfalse;
}

/*
NPC_PickEnemy

Randomly picks a living enemy from the specified team and returns it

FIXME: For now, you MUST specify an enemy team

If you specify choose closest, it will find only the closest enemy

If you specify checkVis, it will return and enemy that is visible

If you specify findPlayersFirst, it will try to find players first

You can mix and match any of those options (example: find closest visible players first)

FIXME: this should go through the snapshot and find the closest enemy
*/
gentity_t* NPC_PickEnemy(const gentity_t* closestTo, const int enemyTeam, const qboolean checkVis,
	const qboolean findPlayersFirst,
	const qboolean findClosest)
{
	int num_choices = 0;
	int choice[128]; //FIXME: need a different way to determine how many choices?
	gentity_t* newenemy;
	gentity_t* closestEnemy = NULL;
	vec3_t diff;
	float relDist;
	float bestDist = 2048.0f;
	qboolean failed = qfalse;
	int visChecks = CHECK_360 | CHECK_FOV | CHECK_VISRANGE;
	int minVis = VIS_FOV;

	if (enemyTeam == NPCTEAM_NEUTRAL)
	{
		return NULL;
	}

	if (NPCS.NPCInfo->behaviorState == BS_STAND_AND_SHOOT ||
		NPCS.NPCInfo->behaviorState == BS_HUNT_AND_KILL)
	{
		//Formations guys don't require inFov to pick up a target
		//These other behavior states are active battle states and should not
		//use FOV.  FOV checks are for enemies who are patrolling, guarding, etc.
		visChecks &= ~CHECK_FOV;
		minVis = VIS_360;
	}

	if (findPlayersFirst)
	{
		//try to find a player first
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			newenemy = &g_entities[i];
			//newenemy = &g_entities[0];
			if (newenemy->client && !(newenemy->flags & FL_NOTARGET) && !(newenemy->s.eFlags &
				EF_NODRAW))
			{
				//racc - this dude is valid and is targetable.
				if (newenemy->health > 0)
				{
					//enemy is alive.
					if (NPC_ValidEnemy(newenemy))
					{
						//FIXME:  check for range and FOV or vis?
						if (newenemy != NPCS.NPC->lastEnemy)
						{
							//Make sure we're not just going back and forth here
							if (trap->InPVS(newenemy->r.currentOrigin, NPCS.NPC->r.currentOrigin))
							{
								//racc - can possibly see this enemy.
								if (NPCS.NPCInfo->behaviorState == BS_INVESTIGATE || NPCS.NPCInfo->
									behaviorState
									== BS_PATROL)
								{
									//racc - specifically looking for enemies.
									if (!NPCS.NPC->enemy)
									{
										//racc - don't currently have an enemy.
										//racc - visibility checks.
										if (!InVisrange(newenemy))
										{
											failed = qtrue;
										}
										else if (NPC_CheckVisibility(
											newenemy, CHECK_360 | CHECK_FOV | CHECK_VISRANGE) !=
											VIS_FOV)
										{
											failed = qtrue;
										}
									}
								}

								if (!failed)
								{
									//racc - still good.
									VectorSubtract(closestTo->r.currentOrigin,
										newenemy->r.currentOrigin, diff);
									relDist = VectorLengthSquared(diff);
									if (newenemy->client->hiddenDist > 0)
									{
										//racc - enemy has a set distance at which they can be detected.
										if (relDist > newenemy->client->hiddenDist * newenemy->client->
											hiddenDist)
										{
											//racc - outside detection range.
											//out of hidden range
											if (VectorLengthSquared(newenemy->client->hiddenDir))
											{
												//They're only hidden from a certain direction, check
												VectorNormalize(diff);
												const float dot = DotProduct(
													newenemy->client->hiddenDir, diff);
												if (dot > 0.5)
												{
													//I'm not looking in the right dir toward them to see them
													failed = qtrue;
												}
												else
												{
													Debug_Printf(
														&d_npcai, DEBUG_LEVEL_INFO,
														"%s saw %s trying to hide - hiddenDir %s targetDir %s dot %f\n",
														NPCS.NPC->targetname, newenemy->targetname,
														vtos(newenemy->client->hiddenDir), vtos(diff),
														dot);
												}
											}
											else
											{
												failed = qtrue;
											}
										}
										else
										{
											Debug_Printf(&d_npcai, DEBUG_LEVEL_INFO,
												"%s saw %s trying to hide - hiddenDist %f\n",
												NPCS.NPC->targetname, newenemy->targetname,
												newenemy->client->hiddenDist);
										}
									}

									if (!failed)
									{
										if (findClosest)
										{
											//racc - if we're suppose to find the closest enemy.
											if (relDist < bestDist)
											{
												if (!NPC_EnemyTooFar(newenemy, relDist, qfalse))
												{
													if (checkVis)
													{
														if (NPC_CheckVisibility(newenemy, visChecks) ==
															minVis)
														{
															bestDist = relDist;
															closestEnemy = newenemy;
														}
													}
													else
													{
														bestDist = relDist;
														closestEnemy = newenemy;
													}
												}
											}
										}
										else if (!NPC_EnemyTooFar(newenemy, 0, qfalse))
										{
											if (checkVis)
											{
												if (NPC_CheckVisibility(
													newenemy,
													CHECK_360 | CHECK_FOV | CHECK_VISRANGE) ==
													VIS_FOV)
												{
													choice[num_choices++] = newenemy->s.number;
												}
											}
											else
											{
												choice[num_choices++] = newenemy->s.number;
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

	if (findClosest && closestEnemy)
	{
		return closestEnemy;
	}

	if (num_choices)
	{
		return &g_entities[choice[rand() % num_choices]];
	}

	num_choices = 0;
	bestDist = 2048.0f;
	closestEnemy = NULL;

	for (int entNum = 0; entNum < level.num_entities; entNum++)
	{
		newenemy = &g_entities[entNum];

		if (newenemy->client && !(newenemy->flags & FL_NOTARGET) && !(newenemy->s.eFlags & EF_NODRAW))
		{
			if (newenemy->health > 0)
			{
				if (newenemy->client && NPC_ValidEnemy(newenemy)
					|| !newenemy->client && newenemy->alliedTeam == enemyTeam)
				{
					//FIXME:  check for range and FOV or vis?
					if (NPCS.NPC->client->playerTeam == NPCTEAM_PLAYER && enemyTeam == NPCTEAM_PLAYER)
					{
						//player allies turning on ourselves?  How?
						if (newenemy->s.number >= MAX_CLIENTS)
						{
							//only turn on the player, not other player allies
							continue;
						}
					}

					if (newenemy != NPCS.NPC->lastEnemy)
					{
						//Make sure we're not just going back and forth here
						if (!trap->InPVS(newenemy->r.currentOrigin, NPCS.NPC->r.currentOrigin))
						{
							continue;
						}

						if (NPCS.NPCInfo->behaviorState == BS_INVESTIGATE || NPCS.NPCInfo->behaviorState
							==
							BS_PATROL)
						{
							if (!NPCS.NPC->enemy)
							{
								if (!InVisrange(newenemy))
								{
									continue;
								}
								if (NPC_CheckVisibility(
									newenemy, CHECK_360 | CHECK_FOV | CHECK_VISRANGE) !=
									VIS_FOV)
								{
									continue;
								}
							}
						}

						VectorSubtract(closestTo->r.currentOrigin, newenemy->r.currentOrigin, diff);
						relDist = VectorLengthSquared(diff);
						if (newenemy->client && newenemy->client->hiddenDist > 0)
						{
							if (relDist > newenemy->client->hiddenDist * newenemy->client->hiddenDist)
							{
								//out of hidden range
								if (VectorLengthSquared(newenemy->client->hiddenDir))
								{
									//They're only hidden from a certain direction, check

									VectorNormalize(diff);
									const float dot = DotProduct(newenemy->client->hiddenDir, diff);
									if (dot > 0.5)
									{
										//I'm not looking in the right dir toward them to see them
										continue;
									}
									Debug_Printf(&d_npcai, DEBUG_LEVEL_INFO,
										"%s saw %s trying to hide - hiddenDir %s targetDir %s dot %f\n",
										NPCS.NPC->targetname, newenemy->targetname,
										vtos(newenemy->client->hiddenDir), vtos(diff), dot);
								}
								else
								{
									continue;
								}
							}
							else
							{
								Debug_Printf(&d_npcai, DEBUG_LEVEL_INFO,
									"%s saw %s trying to hide - hiddenDist %f\n",
									NPCS.NPC->targetname,
									newenemy->targetname, newenemy->client->hiddenDist);
							}
						}

						if (findClosest)
						{
							if (relDist < bestDist)
							{
								if (!NPC_EnemyTooFar(newenemy, relDist, qfalse))
								{
									if (checkVis)
									{
										if (NPC_CheckVisibility(newenemy, visChecks) == minVis)
										{
											bestDist = relDist;
											closestEnemy = newenemy;
										}
									}
									else
									{
										bestDist = relDist;
										closestEnemy = newenemy;
									}
								}
							}
						}
						else if (!NPC_EnemyTooFar(newenemy, 0, qfalse))
						{
							if (checkVis)
							{
								if (NPC_CheckVisibility(newenemy, CHECK_360 | CHECK_VISRANGE) >=
									VIS_360)
								{
									choice[num_choices++] = newenemy->s.number;
								}
							}
							else
							{
								choice[num_choices++] = newenemy->s.number;
							}
						}
					}
				}
			}
		}
	}

	if (findClosest)
	{
		//FIXME: you can pick up an enemy around a corner this way.
		NPCS.NPC->NPC->enemyLastSeenTime = level.time;
		return closestEnemy;
	}

	if (!num_choices)
	{
		return NULL;
	}

	return &g_entities[choice[rand() % num_choices]];
}

gentity_t* NPC_PickAlly(const qboolean facingEachOther, const float range, const qboolean ignoreGroup,
	const qboolean movingOnly)
{
	gentity_t* closestAlly = NULL;
	float bestDist = range;

	for (int entNum = 0; entNum < level.num_entities; entNum++)
	{
		gentity_t* ally = &g_entities[entNum];

		if (ally->client)
		{
			if (ally->health > 0)
			{
				if (ally->client && (ally->client->playerTeam == NPCS.NPC->client->playerTeam ||
					NPCS.NPC->client->playerTeam == NPCTEAM_ENEMY))
				{
					vec3_t diff;
					//if on same team or if player is disguised as your team
					if (ignoreGroup)
					{
						if (ally == NPCS.NPC->client->leader)
						{
							//reject
							continue;
						}
						if (ally->client && ally->client->leader && ally->client->leader == NPCS.NPC)
						{
							//reject
							continue;
						}
					}

					if (!trap->InPVS(ally->r.currentOrigin, NPCS.NPC->r.currentOrigin))
					{
						continue;
					}

					if (movingOnly && ally->client && NPCS.NPC->client)
					{
						//They have to be moving relative to each other
						if (!DistanceSquared(ally->client->ps.velocity, NPCS.NPC->client->ps.velocity))
						{
							continue;
						}
					}

					VectorSubtract(NPCS.NPC->r.currentOrigin, ally->r.currentOrigin, diff);
					const float relDist = VectorNormalize(diff);
					if (relDist < bestDist)
					{
						if (facingEachOther)
						{
							vec3_t vf;

							AngleVectors(ally->client->ps.viewangles, vf, NULL, NULL);
							VectorNormalize(vf);
							float dot = DotProduct(diff, vf);

							if (dot < 0.5)
							{
								//Not facing in dir to me
								continue;
							}
							//He's facing me, am I facing him?
							AngleVectors(NPCS.NPC->client->ps.viewangles, vf, NULL, NULL);
							VectorNormalize(vf);
							dot = DotProduct(diff, vf);

							if (dot > -0.5)
							{
								//I'm not facing opposite of dir to me
								continue;
							}
							//I am facing him
						}

						if (NPC_CheckVisibility(ally, CHECK_360 | CHECK_VISRANGE) >= VIS_360)
						{
							bestDist = relDist;
							closestAlly = ally;
						}
					}
				}
			}
		}
	}

	return closestAlly;
}

gentity_t* NPC_CheckEnemy(const qboolean find_new, const qboolean too_far_ok, const qboolean set_enemy)
{
	qboolean forcefindNew = qfalse;
	gentity_t* newEnemy = NULL;

	if (NPCS.NPC->enemy)
	{
		if (!NPCS.NPC->enemy->inuse)
			//|| NPC->enemy == NPC )//wtf?  NPCs should never get mad at themselves!
		{
			if (set_enemy)
			{
				G_ClearEnemy(NPCS.NPC);
			}
		}
	}

	if (NPCS.NPC->NPC->scriptFlags & SCF_IGNORE_ENEMIES)
	{
		//We're ignoring all enemies for now
		if (set_enemy)
		{
			G_ClearEnemy(NPCS.NPC);
		}
		return NULL;
	}

	// Kyle does not get new enemies if not close to his leader
	if (NPCS.NPC->client->NPC_class == CLASS_KYLE &&
		NPCS.NPC->client->leader &&
		Distance(NPCS.NPC->client->leader->r.currentOrigin, NPCS.NPC->r.currentOrigin) > 3000)
	{
		if (NPCS.NPC->enemy)
		{
			G_ClearEnemy(NPCS.NPC);
		}
		return NULL;
	}

	if (NPCS.NPC->NPC->aiFlags & NPCAI_LOCKEDENEMY)
	{
		//keep this enemy until dead
		if (NPCS.NPC->enemy)
		{
			if (!NPCS.NPC->NPC
				&& NPCS.NPC->enemy->s.eType == ET_NPC
				|| NPCS.NPC->enemy->health > 0)
			{
				//Enemy never had health (a train or info_not_null, etc) or did and is now dead (NPCs, turrets, etc)
				return NULL;
			}
		}
		NPCS.NPC->NPC->aiFlags &= ~NPCAI_LOCKEDENEMY;
	}

	if (NPCS.NPC->enemy)
	{
		if (NPC_EnemyTooFar(NPCS.NPC->enemy, 0, qfalse))
		{
			if (find_new)
			{
				//See if there is a close one and take it if so, else keep this one
				forcefindNew = qtrue;
			}
			else if (!too_far_ok) //FIXME: don't need this extra bool any more
			{
				if (set_enemy)
				{
					G_ClearEnemy(NPCS.NPC);
				}
			}
		}
		else if (!trap->InPVS(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin))
		{
			//FIXME: should this be a line-of site check?
			//FIXME: a lot of things check PVS AGAIN when deciding whether
			//or not to shoot, redundant!
			//Should we lose the enemy?
			//FIXME: if lose enemy, run lostenemyscript
			if (NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->hiddenDist)
			{
				//He ducked into shadow while we weren't looking
				//Drop enemy and see if we should search for him
				NPC_LostEnemyDecideChase();
			}
			else
			{
				//If we're not chasing him, we need to lose him
				//NOTE: since we no longer have bStates, really, this logic doesn't work, so never give him up

				/*
				switch( NPCInfo->behaviorState )
				{
				case BS_HUNT_AND_KILL:
					//Okay to lose PVS, we're chasing them
					break;
				case BS_RUN_AND_SHOOT:
				//FIXME: only do this if !(NPCInfo->scriptFlags&SCF_CHASE_ENEMY)
					//If he's not our goalEntity, we're running somewhere else, so lose him
					if ( NPC->enemy != NPCInfo->goalEntity )
					{
						G_ClearEnemy( NPC );
					}
					break;
				default:
					//We're not chasing him, so lose him as an enemy
					G_ClearEnemy( NPC );
					break;
				}
				*/
			}
		}
	}

	if (NPCS.NPC->enemy)
	{
		if (NPCS.NPC->enemy->health <= 0 || NPCS.NPC->enemy->flags & FL_NOTARGET)
		{
			if (set_enemy)
			{
				G_ClearEnemy(NPCS.NPC);
			}
		}
	}

	const gentity_t* closestTo = NPCS.NPC;
	//FIXME: check your defendEnt, if you have one, see if their enemy is different
	//than yours, or, if they don't have one, pick the closest enemy to THEM?
	if (NPCS.NPCInfo->defendEnt)
	{
		//Trying to protect someone
		if (NPCS.NPCInfo->defendEnt->health > 0)
		{
			//Still alive, We presume we're close to them, navigation should handle this?
			if (NPCS.NPCInfo->defendEnt->enemy)
			{
				//They were shot or acquired an enemy
				if (NPCS.NPC->enemy != NPCS.NPCInfo->defendEnt->enemy)
				{
					//They have a different enemy, take it!
					newEnemy = NPCS.NPCInfo->defendEnt->enemy;
					if (set_enemy)
					{
						G_SetEnemy(NPCS.NPC, NPCS.NPCInfo->defendEnt->enemy);
					}
				}
			}
			else if (NPCS.NPC->enemy == NULL)
			{
				//We don't have an enemy, so find closest to defendEnt
				closestTo = NPCS.NPCInfo->defendEnt;
			}
		}
	}

	if (!NPCS.NPC->enemy || NPCS.NPC->enemy && NPCS.NPC->enemy->health <= 0 || forcefindNew)
	{
		//FIXME: NPCs that are moving after an enemy should ignore the can't hit enemy counter- that should only be for NPCs that are standing still
		//NOTE: cantHitEnemyCounter >= 100 means we couldn't hit enemy for a full
		//	10 seconds, so give up.  This means even if we're chasing him, we would
		//	try to find another enemy after 10 seconds (assuming the cantHitEnemyCounter
		//	is allowed to increment in a chasing b_state)
		qboolean foundenemy = qfalse;

		if (!find_new)
		{
			if (set_enemy)
			{
				NPCS.NPC->lastEnemy = NPCS.NPC->enemy;
				G_ClearEnemy(NPCS.NPC);
			}
			return NULL;
		}

		//If enemy dead or unshootable, look for others on out enemy's team
		if (NPCS.NPC->client->enemyTeam != NPCTEAM_NEUTRAL)
		{
			//NOTE:  this only checks vis if can't hit enemy for 10 tries, which I suppose
			//			means they need to find one that in more than just PVS
			//newenemy = NPC_PickEnemy( closestTo, NPC->client->enemyTeam, (NPC->cantHitEnemyCounter > 10), qfalse, qtrue );//3rd parm was (NPC->enemyTeam == TEAM_STARFLEET)
			//For now, made it so you ALWAYS have to check VIS
			newEnemy = NPC_PickEnemy(closestTo, NPCS.NPC->client->enemyTeam, qtrue, qfalse, qtrue);
			//3rd parm was (NPC->enemyTeam == TEAM_STARFLEET)
			if (newEnemy)
			{
				foundenemy = qtrue;
				if (set_enemy)
				{
					G_SetEnemy(NPCS.NPC, newEnemy);
				}
			}
		}

		//if ( !forcefindNew )
		{
			if (!foundenemy)
			{
				if (set_enemy)
				{
					NPCS.NPC->lastEnemy = NPCS.NPC->enemy;
					G_ClearEnemy(NPCS.NPC);
				}
			}

			NPCS.NPC->cantHitEnemyCounter = 0;
		}
		//FIXME: if we can't find any at all, go into independent NPC AI, pursue and kill
	}

	if (NPCS.NPC->enemy && NPCS.NPC->enemy->client)
	{
		if (NPCS.NPC->enemy->client->playerTeam)
		{
			//racc - NPCTEAM_FREE players don't do this.
			if (NPCS.NPC->client->playerTeam != NPCS.NPC->enemy->client->playerTeam
				&& NPCS.NPC->client->enemyTeam != (npcteam_t)TEAM_FREE
				&& NPCS.NPC->client->enemyTeam != NPCS.NPC->enemy->client->playerTeam)
			{
				//racc - We're not attacking an ally, this isn't a NPCTEAM_FREE enemy,
				//and our enemy is on a team other than our normal enemyTeam.
				//As such, switch our enemyTeam.
				NPCS.NPC->client->enemyTeam = NPCS.NPC->enemy->client->playerTeam;
			}
		}
	}
	return newEnemy;
}

/*
-------------------------
NPC_ClearShot
-------------------------
*/

qboolean NPC_ClearShot(const gentity_t* ent)
{
	vec3_t muzzle;
	trace_t tr;

	if (NPCS.NPC == NULL || ent == NULL)
		return qfalse;

	CalcEntitySpot(NPCS.NPC, SPOT_WEAPON, muzzle);

	// add aim error
	// use weapon instead of specific npc types, although you could add certain npc classes if you wanted
	if (NPCS.NPC->s.weapon == WP_BLASTER) // any other guns to check for?
	{
		const vec3_t mins = { -2, -2, -2 };
		const vec3_t maxs = { 2, 2, 2 };

		trap->Trace(&tr, muzzle, mins, maxs, ent->r.currentOrigin, NPCS.NPC->s.number, MASK_SHOT,
			qfalse, 0, 0);
	}
	else
	{
		trap->Trace(&tr, muzzle, NULL, NULL, ent->r.currentOrigin, NPCS.NPC->s.number, MASK_SHOT,
			qfalse, 0, 0);
	}

	if (tr.startsolid || tr.allsolid)
	{
		return qfalse;
	}

	if (tr.entityNum == ent->s.number)
		return qtrue;

	return qfalse;
}

/*
-------------------------
NPC_ShotEntity
-------------------------
*/

int NPC_ShotEntity(const gentity_t* ent, vec3_t impactPos)
{
	vec3_t muzzle;
	vec3_t targ;
	trace_t tr;

	if (!NPCS.NPC || !ent)
		return qfalse;

	if (NPCS.NPC->s.weapon == WP_THERMAL)
	{
		//thermal aims from slightly above head
		//FIXME: what about low-angle shots, rolling the thermal under something?
		vec3_t angles, forward, end;

		CalcEntitySpot(NPCS.NPC, SPOT_HEAD, muzzle);
		VectorSet(angles, 0, NPCS.NPC->client->ps.viewangles[1], 0);
		AngleVectors(angles, forward, NULL, NULL);
		VectorMA(muzzle, 8, forward, end);
		end[2] += 24;
		trap->Trace(&tr, muzzle, vec3_origin, vec3_origin, end, NPCS.NPC->s.number, MASK_SHOT, qfalse,
			0, 0);
		VectorCopy(tr.endpos, muzzle);
	}
	else
	{
		CalcEntitySpot(NPCS.NPC, SPOT_WEAPON, muzzle);
	}
	CalcEntitySpot(ent, SPOT_CHEST, targ);

	// add aim error
	// use weapon instead of specific npc types, although you could add certain npc classes if you wanted
	if (NPCS.NPC->s.weapon == WP_BLASTER) // any other guns to check for?
	{
		const vec3_t mins = { -2, -2, -2 };
		const vec3_t maxs = { 2, 2, 2 };

		trap->Trace(&tr, muzzle, mins, maxs, targ, NPCS.NPC->s.number, MASK_SHOT, qfalse, 0, 0);
	}
	else
	{
		trap->Trace(&tr, muzzle, NULL, NULL, targ, NPCS.NPC->s.number, MASK_SHOT, qfalse, 0, 0);
	}
	//FIXME: if using a bouncing weapon like the bowcaster, should we check the reflection of the wall, too?
	if (impactPos)
	{
		//they want to know *where* the hit would be, too
		VectorCopy(tr.endpos, impactPos);
	}
	/* // NPCs should be able to shoot even if the muzzle would be inside their target
		if ( tr.startsolid || tr.allsolid )
		{
			return ENTITYNUM_NONE;
		}
	*/
	return tr.entityNum;
}

qboolean NPC_EvaluateShot(const int hit, qboolean glassOK)
{
	if (!NPCS.NPC->enemy)
	{
		return qfalse;
	}

	if (hit == NPCS.NPC->enemy->s.number || &g_entities[hit] != NULL && g_entities[hit].r.svFlags &
		SVF_GLASS_BRUSH)
	{
		//can hit enemy or will hit glass, so shoot anyway
		return qtrue;
	}
	return qfalse;
}

/*
NPC_CheckAttack

Simply checks aggression and returns true or false
*/

qboolean NPC_CheckAttack(float scale)
{
	if (!scale)
		scale = 1.0;

	if ((float)NPCS.NPCInfo->stats.aggression * scale < flrand(0, 4))
	{
		return qfalse;
	}

	if (NPCS.NPCInfo->shotTime > level.time)
		return qfalse;

	return qtrue;
}

/*
NPC_CheckDefend

Simply checks evasion and returns true or false
*/

qboolean NPC_CheckDefend(float scale)
{
	if (!scale)
		scale = 1.0;

	if ((float)NPCS.NPCInfo->stats.evasion > Q_flrand(0.0f, 1.0f) * 4 * scale)
		return qtrue;

	return qfalse;
}

//NOTE: BE SURE TO CHECK PVS BEFORE THIS!
qboolean NPC_CheckCanAttack(float attack_scale, qboolean stationary)
{
	vec3_t delta;
	vec3_t angleToEnemy;
	vec3_t muzzle, enemy_org; //, enemy_head;
	qboolean attack_ok = qfalse;
	//	qboolean	duck_ok = qfalse;
	qboolean dead_on = qfalse;
	const float max_aim_off = 128 - 16 * (float)NPCS.NPCInfo->stats.aim;
	trace_t tr;
	const gentity_t* traceEnt = NULL;

	if (NPCS.NPC->enemy->flags & FL_NOTARGET)
	{
		return qfalse;
	}

	//FIXME: only check to see if should duck if that provides cover from the
	//enemy!!!
	if (!attack_scale)
	{
		attack_scale = 1.0;
	}
	//Yaw to enemy
	CalcEntitySpot(NPCS.NPC->enemy, SPOT_HEAD, enemy_org);
	NPC_AimWiggle(enemy_org);

	CalcEntitySpot(NPCS.NPC, SPOT_WEAPON, muzzle);

	VectorSubtract(enemy_org, muzzle, delta);
	vectoangles(delta, angleToEnemy);
	const float distanceToEnemy = VectorNormalize(delta);

	NPCS.NPC->NPC->desiredYaw = angleToEnemy[YAW];
	NPC_UpdateFiringAngles(qfalse, qtrue);

	if (NPC_EnemyTooFar(NPCS.NPC->enemy, distanceToEnemy * distanceToEnemy, qtrue))
	{
		//Too far away?  Do not attack
		return qfalse;
	}

	if (NPCS.client->ps.weaponTime > 0)
	{
		//already waiting for a shot to fire
		NPCS.NPC->NPC->desiredPitch = angleToEnemy[PITCH];
		NPC_UpdateFiringAngles(qtrue, qfalse);
		return qfalse;
	}

	if (NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE)
	{
		return qfalse;
	}

	NPCS.NPCInfo->enemyLastVisibility = NPCS.enemyVisibility;
	//See if they're in our FOV and we have a clear shot to them
	NPCS.enemyVisibility = NPC_CheckVisibility(NPCS.NPC->enemy, CHECK_360 | CHECK_FOV); ////CHECK_PVS|

	if (NPCS.enemyVisibility >= VIS_FOV)
	{
		vec3_t hitspot;
		vec3_t forward;
		//He's in our FOV
		attack_ok = qtrue;
		//CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemy_head);

		//Check to duck
		if (NPCS.NPC->enemy->client)
		{
			if (NPCS.NPC->enemy->enemy == NPCS.NPC)
			{
				if (NPCS.NPC->enemy->client->buttons & BUTTON_ATTACK)
				{
					//FIXME: determine if enemy fire angles would hit me or get close
					if (NPC_CheckDefend(1.0)) //FIXME: Check self-preservation?  Health?
					{
						//duck and don't shoot
						attack_ok = qfalse;
						NPCS.ucmd.upmove = -127;
					}
				}
			}
		}

		if (attack_ok)
		{
			//are we gonna hit him
			//NEW: use actual forward facing
			AngleVectors(NPCS.client->ps.viewangles, forward, NULL, NULL);
			VectorMA(muzzle, distanceToEnemy, forward, hitspot);
			trap->Trace(&tr, muzzle, NULL, NULL, hitspot, NPCS.NPC->s.number, MASK_SHOT, qfalse, 0, 0);
			ShotThroughGlass(&tr, NPCS.NPC->enemy, hitspot, MASK_SHOT);
			/*
			//OLD: trace regardless of facing
			trap->Trace ( &tr, muzzle, NULL, NULL, enemy_org, NPC->s.number, MASK_SHOT );
			ShotThroughGlass(&tr, NPC->enemy, enemy_org, MASK_SHOT);
			*/

			traceEnt = &g_entities[tr.entityNum];

			/*
			if( traceEnt != NPC->enemy &&//FIXME: if someone on our team is in the way, suggest that they duck if possible
				(!traceEnt || !traceEnt->client || !NPC->client->enemyTeam || NPC->client->enemyTeam != traceEnt->client->playerTeam) )
			{//no, so shoot for somewhere between the head and torso
				//NOTE: yes, I know this looks weird, but it works
				enemy_org[0] += 0.3*Q_flrand(NPC->enemy->r.mins[0], NPC->enemy->r.maxs[0]);
				enemy_org[1] += 0.3*Q_flrand(NPC->enemy->r.mins[1], NPC->enemy->r.maxs[1]);
				enemy_org[2] -= NPC->enemy->r.maxs[2]*Q_flrand(0.0f, 1.0f);

				attack_scale *= 0.75;
				trap->Trace ( &tr, muzzle, NULL, NULL, enemy_org, NPC->s.number, MASK_SHOT );
				ShotThroughGlass(&tr, NPC->enemy, enemy_org, MASK_SHOT);
				traceEnt = &g_entities[tr.entityNum];
			}
			*/

			VectorCopy(tr.endpos, hitspot);

			if (traceEnt == NPCS.NPC->enemy || traceEnt->client && NPCS.NPC->client->enemyTeam && NPCS.
				NPC->
				client->enemyTeam == traceEnt->client->playerTeam)
			{
				dead_on = qtrue;
			}
			else
			{
				attack_scale *= 0.5;
				if (NPCS.NPC->client->playerTeam)
				{
					if (traceEnt && traceEnt->client && traceEnt->client->playerTeam)
					{
						if (NPCS.NPC->client->playerTeam == traceEnt->client->playerTeam)
						{
							//Don't shoot our own team
							attack_ok = qfalse;
						}
					}
				}
			}
		}

		if (attack_ok)
		{
			//ok, now adjust pitch aim
			VectorSubtract(hitspot, muzzle, delta);
			vectoangles(delta, angleToEnemy);
			NPCS.NPC->NPC->desiredPitch = angleToEnemy[PITCH];
			NPC_UpdateFiringAngles(qtrue, qfalse);

			if (!dead_on)
			{
				//We're not going to hit him directly, try a suppressing fire
				//see if where we're going to shoot is too far from his origin
				if (traceEnt && (traceEnt->health <= 30 || EntIsGlass(traceEnt)))
				{
					//easy to kill - go for it
					//if(traceEnt->die == ExplodeDeath_Wait && traceEnt->splashDamage)
				}
				else
				{
					vec3_t diff;
					AngleVectors(NPCS.client->ps.viewangles, forward, NULL, NULL);
					VectorMA(muzzle, distanceToEnemy, forward, hitspot);
					VectorSubtract(hitspot, enemy_org, diff);
					float aim_off = VectorLength(diff);
					if (aim_off > Q_flrand(0.0f, 1.0f) * max_aim_off)
						//FIXME: use aim value to allow poor aim?
					{
						attack_scale *= 0.75;
						//see if where we're going to shoot is too far from his head
						VectorSubtract(hitspot, enemy_org, diff);
						aim_off = VectorLength(diff);
						if (aim_off > Q_flrand(0.0f, 1.0f) * max_aim_off)
						{
							attack_ok = qfalse;
						}
					}
					attack_scale *= (max_aim_off - aim_off + 1) / max_aim_off;
				}
			}
		}
	}
	else
	{
		//Update pitch anyway
		NPCS.NPC->NPC->desiredPitch = angleToEnemy[PITCH];
		NPC_UpdateFiringAngles(qtrue, qfalse);
	}

	if (attack_ok)
	{
		if (NPC_CheckAttack(attack_scale))
		{
			//check aggression to decide if we should shoot
			NPCS.enemyVisibility = VIS_SHOOT;
			WeaponThink();
		}
		else
			attack_ok = qfalse;
	}

	return attack_ok;
}

//========================================================================================
//OLD id-style hunt and kill
//========================================================================================
/*
ideal_distance

determines what the NPC's ideal distance from it's enemy should
be in the current situation
*/
float ideal_distance()
{
	float ideal = 225 - 20 * NPCS.NPCInfo->stats.aggression;
	switch (NPCS.NPC->s.weapon)
	{
	case WP_ROCKET_LAUNCHER:
		ideal += 200;
		break;
	case WP_CONCUSSION:
		ideal += 200;
		break;

	case WP_THERMAL:
		ideal += 50;
		break;

	case WP_SABER:
	case WP_BRYAR_PISTOL:
		//	case WP_BLASTER_PISTOL:
	case WP_BLASTER:
	default:
		break;
	}

	return ideal;
}

/*QUAKED point_combat (0.7 0 0.7) (-16 -16 -24) (16 16 32) DUCK FLEE INVESTIGATE SQUAD LEAN SNIPE
NPCs in b_state BS_COMBAT_POINT will find their closest empty combat_point

DUCK - NPC will duck and fire from this point, NOT IMPLEMENTED?
FLEE - Will choose this point when running
INVESTIGATE - Will look here if a sound is heard near it
SQUAD - NOT IMPLEMENTED
LEAN - Lean-type cover, NOT IMPLEMENTED
SNIPE - Snipers look for these first, NOT IMPLEMENTED
*/

void SP_point_combat(gentity_t* self)
{
	if (level.numCombatPoints >= MAX_COMBAT_POINTS)
	{
#ifndef FINAL_BUILD
		Com_Printf(S_COLOR_RED"ERROR:  Too many combat points, limit is %d\n", MAX_COMBAT_POINTS);
#endif
		G_FreeEntity(self);
		return;
	}

	self->s.origin[2] += 0.125;
	G_SetOrigin(self, self->s.origin);
	trap->LinkEntity((sharedEntity_t*)self);

	if (G_CheckInSolid(self, qtrue))
	{
#ifndef FINAL_BUILD
		Com_Printf(S_COLOR_RED"ERROR: combat point at %s in solid!\n", vtos(self->r.currentOrigin));
#endif
	}

	VectorCopy(self->r.currentOrigin, level.combatPoints[level.numCombatPoints].origin);

	level.combatPoints[level.numCombatPoints].flags = self->spawnflags;
	level.combatPoints[level.numCombatPoints].occupied = qfalse;

	level.numCombatPoints++;

	G_FreeEntity(self);
}

void CP_FindCombatPointWaypoints(void)
{
	for (int i = 0; i < level.numCombatPoints; i++)
	{
		level.combatPoints[i].waypoint = NAV_FindClosestWaypointForPoint2(level.combatPoints[i].origin);
#ifndef FINAL_BUILD
		if (level.combatPoints[i].waypoint == WAYPOINT_NONE)
		{
			Com_Printf(S_COLOR_RED"ERROR: Combat Point at %s has no waypoint!\n", vtos(level.combatPoints[i].origin));
		}
#endif
	}
}

/*
-------------------------
NPC_CollectCombatPoints
-------------------------
*/
typedef struct combatPt_s
{
	float dist;
	int index;
} combatPt_t;

static int NPC_CollectCombatPoints(const vec3_t origin, const float radius, combatPt_t* points,
	const int flags)
{
	const float radiusSqr = radius * radius;
	float distance;
	float bestDistance = Q3_INFINITE;
	int numPoints = 0;

	//Collect all nearest
	for (int i = 0; i < level.numCombatPoints; i++)
	{
		if (numPoints >= MAX_COMBAT_POINTS)
		{
			break;
		}

		//Must be vacant
		if (level.combatPoints[i].occupied == qtrue)
			continue;

		//If we want a duck space, make sure this is one
		if (flags & CP_DUCK && level.combatPoints[i].flags & CPF_DUCK)
			continue;

		//If we want a duck space, make sure this is one
		if (flags & CP_FLEE && level.combatPoints[i].flags & CPF_FLEE)
			continue;

		///Make sure this is an investigate combat point
		if (flags & CP_INVESTIGATE && level.combatPoints[i].flags & CPF_INVESTIGATE)
			continue;

		//Squad points are only valid if we're looking for them
		if (level.combatPoints[i].flags & CPF_SQUAD && (flags & CP_SQUAD) == qfalse)
			continue;

		if (flags & CP_NO_PVS)
		{
			//must not be within PVS of mu current origin
			if (trap->InPVS(origin, level.combatPoints[i].origin))
			{
				continue;
			}
		}

		if (flags & CP_HORZ_DIST_COLL)
		{
			distance = DistanceHorizontalSquared(origin, level.combatPoints[i].origin);
		}
		else
		{
			distance = DistanceSquared(origin, level.combatPoints[i].origin);
		}

		if (distance < radiusSqr)
		{
			if (distance < bestDistance)
			{
				bestDistance = distance;
			}

			points[numPoints].dist = distance;
			points[numPoints].index = i;
			numPoints++;
		}
	}

	return numPoints;
}

/*
-------------------------
NPC_FindCombatPoint
-------------------------
*/

#define	MIN_AVOID_DOT				0.75f
#define MIN_AVOID_DISTANCE			128
#define MIN_AVOID_DISTANCE_SQUARED	( MIN_AVOID_DISTANCE * MIN_AVOID_DISTANCE )
#define	CP_COLLECT_RADIUS			512.0f

int NPC_FindCombatPoint(const vec3_t position, const vec3_t avoidPosition, vec3_t dest_position,
	const int flags, const float avoidDist, const int ignorePoint)
{
	combatPt_t points[MAX_COMBAT_POINTS];
	int best = -1, bestCost = Q3_INFINITE, waypoint = WAYPOINT_NONE;
	float dist;
	trace_t tr;
	float collRad = CP_COLLECT_RADIUS;
	float modifiedAvoidDist = avoidDist;

	if (modifiedAvoidDist <= 0)
	{
		modifiedAvoidDist = MIN_AVOID_DISTANCE_SQUARED;
	}
	else
	{
		modifiedAvoidDist *= modifiedAvoidDist;
	}

	if (flags & CP_HAS_ROUTE || flags & CP_NEAREST)
	{
		//going to be doing macro nav tests
		if (NPCS.NPC->waypoint == WAYPOINT_NONE)
		{
			waypoint = NAV_GetNearestNode(NPCS.NPC, NPCS.NPC->lastWaypoint);
		}
		else
		{
			waypoint = NPCS.NPC->waypoint;
		}
	}

	//Collect our nearest points
	if (flags & CP_NO_PVS)
	{
		//much larger radius since most will be dropped?
		collRad = CP_COLLECT_RADIUS * 4;
	}
	const int numPoints = NPC_CollectCombatPoints(dest_position, collRad, points, flags); //position

	for (int j = 0; j < numPoints; j++)
	{
		//const int i = (*cpi).second;
		const int i = points[j].index;
		const float pdist = points[j].dist;

		//Must not be one we want to ignore
		if (i == ignorePoint)
			continue;

		//FIXME: able to mark certain ones as too dangerous to go to for now?  Like a tripmine/thermal/detpack is near or something?
		//If we need a cover point, check this point
		if (flags & CP_COVER && NPC_ClearLOS(level.combatPoints[i].origin, dest_position) == qtrue)
			//Used to use NPC->enemy
			continue;

		//Need a clear LOS to our target... and be within shot range to enemy position (FIXME: make this a separate CS_ flag? and pass in a range?)
		if (flags & CP_CLEAR)
		{
			if (NPC_ClearLOS3(level.combatPoints[i].origin, NPCS.NPC->enemy) == qfalse)
			{
				continue;
			}
			if (NPCS.NPC->s.weapon == WP_THERMAL)
			{
				//horizontal
				dist = DistanceHorizontalSquared(level.combatPoints[i].origin,
					NPCS.NPC->enemy->r.currentOrigin);
			}
			else
			{
				//actual
				dist = DistanceSquared(level.combatPoints[i].origin, NPCS.NPC->enemy->r.currentOrigin);
			}
			if (dist > NPCS.NPCInfo->stats.visrange * NPCS.NPCInfo->stats.visrange)
			{
				continue;
			}
		}

		//Avoid this position?
		if (flags & CP_AVOID && DistanceSquared(level.combatPoints[i].origin, position) <
			modifiedAvoidDist)
			//was using MIN_AVOID_DISTANCE_SQUARED, not passed in modifiedAvoidDist
			continue;

		//Try to find a point closer to the enemy than where we are
		if (flags & CP_APPROACH_ENEMY)
		{
			if (flags & CP_HORZ_DIST_COLL)
			{
				if (pdist > DistanceHorizontalSquared(position, dest_position))
				{
					continue;
				}
			}
			else
			{
				if (pdist > DistanceSquared(position, dest_position))
				{
					continue;
				}
			}
		}
		//Try to find a point farther from the enemy than where we are
		if (flags & CP_RETREAT)
		{
			if (flags & CP_HORZ_DIST_COLL)
			{
				if (pdist < DistanceHorizontalSquared(position, dest_position))
				{
					//it's closer, don't use it
					continue;
				}
			}
			else
			{
				if (pdist < DistanceSquared(position, dest_position))
				{
					//it's closer, don't use it
					continue;
				}
			}
		}

		//We want a point on other side of the enemy from current pos
		if (flags & CP_FLANK)
		{
			vec3_t eDir2Me, eDir2CP;

			VectorSubtract(position, dest_position, eDir2Me);
			VectorNormalize(eDir2Me);

			VectorSubtract(level.combatPoints[i].origin, dest_position, eDir2CP);
			VectorNormalize(eDir2CP);

			const float dot = DotProduct(eDir2Me, eDir2CP);

			//Not far enough behind enemy from current pos
			if (dot >= 0.4)
				continue;
		}

		//See if we're trying to avoid our enemy
		if (flags & CP_AVOID_ENEMY)
		{
			vec3_t eDir, gDir;
			vec3_t wpOrg;

			VectorSubtract(position, dest_position, eDir);
			VectorNormalize(eDir);
			VectorCopy(level.combatPoints[i].origin, wpOrg);
			VectorSubtract(position, wpOrg, gDir);
			VectorNormalize(gDir);

			const float dot = DotProduct(gDir, eDir);

			//Don't want to run at enemy
			if (dot >= MIN_AVOID_DOT)
				continue;

			//Can't be too close to the enemy
			if (DistanceSquared(wpOrg, dest_position) < modifiedAvoidDist)
				continue;
		}

		//Okay, now make sure it's not blocked
		trap->Trace(&tr, level.combatPoints[i].origin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs,
			level.combatPoints[i].origin, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0);
		if (tr.allsolid || tr.startsolid)
		{
			continue;
		}

		//we must have a route to the combat point
		if (flags & CP_HAS_ROUTE)
		{
			if (waypoint == WAYPOINT_NONE || level.combatPoints[i].waypoint == WAYPOINT_NONE || trap->
				Nav_GetBestNodeAltRoute2(waypoint, level.combatPoints[i].waypoint, NODE_NONE) ==
				WAYPOINT_NONE)
			{
				//can't possibly have a route to any OR can't possibly have a route to this one OR don't have a route to this one
				if (!NAV_ClearPathToPoint(NPCS.NPC, NPCS.NPC->r.mins, NPCS.NPC->r.maxs,
					level.combatPoints[i].origin, NPCS.NPC->clipmask,
					ENTITYNUM_NONE))
				{
					//don't even have a clear straight path to this one
					continue;
				}
			}
		}

		//We want the one with the shortest path from current pos
		if (flags & CP_NEAREST && waypoint != WAYPOINT_NONE && level.combatPoints[i].waypoint !=
			WAYPOINT_NONE)
		{
			const int cost = trap->Nav_GetPathCost(waypoint, level.combatPoints[i].waypoint);
			if (cost < bestCost)
			{
				bestCost = cost;
				best = i;
			}
			continue;
		}

		//we want the combat point closest to the enemy
		//if ( flags & CP_CLOSEST )
		//they are sorted by this distance, so the first one to get this far is the closest
		return i;
	}

	return best;
}

int NPC_FindCombatPointRetry(const vec3_t position,
	const vec3_t avoidPosition,
	vec3_t dest_position,
	int* cpFlags,
	const float avoidDist,
	const int ignorePoint)
{
	//racc - keeps reattempting to find a combat point by removing point flag requirement
	int cp = NPC_FindCombatPoint(position,
		avoidPosition,
		dest_position,
		*cpFlags,
		avoidDist,
		ignorePoint);
	while (cp == -1 && (*cpFlags & ~CP_HAS_ROUTE) != CP_ANY)
	{
		//start "OR"ing out certain flags to see if we can find *any* point
		if (*cpFlags & CP_INVESTIGATE)
		{
			//don't need to investigate
			*cpFlags &= ~CP_INVESTIGATE;
		}
		else if (*cpFlags & CP_SQUAD)
		{
			//don't need to stick to squads
			*cpFlags &= ~CP_SQUAD;
		}
		else if (*cpFlags & CP_DUCK)
		{
			//don't need to duck
			*cpFlags &= ~CP_DUCK;
		}
		else if (*cpFlags & CP_NEAREST)
		{
			//don't need closest one to me
			*cpFlags &= ~CP_NEAREST;
		}
		else if (*cpFlags & CP_FLANK)
		{
			//don't need to flank enemy
			*cpFlags &= ~CP_FLANK;
		}
		else if (*cpFlags & CP_SAFE)
		{
			//don't need one that hasn't been shot at recently
			*cpFlags &= ~CP_SAFE;
		}
		else if (*cpFlags & CP_CLOSEST)
		{
			//don't need to get closest to enemy
			*cpFlags &= ~CP_CLOSEST;
			//but let's try to approach at least
			*cpFlags |= CP_APPROACH_ENEMY;
		}
		else if (*cpFlags & CP_APPROACH_ENEMY)
		{
			//don't need to approach enemy
			*cpFlags &= ~CP_APPROACH_ENEMY;
		}
		else if (*cpFlags & CP_COVER)
		{
			//don't need cover
			*cpFlags &= ~CP_COVER;
			//but let's pick one that makes us duck
			//*cpFlags |= CP_DUCK;
		}
		else if (*cpFlags & CP_CLEAR)
		{
			//don't need a clear shot to enemy
			*cpFlags &= ~CP_CLEAR;
		}
		else if (*cpFlags & CP_AVOID_ENEMY)
		{
			//don't need to avoid enemy
			*cpFlags &= ~CP_AVOID_ENEMY;
		}
		else if (*cpFlags & CP_RETREAT)
		{
			//don't need to retreat
			*cpFlags &= ~CP_RETREAT;
		}
		else if (*cpFlags & CP_FLEE)
		{
			//don't need to flee
			*cpFlags &= ~CP_FLEE;
			//but at least avoid enemy and pick one that gives cover
			*cpFlags |= CP_COVER | CP_AVOID_ENEMY;
		}
		else if (*cpFlags & CP_AVOID)
		{
			//okay, even pick one right by me
			*cpFlags &= ~CP_AVOID;
		}
		else if (*cpFlags & CP_SHORTEST_PATH)
		{
			//okay, don't need the one with the shortest path
			*cpFlags &= ~CP_SHORTEST_PATH;
		}
		else
		{
			//screw it, we give up!
			return -1;
		}
		//now try again
		cp = NPC_FindCombatPoint(position,
			avoidPosition,
			dest_position,
			*cpFlags,
			avoidDist,
			ignorePoint);
	}
	return cp;
}

/*
-------------------------
NPC_FindSquadPoint
-------------------------
*/

int NPC_FindSquadPoint(vec3_t position)
{
	float nearestDist = (float)WORLD_SIZE * (float)WORLD_SIZE;
	int nearestPoint = -1;

	//float			playerDist = DistanceSquared( g_entities[0].currentOrigin, NPC->r.currentOrigin );

	for (int i = 0; i < level.numCombatPoints; i++)
	{
		//Squad points are only valid if we're looking for them
		if ((level.combatPoints[i].flags & CPF_SQUAD) == qfalse)
			continue;

		//Must be vacant
		if (level.combatPoints[i].occupied == qtrue)
			continue;

		const float dist = DistanceSquared(position, level.combatPoints[i].origin);

		//The point cannot take us past the player
		//if ( dist > ( playerDist * DotProduct( dirToPlayer, playerDir ) ) )	//FIXME: Retain this
		//	continue;

		//See if this is closer than the others
		if (dist < nearestDist)
		{
			nearestPoint = i;
			nearestDist = dist;
		}
	}

	return nearestPoint;
}

/*
-------------------------
NPC_ReserveCombatPoint
-------------------------
*/

qboolean NPC_ReserveCombatPoint(const int combatPointID)
{
	//Make sure it's valid
	if (combatPointID > level.numCombatPoints)
		return qfalse;

	//Make sure it's not already occupied
	if (level.combatPoints[combatPointID].occupied)
		return qfalse;

	//Reserve it
	level.combatPoints[combatPointID].occupied = qtrue;

	return qtrue;
}

/*
-------------------------
NPC_FreeCombatPoint
-------------------------
*/

qboolean NPC_FreeCombatPoint(const int combatPointID, const qboolean failed)
{
	if (failed)
	{
		//remember that this one failed for us
		NPCS.NPCInfo->lastFailedCombatPoint = combatPointID;
	}
	//Make sure it's valid
	if (combatPointID > level.numCombatPoints)
		return qfalse;

	//Make sure it's currently occupied
	if (level.combatPoints[combatPointID].occupied == qfalse)
		return qfalse;

	//Free it
	level.combatPoints[combatPointID].occupied = qfalse;

	return qtrue;
}

/*
-------------------------
NPC_SetCombatPoint
-------------------------
*/

qboolean NPC_SetCombatPoint(const int combatPointID)
{
	//Don't set if we're already set to this combatpoint.
	if (combatPointID == NPCS.NPCInfo->combatPoint)
	{
		return qtrue;
	}
	//Free a combat point if we already have one
	if (NPCS.NPCInfo->combatPoint != -1)
	{
		NPC_FreeCombatPoint(NPCS.NPCInfo->combatPoint, qfalse);
	}

	if (NPC_ReserveCombatPoint(combatPointID) == qfalse)
		return qfalse;

	NPCS.NPCInfo->combatPoint = combatPointID;

	return qtrue;
}

extern qboolean G_CanPickUpWeapons(const gentity_t* other);

gentity_t* NPC_SearchForWeapons(void)
{
	gentity_t* bestFound = NULL;
	float bestDist = Q3_INFINITE;

	for (int i = 0; i < level.num_entities; i++)
	{
		if (!g_entities[i].inuse)
			continue;

		gentity_t* found = &g_entities[i];

		//FIXME: Also look for ammo_racks that have weapons on them?
		if (found->s.eType != ET_ITEM)
		{
			continue;
		}
		if (found->item->giType != IT_WEAPON)
		{
			continue;
		}
		if (found->s.eFlags & EF_NODRAW)
		{
			continue;
		}
		if (CheckItemCanBePickedUpByNPC(found, NPCS.NPC))
		{
			if (trap->InPVS(found->r.currentOrigin, NPCS.NPC->r.currentOrigin))
			{
				const float dist = DistanceSquared(found->r.currentOrigin, NPCS.NPC->r.currentOrigin);
				if (dist < bestDist)
				{
					if (!trap->Nav_GetBestPathBetweenEnts(
						(sharedEntity_t*)NPCS.NPC, (sharedEntity_t*)found,
						NF_CLEAR_PATH)
						|| trap->Nav_GetBestNodeAltRoute2(
							NPCS.NPC->waypoint, found->waypoint, NODE_NONE) ==
						WAYPOINT_NONE)
					{
						//can't possibly have a route to any OR can't possibly have a route to this one OR don't have a route to this one
						if (NAV_ClearPathToPoint(NPCS.NPC, NPCS.NPC->r.mins, NPCS.NPC->r.maxs,
							found->r.currentOrigin, NPCS.NPC->clipmask,
							ENTITYNUM_NONE))
						{
							//have a clear straight path to this one
							bestDist = dist;
							bestFound = found;
						}
					}
					else
					{
						//can nav to it
						bestDist = dist;
						bestFound = found;
					}
				}
			}
		}
	}

	return bestFound;
}

static void NPC_SetPickUpGoal(gentity_t* found_weap)
{
	vec3_t org;

	VectorCopy(found_weap->r.currentOrigin, org);
	org[2] += 24 - found_weap->r.mins[2] * -1; //adjust the origin so that I am on the ground
	NPC_SetMoveGoal(NPCS.NPC, org, found_weap->r.maxs[0] * 0.75, qfalse, -1, found_weap);
	NPCS.NPCInfo->tempGoal->waypoint = found_weap->waypoint;
	NPCS.NPCInfo->tempBehavior = BS_DEFAULT;
	NPCS.NPCInfo->squadState = SQUAD_TRANSITION;
}

extern qboolean G_CanPickUpWeapons(const gentity_t* other);

void NPC_CheckGetNewWeapon(void)
{
	if (NPCS.NPC->s.weapon == WP_NONE && NPCS.NPC->enemy)
	{
		//if running away because dropped weapon...
		if (NPCS.NPCInfo->goalEntity
			&& NPCS.NPCInfo->goalEntity == NPCS.NPCInfo->tempGoal
			&& NPCS.NPCInfo->goalEntity->enemy
			&& !NPCS.NPCInfo->goalEntity->enemy->inuse)
		{
			//maybe was running at a weapon that was picked up
			NPC_ClearGoal();
			trap->ICARUS_TaskIDComplete((sharedEntity_t*)NPCS.NPC, TID_MOVE_NAV);
		}
		if (TIMER_Done(NPCS.NPC, "panic") && NPCS.NPCInfo->goalEntity == NULL)
		{
			//need a weapon, any lying around?
			//gentity_t* foundWeap = NPC_SearchForWeapons();
			//if (foundWeap)
			//{//try to nav to it
			//	NPC_SetPickUpGoal(foundWeap);
			//}
		}
	}
}

void NPC_AimAdjust(const int change)
{
	if (!TIMER_Exists(NPCS.NPC, "aimDebounce"))
	{
		const int debounce = 500 + (3 - g_npcspskill.integer) * 100;
		TIMER_Set(NPCS.NPC, "aimDebounce", Q_irand(debounce, debounce + 1000));
		return;
	}
	if (TIMER_Done(NPCS.NPC, "aimDebounce"))
	{
		NPCS.NPCInfo->currentAim += change;
		if (NPCS.NPCInfo->currentAim > NPCS.NPCInfo->stats.aim)
		{
			//can never be better than max aim
			NPCS.NPCInfo->currentAim = NPCS.NPCInfo->stats.aim;
		}
		else if (NPCS.NPCInfo->currentAim < -30)
		{
			//can never be worse than this
			NPCS.NPCInfo->currentAim = -30;
		}

		//Com_Printf( "%s new aim = %d\n", NPC->NPC_type, NPCInfo->currentAim );

		const int debounce = 500 + (3 - g_npcspskill.integer) * 100;
		TIMER_Set(NPCS.NPC, "aimDebounce", Q_irand(debounce, debounce + 1000));
		//int debounce = 1000+(3-g_npcspskill.integer)*500;
		//TIMER_Set( NPC, "aimDebounce", Q_irand( debounce, debounce+2000 ) );
	}
}

void G_AimSet(const gentity_t* self, const int aim)
{
	if (self->NPC)
	{
		self->NPC->currentAim = aim;
		//Com_Printf( "%s new aim = %d\n", self->NPC_type, self->NPC->currentAim );

		const int debounce = 500 + (3 - g_npcspskill.integer) * 100;
		TIMER_Set(self, "aimDebounce", Q_irand(debounce, debounce + 1000));
		//	int debounce = 1000+(3-g_npcspskill.integer)*500;
		//	TIMER_Set( self, "aimDebounce", Q_irand( debounce,debounce+2000 ) );
	}
}