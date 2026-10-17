/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, SerenityJediEngine2027 contributors

This file is part of the SerenityJediEngine2027 source code.

SerenityJediEngine2027 is free software; you can redistribute it and/or modify it
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

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"

/*
==============================================================================

PACKET FILTERING

You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.

==============================================================================
*/

typedef struct ipFilter_s
{
	uint32_t mask, compare;
} ipFilter_t;

#define	MAX_IPFILTERS (1024)

static ipFilter_t ipFilters[MAX_IPFILTERS];
static int numIPFilters;

/*
=================
StringToFilter
=================
*/
static qboolean StringToFilter(char* s, ipFilter_t* f)
{
	char num[128];
	byteAlias_t b, m;

	b.ui = m.ui = 0u;

	for (int i = 0; i < 4; i++)
	{
		if (*s < '0' || *s > '9')
		{
			if (*s == '*')
			{
				// 'match any'
				// b[i] and m[i] to 0
				s++;
				if (!*s)
					break;
				s++;
				continue;
			}
			trap->Print("Bad filter address: %s\n", s);
			return qfalse;
		}

		int j = 0;
		while (*s >= '0' && *s <= '9')
			num[j++] = *s++;

		num[j] = 0;
		b.b[i] = (byte)atoi(num);
		m.b[i] = 0xFF;

		if (!*s)
			break;

		s++;
	}

	f->mask = m.ui;
	f->compare = b.ui;

	return qtrue;
}

/*
=================
UpdateIPBans
=================
*/
static void UpdateIPBans(void)
{
	char iplist_final[MAX_CVAR_VALUE_STRING];

	*iplist_final = 0;
	for (int i = 0; i < numIPFilters; i++)
	{
		char ip[NET_ADDRSTRMAXLEN];
		byteAlias_t m;
		byteAlias_t b;
		if (ipFilters[i].compare == 0xFFFFFFFFu)
			continue;

		b.ui = ipFilters[i].compare;
		m.ui = ipFilters[i].mask;
		*ip = 0;
		for (int j = 0; j < 4; j++)
		{
			if (m.b[j] != 0xFF)
				Q_strcat(ip, sizeof ip, "*");
			else
				Q_strcat(ip, sizeof ip, va("%i", b.b[j]));
			Q_strcat(ip, sizeof ip, j < 3 ? "." : " ");
		}
		if (strlen(iplist_final) + strlen(ip) < MAX_CVAR_VALUE_STRING)
			Q_strcat(iplist_final, sizeof iplist_final, ip);
		else
		{
			Com_Printf("g_banIPs overflowed at MAX_CVAR_VALUE_STRING\n");
			break;
		}
	}

	trap->Cvar_Set("g_banIPs", iplist_final);
}

/*
=================
G_FilterPacket
=================
*/
qboolean G_FilterPacket(char* from)
{
	// FIX: ensure union is fully initialized
	byteAlias_t m = { 0 };

	int i = 0;
	char* p = from;

	// Parse up to 4 octets of an IPv4 address
	while (*p && i < 4)
	{
		m.b[i] = 0;

		// Accumulate numeric digits
		while (*p >= '0' && *p <= '9')
		{
			m.b[i] = (byte)(m.b[i] * 10 + (*p - '0'));
			p++;
		}

		// End of string or port separator
		if (*p == '\0' || *p == ':')
		{
			break;
		}

		i++;
		p++;
	}

	const uint32_t in = m.ui;

	// Compare against all IP filters
	for (i = 0; i < numIPFilters; i++)
	{
		const qboolean match = ((in & ipFilters[i].mask) == ipFilters[i].compare) ? qtrue : qfalse;

		if (match == qtrue)
		{
			// If filterBan is nonzero, ban; otherwise allow
			return (g_filterBan.integer != 0) ? qtrue : qfalse;
		}
	}

	// No match: invert logic
	return (g_filterBan.integer == 0) ? qtrue : qfalse;
}

