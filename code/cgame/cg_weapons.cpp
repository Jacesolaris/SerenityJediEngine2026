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

#include "cg_headers.h"

#include "cg_media.h"
#include "FxScheduler.h"
#include "../game/wp_saber.h"
#include "../game/g_vehicles.h"

#include "../game/anims.h"

extern vmCvar_t cg_com_outcast;
extern vmCvar_t cg_weaponBob;
extern vmCvar_t cg_fallingBob;

extern vmCvar_t cg_gunMomentumDamp;
extern vmCvar_t cg_gunMomentumFall;
extern vmCvar_t cg_gunMomentumEnable;
extern vmCvar_t cg_gunMomentumInterval;
extern vmCvar_t cg_SpinningBarrels;
extern void G_StartNextItemEffect(gentity_t* ent, int me_flags = 0, int length = 1000, float time_scale = 0.0f,
	int spin_time = 0);

extern qboolean PM_ReloadAnim(int anim);
extern void FX_DEMP2_ProjectileThink(centity_t* cent, const weaponInfo_s* weapon);
extern void G_SoundOnEnt(const gentity_t* ent, soundChannel_t channel, const char* sound_path);
const char* CG_DisplayBoxedText(int iBoxX, int iBoxY, int iBoxWidth, int iBoxHeight,
	const char* psText, int iFontHandle, float fScale,
	const vec4_t v4Color);
extern int fire_deley_time();
extern vmCvar_t cg_com_rend2;

/*
==========================
CG_GrappleTrail
==========================
*/
void CG_GrappleTrail(centity_t* ent, const weaponInfo_t* wi)
{
	vec3_t origin;
	vec3_t forward, up;
	refEntity_t beam;

	const entityState_t* es = &ent->currentState;

	EvaluateTrajectory(&es->pos, cg.time, origin);
	ent->trailTime = cg.time;

	memset(&beam, 0, sizeof beam);

	//FIXME adjust for muzzle position
	VectorCopy(cg_entities[ent->currentState.otherentity_num].lerpOrigin, beam.origin);
	beam.origin[2] += 26;

	AngleVectors(cg_entities[ent->currentState.otherentity_num].lerpAngles, forward, nullptr, up);
	VectorMA(beam.origin, -6, up, beam.origin);
	VectorCopy(origin, beam.oldorigin);

	if (Distance(beam.origin, beam.oldorigin) < 64)
		return; // Don't draw if close

	beam.reType = RT_ELECTRICITY;
	beam.customShader = cgs.media.electricBodyShader;

	AxisClear(beam.axis);
	beam.shaderRGBA[0] = 0xff;
	beam.shaderRGBA[1] = 0xff;
	beam.shaderRGBA[2] = 0xff;
	beam.shaderRGBA[3] = 0xff;
	cgi_R_AddRefEntityToScene(&beam);
}

static void CG_StunTrail(centity_t* ent, const weaponInfo_t* wi)
{
	vec3_t origin;
	vec3_t forward, up;
	refEntity_t beam;

	const entityState_t* es = &ent->currentState;

	EvaluateTrajectory(&es->pos, cg.time, origin);
	ent->trailTime = cg.time;

	memset(&beam, 0, sizeof beam);

	//FIXME adjust for muzzle position
	VectorCopy(cg_entities[ent->currentState.otherentity_num].lerpOrigin, beam.origin);
	beam.origin[2] += 26;

	AngleVectors(cg_entities[ent->currentState.otherentity_num].lerpAngles, forward, nullptr, up);
	VectorMA(beam.origin, -6, up, beam.origin);
	VectorCopy(origin, beam.oldorigin);

	if (Distance(beam.origin, beam.oldorigin) < 64)
		return; // Don't draw if close

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.electricBodyShader;

	AxisClear(beam.axis);
	beam.shaderRGBA[0] = 0xff;
	beam.shaderRGBA[1] = 0xff;
	beam.shaderRGBA[2] = 0xff;
	beam.shaderRGBA[3] = 0xff;
	cgi_R_AddRefEntityToScene(&beam);
}

