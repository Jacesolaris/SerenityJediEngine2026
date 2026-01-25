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

#include "../server/exe_headers.h"

#include "server.h"
#include "../game/weapons.h"
#include "../game/g_items.h"
#include "../game/statindex.h"

#include "../win32/AutoVersion.h"

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

qboolean qbLoadTransition = qfalse;
extern vmCvar_t r_ratiofix;

//=========================================================
// don't call this directly, it should only be called from SV_Map_f() or SV_MapTransition_f()
//
static bool SV_Map_(const ForceReload_e eForceReload)
{
	char expanded[MAX_QPATH] = { 0 };

	bool not_jk_map = true;

	char* JKO_Maps[] =
	{
		"kejim_post",
		"kejim_base",
		"kejim_impb",
		"artus_mine",
		"artus_detention",
		"artus_topside",
		"valley",
		"yavin_temple",
		"yavin_trial",
		"ns_streets",
		"ns_hideout",
		"ns_starpad",
		"bespin_undercity",
		"bespin_streets",
		"bespin_platform",
		"cairn_bay",
		"cairn_assembly",
		"cairn_reactor",
		"cairn_dock1",
		"doom_comm",
		"doom_detention",
		"doom_shields",
		"yavin_swamp",
		"yavin_canyon",
		"yavin_courtyard",
		"yavin_final",
		"demo",
		"jodemo",
		"pit"
	};

	char* JKA_Maps[] =
	{
		"academy1",
		"academy2",
		"academy3",
		"academy4",
		"academy5",
		"academy6",
		"hoth2",
		"hoth3",
		"kor1",
		"kor2",
		"t1_danger",
		"t1_fatal",
		"t1_inter",
		"t1_rail",
		"t1_sour",
		"t1_surprise",
		"t2_dpred",
		"t2_rancor",
		"t2_rogue",
		"t2_trip",
		"t2_wedge",
		"t3_bounty",
		"t3_byss",
		"t3_hevil",
		"t3_rift",
		"t3_stamp",
		"taspir1",
		"taspir2",
		"vjun1",
		"vjun2",
		"vjun3",
		"yavin1",
		"yavin1b",
		"yavin2",
		"ladder"
	};

	char* DF_Maps[] =
	{
		"dtention",
		"executor",
		"fuelstat",
		"gromas",
		"jabship",
		"narshada",
		"robotics",
		"secbase",
		"sewers",
		"talay",
		"testbase"
	};

	char* Surv_Maps[] =
	{
		"asajjsurv",
		"asajjwin",
		"clonesurv",
		"clonewin",
		"headtoheadsurv",
		"immysurv",
		"immywin",
		"izzysurv",
		"izzywin"
	};

	char* Izzy_Maps[] =
	{
		"izzysurv",
		"izzywin"
	};

	char* Immy_Maps[] =
	{
		"immysurv",
		"immywin"
	};

	char* Asajj_Maps[] =
	{
		"asajjsurv",
		"asajjwin"
	};

	char* Kotor_Maps[] =
	{
		"CitadelStationDocks126HangarThief",
		"Credits",
		"DantooineMasterVrookLastStand",
		"DarthRevanEncounter",
		"DarthSionDuel",
		"DarthSionFinalDuel",
		"DxunDuelWithTheSithLords",
		"DxunMandalorianOutpostUnderAttack",
		"KreiaVision",
		"MalakVision",
		"NarShaddaaZezKaiEllDuel",
		"NihilusLastStand",
		"OnderonMasterKavarDuel",
		"PeragusFightwithHK50",
		"TelosAtrisDuel",
		"TelosCitadelStationUnderAttackPart1",
		"TelosCitadelStationUnderAttackPart2",
		"TelosCitadelStationUnderAttackPart3",
		"TrayusCoreFight",
		"VisasDuel"
	};

	char* Nina_Maps[] =
	{
		"conclusion",
		"nina01",
		"nina02",
		"nina03",
		"nina04",
		"nina_05",
		"nina_06",
		"nina_06_",
		"nina_07",
		"intro_07",
		"nina_08",
		"intro_08",
		"nina_09",
		"nina_10",
		"yavin_cine"
	};

	char* Yavin_Maps[] =
	{
		"level_jedi_council1",
		"level_jedi_council2",
		"level_jedi_council3",
		"level_jedi_council4",
		"level_jedi_council5",
		"level_jedi_council6",
		"level0",
		"level1",
		"level1_comm_station",
		"level1_comm_station_c",
		"level1_lava",
		"level1_outside",
		"level2",
		"level2_caves",
		"level2_saber_shipment",
		"level2_transport_entry",
		"level2_transport_entry_c",
		"level3",
		"level3_old_reactor",
		"level3_power_reserves",
		"level4",
		"level4_crystal_cavern",
		"level4_lower_lake",
		"level4_weapons_cache",
		"level5",
		"level5_apprentice",
		"level5_fueling_station",
		"level5_sith_command_ship",
		"level6",
		"level6_defeat_council",
		"level6_outskirts",
		"level6_outskirts_sith",
		"level6_queen",
		"level6_queen_sith",
		"level6_sith",
		"level7_fanfare",
		"level7_fanfare_dark",
		"level7_fanfare_sith"
	};

	char* Veng_Maps[] =
	{
		"Part_1",
		"Part_2",
		"Part_3",
		"Part_4"
	};

	char* NoCubeMapping_Maps[] =
	{
		"yavin1",
		"yavin1b"
	};

	char* map = Cmd_Argv(1);
	if (!*map)
	{
		Com_Printf("no map specified\n");
		return false;
	}

	// make sure the level exists before trying to change, so that
	// a typo at the server console won't end the game
	if (strchr(map, '\\'))
	{
		Com_Printf("Can't have mapnames with a \\\n");
		return false;
	}

	Com_sprintf(expanded, sizeof expanded, "maps/%s.bsp", map);

	if (FS_ReadFile(expanded, nullptr) == -1)
	{
		Com_Printf("Can't find map %s\n", expanded);
		extern cvar_t* com_buildScript;
		if (com_buildScript && com_buildScript->integer)
		{
			//yes, it's happened, someone deleted a map during my build...
			Com_Error(ERR_FATAL, "Can't find map %s\n", expanded);
		}
		return false;
	}

	if (g_spskill->integer > 2)
	{
		Cvar_Set("g_spskill", "2");
	}

	for (auto& jka_map : JKA_Maps)
	{
		if (strcmp(map, jka_map) == 0)
		{
			if (com_outcast->integer != 0)
			{
				Cvar_Set("com_outcast", "0");
			}
			not_jk_map = false;
		}
	}
	for (auto& jko_map : JKO_Maps)
	{
		if (strcmp(map, jko_map) == 0)
		{
			if (com_outcast->integer != 1)
			{
				Cvar_Set("com_outcast", "1");
			}
			Cvar_Set("g_char_model", "kyle");
			Cvar_Set("g_char_skin_head", "model_default");
			Cvar_Set("g_char_skin_torso", "model_default");
			Cvar_Set("g_char_skin_legs", "model_default");
			Cvar_Set("g_saber", "kyle");
			Cvar_Set("g_saber_color", "blue");
			Cvar_Set("g_saber2", "none");
			Cvar_Set("snd", "kyle");
			Cvar_Set("sex", "m");
			not_jk_map = false;
		}
	}

	// com_outcast", "2"

	for (auto& Yavin_map : Yavin_Maps)
	{
		if (strcmp(map, Yavin_map) == 0)
		{
			if (com_outcast->integer != 3)
			{
				Cvar_Set("com_outcast", "3");
			}
			not_jk_map = false;
		}
	}
	for (auto& DF_map : DF_Maps)
	{
		if (strcmp(map, DF_map) == 0)
		{
			if (com_outcast->integer != 4)
			{
				Cvar_Set("com_outcast", "4");
			}
			Cvar_Set("g_char_model", "df2_kyle");
			Cvar_Set("g_char_skin_head", "head_a1");
			Cvar_Set("g_char_skin_torso", "torso_a1");
			Cvar_Set("g_char_skin_legs", "lower_a1");
			Cvar_Set("g_saber", "kyle");
			Cvar_Set("g_saber_color", "blue");
			Cvar_Set("g_saber2", "none");
			Cvar_Set("snd", "kyle");
			Cvar_Set("sex", "m");
			not_jk_map = false;
		}
	}
	for (auto& Kotor_map : Kotor_Maps)
	{
		if (strcmp(map, Kotor_map) == 0)
		{
			if (com_outcast->integer != 5)
			{
				Cvar_Set("com_outcast", "5");
			}
			not_jk_map = false;
		}
	}
	for (auto& Surv_map : Surv_Maps)
	{
		if (strcmp(map, Surv_map) == 0)
		{
			if (com_outcast->integer != 6)
			{
				Cvar_Set("com_outcast", "6");
			}
			for (auto& Izzy_map : Izzy_Maps)
			{
				if (strcmp(map, Izzy_map) == 0)
				{
					Cvar_Set("g_char_model", "izzyv3");
					Cvar_Set("g_char_skin_head", "head_a1");
					Cvar_Set("g_char_skin_torso", "torso_a1");
					Cvar_Set("g_char_skin_legs", "lower_a1");
					Cvar_Set("g_saber", "izzysith");
					Cvar_Set("g_saber_color", "orange");
					Cvar_Set("g_saber2", "none");
					Cvar_Set("snd", "jaden_male");
					Cvar_Set("sex", "m");
				}
			}

			for (auto& Immy_map : Immy_Maps)
			{
				if (strcmp(map, Immy_map) == 0)
				{
					Cvar_Set("g_char_model", "Immy");
					Cvar_Set("g_char_skin_head", "head_a1");
					Cvar_Set("g_char_skin_torso", "torso_a1");
					Cvar_Set("g_char_skin_legs", "lower_a1");
					Cvar_Set("g_saber", "dual_1");
					Cvar_Set("g_saber_color", "purple");
					Cvar_Set("g_saber2", "none");
					Cvar_Set("snd", "jaden_fmle");
					Cvar_Set("sex", "f");
				}
			}

			for (auto& Asajj_map : Asajj_Maps)
			{
				if (strcmp(map, Asajj_map) == 0)
				{
					Cvar_Set("g_char_model", "asajj");
					Cvar_Set("g_char_skin_head", "head_a1");
					Cvar_Set("g_char_skin_torso", "torso_a1");
					Cvar_Set("g_char_skin_legs", "lower_a1");
					Cvar_Set("g_saber_color", "red");
					Cvar_Set("snd", "asajj");
					Cvar_Set("sex", "f");
				}
			}
			not_jk_map = false;
		}
	}
	for (auto& Nina_map : Nina_Maps)
	{
		if (strcmp(map, Nina_map) == 0)
		{
			if (com_outcast->integer != 7)
			{
				Cvar_Set("com_outcast", "7");
			}
			Cvar_Set("g_char_model", "jedi_nina");
			Cvar_Set("snd", "tavion");
			Cvar_Set("sex", "f");
			not_jk_map = false;
		}
	}
	for (auto& veng_map : Veng_Maps)
	{
		if (strcmp(map, veng_map) == 0)
		{
			if (com_outcast->integer != 8)
			{
				Cvar_Set("com_outcast", "8");
			}
			Cvar_Set("g_char_model", "mara_ponytail");
			Cvar_Set("snd", "jaden_fmle");
			Cvar_Set("sex", "f");
			not_jk_map = false;
		}
	}
	for (auto& NoCubeMapping_Map : NoCubeMapping_Maps)
	{
		if (strcmp(map, NoCubeMapping_Map) == 0)
		{
			if (r_cubeMapping->integer != 0)
			{
				Cvar_Set("r_cubeMapping", "0");
			}
		}
	}

	//if (not_jk_map) //then it must be a MP Map so just to be sure go to "2".
	//{
	//	if (com_outcast->integer != 2)
	//	{
	//		Cvar_Set("com_outcast", "2");
	//	}
	//}

	if (g_Weather->integer >= 0)
	{
		Cvar_Set("r_weather", "0");
	}

	if (debugNPCFreeze->integer >= 0)
	{
		Cvar_Set("d_npcfreeze", "0");
	}

	if (r_ratiofix.integer >= 0)
	{
		Cvar_Set("r_ratiofix", "0");
	}

	if (map[0] != '_')
	{
		SG_WipeSavegame("auto");
	}

	SV_SpawnServer(map, eForceReload, qtrue); // start up the map
	return true;
}

