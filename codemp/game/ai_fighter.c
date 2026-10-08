/*
===========================================================================
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

/*
===========================================================================
AI pilots for the empty ships of the space maps (mp/siege_destroyer2 and the like): the MP version of the singleplayer
AI_Fighter.cpp

A map's ship spawners (NPC_Vehicle with a "teamowner") make the ships of two sides, team 1 (red: the Rebels on
siege_destroyer2) and team 2 (blue: the Empire). The players of a side fly its ships; the ones they leave standing in
the hangar can get an AI pilot (G_FighterAI_Frame), so the battle is never empty. A side always keeps enough empty
ships for its players who are not flying one: an AI pilot only gets in when the side has more empty ships than that,
and only up to g_spaceWingmen AI ships for team 1 and g_spaceEnemies for team 2. A ship that is lost and comes back
stays empty when a player needs it.

The pilot flies it by filling in its usercmd like a player (NPC_FighterAI, run from NPC_RunBehavior): it picks an enemy
ship, leads it and shoots when it is lined up, breaks away when too close, turns away from the Star Destroyer,
asteroids and walls ahead of it, and uses turbo and its missiles now and then. Without an enemy it flies with a player
of its side, or patrols (along the map's ship route if it has one: see "Ship routes" below).

g_spaceBattle 0 switches it off, g_spaceAISkill sets how good they are (0-3, -1: medium).
===========================================================================
*/

#include "b_local.h"

extern gentity_t* NPC_Spawn_Do(gentity_t* ent);
extern Vehicle_t* G_IsRidingVehicle(const gentity_t* pEnt);
extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);
extern vmCvar_t bot_wp_edit;

#define FIGHTER_PILOT_CLASSNAME "fighter_pilot"
#define FIGHTER_THINK_SECONDS 0.1f // an NPC's behaviour runs every 100 msec in MP
#define FIGHTER_FIRE_CONE 6.0f // degrees off the aim point it still shoots at
#define FIGHTER_FIRE_RANGE 7000.0f
#define FIGHTER_BREAK_DIST 450.0f // closer than this to its target, it breaks away
#define FIGHTER_MAX_PITCH 70.0f // a rider's pitch is clamped to about 75
#define FIGHTER_CREW_INTERVAL 2000 // how often the sides' empty ships are looked at

static int route_num_points; // the map's ship route (see "Ship routes" below)

static int fighter_board[MAX_GENTITIES]; // a pilot made for a ship: the ship he gets into (+1, 0: none)
static int fighter_reserved[MAX_GENTITIES]; // a ship a pilot is being made for: until when nobody else takes it
static int fighter_reserver[MAX_GENTITIES]; // a bot who has a ship reserved: its number + 1 (0: an AI pilot)
static qboolean fighter_bot_flying[MAX_GENTITIES]; // a bot the fighter AI flies (G_FighterAI_BotCmd)
static int fighter_bot_next[MAX_GENTITIES]; // when his ship's moves are worked out next (every FIGHTER_THINK_SECONDS)
static usercmd_t fighter_bot_cmd[MAX_GENTITIES]; // and the moves, kept in between
static int fighter_side[MAX_GENTITIES]; // a made pilot's side (NPC_Begin puts siege NPCs on a team by who they hate)
static qboolean fighter_flown[MAX_GENTITIES]; // a ship a player has flown since it was made
static int fighter_crew_time;
static int fighter_out_since[MAX_GENTITIES]; // an AI ship out past the map's edges: since when
static vec3_t fighter_launch_pos[MAX_GENTITIES]; // where a pilot got into his ship, and the way it stood
static vec3_t fighter_launch_angles[MAX_GENTITIES];
// a ship that has just taken off on a hangar map (Fighter_HangarExit): until when it is still leaving (0: not), and
// the height it took off from (the way it stood: fighter_launch_angles)
#define FIGHTER_EXIT_TIME 12000
#define FIGHTER_EXIT_SPEED_HANGAR 1000 // the speed it leaves the hangar at, and climbs out at
#define FIGHTER_EXIT_SPEED_CLIMB 700
static int fighter_exit_until[MAX_GENTITIES];
static float fighter_exit_z[MAX_GENTITIES];
static int fighter_walk_report[MAX_GENTITIES]; // developer messages of a walking pilot: when next
#define WALK_STUCK_TIME 3000 // a walking pilot who has not got anywhere for this long gets in at once
static vec3_t fighter_walk_last_pos[MAX_GENTITIES]; // where he last got to, and when
static int fighter_walk_last_move[MAX_GENTITIES];
static qboolean Fighter_AutoSpawnMap(void);
static int fighter_jump_wait[MAX_GENTITIES]; // a pilot turning his ship to a hyperspace jump: since when
static vec3_t fighter_box_mins, fighter_box_maxs; // the space the ships fly in (Fighter_FindPlayBox)
static qboolean fighter_box_set;

static void Fighter_FindPlayBox(void);

/*
===========================================================================
Walking to the ships (as the singleplayer AI_Fighter.cpp)

On a map with a walk file (shiproutes/<map>.walk: deathstar_trench_v1 / v2), a ship's AI pilot is not made inside
it: he is made at the hangar's pilot start point nearest to the ship ("s x y z" lines: the player spawn areas) and
walks to it along the hangar's walk points ("w x y z"), each linked to the ones he can walk straight to. Next to the
ship he gets in (Fighter_Board), and from then on the fighter AI flies it (his own AI does nothing until the ship is
gone). One who can't get there in WALK_TIMEOUT is put in it. A map without the file: the pilot is made inside the
ship, as before.
===========================================================================
*/
#define WALK_MAX_POINTS 96
#define WALK_LINK_DIST 1600.0f
#define WALK_REACHED 48.0f // a walk point is reached this close (across)
#define WALK_BOARD_DIST 96.0f // he gets in this close to the ship's side
#define WALK_TIMEOUT 30000

static vec3_t walk_points[WALK_MAX_POINTS];
static qboolean walk_is_start[WALK_MAX_POINTS];
static qboolean walk_links[WALK_MAX_POINTS][WALK_MAX_POINTS];
static qboolean walk_links_dirty;
static int walk_num_points;
static int fighter_walk_since[MAX_GENTITIES]; // a pilot walking to his ship (fighter_board): since when (0: not)

static qboolean Walk_Clear(const vec3_t a, const vec3_t b)
{
	const vec3_t mins = { -12.0f, -12.0f, 0.0f };
	const vec3_t maxs = { 12.0f, 12.0f, 16.0f };
	trace_t tr;
	trap->Trace(&tr, a, mins, maxs, b, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);
	return !tr.startsolid && !tr.allsolid && tr.fraction >= 1.0f ? qtrue : qfalse;
}

static void Walk_BuildLinks(void)
{
	if (!walk_links_dirty)
	{
		return;
	}
	walk_links_dirty = qfalse;
	memset(walk_links, 0, sizeof walk_links);
	for (int i = 0; i < walk_num_points; i++)
	{
		for (int j = i + 1; j < walk_num_points; j++)
		{
			if (Distance(walk_points[i], walk_points[j]) <= WALK_LINK_DIST && Walk_Clear(walk_points[i], walk_points[j]))
			{
				walk_links[i][j] = walk_links[j][i] = qtrue;
			}
		}
	}
}

static void Walk_Load(void)
{
	walk_num_points = 0;
	walk_links_dirty = qtrue;
	memset(fighter_walk_since, 0, sizeof fighter_walk_since);

	char map_name[MAX_QPATH];
	trap->Cvar_VariableStringBuffer("mapname", map_name, sizeof map_name);
	const char* file_name = va("shiproutes/%s.walk", map_name);
	fileHandle_t f;
	const int len = trap->FS_Open(file_name, &f, FS_READ);
	if (len <= 0 || !f)
	{
		if (f)
		{
			trap->FS_Close(f);
		}
		return;
	}
	char* buffer = (char*)BG_TempAlloc(len + 1);
	trap->FS_Read(buffer, len, f);
	trap->FS_Close(f);
	buffer[len] = 0;

	const char* line = buffer;
	while (line && *line && walk_num_points < WALK_MAX_POINTS)
	{
		char kind = 0;
		vec3_t point;
		if (sscanf(line, " %c %f %f %f", &kind, &point[0], &point[1], &point[2]) == 4 && (kind == 's' || kind == 'w'))
		{
			VectorCopy(point, walk_points[walk_num_points]);
			walk_is_start[walk_num_points] = kind == 's' ? qtrue : qfalse;
			walk_num_points++;
		}
		line = strchr(line, '\n');
		if (line)
		{
			line++;
		}
	}
	BG_TempFree(len + 1);
	if (Fighter_Developer())
	{
		Com_Printf("Ship walk: %d points from %s\n", walk_num_points, file_name);
	}
}

// someone (alive) is standing on a pilot start
static qboolean Walk_StartTaken(const vec3_t start)
{
	for (int i = 0; i < level.num_entities; i++)
	{
		const gentity_t* ent = &g_entities[i];
		if (ent->inuse && ent->client && ent->health > 0 && DistanceHorizontal(ent->r.currentOrigin, start) < 48.0f
			&& fabs(ent->r.currentOrigin[2] - start[2]) < 80.0f)
		{
			return qtrue;
		}
	}
	return qfalse;
}

// the free pilot start point nearest to a ship (-1: the map has none, or all are taken)
static int Walk_NearestStart(const vec3_t pos)
{
	int best = -1;
	float best_dist = 1.0e30f;
	for (int i = 0; i < walk_num_points; i++)
	{
		const float dist = DistanceSquared(pos, walk_points[i]);
		if (walk_is_start[i] && dist < best_dist && !Walk_StartTaken(walk_points[i]))
		{
			best_dist = dist;
			best = i;
		}
	}
	return best;
}

// where a walking pilot heads now: the walk point that is on the shortest way to the one nearest his ship, or (past
// it, or with no way) the ship itself
static void Walk_NextGoal(const vec3_t pos, const vec3_t ship_pos, vec3_t goal)
{
	float way[WALK_MAX_POINTS];
	qboolean done[WALK_MAX_POINTS];

	VectorCopy(ship_pos, goal);
	goal[2] = pos[2];
	if (!walk_num_points)
	{
		return;
	}
	Walk_BuildLinks();

	// the walk point nearest the ship (across), and the way there from every point (Dijkstra)
	int end = -1;
	float end_dist = 1.0e30f;
	for (int i = 0; i < walk_num_points; i++)
	{
		const float dist = DistanceHorizontalSquared(walk_points[i], ship_pos);
		if (dist < end_dist)
		{
			end_dist = dist;
			end = i;
		}
	}
	if (DistanceHorizontalSquared(pos, ship_pos) <= end_dist)
	{
		return; // as near the ship as that point: straight on to it
	}
	for (int i = 0; i < walk_num_points; i++)
	{
		way[i] = 1.0e30f;
		done[i] = qfalse;
	}
	way[end] = 0.0f;
	for (;;)
	{
		int cur = -1;
		for (int i = 0; i < walk_num_points; i++)
		{
			if (!done[i] && way[i] < 1.0e29f && (cur < 0 || way[i] < way[cur]))
			{
				cur = i;
			}
		}
		if (cur < 0)
		{
			break;
		}
		done[cur] = qtrue;
		for (int i = 0; i < walk_num_points; i++)
		{
			if (walk_links[cur][i] && way[cur] + Distance(walk_points[cur], walk_points[i]) < way[i])
			{
				way[i] = way[cur] + Distance(walk_points[cur], walk_points[i]);
			}
		}
	}

	// the best point he can walk straight to (not the one he stands on, unless it is the last)
	int best = -1;
	float best_way = 1.0e30f;
	for (int i = 0; i < walk_num_points; i++)
	{
		if (way[i] >= 1.0e29f || i != end && DistanceHorizontal(pos, walk_points[i]) < WALK_REACHED)
		{
			continue;
		}
		const float total = Distance(pos, walk_points[i]) + way[i];
		if (total < best_way && Distance(pos, walk_points[i]) <= WALK_LINK_DIST && Walk_Clear(pos, walk_points[i]))
		{
			best_way = total;
			best = i;
		}
	}
	if (best >= 0 && !(best == end && DistanceHorizontal(pos, walk_points[end]) < WALK_REACHED))
	{
		VectorCopy(walk_points[best], goal);
	}
}

