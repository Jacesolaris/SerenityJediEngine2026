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

#include "cg_headers.h"

#include "cg_media.h"
#include "../game/objectives.h"

// For printing objectives
static constexpr short objectiveStartingYpos = 75; // Y starting position for objective text
static constexpr short objectiveStartingXpos = 60; // X starting position for objective text
static constexpr int objectiveTextBoxWidth = 500; // Width (in pixels) of text box
static constexpr int objectiveTextBoxHeight = 300; // Height (in pixels) of text box
static constexpr short missionYpos = 79;
extern vmCvar_t cg_com_rend2;
extern vmCvar_t cg_com_outcast;

const char* showLoadPowersName[] =
{
	"SP_INGAME_HEAL2",
	"SP_INGAME_JUMP2",
	"SP_INGAME_SPEED2",
	"SP_INGAME_PUSH2",
	"SP_INGAME_PULL2",
	"SP_INGAME_MINDTRICK2",
	"SP_INGAME_GRIP2",
	"SP_INGAME_LIGHTNING2",
	"SP_INGAME_SABER_THROW2",
	"SP_INGAME_SABER_OFFENSE2",
	"SP_INGAME_SABER_DEFENSE2",
	"SP_INGAME_STASIS2",
	"SP_INGAME_DESTRUCTION2",
	"SP_INGAME_GRASP2",
	"SP_INGAME_REPULSE2",
	"SP_INGAME_LIGHTNING_STRIKE2",
	"SP_INGAME_BLAST2",
	"SP_INGAME_FEAR2",
	"SP_INGAME_DEADLYSIGHT2",
	"SP_INGAME_INSANITY2",
	"SP_INGAME_BLINDING2",
	nullptr,
};

constexpr auto MAX_OBJ_GRAPHICS = 4;
constexpr auto OBJ_GRAPHIC_SIZE = 240;
int obj_graphics[MAX_OBJ_GRAPHICS];

qboolean CG_ForcePower_Valid(int force_known_bits, int index);