// Save out some player data for later restore if this is a spawn point with KEEP_PREV (spawnflags&1) set...
//
// (now also called by auto-save code to setup the cvars correctly
void SV_Player_EndOfLevelSave()
{
	// I could just call GetClientState() but that's in sv_bot.cpp, and I'm not sure if that's going to be deleted for
	//	the single player build, so here's the guts again...
	//
	const client_t* cl = &svs.clients[0]; // 0 because only ever us as a player

	if (cl
		&&
		cl->gentity && cl->gentity->client // crash fix for voy4->brig transition when you kill Foster.
		//	Shouldn't happen, but does sometimes...
		)
	{
		int i;
		Cvar_Set(sCVARNAME_PLAYERSAVE, ""); // default to blank

		playerState_t* p_state = cl->gentity->client;
		const char* s2;
		const char* s;
#ifdef JK2_MODE
		s = va("%i %i %i %i %i %i %i %f %f %f %i %i %i %i %i %i",
			p_state->stats[STAT_HEALTH],
			p_state->stats[STAT_ARMOR],
			p_state->stats[STAT_WEAPONS],
			p_state->stats[STAT_ITEMS],
			p_state->weapon,
			p_state->weaponstate,
			p_state->batteryCharge,
			p_state->viewangles[0],
			p_state->viewangles[1],
			p_state->viewangles[2],
			p_state->forcePowersKnown,
			p_state->forcePower,
			p_state->saberActive,
			p_state->saberAnimLevel,
			p_state->saberLockEnemy,
			p_state->saberLockTime
		);
#else
		//				|general info				  |-force powers |-saber 1		|-saber 2										  |-general saber
		s = va(
			"%i %i %i %i %i %i %i %f %f %f %i %i %i %i %i %s %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %s %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
			p_state->stats[STAT_HEALTH],
			p_state->stats[STAT_ARMOR],
			p_state->stats[STAT_WEAPONS],
			p_state->stats[STAT_ITEMS],
			p_state->weapon,
			p_state->weaponstate,
			p_state->batteryCharge,
			p_state->viewangles[0],
			p_state->viewangles[1],
			p_state->viewangles[2],
			//force power data
			p_state->forcePowersKnown,
			p_state->forcePower,
			p_state->forcePowerMax,
			p_state->forcePowerRegenRate,
			p_state->forcePowerRegenAmount,
			//saber 1 data
			p_state->saber[0].name,
			p_state->saber[0].blade[0].active,
			p_state->saber[0].blade[1].active,
			p_state->saber[0].blade[2].active,
			p_state->saber[0].blade[3].active,
			p_state->saber[0].blade[4].active,
			p_state->saber[0].blade[5].active,
			p_state->saber[0].blade[6].active,
			p_state->saber[0].blade[7].active,
			p_state->saber[0].blade[0].color,
			p_state->saber[0].blade[1].color,
			p_state->saber[0].blade[2].color,
			p_state->saber[0].blade[3].color,
			p_state->saber[0].blade[4].color,
			p_state->saber[0].blade[5].color,
			p_state->saber[0].blade[6].color,
			p_state->saber[0].blade[7].color,
			//saber 2 data
			p_state->saber[1].name,
			p_state->saber[1].blade[0].active,
			p_state->saber[1].blade[1].active,
			p_state->saber[1].blade[2].active,
			p_state->saber[1].blade[3].active,
			p_state->saber[1].blade[4].active,
			p_state->saber[1].blade[5].active,
			p_state->saber[1].blade[6].active,
			p_state->saber[1].blade[7].active,
			p_state->saber[1].blade[0].color,
			p_state->saber[1].blade[1].color,
			p_state->saber[1].blade[2].color,
			p_state->saber[1].blade[3].color,
			p_state->saber[1].blade[4].color,
			p_state->saber[1].blade[5].color,
			p_state->saber[1].blade[6].color,
			p_state->saber[1].blade[7].color,
			//general saber data
			p_state->saberStylesKnown,
			p_state->saberAnimLevel,
			p_state->saberLockEnemy,
			p_state->saberLockTime
		);
#endif
		Cvar_Set(sCVARNAME_PLAYERSAVE, s);

		//ammo
		s2 = "";
		for (i = 0; i < AMMO_MAX; i++)
		{
			s2 = va("%s %i", s2, p_state->ammo[i]);
		}
		Cvar_Set("playerammo", s2);

		//inventory
		s2 = "";
		for (i = 0; i < INV_MAX; i++)
		{
			s2 = va("%s %i", s2, p_state->inventory[i]);
		}
		Cvar_Set("playerinv", s2);

		// the new JK2 stuff - force powers, etc...
		//
		s2 = "";
		for (i = 0; i < NUM_FORCE_POWERS; i++)
		{
			s2 = va("%s %i", s2, p_state->forcePowerLevel[i]);
		}
		Cvar_Set("playerfplvl", s2);
	}
}

