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

#include "ui/menudef.h"			// for the voice chats

extern qboolean in_camera;
//rww - for getting bot commands...
int AcceptBotCommand(const char* cmd, const gentity_t* pl);
//end rww

void WP_SetSaber(int entNum, saberInfo_t* sabers, int saberNum, const char* saber_name);

void Cmd_NPC_f(gentity_t* ent);
void Cmd_AdminNPC_f(gentity_t* ent);
void SetTeamQuick(const gentity_t* ent, int team, qboolean doBegin);
extern void G_RemoveWeather(void);
extern void G_SetTauntAnim(gentity_t* ent, int taunt);

extern vmCvar_t d_saberStanceDebug;

extern qboolean WP_SaberCanTurnOffSomeBlades(const saberInfo_t* saber);

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

const gbuyable_t bg_buylist[] =
{
	// text				giTag				giType			quantity    price	wc
	{"melee", WP_MELEE, IT_WEAPON, 0, 0, WC_MELEE}, // weaponless
	{"pistol", WP_BRYAR_PISTOL, IT_WEAPON, 0, 2, WC_PISTOL}, // pistol
	{"blaster", WP_BLASTER, IT_WEAPON, 350, 2, WC_RIFLE}, // rifle
	{"disruptor", WP_DISRUPTOR, IT_WEAPON, 300, 2, WC_RIFLE}, // sniper rifle
	{"bowcaster", WP_BOWCASTER, IT_WEAPON, 350, 2, WC_RIFLE}, // rifle
	{"repeater", WP_REPEATER, IT_WEAPON, 350, 2, WC_RIFLE}, // auto rifle
	{"electro", WP_DEMP2, IT_WEAPON, 350, 2, WC_RIFLE}, // electro gun
	{"flechette", WP_FLECHETTE, IT_WEAPON, 350, 2, WC_RIFLE}, // shotgun
	{"launcher", WP_ROCKET_LAUNCHER, IT_WEAPON, 3, 2, WC_HEAVY}, // rocket
	{"concussion", WP_CONCUSSION, IT_WEAPON, 300, 2, WC_HEAVY}, // mega rifle

	{"energy", AMMO_BLASTER, IT_AMMO, 500, 1, WC_AMMO},
	{"powercells", AMMO_POWERCELL, IT_AMMO, 300, 1, WC_AMMO},
	{"bolts", AMMO_METAL_BOLTS, IT_AMMO, 375, 1, WC_AMMO},
	{"rockets", AMMO_ROCKETS, IT_AMMO, 3, 1, WC_AMMO},
	{"thermal", AMMO_THERMAL, IT_WEAPON, 3, 1, WC_GRENADE},
	{"mine", AMMO_TRIPMINE, IT_AMMO, 5, 1, WC_GRENADE},
	{"detpack", AMMO_DETPACK, IT_AMMO, 5, 1, WC_GRENADE},
	{"ammo", AMMO_NONE, IT_AMMO, 0, 0, WC_AMMO},

	{"health", 4, IT_HEALTH, 25, 5, WC_ARMOR}, // heath boost
	{"shield", 2, IT_ARMOR, 50, 5, WC_ARMOR}, // armor

	{"binoculars", HI_BINOCULARS, IT_HOLDABLE, 1, 2, WC_ITEM}, // binoculars!
	{"jetpack", HI_JETPACK, IT_HOLDABLE, 1, 5, WC_ITEM}, // jetpack!
	{"cloak", HI_CLOAK, IT_HOLDABLE, 1, 3, WC_ITEM}, // cloak pack!
	{"bacta", HI_MEDPAC_BIG, IT_HOLDABLE, 1, 2, WC_ITEM}, // portable health
	{"Flamethrower", HI_FLAMETHROWER, IT_HOLDABLE, 1, 5, WC_ITEM}, // Flamethrower
	{"sentry", HI_SENTRY_GUN, IT_HOLDABLE, 1, 2, WC_DEPLOY}, // sentry
	{"seeker", HI_SEEKER, IT_HOLDABLE, 1, 2, WC_DEPLOY}, // seeker drone
	{"barrier", HI_SHIELD, IT_HOLDABLE, 1, 2, WC_DEPLOY}, // shield barrier
	{"Sphereshield", HI_SPHERESHIELD, IT_HOLDABLE, 1, 2, WC_DEPLOY}, // shield barrier
	// end of list marker
	{NULL}
};

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage(const gentity_t* ent)
{
	char string[1400];
	int i;
	int accuracy;

	// send the latest information on all clients
	string[0] = 0;
	int stringlength = 0;

	int num_sorted = level.numConnectedClients;

	if (num_sorted > MAX_CLIENT_SCORE_SEND)
	{
		num_sorted = MAX_CLIENT_SCORE_SEND;
	}

	for (i = 0; i < num_sorted; i++)
	{
		const int score_flags = 0;
		char entry[1024];
		int ping;

		const gclient_t* cl = &level.clients[level.sortedClients[i]];

		if (cl->pers.connected == CON_CONNECTING)
		{
			ping = -1;
		}
		else if (g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT)
		{
			//make fake pings for bots.
			ping = Q_irand(40, 75);
		}
		else
		{
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		if (cl->accuracy_shots)
		{
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		}
		else
		{
			accuracy = 0;
		}
		const int perfect = cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ? 1 : 0;

		Com_sprintf(entry, sizeof entry,
			" %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i],
			cl->ps.persistant[PERS_SCORE], ping, (level.time - cl->pers.enterTime) / 60000,
			score_flags, g_entities[level.sortedClients[i]].s.powerups, accuracy,
			cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
			cl->ps.persistant[PERS_EXCELLENT_COUNT],
			cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
			cl->ps.persistant[PERS_DEFEND_COUNT],
			cl->ps.persistant[PERS_ASSIST_COUNT],
			perfect,
			cl->ps.persistant[PERS_CAPTURES]);
		const int j = strlen(entry);
		if (stringlength + j > 1022)
			break;
		strcpy(string + stringlength, entry);
		stringlength += j;
	}

	//still want to know the total # of clients
	i = level.numConnectedClients;

	trap->SendServerCommand(ent - g_entities, va("scores %i %i %i%s", i,
		level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE],
		string));
}

/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f(const gentity_t* ent)
{
	DeathmatchScoreboardMessage(ent);
}

/*
==================
ConcatArgs
==================
*/
char* ConcatArgs(const int start)
{
	static char line[MAX_STRING_CHARS];

	int len = 0;
	const int c = trap->Argc();
	for (int i = start; i < c; i++)
	{
		char arg[MAX_STRING_CHARS];
		trap->Argv(i, arg, sizeof arg);
		const int tlen = strlen(arg);
		if (len + tlen >= MAX_STRING_CHARS - 1)
		{
			break;
		}
		memcpy(line + len, arg, tlen);
		len += tlen;
		if (i != c - 1)
		{
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
StringIsInteger
==================
*/
qboolean StringIsInteger(const char* s)
{
	int i, len;
	qboolean found_digit = qfalse;

	for (i = 0, len = strlen(s); i < len; i++)
	{
		if (!isdigit(s[i]))
			return qfalse;

		found_digit = qtrue;
	}

	return found_digit;
}

/*
==================
clientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
static int clientNumberFromString(const gentity_t* to, const char* s, const qboolean allowconnecting)
{
	gclient_t* cl;
	int idnum;
	char clean_input[MAX_NETNAME];

	if (StringIsInteger(s))
	{
		// numeric values could be slot numbers
		idnum = atoi(s);
		if (idnum >= 0 && idnum < level.maxclients)
		{
			cl = &level.clients[idnum];
			if (cl->pers.connected == CON_CONNECTED)
				return idnum;
			if (allowconnecting && cl->pers.connected == CON_CONNECTING)
				return idnum;
		}
	}

	Q_strncpyz(clean_input, s, sizeof clean_input);
	Q_StripColor(clean_input);

	for (idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++)
	{
		// check for a name match
		if (cl->pers.connected != CON_CONNECTED)
			if (!allowconnecting || cl->pers.connected < CON_CONNECTING)
				continue;

		if (!Q_stricmp(cl->pers.netname_nocolor, clean_input))
			return idnum;
	}

	trap->SendServerCommand(to - g_entities, va("print \"User %s is not on the server\n\"", s));
	return -1;
}

static void SanitizeString2(char* in, char* out)
{
	int i = 0;
	int r = 0;

	while (in[i])
	{
		if (i >= MAX_NAME_LENGTH - 1)
		{ //the ui truncates the name here..
			break;
		}

		if (in[i] == '^')
		{
			if (in[i + 1] >= 48 && //'0'
				in[i + 1] <= 57) //'9'
			{ //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else
			{ //just skip the ^
				i++;
				continue;
			}
		}

		if (in[i] < 32)
		{
			i++;
			continue;
		}

		out[r] = tolower(in[i]);
		r++;
		i++;
	}
	out[r] = 0;
}

static int G_ClientNumberFromStrippedSubstring(const char* name)
{
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	int			i, match = -1;
	gclient_t* cl;

	// check for a name match
	SanitizeString2((char*)name, s2);

	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = &level.clients[level.sortedClients[i]];
		SanitizeString2(cl->pers.netname, n2);
		if (strstr(n2, s2))
		{
			if (match != -1)
			{ //found more than one match
				return -2;
			}
			match = level.sortedClients[i];
		}
	}

	return match;
}

static int G_ClientNumberFromArg(char* name)
{
	int client_id = 0;
	char* cp;

	cp = name;
	while (*cp)
	{
		if (*cp >= '0' && *cp <= '9') cp++;
		else
		{
			client_id = -1; //mark as alphanumeric
			break;
		}
	}

	if (client_id == 0)
	{ // arg is assumed to be client number
		client_id = atoi(name);
	}
	// arg is client name
	if (client_id == -1)
	{
		client_id = G_ClientNumberFromStrippedSubstring(name);
	}
	return client_id;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
static void G_Give(gentity_t* ent, const char* name, const char* args, const int argc)
{
	int i;
	qboolean give_all = qfalse;
	trace_t trace;

	if (!Q_stricmp(name, "all"))
		give_all = qtrue;

	if (give_all)
	{
		for (i = 0; i < HI_NUM_HOLDABLE; i++)
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << i;
	}

	if (give_all || !Q_stricmp(name, "health"))
	{
		if (argc == 3)
			ent->health = Com_Clampi(1, ent->client->ps.stats[STAT_MAX_HEALTH], atoi(args));
		else
		{
			if (level.gametype == GT_SIEGE && ent->client->siegeClass != -1)
				ent->health = bgSiegeClasses[ent->client->siegeClass].maxhealth;
			else
				ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if (!give_all)
			return;
	}

	if (give_all || !Q_stricmp(name, "armor") || !Q_stricmp(name, "shield"))
	{
		if (argc == 3)
			ent->client->ps.stats[STAT_ARMOR] = Com_Clampi(0, ent->client->ps.stats[STAT_MAX_HEALTH], atoi(args));
		else
		{
			if (level.gametype == GT_SIEGE && ent->client->siegeClass != -1)
				ent->client->ps.stats[STAT_ARMOR] = bgSiegeClasses[ent->client->siegeClass].maxarmor;
			else
				ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];

			if (ent->client->ps.stats[STAT_ARMOR] > 0)
				ent->client->ps.powerups[PW_BATTLESUIT] = Q3_INFINITE;
			else
				ent->client->ps.powerups[PW_BATTLESUIT] = 0;
		}

		if (!give_all)
			return;
	}

	if (give_all || !Q_stricmp(name, "force"))
	{
		if (argc == 3)
			ent->client->ps.fd.forcePower = Com_Clampi(0, ent->client->ps.fd.forcePowerMax, atoi(args));
		else
			ent->client->ps.fd.forcePower = ent->client->ps.fd.forcePowerMax;

		if (!give_all)
			return;
	}

	if (give_all || !Q_stricmp(name, "weapons"))
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << (LAST_USEABLE_WEAPON + 1)) - (1 << WP_NONE);
		G_AddEvent(ent, EV_WEAPINVCHANGE, ent->client->ps.stats[STAT_WEAPONS]);
		if (!give_all)
			return;
	}
	if (give_all || Q_stricmp(name, "inventory") == 0)
	{
		for (i = 0; i < HI_NUM_HOLDABLE; i++)
		{
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << i;
		}
	}

	if (!give_all && !Q_stricmp(name, "weaponnum"))
	{
		ent->client->ps.stats[STAT_WEAPONS] |= 1 << atoi(args);
		G_AddEvent(ent, EV_WEAPINVCHANGE, ent->client->ps.stats[STAT_WEAPONS]);
		return;
	}

	if (give_all || !Q_stricmp(name, "ammo"))
	{
		int num = 500;
		if (argc == 3)
			num = Com_Clampi(0, 500, atoi(args));
		for (int i1 = AMMO_BLASTER; i1 < AMMO_MAX; i1++)
			ent->client->ps.ammo[i1] = num;
		if (!give_all)
			return;
	}

	if (!Q_stricmp(name, "excellent"))
	{
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if (!Q_stricmp(name, "impressive"))
	{
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if (!Q_stricmp(name, "gauntletaward"))
	{
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}
	if (!Q_stricmp(name, "defend"))
	{
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if (!Q_stricmp(name, "assist"))
	{
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	if (!give_all)
	{
		gitem_t* it = BG_FindItem(name);
		if (!it)
			return;

		gentity_t* it_ent = G_Spawn();
		VectorCopy(ent->r.currentOrigin, it_ent->s.origin);
		it_ent->classname = it->classname;
		G_SpawnItem(it_ent, it);
		if (!it_ent || !it_ent->inuse)
			return;
		FinishSpawningItem(it_ent);
		if (!it_ent || !it_ent->inuse)
			return;
		memset(&trace, 0, sizeof trace);
		Touch_Item(it_ent, ent, &trace);
		if (it_ent->inuse)
			G_FreeEntity(it_ent);
	}
}

static void Cmd_Give_f(gentity_t* ent)
{
	char name[MAX_TOKEN_CHARS] = { 0 };

	trap->Argv(1, name, sizeof name);
	G_Give(ent, name, ConcatArgs(2), trap->Argc());
}

static void Cmd_GiveOther_f(const gentity_t* ent)
{
	char name[MAX_TOKEN_CHARS] = { 0 };
	char otherindex[MAX_TOKEN_CHARS];

	if (trap->Argc() < 3)
	{
		trap->SendServerCommand(ent - g_entities, "print \"Usage: giveother <player id> <givestring>\n\"");
		return;
	}

	trap->Argv(1, otherindex, sizeof otherindex);
	const int i = clientNumberFromString(ent, otherindex, qfalse);
	if (i == -1)
	{
		return;
	}

	gentity_t* other_ent = &g_entities[i];
	if (!other_ent->inuse || !other_ent->client)
	{
		return;
	}

	if (other_ent->health <= 0 || other_ent->client->tempSpectate >= level.time || other_ent->client->sess.sessionTeam ==
		TEAM_SPECTATOR)
	{
		// Intentionally displaying for the command user
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBEALIVE")));
		return;
	}

	trap->Argv(2, name, sizeof name);

	G_Give(other_ent, name, ConcatArgs(3), trap->Argc() - 1);
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
static void Cmd_God_f(gentity_t* ent)
{
	char* msg;

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE))
		msg = "godmode OFF";
	else
		msg = "godmode ON";

	trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", msg));
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
static void Cmd_Notarget_f(gentity_t* ent)
{
	char* msg;

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET))
		msg = "notarget OFF";
	else
		msg = "notarget ON";

	trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", msg));
}

static void Cmd_Noforce_f(gentity_t* ent)
{
	char* msg;

	ent->flags ^= FL_NOFORCE;
	if (!(ent->flags & FL_NOFORCE))
		msg = "No Force OFF";
	else
		msg = "No Force ON";

	trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", msg));
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
static void Cmd_Noclip_f(gentity_t* ent)
{
	char* msg;

	if (in_camera)
		return;

	if (!ent->client->noclip)
	{
		msg = "noclip OFF";
	}
	else
	{
		msg = "noclip ON";
	}

	ent->client->noclip = !ent->client->noclip;
	ent->flags ^= FL_NOFORCE;

	trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", msg));
}

/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
static void Cmd_LevelShot_f(const gentity_t* ent)
{
	if (!ent->client->pers.localClient)
	{
		trap->SendServerCommand(ent - g_entities,
			"print \"The levelshot command must be executed by a local client\n\"");
		return;
	}

	// doesn't work in single player
	if (level.gametype == GT_SINGLE_PLAYER)
	{
		trap->SendServerCommand(ent - g_entities, "print \"Must not be in singleplayer mode for levelshot\n\"");
		return;
	}

	BeginIntermission();
	trap->SendServerCommand(ent - g_entities, "clientLevelShot");
}

#if 0
/*
==================
Cmd_TeamTask_f

From TA.
==================
*/
static void Cmd_TeamTask_f(gentity_t* ent) {
	char userinfo[MAX_INFO_STRING];
	char		arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if (trap->Argc() != 2) {
		return;
	}
	trap->Argv(1, arg, sizeof(arg));
	task = atoi(arg);

	trap->GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap->SetUserinfo(client, userinfo);
	client_userinfo_changed(client);
}
#endif

static void G_Kill(gentity_t* ent)
{
	if (in_camera)
		return;

	if ((level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) &&
		level.numPlayingClients > 1 && !level.warmupTime)
	{
		if (!g_allowDuelSuicide.integer)
		{
			trap->SendServerCommand(ent - g_entities,
				va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "ATTEMPTDUELKILL")));
			return;
		}
	}

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die(ent, ent, ent, 100000, MOD_SUICIDE);
}

/*
=================
Cmd_Kill_f
=================
*/
static void Cmd_Kill_f(gentity_t* ent)
{
	G_Kill(ent);
}

static void Cmd_KillOther_f(const gentity_t* ent)
{
	char otherindex[MAX_TOKEN_CHARS];

	if (trap->Argc() < 2)
	{
		trap->SendServerCommand(ent - g_entities, "print \"Usage: killother <player id>\n\"");
		return;
	}

	trap->Argv(1, otherindex, sizeof otherindex);
	const int i = clientNumberFromString(ent, otherindex, qfalse);
	if (i == -1)
	{
		return;
	}

	gentity_t* other_ent = &g_entities[i];
	if (!other_ent->inuse || !other_ent->client)
	{
		return;
	}

	if (other_ent->health <= 0 || other_ent->client->tempSpectate >= level.time || other_ent->client->sess.sessionTeam ==
		TEAM_SPECTATOR)
	{
		// Intentionally displaying for the command user
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBEALIVE")));
		return;
	}

	G_Kill(other_ent);
}

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange(gclient_t* client, const int old_team)
{
	client->ps.fd.forceDoInit = 1; //every time we change teams make sure our force powers are set right

	if (level.gametype == GT_SIEGE)
	{
		//don't announce these things in siege
		return;
	}

	if (level.gametype == GT_SINGLE_PLAYER)
	{
		if (client->sess.sessionTeam == TEAM_SPECTATOR && old_team != TEAM_SPECTATOR)
		{
			trap->SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname,
				G_GetStringEdString("MP_SVGAME", "JOINEDTHESPECTATORS")));
		}
		else if (client->sess.sessionTeam == TEAM_FREE)
		{
			trap->SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBATTLE")));
		}
	}
	else
	{
		if (client->sess.sessionTeam == TEAM_RED)
		{
			trap->SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEREDTEAM")));
		}
		else if (client->sess.sessionTeam == TEAM_BLUE)
		{
			trap->SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname,
				G_GetStringEdString("MP_SVGAME", "JOINEDTHEBLUETEAM")));
		}
		else if (client->sess.sessionTeam == TEAM_SPECTATOR && old_team != TEAM_SPECTATOR)
		{
			trap->SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname,
				G_GetStringEdString("MP_SVGAME", "JOINEDTHESPECTATORS")));
		}
		else if (client->sess.sessionTeam == TEAM_FREE)
		{
			trap->SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBATTLE")));
		}
	}

	G_LogPrintf("ChangeTeam: %i [%s] (%s) \"%s^7\" %s -> %s\n", client - level.clients, client->sess.IP,
		client->pers.guid, client->pers.netname, TeamName(old_team), TeamName(client->sess.sessionTeam));
}