static int Fighter_Developer(void)
{
	return trap->Cvar_VariableIntegerValue("developer");
}

// 0 (easy) .. 3 (hard)
static int Fighter_Skill(void)
{
	const int skill = g_spaceAISkill.integer < 0 ? 2 : g_spaceAISkill.integer;
	return skill > 3 ? 3 : skill;
}

static qboolean Fighter_IsFighter(const gentity_t* ent)
{
	return ent && ent->inuse && ent->client && ent->client->NPC_class == CLASS_VEHICLE && ent->m_pVehicle
		&& ent->m_pVehicle->m_pVehicleInfo && ent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER;
}

// the side a ship is on is its pilot's (an empty ship is on no side): a player's team, or (no teams) the side whose
// hangar the ship came from
static int Fighter_ShipSide(const gentity_t* ship)
{
	const gentity_t* pilot = (const gentity_t*)ship->m_pVehicle->m_pPilot;
	if (!pilot || !pilot->client || pilot->health <= 0)
	{
		return TEAM_FREE;
	}
	if (pilot->client->sess.sessionTeam == TEAM_RED || pilot->client->sess.sessionTeam == TEAM_BLUE)
	{
		return pilot->client->sess.sessionTeam;
	}
	return ship->s.teamowner == TEAM_RED || ship->s.teamowner == TEAM_BLUE ? ship->s.teamowner : TEAM_FREE;
}

static qboolean Fighter_IsAIPilot(const gentity_t* ent)
{
	return ent && ent->inuse && ent->NPC && ent->classname && !Q_stricmp(ent->classname, FIGHTER_PILOT_CLASSNAME);
}

// the AI pilots of a side (in a ship or about to get in)
static int Fighter_CrewCount(const int side)
{
	int count = 0;
	for (int i = MAX_CLIENTS; i < level.num_entities; i++)
	{
		const gentity_t* ent = &g_entities[i];
		if (Fighter_IsAIPilot(ent) && ent->health > 0 && fighter_side[i] == side)
		{
			count++;
		}
	}
	return count;
}

// the players of a side who are not flying a ship of it (with no teams, every player counts for both sides)
static int Fighter_PlayersWithoutShip(const int side)
{
	int count = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		const gentity_t* ent = &g_entities[i];
		if (!ent->inuse || !ent->client || ent->client->pers.connected != CON_CONNECTED || ent->r.svFlags & SVF_BOT)
		{
			continue;
		}
		const team_t team = ent->client->sess.sessionTeam;
		if (team == TEAM_SPECTATOR || team != TEAM_FREE && team != side)
		{
			continue;
		}
		if (ent->client->ps.m_iVehicleNum && Fighter_IsFighter(&g_entities[ent->client->ps.m_iVehicleNum]))
		{
			continue;
		}
		count++;
	}
	return count;
}

// an empty ship of a side, nobody about to get in
static qboolean Fighter_IsFreeShip(const gentity_t* ship, const int side)
{
	return Fighter_IsFighter(ship) && ship->health > 0 && !ship->m_pVehicle->m_pPilot
		&& ship->s.teamowner == side && fighter_reserved[ship->s.number] < level.time;
}

// an empty ship still waiting where it was made (no player has flown it, not moving): one an AI pilot may take (not
// one a player has left somewhere)
static qboolean Fighter_IsParkedShip(const gentity_t* ship)
{
	return !fighter_flown[ship->s.number] && VectorLength(ship->client->ps.velocity) < 50.0f;
}

// a ship made by its spawner (G_VehicleSpawn): nobody has flown it yet
void G_FighterAI_ShipSpawned(const gentity_t* ship)
{
	if (ship)
	{
		fighter_flown[ship->s.number] = qfalse;
		fighter_out_since[ship->s.number] = 0;
	}
}

// the pilot for a ship: by what it is (a TIE's is the Empire's, whichever team's hangar it is in: the TIEs' hangars
// are team 1's on deathstar_trench), else by its side
static char* Fighter_PilotType(const gentity_t* ship, const int side)
{
	if (ship->NPC_type && Q_stristr(ship->NPC_type, "tie"))
	{
		return "stormpilot";
	}
	if (ship->NPC_type && (Q_stristr(ship->NPC_type, "wing") || Q_stristr(ship->NPC_type, "yt-1300")
		|| Q_stristr(ship->NPC_type, "falcon")))
	{
		return "Rebel_Pilot";
	}
	return side == TEAM_RED ? "Rebel_Pilot" : "stormpilot";
}

// a pilot for this ship, made right in it (hidden in there); he gets in on his first think (NPC_FighterAI)
static void Fighter_MakePilot(gentity_t* ship, const int side)
{
	gentity_t* spawner = G_Spawn();
	if (!spawner)
	{
		return;
	}
	spawner->classname = "NPC_spawner";
	spawner->NPC_type = Fighter_PilotType(ship, side);
	spawner->count = 1;
	spawner->s.teamowner = side;
	// at the hangar's pilot start nearest the ship when the map has a walk file (he walks to it), else inside it
	const int start = Walk_NearestStart(ship->r.currentOrigin);
	vec3_t pos;
	VectorCopy(start >= 0 ? walk_points[start] : ship->r.currentOrigin, pos);
	spawner->spawnflags = start >= 0 ? 0 : 64 | 128; // inside the ship: NOTSOLID | STARTINSOLID
	G_SetOrigin(spawner, pos);
	VectorCopy(pos, spawner->s.origin);

	gentity_t* pilot = NPC_Spawn_Do(spawner); // (frees the spawner)
	if (start >= 0 && (!pilot || !pilot->client || !pilot->NPC))
	{
		// the start is taken: inside the ship, as on a map without a walk file
		spawner = G_Spawn();
		if (!spawner)
		{
			return;
		}
		spawner->classname = "NPC_spawner";
		spawner->NPC_type = Fighter_PilotType(ship, side);
		spawner->count = 1;
		spawner->s.teamowner = side;
		spawner->spawnflags = 64 | 128;
		G_SetOrigin(spawner, ship->r.currentOrigin);
		VectorCopy(ship->r.currentOrigin, spawner->s.origin);
		pilot = NPC_Spawn_Do(spawner);
		if (pilot && pilot->client && pilot->NPC)
		{
			fighter_walk_since[pilot->s.number] = 0;
		}
	}
	else if (start >= 0)
	{
		fighter_walk_since[pilot->s.number] = level.time;
		fighter_walk_report[pilot->s.number] = 0;
		VectorCopy(pilot->r.currentOrigin, fighter_walk_last_pos[pilot->s.number]);
		fighter_walk_last_move[pilot->s.number] = level.time;
	}
	if (!pilot || !pilot->client || !pilot->NPC)
	{
		if (Fighter_Developer())
		{
			Com_Printf("fighter AI: no pilot could be made for %s %d\n", ship->NPC_type, ship->s.number);
		}
		return;
	}
	pilot->classname = FIGHTER_PILOT_CLASSNAME;
	pilot->client->sess.sessionTeam = side;
	pilot->client->ps.persistant[PERS_TEAM] = side;
	fighter_board[pilot->s.number] = ship->s.number + 1;
	fighter_side[pilot->s.number] = side;
	fighter_reserved[ship->s.number] = level.time + 3000;
	fighter_reserver[ship->s.number] = 0;
	if (fighter_walk_since[pilot->s.number])
	{
		// he walks to it first (unarmed: a ship only fires for a pilot with no weapon in his hands)
		pilot->client->ps.weapon = WP_NONE;
		pilot->s.weapon = WP_NONE;
		pilot->client->ps.stats[STAT_WEAPONS] = 0;
		if (Fighter_Developer())
		{
			Com_Printf("fighter AI: %s %d walks to %s %d for team %d\n", pilot->NPC_type, pilot->s.number,
				ship->NPC_type, ship->s.number, side);
		}
	}
}

/*
-------------------------
G_FighterAI_AutoSpawn

On the maps whose ships are only made by a button in their hangar (a trigger_multiple using the NPC_Vehicle
spawner: deathstar_trench_v1 / v2), the game uses the spawners itself, at the start of the map and then every
FIGHTER_AUTOSPAWN_INTERVAL, so there are ships for the AI pilots (G_FighterAI_Frame) and the players. A spawner is
used only when its spot is empty (its last ship has flown out or is gone), so a hangar holds one ship per spot.
g_spaceBattle 0 switches it off. (As the singleplayer AI_Fighter.cpp.)
-------------------------
*/
#define FIGHTER_AUTOSPAWN_INTERVAL 60000
#define FIGHTER_AUTOSPAWN_START 5000 // the first time, after the start of the map
#define FIGHTER_AUTOSPAWN_SPOT 250.0f // a ship this close to the spawner (across) is on its spot
#define FIGHTER_AUTOSPAWN_MAX_CREW 12 // AI pilots of a side at most on those maps (the entities)
static int fighter_autospawn_time;

extern void NPC_VehicleSpawnUse(gentity_t* self, gentity_t* other, gentity_t* activator);
extern void G_VehicleSpawn(gentity_t* self);

static qboolean Fighter_AutoSpawnMap(void)
{
	static const char* maps[] = { "deathstar_trench_v1", "deathstar_trench_v2" };
	char map_name[MAX_QPATH];
	trap->Cvar_VariableStringBuffer("mapname", map_name, sizeof map_name);
	for (int i = 0; i < (int)ARRAY_LEN(maps); i++)
	{
		if (!Q_stricmp(map_name, maps[i]))
		{
			return qtrue;
		}
	}
	return qfalse;
}

// a ship (any vehicle, alive) still on the spawner's spot
static qboolean Fighter_SpawnSpotTaken(const gentity_t* spawner)
{
	for (int i = 0; i < level.num_entities; i++)
	{
		const gentity_t* ent = &g_entities[i];
		if (ent == spawner || !ent->inuse || !ent->client || ent->client->NPC_class != CLASS_VEHICLE
			|| ent->health <= 0)
		{
			continue;
		}
		if (DistanceHorizontal(ent->r.currentOrigin, spawner->r.currentOrigin) < FIGHTER_AUTOSPAWN_SPOT
			&& fabs(ent->r.currentOrigin[2] - spawner->r.currentOrigin[2]) < 300.0f)
		{
			return qtrue;
		}
	}
	return qfalse;
}

static void G_FighterAI_AutoSpawn(void)
{
	if (fighter_autospawn_time > level.time + FIGHTER_AUTOSPAWN_INTERVAL + FIGHTER_AUTOSPAWN_START)
	{
		fighter_autospawn_time = 0; // a new map (the time went back)
	}
	if (!fighter_autospawn_time)
	{
		fighter_autospawn_time = level.time + FIGHTER_AUTOSPAWN_START;
	}
	if (fighter_autospawn_time > level.time)
	{
		return;
	}
	fighter_autospawn_time = level.time + FIGHTER_AUTOSPAWN_INTERVAL;

	if (!g_spaceBattle.integer || !Fighter_AutoSpawnMap())
	{
		return;
	}
	for (int i = MAX_CLIENTS; i < level.num_entities; i++)
	{
		gentity_t* spawner = &g_entities[i];
		if (!spawner->inuse || !spawner->classname || Q_stricmp(spawner->classname, "NPC_Vehicle")
			|| !spawner->targetname || spawner->use != NPC_VehicleSpawnUse
			|| spawner->s.teamowner != TEAM_RED && spawner->s.teamowner != TEAM_BLUE)
		{
			continue; // only the hangars' ship spawners (made by a button)
		}
		if (spawner->think == G_VehicleSpawn && spawner->nextthink > level.time)
		{
			continue; // already making one (its "delay")
		}
		if (Fighter_SpawnSpotTaken(spawner))
		{
			continue;
		}
		if (Fighter_Developer())
		{
			Com_Printf("fighter AI: hangar spawner %s makes a %s\n", spawner->targetname, spawner->NPC_type);
		}
		spawner->use(spawner, spawner, spawner);
	}
}

