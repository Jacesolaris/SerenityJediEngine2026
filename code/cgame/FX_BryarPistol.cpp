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

// Bryar Pistol Weapon Effects

#include "cg_headers.h"

#include "cg_media.h"
#include "FxScheduler.h"

/*
-------------------------

	MAIN FIRE

-------------------------
FX_BryarProjectileThink
-------------------------
*/
void FX_BryarProjectileThink(centity_t* cent, const weaponInfo_s* weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->gent->s.pos.trDelta, forward) == 0.0f)
	{
		if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
		{
			forward[2] = 1.0f;
		}
	}

	// hack the scale of the forward vector if we were just fired or bounced...this will shorten up the tail for a split second so tails don't clip so harshly
	int dif = cg.time - cent->gent->s.pos.trTime;

	if (dif < 75)
	{
		if (dif < 0)
		{
			dif = 0;
		}

		const float scale = dif / 75.0f * 0.95f + 0.05f;

		VectorScale(forward, scale, forward);
	}

	if (cent->gent && cent->gent->owner && cent->gent->owner->s.number > 0)
	{
		theFxScheduler.PlayEffect("bryar/NPCshot", cent->lerpOrigin, forward);
	}
	else
	{
		theFxScheduler.PlayEffect(cgs.effects.bryarShotEffect, cent->lerpOrigin, forward);
	}
}

void FX_briar_pistolProjectileThink(centity_t* cent, const weaponInfo_s* weapon)
{
	vec3_t forward;

	if (cent->currentState.eFlags & EF_USE_ANGLEDELTA)
	{
		AngleVectors(cent->currentState.angles, forward, nullptr, nullptr);
	}
	else
	{
		if (VectorNormalize2(cent->gent->s.pos.trDelta, forward) == 0.0f)
		{
			if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
			{
				forward[2] = 1.0f;
			}
		}
	}

	// hack the scale of the forward vector if we were just fired or bounced...this will shorten up the tail for a split second so tails don't clip so harshly
	int dif = cg.time - cent->gent->s.pos.trTime;

	if (dif < 75)
	{
		if (dif < 0)
		{
			dif = 0;
		}

		const float scale = dif / 75.0f * 0.95f + 0.05f;

		VectorScale(forward, scale, forward);
	}

	if (cent->gent && cent->gent->owner && cent->gent->owner->s.number > 0)
	{
		theFxScheduler.PlayEffect(cgs.effects.briar_pistolNPCShotEffect, cent->lerpOrigin, forward);
	}
	else
	{
		theFxScheduler.PlayEffect(cgs.effects.briar_pistolShotEffect, cent->lerpOrigin, forward);
	}
}

/*
-------------------------
FX_BryarHitWall
-------------------------
*/
void FX_BryarHitWall(vec3_t origin, vec3_t normal)
{
	theFxScheduler.PlayEffect(cgs.effects.bryarWallImpactEffect, origin, normal);
}

void FX_BryarsbdHitWall(vec3_t origin, vec3_t normal)
{
	theFxScheduler.PlayEffect(cgs.effects.briar_pistolWallImpactEffect, origin, normal);
}

/*
-------------------------
FX_BryarHitPlayer
-------------------------
*/
void FX_BryarHitPlayer(gentity_t* hit, vec3_t origin, vec3_t normal, const qboolean humanoid)
{
	if (humanoid)
	{
		if (hit && hit->client && hit->ghoul2.size())
		{
			CG_AddGhoul2Mark(cgs.media.bdecal_bodyburn1, flrand(3.5, 4.0), origin, normal, hit->s.number,
				hit->client->ps.origin, hit->client->renderInfo.legsYaw, hit->ghoul2, hit->s.modelScale,
				Q_irand(10000, 13000));
		}
		theFxScheduler.PlayEffect(cgs.effects.bryarFleshImpactEffect, origin, normal);
	}
	else
	{
		theFxScheduler.PlayEffect(cgs.effects.briar_pistolDroidImpactEffect, origin, normal);
	}
}

