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
///						      System designed by Serenity and modded by JaceSolaris. (c) 2023 SJE   		                    ///
///								    https://www.moddb.com/mods/serenityjediengine-20											///
///																																///
/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///

// g_weaponLoad.cpp
// fills in memory struct with ext_dat\weapons.dat

// this is excluded from PCH usage 'cos it looks kinda scary to me, being game and ui.... -Ste
#include "g_local.h"

using func_t = struct
{
	const char* name;
	void (*func)(centity_t* cent, const weaponInfo_s* weapon);
};

// Bryar
void FX_BryarProjectileThink(centity_t* cent, const weaponInfo_s* weapon);
void FX_BryarAltProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

void FX_BryaroldProjectileThink(centity_t* cent, const weaponInfo_s* weapon);
void FX_BryaroldAltProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

void FX_briar_pistolProjectileThink(centity_t* cent, const weaponInfo_s* weapon);
void FX_briar_pistolAltProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

// Blaster
void FX_BlasterProjectileThink(centity_t* cent, const weaponInfo_s* weapon);
void FX_BlasterAltFireThink(centity_t* cent, const weaponInfo_s* weapon);

void FX_DroidekaProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

// Bowcaster
void FX_BowcasterProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

// Heavy Repeater
void FX_RepeaterProjectileThink(centity_t* cent, const weaponInfo_s* weapon);
void FX_RepeaterAltProjectileThink(centity_t* cent, const weaponInfo_s* weapon);
void FX_CloneProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

// DEMP2
void FX_DEMP2_ProjectileThink(centity_t* cent, const weaponInfo_s* weapon);
void FX_DEMP2_AltProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

// Golan Arms Flechette
void FX_FlechetteProjectileThink(centity_t* cent, const weaponInfo_s* weapon);
void FX_FlechetteAltProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

// Personal Rocket Launcher
void FX_RocketProjectileThink(centity_t* cent, const weaponInfo_s* weapon);
void FX_RocketAltProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

// Concussion Rifle
void FX_ConcProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

// Emplaced weapon
void FX_EmplacedProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

// Turret weapon
void FX_TurretProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

// ATST Main weapon
void FX_ATSTMainProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

// ATST Side weapons
void FX_ATSTSideMainProjectileThink(centity_t* cent, const weaponInfo_s* weapon);
void FX_ATSTSideAltProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

//Tusken projectile
void FX_TuskenShotProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

//Noghri projectile
void FX_NoghriShotProjectileThink(centity_t* cent, const weaponInfo_s* weapon);

// Table used to attach an extern missile function string to the actual cgame function
func_t funcs[] = {
	{"bryar_func", FX_BryarProjectileThink},
	{"bryar_alt_func", FX_BryarAltProjectileThink},
	{"blaster_func", FX_BlasterProjectileThink},
	{"blaster_alt_func", FX_BlasterAltFireThink},
	{"bowcaster_func", FX_BowcasterProjectileThink},
	{"repeater_func", FX_RepeaterProjectileThink},
	{"repeater_alt_func", FX_RepeaterAltProjectileThink},
	{"demp2_func", FX_DEMP2_ProjectileThink},
	{"demp2_alt_func", FX_DEMP2_AltProjectileThink},
	{"flechette_func", FX_FlechetteProjectileThink},
	{"flechette_alt_func", FX_FlechetteAltProjectileThink},
	{"rocket_func", FX_RocketProjectileThink},
	{"rocket_alt_func", FX_RocketAltProjectileThink},
	{"conc_func", FX_ConcProjectileThink},
	{"emplaced_func", FX_EmplacedProjectileThink},
	{"turret_func", FX_TurretProjectileThink},
	{"atstmain_func", FX_ATSTMainProjectileThink},
	{"atst_side_alt_func", FX_ATSTSideAltProjectileThink},
	{"atst_side_main_func", FX_ATSTSideMainProjectileThink},
	{"tusk_shot_func", FX_TuskenShotProjectileThink},
	{"noghri_shot_func", FX_NoghriShotProjectileThink},
	{"briar_pistol_func", FX_briar_pistolProjectileThink},
	{"briar_pistol_alt_func", FX_briar_pistolAltProjectileThink},
	{"bryarold_func", FX_BryaroldProjectileThink},
	{"bryarold_alt_func", FX_BryaroldAltProjectileThink},
	{"clone_func", FX_CloneProjectileThink},
	{"dekka_func", FX_DroidekaProjectileThink},
	{nullptr, nullptr}
};