/*
-------------------------
G_FighterAI_Frame

Every few seconds: an AI pilot for an empty ship of a side that has more empty ships than players who need one.
-------------------------
*/
void G_FighterAI_Frame(void)
{
	if (!g_spaceBattle.integer || fighter_crew_time > level.time)
	{
		return;
	}
	fighter_crew_time = level.time + FIGHTER_CREW_INTERVAL;

	// an AI ship lost off the map (out past its edges, not jumping: a wreck spinning out of control flies straight out
	// of the world, where nothing stops it) is destroyed after a while, as the map's edges do to a broken ship; its
	// spawner makes another
	if (!fighter_box_set)
	{
		Fighter_FindPlayBox();
	}
	for (int i = MAX_CLIENTS; i < level.num_entities; i++)
	{
		gentity_t* ship = &g_entities[i];
		if (!Fighter_IsFighter(ship) || ship->health <= 0 || !ship->m_pVehicle->m_pPilot
			|| !Fighter_IsAIPilot((const gentity_t*)ship->m_pVehicle->m_pPilot))
		{
			fighter_out_since[i] = 0;
			continue;
		}
		qboolean out = qfalse;
		for (int axis = 0; axis < 3; axis++)
		{
			if (ship->r.currentOrigin[axis] < fighter_box_mins[axis] - 500.0f
				|| ship->r.currentOrigin[axis] > fighter_box_maxs[axis] + 500.0f)
			{
				out = qtrue;
			}
		}
		if (!out || ship->client->ps.hyperSpaceTime && level.time - ship->client->ps.hyperSpaceTime < HYPERSPACE_TIME)
		{
			fighter_out_since[i] = 0;
		}
		else if (!fighter_out_since[i])
		{
			fighter_out_since[i] = level.time;
		}
		else if (level.time - fighter_out_since[i] > 15000)
		{
			if (Fighter_Developer())
			{
				Com_Printf("fighter AI: %s %d is lost off the map at %s, destroyed\n", ship->NPC_type, i,
					vtos(ship->r.currentOrigin));
			}
			fighter_out_since[i] = 0;
			G_Damage(ship, ship, ship, NULL, ship->r.currentOrigin, 99999, DAMAGE_NO_PROTECTION, MOD_SUICIDE);
		}
	}

	// the ships players fly (they are not taken by an AI pilot when left)
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		const gentity_t* ent = &g_entities[i];
		if (ent->inuse && ent->client && ent->client->ps.m_iVehicleNum)
		{
			fighter_flown[ent->client->ps.m_iVehicleNum] = qtrue;
		}
	}

	if (Fighter_Developer() > 2)
	{
		// where the AI ships are and what their pilots tell them
		for (int i = MAX_CLIENTS; i < level.num_entities; i++)
		{
			const gentity_t* ship = &g_entities[i];
			if (Fighter_IsFighter(ship) && ship->m_pVehicle->m_pPilot
				&& Fighter_IsAIPilot((const gentity_t*)ship->m_pVehicle->m_pPilot))
			{
				const gentity_t* pilot = (const gentity_t*)ship->m_pVehicle->m_pPilot;
				Com_Printf("fighter AI: %s %d at %s speed %.0f move %d up %d flags %lx enemy %d turn %d orient %s\n",
					ship->NPC_type, i, vtos(ship->r.currentOrigin), VectorLength(ship->client->ps.velocity),
					ship->m_pVehicle->m_ucmd.forwardmove, ship->m_pVehicle->m_ucmd.upmove, ship->m_pVehicle->m_ulFlags,
					pilot->enemy ? pilot->enemy->s.number : -1, ship->client->ps.vehTurnaroundTime - level.time,
					vtos(ship->m_pVehicle->m_vOrientation));
				if (ship->client->ps.hyperSpaceTime && level.time - ship->client->ps.hyperSpaceTime < HYPERSPACE_TIME)
				{
					Com_Printf("    hyperspace %d ms ef2 %x: ship %s jump %s view %s owner %d pilot %d\n",
						level.time - ship->client->ps.hyperSpaceTime, ship->client->ps.eFlags2,
						vtos(ship->m_pVehicle->m_vOrientation), vtos(ship->client->ps.hyperSpaceAngles),
						vtos(pilot->client->ps.viewangles), ship->s.owner, pilot->s.number);
				}
			}
		}
	}

	for (int side = TEAM_RED; side <= TEAM_BLUE; side++)
	{
		// on the maps whose hangars the game fills (G_FighterAI_AutoSpawn) every new ship gets a pilot, up to
		// FIGHTER_AUTOSPAWN_MAX_CREW of a side, in place of g_spaceWingmen / g_spaceEnemies
		const int max_crew = Fighter_AutoSpawnMap() ? FIGHTER_AUTOSPAWN_MAX_CREW
			: side == TEAM_RED ? g_spaceWingmen.integer : g_spaceEnemies.integer;
		const int crew = Fighter_CrewCount(side);
		if (crew >= max_crew)
		{
			continue;
		}
		int free_ships = 0;
		gentity_t* candidate = NULL;
		for (int i = MAX_CLIENTS; i < level.num_entities; i++)
		{
			gentity_t* ship = &g_entities[i];
			if (!Fighter_IsFreeShip(ship, side))
			{
				continue;
			}
			free_ships++;
			if (!candidate && Fighter_IsParkedShip(ship)
				&& !Q_stristr(ship->NPC_type, "shuttle") && !Q_stristr(ship->NPC_type, "yt-1300"))
			{
				candidate = ship; // (the transports are scenery)
			}
		}
		// one at a time (the next one in a moment), the ones the players need left for them
		const int players = Fighter_PlayersWithoutShip(side);
		if (Fighter_Developer() > 1)
		{
			Com_Printf("fighter AI: team %d: %d AI ships (at most %d), %d empty ships, %d players without one%s\n", side,
				crew, max_crew, free_ships, players, candidate ? "" : ", none waiting in a hangar");
		}
		if (candidate && free_ships > players)
		{
			Fighter_MakePilot(candidate, side);
		}
	}
}

// a made pilot walking to his ship: qtrue while he walks (this think's moves are set), qfalse when he is next to it,
// late, or it is gone (Fighter_Board then puts him in it or finds it gone)
static qboolean Fighter_Walk(gentity_t* npc)
{
	if (!fighter_walk_since[npc->s.number])
	{
		return qfalse;
	}
	const int ship_num = fighter_board[npc->s.number] - 1;
	const gentity_t* ship = ship_num >= 0 ? &g_entities[ship_num] : NULL;
	if (!ship || !Fighter_IsFighter(ship) || ship->health <= 0 || ship->m_pVehicle->m_pPilot)
	{
		fighter_walk_since[npc->s.number] = 0;
		return qfalse;
	}
	const int n = npc->s.number;
	if (DistanceHorizontal(npc->r.currentOrigin, fighter_walk_last_pos[n]) > 24.0f)
	{
		VectorCopy(npc->r.currentOrigin, fighter_walk_last_pos[n]);
		fighter_walk_last_move[n] = level.time;
	}
	const qboolean stuck = level.time - fighter_walk_last_move[n] > WALK_STUCK_TIME;
	const float reach = (ship->r.maxs[0] > ship->r.maxs[1] ? ship->r.maxs[0] : ship->r.maxs[1]) + WALK_BOARD_DIST;
	if (DistanceHorizontal(npc->r.currentOrigin, ship->r.currentOrigin) < reach
		|| level.time - fighter_walk_since[n] > WALK_TIMEOUT || stuck)
	{
		if (stuck && Fighter_Developer())
		{
			Com_Printf("fighter AI: %s %d is stuck at %s, gets in now\n", npc->NPC_type, n, vtos(npc->r.currentOrigin));
		}
		fighter_walk_since[npc->s.number] = 0;
		return qfalse;
	}
	fighter_reserved[ship_num] = level.time + 3000; // nobody else takes it while he walks
	fighter_reserver[ship_num] = 0;

	vec3_t goal, dir;
	Walk_NextGoal(npc->r.currentOrigin, ship->r.currentOrigin, goal);
	if (Fighter_Developer() > 1 && fighter_walk_report[npc->s.number] < level.time)
	{
		fighter_walk_report[npc->s.number] = level.time + 2000;
		Com_Printf("fighter AI: %s %d walking at %s to %s (ship %d at %s), speed %.0f\n", npc->NPC_type, npc->s.number,
			vtos(npc->r.currentOrigin), vtos(goal), ship_num, vtos(ship->r.currentOrigin),
			VectorLength(npc->client->ps.velocity));
	}
	VectorSubtract(goal, npc->r.currentOrigin, dir);
	dir[2] = 0.0f;
	NPCS.NPCInfo->desiredYaw = AngleNormalize360(vectoyaw(dir));
	NPCS.NPCInfo->desiredPitch = 0.0f;
	NPC_UpdateAngles(qtrue, qtrue);
	NPCS.ucmd.forwardmove = 127;
	NPCS.ucmd.buttons &= ~BUTTON_WALKING; // he runs
	return qtrue;
}

// the made pilot gets into his ship. qfalse: it is gone (destroyed, taken by a player)
static qboolean Fighter_Board(gentity_t* pilot)
{
	const int ship_num = fighter_board[pilot->s.number] - 1;
	fighter_board[pilot->s.number] = 0;
	if (ship_num < 0)
	{
		return qfalse;
	}
	gentity_t* ship = &g_entities[ship_num];
	fighter_reserved[ship_num] = 0;
	if (!Fighter_IsFighter(ship) || ship->health <= 0 || ship->m_pVehicle->m_pPilot)
	{
		if (Fighter_Developer())
		{
			Com_Printf("fighter AI: %s %d: ship %d is gone or taken\n", pilot->NPC_type, pilot->s.number, ship_num);
		}
		return qfalse;
	}

	// a ship only fires for a pilot with no weapon in his hands
	pilot->client->ps.weapon = WP_NONE;
	pilot->s.weapon = WP_NONE;
	pilot->client->ps.stats[STAT_WEAPONS] = 0;

	// his side (NPC_Begin has put him on a siege team by who he hates, which is not the map's)
	pilot->client->sess.sessionTeam = fighter_side[pilot->s.number];
	pilot->client->ps.persistant[PERS_TEAM] = fighter_side[pilot->s.number];
	pilot->s.teamowner = fighter_side[pilot->s.number];

	Vehicle_t* p_veh = ship->m_pVehicle;
	if (!p_veh->m_pVehicleInfo->Board(p_veh, (bgEntity_t*)pilot))
	{
		if (Fighter_Developer())
		{
			Com_Printf("fighter AI: %s %d could not get into %s %d\n", pilot->NPC_type, pilot->s.number, ship->NPC_type,
				ship_num);
		}
		return qfalse;
	}
	p_veh->m_iBoarding = 0; // in it at once, no boarding time
	pilot->s.m_iVehicleNum = ship->s.number;

	// a ship docked in its rack (SUSPENDED) is let go, as when a player gets in (Board): it drops clear of the rack
	// first ("dropTime"), then flies out (on a hangar map straight on only a moment: it leaves the hangar and the trench
	// by Fighter_HangarExit, not straight on into the trench's far wall)
	const qboolean hangar_map = Fighter_AutoSpawnMap();
	int launch_time = hangar_map ? 1000 : 2500;
	if (ship->spawnflags & 2)
	{
		ship->spawnflags &= ~2;
		G_Sound(ship, CHAN_AUTO, G_SoundIndex("sound/vehicles/common/release.wav"));
		if (ship->fly_sound_debounce_time)
		{
			p_veh->m_iDropTime = level.time + ship->fly_sound_debounce_time;
			launch_time += ship->fly_sound_debounce_time;
		}
	}

	if (Fighter_Developer())
	{
		Com_Printf("fighter AI: %s %d flies %s %d for team %d (on the ground %s, drops for %d ms)\n", pilot->NPC_type,
			pilot->s.number, ship->NPC_type, ship->s.number, pilot->client->sess.sessionTeam,
			ship->client->ps.groundEntityNum != ENTITYNUM_NONE ? "yes" : "no",
			p_veh->m_iDropTime > level.time ? p_veh->m_iDropTime - level.time : 0);
	}

	pilot->painDebounceTime = level.time; // (a hidden pilot is never hurt: the developer messages use it as boarding time)
	VectorCopy(ship->r.currentOrigin, pilot->pos4); // home, the centre of its patrol
	VectorCopy(ship->r.currentOrigin, pilot->pos3);
	TIMER_Set(pilot, "fighterLaunch", launch_time); // drops clear and flies straight out of its hangar first
	// (one standing in a hangar, not hanging in a rack, is slow to get going: it keeps on out until it is clear)
	TIMER_Set(pilot, "fighterLaunchMax", p_veh->m_iDropTime > level.time || hangar_map ? 0 : launch_time + 5500);
	VectorCopy(ship->r.currentOrigin, fighter_launch_pos[pilot->s.number]);
	VectorSet(fighter_launch_angles[pilot->s.number], 0.0f, p_veh->m_vOrientation[YAW], 0.0f);
	fighter_exit_until[pilot->s.number] = hangar_map ? level.time + launch_time + FIGHTER_EXIT_TIME : 0;
	fighter_exit_z[pilot->s.number] = ship->r.currentOrigin[2];
	TIMER_Set(pilot, "fighterRetarget", 0);
	return qtrue;
}

