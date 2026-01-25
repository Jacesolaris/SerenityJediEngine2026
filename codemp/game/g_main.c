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

#include "g_local.h"
#include "g_ICARUScb.h"
#include "g_nav.h"
#include "bg_saga.h"
#include "b_local.h"
#include "g_dynmusic.h"
#include "g_roff.h"

level_locals_t level;

int eventClearTime = 0;
static int navCalcPathTime = 0;
extern int fatalErrors;

int killPlayerTimer = 0;

gentity_t g_entities[MAX_GENTITIES];
gclient_t g_clients[MAX_CLIENTS];

qboolean gDuelExit = qfalse;

void G_InitGame(int levelTime, int randomSeed, int restart);
void G_RunFrame(int levelTime);
void G_ShutdownGame(int restart);
void CheckExitRules(void);
void G_ROFF_NotetrackCallback(gentity_t* cent, const char* notetrack);

extern stringID_table_t setTable[];

extern void Sphereshield_Off(gentity_t* self);

qboolean G_ParseSpawnVars(qboolean inSubBSP);
void G_SpawnGEntityFromSpawnVars(qboolean inSubBSP);

qboolean NAV_ClearPathToPoint(gentity_t* self, vec3_t pmins, vec3_t pmaxs, vec3_t point, int clipmask,
	int ok_to_hit_ent_num);