qboolean playerUsableWeapons[WP_NUM_WEAPONS] =
{
	qtrue, //WP_NONE,

	// Player weapons
	qtrue, //WP_SABER,
	qtrue, //WP_BLASTER_PISTOL,	// player and NPC weapon
	qtrue, //WP_BLASTER,			// player and NPC weapon
	qtrue, //WP_DISRUPTOR,		// player and NPC weapon
	qtrue, //WP_BOWCASTER,		// NPC weapon - player can pick this up, but never starts with them
	qtrue, //WP_REPEATER,		// NPC weapon - player can pick this up, but never starts with them
	qtrue, //WP_DEMP2,			// NPC weapon - player can pick this up, but never starts with them
	qtrue, //WP_FLECHETTE,		// NPC weapon - player can pick this up, but never starts with them
	qtrue, //WP_ROCKET_LAUNCHER,	// NPC weapon - player can pick this up, but never starts with them
	qtrue, //WP_THERMAL,			// player and NPC weapon
	qtrue, //WP_TRIP_MINE,		// NPC weapon - player can pick this up, but never starts with them
	qtrue, //WP_DET_PACK,		// NPC weapon - player can pick this up, but never starts with them
	qtrue, //WP_CONCUSSION,		// NPC weapon - player can pick this up, but never starts with them

	//extras
	qtrue, //WP_MELEE,			// player and NPC weapon - Any ol' melee attack

	//when in atst
	qfalse, //WP_ATST_MAIN,
	qfalse, //WP_ATST_SIDE,

	// These can never be gotten directly by the player
	qtrue, //WP_STUN_BATON,		// stupid weapon, should remove

	//NPC weapons
	qtrue, //WP_BRYAR_PISTOL,	// NPC weapon - player can pick this up, but never starts with them

	qfalse, //WP_SBD_PISTOL,
	qfalse, //WP_WRIST_BLASTER,

	qfalse, //WP_EMPLACED_GUN,
	qfalse, //WP_DROIDEKA,

	qfalse, //WP_BOT_LASER,		// Probe droid	- Laser blast

	qfalse, //WP_TURRET,			// turret guns

	qfalse, //WP_TIE_FIGHTER,

	qfalse, //WP_RAPID_FIRE_CONC,

	qfalse, //WP_JAWA,
	qtrue, //WP_TUSKEN_RIFLE,
	qfalse, //WP_TUSKEN_STAFF,
	qfalse, //WP_SCEPTER,
	qtrue, //WP_NOGHRI_STICK,

	//# #eol
	//WP_NUM_WEAPONS
};

struct wpnParms_s
{
	int weaponNum; // Current weapon number
	int ammoNum;
} wpnParms;

void WPN_Ammo(const char** hold_buf);
void WPN_AmmoIcon(const char** hold_buf);
void WPN_AmmoMax(const char** hold_buf);
void WPN_AmmoLowCnt(const char** hold_buf);
void WPN_AmmoType(const char** hold_buf);
void WPN_EnergyPerShot(const char** hold_buf);
void WPN_FireTime(const char** hold_buf);
void WPN_FiringSnd(const char** hold_buf);
void WPN_AltFiringSnd(const char** hold_buf);
void WPN_StopSnd(const char** hold_buf);
void WPN_ChargeSnd(const char** hold_buf);
void WPN_AltChargeSnd(const char** hold_buf);
void WPN_SelectSnd(const char** hold_buf);
void WPN_Range(const char** hold_buf);
void WPN_WeaponClass(const char** hold_buf);
void WPN_WeaponIcon(const char** hold_buf);
void WPN_WeaponModel(const char** hold_buf);
void WPN_WeaponType(const char** hold_buf);
void WPN_AltEnergyPerShot(const char** hold_buf);
void WPN_AltFireTime(const char** hold_buf);
void WPN_AltRange(const char** hold_buf);
void WPN_BarrelCount(const char** hold_buf);
void WPN_MissileName(const char** hold_buf);
void WPN_AltMissileName(const char** hold_buf);
void WPN_MissileSound(const char** hold_buf);
void WPN_AltMissileSound(const char** hold_buf);
void WPN_MissileLight(const char** hold_buf);
void WPN_AltMissileLight(const char** hold_buf);
void WPN_MissileLightColor(const char** hold_buf);
void WPN_AltMissileLightColor(const char** hold_buf);
void WPN_FuncName(const char** hold_buf);
void WPN_AltFuncName(const char** hold_buf);
void WPN_Missilehit_sound(const char** hold_buf);
void WPN_AltMissilehit_sound(const char** hold_buf);
void WPN_MuzzleEffect(const char** hold_buf);
void WPN_AltMuzzleEffect(const char** hold_buf);
void WPN_overloadmuzzleEffect(const char** hold_buf);

// SerenityJediEngine2026 ADD

void WPN_Damage(const char** hold_buf);
void WPN_AltDamage(const char** hold_buf);
void WPN_SplashDamage(const char** hold_buf);
void WPN_SplashRadius(const char** hold_buf);
void WPN_AltSplashDamage(const char** hold_buf);
void WPN_AltSplashRadius(const char** hold_buf);

// Legacy weapons.dat force fields
void WPN_FuncSkip(const char** hold_buf);

using wpnParms_t = struct
{
	const char* parmName;
	void (*func)(const char** hold_buf);
};

// This is used as a fallback for each new field, in case they're using base files
const int defaultDamage[] = {
	0, // WP_NONE
	0, // WP_SABER				// handled elsewhere
	BRYAR_PISTOL_DAMAGE, // WP_BLASTER_PISTOL
	BLASTER_DAMAGE, // WP_BLASTER
	DISRUPTOR_MAIN_DAMAGE, // WP_DISRUPTOR
	BOWCASTER_DAMAGE, // WP_BOWCASTER
	REPEATER_DAMAGE, // WP_REPEATER
	DEMP2_DAMAGE, // WP_DEMP2
	FLECHETTE_DAMAGE, // WP_FLECHETTE
	ROCKET_DAMAGE, // WP_ROCKET_LAUNCHER
	TD_DAMAGE, // WP_THERMAL
	LT_DAMAGE, // WP_TRIP_MINE
	FLECHETTE_MINE_DAMAGE, // WP_DET_PACK			// HACK, this is what the code sez.
	CONC_DAMAGE, // WP_CONCUSSION

	0, // WP_MELEE				// handled by the melee attack function

	ATST_MAIN_DAMAGE, // WP_ATST_MAIN
	ATST_SIDE_MAIN_DAMAGE, // WP_ATST_SIDE

	STUN_BATON_DAMAGE, // WP_STUN_BATON

	BRYAR_PISTOL_DAMAGE, // WP_BRYAR_PISTOL
	BRYAR_PISTOL_DAMAGE, // WP_SBD_PISTOL
	CLONECOMMANDO_DAMAGE, // WP_WRIST_BLASTER
	EMPLACED_DAMAGE, // WP_EMPLACED_GUN
	BLASTER_DAMAGE, // WP_DROIDEKA
	BRYAR_PISTOL_DAMAGE, // WP_BOT_LASER
	0, // WP_TURRET			// handled elsewhere
	EMPLACED_DAMAGE, // WP_TIE_FIGHTER
	EMPLACED_DAMAGE, // WP_RAPID_FIRE_CONC,

	BRYAR_PISTOL_DAMAGE, // WP_JAWA
	0, // WP_TUSKEN_RIFLE
	0, // WP_TUSKEN_STAFF
	0, // WP_SCEPTER
	0, // WP_NOGHRI_STICK
};