// the ship a player of this side flies nearest to it (a wingman flies with him)
static gentity_t* Fighter_LeaderShip(const gentity_t* ship, const int my_side)
{
	gentity_t* best = NULL;
	float best_dist = 15000.0f;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		const gentity_t* ent = &g_entities[i];
		if (!ent->inuse || !ent->client || !ent->client->ps.m_iVehicleNum || ent->health <= 0)
		{
			continue;
		}
		gentity_t* veh = &g_entities[ent->client->ps.m_iVehicleNum];
		if (!Fighter_IsFighter(veh) || veh == ship || Fighter_ShipSide(veh) != my_side)
		{
			continue;
		}
		const float dist = Distance(veh->r.currentOrigin, ship->r.currentOrigin);
		if (dist < best_dist)
		{
			best_dist = dist;
			best = veh;
		}
	}
	return best;
}

// ships of the other side with a live pilot: the nearest, liking the ones in front of it, and (a wingman) the ones
// after a player of its side
static gentity_t* Fighter_FindTarget(const gentity_t* ship, const int my_side, const vec3_t fwd)
{
	gentity_t* best = NULL;
	float best_score = 1.0e30f;
	// on a map with a route they fly it until they meet the enemy; without one they see the whole map
	const float max_dist = route_num_points ? 20000.0f : 40000.0f;

	for (int i = MAX_CLIENTS; i < level.num_entities; i++)
	{
		gentity_t* ent = &g_entities[i];
		if (ent == ship || !Fighter_IsFighter(ent) || ent->health <= 0)
		{
			continue;
		}
		const int side = Fighter_ShipSide(ent);
		if (side == TEAM_FREE || side == my_side)
		{
			continue;
		}
		const gentity_t* pilot = (const gentity_t*)ent->m_pVehicle->m_pPilot;
		if (pilot->flags & FL_NOTARGET)
		{
			continue;
		}
		vec3_t dir;
		VectorSubtract(ent->r.currentOrigin, ship->r.currentOrigin, dir);
		const float dist = VectorNormalize(dir);
		if (dist > max_dist)
		{
			continue;
		}
		float score = dist;
		if (DotProduct(dir, fwd) > 0.5f)
		{
			score *= 0.6f;
		}
		if (pilot->enemy && pilot->enemy->client && pilot->enemy->m_pVehicle && pilot->enemy->m_pVehicle->m_pPilot
			&& pilot->enemy->m_pVehicle->m_pPilot->s.number < MAX_CLIENTS)
		{
			score *= 0.4f; // on a player's tail
		}
		if (ent == NPCS.NPC->enemy)
		{
			score *= 0.8f; // keep at the one it has
		}
		if (score < best_score)
		{
			best_score = score;
			best = ent;
		}
	}
	return best;
}

// turns the pilot's view (which the ship follows) toward dir, at most max_step degrees
static void Fighter_TurnTowards(const vec3_t dir, const float max_step)
{
	vec3_t want;
	vectoangles(dir, want);
	want[PITCH] = AngleNormalize180(want[PITCH]);

	for (int axis = PITCH; axis <= YAW; axis++)
	{
		const float cur = NPCS.client->ps.viewangles[axis];
		float delta = AngleSubtract(want[axis], cur);
		if (delta > max_step)
		{
			delta = max_step;
		}
		else if (delta < -max_step)
		{
			delta = -max_step;
		}
		float angle = AngleNormalize180(cur + delta);
		if (axis == PITCH)
		{
			angle = Com_Clamp(-FIGHTER_MAX_PITCH, FIGHTER_MAX_PITCH, angle);
			NPCS.NPCInfo->desiredPitch = angle;
		}
		else
		{
			NPCS.NPCInfo->desiredYaw = angle;
		}
		NPCS.ucmd.angles[axis] = ANGLE2SHORT(angle) - NPCS.client->ps.delta_angles[axis];
	}
}

// The space the ships fly in: inside the map's edges (the trigger_shipboundary slabs around it). A ship that flies into
// one is turned around (for a player by his pmove), but at their speed an AI ship is through the thin slab and out of
// the world before it turns, so the AI keeps them off the edges itself.
static void Fighter_FindPlayBox(void)
{
	fighter_box_set = qtrue;
	vec3_t all_mins = { 1.0e9f, 1.0e9f, 1.0e9f };
	vec3_t all_maxs = { -1.0e9f, -1.0e9f, -1.0e9f };
	int count = 0;
	for (int i = MAX_CLIENTS; i < level.num_entities; i++)
	{
		const gentity_t* ent = &g_entities[i];
		if (ent->inuse && ent->classname && !Q_stricmp(ent->classname, "trigger_shipboundary"))
		{
			AddPointToBounds(ent->r.absmin, all_mins, all_maxs);
			AddPointToBounds(ent->r.absmax, all_mins, all_maxs);
			count++;
		}
	}
	VectorCopy(all_mins, fighter_box_mins);
	VectorCopy(all_maxs, fighter_box_maxs);
	if (!count)
	{
		VectorSet(fighter_box_mins, MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD);
		VectorSet(fighter_box_maxs, MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD);
		return;
	}
	// a slab (at least half as wide as the whole lot both other ways) on one side cuts that side off
	for (int i = MAX_CLIENTS; i < level.num_entities; i++)
	{
		const gentity_t* ent = &g_entities[i];
		if (!ent->inuse || !ent->classname || Q_stricmp(ent->classname, "trigger_shipboundary"))
		{
			continue;
		}
		for (int axis = 0; axis < 3; axis++)
		{
			qboolean slab = qtrue;
			for (int other = 0; other < 3; other++)
			{
				if (other != axis && ent->r.absmax[other] - ent->r.absmin[other]
					< 0.5f * (all_maxs[other] - all_mins[other]))
				{
					slab = qfalse;
				}
			}
			if (!slab)
			{
				continue;
			}
			const float centre = (all_mins[axis] + all_maxs[axis]) * 0.5f;
			if (ent->r.absmin[axis] > centre)
			{
				fighter_box_maxs[axis] = Q_min(fighter_box_maxs[axis], ent->r.absmin[axis]);
			}
			else if (ent->r.absmax[axis] < centre)
			{
				fighter_box_mins[axis] = Q_max(fighter_box_mins[axis], ent->r.absmax[axis]);
			}
		}
	}
	if (Fighter_Developer())
	{
		Com_Printf("fighter AI: ships fly in %s - %s\n", vtos(fighter_box_mins), vtos(fighter_box_maxs));
	}
}

// heading out of the play box (where it will be in a moment, a margin inside the edges): the way back in. qfalse: fine
static qboolean Fighter_EdgeAhead(const vec3_t my_pos, const vec3_t ahead, vec3_t back)
{
	if (!fighter_box_set)
	{
		Fighter_FindPlayBox();
	}
	const float margin = 1500.0f;
	qboolean out = qfalse;
	for (int axis = 0; axis < 3; axis++)
	{
		if (ahead[axis] < fighter_box_mins[axis] + margin || ahead[axis] > fighter_box_maxs[axis] - margin)
		{
			out = qtrue;
		}
	}
	if (!out)
	{
		return qfalse;
	}
	for (int axis = 0; axis < 3; axis++)
	{
		back[axis] = (fighter_box_mins[axis] + fighter_box_maxs[axis]) * 0.5f - my_pos[axis];
	}
	return qtrue;
}

// is there a ship of its own side between it and its target (don't shoot through friends)
static qboolean Fighter_FriendInLine(const gentity_t* ship, const vec3_t start, const vec3_t end, const int my_side)
{
	trace_t tr;
	trap->Trace(&tr, start, NULL, NULL, end, ship->s.number, MASK_SHOT, qfalse, 0, 0);
	if (tr.entityNum >= ENTITYNUM_WORLD)
	{
		return qfalse;
	}
	const gentity_t* hit = &g_entities[tr.entityNum];
	if (Fighter_IsFighter(hit))
	{
		return Fighter_ShipSide(hit) == my_side;
	}
	return hit->client && hit->client->sess.sessionTeam == my_side;
}

/*
===========================================================================
Ship routes

Points in open space a map's ships patrol along, kept in shiproutes/<map>.route (shiproutes/mp/siege_destroyer2.route,
the same files as singleplayer's). They are made in the game, flying (or noclipping) around the map, with bot_wp_edit 1
or cheats on:

	ship_wp_add		a point where you are (your ship, if you are in one)
	ship_wp_rem		removes the point nearest to you
	ship_wp_show	shows the points and their links (on/off)
	ship_wp_info	checks the route: points in something solid or not linked, parts not joined to the rest
	ship_wp_save	writes the file
	ship_wp_load	reads it again (undoes changes not saved)
	ship_wp_clear	removes all the points

A point is linked to the nearest ones a ship can fly straight to (nothing in between), so a route never goes through
the Star Destroyer or an asteroid. A ship without an enemy flies from point to point along the links, on to the one
most ahead of it now and then another; when it has fought, it goes back to the nearest point it can see. A map
without a route file: ships patrol around where they started.
===========================================================================
*/

#define ROUTE_MAX_POINTS 128
#define ROUTE_MAX_LINKS 4 // each point is linked to its nearest few it can see (and they to it)
#define ROUTE_LINK_DIST 22000.0f
#define ROUTE_LINK_HULL 64.0f // the room a ship needs between two points
#define ROUTE_REACHED 1500.0f // they fly too fast to turn tighter than this around a point
#define ROUTE_SHOW_DIST 20000.0f
#define ROUTE_SHOW_MAX_LINES 120 // (each line is an event sent to the players)

static vec3_t route_points[ROUTE_MAX_POINTS];
static qboolean route_links[ROUTE_MAX_POINTS][ROUTE_MAX_POINTS];
static qboolean route_links_dirty;
static int route_show_client = -1; // who it is shown to (around him)
static int route_show_time;
static int route_cur[MAX_GENTITIES]; // the point a ship flies to (it is its pos3 then)
static int route_prev[MAX_GENTITIES]; // the one it came from

