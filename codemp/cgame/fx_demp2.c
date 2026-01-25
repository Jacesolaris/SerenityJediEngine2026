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

// DEMP2 Weapon

#include "cg_local.h"
#include "fx_local.h"

/*
---------------------------
FX_DEMP2_ProjectileThink
---------------------------
*/

void FX_DEMP2_ProjectileThink(centity_t* cent, const struct weaponInfo_s* weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	trap->FX_PlayEffectID(cgs.effects.demp2ProjectileEffect, cent->lerpOrigin, forward, -1, -1, qfalse);
}

/*
---------------------------
FX_DEMP2_HitWall
---------------------------
*/

void FX_DEMP2_HitWall(vec3_t origin, vec3_t normal)
{
	trap->FX_PlayEffectID(cgs.effects.demp2WallImpactEffect, origin, normal, -1, -1, qfalse);
}

/*
---------------------------
FX_DEMP2_HitPlayer
---------------------------
*/

void FX_DEMP2_HitPlayer(vec3_t origin, vec3_t normal, const qboolean humanoid)
{
	if (humanoid)
	{
		trap->FX_PlayEffectID(cgs.effects.demp2FleshImpactEffect, origin, normal, -1, -1, qfalse);
	}
	else
	{
		trap->FX_PlayEffectID(cgs.effects.blasterDroidImpactEffect, origin, normal, -1, -1, qfalse);
	}
}

/*
---------------------------
FX_DEMP2_AltBeam
---------------------------
*/
void FX_DEMP2_AltBeam(vec3_t start, vec3_t end, vec3_t normal, vec3_t targ1, vec3_t targ2)
{
	static vec3_t WHITE = { 1.0f, 1.0f, 1.0f };
	static vec3_t BRIGHT = { 0.75f, 0.5f, 1.0f };

	// Let's at least give it something...
	//"concussion/beam"
	trap->FX_AddLine(start, end, 0.3f, 15.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		WHITE, WHITE, 0.0f,
		175, trap->R_RegisterShader("gfx/misc/lightningFlash"),
		FX_SIZE_LINEAR | FX_ALPHA_LINEAR);

	// add some beef
	trap->FX_AddLine(start, end, 0.3f, 11.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		BRIGHT, BRIGHT, 0.0f,
		150, trap->R_RegisterShader("gfx/misc/electric2"),
		FX_SIZE_LINEAR | FX_ALPHA_LINEAR);
}

//---------------------------------------------
void FX_DEMP2_AltDetonate(vec3_t org, const float size)
{
	localEntity_t* ex = CG_AllocLocalEntity();
	ex->leType = LE_FADE_SCALE_MODEL;
	memset(&ex->refEntity, 0, sizeof(refEntity_t));

	ex->refEntity.renderfx |= RF_VOLUMETRIC;

	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 800; //1600;

	ex->radius = size;
	ex->refEntity.customShader = cgs.media.demp2ShellShader;
	ex->refEntity.hModel = cgs.media.demp2Shell;
	VectorCopy(org, ex->refEntity.origin);

	ex->color[0] = ex->color[1] = ex->color[2] = 255.0f;
}