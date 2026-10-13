/*
===========================================================================
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

// bg_weather.c -- gameplay side of the global weather effects, shared by the game (server) and the cgame.
//
// The renderer's weather system is client-only (and missing on dedicated servers), so the gameplay
// effects of weather (saber fizz in rain, acid rain and lava weather damage) keep their own copy of
// the weather state here. It is fed the same "*<command>" strings the renderer gets through the
// CS_EFFECTS configstrings (game: when it creates one, cgame: CG_ParseWeatherEffect), and mirrors
// what the SP renderer does with them (code/rd-vanilla/tr_WorldEffects.cpp RE_WorldEffectCommand /
// R_GetChanceOfSaberFizz / mOutsidePain).
//
// BG_IsOutside is the gameplay "is this point outside" test: inside / outside weather brushes first
// (CONTENTS_OUTSIDE / CONTENTS_INSIDE, the same point contents test the SP renderer's COutside uses),
// falling back to a trace straight up looking for sky. It only uses collision model services, so it
// works on dedicated servers.

#include "qcommon/q_shared.h"
#include "bg_public.h"

#if defined(_GAME)
#include "g_local.h"
#elif defined(_CGAME)
#include "cgame/cg_local.h"
#endif

#if defined(_GAME) || defined(_CGAME)

#define BG_WEATHER_MAX_CLOUDS		5		// MAX_PARTICLE_CLOUDS in the renderers: further clouds are ignored
#define BG_WEATHER_DEFAULT_GRAVITY	300.0f	// CParticleCloud default mGravity (snow, lava)
#define BG_WEATHER_SKY_TRACE_DIST	16384.0f

typedef struct bgWeatherState_s
{
	int numClouds;			// particle clouds the renderer would have
	int numWaterClouds;		// ... of which have mWaterParticles
	float waterGravity;		// sum of mGravity over the water clouds
	qboolean outsidePain;	// acidrain / outsidepain (SP mOutside.mOutsidePain)
	qboolean lava;			// lava cloud active (new: lava weather damage)
} bgWeatherState_t;

static bgWeatherState_t bgWeather;

void BG_WeatherReset(void)
{
	memset(&bgWeather, 0, sizeof bgWeather);
}

// returns qfalse if the renderer would refuse the cloud (all cloud slots used)
static qboolean BG_WeatherAddCloud(const qboolean waterParticles, const float gravity)
{
	if (bgWeather.numClouds >= BG_WEATHER_MAX_CLOUDS)
	{
		return qfalse;
	}
	bgWeather.numClouds++;

	if (waterParticles)
	{
		bgWeather.numWaterClouds++;
		bgWeather.waterGravity += gravity;
	}
	return qtrue;
}

// command: a weather command without the leading '*', e.g. "acidrain 500"
void BG_WeatherCommand(const char* command)
{
	char token[MAX_QPATH];
	int i = 0;

	if (!command)
	{
		return;
	}

	while (*command == ' ' || *command == '\t')
	{
		command++;
	}
	while (command[i] && command[i] != ' ' && command[i] != '\t' && i < (int)sizeof token - 1)
	{
		token[i] = command[i];
		i++;
	}
	token[i] = '\0';

	if (!Q_stricmp(token, "clear") || !Q_stricmp(token, "die"))
	{
		BG_WeatherReset();
	}
	else if (!Q_stricmp(token, "lightrain") || !Q_stricmp(token, "rain"))
	{
		BG_WeatherAddCloud(qtrue, 2000.0f);
	}
	else if (!Q_stricmp(token, "acidrain"))
	{
		if (BG_WeatherAddCloud(qtrue, 2000.0f))
		{
			bgWeather.outsidePain = qtrue;
		}
	}
	else if (!Q_stricmp(token, "heavyrain"))
	{
		BG_WeatherAddCloud(qtrue, 2800.0f);
	}
	else if (!Q_stricmp(token, "snow"))
	{
		BG_WeatherAddCloud(qtrue, BG_WEATHER_DEFAULT_GRAVITY);
	}
	else if (!Q_stricmp(token, "lava"))
	{
		if (BG_WeatherAddCloud(qtrue, BG_WEATHER_DEFAULT_GRAVITY))
		{
			bgWeather.lava = qtrue;
		}
	}
	else if (!Q_stricmp(token, "spacedust"))
	{
		BG_WeatherAddCloud(qtrue, 0.0f);
	}
	else if (!Q_stricmp(token, "sandstorm") || !Q_stricmp(token, "sand") || !Q_stricmp(token, "fog")
		|| !Q_stricmp(token, "heavyrainfog") || !Q_stricmp(token, "light_fog"))
	{
		BG_WeatherAddCloud(qfalse, 0.0f);
	}
	else if (!Q_stricmp(token, "outsidepain"))
	{
		bgWeather.outsidePain = bgWeather.outsidePain ? qfalse : qtrue;
	}
	// wind, zones, freeze, outsideshake etc. don't matter for gameplay
}

// SP R_GetChanceOfSaberFizz: chance per 50 ms server frame (0.1 for rain / acid rain)
float BG_WeatherFizzChance(void)
{
	if (bgWeather.numWaterClouds <= 0)
	{
		return 0.0f;
	}
	return bgWeather.waterGravity / 20000.0f / (float)bgWeather.numWaterClouds;
}

qboolean BG_WeatherOutsidePain(void)
{
	return bgWeather.outsidePain;
}

qboolean BG_WeatherLava(void)
{
	return bgWeather.lava;
}

// The numbered weather presets: the in-game menu's weather selector (r_weather), the "r_weather N" lines of the
// per-map Weather/<mapname>.cfg files and "weather <number>". Each one is the weather command that makes exactly what
// the renderers' old r_weather preset N made (rd-vanilla tr_WorldEffects.cpp / rend2 tr_weather.cpp).
// Returns NULL for 0 (none) and for numbers that are no preset.
const char* BG_WeatherPresetName(const int preset)
{
	static const char* const presets[BG_WEATHER_NUM_PRESETS] = {
		NULL,			// 0: none
		"snow",			// 1
		"rain",			// 2
		"lava",			// 3
		"sand",			// 4
		"heavyrainfog",	// 5
		"fog",			// 6
		"acidrain",		// 7
		"wind",			// 8
		"spacedust",	// 9
	};

	if (preset <= 0 || preset >= BG_WEATHER_NUM_PRESETS)
	{
		return NULL;
	}
	return presets[preset];
}

qboolean BG_IsOutside(const vec3_t pos, const int passEntityNum)
{
	trace_t tr;
	vec3_t end;
	int contents;

#if defined(_GAME)
	contents = trap->PointContents(pos, passEntityNum);
#else
	(void)passEntityNum;
	contents = trap->CM_PointContents(pos, 0);
#endif

	// weather brushes decide first, as in the renderers
	if (contents & CONTENTS_OUTSIDE)
	{
		return qtrue;
	}
	if (contents & CONTENTS_INSIDE)
	{
		return qfalse;
	}
	if (contents & (CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		return qfalse;
	}

	// no weather brush here: outside if nothing but sky is straight above
	VectorSet(end, pos[0], pos[1], pos[2] + BG_WEATHER_SKY_TRACE_DIST);
	memset(&tr, 0, sizeof tr);
#if defined(_GAME)
	trap->Trace(&tr, pos, NULL, NULL, end, passEntityNum, MASK_SOLID, qfalse, 0, 0);
#else
	trap->CM_Trace(&tr, pos, end, NULL, NULL, 0, MASK_SOLID, 0);
#endif

	if (tr.startsolid || tr.allsolid)
	{
		return qfalse;
	}
	if (tr.fraction >= 1.0f)
	{
		return qtrue;
	}
	return (tr.surfaceFlags & SURF_SKY) ? qtrue : qfalse;
}

#endif // _GAME || _CGAME