static void Route_ResetShips(void)
{
	for (int i = 0; i < MAX_GENTITIES; i++)
	{
		route_cur[i] = route_prev[i] = -1;
	}
}

static const char* Route_FileName(void)
{
	char map_name[MAX_QPATH];
	trap->Cvar_VariableStringBuffer("mapname", map_name, sizeof map_name);
	return va("shiproutes/%s.route", map_name);
}

static void Route_Print(const int client_num, const char* text)
{
	if (client_num >= 0)
	{
		trap->SendServerCommand(client_num, va("print \"%s\"", text));
	}
	else
	{
		Com_Printf("%s", text);
	}
}

static qboolean Route_Clear(const vec3_t a, const vec3_t b)
{
	const vec3_t mins = { -ROUTE_LINK_HULL, -ROUTE_LINK_HULL, -ROUTE_LINK_HULL };
	const vec3_t maxs = { ROUTE_LINK_HULL, ROUTE_LINK_HULL, ROUTE_LINK_HULL };
	trace_t tr;
	trap->Trace(&tr, a, mins, maxs, b, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);
	return !tr.startsolid && !tr.allsolid && tr.fraction >= 1.0f;
}

// links each point to the nearest few it can see
static void Route_BuildLinks(void)
{
	if (!route_links_dirty)
	{
		return;
	}
	route_links_dirty = qfalse;
	memset(route_links, 0, sizeof route_links);

	for (int i = 0; i < route_num_points; i++)
	{
		int nearest[ROUTE_MAX_LINKS];
		float nearest_dist[ROUTE_MAX_LINKS];
		int count = 0;

		for (int j = 0; j < route_num_points; j++)
		{
			if (j == i)
			{
				continue;
			}
			const float dist = Distance(route_points[i], route_points[j]);
			if (dist > ROUTE_LINK_DIST)
			{
				continue;
			}
			// would it be one of the nearest (before the trace, the costly part)
			int slot = count;
			while (slot > 0 && nearest_dist[slot - 1] > dist)
			{
				slot--;
			}
			if (slot >= ROUTE_MAX_LINKS || !Route_Clear(route_points[i], route_points[j]))
			{
				continue;
			}
			if (count < ROUTE_MAX_LINKS)
			{
				count++;
			}
			for (int k = count - 1; k > slot; k--)
			{
				nearest[k] = nearest[k - 1];
				nearest_dist[k] = nearest_dist[k - 1];
			}
			nearest[slot] = j;
			nearest_dist[slot] = dist;
		}
		for (int k = 0; k < count; k++)
		{
			route_links[i][nearest[k]] = route_links[nearest[k]][i] = qtrue;
		}
	}
}

static void Route_Load(const int report_to)
{
	route_num_points = 0;
	route_links_dirty = qtrue;
	Route_ResetShips();

	fileHandle_t f;
	const int len = trap->FS_Open(Route_FileName(), &f, FS_READ);
	if (len <= 0 || !f)
	{
		if (f)
		{
			trap->FS_Close(f);
		}
		if (report_to != -2)
		{
			Route_Print(report_to, va(S_COLOR_YELLOW "No ship route file %s\n", Route_FileName()));
		}
		return;
	}
	char* buffer = (char*)BG_TempAlloc(len + 1);
	trap->FS_Read(buffer, len, f);
	trap->FS_Close(f);
	buffer[len] = 0;

	const char* line = buffer;
	while (line && *line && route_num_points < ROUTE_MAX_POINTS)
	{
		vec3_t point;
		if (line[0] != '/' && sscanf(line, "%f %f %f", &point[0], &point[1], &point[2]) == 3)
		{
			VectorCopy(point, route_points[route_num_points]);
			route_num_points++;
		}
		line = strchr(line, '\n');
		if (line)
		{
			line++;
		}
	}
	BG_TempFree(len + 1);

	if (report_to != -2 || Fighter_Developer())
	{
		Route_Print(report_to == -2 ? -1 : report_to,
			va("Ship route: %d points from %s\n", route_num_points, Route_FileName()));
	}
}

static void Route_Save(const int client_num)
{
	fileHandle_t f;
	trap->FS_Open(Route_FileName(), &f, FS_WRITE);
	if (!f)
	{
		Route_Print(client_num, va(S_COLOR_RED "Could not write %s\n", Route_FileName()));
		return;
	}
	char map_name[MAX_QPATH];
	trap->Cvar_VariableStringBuffer("mapname", map_name, sizeof map_name);
	const char* header = va("// ship route for %s: x y z of each point (ship_wp_add, ship_wp_save)\n", map_name);
	trap->FS_Write(header, strlen(header), f);
	for (int i = 0; i < route_num_points; i++)
	{
		const char* line = va("%.0f %.0f %.0f\n", route_points[i][0], route_points[i][1], route_points[i][2]);
		trap->FS_Write(line, strlen(line), f);
	}
	trap->FS_Close(f);
	Route_Print(client_num, va("Ship route: %d points saved to %s\n", route_num_points, Route_FileName()));
}

static int Route_Nearest(const vec3_t pos, const qboolean must_see)
{
	int best = -1;
	float best_dist = 1.0e30f;
	for (int i = 0; i < route_num_points; i++)
	{
		const float dist = DistanceSquared(pos, route_points[i]);
		if (dist < best_dist && (!must_see || Route_Clear(pos, route_points[i])))
		{
			best_dist = dist;
			best = i;
		}
	}
	return best;
}

// where a player is (his ship's centre, when he flies one)
static void Route_PlayerPos(const gentity_t* ent, vec3_t pos)
{
	if (ent->client && ent->client->ps.m_iVehicleNum)
	{
		VectorCopy(g_entities[ent->client->ps.m_iVehicleNum].r.currentOrigin, pos);
	}
	else
	{
		VectorCopy(ent->r.currentOrigin, pos);
	}
}

static void Route_Info(const int client_num)
{
	Route_BuildLinks();
	int links = 0;
	int problems = 0;
	for (int i = 0; i < route_num_points; i++)
	{
		int count = 0;
		for (int j = 0; j < route_num_points; j++)
		{
			count += route_links[i][j] ? 1 : 0;
		}
		links += count;
		if (!Route_Clear(route_points[i], route_points[i]))
		{
			Route_Print(client_num, va(S_COLOR_YELLOW "  point %d (%.0f %.0f %.0f) is in something solid\n", i,
				route_points[i][0], route_points[i][1], route_points[i][2]));
			problems++;
		}
		if (!count)
		{
			Route_Print(client_num, va(S_COLOR_YELLOW "  point %d (%.0f %.0f %.0f) is not linked to any\n", i,
				route_points[i][0], route_points[i][1], route_points[i][2]));
			problems++;
		}
	}
	// the parts of the route (one is right: every point can be reached from every other)
	int group[ROUTE_MAX_POINTS];
	int groups = 0;
	for (int i = 0; i < route_num_points; i++)
	{
		group[i] = -1;
	}
	for (int i = 0; i < route_num_points; i++)
	{
		if (group[i] >= 0)
		{
			continue;
		}
		int stack[ROUTE_MAX_POINTS];
		int top = 0;
		stack[top++] = i;
		group[i] = groups;
		while (top)
		{
			const int p = stack[--top];
			for (int j = 0; j < route_num_points; j++)
			{
				if (route_links[p][j] && group[j] < 0)
				{
					group[j] = groups;
					stack[top++] = j;
				}
			}
		}
		groups++;
	}
	if (groups > 1)
	{
		for (int g = 0; g < groups; g++)
		{
			int count = 0;
			int first = -1;
			for (int i = 0; i < route_num_points; i++)
			{
				if (group[i] == g)
				{
					count++;
					first = first < 0 ? i : first;
				}
			}
			Route_Print(client_num, va(S_COLOR_YELLOW "  part %d: %d points (point %d...)\n", g, count, first));
		}
	}
	Route_Print(client_num, va("Ship route: %d points, %d links, %d part%s, %d problem%s\n", route_num_points,
		links / 2, groups, groups == 1 ? "" : "s", problems, problems == 1 ? "" : "s"));
}

/*
-------------------------
G_FighterRoute_ClientCommand

The ship_wp_ commands (bot_wp_edit 1 or cheats on). qfalse: not one of them.
-------------------------
*/
qboolean G_FighterRoute_ClientCommand(gentity_t* ent, const char* cmd)
{
	if (Q_stricmpn(cmd, "ship_wp_", 8))
	{
		return qfalse;
	}
	const int client_num = ent - g_entities;
	trap->Cvar_Update(&bot_wp_edit);
	if (!bot_wp_edit.integer && !trap->Cvar_VariableIntegerValue("sv_cheats"))
	{
		Route_Print(client_num, "Ship routes are edited with bot_wp_edit 1 (or cheats on)\n");
		return qtrue;
	}
	vec3_t pos;
	Route_PlayerPos(ent, pos);

	if (!Q_stricmp(cmd, "ship_wp_add"))
	{
		if (route_num_points >= ROUTE_MAX_POINTS)
		{
			Route_Print(client_num, va(S_COLOR_RED "Ship route: no more than %d points\n", ROUTE_MAX_POINTS));
			return qtrue;
		}
		const int near_point = Route_Nearest(pos, qfalse);
		if (near_point >= 0 && Distance(pos, route_points[near_point]) < 1000.0f)
		{
			Route_Print(client_num, va(S_COLOR_YELLOW "Ship route: point %d is only %.0f away, added anyway\n",
				near_point, Distance(pos, route_points[near_point])));
		}
		if (!Route_Clear(pos, pos))
		{
			Route_Print(client_num,
				S_COLOR_YELLOW "Ship route: this point is in (or too near) something solid, added anyway\n");
		}
		VectorCopy(pos, route_points[route_num_points]);
		route_num_points++;
		route_links_dirty = qtrue;
		route_show_client = client_num;
		route_show_time = 0;
		Route_Print(client_num, va("Ship route: point %d added at (%.0f %.0f %.0f)\n", route_num_points - 1, pos[0],
			pos[1], pos[2]));
	}
	else if (!Q_stricmp(cmd, "ship_wp_rem"))
	{
		const int point = Route_Nearest(pos, qfalse);
		if (point < 0)
		{
			Route_Print(client_num, "Ship route: there are no points\n");
			return qtrue;
		}
		Route_Print(client_num, va("Ship route: point %d removed, %.0f away\n", point, Distance(pos, route_points[point])));
		for (int i = point; i < route_num_points - 1; i++)
		{
			VectorCopy(route_points[i + 1], route_points[i]);
		}
		route_num_points--;
		route_links_dirty = qtrue;
		Route_ResetShips();
		route_show_time = 0;
	}
	else if (!Q_stricmp(cmd, "ship_wp_show"))
	{
		route_show_client = route_show_client == client_num ? -1 : client_num;
		route_show_time = 0;
		Route_Print(client_num, va("Ship route: %d points, %s\n", route_num_points,
			route_show_client == client_num ? "shown" : "hidden"));
	}
	else if (!Q_stricmp(cmd, "ship_wp_info"))
	{
		Route_Info(client_num);
	}
	else if (!Q_stricmp(cmd, "ship_wp_save"))
	{
		Route_Save(client_num);
	}
	else if (!Q_stricmp(cmd, "ship_wp_load"))
	{
		Route_Load(client_num);
		route_show_time = 0;
	}
	else if (!Q_stricmp(cmd, "ship_wp_clear"))
	{
		route_num_points = 0;
		route_links_dirty = qtrue;
		Route_ResetShips();
		Route_Print(client_num, "Ship route: all points removed (ship_wp_load gets the saved ones back)\n");
	}
	else
	{
		Route_Print(client_num,
			"ship_wp_add, ship_wp_rem, ship_wp_show, ship_wp_info, ship_wp_save, ship_wp_load, ship_wp_clear\n");
	}
	return qtrue;
}

