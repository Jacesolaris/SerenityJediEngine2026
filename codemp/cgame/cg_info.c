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

// cg_info.c -- display information while data is being loading

#include "cg_local.h"

#define MAX_LOADING_PLAYER_ICONS	16
#define MAX_LOADING_ITEM_ICONS		26
//Force Disable Settings
#define FORCE_ALLOFF				262143  //All Force Powers Off
#define FORCE_ALLON					0
#define FORCE_JUMPONLY				262141
#define FORCE_NEUTRALSONLY			16353

void CG_LoadBar(void);

/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString(const char* s)
{
	Q_strncpyz(cg.infoScreenText, s, sizeof cg.infoScreenText);

	trap->UpdateScreen();
}

/*
===================
CG_LoadingItem
===================
*/
void CG_LoadingItem(const int itemNum)
{
	char upper_key[1024];

	const gitem_t* item = &bg_itemlist[itemNum];

	if (!item->classname || !item->classname[0])
	{
		//	CG_LoadingString( "Unknown item" );
		return;
	}

	strcpy(upper_key, item->classname);
	CG_LoadingString(CG_GetStringEdString("SP_INGAME", Q_strupr(upper_key)));
}

/*
===================
CG_LoadingClient
===================
*/
void CG_LoadingClient(const int clientNum)
{
	char personality[MAX_QPATH];

	const char* info = CG_ConfigString(CS_PLAYERS + clientNum);

	Q_strncpyz(personality, Info_ValueForKey(info, "n"), sizeof personality);

	CG_LoadingString(personality);
}

/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
overlays UI_DrawConnectScreen
*/
#define UI_INFOFONT (UI_SMALLFONT)

int SCREENSHOT_TOTAL = -1;

int SCREENSHOT_CHOICE = 0;
int SCREENSHOT_NEXT_UPDATE_TIME = 0;
char SCREENSHOT_CURRENT[64] = { 0 };

static char* cg_GetCurrentLevelshot1(const char* s)
{
	const qhandle_t levelshot1 = trap->R_RegisterShaderNoMip(va("levelshots/%s", s));
	const int time = trap->Milliseconds();

	if (levelshot1 && SCREENSHOT_NEXT_UPDATE_TIME == 0)
	{
		SCREENSHOT_NEXT_UPDATE_TIME = time + 2500;
		memset(SCREENSHOT_CURRENT, 0, sizeof SCREENSHOT_CURRENT);
		strcpy(SCREENSHOT_CURRENT, va("levelshots/%s", s));
		return SCREENSHOT_CURRENT;
	}

	if (SCREENSHOT_NEXT_UPDATE_TIME < time || SCREENSHOT_NEXT_UPDATE_TIME == 0)
	{
		if (SCREENSHOT_TOTAL < 0)
		{
			// Count and register them...
			SCREENSHOT_TOTAL = 0;

			while (1)
			{
				char screenShot[128] = { 0 };

				strcpy(screenShot, va("menu/art/unknownmap_mp%i", SCREENSHOT_TOTAL));

				if (!trap->R_RegisterShaderNoMip(screenShot))
				{
					// Found the last one...
					break;
				}

				SCREENSHOT_TOTAL++;
			}

			SCREENSHOT_TOTAL--;
		}

		SCREENSHOT_NEXT_UPDATE_TIME = time + 2500;

		SCREENSHOT_CHOICE = Q_flrand(0, SCREENSHOT_TOTAL);
		memset(SCREENSHOT_CURRENT, 0, sizeof SCREENSHOT_CURRENT);
		strcpy(SCREENSHOT_CURRENT, va("menu/art/unknownmap_mp%i", SCREENSHOT_CHOICE));
	}

	return SCREENSHOT_CURRENT;
}