/*
====================
ObjectivePrint_Line

Print a single mission objective
====================
*/
static void ObjectivePrint_Line(const int color, const int object_index, int& mission_ycnt)
{
	int y;
	char finalText[2048];
	qhandle_t graphic;

	const int iYPixelsPerLine = cgi_R_Font_HeightPixels(cgs.media.qhFontMedium, 1.0f);

	cgi_SP_GetStringTextString(va("OBJECTIVES_%s", objectiveTable[object_index].name), finalText, sizeof finalText);

	// A hack to be able to count prisoners
	if (object_index == T2_RANCOR_OBJ5)
	{
		char value[64];

		gi.Cvar_VariableStringBuffer("ui_prisonerobj_currtotal", value, sizeof value);
		const int curr_total = atoi(value);
		gi.Cvar_VariableStringBuffer("ui_prisonerobj_maxtotal", value, sizeof value);
		const int min_total = atoi(value);

		Q_strncpyz(finalText, va(finalText, curr_total, min_total), sizeof finalText);
	}

	int pixelLen = cgi_R_Font_StrLenPixels(finalText, cgs.media.qhFontMedium, 1.0f);

	char* str = finalText;

	if (cgi_Language_IsAsian())
	{
		// this is execrable, and should NOT have had to've been done now, but...
		//
		extern const char* CG_DisplayBoxedText(int iBoxX, int iBoxY, int iBoxWidth, int iBoxHeight,
			const char* psText, int iFontHandle, float fScale,
			const vec4_t v4Color);
		extern int gi_lines_output;
		extern float gf_advance_hack;

		gf_advance_hack = 1.0f; // override internal vertical advance
		y = objectiveStartingYpos + iYPixelsPerLine * mission_ycnt;

		// Advance line if a graphic has printed
		for (const int obj_graphic : obj_graphics)
		{
			if (obj_graphic)
			{
				y += OBJ_GRAPHIC_SIZE + 4;
			}
		}

		CG_DisplayBoxedText(
			objectiveStartingXpos,
			y,
			objectiveTextBoxWidth,
			objectiveTextBoxHeight,
			finalText, // int iBoxX, int iBoxY, int iBoxWidth, int iBoxHeight, const char *psText
			cgs.media.qhFontMedium, // int iFontHandle,
			1.0f, // float fScale,
			colorTable[color] // const vec4_t v4Color
		);

		gf_advance_hack = 0.0f; // restore
		mission_ycnt += gi_lines_output;
	}
	else
	{
		// western...
		//
		if (pixelLen < objectiveTextBoxWidth) // One shot - small enough to print entirely on one line
		{
			y = objectiveStartingYpos + iYPixelsPerLine * mission_ycnt;

			cgi_R_Font_DrawString(
				objectiveStartingXpos,
				y,
				str,
				colorTable[color],
				cgs.media.qhFontMedium,
				-1,
				1.0f);

			++mission_ycnt;
		}
		// Text is too long, break into lines.
		else
		{
			constexpr int max_hold_text = 1024;
			char hold_text[max_hold_text];
			char hold_text2[2]{};
			pixelLen = 0;
			int char_len = 0;
			hold_text2[1] = '\0';
			const char* str_begin = str;

			while (*str)
			{
				hold_text2[0] = *str;
				pixelLen += cgi_R_Font_StrLenPixels(hold_text2, cgs.media.qhFontMedium, 1.0f);

				pixelLen += 2; // For kerning
				++char_len;

				if (pixelLen > objectiveTextBoxWidth)
				{
					//Reached max length of this line
					//step back until we find a space
					while (char_len > 10 && *str != ' ')
					{
						--str;
						--char_len;
					}

					if (*str == ' ')
					{
						++str; // To get past space
					}

					assert(char_len < max_hold_text); // Too big?

					Q_strncpyz(hold_text, str_begin, char_len);
					hold_text[char_len] = '\0';
					str_begin = str;
					pixelLen = 0;
					char_len = 1;

					y = objectiveStartingYpos + iYPixelsPerLine * mission_ycnt;

					CG_DrawProportionalString(
						objectiveStartingXpos,
						y,
						hold_text,
						CG_SMALLFONT,
						colorTable[color]);

					++mission_ycnt;
				}
				else if (*(str + 1) == '\0')
				{
					++char_len;

					assert(char_len < max_hold_text); // Too big?

					y = objectiveStartingYpos + iYPixelsPerLine * mission_ycnt;

					Q_strncpyz(hold_text, str_begin, char_len);
					CG_DrawProportionalString(
						objectiveStartingXpos,
						y, hold_text,
						CG_SMALLFONT,
						colorTable[color]);

					++mission_ycnt;
					break;
				}
				++str;
			}
		}
	}

	if (object_index == T3_BOUNTY_OBJ1)
	{
		y = objectiveStartingYpos + iYPixelsPerLine * mission_ycnt;
		if (obj_graphics[1])
		{
			y += OBJ_GRAPHIC_SIZE + 4;
		}
		if (obj_graphics[2])
		{
			y += OBJ_GRAPHIC_SIZE + 4;
		}
		graphic = cgi_R_RegisterShaderNoMip("textures/system/viewscreen1");
		CG_DrawPic(355, 50, OBJ_GRAPHIC_SIZE, OBJ_GRAPHIC_SIZE, graphic);
		obj_graphics[3] = qtrue;
	} //// Special case hack
	else if (object_index == DOOM_COMM_OBJ4)
	{
		y = missionYpos + iYPixelsPerLine * mission_ycnt;
		graphic = cgi_R_RegisterShaderNoMip("textures/system/securitycode");
		CG_DrawPic(320 - 128 / 2, y + 8, 128, 32, graphic);
		obj_graphics[0] = qtrue;
	}
	else if (object_index == KEJIM_POST_OBJ3)
	{
		y = missionYpos + iYPixelsPerLine * mission_ycnt;
		graphic = cgi_R_RegisterShaderNoMip("textures/system/securitycode_red");
		CG_DrawPic(320 - 32 / 2, y + 8, 32, 32, graphic);
		obj_graphics[1] = qtrue;
	}
	else if (object_index == KEJIM_POST_OBJ4)
	{
		y = missionYpos + iYPixelsPerLine * mission_ycnt;
		if (obj_graphics[1])
		{
			y += 32 + 4;
		}
		graphic = cgi_R_RegisterShaderNoMip("textures/system/securitycode_green");
		CG_DrawPic(320 - 32 / 2, y + 8, 32, 32, graphic);
		obj_graphics[2] = qtrue;
	}
	else if (object_index == KEJIM_POST_OBJ5)
	{
		y = missionYpos + iYPixelsPerLine * mission_ycnt;
		if (obj_graphics[1])
		{
			y += 32 + 4;
		}
		if (obj_graphics[2])
		{
			y += 32 + 4;
		}
		graphic = cgi_R_RegisterShaderNoMip("textures/system/securitycode_blue");
		CG_DrawPic(320 - 32 / 2, y + 8, 32, 32, graphic);
		obj_graphics[3] = qtrue;
	}
}

