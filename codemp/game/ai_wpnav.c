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

#include "g_local.h"
#include "qcommon/q_shared.h"
#include "botlib/botlib.h"
#include "ai_main.h"

float gWPRenderTime = 0;
float gDeactivated = 0;
float gBotEdit = 0;
int gWPRenderedFrame = 0;

#define WPARRAY_BUFFER_SIZE 524288
wpobject_t* gWPArray[MAX_WPARRAY_SIZE];
int gWPNum = 0;

int gLastPrintedIndex = -1;

nodeobject_t nodetable[MAX_NODETABLE_SIZE];
int nodenum; //so we can connect broken trails

int gLevelFlags = 0;

static char* GetFlagStr(const int flags)
{
	char* flagstr = B_TempAlloc(128);
	int i = 0;

	if (!flags)
	{
		strcpy(flagstr, "none\0");
		goto fend;
	}

	if (flags & WPFLAG_JUMP)
	{
		flagstr[i] = 'j';
		i++;
	}

	if (flags & WPFLAG_DUCK)
	{
		flagstr[i] = 'd';
		i++;
	}

	if (flags & WPFLAG_SNIPEORCAMPSTAND)
	{
		flagstr[i] = 'c';
		i++;
	}

	if (flags & WPFLAG_WAITFORFUNC)
	{
		flagstr[i] = 'f';
		i++;
	}

	if (flags & WPFLAG_SNIPEORCAMP)
	{
		flagstr[i] = 's';
		i++;
	}

	if (flags & WPFLAG_ONEWAY_FWD)
	{
		flagstr[i] = 'x';
		i++;
	}

	if (flags & WPFLAG_ONEWAY_BACK)
	{
		flagstr[i] = 'y';
		i++;
	}

	if (flags & WPFLAG_GOALPOINT)
	{
		flagstr[i] = 'g';
		i++;
	}

	if (flags & WPFLAG_NOVIS)
	{
		flagstr[i] = 'n';
		i++;
	}

	if (flags & WPFLAG_NOMOVEFUNC)
	{
		flagstr[i] = 'm';
		i++;
	}
	if (flags & WPFLAG_DESTROY_FUNCBREAK)
	{
		flagstr[i] = 'q';
		i++;
	}

	if (flags & WPFLAG_REDONLY)
	{
		flagstr[i] = 'r';
		i++;
	}

	if (flags & WPFLAG_BLUEONLY)
	{
		flagstr[i] = 'b';
		i++;
	}

	if (flags & WPFLAG_FORCEPUSH)
	{
		flagstr[i] = 'p';
		i++;
	}

	if (flags & WPFLAG_FORCEPULL)
	{
		flagstr[i] = 'o';
		i++;
	}

	if (flags & WPFLAG_RED_FLAG)
	{
		if (i)
		{
			flagstr[i] = ' ';
			i++;
		}
		flagstr[i] = 'r';
		i++;
		flagstr[i] = 'e';
		i++;
		flagstr[i] = 'd';
		i++;
		flagstr[i] = ' ';
		i++;
		flagstr[i] = 'f';
		i++;
		flagstr[i] = 'l';
		i++;
		flagstr[i] = 'a';
		i++;
		flagstr[i] = 'g';
		i++;
	}

	if (flags & WPFLAG_BLUE_FLAG)
	{
		if (i)
		{
			flagstr[i] = ' ';
			i++;
		}
		flagstr[i] = 'b';
		i++;
		flagstr[i] = 'l';
		i++;
		flagstr[i] = 'u';
		i++;
		flagstr[i] = 'e';
		i++;
		flagstr[i] = ' ';
		i++;
		flagstr[i] = 'f';
		i++;
		flagstr[i] = 'l';
		i++;
		flagstr[i] = 'a';
		i++;
		flagstr[i] = 'g';
		i++;
	}

	if (flags & WPFLAG_SIEGE_IMPERIALOBJ)
	{
		if (i)
		{
			flagstr[i] = ' ';
			i++;
		}
		flagstr[i] = 's';
		i++;
		flagstr[i] = 'a';
		i++;
		flagstr[i] = 'g';
		i++;
		flagstr[i] = 'a';
		i++;
		flagstr[i] = '_';
		i++;
		flagstr[i] = 'i';
		i++;
		flagstr[i] = 'm';
		i++;
		flagstr[i] = 'p';
		i++;
	}

	if (flags & WPFLAG_SIEGE_REBELOBJ)
	{
		if (i)
		{
			flagstr[i] = ' ';
			i++;
		}
		flagstr[i] = 's';
		i++;
		flagstr[i] = 'a';
		i++;
		flagstr[i] = 'g';
		i++;
		flagstr[i] = 'a';
		i++;
		flagstr[i] = '_';
		i++;
		flagstr[i] = 'r';
		i++;
		flagstr[i] = 'e';
		i++;
		flagstr[i] = 'b';
		i++;
	}

	flagstr[i] = '\0';

	if (i == 0)
	{
		strcpy(flagstr, "unknown\0");
	}

fend:
	return flagstr;
}

void G_TestLine(vec3_t start, vec3_t end, const int color, const int time)
{
	gentity_t* te = G_TempEntity(start, EV_TESTLINE);
	VectorCopy(start, te->s.origin);
	VectorCopy(end, te->s.origin2);
	te->s.time2 = time;
	te->s.weapon = color;
	te->r.svFlags |= SVF_BROADCAST;
}

void G_BlockLine(vec3_t start, vec3_t end, const int color, const int time)
{
	gentity_t* te = G_TempEntity(start, EV_BLOCKLINE);
	VectorCopy(start, te->s.origin);
	VectorCopy(end, te->s.origin2);
	te->s.time2 = time;
	te->s.weapon = color;
	te->r.svFlags |= SVF_BROADCAST;
}

void AutosaveRender();
void SpawnPointRender();

void BotWaypointRender(void)
{
	int i;
	gentity_t* plum;

	if (!gBotEdit)
	{
		return;
	}

	int bestindex = 0;

	if (gWPRenderTime > level.time)
	{
		goto checkprint;
	}

	gWPRenderTime = level.time + 100;

	i = gWPRenderedFrame;
	const int inc_checker = gWPRenderedFrame;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			plum = G_TempEntity(gWPArray[i]->origin, EV_SCOREPLUM);
			plum->r.svFlags |= SVF_BROADCAST;
			plum->s.time = i;

			int n = 0;

			while (n < gWPArray[i]->neighbornum)
			{
				if (gWPArray[i]->neighbors[n].forceJumpTo && gWPArray[gWPArray[i]->neighbors[n].num])
				{
					G_TestLine(gWPArray[i]->origin, gWPArray[gWPArray[i]->neighbors[n].num]->origin, 0x0000ff, 5000);
				}
				n++;
			}

			gWPRenderedFrame++;
		}
		else
		{
			gWPRenderedFrame = 0;
			break;
		}

		if (i - inc_checker > 4)
		{
			break; //don't render too many at once
		}
		i++;
	}

	if (i >= gWPNum)
	{
		gWPRenderedFrame = 0;
	}

	AutosaveRender();
	SpawnPointRender();

checkprint:

	if (!bot_wp_info.value)
	{
		return;
	}

	const gentity_t* viewent = &g_entities[0]; //only show info to the first client

	if (!viewent || !viewent->client)
	{
		//client isn't in the game yet?
		return;
	}

	float bestdist = 256; //max distance for showing point info
	int gotbestindex = 0;

	i = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			vec3_t a;
			VectorSubtract(viewent->client->ps.origin, gWPArray[i]->origin, a);

			const float checkdist = VectorLength(a);

			if (checkdist < bestdist)
			{
				bestdist = checkdist;
				bestindex = i;
				gotbestindex = 1;
			}
		}
		i++;
	}

	if (gotbestindex && bestindex != gLastPrintedIndex)
	{
		char* flagstr = GetFlagStr(gWPArray[bestindex]->flags);
		gLastPrintedIndex = bestindex;
		trap->Print(S_COLOR_YELLOW "Waypoint %i\nFlags - %i (%s) (w%f)\nOrigin - (%i %i %i)\n",
			gWPArray[bestindex]->index, gWPArray[bestindex]->flags, flagstr, gWPArray[bestindex]->weight,
			(int)gWPArray[bestindex]->origin[0], (int)gWPArray[bestindex]->origin[1],
			(int)gWPArray[bestindex]->origin[2]);
		//GetFlagStr allocates 128 bytes for this, if it's changed then obviously this must be as well
		B_TempFree(128); //flagstr

		plum = G_TempEntity(gWPArray[bestindex]->origin, EV_SCOREPLUM);
		plum->r.svFlags |= SVF_BROADCAST;
		plum->s.time = bestindex; //render it once
	}
	else if (!gotbestindex)
	{
		gLastPrintedIndex = -1;
	}
}

static void TransferWPData(const int from, const int to)
{
	if (!gWPArray[to])
	{
		gWPArray[to] = (wpobject_t*)B_Alloc(sizeof(wpobject_t));
	}

	if (!gWPArray[to])
	{
		trap->Print(S_COLOR_RED "FATAL ERROR: Could not allocated memory for waypoint\n");
	}

	gWPArray[to]->flags = gWPArray[from]->flags;
	gWPArray[to]->weight = gWPArray[from]->weight;
	gWPArray[to]->associated_entity = gWPArray[from]->associated_entity;
	gWPArray[to]->disttonext = gWPArray[from]->disttonext;
	gWPArray[to]->forceJumpTo = gWPArray[from]->forceJumpTo;
	gWPArray[to]->index = to;
	gWPArray[to]->inuse = gWPArray[from]->inuse;
	VectorCopy(gWPArray[from]->origin, gWPArray[to]->origin);
}

static void CreateNewWP(vec3_t origin, const int flags)
{
	if (gWPNum >= MAX_WPARRAY_SIZE)
	{
		if (!RMG.integer)
		{
			trap->Print(S_COLOR_YELLOW "Warning: Waypoint limit hit (%i)\n", MAX_WPARRAY_SIZE);
		}
		return;
	}

	if (!gWPArray[gWPNum])
	{
		gWPArray[gWPNum] = (wpobject_t*)B_Alloc(sizeof(wpobject_t));
	}

	if (!gWPArray[gWPNum])
	{
		trap->Print(S_COLOR_RED "ERROR: Could not allocated memory for waypoint\n");
	}

	gWPArray[gWPNum]->flags = flags;
	gWPArray[gWPNum]->weight = 0; //calculated elsewhere
	gWPArray[gWPNum]->associated_entity = ENTITYNUM_NONE; //set elsewhere
	gWPArray[gWPNum]->forceJumpTo = 0;
	gWPArray[gWPNum]->disttonext = 0; //calculated elsewhere
	gWPArray[gWPNum]->index = gWPNum;
	gWPArray[gWPNum]->inuse = 1;
	VectorCopy(origin, gWPArray[gWPNum]->origin);
	gWPNum++;
}

static void CreateNewWP_FromObject(const wpobject_t* wp)
{
	if (gWPNum >= MAX_WPARRAY_SIZE)
	{
		return;
	}

	if (!gWPArray[gWPNum])
	{
		gWPArray[gWPNum] = (wpobject_t*)B_Alloc(sizeof(wpobject_t));
	}

	if (!gWPArray[gWPNum])
	{
		trap->Print(S_COLOR_RED "ERROR: Could not allocated memory for waypoint\n");
	}

	gWPArray[gWPNum]->flags = wp->flags;
	gWPArray[gWPNum]->weight = wp->weight;
	gWPArray[gWPNum]->associated_entity = wp->associated_entity;
	gWPArray[gWPNum]->disttonext = wp->disttonext;
	gWPArray[gWPNum]->forceJumpTo = wp->forceJumpTo;
	gWPArray[gWPNum]->index = gWPNum;
	gWPArray[gWPNum]->inuse = 1;
	VectorCopy(wp->origin, gWPArray[gWPNum]->origin);
	gWPArray[gWPNum]->neighbornum = wp->neighbornum;

	int i = wp->neighbornum;

	while (i >= 0)
	{
		gWPArray[gWPNum]->neighbors[i].num = wp->neighbors[i].num;
		gWPArray[gWPNum]->neighbors[i].forceJumpTo = wp->neighbors[i].forceJumpTo;

		i--;
	}

	if (gWPArray[gWPNum]->flags & WPFLAG_RED_FLAG)
	{
		flagRed = gWPArray[gWPNum];
		oFlagRed = flagRed;
	}
	else if (gWPArray[gWPNum]->flags & WPFLAG_BLUE_FLAG)
	{
		flagBlue = gWPArray[gWPNum];
		oFlagBlue = flagBlue;
	}

	gWPNum++;
}

