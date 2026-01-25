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

// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#include "game/bg_saga.h"

#include "ui/ui_shared.h"
#include "ui/ui_public.h"

extern float CG_RadiusForCent(const centity_t* cent);
qboolean CG_WorldCoordToScreenCoordFloat(vec3_t world_coord, float* x, float* y);
qboolean CG_CalcmuzzlePoint(int entityNum, vec3_t muzzle);
static void CG_DrawSiegeDeathTimer(int timeRemaining);
void CG_DrawDuelistHealth(float x, float y, float w, float h, int duelist);
extern qboolean PM_DeathCinAnim(int anim);

// used for scoreboard
extern displayContextDef_t cgDC;

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int numSortedTeamPlayers;

int lastvalidlockdif;

extern float zoomFov; //this has to be global client-side

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

// The time at which you died and the time it will take for you to rejoin game.
int cg_siegeDeathTime = 0;

#define MAX_HUD_TICS 4
#define MAX_SJEHUD_TICS 15

const char* armorTicName[MAX_HUD_TICS] =
{
	"armor_tic1MP",
	"armor_tic2MP",
	"armor_tic3MP",
	"armor_tic4MP",
};

const char* healthTicName[MAX_HUD_TICS] =
{
	"health_tic1MP",
	"health_tic2MP",
	"health_tic3MP",
	"health_tic4MP",
};

const char* forceTicName[MAX_HUD_TICS] =
{
	"force_tic1MP",
	"force_tic2MP",
	"force_tic3MP",
	"force_tic4MP",
};

forceTicPos_t JK2forceTicPos[] =
{
	{11, 41, 20, 10, "gfx/hud/JK2force_tick1", NULL_HANDLE}, // Left Top
	{12, 45, 20, 10, "gfx/hud/JK2force_tick2", NULL_HANDLE},
	{14, 49, 20, 10, "gfx/hud/JK2force_tick3", NULL_HANDLE},
	{17, 52, 20, 10, "gfx/hud/JK2force_tick4", NULL_HANDLE},
	{22, 55, 10, 10, "gfx/hud/JK2force_tick5", NULL_HANDLE},
	{28, 57, 10, 20, "gfx/hud/JK2force_tick6", NULL_HANDLE},
	{34, 59, 10, 10, "gfx/hud/JK2force_tick7", NULL_HANDLE}, // Left bottom

	{46, 59, -10, 10, "gfx/hud/JK2force_tick7", NULL_HANDLE}, // Right bottom
	{52, 57, -10, 20, "gfx/hud/JK2force_tick6", NULL_HANDLE},
	{58, 55, -10, 10, "gfx/hud/JK2force_tick5", NULL_HANDLE},
	{63, 52, -20, 10, "gfx/hud/JK2force_tick4", NULL_HANDLE},
	{66, 49, -20, 10, "gfx/hud/JK2force_tick3", NULL_HANDLE},
	{68, 45, -20, 10, "gfx/hud/JK2force_tick2", NULL_HANDLE},
	{69, 41, -20, 10, "gfx/hud/JK2force_tick1", NULL_HANDLE}, // Right top
};

const char* ammoTicName[MAX_HUD_TICS] =
{
	"ammo_tic1MP",
	"ammo_tic2MP",
	"ammo_tic3MP",
	"ammo_tic4MP",
};

forceTicPos_t JK2ammoTicPos[] =
{
	{12, 34, 10, 10, "gfx/hud/JK2ammo_tick7-l", NULL_HANDLE}, // Bottom
	{13, 28, 10, 10, "gfx/hud/JK2ammo_tick6-l", NULL_HANDLE},
	{15, 23, 10, 10, "gfx/hud/JK2ammo_tick5-l", NULL_HANDLE},
	{19, 19, 10, 10, "gfx/hud/JK2ammo_tick4-l", NULL_HANDLE},
	{23, 15, 10, 10, "gfx/hud/JK2ammo_tick3-l", NULL_HANDLE},
	{29, 12, 10, 10, "gfx/hud/JK2ammo_tick2-l", NULL_HANDLE},
	{34, 11, 10, 10, "gfx/hud/JK2ammo_tick1-l", NULL_HANDLE},

	{47, 11, -10, 10, "gfx/hud/JK2ammo_tick1-r", NULL_HANDLE},
	{52, 12, -10, 10, "gfx/hud/JK2ammo_tick2-r", NULL_HANDLE},
	{58, 15, -10, 10, "gfx/hud/JK2ammo_tick3-r", NULL_HANDLE},
	{62, 19, -10, 10, "gfx/hud/JK2ammo_tick4-r", NULL_HANDLE},
	{66, 23, -10, 10, "gfx/hud/JK2ammo_tick5-r", NULL_HANDLE},
	{68, 28, -10, 10, "gfx/hud/JK2ammo_tick6-r", NULL_HANDLE},
	{69, 34, -10, 10, "gfx/hud/JK2ammo_tick7-r", NULL_HANDLE},
};

const char* mishapTics[MAX_HUD_TICS] =
{
	"mishap_tic1",
	"mishap_tic2",
	"mishap_tic3",
	"mishap_tic4",
};

const char* sabfatticS[MAX_SJEHUD_TICS] =
{
	"sabfat_tic1",
	"sabfat_tic2",
	"sabfat_tic3",
	"sabfat_tic4",
	"sabfat_tic5",
	"sabfat_tic6",
	"sabfat_tic7",
	"sabfat_tic8",
	"sabfat_tic9",
	"sabfat_tic10",
	"sabfat_tic11",
	"sabfat_tic12",
	"sabfat_tic13",
	"sabfat_tic14",
	"sabfat_tic15",
};

char* showPowersName[] =
{
	"HEAL2", //FP_HEAL
	"JUMP2", //FP_LEVITATION
	"SPEED2", //FP_SPEED
	"PUSH2", //FP_PUSH
	"PULL2", //FP_PULL
	"MINDTRICK2", //FP_TELEPTAHY
	"GRIP2", //FP_GRIP
	"LIGHTNING2", //FP_LIGHTNING
	"DARK_RAGE2", //FP_RAGE
	"PROTECT2", //FP_PROTECT
	"ABSORB2", //FP_ABSORB
	"TEAM_HEAL2", //FP_TEAM_HEAL
	"TEAM_REPLENISH2", //FP_TEAM_FORCE
	"DRAIN2", //FP_DRAIN
	"SEEING2", //FP_SEE
	"SABER_OFFENSE2", //FP_SABER_OFFENSE
	"SABER_DEFENSE2", //FP_SABER_DEFENSE
	"SABER_THROW2", //FP_SABERTHROW
	NULL
};

//Called from UI shared code. For now we'll just redirect to the normal anim load function.

int UI_ParseAnimationFile(const char* filename, animation_t* animset, const qboolean is_humanoid)
{
	return bg_parse_animation_file(filename, animset, is_humanoid);
}

int MenuFontToHandle(const int i_menu_font)
{
	switch (i_menu_font)
	{
	case FONT_SMALL: return cgDC.Assets.qhSmallFont;
	case FONT_SMALL2: return cgDC.Assets.qhSmall2Font;
	case FONT_MEDIUM: return cgDC.Assets.qhMediumFont;
	case FONT_LARGE: return cgDC.Assets.qhMediumFont; //cgDC.Assets.qhBigFont;
	default:;
		//fixme? Big fonr isn't registered...?
	}

	return cgDC.Assets.qhMediumFont;
}

int CG_Text_Width(const char* text, const float scale, const int i_menu_font)
{
	const int i_font_index = MenuFontToHandle(i_menu_font);

	return trap->R_Font_StrLenPixels(text, i_font_index, scale);
}

int CG_Text_Height(const char* text, const float scale, const int i_menu_font)
{
	const int i_font_index = MenuFontToHandle(i_menu_font);

	return trap->R_Font_HeightPixels(i_font_index, scale);
}

#include "qcommon/qfiles.h"	// for STYLE_BLINK etc

void CG_Text_Paint(const float x, const float y, const float scale, vec4_t color, const char* text, float adjust,
	const int limit, const int style,
	const int i_menu_font)
{
	int i_style_or = 0;
	const int i_font_index = MenuFontToHandle(i_menu_font);

	switch (style)
	{
	case ITEM_TEXTSTYLE_NORMAL: i_style_or = 0;
		break; // JK2 normal text
	case ITEM_TEXTSTYLE_BLINK: i_style_or = STYLE_BLINK;
		break; // JK2 fast blinking
	case ITEM_TEXTSTYLE_PULSE: i_style_or = STYLE_BLINK;
		break; // JK2 slow pulsing
	case ITEM_TEXTSTYLE_SHADOWED: i_style_or = (int)STYLE_DROPSHADOW;
		break; // JK2 drop shadow ( need a color for this )
	case ITEM_TEXTSTYLE_OUTLINED: i_style_or = (int)STYLE_DROPSHADOW;
		break; // JK2 drop shadow ( need a color for this )
	case ITEM_TEXTSTYLE_OUTLINESHADOWED: i_style_or = (int)STYLE_DROPSHADOW;
		break; // JK2 drop shadow ( need a color for this )
	case ITEM_TEXTSTYLE_SHADOWEDMORE: i_style_or = (int)STYLE_DROPSHADOW;
		break; // JK2 drop shadow ( need a color for this )
	default:;
	}

	trap->R_Font_DrawString(x, // int ox
		y, // int oy
		text, // const char *text
		color, // paletteRGBA_c c
		i_style_or | i_font_index, // const int iFontHandle
		!limit ? -1 : limit, // iCharLimit (-1 = none)
		scale // const float scale = 1.0f
	);
}

/*
================
CG_DrawZoomMask

================
*/
static void CG_DrawZoomMask(void)
{
	vec4_t color1;
	float level;
	static qboolean flip = qtrue;

	//	int val[5];

	// Check for Binocular specific zooming since we'll want to render different bits in each case
	if (cg.predictedPlayerState.zoomMode == 2)
	{
		// zoom level
		level = (80.0f - cg.predictedPlayerState.zoomFov) / 80.0f;

		// ...so we'll clamp it
		if (level < 0.0f)
		{
			level = 0.0f;
		}
		else if (level > 1.0f)
		{
			level = 1.0f;
		}

		// Using a magic number to convert the zoom level to scale amount
		level *= 162.0f;

		// draw blue tinted distortion mask, trying to make it as small as is necessary to fill in the viewable area
		trap->R_SetColor(colorTable[CT_WHITE]);
		CG_DrawPic(34, 48, 570, 362, cgs.media.binocularStatic);

		// Black out the area behind the numbers
		trap->R_SetColor(colorTable[CT_BLACK]);
		CG_DrawPic(212, 367, 200, 40, cgs.media.whiteShader);

		// Numbers should be kind of greenish
		color1[0] = 0.2f;
		color1[1] = 0.4f;
		color1[2] = 0.2f;
		color1[3] = 0.3f;
		trap->R_SetColor(color1);

		// Draw scrolling numbers, use intervals 10 units apart--sorry, this section of code is just kind of hacked
		//	up with a bunch of magic numbers.....
		int val = (int)((cg.refdef.viewangles[YAW] + 180) / 10) * 10;
		const float off = cg.refdef.viewangles[YAW] + 180 - val;

		for (int i = -10; i < 30; i += 10)
		{
			val -= 10;

			if (val < 0)
			{
				val += 360;
			}

			// we only want to draw the very far left one some of the time, if it's too far to the left it will
			//	poke outside the mask.
			if (off > 3.0f && i == -10 || i > -10)
			{
				// draw the value, but add 200 just to bump the range up...arbitrary, so change it if you like
				CG_DrawNumField(155 + i * 10 + off * 10, 374, 3, val + 200, 24, 14, NUM_FONT_CHUNKY, qtrue);
				CG_DrawPic(245 + (i - 1) * 10 + off * 10, 376, 6, 6, cgs.media.whiteShader);
			}
		}

		CG_DrawPic(212, 367, 200, 28, cgs.media.binocularOverlay);

		color1[0] = sin(cg.time * 0.01f) * 0.5f + 0.5f;
		color1[0] = color1[0] * color1[0];
		color1[1] = color1[0];
		color1[2] = color1[0];
		color1[3] = 1.0f;

		trap->R_SetColor(color1);

		CG_DrawPic(82, 94, 16, 16, cgs.media.binocularCircle);

		// Flickery color
		color1[0] = 0.7f + Q_flrand(-0.1f, 0.1f);
		color1[1] = 0.8f + Q_flrand(-0.1f, 0.1f);
		color1[2] = 0.7f + Q_flrand(-0.1f, 0.1f);
		color1[3] = 1.0f;
		trap->R_SetColor(color1);

		CG_DrawPic(0, 0, 640, 480, cgs.media.binocularMask);

		CG_DrawPic(4, 282 - level, 16, 16, cgs.media.binocularArrow);

		// The top triangle bit randomly flips
		if (flip)
		{
			CG_DrawPic(330, 60, -26, -30, cgs.media.binocularTri);
		}
		else
		{
			CG_DrawPic(307, 40, 26, 30, cgs.media.binocularTri);
		}

		if (Q_flrand(0.0f, 1.0f) > 0.98f && cg.time & 1024)
		{
			flip = !flip;
		}
	}
	else if (cg.predictedPlayerState.zoomMode)
	{
		float max;
		// disruptor zoom mode
		level = (50.0f - zoomFov) / 50.0f; //(float)(80.0f - zoomFov) / 80.0f;

		// ...so we'll clamp it
		if (level < 0.0f)
		{
			level = 0.0f;
		}
		else if (level > 1.0f)
		{
			level = 1.0f;
		}

		// Using a magic number to convert the zoom level to a rotation amount that correlates more or less with the zoom artwork.
		level *= 103.0f;

		// Draw target mask
		trap->R_SetColor(colorTable[CT_WHITE]);
		CG_DrawPic(0, 0, 640, 480, cgs.media.disruptorMask);

		// apparently 99.0f is the full zoom level
		if (level >= 99)
		{
			// Fully zoomed, so make the rotating insert pulse
			color1[0] = 1.0f;
			color1[1] = 1.0f;
			color1[2] = 1.0f;
			color1[3] = 0.7f + sin(cg.time * 0.01f) * 0.3f;

			trap->R_SetColor(color1);
		}

		// Draw rotating insert
		CG_DrawRotatePic2(320, 240, 640, 480, -level, cgs.media.disruptorInsert);

		if (cg.snap->ps.eFlags & EF_DOUBLE_AMMO)
		{
			max = cg.snap->ps.ammo[weaponData[WP_DISRUPTOR].ammoIndex] / ((float)ammoData[weaponData[WP_DISRUPTOR].
				ammoIndex].max * 2.0f);
		}
		else
		{
			max = cg.snap->ps.ammo[weaponData[WP_DISRUPTOR].ammoIndex] / (float)ammoData[weaponData[WP_DISRUPTOR].
				ammoIndex].max;
		}
		if (max > 1.0f)
		{
			max = 1.0f;
		}

		color1[0] = (1.0f - max) * 2.0f;
		color1[1] = max * 1.5f;
		color1[2] = 0.0f;
		color1[3] = 1.0f;

		// If we are low on health, make us flash
		if (max < 0.15f && cg.time & 512)
		{
			VectorClear(color1);
		}

		if (color1[0] > 1.0f)
		{
			color1[0] = 1.0f;
		}

		if (color1[1] > 1.0f)
		{
			color1[1] = 1.0f;
		}

		trap->R_SetColor(color1);

		max *= 58.0f;

		for (float fi = 18.5f; fi <= 18.5f + max; fi += 3) // going from 15 to 45 degrees, with 5 degree increments
		{
			const float cx = 320 + sin((fi + 90.0f) / 57.296f) * 190;
			const float cy = 240 + cos((fi + 90.0f) / 57.296f) * 190;

			CG_DrawRotatePic2(cx, cy, 12, 24, 90 - fi, cgs.media.disruptorInsertTick);
		}

		if (cg.predictedPlayerState.weaponstate == WEAPON_CHARGING_ALT)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			// draw the charge level
			max = (cg.time - cg.predictedPlayerState.weaponChargeTime) / (50.0f * 30.0f);
			// bad hardcodedness 50 is disruptor charge unit and 30 is max charge units allowed.

			if (max > 1.0f)
			{
				max = 1.0f;
			}

			trap->R_DrawStretchPic(257, 435, 134 * max, 34, 0, 0, max, 1, cgs.media.disruptorChargeShader);
		}
	}
}

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel(const float x, const float y, const float w, const float h, const qhandle_t model, void* ghoul2,
	const int g2_radius, const qhandle_t skin,
	vec3_t origin, vec3_t angles)
{
	refdef_t refdef;
	refEntity_t ent;

	if (!cg_draw3DIcons.integer || !cg_drawIcons.integer)
	{
		return;
	}

	memset(&refdef, 0, sizeof refdef);

	memset(&ent, 0, sizeof ent);
	AnglesToAxis(angles, ent.axis);
	VectorCopy(origin, ent.origin);
	ent.hModel = model;
	ent.ghoul2 = ghoul2;
	ent.radius = g2_radius;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW; // no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear(refdef.viewaxis);

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap->R_ClearScene();
	trap->R_AddRefEntityToScene(&ent);
	trap->R_RenderScene(&refdef);
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead(const float x, const float y, const float w, const float h, const int clientNum)
{
	if (clientNum >= MAX_CLIENTS)
	{
		//npc?
		return;
	}

	const clientInfo_t* ci = &cgs.clientinfo[clientNum];

	CG_DrawPic(x, y, w, h, ci->modelIcon);

	// if they are deferred, draw a cross out
	if (ci->deferred)
	{
		CG_DrawPic(x, y, w, h, cgs.media.deferShader);
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel(float x, float y, float w, float h, const int team, const qboolean force_2d)
{
	if (!force_2d && cg_draw3DIcons.integer)
	{
		qhandle_t handle;
		vec3_t maxs;
		vec3_t mins;
		vec3_t angles;
		vec3_t origin;
		x *= cgs.screenXScale;
		y *= cgs.screenYScale;
		w *= cgs.screenXScale;
		h *= cgs.screenYScale;

		VectorClear(angles);

		const qhandle_t cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap->R_ModelBounds(cm, mins, maxs);

		origin[2] = -0.5f * (mins[2] + maxs[2]);
		origin[1] = 0.5f * (mins[1] + maxs[1]);

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		const float len = 0.5f * (maxs[2] - mins[2]);
		origin[0] = len / 0.268; // len / tan( fov/2 )

		angles[YAW] = 60 * sin(cg.time / 2000.0);

		if (team == TEAM_RED)
		{
			handle = cgs.media.redFlagModel;
		}
		else if (team == TEAM_BLUE)
		{
			handle = cgs.media.blueFlagModel;
		}
		else if (team == TEAM_FREE)
		{
			handle = 0; //cgs.media.neutralFlagModel;
		}
		else
		{
			return;
		}
		CG_Draw3DModel(x, y, w, h, handle, NULL, 0, 0, origin, angles);
	}
	else if (cg_drawIcons.integer)
	{
		gitem_t* item;

		if (team == TEAM_RED)
		{
			item = BG_FindItemForPowerup(PW_REDFLAG);
		}
		else if (team == TEAM_BLUE)
		{
			item = BG_FindItemForPowerup(PW_BLUEFLAG);
		}
		else if (team == TEAM_FREE)
		{
			item = BG_FindItemForPowerup(PW_NEUTRALFLAG);
		}
		else
		{
			return;
		}
		if (item)
		{
			CG_DrawPic(x, y, w, h, cg_items[ITEM_INDEX(item)].icon);
		}
	}
}

/*
================
CG_DrawHealth
================
*/

static void CG_DrawCusHealth(const menuDef_t* menu_hud)
{
	vec4_t calc_color;
	itemDef_t* focus_item;

	// Can we find the menu?
	if (!menu_hud)
	{
		return;
	}

	const playerState_t* ps = &cg.snap->ps;

	// What's the health?
	int health_amt = ps->stats[STAT_HEALTH];
	if (health_amt > ps->stats[STAT_MAX_HEALTH])
	{
		health_amt = ps->stats[STAT_MAX_HEALTH];
	}

	const int inc = (float)ps->stats[STAT_MAX_HEALTH] / MAX_HUD_TICS;
	int curr_value = health_amt;

	// Print the health tics, fading out the one which is partial health
	for (int i = MAX_HUD_TICS - 1; i >= 0; i--)
	{
		focus_item = Menu_FindItemByName(menu_hud, healthTicName[i]);

		if (!focus_item) // This is bad
		{
			continue;
		}

		memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));

		if (curr_value <= 0) // don't show tic
		{
			break;
		}
		if (curr_value < inc) // partial tic (alpha it out)
		{
			const float percent = (float)curr_value / inc;
			calc_color[3] *= percent; // Fade it out
		}

		trap->R_SetColor(calc_color);

		CG_DrawPic(
			focus_item->window.rect.x - 3.4,
			focus_item->window.rect.y + 3.5,
			focus_item->window.rect.w,
			focus_item->window.rect.h,
			focus_item->window.background
		);

		curr_value -= inc;
	}

	if (cg.snap->ps.weapon == WP_SABER)
	{
		// Print the mueric amount
		focus_item = Menu_FindItemByName(menu_hud, "healthamount");

		if (focus_item)
		{
			// Print health amount
			trap->R_SetColor(focus_item->window.foreColor);

			CG_DrawNumField(
				focus_item->window.rect.x - 3.4,
				focus_item->window.rect.y + 3.5,
				3,
				ps->stats[STAT_HEALTH],
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				NUM_FONT_SMALL,
				qfalse);
		}
	}
	else
	{
		// Print the mueric amount
		focus_item = Menu_FindItemByName(menu_hud, "healthamountmp");

		if (focus_item)
		{
			// Print health amount
			trap->R_SetColor(focus_item->window.foreColor);

			CG_DrawNumField(
				focus_item->window.rect.x - 3.4,
				focus_item->window.rect.y + 1,
				3,
				ps->stats[STAT_HEALTH],
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				NUM_FONT_SMALL,
				qfalse);
		}
	}
}

static void CG_DrawJK2HealthSJE(const int x, const int y)
{
	vec4_t calc_color;

	const playerState_t* ps = &cg.snap->ps;

	memcpy(calc_color, colorTable[CT_HUD_RED], sizeof(vec4_t));
	const float health_percent = (float)ps->stats[STAT_HEALTH] / ps->stats[STAT_MAX_HEALTH];
	calc_color[0] *= health_percent;
	calc_color[1] *= health_percent;
	calc_color[2] *= health_percent;
	trap->R_SetColor(calc_color);
	CG_DrawPic(x, y, 40, 40, cgs.media.JK2HUDHealth);

	// Draw the ticks
	if (cg.HUDHealthFlag)
	{
		trap->R_SetColor(colorTable[CT_HUD_RED]);
		CG_DrawPic(x, y, 40, 40, cgs.media.JK2HUDHealthTic);
	}

	trap->R_SetColor(colorTable[CT_HUD_RED]);

	CG_DrawNumField(x + 2, y + 26, 3, ps->stats[STAT_HEALTH], 4, 7, NUM_FONT_SMALL, qtrue);
}

static void CG_DrawJK2blockPoints(const int x, const int y, const menuDef_t* menu_hud)
{
	vec4_t calc_color;

	//	Outer block circular
	//==========================================================================================================//

	if (cg.predictedPlayerState.ManualBlockingFlags & 1 << PERFECTBLOCKING)
	{
		memcpy(calc_color, colorTable[CT_GREEN], sizeof(vec4_t));
	}
	else
	{
		memcpy(calc_color, colorTable[CT_MAGENTA], sizeof(vec4_t));
	}

	const float hold = cg.snap->ps.fd.blockPoints - BLOCK_POINTS_MAX / 2;
	float block_percent = (float)hold / (BLOCK_POINTS_MAX / 2);

	// Make the hud flash by setting forceHUDTotalFlashTime above cg.time
	if (cg.blockHUDTotalFlashTime > cg.time || cg.snap->ps.fd.blockPoints < BLOCKPOINTS_WARNING)
	{
		if (cg.blockHUDNextFlashTime < cg.time)
		{
			cg.blockHUDNextFlashTime = cg.time + 400;
			trap->S_StartSound(NULL, 0, CHAN_LOCAL, cgs.media.overload);

			if (cg.blockHUDActive)
			{
				cg.blockHUDActive = qfalse;
			}
			else
			{
				cg.blockHUDActive = qtrue;
			}
		}
	}
	else // turn HUD back on if it had just finished flashing time.
	{
		cg.blockHUDNextFlashTime = 0;
		cg.blockHUDActive = qtrue;
	}

	if (block_percent < 0)
	{
		block_percent = 0;
	}

	calc_color[0] *= block_percent;
	calc_color[1] *= block_percent;
	calc_color[2] *= block_percent;

	trap->R_SetColor(calc_color);

	if (cg.predictedPlayerState.ManualBlockingFlags & 1 << PERFECTBLOCKING)
	{
		CG_DrawPic(x, y, 35, 35, cgs.media.HUDblockpointMB1);
	}
	else
	{
		CG_DrawPic(x, y, 35, 35, cgs.media.HUDblockpoint1);
	}

	// Inner block circular
	//==========================================================================================================//
	if (block_percent > 0)
	{
		block_percent = 1;
	}
	else
	{
		block_percent = (float)cg.snap->ps.fd.blockPoints / (BLOCK_POINTS_MAX / 2);
	}

	if (cg.predictedPlayerState.ManualBlockingFlags & 1 << PERFECTBLOCKING)
	{
		memcpy(calc_color, colorTable[CT_GREEN], sizeof(vec4_t));
	}
	else
	{
		memcpy(calc_color, colorTable[CT_MAGENTA], sizeof(vec4_t));
	}

	calc_color[0] *= block_percent;
	calc_color[1] *= block_percent;
	calc_color[2] *= block_percent;

	trap->R_SetColor(calc_color);

	if (cg.predictedPlayerState.ManualBlockingFlags & 1 << PERFECTBLOCKING)
	{
		CG_DrawPic(x, y, 35, 35, cgs.media.HUDblockpointMB2);
	}
	else
	{
		CG_DrawPic(x, y, 35, 35, cgs.media.HUDblockpoint2);
	}

	const itemDef_t* focus_item = Menu_FindItemByName(menu_hud, "blockpointamount");

	if (focus_item)
	{
		// Print blockpointamount amount
		trap->R_SetColor(focus_item->window.foreColor);

		CG_DrawNumField(
			focus_item->window.rect.x - 21,
			focus_item->window.rect.y + 5,
			3,
			cg.snap->ps.fd.blockPoints,
			focus_item->window.rect.w,
			focus_item->window.rect.h,
			NUM_FONT_SMALL,
			qfalse);
	}
}

static void CG_DrawJK2Health(const int x, const int y)
{
	vec4_t calc_color;

	const playerState_t* ps = &cg.snap->ps;

	memcpy(calc_color, colorTable[CT_HUD_RED], sizeof(vec4_t));
	const float health_percent = (float)ps->stats[STAT_HEALTH] / ps->stats[STAT_MAX_HEALTH];
	calc_color[0] *= health_percent;
	calc_color[1] *= health_percent;
	calc_color[2] *= health_percent;
	trap->R_SetColor(calc_color);
	CG_DrawPic(x, y, 80, 80, cgs.media.JK2HUDHealth);

	// Draw the ticks
	if (cg.HUDHealthFlag)
	{
		trap->R_SetColor(colorTable[CT_HUD_RED]);
		CG_DrawPic(x, y, 80, 80, cgs.media.JK2HUDHealthTic);
	}

	trap->R_SetColor(colorTable[CT_HUD_RED]);
	CG_DrawNumField(x + 16, y + 40, 3, ps->stats[STAT_HEALTH], 6, 12, NUM_FONT_SMALL, qfalse);
}

/*
================
CG_DrawArmor
================
*/

static void CG_Draw_JKA_Armor(const menuDef_t* menu_hud)
{
	vec4_t calc_color;
	itemDef_t* focus_item;

	//ps = &cg.snap->ps;
	const playerState_t* ps = &cg.predictedPlayerState;

	// Can we find the menu?
	if (!menu_hud)
	{
		return;
	}

	const int max_armor = ps->stats[STAT_MAX_HEALTH];

	int curr_value = ps->stats[STAT_ARMOR];
	const int inc = (float)max_armor / MAX_HUD_TICS;

	memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
	for (int i = MAX_HUD_TICS - 1; i >= 0; i--)
	{
		focus_item = Menu_FindItemByName(menu_hud, armorTicName[i]);

		if (!focus_item) // This is bad
		{
			continue;
		}

		memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));

		if (curr_value <= 0) // don't show tic
		{
			break;
		}
		if (curr_value < inc) // partial tic (alpha it out)
		{
			const float percent = (float)curr_value / inc;
			calc_color[3] *= percent;
		}

		trap->R_SetColor(calc_color);

		if (i == MAX_HUD_TICS - 1 && curr_value < inc)
		{
			if (cg.HUDArmorFlag)
			{
				CG_DrawPic(
					focus_item->window.rect.x - 3.4,
					focus_item->window.rect.y + 3.5,
					focus_item->window.rect.w,
					focus_item->window.rect.h,
					focus_item->window.background
				);
			}
		}
		else
		{
			CG_DrawPic(
				focus_item->window.rect.x - 3.4,
				focus_item->window.rect.y + 3.5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}

		curr_value -= inc;
	}

	if (cg.snap->ps.weapon == WP_SABER)
	{
		focus_item = Menu_FindItemByName(menu_hud, "armoramount");

		if (focus_item)
		{
			// Print armor amount
			trap->R_SetColor(focus_item->window.foreColor);

			CG_DrawNumField(
				focus_item->window.rect.x - 3.4,
				focus_item->window.rect.y + 23,
				3,
				ps->stats[STAT_ARMOR],
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				NUM_FONT_SMALL,
				qfalse);
		}
	}
	else
	{
		focus_item = Menu_FindItemByName(menu_hud, "armoramount");

		if (focus_item)
		{
			// Print armor amount
			trap->R_SetColor(focus_item->window.foreColor);

			CG_DrawNumField(
				focus_item->window.rect.x - 3.4,
				focus_item->window.rect.y + 3.5,
				3,
				ps->stats[STAT_ARMOR],
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				NUM_FONT_SMALL,
				qfalse);
		}
	}

	// If armor is low, flash a graphic to warn the player
	if (ps->stats[STAT_ARMOR]) // Is there armor? Draw the HUD Armor TIC
	{
		const float quarter_armor = ps->stats[STAT_MAX_HEALTH] / 4.0f;

		// Make tic flash if armor is at 25% of full armor
		if (ps->stats[STAT_ARMOR] < quarter_armor) // Do whatever the flash timer says
		{
			if (cg.HUDTickFlashTime < cg.time) // Flip at the same time
			{
				cg.HUDTickFlashTime = cg.time + 400;
				if (cg.HUDArmorFlag)
				{
					cg.HUDArmorFlag = qfalse;
				}
				else
				{
					cg.HUDArmorFlag = qtrue;
				}
			}
		}
		else
		{
			cg.HUDArmorFlag = qtrue;
		}
	}
	else // No armor? Don't show it.
	{
		cg.HUDArmorFlag = qfalse;
	}
}

static void CG_DrawJK2Armor(const centity_t* cent, const int x, const int y)
{
	vec4_t calc_color;

	const playerState_t* ps = &cg.snap->ps;

	//	Outer Armor circular
	memcpy(calc_color, colorTable[CT_HUD_GREEN], sizeof(vec4_t));

	int armor = ps->stats[STAT_ARMOR];

	if (armor > ps->stats[STAT_MAX_HEALTH])
	{
		armor = ps->stats[STAT_MAX_HEALTH];
	}

	const float hold = armor - ps->stats[STAT_MAX_HEALTH] / 2;
	float armor_percent = hold / (ps->stats[STAT_MAX_HEALTH] / 2);
	if (armor_percent < 0)
	{
		armor_percent = 0;
	}
	calc_color[0] *= armor_percent;
	calc_color[1] *= armor_percent;
	calc_color[2] *= armor_percent;
	trap->R_SetColor(calc_color);
	CG_DrawPic(x, y, 80, 80, cgs.media.JK2HUDArmor1);

	// Inner Armor circular
	if (armor_percent > 0)
	{
		armor_percent = 1;
	}
	else
	{
		armor_percent = (float)ps->stats[STAT_ARMOR] / (ps->stats[STAT_MAX_HEALTH] / 2);
	}
	memcpy(calc_color, colorTable[CT_HUD_GREEN], sizeof(vec4_t));
	calc_color[0] *= armor_percent;
	calc_color[1] *= armor_percent;
	calc_color[2] *= armor_percent;
	trap->R_SetColor(calc_color);
	CG_DrawPic(x, y, 80, 80, cgs.media.JK2HUDArmor2); //	Inner Armor circular

	if (ps->stats[STAT_ARMOR]) // Is there armor? Draw the HUD Armor TIC
	{
		// Make tic flash if inner armor is at 50% (25% of full armor)
		if (armor_percent < .5) // Do whatever the flash timer says
		{
			if (cg.HUDTickFlashTime < cg.time) // Flip at the same time
			{
				cg.HUDTickFlashTime = cg.time + 100;
				if (cg.HUDArmorFlag)
				{
					cg.HUDArmorFlag = qfalse;
				}
				else
				{
					cg.HUDArmorFlag = qtrue;
				}
			}
		}
		else
		{
			cg.HUDArmorFlag = qtrue;
		}
	}
	else // No armor? Don't show it.
	{
		cg.HUDArmorFlag = qfalse;
	}

	trap->R_SetColor(colorTable[CT_HUD_GREEN]);

	if (cent->currentState.weapon == WP_SABER)
	{
		CG_DrawNumField(x + 16 + 14, y + 40 + 16, 3, ps->stats[STAT_ARMOR], 4, 8, NUM_FONT_SMALL, qfalse);
	}
	else
	{
		CG_DrawNumField(x + 16 + 14, y + 40 + 14, 3, ps->stats[STAT_ARMOR], 6, 12, NUM_FONT_SMALL, qfalse);
	}
}

static void CG_DrawJK2CusArmor(const centity_t* cent, const int x, const int y)
{
	vec4_t calc_color;

	const playerState_t* ps = &cg.snap->ps;

	//	Outer Armor circular
	memcpy(calc_color, colorTable[CT_HUD_GREEN], sizeof(vec4_t));

	int armor = ps->stats[STAT_ARMOR];

	if (armor > ps->stats[STAT_MAX_HEALTH])
	{
		armor = ps->stats[STAT_MAX_HEALTH];
	}

	const float hold = armor - ps->stats[STAT_MAX_HEALTH] / 2;
	float armor_percent = hold / (ps->stats[STAT_MAX_HEALTH] / 2);
	if (armor_percent < 0)
	{
		armor_percent = 0;
	}
	calc_color[0] *= armor_percent;
	calc_color[1] *= armor_percent;
	calc_color[2] *= armor_percent;
	trap->R_SetColor(calc_color);
	CG_DrawPic(x + 2, y - 2, 80, 80, cgs.media.JK2HUDArmor1);

	// Inner Armor circular
	if (armor_percent > 0)
	{
		armor_percent = 1;
	}
	else
	{
		armor_percent = (float)ps->stats[STAT_ARMOR] / (ps->stats[STAT_MAX_HEALTH] / 2);
	}
	memcpy(calc_color, colorTable[CT_HUD_GREEN], sizeof(vec4_t));
	calc_color[0] *= armor_percent;
	calc_color[1] *= armor_percent;
	calc_color[2] *= armor_percent;
	trap->R_SetColor(calc_color);
	CG_DrawPic(x + 2, y - 2, 80, 80, cgs.media.JK2HUDArmor2); //	Inner Armor circular

	if (ps->stats[STAT_ARMOR]) // Is there armor? Draw the HUD Armor TIC
	{
		// Make tic flash if inner armor is at 50% (25% of full armor)
		if (armor_percent < .5) // Do whatever the flash timer says
		{
			if (cg.HUDTickFlashTime < cg.time) // Flip at the same time
			{
				cg.HUDTickFlashTime = cg.time + 100;
				if (cg.HUDArmorFlag)
				{
					cg.HUDArmorFlag = qfalse;
				}
				else
				{
					cg.HUDArmorFlag = qtrue;
				}
			}
		}
		else
		{
			cg.HUDArmorFlag = qtrue;
		}
	}
	else // No armor? Don't show it.
	{
		cg.HUDArmorFlag = qfalse;
	}

	trap->R_SetColor(colorTable[CT_HUD_GREEN]);

	if (cent->currentState.weapon == WP_SABER)
	{
		CG_DrawNumField(x + 31, y + 49, 3, ps->stats[STAT_ARMOR], 4, 8, NUM_FONT_SMALL, qfalse);
	}
	else
	{
		CG_DrawNumField(x + 31, y + 30, 3, ps->stats[STAT_ARMOR], 4, 8, NUM_FONT_SMALL, qfalse);
	}
}

/*
================
CG_DrawSaberStyle

If the weapon is a light saber (which needs no ammo) then draw a graphic showing
the saber style (fast, medium, strong)
================
*/

static void CG_DrawCusSaberStyle(const centity_t* cent, const menuDef_t* menu_hud)
{
	itemDef_t* focus_item;

	if (!cent->currentState.weapon) // We don't have a weapon right now
	{
		return;
	}

	if (cent->currentState.weapon != WP_SABER)
	{
		return;
	}

	// Can we find the menu?
	if (!menu_hud)
	{
		return;
	}

	// draw the current saber style in this window
	switch (cg.predictedPlayerState.fd.saberDrawAnimLevel)
	{
	case 1: //FORCE_LEVEL_1:

		focus_item = Menu_FindItemByName(menu_hud, "saberstyle_fastMP");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}

		break;
	case 2: //FORCE_LEVEL_2:
		focus_item = Menu_FindItemByName(menu_hud, "saberstyle_mediumMP");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
		break;
	case 6: //SS_DUAL
		focus_item = Menu_FindItemByName(menu_hud, "saberstyle_dualMP");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
		break;
	case 7: //SS_STAFF
		focus_item = Menu_FindItemByName(menu_hud, "saberstyle_staffMP");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
		break;
	case 3: //FORCE_LEVEL_3:
		focus_item = Menu_FindItemByName(menu_hud, "saberstyle_strongMP");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
		break;
	case 4: //FORCE_LEVEL_4://Desann
		focus_item = Menu_FindItemByName(menu_hud, "saberstyle_desannMP");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
		break;
	case 5: //FORCE_LEVEL_5://Tavion
		focus_item = Menu_FindItemByName(menu_hud, "saberstyle_tavionMP");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
		break;
	default:;
	}
}