/*
=================
AddIP
=================
*/
static void AddIP(char* str)
{
	int i;

	for (i = 0; i < numIPFilters; i++)
	{
		if (ipFilters[i].compare == 0xFFFFFFFFu)
			break; // free spot
	}
	if (i == numIPFilters)
	{
		if (numIPFilters == MAX_IPFILTERS)
		{
			trap->Print("IP filter list is full\n");
			return;
		}
		numIPFilters++;
	}

	if (!StringToFilter(str, &ipFilters[i]))
		ipFilters[i].compare = 0xFFFFFFFFu;

	UpdateIPBans();
}

/*
=================
G_ProcessIPBans
=================
*/
void G_ProcessIPBans(void)
{
	char* s, str[MAX_CVAR_VALUE_STRING] = { 0 };

	Q_strncpyz(str, g_banIPs.string, sizeof str);

	for (char* t = s = g_banIPs.string; *t; t = s)
	{
		s = strchr(s, ' ');
		if (!s)
			break;

		while (*s == ' ')
			*s++ = 0;

		if (*t)
			AddIP(t);
	}
}

/*
=================
Svcmd_AddIP_f
=================
*/
static void Svcmd_AddIP_f(void)
{
	char str[MAX_TOKEN_CHARS];

	if (trap->Argc() < 2)
	{
		trap->Print("Usage: addip <ip-mask>\n");
		return;
	}

	trap->Argv(1, str, sizeof str);

	AddIP(str);
}

/*
==============================================================================

SERVER WEATHER

The weather everybody gets is the "*<command>" strings in the CS_EFFECTS configstrings. Clients run them in index
order (live as they change, all of them when they connect: CG_RegisterSounds / CG_ApplyWeather), and the game runs
them through bg_weather.c for the acid rain / lava weather damage (G_WeatherRescan).

The game sets the weather from the map's Weather/<mapname>.cfg file (G_WeatherLoadMapDefault, at map start) and with
the "weather" command (G_WeatherCommand_f, server console or an allowed client). The strings it creates for this are
"owned": the next change blanks them (the 64 effect slots would run out otherwise), and new ones always go after every
weather string the game doesn't own (map entities such as fx_rain), so a client that connects later runs everything in
the same order as the clients that saw the changes live.

Before this the map file was exec'd as console commands ("r_weather N"): on a listen server that only reached the host's
renderer (which turned it into r_weather's own preset, or cleared the weather with r_weather 0), on a dedicated server
it only set a cvar. Neither made a configstring, so other players and the gameplay code never had the map's weather.

==============================================================================
*/

#define G_WEATHER_MAX_OWNED		16	// configstrings one weather change may own
#define G_WEATHER_MAX_COMMANDS	8	// weather commands one change may set (the vanilla renderer has 5 clouds anyway)

static int gWeatherOwned[G_WEATHER_MAX_OWNED];
static int gWeatherNumOwned;

