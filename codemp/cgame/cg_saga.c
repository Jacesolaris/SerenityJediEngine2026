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

/*****************************************************************************
 * name:		cg_siege.c
 *
 * desc:		Clientgame-side module for Siege gametype.
 *
 * $Author: osman $
 * $Revision: 1.5 $
 *
 *****************************************************************************/
#include "cg_local.h"
#include "game/bg_saga.h"

int cgSiegeRoundState = 0;
int cgSiegeRoundTime = 0;

static char team1[512];
static char team2[512];

int team1Timed = 0;
int team2Timed = 0;

int cgSiegeTeam1PlShader = 0;
int cgSiegeTeam2PlShader = 0;

#define		MAX_TRUEVIEW_INFO_SIZE					8192
char true_view_info[MAX_TRUEVIEW_INFO_SIZE];
int true_view_valid;

static char cgParseObjectives[MAX_SIEGE_INFO_SIZE];

extern void CG_LoadCISounds(clientInfo_t* ci, qboolean modelloaded); //cg_players.c

void CG_DrawSiegeMessage(const char* str, int objective_screen);
void CG_DrawSiegeMessageNonMenu(const char* str);
void CG_SiegeBriefingDisplay(int team, int dontshow);

/*
===============================
CG_PrecacheSiegeObjectiveAssetsForTeam
- Preloads all sound and shader assets referenced by siege objectives.
- Fixed MSVC C6262 by moving large buffers off the stack.
===============================
*/
static void CG_PrecacheSiegeObjectiveAssetsForTeam(const int myTeam)
{
	/* Large buffers moved off stack → static storage (BSS) */
	static char foundobjective[MAX_SIEGE_INFO_SIZE];
	static char objstr[256];
	static char str[MAX_QPATH];

	char teamstr[64] = { 0 };

	/* Validate siege data */
	if (siege_valid == qfalse)
	{
		trap->Error(ERR_DROP, "Siege data does not exist on client!\n");
		return;
	}

	/* Select team string */
	if (myTeam == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	/* Parse objective group for this team */
	const qboolean gotGroup =
		BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives) ? qtrue : qfalse;

	if (gotGroup == qfalse)
	{
		return;
	}

	/* Iterate through possible objective entries */
	for (int i = 1; i < 32; i++)
	{
		/* Build "ObjectiveX" key */
		Com_sprintf(objstr, sizeof(objstr), "Objective%i", i);

		const qboolean gotObjective =
			BG_SiegeGetValueGroup(cgParseObjectives, objstr, foundobjective) ? qtrue : qfalse;

		if (gotObjective == qfalse)
		{
			/* No more objectives */
			break;
		}

		/* ---------------------------------------------------------
		   Precache all referenced assets for this objective
		   --------------------------------------------------------- */

		   /* Team 1 sound */
		if (BG_SiegeGetPairedValue(foundobjective, "sound_team1", str))
		{
			trap->S_RegisterSound(str);
		}

		/* Team 2 sound */
		if (BG_SiegeGetPairedValue(foundobjective, "sound_team2", str))
		{
			trap->S_RegisterSound(str);
		}

		/* Objective graphics */
		if (BG_SiegeGetPairedValue(foundobjective, "objgfx", str))
		{
			trap->R_RegisterShaderNoMip(str);
		}

		/* Map icons */
		if (BG_SiegeGetPairedValue(foundobjective, "mapicon", str))
		{
			trap->R_RegisterShaderNoMip(str);
		}

		if (BG_SiegeGetPairedValue(foundobjective, "litmapicon", str))
		{
			trap->R_RegisterShaderNoMip(str);
		}

		if (BG_SiegeGetPairedValue(foundobjective, "donemapicon", str))
		{
			trap->R_RegisterShaderNoMip(str);
		}
	}
}