static void CG_DrawCusweapontype(const centity_t* cent, const menuDef_t* menu_hud)
{
	itemDef_t* focus_item;

	if (!cent->currentState.weapon) // We don't have a weapon right now
	{
		return;
	}

	if (cent->currentState.weapon == WP_SABER)
	{
		return;
	}

	// Can we find the menu?
	if (!menu_hud)
	{
		return;
	}

	if (cent->currentState.weapon == WP_MELEE)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_melee");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_STUN_BATON)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_stun_baton");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_BRYAR_OLD)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_sbd_blaster");
		//focus_item = Menu_FindItemByName(menu_hud, "weapontype_briar_pistol");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_BRYAR_PISTOL)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_blaster_pistol");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_BLASTER)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_blaster");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_BOWCASTER)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_bowcaster");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_CONCUSSION)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_concussion");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_DEMP2)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_demp2");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_DET_PACK)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_detpack");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_DISRUPTOR)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_disruptor");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_FLECHETTE)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_flachette");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_REPEATER)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_repeater");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_THERMAL)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_thermal");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_ROCKET_LAUNCHER)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_rocket");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else if (cent->currentState.weapon == WP_TRIP_MINE)
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_tripmine");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
	else
	{
		focus_item = Menu_FindItemByName(menu_hud, "weapontype_melee");

		if (focus_item)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);

			CG_DrawPic(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				focus_item->window.background
			);
		}
	}
}

/*
================
CG_Drawgunfatigue
================
*/

static void CG_DrawCusgunfatigue(const menuDef_t* menu_hud)
{
	//render the CG_Drawgunfatigue meter.
	vec4_t calc_color;
	const int max_blaster_attack_chain_count = BLASTERMISHAPLEVEL_MAX;
	qboolean flash = qfalse;

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	// Can we find the menu?
	if (!menu_hud)
	{
		return;
	}

	// Make the hud flash by setting forceHUDTotalFlashTime above cg.time
	if (cg.mishapHUDTotalFlashTime > cg.time || cg.snap->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_ELEVEN)
	{
		flash = qtrue;
		if (cg.mishapHUDNextFlashTime < cg.time)
		{
			trap->S_StartSound(NULL, 0, CHAN_LOCAL, cgs.media.messageLitSound);
			cg.mishapHUDNextFlashTime = cg.time + 400;

			if (cg.mishapHUDActive)
			{
				cg.mishapHUDActive = qfalse;
			}
			else
			{
				cg.mishapHUDActive = qtrue;
			}
		}
	}
	else // turn HUD back on if it had just finished flashing time.
	{
		cg.mishapHUDNextFlashTime = 0;
		cg.mishapHUDActive = qtrue;
	}

	const float inc = (float)max_blaster_attack_chain_count / MAX_HUD_TICS;
	float value = cg.snap->ps.BlasterAttackChainCount;

	for (int i = MAX_HUD_TICS - 1; i >= 0; i--)
	{
		const itemDef_t* focus_item = Menu_FindItemByName(menu_hud, mishapTics[i]);

		if (!focus_item)
		{
			continue;
		}

		if (value <= 0) // done
		{
			break;
		}
		if (value < inc) // partial tic
		{
			if (flash)
			{
				memcpy(calc_color, colorTable[CT_RED], sizeof(vec4_t));
			}
			else
			{
				memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
			}

			const float percent = value / inc;
			calc_color[3] = percent;
		}
		else
		{
			if (flash)
			{
				memcpy(calc_color, colorTable[CT_RED], sizeof(vec4_t));
			}
			else
			{
				memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
			}
		}

		trap->R_SetColor(calc_color);

		CG_DrawPic(
			focus_item->window.rect.x + 5,
			focus_item->window.rect.y + 5,
			focus_item->window.rect.w,
			focus_item->window.rect.h,
			focus_item->window.background
		);

		value -= inc;
	}
}

static void CG_DrawCussaberfatigue(const menuDef_t* menu_hud)
{
	//render the balance/mishap meter.
	vec4_t calc_color;
	const int saber_fatigue_chain_count = MISHAPLEVEL_MAX;
	qboolean flash = qfalse;

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	// Can we find the menu?
	if (!menu_hud)
	{
		return;
	}

	// Make the hud flash by setting forceHUDTotalFlashTime above cg.time
	if (cg.mishapHUDTotalFlashTime > cg.time || cg.snap->ps.saberFatigueChainCount > MISHAPLEVEL_MAX)
	{
		flash = qtrue;
		if (cg.mishapHUDNextFlashTime < cg.time)
		{
			cg.mishapHUDNextFlashTime = cg.time + 400;
			trap->S_StartSound(NULL, 0, CHAN_LOCAL, cgs.media.overload);

			if (cg.mishapHUDActive)
			{
				cg.mishapHUDActive = qfalse;
			}
			else
			{
				cg.mishapHUDActive = qtrue;
			}
		}
	}
	else // turn HUD back on if it had just finished flashing time.
	{
		cg.mishapHUDNextFlashTime = 0;
		cg.mishapHUDActive = qtrue;
	}

	const float inc = (float)saber_fatigue_chain_count / MAX_SJEHUD_TICS;
	float value = cg.snap->ps.saberFatigueChainCount;

	for (int i = MAX_SJEHUD_TICS - 1; i >= 0; i--)
	{
		const itemDef_t* focus_item = Menu_FindItemByName(menu_hud, sabfatticS[i]);

		if (!focus_item)
		{
			continue;
		}

		if (value <= 0) // done
		{
			break;
		}
		if (value < inc) // partial tic
		{
			if (flash)
			{
				memcpy(calc_color, colorTable[CT_RED], sizeof(vec4_t));
			}
			else
			{
				memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
			}

			const float percent = value / inc;
			calc_color[3] = percent;
		}
		else
		{
			if (flash)
			{
				memcpy(calc_color, colorTable[CT_RED], sizeof(vec4_t));
			}
			else
			{
				memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
			}
		}

		trap->R_SetColor(calc_color);

		CG_DrawPic(
			focus_item->window.rect.x + 5,
			focus_item->window.rect.y + 5,
			focus_item->window.rect.w,
			focus_item->window.rect.h,
			focus_item->window.background
		);

		value -= inc;
	}
}

//draw meter showing blockPoints when it's not full
#define BPFUELBAR_H			100.0f
#define BPFUELBAR_W			5.0f
#define BPFUELBAR_X			(SCREEN_WIDTH-BPFUELBAR_W-4.0f)
#define BPFUELBAR_Y			240.0f

static void CG_DrawoldblockPoints(void)
{
	vec4_t aColor;
	vec4_t b_color;
	vec4_t cColor;
	float x = BPFUELBAR_X;
	const float y = BPFUELBAR_Y;
	float percent = (float)cg.snap->ps.fd.blockPoints / 100.0f * BPFUELBAR_H;

	// Make the hud flash by setting forceHUDTotalFlashTime above cg.time
	if (cg.blockHUDTotalFlashTime > cg.time || cg.snap->ps.fd.blockPoints < BLOCKPOINTS_WARNING)
	{
		if (cg.blockHUDNextFlashTime < cg.time)
		{
			cg.blockHUDNextFlashTime = cg.time + 400;
			trap->S_StartSound(NULL, 0, CHAN_LOCAL, cgs.media.overload);

			if (cg.blockHUDActive)
			{
				cg.blockHUDActive = qfalse;
			}
			else
			{
				cg.blockHUDActive = qtrue;
			}
		}
	}
	else // turn HUD back on if it had just finished flashing time.
	{
		cg.blockHUDNextFlashTime = 0;
		cg.blockHUDActive = qtrue;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (percent > BPFUELBAR_H)
	{
		return;
	}

	if (cg.snap->ps.cloakFuel < 100 && cg.snap->ps.jetpackFuel < 100)
	{
		//if drawing cloakFuel fuel bar too, then move this over...?
		x -= BPFUELBAR_W + 12.0f;
	}

	if (cg.snap->ps.cloakFuel < 100)
	{
		//if drawing cloakFuel fuel bar too, then move this over...?
		x -= BPFUELBAR_W + 6.0f;
	}

	if (cg.snap->ps.jetpackFuel < 100)
	{
		//if drawing jetpack fuel bar too, then move this over...?
		x -= BPFUELBAR_W + 6.0f;
	}

	if (cg.snap->ps.saberFatigueChainCount > MISHAPLEVEL_NONE)
	{
		//if drawing then move this over...?
		x -= BPFUELBAR_W + 6.0f;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	if (cg.snap->ps.fd.blockPoints < 20)
	{
		//color of the bar //red
		aColor[0] = 1.0f;
		aColor[1] = 0.0f;
		aColor[2] = 0.0f;
		aColor[3] = 0.4f;
	}
	else if (cg.snap->ps.fd.blockPoints < BLOCKPOINTS_HALF)
	{
		//color of the bar
		aColor[0] = 0.8f;
		aColor[1] = 0.0f;
		aColor[2] = 0.6f;
		aColor[3] = 0.8f;
	}
	else if (cg.snap->ps.fd.blockPoints < 75)
	{
		//color of the bar
		aColor[0] = 0.4f;
		aColor[1] = 0.0f;
		aColor[2] = 0.6f;
		aColor[3] = 0.8f;
	}
	else
	{
		//color of the bar
		aColor[0] = 85.0f;
		aColor[1] = 0.0f;
		aColor[2] = 85.0f;
		aColor[3] = 0.4f;
	}

	//color of the border
	b_color[0] = 0.0f;
	b_color[1] = 0.0f;
	b_color[2] = 0.0f;
	b_color[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.1f;
	cColor[1] = 0.1f;
	cColor[2] = 0.3f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, BPFUELBAR_W, BPFUELBAR_H, 0.5f, colorTable[CT_BLACK]);

	//now draw the part to show how much fuel there is in the color specified
	CG_FillRect(x + 0.5f, y + 0.5f + (BPFUELBAR_H - percent), BPFUELBAR_W - 0.5f,
		BPFUELBAR_H - 0.5f - (BPFUELBAR_H - percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x + 0.5f, y + 0.5f, BPFUELBAR_W - 0.5f, BPFUELBAR_H - percent, cColor);
}

static void CG_DrawCusblockPoints(const int x, const int y, const menuDef_t* menu_hud)
{
	vec4_t calc_color;

	//	Outer block circular
	//==========================================================================================================//

	if (cg.predictedPlayerState.ManualBlockingFlags & 1 << PERFECTBLOCKING)
	{
		memcpy(calc_color, colorTable[CT_GREEN], sizeof(vec4_t));
	}
	else
	{
		memcpy(calc_color, colorTable[CT_MAGENTA], sizeof(vec4_t));
	}

	const float hold = cg.snap->ps.fd.blockPoints - BLOCK_POINTS_MAX / 2;
	float block_percent = (float)hold / (BLOCK_POINTS_MAX / 2);

	// Make the hud flash by setting forceHUDTotalFlashTime above cg.time
	if (cg.blockHUDTotalFlashTime > cg.time || cg.snap->ps.fd.blockPoints < BLOCKPOINTS_WARNING)
	{
		if (cg.blockHUDNextFlashTime < cg.time)
		{
			cg.blockHUDNextFlashTime = cg.time + 400;
			trap->S_StartSound(NULL, 0, CHAN_LOCAL, cgs.media.overload);

			if (cg.blockHUDActive)
			{
				cg.blockHUDActive = qfalse;
			}
			else
			{
				cg.blockHUDActive = qtrue;
			}
		}
	}
	else // turn HUD back on if it had just finished flashing time.
	{
		cg.blockHUDNextFlashTime = 0;
		cg.blockHUDActive = qtrue;
	}

	if (block_percent < 0)
	{
		block_percent = 0;
	}

	calc_color[0] *= block_percent;
	calc_color[1] *= block_percent;
	calc_color[2] *= block_percent;

	trap->R_SetColor(calc_color);

	if (cg.predictedPlayerState.ManualBlockingFlags & 1 << PERFECTBLOCKING)
	{
		CG_DrawPic(x - 3.4, y + 3.5, 40, 40, cgs.media.HUDblockpointMB1);
	}
	else
	{
		CG_DrawPic(x - 3.4, y + 3.5, 40, 40, cgs.media.HUDblockpoint1);
	}

	// Inner block circular
	//==========================================================================================================//
	if (block_percent > 0)
	{
		block_percent = 1;
	}
	else
	{
		block_percent = (float)cg.snap->ps.fd.blockPoints / (BLOCK_POINTS_MAX / 2);
	}

	if (cg.predictedPlayerState.ManualBlockingFlags & 1 << PERFECTBLOCKING)
	{
		memcpy(calc_color, colorTable[CT_GREEN], sizeof(vec4_t));
	}
	else
	{
		memcpy(calc_color, colorTable[CT_MAGENTA], sizeof(vec4_t));
	}

	calc_color[0] *= block_percent;
	calc_color[1] *= block_percent;
	calc_color[2] *= block_percent;

	trap->R_SetColor(calc_color);

	if (cg.predictedPlayerState.ManualBlockingFlags & 1 << PERFECTBLOCKING)
	{
		CG_DrawPic(x - 3.4, y + 3.5, 40, 40, cgs.media.HUDblockpointMB2);
	}
	else
	{
		CG_DrawPic(x - 3.4, y + 3.5, 40, 40, cgs.media.HUDblockpoint2);
	}

	const itemDef_t* focus_item = Menu_FindItemByName(menu_hud, "blockpointamount");

	if (focus_item)
	{
		// Print blockpointamount amount
		trap->R_SetColor(focus_item->window.foreColor);

		CG_DrawNumField(
			focus_item->window.rect.x - 3.4,
			focus_item->window.rect.y + 3.5,
			3,
			cg.snap->ps.fd.blockPoints,
			focus_item->window.rect.w,
			focus_item->window.rect.h,
			NUM_FONT_SMALL,
			qfalse);
	}
}

/*
================
CG_DrawAmmo
================
*/

static void CG_DrawCusAmmo(const centity_t* cent, const menuDef_t* menu_hud)
{
	vec4_t calc_color;
	float inc = 0.0f;

	const playerState_t* ps = &cg.snap->ps;

	// Can we find the menu?
	if (!menu_hud)
	{
		return;
	}

	if (!cent->currentState.weapon) // We don't have a weapon right now
	{
		return;
	}

	if (cent->currentState.weapon == WP_STUN_BATON || cent->currentState.weapon == WP_MELEE)
	{
		return;
	}

	float value = ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];
	if (value < 0) // No ammo
	{
		return;
	}

	//
	// ammo
	//
	if (cg.oldammo < value)
	{
		cg.oldAmmoTime = cg.time + 200;
	}

	cg.oldammo = value;

	itemDef_t* focus_item;

	if (weaponData[cent->currentState.weapon].energyPerShot == 0 &&
		weaponData[cent->currentState.weapon].altEnergyPerShot == 0)
	{
		//just draw "infinite"
		inc = 8 / MAX_HUD_TICS;
		value = 8;

		focus_item = Menu_FindItemByName(menu_hud, "ammoinfinite");
		trap->R_SetColor(colorTable[CT_YELLOW]);
		if (focus_item)
		{
			CG_DrawProportionalString(focus_item->window.rect.x, focus_item->window.rect.y, "--", NUM_FONT_SMALL,
				focus_item->window.foreColor);
		}
	}
	else
	{
		focus_item = Menu_FindItemByName(menu_hud, "ammoamount");

		// Firing or reloading?
		if (cg.predictedPlayerState.weaponstate == WEAPON_FIRING
			&& cg.predictedPlayerState.weaponTime > 100)
		{
			memcpy(calc_color, colorTable[CT_LTGREY], sizeof(vec4_t));
		}
		else
		{
			if (value > 0)
			{
				if (cg.oldAmmoTime > cg.time)
				{
					memcpy(calc_color, colorTable[CT_YELLOW], sizeof(vec4_t));
				}
				else
				{
					memcpy(calc_color, focus_item->window.foreColor, sizeof(vec4_t));
				}
			}
			else
			{
				memcpy(calc_color, colorTable[CT_RED], sizeof(vec4_t));
			}
		}

		trap->R_SetColor(calc_color);
		if (focus_item)
		{
			if (cent->currentState.eFlags & EF_DOUBLE_AMMO)
			{
				inc = ammoData[weaponData[cent->currentState.weapon].ammoIndex].max * 2.0f / MAX_HUD_TICS;
			}
			else
			{
				inc = (float)ammoData[weaponData[cent->currentState.weapon].ammoIndex].max / MAX_HUD_TICS;
			}
			value = ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];

			CG_DrawNumField(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				3,
				value,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				NUM_FONT_SMALL,
				qfalse);
		}
	}

	trap->R_SetColor(colorTable[CT_WHITE]);

	// Draw tics
	for (int i = MAX_HUD_TICS - 1; i >= 0; i--)
	{
		focus_item = Menu_FindItemByName(menu_hud, ammoTicName[i]);

		if (!focus_item)
		{
			continue;
		}

		memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));

		if (value <= 0) // done
		{
			break;
		}
		if (value < inc) // partial tic
		{
			const float percent = value / inc;
			calc_color[3] = percent;
		}

		trap->R_SetColor(calc_color);

		CG_DrawPic(
			focus_item->window.rect.x + 5,
			focus_item->window.rect.y + 5,
			focus_item->window.rect.w,
			focus_item->window.rect.h,
			focus_item->window.background
		);

		value -= inc;
	}
}

static void CG_DrawJK2Ammo(const centity_t* cent, const int x, const int y)
{
	vec4_t calc_color;

	const playerState_t* ps = &cg.snap->ps;

	if (!cent->currentState.weapon) // We don't have a weapon right now
	{
		return;
	}

	if (cent->currentState.weapon == WP_SABER)
	{
		trap->R_SetColor(colorTable[CT_WHITE]);
		// don't need to draw ammo, but we will draw the current saber style in this window
		switch (cg.predictedPlayerState.fd.saberDrawAnimLevel)
		{
		case 1: //FORCE_LEVEL_1:
			CG_DrawPic(x, y, 80, 40, cgs.media.JK2HUDSaberStyleFast);
			break;
		case 2: //FORCE_LEVEL_2:
			CG_DrawPic(x, y, 80, 40, cgs.media.JK2HUDSaberStyleMed);
			break;
		case 3: //FORCE_LEVEL_3:
			CG_DrawPic(x, y, 80, 40, cgs.media.JK2HUDSaberStyleStrong);
			break;
		case 4: //FORCE_LEVEL_4://Desann
			CG_DrawPic(x, y, 80, 40, cgs.media.JK2HUDSaberStyleDesann);
			break;
		case 5: //FORCE_LEVEL_5://Tavion
			CG_DrawPic(x, y, 80, 40, cgs.media.JK2HUDSaberStyleTavion);
			break;
		case 6: //FORCE_LEVEL_5://STAFF
			CG_DrawPic(x, y, 80, 40, cgs.media.JK2HUDSaberStyleStaff);
			break;
		case 7: //FORCE_LEVEL_5://DUELS
			CG_DrawPic(x, y, 80, 40, cgs.media.JK2HUDSaberStyleDuels);
			break;
		default:;
		}
		return;
	}
	float value = ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];

	if (value < 0) // No ammo
	{
		return;
	}

	const int num_color_i = CT_HUD_ORANGE;

	trap->R_SetColor(colorTable[num_color_i]);

	if (cent->currentState.weapon == WP_STUN_BATON)
	{
	}
	else
	{
		CG_DrawNumField(x + 30, y + 26, 3, value, 6, 12, NUM_FONT_SMALL, qfalse);
	}

	const float inc = (float)ammoData[weaponData[cent->currentState.weapon].ammoIndex].max / MAX_TICS;
	value = ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];

	for (int i = MAX_TICS - 1; i >= 0; i--)
	{
		if (value <= 0) // partial tic
		{
			memcpy(calc_color, colorTable[CT_BLACK], sizeof(vec4_t));
		}
		else if (value < inc) // partial tic
		{
			memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
			const float percent = value / inc;
			calc_color[0] *= percent;
			calc_color[1] *= percent;
			calc_color[2] *= percent;
		}
		else
		{
			memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
		}

		trap->R_SetColor(calc_color);
		CG_DrawPic(x + JK2ammoTicPos[i].x,
			y + JK2ammoTicPos[i].y,
			JK2ammoTicPos[i].width,
			JK2ammoTicPos[i].height,
			JK2ammoTicPos[i].tic);

		value -= inc;
	}
}

/*
================
CG_DrawForcePower
================
*/

static void CG_Draw_JKA_ForcePower(const centity_t* cent, const menuDef_t* menu_hud)
{
	vec4_t calc_color;
	itemDef_t* focus_item;
	const int max_force_power = 100;
	qboolean flash = qfalse;

	// Can we find the menu?
	if (!menu_hud)
	{
		return;
	}

	// Make the hud flash by setting forceHUDTotalFlashTime above cg.time
	if (cg.forceHUDTotalFlashTime > cg.time || cg_entities[cg.snap->ps.clientNum].currentState.userInt3 & 1 << FLAG_FATIGUED)
	{
		flash = qtrue;
		if (cg.forceHUDNextFlashTime < cg.time)
		{
			cg.forceHUDNextFlashTime = cg.time + 400;
			trap->S_StartSound(NULL, 0, CHAN_LOCAL, cgs.media.noforceSound);

			if (cg.forceHUDActive)
			{
				cg.forceHUDActive = qfalse;
			}
			else
			{
				cg.forceHUDActive = qtrue;
			}
		}
	}
	else // turn HUD back on if it had just finished flashing time.
	{
		cg.forceHUDNextFlashTime = 0;
		cg.forceHUDActive = qtrue;
	}

	const float inc = (float)max_force_power / MAX_HUD_TICS;
	float value = cg.snap->ps.fd.forcePower;

	for (int i = MAX_HUD_TICS - 1; i >= 0; i--)
	{
		focus_item = Menu_FindItemByName(menu_hud, forceTicName[i]);

		if (!focus_item)
		{
			continue;
		}

		if (value <= 0) // done
		{
			break;
		}
		if (value < inc) // partial tic
		{
			if (flash)
			{
				memcpy(calc_color, colorTable[CT_RED], sizeof(vec4_t));
			}
			else
			{
				memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
			}

			const float percent = value / inc;
			calc_color[3] = percent;
		}
		else
		{
			if (flash)
			{
				memcpy(calc_color, colorTable[CT_RED], sizeof(vec4_t));
			}
			else
			{
				memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
			}
		}

		trap->R_SetColor(calc_color);

		CG_DrawPic(
			focus_item->window.rect.x + 5,
			focus_item->window.rect.y + 5,
			focus_item->window.rect.w,
			focus_item->window.rect.h,
			focus_item->window.background
		);

		value -= inc;
	}

	if (cent->currentState.weapon == WP_SABER)
	{
		focus_item = Menu_FindItemByName(menu_hud, "forceamount");

		if (focus_item)
		{
			// Print force amount
			if (flash)
			{
				trap->R_SetColor(colorTable[CT_RED]);
			}
			else
			{
				trap->R_SetColor(focus_item->window.foreColor);
			}

			CG_DrawNumField(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				3,
				cg.snap->ps.fd.forcePower,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				NUM_FONT_SMALL,
				qfalse);
		}
	}
	else if (cent->currentState.weapon == WP_MELEE && !cg.snap->ps.BlasterAttackChainCount)
	{
		focus_item = Menu_FindItemByName(menu_hud, "forceamount");

		if (focus_item)
		{
			// Print force amount
			if (flash)
			{
				trap->R_SetColor(colorTable[CT_RED]);
			}
			else
			{
				trap->R_SetColor(focus_item->window.foreColor);
			}

			CG_DrawNumField(
				focus_item->window.rect.x + 5,
				focus_item->window.rect.y + 5,
				3,
				cg.snap->ps.fd.forcePower,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				NUM_FONT_SMALL,
				qfalse);
		}
	}
}

static void CG_DrawJK2ForcePower(const centity_t* cent, const int x, const int y, const menuDef_t* menu_hud)
{
	vec4_t calc_color;
	itemDef_t* focus_item;

	const float inc = (float)100 / MAX_TICS;
	float value = cg.snap->ps.fd.forcePower;

	for (int i = MAX_TICS - 1; i >= 0; i--)
	{
		if (value <= 0) // partial tic
		{
			memcpy(calc_color, colorTable[CT_BLACK], sizeof(vec4_t));
		}
		else if (value < inc) // partial tic
		{
			memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
			const float percent = value / inc;
			calc_color[0] *= percent;
			calc_color[1] *= percent;
			calc_color[2] *= percent;
		}
		else
		{
			memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
		}

		trap->R_SetColor(calc_color);
		CG_DrawPic(x + JK2forceTicPos[i].x,
			y + JK2forceTicPos[i].y,
			JK2forceTicPos[i].width,
			JK2forceTicPos[i].height,
			JK2forceTicPos[i].tic);

		value -= inc;
	}

	if (cent->currentState.weapon == WP_SABER || cent->currentState.weapon == WP_MELEE)
	{
		focus_item = Menu_FindItemByName(menu_hud, "forceamount");

		if (focus_item)
		{
			trap->R_SetColor(focus_item->window.foreColor);

			CG_DrawNumField(
				focus_item->window.rect.x + 4,
				focus_item->window.rect.y + 8,
				3,
				cg.snap->ps.fd.forcePower,
				focus_item->window.rect.w,
				focus_item->window.rect.h,
				NUM_FONT_SMALL,
				qfalse);
		}
	}
	else
	{
		if (!cg.snap->ps.BlasterAttackChainCount && com_outcast.integer == 1)
		{
			focus_item = Menu_FindItemByName(menu_hud, "forceamount");

			if (focus_item)
			{
				trap->R_SetColor(focus_item->window.foreColor);

				CG_DrawNumField(
					focus_item->window.rect.x + 4,
					focus_item->window.rect.y + 8,
					3,
					cg.snap->ps.fd.forcePower,
					focus_item->window.rect.w,
					focus_item->window.rect.h,
					NUM_FONT_SMALL,
					qfalse);
			}
		}
	}
}

static void CG_DrawJK2NoSaberForcePower(const centity_t* cent, const int x, const int y, const menuDef_t* menu_hud)
{
	vec4_t calc_color;

	const float inc = (float)100 / MAX_TICS;
	float value = cg.snap->ps.fd.forcePower;

	for (int i = MAX_TICS - 1; i >= 0; i--)
	{
		if (value <= 0) // partial tic
		{
			memcpy(calc_color, colorTable[CT_BLACK], sizeof(vec4_t));
		}
		else if (value < inc) // partial tic
		{
			memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
			const float percent = value / inc;
			calc_color[0] *= percent;
			calc_color[1] *= percent;
			calc_color[2] *= percent;
		}
		else
		{
			memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
		}

		trap->R_SetColor(calc_color);
		CG_DrawPic(x + JK2forceTicPos[i].x,
			y + JK2forceTicPos[i].y,
			JK2forceTicPos[i].width,
			JK2forceTicPos[i].height,
			JK2forceTicPos[i].tic);

		value -= inc;
	}

	if (cent->currentState.weapon == WP_STUN_BATON)
	{
	}
	else
	{
		if (!cg.snap->ps.saberFatigueChainCount && !cg.snap->ps.BlasterAttackChainCount)
		{
			const itemDef_t* focus_item = Menu_FindItemByName(menu_hud, "forceamount");

			if (focus_item)
			{
				trap->R_SetColor(focus_item->window.foreColor);

				CG_DrawNumField(
					focus_item->window.rect.x + 4,
					focus_item->window.rect.y + 8,
					3,
					cg.snap->ps.fd.forcePower,
					focus_item->window.rect.w,
					focus_item->window.rect.h,
					NUM_FONT_SMALL,
					qfalse);
			}
		}
	}
}

static void CG_DrawJK2GunFatigue(const centity_t* cent, const int x, const int y)
{
	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (!cg.snap->ps.BlasterAttackChainCount)
	{
		return;
	}

	trap->R_SetColor(colorTable[CT_WHITE]);

	if (com_outcast.integer == 1) //jko
	{
		if (cent->currentState.weapon == WP_STUN_BATON)
		{
		}
		else
		{
			if (cg.mishapHUDTotalFlashTime > cg.time || cg.snap->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_ELEVEN)
			{
				if (!(cg.time / 600 & 1))
				{
					if (!cg.messageLitActive)
					{
						trap->S_StartSound(NULL, 0, CHAN_LOCAL, cgs.media.messageLitSound);
						cg.messageLitActive = qtrue;
					}

					trap->R_SetColor(colorTable[CT_HUD_RED]);
					CG_DrawPic(x + 33, y + 41, 16, 16, cgs.media.messageLitOn);
				}
				else
				{
					cg.messageLitActive = qfalse;
				}
			}

			if (cg.snap->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_LIGHT)
			{
				trap->R_SetColor(colorTable[CT_WHITE]);
				CG_DrawPic(x + 33, y + 41, 16, 16, cgs.media.messageObjCircle);
			}
			else
			{
				trap->R_SetColor(colorTable[CT_WHITE]);
				CG_DrawPic(x + 33, y + 41, 16, 16, cgs.media.messageLitOff);
			}
		}
	}
	else
	{
		if (cg.mishapHUDTotalFlashTime > cg.time || cg.snap->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_ELEVEN)
		{
			if (!(cg.time / 600 & 1))
			{
				if (!cg.messageLitActive)
				{
					trap->S_StartSound(NULL, 0, CHAN_LOCAL, cgs.media.messageLitSound);
					cg.messageLitActive = qtrue;
				}

				trap->R_SetColor(colorTable[CT_HUD_RED]);
				CG_DrawPic(x + 33, y + 41, 16, 16, cgs.media.messageLitOn);
			}
			else
			{
				cg.messageLitActive = qfalse;
			}
		}

		if (cg.snap->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_LIGHT)
		{
			trap->R_SetColor(colorTable[CT_WHITE]);
			CG_DrawPic(x + 33, y + 41, 16, 16, cgs.media.messageObjCircle);
		}
		else
		{
			trap->R_SetColor(colorTable[CT_WHITE]);
			CG_DrawPic(x + 33, y + 41, 16, 16, cgs.media.messageLitOff);
		}
	}
}

static void CG_DrawJK2SaberFatigue(const centity_t* cent, const int x, const int y)
{
	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (!cg.snap->ps.saberFatigueChainCount)
	{
		return;
	}

	if (com_outcast.integer == 1) //jko
	{
		if (cg.mishapHUDTotalFlashTime > cg.time || cg.snap->ps.saberFatigueChainCount > MISHAPLEVEL_HUDFLASH)
		{
			if (!(cg.time / 600 & 1))
			{
				if (!cg.messageLitActive)
				{
					trap->S_StartSound(NULL, 0, CHAN_LOCAL, cgs.media.messageLitSound);
					cg.messageLitActive = qtrue;
				}

				trap->R_SetColor(colorTable[CT_HUD_RED]);
				CG_DrawPic(x + 33, y + 41 - 21, 16 + 1, 16, cgs.media.messageLitOn);
			}
			else
			{
				cg.messageLitActive = qfalse;
			}
		}
		else if (cg.mishapHUDTotalFlashTime > cg.time || cg.snap->ps.saberFatigueChainCount > MISHAPLEVEL_TEN)
		{
			if (!(cg.time / 600 & 1))
			{
				if (!cg.messageLitActive)
				{
					cg.messageLitActive = qtrue;
				}

				trap->R_SetColor(colorTable[CT_MAGENTA]);
				CG_DrawPic(x + 33, y + 41 - 21, 16 + 1, 16, cgs.media.messageLitOn);
			}
			else
			{
				cg.messageLitActive = qfalse;
			}
		}
		else if (cg.mishapHUDTotalFlashTime > cg.time || cg.snap->ps.saberFatigueChainCount > MISHAPLEVEL_SIX)
		{
			if (!(cg.time / 600 & 1))
			{
				if (!cg.messageLitActive)
				{
					cg.messageLitActive = qtrue;
				}

				trap->R_SetColor(colorTable[CT_BLUE]);
				CG_DrawPic(x + 33, y + 41 - 21, 16 + 1, 16, cgs.media.messageLitOn);
			}
			else
			{
				cg.messageLitActive = qfalse;
			}
		}
		else if (cg.mishapHUDTotalFlashTime > cg.time || cg.snap->ps.saberFatigueChainCount > MISHAPLEVEL_TWO)
		{
			if (!(cg.time / 600 & 1))
			{
				if (!cg.messageLitActive)
				{
					cg.messageLitActive = qtrue;
				}

				trap->R_SetColor(colorTable[CT_GREEN]);
				CG_DrawPic(x + 33, y + 41 - 21, 16 + 1, 16, cgs.media.messageLitOn);
			}
			else
			{
				cg.messageLitActive = qfalse;
			}
		}
	}
	else
	{
		if (cg.mishapHUDTotalFlashTime > cg.time || cg.snap->ps.saberFatigueChainCount > MISHAPLEVEL_HUDFLASH)
		{
			if (!(cg.time / 600 & 1))
			{
				if (!cg.messageLitActive)
				{
					trap->S_StartSound(NULL, 0, CHAN_LOCAL, cgs.media.messageLitSound);
					cg.messageLitActive = qtrue;
				}

				trap->R_SetColor(colorTable[CT_HUD_RED]);
				CG_DrawPic(x + 33, y + 41 - 18, 16 + 1, 16, cgs.media.messageLitOn);
			}
			else
			{
				cg.messageLitActive = qfalse;
			}
		}
		else if (cg.mishapHUDTotalFlashTime > cg.time || cg.snap->ps.saberFatigueChainCount > MISHAPLEVEL_TEN)
		{
			if (!(cg.time / 600 & 1))
			{
				if (!cg.messageLitActive)
				{
					cg.messageLitActive = qtrue;
				}

				trap->R_SetColor(colorTable[CT_MAGENTA]);
				CG_DrawPic(x + 33, y + 41 - 18, 16 + 1, 16, cgs.media.messageLitOn);
			}
			else
			{
				cg.messageLitActive = qfalse;
			}
		}
		else if (cg.mishapHUDTotalFlashTime > cg.time || cg.snap->ps.saberFatigueChainCount > MISHAPLEVEL_SIX)
		{
			if (!(cg.time / 600 & 1))
			{
				if (!cg.messageLitActive)
				{
					cg.messageLitActive = qtrue;
				}

				trap->R_SetColor(colorTable[CT_BLUE]);
				CG_DrawPic(x + 33, y + 41 - 18, 16 + 1, 16, cgs.media.messageLitOn);
			}
			else
			{
				cg.messageLitActive = qfalse;
			}
		}
		else if (cg.mishapHUDTotalFlashTime > cg.time || cg.snap->ps.saberFatigueChainCount > MISHAPLEVEL_TWO)
		{
			if (!(cg.time / 600 & 1))
			{
				if (!cg.messageLitActive)
				{
					cg.messageLitActive = qtrue;
				}

				trap->R_SetColor(colorTable[CT_GREEN]);
				CG_DrawPic(x + 33, y + 41 - 18, 16 + 1, 16, cgs.media.messageLitOn);
			}
			else
			{
				cg.messageLitActive = qfalse;
			}
		}
	}
}

static void CG_DrawJK2blockingMode(const centity_t* cent, const menuDef_t* menu_hud)
{
	itemDef_t* blockindex;

	if (!cent->currentState.weapon) // We don't have a weapon right now
	{
		return;
	}

	if (cent->currentState.weapon != WP_SABER)
	{
		return;
	}

	// Can we find the menu?
	if (!menu_hud)
	{
		return;
	}

	if (com_outcast.integer == 1) //jko
	{
		if (cg.predictedPlayerState.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK)
		{
			blockindex = Menu_FindItemByName(menu_hud, "jk2MBlockingMode");

			if (blockindex)
			{
				trap->R_SetColor(colorTable[CT_WHITE]);

				CG_DrawPic(
					blockindex->window.rect.x,
					blockindex->window.rect.y - 21,
					blockindex->window.rect.w + 1,
					blockindex->window.rect.h,
					blockindex->window.background
				);
			}
		}
		else if (cg.predictedPlayerState.ManualBlockingFlags & 1 << HOLDINGBLOCK)
		{
			blockindex = Menu_FindItemByName(menu_hud, "jk2BlockingMode");

			if (blockindex)
			{
				trap->R_SetColor(colorTable[CT_WHITE]);

				CG_DrawPic(
					blockindex->window.rect.x,
					blockindex->window.rect.y - 21,
					blockindex->window.rect.w + 1,
					blockindex->window.rect.h,
					blockindex->window.background
				);
			}
		}
		else
		{
			blockindex = Menu_FindItemByName(menu_hud, "jk2NotBlocking");

			if (blockindex)
			{
				trap->R_SetColor(colorTable[CT_WHITE]);

				CG_DrawPic(
					blockindex->window.rect.x,
					blockindex->window.rect.y - 21,
					blockindex->window.rect.w + 1,
					blockindex->window.rect.h,
					blockindex->window.background
				);
			}
		}
	}
	else
	{
		if (cg.predictedPlayerState.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK)
		{
			blockindex = Menu_FindItemByName(menu_hud, "jk2MBlockingMode");

			if (blockindex)
			{
				trap->R_SetColor(colorTable[CT_WHITE]);

				CG_DrawPic(
					blockindex->window.rect.x,
					blockindex->window.rect.y - 18,
					blockindex->window.rect.w + 1,
					blockindex->window.rect.h,
					blockindex->window.background
				);
			}
		}
		else if (cg.predictedPlayerState.ManualBlockingFlags & 1 << HOLDINGBLOCK)
		{
			blockindex = Menu_FindItemByName(menu_hud, "jk2BlockingMode");

			if (blockindex)
			{
				trap->R_SetColor(colorTable[CT_WHITE]);

				CG_DrawPic(
					blockindex->window.rect.x,
					blockindex->window.rect.y - 18,
					blockindex->window.rect.w + 1,
					blockindex->window.rect.h,
					blockindex->window.background
				);
			}
		}
		else
		{
			blockindex = Menu_FindItemByName(menu_hud, "jk2NotBlocking");

			if (blockindex)
			{
				trap->R_SetColor(colorTable[CT_WHITE]);

				CG_DrawPic(
					blockindex->window.rect.x,
					blockindex->window.rect.y - 18,
					blockindex->window.rect.w + 1,
					blockindex->window.rect.h,
					blockindex->window.background
				);
			}
		}
	}
}