const int defaultAltDamage[] = {
	0, // WP_NONE
	0, // WP_SABER					// handled elsewhere
	BRYAR_PISTOL_DAMAGE, // WP_BLASTER_PISTOL
	BLASTER_DAMAGE, // WP_BLASTER
	DISRUPTOR_ALT_DAMAGE, // WP_DISRUPTOR
	BOWCASTER_DAMAGE, // WP_BOWCASTER
	REPEATER_ALT_DAMAGE, // WP_REPEATER
	DEMP2_ALT_DAMAGE, // WP_DEMP2
	FLECHETTE_ALT_DAMAGE, // WP_FLECHETTE
	ROCKET_DAMAGE, // WP_ROCKET_LAUNCHER
	TD_ALT_DAMAGE, // WP_THERMAL
	LT_DAMAGE, // WP_TRIP_MINE
	FLECHETTE_MINE_DAMAGE, // WP_DET_PACK				// HACK, this is what the code sez.
	CONC_ALT_DAMAGE, // WP_CONCUSION

	0, // WP_MELEE					// handled by the melee attack function

	ATST_MAIN_DAMAGE, // WP_ATST_MAIN
	ATST_SIDE_ALT_DAMAGE, // WP_ATST_SIDE

	STUN_BATON_ALT_DAMAGE, // WP_STUN_BATON

	BRYAR_PISTOL_DAMAGE, // WP_BRYAR_PISTOL
	BRYAR_PISTOL_DAMAGE, // WP_SBD_PISTOL
	CLONERIFLE_DAMAGE, // WP_WRIST_BLASTER
	EMPLACED_DAMAGE, // WP_EMPLACED_GUN
	BLASTER_DAMAGE, // WP_DROIDEKA
	BRYAR_PISTOL_DAMAGE, // WP_BOT_LASER
	0, // WP_TURRET				// handled elsewhere
	EMPLACED_DAMAGE, // WP_TIE_FIGHTER
	0, // WP_RAPID_FIRE_CONC		// repeater alt damage is used instead

	BRYAR_PISTOL_DAMAGE, // WP_JAWA
	0, // WP_TUSKEN_RIFLE
	0, // WP_TUSKEN_STAFF
	0, // WP_SCEPTER
	0, // WP_NOGHRI_STICK
};

const int defaultSplashDamage[] = {
	0, // WP_NONE
	0, // WP_SABER
	0, // WP_BLASTER_PISTOL
	0, // WP_BLASTER
	0, // WP_DISRUPTOR
	BOWCASTER_SPLASH_DAMAGE, // WP_BOWCASTER
	0, // WP_REPEATER
	0, // WP_DEMP2
	0, // WP_FLECHETTE
	ROCKET_SPLASH_DAMAGE, // WP_ROCKET_LAUNCHER
	TD_SPLASH_DAM, // WP_THERMAL
	LT_SPLASH_DAM, // WP_TRIP_MINE
	FLECHETTE_MINE_SPLASH_DAMAGE, // WP_DET_PACK		// HACK, this is what the code sez.
	CONC_SPLASH_DAMAGE, // WP_CONCUSSION

	0, // WP_MELEE

	0, // WP_ATST_MAIN
	ATST_SIDE_MAIN_SPLASH_DAMAGE, // WP_ATST_SIDE

	0, // WP_STUN_BATON

	0, // WP_BRYAR_PISTOL
	0, // WP_SBD_PISTOL
	0, // WP_WRIST_BLASTER
	0, // WP_EMPLACED_GUN
	0, // WP_DROIDEKA
	0, // WP_BOT_LASER
	0, // WP_TURRET
	0, // WP_TIE_FIGHTER
	0, // WP_RAPID_FIRE_CONC

	0, // WP_JAWA
	0, // WP_TUSKEN_RIFLE
	0, // WP_TUSKEN_STAFF
	0, // WP_SCEPTER
	0, // WP_NOGHRI_STICK
};

