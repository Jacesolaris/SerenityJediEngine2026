/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"
#include "fx_local.h"
#include "ui/ui_shared.h"
#include "ui/ui_public.h"
#include "ghoul2/G2.h"
//==========================================================================

extern qboolean WP_SaberBladeUseSecondBladeStyle(const saberInfo_t* saber, int blade_num);
extern qboolean CG_VehicleWeaponImpact(centity_t* cent);
extern qboolean CG_InFighter(void);
extern qboolean CG_InATST(void);
extern int cg_saberFlashTime;
extern vec3_t cg_saberFlashPos;
extern char* showPowersName[];
extern float ShortestLineSegBewteen2LineSegs(vec3_t start1, vec3_t end1, vec3_t start2, vec3_t end2, vec3_t close_pnt1,
	vec3_t close_pnt2);

extern int cg_siegeDeathTime;
extern int cg_siegeDeathDelay;
extern int cg_vehicleAmmoWarning;
extern int cg_vehicleAmmoWarningTime;

extern qboolean PM_InKnockDownOnly(int anim);
void CG_BounceEffect(int weapon, vec3_t origin, vec3_t normal);

//I know, not siege, but...
typedef enum
{
	TAUNT_TAUNT = 0,
	TAUNT_BOW,
	TAUNT_MEDITATE,
	TAUNT_FLOURISH,
	TAUNT_GLOAT,
	TAUNT_SURRENDER,
	TAUNT_RELOAD
} tauntTypes_t;

/*
===================
CG_PlaceString

Also called by scoreboard drawing
===================
*/
const char* CG_PlaceString(int rank)
{
	static char str[64];
	char* s, * t;
	// number extenstions, eg 1st, 2nd, 3rd, 4th etc.
	// note that the rules are different for french, but by changing the required strip strings they seem to work
	char s_st[10];
	char s_nd[10];
	char s_rd[10];
	char s_th[10];
	char s_tied_for[64]; // german is much longer, super safe...

	trap->SE_GetStringTextString("MP_INGAME_NUMBER_ST", s_st, sizeof s_st);
	trap->SE_GetStringTextString("MP_INGAME_NUMBER_ND", s_nd, sizeof s_nd);
	trap->SE_GetStringTextString("MP_INGAME_NUMBER_RD", s_rd, sizeof s_rd);
	trap->SE_GetStringTextString("MP_INGAME_NUMBER_TH", s_th, sizeof s_th);
	trap->SE_GetStringTextString("MP_INGAME_TIED_FOR", s_tied_for, sizeof s_tied_for);
	strcat(s_tied_for, " "); // save worrying about translators adding spaces or not

	if (rank & RANK_TIED_FLAG)
	{
		rank &= ~RANK_TIED_FLAG;
		t = s_tied_for; //"Tied for ";
	}
	else
	{
		t = "";
	}

	if (rank == 1)
	{
		s = va("1%s", s_st); //S_COLOR_BLUE "1st" S_COLOR_WHITE;		// draw in blue
	}
	else if (rank == 2)
	{
		s = va("2%s", s_nd); //S_COLOR_RED "2nd" S_COLOR_WHITE;		// draw in red
	}
	else if (rank == 3)
	{
		s = va("3%s", s_rd); //S_COLOR_YELLOW "3rd" S_COLOR_WHITE;		// draw in yellow
	}
	else if (rank == 11)
	{
		s = va("11%s", s_th);
	}
	else if (rank == 12)
	{
		s = va("12%s", s_th);
	}
	else if (rank == 13)
	{
		s = va("13%s", s_th);
	}
	else if (rank % 10 == 1)
	{
		s = va("%i%s", rank, s_st);
	}
	else if (rank % 10 == 2)
	{
		s = va("%i%s", rank, s_nd);
	}
	else if (rank % 10 == 3)
	{
		s = va("%i%s", rank, s_rd);
	}
	else
	{
		s = va("%i%s", rank, s_th);
	}

	Com_sprintf(str, sizeof str, "%s%s", t, s);
	return str;
}

qboolean CG_ThereIsAMaster(void);
stringID_table_t NPCClasses[] =
{
	"None?", CLASS_NONE, //This probably shouldn't happen.
	"AT-ST", CLASS_ATST,
	"Bartender", CLASS_BARTENDER,
	"Bespin Cop", CLASS_BESPIN_COP,
	"Claw", CLASS_CLAW,
	"Commando", CLASS_COMMANDO,
	"Desann", CLASS_DESANN,
	"Fish", CLASS_FISH,
	"Fliers", CLASS_FLIER2,
	"Galak", CLASS_GALAK,
	"Glider", CLASS_GLIDER,
	"Gonk Droid", CLASS_GONK,
	"Gran", CLASS_GRAN,
	"Howler", CLASS_HOWLER,
	"Imperial Officer", CLASS_IMPERIAL,
	"Imperial Worker", CLASS_IMPWORKER,
	"Interrogator Droid", CLASS_INTERROGATOR, // droid
	"Jan", CLASS_JAN,
	"Jedi", CLASS_JEDI,
	"Kyle Katarn", CLASS_KYLE,
	"Lando Calrissian", CLASS_LANDO,
	"Lizard", CLASS_LIZARD,
	"Luke Skywalker", CLASS_LUKE,
	"Mark 1 Droid", CLASS_MARK1, // droid
	"Mark 2 Droid", CLASS_MARK2, // droid
	"Galak's Mech Suit", CLASS_GALAKMECH, // droid
	"Mine Monster", CLASS_MINEMONSTER,
	"Mon Mothma", CLASS_MONMOTHA,
	"Morgan Katarn", CLASS_MORGANKATARN,
	"Mouse Droid", CLASS_MOUSE, // droid
	"Murjj", CLASS_MURJJ,
	"Prisoner", CLASS_PRISONER,
	"Probe Droid", CLASS_PROBE, // droid
	"Protocol Droid", CLASS_PROTOCOL, // droid
	"R2D2", CLASS_R2D2, // droid
	"R5D2", CLASS_R5D2, // droid
	"Rebel Soldier", CLASS_REBEL,
	"Reborn", CLASS_REBORN,
	"Reelo Baruk", CLASS_REELO,
	"Droid Remote", CLASS_REMOTE,
	"Rodian", CLASS_RODIAN,
	"Seeker Droid", CLASS_SEEKER, // droid
	"Sentry", CLASS_SENTRY,
	"Shadowtrooper", CLASS_SHADOWTROOPER,
	"Stormtrooper", CLASS_STORMTROOPER,
	"Swamp?", CLASS_SWAMP,
	"Swamptrooper", CLASS_SWAMPTROOPER,
	"Tavion", CLASS_TAVION,
	"Trandoshan", CLASS_TRANDOSHAN,
	"Ugnaught", CLASS_UGNAUGHT,
	"Jawa", CLASS_JAWA,
	"Weequay", CLASS_WEEQUAY,
	"Boba Fett", CLASS_BOBAFETT,
	"Vehicle", CLASS_VEHICLE,
	"Fluffy the Rancor", CLASS_RANCOR,
	"Frosty the Wampa", CLASS_WAMPA,
	"Saboteur", CLASS_SABOTEUR,
	"Sand Creature", CLASS_SAND_CREATURE,
	"Alora", CLASS_ALORA,
	"Rockettrooper", CLASS_ROCKETTROOPER,
	"Saber Droid", CLASS_SABER_DROID,
	"Assassin Droid", CLASS_ASSASSIN_DROID,
	"Hazard Trooper", CLASS_HAZARD_TROOPER,
	"Player's Standin", CLASS_PLAYER,
	"Tusken Raider", CLASS_TUSKEN,
	"", -1, //CLASS_NUM_CLASSES
};

/*
=============
CG_Obituary
=============
*/
static void CG_Obituary(entityState_t* ent)
{
	int mod;
	int target, attacker;
	char* message;
	const char* target_info;
	const char* attacker_info;
	char target_name[32];
	char attacker_name[32];
	char target_veh_name[32] = { 0 };
	char attacker_veh_name[32] = { 0 };
	char attacker_veh_weap_name[32] = { 0 };
	gender_t gender;
	clientInfo_t* ci;
	qboolean veh_message = qfalse;

	target = ent->otherentity_num;
	attacker = ent->otherentity_num2;
	mod = ent->eventParm;

	if (target < 0 || target >= MAX_CLIENTS)
	{
		trap->Error(ERR_DROP, "CG_Obituary: target out of range");
	}
	ci = &cgs.clientinfo[target];

	if (attacker < 0 || attacker >= MAX_CLIENTS)
	{
		attacker_info = NULL;
	}
	else
	{
		attacker_info = CG_ConfigString(CS_PLAYERS + attacker);
	}

	target_info = CG_ConfigString(CS_PLAYERS + target);
	if (!target_info)
	{
		return;
	}
	Q_strncpyz(target_name, Info_ValueForKey(target_info, "n"), sizeof target_name - 2);
	strcat(target_name, S_COLOR_WHITE);

	if (ent->lookTarget > VEHICLE_BASE && ent->lookTarget < MAX_VEHICLES && g_vehicleInfo[ent->lookTarget].name)
	{
		Q_strncpyz(target_veh_name, g_vehicleInfo[ent->lookTarget].name, sizeof target_veh_name - 2);
	}

	// check for attacker in a vehicle
	if (ent->brokenLimbs >= MAX_CLIENTS)
	{
		centity_t* att_veh_cent = &cg_entities[ent->brokenLimbs];
		if (att_veh_cent && att_veh_cent->m_pVehicle && att_veh_cent->m_pVehicle->m_pVehicleInfo)
		{
			if (att_veh_cent->m_pVehicle->m_pVehicleInfo->name)
			{
				Q_strncpyz(attacker_veh_name, att_veh_cent->m_pVehicle->m_pVehicleInfo->name, sizeof attacker_veh_name - 2);
			}
		}
	}

	//check for specific vehicle weapon
	if (ent->weapon > 0)
	{
		if (g_vehWeaponInfo[ent->weapon - 1].name)
		{
			Q_strncpyz(attacker_veh_weap_name, g_vehWeaponInfo[ent->weapon - 1].name, sizeof attacker_veh_weap_name - 2);
		}
	}

	// check for single client messages
	if (ent->saberInFlight)
	{
		//asteroid->vehicle collision
		switch (Q_irand(0, 2))
		{
		default:
		case 0:
			message = "DIED_ASTEROID1";
			break;
		case 1:
			message = "DIED_ASTEROID2";
			break;
		case 2:
			message = "DIED_ASTEROID3";
			break;
		}
		veh_message = qtrue;
	}
	else
	{
		switch (mod)
		{
		case MOD_VEHICLE:
		case MOD_SUICIDE:
		case MOD_FALLING:
		case MOD_COLLISION:
		case MOD_VEH_EXPLOSION:
		case MOD_CRUSH:
		case MOD_WATER:
		case MOD_SLIME:
		case MOD_LAVA:
		case MOD_BURNING:
		case MOD_TRIGGER_HURT:
		case MOD_GAS:
			message = "DIED_GENERIC";
			break;
		case MOD_TARGET_LASER:
			veh_message = qtrue;
			message = "DIED_TURBOLASER";
			break;
		default:
			message = NULL;
			break;
		}
	}

	// Attacker killed themselves.  Ridicule them for it.
	if (attacker == target)
	{
		veh_message = qfalse;
		gender = ci->gender;
		switch (mod)
		{
		case MOD_BRYAR_PISTOL:
		case MOD_BRYAR_PISTOL_ALT:
		case MOD_BLASTER:
		case MOD_TURBLAST:
		case MOD_DISRUPTOR:
		case MOD_DISRUPTOR_SPLASH:
		case MOD_DISRUPTOR_SNIPER:
		case MOD_BOWCASTER:
		case MOD_REPEATER:
		case MOD_REPEATER_ALT:
		case MOD_FLECHETTE:
			if (gender == GENDER_FEMALE)
				message = "SUICIDE_SHOT_FEMALE";
			else if (gender == GENDER_NEUTER)
				message = "SUICIDE_SHOT_GENDERLESS";
			else
				message = "SUICIDE_SHOT_MALE";
			break;
		case MOD_REPEATER_ALT_SPLASH:
		case MOD_FLECHETTE_ALT_SPLASH:
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
		case MOD_ROCKET_HOMING:
		case MOD_ROCKET_HOMING_SPLASH:
		case MOD_THERMAL:
		case MOD_THERMAL_SPLASH:
		case MOD_TRIP_MINE_SPLASH:
		case MOD_TIMED_MINE_SPLASH:
		case MOD_DET_PACK_SPLASH:
		case MOD_VEHICLE:
		case MOD_CONC:
		case MOD_CONC_ALT:
			if (gender == GENDER_FEMALE)
				message = "SUICIDE_EXPLOSIVES_FEMALE";
			else if (gender == GENDER_NEUTER)
				message = "SUICIDE_EXPLOSIVES_GENDERLESS";
			else
				message = "SUICIDE_EXPLOSIVES_MALE";
			break;
		case MOD_DEMP2:
			if (gender == GENDER_FEMALE)
				message = "SUICIDE_ELECTROCUTED_FEMALE";
			else if (gender == GENDER_NEUTER)
				message = "SUICIDE_ELECTROCUTED_GENDERLESS";
			else
				message = "SUICIDE_ELECTROCUTED_MALE";
			break;
		case MOD_FALLING:
			if (gender == GENDER_FEMALE)
				message = "SUICIDE_FALLDEATH_FEMALE";
			else if (gender == GENDER_NEUTER)
				message = "SUICIDE_FALLDEATH_GENDERLESS";
			else
				message = "SUICIDE_FALLDEATH_MALE";
			break;
		default:
			if (gender == GENDER_FEMALE)
				message = "SUICIDE_GENERICDEATH_FEMALE";
			else if (gender == GENDER_NEUTER)
				message = "SUICIDE_GENERICDEATH_GENDERLESS";
			else
				message = "SUICIDE_GENERICDEATH_MALE";
			break;
		}
	}

	if (target != attacker && target < MAX_CLIENTS && attacker < MAX_CLIENTS)
	{
		goto clientkilled;
	}

	if (message)
	{
		gender = ci->gender;

		if (!message[0])
		{
			veh_message = qfalse;

			if (gender == GENDER_FEMALE)
				message = "SUICIDE_GENERICDEATH_FEMALE";
			else if (gender == GENDER_NEUTER)
				message = "SUICIDE_GENERICDEATH_GENDERLESS";
			else
				message = "SUICIDE_GENERICDEATH_MALE";
		}
		if (veh_message)
		{
			message = (char*)CG_GetStringEdString("OJP_INGAMEVEH", message);
		}
		else
		{
			message = (char*)CG_GetStringEdString("MP_INGAME", message);
		}

		trap->Print("%s %s\n", target_name, message);
		return;
	}

clientkilled:

	// check for kill messages from the current clientNum
	if (attacker == cg.snap->ps.clientNum)
	{
		char* s;

		if (cgs.gametype < GT_TEAM && cgs.gametype != GT_DUEL && cgs.gametype != GT_POWERDUEL)
		{
			if (cgs.gametype == GT_JEDIMASTER &&
				attacker < MAX_CLIENTS &&
				!ent->isJediMaster &&
				!cg.snap->ps.isJediMaster &&
				CG_ThereIsAMaster())
			{
				char part1[512];
				char part2[512];
				trap->SE_GetStringTextString("MP_INGAME_KILLED_MESSAGE", part1, sizeof part1);
				trap->SE_GetStringTextString("MP_INGAME_JMKILLED_NOTJM", part2, sizeof part2);
				s = va("%s %s\n%s\n", part1, target_name, part2);
			}
			else if (cgs.gametype == GT_JEDIMASTER &&
				attacker < MAX_CLIENTS &&
				!ent->isJediMaster &&
				!cg.snap->ps.isJediMaster)
			{
				//no JM, saber must be out
				char part1[512];
				trap->SE_GetStringTextString("MP_INGAME_KILLED_MESSAGE", part1, sizeof part1);
				s = va("%s %s\n", part1, target_name);
			}
			else if (cgs.gametype == GT_POWERDUEL)
			{
				s = "";
			}
			else
			{
				char s_place_with[256];
				char s_killed_str[256];
				trap->SE_GetStringTextString("MP_INGAME_PLACE_WITH", s_place_with, sizeof s_place_with);
				trap->SE_GetStringTextString("MP_INGAME_KILLED_MESSAGE", s_killed_str, sizeof s_killed_str);

				s = va("%s %s.\n%s %s %i.", s_killed_str, target_name,
					CG_PlaceString(cg.snap->ps.persistant[PERS_RANK] + 1),
					s_place_with,
					cg.snap->ps.persistant[PERS_SCORE]);
			}
		}
		else
		{
			char s_killed_str[256];
			trap->SE_GetStringTextString("MP_INGAME_KILLED_MESSAGE", s_killed_str, sizeof s_killed_str);
			s = va("%s %s", s_killed_str, target_name);
		}
		CG_CenterPrint(s, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
	}

	// check for double client messages
	if (!attacker_info)
	{
		//strcpy( attackerName, "noname" );
		strcpy(attacker_name, GetStringForID(NPCClasses, cg_entities[attacker].currentState.NPC_class));
	}
	else
	{
		Q_strncpyz(attacker_name, Info_ValueForKey(attacker_info, "n"), sizeof attacker_name - 2);
		strcat(attacker_name, S_COLOR_WHITE);
		// check for kill messages about the current clientNum
		if (target == cg.snap->ps.clientNum)
		{
			Q_strncpyz(cg.killerName, attacker_name, sizeof cg.killerName);
		}
	}

	if (attacker != ENTITYNUM_WORLD)
	{
		switch (mod)
		{
		case MOD_STUN_BATON:
			message = "KILLED_STUN";
			break;
		case MOD_MELEE:
			message = "KILLED_MELEE";
			break;
		case MOD_SABER:
			message = "KILLED_SABER";
			break;
		case MOD_BRYAR_PISTOL:
		case MOD_BRYAR_PISTOL_ALT:
			message = "KILLED_BRYAR";
			break;
		case MOD_BLASTER:
			message = "KILLED_BLASTER";
			break;
		case MOD_TURBLAST:
			message = "KILLED_BLASTER";
			break;
		case MOD_DISRUPTOR:
		case MOD_DISRUPTOR_SPLASH:
			message = "KILLED_DISRUPTOR";
			break;
		case MOD_DISRUPTOR_SNIPER:
			message = "KILLED_DISRUPTORSNIPE";
			break;
		case MOD_BOWCASTER:
			message = "KILLED_BOWCASTER";
			break;
		case MOD_REPEATER:
			message = "KILLED_REPEATER";
			break;
		case MOD_REPEATER_ALT:
		case MOD_REPEATER_ALT_SPLASH:
			message = "KILLED_REPEATERALT";
			break;
		case MOD_DEMP2:
		case MOD_DEMP2_ALT:
			message = "KILLED_DEMP2";
			break;
		case MOD_FLECHETTE:
			message = "KILLED_FLECHETTE";
			break;
		case MOD_FLECHETTE_ALT_SPLASH:
			message = "KILLED_FLECHETTE_MINE";
			break;
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
			message = "KILLED_ROCKET";
			break;
		case MOD_ROCKET_HOMING:
		case MOD_ROCKET_HOMING_SPLASH:
			message = "KILLED_ROCKET_HOMING";
			break;
		case MOD_THERMAL:
		case MOD_THERMAL_SPLASH:
			message = "KILLED_THERMAL";
			break;
		case MOD_TRIP_MINE_SPLASH:
			message = "KILLED_TRIPMINE";
			break;
		case MOD_TIMED_MINE_SPLASH:
			message = "KILLED_TRIPMINE_TIMED";
			break;
		case MOD_DET_PACK_SPLASH:
			message = "KILLED_DETPACK";
			break;
		case MOD_VEHICLE:
			veh_message = qtrue;
			switch (ent->generic1)
			{
			case WP_BLASTER: //primary blasters
				switch (Q_irand(0, 2))
				{
				case 2:
					message = "KILLED_VEH_BLASTER3";
					break;
				case 1:
					message = "KILLED_VEH_BLASTER2";
					break;
				default:
					message = "KILLED_VEH_BLASTER1";
					break;
				}
				break;
			case WP_ROCKET_LAUNCHER: //missile
				if (Q_irand(0, 1))
				{
					message = "KILLED_VEH_MISSILE2";
				}
				else
				{
					message = "KILLED_VEH_MISSILE1";
				}
				break;
			case WP_THERMAL: //bomb
				message = "KILLED_VEH_BOMB";
				break;
			case WP_DEMP2: //ion cannon
				message = "KILLED_VEH_ION";
				break;
			case WP_TURRET: //turret
				message = "KILLED_VEH_TURRET";
				break;
			default:
				veh_message = qfalse;
				message = "KILLED_GENERIC";
				break;
			}
			break;
		case MOD_CONC:
		case MOD_CONC_ALT:
			message = "KILLED_GENERIC";
			break;
		case MOD_FORCE_DARK:
			message = "KILLED_DARKFORCE";
			break;
		case MOD_SENTRY:
			message = "KILLED_SENTRY";
			break;
		case MOD_TELEFRAG:
			message = "KILLED_TELEFRAG";
			break;
		case MOD_CRUSH:
			message = "KILLED_GENERIC"; //"KILLED_FORCETOSS";
			break;
		case MOD_FALLING:
			message = "KILLED_FORCETOSS";
			break;
		case MOD_COLLISION:
		case MOD_VEH_EXPLOSION:
			switch (Q_irand(0, 2))
			{
			default:
			case 0:
				message = "KILLED_VEH_COLLISION1";
				break;
			case 1:
				message = "KILLED_VEH_COLLISION2";
				break;
			case 2:
				message = "KILLED_VEH_COLLISION3";
				break;
			}
			veh_message = qtrue;
			break;
		case MOD_TRIGGER_HURT:
			message = "KILLED_GENERIC"; //"KILLED_FORCETOSS";
			break;
		case MOD_TARGET_LASER:
			if (Q_irand(0, 1))
			{
				message = "KILLED_TURRET1";
			}
			else
			{
				message = "KILLED_TURRET2";
			}
			veh_message = qtrue;
			break;
		case MOD_SEEKER:
			message = "KILLED_SEEKER";
			veh_message = qtrue;
			break;
		case MOD_FORCE_LIGHTNING:
			message = "KILLED_DARKFORCE";
			break;
		default:
			message = "KILLED_GENERIC";
			break;
		}

		if (message)
		{
			if (veh_message)
			{
				message = (char*)CG_GetStringEdString("OJP_INGAMEVEH", message);
			}
			else
			{
				message = (char*)CG_GetStringEdString("MP_INGAME", message);
			}

			trap->Print("%s %s %s\n", target_name, message, attacker_name);
			if (target_veh_name[0])
			{
				trap->Print("%s %s %s\n", target_veh_name, message, attacker_name);
			}
			if (mod == MOD_TARGET_LASER)
			{
				//no attacker name, just a turbolaser or other kind of turret...
				//CG_Printf("%s", message);
			}
			else
			{
				trap->Print("%s %s %s\n", target_name, message, attacker_name);

				if (attacker_veh_name[0]
					&& attacker_veh_weap_name[0])
				{
					trap->Print("%s %s %s\n", attacker_veh_name, message, attacker_veh_weap_name);
				}
				else
				{
					if (attacker_veh_name[0])
					{
						trap->Print("%s %s %s\n", target_name, message, attacker_veh_name);
					}
					else if (attacker_veh_weap_name[0])
					{
						trap->Print("%s %s %s\n", target_name, message, attacker_veh_weap_name);
					}
				}
			}
			return;
		}
	}

	// we don't know what it was
	trap->Print("%s %s\n", target_name, (char*)CG_GetStringEdString("MP_INGAME", "DIED_GENERIC"));
}

//==========================================================================

static void CG_ToggleBinoculars(const centity_t* cent, const int force_zoom)
{
	if (cent->currentState.number != cg.snap->ps.clientNum)
	{
		return;
	}

	if (cg.snap->ps.weaponstate != WEAPON_READY)
	{
		//So we can't fool it and reactivate while switching to the saber or something.
		return;
	}

	if (force_zoom)
	{
		if (force_zoom == 2)
		{
			cg.snap->ps.zoomMode = 0;
		}
		else if (force_zoom == 1)
		{
			cg.snap->ps.zoomMode = 2;
		}
	}

	if (cg.snap->ps.zoomMode == 0)
	{
		trap->S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.zoomStart);
	}
	else if (cg.snap->ps.zoomMode == 2)
	{
		trap->S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.zoomEnd);
	}
}

