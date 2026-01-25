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

// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_headers.h"

#include "cg_media.h"	//just for cgs....

void CG_TargetCommand_f();
extern qboolean player_locked;
extern void CMD_CGCam_Disable();
void cg_next_inventory_f();
void cg_prev_inventory_f();
void CG_NextForcePower_f();
void CG_PrevForcePower_f();

/*
=================
CG_TargetCommand_f

=================
*/
void CG_TargetCommand_f()
{
	char test[4];

	const int targetNum = CG_CrosshairPlayer();
	if (targetNum == -1)
	{
		return;
	}

	cgi_Argv(1, test, 4);
	cgi_SendClientCommand(va("gc %i %i", targetNum, atoi(test)));
}

/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f()
{
	CG_Printf("%s (%i %i %i) : %i %i %i\n", cgs.mapname, static_cast<int>(cg.refdef.vieworg[0]),
		static_cast<int>(cg.refdef.vieworg[1]), static_cast<int>(cg.refdef.vieworg[2]),
		static_cast<int>(cg.refdefViewAngles[PITCH]),
		static_cast<int>(cg.refdefViewAngles[YAW]), static_cast<int>(cg.refdefViewAngles[ROLL]));
}

static void CG_WriteCam_f()
{
	char text[1024];
	static int numCams;

	numCams++;

	const char* targetname = CG_Argv(1);

	if (!targetname || !targetname[0])
	{
		targetname = "nameme!";
	}

	CG_Printf("Camera #%d ('%s') written to: ", numCams, targetname);
	Com_sprintf(text, sizeof text,
		"//entity %d\n{\n\"classname\"	\"ref_tag\"\n\"targetname\"	\"%s\"\n\"origin\" \"%i %i %i\"\n\"angles\" \"%i %i %i\"\n\"fov\" \"%i\"\n}\n",
		numCams, targetname, static_cast<int>(cg.refdef.vieworg[0]), static_cast<int>(cg.refdef.vieworg[1]),
		static_cast<int>(cg.refdef.vieworg[2]), static_cast<int>(cg.refdefViewAngles[0]),
		static_cast<int>(cg.refdefViewAngles[1]), static_cast<int>(cg.refdefViewAngles[2]), cg_fov.integer);
	gi.WriteCam(text);
}

static void Lock_Disable()
{
	player_locked = qfalse;
}

extern float cg_zoomFov; //from cg_view.cpp

void CG_ToggleBinoculars()
{
	if (in_camera || !cg.snap)
	{
		return;
	}

	if (cg.zoomMode == 0 || cg.zoomMode >= 2) // not zoomed or currently zoomed with the disruptor or LA goggles
	{
		if (cg.snap->ps.saber[0].Active() && cg.snap->ps.saberInFlight || cg.snap->ps.stats[STAT_HEALTH] <= 0)
		{
			return;
		}

		if (cg.snap->ps.viewEntity || cg_entities[cg.snap->ps.clientNum].currentState.eFlags & (EF_LOCKED_TO_WEAPON | EF_IN_ATST))
		{
			return;
		}

		cg.zoomMode = 1;
		cg.zoomLocked = qfalse;

		if (cg.weaponSelect == WP_SABER)
		{
			cg.weaponSelect = WP_MELEE;
		}

		if (cg.snap->ps.batteryCharge)
		{
			// when you have batteries, you can actually zoom in
			cg_zoomFov = 40.0f;
		}
		else if (cg.overrides.active & CG_OVERRIDE_FOV)
		{
			cg_zoomFov = cg.overrides.fov;
		}
		else
		{
			cg_zoomFov = cg_fov.value;
		}

		cgi_S_StartSound(nullptr, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.zoomStart);
	}
	else
	{
		cg.zoomMode = 0;
		cg.zoomTime = cg.time;
		cgi_S_StartSound(nullptr, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.zoomEnd);

		if (cg.weaponSelect == WP_MELEE && cg.snap->ps.stats[STAT_WEAPONS] & 1 << WP_SABER)
		{
			cg.weaponSelect = WP_SABER;
		}
	}
}

void CG_ToggleLAGoggles()
{
	if (in_camera || !cg.snap)
	{
		return;
	}

	if (cg.zoomMode == 0 || cg.zoomMode < 3) // not zoomed or currently zoomed with the disruptor or regular binoculars
	{
		if (cg.snap->ps.saber[0].Active() && cg.snap->ps.saberInFlight || cg.snap->ps.stats[STAT_HEALTH] <= 0)
		{
			return;
		}

		if (cg.snap->ps.viewEntity || cg_entities[cg.snap->ps.clientNum].currentState.eFlags & (EF_LOCKED_TO_WEAPON | EF_IN_ATST))
		{
			return;
		}

		cg.zoomMode = 3;
		cg.zoomLocked = qfalse;
		if (cg.overrides.active & CG_OVERRIDE_FOV)
		{
			cg_zoomFov = cg.overrides.fov;
		}
		else
		{
			cg_zoomFov = cg_fov.value; // does not zoom!!
		}

		cgi_S_StartSound(nullptr, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.zoomStart);
	}
	else
	{
		cg.zoomMode = 0;
		cg.zoomTime = cg.time;
		cgi_S_StartSound(nullptr, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.zoomEnd);
	}
}