static qboolean G_PowerDuelCheckFail(const gentity_t* ent)
{
	int loners = 0;
	int doubles = 0;

	if (!ent->client || ent->client->sess.duelTeam == DUELTEAM_FREE)
	{
		return qtrue;
	}

	G_PowerDuelCount(&loners, &doubles, qfalse);

	if (ent->client->sess.duelTeam == DUELTEAM_LONE && loners >= 1)
	{
		return qtrue;
	}

	if (ent->client->sess.duelTeam == DUELTEAM_DOUBLE && doubles >= 2)
	{
		return qtrue;
	}

	return qfalse;
}

/*
=================
SetTeam
=================
*/
qboolean g_dontPenalizeTeam = qfalse;
qboolean g_preventTeamBegin = qfalse;

void SetTeam(gentity_t* ent, const char* s)
{
	int team;

	// fix: this prevents rare creation of invalid players
	if (!ent->inuse)
	{
		return;
	}

	//
	// see what change is requested
	//
	gclient_t* client = ent->client;

	const int clientNum = client - level.clients;
	int spec_client = 0;
	spectatorState_t spec_state = SPECTATOR_NOT;
	if (!Q_stricmp(s, "scoreboard") || !Q_stricmp(s, "score"))
	{
		team = TEAM_SPECTATOR;
		spec_state = SPECTATOR_FREE;
		// SPECTATOR_SCOREBOARD disabling this for now since it is totally broken on client side
	}
	else if (!Q_stricmp(s, "follow1"))
	{
		team = TEAM_SPECTATOR;
		spec_state = SPECTATOR_FOLLOW;
		spec_client = -1;
	}
	else if (!Q_stricmp(s, "follow2"))
	{
		team = TEAM_SPECTATOR;
		spec_state = SPECTATOR_FOLLOW;
		spec_client = -2;
	}
	else if (!Q_stricmp(s, "spectator") || !Q_stricmp(s, "s") || !Q_stricmp(s, "spectate"))
	{
		team = TEAM_SPECTATOR;
		spec_state = SPECTATOR_FREE;
	}
	else if (g_gametype.integer == GT_SINGLE_PLAYER)
	{
		//players spawn on NPCTEAM_PLAYER
		team = NPCTEAM_PLAYER;
	}
	else if (level.gametype >= GT_TEAM)
	{
		// if running a team game, assign player to one of the teams
		spec_state = SPECTATOR_NOT;
		if (!Q_stricmp(s, "red") || !Q_stricmp(s, "r"))
		{
			team = TEAM_RED;
		}
		else if (!Q_stricmp(s, "blue") || !Q_stricmp(s, "b"))
		{
			team = TEAM_BLUE;
		}
		else
		{
			team = PickTeam(clientNum);
		}

		if (g_teamForceBalance.integer && !g_jediVmerc.integer)
		{
			int counts[TEAM_NUM_TEAMS];

			//JAC: Invalid clientNum was being used
			counts[TEAM_BLUE] = TeamCount(ent - g_entities, TEAM_BLUE);
			counts[TEAM_RED] = TeamCount(ent - g_entities, TEAM_RED);

			// We allow a spread of two
			if (team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1)
			{
				{
					//JAC: Invalid clientNum was being used
					trap->SendServerCommand(ent - g_entities,
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYRED")));
				}
				return; // ignore the request
			}
			if (team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1)
			{
				{
					//JAC: Invalid clientNum was being used
					trap->SendServerCommand(ent - g_entities,
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYBLUE")));
				}
				return; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}
	}
	else
	{
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	const int old_team = client->sess.sessionTeam;

	if (level.gametype == GT_SIEGE)
	{
		if (client->tempSpectate >= level.time &&
			team == TEAM_SPECTATOR)
		{
			//sorry, can't do that.
			return;
		}

		if (team == old_team && team != TEAM_SPECTATOR)
			return;

		client->sess.siegeDesiredTeam = team;

		if (client->sess.sessionTeam != TEAM_SPECTATOR &&
			team != TEAM_SPECTATOR)
		{
			//not a spectator now, and not switching to spec, so you have to wait til you die.
			qboolean do_begin;
			if (ent->client->tempSpectate >= level.time)
			{
				do_begin = qfalse;
			}
			else
			{
				do_begin = qtrue;
			}

			if (do_begin)
			{
				// Kill them so they automatically respawn in the team they wanted.
				if (ent->health > 0)
				{
					ent->flags &= ~FL_GODMODE;
					ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
				}
			}

			if (ent->client->sess.sessionTeam != ent->client->sess.siegeDesiredTeam)
			{
				SetTeamQuick(ent, ent->client->sess.siegeDesiredTeam, qfalse);
			}

			return;
		}
	}

	// override decision if limiting the players
	if (level.gametype == GT_DUEL
		&& level.numNonSpectatorClients >= 2)
	{
		team = TEAM_SPECTATOR;
	}
	else if (level.gametype == GT_POWERDUEL
		&& (level.numPlayingClients >= 3 || G_PowerDuelCheckFail(ent)))
	{
		team = TEAM_SPECTATOR;
	}
	else if (g_maxGameClients.integer > 0 &&
		level.numNonSpectatorClients >= g_maxGameClients.integer)
	{
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	if (team == old_team && team != TEAM_SPECTATOR)
	{
		return;
	}

	// if the player was dead leave the body
	if (client->ps.stats[STAT_HEALTH] <= 0 && client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		MaintainBodyQueue(ent);
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;

	if (old_team != TEAM_SPECTATOR)
	{
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		g_dontPenalizeTeam = qtrue;
		player_die(ent, ent, ent, 100000, MOD_SUICIDE);
		g_dontPenalizeTeam = qfalse;
	}

	// they go to the end of the line for tournaments
	if (team == TEAM_SPECTATOR && old_team != team)
	{
		AddTournamentQueue(client);
	}

	// hack to preserve score information around spectating sessions!
	if (team == TEAM_SPECTATOR && old_team != TEAM_SPECTATOR)
	{
		ent->client->pers.save_score = ent->client->ps.persistant[PERS_SCORE]; // tuck this away (lmo)
	}
	if (team != TEAM_SPECTATOR && old_team == TEAM_SPECTATOR)
	{
		// when oldteam is spectator, take care to restore old score info!
		// this is potentially changed during a specator/follow! (see SpectatorClientEndFrame)
		if (g_spectate_keep_score.integer >= 1
			&& level.gametype != GT_DUEL
			&& level.gametype != GT_POWERDUEL
			&& level.gametype != GT_SIEGE)
		{
			ent->client->ps.persistant[PERS_SCORE] = ent->client->pers.save_score; // restore!
		}
		else
		{
			if (level.gametype != GT_DUEL
				&& level.gametype != GT_POWERDUEL
				&& ent->s.eType != ET_NPC)
			{
				if (g_spectate_keep_score.integer >= 0)
				{
					ent->client->sess.losses = 0;
				}
				ent->client->sess.wins = 0;
			}
		}
	}

	// clear votes if going to spectator (specs can't vote)
	if (team == TEAM_SPECTATOR)
	{
		G_ClearVote(ent);
	}
	// also clear team votes if switching red/blue or going to spec
	G_ClearTeamVote(ent, old_team);

	client->sess.sessionTeam = (team_t)team;
	client->sess.spectatorState = spec_state;
	client->sess.spectatorClient = spec_client;

	client->sess.teamLeader = qfalse;
	if (team == TEAM_RED || team == TEAM_BLUE)
	{
		const int team_leader = TeamLeader(team);
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if (team_leader == -1 || !(g_entities[clientNum].r.svFlags & SVF_BOT) && g_entities[team_leader].r.svFlags &
			SVF_BOT)
		{
			//SetLeader( team, clientNum );
		}
	}
	// make sure there is a team leader on the team the player came from
	if (old_team == TEAM_RED || old_team == TEAM_BLUE)
	{
		CheckTeamLeader(old_team);
	}

	BroadcastTeamChange(client, old_team);

	//make a disappearing effect where they were before teleporting them to the appropriate spawn point,
	//if we were not on the spec team
	if (old_team != TEAM_SPECTATOR)
	{
		gentity_t* tent = G_TempEntity(client->ps.origin, EV_PLAYER_TELEPORT_OUT);
		tent->s.clientNum = clientNum;
	}

	// get and distribute relevent paramters
	if (!client_userinfo_changed(clientNum))
		return;

	if (!g_preventTeamBegin)
	{
		ClientBegin(clientNum, qfalse);
	}
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void G_LeaveVehicle(gentity_t* ent, qboolean con_check);

void StopFollowing(gentity_t* ent)
{
	ent->client->ps.persistant[PERS_TEAM] = TEAM_SPECTATOR;
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
	ent->client->ps.weapon = WP_NONE;
	G_LeaveVehicle(ent, qfalse); // clears m_iVehicleNum as well
	ent->client->ps.emplacedIndex = 0;
	//ent->client->ps.m_iVehicleNum = 0;
	ent->client->ps.viewangles[ROLL] = 0.0f;
	ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
	ent->client->ps.forceHandExtendTime = 0;
	ent->client->ps.zoomMode = 0;
	ent->client->ps.zoomLocked = qfalse;
	ent->client->ps.zoomLockTime = 0;
	ent->client->ps.saber_move = LS_NONE;
	ent->client->ps.legsAnim = 0;
	ent->client->ps.legsTimer = 0;
	ent->client->ps.torsoAnim = 0;
	ent->client->ps.torsoTimer = 0;
	ent->client->ps.duelInProgress = qfalse;
	ent->client->ps.isJediMaster = qfalse;
	// major exploit if you are spectating somebody and they are JM and you reconnect
	ent->client->ps.cloakFuel = 100; // so that fuel goes away after stop following them
	ent->client->ps.jetpackFuel = 100; // so that fuel goes away after stop following them
	ent->client->ps.sprintFuel = 100;
	ent->health = ent->client->ps.stats[STAT_HEALTH] = 100;
	// so that you don't keep dead angles if you were spectating a dead person
	ent->client->ps.bobCycle = 0;
	ent->client->ps.pm_type = PM_SPECTATOR;
	ent->client->ps.eFlags &= ~EF_DISINTEGRATION;
	for (int i = 0; i < PW_NUM_POWERUPS; i++)
		ent->client->ps.powerups[i] = 0;
}

/*
=================
Cmd_Team_f
=================
*/
static void Cmd_Team_f(gentity_t* ent)
{
	char s[MAX_TOKEN_CHARS];

	const int old_team = ent->client->sess.sessionTeam;

	if (trap->Argc() != 2)
	{
		if (level.gametype == GT_SINGLE_PLAYER)
		{
			switch (old_team)
			{
			case NPCTEAM_PLAYER:
				trap->SendServerCommand(ent - g_entities,
					va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTFREETEAM")));
				break;
			case TEAM_SPECTATOR:
				trap->SendServerCommand(ent - g_entities,
					va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTSPECTEAM")));
				break;
			default:;
			}
		}
		else
		{
			switch (old_team)
			{
			case TEAM_BLUE:
				trap->SendServerCommand(ent - g_entities,
					va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTBLUETEAM")));
				break;
			case TEAM_RED:
				trap->SendServerCommand(ent - g_entities,
					va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTREDTEAM")));
				break;
			case TEAM_FREE:
				trap->SendServerCommand(ent - g_entities,
					va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTFREETEAM")));
				break;
			case TEAM_SPECTATOR:
				trap->SendServerCommand(ent - g_entities,
					va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTSPECTEAM")));
				break;
			default:;
			}
		}
		return;
	}

	if (ent->client->switchTeamTime > level.time)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")));
		return;
	}

	if (gEscaping)
	{
		return;
	}

	if (in_camera)
	{
		return;
	}

	// if they are playing a tournament game, count as a loss
	if (level.gametype == GT_DUEL
		&& ent->client->sess.sessionTeam == TEAM_FREE)
	{
		//in a tournament game
		//disallow changing teams
		trap->SendServerCommand(ent - g_entities, "print \"Cannot switch teams in Duel\n\"");
		return;
	}

	if (level.gametype == GT_POWERDUEL)
	{
		//don't let clients change teams manually at all in powerduel, it will be taken care of through automated stuff
		trap->SendServerCommand(ent - g_entities, "print \"Cannot switch teams in Power Duel\n\"");
		return;
	}

	trap->Argv(1, s, sizeof s);

	SetTeam(ent, s);

	// fix: update team switch time only if team change really happend
	if (old_team != ent->client->sess.sessionTeam)
		ent->client->switchTeamTime = level.time + 5000;
}

/*
=================
Cmd_DuelTeam_f
=================
*/
static void Cmd_DuelTeam_f(gentity_t* ent)
{
	int old_team;
	char s[MAX_TOKEN_CHARS];

	if (level.gametype != GT_POWERDUEL)
	{
		//don't bother doing anything if this is not power duel
		return;
	}

	if (trap->Argc() != 2)
	{
		//No arg so tell what team we're currently on.
		old_team = ent->client->sess.duelTeam;
		switch (old_team)
		{
		case DUELTEAM_FREE:
			trap->SendServerCommand(ent - g_entities, va("print \"None\n\""));
			break;
		case DUELTEAM_LONE:
			trap->SendServerCommand(ent - g_entities, va("print \"Single\n\""));
			break;
		case DUELTEAM_DOUBLE:
			trap->SendServerCommand(ent - g_entities, va("print \"Double\n\""));
			break;
		default:
			break;
		}
		return;
	}

	if (ent->client->switchDuelTeamTime > level.time)
	{
		//debounce for changing
		trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")));
		return;
	}

	trap->Argv(1, s, sizeof s);

	old_team = ent->client->sess.duelTeam;

	if (!Q_stricmp(s, "free"))
	{
		ent->client->sess.duelTeam = DUELTEAM_FREE;
	}
	else if (!Q_stricmp(s, "single"))
	{
		ent->client->sess.duelTeam = DUELTEAM_LONE;
	}
	else if (!Q_stricmp(s, "double"))
	{
		ent->client->sess.duelTeam = DUELTEAM_DOUBLE;
	}
	else
	{
		trap->SendServerCommand(ent - g_entities, va("print \"'%s' not a valid duel team.\n\"", s));
	}

	if (old_team == ent->client->sess.duelTeam)
	{
		//didn't actually change, so don't care.
		return;
	}

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		//ok..die
		const int cur_team = ent->client->sess.duelTeam;
		ent->client->sess.duelTeam = old_team;
		G_Damage(ent, ent, ent, NULL, ent->client->ps.origin, 99999, DAMAGE_NO_PROTECTION, MOD_SUICIDE);
		ent->client->sess.duelTeam = cur_team;
	}
	//reset wins and losses
	ent->client->sess.wins = 0;
	ent->client->sess.losses = 0;

	//get and distribute relevent paramters
	if (client_userinfo_changed(ent->s.number))
		return;

	ent->client->switchDuelTeamTime = level.time + 5000;
}

static int G_TeamForSiegeClass(const char* cl_name)
{
	int i = 0;
	int team = SIEGETEAM_TEAM1;
	const siegeTeam_t* stm = BG_SiegeFindThemeForTeam(team);

	if (!stm)
	{
		return 0;
	}

	while (team <= SIEGETEAM_TEAM2)
	{
		const siegeClass_t* scl = stm->classes[i];

		if (scl && scl->name[0])
		{
			if (!Q_stricmp(cl_name, scl->name))
			{
				return team;
			}
		}

		i++;
		if (i >= MAX_SIEGE_CLASSES || i >= stm->numClasses)
		{
			if (team == SIEGETEAM_TEAM2)
			{
				break;
			}
			team = SIEGETEAM_TEAM2;
			stm = BG_SiegeFindThemeForTeam(team);
			i = 0;
		}
	}

	return 0;
}

/*
=================
Cmd_SiegeClass_f
=================
*/
static void Cmd_SiegeClass_f(gentity_t* ent)
{
	char class_name[64];
	qboolean startedAsSpec = qfalse;

	if (level.gametype != GT_SIEGE)
	{
		//classes are only valid for this gametype
		return;
	}

	if (!ent->client)
	{
		return;
	}

	if (trap->Argc() < 1)
	{
		return;
	}

	if (ent->client->switchClassTime > level.time)
	{
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCLASSSWITCH")));
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		startedAsSpec = qtrue;
	}

	trap->Argv(1, class_name, sizeof class_name);

	const int team = G_TeamForSiegeClass(class_name);

	if (!team)
	{
		//not a valid class name
		return;
	}

	if (ent->client->sess.sessionTeam != team)
	{
		//try changing it then
		g_preventTeamBegin = qtrue;
		if (team == TEAM_RED)
		{
			SetTeam(ent, "red");
		}
		else if (team == TEAM_BLUE)
		{
			SetTeam(ent, "blue");
		}
		g_preventTeamBegin = qfalse;

		if (ent->client->sess.sessionTeam != team)
		{
			//failed, oh well
			if (ent->client->sess.sessionTeam != TEAM_SPECTATOR ||
				ent->client->sess.siegeDesiredTeam != team)
			{
				trap->SendServerCommand(ent - g_entities,
					va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCLASSTEAM")));
				return;
			}
		}
	}

	//preserve 'is score
	const int pre_score = ent->client->ps.persistant[PERS_SCORE];

	//Make sure the class is valid for the team
	BG_SiegeCheckClassLegality(team, class_name);

	//Set the session data
	strcpy(ent->client->sess.siegeClass, class_name);

	// get and distribute relevent paramters
	if (!client_userinfo_changed(ent->s.number))
		return;

	if (ent->client->tempSpectate < level.time)
	{
		// Kill him (makes sure he loses flags, etc)
		if (ent->health > 0 && !startedAsSpec)
		{
			ent->flags &= ~FL_GODMODE;
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			player_die(ent, ent, ent, 100000, MOD_SUICIDE);
		}

		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || startedAsSpec)
		{
			//respawn them instantly.
			ClientBegin(ent->s.number, qfalse);
		}
	}
	//set it back after we do all the stuff
	ent->client->ps.persistant[PERS_SCORE] = pre_score;

	ent->client->switchClassTime = level.time + 5000;
}

/*
=================
Cmd_ForceChanged_f
=================
*/
static void Cmd_ForceChanged_f(gentity_t* ent)
{
	char fp_ch_str[1024];
	//	Cmd_Kill_f(ent);
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		//No longer print it, as the UI calls this a lot.
		WP_InitForcePowers(ent);
		goto argCheck;
	}

	const char* buf = G_GetStringEdString("MP_SVGAME", "FORCEPOWERCHANGED");

	strcpy(fp_ch_str, buf);

	trap->SendServerCommand(ent - g_entities, va("print \"%s%s\n\"", S_COLOR_GREEN, fp_ch_str));

	ent->client->ps.fd.forceDoInit = 1;
argCheck:
	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		//If this is duel, don't even bother changing team in relation to this.
		return;
	}

	if (trap->Argc() > 1)
	{
		char arg[MAX_TOKEN_CHARS];

		trap->Argv(1, arg, sizeof arg);

		if (arg[0])
		{
			//if there's an arg, assume it's a combo team command from the UI.
			Cmd_Team_f(ent);
		}
	}
}

extern qboolean WP_SaberStyleValidForSaber(const saberInfo_t* saber1, const saberInfo_t* saber2, int saberHolstered, int saberAnimLevel);
extern qboolean WP_UseFirstValidSaberStyle(const saberInfo_t* saber1, const saberInfo_t* saber2, int saberHolstered, int* saberAnimLevel);

qboolean G_ValidSaberStyle(const gentity_t* ent, int saber_style);
qboolean G_SetSaber(const gentity_t* ent, const int saberNum, const char* saber_name, const qboolean siege_override)
{
	char truncSaberName[MAX_QPATH] = { 0 };

	if (!siege_override && level.gametype == GT_SIEGE && ent->client->siegeClass != -1 &&
		(bgSiegeClasses[ent->client->siegeClass].saberStance || bgSiegeClasses[ent->client->siegeClass].saber1[0] || bgSiegeClasses[ent->client->siegeClass].saber2[0]))
	{ //don't let it be changed if the siege class has forced any saber-related things
		return qfalse;
	}

	Q_strncpyz(truncSaberName, saber_name, sizeof(truncSaberName));

	if (saberNum == 0 && (!Q_stricmp("none", truncSaberName) || !Q_stricmp("remove", truncSaberName)))
	{ //can't remove saber 0 like this
		Q_strncpyz(truncSaberName, DEFAULT_SABER, sizeof(truncSaberName));
	}

	//Set the saber with the arg given. If the arg is
	//not a valid sabername defaults will be used.
	WP_SetSaber(ent->s.number, ent->client->saber, saberNum, truncSaberName);

	if (!ent->client->saber[0].model[0])
	{
		assert(0); //should never happen!
		Q_strncpyz(ent->client->pers.saber1, DEFAULT_SABER, sizeof(ent->client->pers.saber1));
	}
	else
		Q_strncpyz(ent->client->pers.saber1, ent->client->saber[0].name, sizeof(ent->client->pers.saber1));

	if (!ent->client->saber[1].model[0])
		Q_strncpyz(ent->client->pers.saber2, "none", sizeof(ent->client->pers.saber2));
	else
		Q_strncpyz(ent->client->pers.saber2, ent->client->saber[1].name, sizeof(ent->client->pers.saber2));

	if (!WP_SaberStyleValidForSaber(&ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, ent->client->ps.fd.saberAnimLevel))
	{
		WP_UseFirstValidSaberStyle(&ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &ent->client->ps.fd.saberAnimLevel);
		ent->client->ps.fd.saber_anim_levelBase = ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
	}

	return qtrue;
}

/*
=================
Cmd_Follow_f
=================
*/
static void Cmd_Follow_f(gentity_t* ent)
{
	char arg[MAX_TOKEN_CHARS];

	if (ent->client->sess.spectatorState == SPECTATOR_NOT && ent->client->switchTeamTime > level.time)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")));
		return;
	}

	if (trap->Argc() != 2)
	{
		if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW)
		{
			StopFollowing(ent);
		}
		return;
	}

	trap->Argv(1, arg, sizeof arg);
	const int i = clientNumberFromString(ent, arg, qfalse);
	if (i == -1)
	{
		return;
	}

	// can't follow self
	if (&level.clients[i] == ent->client)
	{
		return;
	}

	// can't follow another spectator
	if (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR)
	{
		return;
	}

	if (level.clients[i].tempSpectate >= level.time)
	{
		return;
	}

	// if they are playing a tournament game, count as a loss
	if ((level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
		&& ent->client->sess.sessionTeam == TEAM_FREE)
	{
		//WTF???
		ent->client->sess.losses++;
	}

	// first set them to spectator
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		SetTeam(ent, "spectator");
		// fix: update team switch time only if team change really happend
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			ent->client->switchTeamTime = level.time + 5000;
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f(gentity_t* ent, const int dir)
{
	qboolean looped = qfalse;

	if (ent->client->sess.spectatorState == SPECTATOR_NOT && ent->client->switchTeamTime > level.time)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")));
		return;
	}

	// if they are playing a tournament game, count as a loss
	if ((level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
		&& ent->client->sess.sessionTeam == TEAM_FREE)
	{
		//WTF???
		ent->client->sess.losses++;
	}
	// first set them to spectator
	if (ent->client->sess.spectatorState == SPECTATOR_NOT)
	{
		SetTeam(ent, "spectator");
		// fix: update team switch time only if team change really happend
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			ent->client->switchTeamTime = level.time + 5000;
	}

	if (dir != 1 && dir != -1)
	{
		trap->Error(ERR_DROP, "Cmd_FollowCycle_f: bad dir %i", dir);
	}

	int clientNum = ent->client->sess.spectatorClient;
	const int original = clientNum;

	do
	{
		clientNum += dir;
		if (clientNum >= level.maxclients)
		{
			// Avoid /team follow1 crash
			if (looped)
			{
				break;
			}
			clientNum = 0;
			looped = qtrue;
		}
		if (clientNum < 0)
		{
			if (looped)
			{
				break;
			}
			clientNum = level.maxclients - 1;
			looped = qtrue;
		}

		// can only follow connected clients
		if (level.clients[clientNum].pers.connected != CON_CONNECTED)
		{
			continue;
		}

		// can't follow another spectator
		if (level.clients[clientNum].sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}

		// can't follow another spectator
		if (level.clients[clientNum].tempSpectate >= level.time)
		{
			return;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientNum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while (clientNum != original);

	// leave it where it was
}

static void Cmd_FollowNext_f(gentity_t* ent)
{
	Cmd_FollowCycle_f(ent, 1);
}

static void Cmd_FollowPrev_f(gentity_t* ent)
{
	Cmd_FollowCycle_f(ent, -1);
}

/*
==================
G_Say
==================
*/

static void G_SayTo(const gentity_t* ent, const gentity_t* other, const int mode, const int color, const char* name,
	const char* message,
	char* loc_msg)
{
	if (!other)
	{
		return;
	}
	if (!other->inuse)
	{
		return;
	}
	if (!other->client)
	{
		return;
	}
	if (other->client->pers.connected != CON_CONNECTED)
	{
		return;
	}
	if (mode == SAY_TEAM && !OnSameTeam(ent, other))
	{
		return;
	}
	if (mode == SAY_ALLY && ent != other && sje_is_ally(ent, other) == qfalse)
	{
		// allychat. Send it only to allies and to the player himself
		return;
	}

	if (level.gametype == GT_SIEGE &&
		ent->client && (ent->client->tempSpectate >= level.time || ent->client->sess.sessionTeam == TEAM_SPECTATOR) &&
		other->client->sess.sessionTeam != TEAM_SPECTATOR &&
		other->client->tempSpectate < level.time)
	{
		//siege temp spectators should not communicate to ingame players
		return;
	}

	if (loc_msg)
	{
		trap->SendServerCommand(other - g_entities, va("%s \"%s\" \"%s\" \"%c\" \"%s\" %i",
			mode == SAY_TEAM ? "ltchat" : "lchat",
			name, loc_msg, color, message, ent->s.number));
	}
	else
	{
		trap->SendServerCommand(other - g_entities, va("%s \"%s%c%c%s\" %i",
			mode == SAY_TEAM ? "tchat" : "chat",
			name, Q_COLOR_ESCAPE, color, message, ent->s.number));
	}
}

static void G_Say(const gentity_t* ent, const gentity_t* target, int mode, const char* chat_text)
{
	int color;
	char name[64];
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT];
	char location[64];
	char* loc_msg = NULL;

	if (level.gametype < GT_TEAM && mode == SAY_TEAM)
	{
		mode = SAY_ALL;
	}

	Q_strncpyz(text, chat_text, sizeof text);

	Q_strstrip(text, "\n\r", "  ");

	switch (mode)
	{
	default:
	case SAY_ALL:
		G_LogPrintf("say: %s: %s\n", ent->client->pers.netname, text);
		Com_sprintf(name, sizeof name, "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf("sayteam: %s: %s\n", ent->client->pers.netname, text);
		if (Team_GetLocationMsg(ent, location, sizeof location))
		{
			Com_sprintf(name, sizeof name, EC"(%s%c%c"EC")"EC": ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
			loc_msg = location;
		}
		else
		{
			Com_sprintf(name, sizeof name, EC"(%s%c%c"EC")"EC": ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		}
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && target->inuse && target->client && level.gametype >= GT_TEAM &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof location))
		{
			Com_sprintf(name, sizeof name, EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE,
				COLOR_WHITE);
			loc_msg = location;
		}
		else
		{
			Com_sprintf(name, sizeof name, EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE,
				COLOR_WHITE);
		}
		color = COLOR_MAGENTA;
		break;
	case SAY_ALLY: // say to allies
		G_LogPrintf("sayally: %s: %s\n", ent->client->pers.netname, text);
		Com_sprintf(name, sizeof name, EC"{%s%c%c"EC"}"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);

		color = COLOR_WHITE;
		break;
	}

	if (target)
	{
		G_SayTo(ent, target, mode, color, name, text, loc_msg);
		return;
	}

	// echo the text to the console
	if (dedicated.integer)
	{
		trap->Print("%s%s\n", name, text);
	}

	// send it to all the appropriate clients
	for (int j = 0; j < level.maxclients; j++)
	{
		const gentity_t* other = &g_entities[j];
		G_SayTo(ent, other, mode, color, name, text, loc_msg);
	}
}

/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f(const gentity_t* ent)
{
	if (trap->Argc() < 2)
		return;

	char* p = ConcatArgs(1);

	if (strlen(p) >= MAX_SAY_TEXT)
	{
		p[MAX_SAY_TEXT - 1] = '\0';
		G_SecurityLogPrintf("Cmd_Say_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname,
			p);
	}

	G_Say(ent, NULL, SAY_ALL, p);
}

/*
==================
Cmd_SayTeam_f
==================
*/
static void Cmd_SayTeam_f(const gentity_t* ent)
{
	if (trap->Argc() < 2)
		return;

	char* p = ConcatArgs(1);

	if (strlen(p) >= MAX_SAY_TEXT)
	{
		p[MAX_SAY_TEXT - 1] = '\0';
		G_SecurityLogPrintf("Cmd_SayTeam_f from %d (%s) has been truncated: %s\n", ent->s.number,
			ent->client->pers.netname, p);
	}

	// if not in TEAM gametypes and player has allies, use allychat (SAY_ALLY) instead of SAY_ALL
	if (sje_number_of_allies(ent) > 0)
		G_Say(ent, NULL, level.gametype >= GT_TEAM ? SAY_TEAM : SAY_ALLY, p);
	else
		G_Say(ent, NULL, level.gametype >= GT_TEAM ? SAY_TEAM : SAY_ALL, p);
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f(const gentity_t* ent)
{
	char arg[MAX_TOKEN_CHARS];

	if (trap->Argc() < 3)
	{
		trap->SendServerCommand(ent - g_entities, "print \"Usage: tell <player id> <message>\n\"");
		return;
	}

	trap->Argv(1, arg, sizeof arg);
	const int target_num = clientNumberFromString(ent, arg, qfalse);
	if (target_num == -1)
	{
		return;
	}

	const gentity_t* target = &g_entities[target_num];
	if (!target->inuse || !target->client)
	{
		return;
	}

	char* p = ConcatArgs(2);

	if (strlen(p) >= MAX_SAY_TEXT)
	{
		p[MAX_SAY_TEXT - 1] = '\0';
		G_SecurityLogPrintf("Cmd_Tell_f from %d (%s) has been truncated: %s\n", ent->s.number,
			ent->client->pers.netname, p);
	}

	G_LogPrintf("tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p);
	G_Say(ent, target, SAY_TELL, p);
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if (ent != target && !(ent->r.svFlags & SVF_BOT))
	{
		G_Say(ent, ent, SAY_TELL, p);
	}
}

//siege voice command
static void Cmd_VoiceCommand_f(gentity_t* ent)
{
	char arg[MAX_TOKEN_CHARS];
	char content[MAX_TOKEN_CHARS];
	int i = 0;

	strcpy(content, "");

	if (trap->Argc() < 2)
	{
		// other gamemodes will show info of how to use the voice_cmd
		if (level.gametype < GT_TEAM)
		{
			for (i = 0; i < MAX_CUSTOM_SIEGE_SOUNDS; i++)
			{
				strcpy(content, va("%s%s\n", content, bg_customSiegeSoundNames[i]));
			}
			trap->SendServerCommand(ent - g_entities,
				va(
					"print \"Usage: /voice_cmd <arg> [f]\nThe f argument is optional, it will make the command use female voice.\nThe arg may be one of the following:\n %s\"",
					content));
		}
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
		ent->client->tempSpectate >= level.time)
	{
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOICECHATASSPEC")));
		return;
	}

	trap->Argv(1, arg, sizeof arg);

	if (arg[0] == '*')
	{
		//hmm.. don't expect a * to be prepended already. maybe someone is trying to be sneaky.
		return;
	}

	const char* s = va("*%s", arg);

	//now, make sure it's a valid sound to be playing like this.. so people can't go around
	//screaming out death sounds or whatever.
	while (i < MAX_CUSTOM_SIEGE_SOUNDS)
	{
		if (!bg_customSiegeSoundNames[i])
		{
			break;
		}
		if (!Q_stricmp(bg_customSiegeSoundNames[i], s))
		{
			//it matches this one, so it's ok
			break;
		}
		i++;
	}

	if (i == MAX_CUSTOM_SIEGE_SOUNDS || !bg_customSiegeSoundNames[i])
	{
		//didn't find it in the list
		return;
	}

	if (level.gametype >= GT_TEAM)
	{
		gentity_t* te = G_TempEntity(vec3_origin, EV_VOICECMD_SOUND);
		te->s.groundEntityNum = ent->s.number;
		te->s.eventParm = G_SoundIndex((char*)bg_customSiegeSoundNames[i]);
		te->r.svFlags |= SVF_BROADCAST;
	}
	else
	{
		// in other gamemodes that are not Team ones, just do a G_Sound call to each allied player
		char voice_dir[32];

		strcpy(voice_dir, "mp_generic_male");

		if (trap->Argc() == 3)
		{
			char arg2[MAX_TOKEN_CHARS];
			trap->Argv(2, arg2, sizeof arg2);
			if (Q_stricmp(arg2, "f") == 0)
				strcpy(voice_dir, "mp_generic_female");
		}

		G_Sound(ent, CHAN_VOICE, G_SoundIndex(va("sound/chars/%s/misc/%s.mp3", voice_dir, arg)));

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (sje_is_ally(ent, &g_entities[i]) == qtrue)
			{
				trap->SendServerCommand(i, va("chat \"%s: ^3%s\"", ent->client->pers.netname, arg));
				G_Sound(&g_entities[i], CHAN_VOICE, G_SoundIndex(va("sound/chars/%s/misc/%s.mp3", voice_dir, arg)));
			}
		}
	}
}

static char* gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};
static size_t numgc_orders = ARRAY_LEN(gc_orders);

static void Cmd_GameCommand_f(const gentity_t* ent)
{
	char arg[MAX_TOKEN_CHARS] = { 0 };

	if (trap->Argc() != 3)
	{
		trap->SendServerCommand(ent - g_entities,
			va("print \"Usage: gc <player id> <order 0-%d>\n\"", numgc_orders - 1));
		return;
	}

	trap->Argv(2, arg, sizeof arg);
	const unsigned int order = atoi(arg);

	if (order >= numgc_orders)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"Bad order: %i\n\"", order));
		return;
	}

	trap->Argv(1, arg, sizeof arg);
	const int target_num = clientNumberFromString(ent, arg, qfalse);
	if (target_num == -1)
		return;

	const gentity_t* target = &g_entities[target_num];
	if (!target->inuse || !target->client)
		return;

	G_LogPrintf("tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, gc_orders[order]);
	G_Say(ent, target, SAY_TELL, gc_orders[order]);
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if (ent != target && !(ent->r.svFlags & SVF_BOT))
		G_Say(ent, ent, SAY_TELL, gc_orders[order]);
}

/*
==================
Cmd_Where_f
==================
*/
static void Cmd_Where_f(const gentity_t* ent)
{
	//JAC: This wasn't working for non-spectators since s.origin doesn't update for active players.
	if (ent->client && ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		//active players use currentOrigin
		trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", vtos(ent->r.currentOrigin)));
	}
	else
	{
		trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", vtos(ent->s.origin)));
	}
	//trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) );
}

static const char* gameNames[] = {
	"Free For All",
	"Holocron FFA",
	"Jedi Master",
	"Duel",
	"Power Duel",
	"Single Player",
	"Team FFA",
	"Siege",
	"Capture the Flag",
	"Capture the Ysalamiri"
};

/*
==================
Cmd_CallVote_f
==================
*/
extern void SiegeClearSwitchData(void); //g_saga.c

static qboolean G_VoteCapturelimit(gentity_t* ent, int num_args, const char* arg1, const char* arg2)
{
	const int n = Com_Clampi(0, 0x7FFFFFFF, atoi(arg2));
	Com_sprintf(level.voteString, sizeof level.voteString, "%s %i", arg1, n);
	Com_sprintf(level.voteDisplayString, sizeof level.voteDisplayString, "%s", level.voteString);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof level.voteStringClean);
	return qtrue;
}

static qboolean G_VoteClientkick(const gentity_t* ent, int num_args, const char* arg1, const char* arg2)
{
	const int n = atoi(arg2);

	if (n < 0 || n >= level.maxclients)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"invalid client number %d.\n\"", n));
		return qfalse;
	}

	if (g_entities[n].client->pers.connected == CON_DISCONNECTED)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"there is no client with the client number %d.\n\"", n));
		return qfalse;
	}

	Com_sprintf(level.voteString, sizeof level.voteString, "%s %s", arg1, arg2);
	Com_sprintf(level.voteDisplayString, sizeof level.voteDisplayString, "%s %s", arg1,
		g_entities[n].client->pers.netname);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof level.voteStringClean);
	return qtrue;
}

static qboolean G_VoteFraglimit(gentity_t* ent, int num_args, const char* arg1, const char* arg2)
{
	const int n = Com_Clampi(0, 0x7FFFFFFF, atoi(arg2));
	Com_sprintf(level.voteString, sizeof level.voteString, "%s %i", arg1, n);
	Com_sprintf(level.voteDisplayString, sizeof level.voteDisplayString, "%s", level.voteString);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof level.voteStringClean);
	return qtrue;
}

static qboolean G_VoteGametype(const gentity_t* ent, int num_args, const char* arg1, const char* arg2)
{
	int gt = atoi(arg2);

	// ffa, ctf, tdm, etc
	if (arg2[0] && isalpha(arg2[0]))
	{
		gt = BG_GetGametypeForString(arg2);
		if (gt == -1)
		{
			trap->SendServerCommand(ent - g_entities,
				va("print \"Gametype (%s) unrecognised, defaulting to FFA/Deathmatch\n\"", arg2));
			gt = GT_FFA;
		}
	}
	// numeric but out of range
	else if (gt < 0 || gt >= GT_MAX_GAME_TYPE)
	{
		trap->SendServerCommand(ent - g_entities,
			va("print \"Gametype (%i) is out of range, defaulting to FFA/Deathmatch\n\"", gt));
		gt = GT_FFA;
	}

	// logically invalid gametypes, or gametypes not fully implemented in MP
	if (gt == GT_SINGLE_PLAYER)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"This gametype is not supported (%s).\n\"", arg2));
		return qfalse;
	}

	level.votingGametype = qtrue;
	level.votingGametypeTo = gt;

	Com_sprintf(level.voteString, sizeof level.voteString, "%s %d", arg1, gt);
	Com_sprintf(level.voteDisplayString, sizeof level.voteDisplayString, "%s %s", arg1, gameNames[gt]);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof level.voteStringClean);
	return qtrue;
}