static char* cg_GetCurrentLevelshot2(const char* s)
{
	const qhandle_t levelshot2 = trap->R_RegisterShaderNoMip(va("levelshots/%s2", s));
	const int time = trap->Milliseconds();

	if (levelshot2 && SCREENSHOT_NEXT_UPDATE_TIME == 0)
	{
		SCREENSHOT_NEXT_UPDATE_TIME = time + 2500;
		memset(SCREENSHOT_CURRENT, 0, sizeof SCREENSHOT_CURRENT);
		strcpy(SCREENSHOT_CURRENT, va("levelshots/%s", s));
		return SCREENSHOT_CURRENT;
	}

	if (SCREENSHOT_NEXT_UPDATE_TIME < time || SCREENSHOT_NEXT_UPDATE_TIME == 0)
	{
		if (SCREENSHOT_TOTAL < 0)
		{
			// Count and register them...
			SCREENSHOT_TOTAL = 0;

			while (1)
			{
				char screenShot[128] = { 0 };

				strcpy(screenShot, va("menu/art/unknownmap_mp%i", SCREENSHOT_TOTAL));

				if (!trap->R_RegisterShaderNoMip(screenShot))
				{
					// Found the last one...
					break;
				}

				SCREENSHOT_TOTAL++;
			}

			SCREENSHOT_TOTAL--;
		}

		SCREENSHOT_NEXT_UPDATE_TIME = time + 2500;

		SCREENSHOT_CHOICE = Q_flrand(0, SCREENSHOT_TOTAL);
		memset(SCREENSHOT_CURRENT, 0, sizeof SCREENSHOT_CURRENT);
		strcpy(SCREENSHOT_CURRENT, va("menu/art/unknownmap_mp%i", SCREENSHOT_CHOICE));
	}

	return SCREENSHOT_CURRENT;
}

int SCREENTIP_NEXT_UPDATE_TIME = 0;

