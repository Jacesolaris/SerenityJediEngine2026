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
#include "wp_saber.h"
#include "w_local.h"
extern qboolean PM_RunningAnim(int anim);
extern qboolean PM_WalkingAnim(int anim);

//-------------------
//	Wookiee Bowcaster
//-------------------

extern qboolean WalkCheck(const gentity_t* self);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
//---------------------------------------------------------
static void WP_BowcasterMainFire(gentity_t* ent)
//---------------------------------------------------------
{
    // SAFETY: ent or ent->client may be NULL in edge cases
    if (ent == NULL || ent->client == NULL)
    {
        Com_Printf(S_COLOR_YELLOW "WP_BowcasterMainFire: NULL ent or ent->client\n");
        return;
    }

    int damage = weaponData[WP_BOWCASTER].damage;
    vec3_t angs, start;

    // Starting point of the shot
    VectorCopy(muzzle, start);
    WP_TraceSetStart(ent, start);

    // NPC damage scaling
    if (ent->s.number != 0)
    {
        if (g_spskill->integer == 0)
        {
            damage = BOWCASTER_NPC_DAMAGE_EASY;
        }
        else if (g_spskill->integer == 1)
        {
            damage = BOWCASTER_NPC_DAMAGE_NORMAL;
        }
        else
        {
            damage = BOWCASTER_NPC_DAMAGE_HARD;
        }
    }

    // Charge level
    int count = (level.time - ent->client->ps.weaponChargeTime) / BOWCASTER_CHARGE_UNIT;

    if (count < 1)
    {
        count = 1;
    }
    else if (count > 5)
    {
        count = 5;
    }

    // Must be odd
    if ((count & 1) == 0)
    {
        count--;
    }

    WP_MissileTargetHint(ent, start, forward_vec);

    // Fire multiple bolts based on charge level
    for (int i = 0; i < count; i++)
    {
        vec3_t dir;

        // Random velocity variation
        const float vel =
            BOWCASTER_VELOCITY *
            (Q_flrand(-1.0f, 1.0f) * BOWCASTER_VEL_RANGE + 1.0f);

        vectoangles(forward_vec, angs);

        // AIM / SPREAD LOGIC
        if (ent->client->NPC_class == CLASS_VEHICLE)
        {
            // Vehicles: no inherent aim screw up
        }
        else if (NPC_IsNotHavingEnoughForceSight(ent) == qtrue)
        {
            const qboolean is_player_or_controlled =
                ((ent->s.number < MAX_CLIENTS) || (G_ControlledByPlayer(ent) == qtrue))
                ? qtrue : qfalse;

            if (is_player_or_controlled == qtrue)
            {
                if (PM_CrouchAnim(ent->client->ps.legsAnim) == qtrue)
                {
                    angs[PITCH] += Q_flrand(-0.0f, 0.0f);
                    angs[YAW] += Q_flrand(-0.0f, 0.0f);
                }
                else
                {
                    if (PM_RunningAnim(ent->client->ps.legsAnim) == qtrue ||
                        ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
                    {
                        angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
                        angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
                    }
                    else if (PM_WalkingAnim(ent->client->ps.legsAnim) == qtrue ||
                        ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF)
                    {
                        angs[PITCH] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
                        angs[YAW] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
                    }
                    else
                    {
                        angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
                        angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
                    }
                }
            }
            else
            {
                // NPC spread
                angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
                angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
            }
        }

        AngleVectors(angs, dir, NULL, NULL);

        gentity_t* missile = CreateMissile(start, dir, vel, 10000, ent);

        if (missile == NULL)
        {
            Com_Printf(S_COLOR_YELLOW "WP_BowcasterMainFire: CreateMissile returned NULL\n");
            return;
        }

        missile->classname = "bowcaster_proj";
        missile->s.weapon = WP_BOWCASTER;

        VectorSet(missile->maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE);
        VectorScale(missile->maxs, -1, missile->mins);

        missile->damage = damage;
        missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
        missile->methodOfDeath = MOD_BOWCASTER;
        missile->clipmask = MASK_SHOT;

        missile->splashDamage = weaponData[WP_BOWCASTER].splashDamage;
        missile->splashRadius = weaponData[WP_BOWCASTER].splashRadius;

        missile->bounceCount = 0;

        ent->client->sess.missionStats.shotsFired++;
    }
}


//---------------------------------------------------------
static void WP_BowcasterAltFire(gentity_t* ent)
//---------------------------------------------------------
{
	vec3_t start;
	int damage = weaponData[WP_BOWCASTER].altDamage;

	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, forward_vec);

	gentity_t* missile = CreateMissile(start, forward_vec, BOWCASTER_VELOCITY, 10000, ent, qtrue);

	missile->classname = "bowcaster_alt_proj";
	missile->s.weapon = WP_BOWCASTER;

	// Do the damages
	if (ent->s.number != 0)
	{
		if (g_spskill->integer == 0)
		{
			damage = BOWCASTER_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = BOWCASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BOWCASTER_NPC_DAMAGE_HARD;
		}
	}

	if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_HALF)
	{
		NPC_SetAnim(ent, SETANIM_BOTH, BOTH_H1_S1_TR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}

	VectorSet(missile->maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE);
	VectorScale(missile->maxs, -1, missile->mins);

	missile->s.eFlags |= EF_BOUNCE;
	missile->bounceCount = 3;

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->methodOfDeath = MOD_BOWCASTER_ALT;
	missile->clipmask = MASK_SHOT;
	missile->splashDamage = weaponData[WP_BOWCASTER].altSplashDamage;
	missile->splashRadius = weaponData[WP_BOWCASTER].altSplashRadius;
}

//---------------------------------------------------------
void WP_FireBowcaster(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	if (alt_fire)
	{
		WP_BowcasterAltFire(ent);
	}
	else
	{
		WP_BowcasterMainFire(ent);
	}
}