qboolean NPC_ClearLOS2(gentity_t* ent, const vec3_t end);
int NAVNEW_ClearPathBetweenPoints(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int ignore, int clipmask);
qboolean NAV_CheckNodeFailedForEnt(const gentity_t* ent, int nodeNum);
qboolean G_EntIsUnlockedDoor(int entityNum);
qboolean G_EntIsDoor(int entityNum);
qboolean G_EntIsBreakable(int entityNum);
qboolean G_EntIsRemovableUsable(int entNum);
void CP_FindCombatPointWaypoints(void);
extern void G_CheckSpecialPersistentEvents(gentity_t* ent);
extern qboolean PM_RestAnim(int anim);
extern qboolean PM_CrouchAnim(int anim);
extern void WP_BlockPointsUpdate(const gentity_t* self);

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
static void G_FindTeams(void)
{
	gentity_t* e, * e2;
	int i, j;

	int c = 0;
	int c2 = 0;
	for (i = MAX_CLIENTS, e = g_entities + i; i < level.num_entities; i++, e++)
	{
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		if (e->r.contents == CONTENTS_TRIGGER)
			continue; //triggers NEVER link up in teams!
		e->teammaster = e;
		c++;
		c2++;
		for (j = i + 1, e2 = e + 1; j < level.num_entities; j++, e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (strcmp(e->team, e2->team) == 0)
			{
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if (e2->targetname)
				{
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

	//	trap->Print ("%i teams with %i entities\n", c, c2);
}

sharedBuffer_t gSharedBuffer;

void WP_SaberLoadParms();
void BG_VehicleLoadParms(void);

static void G_CacheGametype(void)
{
	// check some things
	if (g_gametype.string[0] && isalpha(g_gametype.string[0]))
	{
		const int gt = BG_GetGametypeForString(g_gametype.string);
		if (gt == -1)
		{
			trap->Print("Gametype '%s' unrecognised, defaulting to FFA/Deathmatch\n", g_gametype.string);
			level.gametype = GT_FFA;
		}
		else
			level.gametype = gt;
	}
	else if (g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE)
	{
		trap->Print("g_gametype %i is out of range, defaulting to 0 (FFA/Deathmatch)\n", g_gametype.integer);
		level.gametype = GT_FFA;
	}
	else
		level.gametype = atoi(g_gametype.string);

	trap->Cvar_Set("g_gametype", va("%i", level.gametype));
	trap->Cvar_Update(&g_gametype);
}

static void G_CacheMapname(const vmCvar_t* mapname)
{
	Com_sprintf(level.mapname, sizeof level.mapname, "maps/%s.bsp", mapname->string);
	//Com_sprintf( level.rawmapname, sizeof( level.rawmapname ), "maps/%s", mapname->string );
}

// creates a ctf flag spawn point
extern void sje_set_entity_field(gentity_t* ent, const char* key, const char* value);
extern void sje_spawn_entity(gentity_t* ent);
extern void sje_main_set_entity_field(const gentity_t* ent, const char* key, const char* value);
extern void sje_main_spawn_entity(gentity_t* ent);

static void sje_create_info_player_deathmatch(const int x, const int y, const int z, const int yaw)
{
	gentity_t* spawn_ent = G_Spawn();
	if (spawn_ent)
	{
		const gentity_t* spawn_point_ent = NULL;

		for (int i = 0; i < level.num_entities; i++)
		{
			const gentity_t* this_ent = &g_entities[i];
			if (Q_stricmp(this_ent->classname, "info_player_deathmatch") == 0)
			{
				// found the original SP map spawn point
				spawn_point_ent = this_ent;
				break;
			}
		}

		sje_set_entity_field(spawn_ent, "classname", "info_player_deathmatch");
		sje_set_entity_field(spawn_ent, "origin", va("%d %d %d", x, y, z));
		sje_set_entity_field(spawn_ent, "angles", va("0 %d 0", yaw));
		if (spawn_point_ent && spawn_point_ent->target)
		{
			// setting the target for SP map spawn points so they will work properly
			sje_set_entity_field(spawn_ent, "target", spawn_point_ent->target);
		}

		sje_spawn_entity(spawn_ent);
	}
}

static void sje_create_ctf_flag_spawn(const int x, const int y, const int z, const qboolean redteam)
{
	gentity_t* spawn_ent = G_Spawn();
	if (spawn_ent)
	{
		if (redteam == qtrue)
			sje_set_entity_field(spawn_ent, "classname", "team_CTF_redflag");
		else
			sje_set_entity_field(spawn_ent, "classname", "team_CTF_blueflag");

		sje_set_entity_field(spawn_ent, "origin", va("%d %d %d", x, y, z));
		sje_spawn_entity(spawn_ent);
	}
}

// creates a ctf player spawn point
static void sje_create_ctf_player_spawn(const int x, const int y, const int z, const int yaw, const qboolean redteam,
	const qboolean team_begin_spawn_point)
{
	gentity_t* spawn_ent = G_Spawn();
	if (spawn_ent)
	{
		if (redteam == qtrue)
		{
			if (team_begin_spawn_point == qtrue)
				sje_set_entity_field(spawn_ent, "classname", "team_CTF_redplayer");
			else
				sje_set_entity_field(spawn_ent, "classname", "team_CTF_redspawn");
		}
		else
		{
			if (team_begin_spawn_point == qtrue)
				sje_set_entity_field(spawn_ent, "classname", "team_CTF_blueplayer");
			else
				sje_set_entity_field(spawn_ent, "classname", "team_CTF_bluespawn");
		}

		sje_set_entity_field(spawn_ent, "origin", va("%d %d %d", x, y, z));
		sje_set_entity_field(spawn_ent, "angles", va("0 %d 0", yaw));

		sje_spawn_entity(spawn_ent);
	}
}

// used to fix func_door entities in SP maps that wont work and must be removed without causing the door glitch
static void fix_sp_func_door(gentity_t* ent)
{
	ent->spawnflags = 0;
	ent->flags = 0;
	GlobalUse(ent, ent, ent);
	G_FreeEntity(ent);
}

/*
============
G_InitGame

============
*/
extern void RemoveAllWP(void);
extern void BG_ClearVehicleParseParms(void);
gentity_t* SelectRandomDeathmatchSpawnPoint(qboolean isbot);
extern void InitSpawnScriptValues(void);
void SP_info_jedimaster_start(gentity_t* ent);
extern void Load_Autosaves(void);
void SP_light(gentity_t* self);

void G_InitGame(int levelTime, int randomSeed, int restart)
{
	int i;
	vmCvar_t mapname;
	vmCvar_t ckSum;
	char serverinfo[MAX_INFO_STRING] = { 0 };

	// variable used in the SP buged maps fix
	char sje_mapname[128] = { 0 };

	//Init RMG to 0, it will be autoset to 1 if there is terrain on the level.
	trap->Cvar_Set("RMG", "0");
	RMG.integer = 0;

	//Clean up any client-server ghoul2 instance attachments that may still exist exe-side
	trap->G2API_CleanEntAttachments();

	BG_InitAnimsets(); //clear it out

	B_InitAlloc(); //make sure everything is clean

	trap->SV_RegisterSharedMemory(gSharedBuffer.raw);

	//Load external vehicle data
	BG_VehicleLoadParms();

	trap->Print("------- Game Initialization -------\n");
	trap->Print("gamename: %s\n", GAMEVERSION);
	trap->Print("gamedate: %s\n", SOURCE_DATE);

	srand(randomSeed);

	G_RegisterCvars();

	G_ProcessIPBans();

	G_InitMemory();

	// set some level globals
	memset(&level, 0, sizeof level);
	level.time = levelTime;
	level.startTime = levelTime;

	level.follow1 = level.follow2 = -1;

	level.snd_fry = G_SoundIndex("sound/player/fry.wav"); // FIXME standing in lava / slime

	level.snd_hack = G_SoundIndex("sound/player/hacking.wav");
	level.snd_medHealed = G_SoundIndex("sound/player/supp_healed.wav");
	level.snd_medSupplied = G_SoundIndex("sound/player/supp_supplied.wav");

	if (g_log.string[0])
	{
		trap->FS_Open(g_log.string, &level.logFile, g_logSync.integer ? FS_APPEND_SYNC : FS_APPEND);
		if (level.logFile)
			trap->Print("Logging to %s\n", g_log.string);
		else
			trap->Print("WARNING: Couldn't open logfile: %s\n", g_log.string);
	}
	else
		trap->Print("Not logging game events to disk.\n");

	trap->GetServerinfo(serverinfo, sizeof serverinfo);
	G_LogPrintf("------------------------------------------------------------\n");
	G_LogPrintf("InitGame: %s\n", serverinfo);

	if (g_securityLog.integer)
	{
		if (g_securityLog.integer == 1)
			trap->FS_Open(SECURITY_LOG, &level.security.log, FS_APPEND);
		else if (g_securityLog.integer == 2)
			trap->FS_Open(SECURITY_LOG, &level.security.log, FS_APPEND_SYNC);

		if (level.security.log)
			trap->Print("Logging to "SECURITY_LOG"\n");
		else
			trap->Print("WARNING: Couldn't open logfile: "SECURITY_LOG"\n");
	}
	else
		trap->Print("Not logging security events to disk.\n");

	G_LogWeaponInit();

	G_CacheGametype();

	G_InitWorldSession();

	IT_LoadWeatherParms();

	// initialize all entities for this game
	memset(g_entities, 0, MAX_GENTITIES * sizeof g_entities[0]);
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = sv_maxclients.integer;
	memset(g_clients, 0, MAX_CLIENTS * sizeof g_clients[0]);
	level.clients = g_clients;

	// set client fields on player ents
	for (i = 0; i < level.maxclients; i++)
	{
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		g_entities[i].classname = "clientslot";
	}

	// let the server system know where the entites are
	trap->LocateGameData((sharedEntity_t*)level.gentities, level.num_entities, sizeof(gentity_t),
		&level.clients[0].ps, sizeof level.clients[0]);

	//Load sabers.cfg data
	WP_SaberLoadParms();

	NPC_InitGame();

	TIMER_Clear();

	trap->ICARUS_Init();

	// reserve some spots for dead player bodies
	//InitBodyQue();

	InitSpawnScriptValues();

	clear_registered_items();

	//make sure saber data is loaded before this! (so we can precache the appropriate hilts)

	InitSiegeMode();

	trap->Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);
	G_CacheMapname(&mapname);
	trap->Cvar_Register(&ckSum, "sv_mapChecksum", "", CVAR_ROM);

	// navCalculatePaths	= ( trap->Nav_Load( mapname.string, ckSum.integer ) == qfalse );
	// commented line above. Was taking a lot of time to load some maps, example mp/duel7 and mp/siege_desert in FFA Mode
	// now it will always force calculating paths
	navCalculatePaths = qtrue;

	// getting mapname
	Q_strncpyz(sje_mapname, Info_ValueForKey(serverinfo, "mapname"), sizeof sje_mapname);
	strcpy(level.sjemapname, sje_mapname);

	level.is_vjun3_map = qfalse;
	level.is_t3_rift_map = qfalse;
	level.is_t3_hevil_map = qfalse;
	level.is_t1_fatal_map = qfalse;
	level.is_t1_danger_map = qfalse;
	level.is_t1_rail_map = qfalse;
	level.is_t1_sour_map = qfalse;
	level.is_t2_rouge_map = qfalse;
	level.is_t2_trip_map = qfalse;
	level.is_t3_byss_map = qfalse;

	if (Q_stricmp(sje_mapname, "vjun3") == 0)
	{
		level.is_vjun3_map = qtrue;
	}
	if (Q_stricmp(sje_mapname, "t3_rift") == 0)
	{
		level.is_t3_rift_map = qtrue;
	}
	if (Q_stricmp(sje_mapname, "t3_hevil") == 0)
	{
		level.is_t3_hevil_map = qtrue;
	}
	if (Q_stricmp(sje_mapname, "t1_fatal") == 0)
	{
		level.is_t1_fatal_map = qtrue;
	}
	if (Q_stricmp(sje_mapname, "t1_danger") == 0)
	{
		level.is_t1_danger_map = qtrue;
	}
	if (Q_stricmp(sje_mapname, "t1_rail") == 0)
	{
		level.is_t1_rail_map = qtrue;
	}
	if (Q_stricmp(sje_mapname, "t1_sour") == 0)
	{
		level.is_t1_sour_map = qtrue;
	}
	if (Q_stricmp(sje_mapname, "t2_rogue") == 0)
	{
		level.is_t2_rouge_map = qtrue;
	}
	if (Q_stricmp(sje_mapname, "t2_trip") == 0)
	{
		level.is_t2_trip_map = qtrue;
	}
	if (Q_stricmp(sje_mapname, "t3_byss") == 0)
	{
		level.is_t3_byss_map = qtrue;
	}

	if (Q_stricmp(sje_mapname, "yavin1") == 0 || Q_stricmp(sje_mapname, "yavin1b") == 0 ||
		Q_stricmp(sje_mapname, "yavin2") == 0 || Q_stricmp(sje_mapname, "t1_danger") == 0 ||
		Q_stricmp(sje_mapname, "t1_fatal") == 0 || Q_stricmp(sje_mapname, "t1_inter") == 0 ||
		Q_stricmp(sje_mapname, "t1_rail") == 0 || Q_stricmp(sje_mapname, "t1_sour") == 0 ||
		Q_stricmp(sje_mapname, "t1_surprise") == 0 || Q_stricmp(sje_mapname, "hoth2") == 0 ||
		Q_stricmp(sje_mapname, "hoth3") == 0 || Q_stricmp(sje_mapname, "t2_dpred") == 0 ||
		Q_stricmp(sje_mapname, "t2_rancor") == 0 || Q_stricmp(sje_mapname, "t2_rogue") == 0 ||
		Q_stricmp(sje_mapname, "t2_trip") == 0 || Q_stricmp(sje_mapname, "t2_wedge") == 0 ||
		Q_stricmp(sje_mapname, "vjun1") == 0 || Q_stricmp(sje_mapname, "vjun2") == 0 ||
		Q_stricmp(sje_mapname, "vjun3") == 0 || Q_stricmp(sje_mapname, "t3_bounty") == 0 ||
		Q_stricmp(sje_mapname, "t3_byss") == 0 || Q_stricmp(sje_mapname, "t3_hevil") == 0 ||
		Q_stricmp(sje_mapname, "t3_rift") == 0 || Q_stricmp(sje_mapname, "t3_stamp") == 0 ||
		Q_stricmp(sje_mapname, "taspir1") == 0 || Q_stricmp(sje_mapname, "taspir2") == 0 ||
		Q_stricmp(sje_mapname, "kor1") == 0 || Q_stricmp(sje_mapname, "kor2") == 0 ||
		Q_stricmp(sje_mapname, "kejim_post") == 0 || Q_stricmp(sje_mapname, "kejim_base") == 0 ||
		Q_stricmp(sje_mapname, "artus_mine") == 0 || Q_stricmp(sje_mapname, "artus_detention") == 0 ||
		Q_stricmp(sje_mapname, "artus_topside") == 0 || Q_stricmp(sje_mapname, "valley") == 0 ||
		Q_stricmp(sje_mapname, "yavin_temple") == 0 || Q_stricmp(sje_mapname, "yavin_trial") == 0 ||
		Q_stricmp(sje_mapname, "ns_streets") == 0 || Q_stricmp(sje_mapname, "ns_hiedeout") == 0 ||
		Q_stricmp(sje_mapname, "ns_starpad") == 0 || Q_stricmp(sje_mapname, "bespin_undercity") == 0 ||
		Q_stricmp(sje_mapname, "bespin_streets") == 0 || Q_stricmp(sje_mapname, "bespin_platform") == 0 ||
		Q_stricmp(sje_mapname, "cairn_bay") == 0 || Q_stricmp(sje_mapname, "cairn_assembly") == 0 ||
		Q_stricmp(sje_mapname, "cairn_reactor") == 0 || Q_stricmp(sje_mapname, "cairn_dock1") == 0 ||
		Q_stricmp(sje_mapname, "doom_comm") == 0 || Q_stricmp(sje_mapname, "doom_detention") == 0 ||
		Q_stricmp(sje_mapname, "doom_shields") == 0 || Q_stricmp(sje_mapname, "yavin_swamp") == 0 ||
		Q_stricmp(sje_mapname, "yavin_canyon") == 0 || Q_stricmp(sje_mapname, "yavin_courtyard") == 0 ||
		Q_stricmp(sje_mapname, "yavin_final") == 0 || Q_stricmp(sje_mapname, "pit") == 0 ||
		Q_stricmp(sje_mapname, "dtention") == 0 || Q_stricmp(sje_mapname, "fuelstat") == 0 ||
		Q_stricmp(sje_mapname, "gromas") == 0 || Q_stricmp(sje_mapname, "jabship") == 0 ||
		Q_stricmp(sje_mapname, "narshada") == 0 || Q_stricmp(sje_mapname, "robotics") == 0 ||
		Q_stricmp(sje_mapname, "secbase") == 0 || Q_stricmp(sje_mapname, "sewers") == 0 ||
		Q_stricmp(sje_mapname, "talay") == 0 || Q_stricmp(sje_mapname, "testbase") == 0)
	{
		level.sp_map = qtrue;
	}

	if (Q_stricmp(sje_mapname, "kejim_post") == 0 || Q_stricmp(sje_mapname, "kejim_base") == 0 ||
		Q_stricmp(sje_mapname, "artus_mine") == 0 || Q_stricmp(sje_mapname, "artus_detention") == 0 ||
		Q_stricmp(sje_mapname, "artus_topside") == 0 || Q_stricmp(sje_mapname, "valley") == 0 ||
		Q_stricmp(sje_mapname, "yavin_temple") == 0 || Q_stricmp(sje_mapname, "yavin_trial") == 0 ||
		Q_stricmp(sje_mapname, "ns_streets") == 0 || Q_stricmp(sje_mapname, "ns_hiedeout") == 0 ||
		Q_stricmp(sje_mapname, "ns_starpad") == 0 || Q_stricmp(sje_mapname, "bespin_undercity") == 0 ||
		Q_stricmp(sje_mapname, "bespin_streets") == 0 || Q_stricmp(sje_mapname, "bespin_platform") == 0 ||
		Q_stricmp(sje_mapname, "cairn_bay") == 0 || Q_stricmp(sje_mapname, "cairn_assembly") == 0 ||
		Q_stricmp(sje_mapname, "cairn_reactor") == 0 || Q_stricmp(sje_mapname, "cairn_dock1") == 0 ||
		Q_stricmp(sje_mapname, "doom_comm") == 0 || Q_stricmp(sje_mapname, "doom_detention") == 0 ||
		Q_stricmp(sje_mapname, "doom_shields") == 0 || Q_stricmp(sje_mapname, "yavin_swamp") == 0 ||
		Q_stricmp(sje_mapname, "yavin_canyon") == 0 || Q_stricmp(sje_mapname, "yavin_courtyard") == 0 ||
		Q_stricmp(sje_mapname, "yavin_final") == 0 ||
		Q_stricmp(sje_mapname, "dtention") == 0 || Q_stricmp(sje_mapname, "fuelstat") == 0 ||
		Q_stricmp(sje_mapname, "gromas") == 0 || Q_stricmp(sje_mapname, "jabship") == 0 ||
		Q_stricmp(sje_mapname, "narshada") == 0 || Q_stricmp(sje_mapname, "robotics") == 0 ||
		Q_stricmp(sje_mapname, "secbase") == 0 || Q_stricmp(sje_mapname, "sewers") == 0 ||
		Q_stricmp(sje_mapname, "talay") == 0 || Q_stricmp(sje_mapname, "testbase") == 0)
	{
		level.is_outcast_map = qtrue;
	}

	if (Q_stricmp(sje_mapname, "ctf_bespin") == 0 || Q_stricmp(sje_mapname, "ctf_imperial") == 0 ||
		Q_stricmp(sje_mapname, "ctf_ns_streets") == 0 || Q_stricmp(sje_mapname, "ctf_yavin") == 0 ||
		Q_stricmp(sje_mapname, "duel_bay") == 0 || Q_stricmp(sje_mapname, "duel_bespin") == 0 ||
		Q_stricmp(sje_mapname, "duel_carbon") == 0 || Q_stricmp(sje_mapname, "duel_hangar") == 0 ||
		Q_stricmp(sje_mapname, "duel_jedi") == 0 || Q_stricmp(sje_mapname, "duel_pit") == 0 ||
		Q_stricmp(sje_mapname, "duel_temple") == 0 || Q_stricmp(sje_mapname, "duel_training") == 0 ||
		Q_stricmp(sje_mapname, "ffa_bespin") == 0 || Q_stricmp(sje_mapname, "ffa_deathstar") == 0 ||
		Q_stricmp(sje_mapname, "ffa_imperial") == 0 || Q_stricmp(sje_mapname, "ffa_ns_hideout") == 0 ||
		Q_stricmp(sje_mapname, "ffa_ns_streets") == 0 || Q_stricmp(sje_mapname, "ffa_raven") == 0 ||
		Q_stricmp(sje_mapname, "mp/ffa_bonus1") == 0 || Q_stricmp(sje_mapname, "mp/ffa_bonus2") == 0 ||
		Q_stricmp(sje_mapname, "mp/ffa_bonus2") == 0 || Q_stricmp(sje_mapname, "ctf_bonus1") == 0 ||
		Q_stricmp(sje_mapname, "ffa_yavin") == 0)
	{
		level.is_outcast_mp_map = qtrue;
	}

	if (Q_stricmp(sje_mapname, "mp/duel1") == 0 || Q_stricmp(sje_mapname, "mp/duel2") == 0 ||
		Q_stricmp(sje_mapname, "mp/duel3") == 0 || Q_stricmp(sje_mapname, "mp/duel4") == 0 ||
		Q_stricmp(sje_mapname, "mp/duel5") == 0 || Q_stricmp(sje_mapname, "mp/duel6") == 0 ||
		Q_stricmp(sje_mapname, "mp/duel7") == 0 || Q_stricmp(sje_mapname, "mp/duel8") == 0 ||
		Q_stricmp(sje_mapname, "mp/duel9") == 0 || Q_stricmp(sje_mapname, "mp/duel10") == 0)
	{
		level.is_duel_mp_map = qtrue;
	}

	if (Q_stricmp(sje_mapname, "duel_hangar") == 0 && com_rend2.integer == 0)
	{
		level.is_no_Dlight_map = qtrue;
	}

	if (level.is_outcast_mp_map == qtrue || level.is_outcast_map == qtrue)
	{
		if (com_outcast.integer != 1)
		{
			trap->Cvar_Set("com_outcast", "1");
		}
	}
	else if (level.is_duel_mp_map == qtrue)
	{
		if (com_outcast.integer != 2)
		{
			trap->Cvar_Set("com_outcast", "2");
		}
	}
	else
	{
		trap->Cvar_Set("com_outcast", "0");
	}

	if (level.is_no_Dlight_map == qtrue)
	{
		if (r_Turn_Off_dynamiclight.integer != 1)
		{
			trap->Cvar_Set("r_turn_off_dynamiclight", "1");
		}
	}
	else
	{
		trap->Cvar_Set("r_turn_off_dynamiclight", "0");
	}

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString(qfalse);

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	if (level.gametype >= GT_TEAM)
	{
		G_CheckTeamItems();
	}
	else if (level.gametype == GT_JEDIMASTER)
	{
		trap->SetConfigstring(CS_CLIENT_JEDIMASTER, "-1");
	}

	if (level.gametype == GT_POWERDUEL)
	{
		trap->SetConfigstring(CS_CLIENT_DUELISTS, va("-1|-1|-1"));
	}
	else
	{
		trap->SetConfigstring(CS_CLIENT_DUELISTS, va("-1|-1"));
	}
	// nmckenzie: DUEL_HEALTH: Default.
	trap->SetConfigstring(CS_CLIENT_DUELHEALTHS, va("-1|-1|!"));
	trap->SetConfigstring(CS_CLIENT_DUELWINNER, va("-1"));

	save_registered_items();

	//trap->Print ("-----------------------------------\n");

	if (level.gametype == GT_SINGLE_PLAYER || trap->Cvar_VariableIntegerValue("com_buildScript"))
	{
		G_model_index(SP_PODIUM_MODEL);
		G_SoundIndex("sound/player/gurp1.wav");
		G_SoundIndex("sound/player/gurp2.wav");
	}

	if (trap->Cvar_VariableIntegerValue("bot_enable"))
	{
		bot_ai_setup(restart);
		BotAILoadMap();
		G_InitBots();
	}
	else
	{
		if (level.gametype != GT_SINGLE_PLAYER)
		{
			g_load_arenas();
		}
		else
		{
			g_load_sp_arenas();
		}
	}

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		G_LogPrintf("Duel Tournament Begun: kill limit %d, win limit: %d\n", fraglimit.integer, duel_fraglimit.integer);
	}

	if (navCalculatePaths)
	{
		//not loaded - need to calc paths
		navCalcPathTime = level.time + START_TIME_NAV_CALC; //make sure all ents are in and linked
	}
	else
	{
		//loaded
		trap->Nav_SetPathsCalculated(qtrue);
		//need to do this, because combatpoint waypoints aren't saved out...?
		CP_FindCombatPointWaypoints();
		navCalcPathTime = 0;
	}

	//just get these configstrings registered now...
	if (level.gametype == GT_SIEGE)
	{
		//just get these configstrings registered now...
		while (i < MAX_CUSTOM_SIEGE_SOUNDS)
		{
			if (!bg_customSiegeSoundNames[i])
			{
				break;
			}
			G_SoundIndex((char*)bg_customSiegeSoundNames[i]);
			i++;
		}
	}

	if (level.gametype == GT_SINGLE_PLAYER)
	{
		//load in all the manually added savepoints
		Load_Autosaves();
	}

	if (level.gametype == GT_JEDIMASTER)
	{
		gentity_t* ent = NULL;
		int i1 = 0;
		for (i1 = 0, ent = g_entities; i1 < level.num_entities; i1++, ent++)
		{
			if (ent->isSaberEntity)
				break;
		}

		if (i1 == level.num_entities)
		{
			// no JM saber found. drop one at one of the player spawnpoints
			gentity_t* spawnpoint = SelectRandomDeathmatchSpawnPoint(qfalse);

			if (!spawnpoint)
			{
				trap->Error(ERR_DROP, "Couldn't find an FFA spawnpoint to drop the jedimaster saber at!\n");
				return;
			}

			ent = G_Spawn();
			G_SetOrigin(ent, spawnpoint->s.origin);
			SP_info_jedimaster_start(ent);
		}
	}

#ifdef SUN
	{
		gentity_t* sun;
		int i = 0;
		for (i = 0; i < level.num_entities; i++)
		{
			light = &g_entities[i];

			if (light)
			{
				if (Q_stricmp(light->classname, "light") == 0)
				{
					sun = &g_entities[i];
					sun = light;
					G_FreeEntity(light);
				}
			}
		}
		sun = G_Spawn();
		sun->classname = "light";
		sun->targetname = "SUN";
		SP_light(sun);
	}
#endif

	if (level.sp_map == qtrue)
	{
		if (level.is_t2_rouge_map == qtrue && g_allowNPC.integer)
		{
		}
		else
		{
			int i1 = 0;
			gentity_t* ent;
			for (i1 = 0; i1 < level.num_entities; i1++)
				// dont do map change at end of levels so players can stay on maps and erm...play
			{
				ent = &g_entities[i1];
				if (Q_stricmp(ent->targetname, "end_level") == 0)
				{
					// remove the map change entity
					G_FreeEntity(ent);
				}
			}
		}

		// added this fix for SP maps
		if (Q_stricmp(sje_mapname, "academy1") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(-1308, 272, 729, -90);
			sje_create_info_player_deathmatch(-1508, 272, 729, -90);
		}
		else if (Q_stricmp(sje_mapname, "academy2") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(-1308, 272, 729, -90);
			sje_create_info_player_deathmatch(-1508, 272, 729, -90);
		}
		else if (Q_stricmp(sje_mapname, "academy3") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(-1308, 272, 729, -90);
			sje_create_info_player_deathmatch(-1508, 272, 729, -90);
		}
		else if (Q_stricmp(sje_mapname, "academy4") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(-1308, 272, 729, -90);
			sje_create_info_player_deathmatch(-1508, 272, 729, -90);
		}
		else if (Q_stricmp(sje_mapname, "academy5") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(-1308, 272, 729, -90);
			sje_create_info_player_deathmatch(-1508, 272, 729, -90);
		}
		else if (Q_stricmp(sje_mapname, "academy6") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(-1308, 272, 729, -90);
			sje_create_info_player_deathmatch(-1508, 272, 729, -90);

			// hangar spawn points
			sje_create_info_player_deathmatch(-23, 458, -486, 0);
			sje_create_info_player_deathmatch(2053, 3401, -486, -90);
			sje_create_info_player_deathmatch(4870, 455, -486, -179);
		}
		else if (Q_stricmp(sje_mapname, "yavin1") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(472, -4833, 437, 74);
			sje_create_info_player_deathmatch(-167, -4046, 480, 0);
		}
		else if (Q_stricmp(sje_mapname, "yavin1b") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (Q_stricmp(ent->targetname, "door1") == 0)
				{
					fix_sp_func_door(ent);
				}
				else if (Q_stricmp(ent->classname, "trigger_hurt") == 0 && Q_stricmp(
					ent->targetname, "tree_hurt_trigger") != 0)
				{
					// trigger_hurt entity of the bridge area
					G_FreeEntity(ent);
				}
			}
			sje_create_info_player_deathmatch(472, -4833, 437, 74);
			sje_create_info_player_deathmatch(-167, -4046, 480, 0);
		}
		else if (Q_stricmp(sje_mapname, "yavin2") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (Q_stricmp(ent->targetname, "t530") == 0 || Q_stricmp(ent->targetname, "Putz_door") == 0 ||
					Q_stricmp(ent->targetname, "afterdroid_door") == 0 || Q_stricmp(ent->targetname, "pit_door") == 0 ||
					Q_stricmp(ent->targetname, "door1") == 0)
				{
					fix_sp_func_door(ent);
				}
				else if (Q_stricmp(ent->classname, "trigger_hurt") == 0 && ent->spawnflags == 62)
				{
					// removes the trigger hurt entity of the second bridge
					G_FreeEntity(ent);
				}
			}
			sje_create_info_player_deathmatch(2516, -5593, 89, -179);
			sje_create_info_player_deathmatch(2516, -5443, 89, -179);
		}
		else if (Q_stricmp(sje_mapname, "hoth2") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(-2114, 10195, 1027, -14);
			sje_create_info_player_deathmatch(-1808, 9640, 982, -17);
		}
		else if (Q_stricmp(sje_mapname, "hoth3") == 0)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (i2 == 232 || i2 == 233)
				{
					// fixing the final door
					ent->targetname = NULL;
					sje_main_set_entity_field(ent, "targetname", "sjeremovekey");
					sje_main_spawn_entity(ent);
				}
			}

			if (level.gametype != GT_SINGLE_PLAYER)
			{
				sje_create_info_player_deathmatch(-1908, 562, 992, -90);
				sje_create_info_player_deathmatch(-1907, 356, 801, -90);
			}
		}
		else if (Q_stricmp(sje_mapname, "t1_danger") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (Q_stricmp(ent->classname, "NPC_Monster_Sand_Creature") == 0)
				{
					// remove the map change entity
					G_FreeEntity(ent);
				}
			}

			sje_create_info_player_deathmatch(-3705, -3362, 1121, 90);
			sje_create_info_player_deathmatch(-3705, -2993, 1121, 90);
		}
		else if (Q_stricmp(sje_mapname, "t1_fatal") == 0)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];

				if (Q_stricmp(ent->targetname, "door_trap") == 0)
				{
					// zyk: fixing this door so it will not lock
					fix_sp_func_door(ent);
				}

				if (Q_stricmp(ent->targetname, "lobbydoor1") == 0 || Q_stricmp(ent->targetname, "lobbydoor2") == 0 ||
					Q_stricmp(ent->targetname, "t7708018") == 0 || Q_stricmp(ent->targetname, "t7708017") == 0)
				{
					// zyk: fixing these doors so they will not lock
					GlobalUse(ent, ent, ent);
				}

				if (i2 == 443)
				{
					// trigger_hurt at the spawn area
					G_FreeEntity(ent);
				}
			}

			if (level.gametype != GT_SINGLE_PLAYER)
			{
				sje_create_info_player_deathmatch(-1563, -4241, 4569, -157);
				sje_create_info_player_deathmatch(-1135, -4303, 4569, 179);
			}

			if (level.gametype == GT_CTF)
			{
				// in CTF, add the team player spawns
				sje_create_ctf_player_spawn(-3083, -2683, 4696, -90, qtrue, qtrue);
				sje_create_ctf_player_spawn(-2371, -3325, 4536, 90, qtrue, qtrue);
				sje_create_ctf_player_spawn(-1726, -2957, 4536, 90, qtrue, qtrue);

				sje_create_ctf_player_spawn(1277, 2947, 4540, -45, qfalse, qtrue);
				sje_create_ctf_player_spawn(3740, 482, 4536, 135, qfalse, qtrue);
				sje_create_ctf_player_spawn(2489, 1451, 4536, 135, qfalse, qtrue);

				sje_create_ctf_player_spawn(-3083, -2683, 4696, -90, qtrue, qfalse);
				sje_create_ctf_player_spawn(-2371, -3325, 4536, 90, qtrue, qfalse);
				sje_create_ctf_player_spawn(-1726, -2957, 4536, 90, qtrue, qfalse);

				sje_create_ctf_player_spawn(1277, 2947, 4540, -45, qfalse, qfalse);
				sje_create_ctf_player_spawn(3740, 482, 4536, 135, qfalse, qfalse);
				sje_create_ctf_player_spawn(2489, 1451, 4536, 135, qfalse, qfalse);

				sje_create_ctf_flag_spawn(-2366, -2561, 4536, qtrue);
				sje_create_ctf_flag_spawn(2484, 1732, 4656, qfalse);
			}
		}
		else if (Q_stricmp(sje_mapname, "t1_inter") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(-65, -686, 89, 90);
			sje_create_info_player_deathmatch(56, -686, 89, 90);
		}
		else if (Q_stricmp(sje_mapname, "t1_rail") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(-3135, 1, 33, 0);
			sje_create_info_player_deathmatch(-3135, 197, 25, 0);

			if (level.gametype == GT_CTF)
			{
				// in CTF, add the team player spawns
				sje_create_ctf_player_spawn(-2569, -2, 25, 179, qtrue, qtrue);
				sje_create_ctf_player_spawn(-1632, 257, 136, -90, qtrue, qtrue);
				sje_create_ctf_player_spawn(-1743, 0, 500, 0, qtrue, qtrue);

				sje_create_ctf_player_spawn(22760, -128, 152, 90, qfalse, qtrue);
				sje_create_ctf_player_spawn(22866, 0, 440, -179, qfalse, qtrue);
				sje_create_ctf_player_spawn(21102, 2, 464, 179, qfalse, qtrue);

				sje_create_ctf_player_spawn(-2569, -2, 25, 179, qtrue, qfalse);
				sje_create_ctf_player_spawn(-1632, 257, 136, -90, qtrue, qfalse);
				sje_create_ctf_player_spawn(-1743, 0, 500, 0, qtrue, qfalse);

				sje_create_ctf_player_spawn(22760, -128, 152, 90, qfalse, qfalse);
				sje_create_ctf_player_spawn(22866, 0, 440, -179, qfalse, qfalse);
				sje_create_ctf_player_spawn(21102, 2, 464, 179, qfalse, qfalse);

				sje_create_ctf_flag_spawn(-2607, -4, 24, qtrue);
				sje_create_ctf_flag_spawn(23146, -3, 216, qfalse);
			}
		}
		else if (Q_stricmp(sje_mapname, "t1_sour") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(9828, -5521, 153, 90);
			sje_create_info_player_deathmatch(9845, -5262, 153, 153);
		}
		else if (Q_stricmp(sje_mapname, "t1_surprise") == 0)
		{
			int i2 = 0;
			gentity_t* ent;
			qboolean found_bugged_switch = qfalse;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];

				if (Q_stricmp(ent->targetname, "fire_hurt") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->targetname, "droid_door") == 0)
				{
					fix_sp_func_door(ent);
				}
				if (Q_stricmp(ent->targetname, "tube_door") == 0)
				{
					fix_sp_func_door(ent);
				}
				if (found_bugged_switch == qfalse && Q_stricmp(ent->classname, "misc_model_breakable") == 0 &&
					Q_stricmp(ent->model, "models/map_objects/desert/switch3.md3") == 0)
				{
					G_FreeEntity(ent);
					found_bugged_switch = qtrue;
				}
				if (Q_stricmp(ent->classname, "func_static") == 0 && (int)ent->s.origin[0] == 3064 && (int)ent->s.origin
					[1] == 5040 && (int)ent->s.origin[2] == 892)
				{
					// elevator inside sand crawler near the wall fire
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->classname, "func_door") == 0 && i2 > 200 && Q_stricmp(ent->model, "*63") == 0)
				{
					// tube door in which the droid goes in SP
					G_FreeEntity(ent);
				}
			}

			if (level.gametype != GT_SINGLE_PLAYER)
			{
				sje_create_info_player_deathmatch(1913, -6151, 222, 153);
				sje_create_info_player_deathmatch(1921, -5812, 222, -179);
			}

			if (level.gametype == GT_CTF)
			{
				// in CTF, add the team player spawns
				sje_create_ctf_player_spawn(1948, -6020, 222, 138, qtrue, qtrue);
				sje_create_ctf_player_spawn(1994, -4597, 908, 19, qtrue, qtrue);
				sje_create_ctf_player_spawn(404, -4521, 249, -21, qtrue, qtrue);

				sje_create_ctf_player_spawn(2341, 4599, 1056, 83, qfalse, qtrue);
				sje_create_ctf_player_spawn(1901, 5425, 916, -177, qfalse, qtrue);
				sje_create_ctf_player_spawn(918, 3856, 944, 0, qfalse, qtrue);

				sje_create_ctf_player_spawn(1948, -6020, 222, 138, qtrue, qfalse);
				sje_create_ctf_player_spawn(1994, -4597, 908, 19, qtrue, qfalse);
				sje_create_ctf_player_spawn(404, -4521, 249, -21, qtrue, qfalse);

				sje_create_ctf_player_spawn(2341, 4599, 1056, 83, qfalse, qfalse);
				sje_create_ctf_player_spawn(1901, 5425, 916, -177, qfalse, qfalse);
				sje_create_ctf_player_spawn(918, 3856, 944, 0, qfalse, qfalse);

				sje_create_ctf_flag_spawn(1337, -6492, 224, qtrue);
				sje_create_ctf_flag_spawn(2098, 4966, 800, qfalse);
			}
		}
		else if (Q_stricmp(sje_mapname, "t2_rancor") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];

				if (Q_stricmp(ent->targetname, "t857") == 0)
				{
					fix_sp_func_door(ent);
				}
				if (Q_stricmp(ent->targetname, "Kill_Brush_Canyon") == 0)
				{
					// trigger_hurt at the spawn area
					G_FreeEntity(ent);
				}
			}
			sje_create_info_player_deathmatch(-898, 1178, 1718, 90);
			sje_create_info_player_deathmatch(-898, 1032, 1718, 90);
		}
		else if (Q_stricmp(sje_mapname, "t2_rogue") == 0)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (Q_stricmp(ent->targetname, "t475") == 0)
				{
					// remove the invisible wall at the end of the bridge at start
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->target, "field_counter1") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->target, "field_counter2") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->target, "field_counter3") == 0)
				{
					G_FreeEntity(ent);
				}

				if (level.gametype != GT_SINGLE_PLAYER)
				{
					if (Q_stricmp(ent->targetname, "ractoroomdoor") == 0)
					{
						// remove office door
						G_FreeEntity(ent);
					}
					if (i2 == 142)
					{
						// remove the elevator
						G_FreeEntity(ent);
					}
					if (i2 == 166)
					{
						// remove the elevator button
						G_FreeEntity(ent);
					}
				}
			}

			if (level.gametype != GT_SINGLE_PLAYER)
			{
				// adding new elevator and buttons that work properly
				ent = G_Spawn();

				sje_main_set_entity_field(ent, "classname", "func_plat");
				sje_main_set_entity_field(ent, "spawnflags", "4096");
				sje_main_set_entity_field(ent, "targetname", "sje_lift_1");
				sje_main_set_entity_field(ent, "lip", "8");
				sje_main_set_entity_field(ent, "height", "1280");
				sje_main_set_entity_field(ent, "speed", "200");
				sje_main_set_entity_field(ent, "model", "*38");
				sje_main_set_entity_field(ent, "origin", "2848 2144 700");
				sje_main_set_entity_field(ent, "soundSet", "platform");

				sje_main_spawn_entity(ent);

				ent = G_Spawn();

				sje_main_set_entity_field(ent, "classname", "trigger_multiple");
				sje_main_set_entity_field(ent, "spawnflags", "4");
				sje_main_set_entity_field(ent, "target", "sje_lift_1");
				sje_main_set_entity_field(ent, "origin", "2664 2000 728");
				sje_main_set_entity_field(ent, "mins", "-32 -32 -32");
				sje_main_set_entity_field(ent, "maxs", "32 32 32");
				sje_main_set_entity_field(ent, "wait", "1");
				sje_main_set_entity_field(ent, "delay", "2");

				sje_main_spawn_entity(ent);

				ent = G_Spawn();

				sje_main_set_entity_field(ent, "classname", "trigger_multiple");
				sje_main_set_entity_field(ent, "spawnflags", "4");
				sje_main_set_entity_field(ent, "target", "sje_lift_1");
				sje_main_set_entity_field(ent, "origin", "2577 2023 -551");
				sje_main_set_entity_field(ent, "mins", "-32 -32 -32");
				sje_main_set_entity_field(ent, "maxs", "32 32 32");
				sje_main_set_entity_field(ent, "wait", "1");
				sje_main_set_entity_field(ent, "delay", "2");

				sje_main_spawn_entity(ent);
			}

			sje_create_info_player_deathmatch(1974, -1983, -550, 90);
			sje_create_info_player_deathmatch(1779, -1983, -550, 90);
		}
		else if (Q_stricmp(sje_mapname, "t2_trip") == 0)
		{
			if (!g_allowNPC.integer)
			{
				gentity_t* ent;
				int i2 = 0;
				for (i2 = 0; i2 < level.num_entities; i2++)
				{
					ent = &g_entities[i2];

					if (Q_stricmp(ent->targetname, "t546") == 0)
					{
						G_FreeEntity(ent);
					}
					else if (Q_stricmp(ent->targetname, "cin_door") == 0)
					{
						G_FreeEntity(ent);
					}
					else if (Q_stricmp(ent->targetname, "endJaden") == 0)
					{
						G_FreeEntity(ent);
					}
					else if (Q_stricmp(ent->targetname, "endJaden2") == 0)
					{
						G_FreeEntity(ent);
					}
					else if (Q_stricmp(ent->targetname, "endswoop") == 0)
					{
						G_FreeEntity(ent);
					}
					else if (Q_stricmp(ent->classname, "func_door") == 0 && i2 > 200)
					{
						// door after the teleports of the race mode
						G_FreeEntity(ent);
					}
					else if (Q_stricmp(ent->targetname, "t547") == 0)
					{
						// removes swoop at end of map. Must be removed to prevent bug in racemode
						G_FreeEntity(ent);
					}
				}
			}
			sje_create_info_player_deathmatch(-5698, -22304, 1705, 90);
			sje_create_info_player_deathmatch(-5433, -22328, 1705, 90);

			if (level.gametype == GT_CTF)
			{
				// in CTF, add the team player spawns
				sje_create_ctf_player_spawn(-20705, 18794, 1704, 0, qtrue, qtrue);
				sje_create_ctf_player_spawn(-20729, 17692, 1704, 0, qtrue, qtrue);
				sje_create_ctf_player_spawn(-20204, 18736, 1503, 0, qtrue, qtrue);

				sje_create_ctf_player_spawn(20494, -2922, 1672, 90, qfalse, qtrue);
				sje_create_ctf_player_spawn(19321, -2910, 1672, 90, qfalse, qtrue);
				sje_create_ctf_player_spawn(19428, -2404, 1470, 90, qfalse, qtrue);

				sje_create_ctf_player_spawn(-20705, 18794, 1704, 0, qtrue, qfalse);
				sje_create_ctf_player_spawn(-20729, 17692, 1704, 0, qtrue, qfalse);
				sje_create_ctf_player_spawn(-20204, 18736, 1503, 0, qtrue, qfalse);

				sje_create_ctf_player_spawn(20494, -2922, 1672, 90, qfalse, qfalse);
				sje_create_ctf_player_spawn(19321, -2910, 1672, 90, qfalse, qfalse);
				sje_create_ctf_player_spawn(19428, -2404, 1470, 90, qfalse, qfalse);

				sje_create_ctf_flag_spawn(-20421, 18244, 1704, qtrue);
				sje_create_ctf_flag_spawn(19903, -2638, 1672, qfalse);
			}
		}
		else if (Q_stricmp(sje_mapname, "t2_wedge") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(6328, 539, -110, -178);
			sje_create_info_player_deathmatch(6332, 743, -110, -178);
		}
		else if (Q_stricmp(sje_mapname, "t2_dpred") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (Q_stricmp(ent->targetname, "prisonshield1") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->targetname, "t556") == 0)
				{
					fix_sp_func_door(ent);
				}
				if (Q_stricmp(ent->target, "field_counter1") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->target, "t62241") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->target, "t62243") == 0)
				{
					G_FreeEntity(ent);
				}
			}

			sje_create_info_player_deathmatch(-2152, -3885, -134, 90);
			sje_create_info_player_deathmatch(-2152, -3944, -134, 90);

			if (level.gametype == GT_CTF)
			{
				// in CTF, add the team player spawns
				sje_create_ctf_player_spawn(0, -4640, 664, 90, qtrue, qtrue);
				sje_create_ctf_player_spawn(485, -3721, 632, -179, qtrue, qtrue);
				sje_create_ctf_player_spawn(-212, -3325, 656, -179, qtrue, qtrue);

				sje_create_ctf_player_spawn(0, 125, 24, -90, qfalse, qtrue);
				sje_create_ctf_player_spawn(-1242, 128, 24, 0, qfalse, qtrue);
				sje_create_ctf_player_spawn(369, 67, 296, -179, qfalse, qtrue);

				sje_create_ctf_player_spawn(0, -4640, 664, 90, qtrue, qfalse);
				sje_create_ctf_player_spawn(485, -3721, 632, -179, qtrue, qfalse);
				sje_create_ctf_player_spawn(-212, -3325, 656, -179, qtrue, qfalse);

				sje_create_ctf_player_spawn(0, 125, 24, -90, qfalse, qfalse);
				sje_create_ctf_player_spawn(-1242, 128, 24, 0, qfalse, qfalse);
				sje_create_ctf_player_spawn(369, 67, 296, -179, qfalse, qfalse);

				sje_create_ctf_flag_spawn(3, -3974, 664, qtrue);
				sje_create_ctf_flag_spawn(-701, 126, 24, qfalse);
			}
		}
		else if (Q_stricmp(sje_mapname, "vjun1") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			int i2 = 0;
			gentity_t* ent;
			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (i2 == 123 || i2 == 124)
				{
					// removing tie fighter misc_model_breakable entities to prevent client crashes
					G_FreeEntity(ent);
				}
			}
			sje_create_info_player_deathmatch(-6897, 7035, 857, -90);
			sje_create_info_player_deathmatch(-7271, 7034, 857, -90);
		}
		else if (Q_stricmp(sje_mapname, "vjun2") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(-831, 166, 217, 90);
			sje_create_info_player_deathmatch(-700, 166, 217, 90);
		}
		else if (Q_stricmp(sje_mapname, "vjun3") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(-8272, -391, 1433, 179);
			sje_create_info_player_deathmatch(-8375, -722, 1433, 179);
		}
		else if (Q_stricmp(sje_mapname, "t3_hevil") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (i2 == 42)
				{
					G_FreeEntity(ent);
				}
			}
			sje_create_info_player_deathmatch(512, -2747, -742, 90);
			sje_create_info_player_deathmatch(872, -2445, -742, 108);
		}
		else if (Q_stricmp(sje_mapname, "t3_bounty") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(-3721, -726, 73, 75);
			sje_create_info_player_deathmatch(-3198, -706, 73, 90);

			if (level.gametype == GT_CTF)
			{
				// in CTF, add the team player spawns
				sje_create_ctf_player_spawn(-7740, -543, -263, 0, qtrue, qtrue);
				sje_create_ctf_player_spawn(-8470, -210, 24, 90, qtrue, qtrue);
				sje_create_ctf_player_spawn(-7999, -709, -7, 132, qtrue, qtrue);

				sje_create_ctf_player_spawn(616, -978, 344, 0, qfalse, qtrue);
				sje_create_ctf_player_spawn(595, 482, 360, -90, qfalse, qtrue);
				sje_create_ctf_player_spawn(1242, 255, 36, -179, qfalse, qtrue);

				sje_create_ctf_player_spawn(-7740, -543, -263, 0, qtrue, qfalse);
				sje_create_ctf_player_spawn(-8470, -210, 24, 90, qtrue, qfalse);
				sje_create_ctf_player_spawn(-7999, -709, -7, 132, qtrue, qfalse);

				sje_create_ctf_player_spawn(616, -978, 344, 0, qfalse, qfalse);
				sje_create_ctf_player_spawn(595, 482, 360, -90, qfalse, qfalse);
				sje_create_ctf_player_spawn(1242, 255, 36, -179, qfalse, qfalse);

				sje_create_ctf_flag_spawn(-7538, -545, -327, qtrue);
				sje_create_ctf_flag_spawn(614, -509, 344, qfalse);
			}
		}
		else if (Q_stricmp(sje_mapname, "t3_byss") == 0)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];

				/*if (Q_stricmp(ent->targetname, "wall_door1") == 0)
				{
					fix_sp_func_door(ent);
				}*/
				if (Q_stricmp(ent->target, "field_counter1") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->targetname, "wave1_tie1") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->targetname, "wave1_tie2") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->targetname, "wave1_tie3") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->targetname, "wave2_tie1") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->targetname, "wave2_tie2") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->targetname, "wave2_tie3") == 0)
				{
					G_FreeEntity(ent);
				}
			}
			sje_create_info_player_deathmatch(968, 111, 25, -90);
			sje_create_info_player_deathmatch(624, 563, 25, -90);
		}
		else if (Q_stricmp(sje_mapname, "t3_rift") == 0)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (Q_stricmp(ent->targetname, "fakewall1") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->targetname, "t778") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->targetname, "t779") == 0)
				{
					G_FreeEntity(ent);
				}
			}

			sje_create_info_player_deathmatch(2195, 7611, 4380, -90);
			sje_create_info_player_deathmatch(2305, 7640, 4380, -90);
		}
		else if (Q_stricmp(sje_mapname, "t3_stamp") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(1208, 445, 89, 179);
			sje_create_info_player_deathmatch(1208, 510, 89, 179);
		}
		else if (Q_stricmp(sje_mapname, "taspir1") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (Q_stricmp(ent->targetname, "t278") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->targetname, "bldg2_ext_door") == 0)
				{
					fix_sp_func_door(ent);
				}
			}
			sje_create_info_player_deathmatch(-1609, -1792, 649, 112);
			sje_create_info_player_deathmatch(-1791, -1838, 649, 90);
		}
		else if (Q_stricmp(sje_mapname, "taspir2") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (Q_stricmp(ent->targetname, "force_field") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->targetname, "kill_toggle") == 0)
				{
					G_FreeEntity(ent);
				}
			}

			sje_create_info_player_deathmatch(286, -2859, 345, 92);
			sje_create_info_player_deathmatch(190, -2834, 345, 90);
		}
		else if (Q_stricmp(sje_mapname, "kor1") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (i2 >= 418 && i2 <= 422)
				{
					// remove part of the door on the floor on the first puzzle
					G_FreeEntity(ent);
				}
			}
			sje_create_info_player_deathmatch(190, 632, -1006, -89);
			sje_create_info_player_deathmatch(-249, 952, -934, -89);
		}
		else if (Q_stricmp(sje_mapname, "kor2") == 0 && level.gametype != GT_SINGLE_PLAYER)
		{
			sje_create_info_player_deathmatch(2977, 3137, -2526, 0);
			sje_create_info_player_deathmatch(3072, 2992, -2526, 0);
		}
		else if (Q_stricmp(sje_mapname, "mp/siege_korriban") == 0 && g_gametype.integer == GT_FFA && level.gametype !=
			GT_SINGLE_PLAYER)
		{
			// if its a FFA game, then remove some entities
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (Q_stricmp(ent->targetname, "cyrstalsinplace") == 0)
				{
					G_FreeEntity(ent);
				}
				if (i2 >= 236 && i2 <= 238)
				{
					// removing the trigger_hurt from the lava in Guardian of Universe arena
					G_FreeEntity(ent);
				}
			}
		}
		else if (Q_stricmp(sje_mapname, "mp/siege_desert") == 0 && g_gametype.integer == GT_FFA && level.gametype !=
			GT_SINGLE_PLAYER)
		{
			// if its a FFA game, then remove the shield in the final part
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (Q_stricmp(ent->targetname, "rebel_obj_2_doors") == 0)
				{
					fix_sp_func_door(ent);
				}
				if (Q_stricmp(ent->targetname, "shield") == 0)
				{
					G_FreeEntity(ent);
				}
				if (Q_stricmp(ent->targetname, "gatedestroy_doors") == 0)
				{
					G_FreeEntity(ent);
				}
				if (i2 >= 153 && i2 <= 160)
				{
					G_FreeEntity(ent);
				}
			}
		}
		else if (Q_stricmp(sje_mapname, "mp/siege_destroyer") == 0 && g_gametype.integer == GT_FFA && level.gametype !=
			GT_SINGLE_PLAYER)
		{
			// if its a FFA game, then remove the shield at the destroyer
			int i2 = 0;
			gentity_t* ent;

			for (i2 = 0; i2 < level.num_entities; i2++)
			{
				ent = &g_entities[i2];
				if (Q_stricmp(ent->targetname, "ubershield") == 0)
				{
					G_FreeEntity(ent);
				}
				else if (Q_stricmp(ent->classname, "info_player_deathmatch") == 0)
				{
					G_FreeEntity(ent);
				}
			}
			// rebel area spawnpoints
			sje_create_info_player_deathmatch(31729, -32219, 33305, 90);
			sje_create_info_player_deathmatch(31229, -32219, 33305, 90);
			sje_create_info_player_deathmatch(30729, -32219, 33305, 90);
			sje_create_info_player_deathmatch(32229, -32219, 33305, 90);
			sje_create_info_player_deathmatch(32729, -32219, 33305, 90);
			sje_create_info_player_deathmatch(31729, -32019, 33305, 90);

			// imperial area spawnpoints
			sje_create_info_player_deathmatch(2545, 8987, 1817, -90);
			sje_create_info_player_deathmatch(2345, 8987, 1817, -90);
			sje_create_info_player_deathmatch(2745, 8987, 1817, -90);

			sje_create_info_player_deathmatch(2597, 7403, 1817, 90);
			sje_create_info_player_deathmatch(2397, 7403, 1817, 90);
			sje_create_info_player_deathmatch(2797, 7403, 1817, 90);
		}
	}

	level.sp_map = qfalse;
	level.is_outcast_map = qfalse;
	level.is_outcast_mp_map = qfalse;
	level.is_duel_mp_map = qfalse;
	level.is_no_Dlight_map = qfalse;
}