static void RemoveWP(void)
{
	if (gWPNum <= 0)
	{
		return;
	}

	gWPNum--;

	if (!gWPArray[gWPNum] || !gWPArray[gWPNum]->inuse)
	{
		return;
	}

	//B_Free((wpobject_t *)gWPArray[gWPNum]);
	if (gWPArray[gWPNum])
	{
		memset(gWPArray[gWPNum], 0, sizeof * gWPArray[gWPNum]);
	}

	//gWPArray[gWPNum] = NULL;

	if (gWPArray[gWPNum])
	{
		gWPArray[gWPNum]->inuse = 0;
	}
}

void RemoveAllWP(void)
{
	while (gWPNum)
	{
		RemoveWP();
	}
}

static void RemoveWP_InTrail(const int afterindex)
{
	int foundindex = 0;
	int foundanindex = 0;
	int didchange = 0;
	int i = 0;

	if (afterindex < 0 || afterindex >= gWPNum)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint number %i does not exist\n", afterindex);
		return;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index == afterindex)
		{
			foundindex = i;
			foundanindex = 1;
			break;
		}

		i++;
	}

	if (!foundanindex)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint index %i should exist, but does not (?)\n", afterindex);
		return;
	}

	i = 0;

	while (i <= gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->index == foundindex)
		{
			//B_Free(gWPArray[i]);

			//Keep reusing the memory
			memset(gWPArray[i], 0, sizeof * gWPArray[i]);

			//gWPArray[i] = NULL;
			gWPArray[i]->inuse = 0;
			didchange = 1;
		}
		else if (gWPArray[i] && didchange)
		{
			TransferWPData(i, i - 1);
			//B_Free(gWPArray[i]);

			//Keep reusing the memory
			memset(gWPArray[i], 0, sizeof * gWPArray[i]);

			//gWPArray[i] = NULL;
			gWPArray[i]->inuse = 0;
		}

		i++;
	}
	gWPNum--;
}

static int CreateNewWP_InTrail(vec3_t origin, const int flags, const int afterindex)
{
	int foundindex = 0;
	int foundanindex = 0;
	int i = 0;

	if (gWPNum >= MAX_WPARRAY_SIZE)
	{
		if (!RMG.integer)
		{
			trap->Print(S_COLOR_YELLOW "Warning: Waypoint limit hit (%i)\n", MAX_WPARRAY_SIZE);
		}
		return 0;
	}

	if (afterindex < 0 || afterindex >= gWPNum)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint number %i does not exist\n", afterindex);
		return 0;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index == afterindex)
		{
			foundindex = i;
			foundanindex = 1;
			break;
		}

		i++;
	}

	if (!foundanindex)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint index %i should exist, but does not (?)\n", afterindex);
		return 0;
	}

	i = gWPNum;

	while (i >= 0)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index != foundindex)
		{
			TransferWPData(i, i + 1);
		}
		else if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index == foundindex)
		{
			i++;

			if (!gWPArray[i])
			{
				gWPArray[i] = (wpobject_t*)B_Alloc(sizeof(wpobject_t));
			}

			gWPArray[i]->flags = flags;
			gWPArray[i]->weight = 0; //calculated elsewhere
			gWPArray[i]->associated_entity = ENTITYNUM_NONE; //set elsewhere
			gWPArray[i]->disttonext = 0; //calculated elsewhere
			gWPArray[i]->forceJumpTo = 0;
			gWPArray[i]->index = i;
			gWPArray[i]->inuse = 1;
			VectorCopy(origin, gWPArray[i]->origin);
			gWPNum++;
			break;
		}

		i--;
	}

	return 1;
}

static int CreateNewWP_InsertUnder(vec3_t origin, const int flags, const int afterindex)
{
	int foundindex = 0;
	int foundanindex = 0;
	int i = 0;

	if (gWPNum >= MAX_WPARRAY_SIZE)
	{
		if (!RMG.integer)
		{
			trap->Print(S_COLOR_YELLOW "Warning: Waypoint limit hit (%i)\n", MAX_WPARRAY_SIZE);
		}
		return 0;
	}

	if (afterindex < 0 || afterindex >= gWPNum)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint number %i does not exist\n", afterindex);
		return 0;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index == afterindex)
		{
			foundindex = i;
			foundanindex = 1;
			break;
		}

		i++;
	}

	if (!foundanindex)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint index %i should exist, but does not (?)\n", afterindex);
		return 0;
	}

	i = gWPNum;

	while (i >= 0)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index != foundindex)
		{
			TransferWPData(i, i + 1);
		}
		else if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index == foundindex)
		{
			//i++;
			TransferWPData(i, i + 1);

			if (!gWPArray[i])
			{
				gWPArray[i] = (wpobject_t*)B_Alloc(sizeof(wpobject_t));
			}

			gWPArray[i]->flags = flags;
			gWPArray[i]->weight = 0; //calculated elsewhere
			gWPArray[i]->associated_entity = ENTITYNUM_NONE; //set elsewhere
			gWPArray[i]->disttonext = 0; //calculated elsewhere
			gWPArray[i]->forceJumpTo = 0;
			gWPArray[i]->index = i;
			gWPArray[i]->inuse = 1;
			VectorCopy(origin, gWPArray[i]->origin);
			gWPNum++;
			break;
		}

		i--;
	}

	return 1;
}

static void TeleportToWP(const gentity_t* pl, const int afterindex)
{
	if (!pl || !pl->client)
	{
		return;
	}

	int foundindex = 0;
	int foundanindex = 0;
	int i = 0;

	if (afterindex < 0 || afterindex >= gWPNum)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint number %i does not exist\n", afterindex);
		return;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index == afterindex)
		{
			foundindex = i;
			foundanindex = 1;
			break;
		}

		i++;
	}

	if (!foundanindex)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint index %i should exist, but does not (?)\n", afterindex);
		return;
	}

	VectorCopy(gWPArray[foundindex]->origin, pl->client->ps.origin);
}

static void WPFlagsModify(const int wpnum, const int flags)
{
	if (wpnum < 0 || wpnum >= gWPNum || !gWPArray[wpnum] || !gWPArray[wpnum]->inuse)
	{
		trap->Print(S_COLOR_YELLOW "WPFlagsModify: Waypoint %i does not exist\n", wpnum);
		return;
	}

	gWPArray[wpnum]->flags = flags;
}

static int NotWithinRange(const int base, const int extent)
{
	if (extent > base && base + 5 >= extent)
	{
		return 0;
	}

	if (extent < base && base - 5 <= extent)
	{
		return 0;
	}

	return 1;
}

static int NodeHere(vec3_t spot)
{
	int i = 0;

	while (i < nodenum)
	{
		if ((int)nodetable[i].origin[0] == (int)spot[0] &&
			(int)nodetable[i].origin[1] == (int)spot[1])
		{
			if ((int)nodetable[i].origin[2] == (int)spot[2] ||
				(int)nodetable[i].origin[2] < (int)spot[2] && (int)nodetable[i].origin[2] + 5 > (int)spot[2] ||
				(int)nodetable[i].origin[2] > (int)spot[2] && (int)nodetable[i].origin[2] - 5 < (int)spot[2])
			{
				return 1;
			}
		}
		i++;
	}

	return 0;
}

