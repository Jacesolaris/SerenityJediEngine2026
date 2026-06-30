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

#include "g_local.h"
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"

//---------------------------------------------------------
void WP_FireTurboLaserMissile(gentity_t* ent, vec3_t start, vec3_t dir)
//---------------------------------------------------------
{
	const int velocity = ent->mass; //FIXME: externalize

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, qfalse);

	//use a custom shot effect
	//missile->s.otherentityNum2 = G_EffectIndex( "turret/turb_shot" );
	//use a custom impact effect
	//missile->s.emplacedOwner = G_EffectIndex( "turret/turb_impact" );

	missile->classname = "turbo_proj";
	missile->s.weapon = WP_TIE_FIGHTER;

	missile->damage = ent->damage; //FIXME: externalize
	missile->splashDamage = ent->splashDamage; //FIXME: externalize
	missile->splashRadius = ent->splashRadius; //FIXME: externalize
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->methodOfDeath = MOD_EMPLACED; //MOD_TURBLAST; //count as a heavy weap
	missile->splashMethodOfDeath = MOD_EMPLACED; //MOD_TURBLAST;// ?SPLASH;
	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//set veh as cgame side owner for purpose of fx overrides
	//missile->s.owner = ent->s.number;

	//don't let them last forever
	missile->e_ThinkFunc = thinkF_G_FreeEntity;
	missile->nextthink = level.time + 10000; //at 20000 speed, that should be more than enough
}

// Emplaced Gun
//---------------------------------------------------------
void WP_EmplacedFire(gentity_t* ent)
//---------------------------------------------------------
{
    // SAFETY: ent or ent->client may be NULL in edge cases
    if (ent == NULL)
    {
        Com_Printf(S_COLOR_YELLOW "WP_EmplacedFire: NULL ent\n");
        return;
    }

    const float damage = weaponData[WP_EMPLACED_GUN].damage *
        ((ent->NPC != NULL) ? 0.1f : 1.0f);

    const float vel = EMPLACED_VEL *
        ((ent->NPC != NULL) ? 0.4f : 1.0f);

    WP_MissileTargetHint(ent, muzzle, forward_vec);

    gentity_t* missile = CreateMissile(muzzle, forward_vec, vel, 10000, ent);

    if (missile == NULL)
    {
        Com_Printf(S_COLOR_YELLOW "WP_EmplacedFire: CreateMissile returned NULL\n");
        return;
    }

    missile->classname = "emplaced_proj";
    missile->s.weapon = WP_EMPLACED_GUN;

    missile->damage = damage;
    missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_HEAVY_WEAP_CLASS;
    missile->methodOfDeath = MOD_EMPLACED;
    missile->clipmask = MASK_SHOT;

    // Determine real owner (avoid hitting the gun object)
    const qboolean has_client =
        ((ent->client != NULL) ? qtrue : qfalse);

    if (has_client == qtrue &&
        !(ent->client->ps.eFlags & EF_LOCKED_TO_WEAPON))
    {
        missile->owner = ent;
    }
    else
    {
        missile->owner = ent->owner;
    }

    // SAFETY: missile->owner may be NULL
    if (missile->owner != NULL &&
        missile->owner->e_UseFunc == useF_eweb_use)
    {
        missile->alt_fire = qtrue;
    }

    VectorSet(missile->maxs, EMPLACED_SIZE, EMPLACED_SIZE, EMPLACED_SIZE);
    VectorScale(missile->maxs, -1, missile->mins);

    // Alternate muzzle FX toggle
    ent->fxID = ~ent->fxID;
}