/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon(const int weapon_num)
{
	gitem_t* item, * ammo;
	char path[MAX_QPATH];
	vec3_t mins, maxs;
	int i;

	weaponInfo_t* weaponInfo = &cg_weapons[weapon_num];

	// error checking
	if (weapon_num == 0)
	{
		return;
	}

	if (weaponInfo->registered)
	{
		return;
	}

	// clear out the memory we use
	memset(weaponInfo, 0, sizeof * weaponInfo);
	weaponInfo->registered = qtrue;

	// find the weapon in the item list
	for (item = bg_itemlist + 1; item->classname; item++)
	{
		if (item->giType == IT_WEAPON && item->giTag == weapon_num)
		{
			weaponInfo->item = item;
			break;
		}
	}
	// if we couldn't find which weapon this is, give us an error
	if (!item->classname)
	{
		CG_Error("Couldn't find item for weapon %s\nNeed to update Items.dat!", weaponData[weapon_num].classname);
	}
	CG_RegisterItemVisuals(item - bg_itemlist);

	// set up in view weapon model
	weaponInfo->weaponModel = cgi_R_RegisterModel(weaponData[weapon_num].weaponMdl);
	{
		//in case the weaponmodel isn't _w, precache the _w.glm
		char weaponModel[64];

		Q_strncpyz(weaponModel, weaponData[weapon_num].weaponMdl, sizeof weaponModel);
		if (char* spot = strstr(weaponModel, ".md3"))
		{
			*spot = 0;
			spot = strstr(weaponModel, "_w");
			//i'm using the in view weapon array instead of scanning the item list, so put the _w back on
			if (!spot)
			{
				Q_strcat(weaponModel, sizeof weaponModel, "_w");
			}
			Q_strcat(weaponModel, sizeof weaponModel, ".glm"); //and change to ghoul2
		}
		gi.G2API_PrecacheGhoul2Model(weaponModel); // correct way is item->world_model
	}

	if (weaponInfo->weaponModel == 0)
	{
		CG_Error("Couldn't find weapon model %s for weapon %s\n", weaponData[weapon_num].weaponMdl,
			weaponData[weapon_num].classname);
		return;
	}

	// calc midpoint for rotation
	cgi_R_ModelBounds(weaponInfo->weaponModel, mins, maxs);
	for (i = 0; i < 3; i++)
	{
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * (maxs[i] - mins[i]);
	}

	// setup the shader we will use for the icon
	if (weaponData[weapon_num].weaponIcon[0])
	{
		weaponInfo->weaponIcon = cgi_R_RegisterShaderNoMip(weaponData[weapon_num].weaponIcon);
		weaponInfo->weaponIconNoAmmo = cgi_R_RegisterShaderNoMip(va("%s_na", weaponData[weapon_num].weaponIcon));
	}

	for (ammo = bg_itemlist + 1; ammo->classname; ammo++)
	{
		if (ammo->giType == IT_AMMO && ammo->giTag == weaponData[weapon_num].ammoIndex)
		{
			break;
		}
	}

	if (ammo->classname && ammo->world_model)
	{
		weaponInfo->ammoModel = cgi_R_RegisterModel(ammo->world_model);
	}

	for (i = 0; i < weaponData[weapon_num].numBarrels; i++)
	{
		Q_strncpyz(path, weaponData[weapon_num].weaponMdl, sizeof path);
		COM_StripExtension(path, path, sizeof path);
		if (i)
		{
			Q_strcat(path, sizeof path, va("_barrel%d.md3", i + 1));
		}
		else
			Q_strcat(path, sizeof path, "_barrel.md3");
		weaponInfo->barrelModel[i] = cgi_R_RegisterModel(path);
	}

	// set up the world model for the weapon
	weaponInfo->weaponWorldModel = cgi_R_RegisterModel(item->world_model);
	if (!weaponInfo->weaponWorldModel)
	{
		weaponInfo->weaponWorldModel = weaponInfo->weaponModel;
	}

	// set up the hand that holds the in view weapon - assuming we have one
	Q_strncpyz(path, weaponData[weapon_num].weaponMdl, sizeof path);
	COM_StripExtension(path, path, sizeof path);
	Q_strcat(path, sizeof path, "_hand.md3");
	weaponInfo->handsModel = cgi_R_RegisterModel(path);

	if (!weaponInfo->handsModel)
	{
		weaponInfo->handsModel = cgi_R_RegisterModel("models/weapons2/briar_pistol/briar_pistol_hand.md3");
	}

	// register the sounds for the weapon
	if (weaponData[weapon_num].firingSnd[0])
	{
		weaponInfo->firingSound = cgi_S_RegisterSound(weaponData[weapon_num].firingSnd);
	}
	if (weaponData[weapon_num].altFiringSnd[0])
	{
		weaponInfo->altFiringSound = cgi_S_RegisterSound(weaponData[weapon_num].altFiringSnd);
	}
	if (weaponData[weapon_num].stopSnd[0])
	{
		weaponInfo->stopSound = cgi_S_RegisterSound(weaponData[weapon_num].stopSnd);
	}
	if (weaponData[weapon_num].chargeSnd[0])
	{
		weaponInfo->chargeSound = cgi_S_RegisterSound(weaponData[weapon_num].chargeSnd);
	}
	if (weaponData[weapon_num].altChargeSnd[0])
	{
		weaponInfo->altChargeSound = cgi_S_RegisterSound(weaponData[weapon_num].altChargeSnd);
	}
	if (weaponData[weapon_num].selectSnd[0])
	{
		weaponInfo->selectSound = cgi_S_RegisterSound(weaponData[weapon_num].selectSnd);
	}

	// give us missile models if we should
	if (weaponData[weapon_num].missileMdl[0])
	{
		weaponInfo->missileModel = cgi_R_RegisterModel(weaponData[weapon_num].missileMdl);
	}
	if (weaponData[weapon_num].alt_missileMdl[0])
	{
		weaponInfo->alt_missileModel = cgi_R_RegisterModel(weaponData[weapon_num].alt_missileMdl);
	}
	if (weaponData[weapon_num].missileSound[0])
	{
		weaponInfo->missileSound = cgi_S_RegisterSound(weaponData[weapon_num].missileSound);
	}
	if (weaponData[weapon_num].alt_missileSound[0])
	{
		weaponInfo->alt_missileSound = cgi_S_RegisterSound(weaponData[weapon_num].alt_missileSound);
	}
	if (weaponData[weapon_num].missilehit_sound[0])
	{
		weaponInfo->missilehit_sound = cgi_S_RegisterSound(weaponData[weapon_num].missilehit_sound);
	}
	if (weaponData[weapon_num].altmissilehit_sound[0])
	{
		weaponInfo->altmissilehit_sound = cgi_S_RegisterSound(weaponData[weapon_num].altmissilehit_sound);
	}
	if (weaponData[weapon_num].mMuzzleEffect[0])
	{
		weaponData[weapon_num].mMuzzleEffectID = theFxScheduler.RegisterEffect(weaponData[weapon_num].mMuzzleEffect);
	}
	if (weaponData[weapon_num].mAltMuzzleEffect[0])
	{
		weaponData[weapon_num].mAltMuzzleEffectID = theFxScheduler.RegisterEffect(weaponData[weapon_num].mAltMuzzleEffect);
	}
	if (weaponData[weapon_num].mOverloadMuzzleEffect[0])
	{
		weaponData[weapon_num].mOverloadMuzzleEffectID = theFxScheduler.RegisterEffect(weaponData[weapon_num].mOverloadMuzzleEffect);
	}

	//fixme: don't really need to copy these, should just use directly
	// give ourselves the functions if we can
	if (weaponData[weapon_num].func)
	{
		weaponInfo->missileTrailFunc = static_cast<void(*)(centity_s*, const weaponInfo_s*)>(weaponData[weapon_num].
			func);
	}
	if (weaponData[weapon_num].altfunc)
	{
		weaponInfo->alt_missileTrailFunc = static_cast<void(*)(centity_s*, const weaponInfo_s*)>(weaponData[weapon_num].
			altfunc);
	}

	switch (weapon_num) //extra client only stuff
	{
	case WP_SABER:
		//saber/force FX
		theFxScheduler.RegisterEffect("sparks/spark_nosnd");
		theFxScheduler.RegisterEffect("sparks/blood_sparks2");
		theFxScheduler.RegisterEffect("sparks/spark");
		theFxScheduler.RegisterEffect("force/force_touch");
		theFxScheduler.RegisterEffect("saber/saber_block");
		theFxScheduler.RegisterEffect("saber/saber_cut");
		theFxScheduler.RegisterEffect("saber/saber_cut_droid");
		theFxScheduler.RegisterEffect("saber/saber_touch_droid");
		theFxScheduler.RegisterEffect("saber/saber_friction");
		theFxScheduler.RegisterEffect("saber/saber_goodparry");
		theFxScheduler.RegisterEffect("saber/saber_badparry");
		theFxScheduler.RegisterEffect("saber/saber_bodyhit");
		theFxScheduler.RegisterEffect("saber/saber_deflect");
		theFxScheduler.RegisterEffect("saber/saber_Lightninghit");
		theFxScheduler.RegisterEffect("saber/saber_lock");
		theFxScheduler.RegisterEffect("saber/limb_bolton");
		theFxScheduler.RegisterEffect("saber/fizz");
		theFxScheduler.RegisterEffect("saber/boil");

		cgs.effects.forceHeal = theFxScheduler.RegisterEffect("force/heal");
		cgs.effects.forceInvincibility = theFxScheduler.RegisterEffect("force/invin");
		cgs.effects.forceConfusion = theFxScheduler.RegisterEffect("force/confusion");
		cgs.effects.forceLightning = theFxScheduler.RegisterEffect("force/lightning");
		cgs.effects.forceLightningWide = theFxScheduler.RegisterEffect("force/lightningwide");
		cgs.effects.redlightningwide = theFxScheduler.RegisterEffect("force/redlightningwide");
		cgs.effects.redlightning = theFxScheduler.RegisterEffect("force/redlightning");
		cgs.effects.greenlightningwide = theFxScheduler.RegisterEffect("force/greenlightningwide");
		cgs.effects.greenlightning = theFxScheduler.RegisterEffect("force/greenlightning");
		cgs.effects.purplelightningwide = theFxScheduler.RegisterEffect("force/purplelightningwide");
		cgs.effects.purplelightning = theFxScheduler.RegisterEffect("force/purplelightning");

		cgs.effects.yellowlightningwide = theFxScheduler.RegisterEffect("force/yellowlightningwide");
		cgs.effects.yellowlightning = theFxScheduler.RegisterEffect("force/yellowlightning");
		//new Jedi Academy force power effects
		cgs.effects.forceDrain = theFxScheduler.RegisterEffect("mp/drain");
		cgs.effects.forceDrainWide = theFxScheduler.RegisterEffect("mp/drainwide");
		cgs.effects.forceDrained = theFxScheduler.RegisterEffect("mp/drainhit");

		cgs.effects.mDisruptorDeathSmoke = theFxScheduler.RegisterEffect("disruptor/death_smoke");

		cgs.effects.destructionProjectile = theFxScheduler.RegisterEffect("destruction/shot");
		cgs.effects.destructionHit = theFxScheduler.RegisterEffect("destruction/explosion");
		cgs.media.destructionSound = cgi_S_RegisterSound("sound/weapons/concussion/missileloop.wav");

		cgs.effects.blastProjectile = theFxScheduler.RegisterEffect("repeater/alt_projectile");
		cgs.effects.blastHit = theFxScheduler.RegisterEffect("force/blast");
		cgs.media.blastSound = cgi_S_RegisterSound("sound/weapons/force/absorbloop.wav");

		cgs.effects.strikeProjectile = theFxScheduler.RegisterEffect("env/huge_lightning");
		cgs.effects.strikeHit = theFxScheduler.RegisterEffect("env/small_fire_blue");
		cgs.media.strikeSound = cgi_S_RegisterSound("sound/weapons/explosions/explode5.wav");

		//saber sounds
		cgi_S_RegisterSound("sound/weapons/saber/saberonquick.mp3");
		cgi_S_RegisterSound("sound/weapons/saber/saberspinoff.mp3");
		cgi_S_RegisterSound("sound/weapons/saber/saberoffquick.mp3");
		cgi_S_RegisterSound("sound/weapons/saber/saberoff.mp3");
		cgi_S_RegisterSound("sound/weapons/saber/saberoff2.mp3");
		cgi_S_RegisterSound("sound/weapons/saber/saberoff3.mp3");
		cgi_S_RegisterSound("sound/weapons/saber/saberoff6.mp3");

		cgi_S_RegisterSound("sound/weapons/saber/lightning1.mp3");
		cgi_S_RegisterSound("sound/weapons/saber/lightning2.mp3");
		cgi_S_RegisterSound("sound/weapons/saber/lightning3.mp3");

		cgi_S_RegisterSound("sound/weapons/saber/saberlockstart.mp3");
		cgi_S_RegisterSound("sound/weapons/saber/saberlockend.mp3");

		for (i = 1; i < 5; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saber_locking_start%d.mp3", i));
		}

		for (i = 1; i < 5; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saber_locking_end%d.mp3wav", i));
		}

		for (i = 1; i < 6; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saberbounce%d.wav", i));
		}
		for (i = 1; i < 15; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saberhit%d.mp3", i));
		}
		for (i = 1; i < 4; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saberhitwall%d.wav", i));
		}
		for (i = 1; i < 18; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saberkill%d.mp3", i));
		}
		for (i = 1; i < 12; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saberstabdown%d.mp3", i));
		}
		for (i = 1; i < 18; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saberstrikewall%d.mp3", i));
		}
		for (i = 1; i < 90; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saberblock%d.mp3", i));
		}
		for (i = 1; i < 10; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saberlock%d.mp3", i));
		}

		for (i = 1; i < 8; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saberhum%d.wav", i));
		}

		for (i = 1; i < 11; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saberhup%d.wav", i));
		}
		for (i = 1; i < 4; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saberspin%d.wav", i));
		}
		for (i = 1; i < 4; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saberknock%d.mp3", i));
		}
		cgi_S_RegisterSound("sound/weapons/saber/saber_catch.mp3");
		cgi_S_RegisterSound("sound/weapons/saber/sabercatch.mp3");
		for (i = 1; i < 4; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/bounce%d.wav", i));
		}
		for (i = 1; i < 4; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saber_goodparry%d.mp3", i));
		}
		for (i = 1; i < 4; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/saber_perfectblock%d.mp3", i));
		}
		cgi_S_RegisterSound("sound/weapons/saber/hitwater.wav");
		cgi_S_RegisterSound("sound/weapons/saber/boiling.wav");
		for (i = 1; i < 4; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/saber/rainfizz%d.wav", i));
		}
		cgi_S_RegisterSound("sound/movers/objects/saber_slam");

		//force sounds
		cgi_S_RegisterSound("sound/weapons/force/heal.mp3");
		cgi_S_RegisterSound("sound/weapons/force/speed.mp3");
		cgi_S_RegisterSound("sound/weapons/force/dash.mp3");
		cgi_S_RegisterSound("sound/weapons/force/speedloop.mp3");
		cgi_S_RegisterSound("sound/weapons/force/destruction.mp3");
		for (i = 1; i < 5; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/force/heal%d.mp3", i));
			cgi_S_RegisterSound(va("sound/weapons/force/heal%d_m.mp3", i));
			cgi_S_RegisterSound(va("sound/weapons/force/heal%d_f.mp3", i));
		}
		cgi_S_RegisterSound("sound/weapons/force/lightning.mp3");
		cgi_S_RegisterSound("sound/weapons/force/lightning2.wav");
		cgi_S_RegisterSound("sound/weapons/force/lightning3.mp3");
		cgi_S_RegisterSound("sound/weapons/force/strike.wav");
		for (i = 1; i < 4; i++)
		{
			cgi_S_RegisterSound(va("sound/weapons/force/lightninghit%d.wav", i));
		}
		cgi_S_RegisterSound("sound/weapons/force/push.mp3");
		cgi_S_RegisterSound("sound/weapons/force/pushed.mp3");
		cgi_S_RegisterSound("sound/weapons/force/pushlow.mp3");
		cgi_S_RegisterSound("sound/weapons/force/pushhard.mp3");
		cgi_S_RegisterSound("sound/weapons/force/pushyoda.mp3");
		cgi_S_RegisterSound("sound/weapons/force/repulsepush.wav");
		cgi_S_RegisterSound("sound/weapons/force/pull.wav");
		cgi_S_RegisterSound("sound/weapons/force/jump.mp3");
		cgi_S_RegisterSound("sound/weapons/force/jumpsmall.mp3");
		cgi_S_RegisterSound("sound/weapons/force/jumpbuild.mp3");
		cgi_S_RegisterSound("sound/weapons/force/grab.mp3");
		cgi_S_RegisterSound("sound/weapons/force/grip.mp3");
		cgi_S_RegisterSound("sound/weapons/force/griploop.wav");
		//new Jedi Academy force sounds
		cgi_S_RegisterSound("sound/weapons/force/absorb.mp3");
		cgi_S_RegisterSound("sound/weapons/force/absorbhit.mp3");
		cgi_S_RegisterSound("sound/weapons/force/absorbloop.mp3");
		cgi_S_RegisterSound("sound/weapons/force/protect.mp3");
		cgi_S_RegisterSound("sound/weapons/force/protecthit.mp3");
		cgi_S_RegisterSound("sound/weapons/force/protectloop.mp3");
		cgi_S_RegisterSound("sound/weapons/force/rage.mp3");
		cgi_S_RegisterSound("sound/weapons/force/ragehit.mp3");
		cgi_S_RegisterSound("sound/weapons/force/rageloop.mp3");
		cgi_S_RegisterSound("sound/weapons/force/see.mp3");
		cgi_S_RegisterSound("sound/weapons/force/seeloop.mp3");
		cgi_S_RegisterSound("sound/weapons/force/drain.mp3");
		cgi_S_RegisterSound("sound/weapons/force/drained.mp3");
		cgi_S_RegisterSound("sound/weapons/force/force_stasis.mp3");
		cgi_S_RegisterSound("sound/weapons/force/repulsecharge.mp3");
		cgi_S_RegisterSound("sound/weapons/force/repulseloop.wav");

		cgi_S_RegisterSound("sound/weapons/force/fear.mp3");
		cgi_S_RegisterSound("sound/weapons/force/fearfail.mp3");
		//force graphics
		cgs.media.playerShieldDamage = cgi_R_RegisterShader("gfx/misc/personalshield");
		cgs.media.forceShell = cgi_R_RegisterShader("powerups/forceshell");
		cgs.media.sightShell = cgi_R_RegisterShader("powerups/sightshell");
		cgi_R_RegisterShader("gfx/2d/jsense");
		cgi_R_RegisterShader("gfx/2d/dmgsense");
		cgi_R_RegisterShader("gfx/2d/cloaksense");
		cgi_R_RegisterShader("gfx/2d/firesense");
		cgi_R_RegisterShader("gfx/2d/stasissense");
		//force effects - FIXME: only if someone has these powers?
		theFxScheduler.RegisterEffect("force/rage2");
		theFxScheduler.RegisterEffect("force/heal2");
		theFxScheduler.RegisterEffect("force/drain_hand");

		//saber graphics
		cgs.media.saberBlurShader = cgi_R_RegisterShader("gfx/effects/sabers/saberBlur");
		cgs.media.swordTrailShader = cgi_R_RegisterShader("gfx/effects/sabers/swordTrail");
		cgs.media.yellowDroppedSaberShader = cgi_R_RegisterShader("gfx/effects/yellow_glow");
		cgi_R_RegisterShader("gfx/effects/saberDamageGlow");
		cgi_R_RegisterShader("gfx/effects/solidWhite_cull");
		cgi_R_RegisterShader("gfx/effects/forcePush");
		cgi_R_RegisterShader("gfx/effects/saberFlare");
		cgs.media.redSaberGlowShader = cgi_R_RegisterShader("gfx/effects/sabers/red_glow");
		cgs.media.redSaberCoreShader = cgi_R_RegisterShader("gfx/effects/sabers/red_line");
		cgs.media.orangeSaberGlowShader = cgi_R_RegisterShader("gfx/effects/sabers/orange_glow");
		cgs.media.orangeSaberCoreShader = cgi_R_RegisterShader("gfx/effects/sabers/orange_line");
		cgs.media.yellowSaberGlowShader = cgi_R_RegisterShader("gfx/effects/sabers/yellow_glow");
		cgs.media.yellowSaberCoreShader = cgi_R_RegisterShader("gfx/effects/sabers/yellow_line");
		cgs.media.greenSaberGlowShader = cgi_R_RegisterShader("gfx/effects/sabers/green_glow");
		cgs.media.greenSaberCoreShader = cgi_R_RegisterShader("gfx/effects/sabers/green_line");
		cgs.media.blueSaberGlowShader = cgi_R_RegisterShader("gfx/effects/sabers/blue_glow");
		cgs.media.blueSaberCoreShader = cgi_R_RegisterShader("gfx/effects/sabers/blue_line");
		cgs.media.purpleSaberGlowShader = cgi_R_RegisterShader("gfx/effects/sabers/purple_glow");
		cgs.media.purpleSaberCoreShader = cgi_R_RegisterShader("gfx/effects/sabers/purple_line");
		cgs.media.sfxSaberTrailShader = cgi_R_RegisterShader("gfx/effects/sabers/saber_trail");
		cgs.media.sfxSaberBladeShader = cgi_R_RegisterShader("gfx/effects/sabers/saber_blade");
		cgs.media.sfxSaberBlade2Shader = cgi_R_RegisterShader("gfx/effects/sabers/saber_blade_rgb");
		cgs.media.sfxSaberEndShader = cgi_R_RegisterShader("gfx/effects/sabers/saber_end");
		cgs.media.sfxSaberEnd2Shader = cgi_R_RegisterShader("gfx/effects/sabers/saber_end_rgb");
		cgs.media.rgbSaberGlowShader = cgi_R_RegisterShader("gfx/effects/sabers/rgb_glow");
		cgs.media.rgbSaberCoreShader = cgi_R_RegisterShader("gfx/effects/sabers/rgb_line");
		cgs.media.rgbSaberCore2Shader = cgi_R_RegisterShader("gfx/effects/sabers/rgb_core");
		cgs.media.blackSaberGlowShader = cgi_R_RegisterShader("gfx/effects/sabers/black_glow");
		cgs.media.blackSaberCoreShader = cgi_R_RegisterShader("gfx/effects/sabers/black_blade");
		cgs.media.blackSaberTrail = cgi_R_RegisterShader("gfx/effects/sabers/blacksaberBlur");
		cgs.media.blackSaberBlurShader = cgi_R_RegisterShader("gfx/effects/sabers/blackSaberBlur");
		cgs.media.redIgniteFlare = cgi_R_RegisterShader("gfx/effects/sabers/red_ignite_flare");
		cgs.media.greenIgniteFlare = cgi_R_RegisterShader("gfx/effects/sabers/green_ignite_flare");
		cgs.media.purpleIgniteFlare = cgi_R_RegisterShader("gfx/effects/sabers/purple_ignite_flare");
		cgs.media.blueIgniteFlare = cgi_R_RegisterShader("gfx/effects/sabers/blue_ignite_flare");
		cgs.media.orangeIgniteFlare = cgi_R_RegisterShader("gfx/effects/sabers/orange_ignite_flare");
		cgs.media.yellowIgniteFlare = cgi_R_RegisterShader("gfx/effects/sabers/yellow_ignite_flare");
		cgs.media.limeSaberGlowShader = cgi_R_RegisterShader("gfx/effects/sabers/lime_glow");
		cgs.media.limeSaberCoreShader = cgi_R_RegisterShader("gfx/effects/sabers/lime_line");
		//TFA Sabers
		cgs.media.rgbTFASaberCoreShader = cgi_R_RegisterShader("gfx/effects/TFASabers/blade_TFA");
		cgs.media.unstableBlurShader = cgi_R_RegisterShader("gfx/effects/TFASabers/trail_unstable");
		cgs.media.unstableRedSaberCoreShader = cgi_R_RegisterShader("gfx/effects/sabers/unstable_red_line");

		//Original Trilogy Sabers
		cgs.media.otSaberCoreShader = cgi_R_RegisterShader("gfx/effects/OTsabers/ot_saberCore");
		cgs.media.redOTGlowShader = cgi_R_RegisterShader("gfx/effects/OTsabers/ot_redGlow");
		cgs.media.orangeOTGlowShader = cgi_R_RegisterShader("gfx/effects/OTsabers/ot_orangeGlow");
		cgs.media.yellowOTGlowShader = cgi_R_RegisterShader("gfx/effects/OTsabers/ot_yellowGlow");
		cgs.media.greenOTGlowShader = cgi_R_RegisterShader("gfx/effects/OTsabers/ot_greenGlow");
		cgs.media.blueOTGlowShader = cgi_R_RegisterShader("gfx/effects/OTsabers/ot_blueGlow");
		cgs.media.purpleOTGlowShader = cgi_R_RegisterShader("gfx/effects/OTsabers/ot_purpleGlow");

		//Episode I Sabers
		cgs.media.ep1SaberCoreShader = cgi_R_RegisterShader("gfx/effects/Ep1Sabers/saber_core");
		cgs.media.redEp1GlowShader = cgi_R_RegisterShader("gfx/effects/Ep1Sabers/red_glowa");
		cgs.media.orangeEp1GlowShader = cgi_R_RegisterShader("gfx/effects/Ep1Sabers/orange_glowa");
		cgs.media.yellowEp1GlowShader = cgi_R_RegisterShader("gfx/effects/Ep1Sabers/yellow_glowa");
		cgs.media.greenEp1GlowShader = cgi_R_RegisterShader("gfx/effects/Ep1Sabers/green_glowa");
		cgs.media.blueEp1GlowShader = cgi_R_RegisterShader("gfx/effects/Ep1Sabers/blue_glowa");
		cgs.media.purpleEp1GlowShader = cgi_R_RegisterShader("gfx/effects/Ep1Sabers/purple_glowa");
		//Episode II Sabers
		cgs.media.ep2SaberCoreShader = cgi_R_RegisterShader("gfx/effects/Ep2Sabers/saber_core");
		cgs.media.whiteIgniteFlare = cgi_R_RegisterShader("gfx/effects/Ep2Sabers/white_ignite_flare");
		cgs.media.blackIgniteFlare = cgi_R_RegisterShader("gfx/effects/Ep3Sabers/black_ignite_flare");
		cgs.media.redEp2GlowShader = cgi_R_RegisterShader("gfx/effects/Ep2Sabers/red_glowa");
		cgs.media.orangeEp2GlowShader = cgi_R_RegisterShader("gfx/effects/Ep2Sabers/orange_glowa");
		cgs.media.yellowEp2GlowShader = cgi_R_RegisterShader("gfx/effects/Ep2Sabers/yellow_glowa");
		cgs.media.greenEp2GlowShader = cgi_R_RegisterShader("gfx/effects/Ep2Sabers/green_glowa");
		cgs.media.blueEp2GlowShader = cgi_R_RegisterShader("gfx/effects/Ep2Sabers/blue_glowa");
		cgs.media.purpleEp2GlowShader = cgi_R_RegisterShader("gfx/effects/Ep2Sabers/purple_glowa");
		//Episode III Sabers
		cgs.media.ep3SaberCoreShader = cgi_R_RegisterShader("gfx/effects/Ep3Sabers/saber_core");
		cgs.media.whiteIgniteFlare02 = cgi_R_RegisterShader("gfx/effects/Ep3Sabers/white_ignite_flare");
		cgs.media.blackIgniteFlare02 = cgi_R_RegisterShader("gfx/effects/Ep3Sabers/black_ignite_flare");
		cgs.media.redEp3GlowShader = cgi_R_RegisterShader("gfx/effects/Ep3Sabers/red_glowa");
		cgs.media.orangeEp3GlowShader = cgi_R_RegisterShader("gfx/effects/Ep3Sabers/orange_glowa");
		cgs.media.yellowEp3GlowShader = cgi_R_RegisterShader("gfx/effects/Ep3Sabers/yellow_glowa");
		cgs.media.greenEp3GlowShader = cgi_R_RegisterShader("gfx/effects/Ep3Sabers/green_glowa");
		cgs.media.blueEp3GlowShader = cgi_R_RegisterShader("gfx/effects/Ep3Sabers/blue_glowa");
		cgs.media.purpleEp3GlowShader = cgi_R_RegisterShader("gfx/effects/Ep3Sabers/purple_glowa");

		cgs.media.forceCoronaShader = cgi_R_RegisterShaderNoMip("gfx/hud/force_swirl");

		//new Jedi Academy force graphics
		cgs.media.drainShader = cgi_R_RegisterShader("gfx/misc/redLine");

		//for grip slamming into walls
		theFxScheduler.RegisterEffect("env/impact_dustonly");
		cgi_S_RegisterSound("sound/weapons/melee/punch1.mp3");
		cgi_S_RegisterSound("sound/weapons/melee/punch2.mp3");
		cgi_S_RegisterSound("sound/weapons/melee/punch3.mp3");
		cgi_S_RegisterSound("sound/weapons/melee/punch4.mp3");

		//For kicks with saber staff...
		theFxScheduler.RegisterEffect("melee/kick_impact");

		//Kothos beam
		cgi_R_RegisterShader("gfx/misc/dr1");
		break;

	case WP_SBD_PISTOL: //SBD WEAPON
		cgs.effects.briar_pistolShotEffect = theFxScheduler.RegisterEffect("briar_pistol/shot");
		cgs.effects.briar_pistolNPCShotEffect = theFxScheduler.RegisterEffect("briar_pistol/npcshot");
		cgs.effects.briar_pistolPowerupShotEffect = theFxScheduler.RegisterEffect("briar_pistol/crackleShot");
		cgs.effects.briar_pistolWallImpactEffect = theFxScheduler.RegisterEffect("briar_pistol/wall_impact");
		cgs.effects.briar_pistolWallImpactEffect2 = theFxScheduler.RegisterEffect("briar_pistol/wall_impact2");
		cgs.effects.briar_pistolWallImpactEffect3 = theFxScheduler.RegisterEffect("briar_pistol/wall_impact3");
		cgs.effects.briar_pistolFleshImpactEffect = theFxScheduler.RegisterEffect("briar_pistol/flesh_impact");
		cgs.effects.briar_pistolDroidImpactEffect = theFxScheduler.RegisterEffect("briar_pistol/droid_impact");
		// Note....these are temp shared effects
		theFxScheduler.RegisterEffect("briar_pistol/deflectblock");
		theFxScheduler.RegisterEffect("briar_pistol/deflect");
		theFxScheduler.RegisterEffect("blaster/smoke_bolton"); // note: this will be called game side
		break;
	case WP_BRYAR_PISTOL: //KYLE WEAPON
		cgs.effects.bryaroldShotEffect = theFxScheduler.RegisterEffect("bryar_old/shot");
		theFxScheduler.RegisterEffect("bryar_old/NPCshot");
		cgs.effects.bryaroldPowerupShotEffect = theFxScheduler.RegisterEffect("bryar_old/crackleShot");
		cgs.effects.bryaroldWallImpactEffect = theFxScheduler.RegisterEffect("bryar_old/wall_impact");
		cgs.effects.bryaroldWallImpactEffect2 = theFxScheduler.RegisterEffect("bryar_old/wall_impact2");
		cgs.effects.bryaroldWallImpactEffect3 = theFxScheduler.RegisterEffect("bryar_old/wall_impact3");
		cgs.effects.bryaroldFleshImpactEffect = theFxScheduler.RegisterEffect("bryar_old/flesh_impact");

		// Note....these are temp shared effects
		theFxScheduler.RegisterEffect("blaster/deflect");
		theFxScheduler.RegisterEffect("blaster/smoke_bolton"); // note: this will be called game side
		break;
	case WP_BLASTER_PISTOL: // enemy version
	case WP_JAWA:
		cgs.effects.bryarShotEffect = theFxScheduler.RegisterEffect("bryar/shot");
		theFxScheduler.RegisterEffect("bryar/NPCshot");
		cgs.effects.bryarPowerupShotEffect = theFxScheduler.RegisterEffect("bryar/crackleShot");
		cgs.effects.bryarWallImpactEffect = theFxScheduler.RegisterEffect("bryar/wall_impact");
		cgs.effects.bryarWallImpactEffect2 = theFxScheduler.RegisterEffect("bryar/wall_impact2");
		cgs.effects.bryarWallImpactEffect3 = theFxScheduler.RegisterEffect("bryar/wall_impact3");
		cgs.effects.bryarFleshImpactEffect = theFxScheduler.RegisterEffect("bryar/flesh_impact");

		// Note....these are temp shared effects
		theFxScheduler.RegisterEffect("blaster/deflect");
		theFxScheduler.RegisterEffect("blaster/smoke_bolton"); // note: this will be called game side
		break;

	case WP_BLASTER:
	case WP_DROIDEKA:
		cgs.effects.blasterShotEffect = theFxScheduler.RegisterEffect("blaster/shot");
		theFxScheduler.RegisterEffect("blaster/NPCshot");
		cgs.effects.DroidekaShotEffect = theFxScheduler.RegisterEffect("droideka/shot");
		theFxScheduler.RegisterEffect("droideka/shotnpc");
		cgs.effects.blasterWallImpactEffect = theFxScheduler.RegisterEffect("blaster/wall_impact");
		cgs.effects.blasterFleshImpactEffect = theFxScheduler.RegisterEffect("blaster/flesh_impact");
		cgs.effects.blasterDroidImpactEffect = theFxScheduler.RegisterEffect("blaster/droid_impact");
		theFxScheduler.RegisterEffect("blaster/deflect");
		theFxScheduler.RegisterEffect("blaster/deflect_passthrough");
		theFxScheduler.RegisterEffect("blaster/smoke_bolton"); // note: this will be called game side
		break;

	case WP_DISRUPTOR:
		theFxScheduler.RegisterEffect("disruptor/wall_impact");
		theFxScheduler.RegisterEffect("disruptor/flesh_impact");
		theFxScheduler.RegisterEffect("disruptor/alt_miss");
		theFxScheduler.RegisterEffect("disruptor/alt_hit");
		theFxScheduler.RegisterEffect("disruptor/line_cap");
		theFxScheduler.RegisterEffect("disruptor/death_smoke");

		cgi_R_RegisterShader("gfx/effects/redLine");
		cgi_R_RegisterShader("gfx/misc/whiteline2");
		cgi_R_RegisterShader("gfx/effects/smokeTrail");
		cgi_R_RegisterShader("gfx/effects/burn");

		cgi_R_RegisterShaderNoMip("gfx/2d/crop_charge");

		// zoom sounds
		cgi_S_RegisterSound("sound/weapons/disruptor/zoomstart.wav");
		cgi_S_RegisterSound("sound/weapons/disruptor/zoomend.wav");
		cgs.media.disruptorZoomLoop = cgi_S_RegisterSound("sound/weapons/disruptor/zoomloop.wav");

		// Disruptor gun zoom interface
		cgs.media.disruptorMask = cgi_R_RegisterShader("gfx/2d/cropCircle2");
		cgs.media.disruptorInsert = cgi_R_RegisterShader("gfx/2d/cropCircle");
		cgs.media.disruptorLight = cgi_R_RegisterShader("gfx/2d/cropCircleGlow");
		cgs.media.disruptorInsertTick = cgi_R_RegisterShader("gfx/2d/insertTick");
		break;

	case WP_BOWCASTER:
		cgs.effects.bowcasterShotEffect = theFxScheduler.RegisterEffect("bowcaster/shot");
		cgs.effects.bowcasterBounceEffect = theFxScheduler.RegisterEffect("bowcaster/bounce_wall");
		cgs.effects.bowcasterImpactEffect = theFxScheduler.RegisterEffect("bowcaster/explosion");
		theFxScheduler.RegisterEffect("bowcaster/deflect");
		break;

	case WP_WRIST_BLASTER:
	case WP_REPEATER:
		theFxScheduler.RegisterEffect("repeater/muzzle_smoke");
		theFxScheduler.RegisterEffect("repeater/projectile");
		theFxScheduler.RegisterEffect("repeater/alt_projectile");
		theFxScheduler.RegisterEffect("repeater/wall_impact");
		theFxScheduler.RegisterEffect("repeater/concussion");

		cgs.effects.cloneShotEffect = theFxScheduler.RegisterEffect("clone/projectile");
		cgs.effects.repeaterProjectileEffect = theFxScheduler.RegisterEffect("repeater/projectile");
		cgs.effects.repeaterAltProjectileEffect = theFxScheduler.RegisterEffect("repeater/alt_projectile");
		cgs.effects.repeaterWallImpactEffect = theFxScheduler.RegisterEffect("repeater/wall_impact");
		cgs.effects.repeaterFleshImpactEffect = theFxScheduler.RegisterEffect("repeater/flesh_impact");
		cgs.effects.repeaterAltWallImpactEffect = theFxScheduler.RegisterEffect("repeater/concussion");
		break;

	case WP_DEMP2:
		theFxScheduler.RegisterEffect("demp2/projectile");
		theFxScheduler.RegisterEffect("demp2/wall_impact");
		theFxScheduler.RegisterEffect("demp2/flesh_impact");
		theFxScheduler.RegisterEffect("demp2/altDetonate");
		cgi_R_RegisterModel("models/items/sphere.md3");
		cgi_R_RegisterShader("gfx/effects/demp2shell");
		cgs.effects.demp2ProjectileEffect = theFxScheduler.RegisterEffect("demp2/projectile");
		cgs.effects.demp2WallImpactEffect = theFxScheduler.RegisterEffect("demp2/wall_impact");
		cgs.effects.demp2FleshImpactEffect = theFxScheduler.RegisterEffect("demp2/flesh_impact");
		break;

	case WP_ATST_MAIN:
		theFxScheduler.RegisterEffect("atst/shot");
		theFxScheduler.RegisterEffect("atst/wall_impact");
		theFxScheduler.RegisterEffect("atst/flesh_impact");
		theFxScheduler.RegisterEffect("atst/droid_impact");
		break;

	case WP_ATST_SIDE:
		// For the ALT fire
		theFxScheduler.RegisterEffect("atst/side_alt_shot");
		theFxScheduler.RegisterEffect("atst/side_alt_explosion");

		// For the regular fire
		theFxScheduler.RegisterEffect("atst/side_main_shot");
		theFxScheduler.RegisterEffect("atst/side_main_impact");
		break;

	case WP_FLECHETTE:
		cgs.effects.flechetteShotEffect = theFxScheduler.RegisterEffect("flechette/shot");
		cgs.effects.flechetteAltShotEffect = theFxScheduler.RegisterEffect("flechette/alt_shot");
		cgs.effects.flechetteShotDeathEffect = theFxScheduler.RegisterEffect("flechette/wall_impact"); // shot death
		cgs.effects.flechetteFleshImpactEffect = theFxScheduler.RegisterEffect("flechette/flesh_impact");
		cgs.effects.flechetteRicochetEffect = theFxScheduler.RegisterEffect("flechette/ricochet");

		theFxScheduler.RegisterEffect("flechette/alt_blow");
		break;

	case WP_ROCKET_LAUNCHER:
		theFxScheduler.RegisterEffect("rocket/shot");
		theFxScheduler.RegisterEffect("rocket/explosion");

		cgi_R_RegisterShaderNoMip("gfx/2d/wedge");
		cgi_R_RegisterShaderNoMip("gfx/2d/lock");

		cgi_S_RegisterSound("sound/weapons/rocket/lock.wav");
		cgi_S_RegisterSound("sound/weapons/rocket/tick.wav");
		break;

	case WP_CONCUSSION:
		//Primary
		theFxScheduler.RegisterEffect("concussion/shot");
		theFxScheduler.RegisterEffect("concussion/explosion");
		//Alt
		theFxScheduler.RegisterEffect("concussion/alt_miss");
		theFxScheduler.RegisterEffect("concussion/alt_hit");
		theFxScheduler.RegisterEffect("concussion/alt_ring");
		//not used (eventually)?
		cgi_R_RegisterShader("gfx/effects/blueLine");
		cgi_R_RegisterShader("gfx/misc/whiteline2");
		break;

	case WP_THERMAL:
		cgs.media.grenadeBounce1 = cgi_S_RegisterSound("sound/weapons/thermal/bounce1.wav");
		cgs.media.grenadeBounce2 = cgi_S_RegisterSound("sound/weapons/thermal/bounce2.wav");

		cgi_S_RegisterSound("sound/weapons/thermal/thermloop.wav");
		cgi_S_RegisterSound("sound/weapons/thermal/warning.wav");
		theFxScheduler.RegisterEffect("thermal/explosion");
		theFxScheduler.RegisterEffect("thermal/shockwave");
		break;

	case WP_TRIP_MINE:
		theFxScheduler.RegisterEffect("tripMine/explosion");
		theFxScheduler.RegisterEffect("tripMine/laser");
		theFxScheduler.RegisterEffect("tripMine/laserImpactGlow");
		theFxScheduler.RegisterEffect("tripMine/glowBit");

		cgs.effects.tripminelaser = theFxScheduler.RegisterEffect("tripMine/laser");
		cgs.effects.tripminelaserImpactGlow = theFxScheduler.RegisterEffect("tripMine/laserImpactGlow");
		cgs.effects.tripmineglowBit = theFxScheduler.RegisterEffect("tripMine/glowBit");

		cgs.media.tripMineStickSound = cgi_S_RegisterSound("sound/weapons/laser_trap/stick.wav");
		cgi_S_RegisterSound("sound/weapons/laser_trap/warning.wav");
		cgi_S_RegisterSound("sound/weapons/laser_trap/hum_loop.wav");
		break;

	case WP_DET_PACK:
		theFxScheduler.RegisterEffect("detpack/explosion.efx");

		cgs.media.detPackStickSound = cgi_S_RegisterSound("sound/weapons/detpack/stick.wav");
		cgi_R_RegisterModel("models/weapons2/detpack/detpack.md3");
		cgi_S_RegisterSound("sound/weapons/detpack/warning.wav");
		cgi_S_RegisterSound("sound/weapons/explosions/explode5.wav");
		break;

	case WP_EMPLACED_GUN:
		theFxScheduler.RegisterEffect("emplaced/shot");
		theFxScheduler.RegisterEffect("emplaced/shotNPC");
		theFxScheduler.RegisterEffect("emplaced/wall_impact");
		//E-Web, too, can't tell here which one you wanted, so...
		theFxScheduler.RegisterEffect("eweb/shot");
		theFxScheduler.RegisterEffect("eweb/shotNPC");
		theFxScheduler.RegisterEffect("eweb/wall_impact");
		theFxScheduler.RegisterEffect("eweb/flesh_impact");

		cgi_R_RegisterShader("models/map_objects/imp_mine/turret_chair_dmg");
		cgi_R_RegisterShader("models/map_objects/imp_mine/turret_chair_on");

		cgs.media.emplacedHealthBarShader = cgi_R_RegisterShaderNoMip("gfx/hud/health_frame");
		cgs.media.atstHealthBarShader = cgi_R_RegisterShaderNoMip("gfx/hud/health_frame");
		cgs.media.ladyLuckHealthShader = cgi_R_RegisterShaderNoMip("gfx/hud/ladyluck_health_frame");
		cgs.media.turretComputerOverlayShader = cgi_R_RegisterShaderNoMip("gfx/hud/generic_target");
		cgs.media.turretCrossHairShader = cgi_R_RegisterShaderNoMip("gfx/2d/panel_crosshair");
		break;

	case WP_MELEE:
		theFxScheduler.RegisterEffect("melee/punch_impact");
		theFxScheduler.RegisterEffect("melee/kick_impact");
		cgi_S_RegisterSound("sound/weapons/melee/punch1.mp3");
		cgi_S_RegisterSound("sound/weapons/melee/punch2.mp3");
		cgi_S_RegisterSound("sound/weapons/melee/punch3.mp3");
		cgi_S_RegisterSound("sound/weapons/melee/punch4.mp3");

		theFxScheduler.RegisterEffect("blaster/flesh_impact");
		theFxScheduler.RegisterEffect("impacts/droid_impact1");

		if (weapon_num == WP_MELEE)
		{
			//grapple hook
			MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
			MAKERGB(weaponInfo->missileDlightColor, 1, 0.75f, 0.6f);
			weaponInfo->missileModel = cgi_R_RegisterModel("models/items/hook.md3");
			weaponInfo->missileSound = cgi_S_RegisterSound("sound/weapons/grapple/hookfire.wav");
			weaponInfo->missileTrailFunc = CG_GrappleTrail;
			weaponInfo->missileDlight = 200;
			weaponInfo->wiTrailTime = 600;
			weaponInfo->trailRadius = 64;
		}
		break;
	case WP_TUSKEN_STAFF:
		//TEMP
		theFxScheduler.RegisterEffect("melee/punch_impact");
		theFxScheduler.RegisterEffect("melee/kick_impact");
		cgi_S_RegisterSound("sound/weapons/melee/punch1.mp3");
		cgi_S_RegisterSound("sound/weapons/melee/punch2.mp3");
		cgi_S_RegisterSound("sound/weapons/melee/punch3.mp3");
		cgi_S_RegisterSound("sound/weapons/melee/punch4.mp3");
		break;

	case WP_STUN_BATON:
		cgi_R_RegisterShader("gfx/effects/stunPass");
		theFxScheduler.RegisterEffect("stunBaton/flesh_impact");
		theFxScheduler.RegisterEffect("impacts/droid_impact1");
		cgi_S_RegisterSound("sound/weapons/baton/idle.wav");
		cgi_S_RegisterSound("sound/weapons/baton/idle.wav");
		cgi_S_RegisterSound("sound/weapons/concussion/idle_lp.wav");
		cgi_S_RegisterSound("sound/weapons/noghri/fire.mp3");
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		MAKERGB(weaponInfo->missileDlightColor, 1, 0.75f, 0.6f);
		weaponInfo->missileModel = cgi_R_RegisterModel("models/weapons2/golan_arms/projectileMain.md3");
		weaponInfo->alt_missileModel = cgi_R_RegisterModel("models/weapons2/golan_arms/projectileMain.md3");
		if (cg_com_rend2.integer == 0) //rend2 is off
		{
			weaponInfo->missileTrailFunc = CG_StunTrail;
		}
		break;

	case WP_TURRET:
		theFxScheduler.RegisterEffect("turret/shot");
		theFxScheduler.RegisterEffect("turret/wall_impact");
		theFxScheduler.RegisterEffect("turret/flesh_impact");
		break;

	case WP_TUSKEN_RIFLE:
		//melee
		theFxScheduler.RegisterEffect("melee/punch_impact");
		cgi_S_RegisterSound("sound/weapons/melee/punch1.mp3");
		cgi_S_RegisterSound("sound/weapons/melee/punch2.mp3");
		cgi_S_RegisterSound("sound/weapons/melee/punch3.mp3");
		cgi_S_RegisterSound("sound/weapons/melee/punch4.mp3");
		//fire
		theFxScheduler.RegisterEffect("tusken/shot");
		theFxScheduler.RegisterEffect("tusken/hit");
		theFxScheduler.RegisterEffect("tusken/hitwall");

		break;

	case WP_SCEPTER:
		//???
		break;

	case WP_NOGHRI_STICK:
		//fire
		theFxScheduler.RegisterEffect("noghri_stick/shot");
		theFxScheduler.RegisterEffect("noghri_stick/flesh_impact");
		//explosion
		theFxScheduler.RegisterEffect("noghri_stick/gas_cloud");
		//cgi_S_RegisterSound("sound/weapons/noghri/smoke.wav");
		break;

	case WP_TIE_FIGHTER:
		theFxScheduler.RegisterEffect("ships/imp_blastershot");
		break;
	default:;
	}
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals(const int itemNum)
{
	itemInfo_t* itemInfo = &cg_items[itemNum];
	if (itemInfo->registered)
	{
		return;
	}

	const gitem_t* item = &bg_itemlist[itemNum];

	memset(itemInfo, 0, sizeof * itemInfo);
	itemInfo->registered = qtrue;

	itemInfo->models = cgi_R_RegisterModel(item->world_model);

	if (item->icon && item->icon[0])
	{
		itemInfo->icon = cgi_R_RegisterShaderNoMip(item->icon);
	}
	else
	{
		itemInfo->icon = -1;
	}

	if (item->giType == IT_WEAPON)
	{
		CG_RegisterWeapon(item->giTag);
	}

	// some ammo types are actually the weapon, like in the case of explosives
	if (item->giType == IT_AMMO)
	{
		switch (item->giTag)
		{
		case AMMO_THERMAL:
			CG_RegisterWeapon(WP_THERMAL);
			break;
		case AMMO_TRIPMINE:
			CG_RegisterWeapon(WP_TRIP_MINE);
			break;
		case AMMO_DETPACK:
			CG_RegisterWeapon(WP_DET_PACK);
			break;
		default:;
		}
	}

	if (item->giType == IT_HOLDABLE)
	{
		// This should be set up to actually work.
		switch (item->giTag)
		{
		case INV_SEEKER:
			cgi_S_RegisterSound("sound/chars/seeker/misc/fire.wav");
			cgi_S_RegisterSound("sound/chars/seeker/misc/hiss.wav");
			theFxScheduler.RegisterEffect("env/small_explode");

			CG_RegisterWeapon(WP_BLASTER);
			break;

		case INV_SENTRY:
			CG_RegisterWeapon(WP_TURRET);
			cgi_S_RegisterSound("sound/player/use_sentry");
			break;

		case INV_ELECTROBINOCULARS:
			// Binocular interface
			cgs.media.binocularCircle = cgi_R_RegisterShader("gfx/2d/binCircle");
			cgs.media.binocularMask = cgi_R_RegisterShader("gfx/2d/binMask");
			cgs.media.binocularArrow = cgi_R_RegisterShader("gfx/2d/binSideArrow");
			cgs.media.binocularTri = cgi_R_RegisterShader("gfx/2d/binTopTri");
			cgs.media.binocularStatic = cgi_R_RegisterShader("gfx/2d/binocularWindow");
			cgs.media.binocularOverlay = cgi_R_RegisterShader("gfx/2d/binocularNumOverlay");
			break;

		case INV_LIGHTAMP_GOGGLES:
			// LA Goggles Shaders
			cgs.media.laGogglesStatic = cgi_R_RegisterShader("gfx/2d/lagogglesWindow");
			cgs.media.laGogglesMask = cgi_R_RegisterShader("gfx/2d/amp_mask");
			cgs.media.laGogglesSideBit = cgi_R_RegisterShader("gfx/2d/side_bit");
			cgs.media.laGogglesBracket = cgi_R_RegisterShader("gfx/2d/bracket");
			cgs.media.laGogglesArrow = cgi_R_RegisterShader("gfx/2d/bracket2");
			break;

		case INV_BACTA_CANISTER:
			for (int i = 1; i < 5; i++)
			{
				cgi_S_RegisterSound(va("sound/weapons/force/heal%d.mp3", i));
				cgi_S_RegisterSound(va("sound/weapons/force/heal%d_m.mp3", i));
				cgi_S_RegisterSound(va("sound/weapons/force/heal%d_f.mp3", i));
			}
			break;

		case INV_BARRIER:
			cgi_S_RegisterSound("sound/barrier/barrier_on.mp3");
			cgi_S_RegisterSound("sound/barrier/barrier_loop.wav");
			cgi_S_RegisterSound("sound/barrier/barrier_off.mp3");
			break;
		default:;
		}
	}
}

/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

animations MUST match the defined pattern!
the weapon hand animation has 3 anims,
	6 frames of attack
	4 frames of drop
	5 frames of raise

  if the torso anim does not match these lengths, it will not animate correctly!
=================
*/
extern qboolean ValidAnimFileIndex(int index);

static int CG_MapTorsoToWeaponFrame(const clientInfo_t* ci, const int frame, const int animNum, int weaponNum, int firing)
{
	// we should use the animNum to map a weapon frame instead of relying on the torso frame
	if (!ValidAnimFileIndex(ci->animFileIndex))
	{
		return 0;
	}
	const animation_t* animations = level.knownAnimFileSets[ci->animFileIndex].animations;
	int ret = 0;

	switch (animNum)
	{
	case TORSO_WEAPONREADY1:
	case TORSO_WEAPONREADY2:
	case TORSO_WEAPONREADY3:
	case TORSO_WEAPONREADY4:
	case TORSO_WEAPONREADY10:
		ret = 0;
		break;

	case TORSO_DROPWEAP1:
		if (frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 5)
		{
			ret = frame - animations[animNum].firstFrame + 6;
		}
		break;

	case TORSO_RAISEWEAP1:
		if (frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 4)
		{
			ret = frame - animations[animNum].firstFrame + 6 + 5;
		}
		break;

	case BOTH_ATTACK1:
	case BOTH_ATTACK2:
	case BOTH_ATTACK3:
	case BOTH_ATTACK4:
	case BOTH_ATTACK_FP:
	case BOTH_ATTACK_DUAL:
		if (frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 6)
		{
			ret = 1 + (frame - animations[animNum].firstFrame);
		}
		break;
	default:
		break;
	}

	return ret;
}

/*
======================
CG_MachinegunSpinAngle
======================
*/
constexpr auto SPIN_SPEED = 0.9;
constexpr auto COAST_TIME = 1000;

static float cg_machinegun_spin_angle(centity_t* cent)
{
	float angle;

	int delta = cg.time - cent->pe.barrelTime;

	if (cent->pe.barrelSpinning)
	{
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	}
	else
	{
		if (delta > COAST_TIME)
		{
			delta = COAST_TIME;
		}

		const float speed = 0.5 * (SPIN_SPEED + static_cast<float>(COAST_TIME - delta) / COAST_TIME);
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if (cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING))
	{
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleNormalize360(angle);
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
		// just switching between not spinning and spinning, play the appropriate weapon sound
		cgi_S_StartSound(nullptr, cent->currentState.number, CHAN_WEAPON,
			cgi_S_RegisterSound("sound/weapons/barrelSpinning/barrelSpinning.wav"));
	}

	return angle;
}

/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition(vec3_t origin, vec3_t angles)
{
	float scale;

	VectorCopy(cg.refdef.vieworg, origin);
	VectorCopy(cg.refdefViewAngles, angles);

	// on odd legs, invert some angles
	if (cg.bobcycle & 1)
	{
		scale = -cg.xyspeed;
	}
	else
	{
		scale = cg.xyspeed;
	}

	if (cg_weaponBob.value)
	{
		// gun angles from bobbing
		angles[ROLL] += scale * cg.bobfracsin * 0.0075;
		angles[YAW] += scale * cg.bobfracsin * 0.01;
		angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.0075;
	}

	if (cg_fallingBob.value)
	{
		// drop the weapon when landing
		int delta = cg.time - cg.landTime;

		if (delta < LAND_DEFLECT_TIME)
		{
			origin[2] += cg.landChange * 0.25 * delta / LAND_DEFLECT_TIME;
		}
		else if (delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME)
		{
			origin[2] += cg.landChange * 0.25 *
				(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
		}
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if (delta < STEP_TIME / 2) {
		origin[2] -= cg.stepChange * 0.25 * delta / (STEP_TIME / 2);
	}
	else if (delta < STEP_TIME) {
		origin[2] -= cg.stepChange * 0.25 * (STEP_TIME - delta) / (STEP_TIME / 2);
	}
#endif

	if (cg_weaponBob.value)
	{
		// idle drift
		scale = 40;
		const float fracsin = sin(cg.time * 0.001);
		angles[ROLL] += scale * fracsin * 0.01;
		angles[YAW] += scale * fracsin * 0.01;
		angles[PITCH] += scale * 0.5f * fracsin * 0.01;
	}

	if (cg_gunMomentumEnable.integer)
	{
		// sway viewmodel when changing viewangles
		static vec3_t previousAngles = { 0, 0, 0 };
		static int previousTime = 0;

		vec3_t deltaAngles;
		AnglesSubtract(angles, previousAngles, deltaAngles);
		VectorScale(deltaAngles, 1.0f, deltaAngles);

		const double dampTime = static_cast<double>(cg.time - previousTime) * (1.0 / static_cast<double>(
			cg_gunMomentumInterval.integer));
		const double dampRatio = std::pow(std::abs(static_cast<double>(cg_gunMomentumDamp.value)), dampTime);
		VectorMA(previousAngles, dampRatio, deltaAngles, angles);
		VectorCopy(angles, previousAngles);
		previousTime = cg.time;

		// move viewmodel downwards when jumping etc
		static float previousOriginZ = 0.0f;
		const float deltaZ = origin[2] - previousOriginZ;
		if (deltaZ > 0.0f)
		{
			origin[2] = origin[2] - deltaZ * cg_gunMomentumFall.value;
		}
		previousOriginZ = origin[2];
	}
}

/*
Ghoul2 Insert Start
*/
// set up the appropriate ghoul2 info to a refent
static void CG_SetGhoul2InfoRef(refEntity_t* ent, const refEntity_t* s1)
{
	ent->ghoul2 = s1->ghoul2;
	VectorCopy(s1->modelScale, ent->modelScale);
	ent->radius = s1->radius;
	VectorCopy(s1->angles, ent->angles);
}

//--------------------------------------------------------------------------
static void CG_DoMuzzleFlash(centity_t* cent, vec3_t org, vec3_t dir, const weaponData_t* wData)
{
	// Handle muzzle flashes, really this could just be a qboolean instead of a time.......
	if (cent->muzzleFlashTime > 0)
	{
		cent->muzzleFlashTime = 0;
		const char* effect = nullptr;

		// Try and get a default muzzle so we have one to fall back on
		if (wData->mMuzzleEffect[0])
		{
			effect = &wData->mMuzzleEffect[0];
		}

		if (cent->alt_fire)
		{
			// We're alt-firing, so see if we need to override with a custom alt-fire effect
			if (wData->mAltMuzzleEffect[0])
			{
				effect = &wData->mAltMuzzleEffect[0];
			}
		}

		if (effect)
		{
			if (cent->gent && cent->gent->NPC || cg.renderingThirdPerson)
			{
				theFxScheduler.PlayEffect(effect, org, dir);
			}
			else
			{
				// We got an effect and we're firing, so let 'er rip.
				theFxScheduler.PlayEffect(effect, cent->currentState.clientNum);
			}
		}
	}
	else
	{
		//
	}

	if (!in_camera && cent->muzzleOverheatTime > 0)
	{
		const char* effect = nullptr;
		if (wData->mOverloadMuzzleEffect[0])
		{
			effect = &wData->mOverloadMuzzleEffect[0];
		}

		if (effect)
		{
			if (cent->gent && cent->gent->NPC || cg.renderingThirdPerson)
			{
				theFxScheduler.PlayEffect(effect, org, dir);
			}
			else
			{
				// We got an effect and we're firing, so let 'er rip.
				theFxScheduler.PlayEffect(effect, cent->currentState.clientNum);
			}
		}
		cent->muzzleOverheatTime = 0;
	}
}

/*
Ghoul2 Insert End
*/

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
extern int PM_TorsoAnimForFrame(gentity_t* ent, int torso_frame);
extern float CG_ForceSpeedFOV();
void CG_AddViewWeaponDuals(playerState_t* ps);

void CG_AddViewWeapon(playerState_t* ps)
{
	refEntity_t barrel;
	refEntity_t hand;
	refEntity_t flash;
	vec3_t angles;
	const weaponInfo_t* weapon;
	weaponData_t* w_data;
	centity_t* cent;
	float fov_offset, lean_offset;
	const qboolean doing_dash_action = cg.predictedPlayerState.communicatingflags & 1 << DASHING ? qtrue : qfalse;

	if (cg.renderingThirdPerson && !cg_trueguns.integer && !cg.zoomMode && (ps->eFlags & EF2_DUAL_WEAPONS || ps->weapon == WP_DROIDEKA))
	{
		vec3_t origin;
		cent = &cg_entities[ps->clientNum];
		// special hack for lightning guns...
		VectorCopy(cg.refdef.vieworg, origin);
		VectorMA(origin, -10, cg.refdef.viewaxis[2], origin);
		VectorMA(origin, 16, cg.refdef.viewaxis[0], origin);

		// We should still do muzzle flashes though...
		CG_RegisterWeapon(ps->weapon);
		weapon = &cg_weapons[ps->weapon];
		w_data = &weaponData[ps->weapon];

		// Not doing regular flashes
		if (!(ps->eFlags & EF2_DUAL_WEAPONS) && ps->weapon != WP_DROIDEKA)
		{
			CG_DoMuzzleFlash(cent, origin, cg.refdef.viewaxis[0], w_data);
		}

		// If we don't update this, the muzzle flash point won't even be updated properly
		VectorCopy(origin, cent->gent->client->renderInfo.muzzlePointOld);
		VectorCopy(cg.refdef.viewaxis[0], cent->gent->client->renderInfo.muzzleDirOld);

		cent->gent->client->renderInfo.mPCalcTime = cg.time;
		return;
	}

	// no gun if in third person view
	if (cg.renderingThirdPerson)
	{
		return;
	}

	if (cg_trueguns.integer && !cg.zoomMode)
	{
		return;
	}

	if (ps->pm_type == PM_INTERMISSION)
	{
		return;
	}

	cent = &cg_entities[ps->clientNum];

	if (ps->eFlags & EF_LOCKED_TO_WEAPON)
	{
		return;
	}

	if (cent->gent && cent->gent->client && cent->gent->client->ps.forcePowersActive & 1 << FP_LIGHTNING_STRIKE)
	{
		//doing the electrocuting
		vec3_t temp;

		VectorCopy(cent->gent->client->renderInfo.handLPoint, temp);
		VectorMA(temp, -5, cg.refdef.viewaxis[0], temp);
		if (cent->gent->client->ps.forcePowerLevel[FP_LIGHTNING_STRIKE] > FORCE_LEVEL_2)
		{
			//arc
			theFxScheduler.PlayEffect(cgs.effects.yellowlightningwide, temp, cg.refdef.viewaxis);
		}
		else
		{
			//line
			theFxScheduler.PlayEffect(cgs.effects.yellowlightning, temp, cg.refdef.viewaxis[0]);
		}
	}

	if (cent->gent && cent->gent->client && cent->gent->client->ps.forcePowersActive & 1 << FP_LIGHTNING)
	{
		//doing the electrocuting
		vec3_t temp;

		VectorCopy(cent->gent->client->renderInfo.handLPoint, temp);
		VectorMA(temp, -5, cg.refdef.viewaxis[0], temp);

		if (cent->gent->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2)
		{
			//arc
			if (cent->gent && cent->gent->client->NPC_class == CLASS_DESANN)
			{
				if (cg_com_outcast.integer == 2) //jko version
				{
					theFxScheduler.PlayEffect(cgs.effects.redlightningwide, temp, cg.refdef.viewaxis);
				}
				else
				{
					theFxScheduler.PlayEffect(cgs.effects.forceLightningWide, temp, cg.refdef.viewaxis);
				}
			}
			else if (cent->gent->client->NPC_class == CLASS_ALORA)
			{
				if (cg_com_outcast.integer == 2) //jko version
				{
					theFxScheduler.PlayEffect(cgs.effects.purplelightningwide, temp, cg.refdef.viewaxis);
				}
				else
				{
					theFxScheduler.PlayEffect(cgs.effects.forceLightningWide, temp, cg.refdef.viewaxis);
				}
			}
			else if (cent->gent->client->NPC_class == CLASS_TAVION)
			{
				if (cg_com_outcast.integer == 2) //jko version
				{
					theFxScheduler.PlayEffect(cgs.effects.purplelightningwide, temp, cg.refdef.viewaxis);
				}
				else
				{
					theFxScheduler.PlayEffect(cgs.effects.forceLightningWide, temp, cg.refdef.viewaxis);
				}
			}
			else if (cent->gent->client->NPC_class == CLASS_KYLE || (cg_com_outcast.integer == 1 || cg_com_outcast.integer == 4) && cent->gent->client->
				NPC_class == CLASS_PLAYER)
			{
				theFxScheduler.PlayEffect(cgs.effects.yellowlightningwide, temp, cg.refdef.viewaxis);
			}
			else if (cent->gent->client->NPC_class == CLASS_SHADOWTROOPER)
			{
				theFxScheduler.PlayEffect(cgs.effects.greenlightningwide, temp, cg.refdef.viewaxis);
			}
			else
			{
				theFxScheduler.PlayEffect(cgs.effects.forceLightningWide, temp, cg.refdef.viewaxis);
			}
		}
		else
		{
			//line
			if (cent->gent->client->NPC_class == CLASS_DESANN)
			{
				if (cg_com_outcast.integer == 2) //jko version
				{
					theFxScheduler.PlayEffect(cgs.effects.redlightning, temp, cg.refdef.viewaxis);
				}
				else
				{
					theFxScheduler.PlayEffect(cgs.effects.forceLightning, temp, cg.refdef.viewaxis);
				}
			}
			else if (cent->gent->client->NPC_class == CLASS_ALORA)
			{
				if (cg_com_outcast.integer == 2) //jko version
				{
					theFxScheduler.PlayEffect(cgs.effects.purplelightning, temp, cg.refdef.viewaxis);
				}
				else
				{
					theFxScheduler.PlayEffect(cgs.effects.forceLightning, temp, cg.refdef.viewaxis);
				}
			}
			else if (cent->gent->client->NPC_class == CLASS_TAVION)
			{
				if (cg_com_outcast.integer == 2) //jko version
				{
					theFxScheduler.PlayEffect(cgs.effects.purplelightning, temp, cg.refdef.viewaxis);
				}
				else
				{
					theFxScheduler.PlayEffect(cgs.effects.forceLightning, temp, cg.refdef.viewaxis);
				}
			}
			else if (cent->gent->client->NPC_class == CLASS_KYLE || (cg_com_outcast.integer == 1 || cg_com_outcast.integer == 4) && cent->gent->client->
				NPC_class == CLASS_PLAYER)
			{
				theFxScheduler.PlayEffect(cgs.effects.yellowlightning, temp, cg.refdef.viewaxis);
			}
			else if (cent->gent->client->NPC_class == CLASS_SHADOWTROOPER)
			{
				theFxScheduler.PlayEffect(cgs.effects.greenlightning, temp, cg.refdef.viewaxis);
			}
			else
			{
				theFxScheduler.PlayEffect(cgs.effects.forceLightning, temp, cg.refdef.viewaxis);
			}
		}
	}

	if (cent->gent && cent->gent->client && cent->gent->client->ps.forcePowersActive & 1 << FP_DRAIN)
	{
		//doing the draining
		vec3_t temp;

		VectorCopy(cent->gent->client->renderInfo.handLPoint, temp);
		VectorMA(temp, -5, cg.refdef.viewaxis[0], temp);
		if (cent->gent->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2)
		{
			//arc
			theFxScheduler.PlayEffect(cgs.effects.forceDrainWide, temp, cg.refdef.viewaxis);
		}
		else
		{
			//line
			theFxScheduler.PlayEffect(cgs.effects.forceDrain, temp, cg.refdef.viewaxis[0]);
		}
	}

	// allow the gun to be completely removed
	if (!cg_drawGun.integer || cg.zoomMode)
	{
		vec3_t origin;

		// special hack for lightning guns...
		VectorCopy(cg.refdef.vieworg, origin);
		VectorMA(origin, -10, cg.refdef.viewaxis[2], origin);
		VectorMA(origin, 16, cg.refdef.viewaxis[0], origin);

		// We should still do muzzle flashes though...
		CG_RegisterWeapon(ps->weapon);
		weapon = &cg_weapons[ps->weapon];
		w_data = &weaponData[ps->weapon];

		if (!(ps->eFlags & EF2_DUAL_WEAPONS) && ps->weapon != WP_DROIDEKA)
		{
			CG_DoMuzzleFlash(cent, origin, cg.refdef.viewaxis[0], w_data);
		}

		// If we don't update this, the muzzle flash point won't even be updated properly
		VectorCopy(origin, cent->gent->client->renderInfo.muzzlePoint);
		VectorCopy(cg.refdef.viewaxis[0], cent->gent->client->renderInfo.muzzleDir);

		cent->gent->client->renderInfo.mPCalcTime = cg.time;
		return;
	}

	// drop gun lower at higher fov
	float actualFOV;

	if (!doing_dash_action && cg.snap->ps.forcePowersActive & 1 << FP_SPEED && player->client->ps.forcePowerDuration[
		FP_SPEED]) //cg.renderingThirdPerson &&
	{
		actualFOV = CG_ForceSpeedFOV();
	}
	else
	{
		if (cg.overrides.active & CG_OVERRIDE_FOV)
		{
			actualFOV = cg.overrides.fov;
		}
		else
		{
			actualFOV = cg_fovViewmodel.integer ? cg_fovViewmodel.value : cg_fov.value;
		}
	}

	if (cg_fovViewmodelAdjust.integer && actualFOV > 90)
	{
		fov_offset = -0.1 * (actualFOV - 80);
	}
	else
	{
		fov_offset = 0;
	}

	if (ps->leanofs != 0)
	{
		//add leaning offset
		lean_offset = ps->leanofs * 0.25f;
		fov_offset += abs(ps->leanofs) * -0.1f;
	}
	else
	{
		lean_offset = 0;
	}

	CG_RegisterWeapon(ps->weapon);
	weapon = &cg_weapons[ps->weapon];
	w_data = &weaponData[ps->weapon];

	memset(&hand, 0, sizeof hand);

	if (ps->weapon == WP_STUN_BATON || ps->weapon == WP_CONCUSSION)
	{
		if (ps->weapon == WP_STUN_BATON && cent->currentState.eFlags & EF_ALT_FIRING)
		{
			cgi_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->altChargeSound);
		}
		else
		{
			cgi_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound);
		}
	}

	// set up gun position
	CG_CalculateWeaponPosition(hand.origin, angles);

	vec3_t extraOffset{};
	extraOffset[0] = extraOffset[1] = extraOffset[2] = 0.0f;

	if (ps->weapon == WP_TUSKEN_RIFLE || ps->weapon == WP_NOGHRI_STICK || ps->weapon == WP_TUSKEN_STAFF)
	{
		extraOffset[0] = 2;
		extraOffset[1] = -3;
		extraOffset[2] = -6;
	}

	VectorMA(hand.origin, cg_gun_x.value + extraOffset[0], cg.refdef.viewaxis[0], hand.origin);
	VectorMA(hand.origin, cg_gun_y.value + lean_offset + extraOffset[1], cg.refdef.viewaxis[1], hand.origin);
	VectorMA(hand.origin, cg_gun_z.value + fov_offset + extraOffset[2], cg.refdef.viewaxis[2], hand.origin);

	AnglesToAxis(angles, hand.axis);

	if (cg_fovViewmodel.integer)
	{
		float fracDistFOV = tanf(cg.refdef.fov_x * (M_PI / 180) * 0.5f);
		float fracWeapFOV = 1.0f / fracDistFOV * tanf(actualFOV * (M_PI / 180) * 0.5f);
		VectorScale(hand.axis[0], fracWeapFOV, hand.axis[0]);
	}

	// map torso animations to weapon animations
#ifndef FINAL_BUILD
	if (cg_gun_frame.integer)
	{
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	}
	else
#endif
	{
		// get clientinfo for animation map
		const clientInfo_t* ci = &cent->gent->client->clientInfo;

		int torso_anim = cent->gent->client->ps.torsoAnim; //pe.torso.animationNumber;
		float currentFrame;
		int startFrame, endFrame, flags;
		float animSpeed;
		if (cent->gent->lowerLumbarBone >= 0 && gi.G2API_GetBoneAnimIndex(
			&cent->gent->ghoul2[cent->gent->playerModel], cent->gent->lowerLumbarBone, cg.time, &currentFrame,
			&startFrame, &endFrame, &flags, &animSpeed, nullptr))
		{
			hand.oldframe = CG_MapTorsoToWeaponFrame(ci, floor(currentFrame), torso_anim, cent->currentState.weapon,
				cent->currentState.eFlags & EF_FIRING);
			hand.frame = CG_MapTorsoToWeaponFrame(ci, ceil(currentFrame), torso_anim, cent->currentState.weapon,
				cent->currentState.eFlags & EF_FIRING);
			hand.backlerp = 1.0f - (currentFrame - floor(currentFrame));
			if (cg_debugAnim.integer == 1 && cent->currentState.clientNum == 0)
			{
				Com_Printf("Torso frame %d to %d makes Weapon frame %d to %d\n", cent->pe.torso.oldFrame,
					cent->pe.torso.frame, hand.oldframe, hand.frame);
			}
		}
		else
		{
			hand.oldframe = 0;
			hand.frame = 0;
			hand.backlerp = 0.0f;
		}
	}

	// add the weapon(s)
	int numSabers = 1;
	if (cent->gent->client->ps.dualSabers)
	{
		numSabers = 2;
	}
	for (int saberNum = 0; saberNum < numSabers; saberNum++)
	{
		refEntity_t gun = {};

		gun.hModel = weapon->weaponModel;
		if (!gun.hModel)
		{
			return;
		}

		AnglesToAxis(angles, gun.axis);

		CG_PositionEntityOnTag(&gun, &hand, weapon->handsModel, "tag_weapon");

		gun.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;

		if (cent->gent && cent->gent->client && cent->currentState.weapon == WP_SABER)
		{
			for (int blade_num = 0; blade_num < cent->gent->client->ps.saber[saberNum].numBlades; blade_num++)
			{
				vec3_t axis_[3];
				vec3_t org_;
				CG_GetTagWorldPosition(&gun, "tag_flash", org_, axis_);

				//loop this and do for both blades
				if (cent->gent->client->ps.saber[0].blade[0].active && cent->gent->client->ps.saber[0].blade[0].length <
					cent->gent->client->ps.saber[0].blade[0].lengthMax)
				{
					cent->gent->client->ps.saber[0].blade[0].length += cg.frametime * 0.03;
					if (cent->gent->client->ps.saber[0].blade[0].length > cent->gent->client->ps.saber[0].blade[0].
						lengthMax)
					{
						cent->gent->client->ps.saber[0].blade[0].length = cent->gent->client->ps.saber[0].blade[0].
							lengthMax;
					}
				}
				if (saberNum == 0 && blade_num == 0)
				{
					VectorCopy(axis_[0], cent->gent->client->renderInfo.muzzleDir);
				}
				else
				{
					//need these points stored here when in 1st person saber
					VectorCopy(org_, cent->gent->client->ps.saber[saberNum].blade[blade_num].muzzlePoint);
				}
				VectorCopy(axis_[0], cent->gent->client->ps.saber[saberNum].blade[blade_num].muzzleDir);
			}
		}
		//---------
		cgi_R_AddRefEntityToScene(&gun);

		// add the spinning barrel[s]
		for (int i = 0; i < w_data->numBarrels; i++)
		{
			memset(&barrel, 0, sizeof barrel);
			barrel.hModel = weapon->barrelModel[i];
			barrel.renderfx = gun.renderfx;

			angles[YAW] = 0;
			angles[PITCH] = 0;

			if (cg_SpinningBarrels.integer && ps->weapon == WP_REPEATER)
			{
				angles[ROLL] = cg_machinegun_spin_angle(cent);
			}
			else
			{
				angles[ROLL] = 0;
			}
			AnglesToAxis(angles, barrel.axis);
			if (!i)
			{
				CG_PositionRotatedEntityOnTag(&barrel, &hand, weapon->handsModel, "tag_barrel", nullptr);
			}
			else
			{
				CG_PositionRotatedEntityOnTag(&barrel, &hand, weapon->handsModel, va("tag_barrel%d", i + 1), nullptr);
			}
			cgi_R_AddRefEntityToScene(&barrel);
		}

		memset(&flash, 0, sizeof flash);

		// Seems like we should always do this in case we have an animating muzzle flash....that way we can always store the correct muzzle dir, etc.
		CG_PositionEntityOnTag(&flash, &gun, gun.hModel, "tag_flash");

		if (!(ps->eFlags & EF2_DUAL_WEAPONS) && ps->weapon != WP_DROIDEKA)
		{
			CG_DoMuzzleFlash(cent, flash.origin, flash.axis[0], w_data);
		}

		if (cent->gent && cent->gent->client)
		{
			if (saberNum == 0)
			{
				VectorCopy(flash.origin, cent->gent->client->renderInfo.muzzlePoint);
				VectorCopy(flash.axis[0], cent->gent->client->renderInfo.muzzleDir);
			}
			cent->gent->client->renderInfo.mPCalcTime = cg.time;
		}
	}

	// Do special charge bits
	//-----------------------
	if (ps->weaponstate == WEAPON_CHARGING_ALT && ps->weapon == WP_BRYAR_PISTOL
		|| ps->weaponstate == WEAPON_CHARGING_ALT && ps->weapon == WP_BLASTER_PISTOL
		|| ps->weaponstate == WEAPON_CHARGING_ALT && ps->weapon == WP_SBD_PISTOL
		|| ps->weapon == WP_BOWCASTER && ps->weaponstate == WEAPON_CHARGING
		|| ps->weapon == WP_DEMP2 && ps->weaponstate == WEAPON_CHARGING_ALT)
	{
		int shader = 0;
		float val = 0.0f, scale = 1.0f;
		vec3_t white = { 1.0f, 1.0f, 1.0f };

		if (ps->weapon == WP_BRYAR_PISTOL || ps->weapon == WP_BLASTER_PISTOL || ps->weapon == WP_SBD_PISTOL)
		{
			// Hardcoded max charge time of 1 second
			val = (cg.time - ps->weaponChargeTime) * 0.001f;
			shader = cgi_R_RegisterShader("gfx/effects/bryarFrontFlash");
		}
		else if (ps->weapon == WP_BOWCASTER)
		{
			// Hardcoded max charge time of 1 second
			val = (cg.time - ps->weaponChargeTime) * 0.001f;
			shader = cgi_R_RegisterShader("gfx/effects/greenFrontFlash");
		}
		else if (ps->weapon == WP_DEMP2)
		{
			// Hard coded max charge time of 1 second
			val = (cg.time - ps->weaponChargeTime) * 0.001f;
			shader = cgi_R_RegisterShader("gfx/misc/lightningFlash");
			scale = 1.75f;
		}

		if (val < 0.0f)
		{
			val = 0.0f;
		}
		else if (val > 1.0f)
		{
			val = 1.0f;
			CGCam_Shake(0.1f, 100);
		}
		else
		{
			CGCam_Shake(val * val * 0.3f, 100);
		}

		val += Q_flrand(0.0f, 1.0f) * 0.5f;

		FX_AddSprite(flash.origin, nullptr, nullptr, 3.0f * val * scale, 0.7f, 0.7f, white, white, Q_flrand(0.0f, 1.0f) * 360,
			0.0f, 1.0f, shader, FX_USE_ALPHA | FX_DEPTH_HACK);
	}

	// Check if the heavy repeater is finishing up a sustained burst
	//-------------------------------
	if (ps->weapon == WP_REPEATER && ps->weaponstate == WEAPON_FIRING)
	{
		if (cent->gent && cent->gent->client && cent->gent->client->ps.weaponstate != WEAPON_FIRING)
		{
			int ct = 0;

			// the more continuous shots we've got, the more smoke we spawn
			if (cent->gent->client->ps.weaponShotCount > 60)
			{
				ct = 5;
			}
			else if (cent->gent->client->ps.weaponShotCount > 35)
			{
				ct = 3;
			}
			else if (cent->gent->client->ps.weaponShotCount > 15)
			{
				ct = 1;
			}

			for (int i = 0; i < ct; i++)
			{
				theFxScheduler.PlayEffect("repeater/muzzle_smoke", cent->currentState.clientNum);
			}

			cent->gent->client->ps.weaponShotCount = 0;
		}
	}

	if (ps->eFlags & EF2_DUAL_WEAPONS || ps->weapon == WP_DROIDEKA)
	{
		if (cg.time - cent->muzzleFlashTimeR <= MUZZLE_FLASH_TIME - 10)
		{
			if (cent->currentState.eFlags & EF_FIRING)
			{
				theFxScheduler.PlayEffect(weaponData[cent->gent->client->ps.weapon].mMuzzleEffect,
					cent->gent->client->renderInfo.muzzlePoint,
					cent->gent->client->renderInfo.muzzleDir);
			}

			if (cent->currentState.eFlags & EF_ALT_FIRING)
			{
				theFxScheduler.PlayEffect(weaponData[cent->gent->client->ps.weapon].mAltMuzzleEffect,
					cent->gent->client->renderInfo.muzzlePoint,
					cent->gent->client->renderInfo.muzzleDir);
			}

			cent->muzzleFlashTimeR = 0;
		}

		if (!in_camera && cent->muzzleOverheatTime > 0)
		{
			theFxScheduler.PlayEffect(weaponData[cent->gent->client->ps.weapon].mOverloadMuzzleEffect,
				cent->gent->client->renderInfo.muzzlePoint,
				cent->gent->client->renderInfo.muzzleDir);
			cent->muzzleOverheatTime = 0;
		}
		CG_AddViewWeaponDuals(ps);
	}
}

void CG_AddViewWeaponDuals(playerState_t* ps)
{
	refEntity_t barrel;
	refEntity_t hand;
	refEntity_t flash;
	vec3_t angles;
	const weaponInfo_t* weapon;
	weaponData_t* wData;
	centity_t* cent;
	float fovOffset, leanOffset;
	const qboolean doing_dash_action = cg.predictedPlayerState.communicatingflags & 1 << DASHING ? qtrue : qfalse;

	// no gun if in third person view
	if (cg.renderingThirdPerson)
	{
		return;
	}

	if (cg_trueguns.integer && !cg.zoomMode)
	{
		return;
	}

	if (ps->pm_type == PM_INTERMISSION)
	{
		return;
	}

	cent = &cg_entities[ps->clientNum];

	if (ps->eFlags & EF_LOCKED_TO_WEAPON)
	{
		return;
	}

	if (cent->gent && cent->gent->client && cent->gent->client->ps.forcePowersActive & 1 << FP_LIGHTNING)
	{
		//doing the electrocuting
		vec3_t temp;

		VectorCopy(cent->gent->client->renderInfo.handLPoint, temp);
		VectorMA(temp, -5, cg.refdef.viewaxis[0], temp);

		if (cent->gent->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2)
		{
			//arc
			if (cent->gent->client->NPC_class == CLASS_DESANN)
			{
				if (cg_com_outcast.integer == 2) //jko version
				{
					theFxScheduler.PlayEffect(cgs.effects.redlightningwide, temp, cg.refdef.viewaxis);
				}
				else
				{
					theFxScheduler.PlayEffect(cgs.effects.forceLightningWide, temp, cg.refdef.viewaxis);
				}
			}
			else if (cent->gent->client->NPC_class == CLASS_ALORA)
			{
				if (cg_com_outcast.integer == 2) //jko version
				{
					theFxScheduler.PlayEffect(cgs.effects.purplelightningwide, temp, cg.refdef.viewaxis);
				}
				else
				{
					theFxScheduler.PlayEffect(cgs.effects.forceLightningWide, temp, cg.refdef.viewaxis);
				}
			}
			else if (cent->gent->client->NPC_class == CLASS_TAVION)
			{
				if (cg_com_outcast.integer == 2) //jko version
				{
					theFxScheduler.PlayEffect(cgs.effects.purplelightningwide, temp, cg.refdef.viewaxis);
				}
				else
				{
					theFxScheduler.PlayEffect(cgs.effects.forceLightningWide, temp, cg.refdef.viewaxis);
				}
			}
			else if (cent->gent->client->NPC_class == CLASS_KYLE || (cg_com_outcast.integer == 1 || cg_com_outcast.integer == 4) && cent->gent->client->
				NPC_class == CLASS_PLAYER)
			{
				theFxScheduler.PlayEffect(cgs.effects.yellowlightningwide, temp, cg.refdef.viewaxis);
			}
			else if (cent->gent->client->NPC_class == CLASS_SHADOWTROOPER)
			{
				theFxScheduler.PlayEffect(cgs.effects.greenlightningwide, temp, cg.refdef.viewaxis);
			}
			else
			{
				theFxScheduler.PlayEffect(cgs.effects.forceLightningWide, temp, cg.refdef.viewaxis);
			}
		}
		else
		{
			//line
			if (cent->gent->client->NPC_class == CLASS_DESANN)
			{
				if (cg_com_outcast.integer == 2) //jko version
				{
					theFxScheduler.PlayEffect(cgs.effects.redlightning, temp, cg.refdef.viewaxis);
				}
				else
				{
					theFxScheduler.PlayEffect(cgs.effects.forceLightning, temp, cg.refdef.viewaxis);
				}
			}
			else if (cent->gent->client->NPC_class == CLASS_ALORA)
			{
				if (cg_com_outcast.integer == 2) //jko version
				{
					theFxScheduler.PlayEffect(cgs.effects.purplelightning, temp, cg.refdef.viewaxis);
				}
				else
				{
					theFxScheduler.PlayEffect(cgs.effects.forceLightning, temp, cg.refdef.viewaxis);
				}
			}
			else if (cent->gent->client->NPC_class == CLASS_TAVION)
			{
				if (cg_com_outcast.integer == 2) //jko version
				{
					theFxScheduler.PlayEffect(cgs.effects.purplelightning, temp, cg.refdef.viewaxis);
				}
				else
				{
					theFxScheduler.PlayEffect(cgs.effects.forceLightning, temp, cg.refdef.viewaxis);
				}
			}
			else if (cent->gent->client->NPC_class == CLASS_KYLE || (cg_com_outcast.integer == 1 || cg_com_outcast.integer == 4) && cent->gent->client->
				NPC_class == CLASS_PLAYER)
			{
				theFxScheduler.PlayEffect(cgs.effects.yellowlightning, temp, cg.refdef.viewaxis);
			}
			else if (cent->gent->client->NPC_class == CLASS_SHADOWTROOPER)
			{
				theFxScheduler.PlayEffect(cgs.effects.greenlightning, temp, cg.refdef.viewaxis);
			}
			else
			{
				theFxScheduler.PlayEffect(cgs.effects.forceLightning, temp, cg.refdef.viewaxis);
			}
		}
	}

	if (cent->gent && cent->gent->client && cent->gent->client->ps.forcePowersActive & 1 << FP_DRAIN)
	{
		//doing the draining
		vec3_t temp;

		VectorCopy(cent->gent->client->renderInfo.handLPoint, temp);
		VectorMA(temp, -5, cg.refdef.viewaxis[0], temp);
		if (cent->gent->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2)
		{
			//arc
			theFxScheduler.PlayEffect(cgs.effects.forceDrainWide, temp, cg.refdef.viewaxis);
		}
		else
		{
			//line
			theFxScheduler.PlayEffect(cgs.effects.forceDrain, temp, cg.refdef.viewaxis[0]);
		}
	}

	// allow the gun to be completely removed
	if (!cg_drawGun.integer || cg.zoomMode)
	{
		vec3_t origin;

		// special hack for lightning guns...
		VectorCopy(cg.refdef.vieworg, origin);
		VectorMA(origin, -10, cg.refdef.viewaxis[2], origin);
		VectorMA(origin, 16, cg.refdef.viewaxis[0], origin);

		// We should still do muzzle flashes though...
		CG_RegisterWeapon(ps->weapon);
		weapon = &cg_weapons[ps->weapon];
		wData = &weaponData[ps->weapon];

		// If we don't update this, the muzzle flash point won't even be updated properly
		VectorCopy(origin, cent->gent->client->renderInfo.muzzlePointOld);
		VectorCopy(cg.refdef.viewaxis[0], cent->gent->client->renderInfo.muzzleDirOld);

		cent->gent->client->renderInfo.mPCalcTime = cg.time;
		return;
	}

	// drop gun lower at higher fov
	float actualFOV;

	if (!doing_dash_action && cg.snap->ps.forcePowersActive & 1 << FP_SPEED && player->client->ps.forcePowerDuration[
		FP_SPEED]) //cg.renderingThirdPerson &&
	{
		actualFOV = CG_ForceSpeedFOV();
	}
	else
	{
		if (cg.overrides.active & CG_OVERRIDE_FOV)
		{
			actualFOV = cg.overrides.fov;
		}
		else
		{
			actualFOV = cg_fovViewmodel.integer ? cg_fovViewmodel.value : cg_fov.value;
		}
	}

	if (cg_fovViewmodelAdjust.integer && actualFOV > 90)
	{
		fovOffset = -0.1 * (actualFOV - 80);
	}
	else
	{
		fovOffset = 0;
	}

	if (ps->leanofs != 0)
	{
		//add leaning offset
		leanOffset = ps->leanofs * 0.25f;
		fovOffset += abs(ps->leanofs) * -0.1f;
	}
	else
	{
		leanOffset = 0;
	}

	CG_RegisterWeapon(ps->weapon);
	weapon = &cg_weapons[ps->weapon];
	wData = &weaponData[ps->weapon];

	memset(&hand, 0, sizeof hand);

	if (ps->weapon == WP_STUN_BATON || ps->weapon == WP_CONCUSSION)
	{
		if (ps->weapon == WP_STUN_BATON && cent->currentState.eFlags & EF_ALT_FIRING)
		{
			cgi_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->altChargeSound);
		}
		else
		{
			cgi_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound);
		}
	}

	// set up gun position
	CG_CalculateWeaponPosition(hand.origin, angles);

	vec3_t extraOffset{};
	extraOffset[0] = extraOffset[1] = extraOffset[2] = 0.0f;

	if (ps->weapon == WP_TUSKEN_RIFLE || ps->weapon == WP_NOGHRI_STICK || ps->weapon == WP_TUSKEN_STAFF)
	{
		extraOffset[0] = 2;
		extraOffset[1] = -3;
		extraOffset[2] = -6;
	}

	VectorMA(hand.origin, cg_gun_x.value + extraOffset[0], cg.refdef.viewaxis[0], hand.origin);
	VectorMA(hand.origin, cg_gun_y.value + leanOffset + extraOffset[1] + 9, cg.refdef.viewaxis[1], hand.origin);
	VectorMA(hand.origin, cg_gun_z.value + fovOffset + extraOffset[2], cg.refdef.viewaxis[2], hand.origin);

	AnglesToAxis(angles, hand.axis);

	if (cg_fovViewmodel.integer)
	{
		float fracDistFOV = tanf(cg.refdef.fov_x * (M_PI / 180) * 0.5f);
		float fracWeapFOV = 1.0f / fracDistFOV * tanf(actualFOV * (M_PI / 180) * 0.5f);
		VectorScale(hand.axis[0], fracWeapFOV, hand.axis[0]);
	}

	// map torso animations to weapon animations