static int CanGetToVector(vec3_t org1, vec3_t org2, vec3_t mins, vec3_t maxs)
{
	trace_t tr;

	trap->Trace(&tr, org1, mins, maxs, org2, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
	{
		return 1;
	}

	return 0;
}

#if 0
static int CanGetToVectorTravel(vec3_t org1, vec3_t org2, vec3_t mins, vec3_t maxs)
{
	trace_t tr;
	vec3_t a, ang, fwd;
	vec3_t midpos, dmid;
	float startheight, midheight, fLen;

	mins[2] = -13;
	maxs[2] = 13;

	trap->Trace(&tr, org1, mins, maxs, org2, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction != 1 || tr.startsolid || tr.allsolid)
	{
		return 0;
	}

	VectorSubtract(org2, org1, a);

	vectoangles(a, ang);

	AngleVectors(ang, fwd, NULL, NULL);

	fLen = VectorLength(a) / 2;

	midpos[0] = org1[0] + fwd[0] * fLen;
	midpos[1] = org1[1] + fwd[1] * fLen;
	midpos[2] = org1[2] + fwd[2] * fLen;

	VectorCopy(org1, dmid);
	dmid[2] -= 1024;

	trap->Trace(&tr, midpos, NULL, NULL, dmid, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	startheight = org1[2] - tr.endpos[2];

	VectorCopy(midpos, dmid);
	dmid[2] -= 1024;

	trap->Trace(&tr, midpos, NULL, NULL, dmid, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	if (tr.startsolid || tr.allsolid)
	{
		return 1;
	}

	midheight = midpos[2] - tr.endpos[2];

	if (midheight > startheight * 2)
	{
		return 0; //too steep of a drop.. can't go on
	}

	return 1;
}
#else
static int CanGetToVectorTravel(vec3_t org1, vec3_t moveTo, vec3_t mins, vec3_t maxs)
//int ExampleAnimEntMove(gentity_t *self, vec3_t moveTo, float stepSize)
{
	trace_t tr;
	vec3_t stepTo;
	vec3_t stepSub;
	vec3_t stepGoal;
	vec3_t workingOrg;
	vec3_t lastIncrement;
	int didMove = 0;
	const int traceMask = MASK_PLAYERSOLID;
	qboolean initialDone = qfalse;

	VectorCopy(org1, workingOrg);
	VectorCopy(org1, lastIncrement);

	VectorCopy(moveTo, stepTo);
	stepTo[2] = workingOrg[2];

	VectorSubtract(stepTo, workingOrg, stepSub);
	float stepSize = VectorLength(stepSub); //make the step size the length of the original positions without Z

	VectorNormalize(stepSub);

	while (!initialDone || didMove)
	{
		vec3_t finalMeasure;
		initialDone = qtrue;
		didMove = 0;

		stepGoal[0] = workingOrg[0] + stepSub[0] * stepSize;
		stepGoal[1] = workingOrg[1] + stepSub[1] * stepSize;
		stepGoal[2] = workingOrg[2] + stepSub[2] * stepSize;

		trap->Trace(&tr, workingOrg, mins, maxs, stepGoal, ENTITYNUM_NONE, traceMask, qfalse, 0, 0);

		if (!tr.startsolid && !tr.allsolid && tr.fraction)
		{
			vec3_t vecSub;
			VectorSubtract(workingOrg, tr.endpos, vecSub);

			if (VectorLength(vecSub) > stepSize / 2)
			{
				workingOrg[0] = tr.endpos[0];
				workingOrg[1] = tr.endpos[1];
				//trap->LinkEntity(self);
				didMove = 1;
			}
		}

		if (didMove != 1)
		{
			//stair check
			vec3_t trFrom;
			vec3_t trTo;
			vec3_t trDir;
			vec3_t vecMeasure;

			VectorCopy(tr.endpos, trFrom);
			trFrom[2] += 16;

			VectorSubtract(/*tr.endpos*/stepGoal, workingOrg, trDir);
			VectorNormalize(trDir);
			trTo[0] = tr.endpos[0] + trDir[0] * 2;
			trTo[1] = tr.endpos[1] + trDir[1] * 2;
			trTo[2] = tr.endpos[2] + trDir[2] * 2;
			trTo[2] += 16;

			VectorSubtract(trFrom, trTo, vecMeasure);

			if (VectorLength(vecMeasure) > 1)
			{
				trap->Trace(&tr, trFrom, mins, maxs, trTo, ENTITYNUM_NONE, traceMask, qfalse, 0, 0);

				if (!tr.startsolid && !tr.allsolid && tr.fraction == 1)
				{
					//clear trace here, probably up a step
					vec3_t trDown;
					vec3_t trUp;
					VectorCopy(tr.endpos, trUp);
					VectorCopy(tr.endpos, trDown);
					trDown[2] -= 16;

					trap->Trace(&tr, trFrom, mins, maxs, trTo, ENTITYNUM_NONE, traceMask, qfalse, 0, 0);

					if (!tr.startsolid && !tr.allsolid)
					{
						//plop us down on the step after moving up
						VectorCopy(tr.endpos, workingOrg);
						//trap->LinkEntity(self);
						didMove = 1;
					}
				}
			}
		}

		VectorSubtract(lastIncrement, workingOrg, finalMeasure);
		const float measureLength = VectorLength(finalMeasure);

		if (!measureLength)
		{
			//no progress, break out. If last movement was a sucess didMove will equal 1.
			break;
		}

		stepSize -= measureLength; //subtract the progress distance from the step size so we don't overshoot the mark.
		if (stepSize <= 0)
		{
			break;
		}

		VectorCopy(workingOrg, lastIncrement);
	}

	return didMove;
}
#endif

static int ConnectTrail(const int startindex, const int endindex, const qboolean behind_the_scenes)
{
	static byte extendednodes[MAX_NODETABLE_SIZE];
	//for storing checked nodes and not trying to extend them each a bazillion times
	float branchDistance;
	float maxDistFactor = 256;
	vec3_t a;
	vec3_t startplace, starttrace;
	vec3_t mins, maxs;
	vec3_t testspot;
	vec3_t validspotpos;
	trace_t tr;

	memset(extendednodes, 0, sizeof extendednodes);

	if (RMG.integer)
	{
		//this might be temporary. Or not.
		if (!(gWPArray[startindex]->flags & WPFLAG_NEVERONEWAY) &&
			!(gWPArray[endindex]->flags & WPFLAG_NEVERONEWAY))
		{
			gWPArray[startindex]->flags |= WPFLAG_ONEWAY_FWD;
			gWPArray[endindex]->flags |= WPFLAG_ONEWAY_BACK;
		}
		return 0;
	}

	if (!RMG.integer)
	{
		branchDistance = TABLE_BRANCH_DISTANCE;
	}
	else
	{
		branchDistance = 512;
		//be less precise here, terrain is fairly broad, and we don't want to take an hour precalculating
	}

	if (RMG.integer)
	{
		maxDistFactor = 700;
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 0;

	nodenum = 0;
	int foundit = 0;

	int i = 0;

	int successnodeindex = 0;

	while (i < MAX_NODETABLE_SIZE) //clear it out before using it
	{
		nodetable[i].flags = 0;
		//		nodetable[i].index = 0;
		nodetable[i].inuse = 0;
		nodetable[i].neighbornum = 0;
		nodetable[i].origin[0] = 0;
		nodetable[i].origin[1] = 0;
		nodetable[i].origin[2] = 0;
		nodetable[i].weight = 0;

		extendednodes[i] = 0;

		i++;
	}

	if (!behind_the_scenes)
	{
		trap->Print(S_COLOR_YELLOW "Point %i is not connected to %i - Repairing...\n", startindex, endindex);
	}

	VectorCopy(gWPArray[startindex]->origin, startplace);

	VectorCopy(startplace, starttrace);

	starttrace[2] -= 4096;

	trap->Trace(&tr, startplace, NULL, NULL, starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	const float baseheight = startplace[2] - tr.endpos[2];

	int cancontinue = 1;

	VectorCopy(startplace, nodetable[nodenum].origin);
	nodetable[nodenum].weight = 1;
	nodetable[nodenum].inuse = 1;
	//	nodetable[nodenum].index = nodenum;
	nodenum++;

	while (nodenum < MAX_NODETABLE_SIZE && !foundit && cancontinue)
	{
		if (RMG.integer)
		{
			//adjust the branch distance dynamically depending on the distance from the start and end points.
			vec3_t start_dist;
			vec3_t end_dist;

			VectorSubtract(nodetable[nodenum - 1].origin, gWPArray[startindex]->origin, start_dist);
			VectorSubtract(nodetable[nodenum - 1].origin, gWPArray[endindex]->origin, end_dist);

			const float start_distf = VectorLength(start_dist);
			const float end_distf = VectorLength(end_dist);

			if (start_distf < 64 || end_distf < 64)
			{
				branchDistance = 64;
			}
			else if (start_distf < 128 || end_distf < 128)
			{
				branchDistance = 128;
			}
			else if (start_distf < 256 || end_distf < 256)
			{
				branchDistance = 256;
			}
			else if (start_distf < 512 || end_distf < 512)
			{
				branchDistance = 512;
			}
			else
			{
				branchDistance = 800;
			}
		}
		cancontinue = 0;
		i = 0;
		const int prenodestart = nodenum;

		while (i < prenodestart)
		{
			if (extendednodes[i] != 1)
			{
				VectorSubtract(gWPArray[endindex]->origin, nodetable[i].origin, a);
				const float fvecmeas = VectorLength(a);

				if (fvecmeas < 128 && CanGetToVector(gWPArray[endindex]->origin, nodetable[i].origin, mins, maxs))
				{
					foundit = 1;
					successnodeindex = i;
					break;
				}

				VectorCopy(nodetable[i].origin, testspot);
				testspot[0] += branchDistance;

				VectorCopy(testspot, starttrace);

				starttrace[2] -= 4096;

				trap->Trace(&tr, testspot, NULL, NULL, starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

				testspot[2] = tr.endpos[2] + baseheight;

				if (!NodeHere(testspot) && !tr.startsolid && !tr.allsolid && CanGetToVector(
					nodetable[i].origin, testspot, mins, maxs))
				{
					VectorCopy(testspot, nodetable[nodenum].origin);
					nodetable[nodenum].inuse = 1;
					//					nodetable[nodenum].index = nodenum;
					nodetable[nodenum].weight = nodetable[i].weight + 1;
					nodetable[nodenum].neighbornum = i;
					if (nodetable[i].origin[2] - nodetable[nodenum].origin[2] > 50)
					{
						//if there's a big drop, make sure we know we can't just magically fly back up
						nodetable[nodenum].flags = WPFLAG_ONEWAY_FWD;
					}
					nodenum++;
					cancontinue = 1;
				}

				if (nodenum >= MAX_NODETABLE_SIZE)
				{
					break; //failure
				}

				VectorCopy(nodetable[i].origin, testspot);
				testspot[0] -= branchDistance;

				VectorCopy(testspot, starttrace);

				starttrace[2] -= 4096;

				trap->Trace(&tr, testspot, NULL, NULL, starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

				testspot[2] = tr.endpos[2] + baseheight;

				if (!NodeHere(testspot) && !tr.startsolid && !tr.allsolid && CanGetToVector(
					nodetable[i].origin, testspot, mins, maxs))
				{
					VectorCopy(testspot, nodetable[nodenum].origin);
					nodetable[nodenum].inuse = 1;
					//					nodetable[nodenum].index = nodenum;
					nodetable[nodenum].weight = nodetable[i].weight + 1;
					nodetable[nodenum].neighbornum = i;
					if (nodetable[i].origin[2] - nodetable[nodenum].origin[2] > 50)
					{
						//if there's a big drop, make sure we know we can't just magically fly back up
						nodetable[nodenum].flags = WPFLAG_ONEWAY_FWD;
					}
					nodenum++;
					cancontinue = 1;
				}

				if (nodenum >= MAX_NODETABLE_SIZE)
				{
					break; //failure
				}

				VectorCopy(nodetable[i].origin, testspot);
				testspot[1] += branchDistance;

				VectorCopy(testspot, starttrace);

				starttrace[2] -= 4096;

				trap->Trace(&tr, testspot, NULL, NULL, starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

				testspot[2] = tr.endpos[2] + baseheight;

				if (!NodeHere(testspot) && !tr.startsolid && !tr.allsolid && CanGetToVector(
					nodetable[i].origin, testspot, mins, maxs))
				{
					VectorCopy(testspot, nodetable[nodenum].origin);
					nodetable[nodenum].inuse = 1;
					//					nodetable[nodenum].index = nodenum;
					nodetable[nodenum].weight = nodetable[i].weight + 1;
					nodetable[nodenum].neighbornum = i;
					if (nodetable[i].origin[2] - nodetable[nodenum].origin[2] > 50)
					{
						//if there's a big drop, make sure we know we can't just magically fly back up
						nodetable[nodenum].flags = WPFLAG_ONEWAY_FWD;
					}
					nodenum++;
					cancontinue = 1;
				}

				if (nodenum >= MAX_NODETABLE_SIZE)
				{
					break; //failure
				}

				VectorCopy(nodetable[i].origin, testspot);
				testspot[1] -= branchDistance;

				VectorCopy(testspot, starttrace);

				starttrace[2] -= 4096;

				trap->Trace(&tr, testspot, NULL, NULL, starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

				testspot[2] = tr.endpos[2] + baseheight;

				if (!NodeHere(testspot) && !tr.startsolid && !tr.allsolid && CanGetToVector(
					nodetable[i].origin, testspot, mins, maxs))
				{
					VectorCopy(testspot, nodetable[nodenum].origin);
					nodetable[nodenum].inuse = 1;
					//					nodetable[nodenum].index = nodenum;
					nodetable[nodenum].weight = nodetable[i].weight + 1;
					nodetable[nodenum].neighbornum = i;
					if (nodetable[i].origin[2] - nodetable[nodenum].origin[2] > 50)
					{
						//if there's a big drop, make sure we know we can't just magically fly back up
						nodetable[nodenum].flags = WPFLAG_ONEWAY_FWD;
					}
					nodenum++;
					cancontinue = 1;
				}

				if (nodenum >= MAX_NODETABLE_SIZE)
				{
					break; //failure
				}

				extendednodes[i] = 1;
			}

			i++;
		}
	}

	if (!foundit)
	{
#ifndef _DEBUG //if debug just always print this.
		if (!behind_the_scenes)
#endif
		{
			trap->Print(S_COLOR_RED "Could not link %i to %i, unreachable by node branching.\n", startindex, endindex);
		}
		gWPArray[startindex]->flags |= WPFLAG_ONEWAY_FWD;
		gWPArray[endindex]->flags |= WPFLAG_ONEWAY_BACK;
		if (!behind_the_scenes)
		{
			trap->Print(
				S_COLOR_YELLOW
				"Since points cannot be connected, point %i has been flagged as only-forward and point %i has been flagged as only-backward.\n",
				startindex, endindex);
		}

		/*while (nodenum >= 0)
		{
			if (nodetable[nodenum].origin[0] || nodetable[nodenum].origin[1] || nodetable[nodenum].origin[2])
			{
				CreateNewWP(nodetable[nodenum].origin, nodetable[nodenum].flags);
			}

			nodenum--;
		}*/
		//The above code transfers nodes into the "rendered" waypoint array. Strictly for debugging.

		if (!behind_the_scenes)
		{
			//just use what we have if we're auto-pathing the level
			return 0;
		}
		int n_count = 0;
		int ideal_node = -1;
		float best_dist = 0;

		if (nodenum <= 10)
		{
			//not enough to even really bother.
			return 0;
		}

		//Since it failed, find whichever node is closest to the desired end.
		while (n_count < nodenum)
		{
			vec3_t end_dist;
			VectorSubtract(nodetable[n_count].origin, gWPArray[endindex]->origin, end_dist);
			const float test_dist = VectorLength(end_dist);
			if (ideal_node == -1)
			{
				ideal_node = n_count;
				best_dist = test_dist;
				n_count++;
				continue;
			}

			if (test_dist < best_dist)
			{
				ideal_node = n_count;
				best_dist = test_dist;
			}

			n_count++;
		}

		if (ideal_node == -1)
		{
			return 0;
		}

		successnodeindex = ideal_node;
	}

	i = successnodeindex;
	const int insertindex = startindex;
	int failsafe = 0;
	VectorCopy(gWPArray[startindex]->origin, validspotpos);

	while (failsafe < MAX_NODETABLE_SIZE && i < MAX_NODETABLE_SIZE && i >= 0)
	{
		VectorSubtract(validspotpos, nodetable[i].origin, a);
		if (!nodetable[nodetable[i].neighbornum].inuse || !CanGetToVectorTravel(
			validspotpos, /*nodetable[nodetable[i].neighbornum].origin*/nodetable[i].origin, mins, maxs) ||
			VectorLength(a) > maxDistFactor || !CanGetToVectorTravel(validspotpos, gWPArray[endindex]->origin, mins,
				maxs)
			&& CanGetToVectorTravel(nodetable[i].origin, gWPArray[endindex]->origin, mins, maxs))
		{
			nodetable[i].flags |= WPFLAG_CALCULATED;
			if (!CreateNewWP_InTrail(nodetable[i].origin, nodetable[i].flags, insertindex))
			{
				if (!behind_the_scenes)
				{
					trap->Print(S_COLOR_RED "Could not link %i to %i, waypoint limit hit.\n", startindex, endindex);
				}
				return 0;
			}

			VectorCopy(nodetable[i].origin, validspotpos);
		}

		if (i == 0)
		{
			break;
		}

		i = nodetable[i].neighbornum;

		failsafe++;
	}

	if (!behind_the_scenes)
	{
		trap->Print(S_COLOR_YELLOW "Finished connecting %i to %i.\n", startindex, endindex);
	}

	return 1;
}

static int OpposingEnds(const int start, const int end)
{
	if (!gWPArray[start] || !gWPArray[start]->inuse || !gWPArray[end] || !gWPArray[end]->inuse)
	{
		return 0;
	}

	if (gWPArray[start]->flags & WPFLAG_ONEWAY_FWD &&
		gWPArray[end]->flags & WPFLAG_ONEWAY_BACK)
	{
		return 1;
	}

	return 0;
}

static int DoorBlockingSection(const int start, const int end)
{
	//if a door blocks the trail, we'll just have to assume the points on each side are in visibility when it's open
	trace_t tr;

	if (!gWPArray[start] || !gWPArray[start]->inuse || !gWPArray[end] || !gWPArray[end]->inuse)
	{
		return 0;
	}

	trap->Trace(&tr, gWPArray[start]->origin, NULL, NULL, gWPArray[end]->origin, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0,
		0);

	if (tr.fraction == 1)
	{
		return 0;
	}

	const gentity_t* testdoor = &g_entities[tr.entityNum];

	if (!testdoor)
	{
		return 0;
	}

	if (!strstr(testdoor->classname, "func_"))
	{
		return 0;
	}

	const int start_trace_index = tr.entityNum;

	trap->Trace(&tr, gWPArray[end]->origin, NULL, NULL, gWPArray[start]->origin, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0,
		0);

	if (tr.fraction == 1)
	{
		return 0;
	}

	if (start_trace_index == tr.entityNum)
	{
		return 1;
	}

	return 0;
}

static int RepairPaths(const qboolean behind_the_scenes)
{
	//	int ctRet;
	float max_dist_factor = 400;

	if (!gWPNum)
	{
		return 0;
	}

	if (RMG.integer)
	{
		max_dist_factor = 800; //higher tolerance here.
	}

	int i = 0;

	trap->Cvar_Update(&bot_wp_distconnect);
	trap->Cvar_Update(&bot_wp_visconnect);

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i + 1] && gWPArray[i + 1]->inuse)
		{
			vec3_t a;
			VectorSubtract(gWPArray[i]->origin, gWPArray[i + 1]->origin, a);

			if (!(gWPArray[i + 1]->flags & WPFLAG_NOVIS) &&
				!(gWPArray[i + 1]->flags & WPFLAG_JUMP) &&
				//don't calculate on jump points because they might not always want to be visible (in cases of force jumping)
				!(gWPArray[i]->flags & WPFLAG_CALCULATED) && //don't calculate it again
				!OpposingEnds(i, i + 1) &&
				(bot_wp_distconnect.value && VectorLength(a) > max_dist_factor || !org_visible(
					gWPArray[i]->origin, gWPArray[i + 1]->origin, ENTITYNUM_NONE) && bot_wp_visconnect.value) &&
				!DoorBlockingSection(i, i + 1))
			{
				/*ctRet = */
				ConnectTrail(i, i + 1, behind_the_scenes);

				if (gWPNum >= MAX_WPARRAY_SIZE)
				{
					//Bad!
					gWPNum = MAX_WPARRAY_SIZE;
					break;
				}
			}
		}

		i++;
	}

	return 1;
}

static int OrgVisibleCurve(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, const int ignore)
{
	trace_t tr;
	vec3_t evenorg1;

	VectorCopy(org1, evenorg1);
	evenorg1[2] = org2[2];

	trap->Trace(&tr, evenorg1, mins, maxs, org2, ignore, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
	{
		trap->Trace(&tr, evenorg1, mins, maxs, org1, ignore, MASK_SOLID, qfalse, 0, 0);

		if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
		{
			return 1;
		}
	}

	return 0;
}

static int CanForceJumpTo(const int baseindex, const int testingindex, const float distance)
{
	float heightdif;
	vec3_t xy_base, xy_test, v, mins, maxs;
	wpobject_t* wp_base = gWPArray[baseindex];
	wpobject_t* wp_test = gWPArray[testingindex];

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -15; //-1
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 15; //1

	if (!wp_base || !wp_base->inuse || !wp_test || !wp_test->inuse)
	{
		return 0;
	}

	if (distance > 400)
	{
		return 0;
	}

	VectorCopy(wp_base->origin, xy_base);
	VectorCopy(wp_test->origin, xy_test);

	xy_base[2] = xy_test[2];

	VectorSubtract(xy_base, xy_test, v);

	if (VectorLength(v) > MAX_NEIGHBOR_LINK_DISTANCE)
	{
		return 0;
	}

	if ((int)wp_base->origin[2] < (int)wp_test->origin[2])
	{
		heightdif = wp_test->origin[2] - wp_base->origin[2];
	}
	else
	{
		return 0; //err..
	}

	if (heightdif < 128)
	{
		//don't bother..
		return 0;
	}

	if (heightdif > 512)
	{
		//too high
		return 0;
	}

	if (!OrgVisibleCurve(wp_base->origin, mins, maxs, wp_test->origin, ENTITYNUM_NONE))
	{
		return 0;
	}

	if (heightdif > 400)
	{
		return 3;
	}
	if (heightdif > 256)
	{
		return 2;
	}
	return 1;
}

static void CalculatePaths(void)
{
	int max_neighbor_dist = MAX_NEIGHBOR_LINK_DISTANCE;
	vec3_t mins, maxs;

	if (!gWPNum)
	{
		return;
	}

	if (RMG.integer)
	{
		max_neighbor_dist = DEFAULT_GRID_SPACING + DEFAULT_GRID_SPACING * 0.5;
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -15; //-1
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 15; //1

	//now clear out all the neighbor data before we recalculate
	int i = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->neighbornum)
		{
			while (gWPArray[i]->neighbornum >= 0)
			{
				gWPArray[i]->neighbors[gWPArray[i]->neighbornum].num = 0;
				gWPArray[i]->neighbors[gWPArray[i]->neighbornum].forceJumpTo = 0;
				gWPArray[i]->neighbornum--;
			}
			gWPArray[i]->neighbornum = 0;
		}

		i++;
	}

	i = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			int c = 0;

			while (c < gWPNum)
			{
				if (gWPArray[c] && gWPArray[c]->inuse && i != c &&
					NotWithinRange(i, c))
				{
					vec3_t a;
					VectorSubtract(gWPArray[i]->origin, gWPArray[c]->origin, a);

					const float nLDist = VectorLength(a);
					const int force_jumpable = CanForceJumpTo(i, c, nLDist);

					if ((nLDist < max_neighbor_dist || force_jumpable) &&
						((int)gWPArray[i]->origin[2] == (int)gWPArray[c]->origin[2] || force_jumpable) &&
						(org_visible_box(gWPArray[i]->origin, mins, maxs, gWPArray[c]->origin, ENTITYNUM_NONE) ||
							force_jumpable))
					{
						gWPArray[i]->neighbors[gWPArray[i]->neighbornum].num = c;
						if (force_jumpable && ((int)gWPArray[i]->origin[2] != (int)gWPArray[c]->origin[2] || nLDist <
							max_neighbor_dist))
						{
							gWPArray[i]->neighbors[gWPArray[i]->neighbornum].forceJumpTo = 999; //forceJumpable; //FJSR
						}
						else
						{
							gWPArray[i]->neighbors[gWPArray[i]->neighbornum].forceJumpTo = 0;
						}
						gWPArray[i]->neighbornum++;
					}

					if (gWPArray[i]->neighbornum >= MAX_NEIGHBOR_SIZE)
					{
						break;
					}
				}
				c++;
			}
		}
		i++;
	}
}

static gentity_t* GetObjectThatTargets(const gentity_t* ent)
{
	gentity_t* next = NULL;

	if (!ent->targetname)
	{
		return NULL;
	}

	next = G_Find(next, FOFS(target), ent->targetname);

	if (next)
	{
		return next;
	}

	return NULL;
}

static void CalculateSiegeGoals(void)
{
	int i = 0;
	vec3_t dif;

	while (i < level.num_entities)
	{
		const gentity_t* ent = &g_entities[i];

		const gentity_t* tent = NULL;

		if (ent && ent->classname && strcmp(ent->classname, "info_siege_objective") == 0)
		{
			tent = ent;
			const gentity_t* t2_ent = GetObjectThatTargets(tent);
			int looptracker = 0;

			while (t2_ent && looptracker < 2048)
			{
				//looptracker keeps us from getting stuck in case something is set up weird on this map
				tent = t2_ent;
				t2_ent = GetObjectThatTargets(tent);
				looptracker++;
			}

			if (looptracker >= 2048)
			{
				break;
			}
		}

		if (tent && ent && tent != ent)
		{
			//tent should now be the object attached to the mission objective
			dif[0] = (tent->r.absmax[0] + tent->r.absmin[0]) / 2;
			dif[1] = (tent->r.absmax[1] + tent->r.absmin[1]) / 2;
			dif[2] = (tent->r.absmax[2] + tent->r.absmin[2]) / 2;

			const int wpindex = get_nearest_visible_wp(dif, tent->s.number);

			if (wpindex != -1 && gWPArray[wpindex] && gWPArray[wpindex]->inuse)
			{
				//found the waypoint nearest the center of this objective-related object
				if (ent->side == SIEGETEAM_TEAM1)
				{
					gWPArray[wpindex]->flags |= WPFLAG_SIEGE_IMPERIALOBJ;
				}
				else
				{
					gWPArray[wpindex]->flags |= WPFLAG_SIEGE_REBELOBJ;
				}

				gWPArray[wpindex]->associated_entity = tent->s.number;
			}
		}

		i++;
	}
}

float botGlobalNavWeaponWeights[WP_NUM_WEAPONS] =
{
	0, //WP_NONE,

	0, //WP_STUN_BATON,
	0, //WP_MELEE
	0, //WP_SABER,				 // NOTE: lots of code assumes this is the first weapon (... which is crap) so be careful -Ste.
	0, //WP_BRYAR_PISTOL,
	3, //WP_BLASTER,
	5, //WP_DISRUPTOR,
	4, //WP_BOWCASTER,
	6, //WP_REPEATER,
	7, //WP_DEMP2,
	8, //WP_FLECHETTE,
	9, //WP_ROCKET_LAUNCHER,
	3, //WP_THERMAL,
	3, //WP_TRIP_MINE,
	3, //WP_DET_PACK,
	2, //WP_CONCUSSION,
	0, //WP_BRYAR_OLD,
	0 //WP_EMPLACED_GUN,
};

static int GetNearestVisibleWPToItem(vec3_t org, const int ignore)
{
	vec3_t mins, maxs;

	int i = 0;
	float bestdist = 64; //has to be less than 64 units to the item or it isn't safe enough
	int bestindex = -1;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse &&
			gWPArray[i]->origin[2] - 15 < org[2] &&
			gWPArray[i]->origin[2] + 15 > org[2])
		{
			vec3_t a;
			VectorSubtract(org, gWPArray[i]->origin, a);
			const float fl_len = VectorLength(a);

			if (fl_len < bestdist && trap->InPVS(org, gWPArray[i]->origin) && org_visible_box(
				org, mins, maxs, gWPArray[i]->origin, ignore))
			{
				bestdist = fl_len;
				bestindex = i;
			}
		}

		i++;
	}

	return bestindex;
}

static void CalculateWeightGoals(void)
{
	//set waypoint weights depending on weapon and item placement
	int i = 0;

	trap->Cvar_Update(&bot_wp_clearweight);

	if (bot_wp_clearweight.integer)
	{
		//if set then flush out all weight/goal values before calculating them again
		while (i < gWPNum)
		{
			if (gWPArray[i] && gWPArray[i]->inuse)
			{
				gWPArray[i]->weight = 0;

				if (gWPArray[i]->flags & WPFLAG_GOALPOINT)
				{
					gWPArray[i]->flags -= WPFLAG_GOALPOINT;
				}
			}

			i++;
		}
	}

	i = 0;

	while (i < level.num_entities)
	{
		gentity_t* ent = &g_entities[i];

		float weight = 0;

		if (ent && ent->classname)
		{
			if (strcmp(ent->classname, "item_seeker") == 0)
			{
				weight = 2;
			}
			else if (strcmp(ent->classname, "item_shield") == 0)
			{
				weight = 2;
			}
			else if (strcmp(ent->classname, "item_medpac") == 0)
			{
				weight = 2;
			}
			else if (strcmp(ent->classname, "item_sentry_gun") == 0)
			{
				weight = 2;
			}
			else if (strcmp(ent->classname, "item_force_enlighten_dark") == 0)
			{
				weight = 5;
			}
			else if (strcmp(ent->classname, "item_force_enlighten_light") == 0)
			{
				weight = 5;
			}
			else if (strcmp(ent->classname, "item_force_boon") == 0)
			{
				weight = 5;
			}
			else if (strcmp(ent->classname, "item_ysalimari") == 0)
			{
				weight = 2;
			}
			else if (strstr(ent->classname, "weapon_") && ent->item)
			{
				weight = botGlobalNavWeaponWeights[ent->item->giTag];
			}
			else if (ent->item && ent->item->giType == IT_AMMO)
			{
				weight = 3;
			}
		}

		if (ent && weight)
		{
			const int wpindex = GetNearestVisibleWPToItem(ent->s.pos.trBase, ent->s.number);

			if (wpindex != -1 && gWPArray[wpindex] && gWPArray[wpindex]->inuse)
			{
				//found the waypoint nearest the center of this object
				gWPArray[wpindex]->weight = weight;
				gWPArray[wpindex]->flags |= WPFLAG_GOALPOINT;
				gWPArray[wpindex]->associated_entity = ent->s.number;
			}
		}

		i++;
	}
}

static void CalculateJumpRoutes(void)
{
	int i = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			if (gWPArray[i]->flags & WPFLAG_JUMP)
			{
				float nheightdif = 0;
				float pheightdif = 0;

				gWPArray[i]->forceJumpTo = 0;

				if (gWPArray[i - 1] && gWPArray[i - 1]->inuse && gWPArray[i - 1]->origin[2] + 16 < gWPArray[i]->origin[
					2])
				{
					nheightdif = gWPArray[i]->origin[2] - gWPArray[i - 1]->origin[2];
				}

				if (gWPArray[i + 1] && gWPArray[i + 1]->inuse && gWPArray[i + 1]->origin[2] + 16 < gWPArray[i]->origin[
					2])
				{
					pheightdif = gWPArray[i]->origin[2] - gWPArray[i + 1]->origin[2];
				}

				if (nheightdif > pheightdif)
				{
					pheightdif = nheightdif;
				}

				if (pheightdif)
				{
					if (pheightdif > 500)
					{
						gWPArray[i]->forceJumpTo = 999; //FORCE_LEVEL_3; //FJSR
					}
					else if (pheightdif > 256)
					{
						gWPArray[i]->forceJumpTo = 999; //FORCE_LEVEL_2; //FJSR
					}
					else if (pheightdif > 128)
					{
						gWPArray[i]->forceJumpTo = 999; //FORCE_LEVEL_1; //FJSR
					}
				}
			}
		}

		i++;
	}
}