//set the local timing bar
extern int cg_genericTimerBar;
extern int cg_genericTimerDur;
extern vec4_t cg_genericTimerColor;

static void CG_LocalTimingBar(const int start_time, const int duration)
{
	cg_genericTimerBar = start_time + duration;
	cg_genericTimerDur = duration;

	cg_genericTimerColor[0] = 1.0f;
	cg_genericTimerColor[1] = 1.0f;
	cg_genericTimerColor[2] = 0.0f;
	cg_genericTimerColor[3] = 1.0f;
}

/*
===============
CG_UseItem
===============
*/
static void CG_UseItem(const centity_t* cent)
{
	clientInfo_t* ci;
	int clientNum;

	const entityState_t* es = &cent->currentState;

	int itemNum = (es->event & ~EV_EVENT_BITS) - EV_USE_ITEM0;
	if (itemNum < 0 || itemNum > HI_NUM_HOLDABLE)
	{
		itemNum = 0;
	}

	switch (itemNum)
	{
	default:
	case HI_NONE:
		break;

	case HI_SEEKER:
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.deploySeeker);
		break;

	case HI_SHIELD:
		break;

	case HI_MEDPAC:
		clientNum = cent->currentState.clientNum;
		if (clientNum >= 0 && clientNum < MAX_CLIENTS)
		{
			ci = &cgs.clientinfo[clientNum];
			ci->medkitUsageTime = cg.time;
		}
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.medkitSound2);
		break;
	case HI_MEDPAC_BIG:
		clientNum = cent->currentState.clientNum;
		if (clientNum >= 0 && clientNum < MAX_CLIENTS)
		{
			ci = &cgs.clientinfo[clientNum];
			ci->medkitUsageTime = cg.time;
		}
		//Different sound for big bacta?
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.medkitSound);
		break;

	case HI_BINOCULARS:
		CG_ToggleBinoculars(cent, es->eventParm);
		break;
	case HI_SENTRY_GUN:
		break;
	case HI_JETPACK://done dont show
		break; //Do something?

	case HI_HEALTHDISP:
		break;
	case HI_AMMODISP:
		break;
	case HI_EWEB:
		break;
	case HI_CLOAK:
		break; //Do something?

	case HI_FLAMETHROWER:
		break;
	case HI_SWOOP:
		break;
	case HI_DROIDEKA:
		break;
	case HI_SPHERESHIELD:
		break; //Do something?
	case HI_GRAPPLE:   //done dont show
		break; //Do something?
	}
	// These items never run out
	if (cg.snap && cg.snap->ps.clientNum == cent->currentState.number &&
		//itemNum != HI_SEEKER &&
		//itemNum != HI_SHIELD &&
		//itemNum != HI_MEDPAC &&
		//itemNum != HI_MEDPAC_BIG &&
		itemNum != HI_BINOCULARS &&
		//itemNum != HI_SENTRY_GUN &&
		itemNum != HI_JETPACK &&        //done dont show
		itemNum != HI_HEALTHDISP &&
		itemNum != HI_AMMODISP &&
		itemNum != HI_EWEB &&
		itemNum != HI_CLOAK &&
		itemNum != HI_FLAMETHROWER &&
		//itemNum != HI_SWOOP &&
		//itemNum != HI_DROIDEKA &&
		itemNum != HI_SPHERESHIELD &&       //done dont show
		itemNum != HI_GRAPPLE)
	{
		//if not using binoculars/jetpack/dispensers/cloak, we just used that item up, so switch
		BG_CycleInven(&cg.snap->ps, 1);
		cg.itemSelect = -1; //update the client-side selection display
	}
}

/*
================
CG_ItemPickup

A new item was picked up this frame
================
*/
static void CG_ItemPickup(const int itemNum)
{
	cg.itemPickup = itemNum;
	cg.itemPickupTime = cg.time;
	cg.itemPickupBlendTime = cg.time;
	// see if it should be the grabbed weapon
	if (cg.snap && bg_itemlist[itemNum].giType == IT_WEAPON)
	{
		// 0 == no switching
		// 1 == automatically switch to best SAFE weapon
		// 2 == automatically switch to best weapon, safe or otherwise
		// 3 == if not saber, automatically switch to best weapon, safe or otherwise

		if (0 == cg_autoSwitch.integer)
		{
			// don't switch
		}
		else if (cg_autoSwitch.integer == 1)
		{
			//only autoselect if not explosive ("safe")
			if (bg_itemlist[itemNum].giTag != WP_TRIP_MINE &&
				bg_itemlist[itemNum].giTag != WP_DET_PACK &&
				bg_itemlist[itemNum].giTag != WP_THERMAL &&
				bg_itemlist[itemNum].giTag != WP_ROCKET_LAUNCHER &&
				bg_itemlist[itemNum].giTag > cg.snap->ps.weapon &&
				cg.snap->ps.weapon != WP_SABER)
			{
				if (!cg.snap->ps.emplacedIndex)
				{
					cg.weaponSelectTime = cg.time;
				}
				cg.weaponSelect = bg_itemlist[itemNum].giTag;
			}
		}
		else if (cg_autoSwitch.integer == 2)
		{
			//autoselect if better
			if (bg_itemlist[itemNum].giTag > cg.snap->ps.weapon &&
				cg.snap->ps.weapon != WP_SABER)
			{
				if (!cg.snap->ps.emplacedIndex)
				{
					cg.weaponSelectTime = cg.time;
				}
				cg.weaponSelect = bg_itemlist[itemNum].giTag;
			}
		}
	}

	//rww - print pickup messages
	//if (bg_itemlist[itemNum].classname && bg_itemlist[itemNum].classname[0] &&
	//	(bg_itemlist[itemNum].giType != IT_TEAM || (bg_itemlist[itemNum].giTag != PW_REDFLAG && bg_itemlist[itemNum].giTag != PW_BLUEFLAG)))
	//{ //don't print messages for flags, they have their own pickup event broadcasts
	//	char	text[1024];
	//	char	upperKey[1024];

	//	strcpy(upperKey, bg_itemlist[itemNum].classname);

	//	if (trap->SE_GetStringTextString(va("SP_INGAME_%s", Q_strupr(upperKey)), text, sizeof(text)))
	//	{
	//		Com_Printf("%s %s\n", CG_GetStringEdString("MP_INGAME", "PICKUPLINE"), text);
	//	}
	//	else
	//	{
	//		Com_Printf("%s %s\n", CG_GetStringEdString("MP_INGAME", "PICKUPLINE"), bg_itemlist[itemNum].classname);
	//	}
	//}
}

/*
================
CG_PainEvent

Also called by playerstate transition
================
*/
void CG_PainEvent(centity_t* cent, const int health)
{
	char* snd;

	// don't do more than two pain sounds a second
	if (cg.time - cent->pe.painTime < 1500)
	{
		return;
	}

	if (health < 25)
	{
		snd = "*pain25.wav";
	}
	else if (health < 50)
	{
		snd = "*pain50.wav";
	}
	else if (health < 75)
	{
		snd = "*pain75.wav";
	}
	else
	{
		snd = "*pain100.wav";
	}
	trap->S_StartSound(NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound(cent->currentState.number, snd));

	// save pain time for programitic twitch animation
	cent->pe.painTime = cg.time;
	cent->pe.painDirection ^= 1;
}

extern qboolean BG_GetRootSurfNameWithVariant(void* ghoul2, const char* rootSurfName, char* return_surf_name,
	int return_size);

void CG_ReattachLimb(centity_t* source)
{
	const clientInfo_t* ci;

	if (source->currentState.number >= MAX_CLIENTS)
	{
		ci = source->npcClient;
	}
	else
	{
		ci = &cgs.clientinfo[source->currentState.number];
	}
	if (ci)
	{
		//re-apply the skin
		if (ci->torsoSkin > 0)
		{
			trap->G2API_SetSkin(source->ghoul2, 0, ci->torsoSkin, ci->torsoSkin);
		}
	}

	/*
	char *limbName;
	char *stubCapName;
	int i = G2_MODELPART_HEAD;

	//rww NOTE: Assumes G2_MODELPART_HEAD is first and G2_MODELPART_RLEG is last
	while (i <= G2_MODELPART_RLEG)
	{
		if (source->torsoBolt & (1 << (i-10)))
		{
			switch (i)
			{
			case G2_MODELPART_HEAD:
				limbName = "head";
				stubCapName = "torso_cap_head";
				break;
			case G2_MODELPART_WAIST:
				limbName = "torso";
				stubCapName = "hips_cap_torso";
				break;
			case G2_MODELPART_LARM:
				limbName = "l_arm";
				stubCapName = "torso_cap_l_arm";
				break;
			case G2_MODELPART_RARM:
				limbName = "r_arm";
				stubCapName = "torso_cap_r_arm";
				break;
			case G2_MODELPART_RHAND:
				limbName = "r_hand";
				stubCapName = "r_arm_cap_r_hand";
				break;
			case G2_MODELPART_LLEG:
				limbName = "l_leg";
				stubCapName = "hips_cap_l_leg";
				break;
			case G2_MODELPART_RLEG:
				limbName = "r_leg";
				stubCapName = "hips_cap_r_leg";
				break;
			default:
				source->torsoBolt = 0;
				source->ghoul2weapon = NULL;
				return;
			}

			trap->G2API_SetSurfaceOnOff(source->ghoul2, limbName, 0);
			trap->G2API_SetSurfaceOnOff(source->ghoul2, stubCapName, 0x00000100);
		}
		i++;
	}
	*/
	source->torsoBolt = 0;

	source->ghoul2weapon = NULL;
}

static const char* CG_TeamName(const int team)
{
	if (team == TEAM_RED)
		return "RED";
	if (team == TEAM_BLUE)
		return "BLUE";
	if (team == TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
}

static void CG_PrintCTFMessage(clientInfo_t* ci, const char* team_name, const int ctf_message)
{
	char printMsg[1024];
	char* ref_name;

	switch (ctf_message)
	{
	case CTFMESSAGE_FRAGGED_FLAG_CARRIER:
		ref_name = "FRAGGED_FLAG_CARRIER";
		break;
	case CTFMESSAGE_FLAG_RETURNED:
		ref_name = "FLAG_RETURNED";
		break;
	case CTFMESSAGE_PLAYER_RETURNED_FLAG:
		ref_name = "PLAYER_RETURNED_FLAG";
		break;
	case CTFMESSAGE_PLAYER_CAPTURED_FLAG:
		ref_name = "PLAYER_CAPTURED_FLAG";
		break;
	case CTFMESSAGE_PLAYER_GOT_FLAG:
		ref_name = "PLAYER_GOT_FLAG";
		break;
	default:
		return;
	}

	const char* ps_string_ed_string = CG_GetStringEdString("MP_INGAME", ref_name);

	if (!ps_string_ed_string || !ps_string_ed_string[0])
	{
		return;
	}

	if (team_name && team_name[0])
	{
		const char* f = strstr(ps_string_ed_string, "%s");

		if (f)
		{
			int str_len = 0;
			int i = 0;

			if (ci)
			{
				Com_sprintf(printMsg, sizeof printMsg, "%s^7 ", ci->name);
				str_len = strlen(printMsg);
			}

			while (ps_string_ed_string[i] && i < 512)
			{
				if (ps_string_ed_string[i] == '%' &&
					ps_string_ed_string[i + 1] == 's')
				{
					printMsg[str_len] = '\0';
					Q_strcat(printMsg, sizeof printMsg, team_name);
					str_len = strlen(printMsg);

					i++;
				}
				else
				{
					printMsg[str_len] = ps_string_ed_string[i];
					str_len++;
				}

				i++;
			}

			printMsg[str_len] = '\0';

			goto doPrint;
		}
	}

	if (ci)
	{
		Com_sprintf(printMsg, sizeof printMsg, "%s^7 %s", ci->name, ps_string_ed_string);
	}
	else
	{
		Com_sprintf(printMsg, sizeof printMsg, "%s", ps_string_ed_string);
	}

doPrint:
	Com_Printf("%s\n", printMsg);
}

static void CG_GetCTFMessageEvent(const entityState_t* es)
{
	const int cl_index = es->trickedentindex;
	const int team_index = es->trickedentindex2;
	clientInfo_t* ci = NULL;
	const char* team_name = NULL;

	if (cl_index < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[cl_index];
	}

	if (team_index < 50)
	{
		team_name = CG_TeamName(team_index);
	}

	if (!ci)
	{
		return;
	}

	CG_PrintCTFMessage(ci, team_name, es->eventParm);
}

static void DoFall(centity_t* cent, const entityState_t* es, const int clientNum)
{
	const int delta = es->eventParm;

	if (cent->currentState.eFlags & EF_DEAD)
	{
		//corpses crack into the ground ^_^
		if (delta > 25)
		{
			trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.fallSound);
		}
		else
		{
			trap->S_StartSound(NULL, es->number, CHAN_AUTO,
				trap->S_RegisterSound("sound/movers/objects/objectHit.wav"));
		}
	}
	else if (PM_InKnockDownOnly(es->legsAnim))
	{
		if (delta > 14)
		{
			trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.fallSound);
		}
		else
		{
			trap->S_StartSound(NULL, es->number, CHAN_AUTO,
				trap->S_RegisterSound("sound/movers/objects/objectHit.wav"));
		}
	}
	else if (delta > 50)
	{
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.fallSound);
		trap->S_StartSound(NULL, cent->currentState.number, CHAN_VOICE,
			CG_CustomSound(cent->currentState.number, "*land1.wav"));
		cent->pe.painTime = cg.time; // don't play a pain sound right after this
	}
	else if (delta > 44)
	{
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.fallSound);
		trap->S_StartSound(NULL, cent->currentState.number, CHAN_VOICE,
			CG_CustomSound(cent->currentState.number, "*land1.wav"));
		cent->pe.painTime = cg.time; // don't play a pain sound right after this
	}
	else
	{
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.landSound);
	}

	if (clientNum == cg.predictedPlayerState.clientNum)
	{
		// smooth landing z changes
		cg.landChange = -delta;
		if (cg.landChange > 32)
		{
			cg.landChange = 32;
		}
		if (cg.landChange < -32)
		{
			cg.landChange = -32;
		}
		cg.landTime = cg.time;
	}
}

static int CG_InClientBitflags(const entityState_t* ent, const int client)
{
	int check_in;
	int sub = 0;

	if (client > 47)
	{
		check_in = ent->trickedentindex4;
		sub = 48;
	}
	else if (client > 31)
	{
		check_in = ent->trickedentindex3;
		sub = 32;
	}
	else if (client > 15)
	{
		check_in = ent->trickedentindex2;
		sub = 16;
	}
	else
	{
		check_in = ent->trickedentindex;
	}

	if (check_in & (1 << (client - sub)))
	{
		return 1;
	}

	return 0;
}

void CG_PlayDoorLoopSound(const centity_t* cent);
void CG_PlayDoorSound(const centity_t* cent, int type);

static void CG_TryPlayCustomSound(vec3_t origin, const int entityNum, const soundChannel_t channel, const char* sound_name)
{
	const sfxHandle_t c_sound = CG_CustomSound(entityNum, sound_name);

	if (c_sound <= 0)
	{
		return;
	}

	trap->S_StartSound(origin, entityNum, channel, c_sound);
}