static qboolean G_VoteKick(const gentity_t* ent, int num_args, const char* arg1, const char* arg2)
{
	const int clientid = clientNumberFromString(ent, arg2, qtrue);

	if (clientid == -1)
		return qfalse;

	const gentity_t* target = &g_entities[clientid];
	if (!target || !target->inuse || !target->client)
		return qfalse;

	Com_sprintf(level.voteString, sizeof level.voteString, "clientkick %d", clientid);
	Com_sprintf(level.voteDisplayString, sizeof level.voteDisplayString, "kick %s", target->client->pers.netname);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof level.voteStringClean);
	return qtrue;
}

const char* G_GetArenaInfoByMap(const char* map);

static void Cmd_MapList_f(const gentity_t* ent)
{
	int toggle = 0;
	char map[24] = "--", buf[512] = { 0 };

	Q_strcat(buf, sizeof buf, "Map list:");

	for (int i = 0; i < level.arenas.num; i++)
	{
		Q_strncpyz(map, Info_ValueForKey(level.arenas.infos[i], "map"), sizeof map);
		Q_StripColor(map);

		if (G_DoesMapSupportGametype(map, level.gametype))
		{
			const char* tmpMsg = va(" ^%c%s", ++toggle & 1 ? COLOR_GREEN : COLOR_YELLOW, map);
			if (strlen(buf) + strlen(tmpMsg) >= sizeof buf)
			{
				trap->SendServerCommand(ent - g_entities, va("print \"%s\"", buf));
				buf[0] = '\0';
			}
			Q_strcat(buf, sizeof buf, tmpMsg);
		}
	}

	trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));
}

