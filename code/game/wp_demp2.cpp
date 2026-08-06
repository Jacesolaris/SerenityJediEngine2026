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

/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///
///																																///
///																																///
///													SERENITY JEDI ENGINE														///
///										          LIGHTSABER COMBAT SYSTEM													    ///
///																																///
///						      System designed by Serenity and modded by JaceSolaris. (c) 2026 SJE   		                    ///
///								    https://www.moddb.com/mods/serenityjediengine-20											///
///																																///
/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///

#include "g_local.h"
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"

//-------------------
//	DEMP2
//-------------------
extern void player_Decloak(gentity_t* self);
//---------------------------------------------------------
static void WP_DEMP2_MainFire(gentity_t* ent)
//---------------------------------------------------------
{
	vec3_t start;
	int damage = weaponData[WP_DEMP2].damage;

	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, forward_vec);

	gentity_t* missile = CreateMissile(start, forward_vec, DEMP2_VELOCITY, 10000, ent);

	missile->classname = "demp2_proj";
	missile->s.weapon = WP_DEMP2;

	// Do the damages
	if (ent->s.number != 0)
	{
		if (g_spskill->integer == 0)
		{
			damage = DEMP2_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = DEMP2_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = DEMP2_NPC_DAMAGE_HARD;
		}
	}

	VectorSet(missile->maxs, DEMP2_SIZE, DEMP2_SIZE, DEMP2_SIZE);
	VectorScale(missile->maxs, -1, missile->mins);
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_DEMP2;
	missile->clipmask = MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

// NOTE: this is 100% for the demp2 alt-fire effect, so changes to the visual effect will affect game side demp2 code
//--------------------------------------------------
void DEMP2_AltRadiusDamage(gentity_t* ent)
{
    // SAFETY: ent may be NULL in edge cases
    if (ent == NULL)
    {
        Com_Printf(S_COLOR_YELLOW "DEMP2_AltRadiusDamage: NULL ent\n");
        return;
    }

    float frac = (level.time - ent->fx_time) / 1300.0f;

    // Allocate entity list on heap instead of stack (fixes C6262)
    gentity_t** entity_list = (gentity_t**)G_Alloc(sizeof(gentity_t*) * MAX_GENTITIES);
    if (entity_list == NULL)
    {
        Com_Printf(S_COLOR_YELLOW "DEMP2_AltRadiusDamage: Failed to allocate entity_list\n");
        return;
    }

    vec3_t mins = {0}, maxs = {0};
    vec3_t v, dir;

    frac *= frac * frac;

    const float radius = frac * 200.0f;

    for (int i = 0; i < 3; i++)
    {
        mins[i] = ent->currentOrigin[i] - radius;
        maxs[i] = ent->currentOrigin[i] + radius;
    }

    const int num_listed_entities = gi.EntitiesInBox(mins, maxs, entity_list, MAX_GENTITIES);

    for (int e = 0; e < num_listed_entities; e++)
    {
        gentity_t* gent = entity_list[e];

        if (gent == NULL || gent->takedamage == qfalse || gent->contents == 0)
        {
            continue;
        }

        // Distance from bounding box edge
        for (int i = 0; i < 3; i++)
        {
            if (ent->currentOrigin[i] < gent->absmin[i])
            {
                v[i] = gent->absmin[i] - ent->currentOrigin[i];
            }
            else if (ent->currentOrigin[i] > gent->absmax[i])
            {
                v[i] = ent->currentOrigin[i] - gent->absmax[i];
            }
            else
            {
                v[i] = 0.0f;
            }
        }

        // Ellipsoid vertical compression
        v[2] *= 0.5f;

        const float dist = VectorLength(v);

        if (dist >= radius)
        {
            continue;
        }

        if (dist < ent->radius)
        {
            continue;
        }

        VectorCopy(gent->currentOrigin, v);
        VectorSubtract(v, ent->currentOrigin, dir);

        dir[2] += 12.0f;

        G_Damage(
            gent,
            ent,
            ent->owner,
            dir,
            ent->currentOrigin,
            weaponData[WP_DEMP2].altDamage,
            DAMAGE_DEATH_KNOCKBACK,
            ent->splashMethodOfDeath
        );

        if (gent->takedamage == qtrue && gent->client != NULL)
        {
            gent->s.powerups |= (1 << PW_SHOCKED);
            gent->client->ps.powerups[PW_SHOCKED] = level.time + 2000;

            Saboteur_Decloak(gent, Q_irand(3000, 10000));

            if (gent->client->ps.powerups[PW_CLOAKED])
            {
                player_Decloak(gent);
                gent->client->cloakToggleTime = level.time + Q_irand(3000, 10000);
            }
        }
    }

    // Update shockwave radius
    ent->radius = radius;

    if (frac < 1.0f)
    {
        ent->nextthink = level.time + 50;
    }
}


//---------------------------------------------------------
void DEMP2_AltDetonate(gentity_t* ent)
//---------------------------------------------------------
{
	G_SetOrigin(ent, ent->currentOrigin);

	// start the effects, unfortunately, I wanted to do some custom things that I couldn't easily do with the fx system, so part of it uses an event and localEntities
	G_PlayEffect("demp2/altDetonate", ent->currentOrigin, ent->pos1);
	G_AddEvent(ent, EV_DEMP2_ALT_IMPACT, ent->count * 2);

	ent->fx_time = level.time;
	ent->radius = 0;
	ent->nextthink = level.time + 50;
	ent->e_ThinkFunc = thinkF_DEMP2_AltRadiusDamage;
	ent->s.eType = ET_GENERAL; // make us a missile no longer
}

//---------------------------------------------------------
static void WP_DEMP2_AltFire(gentity_t* ent)
//---------------------------------------------------------
{
	int damage = weaponData[WP_REPEATER].altDamage;
	vec3_t start;
	trace_t tr;

	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_HALF)
	{
		NPC_SetAnim(ent, SETANIM_BOTH, BOTH_H1_S1_TR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}

	int count = (level.time - ent->client->ps.weaponChargeTime) / DEMP2_CHARGE_UNIT;

	if (count < 1)
	{
		count = 1;
	}
	else if (count > 3)
	{
		count = 3;
	}

	damage *= 1 + count * (count - 1); // yields damage of 12,36,84...gives a higher bonus for longer charge

	// the shot can travel a whopping 4096 units in 1 second. Note that the shot will auto-detonate at 4096 units...we'll see if this looks cool or not
	WP_MissileTargetHint(ent, start, forward_vec);
	gentity_t* missile = CreateMissile(start, forward_vec, DEMP2_ALT_RANGE, 1000, ent, qtrue);

	// letting it know what the charge size is.
	missile->count = count;

	//	missile->speed = missile->nextthink;
	VectorCopy(tr.plane.normal, missile->pos1);

	missile->classname = "demp2_alt_proj";
	missile->s.weapon = WP_DEMP2;

	missile->e_ThinkFunc = thinkF_DEMP2_AltDetonate;

	missile->splashDamage = missile->damage = damage;
	missile->splashMethodOfDeath = missile->methodOfDeath = MOD_DEMP2_ALT;
	missile->splashRadius = weaponData[WP_DEMP2].altSplashRadius;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->clipmask = MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

//---------------------------------------------------------
void WP_FireDEMP2(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	if (alt_fire)
	{
		WP_DEMP2_AltFire(ent);
	}
	else
	{
		WP_DEMP2_MainFire(ent);
	}
}