// Restart the server on a different map
//
static void SV_MapTransition_f()
{
	const char* spawntarget;

#ifdef JK2_MODE
	SCR_PrecacheScreenshot();
#endif
	SV_Player_EndOfLevelSave();

	spawntarget = Cmd_Argv(2);
	if (*spawntarget != '\0')
	{
		Cvar_Set("spawntarget", spawntarget);
	}
	else
	{
		Cvar_Set("spawntarget", "");
	}

	SV_Map_(eForceReload_NOTHING);
}

/*
==================
SV_Map_f

Restart the server on a different map, but clears a cvar so that typing "map blah" doesn't try and preserve
player weapons/ammo/etc from the previous level that you haven't really exited (ie ignores KEEP_PREV on spawn points)
==================
*/
#ifdef JK2_MODE
extern void SCR_UnprecacheScreenshot();
#endif
static void SV_Map_f()
{
	Cvar_Set(sCVARNAME_PLAYERSAVE, "");
	Cvar_Set("spawntarget", "");
	Cvar_Set("tier_storyinfo", "0");
	Cvar_Set("tiers_complete", "");
#ifdef JK2_MODE
	SCR_UnprecacheScreenshot();
#endif

	ForceReload_e e_force_reload = eForceReload_NOTHING; // default for normal load

	const char* cmd = Cmd_Argv(0);
	if (!Q_stricmp(cmd, "devmapbsp"))
		e_force_reload = eForceReload_BSP;
	else if (!Q_stricmp(cmd, "devmapmdl"))
		e_force_reload = eForceReload_MODELS;
	else if (!Q_stricmp(cmd, "devmapall"))
		e_force_reload = eForceReload_ALL;

	auto cheat = static_cast<qboolean>(!Q_stricmpn(cmd, "devmap", 6));

	// retain old cheat state
	if (!cheat && Cvar_VariableIntegerValue("helpUsObi"))
		cheat = qtrue;

	if (SV_Map_(e_force_reload))
	{
		// set the cheat value
		// if the level was started with "map <levelname>", then
		// cheats will not be allowed.  If started with "devmap <levelname>"
		// then cheats will be allowed
		Cvar_Set("helpUsObi", cheat ? "1" : "0");
	}
#ifdef JK2_MODE
	Cvar_Set("cg_missionstatusscreen", "1");
#endif
}