#ifndef FINAL_BUILD
	if (cg_gun_frame.integer)
	{
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	}
	else
#endif
	{
		// get clientinfo for animation map
		const clientInfo_t* ci = &cent->gent->client->clientInfo;

		int torsoAnim = cent->gent->client->ps.torsoAnim; //pe.torso.animationNumber;
		float currentFrame;
		int startFrame, endFrame, flags;
		float animSpeed;
		if (cent->gent->lowerLumbarBone >= 0 && gi.G2API_GetBoneAnimIndex(
			&cent->gent->ghoul2[cent->gent->playerModel], cent->gent->lowerLumbarBone, cg.time, &currentFrame,
			&startFrame, &endFrame, &flags, &animSpeed, nullptr))
		{
			hand.oldframe = CG_MapTorsoToWeaponFrame(ci, floor(currentFrame), torsoAnim, cent->currentState.weapon,
				cent->currentState.eFlags & EF_FIRING);
			hand.frame = CG_MapTorsoToWeaponFrame(ci, ceil(currentFrame), torsoAnim, cent->currentState.weapon,
				cent->currentState.eFlags & EF_FIRING);
			hand.backlerp = 1.0f - (currentFrame - floor(currentFrame));
			if (cg_debugAnim.integer == 1 && cent->currentState.clientNum == 0)
			{
				Com_Printf("Torso frame %d to %d makes Weapon frame %d to %d\n", cent->pe.torso.oldFrame,
					cent->pe.torso.frame, hand.oldframe, hand.frame);
			}
		}
		else
		{
			hand.oldframe = 0;
			hand.frame = 0;
			hand.backlerp = 0.0f;
		}
	}

	// add the weapon(s)
	int numSabers = 1;
	if (cent->gent->client->ps.dualSabers)
	{
		numSabers = 2;
	}
	for (int saberNum = 0; saberNum < numSabers; saberNum++)
	{
		refEntity_t gun = {};

		gun.hModel = weapon->weaponModel;
		if (!gun.hModel)
		{
			return;
		}

		AnglesToAxis(angles, gun.axis);
		CG_PositionEntityOnTag(&gun, &hand, weapon->handsModel, "tag_weapon");

		gun.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;

		if (cent->gent && cent->gent->client && cent->currentState.weapon == WP_SABER)
		{
			for (int blade_num = 0; blade_num < cent->gent->client->ps.saber[saberNum].numBlades; blade_num++)
			{
				vec3_t axis_[3];
				vec3_t org_;
				//FIXME: need to get from tag_flash2 for saberstaff's second blade?
				CG_GetTagWorldPosition(&gun, "tag_flash", org_, axis_);
				//loop this and do for both blades
				if (cent->gent->client->ps.saber[0].blade[0].active && cent->gent->client->ps.saber[0].blade[0].length <
					cent->gent->client->ps.saber[0].blade[0].lengthMax)
				{
					cent->gent->client->ps.saber[0].blade[0].length += cg.frametime * 0.03;
					if (cent->gent->client->ps.saber[0].blade[0].length > cent->gent->client->ps.saber[0].blade[0].
						lengthMax)
					{
						cent->gent->client->ps.saber[0].blade[0].length = cent->gent->client->ps.saber[0].blade[0].
							lengthMax;
					}
				}
				if (saberNum == 0 && blade_num == 0)
				{
					VectorCopy(axis_[0], cent->gent->client->renderInfo.muzzleDir);
				}
				else
				{
					//need these points stored here when in 1st person saber
					VectorCopy(org_, cent->gent->client->ps.saber[saberNum].blade[blade_num].muzzlePoint);
				}
				VectorCopy(axis_[0], cent->gent->client->ps.saber[saberNum].blade[blade_num].muzzleDir);
			}
		}
		//---------
		cgi_R_AddRefEntityToScene(&gun);

		// add the spinning barrel[s]
		for (int i = 0; i < wData->numBarrels; i++)
		{
			memset(&barrel, 0, sizeof barrel);
			barrel.hModel = weapon->barrelModel[i];
			barrel.renderfx = gun.renderfx;

			angles[YAW] = 0;
			angles[PITCH] = 0;

			if (cg_SpinningBarrels.integer && ps->weapon == WP_REPEATER)
			{
				angles[ROLL] = cg_machinegun_spin_angle(cent);
			}
			else
			{
				angles[ROLL] = 0;
			}
			AnglesToAxis(angles, barrel.axis);
			if (!i)
			{
				CG_PositionRotatedEntityOnTag(&barrel, &hand, weapon->handsModel, "tag_barrel", nullptr);
			}
			else
			{
				CG_PositionRotatedEntityOnTag(&barrel, &hand, weapon->handsModel, va("tag_barrel%d", i + 1), nullptr);
			}
			cgi_R_AddRefEntityToScene(&barrel);
		}

		memset(&flash, 0, sizeof flash);

		// Seems like we should always do this in case we have an animating muzzle flash....that way we can always store the correct muzzle dir, etc.
		CG_PositionEntityOnTag(&flash, &gun, gun.hModel, "tag_flash");

		if (cent->gent && cent->gent->client)
		{
			if (saberNum == 0)
			{
				VectorCopy(flash.origin, cent->gent->client->renderInfo.muzzlePointOld);
				VectorCopy(flash.axis[0], cent->gent->client->renderInfo.muzzleDirOld);
			}
			cent->gent->client->renderInfo.mPCalcTime = cg.time;
		}
	}

	// Do special charge bits
	//-----------------------
	if (ps->weaponstate == WEAPON_CHARGING_ALT && ps->weapon == WP_BRYAR_PISTOL
		|| ps->weaponstate == WEAPON_CHARGING_ALT && ps->weapon == WP_BLASTER_PISTOL
		|| ps->weaponstate == WEAPON_CHARGING_ALT && ps->weapon == WP_SBD_PISTOL
		|| ps->weapon == WP_BOWCASTER && ps->weaponstate == WEAPON_CHARGING
		|| ps->weapon == WP_DEMP2 && ps->weaponstate == WEAPON_CHARGING_ALT)
	{
		int shader = 0;
		float val = 0.0f, scale = 1.0f;
		vec3_t white = { 1.0f, 1.0f, 1.0f };

		if (ps->weapon == WP_BRYAR_PISTOL || ps->weapon == WP_BLASTER_PISTOL || ps->weapon == WP_SBD_PISTOL)
		{
			// Hardcoded max charge time of 1 second
			val = (cg.time - ps->weaponChargeTime) * 0.001f;
			shader = cgi_R_RegisterShader("gfx/effects/bryarFrontFlash");
		}
		else if (ps->weapon == WP_BOWCASTER)
		{
			// Hardcoded max charge time of 1 second
			val = (cg.time - ps->weaponChargeTime) * 0.001f;
			shader = cgi_R_RegisterShader("gfx/effects/greenFrontFlash");
		}
		else if (ps->weapon == WP_DEMP2)
		{
			// Hardcoded max charge time of 1 second
			val = (cg.time - ps->weaponChargeTime) * 0.001f;
			shader = cgi_R_RegisterShader("gfx/misc/lightningFlash");
			scale = 1.75f;
		}

		if (val < 0.0f)
		{
			val = 0.0f;
		}
		else if (val > 1.0f)
		{
			val = 1.0f;
			CGCam_Shake(0.1f, 100);
		}
		else
		{
			CGCam_Shake(val * val * 0.3f, 100);
		}

		val += Q_flrand(0.0f, 1.0f) * 0.5f;

		FX_AddSprite(flash.origin, nullptr, nullptr, 3.0f * val * scale, 0.7f, 0.7f, white, white, Q_flrand(0.0f, 1.0f) * 360,
			0.0f, 1.0f, shader, FX_USE_ALPHA | FX_DEPTH_HACK);
	}

	// Check if the heavy repeater is finishing up a sustained burst
	//-------------------------------
	if (ps->weapon == WP_REPEATER && ps->weaponstate == WEAPON_FIRING)
	{
		if (cent->gent && cent->gent->client && cent->gent->client->ps.weaponstate != WEAPON_FIRING)
		{
			int ct = 0;

			// the more continuous shots we've got, the more smoke we spawn
			if (cent->gent->client->ps.weaponShotCount > 60)
			{
				ct = 5;
			}
			else if (cent->gent->client->ps.weaponShotCount > 35)
			{
				ct = 3;
			}
			else if (cent->gent->client->ps.weaponShotCount > 15)
			{
				ct = 1;
			}

			for (int i = 0; i < ct; i++)
			{
				theFxScheduler.PlayEffect("repeater/muzzle_smoke", cent->currentState.clientNum);
			}

			cent->gent->client->ps.weaponShotCount = 0;
		}
	}

	if (cg.time - cent->muzzleFlashTimeL <= MUZZLE_FLASH_TIME - 10)
	{
		if (cent->currentState.eFlags & EF_FIRING)
		{
			theFxScheduler.PlayEffect(weaponData[cent->gent->client->ps.weapon].mMuzzleEffect,
				cent->gent->client->renderInfo.muzzlePointOld,
				cent->gent->client->renderInfo.muzzleDirOld);
		}

		if (cent->currentState.eFlags & EF_ALT_FIRING)
		{
			theFxScheduler.PlayEffect(weaponData[cent->gent->client->ps.weapon].mAltMuzzleEffect,
				cent->gent->client->renderInfo.muzzlePointOld,
				cent->gent->client->renderInfo.muzzleDirOld);
		}

		cent->muzzleFlashTimeL = 0;
	}

	if (!in_camera && cent->muzzleOverheatTime > 0)
	{
		theFxScheduler.PlayEffect(weaponData[cent->gent->client->ps.weapon].mOverloadMuzzleEffect,
			cent->gent->client->renderInfo.muzzlePoint,
			cent->gent->client->renderInfo.muzzleDir);
		cent->muzzleOverheatTime = 0;
	}
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