static qboolean G_WeatherIsOwned(const int index)
{
	for (int i = 0; i < gWeatherNumOwned; i++)
	{
		if (gWeatherOwned[i] == index)
		{
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean G_WeatherStringInUse(const char* str)
{
	char s[MAX_STRING_CHARS];

	for (int i = 1; i < MAX_FX; i++)
	{
		trap->GetConfigstring(CS_EFFECTS + i, s, sizeof s);
		if (s[0] && !strcmp(s, str))
		{
			return qtrue;
		}
	}
	return qfalse;
}

// sets "*<command>" in the first free effect slot after 'after', returns the slot or -1 if there is none
static int G_WeatherAddString(const char* command, const int after)
{
	char str[MAX_QPATH];
	char s[MAX_STRING_CHARS];
	int i;

	Com_sprintf(str, sizeof str, "*%s", command);

	// keep it different from every other effect string, or G_EffectIndex would hand this slot to a map entity
	// asking for the same weather (and the next change would blank it). Trailing spaces: every weather parser skips them
	size_t len = strlen(str);
	while (G_WeatherStringInUse(str) && len < sizeof str - 1)
	{
		str[len++] = ' ';
		str[len] = '\0';
	}

	for (i = after + 1; i < MAX_FX; i++)
	{
		trap->GetConfigstring(CS_EFFECTS + i, s, sizeof s);
		if (!s[0])
		{
			break;
		}
	}

	if (i >= MAX_FX || gWeatherNumOwned >= G_WEATHER_MAX_OWNED)
	{
		trap->Print(S_COLOR_YELLOW "WARNING: no free effect configstring for the weather \"%s\"\n", command);
		return -1;
	}

	trap->SetConfigstring(CS_EFFECTS + i, str);
	gWeatherOwned[gWeatherNumOwned++] = i;
	return i;
}

// new map (not on map_restart, which keeps the configstrings): nothing is owned yet
void G_WeatherNewMap(void)
{
	gWeatherNumOwned = 0;
}

/*
=================
G_SetWeather

Replaces the weather the game set before with these commands (without the '*'). clearFirst sends a "clear" before
them, which also removes the weather of the map's entities.
=================
*/
static void G_SetWeather(const char* const* commands, const int numCommands, const qboolean clearFirst)
{
	char s[MAX_STRING_CHARS];
	int last = 0;
	int i;

	// the strings of the last change go
	for (i = 0; i < gWeatherNumOwned; i++)
	{
		trap->SetConfigstring(CS_EFFECTS + gWeatherOwned[i], "");
	}
	gWeatherNumOwned = 0;

	// the new ones go after all the other weather strings
	for (i = 1; i < MAX_FX; i++)
	{
		trap->GetConfigstring(CS_EFFECTS + i, s, sizeof s);
		if (s[0] == '*')
		{
			last = i;
		}
	}

	if (clearFirst)
	{
		last = G_WeatherAddString("clear", last);
	}

	for (i = 0; i < numCommands && last >= 0; i++)
	{
		if (commands[i] && commands[i][0])
		{
			last = G_WeatherAddString(commands[i], last);
		}
	}

	// the gameplay weather is now what a client that connects now would get
	G_WeatherRescan();
}

// a weather command for G_SetWeather from a preset number ("4") or a weather command name, NULL if it is neither
static const char* G_WeatherCommandForWord(const char* word)
{
	static const char* const names[] = {
		"lightrain", "rain", "acidrain", "heavyrain", "snow", "lava", "spacedust", "sand", "Sandstorm", "fog",
		"heavyrainfog", "light_fog", "wind", "gustingwind", "outsidepain"
	};

	if (!word || !word[0])
	{
		return NULL;
	}

	if (word[0] >= '0' && word[0] <= '9')
	{
		return BG_WeatherPresetName(atoi(word));
	}

	for (size_t i = 0; i < ARRAY_LEN(names); i++)
	{
		if (!Q_stricmp(word, names[i]))
		{
			return names[i];
		}
	}
	return NULL;
}

// the map's Weather/<mapname>.cfg ("r_weather N" lines, one weather each, as in SP): returns the number of commands
static int G_WeatherReadMapFile(const char** commands, const int maxCommands)
{
	char mapname[MAX_QPATH];
	char buf[2048];
	fileHandle_t f = 0;
	int num = 0;

	trap->Cvar_VariableStringBuffer("mapname", mapname, sizeof mapname);
	if (!mapname[0])
	{
		return 0;
	}

	const int len = trap->FS_Open(va("Weather/%s.cfg", mapname), &f, FS_READ);
	if (!f)
	{
		return 0; // most maps have none
	}
	if (len <= 0 || len >= (int)sizeof buf)
	{
		trap->FS_Close(f);
		return 0;
	}
	trap->FS_Read(buf, len, f);
	trap->FS_Close(f);
	buf[len] = '\0';

	// statements are split by line ends and ';', each is "r_weather <preset>" (or "weather <preset or name>")
	char* p = buf;
	while (*p && num < maxCommands)
	{
		char* end = p;
		char word[2][MAX_QPATH];
		int words = 0;

		while (*end && *end != '\n' && *end != '\r' && *end != ';')
		{
			end++;
		}
		const char next = *end;
		*end = '\0';

		while (*p && words < 2)
		{
			int n = 0;
			while (*p == ' ' || *p == '\t')
			{
				p++;
			}
			while (*p && *p != ' ' && *p != '\t' && n < MAX_QPATH - 1)
			{
				word[words][n++] = *p++;
			}
			word[words][n] = '\0';
			if (!n)
			{
				break;
			}
			words++;
		}

		if (words == 2 && (!Q_stricmp(word[0], "r_weather") || !Q_stricmp(word[0], "weather")))
		{
			const char* command = G_WeatherCommandForWord(word[1]);
			if (command)
			{
				commands[num++] = command;
			}
		}

		p = next ? end + 1 : end;
	}

	return num;
}

/*
=================
G_WeatherLoadMapDefault

Map start: the weather from the map's Weather/<mapname>.cfg file, for everybody, added to the weather the map's
entities set (as SP adds it). Players can show their own choice instead (r_weather) or none (cg_weather 0), see
CG_ApplyWeather; the damage always follows this server weather.
=================
*/
void G_WeatherLoadMapDefault(void)
{
	const char* commands[G_WEATHER_MAX_COMMANDS];
	char mapname[MAX_QPATH];
	static const char* const rend2NoWeatherMaps[] = {
		// as SP g_itemLoad.cpp: no file weather on these with rend2
		"yavin1", "demo", "01nar", "md2_bd_ch", "md_sn_intro_jedi", "md_ch_battledroids", "md_ep4_intro", "secbase",
		"level0", "SJE_ImperialBase", "kejim_post"
	};

	if (g_AllowWeather.integer != 1)
	{
		return;
	}

	trap->Cvar_VariableStringBuffer("mapname", mapname, sizeof mapname);
	if (com_rend2.integer == 1)
	{
		for (size_t i = 0; i < ARRAY_LEN(rend2NoWeatherMaps); i++)
		{
			if (!Q_stricmp(mapname, rend2NoWeatherMaps[i]))
			{
				return;
			}
		}
	}

	const int num = G_WeatherReadMapFile(commands, G_WEATHER_MAX_COMMANDS);
	if (num > 0)
	{
		G_SetWeather(commands, num, qfalse);
	}
}

static void G_WeatherUsage(const gentity_t* ent)
{
	const char* text =
		"usage: weather <type>   sets the server weather for everybody\n"
		"  clear (or 0, none) | default (the map's weather) | snow (1) | rain (2) | lava (3) | sand (4) |\n"
		"  heavyrainfog (5) | fog (6) | acidrain (7) | wind (8) | spacedust (9) |\n"
		"  lightrain | heavyrain | light_fog | gustingwind | Sandstorm\n";

	// the weather strings everybody runs, what the weather damage sees now and whether the asking player is outside
	char list[256] = { 0 };
	char s[MAX_STRING_CHARS];
	for (int i = 1; i < MAX_FX; i++)
	{
		trap->GetConfigstring(CS_EFFECTS + i, s, sizeof s);
		if (s[0] == '*')
		{
			Q_strcat(list, sizeof list, va(" [%s]", s + 1));
		}
	}
	const char* state = va("Server weather:%s\nacid rain %s (g_acidRainDamage %d), lava %s (g_lavaWeatherDamage %d)%s\n",
		list[0] ? list : " none", BG_WeatherOutsidePain() ? "on" : "off", g_acidRainDamage.integer,
		BG_WeatherLava() ? "on" : "off", g_lavaWeatherDamage.integer,
		ent && ent->client ? (BG_IsOutside(ent->r.currentOrigin, ent->s.number) ? ", you are outside" : ", you are inside") : "");

	if (ent)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"%s%s\"", text, state));
	}
	else
	{
		trap->Print("%s%s", text, state);
	}
}