/*
==================
SV_LoadTransition_f
==================
*/
void SV_LoadTransition_f()
{
	const char* spawntarget;

	const char* map = Cmd_Argv(1);
	if (!*map)
	{
		return;
	}

	qbLoadTransition = qtrue;

#ifdef JK2_MODE
	SCR_PrecacheScreenshot();
#endif
	SV_Player_EndOfLevelSave();

	//Save the full current state of the current map so we can return to it later
	SG_WriteSavegame(va("hub/%s", sv_mapname->string), qfalse);

	//set the spawntarget if there is one
	spawntarget = Cmd_Argv(2);
	if (*spawntarget != '\0')
	{
		Cvar_Set("spawntarget", spawntarget);
	}
	else
	{
		Cvar_Set("spawntarget", "");
	}

	if (!SV_TryLoadTransition(map))
	{
		//couldn't load a savegame
		SV_Map_(eForceReload_NOTHING);
	}
	qbLoadTransition = qfalse;
}

//===============================================================

static char* ivtos(const vec3_t v)
{
	static int index;
	static char str[8][32];

	// use an array so that multiple vtos won't collide
	char* s = str[index];
	index = index + 1 & 7;

	Com_sprintf(s, 32, "( %i %i %i )", static_cast<int>(v[0]), static_cast<int>(v[1]), static_cast<int>(v[2]));

	return s;
}