/*
===================
CG_WeaponCheck
===================
*/
int CG_WeaponCheck(const int weapon_index)
{
	if (weapon_index == WP_SABER || weapon_index == WP_STUN_BATON)
	{
		return qtrue;
	}

	int value = weaponData[weapon_index].energyPerShot < weaponData[weapon_index].altEnergyPerShot
		? weaponData[weapon_index].energyPerShot
		: weaponData[weapon_index].altEnergyPerShot;

	if (!cg.snap)
	{
		return qfalse;
	}

	// check how much energy(ammo) it takes to fire this weapon against how much ammo we have
	if (value > cg.snap->ps.ammo[weaponData[weapon_index].ammoIndex])
	{
		value = qfalse;
	}
	else
	{
		value = qtrue;
	}

	return value;
}

int cgi_UI_GetItemText(char* menuFile, char* itemName, char* text);

const char* weaponDesc[WP_NUM_WEAPONS - 1] =
{
	"SABER_DESC",
	"NEW_BLASTER_PISTOL_DESC",
	"BLASTER_RIFLE_DESC",
	"DISRUPTOR_RIFLE_DESC",
	"BOWCASTER_DESC",
	"HEAVYREPEATER_DESC",
	"DEMP2_DESC",
	"FLECHETTE_DESC",
	"MERR_SONN_DESC",
	"THERMAL_DETONATOR_DESC",
	"TRIP_MINE_DESC",
	"DET_PACK_DESC",
	"CONCUSSION_DESC",
	"MELEE_DESC",
	"ATST_MAIN_DESC",
	"ATST_SIDE_DESC",
	"STUN_BATON_DESC",
	"BLASTER_PISTOL_DESC",
	"SBD_PISTOL_DESC",
	"WRIST_DESC",
	"EMPLACED_GUN_DESC",
	"BOT_LASER_DESC",
	"TURRET_DESC",
	"TIE_FIGHTER_DESC",
	"RAPID_CONCUSSION_DESC",
	"JAWA_DESC",
	"TUSKEN_RIFLE_DESC",
	"TUSKEN_STAFF_DESC",
	"SCEPTER_DESC",
	"NOGHRI_STICK_DESC",
};