/*
====================
CG_DrawDataPadObjectives

Draw routine for the objective info screen of the data pad.
====================
*/
void CG_DrawDataPadObjectives(const centity_t* cent)
{
	int i;
	const int i_y_pixels_per_line = cgi_R_Font_HeightPixels(cgs.media.qhFontMedium, 1.0f);

	constexpr short title_x_pos = objectiveStartingXpos - 22; // X starting position for title text
	constexpr short title_y_pos = objectiveStartingYpos - 23; // Y starting position for title text
	constexpr short graphic_size = 16; // Size (width and height) of graphic used to show status of objective
	constexpr short graphic_xpos = objectiveStartingXpos - graphic_size - 8;
	// Amount of X to backup from text starting position
	const short graphic_y_offset = (i_y_pixels_per_line - graphic_size) / 2;
	// Amount of Y to raise graphic so it's in the center of the text line

	missionInfo_Updated = qfalse; // This will stop the text from flashing
	cg.missionInfoFlashTime = 0;

	// zero out objective graphics
	for (i = 0; i < MAX_OBJ_GRAPHICS; i++)
	{
		obj_graphics[i] = qfalse;
	}

	// Title Text at the top
	char text[1024] = { 0 };
	cgi_SP_GetStringTextString("SP_INGAME_OBJECTIVES", text, sizeof text);
	cgi_R_Font_DrawString(title_x_pos, title_y_pos, text, colorTable[CT_TITLE], cgs.media.qhFontMedium, -1, 1.0f);

	int mission_ycnt = 0;

	// Print all active objectives
	for (i = 0; i < MAX_OBJECTIVES; i++)
	{
		// Is there an objective to see?
		if (cent->gent->client->sess.mission_objectives[i].display)
		{
			// Calculate the Y position
			const int total_y = objectiveStartingYpos + i_y_pixels_per_line * mission_ycnt + i_y_pixels_per_line / 2;

			//	Draw graphics that show if mission has been accomplished or not
			cgi_R_SetColor(colorTable[CT_BLUE3]);
			CG_DrawPic(graphic_xpos, total_y - graphic_y_offset, graphic_size, graphic_size,
				cgs.media.messageObjCircle); // Circle in front
			if (cent->gent->client->sess.mission_objectives[i].status == OBJECTIVE_STAT_SUCCEEDED)
			{
				CG_DrawPic(graphic_xpos, total_y - graphic_y_offset, graphic_size, graphic_size,
					cgs.media.messageLitOn); // Center Dot
			}

			// Print current objective text
			ObjectivePrint_Line(CT_BLUE3, i, mission_ycnt);
		}
	}

	// No mission text?
	if (!mission_ycnt)
	{
		// Set the message a quarter of the way down and in the center of the text box
		constexpr int message_y_position = objectiveStartingYpos + objectiveTextBoxHeight / 4;

		cgi_SP_GetStringTextString("SP_INGAME_OBJNONE", text, sizeof text);
		const int message_x_position = objectiveStartingXpos + objectiveTextBoxWidth / 2 - cgi_R_Font_StrLenPixels(
			text, cgs.media.qhFontMedium, 1.0f) / 2;

		cgi_R_Font_DrawString(
			message_x_position,
			message_y_position,
			text,
			colorTable[CT_LTBLUE1],
			cgs.media.qhFontMedium,
			-1,
			1.0f);
	}
}