/*
================
SV_Status_f
================
*/
static void SV_Status_f()
{
	// make sure server is running
	if (!com_sv_running->integer)
	{
		Com_Printf("Server is not running.\n");
		return;
	}

	client_t* cl = &svs.clients[0];

	if (!cl)
	{
		Com_Printf("Server is not running.\n");
		return;
	}

#if defined(_WIN32)
#define STATUS_OS "Windows"
#elif defined(__linux__)
#define STATUS_OS "Linux"
#elif defined(MACOS_X)
#define STATUS_OS "OSX"
#else
#define STATUS_OS "Unknown"
#endif

	Com_Printf("name    : %s^7\n", cl->name);
	Com_Printf("score   : %i\n", cl->gentity->client->persistant[PERS_SCORE]);
	Com_Printf("version : %s %s %i\n", STATUS_OS, VERSION_STRING_DOTTED, PROTOCOL_VERSION);
#ifdef JK2_MODE
	Com_Printf("game    : Jedi Outcast %s\n", FS_GetCurrentGameDir());
#else
	Com_Printf("game    : Jedi Academy %s\n", FS_GetCurrentGameDir());
#endif
	Com_Printf("map     : %s at %s\n", sv_mapname->string, ivtos(cl->gentity->client->origin));
}

/*
===========
SV_Serverinfo_f

Examine the serverinfo string
===========
*/
static void SV_Serverinfo_f()
{
	Com_Printf("Server info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_SERVERINFO));
}

/*
===========
SV_Systeminfo_f

Examine or change the serverinfo string
===========
*/
static void SV_Systeminfo_f()
{
	Com_Printf("System info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_SYSTEMINFO));
}

/*
===========
SV_DumpUser_f

Examine all a users info strings FIXME: move to game
===========
*/
static void SV_DumpUser_f()
{
	// make sure server is running
	if (!com_sv_running->integer)
	{
		Com_Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() != 1)
	{
		Com_Printf("Usage: info\n");
		return;
	}

	const client_t* cl = &svs.clients[0];
	if (!cl->state)
	{
		Com_Printf("Client is not active\n");
		return;
	}

	Com_Printf("userinfo\n");
	Com_Printf("--------\n");
	Info_Print(cl->userinfo);
}

//===========================================================

/*
==================
SV_CompleteMapName
==================
*/
static void SV_CompleteMapName(char* args, const int arg_num)
{
	if (arg_num == 2)
		Field_CompleteFilename("maps", "bsp", qtrue, qfalse);
}

/*
==================
SV_CompleteMapName
==================
*/
static void SV_CompleteSaveName(char* args, const int arg_num)
{
	if (arg_num == 2)
	{
		if (com_outcast->integer == 0)
		{
			if (!g_newgameplusJKA->integer)
			{
				Field_CompleteFilename("Account/Saved-Missions-JediAcademy/", "sav", qtrue, qtrue);
			}
			else
			{
				Field_CompleteFilename("Account/Saved-Missions-JediAcademyplus/", "sav", qtrue, qtrue);
			}
		}
		else if (com_outcast->integer == 1)
		{
			if (!g_newgameplusJKO->integer)
			{
				Field_CompleteFilename("Account/Saved-Missions-JediOutcast/", "sav", qtrue, qtrue);
			}
			else
			{
				Field_CompleteFilename("Account/Saved-Missions-JediOutcastplus/", "sav", qtrue, qtrue);
			}
		}
		else if (com_outcast->integer == 2)//playing creative
		{
			Field_CompleteFilename("Account/Saved-Missions-JediCreative/", "sav", qtrue, qtrue);
		}
		else if (com_outcast->integer == 3) //playing yav
		{
			Field_CompleteFilename("Account/Saved-Missions-Yavin4/", "sav", qtrue, qtrue);
		}
		else if (com_outcast->integer == 4) //playing darkforces
		{
			Field_CompleteFilename("Account/Saved-Missions-DarkForces/", "sav", qtrue, qtrue);
		}
		else if (com_outcast->integer == 5)//playing kotor
		{
			Field_CompleteFilename("Account/Saved-Missions-Kotor/", "sav", qtrue, qtrue);
		}
		else if (com_outcast->integer == 6) // playing survival
		{
			Field_CompleteFilename("Account/Saved-Missions-Survival/", "sav", qtrue, qtrue);
		}
		else if (com_outcast->integer == 7) // playing nina
		{
			Field_CompleteFilename("Account/Saved-Missions-Nina/", "sav", qtrue, qtrue);
		}
		else if (com_outcast->integer == 8) // playing veng
		{
			Field_CompleteFilename("Account/Saved-Missions-Veng/", "sav", qtrue, qtrue);
		}
		else
		{
			if (!g_newgameplusJKA->integer)
			{
				Field_CompleteFilename("Account/Saved-Missions-JediAcademy/", "sav", qtrue, qtrue);
			}
			else
			{
				Field_CompleteFilename("Account/Saved-Missions-JediAcademyplus/", "sav", qtrue, qtrue);
			}
		}
	}
}

/*
==================
SV_AddOperatorCommands
==================
*/
void SV_AddOperatorCommands()
{
	static qboolean initialized;

	if (initialized)
	{
		return;
	}
	initialized = qtrue;

	Cmd_AddCommand("status", SV_Status_f);
	Cmd_AddCommand("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand("systeminfo", SV_Systeminfo_f);
	Cmd_AddCommand("dumpuser", SV_DumpUser_f);
	Cmd_AddCommand("sectorlist", SV_SectorList_f);
	Cmd_AddCommand("map", SV_Map_f);
	Cmd_SetCommandCompletionFunc("map", SV_CompleteMapName);
	Cmd_AddCommand("devmap", SV_Map_f);
	Cmd_SetCommandCompletionFunc("devmap", SV_CompleteMapName);
	Cmd_AddCommand("devmapbsp", SV_Map_f);
	Cmd_SetCommandCompletionFunc("devmapbsp", SV_CompleteMapName);
	Cmd_AddCommand("devmapmdl", SV_Map_f);
	Cmd_SetCommandCompletionFunc("devmapmdl", SV_CompleteMapName);
	Cmd_AddCommand("devmapsnd", SV_Map_f);
	Cmd_SetCommandCompletionFunc("devmapsnd", SV_CompleteMapName);
	Cmd_AddCommand("devmapall", SV_Map_f);
	Cmd_SetCommandCompletionFunc("devmapall", SV_CompleteMapName);
	Cmd_AddCommand("maptransition", SV_MapTransition_f);
	Cmd_AddCommand("load", SV_LoadGame_f);
	Cmd_SetCommandCompletionFunc("load", SV_CompleteSaveName);
	Cmd_AddCommand("loadtransition", SV_LoadTransition_f);
	Cmd_AddCommand("save", SV_SaveGame_f);
	Cmd_AddCommand("wipe", SV_WipeGame_f);
}

/*
==================
SV_RemoveOperatorCommands
==================
*/
void SV_RemoveOperatorCommands()
{
#if 0
	// removing these won't let the server start again
	Cmd_RemoveCommand("status");
	Cmd_RemoveCommand("serverinfo");
	Cmd_RemoveCommand("systeminfo");
	Cmd_RemoveCommand("dumpuser");
	Cmd_RemoveCommand("serverrecord");
	Cmd_RemoveCommand("serverstop");
	Cmd_RemoveCommand("sectorlist");
#endif
}