static void CG_PrecachePlayersForSiegeTeam(const int team)
{
	int i = 0;

	const siegeTeam_t* stm = BG_SiegeFindThemeForTeam(team);

	if (!stm)
	{
		//invalid team/no theme for team?
		return;
	}

	while (i < stm->numClasses)
	{
		siegeClass_t* scl = stm->classes[i];

		if (scl->forcedModel[0])
		{
			clientInfo_t fake = { 0 };

			Q_strncpyz(fake.modelName, scl->forcedModel, sizeof fake.modelName);

			trap->R_RegisterModel(va("models/players/%s/model.glm", scl->forcedModel));
			if (scl->forcedSkin[0])
			{
				trap->R_RegisterSkin(va("models/players/%s/model_%s.skin", scl->forcedModel, scl->forcedSkin));
				Q_strncpyz(fake.skinName, scl->forcedSkin, sizeof fake.modelName);
			}
			else
			{
				Q_strncpyz(fake.skinName, "default", sizeof fake.skinName);
			}

			//precache the sounds for the model...
			CG_LoadCISounds(&fake, qtrue);
		}

		i++;
	}
}

/*
===============================
CG_InitSiegeMode
- Loads and parses siege mode configuration for the current map.
- Fixed MSVC C6262 by moving large buffers off the stack.
===============================
*/
void CG_InitSiegeMode(void)
{
	/* Large buffers moved off stack → static storage (BSS) */
	static char btime[1024];
	static char teams[2048];
	static char teamInfo[MAX_SIEGE_INFO_SIZE];
	static char teamIcon[128];
	static char buf[1024];
	static char b[256];

	char levelname[MAX_QPATH];
	fileHandle_t f;

	if (cgs.gametype != GT_SIEGE)
	{
		goto failure;
	}

	/* Build "<mapname>.siege" */
	Com_sprintf(levelname, sizeof(levelname), "%s.siege", cgs.rawmapname);

	if (!levelname[0])
	{
		goto failure;
	}

	/* Load siege file */
	const int len = trap->FS_Open(levelname, &f, FS_READ);

	if (!f)
	{
		goto failure;
	}

	if (len >= MAX_SIEGE_INFO_SIZE)
	{
		trap->FS_Close(f);
		goto failure;
	}

	/* Read into global siege_info buffer */
	trap->FS_Read(siege_info, len, f);
	trap->FS_Close(f);

	siege_valid = qtrue;

	/* ---------------------------------------------------------
	   Parse "Teams" section
	   --------------------------------------------------------- */
	if (BG_SiegeGetValueGroup(siege_info, "Teams", teams))
	{
		/* Team 1 override or default */
		trap->Cvar_VariableStringBuffer("cg_siegeTeam1", buf, sizeof(buf));
		if (buf[0] && Q_stricmp(buf, "none"))
		{
			Q_strncpyz(team1, buf, sizeof(team1));
		}
		else
		{
			BG_SiegeGetPairedValue(teams, "team1", team1);
		}

		/* Team 1 name (stringed reference or literal) */
		if (team1[0] == '@')
		{
			trap->SE_GetStringTextString(team1 + 1, b, sizeof(b));
			trap->Cvar_Set("cg_siegeTeam1Name", b);
		}
		else
		{
			trap->Cvar_Set("cg_siegeTeam1Name", team1);
		}

		/* Team 2 override or default */
		trap->Cvar_VariableStringBuffer("cg_siegeTeam2", buf, sizeof(buf));
		if (buf[0] && Q_stricmp(buf, "none"))
		{
			Q_strncpyz(team2, buf, sizeof(team2));
		}
		else
		{
			BG_SiegeGetPairedValue(teams, "team2", team2);
		}

		/* Team 2 name */
		if (team2[0] == '@')
		{
			trap->SE_GetStringTextString(team2 + 1, b, sizeof(b));
			trap->Cvar_Set("cg_siegeTeam2Name", b);
		}
		else
		{
			trap->Cvar_Set("cg_siegeTeam2Name", team2);
		}
	}
	else
	{
		trap->Error(ERR_DROP, "Siege teams not defined");
	}

	/* ---------------------------------------------------------
	   Team 1 info
	   --------------------------------------------------------- */
	if (BG_SiegeGetValueGroup(siege_info, team1, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "TeamIcon", teamIcon))
		{
			trap->Cvar_Set("team1_icon", teamIcon);
		}

		if (BG_SiegeGetPairedValue(teamInfo, "Timed", btime))
		{
			team1Timed = atoi(btime) * 1000;
			CG_SetSiegeTimerCvar(team1Timed);
		}
		else
		{
			team1Timed = 0;
		}
	}
	else
	{
		trap->Error(ERR_DROP, "No team entry for '%s'\n", team1);
	}

	/* Map graphic */
	if (BG_SiegeGetPairedValue(siege_info, "mapgraphic", teamInfo))
	{
		trap->Cvar_Set("siege_mapgraphic", teamInfo);
	}
	else
	{
		trap->Cvar_Set("siege_mapgraphic", "gfx/mplevels/siege1_hoth");
	}

	/* Mission name */
	if (BG_SiegeGetPairedValue(siege_info, "missionname", teamInfo))
	{
		trap->Cvar_Set("siege_missionname", teamInfo);
	}
	else
	{
		trap->Cvar_Set("siege_missionname", " ");
	}

	/* ---------------------------------------------------------
	   Team 2 info
	   --------------------------------------------------------- */
	if (BG_SiegeGetValueGroup(siege_info, team2, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "TeamIcon", teamIcon))
		{
			trap->Cvar_Set("team2_icon", teamIcon);
		}

		if (BG_SiegeGetPairedValue(teamInfo, "Timed", btime))
		{
			team2Timed = atoi(btime) * 1000;
			CG_SetSiegeTimerCvar(team2Timed);
		}
		else
		{
			team2Timed = 0;
		}
	}
	else
	{
		trap->Error(ERR_DROP, "No team entry for '%s'\n", team2);
	}

	/* Load classes */
	BG_SiegeLoadClasses(NULL);

	if (!bgNumSiegeClasses)
	{
		trap->Error(ERR_DROP, "Couldn't find any player classes for Siege");
	}

	/* Load teams */
	BG_SiegeLoadTeams();

	if (!bgNumSiegeTeams)
	{
		trap->Error(ERR_DROP, "Couldn't find any player teams for Siege");
	}

	/* Team themes */
	if (BG_SiegeGetValueGroup(siege_info, team1, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "UseTeam", btime))
		{
			BG_SiegeSetTeamTheme(SIEGETEAM_TEAM1, btime);
		}

		if (BG_SiegeGetPairedValue(teamInfo, "FriendlyShader", btime))
		{
			cgSiegeTeam1PlShader = trap->R_RegisterShaderNoMip(btime);
		}
		else
		{
			cgSiegeTeam1PlShader = 0;
		}
	}

	if (BG_SiegeGetValueGroup(siege_info, team2, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "UseTeam", btime))
		{
			BG_SiegeSetTeamTheme(SIEGETEAM_TEAM2, btime);
		}

		if (BG_SiegeGetPairedValue(teamInfo, "FriendlyShader", btime))
		{
			cgSiegeTeam2PlShader = trap->R_RegisterShaderNoMip(btime);
		}
		else
		{
			cgSiegeTeam2PlShader = 0;
		}
	}

	/* Precache forced models/skins */
	for (int i = SIEGETEAM_TEAM1; i <= SIEGETEAM_TEAM2; i++)
	{
		const siegeTeam_t* sTeam = BG_SiegeFindThemeForTeam(i);

		if (!sTeam)
		{
			continue;
		}

		if (i == SIEGETEAM_TEAM1)
		{
			cgSiegeTeam1PlShader = sTeam->friendlyShader;
		}
		else
		{
			cgSiegeTeam2PlShader = sTeam->friendlyShader;
		}

		for (int j = 0; j < sTeam->numClasses; j++)
		{
			siegeClass_t* cl = sTeam->classes[j];

			if (cl->forcedModel[0])
			{
				trap->R_RegisterModel(va("models/players/%s/model.glm", cl->forcedModel));

				if (cl->forcedSkin[0])
				{
					char* useSkinName;

					if (strchr(cl->forcedSkin, '|'))
					{
						useSkinName = va("models/players/%s/|%s", cl->forcedModel, cl->forcedSkin);
					}
					else
					{
						useSkinName = va("models/players/%s/model_%s.skin", cl->forcedModel, cl->forcedSkin);
					}

					trap->R_RegisterSkin(useSkinName);
				}
			}
		}
	}

	/* Sabers + players */
	BG_PrecacheSabersForSiegeTeam(SIEGETEAM_TEAM1);
	BG_PrecacheSabersForSiegeTeam(SIEGETEAM_TEAM2);

	CG_PrecachePlayersForSiegeTeam(SIEGETEAM_TEAM1);
	CG_PrecachePlayersForSiegeTeam(SIEGETEAM_TEAM2);

	CG_PrecachePlayersForSiegeTeam(SIEGETEAM_TEAM1);
	CG_PrecachePlayersForSiegeTeam(SIEGETEAM_TEAM2);

	/* Objectives */
	CG_PrecacheSiegeObjectiveAssetsForTeam(SIEGETEAM_TEAM1);
	CG_PrecacheSiegeObjectiveAssetsForTeam(SIEGETEAM_TEAM2);

	return;