static qboolean G_VoteMap(const gentity_t* ent, const int num_args, const char* arg1, const char* arg2)
{
	char s[MAX_CVAR_VALUE_STRING] = { 0 }, bsp_name[MAX_QPATH] = { 0 }, * map_name = NULL, * map_name2 = NULL;
	fileHandle_t fp = NULL_FILE;

	// didn't specify a map, show available maps
	if (num_args < 3)
	{
		Cmd_MapList_f(ent);
		return qfalse;
	}

	if (strchr(arg2, '\\'))
	{
		trap->SendServerCommand(ent - g_entities, "print \"Can't have mapnames with a \\\n\"");
		return qfalse;
	}

	Com_sprintf(bsp_name, sizeof bsp_name, "maps/%s.bsp", arg2);
	if (trap->FS_Open(bsp_name, &fp, FS_READ) <= 0)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"Can't find map %s on server\n\"", bsp_name));
		if (fp != NULL_FILE)
			trap->FS_Close(fp);
		return qfalse;
	}
	trap->FS_Close(fp);

	if (!G_DoesMapSupportGametype(arg2, level.gametype))
	{
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME")));
		return qfalse;
	}

	// preserve the map rotation
	trap->Cvar_VariableStringBuffer("nextmap", s, sizeof s);
	if (*s)
		Com_sprintf(level.voteString, sizeof level.voteString, "%s %s; set nextmap \"%s\"", arg1, arg2, s);
	else
		Com_sprintf(level.voteString, sizeof level.voteString, "%s %s", arg1, arg2);

	const char* arena_info = G_GetArenaInfoByMap(arg2);
	if (arena_info)
	{
		map_name = Info_ValueForKey(arena_info, "longname");
		map_name2 = Info_ValueForKey(arena_info, "map");
	}

	if (!map_name || !map_name[0])
		map_name = "ERROR";

	if (!map_name2 || !map_name2[0])
		map_name2 = "ERROR";

	Com_sprintf(level.voteDisplayString, sizeof level.voteDisplayString, "map %s (%s)", map_name, map_name2);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof level.voteStringClean);
	return qtrue;
}

static qboolean G_VoteMapRestart(gentity_t* ent, const int num_args, const char* arg1, const char* arg2)
{
	int n = Com_Clampi(0, 60, atoi(arg2));
	if (num_args < 3)
		n = 5;
	Com_sprintf(level.voteString, sizeof level.voteString, "%s %i", arg1, n);
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof level.voteDisplayString);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof level.voteStringClean);
	return qtrue;
}

static qboolean G_VoteNextmap(const gentity_t* ent, int num_args, const char* arg1, const char* arg2)
{
	char s[MAX_CVAR_VALUE_STRING];

	trap->Cvar_VariableStringBuffer("nextmap", s, sizeof s);
	if (!*s)
	{
		trap->SendServerCommand(ent - g_entities, "print \"nextmap not set.\n\"");
		return qfalse;
	}
	SiegeClearSwitchData();
	Com_sprintf(level.voteString, sizeof level.voteString, "vstr nextmap");
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof level.voteDisplayString);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof level.voteStringClean);
	return qtrue;
}

static qboolean G_VoteTimelimit(gentity_t* ent, int num_args, const char* arg1, const char* arg2)
{
	const float tl = Com_Clamp(0.0f, 35790.0f, atof(arg2));
	if (Q_isintegral(tl))
		Com_sprintf(level.voteString, sizeof level.voteString, "%s %i", arg1, (int)tl);
	else
		Com_sprintf(level.voteString, sizeof level.voteString, "%s %.3f", arg1, tl);
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof level.voteDisplayString);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof level.voteStringClean);
	return qtrue;
}