int SCREENTIP_NEXT_UPDATE_TIME = 0;

static void LoadTips(void)
{
	const int time = cgi_Milliseconds();

	// Determine whether tips are allowed to update
	qboolean allowUpdate = qfalse;

	if (cg_com_outcast.integer > 1)  // JKO version
	{
		if (cg.loadLCARSStage >= 3)
			allowUpdate = qtrue;
	}
	else
	{
		allowUpdate = qtrue;
	}

	if (allowUpdate &&
		(SCREENTIP_NEXT_UPDATE_TIME == 0 || SCREENTIP_NEXT_UPDATE_TIME < time))
	{
		static int lastIndex = -1;
		int index;

		// Prevent repeating the same tip twice
		do {
			index = rand() % 15;
		} while (index == lastIndex);

		lastIndex = index;

		cgi_Cvar_Set("ui_tipsbriefing", va("@LOADTIPS_TIP%d", index));
		SCREENTIP_NEXT_UPDATE_TIME = time + 3500;
	}
}

constexpr auto LOADBAR_CLIP_WIDTH = 256;
constexpr auto LOADBAR_CLIP_HEIGHT = 64;

void CG_LoadBar(void)
{
	constexpr int numticks = 9, tickwidth = 40, tickheight = 8;
	constexpr int tickpadx = 20, tickpady = 12;
	constexpr int capwidth = 8;
	constexpr int barwidth = numticks * tickwidth + tickpadx * 2 + capwidth * 2, barleft = (640 - barwidth) / 2;
	constexpr int barheight = tickheight + tickpady * 2, bartop = 475 - barheight;
	constexpr int capleft = barleft + tickpadx, tickleft = capleft + capwidth, ticktop = bartop + tickpady;

	cgi_R_SetColor(colorTable[CT_WHITE]);

	if (cg_com_outcast.integer > 1) //jko version
	{
		if (cg.loadLCARSStage >= 3)
		{
			// Draw background
			CG_DrawPic(barleft, bartop, barwidth, barheight, cgs.media.levelLoad);

			// Draw left cap (backwards)
			CG_DrawPic(tickleft, ticktop, -capwidth, tickheight, cgs.media.loadTickCap);

			// Draw bar
			CG_DrawPic(tickleft, ticktop, tickwidth * cg.loadLCARSStage, tickheight, cgs.media.loadTick);

			// Draw right cap
			CG_DrawPic(tickleft + tickwidth * cg.loadLCARSStage, ticktop, capwidth, tickheight, cgs.media.loadTickCap);
		}
	}
	else
	{
		// Draw background
		CG_DrawPic(barleft, bartop, barwidth, barheight, cgs.media.levelLoad);

		// Draw left cap (backwards)
		CG_DrawPic(tickleft, ticktop, -capwidth, tickheight, cgs.media.loadTickCap);

		// Draw bar
		CG_DrawPic(tickleft, ticktop, tickwidth * cg.loadLCARSStage, tickheight, cgs.media.loadTick);

		// Draw right cap
		CG_DrawPic(tickleft + tickwidth * cg.loadLCARSStage, ticktop, capwidth, tickheight, cgs.media.loadTickCap);
	}

	if (cg.loadLCARSStage >= 3)
	{
		if (cg.loadLCARSStage <= 6)
		{
			if (cg_com_rend2.integer == 1) //rend2 is on
			{
				cgi_R_Font_DrawString(10, 10, va("Warning: When using Quality mode, longer loading times can be expected."), colorTable[CT_WHITE], cgs.media.qhFontMedium, -1, 0.5f);
			}
		}
		constexpr int x = (640 - LOADBAR_CLIP_WIDTH) / 2;
		constexpr int y = 340;

		CG_DrawPic(x, y, LOADBAR_CLIP_WIDTH, LOADBAR_CLIP_HEIGHT, cgs.media.load_SerenitySaberSystems);
	}
}