static void CG_LoadHud_f()
{
	const char* hudSet = cg_hudFiles.string;

	if (hudSet[0] == '\0')
	{
		hudSet = "ui/sje-hud.txt";
	}
	CG_LoadMenus(hudSet);
}

using consoleCommand_t = struct
{
	const char* cmd;
	void (*func)();
};

static int cmdcmp(const void* a, const void* b)
{
	return Q_stricmp(static_cast<const char*>(a), ((consoleCommand_t*)b)->cmd);
}

/* This array MUST be sorted correctly by alphabetical name field */
static consoleCommand_t commands[] = {
	{"cam_disable", CMD_CGCam_Disable}, //gets out of camera mode for debuggin
	{"cam_enable", CGCam_Enable}, //gets into camera mode for precise camera placement
	{"dpforcenext", CG_DPNextForcePower_f},
	{"dpforceprev", CG_DPPrevForcePower_f},
	{"dpinvnext", CG_DPNextInventory_f},
	{"dpinvprev", CG_DPPrevInventory_f},
	{"dpweapnext", CG_DPNextWeapon_f},
	{"dpweapprev", CG_DPPrevWeapon_f},
	{"forcenext", CG_NextForcePower_f},
	{"forceprev", CG_PrevForcePower_f},
	{"invnext", cg_next_inventory_f},
	{"invprev", cg_prev_inventory_f},
	{"la_zoom", CG_ToggleLAGoggles},
	{"loadhud", CG_LoadHud_f},
	{"lock_disable", Lock_Disable}, //player can move now
	{"nextframe", CG_TestModelNextFrame_f},
	{"nextskin", CG_TestModelNextSkin_f},
	{"prevframe", CG_TestModelPrevFrame_f},
	{"prevskin", CG_TestModelPrevSkin_f},
	{"tcmd", CG_TargetCommand_f},
	{"testG2Model", CG_TestG2Model_f},
	{"testanglespost", CG_TestModelSetAnglespost_f},
	{"testanglespre", CG_TestModelSetAnglespre_f},
	{"testanimate", CG_TestModelAnimate_f},
	{"testlistbones", CG_ListModelBones_f},
	{"testlistsurfaces", CG_ListModelSurfaces_f},
	{"testmodel", CG_TestModel_f},
	{"testsurface", CG_TestModelSurfaceOnOff_f},
	{"viewpos", CG_Viewpos_f},
	{"weapnext", CG_NextWeapon_f},
	{"weapon", CG_Weapon_f},
	{"weapprev", CG_PrevWeapon_f},
	{"writecam", CG_WriteCam_f},
	{"zoom", CG_ToggleBinoculars},
};

static constexpr size_t num_commands = std::size(commands);

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand()
{
	const consoleCommand_t* command = static_cast<consoleCommand_t*>(Q_LinearSearch(CG_Argv(0), commands, num_commands,
		sizeof commands[0], cmdcmp));

	if (!command)
		return qfalse;

	command->func();
	return qtrue;
}

static const char* gcmds[] =
{
	"bow",
	"entitylist",
	"difficulty",
	"flourish",
	"force_absorb",
	"force_distract",
	"force_grip",
	"force_heal",
	"force_protect",
	"force_pull",
	"force_rage",
	"force_sight",
	"force_speed",
	"force_stasis",
	"force_destruction",
	"force_grasp",
	"force_repulse",
	"force_strike",
	"force_blast",
	"force_fear",
	"force_deadlysight",
	"force_insanity",
	"force_blinding",
	"force_throw",
	"give",
	"gloat",
	"god",
	"invuse",
	"kill",
	"meditate",
	"nav",
	"noclip",
	"notarget",
	"npc",
	"playermodel",
	"playerteam",
	"playertint",
	"runscript",
	"saber",
	"saberAttackCycle",
	"saberColor",
	"saberblade",
	"secrets",
	"setForceAll",
	"setSaberAll",
	"setobjective",
	"setviewpos",
	"taunt",
	"undying",
	"use_bacta",
	"use_electrobinoculars",
	"use_lightamp_goggles",
	"use_seeker",
	"use_sentry",
	"viewobjective",
	"+button9",
	"+button10",
	"+button11",
	"+button12",
	"+button13",
	"+button14",
	"+button15",
	"+button16",
	"+button17",
	"+button18",
	"+button19",
	"+button20",
	"giveForceAll",
	"use_Cloak",
	"weather",
	"saberdown",
	//emotes
	"myhead",
	"cower",
	"smack",
	"enraged",
	"victory2",
	"victory3",
	"swirl",
	"dance",
	"dance2",
	"dance3",
	"sit2",
	"point",
	"kneel2",
	"kneel",
	"laydown",
	"breakdance",
	"cheer",
	"comeon",
	"headshake",
	"headnod",
	"surrender",
	"reload",
	"atease",
	"punch",
	"intimidate",
	"slash",
	"sit",
	"harlem",
	"hello",
	"hips",
	"won",
	"emote",
	"use_barrier",
	"use_jetpack",
	"scale",
	"r_weather"
};

static constexpr size_t numgcmds = std::size(gcmds);

/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands()
{
	size_t i;

	for (i = 0; i < num_commands; i++)
		cgi_AddCommand(commands[i].cmd);

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	for (i = 0; i < numgcmds; i++)
		cgi_AddCommand(gcmds[i]);
}