/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame(const int restart)
{
	int i = 0;

	G_CleanAllFakeClients(); //get rid of dynamically allocated fake client structs.

	BG_ClearAnimsets(); //free all dynamic allocations made through the engine

	//	Com_Printf("... Gameside GHOUL2 Cleanup\n");
	while (i < MAX_GENTITIES)
	{
		//clean up all the ghoul2 instances
		gentity_t* ent = &g_entities[i];

		if (ent->ghoul2 && trap->G2API_HaveWeGhoul2Models(ent->ghoul2))
		{
			trap->G2API_CleanGhoul2Models(&ent->ghoul2);
			ent->ghoul2 = NULL;
		}
		if (ent->client)
		{
			int j = 0;

			while (j < MAX_SABERS)
			{
				if (ent->client->weaponGhoul2[j] && trap->G2API_HaveWeGhoul2Models(ent->client->weaponGhoul2[j]))
				{
					trap->G2API_CleanGhoul2Models(&ent->client->weaponGhoul2[j]);
				}
				j++;
			}
		}
		i++;
	}
	if (g2SaberInstance && trap->G2API_HaveWeGhoul2Models(g2SaberInstance))
	{
		trap->G2API_CleanGhoul2Models(&g2SaberInstance);
		g2SaberInstance = NULL;
	}
	if (precachedKyle && trap->G2API_HaveWeGhoul2Models(precachedKyle))
	{
		trap->G2API_CleanGhoul2Models(&precachedKyle);
		precachedKyle = NULL;
	}

	//	Com_Printf ("... ICARUS_Shutdown\n");
	trap->ICARUS_Shutdown(); //Shut ICARUS down

	//	Com_Printf ("... Reference Tags Cleared\n");
	TAG_Init(); //Clear the reference tags

	G_LogWeaponOutput();

	if (level.logFile)
	{
		G_LogPrintf("ShutdownGame:\n------------------------------------------------------------\n");
		trap->FS_Close(level.logFile);
		level.logFile = 0;
	}

	if (level.security.log)
	{
		G_SecurityLogPrintf("ShutdownGame\n\n");
		trap->FS_Close(level.security.log);
		level.security.log = 0;
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();

	trap->ROFF_Clean();

	if (trap->Cvar_VariableIntegerValue("bot_enable"))
	{
		bot_ai_shutdown(restart);
	}

	B_CleanupAlloc(); //clean up all allocations made with B_Alloc
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
AddTournamentPlayer

If there are less than two tournament players, put a
spectator in the game and restart
=============
*/
static void AddTournamentPlayer(void)
{
	if (level.numPlayingClients >= 2)
	{
		return;
	}

	// never change during intermission
	//	if ( level.intermissiontime ) {
	//		return;
	//	}

	const gclient_t* nextInLine = NULL;

	for (int i = 0; i < level.maxclients; i++)
	{
		const gclient_t* client = &level.clients[i];
		if (client->pers.connected != CON_CONNECTED)
		{
			continue;
		}
		if (!g_allowHighPingDuelist.integer && client->ps.ping >= 999)
		{
			//don't add people who are lagging out if cvar is not set to allow it.
			continue;
		}
		if (client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if (client->sess.spectatorState == SPECTATOR_SCOREBOARD ||
			client->sess.spectatorClient < 0)
		{
			continue;
		}

		if (!nextInLine || client->sess.spectatorNum > nextInLine->sess.spectatorNum)
			nextInLine = client;
	}

	if (!nextInLine)
	{
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam(&g_entities[nextInLine - level.clients], "f");
}

/*
=======================
AddTournamentQueue

Add client to end of tournament queue
=======================
*/

void AddTournamentQueue(const gclient_t* client)
{
	for (int index = 0; index < level.maxclients; index++)
	{
		gclient_t* curclient = &level.clients[index];

		if (curclient->pers.connected != CON_DISCONNECTED)
		{
			if (curclient == client)
				curclient->sess.spectatorNum = 0;
			else if (curclient->sess.sessionTeam == TEAM_SPECTATOR)
				curclient->sess.spectatorNum++;
		}
	}
}

/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
static void RemoveTournamentLoser(void)
{
	if (level.numPlayingClients != 2)
	{
		return;
	}

	const int clientNum = level.sortedClients[1];

	if (level.clients[clientNum].pers.connected != CON_CONNECTED)
	{
		return;
	}

	// make them a spectator
	SetTeam(&g_entities[clientNum], "s");
}

void G_PowerDuelCount(int* loners, int* doubles, const qboolean countSpec)
{
	int i = 0;

	while (i < MAX_CLIENTS)
	{
		const gclient_t* cl = g_entities[i].client;

		if (g_entities[i].inuse && cl && (countSpec || cl->sess.sessionTeam != TEAM_SPECTATOR))
		{
			if (cl->sess.duelTeam == DUELTEAM_LONE)
			{
				(*loners)++;
			}
			else if (cl->sess.duelTeam == DUELTEAM_DOUBLE)
			{
				(*doubles)++;
			}
		}
		i++;
	}
}

qboolean g_duelAssigning = qfalse;

static void AddPowerDuelPlayers(void)
{
	int loners = 0;
	int doubles = 0;
	int nonspecLoners = 0;
	int nonspecDoubles = 0;

	if (level.numPlayingClients >= 3)
	{
		return;
	}

	const gclient_t* nextInLine = NULL;

	G_PowerDuelCount(&nonspecLoners, &nonspecDoubles, qfalse);
	if (nonspecLoners >= 1 && nonspecDoubles >= 2)
	{
		//we have enough people, stop
		return;
	}

	//Could be written faster, but it's not enough to care I suppose.
	G_PowerDuelCount(&loners, &doubles, qtrue);

	if (loners < 1 || doubles < 2)
	{
		//don't bother trying to spawn anyone yet if the balance is not even set up between spectators
		return;
	}

	//Count again, with only in-game clients in mind.
	loners = nonspecLoners;
	doubles = nonspecDoubles;
	//	G_PowerDuelCount(&loners, &doubles, qfalse);

	for (int i = 0; i < level.maxclients; i++)
	{
		const gclient_t* client = &level.clients[i];
		if (client->pers.connected != CON_CONNECTED)
		{
			continue;
		}
		if (client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			continue;
		}
		if (client->sess.duelTeam == DUELTEAM_FREE)
		{
			continue;
		}
		if (client->sess.duelTeam == DUELTEAM_LONE && loners >= 1)
		{
			continue;
		}
		if (client->sess.duelTeam == DUELTEAM_DOUBLE && doubles >= 2)
		{
			continue;
		}

		// never select the dedicated follow or scoreboard clients
		if (client->sess.spectatorState == SPECTATOR_SCOREBOARD ||
			client->sess.spectatorClient < 0)
		{
			continue;
		}

		if (!nextInLine || client->sess.spectatorNum > nextInLine->sess.spectatorNum)
			nextInLine = client;
	}

	if (!nextInLine)
	{
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam(&g_entities[nextInLine - level.clients], "f");

	//Call recursively until everyone is in
	AddPowerDuelPlayers();
}

qboolean g_dontFrickinCheck = qfalse;

static void RemovePowerDuelLosers(void)
{
	int remClients[3];
	int remNum = 0;
	int i = 0;

	while (i < MAX_CLIENTS && remNum < 3)
	{
		//cl = &level.clients[level.sortedClients[i]];
		const gclient_t* cl = &level.clients[i];

		if (cl->pers.connected == CON_CONNECTED)
		{
			if ((cl->ps.stats[STAT_HEALTH] <= 0 || cl->iAmALoser) &&
				(cl->sess.sessionTeam != TEAM_SPECTATOR || cl->iAmALoser))
			{
				//he was dead or he was spectating as a loser
				remClients[remNum] = i;
				remNum++;
			}
		}

		i++;
	}

	if (!remNum)
	{
		//Time ran out or something? Oh well, just remove the main guy.
		remClients[remNum] = level.sortedClients[0];
		remNum++;
	}

	i = 0;
	while (i < remNum)
	{
		//set them all to spectator
		SetTeam(&g_entities[remClients[i]], "s");
		i++;
	}

	g_dontFrickinCheck = qfalse;

	//recalculate stuff now that we have reset teams.
	CalculateRanks();
}

static void RemoveDuelDrawLoser(void)
{
	int cl_failure;

	if (level.clients[level.sortedClients[0]].pers.connected != CON_CONNECTED)
	{
		return;
	}
	if (level.clients[level.sortedClients[1]].pers.connected != CON_CONNECTED)
	{
		return;
	}

	const int clFirst = level.clients[level.sortedClients[0]].ps.stats[STAT_HEALTH] + level.clients[level.sortedClients[0]].ps.stats[STAT_ARMOR];
	const int clSec = level.clients[level.sortedClients[1]].ps.stats[STAT_HEALTH] + level.clients[level.sortedClients[1]].ps.stats[STAT_ARMOR];

	if (clFirst > clSec)
	{
		cl_failure = 1;
	}
	else if (clSec > clFirst)
	{
		cl_failure = 0;
	}
	else
	{
		cl_failure = 2;
	}

	if (cl_failure != 2)
	{
		SetTeam(&g_entities[level.sortedClients[cl_failure]], "s");
	}
	else
	{
		//we could be more elegant about this, but oh well.
		SetTeam(&g_entities[level.sortedClients[1]], "s");
	}
}

/*
=======================
RemoveTournamentWinner
=======================
*/
static void RemoveTournamentWinner(void)
{
	if (level.numPlayingClients != 2)
	{
		return;
	}

	const int clientNum = level.sortedClients[0];

	if (level.clients[clientNum].pers.connected != CON_CONNECTED)
	{
		return;
	}

	// make them a spectator
	SetTeam(&g_entities[clientNum], "s");
}

/*
=======================
AdjustTournamentScores
=======================
*/
static void AdjustTournamentScores(void)
{
	int clientNum;

	if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
		level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
		level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
		level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
	{
		const int clFirst = level.clients[level.sortedClients[0]].ps.stats[STAT_HEALTH] + level.clients[level.
			sortedClients[0]].ps.stats[STAT_ARMOR];
		const int clSec = level.clients[level.sortedClients[1]].ps.stats[STAT_HEALTH] + level.clients[level.
			sortedClients[1]].ps.stats[STAT_ARMOR];
		int cl_failure;
		int cl_success;

		if (clFirst > clSec)
		{
			cl_failure = 1;
			cl_success = 0;
		}
		else if (clSec > clFirst)
		{
			cl_failure = 0;
			cl_success = 1;
		}
		else
		{
			cl_failure = 2;
			cl_success = 2;
		}

		if (cl_failure != 2)
		{
			clientNum = level.sortedClients[cl_success];

			level.clients[clientNum].sess.wins++;
			client_userinfo_changed(clientNum);
			trap->SetConfigstring(CS_CLIENT_DUELWINNER, va("%i", clientNum));

			clientNum = level.sortedClients[cl_failure];

			level.clients[clientNum].sess.losses++;
			client_userinfo_changed(clientNum);
		}
		else
		{
			cl_success = 0;
			cl_failure = 1;

			clientNum = level.sortedClients[cl_success];

			level.clients[clientNum].sess.wins++;
			client_userinfo_changed(clientNum);
			trap->SetConfigstring(CS_CLIENT_DUELWINNER, va("%i", clientNum));

			clientNum = level.sortedClients[cl_failure];

			level.clients[clientNum].sess.losses++;
			client_userinfo_changed(clientNum);
		}
	}
	else
	{
		clientNum = level.sortedClients[0];
		if (level.clients[clientNum].pers.connected == CON_CONNECTED)
		{
			level.clients[clientNum].sess.wins++;
			client_userinfo_changed(clientNum);

			trap->SetConfigstring(CS_CLIENT_DUELWINNER, va("%i", clientNum));
		}

		clientNum = level.sortedClients[1];
		if (level.clients[clientNum].pers.connected == CON_CONNECTED)
		{
			level.clients[clientNum].sess.losses++;
			client_userinfo_changed(clientNum);
		}
	}
}

/*
=============
SortRanks

=============
*/
static int QDECL SortRanks(const void* a, const void* b)
{
	const gclient_t* ca = &level.clients[*(int*)a];
	const gclient_t* cb = &level.clients[*(int*)b];

	if (level.gametype == GT_POWERDUEL)
	{
		//sort single duelists first
		if (ca->sess.duelTeam == DUELTEAM_LONE && ca->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return -1;
		}
		if (cb->sess.duelTeam == DUELTEAM_LONE && cb->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return 1;
		}

		//others will be auto-sorted below but above spectators.
	}

	// sort special clients last
	if (ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0)
	{
		return 1;
	}
	if (cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0)
	{
		return -1;
	}

	// then connecting clients
	if (ca->pers.connected == CON_CONNECTING)
	{
		return 1;
	}
	if (cb->pers.connected == CON_CONNECTING)
	{
		return -1;
	}

	// then spectators
	if (ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR)
	{
		if (ca->sess.spectatorNum > cb->sess.spectatorNum)
		{
			return -1;
		}
		if (ca->sess.spectatorNum < cb->sess.spectatorNum)
		{
			return 1;
		}
		return 0;
	}
	if (ca->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return 1;
	}
	if (cb->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return -1;
	}

	// then sort by score
	if (ca->ps.persistant[PERS_SCORE]
		> cb->ps.persistant[PERS_SCORE])
	{
		return -1;
	}
	if (ca->ps.persistant[PERS_SCORE]
		< cb->ps.persistant[PERS_SCORE])
	{
		return 1;
	}
	return 0;
}

qboolean gQueueScoreMessage = qfalse;
int gQueueScoreMessageTime = 0;

//A new duel started so respawn everyone and make sure their stats are reset
static qboolean G_CanResetDuelists(void)
{
	int i = 0;
	while (i < 3)
	{
		//precheck to make sure they are all respawnable
		const gentity_t* ent = &g_entities[level.sortedClients[i]];

		if (!ent->inuse || !ent->client || ent->health <= 0 ||
			ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
			ent->client->sess.duelTeam <= DUELTEAM_FREE)
		{
			return qfalse;
		}
		i++;
	}

	return qtrue;
}

qboolean g_noPDuelCheck = qfalse;

static void G_ResetDuelists(void)
{
	int i = 0;
	while (i < 3)
	{
		gentity_t* ent = &g_entities[level.sortedClients[i]];

		g_noPDuelCheck = qtrue;
		player_die(ent, ent, ent, 999, MOD_SUICIDE);
		g_noPDuelCheck = qfalse;
		trap->UnlinkEntity((sharedEntity_t*)ent);
		ClientSpawn(ent);
		i++;
	}
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks(void)
{
	int i;
	//	int		preNumSpec = 0;
	//int		nonSpecIndex = -1;
	gclient_t* cl;

	//	preNumSpec = level.numNonSpectatorClients;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0; // don't count bots

	for (i = 0; i < ARRAY_LEN(level.numteamVotingClients); i++)
	{
		level.numteamVotingClients[i] = 0;
	}
	for (i = 0; i < level.maxclients; i++)
	{
		if (level.clients[i].pers.connected != CON_DISCONNECTED)
		{
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if (level.clients[i].sess.sessionTeam != TEAM_SPECTATOR || level.gametype == GT_DUEL || level.gametype ==
				GT_POWERDUEL)
			{
				if (level.clients[i].sess.sessionTeam != TEAM_SPECTATOR)
				{
					level.numNonSpectatorClients++;
					//nonSpecIndex = i;
				}

				// decide if this should be auto-followed
				if (level.clients[i].pers.connected == CON_CONNECTED)
				{
					if (level.clients[i].sess.sessionTeam != TEAM_SPECTATOR || level.clients[i].iAmALoser)
					{
						level.numPlayingClients++;
					}
					if (!(g_entities[i].r.svFlags & SVF_BOT))
					{
						level.numVotingClients++;
						if (level.clients[i].sess.sessionTeam == TEAM_RED)
							level.numteamVotingClients[0]++;
						else if (level.clients[i].sess.sessionTeam == TEAM_BLUE)
							level.numteamVotingClients[1]++;
					}
					if (level.follow1 == -1)
					{
						level.follow1 = i;
					}
					else if (level.follow2 == -1)
					{
						level.follow2 = i;
					}
				}
			}
		}
	}

	if (!g_warmup.integer || level.gametype == GT_SIEGE)
		level.warmupTime = 0;

	qsort(level.sortedClients, level.numConnectedClients,
		sizeof level.sortedClients[0], SortRanks);

	// set the rank value for all clients that are connected and not spectators
	if (level.gametype >= GT_TEAM)
	{
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for (i = 0; i < level.numConnectedClients; i++)
		{
			cl = &level.clients[level.sortedClients[i]];
			if (level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE])
			{
				cl->ps.persistant[PERS_RANK] = 2;
			}
			else if (level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE])
			{
				cl->ps.persistant[PERS_RANK] = 0;
			}
			else
			{
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
	}
	else
	{
		int rank = -1;
		int score = 0;
		for (i = 0; i < level.numPlayingClients; i++)
		{
			cl = &level.clients[level.sortedClients[i]];
			const int newScore = cl->ps.persistant[PERS_SCORE];
			if (i == 0 || newScore != score)
			{
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank;
			}
			else if (i != 0)
			{
				// we are tied with the previous client
				level.clients[level.sortedClients[i - 1]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if (level.gametype == GT_SINGLE_PLAYER && level.numPlayingClients == 1)
			{
				level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if (level.gametype >= GT_TEAM)
	{
		trap->SetConfigstring(CS_SCORES1, va("%i", level.teamScores[TEAM_RED]));
		trap->SetConfigstring(CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE]));
	}
	else
	{
		if (level.numConnectedClients == 0)
		{
			trap->SetConfigstring(CS_SCORES1, va("%i", SCORE_NOT_PRESENT));
			trap->SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
		}
		else if (level.numConnectedClients == 1)
		{
			trap->SetConfigstring(
				CS_SCORES1, va("%i", level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE]));
			trap->SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
		}
		else
		{
			trap->SetConfigstring(
				CS_SCORES1, va("%i", level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE]));
			trap->SetConfigstring(
				CS_SCORES2, va("%i", level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE]));
		}

		if (level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL)
		{
			//when not in duel, use this configstring to pass the index of the player currently in first place
			if (level.numConnectedClients >= 1)
			{
				trap->SetConfigstring(CS_CLIENT_DUELWINNER, va("%i", level.sortedClients[0]));
			}
			else
			{
				trap->SetConfigstring(CS_CLIENT_DUELWINNER, "-1");
			}
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission or in multi-frag Duel game mode, send the new info to everyone
	if (level.intermissiontime || level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		gQueueScoreMessage = qtrue;
		gQueueScoreMessageTime = level.time + 500;
		//SendScoreboardMessageToAllClients();
		//rww - Made this operate on a "queue" system because it was causing large overflows
	}
}

/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients(void)
{
	for (int i = 0; i < level.maxclients; i++)
	{
		if (level.clients[i].pers.connected == CON_CONNECTED)
		{
			DeathmatchScoreboardMessage(g_entities + i);
		}
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
extern void G_LeaveVehicle(gentity_t* ent, qboolean con_check);

void MoveClientToIntermission(gentity_t* ent)
{
	// take out of follow mode if needed
	if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW)
	{
		StopFollowing(ent);
	}

	FindIntermissionPoint();
	// move to the spot
	VectorCopy(level.intermission_origin, ent->s.origin);
	VectorCopy(level.intermission_origin, ent->client->ps.origin);
	VectorCopy(level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset(ent->client->ps.powerups, 0, sizeof ent->client->ps.powerups);

	G_LeaveVehicle(ent, qfalse);

	ent->client->ps.rocketLockIndex = ENTITYNUM_NONE;
	ent->client->ps.rocketLockTime = 0;

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->client->ps.eFlags2 = 0;
	ent->s.eFlags2 = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelIndex = 0;
	ent->s.loopSound = 0;
	ent->s.loopIsSoundset = qfalse;
	ent->s.event = 0;
	ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
extern qboolean gSiegeRoundBegun;
extern qboolean gSiegeRoundEnded;
extern int gSiegeRoundWinningTeam;

void FindIntermissionPoint(void)
{
	gentity_t* ent = NULL;

	// find the intermission spot
	if (level.gametype == GT_SIEGE
		&& level.intermissiontime
		&& level.intermissiontime <= level.time
		&& gSiegeRoundEnded)
	{
		if (gSiegeRoundWinningTeam == SIEGETEAM_TEAM1)
		{
			ent = G_Find(NULL, FOFS(classname), "info_player_intermission_red");
			if (ent && ent->target2)
			{
				G_UseTargets2(ent, ent, ent->target2);
			}
		}
		else if (gSiegeRoundWinningTeam == SIEGETEAM_TEAM2)
		{
			ent = G_Find(NULL, FOFS(classname), "info_player_intermission_blue");
			if (ent && ent->target2)
			{
				G_UseTargets2(ent, ent, ent->target2);
			}
		}
	}
	if (!ent)
	{
		ent = G_Find(NULL, FOFS(classname), "info_player_intermission");
	}
	if (!ent)
	{
		// the map creator forgot to put in an intermission point...
		SelectSpawnPoint(vec3_origin, level.intermission_origin, level.intermission_angle, TEAM_SPECTATOR, qfalse);
	}
	else
	{
		VectorCopy(ent->s.origin, level.intermission_origin);
		VectorCopy(ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if (ent->target)
		{
			const gentity_t* target = G_PickTarget(ent->target);
			if (target)
			{
				vec3_t dir;
				VectorSubtract(target->s.origin, level.intermission_origin, dir);
				vectoangles(dir, level.intermission_angle);
			}
		}
	}
}

qboolean DuelLimitHit(void);

/*
==================
BeginIntermission
==================
*/
void BeginIntermission(void)
{
	if (level.intermissiontime)
	{
		return; // already active
	}

	// if in tournament mode, change the wins / losses
	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		trap->SetConfigstring(CS_CLIENT_DUELWINNER, "-1");

		if (level.gametype != GT_POWERDUEL)
		{
			AdjustTournamentScores();
		}
		if (DuelLimitHit())
		{
			gDuelExit = qtrue;
		}
		else
		{
			gDuelExit = qfalse;
		}
	}

	level.intermissiontime = level.time;

	// move all clients to the intermission point
	for (int i = 0; i < level.maxclients; i++)
	{
		gentity_t* client = g_entities + i;
		if (!client->inuse)
			continue;
		// respawn if dead
		if (client->health <= 0)
		{
			if (level.gametype != GT_POWERDUEL ||
				!client->client ||
				client->client->sess.sessionTeam != TEAM_SPECTATOR)
			{
				//don't respawn spectators in powerduel or it will mess the line order all up
				ClientRespawn(client);
			}
		}
		MoveClientToIntermission(client);
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();
}

qboolean DuelLimitHit(void)
{
	for (int i = 0; i < sv_maxclients.integer; i++)
	{
		const gclient_t* cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED)
		{
			continue;
		}

		if (duel_fraglimit.integer && cl->sess.wins >= duel_fraglimit.integer)
		{
			return qtrue;
		}
	}

	return qfalse;
}

static void DuelResetWinsLosses(void)
{
	for (int i = 0; i < sv_maxclients.integer; i++)
	{
		gclient_t* cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED)
		{
			continue;
		}

		cl->sess.wins = 0;
		cl->sess.losses = 0;
	}
}

/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar

=============
*/
extern void SiegeDoTeamAssign(void); //g_saga.c
extern siegePers_t g_siegePersistant; //g_saga.c
void ExitLevel(void)
{
	int i;

	// if we are running a tournament map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		if (!DuelLimitHit())
		{
			if (!level.restarted)
			{
				trap->SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
				level.restarted = qtrue;
				level.changemap = NULL;
				level.intermissiontime = 0;
			}
			return;
		}

		DuelResetWinsLosses();
	}

	if (level.gametype == GT_SIEGE &&
		g_siegeTeamSwitch.integer &&
		g_siegePersistant.beatingTime)
	{
		//restart same map...
		trap->SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
	}
	else
	{
		trap->SendConsoleCommand(EXEC_APPEND, "vstr nextmap\n");
	}
	level.changemap = NULL;
	level.intermissiontime = 0;

	if (level.gametype == GT_SIEGE &&
		g_siegeTeamSwitch.integer)
	{
		//switch out now
		SiegeDoTeamAssign();
	}

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for (i = 0; i < sv_maxclients.integer; i++)
	{
		gclient_t* cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED)
		{
			continue;
		}
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before chaning to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i = 0; i < sv_maxclients.integer; i++)
	{
		if (level.clients[i].pers.connected == CON_CONNECTED)
		{
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}
}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf(const char* fmt, ...)
{
	va_list		argptr;
	char		string[1024] = { 0 };
	int			mins, seconds, msec, l;

	msec = level.time - level.startTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds %= 60;

	Com_sprintf(string, sizeof(string), "%i:%02i ", mins, seconds);

	l = strlen(string);

	va_start(argptr, fmt);
	Q_vsnprintf(string + l, sizeof(string) - l, fmt, argptr);
	va_end(argptr);

	if (dedicated.integer)
		trap->Print("%s", string + l);

	if (!level.logFile)
		return;

	trap->FS_Write(string, strlen(string), level.logFile);
}

/*
=================
G_SecurityLogPrintf

Print to the security logfile with a time stamp if it is open
=================
*/
void QDECL G_SecurityLogPrintf(const char* fmt, ...)
{
	va_list argptr;
	char string[1024] = { 0 };
	time_t rawtime;

	time(&rawtime);
	localtime(&rawtime);
	strftime(string, sizeof string, "[%Y-%m-%d] [%H:%M:%S] ", gmtime(&rawtime));
	const int timeLen = strlen(string);

	va_start(argptr, fmt);
	Q_vsnprintf(string + timeLen, sizeof string - timeLen, fmt, argptr);
	va_end(argptr);

	if (dedicated.integer)
		trap->Print("%s", string + timeLen);

	if (!level.security.log)
		return;

	trap->FS_Write(string, strlen(string), level.security.log);
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit(const char* string)
{
	//	qboolean		won = qtrue;
	G_LogPrintf("Exit: %s\n", string);

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap->SetConfigstring(CS_INTERMISSION, "1");

	// don't send more than 32 scores (FIXME?)
	int numSorted = level.numConnectedClients;
	if (numSorted > 32)
	{
		numSorted = 32;
	}

	if (level.gametype >= GT_TEAM)
	{
		G_LogPrintf("red:%i  blue:%i\n",
			level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE]);
	}

	for (int i = 0; i < numSorted; i++)
	{
		gclient_t* cl = &level.clients[level.sortedClients[i]];

		if (cl->sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}
		if (cl->pers.connected == CON_CONNECTING)
		{
			continue;
		}

		const int ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		if (level.gametype >= GT_TEAM)
		{
			G_LogPrintf("(%s) score: %i  ping: %i  client: [%s] %i \"%s^7\"\n", TeamName(cl->ps.persistant[PERS_TEAM]),
				cl->ps.persistant[PERS_SCORE], ping, cl->pers.guid, level.sortedClients[i], cl->pers.netname);
		}
		else
		{
			G_LogPrintf("score: %i  ping: %i  client: [%s] %i \"%s^7\"\n", cl->ps.persistant[PERS_SCORE], ping,
				cl->pers.guid, level.sortedClients[i], cl->pers.netname);
		}
	}
}

qboolean gDidDuelStuff = qfalse; //gets reset on game reinit

/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
static void CheckIntermissionExit(void)
{
	int i;
	gclient_t* cl;

	// see which players are ready
	int ready = 0;
	int notReady = 0;
	int readyMask = 0;
	for (i = 0; i < sv_maxclients.integer; i++)
	{
		cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED)
		{
			continue;
		}
		if (g_entities[i].r.svFlags & SVF_BOT)
		{
			continue;
		}

		if (cl->readyToExit)
		{
			ready++;
			if (i < 16)
			{
				readyMask |= 1 << i;
			}
		}
		else
		{
			notReady++;
		}
	}

	if ((level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && !gDidDuelStuff &&
		level.time > level.intermissiontime + 2000)
	{
		gDidDuelStuff = qtrue;

		if (g_austrian.integer && level.gametype != GT_POWERDUEL)
		{
			G_LogPrintf("Duel Results:\n");
			//G_LogPrintf("Duel Time: %d\n", level.time );
			G_LogPrintf("winner: %s, score: %d, wins/losses: %d/%d\n",
				level.clients[level.sortedClients[0]].pers.netname,
				level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE],
				level.clients[level.sortedClients[0]].sess.wins,
				level.clients[level.sortedClients[0]].sess.losses);
			G_LogPrintf("loser: %s, score: %d, wins/losses: %d/%d\n",
				level.clients[level.sortedClients[1]].pers.netname,
				level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE],
				level.clients[level.sortedClients[1]].sess.wins,
				level.clients[level.sortedClients[1]].sess.losses);
		}
		// if we are running a tournament map, kick the loser to spectator status,
		// which will automatically grab the next spectator and restart
		if (!DuelLimitHit())
		{
			if (level.gametype == GT_POWERDUEL)
			{
				RemovePowerDuelLosers();
				AddPowerDuelPlayers();
			}
			else
			{
				if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
					level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
					level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
					level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
				{
					RemoveDuelDrawLoser();
				}
				else
				{
					RemoveTournamentLoser();
				}
				AddTournamentPlayer();
			}

			if (g_austrian.integer)
			{
				if (level.gametype == GT_POWERDUEL)
				{
					G_LogPrintf("Power Duel Initiated: %s %d/%d vs %s %d/%d and %s %d/%d, kill limit: %d\n",
						level.clients[level.sortedClients[0]].pers.netname,
						level.clients[level.sortedClients[0]].sess.wins,
						level.clients[level.sortedClients[0]].sess.losses,
						level.clients[level.sortedClients[1]].pers.netname,
						level.clients[level.sortedClients[1]].sess.wins,
						level.clients[level.sortedClients[1]].sess.losses,
						level.clients[level.sortedClients[2]].pers.netname,
						level.clients[level.sortedClients[2]].sess.wins,
						level.clients[level.sortedClients[2]].sess.losses,
						fraglimit.integer);
				}
				else
				{
					G_LogPrintf("Duel Initiated: %s %d/%d vs %s %d/%d, kill limit: %d\n",
						level.clients[level.sortedClients[0]].pers.netname,
						level.clients[level.sortedClients[0]].sess.wins,
						level.clients[level.sortedClients[0]].sess.losses,
						level.clients[level.sortedClients[1]].pers.netname,
						level.clients[level.sortedClients[1]].sess.wins,
						level.clients[level.sortedClients[1]].sess.losses,
						fraglimit.integer);
				}
			}

			if (level.gametype == GT_POWERDUEL)
			{
				if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
				{
					trap->SetConfigstring(CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0],
						level.sortedClients[1], level.sortedClients[2]));
					trap->SetConfigstring(CS_CLIENT_DUELWINNER, "-1");
				}
			}
			else
			{
				if (level.numPlayingClients >= 2)
				{
					trap->SetConfigstring(
						CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1]));
					trap->SetConfigstring(CS_CLIENT_DUELWINNER, "-1");
				}
			}

			return;
		}

		if (g_austrian.integer && level.gametype != GT_POWERDUEL)
		{
			G_LogPrintf("Duel Tournament Winner: %s wins/losses: %d/%d\n",
				level.clients[level.sortedClients[0]].pers.netname,
				level.clients[level.sortedClients[0]].sess.wins,
				level.clients[level.sortedClients[0]].sess.losses);
		}

		if (level.gametype == GT_POWERDUEL)
		{
			RemovePowerDuelLosers();
			AddPowerDuelPlayers();

			if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
			{
				trap->SetConfigstring(CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1],
					level.sortedClients[2]));
				trap->SetConfigstring(CS_CLIENT_DUELWINNER, "-1");
			}
		}
		else
		{
			//this means we hit the duel limit so reset the wins/losses
			//but still push the loser to the back of the line, and retain the order for
			//the map change
			if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
				level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
				level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
				level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
			{
				RemoveDuelDrawLoser();
			}
			else
			{
				RemoveTournamentLoser();
			}

			AddTournamentPlayer();

			if (level.numPlayingClients >= 2)
			{
				trap->SetConfigstring(CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1]));
				trap->SetConfigstring(CS_CLIENT_DUELWINNER, "-1");
			}
		}
	}

	if ((level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && !gDuelExit)
	{
		//in duel, we have different behaviour for between-round intermissions
		if (level.time > level.intermissiontime + 4000)
		{
			//automatically go to next after 4 seconds
			ExitLevel();
			return;
		}

		for (i = 0; i < sv_maxclients.integer; i++)
		{
			//being in a "ready" state is not necessary here, so clear it for everyone
			//yes, I also thinking holding this in a ps value uniquely for each player
			//is bad and wrong, but it wasn't my idea.
			cl = level.clients + i;
			if (cl->pers.connected != CON_CONNECTED)
			{
				continue;
			}
			cl->ps.stats[STAT_CLIENTS_READY] = 0;
		}
		return;
	}

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for (i = 0; i < sv_maxclients.integer; i++)
	{
		cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED)
		{
			continue;
		}
		cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}

	// never exit in less than five seconds
	if (level.time < level.intermissiontime + 5000)
	{
		return;
	}

	if (d_noIntermissionWait.integer)
	{
		//don't care who wants to go, just go.
		ExitLevel();
		return;
	}

	// if nobody wants to go, clear timer
	if (!ready)
	{
		level.readyToExit = qfalse;
		return;
	}

	// if everyone wants to go, go now
	if (!notReady)
	{
		ExitLevel();
		return;
	}

	// the first person to ready starts the ten second timeout
	if (!level.readyToExit)
	{
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if (level.time < level.exitTime + 10000)
	{
		return;
	}

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
static qboolean ScoreIsTied(void)
{
	if (level.numPlayingClients < 2)
	{
		return qfalse;
	}

	if (level.gametype >= GT_TEAM)
	{
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	const int a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	const int b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

#define LMSRESETTIME		5000		//amount of time LMS being found and the LMS round reset.
qboolean LMSReset = qfalse;
int LMSResetTime = 0;

static void CheckLMS()
{
	int i;
	if (!LMSReset //not already reseting
		&& g_lms.integer > 0 && BG_IsLMSGametype(g_gametype.integer)
		&& LMS_EnoughPlayers())
	{
		//check to see if there's only one LAST MAN STANDING!
		int counts[TEAM_NUM_TEAMS];
		counts[TEAM_FREE] = 0;
		counts[TEAM_RED] = 0;
		counts[TEAM_BLUE] = 0;

		for (i = 0; i < level.numNonSpectatorClients; i++)
		{
			const gentity_t* ent = &g_entities[level.sortedClients[i]];
			if (ent->health > 0 && ent->client->tempSpectate < level.time //actively alive ingame
				|| ent->lives >= 1) //or still has lives left and can respawn
			{
				//this dude is alive and has lives
				counts[ent->client->sess.sessionTeam]++;
			}
		}

		if (level.gametype >= GT_TEAM)
		{
			//either team must have no players
			if (counts[TEAM_RED] <= 0 || counts[TEAM_BLUE] <= 0)
			{
				//only one side has players left
				LMSReset = qtrue;
				LMSResetTime = level.time + LMSRESETTIME;
			}
		}
		else
		{
			if (counts[TEAM_FREE] <= 1)
			{
				//one or fewer left, LMS!
				LMSReset = qtrue;
				LMSResetTime = level.time + LMSRESETTIME;
			}
		}
	}
	else if (LMSReset && LMSResetTime < level.time)
	{
		//reset all the players if we've finished a LMS round.
		const gentity_t* winner = NULL;
		for (i = 0; i < level.numNonSpectatorClients; i++)
		{
			gentity_t* ent = &g_entities[level.sortedClients[i]];
			ent->lives = g_lmslives.integer >= 1 ? g_lmslives.integer : 1;
			if (ent->health <= 0 || ent->client->tempSpectate > level.time)
			{
				ClientRespawn(ent);
				ent->client->tempSpectate = 0;
				if (!(ent->r.svFlags & SVF_BOT))
				{
					//let them know that they suck.
					trap->SendServerCommand(ent->s.number, va("LMSLose"));
				}
			}
			else
			{
				winner = ent;
				ent->lives--; //deduct survivor's initial life since they're already alive.
				if (!(ent->r.svFlags & SVF_BOT))
				{
					//let them know that they suck.
					trap->SendServerCommand(ent->s.number, va("LMSWin"));
				}
			}
		}

		if (winner)
		{
			if (level.gametype < GT_TEAM)
			{
				trap->SendServerCommand(-1, va("cp \"%s" S_COLOR_YELLOW " was the last man standing!\n\"",
					winner->client->pers.netname));
			}
			else
			{
				switch (winner->client->sess.sessionTeam)
				{
				case TEAM_BLUE:
					trap->SendServerCommand(
						-1, va("cp \"" S_COLOR_BLUE "Blue team" S_COLOR_YELLOW " was the last team standing!\n\""));
					break;
				case TEAM_RED:
					trap->SendServerCommand(
						-1, va("cp \"" S_COLOR_RED "Red team" S_COLOR_YELLOW " was the last team standing!\n\""));
					break;
				default:;
				}
			}
		}

		LMSReset = qfalse;
		LMSResetTime = 0;
	}
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
qboolean g_endPDuel = qfalse;

void CheckExitRules(void)
{
	int i;
	char* sKillLimit;
	qboolean printLimit = qtrue;
	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if (level.intermissiontime)
	{
		CheckIntermissionExit();
		return;
	}

	if (gDoSlowMoDuel)
	{
		//don't go to intermission while in slow motion
		return;
	}

	if (gEscaping)
	{
		int numLiveClients = 0;

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (g_entities[i].inuse && g_entities[i].client && g_entities[i].health > 0)
			{
				if (g_entities[i].client->sess.sessionTeam != TEAM_SPECTATOR &&
					!(g_entities[i].client->ps.pm_flags & PMF_FOLLOW))
				{
					numLiveClients++;
				}
			}
		}
		if (gEscapeTime < level.time)
		{
			gEscaping = qfalse;
			LogExit("Escape time ended.");
			return;
		}
		if (!numLiveClients)
		{
			gEscaping = qfalse;
			LogExit("Everyone failed to escape.");
			return;
		}
	}

	if (level.intermissionQueued)
	{
		const int time = INTERMISSION_DELAY_TIME;
		if (level.time - level.intermissionQueued >= time)
		{
			level.intermissionQueued = 0;
			BeginIntermission();
		}
		return;
	}

	// check for sudden death
	if (level.gametype != GT_SIEGE)
	{
		if (ScoreIsTied())
		{
			// always wait for sudden death
			if (level.gametype != GT_DUEL || !timelimit.value)
			{
				if (level.gametype != GT_POWERDUEL)
				{
					return;
				}
			}
		}
	}

	if (level.gametype != GT_SIEGE)
	{
		if (timelimit.value > 0.0f && !level.warmupTime)
		{
			if (level.time - level.startTime >= timelimit.value * 60000)
			{
				//				trap->SendServerCommand( -1, "print \"Timelimit hit.\n\"");
				trap->SendServerCommand(-1, va("print \"%s.\n\"", G_GetStringEdString("MP_SVGAME", "TIMELIMIT_HIT")));
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Timelimit hit (1)\n");
				}
				LogExit("Timelimit hit.");
				return;
			}
		}
	}

	if (level.gametype == GT_SINGLE_PLAYER)
	{
		//don't check for fraglimit hit.
		return;
	}

	if (level.gametype == GT_POWERDUEL && level.numPlayingClients >= 3)
	{
		if (g_endPDuel)
		{
			g_endPDuel = qfalse;
			LogExit("Powerduel ended.");
		}
		return;
	}

	if (level.numPlayingClients < 2)
	{
		return;
	}

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		if (fraglimit.integer > 1)
		{
			sKillLimit = "Kill limit hit.";
		}
		else
		{
			sKillLimit = "";
			printLimit = qfalse;
		}
	}
	else
	{
		sKillLimit = "Kill limit hit.";
	}
	if (level.gametype < GT_SIEGE && fraglimit.integer)
	{
		if (level.teamScores[TEAM_RED] >= fraglimit.integer)
		{
			trap->SendServerCommand(
				-1, va("print \"Red %s\n\"", G_GetStringEdString("MP_SVGAME", "HIT_THE_KILL_LIMIT")));
			if (d_powerDuelPrint.integer)
			{
				Com_Printf("POWERDUEL WIN CONDITION: Kill limit (1)\n");
			}
			LogExit(sKillLimit);
			return;
		}

		if (level.teamScores[TEAM_BLUE] >= fraglimit.integer)
		{
			trap->SendServerCommand(-1, va("print \"Blue %s\n\"",
				G_GetStringEdString("MP_SVGAME", "HIT_THE_KILL_LIMIT")));
			if (d_powerDuelPrint.integer)
			{
				Com_Printf("POWERDUEL WIN CONDITION: Kill limit (2)\n");
			}
			LogExit(sKillLimit);
			return;
		}

		for (i = 0; i < sv_maxclients.integer; i++)
		{
			gclient_t* cl = level.clients + i;
			if (cl->pers.connected != CON_CONNECTED)
			{
				continue;
			}
			if (cl->sess.sessionTeam != TEAM_FREE)
			{
				continue;
			}

			if ((level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && duel_fraglimit.integer && cl->sess.wins
				>= duel_fraglimit.integer)
			{
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Duel limit hit (1)\n");
				}
				LogExit("Duel limit hit.");
				gDuelExit = qtrue;
				trap->SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " hit the win limit.\n\"",
					cl->pers.netname));
				return;
			}

			if (cl->ps.persistant[PERS_SCORE] >= fraglimit.integer)
			{
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Kill limit (3)\n");
				}
				LogExit(sKillLimit);
				gDuelExit = qfalse;
				if (printLimit)
				{
					trap->SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " %s.\n\"",
						cl->pers.netname,
						G_GetStringEdString("MP_SVGAME", "HIT_THE_KILL_LIMIT")
					)
					);
				}
				return;
			}
		}
	}

	if (level.gametype >= GT_CTF && capturelimit.integer)
	{
		if (level.teamScores[TEAM_RED] >= capturelimit.integer)
		{
			trap->SendServerCommand(-1, va("print \"%s \"", G_GetStringEdString("MP_SVGAME", "PRINTREDTEAM")));
			trap->SendServerCommand(-1, va("print \"%s.\n\"", G_GetStringEdString("MP_SVGAME", "HIT_CAPTURE_LIMIT")));
			LogExit("Capturelimit hit.");
			return;
		}

		if (level.teamScores[TEAM_BLUE] >= capturelimit.integer)
		{
			trap->SendServerCommand(-1, va("print \"%s \"", G_GetStringEdString("MP_SVGAME", "PRINTBLUETEAM")));
			trap->SendServerCommand(-1, va("print \"%s.\n\"", G_GetStringEdString("MP_SVGAME", "HIT_CAPTURE_LIMIT")));
			LogExit("Capturelimit hit.");
		}
	}
}