// map start: the map's route, nothing left over from the last map
void G_FighterRoute_Load(void)
{
	memset(fighter_board, 0, sizeof fighter_board);
	memset(fighter_reserved, 0, sizeof fighter_reserved);
	memset(fighter_reserver, 0, sizeof fighter_reserver);
	memset(fighter_bot_flying, 0, sizeof fighter_bot_flying);
	memset(fighter_side, 0, sizeof fighter_side);
	memset(fighter_flown, 0, sizeof fighter_flown);
	memset(fighter_out_since, 0, sizeof fighter_out_since);
	memset(fighter_jump_wait, 0, sizeof fighter_jump_wait);
	fighter_box_set = qfalse; // (worked out when first needed, once the map's triggers are linked)
	fighter_crew_time = 0;
	route_show_client = -1;
	Route_Load(-2);
	Walk_Load(); // the hangars' walk to the ships (shiproutes/<map>.walk)
}

// draws the route while it is shown, around the one who asked: the points as white posts, the links as red lines
void G_FighterRoute_Frame(void)
{
	G_FighterAI_AutoSpawn(); // the hangars' ships on the maps where only a button makes them
	G_FighterAI_Frame();

	if (route_show_client < 0 || route_show_time > level.time)
	{
		return;
	}
	const gentity_t* viewer = &g_entities[route_show_client];
	if (!viewer->inuse || !viewer->client || viewer->client->pers.connected != CON_CONNECTED)
	{
		route_show_client = -1;
		return;
	}
	const int redraw = 1000;
	route_show_time = level.time + redraw;
	Route_BuildLinks();

	vec3_t pos;
	Route_PlayerPos(viewer, pos);
	int lines = 0;
	for (int i = 0; i < route_num_points && lines < ROUTE_SHOW_MAX_LINES; i++)
	{
		if (Distance(pos, route_points[i]) > ROUTE_SHOW_DIST)
		{
			continue;
		}
		vec3_t bottom, top;
		VectorCopy(route_points[i], bottom);
		VectorCopy(route_points[i], top);
		bottom[2] -= 400.0f;
		top[2] += 400.0f;
		G_TestLine(bottom, top, 0, redraw + 100);
		lines++;

		for (int j = i + 1; j < route_num_points && lines < ROUTE_SHOW_MAX_LINES; j++)
		{
			if (route_links[i][j])
			{
				G_TestLine(route_points[i], route_points[j], 0x0000ff, redraw + 100);
				lines++;
			}
		}
	}
}

// a ship without an enemy on a route: pos3 becomes the point it flies to. qfalse: no route here.
static qboolean Fighter_RouteGoal(const vec3_t my_pos, const vec3_t fwd)
{
	if (route_num_points <= 0)
	{
		return qfalse;
	}
	Route_BuildLinks();

	const int n = NPCS.NPC->s.number;
	int cur = route_cur[n];
	if (cur < 0 || cur >= route_num_points || !VectorCompare(NPCS.NPC->pos3, route_points[cur]))
	{
		// (back) onto the route: the nearest point it can see
		cur = Route_Nearest(my_pos, qtrue);
		if (cur < 0)
		{
			cur = Route_Nearest(my_pos, qfalse);
		}
		route_prev[n] = -1;
	}
	else
	{
		vec3_t to_point;
		VectorSubtract(route_points[cur], my_pos, to_point);
		const float dist = VectorNormalize(to_point);
		if (dist < ROUTE_REACHED || dist < ROUTE_REACHED * 2.5f && DotProduct(to_point, fwd) < 0.0f)
		{
			// there (or flown past it): on to a linked one, liking the ones straight on, not back unless it must
			vec3_t heading;
			if (route_prev[n] >= 0 && route_prev[n] < route_num_points)
			{
				VectorSubtract(route_points[cur], route_points[route_prev[n]], heading);
				VectorNormalize(heading);
			}
			else
			{
				VectorCopy(fwd, heading);
			}
			int next = -1;
			float best_score = -1.0e30f;
			for (int j = 0; j < route_num_points; j++)
			{
				if (!route_links[cur][j])
				{
					continue;
				}
				vec3_t dir;
				VectorSubtract(route_points[j], route_points[cur], dir);
				VectorNormalize(dir);
				float score = DotProduct(dir, heading) + Q_flrand(0.0f, 1.2f);
				if (j == route_prev[n])
				{
					score -= 10.0f; // a dead end only
				}
				if (score > best_score)
				{
					best_score = score;
					next = j;
				}
			}
			if (next >= 0)
			{
				if (Fighter_Developer() > 1)
				{
					Com_Printf("fighter AI: %s %d at route point %d, on to %d\n", NPCS.NPC->NPC_type ? NPCS.NPC->NPC_type : "bot", n, cur,
						next);
				}
				route_prev[n] = cur;
				cur = next;
			}
		}
	}
	route_cur[n] = cur;
	VectorCopy(route_points[cur], NPCS.NPC->pos3);
	return qtrue;
}

/*
-------------------------
NPC_FighterAI

An NPC flying a fighter: fills in its usercmd. qfalse: not flying one (normal behaviour).
-------------------------
*/
static qboolean Fighter_Fly(gentity_t* npc, Vehicle_t* p_veh);

// a ship still lifting off a floor: the vehicle code (ProcessMoveCommands) only lets it rise, with no speed forward,
// until it is FIGHTER_MIN_TAKEOFF_FRACTION (0.7) of its landingHeight up, as a player holds the jump key until then.
// (The ship's groundEntityNum is no use: an MP ship standing on a floor has none, so its pilot never took off.) Not
// with a roof just above it.
static qboolean Fighter_TakingOff(const gentity_t* ship, const Vehicle_t* p_veh)
{
	if (p_veh->m_LandTrace.fraction >= 1.0f || p_veh->m_LandTrace.fraction > 0.7f || ship->client->ps.speed > 200)
	{
		return qfalse;
	}
	trace_t tr;
	vec3_t up;
	VectorCopy(ship->r.currentOrigin, up);
	up[2] += ship->r.maxs[2] + 48.0f;
	trap->Trace(&tr, ship->r.currentOrigin, NULL, NULL, up, ship->s.number, MASK_SOLID, qfalse, 0, 0);
	return tr.fraction >= 1.0f ? qtrue : qfalse;
}

/*
-------------------------
Fighter_HangarExit

A ship that has just taken off on a hangar map (deathstar_trench: the TIEs' hangars open sideways into the trench, just
across from its far wall) leaves the hangar the way it stood while there is a roof over it, then climbs out of the
trench straight ahead until the way on is clear and it is well above where it took off. Only then does it look for a
fight or follow the route (its pilot would turn after an enemy into a wall). qtrue while it is leaving (the moves
are set). A ship in a hangar that is a hyperspace tunnel (the Rebels') flies on into the jump. (As the singleplayer
AI_Fighter.cpp.)
-------------------------
*/
static qboolean Fighter_HangarExit(gentity_t* npc, const gentity_t* ship, const Vehicle_t* p_veh, const vec3_t my_pos,
	const float max_step)
{
	const int n = npc->s.number;
	if (!fighter_exit_until[n])
	{
		return qfalse;
	}
	if (level.time > fighter_exit_until[n])
	{
		fighter_exit_until[n] = 0;
		if (Fighter_Developer())
		{
			Com_Printf("fighter AI: %s %d gave up leaving its hangar at %s\n", ship->NPC_type, ship->s.number,
				vtos(my_pos));
		}
		return qfalse;
	}
	vec3_t out_dir;
	AngleVectors(fighter_launch_angles[n], out_dir, NULL, NULL);
	out_dir[2] = 0.0f;
	VectorNormalize(out_dir);

	trace_t tr;
	vec3_t end;
	VectorCopy(my_pos, end);
	end[2] += 1500.0f;
	trap->Trace(&tr, my_pos, NULL, NULL, end, ship->s.number, MASK_SOLID, qfalse, 0, 0);
	vec3_t want;
	VectorCopy(out_dir, want);
	float step = max_step;
	int cruise; // the speed it leaves at: slow enough to climb out of the trench before its far wall
	if (tr.fraction < 1.0f)
	{
		// under the hangar's roof: straight on out
		cruise = FIGHTER_EXIT_SPEED_HANGAR;
	}
	else
	{
		// out of it: climb on ahead, steeply, until nothing is in the way
		VectorMA(my_pos, 6000.0f, out_dir, end);
		trap->Trace(&tr, my_pos, ship->r.mins, ship->r.maxs, end, ship->s.number, MASK_SOLID, qfalse, 0, 0);
		if (!tr.startsolid && !tr.allsolid && tr.fraction >= 1.0f && my_pos[2] > fighter_exit_z[n] + 1000.0f)
		{
			fighter_exit_until[n] = 0;
			if (Fighter_Developer())
			{
				Com_Printf("fighter AI: %s %d is out of its hangar at %s\n", ship->NPC_type, ship->s.number,
					vtos(my_pos));
			}
			return qfalse;
		}
		want[2] = 2.5f; // about 68 degrees up (FIGHTER_MAX_PITCH is 70)
		step = max_step * 1.5f;
		cruise = FIGHTER_EXIT_SPEED_CLIMB;
	}
	// the throttle: on to its speed, back off above it (not so slow it would land)
	const int speed_now = (int)(ship->client->ps.speed);
	if (speed_now > cruise + 150 && speed_now > 400)
	{
		NPCS.ucmd.forwardmove = -127;
	}
	else
	{
		NPCS.ucmd.forwardmove = speed_now < cruise ? 127 : 0;
	}
	if (Fighter_TakingOff(ship, p_veh))
	{
		NPCS.ucmd.forwardmove = 127;
		NPCS.ucmd.upmove = 127; // still lifting off the floor
	}
	Fighter_TurnTowards(want, step);
	VectorCopy(my_pos, npc->pos4); // home is where it is out of the hangar
	VectorCopy(my_pos, npc->pos3);
	return qtrue;
}

qboolean NPC_FighterAI(void)
{
	gentity_t* npc = NPCS.NPC;
	if (!Fighter_IsAIPilot(npc))
	{
		return qfalse; // only the pilots G_FighterAI_Frame made (scripted ones are left alone)
	}
	memset(&NPCS.ucmd, 0, sizeof NPCS.ucmd);

	Vehicle_t* p_veh = G_IsRidingVehicle(npc);
	if (p_veh && (gentity_t*)p_veh->m_pPilot != npc)
	{
		p_veh = NULL; // thrown out of it (his own number is left behind)
	}
	if (!p_veh && fighter_board[npc->s.number])
	{
		if (Fighter_Walk(npc))
		{
			return qtrue; // still walking to his ship (his own AI does nothing)
		}
		memset(&NPCS.ucmd, 0, sizeof NPCS.ucmd);
		Fighter_Board(npc); // just made (or next to it): into his ship
		p_veh = G_IsRidingVehicle(npc);
	}
	if (!p_veh || !p_veh->m_pVehicleInfo || p_veh->m_pVehicleInfo->type != VH_FIGHTER)
	{
		if (npc->health > 0)
		{
			// a pilot without his ship (thrown out of it, or it was gone before he got in): he goes
			if (Fighter_Developer())
			{
				Com_Printf("fighter AI: %s %d has no ship (vehicle %d / %d), leaves\n", npc->NPC_type, npc->s.number,
					npc->s.m_iVehicleNum, npc->client->ps.m_iVehicleNum);
			}
			npc->s.eFlags |= EF_NODRAW;
			npc->think = G_FreeEntity;
			npc->nextthink = level.time + FRAMETIME;
			return qtrue;
		}
		return qfalse;
	}
	return Fighter_Fly(npc, p_veh);
}