constexpr float defaultSplashRadius[] = {
	0.0f, // WP_NONE
	0.0f, // WP_SABER
	0.0f, // WP_BLASTER_PISTOL
	0.0f, // WP_BLASTER
	0.0f, // WP_DISRUPTOR
	BOWCASTER_SPLASH_RADIUS, // WP_BOWCASTER
	0.0f, // WP_REPEATER
	0.0f, // WP_DEMP2
	0.0f, // WP_FLECHETTE
	ROCKET_SPLASH_RADIUS, // WP_ROCKET_LAUNCHER
	TD_SPLASH_RAD, // WP_THERMAL
	LT_SPLASH_RAD, // WP_TRIP_MINE
	FLECHETTE_MINE_SPLASH_RADIUS, // WP_DET_PACK		// HACK, this is what the code sez.
	CONC_SPLASH_RADIUS, // WP_CONCUSSION

	0.0f, // WP_MELEE

	0.0f, // WP_ATST_MAIN
	ATST_SIDE_MAIN_SPLASH_RADIUS, // WP_ATST_SIDE

	0.0f, // WP_STUN_BATON

	0.0f, // WP_BRYAR_PISTOL
	0.0f, // WP_SBD_PISTOL
	0.0f, // WP_WRIST_BLASTER
	0.0f, // WP_EMPLACED_GUN
	0.0f, // WP_DROIDEKA
	0.0f, // WP_BOT_LASER
	0.0f, // WP_TURRET
	0.0f, // WP_TIE_FIGHTER
	0.0f, // WP_RAPID_FIRE_CONC

	0.0f, // WP_JAWA
	0.0f, // WP_TUSKEN_RIFLE
	0.0f, // WP_TUSKEN_STAFF
	0.0f, // WP_SCEPTER
	0.0f, // WP_NOGHRI_STICK
};

const int defaultAltSplashDamage[] = {
	0, // WP_NONE
	0, // WP_SABER			// handled elsewhere
	0, // WP_BLASTER_PISTOL
	0, // WP_BLASTER
	0, // WP_DISRUPTOR
	BOWCASTER_SPLASH_DAMAGE, // WP_BOWCASTER
	REPEATER_ALT_SPLASH_DAMAGE, // WP_REPEATER
	DEMP2_ALT_DAMAGE, // WP_DEMP2
	FLECHETTE_ALT_SPLASH_DAM, // WP_FLECHETTE
	ROCKET_SPLASH_DAMAGE, // WP_ROCKET_LAUNCHER
	TD_ALT_SPLASH_DAM, // WP_THERMAL
	TD_ALT_SPLASH_DAM, // WP_TRIP_MINE
	FLECHETTE_MINE_SPLASH_DAMAGE, // WP_DET_PACK		// HACK, this is what the code sez.
	0, // WP_CONCUSSION

	0, // WP_MELEE			// handled by the melee attack function

	0, // WP_ATST_MAIN
	ATST_SIDE_ALT_SPLASH_DAMAGE, // WP_ATST_SIDE

	0, // WP_STUN_BATON

	0, // WP_BRYAR_PISTOL
	0, // WP_SBD_PISTOL
	0, // WP_WRIST_BLASTER
	0, // WP_EMPLACED_GUN
	0, // WP_EMPLACED_GUN
	0, // WP_BOT_LASER
	0, // WP_TURRET		// handled elsewhere
	0, // WP_TIE_FIGHTER
	0, // WP_RAPID_FIRE_CONC

	0, // WP_JAWA
	0, // WP_TUSKEN_RIFLE
	0, // WP_TUSKEN_STAFF
	0, // WP_SCEPTER
	0, // WP_NOGHRI_STICK
};

constexpr float defaultAltSplashRadius[] = {
	0.0f, // WP_NONE
	0.0f, // WP_SABER		// handled elsewhere
	0.0f, // WP_BLASTER_PISTOL
	0.0f, // WP_BLASTER
	0.0f, // WP_DISRUPTOR
	BOWCASTER_SPLASH_RADIUS, // WP_BOWCASTER
	REPEATER_ALT_SPLASH_RADIUS, // WP_REPEATER
	DEMP2_ALT_SPLASHRADIUS, // WP_DEMP2
	FLECHETTE_ALT_SPLASH_RAD, // WP_FLECHETTE
	ROCKET_SPLASH_RADIUS, // WP_ROCKET_LAUNCHER
	TD_ALT_SPLASH_RAD, // WP_THERMAL
	LT_SPLASH_RAD, // WP_TRIP_MINE
	FLECHETTE_ALT_SPLASH_RAD, // WP_DET_PACK		// HACK, this is what the code sez.
	0.0f, // WP_CONCUSSION

	0.0f, // WP_MELEE			// handled by the melee attack function

	0.0f, // WP_ATST_MAIN
	ATST_SIDE_ALT_SPLASH_RADIUS, // WP_ATST_SIDE

	0.0f, // WP_STUN_BATON

	0.0f, // WP_BRYAR_PISTOL
	0.0f, // WP_SBD_PISTOL
	0.0f, // WP_WRIST_BLASTER
	0.0f, // WP_EMPLACED_GUN
	0.0f, // WP_DROIDEKA
	0.0f, // WP_BOT_LASER
	0.0f, // WP_TURRET		// handled elsewhere
	0.0f, // WP_TIE_FIGHTER
	0.0f, // WP_RAPID_FIRE_CONC

	0.0f, // WP_JAWA
	0.0f, // WP_TUSKEN_RIFLE
	0.0f, // WP_TUSKEN_STAFF
	0.0f, // WP_SCEPTER
	0.0f, // WP_NOGHRI_STICK
};