static void CG_DrawSimpleSaberStyle(const centity_t* cent)
{
	uint32_t calc_color;
	char num[7] = { 0 };
	int weap_x = 16;

	if (!cent->currentState.weapon) // We don't have a weapon right now
	{
		return;
	}

	if (cent->currentState.weapon != WP_SABER)
	{
		return;
	}

	switch (cg.predictedPlayerState.fd.saberDrawAnimLevel)
	{
	default:
	case SS_FAST:
		Com_sprintf(num, sizeof num, "FAST");
		calc_color = CT_ICON_BLUE;
		weap_x = 0;
		break;
	case SS_MEDIUM:
		Com_sprintf(num, sizeof num, "MEDIUM");
		calc_color = CT_YELLOW;
		break;
	case SS_STRONG:
		Com_sprintf(num, sizeof num, "STRONG");
		calc_color = CT_HUD_RED;
		break;
	case SS_DESANN:
		Com_sprintf(num, sizeof num, "DESANN");
		calc_color = CT_HUD_RED;
		break;
	case SS_TAVION:
		Com_sprintf(num, sizeof num, "TAVION");
		calc_color = CT_ICON_BLUE;
		break;
	case SS_DUAL:
		Com_sprintf(num, sizeof num, "AKIMBO");
		calc_color = CT_HUD_ORANGE;
		break;
	case SS_STAFF:
		Com_sprintf(num, sizeof num, "STAFF");
		calc_color = CT_HUD_ORANGE;
		break;
	}

	CG_DrawProportionalString(SCREEN_WIDTH - (weap_x + 16 + 32), SCREEN_HEIGHT - 80 + 40, num,
		UI_SMALLFONT | UI_DROPSHADOW, colorTable[calc_color]);
}

static void CG_DrawSimpleAmmo(const centity_t* cent)
{
	uint32_t calc_color;
	char num[16] = { 0 };

	if (!cent->currentState.weapon) // We don't have a weapon right now
	{
		return;
	}

	const playerState_t* ps = &cg.snap->ps;

	const int curr_value = ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];

	// No ammo
	if (curr_value < 0 || weaponData[cent->currentState.weapon].energyPerShot == 0 && weaponData[cent->currentState.
		weapon].altEnergyPerShot == 0)
	{
		CG_DrawProportionalString(SCREEN_WIDTH - (16 + 32), SCREEN_HEIGHT - 80 + 40, "--", UI_SMALLFONT | UI_DROPSHADOW,
			colorTable[CT_HUD_ORANGE]);
		return;
	}

	//
	// ammo
	//
	if (cg.oldammo < curr_value)
	{
		cg.oldAmmoTime = cg.time + 200;
	}

	cg.oldammo = curr_value;

	// Determine the color of the numeric field

	// Firing or reloading?
	if (cg.predictedPlayerState.weaponstate == WEAPON_FIRING
		&& cg.predictedPlayerState.weaponTime > 100)
	{
		calc_color = CT_LTGREY;
	}
	else
	{
		if (curr_value > 0)
		{
			if (cg.oldAmmoTime > cg.time)
			{
				calc_color = CT_YELLOW;
			}
			else
			{
				calc_color = CT_HUD_ORANGE;
			}
		}
		else
		{
			calc_color = CT_RED;
		}
	}

	Com_sprintf(num, sizeof num, "%i", curr_value);

	CG_DrawProportionalString(SCREEN_WIDTH - (16 + 32), SCREEN_HEIGHT - 80 + 40, num, UI_SMALLFONT | UI_DROPSHADOW,
		colorTable[calc_color]);
}

static void CG_DrawSimpleForcePower()
{
	char num[16] = { 0 };
	qboolean flash = qfalse;

	if (!cg.snap->ps.fd.forcePowersKnown)
	{
		return;
	}

	// Make the hud flash by setting forceHUDTotalFlashTime above cg.time
	if (cg.forceHUDTotalFlashTime > cg.time || cg_entities[cg.snap->ps.clientNum].currentState.userInt3 & 1 <<
		FLAG_FATIGUED)
	{
		flash = qtrue;
		if (cg.forceHUDNextFlashTime < cg.time)
		{
			cg.forceHUDNextFlashTime = cg.time + 400;
			trap->S_StartSound(NULL, 0, CHAN_LOCAL, cgs.media.noforceSound);
			if (cg.forceHUDActive)
			{
				cg.forceHUDActive = qfalse;
			}
			else
			{
				cg.forceHUDActive = qtrue;
			}
		}
	}
	else // turn HUD back on if it had just finished flashing time.
	{
		cg.forceHUDNextFlashTime = 0;
		cg.forceHUDActive = qtrue;
	}

	// Determine the color of the numeric field
	const uint32_t calc_color = flash ? CT_RED : CT_ICON_BLUE;

	Com_sprintf(num, sizeof num, "%i", cg.snap->ps.fd.forcePower);

	CG_DrawProportionalString(SCREEN_WIDTH - (18 + 14 + 32), SCREEN_HEIGHT - 80 + 40 + 14, num,
		UI_SMALLFONT | UI_DROPSHADOW, colorTable[calc_color]);
};

//jk2 hud
vec4_t bluehudtint = { 0.5, 0.5, 1.0, 1.0 };
vec4_t redhudtint = { 1.0, 0.5, 0.5, 1.0 };
float* hudTintColor;

/*
================
CG_DrawHUDJK2LeftFrame1
================
*/
static void CG_DrawHUDJK2LeftFrame1(const int x, const int y)
{
	// Inner gray wire frame
	trap->R_SetColor(hudTintColor);
	CG_DrawPic(x, y, 80, 80, cgs.media.JK2HUDInnerLeft);
}

/*
================
CG_DrawHUDJK2LeftFrame2
================
*/
static void CG_DrawHUDJK2LeftFrame2(const int x, const int y)
{
	// Inner gray wire frame
	trap->R_SetColor(colorTable[CT_WHITE]);
	CG_DrawPic(x, y, 80, 80, cgs.media.JK2HUDLeftFrame); // Metal frame
}

/*
================
CG_DrawHUDJK2RightFrame1
================
*/
static void CG_DrawHUDJK2RightFrame1(const int x, const int y)
{
	trap->R_SetColor(hudTintColor);
	// Inner gray wire frame
	CG_DrawPic(x, y, 80, 80, cgs.media.JK2HUDInnerRight); //
}

/*
================
CG_DrawHUDJK2RightFrame2
================
*/
static void CG_DrawHUDJK2RightFrame2(const int x, const int y)
{
	trap->R_SetColor(colorTable[CT_WHITE]);
	CG_DrawPic(x, y, 80, 80, cgs.media.JK2HUDRightFrame); // Metal frame
}

/*
================
CG_DrawHUD
================
*/
static void CG_DrawHUD(const centity_t* cent)
{
	// don't display if dead
	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (cg.predictedPlayerState.pm_flags & PMF_FOLLOW || cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.predictedPlayerState.communicatingflags & (1 << CF_SABERLOCKING) && g_saberLockCinematicCamera.integer)
	{
		return;
	}

	if (cg.predictedPlayerState.m_iVehicleNum)
	{
		//I'm in a vehicle
		return;
	}

	if (cg_hudFiles.integer)
	{
		const int y1 = SCREEN_HEIGHT - 80;

		if (cg.predictedPlayerState.pm_type != PM_SPECTATOR)
		{
			const int i = 0;
			CG_DrawProportionalString(i + 16, y1 + 40, va("%i", cg.snap->ps.stats[STAT_HEALTH]),
				UI_SMALLFONT | UI_DROPSHADOW, colorTable[CT_HUD_RED]);

			CG_DrawProportionalString(i + 18 + 14, y1 + 40 + 14, va("%i", cg.snap->ps.stats[STAT_ARMOR]),
				UI_SMALLFONT | UI_DROPSHADOW, colorTable[CT_HUD_GREEN]);

			CG_DrawSimpleForcePower();

			if (cent->currentState.weapon == WP_SABER)
			{
				CG_DrawSimpleSaberStyle(cent);

				if (cg.snap->ps.fd.blockPoints < BLOCK_POINTS_MAX)
				{
					//draw it as long as it isn't full
					CG_DrawoldblockPoints();
				}
			}
			else
			{
				CG_DrawSimpleAmmo(cent);
			}
		}

		return;
	}

	if (com_outcast.integer == 1)
	{
		if (cg.snap->ps.fd.forcePowersActive & 1 << FP_GRIP
			|| cg.snap->ps.fd.forcePowersActive & 1 << FP_DRAIN
			|| cg.snap->ps.fd.forcePowersActive & 1 << FP_LIGHTNING
			|| cg.snap->ps.fd.forcePowersActive & 1 << FP_RAGE)
		{
			hudTintColor = redhudtint;
		}
		else if (cg.snap->ps.fd.forcePowersActive & 1 << FP_ABSORB
			|| cg.snap->ps.fd.forcePowersActive & 1 << FP_HEAL
			|| cg.snap->ps.fd.forcePowersActive & 1 << FP_PROTECT
			|| cg.snap->ps.fd.forcePowersActive & 1 << FP_TELEPATHY)
		{
			hudTintColor = bluehudtint;
		}
		else
		{
			hudTintColor = colorTable[CT_WHITE];
		}
	}

	// Draw the left HUD
	if (cg.predictedPlayerState.pm_type != PM_SPECTATOR)
	{
		const char* score_str;
		menuDef_t* menu_hud = Menus_FindByName("lefthud");
		Menu_Paint(menu_hud, qtrue);

		if (menu_hud)
		{
			// Print frame
			if (com_outcast.integer == 0) //jka
			{
				CG_DrawHUDJK2LeftFrame2(0, SCREEN_HEIGHT - 80);

				// Print scanline
				const itemDef_t* focus_item = Menu_FindItemByName(menu_hud, "scanline");

				if (focus_item)
				{
					trap->R_SetColor(colorTable[CT_WHITE]);
					CG_DrawPic(
						focus_item->window.rect.x - 3.3,
						focus_item->window.rect.y + 3.5,
						focus_item->window.rect.w,
						focus_item->window.rect.h,
						focus_item->window.background
					);
				}

				focus_item = Menu_FindItemByName(menu_hud, "frame");

				if (focus_item)
				{
					trap->R_SetColor(colorTable[CT_WHITE]);
					CG_DrawPic(
						focus_item->window.rect.x - 3.3,
						focus_item->window.rect.y + 3.5,
						focus_item->window.rect.w,
						focus_item->window.rect.h,
						focus_item->window.background
					);
				}
				CG_Draw_JKA_Armor(menu_hud);
				CG_DrawCusHealth(menu_hud);
				if (cent->currentState.weapon == WP_SABER)
				{
					const int x = 26;
					const int y = 415;
					CG_DrawCusblockPoints(x, y, menu_hud);
				}
			}
			else if (com_outcast.integer == 1) //jko
			{
				CG_DrawHUDJK2LeftFrame1(0, SCREEN_HEIGHT - 80);

				CG_DrawJK2Armor(cent, 0, SCREEN_HEIGHT - 80);

				if (cent->currentState.weapon == WP_SABER)
				{
					CG_DrawJK2HealthSJE(21.1, 421);

					CG_DrawJK2blockPoints(23.7, 424, menu_hud);
				}
				else
				{
					CG_DrawJK2Health(0, SCREEN_HEIGHT - 80);
				}

				CG_DrawHUDJK2LeftFrame2(0, SCREEN_HEIGHT - 80);
			}
			else //custom
			{
				CG_DrawHUDJK2LeftFrame2(0, SCREEN_HEIGHT - 80);

				// Print scanline
				const itemDef_t* focus_item = Menu_FindItemByName(menu_hud, "scanline");

				if (focus_item)
				{
					trap->R_SetColor(colorTable[CT_WHITE]);
					CG_DrawPic(
						focus_item->window.rect.x - 3.3,
						focus_item->window.rect.y + 3.5,
						focus_item->window.rect.w,
						focus_item->window.rect.h,
						focus_item->window.background
					);
				}

				focus_item = Menu_FindItemByName(menu_hud, "frame");

				if (focus_item)
				{
					trap->R_SetColor(colorTable[CT_WHITE]);
					CG_DrawPic(
						focus_item->window.rect.x - 3.3,
						focus_item->window.rect.y + 3.5,
						focus_item->window.rect.w,
						focus_item->window.rect.h,
						focus_item->window.background
					);
				}
				CG_DrawJK2CusArmor(cent, 0, SCREEN_HEIGHT - 80);
				CG_DrawCusHealth(menu_hud);
				if (cent->currentState.weapon == WP_SABER)
				{
					const int x = 26;
					const int y = 415;
					CG_DrawCusblockPoints(x, y, menu_hud);
				}
			}
		}

		if (cgs.gametype == GT_DUEL)
		{
			//A duel that requires more than one kill to knock the current enemy back to the queue
			//show current kills out of how many needed
			score_str = va("%s: %i/%i", CG_GetStringEdString("MP_INGAME", "SCORE"), cg.snap->ps.persistant[PERS_SCORE],
				cgs.fraglimit);
		}
		else
		{
			// Don't draw a bias.
			score_str = va("%s: %i", CG_GetStringEdString("MP_INGAME", "SCORE"), cg.snap->ps.persistant[PERS_SCORE]);
		}

		menu_hud = Menus_FindByName("righthud");
		Menu_Paint(menu_hud, qtrue);

		if (menu_hud)
		{
			if (com_outcast.integer == 0) //jka
			{
				CG_DrawHUDJK2RightFrame2(SCREEN_WIDTH - 80, SCREEN_HEIGHT - 80);

				itemDef_t* focus_item;

				if (cgs.gametype != GT_POWERDUEL)
				{
					focus_item = Menu_FindItemByName(menu_hud, "score_line");

					if (focus_item)
					{
						CG_DrawScaledProportionalString(
							focus_item->window.rect.x,
							focus_item->window.rect.y,
							score_str,
							UI_RIGHT | UI_DROPSHADOW,
							focus_item->window.foreColor,
							0.5f);
					}
				}

				// Print scanline
				focus_item = Menu_FindItemByName(menu_hud, "scanline");

				if (focus_item)
				{
					trap->R_SetColor(colorTable[CT_WHITE]);
					CG_DrawPic(
						focus_item->window.rect.x + 4.5,
						focus_item->window.rect.y + 6,
						focus_item->window.rect.w,
						focus_item->window.rect.h,
						focus_item->window.background
					);
				}

				focus_item = Menu_FindItemByName(menu_hud, "frame");

				if (focus_item)
				{
					trap->R_SetColor(colorTable[CT_WHITE]);
					CG_DrawPic(
						focus_item->window.rect.x + 4.5,
						focus_item->window.rect.y + 6,
						focus_item->window.rect.w,
						focus_item->window.rect.h,
						focus_item->window.background
					);
				}

				if (cent->currentState.weapon == WP_SABER)
				{
					CG_DrawCusSaberStyle(cent, menu_hud);
					CG_DrawJK2blockingMode(cent, menu_hud);
					CG_DrawJK2SaberFatigue(cent, 560, 400);
				}
				else if (cent->currentState.weapon == WP_SABER && cent->currentState.saberHolstered)
				{
					CG_DrawCusgunfatigue(menu_hud);
					CG_DrawCusAmmo(cent, menu_hud);
				}
				else if (cent->currentState.weapon == WP_MELEE)
				{
					CG_DrawCusgunfatigue(menu_hud);
					CG_DrawCusweapontype(cent, menu_hud);
				}
				else
				{
					CG_DrawCusAmmo(cent, menu_hud);
					CG_DrawCusgunfatigue(menu_hud);
					CG_DrawCusweapontype(cent, menu_hud);
				}
				CG_Draw_JKA_ForcePower(cent, menu_hud);
			}
			else if (com_outcast.integer == 1) //jko
			{
				const int y = 400;
				const int x = 560;

				CG_DrawHUDJK2RightFrame1(SCREEN_WIDTH - 80, SCREEN_HEIGHT - 80);

				if (cent->currentState.weapon == WP_SABER)
				{
					CG_DrawJK2ForcePower(cent, SCREEN_WIDTH - 80, SCREEN_HEIGHT - 80, menu_hud);
					CG_DrawJK2Ammo(cent, SCREEN_WIDTH - 80, SCREEN_HEIGHT - 80);

					CG_DrawJK2blockingMode(cent, menu_hud);
					CG_DrawJK2SaberFatigue(cent, x, y);
				}
				else if (cent->currentState.weapon == WP_SABER && cent->currentState.saberHolstered)
				{
					CG_DrawJK2ForcePower(cent, SCREEN_WIDTH - 80, SCREEN_HEIGHT - 80, menu_hud);
					CG_DrawJK2Ammo(cent, SCREEN_WIDTH - 80, SCREEN_HEIGHT - 80);
					CG_DrawJK2GunFatigue(cent, x, y);
				}
				else if (cent->currentState.weapon == WP_MELEE)
				{
					CG_DrawJK2NoSaberForcePower(cent, SCREEN_WIDTH - 80, SCREEN_HEIGHT - 80, menu_hud);
					CG_DrawJK2GunFatigue(cent, x, y);
				}
				else
				{
					CG_DrawJK2NoSaberForcePower(cent, SCREEN_WIDTH - 80, SCREEN_HEIGHT - 80, menu_hud);
					CG_DrawJK2Ammo(cent, SCREEN_WIDTH - 80, SCREEN_HEIGHT - 80);
					CG_DrawJK2GunFatigue(cent, x, y);
				}

				if (cent->currentState.weapon == WP_STUN_BATON)
				{
					CG_DrawCusweapontype(cent, menu_hud);
				}

				CG_DrawHUDJK2RightFrame2(SCREEN_WIDTH - 80, SCREEN_HEIGHT - 80);
			}
			else //custom
			{
				CG_DrawHUDJK2RightFrame2(SCREEN_WIDTH - 80, SCREEN_HEIGHT - 80);

				itemDef_t* focus_item;

				if (cgs.gametype != GT_POWERDUEL)
				{
					focus_item = Menu_FindItemByName(menu_hud, "score_line");

					if (focus_item)
					{
						CG_DrawScaledProportionalString(
							focus_item->window.rect.x,
							focus_item->window.rect.y,
							score_str,
							UI_RIGHT | UI_DROPSHADOW,
							focus_item->window.foreColor,
							0.5f);
					}
				}

				// Print scanline
				focus_item = Menu_FindItemByName(menu_hud, "scanline");

				if (focus_item)
				{
					trap->R_SetColor(colorTable[CT_WHITE]);
					CG_DrawPic(
						focus_item->window.rect.x + 4.5,
						focus_item->window.rect.y + 6,
						focus_item->window.rect.w,
						focus_item->window.rect.h,
						focus_item->window.background
					);
				}

				focus_item = Menu_FindItemByName(menu_hud, "frame");

				if (focus_item)
				{
					trap->R_SetColor(colorTable[CT_WHITE]);
					CG_DrawPic(
						focus_item->window.rect.x + 4.5,
						focus_item->window.rect.y + 6,
						focus_item->window.rect.w,
						focus_item->window.rect.h,
						focus_item->window.background
					);
				}

				if (cent->currentState.weapon == WP_SABER)
				{
					CG_DrawCusSaberStyle(cent, menu_hud);

					CG_DrawJK2blockingMode(cent, menu_hud);
					CG_DrawJK2SaberFatigue(cent, 560, 400);
				}
				else if (cent->currentState.weapon == WP_SABER && cent->currentState.saberHolstered)
				{
					CG_DrawCusgunfatigue(menu_hud);
					CG_DrawCusAmmo(cent, menu_hud);
				}
				else if (cent->currentState.weapon == WP_MELEE)
				{
					CG_DrawCusgunfatigue(menu_hud);
					CG_DrawCusweapontype(cent, menu_hud);
				}
				else
				{
					CG_DrawCusAmmo(cent, menu_hud);
					CG_DrawCusgunfatigue(menu_hud);
					CG_DrawCusweapontype(cent, menu_hud);
				}
				CG_DrawJK2ForcePower(cent, SCREEN_WIDTH - 80, SCREEN_HEIGHT - 80, menu_hud);
			}
		}
	}
}

#define MAX_SHOWPOWERS NUM_FORCE_POWERS

static qboolean ForcePower_Valid(const int i)
{
	if (i == FP_LEVITATION ||
		i == FP_SABER_OFFENSE ||
		i == FP_SABER_DEFENSE ||
		i == FP_SABERTHROW)
	{
		return qfalse;
	}

	if (cg.snap->ps.fd.forcePowersKnown & 1 << i)
	{
		return qtrue;
	}

	return qfalse;
}

/*
===================
CG_DrawForceSelect
===================
*/
void CG_DrawForceSelect(void)
{
	int i;
	int side_left_icon_cnt, side_right_icon_cnt;
	int icon_cnt;
	const int y_offset = 0;

	// don't display if dead
	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (cg.forceSelectTime + WEAPON_SELECT_TIME < cg.time) // Time is up for the HUD to display
	{
		cg.forceSelect = cg.snap->ps.fd.forcePowerSelected;
		return;
	}

	if (!cg.snap->ps.fd.forcePowersKnown)
	{
		return;
	}

	if (cg.predictedPlayerState.communicatingflags & (1 << CF_SABERLOCKING) && g_saberLockCinematicCamera.integer)
	{
		return;
	}

	// count the number of powers owned
	int count = 0;

	for (i = 0; i < NUM_FORCE_POWERS; ++i)
	{
		if (ForcePower_Valid(i))
		{
			count++;
		}
	}

	if (count == 0) // If no force powers, don't display
	{
		return;
	}

	const int side_max = 3; // Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	const int hold_count = count - 1; // -1 for the center icon
	if (hold_count == 0) // No icons to either side
	{
		side_left_icon_cnt = 0;
		side_right_icon_cnt = 0;
	}
	else if (count > 2 * side_max) // Go to the max on each side
	{
		side_left_icon_cnt = side_max;
		side_right_icon_cnt = side_max;
	}
	else // Less than max, so do the calc
	{
		side_left_icon_cnt = hold_count / 2;
		side_right_icon_cnt = hold_count - side_left_icon_cnt;
	}

	const int small_icon_size = 22;
	const int big_icon_size = 45;
	const int pad = 12;

	const int x = 320;
	const int y = 425;

	i = BG_ProperForceIndex(cg.forceSelect) - 1;
	if (i < 0)
	{
		i = MAX_SHOWPOWERS - 1;
	}

	trap->R_SetColor(NULL);
	// Work backwards from current icon
	int hold_x = x - (big_icon_size / 2 + pad + small_icon_size);
	for (icon_cnt = 1; icon_cnt < side_left_icon_cnt + 1; i--)
	{
		if (i < 0)
		{
			i = MAX_SHOWPOWERS - 1;
		}

		if (!ForcePower_Valid(forcePowerSorted[i])) // Does he have this power?
		{
			continue;
		}

		++icon_cnt; // Good icon

		if (cgs.media.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic(hold_x, y + y_offset, small_icon_size, small_icon_size,
				cgs.media.forcePowerIcons[forcePowerSorted[i]]);
			hold_x -= small_icon_size + pad;
		}
	}

	if (ForcePower_Valid(cg.forceSelect))
	{
		// Current Center Icon
		if (cgs.media.forcePowerIcons[cg.forceSelect])
		{
			CG_DrawPic(x - big_icon_size / 2, y - (big_icon_size - small_icon_size) / 2 + y_offset, big_icon_size,
				big_icon_size,
				cgs.media.forcePowerIcons[cg.forceSelect]); //only cache the icon for display
		}
	}

	i = BG_ProperForceIndex(cg.forceSelect) + 1;
	if (i >= MAX_SHOWPOWERS)
	{
		i = 0;
	}

	// Work forwards from current icon
	hold_x = x + big_icon_size / 2 + pad;
	for (icon_cnt = 1; icon_cnt < side_right_icon_cnt + 1; i++)
	{
		if (i >= MAX_SHOWPOWERS)
		{
			i = 0;
		}

		if (!ForcePower_Valid(forcePowerSorted[i])) // Does he have this power?
		{
			continue;
		}

		++icon_cnt; // Good icon

		if (cgs.media.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic(hold_x, y + y_offset, small_icon_size, small_icon_size,
				cgs.media.forcePowerIcons[forcePowerSorted[i]]); //only cache the icon for display
			hold_x += small_icon_size + pad;
		}
	}

	if (showPowersName[cg.forceSelect])
	{
		CG_DrawProportionalString(320, y + 30 + y_offset,
			CG_GetStringEdString("SP_INGAME", showPowersName[cg.forceSelect]),
			UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
	}
}

/*
===================
CG_DrawInventorySelect
===================
*/
void cg_draw_inventory_select(void)
{
	int i;
	int icon_cnt;
	int side_left_icon_cnt, side_right_icon_cnt;

	// don't display if dead
	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (cg.invenSelectTime + WEAPON_SELECT_TIME < cg.time) // Time is up for the HUD to display
	{
		return;
	}

	if (!cg.snap->ps.stats[STAT_HOLDABLE_ITEM] || !cg.snap->ps.stats[STAT_HOLDABLE_ITEMS])
	{
		return;
	}

	if (cg.predictedPlayerState.communicatingflags & (1 << CF_SABERLOCKING) && g_saberLockCinematicCamera.integer)
	{
		return;
	}

	if (cg.itemSelect == -1)
	{
		cg.itemSelect = bg_itemlist[cg.snap->ps.stats[STAT_HOLDABLE_ITEM]].giTag;
	}

	// count the number of items owned
	int count = 0;
	for (i = 0; i < HI_NUM_HOLDABLE; i++)
	{
		if (cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << i)
		{
			count++;
		}
	}

	if (!count)
	{
		const int y2 = 0; //err?
		CG_DrawProportionalString(320, y2 + 22, "EMPTY INVENTORY", UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
		return;
	}

	const int side_max = 3; // Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	const int hold_count = count - 1; // -1 for the center icon
	if (hold_count == 0) // No icons to either side
	{
		side_left_icon_cnt = 0;
		side_right_icon_cnt = 0;
	}
	else if (count > 2 * side_max) // Go to the max on each side
	{
		side_left_icon_cnt = side_max;
		side_right_icon_cnt = side_max;
	}
	else // Less than max, so do the calc
	{
		side_left_icon_cnt = hold_count / 2;
		side_right_icon_cnt = hold_count - side_left_icon_cnt;
	}

	i = cg.itemSelect - 1;
	if (i < 0)
	{
		i = HI_NUM_HOLDABLE - 1;
	}

	const int small_icon_size = 22;
	const int big_icon_size = 45;
	const int pad = 16;

	const int x = 320;
	const int y = 410;

	// Left side ICONS
	// Work backwards from current icon
	int hold_x = x - (big_icon_size / 2 + pad + small_icon_size);

	for (icon_cnt = 0; icon_cnt < side_left_icon_cnt; i--)
	{
		if (i < 0)
		{
			i = HI_NUM_HOLDABLE - 1;
		}

		if (!(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << i) || i == cg.itemSelect)
		{
			continue;
		}

		++icon_cnt; // Good icon

		if (!BG_IsItemSelectable(i))
		{
			continue;
		}

		if (cgs.media.invenIcons[i])
		{
			trap->R_SetColor(NULL);
			CG_DrawPic(hold_x, y + 10, small_icon_size, small_icon_size, cgs.media.invenIcons[i]);

			trap->R_SetColor(colorTable[CT_ICON_BLUE]);

			hold_x -= small_icon_size + pad;
		}
	}

	// Current Center Icon
	if (cgs.media.invenIcons[cg.itemSelect] && BG_IsItemSelectable(cg.itemSelect))
	{
		trap->R_SetColor(NULL);
		CG_DrawPic(x - big_icon_size / 2, y - (big_icon_size - small_icon_size) / 2 + 10, big_icon_size, big_icon_size,
			cgs.media.invenIcons[cg.itemSelect]);
		trap->R_SetColor(colorTable[CT_ICON_BLUE]);

		const int item_ndex = BG_GetItemIndexByTag(cg.itemSelect, IT_HOLDABLE);
		if (bg_itemlist[item_ndex].classname)
		{
			vec4_t text_color = { .312f, .75f, .621f, 1.0f };
			char text[1024];
			char upper_key[1024];

			strcpy(upper_key, bg_itemlist[item_ndex].classname);

			if (trap->SE_GetStringTextString(va("SP_INGAME_%s", Q_strupr(upper_key)), text, sizeof text)
				|| trap->SE_GetStringTextString(va("OJP_MENUS_%s", Q_strupr(upper_key)), text, sizeof text))
			{
				CG_DrawProportionalString(320, y + 45, text, UI_CENTER | UI_SMALLFONT, text_color);
			}
			else
			{
				CG_DrawProportionalString(320, y + 45, bg_itemlist[item_ndex].classname, UI_CENTER | UI_SMALLFONT,
					text_color);
			}
		}
	}

	i = cg.itemSelect + 1;
	if (i > HI_NUM_HOLDABLE - 1)
	{
		i = 0;
	}

	// Right side ICONS
	// Work forwards from current icon
	hold_x = x + big_icon_size / 2 + pad;
	for (icon_cnt = 0; icon_cnt < side_right_icon_cnt; i++)
	{
		if (i > HI_NUM_HOLDABLE - 1)
		{
			i = 0;
		}

		if (!(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & 1 << i) || i == cg.itemSelect)
		{
			continue;
		}

		++icon_cnt; // Good icon

		if (!BG_IsItemSelectable(i))
		{
			continue;
		}

		if (cgs.media.invenIcons[i])
		{
			trap->R_SetColor(NULL);
			CG_DrawPic(hold_x, y + 10, small_icon_size, small_icon_size, cgs.media.invenIcons[i]);

			trap->R_SetColor(colorTable[CT_ICON_BLUE]);

			hold_x += small_icon_size + pad;
		}
	}
}

int cg_targVeh = ENTITYNUM_NONE;
int cg_targVehLastTime = 0;

static qboolean CG_CheckTargetVehicle(centity_t** p_target_veh, float* alpha)
{
	int target_num = ENTITYNUM_NONE;
	centity_t* target_veh = NULL;

	if (!p_target_veh || !alpha)
	{
		//hey, where are my pointers?
		return qfalse;
	}

	*alpha = 1.0f;

	//FIXME: need to clear all of these when you die?
	if (cg.predictedPlayerState.rocketLockIndex < ENTITYNUM_WORLD)
	{
		target_num = cg.predictedPlayerState.rocketLockIndex;
	}
	else if (cg.crosshairVehNum < ENTITYNUM_WORLD
		&& cg.time - cg.crosshairVehTime < 3000)
	{
		//crosshair was on a vehicle in the last 3 seconds
		target_num = cg.crosshairVehNum;
	}
	else if (cg.crosshairclientNum < ENTITYNUM_WORLD)
	{
		target_num = cg.crosshairclientNum;
	}

	if (target_num < MAX_CLIENTS)
	{
		//real client
		if (cg_entities[target_num].currentState.m_iVehicleNum >= MAX_CLIENTS)
		{
			//in a vehicle
			target_num = cg_entities[target_num].currentState.m_iVehicleNum;
		}
	}
	if (target_num < ENTITYNUM_WORLD
		&& target_num >= MAX_CLIENTS)
	{
		target_veh = &cg_entities[target_num];
		if (target_veh->currentState.NPC_class == CLASS_VEHICLE
			&& target_veh->m_pVehicle
			&& target_veh->m_pVehicle->m_pVehicleInfo
			&& target_veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
		{
			//it's a vehicle
			cg_targVeh = target_num;
			cg_targVehLastTime = cg.time;
			*alpha = 1.0f;
		}
		else
		{
			target_veh = NULL;
		}
	}
	if (target_veh)
	{
		*p_target_veh = target_veh;
		return qtrue;
	}

	if (cg_targVehLastTime && cg.time - cg_targVehLastTime < 3000)
	{
		//stay at full alpha for 1 sec after lose them from crosshair
		if (cg.time - cg_targVehLastTime < 1000)
			*alpha = 1.0f;
		else //fade out over 2 secs
			*alpha = 1.0f - (cg.time - cg_targVehLastTime - 1000) / 2000.0f;
	}
	return qfalse;
}

#define MAX_VHUD_SHIELD_TICS 12
#define MAX_VHUD_SPEED_TICS 5
#define MAX_VHUD_ARMOR_TICS 5
#define MAX_VHUD_AMMO_TICS 5

static float CG_DrawVehicleShields(const menuDef_t* menu_hud, const centity_t* veh)
{
	vec4_t calc_color;

	const itemDef_t* item = Menu_FindItemByName((menuDef_t*)menu_hud, "armorbackground");

	if (item)
	{
		trap->R_SetColor(item->window.foreColor);
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);
	}

	const float max_shields = veh->m_pVehicle->m_pVehicleInfo->shields;
	float cur_value = cg.predictedVehicleState.stats[STAT_ARMOR];
	const float perc_shields = cur_value / (float)max_shields;
	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	const float inc = (float)max_shields / MAX_VHUD_ARMOR_TICS;
	for (int i = 1; i <= MAX_VHUD_ARMOR_TICS; i++)
	{
		char item_name[64];
		sprintf(item_name, "armor_tic%d", i);

		item = Menu_FindItemByName((menuDef_t*)menu_hud, item_name);

		if (!item)
		{
			continue;
		}

		memcpy(calc_color, item->window.foreColor, sizeof(vec4_t));

		if (cur_value <= 0) // don't show tic
		{
			break;
		}
		if (cur_value < inc) // partial tic (alpha it out)
		{
			const float percent = cur_value / inc;
			calc_color[3] *= percent; // Fade it out
		}

		trap->R_SetColor(calc_color);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);

		cur_value -= inc;
	}

	return perc_shields;
}

int cg_vehicleAmmoWarning = 0;
int cg_vehicleAmmoWarningTime = 0;

static void CG_DrawVehicleAmmo(const menuDef_t* menu_hud, const centity_t* veh)
{
	vec4_t calc_color;

	const itemDef_t* item = Menu_FindItemByName((menuDef_t*)menu_hud, "ammobackground");

	if (item)
	{
		trap->R_SetColor(item->window.foreColor);
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);
	}

	const float max_ammo = veh->m_pVehicle->m_pVehicleInfo->weapon[0].ammoMax;
	float curr_value = cg.predictedVehicleState.ammo[0];

	const float inc = (float)max_ammo / MAX_VHUD_AMMO_TICS;
	for (int i = 1; i <= MAX_VHUD_AMMO_TICS; i++)
	{
		char itemName[64];
		sprintf(itemName, "ammo_tic%d", i);

		item = Menu_FindItemByName((menuDef_t*)menu_hud, itemName);

		if (!item)
		{
			continue;
		}

		if (cg_vehicleAmmoWarningTime > cg.time
			&& cg_vehicleAmmoWarning == 0)
		{
			memcpy(calc_color, g_color_table[ColorIndex(COLOR_RED)], sizeof(vec4_t));
			calc_color[3] = sin(cg.time * 0.005) * 0.5f + 0.5f;
		}
		else
		{
			memcpy(calc_color, item->window.foreColor, sizeof(vec4_t));

			if (curr_value <= 0) // don't show tic
			{
				break;
			}
			if (curr_value < inc) // partial tic (alpha it out)
			{
				const float percent = curr_value / inc;
				calc_color[3] *= percent; // Fade it out
			}
		}

		trap->R_SetColor(calc_color);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);

		curr_value -= inc;
	}
}

static void CG_DrawVehicleAmmoUpper(const menuDef_t* menu_hud, const centity_t* veh)
{
	vec4_t calc_color;

	const itemDef_t* item = Menu_FindItemByName((menuDef_t*)menu_hud, "ammoupperbackground");

	if (item)
	{
		trap->R_SetColor(item->window.foreColor);
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);
	}

	const float max_ammo = veh->m_pVehicle->m_pVehicleInfo->weapon[0].ammoMax;
	float curr_value = cg.predictedVehicleState.ammo[0];

	const float inc = (float)max_ammo / MAX_VHUD_AMMO_TICS;
	for (int i = 1; i < MAX_VHUD_AMMO_TICS; i++)
	{
		char itemName[64];
		sprintf(itemName, "ammoupper_tic%d", i);

		item = Menu_FindItemByName((menuDef_t*)menu_hud, itemName);

		if (!item)
		{
			continue;
		}

		if (cg_vehicleAmmoWarningTime > cg.time
			&& cg_vehicleAmmoWarning == 0)
		{
			memcpy(calc_color, g_color_table[ColorIndex(COLOR_RED)], sizeof(vec4_t));
			calc_color[3] = sin(cg.time * 0.005) * 0.5f + 0.5f;
		}
		else
		{
			memcpy(calc_color, item->window.foreColor, sizeof(vec4_t));

			if (curr_value <= 0) // don't show tic
			{
				break;
			}
			if (curr_value < inc) // partial tic (alpha it out)
			{
				const float percent = curr_value / inc;
				calc_color[3] *= percent; // Fade it out
			}
		}

		trap->R_SetColor(calc_color);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);

		curr_value -= inc;
	}
}

static void CG_DrawVehicleAmmoLower(const menuDef_t* menu_hud, const centity_t* veh)
{
	vec4_t calc_color;

	const itemDef_t* item = Menu_FindItemByName((menuDef_t*)menu_hud, "ammolowerbackground");

	if (item)
	{
		trap->R_SetColor(item->window.foreColor);
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);
	}

	const float max_ammo = veh->m_pVehicle->m_pVehicleInfo->weapon[1].ammoMax;
	float curr_value = cg.predictedVehicleState.ammo[1];

	const float inc = (float)max_ammo / MAX_VHUD_AMMO_TICS;
	for (int i = 1; i < MAX_VHUD_AMMO_TICS; i++)
	{
		char item_name[64];
		sprintf(item_name, "ammolower_tic%d", i);

		item = Menu_FindItemByName((menuDef_t*)menu_hud, item_name);

		if (!item)
		{
			continue;
		}

		if (cg_vehicleAmmoWarningTime > cg.time
			&& cg_vehicleAmmoWarning == 1)
		{
			memcpy(calc_color, g_color_table[ColorIndex(COLOR_RED)], sizeof(vec4_t));
			calc_color[3] = sin(cg.time * 0.005) * 0.5f + 0.5f;
		}
		else
		{
			memcpy(calc_color, item->window.foreColor, sizeof(vec4_t));

			if (curr_value <= 0) // don't show tic
			{
				break;
			}
			if (curr_value < inc) // partial tic (alpha it out)
			{
				const float percent = curr_value / inc;
				calc_color[3] *= percent; // Fade it out
			}
		}

		trap->R_SetColor(calc_color);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);

		curr_value -= inc;
	}
}