/*
=================
G_WeatherCommand_f

"weather <type>" from the server console (ent NULL) or a client (g_cmds.c, which checks it is allowed).
The named types keep their old meaning (sand = wind + sand, fog = heavyrainfog), numbers are the menu's presets.
=================
*/
void G_WeatherCommand_f(const gentity_t* ent)
{
	char arg[MAX_TOKEN_CHARS];
	const char* commands[G_WEATHER_MAX_COMMANDS];
	int num = 0;

	trap->Argv(1, arg, sizeof arg);

	if (!arg[0])
	{
		G_WeatherUsage(ent);
		return;
	}

	if (!Q_stricmp(arg, "clear") || !Q_stricmp(arg, "none") || !Q_stricmp(arg, "off") || !strcmp(arg, "0"))
	{
		// nothing
	}
	else if (!Q_stricmp(arg, "default") || !Q_stricmp(arg, "map"))
	{
		// the map's weather again: its entities' weather strings (copied, they come after the "clear"), then the file
		char s[MAX_STRING_CHARS];
		static char entityWeather[G_WEATHER_MAX_COMMANDS][MAX_QPATH];

		for (int i = 1; i < MAX_FX && num < G_WEATHER_MAX_COMMANDS; i++)
		{
			trap->GetConfigstring(CS_EFFECTS + i, s, sizeof s);
			if (s[0] == '*' && !G_WeatherIsOwned(i))
			{
				Q_strncpyz(entityWeather[num], s + 1, sizeof entityWeather[num]);
				commands[num] = entityWeather[num];
				num++;
			}
		}
		if (g_AllowWeather.integer == 1)
		{
			num += G_WeatherReadMapFile(commands + num, G_WEATHER_MAX_COMMANDS - num);
		}
	}
	else if (!Q_stricmp(arg, "rain"))
	{
		commands[num++] = "rain 500";
	}
	else if (!Q_stricmp(arg, "sand") || !Q_stricmp(arg, "sandstorm"))
	{
		commands[num++] = "wind";
		commands[num++] = "sand";
	}
	else if (!Q_stricmp(arg, "fog"))
	{
		commands[num++] = "heavyrainfog";
	}
	else if (!Q_stricmp(arg, "spacedust"))
	{
		commands[num++] = "spacedust 4000";
	}
	else if (!Q_stricmp(arg, "acidrain"))
	{
		commands[num++] = "acidrain 500";
	}
	else
	{
		const char* command = G_WeatherCommandForWord(arg);
		if (!command)
		{
			G_WeatherUsage(ent);
			return;
		}
		commands[num++] = command;
	}

	G_SetWeather(commands, num, qtrue);

	if (ent)
	{
		trap->SendServerCommand(ent - g_entities, va("print \"Server weather: %s\n\"", arg));
	}
	else
	{
		trap->Print("Server weather: %s\n", arg);
	}
}