wpnParms_t WpnParms[] =
{
	{"ammo", WPN_Ammo}, //ammo
	{"ammoicon", WPN_AmmoIcon},
	{"ammomax", WPN_AmmoMax},
	{"ammolowcount", WPN_AmmoLowCnt}, //weapons
	{"ammotype", WPN_AmmoType},
	{"energypershot", WPN_EnergyPerShot},
	{"fireTime", WPN_FireTime},
	{"firingsound", WPN_FiringSnd},
	{"altfiringsound", WPN_AltFiringSnd},
	{"stopsound", WPN_StopSnd},
	{"chargesound", WPN_ChargeSnd},
	{"altchargesound", WPN_AltChargeSnd},
	{"selectsound", WPN_SelectSnd},
	{"range", WPN_Range},
	{"weaponclass", WPN_WeaponClass},
	{"weaponicon", WPN_WeaponIcon},
	{"weaponmodel", WPN_WeaponModel},
	{"weapontype", WPN_WeaponType},
	{"altenergypershot", WPN_AltEnergyPerShot},
	{"altfireTime", WPN_AltFireTime},
	{"altrange", WPN_AltRange},
	{"barrelcount", WPN_BarrelCount},
	{"missileModel", WPN_MissileName},
	{"altmissileModel", WPN_AltMissileName},
	{"missileSound", WPN_MissileSound},
	{"altmissileSound", WPN_AltMissileSound},
	{"missileLight", WPN_MissileLight},
	{"altmissileLight", WPN_AltMissileLight},
	{"missileLightColor", WPN_MissileLightColor},
	{"altmissileLightColor", WPN_AltMissileLightColor},
	{"missileFuncName", WPN_FuncName},
	{"altmissileFuncName", WPN_AltFuncName},
	{"missilehit_sound", WPN_Missilehit_sound},
	{"altmissilehit_sound", WPN_AltMissilehit_sound},
	{"muzzleEffect", WPN_MuzzleEffect},
	{"altmuzzleEffect", WPN_AltMuzzleEffect},
	{"overloadmuzzleEffect", WPN_overloadmuzzleEffect},
	// SerenityJediEngine2026 NEW FIELDS
	{"damage", WPN_Damage},
	{"altdamage", WPN_AltDamage},
	{"splashDamage", WPN_SplashDamage},
	{"splashRadius", WPN_SplashRadius},
	{"altSplashDamage", WPN_AltSplashDamage},
	{"altSplashRadius", WPN_AltSplashRadius},

	// Old legacy files contain these, so we skip them to shut up warnings
	{"firingforce", WPN_FuncSkip},
	{"chargeforce", WPN_FuncSkip},
	{"altchargeforce", WPN_FuncSkip},
	{"selectforce", WPN_FuncSkip},
};

static constexpr size_t numWpnParms = std::size(WpnParms);

void WPN_FuncSkip(const char** hold_buf)
{
	SkipRestOfLine(hold_buf);
}

void WPN_WeaponType(const char** hold_buf)
{
	int weapon_num;
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	// FIXME : put this in an array (maybe a weaponDataInternal array???)
	if (!Q_stricmp(token_str, "WP_NONE"))
		weapon_num = WP_NONE;
	else if (!Q_stricmp(token_str, "WP_SABER"))
		weapon_num = WP_SABER;
	else if (!Q_stricmp(token_str, "WP_BLASTER_PISTOL"))
		weapon_num = WP_BLASTER_PISTOL;
	else if (!Q_stricmp(token_str, "WP_BRYAR_PISTOL"))
		weapon_num = WP_BRYAR_PISTOL;
	else if (!Q_stricmp(token_str, "WP_WRIST_BLASTER"))
		weapon_num = WP_WRIST_BLASTER;
	else if (!Q_stricmp(token_str, "WP_SBD_PISTOL"))
		weapon_num = WP_SBD_PISTOL;
	else if (!Q_stricmp(token_str, "WP_BLASTER"))
		weapon_num = WP_BLASTER;
	else if (!Q_stricmp(token_str, "WP_DISRUPTOR"))
		weapon_num = WP_DISRUPTOR;
	else if (!Q_stricmp(token_str, "WP_BOWCASTER"))
		weapon_num = WP_BOWCASTER;
	else if (!Q_stricmp(token_str, "WP_REPEATER"))
		weapon_num = WP_REPEATER;
	else if (!Q_stricmp(token_str, "WP_DEMP2"))
		weapon_num = WP_DEMP2;
	else if (!Q_stricmp(token_str, "WP_FLECHETTE"))
		weapon_num = WP_FLECHETTE;
	else if (!Q_stricmp(token_str, "WP_ROCKET_LAUNCHER"))
		weapon_num = WP_ROCKET_LAUNCHER;
	else if (!Q_stricmp(token_str, "WP_CONCUSSION"))
		weapon_num = WP_CONCUSSION;
	else if (!Q_stricmp(token_str, "WP_THERMAL"))
		weapon_num = WP_THERMAL;
	else if (!Q_stricmp(token_str, "WP_TRIP_MINE"))
		weapon_num = WP_TRIP_MINE;
	else if (!Q_stricmp(token_str, "WP_DET_PACK"))
		weapon_num = WP_DET_PACK;
	else if (!Q_stricmp(token_str, "WP_STUN_BATON"))
		weapon_num = WP_STUN_BATON;
	else if (!Q_stricmp(token_str, "WP_BOT_LASER"))
		weapon_num = WP_BOT_LASER;
	else if (!Q_stricmp(token_str, "WP_EMPLACED_GUN"))
		weapon_num = WP_EMPLACED_GUN;
	else if (!Q_stricmp(token_str, "WP_DROIDEKA"))
		weapon_num = WP_DROIDEKA;
	else if (!Q_stricmp(token_str, "WP_MELEE"))
		weapon_num = WP_MELEE;
	else if (!Q_stricmp(token_str, "WP_TURRET"))
		weapon_num = WP_TURRET;
	else if (!Q_stricmp(token_str, "WP_ATST_MAIN"))
		weapon_num = WP_ATST_MAIN;
	else if (!Q_stricmp(token_str, "WP_ATST_SIDE"))
		weapon_num = WP_ATST_SIDE;
	else if (!Q_stricmp(token_str, "WP_TIE_FIGHTER"))
		weapon_num = WP_TIE_FIGHTER;
	else if (!Q_stricmp(token_str, "WP_RAPID_FIRE_CONC"))
		weapon_num = WP_RAPID_FIRE_CONC;
	else if (!Q_stricmp(token_str, "WP_JAWA"))
		weapon_num = WP_JAWA;
	else if (!Q_stricmp(token_str, "WP_TUSKEN_RIFLE"))
		weapon_num = WP_TUSKEN_RIFLE;
	else if (!Q_stricmp(token_str, "WP_TUSKEN_STAFF"))
		weapon_num = WP_TUSKEN_STAFF;
	else if (!Q_stricmp(token_str, "WP_SCEPTER"))
		weapon_num = WP_SCEPTER;
	else if (!Q_stricmp(token_str, "WP_NOGHRI_STICK"))
		weapon_num = WP_NOGHRI_STICK;
	else
	{
		weapon_num = 0;
		gi.Printf(S_COLOR_YELLOW"WARNING: bad weapontype in external weapon data '%s'\n", token_str);
	}

	wpnParms.weaponNum = weapon_num;
}