// The HUD.menu file has the graphic print with a negative height, so it will print from the bottom up.
static void CG_DrawVehicleTurboRecharge(const menuDef_t* menu_hud, const centity_t* veh)
{
	const itemDef_t* item = Menu_FindItemByName((menuDef_t*)menu_hud, "turborecharge");

	if (item)
	{
		float percent;
		const int diff = cg.time - veh->m_pVehicle->m_iTurboTime;

		int height = item->window.rect.h;

		if (diff > veh->m_pVehicle->m_pVehicleInfo->turboRecharge)
		{
			percent = 1.0f;
			trap->R_SetColor(colorTable[CT_GREEN]);
		}
		else
		{
			percent = (float)diff / veh->m_pVehicle->m_pVehicleInfo->turboRecharge;
			if (percent < 0.0f)
			{
				percent = 0.0f;
			}
			trap->R_SetColor(colorTable[CT_RED]);
		}

		height *= percent;

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			height,
			cgs.media.whiteShader);
	}
}

qboolean cg_drawLink = qfalse;

static void CG_DrawVehicleWeaponsLinked(const menuDef_t* menu_hud, const centity_t* veh)
{
	qboolean draw_link = qfalse;
	if (veh->m_pVehicle
		&& veh->m_pVehicle->m_pVehicleInfo
		&& (veh->m_pVehicle->m_pVehicleInfo->weapon[0].linkable == 2 || veh->m_pVehicle->m_pVehicleInfo->weapon[1].
			linkable == 2))
	{
		//weapon is always linked
		draw_link = qtrue;
	}
	else
	{
		//MP way:
		//must get sent over network
		if (cg.predictedVehicleState.vehWeaponsLinked)
		{
			draw_link = qtrue;
		}
	}

	if (cg_drawLink != draw_link)
	{
		//state changed, play sound
		cg_drawLink = draw_link;
		trap->S_StartSound(NULL, cg.predictedPlayerState.clientNum, CHAN_LOCAL,
			trap->S_RegisterSound("sound/vehicles/common/linkweaps.wav"));
	}

	if (draw_link)
	{
		const itemDef_t* item = Menu_FindItemByName((menuDef_t*)menu_hud, "weaponslinked");

		if (item)
		{
			trap->R_SetColor(colorTable[CT_CYAN]);

			CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				cgs.media.whiteShader);
		}
	}
}

static void CG_DrawVehicleSpeed(const menuDef_t* menu_hud, const centity_t* veh)
{
	vec4_t calc_color;

	const itemDef_t* item = Menu_FindItemByName((menuDef_t*)menu_hud, "speedbackground");

	if (item)
	{
		trap->R_SetColor(item->window.foreColor);
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);
	}

	const float max_speed = veh->m_pVehicle->m_pVehicleInfo->speedMax;
	float curr_value = cg.predictedVehicleState.speed;

	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	const float inc = (float)max_speed / MAX_VHUD_SPEED_TICS;
	for (int i = 1; i <= MAX_VHUD_SPEED_TICS; i++)
	{
		char item_name[64];
		sprintf(item_name, "speed_tic%d", i);

		item = Menu_FindItemByName((menuDef_t*)menu_hud, item_name);

		if (!item)
		{
			continue;
		}

		if (cg.time > veh->m_pVehicle->m_iTurboTime)
		{
			memcpy(calc_color, item->window.foreColor, sizeof(vec4_t));
		}
		else // In turbo mode
		{
			if (cg.VHUDFlashTime < cg.time)
			{
				cg.VHUDFlashTime = cg.time + 200;
				if (cg.VHUDTurboFlag)
				{
					cg.VHUDTurboFlag = qfalse;
				}
				else
				{
					cg.VHUDTurboFlag = qtrue;
				}
			}

			if (cg.VHUDTurboFlag)
			{
				memcpy(calc_color, colorTable[CT_LTRED1], sizeof(vec4_t));
			}
			else
			{
				memcpy(calc_color, item->window.foreColor, sizeof(vec4_t));
			}
		}

		if (curr_value <= 0) // don't show tic
		{
			break;
		}
		if (curr_value < inc) // partial tic (alpha it out)
		{
			const float percent = curr_value / inc;
			calc_color[3] *= percent; // Fade it out
		}

		trap->R_SetColor(calc_color);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);

		curr_value -= inc;
	}
}

static void CG_DrawVehicleArmor(const menuDef_t* menu_hud, const centity_t* veh)
{
	vec4_t calc_color;

	const float max_armor = veh->m_pVehicle->m_pVehicleInfo->armor;
	float curr_value = cg.predictedVehicleState.stats[STAT_HEALTH];

	const itemDef_t* item = Menu_FindItemByName((menuDef_t*)menu_hud, "shieldbackground");

	if (item)
	{
		trap->R_SetColor(item->window.foreColor);
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);
	}

	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	const float inc = (float)max_armor / MAX_VHUD_SHIELD_TICS;
	for (int i = 1; i <= MAX_VHUD_SHIELD_TICS; i++)
	{
		char item_name[64];
		sprintf(item_name, "shield_tic%d", i);

		item = Menu_FindItemByName((menuDef_t*)menu_hud, item_name);

		if (!item)
		{
			continue;
		}

		memcpy(calc_color, item->window.foreColor, sizeof(vec4_t));

		if (curr_value <= 0) // don't show tic
		{
			break;
		}
		if (curr_value < inc) // partial tic (alpha it out)
		{
			const float percent = curr_value / inc;
			calc_color[3] *= percent; // Fade it out
		}

		trap->R_SetColor(calc_color);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);

		curr_value -= inc;
	}
}

enum
{
	VEH_DAMAGE_FRONT = 0,
	VEH_DAMAGE_BACK,
	VEH_DAMAGE_LEFT,
	VEH_DAMAGE_RIGHT,
};

typedef struct
{
	const char* itemName;
	short heavyDamage;
	short lightDamage;
} veh_damage_t;

veh_damage_t vehDamageData[4] =
{
	{"vehicle_front",SHIPSURF_DAMAGE_FRONT_HEAVY,SHIPSURF_DAMAGE_FRONT_LIGHT},
	{"vehicle_back",SHIPSURF_DAMAGE_BACK_HEAVY,SHIPSURF_DAMAGE_BACK_LIGHT},
	{"vehicle_left",SHIPSURF_DAMAGE_LEFT_HEAVY,SHIPSURF_DAMAGE_LEFT_LIGHT},
	{"vehicle_right",SHIPSURF_DAMAGE_RIGHT_HEAVY,SHIPSURF_DAMAGE_RIGHT_LIGHT},
};

// Draw health graphic for given part of vehicle
static void CG_DrawVehicleDamage(const centity_t* veh, const int broken_limbs, const menuDef_t* menu_hud, const float alpha,
	const int index)
{
	const itemDef_t* item = Menu_FindItemByName((menuDef_t*)menu_hud, vehDamageData[index].itemName);
	if (item)
	{
		vec4_t color;
		int graphic_handle = 0;
		int color_i;
		if (broken_limbs & 1 << vehDamageData[index].heavyDamage)
		{
			color_i = CT_RED;
			if (broken_limbs & 1 << vehDamageData[index].lightDamage)
			{
				color_i = CT_DKGREY;
			}
		}
		else if (broken_limbs & 1 << vehDamageData[index].lightDamage)
		{
			color_i = CT_YELLOW;
		}
		else
		{
			color_i = CT_GREEN;
		}

		VectorCopy4(colorTable[color_i], color);
		color[3] = alpha;
		trap->R_SetColor(color);

		switch (index)
		{
		case VEH_DAMAGE_FRONT:
			graphic_handle = veh->m_pVehicle->m_pVehicleInfo->iconFrontHandle;
			break;
		case VEH_DAMAGE_BACK:
			graphic_handle = veh->m_pVehicle->m_pVehicleInfo->iconBackHandle;
			break;
		case VEH_DAMAGE_LEFT:
			graphic_handle = veh->m_pVehicle->m_pVehicleInfo->iconLeftHandle;
			break;
		case VEH_DAMAGE_RIGHT:
			graphic_handle = veh->m_pVehicle->m_pVehicleInfo->iconRightHandle;
			break;
		default:;
		}

		if (graphic_handle)
		{
			CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				graphic_handle);
		}
	}
}

// Used on both damage indicators :  player vehicle and the vehicle the player is locked on
static void CG_DrawVehicleDamageHUD(const centity_t* veh, const int broken_limbs, const float perc_shields,
	const char* menu_name,
	const float alpha)
{
	vec4_t color;

	const menuDef_t* menu_hud = Menus_FindByName(menu_name);

	if (!menu_hud)
	{
		return;
	}

	const itemDef_t* item = Menu_FindItemByName(menu_hud, "background");
	if (item)
	{
		if (veh->m_pVehicle->m_pVehicleInfo->dmgIndicBackgroundHandle)
		{
			if (veh->damageTime > cg.time)
			{
				//ship shields currently taking damage
				//NOTE: cent->damageAngle can be accessed to get the direction from the ship origin to the impact point (in 3-D space)
				float perc = 1.0f - (veh->damageTime - cg.time) / 2000.0f/*MIN_SHIELD_TIME*/;
				if (perc < 0.0f)
				{
					perc = 0.0f;
				}
				else if (perc > 1.0f)
				{
					perc = 1.0f;
				}
				color[0] = item->window.foreColor[0]; //flash red
				color[1] = item->window.foreColor[1] * perc; //fade other colors back in over time
				color[2] = item->window.foreColor[2] * perc; //fade other colors back in over time
				color[3] = item->window.foreColor[3]; //always normal alpha
				trap->R_SetColor(color);
			}
			else
			{
				trap->R_SetColor(item->window.foreColor);
			}

			CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				veh->m_pVehicle->m_pVehicleInfo->dmgIndicBackgroundHandle);
		}
	}

	item = Menu_FindItemByName(menu_hud, "outer_frame");
	if (item)
	{
		if (veh->m_pVehicle->m_pVehicleInfo->dmgIndicFrameHandle)
		{
			trap->R_SetColor(item->window.foreColor);
			CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				veh->m_pVehicle->m_pVehicleInfo->dmgIndicFrameHandle);
		}
	}

	item = Menu_FindItemByName(menu_hud, "shields");
	if (item)
	{
		if (veh->m_pVehicle->m_pVehicleInfo->dmgIndicShieldHandle)
		{
			VectorCopy4(colorTable[CT_HUD_GREEN], color);
			color[3] = perc_shields;
			trap->R_SetColor(color);
			CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				veh->m_pVehicle->m_pVehicleInfo->dmgIndicShieldHandle);
		}
	}

	//FIXME: when ship explodes, either stop drawing ship or draw all parts black
	CG_DrawVehicleDamage(veh, broken_limbs, menu_hud, alpha, VEH_DAMAGE_FRONT);
	CG_DrawVehicleDamage(veh, broken_limbs, menu_hud, alpha, VEH_DAMAGE_BACK);
	CG_DrawVehicleDamage(veh, broken_limbs, menu_hud, alpha, VEH_DAMAGE_LEFT);
	CG_DrawVehicleDamage(veh, broken_limbs, menu_hud, alpha, VEH_DAMAGE_RIGHT);
}

static qboolean CG_DrawVehicleHud(const centity_t* cent)
{
	centity_t* veh;
	float alpha;

	const menuDef_t* menu_hud = Menus_FindByName("swoopvehiclehud");
	if (!menu_hud)
	{
		return qtrue; // Draw player HUD
	}

	const playerState_t* ps = &cg.predictedPlayerState;

	if (!ps || !ps->m_iVehicleNum)
	{
		return qtrue; // Draw player HUD
	}
	veh = &cg_entities[ps->m_iVehicleNum];

	if (!veh || !veh->m_pVehicle)
	{
		return qtrue; // Draw player HUD
	}

	CG_DrawVehicleTurboRecharge(menu_hud, veh);
	CG_DrawVehicleWeaponsLinked(menu_hud, veh);

	const itemDef_t* item = Menu_FindItemByName(menu_hud, "leftframe");

	// Draw frame
	if (item)
	{
		trap->R_SetColor(item->window.foreColor);
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);
	}

	item = Menu_FindItemByName(menu_hud, "rightframe");

	if (item)
	{
		trap->R_SetColor(item->window.foreColor);
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);
	}

	CG_DrawVehicleArmor(menu_hud, veh);

	CG_DrawVehicleSpeed(menu_hud, veh);

	const float shieldPerc = CG_DrawVehicleShields(menu_hud, veh);

	if (veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID && !veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID)
	{
		CG_DrawVehicleAmmo(menu_hud, veh);
	}
	else if (veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID && veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID)
	{
		CG_DrawVehicleAmmoUpper(menu_hud, veh);
		CG_DrawVehicleAmmoLower(menu_hud, veh);
	}

	// If he's hidden, he must be in a vehicle
	if (veh->m_pVehicle->m_pVehicleInfo->hideRider)
	{
		CG_DrawVehicleDamageHUD(veh, cg.predictedVehicleState.brokenLimbs, shieldPerc, "vehicledamagehud", 1.0f);

		// Has he targeted an enemy?
		if (CG_CheckTargetVehicle(&veh, &alpha))
		{
			CG_DrawVehicleDamageHUD(veh, veh->currentState.brokenLimbs,
				(float)veh->currentState.activeForcePass / 10.0f, "enemyvehicledamagehud", alpha);
		}

		return qfalse; // Don't draw player HUD
	}

	return qtrue; // Draw player HUD
}

/*
================
CG_DrawStats

================
*/
static void CG_DrawStats(void)
{
	qboolean drawHUD = qtrue;

	const centity_t* cent = &cg_entities[cg.snap->ps.clientNum];

	if (cent)
	{
		const playerState_t* ps = &cg.predictedPlayerState;

		if (ps->m_iVehicleNum) // In a vehicle???
		{
			drawHUD = CG_DrawVehicleHud(cent);
		}
	}

	if (drawHUD)
	{
		CG_DrawHUD(cent);
	}
}

/*
===================
CG_DrawPickupItem
===================
*/
#define	PICKUP_ICON_SIZE 22