void G_Rename_Player(gentity_t* player, const char* newname)
{
	char userinfo[MAX_INFO_STRING];
	int clientNum = player - g_entities;
	trap->GetUserinfo(clientNum, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "name", newname);
	trap->SetUserinfo(clientNum, userinfo);
	client_userinfo_changed(clientNum);
	player->client->pers.netnameTime = level.time + 5000; // hmmm...
}

/*
=================
Svcmd_RemoveIP_f
=================
*/
static void Svcmd_RemoveIP_f(void)
{
	ipFilter_t f;
	char str[MAX_TOKEN_CHARS];

	if (trap->Argc() < 2)
	{
		trap->Print("Usage: removeip <ip-mask>\n");
		return;
	}

	trap->Argv(1, str, sizeof str);

	if (!StringToFilter(str, &f))
		return;

	for (int i = 0; i < numIPFilters; i++)
	{
		if (ipFilters[i].mask == f.mask &&
			ipFilters[i].compare == f.compare)
		{
			ipFilters[i].compare = 0xffffffffu;
			trap->Print("Removed.\n");

			UpdateIPBans();
			return;
		}
	}

	trap->Print("Didn't find %s.\n", str);
}

static void Svcmd_ListIP_f(void)
{
	int count = 0;

	for (int i = 0; i < numIPFilters; i++)
	{
		byteAlias_t b;
		if (ipFilters[i].compare == 0xffffffffu)
			continue;

		b.ui = ipFilters[i].compare;
		trap->Print("%i.%i.%i.%i\n", b.b[0], b.b[1], b.b[2], b.b[3]);
		count++;
	}
	trap->Print("%i bans.\n", count);
}