// flies the ship: the pilot (an AI pilot, or a bot: G_FighterAI_BotCmd) is NPCS.NPC, his moves go in NPCS.ucmd
static qboolean Fighter_Fly(gentity_t* npc, Vehicle_t* p_veh)
{
	gentity_t* ship = (gentity_t*)p_veh->m_pParentEntity;
	if (ship->health <= 0)
	{
		return qtrue;
	}

	const int skill = Fighter_Skill();
	const float turn_rate = 60.0f + 20.0f * (float)skill; // degrees per second
	const float max_step = turn_rate * FIGHTER_THINK_SECONDS;
	const float aim_error = 3.0f - 0.8f * (float)skill; // degrees
	// his side: his team's, or (no teams: a bot in a free for all) the side of the hangar the ship came from
	const int my_side = npc->client->sess.sessionTeam == TEAM_RED || npc->client->sess.sessionTeam == TEAM_BLUE
		? npc->client->sess.sessionTeam
		: ship->s.teamowner;

	vec3_t fwd, my_pos;
	AngleVectors(p_veh->m_vOrientation, fwd, NULL, NULL);
	VectorCopy(ship->r.currentOrigin, my_pos);
	const float speed = VectorLength(ship->client->ps.velocity);

	// going into hyperspace (a trigger_hyperspace: the Rebels' hangar on siege_destroyer2 is far off, in one): the ship
	// follows its pilot's view, roll and all, until it faces the jump. For a player his pmove turns the view there
	// (PM_VehFaceHyperspacePoint), an NPC rider's does not get there: the pilot turns it himself, as fast (90 degrees a
	// second), and one still not there after a while is put there. Turbo as that does for a player (a ship standing
	// still does not turn).
	if (ship->client->ps.hyperSpaceTime&& level.time - ship->client->ps.hyperSpaceTime < HYPERSPACE_TIME)
	{
		NPCS.ucmd.upmove = 127;
		if (!fighter_jump_wait[npc->s.number])
		{
			fighter_jump_wait[npc->s.number] = level.time;
		}
		const qboolean snap = level.time - fighter_jump_wait[npc->s.number] > 5000;
		const float step = 90.0f * FIGHTER_THINK_SECONDS;
		vec3_t view;
		for (int axis = PITCH; axis <= ROLL; axis++)
		{
			const float want = ship->client->ps.hyperSpaceAngles[axis];
			const float delta = AngleSubtract(want, NPCS.client->ps.viewangles[axis]);
			view[axis] = snap || fabs(delta) <= step ? want
				: NPCS.client->ps.viewangles[axis] + (delta > 0.0f ? step : -step);
			view[axis] = axis == YAW ? AngleNormalize360(view[axis]) : AngleNormalize180(view[axis]);
		}
		SetClientViewAngle(npc, view);
		for (int axis = PITCH; axis <= ROLL; axis++)
		{
			NPCS.ucmd.angles[axis] = ANGLE2SHORT(view[axis]) - NPCS.client->ps.delta_angles[axis];
		}
		// (between thinks NPC_UpdateAngles turns the view to these: the jump's, not back to where it was)
		NPCS.NPCInfo->desiredYaw = AngleNormalize360(ship->client->ps.hyperSpaceAngles[YAW]);
		NPCS.NPCInfo->desiredPitch = AngleNormalize180(ship->client->ps.hyperSpaceAngles[PITCH]);

		// facing the jump: ready to go (for a player his pmove says so, PM_VehFaceHyperspacePoint; an NPC rider's
		// does not get there). Until then the jump's clock waits, as there: the ship has to fly its full time through
		// the trigger before it is moved, or it comes out far off the map.
		if (!(ship->client->ps.eFlags2 & EF2_HYPERSPACE)
			&& (float)(level.time - ship->client->ps.hyperSpaceTime) / HYPERSPACE_TIME < HYPERSPACE_TELEPORT_FRAC)
		{
			qboolean facing = qtrue;
			for (int axis = PITCH; axis <= ROLL; axis++)
			{
				if (fabs(AngleSubtract(ship->client->ps.hyperSpaceAngles[axis], p_veh->m_vOrientation[axis])) > 2.0f)
				{
					facing = qfalse;
				}
			}
			ship->client->ps.hyperSpaceTime = level.time;
			if (facing)
			{
				ship->client->ps.eFlags2 |= EF2_HYPERSPACE;
			}
		}
		return qtrue;
	}
	fighter_jump_wait[npc->s.number] = 0;

	// out of the hangar first: straight on the way it stood, full throttle (taking off if it stands on the floor), until
	// it is well clear of where it got in (a ship taking off from a floor is slow to get going) or long enough
	if (!TIMER_Done(npc, "fighterLaunch")
		|| !TIMER_Done(npc, "fighterLaunchMax") && Distance(my_pos, fighter_launch_pos[npc->s.number]) < 2000.0f)
	{
		if (p_veh->m_iDropTime < level.time) // not still dropping clear of its rack
		{
			NPCS.ucmd.forwardmove = 127;
			if (Fighter_TakingOff(ship, p_veh))
			{
				NPCS.ucmd.upmove = 127; // lifting off the hangar floor (in the air upmove is turbo)
			}
		}
		vec3_t out_dir;
		AngleVectors(fighter_launch_angles[npc->s.number], out_dir, NULL, NULL);
		Fighter_TurnTowards(out_dir, max_step);
		VectorCopy(my_pos, npc->pos4); // home is where it is out of the hangar, not in it
		VectorCopy(my_pos, npc->pos3);
		return qtrue;
	}
	// a hangar map's ship: out of the hangar and the trench before anything else
	if (Fighter_HangarExit(npc, ship, p_veh, my_pos, max_step))
	{
		return qtrue;
	}

	// a map edge (trigger_shipboundary) turns the ship around, to the point it names: for a player his pmove does it
	// (PM_VehForcedTurning), an NPC rider's does not get there, so the pilot turns to it, on turbo as that does
	if (ship->client->ps.vehTurnaroundTime > level.time)
	{
		const gentity_t* dst = &g_entities[ship->client->ps.vehTurnaroundIndex];
		if (dst->inuse)
		{
			vec3_t back;
			VectorSubtract(dst->s.origin, my_pos, back);
			if (VectorNormalize(back) > 0.0f)
			{
				Fighter_TurnTowards(back, max_step * 2.0f);
			}
		}
		NPCS.ucmd.upmove = 127;
		return qtrue;
	}

	// the enemy
	if (npc->enemy && (!Fighter_IsFighter(npc->enemy) || npc->enemy->health <= 0
		|| Fighter_ShipSide(npc->enemy) == TEAM_FREE || Fighter_ShipSide(npc->enemy) == my_side))
	{
		npc->enemy = NULL;
	}
	if (TIMER_Done(npc, "fighterRetarget"))
	{
		gentity_t* target = Fighter_FindTarget(ship, my_side, fwd);
		if (target != npc->enemy)
		{
			npc->enemy = target;
			TIMER_Set(npc, "fighterReact", 900 - 200 * skill); // a moment to react before shooting
		}
		TIMER_Set(npc, "fighterRetarget", Q_irand(800, 1500));
	}
	gentity_t* enemy = npc->enemy;

	vec3_t want_dir;
	VectorCopy(fwd, want_dir);
	qboolean may_fire = qfalse;
	vec3_t aim_point;
	float enemy_dist = 0.0f;

	if (!TIMER_Done(npc, "fighterEvade"))
	{
		// breaking away
		VectorSubtract(npc->pos3, my_pos, want_dir);
		NPCS.ucmd.forwardmove = 127;
	}
	else if (enemy)
	{
		// lead it: where it will be when a shot gets there
		const int weap = p_veh->m_pVehicleInfo->weapon[0].ID;
		const float shot_speed = weap > VEH_WEAPON_NONE && g_vehWeaponInfo[weap].fSpeed > 0.0f
			? g_vehWeaponInfo[weap].fSpeed
			: 6000.0f;
		enemy_dist = Distance(my_pos, enemy->r.currentOrigin);
		VectorMA(enemy->r.currentOrigin, enemy_dist / shot_speed, enemy->client->ps.velocity, aim_point);
		VectorSubtract(aim_point, my_pos, want_dir);

		// a little off, more so for a poorer pilot
		vec3_t want_angles;
		vectoangles(want_dir, want_angles);
		want_angles[PITCH] += Q_flrand(-aim_error, aim_error);
		want_angles[YAW] += Q_flrand(-aim_error, aim_error);
		const float len = VectorLength(want_dir);
		AngleVectors(want_angles, want_dir, NULL, NULL);
		VectorScale(want_dir, len, want_dir);

		vec3_t to_enemy;
		VectorSubtract(enemy->r.currentOrigin, my_pos, to_enemy);
		VectorNormalize(to_enemy);
		const float facing = DotProduct(fwd, to_enemy);

		if (enemy_dist < FIGHTER_BREAK_DIST && facing > 0.0f)
		{
			// about to fly into it: break away, up or down and to a side
			vec3_t right, up;
			AngleVectors(p_veh->m_vOrientation, NULL, right, up);
			VectorMA(my_pos, 2500.0f, fwd, npc->pos3);
			VectorMA(npc->pos3, Q_irand(0, 1) ? 2000.0f : -2000.0f, right, npc->pos3);
			VectorMA(npc->pos3, Q_irand(0, 1) ? 1500.0f : -1500.0f, up, npc->pos3);
			TIMER_Set(npc, "fighterEvade", Q_irand(1500, 2500));
		}

		// throttle: catch up, don't overshoot
		vec3_t rel_vel;
		VectorSubtract(ship->client->ps.velocity, enemy->client->ps.velocity, rel_vel);
		const float closing = DotProduct(rel_vel, to_enemy);
		NPCS.ucmd.forwardmove = enemy_dist < 1500.0f && closing > 400.0f && facing > 0.7f ? -127 : 127;
		if (enemy_dist > 6000.0f && facing > 0.85f && TIMER_Done(npc, "fighterTurbo"))
		{
			NPCS.ucmd.upmove = 127; // turbo
			TIMER_Set(npc, "fighterTurbo", Q_irand(6000, 12000));
		}
		may_fire = TIMER_Done(npc, "fighterReact");
	}
	else
	{
		const gentity_t* leader = Fighter_LeaderShip(ship, my_side);
		if (leader)
		{
			// a wingman flies with a player of its side: behind him and to a side (one side each by number)
			vec3_t l_fwd, l_right;
			AngleVectors(leader->m_pVehicle->m_vOrientation, l_fwd, l_right, NULL);
			const float side = npc->s.number % 2 ? 500.0f : -500.0f;
			VectorMA(leader->r.currentOrigin, -400.0f - 150.0f * (float)(npc->s.number % 3), l_fwd, npc->pos3);
			VectorMA(npc->pos3, side, l_right, npc->pos3);
			VectorSubtract(npc->pos3, my_pos, want_dir);
			NPCS.ucmd.forwardmove = VectorLength(want_dir) > 700.0f ? 127 : 0;
		}
		else
		{
			// along the map's route, or (none) patrol around where it started
			if (!Fighter_RouteGoal(my_pos, fwd)
				&& (TIMER_Done(npc, "fighterPatrol") || Distance(my_pos, npc->pos3) < 800.0f))
			{
				// a point around home it can fly straight to (not inside the Star Destroyer or an asteroid)
				for (int tries = 0; tries < 6; tries++)
				{
					vec3_t point;
					for (int axis = 0; axis < 3; axis++)
					{
						point[axis] = npc->pos4[axis] + Q_flrand(-5000.0f, 5000.0f);
					}
					trace_t tr;
					trap->Trace(&tr, my_pos, ship->r.mins, ship->r.maxs, point, ship->s.number, MASK_SOLID, qfalse, 0, 0);
					if (!tr.startsolid && !tr.allsolid && tr.fraction >= 1.0f)
					{
						VectorCopy(point, npc->pos3);
						break;
					}
				}
				TIMER_Set(npc, "fighterPatrol", Q_irand(8000, 15000));
			}
			VectorSubtract(npc->pos3, my_pos, want_dir);
			NPCS.ucmd.forwardmove = 100;
		}
	}

	// something in the way ahead (the Star Destroyer, an asteroid, a wall): turn away from it, harder and braking when
	// it is near. It looks about two seconds ahead (they fly at thousands of units a second), with a box near the
	// ship's own size.
	float turn_step = max_step;
	{
		const float half = Com_Clamp(24.0f, 96.0f, ship->r.maxs[0] * 0.6f);
		const vec3_t look_mins = { -half, -half, -half };
		const vec3_t look_maxs = { half, half, half };
		const float look = Com_Clamp(1200.0f, 9000.0f, speed * 2.0f);
		vec3_t end;
		trace_t tr;
		VectorMA(my_pos, look, fwd, end);
		trap->Trace(&tr, my_pos, look_mins, look_maxs, end, ship->s.number, MASK_SOLID | CONTENTS_BODY, qfalse, 0,
			0); // other ships too
		vec3_t back;
		if (!tr.startsolid && tr.fraction < 1.0f && (!enemy || tr.entityNum != enemy->s.number))
		{
			const float dist_ahead = tr.fraction * look;
			vec3_t away;
			VectorScale(fwd, 0.2f, away);
			VectorMA(away, 1.0f, tr.plane.normal, away);
			VectorCopy(away, want_dir);
			may_fire = qfalse;
			NPCS.ucmd.upmove = 0; // no turbo into it
			if (dist_ahead < speed * 1.0f || dist_ahead < 900.0f)
			{
				NPCS.ucmd.forwardmove = -127;
				turn_step = max_step * 2.0f; // pull hard
			}
		}
		else if (Fighter_EdgeAhead(my_pos, end, back))
		{
			// the map's edge ahead: back towards the middle, hard
			VectorCopy(back, want_dir);
			NPCS.ucmd.upmove = 0;
			turn_step = max_step * 2.0f;
		}
	}

	if (VectorNormalize(want_dir) > 0.0f)
	{
		Fighter_TurnTowards(want_dir, turn_step);
	}

	// shoot when lined up with the aim point
	if (may_fire && enemy && enemy_dist < FIGHTER_FIRE_RANGE)
	{
		vec3_t aim_dir;
		VectorSubtract(aim_point, my_pos, aim_dir);
		VectorNormalize(aim_dir);
		const float cos_cone = cos(DEG2RAD(FIGHTER_FIRE_CONE));
		if (DotProduct(fwd, aim_dir) > cos_cone)
		{
			vec3_t start;
			VectorMA(my_pos, 150.0f, fwd, start);
			if (!Fighter_FriendInLine(ship, start, enemy->r.currentOrigin, my_side))
			{
				NPCS.ucmd.buttons |= BUTTON_ATTACK;
				// now and then its other weapon (missiles, torpedoes) from a fair distance
				if (p_veh->m_pVehicleInfo->weapon[1].ID > VEH_WEAPON_NONE && p_veh->weaponStatus[1].ammo > 0
					&& enemy_dist > 1200.0f && enemy_dist < 6000.0f && TIMER_Done(npc, "fighterAlt"))
				{
					NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
					TIMER_Set(npc, "fighterAlt", Q_irand(6000, 12000));
				}
			}
		}
	}

	return qtrue;
}

