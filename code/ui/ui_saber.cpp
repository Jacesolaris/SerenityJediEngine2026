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

/*
=======================================================================

USER INTERFACE SABER LOADING & DISPLAY CODE

=======================================================================
*/

// leave this at the top of all UI_xxxx files for PCH reasons...
//
#include "../server/exe_headers.h"
#include "ui_local.h"
#include "ui_shared.h"
#include "../ghoul2/G2.h"

constexpr auto MAX_SABER_DATA_SIZE = 0x80000;
char SaberParms[MAX_SABER_DATA_SIZE];
qboolean ui_saber_parms_parsed = qfalse;

extern vmCvar_t ui_rgb_saber_red;
extern vmCvar_t ui_rgb_saber_green;
extern vmCvar_t ui_rgb_saber_blue;
extern vmCvar_t ui_rgb_saber2_red;
extern vmCvar_t ui_rgb_saber2_green;
extern vmCvar_t ui_rgb_saber2_blue;

extern vmCvar_t ui_SFXSabers;
extern vmCvar_t ui_SFXSabersGlowSize;
extern vmCvar_t ui_SFXSabersCoreSize;

extern vmCvar_t ui_SFXSabersGlowSizeBF2;
extern vmCvar_t ui_SFXSabersCoreSizeBF2;
extern vmCvar_t ui_SFXSabersGlowSizeSFX;
extern vmCvar_t ui_SFXSabersCoreSizeSFX;
extern vmCvar_t ui_SFXSabersGlowSizeEP1;
extern vmCvar_t ui_SFXSabersCoreSizeEP1;
extern vmCvar_t ui_SFXSabersGlowSizeEP2;
extern vmCvar_t ui_SFXSabersCoreSizeEP2;
extern vmCvar_t ui_SFXSabersGlowSizeEP3;
extern vmCvar_t ui_SFXSabersCoreSizeEP3;
extern vmCvar_t ui_SFXSabersGlowSizeOT;
extern vmCvar_t ui_SFXSabersCoreSizeOT;
extern vmCvar_t ui_SFXSabersGlowSizeTFA;
extern vmCvar_t ui_SFXSabersCoreSizeTFA;
extern vmCvar_t ui_SFXSabersGlowSizeCSM;
extern vmCvar_t ui_SFXSabersCoreSizeCSM;

static qhandle_t redSaberGlowShader;
static qhandle_t redSaberCoreShader;
static qhandle_t orangeSaberGlowShader;
static qhandle_t orangeSaberCoreShader;
static qhandle_t yellowSaberGlowShader;
static qhandle_t yellowSaberCoreShader;
static qhandle_t greenSaberGlowShader;
static qhandle_t greenSaberCoreShader;
static qhandle_t blueSaberGlowShader;
static qhandle_t blueSaberCoreShader;
static qhandle_t purpleSaberGlowShader;
static qhandle_t purpleSaberCoreShader;
static qhandle_t rgbSaberGlowShader;
static qhandle_t rgbSaberCoreShader;
static qhandle_t rgbSaberCore2Shader;
static qhandle_t blackSaberGlowShader;
static qhandle_t blackSaberCoreShader;
static qhandle_t SaberBladeShader;
static qhandle_t blackSaberBladeShader;
static qhandle_t limeSaberGlowShader;
static qhandle_t limeSaberCoreShader;
static qhandle_t rgbTFASaberCoreShader;

//Episode I Sabers
static qhandle_t ep1SaberCoreShader;
static qhandle_t redEp1GlowShader;
static qhandle_t orangeEp1GlowShader;
static qhandle_t yellowEp1GlowShader;
static qhandle_t greenEp1GlowShader;
static qhandle_t blueEp1GlowShader;
static qhandle_t purpleEp1GlowShader;

//Episode II Sabers
static qhandle_t ep2SaberCoreShader;
static qhandle_t redEp2GlowShader;
static qhandle_t orangeEp2GlowShader;
static qhandle_t yellowEp2GlowShader;
static qhandle_t greenEp2GlowShader;
static qhandle_t blueEp2GlowShader;
static qhandle_t purpleEp2GlowShader;

//Episode III Sabers
static qhandle_t ep3SaberCoreShader;
static qhandle_t whiteIgniteFlare02;
static qhandle_t blackIgniteFlare02;
static qhandle_t redEp3GlowShader;
static qhandle_t orangeEp3GlowShader;
static qhandle_t yellowEp3GlowShader;
static qhandle_t greenEp3GlowShader;
static qhandle_t blueEp3GlowShader;
static qhandle_t purpleEp3GlowShader;

//Original Trilogy Sabers
static qhandle_t otSaberCoreShader;
static qhandle_t redOTGlowShader;
static qhandle_t orangeOTGlowShader;
static qhandle_t yellowOTGlowShader;
static qhandle_t greenOTGlowShader;
static qhandle_t blueOTGlowShader;
static qhandle_t purpleOTGlowShader;

static qhandle_t unstableRedSaberCoreShader;
extern vmCvar_t ui_com_rend2;
extern vmCvar_t ui_r_AdvancedsurfaceSprites;

void UI_CacheSaberGlowGraphics()
{
	//FIXME: these get fucked by vid_restarts
	redSaberGlowShader = re.RegisterShader("gfx/effects/sabers/red_glow");
	redSaberCoreShader = re.RegisterShader("gfx/effects/sabers/red_line");
	orangeSaberGlowShader = re.RegisterShader("gfx/effects/sabers/orange_glow");
	orangeSaberCoreShader = re.RegisterShader("gfx/effects/sabers/orange_line");
	yellowSaberGlowShader = re.RegisterShader("gfx/effects/sabers/yellow_glow");
	yellowSaberCoreShader = re.RegisterShader("gfx/effects/sabers/yellow_line");
	greenSaberGlowShader = re.RegisterShader("gfx/effects/sabers/green_glow");
	greenSaberCoreShader = re.RegisterShader("gfx/effects/sabers/green_line");
	blueSaberGlowShader = re.RegisterShader("gfx/effects/sabers/blue_glow");
	blueSaberCoreShader = re.RegisterShader("gfx/effects/sabers/blue_line");
	purpleSaberGlowShader = re.RegisterShader("gfx/effects/sabers/purple_glow");
	purpleSaberCoreShader = re.RegisterShader("gfx/effects/sabers/purple_line");
	rgbSaberGlowShader = re.RegisterShader("gfx/effects/sabers/rgb_glow");
	rgbSaberCoreShader = re.RegisterShader("gfx/effects/sabers/rgb_line");
	rgbSaberCore2Shader = re.RegisterShader("gfx/effects/sabers/rgb_core");
	blackSaberGlowShader = re.RegisterShader("gfx/effects/sabers/black_glow");
	blackSaberCoreShader = re.RegisterShader("gfx/effects/sabers/black_blade");
	SaberBladeShader = re.RegisterShader("gfx/effects/sabers/saber_blade");
	blackSaberBladeShader = re.RegisterShader("gfx/effects/sabers/black_blade");
	limeSaberGlowShader = re.RegisterShader("gfx/effects/sabers/lime_glow");
	limeSaberCoreShader = re.RegisterShader("gfx/effects/sabers/lime_line");
	rgbTFASaberCoreShader = re.RegisterShader("gfx/effects/TFASabers/blade_TFA");

	//Episode I Sabers
	ep1SaberCoreShader = re.RegisterShader("gfx/effects/Ep1Sabers/saber_core");
	redEp1GlowShader = re.RegisterShader("gfx/effects/Ep1Sabers/red_glowa");
	orangeEp1GlowShader = re.RegisterShader("gfx/effects/Ep1Sabers/orange_glowa");
	yellowEp1GlowShader = re.RegisterShader("gfx/effects/Ep1Sabers/yellow_glowa");
	greenEp1GlowShader = re.RegisterShader("gfx/effects/Ep1Sabers/green_glowa");
	blueEp1GlowShader = re.RegisterShader("gfx/effects/Ep1Sabers/blue_glowa");
	purpleEp1GlowShader = re.RegisterShader("gfx/effects/Ep1Sabers/purple_glowa");
	//Episode II Sabers
	ep2SaberCoreShader = re.RegisterShader("gfx/effects/Ep2Sabers/saber_core");
	redEp2GlowShader = re.RegisterShader("gfx/effects/Ep2Sabers/red_glowa");
	orangeEp2GlowShader = re.RegisterShader("gfx/effects/Ep2Sabers/orange_glowa");
	yellowEp2GlowShader = re.RegisterShader("gfx/effects/Ep2Sabers/yellow_glowa");
	greenEp2GlowShader = re.RegisterShader("gfx/effects/Ep2Sabers/green_glowa");
	blueEp2GlowShader = re.RegisterShader("gfx/effects/Ep2Sabers/blue_glowa");
	purpleEp2GlowShader = re.RegisterShader("gfx/effects/Ep2Sabers/purple_glowa");
	//Episode III Sabers
	ep3SaberCoreShader = re.RegisterShader("gfx/effects/Ep3Sabers/saber_core");
	whiteIgniteFlare02 = re.RegisterShader("gfx/effects/Ep3Sabers/white_ignite_flare");
	blackIgniteFlare02 = re.RegisterShader("gfx/effects/Ep3Sabers/black_ignite_flare");
	redEp3GlowShader = re.RegisterShader("gfx/effects/Ep3Sabers/red_glowa");
	orangeEp3GlowShader = re.RegisterShader("gfx/effects/Ep3Sabers/orange_glowa");
	yellowEp3GlowShader = re.RegisterShader("gfx/effects/Ep3Sabers/yellow_glowa");
	greenEp3GlowShader = re.RegisterShader("gfx/effects/Ep3Sabers/green_glowa");
	blueEp3GlowShader = re.RegisterShader("gfx/effects/Ep3Sabers/blue_glowa");
	purpleEp3GlowShader = re.RegisterShader("gfx/effects/Ep3Sabers/purple_glowa");
	//Original Trilogy Sabers
	otSaberCoreShader = re.RegisterShader("gfx/effects/OTsabers/ot_saberCore");
	redOTGlowShader = re.RegisterShader("gfx/effects/OTsabers/ot_redGlow");
	orangeOTGlowShader = re.RegisterShader("gfx/effects/OTsabers/ot_orangeGlow");
	yellowOTGlowShader = re.RegisterShader("gfx/effects/OTsabers/ot_yellowGlow");
	greenOTGlowShader = re.RegisterShader("gfx/effects/OTsabers/ot_greenGlow");
	blueOTGlowShader = re.RegisterShader("gfx/effects/OTsabers/ot_blueGlow");
	purpleOTGlowShader = re.RegisterShader("gfx/effects/OTsabers/ot_purpleGlow");

	unstableRedSaberCoreShader = re.RegisterShader("gfx/effects/sabers/unstable_red_line");
}