static void CG_DrawPickupItem(void)
{
	const int value = cg.itemPickup;
	if (value && cg_items[value].icon != -1)
	{
		const float* fadeColor = CG_FadeColor(cg.itemPickupTime, 3000);
		if (fadeColor)
		{
			CG_RegisterItemVisuals(value);
			trap->R_SetColor(fadeColor);
			CG_DrawPic(290, 20, PICKUP_ICON_SIZE, PICKUP_ICON_SIZE, cg_items[value].icon);
			trap->R_SetColor(NULL);
		}
	}
}

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground(const int x, const int y, const int w, const int h, const float alpha, const int team)
{
	vec4_t hcolor;

	hcolor[3] = alpha;
	if (team == TEAM_RED)
	{
		hcolor[0] = 1;
		hcolor[1] = .2f;
		hcolor[2] = .2f;
	}
	else if (team == TEAM_BLUE)
	{
		hcolor[0] = .2f;
		hcolor[1] = .2f;
		hcolor[2] = 1;
	}
	else
	{
		return;
	}
	//	trap->R_SetColor( hcolor );

	CG_FillRect(x, y, w, h, hcolor);
	//	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap->R_SetColor(NULL);
}

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawMiniScoreboard
================
*/
static float CG_DrawMiniScoreboard(float y)
{
	if (!cg_drawScores.integer)
	{
		return y;
	}

	if (cgs.gametype == GT_SIEGE)
	{
		//don't bother with this in siege
		return y;
	}

	if (cgs.gametype >= GT_TEAM)
	{
		const int x_offset = 0;
		char temp[MAX_QPATH];
		Q_strncpyz(temp, va("%s: ", CG_GetStringEdString("MP_INGAME", "RED")), sizeof temp);
		Q_strcat(temp, sizeof temp, cgs.scores1 == SCORE_NOT_PRESENT ? "-" : va("%i", cgs.scores1));
		Q_strcat(temp, sizeof temp, va(" %s: ", CG_GetStringEdString("MP_INGAME", "BLUE")));
		Q_strcat(temp, sizeof temp, cgs.scores2 == SCORE_NOT_PRESENT ? "-" : va("%i", cgs.scores2));

		CG_Text_Paint(630 - CG_Text_Width(temp, 0.7f, FONT_MEDIUM) + x_offset, y, 0.7f, colorWhite, temp, 0, 0,
			ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
		y += 15;
	}
	else
	{
		//rww - no longer doing this. Since the attacker now shows who is first, we print the score there.
	}

	return y;
}

/*
================
CG_DrawEnemyInfo
================
*/
static float CG_DrawEnemyInfo(float y)
{
	float size;
	int clientNum;
	const char* title;
	const int x_offset = 0;

	if (!cg.snap)
	{
		return y;
	}

	if (!cg_drawEnemyInfo.integer)
	{
		return y;
	}

	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0)
	{
		return y;
	}

	if (cgs.gametype == GT_POWERDUEL)
	{
		//just get out of here then
		return y;
	}

	if (cgs.gametype == GT_JEDIMASTER)
	{
		//title = "Jedi Master";
		title = CG_GetStringEdString("MP_INGAME", "MASTERY7");
		clientNum = cgs.jediMaster;

		if (clientNum < 0)
		{
			//return y;
			//			title = "Get Saber!";
			title = CG_GetStringEdString("MP_INGAME", "GET_SABER");

			size = ICON_SIZE * 1.25;
			y += 5;

			CG_DrawPic(640 - size - 12 + x_offset, y, size, size, cgs.media.weaponIcons[WP_SABER]);

			y += size;

			CG_Text_Paint(630 - CG_Text_Width(title, 0.7f, FONT_MEDIUM) + x_offset, y, 0.7f, colorWhite, title, 0, 0, 0,
				FONT_MEDIUM);

			return y + BIGCHAR_HEIGHT + 2;
		}
	}
	else if (cg.snap->ps.duelInProgress)
	{
		//		title = "Dueling";
		title = CG_GetStringEdString("MP_INGAME", "DUELING");
		clientNum = cg.snap->ps.duelIndex;
	}
	else if (cgs.gametype == GT_DUEL && cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR)
	{
		title = CG_GetStringEdString("MP_INGAME", "DUELING");
		if (cg.snap->ps.clientNum == cgs.duelist1)
		{
			clientNum = cgs.duelist2; //if power duel, should actually draw both duelists 2 and 3 I guess
		}
		else if (cg.snap->ps.clientNum == cgs.duelist2)
		{
			clientNum = cgs.duelist1;
		}
		else if (cg.snap->ps.clientNum == cgs.duelist3)
		{
			clientNum = cgs.duelist1;
		}
		else
		{
			return y;
		}
	}
	else
	{
		//As of current, we don't want to draw the attacker. Instead, draw whoever is in first place.
		if (cgs.duelWinner < 0 || cgs.duelWinner >= MAX_CLIENTS)
		{
			return y;
		}

		title = va("%s: %i", CG_GetStringEdString("MP_INGAME", "LEADER"), cgs.scores1);

		clientNum = cgs.duelWinner;
	}

	if (clientNum >= MAX_CLIENTS || !&cgs.clientinfo[clientNum])
	{
		return y;
	}

	const clientInfo_t* ci = &cgs.clientinfo[clientNum];

	size = ICON_SIZE * 1.25;
	y += 5;

	if (ci->modelIcon)
	{
		CG_DrawPic(640 - size - 5 + x_offset, y, size, size, ci->modelIcon);
	}

	y += size;

	//	CG_Text_Paint( 630 - CG_Text_Width ( ci->name, 0.7f, FONT_MEDIUM ) + xOffset, y, 0.7f, colorWhite, ci->name, 0, 0, 0, FONT_MEDIUM );
	CG_Text_Paint(630 - CG_Text_Width(ci->name, 1.0f, FONT_SMALL2) + x_offset, y, 1.0f, colorWhite, ci->name, 0, 0, 0,
		FONT_SMALL2);

	y += 15;
	//	CG_Text_Paint( 630 - CG_Text_Width ( title, 0.7f, FONT_MEDIUM ) + xOffset, y, 0.7f, colorWhite, title, 0, 0, 0, FONT_MEDIUM );
	CG_Text_Paint(630 - CG_Text_Width(title, 1.0f, FONT_SMALL2) + x_offset, y, 1.0f, colorWhite, title, 0, 0, 0,
		FONT_SMALL2);

	if ((cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) && cgs.clientinfo[cg.snap->ps.clientNum].team !=
		TEAM_SPECTATOR)
	{
		//also print their score
		char text[1024];
		y += 15;
		Com_sprintf(text, sizeof text, "%i/%i", cgs.clientinfo[clientNum].score, cgs.fraglimit);
		CG_Text_Paint(630 - CG_Text_Width(text, 0.7f, FONT_MEDIUM) + x_offset, y, 0.7f, colorWhite, text, 0, 0, 0,
			FONT_MEDIUM);
	}

	// nmckenzie: DUEL_HEALTH - fixme - need checks and such here.  And this is coded to duelist 1 right now, which is wrongly.
	if (cgs.showDuelHealths >= 2)
	{
		y += 15;
		if (cgs.duelist1 == clientNum)
		{
			CG_DrawDuelistHealth(640 - size - 5 + x_offset, y, 64, 8, 1);
		}
		else if (cgs.duelist2 == clientNum)
		{
			CG_DrawDuelistHealth(640 - size - 5 + x_offset, y, 64, 8, 2);
		}
	}

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot(const float y)
{
	const int x_offset = 0;

	const char* s = va("time:%i snap:%i cmd:%i", cg.snap->serverTime,
		cg.latestSnapshotNum, cgs.serverCommandSequence);
	const int w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

	CG_DrawBigString(635 - w + x_offset, y + 2, s, 1.0f);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	16

static float CG_DrawFPS(const float y)
{
	static unsigned short previousTimes[FPS_FRAMES];
	static int previous, lastupdate;
	const int x_offset = 0;

	if (cg.predictedPlayerState.communicatingflags & (1 << CF_SABERLOCKING) && g_saberLockCinematicCamera.integer)
	{
		return y;
	}

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	const int t = trap->Milliseconds();
	const unsigned short frameTime = t - previous;
	previous = t;
	if (t - lastupdate > 50) //don't sample faster than this
	{
		static unsigned short index;
		lastupdate = t;
		previousTimes[index % FPS_FRAMES] = frameTime;
		index++;
	}
	// average multiple frames together to smooth changes out a bit
	int total = 0;
	for (int i = 0; i < FPS_FRAMES; i++)
	{
		total += previousTimes[i];
	}
	if (!total)
	{
		total = 1;
	}
	const int fps = 1000 * FPS_FRAMES / total;

	const char* s = va("%ifps", fps);
	const int w = CG_DrawStrlen(s) * 8;

	CG_DrawSmallString(635 - w + x_offset, y + 2, s, 1.0f);

	return y + BIGCHAR_HEIGHT + 4;
}

// nmckenzie: DUEL_HEALTH
#define MAX_HEALTH_FOR_IFACE	100

static void CG_DrawHealthBarRough(const float x, const float y, const int width, const int height, const float ratio,
	const float* color1,
	const float* color2)
{
	float color3[4] = { 1, 0, 0, .7f };

	const float midpoint = width * ratio - 1;
	const float remainder = width - midpoint;
	color3[0] = color1[0] * 0.5f;

	assert(!(height % 4)); //this won't line up otherwise.
	CG_DrawRect(x + 1, y + height / 2 - 1, midpoint, 1, height / 4 + 1, color1); // creme-y filling.
	CG_DrawRect(x + midpoint, y + height / 2 - 1, remainder, 1, height / 4 + 1, color3); // used-up-ness.
	CG_DrawRect(x, y, width, height, 1, color2); // hard crispy shell
}

void CG_DrawDuelistHealth(const float x, const float y, const float w, const float h, const int duelist)
{
	float duel_health_color[4] = { 1, 0, 0, 0.7f };
	float health_src = 0.0f;

	if (duelist == 1)
	{
		health_src = cgs.duelist1health;
	}
	else if (duelist == 2)
	{
		health_src = cgs.duelist2health;
	}

	float ratio = health_src / MAX_HEALTH_FOR_IFACE;
	if (ratio > 1.0f)
	{
		ratio = 1.0f;
	}
	if (ratio < 0.0f)
	{
		ratio = 0.0f;
	}
	duel_health_color[0] = ratio * 0.2f + 0.5f;

	CG_DrawHealthBarRough(x, y, w, h, ratio, duel_health_color, colorTable[CT_WHITE]);
	// new art for this?  I'm not crazy about how this looks.
}

/*
=====================
CG_DrawRadar
=====================
*/
float cg_radarRange = 2500.0f;

#define RADAR_RADIUS			40
#define RADAR_X					SCREEN_WIDTH - 2 * RADAR_RADIUS
#define RADAR_RADIUS_X			40
static int radarLockSoundDebounceTime = 0;
static int impactSoundDebounceTime = 0;
#define	RADAR_MISSILE_RANGE					3000.0f
#define	RADAR_ASTEROID_RANGE				10000.0f
#define	RADAR_MIN_ASTEROID_SURF_WARN_DIST	1200.0f

static float cg_draw_radar(float y)
{
	vec4_t color;
	vec4_t team_color;
	float arrow_w;
	float arrow_h;
	clientInfo_t* cl;
	clientInfo_t* local;
	float arrow_base_scale;
	float z_scale;
	int i;
	int x_offset = 0;
	int team;

	if (!cg.snap)
	{
		return y;
	}

	// Make sure the radar should be showing
	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return y;
	}

	if (cg.predictedPlayerState.pm_flags & PMF_FOLLOW || cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return y;
	}

	if (cg.predictedPlayerState.communicatingflags & (1 << CF_SABERLOCKING) && g_saberLockCinematicCamera.integer)
	{
		return y;
	}

	team = cg.snap->ps.persistant[PERS_TEAM];
	local = &cgs.clientinfo[cg.snap->ps.clientNum];

	if (!local->infoValid)
	{
		return y;
	}

	// Draw the radar background image
	color[0] = color[1] = color[2] = 1.0f;
	color[3] = 0.6f;
	trap->R_SetColor(color);

	CG_DrawPic(RADAR_X + x_offset + 5, y + 5, RADAR_RADIUS_X * 2 - 11, RADAR_RADIUS * 2 - 11, cgs.media.radarScanShader);
	CG_DrawPic(RADAR_X + x_offset, y, RADAR_RADIUS * 2, RADAR_RADIUS * 2, cgs.media.radarShader);

	if (cgs.gametype == GT_FFA)
	{
		VectorCopy(g_color_table[ColorIndex(COLOR_RED)], team_color);
		team_color[3] = 1.0f;
	}
	else if (cgs.gametype == GT_TEAM || cgs.gametype == GT_CTF)
	{
		VectorCopy(g_color_table[ColorIndex(COLOR_ORANGE)], team_color);
		team_color[3] = 1.0f;
	}
	else if (cgs.gametype == GT_SINGLE_PLAYER)
	{
		VectorCopy(g_color_table[ColorIndex(COLOR_RED)], team_color);
		team_color[3] = 1.0f;
	}
	else
	{
		VectorCopy(g_color_table[ColorIndex(COLOR_MAGENTA)], team_color);
		team_color[3] = 1.0f;
	}

	// Draw all of the radar entities.  Draw them backwards so players are drawn last
	for (i = cg.radarEntityCount - 1; i >= 0; i--)
	{
		vec3_t dir_look;
		vec3_t dir_player;
		float angle_look;
		float angle_player;
		float angle;
		float distance, actual_dist;
		centity_t* cent;
		qboolean far_away = qfalse;

		cent = &cg_entities[cg.radarEntities[i]];

		// Get the distances first
		VectorSubtract(cg.predictedPlayerState.origin, cent->lerpOrigin, dir_player);
		dir_player[2] = 0;
		actual_dist = distance = VectorNormalize(dir_player);

		if (distance > cg_radarRange * 0.8f)
		{
			if (cent->currentState.eFlags & EF_RADAROBJECT //still want to draw the direction
				|| cent->currentState.eType == ET_NPC //FIXME: draw last, with players...
				&& cent->currentState.NPC_class == CLASS_VEHICLE
				&& cent->currentState.speed > 0) //always draw vehicles
			{
				distance = cg_radarRange * 0.8f;
				far_away = qtrue;
			}
			else
			{
				continue;
			}
		}

		distance = distance / cg_radarRange;
		distance *= RADAR_RADIUS;

		AngleVectors(cg.predictedPlayerState.viewangles, dir_look, NULL, NULL);

		dir_look[2] = 0;
		angle_player = atan2(dir_player[0], dir_player[1]);
		VectorNormalize(dir_look);
		angle_look = atan2(dir_look[0], dir_look[1]);
		angle = angle_look - angle_player;

		switch (cent->currentState.eType)
		{
		default:
		{
			float x;
			float ly;
			qhandle_t shader;

			x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin(angle) * distance;
			ly = y + (float)RADAR_RADIUS + (float)cos(angle) * distance;

			arrow_base_scale = 6.0f;
			shader = 0;
			z_scale = 1.0f;

			if (!far_away)
			{
				//we want to scale the thing up/down based on the relative Z (up/down) positioning
				if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
				{
					//higher, scale up (between 16 and 24)
					float dif = cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2];

					//max out to 1.5x scale at 512 units above local player's height
					dif /= 1024.0f;
					if (dif > 0.5f)
					{
						dif = 0.5f;
					}
					z_scale += dif;
				}
				else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
				{
					//lower, scale down (between 16 and 8)
					float dif = cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2];

					//half scale at 512 units below local player's height
					dif /= 1024.0f;
					if (dif > 0.5f)
					{
						dif = 0.5f;
					}
					z_scale -= dif;
				}
			}

			arrow_base_scale *= z_scale;

			if (cent->currentState.brokenLimbs)
			{
				//slightly misleading to use this value, but don't want to add more to entstate.
				//any ent with brokenLimbs non-0 and on radar is an objective ent.
				//brokenLimbs is literal team value.
				char obj_state[1024];
				int complete;

				//we only want to draw it if the objective for it is not complete.
				//frame represents objective num.
				trap->Cvar_VariableStringBuffer(
					va("team%i_objective%i", cent->currentState.brokenLimbs, cent->currentState.frame), obj_state,
					1024);

				complete = atoi(obj_state);

				if (!complete)
				{
					// generic enemy index specifies a shader to use for the radar entity.
					if (cent->currentState.genericenemyindex && cent->currentState.genericenemyindex < MAX_ICONS)
					{
						color[0] = color[1] = color[2] = color[3] = 1.0f;
						shader = cgs.gameIcons[cent->currentState.genericenemyindex];
					}
					else
					{
						if (cg.snap && cent->currentState.brokenLimbs == cg.snap->ps.persistant[PERS_TEAM])
						{
							VectorCopy(g_color_table[ColorIndex(COLOR_RED)], color);
						}
						else
						{
							VectorCopy(g_color_table[ColorIndex(COLOR_GREEN)], color);
						}

						shader = cgs.media.siegeItemShader;
					}
				}
			}
			else
			{
				color[0] = color[1] = color[2] = color[3] = 1.0f;

				// generic enemy index specifies a shader to use for the radar entity.
				if (cent->currentState.genericenemyindex)
				{
					shader = cgs.gameIcons[cent->currentState.genericenemyindex];
				}
				else
				{
					shader = cgs.media.siegeItemShader;
				}
			}

			if (shader)
			{
				// Pulse the alpha if time2 is set.  time2 gets set when the entity takes pain
				if (cent->currentState.time2 && cg.time - cent->currentState.time2 < 5000 ||
					cent->currentState.time2 == 0xFFFFFFFF)
				{
					if (cg.time / 200 & 1)
					{
						color[3] = 0.1f + 0.9f * (float)(cg.time % 200) / 200.0f;
					}
					else
					{
						color[3] = 1.0f - 0.9f * (float)(cg.time % 200) / 200.0f;
					}
				}

				trap->R_SetColor(color);
				CG_DrawPic(x - 4 + x_offset, ly - 4, arrow_base_scale, arrow_base_scale, shader);
			}
		}
		break;

		case ET_NPC:
			if (cent->currentState.NPC_class == CLASS_VEHICLE)
			{
				if (cent->m_pVehicle && cent->m_pVehicle->m_pVehicleInfo->radarIconHandle)
				{
					float x;
					float ly;

					x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin(angle) * distance;
					ly = y + (float)RADAR_RADIUS + (float)cos(angle) * distance;

					arrow_base_scale = 6.0f;
					z_scale = 1.0f;

					//we want to scale the thing up/down based on the relative Z (up/down) positioning
					if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
					{
						//higher, scale up (between 16 and 24)
						float dif = cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2];

						//max out to 1.5x scale at 512 units above local player's height
						dif /= 4096.0f;
						if (dif > 0.5f)
						{
							dif = 0.5f;
						}
						z_scale += dif;
					}
					else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
					{
						//lower, scale down (between 16 and 8)
						float dif = cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2];

						//half scale at 512 units below local player's height
						dif /= 4096.0f;
						if (dif > 0.5f)
						{
							dif = 0.5f;
						}
						z_scale -= dif;
					}

					arrow_base_scale *= z_scale;

					if (cent->currentState.m_iVehicleNum //vehicle has a driver
						&& cgs.clientinfo[cent->currentState.m_iVehicleNum - 1].infoValid)
					{
						if (cgs.clientinfo[cent->currentState.m_iVehicleNum - 1].team == local->team)
						{
							trap->R_SetColor(team_color);
						}
						else
						{
							trap->R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
						}
					}
					else
					{
						trap->R_SetColor(NULL);
					}
					CG_DrawPic(x - 4 + x_offset, ly - 4, arrow_base_scale, arrow_base_scale,
						cent->m_pVehicle->m_pVehicleInfo->radarIconHandle);
				}
			}
			else
			{
				// Normal NPCs...
				vec4_t colour;

				VectorCopy4(team_color, colour);

				arrow_base_scale = 8.0f;
				z_scale = 1.0f;

				// Pulse the radar icon after a voice message
				if (cent->vChatTime + 2000 > cg.time)
				{
					float f = (cent->vChatTime + 2000 - cg.time) / 3000.0f;
					arrow_base_scale = 8.0f + 4.0f * f;
					colour[0] = team_color[0] + (1.0f - team_color[0]) * f;
					colour[1] = team_color[1] + (1.0f - team_color[1]) * f;
					colour[2] = team_color[2] + (1.0f - team_color[2]) * f;
				}

				trap->R_SetColor(colour);

				//we want to scale the thing up/down based on the relative Z (up/down) positioning
				if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
				{
					//higher, scale up (between 16 and 32)
					float dif = cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2];

					//max out to 2x scale at 1024 units above local player's height
					dif /= 1024.0f;
					if (dif > 1.0f)
					{
						dif = 1.0f;
					}
					z_scale += dif;
				}
				else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
				{
					//lower, scale down (between 16 and 8)
					float dif = cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2];

					//half scale at 512 units below local player's height
					dif /= 1024.0f;
					if (dif > 0.5f)
					{
						dif = 0.5f;
					}
					z_scale -= dif;
				}

				arrow_base_scale *= z_scale;

				arrow_w = arrow_base_scale * RADAR_RADIUS / 128;
				arrow_h = arrow_base_scale * RADAR_RADIUS / 128;

				CG_DrawRotatePic2(RADAR_X + RADAR_RADIUS + sin(angle) * distance + x_offset,
					y + RADAR_RADIUS + cos(angle) * distance,
					arrow_w, arrow_h,
					360 - cent->lerpAngles[YAW] + cg.predictedPlayerState.viewangles[YAW],
					cgs.media.mAutomapPlayerIcon);
				break;
			}
			break; //maybe do something?

		case ET_MOVER:
			if (cent->currentState.speed //the mover's size, actually
				&& actual_dist < cent->currentState.speed + RADAR_ASTEROID_RANGE
				&& cg.predictedPlayerState.m_iVehicleNum)
			{
				//a mover that's close to me and I'm in a vehicle
				qboolean may_impact = qfalse;
				float surface_dist = actual_dist - cent->currentState.speed;
				if (surface_dist < 0.0f)
				{
					surface_dist = 0.0f;
				}
				if (surface_dist < RADAR_MIN_ASTEROID_SURF_WARN_DIST)
				{
					//always warn!
					may_impact = qtrue;
				}
				else
				{
					//not close enough to always warn, yet, so check its direction
					int predict_time, timeStep = 500;
					float new_dist;
					for (predict_time = timeStep; predict_time < 5000; predict_time += timeStep)
					{
						vec3_t move_dir;
						vec3_t my_pos;
						vec3_t asteroid_pos;
						//asteroid dir, speed, size, + my dir & speed...
						BG_EvaluateTrajectory(&cent->currentState.pos, cg.time + predict_time, asteroid_pos);
						//FIXME: I don't think it's calcing "myPos" correctly
						AngleVectors(cg.predictedVehicleState.viewangles, move_dir, NULL, NULL);
						VectorMA(cg.predictedVehicleState.origin,
							cg.predictedVehicleState.speed * predict_time / 1000.0f, move_dir, my_pos);
						new_dist = Distance(my_pos, asteroid_pos);
						if (new_dist - cent->currentState.speed <= RADAR_MIN_ASTEROID_SURF_WARN_DIST) //200.0f )
						{
							//heading for an impact within the next 5 seconds
							may_impact = qtrue;
							break;
						}
					}
				}
				if (may_impact)
				{
					//possible collision
					vec4_t asteroid_color = { 0.5f, 0.5f, 0.5f, 1.0f };
					float x;
					float ly;
					float asteroid_scale = cent->currentState.speed / 2000.0f; //average asteroid radius?
					if (actual_dist > RADAR_ASTEROID_RANGE)
					{
						actual_dist = RADAR_ASTEROID_RANGE;
					}
					distance = actual_dist / RADAR_ASTEROID_RANGE * RADAR_RADIUS;

					x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin(angle) * distance;
					ly = y + (float)RADAR_RADIUS + (float)cos(angle) * distance;

					if (asteroid_scale > 3.0f)
					{
						asteroid_scale = 3.0f;
					}
					else if (asteroid_scale < 0.2f)
					{
						asteroid_scale = 0.2f;
					}
					arrow_base_scale = 6.0f * asteroid_scale;
					if (impactSoundDebounceTime < cg.time)
					{
						vec3_t soundOrg;
						if (surface_dist > RADAR_ASTEROID_RANGE * 0.66f)
						{
							impactSoundDebounceTime = cg.time + 1000;
						}
						else if (surface_dist > RADAR_ASTEROID_RANGE / 3.0f)
						{
							impactSoundDebounceTime = cg.time + 400;
						}
						else
						{
							impactSoundDebounceTime = cg.time + 100;
						}
						VectorMA(cg.refdef.vieworg, -500.0f * (surface_dist / RADAR_ASTEROID_RANGE), dir_player,
							soundOrg);
						trap->S_StartSound(soundOrg, ENTITYNUM_WORLD, CHAN_AUTO,
							trap->S_RegisterSound("sound/vehicles/common/impactalarm.wav"));
					}
					//brighten it the closer it is
					if (surface_dist > RADAR_ASTEROID_RANGE * 0.66f)
					{
						asteroid_color[0] = asteroid_color[1] = asteroid_color[2] = 0.7f;
					}
					else if (surface_dist > RADAR_ASTEROID_RANGE / 3.0f)
					{
						asteroid_color[0] = asteroid_color[1] = asteroid_color[2] = 0.85f;
					}
					else
					{
						asteroid_color[0] = asteroid_color[1] = asteroid_color[2] = 1.0f;
					}
					//alpha out the longer it's been since it was considered dangerous
					if (cg.time - impactSoundDebounceTime > 100)
					{
						asteroid_color[3] = (float)(cg.time - impactSoundDebounceTime - 100) / 900.0f;
					}

					trap->R_SetColor(asteroid_color);
					CG_DrawPic(x - 4 + x_offset, ly - 4, arrow_base_scale, arrow_base_scale,
						trap->R_RegisterShaderNoMip("gfx/menus/radar/asteroid"));
				}
			}
			break;

		case ET_MISSILE:
			if (cent->currentState.owner > MAX_CLIENTS //belongs to an NPC
				&& cg_entities[cent->currentState.owner].currentState.NPC_class == CLASS_VEHICLE)
			{
				//a rocket belonging to an NPC, FIXME: only tracking rockets!
				float x;
				float ly;

				x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin(angle) * distance;
				ly = y + (float)RADAR_RADIUS + (float)cos(angle) * distance;

				arrow_base_scale = 3.0f;
				if (cg.predictedPlayerState.m_iVehicleNum)
				{
					//I'm in a vehicle
					//if it's targetting me, then play an alarm sound if I'm in a vehicle
					if (cent->currentState.otherentity_num == cg.predictedPlayerState.clientNum || cent->currentState.
						otherentity_num == cg.predictedPlayerState.m_iVehicleNum)
					{
						if (radarLockSoundDebounceTime < cg.time)
						{
							vec3_t sound_org;
							int alarm_sound;
							if (actual_dist > RADAR_MISSILE_RANGE * 0.66f)
							{
								radarLockSoundDebounceTime = cg.time + 1000;
								arrow_base_scale = 3.0f;
								alarm_sound = trap->S_RegisterSound("sound/vehicles/common/lockalarm1.wav");
							}
							else if (actual_dist > RADAR_MISSILE_RANGE / 3.0f)
							{
								radarLockSoundDebounceTime = cg.time + 500;
								arrow_base_scale = 6.0f;
								alarm_sound = trap->S_RegisterSound("sound/vehicles/common/lockalarm2.wav");
							}
							else
							{
								radarLockSoundDebounceTime = cg.time + 250;
								arrow_base_scale = 9.0f;
								alarm_sound = trap->S_RegisterSound("sound/vehicles/common/lockalarm3.wav");
							}
							if (actual_dist > RADAR_MISSILE_RANGE)
							{
								actual_dist = RADAR_MISSILE_RANGE;
							}
							VectorMA(cg.refdef.vieworg, -500.0f * (actual_dist / RADAR_MISSILE_RANGE), dir_player,
								sound_org);
							trap->S_StartSound(sound_org, ENTITYNUM_WORLD, CHAN_AUTO, alarm_sound);
						}
					}
				}
				z_scale = 1.0f;

				//we want to scale the thing up/down based on the relative Z (up/down) positioning
				if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
				{
					//higher, scale up (between 16 and 24)
					float dif = cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2];

					//max out to 1.5x scale at 512 units above local player's height
					dif /= 1024.0f;
					if (dif > 0.5f)
					{
						dif = 0.5f;
					}
					z_scale += dif;
				}
				else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
				{
					//lower, scale down (between 16 and 8)
					float dif = cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2];

					//half scale at 512 units below local player's height
					dif /= 1024.0f;
					if (dif > 0.5f)
					{
						dif = 0.5f;
					}
					z_scale -= dif;
				}

				arrow_base_scale *= z_scale;

				if (cent->currentState.owner >= MAX_CLIENTS //missile owned by an NPC
					&& cg_entities[cent->currentState.owner].currentState.NPC_class == CLASS_VEHICLE //NPC is a vehicle
					&& cg_entities[cent->currentState.owner].currentState.m_iVehicleNum <= MAX_CLIENTS
					//Vehicle has a player driver
					&& cgs.clientinfo[cg_entities[cent->currentState.owner].currentState.m_iVehicleNum - 1].infoValid)
					//player driver is valid
				{
					cl = &cgs.clientinfo[cg_entities[cent->currentState.owner].currentState.m_iVehicleNum - 1];
					if (cl->team == local->team)
					{
						trap->R_SetColor(team_color);
					}
					else
					{
						trap->R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
					}
				}
				else
				{
					trap->R_SetColor(NULL);
				}
				CG_DrawPic(x - 4 + x_offset, ly - 4, arrow_base_scale, arrow_base_scale, cgs.media.mAutomapRocketIcon);
			}
			break;

		case ET_PLAYER:
		{
			vec4_t colour;

			cl = &cgs.clientinfo[cent->currentState.number];

			// not valid then dont draw it
			if (!cl->infoValid)
			{
				continue;
			}

			VectorCopy4(team_color, colour);

			arrow_base_scale = 8.0f;
			z_scale = 1.0f;

			// Pulse the radar icon after a voice message
			if (cent->vChatTime + 2000 > cg.time)
			{
				float f = (cent->vChatTime + 2000 - cg.time) / 3000.0f;
				arrow_base_scale = 8.0f + 4.0f * f;
				colour[0] = team_color[0] + (1.0f - team_color[0]) * f;
				colour[1] = team_color[1] + (1.0f - team_color[1]) * f;
				colour[2] = team_color[2] + (1.0f - team_color[2]) * f;
			}

			trap->R_SetColor(colour);

			if (!far_away)
			{
				//we want to scale the thing up/down based on the relative Z (up/down) positioning
				if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
				{
					//higher, scale up (between 16 and 32)
					float dif = cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2];

					//max out to 2x scale at 1024 units above local player's height
					dif /= 1024.0f;
					if (dif > 1.0f)
					{
						dif = 1.0f;
					}
					z_scale += dif;
				}
				else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
				{
					//lower, scale down (between 16 and 8)
					float dif = cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2];

					//half scale at 512 units below local player's height
					dif /= 1024.0f;
					if (dif > 0.5f)
					{
						dif = 0.5f;
					}
					z_scale -= dif;
				}
			}

			arrow_base_scale *= z_scale;

			arrow_w = arrow_base_scale * RADAR_RADIUS / 128;
			arrow_h = arrow_base_scale * RADAR_RADIUS / 128;

			CG_DrawRotatePic2(RADAR_X + RADAR_RADIUS + sin(angle) * distance + x_offset, y + RADAR_RADIUS + cos(angle) * distance, arrow_w, arrow_h, 360 - cent->lerpAngles[YAW] + cg.predictedPlayerState.viewangles[YAW], cgs.media.mAutomapPlayerIcon);
			break;
		}
		}
	}

	arrow_base_scale = 8.0f;

	arrow_w = arrow_base_scale * RADAR_RADIUS / 128;
	arrow_h = arrow_base_scale * RADAR_RADIUS / 128;

	trap->R_SetColor(colorWhite);
	CG_DrawRotatePic2(RADAR_X + RADAR_RADIUS + x_offset, y + RADAR_RADIUS, arrow_w, arrow_h, 0, cgs.media.mAutomapPlayerIcon);

	return y + RADAR_RADIUS * 2;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer(const float y)
{
	const int x_offset = 0;

	const int msec = cg.time - cgs.levelStartTime;

	int seconds = msec / 1000;
	const int mins = seconds / 60;
	seconds -= mins * 60;
	const int tens = seconds / 10;
	seconds -= tens * 10;

	if (cg.predictedPlayerState.communicatingflags & (1 << CF_SABERLOCKING) && g_saberLockCinematicCamera.integer)
	{
		return y;
	}

	const char* s = va("%i:%i%i", mins, tens, seconds);
	const int w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

	CG_DrawBigString(635 - w + x_offset, y + 2, s, 1.0f);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTeamOverlay
=================
*/
extern const char* CG_GetLocationString(const char* loc); //cg_main.c
static float CG_DrawTeamOverlay(float y, const qboolean right, const qboolean upper)
{
	int x;
	int i, len;
	const char* p;
	vec4_t hcolor;
	clientInfo_t* ci;
	int ret_y;
	const int x_offset = 0;

	if (!cg_drawTeamOverlay.integer)
	{
		return y;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE)
	{
		return y; // Not on any team
	}

	int plyrs = 0;

	//		Find a way to detect invalid info and return early?

	// max player name width
	int pwidth = 0;
	const int count = numSortedTeamPlayers > 8 ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++)
	{
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if (ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM])
		{
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	int lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++)
	{
		p = CG_GetLocationString(CG_ConfigString(CS_LOCATIONS + i));
		if (p && *p)
		{
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	const int w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if (right)
		x = 640 - w;
	else
		x = 0;

	const int h = plyrs * TINYCHAR_HEIGHT;

	if (upper)
	{
		ret_y = y + h;
	}
	else
	{
		y -= h;
		ret_y = y;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
	{
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	}
	else
	{
		// if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	trap->R_SetColor(hcolor);
	CG_DrawPic(x + x_offset, y, w, h, cgs.media.teamStatusBar);
	trap->R_SetColor(NULL);

	for (i = 0; i < count; i++)
	{
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if (ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM])
		{
			char st[16];
			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			int xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt(xx + x_offset, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth)
			{
				p = CG_GetLocationString(CG_ConfigString(CS_LOCATIONS + ci->location));
				if (!p || !*p)
					p = "unknown";
				len = CG_DrawStrlen(p);

				if (len > lwidth)
				{
					//len = lwidth;
				}

				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt(xx + x_offset, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth(ci->health, ci->armor, hcolor);

			Com_sprintf(st, sizeof st, "%3i %3i", ci->health, ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 +
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt(xx + x_offset, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if (cg_weapons[ci->curWeapon].weaponIcon)
			{
				CG_DrawPic(xx + x_offset, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cg_weapons[ci->curWeapon].weaponIcon);
			}
			else
			{
				CG_DrawPic(xx + x_offset, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cgs.media.deferShader);
			}

			// Draw powerup icons
			if (right)
			{
				xx = x;
			}
			else
			{
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (int j = 0; j < PW_NUM_POWERUPS; j++)
			{
				if (ci->powerups & 1 << j)
				{
					const gitem_t* item = BG_FindItemForPowerup(j);

					if (item)
					{
						CG_DrawPic(xx + x_offset, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
							trap->R_RegisterShader(item->icon));
						if (right)
						{
							xx -= TINYCHAR_WIDTH;
						}
						else
						{
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
	//#endif
}

static void CG_DrawPowerupIcons(int y)
{
	trap->R_SetColor(NULL);

	if (!cg.snap)
	{
		return;
	}

	y += 16;

	for (int j = 0; j < PW_NUM_POWERUPS; j++)
	{
		if (cg.snap->ps.powerups[j] > cg.time)
		{
			const int secondsleft = (cg.snap->ps.powerups[j] - cg.time) / 1000;

			const gitem_t* item = BG_FindItemForPowerup(j);

			if (item)
			{
				const int x_offset = 0;
				const int ico_size = 64;
				int ico_shader;
				if (cgs.gametype == GT_CTY && (j == PW_REDFLAG || j == PW_BLUEFLAG))
				{
					if (j == PW_REDFLAG)
					{
						ico_shader = trap->R_RegisterShaderNoMip("gfx/hud/mpi_rflag_ys");
					}
					else
					{
						ico_shader = trap->R_RegisterShaderNoMip("gfx/hud/mpi_bflag_ys");
					}
				}
				else
				{
					ico_shader = trap->R_RegisterShader(item->icon);
				}

				CG_DrawPic(640 - ico_size * 1.1 + x_offset, y, ico_size, ico_size, ico_shader);

				y += ico_size;

				if (j != PW_REDFLAG && j != PW_BLUEFLAG && secondsleft < 999)
				{
					CG_DrawProportionalString(640 - ico_size * 1.1 + ico_size / 2 + x_offset, y - 8,
						va("%i", secondsleft), UI_CENTER | UI_BIGFONT | UI_DROPSHADOW,
						colorTable[CT_WHITE]);
				}

				y += ico_size / 3;
			}
		}
	}
}

/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight(void)
{
	float y = 0;

	trap->R_SetColor(colorTable[CT_WHITE]);

	if (cgs.gametype >= GT_SINGLE_PLAYER && cg_drawTeamOverlay.integer == 1)
	{
		y = CG_DrawTeamOverlay(y, qtrue, qtrue);
	}
	if (cg_drawSnapshot.integer)
	{
		y = CG_DrawSnapshot(y);
	}

	if (cg_drawFPS.integer)
	{
		y = CG_DrawFPS(y);
	}
	if (cg_drawTimer.integer)
	{
		y = CG_DrawTimer(y);
	}
	if (cg_drawRadar.integer)
	{
		//draw Radar
		y = cg_draw_radar(y);
	}

	y = CG_DrawEnemyInfo(y);

	y = CG_DrawMiniScoreboard(y);

	CG_DrawPowerupIcons(y);
}

/*
===================
CG_DrawReward
===================
*/
//#ifdef JK2AWARDS
//static void CG_DrawReward(void)
//{
//	int i;
//	float x, y;
//	char buf[32];
//
//	if (!cg_drawRewards.integer)
//	{
//		return;
//	}
//
//	const float* color = CG_FadeColor(cg.rewardTime, REWARD_TIME);
//	if (!color)
//	{
//		if (cg.rewardStack > 0)
//		{
//			for (i = 0; i < cg.rewardStack; i++)
//			{
//				cg.rewardSound[i] = cg.rewardSound[i + 1];
//				cg.rewardShader[i] = cg.rewardShader[i + 1];
//				cg.rewardCount[i] = cg.rewardCount[i + 1];
//			}
//			cg.rewardTime = cg.time;
//			cg.rewardStack--;
//			color = CG_FadeColor(cg.rewardTime, REWARD_TIME);
//			trap->S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
//		}
//		else
//		{
//			return;
//		}
//	}
//
//	trap->R_SetColor(color);
//
//	if (cg.rewardCount[0] >= 10)
//	{
//		y = 56;
//		x = 320 - ICON_SIZE / 2;
//		CG_DrawPic(x, y, ICON_SIZE - 4, ICON_SIZE - 4, cg.rewardShader[0]);
//		Com_sprintf(buf, sizeof buf, "%d", cg.rewardCount[0]);
//		x = (SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen(buf)) / 2;
//		CG_DrawStringExt(x, y + ICON_SIZE, buf, color, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);
//	}
//	else
//	{
//		const int count = cg.rewardCount[0];
//
//		y = 56;
//		x = 320 - count * ICON_SIZE / 2;
//		for (i = 0; i < count; i++)
//		{
//			CG_DrawPic(x, y, ICON_SIZE - 4, ICON_SIZE - 4, cg.rewardShader[0]);
//			x += ICON_SIZE;
//		}
//	}
//	trap->R_SetColor(NULL);
//}

//#endif

/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128

struct lagometer_s
{
	int frameSamples[LAG_SAMPLES];
	int frameCount;
	int snapshotFlags[LAG_SAMPLES];
	int snapshotSamples[LAG_SAMPLES];
	int snapshotCount;
} lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo(void)
{
	const int offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[lagometer.frameCount & LAG_SAMPLES - 1] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo(const snapshot_t* snap)
{
	// dropped packet
	if (!snap)
	{
		lagometer.snapshotSamples[lagometer.snapshotCount & LAG_SAMPLES - 1] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[lagometer.snapshotCount & LAG_SAMPLES - 1] = snap->ping;
	lagometer.snapshotFlags[lagometer.snapshotCount & LAG_SAMPLES - 1] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something different for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect(void)
{
	usercmd_t cmd;
	const char* s;
	int w; // bk010215 - FIXME char message[1024];

	if (cg.mMapChange)
	{
		s = CG_GetStringEdString("MP_INGAME", "SERVER_CHANGING_MAPS"); // s = "Server Changing Maps";
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
		CG_DrawBigString(320 - w / 2, 100, s, 1.0f);

		s = CG_GetStringEdString("MP_INGAME", "PLEASE_WAIT"); // s = "Please wait...";
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
		CG_DrawBigString(320 - w / 2, 200, s, 1.0f);
		return;
	}

	// draw the phone jack if we are completely past our buffers
	const int cmd_num = trap->GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap->GetUserCmd(cmd_num, &cmd);
	if (cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time)
	{
		// special check for map_restart // bk 0102165 - FIXME
		return;
	}

	// also add text in center of screen
	s = CG_GetStringEdString("MP_INGAME", "CONNECTION_INTERRUPTED");
	// s = "Connection Interrupted"; // bk 010215 - FIXME
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 100, s, 1.0f);

	// blink the icon
	if (cg.time >> 9 & 1)
	{
		return;
	}

	const float x = 640 - 48;
	const float y = 480 - 48;

	CG_DrawPic(x, y, 48, 48, trap->R_RegisterShader("gfx/2d/net.tga"));
}

#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer(void)
{
	int a, i;
	float v;

	if (!cg_lagometer.integer || cgs.localServer)
	{
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
	const int x = 640 - 48;
	const int y = 480 - 144;

	trap->R_SetColor(NULL);
	CG_DrawPic(x, y, 48, 48, cgs.media.lagometerShader);

	const float ax = x;
	const float ay = y;
	const float aw = 48;
	const float ah = 48;

	int color = -1;
	float range = ah / 3;
	const float mid = ay + range;

	float vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for (a = 0; a < aw; a++)
	{
		i = lagometer.frameCount - 1 - a & LAG_SAMPLES - 1;
		v = lagometer.frameSamples[i];
		v *= vscale;
		if (v > 0)
		{
			if (color != 1)
			{
				color = 1;
				trap->R_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)]);
			}
			if (v > range)
			{
				v = range;
			}
			trap->R_DrawStretchPic(ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if (v < 0)
		{
			if (color != 2)
			{
				color = 2;
				trap->R_SetColor(g_color_table[ColorIndex(COLOR_BLUE)]);
			}
			v = -v;
			if (v > range)
			{
				v = range;
			}
			trap->R_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for (a = 0; a < aw; a++)
	{
		i = lagometer.snapshotCount - 1 - a & LAG_SAMPLES - 1;
		v = lagometer.snapshotSamples[i];
		if (v > 0)
		{
			if (lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED)
			{
				if (color != 5)
				{
					color = 5; // YELLOW for rate delay
					trap->R_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)]);
				}
			}
			else
			{
				if (color != 3)
				{
					color = 3;
					trap->R_SetColor(g_color_table[ColorIndex(COLOR_GREEN)]);
				}
			}
			v = v * vscale;
			if (v > range)
			{
				v = range;
			}
			trap->R_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if (v < 0)
		{
			if (color != 4)
			{
				color = 4; // RED for dropped snapshots
				trap->R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
			}
			trap->R_DrawStretchPic(ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	trap->R_SetColor(NULL);

	if (cg_noPredict.integer || g_synchronousClients.integer)
	{
		CG_DrawBigString(ax, ay, "snc", 1.0);
	}

	CG_DrawDisconnect();
}

void CG_DrawSiegeMessage(const char* str, const int objective_screen)
{
	//	if (!( trap->Key_GetCatcher() & KEYCATCH_UI ))
	{
		trap->OpenUIMenu(UIMENU_CLOSEALL);
		trap->Cvar_Set("cg_siegeMessage", str);
		if (objective_screen)
		{
			trap->OpenUIMenu(UIMENU_SIEGEOBJECTIVES);
		}
		else
		{
			trap->OpenUIMenu(UIMENU_SIEGEMESSAGE);
		}
	}
}

void CG_DrawSiegeMessageNonMenu(const char* str)
{
	if (str[0] == '@')
	{
		char text[1024];
		trap->SE_GetStringTextString(str + 1, text, sizeof text);
		str = text;
	}
	CG_CenterPrint(str, SCREEN_HEIGHT * 0.20, BIGCHAR_WIDTH);
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint(const char* str, const int y, const int char_width)
{
	int i = 0;

	Q_strncpyz(cg.centerPrint, str, sizeof cg.centerPrint);

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = char_width;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	char* s = cg.centerPrint;
	while (*s)
	{
		i++;
		if (i >= 50)
		{
			//maxed out a line of text, this will make the line spill over onto another line.
			i = 0;
			cg.centerPrintLines++;
		}
		else if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}

/*
===================
CG_DrawCenterString
===================
*/

static void CG_DrawCenterString(void)
{
	int l;

	if (!cg.centerPrintTime)
	{
		return;
	}

	float* color = CG_FadeColor(cg.centerPrintTime, 1000 * cg_centerTime.value);

	if (!color)
	{
		return;
	}

	trap->R_SetColor(color);

	char* start = cg.centerPrint;

	int y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while (1)
	{
		const float scale = Q_max(cg_textprintscale.value, 0.0f);
		char linebuffer[1024];

		for (l = 0; l < 50; l++)
		{
			if (!start[l] || start[l] == '\n')
			{
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		if (!BG_IsWhiteSpace(start[l]) && !BG_IsWhiteSpace(linebuffer[l - 1]))
		{
			//we might have cut a word off, attempt to find a spot where we won't cut words off at.
			const int saved_l = l;
			int counter = l - 2;

			for (; counter >= 0; counter--)
			{
				if (BG_IsWhiteSpace(start[counter]))
				{
					//this location is whitespace, line break from this position
					linebuffer[counter] = 0;
					l = counter + 1;
					break;
				}
			}
			if (counter < 0)
			{
				//couldn't find a break in the text, just go ahead and cut off the word mid-word.
				l = saved_l;
			}
		}

		const int w = CG_Text_Width(linebuffer, scale, FONT_MEDIUM);
		const int h = CG_Text_Height(linebuffer, scale, FONT_MEDIUM);
		const int x = (SCREEN_WIDTH - w) / 2;
		CG_Text_Paint(x, y + h, scale, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
		y += h + 6;

		//this method of advancing to new line from the start of the array was causing long lines without
		//new lines to be totally truncated.
		if (start[l] && start[l] == '\n')
		{
			//next char is a newline, advance past
			l++;
		}

		if (!start[l])
		{
			//end of string, we're done.
			break;
		}

		//advance pointer to the last character that we didn't read in.
		start = &start[l];
	}

	trap->R_SetColor(NULL);
}

/*
================================================================================

CROSSHAIR

================================================================================
*/

#define HEALTH_WIDTH		50.0f
#define HEALTH_HEIGHT		5.0f

//see if we can draw some extra info on this guy based on our class
static void CG_DrawSiegeInfo(const centity_t* cent, const float chX, const float chY, const float ch_w, const float ch_h)
{
	const siegeExtended_t* se = &cg_siegeExtendedData[cent->currentState.number];
	vec4_t aColor;
	vec4_t cColor;

	assert(cent->currentState.number < MAX_CLIENTS);

	if (se->lastUpdated > cg.time)
	{
		//strange, shouldn't happen
		return;
	}

	if (cg.time - se->lastUpdated > 10000)
	{
		//if you haven't received a status update on this guy in 10 seconds, forget about it
		return;
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{
		//he's dead, don't display info on him
		return;
	}

	if (cent->currentState.weapon != se->weapon)
	{
		//data is invalidated until it syncs back again
		return;
	}

	const clientInfo_t* ci = &cgs.clientinfo[cent->currentState.number];
	if (ci->team != cg.predictedPlayerState.persistant[PERS_TEAM])
	{
		//not on the same team
		return;
	}

	const char* configstring = CG_ConfigString(cg.predictedPlayerState.clientNum + CS_PLAYERS);
	const char* v = Info_ValueForKey(configstring, "siegeclass");

	if (!v || !v[0])
	{
		//don't have siege class in info?
		return;
	}

	const siegeClass_t* siege_class = BG_SiegeFindClassByName(v);

	if (!siege_class)
	{
		//invalid
		return;
	}

	if (!(siege_class->classflags & 1 << CFL_STATVIEWER))
	{
		//doesn't really have the ability to see others' stats
		return;
	}

	float x = chX + (ch_w / 2 - HEALTH_WIDTH / 2);
	float y = chY + ch_h + 8.0f;
	float percent = (float)se->health / (float)se->maxhealth * HEALTH_WIDTH;

	//color of the bar
	aColor[0] = 0.0f;
	aColor[1] = 1.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.4f;

	//color of greyed out "missing health"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.4f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH, HEALTH_HEIGHT, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x + 1.0f, y + 1.0f, percent - 1.0f, HEALTH_HEIGHT - 1.0f, aColor);

	//then draw the other part greyed out
	CG_FillRect(x + percent, y + 1.0f, HEALTH_WIDTH - percent - 1.0f, HEALTH_HEIGHT - 1.0f, cColor);

	//now draw his ammo
	int ammo_max = ammoData[weaponData[cent->currentState.weapon].ammoIndex].max;
	if (cent->currentState.eFlags & EF_DOUBLE_AMMO)
	{
		ammo_max *= 2;
	}

	x = chX + (ch_w / 2 - HEALTH_WIDTH / 2);
	y = chY + ch_h + HEALTH_HEIGHT + 10.0f;

	if (!weaponData[cent->currentState.weapon].energyPerShot &&
		!weaponData[cent->currentState.weapon].altEnergyPerShot)
	{
		//a weapon that takes no ammo, so show full
		percent = HEALTH_WIDTH;
	}
	else
	{
		percent = (float)se->ammo / (float)ammo_max * HEALTH_WIDTH;
	}

	//color of the bar
	aColor[0] = 1.0f;
	aColor[1] = 1.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.4f;

	//color of greyed out "missing health"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.4f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH, HEALTH_HEIGHT, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x + 1.0f, y + 1.0f, percent - 1.0f, HEALTH_HEIGHT - 1.0f, aColor);

	//then draw the other part greyed out
	CG_FillRect(x + percent, y + 1.0f, HEALTH_WIDTH - percent - 1.0f, HEALTH_HEIGHT - 1.0f, cColor);
}

//draw the health bar based on current "health" and maxhealth
static void CG_DrawHealthBar(const centity_t* cent, const float chX, const float chY, const float ch_w, const float ch_h)
{
	vec4_t aColor;
	vec4_t cColor;
	const float x = chX + (ch_w / 2 - HEALTH_WIDTH / 2);
	const float y = chY + ch_h + 8.0f;
	const float percent = (float)cent->currentState.health / (float)cent->currentState.maxhealth * HEALTH_WIDTH;

	if (percent <= 0)
	{
		return;
	}

	//color of the bar
	if (!cent->currentState.teamowner || cgs.gametype < GT_TEAM)
	{
		//not owned by a team or teamplay
		aColor[0] = 1.0f;
		aColor[1] = 1.0f;
		aColor[2] = 0.0f;
		aColor[3] = 0.4f;
	}
	else if (cent->currentState.teamowner == cg.predictedPlayerState.persistant[PERS_TEAM])
	{
		//owned by my team
		aColor[0] = 0.0f;
		aColor[1] = 1.0f;
		aColor[2] = 0.0f;
		aColor[3] = 0.4f;
	}
	else
	{
		//hostile
		aColor[0] = 1.0f;
		aColor[1] = 0.0f;
		aColor[2] = 0.0f;
		aColor[3] = 0.4f;
	}

	//color of greyed out "missing health"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.4f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH, HEALTH_HEIGHT, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x + 1.0f, y + 1.0f, percent - 1.0f, HEALTH_HEIGHT - 1.0f, aColor);

	//then draw the other part greyed out
	CG_FillRect(x + percent, y + 1.0f, HEALTH_WIDTH - percent - 1.0f, HEALTH_HEIGHT - 1.0f, cColor);
}

#define HACK_WIDTH		33.5f
#define HACK_HEIGHT		3.5f
//same routine (at least for now), draw progress of a "hack" or whatever
static void CG_DrawHaqrBar(const float chX, const float chY, const float chW, const float chH)
{
	vec4_t aColor;
	vec4_t cColor;
	const float x = chX + (chW / 2 - HACK_WIDTH / 2);
	const float y = chY + chH + 8.0f;
	const float percent = ((float)cg.predictedPlayerState.hackingTime - (float)cg.time) / (float)cg.predictedPlayerState.hackingBaseTime * HACK_WIDTH;

	if (percent > HACK_WIDTH ||
		percent < 1.0f)
	{
		return;
	}

	//color of the bar
	aColor[0] = 1.0f;
	aColor[1] = 1.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.4f;

	//color of greyed out done area
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, HACK_WIDTH, HACK_HEIGHT, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x + 1.0f, y + 1.0f, percent - 1.0f, HACK_HEIGHT - 1.0f, aColor);

	//then draw the other part greyed out
	CG_FillRect(x + percent, y + 1.0f, HACK_WIDTH - percent - 1.0f, HACK_HEIGHT - 1.0f, cColor);

	//draw the hacker icon
	CG_DrawPic(x, y - HACK_WIDTH, HACK_WIDTH, HACK_WIDTH, cgs.media.hackerIconShader);
}

//generic timing bar
int cg_genericTimerBar = 0;
int cg_genericTimerDur = 0;
vec4_t cg_genericTimerColor;
#define CGTIMERBAR_H			50.0f
#define CGTIMERBAR_W			10.0f
#define CGTIMERBAR_X			(SCREEN_WIDTH-CGTIMERBAR_W-120.0f)
#define CGTIMERBAR_Y			(SCREEN_HEIGHT-CGTIMERBAR_H-20.0f)

static void CG_DrawGenericTimerBar(void)
{
	vec4_t aColor;
	vec4_t cColor;
	const float x = CGTIMERBAR_X;
	const float y = CGTIMERBAR_Y;
	float percent = (float)(cg_genericTimerBar - cg.time) / (float)cg_genericTimerDur * CGTIMERBAR_H;

	if (percent > CGTIMERBAR_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//color of the bar
	aColor[0] = cg_genericTimerColor[0];
	aColor[1] = cg_genericTimerColor[1];
	aColor[2] = cg_genericTimerColor[2];
	aColor[3] = cg_genericTimerColor[3];

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, CGTIMERBAR_W, CGTIMERBAR_H, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x + 1.0f, y + 1.0f + (CGTIMERBAR_H - percent), CGTIMERBAR_W - 2.0f,
		CGTIMERBAR_H - 1.0f - (CGTIMERBAR_H - percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x + 1.0f, y + 1.0f, CGTIMERBAR_W - 2.0f, CGTIMERBAR_H - percent, cColor);
}

/*
=================
CG_DrawCrosshair
=================
*/

float cg_crosshairPrevPosX = 0;
float cg_crosshairPrevPosY = 0;
#define CRAZY_CROSSHAIR_MAX_ERROR_X	(100.0f*640.0f/480.0f)
#define CRAZY_CROSSHAIR_MAX_ERROR_Y	(100.0f)

static void CG_LerpCrosshairPos(float* x, float* y)
{
	if (cg_crosshairPrevPosX)
	{
		//blend from old pos
		float max_move = 30.0f * ((float)cg.frametime / 500.0f) * 640.0f / 480.0f;
		const float x_diff = *x - cg_crosshairPrevPosX;
		if (fabs(x_diff) > CRAZY_CROSSHAIR_MAX_ERROR_X)
		{
			max_move = CRAZY_CROSSHAIR_MAX_ERROR_X;
		}
		if (x_diff > max_move)
		{
			*x = cg_crosshairPrevPosX + max_move;
		}
		else if (x_diff < -max_move)
		{
			*x = cg_crosshairPrevPosX - max_move;
		}
	}
	cg_crosshairPrevPosX = *x;

	if (cg_crosshairPrevPosY)
	{
		//blend from old pos
		float max_move = 30.0f * ((float)cg.frametime / 500.0f);
		const float y_diff = *y - cg_crosshairPrevPosY;
		if (fabs(y_diff) > CRAZY_CROSSHAIR_MAX_ERROR_Y)
		{
			max_move = CRAZY_CROSSHAIR_MAX_ERROR_X;
		}
		if (y_diff > max_move)
		{
			*y = cg_crosshairPrevPosY + max_move;
		}
		else if (y_diff < -max_move)
		{
			*y = cg_crosshairPrevPosY - max_move;
		}
	}
	cg_crosshairPrevPosY = *y;
}

vec3_t cg_crosshairPos = { 0, 0, 0 };
extern qboolean in_camera;

static void CG_DrawCrosshair(vec3_t world_point, const int ch_ent_valid)
{
	float w, h;
	qhandle_t hShader = 0;
	float x, y;
	qboolean corona = qfalse;
	vec4_t ecolor = { 0, 0, 0, 0 };
	const centity_t* crossEnt = NULL;

	if (in_camera)
	{
		return;
	}

	if (cg.snap->ps.weapon == WP_MELEE || cg.snap->ps.weapon == WP_NONE)
	{
		return;
	}

	if (cg.predictedPlayerState.communicatingflags & (1 << CF_SABERLOCKING) && g_saberLockCinematicCamera.integer)
	{
		return;
	}

	if (world_point)
	{
		VectorCopy(world_point, cg_crosshairPos);
	}

	if (!cg_drawCrosshair.integer)
	{
		return;
	}

	if (cg.snap->ps.fallingToDeath)
	{
		return;
	}

	if (cg.predictedPlayerState.zoomMode != 0)
	{
		//not while scoped
		return;
	}

	if (cg_crosshairHealth.integer)
	{
		vec4_t hcolor;

		CG_ColorForHealth(hcolor);
		trap->R_SetColor(hcolor);
	}
	else
	{
		//set color based on what kind of ent is under crosshair
		if (cg.crosshairclientNum >= ENTITYNUM_WORLD)
		{
			trap->R_SetColor(NULL);
		}
		else if (ch_ent_valid &&
			(cg_entities[cg.crosshairclientNum].currentState.number < MAX_CLIENTS ||
				cg_entities[cg.crosshairclientNum].currentState.eType == ET_NPC ||
				cg_entities[cg.crosshairclientNum].currentState.shouldtarget ||
				cg_entities[cg.crosshairclientNum].currentState.health ||
				//always show ents with health data under crosshair
				cg_entities[cg.crosshairclientNum].currentState.eType == ET_MOVER && cg_entities[cg.
				crosshairclientNum].
				currentState.bolt1 && cg.predictedPlayerState.weapon == WP_SABER ||
				cg_entities[cg.crosshairclientNum].currentState.eType == ET_MOVER && cg_entities[cg.
				crosshairclientNum].
				currentState.teamowner))
		{
			crossEnt = &cg_entities[cg.crosshairclientNum];

			if (crossEnt->currentState.powerups & 1 << PW_CLOAKED)
			{
				//don't show up for cloaked guys
				ecolor[0] = 1.0; //R
				ecolor[1] = 1.0; //G
				ecolor[2] = 1.0; //B
			}
			else if (crossEnt->currentState.number < MAX_CLIENTS)
			{
				if (cgs.gametype >= GT_TEAM &&
					cgs.clientinfo[crossEnt->currentState.number].team == cgs.clientinfo[cg.snap->ps.clientNum].team)
				{
					//Allies are green
					ecolor[0] = 0.0; //R
					ecolor[1] = 1.0; //G
					ecolor[2] = 0.0; //B
				}
				else
				{
					if (cgs.gametype == GT_POWERDUEL &&
						cgs.clientinfo[crossEnt->currentState.number].duelTeam == cgs.clientinfo[cg.snap->ps.
						clientNum].
						duelTeam)
					{
						//on the same duel team in powerduel, so he's a friend
						ecolor[0] = 0.0; //R
						ecolor[1] = 1.0; //G
						ecolor[2] = 0.0; //B
					}
					else
					{
						//Enemies are red
						ecolor[0] = 1.0; //R
						ecolor[1] = 0.0; //G
						ecolor[2] = 0.0; //B
					}
				}

				if (cg.snap->ps.duelInProgress)
				{
					if (crossEnt->currentState.number != cg.snap->ps.duelIndex)
					{
						//grey out crosshair for everyone but your foe if you're in a duel
						ecolor[0] = 0.4f;
						ecolor[1] = 0.4f;
						ecolor[2] = 0.4f;
					}
				}
				else if (crossEnt->currentState.bolt1)
				{
					//this fellow is in a duel. We just checked if we were in a duel above, so
					//this means we aren't and he is. Which of course means our crosshair greys out over him.
					ecolor[0] = 0.4f;
					ecolor[1] = 0.4f;
					ecolor[2] = 0.4f;
				}
			}
			else if (crossEnt->currentState.shouldtarget || crossEnt->currentState.eType == ET_NPC)
			{
				//VectorCopy( crossEnt->startRGBA, ecolor );
				if (!ecolor[0] && !ecolor[1] && !ecolor[2])
				{
					// We really don't want black, so set it to yellow
					ecolor[0] = 1.0f; //R
					ecolor[1] = 0.8f; //G
					ecolor[2] = 0.3f; //B
				}

				if (crossEnt->currentState.eType == ET_NPC)
				{
					int pl_team;
					if (cgs.gametype == GT_SIEGE)
					{
						pl_team = cg.predictedPlayerState.persistant[PERS_TEAM];
					}
					else
					{
						pl_team = NPCTEAM_PLAYER;
					}

					if (crossEnt->currentState.powerups & 1 << PW_CLOAKED)
					{
						ecolor[0] = 1.0f; //R
						ecolor[1] = 1.0f; //G
						ecolor[2] = 1.0f; //B
					}
					else if (!crossEnt->currentState.teamowner)
					{
						//not on a team
						if (!crossEnt->currentState.teamowner ||
							crossEnt->currentState.NPC_class == CLASS_VEHICLE)
						{
							//neutral
							if (crossEnt->currentState.owner < MAX_CLIENTS)
							{
								//base color on who is pilotting this thing
								const clientInfo_t* ci = &cgs.clientinfo[crossEnt->currentState.owner];

								if (cgs.gametype >= GT_TEAM && ci->team == cg.predictedPlayerState.persistant[
									PERS_TEAM])
								{
									//friendly
									ecolor[0] = 0.0f; //R
									ecolor[1] = 1.0f; //G
									ecolor[2] = 0.0f; //B
								}
								else
								{
									//hostile
									ecolor[0] = 1.0f; //R
									ecolor[1] = 0.0f; //G
									ecolor[2] = 0.0f; //B
								}
							}
							else
							{
								//unmanned
								ecolor[0] = 1.0f; //R
								ecolor[1] = 1.0f; //G
								ecolor[2] = 0.0f; //B
							}
						}
						else
						{
							ecolor[0] = 1.0f; //R
							ecolor[1] = 0.0f; //G
							ecolor[2] = 0.0f; //B
						}
					}
					else if (crossEnt->currentState.teamowner != pl_team)
					{
						// on enemy team
						ecolor[0] = 1.0f; //R
						ecolor[1] = 0.0f; //G
						ecolor[2] = 0.0f; //B
					}
					else
					{
						//a friend
						ecolor[0] = 0.0f; //R
						ecolor[1] = 1.0f; //G
						ecolor[2] = 0.0f; //B
					}
				}
				else if (crossEnt->currentState.teamowner == TEAM_RED
					|| crossEnt->currentState.teamowner == TEAM_BLUE)
				{
					if (cgs.gametype < GT_TEAM)
					{
						//not teamplay, just neutral then
						ecolor[0] = 1.0f; //R
						ecolor[1] = 1.0f; //G
						ecolor[2] = 0.0f; //B
					}
					else if (crossEnt->currentState.teamowner != cgs.clientinfo[cg.snap->ps.clientNum].team)
					{
						//on the enemy team
						ecolor[0] = 1.0f; //R
						ecolor[1] = 0.0f; //G
						ecolor[2] = 0.0f; //B
					}
					else
					{
						//on my team
						ecolor[0] = 0.0f; //R
						ecolor[1] = 1.0f; //G
						ecolor[2] = 0.0f; //B
					}
				}
				else if (crossEnt->currentState.owner == cg.snap->ps.clientNum ||
					cgs.gametype >= GT_TEAM && crossEnt->currentState.teamowner == cgs.clientinfo[cg.snap->ps.
					clientNum]
					.team)
				{
					ecolor[0] = 0.0f; //R
					ecolor[1] = 1.0f; //G
					ecolor[2] = 0.0f; //B
				}
				else if (crossEnt->currentState.teamowner == 16 ||
					cgs.gametype >= GT_TEAM && crossEnt->currentState.teamowner && crossEnt->currentState.teamowner !=
					cgs.clientinfo[cg.snap->ps.clientNum].team)
				{
					ecolor[0] = 1.0f; //R
					ecolor[1] = 0.0f; //G
					ecolor[2] = 0.0f; //B
				}
			}
			else if (crossEnt->currentState.eType == ET_MOVER && crossEnt->currentState.bolt1 && cg.
				predictedPlayerState
				.weapon == WP_SABER)
			{
				//can push/pull this mover. Only show it if we're using the saber.
				ecolor[0] = 0.2f;
				ecolor[1] = 0.5f;
				ecolor[2] = 1.0f;

				corona = qtrue;
			}
			else if (crossEnt->currentState.eType == ET_MOVER && crossEnt->currentState.teamowner)
			{
				//a team owns this - if it's my team green, if not red, if not teamplay then yellow
				if (cgs.gametype < GT_TEAM)
				{
					ecolor[0] = 1.0f; //R
					ecolor[1] = 1.0f; //G
					ecolor[2] = 0.0f; //B
				}
				else if (cg.predictedPlayerState.persistant[PERS_TEAM] != crossEnt->currentState.teamowner)
				{
					//not my team
					ecolor[0] = 1.0f; //R
					ecolor[1] = 0.0f; //G
					ecolor[2] = 0.0f; //B
				}
				else
				{
					//my team
					ecolor[0] = 0.0f; //R
					ecolor[1] = 1.0f; //G
					ecolor[2] = 0.0f; //B
				}
			}
			else if (crossEnt->currentState.health)
			{
				if (!crossEnt->currentState.teamowner || cgs.gametype < GT_TEAM)
				{
					//not owned by a team or teamplay
					ecolor[0] = 1.0f;
					ecolor[1] = 1.0f;
					ecolor[2] = 0.0f;
				}
				else if (crossEnt->currentState.teamowner == cg.predictedPlayerState.persistant[PERS_TEAM])
				{
					//owned by my team
					ecolor[0] = 0.0f;
					ecolor[1] = 1.0f;
					ecolor[2] = 0.0f;
				}
				else
				{
					//hostile
					ecolor[0] = 1.0f;
					ecolor[1] = 0.0f;
					ecolor[2] = 0.0f;
				}
			}

			ecolor[3] = 1.0f;

			trap->R_SetColor(ecolor);
		}
	}

	if (cg.predictedPlayerState.m_iVehicleNum)
	{
		//I'm in a vehicle
		const centity_t* veh_cent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
		if (veh_cent
			&& veh_cent->m_pVehicle
			&& veh_cent->m_pVehicle->m_pVehicleInfo
			&& veh_cent->m_pVehicle->m_pVehicleInfo->crosshairShaderHandle)
		{
			hShader = veh_cent->m_pVehicle->m_pVehicleInfo->crosshairShaderHandle;
		}
		//bigger by default
		w = cg_crosshairSize.value * 2.0f;
		h = w;
	}
	else
	{
		if (cg.snap->ps.weapon == WP_BRYAR_PISTOL)
		{
			w = h = cg_crosshairDualSize.value;
		}
		else
		{
			w = h = cg_crosshairSize.value;
		}
	}

	// pulse the size of the crosshair when picking up items
	float f = cg.time - cg.itemPickupBlendTime;
	if (f > 0 && f < ITEM_BLOB_TIME)
	{
		f /= ITEM_BLOB_TIME;
		w *= 1 + f;
		h *= 1 + f;
	}

	if (world_point && VectorLength(world_point))
	{
		if (!CG_WorldCoordToScreenCoordFloat(world_point, &x, &y))
		{
			//off screen, don't draw it
			return;
		}
		x -= 320;
		y -= 240;
	}
	else
	{
		x = cg_crosshairX.integer;
		y = cg_crosshairY.integer;
	}

	if (!hShader)
	{
		if (cg_weaponcrosshairs.integer)
		{
			if (cg.snap->ps.weapon == WP_SABER ||
				cg.snap->ps.weapon == WP_MELEE)
			{
				hShader = cgs.media.crosshairShader[1];
			}
			else if (cg.snap->ps.weapon == WP_REPEATER ||
				cg.snap->ps.weapon == WP_BLASTER)
			{
				hShader = cgs.media.crosshairShader[2];
			}
			else if (cg.snap->ps.weapon == WP_FLECHETTE ||
				cg.snap->ps.weapon == WP_CONCUSSION ||
				cg.snap->ps.weapon == WP_BOWCASTER ||
				cg.snap->ps.weapon == WP_DEMP2)
			{
				hShader = cgs.media.crosshairShader[3];
			}
			else if (cg.snap->ps.weapon == WP_THERMAL ||
				cg.snap->ps.weapon == WP_DET_PACK ||
				cg.snap->ps.weapon == WP_TRIP_MINE)
			{
				hShader = cgs.media.crosshairShader[5];
			}
			else if (cg.snap->ps.weapon == WP_DISRUPTOR)
			{
				hShader = cgs.media.crosshairShader[6];
			}
			else if (cg.snap->ps.weapon == WP_BRYAR_OLD ||
				cg.snap->ps.weapon == WP_BRYAR_PISTOL ||
				cg.snap->ps.weapon == WP_STUN_BATON)
			{
				hShader = cgs.media.crosshairShader[7];
			}
			else if (cg.snap->ps.weapon == WP_ROCKET_LAUNCHER)
			{
				hShader = cgs.media.crosshairShader[8];
			}
			else
			{
				hShader = cgs.media.crosshairShader[Com_Clampi(1, NUM_CROSSHAIRS, cg_drawCrosshair.integer) - 1];
			}
		}
		else
		{
			hShader = cgs.media.crosshairShader[Com_Clampi(1, NUM_CROSSHAIRS, cg_drawCrosshair.integer) - 1];
		}
	}

	const float chX = x + cg.refdef.x + 0.5f * (640 - w);
	float chY = y + cg.refdef.y + 0.5f * (480 - h);
	trap->R_DrawStretchPic(chX, chY, w, h, 0, 0, 1, 1, hShader);

	//draw a health bar directly under the crosshair if we're looking at something
	//that takes damage
	if (crossEnt &&
		crossEnt->currentState.maxhealth)
	{
		CG_DrawHealthBar(crossEnt, chX, chY, w, h);
		chY += HEALTH_HEIGHT * 2;
	}
	else if (crossEnt && crossEnt->currentState.number < MAX_CLIENTS)
	{
		if (cgs.gametype == GT_SIEGE)
		{
			CG_DrawSiegeInfo(crossEnt, chX, chY, w, h);
			chY += HEALTH_HEIGHT * 4;
		}
		if (cg.crosshairVehNum && cg.time == cg.crosshairVehTime)
		{
			//it was in the crosshair this frame
			const centity_t* his_veh = &cg_entities[cg.crosshairVehNum];

			if (his_veh->currentState.eType == ET_NPC &&
				his_veh->currentState.NPC_class == CLASS_VEHICLE &&
				his_veh->currentState.maxhealth &&
				his_veh->m_pVehicle)
			{
				//draw the health for this vehicle
				CG_DrawHealthBar(his_veh, chX, chY, w, h);
				chY += HEALTH_HEIGHT * 2;
			}
		}
	}

	if (cg.predictedPlayerState.hackingTime)
	{//hacking something
		CG_DrawHaqrBar(chX, chY - 50, w, h);
	}

	if (cg_genericTimerBar > cg.time)
	{
		//draw generic timing bar, can be used for whatever
		CG_DrawGenericTimerBar();
	}

	if (corona) // drawing extra bits
	{
		ecolor[3] = 0.5f;
		ecolor[0] = ecolor[1] = ecolor[2] = (1 - ecolor[3]) * (sin(cg.time * 0.001f) * 0.08f + 0.35f);
		// don't draw full color
		ecolor[3] = 1.0f;

		trap->R_SetColor(ecolor);

		w *= 2.0f;
		h *= 2.0f;

		trap->R_DrawStretchPic(x + cg.refdef.x + 0.5f * (640 - w), y + cg.refdef.y + 0.5f * (480 - h), w, h, 0, 0, 1, 1,
			cgs.media.forceCoronaShader);
	}

	trap->R_SetColor(NULL);
}

qboolean CG_WorldCoordToScreenCoordFloat(vec3_t world_coord, float* x, float* y)
{
	vec3_t trans;

	const float px = tan(cg.refdef.fov_x * (M_PI / 360));
	const float py = tan(cg.refdef.fov_y * (M_PI / 360));

	VectorSubtract(world_coord, cg.refdef.vieworg, trans);

	const float xc = 640 / 2.0;
	const float yc = 480 / 2.0;

	// z = how far is the object in our forward direction
	const float z = DotProduct(trans, cg.refdef.viewaxis[0]);
	if (z <= 0.001)
		return qfalse;

	*x = xc - DotProduct(trans, cg.refdef.viewaxis[1]) * xc / (z * px);
	*y = yc - DotProduct(trans, cg.refdef.viewaxis[2]) * yc / (z * py);

	return qtrue;
}

qboolean CG_WorldCoordToScreenCoord(vec3_t world_coord, int* x, int* y)
{
	float x_f, y_f;

	if (CG_WorldCoordToScreenCoordFloat(world_coord, &x_f, &y_f))
	{
		*x = (int)x_f;
		*y = (int)y_f;
		return qtrue;
	}

	return qfalse;
}

/*
====================
CG_SaberClashFlare
====================
*/
int cg_saberFlashTime = 0;
vec3_t cg_saberFlashPos = { 0, 0, 0 };

void CG_SaberClashFlare(void)
{
	const int max_time = 150;
	vec3_t dif;
	vec4_t color;
	int x, y;
	trace_t tr;

	const int t = cg.time - cg_saberFlashTime;

	if (t <= 0 || t >= max_time)
	{
		return;
	}

	if (cg.predictedPlayerState.communicatingflags & (1 << CF_SABERLOCKING))
	{
		return;
	}

	// Don't do clashes for things that are behind us
	VectorSubtract(cg_saberFlashPos, cg.refdef.vieworg, dif);

	if (DotProduct(dif, cg.refdef.viewaxis[0]) < 0.2)
	{
		return;
	}

	CG_Trace(&tr, cg.refdef.vieworg, NULL, NULL, cg_saberFlashPos, -1, CONTENTS_SOLID);

	if (tr.fraction < 1.0f)
	{
		return;
	}

	const float len = VectorNormalize(dif);
	if (len > 1200)
	{
		return;
	}

	float v = (1.0f - (float)t / max_time) * ((1.0f - len / 800.0f) * 2.0f + 0.35f);
	if (v < 0.001f)
	{
		v = 0.001f;
	}

	if (!CG_WorldCoordToScreenCoord(cg_saberFlashPos, &x, &y))
	{
		return;
	}

	VectorSet4(color, 0.8f, 0.8f, 0.8f, 1.0f);
	trap->R_SetColor(color);

	CG_DrawPic(x - v * 300, y - v * 300, v * 600, v * 600, trap->R_RegisterShader("gfx/effects/saberFlare"));
}

void CG_SaberBlockFlare(void)
{
	const int max_time = 150;
	vec3_t dif;
	vec4_t color;
	int x, y;
	trace_t tr;

	const int t = cg.time - cg_saberFlashTime;

	if (t <= 0 || t >= max_time)
	{
		return;
	}

	// Don't do clashes for things that are behind us
	VectorSubtract(cg_saberFlashPos, cg.refdef.vieworg, dif);

	if (DotProduct(dif, cg.refdef.viewaxis[0]) < 0.2)
	{
		return;
	}

	CG_Trace(&tr, cg.refdef.vieworg, NULL, NULL, cg_saberFlashPos, -1, CONTENTS_SOLID);

	if (tr.fraction < 1.0f)
	{
		return;
	}

	const float len = VectorNormalize(dif);
	if (len > 1200)
	{
		return;
	}

	float v = (1.0f - (float)t / max_time) * ((1.0f - len / 800.0f) * 2.0f + 0.35f);
	if (v < 0.001f)
	{
		v = 0.001f;
	}

	if (!CG_WorldCoordToScreenCoord(cg_saberFlashPos, &x, &y))
	{
		return;
	}

	VectorSet4(color, 0.8f, 0.8f, 0.8f, 1.0f);
	trap->R_SetColor(color);

	CG_DrawPic(x - v * 300, y - v * 300,
		v * 600, v * 600,
		trap->R_RegisterShader("gfx/effects/BlockFlare"));
}

static void CG_DottedLine(const float x1, const float y1, const float x2, const float y2, const float dot_size,
	const int num_dots, vec4_t color, const float alpha)
{
	vec4_t color_rgba;

	VectorCopy4(color, color_rgba);
	color_rgba[3] = alpha;

	trap->R_SetColor(color_rgba);

	const float x_diff = x2 - x1;
	const float y_diff = y2 - y1;
	const float x_step = x_diff / (float)num_dots;
	const float y_step = y_diff / (float)num_dots;

	for (int dot_num = 0; dot_num < num_dots; dot_num++)
	{
		const float x = x1 + x_step * dot_num - dot_size * 0.5f;
		const float y = y1 + y_step * dot_num - dot_size * 0.5f;

		CG_DrawPic(x, y, dot_size, dot_size, cgs.media.whiteShader);
	}
}

static void CG_BracketEntity(centity_t* cent, const float radius)
{
	trace_t tr;
	vec3_t dif;
	float size;
	float x, y;
	qboolean is_enemy = qfalse;

	VectorSubtract(cent->lerpOrigin, cg.refdef.vieworg, dif);
	const float len = VectorNormalize(dif);

	if (cg.crosshairclientNum != cent->currentState.clientNum
		&& (!cg.snap || cg.snap->ps.rocketLockIndex != cent->currentState.clientNum))
	{
		//if they're the entity you're locking onto or under your crosshair, always draw bracket
		//Hmm... for now, if they're closer than 2000, don't bracket?
		if (len < 2000.0f)
		{
			return;
		}

		CG_Trace(&tr, cg.refdef.vieworg, NULL, NULL, cent->lerpOrigin, -1, CONTENTS_OPAQUE);

		//don't bracket if can't see them
		if (tr.fraction < 1.0f)
		{
			return;
		}
	}

	if (!CG_WorldCoordToScreenCoordFloat(cent->lerpOrigin, &x, &y))
	{
		//off-screen, don't draw it
		return;
	}

	//just to see if it's centered
	//CG_DrawPic( x-2, y-2, 4, 4, cgs.media.whiteShader );

	const clientInfo_t* local = &cgs.clientinfo[cg.snap->ps.clientNum];
	if (cent->currentState.m_iVehicleNum //vehicle has a driver
		&& cent->currentState.m_iVehicleNum - 1 < MAX_CLIENTS
		&& cgs.clientinfo[cent->currentState.m_iVehicleNum - 1].infoValid)
	{
		if (cgs.gametype < GT_TEAM)
		{
			//ffa?
			is_enemy = qtrue;
			trap->R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
		}
		else if (cgs.clientinfo[cent->currentState.m_iVehicleNum - 1].team == local->team)
		{
			trap->R_SetColor(g_color_table[ColorIndex(COLOR_GREEN)]);
		}
		else
		{
			is_enemy = qtrue;
			trap->R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
		}
	}
	else if (cent->currentState.teamowner)
	{
		if (cgs.gametype < GT_TEAM)
		{
			//ffa?
			is_enemy = qtrue;
			trap->R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
		}
		else if (cent->currentState.teamowner != cg.predictedPlayerState.persistant[PERS_TEAM])
		{
			// on enemy team
			is_enemy = qtrue;
			trap->R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
		}
		else
		{
			//a friend
			trap->R_SetColor(g_color_table[ColorIndex(COLOR_GREEN)]);
		}
	}
	else
	{
		//FIXME: if we want to ever bracket anything besides vehicles (like siege objectives we want to blow up), we should handle the coloring here
		trap->R_SetColor(NULL);
	}

	if (len <= 1.0f)
	{
		//super-close, max out at 400 times radius (which is HUGE)
		size = radius * 400.0f;
	}
	else
	{
		//scale by dist
		size = radius * (400.0f / len);
	}

	if (size < 1.0f)
	{
		size = 1.0f;
	}

	//length scales with dist
	float line_length = size * 0.1f;
	if (line_length < 0.5f)
	{
		//always visible
		line_length = 0.5f;
	}
	//always visible width

	x -= size * 0.5f;
	y -= size * 0.5f;

	{
		const float line_width = 1.0f;
		//brackets would be drawn on the screen, so draw them
		//upper left corner
		//horz
		CG_DrawPic(x, y, line_length, line_width, cgs.media.whiteShader);
		//vert
		CG_DrawPic(x, y, line_width, line_length, cgs.media.whiteShader);
		//upper right corner
		//horz
		CG_DrawPic(x + size - line_length, y, line_length, line_width, cgs.media.whiteShader);
		//vert
		CG_DrawPic(x + size - line_width, y, line_width, line_length, cgs.media.whiteShader);
		//lower left corner
		//horz
		CG_DrawPic(x, y + size - line_width, line_length, line_width, cgs.media.whiteShader);
		//vert
		CG_DrawPic(x, y + size - line_length, line_width, line_length, cgs.media.whiteShader);
		//lower right corner
		//horz
		CG_DrawPic(x + size - line_length, y + size - line_width, line_length, line_width, cgs.media.whiteShader);
		//vert
		CG_DrawPic(x + size - line_width, y + size - line_length, line_width, line_length, cgs.media.whiteShader);
	}
	//Lead Indicator...
	if (cg_drawVehLeadIndicator.integer)
	{
		//draw the lead indicator
		if (is_enemy)
		{
			//an enemy object
			if (cent->currentState.NPC_class == CLASS_VEHICLE)
			{
				//enemy vehicle
				if (!VectorCompare(cent->currentState.pos.trDelta, vec3_origin))
				{
					//enemy vehicle is moving
					if (cg.predictedPlayerState.m_iVehicleNum)
					{
						//I'm in a vehicle
						const centity_t* veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
						if (veh //vehicle cent
							&& veh->m_pVehicle //vehicle
							&& veh->m_pVehicle->m_pVehicleInfo //vehicle stats
							&& veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID > VEH_WEAPON_BASE) //valid vehicle weapon
						{
							const vehWeaponInfo_t* veh_weapon = &g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon
								[
									0].ID];
							if (veh_weapon
								&& veh_weapon->bIsProjectile //primary weapon's shot is a projectile
								&& !veh_weapon->bHasGravity //primary weapon's shot is not affected by gravity
								&& !veh_weapon->fHoming //primary weapon's shot is not homing
								&& veh_weapon->fSpeed) //primary weapon's shot has speed
							{
								//our primary weapon's projectile has a speed
								vec3_t veh_diff, veh_lead_pos;
								float lead_x, lead_y;

								VectorSubtract(cent->lerpOrigin, cg.predictedVehicleState.origin, veh_diff);
								const float veh_dist = VectorNormalize(veh_diff);
								const float eta = veh_dist / veh_weapon->fSpeed;
								//how many seconds it would take for my primary weapon's projectile to get from my ship to theirs
								//now extrapolate their position that number of seconds into the future based on their velocity
								VectorMA(cent->lerpOrigin, eta, cent->currentState.pos.trDelta, veh_lead_pos);
								//now we have where we should be aiming at, project that onto the screen at a 2D co-ord
								if (!CG_WorldCoordToScreenCoordFloat(cent->lerpOrigin, &x, &y))
								{
									//off-screen, don't draw it
									return;
								}
								if (!CG_WorldCoordToScreenCoordFloat(veh_lead_pos, &lead_x, &lead_y))
								{
									//off-screen, don't draw it
									//just draw the line
									CG_DottedLine(x, y, lead_x, lead_y, 1, 10, g_color_table[ColorIndex(COLOR_RED)],
										0.5f);
									return;
								}
								//draw a line from the ship's cur pos to the lead pos
								CG_DottedLine(x, y, lead_x, lead_y, 1, 10, g_color_table[ColorIndex(COLOR_RED)], 0.5f);
								//now draw the lead indicator
								trap->R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
								CG_DrawPic(lead_x - 8, lead_y - 8, 16, 16,
									trap->R_RegisterShader("gfx/menus/radar/lead"));
							}
						}
					}
				}
			}
		}
	}
}

qboolean CG_InFighter(void)
{
	if (cg.predictedPlayerState.m_iVehicleNum)
	{
		//I'm in a vehicle
		const centity_t* veh_cent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
		if (veh_cent
			&& veh_cent->m_pVehicle
			&& veh_cent->m_pVehicle->m_pVehicleInfo
			&& veh_cent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
		{
			//I'm in a fighter
			return qtrue;
		}
	}
	return qfalse;
}

qboolean CG_InATST(void)
{
	if (cg.predictedPlayerState.m_iVehicleNum)
	{
		//I'm in a vehicle
		const centity_t* veh_cent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
		if (veh_cent
			&& veh_cent->m_pVehicle
			&& veh_cent->m_pVehicle->m_pVehicleInfo
			&& veh_cent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)
		{
			//I'm in an atst
			return qtrue;
		}
	}
	return qfalse;
}

static void CG_DrawBracketedEntities(void)
{
	for (int i = 0; i < cg.bracketedEntityCount; i++)
	{
		centity_t* cent = &cg_entities[cg.bracketedEntities[i]];
		CG_BracketEntity(cent, CG_RadiusForCent(cent));
	}
}

//--------------------------------------------------------------
static void CG_DrawHolocronIcons(void)
//--------------------------------------------------------------
{
	const int icon_size = 40;
	int i = 0;
	int startx = 10;
	int starty = 10; //SCREEN_HEIGHT - icon_size*3;

	const int endx = icon_size;
	const int endy = icon_size;

	if (cg.snap->ps.zoomMode)
	{
		//don't display over zoom mask
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	while (i < NUM_FORCE_POWERS)
	{
		if (cg.snap->ps.holocronBits & 1 << forcePowerSorted[i])
		{
			CG_DrawPic(startx, starty, endx, endy, cgs.media.forcePowerIcons[forcePowerSorted[i]]);
			starty += icon_size + 2; //+2 for spacing
			if (starty + icon_size >= SCREEN_HEIGHT - 80)
			{
				starty = 10; //SCREEN_HEIGHT - icon_size*3;
				startx += icon_size + 2;
			}
		}

		i++;
	}
}

static qboolean CG_IsDurationPower(const int power)
{
	if (power == FP_HEAL ||
		power == FP_SPEED ||
		power == FP_TELEPATHY ||
		power == FP_RAGE ||
		power == FP_PROTECT ||
		power == FP_ABSORB ||
		power == FP_SEE)
	{
		return qtrue;
	}

	return qfalse;
}

//--------------------------------------------------------------
static void CG_DrawActivePowers(void)
//--------------------------------------------------------------
{
	const int icon_size = 20;
	const int icon_size2 = 10;
	const int icon_size3 = 60;
	int i = 0;
	int startx = icon_size2 * 2 + 16;
	int starty = SCREEN_HEIGHT - icon_size3 * 2;

	const int endx = icon_size;
	const int endy = icon_size;

	if (cg.snap->ps.zoomMode)
	{
		//don't display over zoom mask
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	trap->R_SetColor(NULL);

	while (i < NUM_FORCE_POWERS)
	{
		if (cg.snap->ps.fd.forcePowersActive & 1 << forcePowerSorted[i] &&
			CG_IsDurationPower(forcePowerSorted[i]))
		{
			CG_DrawPic(startx, starty, endx, endy, cgs.media.forcePowerIcons[forcePowerSorted[i]]);
			startx += icon_size + 2; //+2 for spacing
			if (startx + icon_size >= SCREEN_WIDTH - 80)
			{
				startx = icon_size * 2 + 16;
				starty += icon_size + 2;
			}
		}

		i++;
	}

	//additionally, draw an icon force force rage recovery
	if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
	{
		CG_DrawPic(startx, starty, endx, endy, cgs.media.rageRecShader);
	}
}

//--------------------------------------------------------------
static void CG_DrawRocketLocking(const int lock_ent_num)
//--------------------------------------------------------------
{
	int cx, cy;
	vec3_t org;
	const centity_t* cent = &cg_entities[lock_ent_num];
	vec4_t color = { 0.0f, 0.0f, 0.0f, 0.0f };
	float lock_time_interval = (cgs.gametype == GT_SIEGE ? 2400.0f : 1200.0f) / 16.0f;
	//FIXME: if in a vehicle, use the vehicle's lockOnTime...
	int dif = (cg.time - cg.snap->ps.rocketLockTime) / lock_time_interval;

	if (!cg.snap->ps.rocketLockTime)
	{
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.m_iVehicleNum)
	{
		//driving a vehicle
		const centity_t* veh = &cg_entities[cg.snap->ps.m_iVehicleNum];
		if (veh->m_pVehicle)
		{
			const vehWeaponInfo_t* vehWeapon = NULL;
			if (cg.predictedVehicleState.weaponstate == WEAPON_CHARGING_ALT)
			{
				if (veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID > VEH_WEAPON_BASE
					&& veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID < MAX_VEH_WEAPONS)
				{
					vehWeapon = &g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID];
				}
			}
			else
			{
				if (veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID > VEH_WEAPON_BASE
					&& veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID < MAX_VEH_WEAPONS)
				{
					vehWeapon = &g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID];
				}
			}
			if (vehWeapon != NULL)
			{
				//we are trying to lock on with a valid vehicle weapon, so use *its* locktime, not the hard-coded one
				if (!vehWeapon->iLockOnTime)
				{
					//instant lock-on
					dif = 10.0f;
				}
				else
				{
					//use the custom vehicle lockOnTime
					lock_time_interval = vehWeapon->iLockOnTime / 16.0f;
					dif = (cg.time - cg.snap->ps.rocketLockTime) / lock_time_interval;
				}
			}
		}
	}
	//We can't check to see in pmove if players are on the same team, so we resort
	//to just not drawing the lock if a teammate is the locked on ent
	if (cg.snap->ps.rocketLockIndex >= 0 &&
		cg.snap->ps.rocketLockIndex < ENTITYNUM_NONE)
	{
		const clientInfo_t* ci;

		if (cg.snap->ps.rocketLockIndex < MAX_CLIENTS)
		{
			ci = &cgs.clientinfo[cg.snap->ps.rocketLockIndex];
		}
		else
		{
			ci = cg_entities[cg.snap->ps.rocketLockIndex].npcClient;
		}

		if (ci)
		{
			if (ci->team == cgs.clientinfo[cg.snap->ps.clientNum].team)
			{
				if (cgs.gametype >= GT_TEAM)
				{
					return;
				}
			}
			else if (cgs.gametype >= GT_TEAM)
			{
				const centity_t* hit_ent = &cg_entities[cg.snap->ps.rocketLockIndex];
				if (hit_ent->currentState.eType == ET_NPC &&
					hit_ent->currentState.NPC_class == CLASS_VEHICLE &&
					hit_ent->currentState.owner < ENTITYNUM_WORLD)
				{
					//this is a vehicle, if it has a pilot and that pilot is on my team, then...
					if (hit_ent->currentState.owner < MAX_CLIENTS)
					{
						ci = &cgs.clientinfo[hit_ent->currentState.owner];
					}
					else
					{
						ci = cg_entities[hit_ent->currentState.owner].npcClient;
					}
					if (ci && ci->team == cgs.clientinfo[cg.snap->ps.clientNum].team)
					{
						return;
					}
				}
			}
		}
	}

	if (cg.snap->ps.rocketLockTime != -1)
	{
		lastvalidlockdif = dif;
	}
	else
	{
		dif = lastvalidlockdif;
	}

	if (!cent)
	{
		return;
	}

	VectorCopy(cent->lerpOrigin, org);

	if (CG_WorldCoordToScreenCoord(org, &cx, &cy))
	{
		static int old_dif = 0;
		// we care about distance from enemy to eye, so this is good enough
		float sz = Distance(cent->lerpOrigin, cg.refdef.vieworg) / 1024.0f;

		if (sz > 1.0f)
		{
			sz = 1.0f;
		}
		else if (sz < 0.0f)
		{
			sz = 0.0f;
		}

		sz = (1.0f - sz) * (1.0f - sz) * 32 + 6;

		if (cg.snap->ps.m_iVehicleNum)
		{
			sz *= 2.0f;
		}

		cy += sz * 0.5f;

		if (dif < 0)
		{
			old_dif = 0;
			return;
		}
		if (dif > 8)
		{
			dif = 8;
		}

		// do sounds
		if (old_dif != dif)
		{
			if (dif == 8)
			{
				if (cg.snap->ps.m_iVehicleNum)
				{
					trap->S_StartSound(org, 0, CHAN_AUTO,
						trap->S_RegisterSound("sound/vehicles/weapons/common/lock.wav"));
				}
				else
				{
					trap->S_StartSound(org, 0, CHAN_AUTO, trap->S_RegisterSound("sound/weapons/rocket/lock.wav"));
				}
			}
			else
			{
				if (cg.snap->ps.m_iVehicleNum)
				{
					trap->S_StartSound(org, 0, CHAN_AUTO,
						trap->S_RegisterSound("sound/vehicles/weapons/common/tick.wav"));
				}
				else
				{
					trap->S_StartSound(org, 0, CHAN_AUTO, trap->S_RegisterSound("sound/weapons/rocket/tick.wav"));
				}
			}
		}

		old_dif = dif;

		for (int i = 0; i < dif; i++)
		{
			color[0] = 1.0f;
			color[1] = 0.0f;
			color[2] = 0.0f;
			color[3] = 0.1f * i + 0.2f;

			trap->R_SetColor(color);

			// our slices are offset by about 45 degrees.
			CG_DrawRotatePic(cx - sz, cy - sz, sz, sz, i * 45.0f, trap->R_RegisterShaderNoMip("gfx/2d/wedge"));
		}

		// we are locked and loaded baby
		if (dif == 8)
		{
			color[0] = color[1] = color[2] = sin(cg.time * 0.05f) * 0.5f + 0.5f;
			color[3] = 1.0f; // this art is additive, so the alpha value does nothing

			trap->R_SetColor(color);

			CG_DrawPic(cx - sz, cy - sz * 2, sz * 2, sz * 2, trap->R_RegisterShaderNoMip("gfx/2d/lock"));
		}
	}
}

extern void CG_CalcVehMuzzle(Vehicle_t* p_veh, centity_t* ent, int muzzle_num);

static qboolean CG_CalcVehiclemuzzlePoint(const int entityNum, vec3_t start, vec3_t d_f, vec3_t d_rt, vec3_t d_up)
{
	centity_t* veh_cent = &cg_entities[entityNum];
	if (veh_cent->m_pVehicle && veh_cent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)
	{
		//draw from barrels
		VectorCopy(veh_cent->lerpOrigin, start);
		start[2] += veh_cent->m_pVehicle->m_pVehicleInfo->height - DEFAULT_MINS_2 - 48;
		AngleVectors(veh_cent->lerpAngles, d_f, d_rt, d_up);
	}
	else
	{
		//check to see if we're a turret gunner on this vehicle
		if (cg.predictedPlayerState.generic1) //as a passenger
		{
			//passenger in a vehicle
			if (veh_cent->m_pVehicle
				&& veh_cent->m_pVehicle->m_pVehicleInfo
				&& veh_cent->m_pVehicle->m_pVehicleInfo->maxPassengers)
			{
				//a vehicle capable of carrying passengers
				for (int turret_num = 0; turret_num < MAX_VEHICLE_TURRETS; turret_num++)
				{
					if (veh_cent->m_pVehicle->m_pVehicleInfo->turret[turret_num].iAmmoMax)
					{
						// valid turret
						if (veh_cent->m_pVehicle->m_pVehicleInfo->turret[turret_num].passengerNum == cg.
							predictedPlayerState.generic1)
						{
							//I control this turret
							//Go through all muzzles, average their positions and directions and use the result for crosshair trace
							int num_muzzles = 0;
							vec3_t muzzles_avg_pos = { 0 }, muzzles_avg_dir = { 0 };

							for (int i = 0; i < MAX_VEHICLE_TURRET_MUZZLES; i++)
							{
								int vehMuzzle = veh_cent->m_pVehicle->m_pVehicleInfo->turret[turret_num].iMuzzle[i];
								if (vehMuzzle)
								{
									vehMuzzle -= 1;
									CG_CalcVehMuzzle(veh_cent->m_pVehicle, veh_cent, vehMuzzle);
									VectorAdd(muzzles_avg_pos, veh_cent->m_pVehicle->m_vMuzzlePos[vehMuzzle],
										muzzles_avg_pos);
									VectorAdd(muzzles_avg_dir, veh_cent->m_pVehicle->m_vMuzzleDir[vehMuzzle],
										muzzles_avg_dir);
									num_muzzles++;
								}
								if (num_muzzles)
								{
									VectorScale(muzzles_avg_pos, 1.0f / (float)num_muzzles, start);
									VectorScale(muzzles_avg_dir, 1.0f / (float)num_muzzles, d_f);
									VectorClear(d_rt);
									VectorClear(d_up);
									return qtrue;
								}
							}
						}
					}
				}
			}
		}
		VectorCopy(veh_cent->lerpOrigin, start);
		AngleVectors(veh_cent->lerpAngles, d_f, d_rt, d_up);
	}
	return qfalse;
}

//calc the muzzle point from the e-web itself
static void CG_CalcEWebmuzzlePoint(centity_t* cent, vec3_t start, vec3_t d_f, vec3_t d_rt, vec3_t d_up)
{
	const int bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*cannonflash");

	assert(bolt != -1);

	if (bolt != -1)
	{
		mdxaBone_t boltMatrix;

		trap->G2API_GetBoltMatrix_NoRecNoRot(cent->ghoul2, 0, bolt, &boltMatrix, cent->lerpAngles, cent->lerpOrigin,
			cg.time, NULL, cent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, start);
		BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, d_f);

		//these things start the shot a little inside the bbox to assure not starting in something solid
		VectorMA(start, -16.0f, d_f, start);

		//I guess
		VectorClear(d_rt); //don't really need this, do we?
		VectorClear(d_up); //don't really need this, do we?
	}
}

/*
=================
CG_`Entity
=================
*/
#define MAX_XHAIR_DIST_ACCURACY	20000.0f
void CG_TraceItem(trace_t* result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
	int skip_number);

static void CG_ScanForCrosshairEntity(void)
{
	trace_t trace;
	vec3_t start, end;
	qboolean b_veh_check_trace_from_cam_pos = qfalse;

	int ignore = cg.predictedPlayerState.clientNum;

	if (cg_dynamicCrosshair.integer)
	{
		vec3_t d_f, d_rt, d_up;
		//For now we still want to draw the crosshair in relation to the player's world coordinates
		//even if we have a melee weapon/no weapon.
		if (cg.predictedPlayerState.m_iVehicleNum && cg.predictedPlayerState.eFlags & EF_NODRAW)
		{
			//we're *inside* a vehicle
			//do the vehicle's crosshair instead
			const centity_t* veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
			qboolean gunner;

			//if (veh->currentState.owner == cg.predictedPlayerState.clientNum)
			{
				//the pilot
				ignore = cg.predictedPlayerState.m_iVehicleNum;
				gunner = CG_CalcVehiclemuzzlePoint(cg.predictedPlayerState.m_iVehicleNum, start, d_f, d_rt, d_up);
			}
			if (veh->m_pVehicle
				&& veh->m_pVehicle->m_pVehicleInfo
				&& veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER
				&& cg.distanceCull > MAX_XHAIR_DIST_ACCURACY
				&& !gunner)
			{
				//NOTE: on huge maps, the crosshair gets inaccurate at close range,
				//		so we'll do an extra G2 trace from the cg.refdef.vieworg
				//		to see if we hit anything closer and auto-aim at it if so
				b_veh_check_trace_from_cam_pos = qtrue;
			}
		}
		else if (cg.snap && cg.snap->ps.weapon == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex &&
			cg_entities[cg.snap->ps.emplacedIndex].ghoul2 && cg_entities[cg.snap->ps.emplacedIndex].currentState.weapon
			== WP_NONE)
		{
			//locked into our e-web, calc the muzzle from it
			CG_CalcEWebmuzzlePoint(&cg_entities[cg.snap->ps.emplacedIndex], start, d_f, d_rt, d_up);
		}
		else
		{
			if (cg.snap && cg.snap->ps.weapon == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex)
			{
				vec3_t pitch_constraint;

				ignore = cg.snap->ps.emplacedIndex;

				VectorCopy(cg.refdef.viewangles, pitch_constraint);

				if (cg.renderingThirdPerson)
				{
					VectorCopy(cg.predictedPlayerState.viewangles, pitch_constraint);
				}
				else
				{
					VectorCopy(cg.refdef.viewangles, pitch_constraint);
				}

				if (pitch_constraint[PITCH] > 40)
				{
					pitch_constraint[PITCH] = 40;
				}

				AngleVectors(pitch_constraint, d_f, d_rt, d_up);
			}
			else
			{
				vec3_t pitchConstraint;

				if (cg.renderingThirdPerson)
				{
					VectorCopy(cg.predictedPlayerState.viewangles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdef.viewangles, pitchConstraint);
				}

				AngleVectors(pitchConstraint, d_f, d_rt, d_up);
			}
			CG_CalcmuzzlePoint(cg.snap->ps.clientNum, start);
		}

		VectorMA(start, cg.distanceCull, d_f, end);
	}
	else
	{
		VectorCopy(cg.refdef.vieworg, start);
		VectorMA(start, 131072, cg.refdef.viewaxis[0], end);
	}

	if (cg_dynamicCrosshair.integer && cg_dynamicCrosshairPrecision.integer)
	{
		//then do a trace with ghoul2 models in mind
		CG_G2Trace(&trace, start, vec3_origin, vec3_origin, end, ignore, CONTENTS_SOLID | CONTENTS_BODY);
		if (b_veh_check_trace_from_cam_pos)
		{
			//NOTE: this MUST stay up to date with the method used in WP_VehCheckTraceFromCamPos
			const centity_t* veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
			trace_t extra_trace;
			vec3_t view_dir2_end, extra_end;
			const float min_auto_aim_dist = Distance(veh->lerpOrigin, cg.refdef.vieworg) + veh->m_pVehicle->
				m_pVehicleInfo
				->length / 2.0f + 200.0f;

			VectorSubtract(end, cg.refdef.vieworg, view_dir2_end);
			VectorNormalize(view_dir2_end);
			VectorMA(cg.refdef.vieworg, MAX_XHAIR_DIST_ACCURACY, view_dir2_end, extra_end);
			CG_G2Trace(&extra_trace, cg.refdef.vieworg, vec3_origin, vec3_origin, extra_end,
				ignore, CONTENTS_SOLID | CONTENTS_BODY);
			if (!extra_trace.allsolid
				&& !extra_trace.startsolid)
			{
				if (extra_trace.fraction < 1.0f)
				{
					if (extra_trace.fraction * MAX_XHAIR_DIST_ACCURACY > min_auto_aim_dist)
					{
						if (extra_trace.fraction * MAX_XHAIR_DIST_ACCURACY - Distance(
							veh->lerpOrigin, cg.refdef.vieworg)
							< trace.fraction * cg.distanceCull)
						{
							//this trace hit *something* that's closer than the thing the main trace hit, so use this result instead
							memcpy(&trace, &extra_trace, sizeof(trace_t));
						}
					}
				}
			}
		}
	}
	else
	{
		CG_Trace(&trace, start, vec3_origin, vec3_origin, end, ignore, CONTENTS_SOLID | CONTENTS_BODY);
	}

	if (trace.entityNum < MAX_CLIENTS)
	{
		if (CG_IsMindTricked(cg_entities[trace.entityNum].currentState.trickedentindex,
			cg_entities[trace.entityNum].currentState.trickedentindex2,
			cg_entities[trace.entityNum].currentState.trickedentindex3,
			cg_entities[trace.entityNum].currentState.trickedentindex4,
			cg.snap->ps.clientNum))
		{
			if (cg.crosshairclientNum == trace.entityNum)
			{
				cg.crosshairclientNum = ENTITYNUM_NONE;
				cg.crosshairClientTime = 0;
			}

			CG_DrawCrosshair(trace.endpos, 0);

			return; //this entity is mind-tricking the current client, so don't render it
		}
	}

	if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{
		if (trace.entityNum < /*MAX_CLIENTS*/ENTITYNUM_WORLD)
		{
			const centity_t* veh = &cg_entities[trace.entityNum];
			cg.crosshairclientNum = trace.entityNum;
			cg.crosshairClientTime = cg.time;

			if (veh->currentState.eType == ET_NPC &&
				veh->currentState.NPC_class == CLASS_VEHICLE &&
				veh->currentState.owner < MAX_CLIENTS)
			{
				//draw the name of the pilot then
				cg.crosshairclientNum = veh->currentState.owner;
				cg.crosshairVehNum = veh->currentState.number;
				cg.crosshairVehTime = cg.time;
			}

			CG_DrawCrosshair(trace.endpos, 1);
		}
		else
		{
			CG_DrawCrosshair(trace.endpos, 0);
		}
	}

	if (trace.entityNum >= MAX_CLIENTS)
	{
		return;
	}

	// if the player is in fog, don't show it
	const int content = CG_PointContents(trace.endpos, 0);
	if (content & CONTENTS_FOG)
	{
		return;
	}

	// update the fade timer
	cg.crosshairclientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}

static void CG_SanitizeString(const char* in, char* out)
{
	int i = 0;
	int r = 0;

	while (in[i])
	{
		if (i >= 128 - 1)
		{
			//the ui truncates the name here..
			break;
		}

		if (in[i] == '^')
		{
			if (in[i + 1] >= 48 && //'0'
				in[i + 1] <= 57) //'9'
			{
				//only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			//just skip the ^
			i++;
			continue;
		}

		if (in[i] < 32)
		{
			i++;
			continue;
		}

		out[r] = in[i];
		r++;
		i++;
	}
	out[r] = 0;
}

int next_vischeck[MAX_GENTITIES];
qboolean currently_visible[MAX_GENTITIES];

static qboolean CG_CheckClientVisibility(const centity_t* cent)
{
	trace_t trace;
	vec3_t start, end; //, forward, right, up;

	if (next_vischeck[cent->currentState.number] > cg.time)
	{
		return currently_visible[cent->currentState.number];
	}

	next_vischeck[cent->currentState.number] = cg.time + 500 + Q_irand(500, 1000);

	VectorCopy(cg.refdef.vieworg, start);
	start[2] += 42;

	VectorCopy(cent->lerpOrigin, end);
	end[2] += 42;

	CG_Trace(&trace, start, NULL, NULL, end, cg.clientNum, MASK_PLAYERSOLID);

	const centity_t* traceEnt = &cg_entities[trace.entityNum];

	if (traceEnt == cent || trace.fraction == 1.0f)
	{
		currently_visible[cent->currentState.number] = qtrue;
		return qtrue;
	}

	currently_visible[cent->currentState.number] = qfalse;
	return qfalse;
}

static void CG_DrawCrosshairItem(void)
{
	if (!cg_DrawCrosshairItem.integer)
	{
		return;
	}
	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	if (cg_entities[cg.crosshairclientNum].currentState.eType == ET_ITEM)
	{
		CG_DrawPic(50, 285, 32, 32, cgs.media.useableHint);
	}

	trap->R_SetColor(NULL);
}

/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames(void)
{
	char sanitized[1024];
	//int baseColor;
	qboolean is_veh = qfalse;

	if (in_camera)
	{
		//no crosshair while in cutscenes
		return;
	}

	if (!cg_drawCrosshairNames.integer)
	{
		return;
	}

	CG_ScanForCrosshairEntity();

	if (cg.crosshairclientNum < ENTITYNUM_WORLD)
	{
		const centity_t* veh = &cg_entities[cg.crosshairclientNum];

		if (veh->currentState.eType == ET_NPC &&
			veh->currentState.NPC_class == CLASS_VEHICLE &&
			veh->currentState.owner < MAX_CLIENTS)
		{
			//draw the name of the pilot then
			cg.crosshairclientNum = veh->currentState.owner;
			cg.crosshairVehNum = veh->currentState.number;
			cg.crosshairVehTime = cg.time;
			is_veh = qtrue; //so we know we're drawing the pilot's name
		}
	}

	if (cg_entities[cg.crosshairclientNum].currentState.powerups & 1 << PW_CLOAKED)
	{
		return;
	}

	// draw the name of the player being looked at
	const float* color = CG_FadeColor(cg.crosshairClientTime, 1000);

	if (!color)
	{
		trap->R_SetColor(NULL);
		return;
	}

	char* name = cgs.clientinfo[cg.crosshairclientNum].cleanname;

	const float w = CG_DrawStrlen(va("Civilian")) * TINYCHAR_WIDTH;

	if (cg.snap->ps.duelInProgress)
	{
		if (cg.crosshairclientNum != cg.snap->ps.duelIndex)
		{
			//grey out crosshair for everyone but your foe if you're in a duel
			//baseColor = CT_BLACK;
		}
	}
	else if (cg_entities[cg.crosshairclientNum].currentState.bolt1)
	{
		//this fellow is in a duel. We just checked if we were in a duel above, so
		//this means we aren't and he is. Which of course means our crosshair greys out over him.
		//baseColor = CT_BLACK;
	}

	CG_SanitizeString(name, sanitized);

	if (is_veh)
	{
		vec4_t tcolor;
		char str[MAX_STRING_CHARS];
		Com_sprintf(str, MAX_STRING_CHARS, "%s (pilot)", name);
		CG_DrawProportionalString(320, 170, str, UI_CENTER, tcolor);
	}
	else if (cg_entities[cg.crosshairclientNum].currentState.eType == ET_NPC)
	{
		const centity_t* cent = &cg_entities[cg.crosshairclientNum];

		switch (cent->currentState.NPC_class)
		{
		case CLASS_REBEL:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Rebel"), colorTable[CT_GREEN]);
			break;
		case CLASS_WOOKIE:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Wookie"), colorTable[CT_GREEN]);
			break;
		case CLASS_JEDI:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Jedi Knight"), colorTable[CT_CYAN]);
			break;
		case CLASS_KYLE:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("kyle Katarn"), colorTable[CT_VLTPURPLE1]);
			break;
		case CLASS_YODA:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Master Yoda"), colorTable[CT_VLTPURPLE1]);
			break;
		case CLASS_LUKE:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Luke Skywalker"), colorTable[CT_VLTPURPLE1]);
			break;
		case CLASS_JAN:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Jan Ors"), colorTable[CT_MAGENTA]);
			break;
		case CLASS_MONMOTHA:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Monmothma"), colorTable[CT_GREEN]);
			break;
		case CLASS_MORGANKATARN:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Morgan Katarn"), colorTable[CT_LTORANGE]);
			break;
		case CLASS_GONK:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("gonk"), colorTable[CT_LTORANGE]);
			break;
		case CLASS_STORMTROOPER:
		case CLASS_CLONETROOPER:
		case CLASS_STORMCOMMANDO:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Trooper"), colorTable[CT_RED]);
			break;
		case CLASS_SWAMPTROOPER:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Swamptrooper"), colorTable[CT_RED]);
			break;
		case CLASS_IMPWORKER:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("ImpWorker"), colorTable[CT_RED]);
			break;
		case CLASS_IMPERIAL:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Imperial"), colorTable[CT_RED]);
			break;
		case CLASS_GALAKMECH:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("GalakMech"), colorTable[CT_RED]);
			break;
		case CLASS_SHADOWTROOPER:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Shadowtrooper"), colorTable[CT_DKRED1]);
			break;
		case CLASS_COMMANDO:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Commando"), colorTable[CT_RED]);
			break;
		case CLASS_TAVION:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Tavion"), colorTable[CT_VLTPURPLE1]);
			break;
		case CLASS_ALORA:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Alora"), colorTable[CT_VLTPURPLE1]);
			break;
		case CLASS_DESANN:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Desann"), colorTable[CT_VLTPURPLE1]);
			break;
		case CLASS_REBORN:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Reborn"), colorTable[CT_VLTPURPLE1]);
			break;
		case CLASS_BOBAFETT:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Bobafett"), colorTable[CT_LTORANGE]);
			break;
		case CLASS_MANDO:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Mandolorian"), colorTable[CT_VLTPURPLE1]);
			break;
		case CLASS_ATST:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("ATST"), colorTable[CT_BLUE]);
			break;
		case CLASS_HOWLER:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Howler"), colorTable[CT_MDGREY]);
			break;
		case CLASS_MINEMONSTER:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("MineMonster"), colorTable[CT_MDGREY]);
			break;
		case CLASS_RANCOR:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Rancor"), colorTable[CT_MDGREY]);
			break;
		case CLASS_WAMPA:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Wampa"), colorTable[CT_MDGREY]);
			break;
		case CLASS_VEHICLE:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Vehicle"), colorTable[CT_BLUE]);
			break;
		case CLASS_BESPIN_COP:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Bespin Cop"), colorTable[CT_GREEN]);
			break;
		case CLASS_LANDO:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Lando"), colorTable[CT_GREEN]);
			break;
		case CLASS_PRISONER:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Prisoner"), colorTable[CT_GREEN]);
			break;
		case CLASS_GALAK:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Galak"), colorTable[CT_RED]);
			break;
		case CLASS_GRAN:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Gran"), colorTable[CT_RED]);
			break;
		case CLASS_REELO:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Reelo"), colorTable[CT_RED]);
			break;
		case CLASS_MURJJ:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Murjj"), colorTable[CT_RED]);
			break;
		case CLASS_RODIAN:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Rodian"), colorTable[CT_RED]);
			break;
		case CLASS_TRANDOSHAN:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Trandoshan"), colorTable[CT_RED]);
			break;
		case CLASS_UGNAUGHT:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Ugnaught"), colorTable[CT_GREEN]);
			break;
		case CLASS_WEEQUAY:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Weequay"), colorTable[CT_RED]);
			break;
		case CLASS_BARTENDER:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Bartender"), colorTable[CT_BLUE]);
			break;
		case CLASS_JAWA:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Jawa"), colorTable[CT_MAGENTA]);
			break;
		case CLASS_REMOTE:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Remote Droid"), colorTable[CT_YELLOW]);
			break;
		case CLASS_SEEKER:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Seeker Droid"), colorTable[CT_YELLOW]);
			break;
		case CLASS_PROTOCOL:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Protocol Droid"), colorTable[CT_YELLOW]);
			break;
		case CLASS_R2D2:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("R2 Astromech Droid"), colorTable[CT_YELLOW]);
			break;
		case CLASS_PROBE:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Probe Droid"), colorTable[CT_YELLOW]);
			break;
		case CLASS_R5D2:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("R5 Astromech Droid"), colorTable[CT_YELLOW]);
			break;
		case CLASS_DROIDEKA:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Droideka"), colorTable[CT_MAGENTA]);
			break;
		case CLASS_BATTLEDROID:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Battle Droid"), colorTable[CT_MAGENTA]);
			break;
		case CLASS_SBD:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Super Battle Droid"), colorTable[CT_MAGENTA]);
			break;
		case CLASS_ASSASSIN_DROID:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Assassin droid"), colorTable[CT_YELLOW]);
			break;
		case CLASS_SABER_DROID:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Saber Droid"), colorTable[CT_MAGENTA]);
			break;
		case CLASS_TUSKEN:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Tusken"), colorTable[CT_VLTORANGE]);
			break;
		case CLASS_MARK1:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Droid"), colorTable[CT_VLTORANGE]);
			break;
		case CLASS_MARK2:
			CG_DrawSmallStringColor(320 - w / 2, 170, va("Droid"), colorTable[CT_VLTORANGE]);
			break;
		default:
			//CG_DrawSmallStringColor(320 - w / 2, 170, va("Civilian"), colorTable[CT_VDKORANGE]);
			break;
		}
	}
	else
	{
		if (cg_entities[cg.crosshairclientNum].currentState.eType == ET_ITEM)
		{
			CG_DrawPic(50, 285, 32, 32, cgs.media.useableHint);
		}
		else
		{
			if (cgs.gametype == GT_FFA)
			{
				CG_DrawSmallStringColor(320 - w / 2, 170, name, colorTable[CT_RED]);
			}
			else
			{
				if (cgs.clientinfo[cg.crosshairclientNum].team == TEAM_RED)
				{
					CG_DrawSmallStringColor(320 - w / 2, 170, name, colorTable[CT_RED]);
				}
				else if (cgs.clientinfo[cg.crosshairclientNum].team == TEAM_BLUE)
				{
					CG_DrawSmallStringColor(320 - w / 2, 170, name, colorTable[CT_GREEN]);
				}
				else if (cgs.clientinfo[cg.crosshairclientNum].team == TEAM_SPECTATOR)
				{
					CG_DrawSmallStringColor(320 - w / 2, 170, name, colorTable[CT_CYAN]);
				}
				else if (cgs.clientinfo[cg.crosshairclientNum].npcteam == NPCTEAM_PLAYER)
				{
					CG_DrawSmallStringColor(320 - w / 2, 170, name, colorTable[CT_GREEN]);
				}
				else if (cgs.clientinfo[cg.crosshairclientNum].npcteam == NPCTEAM_ENEMY)
				{
					CG_DrawSmallStringColor(320 - w / 2, 170, name, colorTable[CT_RED]);
				}
				else
				{
					CG_DrawSmallStringColor(320 - w / 2, 170, name, colorTable[CT_RED]);
				}
			}
		}
	}

	trap->R_SetColor(NULL);
}

//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
//static void CG_DrawSpectator(void)
//{
//	const char* s = CG_GetStringEdString("MP_INGAME", "SPECTATOR");
//	if ((cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) &&
//		cgs.duelist1 != -1 &&
//		cgs.duelist2 != -1)
//	{
//		char text[1024];
//		const int size = 64;
//
//		if (cgs.gametype == GT_POWERDUEL && cgs.duelist3 != -1)
//		{
//			Com_sprintf(text, sizeof text, "%s^7 %s %s^7 %s %s", cgs.clientinfo[cgs.duelist1].name,
//				CG_GetStringEdString("MP_INGAME", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name,
//				CG_GetStringEdString("MP_INGAME", "AND"), cgs.clientinfo[cgs.duelist3].name);
//		}
//		else
//		{
//			Com_sprintf(text, sizeof text, "%s^7 %s %s", cgs.clientinfo[cgs.duelist1].name,
//				CG_GetStringEdString("MP_INGAME", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name);
//		}
//		CG_Text_Paint(320 - CG_Text_Width(text, 1.0f, 3) / 2, 420, 1.0f, colorWhite, text, 0, 0, 0, 3);
//
//		trap->R_SetColor(colorTable[CT_WHITE]);
//		if (cgs.clientinfo[cgs.duelist1].modelIcon)
//		{
//			CG_DrawPic(10, SCREEN_HEIGHT - size * 1.5, size, size, cgs.clientinfo[cgs.duelist1].modelIcon);
//		}
//		if (cgs.clientinfo[cgs.duelist2].modelIcon)
//		{
//			CG_DrawPic(SCREEN_WIDTH - size - 10, SCREEN_HEIGHT - size * 1.5, size, size,
//				cgs.clientinfo[cgs.duelist2].modelIcon);
//		}
//
//		// nmckenzie: DUEL_HEALTH
//		if (cgs.gametype == GT_DUEL)
//		{
//			if (cgs.showDuelHealths >= 1)
//			{
//				// draw the healths on the two guys - how does this interact with power duel, though?
//				CG_DrawDuelistHealth(10, SCREEN_HEIGHT - size * 1.5 - 12, 64, 8, 1);
//				CG_DrawDuelistHealth(SCREEN_WIDTH - size - 10, SCREEN_HEIGHT - size * 1.5 - 12, 64, 8, 2);
//			}
//		}
//
//		if (cgs.gametype != GT_POWERDUEL)
//		{
//			Com_sprintf(text, sizeof text, "%i/%i", cgs.clientinfo[cgs.duelist1].score, cgs.fraglimit);
//			CG_Text_Paint(42 - CG_Text_Width(text, 1.0f, 2) / 2, SCREEN_HEIGHT - size * 1.5 + 64, 1.0f, colorWhite,
//				text, 0, 0, 0, 2);
//
//			Com_sprintf(text, sizeof text, "%i/%i", cgs.clientinfo[cgs.duelist2].score, cgs.fraglimit);
//			CG_Text_Paint(SCREEN_WIDTH - size + 22 - CG_Text_Width(text, 1.0f, 2) / 2, SCREEN_HEIGHT - size * 1.5 + 64,
//				1.0f, colorWhite, text, 0, 0, 0, 2);
//		}
//
//		if (cgs.gametype == GT_POWERDUEL && cgs.duelist3 != -1)
//		{
//			if (cgs.clientinfo[cgs.duelist3].modelIcon)
//			{
//				CG_DrawPic(SCREEN_WIDTH - size - 10, SCREEN_HEIGHT - size * 2.8, size, size,
//					cgs.clientinfo[cgs.duelist3].modelIcon);
//			}
//		}
//	}
//	else
//	{
//		CG_Text_Paint(320 - CG_Text_Width(s, 1.0f, 3) / 2, 420, 1.0f, colorWhite, s, 0, 0, 0, 3);
//	}
//
//	if (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL)
//	{
//		s = CG_GetStringEdString("MP_INGAME", "WAITING_TO_PLAY"); // "waiting to play";
//		CG_Text_Paint(320 - CG_Text_Width(s, 1.0f, 3) / 2, 440, 1.0f, colorWhite, s, 0, 0, 0, 3);
//	}
//	else //if ( cgs.gametype >= GT_TEAM )
//	{
//		//s = "press ESC and use the JOIN menu to play";
//		s = CG_GetStringEdString("MP_INGAME", "SPEC_CHOOSEJOIN");
//		CG_Text_Paint(320 - CG_Text_Width(s, 1.0f, 3) / 2, 440, 1.0f, colorWhite, s, 0, 0, 0, 3);
//	}
//}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void)
{
	const char* s, * s_parm = NULL;
	char s_yes[20] = { 0 }, s_no[20] = { 0 }, s_vote[20] = { 0 }, s_cmd[100] = { 0 };

	if (!cgs.voteTime)
		return;

	// play a talk beep whenever it is modified
	if (cgs.voteModified)
	{
		cgs.voteModified = qfalse;
		trap->S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	}

	int sec = (VOTE_TIME - (cg.time - cgs.voteTime)) / 1000;
	if (sec < 0)
	{
		sec = 0;
	}

	if (!Q_strncmp(cgs.voteString, "map_restart", 11))
		trap->SE_GetStringTextString("MENUS_RESTART_MAP", s_cmd, sizeof s_cmd);
	else if (!Q_strncmp(cgs.voteString, "vstr nextmap", 12))
		trap->SE_GetStringTextString("MENUS_NEXT_MAP", s_cmd, sizeof s_cmd);
	else if (!Q_strncmp(cgs.voteString, "g_doWarmup", 10))
		trap->SE_GetStringTextString("MENUS_WARMUP", s_cmd, sizeof s_cmd);
	else if (!Q_strncmp(cgs.voteString, "g_gametype", 10))
	{
		trap->SE_GetStringTextString("MENUS_GAME_TYPE", s_cmd, sizeof s_cmd);

		if (!Q_stricmp("Free For All", cgs.voteString + 11)) s_parm = CG_GetStringEdString("MENUS", "FREE_FOR_ALL");
		else if (!Q_stricmp("Duel", cgs.voteString + 11)) s_parm = CG_GetStringEdString("MENUS", "DUEL");
		else if (!Q_stricmp("Holocron FFA", cgs.voteString + 11)) s_parm =
			CG_GetStringEdString("MENUS", "HOLOCRON_FFA");
		else if (!Q_stricmp("Power Duel", cgs.voteString + 11)) s_parm = CG_GetStringEdString("MENUS", "POWERDUEL");
		else if (!Q_stricmp("Team FFA", cgs.voteString + 11)) s_parm = CG_GetStringEdString("MENUS", "TEAM_FFA");
		else if (!Q_stricmp("Siege", cgs.voteString + 11)) s_parm = CG_GetStringEdString("MENUS", "SIEGE");
		else if (!Q_stricmp("Capture the Flag", cgs.voteString + 11))
			s_parm = CG_GetStringEdString(
				"MENUS", "CAPTURE_THE_FLAG");
		else if (!Q_stricmp("Capture the Ysalamiri", cgs.voteString + 11))
			s_parm = CG_GetStringEdString(
				"MENUS", "CAPTURE_THE_YSALIMARI");
		else if (!Q_stricmp("Single Player", cgs.voteString + 11)) s_parm = CG_GetStringEdString("OJP_MENUS", "COOP");
	}
	else if (!Q_strncmp(cgs.voteString, "map", 3))
	{
		trap->SE_GetStringTextString("MENUS_NEW_MAP", s_cmd, sizeof s_cmd);
		s_parm = cgs.voteString + 4;
	}
	else if (!Q_strncmp(cgs.voteString, "kick", 4))
	{
		trap->SE_GetStringTextString("MENUS_KICK_PLAYER", s_cmd, sizeof s_cmd);
		s_parm = cgs.voteString + 5;
	}
	else
	{
		// custom votes like ampoll, cointoss, etc
		s_parm = cgs.voteString;
	}

	trap->SE_GetStringTextString("MENUS_VOTE", s_vote, sizeof s_vote);
	trap->SE_GetStringTextString("MENUS_YES", s_yes, sizeof s_yes);
	trap->SE_GetStringTextString("MENUS_NO", s_no, sizeof s_no);

	if (s_parm && s_parm[0])
		s = va("%s(%i):<%s %s> %s:%i %s:%i", s_vote, sec, s_cmd, s_parm, s_yes, cgs.voteYes, s_no, cgs.voteNo);
	else
		s = va("%s(%i):<%s> %s:%i %s:%i", s_vote, sec, s_cmd, s_yes, cgs.voteYes, s_no, cgs.voteNo);
	CG_DrawSmallString(4, 58, s, 1.0f);
	if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR)
	{
		s = CG_GetStringEdString("MP_INGAME", "OR_PRESS_ESC_THEN_CLICK_VOTE"); //	s = "or press ESC then click Vote";
		CG_DrawSmallString(4, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0f);
	}
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void)
{
	char* s;
	int cs_offset;

	if (cgs.clientinfo[cg.clientNum].team == TEAM_RED)
		cs_offset = 0;
	else if (cgs.clientinfo[cg.clientNum].team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if (!cgs.teamVoteTime[cs_offset])
	{
		return;
	}

	// play a talk beep whenever it is modified
	if (cgs.teamVoteModified[cs_offset])
	{
		cgs.teamVoteModified[cs_offset] = qfalse;
		//		trap->S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	int sec = (VOTE_TIME - (cg.time - cgs.teamVoteTime[cs_offset])) / 1000;
	if (sec < 0)
	{
		sec = 0;
	}
	if (strstr(cgs.teamVoteString[cs_offset], "leader"))
	{
		int i = 0;

		while (cgs.teamVoteString[cs_offset][i] && cgs.teamVoteString[cs_offset][i] != ' ')
		{
			i++;
		}

		if (cgs.teamVoteString[cs_offset][i] == ' ')
		{
			int vote_index = 0;
			char vote_index_str[256];

			i++;

			while (cgs.teamVoteString[cs_offset][i])
			{
				vote_index_str[vote_index] = cgs.teamVoteString[cs_offset][i];
				vote_index++;
				i++;
			}
			vote_index_str[vote_index] = 0;

			vote_index = atoi(vote_index_str);

			s = va("TEAMVOTE(%i):(Make %s the new team leader) yes:%i no:%i", sec, cgs.clientinfo[vote_index].name,
				cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset]);
		}
		else
		{
			s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
				cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset]);
		}
	}
	else
	{
		s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
			cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset]);
	}
	CG_DrawSmallString(4, 90, s, 1.0f);
}

static qboolean CG_DrawScoreboard()
{
	return CG_DrawOldScoreboard();
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission(void)
{
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow(void)
{
	const char* s;

	if (!(cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		return qfalse;
	}

	if (cgs.gametype == GT_POWERDUEL)
	{
		const clientInfo_t* ci = &cgs.clientinfo[cg.snap->ps.clientNum];

		if (ci->duelTeam == DUELTEAM_LONE)
		{
			s = CG_GetStringEdString("MP_INGAME", "FOLLOWINGLONE");
		}
		else if (ci->duelTeam == DUELTEAM_DOUBLE)
		{
			s = CG_GetStringEdString("MP_INGAME", "FOLLOWINGDOUBLE");
		}
		else
		{
			s = CG_GetStringEdString("MP_INGAME", "FOLLOWING");
		}
	}
	else
	{
		s = CG_GetStringEdString("MP_INGAME", "FOLLOWING");
	}

	CG_Text_Paint(320 - CG_Text_Width(s, 0.5f, FONT_SMALL) / 2, 30, 0.5f, colorWhite, s, 0, 0, 0, FONT_SMALL);

	s = cgs.clientinfo[cg.snap->ps.clientNum].name;
	CG_Text_Paint(320 - CG_Text_Width(s, 1.0f, FONT_SMALL) / 2, 60, 1.0f, colorWhite, s, 0, 0, 0, FONT_SMALL);

	return qtrue;
}

#if 0
static void CG_DrawTemporaryStats()
{ //placeholder for testing (draws ammo and force power)
	char s[512];

	if (!cg.snap)
	{
		return;
	}

	sprintf(s, "Force: %i", cg.snap->ps.fd.forcePower);

	CG_DrawBigString(SCREEN_WIDTH - 164, SCREEN_HEIGHT - dmgIndicSize, s, 1.0f);

	sprintf(s, "Ammo: %i", cg.snap->ps.ammo[weaponData[cg.snap->ps.weapon].ammoIndex]);

	CG_DrawBigString(SCREEN_WIDTH - 164, SCREEN_HEIGHT - 112, s, 1.0f);

	sprintf(s, "Health: %i", cg.snap->ps.stats[STAT_HEALTH]);

	CG_DrawBigString(8, SCREEN_HEIGHT - dmgIndicSize, s, 1.0f);

	sprintf(s, "Armor: %i", cg.snap->ps.stats[STAT_ARMOR]);

	CG_DrawBigString(8, SCREEN_HEIGHT - 112, s, 1.0f);
}
#endif

/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning(void)
{
#if 0
	const char* s;
	int			w;

	if (!cg_drawStatus.integer)
	{
		return;
	}

	if (cg_drawAmmoWarning.integer == 0) {
		return;
	}

	if (!cg.lowAmmoWarning) {
		return;
	}

	if (cg.lowAmmoWarning == 2) {
		s = "OUT OF AMMO";
	}
	else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 64, s, 1.0f);
#endif
}

/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup(void)
{
	int w;
	const char* s;

	int sec = cg.warmup;
	if (!sec)
	{
		return;
	}

	if (sec < 0)
	{
		//		s = "Waiting for players";
		s = CG_GetStringEdString("MP_INGAME", "WAITING_FOR_PLAYERS");
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
		CG_DrawBigString(320 - w / 2, 24, s, 1.0f);
		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL)
	{
		clientInfo_t* ci1 = NULL;
		clientInfo_t* ci2 = NULL;
		clientInfo_t* ci3 = NULL;

		if (cgs.gametype == GT_POWERDUEL)
		{
			if (cgs.duelist1 != -1)
			{
				ci1 = &cgs.clientinfo[cgs.duelist1];
			}
			if (cgs.duelist2 != -1)
			{
				ci2 = &cgs.clientinfo[cgs.duelist2];
			}
			if (cgs.duelist3 != -1)
			{
				ci3 = &cgs.clientinfo[cgs.duelist3];
			}
		}
		else
		{
			for (int i = 0; i < cgs.maxclients; i++)
			{
				if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE)
				{
					if (!ci1)
					{
						ci1 = &cgs.clientinfo[i];
					}
					else
					{
						ci2 = &cgs.clientinfo[i];
					}
				}
			}
		}
		if (ci1 && ci2)
		{
			if (ci3)
			{
				s = va("%s vs %s and %s", ci1->name, ci2->name, ci3->name);
			}
			else
			{
				s = va("%s vs %s", ci1->name, ci2->name);
			}
			w = CG_Text_Width(s, 0.6f, FONT_MEDIUM);
			CG_Text_Paint(320 - w / 2, 60, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
		}
	}
	else
	{
		if (cgs.gametype == GT_FFA) s = CG_GetStringEdString("MENUS", "FREE_FOR_ALL"); //"Free For All";
		else if (cgs.gametype == GT_HOLOCRON) s = CG_GetStringEdString("MENUS", "HOLOCRON_FFA"); //"Holocron FFA";
		else if (cgs.gametype == GT_JEDIMASTER) s = "Jedi Master";
		else if (cgs.gametype == GT_TEAM) s = CG_GetStringEdString("MENUS", "TEAM_FFA"); //"Team FFA";
		else if (cgs.gametype == GT_SIEGE) s = CG_GetStringEdString("MENUS", "SIEGE"); //"Siege";
		else if (cgs.gametype == GT_CTF) s = CG_GetStringEdString("MENUS", "CAPTURE_THE_FLAG"); //"Capture the Flag";
		else if (cgs.gametype == GT_CTY) s = CG_GetStringEdString("MENUS", "CAPTURE_THE_YSALIMARI");
		//"Capture the Ysalamiri";
		else if (cgs.gametype == GT_SINGLE_PLAYER) s = "missions";
		else s = "";
		w = CG_Text_Width(s, 1.5f, FONT_MEDIUM);
		CG_Text_Paint(320 - w / 2, 90, 1.5f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
	}

	sec = (sec - cg.time) / 1000;
	if (sec < 0)
	{
		cg.warmup = 0;
		sec = 0;
	}
	//	s = va( "Starts in: %i", sec + 1 );
	s = va("%s: %i", CG_GetStringEdString("MP_INGAME", "STARTS_IN"), sec + 1);
	if (sec != cg.warmupCount)
	{
		cg.warmupCount = sec;

		if (cgs.gametype != GT_SIEGE)
		{
			switch (sec)
			{
			case 0:
				trap->S_StartLocalSound(cgs.media.count1Sound, CHAN_ANNOUNCER);
				break;
			case 1:
				trap->S_StartLocalSound(cgs.media.count2Sound, CHAN_ANNOUNCER);
				break;
			case 2:
				trap->S_StartLocalSound(cgs.media.count3Sound, CHAN_ANNOUNCER);
				break;
			default:
				break;
			}
		}
	}
	float scale;
	switch (cg.warmupCount)
	{
	case 0:
		scale = 1.25f;
		break;
	case 1:
		scale = 1.15f;
		break;
	case 2:
		scale = 1.05f;
		break;
	default:
		scale = 0.9f;
		break;
	}

	w = CG_Text_Width(s, scale, FONT_MEDIUM);
	CG_Text_Paint(320 - w / 2, 125, scale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
}

//==================================================================================
/*
=================
CG_DrawTimedMenus
=================
*/
static void CG_DrawTimedMenus()
{
	if (cg.voiceTime)
	{
		const int t = cg.time - cg.voiceTime;
		if (t > 2500)
		{
			Menus_CloseByName("voiceMenu");
			trap->Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}

static void CG_DrawFlagStatus()
{
	int my_flag_taken_shader;
	int their_flag_shader;
	int start_draw_pos = 2;
	const int ico_size = 32;

	//Raz: was missing this
	trap->R_SetColor(NULL);

	if (!cg.snap)
	{
		return;
	}

	if (cgs.gametype != GT_CTF && cgs.gametype != GT_CTY)
	{
		return;
	}

	const int team = cg.snap->ps.persistant[PERS_TEAM];

	if (cgs.gametype == GT_CTY)
	{
		if (team == TEAM_RED)
		{
			my_flag_taken_shader = trap->R_RegisterShaderNoMip("gfx/hud/mpi_rflag_x");
			their_flag_shader = trap->R_RegisterShaderNoMip("gfx/hud/mpi_bflag_ys");
		}
		else
		{
			my_flag_taken_shader = trap->R_RegisterShaderNoMip("gfx/hud/mpi_bflag_x");
			their_flag_shader = trap->R_RegisterShaderNoMip("gfx/hud/mpi_rflag_ys");
		}
	}
	else
	{
		if (team == TEAM_RED)
		{
			my_flag_taken_shader = trap->R_RegisterShaderNoMip("gfx/hud/mpi_rflag_x");
			their_flag_shader = trap->R_RegisterShaderNoMip("gfx/hud/mpi_bflag");
		}
		else
		{
			my_flag_taken_shader = trap->R_RegisterShaderNoMip("gfx/hud/mpi_bflag_x");
			their_flag_shader = trap->R_RegisterShaderNoMip("gfx/hud/mpi_rflag");
		}
	}

	if (CG_YourTeamHasFlag())
	{
		CG_DrawPic(2, 330 - start_draw_pos, ico_size, ico_size, their_flag_shader);
		start_draw_pos += ico_size + 2;
	}

	if (CG_OtherTeamHasFlag())
	{
		CG_DrawPic(2, 330 - start_draw_pos, ico_size, ico_size, my_flag_taken_shader);
	}
}

//draw meter showing sprint fuel when it's not full
#define SPFUELBAR_H			100.0f
#define SPFUELBAR_W			5.0f
#define SPFUELBAR_X			(SCREEN_WIDTH-SPFUELBAR_W-4.0f)
#define SPFUELBAR_Y			240.0f

static void CG_DrawSprintFuel()
{
	vec4_t aColor;
	vec4_t b_color;
	vec4_t cColor;
	const float x = SPFUELBAR_X;
	const float y = SPFUELBAR_Y;
	float percent = (float)cg.snap->ps.sprintFuel / 100.0f * SPFUELBAR_H;

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (percent > SPFUELBAR_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//color of the bar
	aColor[0] = 0.0f;
	aColor[1] = 0.3f;
	aColor[2] = 0.6f;
	aColor[3] = 0.8f;

	//color of the border
	b_color[0] = 0.0f;
	b_color[1] = 0.0f;
	b_color[2] = 0.0f;
	b_color[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, SPFUELBAR_W, SPFUELBAR_H, 0.5f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x + 0.5f, y + 0.5f + (SPFUELBAR_H - percent), SPFUELBAR_W - 0.5f,
		SPFUELBAR_H - 0.5f - (SPFUELBAR_H - percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x + 0.5f, y + 0.5f, SPFUELBAR_W - 0.5f, SPFUELBAR_H - percent, cColor);
}

//draw meter showing jetpack fuel when it's not full

#define JPFUELBAR_H			100.0f
#define JPFUELBAR_W			5.0f
#define JPFUELBAR_X			(SCREEN_WIDTH-JPFUELBAR_W-4.0f)
#define JPFUELBAR_Y			240.0f

static void CG_DrawJetpackFuel(void)
{
	vec4_t aColor;
	vec4_t b_color;
	vec4_t cColor;
	float x = JPFUELBAR_X;
	const float y = JPFUELBAR_Y;
	float percent = (float)cg.snap->ps.jetpackFuel / 100.0f * JPFUELBAR_H;

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (percent > JPFUELBAR_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	if (cg.snap->ps.sprintFuel < 100)
	{
		x -= SPFUELBAR_W + 4.0f;
	}

	//color of the bar
	aColor[0] = 0.5f;
	aColor[1] = 0.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.8f;

	//color of the border
	b_color[0] = 0.0f;
	b_color[1] = 0.0f;
	b_color[2] = 0.0f;
	b_color[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, JPFUELBAR_W, JPFUELBAR_H, 0.5f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x + 0.5f, y + 0.5f + (JPFUELBAR_H - percent), JPFUELBAR_W - 0.5f,
		JPFUELBAR_H - 0.5f - (JPFUELBAR_H - percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x + 0.5f, y + 0.5f, JPFUELBAR_W - 0.5f, JPFUELBAR_H - percent, cColor);
}

//draw meter showing cloak fuel when it's not full
#define CLFUELBAR_H			100.0f
#define CLFUELBAR_W			5.0f
#define CLFUELBAR_X			(SCREEN_WIDTH-CLFUELBAR_W-4.0f)
#define CLFUELBAR_Y			240.0f

static void CG_DrawCloakFuel(void)
{
	vec4_t aColor;
	vec4_t b_color;
	vec4_t cColor;
	float x = CLFUELBAR_X;
	const float y = CLFUELBAR_Y;
	float percent = (float)cg.snap->ps.cloakFuel / 100.0f * CLFUELBAR_H;

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (percent > CLFUELBAR_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	if (cg.snap->ps.sprintFuel < 100)
	{
		x -= SPFUELBAR_W + 4.0f;
	}

	if (cg.snap->ps.jetpackFuel < 100)
	{
		x -= JPFUELBAR_W + 8.0f;
	}

	//color of the bar
	aColor[0] = 0.0f;
	aColor[1] = 0.0f;
	aColor[2] = 0.6f;
	aColor[3] = 0.8f;

	//color of the border
	b_color[0] = 0.0f;
	b_color[1] = 0.0f;
	b_color[2] = 0.0f;
	b_color[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.1f;
	cColor[1] = 0.1f;
	cColor[2] = 0.3f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, CLFUELBAR_W, CLFUELBAR_H, 0.5f, colorTable[CT_BLACK]);

	//now draw the part to show how much fuel there is in the color specified
	CG_FillRect(x + 0.5f, y + 0.5f + (CLFUELBAR_H - percent), CLFUELBAR_W - 0.5f,
		CLFUELBAR_H - 0.5f - (CLFUELBAR_H - percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x + 0.5f, y + 0.5f, CLFUELBAR_W - 0.5f, CLFUELBAR_H - percent, cColor);
}

//draw meter showing e-web health when it is in use
#define EWEBHEALTH_H			100.0f
#define EWEBHEALTH_W			5.0f
#define EWEBHEALTH_X			(SCREEN_WIDTH-EWEBHEALTH_W-4.0f)
#define EWEBHEALTH_Y			240.0f

static void CG_DrawEWebHealth(void)
{
	vec4_t aColor;
	vec4_t cColor;
	float x = EWEBHEALTH_X;
	const float y = EWEBHEALTH_Y;
	const centity_t* eweb = &cg_entities[cg.predictedPlayerState.emplacedIndex];
	float percent = (float)eweb->currentState.health / eweb->currentState.maxhealth * EWEBHEALTH_H;

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (percent > EWEBHEALTH_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	if (cg.snap->ps.sprintFuel < 100)
	{
		x -= SPFUELBAR_W + 4.0f;
	}

	if (cg.snap->ps.jetpackFuel < 100)
	{
		x -= JPFUELBAR_W + 8.0f;
	}
	if (cg.snap->ps.cloakFuel < 100)
	{
		x -= JPFUELBAR_W + 12.0f;
	}

	//color of the bar
	aColor[0] = 0.5f;
	aColor[1] = 0.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.8f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, EWEBHEALTH_W, EWEBHEALTH_H, 0.5f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x + 0.5f, y + 0.5f + (EWEBHEALTH_H - percent), EWEBHEALTH_W - 0.5f,
		EWEBHEALTH_H - 0.5f - (EWEBHEALTH_H - percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x + 0.5f, y + 0.5f, EWEBHEALTH_W - 0.5f, EWEBHEALTH_H - percent, cColor);
}

int cgRageTime = 0;
int cgRageFadeTime = 0;
float cgRageFadeVal = 0;

int cgRageRecTime = 0;
int cgRageRecFadeTime = 0;
float cgRageRecFadeVal = 0;

int cgAbsorbTime = 0;
int cgAbsorbFadeTime = 0;
float cgAbsorbFadeVal = 0;

int cgProtectTime = 0;
int cgProtectFadeTime = 0;
float cgProtectFadeVal = 0;

int cgYsalTime = 0;
int cgYsalFadeTime = 0;
float cgYsalFadeVal = 0;

qboolean gCGHasFallVector = qfalse;
vec3_t gCGFallVector;

/*
=================
CG_Draw2D
=================
*/
extern int cgSiegeRoundState;
extern int cgSiegeRoundTime;

extern int team1Timed;
extern int team2Timed;

int cg_beatingSiegeTime = 0;

int cgSiegeRoundBeganTime = 0;
int cgSiegeRoundCountTime = 0;

static void CG_DrawSiegeTimer(const int timeRemaining, const qboolean isMyTeam)
{
	int f_color;
	int minutes = 0;
	char time_str[1024];

	const menuDef_t* menu_hud = Menus_FindByName("mp_timer");
	if (!menu_hud)
	{
		return;
	}

	const itemDef_t* item = Menu_FindItemByName(menu_hud, "frame");

	if (item)
	{
		trap->R_SetColor(item->window.foreColor);
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);
	}

	int seconds = timeRemaining;

	while (seconds >= 60)
	{
		minutes++;
		seconds -= 60;
	}

	strcpy(time_str, va("%i:%02i", minutes, seconds));

	if (isMyTeam)
	{
		f_color = CT_HUD_RED;
	}
	else
	{
		f_color = CT_HUD_GREEN;
	}

	item = Menu_FindItemByName(menu_hud, "timer");
	if (item)
	{
		CG_DrawProportionalString(
			item->window.rect.x,
			item->window.rect.y,
			time_str,
			UI_SMALLFONT | UI_DROPSHADOW,
			colorTable[f_color]);
	}
}

static void CG_DrawSiegeDeathTimer(const int timeRemaining)
{
	int minutes = 0;
	char time_str[1024];

	const menuDef_t* menu_hud = Menus_FindByName("mp_timer");
	if (!menu_hud)
	{
		return;
	}

	itemDef_t* item = Menu_FindItemByName(menu_hud, "frame");

	if (item)
	{
		trap->R_SetColor(item->window.foreColor);
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background);
	}

	int seconds = timeRemaining;

	while (seconds >= 60)
	{
		minutes++;
		seconds -= 60;
	}

	if (seconds < 10)
	{
		strcpy(time_str, va("%i:0%i", minutes, seconds));
	}
	else
	{
		strcpy(time_str, va("%i:%i", minutes, seconds));
	}

	item = Menu_FindItemByName(menu_hud, "deathtimer");
	if (item)
	{
		CG_DrawProportionalString(
			item->window.rect.x,
			item->window.rect.y,
			time_str,
			UI_SMALLFONT | UI_DROPSHADOW,
			item->window.foreColor);
	}
}

int cgSiegeEntityRender = 0;

static void CG_DrawSiegeHUDItem(void)
{
	void* g2;
	qhandle_t handle;
	vec3_t origin, angles;
	vec3_t mins, maxs;
	const centity_t* cent = &cg_entities[cgSiegeEntityRender];

	if (cent->ghoul2)
	{
		g2 = cent->ghoul2;
		handle = 0;
	}
	else
	{
		handle = cgs.game_models[cent->currentState.modelIndex];
		g2 = NULL;
	}

	if (handle)
	{
		trap->R_ModelBounds(handle, mins, maxs);
	}
	else
	{
		VectorSet(mins, -16, -16, -20);
		VectorSet(maxs, 16, 16, 32);
	}

	origin[2] = -0.5f * (mins[2] + maxs[2]);
	origin[1] = 0.5f * (mins[1] + maxs[1]);
	const float len = 0.5f * (maxs[2] - mins[2]);
	origin[0] = len / 0.268;

	VectorClear(angles);
	angles[YAW] = cg.autoAngles[YAW];

	CG_Draw3DModel(8, 8, 64, 64, handle, g2, cent->currentState.g2radius, 0, origin, angles);

	cgSiegeEntityRender = 0; //reset for next frame
}

/*====================================
chatbox functionality -rww
====================================*/
#define	CHATBOX_CUTOFF_LEN	550
#define	CHATBOX_FONT_HEIGHT	20

//utility func, insert a string into a string at the specified
//place (assuming this will not overflow the buffer)
static void CG_ChatBox_StrInsert(char* buffer, const int place, const char* str)
{
	const int ins_len = strlen(str);
	int i = strlen(buffer);
	int k = 0;

	buffer[i + ins_len + 1] = 0; //terminate the string at its new length
	while (i >= place)
	{
		buffer[i + ins_len] = buffer[i];
		i--;
	}

	i++;
	while (k < ins_len)
	{
		buffer[i] = str[k];
		i++;
		k++;
	}
}

//add chatbox string
void CG_ChatBox_AddString(char* chat_str)
{
	chatBoxItem_t* chat = &cg.chatItems[cg.chatItemActive];

	if (cg_chatBox.integer <= 0)
	{
		//don't bother then.
		return;
	}

	memset(chat, 0, sizeof(chatBoxItem_t));

	if (strlen(chat_str) > sizeof chat->string)
	{
		//too long, terminate at proper len.
		chat_str[sizeof chat->string - 1] = 0;
	}

	strcpy(chat->string, chat_str);
	chat->time = cg.time + cg_chatBox.integer;

	chat->lines = 1;

	float chat_len = CG_Text_Width(chat->string, 1.0f, FONT_SMALL);
	if (chat_len > CHATBOX_CUTOFF_LEN)
	{
		//we have to break it into segments...
		int i = 0;
		int last_line_pt = 0;
		char s[2];

		chat_len = 0;
		while (chat->string[i])
		{
			s[0] = chat->string[i];
			s[1] = 0;
			chat_len += CG_Text_Width(s, 0.65f, FONT_SMALL);

			if (chat_len >= CHATBOX_CUTOFF_LEN)
			{
				int j = i;
				while (j > 0 && j > last_line_pt)
				{
					if (chat->string[j] == ' ')
					{
						break;
					}
					j--;
				}
				if (chat->string[j] == ' ')
				{
					i = j;
				}

				chat->lines++;
				CG_ChatBox_StrInsert(chat->string, i, "\n");
				i++;
				chat_len = 0;
				last_line_pt = i + 1;
			}
			i++;
		}
	}

	cg.chatItemActive++;
	if (cg.chatItemActive >= MAX_CHATBOX_ITEMS)
	{
		cg.chatItemActive = 0;
	}
}

//insert item into array (rearranging the array if necessary)
static void CG_ChatBox_ArrayInsert(chatBoxItem_t** array, const int ins_point, const int max_num, chatBoxItem_t* item)
{
	if (array[ins_point])
	{
		//recursively call, to move everything up to the top
		if (ins_point + 1 >= max_num)
		{
			trap->Error(ERR_DROP, "CG_ChatBox_ArrayInsert: Exceeded array size");
		}
		CG_ChatBox_ArrayInsert(array, ins_point + 1, max_num, array[ins_point]);
	}

	//now that we have moved anything that would be in this slot up, insert what we want into the slot
	array[ins_point] = item;
}

//go through all the chat strings and draw them if they are not yet expired
static QINLINE void CG_ChatBox_DrawStrings(void)
{
	chatBoxItem_t* draw_these[MAX_CHATBOX_ITEMS];
	int num_to_draw = 0;
	int lines_to_draw = 0;
	int i = 0;
	float y = cg.scoreBoardShowing ? 475 : cg_chatBoxHeight.integer;
	const float font_scale = 0.65f;

	if (!cg_chatBox.integer)
	{
		return;
	}

	memset(draw_these, 0, sizeof draw_these);

	while (i < MAX_CHATBOX_ITEMS)
	{
		if (cg.chatItems[i].time >= cg.time)
		{
			int check = num_to_draw;
			int insertion_point = num_to_draw;

			while (check >= 0)
			{
				if (draw_these[check] &&
					cg.chatItems[i].time < draw_these[check]->time)
				{
					//insert here
					insertion_point = check;
				}
				check--;
			}
			CG_ChatBox_ArrayInsert(draw_these, insertion_point, MAX_CHATBOX_ITEMS, &cg.chatItems[i]);
			num_to_draw++;
			lines_to_draw += cg.chatItems[i].lines;
		}
		i++;
	}

	if (!num_to_draw)
	{
		//nothing, then, just get out of here now.
		return;
	}

	//move initial point up so we draw bottom-up (visually)
	y -= CHATBOX_FONT_HEIGHT * font_scale * lines_to_draw;

	//we have the items we want to draw, just quickly loop through them now
	i = 0;
	while (i < num_to_draw)
	{
		const int x = 10;
		CG_Text_Paint(x, y - 50, font_scale, colorWhite, draw_these[i]->string, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL);
		y += CHATBOX_FONT_HEIGHT * font_scale * draw_these[i]->lines;
		i++;
	}
}

void CGCam_DoFade(void);

static void CG_Draw2DScreenTints(void)
{
	vec4_t hcolor;

	//cutscene camera fade code
	CGCam_DoFade();

	if (cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR)
	{
		float ysal_time;
		float protect_time;
		float absorb_time;
		float rage_rec_time;
		float rage_time;
		if (cg.snap->ps.fd.forcePowersActive & 1 << FP_RAGE)
		{
			if (!cgRageTime)
			{
				cgRageTime = cg.time;
			}

			rage_time = (float)(cg.time - cgRageTime);

			rage_time /= 9000;

			if (rage_time < 0)
			{
				rage_time = 0;
			}
			if (rage_time > 0.15)
			{
				rage_time = 0.15f;
			}

			hcolor[3] = rage_time;
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;

			if (!cg.renderingThirdPerson)
			{
				CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			}

			cgRageFadeTime = 0;
			cgRageFadeVal = 0;
		}
		else if (cgRageTime)
		{
			if (!cgRageFadeTime)
			{
				cgRageFadeTime = cg.time;
				cgRageFadeVal = 0.15f;
			}

			rage_time = cgRageFadeVal;

			cgRageFadeVal -= (cg.time - cgRageFadeTime) * 0.000005f;

			if (rage_time < 0)
			{
				rage_time = 0;
			}
			if (rage_time > 0.15f)
			{
				rage_time = 0.15f;
			}

			if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
			{
				float check_rage_rec_time = rage_time;

				if (check_rage_rec_time < 0.15f)
				{
					check_rage_rec_time = 0.15f;
				}

				hcolor[3] = check_rage_rec_time;
				hcolor[0] = rage_time * 4;
				if (hcolor[0] < 0.2f)
				{
					hcolor[0] = 0.2f;
				}
				hcolor[1] = 0.2f;
				hcolor[2] = 0.2f;
			}
			else
			{
				hcolor[3] = rage_time;
				hcolor[0] = 0.7f;
				hcolor[1] = 0;
				hcolor[2] = 0;
			}

			if (!cg.renderingThirdPerson && rage_time)
			{
				CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			}
			else
			{
				if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
				{
					hcolor[3] = 0.15f;
					hcolor[0] = 0.2f;
					hcolor[1] = 0.2f;
					hcolor[2] = 0.2f;
					CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
				}
				cgRageTime = 0;
			}
		}
		else if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
		{
			if (!cgRageRecTime)
			{
				cgRageRecTime = cg.time;
			}

			rage_rec_time = (float)(cg.time - cgRageRecTime);

			rage_rec_time /= 9000;

			if (rage_rec_time < 0.15f) //0)
			{
				rage_rec_time = 0.15f; //0;
			}
			if (rage_rec_time > 0.15f)
			{
				rage_rec_time = 0.15f;
			}

			hcolor[3] = rage_rec_time;
			hcolor[0] = 0.2f;
			hcolor[1] = 0.2f;
			hcolor[2] = 0.2f;

			if (!cg.renderingThirdPerson)
			{
				CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			}

			cgRageRecFadeTime = 0;
			cgRageRecFadeVal = 0;
		}
		else if (cgRageRecTime)
		{
			if (!cgRageRecFadeTime)
			{
				cgRageRecFadeTime = cg.time;
				cgRageRecFadeVal = 0.15f;
			}

			rage_rec_time = cgRageRecFadeVal;

			cgRageRecFadeVal -= (cg.time - cgRageRecFadeTime) * 0.000005f;

			if (rage_rec_time < 0)
			{
				rage_rec_time = 0;
			}
			if (rage_rec_time > 0.15f)
			{
				rage_rec_time = 0.15f;
			}

			hcolor[3] = rage_rec_time;
			hcolor[0] = 0.2f;
			hcolor[1] = 0.2f;
			hcolor[2] = 0.2f;

			if (!cg.renderingThirdPerson && rage_rec_time)
			{
				CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			}
			else
			{
				cgRageRecTime = 0;
			}
		}

		if (cg.snap->ps.fd.forcePowersActive & 1 << FP_ABSORB)
		{
			if (!cgAbsorbTime)
			{
				cgAbsorbTime = cg.time;
			}

			absorb_time = (float)(cg.time - cgAbsorbTime);

			absorb_time /= 9000;

			if (absorb_time < 0)
			{
				absorb_time = 0;
			}
			if (absorb_time > 0.15f)
			{
				absorb_time = 0.15f;
			}

			hcolor[3] = absorb_time / 2;
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;

			if (!cg.renderingThirdPerson)
			{
				CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			}

			cgAbsorbFadeTime = 0;
			cgAbsorbFadeVal = 0;
		}
		else if (cgAbsorbTime)
		{
			if (!cgAbsorbFadeTime)
			{
				cgAbsorbFadeTime = cg.time;
				cgAbsorbFadeVal = 0.15f;
			}

			absorb_time = cgAbsorbFadeVal;

			cgAbsorbFadeVal -= (cg.time - cgAbsorbFadeTime) * 0.000005f;

			if (absorb_time < 0)
			{
				absorb_time = 0;
			}
			if (absorb_time > 0.15f)
			{
				absorb_time = 0.15f;
			}

			hcolor[3] = absorb_time / 2;
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;

			if (!cg.renderingThirdPerson && absorb_time)
			{
				CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			}
			else
			{
				cgAbsorbTime = 0;
			}
		}

		if (cg.snap->ps.fd.forcePowersActive & 1 << FP_PROTECT)
		{
			if (!cgProtectTime)
			{
				cgProtectTime = cg.time;
			}

			protect_time = (float)(cg.time - cgProtectTime);

			protect_time /= 9000;

			if (protect_time < 0)
			{
				protect_time = 0;
			}
			if (protect_time > 0.15f)
			{
				protect_time = 0.15f;
			}

			hcolor[3] = protect_time / 2;
			hcolor[0] = 0;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;

			if (!cg.renderingThirdPerson)
			{
				CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			}

			cgProtectFadeTime = 0;
			cgProtectFadeVal = 0;
		}
		else if (cgProtectTime)
		{
			if (!cgProtectFadeTime)
			{
				cgProtectFadeTime = cg.time;
				cgProtectFadeVal = 0.15f;
			}

			protect_time = cgProtectFadeVal;

			cgProtectFadeVal -= (cg.time - cgProtectFadeTime) * 0.000005f;

			if (protect_time < 0)
			{
				protect_time = 0;
			}
			if (protect_time > 0.15f)
			{
				protect_time = 0.15f;
			}

			hcolor[3] = protect_time / 2;
			hcolor[0] = 0;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;

			if (!cg.renderingThirdPerson && protect_time)
			{
				CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			}
			else
			{
				cgProtectTime = 0;
			}
		}

		if (cg.snap->ps.rocketLockIndex != ENTITYNUM_NONE && cg.time - cg.snap->ps.rocketLockTime > 0)
		{
			CG_DrawRocketLocking(cg.snap->ps.rocketLockIndex);
		}

		if (BG_HasYsalamiri(cgs.gametype, &cg.snap->ps))
		{
			if (!cgYsalTime)
			{
				cgYsalTime = cg.time;
			}

			ysal_time = (float)(cg.time - cgYsalTime);

			ysal_time /= 9000;

			if (ysal_time < 0)
			{
				ysal_time = 0;
			}
			if (ysal_time > 0.15f)
			{
				ysal_time = 0.15f;
			}

			hcolor[3] = ysal_time / 2;
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;

			if (!cg.renderingThirdPerson)
			{
				CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			}

			cgYsalFadeTime = 0;
			cgYsalFadeVal = 0;
		}
		else if (cgYsalTime)
		{
			if (!cgYsalFadeTime)
			{
				cgYsalFadeTime = cg.time;
				cgYsalFadeVal = 0.15f;
			}

			ysal_time = cgYsalFadeVal;

			cgYsalFadeVal -= (cg.time - cgYsalFadeTime) * 0.000005f;

			if (ysal_time < 0)
			{
				ysal_time = 0;
			}
			if (ysal_time > 0.15f)
			{
				ysal_time = 0.15f;
			}

			hcolor[3] = ysal_time / 2;
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;

			if (!cg.renderingThirdPerson && ysal_time)
			{
				CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			}
			else
			{
				cgYsalTime = 0;
			}
		}
	}

	if (cg.refdef.viewContents & CONTENTS_LAVA)
	{
		//tint screen red
		const float phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		hcolor[3] = 0.5 + 0.15f * sin(phase);
		hcolor[0] = 0.7f;
		hcolor[1] = 0;
		hcolor[2] = 0;

		CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
	}
	else if (cg.refdef.viewContents & CONTENTS_SLIME)
	{
		//tint screen green
		const float phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		hcolor[3] = 0.4 + 0.1f * sin(phase);
		hcolor[0] = 0;
		hcolor[1] = 0.7f;
		hcolor[2] = 0;

		CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
	}
	else if (cg.refdef.viewContents & CONTENTS_WATER)
	{
		//tint screen light blue -- FIXME: don't do this if CONTENTS_FOG? (in case someone *does* make a water shader with fog in it?)
		const float phase = cg.time / 1000.0f * WAVE_FREQUENCY * M_PI * 2;
		hcolor[3] = 0.3f + 0.05f * sinf(phase);
		hcolor[0] = 0;
		hcolor[1] = 0.2f;
		hcolor[2] = 0.8f;

		CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
	}
}

static void CG_Draw2D(void)
{
	const float in_time = cg.invenSelectTime + WEAPON_SELECT_TIME;
	const float wp_time = cg.weaponSelectTime + WEAPON_SELECT_TIME;
	const centity_t* cent = &cg_entities[cg.snap->ps.clientNum];

	const playerState_t* ps = &cg.predictedPlayerState;

	// if we are taking a levelshot for the menu, don't draw anything
	if (cg.levelShot)
	{
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		cgRageTime = 0;
		cgRageFadeTime = 0;
		cgRageFadeVal = 0;

		cgRageRecTime = 0;
		cgRageRecFadeTime = 0;
		cgRageRecFadeVal = 0;

		cgAbsorbTime = 0;
		cgAbsorbFadeTime = 0;
		cgAbsorbFadeVal = 0;

		cgProtectTime = 0;
		cgProtectFadeTime = 0;
		cgProtectFadeVal = 0;

		cgYsalTime = 0;
		cgYsalFadeTime = 0;
		cgYsalFadeVal = 0;
	}

	if (!cg_draw2D.integer)
	{
		gCGHasFallVector = qfalse;
		VectorClear(gCGFallVector);
		return;
	}

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		CG_DrawIntermission();
		CG_ChatBox_DrawStrings();
		return;
	}

	if (cg.snap->ps.powerups[PW_CLOAKED])
	{
		//draw it as long as it isn't full
		CG_DrawPic(0, 0, 640, 480, trap->R_RegisterShader("gfx/2d/cloaksense"));
	}
	else if (cent->currentState.PlayerEffectFlags & 1 << PEF_BURNING)
	{
		//draw it as long as it isn't full
		CG_DrawPic(0, 0, 640, 480, trap->R_RegisterShader("gfx/2d/firesense"));
	}
	else if (cent->currentState.PlayerEffectFlags & 1 << PEF_FREEZING)
	{
		//draw it as long as it isn't full
		CG_DrawPic(0, 0, 640, 480, trap->R_RegisterShader("gfx/2d/stasissense"));
	}
	else if ((cg.snap->ps.stats[STAT_HEALTH] <= 25 && cg.snap->ps.stats[STAT_MAX_HEALTH] >= 100) ||
		(cg.snap->ps.stats[STAT_HEALTH] <= (cg.snap->ps.stats[STAT_MAX_HEALTH] / 3) && cg.snap->ps.stats[STAT_MAX_HEALTH] < 100))
	{
		//tint screen red
		CG_DrawPic(0, 0, 640, 480, trap->R_RegisterShader("gfx/2d/dmgsense"));
	}
	if (cg.snap->ps.fd.forcePowersActive & 1 << FP_SEE)
	{
		//force sight is on
		//indicate this with sight cone thingy
		CG_DrawPic(0, 0, 640, 480, trap->R_RegisterShader("gfx/2d/jsense"));
	}

	//if (cg.predictedPlayerState.communicatingflags & (1 << HACKER))
	//if (cg.snap->ps.userInt3 & (1 << FLAG_PERFECTBLOCK))
	//if (cg.predictedPlayerState.communicatingflags & (1 << CF_SABERLOCK_ADVANCE))
	//{//test for all sorts of shit... does it work? show me.
		//CG_DrawPic(0, 0, 640, 480, trap->R_RegisterShader("gfx/2d/jsense"));
		//CG_DrawPic(0, 0, 640, 480, trap->R_RegisterShader("gfx/2d/droid_view"));
	//}

	CG_Draw2DScreenTints();

	if (cg.snap->ps.rocketLockIndex != ENTITYNUM_NONE && cg.time - cg.snap->ps.rocketLockTime > 0)
	{
		CG_DrawRocketLocking(cg.snap->ps.rocketLockIndex);
	}

	if (cg.snap->ps.holocronBits)
	{
		CG_DrawHolocronIcons();
	}
	if (cg.snap->ps.fd.forcePowersActive || cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
	{
		CG_DrawActivePowers();
	}

	if (cg.snap->ps.jetpackFuel < 100)
	{
		//draw it as long as it isn't full
		CG_DrawJetpackFuel();
	}

	if (cg.snap->ps.sprintFuel < 100)
	{
		//draw it as long as it isn't full
		CG_DrawSprintFuel();
	}

	if (cg.snap->ps.cloakFuel < 100)
	{
		//draw it as long as it isn't full
		CG_DrawCloakFuel();
	}
	if (cg.predictedPlayerState.emplacedIndex > 0)
	{
		const centity_t* eweb = &cg_entities[cg.predictedPlayerState.emplacedIndex];

		if (eweb->currentState.weapon == WP_NONE)
		{
			//using an e-web, draw its health
			CG_DrawEWebHealth();
		}
	}

	// Draw this before the text so that any text won't get clipped off
	CG_DrawZoomMask();

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		CG_DrawCrosshair(NULL, 0);
		CG_DrawCrosshairNames();
		CG_SaberClashFlare();
		CG_SaberBlockFlare();
	}
	else
	{
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if (!cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0)
		{
			float best_time;
			int draw_select;

			CG_DrawAmmoWarning();

			CG_DrawCrosshairNames();

			CG_DrawCrosshairItem();

			if (cg_drawStatus.integer)
			{
				CG_DrawIconBackground();
			}

			if (in_time > wp_time)
			{
				draw_select = 1;
				best_time = cg.invenSelectTime;
			}
			else //only draw the most recent since they're drawn in the same place
			{
				draw_select = 2;
				best_time = cg.weaponSelectTime;
			}

			if (cg.forceSelectTime > best_time)
			{
				draw_select = 3;
			}

			if (!ps->m_iVehicleNum)
			{
				switch (draw_select)
				{
				case 1:
					cg_draw_inventory_select();
					break;
				case 2:
					CG_DrawWeaponSelect();
					break;
				case 3:
					CG_DrawForceSelect();
					break;
				default:
					break;
				}
			}

			if (cg_drawStatus.integer)
			{
				CG_DrawFlagStatus();
			}

			CG_SaberClashFlare();

			if (cg_drawStatus.integer)
			{
				CG_DrawStats();
			}

			CG_DrawPickupItem();
			//Do we want to use this system again at some point?
			//CG_DrawReward();
		}
	}

	if (cg.snap->ps.fallingToDeath)
	{
		vec4_t hcolor;

		float fall_time = (float)(cg.time - cg.snap->ps.fallingToDeath);

		fall_time /= FALL_FADE_TIME / 2;

		if (fall_time < 0)
		{
			fall_time = 0;
		}
		if (fall_time > 1)
		{
			fall_time = 1;
		}

		hcolor[3] = fall_time;
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 0;

		CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);

		if (!gCGHasFallVector)
		{
			VectorCopy(cg.snap->ps.origin, gCGFallVector);
			gCGHasFallVector = qtrue;
		}
	}
	else
	{
		if (gCGHasFallVector)
		{
			gCGHasFallVector = qfalse;
			VectorClear(gCGFallVector);
		}
	}

	CG_DrawVote();
	CG_DrawTeamVote();

	CG_DrawLagometer();

	if (!cl_paused.integer)
	{
		CG_DrawBracketedEntities();
		CG_DrawUpperRight();
	}

	if (!CG_DrawFollow())
	{
		CG_DrawWarmup();
	}

	if (cgSiegeRoundState)
	{
		char p_str[1024];
		int r_time;

		switch (cgSiegeRoundState)
		{
		case 1:
			CG_CenterPrint(CG_GetStringEdString("MP_INGAME", "WAITING_FOR_PLAYERS"), SCREEN_HEIGHT * 0.30,
				BIGCHAR_WIDTH);
			break;
		case 2:
			r_time = SIEGE_ROUND_BEGIN_TIME - (cg.time - cgSiegeRoundTime);

			if (r_time < 0)
			{
				r_time = 0;
			}
			if (r_time > SIEGE_ROUND_BEGIN_TIME)
			{
				r_time = SIEGE_ROUND_BEGIN_TIME;
			}

			r_time /= 1000;

			r_time += 1;

			if (r_time < 1)
			{
				r_time = 1;
			}

			if (r_time <= 3 && r_time != cgSiegeRoundCountTime)
			{
				cgSiegeRoundCountTime = r_time;

				switch (r_time)
				{
				case 1:
					trap->S_StartLocalSound(cgs.media.count1Sound, CHAN_ANNOUNCER);
					break;
				case 2:
					trap->S_StartLocalSound(cgs.media.count2Sound, CHAN_ANNOUNCER);
					break;
				case 3:
					trap->S_StartLocalSound(cgs.media.count3Sound, CHAN_ANNOUNCER);
					break;
				default:
					break;
				}
			}

			Q_strncpyz(p_str, va("%s %i...", CG_GetStringEdString("MP_INGAME", "ROUNDBEGINSIN"), r_time), sizeof p_str);
			CG_CenterPrint(p_str, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
			//same
			break;
		default:
			break;
		}

		cgSiegeEntityRender = 0;
	}
	else if (cgSiegeRoundTime)
	{
		CG_CenterPrint("", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
		cgSiegeRoundTime = 0;

		//cgSiegeRoundBeganTime = cg.time;
		cgSiegeEntityRender = 0;
	}
	else if (cgSiegeRoundBeganTime)
	{
		//Draw how much time is left in the round based on local info.
		int timed_team = TEAM_FREE;
		int timed_value = 0;

		if (cgSiegeEntityRender)
		{
			//render the objective item model since this client has it
			CG_DrawSiegeHUDItem();
		}

		if (team1Timed)
		{
			timed_team = TEAM_RED; //team 1
			if (cg_beatingSiegeTime)
			{
				timed_value = cg_beatingSiegeTime;
			}
			else
			{
				timed_value = team1Timed;
			}
		}
		else if (team2Timed)
		{
			timed_team = TEAM_BLUE; //team 2
			if (cg_beatingSiegeTime)
			{
				timed_value = cg_beatingSiegeTime;
			}
			else
			{
				timed_value = team2Timed;
			}
		}

		if (timed_team != TEAM_FREE)
		{
			//one of the teams has a timer
			int timeRemaining;
			qboolean isMyTeam = qfalse;

			if (cgs.siegeTeamSwitch && !cg_beatingSiegeTime)
			{
				//in switchy mode but not beating a time, so count up.
				timeRemaining = cg.time - cgSiegeRoundBeganTime;
				if (timeRemaining < 0)
				{
					timeRemaining = 0;
				}
			}
			else
			{
				timeRemaining = cgSiegeRoundBeganTime + timed_value - cg.time;
			}

			if (timeRemaining > timed_value)
			{
				timeRemaining = timed_value;
			}
			else if (timeRemaining < 0)
			{
				timeRemaining = 0;
			}

			if (timeRemaining)
			{
				timeRemaining /= 1000;
			}

			if (cg.predictedPlayerState.persistant[PERS_TEAM] == timed_team)
			{
				//the team that's timed is the one this client is on
				isMyTeam = qtrue;
			}

			CG_DrawSiegeTimer(timeRemaining, isMyTeam);
		}
	}
	else
	{
		cgSiegeEntityRender = 0;
	}

	if (cg_siegeDeathTime)
	{
		int timeRemaining = cg_siegeDeathTime - cg.time;

		if (timeRemaining < 0)
		{
			timeRemaining = 0;
			cg_siegeDeathTime = 0;
		}

		if (timeRemaining)
		{
			timeRemaining /= 1000;
		}

		CG_DrawSiegeDeathTimer(timeRemaining);
	}

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if (!cg.scoreBoardShowing)
	{
		CG_DrawCenterString();
	}

	// always draw chat
	CG_ChatBox_DrawStrings();
}

qboolean CG_CullPointAndRadius(const vec3_t pt, float radius);

static void CG_DrawMiscStaticModels(void)
{
	refEntity_t ent;
	vec3_t cullorg;

	memset(&ent, 0, sizeof ent);

	ent.reType = RT_MODEL;
	ent.frame = 0;
	ent.nonNormalizedAxes = qtrue;

	// static models don't project shadows
	ent.renderfx = RF_NOSHADOW;

	for (int i = 0; i < cgs.numMiscStaticModels; i++)
	{
		vec3_t diff;
		VectorCopy(cgs.miscStaticModels[i].org, cullorg);
		cullorg[2] += 1.0f;

		if (cgs.miscStaticModels[i].zoffset)
		{
			cullorg[2] += cgs.miscStaticModels[i].zoffset;
		}
		if (cgs.miscStaticModels[i].radius)
		{
			if (CG_CullPointAndRadius(cullorg, cgs.miscStaticModels[i].radius))
			{
				continue;
			}
		}

		if (!trap->R_InPVS(cg.refdef.vieworg, cullorg, cg.refdef.areamask))
		{
			continue;
		}

		VectorCopy(cgs.miscStaticModels[i].org, ent.origin);
		VectorCopy(cgs.miscStaticModels[i].org, ent.oldorigin);
		VectorCopy(cgs.miscStaticModels[i].org, ent.lightingOrigin);

		for (int j = 0; j < 3; j++)
		{
			VectorCopy(cgs.miscStaticModels[i].axes[j], ent.axis[j]);
		}
		ent.hModel = cgs.miscStaticModels[i].model;

		VectorSubtract(ent.origin, cg.refdef.vieworg, diff);
		if (VectorLength(diff) - cgs.miscStaticModels[i].radius <= cg.distanceCull)
		{
			trap->R_AddRefEntityToScene(&ent);
		}
	}
}

static void CG_DrawTourneyScoreboard()
{
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive(const stereoFrame_t stereo_view)
{
	float separation;
	vec3_t base_org;

	// optionally draw the info screen instead
	if (!cg.snap)
	{
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournament scoreboard instead
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
		cg.snap->ps.pm_flags & PMF_SCOREBOARD)
	{
		CG_DrawTourneyScoreboard();
		return;
	}

	switch (stereo_view)
	{
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value / 2;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value / 2;
		break;
	default:
		separation = 0;
		trap->Error(ERR_DROP, "CG_DrawActive: Undefined stereoView");
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy(cg.refdef.vieworg, base_org);
	if (separation != 0)
	{
		VectorMA(cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg);
	}

	if (cg.snap->ps.fd.forcePowersActive & 1 << FP_SEE)
		cg.refdef.rdflags |= RDF_ForceSightOn;

	cg.refdef.rdflags |= RDF_DRAWSKYBOX;

	CG_DrawMiscStaticModels();

	// draw 3D view
	trap->R_RenderScene(&cg.refdef);

	// restore original viewpoint if running stereo
	if (separation != 0)
	{
		VectorCopy(base_org, cg.refdef.vieworg);
	}

	// draw status bar and other floating elements
	CG_Draw2D();
}