static void CG_G2MarkEvent(entityState_t* es)
{
	//es->origin should be the hit location of the projectile,
	//whereas es->origin2 is the predicted position of the
	//projectile. (based on the trajectory upon impact) -rww
	centity_t* p_owner = &cg_entities[es->otherentity_num];
	vec3_t start_point;
	float size = 0.0f;
	qhandle_t shader = 0;

	if (!p_owner->ghoul2)
	{
		//can't do anything then...
		return;
	}

	//es->eventParm being non-0 means to do a special trace check
	//first. This will give us an impact right at the surface to
	//project the mark on. Typically this is used for radius
	//explosions and such, where the source position could be
	//way outside of model space.
	if (es->eventParm)
	{
		trace_t tr;
		int ignore = ENTITYNUM_NONE;

		CG_G2Trace(&tr, es->origin, NULL, NULL, es->origin2, ignore, MASK_PLAYERSOLID);

		if (tr.entityNum != es->otherentity_num)
		{
			//try again if we hit an ent but not the one we wanted.
			if (tr.entityNum < ENTITYNUM_WORLD)
			{
				ignore = tr.entityNum;
				CG_G2Trace(&tr, es->origin, NULL, NULL, es->origin2, ignore, MASK_PLAYERSOLID);
				if (tr.entityNum != es->otherentity_num)
				{
					//didn't manage to collide with the desired person. No mark will be placed then.
					return;
				}
			}
		}

		//otherwise we now have a valid starting point.
		VectorCopy(tr.endpos, start_point);
	}
	else
	{
		VectorCopy(es->origin, start_point);
	}

	if (es->eFlags & EF_JETPACK_ACTIVE)
	{
		// a vehicle weapon, make it a larger size mark
		//OR base this on the size of the thing you hit?
		if (g_vehWeaponInfo[es->otherentity_num2].fG2MarkSize)
		{
			size = flrand(0.6f, 1.4f) * g_vehWeaponInfo[es->otherentity_num2].fG2MarkSize;
		}
		else
		{
			size = flrand(32.0f, 72.0f);
		}
		//specify mark shader in vehWeapon file
		if (g_vehWeaponInfo[es->otherentity_num2].iG2MarkShaderHandle)
		{
			//have one we want to use instead of defaults
			shader = g_vehWeaponInfo[es->otherentity_num2].iG2MarkShaderHandle;
		}
	}
	switch (es->weapon)
	{
	case WP_BRYAR_PISTOL:
	case WP_CONCUSSION:
	case WP_BRYAR_OLD:
	case WP_BLASTER:
	case WP_DISRUPTOR:
	case WP_BOWCASTER:
	case WP_REPEATER:
	case WP_TURRET:
		if (!size)
		{
			size = 4.0f;
		}
		if (!shader)
		{
			shader = cgs.media.bdecal_bodyburn1;
		}
		CG_AddGhoul2Mark(shader, size,
			start_point, es->origin2, es->owner, p_owner->lerpOrigin,
			p_owner->lerpAngles[YAW], p_owner->ghoul2,
			p_owner->modelScale, Q_irand(10000, 20000));
		break;
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
		if (!size)
		{
			size = 24.0f;
		}
		if (!shader)
		{
			shader = cgs.media.bdecal_burn1;
		}
		CG_AddGhoul2Mark(shader, size,
			start_point, es->origin2, es->owner, p_owner->lerpOrigin,
			p_owner->lerpAngles[YAW], p_owner->ghoul2,
			p_owner->modelScale, Q_irand(10000, 20000));
		break;
		/*
	case WP_FLECHETTE:
		CG_AddGhoul2Mark(cgs.media.bdecal_bodyburn1, flrand(0.5f, 1.0f),
			startPoint, es->origin2, es->owner, pOwner->lerpOrigin,
			pOwner->lerpAngles[YAW], pOwner->ghoul2,
			pOwner->modelScale);
		break;
		*/
		//Issues with small scale?
	default:
		break;
	}
}

void CG_CalcVehMuzzle(Vehicle_t* p_veh, centity_t* ent, const int muzzle_num)
{
	mdxaBone_t boltMatrix;
	vec3_t veh_angles;

	assert(p_veh);

	if (p_veh->m_iMuzzleTime[muzzle_num] == cg.time)
	{
		//already done for this frame, don't need to do it again
		return;
	}
	//Uh... how about we set this, hunh...?  :)
	p_veh->m_iMuzzleTime[muzzle_num] = cg.time;

	VectorCopy(ent->lerpAngles, veh_angles);
	if (p_veh->m_pVehicleInfo)
	{
		if (p_veh->m_pVehicleInfo->type == VH_ANIMAL
			|| p_veh->m_pVehicleInfo->type == VH_WALKER)
		{
			veh_angles[PITCH] = veh_angles[ROLL] = 0.0f;
		}
		else if (p_veh->m_pVehicleInfo->type == VH_SPEEDER)
		{
			veh_angles[PITCH] = 0.0f;
		}
	}
	trap->G2API_GetBoltMatrix_NoRecNoRot(ent->ghoul2, 0, p_veh->m_iMuzzleTag[muzzle_num], &boltMatrix, veh_angles,
		ent->lerpOrigin, cg.time, NULL, ent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, p_veh->m_vMuzzlePos[muzzle_num]);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, p_veh->m_vMuzzleDir[muzzle_num]);
}

//corresponds to G_VehMuzzleFireFX -rww
static void CG_VehMuzzleFireFX(centity_t* veh, const entityState_t* broadcaster)
{
	const Vehicle_t* p_veh = veh->m_pVehicle;

	if (!p_veh || !veh->ghoul2)
	{
		return;
	}

	for (int cur_muz = 0; cur_muz < MAX_VEHICLE_MUZZLES; cur_muz++)
	{
		//go through all muzzles and
		if (p_veh->m_iMuzzleTag[cur_muz] != -1 //valid muzzle bolt
			&& broadcaster->trickedentindex & 1 << cur_muz) //fired
		{
			//this muzzle fired
			int muz_fx = 0;
			if (p_veh->m_pVehicleInfo->weapMuzzle[cur_muz] == 0)
			{
				//no weaopon for this muzzle?  check turrets
				for (int i = 0; i < MAX_VEHICLE_TURRETS; i++)
				{
					for (int j = 0; j < MAX_VEHICLE_TURRETS; j++)
					{
						if (p_veh->m_pVehicleInfo->turret[i].iMuzzle[j] - 1 == cur_muz)
						{
							//this muzzle belongs to this turret
							muz_fx = g_vehWeaponInfo[p_veh->m_pVehicleInfo->turret[i].iWeapon].iMuzzleFX;
							break;
						}
					}
				}
			}
			else
			{
				muz_fx = g_vehWeaponInfo[p_veh->m_pVehicleInfo->weapMuzzle[cur_muz]].iMuzzleFX;
			}
			if (muz_fx)
			{
				trap->FX_PlayBoltedEffectID(muz_fx, veh->currentState.origin, veh->ghoul2,
					p_veh->m_iMuzzleTag[cur_muz], veh->currentState.number, 0, 0, qtrue);
			}
		}
	}
}

const char* cg_stringEdVoiceChatTable[MAX_CUSTOM_SIEGE_SOUNDS] =
{
	"VC_ATT", //"*att_attack",
	"VC_ATT_PRIMARY", //"*att_primary",
	"VC_ATT_SECONDARY", //"*att_second",
	"VC_DEF_GUNS", //"*def_guns",
	"VC_DEF_POSITION", //"*def_position",
	"VC_DEF_PRIMARY", //"*def_primary",
	"VC_DEF_SECONDARY", //"*def_second",
	"VC_REPLY_COMING", //"*reply_coming",
	"VC_REPLY_GO", //"*reply_go",
	"VC_REPLY_NO", //"*reply_no",
	"VC_REPLY_STAY", //"*reply_stay",
	"VC_REPLY_YES", //"*reply_yes",
	"VC_REQ_ASSIST", //"*req_assist",
	"VC_REQ_DEMO", //"*req_demo",
	"VC_REQ_HVY", //"*req_hvy",
	"VC_REQ_MEDIC", //"*req_medic",
	"VC_REQ_SUPPLY", //"*req_sup",
	"VC_REQ_TECH", //"*req_tech",
	"VC_SPOT_AIR", //"*spot_air",
	"VC_SPOT_DEF", //"*spot_defenses",
	"VC_SPOT_EMPLACED", //"*spot_emplaced",
	"VC_SPOT_SNIPER", //"*spot_sniper",
	"VC_SPOT_TROOP", //"*spot_troops",
	"VC_TAC_COVER", //"*tac_cover",
	"VC_TAC_FALLBACK", //"*tac_fallback",
	"VC_TAC_FOLLOW", //"*tac_follow",
	"VC_TAC_HOLD", //"*tac_hold",
	"VC_TAC_SPLIT", //"*tac_split",
	"VC_TAC_TOGETHER", //"*tac_together",
	NULL
};

//stupid way of figuring out what string to use for voice chats
static const char* CG_GetStringForVoiceSound(const char* s)
{
	int i = 0;
	while (i < MAX_CUSTOM_SIEGE_SOUNDS)
	{
		if (bg_customSiegeSoundNames[i] && !Q_stricmp(bg_customSiegeSoundNames[i], s))
		{
			//get the matching reference name
			assert(cg_stringEdVoiceChatTable[i]);
			return CG_GetStringEdString("MENUS", (char*)cg_stringEdVoiceChatTable[i]);
		}
		i++;
	}

	return "voice chat";
}

/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/
#define	DEBUGNAME(x) if(cg_debugEvents.integer){trap->Print(x"\n");}

extern void CG_ChatBox_AddString(char* chat_str);