int CG_WeaponCheck(int weapon_index);

// For printing load screen icons
constexpr int MAXLOADICONSPERROW = 8; // Max icons displayed per row
constexpr int MAXLOADWEAPONS = 16;
constexpr int MAXLOAD_FORCEICONSIZE = 40; // Size of force power icons
constexpr int MAXLOAD_FORCEICONPAD = 12; // Padding space between icons

static int CG_DrawLoadWeaponsPrintRow(const char* item_name, const int weapons_bits, const int row_icon_cnt,
	const int start_index)
{
	int end_index = 0, printed_icon_cnt = 0;
	int x, y;
	int width, height;
	vec4_t color;
	qhandle_t background;

	if (!cgi_UI_GetMenuItemInfo(
		"loadScreen",
		item_name,
		&x,
		&y,
		&width,
		&height,
		color,
		&background))
	{
		return 0;
	}

	cgi_R_SetColor(color);

	constexpr int icon_size = 60;
	constexpr int pad = 12;

	// calculate placement of weapon icons
	int hold_x = x + (width - (icon_size * row_icon_cnt + pad * (row_icon_cnt - 1))) / 2;

	for (int i = start_index; i < MAXLOADWEAPONS; i++)
	{
		if (!(weapons_bits & 1 << i)) // Does he have this weapon?
		{
			continue;
		}

		if (weaponData[i].weaponIcon[0])
		{
			constexpr int y_offset = 0;
			CG_RegisterWeapon(i);
			const weaponInfo_t* weapon_info = &cg_weapons[i];
			end_index = i;

			CG_DrawPic(hold_x, y + y_offset, icon_size, icon_size, weapon_info->weaponIcon);

			printed_icon_cnt++;
			if (printed_icon_cnt == MAXLOADICONSPERROW)
			{
				break;
			}

			hold_x += icon_size + pad;
		}
	}

	return end_index;
}

// Print weapons the player is carrying
// Two rows print if there are too many
static void CG_DrawLoadWeapons(const int weapon_bits)
{
	// count the number of weapons owned
	int icon_cnt = 0;
	for (int i = 1; i < MAXLOADWEAPONS; i++)
	{
		if (weapon_bits & 1 << i)
		{
			icon_cnt++;
		}
	}

	if (!icon_cnt) // If no weapons, don't display
	{
		return;
	}

	// Single line of icons
	if (icon_cnt <= MAXLOADICONSPERROW)
	{
		CG_DrawLoadWeaponsPrintRow("weaponicons_singlerow", weapon_bits, icon_cnt, 0);
	}
	// Two lines of icons
	else
	{
		// Print top row
		const int end_index = CG_DrawLoadWeaponsPrintRow("weaponicons_row1", weapon_bits, MAXLOADICONSPERROW, 0);

		// Print second row
		const int row_icon_cnt = icon_cnt - MAXLOADICONSPERROW;
		CG_DrawLoadWeaponsPrintRow("weaponicons_row2", weapon_bits, row_icon_cnt, end_index + 1);
	}

	cgi_R_SetColor(nullptr);
}