static int LoadPathData(const char* filename)
{
	fileHandle_t f;
	char fileString[WPARRAY_BUFFER_SIZE];
	char routePath[MAX_QPATH];
	wpobject_t thiswp;

	int i = 0;
	int i_cv;

	Com_sprintf(routePath, sizeof routePath, "botroutes/%s.wnt\0", filename);

	const int len = trap->FS_Open(routePath, &f, FS_READ);

	if (!f)
	{
		trap->Print(S_COLOR_YELLOW "Bot route data not found for %s\n", filename);
		return 2;
	}

	if (len >= WPARRAY_BUFFER_SIZE)
	{
		trap->Print(S_COLOR_RED "Route file exceeds maximum length\n");
		trap->FS_Close(f);
		return 0;
	}

	char* current_var = B_TempAlloc(2048);

	trap->FS_Read(fileString, len, f);

	if (fileString[i] == 'l')
	{
		//contains a "levelflags" entry..
		char read_l_flags[64];
		i_cv = 0;

		while (fileString[i] != ' ')
		{
			i++;
		}
		i++;
		while (fileString[i] != '\n')
		{
			read_l_flags[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		read_l_flags[i_cv] = 0;
		i++;

		gLevelFlags = atoi(read_l_flags);
	}
	else
	{
		gLevelFlags = 0;
	}

	while (i < len)
	{
		i_cv = 0;

		thiswp.index = 0;
		thiswp.flags = 0;
		thiswp.inuse = 0;
		thiswp.neighbornum = 0;
		thiswp.origin[0] = 0;
		thiswp.origin[1] = 0;
		thiswp.origin[2] = 0;
		thiswp.weight = 0;
		thiswp.associated_entity = ENTITYNUM_NONE;
		thiswp.forceJumpTo = 0;
		thiswp.disttonext = 0;
		int nei_num = 0;

		while (nei_num < MAX_NEIGHBOR_SIZE)
		{
			thiswp.neighbors[nei_num].num = 0;
			thiswp.neighbors[nei_num].forceJumpTo = 0;

			nei_num++;
		}

		while (fileString[i] != ' ')
		{
			current_var[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		current_var[i_cv] = '\0';

		thiswp.index = atoi(current_var);

		i_cv = 0;
		i++;

		while (fileString[i] != ' ')
		{
			current_var[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		current_var[i_cv] = '\0';

		thiswp.flags = atoi(current_var);

		i_cv = 0;
		i++;

		while (fileString[i] != ' ')
		{
			current_var[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		current_var[i_cv] = '\0';

		thiswp.weight = atof(current_var);

		i_cv = 0;
		i++;
		i++;

		while (fileString[i] != ' ')
		{
			current_var[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		current_var[i_cv] = '\0';

		thiswp.origin[0] = atof(current_var);

		i_cv = 0;
		i++;

		while (fileString[i] != ' ')
		{
			current_var[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		current_var[i_cv] = '\0';

		thiswp.origin[1] = atof(current_var);

		i_cv = 0;
		i++;

		while (fileString[i] != ')')
		{
			current_var[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		current_var[i_cv] = '\0';

		thiswp.origin[2] = atof(current_var);

		i += 4;

		while (fileString[i] != '}')
		{
			i_cv = 0;
			while (fileString[i] != ' ' && fileString[i] != '-')
			{
				current_var[i_cv] = fileString[i];
				i_cv++;
				i++;
			}
			current_var[i_cv] = '\0';

			thiswp.neighbors[thiswp.neighbornum].num = atoi(current_var);

			if (fileString[i] == '-')
			{
				i_cv = 0;
				i++;

				while (fileString[i] != ' ')
				{
					current_var[i_cv] = fileString[i];
					i_cv++;
					i++;
				}
				current_var[i_cv] = '\0';

				thiswp.neighbors[thiswp.neighbornum].forceJumpTo = 999; //atoi(currentVar); //FJSR
			}
			else
			{
				thiswp.neighbors[thiswp.neighbornum].forceJumpTo = 0;
			}

			thiswp.neighbornum++;

			i++;
		}

		i_cv = 0;
		i++;
		i++;

		while (fileString[i] != '\n')
		{
			current_var[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		current_var[i_cv] = '\0';

		thiswp.disttonext = atof(current_var);

		CreateNewWP_FromObject(&thiswp);
		i++;
	}

	B_TempFree(2048); //currentVar

	trap->FS_Close(f);

	CalculateSiegeGoals();

	CalculateWeightGoals();
	//calculate weights for idle activity goals when
	//the bot has absolutely nothing else to do

	CalculateJumpRoutes();
	//Look at jump points and mark them as requiring
	//force jumping as needed

	return 1;
}

static void FlagObjects(void)
{
	int i = 0, bestindex = 0, found = 0;
	float bestdist = 999999, tlen;
	vec3_t a, mins, maxs;
	trace_t tr;

	gentity_t* flag_red = NULL;
	gentity_t* flag_blue = NULL;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -5;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 5;

	while (i < level.num_entities)
	{
		gentity_t* ent = &g_entities[i];

		if (ent && ent->inuse && ent->classname)
		{
			if (!flag_red && strcmp(ent->classname, "team_CTF_redflag") == 0)
			{
				flag_red = ent;
			}
			else if (!flag_blue && strcmp(ent->classname, "team_CTF_blueflag") == 0)
			{
				flag_blue = ent;
			}

			if (flag_red && flag_blue)
			{
				break;
			}
		}

		i++;
	}

	i = 0;

	if (!flag_red || !flag_blue)
	{
		return;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			VectorSubtract(flag_red->s.pos.trBase, gWPArray[i]->origin, a);
			tlen = VectorLength(a);

			if (tlen < bestdist)
			{
				trap->Trace(&tr, flag_red->s.pos.trBase, mins, maxs, gWPArray[i]->origin, flag_red->s.number,
					MASK_SOLID, qfalse, 0, 0);

				if (tr.fraction == 1 || tr.entityNum == flag_red->s.number)
				{
					bestdist = tlen;
					bestindex = i;
					found = 1;
				}
			}
		}

		i++;
	}

	if (found)
	{
		gWPArray[bestindex]->flags |= WPFLAG_RED_FLAG;
		flagRed = gWPArray[bestindex];
		oFlagRed = flagRed;
		eFlagRed = flag_red;
	}

	bestdist = 999999;
	bestindex = 0;
	found = 0;
	i = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			VectorSubtract(flag_blue->s.pos.trBase, gWPArray[i]->origin, a);
			tlen = VectorLength(a);

			if (tlen < bestdist)
			{
				trap->Trace(&tr, flag_blue->s.pos.trBase, mins, maxs, gWPArray[i]->origin, flag_blue->s.number,
					MASK_SOLID, qfalse, 0, 0);

				if (tr.fraction == 1 || tr.entityNum == flag_blue->s.number)
				{
					bestdist = tlen;
					bestindex = i;
					found = 1;
				}
			}
		}

		i++;
	}

	if (found)
	{
		gWPArray[bestindex]->flags |= WPFLAG_BLUE_FLAG;
		flagBlue = gWPArray[bestindex];
		oFlagBlue = flagBlue;
		eFlagBlue = flag_blue;
	}
}

static int SavePathData(const char* filename)
{
	fileHandle_t f;
	char fileString[WPARRAY_BUFFER_SIZE];
	vec3_t a;
	float fl_len;

	int i = 0;

	if (!gWPNum)
	{
		return 0;
	}

	const char* routePath = va("botroutes/%s.wnt\0", filename);

	trap->FS_Open(routePath, &f, FS_WRITE);

	if (!f)
	{
		trap->Print(S_COLOR_RED "ERROR: Could not open file to write path data\n");
		return 0;
	}

	if (!RepairPaths(qfalse)) //check if we can see all waypoints from the last. If not, try to branch over.
	{
		trap->FS_Close(f);
		return 0;
	}

	CalculatePaths(); //make everything nice and connected before saving

	FlagObjects(); //currently only used for flagging waypoints nearest CTF flags

	char* storeString = B_TempAlloc(4096);

	Com_sprintf(fileString, WPARRAY_BUFFER_SIZE, "%i %i %f (%f %f %f) { ", gWPArray[i]->index, gWPArray[i]->flags,
		gWPArray[i]->weight, gWPArray[i]->origin[0], gWPArray[i]->origin[1], gWPArray[i]->origin[2]);

	int n = 0;

	while (n < gWPArray[i]->neighbornum)
	{
		if (gWPArray[i]->neighbors[n].forceJumpTo)
		{
			Com_sprintf(storeString, 4096, "%s%i-%i ", storeString, gWPArray[i]->neighbors[n].num,
				gWPArray[i]->neighbors[n].forceJumpTo);
		}
		else
		{
			Com_sprintf(storeString, 4096, "%s%i ", storeString, gWPArray[i]->neighbors[n].num);
		}
		n++;
	}

	if (gWPArray[i + 1] && gWPArray[i + 1]->inuse && gWPArray[i + 1]->index)
	{
		VectorSubtract(gWPArray[i]->origin, gWPArray[i + 1]->origin, a);
		fl_len = VectorLength(a);
	}
	else
	{
		fl_len = 0;
	}

	gWPArray[i]->disttonext = fl_len;

	Com_sprintf(fileString, WPARRAY_BUFFER_SIZE, "%s} %f\n", fileString, fl_len);

	i++;

	while (i < gWPNum)
	{
		Com_sprintf(storeString, 4096, "%i %i %f (%f %f %f) { ", gWPArray[i]->index, gWPArray[i]->flags, gWPArray[i]->weight, gWPArray[i]->origin[0], gWPArray[i]->origin[1], gWPArray[i]->origin[2]);

		n = 0;

		while (n < gWPArray[i]->neighbornum)
		{
			if (gWPArray[i]->neighbors[n].forceJumpTo)
			{
				Com_sprintf(storeString, 4096, "%s%i-%i ", storeString, gWPArray[i]->neighbors[n].num,
					gWPArray[i]->neighbors[n].forceJumpTo);
			}
			else
			{
				Com_sprintf(storeString, 4096, "%s%i ", storeString, gWPArray[i]->neighbors[n].num);
			}
			n++;
		}

		if (gWPArray[i + 1] && gWPArray[i + 1]->inuse && gWPArray[i + 1]->index)
		{
			VectorSubtract(gWPArray[i]->origin, gWPArray[i + 1]->origin, a);
			fl_len = VectorLength(a);
		}
		else
		{
			fl_len = 0;
		}

		gWPArray[i]->disttonext = fl_len;

		Com_sprintf(storeString, 4096, "%s} %f\n", storeString, fl_len);

		Q_strcat(fileString, WPARRAY_BUFFER_SIZE, storeString);

		i++;
	}

	trap->FS_Write(fileString, strlen(fileString), f);

	B_TempFree(4096); //storeString

	trap->FS_Close(f);

	trap->Print(
		"Path data has been saved and updated. You may need to restart the level for some things to be properly calculated.\n");

	return 1;
}

#define MAX_SPAWNPOINT_ARRAY 64
int gSpawnPointNum = 0;
gentity_t* gSpawnPoints[MAX_SPAWNPOINT_ARRAY];

static int G_NearestNodeToPoint(vec3_t point)
{
	//gets the node on the entire grid which is nearest to the specified coordinates.
	int best_index = -1;
	int i = 0;
	float best_dist = 0;

	while (i < nodenum)
	{
		vec3_t v_sub;
		VectorSubtract(nodetable[i].origin, point, v_sub);
		const float testDist = VectorLength(v_sub);

		if (best_index == -1)
		{
			best_index = i;
			best_dist = testDist;

			i++;
			continue;
		}

		if (testDist < best_dist)
		{
			best_index = i;
			best_dist = testDist;
		}
		i++;
	}

	return best_index;
}

static void G_NodeClearForNext(void)
{
	//reset nodes for the next trail connection.
	int i = 0;

	while (i < nodenum)
	{
		nodetable[i].flags = 0;
		nodetable[i].weight = 99999;

		i++;
	}
}

static void G_NodeClearFlags(void)
{
	//only clear out flags so nodes can be reused.
	int i = 0;

	while (i < nodenum)
	{
		nodetable[i].flags = 0;

		i++;
	}
}

static int G_NodeMatchingXY(const float x, const float y)
{
	//just get the first unflagged node with the matching x,y coordinates.
	int i = 0;

	while (i < nodenum)
	{
		if (nodetable[i].origin[0] == x &&
			nodetable[i].origin[1] == y &&
			!nodetable[i].flags)
		{
			return i;
		}

		i++;
	}

	return -1;
}

static int G_NodeMatchingXY_BA(const int x, const int y, const int final)
{
	//return the node with the lowest weight that matches the specified x,y coordinates.
	int i = 0;
	int bestindex = -1;
	float best_weight = 9999;

	while (i < nodenum)
	{
		if ((int)nodetable[i].origin[0] == x &&
			(int)nodetable[i].origin[1] == y &&
			!nodetable[i].flags &&
			(nodetable[i].weight < best_weight || i == final))
		{
			if (i == final)
			{
				return i;
			}
			bestindex = i;
			best_weight = nodetable[i].weight;
		}

		i++;
	}

	return bestindex;
}

static int G_RecursiveConnection(const int start, const int end, const int weight, const qboolean trace_check,
	const float base_height)
{
	int index_directions[4]; //0 == down, 1 == up, 2 == left, 3 == right
	int recursive_index = -1;
	int pass_weight = weight;
	vec2_t given_xy;
	trace_t tr;

	pass_weight++;
	nodetable[start].weight = pass_weight;

	given_xy[0] = nodetable[start].origin[0];
	given_xy[1] = nodetable[start].origin[1];
	given_xy[0] -= DEFAULT_GRID_SPACING;
	index_directions[0] = G_NodeMatchingXY(given_xy[0], given_xy[1]);

	given_xy[0] = nodetable[start].origin[0];
	given_xy[1] = nodetable[start].origin[1];
	given_xy[0] += DEFAULT_GRID_SPACING;
	index_directions[1] = G_NodeMatchingXY(given_xy[0], given_xy[1]);

	given_xy[0] = nodetable[start].origin[0];
	given_xy[1] = nodetable[start].origin[1];
	given_xy[1] -= DEFAULT_GRID_SPACING;
	index_directions[2] = G_NodeMatchingXY(given_xy[0], given_xy[1]);

	given_xy[0] = nodetable[start].origin[0];
	given_xy[1] = nodetable[start].origin[1];
	given_xy[1] += DEFAULT_GRID_SPACING;
	index_directions[3] = G_NodeMatchingXY(given_xy[0], given_xy[1]);

	int i = 0;
	while (i < 4)
	{
		if (index_directions[i] == end)
		{
			//we've connected all the way to the destination.
			return index_directions[i];
		}

		if (index_directions[i] != -1 && nodetable[index_directions[i]].flags)
		{
			//this point is already used, so it's not valid.
			index_directions[i] = -1;
		}
		else if (index_directions[i] != -1)
		{
			//otherwise mark it as used.
			nodetable[index_directions[i]].flags = 1;
		}

		if (index_directions[i] != -1 && trace_check)
		{
			//if we care about trace visibility between nodes, perform the check and mark as not valid if the trace isn't clear.
			trap->Trace(&tr, nodetable[start].origin, NULL, NULL, nodetable[index_directions[i]].origin, ENTITYNUM_NONE,
				CONTENTS_SOLID, qfalse, 0, 0);

			if (tr.fraction != 1)
			{
				index_directions[i] = -1;
			}
		}

		if (index_directions[i] != -1)
		{
			//it's still valid, so keep connecting via this point.
			recursive_index = G_RecursiveConnection(index_directions[i], end, pass_weight, trace_check, base_height);
		}

		if (recursive_index != -1)
		{
			//the result of the recursive check was valid, so return it.
			return recursive_index;
		}

		i++;
	}

	return recursive_index;
}

#ifdef DEBUG_NODE_FILE
static void G_DebugNodeFile()
{
	fileHandle_t f;
	int i = 0;
	float placeX;
	char fileString[131072];
	gentity_t* terrain = G_Find(NULL, FOFS(classname), "terrain");

	fileString[0] = 0;

	placeX = terrain->r.absmin[0];

	while (i < nodenum)
	{
		Q_strcat(fileString, sizeof(fileString), va("%i-%f ", i, nodetable[i].weight));
		placeX += DEFAULT_GRID_SPACING;

		if (placeX >= terrain->r.absmax[0])
		{
			Q_strcat(fileString, sizeof(fileString), "\n");
			placeX = terrain->r.absmin[0];
		}
		i++;
	}

	trap->FS_Open("ROUTEDEBUG.txt", &f, FS_WRITE);
	trap->FS_Write(fileString, strlen(fileString), f);
	trap->FS_Close(f);
}
#endif

//#define ASCII_ART_DEBUG
//#define ASCII_ART_NODE_DEBUG

#ifdef ASCII_ART_DEBUG

#define ALLOWABLE_DEBUG_FILE_SIZE 1048576

static void CreateAsciiTableRepresentation()
{ //Draw a text grid of the entire waypoint array (useful for debugging final waypoint placement)
	fileHandle_t f;
	int i = 0;
	int sP = 0;
	int placeX;
	int placeY;
	int oldX;
	int oldY;
	char fileString[ALLOWABLE_DEBUG_FILE_SIZE];
	char bChr = '+';
	gentity_t* terrain = G_Find(NULL, FOFS(classname), "terrain");

	placeX = terrain->r.absmin[0];
	placeY = terrain->r.absmin[1];

	oldX = placeX - 1;
	oldY = placeY - 1;

	while (placeY < terrain->r.absmax[1])
	{
		while (placeX < terrain->r.absmax[0])
		{
			qboolean gotit = qfalse;

			i = 0;
			while (i < gWPNum)
			{
				if (((int)gWPArray[i]->origin[0] <= placeX && (int)gWPArray[i]->origin[0] > oldX) &&
					((int)gWPArray[i]->origin[1] <= placeY && (int)gWPArray[i]->origin[1] > oldY))
				{
					gotit = qtrue;
					break;
				}
				i++;
			}

			if (gotit)
			{
				if (gWPArray[i]->flags & WPFLAG_ONEWAY_FWD)
				{
					bChr = 'F';
				}
				else if (gWPArray[i]->flags & WPFLAG_ONEWAY_BACK)
				{
					bChr = 'B';
				}
				else
				{
					bChr = '+';
				}

				if (gWPArray[i]->index < 10)
				{
					fileString[sP] = bChr;
					fileString[sP + 1] = '0';
					fileString[sP + 2] = '0';
					fileString[sP + 3] = va("%i", gWPArray[i]->index)[0];
				}
				else if (gWPArray[i]->index < 100)
				{
					char* vastore = va("%i", gWPArray[i]->index);

					fileString[sP] = bChr;
					fileString[sP + 1] = '0';
					fileString[sP + 2] = vastore[0];
					fileString[sP + 3] = vastore[1];
				}
				else if (gWPArray[i]->index < 1000)
				{
					char* vastore = va("%i", gWPArray[i]->index);

					fileString[sP] = bChr;
					fileString[sP + 1] = vastore[0];
					fileString[sP + 2] = vastore[1];
					fileString[sP + 3] = vastore[2];
				}
				else
				{
					fileString[sP] = 'X';
					fileString[sP + 1] = 'X';
					fileString[sP + 2] = 'X';
					fileString[sP + 3] = 'X';
				}
			}
			else
			{
				fileString[sP] = '-';
				fileString[sP + 1] = '-';
				fileString[sP + 2] = '-';
				fileString[sP + 3] = '-';
			}

			sP += 4;

			if (sP >= ALLOWABLE_DEBUG_FILE_SIZE - 16)
			{
				break;
			}
			oldX = placeX;
			placeX += DEFAULT_GRID_SPACING;
		}

		placeX = terrain->r.absmin[0];
		oldX = placeX - 1;
		fileString[sP] = '\n';
		sP++;

		if (sP >= ALLOWABLE_DEBUG_FILE_SIZE - 16)
		{
			break;
		}

		oldY = placeY;
		placeY += DEFAULT_GRID_SPACING;
	}

	fileString[sP] = 0;

	trap->FS_Open("ROUTEDRAWN.txt", &f, FS_WRITE);
	trap->FS_Write(fileString, strlen(fileString), f);
	trap->FS_Close(f);
}

static void CreateAsciiNodeTableRepresentation(int start, int end)
{ //draw a text grid of a single node path, from point A to Z.
	fileHandle_t f;
	int i = 0;
	int sP = 0;
	int placeX;
	int placeY;
	int oldX;
	int oldY;
	char fileString[ALLOWABLE_DEBUG_FILE_SIZE];
	gentity_t* terrain = G_Find(NULL, FOFS(classname), "terrain");

	placeX = terrain->r.absmin[0];
	placeY = terrain->r.absmin[1];

	oldX = placeX - 1;
	oldY = placeY - 1;

	while (placeY < terrain->r.absmax[1])
	{
		while (placeX < terrain->r.absmax[0])
		{
			qboolean gotit = qfalse;

			i = 0;
			while (i < nodenum)
			{
				if (((int)nodetable[i].origin[0] <= placeX && (int)nodetable[i].origin[0] > oldX) &&
					((int)nodetable[i].origin[1] <= placeY && (int)nodetable[i].origin[1] > oldY))
				{
					gotit = qtrue;
					break;
				}
				i++;
			}

			if (gotit)
			{
				if (i == start)
				{ //beginning of the node trail
					fileString[sP] = 'A';
					fileString[sP + 1] = 'A';
					fileString[sP + 2] = 'A';
					fileString[sP + 3] = 'A';
				}
				else if (i == end)
				{ //destination of the node trail
					fileString[sP] = 'Z';
					fileString[sP + 1] = 'Z';
					fileString[sP + 2] = 'Z';
					fileString[sP + 3] = 'Z';
				}
				else if (nodetable[i].weight < 10)
				{
					fileString[sP] = '+';
					fileString[sP + 1] = '0';
					fileString[sP + 2] = '0';
					fileString[sP + 3] = va("%f", nodetable[i].weight)[0];
				}
				else if (nodetable[i].weight < 100)
				{
					char* vastore = va("%f", nodetable[i].weight);

					fileString[sP] = '+';
					fileString[sP + 1] = '0';
					fileString[sP + 2] = vastore[0];
					fileString[sP + 3] = vastore[1];
				}
				else if (nodetable[i].weight < 1000)
				{
					char* vastore = va("%f", nodetable[i].weight);

					fileString[sP] = '+';
					fileString[sP + 1] = vastore[0];
					fileString[sP + 2] = vastore[1];
					fileString[sP + 3] = vastore[2];
				}
				else
				{
					fileString[sP] = 'X';
					fileString[sP + 1] = 'X';
					fileString[sP + 2] = 'X';
					fileString[sP + 3] = 'X';
				}
			}
			else
			{
				fileString[sP] = '-';
				fileString[sP + 1] = '-';
				fileString[sP + 2] = '-';
				fileString[sP + 3] = '-';
			}

			sP += 4;

			if (sP >= ALLOWABLE_DEBUG_FILE_SIZE - 16)
			{
				break;
			}
			oldX = placeX;
			placeX += DEFAULT_GRID_SPACING;
		}

		placeX = terrain->r.absmin[0];
		oldX = placeX - 1;
		fileString[sP] = '\n';
		sP++;

		if (sP >= ALLOWABLE_DEBUG_FILE_SIZE - 16)
		{
			break;
		}

		oldY = placeY;
		placeY += DEFAULT_GRID_SPACING;
	}

	fileString[sP] = 0;

	trap->FS_Open("ROUTEDRAWN.txt", &f, FS_WRITE);
	trap->FS_Write(fileString, strlen(fileString), f);
	trap->FS_Close(f);
}
#endif

static qboolean G_BackwardAttachment(const int start, const int final_destination, const int insert_after)
{
	//After creating a node path between 2 points, this function links the 2 points with actual waypoint data.
	int index_directions[4]; //0 == down, 1 == up, 2 == left, 3 == right
	int i = 0;
	int lowest_weight = 9999;
	int desired_index = -1;
	vec2_t given_xy;

	given_xy[0] = nodetable[start].origin[0];
	given_xy[1] = nodetable[start].origin[1];
	given_xy[0] -= DEFAULT_GRID_SPACING;
	index_directions[0] = G_NodeMatchingXY_BA(given_xy[0], given_xy[1], final_destination);

	given_xy[0] = nodetable[start].origin[0];
	given_xy[1] = nodetable[start].origin[1];
	given_xy[0] += DEFAULT_GRID_SPACING;
	index_directions[1] = G_NodeMatchingXY_BA(given_xy[0], given_xy[1], final_destination);

	given_xy[0] = nodetable[start].origin[0];
	given_xy[1] = nodetable[start].origin[1];
	given_xy[1] -= DEFAULT_GRID_SPACING;
	index_directions[2] = G_NodeMatchingXY_BA(given_xy[0], given_xy[1], final_destination);

	given_xy[0] = nodetable[start].origin[0];
	given_xy[1] = nodetable[start].origin[1];
	given_xy[1] += DEFAULT_GRID_SPACING;
	index_directions[3] = G_NodeMatchingXY_BA(given_xy[0], given_xy[1], final_destination);

	while (i < 4)
	{
		if (index_directions[i] != -1)
		{
			if (index_directions[i] == final_destination)
			{
				//hooray, we've found the original point and linked all the way back to it.
				CreateNewWP_InsertUnder(nodetable[start].origin, 0, insert_after);
				CreateNewWP_InsertUnder(nodetable[index_directions[i]].origin, 0, insert_after);
				return qtrue;
			}

			if (nodetable[index_directions[i]].weight < lowest_weight && nodetable[index_directions[i]].weight && !
				nodetable[index_directions[i]].flags
				/*&& (nodetable[indexDirections[i]].origin[2]-64 < nodetable[start].origin[2])*/)
			{
				desired_index = index_directions[i];
				lowest_weight = nodetable[index_directions[i]].weight;
			}
		}
		i++;
	}

	if (desired_index != -1)
	{
		//Create a waypoint here, and then recursively call this function for the next neighbor with the lowest weight.
		if (gWPNum < 3900)
		{
			CreateNewWP_InsertUnder(nodetable[start].origin, 0, insert_after);
		}
		else
		{
			return qfalse;
		}

		nodetable[start].flags = 1;
		return G_BackwardAttachment(desired_index, final_destination, insert_after);
	}

	return qfalse;
}

#ifdef _DEBUG
#define PATH_TIME_DEBUG
#endif

static void G_RMGPathing(void)
{
	//Generate waypoint information on-the-fly for the random mission.
	float place_x, place_y, place_z;
	int i = 0;
	int nearest_index;
	int nearest_index_for_next;
#ifdef PATH_TIME_DEBUG
	int startTime;
	int endTime;
#endif
	vec3_t down_vec, tr_mins, tr_maxs;
	trace_t tr;
	const gentity_t* terrain = G_Find(NULL, FOFS(classname), "terrain");

	if (!terrain || !terrain->inuse || terrain->s.eType != ET_TERRAIN)
	{
		trap->Print("Error: RMG with no terrain!\n");
		return;
	}

#ifdef PATH_TIME_DEBUG
	startTime = trap->Milliseconds();
#endif

	nodenum = 0;
	memset(&nodetable, 0, sizeof nodetable);

	VectorSet(tr_mins, -15, -15, DEFAULT_MINS_2);
	VectorSet(tr_maxs, 15, 15, DEFAULT_MAXS_2);

	place_x = terrain->r.absmin[0];
	place_y = terrain->r.absmin[1];
	place_z = terrain->r.absmax[2] - 400;

	//skim through the entirety of the terrain limits and drop nodes, removing
	//nodes that start in solid or fall too high on the terrain.
	while (place_y < terrain->r.absmax[1])
	{
		const int grid_spacing = DEFAULT_GRID_SPACING;
		if (nodenum >= MAX_NODETABLE_SIZE)
		{
			break;
		}

		while (place_x < terrain->r.absmax[0])
		{
			if (nodenum >= MAX_NODETABLE_SIZE)
			{
				break;
			}

			nodetable[nodenum].origin[0] = place_x;
			nodetable[nodenum].origin[1] = place_y;
			nodetable[nodenum].origin[2] = place_z;

			VectorCopy(nodetable[nodenum].origin, down_vec);
			down_vec[2] -= 3000;
			trap->Trace(&tr, nodetable[nodenum].origin, tr_mins, tr_maxs, down_vec, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0,
				0);

			if ((tr.entityNum >= ENTITYNUM_WORLD || g_entities[tr.entityNum].s.eType == ET_TERRAIN) && tr.endpos[2] <
				terrain->r.absmin[2] + 750)
			{
				//only drop nodes on terrain directly
				VectorCopy(tr.endpos, nodetable[nodenum].origin);
				nodenum++;
			}
			else
			{
				VectorClear(nodetable[nodenum].origin);
			}

			place_x += grid_spacing;
		}

		place_x = terrain->r.absmin[0];
		place_y += grid_spacing;
	}

	G_NodeClearForNext();

	//The grid has been placed down, now use it to connect the points in the level.
	while (i < gSpawnPointNum - 1)
	{
		if (!gSpawnPoints[i] || !gSpawnPoints[i]->inuse || !gSpawnPoints[i + 1] || !gSpawnPoints[i + 1]->inuse)
		{
			i++;
			continue;
		}

		nearest_index = G_NearestNodeToPoint(gSpawnPoints[i]->s.origin);
		nearest_index_for_next = G_NearestNodeToPoint(gSpawnPoints[i + 1]->s.origin);

		if (nearest_index == -1 || nearest_index_for_next == -1)
		{
			//Looks like there is no grid data near one of the points. Ideally, this will never happen.
			i++;
			continue;
		}

		if (nearest_index == nearest_index_for_next)
		{
			//Two spawn points on top of each other? We don't need to do both points, keep going until the next differs.
			i++;
			continue;
		}

		//So, nearestIndex is now the node for the spawn point we're on, and nearestIndexForNext is the
		//node we want to get to from here.

		//For now I am going to branch out mindlessly, but I will probably want to use some sort of A* algorithm
		//here to lessen the time taken.
		if (G_RecursiveConnection(nearest_index, nearest_index_for_next, 0, qtrue, terrain->r.absmin[2]) !=
			nearest_index_for_next)
		{
			//failed to branch to where we want. Oh well, try it without trace checks.
			G_NodeClearForNext();

			if (G_RecursiveConnection(nearest_index, nearest_index_for_next, 0, qfalse, terrain->r.absmin[2]) !=
				nearest_index_for_next)
			{
				//still failed somehow. Just disregard this point.
				G_NodeClearForNext();
				i++;
				continue;
			}
		}

		//Now our node array is set up so that highest reasonable weight is the destination node, and 2 is next to the original index,
		//so trace back to that point.
		G_NodeClearFlags();

#ifdef ASCII_ART_DEBUG
#ifdef ASCII_ART_NODE_DEBUG
		CreateAsciiNodeTableRepresentation(nearestIndex, nearestIndexForNext);
#endif
#endif
		if (G_BackwardAttachment(nearest_index_for_next, nearest_index, gWPNum - 1))
		{
			//successfully connected the trail from nearestIndex to nearestIndexForNext
			if (gSpawnPoints[i + 1]->inuse && gSpawnPoints[i + 1]->item &&
				gSpawnPoints[i + 1]->item->giType == IT_TEAM)
			{
				//This point is actually a CTF flag.
				if (gSpawnPoints[i + 1]->item->giTag == PW_REDFLAG || gSpawnPoints[i + 1]->item->giTag == PW_BLUEFLAG)
				{
					//Place a waypoint on the flag next in the trail, so the nearest grid point will link to it.
					CreateNewWP_InsertUnder(gSpawnPoints[i + 1]->s.origin, WPFLAG_NEVERONEWAY, gWPNum - 1);
				}
			}
		}
		else
		{
			break;
		}

#ifdef DEBUG_NODE_FILE
		G_DebugNodeFile();
#endif

		G_NodeClearForNext();
		i++;
	}

	RepairPaths(qtrue);
	//this has different behaviour for RMG and will just flag all points one way that don't trace to each other.

#ifdef PATH_TIME_DEBUG
	endTime = trap->Milliseconds();

	trap->Print("Total routing time taken: %ims\n", endTime - startTime);
#endif

#ifdef ASCII_ART_DEBUG
	CreateAsciiTableRepresentation();
#endif
}

static void BeginAutoPathRoutine(void)
{
	//Called for RMG levels.
	int i = 0;

	gSpawnPointNum = 0;

	CreateNewWP(vec3_origin, 0); //create a dummy waypoint to insert under

	while (i < level.num_entities)
	{
		gentity_t* ent = &g_entities[i];

		if (ent && ent->inuse && ent->classname && ent->classname[0] && !Q_stricmp(
			ent->classname, "info_player_deathmatch"))
		{
			if (ent->s.origin[2] < 1280)
			{
				//h4x
				gSpawnPoints[gSpawnPointNum] = ent;
				gSpawnPointNum++;
			}
		}
		else if (ent && ent->inuse && ent->item && ent->item->giType == IT_TEAM &&
			(ent->item->giTag == PW_REDFLAG || ent->item->giTag == PW_BLUEFLAG))
		{
			//also make it path to flags in CTF.
			gSpawnPoints[gSpawnPointNum] = ent;
			gSpawnPointNum++;
		}

		i++;
	}

	if (gSpawnPointNum < 1)
	{
		return;
	}

	G_RMGPathing();

	//rww - Using a faster in-engine version because we're having to wait for this stuff to get done as opposed to just saving it once.
	trap->BotUpdateWaypoints(gWPNum, gWPArray);
	trap->BotCalculatePaths(RMG.integer);
	//CalculatePaths(); //make everything nice and connected

	FlagObjects(); //currently only used for flagging waypoints nearest CTF flags

	i = 0;

	while (i < gWPNum - 1)
	{
		vec3_t v;
		//disttonext is normally set on save, and when a file is loaded. For RMG we must do it after calc'ing.
		VectorSubtract(gWPArray[i]->origin, gWPArray[i + 1]->origin, v);
		gWPArray[i]->disttonext = VectorLength(v);
		i++;
	}

	RemoveWP(); //remove the dummy point at the end of the trail
}

extern vmCvar_t bot_normgpath;

void LoadPath_ThisLevel(void)
{
	vmCvar_t mapname;
	int i = 0;

	trap->Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);

	if (RMG.integer)
	{
		//If RMG, generate the path on-the-fly
		trap->Cvar_Register(&bot_normgpath, "bot_normgpath", "1", CVAR_CHEAT);
		//note: This is disabled for now as I'm using standard bot nav
		//on premade terrain levels.

		if (!bot_normgpath.integer)
		{
			//autopath the random map
			BeginAutoPathRoutine();
		}
		else
		{
			//try loading standard nav data
			LoadPathData(mapname.string);
		}

		gLevelFlags |= LEVELFLAG_NOPOINTPREDICTION;
	}
	else
	{
		if (LoadPathData(mapname.string) == 2)
		{
			//enter "edit" mode if cheats enabled?
		}
	}

	trap->Cvar_Update(&bot_wp_edit);

	if (bot_wp_edit.value)
	{
		gBotEdit = 1;
	}
	else
	{
		gBotEdit = 0;
	}

	//set the flag entities
	while (i < level.num_entities)
	{
		gentity_t* ent = &g_entities[i];

		if (ent && ent->inuse && ent->classname)
		{
			if (!eFlagRed && strcmp(ent->classname, "team_CTF_redflag") == 0)
			{
				eFlagRed = ent;
			}
			else if (!eFlagBlue && strcmp(ent->classname, "team_CTF_blueflag") == 0)
			{
				eFlagBlue = ent;
			}

			if (eFlagRed && eFlagBlue)
			{
				break;
			}
		}

		i++;
	}
}

static gentity_t* GetClosestSpawn(const gentity_t* ent)
{
	gentity_t* closest_spawn = NULL;
	float closest_dist = -1;
	int i = MAX_CLIENTS;

	while (i < level.num_entities)
	{
		gentity_t* spawn = &g_entities[i];

		if (spawn && spawn->inuse && (!Q_stricmp(spawn->classname, "info_player_start") || !Q_stricmp(
			spawn->classname, "info_player_deathmatch")))
		{
			vec3_t v_sub;

			VectorSubtract(ent->client->ps.origin, spawn->r.currentOrigin, v_sub);
			const float checkDist = VectorLength(v_sub);

			if (closest_dist == -1 || checkDist < closest_dist)
			{
				closest_spawn = spawn;
				closest_dist = checkDist;
			}
		}

		i++;
	}

	return closest_spawn;
}

static gentity_t* GetNextSpawnInIndex(const gentity_t* current_spawn)
{
	gentity_t* next_spawn = NULL;
	int i = current_spawn->s.number + 1;

	gentity_t* spawn;

	while (i < level.num_entities)
	{
		spawn = &g_entities[i];

		if (spawn && spawn->inuse && (!Q_stricmp(spawn->classname, "info_player_start") || !Q_stricmp(
			spawn->classname, "info_player_deathmatch")))
		{
			next_spawn = spawn;
			break;
		}

		i++;
	}

	if (!next_spawn)
	{
		//loop back around to 0
		i = MAX_CLIENTS;

		while (i < level.num_entities)
		{
			spawn = &g_entities[i];

			if (spawn && spawn->inuse && (!Q_stricmp(spawn->classname, "info_player_start") || !Q_stricmp(
				spawn->classname, "info_player_deathmatch")))
			{
				next_spawn = spawn;
				break;
			}

			i++;
		}
	}

	return next_spawn;
}

int AcceptBotCommand(const char* cmd, const gentity_t* pl)
{
	vmCvar_t mapname;

	if (!gBotEdit)
	{
		return 0;
	}

	int optional_argument = 0;
	int i = 0;
	int flags_from_argument = 0;
	const char* optional_s_argument;
	const char* required_s_argument;

	//if a waypoint editing related command is issued, bots will deactivate.
	//once bot_wp_save is issued and the trail is recalculated, bots will activate again.

	if (!pl || !pl->client)
	{
		return 0;
	}

	if (Q_stricmp(cmd, "bot_wp_Cmdlist") == 0) //lists all the bot waypoint commands.
	{
		trap->Print(
			S_COLOR_YELLOW "bot_wp_add" S_COLOR_WHITE
			" - Add a waypoint (optional int parameter will insert the point after the specified waypoint index in a trail)\n\n");
		trap->Print(
			S_COLOR_YELLOW "bot_wp_rem" S_COLOR_WHITE
			" - Remove a waypoint (removes last unless waypoint index is specified as a parameter)\n\n");
		trap->Print(
			S_COLOR_YELLOW "bot_wp_addflagged" S_COLOR_WHITE
			" - Same as wp_add, but adds a flagged point (type bot_wp_addflagged for help)\n\n");
		trap->Print(
			S_COLOR_YELLOW "bot_wp_switchflags" S_COLOR_WHITE
			" - Switches flags on an existing waypoint (type bot_wp_switchflags for help)\n\n");
		trap->Print(
			S_COLOR_YELLOW "bot_wp_tele" S_COLOR_WHITE " - Teleport yourself to the specified waypoint's location\n");
		trap->Print(
			S_COLOR_YELLOW "bot_wp_killoneways" S_COLOR_WHITE
			" - Removes oneway (backward and forward) flags on all waypoints in the level\n\n");
		trap->Print(
			S_COLOR_YELLOW "bot_wp_save" S_COLOR_WHITE " - Saves all waypoint data into a file for later use\n");

		return 1;
	}

	if (Q_stricmp(cmd, "bot_wp_add") == 0)
	{
		gDeactivated = 1;
		optional_s_argument = ConcatArgs(1);

		if (optional_s_argument)
		{
			optional_argument = atoi(optional_s_argument);
		}

		if (optional_s_argument && optional_s_argument[0])
		{
			CreateNewWP_InTrail(pl->client->ps.origin, 0, optional_argument);
		}
		else
		{
			CreateNewWP(pl->client->ps.origin, 0);
		}
		return 1;
	}

	if (Q_stricmp(cmd, "bot_wp_rem") == 0)
	{
		gDeactivated = 1;

		optional_s_argument = ConcatArgs(1);

		if (optional_s_argument)
		{
			optional_argument = atoi(optional_s_argument);
		}

		if (optional_s_argument && optional_s_argument[0])
		{
			RemoveWP_InTrail(optional_argument);
		}
		else
		{
			RemoveWP();
		}

		return 1;
	}

	if (Q_stricmp(cmd, "bot_wp_tele") == 0)
	{
		gDeactivated = 1;
		optional_s_argument = ConcatArgs(1);

		if (optional_s_argument)
		{
			optional_argument = atoi(optional_s_argument);
		}

		if (optional_s_argument && optional_s_argument[0])
		{
			TeleportToWP(pl, optional_argument);
		}
		else
		{
			trap->Print(S_COLOR_YELLOW "You didn't specify an index. Assuming last.\n");
			TeleportToWP(pl, gWPNum - 1);
		}
		return 1;
	}

	if (Q_stricmp(cmd, "bot_wp_spawntele") == 0)
	{
		const gentity_t* closest_spawn = GetClosestSpawn(pl);

		if (!closest_spawn)
		{
			//There should always be a spawn point..
			return 1;
		}

		closest_spawn = GetNextSpawnInIndex(closest_spawn);

		if (closest_spawn)
		{
			VectorCopy(closest_spawn->r.currentOrigin, pl->client->ps.origin);
		}
		return 1;
	}

	if (Q_stricmp(cmd, "bot_wp_addflagged") == 0)
	{
		gDeactivated = 1;

		required_s_argument = ConcatArgs(1);

		if (!required_s_argument || !required_s_argument[0])
		{
			trap->Print(
				S_COLOR_YELLOW
				"Flag string needed for bot_wp_addflagged\nj - Jump point\nd - Duck point\nc - Snipe or camp standing\nf - Wait for func\nm - Do not move to when func is under\ns - Snipe or camp\nx - Oneway, forward\ny - Oneway, back\ng - Mission goal\nn - No visibility\nExample (for a point the bot would jump at, and reverse on when traveling a trail backwards):\nbot_wp_addflagged jx\n");
			return 1;
		}

		while (required_s_argument[i])
		{
			if (required_s_argument[i] == 'j')
			{
				flags_from_argument |= WPFLAG_JUMP;
			}
			else if (required_s_argument[i] == 'd')
			{
				flags_from_argument |= WPFLAG_DUCK;
			}
			else if (required_s_argument[i] == 'c')
			{
				flags_from_argument |= WPFLAG_SNIPEORCAMPSTAND;
			}
			else if (required_s_argument[i] == 'f')
			{
				flags_from_argument |= WPFLAG_WAITFORFUNC;
			}
			else if (required_s_argument[i] == 's')
			{
				flags_from_argument |= WPFLAG_SNIPEORCAMP;
			}
			else if (required_s_argument[i] == 'x')
			{
				flags_from_argument |= WPFLAG_ONEWAY_FWD;
			}
			else if (required_s_argument[i] == 'y')
			{
				flags_from_argument |= WPFLAG_ONEWAY_BACK;
			}
			else if (required_s_argument[i] == 'g')
			{
				flags_from_argument |= WPFLAG_GOALPOINT;
			}
			else if (required_s_argument[i] == 'n')
			{
				flags_from_argument |= WPFLAG_NOVIS;
			}
			else if (required_s_argument[i] == 'm')
			{
				flags_from_argument |= WPFLAG_NOMOVEFUNC;
			}
			else if (required_s_argument[i] == 'q')
			{
				flags_from_argument |= WPFLAG_DESTROY_FUNCBREAK;
			}
			else if (required_s_argument[i] == 'r')
			{
				flags_from_argument |= WPFLAG_REDONLY;
			}
			else if (required_s_argument[i] == 'b')
			{
				flags_from_argument |= WPFLAG_BLUEONLY;
			}
			else if (required_s_argument[i] == 'p')
			{
				flags_from_argument |= WPFLAG_FORCEPUSH;
			}
			else if (required_s_argument[i] == 'o')
			{
				flags_from_argument |= WPFLAG_FORCEPULL;
			}

			i++;
		}

		optional_s_argument = ConcatArgs(2);

		if (optional_s_argument)
		{
			optional_argument = atoi(optional_s_argument);
		}

		if (optional_s_argument && optional_s_argument[0])
		{
			CreateNewWP_InTrail(pl->client->ps.origin, flags_from_argument, optional_argument);
		}
		else
		{
			CreateNewWP(pl->client->ps.origin, flags_from_argument);
		}
		return 1;
	}

	if (Q_stricmp(cmd, "bot_wp_switchflags") == 0)
	{
		gDeactivated = 1;

		required_s_argument = ConcatArgs(1);

		if (!required_s_argument || !required_s_argument[0])
		{
			trap->Print(
				S_COLOR_YELLOW
				"Flag string needed for bot_wp_switchflags\nType bot_wp_addflagged for a list of flags and their corresponding characters, or use 0 for no flags.\nSyntax: bot_wp_switchflags <flags> <n>\n");
			return 1;
		}

		while (required_s_argument[i])
		{
			if (required_s_argument[i] == 'j')
			{
				flags_from_argument |= WPFLAG_JUMP;
			}
			else if (required_s_argument[i] == 'd')
			{
				flags_from_argument |= WPFLAG_DUCK;
			}
			else if (required_s_argument[i] == 'c')
			{
				flags_from_argument |= WPFLAG_SNIPEORCAMPSTAND;
			}
			else if (required_s_argument[i] == 'f')
			{
				flags_from_argument |= WPFLAG_WAITFORFUNC;
			}
			else if (required_s_argument[i] == 's')
			{
				flags_from_argument |= WPFLAG_SNIPEORCAMP;
			}
			else if (required_s_argument[i] == 'x')
			{
				flags_from_argument |= WPFLAG_ONEWAY_FWD;
			}
			else if (required_s_argument[i] == 'y')
			{
				flags_from_argument |= WPFLAG_ONEWAY_BACK;
			}
			else if (required_s_argument[i] == 'g')
			{
				flags_from_argument |= WPFLAG_GOALPOINT;
			}
			else if (required_s_argument[i] == 'n')
			{
				flags_from_argument |= WPFLAG_NOVIS;
			}
			else if (required_s_argument[i] == 'm')
			{
				flags_from_argument |= WPFLAG_NOMOVEFUNC;
			}
			else if (required_s_argument[i] == 'q')
			{
				flags_from_argument |= WPFLAG_DESTROY_FUNCBREAK;
			}
			else if (required_s_argument[i] == 'r')
			{
				flags_from_argument |= WPFLAG_REDONLY;
			}
			else if (required_s_argument[i] == 'b')
			{
				flags_from_argument |= WPFLAG_BLUEONLY;
			}
			else if (required_s_argument[i] == 'p')
			{
				flags_from_argument |= WPFLAG_FORCEPUSH;
			}
			else if (required_s_argument[i] == 'o')
			{
				flags_from_argument |= WPFLAG_FORCEPULL;
			}

			i++;
		}

		optional_s_argument = ConcatArgs(2);

		if (optional_s_argument)
		{
			optional_argument = atoi(optional_s_argument);
		}

		if (optional_s_argument && optional_s_argument[0])
		{
			WPFlagsModify(optional_argument, flags_from_argument);
		}
		else
		{
			trap->Print(
				S_COLOR_YELLOW
				"Waypoint number (to modify) needed for bot_wp_switchflags\nSyntax: bot_wp_switchflags <flags> <n>\n");
		}
		return 1;
	}

	if (Q_stricmp(cmd, "bot_wp_killoneways") == 0)
	{
		i = 0;

		while (i < gWPNum)
		{
			if (gWPArray[i] && gWPArray[i]->inuse)
			{
				if (gWPArray[i]->flags & WPFLAG_ONEWAY_FWD)
				{
					gWPArray[i]->flags -= WPFLAG_ONEWAY_FWD;
				}
				if (gWPArray[i]->flags & WPFLAG_ONEWAY_BACK)
				{
					gWPArray[i]->flags -= WPFLAG_ONEWAY_BACK;
				}
			}

			i++;
		}

		return 1;
	}

	if (Q_stricmp(cmd, "bot_wp_save") == 0)
	{
		gDeactivated = 0;
		trap->Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);
		SavePathData(mapname.string);
		return 1;
	}

	return 0;
}