//--------------------------------------------
void WPN_WeaponClass(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 32)
	{
		len = 32;
		gi.Printf(S_COLOR_YELLOW"WARNING: weaponclass too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].classname, token_str, len);
}

//--------------------------------------------
void WPN_WeaponModel(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: weaponMdl too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].weaponMdl, token_str, len);
}

//--------------------------------------------
void WPN_WeaponIcon(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: weaponIcon too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].weaponIcon, token_str, len);
}

//--------------------------------------------
void WPN_AmmoType(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	if (tokenInt < AMMO_NONE || tokenInt >= AMMO_MAX)
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad Ammotype in external weapon data '%d'\n", tokenInt);
		return;
	}

	weaponData[wpnParms.weaponNum].ammoIndex = tokenInt;
}

//--------------------------------------------
void WPN_AmmoLowCnt(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	if (tokenInt < 0 || tokenInt > 200) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad Ammolowcount in external weapon data '%d'\n", tokenInt);
		return;
	}

	weaponData[wpnParms.weaponNum].ammoLow = tokenInt;
}

//--------------------------------------------
void WPN_FiringSnd(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: firingSnd too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].firingSnd, token_str, len);
}

//--------------------------------------------
void WPN_AltFiringSnd(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: altFiringSnd too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].altFiringSnd, token_str, len);
}

//--------------------------------------------
void WPN_StopSnd(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: stopSnd too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].stopSnd, token_str, len);
}

//--------------------------------------------
void WPN_ChargeSnd(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: chargeSnd too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].chargeSnd, token_str, len);
}

//--------------------------------------------
void WPN_AltChargeSnd(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: altChargeSnd too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].altChargeSnd, token_str, len);
}

//--------------------------------------------
void WPN_SelectSnd(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;

	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: selectSnd too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].selectSnd, token_str, len);
}

//--------------------------------------------
void WPN_FireTime(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	if (tokenInt < 0 || tokenInt > 10000) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad Firetime in external weapon data '%d'\n", tokenInt);
		return;
	}
	weaponData[wpnParms.weaponNum].fireTime = tokenInt;
}

//--------------------------------------------
void WPN_Range(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	if (tokenInt < 0 || tokenInt > 10000) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad Range in external weapon data '%d'\n", tokenInt);
		return;
	}

	weaponData[wpnParms.weaponNum].range = tokenInt;
}

//--------------------------------------------
void WPN_EnergyPerShot(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	if (tokenInt < 0 || tokenInt > 1000) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad EnergyPerShot in external weapon data '%d'\n", tokenInt);
		return;
	}
	weaponData[wpnParms.weaponNum].energyPerShot = tokenInt;
}

//--------------------------------------------
void WPN_AltFireTime(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	if (tokenInt < 0 || tokenInt > 10000) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad altFireTime in external weapon data '%d'\n", tokenInt);
		return;
	}
	weaponData[wpnParms.weaponNum].altFireTime = tokenInt;
}

//--------------------------------------------
void WPN_AltRange(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	if (tokenInt < 0 || tokenInt > 10000) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad AltRange in external weapon data '%d'\n", tokenInt);
		return;
	}

	weaponData[wpnParms.weaponNum].altRange = tokenInt;
}

//--------------------------------------------
void WPN_AltEnergyPerShot(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	if (tokenInt < 0 || tokenInt > 1000) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad AltEnergyPerShot in external weapon data '%d'\n", tokenInt);
		return;
	}
	weaponData[wpnParms.weaponNum].altEnergyPerShot = tokenInt;
}

