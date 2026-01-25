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

//b_spawn.cpp
//added by MCG
#include "b_local.h"
#include "anims.h"
#include "w_saber.h"
#include "bg_saga.h"
#include "bg_vehicles.h"
#include "g_nav.h"

extern void G_DebugPrint(int level, const char* format, ...);

extern qboolean G_CheckInSolid(gentity_t* self, qboolean fix);
extern qboolean client_userinfo_changed(int clientNum);
extern qboolean spot_would_telefrag2(const gentity_t* mover, vec3_t dest);
extern void Jedi_Cloak(gentity_t* self);

extern void Q3_SetParm(int entID, int parmNum, const char* parmValue);
extern char* TeamNames[TEAM_NUM_TEAMS];
extern void bubble_shield_update(void);

extern void ST_ClearTimers(const gentity_t* ent);
extern void Jedi_ClearTimers(const gentity_t* ent);
extern void npc_shadow_trooper_precache(void);
extern void NPC_Gonk_Precache(void);
extern void NPC_Mouse_Precache(void);
extern void NPC_Seeker_Precache(void);
extern void NPC_Remote_Precache(void);
extern void NPC_R2D2_Precache(void);
extern void NPC_R5D2_Precache(void);
extern void NPC_Probe_Precache(void);
extern void NPC_Interrogator_Precache(gentity_t* self);
extern void NPC_MineMonster_Precache(void);
extern void NPC_Howler_Precache(void);
extern void NPC_ATST_Precache(void);
extern void NPC_Sentry_Precache(void);
extern void NPC_Mark1_Precache(void);
extern void NPC_Mark2_Precache(void);
extern void NPC_GalakMech_Precache(void);
extern void NPC_GalakMech_Init(gentity_t* ent);
extern void NPC_Protocol_Precache(void);
extern void Boba_Precache(void);
extern void NPC_Wampa_Precache(void);
extern void npc_rosh_dark_precache(void);
gentity_t* NPC_SpawnType(const gentity_t* ent, const char* npc_type, const char* targetname, qboolean isVehicle);
extern void Howler_ClearTimers(const gentity_t* self);

extern void Rancor_SetBolts(const gentity_t* self);
extern void Wampa_SetBolts(gentity_t* self);

#define	NSF_DROP_TO_FLOOR	16

// PAIN functions...
//
extern void funcBBrushPain(gentity_t* self, gentity_t* attacker, int damage);
extern void misc_model_breakable_pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void station_pain(gentity_t* self, gentity_t* attacker, int damage);
extern void func_usable_pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_ATST_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_ST_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_Jedi_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_Droid_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_Probe_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_MineMonster_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_Howler_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_Seeker_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_Remote_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void emplaced_gun_pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_Mark1_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_GM_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_Sentry_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_Mark2_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void PlayerPain(gentity_t* self, int damage);
extern void GasBurst(gentity_t* self, gentity_t* attacker, int damage);
extern void CrystalCratePain(gentity_t* self, gentity_t* attacker, int damage);
extern void TurretPain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_Wampa_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_Rancor_Pain(gentity_t* self, gentity_t* attacker, int damage);
extern void NPC_SandCreature_Pain(gentity_t* self, gentity_t* attacker, int damage);

int wp_set_saber_model(gclient_t* client, class_t npcClass)
{
	//if (client)
	//{
	//	switch (npcClass)
	//	{
	//	case CLASS_DESANN://Desann
	//		client->ps.saber[0].model = "models/weapons2/saber_desann/saber_w.glm";
	//		break;
	//	case CLASS_VADER://Desann
	//		client->ps.saber[0].model = "models/weapons2/saber_VaderEp5/saber_w.glm";
	//		break;
	//	case CLASS_LUKE://Luke
	//		client->ps.saber[0].model = "models/weapons2/saber_luke/saber_w.glm";
	//		break;
	//	case CLASS_YODA://YODA
	//		client->ps.saber[0].model = "models/weapons2/saber_yoda/saber_w.glm";
	//		break;
	//	case CLASS_PLAYER://Kyle NPC and player
	//	case CLASS_KYLE://Kyle NPC and player
	//		client->ps.saber[0].model = DEFAULT_SABER_MODEL;
	//		break;
	//	default://reborn and tavion and everyone else
	//		client->ps.saber[0].model = DEFAULT_SABER_MODEL;
	//		break;
	//	}
	//	return (G_model_index(client->ps.saber[0].model));
	//}
	//else
	//{
	//	switch (npcClass)
	//	{
	//	case CLASS_DESANN://Desann
	//		return (G_model_index("models/weapons2/saber_desann/saber_w.glm"));
	//		break;
	//	case CLASS_VADER://Desann
	//		return (G_model_index("models/weapons2/saber_VaderEp5/saber_w.glm"));
	//		break;
	//	case CLASS_LUKE://Luke
	//		return (G_model_index("models/weapons2/saber_luke/saber_w.glm"));
	//		break;
	//	case CLASS_YODA://Luke
	//		return (G_model_index("models/weapons2/saber_yoda/saber_w.glm"));
	//		break;
	//	case CLASS_PLAYER://Kyle NPC and player
	//	case CLASS_KYLE://Kyle NPC and player
	//		return (G_model_index(DEFAULT_SABER_MODEL));
	//		break;
	//	default://reborn and tavion and everyone else
	//		return (G_model_index(DEFAULT_SABER_MODEL));
	//		break;
	//	}
	//}
	return 1;
}

/*
-------------------------
NPC_PainFunc
-------------------------
*/
typedef void (PAIN_FUNC)(gentity_t* self, gentity_t* attacker, int damage);

static PAIN_FUNC* NPC_PainFunc(const gentity_t* ent)
{
	void (*func)(gentity_t * self, gentity_t * attacker, int damage);

	if (ent->client->ps.weapon == WP_SABER)
	{
		func = NPC_Jedi_Pain;
	}
	else
	{
		switch (ent->client->NPC_class)
		{
			// troopers get special pain
		case CLASS_STORMTROOPER:
		case CLASS_STORMCOMMANDO:
		case CLASS_CLONETROOPER:
		case CLASS_SWAMPTROOPER:
			func = NPC_ST_Pain;
			break;

		case CLASS_SEEKER:
			func = NPC_Seeker_Pain;
			break;

		case CLASS_REMOTE:
			func = NPC_Remote_Pain;
			break;

		case CLASS_MINEMONSTER:
			func = NPC_MineMonster_Pain;
			break;

		case CLASS_HOWLER:
			func = NPC_Howler_Pain;
			break;

			// all other droids, did I miss any?
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MOUSE:
		case CLASS_PROTOCOL:
		case CLASS_INTERROGATOR:
		case CLASS_SBD:
		case CLASS_DROIDEKA:
			func = NPC_Droid_Pain;
			break;
		case CLASS_PROBE:
			func = NPC_Probe_Pain;
			break;

		case CLASS_SENTRY:
			func = NPC_Sentry_Pain;
			break;
		case CLASS_MARK1:
			func = NPC_Mark1_Pain;
			break;
		case CLASS_MARK2:
			func = NPC_Mark2_Pain;
			break;
		case CLASS_ATST:
			func = NPC_ATST_Pain;
			break;
		case CLASS_GALAKMECH:
			func = NPC_GM_Pain;
			break;
		case CLASS_RANCOR:
			func = NPC_Rancor_Pain;
			break;
		case CLASS_WAMPA:
			func = NPC_Wampa_Pain;
			break;
		case CLASS_SAND_CREATURE:
			func = NPC_SandCreature_Pain;
			break;
			// everyone else gets the normal pain func
		default:
			func = NPC_Pain;
			break;
		}
	}

	return func;
}

/*
-------------------------
NPC_TouchFunc
-------------------------
*/
typedef void (TOUCH_FUNC)(gentity_t* self, gentity_t* other, trace_t* trace);

static TOUCH_FUNC* NPC_TouchFunc(gentity_t* ent)
{
	void(*func)(gentity_t * self, gentity_t * other, trace_t * trace) = NPC_Touch;

	return func;
}

/*
-------------------------
NPC_SetMiscDefaultData
-------------------------
*/
//this function sets the default fleeing behavior for this NPC
static void G_ClassSetDontFlee(const gentity_t* self)
{
	if (!self || !self->client || !self->NPC)
	{
		return;
	}
	switch (self->client->NPC_class)
	{
	case CLASS_ATST:
	case CLASS_CLAW:
	case CLASS_DESANN:
	case CLASS_FISH:
	case CLASS_FLIER2:
	case CLASS_GALAK:
	case CLASS_GLIDER:
	case CLASS_RANCOR:
	case CLASS_SAND_CREATURE:
	case CLASS_INTERROGATOR: // droid
	case CLASS_JAN:
	case CLASS_JEDI:
	case CLASS_KYLE:
	case CLASS_LANDO:
	case CLASS_LIZARD:
	case CLASS_LUKE:
	case CLASS_MARK1: // droid
	case CLASS_MARK2: // droid
	case CLASS_GALAKMECH: // droid
	case CLASS_MONMOTHA:
	case CLASS_MORGANKATARN:
	case CLASS_MURJJ:
	case CLASS_PROBE: // droid
	case CLASS_REBORN:
	case CLASS_REELO:
	case CLASS_REMOTE:
	case CLASS_SEEKER: // droid
	case CLASS_SENTRY:
	case CLASS_SHADOWTROOPER:
	case CLASS_SWAMP:
	case CLASS_TAVION:
	case CLASS_ALORA:
	case CLASS_BOBAFETT:
	case CLASS_SABER_DROID:
	case CLASS_ASSASSIN_DROID:
	case CLASS_DROIDEKA:
	case CLASS_PLAYER:
	case CLASS_VEHICLE:
	case CLASS_MANDO:
	case CLASS_SBD:
	case CLASS_WOOKIE:
		self->NPC->scriptFlags |= SCF_DONT_FLEE;
		break;
	default:
		break;
	}

	if (self->NPC->aiFlags & NPCAI_BOSS_CHARACTER)
	{
		self->NPC->scriptFlags |= SCF_DONT_FLEE;
	}
	if (self->NPC->aiFlags & NPCAI_SUBBOSS_CHARACTER)
	{
		self->NPC->scriptFlags |= SCF_DONT_FLEE;
	}
	if (self->NPC->aiFlags & NPCAI_ROSH)
	{
		self->NPC->scriptFlags |= SCF_DONT_FLEE;
	}
	if (self->NPC->aiFlags & NPCAI_HEAL_ROSH)
	{
		self->NPC->scriptFlags |= SCF_DONT_FLEE;
	}
}

extern void SandCreature_ClearTimers(gentity_t* ent);
extern void Boba_SetBolts(const gentity_t* ent);
void DetermineDodgeMax(const gentity_t* ent);
extern void Saboteur_Cloak(gentity_t* self);