qboolean SJE_AllPlayersHaveClientPlugin(void);
char* ConcatArgs(int start);
static void Svcmd_CenterSay_f(void)
{
	if (SJE_AllPlayersHaveClientPlugin())
	{//just fire off the text without editting it for line breaks.
		trap->SendServerCommand(-1, va("cp \"%s\n\"", ConcatArgs(1)));
	}
	else
	{//since someone is running basejka, we need to add line breaks to make up for the
		//50 chars per line limit of basejka's cp code.
		char temp[1024];  //MAX_STRINGED_SV_STRING
		char output[1024];

		//copy the print text.
		strcpy(temp, ConcatArgs(1));

		TextWrapCenterPrint(temp, output);

		trap->SendServerCommand(-1, va("cp \"%s\n\"", output));
	}

	//also duplicate the message in the chat buffer and in the game log. (so the admin can follow admin<->player
	//conversations
	trap->SendServerCommand(-1, va("print \"server: %s\n\"", ConcatArgs(1)));
	G_LogPrintf("server: %s\n", ConcatArgs(1));
	return;
}

/*
===================
Svcmd_EntityList_f
===================
*/
static void Svcmd_EntityList_f(void)
{
	gentity_t* check = g_entities;
	for (int e = 0; e < level.num_entities; e++, check++)
	{
		if (!check->inuse)
		{
			continue;
		}
		trap->Print("%3i:", e);
		switch (check->s.eType)
		{
		case ET_GENERAL:
			trap->Print("ET_GENERAL          ");
			break;
		case ET_PLAYER:
			trap->Print("ET_PLAYER           ");
			break;
		case ET_ITEM:
			trap->Print("ET_ITEM             ");
			break;
		case ET_MISSILE:
			trap->Print("ET_MISSILE          ");
			break;
		case ET_SPECIAL:
			trap->Print("ET_SPECIAL          ");
			break;
		case ET_HOLOCRON:
			trap->Print("ET_HOLOCRON         ");
			break;
		case ET_MOVER:
			trap->Print("ET_MOVER            ");
			break;
		case ET_BEAM:
			trap->Print("ET_BEAM             ");
			break;
		case ET_PORTAL:
			trap->Print("ET_PORTAL           ");
			break;
		case ET_SPEAKER:
			trap->Print("ET_SPEAKER          ");
			break;
		case ET_PUSH_TRIGGER:
			trap->Print("ET_PUSH_TRIGGER     ");
			break;
		case ET_TELEPORT_TRIGGER:
			trap->Print("ET_TELEPORT_TRIGGER ");
			break;
		case ET_INVISIBLE:
			trap->Print("ET_INVISIBLE        ");
			break;
		case ET_NPC:
			trap->Print("ET_NPC              ");
			break;
		case ET_BODY:
			trap->Print("ET_BODY             ");
			break;
		case ET_TERRAIN:
			trap->Print("ET_TERRAIN          ");
			break;
		case ET_FX:
			trap->Print("ET_FX               ");
			break;
		case ET_GRAPPLE:
			trap->Print("ET_GRAPPLE          ");
			break;
		default:
			trap->Print("%-3i                ", check->s.eType);
			break;
		}

		if (check->classname)
		{
			trap->Print("%s", check->classname);
		}
		trap->Print("\n");
	}
}