//--------------------------------------------
void WPN_Ammo(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	if (!Q_stricmp(token_str, "AMMO_NONE"))
		wpnParms.ammoNum = AMMO_NONE;
	else if (!Q_stricmp(token_str, "AMMO_FORCE"))
		wpnParms.ammoNum = AMMO_FORCE;
	else if (!Q_stricmp(token_str, "AMMO_BLASTER"))
		wpnParms.ammoNum = AMMO_BLASTER;
	else if (!Q_stricmp(token_str, "AMMO_POWERCELL"))
		wpnParms.ammoNum = AMMO_POWERCELL;
	else if (!Q_stricmp(token_str, "AMMO_METAL_BOLTS"))
		wpnParms.ammoNum = AMMO_METAL_BOLTS;
	else if (!Q_stricmp(token_str, "AMMO_ROCKETS"))
		wpnParms.ammoNum = AMMO_ROCKETS;
	else if (!Q_stricmp(token_str, "AMMO_EMPLACED"))
		wpnParms.ammoNum = AMMO_EMPLACED;
	else if (!Q_stricmp(token_str, "AMMO_THERMAL"))
		wpnParms.ammoNum = AMMO_THERMAL;
	else if (!Q_stricmp(token_str, "AMMO_TRIPMINE"))
		wpnParms.ammoNum = AMMO_TRIPMINE;
	else if (!Q_stricmp(token_str, "AMMO_DETPACK"))
		wpnParms.ammoNum = AMMO_DETPACK;
	else
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad ammotype in external weapon data '%s'\n", token_str);
		wpnParms.ammoNum = 0;
	}
}

//--------------------------------------------
void WPN_AmmoIcon(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: ammoicon too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(ammoData[wpnParms.ammoNum].icon, token_str, len);
}

//--------------------------------------------
void WPN_AmmoMax(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	if (tokenInt < 0 || tokenInt > 1000)
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad Ammo Max in external weapon data '%d'\n", tokenInt);
		return;
	}
	ammoData[wpnParms.ammoNum].max = tokenInt;
}

//--------------------------------------------
void WPN_BarrelCount(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	if (tokenInt < 0 || tokenInt > 4)
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad Range in external weapon data '%d'\n", tokenInt);
		return;
	}

	weaponData[wpnParms.weaponNum].numBarrels = tokenInt;
}

//--------------------------------------------
static void WP_ParseWeaponParms(const char** hold_buf)
{
	size_t i;

	while (hold_buf)
	{
		const char* token = COM_ParseExt(hold_buf, qtrue);

		if (!Q_stricmp(token, "}")) // End of data for this weapon
			break;

		// Loop through possible parameters
		for (i = 0; i < numWpnParms; ++i)
		{
			if (!Q_stricmp(token, WpnParms[i].parmName))
			{
				WpnParms[i].func(hold_buf);
				break;
			}
		}

		if (i < numWpnParms) // Find parameter???
		{
			continue;
		}
		Com_Printf("^3WARNING: bad parameter in external weapon data '%s'\n", token);
	}
}

//--------------------------------------------
void WPN_MissileName(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: MissileName too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].missileMdl, token_str, len);
}

//--------------------------------------------
void WPN_AltMissileName(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: AltMissileName too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].alt_missileMdl, token_str, len);
}

//--------------------------------------------
void WPN_Missilehit_sound(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: Missilehit_sound too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].missilehit_sound, token_str, len);
}

//--------------------------------------------
void WPN_AltMissilehit_sound(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: AltMissilehit_sound too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].altmissilehit_sound, token_str, len);
}

//--------------------------------------------
void WPN_MissileSound(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: MissileSound too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].missileSound, token_str, len);
}

//--------------------------------------------
void WPN_AltMissileSound(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	int len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: AltMissileSound too long in external WEAPONS.DAT '%s'\n", token_str);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].alt_missileSound, token_str, len);
}

//--------------------------------------------
void WPN_MissileLightColor(const char** hold_buf)
{
	float tokenFlt;

	for (int i = 0; i < 3; ++i)
	{
		if (COM_ParseFloat(hold_buf, &tokenFlt))
		{
			SkipRestOfLine(hold_buf);
			continue;
		}

		if (tokenFlt < 0 || tokenFlt > 1)
		{
			gi.Printf(S_COLOR_YELLOW"WARNING: bad missilelightcolor in external weapon data '%f'\n", tokenFlt);
			continue;
		}
		weaponData[wpnParms.weaponNum].missileDlightColor[i] = tokenFlt;
	}
}

//--------------------------------------------
void WPN_AltMissileLightColor(const char** hold_buf)
{
	float tokenFlt;

	for (int i = 0; i < 3; ++i)
	{
		if (COM_ParseFloat(hold_buf, &tokenFlt))
		{
			SkipRestOfLine(hold_buf);
			continue;
		}

		if (tokenFlt < 0 || tokenFlt > 1)
		{
			gi.Printf(S_COLOR_YELLOW"WARNING: bad altmissilelightcolor in external weapon data '%f'\n", tokenFlt);
			continue;
		}
		weaponData[wpnParms.weaponNum].alt_missileDlightColor[i] = tokenFlt;
	}
}

//--------------------------------------------
void WPN_MissileLight(const char** hold_buf)
{
	float tokenFlt;

	if (COM_ParseFloat(hold_buf, &tokenFlt))
	{
		SkipRestOfLine(hold_buf);
	}

	if (tokenFlt < 0 || tokenFlt > 255) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad missilelight in external weapon data '%f'\n", tokenFlt);
	}
	weaponData[wpnParms.weaponNum].missileDlight = tokenFlt;
}