static int CG_DrawLoadForcePrintRow(const char* item_name, const int force_bits, const int row_icon_cnt,
	const int start_index)
{
	int end_index = 0, printedIconCnt = 0;
	int x, y;
	int width, height;
	vec4_t color;
	qhandle_t background;

	if (!cgi_UI_GetMenuItemInfo(
		"loadScreen",
		item_name,
		&x,
		&y,
		&width,
		&height,
		color,
		&background))
	{
		return 0;
	}

	cgi_R_SetColor(color);

	// calculate placement of weapon icons
	int hold_x = x + (width - (MAXLOAD_FORCEICONSIZE * row_icon_cnt + MAXLOAD_FORCEICONPAD * (row_icon_cnt - 1))) / 2;

	for (int i = start_index; i < MAX_SHOWPOWERS; i++)
	{
		if (!CG_ForcePower_Valid(force_bits, i)) // Does he have this power?
		{
			continue;
		}

		if (force_icons[showPowers[i]])
		{
			constexpr int yOffset = 0;
			end_index = i;

			CG_DrawPic(hold_x, y + yOffset, MAXLOAD_FORCEICONSIZE, MAXLOAD_FORCEICONSIZE, force_icons[showPowers[i]]);

			printedIconCnt++;
			if (printedIconCnt == MAXLOADICONSPERROW)
			{
				break;
			}

			hold_x += MAXLOAD_FORCEICONSIZE + MAXLOAD_FORCEICONPAD;
		}
	}

	return end_index;
}

int loadForcePowerLevel[NUM_FORCE_POWERS];

/*
===============
ForcePowerDataPad_Valid
===============
*/
qboolean CG_ForcePower_Valid(const int force_known_bits, const int index)
{
	if (force_known_bits & 1 << showPowers[index] &&
		loadForcePowerLevel[showPowers[index]]) // Does he have the force power?
	{
		return qtrue;
	}

	return qfalse;
}

// Print force powers the player is using
// Two rows print if there are too many
static void CG_DrawLoadForcePowers(const int force_bits)
{
	int icon_cnt = 0;

	// Count the number of force powers known
	for (int i = 0; i < MAX_SHOWPOWERS; ++i)
	{
		if (CG_ForcePower_Valid(force_bits, i))
		{
			icon_cnt++;
		}
	}

	if (!icon_cnt) // If no force powers, don't display
	{
		return;
	}

	// Single line of icons
	if (icon_cnt <= MAXLOADICONSPERROW)
	{
		CG_DrawLoadForcePrintRow("forceicons_singlerow", force_bits, icon_cnt, 0);
	}
	// Two lines of icons
	else
	{
		// Print top row
		const int end_index = CG_DrawLoadForcePrintRow("forceicons_row1", force_bits, MAXLOADICONSPERROW, 0);

		// Print second row
		const int row_icon_cnt = icon_cnt - MAXLOADICONSPERROW;
		CG_DrawLoadForcePrintRow("forceicons_row2", force_bits, row_icon_cnt, end_index + 1);
	}

	cgi_R_SetColor(nullptr);
}

// Get the player weapons and force power info
static void CG_GetLoadScreenInfo(int* weapon_bits, int* force_bits)
{
	char s[MAX_STRING_CHARS];
	int i_dummy;
	float f_dummy;

	gi.Cvar_VariableStringBuffer(sCVARNAME_PLAYERSAVE, s, sizeof s);

	// Get player weapons and force powers known
	if (s[0])
	{
		//				|general info				  |-force powers
		sscanf(s, "%i %i %i %i %i %i %i %f %f %f %i %i",
			&i_dummy, //	&client->ps.stats[STAT_HEALTH],
			&i_dummy, //	&client->ps.stats[STAT_ARMOR],
			&*weapon_bits, //	&client->ps.stats[STAT_WEAPONS],
			&i_dummy, //	&client->ps.stats[STAT_ITEMS],
			&i_dummy, //	&client->ps.weapon,
			&i_dummy, //	&client->ps.weaponstate,
			&i_dummy, //	&client->ps.batteryCharge,
			&f_dummy, //	&client->ps.viewangles[0],
			&f_dummy, //	&client->ps.viewangles[1],
			&f_dummy, //	&client->ps.viewangles[2],
			//force power data
			&*force_bits, //	&client->ps.forcePowersKnown,
			&i_dummy //	&client->ps.forcePower,

		);
	}

	// the new JK2 stuff - force powers, etc...
	//
	gi.Cvar_VariableStringBuffer("playerfplvl", s, sizeof s);
	int i = 0;
	const char* var = strtok(s, " ");
	while (var != nullptr)
	{
		/* While there are tokens in "s" */
		loadForcePowerLevel[i++] = atoi(var);
		/* Get next token: */
		var = strtok(nullptr, " ");
	}
}

