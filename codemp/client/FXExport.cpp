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

#include "client.h"
#include "FxScheduler.h"

//#define __FXCHECKER

#ifdef __FXCHECKER
#include <float.h>
#endif // __FXCHECKER

int FX_RegisterEffect(const char* file)
{
	return theFxScheduler.RegisterEffect(file, true);
}

void FX_PlayEffect(const char* file, vec3_t org, vec3_t fwd, const int vol, const int rad)
{
#ifdef __FXCHECKER
	if (_isnan(org[0]) || _isnan(org[1]) || _isnan(org[2]))
	{
		assert(0);
	}
	if (_isnan(fwd[0]) || _isnan(fwd[1]) || _isnan(fwd[2]))
	{
		assert(0);
	}
	if (fabs(fwd[0]) < 0.1 && fabs(fwd[1]) < 0.1 && fabs(fwd[2]) < 0.1)
	{
		assert(0);
	}
#endif // __FXCHECKER

	theFxScheduler.PlayEffect(file, org, fwd, vol, rad);
}

void FX_PlayEffectID(const int id, vec3_t org, vec3_t fwd, const int vol, const int rad, const qboolean isPortal)
{
#ifdef __FXCHECKER
	if (_isnan(org[0]) || _isnan(org[1]) || _isnan(org[2]))
	{
		assert(0);
	}
	if (_isnan(fwd[0]) || _isnan(fwd[1]) || _isnan(fwd[2]))
	{
		assert(0);
	}
	if (fabs(fwd[0]) < 0.1 && fabs(fwd[1]) < 0.1 && fabs(fwd[2]) < 0.1)
	{
		assert(0);
	}
#endif // __FXCHECKER

	theFxScheduler.PlayEffect(id, org, fwd, vol, rad, !!isPortal);
}

void FX_PlayBoltedEffectID(const int id, vec3_t org,
	const int boltInfo, CGhoul2Info_v* ghoul2, const int iLooptime, const qboolean isRelative)
{
	theFxScheduler.PlayEffect(id, org, nullptr, boltInfo, ghoul2, -1, -1, -1, qfalse, iLooptime, !!isRelative);
}

void FX_PlayEntityEffectID(const int id, vec3_t org,
	matrix3_t axis, const int boltInfo, const int entNum, const int vol, const int rad)
{
#ifdef __FXCHECKER
	if (_isnan(org[0]) || _isnan(org[1]) || _isnan(org[2]))
	{
		assert(0);
	}
#endif // __FXCHECKER

	theFxScheduler.PlayEffect(id, org, axis, boltInfo, nullptr, -1, vol, rad);
}

void FX_AddScheduledEffects(const qboolean portal)
{
	theFxScheduler.AddScheduledEffects(!!portal);
}

void FX_Draw2DEffects(const float screenXScale, const float screenYScale)
{
	theFxScheduler.Draw2DEffects(screenXScale, screenYScale);
}

int FX_InitSystem(refdef_t* refdef)
{
	return FX_Init(refdef);
}

void FX_SetRefDefFromCGame(refdef_t* refdef)
{
	FX_SetRefDef(refdef);
}

qboolean FX_FreeSystem(void)
{
	return static_cast<qboolean>(FX_Free(true));
}

void FX_AdjustTime(const int time)
{
	theFxHelper.AdjustTime(time);
}