/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/

static void G_RemoveDuelist(const int team)
{
	int i = 0;
	while (i < MAX_CLIENTS)
	{
		gentity_t* ent = &g_entities[i];

		if (ent->inuse && ent->client && ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
			ent->client->sess.duelTeam == team)
		{
			SetTeam(ent, "s");
		}
		i++;
	}
}

/*
=============
CheckTournament

Once a frame, check for changes in tournament player state
=============
*/
int g_duelPrintTimer = 0;

static void CheckTournament(void)
{
	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	//	if ( level.numPlayingClients == 0 && (level.gametype != GT_POWERDUEL) ) {
	//		return;
	//	}

	if (level.gametype == GT_POWERDUEL)
	{
		if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
		{
			trap->SetConfigstring(CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1],
				level.sortedClients[2]));
		}
	}
	else
	{
		if (level.numPlayingClients >= 2)
		{
			trap->SetConfigstring(CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1]));
		}
	}

	if (level.gametype == GT_DUEL)
	{
		// pull in a spectator if needed
		if (level.numPlayingClients < 2 && !level.intermissiontime && !level.intermissionQueued)
		{
			AddTournamentPlayer();

			if (level.numPlayingClients >= 2)
			{
				trap->SetConfigstring(CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1]));
			}
		}

		if (level.numPlayingClients >= 2)
		{
			// nmckenzie: DUEL_HEALTH
			if (g_showDuelHealths.integer >= 1)
			{
				const playerState_t* ps1 = &level.clients[level.sortedClients[0]].ps;
				const playerState_t* ps2 = &level.clients[level.sortedClients[1]].ps;
				trap->SetConfigstring(CS_CLIENT_DUELHEALTHS, va("%i|%i|!",
					ps1->stats[STAT_HEALTH], ps2->stats[STAT_HEALTH]));
			}
		}

		//rww - It seems we have decided there will be no warmup in duel.
		//if (!g_warmup.integer)
		{
			//don't care about any of this stuff then, just add people and leave me alone
			level.warmupTime = 0;
			return;
		}