/*
====================
CG_DrawLoadingScreen

Load screen displays the map pic, the mission briefing and weapons/force powers
====================
*/
static void CG_DrawLoadingScreen(const qhandle_t levelshot, const char* map_name)
{
	int xPos, yPos, width, height;
	vec4_t color;
	qhandle_t background;
	int weapons = 0, forcepowers = 0;

	if (cg.loadLCARSStage >= 4)
	{
		// Get mission briefing for load screen
		if (cgi_SP_GetStringTextString(va("BRIEFINGS_%s", map_name), nullptr, 0) == 0)
		{
			cgi_Cvar_Set("ui_missiontext", "@BRIEFINGS_NONE");
		}
		else
		{
			cgi_Cvar_Set("ui_missionbriefing", va("@BRIEFINGS_%s", map_name));
		}
	}
	else
	{
		//
	}

	// Print background
	if (cgi_UI_GetMenuItemInfo(
		"loadScreen",
		"background",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor(color);
		CG_DrawPic(xPos, yPos, width, height, background);
	}

	// Print level pic
	if (cgi_UI_GetMenuItemInfo(
		"loadScreen",
		"mappic",
		&xPos,
		&yPos,
		&width,
		&height,
		color,
		&background))
	{
		cgi_R_SetColor(color);
		CG_DrawPic(xPos, yPos, width, height, levelshot);
	}

	// Get player weapons and force power info
	CG_GetLoadScreenInfo(&weapons, &forcepowers);

	// Print weapon icons
	if (weapons)
	{
		CG_DrawLoadWeapons(weapons);
	}

	// Print force power icons
	if (forcepowers)
	{
		CG_DrawLoadForcePowers(forcepowers);
	}
}

/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
int SCREENSHOT_TOTAL = -1;
int SCREENSHOT_CHOICE = 0;
int SCREENSHOT_NEXT_UPDATE_TIME = 0;
char SCREENSHOT_CURRENT[64] = { 0 };

static char* cg_GetCurrentLevelshot1(const char* s)
{
	const qhandle_t levelshot1 = cgi_R_RegisterShaderNoMip(va("levelshots/%s", s));
	const int time = cgi_Milliseconds();

	if (levelshot1 && SCREENSHOT_NEXT_UPDATE_TIME == 0)
	{
		SCREENSHOT_NEXT_UPDATE_TIME = time + 2500;
		memset(SCREENSHOT_CURRENT, 0, sizeof SCREENSHOT_CURRENT);
		strcpy(SCREENSHOT_CURRENT, va("levelshots/%s2", s));
		return SCREENSHOT_CURRENT;
	}

	if (SCREENSHOT_NEXT_UPDATE_TIME < time || SCREENSHOT_NEXT_UPDATE_TIME == 0)
	{
		if (SCREENSHOT_TOTAL < 0)
		{
			// Count and register them...
			SCREENSHOT_TOTAL = 0;

			while (true)
			{
				char screen_shot[128] = { 0 };

				strcpy(screen_shot, va("menu/art/unknownmap_mp%i", SCREENSHOT_TOTAL));

				if (!cgi_R_RegisterShaderNoMip(screen_shot))
				{
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
	const qhandle_t levelshot2 = cgi_R_RegisterShaderNoMip(va("levelshots/%s2", s));
	const int time = cgi_Milliseconds();

	if (levelshot2 && SCREENSHOT_NEXT_UPDATE_TIME == 0)
	{
		SCREENSHOT_NEXT_UPDATE_TIME = time + 2500;
		memset(SCREENSHOT_CURRENT, 0, sizeof SCREENSHOT_CURRENT);
		strcpy(SCREENSHOT_CURRENT, va("levelshots/%s2", s));
		return SCREENSHOT_CURRENT;
	}

	if (SCREENSHOT_NEXT_UPDATE_TIME < time || SCREENSHOT_NEXT_UPDATE_TIME == 0)
	{
		if (SCREENSHOT_TOTAL < 0)
		{
			// Count and register them...
			SCREENSHOT_TOTAL = 0;

			while (true)
			{
				char screen_shot[128] = { 0 };

				strcpy(screen_shot, va("menu/art/unknownmap_mp%i", SCREENSHOT_TOTAL));

				if (!cgi_R_RegisterShaderNoMip(screen_shot))
				{
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

void CG_DrawInformation(void)
{
	// draw the dialog background
	const char* info = CG_ConfigString(CS_SERVERINFO);
	const char* s = Info_ValueForKey(info, "mapname");

	extern SavedGameJustLoaded_e g_eSavedGameJustLoaded; // hack! (hey, it's the last week of coding, ok?

	qhandle_t levelshot = cgi_R_RegisterShaderNoMip(va("levelshots/%s", s));
	qhandle_t levelshot2 = cgi_R_RegisterShaderNoMip(va("levelshots/%s2", s));
	qhandle_t Loadshot = cgi_R_RegisterShaderNoMip("menu/art/loadshot");
	qhandle_t Loadshot2 = cgi_R_RegisterShaderNoMip("menu/art/loadshot2");

	if (!levelshot)
	{
		levelshot = cgi_R_RegisterShaderNoMip(cg_GetCurrentLevelshot1(s));
	}
	if (!levelshot2)
	{
		levelshot2 = cgi_R_RegisterShaderNoMip(cg_GetCurrentLevelshot2(s));
	}
	if (!Loadshot)
	{
		Loadshot = cgi_R_RegisterShaderNoMip(cg_GetCurrentLevelshot1(s));
	}
	if (!Loadshot2)
	{
		Loadshot2 = cgi_R_RegisterShaderNoMip(cg_GetCurrentLevelshot1(s));
	}

	if (g_eSavedGameJustLoaded != eFULL
		&& (strcmp(s, "yavin1") == 0
			|| strcmp(s, "demo") == 0
			|| strcmp(s, "jodemo") == 0
			|| strcmp(s, "01nar") == 0
			|| strcmp(s, "md2_bd_ch") == 0
			|| strcmp(s, "md_sn_intro_jedi") == 0
			|| strcmp(s, "md_ch_battledroids") == 0
			|| strcmp(s, "md_ep4_intro") == 0
			|| strcmp(s, "secbase") == 0
			|| strcmp(s, "level0") == 0
			|| strcmp(s, "part_1") == 0
			|| strcmp(s, "kejim_post") == 0)) //special case for first map!
	{
		constexpr char text[1024] = { 0 };

		if (cg.loadLCARSStage <= 3 && cg_com_rend2.integer == 1)
		{
			CG_DrawPic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Loadshot2);
		}
		else
		{
			CG_DrawPic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Loadshot);

			const int w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontMedium, 1.0f);
			cgi_R_Font_DrawString(320 - w / 2, 140, text, colorTable[CT_ICON_BLUE], cgs.media.qhFontMedium, -1, 1.0f);
		}
	}
	else
	{
		if (cg.loadLCARSStage >= 4)
		{
			CG_DrawLoadingScreen(levelshot2, s);

			if (cg_com_outcast.integer == 1)
			{
				CG_MissionCompletion();
			}
		}
		else
		{
			CG_DrawLoadingScreen(levelshot, s);
		}
		cgi_UI_MenuPaintAll();
		CG_LoadBar();
		LoadTips();
	}

	// draw info string information

	// map-specific message (long map name)
	s = CG_ConfigString(CS_MESSAGE);

	if (s[0])
	{
		int y = 20;
		if (s[0] == '@')
		{
			char text[1024] = { 0 };
			cgi_SP_GetStringTextString(s + 1, text, sizeof text);
			cgi_R_Font_DrawString(15, y, va("\"%s\"", text), colorTable[CT_WHITE], cgs.media.qhFontMedium, -1, 1.0f);
		}
		else
		{
			cgi_R_Font_DrawString(15, y, va("\"%s\"", s), colorTable[CT_WHITE], cgs.media.qhFontMedium, -1, 1.0f);
		}
		y += 20;
	}
}