static qboolean G_VoteWarmup(gentity_t* ent, int num_args, const char* arg1, const char* arg2)
{
	const int n = Com_Clampi(0, 1, atoi(arg2));
	Com_sprintf(level.voteString, sizeof level.voteString, "%s %i", arg1, n);
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof level.voteDisplayString);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof level.voteStringClean);
	return qtrue;
}

typedef struct voteString_s
{
	const char* string;
	const char* aliases; // space delimited list of aliases, will always show the real vote string
	qboolean(*func)(gentity_t* ent, int num_args, const char* arg1, const char* arg2);
	int num_args; // number of REQUIRED arguments, not total/optional arguments
	uint32_t validGT; // bit-flag of valid gametypes
	qboolean voteDelay; // if true, will delay executing the vote string after it's accepted by g_voteDelay
	const char* shortHelp; // NULL if no arguments needed
} voteString_t;

static voteString_t validVoteStrings[] = {
	//	vote string				aliases										# args	valid gametypes							exec delay		short help
	{"capturelimit", "caps", G_VoteCapturelimit, 1, GTB_CTF | GTB_CTY, qtrue, "<num>"},
	{"clientkick", NULL, G_VoteClientkick, 1, GTB_ALL, qfalse, "<clientNum>"},
	{"fraglimit", "frags", G_VoteFraglimit, 1, GTB_ALL & ~(GTB_SIEGE | GTB_CTF | GTB_CTY), qtrue, "<num>"},
	{"g_doWarmup", "dowarmup warmup", G_VoteWarmup, 1, GTB_ALL, qtrue, "<0-1>"},
	{"g_gametype", "gametype gt mode", G_VoteGametype, 1, GTB_ALL, qtrue, "<num or name>"},
	{"kick", NULL, G_VoteKick, 1, GTB_ALL, qfalse, "<client name>"},
	{"map", NULL, G_VoteMap, 0, GTB_ALL, qtrue, "<name>"},
	{"map_restart", "restart", G_VoteMapRestart, 0, GTB_ALL, qtrue, "<optional delay>"},
	{"nextmap", NULL, G_VoteNextmap, 0, GTB_ALL, qtrue, NULL},
	{"timelimit", "time", G_VoteTimelimit, 1, GTB_ALL & ~GTB_SIEGE, qtrue, "<num>"},
};
static const int validVoteStringsSize = ARRAY_LEN(validVoteStrings);

void Svcmd_ToggleAllowVote_f(void)
{
	if (trap->Argc() == 1)
	{
		for (int i = 0; i < validVoteStringsSize; i++)
		{
			if (g_allowVote.integer & 1 << i) trap->Print("%2d [X] %s\n", i, validVoteStrings[i].string);
			else trap->Print("%2d [ ] %s\n", i, validVoteStrings[i].string);
		}
		return;
	}
	char arg[8] = { 0 };

	trap->Argv(1, arg, sizeof arg);
	const int index = atoi(arg);

	if (index < 0 || index >= validVoteStringsSize)
	{
		Com_Printf("ToggleAllowVote: Invalid range: %i [0, %i]\n", index, validVoteStringsSize - 1);
		return;
	}

	trap->Cvar_Set("g_allowVote", va("%i", 1 << index ^ g_allowVote.integer & (1 << validVoteStringsSize) - 1));
	trap->Cvar_Update(&g_allowVote);

	Com_Printf("%s %s^7\n", validVoteStrings[index].string,
		g_allowVote.integer & 1 << index ? "^2Enabled" : "^1Disabled");
}

static void CG_AdminMenu(gentity_t* ent)
{
	if (!(ent->r.svFlags & SVF_ADMIN))
	{
		trap->SendServerCommand(ent - g_entities, "print \"Must login with /adminlogin (password)\n\"");
		return;
	}

	trap->SendServerCommand(ent - g_entities, "openadminmenu");
}

static void Cmd_ChangeMap(gentity_t* ent)
{
	char   arg1[MAX_STRING_CHARS];

	if (!(ent->r.svFlags & SVF_ADMIN))
	{
		trap->SendServerCommand(ent - g_entities, "print \"Must login with /adminlogin (password)\n\"");
		return;
	}

	trap->Argv(1, arg1, sizeof(arg1));
	trap->SendConsoleCommand(EXEC_APPEND, va("g_gametype %s\n", arg1));
	G_LogPrintf("ChangeMap admin command executed by SERVER to GAMETYPE:%s", arg1);
	trap->Argv(2, arg1, sizeof(arg1));
	trap->SendConsoleCommand(EXEC_APPEND, va("map %s\n", arg1));
	G_LogPrintf(" MAP:%s.\n", arg1);
}

static void Cmd_Punish(gentity_t* ent)
{
	int client_id = -1;
	char   arg1[MAX_STRING_CHARS];

	if (!(ent->r.svFlags & SVF_ADMIN))
	{
		trap->SendServerCommand(ent - g_entities, "print \"Must login with /adminlogin (password)\n\"");
		return;
	}

	trap->Argv(1, arg1, sizeof(arg1));
	client_id = G_ClientNumberFromArg(arg1);

	if (client_id == -1)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"Can't find client ID for %s\n\"", arg1));
		return;
	}
	if (client_id == -2)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"Ambiguous client ID for %s\n\"", arg1));
		return;
	}
	if (client_id >= MAX_CLIENTS || client_id < 0)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"Bad client ID for %s\n\"", arg1));
		return;
	}
	// either we have the client id or the string did not match
	if (!g_entities[client_id].inuse)
	{ // check to make sure client slot is in use
		trap->SendServerCommand(ent - g_entities, va("print \"Client %s is not active\n\"", arg1));
		return;
	}
	if (g_entities[client_id].client->ps.duelInProgress == 1)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"Client is currently dueling. Please wait for them to finish.\n\""));
		return;
	}

	if (g_entities[client_id].health > 0)
	{
		gentity_t* kEnt = &g_entities[client_id];

		G_ScreenShake(g_entities[client_id].client->ps.origin, &g_entities[client_id], 3.0f, 2000, qtrue);

		G_LogPrintf("Punishment admin command executed by %s on %s.\n", ent->client->pers.netname, g_entities[client_id].client->pers.netname);

		trap->SendServerCommand(-1, va("cp \"%s^7\n%s\n\"", g_entities[client_id].client->pers.netname, g_adminpunishment_saying.string));

		g_entities[client_id].client->ps.velocity[2] += 1000;
		g_entities[client_id].client->pers.isbeingpunished = 1;
		g_entities[client_id].client->ps.forceDodgeAnim = 0;
		g_entities[client_id].client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
		g_entities[client_id].client->ps.forceHandExtendTime = level.time + Q3_INFINITE;
		g_entities[client_id].client->ps.quickerGetup = qfalse;

		if (kEnt->inuse && kEnt->client)
		{
			g_entities[client_id].flags &= ~FL_GODMODE;
			g_entities[client_id].client->ps.stats[STAT_HEALTH] = kEnt->health = -99999;
			player_die(kEnt, kEnt, kEnt, 100000, MOD_SUICIDE);
		}
	}
	else
	{
		return;
	}
}

static void Cmd_Kick(gentity_t* ent)
{
	int client_id = -1;
	char   arg1[MAX_STRING_CHARS];

	if (!(ent->r.svFlags & SVF_ADMIN))
	{
		trap->SendServerCommand(ent - g_entities, "print \"Must login with /adminlogin (password)\n\"");
		return;
	}

	trap->Argv(1, arg1, sizeof(arg1));
	client_id = G_ClientNumberFromArg(arg1);

	if (client_id == -1)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"Can't find client ID for %s\n\"", arg1));
		return;
	}
	if (client_id == -2)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"Ambiguous client ID for %s\n\"", arg1));
		return;
	}
	if (client_id >= MAX_CLIENTS || client_id < 0)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"Bad client ID for %s\n\"", arg1));
		return;
	}
	// either we have the client id or the string did not match
	if (!g_entities[client_id].inuse)
	{ // check to make sure client slot is in use
		trap->SendServerCommand(ent - g_entities, va("print \"Client %s is not active\n\"", arg1));
		return;
	}
	if (client_id == ent->client->ps.clientNum)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"You cant kick yourself.\n\""));
		return;
	}
	if (g_entities[client_id].client->sess.spectatorState == SPECTATOR_FOLLOW)
	{
		g_entities[client_id].client->sess.spectatorState = SPECTATOR_FREE;
	}
	trap->DropClient(client_id, "was Kicked");

	G_LogPrintf("Kick command executed by %s on %s.\n", ent->client->pers.netname, g_entities[client_id].client->pers.netname);
}

static void Cmd_AdminLogin(gentity_t* ent)
{
	char   password[MAX_STRING_CHARS];

	trap->Cvar_Set("cl_noprint", "0");

	if (trap->Argc() != 2)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"Usage: /adminlogin <password>\n\""));
		return;
	}

	if (ent->r.svFlags & SVF_ADMIN)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"You are already logged in. Type in /adminlogout to remove admin status.\n\""));
		return;
	}

	trap->Argv(1, password, sizeof(password)); // password

	if (ent->client->pers.plugindetect == qtrue)
	{
		if (strcmp(password, g_adminpassword.string) == 0)
		{
			strcpy(ent->client->pers.login, g_adminlogin_saying.string);
			strcpy(ent->client->pers.logout, g_adminlogout_saying.string);

			ent->client->pers.iamanadmin = 1;

			ent->r.svFlags |= SVF_ADMIN;

			G_LogPrintf("%s %s\n", ent->client->pers.netname, ent->client->pers.login);

			trap->SendServerCommand(-1, va("print \"%s ^7%s\n\"", ent->client->pers.netname, ent->client->pers.login));

			Com_Printf("-----Now type Adminmenu to see admin commands ----------\n");
			Com_Printf("--------------------------------------------------------\n");
			Com_Printf("-----Adminlogout---To log out as admin------------------\n");
			Com_Printf("--------------------------------------------------------\n");
			Com_Printf("-----Adminchangemap <Map name>--To change the map-------\n");
			Com_Printf("--------------------------------------------------------\n");
			Com_Printf("-----Adminpunish <Player name>--To punish the player----\n");
			Com_Printf("--------------------------------------------------------\n");
			Com_Printf("-----Adminkick <Player name>----To kick the player------\n");
			Com_Printf("--------------------------------------------------------\n");
			Com_Printf("-----Adminnpc spawn <NPC name>--To spawn an npc---------\n");
			Com_Printf("--------------------------------------------------------\n");
			Com_Printf("-----Type seta g_cheatoverride 1 to override cheats-----\n");
			Com_Printf("--------------------------------------------------------\n");
		}
		else
		{
			trap->SendServerCommand(ent - g_entities, va("print \"Incorrect password.\n\""));
		}
	}
	else
	{
		trap->SendServerCommand(ent - g_entities, va("cp \"^1You are not qualified to be admin.\n\""));
	}
}

static void Cmd_AdminLogout(gentity_t* ent)
{
	if (ent->r.svFlags & SVF_ADMIN)
	{
		G_LogPrintf("%s %s\n", ent->client->pers.netname, ent->client->pers.logout);

		trap->SendServerCommand(-1, va("print \"%s ^7%s\n\"", ent->client->pers.netname, ent->client->pers.logout));

		ent->client->pers.iamanadmin = 0;
		ent->r.svFlags &= ~SVF_ADMIN;
		ent->client->pers.bitvalue = 0;
	}
}

static void Cmd_CallVote_f(gentity_t* ent)
{
	int i;
	char arg1[MAX_CVAR_VALUE_STRING] = { 0 };
	char arg2[MAX_CVAR_VALUE_STRING] = { 0 };
	voteString_t* vote = NULL;

	// not allowed to vote at all
	if (!g_allowVote.integer)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE")));
		return;
	}

	// vote in progress
	if (level.voteTime)
	{
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEINPROGRESS")));
		return;
	}

	// can't vote as a spectator, except in (power)duel
	if (level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL && ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSPECVOTE")));
		return;
	}

	// make sure it is a valid command to vote on
	const int num_args = trap->Argc();
	trap->Argv(1, arg1, sizeof arg1);
	if (num_args > 1)
		Q_strncpyz(arg2, ConcatArgs(2), sizeof arg2);

	// filter ; \n \r
	if (Q_strchrs(arg1, ";\r\n") || Q_strchrs(arg2, ";\r\n"))
	{
		trap->SendServerCommand(ent - g_entities, "print \"Invalid vote string.\n\"");
		return;
	}

	// check for invalid votes
	for (i = 0; i < validVoteStringsSize; i++)
	{
		if (!(g_allowVote.integer & 1 << i))
			continue;

		if (!Q_stricmp(arg1, validVoteStrings[i].string))
			break;

		// see if they're using an alias, and set arg1 to the actual vote string
		if (validVoteStrings[i].aliases)
		{
			char tmp[MAX_TOKEN_CHARS] = { 0 };
			const char* delim = " ";
			Q_strncpyz(tmp, validVoteStrings[i].aliases, sizeof tmp);
			const char* p = strtok(tmp, delim);
			while (p != NULL)
			{
				if (!Q_stricmp(arg1, p))
				{
					Q_strncpyz(arg1, validVoteStrings[i].string, sizeof arg1);
					goto validVote;
				}
				p = strtok(NULL, delim);
			}
		}
	}
	// invalid vote string, abandon ship
	if (i == validVoteStringsSize)
	{
		char buf[1024] = { 0 };
		int toggle = 0;
		trap->SendServerCommand(ent - g_entities, "print \"Invalid vote string.\n\"");
		trap->SendServerCommand(ent - g_entities, "print \"Allowed vote strings are: \"");
		for (i = 0; i < validVoteStringsSize; i++)
		{
			if (!(g_allowVote.integer & 1 << i))
				continue;

			toggle = !toggle;
			if (validVoteStrings[i].shortHelp)
			{
				Q_strcat(buf, sizeof buf, va("^%c%s %s ",
					toggle ? COLOR_GREEN : COLOR_YELLOW,
					validVoteStrings[i].string,
					validVoteStrings[i].shortHelp));
			}
			else
			{
				Q_strcat(buf, sizeof buf, va("^%c%s ",
					toggle ? COLOR_GREEN : COLOR_YELLOW,
					validVoteStrings[i].string));
			}
		}

		//FIXME: buffer and send in multiple messages in case of overflow
		trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));
		return;
	}

validVote:
	vote = &validVoteStrings[i];

	if (!(vote->validGT & 1 << level.gametype))
	{
		trap->SendServerCommand(ent - g_entities, va("print \"%s is not applicable in this gametype.\n\"", arg1));
		return;
	}

	if (num_args < vote->num_args + 2)
	{
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s requires more arguments: %s\n\"", arg1, vote->shortHelp));
		return;
	}

	level.votingGametype = qfalse;

	level.voteExecuteDelay = vote->voteDelay ? g_voteDelay.integer : 0;

	// there is still a vote to be executed, execute it and store the new vote
	if (level.voteExecuteTime)
	{
		level.voteExecuteTime = 0;
		trap->SendConsoleCommand(EXEC_APPEND, va("%s\n", level.voteString));
	}

	// pass the args onto vote-specific handlers for parsing/filtering
	if (vote->func)
	{
		if (!vote->func(ent, num_args, arg1, arg2))
			return;
	}
	// otherwise assume it's a command
	else
	{
		Com_sprintf(level.voteString, sizeof level.voteString, "%s \"%s\"", arg1, arg2);
		Q_strncpyz(level.voteDisplayString, level.voteString, sizeof level.voteDisplayString);
		Q_strncpyz(level.voteStringClean, level.voteString, sizeof level.voteStringClean);
	}
	Q_strstrip(level.voteStringClean, "\"\n\r", NULL);

	trap->SendServerCommand(-1, va("print \"%s^7 %s (%s)\n\"", ent->client->pers.netname,
		G_GetStringEdString("MP_SVGAME", "PLCALLEDVOTE"), level.voteStringClean));

	// start the voting, the caller automatically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;

	for (i = 0; i < level.maxclients; i++)
	{
		level.clients[i].mGameFlags &= ~PSG_VOTED;
		level.clients[i].pers.vote = 0;
	}

	ent->client->mGameFlags |= PSG_VOTED;
	ent->client->pers.vote = 1;

	trap->SetConfigstring(CS_VOTE_TIME, va("%i", level.voteTime));
	trap->SetConfigstring(CS_VOTE_STRING, level.voteDisplayString);
	trap->SetConfigstring(CS_VOTE_YES, va("%i", level.voteYes));
	trap->SetConfigstring(CS_VOTE_NO, va("%i", level.voteNo));
}