static void LoadTips(void)
{
	const int time = trap->Milliseconds();
	const int index = rand() % 15;

	if ((SCREENTIP_NEXT_UPDATE_TIME < time || SCREENTIP_NEXT_UPDATE_TIME == 0))
	{
		switch (index)
		{
		case 0:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP2"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 1:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP3"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 2:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP4"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 3:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP5"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 4:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP6"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 5:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP7"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 6:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP8"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 7:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP9"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 8:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP10"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 9:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP11"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 10:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP12"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 11:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP13"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 12:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP14"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 13:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP15"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		case 14:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP1"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		default:
			CG_DrawProportionalString(300, 390, CG_GetStringEdString("LOADTIPS", "TIP1"),
				UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			break;
		}
		SCREENTIP_NEXT_UPDATE_TIME = time + 3500;
	}
}

void CG_DrawInformation(void)
{
	int value;
	char buf[1024];
	const int i_prop_height = 18; // I know, this is total crap, but as a post release asian-hack....  -Ste

	const char* info = CG_ConfigString(CS_SERVERINFO);
	const char* sys_info = CG_ConfigString(CS_SYSTEMINFO);

	const char* s = Info_ValueForKey(info, "mapname");
	qhandle_t levelshot = trap->R_RegisterShaderNoMip(va("levelshots/%s", s));
	qhandle_t levelshot2 = trap->R_RegisterShaderNoMip(va("levelshots/%s2", s));

	if (!levelshot)
	{
		levelshot = trap->R_RegisterShaderNoMip(cg_GetCurrentLevelshot1(s));
	}
	if (!levelshot2)
	{
		levelshot2 = trap->R_RegisterShaderNoMip(cg_GetCurrentLevelshot2(s));
	}

	trap->R_SetColor(NULL);

	if (cg.loadLCARSStage >= 4)
	{
		CG_DrawPic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot2);
	}
	else
	{
		CG_DrawPic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot);
	}

	CG_LoadBar();

	LoadTips();

	int y = 180 - 32;

	// don't print server lines if playing a local game
	trap->Cvar_VariableStringBuffer("sv_running", buf, sizeof buf);

	if (!atoi(buf))
	{
		// server hostname
		Q_strncpyz(buf, Info_ValueForKey(info, "sv_hostname"), sizeof buf);
		Q_CleanStr(buf);
		CG_DrawProportionalString(320, y, buf, UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
		y += i_prop_height;

		// pure server
		s = Info_ValueForKey(sys_info, "sv_pure");
		if (s[0] == '1')
		{
			const char* psPure = CG_GetStringEdString("MP_INGAME", "PURE_SERVER");
			CG_DrawProportionalString(320, y, psPure, UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += i_prop_height;
		}

		// server-specific message of the day
		s = CG_ConfigString(CS_MOTD);
		if (s[0])
		{
			CG_DrawProportionalString(320, y, s, UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += i_prop_height;
		}

		{
			// display global MOTD at bottom (mirrors ui_main UI_DrawConnectScreen
			char motd_string[1024];
			trap->Cvar_VariableStringBuffer("cl_motdString", motd_string, sizeof motd_string);

			if (motd_string[0])
			{
				CG_DrawProportionalString(320, 425, motd_string, UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			}
		}

		// some extra space after hostname and motd
		y += 10;
	}

	// map-specific message (long map name)
	s = CG_ConfigString(CS_MESSAGE);
	if (s[0])
	{
		CG_DrawProportionalString(320, y, s, UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
		y += i_prop_height;
	}

	// cheats warning
	s = Info_ValueForKey(sys_info, "sv_cheats");
	if (s[0] == '1')
	{
		CG_DrawProportionalString(320, y, CG_GetStringEdString("MP_INGAME", "CHEATSAREENABLED"),
			UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
		y += i_prop_height;
	}

	// game type
	s = BG_GetGametypeString(cgs.gametype);
	CG_DrawProportionalString(320, y, s, UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
	y += i_prop_height;

	if (cgs.gametype != GT_SIEGE)
	{
		if (cgs.gametype != GT_SINGLE_PLAYER)
		{
			value = atoi(Info_ValueForKey(info, "timelimit"));
			if (value)
			{
				CG_DrawProportionalString(320, y, va("%s %i", CG_GetStringEdString("MP_INGAME", "TIMELIMIT"), value),
					UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
				y += i_prop_height;
			}
		}

		if (cgs.gametype < GT_CTF && cgs.gametype != GT_SINGLE_PLAYER)
		{
			value = atoi(Info_ValueForKey(info, "fraglimit"));
			if (value)
			{
				CG_DrawProportionalString(320, y, va("%s %i", CG_GetStringEdString("MP_INGAME", "FRAGLIMIT"), value),
					UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
				y += i_prop_height;
			}

			if (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL)
			{
				value = atoi(Info_ValueForKey(info, "duel_fraglimit"));
				if (value)
				{
					CG_DrawProportionalString(320, y, va("%s %i", CG_GetStringEdString("MP_INGAME", "WINLIMIT"), value),
						UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
					y += i_prop_height;
				}
			}
		}
	}

	if (cgs.gametype >= GT_CTF)
	{
		value = atoi(Info_ValueForKey(info, "capturelimit"));
		if (value)
		{
			CG_DrawProportionalString(320, y, va("%s %i", CG_GetStringEdString("MP_INGAME", "CAPTURELIMIT"), value),
				UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += i_prop_height;
		}
	}

	if (cgs.gametype >= GT_TEAM)
	{
		value = atoi(Info_ValueForKey(info, "g_forceBasedTeams"));
		if (value)
		{
			CG_DrawProportionalString(320, y, CG_GetStringEdString("MP_INGAME", "FORCEBASEDTEAMS"),
				UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += i_prop_height;
		}
	}

	if (cgs.gametype != GT_SIEGE)
	{
		const int value_nofp = atoi(Info_ValueForKey(info, "g_forcePowerDisable"));

		value = atoi(Info_ValueForKey(info, "g_maxForceRank"));
		if (value && value_nofp != FORCE_ALLOFF && value < NUM_FORCE_MASTERY_LEVELS)
		{
			char fm_str[1024];

			trap->SE_GetStringTextString("MP_INGAME_MAXFORCERANK", fm_str, sizeof fm_str);

			CG_DrawProportionalString(
				320, y, va("%s %s", fm_str, CG_GetStringEdString("MP_INGAME", forceMasteryLevels[value])),
				UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += i_prop_height;
		}
		else if (value_nofp != FORCE_ALLOFF)
		{
			char fm_str[1024];
			trap->SE_GetStringTextString("MP_INGAME_MAXFORCERANK", fm_str, sizeof fm_str);

			CG_DrawProportionalString(
				320, y, va("%s %s", fm_str, (char*)CG_GetStringEdString("MP_INGAME", forceMasteryLevels[7])),
				UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += i_prop_height;
		}

		if (value_nofp == FORCE_ALLOFF)
		{
			CG_DrawProportionalString(320, y, va("%s", (char*)CG_GetStringEdString("MP_INGAME", "NOFPSET")),
				UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += i_prop_height;
		}
		else if (value_nofp == FORCE_JUMPONLY)
		{
			CG_DrawProportionalString(320, y, va("%s", (char*)CG_GetStringEdString("MP_INGAME", "NOFPSET")),
				UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += i_prop_height;
		}
		else if (value_nofp == FORCE_NEUTRALSONLY)
		{
			CG_DrawProportionalString(320, y, "Neutral Force Powers Only", UI_CENTER | UI_INFOFONT | UI_DROPSHADOW,
				colorWhite);
			y += i_prop_height;
		}
		else if (value_nofp)
		{
			CG_DrawProportionalString(320, y, "Custom Force Setup", UI_CENTER | UI_INFOFONT | UI_DROPSHADOW,
				colorWhite);
			y += i_prop_height;
		}

		if (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL)
		{
			value = atoi(Info_ValueForKey(info, "g_duelWeaponDisable"));
		}
		else
		{
			value = atoi(Info_ValueForKey(info, "g_weaponDisable"));
		}
		if (cgs.gametype != GT_JEDIMASTER && value == WP_SABERSONLY)
		{
			CG_DrawProportionalString(320, y, va("%s", (char*)CG_GetStringEdString("MP_INGAME", "SABERONLYSET")),
				UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += i_prop_height;
		}
		else if (value == WP_MELEEONLY)
		{
			CG_DrawProportionalString(320, y, "Melee Only", UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += i_prop_height;
		}
		else if (value == WP_MELEESABERS)
		{
			CG_DrawProportionalString(320, y, "Sabers & Melee Only", UI_CENTER | UI_INFOFONT | UI_DROPSHADOW,
				colorWhite);
			y += i_prop_height;
		}
		else if (value == WP_NOEXPLOS)
		{
			CG_DrawProportionalString(320, y, "No Explosives", UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += i_prop_height;
		}
		else if (value)
		{
			CG_DrawProportionalString(320, y, "Custom Weapon Setup", UI_CENTER | UI_INFOFONT | UI_DROPSHADOW,
				colorWhite);
			y += i_prop_height;
		}
	}
}

/*
===================
CG_LoadBar
===================
*/

#define LOADBAR_CLIP_WIDTH		256
#define LOADBAR_CLIP_HEIGHT		64

void CG_LoadBar(void)
{
	const int numticks = 9, tickwidth = 40, tickheight = 8;
	const int tickpadx = 20, tickpady = 12;
	const int capwidth = 8;
	const int barwidth = numticks * tickwidth + tickpadx * 2 + capwidth * 2, barleft = (640 - barwidth) / 2;
	const int barheight = tickheight + tickpady * 2, bartop = 480 - barheight;
	const int capleft = barleft + tickpadx, tickleft = capleft + capwidth, ticktop = bartop + tickpady;

	trap->R_SetColor(colorWhite);
	// Draw background
	CG_DrawPic(barleft, bartop, barwidth, barheight, cgs.media.loadBarLEDSurround);

	// Draw left cap (backwards)
	CG_DrawPic(tickleft, ticktop, -capwidth, tickheight, cgs.media.loadBarLEDCap);

	// Draw bar
	CG_DrawPic(tickleft, ticktop, tickwidth * cg.loadLCARSStage, tickheight, cgs.media.loadBarLED);

	// Draw right cap
	CG_DrawPic(tickleft + tickwidth * cg.loadLCARSStage, ticktop, capwidth, tickheight, cgs.media.loadBarLEDCap);

	if (cg.loadLCARSStage >= 3)
	{
		if (cg.loadLCARSStage <= 6)
		{
			if (com_rend2.integer == 1) //rend2 is on
			{
				CG_DrawProportionalString(300, 2, CG_GetStringEdString("LOADTIPS", "REND2TIP"), UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			}
		}
		const int x = (640 - LOADBAR_CLIP_WIDTH) / 2;
		const int y = 50;

		CG_DrawPic(x, y, LOADBAR_CLIP_WIDTH, LOADBAR_CLIP_HEIGHT, cgs.media.load_SerenitySaberSystems);
	}
}