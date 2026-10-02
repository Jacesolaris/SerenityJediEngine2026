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

/*****************************************************************************
 * name:		be_ea.c
 *
 * desc:		elementary actions
 *
 * $Archive: /MissionPack/code/botlib/be_ea.c $
 * $Author: Zaphod $
 * $Revision: 5 $
 * $Modtime: 11/22/00 8:50a $
 * $Date: 11/22/00 8:55a $
 *
 *****************************************************************************/

#include "qcommon/q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "botlib.h"
#include "be_interface.h"
#include "be_ea.h"

constexpr auto MAX_USERMOVE = 400;
constexpr auto MAX_COMMANDARGUMENTS = 10;
constexpr auto ACTION_JUMPEDLASTFRAME = 0x0800000;//128;

bot_input_t* botinputs;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_Say(const int client, char* str)
{
	botimport.BotClientCommand(client, va("say %s", str));
} //end of the function EA_Say
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_SayTeam(const int client, char* str)
{
	botimport.BotClientCommand(client, va("say_team %s", str));
} //end of the function EA_SayTeam
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_Tell(const int client, const int clientto, char* str)
{
	botimport.BotClientCommand(client, va("tell %d, %s", clientto, str));
} //end of the function EA_SayTeam
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_UseItem(const int client, char* it)
{
	botimport.BotClientCommand(client, va("use %s", it));
} //end of the function EA_UseItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_DropItem(const int client, char* it)
{
	botimport.BotClientCommand(client, va("drop %s", it));
} //end of the function EA_DropItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_UseInv(const int client, char* inv)
{
	botimport.BotClientCommand(client, va("invuse %s", inv));
} //end of the function EA_UseInv
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_DropInv(const int client, char* inv)
{
	botimport.BotClientCommand(client, va("invdrop %s", inv));
} //end of the function EA_DropInv
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Gesture(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_GESTURE;
} //end of the function EA_Gesture
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_Command(const int client, char* command)
{
	botimport.BotClientCommand(client, command);
} //end of the function EA_Command
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_SelectWeapon(const int client, const int weapon)
{
	bot_input_t* bi = &botinputs[client];

	bi->weapon = weapon;
} //end of the function EA_SelectWeapon
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Attack(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_ATTACK;
} //end of the function EA_Attack
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Alt_Attack(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_ALT_ATTACK;
} //end of the function EA_Alt_Attack
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_ForcePower(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_FORCEPOWER;
} //end of the function EA_ForcePower
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Talk(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_TALK;
} //end of the function EA_Talk
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Use(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_USE;
} //end of the function EA_Use
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Respawn(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_RESPAWN;
} //end of the function EA_Respawn
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Jump(const int client)
{
	bot_input_t* bi = &botinputs[client];

	if (bi->actionflags & ACTION_JUMPEDLASTFRAME)
	{
		bi->actionflags &= ~ACTION_JUMP;
	} //end if
	else
	{
		bi->actionflags |= ACTION_JUMP;
	} //end if
} //end of the function EA_Jump
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_DelayedJump(const int client)
{
	bot_input_t* bi = &botinputs[client];

	if (bi->actionflags & ACTION_JUMPEDLASTFRAME)
	{
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	} //end if
	else
	{
		bi->actionflags |= ACTION_DELAYEDJUMP;
	} //end if
} //end of the function EA_DelayedJump
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Crouch(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_CROUCH;
} //end of the function EA_Crouch
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Walk(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_WALK;
} //end of the function EA_Walk
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Action(const int client, const int action)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= action;
} //end of function EA_Action
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveUp(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEUP;
} //end of the function EA_MoveUp
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveDown(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEDOWN;
} //end of the function EA_MoveDown
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveForward(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEFORWARD;
} //end of the function EA_MoveForward
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveBack(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEBACK;
} //end of the function EA_MoveBack
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveLeft(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVELEFT;
} //end of the function EA_MoveLeft
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveRight(const int client)
{
	bot_input_t* bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVERIGHT;
} //end of the function EA_MoveRight
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Move(const int client, vec3_t dir, float speed)
{
	bot_input_t* bi = &botinputs[client];

	VectorCopy(dir, bi->dir);
	//cap speed
	if (speed > MAX_USERMOVE) speed = MAX_USERMOVE;
	else if (speed < -MAX_USERMOVE) speed = -MAX_USERMOVE;
	bi->speed = speed;
} //end of the function EA_Move
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_View(const int client, vec3_t viewangles)
{
	bot_input_t* bi = &botinputs[client];

	VectorCopy(viewangles, bi->viewangles);
} //end of the function EA_View
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_EndRegular(int client, float thinktime)
{
} //end of the function EA_EndRegular
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_GetInput(const int client, const float thinktime, bot_input_t* input)
{
	bot_input_t* bi = &botinputs[client];
	bi->thinktime = thinktime;
	Com_Memcpy(input, bi, sizeof(bot_input_t));
} //end of the function EA_GetInput
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_ResetInput(const int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags &= ~ACTION_JUMPEDLASTFRAME;

	bi->thinktime = 0;
	VectorClear(bi->dir);
	bi->speed = 0;
	const int jumped = bi->actionflags & ACTION_JUMP;
	bi->actionflags = 0;
	if (jumped) bi->actionflags |= ACTION_JUMPEDLASTFRAME;
} //end of the function EA_ResetInput
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int EA_Setup(void)
{
	//initialize the bot inputs
	botinputs = static_cast<bot_input_t*>(GetClearedHunkMemory(
		botlibglobals.maxclients * sizeof(bot_input_t)));
	return BLERR_NOERROR;
} //end of the function EA_Setup
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Shutdown(void)
{
	FreeMemory(botinputs);
	botinputs = nullptr;
} //end of the function EA_Shutdown