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
static int fighter_side[MAX_GENTITIES]; // a made pilot's side (NPC_Begin puts siege NPCs on a team by who they hate)
static qboolean fighter_flown[MAX_GENTITIES]; // a ship a player has flown since it was made
static int fighter_crew_time;
static int fighter_out_since[MAX_GENTITIES]; // an AI ship out past the map's edges: since when
static vec3_t fighter_launch_pos[MAX_GENTITIES]; // where a pilot got into his ship, and the way it stood
static vec3_t fighter_launch_angles[MAX_GENTITIES];
static vec3_t fighter_box_mins, fighter_box_maxs; // the space the ships fly in (Fighter_FindPlayBox)
static qboolean fighter_box_set;

static void Fighter_FindPlayBox(void);

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

// a pilot for this ship, made right in it (hidden in there); he gets in on his first think (NPC_FighterAI)
static void Fighter_MakePilot(gentity_t* ship, const int side)
{
	gentity_t* spawner = G_Spawn();
	if (!spawner)
	{
		return;
	}
	spawner->classname = "NPC_spawner";
	spawner->NPC_type = side == TEAM_RED ? "rebel" : "stormpilot";
	spawner->count = 1;
	spawner->spawnflags = 64 | 128; // NOTSOLID | STARTINSOLID: it starts inside the ship
	spawner->s.teamowner = side;
	G_SetOrigin(spawner, ship->r.currentOrigin);
	VectorCopy(ship->r.currentOrigin, spawner->s.origin);

	gentity_t* pilot = NPC_Spawn_Do(spawner); // (frees the spawner)
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
		const int max_crew = side == TEAM_RED ? g_spaceWingmen.integer : g_spaceEnemies.integer;
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
	// first ("dropTime"), then flies out
	int launch_time = 2500;
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
		Com_Printf("fighter AI: %s %d flies %s %d for team %d\n", pilot->NPC_type, pilot->s.number, ship->NPC_type,
			ship->s.number, pilot->client->sess.sessionTeam);
	}

	pilot->painDebounceTime = level.time; // (a hidden pilot is never hurt: the developer messages use it as boarding time)
	VectorCopy(ship->r.currentOrigin, pilot->pos4); // home, the centre of its patrol
	VectorCopy(ship->r.currentOrigin, pilot->pos3);
	TIMER_Set(pilot, "fighterLaunch", launch_time); // drops clear and flies straight out of its hangar first
	// (one standing in a hangar, not hanging in a rack, is slow to get going: it keeps on out until it is clear)
	TIMER_Set(pilot, "fighterLaunchMax", p_veh->m_iDropTime > level.time ? 0 : launch_time + 5500);
	VectorCopy(ship->r.currentOrigin, fighter_launch_pos[pilot->s.number]);
	VectorSet(fighter_launch_angles[pilot->s.number], 0.0f, p_veh->m_vOrientation[YAW], 0.0f);
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
	memset(fighter_side, 0, sizeof fighter_side);
	memset(fighter_flown, 0, sizeof fighter_flown);
	memset(fighter_out_since, 0, sizeof fighter_out_since);
	fighter_box_set = qfalse; // (worked out when first needed, once the map's triggers are linked)
	fighter_crew_time = 0;
	route_show_client = -1;
	Route_Load(-2);
}

// draws the route while it is shown, around the one who asked: the points as white posts, the links as red lines
void G_FighterRoute_Frame(void)
{
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
					Com_Printf("fighter AI: %s %d at route point %d, on to %d\n", NPCS.NPC->NPC_type, n, cur, next);
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
		Fighter_Board(npc); // just made: into his ship
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
	gentity_t* ship = (gentity_t*)p_veh->m_pParentEntity;
	if (ship->health <= 0)
	{
		return qtrue;
	}

	const int skill = Fighter_Skill();
	const float turn_rate = 60.0f + 20.0f * (float)skill; // degrees per second
	const float max_step = turn_rate * FIGHTER_THINK_SECONDS;
	const float aim_error = 3.0f - 0.8f * (float)skill; // degrees
	const int my_side = npc->client->sess.sessionTeam;

	vec3_t fwd, my_pos;
	AngleVectors(p_veh->m_vOrientation, fwd, NULL, NULL);
	VectorCopy(ship->r.currentOrigin, my_pos);
	const float speed = VectorLength(ship->client->ps.velocity);

	// going into hyperspace (a trigger_hyperspace: the Rebels' hangar on siege_destroyer2 is far off, in one): the ship
	// turns to the jump by itself (PM_VehFaceHyperspacePoint), keep the view where it puts it, and turbo as that does
	// for a player (a ship standing still does not turn)
	if (ship->client->ps.hyperSpaceTime && level.time - ship->client->ps.hyperSpaceTime < HYPERSPACE_TIME)
	{
		NPCS.ucmd.upmove = 127;
		for (int axis = PITCH; axis <= ROLL; axis++)
		{
			NPCS.ucmd.angles[axis] = ANGLE2SHORT(NPCS.client->ps.viewangles[axis]) - NPCS.client->ps.delta_angles[axis];
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

	// out of the hangar first: straight on the way it stood, full throttle (taking off if it stands on the floor), until
	// it is well clear of where it got in (a ship taking off from a floor is slow to get going) or long enough
	if (!TIMER_Done(npc, "fighterLaunch")
		|| !TIMER_Done(npc, "fighterLaunchMax") && Distance(my_pos, fighter_launch_pos[npc->s.number]) < 2000.0f)
	{
		if (p_veh->m_iDropTime < level.time) // not still dropping clear of its rack
		{
			NPCS.ucmd.forwardmove = 127;
			if (ship->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				NPCS.ucmd.upmove = 127; // standing on the hangar floor: take off (in the air upmove is turbo)
			}
		}
		vec3_t out_dir;
		AngleVectors(fighter_launch_angles[npc->s.number], out_dir, NULL, NULL);
		Fighter_TurnTowards(out_dir, max_step);
		VectorCopy(my_pos, npc->pos4); // home is where it is out of the hangar, not in it
		VectorCopy(my_pos, npc->pos3);
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
