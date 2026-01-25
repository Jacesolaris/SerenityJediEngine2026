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

#pragma once

#include "client/cl_cgameapi.h"
#include "ghoul2/G2.h"

extern cvar_t* fx_debug;

#ifdef _DEBUG
extern cvar_t* fx_freeze;
#endif

extern cvar_t* fx_countScale;
extern cvar_t* fx_nearCull;
extern cvar_t* fx_optimizedParticles;

class SFxHelper
{
public:
	int mTime;
	int mOldTime;
	int mFrameTime;
	bool mTimeFrozen;
	float mRealTime;
	refdef_t* refdef;
#ifdef _DEBUG
	int mMainRefs;
	int mMiniRefs;
#endif

public:
	SFxHelper();

	int GetTime(void) const { return mTime; }
	int GetFrameTime(void) const { return mFrameTime; }

	void ReInit(refdef_t* pRefdef);
	void AdjustTime(int frametime);

	// These functions are wrapped and used by the fx system in case it makes things a bit more portable
	static void Print(const char* msg, ...);

	// File handling
	static int OpenFile(const char* file, fileHandle_t* fh, int mode)
	{
		return FS_FOpenFileByMode(file, fh, FS_READ);
	}

	static int ReadFile(void* data, const int len, const fileHandle_t fh)
	{
		FS_Read(data, len, fh);
		return 1;
	}

	static void CloseFile(const fileHandle_t fh)
	{
		FS_FCloseFile(fh);
	}

	// Sound
	static void PlaySound(vec3_t origin, int entityNum, int entchannel, const sfxHandle_t sfxHandle, int volume,
		int radius)
	{
		//S_StartSound( origin, ENTITYNUM_NONE, CHAN_AUTO, sfxHandle, volume, radius );
		S_StartSound(origin, ENTITYNUM_NONE, CHAN_AUTO, sfxHandle);
	}

	static void PlayLocalSound(const sfxHandle_t sfxHandle, const int entchannel)
	{
		//S_StartSound( origin, ENTITYNUM_NONE, CHAN_AUTO, sfxHandle, volume, radius );
		S_StartLocalSound(sfxHandle, entchannel);
	}

	static int RegisterSound(const char* sound)
	{
		return S_RegisterSound(sound);
	}

	// Physics/collision
	static void Trace(trace_t& tr, vec3_t start, vec3_t min, vec3_t max, vec3_t end, const int skipEntNum,
		const int flags)
	{
		const auto td = reinterpret_cast<TCGTrace*>(cl.mSharedMemory);

		if (!min)
		{
			min = vec3_origin;
		}

		if (!max)
		{
			max = vec3_origin;
		}

		memset(td, 0, sizeof * td);
		VectorCopy(start, td->mStart);
		VectorCopy(min, td->mMins);
		VectorCopy(max, td->mMaxs);
		VectorCopy(end, td->mEnd);
		td->mSkipNumber = skipEntNum;
		td->mMask = flags;

		CGVM_Trace();

		tr = td->mResult;
	}

	static void G2Trace(trace_t& tr, vec3_t start, vec3_t min, vec3_t max, vec3_t end, const int skipEntNum,
		const int flags)
	{
		const auto td = reinterpret_cast<TCGTrace*>(cl.mSharedMemory);

		if (!min)
		{
			min = vec3_origin;
		}

		if (!max)
		{
			max = vec3_origin;
		}

		memset(td, 0, sizeof * td);
		VectorCopy(start, td->mStart);
		VectorCopy(min, td->mMins);
		VectorCopy(max, td->mMaxs);
		VectorCopy(end, td->mEnd);
		td->mSkipNumber = skipEntNum;
		td->mMask = flags;

		CGVM_G2Trace();

		tr = td->mResult;
	}

	static void AddGhoul2Decal(const int shader, vec3_t start, vec3_t dir, const float size)
	{
		const auto td = reinterpret_cast<TCGG2Mark*>(cl.mSharedMemory);

		td->size = size;
		td->shader = shader;
		VectorCopy(start, td->start);
		VectorCopy(dir, td->dir);

		CGVM_G2Mark();
	}

	void AddFxToScene(const refEntity_t* ent)
	{
#ifdef _DEBUG
		mMainRefs++;

		assert(!ent || ent->renderfx >= 0);
#endif
		re->AddRefEntityToScene(ent);
	}

	void AddFxToScene(const miniRefEntity_t* ent)
	{
#ifdef _DEBUG
		mMiniRefs++;

		assert(!ent || ent->renderfx >= 0);
#endif
		re->AddMiniRefEntityToScene(ent);
	}

	static void AddLightToScene(vec3_t org, const float radius, const float red, const float green, const float blue)
	{
		re->AddLightToScene(org, radius, red, green, blue);
	}

	static int RegisterShader(const char* shader)
	{
		return re->RegisterShader(shader);
	}

	static int RegisterModel(const char* model)
	{
		return re->RegisterModel(model);
	}

	static void AddPolyToScene(const int shader, const int count, const polyVert_t* verts)
	{
		re->AddPolyToScene(shader, count, verts, 1);
	}

	static void AddDecalToScene(const qhandle_t shader, const vec3_t origin, const vec3_t dir, const float orientation,
		const float r, const float g, const float b, const float a, const qboolean alphaFade,
		const float radius, const qboolean temporary)
	{
		re->AddDecalToScene(shader, origin, dir, orientation, r, g, b, a, alphaFade, radius, temporary);
	}

	static void CameraShake(vec3_t origin, float intensity, int radius, int time);
	static qboolean GetOriginAxisFromBolt(CGhoul2Info_v* pGhoul2, int mEntNum, int modelNum, int boltNum,
		vec3_t /*out*/origin, vec3_t /*out*/axis[3]);
};

extern SFxHelper theFxHelper;