#if 0
		// if we don't have two players, go back to "waiting for players"
		if (level.numPlayingClients != 2) {
			if (level.warmupTime != -1) {
				level.warmupTime = -1;
				trap->SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
				G_LogPrintf("Warmup:\n");
			}
			return;
		}

		if (level.warmupTime == 0) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if (g_warmup.modificationCount != level.warmupModificationCount) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if (level.warmupTime < 0) {
			if (level.numPlayingClients == 2) {
				// fudge by -1 to account for extra delays
				level.warmupTime = level.time + (g_warmup.integer - 1) * 1000;

				if (level.warmupTime < (level.time + 3000))
				{ //rww - this is an unpleasent hack to keep the level from resetting completely on the client (this happens when two map_restarts are issued rapidly)
					level.warmupTime = level.time + 3000;
				}
				trap->SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
			}
			return;
		}

		// if the warmup time has counted down, restart
		if (level.time > level.warmupTime) {
			level.warmupTime += 10000;
			trap->Cvar_Set("g_restarted", "1");
			trap->SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
			level.restarted = qtrue;
			return;
		}
#endif
	}
	if (level.gametype == GT_POWERDUEL)
	{
		if (level.numPlayingClients < 2)
		{
			//hmm, ok, pull more in.
			g_dontFrickinCheck = qfalse;
		}

		if (level.numPlayingClients > 3)
		{
			//umm..yes..lets take care of that then.
			int lone = 0, dbl = 0;

			G_PowerDuelCount(&lone, &dbl, qfalse);
			if (lone > 1)
			{
				G_RemoveDuelist(DUELTEAM_LONE);
			}
			else if (dbl > 2)
			{
				G_RemoveDuelist(DUELTEAM_DOUBLE);
			}
		}
		else if (level.numPlayingClients < 3)
		{
			//hmm, someone disconnected or something and we need em
			int lone = 0, dbl = 0;

			G_PowerDuelCount(&lone, &dbl, qfalse);
			if (lone < 1)
			{
				g_dontFrickinCheck = qfalse;
			}
			else if (dbl < 1)
			{
				g_dontFrickinCheck = qfalse;
			}
		}

		// pull in a spectator if needed
		if (level.numPlayingClients < 3 && !g_dontFrickinCheck)
		{
			AddPowerDuelPlayers();

			if (level.numPlayingClients >= 3 &&
				G_CanResetDuelists())
			{
				gentity_t* te = G_TempEntity(vec3_origin, EV_GLOBAL_DUEL);
				te->r.svFlags |= SVF_BROADCAST;
				//this is really pretty nasty, but..
				te->s.otherentity_num = level.sortedClients[0];
				te->s.otherentity_num2 = level.sortedClients[1];
				te->s.groundEntityNum = level.sortedClients[2];

				trap->SetConfigstring(CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1],
					level.sortedClients[2]));
				G_ResetDuelists();

				g_dontFrickinCheck = qtrue;
			}
			else if (level.numPlayingClients > 0 ||
				level.numConnectedClients > 0)
			{
				if (g_duelPrintTimer < level.time)
				{
					//print once every 10 seconds
					int lone = 0, dbl = 0;

					G_PowerDuelCount(&lone, &dbl, qtrue);
					if (lone < 1)
					{
						trap->SendServerCommand(-1, va("cp \"%s\n\"",
							G_GetStringEdString("MP_SVGAME", "DUELMORESINGLE")));
					}
					else
					{
						trap->SendServerCommand(-1, va("cp \"%s\n\"",
							G_GetStringEdString("MP_SVGAME", "DUELMOREPAIRED")));
					}
					g_duelPrintTimer = level.time + 10000;
				}
			}

			if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
			{
				//pulled in a needed person
				if (G_CanResetDuelists())
				{
					gentity_t* te = G_TempEntity(vec3_origin, EV_GLOBAL_DUEL);
					te->r.svFlags |= SVF_BROADCAST;
					//this is really pretty nasty, but..
					te->s.otherentity_num = level.sortedClients[0];
					te->s.otherentity_num2 = level.sortedClients[1];
					te->s.groundEntityNum = level.sortedClients[2];

					trap->SetConfigstring(CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0],
						level.sortedClients[1], level.sortedClients[2]));

					if (g_austrian.integer)
					{
						G_LogPrintf("Duel Initiated: %s %d/%d vs %s %d/%d and %s %d/%d, kill limit: %d\n",
							level.clients[level.sortedClients[0]].pers.netname,
							level.clients[level.sortedClients[0]].sess.wins,
							level.clients[level.sortedClients[0]].sess.losses,
							level.clients[level.sortedClients[1]].pers.netname,
							level.clients[level.sortedClients[1]].sess.wins,
							level.clients[level.sortedClients[1]].sess.losses,
							level.clients[level.sortedClients[2]].pers.netname,
							level.clients[level.sortedClients[2]].sess.wins,
							level.clients[level.sortedClients[2]].sess.losses,
							fraglimit.integer);
					}
					//trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
					//FIXME: This seems to cause problems. But we'd like to reset things whenever a new opponent is set.
				}
			}
		}
		else
		{
			//if you have proper num of players then don't try to add again
			g_dontFrickinCheck = qtrue;
		}

		level.warmupTime = 0;
		return;
	}
	if (level.warmupTime != 0)
	{
		qboolean notEnough = qfalse;

		if (level.gametype > GT_TEAM)
		{
			int counts[TEAM_NUM_TEAMS];
			counts[TEAM_BLUE] = TeamCount(-1, TEAM_BLUE);
			counts[TEAM_RED] = TeamCount(-1, TEAM_RED);

			if (counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1)
			{
				notEnough = qtrue;
			}
		}
		else if (level.numPlayingClients < 2)
		{
			notEnough = qtrue;
		}

		if (notEnough)
		{
			if (level.warmupTime != -1)
			{
				level.warmupTime = -1;
				trap->SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
				G_LogPrintf("Warmup:\n");
			}
			return; // still waiting for team members
		}

		if (level.warmupTime == 0)
		{
			return;
		}

		// if all players have arrived, start the countdown
		if (level.warmupTime < 0)
		{
			// fudge by -1 to account for extra delays
			if (g_warmup.integer > 1)
			{
				level.warmupTime = level.time + (g_warmup.integer - 1) * 1000;
			}
			else
			{
				level.warmupTime = 0;
			}
			trap->SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
			return;
		}

		// if the warmup time has counted down, restart
		if (level.time > level.warmupTime)
		{
			level.warmupTime += 10000;
			trap->Cvar_Set("g_restarted", "1");
			trap->Cvar_Update(&g_restarted);
			trap->SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
			level.restarted = qtrue;
		}
	}
}