//--------------------------------------------
void WPN_AltMissileLight(const char** hold_buf)
{
	float tokenFlt;

	if (COM_ParseFloat(hold_buf, &tokenFlt))
	{
		SkipRestOfLine(hold_buf);
	}

	if (tokenFlt < 0 || tokenFlt > 255) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad altmissilelight in external weapon data '%f'\n", tokenFlt);
	}
	weaponData[wpnParms.weaponNum].alt_missileDlight = tokenFlt;
}

//--------------------------------------------
void WPN_FuncName(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}
	size_t len = strlen(token_str);

	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: FuncName '%s' too long in external WEAPONS.DAT\n", token_str);
	}

	for (func_t* s = funcs; s->name; s++)
	{
		if (!Q_stricmp(s->name, token_str))
		{
			// found it
			weaponData[wpnParms.weaponNum].func = (void*)s->func;
			return;
		}
	}
	gi.Printf(S_COLOR_YELLOW"WARNING: FuncName '%s' in external WEAPONS.DAT does not exist\n", token_str);
}

//--------------------------------------------
void WPN_AltFuncName(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}

	size_t len = strlen(token_str);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: AltFuncName '%s' too long in external WEAPONS.DAT\n", token_str);
	}

	for (func_t* s = funcs; s->name; s++)
	{
		if (!Q_stricmp(s->name, token_str))
		{
			// found it
			weaponData[wpnParms.weaponNum].altfunc = (void*)s->func;
			return;
		}
	}
	gi.Printf(S_COLOR_YELLOW"WARNING: AltFuncName %s in external WEAPONS.DAT does not exist\n", token_str);
}

//--------------------------------------------
void WPN_MuzzleEffect(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}
	size_t len = strlen(token_str);

	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: MuzzleEffect '%s' too long in external WEAPONS.DAT\n", token_str);
	}

	G_EffectIndex(token_str);
	Q_strncpyz(weaponData[wpnParms.weaponNum].mMuzzleEffect, token_str, len);
}

//--------------------------------------------
void WPN_AltMuzzleEffect(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}
	size_t len = strlen(token_str);

	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: AltMuzzleEffect '%s' too long in external WEAPONS.DAT\n", token_str);
	}

	G_EffectIndex(token_str);
	Q_strncpyz(weaponData[wpnParms.weaponNum].mAltMuzzleEffect, token_str, len);
}

//--------------------------------------------
void WPN_overloadmuzzleEffect(const char** hold_buf)
{
	const char* token_str;

	if (COM_ParseString(hold_buf, &token_str))
	{
		return;
	}
	size_t len = strlen(token_str);

	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: mOverloadMuzzleEffect '%s' too long in external WEAPONS.DAT\n", token_str);
	}

	G_EffectIndex(token_str);
	Q_strncpyz(weaponData[wpnParms.weaponNum].mOverloadMuzzleEffect, token_str, len);
}

//--------------------------------------------

void WPN_Damage(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	weaponData[wpnParms.weaponNum].damage = tokenInt;
}

//--------------------------------------------

void WPN_AltDamage(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	weaponData[wpnParms.weaponNum].altDamage = tokenInt;
}

//--------------------------------------------

void WPN_SplashDamage(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	weaponData[wpnParms.weaponNum].splashDamage = tokenInt;
}

//--------------------------------------------

void WPN_SplashRadius(const char** hold_buf)
{
	float tokenFlt;

	if (COM_ParseFloat(hold_buf, &tokenFlt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	weaponData[wpnParms.weaponNum].splashRadius = tokenFlt;
}

//--------------------------------------------

void WPN_AltSplashDamage(const char** hold_buf)
{
	int tokenInt;

	if (COM_ParseInt(hold_buf, &tokenInt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	weaponData[wpnParms.weaponNum].altSplashDamage = tokenInt;
}

//--------------------------------------------

void WPN_AltSplashRadius(const char** hold_buf)
{
	float tokenFlt;

	if (COM_ParseFloat(hold_buf, &tokenFlt))
	{
		SkipRestOfLine(hold_buf);
		return;
	}

	weaponData[wpnParms.weaponNum].altSplashRadius = tokenFlt;
}

//--------------------------------------------
void WP_ParseParms(const char* buffer)
{
	const char* hold_buf;

	hold_buf = buffer;
	COM_BeginParseSession();

	while (hold_buf)
	{
		const char* token = COM_ParseExt(&hold_buf, qtrue);

		if (!Q_stricmp(token, "{"))
		{
			WP_ParseWeaponParms(&hold_buf);
		}
	}

	COM_EndParseSession();
}

//--------------------------------------------
void WP_LoadWeaponParms()
{
	char* buffer = nullptr;

	const int len = gi.FS_ReadFile("ext_data/weapons.dat", reinterpret_cast<void**>(&buffer));

	if (len == -1)
	{
		Com_Error(ERR_FATAL, "Cannot find ext_data/weapons.dat!\n");
	}

	// initialise the data area
	memset(weaponData, 0, sizeof weaponData);

	// put in the default values, because backwards compatibility is awesome!
	for (int i = 0; i < WP_NUM_WEAPONS; i++)
	{
		weaponData[i].damage = defaultDamage[i];
		weaponData[i].altDamage = defaultAltDamage[i];
		weaponData[i].splashDamage = defaultSplashDamage[i];
		weaponData[i].altSplashDamage = defaultAltSplashDamage[i];
		weaponData[i].splashRadius = defaultSplashRadius[i];
		weaponData[i].altSplashRadius = defaultAltSplashRadius[i];
	}

	WP_ParseParms(buffer);

	gi.FS_FreeFile(buffer); //let go of the buffer
}