void FX_BryarsbdHitPlayer(gentity_t* hit, vec3_t origin, vec3_t normal, const qboolean humanoid)
{
	if (humanoid)
	{
		if (hit && hit->client && hit->ghoul2.size())
		{
			CG_AddGhoul2Mark(cgs.media.bdecal_bodyburn1, flrand(3.5, 4.0), origin, normal, hit->s.number,
				hit->client->ps.origin, hit->client->renderInfo.legsYaw, hit->ghoul2, hit->s.modelScale,
				Q_irand(10000, 13000));
		}
		theFxScheduler.PlayEffect(cgs.effects.briar_pistolFleshImpactEffect, origin, normal);
	}
	else
	{
		theFxScheduler.PlayEffect(cgs.effects.briar_pistolDroidImpactEffect, origin, normal);
	}
}

/*
-------------------------

	ALT FIRE

-------------------------
FX_BryarAltProjectileThink
-------------------------
*/
void FX_BryarAltProjectileThink(centity_t* cent, const weaponInfo_s* weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->gent->s.pos.trDelta, forward) == 0.0f)
	{
		if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
		{
			forward[2] = 1.0f;
		}
	}

	// hack the scale of the forward vector if we were just fired or bounced...this will shorten up the tail for a split second so tails don't clip so harshly
	int dif = cg.time - cent->gent->s.pos.trTime;

	if (dif < 75)
	{
		if (dif < 0)
		{
			dif = 0;
		}

		const float scale = dif / 75.0f * 0.95f + 0.05f;

		VectorScale(forward, scale, forward);
	}

	// see if we have some sort of extra charge going on
	for (int t = 1; t < cent->gent->count; t++)
	{
		// just add ourselves over, and over, and over when we are charged
		theFxScheduler.PlayEffect(cgs.effects.bryarPowerupShotEffect, cent->lerpOrigin, forward);
	}

	theFxScheduler.PlayEffect(cgs.effects.bryarShotEffect, cent->lerpOrigin, forward);
}

void FX_briar_pistolAltProjectileThink(centity_t* cent, const weaponInfo_s* weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->gent->s.pos.trDelta, forward) == 0.0f)
	{
		if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
		{
			forward[2] = 1.0f;
		}
	}

	// hack the scale of the forward vector if we were just fired or bounced...this will shorten up the tail for a split second so tails don't clip so harshly
	int dif = cg.time - cent->gent->s.pos.trTime;

	if (dif < 75)
	{
		if (dif < 0)
		{
			dif = 0;
		}

		const float scale = dif / 75.0f * 0.95f + 0.05f;

		VectorScale(forward, scale, forward);
	}

	// see if we have some sort of extra charge going on
	for (int t = 1; t < cent->gent->count; t++)
	{
		// just add ourselves over, and over, and over when we are charged
		theFxScheduler.PlayEffect(cgs.effects.briar_pistolPowerupShotEffect, cent->lerpOrigin, forward);
	}

	theFxScheduler.PlayEffect(cgs.effects.briar_pistolShotEffect, cent->lerpOrigin, forward);
}

/*
-------------------------
FX_BryarAltHitWall
-------------------------
*/
void FX_BryarAltHitWall(vec3_t origin, vec3_t normal, const int power)
{
	switch (power)
	{
	case 4:
	case 5:
		theFxScheduler.PlayEffect(cgs.effects.bryarWallImpactEffect3, origin, normal);
		break;

	case 2:
	case 3:
		theFxScheduler.PlayEffect(cgs.effects.bryarWallImpactEffect2, origin, normal);
		break;

	default:
		theFxScheduler.PlayEffect(cgs.effects.bryarWallImpactEffect, origin, normal);
		break;
	}
}

void FX_BryarsbdAltHitWall(vec3_t origin, vec3_t normal, const int power)
{
	switch (power)
	{
	case 4:
	case 5:
		theFxScheduler.PlayEffect(cgs.effects.briar_pistolWallImpactEffect3, origin, normal);
		break;

	case 2:
	case 3:
		theFxScheduler.PlayEffect(cgs.effects.briar_pistolWallImpactEffect2, origin, normal);
		break;

	default:
		theFxScheduler.PlayEffect(cgs.effects.briar_pistolWallImpactEffect, origin, normal);
		break;
	}
}

