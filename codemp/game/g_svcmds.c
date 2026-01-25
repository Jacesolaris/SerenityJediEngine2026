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
extern cvar_t* r_weather;

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
	byteAlias_t m;

	int i = 0;
	char* p = from;
	while (*p && i < 4)
	{
		m.b[i] = 0;
		while (*p >= '0' && *p <= '9')
		{
			m.b[i] = m.b[i] * 10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
			break;
		i++, p++;
	}

	const uint32_t in = m.ui;

	for (i = 0; i < numIPFilters; i++)
	{
		if ((in & ipFilters[i].mask) == ipFilters[i].compare)
			return g_filterBan.integer != 0;
	}

	return g_filterBan.integer == 0;
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

void G_RemoveWeather(void)
{
	trap->SendConsoleCommand(EXEC_APPEND, "execq Weather/clear.cfg");
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

static void Svcmd_Weather_f(void)
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

svcmd_t svcmds[] = {
	{"addbot", Svcmd_AddBot_f, qfalse},
	{"addip", Svcmd_AddIP_f, qfalse},
	{"botlist", Svcmd_BotList_f, qfalse},
	{"entitylist", Svcmd_EntityList_f, qfalse},
	{"forceteam", Svcmd_ForceTeam_f, qfalse},
	{"game_memory", Svcmd_GameMem_f, qfalse},
	{"listip", Svcmd_ListIP_f, qfalse},
	{"removeip", Svcmd_RemoveIP_f, qfalse},
	{"say", Svcmd_Say_f, qtrue},
	{"toggleallowvote", Svcmd_ToggleAllowVote_f, qfalse},
	{"toggleuserinfovalidation", Svcmd_ToggleUserinfoValidation_f, qfalse},
	{"centersay", Svcmd_CenterSay_f, qfalse},
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

	if ((Q_stricmp(cmd, "r_weather") == 0) || (Q_stricmp(cmd, "weather") == 0))
	{
		Svcmd_Weather_f();
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