/*
==================
Cmd_AllyChat_f
==================
*/
static void Cmd_AllyChat_f(const gentity_t* ent)
{
	// allows chatting with allies
	if (trap->Argc() < 2)
		return;

	char* p = ConcatArgs(1);

	if (strlen(p) >= MAX_SAY_TEXT)
	{
		p[MAX_SAY_TEXT - 1] = '\0';
		G_SecurityLogPrintf("Cmd_AllyChat_f from %d (%s) has been truncated: %s\n", ent->s.number,
			ent->client->pers.netname, p);
	}

	G_Say(ent, NULL, SAY_ALLY, p);
}

/*
==================
Cmd_Vote_f
==================
*/
static void Cmd_Vote_f(const gentity_t* ent)
{
	char msg[64] = { 0 };

	if (!level.voteTime)
	{
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEINPROG")));
		return;
	}
	if (ent->client->mGameFlags & PSG_VOTED)
	{
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEALREADY")));
		return;
	}
	if (level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL)
	{
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			trap->SendServerCommand(ent - g_entities,
				va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEASSPEC")));
			return;
		}
	}

	trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PLVOTECAST")));

	ent->client->mGameFlags |= PSG_VOTED;

	trap->Argv(1, msg, sizeof msg);

	if (tolower(msg[0]) == 'y' || msg[0] == '1')
	{
		level.voteYes++;
		ent->client->pers.vote = 1;
		trap->SetConfigstring(CS_VOTE_YES, va("%i", level.voteYes));
	}
	else
	{
		level.voteNo++;
		ent->client->pers.vote = 2;
		trap->SetConfigstring(CS_VOTE_NO, va("%i", level.voteNo));
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

static qboolean G_TeamVoteLeader(const gentity_t* ent, const int cs_offset, const team_t team, const int num_args,
	const char* arg2)
{
	const int clientid = num_args == 2 ? ent->s.number : clientNumberFromString(ent, arg2, qfalse);

	if (clientid == -1)
		return qfalse;

	const gentity_t* target = &g_entities[clientid];
	if (!target || !target->inuse || !target->client)
		return qfalse;

	if (target->client->sess.sessionTeam != team)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"User %s is not on your team\n\"", arg2));
		return qfalse;
	}

	Com_sprintf(level.teamVoteString[cs_offset], sizeof level.teamVoteString[cs_offset], "leader %d", clientid);
	Q_strncpyz(level.teamVoteDisplayString[cs_offset], level.teamVoteString[cs_offset],
		sizeof level.teamVoteDisplayString[cs_offset]);
	Q_strncpyz(level.teamVoteStringClean[cs_offset], level.teamVoteString[cs_offset],
		sizeof level.teamVoteStringClean[cs_offset]);
	return qtrue;
}

/*
==================
Cmd_CallTeamVote_f
==================
*/
static void Cmd_CallTeamVote_f(const gentity_t* ent)
{
	const team_t team = ent->client->sess.sessionTeam;
	int i, cs_offset;
	char arg1[MAX_CVAR_VALUE_STRING] = { 0 };
	char arg2[MAX_CVAR_VALUE_STRING] = { 0 };

	if (team == TEAM_RED)
		cs_offset = 0;
	else if (team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	// not allowed to vote at all
	if (!g_allowTeamVote.integer)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE")));
		return;
	}

	// vote in progress
	if (level.teamVoteTime[cs_offset])
	{
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEALREADY")));
		return;
	}

	// can't vote as a spectator
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSPECVOTE")));
		return;
	}

	// make sure it is a valid command to vote on
	const int num_args = trap->Argc();
	trap->Argv(1, arg1, sizeof arg1);
	if (num_args > 1)
		Q_strncpyz(arg2, ConcatArgs(2), sizeof arg2);

	// filter ; \n \r
	if (Q_strchrs(arg1, ";\r\n") || Q_strchrs(arg2, ";\r\n"))
	{
		trap->SendServerCommand(ent - g_entities, "print \"Invalid team vote string.\n\"");
		return;
	}

	// pass the args onto vote-specific handlers for parsing/filtering
	if (!Q_stricmp(arg1, "leader"))
	{
		if (!G_TeamVoteLeader(ent, cs_offset, team, num_args, arg2))
			return;
	}
	else
	{
		trap->SendServerCommand(ent - g_entities, "print \"Invalid team vote string.\n\"");
		trap->SendServerCommand(ent - g_entities, va("print \"Allowed team vote strings are: ^%c%s %s\n\"", COLOR_GREEN,
			"leader", "<optional client name or number>"));
		return;
	}

	Q_strstrip(level.teamVoteStringClean[cs_offset], "\"\n\r", NULL);

	for (i = 0; i < level.maxclients; i++)
	{
		if (level.clients[i].pers.connected == CON_DISCONNECTED)
			continue;
		if (level.clients[i].sess.sessionTeam == team)
			trap->SendServerCommand(i, va("print \"%s^7 called a team vote (%s)\n\"", ent->client->pers.netname,
				level.teamVoteStringClean[cs_offset]));
	}

	// start the voting, the caller autoamtically votes yes
	level.teamVoteTime[cs_offset] = level.time;
	level.teamVoteYes[cs_offset] = 1;
	level.teamVoteNo[cs_offset] = 0;

	for (i = 0; i < level.maxclients; i++)
	{
		if (level.clients[i].pers.connected == CON_DISCONNECTED)
			continue;
		if (level.clients[i].sess.sessionTeam == team)
		{
			level.clients[i].mGameFlags &= ~PSG_TEAMVOTED;
			level.clients[i].pers.teamvote = 0;
		}
	}
	ent->client->mGameFlags |= PSG_TEAMVOTED;
	ent->client->pers.teamvote = 1;

	trap->SetConfigstring(CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamVoteTime[cs_offset]));
	trap->SetConfigstring(CS_TEAMVOTE_STRING + cs_offset, level.teamVoteDisplayString[cs_offset]);
	trap->SetConfigstring(CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset]));
	trap->SetConfigstring(CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset]));
}

/*
==================
Cmd_TeamVote_f
==================
*/
static void Cmd_TeamVote_f(const gentity_t* ent)
{
	const team_t team = ent->client->sess.sessionTeam;
	int cs_offset;
	char msg[64] = { 0 };

	if (team == TEAM_RED)
		cs_offset = 0;
	else if (team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if (!level.teamVoteTime[cs_offset])
	{
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOTEAMVOTEINPROG")));
		return;
	}
	if (ent->client->mGameFlags & PSG_TEAMVOTED)
	{
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEALREADYCAST")));
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEASSPEC")));
		return;
	}

	trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PLTEAMVOTECAST")));

	ent->client->mGameFlags |= PSG_TEAMVOTED;

	trap->Argv(1, msg, sizeof msg);

	if (tolower(msg[0]) == 'y' || msg[0] == '1')
	{
		level.teamVoteYes[cs_offset]++;
		ent->client->pers.teamvote = 1;
		trap->SetConfigstring(CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset]));
	}
	else
	{
		level.teamVoteNo[cs_offset]++;
		ent->client->pers.teamvote = 2;
		trap->SetConfigstring(CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset]));
	}

	// a majority will be determined in TeamCheckVote, which will also account
	// for players entering or leaving
}

/*
=================
Cmd_SetViewpos_f
=================
*/
static void Cmd_SetViewpos_f(gentity_t* ent)
{
	vec3_t origin, angles;
	char buffer[MAX_TOKEN_CHARS];

	if (trap->Argc() != 5)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear(angles);
	for (int i = 0; i < 3; i++)
	{
		trap->Argv(i + 1, buffer, sizeof buffer);
		origin[i] = atof(buffer);
	}

	trap->Argv(4, buffer, sizeof buffer);
	angles[YAW] = atof(buffer);

	TeleportPlayer(ent, origin, angles);
}

void G_LeaveVehicle(gentity_t* ent, const qboolean con_check)
{
	if (ent->client->ps.m_iVehicleNum)
	{
		//tell it I'm getting off
		const gentity_t* veh = &g_entities[ent->client->ps.m_iVehicleNum];

		if (veh->inuse && veh->client && veh->m_pVehicle)
		{
			if (con_check)
			{
				// check connection
				const clientConnected_t pCon = ent->client->pers.connected;
				ent->client->pers.connected = CON_DISCONNECTED;
				veh->m_pVehicle->m_pVehicleInfo->Eject(veh->m_pVehicle, (bgEntity_t*)ent, qtrue);
				ent->client->pers.connected = pCon;
			}
			else
			{
				// or not.
				veh->m_pVehicle->m_pVehicleInfo->Eject(veh->m_pVehicle, (bgEntity_t*)ent, qtrue);
			}
		}
	}

	ent->client->ps.m_iVehicleNum = 0;
}

int G_ItemUsable(const playerState_t* ps, int forcedUse)
{
	vec3_t fwd, fwdorg, dest;
	vec3_t yawonly;
	vec3_t mins, maxs;
	vec3_t trtest;
	trace_t tr;

	// fix: dead players shouldn't use items
	if (ps->stats[STAT_HEALTH] <= 0)
	{
		return 0;
	}

	if (ps->m_iVehicleNum)
	{
		return 0;
	}

	if (ps->pm_flags & PMF_USE_ITEM_HELD)
	{
		//force to let go first
		return 0;
	}

	if (!forcedUse)
	{
		forcedUse = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	}

	if (!BG_IsItemSelectable(forcedUse))
	{
		return 0;
	}

	switch (forcedUse)
	{
	case HI_SEEKER:
		if (ps->eFlags & EF_SEEKERDRONE)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SEEKER_ALREADYDEPLOYED);
			return 0;
		}

		return 1;
	case HI_SHIELD:
		mins[0] = -8;
		mins[1] = -8;
		mins[2] = 0;

		maxs[0] = 8;
		maxs[1] = 8;
		maxs[2] = 8;

		AngleVectors(ps->viewangles, fwd, NULL, NULL);
		fwd[2] = 0;
		VectorMA(ps->origin, 64, fwd, dest);
		trap->Trace(&tr, ps->origin, mins, maxs, dest, ps->clientNum, MASK_SHOT, qfalse, 0, 0);
		if (tr.fraction > 0.9 && !tr.startsolid && !tr.allsolid)
		{
			vec3_t pos;
			VectorCopy(tr.endpos, pos);
			VectorSet(dest, pos[0], pos[1], pos[2] - 4096);
			trap->Trace(&tr, pos, mins, maxs, dest, ps->clientNum, MASK_SOLID, qfalse, 0, 0);
			if (!tr.startsolid && !tr.allsolid)
			{
				return 1;
			}
		}
		G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SHIELD_NOROOM);
		return 0;
	case HI_MEDPAC:
	case HI_MEDPAC_BIG:
		if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
		{
			return 0;
		}

		if (ps->stats[STAT_HEALTH] <= 0)
		{
			return 0;
		}

		return 1;
	case HI_BINOCULARS:
		return 1;
	case HI_SENTRY_GUN:
		if (ps->fd.sentryDeployed)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_ALREADYPLACED);
			return 0;
		}

		yawonly[ROLL] = 0;
		yawonly[PITCH] = 0;
		yawonly[YAW] = ps->viewangles[YAW];

		VectorSet(mins, -8, -8, 0);
		VectorSet(maxs, 8, 8, 24);

		AngleVectors(yawonly, fwd, NULL, NULL);

		fwdorg[0] = ps->origin[0] + fwd[0] * 64;
		fwdorg[1] = ps->origin[1] + fwd[1] * 64;
		fwdorg[2] = ps->origin[2] + fwd[2] * 64;

		trtest[0] = fwdorg[0] + fwd[0] * 16;
		trtest[1] = fwdorg[1] + fwd[1] * 16;
		trtest[2] = fwdorg[2] + fwd[2] * 16;

		trap->Trace(&tr, ps->origin, mins, maxs, trtest, ps->clientNum, MASK_PLAYERSOLID, qfalse, 0, 0);

		if (tr.fraction != 1 && tr.entityNum != ps->clientNum || tr.startsolid || tr.allsolid)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_NOROOM);
			return 0;
		}

		return 1;
	case HI_JETPACK: //done dont show
		return 1;
	case HI_HEALTHDISP:
		return 1;
	case HI_AMMODISP:
		return 1;
	case HI_EWEB:
		return 1;
	case HI_CLOAK:
		return 1;
	case HI_FLAMETHROWER:
		return 1;
		return 1;
	case HI_SWOOP:
		return 1;
	case HI_DROIDEKA:
		return 1;
	case HI_SPHERESHIELD:
		return 1;
	case HI_GRAPPLE: //done dont show
		return 1;
	default:
		return 1;
	}
}

void Cmd_ToggleSaber_f(gentity_t* ent)
{
	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}

	if (level.intermissiontime)
	{
		// not during intermission
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		// not when spec
		return;
	}

	if (ent->client->tempSpectate >= level.time)
	{
		// not when tempSpec
		return;
	}

	if (ent->client->ps.emplacedIndex)
	{
		//on an emplaced gun
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
	}

	if (ent->client->ps.fd.forceGripCripple)
	{
		//if they are being gripped, don't let them unholster their saber
		if (ent->client->ps.saberHolstered)
		{
			return;
		}
	}

	if (ent->client->ps.saberInFlight)
	{
		if (ent->client->ps.saberEntityNum)
		{
			//our saber is dead, Try pulling it back.
			ent->client->ps.forceHandExtend = HANDEXTEND_SABERPULL;
			ent->client->ps.forceHandExtendTime = level.time + 300;
		}
		return;
	}

	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (ent->client->ps.duelTime >= level.time)
	{
		return;
	}

	if (ent->client->ps.saberLockTime >= level.time)
	{
		return;
	}

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

qboolean G_ValidSaberStyle(const gentity_t* ent, const int saber_style)
{
	if (saber_style == SS_MEDIUM
		&& !(ent->client->saber[0].type == SABER_BACKHAND
			|| ent->client->saber[0].type == SABER_ASBACKHAND))
	{
		//SS_YELLOW is the default and always valid
		return qtrue;
	}

	if ((ent->client->saber[0].type == SABER_BACKHAND) && (saber_style != SS_STAFF))
	{
		return qfalse;
	}
	if ((ent->client->saber[0].type == SABER_ASBACKHAND) && (saber_style != SS_STAFF))
	{
		return qfalse;
	}
	//otherwise, check to see if the player has the skill to use this style
	switch (saber_style)
	{
	case SS_FAST:
		return qtrue;
	default:
		return qtrue;
	}
}