/*
-------------------------
FX_BryarAltHitPlayer
-------------------------
*/
void FX_BryarAltHitPlayer(gentity_t* hit, vec3_t origin, vec3_t normal, const qboolean humanoid)
{
	if (humanoid)
	{
		if (hit && hit->client && hit->ghoul2.size())
		{
			CG_AddGhoul2Mark(cgs.media.bdecal_bodyburn1, flrand(3.5, 4.0), origin, normal, hit->s.number,
				hit->client->ps.origin, hit->client->renderInfo.legsYaw, hit->ghoul2, hit->s.modelScale,
				Q_irand(10000, 13000));
		}
		theFxScheduler.PlayEffect(cgs.effects.bryarFleshImpactEffect, origin, normal);
	}
	else
	{
		theFxScheduler.PlayEffect(cgs.effects.briar_pistolDroidImpactEffect, origin, normal);
	}
}

void FX_BryarsbdAltHitPlayer(gentity_t* hit, vec3_t origin, vec3_t normal, const qboolean humanoid)
{
	if (humanoid)
	{
		if (hit && hit->client && hit->ghoul2.size())
		{
			CG_AddGhoul2Mark(cgs.media.bdecal_bodyburn1, flrand(3.5, 4.0), origin, normal, hit->s.number,
				hit->client->ps.origin, hit->client->renderInfo.legsYaw, hit->ghoul2, hit->s.modelScale,
				Q_irand(10000, 13000));
		}
		theFxScheduler.PlayEffect(cgs.effects.briar_pistolFleshImpactEffect, origin, normal);
	}
	else
	{
		theFxScheduler.PlayEffect(cgs.effects.briar_pistolDroidImpactEffect, origin, normal);
	}
}

/*
-------------------------

	MAIN FIRE

-------------------------
FX_BryaroldProjectileThink
-------------------------
*/
void FX_BryaroldProjectileThink(centity_t* cent, const weaponInfo_s* weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->gent->s.pos.trDelta, forward) == 0.0f)
	{
		if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
		{
			forward[2] = 1.0f;
		}
	}

	// hack the scale of the forward vector if we were just fired or bounced...this will shorten up the tail for a split second so tails don't clip so harshly
	int dif = cg.time - cent->gent->s.pos.trTime;

	if (dif < 75)
	{
		if (dif < 0)
		{
			dif = 0;
		}

		const float scale = dif / 75.0f * 0.95f + 0.05f;

		VectorScale(forward, scale, forward);
	}

	if (cent->gent && cent->gent->owner && cent->gent->owner->s.number > 0)
	{
		theFxScheduler.PlayEffect("bryar_old/NPCshot", cent->lerpOrigin, forward);
	}
	else
	{
		theFxScheduler.PlayEffect(cgs.effects.bryaroldShotEffect, cent->lerpOrigin, forward);
	}
}

/*
-------------------------

	ALT FIRE

-------------------------
FX_BryarAltProjectileThink
-------------------------
*/
void FX_BryaroldAltProjectileThink(centity_t* cent, const weaponInfo_s* weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->gent->s.pos.trDelta, forward) == 0.0f)
	{
		if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
		{
			forward[2] = 1.0f;
		}
	}

	// hack the scale of the forward vector if we were just fired or bounced...this will shorten up the tail for a split second so tails don't clip so harshly
	int dif = cg.time - cent->gent->s.pos.trTime;

	if (dif < 75)
	{
		if (dif < 0)
		{
			dif = 0;
		}

		const float scale = dif / 75.0f * 0.95f + 0.05f;

		VectorScale(forward, scale, forward);
	}

	// see if we have some sort of extra charge going on
	for (int t = 1; t < cent->gent->count; t++)
	{
		// just add ourselves over, and over, and over when we are charged
		theFxScheduler.PlayEffect(cgs.effects.bryaroldPowerupShotEffect, cent->lerpOrigin, forward);
	}

	theFxScheduler.PlayEffect(cgs.effects.bryaroldShotEffect, cent->lerpOrigin, forward);
}