qboolean StringIsInteger(const char* s);
/*
===================
ClientForString
===================
*/
static gclient_t* ClientForString(const char* s)
{
	gclient_t* cl;
	int idnum;
	char cleanInput[MAX_STRING_CHARS];

	// numeric values could be slot numbers
	if (StringIsInteger(s))
	{
		idnum = atoi(s);
		if (idnum >= 0 && idnum < level.maxclients)
		{
			cl = &level.clients[idnum];
			if (cl->pers.connected == CON_CONNECTED)
			{
				return cl;
			}
		}
	}

	Q_strncpyz(cleanInput, s, sizeof cleanInput);
	Q_StripColor(cleanInput);

	// check for a name match
	for (idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++)
	{
		if (cl->pers.connected != CON_CONNECTED)
		{
			continue;
		}
		if (!Q_stricmp(cl->pers.netname_nocolor, cleanInput))
		{
			return cl;
		}
	}

	trap->Print("User %s is not on the server\n", s);
	return NULL;
}

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
void Svcmd_ForceTeam_f(void)
{
	char str[MAX_TOKEN_CHARS];

	if (trap->Argc() < 3)
	{
		trap->Print("Usage: forceteam <player> <team>\n");
		return;
	}

	// find the player
	trap->Argv(1, str, sizeof str);
	const gclient_t* cl = ClientForString(str);
	if (!cl)
	{
		return;
	}

	// set the team
	trap->Argv(2, str, sizeof str);
	SetTeam(&g_entities[cl - level.clients], str);
}

static void Svcmd_Say_f(void)
{
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT] = { 0 };

	if (trap->Argc() < 2)
		return;

	char* p = ConcatArgs(1);

	if (strlen(p) >= MAX_SAY_TEXT)
	{
		p[MAX_SAY_TEXT - 1] = '\0';
		G_SecurityLogPrintf("Cmd_Say_f from -1 (server) has been truncated: %s\n", p);
	}

	Q_strncpyz(text, p, sizeof text);
	Q_strstrip(text, "\n\r", "  ");

	trap->SendServerCommand(-1, va("print \"server: %s\n\"", text));
}

typedef struct svcmd_s
{
	const char* name;
	void (*func)(void);
	qboolean dedicated;
} svcmd_t;

static int svcmdcmp(const void* a, const void* b)
{
	return Q_stricmp(a, ((svcmd_t*)b)->name);
}

extern void G_FighterTest_f(void); // (2026-10-13) g_vehicles.c, developer test (sv_cheats 1)

svcmd_t svcmds[] = {
	{ "addbot", Svcmd_AddBot_f, qfalse },
	{ "fightertest", G_FighterTest_f, qfalse },
	{ "addip", Svcmd_AddIP_f, qfalse },
	{ "botlist", Svcmd_BotList_f, qfalse },
	{ "entitylist", Svcmd_EntityList_f, qfalse },
	{ "forceteam", Svcmd_ForceTeam_f, qfalse },
	{ "game_memory", Svcmd_GameMem_f, qfalse },
	{ "listip", Svcmd_ListIP_f, qfalse },
	{ "removeip", Svcmd_RemoveIP_f, qfalse },
	{ "say", Svcmd_Say_f, qtrue },
	{ "toggleallowvote", Svcmd_ToggleAllowVote_f, qfalse },
	{ "toggleuserinfovalidation", Svcmd_ToggleUserinfoValidation_f, qfalse },
	{ "centersay", Svcmd_CenterSay_f, qfalse },
};
static const size_t numsvcmds = ARRAY_LEN(svcmds);

/*
=================
ConsoleCommand

=================
*/
qboolean ConsoleCommand(void)
{
	char cmd[MAX_TOKEN_CHARS] = { 0 };

	trap->Argv(0, cmd, sizeof cmd);

	// "weather" (the engine has an "r_weather" cvar everywhere, so that name normally never gets here)
	if ((Q_stricmp(cmd, "r_weather") == 0) || (Q_stricmp(cmd, "weather") == 0))
	{
		G_WeatherCommand_f(NULL);
		return qtrue;
	}

	const svcmd_t* command = (svcmd_t*)Q_LinearSearch(cmd, svcmds, numsvcmds, sizeof svcmds[0], svcmdcmp);

	if (!command)
		return qfalse;

	if (command->dedicated && !dedicated.integer)
		return qfalse;

	command->func();
	return qtrue;
}