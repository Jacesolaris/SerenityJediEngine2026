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

#include "../server/exe_headers.h"

#include "ui_local.h"

/*
===============================================================================

CONNECTION SCREEN

===============================================================================
*/

char connectionDialogString[1024];
char connectionMessageString[1024];

#ifdef JK2_MODE
/*
 =================
 UI_DrawThumbNail
 =================
 */
static void UI_DrawThumbNail(float x, float y, float w, float h, byte* pic)
{
	ui.DrawStretchRaw(x, y, w, h, SG_SCR_WIDTH, SG_SCR_HEIGHT, pic, 0, qtrue);
}
#endif

/*
========================
UI_DrawConnect

========================
*/

void UI_DrawConnect(const char* servername, const char* updateInfoString)
{
	UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader);
}

/*
========================
UI_UpdateConnectionString

========================
*/
void UI_UpdateConnectionString(const char* string)
{
	Q_strncpyz(connectionDialogString, string, sizeof connectionDialogString);
	UI_UpdateScreen();
}

/*
========================
UI_UpdateConnectionMessageString

========================
*/
void UI_UpdateConnectionMessageString(const char* string)
{
	Q_strncpyz(connectionMessageString, string, sizeof connectionMessageString);

	// strip \n
	char* s = strstr(connectionMessageString, "\n");
	if (s)
	{
		*s = 0;
	}
	UI_UpdateScreen();
}

/*
===================
UI_KeyConnect
===================
*/
static void UI_KeyConnect(const int key)
{
	if (key == A_ESCAPE)
	{
		ui.Cmd_ExecuteText(EXEC_APPEND, "disconnect\n");
	}
}