static void NPC_SetMiscDefaultData(gentity_t* ent)
{
	if (ent->spawnflags & SFB_CINEMATIC)
	{
		//if a cinematic guy, default us to wait b_state
		ent->NPC->behaviorState = BS_CINEMATIC;
	}
	if (ent->client->NPC_class == CLASS_BOBAFETT)
	{
		//set some stuff, precache
		Boba_Precache();
		ent->client->ps.fd.forcePowersKnown |= 1 << FP_LEVITATION;
		ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_3;
		ent->client->ps.fd.forcePower = 100;
		ent->NPC->scriptFlags |= SCF_altFire | SCF_NO_GROUPS;
	}
	if (ent->s.NPC_class == CLASS_VEHICLE && ent->m_pVehicle)
	{
		ent->s.g2radius = 255; //MAX for this value, was (ent->r.maxs[2]-ent->r.mins[2]), which is 272 or something

		if (ent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)
		{
			ent->mass = 2000; //???
			ent->flags |= FL_SHIELDED | FL_NO_KNOCKBACK;
			ent->pain = NPC_ATST_Pain;
		}
		//turn the damn hatch cover on and LEAVE it on
		trap->G2API_SetSurfaceOnOff(ent->ghoul2, "head_hatchcover", 0/*TURN_ON*/);
	}
	if (!Q_stricmp("wampa", ent->NPC_type))
	{
		//FIXME: extern this into NPC.cfg?
		Wampa_SetBolts(ent);
		ent->s.g2radius = 80; //???
		ent->mass = 300; //???
		ent->flags |= FL_NO_KNOCKBACK;
		ent->pain = NPC_Wampa_Pain;
	}
	if (ent->client->NPC_class == CLASS_RANCOR)
	{
		Rancor_SetBolts(ent);
		ent->s.g2radius = 255; //MAX for this value, was (ent->r.maxs[2]-ent->r.mins[2]), which is 272 or something
		ent->mass = 1000; //???
		ent->flags |= FL_NO_KNOCKBACK;
		ent->pain = NPC_Rancor_Pain;
		ent->health *= 4;
	}
	if (ent->client->NPC_class == CLASS_SAND_CREATURE)
	{
		//???
		ent->clipmask = CONTENTS_SOLID | CONTENTS_MONSTERCLIP; //it can go through others
		ent->r.contents = 0; //can't be hit?
		ent->takedamage = qfalse; //can't be killed
		ent->flags |= FL_NO_KNOCKBACK;
		SandCreature_ClearTimers(ent);
		Rancor_SetBolts(ent); //racc - need for the MP system for getting picked up by monsters.
	}

	if (Q_stricmpn(ent->NPC_type, "hazardtrooper", 13) == 0)
	{
		//hazard trooper
		ent->NPC->scriptFlags |= SCF_NO_GROUPS; //don't use combat points or group AI
		ent->flags |= FL_SHIELDED | FL_NO_KNOCKBACK; //low-level shots bounce off, no knockback
	}
	if (!Q_stricmp("Yoda", ent->NPC_type) || !Q_stricmp("Ep1_Yoda", ent->NPC_type) ||
		!Q_stricmp("Ep2_Yoda", ent->NPC_type) || !Q_stricmp("Ep3_Yoda", ent->NPC_type))
	{
		//FIXME: extern this into NPC.cfg?
		ent->NPC->scriptFlags |= SCF_NO_FORCE; //force powers don't work on him
		ent->NPC->aiFlags |= NPCAI_BOSS_CHARACTER; //added boss flag
	}
	if (!Q_stricmp("emperor", ent->NPC_type)
		|| !Q_stricmp("cultist_grip", ent->NPC_type)
		|| !Q_stricmp("cultist_drain", ent->NPC_type)
		|| !Q_stricmp("cultist_destroyer", ent->NPC_type)
		|| !Q_stricmp("cultist_lightning", ent->NPC_type))
	{
		//FIXME: extern this into NPC.cfg?
		ent->NPC->scriptFlags |= SCF_DONT_FIRE; //so he uses only force powers
	}
	if (!Q_stricmp("cultist_destroyer", ent->NPC_type))
	{
		ent->splashDamage = 1000;
		ent->splashRadius = 384;
		//ent->fxID = G_EffectIndex("explosions/droidexplosion1");
		ent->NPC->scriptFlags |= SCF_DONT_FLEE | SCF_IGNORE_ALERTS;
		ent->NPC->ignorePain = qtrue;
	}
	if (ent->client->NPC_class == CLASS_BOBAFETT)
	{
		Boba_SetBolts(ent);
	}
	//==================
	if (ent->client->ps.weapon == WP_SABER)
	{
		//if I'm equipped with a saber, initialize it (them)
		wp_saber_init_blade_data(ent);
		ent->client->ps.saberHolstered = 2;
		Jedi_ClearTimers(ent);
		ent->client->ps.fd.blockPoints = BLOCK_POINTS_MAX;
	}
	if (ent->client->ps.fd.forcePowersKnown != 0)
	{
		WP_InitForcePowers(ent);
		WP_SpawnInitForcePowers(ent); //rww
	}
	//allocate the NPC's skills based on their weapon selection
	if (ent->client->ps.weapon == WP_BLASTER)
	{
		ent->client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
	}
	if (ent->client->ps.weapon == WP_BOWCASTER)
	{
		ent->client->skillLevel[SK_BOWCASTER] = FORCE_LEVEL_3;
	}
	if (ent->client->ps.weapon == WP_ROCKET_LAUNCHER)
	{
		ent->client->skillLevel[SK_ROCKET] = FORCE_LEVEL_3;
	}
	if (ent->client->ps.weapon == WP_THERMAL)
	{
		ent->client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
		ent->client->skillLevel[SK_CRYOBAN] = FORCE_LEVEL_3;
	}
	if (ent->client->ps.weapon == WP_BRYAR_PISTOL || ent->client->ps.weapon == WP_BRYAR_OLD)
	{
		ent->client->skillLevel[SK_PISTOL] = FORCE_LEVEL_3;
	}
	if (ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] > 0)
	{
		//just bump all the NPC's other saber styles to their saber offense skill level
		for (int count = SK_BLUESTYLE; count <= SK_STAFFSTYLE; count++)
		{
			ent->client->skillLevel[count] = ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
		}
	}
	//determine DodgeMax since NPCs kick out of InitForcePowers early.
	DetermineDodgeMax(ent);

	if (ent->client->NPC_class == CLASS_HOWLER)
	{
		Howler_ClearTimers(ent);
		ent->NPC->scriptFlags |= SCF_NO_FALLTODEATH;
		ent->flags |= FL_NO_IMPACT_DMG;
		ent->NPC->scriptFlags |= SCF_NAV_CAN_JUMP; // These jokers can jump
	}
	if (ent->client->NPC_class == CLASS_DESANN
		|| ent->client->NPC_class == CLASS_VADER
		|| ent->client->NPC_class == CLASS_SITHLORD
		|| ent->client->NPC_class == CLASS_ALORA
		|| ent->client->NPC_class == CLASS_TAVION
		|| ent->client->NPC_class == CLASS_LUKE
		|| ent->client->NPC_class == CLASS_KYLE
		|| ent->client->NPC_class == CLASS_REBORN
		|| Q_stricmp("tavion_scepter", ent->NPC_type) == 0
		|| Q_stricmp("alora_dual", ent->NPC_type) == 0)
	{
		//boss character flag set
		ent->NPC->aiFlags |= NPCAI_BOSS_CHARACTER;
	}
	else if (Q_stricmp("alora", ent->NPC_type) == 0
		|| Q_stricmp("rosh_dark", ent->NPC_type) == 0)
	{
		//sub-boss character flag set
		ent->NPC->aiFlags |= NPCAI_SUBBOSS_CHARACTER;
	}
	if (ent->client->NPC_class == CLASS_SABER_DROID)
	{
		ent->flags |= FL_NO_KNOCKBACK;
	}
	else if (ent->client->NPC_class == CLASS_SABOTEUR)
	{
		//can cloak
		ent->NPC->aiFlags |= NPCAI_SHIELDS; //give them the ability to cloak
		if (ent->spawnflags & 16)
		{
			//start cloaked
			Saboteur_Cloak(ent);
		}
	}
	else if (ent->client->NPC_class == CLASS_ASSASSIN_DROID)
	{
		ent->client->ps.stats[STAT_ARMOR] = 250; // start with full armor
		if (ent->s.weapon == WP_BLASTER)
		{
			ent->NPC->scriptFlags |= SCF_altFire;
		}
		ent->flags |= FL_NO_KNOCKBACK;
	}
	else if (ent->client->NPC_class == CLASS_SBD)
	{
		ent->client->ps.stats[STAT_ARMOR] = 250; // start with full armor
		ent->flags |= FL_NO_KNOCKBACK;
		ent->NPC->scriptFlags = SCF_CHASE_ENEMIES | SCF_LOOK_FOR_ENEMIES;
		ent->NPC->scriptFlags |= SCF_DONT_FLEE;
	}
	else if (ent->client->NPC_class == CLASS_DROIDEKA)
	{
		ent->client->ps.stats[STAT_ARMOR] = 250; // start with full armor
		ent->flags |= FL_NO_KNOCKBACK;
		ent->NPC->scriptFlags = SCF_CHASE_ENEMIES | SCF_LOOK_FOR_ENEMIES;
		ent->NPC->scriptFlags |= SCF_DONT_FLEE;
	}
	//rosh boss special flags
	if (Q_stricmp("DKothos", ent->NPC_type) == 0
		|| Q_stricmp("VKothos", ent->NPC_type) == 0)
	{
		//flags for the force healer dudes.
		ent->NPC->scriptFlags |= SCF_DONT_FIRE;
		ent->NPC->aiFlags |= NPCAI_HEAL_ROSH;
		ent->count = 100;
	}
	else if (Q_stricmp("rosh_dark", ent->NPC_type) == 0)
	{
		//boss flag for Rosh
		ent->NPC->aiFlags |= NPCAI_ROSH;
	}
	if (ent->client->NPC_class == CLASS_SEEKER)
	{
		ent->NPC->defaultBehavior = BS_DEFAULT;
		ent->client->ps.gravity = 0;
		ent->NPC->aiFlags |= NPCAI_CUSTOM_GRAVITY;
		ent->client->ps.eFlags2 |= EF2_FLYING;
		if (ent->client->leader //has leader
			&& ent->client->leader->client //leader is a client
			&& ent->client->leader->client->remote == ent) //has us as their remote.
		{
			//player's seeker item
			ent->count = -1;
		}
		else
		{
			//normal NPC seeker
			ent->count = 30; // SEEKER shot ammo count
		}
	}
	if (ent->client->NPC_class == CLASS_GONK)
	{
		// I guess we generically make them player usable
		ent->r.svFlags |= SVF_PLAYER_USABLE;
	}

	//***I'm not sure whether I should leave this as a TEAM_ switch, I think NPC_class may be more appropriate - dmv
	switch ((team_t)ent->client->playerTeam)
	{
	case (team_t)NPCTEAM_PLAYER:
	{
		if (ent->client->NPC_class == CLASS_SEEKER)
		{
			ent->NPC->defaultBehavior = BS_DEFAULT;
			ent->client->ps.gravity = 0;
			ent->client->ps.eFlags2 |= EF2_FLYING;

			ent->client->moveType = MT_FLYSWIM;
			ent->count = 30; // SEEKER shot ammo count
			return;
		}
		if (ent->client->NPC_class == CLASS_JEDI
			|| ent->client->NPC_class == CLASS_KYLE
			|| ent->client->NPC_class == CLASS_LUKE)
		{
			//good jedi
			ent->client->enemyTeam = NPCTEAM_ENEMY;
			if (ent->spawnflags & JSF_AMBUSH)
			{
				//ambusher
				ent->NPC->scriptFlags |= SCF_IGNORE_ALERTS;
				ent->client->noclip = qtrue; //hang
			}
		}
		else
		{
			switch (ent->client->ps.weapon)
			{
			case WP_BRYAR_PISTOL: //FIXME: new weapon: imp blaster pistol
			case WP_DISRUPTOR:
			case WP_BOWCASTER:
			case WP_REPEATER:
			case WP_DEMP2:
			case WP_FLECHETTE:
			case WP_ROCKET_LAUNCHER:
			default:
				break;
			case WP_THERMAL:
			case WP_BLASTER:
				ST_ClearTimers(ent);
				if (ent->NPC->rank >= RANK_LT || ent->client->ps.weapon == WP_THERMAL)
				{
					//officers, grenade-throwers use alt-fire
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				break;
			}
			if (!Q_stricmp("galak_mech", ent->NPC_type))
			{
				//starts with armor
				NPC_GalakMech_Init(ent);
				bubble_shield_update();
			}
		}
	}
	if (ent->client->NPC_class == CLASS_KYLE || ent->client->NPC_class == CLASS_VEHICLE || ent->spawnflags &
		SFB_CINEMATIC)
	{
		ent->NPC->defaultBehavior = BS_CINEMATIC;
	}
	else
	{
		ent->NPC->defaultBehavior = BS_FOLLOW_LEADER;
		ent->client->leader = &g_entities[0]; //player
	}
	break;

	case (team_t)NPCTEAM_NEUTRAL:

		if (Q_stricmp(ent->NPC_type, "gonk") == 0)
		{
			// I guess we generically make them player usable
			ent->r.svFlags |= SVF_PLAYER_USABLE;

			// Not even sure if we want to give different levels of batteries?  ...Or even that these are the values we'd want to use.
			switch (g_npcspskill.integer)
			{
			case 0: //	EASY
				ent->client->ps.batteryCharge = MAX_BATTERIES * 0.8f;
				break;
			case 1: //	MEDIUM
				ent->client->ps.batteryCharge = MAX_BATTERIES * 0.75f;
				break;
			default:
			case 2: //	HARD
				ent->client->ps.batteryCharge = MAX_BATTERIES * 0.5f;
				break;
			}
		}
		break;

	case (team_t)NPCTEAM_ENEMY:
	{
		ent->NPC->defaultBehavior = BS_DEFAULT;

		if (ent->client->NPC_class == CLASS_SHADOWTROOPER && Q_stricmpn("shadowtrooper", ent->NPC_type, 13) == 0)
		{
			if (!PM_SaberInAttack(ent->client->ps.saber_move))
			{
				Jedi_Cloak(ent);
			}
		}
		if (ent->client->NPC_class == CLASS_TAVION ||
			ent->client->NPC_class == CLASS_ALORA ||
			ent->client->NPC_class == CLASS_SBD ||
			ent->client->NPC_class == CLASS_REBORN && ent->client->ps.weapon == WP_SABER ||
			ent->client->NPC_class == CLASS_DESANN ||
			ent->client->NPC_class == CLASS_SITHLORD ||
			ent->client->NPC_class == CLASS_VADER ||
			ent->client->NPC_class == CLASS_SHADOWTROOPER)
		{
			ent->client->enemyTeam = NPCTEAM_PLAYER;

			if (ent->spawnflags & JSF_AMBUSH)
			{
				//ambusher
				ent->NPC->scriptFlags |= SCF_IGNORE_ALERTS;
				ent->client->noclip = qtrue; //hang
			}
		}
		else if (ent->client->NPC_class == CLASS_PROBE ||
			ent->client->NPC_class == CLASS_REMOTE ||
			ent->client->NPC_class == CLASS_GLIDER ||
			ent->client->NPC_class == CLASS_INTERROGATOR ||
			ent->client->NPC_class == CLASS_SENTRY)
		{
			ent->NPC->defaultBehavior = BS_DEFAULT;
			ent->client->ps.gravity = 0;
			ent->NPC->aiFlags |= NPCAI_CUSTOM_GRAVITY;
			ent->client->ps.eFlags2 |= EF2_FLYING;
		}
		else
		{
			switch (ent->client->ps.weapon)
			{
			case WP_BRYAR_PISTOL:
				NPCS.NPCInfo->scriptFlags |= SCF_PILOT;
				if (!Q_stricmp("Imperial", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				if (!Q_stricmp("StormPilot", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				break;
			case WP_DISRUPTOR:
				NPCS.NPCInfo->scriptFlags |= SCF_PILOT;
				if (!Q_stricmp("saboteursniper", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				break;
			case WP_BOWCASTER:
				NPCS.NPCInfo->scriptFlags |= SCF_PILOT;
				if (!Q_stricmp("human_merc_bow", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				break;
			case WP_REPEATER:
				NPCS.NPCInfo->scriptFlags |= SCF_PILOT;
				if (!Q_stricmp("human_merc_rep", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				if (!Q_stricmp("StormTrooper_red", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				break;
			case WP_DEMP2:
				NPCS.NPCInfo->scriptFlags |= SCF_PILOT;
				break;
			case WP_FLECHETTE:
				NPCS.NPCInfo->scriptFlags |= SCF_PILOT;
				//shotgunner
				if (!Q_stricmp("stofficeralt", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				if (!Q_stricmp("STOfficer", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				if (!Q_stricmp("STOfficer2", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				if (!Q_stricmp("human_merc_flc", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				if (!Q_stricmp("StormTrooper_blue", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				break;
			case WP_ROCKET_LAUNCHER:
				NPCS.NPCInfo->scriptFlags |= SCF_PILOT;
				break;
			case WP_CONCUSSION:
				NPCS.NPCInfo->scriptFlags |= SCF_PILOT;
				if (!Q_stricmp("human_merc_cnc", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				break;
			case WP_THERMAL:
				NPCS.NPCInfo->scriptFlags |= SCF_PILOT;
				break;
			case WP_STUN_BATON:
				break;
			default:
			case WP_BLASTER:
				NPCS.NPCInfo->scriptFlags |= SCF_PILOT;
				ST_ClearTimers(ent);
				if (ent->NPC->rank >= RANK_COMMANDER)
				{
					//commanders use alt-fire
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				if (ent->client->NPC_class == CLASS_IMPERIAL)
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				if (ent->client->NPC_class == CLASS_RODIAN)
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				if (!Q_stricmp("human_merc", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				if (!Q_stricmp("rodian2", ent->NPC_type))
				{
					ent->NPC->scriptFlags |= SCF_altFire;
				}
				break;
			}
			if (!Q_stricmp("galak_mech", ent->NPC_type))
			{
				//starts with armor
				NPC_GalakMech_Init(ent);
				bubble_shield_update();
			}
		}
	}
	break;

	default:
		ent->NPC->defaultBehavior = BS_DEFAULT;
		break;
	}

	if (ent->client->NPC_class == CLASS_ATST || ent->client->NPC_class == CLASS_MARK1)
		// chris/steve/kevin requested that the mark1 be shielded also
	{
		ent->flags |= FL_SHIELDED | FL_NO_KNOCKBACK;
	}

	// Set CAN FLY Flag for Navigation On The Following Classes
	//----------------------------------------------------------
	if (ent->client->NPC_class == CLASS_PROBE ||
		ent->client->NPC_class == CLASS_REMOTE ||
		ent->client->NPC_class == CLASS_SEEKER ||
		ent->client->NPC_class == CLASS_SENTRY ||
		ent->client->NPC_class == CLASS_GLIDER ||
		ent->client->NPC_class == CLASS_IMPWORKER ||
		ent->client->NPC_class == CLASS_BOBAFETT ||
		ent->client->NPC_class == CLASS_MANDO ||
		ent->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		ent->NPC->scriptFlags |= SCF_NAV_CAN_FLY;
	}

	if (ent->client->NPC_class == CLASS_SEEKER && ent->activator)
	{
		//assume my teams are already set correctly
	}
	else
	{
		//for siege, want "bad" npc's allied with the "bad" team
		if (level.gametype == GT_SIEGE && ent->s.NPC_class != CLASS_VEHICLE)
		{
			if (ent->client->enemyTeam == NPCTEAM_PLAYER)
			{
				ent->client->sess.sessionTeam = SIEGETEAM_TEAM1;
			}
			else if (ent->client->enemyTeam == NPCTEAM_ENEMY)
			{
				ent->client->sess.sessionTeam = SIEGETEAM_TEAM2;
			}
			else
			{
				ent->client->sess.sessionTeam = TEAM_FREE;
			}
		}
	}
	G_ClassSetDontFlee(ent);
}

/*
-------------------------
NPC_WeaponsForTeam
-------------------------
*/

int NPC_WeaponsForTeam(const team_t team, const int spawnflags, const char* NPC_type)
{
	//*** not sure how to handle this, should I pass in class instead of team and go from there? - dmv
	switch (team)
	{
	case (team_t)NPCTEAM_ENEMY:
		if (Q_stricmp("tavion", NPC_type) == 0 ||
			Q_strncmp("reborn", NPC_type, 6) == 0 ||
			Q_stricmp("desann", NPC_type) == 0 ||
			Q_strncmp("shadowtrooper", NPC_type, 13) == 0)
			return 1 << WP_SABER;

		if (Q_strncmp("stofficer", NPC_type, 9) == 0)
		{
			return 1 << WP_FLECHETTE;
		}
		if (Q_strncmp("stofficer2", NPC_type, 9) == 0)
		{
			return 1 << WP_FLECHETTE;
		}
		if (Q_stricmp("stcommander", NPC_type) == 0)
		{
			return 1 << WP_REPEATER;
		}
		if (Q_stricmp("swamptrooper", NPC_type) == 0)
		{
			return 1 << WP_FLECHETTE;
		}
		if (Q_stricmp("swamptrooper2", NPC_type) == 0)
		{
			return 1 << WP_REPEATER;
		}
		if (Q_stricmp("rockettrooper", NPC_type) == 0)
		{
			return 1 << WP_ROCKET_LAUNCHER;
		}
		if (Q_strncmp("shadowtrooper", NPC_type, 13) == 0)
		{
			return 1 << WP_SABER;
		}
		if (Q_stricmp("imperial", NPC_type) == 0)
		{
			return 1 << WP_BRYAR_PISTOL;
		}
		if (Q_strncmp("impworker", NPC_type, 9) == 0)
		{
			return 1 << WP_BRYAR_PISTOL;
		}
		if (Q_stricmp("stormpilot", NPC_type) == 0)
		{
			return 1 << WP_BRYAR_PISTOL;
		}
		if (Q_stricmp("galak", NPC_type) == 0)
		{
			return 1 << WP_BLASTER;
		}
		if (Q_stricmp("galak_mech", NPC_type) == 0)
		{
			return 1 << WP_REPEATER;
		}
		if (Q_strncmp("ugnaught", NPC_type, 8) == 0)
		{
			return WP_NONE;
		}
		if (Q_stricmp("granshooter", NPC_type) == 0)
		{
			return 1 << WP_BLASTER;
		}
		if (Q_stricmp("granboxer", NPC_type) == 0)
		{
			return 1 << WP_MELEE;
		}
		if (Q_strncmp("gran", NPC_type, 4) == 0)
		{
			return 1 << WP_THERMAL | 1 << WP_MELEE;
		}
		if (Q_stricmp("rodian", NPC_type) == 0)
		{
			return 1 << WP_DISRUPTOR;
		}
		if (Q_stricmp("rodian2", NPC_type) == 0)
		{
			return 1 << WP_BLASTER;
		}

		if (Q_stricmp("interrogator", NPC_type) == 0 || Q_stricmp("sentry", NPC_type) == 0 || Q_strncmp(
			"protocol", NPC_type, 8) == 0)
		{
			return WP_NONE;
		}

		if (Q_strncmp("weequay", NPC_type, 7) == 0)
		{
			return 1 << WP_BOWCASTER; //|( 1 << WP_STAFF )(FIXME: new weap?)
		}
		if (Q_stricmp("impofficer", NPC_type) == 0)
		{
			return 1 << WP_BLASTER;
		}
		if (Q_stricmp("impcommander", NPC_type) == 0)
		{
			return 1 << WP_BLASTER;
		}
		if (Q_stricmp("probe", NPC_type) == 0 || Q_stricmp("seeker", NPC_type) == 0)
		{
			//return ( 1 << WP_BOT_LASER);
			return 0;
		}
		if (Q_stricmp("remote", NPC_type) == 0)
		{
			//return ( 1 << WP_BOT_LASER );
			return 0;
		}
		if (Q_stricmp("trandoshan", NPC_type) == 0)
		{
			return 1 << WP_REPEATER;
		}
		if (Q_stricmp("atst", NPC_type) == 0)
		{
			//return (( 1 << WP_ATST_MAIN)|( 1 << WP_ATST_SIDE));
			return 0;
		}
		if (Q_stricmp("mark1", NPC_type) == 0)
		{
			//return ( 1 << WP_BOT_LASER);
			return 0;
		}
		if (Q_stricmp("mark2", NPC_type) == 0)
		{
			//return ( 1 << WP_BOT_LASER);
			return 0;
		}
		if (Q_stricmp("minemonster", NPC_type) == 0)
		{
			return 1 << WP_MELEE;
		}
		if (Q_stricmp("howler", NPC_type) == 0)
		{
			return 1 << WP_MELEE;
		}
		//Stormtroopers, etc.
		return 1 << WP_BLASTER;

	case  (team_t)NPCTEAM_PLAYER:

		if (spawnflags & SFB_RIFLEMAN)
			return 1 << WP_REPEATER;

		if (spawnflags & SFB_PHASER)
			return 1 << WP_BRYAR_PISTOL;

		if (Q_strncmp("jedi", NPC_type, 4) == 0 || Q_stricmp("luke", NPC_type) == 0)
			return 1 << WP_SABER;

		if (Q_strncmp("prisoner", NPC_type, 8) == 0
			|| Q_strncmp("elder", NPC_type, 5) == 0)
		{
			return WP_NONE;
		}
		if (Q_strncmp("bespincop", NPC_type, 9) == 0)
		{
			return 1 << WP_BRYAR_PISTOL;
		}

		if (Q_stricmp("MonMothma", NPC_type) == 0)
		{
			return WP_NONE;
		}

		//rebel
		return 1 << WP_BLASTER;

	case  (team_t)NPCTEAM_NEUTRAL:

		if (Q_stricmp("mark1", NPC_type) == 0)
		{
			return WP_NONE;
		}
		if (Q_stricmp("mark2", NPC_type) == 0)
		{
			return WP_NONE;
		}
		if (Q_strncmp("ugnaught", NPC_type, 8) == 0)
		{
			return WP_NONE;
		}
		if (Q_stricmp("bartender", NPC_type) == 0)
		{
			return WP_NONE;
		}
		if (Q_stricmp("morgankatarn", NPC_type) == 0)
		{
			return WP_NONE;
		}

		break;

	default:
		break;
	}

	return WP_NONE;
}

extern void ChangeWeapon(const gentity_t* ent, int new_weapon);

/*
-------------------------
NPC_SetWeapons
-------------------------
*/

static void NPC_SetWeapons(gentity_t* ent)
{
	int bestWeap = WP_NONE;
	const int weapons = NPC_WeaponsForTeam(ent->client->playerTeam, ent->spawnflags, ent->NPC_type);

	ent->client->ps.stats[STAT_WEAPONS] = 0;
	for (int curWeap = WP_SABER; curWeap < WP_NUM_WEAPONS; curWeap++)
	{
		if (weapons & 1 << curWeap)
		{
			ent->client->ps.stats[STAT_WEAPONS] |= 1 << curWeap;
			register_item(BG_FindItemForWeapon(curWeap)); //precache the weapon
			//rwwFIXMEFIXME: Precache
			ent->NPC->currentAmmo = ent->client->ps.ammo[weaponData[curWeap].ammoIndex] = 100; //FIXME: max ammo

			if (bestWeap == WP_SABER)
			{
				// still want to register other weapons -- force saber to be best weap
				continue;
			}

			if (curWeap == WP_MELEE)
			{
				if (bestWeap == WP_NONE)
				{
					// We'll only consider giving Melee since we haven't found anything better yet.
					bestWeap = curWeap;
				}
			}
			else if (curWeap > bestWeap || bestWeap == WP_MELEE)
			{
				// This will never override saber as best weap.  Also will override WP_STUN_BATON if something better comes later in the list
				bestWeap = curWeap;
			}
		}
	}

	ent->client->ps.weapon = bestWeap;
	G_AddEvent(ent, EV_WEAPINVCHANGE, ent->client->ps.stats[STAT_WEAPONS]);
}

/*
-------------------------
NPC_SpawnEffect

  NOTE:  Make sure any effects called here have their models, tga's and sounds precached in
			CG_RegisterNPCEffects in cg_player.cpp
-------------------------
*/

static void NPC_SpawnEffect(gentity_t* ent)
{
}

//--------------------------------------------------------------
// NPC_SetFX_SpawnStates
//
// Set up any special parms for spawn effects
//--------------------------------------------------------------
static void NPC_SetFX_SpawnStates(const gentity_t* ent)
{
	if (!(ent->NPC->aiFlags & NPCAI_CUSTOM_GRAVITY))
	{
		ent->client->ps.gravity = g_gravity.value;
	}
}

/*
================
NPC_SpotWouldTelefrag

================
*/
static qboolean NPC_SpotWouldTelefrag(const gentity_t* npc)
{
	int touch[MAX_GENTITIES];
	vec3_t mins, maxs;

	VectorAdd(npc->r.currentOrigin, npc->r.mins, mins);
	VectorAdd(npc->r.currentOrigin, npc->r.maxs, maxs);
	const int num = trap->EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	for (int i = 0; i < num; i++)
	{
		const gentity_t* hit = &g_entities[touch[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if (hit->inuse
			&& hit->client
			&& hit->s.number != npc->s.number
			&& hit->r.contents & MASK_NPCSOLID
			&& hit->s.number != npc->r.ownerNum
			&& hit->r.ownerNum != npc->s.number)
		{
			return qtrue;
		}
	}

	return qfalse;
}

extern qboolean G_ValidSaberStyle(const gentity_t* ent, int saber_style);
extern qboolean WP_SaberCanTurnOffSomeBlades(const saberInfo_t* saber);
//--------------------------------------------------------------
void NPC_Begin(gentity_t* ent)
{
	vec3_t spawn_origin, spawn_angles;
	usercmd_t ucmd;
	gentity_t* spawnPoint = NULL;

	memset(&ucmd, 0, sizeof ucmd);

	if (!(ent->spawnflags & SFB_NOTSOLID))
	{
		//No NPCs should telefrag
		if (NPC_SpotWouldTelefrag(ent))
		{
			if (ent->wait < 0)
			{
				//remove yourself
				G_DebugPrint(WL_DEBUG, "NPC %s could not spawn, firing target3 (%s) and removing self\n",
					ent->targetname, ent->target3);
				//Fire off our target3
				G_UseTargets2(ent, ent, ent->target3);

				//Kill us
				ent->think = G_FreeEntity;
				ent->nextthink = level.time + 100;
			}
			else
			{
				G_DebugPrint(WL_DEBUG, "NPC %s could not spawn, waiting %4.2 secs to try again\n", ent->targetname,
					ent->wait / 1000.0f);
				ent->think = NPC_Begin;
				ent->nextthink = level.time + ent->wait; //try again in half a second
			}
			return;
		}
	}
	//Spawn effect
	NPC_SpawnEffect(ent);

	VectorCopy(ent->client->ps.origin, spawn_origin);
	VectorCopy(ent->s.angles, spawn_angles);
	spawn_angles[YAW] = ent->NPC->desiredYaw;

	gclient_t* client = ent->client;

	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;

	client->airOutTime = level.time + 12000;

	client->ps.clientNum = ent->s.number;
	// clear entity values

	if (ent->health) // Was health supplied in map
	{
		client->pers.maxHealth = client->ps.stats[STAT_MAX_HEALTH] = ent->health;
	}
	else if (ent->NPC->stats.health) // Was health supplied in NPC.cfg?
	{
		if (ent->client->NPC_class != CLASS_REBORN
			&& ent->client->NPC_class != CLASS_SHADOWTROOPER
			&& ent->client->NPC_class != CLASS_JEDI)
		{
			// up everyone except jedi
			if (!Q_stricmp("tavion_sith_sword", ent->NPC_type)
				|| !Q_stricmp("tavion_scepter", ent->NPC_type)
				|| !Q_stricmp("kyle_boss", ent->NPC_type)
				|| !Q_stricmp("alora_dual", ent->NPC_type)
				|| !Q_stricmp("alora_boss", ent->NPC_type))
			{
				//bosses are a bit different
				ent->NPC->stats.health = ceil(
					(float)ent->NPC->stats.health * 0.75f + (float)ent->NPC->stats.health / 4.0f * g_npcspskill.
					integer); // 75% on easy, 100% on medium, 125% on hard
			}
			else
			{
				ent->NPC->stats.health += ent->NPC->stats.health / 4 * g_npcspskill.integer;
				// 100% on easy, 125% on medium, 150% on hard
			}
		}

		client->pers.maxHealth = client->ps.stats[STAT_MAX_HEALTH] = ent->NPC->stats.health;
	}
	else
	{
		client->pers.maxHealth = client->ps.stats[STAT_MAX_HEALTH] = 100;
	}

	if (!Q_stricmp("rodian", ent->NPC_type))
	{
		//sniper
		//NOTE: this will get overridden by any aim settings in their spawnscripts
		switch (g_npcspskill.integer)
		{
		case 0:
			ent->NPC->stats.aim = 1;
			break;
		case 1:
			ent->NPC->stats.aim = Q_irand(2, 3);
			break;
		case 2:
			ent->NPC->stats.aim = Q_irand(3, 4);
			break;
		default:;
		}
	}
	else if (ent->client->NPC_class == CLASS_STORMTROOPER
		|| ent->client->NPC_class == CLASS_CLONETROOPER
		|| ent->client->NPC_class == CLASS_STORMCOMMANDO
		|| ent->client->NPC_class == CLASS_SBD
		|| ent->client->NPC_class == CLASS_DROIDEKA
		|| ent->client->NPC_class == CLASS_SWAMPTROOPER
		|| ent->client->NPC_class == CLASS_IMPWORKER
		|| !Q_stricmp("rodian2", ent->NPC_type))
	{
		//tweak yawspeed for these NPCs based on difficulty
		switch (g_npcspskill.integer)
		{
		case 0:
			ent->NPC->stats.yawSpeed *= 0.75f;
			if (ent->client->NPC_class == CLASS_IMPWORKER)
			{
				ent->NPC->stats.aim -= Q_irand(3, 6);
			}
			break;
		case 1:
			if (ent->client->NPC_class == CLASS_IMPWORKER)
			{
				ent->NPC->stats.aim -= Q_irand(2, 4);
			}
			break;
		case 2:
			ent->NPC->stats.yawSpeed *= 1.5f;
			if (ent->client->NPC_class == CLASS_IMPWORKER)
			{
				ent->NPC->stats.aim -= Q_irand(0, 2);
			}
			break;
		default:;
		}
	}
	else if (ent->client->NPC_class == CLASS_REBORN
		|| ent->client->NPC_class == CLASS_SHADOWTROOPER)
	{
		switch (g_npcspskill.integer)
		{
		case 1:
			ent->NPC->stats.yawSpeed *= 1.25f;
			break;
		case 2:
			ent->NPC->stats.yawSpeed *= 1.5f;
			break;
		default:;
		}
	}

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->mass = 10;
	ent->takedamage = qtrue;

	ent->inuse = qtrue;

	if (!ent->classname || Q_stricmp(ent->classname, "noclass") == 0)
		ent->classname = "NPC";

	if (!(ent->spawnflags & SFB_NOTSOLID))
	{
		ent->r.contents = CONTENTS_BODY;
		ent->clipmask = MASK_NPCSOLID;
	}
	else
	{
		ent->r.contents = 0;
		ent->clipmask = MASK_NPCSOLID & ~CONTENTS_BODY;
	}

	if (!ent->client->moveType) //Static?
	{
		ent->client->moveType = MT_RUNJUMP;
	}
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->client->ps.rocketLockIndex = ENTITYNUM_NONE;
	ent->client->ps.rocketLockTime = 0;

	//visible to player and NPCs
	if (ent->client->NPC_class != CLASS_R2D2 &&
		ent->client->NPC_class != CLASS_R5D2 &&
		ent->client->NPC_class != CLASS_MOUSE &&
		ent->client->NPC_class != CLASS_GONK &&
		ent->client->NPC_class != CLASS_PROTOCOL)
	{
		ent->flags &= ~FL_NOTARGET;
	}
	ent->s.eFlags &= ~EF_NODRAW;

	NPC_SetFX_SpawnStates(ent);

	if (ent->client->ps.weapon == WP_NONE)
	{
		//not set by the NPCs.cfg
		NPC_SetWeapons(ent);
	}
	//select the weapon
	ent->NPC->currentAmmo = ent->client->ps.ammo[weaponData[ent->client->ps.weapon].ammoIndex];
	ent->client->ps.weaponstate = WEAPON_IDLE;
	ChangeWeapon(ent, ent->client->ps.weapon);

	VectorCopy(spawn_origin, client->ps.origin);

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	// clear entity state values
	ent->s.eType = ET_NPC;

	VectorCopy(spawn_origin, ent->s.origin);

	SetClientViewAngle(ent, spawn_angles);
	client->renderInfo.lookTarget = ENTITYNUM_NONE;
	//clear IK grabbing stuff
	client->ps.heldClient = client->ps.heldByClient = ENTITYNUM_NONE;

	if (!(ent->spawnflags & 64))
	{
		if (Q_stricmp(ent->NPC_type, "nullDriver") == 0)
		{
			//FIXME: should be a better way to check this
		}
		else
		{
			g_kill_box(ent);
		}
		trap->LinkEntity((sharedEntity_t*)ent);
	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.value * 1000;
	client->latched_buttons = 0;

	if (ent->s.m_iVehicleNum)
	{
		//I'm an NPC in a vehicle (or a vehicle), I already have owner set
	}
	else if (client->NPC_class == CLASS_SEEKER && ent->activator != NULL)
	{
		//somebody else "owns" me
		ent->s.owner = ent->r.ownerNum = ent->activator->s.number;
	}
	else
	{
		ent->s.owner = ENTITYNUM_NONE;
	}

	// set default animations
	if (ent->client->NPC_class != CLASS_VEHICLE)
	{
		NPC_SetAnim(ent, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_NORMAL);
	}

	if (spawnPoint)
	{
		// fire the targets of the spawn point
		G_UseTargets(spawnPoint, ent);
	}

	//ICARUS include
	trap->ICARUS_InitEnt((sharedEntity_t*)ent);

	//==NPC initialization
	SetNPCGlobals(ent);

	ent->enemy = NULL;
	NPCS.NPCInfo->timeOfDeath = 0;
	NPCS.NPCInfo->shotTime = 0;
	NPC_ClearGoal();
	NPC_ChangeWeapon(ent->client->ps.weapon);

	//==Final NPC initialization
	ent->pain = NPC_PainFunc(ent); //painF_NPC_Pain;
	ent->touch = NPC_TouchFunc(ent); //touchF_NPC_Touch;

	ent->client->ps.ping = ent->NPC->stats.reactions * 50;

	//MCG - Begin: NPC hacks
	//FIXME: Set the team correctly
	if (ent->s.NPC_class != CLASS_VEHICLE || level.gametype != GT_SIEGE)
	{
		ent->client->ps.persistant[PERS_TEAM] = ent->client->playerTeam;
	}

	ent->use = NPC_Use;
	ent->think = NPC_Think;
	ent->nextthink = level.time + FRAMETIME + Q_irand(0, 100);

	NPC_SetMiscDefaultData(ent);
	if (ent->health <= 0)
	{
		//ORIGINAL ID: health will count down towards max_health
		ent->health = client->ps.stats[STAT_HEALTH] = ent->client->pers.maxHealth;
	}
	else
	{
		client->ps.stats[STAT_HEALTH] = ent->health;
	}

	if (ent->s.shouldtarget)
	{
		ent->maxHealth = ent->health;
		G_ScaleNetHealth(ent);
	}

	ChangeWeapon(ent, ent->client->ps.weapon); //yes, again... sigh

	//set saber_anim_levelBase
	if (ent->client->saber[0].model[0] && ent->client->saber[1].model[0])
	{
		ent->client->ps.fd.saber_anim_levelBase = ent->client->ps.fd.saberAnimLevel = SS_DUAL;
	}
	else if (ent->client->saber[0].numBlades > 1
		&& WP_SaberCanTurnOffSomeBlades(&ent->client->saber[0]))
	{
		ent->client->ps.fd.saber_anim_levelBase = ent->client->ps.fd.saberAnimLevel = SS_STAFF;
	}
	else
	{
		int newLevel = Q_irand(SS_FAST, SS_STAFF);
		ent->client->ps.fd.saber_anim_levelBase = SS_MEDIUM;

		//new validation technique.
		if (!G_ValidSaberStyle(ent, newLevel))
		{
			//had an illegal style, revert to a valid one
			for (int count = SS_FAST; count < SS_STAFF; count++)
			{
				newLevel++;
				if (newLevel > SS_STAFF)
				{
					if (ent->client->saber[0].type == SABER_BACKHAND
						|| ent->client->saber[0].type == SABER_ASBACKHAND)
					{
						newLevel = SS_STAFF;
					}
					else if (ent->client->saber[0].type == SABER_STAFF
						|| ent->client->saber[0].type == SABER_STAFF_UNSTABLE
						|| ent->client->saber[0].type == SABER_STAFF_THIN)
					{
						newLevel = SS_TAVION;
					}
					else
					{
						newLevel = SS_FAST;
					}
				}

				if (G_ValidSaberStyle(ent, newLevel))
				{
					break;
				}
			}
		}

		ent->client->ps.fd.saberAnimLevel = newLevel;
	}

	if (!(ent->spawnflags & SFB_STARTINSOLID))
	{
		//Not okay to start in solid
		G_CheckInSolid(ent, qtrue);
	}
	VectorClear(ent->NPC->lastClearOrigin);

	//Run a script if you have one assigned to you
	if (G_ActivateBehavior(ent, BSET_SPAWN))
	{
		trap->ICARUS_MaintainTaskManager(ent->s.number);
	}

	VectorCopy(ent->r.currentOrigin, ent->client->renderInfo.eyePoint);

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	memset(&ucmd, 0, sizeof ucmd);
	//_VectorCopy( client->pers.cmd_angles, ucmd.angles );
	VectorCopyM(client->pers.cmd.angles, ucmd.angles);

	ent->client->ps.groundEntityNum = ENTITYNUM_NONE;

	if (ent->NPC->aiFlags & NPCAI_MATCHPLAYERWEAPON)
	{
		//G_MatchPlayerWeapon( ent );
		//rwwFIXMEFIXME: Use this? Probably doesn't really matter for MP.
	}

	ClientThink(ent->s.number, &ucmd);

	trap->LinkEntity((sharedEntity_t*)ent);

	if (ent->client->playerTeam == NPCTEAM_ENEMY)
	{
		//valid enemy spawned
		if (!(ent->spawnflags & SFB_CINEMATIC) && ent->NPC->behaviorState != BS_CINEMATIC)
		{
			//not a cinematic enemy
			if (g_entities[0].client)
			{
				//g_entities[0].client->sess.missionStats.enemiesSpawned++;
				//rwwFIXMEFIXME: Do something here? And this is a rather strange place to be storing
				//this sort of data.
			}
		}
	}
	ent->waypoint = ent->NPC->homeWp = WAYPOINT_NONE;

	if (ent->m_pVehicle)
	{
		//a vehicle
		//check for droidunit
		if (ent->m_pVehicle->m_iDroidUnitTag != -1)
		{
			char* droidNPCType = NULL;
			if (ent->model2
				&& ent->model2[0])
			{
				//specified on the NPC_Vehicle spawner ent
				droidNPCType = ent->model2;
			}
			else if (ent->m_pVehicle->m_pVehicleInfo->droidNPC
				&& ent->m_pVehicle->m_pVehicleInfo->droidNPC[0])
			{
				//specified in the vehicle's .veh file
				droidNPCType = ent->m_pVehicle->m_pVehicleInfo->droidNPC;
			}

			if (droidNPCType != NULL)
			{
				if (Q_stricmp("random", droidNPCType) == 0
					|| Q_stricmp("default", droidNPCType) == 0)
				{
					//use default - R2D2 or R5D2
					if (Q_irand(0, 1))
					{
						droidNPCType = "r2d2";
					}
					else
					{
						droidNPCType = "r5d2";
					}
				}
				gentity_t* droidEnt = NPC_SpawnType(ent, droidNPCType, NULL, qfalse);
				if (droidEnt != NULL)
				{
					if (droidEnt->client)
					{
						droidEnt->client->ps.m_iVehicleNum =
							droidEnt->s.m_iVehicleNum =
							//droidEnt->s.otherentity_num2 =
							droidEnt->s.owner =
							droidEnt->r.ownerNum = ent->s.number;
						ent->m_pVehicle->m_pDroidUnit = (bgEntity_t*)droidEnt;
						//set team
						droidEnt->alliedTeam = ent->alliedTeam;
						droidEnt->teamnodmg = ent->teamnodmg;
						droidEnt->client->sess.sessionTeam = ent->client->sess.sessionTeam;
						droidEnt->client->ps.persistant[PERS_TEAM] = ent->client->ps.persistant[PERS_TEAM];
						//position
						VectorCopy(ent->r.currentOrigin, droidEnt->s.origin);
						VectorCopy(ent->r.currentOrigin, droidEnt->client->ps.origin);
						G_SetOrigin(droidEnt, droidEnt->s.origin);
						trap->LinkEntity((sharedEntity_t*)droidEnt);
						VectorCopy(ent->r.currentAngles, droidEnt->s.angles);
						G_SetAngles(droidEnt, droidEnt->s.angles);
						if (droidEnt->NPC)
						{
							droidEnt->NPC->desiredYaw = droidEnt->s.angles[YAW];
							droidEnt->NPC->desiredPitch = droidEnt->s.angles[PITCH];
						}
						droidEnt->flags |= FL_UNDYING;
					}
					else
					{
						//wtf?
						G_FreeEntity(droidEnt);
					}
				}
			}
		}
	}
}

gNPC_t* gNPCPtrs[MAX_GENTITIES];

static gNPC_t* New_NPC_t(const int entNum)
{
	if (!gNPCPtrs[entNum])
	{
		gNPCPtrs[entNum] = (gNPC_t*)BG_Alloc(sizeof(gNPC_t));
	}

	gNPC_t* ptr = gNPCPtrs[entNum];

	if (ptr)
	{
		// clear it...
		//
		memset(ptr, 0, sizeof * ptr);
	}

	return ptr;
}

static void NPC_DefaultScriptFlags(const gentity_t* ent)
{
	if (!ent || !ent->NPC)
	{
		return;
	}
	//Set up default script flags
	ent->NPC->scriptFlags = SCF_CHASE_ENEMIES | SCF_LOOK_FOR_ENEMIES;
}

/*
-------------------------
NPC_Spawn_Go
-------------------------
*/
extern void G_CreateAnimalNPC(Vehicle_t** p_veh, const char* strAnimalType);
extern void G_CreateSpeederNPC(Vehicle_t** p_veh, const char* strType);
extern void G_CreateWalkerNPC(Vehicle_t** p_veh, const char* strAnimalType);
extern void G_CreateFighterNPC(Vehicle_t** p_veh, const char* strType);

#define TURN_ON				0x00000000
#define TURN_OFF			0x00000100
#define MAX_SAFESPAWN_ENTS 4

static qboolean NPC_SafeSpawn(const gentity_t* ent, const float safeRadius)
{
	int radius_ents[MAX_SAFESPAWN_ENTS];
	vec3_t safeMins, safeMaxs;
	int i;
	const float safeRadiusSquared = safeRadius * safeRadius;

	if (!ent)
	{
		return qfalse;
	}

	//Setup the bbox to search in
	for (i = 0; i < 3; i++)
	{
		safeMins[i] = ent->r.currentOrigin[i] - safeRadius;
		safeMaxs[i] = ent->r.currentOrigin[i] + safeRadius;
	}

	//Get a number of entities in a given space
	const int num_ents = trap->EntitiesInBox(safeMins, safeMaxs, radius_ents, MAX_SAFESPAWN_ENTS);

	for (i = 0; i < num_ents; i++)
	{
		const gentity_t* FoundEnt = &g_entities[radius_ents[i]];

		//Don't consider self
		if (FoundEnt == ent)
			continue;

		if (FoundEnt->NPC && FoundEnt->health <= 0)
		{
			// ignore dead guys
			continue;
		}

		const float distance = DistanceSquared(ent->r.currentOrigin, FoundEnt->r.currentOrigin);

		//Found one too close to us
		if (distance < safeRadiusSquared)
		{
			return qfalse;
		}
	}
	return qtrue;
}

gentity_t* NPC_Spawn_Do(gentity_t* ent)
{
	vec3_t saveOrg;

	if (ent->spawnflags & 4096)
	{
		if (!NPC_SafeSpawn(ent, 64))
		{
			return NULL;
		}
	}

	//Test for drop to floor
	if (ent->spawnflags & NSF_DROP_TO_FLOOR)
	{
		trace_t tr;
		vec3_t bottom;

		VectorCopy(ent->r.currentOrigin, saveOrg);
		VectorCopy(ent->r.currentOrigin, bottom);
		bottom[2] = MIN_WORLD_COORD;
		trap->Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, bottom, ent->s.number, MASK_NPCSOLID, qfalse,
			0, 0);
		if (!tr.allsolid && !tr.startsolid && tr.fraction < 1.0)
		{
			G_SetOrigin(ent, tr.endpos);
		}
	}

	//Check the spawner's count
	if (ent->count != -1)
	{
		ent->count--;

		if (ent->count <= 0)
		{
			ent->use = 0; //never again
			//FIXME: why not remove me...?  Because of all the string pointers?  Just do G_NewStrings?
		}
	}

	gentity_t* newent = G_Spawn();

	if (newent == NULL)
	{
		Com_Printf(S_COLOR_RED "ERROR: NPC G_Spawn failed\n");
		return NULL;
	}

	newent->fullName = ent->fullName;

	newent->NPC = New_NPC_t(newent->s.number);
	if (newent->NPC == NULL)
	{
		Com_Printf(S_COLOR_RED "ERROR: NPC G_Alloc NPC failed\n");
		goto finish;
		//	return NULL;
	}

	G_CreateFakeClient(newent->s.number, &newent->client);

	newent->NPC->tempGoal = G_Spawn();

	if (newent->NPC->tempGoal == NULL)
	{
		newent->NPC = NULL;
		goto finish;
		//	return NULL;
	}

	newent->NPC->tempGoal->classname = "NPC_goal";
	newent->NPC->tempGoal->parent = newent;
	newent->NPC->tempGoal->r.svFlags |= SVF_NOCLIENT;

	if (newent->client == NULL)
	{
		Com_Printf(S_COLOR_RED "ERROR: NPC BG_Alloc client failed\n");
		goto finish;
		//	return NULL;
	}

	memset(newent->client, 0, sizeof * newent->client);

	//Assign the pointer for bg entity access
	newent->playerState = &newent->client->ps;

	//==NPC_Connect( newent, net_name );===================================

	if (ent->NPC_type == NULL)
	{
		ent->NPC_type = "random";
	}
	else
	{
		ent->NPC_type = Q_strlwr(G_NewString(ent->NPC_type));
	}

	if (ent->r.svFlags & SVF_NO_BASIC_SOUNDS)
	{
		newent->r.svFlags |= SVF_NO_BASIC_SOUNDS;
	}
	if (ent->r.svFlags & SVF_NO_COMBAT_SOUNDS)
	{
		newent->r.svFlags |= SVF_NO_COMBAT_SOUNDS;
	}
	if (ent->r.svFlags & SVF_NO_EXTRA_SOUNDS)
	{
		newent->r.svFlags |= SVF_NO_EXTRA_SOUNDS;
	}

	if (ent->message)
	{
		//has a key
		newent->message = ent->message; //transfer the key name
		newent->flags |= FL_NO_KNOCKBACK; //don't fall off ledges
		newent->client->ps.generic1 = 100;
	}

	// If this is a vehicle we need to see what kind it is so we properlly allocate it.
	if (Q_stricmp(ent->classname, "NPC_Vehicle") == 0)
	{
		// Get the vehicle entry index.
		const int iVehIndex = BG_VehicleGetIndex(ent->NPC_type);

		if (iVehIndex == VEHICLE_NONE)
		{
			G_FreeEntity(newent);
			//get rid of the spawner, too, I guess
			G_FreeEntity(ent);
			return NULL;
		}
		// NOTE: If you change/add any of these, update NPC_Spawn_f for the new vehicle you
		// want to be able to spawn in manually.

		// See what kind of vehicle this is and allocate it properly.
		switch (g_vehicleInfo[iVehIndex].type)
		{
		case VH_ANIMAL:
			// Create the animal (making sure all it's data is initialized).
			G_CreateAnimalNPC(&newent->m_pVehicle, ent->NPC_type);
			break;
		case VH_SPEEDER:
			// Create the speeder (making sure all it's data is initialized).
			G_CreateSpeederNPC(&newent->m_pVehicle, ent->NPC_type);
			break;
		case VH_FIGHTER:
			// Create the fighter (making sure all it's data is initialized).
			G_CreateFighterNPC(&newent->m_pVehicle, ent->NPC_type);
			break;
		case VH_WALKER:
			// Create the walker (making sure all it's data is initialized).
			G_CreateWalkerNPC(&newent->m_pVehicle, ent->NPC_type);
			break;

		default:
			Com_Printf(S_COLOR_RED "ERROR: Couldn't spawn NPC %s\n", ent->NPC_type);
			G_FreeEntity(newent);
			//get rid of the spawner, too, I guess
			G_FreeEntity(ent);
			return NULL;
		}

		assert(newent->m_pVehicle &&
			newent->m_pVehicle->m_pVehicleInfo &&
			newent->m_pVehicle->m_pVehicleInfo->Initialize);

		//set up my happy prediction hack
		newent->m_pVehicle->m_vOrientation = &newent->client->ps.vehOrientation[0];

		// Setup the vehicle.
		newent->m_pVehicle->m_pParentEntity = (bgEntity_t*)newent;
		newent->m_pVehicle->m_pVehicleInfo->Initialize(newent->m_pVehicle);

		//cache all the assets
		newent->m_pVehicle->m_pVehicleInfo->RegisterAssets(newent->m_pVehicle);
		//set the class
		newent->client->NPC_class = CLASS_VEHICLE;
		if (g_vehicleInfo[iVehIndex].type == VH_FIGHTER)
		{
			//FIXME: EXTERN!!!
			newent->flags |= FL_NO_KNOCKBACK | FL_SHIELDED | FL_DMG_BY_HEAVY_WEAP_ONLY;
			//don't get pushed around, blasters bounce off, only damage from heavy weaps
		}
		//WTF?!!! Ships spawning in pointing straight down!
		//set them up to start landed
		newent->m_pVehicle->m_vOrientation[YAW] = ent->s.angles[YAW];
		newent->m_pVehicle->m_vOrientation[PITCH] = newent->m_pVehicle->m_vOrientation[ROLL] = 0.0f;
		G_SetAngles(newent, newent->m_pVehicle->m_vOrientation);
		SetClientViewAngle(newent, newent->m_pVehicle->m_vOrientation);

		//newent->m_pVehicle->m_ulFlags |= VEH_GEARSOPEN;
		//why? this would just make it so the initial anim never got played... -rww
		//There was no initial anim, it would just open the gear even though it's already on the ground (fixed now, made an initial anim)

		//For SUSPEND spawnflag, the amount of time to drop like a rock after SUSPEND turns off
		newent->fly_sound_debounce_time = ent->fly_sound_debounce_time;

		//for no-pilot-death delay
		newent->damage = ent->damage;

		//no-pilot-death distance
		newent->speed = ent->speed;

		//for veh transfer all healy stuff
		newent->healingclass = ent->healingclass;
		newent->healingsound = ent->healingsound;
		newent->healingrate = ent->healingrate;
		newent->model2 = ent->model2; //for droidNPC
	}
	else
	{
		newent->client->ps.weapon = WP_NONE; //init for later check in NPC_Begin
	}

	VectorCopy(ent->s.origin, newent->s.origin);
	VectorCopy(ent->s.origin, newent->client->ps.origin);
	VectorCopy(ent->s.origin, newent->r.currentOrigin);
	G_SetOrigin(newent, ent->s.origin); //just to be sure!
	//NOTE: on vehicles, anything in the .npc file will STOMP data on the NPC that's set by the vehicle
	if (!NPC_ParseParms(ent->NPC_type, newent))
	{
		Com_Printf(S_COLOR_RED "ERROR: Couldn't spawn NPC %s\n", ent->NPC_type);
		G_FreeEntity(newent);
		//get rid of the spawner, too, I guess
		G_FreeEntity(ent);
		return NULL;
	}

	if (ent->NPC_type)
	{
		if (!Q_stricmp(ent->NPC_type, "player"))
		{
			//FIXME: "player", not Kyle?  Or check NPC_type against player's NPC_type?
			newent->NPC->aiFlags |= NPCAI_MATCHPLAYERWEAPON;
		}
		else if (!Q_stricmp(ent->NPC_type, "test"))
		{
			for (int n = 0; n < 1; n++)
			{
				if (g_entities[n].s.eType != ET_NPC && g_entities[n].client)
				{
					VectorCopy(g_entities[n].s.origin, newent->s.origin);
					newent->client->playerTeam = newent->s.teamowner = g_entities[n].client->playerTeam;
					break;
				}
			}
			newent->NPC->defaultBehavior = newent->NPC->behaviorState = BS_WAIT;
			newent->classname = "NPC";
		}
	}
	//=====================================================================
	//set the info we want
	if (!newent->health)
	{
		newent->health = ent->health;
	}
	newent->script_targetname = ent->NPC_targetname;
	newent->targetname = ent->NPC_targetname;
	newent->target = ent->NPC_target; //death
	newent->target2 = ent->target2; //knocked out death
	newent->target3 = ent->target3; //???
	newent->target4 = ent->target4; //ffire death
	newent->wait = ent->wait;

	for (int index = BSET_FIRST; index < NUM_BSETS; index++)
	{
		if (ent->behaviorSet[index])
		{
			newent->behaviorSet[index] = ent->behaviorSet[index];
		}
	}

	newent->classname = "NPC";
	newent->NPC_type = ent->NPC_type;
	trap->UnlinkEntity((sharedEntity_t*)newent);

	VectorCopy(ent->s.angles, newent->s.angles);
	VectorCopy(ent->s.angles, newent->r.currentAngles);
	VectorCopy(ent->s.angles, newent->client->ps.viewangles);
	newent->NPC->desiredYaw = ent->s.angles[YAW];

	trap->LinkEntity((sharedEntity_t*)newent);
	newent->spawnflags = ent->spawnflags;

	if (ent->paintarget)
	{
		//safe to point at owner's string since memory is never freed during game
		newent->paintarget = ent->paintarget;
	}
	if (ent->opentarget)
	{
		newent->opentarget = ent->opentarget;
	}

	//==New stuff=====================================================================
	newent->s.eType = ET_NPC; //ET_PLAYER;

	//FIXME: Call CopyParms
	if (ent->parms)
	{
		for (int parmNum = 0; parmNum < MAX_PARMS; parmNum++)
		{
			if (ent->parms->parm[parmNum][0])
			{
				Q3_SetParm(newent->s.number, parmNum, ent->parms->parm[parmNum]);
			}
		}
	}
	//FIXME: copy cameraGroup, store mine in message or other string field

	//set origin
	newent->s.pos.trType = TR_INTERPOLATE;
	newent->s.pos.trTime = level.time;
	VectorCopy(newent->r.currentOrigin, newent->s.pos.trBase);
	VectorClear(newent->s.pos.trDelta);
	newent->s.pos.trDuration = 0;
	//set angles
	newent->s.apos.trType = TR_INTERPOLATE;
	newent->s.apos.trTime = level.time;
	//VectorCopy( newent->r.currentOrigin, newent->s.apos.trBase );
	//Why was the origin being used as angles? Typo I'm assuming -rww
	VectorCopy(newent->s.angles, newent->s.apos.trBase);

	VectorClear(newent->s.apos.trDelta);
	newent->s.apos.trDuration = 0;

	newent->NPC->combatPoint = -1;

	newent->flags |= FL_NOTARGET; //So he's ignored until he's fully spawned
	newent->s.eFlags |= EF_NODRAW; //So he's ignored until he's fully spawned

	newent->think = NPC_Begin;
	newent->nextthink = level.time + FRAMETIME;
	NPC_DefaultScriptFlags(newent);

	//copy over team variables, too
	newent->s.shouldtarget = ent->s.shouldtarget;
	newent->s.teamowner = ent->s.teamowner;
	newent->alliedTeam = ent->alliedTeam;
	newent->teamnodmg = ent->teamnodmg;

	//allow the NPC's default team to override if the team isn't set and you're in CoOp mode
	if (ent->s.teamowner != TEAM_FREE || level.gametype != GT_SINGLE_PLAYER)
	{
		newent->s.teamowner = ent->s.teamowner;
	}

	if (!newent->message && newent->client->NPC_class == CLASS_IMPERIAL)
	{
		//the imperial model has a key by default, remove it since we don't have a key
		NPC_SetSurfaceOnOff(newent, "l_arm_key", TURN_OFF);
	}

	if (ent->team && ent->team[0])
	{
		//specified team directly?
		newent->client->sess.sessionTeam = atoi(ent->team);
	}
	else if (newent->s.teamowner != TEAM_FREE)
	{
		newent->client->sess.sessionTeam = newent->s.teamowner;
	}
	else if (newent->alliedTeam != TEAM_FREE)
	{
		newent->client->sess.sessionTeam = newent->alliedTeam;
	}
	else if (newent->teamnodmg != TEAM_FREE)
	{
		newent->client->sess.sessionTeam = newent->teamnodmg;
	}
	else
	{
		newent->client->sess.sessionTeam = TEAM_FREE;
	}
	newent->client->ps.persistant[PERS_TEAM] = newent->client->sess.sessionTeam;

	trap->LinkEntity((sharedEntity_t*)newent);

	if (!ent->use)
	{
		if (ent->target)
		{
			//use any target we're pointed at
			G_UseTargets(ent, ent);
		}
		if (ent->closetarget)
		{
			//last guy should fire this target when he dies
			newent->target = ent->closetarget;
		}
		ent->targetname = NULL;
		//why not remove me...?  Because of all the string pointers?  Just do G_NewStrings?
		G_FreeEntity(ent); //bye!
	}

finish:
	if (ent->spawnflags & NSF_DROP_TO_FLOOR)
	{
		G_SetOrigin(ent, saveOrg);
	}

	// added some attributes initialization
	if (newent && newent->client)
	{
		newent->client->cloakDebReduce = 0;

		// saboteur npcs start with cloak
		if (Q_stristr(newent->NPC_type, "saboteur"))
		{
			Saboteur_Cloak(newent);
		}
	}

	return newent;
}

void NPC_Spawn_Go(gentity_t* ent)
{
	NPC_Spawn_Do(ent);
}

/*
-------------------------
NPC_ShySpawn
-------------------------
*/

#define SHY_THINK_TIME			1000
#define SHY_SPAWN_DISTANCE		128
#define SHY_SPAWN_DISTANCE_SQR	( SHY_SPAWN_DISTANCE * SHY_SPAWN_DISTANCE )

void NPC_ShySpawn(gentity_t* ent)
{
	ent->nextthink = level.time + SHY_THINK_TIME;
	ent->think = NPC_ShySpawn;

	//rwwFIXMEFIXME: Care about other clients not just 0?
	if (DistanceSquared(g_entities[0].r.currentOrigin, ent->r.currentOrigin) <= SHY_SPAWN_DISTANCE_SQR)
		return;

	if (InFOV(ent, &g_entities[0], 80, 64)) // FIXME: hardcoded fov
		if (NPC_ClearLOS2(&g_entities[0], ent->r.currentOrigin))
			return;

	ent->think = 0;
	ent->nextthink = 0;

	NPC_Spawn_Go(ent);
}

/*
-------------------------
NPC_Spawn
-------------------------
*/

void NPC_Spawn(gentity_t* ent, const gentity_t* other, gentity_t* activator)
{
	//delay before spawning NPC
	if (ent->delay)
	{
		if (ent->spawnflags & 2048) // SHY
		{
			ent->think = NPC_ShySpawn;
		}
		else
		{
			ent->think = NPC_Spawn_Go;
		}

		ent->nextthink = level.time + ent->delay;
	}
	else
	{
		if (ent->spawnflags & 2048) // SHY
		{
			NPC_ShySpawn(ent);
		}
		else
		{
			NPC_Spawn_Do(ent);
		}
	}
}

/*QUAKED NPC_spawner (1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

NPC_type - name of NPC (in npcs.cfg) to spawn in

targetname - name this NPC goes by for targetting
target - NPC will fire this when it spawns it's last NPC (should this be when the last NPC it spawned dies?)
target2 - Fired by stasiss spawners when they try to spawn while their spawner model is broken
target3 - Fired by spawner if they try to spawn and are blocked and have a wait < 0 (removes them)

If targeted, will only spawn a NPC when triggered
count - how many NPCs to spawn (only if targetted) default = 1
delay - how long to wait to spawn after used
wait - if trying to spawn and blocked, how many seconds to wait before trying again (default = 0.5, < 0 = never try again and fire target2)

NPC_targetname - NPC's targetname AND script_targetname
NPC_target - NPC's target to fire when killed
NPC_target2 - NPC's target to fire when knocked out
NPC_target4 - NPC's target to fire when killed by friendly fire
NPC_type - type of NPC ("Borg" (default), "Xian", etc)
health - starting health (default = 100)

spawnscript - default script to run once spawned (none by default)
usescript - default script to run when used (none by default)
awakescript - default script to run once awoken (none by default)
angerscript - default script to run once angered (none by default)
painscript - default script to run when hit (none by default)
fleescript - default script to run when hit and below 50% health (none by default)
deathscript - default script to run when killed (none by default)
These strings can be used to activate behaviors instead of scripts - these are checked
first and so no scripts should be names with these names:
	default - 0: whatever
	idle - 1: Stand around, do abolutely nothing
	roam - 2: Roam around, collect stuff
	walk - 3: Crouch-Walk toward their goals
	run - 4: Run toward their goals
	standshoot - 5: Stay in one spot and shoot- duck when neccesary
	standguard - 6: Wait around for an enemy
	patrol - 7: Follow a path, looking for enemies
	huntkill - 8: Track down enemies and kill them
	evade - 9: Run from enemies
	evadeshoot - 10: Run from enemies, shoot them if they hit you
	runshoot - 11: Run to your goal and shoot enemy when possible
	defend - 12: Defend an entity or spot?
	snipe - 13: Stay hidden, shoot enemy only when have perfect shot and back turned
	combat - 14: Attack, evade, use cover, move about, etc.  Full combat AI - id NPC code
	medic - 15: Go for lowest health buddy, hide and heal him.
	takecover - 16: Find nearest cover from enemies
	getammo - 17: Go get some ammo
	advancefight - 18: Go somewhere and fight along the way
	face - 19: turn until facing desired angles
	wait - 20: do nothing
	formation - 21: Maintain a formation
	crouch - 22: Crouch-walk toward their goals

delay - after spawned or triggered, how many seconds to wait to spawn the NPC

showhealth - set to 1 to show health bar on this entity when crosshair is over it

teamowner - crosshair shows green for this team, red for opposite team
	0 - none
	1 - red
	2 - blue

teamuser - only this team can use this NPC
	0 - none
	1 - red
	2 - blue

teamnodmg - team that NPC does not take damage from (turrets and other auto-defenses that have "alliedTeam" set to this team won't target this NPC)
	0 - none
	1 - red
	2 - blue

"noBasicSounds" - set to 1 to prevent loading and usage of basic sounds (pain, death, etc)
"noCombatSounds" - set to 1 to prevent loading and usage of combat sounds (anger, victory, etc.)
"noExtraSounds" - set to 1 to prevent loading and usage of "extra" sounds (chasing the enemy - detecting them, flanking them... also jedi combat sounds)
*/
extern void NPC_PrecacheAnimationCFG(const char* npc_type);
void NPC_Precache(gentity_t* spawner);

static void NPC_PrecacheType(char* NPC_type)
{
	gentity_t* fakespawner = G_Spawn();
	if (fakespawner)
	{
		fakespawner->NPC_type = NPC_type;
		NPC_Precache(fakespawner);
		//NOTE: does the spawner have to stay around to send any precached info to the clients...?
		G_FreeEntity(fakespawner);
	}
}

extern void NPC_PrecacheByClassName(const char* type);
extern void Precashe_Key(void);

void SP_NPC_spawner(gentity_t* self)
{
	int t;

	if (level.gametype == GT_SINGLE_PLAYER && !g_allowNPC.integer) //singleplayer npcs turned off
	{
		// exceptions so scripts can run
		if (!(Q_stricmp(self->NPC_type, "ImpOfficer") == 0
			|| Q_stricmp(self->NPC_type, "ImpOfficer2") == 0
			|| Q_stricmp(self->NPC_type, "ImpCommander") == 0
			|| Q_stricmp(self->NPC_type, "Imperial") == 0
			|| Q_stricmp(self->NPC_type, "Imperial2") == 0
			|| Q_stricmp(self->NPC_type, "ImperialJK2") == 0
			|| Q_stricmp(self->NPC_type, "kyle") == 0
			|| Q_stricmp(self->NPC_type, "Kyle2") == 0
			|| Q_stricmp(self->NPC_type, "Kyle_boss") == 0
			|| Q_stricmp(self->NPC_type, "KyleJK2") == 0
			|| Q_stricmp(self->NPC_type, "KyleJK2noforce") == 0
			|| Q_stricmp(self->NPC_type, "Kyle_JKA") == 0
			|| Q_stricmp(self->NPC_type, "outcast_jedi") == 0
			|| Q_stricmp(self->NPC_type, "Player") == 0
			|| Q_stricmp(self->NPC_type, "Player1") == 0
			|| Q_stricmp(self->NPC_type, "Player2") == 0
			|| Q_stricmp(self->NPC_type, "Player3") == 0
			|| Q_stricmp(self->NPC_type, "Jan") == 0
			|| Q_stricmp(self->NPC_type, "Jan2") == 0
			|| Q_stricmp(self->NPC_type, "JanJK2") == 0
			|| Q_stricmp(self->NPC_type, "sand_creature") == 0
			|| Q_stricmp(self->NPC_type, "sand_creature_fast") == 0
			|| Q_stricmp(self->NPC_type, "mutant_rancor") == 0
			|| Q_stricmp(self->NPC_type, "wampa") == 0
			|| Q_stricmp(self->NPC_type, "Prisoner") == 0
			|| Q_stricmp(self->NPC_type, "Prisoner2") == 0
			|| Q_stricmp(self->NPC_type, "Prisoner3") == 0
			|| Q_stricmp(self->NPC_type, "rancor") == 0
			|| Q_stricmp(self->NPC_type, "rancor2") == 0
			|| Q_stricmp(self->NPC_type, "probe") == 0
			|| Q_stricmp(self->NPC_type, "mark1") == 0
			|| Q_stricmp(self->NPC_type, "mark2") == 0
			|| Q_stricmp(self->NPC_type, "Luke") == 0
			|| Q_stricmp(self->NPC_type, "LukeJK2") == 0
			|| Q_stricmp(self->NPC_type, "lannik_racto") == 0
			|| Q_stricmp(self->NPC_type, "interrogator") == 0
			|| Q_stricmp(self->NPC_type, "gonk") == 0
			|| Q_stricmp(self->NPC_type, "gonk2") == 0
			|| Q_stricmp(self->NPC_type, "wampa") == 0))
		{
			self->think = G_FreeEntity;
			self->nextthink = level.time;
			return;
		}
	}
	else
	{
		if (level.gametype != GT_SINGLE_PLAYER) //not single player so no npcs
		{
			self->think = G_FreeEntity;
			self->nextthink = level.time;
			return;
		}
		//restrictions for stability if using devmap or allowwnpc is on
		if (level.is_t1_fatal_map == qtrue &&
			(Q_stricmp(self->NPC_type, "ImpWorker") == 0 ||
				Q_stricmp(self->NPC_type, "ImpWorker2") == 0 ||
				Q_stricmp(self->NPC_type, "ImpWorker3") == 0))
		{
			// t1_fatal map will not load these npcs to avoid crashing clients
			self->think = G_FreeEntity;
			self->nextthink = level.time;
			return;
		}

		if (level.is_t3_hevil_map == qtrue &&
			Q_stricmp(self->NPC_type, "Noghri") == 0)
		{
			// t3_hevil map will not load these npcs to avoid crashing clients
			self->think = G_FreeEntity;
			self->nextthink = level.time;
			return;
		}

		if (level.is_vjun3_map == qtrue &&
			(Q_stricmp(self->NPC_type, "protocol_imp") == 0 ||
				Q_stricmp(self->NPC_type, "r2d2_imp") == 0))
		{
			// vjun3 map will not load these npcs to avoid exceeding npc register limit and crashing clients
			self->think = G_FreeEntity;
			self->nextthink = level.time;
			return;
		}

		if (level.is_t3_rift_map == qtrue &&
			(Q_stricmp(self->NPC_type, "reborn_dual") == 0 ||
				Q_stricmp(self->NPC_type, "reborn_dual2") == 0 ||
				Q_stricmp(self->NPC_type, "RebornMasterDual") == 0 ||
				Q_stricmp(self->NPC_type, "cultist_grip") == 0 ||
				Q_stricmp(self->NPC_type, "human_merc_flc") == 0 ||
				Q_stricmp(self->NPC_type, "human_merc_cnc") == 0 ||
				Q_stricmp(self->NPC_type, "human_merc") == 0))
		{
			// t3_rift map will not load these npcs to avoid crashing clients
			self->think = G_FreeEntity;
			self->nextthink = level.time;
			return;
		}

		if (level.is_t1_rail_map == qtrue &&
			(Q_stricmp(self->NPC_type, "Reborn") == 0 ||
				Q_stricmp(self->NPC_type, "cultist_saber") == 0 ||
				Q_stricmp(self->NPC_type, "human_merc") == 0 ||
				Q_stricmp(self->NPC_type, "human_merc2") == 0))
		{
			// map will not load these npcs to avoid  crashing clients
			self->think = G_FreeEntity;
			self->nextthink = level.time;
			return;
		}

		if (level.is_t1_sour_map == qtrue &&
			Q_stricmp(self->NPC_type, "chewie") == 0)
		{
			// map will not load these npcs to avoid  crashing clients
			self->think = G_FreeEntity;
			self->nextthink = level.time;
			return;
		}

		if (level.is_t2_rouge_map == qtrue &&
			(Q_stricmp(self->NPC_type, "jan") == 0 ||
				Q_stricmp(self->NPC_type, "human_merc_bow") == 0))
		{
			// map will not load these npcs to avoid  crashing clients
			self->think = G_FreeEntity;
			self->nextthink = level.time;
			return;
		}

		if (level.is_t3_byss_map == qtrue &&
			(Q_stricmp(self->NPC_type, "mark1") == 0 ||
				Q_stricmp(self->NPC_type, "mark2") == 0 ||
				Q_stricmp(self->NPC_type, "sentry") == 0))
		{
			// map will not load these npcs to avoid  crashing clients
			self->think = G_FreeEntity;
			self->nextthink = level.time;
			return;
		}
	}

	if (!self->fullName || !self->fullName[0])
	{
		//FIXME: make an index into an external string table for localization
		self->fullName = "Humanoid Lifeform";
	}

	if (!self->count)
	{
		self->count = 1;
	}

	{
		//Stop loading of certain extra sounds
		static int garbage;

		if (G_SpawnInt("noBasicSounds", "0", &garbage))
		{
			self->r.svFlags |= SVF_NO_BASIC_SOUNDS;
		}
		if (G_SpawnInt("noCombatSounds", "0", &garbage))
		{
			self->r.svFlags |= SVF_NO_COMBAT_SOUNDS;
		}
		if (G_SpawnInt("noExtraSounds", "0", &garbage))
		{
			self->r.svFlags |= SVF_NO_EXTRA_SOUNDS;
		}
	}

	if (!self->wait)
	{
		self->wait = 500;
	}
	else
	{
		if (!(self->spawnflags & 65536)) // dont multiply if has this flag
		{
			self->wait *= 1000; //1 = 1 msec, 1000 = 1 sec
		}
	}

	if (!(self->spawnflags & 65536)) // dont multiply if has this flag
	{
		self->wait *= 1000; //1 = 1 msec, 1000 = 1 sec
	}

	self->spawnflags |= 65536;

	G_SpawnInt("showhealth", "0", &t);
	if (t)
	{
		self->s.shouldtarget = qtrue;
	}

	//We have to load the animation.cfg now because spawnscripts are going to want to set anims and we need to know their length and if they're valid
	NPC_PrecacheAnimationCFG(self->NPC_type);

	//rww - can't cheat and do this on the client like in SP, so I'm doing this.
	NPC_Precache(self);

	if (self->targetname)
	{
		//Wait for triggering
		self->use = NPC_Spawn;
		self->r.svFlags |= SVF_NPC_PRECACHE;
	}
	else
	{
		//NOTE: auto-spawners never check for shy spawning

		if (1) //just gonna always do this I suppose.
		{
			//in entity spawn stage - map starting up
			self->think = NPC_Spawn_Go;
			self->nextthink = level.time + START_TIME_REMOVE_ENTS + 50;
		}
	}

	NPC_PrecacheByClassName(self->NPC_type); //precashe additional fx/sounds/models/etc
	//for special NPC classes
	if (self->message)
	{
		//precashe key stuff if we're set to carry a key.
		Precashe_Key();
	}
}

extern void G_VehicleSpawn(gentity_t* self);
/*QUAKED NPC_Vehicle (1 0 0) (-16 -16 -24) (16 16 32) NO_PILOT_DIE SUSPENDED x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
NO_PILOT_DIE - die after certain amount of time of not having a pilot
SUSPENDED - Fighters: Don't drop until someone gets in it (this only works as long as no-nw has *ever* ridden the vehicle, to simulate ships that are suspended-docked) - note: ships inside trigger_spaces do not drop when unoccupied
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

set NPC_type to vehicle name in vehicles.dat

"dropTime" use with SUSPENDED - if set, the vehicle will drop straight down for this number of seconds before flying forward
"dmg" use with NO_PILOT_DIE - delay in milliseconds for ship to explode if no pilot (default 10000)
"speed" use with NO_PILOT_DIE - distance for pilot to get away from ship after dismounting before it starts counting down the death timer
"model2" - if the vehicle can have a droid (has "*droidunit" tag), this NPC will be spawned and placed there - note: game will automatically use the one specified in the .veh file (if any) or, absent that, it will use an R2D2 or R5D2 NPC)

showhealth - set to 1 to show health bar on this entity when crosshair is over it

teamowner - crosshair shows green for this team, red for opposite team
	0 - none
	1 - red
	2 - blue

teamuser - only this team can use this NPC
	0 - none
	1 - red
	2 - blue

teamnodmg - team that NPC does not take damage from (turrets and other auto-defenses that have "alliedTeam" set to this team won't target this NPC)
	0 - none
	1 - red
	2 - blue
*/
static qboolean NPC_VehiclePrecache(const gentity_t* spawner)
{
	char* droidNPCType = NULL;
	const int iVehIndex = BG_VehicleGetIndex(spawner->NPC_type);
	if (iVehIndex == VEHICLE_NONE)
	{
		//fixme: error msg?
		return qfalse;
	}

	G_model_index(va("$%s", spawner->NPC_type)); //make sure the thing is frickin precached
	//now cache his model/skin/anim config
	const vehicleInfo_t* pVehInfo = &g_vehicleInfo[iVehIndex];
	if (pVehInfo->model && pVehInfo->model[0])
	{
		void* tempG2 = NULL;
		int skin = 0;
		if (pVehInfo->skin && pVehInfo->skin[0])
		{
			skin = trap->R_RegisterSkin(va("models/players/%s/model_%s.skin", pVehInfo->model, pVehInfo->skin));
		}
		trap->G2API_InitGhoul2Model(&tempG2, va("models/players/%s/model.glm", pVehInfo->model), 0, skin, 0, 0, 0);
		if (tempG2)
		{
			//now, cache the anim config.
			char gla_name[1024];

			gla_name[0] = 0;
			trap->G2API_GetGLAName(tempG2, 0, gla_name);

			if (gla_name[0])
			{
				char* slash = Q_strrchr(gla_name, '/');
				if (slash)
				{
					strcpy(slash, "/animation.cfg");

					bg_parse_animation_file(gla_name, NULL, qfalse);
				}
			}
			trap->G2API_CleanGhoul2Models(&tempG2);
		}
	}

	//also precache the droid NPC if there is one
	if (spawner->model2
		&& spawner->model2[0])
	{
		droidNPCType = spawner->model2;
	}
	else if (g_vehicleInfo[iVehIndex].droidNPC
		&& g_vehicleInfo[iVehIndex].droidNPC[0])
	{
		droidNPCType = g_vehicleInfo[iVehIndex].droidNPC;
	}

	if (droidNPCType)
	{
		if (Q_stricmp("random", droidNPCType) == 0
			|| Q_stricmp("default", droidNPCType) == 0)
		{
			//precache both r2 and r5, as defaults
			NPC_PrecacheType("r2d2");
			NPC_PrecacheType("r5d2");
		}
		else
		{
			NPC_PrecacheType(droidNPCType);
		}
	}
	return qtrue;
}

void NPC_VehicleSpawnUse(gentity_t* self, gentity_t* other, gentity_t* activator)
{
	if (self->delay)
	{
		self->think = G_VehicleSpawn;
		self->nextthink = level.time + self->delay;
	}
	else
	{
		G_VehicleSpawn(self);
	}
}

void SP_NPC_Vehicle(gentity_t* self)
{
	float dropTime;
	int t;
	if (!self->NPC_type)
	{
		self->NPC_type = "swoop";
	}

	if (!self->classname)
	{
		self->classname = "NPC_Vehicle";
	}

	if (!self->wait)
	{
		self->wait = 500;
	}
	else
	{
		self->wait *= 1000; //1 = 1 msec, 1000 = 1 sec
	}
	self->delay *= 1000; //1 = 1 msec, 1000 = 1 sec

	G_SetOrigin(self, self->s.origin);
	G_SetAngles(self, self->s.angles);
	G_SpawnFloat("dropTime", "0", &dropTime);
	if (dropTime)
	{
		self->fly_sound_debounce_time = ceil(dropTime * 1000.0);
	}

	G_SpawnInt("showhealth", "0", &t);
	if (t)
	{
		self->s.shouldtarget = qtrue;
	}

	if (self->targetname)
	{
		if (!NPC_VehiclePrecache(self))
		{
			//FIXME: err msg?
			G_FreeEntity(self);
			return;
		}
		self->use = NPC_VehicleSpawnUse;
	}
	else
	{
		if (self->delay)
		{
			if (!NPC_VehiclePrecache(self))
			{
				//FIXME: err msg?
				G_FreeEntity(self);
				return;
			}
			self->think = G_VehicleSpawn;
			self->nextthink = level.time + self->delay;
		}
		else
		{
			G_VehicleSpawn(self);
		}
	}
}

//Characters

//STAR WARS NPCs============================================================================

//=============================================================================================
//CHARACTERS
//=============================================================================================

/*QUAKED NPC_Kyle (1 0 0) (-16 -16 -24) (16 16 32) x RIFLEMAN PHASER TRICORDER DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Kyle(gentity_t* self)
{
	if (self->spawnflags & 1)
	{
		self->NPC_type = "Kyle_boss";
	}
	else
	{
		self->NPC_type = "Kyle";
	}

	wp_set_saber_model(NULL, CLASS_KYLE);

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Lando(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Lando(gentity_t* self)
{
	self->NPC_type = "Lando";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Jan(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Jan(gentity_t* self)
{
	self->NPC_type = "Jan";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Luke(1 0 0) (-16 -16 -24) (16 16 40) x x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Luke(gentity_t* self)
{
	self->NPC_type = "Luke";

	wp_set_saber_model(NULL, CLASS_LUKE);

	SP_NPC_spawner(self);
}

void SP_NPC_Merchant(gentity_t* self)
{
	self->NPC_type = "merchant";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_MonMothma(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_MonMothma(gentity_t* self)
{
	self->NPC_type = "MonMothma";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Tavion (1 0 0) (-16 -16 -24) (16 16 32) x x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Tavion(gentity_t* self)
{
	self->NPC_type = "Tavion";

	wp_set_saber_model(NULL, CLASS_TAVION);

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Tavion_New (1 0 0) (-16 -16 -24) (16 16 32) SCEPTER SITH_SWORD x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
Has a red lightsaber and force powers, uses her saber style from JK2

SCEPTER - Has a red lightsaber and force powers, Ragnos' Scepter in left hand, uses dual saber style and occasionally attacks with Scepter
SITH_SWORD - Has Ragnos' Sith Sword in right hand and force powers, uses strong style
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Tavion_New(gentity_t* self)
{
	if (self->spawnflags & 1)
	{
		self->NPC_type = "tavion_scepter";
	}
	else if (self->spawnflags & 2)
	{
		self->NPC_type = "tavion_sith_sword";
	}
	else
	{
		self->NPC_type = "tavion_new";
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Alora (1 0 0) (-16 -16 -24) (16 16 32) DUAL x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
Lightsaber and level 2 force powers, 300 health

DUAL - Dual sabers and level 3 force powers, 500 health
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Alora(gentity_t* self)
{
	if (self->spawnflags & 1)
	{
		self->NPC_type = "alora_dual";
	}
	else
	{
		self->NPC_type = "alora";
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Reborn_New(1 0 0) (-16 -16 -24) (16 16 40) DUAL STAFF WEAK x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY

Reborn is an excellent lightsaber fighter, acrobatic and uses force powers.  Full-length red saber, 200 health.

DUAL - Use 2 shorter sabers
STAFF - Uses a saber staff
WEAK - Is a bit less tough
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Reborn_New(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 4)
		{
			//weaker guys
			if (self->spawnflags & 1)
			{
				self->NPC_type = "reborn_dual2";
			}
			else if (self->spawnflags & 2)
			{
				self->NPC_type = "reborn_staff2";
			}
			else
			{
				self->NPC_type = "reborn_new2";
			}
		}
		else
		{
			if (self->spawnflags & 1)
			{
				self->NPC_type = "reborn_dual";
			}
			else if (self->spawnflags & 2)
			{
				self->NPC_type = "reborn_staff";
			}
			else
			{
				self->NPC_type = "reborn_new";
			}
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Cultist_Saber(1 0 0) (-16 -16 -24) (16 16 40) MED STRONG ALL THROW CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
Uses a saber and no force powers.  100 health.

default fencer uses fast style - weak, but can attack rapidly.  Good defense.

MED - Uses medium style - average speed and attack strength, average defense.
STRONG - Uses strong style, slower than others, but can do a lot of damage with one blow.  Weak defense.
ALL - Knows all 3 styles, switches between them, good defense.
THROW - can throw their saber (level 2) - reduces their defense some (can use this spawnflag alone or in combination with any *one* of the previous 3 spawnflags)

CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Cultist_Saber(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			if (self->spawnflags & 8)
			{
				self->NPC_type = "cultist_saber_med_throw";
			}
			else
			{
				self->NPC_type = "cultist_saber_med";
			}
		}
		else if (self->spawnflags & 2)
		{
			if (self->spawnflags & 8)
			{
				self->NPC_type = "cultist_saber_strong_throw";
			}
			else
			{
				self->NPC_type = "cultist_saber_strong";
			}
		}
		else if (self->spawnflags & 4)
		{
			if (self->spawnflags & 8)
			{
				self->NPC_type = "cultist_saber_all_throw";
			}
			else
			{
				self->NPC_type = "cultist_saber_all";
			}
		}
		else
		{
			if (self->spawnflags & 8)
			{
				self->NPC_type = "cultist_saber_throw";
			}
			else
			{
				self->NPC_type = "cultist_saber";
			}
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Cultist_Saber_Powers(1 0 0) (-16 -16 -24) (16 16 40) MED STRONG ALL THROW CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
Uses a saber and has a couple low-level powers.  150 health.

default fencer uses fast style - weak, but can attack rapidly.  Good defense.

MED - Uses medium style - average speed and attack strength, average defense.
STRONG - Uses strong style, slower than others, but can do a lot of damage with one blow.  Weak defense.
ALL - Knows all 3 styles, switches between them, good defense.
THROW - can throw their saber (level 2) - reduces their defense some (can use this spawnflag alone or in combination with any *one* of the previous 3 spawnflags)

CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Cultist_Saber_Powers(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			if (self->spawnflags & 8)
			{
				self->NPC_type = "cultist_saber_med_throw2";
			}
			else
			{
				self->NPC_type = "cultist_saber_med2";
			}
		}
		else if (self->spawnflags & 2)
		{
			if (self->spawnflags & 8)
			{
				self->NPC_type = "cultist_saber_strong_throw2";
			}
			else
			{
				self->NPC_type = "cultist_saber_strong2";
			}
		}
		else if (self->spawnflags & 4)
		{
			if (self->spawnflags & 8)
			{
				self->NPC_type = "cultist_saber_all_throw2";
			}
			else
			{
				self->NPC_type = "cultist_saber_all2";
			}
		}
		else
		{
			if (self->spawnflags & 8)
			{
				self->NPC_type = "cultist_saber_throw";
			}
			else
			{
				self->NPC_type = "cultist_saber2";
			}
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Cultist(1 0 0) (-16 -16 -24) (16 16 40) SABER GRIP LIGHTNING DRAIN CEILING CINEMATIC NOTSOLID STARTINSOLID SHY

Cultist uses a blaster and force powers.  40 health.

SABER - Uses a saber and no force powers
GRIP - Uses no weapon and grip, push and pull
LIGHTNING - Uses no weapon and lightning and push
DRAIN - Uses no weapons and drain and push

CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Cultist(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			self->NPC_type = NULL;
			self->spawnflags = 0; //fast, no throw
			switch (Q_irand(0, 2))
			{
			case 0: //medium
				self->spawnflags |= 1;
				break;
			case 1: //strong
				self->spawnflags |= 2;
				break;
			case 2: //all
				self->spawnflags |= 4;
				break;
			default:;
			}
			if (Q_irand(0, 1))
			{
				//throw
				self->spawnflags |= 8;
			}
			SP_NPC_Cultist_Saber(self);
			return;
		}
		if (self->spawnflags & 2)
		{
			self->NPC_type = "cultist_grip";
		}
		else if (self->spawnflags & 4)
		{
			self->NPC_type = "cultist_lightning";
		}
		else if (self->spawnflags & 8)
		{
			if (!Q_irand(0, 1))
			{
				self->NPC_type = "cultist_drain";
			}
			else
			{
				self->NPC_type = "cultist_destroyer";
			}
		}
		else
		{
			self->NPC_type = "cultist";
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Cultist_Commando(1 0 0) (-16 -16 -24) (16 16 40) x x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY

Cultist uses dual blaster pistols and force powers.  40 health.

CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Cultist_Commando(gentity_t* self)
{
	if (!self->NPC_type)
	{
		self->NPC_type = "cultistcommando";
	}
	SP_NPC_spawner(self);
}

/*QUAKED NPC_Cultist_Destroyer(1 0 0) (-16 -16 -24) (16 16 40) x x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY

Cultist has no weapons, runs up to you chanting & building up a Force Destruction blast - when gets to you, screams & explodes

CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Cultist_Destroyer(gentity_t* self)
{
	self->NPC_type = "cultist_destroyer"; //"cultist_explode";
	SP_NPC_spawner(self);
}

/*QUAKED NPC_Reelo(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Reelo(gentity_t* self)
{
	self->NPC_type = "Reelo";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Galak(1 0 0) (-16 -16 -24) (16 16 40) MECH x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
MECH - will be the armored Galak

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Galak(gentity_t* self)
{
	if (self->spawnflags & 1)
	{
		self->NPC_type = "Galak_Mech";
		NPC_GalakMech_Precache();
	}
	else
	{
		self->NPC_type = "Galak";
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Desann(1 0 0) (-16 -16 -24) (16 16 40) x x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Desann(gentity_t* self)
{
	self->NPC_type = "Desann";

	wp_set_saber_model(NULL, CLASS_DESANN);

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Bartender(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Bartender(gentity_t* self)
{
	self->NPC_type = "Bartender";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_MorganKatarn(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_MorganKatarn(gentity_t* self)
{
	self->NPC_type = "MorganKatarn";

	SP_NPC_spawner(self);
}

//=============================================================================================
//ALLIES
//=============================================================================================

/*QUAKED NPC_Jedi(1 0 0) (-16 -16 -24) (16 16 40) TRAINER MASTER RANDOM x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
TRAINER - Special Jedi- instructor
MASTER - Special Jedi- master
RANDOM - creates a random Jedi student using the available player models/skins
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Ally Jedi NPC Buddy - tags along with player
*/
void SP_NPC_Jedi(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 4)
		{
			//random!
			int sanityCheck = 20; //just in case
			while (sanityCheck--)
			{
				switch (Q_irand(0, 11))
				{
				case 0:
					self->NPC_type = "jedi_hf1";
					break;
				case 1:
					self->NPC_type = "jedi_hf2";
					break;
				case 2:
					self->NPC_type = "jedi_hm1";
					break;
				case 3:
					self->NPC_type = "jedi_hm2";
					break;
				case 4:
					self->NPC_type = "jedi_kdm1";
					break;
				case 5:
					self->NPC_type = "jedi_kdm2";
					break;
				case 6:
					self->NPC_type = "jedi_rm1";
					break;
				case 7:
					self->NPC_type = "jedi_rm2";
					break;
				case 8:
					self->NPC_type = "jedi_tf1";
					break;
				case 9:
					self->NPC_type = "jedi_tf2";
					break;
				case 10:
					self->NPC_type = "jedi_zf1";
					break;
				case 11:
				default: //just in case
					self->NPC_type = "jedi_zf2";
					break;
				}
			}
		}
		else if (self->spawnflags & 2)
		{
			switch (Q_irand(0, 2))
			{
			case 0:
				self->NPC_type = "jedimaster";
				break;
			case 1:
				self->NPC_type = "jedimaster2";
				break;
			case 2:
				self->NPC_type = "jedimaster3";
				break;
			default:;
			}
		}
		else if (self->spawnflags & 1)
		{
			switch (Q_irand(0, 3))
			{
			case 0:
				self->NPC_type = "jeditrainer";
				break;
			case 1:
				self->NPC_type = "jeditrainer2";
				break;
			case 2:
				self->NPC_type = "jeditrainer3";
				break;
			case 3:
				self->NPC_type = "jeditrainer4";
				break;
			default:;
			}
		}
		else
		{
			switch (Q_irand(0, 3))
			{
			case 0:
				self->NPC_type = "JediF";
				break;
			case 1:
				self->NPC_type = "Jedi";
				break;
			case 2:
				self->NPC_type = "Jedi2";
				break;
			case 3:
				self->NPC_type = "Jedi3";
				break;
			default:;
			}
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Prisoner(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Prisoner(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			if (Q_irand(0, 1))
			{
				self->NPC_type = "elder";
			}
			else
			{
				self->NPC_type = "elder2";
			}
		}
		else
		{
			if (Q_irand(0, 1))
			{
				self->NPC_type = "Prisoner";
			}
			else
			{
				self->NPC_type = "Prisoner2";
			}
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Rebel(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Rebel(gentity_t* self)
{
	if (!self->NPC_type)
	{
		switch (Q_irand(0, 3))
		{
		case 0:
			self->NPC_type = "Rebel";
			break;
		case 1:
			self->NPC_type = "Rebel2";
			break;
		case 2:
			self->NPC_type = "Rebel_Pilot";
			break;
		case 3:
			self->NPC_type = "Rebel3";
			break;
		default:;
		}
	}

	SP_NPC_spawner(self);
}

//=============================================================================================
//ENEMIES
//=============================================================================================

/*QUAKED NPC_Human_Merc(1 0 0) (-16 -16 -24) (16 16 40) BOWCASTER REPEATER FLECHETTE CONCUSSION DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
100 health, blaster rifle

BOWCASTER - Starts with a Bowcaster
REPEATER - Starts with a Repeater
FLECHETTE - Starts with a Flechette gun
CONCUSSION - Starts with a Concussion Rifle

If you want them to start with any other kind of weapon, make a spawnscript for them that sets their weapon.

"message" - turns on his key surface.  This is the name of the key you get when you walk over his body.  This must match the "message" field of the func_security_panel you want this key to open.  Set to "goodie" to have him carrying a goodie key that player can use to operate doors with "GOODIE" spawnflag.  NOTE: this overrides all the weapon spawnflags

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Human_Merc(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->message)
		{
			self->NPC_type = "human_merc_key";
		}
		else if (self->spawnflags & 1)
		{
			if (Q_irand(0, 1))
			{
				self->NPC_type = "human_merc_bow";
			}
			else
			{
				self->NPC_type = "human_merc_bow2";
			}
		}
		else if (self->spawnflags & 2)
		{
			if (Q_irand(0, 1))
			{
				self->NPC_type = "human_merc_rep";
			}
			else
			{
				self->NPC_type = "human_merc_rep2";
			}
		}
		else if (self->spawnflags & 4)
		{
			if (Q_irand(0, 1))
			{
				self->NPC_type = "human_merc_flc";
			}
			else
			{
				self->NPC_type = "human_merc_flc2";
			}
		}
		else if (self->spawnflags & 8)
		{
			self->NPC_type = "human_merc_cnc";
		}
		else
		{
			if (Q_irand(0, 1))
			{
				self->NPC_type = "human_merc";
			}
			else
			{
				self->NPC_type = "human_merc2";
			}
		}
	}
	register_item(BG_FindItemForWeapon(WP_ROCKET_LAUNCHER));
	SP_NPC_spawner(self);
}

/*QUAKED NPC_Stormtrooper(1 0 0) (-16 -16 -24) (16 16 40) OFFICER COMMANDER ALTOFFICER ROCKET DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
30 health, blaster

OFFICER - 60 health, flechette
COMMANDER - 60 health, heavy repeater
ALTOFFICER - 60 health, alt-fire flechette (grenades)
ROCKET - 60 health, rocket launcher

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Stormtrooper(gentity_t* self)
{
	if (self->spawnflags & 8)
	{
		//rocketer
		self->NPC_type = "rockettrooper";
	}
	else if (self->spawnflags & 4)
	{
		//alt-officer
		self->NPC_type = "stofficeralt";
	}
	else if (self->spawnflags & 2)
	{
		//commander
		self->NPC_type = "stcommander";
	}
	else if (self->spawnflags & 1)
	{
		//officer
		self->NPC_type = "stofficer";
	}
	else
	{
		//regular trooper
		if (Q_irand(0, 1))
		{
			self->NPC_type = "StormTrooper";
		}
		else
		{
			self->NPC_type = "StormTrooper2";
		}
	}

	SP_NPC_spawner(self);
}

void SP_NPC_StormtrooperOfficer(gentity_t* self)
{
	self->spawnflags |= 1;
	SP_NPC_Stormtrooper(self);
}

/*QUAKED NPC_Rosh_Penin (1 0 0) (-16 -16 -24) (16 16 32) DARKSIDE NOFORCE x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
Good Rosh
DARKSIDE (1) - Evil Rosh
NOFORCE (2) - Can't jump, starts with no saber
DROPTOFLOOR (16) - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC (32) - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID (64) - Starts not solid
STARTINSOLID (128) - Don't try to fix if spawn in solid
SHY (256) - Spawner is shy
*/
extern void npc_rosh_dark_precache(void);

void SP_NPC_Rosh_Penin(gentity_t* self)
{
	if (self->spawnflags & 1)
	{
		self->NPC_type = "rosh_dark";
		npc_rosh_dark_precache();
	}
	else if (self->spawnflags & 2)
	{
		self->NPC_type = "rosh_penin_noforce";
	}
	else
	{
		self->NPC_type = "rosh_penin";
	}
	SP_NPC_spawner(self);
}

/*QUAKED NPC_Saboteur(1 0 0) (-16 -16 -24) (16 16 40) SNIPER PISTOL x x CLOAKED CINEMATIC NOTSOLID STARTINSOLID SHY
Has a blaster rifle, can cloak and roll

SNIPER - Has a sniper rifle, no acrobatics, but can dodge
PISTOL - Just has a pistol, can roll
COMMANDO - Has 2 pistols and can roll & dodge

CLOAKED - Starts cloaked
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Saboteur(gentity_t* self)
{
	if (self->spawnflags & 1)
	{
		self->NPC_type = "saboteursniper";
	}
	else if (self->spawnflags & 2)
	{
		self->NPC_type = "saboteurpistol";
	}
	else if (self->spawnflags & 4)
	{
		self->NPC_type = "saboteurcommando";
	}
	else
	{
		self->NPC_type = "saboteur";
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Chewbacca
DROPTOFLOOR (16) - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC (32) - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID (64) - Starts not solid
STARTINSOLID (128) - Don't try to fix if spawn in solid
SHY (256) - Spawner is shy
*/
void SP_NPC_Chewbacca(gentity_t* self)
{
	self->NPC_type = "chewie";
	SP_NPC_spawner(self);
}

/*QUAKED NPC_Player
Creates a clone of the player

DROPTOFLOOR (16) - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC (32) - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID (64) - Starts not solid
STARTINSOLID (128) - Don't try to fix if spawn in solid
SHY (256) - Spawner is shy
*/
void SP_NPC_Player(gentity_t* self)
{
	self->NPC_type = "player";
	SP_NPC_spawner(self);
}

/*QUAKED NPC_Snowtrooper(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
30 health, blaster

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Snowtrooper(gentity_t* self)
{
	self->NPC_type = "snowtrooper";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Tie_Pilot(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
30 health, blaster

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Tie_Pilot(gentity_t* self)
{
	if (Q_irand(0, 1))
	{
		self->NPC_type = "stormpilot";
	}
	else
	{
		self->NPC_type = "stormpilot2";
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Ugnaught(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Ugnaught(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (Q_irand(0, 1))
		{
			self->NPC_type = "Ugnaught";
		}
		else
		{
			self->NPC_type = "Ugnaught2";
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Jawa(1 0 0) (-16 -16 -24) (16 16 40) ARMED x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
ARMED - starts with the Jawa gun in-hand

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Jawa(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			self->NPC_type = "jawa_armed";
		}
		else
		{
			self->NPC_type = "jawa";
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Gran(1 0 0) (-16 -16 -24) (16 16 40) SHOOTER BOXER x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
Uses grenade

SHOOTER - uses blaster instead of
BOXER - uses fists only
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Gran(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			self->NPC_type = "granshooter";
		}
		else if (self->spawnflags & 2)
		{
			self->NPC_type = "granboxer";
		}
		else
		{
			if (Q_irand(0, 1))
			{
				self->NPC_type = "gran";
			}
			else
			{
				self->NPC_type = "gran2";
			}
		}
	}

	SP_NPC_spawner(self);
}

void SP_NPC_RocketTrooper(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			self->NPC_type = "rockettrooper2Officer";
		}
		else
		{
			self->NPC_type = "rockettrooper2";
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Rodian(1 0 0) (-16 -16 -24) (16 16 40) BLASTER NO_HIDE x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
BLASTER uses a blaster instead of sniper rifle, different skin
NO_HIDE (only applicable with snipers) does not duck and hide between shots
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Rodian(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			self->NPC_type = "rodian2";
		}
		else
		{
			self->NPC_type = "rodian";
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Weequay(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Weequay(gentity_t* self)
{
	if (!self->NPC_type)
	{
		switch (Q_irand(0, 3))
		{
		case 0:
			self->NPC_type = "Weequay";
			break;
		case 1:
			self->NPC_type = "Weequay2";
			break;
		case 2:
			self->NPC_type = "Weequay3";
			break;
		case 3:
			self->NPC_type = "Weequay4";
			break;
		default:;
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Trandoshan(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Trandoshan(gentity_t* self)
{
	if (!self->NPC_type)
	{
		self->NPC_type = "Trandoshan";
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Tusken(1 0 0) (-16 -16 -24) (16 16 40) SNIPER x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Tusken(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			self->NPC_type = "tuskensniper";
		}
		else
		{
			self->NPC_type = "tusken";
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Noghri(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Noghri(gentity_t* self)
{
	if (!self->NPC_type)
	{
		self->NPC_type = "noghri";
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_SwampTrooper(1 0 0) (-16 -16 -24) (16 16 40) REPEATER x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
REPEATER - Swaptrooper who uses the repeater
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_SwampTrooper(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			self->NPC_type = "SwampTrooper2";
		}
		else
		{
			self->NPC_type = "SwampTrooper";
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Imperial(1 0 0) (-16 -16 -24) (16 16 40) OFFICER COMMANDER x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY

Greyshirt grunt, uses blaster pistol, 20 health.

OFFICER - Brownshirt Officer, uses blaster rifle, 40 health
COMMANDER - Blackshirt Commander, uses rapid-fire blaster rifle, 80 healt

"message" - if a COMMANDER, turns on his key surface.  This is the name of the key you get when you walk over his body.  This must match the "message" field of the func_security_panel you want this key to open.  Set to "goodie" to have him carrying a goodie key that player can use to operate doors with "GOODIE" spawnflag.

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Imperial(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			self->NPC_type = "ImpOfficer";
		}
		else if (self->spawnflags & 2)
		{
			self->NPC_type = "ImpCommander";
		}
		else
		{
			self->NPC_type = "Imperial";
		}
	}
	//rwwFIXMEFIXME: Allow goodie keys
	SP_NPC_spawner(self);
}

/*QUAKED NPC_ImpWorker(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_ImpWorker(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (!Q_irand(0, 2))
		{
			self->NPC_type = "ImpWorker";
		}
		else if (Q_irand(0, 1))
		{
			self->NPC_type = "ImpWorker2";
		}
		else
		{
			self->NPC_type = "ImpWorker3";
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_BespinCop(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_BespinCop(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (!Q_irand(0, 1))
		{
			self->NPC_type = "BespinCop";
		}
		else
		{
			self->NPC_type = "BespinCop2";
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_BobaFett(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/

void SP_NPC_BobaFett(gentity_t* self)
{
	self->NPC_type = "Boba_Fett";
	SP_NPC_spawner(self);
}

void SP_NPC_BobaFett2(gentity_t* self)
{
	self->NPC_type = "Boba_Fett2";
	SP_NPC_spawner(self);
}

void SP_NPC_BobaFett3(gentity_t* self)
{
	self->NPC_type = "Boba_Fett3";
	SP_NPC_spawner(self);
}

void SP_NPC_BobaFett4(gentity_t* self)
{
	self->NPC_type = "Boba_Fett4";
	SP_NPC_spawner(self);
}

/*QUAKED NPC_Reborn(1 0 0) (-16 -16 -24) (16 16 40) FORCE FENCER ACROBAT BOSS CEILING CINEMATIC NOTSOLID STARTINSOLID SHY

Default Reborn is A poor lightsaber fighter, acrobatic and uses no force powers.  Yellow saber, 40 health.

FORCE - Uses force powers but is not the best lightsaber fighter and not acrobatic.  Purple saber, 75 health.
FENCER - A good lightsaber fighter, but not acrobatic and uses no force powers.  Yellow saber, 100 health.
ACROBAT - quite acrobatic, but not the best lightsaber fighter and uses no force powers.  Red saber, 100 health.
BOSS - quite acrobatic, good lightsaber fighter and uses force powers.  Orange saber, 150 health.

NOTE: Saber colors are temporary until they have different by skins to tell them apart

CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Reborn(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			self->NPC_type = "rebornforceuser";
		}
		else if (self->spawnflags & 2)
		{
			self->NPC_type = "rebornfencer";
		}
		else if (self->spawnflags & 4)
		{
			self->NPC_type = "rebornacrobat";
		}
		else if (self->spawnflags & 8)
		{
			self->NPC_type = "rebornboss";
		}
		else
		{
			self->NPC_type = "reborn";
		}
	}

	wp_set_saber_model(NULL, CLASS_REBORN);
	SP_NPC_spawner(self);
}

/*QUAKED NPC_ShadowTrooper(1 0 0) (-16 -16 -24) (16 16 40) x x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_ShadowTrooper(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (!Q_irand(0, 1))
		{
			self->NPC_type = "ShadowTrooper";
		}
		else
		{
			self->NPC_type = "ShadowTrooper2";
		}
	}

	npc_shadow_trooper_precache();
	wp_set_saber_model(NULL, CLASS_SHADOWTROOPER);

	SP_NPC_spawner(self);
}

//=============================================================================================
//MONSTERS
//=============================================================================================

/*QUAKED NPC_Monster_Murjj (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Murjj(gentity_t* self)
{
	self->NPC_type = "Murjj";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Monster_Swamp (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Swamp(gentity_t* self)
{
	self->NPC_type = "Swamp";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Monster_Howler (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Howler(gentity_t* self)
{
	self->NPC_type = "howler";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_MineMonster (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_MineMonster(gentity_t* self)
{
	self->NPC_type = "minemonster";

	SP_NPC_spawner(self);
	NPC_MineMonster_Precache();
}

/*QUAKED NPC_Monster_Claw (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Claw(gentity_t* self)
{
	self->NPC_type = "Claw";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Monster_Glider (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Glider(gentity_t* self)
{
	self->NPC_type = "Glider";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Monster_Flier2 (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Flier2(gentity_t* self)
{
	self->NPC_type = "Flier2";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Monster_Lizard (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Lizard(gentity_t* self)
{
	self->NPC_type = "Lizard";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Monster_Fish (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Fish(gentity_t* self)
{
	self->NPC_type = "Fish";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Monster_Wampa (1 0 0) (-12 -12 -24) (12 12 40) WANDER SEARCH x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
WANDER - When I don't have an enemy, I'll just wander the waypoint network aimlessly
SEARCH - When I don't have an enemy, I'll go back and forth between the nearest waypoints, looking for enemies
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Wampa(gentity_t* self)
{
	self->NPC_type = "wampa";

	NPC_Wampa_Precache();

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Monster_Rancor (1 0 0) (-30 -30 -24) (30 30 104) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Rancor(gentity_t* self)
{
	self->NPC_type = "rancor";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Monster_Sand_Creature (1 0 0) (-24 -24 -24) (24 24 0) FAST x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

turfrange - if set, they will not go beyond this dist from their spawn position
*/
void SP_NPC_Monster_Sand_Creature(gentity_t* self) //wahoo sand creature
{
	if (self->spawnflags & 1)
	{
		self->NPC_type = "sand_creature_fast";
	}
	else
	{
		self->NPC_type = "sand_creature";
	}

	SP_NPC_spawner(self);
}

//=============================================================================================
//DROIDS
//=============================================================================================

/*QUAKED NPC_Droid_Interrogator (1 0 0) (-12 -12 -24) (12 12 0) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Droid_Interrogator(gentity_t* self)
{
	self->NPC_type = "interrogator";

	SP_NPC_spawner(self);

	NPC_Interrogator_Precache(self);
}

/*QUAKED NPC_Droid_Probe (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Imperial Probe Droid - the multilegged floating droid that Han and Chewie shot on the ice planet Hoth
*/
void SP_NPC_Droid_Probe(gentity_t* self)
{
	self->NPC_type = "probe";

	SP_NPC_spawner(self);

	NPC_Probe_Precache();
}

/*QUAKED NPC_Droid_Mark1 (1 0 0) (-36 -36 -24) (36 36 80) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Big walking droid

*/
void SP_NPC_Droid_Mark1(gentity_t* self)
{
	self->NPC_type = "mark1";

	SP_NPC_spawner(self);

	NPC_Mark1_Precache();
}

/*QUAKED NPC_Droid_Mark2 (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Small rolling droid with one gun.

*/
void SP_NPC_Droid_Mark2(gentity_t* self)
{
	self->NPC_type = "mark2";

	SP_NPC_spawner(self);

	NPC_Mark2_Precache();
}

/*QUAKED NPC_Droid_ATST (1 0 0) (-40 -40 -24) (40 40 248) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Droid_ATST(gentity_t* self)
{
	if (self->spawnflags & 1)
	{
		self->NPC_type = "atst_vehicle";
	}
	else
	{
		self->NPC_type = "atst";
	}

	SP_NPC_spawner(self);

	NPC_ATST_Precache();
}

/*QUAKED NPC_Droid_Remote (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Remote Droid - the floating round droid used by Obi Wan to train Luke about the force while on the Millenium Falcon.
*/
void SP_NPC_Droid_Remote(gentity_t* self)
{
	self->NPC_type = "remote_sp";

	SP_NPC_spawner(self);

	NPC_Remote_Precache();
}

/*QUAKED NPC_Droid_Seeker (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Seeker Droid - floating round droids that shadow troopers spawn
*/
void SP_NPC_Droid_Seeker(gentity_t* self)
{
	self->NPC_type = "seeker";

	SP_NPC_spawner(self);

	NPC_Seeker_Precache();
}

/*QUAKED NPC_Droid_Sentry (1 0 0) (-24 -24 -24) (24 24 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Sentry Droid - Large, armored floating Imperial droids with 3 forward-facing gun turrets
*/
void SP_NPC_Droid_Sentry(gentity_t* self)
{
	self->NPC_type = "sentry";

	SP_NPC_spawner(self);

	NPC_Sentry_Precache();
}

/*QUAKED NPC_Droid_Gonk (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Gonk Droid - the droid that looks like a walking ice machine. Was in the Jawa land crawler, walking around talking to itself.

NOTARGET by default
*/
void SP_NPC_Droid_Gonk(gentity_t* self)
{
	self->NPC_type = "gonk";

	SP_NPC_spawner(self);

	//precache the Gonk sounds
	NPC_Gonk_Precache();
}

/*QUAKED NPC_Droid_Mouse (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Mouse Droid - small, box shaped droid, first seen on the Death Star. Chewie yelled at it and it backed up and ran away.

NOTARGET by default
*/
void SP_NPC_Droid_Mouse(gentity_t* self)
{
	self->NPC_type = "mouse";

	SP_NPC_spawner(self);

	//precache the Mouse sounds
	NPC_Mouse_Precache();
}

/*QUAKED NPC_Droid_R2D2 (1 0 0) (-12 -12 -24) (12 12 40) IMPERIAL x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

R2D2 Droid - you probably know this one already.

NOTARGET by default
*/
void SP_NPC_Droid_R2D2(gentity_t* self)
{
	if (self->spawnflags & 1)
	{
		//imperial skin
		self->NPC_type = "r2d2_imp";
	}
	else
	{
		self->NPC_type = "r2d2";
	}

	SP_NPC_spawner(self);

	NPC_R2D2_Precache();
}

/*QUAKED NPC_Droid_R5D2 (1 0 0) (-12 -12 -24) (12 12 40) IMPERIAL ALWAYSDIE x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
ALWAYSDIE - won't go into spinning zombie AI when at low health.
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

R5D2 Droid - the droid originally chosen by Uncle Owen until it blew a bad motivator, and they took R2D2 instead.

NOTARGET by default
*/
void SP_NPC_Droid_R5D2(gentity_t* self)
{
	if (self->spawnflags & 1)
	{
		//imperial skin
		self->NPC_type = "r5d2_imp";
	}
	else
	{
		self->NPC_type = "r5d2";
	}

	SP_NPC_spawner(self);

	NPC_R5D2_Precache();
}

/*QUAKED NPC_Droid_Protocol (1 0 0) (-12 -12 -24) (12 12 40) IMPERIAL x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

NOTARGET by default
*/
void SP_NPC_Droid_Protocol(gentity_t* self)
{
	if (self->spawnflags & 1)
	{
		//imperial skin
		self->NPC_type = "protocol_imp";
	}
	else
	{
		self->NPC_type = "protocol";
	}

	SP_NPC_spawner(self);
	NPC_Protocol_Precache();
}

//NPC console commands
/*
NPC_Spawn_f
*/

gentity_t* NPC_SpawnType(const gentity_t* ent, const char* npc_type, const char* targetname, const qboolean isVehicle)
{
	gentity_t* NPCspawner = G_Spawn();
	vec3_t forward, end;
	trace_t trace;

	if (!NPCspawner)
	{
		Com_Printf(S_COLOR_RED"NPC_Spawn Error: Out of entities!\n");
		return NULL;
	}

	NPCspawner->think = G_FreeEntity;
	NPCspawner->nextthink = level.time + FRAMETIME;

	if (!npc_type)
	{
		return NULL;
	}

	if (!npc_type[0])
	{
		Com_Printf(
			S_COLOR_RED"Error, expected one of:\n"S_COLOR_WHITE
			" NPC spawn [NPC type (from ext_data/mpnpcs)]\n NPC spawn vehicle [VEH type (from ext_data/vehicles)]\n");
		return NULL;
	}

	if (!ent || !ent->client)
	{
		//screw you, go away
		return NULL;
	}

	//rwwFIXMEFIXME: Care about who is issuing this command/other clients besides 0?
	//Spawn it at spot of first player
	//FIXME: will gib them!
	AngleVectors(ent->client->ps.viewangles, forward, NULL, NULL);
	VectorNormalize(forward);
	VectorMA(ent->r.currentOrigin, 128, forward, end);
	trap->Trace(&trace, ent->r.currentOrigin, NULL, NULL, end, 0, MASK_SOLID, qfalse, 0, 0);
	VectorCopy(trace.endpos, end);
	end[2] -= 24;
	trap->Trace(&trace, trace.endpos, NULL, NULL, end, 0, MASK_SOLID, qfalse, 0, 0);
	VectorCopy(trace.endpos, end);
	end[2] += 24;
	G_SetOrigin(NPCspawner, end);
	VectorCopy(NPCspawner->r.currentOrigin, NPCspawner->s.origin);
	//set the yaw so that they face away from player
	NPCspawner->s.angles[1] = ent->client->ps.viewangles[1];

	trap->LinkEntity((sharedEntity_t*)NPCspawner);

	NPCspawner->NPC_type = G_NewString(npc_type);

	if (targetname)
	{
		NPCspawner->NPC_targetname = G_NewString(targetname);
	}

	NPCspawner->count = 1;

	NPCspawner->delay = 0;

	if (isVehicle)
	{
		NPCspawner->classname = "NPC_Vehicle";
	}

	//call precache funcs for James' builds
	if (!Q_stricmp("gonk", NPCspawner->NPC_type))
	{
		NPC_Gonk_Precache();
	}
	else if (!Q_stricmp("mouse", NPCspawner->NPC_type))
	{
		NPC_Mouse_Precache();
	}
	else if (!Q_strncmp("r2d2", NPCspawner->NPC_type, 4))
	{
		NPC_R2D2_Precache();
	}
	else if (!Q_stricmp("atst", NPCspawner->NPC_type))
	{
		NPC_ATST_Precache();
	}
	else if (!Q_strncmp("r5d2", NPCspawner->NPC_type, 4))
	{
		NPC_R5D2_Precache();
	}
	else if (!Q_stricmp("mark1", NPCspawner->NPC_type))
	{
		NPC_Mark1_Precache();
	}
	else if (!Q_stricmp("mark2", NPCspawner->NPC_type))
	{
		NPC_Mark2_Precache();
	}
	else if (!Q_stricmp("interrogator", NPCspawner->NPC_type))
	{
		NPC_Interrogator_Precache(NULL);
	}
	else if (!Q_stricmp("rosh_dark", NPCspawner->NPC_type))
	{
		npc_rosh_dark_precache();
	}
	else if (!Q_stricmp("probe", NPCspawner->NPC_type))
	{
		NPC_Probe_Precache();
	}
	else if (!Q_stricmp("seeker", NPCspawner->NPC_type))
	{
		NPC_Seeker_Precache();
	}
	else if (!Q_stricmp("remote", NPCspawner->NPC_type))
	{
		NPC_Remote_Precache();
	}
	else if (!Q_strncmp("shadowtrooper", NPCspawner->NPC_type, 13))
	{
		npc_shadow_trooper_precache();
	}
	else if (!Q_stricmp("minemonster", NPCspawner->NPC_type))
	{
		NPC_MineMonster_Precache();
	}
	else if (!Q_stricmp("howler", NPCspawner->NPC_type))
	{
		NPC_Howler_Precache();
	}
	else if (!Q_stricmp("sentry", NPCspawner->NPC_type))
	{
		NPC_Sentry_Precache();
	}
	else if (!Q_stricmp("protocol", NPCspawner->NPC_type))
	{
		NPC_Protocol_Precache();
	}
	else if (!Q_stricmp("galak_mech", NPCspawner->NPC_type))
	{
		NPC_GalakMech_Precache();
	}
	else if (!Q_stricmp("wampa", NPCspawner->NPC_type))
	{
		NPC_Wampa_Precache();
	}
	else
	{
		NPC_PrecacheByClassName(NPCspawner->NPC_type);
	}

	return NPC_Spawn_Do(NPCspawner);
}

static void NPC_Spawn_f(gentity_t* ent)
{
	char npc_type[1024];
	char targetname[1024];
	qboolean isVehicle = qfalse;

	trap->Argv(2, npc_type, 1024);
	if (Q_stricmp("vehicle", npc_type) == 0)
	{
		isVehicle = qtrue;
		trap->Argv(3, npc_type, 1024);
		trap->Argv(4, targetname, 1024);
	}
	else
	{
		trap->Argv(3, targetname, 1024);
	}

	NPC_SpawnType(ent, npc_type, targetname, isVehicle);
}

/*
NPC_Kill_f
*/
extern stringID_table_t TeamTable[];

static void NPC_Kill_f(void)
{
	int n;
	char name[1024];
	npcteam_t killTeam = NPCTEAM_FREE;
	qboolean killNonSF = qfalse;

	trap->Argv(2, name, 1024);

	if (!name[0])
	{
		Com_Printf(S_COLOR_RED"Error, Expected:\n");
		Com_Printf(S_COLOR_RED"NPC kill '[NPC targetname]' - kills NPCs with certain targetname\n");
		Com_Printf(S_COLOR_RED"or\n");
		Com_Printf(S_COLOR_RED"NPC kill 'all' - kills all NPCs\n");
		Com_Printf(S_COLOR_RED"or\n");
		Com_Printf(
			S_COLOR_RED"NPC team '[teamname]' - kills all NPCs of a certain team ('nonally' is all but your allies)\n");
		return;
	}

	if (Q_stricmp("team", name) == 0)
	{
		trap->Argv(3, name, 1024);

		if (!name[0])
		{
			Com_Printf(S_COLOR_RED"NPC_Kill Error: 'npc kill team' requires a team name!\n");
			Com_Printf(S_COLOR_RED"Valid team names are:\n");
			for (n = TEAM_FREE + 1; n < TEAM_NUM_TEAMS; n++)
			{
				Com_Printf(S_COLOR_RED"%s\n", TeamNames[n]);
			}
			Com_Printf(S_COLOR_RED"nonally - kills all but your teammates\n");
			return;
		}

		if (Q_stricmp("nonally", name) == 0)
		{
			killNonSF = qtrue;
		}
		else
		{
			killTeam = GetIDForString(TeamTable, name);

			if (killTeam == NPCTEAM_FREE)
			{
				Com_Printf(S_COLOR_RED"NPC_Kill Error: team '%s' not recognized\n", name);
				Com_Printf(S_COLOR_RED"Valid team names are:\n");
				for (n = TEAM_FREE + 1; n < TEAM_NUM_TEAMS; n++)
				{
					Com_Printf(S_COLOR_RED"%s\n", TeamNames[n]);
				}
				Com_Printf(S_COLOR_RED"nonally - kills all but your teammates\n");
				return;
			}
		}
	}

	for (n = 1; n < ENTITYNUM_MAX_NORMAL; n++)
	{
		gentity_t* player = &g_entities[n];
		if (!player->inuse)
		{
			continue;
		}
		if (killNonSF)
		{
			if (player)
			{
				if (player->client)
				{
					if (player->client->playerTeam != NPCTEAM_PLAYER)
					{
						Com_Printf(S_COLOR_GREEN"Killing NPC %s named %s\n", player->NPC_type, player->targetname);
						player->health = 0;

						if (player->die && player->client)
						{
							player->die(player, player, player, player->client->pers.maxHealth, MOD_UNKNOWN);
						}
					}
				}
				else if (player->NPC_type && player->classname && player->classname[0] && Q_stricmp(
					"NPC_starfleet", player->classname) != 0)
				{
					//A spawner, remove it
					Com_Printf(S_COLOR_GREEN"Removing NPC spawner %s with NPC named %s\n", player->NPC_type,
						player->NPC_targetname);
					G_FreeEntity(player);
				}
			}
		}
		else if (player && player->NPC && player->client)
		{
			if (killTeam != NPCTEAM_FREE)
			{
				if (player->client->playerTeam == killTeam)
				{
					Com_Printf(S_COLOR_GREEN"Killing NPC %s named %s\n", player->NPC_type, player->targetname);
					player->health = 0;
					if (player->die)
					{
						player->die(player, player, player, player->client->pers.maxHealth, MOD_UNKNOWN);
					}
				}
			}
			else if (player->targetname && Q_stricmp(name, player->targetname) == 0
				|| Q_stricmp(name, "all") == 0)
			{
				Com_Printf(S_COLOR_GREEN"Killing NPC %s named %s\n", player->NPC_type, player->targetname);
				player->health = 0;
				player->client->ps.stats[STAT_HEALTH] = 0;
				if (player->die)
				{
					player->die(player, player, player, 100, MOD_UNKNOWN);
				}
			}
		}
		else if (player && player->r.svFlags & SVF_NPC_PRECACHE)
		{
			//a spawner
			if (player->targetname && Q_stricmp(name, player->targetname) == 0
				|| Q_stricmp(name, "all") == 0)
			{
				Com_Printf(S_COLOR_GREEN"Removing NPC spawner %s named %s\n", player->NPC_type, player->targetname);
				G_FreeEntity(player);
			}
		}
	}
}

static void NPC_PrintScore(const gentity_t* ent)
{
	Com_Printf("%s: %d\n", ent->targetname, ent->client->ps.persistant[PERS_SCORE]);
}

/*
Svcmd_NPC_f

parse and dispatch bot commands
*/
qboolean showBBoxes = qfalse;

void Cmd_NPC_f(gentity_t* ent)
{
	char cmd[1024];

	trap->Argv(1, cmd, 1024);

	if (!cmd[0])
	{
		Com_Printf("Valid NPC commands are:\n");
		Com_Printf(" spawn [NPC type (from NPCs.cfg)]\n");
		Com_Printf(" kill [NPC targetname] or [all(kills all NPCs)] or 'team [teamname]'\n");
		Com_Printf(" showbounds (draws exact bounding boxes of NPCs)\n");
		Com_Printf(" score [NPC targetname] (prints number of kills per NPC)\n");
	}
	else if (Q_stricmp(cmd, "spawn") == 0)
	{
		NPC_Spawn_f(ent);
	}
	else if (Q_stricmp(cmd, "kill") == 0)
	{
		NPC_Kill_f();
	}
	else if (Q_stricmp(cmd, "showbounds") == 0)
	{
		//Toggle on and off
		showBBoxes = showBBoxes ? qfalse : qtrue;
	}
	else if (Q_stricmp(cmd, "score") == 0)
	{
		char cmd2[1024];
		const gentity_t* thisent;

		trap->Argv(2, cmd2, sizeof cmd2);

		if (!cmd2[0])
		{
			//Show the score for all NPCs

			Com_Printf("SCORE LIST:\n");
			for (int i = 0; i < ENTITYNUM_WORLD; i++)
			{
				thisent = &g_entities[i];
				if (!thisent || !thisent->client)
				{
					continue;
				}
				NPC_PrintScore(thisent);
			}
		}
		else
		{
			if ((thisent = G_Find(NULL, FOFS(targetname), cmd2)) != NULL && thisent->client)
			{
				NPC_PrintScore(thisent);
			}
			else
			{
				Com_Printf("ERROR: NPC score - no such NPC %s\n", cmd2);
			}
		}
	}
}

void Cmd_AdminNPC_f(gentity_t* ent)
{
	char cmd[1024];

	if (!(ent->r.svFlags & SVF_ADMIN))
	{
		trap->SendServerCommand(ent - g_entities, "print \"Must login with /adminlogin (password)\n\"");
		return;
	}

	trap->Argv(1, cmd, 1024);

	if (!cmd[0])
	{
		Com_Printf("Valid NPC commands are:\n");
		Com_Printf(" spawn [NPC type (from NPCs.cfg)]\n");
		Com_Printf(" kill [NPC targetname] or [all(kills all NPCs)] or 'team [teamname]'\n");
	}
	else if (Q_stricmp(cmd, "spawn") == 0)
	{
		NPC_Spawn_f(ent);
	}
	else if (Q_stricmp(cmd, "kill") == 0)
	{
		NPC_Kill_f();
	}
}

/*QUAKED NPC_Rax(1 0 0) (-16 -16 -24) (16 16 40) FUN x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
FUN - Makes him magically the funnest thing ever in any game ever made. (actually does nothing, it'll just be fun by the power of suggestion).
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Rax(gentity_t* self)
{
	self->NPC_type = "Rax";
	SP_NPC_spawner(self);
}

//The reborn boss twins
/*QUAKED NPC_Kothos(1 0 0) (-16 -16 -24) (16 16 40) VIL x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
VIL - spawns Vil Kothos instead of Dasariah (can anyone tell them apart anyway...?)

Force only... will (eventually) be set up to re-inforce their leader (use SET_LEADER) by healing them, recharging them, keeping the player away, etc.

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Kothos(gentity_t* self)
{
	if (self->spawnflags & 1)
	{
		self->NPC_type = "VKothos";
	}
	else
	{
		self->NPC_type = "DKothos";
	}
	SP_NPC_spawner(self);
}

/*QUAKED NPC_Droid_Saber (1 0 0) (-12 -12 -24) (12 12 40) TRAINING x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
TRAINING - much weaker version 50 health vs the normal 300 health

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Droid_Saber(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			self->NPC_type = "saber_droid_training";
		}
		else
		{
			self->NPC_type = "saber_droid";
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_HazardTrooper(1 0 0) (-16 -16 -24) (16 16 40) OFFICER CONCUSSION x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
250 health, repeater

OFFICER - 400 health, flechette
CONCUSSION - 400 health, concussion rifle
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_HazardTrooper(gentity_t* self)
{
	if (!self->NPC_type)
	{
		if (self->spawnflags & 1)
		{
			self->NPC_type = "hazardtrooperofficer";
		}
		else if (self->spawnflags & 2)
		{
			self->NPC_type = "hazardtrooperconcussion";
		}
		else
		{
			self->NPC_type = "hazardtrooper";
		}
	}

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Lannik_Racto(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Lannik_Racto(gentity_t* self)
{
	self->NPC_type = "lannik_racto";

	SP_NPC_spawner(self);
}

/*QUAKED NPC_Droid_Assassin (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Droid_Assassin(gentity_t* self)
{
	if (!self->NPC_type)
	{
		self->NPC_type = "assassin_droid";
	}

	SP_NPC_spawner(self);
}