extern qboolean PM_InKnockDown(const playerState_t* ps);
extern qboolean PM_InSlapDown(const playerState_t* ps);
extern qboolean PM_InRoll(const playerState_t* ps);
void Cmd_SaberAttackCycle_f(gentity_t* ent)
{
	int select_level = 0;
	qboolean usingSiegeStyle = qfalse;

	if (!ent || !ent->client)
	{
		return;
	}

	if (level.intermissionQueued || level.intermissiontime)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"%s (saberAttackCycle)\n\"", G_GetStringEdString("MP_SVGAME", "CANNOT_TASK_INTERMISSION")));
		return;
	}

	if (ent->health <= 0
		|| ent->client->tempSpectate >= level.time
		|| ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBEALIVE")));
		return;
	}

	if (ent->health <= 0
		|| ent->client->tempSpectate >= level.time
		|| ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		//not for spectators
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
	}

	if (ent->r.svFlags & SVF_BOT)
	{
		if (ent->client->ps.weapon == WP_SABER && ent->client->ps.saberHolstered == 0)
		{
			return;
		}

		if (ent->client->ps.weapon == WP_SABER && ent->client->ps.saberInFlight)
		{
			//saber not currently in use or available.
			return;
		}

		if (PM_InKnockDown(&ent->client->ps) || PM_InSlapDown(&ent->client->ps) || PM_InRoll(&ent->client->ps))
		{
			return;
		}
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}

	if ((ent->client->saber[0].type == SABER_BACKHAND) && ent->client->ps.fd.saberAnimLevel == SS_STAFF)
	{
		return;
	}

	if ((ent->client->saber[0].type == SABER_ASBACKHAND) && ent->client->ps.fd.saberAnimLevel == SS_STAFF)
	{
		return;
	}

	if ((ent->client->saber[0].type == SABER_STAFF_MAUL) && ent->client->ps.fd.saberAnimLevel == SS_STAFF)
	{
		return;
	}

	if ((ent->client->saber[0].type == SABER_ELECTROSTAFF) && ent->client->ps.fd.saberAnimLevel == SS_STAFF)
	{
		return;
	}

	if (ent->client->saber[0].model[0] && ent->client->saber[1].model[0])
	{ //no cycling for akimbo
		if (WP_SaberCanTurnOffSomeBlades(&ent->client->saber[1]))
		{//can turn second saber off
			if (ent->client->ps.saberHolstered == 1)
			{//have one holstered
				//unholster it
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOn);
				ent->client->ps.saberHolstered = 0;
				//g_active should take care of this, but...
				ent->client->ps.fd.saberAnimLevel = SS_DUAL;
			}
			else if (ent->client->ps.saberHolstered == 0)
			{//have none holstered
				if ((ent->client->saber[1].saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE))
				{//can't turn it off manually
				}
				else if (ent->client->saber[1].bladeStyle2Start > 0
					&& (ent->client->saber[1].saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE2))
				{//can't turn it off manually
				}
				else
				{
					//turn it off
					G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
					ent->client->ps.saberHolstered = 1;
					//g_active should take care of this, but...
					ent->client->ps.fd.saberAnimLevel = SS_FAST;
				}
			}

			if (d_saberStanceDebug.integer)
			{
				trap->SendServerCommand(ent - g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle dual saber blade.\n\""));
			}
			return;
		}
	}
	else if (ent->client->saber[0].numBlades > 1
		&& WP_SaberCanTurnOffSomeBlades(&ent->client->saber[0]))
	{ //use staff stance then.
		if (ent->client->ps.saberHolstered == 1)
		{//second blade off
			if (ent->client->ps.saberInFlight)
			{//can't turn second blade back on if it's in the air, you naughty boy!
				if (d_saberStanceDebug.integer)
				{
					trap->SendServerCommand(ent - g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle staff blade in air.\n\""));
				}
				return;
			}
			//turn it on
			G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOn);
			ent->client->ps.saberHolstered = 0;
			//g_active should take care of this, but...
			if (ent->client->saber[0].stylesForbidden)
			{//have a style we have to use
				WP_UseFirstValidSaberStyle(&ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &select_level);
				if (ent->client->ps.weaponTime <= 0)
				{ //not busy, set it now
					ent->client->ps.fd.saberAnimLevel = select_level;
				}
				else
				{ //can't set it now or we might cause unexpected chaining, so queue it
					ent->client->saberCycleQueue = select_level;
				}
			}
		}
		else if (ent->client->ps.saberHolstered == 0)
		{//both blades on
			if ((ent->client->saber[0].saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE))
			{//can't turn it off manually
			}
			else if (ent->client->saber[0].bladeStyle2Start > 0
				&& (ent->client->saber[0].saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE2))
			{//can't turn it off manually
			}
			else
			{
				//turn second one off
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
				ent->client->ps.saberHolstered = 1;
				//g_active should take care of this, but...
				if (ent->client->saber[0].singleBladeStyle != SS_NONE)
				{
					if (ent->client->ps.weaponTime <= 0)
					{ //not busy, set it now
						ent->client->ps.fd.saberAnimLevel = ent->client->saber[0].singleBladeStyle;
					}
					else
					{ //can't set it now or we might cause unexpected chaining, so queue it
						ent->client->saberCycleQueue = ent->client->saber[0].singleBladeStyle;
					}
				}
			}
		}
		if (d_saberStanceDebug.integer)
		{
			trap->SendServerCommand(ent - g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle staff blade.\n\""));
		}
		return;
	}

	if (ent->client->saberCycleQueue)
	{ //resume off of the queue if we haven't gotten a chance to update it yet
		select_level = ent->client->saberCycleQueue;
	}
	else
	{
		select_level = ent->client->ps.fd.saberAnimLevel;
	}

	if (level.gametype == GT_SIEGE &&
		ent->client->siegeClass != -1 &&
		bgSiegeClasses[ent->client->siegeClass].saberStance)
	{ //we have a flag of useable stances so cycle through it instead
		int i = select_level + 1;

		usingSiegeStyle = qtrue;

		while (i != select_level)
		{ //cycle around upward til we hit the next style or end up back on this one
			if (i >= SS_NUM_SABER_STYLES)
			{ //loop back around to the first valid
				i = SS_FAST;
			}

			if (bgSiegeClasses[ent->client->siegeClass].saberStance & (1 << i))
			{ //we can use this one, select it and break out.
				select_level = i;
				break;
			}
			i++;
		}

		if (d_saberStanceDebug.integer)
		{
			trap->SendServerCommand(ent - g_entities, va("print \"SABERSTANCEDEBUG: Attempted to cycle given class stance.\n\""));
		}
	}
	else
	{
		select_level++;

		for (int attempts = 0; attempts < SS_TAVION; attempts++)
		{
			if (select_level > SS_TAVION)
			{
				if ((ent->client->saber[0].type == SABER_BACKHAND))
				{
					select_level = SS_STAFF;
				}
				else if ((ent->client->saber[0].type == SABER_ASBACKHAND))
				{
					select_level = SS_STAFF;
				}
				else
				{
					select_level = SS_FAST;
				}
			}

			if (G_ValidSaberStyle(ent, select_level))
			{
				break;
			}

			//no dice, keep looking
			select_level++;
		}

		/*if (select_level > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
		{
			select_level = FORCE_LEVEL_1;
		}*/

		if (d_saberStanceDebug.integer)
		{
			trap->SendServerCommand(ent - g_entities, va("print \"SABERSTANCEDEBUG: Attempted to cycle stance normally.\n\""));
		}
	}

	if (!usingSiegeStyle)
	{
		//make sure it's valid, change it if not
		WP_UseFirstValidSaberStyle(&ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &select_level);
	}

	if (ent->client->ps.weaponTime <= 0)
	{ //not busy, set it now
		ent->client->ps.fd.saber_anim_levelBase = ent->client->ps.fd.saberAnimLevel = select_level;

		if (!ent->client->ps.saberInFlight)
		{
			if (!(ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK) && !PM_SaberInAttack(ent->client->ps.saber_move) && ent->client->ps.saberLockTime < level.time) // lets do a movement when changing styles // need better anims for this
			{
				if (select_level == SS_DUAL)
				{
					G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_TORSO, BOTH_STAND2TO1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
				}
				else if (select_level == SS_STAFF)
				{
					G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_TORSO, BOTH_STAND2TO1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
				}
				else
				{
					G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_TORSO, BOTH_STAND2TO1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
				}
				if (!(ent->r.svFlags & SVF_BOT)) // only player
				{
					if (ent->client->ps.saberHolstered)
					{
						G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/weapons/saber/saber_catch.mp3"));
					}
					else
					{
						G_Sound(ent, CHAN_AUTO, G_SoundIndex(va("sound/weapons/saber/lowswing%i.wav", Q_irand(1, 7))));
					}
				}
			}
		}
	}
	else
	{ //can't set it now or we might cause unexpected chaining, so queue it
		ent->client->ps.fd.saber_anim_levelBase = ent->client->saberCycleQueue = select_level;
	}
}

void Cmd_EngageDuel_f(gentity_t* ent)
{
	trace_t tr;
	vec3_t forward, fwd_org;

	if (!g_privateDuel.integer)
	{
		return;
	}

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		//rather pointless in this mode..
		trap->SendServerCommand(ent - g_entities,
			va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NODUEL_GAMETYPE")));
		return;
	}

	if (ent->client->ps.duelTime >= level.time)
	{
		return;
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}

	if (ent->client->ps.saberInFlight)
	{
		return;
	}

	if (ent->client->ps.duelInProgress)
	{
		return;
	}

	AngleVectors(ent->client->ps.viewangles, forward, NULL, NULL);

	fwd_org[0] = ent->client->ps.origin[0] + forward[0] * 256;
	fwd_org[1] = ent->client->ps.origin[1] + forward[1] * 256;
	fwd_org[2] = ent->client->ps.origin[2] + ent->client->ps.viewheight + forward[2] * 256;

	trap->Trace(&tr, ent->client->ps.origin, NULL, NULL, fwd_org, ent->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

	G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_TORSO, BOTH_ENGAGETAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);

	if (tr.fraction != 1 && tr.entityNum < MAX_CLIENTS)
	{
		gentity_t* challenged = &g_entities[tr.entityNum];

		if (!challenged || !challenged->client || !challenged->inuse ||
			challenged->health < 1 || challenged->client->ps.stats[STAT_HEALTH] < 1 ||
			challenged->client->ps.weapon != WP_SABER || challenged->client->ps.duelInProgress ||
			challenged->client->ps.saberInFlight)
		{
			return;
		}

		if (!g_friendlyFire.integer && (level.gametype >= GT_TEAM && OnSameTeam(ent, challenged)))
		{
			return;
		}

		if (challenged->client->ps.duelIndex == ent->s.number && challenged->client->ps.duelTime >= level.time)
		{
			trap->SendServerCommand(-1, va("print \"%s %s %s!\n\"", challenged->client->pers.netname,
				G_GetStringEdString("MP_SVGAME", "PLDUELACCEPT"),
				ent->client->pers.netname));

			ent->client->ps.duelInProgress = qtrue;
			challenged->client->ps.duelInProgress = qtrue;

			ent->client->ps.duelTime = level.time + 2000;
			challenged->client->ps.duelTime = level.time + 2000;

			ent->client->ps.powerups[PW_FORCE_ENLIGHTENED_LIGHT] = 0;
			ent->client->ps.powerups[PW_FORCE_ENLIGHTENED_DARK] = 0;
			ent->client->ps.powerups[PW_FORCE_BOON] = 0;
			challenged->client->ps.powerups[PW_FORCE_ENLIGHTENED_LIGHT] = 0;
			challenged->client->ps.powerups[PW_FORCE_ENLIGHTENED_DARK] = 0;
			challenged->client->ps.powerups[PW_FORCE_BOON] = 0;

			G_AddEvent(ent, EV_PRIVATE_DUEL, 1);
			G_AddEvent(challenged, EV_PRIVATE_DUEL, 1);

			ent->client->ps.stats[STAT_HEALTH] = ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
			ent->client->ps.stats[STAT_ARMOR] = 100;
			challenged->client->ps.stats[STAT_HEALTH] = challenged->health = challenged->client->ps.stats[
				STAT_MAX_HEALTH];
			challenged->client->ps.stats[STAT_ARMOR] = 100;

			//Holster their sabers now, until the duel starts (then they'll get auto-turned on to look cool)
			G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, BOTH_BOW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD,
				0);
			G_SetAnim(challenged, &ent->client->pers.cmd, SETANIM_BOTH, BOTH_BOW,
				SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);

			if (!ent->client->ps.saberHolstered)
			{
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
				ent->client->ps.weaponTime = 400;
				ent->client->ps.saberHolstered = 2;
			}
			if (!challenged->client->ps.saberHolstered)
			{
				if (challenged->client->saber[0].soundOff)
				{
					G_Sound(challenged, CHAN_AUTO, challenged->client->saber[0].soundOff);
				}
				if (challenged->client->saber[1].soundOff &&
					challenged->client->saber[1].model[0])
				{
					G_Sound(challenged, CHAN_AUTO, challenged->client->saber[1].soundOff);
				}
				challenged->client->ps.weaponTime = 400;
				challenged->client->ps.saberHolstered = 2;
			}

			//fully heal duelers at the start of duels
			if (ent->health > 0 && ent->client->ps.stats[STAT_HEALTH] > 0)
			{
				if (ent->health != ent->client->ps.stats[STAT_MAX_HEALTH])
				{
					ent->client->ps.stats[STAT_HEALTH] = ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
				}
				if (ent->client->ps.stats[STAT_ARMOR] != ent->client->ps.stats[STAT_MAX_HEALTH])
				{
					ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];
				}
			}

			if (challenged->health > 0 && challenged->client->ps.stats[STAT_HEALTH] > 0)
			{
				if (challenged->health != challenged->client->ps.stats[STAT_MAX_HEALTH])
				{
					challenged->client->ps.stats[STAT_HEALTH] = challenged->health = challenged->client->ps.stats[
						STAT_MAX_HEALTH];
				}
				if (challenged->client->ps.stats[STAT_ARMOR] != challenged->client->ps.stats[STAT_MAX_HEALTH])
				{
					challenged->client->ps.stats[STAT_ARMOR] = challenged->client->ps.stats[STAT_MAX_HEALTH];
				}
			}
		}
		else
		{
			//Print the message that a player has been challenged in private, only announce the actual duel initiation in private
			trap->SendServerCommand(challenged - g_entities,
				va("cp \"%s %s\n\"", ent->client->pers.netname,
					G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGE")));
			trap->SendServerCommand(ent - g_entities,
				va("cp \"%s %s\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGED"),
					challenged->client->pers.netname));
		}

		challenged->client->ps.fd.privateDuelTime = 0;
		//reset the timer in case this player just got out of a duel. He should still be able to accept the challenge.

		ent->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
		ent->client->ps.forceHandExtendTime = level.time + 1000;

		ent->client->ps.duelIndex = challenged->s.number;
		ent->client->ps.duelTime = level.time + 5000;
	}
}

#ifndef FINAL_BUILD
extern stringID_table_t animTable[MAX_ANIMATIONS + 1];

static void Cmd_DebugSetSaberMove_f(gentity_t* self)
{
	int argNum = trap->Argc();
	char arg[MAX_STRING_CHARS];

	if (argNum < 2)
	{
		return;
	}

	trap->Argv(1, arg, sizeof(arg));

	if (!arg[0])
	{
		return;
	}

	self->client->ps.saber_move = atoi(arg);
	self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;

	if (self->client->ps.saber_move >= LS_MOVE_MAX)
	{
		self->client->ps.saber_move = LS_MOVE_MAX - 1;
	}

	Com_Printf("Anim for move: %s\n", animTable[saber_moveData[self->client->ps.saber_move].animToUse].name);
}

static void Cmd_DebugSetSaberBlock_f(gentity_t* self)
{//This is a simple debugging function for debugging the saberblocked code.
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];

	if (argNum < 2)
	{
		return;
	}

	trap_Argv(1, arg, sizeof(arg));

	if (!arg[0])
	{
		return;
	}
	self->client->ps.saberBlocked = atoi(arg);

	if (self->client->ps.saberBlocked > BLOCKED_TOP_PROJ)
	{
		self->client->ps.saberBlocked = BLOCKED_TOP_PROJ;
	}
}

static void Cmd_DebugSetBodyAnim_f(gentity_t* self)
{
	int argNum = trap->Argc();
	char arg[MAX_STRING_CHARS];
	int i = 0;

	if (argNum < 2)
	{
		return;
	}

	trap->Argv(1, arg, sizeof(arg));

	if (!arg[0])
	{
		return;
	}

	while (i < MAX_ANIMATIONS)
	{
		if (!Q_stricmp(arg, animTable[i].name))
		{
			break;
		}
		i++;
	}

	if (i == MAX_ANIMATIONS)
	{
		Com_Printf("Animation '%s' does not exist\n", arg);
		return;
	}

	G_SetAnim(self, NULL, SETANIM_BOTH, i, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);

	Com_Printf("Set body anim to %s\n", arg);
}
#endif

static void StandardSetBodyAnim(gentity_t* self, const int anim, const int flags)
{
	G_SetAnim(self, NULL, SETANIM_BOTH, anim, flags, 0);
}

void DismembermentTest(gentity_t* self);

void bot_set_forced_movement(int bot, int forward, int right, int up);

#ifndef FINAL_BUILD
extern void DismembermentByNum(gentity_t* self, int num);
extern void G_SetVehDamageFlags(gentity_t* veh, int shipSurf, int damageLevel);
#endif