static qboolean UI_ParseLiteral(const char** data, const char* string)
{
	const char* token = COM_ParseExt(data, qtrue);
	if (token[0] == 0)
	{
		ui.Printf("unexpected EOF\n");
		return qtrue;
	}

	if (Q_stricmp(token, string))
	{
		ui.Printf("required string '%s' missing\n", string);
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_SaberParseParm(const char* saber_name, const char* parmname, char* saberData)
{
	const char* token;
	const char* value;
	const char* p;

	if (!saber_name || !saber_name[0])
	{
		return qfalse;
	}

	//try to parse it out
	p = SaberParms;
	COM_BeginParseSession();

	// look for the right saber
	while (p)
	{
		token = COM_ParseExt(&p, qtrue);
		if (token[0] == 0)
		{
			COM_EndParseSession();
			return qfalse;
		}

		if (!Q_stricmp(token, saber_name))
		{
			break;
		}

		SkipBracedSection(&p);
	}
	if (!p)
	{
		COM_EndParseSession();
		return qfalse;
	}

	if (UI_ParseLiteral(&p, "{"))
	{
		COM_EndParseSession();
		return qfalse;
	}

	// parse the saber info block
	while (true)
	{
		token = COM_ParseExt(&p, qtrue);
		if (!token[0])
		{
			ui.Printf(S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", saber_name);
			COM_EndParseSession();
			return qfalse;
		}

		if (!Q_stricmp(token, "}"))
		{
			break;
		}

		if (!Q_stricmp(token, parmname))
		{
			if (COM_ParseString(&p, &value))
			{
				continue;
			}
			strcpy(saberData, value);
			COM_EndParseSession();
			return qtrue;
		}

		SkipRestOfLine(&p);
	}

	COM_EndParseSession();
	return qfalse;
}

qboolean UI_SaberProperNameForSaber(const char* saber_name, char* saberProperName)
{
	return UI_SaberParseParm(saber_name, "name", saberProperName);
}

qboolean UI_SaberModelForSaber(const char* saber_name, char* saberModel)
{
	return UI_SaberParseParm(saber_name, "saberModel", saberModel);
}

qboolean UI_SaberSkinForSaber(const char* saber_name, char* saberSkin)
{
	return UI_SaberParseParm(saber_name, "customSkin", saberSkin);
}

qboolean UI_SaberTypeForSaber(const char* saber_name, char* saberType)
{
	return UI_SaberParseParm(saber_name, "saberType", saberType);
}

static int UI_saber_numBladesForSaber(const char* saber_name)
{
	char numBladesString[8] = { 0 };
	UI_SaberParseParm(saber_name, "numBlades", numBladesString);
	int numBlades = atoi(numBladesString);
	if (numBlades < 1)
	{
		numBlades = 1;
	}
	else if (numBlades > 8)
	{
		numBlades = 8;
	}
	return numBlades;
}

static qboolean UI_SaberShouldDrawBlade(const char* saber_name, int blade_num)
{
	int bladeStyle2Start = 0, noBlade = 0;
	char bladeStyle2StartString[8] = { 0 };
	char noBladeString[8] = { 0 };
	UI_SaberParseParm(saber_name, "bladeStyle2Start", bladeStyle2StartString);
	if (bladeStyle2StartString[0])
	{
		bladeStyle2Start = atoi(bladeStyle2StartString);
	}
	if (bladeStyle2Start
		&& blade_num >= bladeStyle2Start)
	{
		//use second blade style
		UI_SaberParseParm(saber_name, "noBlade2", noBladeString);
		if (noBladeString[0])
		{
			noBlade = atoi(noBladeString);
		}
	}
	else
	{
		//use first blade style
		UI_SaberParseParm(saber_name, "noBlade", noBladeString);
		if (noBladeString[0])
		{
			noBlade = atoi(noBladeString);
		}
	}
	return static_cast<qboolean>(noBlade == 0);
}

static float UI_SaberBladeLengthForSaber(const char* saber_name, int blade_num)
{
	char lengthString[8] = { 0 };
	float length = 40.0f;
	UI_SaberParseParm(saber_name, "saberLength", lengthString);
	if (lengthString[0])
	{
		length = atof(lengthString);
		if (length < 0.0f)
		{
			length = 0.0f;
		}
	}

	UI_SaberParseParm(saber_name, va("saberLength%d", blade_num + 1), lengthString);
	if (lengthString[0])
	{
		length = atof(lengthString);
		if (length < 0.0f)
		{
			length = 0.0f;
		}
	}

	return length;
}

static float UI_SaberBladeRadiusForSaber(const char* saber_name, int blade_num)
{
	char radiusString[8] = { 0 };
	float radius = 3.0f;
	UI_SaberParseParm(saber_name, "saberRadius", radiusString);
	if (radiusString[0])
	{
		radius = atof(radiusString);
		if (radius < 0.0f)
		{
			radius = 0.0f;
		}
	}

	UI_SaberParseParm(saber_name, va("saberRadius%d", blade_num + 1), radiusString);
	if (radiusString[0])
	{
		radius = atof(radiusString);
		if (radius < 0.0f)
		{
			radius = 0.0f;
		}
	}

	return radius;
}

void UI_SaberLoadParms()
{
	int saberExtFNLen = 0;
	char* buffer = nullptr;
	char saberExtensionListBuf[2048]; //	The list of file names read in

	//ui.Printf( "UI Parsing *.sab saber definitions\n" );

	ui_saber_parms_parsed = qtrue;
	UI_CacheSaberGlowGraphics();

	//set where to store the first one
	int totallen = 0;
	char* marker = SaberParms;
	marker[0] = '\0';

	//now load in the sabers
	const int fileCnt = ui.FS_GetFileList("ext_data/sabers", ".sab", saberExtensionListBuf,
		sizeof saberExtensionListBuf);

	char* holdChar = saberExtensionListBuf;
	for (int i = 0; i < fileCnt; i++, holdChar += saberExtFNLen + 1)
	{
		saberExtFNLen = strlen(holdChar);

		int len = ui.FS_ReadFile(va("ext_data/sabers/%s", holdChar), reinterpret_cast<void**>(&buffer));

		if (len == -1)
		{
			ui.Printf("UI_SaberLoadParms: error reading %s\n", holdChar);
		}
		else
		{
			if (totallen && *(marker - 1) == '}')
			{
				//don't let it end on a } because that should be a stand-alone token
				strcat(marker, " ");
				totallen++;
				marker++;
			}
			len = COM_Compress(buffer);

			if (totallen + len >= MAX_SABER_DATA_SIZE)
			{
				Com_Error(ERR_FATAL,
					"UI_SaberLoadParms: ran out of space before reading %s\n(you must make the .npc files smaller)",
					holdChar);
			}
			strcat(marker, buffer);
			ui.FS_FreeFile(buffer);

			totallen += len;
			marker += len;
		}
	}
}

static void RGB_LerpColor(vec3_t from, vec3_t to, float frac, vec3_t out)
{
	vec3_t diff;

	VectorSubtract(to, from, diff);

	VectorCopy(from, out);

	for (int i = 0; i < 3; i++)
		out[i] += diff[i] * frac;
}

static int getint(char** buf)
{
	const double temp = strtod(*buf, buf);
	return static_cast<int>(temp);
}

void ParseRGBSaber(char* str, vec3_t c)
{
	char* p = str;

	for (int i = 0; i < 3; i++)
	{
		c[i] = getint(&p);
		p++;
	}
}

vec3_t ScriptedColors[10][2] = { 0 };
int ScriptedTimes[10][2] = { 0 };
int ScriptedNum[2] = { 0 };
int ScriptedActualNum[2] = { 0 };
int ScriptedStartTime[2] = { 0 };
int ScriptedEndTime[2] = { 0 };

void UI_ParseScriptedSaber(char* script, int snum)
{
	int n = 0;
	char* p = script;

	const int l = strlen(p);
	p++;

	while (p[0] && p - script < l && n < 10)
	{
		ParseRGBSaber(p, ScriptedColors[n][snum]);
		while (p[0] != ':')
			p++;
		p++;

		ScriptedTimes[n][snum] = getint(&p);

		p++;
		n++;
	}
	ScriptedNum[snum] = n;
}

static void rgb_adjust_scipted_saber_color(vec3_t color, const int n)
{
	int actual;
	const int time = uiInfo.uiDC.realTime;

	if (!ScriptedStartTime[n])
	{
		ScriptedActualNum[n] = 0;
		ScriptedStartTime[n] = time;
		ScriptedEndTime[n] = time + ScriptedTimes[0][n];
	}
	else if (ScriptedEndTime[n] < time)
	{
		ScriptedActualNum[n] = (ScriptedActualNum[n] + 1) % ScriptedNum[n];
		actual = ScriptedActualNum[n];
		ScriptedStartTime[n] = time;
		ScriptedEndTime[n] = time + ScriptedTimes[actual][n];
	}

	actual = ScriptedActualNum[n];

	const float frac = static_cast<float>(time - ScriptedStartTime[n]) / static_cast<float>(ScriptedEndTime[n] -
		ScriptedStartTime[n]);

	if (actual + 1 != ScriptedNum[n])
		RGB_LerpColor(ScriptedColors[actual][n], ScriptedColors[actual + 1][n], frac, color);
	else
		RGB_LerpColor(ScriptedColors[actual][n], ScriptedColors[0][n], frac, color);

	for (int i = 0; i < 3; i++)
		color[i] /= 255;
}

constexpr auto PIMP_MIN_INTESITY = 120;

static void RGB_RandomRGB(vec3_t c)
{
	int i;
	for (i = 0; i < 3; i++)
		c[i] = 0;

	while (c[0] + c[1] + c[2] < PIMP_MIN_INTESITY)
		for (i = 0; i < 3; i++)
			c[i] = rand() % 255;
}

int PimpStartTime[2];
int PimpEndTime[2];
vec3_t PimpColorFrom[2];
vec3_t PimpColorTo[2];

static void RGB_AdjustPimpSaberColor(vec3_t color, int n)
{
	int time;

	if (!PimpStartTime[n])
	{
		PimpStartTime[n] = uiInfo.uiDC.realTime;
		RGB_RandomRGB(PimpColorFrom[n]);
		RGB_RandomRGB(PimpColorTo[n]);
		time = 250 + rand() % 250;
		PimpEndTime[n] = uiInfo.uiDC.realTime + time;
	}
	else if (PimpEndTime[n] < uiInfo.uiDC.realTime)
	{
		VectorCopy(PimpColorTo[n], PimpColorFrom[n]);
		RGB_RandomRGB(PimpColorTo[n]);
		time = 250 + rand() % 250;
		PimpStartTime[n] = uiInfo.uiDC.realTime;
		PimpEndTime[n] = uiInfo.uiDC.realTime + time;
	}

	const float frac = static_cast<float>(uiInfo.uiDC.realTime - PimpStartTime[n]) / static_cast<float>(PimpEndTime[n] -
		PimpStartTime[n]);

	RGB_LerpColor(PimpColorFrom[n], PimpColorTo[n], frac, color);

	for (int i = 0; i < 3; i++)
		color[i] /= 255;
}

static void UI_DoBattlefrontSaber(vec3_t blade_muz, vec3_t blade_dir, float lengthMax, float radius, saber_colors_t color,
	int whichSaber)
{
	vec3_t mid;
	float radiusmult, effectradius, coreradius;
	float blade_len;

	qhandle_t blade = 0, glow = 0;
	refEntity_t saber;

	blade_len = lengthMax;

	if (blade_len < 0.5f)
	{
		return;
	}

	switch (color)
	{
	case SABER_RED:
		glow = redSaberGlowShader;
		blade = redSaberCoreShader;
		break;
	case SABER_ORANGE:
		glow = orangeSaberGlowShader;
		blade = orangeSaberCoreShader;
		break;
	case SABER_YELLOW:
		glow = yellowSaberGlowShader;
		blade = yellowSaberCoreShader;
		break;
	case SABER_GREEN:
		glow = greenSaberGlowShader;
		blade = greenSaberCoreShader;
		break;
	case SABER_BLUE:
		glow = blueSaberGlowShader;
		blade = blueSaberCoreShader;
		break;
	case SABER_PURPLE:
		glow = purpleSaberGlowShader;
		blade = purpleSaberCoreShader;
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = limeSaberCoreShader;
		break;
	case SABER_RGB:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < lengthMax)
	{
		radiusmult = 0.5 + blade_len / lengthMax / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersGlowSizeBF2.value;
	coreradius = (radius * 0.4f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersCoreSizeBF2.value;

	{
		float angle_scale = 1.0f;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			float effectalpha = 0.8f;
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			saber.shaderRGBA[0] = 0xff * effectalpha;
			saber.shaderRGBA[1] = 0xff * effectalpha;
			saber.shaderRGBA[2] = 0xff * effectalpha;
			saber.shaderRGBA[3] = 0xff * effectalpha;

			if (color >= SABER_RGB)
			{
				if (whichSaber == 0)
				{
					saber.shaderRGBA[0] = ui_rgb_saber_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber_blue.integer * effectalpha;
				}
				else
				{
					saber.shaderRGBA[0] = ui_rgb_saber2_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber2_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber2_blue.integer * effectalpha;
				}
			}

			DC->addRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;

		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		DC->addRefEntityToScene(&saber);
	}
}

static void UI_DoSFXSaber(vec3_t blade_muz, vec3_t blade_dir, float lengthMax, float radius, saber_colors_t color,
	int whichSaber)
{
	vec3_t mid;
	float radiusmult, effectradius, coreradius;
	float blade_len;

	qhandle_t glow = 0, blade = 0;
	refEntity_t saber;

	blade_len = lengthMax;

	if (blade_len < 0.5f)
	{
		return;
	}

	switch (color)
	{
	case SABER_RED:
		glow = redSaberGlowShader;
		blade = rgbSaberCore2Shader;
		break;
	case SABER_ORANGE:
		glow = orangeSaberGlowShader;
		blade = rgbSaberCore2Shader;
		break;
	case SABER_YELLOW:
		glow = yellowSaberGlowShader;
		blade = rgbSaberCore2Shader;
		break;
	case SABER_GREEN:
		glow = greenSaberGlowShader;
		blade = rgbSaberCore2Shader;
		break;
	case SABER_BLUE:
		glow = blueSaberGlowShader;
		blade = rgbSaberCore2Shader;
		break;
	case SABER_PURPLE:
		glow = purpleSaberGlowShader;
		blade = rgbSaberCore2Shader;
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = rgbSaberCore2Shader;
		break;
	case SABER_RGB:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCore2Shader;
		break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCore2Shader;
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCore2Shader;
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCore2Shader;
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCore2Shader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCore2Shader;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < lengthMax)
	{
		radiusmult = 0.5 + blade_len / lengthMax / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersGlowSizeSFX.value;
	coreradius = (radius * 0.4f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersCoreSizeSFX.value;

	{
		float angle_scale = 1.0f;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			float effectalpha = 0.8f;
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			saber.shaderRGBA[0] = 0xff * effectalpha;
			saber.shaderRGBA[1] = 0xff * effectalpha;
			saber.shaderRGBA[2] = 0xff * effectalpha;
			saber.shaderRGBA[3] = 0xff * effectalpha;

			if (color >= SABER_RGB)
			{
				if (whichSaber == 0)
				{
					saber.shaderRGBA[0] = ui_rgb_saber_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber_blue.integer * effectalpha;
				}
				else
				{
					saber.shaderRGBA[0] = ui_rgb_saber2_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber2_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber2_blue.integer * effectalpha;
				}
			}

			DC->addRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;

		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		DC->addRefEntityToScene(&saber);
	}
}

static void UI_DoEp1Saber(vec3_t blade_muz, vec3_t blade_dir, float lengthMax, float radius, saber_colors_t color,
	int whichSaber)
{
	vec3_t mid, rgb = { 1, 1, 1 };
	float radiusmult, effectradius, coreradius;
	float blade_len;

	qhandle_t glow = 0, blade = 0;
	refEntity_t saber;

	blade_len = lengthMax;

	if (blade_len < 0.5f)
	{
		return;
	}

	switch (color)
	{
	case SABER_RED:
		glow = redEp1GlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeEp1GlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowEp1GlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenEp1GlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueEp1GlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleEp1GlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
		glow = rgbSaberGlowShader;
		blade = ep1SaberCoreShader;
		break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = ep1SaberCoreShader;
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = ep1SaberCoreShader;
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = ep1SaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = ep1SaberCoreShader;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < lengthMax)
	{
		radiusmult = 0.5 + blade_len / lengthMax / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersGlowSizeEP1.value;
	coreradius = (radius * 0.4f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersCoreSizeEP1.value;

	{
		float angle_scale = 1.0f;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			float effectalpha = 0.8f;
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			saber.shaderRGBA[0] = 0xff * effectalpha;
			saber.shaderRGBA[1] = 0xff * effectalpha;
			saber.shaderRGBA[2] = 0xff * effectalpha;
			saber.shaderRGBA[3] = 0xff * effectalpha;

			if (color >= SABER_RGB)
			{
				if (whichSaber == 0)
				{
					saber.shaderRGBA[0] = ui_rgb_saber_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber_blue.integer * effectalpha;
				}
				else
				{
					saber.shaderRGBA[0] = ui_rgb_saber2_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber2_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber2_blue.integer * effectalpha;
				}
			}

			DC->addRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;

		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		DC->addRefEntityToScene(&saber);
	}
}

static void UI_DoEp2Saber(vec3_t blade_muz, vec3_t blade_dir, float lengthMax, float radius, saber_colors_t color,
	int whichSaber)
{
	vec3_t mid, rgb = { 1, 1, 1 };
	float radiusmult, effectradius, coreradius;
	float blade_len;

	qhandle_t glow = 0, blade = 0;
	refEntity_t saber;

	blade_len = lengthMax;

	if (blade_len < 0.5f)
	{
		return;
	}

	switch (color)
	{
	case SABER_RED:
		glow = redEp2GlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeEp2GlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowEp2GlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenEp2GlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueEp2GlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleEp2GlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
		glow = rgbSaberGlowShader;
		blade = ep2SaberCoreShader;
		break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = ep2SaberCoreShader;
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = ep2SaberCoreShader;
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = ep2SaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = ep2SaberCoreShader;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < lengthMax)
	{
		radiusmult = 0.5 + blade_len / lengthMax / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersGlowSizeEP2.value;
	coreradius = (radius * 0.4f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersCoreSizeEP2.value;

	{
		float angle_scale = 1.0f;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			float effectalpha = 0.8f;
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			saber.shaderRGBA[0] = 0xff * effectalpha;
			saber.shaderRGBA[1] = 0xff * effectalpha;
			saber.shaderRGBA[2] = 0xff * effectalpha;
			saber.shaderRGBA[3] = 0xff * effectalpha;

			if (color >= SABER_RGB)
			{
				if (whichSaber == 0)
				{
					saber.shaderRGBA[0] = ui_rgb_saber_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber_blue.integer * effectalpha;
				}
				else
				{
					saber.shaderRGBA[0] = ui_rgb_saber2_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber2_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber2_blue.integer * effectalpha;
				}
			}

			DC->addRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;

		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		DC->addRefEntityToScene(&saber);
	}
}

static void UI_DoEp3Saber(vec3_t blade_muz, vec3_t blade_dir, float lengthMax, float radius, saber_colors_t color,
	int whichSaber)
{
	vec3_t mid, rgb = { 1, 1, 1 };
	float radiusmult, effectradius, coreradius;
	float blade_len;

	qhandle_t glow = 0, blade = 0;
	refEntity_t saber;

	blade_len = lengthMax;

	if (blade_len < 0.5f)
	{
		return;
	}

	switch (color)
	{
	case SABER_RED:
		glow = redEp3GlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeEp3GlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowEp3GlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenEp3GlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueEp3GlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleEp3GlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < lengthMax)
	{
		radiusmult = 0.5 + blade_len / lengthMax / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersGlowSizeEP3.value;
	coreradius = (radius * 0.4f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersCoreSizeEP3.value;

	{
		float angle_scale = 1.0f;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			float effectalpha = 0.8f;
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			saber.shaderRGBA[0] = 0xff * effectalpha;
			saber.shaderRGBA[1] = 0xff * effectalpha;
			saber.shaderRGBA[2] = 0xff * effectalpha;
			saber.shaderRGBA[3] = 0xff * effectalpha;

			if (color >= SABER_RGB)
			{
				if (whichSaber == 0)
				{
					saber.shaderRGBA[0] = ui_rgb_saber_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber_blue.integer * effectalpha;
				}
				else
				{
					saber.shaderRGBA[0] = ui_rgb_saber2_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber2_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber2_blue.integer * effectalpha;
				}
			}

			DC->addRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;

		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		DC->addRefEntityToScene(&saber);
	}
}

static void UI_DoOTSaber(vec3_t blade_muz, vec3_t blade_dir, float lengthMax, float radius, saber_colors_t color,
	int whichSaber)
{
	vec3_t mid, rgb = { 1, 1, 1 };
	float radiusmult, effectradius, coreradius;
	float blade_len;

	qhandle_t glow = 0, blade = 0;
	refEntity_t saber;

	blade_len = lengthMax;

	if (blade_len < 0.5f)
	{
		return;
	}

	switch (color)
	{
	case SABER_RED:
		glow = redOTGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeOTGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowOTGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenOTGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueOTGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleOTGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
		glow = rgbSaberGlowShader;
		blade = otSaberCoreShader;
		break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = otSaberCoreShader;
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = otSaberCoreShader;
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = otSaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = otSaberCoreShader;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < lengthMax)
	{
		radiusmult = 0.5 + blade_len / lengthMax / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersGlowSizeOT.value;
	coreradius = (radius * 0.4f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersCoreSizeOT.value;

	coreradius *= 0.5f;

	{
		float angle_scale = 1.0f;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			float effectalpha = 0.8f;
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			saber.shaderRGBA[0] = 0xff * effectalpha;
			saber.shaderRGBA[1] = 0xff * effectalpha;
			saber.shaderRGBA[2] = 0xff * effectalpha;
			saber.shaderRGBA[3] = 0xff * effectalpha;

			if (color >= SABER_RGB)
			{
				if (whichSaber == 0)
				{
					saber.shaderRGBA[0] = ui_rgb_saber_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber_blue.integer * effectalpha;
				}
				else
				{
					saber.shaderRGBA[0] = ui_rgb_saber2_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber2_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber2_blue.integer * effectalpha;
				}
			}

			DC->addRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;

		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		DC->addRefEntityToScene(&saber);
	}
}

static void UI_DoTFASaber(vec3_t blade_muz, vec3_t blade_dir, float lengthMax, float radius, saber_colors_t color,
	int whichSaber)
{
	vec3_t mid, rgb = { 1, 1, 1 };
	float radiusmult, effectradius, coreradius;
	float blade_len;

	qhandle_t glow = 0, blade = 0;
	refEntity_t saber;

	blade_len = lengthMax;

	if (blade_len < 0.5f)
	{
		return;
	}

	switch (color)
	{
	case SABER_RED:
		glow = redSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
		glow = rgbSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < lengthMax)
	{
		radiusmult = 0.5 + blade_len / lengthMax / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersGlowSizeTFA.value;
	coreradius = (radius * 0.4f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersCoreSizeTFA.value;

	{
		float angle_scale = 1.0f;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			float effectalpha = 0.8f;
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			saber.shaderRGBA[0] = 0xff * effectalpha;
			saber.shaderRGBA[1] = 0xff * effectalpha;
			saber.shaderRGBA[2] = 0xff * effectalpha;
			saber.shaderRGBA[3] = 0xff * effectalpha;

			if (color >= SABER_RGB)
			{
				if (whichSaber == 0)
				{
					saber.shaderRGBA[0] = ui_rgb_saber_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber_blue.integer * effectalpha;
				}
				else
				{
					saber.shaderRGBA[0] = ui_rgb_saber2_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber2_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber2_blue.integer * effectalpha;
				}
			}

			DC->addRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;

		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		DC->addRefEntityToScene(&saber);
	}
}

static void UI_DoSaberUnstable(vec3_t blade_muz, vec3_t blade_dir, float lengthMax, float radius, saber_colors_t color,
	int whichSaber)
{
	vec3_t mid, rgb = { 1, 1, 1 };
	float radiusmult, effectradius, coreradius;
	float blade_len;

	qhandle_t glow = 0, blade = 0;
	refEntity_t saber;

	blade_len = lengthMax;

	if (blade_len < 0.5f)
	{
		return;
	}

	switch (color)
	{
	case SABER_RED:
		glow = redSaberGlowShader;
		blade = unstableRedSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeSaberGlowShader;
		blade = unstableRedSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowSaberGlowShader;
		blade = unstableRedSaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenSaberGlowShader;
		blade = unstableRedSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueSaberGlowShader;
		blade = unstableRedSaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleSaberGlowShader;
		blade = unstableRedSaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_WHITE:
	case SABER_RGB:
		glow = rgbSaberGlowShader;
		blade = unstableRedSaberCoreShader;
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = unstableRedSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = unstableRedSaberCoreShader;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < lengthMax)
	{
		radiusmult = 0.5 + blade_len / lengthMax / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersGlowSize.value;
	coreradius = (radius * 0.4f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersCoreSize.value;

	{
		float angle_scale = 1.0f;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			float effectalpha = 0.8f;
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			saber.shaderRGBA[0] = 0xff * effectalpha;
			saber.shaderRGBA[1] = 0xff * effectalpha;
			saber.shaderRGBA[2] = 0xff * effectalpha;
			saber.shaderRGBA[3] = 0xff * effectalpha;

			if (color >= SABER_RGB)
			{
				if (whichSaber == 0)
				{
					saber.shaderRGBA[0] = ui_rgb_saber_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber_blue.integer * effectalpha;
				}
				else
				{
					saber.shaderRGBA[0] = ui_rgb_saber2_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber2_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber2_blue.integer * effectalpha;
				}
			}

			DC->addRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;

		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		DC->addRefEntityToScene(&saber);
	}
}

static void UI_DoCustomSaber(vec3_t blade_muz, vec3_t blade_dir, float lengthMax, float radius, saber_colors_t color,
	int whichSaber)
{
	vec3_t mid;
	float radiusmult, effectradius, coreradius;
	float blade_len;

	qhandle_t glow = 0, blade = 0;
	refEntity_t saber;

	blade_len = lengthMax;

	if (blade_len < 0.5f)
	{
		return;
	}

	switch (color)
	{
	case SABER_RED:
		glow = redSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_ORANGE:
		glow = orangeSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_YELLOW:
		glow = yellowSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_GREEN:
		glow = greenSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_BLUE:
		glow = blueSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_PURPLE:
		glow = purpleSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_RGB:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < lengthMax)
	{
		radiusmult = 0.5 + blade_len / lengthMax / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersGlowSizeCSM.value;
	coreradius = (radius * 0.4f + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * ui_SFXSabersCoreSizeCSM.value;

	{
		float angle_scale = 1.0f;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			float effectalpha = 0.8f;
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			saber.shaderRGBA[0] = 0xff * effectalpha;
			saber.shaderRGBA[1] = 0xff * effectalpha;
			saber.shaderRGBA[2] = 0xff * effectalpha;
			saber.shaderRGBA[3] = 0xff * effectalpha;

			if (color >= SABER_RGB)
			{
				if (whichSaber == 0)
				{
					saber.shaderRGBA[0] = ui_rgb_saber_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber_blue.integer * effectalpha;
				}
				else
				{
					saber.shaderRGBA[0] = ui_rgb_saber2_red.integer * effectalpha;
					saber.shaderRGBA[1] = ui_rgb_saber2_green.integer * effectalpha;
					saber.shaderRGBA[2] = ui_rgb_saber2_blue.integer * effectalpha;
				}
			}

			DC->addRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;

		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		DC->addRefEntityToScene(&saber);
	}
}

static void UI_DoSaber(vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color,
	int whichSaber)
{
	vec3_t mid, rgb = { 1, 1, 1 };
	qhandle_t blade = 0, glow = 0;
	refEntity_t saber;
	float radiusmult;
	float radiusRange;
	float radiusStart;

	if (length < 0.5f)
	{
		// if the thing is so short, just forget even adding me.
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	switch (color)
	{
	case SABER_RED:
		glow = redSaberGlowShader;
		blade = redSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeSaberGlowShader;
		blade = orangeSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowSaberGlowShader;
		blade = yellowSaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenSaberGlowShader;
		blade = greenSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueSaberGlowShader;
		blade = blueSaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleSaberGlowShader;
		blade = purpleSaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = limeSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
	{
		int i;
		if (whichSaber == 0)
			VectorSet(rgb, ui_rgb_saber_red.integer, ui_rgb_saber_green.integer, ui_rgb_saber_blue.integer);
		else
			VectorSet(rgb, ui_rgb_saber2_red.integer, ui_rgb_saber2_green.integer, ui_rgb_saber2_blue.integer);

		for (i = 0; i < 3; i++)
			rgb[i] /= 255;

		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
	}
	break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		rgb_adjust_scipted_saber_color(rgb, whichSaber);
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		rgb_adjust_scipted_saber_color(rgb, whichSaber);
		break;
	default:
		glow = blueSaberGlowShader;
		blade = blueSaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	}

	memset(&saber, 0, sizeof(refEntity_t));

	saber.saberLength = length;

	if (length < lengthMax)
	{
		radiusmult = 1.0 + 2.0 / length; // Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	radiusRange = radius * 0.075f;
	radiusStart = radius - radiusRange;

	saber.radius = (radiusStart + Q_flrand(-1.0f, 1.0f) * radiusRange) * radiusmult;

	VectorCopy(origin, saber.origin);
	VectorCopy(dir, saber.axis[0]);
	saber.reType = RT_SABER_GLOW;
	saber.customShader = glow;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

	if (color != SABER_RGB)
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	else
	{
		int i1;
		for (i1 = 0; i1 < 3; i1++)
			saber.shaderRGBA[i1] = rgb[i1] * 255;
		saber.shaderRGBA[3] = 255;
	}

	DC->addRefEntityToScene(&saber);

	// Do the hot core
	VectorMA(origin, length, dir, saber.origin);
	VectorMA(origin, -1, dir, saber.oldorigin);
	saber.customShader = blade;
	saber.reType = RT_LINE;
	radiusStart = radius / 3.0f;
	saber.radius = (radiusStart + Q_flrand(-1.0f, 1.0f) * radiusRange) * radiusmult;

	DC->addRefEntityToScene(&saber);

	if (color != SABER_RGB)
		return;

	saber.customShader = blade;
	saber.reType = RT_LINE;
	saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	saber.radius = (radiusStart + Q_flrand(-1.0f, 1.0f) * radiusRange) * radiusmult;
	DC->addRefEntityToScene(&saber);
}

saber_colors_t TranslateSaberColor(const char* name)
{
	if (!Q_stricmp(name, "red"))
	{
		return SABER_RED;
	}
	if (!Q_stricmp(name, "orange"))
	{
		return SABER_ORANGE;
	}
	if (!Q_stricmp(name, "yellow"))
	{
		return SABER_YELLOW;
	}
	if (!Q_stricmp(name, "green"))
	{
		return SABER_GREEN;
	}
	if (!Q_stricmp(name, "blue"))
	{
		return SABER_BLUE;
	}
	if (!Q_stricmp(name, "purple"))
	{
		return SABER_PURPLE;
	}
	if (!Q_stricmp(name, "rgb"))
	{
		return SABER_RGB;
	}
	if (!Q_stricmp(name, "pimp"))
	{
		return SABER_PIMP;
	}
	if (!Q_stricmp(name, "white"))
	{
		return SABER_WHITE;
	}
	if (!Q_stricmp(name, "black"))
	{
		return SABER_BLACK;
	}
	if (!Q_stricmp(name, "scripted"))
	{
		return SABER_SCRIPTED;
	}
	if (!Q_stricmp(name, "custom"))
	{
		return SABER_CUSTOM;
	}
	if (!Q_stricmp(name, "lime"))
	{
		return SABER_LIME;
	}
	if (!Q_stricmp(name, "random"))
	{
		return static_cast<saber_colors_t>(Q_irand(SABER_ORANGE, SABER_PURPLE));
	}
	float colors[3];
	Q_parseSaberColor(name, colors);
	int colourArray[3]{};
	for (int i = 0; i < 3; i++)
	{
		colourArray[i] = static_cast<int>(colors[i] * 255);
	}
	return static_cast<saber_colors_t>(colourArray[0] + (colourArray[1] << 8) + (colourArray[2] << 16) + (1 << 24));
}

saberType_t TranslateSaberType(const char* name)
{
	if (!Q_stricmp(name, "SABER_SINGLE"))
	{
		return SABER_SINGLE;
	}
	if (!Q_stricmp(name, "SABER_STAFF"))
	{
		return SABER_STAFF;
	}
	if (!Q_stricmp(name, "SABER_BROAD"))
	{
		return SABER_BROAD;
	}
	if (!Q_stricmp(name, "SABER_PRONG"))
	{
		return SABER_PRONG;
	}
	if (!Q_stricmp(name, "SABER_DAGGER"))
	{
		return SABER_DAGGER;
	}
	if (!Q_stricmp(name, "SABER_ARC"))
	{
		return SABER_ARC;
	}
	if (!Q_stricmp(name, "SABER_SAI"))
	{
		return SABER_SAI;
	}
	if (!Q_stricmp(name, "SABER_CLAW"))
	{
		return SABER_CLAW;
	}
	if (!Q_stricmp(name, "SABER_LANCE"))
	{
		return SABER_LANCE;
	}
	if (!Q_stricmp(name, "SABER_STAR"))
	{
		return SABER_STAR;
	}
	if (!Q_stricmp(name, "SABER_TRIDENT"))
	{
		return SABER_TRIDENT;
	}
	if (!Q_stricmp(name, "SABER_SITH_SWORD"))
	{
		return SABER_SITH_SWORD;
	}
	if (!Q_stricmp(name, "SABER_UNSTABLE"))
	{
		return SABER_UNSTABLE;
	}
	if (!Q_stricmp(name, "SABER_STAFF_UNSTABLE"))
	{
		return SABER_STAFF_UNSTABLE;
	}
	if (!Q_stricmp(name, "SABER_THIN"))
	{
		return SABER_THIN;
	}
	if (!Q_stricmp(name, "SABER_STAFF_THIN"))
	{
		return SABER_STAFF_THIN;
	}
	if (!Q_stricmp(name, "SABER_SFX"))
	{
		return SABER_SFX;
	}
	if (!Q_stricmp(name, "SABER_STAFF_SFX"))
	{
		return SABER_STAFF_SFX;
	}
	if (!Q_stricmp(name, "SABER_CUSTOMSFX"))
	{
		return SABER_CUSTOMSFX;
	}
	if (!Q_stricmp(name, "SABER_BACKHAND"))
	{
		return SABER_BACKHAND;
	}
	if (!Q_stricmp(name, "SABER_YODA"))
	{
		return SABER_YODA;
	}
	if (!Q_stricmp(name, "SABER_DOOKU"))
	{
		return SABER_DOOKU;
	}
	if (!Q_stricmp(name, "SABER_PALP"))
	{
		return SABER_PALP;
	}
	if (!Q_stricmp(name, "SABER_ANAKIN"))
	{
		return SABER_ANAKIN;
	}
	if (!Q_stricmp(name, "SABER_GRIE"))
	{
		return SABER_GRIE;
	}
	if (!Q_stricmp(name, "SABER_GRIE4"))
	{
		return SABER_GRIE4;
	}
	if (!Q_stricmp(name, "SABER_OBIWAN"))
	{
		return SABER_OBIWAN;
	}
	if (!Q_stricmp(name, "SABER_ASBACKHAND"))
	{
		return SABER_ASBACKHAND;
	}
	if (!Q_stricmp(name, "SABER_ELECTROSTAFF"))
	{
		return SABER_ELECTROSTAFF;
	}
	if (!Q_stricmp(name, "SABER_WINDU"))
	{
		return SABER_WINDU;
	}
	if (!Q_stricmp(name, "SABER_VADER"))
	{
		return SABER_VADER;
	}
	if (!Q_stricmp(name, "SABER_STAFF_MAUL"))
	{
		return SABER_STAFF_MAUL;
	}
	if (!Q_stricmp(name, "SABER_KENOBI"))
	{
		return SABER_KENOBI;
	}
	if (!Q_stricmp(name, "SABER_REY"))
	{
		return SABER_REY;
	}
	return SABER_SINGLE;
}

static void UI_SaberDrawBlade(itemDef_t* item, const char* saber_name, int saberModel, saberType_t saberType, vec3_t origin,
	float curYaw, int blade_num)
{
	char bladeColorString[MAX_QPATH];
	vec3_t angles = { 0 };
	int whichSaber;
	int snum;

	if (item->flags & (ITF_ISANYSABER) && item->flags & ITF_ISCHARACTER)
	{
		//it's bolted to a dude!
		angles[YAW] = curYaw;
	}
	else
	{
		angles[PITCH] = curYaw;
		angles[ROLL] = 90;
	}

	if (saberModel >= item->ghoul2.size())
	{
		//uhh... invalid index!
		return;
	}

	if (item->flags & ITF_ISSABER && saberModel < 2)
	{
		whichSaber = 0;
		snum = 0;
		DC->getCVarString("ui_saber_color", bladeColorString, sizeof bladeColorString);
	}
	else
	{
		whichSaber = 1;
		snum = 1;
		DC->getCVarString("ui_saber2_color", bladeColorString, sizeof bladeColorString);
	}
	const saber_colors_t bladeColor = TranslateSaberColor(bladeColorString);

	const float bladeLength = UI_SaberBladeLengthForSaber(saber_name, blade_num);
	const float bladeRadius = UI_SaberBladeRadiusForSaber(saber_name, blade_num);
	vec3_t bladeOrigin = { 0 };
	vec3_t axis[3] = {};
	mdxaBone_t boltMatrix;
	qboolean tagHack = qfalse;

	const char* tagName = va("*blade%d", blade_num + 1);
	int bolt = DC->g2_AddBolt(&item->ghoul2[saberModel], tagName);

	if (bolt == -1)
	{
		tagHack = qtrue;
		//hmm, just fall back to the most basic tag (this will also make it work with pre-JKA saber models
		bolt = DC->g2_AddBolt(&item->ghoul2[saberModel], "*flash");
		if (bolt == -1)
		{
			//no tag_flash either?!!
			bolt = 0;
		}
	}

	DC->g2_GetBoltMatrix(item->ghoul2, saberModel, bolt, &boltMatrix, angles, origin, uiInfo.uiDC.realTime, nullptr,
		vec3_origin); //NULL was cgs.model_draw

	// work the matrix axis stuff into the original axis and origins used.
	DC->g2_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, bladeOrigin);
	DC->g2_GiveMeVectorFromMatrix(boltMatrix, NEGATIVE_X, axis[0]);
	//front (was NEGATIVE_Y, but the md3->glm exporter screws up this tag somethin' awful)
	DC->g2_GiveMeVectorFromMatrix(boltMatrix, NEGATIVE_Y, axis[1]); //right
	DC->g2_GiveMeVectorFromMatrix(boltMatrix, POSITIVE_Z, axis[2]); //up

	const float scale = DC->xscale;

	if (tagHack)
	{
		switch (saberType)
		{
		case SABER_SINGLE:
		case SABER_DAGGER:
		case SABER_LANCE:
		case SABER_UNSTABLE:
		case SABER_THIN:
		case SABER_SFX:
		case SABER_CUSTOMSFX:
		case SABER_YODA:
		case SABER_DOOKU:
		case SABER_BACKHAND:
		case SABER_PALP:
		case SABER_ANAKIN:
		case SABER_GRIE:
		case SABER_GRIE4:
		case SABER_OBIWAN:
		case SABER_ASBACKHAND:
		case SABER_WINDU:
		case SABER_VADER:
		case SABER_KENOBI:
		case SABER_REY:
			break;
		case SABER_STAFF:
		case SABER_STAFF_UNSTABLE:
		case SABER_STAFF_THIN:
		case SABER_STAFF_SFX:
		case SABER_STAFF_MAUL:
		case SABER_ELECTROSTAFF:
			if (blade_num == 1)
			{
				VectorScale(axis[0], -1, axis[0]);
				VectorMA(bladeOrigin, 16 * scale, axis[0], bladeOrigin);
			}
			break;
		case SABER_BROAD:
			if (blade_num == 0)
			{
				VectorMA(bladeOrigin, -1 * scale, axis[1], bladeOrigin);
			}
			else if (blade_num == 1)
			{
				VectorMA(bladeOrigin, 1 * scale, axis[1], bladeOrigin);
			}
			break;
		case SABER_PRONG:
			if (blade_num == 0)
			{
				VectorMA(bladeOrigin, -3 * scale, axis[1], bladeOrigin);
			}
			else if (blade_num == 1)
			{
				VectorMA(bladeOrigin, 3 * scale, axis[1], bladeOrigin);
			}
			break;
		case SABER_ARC:
			VectorSubtract(axis[1], axis[2], axis[1]);
			VectorNormalize(axis[1]);
			switch (blade_num)
			{
			case 0:
				VectorMA(bladeOrigin, 8 * scale, axis[0], bladeOrigin);
				VectorScale(axis[0], 0.75f, axis[0]);
				VectorScale(axis[1], 0.25f, axis[1]);
				VectorAdd(axis[0], axis[1], axis[0]);
				break;
			case 1:
				VectorScale(axis[0], 0.25f, axis[0]);
				VectorScale(axis[1], 0.75f, axis[1]);
				VectorAdd(axis[0], axis[1], axis[0]);
				break;
			case 2:
				VectorMA(bladeOrigin, -8 * scale, axis[0], bladeOrigin);
				VectorScale(axis[0], -0.25f, axis[0]);
				VectorScale(axis[1], 0.75f, axis[1]);
				VectorAdd(axis[0], axis[1], axis[0]);
				break;
			case 3:
				VectorMA(bladeOrigin, -16 * scale, axis[0], bladeOrigin);
				VectorScale(axis[0], -0.75f, axis[0]);
				VectorScale(axis[1], 0.25f, axis[1]);
				VectorAdd(axis[0], axis[1], axis[0]);
				break;
			default:;
			}
			break;
		case SABER_SAI:
			if (blade_num == 1)
			{
				VectorMA(bladeOrigin, -3 * scale, axis[1], bladeOrigin);
			}
			else if (blade_num == 2)
			{
				VectorMA(bladeOrigin, 3 * scale, axis[1], bladeOrigin);
			}
			break;
		case SABER_CLAW:
			switch (blade_num)
			{
			case 0:
				VectorMA(bladeOrigin, 2 * scale, axis[0], bladeOrigin);
				VectorMA(bladeOrigin, 2 * scale, axis[2], bladeOrigin);
				break;
			case 1:
				VectorMA(bladeOrigin, 2 * scale, axis[0], bladeOrigin);
				VectorMA(bladeOrigin, 2 * scale, axis[2], bladeOrigin);
				VectorMA(bladeOrigin, 2 * scale, axis[1], bladeOrigin);
				break;
			case 2:
				VectorMA(bladeOrigin, 2 * scale, axis[0], bladeOrigin);
				VectorMA(bladeOrigin, 2 * scale, axis[2], bladeOrigin);
				VectorMA(bladeOrigin, -2 * scale, axis[1], bladeOrigin);
				break;
			default:;
			}
			break;
		case SABER_STAR:
			switch (blade_num)
			{
			case 0:
				VectorMA(bladeOrigin, 8 * scale, axis[0], bladeOrigin);
				break;
			case 1:
				VectorScale(axis[0], 0.33f, axis[0]);
				VectorScale(axis[2], 0.67f, axis[2]);
				VectorAdd(axis[0], axis[2], axis[0]);
				VectorMA(bladeOrigin, 8 * scale, axis[0], bladeOrigin);
				break;
			case 2:
				VectorScale(axis[0], -0.33f, axis[0]);
				VectorScale(axis[2], 0.67f, axis[2]);
				VectorAdd(axis[0], axis[2], axis[0]);
				VectorMA(bladeOrigin, 8 * scale, axis[0], bladeOrigin);
				break;
			case 3:
				VectorScale(axis[0], -1, axis[0]);
				VectorMA(bladeOrigin, 8 * scale, axis[0], bladeOrigin);
				break;
			case 4:
				VectorScale(axis[0], -0.33f, axis[0]);
				VectorScale(axis[2], -0.67f, axis[2]);
				VectorAdd(axis[0], axis[2], axis[0]);
				VectorMA(bladeOrigin, 8 * scale, axis[0], bladeOrigin);
				break;
			case 5:
				VectorScale(axis[0], 0.33f, axis[0]);
				VectorScale(axis[2], -0.67f, axis[2]);
				VectorAdd(axis[0], axis[2], axis[0]);
				VectorMA(bladeOrigin, 8 * scale, axis[0], bladeOrigin);
				break;
			default:;
			}
			break;
		case SABER_TRIDENT:
			switch (blade_num)
			{
			case 0:
				VectorMA(bladeOrigin, 24 * scale, axis[0], bladeOrigin);
				break;
			case 1:
				VectorMA(bladeOrigin, -6 * scale, axis[1], bladeOrigin);
				VectorMA(bladeOrigin, 24 * scale, axis[0], bladeOrigin);
				break;
			case 2:
				VectorMA(bladeOrigin, 6 * scale, axis[1], bladeOrigin);
				VectorMA(bladeOrigin, 24 * scale, axis[0], bladeOrigin);
				break;
			case 3:
				VectorMA(bladeOrigin, -32 * scale, axis[0], bladeOrigin);
				VectorScale(axis[0], -1, axis[0]);
				break;
			default:;
			}
			break;
		case SABER_SITH_SWORD:
			//no blade
			break;
		default:
			break;
		}
	}
	if (saberType == SABER_SITH_SWORD)
	{
		//draw no blade
		return;
	}

	if (ui_SFXSabers.integer < 1)
	{
		// Draw the Raven blade.
		if (saberType == SABER_UNSTABLE)
		{
			UI_DoSaberUnstable(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
		}
		else
		{
			UI_DoSaber(bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, whichSaber);
		}
	}
	else if (saberType != SABER_SITH_SWORD)
	{
		switch (ui_SFXSabers.integer)
		{
		case 1:
			if (saberType == SABER_UNSTABLE || saberType == SABER_STAFF_UNSTABLE)
			{
				UI_DoTFASaber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			else
			{
				UI_DoBattlefrontSaber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			break;
		case 2:
			if (saberType == SABER_UNSTABLE || saberType == SABER_STAFF_UNSTABLE)
			{
				UI_DoTFASaber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			else
			{
				UI_DoSFXSaber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			break;
		case 3:
			if (saberType == SABER_UNSTABLE || saberType == SABER_STAFF_UNSTABLE)
			{
				UI_DoTFASaber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			else
			{
				UI_DoEp1Saber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			break;
		case 4:
			if (saberType == SABER_UNSTABLE || saberType == SABER_STAFF_UNSTABLE)
			{
				UI_DoTFASaber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			else
			{
				UI_DoEp2Saber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			break;
		case 5:
			if (saberType == SABER_UNSTABLE || saberType == SABER_STAFF_UNSTABLE)
			{
				UI_DoTFASaber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			else
			{
				UI_DoEp3Saber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			break;
		case 6:
			if (saberType == SABER_UNSTABLE || saberType == SABER_STAFF_UNSTABLE)
			{
				UI_DoTFASaber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			else
			{
				UI_DoOTSaber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			break;
		case 7:
			if (saberType == SABER_UNSTABLE || saberType == SABER_STAFF_UNSTABLE)
			{
				UI_DoTFASaber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			else
			{
				UI_DoTFASaber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			break;
		case 8:
			if (saberType == SABER_UNSTABLE || saberType == SABER_STAFF_UNSTABLE)
			{
				UI_DoTFASaber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			else
			{
				UI_DoCustomSaber(bladeOrigin, axis[0], bladeLength, bladeRadius, bladeColor, whichSaber);
			}
			break;
		default:;
		}
	}
}

extern qboolean ItemParse_asset_model_go(itemDef_t* item, const char* name);
extern qboolean ItemParse_model_g2skin_go(itemDef_t* item, const char* skinName);

static void UI_GetSaberForMenu(char* saber, int saberNum)
{
	char saberTypeString[MAX_QPATH] = { 0 };
	saberType_t saberType = SABER_NONE;

	if (saberNum == 0)
	{
		DC->getCVarString("g_saber", saber, MAX_QPATH);
	}
	else
	{
		DC->getCVarString("g_saber2", saber, MAX_QPATH);
	}
	//read this from the sabers.cfg
	UI_SaberTypeForSaber(saber, saberTypeString);
	if (saberTypeString[0])
	{
		saberType = TranslateSaberType(saberTypeString);
	}

	switch (uiInfo.movesTitleIndex)
	{
	case 0: //MD_ACROBATICS:
		break;
	case 1: //MD_SINGLE_FAST:
	case 2: //MD_SINGLE_MEDIUM:
	case 3: //MD_SINGLE_STRONG:
		if (saberType != SABER_SINGLE)
		{
			Q_strncpyz(saber, "single_1", MAX_QPATH);
		}
		break;
	case 4: //MD_DUAL_SABERS:
		if (saberType != SABER_SINGLE)
		{
			Q_strncpyz(saber, "single_1", MAX_QPATH);
		}
		break;
	case 5: //MD_SABER_STAFF:
		if (saberType == SABER_SINGLE || saberType == SABER_NONE)
		{
			Q_strncpyz(saber, "dual_1", MAX_QPATH);
		}
		break;
	default:;
	}
}

void UI_SaberDrawBlades(itemDef_t* item, vec3_t origin, float curYaw)
{
	//NOTE: only allows one saber type in view at a time
	int saberModel;
	int numSabers = 1;

	if (item->flags & ITF_ISCHARACTER //hacked sabermoves sabers in character's hand
		&& uiInfo.movesTitleIndex == 4 /*MD_DUAL_SABERS*/)
	{
		numSabers = 2;
	}

	for (int saberNum = 0; saberNum < numSabers; saberNum++)
	{
		char saber[MAX_QPATH];
		if (item->flags & ITF_ISCHARACTER) //hacked sabermoves sabers in character's hand
		{
			UI_GetSaberForMenu(saber, saberNum);
			saberModel = saberNum + 1;
		}
		else if (item->flags & ITF_ISSABER)
		{
			DC->getCVarString("ui_saber", saber, sizeof saber);
			saberModel = 0;
		}
		else if (item->flags & ITF_ISSABER2)
		{
			DC->getCVarString("ui_saber2", saber, sizeof saber);
			saberModel = 0;
		}
		else
		{
			return;
		}
		if (saber[0])
		{
			const int numBlades = UI_saber_numBladesForSaber(saber);
			if (numBlades)
			{
				//okay, here we go, time to draw each blade...
				char saberTypeString[MAX_QPATH] = { 0 };
				UI_SaberTypeForSaber(saber, saberTypeString);
				const saberType_t saberType = TranslateSaberType(saberTypeString);
				for (int curBlade = 0; curBlade < numBlades; curBlade++)
				{
					if (UI_SaberShouldDrawBlade(saber, curBlade))
					{
						UI_SaberDrawBlade(item, saber, saberModel, saberType, origin, curYaw, curBlade);
					}
				}
			}
		}
	}
}

void UI_SaberAttachToChar(itemDef_t* item)
{
	int numSabers = 1;

	if (item->ghoul2.size() > 2 && item->ghoul2[2].mModelindex >= 0)
	{
		//remove any extra models
		DC->g2_RemoveGhoul2Model(item->ghoul2, 2);
	}
	if (item->ghoul2.size() > 1 && item->ghoul2[1].mModelindex >= 0)
	{
		//remove any extra models
		DC->g2_RemoveGhoul2Model(item->ghoul2, 1);
	}

	if (uiInfo.movesTitleIndex == 4 /*MD_DUAL_SABERS*/)
	{
		numSabers = 2;
	}

	for (int saberNum = 0; saberNum < numSabers; saberNum++)
	{
		//bolt sabers
		char modelPath[MAX_QPATH];
		char saber[MAX_QPATH];

		UI_GetSaberForMenu(saber, saberNum);

		if (UI_SaberModelForSaber(saber, modelPath))
		{
			//successfully found a model
			const int g2Saber = DC->g2_InitGhoul2Model(item->ghoul2, modelPath, 0, 0, 0, 0, 0); //add the model
			if (g2Saber)
			{
				char skinPath[MAX_QPATH];
				//get the customSkin, if any
				if (UI_SaberSkinForSaber(saber, skinPath))
				{
					const int g2skin = DC->registerSkin(skinPath);
					DC->g2_SetSkin(&item->ghoul2[g2Saber], 0, g2skin);
					//this is going to set the surfs on/off matching the skin file
				}
				else
				{
					DC->g2_SetSkin(&item->ghoul2[g2Saber], -1, 0); //turn off custom skin
				}
				int boltNum;
				if (saberNum == 0)
				{
					boltNum = DC->g2_AddBolt(&item->ghoul2[0], "*r_hand");
				}
				else
				{
					boltNum = DC->g2_AddBolt(&item->ghoul2[0], "*l_hand");
				}
				re.G2API_AttachG2Model(&item->ghoul2[g2Saber], &item->ghoul2[0], boltNum, 0);
			}
		}
	}
}