/*
===================
CG_DrawDataPadWeaponSelect

Allows user to cycle through the various weapons currently owned and view the description
===================
*/
void CG_DrawDataPadWeaponSelect()
{
	int i;
	int sideLeftIconCnt, sideRightIconCnt;
	int iconCnt;
	vec4_t calc_color;
	char text[1024] = { 0 };
	constexpr vec4_t textColor = { .875f, .718f, .121f, 1.0f };

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	const int bits = cg.snap->ps.stats[STAT_WEAPONS];

	// count the number of weapons owned
	int count = 0;
	for (i = 1; i < 16; i++)
	{
		if (bits & 1 << i)
		{
			count++;
		}
	}

	if (count == 0) // If no weapons, don't display
	{
		return;
	}

	constexpr int sideMax = 0; // Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	const int holdCount = count - 1; // -1 for the center icon
	if (holdCount == 0) // No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > 2 * sideMax) // Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else // Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount / 2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	// This seems to be a problem is datapad comes up too early
	if (cg.DataPadWeaponSelect < FIRST_WEAPON)
	{
		cg.DataPadWeaponSelect = WP_MELEE;
	}
	else if (cg.DataPadWeaponSelect > MAX_PLAYER_WEAPONS)
	{
		cg.DataPadWeaponSelect = MAX_PLAYER_WEAPONS;
	}

	i = cg.DataPadWeaponSelect - 1;
	if (i < 1)
	{
		i = 13;
	}

	constexpr int smallIconSize = 22;
	constexpr int bigIconSize = 45;
	constexpr int pad = 8;

	int x = 320;
	int y = 300;

	// Background
	memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
	calc_color[3] = .60f;
	cgi_R_SetColor(calc_color);

	// Left side ICONS
	cgi_R_SetColor(calc_color);
	// Work backwards from current icon
	int holdX = x - (bigIconSize / 2 + pad + smallIconSize);

	for (iconCnt = 1; iconCnt < sideLeftIconCnt + 1; i--)
	{
		if (i < 1)
		{
			i = 13;
		}

		if (!(bits & 1 << i)) // Does he have this weapon?
		{
			continue;
		}

		++iconCnt; // Good icon

		if (weaponData[i].weaponIcon[0])
		{
			CG_RegisterWeapon(i);
			const weaponInfo_t* weaponInfo = &cg_weapons[i];

			if (!CG_WeaponCheck(i))
			{
				CG_DrawPic(holdX, y + 10, smallIconSize, smallIconSize, weaponInfo->weaponIconNoAmmo);
			}
			else
			{
				CG_DrawPic(holdX, y + 10, smallIconSize, smallIconSize, weaponInfo->weaponIcon);
			}

			holdX -= smallIconSize + pad;
		}
	}

	// Current Center Icon
	//height = bigIconSize * cg.iconHUDPercent;
	cgi_R_SetColor(nullptr);

	if (weaponData[cg.DataPadWeaponSelect].weaponIcon[0])
	{
		CG_RegisterWeapon(cg.DataPadWeaponSelect);
		const weaponInfo_t* weaponInfo = &cg_weapons[cg.DataPadWeaponSelect];

		if (!CG_WeaponCheck(cg.DataPadWeaponSelect))
		{
			CG_DrawPic(x - bigIconSize / 2, y - (bigIconSize - smallIconSize) / 2 + 10, bigIconSize, bigIconSize,
				weaponInfo->weaponIconNoAmmo);
		}
		else
		{
			CG_DrawPic(x - bigIconSize / 2, y - (bigIconSize - smallIconSize) / 2 + 10, bigIconSize, bigIconSize,
				weaponInfo->weaponIcon);
		}
	}

	i = cg.DataPadWeaponSelect + 1;
	if (i > 13)
	{
		i = 1;
	}

	// Right side ICONS
	// Work forwards from current icon
	cgi_R_SetColor(calc_color);
	holdX = x + bigIconSize / 2 + pad;
	//height = smallIconSize * cg.iconHUDPercent;
	for (iconCnt = 1; iconCnt < sideRightIconCnt + 1; i++)
	{
		if (i > 13)
		{
			i = 1;
		}

		if (!(bits & 1 << i)) // Does he have this weapon?
		{
			continue;
		}

		++iconCnt; // Good icon

		if (weaponData[i].weaponIcon[0])
		{
			CG_RegisterWeapon(i);
			const weaponInfo_t* weaponInfo = &cg_weapons[i];
			// No ammo for this weapon?
			if (!CG_WeaponCheck(i))
			{
				CG_DrawPic(holdX, y + 10, smallIconSize, smallIconSize, weaponInfo->weaponIconNoAmmo);
			}
			else
			{
				CG_DrawPic(holdX, y + 10, smallIconSize, smallIconSize, weaponInfo->weaponIcon);
			}

			holdX += smallIconSize + pad;
		}
	}

	// draw the weapon description
	x = 40;
	y = 70;

	if (!cgi_SP_GetStringTextString(va("SP_INGAME_%s", weaponDesc[cg.DataPadWeaponSelect - 1]), text, sizeof text))
	{
		cgi_SP_GetStringTextString(va("SPMOD_INGAME_%s", weaponDesc[cg.DataPadWeaponSelect - 1]), text, sizeof text);
	}

	if (text[0])
	{
		CG_DisplayBoxedText(70, 50, 500, 300, text,
			cgs.media.qhFontSmall,
			0.7f,
			textColor
		);
	}

	cgi_R_SetColor(nullptr);
}