failure:
	siege_valid = qfalse;
}

static char QINLINE* CG_SiegeObjectiveBuffer(const int team, const int objective)
{
	char teamstr[1024];

	if (team == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof teamstr, team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof teamstr, team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{
		static char buf[8192];
		//found the team group
		if (BG_SiegeGetValueGroup(cgParseObjectives, va("Objective%i", objective), buf))
		{
			//found the objective group
			return buf;
		}
	}

	return NULL;
}

void CG_ParseSiegeObjectiveStatus(const char* str)
{
	int i = 0;
	int team = SIEGETEAM_TEAM1;
	int objectiveNum = 0;

	if (!str || !str[0])
	{
		return;
	}

	while (str[i])
	{
		if (str[i] == '|')
		{
			//switch over to team2, this is the next section
			team = SIEGETEAM_TEAM2;
			objectiveNum = 0;
		}
		else if (str[i] == '-')
		{
			objectiveNum++;
			i++;

			const char* cvarName = va("team%i_objective%i", team, objectiveNum);
			if (str[i] == '1')
			{
				//it's completed
				trap->Cvar_Set(cvarName, "1");
			}
			else
			{
				//otherwise assume it is not
				trap->Cvar_Set(cvarName, "0");
			}

			const char* s = CG_SiegeObjectiveBuffer(team, objectiveNum);
			if (s && s[0])
			{
				//now set the description and graphic cvars to by read by the menu
				char buffer[8192];

				cvarName = va("team%i_objective%i_longdesc", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "objdesc", buffer))
				{
					trap->Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap->Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_gfx", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "objgfx", buffer))
				{
					trap->Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap->Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_mapicon", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "mapicon", buffer))
				{
					trap->Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap->Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_litmapicon", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "litmapicon", buffer))
				{
					trap->Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap->Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_donemapicon", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "donemapicon", buffer))
				{
					trap->Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap->Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_mappos", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "mappos", buffer))
				{
					trap->Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap->Cvar_Set(cvarName, "0 0 32 32");
				}
			}
		}
		i++;
	}

	if (cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{
		//update menu cvars
		CG_SiegeBriefingDisplay(cg.predictedPlayerState.persistant[PERS_TEAM], 1);
	}
}

void CG_SiegeRoundOver(centity_t* ent, const int won)
{
	char teamstr[64];
	const playerState_t* ps;

	if (!siege_valid)
	{
		trap->Error(ERR_DROP, "ERROR: Siege data does not exist on client!\n");
		return;
	}

	if (cg.snap)
	{
		//this should always be true, if it isn't though use the predicted ps as a fallback
		ps = &cg.snap->ps;
	}
	else
	{
		ps = &cg.predictedPlayerState;
	}

	if (!ps)
	{
		assert(0);
		return;
	}

	const int myTeam = ps->persistant[PERS_TEAM];

	if (myTeam == TEAM_SPECTATOR)
	{
		return;
	}

	if (myTeam == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof teamstr, team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof teamstr, team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{
		char soundstr[1024] = { 0 };
		char appstring[1024] = { 0 };
		int success;
		if (won == myTeam)
		{
			success = BG_SiegeGetPairedValue(cgParseObjectives, "wonround", appstring);
		}
		else
		{
			success = BG_SiegeGetPairedValue(cgParseObjectives, "lostround", appstring);
		}

		if (success)
		{
			CG_DrawSiegeMessage(appstring, 0);
		}

		appstring[0] = 0;
		soundstr[0] = 0;

		if (myTeam == won)
		{
			Com_sprintf(teamstr, sizeof teamstr, "roundover_sound_wewon");
		}
		else
		{
			Com_sprintf(teamstr, sizeof teamstr, "roundover_sound_welost");
		}

		if (BG_SiegeGetPairedValue(cgParseObjectives, teamstr, appstring))
		{
			Com_sprintf(soundstr, sizeof soundstr, appstring);
		}

		if (soundstr[0])
		{
			trap->S_StartLocalSound(trap->S_RegisterSound(soundstr), CHAN_ANNOUNCER);
		}
	}
}

static void CG_SiegeGetObjectiveDescription(const int team, const int objective, char* buffer)
{
	char teamstr[1024];

	buffer[0] = 0; //set to 0 ahead of time in case we fail to find the objective group/name

	if (team == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof teamstr, team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof teamstr, team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{
		char objectiveStr[8192];
		//found the team group
		if (BG_SiegeGetValueGroup(cgParseObjectives, va("Objective%i", objective), objectiveStr))
		{
			//found the objective group
			//Parse the name right into the buffer.
			BG_SiegeGetPairedValue(objectiveStr, "goalname", buffer);
		}
	}
}

static int CG_SiegeGetObjectiveFinal(const int team, const int objective)
{
	char teamstr[1024];

	if (team == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof teamstr, team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof teamstr, team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{
		char objectiveStr[8192];
		//found the team group
		if (BG_SiegeGetValueGroup(cgParseObjectives, va("Objective%i", objective), objectiveStr))
		{
			char finalStr[64];
			//found the objective group
			//Parse the name right into the buffer.
			BG_SiegeGetPairedValue(objectiveStr, "final", finalStr);
			return atoi(finalStr);
		}
	}
	return 0;
}

void CG_SiegeBriefingDisplay(const int team, const int dontshow)
{
	char teamstr[64];
	char properValue[1024] = { 0 };
	int i = 1;
	int useTeam = team;

	if (!siege_valid)
	{
		return;
	}

	if (team == TEAM_SPECTATOR)
	{
		return;
	}

	if (team == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof teamstr, team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof teamstr, team2);
	}

	if (useTeam != SIEGETEAM_TEAM1 && useTeam != SIEGETEAM_TEAM2)
	{
		//This shouldn't be happening. But just fall back to team 2 anyway.
		useTeam = SIEGETEAM_TEAM2;
	}

	trap->Cvar_Set(va("siege_primobj_inuse"), "0");

	while (i < 16)
	{
		char objectiveDesc[1024];
		//do up to 16 objectives I suppose
		//Get the value for this objective on this team
		//Now set the cvar for the menu to display.

		//primary = (CG_SiegeGetObjectiveFinal(useTeam, i)>-1)?qtrue:qfalse;
		const qboolean primary = CG_SiegeGetObjectiveFinal(useTeam, i) > 0 ? qtrue : qfalse;

		properValue[0] = 0;
		trap->Cvar_VariableStringBuffer(va("team%i_objective%i", useTeam, i), properValue, 1024);
		if (primary)
		{
			trap->Cvar_Set(va("siege_primobj"), properValue);
		}
		else
		{
			trap->Cvar_Set(va("siege_objective%i", i), properValue);
		}

		//Now set the long desc cvar for the menu to display.
		properValue[0] = 0;
		trap->Cvar_VariableStringBuffer(va("team%i_objective%i_longdesc", useTeam, i), properValue, 1024);
		if (primary)
		{
			trap->Cvar_Set(va("siege_primobj_longdesc"), properValue);
		}
		else
		{
			trap->Cvar_Set(va("siege_objective%i_longdesc", i), properValue);
		}

		//Now set the gfx cvar for the menu to display.
		properValue[0] = 0;
		trap->Cvar_VariableStringBuffer(va("team%i_objective%i_gfx", useTeam, i), properValue, 1024);
		if (primary)
		{
			trap->Cvar_Set(va("siege_primobj_gfx"), properValue);
		}
		else
		{
			trap->Cvar_Set(va("siege_objective%i_gfx", i), properValue);
		}

		//Now set the mapicon cvar for the menu to display.
		properValue[0] = 0;
		trap->Cvar_VariableStringBuffer(va("team%i_objective%i_mapicon", useTeam, i), properValue, 1024);
		if (primary)
		{
			trap->Cvar_Set(va("siege_primobj_mapicon"), properValue);
		}
		else
		{
			trap->Cvar_Set(va("siege_objective%i_mapicon", i), properValue);
		}

		//Now set the mappos cvar for the menu to display.
		properValue[0] = 0;
		trap->Cvar_VariableStringBuffer(va("team%i_objective%i_mappos", useTeam, i), properValue, 1024);
		if (primary)
		{
			trap->Cvar_Set(va("siege_primobj_mappos"), properValue);
		}
		else
		{
			trap->Cvar_Set(va("siege_objective%i_mappos", i), properValue);
		}

		//Now set the description cvar for the objective
		CG_SiegeGetObjectiveDescription(useTeam, i, objectiveDesc);

		if (objectiveDesc[0])
		{
			//found a valid objective description
			if (primary)
			{
				trap->Cvar_Set(va("siege_primobj_desc"), objectiveDesc);
				//this one is marked not in use because it gets primobj
				trap->Cvar_Set(va("siege_objective%i_inuse", i), "0");
				trap->Cvar_Set(va("siege_primobj_inuse"), "1");

				trap->Cvar_Set(va("team%i_objective%i_inuse", useTeam, i), "1");
			}
			else
			{
				trap->Cvar_Set(va("siege_objective%i_desc", i), objectiveDesc);
				trap->Cvar_Set(va("siege_objective%i_inuse", i), "2");
				trap->Cvar_Set(va("team%i_objective%i_inuse", useTeam, i), "2");
			}
		}
		else
		{
			//didn't find one, so set the "inuse" cvar to 0 for the objective and mark it non-complete.
			trap->Cvar_Set(va("siege_objective%i_inuse", i), "0");
			trap->Cvar_Set(va("siege_objective%i", i), "0");
			trap->Cvar_Set(va("team%i_objective%i_inuse", useTeam, i), "0");
			trap->Cvar_Set(va("team%i_objective%i", useTeam, i), "0");

			trap->Cvar_Set(va("siege_objective%i_mappos", i), "");
			trap->Cvar_Set(va("team%i_objective%i_mappos", useTeam, i), "");
			trap->Cvar_Set(va("siege_objective%i_gfx", i), "");
			trap->Cvar_Set(va("team%i_objective%i_gfx", useTeam, i), "");
			trap->Cvar_Set(va("siege_objective%i_mapicon", i), "");
			trap->Cvar_Set(va("team%i_objective%i_mapicon", useTeam, i), "");
		}

		i++;
	}

	if (dontshow)
	{
		return;
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{
		char briefing[8192];
		if (BG_SiegeGetPairedValue(cgParseObjectives, "briefing", briefing))
		{
			CG_DrawSiegeMessage(briefing, 1);
		}
	}
}

/*
===============================
CG_SiegeObjectiveCompleted
- Displays message and plays sound when a siege objective is completed.
- Fixed MSVC C6262 by moving large buffers off the stack.
- Replaced assert with debug print.
===============================
*/
void CG_SiegeObjectiveCompleted(centity_t* ent, const int won, const int objectivenum)
{
	/* Large buffers moved off stack → static storage (BSS) */
	static char foundobjective[MAX_SIEGE_INFO_SIZE];
	static char objstr[256];
	static char soundstr[1024];
	static char appstring[1024];

	char teamstr[64];
	const playerState_t* ps = NULL;

	/* Validate siege data */
	if (siege_valid == qfalse)
	{
		trap->Error(ERR_DROP, "Siege data does not exist on client!\n");
		return;
	}

	/* Prefer snapshot playerstate */
	if (cg.snap != NULL)
	{
		ps = &cg.snap->ps;
	}
	else
	{
		ps = &cg.predictedPlayerState;
	}

	if (ps == NULL)
	{
		Com_Printf(S_COLOR_RED "CG_SiegeObjectiveCompleted: playerState is NULL — aborting.\n");
		return;
	}

	const int myTeam = ps->persistant[PERS_TEAM];

	/* Spectators do not receive objective notifications */
	if (myTeam == TEAM_SPECTATOR)
	{
		return;
	}

	/* Determine team string */
	if (won == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	/* Parse objective group for this team */
	const qboolean gotGroup =
		BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives) ? qtrue : qfalse;

	if (gotGroup == qfalse)
	{
		return;
	}

	/* Build "ObjectiveX" key */
	Com_sprintf(objstr, sizeof(objstr), "Objective%i", objectivenum);

	const qboolean gotObjective =
		BG_SiegeGetValueGroup(cgParseObjectives, objstr, foundobjective) ? qtrue : qfalse;

	if (gotObjective == qfalse)
	{
		return;
	}

	/* ---------------------------------------------------------
	   Display team‑specific completion message
	   --------------------------------------------------------- */
	appstring[0] = '\0';
	soundstr[0] = '\0';

	qboolean success = qfalse;

	if (myTeam == SIEGETEAM_TEAM1)
	{
		success = BG_SiegeGetPairedValue(foundobjective, "message_team1", appstring) ? qtrue : qfalse;
	}
	else
	{
		success = BG_SiegeGetPairedValue(foundobjective, "message_team2", appstring) ? qtrue : qfalse;
	}

	if (success == qtrue)
	{
		CG_DrawSiegeMessageNonMenu(appstring);
	}

	/* Reset for sound lookup */
	appstring[0] = '\0';
	soundstr[0] = '\0';

	/* ---------------------------------------------------------
	   Resolve and play team‑specific completion sound
	   --------------------------------------------------------- */
	if (myTeam == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), "sound_team1");
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), "sound_team2");
	}

	if (BG_SiegeGetPairedValue(foundobjective, teamstr, appstring))
	{
		Com_sprintf(soundstr, sizeof(soundstr), "%s", appstring);
	}

	if (soundstr[0] != '\0')
	{
		trap->S_StartLocalSound(trap->S_RegisterSound(soundstr), CHAN_ANNOUNCER);
	}
}

siegeExtended_t cg_siegeExtendedData[MAX_CLIENTS];

//parse a single extended siege data entry
static void CG_ParseSiegeExtendedDataEntry(const char* conStr)
{
	char s[MAX_STRING_CHARS] = { 0 };
	char* str = (char*)conStr;
	int argParses = 0;
	int clNum = -1, health = 1, maxhealth = 1, ammo = 1;

	if (!conStr || !conStr[0])
	{
		return;
	}

	while (*str && argParses < 4)
	{
		int i = 0;
		while (*str && *str != '|')
		{
			s[i] = *str;
			i++;
			str++;
		}
		s[i] = 0;
		switch (argParses)
		{
		case 0:
			clNum = atoi(s);
			break;
		case 1:
			health = atoi(s);
			break;
		case 2:
			maxhealth = atoi(s);
			break;
		case 3:
			ammo = atoi(s);
			break;
		default:
			break;
		}
		argParses++;
		str++;
	}

	if (clNum < 0 || clNum >= MAX_CLIENTS)
	{
		return;
	}

	cg_siegeExtendedData[clNum].health = health;
	cg_siegeExtendedData[clNum].maxhealth = maxhealth;
	cg_siegeExtendedData[clNum].ammo = ammo;

	const centity_t* cent = &cg_entities[clNum];

	int maxAmmo = ammoData[weaponData[cent->currentState.weapon].ammoIndex].max;
	if (cent->currentState.eFlags & EF_DOUBLE_AMMO)
	{
		maxAmmo *= 2.0f;
	}
	if (ammo >= 0 && ammo <= maxAmmo)
	{
		//assure the weapon number is valid and not over max
		//keep the weapon so if it changes before our next ext data update we'll know
		//that the ammo is not applicable.
		cg_siegeExtendedData[clNum].weapon = cent->currentState.weapon;
	}
	else
	{
		//not valid? Oh well, just invalidate the weapon too then so we don't display ammo
		cg_siegeExtendedData[clNum].weapon = -1;
	}

	cg_siegeExtendedData[clNum].lastUpdated = cg.time;
}

//parse incoming siege data, see counterpart in g_saga.c
void CG_ParseSiegeExtendedData(void)
{
	const int numEntries = trap->Cmd_Argc();
	int i = 0;

	if (numEntries < 1)
	{
		assert(!"Bad numEntries for sxd");
		return;
	}

	while (i < numEntries)
	{
		CG_ParseSiegeExtendedDataEntry(CG_Argv(i + 1));
		i++;
	}
}

void CG_SetSiegeTimerCvar(const int msec)
{
	int seconds = msec / 1000;
	const int mins = seconds / 60;
	seconds -= mins * 60;
	const int tens = seconds / 10;
	seconds -= tens * 10;

	trap->Cvar_Set("ui_siegeTimer", va("%i:%i%i", mins, tens, seconds));
}

void CG_TrueViewInit(void)
{
	fileHandle_t f;

	const int len = trap->FS_Open("trueview.cfg", &f, FS_READ);

	if (!f)
	{
		true_view_valid = 0;
		return;
	}

	if (len >= MAX_TRUEVIEW_INFO_SIZE)
	{
		trap->FS_Close(f);
		true_view_valid = 0;
		return;
	}

	trap->FS_Read(true_view_info, len, f);

	true_view_valid = 1;

	trap->FS_Close(f);
}

//Tries to adjust the eye position from the data in cfg file if possible.
void CG_AdjustEyePos(const char* modelName)
{
	//eye position

	if (true_view_valid)
	{
		char eyepos[MAX_QPATH];
		if (BG_SiegeGetPairedValue(true_view_info, (char*)modelName, eyepos))
		{
			trap->Cvar_Set("cg_trueeyeposition", eyepos);
		}
		else
		{
			//Couldn't find an entry for the desired model.  Not nessicarily a bad thing.
			trap->Cvar_Set("cg_trueeyeposition", "0");
		}
	}
	else
	{
		//The model eye position list is messed up.  Default to 0.0 for the eye position
		trap->Cvar_Set("cg_trueeyeposition", "0");
	}
}