void CG_EntityEvent(centity_t* cent, vec3_t position)
{
	entityState_t* es;
	int event;
	vec3_t dir;
	const char* s;
	int clientNum;
	int e_id = 0;
	int isnd = 0;
	centity_t* cl_ent;

	es = &cent->currentState;
	event = es->event & ~EV_EVENT_BITS;

	if (cg_debugEvents.integer)
	{
		trap->Print("ent:%3i  event:%3i ", es->number, event);
	}

	if (!event)
	{
		DEBUGNAME("ZEROEVENT");
		return;
	}

	clientNum = es->clientNum;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		clientNum = 0;
	}

	if (es->eType == ET_NPC)
	{
		clientNum = es->number;

		if (!cent->npcClient)
		{
			CG_CreateNPCClient(&cent->npcClient); //allocate memory for it

			if (!cent->npcClient)
			{
				assert(0);
				return;
			}

			memset(cent->npcClient, 0, sizeof(clientInfo_t));
			cent->npcClient->ghoul2Model = NULL;
			cent->npcClient->gender = FindGender(CG_ConfigString(CS_MODELS + cent->currentState.modelIndex), cent);
		}

		assert(cent->npcClient);
	}

	switch (event)
	{
		//
		// movement generated events
		//
	case EV_CLIENTJOIN:
		DEBUGNAME("EV_CLIENTJOIN");

		//Slight hack to force a local reinit of client entity on join.
		cl_ent = &cg_entities[es->eventParm];

		if (cl_ent)
		{
			//cl_ent->torsoBolt = 0;
			cl_ent->bolt1 = 0;
			cl_ent->bolt2 = 0;
			cl_ent->bolt3 = 0;
			cl_ent->bolt4 = 0;
			cl_ent->bodyHeight = 0; //SABER_LENGTH_MAX;
			//cl_ent->saberExtendTime = 0;
			cl_ent->boltInfo = 0;
			cl_ent->frame_minus1_refreshed = 0;
			cl_ent->frame_minus2_refreshed = 0;
			cl_ent->frame_hold_time = 0;
			cl_ent->frame_hold_refreshed = 0;
			cl_ent->trickAlpha = 0;
			cl_ent->trickAlphaTime = 0;
			cl_ent->ghoul2weapon = NULL;
			cl_ent->weapon = WP_NONE;
			cl_ent->teamPowerEffectTime = 0;
			cl_ent->teamPowerType = 0;
			cl_ent->numLoopingSounds = 0;
			//cl_ent->localAnimIndex = 0;
		}
		break;

	case EV_FOOTSTEP:
		DEBUGNAME("EV_FOOTSTEP");
		if (cg_footsteps.integer)
		{
			footstep_t sound_type;
			switch (es->eventParm)
			{
			case MATERIAL_MUD:
				sound_type = FOOTSTEP_MUDWALK;
				break;
			case MATERIAL_DIRT:
				sound_type = FOOTSTEP_DIRTWALK;
				break;
			case MATERIAL_SAND:
				sound_type = FOOTSTEP_SANDWALK;
				break;
			case MATERIAL_SNOW:
				sound_type = FOOTSTEP_SNOWWALK;
				break;
			case MATERIAL_SHORTGRASS:
			case MATERIAL_LONGGRASS:
				sound_type = FOOTSTEP_GRASSWALK;
				break;
			case MATERIAL_SOLIDMETAL:
				sound_type = FOOTSTEP_METALWALK;
				break;
			case MATERIAL_HOLLOWMETAL:
				sound_type = FOOTSTEP_PIPEWALK;
				break;
			case MATERIAL_GRAVEL:
				sound_type = FOOTSTEP_GRAVELWALK;
				break;
			case MATERIAL_CARPET:
			case MATERIAL_FABRIC:
			case MATERIAL_CANVAS:
			case MATERIAL_RUBBER:
			case MATERIAL_PLASTIC:
				sound_type = FOOTSTEP_RUGWALK;
				break;
			case MATERIAL_SOLIDWOOD:
			case MATERIAL_HOLLOWWOOD:
				sound_type = FOOTSTEP_WOODWALK;
				break;

			default:
				sound_type = FOOTSTEP_STONEWALK;
				break;
			}

			trap->S_StartSound(NULL, es->number, CHAN_BODY, cgs.media.footsteps[sound_type][rand() & 3]);
		}
		break;
	case EV_FOOTSTEP_METAL:
		DEBUGNAME("EV_FOOTSTEP_METAL");
		if (cg_footsteps.integer)
		{
			trap->S_StartSound(NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[FOOTSTEP_METALWALK][rand() & 3]);
		}
		break;
	case EV_FOOTSPLASH:
		DEBUGNAME("EV_FOOTSPLASH");
		if (cg_footsteps.integer)
		{
			trap->S_StartSound(NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[FOOTSTEP_SPLASH][rand() & 3]);
		}
		break;
	case EV_FOOTWADE:
		DEBUGNAME("EV_FOOTWADE");
		if (cg_footsteps.integer)
		{
			trap->S_StartSound(NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[FOOTSTEP_SPLASH][rand() & 3]);
		}
		break;
	case EV_SWIM:
		DEBUGNAME("EV_SWIM");
		if (cg_footsteps.integer)
		{
			trap->S_StartSound(NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[FOOTSTEP_SPLASH][rand() & 3]);
		}
		break;

	case EV_FALL:
		DEBUGNAME("EV_FALL");
		if (es->number == cg.snap->ps.clientNum && cg.snap->ps.fallingToDeath)
		{
			break;
		}
		DoFall(cent, es, clientNum);
		break;
	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16: // smooth out step up transitions
		DEBUGNAME("EV_STEP");
		{
			float old_step;
			int delta;
			int step;

			if (clientNum != cg.predictedPlayerState.clientNum)
			{
				break;
			}
			// if we are interpolating, we don't need to smooth steps
			if (cg.demoPlayback || cg.snap->ps.pm_flags & PMF_FOLLOW ||
				cg_noPredict.integer || g_synchronousClients.integer)
			{
				break;
			}
			// check for stepping up before a previous step is completed
			delta = cg.time - cg.stepTime;
			if (delta < STEP_TIME)
			{
				old_step = cg.stepChange * (STEP_TIME - delta) / STEP_TIME;
			}
			else
			{
				old_step = 0;
			}

			// add this amount
			step = 4 * (event - EV_STEP_4 + 1);
			cg.stepChange = old_step + step;
			if (cg.stepChange > MAX_STEP_CHANGE)
			{
				cg.stepChange = MAX_STEP_CHANGE;
			}
			cg.stepTime = cg.time;
			break;
		}
		//new saberlock clash effects
	case EV_SABERLOCK:
		DEBUGNAME("EV_SABERLOCK");
		{
			//entity is in saberlock, render blade lock and sound effects
			centity_t* enemy;
			clientInfo_t* enemy_client = NULL;
			clientInfo_t* client = NULL;
			int index = 1;

			if (es->eventParm < 0 && es->eventParm >= ENTITYNUM_WORLD)
			{
				//invalid entity number.  This is possible with limited bits of eventParm
				break;
			}
			enemy = &cg_entities[es->eventParm];
			index = Q_irand(1, 9);

			if (!enemy || !enemy->ghoul2 || !cent || !cent->ghoul2)
			{
				//no enemy data, can't do our visual effects.
				break;
			}
			if (es->number != cg.snap->ps.clientNum //not the client
				&& (es->eventParm == cg.snap->ps.clientNum //other lock player is the client
					|| es->eventParm < es->number)) //other player has lower entity number.
			{
				//don't render the effects twice.
				break;
			}
			//set client
			if (cg_entities[es->number].currentState.eType == ET_NPC)
			{
				client = cg_entities[es->number].npcClient;
			}
			else if (es->number < MAX_CLIENTS)
			{
				client = &cgs.clientinfo[es->number];
			}

			if (cg_entities && cg_entities[es->eventParm].currentState.eType == ET_NPC)
			{
				enemy_client = cg_entities[es->eventParm].npcClient;
			}
			else if (es->number < MAX_CLIENTS)
			{
				enemy_client = &cgs.clientinfo[es->eventParm];
			}
			if (client && client->infoValid && enemy_client && enemy_client->infoValid)
			{
				vec3_t our_base, our_tip, their_base, their_tip;
				mdxaBone_t boltMatrix;
				vec3_t flash_point, temp;
				qboolean cull_pass = qfalse;
				qhandle_t lock_sound = trap->S_RegisterSound(va("sound/weapons/saber/saberlock%d.mp3", index));

				//get our blade
				trap->G2API_GetBoltMatrix(cent->ghoul2, 1, 0, &boltMatrix, cent->lerpAngles, cent->lerpOrigin,
					cg.time, cgs.game_models, cent->modelScale);
				BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, our_base);
				BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, temp);
				VectorMA(our_base, client->saber[0].blade[0].length, temp, our_tip);
				//get their blade.
				trap->G2API_GetBoltMatrix(enemy->ghoul2, 1, 0, &boltMatrix, enemy->lerpAngles, enemy->lerpOrigin,
					cg.time, cgs.game_models, enemy->modelScale);
				BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, their_base);
				BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, temp);
				VectorMA(their_base, client->saber[0].blade[0].length, temp, their_tip);
				ShortestLineSegBewteen2LineSegs(our_base, our_tip, their_base, their_tip, flash_point, temp);

				if (cg.mInRMG)
				{
					trace_t tr;
					vec3_t vec_sub;
					VectorSubtract(cg.refdef.vieworg, flash_point, vec_sub);
					if (VectorLength(vec_sub) < 5000)
					{
						CG_Trace(&tr, cg.refdef.vieworg, NULL, NULL, flash_point, ENTITYNUM_NONE, CONTENTS_TERRAIN | CONTENTS_SOLID);
						if (tr.fraction == 1.0 || tr.entityNum < MAX_CLIENTS)
						{
							cull_pass = qtrue;
						}
					}
				}
				else
				{
					cull_pass = qtrue;
				}
				if (cull_pass)
				{
					vec3_t fx_dir;

					//angle setup
					VectorCopy(vec3_origin, fx_dir);
					if (!fx_dir[0] && !fx_dir[1] && !fx_dir[2])
					{
						fx_dir[1] = 1;
					}
					if (client->saberLockSoundDebounce < cg.time)
					{
						trap->S_StartSound(flash_point, es->number, CHAN_AUTO, lock_sound);
						client->saberLockSoundDebounce = cg.time + 200;
					}
					trap->FX_PlayEffectID(cgs.effects.mSaberLock, flash_point, fx_dir, -1, -1, qfalse);
				}
			}
		}
		break;
	case EV_FLASHGRENADE:
		DEBUGNAME("EV_FLASHGRENADE");
		{
			vec3_t up = { 0, 0, 1 };
			trap->FX_PlayEffectID(trap->FX_RegisterEffect("flash/faceflash"), position, up, -1, -1, qfalse);
		}
		break;
	case EV_SMOKEGRENADE:
		DEBUGNAME("EV_SMOKEGRENADE");
		{
			vec3_t up = { 0, 0, 1 };
			trap->FX_PlayEffectID(trap->FX_RegisterEffect("smoke/smokegrenade"), position, up, -1, -1, qfalse);
		}
		break;
	case EV_CRYOBAN_EXPLODE:
		DEBUGNAME("EV_CRYOBAN_EXPLODE");
		{
			vec3_t up = { 0, 0, 1 };
			trap->FX_PlayEffectID(trap->FX_RegisterEffect("cryoban/explosion"), position, up, -1, -1, qfalse);
		}
		break;
	case EV_CRYOBAN:
		DEBUGNAME("EV_CRYOBAN");
		{
			vec3_t up = { 0, 0, 1 };
			trap->FX_PlayEffectID(trap->FX_RegisterEffect("cryoban/stuck"), cent->playerState->origin, up, -1, -1,
				qfalse);
		}
		break;
	case EV_STASIS:
		DEBUGNAME("EV_STASIS");
		{
			vec3_t up = { 0, 0, 1 };
			trap->FX_PlayEffectID(trap->FX_RegisterEffect("cryoban/stasis"), cent->playerState->origin, up, -1, -1,
				qfalse);
		}
		break;

	case EV_JUMP_PAD:
		DEBUGNAME("EV_JUMP_PAD");
		break;

	case EV_GHOUL2_MARK:
		DEBUGNAME("EV_GHOUL2_MARK");

		if (cg_ghoul2Marks.integer)
		{
			//Can we put a burn mark on him?
			CG_G2MarkEvent(es);
		}
		break;

	case EV_GLOBAL_DUEL:
		DEBUGNAME("EV_GLOBAL_DUEL");
		//used for beginning of power duels
		//if (cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_SPECTATOR)
		if (es->otherentity_num == cg.predictedPlayerState.clientNum ||
			es->otherentity_num2 == cg.predictedPlayerState.clientNum ||
			es->groundEntityNum == cg.predictedPlayerState.clientNum)
		{
			CG_CenterPrint(CG_GetStringEdString("MP_SVGAME", "BEGIN_DUEL"), 120, GIANTCHAR_WIDTH * 2);
			trap->S_StartLocalSound(cgs.media.countFightSound, CHAN_ANNOUNCER);
		}
		break;

	case EV_PRIVATE_DUEL:
		DEBUGNAME("EV_PRIVATE_DUEL");

		if (cg.snap->ps.clientNum != es->number)
		{
			break;
		}

		if (es->eventParm)
		{
			//starting the duel
			if (es->eventParm == 2)
			{
				CG_CenterPrint(CG_GetStringEdString("MP_SVGAME", "BEGIN_DUEL"), 120, GIANTCHAR_WIDTH * 2);
				trap->S_StartLocalSound(cgs.media.countFightSound, CHAN_ANNOUNCER);
			}
			else
			{
				trap->S_StartBackgroundTrack("music/mp/duel.mp3", "music/mp/duel.mp3", qfalse);
			}
		}
		else
		{
			//ending the duel
			CG_StartMusic(qtrue);
		}
		break;

	case EV_JUMP:
		DEBUGNAME("EV_JUMP");
		if (!cg_jumpSounds.integer)
		{
			break;
		}
		if (cg.time - cent->pe.painTime < 500) //don't play immediately after pain/fall sound?
		{
			break;
		}
		if (cg.predictedPlayerState.duelInProgress &&
			(cg.predictedPlayerState.clientNum != es->clientNum &&
				cg.predictedPlayerState.duelIndex != es->clientNum))
		{
			break;
		}
		if (cg_jumpSounds.integer == 1)
		{
			trap->S_StartSound(NULL, es->number, CHAN_VOICE, CG_CustomSound(es->number, "*jump1.wav"));
			//play all jump sounds
		}
		else if (cg_jumpSounds.integer == 2 && cg.snap->ps.clientNum != es->number)
		{
			trap->S_StartSound(NULL, es->number, CHAN_VOICE, CG_CustomSound(es->number, "*jump1.wav"));
			//only play other players' jump sounds
		}
		else if (cg_jumpSounds.integer > 2 && cg.snap->ps.clientNum == es->number)
		{
			trap->S_StartSound(NULL, es->number, CHAN_VOICE, CG_CustomSound(es->number, "*jump1.wav"));
			//only play my jump sounds
		}
		break;

	case EV_ROLL:
		DEBUGNAME("EV_ROLL");
		if (!cg_rollSounds.integer)
		{
			break;
		}
		if (cg_entities[es->number].currentState.eType != ET_NPC && !cg_entities[es->number].currentState.
			m_iVehicleNum)
		{
			cg_entities[es->number].currentState.torsoFlip ^= qtrue;
			cg_entities[es->number].currentState.legsFlip ^= qtrue;

			if (cg.predictedPlayerState.pm_flags & PMF_FOLLOW && es->number == cg.predictedPlayerState.clientNum)
			{
				cg.predictedPlayerState.torsoFlip ^= qtrue;
				cg.predictedPlayerState.legsFlip ^= qtrue;

				if (cg.snap)
				{
					//ugly hack
					cg.snap->ps.torsoFlip ^= qtrue;
					cg.snap->ps.legsFlip ^= qtrue;
				}

				if (cg.nextSnap)
				{
					//xd
					cg.nextSnap->ps.torsoFlip ^= qtrue;
					cg.nextSnap->ps.legsFlip ^= qtrue;
				}
			}
		}
		if (es->number == cg.snap->ps.clientNum && cg.snap->ps.fallingToDeath)
		{
			break;
		}

		if (cg.predictedPlayerState.duelInProgress &&
			(es->clientNum != cg.predictedPlayerState.clientNum &&
				es->clientNum != cg.predictedPlayerState.duelIndex))
		{
			break;
		}

		if (es->eventParm) //fall-roll-in-one event
		{
			DoFall(cent, es, clientNum);
		}

		if (cg_rollSounds.integer == 1)
		{
			trap->S_StartSound(NULL, es->number, CHAN_VOICE, CG_CustomSound(es->number, "*jump1.wav"));
		}
		else if (cg_rollSounds.integer == 2 && cg.snap->ps.clientNum != es->number)
		{
			trap->S_StartSound(NULL, es->number, CHAN_VOICE, CG_CustomSound(es->number, "*jump1.wav"));
		}
		else if (cg_rollSounds.integer > 2 && cg.snap->ps.clientNum == es->number)
		{
			trap->S_StartSound(NULL, es->number, CHAN_VOICE, CG_CustomSound(es->number, "*jump1.wav"));
		}
		trap->S_StartSound(NULL, es->number, CHAN_BODY, cgs.media.rollSound);
		break;

	case EV_TAUNT:
		DEBUGNAME("EV_TAUNT");
		int sound_index = 0;

		if (cgs.gametype != GT_DUEL
			&& cgs.gametype != GT_POWERDUEL)
		{//normal taunt
			switch (es->eventParm)
			{
			case TAUNT_TAUNT:
				sound_index = CG_CustomSound(es->number, va("*taunt%d", Q_irand(1, 5)));

				if (!sound_index)
				{
					sound_index = CG_CustomSound(es->number, va("*anger%d", Q_irand(1, 3)));

					if (!sound_index)
					{
						sound_index = CG_CustomSound(es->number, va("*victory%d", Q_irand(1, 3)));

						if (!sound_index)
						{
							sound_index = CG_CustomSound(es->number, va("*deflect%d", Q_irand(1, 3)));

							if (!sound_index)
							{
								sound_index = CG_CustomSound(es->number, va("*gloat%d", Q_irand(1, 3)));

								if (!sound_index)
								{
									sound_index = CG_CustomSound(es->number, va("*giveup%d", Q_irand(1, 3)));

									if (!sound_index)
									{
										sound_index = CG_CustomSound(es->number, "*taunt");
									}
								}
							}
						}
					}
				}
				break;
			case TAUNT_BOW:
				break;
			case TAUNT_MEDITATE:
				/*sound_index = CG_CustomSound(es->number, va("*pushfail"));

				if (!sound_index)
				{
					sound_index = CG_CustomSound(es->number, "*taunt");
				}*/
				break;
			case TAUNT_FLOURISH:
				sound_index = CG_CustomSound(es->number, va("*victory%d", Q_irand(1, 3)));

				if (!sound_index)
				{
					sound_index = CG_CustomSound(es->number, va("*anger%d", Q_irand(1, 3)));

					if (!sound_index)
					{
						sound_index = CG_CustomSound(es->number, va("*victory%d", Q_irand(1, 3)));

						if (!sound_index)
						{
							sound_index = CG_CustomSound(es->number, va("*deflect%d", Q_irand(1, 3)));

							if (!sound_index)
							{
								sound_index = CG_CustomSound(es->number, va("*gloat%d", Q_irand(1, 3)));

								if (!sound_index)
								{
									sound_index = CG_CustomSound(es->number, va("*giveup%d", Q_irand(1, 3)));

									if (!sound_index)
									{
										sound_index = CG_CustomSound(es->number, "*taunt");
									}
								}
							}
						}
					}
				}
				break;
			case TAUNT_GLOAT:
				sound_index = CG_CustomSound(es->number, va("*gloat%d", Q_irand(1, 3)));

				if (!sound_index)
				{
					sound_index = CG_CustomSound(es->number, va("*anger%d", Q_irand(1, 3)));

					if (!sound_index)
					{
						sound_index = CG_CustomSound(es->number, va("*victory%d", Q_irand(1, 3)));

						if (!sound_index)
						{
							sound_index = CG_CustomSound(es->number, va("*deflect%d", Q_irand(1, 3)));

							if (!sound_index)
							{
								sound_index = CG_CustomSound(es->number, va("*gloat%d", Q_irand(1, 3)));

								if (!sound_index)
								{
									sound_index = CG_CustomSound(es->number, va("*giveup%d", Q_irand(1, 3)));

									if (!sound_index)
									{
										sound_index = CG_CustomSound(es->number, "*taunt");
									}
								}
							}
						}
					}
				}
				break;
			case TAUNT_SURRENDER:
				sound_index = CG_CustomSound(es->number, va("*confuse%d", Q_irand(1, 3)));

				if (!sound_index)
				{
					sound_index = CG_CustomSound(es->number, "*taunt");
				}
				break;
			case TAUNT_RELOAD:
				break;
			default:;
				break;
			}
		}
		else
		{
			switch (es->eventParm)
			{
			case TAUNT_TAUNT:
			default:
				if (Q_irand(0, 1))
				{
					sound_index = CG_CustomSound(es->number, va("*anger%d.wav", Q_irand(1, 3)));
				}
				else
				{
					sound_index = CG_CustomSound(es->number, va("*taunt%d.wav", Q_irand(1, 5)));
					if (!sound_index)
					{
						sound_index = CG_CustomSound(es->number, va("*anger%d.wav", Q_irand(1, 3)));
					}
				}
				break;
			case TAUNT_BOW:
				break;
			case TAUNT_MEDITATE:
				break;
			case TAUNT_FLOURISH:
				if (Q_irand(0, 1))
				{
					sound_index = CG_CustomSound(es->number, va("*deflect%d.wav", Q_irand(1, 3)));
					if (!sound_index)
					{
						sound_index = CG_CustomSound(es->number, va("*gloat%d.wav", Q_irand(1, 3)));
						if (!sound_index)
						{
							sound_index = CG_CustomSound(es->number, va("*anger%d.wav", Q_irand(1, 3)));
						}
					}
				}
				else
				{
					sound_index = CG_CustomSound(es->number, va("*gloat%d.wav", Q_irand(1, 3)));
					if (!sound_index)
					{
						sound_index = CG_CustomSound(es->number, va("*deflect%d.wav", Q_irand(1, 3)));
						if (!sound_index)
						{
							sound_index = CG_CustomSound(es->number, va("*anger%d.wav", Q_irand(1, 3)));
						}
					}
				}
				break;
			case TAUNT_GLOAT:
				sound_index = CG_CustomSound(es->number, va("*victory%d.wav", Q_irand(1, 3)));
				break;
			case TAUNT_SURRENDER:
				sound_index = CG_CustomSound(es->number, va("*confuse%d.wav", Q_irand(1, 3)));

				if (!sound_index)
				{
					sound_index = CG_CustomSound(es->number, "*taunt.wav");
				}
				break;
			case TAUNT_RELOAD:
				break;
			}
		}
		if (!sound_index)
		{
			sound_index = CG_CustomSound(es->number, "*taunt.wav");
		}
		if (sound_index)
		{
			trap->S_StartSound(NULL, es->number, CHAN_VOICE, sound_index);
		}
		break;

		//Begin NPC sounds
	case EV_ANGER1: //Say when acquire an enemy when didn't have one before
	case EV_ANGER2:
	case EV_ANGER3:
		DEBUGNAME("EV_ANGERx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*anger%i.wav", event - EV_ANGER1 + 1));
		break;

	case EV_VICTORY1: //Say when killed an enemy
	case EV_VICTORY2:
	case EV_VICTORY3:
		DEBUGNAME("EV_VICTORYx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*victory%i.wav", event - EV_VICTORY1 + 1));
		break;

	case EV_CONFUSE1: //Say when confused
	case EV_CONFUSE2:
	case EV_CONFUSE3:
		DEBUGNAME("EV_CONFUSEDx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*confuse%i.wav", event - EV_CONFUSE1 + 1));
		break;

	case EV_PUSHED1: //Say when pushed
	case EV_PUSHED2:
	case EV_PUSHED3:
		DEBUGNAME("EV_PUSHEDx");
		// Delay between pushed sounds
		if (cg.time - cent->pe.pushedTime < 2500)
		{
			return;
		}
		cent->pe.pushedTime = cg.time;
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*pushed%i.wav", event - EV_PUSHED1 + 1));
		break;

	case EV_CHOKE1: //Say when choking
	case EV_CHOKE2:
	case EV_CHOKE3:
		DEBUGNAME("EV_CHOKEx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*choke%i.wav", event - EV_CHOKE1 + 1));
		break;

	case EV_FFWARN: //Warn ally to stop shooting you
		DEBUGNAME("EV_FFWARN");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, "*ffwarn.wav");
		break;

	case EV_FFTURN: //Turn on ally after being shot by them
		DEBUGNAME("EV_FFTURN");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, "*ffturn.wav");
		break;

		//extra sounds for ST
	case EV_CHASE1:
	case EV_CHASE2:
	case EV_CHASE3:
		DEBUGNAME("EV_CHASEx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*chase%i.wav", event - EV_CHASE1 + 1));
		break;
	case EV_COVER1:
	case EV_COVER2:
	case EV_COVER3:
	case EV_COVER4:
	case EV_COVER5:
		DEBUGNAME("EV_COVERx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*cover%i.wav", event - EV_COVER1 + 1));
		break;
	case EV_DETECTED1:
	case EV_DETECTED2:
	case EV_DETECTED3:
	case EV_DETECTED4:
	case EV_DETECTED5:
		DEBUGNAME("EV_DETECTEDx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*detected%i.wav", event - EV_DETECTED1 + 1));
		break;
	case EV_GIVEUP1:
	case EV_GIVEUP2:
	case EV_GIVEUP3:
	case EV_GIVEUP4:
		DEBUGNAME("EV_GIVEUPx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*giveup%i.wav", event - EV_GIVEUP1 + 1));
		break;
	case EV_LOOK1:
	case EV_LOOK2:
		DEBUGNAME("EV_LOOKx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*look%i.wav", event - EV_LOOK1 + 1));
		break;
	case EV_LOST1:
		DEBUGNAME("EV_LOST1");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, "*lost1.wav");
		break;
	case EV_OUTFLANK1:
	case EV_OUTFLANK2:
		DEBUGNAME("EV_OUTFLANKx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*outflank%i.wav", event - EV_OUTFLANK1 + 1));
		break;
	case EV_ESCAPING1:
	case EV_ESCAPING2:
	case EV_ESCAPING3:
		DEBUGNAME("EV_ESCAPINGx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*escaping%i.wav", event - EV_ESCAPING1 + 1));
		break;
	case EV_SIGHT1:
	case EV_SIGHT2:
	case EV_SIGHT3:
		DEBUGNAME("EV_SIGHTx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*sight%i.wav", event - EV_SIGHT1 + 1));
		break;
	case EV_SOUND1:
	case EV_SOUND2:
	case EV_SOUND3:
		DEBUGNAME("EV_SOUNDx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*sound%i.wav", event - EV_SOUND1 + 1));
		break;
	case EV_SUSPICIOUS1:
	case EV_SUSPICIOUS2:
	case EV_SUSPICIOUS3:
	case EV_SUSPICIOUS4:
	case EV_SUSPICIOUS5:
		DEBUGNAME("EV_SUSPICIOUSx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*suspicious%i.wav", event - EV_SUSPICIOUS1 + 1));
		break;
		//extra sounds for Jedi
	case EV_COMBAT1:
	case EV_COMBAT2:
	case EV_COMBAT3:
		DEBUGNAME("EV_COMBATx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*combat%i.wav", event - EV_COMBAT1 + 1));
		break;
	case EV_JDETECTED1:
	case EV_JDETECTED2:
	case EV_JDETECTED3:
		DEBUGNAME("EV_JDETECTEDx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*jdetected%i.wav", event - EV_JDETECTED1 + 1));
		break;
	case EV_TAUNT1:
	case EV_TAUNT2:
	case EV_TAUNT3:
		DEBUGNAME("EV_TAUNTx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*taunt%i.wav", event - EV_TAUNT1 + 1));
		break;
	case EV_JCHASE1:
	case EV_JCHASE2:
	case EV_JCHASE3:
		DEBUGNAME("EV_JCHASEx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*jchase%i.wav", event - EV_JCHASE1 + 1));
		break;
	case EV_JLOST1:
	case EV_JLOST2:
	case EV_JLOST3:
		DEBUGNAME("EV_JLOSTx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*jlost%i.wav", event - EV_JLOST1 + 1));
		break;
	case EV_DEFLECT1:
	case EV_DEFLECT2:
	case EV_DEFLECT3:
		DEBUGNAME("EV_DEFLECTx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*deflect%i.wav", event - EV_DEFLECT1 + 1));
		break;
	case EV_GLOAT1:
	case EV_GLOAT2:
	case EV_GLOAT3:
		DEBUGNAME("EV_GLOATx");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, va("*gloat%i.wav", event - EV_GLOAT1 + 1));
		break;
	case EV_PUSHFAIL:
		DEBUGNAME("EV_PUSHFAIL");
		CG_TryPlayCustomSound(NULL, es->number, CHAN_VOICE, "*pushfail.wav");
		break;
		//End NPC sounds

	case EV_SIEGESPEC:
		DEBUGNAME("EV_SIEGESPEC");
		if (es->owner == cg.predictedPlayerState.clientNum)
		{
			cg_siegeDeathTime = es->time;
		}

		break;

	case EV_WATER_TOUCH:
		DEBUGNAME("EV_WATER_TOUCH");
		if (cent->currentState.NPC_class == CLASS_ATST)
		{
			trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.atstWaterOutSound[Q_irand(0, 1)]);
		}
		else
		{
			trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrInSound);
		}
		break;

	case EV_WATER_LEAVE:
		DEBUGNAME("EV_WATER_LEAVE");
		if (cent->currentState.NPC_class == CLASS_ATST)
		{
			trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.atstWaterOutSound[Q_irand(0, 1)]);
		}
		else
		{
			trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound);
		}
		break;

	case EV_WATER_UNDER:
		DEBUGNAME("EV_WATER_UNDER");
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound);
		break;
	case EV_WATER_CLEAR:
		DEBUGNAME("EV_WATER_CLEAR");
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, CG_CustomSound(es->number, "*gasp.wav"));
		break;

	case EV_WATER_GURP1:
	case EV_WATER_GURP2:
		DEBUGNAME("EV_WATER_GURPx");
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, CG_CustomSound(es->number, "*gurp%d.wav"));
		break;

	case EV_WATER_DROWN:
		DEBUGNAME("EV_WATER_DROWN");
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, CG_CustomSound(es->number, "*drown.wav"));
		break;

	case EV_LAVA_TOUCH:
		DEBUGNAME("EV_LAVA_TOUCH");
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.lavaInSound);
		break;

	case EV_LAVA_LEAVE:
		DEBUGNAME("EV_LAVA_LEAVE");
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.lavaOutSound);
		break;

	case EV_LAVA_UNDER:
		DEBUGNAME("EV_LAVA_UNDER");
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.lavaUnSound);
		break;

	case EV_ITEM_PICKUP:
		DEBUGNAME("EV_ITEM_PICKUP");
		{
			gitem_t* item;
			int index;
			qboolean newindex = qfalse;

			index = cg_entities[es->eventParm].currentState.modelIndex; // player predicted

			if (index < 1 && cg_entities[es->eventParm].currentState.isJediMaster)
			{
				//a holocron most likely
				index = cg_entities[es->eventParm].currentState.trickedentindex4;
				trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.holocronPickup);

				if (es->number == cg.snap->ps.clientNum && showPowersName[index])
				{
					const char* str_text = CG_GetStringEdString("MP_INGAME", "PICKUPLINE");

					//Com_Printf("%s %s\n", strText, showPowersName[index]);
					CG_CenterPrint(va("%s %s\n", str_text, CG_GetStringEdString("SP_INGAME", showPowersName[index])),
						SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				}

				//Show the player their force selection bar in case picking the holocron up changed the current selection
				if (index != FP_SABER_OFFENSE && index != FP_SABER_DEFENSE && index != FP_SABERTHROW &&
					index != FP_LEVITATION &&
					es->number == cg.snap->ps.clientNum &&
					(index == cg.snap->ps.fd.forcePowerSelected || !(cg.snap->ps.fd.forcePowersActive & 1 << cg.snap
						->ps.fd.forcePowerSelected)))
				{
					if (cg.forceSelect != index)
					{
						cg.forceSelect = index;
						newindex = qtrue;
					}
				}

				if (es->number == cg.snap->ps.clientNum && newindex)
				{
					if (cg.forceSelectTime < cg.time)
					{
						cg.forceSelectTime = cg.time;
					}
				}

				break;
			}

			if (cg_entities[es->eventParm].weapon >= cg.time)
			{
				//rww - an unfortunately necessary hack to prevent double item pickups
				break;
			}

			//Hopefully even if this entity is somehow removed and replaced with, say, another
			//item, this time will have expired by the time that item needs to be picked up.
			//Of course, it's quite possible this will fail miserably, so if you've got a better
			//solution then please do use it.
			cg_entities[es->eventParm].weapon = cg.time + 500;

			if (index < 1 || index >= bg_numItems)
			{
				break;
			}
			item = &bg_itemlist[index];

			if (/*item->giType != IT_POWERUP && */item->giType != IT_TEAM)
			{
				if (item->pickup_sound && item->pickup_sound[0])
				{
					trap->S_StartSound(NULL, es->number, CHAN_AUTO, trap->S_RegisterSound(item->pickup_sound));
				}
			}

			// show icon and name on status bar
			if (es->number == cg.snap->ps.clientNum)
			{
				CG_ItemPickup(index);
			}
		}
		break;

	case EV_GLOBAL_ITEM_PICKUP:
		DEBUGNAME("EV_GLOBAL_ITEM_PICKUP");
		{
			gitem_t* item;
			int index;

			index = es->eventParm; // player predicted

			if (index < 1 || index >= bg_numItems)
			{
				break;
			}
			item = &bg_itemlist[index];
			// powerup pickups are global
			if (item->pickup_sound && item->pickup_sound[0])
			{
				trap->S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO,
					trap->S_RegisterSound(item->pickup_sound));
			}

			// show icon and name on status bar
			if (es->number == cg.snap->ps.clientNum)
			{
				CG_ItemPickup(index);
			}
		}
		break;

	case EV_VEH_FIRE:
		DEBUGNAME("EV_VEH_FIRE");
		{
			centity_t* veh = &cg_entities[es->owner];
			CG_VehMuzzleFireFX(veh, es);
		}
		break;

		//
		// weapon events
		//
	case EV_NOAMMO:
		DEBUGNAME("EV_NOAMMO");
		//		trap->S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.noAmmoSound );
		if (es->number == cg.snap->ps.clientNum)
		{
			if (CG_InFighter() || CG_InATST() || cg.snap->ps.weapon == WP_NONE)
			{
				//just letting us know our vehicle is out of ammo
				//FIXME: flash something on HUD or give some message so we know we have no ammo
				centity_t* local_cent = &cg_entities[cg.snap->ps.clientNum];
				if (local_cent->m_pVehicle
					&& local_cent->m_pVehicle->m_pVehicleInfo
					&& local_cent->m_pVehicle->m_pVehicleInfo->weapon[es->eventParm].soundNoAmmo)
				{
					//play the "no Ammo" sound for this weapon
					trap->S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO,
						local_cent->m_pVehicle->m_pVehicleInfo->weapon[es->eventParm].soundNoAmmo);
				}
				else
				{
					//play the default "no ammo" sound
					trap->S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.noAmmoSound);
				}
				//flash the HUD so they associate the sound with the visual indicator that they don't have enough ammo
				if (cg_vehicleAmmoWarningTime < cg.time
					|| cg_vehicleAmmoWarning != es->eventParm)
				{
					//if there's already one going, don't interrupt it (unless they tried to fire another weapon that's out of ammo)
					cg_vehicleAmmoWarning = es->eventParm;
					cg_vehicleAmmoWarningTime = cg.time + 500;
				}
			}
			else if (cg.snap->ps.weapon == WP_SABER)
			{
				cg.forceHUDTotalFlashTime = cg.time + 1000;
				cg.mishapHUDTotalFlashTime = cg.time + 500;
				cg.blockHUDTotalFlashTime = cg.time + 500;
			}
			else
			{
				int weap = 0;

				if (es->eventParm && es->eventParm < WP_NUM_WEAPONS)
				{
					cg.snap->ps.stats[STAT_WEAPONS] &= ~(1 << es->eventParm);
					weap = cg.snap->ps.weapon;
				}
				else if (es->eventParm)
				{
					weap = es->eventParm - WP_NUM_WEAPONS;
				}
				CG_OutOfAmmoChange(weap);
			}
		}
		break;
	case EV_CHANGE_WEAPON:
		DEBUGNAME("EV_CHANGE_WEAPON");
		{
			int weapon = es->eventParm;
			weaponInfo_t* weapon_info;

			assert(weapon >= 0 && weapon < MAX_WEAPONS);

			weapon_info = &cg_weapons[weapon];

			assert(weapon_info);

			if (weapon_info->selectSound)
			{
				trap->S_StartSound(NULL, es->number, CHAN_AUTO, weapon_info->selectSound);
			}
			else if (weapon != WP_SABER)
			{
				//not sure what SP is doing for this but I don't want a select sound for saber (it has the saber-turn-on)
				trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.selectSound);
			}
		}
		break;
	case EV_FIRE_WEAPON:
		DEBUGNAME("EV_FIRE_WEAPON");
		if (cent->currentState.number >= MAX_CLIENTS && cent->currentState.eType != ET_NPC)
		{
			//special case for turret firing
			vec3_t gunpoint, gunangle;
			mdxaBone_t matrix;

			weaponInfo_t* weapon_info = &cg_weapons[WP_TURRET];

			if (!weapon_info->registered)
			{
				CG_RegisterWeapon(WP_TURRET);
			}

			if (cent->ghoul2)
			{
				if (!cent->bolt1)
				{
					cent->bolt1 = trap->G2API_AddBolt(cent->ghoul2, 0, "*flash01");
				}
				if (!cent->bolt2)
				{
					cent->bolt2 = trap->G2API_AddBolt(cent->ghoul2, 0, "*flash02");
				}
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, "Bone02", 1, 4,
					BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND,
					1.0f, cg.time, -1, 300);
			}
			else
			{
				break;
			}

			if (cent->currentState.eventParm)
			{
				trap->G2API_GetBoltMatrix(cent->ghoul2, 0, cent->bolt2, &matrix, cent->currentState.angles,
					cent->currentState.origin, cg.time, cgs.game_models, cent->modelScale);
			}
			else
			{
				trap->G2API_GetBoltMatrix(cent->ghoul2, 0, cent->bolt1, &matrix, cent->currentState.angles,
					cent->currentState.origin, cg.time, cgs.game_models, cent->modelScale);
			}

			gunpoint[0] = matrix.matrix[0][3];
			gunpoint[1] = matrix.matrix[1][3];
			gunpoint[2] = matrix.matrix[2][3];

			gunangle[0] = -matrix.matrix[0][0];
			gunangle[1] = -matrix.matrix[1][0];
			gunangle[2] = -matrix.matrix[2][0];

			trap->FX_PlayEffectID(cgs.effects.mEmplacedMuzzleFlash, gunpoint, gunangle, -1, -1, qfalse);
		}
		else if (cent->currentState.weapon != WP_EMPLACED_GUN || cent->currentState.eType == ET_NPC)
		{
			int doit = 1;

			if (cent->currentState.eType == ET_NPC &&
				cent->currentState.NPC_class == CLASS_VEHICLE &&
				cent->m_pVehicle)
			{
				//vehicles do nothing for clientside weapon fire events.. at least for now.
				break;
			}

			if (cg.snap->ps.duelInProgress)
			{ // this client is dueling
				if (es->number != cg.snap->ps.clientNum && es->number != cg.snap->ps.duelIndex)
				{	// event did not origniate from one of the duelers
					doit = 0;
				}
			}
			if (doit)
			{
				CG_FireWeapon(cent, qfalse);
			}
		}
		break;

	case EV_ALTFIRE:
		DEBUGNAME("EV_ALTFIRE");
		{
			int doit = 1; // lmo consider duel nonx

			if (cent->currentState.weapon == WP_EMPLACED_GUN)
			{
				//don't do anything for emplaced stuff
				break;
			}

			if (cent->currentState.eType == ET_NPC &&
				cent->currentState.NPC_class == CLASS_VEHICLE &&
				cent->m_pVehicle)
			{
				//vehicles do nothing for clientside weapon fire events.. at least for now.
				break;
			}

			if (cg.snap->ps.duelInProgress)
			{ // this client is dueling
				if (es->number != cg.snap->ps.clientNum && es->number != cg.snap->ps.duelIndex)
				{	// event did not origniate from one of the duelers
					doit = 0;
				}
			}
			if (doit)
			{
				CG_FireWeapon(cent, qtrue);
			}

			//if you just exploded your detpacks and you have no ammo left for them, autoswitch
			if (cg.snap->ps.clientNum == cent->currentState.number &&
				cg.snap->ps.weapon == WP_DET_PACK)
			{
				if (cg.snap->ps.ammo[weaponData[WP_DET_PACK].ammoIndex] == 0)
				{
					CG_OutOfAmmoChange(WP_DET_PACK);
				}
			}
		}
		break;

	case EV_SABER_ATTACK:
		DEBUGNAME("EV_SABER_ATTACK");
		{
			qhandle_t swing_sound = trap->S_RegisterSound(va("sound/weapons/saber/saberhup%i.wav", Q_irand(1, 11)));
			clientInfo_t* client = NULL;
			if (cg_entities[es->number].currentState.eType == ET_NPC)
			{
				client = cg_entities[es->number].npcClient;
			}
			else if (es->number < MAX_CLIENTS)
			{
				client = &cgs.clientinfo[es->number];
			}
			if (client && client->infoValid && client->saber[0].swingSound[0])
			{
				//custom swing sound
				swing_sound = client->saber[0].swingSound[Q_irand(0, 2)];
			}
			trap->S_StartSound(es->pos.trBase, es->number, CHAN_WEAPON, swing_sound);
		}
		break;

	case EV_SABER_HIT:
		DEBUGNAME("EV_SABER_HIT");
		{
			int hit_person_fx_id = cgs.effects.mSaberBloodSparks;
			int hit_person_small_fx_id = cgs.effects.mSaberBloodSparksSmall;
			int hit_person_mid_fx_id = cgs.effects.mSaberBloodSparksMid;
			int hit_other_fx_id = cgs.effects.mSaberCut;
			int hit_droid_fx_id = cgs.effects.mDroidCut;
			int hit_other_fx_id2 = cgs.effects.mSaberBodyHit;
			int hit_sound = trap->S_RegisterSound(va("sound/weapons/saber/saberhit%i.mp3", Q_irand(1, 15)));
			int kill_sound = trap->S_RegisterSound(va("sound/weapons/saber/saberkill%i.wav", Q_irand(1, 17)));

			if (es->otherentity_num2 >= 0
				&& es->otherentity_num2 < ENTITYNUM_NONE)
			{
				//we have a specific person who is causing this effect, see if we should override it with any custom saber effects/sounds
				clientInfo_t* client = NULL;
				if (cg_entities[es->otherentity_num2].currentState.eType == ET_NPC)
				{
					client = cg_entities[es->otherentity_num2].npcClient;
				}
				else if (es->otherentity_num2 < MAX_CLIENTS)
				{
					client = &cgs.clientinfo[es->otherentity_num2];
				}

				if (client && client->infoValid)
				{
					int saberNum = es->weapon;
					int blade_num = es->legsAnim;

					if (WP_SaberBladeUseSecondBladeStyle(&client->saber[saberNum], blade_num))
					{
						//use second blade style values
						if (client->saber[saberNum].hitPersonEffect2)
						{
							//custom hit person effect
							hit_person_fx_id = hit_person_small_fx_id = hit_person_mid_fx_id = client->saber[saberNum].
								hitPersonEffect2;
						}
						if (client->saber[saberNum].hitOtherEffect2)
						{
							//custom hit other effect
							hit_other_fx_id = client->saber[saberNum].hitOtherEffect2;
						}
						if (client->saber[saberNum].hit2Sound[0])
						{
							//custom hit sound
							hit_sound = client->saber[saberNum].hit2Sound[Q_irand(0, 2)];
						}
					}
					else
					{
						//use first blade style values
						if (client->saber[saberNum].hitPersonEffect)
						{
							//custom hit person effect
							if (cent->currentState.botclass == BCLASS_SBD
								|| cent->currentState.botclass == BCLASS_ASSASSIN_DROID
								|| cent->currentState.botclass == BCLASS_BATTLEDROID
								|| cent->currentState.botclass == BCLASS_DROIDEKA
								|| cent->currentState.botclass == BCLASS_SABER_DROID
								|| cent->currentState.botclass == BCLASS_R2D2
								|| cent->currentState.botclass == BCLASS_R5D2
								|| cent->currentState.botclass == BCLASS_PROTOCOL
								|| cent->currentState.botclass == BCLASS_BOBAFETT
								|| cent->currentState.botclass == BCLASS_ROCKETTROOPER
								|| cent->currentState.botclass == BCLASS_MANDOLORIAN
								|| cent->currentState.botclass == BCLASS_MANDOLORIAN1
								|| cent->currentState.botclass == BCLASS_MANDOLORIAN2)
							{
								hit_droid_fx_id = hit_person_small_fx_id = hit_person_mid_fx_id = client->saber[saberNum].
									hitPersonEffect;
							}
							else
							{
								hit_person_fx_id = hit_person_small_fx_id = hit_person_mid_fx_id = client->saber[saberNum].
									hitPersonEffect;
							}
						}
						if (client->saber[saberNum].hitOtherEffect)
						{
							//custom hit other effect
							if (cent->currentState.botclass == BCLASS_SBD
								|| cent->currentState.botclass == BCLASS_ASSASSIN_DROID
								|| cent->currentState.botclass == BCLASS_BATTLEDROID
								|| cent->currentState.botclass == BCLASS_DROIDEKA
								|| cent->currentState.botclass == BCLASS_SABER_DROID
								|| cent->currentState.botclass == BCLASS_R2D2
								|| cent->currentState.botclass == BCLASS_R5D2
								|| cent->currentState.botclass == BCLASS_PROTOCOL
								|| cent->currentState.botclass == BCLASS_BOBAFETT
								|| cent->currentState.botclass == BCLASS_ROCKETTROOPER
								|| cent->currentState.botclass == BCLASS_MANDOLORIAN
								|| cent->currentState.botclass == BCLASS_MANDOLORIAN1
								|| cent->currentState.botclass == BCLASS_MANDOLORIAN2)
							{
								hit_droid_fx_id = client->saber[0].hitOtherEffect;
							}
							else
							{
								hit_other_fx_id = client->saber[0].hitOtherEffect;
							}
						}
						if (client->saber[saberNum].hit_sound[0])
						{
							//custom hit sound
							hit_sound = client->saber[saberNum].hit_sound[Q_irand(0, 2)];
						}
					}
				}
			}

			if (es->eventParm == 16)
			{
				//Make lots of sparks, something special happened
				vec3_t fx_dir;
				VectorCopy(es->angles, fx_dir);
				if (!fx_dir[0] && !fx_dir[1] && !fx_dir[2])
				{
					fx_dir[1] = 1;
				}
				trap->S_StartSound(es->origin, es->number, CHAN_AUTO, hit_sound);

				if (cent->currentState.botclass == BCLASS_SBD
					|| cent->currentState.botclass == BCLASS_ASSASSIN_DROID
					|| cent->currentState.botclass == BCLASS_BATTLEDROID
					|| cent->currentState.botclass == BCLASS_DROIDEKA
					|| cent->currentState.botclass == BCLASS_SABER_DROID
					|| cent->currentState.botclass == BCLASS_R2D2
					|| cent->currentState.botclass == BCLASS_R5D2
					|| cent->currentState.botclass == BCLASS_PROTOCOL
					|| cent->currentState.botclass == BCLASS_BOBAFETT
					|| cent->currentState.botclass == BCLASS_ROCKETTROOPER
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN1
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN2)
				{
					trap->FX_PlayEffectID(hit_droid_fx_id, es->origin, fx_dir, -1, -1, qfalse);
				}
				else
				{
					trap->FX_PlayEffectID(hit_person_fx_id, es->origin, fx_dir, -1, -1, qfalse);
					trap->FX_PlayEffectID(hit_person_fx_id, es->origin, fx_dir, -1, -1, qfalse);
					trap->FX_PlayEffectID(hit_person_fx_id, es->origin, fx_dir, -1, -1, qfalse);
					trap->FX_PlayEffectID(hit_person_fx_id, es->origin, fx_dir, -1, -1, qfalse);
					trap->FX_PlayEffectID(hit_person_fx_id, es->origin, fx_dir, -1, -1, qfalse);
					trap->FX_PlayEffectID(hit_person_fx_id, es->origin, fx_dir, -1, -1, qfalse);
				}
			}
			else if (es->eventParm)
			{
				//hit a person
				vec3_t fx_dir;
				VectorCopy(es->angles, fx_dir);
				if (!fx_dir[0] && !fx_dir[1] && !fx_dir[2])
				{
					fx_dir[1] = 1;
				}
				trap->S_StartSound(es->origin, es->number, CHAN_AUTO, kill_sound);

				if (es->eventParm == 3)
				{
					// moderate or big hits.
					if (cent->currentState.botclass == BCLASS_SBD
						|| cent->currentState.botclass == BCLASS_ASSASSIN_DROID
						|| cent->currentState.botclass == BCLASS_BATTLEDROID
						|| cent->currentState.botclass == BCLASS_DROIDEKA
						|| cent->currentState.botclass == BCLASS_SABER_DROID
						|| cent->currentState.botclass == BCLASS_R2D2
						|| cent->currentState.botclass == BCLASS_R5D2
						|| cent->currentState.botclass == BCLASS_PROTOCOL
						|| cent->currentState.botclass == BCLASS_BOBAFETT
						|| cent->currentState.botclass == BCLASS_ROCKETTROOPER
						|| cent->currentState.botclass == BCLASS_MANDOLORIAN
						|| cent->currentState.botclass == BCLASS_MANDOLORIAN1
						|| cent->currentState.botclass == BCLASS_MANDOLORIAN2)
					{
						trap->FX_PlayEffectID(hit_droid_fx_id, es->origin, fx_dir, -1, -1, qfalse);
					}
					else
					{
						trap->FX_PlayEffectID(hit_person_small_fx_id, es->origin, fx_dir, -1, -1, qfalse);
					}
				}
				else if (es->eventParm == 2)
				{
					// this is for really big hits.
					if (cent->currentState.botclass == BCLASS_SBD
						|| cent->currentState.botclass == BCLASS_ASSASSIN_DROID
						|| cent->currentState.botclass == BCLASS_BATTLEDROID
						|| cent->currentState.botclass == BCLASS_DROIDEKA
						|| cent->currentState.botclass == BCLASS_SABER_DROID
						|| cent->currentState.botclass == BCLASS_R2D2
						|| cent->currentState.botclass == BCLASS_R5D2
						|| cent->currentState.botclass == BCLASS_PROTOCOL
						|| cent->currentState.botclass == BCLASS_BOBAFETT
						|| cent->currentState.botclass == BCLASS_ROCKETTROOPER
						|| cent->currentState.botclass == BCLASS_MANDOLORIAN
						|| cent->currentState.botclass == BCLASS_MANDOLORIAN1
						|| cent->currentState.botclass == BCLASS_MANDOLORIAN2)
					{
						trap->FX_PlayEffectID(hit_droid_fx_id, es->origin, fx_dir, -1, -1, qfalse);
					}
					else
					{
						trap->FX_PlayEffectID(hit_person_mid_fx_id, es->origin, fx_dir, -1, -1, qfalse);
					}
				}
				else
				{
					// this should really just be done in the effect itself, no?
					if (cent->currentState.botclass == BCLASS_SBD
						|| cent->currentState.botclass == BCLASS_ASSASSIN_DROID
						|| cent->currentState.botclass == BCLASS_BATTLEDROID
						|| cent->currentState.botclass == BCLASS_DROIDEKA
						|| cent->currentState.botclass == BCLASS_SABER_DROID
						|| cent->currentState.botclass == BCLASS_R2D2
						|| cent->currentState.botclass == BCLASS_R5D2
						|| cent->currentState.botclass == BCLASS_PROTOCOL
						|| cent->currentState.botclass == BCLASS_BOBAFETT
						|| cent->currentState.botclass == BCLASS_ROCKETTROOPER
						|| cent->currentState.botclass == BCLASS_MANDOLORIAN
						|| cent->currentState.botclass == BCLASS_MANDOLORIAN1
						|| cent->currentState.botclass == BCLASS_MANDOLORIAN2)
					{
						trap->FX_PlayEffectID(hit_droid_fx_id, es->origin, fx_dir, -1, -1, qfalse);
					}
					else
					{
						trap->FX_PlayEffectID(hit_person_fx_id, es->origin, fx_dir, -1, -1, qfalse);
						trap->FX_PlayEffectID(hit_person_fx_id, es->origin, fx_dir, -1, -1, qfalse);
						trap->FX_PlayEffectID(hit_person_fx_id, es->origin, fx_dir, -1, -1, qfalse);
					}
				}
			}
			else
			{
				//hit something else
				vec3_t fx_dir;
				VectorCopy(es->angles, fx_dir);
				if (!fx_dir[0] && !fx_dir[1] && !fx_dir[2])
				{
					fx_dir[1] = 1;
				}
				if (cent->currentState.botclass == BCLASS_SBD
					|| cent->currentState.botclass == BCLASS_ASSASSIN_DROID
					|| cent->currentState.botclass == BCLASS_BATTLEDROID
					|| cent->currentState.botclass == BCLASS_DROIDEKA
					|| cent->currentState.botclass == BCLASS_SABER_DROID
					|| cent->currentState.botclass == BCLASS_R2D2
					|| cent->currentState.botclass == BCLASS_R5D2
					|| cent->currentState.botclass == BCLASS_PROTOCOL
					|| cent->currentState.botclass == BCLASS_BOBAFETT
					|| cent->currentState.botclass == BCLASS_ROCKETTROOPER
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN1
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN2)
				{
					trap->FX_PlayEffectID(hit_droid_fx_id, es->origin, fx_dir, -1, -1, qfalse);
				}
				else
				{
					trap->FX_PlayEffectID(hit_other_fx_id2, es->origin, fx_dir, -1, -1, qfalse);
				}
			}

			//rww - this means we have the number of the ent being hit and the ent that owns the saber doing
			//the hit. This being the case, we can store these indecies and the current time in order to do
			//some visual tricks on the client between frames to make it look like we're actually continuing
			//to hit between server frames.
			if (es->otherentity_num != ENTITYNUM_NONE && es->otherentity_num2 != ENTITYNUM_NONE)
			{
				centity_t* saber_owner;

				saber_owner = &cg_entities[es->otherentity_num2];

				saber_owner->serverSaberHitIndex = es->otherentity_num;
				saber_owner->serverSaberHitTime = cg.time;

				if (es->eventParm)
				{
					saber_owner->serverSaberFleshImpact = qtrue;
				}
				else
				{
					saber_owner->serverSaberFleshImpact = qfalse;
				}
			}
		}
		break;

	case EV_SABER_BLOCK:
		DEBUGNAME("EV_SABER_BLOCK");
		{
			if (es->eventParm)
			{
				//saber block
				qboolean cull_pass = qfalse;
				int block_fxid = cgs.effects.mSaberBlock;
				int perfectblock_fxid = cgs.effects.mSaberprfectBlock;
				qhandle_t block_sound = trap->S_RegisterSound(va("sound/weapons/saber/saberblock%d", Q_irand(1, 90)));
				qhandle_t knock_sound = trap->S_RegisterSound(va("sound/weapons/saber/saberknock%d", Q_irand(1, 4)));

				if (es->otherentity_num2 >= 0 && es->otherentity_num2 < ENTITYNUM_NONE)
				{
					//we have a specific person who is causing this effect, see if we should override it with any custom saber effects/sounds
					clientInfo_t* client = NULL;
					if (cg_entities[es->otherentity_num2].currentState.eType == ET_NPC)
					{
						client = cg_entities[es->otherentity_num2].npcClient;
					}
					else if (es->otherentity_num2 < MAX_CLIENTS)
					{
						client = &cgs.clientinfo[es->otherentity_num2];
					}
					if (client && client->infoValid)
					{
						int saberNum = es->weapon;
						int blade_num = es->legsAnim;

						if (WP_SaberBladeUseSecondBladeStyle(&client->saber[saberNum], blade_num))
						{
							//use second blade style values
							if (client->saber[saberNum].blockEffect2)
							{
								//custom saber block effect
								block_fxid = client->saber[saberNum].blockEffect2;
							}
							if (client->saber[saberNum].block2Sound[0])
							{
								//custom hit sound
								if (cg.snap->ps.fd.blockPoints < BLOCKPOINTS_HALF)
								{
									knock_sound = client->saber[saberNum].block2Sound[Q_irand(0, 2)];
								}
								else
								{
									block_sound = client->saber[saberNum].block2Sound[Q_irand(0, 2)];
								}
							}
						}
						else
						{
							if (client->saber[saberNum].blockEffect)
							{
								//custom saber block effect
								if (cent->currentState.userInt3 & 1 << FLAG_PERFECTBLOCK)
								{
									perfectblock_fxid = client->saber[saberNum].blockEffect;
								}
								else
								{
									block_fxid = client->saber[saberNum].blockEffect;
								}
							}
							if (client->saber[saberNum].blockSound[0])
							{
								//custom hit sound
								if (cg.snap->ps.fd.blockPoints < BLOCKPOINTS_HALF)
								{
									knock_sound = client->saber[saberNum].blockSound[Q_irand(0, 2)];
								}
								else
								{
									block_sound = client->saber[saberNum].blockSound[Q_irand(0, 2)];
								}
							}
						}
					}
				}
				if (cg.mInRMG)
				{
					trace_t tr;
					vec3_t vec_sub;

					VectorSubtract(cg.refdef.vieworg, es->origin, vec_sub);

					if (VectorLength(vec_sub) < 5000)
					{
						CG_Trace(&tr, cg.refdef.vieworg, NULL, NULL, es->origin, ENTITYNUM_NONE, CONTENTS_TERRAIN | CONTENTS_SOLID);

						if (tr.fraction == 1.0 || tr.entityNum < MAX_CLIENTS)
						{
							cull_pass = qtrue;
						}
					}
				}
				else
				{
					cull_pass = qtrue;
				}

				if (cull_pass)
				{
					vec3_t fx_dir;

					VectorCopy(es->angles, fx_dir);
					if (!fx_dir[0] && !fx_dir[1] && !fx_dir[2])
					{
						fx_dir[1] = 1;
					}
					trap->S_StartSound(es->origin, es->number, CHAN_AUTO, block_sound);
					trap->FX_PlayEffectID(block_fxid, es->origin, fx_dir, -1, -1, qfalse);
				}
			}
			else
			{
				//projectile block
				vec3_t fx_dir;
				VectorCopy(es->angles, fx_dir);
				if (!fx_dir[0] && !fx_dir[1] && !fx_dir[2])
				{
					fx_dir[1] = 1;
				}
				if (Q_irand(0, 1))
				{
					trap->FX_PlayEffectID(cgs.effects.mBlasterDeflectpassthrough, es->origin, fx_dir, -1, -1, qfalse);
				}
				else
				{
					trap->FX_PlayEffectID(cgs.effects.mBlasterDeflect, es->origin, fx_dir, -1, -1, qfalse);
				}
			}
		}
		break;

	case EV_SABER_BODY_HIT:
		DEBUGNAME("EV_SABER_BODY_HIT");
		{
			if (es->eventParm)
			{
				// Hit body
				vec3_t fx_dir;
				VectorCopy(es->angles, fx_dir);
				if (!fx_dir[0] && !fx_dir[1] && !fx_dir[2])
				{
					fx_dir[1] = 1;
				}
				if (cent->currentState.botclass == BCLASS_SBD
					|| cent->currentState.botclass == BCLASS_ASSASSIN_DROID
					|| cent->currentState.botclass == BCLASS_BATTLEDROID
					|| cent->currentState.botclass == BCLASS_DROIDEKA
					|| cent->currentState.botclass == BCLASS_SABER_DROID
					|| cent->currentState.botclass == BCLASS_R2D2
					|| cent->currentState.botclass == BCLASS_R5D2
					|| cent->currentState.botclass == BCLASS_PROTOCOL
					|| cent->currentState.botclass == BCLASS_BOBAFETT
					|| cent->currentState.botclass == BCLASS_ROCKETTROOPER
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN1
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN2)
				{
					trap->FX_PlayEffectID(cgs.effects.mDroidtouch, es->origin, fx_dir, -1, -1, qfalse);
				}
				else
				{
					trap->FX_PlayEffectID(cgs.effects.mSaberlimb_Bolton, es->origin, fx_dir, -1, -1, qfalse);
				}
			}
			else
			{
				// Hit bbox
				vec3_t fx_dir;
				VectorCopy(es->angles, fx_dir);
				if (!fx_dir[0] && !fx_dir[1] && !fx_dir[2])
				{
					fx_dir[1] = 1;
				}
				if (cent->currentState.botclass == BCLASS_SBD
					|| cent->currentState.botclass == BCLASS_ASSASSIN_DROID
					|| cent->currentState.botclass == BCLASS_BATTLEDROID
					|| cent->currentState.botclass == BCLASS_DROIDEKA
					|| cent->currentState.botclass == BCLASS_SABER_DROID
					|| cent->currentState.botclass == BCLASS_R2D2
					|| cent->currentState.botclass == BCLASS_R5D2
					|| cent->currentState.botclass == BCLASS_PROTOCOL
					|| cent->currentState.botclass == BCLASS_BOBAFETT
					|| cent->currentState.botclass == BCLASS_ROCKETTROOPER
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN1
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN2)
				{
					trap->FX_PlayEffectID(cgs.effects.mDroidCut, es->origin, fx_dir, -1, -1, qfalse);
				}
				else
				{
					trap->FX_PlayEffectID(cgs.effects.mSaberCut, es->origin, fx_dir, -1, -1, qfalse);
				}
			}
		}
		break;

	case EV_SABER_CLASHFLARE:
		DEBUGNAME("EV_SABER_CLASHFLARE");
		{
			qboolean cull_pass = qfalse;

			if (cg.mInRMG)
			{
				trace_t tr;
				vec3_t vec_sub;

				VectorSubtract(cg.refdef.vieworg, es->origin, vec_sub);

				if (VectorLength(vec_sub) < 5000)
				{
					CG_Trace(&tr, cg.refdef.vieworg, NULL, NULL, es->origin, ENTITYNUM_NONE, CONTENTS_TERRAIN | CONTENTS_SOLID);

					if (tr.fraction == 1.0 || tr.entityNum < MAX_CLIENTS)
					{
						cull_pass = qtrue;
					}
				}
			}
			else
			{
				cull_pass = qtrue;
			}

			if (cull_pass)
			{
				cg_saberFlashTime = cg.time - 50;
				VectorCopy(es->origin, cg_saberFlashPos);
			}
			trap->S_StartSound(es->origin, -1, CHAN_WEAPON, trap->S_RegisterSound(va("sound/weapons/saber/saberhitwall%i", Q_irand(1, 3))));
		}
		break;

	case EV_SABER_UNHOLSTER:
		DEBUGNAME("EV_SABER_UNHOLSTER");
		{
			clientInfo_t* ci = NULL;

			if (es->eType == ET_NPC)
			{
				ci = cg_entities[es->number].npcClient;
			}
			else if (es->number < MAX_CLIENTS)
			{
				ci = &cgs.clientinfo[es->number];
			}

			if (ci)
			{
				if (ci->saber[0].soundOn)
				{
					trap->S_StartSound(NULL, es->number, CHAN_AUTO, ci->saber[0].soundOn);
				}
				if (ci->saber[1].soundOn)
				{
					trap->S_StartSound(NULL, es->number, CHAN_AUTO, ci->saber[1].soundOn);
				}
			}
		}
		break;

	case EV_BECOME_JEDIMASTER:
		DEBUGNAME("EV_SABER_UNHOLSTER");
		{
			trace_t tr;
			vec3_t player_mins = { -15, -15, DEFAULT_MINS_2 + 8 };
			vec3_t player_maxs = { 15, 15, DEFAULT_MAXS_2 };
			vec3_t ang, pos, dpos;

			VectorClear(ang);
			ang[ROLL] = 1;

			VectorCopy(position, dpos);
			dpos[2] -= 4096;

			CG_Trace(&tr, position, player_mins, player_maxs, dpos, es->number, MASK_SOLID);
			VectorCopy(tr.endpos, pos);

			if (tr.fraction == 1)
			{
				break;
			}
			trap->FX_PlayEffectID(cgs.effects.mJediSpawn, pos, ang, -1, -1, qfalse);

			trap->S_StartSound(NULL, es->number, CHAN_AUTO,
				trap->S_RegisterSound("sound/weapons/saber/saberon.wav"));

			if (cg.snap->ps.clientNum == es->number)
			{
				trap->S_StartLocalSound(cgs.media.happyMusic, CHAN_LOCAL);
				CGCam_SetMusicMult(0.3f, 5000);
			}
		}
		break;

	case EV_DISRUPTOR_MAIN_SHOT:
		DEBUGNAME("EV_DISRUPTOR_MAIN_SHOT");
		{
			int doit = 1;

			if (cg.snap->ps.duelInProgress)
			{ // this client is dueling
				if (es->number >= 0 && es->number < MAX_CLIENTS && es->number != cg.snap->ps.clientNum && es->number != cg.snap->ps.duelIndex)
				{	// sound originated from a client, but not one of the duelers
					doit = 0;
				}
				else if (es->number >= MAX_CLIENTS && es->otherentity_num != ENTITYNUM_NONE &&
					es->otherentity_num != cg.snap->ps.clientNum &&
					es->otherentity_num != cg.snap->ps.duelIndex)
				{  // sound generated by an temp entity in a snap, the otherentity_num should be the orinating
				   // client number (by hack!). If otherentity_num is ENTITYNUM_NONE, then it is one of the many sounds
				   // I don't bother filtering, so play it. Otherwise, if not from the duelers, supress it
					doit = 0;
				}
			}
			if (doit)
			{
				if (cent->currentState.eventParm != cg.snap->ps.clientNum || cg.renderingThirdPerson || cg_trueguns.integer
					|| cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE)
				{
					//h4q3ry
					if (cent->currentState.eFlags & EF3_DUAL_WEAPONS)
					{
						CG_GetClientWeaponMuzzleBoltPointduals(cent->currentState.eventParm, cent->currentState.origin2,
							qtrue);
					}
					else
					{
						CG_GetClientWeaponMuzzleBoltPoint(cent->currentState.eventParm, cent->currentState.origin2);
					}
				}
				else
				{
					if (cg.lastFPFlashPoint[0] || cg.lastFPFlashPoint[1] || cg.lastFPFlashPoint[2])
					{
						//get the position of the muzzle flash for the first person weapon model from the last frame
						VectorCopy(cg.lastFPFlashPoint, cent->currentState.origin2);
					}
				}
				FX_DisruptorMainShot(cent->currentState.origin2, cent->lerpOrigin);
			}
		}
		break;

	case EV_DISRUPTOR_SNIPER_SHOT:
		DEBUGNAME("EV_DISRUPTOR_SNIPER_SHOT");
		{
			int doit = 1;

			if (cg.snap->ps.duelInProgress)
			{ // this client is dueling
				if (es->number >= 0 && es->number < MAX_CLIENTS && es->number != cg.snap->ps.clientNum && es->number != cg.snap->ps.duelIndex)
				{	// sound originated from a client, but not one of the duelers
					doit = 0;
				}
				else if (es->number >= MAX_CLIENTS && es->otherentity_num != ENTITYNUM_NONE &&
					es->otherentity_num != cg.snap->ps.clientNum &&
					es->otherentity_num != cg.snap->ps.duelIndex)
				{  // sound generated by an temp entity in a snap, the otherentity_num should be the orinating
				   // client number (by hack!). If otherentity_num is ENTITYNUM_NONE, then it is one of the many sounds
				   // I don't bother filtering, so play it. Otherwise, if not from the duelers, supress it
					doit = 0;
				}
			}
			if (doit)
			{
				if (cent->currentState.eventParm != cg.snap->ps.clientNum ||
					cg.renderingThirdPerson)
				{
					//h4q3ry
					if (cent->currentState.eFlags & EF3_DUAL_WEAPONS)
					{
						CG_GetClientWeaponMuzzleBoltPointduals(cent->currentState.eventParm, cent->currentState.origin2,
							qtrue);
					}
					else
					{
						CG_GetClientWeaponMuzzleBoltPoint(cent->currentState.eventParm, cent->currentState.origin2);
					}
				}
				else
				{
					if (cg.lastFPFlashPoint[0] || cg.lastFPFlashPoint[1] || cg.lastFPFlashPoint[2])
					{
						//get the position of the muzzle flash for the first person weapon model from the last frame
						VectorCopy(cg.lastFPFlashPoint, cent->currentState.origin2);
					}
				}
				FX_DisruptorAltShot(cent->currentState.origin2, cent->lerpOrigin, cent->currentState.shouldtarget);
			}
		}
		break;

	case EV_DISRUPTOR_SNIPER_MISS:
		DEBUGNAME("EV_DISRUPTOR_SNIPER_MISS");
		ByteToDir(es->eventParm, dir);
		if (es->weapon)
		{
			//primary
			FX_DisruptorHitWall(cent->lerpOrigin, dir);
		}
		else
		{
			//secondary
			FX_DisruptorAltMiss(cent->lerpOrigin, dir);
		}
		break;

	case EV_DISRUPTOR_HIT:
		DEBUGNAME("EV_DISRUPTOR_HIT");
		{
			int doit = 1;

			if (cg.snap->ps.duelInProgress)
			{ // this client is dueling
				if (es->number >= 0 && es->number < MAX_CLIENTS && es->number != cg.snap->ps.clientNum && es->number != cg.snap->ps.duelIndex)
				{	// sound originated from a client, but not one of the duelers
					doit = 0;
				}
				else if (es->number >= MAX_CLIENTS && es->otherentity_num != ENTITYNUM_NONE &&
					es->otherentity_num != cg.snap->ps.clientNum &&
					es->otherentity_num != cg.snap->ps.duelIndex)
				{  // sound generated by an temp entity in a snap, the otherentity_num should be the orinating
				   // client number (by hack!). If otherentity_num is ENTITYNUM_NONE, then it is one of the many sounds
				   // I don't bother filtering, so play it. Otherwise, if not from the duelers, supress it
					doit = 0;
				}
			}
			if (doit)
			{
				ByteToDir(es->eventParm, dir);
				if (es->weapon)
				{
					//client
					FX_DisruptorHitPlayer(cent->lerpOrigin, dir, qtrue);
				}
				else
				{
					//non-client
					FX_DisruptorHitWall(cent->lerpOrigin, dir);
				}
			}
		}
		break;

	case EV_DISRUPTOR_ZOOMSOUND:
		DEBUGNAME("EV_DISRUPTOR_ZOOMSOUND");
		if (es->number == cg.snap->ps.clientNum)
		{
			if (cg.snap->ps.zoomMode)
			{
				trap->S_StartLocalSound(trap->S_RegisterSound("sound/weapons/disruptor/zoomstart.wav"), CHAN_AUTO);
			}
			else
			{
				trap->S_StartLocalSound(trap->S_RegisterSound("sound/weapons/disruptor/zoomend.wav"), CHAN_AUTO);
			}
		}
		break;
	case EV_PREDEFSOUND:
		DEBUGNAME("EV_PREDEFSOUND");
		{
			int s_id = -1;

			switch (es->eventParm)
			{
			case PDSOUND_PROTECTHIT:
				s_id = trap->S_RegisterSound("sound/weapons/force/protecthit.mp3");
				break;
			case PDSOUND_PROTECT:
				s_id = trap->S_RegisterSound("sound/weapons/force/protect.mp3");
				break;
			case PDSOUND_ABSORBHIT:
				s_id = trap->S_RegisterSound("sound/weapons/force/absorbhit.mp3");
				if (es->trickedentindex >= 0 && es->trickedentindex < MAX_CLIENTS)
				{
					int clnum = es->trickedentindex;

					cg_entities[clnum].teamPowerEffectTime = cg.time + 1000;
					cg_entities[clnum].teamPowerType = 3;
				}
				break;
			case PDSOUND_ABSORB:
				s_id = trap->S_RegisterSound("sound/weapons/force/absorb.mp3");
				break;
			case PDSOUND_FORCEJUMP:
				s_id = trap->S_RegisterSound("sound/weapons/force/jump.mp3");
				break;
			case PDSOUND_FORCEGRIP:
				s_id = trap->S_RegisterSound("sound/weapons/force/grip.mp3");
				break;
			default:
				break;
			}

			if (s_id != 1)
			{
				trap->S_StartSound(es->origin, es->number, CHAN_AUTO, s_id);
			}
		}
		break;

	case EV_TEAM_POWER:
		DEBUGNAME("EV_TEAM_POWER");
		{
			int clnum = 0;

			while (clnum < MAX_CLIENTS)
			{
				if (CG_InClientBitflags(es, clnum))
				{
					if (es->eventParm == 1)
					{
						//eventParm 1 is heal
						trap->S_StartSound(NULL, clnum, CHAN_AUTO, cgs.media.teamHealSound);
						cg_entities[clnum].teamPowerEffectTime = cg.time + 1000;
						cg_entities[clnum].teamPowerType = 1;
					}
					else
					{
						//eventParm 2 is force regen
						trap->S_StartSound(NULL, clnum, CHAN_AUTO, cgs.media.teamRegenSound);
						cg_entities[clnum].teamPowerEffectTime = cg.time + 1000;
						cg_entities[clnum].teamPowerType = 0;
					}
				}
				clnum++;
			}
		}
		break;

	case EV_SCREENSHAKE:
		DEBUGNAME("EV_SCREENSHAKE");
		if (!es->modelIndex || cg.predictedPlayerState.clientNum == es->modelIndex - 1)
		{
			CGCam_Shake(es->angles[0], es->time);
		}
		break;

	case EV_BLOCKSHAKE:
		DEBUGNAME("EV_BLOCKSHAKE");

		/*if (es->owner == cg.predictedPlayerState.clientNum)
		{
			CGCam_BlockShake(es->angles[0], es->time);
		}*/

		if (!es->modelIndex || cg.predictedPlayerState.clientNum == es->modelIndex - 1)
		{
			CGCam_BlockShake(es->angles[0], es->time);
		}
		break;

	case EV_LOCALTIMER:
		DEBUGNAME("EV_LOCALTIMER");
		if (es->owner == cg.predictedPlayerState.clientNum)
		{
			CG_LocalTimingBar(es->time, es->time2);
		}
		break;
	case EV_USE_ITEM0:
		DEBUGNAME("EV_USE_ITEM0");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM1:
		DEBUGNAME("EV_USE_ITEM1");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM2:
		DEBUGNAME("EV_USE_ITEM2");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM3:
		DEBUGNAME("EV_USE_ITEM3");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM4:
		DEBUGNAME("EV_USE_ITEM4");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM5:
		DEBUGNAME("EV_USE_ITEM5");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM6:
		DEBUGNAME("EV_USE_ITEM6");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM7:
		DEBUGNAME("EV_USE_ITEM7");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM8:
		DEBUGNAME("EV_USE_ITEM8");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM9:
		DEBUGNAME("EV_USE_ITEM9");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM10:
		DEBUGNAME("EV_USE_ITEM10");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM11:
		DEBUGNAME("EV_USE_ITEM11");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM12:
		DEBUGNAME("EV_USE_ITEM12");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM13:
		DEBUGNAME("EV_USE_ITEM13");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM14:
		DEBUGNAME("EV_USE_ITEM14");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM15:
		DEBUGNAME("EV_USE_ITEM15");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM16:
		DEBUGNAME("EV_USE_ITEM16");
		CG_UseItem(cent);
		break;

	case EV_ITEMUSEFAIL:
		DEBUGNAME("EV_ITEMUSEFAIL");
		if (cg.snap->ps.clientNum == es->number)
		{
			char* ps_string_ed_ref = NULL;

			switch (es->eventParm)
			{
			case SENTRY_NOROOM:
				ps_string_ed_ref = (char*)CG_GetStringEdString("MP_INGAME", "SENTRY_NOROOM");
				break;
			case SENTRY_ALREADYPLACED:
				ps_string_ed_ref = (char*)CG_GetStringEdString("MP_INGAME", "SENTRY_ALREADYPLACED");
				break;
			case SHIELD_NOROOM:
				ps_string_ed_ref = (char*)CG_GetStringEdString("MP_INGAME", "SHIELD_NOROOM");
				break;
			case SEEKER_ALREADYDEPLOYED:
				ps_string_ed_ref = (char*)CG_GetStringEdString("MP_INGAME", "SEEKER_ALREADYDEPLOYED");
				break;
			default:
				break;
			}

			if (!ps_string_ed_ref)
			{
				break;
			}

			Com_Printf("%s\n", ps_string_ed_ref);
		}
		break;

		//=================================================================

		//
		// other events
		//
	case EV_PLAYER_TELEPORT_IN:
		DEBUGNAME("EV_PLAYER_TELEPORT_IN");
		{
			trace_t tr;
			vec3_t player_mins = { -15, -15, DEFAULT_MINS_2 + 8 };
			vec3_t player_maxs = { 15, 15, DEFAULT_MAXS_2 };
			vec3_t ang, pos, dpos;

			int doit = 1; //consider NonX (lmo)

			if (cg.snap->ps.duelInProgress)
			{ // this client is dueling
				if (es->number >= 0 && es->number < MAX_CLIENTS && es->number != cg.snap->ps.clientNum && es->number != cg.snap->ps.duelIndex)
				{	// sound originated from a client, but not one of the duelers
					doit = 0;
				}
				else if (es->number >= MAX_CLIENTS && es->otherentity_num != ENTITYNUM_NONE &&
					es->otherentity_num != cg.snap->ps.clientNum &&
					es->otherentity_num != cg.snap->ps.duelIndex)
				{  // sound generated by an temp entity in a snap, the otherentity_num should be the orinating
				   // client number (by hack!). If otherentity_num is ENTITYNUM_NONE, then it is one of the many sounds
				   // I don't bother filtering, so play it. Otherwise, if not from the duelers, supress it
					doit = 0;
				}
			}
			if (doit)
			{
				VectorClear(ang);
				ang[ROLL] = 1;

				VectorCopy(position, dpos);
				dpos[2] -= 4096;

				CG_Trace(&tr, position, player_mins, player_maxs, dpos, es->number, MASK_SOLID);
				VectorCopy(tr.endpos, pos);

				trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.teleInSound);

				if (tr.fraction == 1)
				{
					break;
				}
				trap->FX_PlayEffectID(cgs.effects.mSpawn, pos, ang, -1, -1, qfalse);
			}
		}
		break;

	case EV_PLAYER_TELEPORT_OUT:
		DEBUGNAME("EV_PLAYER_TELEPORT_OUT");
		{
			trace_t tr;
			vec3_t player_mins = { -15, -15, DEFAULT_MINS_2 + 8 };
			vec3_t player_maxs = { 15, 15, DEFAULT_MAXS_2 };
			vec3_t ang, pos, dpos;

			int doit = 1; //consider NonX (lmo)

			if (cg.snap->ps.duelInProgress)
			{ // this client is dueling
				if (es->number >= 0 && es->number < MAX_CLIENTS && es->number != cg.snap->ps.clientNum && es->number != cg.snap->ps.duelIndex)
				{	// sound originated from a client, but not one of the duelers
					doit = 0;
				}
				else if (es->number >= MAX_CLIENTS && es->otherentity_num != ENTITYNUM_NONE &&
					es->otherentity_num != cg.snap->ps.clientNum &&
					es->otherentity_num != cg.snap->ps.duelIndex)
				{  // sound generated by an temp entity in a snap, the otherentity_num should be the orinating
				   // client number (by hack!). If otherentity_num is ENTITYNUM_NONE, then it is one of the many sounds
				   // I don't bother filtering, so play it. Otherwise, if not from the duelers, supress it
					doit = 0;
				}
			}
			if (doit)
			{
				VectorClear(ang);
				ang[ROLL] = 1;

				VectorCopy(position, dpos);
				dpos[2] -= 4096;

				CG_Trace(&tr, position, player_mins, player_maxs, dpos, es->number, MASK_SOLID);
				VectorCopy(tr.endpos, pos);

				trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.teleOutSound);

				if (tr.fraction == 1)
				{
					break;
				}
				trap->FX_PlayEffectID(cgs.effects.mSpawn, pos, ang, -1, -1, qfalse);
			}
		}
		break;

	case EV_ITEM_POP:
		DEBUGNAME("EV_ITEM_POP");
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.respawnSound);
		break;
	case EV_ITEM_RESPAWN:
		DEBUGNAME("EV_ITEM_RESPAWN");
		cent->miscTime = cg.time; // scale up from this
		trap->S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.respawnSound);
		break;

	case EV_GRENADE_BOUNCE:
		ByteToDir(es->eventParm, dir);
		DEBUGNAME("EV_GRENADE_BOUNCE");
		CG_BounceEffect(es->weapon, position, dir);
		break;

	case EV_SCOREPLUM:
		DEBUGNAME("EV_SCOREPLUM");
		CG_ScorePlum(cent->currentState.otherentity_num, cent->lerpOrigin, cent->currentState.time);
		break;

	case EV_CTFMESSAGE:
		DEBUGNAME("EV_CTFMESSAGE");
		CG_GetCTFMessageEvent(es);
		break;

	case EV_BODYFADE:
		if (es->eType != ET_BODY)
		{
			assert(!"EV_BODYFADE event from a non-corpse");
			break;
		}

		if (cent->ghoul2 && trap->G2_HaveWeGhoul2Models(cent->ghoul2))
		{
			//turn the inside of the face off, to avoid showing the mouth when we start alpha fading the corpse
			trap->G2API_SetSurfaceOnOff(cent->ghoul2, "head_eyes_mouth", 0x00000002);
		}

		cent->bodyFadeTime = cg.time + 60000;
		break;

		//
		// siege gameplay events
		//
	case EV_SIEGE_ROUNDOVER:
		DEBUGNAME("EV_SIEGE_ROUNDOVER");
		CG_SiegeRoundOver(&cg_entities[cent->currentState.weapon], cent->currentState.eventParm);
		break;
	case EV_SIEGE_OBJECTIVECOMPLETE:
		DEBUGNAME("EV_SIEGE_OBJECTIVECOMPLETE");
		CG_SiegeObjectiveCompleted(&cg_entities[cent->currentState.weapon], cent->currentState.eventParm,
			cent->currentState.trickedentindex);
		break;

	case EV_DESTROY_GHOUL2_INSTANCE:
		DEBUGNAME("EV_DESTROY_GHOUL2_INSTANCE");
		if (cg_entities[es->eventParm].ghoul2 && trap->G2_HaveWeGhoul2Models(cg_entities[es->eventParm].ghoul2))
		{
			if (es->eventParm < MAX_CLIENTS)
			{
				//You try to do very bad thing!
#ifdef _DEBUG
				Com_Printf("WARNING: Tried to kill a client ghoul2 instance with a server event!\n");
#endif
				break;
			}
			trap->G2API_CleanGhoul2Models(&cg_entities[es->eventParm].ghoul2);
		}
		break;

	case EV_DESTROY_WEAPON_MODEL:
		DEBUGNAME("EV_DESTROY_WEAPON_MODEL");
		if (cg_entities[es->eventParm].ghoul2 && trap->G2_HaveWeGhoul2Models(cg_entities[es->eventParm].ghoul2) &&
			trap->G2API_HasGhoul2ModelOnIndex(&cg_entities[es->eventParm].ghoul2, 1))
		{
			trap->G2API_RemoveGhoul2Model(&cg_entities[es->eventParm].ghoul2, 1);
		}
		break;

	case EV_GIVE_NEW_RANK:
		DEBUGNAME("EV_GIVE_NEW_RANK");
		if (es->trickedentindex == cg.snap->ps.clientNum)
		{
			trap->Cvar_Set("ui_rankChange", va("%i", es->eventParm));

			trap->Cvar_Set("ui_myteam", va("%i", es->bolt2));

			if (!(trap->Key_GetCatcher() & KEYCATCH_UI) && !es->bolt1)
			{
				trap->OpenUIMenu(UIMENU_PLAYERCONFIG);
			}
		}
		break;

	case EV_SET_FREE_SABER:
		DEBUGNAME("EV_SET_FREE_SABER");

		trap->Cvar_Set("ui_freeSaber", va("%i", es->eventParm));
		break;

	case EV_SET_FORCE_DISABLE:
		DEBUGNAME("EV_SET_FORCE_DISABLE");

		trap->Cvar_Set("ui_forcePowerDisable", va("%i", es->eventParm));
		break;

		//
		// missile impacts
		//
	case EV_CONC_ALT_IMPACT:
		DEBUGNAME("EV_CONC_ALT_IMPACT");
		{
			float dist;
			float shot_dist = VectorNormalize(es->angles);
			vec3_t spot;

			for (dist = 0.0f; dist < shot_dist; dist += 64.0f)
			{
				//one effect would be.. a whole lot better
				VectorMA(es->origin2, dist, es->angles, spot);
				trap->FX_PlayEffectID(cgs.effects.mConcussionAltRing, spot, es->angles2, -1, -1, qfalse);
			}

			ByteToDir(es->eventParm, dir);
			cg_missile_hit_wall(WP_CONCUSSION, position, dir, qtrue, 0);

			FX_ConcAltShot(es->origin2, spot);

			//steal the bezier effect from the disruptor
			FX_DisruptorAltMiss(position, dir);
		}
		break;

	case EV_MISSILE_STICK:
		DEBUGNAME("EV_MISSILE_STICK");
		break;

	case EV_MISSILE_HIT:
		DEBUGNAME("EV_MISSILE_HIT");
		{
			int doit = 1; //consider NonX (lmo)
			ByteToDir(es->eventParm, dir);

			if (cg.snap->ps.duelInProgress)
			{ // this client is dueling
				// note special encoding:
				//  otherentity_num has owner number
				//  otherentity_num2 has hit entitiy number
				if (es->otherentity_num != cg.snap->ps.clientNum && es->otherentity_num != cg.snap->ps.duelIndex)
				{
					doit = 0;
				}
			}
			if (doit) {
				if (es->emplacedOwner)
				{
					//hack: this is an index to a custom effect to use
					trap->FX_PlayEffectID(cgs.gameEffects[es->emplacedOwner], position, dir, -1, -1, qfalse);
				}
				else if (CG_VehicleWeaponImpact(cent))
				{
					//a vehicle missile that uses an overridden impact effect...
				}
				else if (cent->currentState.eFlags & EF_ALT_FIRING)
				{
					cg_missile_hit_player(es->weapon, position, dir, qtrue);
				}
				else
				{
					cg_missile_hit_player(es->weapon, position, dir, qfalse);
				}

				if (cg_ghoul2Marks.integer &&
					es->trickedentindex)
				{
					//flag to place a ghoul2 mark
					CG_G2MarkEvent(es);
				}
			}
		}
		break;

	case EV_MISSILE_MISS:
		DEBUGNAME("EV_MISSILE_MISS");
		{
			int doit = 1; //consider NonX (lmo)

			ByteToDir(es->eventParm, dir);

			if (cg.snap->ps.duelInProgress)
			{ // this client is dueling
				// note special encoding:
				//  otherentity_num has owner number
				//  otherentity_num2 has hit entitiy number
				if (es->otherentity_num != cg.snap->ps.clientNum && es->otherentity_num != cg.snap->ps.duelIndex)
				{
					doit = 0;
				}
			}
			if (doit)
			{
				if (es->emplacedOwner)
				{
					//hack: this is an index to a custom effect to use
					trap->FX_PlayEffectID(cgs.gameEffects[es->emplacedOwner], position, dir, -1, -1, qfalse);
				}
				else if (CG_VehicleWeaponImpact(cent))
				{
					//a vehicle missile that used an overridden impact effect...
				}
				else if (cent->currentState.eFlags & EF_ALT_FIRING)
				{
					cg_missile_hit_wall(es->weapon, position, dir, qtrue, es->generic1);
				}
				else
				{
					cg_missile_hit_wall(es->weapon, position, dir, qfalse, 0);
				}

				if (cg_ghoul2Marks.integer &&
					es->trickedentindex)
				{
					//flag to place a ghoul2 mark
					CG_G2MarkEvent(es);
				}
			}
		}
		break;

	case EV_MISSILE_MISS_METAL:
		DEBUGNAME("EV_MISSILE_MISS_METAL");
		{
			int doit = 1; //consider NonX (lmo)
			ByteToDir(es->eventParm, dir);

			if (cg.snap->ps.duelInProgress)
			{ // this client is dueling
				// note special encoding:
				//  otherentity_num has owner number
				//  otherentity_num2 has hit entitiy number
				if (es->otherentity_num != cg.snap->ps.clientNum && es->otherentity_num != cg.snap->ps.duelIndex)
				{
					doit = 0;
				}
			}
			if (doit)
			{
				if (es->emplacedOwner)
				{
					//hack: this is an index to a custom effect to use
					trap->FX_PlayEffectID(cgs.gameEffects[es->emplacedOwner], position, dir, -1, -1, qfalse);
				}
				else if (CG_VehicleWeaponImpact(cent))
				{
					//a vehicle missile that used an overridden impact effect...
				}
				else if (cent->currentState.eFlags & EF_ALT_FIRING)
				{
					cg_missile_hit_wall(es->weapon, position, dir, qtrue, es->generic1);
				}
				else
				{
					cg_missile_hit_wall(es->weapon, position, dir, qfalse, 0);
				}
			}
		}
		break;

	case EV_PLAY_EFFECT:
		DEBUGNAME("EV_PLAY_EFFECT");
		{
			int doit = 1;

			if (cg.snap->ps.duelInProgress)
			{ // this client is dueling
				// note special encoding:
				//  otherentity_num has owner number
				//  if owner num is ENTITYNUM_NONE then always play it
				if (es->otherentity_num != ENTITYNUM_NONE &&
					es->otherentity_num != cg.snap->ps.clientNum &&
					es->otherentity_num != cg.snap->ps.duelIndex)
				{
					doit = 0;
				}
			}
			if (doit)
			{
				switch (es->eventParm)
				{
					//it isn't a hack, it's ingenuity!
				case EFFECT_SMOKE:
					e_id = cgs.effects.mEmplacedDeadSmoke;
					break;
				case EFFECT_EXPLOSION:
					e_id = cgs.effects.mEmplacedExplode;
					break;
				case EFFECT_EXPLOSION_PAS:
					e_id = cgs.effects.mTurretExplode;
					break;
				case EFFECT_SPARK_EXPLOSION:
					e_id = cgs.effects.mSparkExplosion;
					break;
				case EFFECT_EXPLOSION_TRIPMINE:
					e_id = cgs.effects.mTripmineExplosion;
					break;
				case EFFECT_EXPLOSION_DETPACK:
					e_id = cgs.effects.mDetpackExplosion;
					break;
				case EFFECT_EXPLOSION_FLECHETTE:
					e_id = cgs.effects.mFlechetteAltBlow;
					break;
				case EFFECT_STUNHIT:
					e_id = cgs.effects.mStunBatonFleshImpact;
					break;
				case EFFECT_EXPLOSION_DEMP2ALT:
					FX_DEMP2_AltDetonate(cent->lerpOrigin, es->weapon);
					e_id = cgs.effects.mAltDetonate;
					break;
				case EFFECT_EXPLOSION_TURRET:
					e_id = cgs.effects.mTurretExplode;
					break;
				case EFFECT_SPARKS:
					e_id = cgs.effects.mSparksExplodeNoSound;
					break;
				case EFFECT_WATER_SPLASH:
					e_id = cgs.effects.waterSplash;
					break;
				case EFFECT_ACID_SPLASH:
					e_id = cgs.effects.acidSplash;
					break;
				case EFFECT_LAVA_SPLASH:
					e_id = cgs.effects.lavaSplash;
					break;
				case EFFECT_LANDING_MUD:
					e_id = cgs.effects.landingMud;
					break;
				case EFFECT_LANDING_SAND:
					e_id = cgs.effects.landingSand;
					break;
				case EFFECT_LANDING_DIRT:
					e_id = cgs.effects.landingDirt;
					break;
				case EFFECT_LANDING_SNOW:
					e_id = cgs.effects.landingSnow;
					break;
				case EFFECT_LANDING_GRAVEL:
					e_id = cgs.effects.landingGravel;
					break;
				default:
					e_id = -1;
					break;
				}

				if (e_id != -1)
				{
					vec3_t fx_dir;

					VectorCopy(es->angles, fx_dir);

					if (!fx_dir[0] && !fx_dir[1] && !fx_dir[2])
					{
						fx_dir[1] = 1;
					}

					trap->FX_PlayEffectID(e_id, es->origin, fx_dir, -1, -1, qfalse);
				}
			}
		}
		break;

	case EV_PLAY_EFFECT_BOLTED:
		DEBUGNAME("EV_PLAY_EFFECT_BOLTED");
		{
			centity_t* effect_on = &cg_entities[es->owner];

			int doit = 1;

			if (cg.snap->ps.duelInProgress)
			{ // this client is dueling
				// note special encoding:
				//  otherentity_num has owner number
				//  if owner num is ENTITYNUM_NONE then always play it
				if (es->otherentity_num != ENTITYNUM_NONE &&
					es->otherentity_num != cg.snap->ps.clientNum &&
					es->otherentity_num != cg.snap->ps.duelIndex)
				{
					doit = 0;
				}
			}
			if (doit)
			{
				if (!effect_on->ghoul2) // don't play bolted effect if no ghoul on the entity
					break;

				e_id = 0;

				//if the effect is already registered go ahead grab it
				if (cgs.gameEffects[es->eventParm])
					e_id = cgs.gameEffects[es->eventParm];
				else
				{
					//else it must be registered before using it
					s = CG_ConfigString(CS_EFFECTS + es->eventParm);
					if (s && s[0])
						e_id = trap->FX_RegisterEffect(s);
				}

				if (es->bolt1 == -1 || !e_id) //we don't have this particular bone or effect so can't play it
					break;

				//attach the effect on the entity
				trap->FX_PlayBoltedEffectID(e_id, es->origin, effect_on->ghoul2,
					es->generic1, es->owner, 0, 0, qtrue);
			}
		}
		break;

	case EV_PLAY_EFFECT_ID:
	case EV_PLAY_PORTAL_EFFECT_ID:
		DEBUGNAME("EV_PLAY_EFFECT_ID");
		{
			vec3_t fx_dir;
			qboolean portal_effect = qfalse;
			int efx_index = 0;

			if (event == EV_PLAY_PORTAL_EFFECT_ID)
			{
				//This effect should only be played inside sky portals.
				portal_effect = qtrue;
			}

			AngleVectors(es->angles, fx_dir, 0, 0);

			if (!fx_dir[0] && !fx_dir[1] && !fx_dir[2])
			{
				fx_dir[1] = 1;
			}

			if (cgs.gameEffects[es->eventParm])
			{
				efx_index = cgs.gameEffects[es->eventParm];
			}
			else
			{
				s = CG_ConfigString(CS_EFFECTS + es->eventParm);
				if (s && s[0])
				{
					efx_index = trap->FX_RegisterEffect(s);
				}
			}

			if (efx_index)
			{
				if (portal_effect)
				{
					trap->FX_PlayEffectID(efx_index, position, fx_dir, -1, -1, qtrue);
				}
				else
				{
					trap->FX_PlayEffectID(efx_index, position, fx_dir, -1, -1, qfalse);
				}
			}
		}
		break;

	case EV_PLAYDOORSOUND:
		CG_PlayDoorSound(cent, es->eventParm);
		break;
	case EV_PLAYDOORLOOPSOUND:
		CG_PlayDoorLoopSound(cent);
		break;
	case EV_BMODEL_SOUND:
		DEBUGNAME("EV_BMODEL_SOUND");
		{
			sfxHandle_t sfx;
			const char* sound_set;

			sound_set = CG_ConfigString(CS_AMBIENT_SET + es->soundSetIndex);

			if (!sound_set || !sound_set[0])
			{
				break;
			}

			sfx = trap->AS_GetBModelSound(sound_set, es->eventParm);

			if (sfx == -1)
			{
				break;
			}

			trap->S_StartSound(NULL, es->number, CHAN_AUTO, sfx);
		}
		break;

	case EV_MUTE_SOUND:
		DEBUGNAME("EV_MUTE_SOUND");
		if (cg_entities[es->trickedentindex2].currentState.eFlags & EF_SOUNDTRACKER)
		{
			cg_entities[es->trickedentindex2].currentState.eFlags &= ~EF_SOUNDTRACKER;
		}
		trap->S_MuteSound(es->trickedentindex2, es->trickedentindex);
		CG_S_StopLoopingSound(es->trickedentindex2, -1);
		break;

	case EV_VOICECMD_SOUND:
		DEBUGNAME("EV_VOICECMD_SOUND");
		if (es->groundEntityNum < MAX_CLIENTS && es->groundEntityNum >= 0)
		{
			int entnum = es->groundEntityNum;
			sfxHandle_t sfx = cgs.gameSounds[es->eventParm];
			clientInfo_t* ci = &cgs.clientinfo[entnum];
			centity_t* v_chat_ent = &cg_entities[entnum];
			char descr[1024] = { 0 };

			Q_strncpyz(descr, CG_GetStringForVoiceSound(CG_ConfigString(CS_SOUNDS + es->eventParm)), sizeof descr);

			if (!sfx)
			{
				s = CG_ConfigString(CS_SOUNDS + es->eventParm);
				sfx = CG_CustomSound(entnum, s);
			}

			if (sfx)
			{
				if (entnum != cg.predictedPlayerState.clientNum)
				{
					//play on the head as well to simulate hearing in radio and in world
					if (ci->team == cg.predictedPlayerState.persistant[PERS_TEAM])
					{
						//don't hear it if this person is on the other team, but they can still
						//hear it in the world spot.
						trap->S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_MENU1, sfx);
					}
				}
				if (ci->team == cg.predictedPlayerState.persistant[PERS_TEAM])
				{
					//add to the chat box
					//hear it in the world spot.
					char vchatstr[1024] = { 0 };
					Q_strncpyz(vchatstr, va("<%s^7: %s>\n", ci->name, descr), sizeof vchatstr);
					CG_ChatBox_AddString(vchatstr);
					trap->Print("*%s", vchatstr);
				}

				//and play in world for everyone
				trap->S_StartSound(NULL, entnum, CHAN_VOICE, sfx);
				v_chat_ent->vChatTime = cg.time + 1000;
			}
		}
		break;

	case EV_GENERAL_SOUND:
		DEBUGNAME("EV_GENERAL_SOUND");
		if (es->saberEntityNum == TRACK_CHANNEL_2 || es->saberEntityNum == TRACK_CHANNEL_3 ||
			es->saberEntityNum == TRACK_CHANNEL_5)
		{
			//channels 2 and 3 are for speed and rage, 5 for sight
			if (cgs.gameSounds[es->eventParm])
			{
				CG_S_AddRealLoopingSound(es->number, es->pos.trBase, vec3_origin, cgs.gameSounds[es->eventParm]);
			}
		}
		else
		{
			if (cgs.gameSounds[es->eventParm])
			{
				trap->S_StartSound(NULL, es->number, es->saberEntityNum, cgs.gameSounds[es->eventParm]);
			}
			else
			{
				s = CG_ConfigString(CS_SOUNDS + es->eventParm);
				trap->S_StartSound(NULL, es->number, es->saberEntityNum, CG_CustomSound(es->number, s));
			}
		}
		break;

	case EV_GLOBAL_SOUND: // play from the player's head so it never diminishes
		DEBUGNAME("EV_GLOBAL_SOUND");
		if (cgs.gameSounds[es->eventParm])
		{
			trap->S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_MENU1, cgs.gameSounds[es->eventParm]);
		}
		else
		{
			s = CG_ConfigString(CS_SOUNDS + es->eventParm);
			trap->S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_MENU1, CG_CustomSound(es->number, s));
		}
		break;

	case EV_GLOBAL_TEAM_SOUND: // play from the player's head so it never diminishes
	{
		DEBUGNAME("EV_GLOBAL_TEAM_SOUND");
		switch (es->eventParm)
		{
		case GTS_RED_CAPTURE: // CTF: red team captured the blue flag, 1FCTF: red team captured the neutral flag
			break;
		case GTS_BLUE_CAPTURE:
			// CTF: blue team captured the red flag, 1FCTF: blue team captured the neutral flag
			break;
		case GTS_RED_RETURN: // CTF: blue flag returned, 1FCTF: never used
			if (cgs.gametype == GT_CTY)
			{
				CG_AddBufferedSound(cgs.media.blueYsalReturnedSound);
			}
			else
			{
				CG_AddBufferedSound(cgs.media.blueFlagReturnedSound);
			}
			break;
		case GTS_BLUE_RETURN: // CTF red flag returned, 1FCTF: neutral flag returned
			if (cgs.gametype == GT_CTY)
			{
				CG_AddBufferedSound(cgs.media.redYsalReturnedSound);
			}
			else
			{
				CG_AddBufferedSound(cgs.media.redFlagReturnedSound);
			}
			break;

		case GTS_RED_TAKEN: // CTF: red team took blue flag, 1FCTF: blue team took the neutral flag
			// if this player picked up the flag then a sound is played in CG_CheckLocalSounds
			if (cgs.gametype == GT_CTY)
			{
				CG_AddBufferedSound(cgs.media.redTookYsalSound);
			}
			else
			{
				CG_AddBufferedSound(cgs.media.redTookFlagSound);
			}
			break;
		case GTS_BLUE_TAKEN: // CTF: blue team took the red flag, 1FCTF red team took the neutral flag
			// if this player picked up the flag then a sound is played in CG_CheckLocalSounds
			if (cgs.gametype == GT_CTY)
			{
				CG_AddBufferedSound(cgs.media.blueTookYsalSound);
			}
			else
			{
				CG_AddBufferedSound(cgs.media.blueTookFlagSound);
			}
			break;
		case GTS_REDTEAM_SCORED:
			CG_AddBufferedSound(cgs.media.redScoredSound);
			break;
		case GTS_BLUETEAM_SCORED:
			CG_AddBufferedSound(cgs.media.blueScoredSound);
			break;
		case GTS_REDTEAM_TOOK_LEAD:
			CG_AddBufferedSound(cgs.media.redLeadsSound);
			break;
		case GTS_BLUETEAM_TOOK_LEAD:
			CG_AddBufferedSound(cgs.media.blueLeadsSound);
			break;
		case GTS_TEAMS_ARE_TIED:
			CG_AddBufferedSound(cgs.media.teamsTiedSound);
			break;
		default:
			break;
		}
		break;
	}

	case EV_ENTITY_SOUND:
		DEBUGNAME("EV_ENTITY_SOUND");
		//somewhat of a hack - weapon is the caller entity's index, trickedentindex is the proper sound channel
		if (es->trickedentindex == CHAN_VOICE || es->trickedentindex == CHAN_VOICE_GLOBAL)
		{
			int gender;

			if (es->clientNum < MAX_CLIENTS && cg_entities[es->clientNum].currentValid)
			{
				//already a player
				gender = cgs.clientinfo[es->clientNum].gender;
			}
			else if (cg_entities[es->clientNum].currentState.eType == ET_NPC)
			{
				//NPC
				gender = cg_entities[es->clientNum].npcClient->gender;
			}
			else
			{
				//use the highest scorer's gender
				gender = cgs.clientinfo[cg.scores[0].client].gender;
			}

			if (gender == GENDER_FEMALE)
			{
				char* r;
				s = CG_ConfigString(CS_SOUNDS + es->eventParm);
				r = strstr(s, "SOUND/CHARS/JADEN_MALE/");

				if (r)
				{
					char sound_name[MAX_QPATH];
					char* name_slash;
					char* fileName;

					if (!strstr(s, "SOUND/CHARS/JADEN_MALE/MISC/"))
					{
						//check for misc sounds
						strcpy(sound_name, "SOUND/CHARS/JADEN_FMLE/");
					}
					else
					{
						strcpy(sound_name, "SOUND/CHARS/JADEN_FMLE/MISC/");
					}

					//find the slash
					name_slash = Q_strrchr(sound_name, '/');

					//find the filename
					fileName = Q_strrchr(r, '/');
					strcpy(name_slash, fileName);

					trap->S_StartSound(NULL, es->clientNum, es->trickedentindex,
						CG_CustomSound(es->clientNum, sound_name));
					break;
				}
			}
		}
		if (cgs.gameSounds[es->eventParm])
		{
			trap->S_StartSound(NULL, es->clientNum, es->trickedentindex, cgs.gameSounds[es->eventParm]);
		}
		else
		{
			s = CG_ConfigString(CS_SOUNDS + es->eventParm);
			trap->S_StartSound(NULL, es->clientNum, es->trickedentindex, CG_CustomSound(es->clientNum, s));
		}
		break;

	case EV_PLAY_ROFF:
		DEBUGNAME("EV_PLAY_ROFF");
		trap->ROFF_Play(es->weapon, es->eventParm, es->trickedentindex);
		break;

	case EV_GLASS_SHATTER:
		DEBUGNAME("EV_GLASS_SHATTER");
		CG_GlassShatter(es->genericenemyindex, es->origin, es->angles, es->trickedentindex, es->pos.trTime);
		break;

	case EV_DEBRIS:
		DEBUGNAME("EV_DEBRIS");
		CG_Chunks(es->owner, es->origin, es->origin2, es->angles2, es->speed, es->eventParm,
			es->trickedentindex, es->modelIndex, es->apos.trBase[0]);
		break;

	case EV_MISC_MODEL_EXP:
		DEBUGNAME("EV_MISC_MODEL_EXP");
		CG_MiscModelExplosion(es->origin2, es->angles2, es->time, es->eventParm);
		break;

	case EV_PAIN:
		// local player sounds are triggered in CG_CheckLocalSounds,
		// so ignore events on the player
		DEBUGNAME("EV_PAIN");
		CG_PainEvent(cent, es->eventParm);
		break;

	case EV_DEATH1:
	case EV_DEATH2:
	case EV_DEATH3:
		DEBUGNAME("EV_DEATHx");
		trap->S_StartSound(NULL, es->number, CHAN_VOICE,
			CG_CustomSound(es->number, va("*death%i.wav", event - EV_DEATH1 + 1)));
		if (es->eventParm && es->number == cg.snap->ps.clientNum)
		{
			trap->S_StartLocalSound(cgs.media.dramaticFailure, CHAN_LOCAL);
			CGCam_SetMusicMult(0.3f, 5000);
		}
		break;

	case EV_OBITUARY:
		DEBUGNAME("EV_OBITUARY");
		CG_Obituary(es);
		break;

		//
		// powerup events
		//
	case EV_POWERUP_QUAD:
		DEBUGNAME("EV_POWERUP_QUAD");
		if (es->number == cg.snap->ps.clientNum)
		{
			cg.powerupActive = PW_MEDITATE;
			cg.powerupTime = cg.time;
		}
		break;

	case EV_POWERUP_BATTLESUIT:
		DEBUGNAME("EV_POWERUP_BATTLESUIT");
		if (es->number == cg.snap->ps.clientNum)
		{
			cg.powerupActive = PW_BATTLESUIT;
			cg.powerupTime = cg.time;
		}
		break;

	case EV_FORCE_DRAINED:
		DEBUGNAME("EV_FORCE_DRAINED");
		ByteToDir(es->eventParm, dir);
		trap->S_StartSound(NULL, es->owner, CHAN_AUTO, cgs.media.drainSound);
		cg_entities[es->owner].teamPowerEffectTime = cg.time + 1000;
		cg_entities[es->owner].teamPowerType = 2;
		break;

	case EV_GIB_PLAYER:
		DEBUGNAME("EV_GIB_PLAYER");
		CG_GibPlayer(cent->lerpOrigin);
		break;

	case EV_GIB_PLAYER_HEADSHOT:
		DEBUGNAME("EV_GIB_PLAYER_HEADSHOT");
		cent->pe.noHead = qtrue;
		CG_GibPlayerHeadshot(cent->lerpOrigin);
		break;

	case EV_BODY_NOHEAD:
		DEBUGNAME("EV_BODY_NOHEAD");
		cent->pe.noHead = qtrue;
		break;

	case EV_DRUGGED:
		DEBUGNAME("EV_DRUGGED");
		if (es->owner == cg.predictedPlayerState.clientNum)
		{
			// Only allow setting up the wonky vision on the player..do it for 10 seconds...must be synchronized with calcs done in cg_view.  Just search for cg.wonkyTime to find 'em.
			cg.wonkyTime = cg.time + 10000;
		}
		break;

	case EV_STUNNED:
		DEBUGNAME("EV_STUNNED");
		if (es->owner == cg.predictedPlayerState.clientNum)
		{
			// Only allow setting up the wonky vision on the player..do it for 5 seconds...must be synchronized with calcs done in cg_view.  Just search for cg.wonkyTime to find 'em.
			cg.stunnedTime = cg.time + 5000;
		}
		break;

	case EV_STARTLOOPINGSOUND:
		DEBUGNAME("EV_STARTLOOPINGSOUND");
		if (cgs.gameSounds[es->eventParm])
		{
			isnd = cgs.gameSounds[es->eventParm];
		}
		else
		{
			s = CG_ConfigString(CS_SOUNDS + es->eventParm);
			isnd = CG_CustomSound(es->number, s);
		}

		CG_S_AddRealLoopingSound(es->number, es->pos.trBase, vec3_origin, isnd);
		es->loopSound = isnd;
		break;

	case EV_STOPLOOPINGSOUND:
		DEBUGNAME("EV_STOPLOOPINGSOUND");
		CG_S_StopLoopingSound(es->number, -1);
		es->loopSound = 0;
		break;

	case EV_WEAPON_CHARGE:
		DEBUGNAME("EV_WEAPON_CHARGE");
		assert(es->eventParm > WP_NONE && es->eventParm < WP_NUM_WEAPONS);
		if (cg_weapons[es->eventParm].chargeSound)
		{
			trap->S_StartSound(NULL, es->number, CHAN_WEAPON, cg_weapons[es->eventParm].chargeSound);
		}
		else if (es->eventParm == WP_DISRUPTOR)
		{
			trap->S_StartSound(NULL, es->number, CHAN_WEAPON, cgs.media.disruptorZoomLoop);
		}
		break;

	case EV_WEAPON_CHARGE_ALT:
		DEBUGNAME("EV_WEAPON_CHARGE_ALT");
		assert(es->eventParm > WP_NONE && es->eventParm < WP_NUM_WEAPONS);
		if (cg_weapons[es->eventParm].altChargeSound)
		{
			trap->S_StartSound(NULL, es->number, CHAN_WEAPON, cg_weapons[es->eventParm].altChargeSound);
		}
		break;

	case EV_SHIELD_HIT:
		DEBUGNAME("EV_SHIELD_HIT");
		ByteToDir(es->eventParm, dir);
		CG_PlayerShieldHit(es->otherentity_num, dir, es->time2);
		break;

	case EV_DEBUG_LINE:
		DEBUGNAME("EV_DEBUG_LINE");
		CG_Beam(cent);
		break;

	case EV_TESTLINE:
		DEBUGNAME("EV_TESTLINE");
		CG_TestLine(es->origin, es->origin2, es->time2, es->weapon, 1);
		break;
		//this event is sent whenever a player has lost/picked up a weapon.  This is used
		//to update the client side's weapon stats for the holstered weapons rendering.
	case EV_WEAPINVCHANGE:
		DEBUGNAME("EV_WEAPINVCHANGE");
		cg_entities[es->number].weapons = es->eventParm;
		break;

	case EV_BATTERIES_CHARGED:
		cg.batteryChargeTime = cg.time + 3000;
		trap->S_StartSound(vec3_origin, es->number, CHAN_AUTO, cgs.media.batteryChargeSound);
		break;

	case EV_BLOCKLINE:
		DEBUGNAME("EV_BLOCKLINE");
		CG_BlockLine(es->origin, es->origin2, es->time2, es->weapon, 1);
		break;

	default:
		DEBUGNAME("UNKNOWN");
		trap->Error(ERR_DROP, "Unknown event: %i", event);
		break;
	}
}

/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents(centity_t* cent)
{
	// check for event-only entities
	if (cent->currentState.eType > ET_EVENTS)
	{
		if (cent->previousEvent)
		{
			return; // already fired
		}
		// if this is a player event set the entity number of the client entity number
		if (cent->currentState.eFlags & EF_PLAYER_EVENT)
		{
			cent->currentState.number = cent->currentState.otherentity_num;
		}

		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	}
	else
	{
		// check for events riding with another entity
		if (cent->currentState.event == cent->previousEvent)
		{
			return;
		}
		cent->previousEvent = cent->currentState.event;
		if ((cent->currentState.event & ~EV_EVENT_BITS) == 0)
		{
			return;
		}
	}

	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin);
	CG_SetEntitySoundPosition(cent);

	CG_EntityEvent(cent, cent->lerpOrigin);
}