/*
===================
CG_DrawDataPadIconBackground

Draw the proper background graphic for the icons being displayed on the datapad
===================
*/
void CG_DrawDataPadIconBackground(const int backgroundType)
{
	qhandle_t background;

	constexpr int x2 = 0;
	constexpr int y2 = 295;

	if (backgroundType == ICON_INVENTORY) // Display inventory background?
	{
		background = cgs.media.inventoryIconBackground;
	}
	else if (backgroundType == ICON_WEAPONS) // Display weapon background?
	{
		background = cgs.media.weaponIconBackground;
	}
	else // Display force background?
	{
		background = cgs.media.forceIconBackground;
	}

	constexpr int prongLeftX = x2 + 97;
	constexpr int prongRightX = x2 + 544;

	cgi_R_SetColor(colorTable[CT_WHITE]);
	constexpr int height = static_cast<int>(60.0f);
	CG_DrawPic(x2 + 110, y2 + 30, 410, -height, background); // Top half
	CG_DrawPic(x2 + 110, y2 + 30 - 2, 410, height, background); // Bottom half

	// And now for the prongs
	if (backgroundType == ICON_INVENTORY)
	{
		cgs.media.currentDataPadIconBackground = ICON_INVENTORY;
		background = cgs.media.inventoryProngsOn;
	}
	else if (backgroundType == ICON_WEAPONS)
	{
		cgs.media.currentDataPadIconBackground = ICON_WEAPONS;
		background = cgs.media.weaponProngsOn;
	}
	else
	{
		cgs.media.currentDataPadIconBackground = ICON_FORCE;
		background = cgs.media.forceProngsOn;
	}

	// Side Prongs
	cgi_R_SetColor(colorTable[CT_WHITE]);
	constexpr int xAdd = 8;
	CG_DrawPic(prongLeftX + xAdd, y2 - 10, 40, 80, background);
	CG_DrawPic(prongRightX - xAdd, y2 - 10, -40, 80, background);
}

/*
===============
SetWeaponSelectTime
===============
*/
void SetWeaponSelectTime()
{
	if (cg.inventorySelectTime + WEAPON_SELECT_TIME > cg.time ||
		// The Inventory HUD was currently active to just swap it out with Force HUD
		cg.forcepowerSelectTime + WEAPON_SELECT_TIME > cg.time)
		// The Force HUD was currently active to just swap it out with Force HUD
	{
		cg.inventorySelectTime = 0;
		cg.forcepowerSelectTime = 0;
		cg.weaponSelectTime = cg.time + 130.0f;
	}
	else
	{
		cg.weaponSelectTime = cg.time;
	}
}

/*
===================
CG_DrawWeaponSelect
===================
*/
extern Vehicle_t* G_IsRidingVehicle(const gentity_t* pEnt);
extern bool G_IsRidingTurboVehicle(const gentity_t* ent);

void CG_DrawWeaponSelect()
{
	int i;
	int smallIconSize, bigIconSize;
	int x2, y2, w2, h2;
	int sideLeftIconCnt, sideRightIconCnt;
	int sideMax, iconCnt;
	vec4_t calc_color;
	constexpr vec4_t textColor = { .875f, .718f, .121f, 1.0f };
	constexpr int yOffset = 0;

	if (cg.weaponSelectTime + WEAPON_SELECT_TIME < cg.time) // Time is up for the HUD to display
	{
		return;
	}

	// don't display if dead
	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	cg.iconSelectTime = cg.weaponSelectTime;

	// showing weapon select clears pickup item display, but not the blend blob
	//cg.itemPickupTime = 0;

	const int bits = cg.snap->ps.stats[STAT_WEAPONS];

	// count the number of weapons owned
	int count = 0;
	const bool is_on_veh = G_IsRidingVehicle(cg_entities[0].gent) != nullptr;

	for (i = 1; i < MAX_PLAYER_WEAPONS; i++)
	{
		if (bits & 1 << i && playerUsableWeapons[i] &&
			(!is_on_veh //PM_WeaponOkOnVehicle
				|| i == WP_NONE
				|| i == WP_SABER
				//
				|| i == WP_BLASTER_PISTOL
				|| i == WP_BLASTER
				|| i == WP_BRYAR_PISTOL
				|| i == WP_BOWCASTER
				|| i == WP_REPEATER
				|| i == WP_DEMP2
				|| i == WP_FLECHETTE))
		{
			count++;
		}
	}

	if (count == 0) // If no weapons, don't display
	{
		return;
	}

	if (is_on_veh) //PM_WeaponOkOnVehicle
	{
		sideMax = 1; // Max number of icons on the side
	}
	else
	{
		sideMax = 3; // Max number of icons on the side
	}

	// Calculate how many icons will appear to either side of the center one
	const int holdCount = count - 1; // -1 for the center icon
	if (holdCount == 0) // No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > 2 * sideMax) // Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else // Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount / 2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	if (cg.weaponSelect == WP_CONCUSSION)
	{
		i = WP_FLECHETTE;
	}
	else
	{
		i = cg.weaponSelect - 1;
	}
	if (i < 1)
	{
		i = MAX_PLAYER_WEAPONS;
	}

	if (is_on_veh) //PM_WeaponOkOnVehicle
	{
		smallIconSize = 11;
		bigIconSize = 22.5;
	}
	else
	{
		smallIconSize = 22;
		bigIconSize = 45;
	}

	constexpr int pad = 12;

	if (!cgi_UI_GetMenuInfo("weaponselecthud", &x2, &y2, &w2, &h2))
	{
		return;
	}
	constexpr int x = 320;
	constexpr int y = 410;

	// Background
	memcpy(calc_color, colorTable[CT_WHITE], sizeof(vec4_t));
	calc_color[3] = .60f;
	cgi_R_SetColor(calc_color);

	// Left side ICONS
	cgi_R_SetColor(calc_color);
	// Work backwards from current icon
	int holdX = x - (bigIconSize / 2 + pad + smallIconSize);
	//height = smallIconSize * cg.iconHUDPercent;
	qboolean drewConc = qfalse;

	for (iconCnt = 1; iconCnt < sideLeftIconCnt + 1; i--)
	{
		if (i == WP_CONCUSSION)
		{
			i--;
		}
		else if (i == WP_FLECHETTE && !drewConc && cg.weaponSelect != WP_CONCUSSION)
		{
			i = WP_CONCUSSION;
		}
		if (i < 1)
		{
			i = MAX_PLAYER_WEAPONS;
		}

		if (!(bits & 1 << i)) // Does he have this weapon?
		{
			if (i == WP_CONCUSSION)
			{
				drewConc = qtrue;
				i = WP_ROCKET_LAUNCHER;
			}
			continue;
		}
		if (is_on_veh)
		{
			if (i != WP_NONE &&
				i != WP_SABER &&
				//
				i != WP_BLASTER_PISTOL &&
				i != WP_BLASTER &&
				i != WP_BRYAR_PISTOL &&
				i != WP_BOWCASTER &&
				i != WP_REPEATER &&
				i != WP_DEMP2 &&
				i != WP_FLECHETTE)
			{
				if (i == WP_CONCUSSION)
				{
					drewConc = qtrue;
					i = WP_ROCKET_LAUNCHER;
				}
				continue; // Don't draw anything else if on a vehicle
			}
		}

		++iconCnt; // Good icon

		if (weaponData[i].weaponIcon[0])
		{
			CG_RegisterWeapon(i);
			const weaponInfo_t* weaponInfo = &cg_weapons[i];

			if (is_on_veh) //PM_WeaponOkOnVehicle
			{
				if (!CG_WeaponCheck(i))
				{
					CG_DrawPic(holdX, y + yOffset, smallIconSize, smallIconSize, weaponInfo->weaponIconNoAmmo);
				}
				else
				{
					CG_DrawPic(holdX, y + yOffset, smallIconSize, smallIconSize, weaponInfo->weaponIcon);
				}
			}
			else
			{
				if (!CG_WeaponCheck(i))
				{
					CG_DrawPic(holdX, y + 10 + yOffset, smallIconSize, smallIconSize, weaponInfo->weaponIconNoAmmo);
				}
				else
				{
					CG_DrawPic(holdX, y + 10 + yOffset, smallIconSize, smallIconSize, weaponInfo->weaponIcon);
				}
			}

			holdX -= smallIconSize + pad;
		}
		if (i == WP_CONCUSSION)
		{
			drewConc = qtrue;
			i = WP_ROCKET_LAUNCHER;
		}
	}

	// Current Center Icon
	//height = bigIconSize * cg.iconHUDPercent;
	cgi_R_SetColor(nullptr);
	if (weaponData[cg.weaponSelect].weaponIcon[0])
	{
		CG_RegisterWeapon(cg.weaponSelect);
		const weaponInfo_t* weaponInfo = &cg_weapons[cg.weaponSelect];

		if (is_on_veh) //PM_WeaponOkOnVehicle
		{
			if (!CG_WeaponCheck(cg.weaponSelect))
			{
				CG_DrawPic(x - bigIconSize / static_cast<float>(2), y - (static_cast<float>(bigIconSize) - smallIconSize) / 2 + yOffset, bigIconSize,
					bigIconSize, weaponInfo->weaponIconNoAmmo);
			}
			else
			{
				CG_DrawPic(x - bigIconSize / static_cast<float>(2), y - (static_cast<float>(bigIconSize) - smallIconSize) / 2 + yOffset, bigIconSize,
					bigIconSize, weaponInfo->weaponIcon);
			}
		}
		else
		{
			if (!CG_WeaponCheck(cg.weaponSelect))
			{
				CG_DrawPic(x - bigIconSize / static_cast<float>(2), y - (static_cast<float>(bigIconSize) - smallIconSize) / 2 + 10 + yOffset, bigIconSize,
					bigIconSize, weaponInfo->weaponIconNoAmmo);
			}
			else
			{
				CG_DrawPic(x - bigIconSize / static_cast<float>(2), y - (static_cast<float>(bigIconSize) - smallIconSize) / 2 + 10 + yOffset, bigIconSize,
					bigIconSize, weaponInfo->weaponIcon);
			}
		}
	}

	if (cg.weaponSelect == WP_CONCUSSION)
	{
		i = WP_ROCKET_LAUNCHER;
	}
	else
	{
		i = cg.weaponSelect + 1;
	}
	if (i > MAX_PLAYER_WEAPONS)
	{
		i = 1;
	}

	// Right side ICONS
	// Work forwards from current icon
	cgi_R_SetColor(calc_color);
	holdX = x + bigIconSize / 2 + pad;
	//height = smallIconSize * cg.iconHUDPercent;
	drewConc = qfalse;
	for (iconCnt = 1; iconCnt < sideRightIconCnt + 1; i++)
	{
		if (i == WP_CONCUSSION)
		{
			i++;
		}
		else if (i == WP_ROCKET_LAUNCHER && !drewConc && cg.weaponSelect != WP_CONCUSSION)
		{
			i = WP_CONCUSSION;
		}
		if (i > MAX_PLAYER_WEAPONS)
		{
			i = 1;
		}

		if (!(bits & 1 << i)) // Does he have this weapon?
		{
			if (i == WP_CONCUSSION)
			{
				drewConc = qtrue;
				i = WP_FLECHETTE;
			}
			continue;
		}
		if (is_on_veh)
		{
			if (i != WP_NONE &&
				i != WP_SABER &&
				//
				i != WP_BLASTER_PISTOL &&
				i != WP_BLASTER &&
				i != WP_BRYAR_PISTOL &&
				i != WP_BOWCASTER &&
				i != WP_REPEATER &&
				i != WP_DEMP2 &&
				i != WP_FLECHETTE)
			{
				if (i == WP_CONCUSSION)
				{
					drewConc = qtrue;
					i = WP_FLECHETTE;
				}
				continue; // Don't draw anything else if on a vehicle
			}
		}

		++iconCnt; // Good icon

		if (weaponData[i].weaponIcon[0])
		{
			CG_RegisterWeapon(i);
			const weaponInfo_t* weaponInfo = &cg_weapons[i];
			// No ammo for this weapon?

			if (is_on_veh) //PM_WeaponOkOnVehicle
			{
				if (!CG_WeaponCheck(i))
				{
					CG_DrawPic(holdX, y + yOffset, smallIconSize, smallIconSize, weaponInfo->weaponIconNoAmmo);
				}
				else
				{
					CG_DrawPic(holdX, y + yOffset, smallIconSize, smallIconSize, weaponInfo->weaponIcon);
				}
			}
			else
			{
				if (!CG_WeaponCheck(i))
				{
					CG_DrawPic(holdX, y + 10 + yOffset, smallIconSize, smallIconSize, weaponInfo->weaponIconNoAmmo);
				}
				else
				{
					CG_DrawPic(holdX, y + 10 + yOffset, smallIconSize, smallIconSize, weaponInfo->weaponIcon);
				}
			}

			holdX += smallIconSize + pad;
		}
		if (i == WP_CONCUSSION)
		{
			drewConc = qtrue;
			i = WP_FLECHETTE;
		}
	}

	const gitem_t* item = cg_weapons[cg.weaponSelect].item;

	// draw the selected name
	if (item && item->classname && item->classname[0])
	{
		if (is_on_veh) //PM_WeaponOkOnVehicle
		{
			//
		}
		else
		{
			char text[1024];

			if (cgi_SP_GetStringTextString(va("SP_INGAME_%s", item->classname), text, sizeof text))
			{
				const int w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 1.0f);
				const int ox = (SCREEN_WIDTH - w) / 2;
				cgi_R_Font_DrawString(ox, SCREEN_HEIGHT - 24 + yOffset, text, textColor, cgs.media.qhFontSmall, -1,
					1.0f);
			}
		}
	}

	cgi_R_SetColor(nullptr);
}