qboolean TryGrapple(gentity_t* ent)
{
	if (ent->client->ps.weaponTime > 0)
	{
		//weapon busy
		return qfalse;
	}
	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		//force power or knockdown or something
		return qfalse;
	}
	if (ent->client->grappleState)
	{
		//already grappling? but weapontime should be > 0 then..
		return qfalse;
	}

	if (ent->client->ps.weapon != WP_SABER && ent->client->ps.weapon != WP_MELEE)
	{
		return qfalse;
	}

	if (ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered)
	{
		Cmd_ToggleSaber_f(ent);
		if (!ent->client->ps.saberHolstered)
		{
			//must have saber holstered
			return qfalse;
		}
	}

	G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
	if (ent->client->ps.torsoAnim == BOTH_KYLE_GRAB)
	{
		//providing the anim set succeeded..
		ent->client->ps.torsoTimer += 500; //make the hand stick out a little longer than it normally would
		if (ent->client->ps.legsAnim == ent->client->ps.torsoAnim)
		{
			ent->client->ps.legsTimer = ent->client->ps.torsoTimer;
		}
		ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
		ent->client->dangerTime = level.time;
		return qtrue;
	}

	return qfalse;
}

static void Cmd_TargetUse_f(gentity_t* ent)
{
	if (trap->Argc() > 1)
	{
		char s_arg[MAX_STRING_CHARS] = { 0 };

		trap->Argv(1, s_arg, sizeof s_arg);
		gentity_t* targ = G_Find(NULL, FOFS(targetname), s_arg);

		while (targ)
		{
			if (targ->use)
				targ->use(targ, ent, ent);
			targ = G_Find(targ, FOFS(targetname), s_arg);
		}
	}
}

static void Cmd_TheDestroyer_f(gentity_t* ent)
{
	if (!ent->client->ps.saberHolstered || ent->client->ps.weapon != WP_SABER)
		return;

	Cmd_ToggleSaber_f(ent);
}

static void Cmd_BotMoveForward_f(gentity_t* ent)
{
	const int arg = 4000;
	char sarg[MAX_STRING_CHARS];

	assert(trap->Argc() > 1);
	trap->Argv(1, sarg, sizeof sarg);

	assert(sarg[0]);
	const int b_cl = atoi(sarg);
	bot_set_forced_movement(b_cl, arg, -1, -1);
}

static void Cmd_BotMoveBack_f(gentity_t* ent)
{
	const int arg = -4000;
	char sarg[MAX_STRING_CHARS];

	assert(trap->Argc() > 1);
	trap->Argv(1, sarg, sizeof sarg);

	assert(sarg[0]);
	const int b_cl = atoi(sarg);
	bot_set_forced_movement(b_cl, arg, -1, -1);
}

static void Cmd_BotMoveRight_f(gentity_t* ent)
{
	const int arg = 4000;
	char sarg[MAX_STRING_CHARS];

	assert(trap->Argc() > 1);
	trap->Argv(1, sarg, sizeof sarg);

	assert(sarg[0]);
	const int b_cl = atoi(sarg);
	bot_set_forced_movement(b_cl, -1, arg, -1);
}

static void Cmd_BotMoveLeft_f(gentity_t* ent)
{
	const int arg = -4000;
	char sarg[MAX_STRING_CHARS];

	assert(trap->Argc() > 1);
	trap->Argv(1, sarg, sizeof sarg);

	assert(sarg[0]);
	const int b_cl = atoi(sarg);
	bot_set_forced_movement(b_cl, -1, arg, -1);
}

static void Cmd_BotMoveUp_f(gentity_t* ent)
{
	const int arg = 4000;
	char sarg[MAX_STRING_CHARS];

	assert(trap->Argc() > 1);
	trap->Argv(1, sarg, sizeof sarg);

	assert(sarg[0]);
	const int b_cl = atoi(sarg);
	bot_set_forced_movement(b_cl, -1, -1, arg);
}

static void Cmd_AddBot_f(const gentity_t* ent)
{
	//because addbot isn't a recognized command unless you're the server, but it is in the menus regardless
	trap->SendServerCommand(ent - g_entities,
		va("print \"%s.\n\"", G_GetStringEdString("MP_SVGAME", "ONLY_ADD_BOTS_AS_SERVER")));
}

/*
=================
ClientCommand
=================
*/

#define CMD_NOINTERMISSION		(1<<0)
#define CMD_CHEAT				(1<<1)
#define CMD_ALIVE				(1<<2)
#define CMD_CHEATOVERRIDE		(1<<3)

typedef struct command_s
{
	const char* name;
	void (*func)(gentity_t* ent);
	int flags;
} command_t;

static int cmdcmp(const void* a, const void* b)
{
	return Q_stricmp(a, ((command_t*)b)->name);
}

command_t commands[] = {
	{"addbot", Cmd_AddBot_f, 0},
	{"callteamvote", Cmd_CallTeamVote_f, CMD_NOINTERMISSION},
	{"callvote", Cmd_CallVote_f, CMD_NOINTERMISSION},
	{"allychat", Cmd_AllyChat_f, CMD_NOINTERMISSION},
	{"debugBMove_Back", Cmd_BotMoveBack_f, CMD_CHEAT | CMD_ALIVE},
	{"debugBMove_Forward", Cmd_BotMoveForward_f, CMD_CHEAT | CMD_ALIVE},
	{"debugBMove_Left", Cmd_BotMoveLeft_f, CMD_CHEAT | CMD_ALIVE},
	{"debugBMove_Right", Cmd_BotMoveRight_f, CMD_CHEAT | CMD_ALIVE},
	{"debugBMove_Up", Cmd_BotMoveUp_f, CMD_CHEAT | CMD_ALIVE},
	{"duelteam", Cmd_DuelTeam_f, CMD_NOINTERMISSION},
	{"follow", Cmd_Follow_f, CMD_NOINTERMISSION},
	{"follownext", Cmd_FollowNext_f, CMD_NOINTERMISSION},
	{"followprev", Cmd_FollowPrev_f, CMD_NOINTERMISSION},
	{"forcechanged", Cmd_ForceChanged_f, 0},
	{"gc", Cmd_GameCommand_f, CMD_NOINTERMISSION},
	{"give", Cmd_Give_f, CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION},
	{"giveother", Cmd_GiveOther_f, CMD_CHEAT | CMD_NOINTERMISSION},
	{"god", Cmd_God_f, CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION},
	{"kill", Cmd_Kill_f, CMD_ALIVE | CMD_NOINTERMISSION},
	{"killother", Cmd_KillOther_f, CMD_CHEAT | CMD_NOINTERMISSION},
	{"levelshot", Cmd_LevelShot_f, CMD_NOINTERMISSION},
	{"maplist", Cmd_MapList_f, CMD_NOINTERMISSION},
	{"noclip", Cmd_Noclip_f, CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION},
	{"notarget", Cmd_Notarget_f, CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION},
	{"npc", Cmd_NPC_f, CMD_CHEAT | CMD_ALIVE},
	{"say", Cmd_Say_f, 0},
	{"say_team", Cmd_SayTeam_f, 0},
	{"score", Cmd_Score_f, 0},
	{"setviewpos", Cmd_SetViewpos_f, CMD_CHEAT | CMD_NOINTERMISSION},
	{"siegeclass", Cmd_SiegeClass_f, CMD_NOINTERMISSION},
	{"team", Cmd_Team_f, CMD_NOINTERMISSION},
	{"teamvote", Cmd_TeamVote_f, CMD_NOINTERMISSION},
	{"tell", Cmd_Tell_f, 0},
	{"thedestroyer", Cmd_TheDestroyer_f, CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION},
	{"t_use", Cmd_TargetUse_f, CMD_CHEAT | CMD_ALIVE},
	{"voice_cmd", Cmd_VoiceCommand_f, CMD_NOINTERMISSION},
	{"vote", Cmd_Vote_f, CMD_NOINTERMISSION},
	{"where", Cmd_Where_f, CMD_NOINTERMISSION},
	{"sje", Cmd_NPC_f, CMD_CHEATOVERRIDE | CMD_ALIVE},
	// Movieduels secrect commands
	{"Adminlogin", Cmd_AdminLogin, 0},
	{"Adminlogout", Cmd_AdminLogout, 0},
	{"Adminnpc", Cmd_AdminNPC_f, CMD_CHEATOVERRIDE | CMD_ALIVE},
	{"Adminchangemap", Cmd_ChangeMap, CMD_NOINTERMISSION},
	{"Adminpunish", Cmd_Punish, CMD_NOINTERMISSION},
	{"Adminkick", Cmd_Kick, CMD_NOINTERMISSION},
	{"Adminmenu", CG_AdminMenu, CMD_NOINTERMISSION},
	{"noforce", Cmd_Noforce_f, CMD_ALIVE | CMD_NOINTERMISSION},
};

static const size_t num_commands = ARRAY_LEN(commands);

extern qboolean inGameCinematic;
extern void Create_Autosave(vec3_t origin, int size, qboolean teleportPlayers);
extern void Save_Autosaves(void);
extern void Delete_Autosaves(const gentity_t* ent);
extern void G_SetsaberdownorAnim(gentity_t* ent);

void ClientCommand(const int clientNum)
{
	char cmd[MAX_TOKEN_CHARS] = { 0 };

	gentity_t* ent = g_entities + clientNum;
	if (!ent->client || ent->client->pers.connected != CON_CONNECTED)
	{
		G_SecurityLogPrintf("ClientCommand(%d) without an active connection\n", clientNum);
		return; // not fully in game yet
	}

	trap->Argv(0, cmd, sizeof cmd);

	if (Q_stricmp(cmd, "EndCinematic") == 0)
	{
		//one of the clients just finished their cutscene, start rendering server frames again.
		inGameCinematic = qfalse;
		return;
	}

	if (Q_stricmp(cmd, "autosave_add") == 0)
	{
		const int args = trap->Argc();

		if (args < 1)
		{
			//no args, use defaults
			Create_Autosave(ent->r.currentOrigin, 0, qfalse);
		}
		else
		{
			char arg1[MAX_STRING_CHARS];
			trap->Argv(1, arg1, sizeof arg1);
			if (arg1[0] == 't')
			{
				//use default size with teleport flag
				Create_Autosave(ent->r.currentOrigin, 0, qtrue);
			}
			else if (args > 1)
			{
				char arg2[MAX_STRING_CHARS];
				//size and teleport flag
				trap->Argv(2, arg2, sizeof arg2);
				Create_Autosave(ent->r.currentOrigin, atoi(arg1), arg2[0] == 't' ? qtrue : qfalse);
			}
			else
			{
				//just size
				Create_Autosave(ent->r.currentOrigin, atoi(arg1), qfalse);
			}
		}
	}
	else if (Q_stricmp(cmd, "autosave_save") == 0)
	{
		Save_Autosaves();
	}
	else if (Q_stricmp(cmd, "autosave_delete") == 0)
	{
		Delete_Autosaves(ent);
	}
	//rww - redirect bot commands
	if (strstr(cmd, "bot_") && AcceptBotCommand(cmd, ent))
	{
		return;
	}
	if (Q_stricmp(cmd, "saberdown") == 0)
	{
		G_SetsaberdownorAnim(ent);
	}
	else if (Q_stricmp(cmd, "taunt") == 0)
	{
		G_SetTauntAnim(ent, TAUNT_TAUNT);
	}
	else if (Q_stricmp(cmd, "bow") == 0)
	{
		G_SetTauntAnim(ent, TAUNT_BOW);
	}
	else if (Q_stricmp(cmd, "meditate") == 0)
	{
		G_SetTauntAnim(ent, TAUNT_MEDITATE);
	}
	else if (Q_stricmp(cmd, "flourish") == 0)
	{
		G_SetTauntAnim(ent, TAUNT_FLOURISH);
	}
	else if (Q_stricmp(cmd, "gloat") == 0)
	{
		G_SetTauntAnim(ent, TAUNT_GLOAT);
	}
	else if (Q_stricmp(cmd, "surrender") == 0)
	{
		G_SetTauntAnim(ent, TAUNT_SURRENDER);
	}
	else if (Q_stricmp(cmd, "reload") == 0)
	{
		G_SetTauntAnim(ent, TAUNT_RELOAD);
	}
	else if ((Q_stricmp(cmd, "weather") == 0) || (Q_stricmp(cmd, "r_weather") == 0))
	{
		char arg1[MAX_STRING_CHARS];
		int num;
		trap->Argv(1, arg1, sizeof arg1);

		if ((Q_stricmp(arg1, "snow") == 0) || (Q_stricmp(arg1, "1") == 0))
		{
			G_RemoveWeather();
			num = G_EffectIndex("*clear");
			trap->SetConfigstring(CS_EFFECTS + num, "");
			G_EffectIndex("*snow");
		}
		else if ((Q_stricmp(arg1, "lava") == 0) || (Q_stricmp(arg1, "3") == 0))
		{
			G_RemoveWeather();
			num = G_EffectIndex("*clear");
			trap->SetConfigstring(CS_EFFECTS + num, "");
			G_EffectIndex("*lava");
		}
		else if ((Q_stricmp(arg1, "rain") == 0) || (Q_stricmp(arg1, "2") == 0))
		{
			G_RemoveWeather();
			num = G_EffectIndex("*clear");
			trap->SetConfigstring(CS_EFFECTS + num, "");
			G_EffectIndex("*rain 500");
		}
		else if ((Q_stricmp(arg1, "sandstorm") == 0) || (Q_stricmp(arg1, "sand") == 0) || (Q_stricmp(arg1, "4") == 0))
		{
			G_RemoveWeather();
			num = G_EffectIndex("*clear");
			trap->SetConfigstring(CS_EFFECTS + num, "");
			G_EffectIndex("*wind");
			G_EffectIndex("*sand");
		}
		else if ((Q_stricmp(arg1, "fog") == 0) || (Q_stricmp(arg1, "6") == 0))
		{
			G_RemoveWeather();
			num = G_EffectIndex("*clear");
			trap->SetConfigstring(CS_EFFECTS + num, "");
			G_EffectIndex("*heavyrainfog");
		}
		else if ((Q_stricmp(arg1, "spacedust") == 0) || (Q_stricmp(arg1, "9") == 0))
		{
			G_RemoveWeather();
			num = G_EffectIndex("*clear");
			trap->SetConfigstring(CS_EFFECTS + num, "");
			G_EffectIndex("*spacedust 4000");
		}
		else if ((Q_stricmp(arg1, "acidrain") == 0) || (Q_stricmp(arg1, "7") == 0))
		{
			G_RemoveWeather();
			num = G_EffectIndex("*clear");
			trap->SetConfigstring(CS_EFFECTS + num, "");
			G_EffectIndex("*acidrain 500");
		}
		if ((Q_stricmp(arg1, "clear") == 0) || (Q_stricmp(arg1, "0") == 0))
		{
			G_RemoveWeather();
			num = G_EffectIndex("*clear");
			trap->SetConfigstring(CS_EFFECTS + num, "");
		}
	}

	const command_t* command = (command_t*)Q_LinearSearch(cmd, commands, num_commands, sizeof commands[0], cmdcmp);
	if (!command)
	{
		return;
	}
	if (command->flags & CMD_NOINTERMISSION
		&& (level.intermissionQueued || level.intermissiontime))
	{
		trap->SendServerCommand(clientNum, va("print \"%s (%s)\n\"",
			G_GetStringEdString("MP_SVGAME", "CANNOT_TASK_INTERMISSION"), cmd));
		return;
	}
	if (command->flags & CMD_CHEAT
		&& !sv_cheats.integer)
	{
		trap->SendServerCommand(clientNum, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCHEATS")));
		return;
	}
	if (command->flags & CMD_CHEATOVERRIDE && !g_cheatoverride.integer)
	{
		trap->SendServerCommand(clientNum, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCHEATS")));
		return;
	}
	if (command->flags & CMD_ALIVE
		&& (ent->health <= 0
			|| ent->client->tempSpectate >= level.time
			|| ent->client->sess.sessionTeam == TEAM_SPECTATOR))
	{
		trap->SendServerCommand(clientNum, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBEALIVE")));
		return;
	}
	command->func(ent);
}