static void G_KickAllBots(void)
{
	for (int i = 0; i < sv_maxclients.integer; i++)
	{
		const gclient_t* cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED)
		{
			continue;
		}
		if (!(g_entities[i].r.svFlags & SVF_BOT))
		{
			continue;
		}
		trap->SendConsoleCommand(EXEC_INSERT, va("clientkick %d\n", i));
	}
}

/*
==================
CheckVote
==================
*/
static void CheckVote(void)
{
	if (level.voteExecuteTime && level.voteExecuteTime < level.time)
	{
		level.voteExecuteTime = 0;
		trap->SendConsoleCommand(EXEC_APPEND, va("%s\n", level.voteString));

		if (level.votingGametype)
		{
			if (level.gametype != level.votingGametypeTo)
			{
				//If we're voting to a different game type, be sure to refresh all the map stuff
				const char* nextMap = G_RefreshNextMap(level.votingGametypeTo, qtrue);

				//if (level.votingGametypeTo == GT_SIEGE)
				//{ //ok, kick all the bots, cause the aren't supported!
				//  G_KickAllBots();
				//	//just in case, set this to 0 too... I guess...maybe?
				//	//trap->Cvar_Set("bot_minplayers", "0");
				//}

				if (nextMap && nextMap[0])
				{
					trap->SendConsoleCommand(EXEC_APPEND, va("map %s\n", nextMap));
				}
			}
			else
			{
				//otherwise, just leave the map until a restart
				G_RefreshNextMap(level.votingGametypeTo, qfalse);
			}

			if (g_fraglimitVoteCorrection.integer)
			{
				//This means to auto-correct fraglimit when voting to and from duel.
				const int currentGT = level.gametype;
				const int currentFL = fraglimit.integer;
				const int currentTL = timelimit.integer;

				if ((level.votingGametypeTo == GT_DUEL || level.votingGametypeTo == GT_POWERDUEL) && currentGT !=
					GT_DUEL && currentGT != GT_POWERDUEL)
				{
					if (currentFL > 3 || !currentFL)
					{
						//if voting to duel, and fraglimit is more than 3 (or unlimited), then set it down to 3
						trap->SendConsoleCommand(EXEC_APPEND, "fraglimit 3\n");
					}
					if (currentTL)
					{
						//if voting to duel, and timelimit is set, make it unlimited
						trap->SendConsoleCommand(EXEC_APPEND, "timelimit 0\n");
					}
				}
				else if (level.votingGametypeTo != GT_DUEL && level.votingGametypeTo != GT_POWERDUEL &&
					(currentGT == GT_DUEL || currentGT == GT_POWERDUEL))
				{
					if (currentFL && currentFL < 20)
					{
						//if voting from duel, an fraglimit is less than 20, then set it up to 20
						trap->SendConsoleCommand(EXEC_APPEND, "fraglimit 20\n");
					}
				}
			}

			level.votingGametype = qfalse;
			level.votingGametypeTo = 0;
		}
	}
	if (!level.voteTime)
	{
		return;
	}
	if (level.time - level.voteTime >= VOTE_TIME || level.voteYes + level.voteNo == 0)
	{
		trap->SendServerCommand(-1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEFAILED"),
			level.voteStringClean));
	}
	else
	{
		if (level.voteYes > level.numVotingClients / 2)
		{
			// execute the command, then remove the vote
			trap->SendServerCommand(-1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEPASSED"),
				level.voteStringClean));
			level.voteExecuteTime = level.time + level.voteExecuteDelay;
		}

		// same behavior as a timeout
		else if (level.voteNo >= (level.numVotingClients + 1) / 2)
			trap->SendServerCommand(-1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEFAILED"),
				level.voteStringClean));

		else // still waiting for a majority
			return;
	}
	level.voteTime = 0;
	trap->SetConfigstring(CS_VOTE_TIME, "");
}

/*
==================
PrintTeam
==================
*/
static void PrintTeam(const int team, const char* message)
{
	for (int i = 0; i < level.maxclients; i++)
	{
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		trap->SendServerCommand(i, message);
	}
}

/*
==================
SetLeader
==================
*/
void SetLeader(const int team, const int client)
{
	if (level.clients[client].pers.connected == CON_DISCONNECTED)
	{
		PrintTeam(team, va("print \"%s is not connected\n\"", level.clients[client].pers.netname));
		return;
	}
	if (level.clients[client].sess.sessionTeam != team)
	{
		PrintTeam(team, va("print \"%s is not on the team anymore\n\"", level.clients[client].pers.netname));
		return;
	}
	for (int i = 0; i < level.maxclients; i++)
	{
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader)
		{
			level.clients[i].sess.teamLeader = qfalse;
			client_userinfo_changed(i);
		}
	}
	level.clients[client].sess.teamLeader = qtrue;
	client_userinfo_changed(client);
	PrintTeam(team, va("print \"%s %s\n\"", level.clients[client].pers.netname,
		G_GetStringEdString("MP_SVGAME", "NEWTEAMLEADER")));
}