/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable(int i, const int original, const qboolean dpMode)
{
	if (i > MAX_PLAYER_WEAPONS || !playerUsableWeapons[i])
	{
#ifndef FINAL_BUILD
		Com_Printf("CG_WeaponSelectable() passed illegal index of %d!\n", i);
#endif
		return qfalse;
	}

	if (cg.weaponSelectTime + 200 > cg.time)
	{
		//TEMP standard weapon cycle debounce for E3 because G2 can't keep up with fast weapon changes
		return qfalse;
	}

	//FIXME: this doesn't work below, can still cycle too fast!
	if (original == WP_SABER && cg.weaponSelectTime + 500 > cg.time)
	{
		//when switch to lightsaber, have to stay there for at least half a second!
		return qfalse;
	}

	if (G_IsRidingVehicle(cg_entities[0].gent))
	{
		//PM_WeaponOkOnVehicle
		if (G_IsRidingTurboVehicle(cg_entities[0].gent) ||
			i != WP_NONE &&
			i != WP_SABER &&
			//
			i != WP_BLASTER_PISTOL &&
			i != WP_BLASTER &&
			i != WP_BRYAR_PISTOL &&
			i != WP_BOWCASTER &&
			i != WP_REPEATER &&
			i != WP_DEMP2 &&
			i != WP_FLECHETTE)
		{
			return qfalse;
		}
	}

	if (weaponData[i].ammoIndex != AMMO_NONE && !dpMode)
	{
		//weapon uses ammo, see if we have any
		const int usage_for_weap = weaponData[i].energyPerShot < weaponData[i].altEnergyPerShot
			? weaponData[i].energyPerShot
			: weaponData[i].altEnergyPerShot;

		if (cg.snap->ps.ammo[weaponData[i].ammoIndex] - usage_for_weap < 0)
		{
			if (i != WP_DET_PACK)
				// detpack can be switched to...should possibly check if there are any stuck to a wall somewhere?
			{
				// This weapon doesn't have enough ammo to shoot either the main or the alt-fire
				return qfalse;
			}
		}
	}

	if (!(cg.snap->ps.stats[STAT_WEAPONS] & 1 << i))
	{
		// Don't have this weapon to start with.
		return qfalse;
	}

	return qtrue;
}

static void CG_ToggleATSTWeapon()
{
	if (cg.weaponSelect == WP_ATST_MAIN)
	{
		cg.weaponSelect = WP_ATST_SIDE;
	}
	else
	{
		cg.weaponSelect = WP_ATST_MAIN;
	}
	SetWeaponSelectTime();
}

extern qboolean Q3_TaskIDPending(const gentity_t* ent, taskID_t taskType);

void CG_PlayerLockedWeaponSpeech(const int jumping)
{
	if (!in_camera)
	{
		static int speechDebounceTime = 0;
		//not in a cinematic
		if (speechDebounceTime < cg.time)
		{
			//spoke more than 3 seconds ago
			if (!Q3_TaskIDPending(&g_entities[0], TID_CHAN_VOICE))
			{
				//not waiting on a scripted sound to finish
				if (!jumping)
				{
					if (Q_flrand(0.0f, 1.0f) > 0.5)
					{
						if (!Q_stricmp(player->model, "kyle") || !Q_stricmp(player->model, "kyleJK2") && cg_com_outcast.
							integer == 1)
						{
							G_SoundOnEnt(player, CHAN_VOICE, va("sound/chars/kyle/09kyk015.wav"));
						}
					}
					else
					{
						if (!Q_stricmp(player->model, "kyle") || !Q_stricmp(player->model, "kyleJK2") && cg_com_outcast.
							integer == 1)
						{
							G_SoundOnEnt(player, CHAN_VOICE, va("sound/chars/kyle/09kyk016.wav"));
						}
					}
				}
				else
				{
					if (!Q_stricmp(player->model, "kyle") || !Q_stricmp(player->model, "kyleJK2") && cg_com_outcast.
						integer == 1)
					{
						G_SoundOnEnt(player, CHAN_VOICE, va("sound/chars/kyle/16kyk007.wav"));
					}
				}
				speechDebounceTime = cg.time + 3000;
			}
		}
	}
}