/*
===========================================================================
Bots in the ships (ai_main.c bot_fighter_ai)

On a map with a walk file (deathstar_trench_v1 / v2) a bot on foot goes for a free ship standing in a hangar (his
team's, or with no teams the nearest one), walks to it along the hangar's walk points, gets in, and from then on the
fighter AI flies it for him (G_FighterAI_BotCmd: his own AI does nothing) until it is destroyed. A ship he goes for
is kept for him while he walks to it.
===========================================================================
*/

// the map has the ships' walk (and the space battle is on)
qboolean G_FighterAI_BotFighterMap(void)
{
	return walk_num_points > 0 && g_spaceBattle.integer ? qtrue : qfalse;
}

// a ship standing in a hangar that this bot may take: empty, not kept for somebody else, not a transport
static qboolean Fighter_BotShipFree(const gentity_t* ship, const int bot_num)
{
	return Fighter_IsFighter(ship) && ship->health > 0 && !ship->m_pVehicle->m_pPilot && Fighter_IsParkedShip(ship)
		&& (fighter_reserved[ship->s.number] < level.time || fighter_reserver[ship->s.number] == bot_num + 1)
		&& !Q_stristr(ship->NPC_type, "shuttle") && !Q_stristr(ship->NPC_type, "yt-1300") ? qtrue : qfalse;
}

// the ship the bot goes for: the one he has if it is still free, else the nearest free one of his team (any with no
// teams); it is kept for him. NULL: none
gentity_t* G_FighterAI_BotShip(const gentity_t* bot, gentity_t* current)
{
	const int n = bot->s.number;
	const int team = bot->client->sess.sessionTeam;
	gentity_t* ship = current && Fighter_BotShipFree(current, n) ? current : NULL;
	if (!ship)
	{
		float best = 1.0e30f;
		for (int i = MAX_CLIENTS; i < level.num_entities; i++)
		{
			gentity_t* cand = &g_entities[i];
			if (!Fighter_BotShipFree(cand, n)
				|| (team == TEAM_RED || team == TEAM_BLUE) && cand->s.teamowner != team)
			{
				continue;
			}
			const float dist = DistanceSquared(cand->r.currentOrigin, bot->r.currentOrigin);
			if (dist < best)
			{
				best = dist;
				ship = cand;
			}
		}
	}
	if (ship)
	{
		fighter_reserved[ship->s.number] = level.time + 3000;
		fighter_reserver[ship->s.number] = n + 1;
	}
	return ship;
}

// where a bot walking to his ship heads now (Walk_NextGoal)
void G_FighterAI_WalkGoal(const vec3_t pos, const gentity_t* ship, vec3_t goal)
{
	Walk_NextGoal(pos, ship->r.currentOrigin, goal);
}

// close enough to his ship to get in
qboolean G_FighterAI_BotAtShip(const gentity_t* bot, const gentity_t* ship)
{
	const float reach = (ship->r.maxs[0] > ship->r.maxs[1] ? ship->r.maxs[0] : ship->r.maxs[1]) + WALK_BOARD_DIST;
	return DistanceHorizontal(bot->r.currentOrigin, ship->r.currentOrigin) < reach ? qtrue : qfalse;
}

// the bot gets into the ship (at once) and the fighter AI flies it from now on. qfalse: he couldn't
qboolean G_FighterAI_BotBoard(gentity_t* bot, gentity_t* ship)
{
	if (!Fighter_BotShipFree(ship, bot->s.number))
	{
		return qfalse;
	}
	Vehicle_t* p_veh = ship->m_pVehicle;
	if (!p_veh->m_pVehicleInfo->Board(p_veh, (bgEntity_t*)bot))
	{
		return qfalse;
	}
	p_veh->m_iBoarding = 0; // in it at once, no boarding time
	fighter_reserved[ship->s.number] = 0;
	fighter_reserver[ship->s.number] = 0;

	// a ship docked in its rack (SUSPENDED) is let go: it drops clear of the rack first ("dropTime"), then flies out
	// (on a hangar map out of the hangar and the trench by Fighter_HangarExit)
	const qboolean hangar_map = Fighter_AutoSpawnMap();
	int launch_time = hangar_map ? 1000 : 2500;
	if (ship->spawnflags & 2)
	{
		ship->spawnflags &= ~2;
		G_Sound(ship, CHAN_AUTO, G_SoundIndex("sound/vehicles/common/release.wav"));
		if (ship->fly_sound_debounce_time)
		{
			p_veh->m_iDropTime = level.time + ship->fly_sound_debounce_time;
			launch_time += ship->fly_sound_debounce_time;
		}
	}
	const int n = bot->s.number;
	VectorCopy(ship->r.currentOrigin, bot->pos4); // home, the centre of its patrol
	VectorCopy(ship->r.currentOrigin, bot->pos3);
	TIMER_Set(bot, "fighterLaunch", launch_time); // drops clear and flies straight out of its hangar first
	TIMER_Set(bot, "fighterLaunchMax", p_veh->m_iDropTime > level.time || hangar_map ? 0 : launch_time + 5500);
	fighter_exit_until[n] = hangar_map ? level.time + launch_time + FIGHTER_EXIT_TIME : 0;
	fighter_exit_z[n] = ship->r.currentOrigin[2];
	TIMER_Set(bot, "fighterRetarget", 0);
	TIMER_Set(bot, "fighterEvade", 0);
	VectorCopy(ship->r.currentOrigin, fighter_launch_pos[n]);
	VectorSet(fighter_launch_angles[n], 0.0f, p_veh->m_vOrientation[YAW], 0.0f);
	fighter_jump_wait[n] = 0;
	fighter_bot_flying[n] = qtrue;
	fighter_bot_next[n] = 0;
	bot->enemy = NULL;
	if (Fighter_Developer())
	{
		Com_Printf("fighter AI: bot %s flies %s %d\n", bot->client->pers.netname, ship->NPC_type, ship->s.number);
	}
	return qtrue;
}

// the bot is the pilot of a ship the fighter AI flies for him
qboolean G_FighterAI_BotFlying(const gentity_t* bot)
{
	const int n = bot->s.number;
	if (!fighter_bot_flying[n])
	{
		return qfalse;
	}
	const gentity_t* ship = bot->client && bot->client->ps.m_iVehicleNum ? &g_entities[bot->client->ps.m_iVehicleNum] : NULL;
	if (!ship || bot->health <= 0 || !Fighter_IsFighter(ship) || (const gentity_t*)ship->m_pVehicle->m_pPilot != bot)
	{
		fighter_bot_flying[n] = qfalse; // out of it (it was destroyed, or he died): his own AI again
		return qfalse;
	}
	return qtrue;
}

// a bot the fighter AI flies: his moves this frame are the ship's (worked out every FIGHTER_THINK_SECONDS, as an AI
// pilot's think, and kept in between), in place of his own AI's
void G_FighterAI_BotCmd(gentity_t* bot, usercmd_t* cmd)
{
	if (!G_FighterAI_BotFlying(bot))
	{
		return;
	}
	const int n = bot->s.number;
	if (fighter_bot_next[n] <= level.time)
	{
		static gNPC_t bot_info; // (the fighter AI only keeps the pilot's desired view in it)
		const npcStatic_t saved = NPCS;
		gentity_t* ship = &g_entities[bot->client->ps.m_iVehicleNum];

		memset(&bot_info, 0, sizeof bot_info);
		NPCS.NPC = bot;
		NPCS.client = bot->client;
		NPCS.NPCInfo = &bot_info;
		memset(&NPCS.ucmd, 0, sizeof NPCS.ucmd);
		for (int axis = 0; axis < 3; axis++)
		{
			NPCS.ucmd.angles[axis] = cmd->angles[axis]; // (kept where the fighter AI leaves them)
		}
		Fighter_Fly(bot, ship->m_pVehicle);
		fighter_bot_cmd[n] = NPCS.ucmd;
		NPCS = saved;
		fighter_bot_next[n] = level.time + (int)(FIGHTER_THINK_SECONDS * 1000.0f);
	}
	cmd->forwardmove = fighter_bot_cmd[n].forwardmove;
	cmd->rightmove = fighter_bot_cmd[n].rightmove;
	cmd->upmove = fighter_bot_cmd[n].upmove;
	cmd->buttons = fighter_bot_cmd[n].buttons;
	cmd->generic_cmd = 0;
	cmd->weapon = bot->client->ps.weapon;
	for (int axis = 0; axis < 3; axis++)
	{
		cmd->angles[axis] = fighter_bot_cmd[n].angles[axis];
	}
}