/*
==================
CheckTeamLeader
==================
*/
void CheckTeamLeader(const int team)
{
	int i;

	for (i = 0; i < level.maxclients; i++)
	{
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader)
			break;
	}
	if (i >= level.maxclients)
	{
		for (i = 0; i < level.maxclients; i++)
		{
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			if (!(g_entities[i].r.svFlags & SVF_BOT))
			{
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
		if (i >= level.maxclients)
		{
			for (i = 0; i < level.maxclients; i++)
			{
				if (level.clients[i].sess.sessionTeam != team)
					continue;
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
	}
}

/*
==================
CheckTeamVote
==================
*/
static void CheckTeamVote(const int team)
{
	int cs_offset;

	if (team == TEAM_RED)
		cs_offset = 0;
	else if (team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if (level.teamVoteExecuteTime[cs_offset] && level.teamVoteExecuteTime[cs_offset] < level.time)
	{
		level.teamVoteExecuteTime[cs_offset] = 0;
		if (!Q_strncmp("leader", level.teamVoteString[cs_offset], 6))
		{
			//set the team leader
			SetLeader(team, atoi(level.teamVoteString[cs_offset] + 7));
		}
		else
		{
			trap->SendConsoleCommand(EXEC_APPEND, va("%s\n", level.teamVoteString[cs_offset]));
		}
	}

	if (!level.teamVoteTime[cs_offset])
	{
		return;
	}

	if (level.time - level.teamVoteTime[cs_offset] >= VOTE_TIME || level.teamVoteYes[cs_offset] + level.teamVoteNo[
		cs_offset] == 0)
	{
		trap->SendServerCommand(-1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEFAILED"),
			level.teamVoteStringClean[cs_offset]));
	}
	else
	{
		if (level.teamVoteYes[cs_offset] > level.numteamVotingClients[cs_offset] / 2)
		{
			// execute the command, then remove the vote
			trap->SendServerCommand(-1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEPASSED"),
				level.teamVoteStringClean[cs_offset]));
			level.voteExecuteTime = level.time + 3000;
		}

		// same behavior as a timeout
		else if (level.teamVoteNo[cs_offset] >= (level.numteamVotingClients[cs_offset] + 1) / 2)
			trap->SendServerCommand(-1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEFAILED"),
				level.teamVoteStringClean[cs_offset]));

		else // still waiting for a majority
			return;
	}
	level.teamVoteTime[cs_offset] = 0;
	trap->SetConfigstring(CS_TEAMVOTE_TIME + cs_offset, "");
}

/*
==================
CheckCvars
==================
*/
static void CheckCvars(void)
{
	static int lastMod = -1;

	if (g_password.modificationCount != lastMod)
	{
		char password[MAX_INFO_STRING];
		char* c = password;
		lastMod = g_password.modificationCount;

		strcpy(password, g_password.string);
		while (*c)
		{
			if (*c == '%')
			{
				*c = '.';
			}
			c++;
		}
		trap->Cvar_Set("g_password", password);

		if (*g_password.string && Q_stricmp(g_password.string, "none"))
		{
			trap->Cvar_Set("g_needpass", "1");
		}
		else
		{
			trap->Cvar_Set("g_needpass", "0");
		}
	}
}

//This checks for the end of a sound script being played on this entity
static void ICARUS_SoundCheck(gentity_t* ent)
{
	if (trap->ICARUS_TaskIDPending((sharedEntity_t*)ent, TID_CHAN_VOICE) && ent->IcarusSoundTime < level.time)
	{
		trap->ICARUS_TaskIDComplete((sharedEntity_t*)ent, TID_CHAN_VOICE);
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink(gentity_t* ent)
{
	const float thinktime = ent->nextthink;
	if (thinktime <= 0)
	{
		goto runicarus;
	}
	if (thinktime > level.time)
	{
		goto runicarus;
	}

	ent->nextthink = 0;
	if (!ent->think)
	{
		//trap->Error( ERR_DROP, "NULL ent->think");
		goto runicarus;
	}
	ent->think(ent);

runicarus:
	if (ent->inuse)
	{
		SaveNPCGlobals();
		if (NPCS.NPCInfo == NULL && ent->NPC != NULL)
		{
			SetNPCGlobals(ent);
		}
		//total hack to let ICARUS know that dowaits on sound files have ended
		ICARUS_SoundCheck(ent);
		trap->ICARUS_MaintainTaskManager(ent->s.number);
		RestoreNPCGlobals();
	}
}

int g_LastFrameTime = 0;
int g_TimeSinceLastFrame = 0;

qboolean gDoSlowMoDuel = qfalse;
int gSlowMoDuelTime = 0;

//#define _G_FRAME_PERFANAL

static void NAV_CheckCalcPaths(void)
{
	if (navCalcPathTime && navCalcPathTime < level.time)
	{
		//first time we've ever loaded this map...
		vmCvar_t mapname;
		vmCvar_t ckSum;

		trap->Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);
		trap->Cvar_Register(&ckSum, "sv_mapChecksum", "", CVAR_ROM);

		//clear all the failed edges
		trap->Nav_ClearAllFailedEdges();

		//Calculate all paths
		NAV_CalculatePaths(mapname.string, ckSum.integer);

		trap->Nav_CalculatePaths(qfalse);

#ifndef FINAL_BUILD
		if (fatalErrors)
		{
			Com_Printf(S_COLOR_RED"Not saving .nav file due to fatal nav errors\n");
		}
		else
#endif
			if (trap->Nav_Save(mapname.string, ckSum.integer) == qfalse)
			{
				Com_Printf("Unable to save navigations data for map \"%s\" (checksum:%d)\n", mapname.string, ckSum.integer);
			}
		navCalcPathTime = 0;
	}
}

/*
-------------------------
G_Animate
-------------------------
*/

static void G_Animate(gentity_t* self)
{
	if (self->s.eFlags & EF_SHADER_ANIM)
	{
		return;
	}
	if (self->s.frame == self->endFrame)
	{
		if (self->r.svFlags & SVF_ANIMATING)
		{
			{
				if (self->loopAnim)
				{
					self->s.frame = self->startFrame;
				}
				else
				{
					self->r.svFlags &= ~SVF_ANIMATING;
				}

				trap->ICARUS_TaskIDComplete((sharedEntity_t*)self, TID_ANIM_BOTH);
			}
		}
		return;
	}

	self->r.svFlags |= SVF_ANIMATING;

	if (self->startFrame < self->endFrame)
	{
		if (self->s.frame < self->startFrame || self->s.frame > self->endFrame)
		{
			self->s.frame = self->startFrame;
		}
		else
		{
			self->s.frame++;
		}
	}
	else if (self->startFrame > self->endFrame)
	{
		if (self->s.frame > self->startFrame || self->s.frame < self->endFrame)
		{
			self->s.frame = self->startFrame;
		}
		else
		{
			self->s.frame--;
		}
	}
	else
	{
		self->s.frame = self->endFrame;
	}
}

//so shared code can get the local time depending on the side it's executed on
int BG_GetTime(void)
{
	return level.time;
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void ClearNPCGlobals(void);
void AI_UpdateGroups(void);
void ClearPlayerAlertEvents(void);
void SiegeCheckTimers(void);
extern void wp_saber_start_missile_block_check(gentity_t* self, const usercmd_t* ucmd);
extern void Jedi_Decloak(gentity_t* self);
qboolean G_PointInBounds(vec3_t point, vec3_t mins, vec3_t maxs);
extern qboolean inGameCinematic;
extern void UpdatePlayerScriptTarget(void);
extern void UpdatePlayerCameraPos(void);
int g_siegeRespawnCheck = 0;
int g_ffaRespawnTimerCheck = 0;
void SetMoverState(gentity_t* ent, moverState_t moverState, int time);
extern qboolean BG_SabersOff(const playerState_t* ps);
extern void G_Roffs(gentity_t* ent);
extern void TieFighterThink(gentity_t* self);

#define CLOAK_DEFUEL_RATE		150 //approx. 20 seconds of idle use from a fully charged fuel amt
#define CLOAK_REFUEL_RATE		100 //seems fair

#define JETPACK_DEFUEL_RATE		500 //approx. 50 seconds of idle use from a fully charged fuel amt
#define JETPACK_REFUEL_RATE		150 //seems fair

#define SPRINT_DEFUEL_RATE		150
#define ENHANCED_REFUEL_RATE	75 //seems fair

void G_RunFrame(const int levelTime)
{
	int i = 0;
	gentity_t* ent;
#ifdef _G_FRAME_PERFANAL
	int			iTimer_ItemRun = 0;
	int			iTimer_ROFF = 0;
	int			iTimer_ClientEndframe = 0;
	int			iTimer_GameChecks = 0;
	int			iTimer_Queues = 0;
	void* timer_ItemRun;
	void* timer_ROFF;
	void* timer_ClientEndframe;
	void* timer_GameChecks;
	void* timer_Queues;
#endif

	if (inGameCinematic)
	{
		//don't update the game world if an ROQ files is running.
		return;
	}
	if (level.gametype == GT_SIEGE &&
		g_siegeRespawn.integer &&
		g_siegeRespawnCheck < level.time)
	{
		//check for a respawn wave
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			gentity_t* clEnt = &g_entities[i];

			if (clEnt->inuse && clEnt->client &&
				clEnt->client->tempSpectate >= level.time &&
				clEnt->client->sess.sessionTeam != TEAM_SPECTATOR)
			{
				ClientRespawn(clEnt);
				clEnt->client->tempSpectate = 0;
			}
		}

		g_siegeRespawnCheck = level.time + g_siegeRespawn.integer * 1000;
	}

	if ((level.gametype == GT_FFA
		|| level.gametype == GT_TEAM
		|| level.gametype == GT_CTF) &&
		g_ffaRespawnTimer.integer &&
		g_ffaRespawnTimerCheck < level.time)
	{
		while (i < MAX_CLIENTS)
		{
			gentity_t* clEnt = &g_entities[i];

			if (clEnt->inuse && clEnt->client &&
				clEnt->client->tempSpectate > level.time &&
				clEnt->client->sess.sessionTeam != TEAM_SPECTATOR)
			{
				ClientRespawn(clEnt);

				clEnt->client->tempSpectate = 0;
			}
			i++;
		}
		g_ffaRespawnTimerCheck = level.time + Q_irand(5000, 15000);
	}

	if (gDoSlowMoDuel)
	{
		if (level.restarted)
		{
			char buf[128];

			trap->Cvar_VariableStringBuffer("timescale", buf, sizeof buf);

			const float tFVal = atof(buf);

			trap->Cvar_Set("timescale", "1");
			if (tFVal == 1.0f)
			{
				gDoSlowMoDuel = qfalse;
			}
		}
		else
		{
			const float timeDif = level.time - gSlowMoDuelTime;
			//difference in time between when the slow motion was initiated and now

			if (timeDif < 150)
			{
				trap->Cvar_Set("timescale", "0.1f");
			}
			else if (timeDif < 1150)
			{
				float useDif = timeDif / 1000; //scale from 0.1 up to 1
				if (useDif < 0.1f)
				{
					useDif = 0.1f;
				}
				if (useDif > 1.0f)
				{
					useDif = 1.0f;
				}
				trap->Cvar_Set("timescale", va("%f", useDif));
			}
			else
			{
				char buf[128];

				trap->Cvar_VariableStringBuffer("timescale", buf, sizeof buf);

				const float tFVal = atof(buf);

				trap->Cvar_Set("timescale", "1");
				if (timeDif > 1500 && tFVal == 1.0f)
				{
					gDoSlowMoDuel = qfalse;
				}
			}
		}
	}

	// if we are waiting for the level to restart, do nothing
	if (level.restarted)
	{
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;

	if (level.gametype == GT_SINGLE_PLAYER && g_allowNPC.integer)
	{
		NAV_CheckCalcPaths();
	}

	AI_UpdateGroups();

	if (level.gametype == GT_SINGLE_PLAYER && g_allowNPC.integer)
	{
		if (d_altRoutes.integer)
		{
			trap->Nav_CheckAllFailedEdges();
		}
		trap->Nav_ClearCheckedNodes();

		//remember last waypoint, clear current one
		for (i = 0; i < level.num_entities; i++)
		{
			ent = &g_entities[i];

			if (!ent->inuse)
				continue;

			if (ent->waypoint != WAYPOINT_NONE
				&& ent->noWaypointTime < level.time)
			{
				ent->lastWaypoint = ent->waypoint;
				ent->waypoint = WAYPOINT_NONE;
			}
			if (d_altRoutes.integer)
			{
				trap->Nav_CheckFailedNodes((sharedEntity_t*)ent);
			}
		}

		//Look to clear out old events
		ClearPlayerAlertEvents();
	}

	g_TimeSinceLastFrame = level.time - g_LastFrameTime;

	// get any cvar changes
	G_UpdateCvars();

#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_ItemRun);
#endif
	//
	// go through all allocated objects
	//
	ent = &g_entities[0];
	for (i = 0; i < level.num_entities; i++, ent++)
	{
		if (!ent->inuse)
		{
			continue;
		}

		// clear events that are too old
		if (level.time - ent->eventTime > EVENT_VALID_MSEC)
		{
			if (ent->s.event)
			{
				ent->s.event = 0; // &= EV_EVENT_BITS;
				if (ent->client)
				{
					ent->client->ps.externalEvent = 0;
				}
			}
			if (ent->freeAfterEvent)
			{
				// tempEntities or dropped items completely go away after their event
				if (ent->s.eFlags & EF_SOUNDTRACKER)
				{
					//don't trigger the event again..
					ent->s.event = 0;
					ent->s.eventParm = 0;
					ent->s.eType = 0;
					ent->eventTime = 0;
				}
				else
				{
					G_FreeEntity(ent);
					continue;
				}
			}
			else if (ent->unlinkAfterEvent)
			{
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap->UnlinkEntity((sharedEntity_t*)ent);
			}
		}

		// temporary entities don't think
		if (ent->freeAfterEvent)
		{
			continue;
		}

		G_Roffs(ent);

		if (!ent->client)
		{
			//handle map entity animations
			if (!(ent->r.svFlags & SVF_SELF_ANIMATING))
			{
				//FIXME: make sure this is done only for models with frames?
				//Or just flag as animating?
				if (ent->s.eFlags & EF_ANIM_ONCE)
				{
					ent->s.frame++;
				}
				else if (!(ent->s.eFlags & EF_ANIM_ALLFAST))
				{
					G_Animate(ent);
				}
			}
		}

		if (!ent->r.linked && ent->neverFree)
		{
			continue;
		}

		G_CheckSpecialPersistentEvents(ent);

		if (ent->s.eType == ET_MISSILE)
		{
			g_run_missile(ent);
			continue;
		}

		if (ent->s.eType == ET_ITEM || ent->physicsObject)
		{
#if 0 //use if body dragging enabled?
			if (ent->s.eType == ET_BODY)
			{ //special case for bodies
				float grav = 3.0f;
				float mass = 0.14f;
				float bounce = 1.15f;

				G_RunExPhys(ent, grav, mass, bounce, qfalse, NULL, 0);
			}
			else
			{
				G_RunItem(ent);
			}
#else
			G_RunItem(ent);
#endif
			continue;
		}

		if (ent->s.eType == ET_MOVER)
		{
			if (ent->model && Q_stricmp("models/test/mikeg/tie_fighter.md3", ent->model) == 0)
			{
				TieFighterThink(ent);
			}
			G_RunMover(ent);
			continue;
		}

		//fix for self-deactivating areaportals in Siege
		if (ent->s.eType == ET_MOVER && level.gametype == GT_SIEGE && level.intermissiontime)
		{
			if (!Q_stricmp("func_door", ent->classname) && ent->moverState != MOVER_POS1)
			{
				SetMoverState(ent, MOVER_POS1, level.time);
				if (ent->teammaster == ent || !ent->teammaster)
				{
					trap->AdjustAreaPortalState((sharedEntity_t*)ent, qfalse);
				}

				//stop the looping sound
				ent->s.loopSound = 0;
				ent->s.loopIsSoundset = qfalse;
			}
			continue;
		}

		if (ent->client && (ent->s.eType == ET_PLAYER || ent->s.eType == ET_NPC))
		{//  NPCs can hack/use too!!!
			if (ent->client->isHacking > MAX_CLIENTS)
			{ //hacking checks
				gentity_t* hacked = &g_entities[ent->client->isHacking];
				vec3_t angDif;

				VectorSubtract(ent->client->ps.viewangles, ent->client->hackingAngles, angDif);

				//keep him in the "use" anim
				if (ent->client->ps.torsoAnim != BOTH_CONSOLE1)
				{
					G_SetAnim(ent, NULL, SETANIM_TORSO, BOTH_CONSOLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
				}
				else
				{
					ent->client->ps.torsoTimer = 500;
				}

				ent->client->ps.weaponTime = ent->client->ps.torsoTimer;

				if (!(ent->client->pers.cmd.buttons & BUTTON_USE) && (ent->s.eType == ET_PLAYER || ent->s.eType == ET_NPC))
				{ //have to keep holding use
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (!hacked->inuse)
				{ //shouldn't happen, but safety first
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (!G_PointInBounds(ent->client->ps.origin, hacked->r.absmin, hacked->r.absmax))
				{ //they stepped outside the thing they're hacking, so reset hacking time
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (VectorLength(angDif) > 10.0f && (ent->s.eType == ET_PLAYER || ent->s.eType == ET_NPC))
				{ //must remain facing generally the same angle as when we start (But only for players)
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
			}
		}

		if (i < MAX_CLIENTS)
		{
			G_CheckClientTimeouts(ent);

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
						if (ent->health > 0 && ent->takedamage)
						{
							//if they're still alive..
							G_Damage(ent, spacetrigger, spacetrigger, NULL, ent->client->ps.origin, Q_irand(50, 70),
								DAMAGE_NO_ARMOR, MOD_SUICIDE);

							if (ent->health > 0)
							{
								//did that last one kill them?
								//play the choking sound
								G_EntitySound(ent, CHAN_VOICE, G_SoundIndex(va("*choke%d.wav", Q_irand(1, 3))));

								//make them grasp their throat
								ent->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
								ent->client->ps.forceHandExtendTime = level.time + 2000;
							}
						}

						ent->client->inSpaceSuffocation = level.time + Q_irand(100, 200);
					}
				}
			}

			if (ent->client->isHacking)
			{ //hacking checks
				gentity_t* hacked = &g_entities[ent->client->isHacking];
				vec3_t angDif;

				VectorSubtract(ent->client->ps.viewangles, ent->client->hackingAngles, angDif);

				//keep him in the "use" anim
				if (ent->client->ps.torsoAnim != BOTH_CONSOLE1)
				{
					G_SetAnim(ent, NULL, SETANIM_TORSO, BOTH_CONSOLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
				}
				else
				{
					ent->client->ps.torsoTimer = 500;
				}
				ent->client->ps.weaponTime = ent->client->ps.torsoTimer;

				if (!(ent->client->pers.cmd.buttons & BUTTON_USE))
				{//have to keep holding use
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (!hacked || !hacked->inuse)
				{//shouldn't happen, but safety first
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (!G_PointInBounds(ent->client->ps.origin, hacked->r.absmin, hacked->r.absmax))
				{//they stepped outside the thing they're hacking, so reset hacking time
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (VectorLength(angDif) > 10.0f)
				{//must remain facing generally the same angle as when we start
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
			}

			if ((ent->client->jetPackOn || ent->client->flamethrowerOn))
			{
				//using jetpack, drain fuel
				if (ent->client->jetPackDebReduce < level.time)
				{
					if (ent->client->pers.cmd.forwardmove || ent->client->pers.cmd.upmove || ent->client->pers.cmd.
						rightmove)
					{
						if (!(ent->r.svFlags & SVF_BOT))
						{
							//take more if they're thrusting
							ent->client->ps.jetpackFuel -= 4;
						}
					}
					else
					{
						if (!(ent->r.svFlags & SVF_BOT))
						{
							if (ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								//in midair
								ent->client->ps.jetpackFuel--;
							}
							else
							{
								if (ent->client->flamethrowerOn)
								{
									ent->client->ps.jetpackFuel--;
								}
							}
						}
					}

					if (ent->client->ps.jetpackFuel <= 0)
					{
						//turn it off
						ent->client->ps.jetpackFuel = 0;
						Jetpack_Off(ent);
					}
					ent->client->jetPackDebReduce = level.time + JETPACK_DEFUEL_RATE;
				}
			}
			else if (ent->client->ps.jetpackFuel < 100 && ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				//recharge jetpack
				if (ent->client->jetPackDebRecharge < level.time && !ent->client->flamethrowerOn)
				{
					ent->client->ps.jetpackFuel++;
					ent->client->jetPackDebRecharge = level.time + JETPACK_REFUEL_RATE;
				}
			}

			if (ent->client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING || ent->client->ps.PlayerEffectFlags & 1 <<
				PEF_WEAPONSPRINTING)
			{
				//using jetpack, drain fuel
				if (ent->client->sprintDebReduce < level.time)
				{
					ent->client->ps.sprintFuel--;

					if (ent->client->ps.sprintFuel <= 0)
					{
						//turn it off
						ent->client->ps.sprintFuel = 0;
					}
					ent->client->sprintDebReduce = level.time + SPRINT_DEFUEL_RATE;
				}
			}
			else if (ent->client->ps.sprintFuel < 100 && !(ent->client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING) && !(
				ent->client->ps.PlayerEffectFlags & 1 << PEF_WEAPONSPRINTING))
			{
				//recharge jetpack
				if (ent->client->sprintkDebRecharge < level.time && !ent->client->IsSprinting)
				{
					ent->client->ps.sprintFuel++;
					if (PM_RestAnim(ent->client->ps.legsAnim) || PM_CrouchAnim(ent->client->ps.legsAnim))
					{
						ent->client->sprintkDebRecharge = level.time + ENHANCED_REFUEL_RATE;
					}
					else
					{
						ent->client->sprintkDebRecharge = level.time + JETPACK_REFUEL_RATE;
					}
				}
			}

			if (ent->client->ps.powerups[PW_CLOAKED])
			{
				//using cloak, drain battery
				if (ent->client->cloakDebReduce < level.time)
				{
					ent->client->ps.cloakFuel--;

					if (ent->client->ps.cloakFuel <= 0)
					{
						//turn it off
						ent->client->ps.cloakFuel = 0;
						Jedi_Decloak(ent);
					}
					ent->client->cloakDebReduce = level.time + CLOAK_DEFUEL_RATE;
				}
			}
			else if (ent->client->ps.powerups[PW_SPHERESHIELDED])
			{ //using cloak, drain battery
				if (ent->client->cloakDebReduce < level.time)
				{
					ent->client->ps.cloakFuel--;

					if (ent->client->ps.cloakFuel <= 0)
					{ //turn it off
						ent->client->ps.cloakFuel = 0;
						Sphereshield_Off(ent);
					}
					ent->client->cloakDebReduce = level.time + CLOAK_DEFUEL_RATE;
				}
			}
			else if (ent->client->ps.cloakFuel < 100)
			{
				//recharge cloak
				if (ent->client->cloakDebRecharge < level.time)
				{
					ent->client->ps.cloakFuel++;
					ent->client->cloakDebRecharge = level.time + CLOAK_REFUEL_RATE;
					ent->client->cloak_is_charging = qtrue;
				}
			}
			else if (ent->client->ps.cloakFuel >= 90)
			{
				ent->client->cloak_is_charging = qfalse;
			}

			if (level.gametype == GT_SIEGE &&
				ent->client->siegeClass != -1 &&
				bgSiegeClasses[ent->client->siegeClass].classflags & 1 << CFL_STATVIEWER)
			{
				//see if it's time to send this guy an update of extended info
				if (ent->client->siegeEDataSend < level.time)
				{
					G_SiegeClientExData(ent);
					ent->client->siegeEDataSend = level.time + 1000; //once every sec seems ok
				}
			}

			if (!level.intermissiontime && !(ent->client->ps.pm_flags & PMF_FOLLOW) && ent->client->sess.sessionTeam != TEAM_SPECTATOR)
			{
				WP_ForcePowersUpdate(ent, &ent->client->pers.cmd);
				WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
				wp_saber_start_missile_block_check(ent, &ent->client->pers.cmd);

				if (ent->client->ps.fd.blockPoints < BLOCK_POINTS_MAX)
				{
					WP_BlockPointsUpdate(ent);
				}
			}

			if (level.gametype == GT_SINGLE_PLAYER && g_allowNPC.integer)
			{
				//This was originally intended to only be done for client 0.
				//Make sure it doesn't slow things down too much with lots of clients in game.
				NAV_FindPlayerWaypoint(i);
			}
			//total hack to let ICARUS know that dowaits on sound files have ended
			ICARUS_SoundCheck(ent);

			trap->ICARUS_MaintainTaskManager(ent->s.number);

			G_RunClient(ent);
			continue;
		}
		if (ent->s.eType == ET_NPC)
		{
			// turn off any expired powerups
			for (int j = 0; j < MAX_POWERUPS; j++)
			{
				if (ent->client->ps.powerups[j] < level.time)
				{
					ent->client->ps.powerups[j] = 0;
				}
			}

			WP_ForcePowersUpdate(ent, &ent->client->pers.cmd);
			WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
			wp_saber_start_missile_block_check(ent, &ent->client->pers.cmd);

			if (ent->client->ps.fd.blockPoints < BLOCK_POINTS_MAX)
			{
				WP_BlockPointsUpdate(ent);
			}
		}

		G_RunThink(ent);

		if (level.gametype == GT_SINGLE_PLAYER && g_allowNPC.integer)
		{
			ClearNPCGlobals();
		}
	}
#ifdef _G_FRAME_PERFANAL
	iTimer_ItemRun = trap->PrecisionTimer_End(timer_ItemRun);
#endif

	SiegeCheckTimers();
	UpdatePlayerScriptTarget();
	UpdatePlayerCameraPos();

	if (DMSData.valid)
	{
		//if we're using DMS so we need to update the system.
		G_DynamicMusicUpdate();
	}

#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_ROFF);
#endif
	trap->ROFF_UpdateEntities();
#ifdef _G_FRAME_PERFANAL
	iTimer_ROFF = trap->PrecisionTimer_End(timer_ROFF);
#endif

#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_ClientEndframe);
#endif
	// perform final fixups on the players
	ent = &g_entities[0];
	for (i = 0; i < level.maxclients; i++, ent++)
	{
		if (ent->inuse)
		{
			ClientEndFrame(ent);
		}
	}
#ifdef _G_FRAME_PERFANAL
	iTimer_ClientEndframe = trap->PrecisionTimer_End(timer_ClientEndframe);
#endif

#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_GameChecks);
#endif
	// see if it is time to do a tournament restart
	CheckTournament();

	// see if it is time to end the level
	CheckExitRules();

	CheckLMS();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// check team votes
	CheckTeamVote(TEAM_RED);
	CheckTeamVote(TEAM_BLUE);

	// for tracking changes
	CheckCvars();

#ifdef _G_FRAME_PERFANAL
	iTimer_GameChecks = trap->PrecisionTimer_End(timer_GameChecks);
#endif

#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_Queues);
#endif
	//At the end of the frame, send out the ghoul2 kill queue, if there is one
	G_SendG2KillQueue();

	if (gQueueScoreMessage)
	{
		if (gQueueScoreMessageTime < level.time)
		{
			SendScoreboardMessageToAllClients();

			gQueueScoreMessageTime = 0;
			gQueueScoreMessage = 0;
		}
	}
#ifdef _G_FRAME_PERFANAL
	iTimer_Queues = trap->PrecisionTimer_End(timer_Queues);
#endif

#ifdef _G_FRAME_PERFANAL
	Com_Printf("---------------\nItemRun: %i\nROFF: %i\nClientEndframe: %i\nGameChecks: %i\nQueues: %i\n---------------\n",
		iTimer_ItemRun,
		iTimer_ROFF,
		iTimer_ClientEndframe,
		iTimer_GameChecks,
		iTimer_Queues);
#endif

	g_LastFrameTime = level.time;
}

const char* G_GetStringEdString(char* refSection, char* refName)
{
	/*
	static char text[1024]={0};
	trap->SP_GetStringTextString(va("%s_%s", refSection, refName), text, sizeof(text));
	return text;
	*/

	//Well, it would've been lovely doing it the above way, but it would mean mixing
	//languages for the client depending on what the server is. So we'll mark this as
	//a stringed reference with @@@ and send the refname to the client, and when it goes
	//to print it will get scanned for the stringed reference indication and dealt with
	//properly.
	static char text[1024] = { 0 };
	Com_sprintf(text, sizeof text, "@@@%s", refName);
	return text;
}

static void G_SpawnRMGEntity(void)
{
	if (G_ParseSpawnVars(qfalse))
		G_SpawnGEntityFromSpawnVars(qfalse);
}

static void _G_ROFF_NotetrackCallback(const int entID, const char* notetrack)
{
	G_ROFF_NotetrackCallback(&g_entities[entID], notetrack);
}

static int G_ICARUS_PlaySound(void)
{
	const T_G_ICARUS_PLAYSOUND* sharedMem = &gSharedBuffer.playSound;
	return Q3_PlaySound(sharedMem->taskID, sharedMem->entID, sharedMem->name, sharedMem->channel);
}

static qboolean G_ICARUS_Set(void)
{
	const T_G_ICARUS_SET* sharedMem = &gSharedBuffer.set;
	return Q3_Set(sharedMem->taskID, sharedMem->entID, sharedMem->type_name, sharedMem->data);
}

static void G_ICARUS_Lerp2Pos(void)
{
	T_G_ICARUS_LERP2POS* sharedMem = &gSharedBuffer.lerp2Pos;
	Q3_Lerp2Pos(sharedMem->taskID, sharedMem->entID, sharedMem->origin,
		sharedMem->nullAngles ? NULL : sharedMem->angles, sharedMem->duration);
}

static void G_ICARUS_Lerp2Origin(void)
{
	T_G_ICARUS_LERP2ORIGIN* sharedMem = &gSharedBuffer.lerp2Origin;
	Q3_Lerp2Origin(sharedMem->taskID, sharedMem->entID, sharedMem->origin, sharedMem->duration);
}

static void G_ICARUS_Lerp2Angles(void)
{
	T_G_ICARUS_LERP2ANGLES* sharedMem = &gSharedBuffer.lerp2Angles;
	Q3_Lerp2Angles(sharedMem->taskID, sharedMem->entID, sharedMem->angles, sharedMem->duration);
}

static int G_ICARUS_GetTag(void)
{
	T_G_ICARUS_GETTAG* sharedMem = &gSharedBuffer.getTag;
	return Q3_GetTag(sharedMem->entID, sharedMem->name, sharedMem->lookup, sharedMem->info);
}

static void G_ICARUS_Lerp2Start(void)
{
	const T_G_ICARUS_LERP2START* sharedMem = &gSharedBuffer.lerp2Start;
	Q3_Lerp2Start(sharedMem->entID, sharedMem->taskID, sharedMem->duration);
}

static void G_ICARUS_Lerp2End(void)
{
	const T_G_ICARUS_LERP2END* sharedMem = &gSharedBuffer.lerp2End;
	Q3_Lerp2End(sharedMem->entID, sharedMem->taskID, sharedMem->duration);
}

static void G_ICARUS_Use(void)
{
	const T_G_ICARUS_USE* sharedMem = &gSharedBuffer.use;
	Q3_Use(sharedMem->entID, sharedMem->target);
}

static void G_ICARUS_Kill(void)
{
	const T_G_ICARUS_KILL* sharedMem = &gSharedBuffer.kill;
	Q3_Kill(sharedMem->entID, sharedMem->name);
}

static void G_ICARUS_Remove(void)
{
	const T_G_ICARUS_REMOVE* sharedMem = &gSharedBuffer.remove;
	Q3_Remove(sharedMem->entID, sharedMem->name);
}

static void G_ICARUS_Play(void)
{
	const T_G_ICARUS_PLAY* sharedMem = &gSharedBuffer.play;
	Q3_Play(sharedMem->taskID, sharedMem->entID, sharedMem->type, sharedMem->name);
}

static int G_ICARUS_GetFloat(void)
{
	T_G_ICARUS_GETFLOAT* sharedMem = &gSharedBuffer.getFloat;
	return Q3_GetFloat(sharedMem->entID, sharedMem->type, sharedMem->name, &sharedMem->value);
}

static int G_ICARUS_GetVector(void)
{
	T_G_ICARUS_GETVECTOR* sharedMem = &gSharedBuffer.getVector;
	return Q3_GetVector(sharedMem->entID, sharedMem->type, sharedMem->name, sharedMem->value);
}

static int G_ICARUS_GetString(void)
{
	T_G_ICARUS_GETSTRING* sharedMem = &gSharedBuffer.getString;
	char* crap = NULL; //I am sorry for this -rww
	char** morecrap = &crap; //and this
	const int r = Q3_GetString(sharedMem->entID, sharedMem->type, sharedMem->name, morecrap);

	if (crap)
		strcpy(sharedMem->value, crap);

	return r;
}

static void G_ICARUS_SoundIndex(void)
{
	const T_G_ICARUS_SOUNDINDEX* sharedMem = &gSharedBuffer.soundIndex;
	G_SoundIndex(sharedMem->filename);
}

static int G_ICARUS_GetSetIDForString(void)
{
	const T_G_ICARUS_GETSETIDFORSTRING* sharedMem = &gSharedBuffer.getSetIDForString;
	return GetIDForString(setTable, sharedMem->string);
}

static qboolean G_NAV_ClearPathToPoint(const int entID, vec3_t pmins, vec3_t pmaxs, vec3_t point, const int clipmask,
	const int okToHitEnt)
{
	return NAV_ClearPathToPoint(&g_entities[entID], pmins, pmaxs, point, clipmask, okToHitEnt);
}

static qboolean G_NPC_ClearLOS2(const int entID, const vec3_t end)
{
	return NPC_ClearLOS2(&g_entities[entID], end);
}

static qboolean G_NAV_CheckNodeFailedForEnt(const int entID, const int nodeNum)
{
	return NAV_CheckNodeFailedForEnt(&g_entities[entID], nodeNum);
}

/*
============
GetModuleAPI
============
*/

gameImport_t* trap = NULL;

Q_EXPORT gameExport_t* QDECL GetModuleAPI(const int apiVersion, gameImport_t* import)
{
	static gameExport_t ge = { 0 };

	assert(import);
	trap = import;
	Com_Printf = trap->Print;
	Com_Error = trap->Error;

	memset(&ge, 0, sizeof ge);

	if (apiVersion != GAME_API_VERSION)
	{
		trap->Print("Mismatched GAME_API_VERSION: expected %i, got %i\n", GAME_API_VERSION, apiVersion);
		return NULL;
	}

	ge.InitGame = G_InitGame;
	ge.ShutdownGame = G_ShutdownGame;
	ge.ClientConnect = ClientConnect;
	ge.ClientBegin = ClientBegin;
	ge.client_userinfo_changed = client_userinfo_changed;
	ge.ClientDisconnect = ClientDisconnect;
	ge.ClientCommand = ClientCommand;
	ge.ClientThink = ClientThink;
	ge.RunFrame = G_RunFrame;
	ge.ConsoleCommand = ConsoleCommand;
	ge.BotAIStartFrame = bot_ai_startframe;
	ge.ROFF_NotetrackCallback = _G_ROFF_NotetrackCallback;
	ge.SpawnRMGEntity = G_SpawnRMGEntity;
	ge.ICARUS_PlaySound = G_ICARUS_PlaySound;
	ge.ICARUS_Set = G_ICARUS_Set;
	ge.ICARUS_Lerp2Pos = G_ICARUS_Lerp2Pos;
	ge.ICARUS_Lerp2Origin = G_ICARUS_Lerp2Origin;
	ge.ICARUS_Lerp2Angles = G_ICARUS_Lerp2Angles;
	ge.ICARUS_GetTag = G_ICARUS_GetTag;
	ge.ICARUS_Lerp2Start = G_ICARUS_Lerp2Start;
	ge.ICARUS_Lerp2End = G_ICARUS_Lerp2End;
	ge.ICARUS_Use = G_ICARUS_Use;
	ge.ICARUS_Kill = G_ICARUS_Kill;
	ge.ICARUS_Remove = G_ICARUS_Remove;
	ge.ICARUS_Play = G_ICARUS_Play;
	ge.ICARUS_GetFloat = G_ICARUS_GetFloat;
	ge.ICARUS_GetVector = G_ICARUS_GetVector;
	ge.ICARUS_GetString = G_ICARUS_GetString;
	ge.ICARUS_SoundIndex = G_ICARUS_SoundIndex;
	ge.ICARUS_GetSetIDForString = G_ICARUS_GetSetIDForString;
	ge.NAV_ClearPathToPoint = G_NAV_ClearPathToPoint;
	ge.NPC_ClearLOS2 = G_NPC_ClearLOS2;
	ge.NAVNEW_ClearPathBetweenPoints = NAVNEW_ClearPathBetweenPoints;
	ge.NAV_CheckNodeFailedForEnt = G_NAV_CheckNodeFailedForEnt;
	ge.NAV_EntIsUnlockedDoor = G_EntIsUnlockedDoor;
	ge.NAV_EntIsDoor = G_EntIsDoor;
	ge.NAV_EntIsBreakable = G_EntIsBreakable;
	ge.NAV_EntIsRemovableUsable = G_EntIsRemovableUsable;
	ge.NAV_FindCombatPointWaypoints = CP_FindCombatPointWaypoints;
	ge.BG_GetItemIndexByTag = BG_GetItemIndexByTag;

	return &ge;
}

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain(const int command, const intptr_t arg0, const intptr_t arg1, const intptr_t arg2,
	const intptr_t arg3, const intptr_t arg4,
	const intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9,
	intptr_t arg10,
	intptr_t arg11)
{
	switch (command)
	{
	case GAME_INIT:
		G_InitGame(arg0, arg1, arg2);
		return 0;

	case GAME_SHUTDOWN:
		G_ShutdownGame(arg0);
		return 0;

	case GAME_CLIENT_CONNECT:
		return (intptr_t)ClientConnect(arg0, arg1, arg2);

	case GAME_CLIENT_THINK:
		ClientThink(arg0, NULL);
		return 0;

	case GAME_CLIENT_USERINFO_CHANGED:
		client_userinfo_changed(arg0);
		return 0;

	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect(arg0);
		return 0;

	case GAME_CLIENT_BEGIN:
		ClientBegin(arg0, qtrue);
		return 0;

	case GAME_CLIENT_COMMAND:
		ClientCommand(arg0);
		return 0;

	case GAME_RUN_FRAME:
		G_RunFrame(arg0);
		return 0;

	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();

	case BOTAI_startFrame:
		return bot_ai_startframe(arg0);

	case GAME_ROFF_NOTETRACK_CALLBACK:
		_G_ROFF_NotetrackCallback(arg0, (const char*)arg1);
		return 0;

	case GAME_SPAWN_RMG_ENTITY:
		G_SpawnRMGEntity();
		return 0;

	case GAME_ICARUS_PLAYSOUND:
		return G_ICARUS_PlaySound();

	case GAME_ICARUS_SET:
		return G_ICARUS_Set();

	case GAME_ICARUS_LERP2POS:
		G_ICARUS_Lerp2Pos();
		return 0;

	case GAME_ICARUS_LERP2ORIGIN:
		G_ICARUS_Lerp2Origin();
		return 0;

	case GAME_ICARUS_LERP2ANGLES:
		G_ICARUS_Lerp2Angles();
		return 0;

	case GAME_ICARUS_GETTAG:
		return G_ICARUS_GetTag();

	case GAME_ICARUS_LERP2START:
		G_ICARUS_Lerp2Start();
		return 0;

	case GAME_ICARUS_LERP2END:
		G_ICARUS_Lerp2End();
		return 0;

	case GAME_ICARUS_USE:
		G_ICARUS_Use();
		return 0;

	case GAME_ICARUS_KILL:
		G_ICARUS_Kill();
		return 0;

	case GAME_ICARUS_REMOVE:
		G_ICARUS_Remove();
		return 0;

	case GAME_ICARUS_PLAY:
		G_ICARUS_Play();
		return 0;

	case GAME_ICARUS_GETFLOAT:
		return G_ICARUS_GetFloat();

	case GAME_ICARUS_GETVECTOR:
		return G_ICARUS_GetVector();

	case GAME_ICARUS_GETSTRING:
		return G_ICARUS_GetString();

	case GAME_ICARUS_SOUNDINDEX:
		G_ICARUS_SoundIndex();
		return 0;

	case GAME_ICARUS_GETSETIDFORSTRING:
		return G_ICARUS_GetSetIDForString();

	case GAME_NAV_CLEARPATHTOPOINT:
		return G_NAV_ClearPathToPoint(arg0, (float*)arg1, (float*)arg2, (float*)arg3, arg4, arg5);

	case GAME_NAV_CLEARLOS:
		return G_NPC_ClearLOS2(arg0, (const float*)arg1);

	case GAME_NAV_CLEARPATHBETWEENPOINTS:
		return NAVNEW_ClearPathBetweenPoints((float*)arg0, (float*)arg1, (float*)arg2, (float*)arg3, arg4, arg5);

	case GAME_NAV_CHECKNODEFAILEDFORENT:
		return NAV_CheckNodeFailedForEnt(&g_entities[arg0], arg1);

	case GAME_NAV_ENTISUNLOCKEDDOOR:
		return G_EntIsUnlockedDoor(arg0);

	case GAME_NAV_ENTISDOOR:
		return G_EntIsDoor(arg0);

	case GAME_NAV_ENTISBREAKABLE:
		return G_EntIsBreakable(arg0);

	case GAME_NAV_ENTISREMOVABLEUSABLE:
		return G_EntIsRemovableUsable(arg0);

	case GAME_NAV_FINDCOMBATPOINTWAYPOINTS:
		CP_FindCombatPointWaypoints();
		return 0;

	case GAME_GETITEMINDEXBYTAG:
		return BG_GetItemIndexByTag(arg0, arg1);
	default:;
	}

	return -1;
}