/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f()
{
	if (!cg.snap)
	{
		return;
	}

	if (g_entities[0].flags & FL_LOCK_PLAYER_WEAPONS)
	{
		CG_PlayerLockedWeaponSpeech(qfalse);
		return;
	}

	if (g_entities[0].client && g_entities[0].client->NPC_class == CLASS_SBD)
	{
		return;
	}

	if (cg_entities[0].gent->s.weapon == WP_DROIDEKA)
	{
		return;
	}

	if (g_entities[0].client && g_entities[0].client->NPC_class == CLASS_DROIDEKA)
	{
		return;
	}

	if (cg.predictedPlayerState.ManualBlockingFlags & 1 << HOLDINGBLOCK)
	{
		return;
	}

	if (g_entities[0].client && g_entities[0].client->NPC_class == CLASS_ATST)
	{
		CG_ToggleATSTWeapon();
		return;
	}

	if (g_entities[0].client && g_entities[0].client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_TWELVE)
	{
		NPC_SetAnim(cg_entities[0].gent, SETANIM_TORSO, BOTH_RELOADFAIL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		G_SoundOnEnt(cg_entities[0].gent, CHAN_WEAPON, "sound/weapons/reloadfail.mp3");
		G_SoundOnEnt(cg_entities[0].gent, CHAN_VOICE_ATTEN, "*pain25.wav");
		G_Damage(cg_entities[0].gent, nullptr, nullptr, nullptr, cg_entities[0].gent->currentOrigin, 2, DAMAGE_NO_ARMOR, MOD_LAVA);
		cg_entities[0].gent->reloadTime = level.time + fire_deley_time();
		return;
	}

	if (cg.snap->ps.eFlags & EF_FORCE_DRAINED)
	{
		// can't do any sort of weapon switching when being drained
		return;
	}

	if (cg.snap->ps.eFlags & EF_FORCE_GRIPPED)
	{
		// can't do any sort of weapon switching when being gripped
		return;
	}

	if (cg.snap->ps.eFlags & EF_FORCE_GRABBED)
	{
		// can't do any sort of weapon switching when being gripped
		return;
	}

	if (cg.snap->ps.eFlags & EF_LOCKED_TO_WEAPON)
	{
		// can't do any sort of weapon switching when in the emplaced gun
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_STUCK_TO_WALL)
	{
		return;
	}

	if (cg.snap->ps.viewEntity)
	{
		// yeah, probably need a better check here
		if (g_entities[cg.snap->ps.viewEntity].client && (g_entities[cg.snap->ps.viewEntity].client->NPC_class ==
			CLASS_R5D2
			|| g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_R2D2
			|| g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_MOUSE))
		{
			return;
		}
	}

	const int original = cg.weaponSelect;

	constexpr int first_weapon = FIRST_WEAPON;

	constexpr int first_vehicle_weapon = FIRST_VEHICLEWEAPON;

	if (G_IsRidingVehicle(&g_entities[cg.snap->ps.viewEntity])) //PM_WeaponOkOnVehicle
	{
		if (cg.weaponSelect < first_vehicle_weapon || cg.weaponSelect >= WP_NUM_WEAPONS)
		{
			cg.weaponSelect = first_vehicle_weapon;
		}
	}

	for (int i = 0; i <= MAX_PLAYER_WEAPONS; i++)
	{
		//*SIGH*... Hack to put concussion rifle before rocketlauncher
		if (cg.weaponSelect == WP_FLECHETTE)
		{
			cg.weaponSelect = WP_CONCUSSION;
		}
		else if (cg.weaponSelect == WP_CONCUSSION)
		{
			cg.weaponSelect = WP_ROCKET_LAUNCHER;
		}
		else if (cg.weaponSelect == WP_DET_PACK)
		{
			cg.weaponSelect = WP_MELEE;
		}
		else
		{
			cg.weaponSelect++;
		}

		if (cg.weaponSelect < first_weapon || cg.weaponSelect > MAX_PLAYER_WEAPONS)
		{
			cg.weaponSelect = first_weapon;
		}

		if (CG_WeaponSelectable(cg.weaponSelect, original, qfalse))
		{
			G_StartNextItemEffect(cg_entities[0].gent, MEF_NO_SPIN, 700, 0.3f, 0);
			SetWeaponSelectTime();
			return;
		}
	}

	cg.weaponSelect = original;
}

/*
===============
CG_DPNextWeapon_f
===============
*/
void CG_DPNextWeapon_f()
{
	if (!cg.snap)
	{
		return;
	}

	const int original = cg.DataPadWeaponSelect;

	for (int i = 0; i <= MAX_PLAYER_WEAPONS; i++)
	{
		if (cg.DataPadWeaponSelect == WP_FLECHETTE)
		{
			cg.DataPadWeaponSelect = WP_CONCUSSION;
		}
		else if (cg.DataPadWeaponSelect == WP_CONCUSSION)
		{
			cg.DataPadWeaponSelect = WP_ROCKET_LAUNCHER;
		}
		else if (cg.DataPadWeaponSelect == WP_DET_PACK)
		{
			cg.DataPadWeaponSelect = WP_MELEE;
		}
		else
		{
			cg.DataPadWeaponSelect++;
		}

		if (cg.DataPadWeaponSelect < FIRST_WEAPON || cg.DataPadWeaponSelect > MAX_PLAYER_WEAPONS)
		{
			cg.DataPadWeaponSelect = FIRST_WEAPON;
		}

		if (CG_WeaponSelectable(cg.DataPadWeaponSelect, original, qtrue))
		{
			return;
		}
	}

	cg.DataPadWeaponSelect = original;
}

/*
===============
CG_DPPrevWeapon_f
===============
*/
void CG_DPPrevWeapon_f()
{
	if (!cg.snap)
	{
		return;
	}

	const int original = cg.DataPadWeaponSelect;

	for (int i = 0; i <= MAX_PLAYER_WEAPONS; i++)
	{
		//*SIGH*... Hack to put concussion rifle before rocketlauncher
		if (cg.DataPadWeaponSelect == WP_ROCKET_LAUNCHER)
		{
			cg.DataPadWeaponSelect = WP_CONCUSSION;
		}
		else if (cg.DataPadWeaponSelect == WP_CONCUSSION)
		{
			cg.DataPadWeaponSelect = WP_FLECHETTE;
		}
		else if (cg.DataPadWeaponSelect == WP_MELEE)
		{
			cg.DataPadWeaponSelect = WP_DET_PACK;
		}
		else
		{
			cg.DataPadWeaponSelect--;
		}

		if (cg.DataPadWeaponSelect < FIRST_WEAPON || cg.DataPadWeaponSelect > MAX_PLAYER_WEAPONS)
		{
			cg.DataPadWeaponSelect = MAX_PLAYER_WEAPONS;
		}

		if (CG_WeaponSelectable(cg.DataPadWeaponSelect, original, qtrue))
		{
			return;
		}
	}

	cg.DataPadWeaponSelect = original;
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f()
{
	if (!cg.snap)
	{
		return;
	}

	if (g_entities[0].flags & FL_LOCK_PLAYER_WEAPONS)
	{
		CG_PlayerLockedWeaponSpeech(qfalse);
		return;
	}

	if (g_entities[0].client && g_entities[0].client->NPC_class == CLASS_SBD)
	{
		return;
	}

	if (cg_entities[0].gent->s.weapon == WP_DROIDEKA)
	{
		return;
	}

	if (g_entities[0].client && g_entities[0].client->NPC_class == CLASS_DROIDEKA)
	{
		return;
	}

	if (cg.predictedPlayerState.ManualBlockingFlags & 1 << HOLDINGBLOCK)
	{
		return;
	}

	if (g_entities[0].client && g_entities[0].client->NPC_class == CLASS_ATST)
	{
		CG_ToggleATSTWeapon();
		return;
	}

	if (g_entities[0].client && g_entities[0].client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_TWELVE)
	{
		NPC_SetAnim(cg_entities[0].gent, SETANIM_TORSO, BOTH_RELOADFAIL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		G_SoundOnEnt(cg_entities[0].gent, CHAN_WEAPON, "sound/weapons/reloadfail.mp3");
		G_SoundOnEnt(cg_entities[0].gent, CHAN_VOICE_ATTEN, "*pain25.wav");
		G_Damage(cg_entities[0].gent, nullptr, nullptr, nullptr, cg_entities[0].gent->currentOrigin, 2, DAMAGE_NO_ARMOR, MOD_LAVA);
		cg_entities[0].gent->reloadTime = level.time + fire_deley_time();
		return;
	}

	if (cg.snap->ps.eFlags & EF_FORCE_DRAINED)
	{
		// can't do any sort of weapon switching when being drained
		return;
	}

	if (cg.snap->ps.eFlags & EF_FORCE_GRIPPED)
	{
		// can't do any sort of weapon switching when being gripped
		return;
	}

	if (cg.snap->ps.eFlags & EF_FORCE_GRABBED)
	{
		// can't do any sort of weapon switching when being gripped
		return;
	}

	if (cg.snap->ps.eFlags & EF_LOCKED_TO_WEAPON)
	{
		// can't do any sort of weapon switching when in the emplaced gun
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_STUCK_TO_WALL)
	{
		return;
	}

	if (cg.snap->ps.viewEntity)
	{
		// yeah, probably need a better check here
		if (g_entities[cg.snap->ps.viewEntity].client && (g_entities[cg.snap->ps.viewEntity].client->NPC_class ==
			CLASS_R5D2
			|| g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_R2D2
			|| g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_MOUSE))
		{
			return;
		}
	}

	const int original = cg.weaponSelect;

	constexpr int first_weapon = FIRST_WEAPON;

	constexpr int first_vehicle_weapon = FIRST_VEHICLEWEAPON;

	if (G_IsRidingVehicle(&g_entities[cg.snap->ps.viewEntity])) //PM_WeaponOkOnVehicle
	{
		if (cg.weaponSelect < first_vehicle_weapon || cg.weaponSelect >= WP_NUM_WEAPONS)
		{
			cg.weaponSelect = first_vehicle_weapon;
		}
	}

	for (int i = 0; i <= MAX_PLAYER_WEAPONS; i++)
	{
		//*SIGH*... Hack to put concussion rifle before rocketlauncher
		if (cg.weaponSelect == WP_ROCKET_LAUNCHER)
		{
			cg.weaponSelect = WP_CONCUSSION;
		}
		else if (cg.weaponSelect == WP_CONCUSSION)
		{
			cg.weaponSelect = WP_FLECHETTE;
		}
		else if (cg.weaponSelect == WP_MELEE)
		{
			cg.weaponSelect = WP_DET_PACK;
		}
		else
		{
			cg.weaponSelect--;
		}

		if (cg.weaponSelect < first_weapon || cg.weaponSelect > MAX_PLAYER_WEAPONS)
		{
			cg.weaponSelect = MAX_PLAYER_WEAPONS;
		}

		if (CG_WeaponSelectable(cg.weaponSelect, original, qfalse))
		{
			G_StartNextItemEffect(cg_entities[0].gent, MEF_NO_SPIN, 700, 0.3f, 0);
			SetWeaponSelectTime();
			return;
		}
	}

	cg.weaponSelect = original;
}

/*
void CG_ChangeWeapon( int num )

  Meant to be called from the normal game, so checks the game-side weapon inventory data
*/
void CG_ChangeWeapon(const int num)
{
	const gentity_t* player = &g_entities[0];

	if (num < WP_NONE || num >= WP_NUM_WEAPONS)
	{
		return;
	}

	if (g_entities[0].client && g_entities[0].client->NPC_class == CLASS_SBD)
	{
		return;
	}

	if (cg_entities[0].gent->s.weapon == WP_DROIDEKA)
	{
		return;
	}

	if (g_entities[0].client && g_entities[0].client->NPC_class == CLASS_DROIDEKA)
	{
		return;
	}

	if (cg.predictedPlayerState.ManualBlockingFlags & 1 << HOLDINGBLOCK)
	{
		return;
	}

	if (g_entities[0].client && g_entities[0].client->NPC_class == CLASS_ATST)
	{
		CG_ToggleATSTWeapon();
		return;
	}

	if (g_entities[0].client && g_entities[0].client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_TWELVE)
	{
		NPC_SetAnim(cg_entities[0].gent, SETANIM_TORSO, BOTH_RELOADFAIL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		G_SoundOnEnt(cg_entities[0].gent, CHAN_WEAPON, "sound/weapons/reloadfail.mp3");
		G_SoundOnEnt(cg_entities[0].gent, CHAN_VOICE_ATTEN, "*pain25.wav");
		G_Damage(cg_entities[0].gent, nullptr, nullptr, nullptr, cg_entities[0].gent->currentOrigin, 2, DAMAGE_NO_ARMOR, MOD_LAVA);
		cg_entities[0].gent->reloadTime = level.time + fire_deley_time();
		return;
	}

	if (cg.predictedPlayerState.eFlags & EF_FORCE_DRAINED)
	{
		// can't do any sort of weapon switching when being drained
		return;
	}

	if (cg.predictedPlayerState.eFlags & EF_FORCE_GRIPPED)
	{
		// can't do any sort of weapon switching when being gripped
		return;
	}

	if (cg.predictedPlayerState.eFlags & EF_FORCE_GRABBED)
	{
		// can't do any sort of weapon switching when being gripped
		return;
	}

	if (cg.predictedPlayerState.eFlags & EF_LOCKED_TO_WEAPON)
	{
		// can't do any sort of weapon switching when in the emplaced gun
		return;
	}

	if (player->flags & FL_LOCK_PLAYER_WEAPONS)
	{
		CG_PlayerLockedWeaponSpeech(qfalse);
		return;
	}

	if (player->client != nullptr && !(player->client->ps.stats[STAT_WEAPONS] & 1 << num))
	{
		return; // don't have the weapon
	}

	if (cg.predictedPlayerState.pm_flags & PMF_STUCK_TO_WALL)
	{
		return;
	}

	// because we don't have an empty hand model for the thermal, don't allow selecting that weapon if it has no ammo
	if (num == WP_THERMAL)
	{
		if (cg.snap && cg.snap->ps.ammo[AMMO_THERMAL] <= 0)
		{
			return;
		}
	}

	// because we don't have an empty hand model for the thermal, don't allow selecting that weapon if it has no ammo
	if (num == WP_TRIP_MINE)
	{
		if (cg.snap && cg.snap->ps.ammo[AMMO_TRIPMINE] <= 0)
		{
			return;
		}
	}

	SetWeaponSelectTime();
	cg.weaponSelect = num;
}

/*
===============
CG_Weapon_f
===============
*/
extern void G_SetWeapon(gentity_t* self, int wp);

void CG_Weapon_f()
{
	if (cg.weaponSelectTime + 200 > cg.time)
	{
		return;
	}

	if (!cg.snap)
	{
		return;
	}

	if (g_entities[0].flags & FL_LOCK_PLAYER_WEAPONS)
	{
		CG_PlayerLockedWeaponSpeech(qfalse);
		return;
	}

	if (g_entities[0].client && g_entities[0].client->NPC_class == CLASS_SBD)
	{
		return;
	}

	if (cg_entities[0].gent->s.weapon == WP_DROIDEKA)
	{
		return;
	}

	if (g_entities[0].client && g_entities[0].client->NPC_class == CLASS_DROIDEKA)
	{
		return;
	}

	if (cg.predictedPlayerState.ManualBlockingFlags & 1 << HOLDINGBLOCK)
	{
		return;
	}

	if (g_entities[0].client && g_entities[0].client->NPC_class == CLASS_ATST)
	{
		CG_ToggleATSTWeapon();
		return;
	}

	if (g_entities[0].client && g_entities[0].client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_TWELVE)
	{
		NPC_SetAnim(cg_entities[0].gent, SETANIM_TORSO, BOTH_RELOADFAIL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		G_SoundOnEnt(cg_entities[0].gent, CHAN_WEAPON, "sound/weapons/reloadfail.mp3");
		G_SoundOnEnt(cg_entities[0].gent, CHAN_VOICE_ATTEN, "*pain25.wav");
		G_Damage(cg_entities[0].gent, nullptr, nullptr, nullptr, cg_entities[0].gent->currentOrigin, 2, DAMAGE_NO_ARMOR, MOD_LAVA);
		cg_entities[0].gent->reloadTime = level.time + fire_deley_time();
		return;
	}

	if (cg.snap->ps.eFlags & EF_LOCKED_TO_WEAPON)
	{
		// can't do any sort of weapon switching when in the emplaced gun
		return;
	}

	if (cg.snap->ps.viewEntity)
	{
		// yeah, probably need a better check here
		if (g_entities[cg.snap->ps.viewEntity].client && (g_entities[cg.snap->ps.viewEntity].client->NPC_class ==
			CLASS_R5D2
			|| g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_R2D2
			|| g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_MOUSE))
		{
			return;
		}
	}

	int num = atoi(CG_Argv(1));

	if (num < WP_NONE || num >= WP_NUM_WEAPONS)
	{
		return;
	}

	if (num == WP_SABER)
	{
		//lightsaber
		if (!(cg.snap->ps.stats[STAT_WEAPONS] & 1 << num))
		{
			//don't have saber, try melee
			num = WP_MELEE;
		}
		else if (num == cg.snap->ps.weapon)
		{
			//already have it up, let's try to toggle it
			if (!in_camera && !(cg.predictedPlayerState.ManualBlockingFlags & 1 << HOLDINGBLOCK))
			{
				//player can't activate/deactivate saber when in a cinematic
				//can't toggle it if not holding it and not controlling it or dead
				if (cg.predictedPlayerState.stats[STAT_HEALTH] > 0
					&& (!cg_entities[0].gent->client->ps.saberInFlight ||
						&g_entities[cg_entities[0].gent->client->ps.saberEntityNum] != nullptr &&
						g_entities[cg_entities[0].gent->client->ps.saberEntityNum].s.pos.trType == TR_LINEAR))
				{
					//it's either in-hand or it's under telekinetic control
					if (cg_entities[0].gent->client->ps.SaberActive())
					{
						//a saber is on
						if (cg_entities[0].gent->client->ps.dualSabers
							&& cg_entities[0].gent->client->ps.saber[1].Active())
						{
							//2nd saber is on, turn it off, too
							if (G_IsRidingVehicle(cg_entities[0].gent))
							{
								//PM_WeaponOkOnVehicle
								CG_RegisterWeapon(WP_NONE);
								G_SetWeapon(&g_entities[0], WP_NONE);
							}
							else
							{
								CG_RegisterWeapon(WP_MELEE);
								G_SetWeapon(&g_entities[0], WP_MELEE);
							}
						}
						if (G_IsRidingVehicle(cg_entities[0].gent))
						{
							//PM_WeaponOkOnVehicle
							CG_RegisterWeapon(WP_NONE);
							G_SetWeapon(&g_entities[0], WP_NONE);
							num = WP_NONE;
						}
						else
						{
							CG_RegisterWeapon(WP_MELEE);
							G_SetWeapon(&g_entities[0], WP_MELEE);
							num = WP_MELEE;
						}
						if (cg_entities[0].gent->client->ps.saberInFlight)
						{
							//play it on the saber
							cgi_S_UpdateEntityPosition(cg_entities[0].gent->client->ps.saberEntityNum,
								g_entities[cg_entities[0].gent->client->ps.saberEntityNum].
								currentOrigin);
							cgi_S_StartSound(nullptr, cg_entities[0].gent->client->ps.saberEntityNum, CHAN_AUTO,
								cgs.sound_precache[cg_entities[0].gent->client->ps.saber[0].soundOff]);
						}
						else
						{
							cgi_S_StartSound(nullptr, cg.snap->ps.clientNum, CHAN_AUTO,
								cgs.sound_precache[cg_entities[0].gent->client->ps.saber[0].soundOff]);
						}
					}
					else
					{
						//turn them both on
						cg_entities[0].gent->client->ps.SaberActivate();
						//if we'd holstered the second saber, best make sure it's in the left hand!
						if (cg_entities[0].gent->client->ps.dualSabers)
						{
							G_RemoveHolsterModels(cg_entities[0].gent);
							wp_saber_add_g2_saber_models(cg_entities[0].gent, qtrue);
						}
					}
				}
			}
		}
	}
	else if (num >= WP_THERMAL && num <= WP_DET_PACK) // these weapons cycle
	{
		int weap, i = 0;

		if (cg.snap->ps.weapon >= WP_THERMAL && cg.snap->ps.weapon <= WP_DET_PACK)
		{
			// already in cycle range so start with next cycle item
			weap = cg.snap->ps.weapon + 1;
		}
		else
		{
			// not in cycle range, so start with thermal detonator
			weap = WP_THERMAL;
		}

		// prevent an endless loop
		while (i <= 4)
		{
			if (weap > WP_DET_PACK)
			{
				weap = WP_THERMAL;
			}

			if (cg.snap->ps.ammo[weaponData[weap].ammoIndex] > 0 || weap == WP_DET_PACK)
			{
				if (CG_WeaponSelectable(weap, cg.snap->ps.weapon, qfalse))
				{
					num = weap;
					break;
				}
			}

			weap++;
			i++;
		}
	}
	else if (num == WP_BLASTER_PISTOL || num == WP_BRYAR_PISTOL)
	{
		int i = 0;

		int weap = cg.snap->ps.weapon;

		while (i < 2)
		{
			if (weap == WP_BLASTER_PISTOL)
			{
				weap = WP_BRYAR_PISTOL;
			}
			else if (weap == WP_BRYAR_PISTOL)
			{
				weap = WP_BLASTER_PISTOL;
			}
			else
			{
				weap = num;
			}

			if (cg.snap->ps.ammo[weaponData[weap].ammoIndex] > 0)
			{
				if (CG_WeaponSelectable(weap, cg.snap->ps.weapon, qfalse))
				{
					num = weap;
					break;
				}
			}
			i++;
		}
	}

	if (!CG_WeaponSelectable(num, cg.snap->ps.weapon, qfalse))
	{
		return;
	}

	SetWeaponSelectTime();
	cg.weaponSelect = num;
}

/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange()
{
	int i;

	if (cg.weaponSelectTime + 200 > cg.time)
		return;

	if (g_entities[0].client && g_entities[0].client->NPC_class == CLASS_ATST)
	{
		CG_ToggleATSTWeapon();
		return;
	}

	const int original = cg.weaponSelect;

	for (i = WP_ROCKET_LAUNCHER; i > 0; i--)
	{
		// We don't want the emplaced, melee, or explosive devices here
		if (original != i && CG_WeaponSelectable(i, original, qfalse))
		{
			SetWeaponSelectTime();
			cg.weaponSelect = i;
			break;
		}
	}

	if (cg_autoswitch.integer != 1)
	{
		// didn't have that, so try these. Start with thermal...
		for (i = WP_THERMAL; i <= WP_DET_PACK; i++)
		{
			// We don't want the emplaced, or melee here
			if (original != i && CG_WeaponSelectable(i, original, qfalse))
			{
				if (i == WP_DET_PACK && cg.snap->ps.ammo[weaponData[i].ammoIndex] <= 0)
				{
					// crap, no point in switching to this
				}
				else
				{
					SetWeaponSelectTime();
					cg.weaponSelect = i;
					break;
				}
			}
		}
	}

	// try stun baton as a last ditch effort
	if (CG_WeaponSelectable(WP_MELEE, original, qfalse))
	{
		SetWeaponSelectTime();
		cg.weaponSelect = WP_MELEE;
	}
}

/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
extern qboolean PM_WeponRestAnim(int anim);
extern qboolean PM_RunningAnim(int anim);

void CG_FireWeapon(centity_t* cent, const qboolean alt_fire)
{
	const entityState_t* ent = &cent->currentState;

	if (ent->weapon == WP_NONE)
	{
		return;
	}

	if (PM_ReloadAnim(cent->currentState.torsoAnim) || PM_WeponRestAnim(cent->currentState.torsoAnim))
	{
		return;
	}

	if (ent->weapon == WP_DROIDEKA && PM_RunningAnim(cent->gent->client->ps.legsAnim))
	{
		return;
	}

	if (ent->weapon >= WP_NUM_WEAPONS)
	{
		CG_Error("CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS");
		return;
	}
	if (ent->weapon == WP_TUSKEN_RIFLE && cent->gent->client)
	{
		if (cent->gent->client->ps.torsoAnim == BOTH_TUSKENATTACK1 ||
			cent->gent->client->ps.torsoAnim == BOTH_TUSKENATTACK2 ||
			cent->gent->client->ps.torsoAnim == BOTH_TUSKENATTACK3 ||
			cent->gent->client->ps.torsoAnim == BOTH_TUSKENLUNGE1)
		{
			return;
		}
	}

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	if (cent->currentState.eFlags & EF2_DUAL_WEAPONS || ent->weapon == WP_DROIDEKA)
	{
		cent->muzzleFlashTimeR = cg.time;
		cent->muzzleFlashTimeL = cg.time;
	}

	if (g_entities[0].client && g_entities[0].client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_OVERLOAD)
	{
		cent->muzzleOverheatTime = cg.time;
	}
	else
	{
		cent->muzzleOverheatTime = 0;
	}

	cent->alt_fire = alt_fire;

	if (ent->weapon == WP_SABER)
	{
		if (cent->pe.lightningFiring)
		{
		}
	}
}

/*
=================
CG_BounceEffect

Caused by an EV_BOUNCE | EV_BOUNCE_HALF event
=================
*/
void CG_BounceEffect(const int weapon, vec3_t origin, vec3_t normal)
{
	switch (weapon)
	{
	case WP_THERMAL:
		if (rand() & 1)
		{
			cgi_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.grenadeBounce1);
		}
		else
		{
			cgi_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.grenadeBounce2);
		}
		break;

	case WP_BOWCASTER:
		theFxScheduler.PlayEffect(cgs.effects.bowcasterBounceEffect, origin, normal);
		break;

	case WP_FLECHETTE:
		theFxScheduler.PlayEffect(cgs.effects.flechetteRicochetEffect, origin, normal);
		break;

	default:
		if (rand() & 1)
		{
			cgi_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.grenadeBounce1);
		}
		else
		{
			cgi_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.grenadeBounce2);
		}
		break;
	}
}

//----------------------------------------------------------------------
void CG_MissileStick(const centity_t* cent, const int weapon)
//----------------------------------------------------------------------
{
	sfxHandle_t snd = 0;

	switch (weapon)
	{
	case WP_FLECHETTE:
		snd = cgs.media.flechetteStickSound;
		break;

	case WP_DET_PACK:
		snd = cgs.media.detPackStickSound;
		break;

	case WP_TRIP_MINE:
		snd = cgs.media.tripMineStickSound;
		break;
	default:;
	}

	if (snd)
	{
		cgi_S_StartSound(nullptr, cent->currentState.number, CHAN_AUTO, snd);
	}
}

qboolean CG_VehicleWeaponImpact(centity_t* cent)
{
	//see if this is a missile entity that's owned by a vehicle and should do a special, overridden impact effect
	if (cent->currentState.otherentity_num2
		&& g_vehWeaponInfo[cent->currentState.otherentity_num2].iImpactFX)
	{
		//missile is from a special vehWeapon
		CG_PlayEffectID(g_vehWeaponInfo[cent->currentState.otherentity_num2].iImpactFX, cent->lerpOrigin,
			cent->gent->pos1);
		return qtrue;
	}
	return qfalse;
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void cg_missile_hit_wall(const centity_t* cent, const int weapon, vec3_t origin, vec3_t dir, const qboolean alt_fire)
{
	int parm;

	switch (weapon)
	{
	case WP_SBD_PISTOL: //SBD WEAPON
		if (alt_fire)
		{
			parm = 0;

			if (cent->gent)
			{
				parm += cent->gent->count;
			}

			FX_BryarsbdAltHitWall(origin, dir, parm);
		}
		else
		{
			FX_BryarsbdHitWall(origin, dir);
		}
		break;

	case WP_BRYAR_PISTOL:
	case WP_BLASTER_PISTOL:
	case WP_JAWA:
		if (alt_fire)
		{
			parm = 0;

			if (cent->gent)
			{
				parm += cent->gent->count;
			}

			FX_BryarAltHitWall(origin, dir, parm);
		}
		else
		{
			FX_BryarHitWall(origin, dir);
		}
		break;

	case WP_BLASTER:
	case WP_WRIST_BLASTER:
	case WP_DROIDEKA:
		FX_BlasterWeaponHitWall(origin, dir);
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitWall(origin, dir);
		break;

	case WP_DISRUPTOR:
		if (cent->currentState.powerups & 1 << PW_FORCE_PROJECTILE)
		{
			FX_StrikeHitWall(origin, dir);
		}
		break;

	case WP_REPEATER:
		if (alt_fire)
		{
			FX_RepeaterAltHitWall(origin, dir);
		}
		else
		{
			FX_RepeaterHitWall(origin, dir);
		}
		break;

	case WP_DEMP2:
		if (alt_fire)
		{
		}
		else
		{
			FX_DEMP2_HitWall(origin, dir);
		}
		break;

	case WP_FLECHETTE:
		if (alt_fire)
		{
			theFxScheduler.PlayEffect("flechette/alt_blow", origin, dir);
		}
		else
		{
			FX_FlechetteWeaponHitWall(origin, dir);
		}
		break;

	case WP_ROCKET_LAUNCHER:
		if (cent->currentState.powerups & 1 << PW_FORCE_PROJECTILE)
		{
			FX_BlastHitWall(origin, dir);
		}
		else
		{
			FX_RocketHitWall(origin, dir);
		}
		break;

	case WP_CONCUSSION:
		if (cent->currentState.powerups & 1 << PW_FORCE_PROJECTILE)
		{
			FX_DestructionHitWall(origin, dir);
		}
		else
		{
			FX_ConcHitWall(origin, dir);
		}
		break;

	case WP_THERMAL:
		theFxScheduler.PlayEffect("thermal/explosion", origin, dir);
		theFxScheduler.PlayEffect("thermal/shockwave", origin);
		break;

	case WP_EMPLACED_GUN:
		FX_EmplacedHitWall(origin, dir, static_cast<qboolean>(cent->gent && cent->gent->alt_fire));
		break;

	case WP_ATST_MAIN:
		FX_ATSTMainHitWall(origin, dir);
		break;

	case WP_ATST_SIDE:
		if (alt_fire)
		{
			theFxScheduler.PlayEffect("atst/side_alt_explosion", origin, dir);
		}
		else
		{
			theFxScheduler.PlayEffect("atst/side_main_impact", origin, dir);
		}
		break;

	case WP_TRIP_MINE:
		theFxScheduler.PlayEffect("tripmine/explosion", origin, dir);
		break;

	case WP_DET_PACK:
		theFxScheduler.PlayEffect("detpack/explosion", origin, dir);
		break;

	case WP_TURRET:
		theFxScheduler.PlayEffect("turret/wall_impact", origin, dir);
		break;

	case WP_TUSKEN_RIFLE:
		FX_TuskenShotWeaponHitWall(origin, dir);
		break;

	case WP_NOGHRI_STICK:
		FX_NoghriShotWeaponHitWall(origin, dir);
		break;
	default:;
	}
}

/*
-------------------------
cg_missile_hit_player
-------------------------
*/

void cg_missile_hit_player(const centity_t* cent, const int weapon, vec3_t origin, vec3_t dir, const qboolean alt_fire)
{
	gentity_t* other = nullptr;
	qboolean humanoid = qtrue;

	if (cent->gent)
	{
		other = &g_entities[cent->gent->s.otherentity_num];

		if (other->client)
		{
			const class_t npc_class = other->client->NPC_class;
			// check for all droids, maybe check for certain monsters if they're considered non-humanoid..?
			if (npc_class == CLASS_ATST || npc_class == CLASS_GONK ||
				npc_class == CLASS_INTERROGATOR || npc_class == CLASS_MARK1 ||
				npc_class == CLASS_MARK2 || npc_class == CLASS_MOUSE ||
				npc_class == CLASS_PROBE || npc_class == CLASS_PROTOCOL ||
				npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 ||
				npc_class == CLASS_SEEKER || npc_class == CLASS_SENTRY ||
				npc_class == CLASS_SBD || npc_class == CLASS_BATTLEDROID ||
				npc_class == CLASS_DROIDEKA || npc_class == CLASS_OBJECT ||
				npc_class == CLASS_ASSASSIN_DROID || npc_class == CLASS_SABER_DROID)
			{
				// special droid only behaviors
				humanoid = qfalse;
			}
		}
	}

	switch (weapon)
	{
	case WP_SBD_PISTOL: //SBD WEAPON
		if (alt_fire)
		{
			FX_BryarsbdAltHitPlayer(other, origin, dir, humanoid);
		}
		else
		{
			FX_BryarsbdHitPlayer(other, origin, dir, humanoid);
		}
		break;
	case WP_BRYAR_PISTOL:
	case WP_BLASTER_PISTOL:
	case WP_JAWA:
		if (alt_fire)
		{
			FX_BryarAltHitPlayer(other, origin, dir, humanoid);
		}
		else
		{
			FX_BryarHitPlayer(other, origin, dir, humanoid);
		}
		break;

	case WP_BLASTER:
	case WP_WRIST_BLASTER:
	case WP_DROIDEKA:
		FX_BlasterWeaponHitPlayer(other, origin, dir, humanoid);
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitPlayer(other, origin, dir, humanoid);
		break;

	case WP_DISRUPTOR:
		if (cent->currentState.powerups & 1 << PW_FORCE_PROJECTILE)
		{
			FX_StrikeHitPlayer(origin, dir);
		}
		break;

	case WP_REPEATER:
		if (alt_fire)
		{
			FX_RepeaterAltHitPlayer(other, origin, dir, humanoid);
		}
		else
		{
			FX_RepeaterHitPlayer(other, origin, dir, humanoid);
		}
		break;

	case WP_DEMP2:
		if (!alt_fire)
		{
			FX_DEMP2_HitPlayer(other, origin, dir, humanoid);
		}

		// Do a full body effect here for some more feedback
		if (other && other->client)
		{
			other->s.powerups |= 1 << PW_SHOCKED;
			other->client->ps.powerups[PW_SHOCKED] = cg.time + 1000;
		}
		break;

	case WP_FLECHETTE:
		if (alt_fire)
		{
			theFxScheduler.PlayEffect("flechette/alt_blow", origin, dir);
		}
		else
		{
			FX_FlechetteWeaponHitPlayer(other, origin, dir, humanoid);
		}
		break;

	case WP_ROCKET_LAUNCHER:
		if (cent->currentState.powerups & 1 << PW_FORCE_PROJECTILE)
		{
			FX_BlastHitPlayer(origin, dir);
		}
		else
		{
			FX_RocketHitPlayer(origin, dir, humanoid);
		}
		break;

	case WP_CONCUSSION:
		if (cent->currentState.powerups & 1 << PW_FORCE_PROJECTILE)
		{
			FX_DestructionHitPlayer(other, origin, dir, humanoid);
		}
		else
		{
			FX_ConcHitPlayer(other, origin, dir, humanoid);
		}
		break;

	case WP_THERMAL:
		theFxScheduler.PlayEffect("thermal/explosion", origin, dir);
		theFxScheduler.PlayEffect("thermal/shockwave", origin);
		break;

	case WP_EMPLACED_GUN:
		FX_EmplacedHitPlayer(other, origin, dir, static_cast<qboolean>(cent->gent && cent->gent->alt_fire));
		break;

	case WP_TRIP_MINE:
		theFxScheduler.PlayEffect("tripmine/explosion", origin, dir);
		break;

	case WP_DET_PACK:
		theFxScheduler.PlayEffect("detpack/explosion", origin, dir);
		break;

	case WP_TURRET:
		theFxScheduler.PlayEffect("turret/flesh_impact", origin, dir);
		break;

	case WP_ATST_MAIN:
		FX_EmplacedHitWall(origin, dir, qfalse);
		break;

	case WP_ATST_SIDE:
		if (alt_fire)
		{
			theFxScheduler.PlayEffect("atst/side_alt_explosion", origin, dir);
		}
		else
		{
			theFxScheduler.PlayEffect("atst/side_main_impact", origin, dir);
		}
		break;
	case WP_TUSKEN_RIFLE:
		FX_TuskenShotWeaponHitPlayer(other, origin, dir, humanoid);
		break;

	case WP_NOGHRI_STICK:
		FX_NoghriShotWeaponHitPlayer(other, origin, dir, humanoid);
		break;
	default:;
	}
}