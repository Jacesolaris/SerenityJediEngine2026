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

#ifndef __CG_MEDIA_H_
#define __CG_MEDIA_H_

#include "../qcommon/q_shared.h"
#include "../rd-common/tr_types.h"
#include "../cgame/cg_local.h"
#include "g_shared.h"

constexpr auto NUM_CROSSHAIRS = 9;

using footstep_t = enum
{
	FOOTSTEP_STONEWALK,
	FOOTSTEP_STONERUN,
	FOOTSTEP_METALWALK,
	FOOTSTEP_METALRUN,
	FOOTSTEP_PIPEWALK,
	FOOTSTEP_PIPERUN,
	FOOTSTEP_NORMAL,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,
	FOOTSTEP_WADE,
	FOOTSTEP_SWIM,
	FOOTSTEP_SNOWWALK,
	FOOTSTEP_SNOWRUN,
	FOOTSTEP_SANDWALK,
	FOOTSTEP_SANDRUN,
	FOOTSTEP_GRASSWALK,
	FOOTSTEP_GRASSRUN,
	FOOTSTEP_DIRTWALK,
	FOOTSTEP_DIRTRUN,
	FOOTSTEP_MUDWALK,
	FOOTSTEP_MUDRUN,
	FOOTSTEP_GRAVELWALK,
	FOOTSTEP_GRAVELRUN,
	FOOTSTEP_RUGWALK,
	FOOTSTEP_RUGRUN,
	FOOTSTEP_WOODWALK,
	FOOTSTEP_WOODRUN,
	FOOTSTEP_SBDWALK,
	FOOTSTEP_SBDRUN,

	FOOTSTEP_TOTAL
};

constexpr auto ICON_WEAPONS = 0;
constexpr auto ICON_FORCE = 1;
constexpr auto ICON_INVENTORY = 2;

constexpr auto MAX_HUD_TICS = 4;
constexpr auto MAX_SJEHUD_TICS = 15;
constexpr auto MAX_DATAPADTICS = 14;

using forceTicPos_t = struct forceTicPos_s
{
	int x;
	int y;
	int width;
	int height;
	char* file;
	qhandle_t tic;
};
extern forceTicPos_t forceTicPos[];
extern forceTicPos_t JK2forceTicPos[];
extern forceTicPos_t ammoTicPos[];
extern forceTicPos_t JK2ammoTicPos[];

using HUDMenuItem_t = struct HUDMenuItem_s
{
	const char* menuName;
	const char* itemName;
	int xPos;
	int yPos;
	int width;
	int height;
	vec4_t color;
	qhandle_t background;
};

extern HUDMenuItem_t healthTics[];
extern HUDMenuItem_t armorTics[];
extern HUDMenuItem_t ammoTics[];
extern HUDMenuItem_t forceTics[];
extern HUDMenuItem_t otherHUDBits[];
extern HUDMenuItem_t mishapTics[];
extern HUDMenuItem_t sabfatticS[];

using otherhudbits_t = enum
{
	OHB_HEALTHAMOUNT = 0,
	OHB_HEALTHAMOUNTALT,
	OHB_ARMORAMOUNT,
	OHB_BLOCKAMOUNT,
	OHB_JK2BLOCKAMOUNT,
	OHB_FORCEAMOUNT,
	OHB_JK2FORCEAMOUNT,
	OHB_AMMOAMOUNT,
	OHB_SABERSTYLE_STRONG,
	OHB_SABERSTYLE_MEDIUM,
	OHB_SABERSTYLE_FAST,
	OHB_SABERSTYLE_TAVION,
	OHB_SABERSTYLE_DESANN,
	OHB_SABERSTYLE_STAFF,
	OHB_SABERSTYLE_DUAL,
	OHB_SBD_BLASTER,
	OHB_WRIST,
	OHB_STUN_BATON,
	OHB_BRIAR_PISTOL,
	OHB_BLASTER_PISTOL,
	OHB_BLASTER,
	OHB_BOWCASTER,
	OHB_CONCUSSION,
	OHB_DEMP2,
	OHB_DETPACK,
	OHB_DISRUPTOR,
	OHB_FLACHETTE,
	OHB_REPEATER,
	OHB_THERMAL,
	OHB_ROCKET,
	OHB_TRIPMINE,
	OHB_MELEE,
	OHB_DEKA,
	OHB_SCANLINE_LEFT,
	OHB_SCANLINE_RIGHT,
	OHB_FRAME_LEFT,
	OHB_FRAME_RIGHT,
	OHB_MBLOCKINGMODE,
	OHB_BLOCKINGMODE,
	OHB_NOTBLOCKING,
	OHB_JK2MBLOCKINGMODE,
	OHB_JK2BLOCKINGMODE,
	OHB_JK2NOTBLOCKING,
	OHB_MAX
};

constexpr auto NUM_CHUNK_MODELS = 4;

enum
{
	CHUNK_METAL1 = 0,
	CHUNK_METAL2,
	CHUNK_ROCK1,
	CHUNK_ROCK2,
	CHUNK_ROCK3,
	CHUNK_CRATE1,
	CHUNK_CRATE2,
	CHUNK_WHITE_METAL,
	NUM_CHUNK_TYPES
};

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
using cgMedia_t = struct
{
	qhandle_t charsetShader;
	qhandle_t whiteShader;

	qhandle_t crosshairShader[NUM_CROSSHAIRS];
	qhandle_t backTileShader;
	//qhandle_t	noammoShader;

	qhandle_t numberShaders[11];
	qhandle_t load_SerenitySaberSystems;
	qhandle_t smallnumberShaders[11];
	qhandle_t chunkyNumberShaders[11];

	qhandle_t loadTick;
	qhandle_t loadTickCap;

	//Radar
	qhandle_t radarShader;
	qhandle_t radarScanShader;

	qhandle_t siegeItemShader;
	qhandle_t mAutomapPlayerIcon;
	qhandle_t mAutomapRocketIcon;

	qhandle_t radarIcons[MAX_ICONS];

	//			HUD artwork
	int currentBackground;
	qhandle_t weaponbox;
	qhandle_t weaponIconBackground;
	qhandle_t weaponProngsOff;
	qhandle_t weaponProngsOn;
	qhandle_t forceIconBackground;
	qhandle_t forceProngsOn;
	qhandle_t inventoryIconBackground;
	qhandle_t inventoryProngsOn;
	qhandle_t ladyLuckHealthShader;
	qhandle_t turretComputerOverlayShader;
	qhandle_t turretCrossHairShader;

	int currentDataPadIconBackground;

	//Chunks
	qhandle_t chunkModels[NUM_CHUNK_TYPES][4];
	sfxHandle_t chunkSound;
	sfxHandle_t grateSound;
	sfxHandle_t rockBreakSound;
	sfxHandle_t rockBounceSound[2];
	sfxHandle_t metalBounceSound[2];
	sfxHandle_t glassChunkSound;
	sfxHandle_t crateBreakSound[2];

	// Saber shaders
	//-----------------------------
	qhandle_t forceCoronaShader;
	qhandle_t saberBlurShader;
	qhandle_t swordTrailShader;
	qhandle_t yellowDroppedSaberShader; // glow

	qhandle_t redSaberGlowShader;
	qhandle_t redSaberCoreShader;
	qhandle_t orangeSaberGlowShader;
	qhandle_t orangeSaberCoreShader;
	qhandle_t yellowSaberGlowShader;
	qhandle_t yellowSaberCoreShader;
	qhandle_t greenSaberGlowShader;
	qhandle_t greenSaberCoreShader;
	qhandle_t blueSaberGlowShader;
	qhandle_t blueSaberCoreShader;
	qhandle_t purpleSaberGlowShader;
	qhandle_t purpleSaberCoreShader;
	qhandle_t rgbSaberGlowShader;
	qhandle_t rgbSaberCoreShader;
	qhandle_t rgbSaberCore2Shader;
	qhandle_t blackSaberGlowShader;
	qhandle_t blackSaberCoreShader;
	qhandle_t blackSaberBlurShader;
	qhandle_t blackSaberTrail;
	qhandle_t sfxSaberTrailShader;
	qhandle_t sfxSaberBladeShader;
	qhandle_t sfxSaberBlade2Shader;
	qhandle_t sfxSaberEndShader;
	qhandle_t sfxSaberEnd2Shader;
	qhandle_t redIgniteFlare;
	qhandle_t greenIgniteFlare;
	qhandle_t purpleIgniteFlare;
	qhandle_t blueIgniteFlare;
	qhandle_t orangeIgniteFlare;
	qhandle_t yellowIgniteFlare;
	qhandle_t rgbTFASaberCoreShader;
	qhandle_t unstableBlurShader;
	qhandle_t unstableRedSaberCoreShader;
	// Custom saber glow, blade & dlight color code
	qhandle_t customSaberGlowShader;
	qhandle_t customSaberCoreShader;
	qhandle_t limeSaberGlowShader;
	qhandle_t limeSaberCoreShader;

	//Original Trilogy Sabers
	qhandle_t otSaberCoreShader;
	qhandle_t redOTGlowShader;
	qhandle_t orangeOTGlowShader;
	qhandle_t yellowOTGlowShader;
	qhandle_t greenOTGlowShader;
	qhandle_t blueOTGlowShader;
	qhandle_t purpleOTGlowShader;

	//Episode I Sabers
	qhandle_t ep1SaberCoreShader;
	qhandle_t redEp1GlowShader;
	qhandle_t orangeEp1GlowShader;
	qhandle_t yellowEp1GlowShader;
	qhandle_t greenEp1GlowShader;
	qhandle_t blueEp1GlowShader;
	qhandle_t purpleEp1GlowShader;

	//Episode II Sabers
	qhandle_t ep2SaberCoreShader;
	qhandle_t whiteIgniteFlare;
	qhandle_t blackIgniteFlare;
	qhandle_t redEp2GlowShader;
	qhandle_t orangeEp2GlowShader;
	qhandle_t yellowEp2GlowShader;
	qhandle_t greenEp2GlowShader;
	qhandle_t blueEp2GlowShader;
	qhandle_t purpleEp2GlowShader;

	//Episode III Sabers
	qhandle_t ep3SaberCoreShader;
	qhandle_t whiteIgniteFlare02;
	qhandle_t blackIgniteFlare02;
	qhandle_t redEp3GlowShader;
	qhandle_t orangeEp3GlowShader;
	qhandle_t yellowEp3GlowShader;
	qhandle_t greenEp3GlowShader;
	qhandle_t blueEp3GlowShader;
	qhandle_t purpleEp3GlowShader;

	qhandle_t explosionModel;
	qhandle_t surfaceExplosionShader;

	qhandle_t halfShieldModel;
	qhandle_t halfShieldShader;

	qhandle_t solidWhiteShader;
	qhandle_t electricBodyShader;
	qhandle_t electricBody2Shader;
	qhandle_t refractShader;
	qhandle_t boltShader;

	qhandle_t shockBodyShader;
	qhandle_t shockBody2Shader;
	qhandle_t shockShader;

	// Disruptor zoom graphics
	qhandle_t disruptorMask;
	qhandle_t disruptorInsert;
	qhandle_t disruptorLight;
	qhandle_t disruptorInsertTick;

	// Binocular graphics
	qhandle_t binocularCircle;
	qhandle_t binocularMask;
	qhandle_t binocularArrow;
	qhandle_t binocularTri;
	qhandle_t binocularStatic;
	qhandle_t binocularOverlay;

	// LA Goggles graphics
	qhandle_t laGogglesStatic;
	qhandle_t laGogglesMask;
	qhandle_t laGogglesSideBit;
	qhandle_t laGogglesBracket;
	qhandle_t laGogglesArrow;

	// wall mark shaders
	qhandle_t scavMarkShader;
	qhandle_t rivetMarkShader;

	qhandle_t shadowMarkShader;
	qhandle_t wakeMarkShader;
	qhandle_t fsrMarkShader;
	qhandle_t fslMarkShader;
	qhandle_t fshrMarkShader;
	qhandle_t fshlMarkShader;

	qhandle_t damageBlendBlobShader;

	qhandle_t HUDblockpoint1;
	qhandle_t HUDblockpoint2;

	qhandle_t HUDblockpointMB1;
	qhandle_t HUDblockpointMB2;

	qhandle_t HUDRightFrame;
	qhandle_t HUDInnerRight;
	qhandle_t HUDInnerLeft;
	qhandle_t HUDLeftFrame;
	qhandle_t HUDHealth;
	qhandle_t HUDHealthTic;
	qhandle_t HUDArmor1;
	qhandle_t HUDArmor2;
	qhandle_t HUDSaberStyleFast;
	qhandle_t HUDSaberStyleMed;
	qhandle_t HUDSaberStyleStrong;
	qhandle_t HUDSaberStyleDesann;
	qhandle_t HUDSaberStyleTavion;
	qhandle_t HUDSaberStyleStaff;
	qhandle_t HUDSaberStyleDuels;

	//JK2 HUD ONLY

	qhandle_t JK2HUDArmor1;
	qhandle_t JK2HUDArmor2;
	qhandle_t JK2HUDHealth;
	qhandle_t JK2HUDHealthTic;
	qhandle_t JK2HUDInnerLeft;
	qhandle_t JK2HUDLeftFrame;

	qhandle_t JK2HUDSaberStyleFast;
	qhandle_t JK2HUDSaberStyleMed;
	qhandle_t JK2HUDSaberStyleStrong;
	qhandle_t JK2HUDSaberStyleDesann;
	qhandle_t JK2HUDSaberStyleTavion;
	qhandle_t JK2HUDSaberStyleStaff;
	qhandle_t JK2HUDSaberStyleDuels;

	qhandle_t JK2HUDRightFrame;
	qhandle_t JK2HUDInnerRight;

	// fonts...
	//
	qhandle_t qhFontSmall;
	qhandle_t qhFontMedium;

	// special effects models / etc.
	qhandle_t personalShieldShader;
	qhandle_t cloakedShader;

	// Interface media
	qhandle_t ammoslider;
	qhandle_t emplacedHealthBarShader;
	qhandle_t atstHealthBarShader;

	qhandle_t dataPadFrame;
	qhandle_t DPForcePowerOverlay;

	qhandle_t bdecal_bodyburn1;
	qhandle_t bdecal_burnmark1;
	qhandle_t bdecal_saberglowmark;
	qhandle_t bdecal_saberglow;

	qhandle_t messageLitOn;
	qhandle_t messageLitOff;
	qhandle_t messageObjCircle;

	qhandle_t batteryChargeShader;
	qhandle_t useableHint;

	qhandle_t hackerIconShader;

	qhandle_t levelLoad;

	//new stuff for Jedi Academy
	//force power icons
	//	qhandle_t	forcePowerIcons[NUM_FORCE_POWERS];
	qhandle_t rageRecShader;
	qhandle_t playerShieldDamage;
	qhandle_t forceSightBubble;
	qhandle_t forceShell;
	qhandle_t sightShell;
	qhandle_t drainShader;

	// sounds
	sfxHandle_t disintegrateSound;
	sfxHandle_t disintegrate2Sound;

	sfxHandle_t grenadeBounce1;
	sfxHandle_t grenadeBounce2;

	sfxHandle_t flechetteStickSound;
	sfxHandle_t detPackStickSound;
	sfxHandle_t tripMineStickSound;

	sfxHandle_t selectSound;
	sfxHandle_t selectSound2;
	sfxHandle_t overchargeSlowSound;
	sfxHandle_t overchargeFastSound;
	sfxHandle_t overchargeLoopSound;
	sfxHandle_t overchargeEndSound;

	sfxHandle_t footsteps[FOOTSTEP_TOTAL][4];

	sfxHandle_t hit_sound;
	sfxHandle_t hitTeamSound;
	sfxHandle_t impressiveSound;
	sfxHandle_t excellentSound;
	sfxHandle_t deniedSound;
	sfxHandle_t humiliationSound;
	sfxHandle_t defendSound;

	sfxHandle_t atstWaterInSound[2];
	sfxHandle_t atstWaterOutSound[2];

	sfxHandle_t noAmmoSound;

	sfxHandle_t landSound;
	sfxHandle_t rollSound;
	sfxHandle_t messageLitSound;

	sfxHandle_t batteryChargeSound;

	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;

	sfxHandle_t lavaInSound;
	sfxHandle_t lavaOutSound;
	sfxHandle_t lavaUnSound;

	sfxHandle_t noforceSound;

	sfxHandle_t overload;

	sfxHandle_t overheatgun;

	// Zoom
	sfxHandle_t zoomStart;
	sfxHandle_t zoomLoop;
	sfxHandle_t zoomEnd;
	sfxHandle_t disruptorZoomLoop;

	//new stuff for Jedi Academy
	sfxHandle_t drainSound;

	sfxHandle_t hitmarkerSound;
	qhandle_t hitmarkerGraphic;

	sfxHandle_t destructionSound;

	sfxHandle_t blastSound;

	sfxHandle_t strikeSound;

	qhandle_t disruptorShader;

	// medals shown during gameplay
	qhandle_t medalImpressive;
	qhandle_t medalExcellent;
	qhandle_t medalGauntlet;
	qhandle_t medalDefend;
	qhandle_t medalAssist;
	qhandle_t medalCapture;
	qhandle_t balloonShader;
	qhandle_t meditateShader;

	qhandle_t	ysaliredShader;
	qhandle_t	ysaliblueShader;
	qhandle_t	ysalimariShader;
	qhandle_t	boonShader;
	qhandle_t	endarkenmentShader;
	qhandle_t	enlightenmentShader;
	qhandle_t	invulnerabilityShader;
	qhandle_t	galakShader;
	qhandle_t	barriershader;
};

// Stored FX handles
//--------------------
using cgEffects_t = struct
{
	// BRYAR PISTOL
	fxHandle_t briar_pistolShotEffect;
	fxHandle_t briar_pistolNPCShotEffect;
	fxHandle_t briar_pistolPowerupShotEffect;
	fxHandle_t briar_pistolWallImpactEffect;
	fxHandle_t briar_pistolWallImpactEffect2;
	fxHandle_t briar_pistolWallImpactEffect3;
	fxHandle_t briar_pistolFleshImpactEffect;
	fxHandle_t briar_pistolDroidImpactEffect;

	fxHandle_t bryarShotEffect;
	fxHandle_t bryarPowerupShotEffect;
	fxHandle_t bryarWallImpactEffect;
	fxHandle_t bryarWallImpactEffect2;
	fxHandle_t bryarWallImpactEffect3;
	fxHandle_t bryarFleshImpactEffect;

	fxHandle_t bryaroldShotEffect;
	fxHandle_t bryaroldPowerupShotEffect;
	fxHandle_t bryaroldWallImpactEffect;
	fxHandle_t bryaroldWallImpactEffect2;
	fxHandle_t bryaroldWallImpactEffect3;
	fxHandle_t bryaroldFleshImpactEffect;

	// BLASTER
	fxHandle_t blasterShotEffect;
	fxHandle_t blasterOverchargeEffect;
	fxHandle_t blasterWallImpactEffect;
	fxHandle_t blasterFleshImpactEffect;
	fxHandle_t blasterDroidImpactEffect;

	// BOWCASTER
	fxHandle_t bowcasterShotEffect;
	fxHandle_t bowcasterBounceEffect;
	fxHandle_t bowcasterImpactEffect;

	// REPEATER
	fxHandle_t cloneShotEffect;
	fxHandle_t repeaterProjectileEffect;
	fxHandle_t repeaterAltProjectileEffect;
	fxHandle_t repeaterWallImpactEffect;
	fxHandle_t repeaterFleshImpactEffect;
	fxHandle_t repeaterAltWallImpactEffect;

	// DEMP2
	fxHandle_t demp2ProjectileEffect;
	fxHandle_t demp2WallImpactEffect;
	fxHandle_t demp2FleshImpactEffect;

	// FLECHETTE
	fxHandle_t flechetteShotEffect;
	fxHandle_t flechetteAltShotEffect;
	fxHandle_t flechetteShotDeathEffect;
	fxHandle_t flechetteFleshImpactEffect;
	fxHandle_t flechetteRicochetEffect;

	// DROIDEKA
	fxHandle_t DroidekaShotEffect;

	fxHandle_t tripminelaser;
	fxHandle_t tripminelaserImpactGlow;
	fxHandle_t tripmineglowBit;

	//FORCE
	fxHandle_t forceConfusion;
	fxHandle_t forceLightning;
	fxHandle_t forceLightningWide;
	fxHandle_t redlightningwide;
	fxHandle_t redlightning;
	fxHandle_t greenlightningwide;
	fxHandle_t greenlightning;
	fxHandle_t purplelightningwide;
	fxHandle_t purplelightning;
	fxHandle_t forceInvincibility;
	fxHandle_t forceHeal;

	fxHandle_t yellowlightningwide;
	fxHandle_t yellowlightning;

	//new stuff for Jedi Academy
	fxHandle_t forceDrain;
	fxHandle_t forceDrainWide;
	fxHandle_t forceDrained;

	fxHandle_t destructionProjectile;
	fxHandle_t destructionHit;

	fxHandle_t blastProjectile;
	fxHandle_t blastHit;

	fxHandle_t strikeProjectile;
	fxHandle_t strikeHit;

	fxHandle_t mHyperspaceStars;

	//footstep effects
	fxHandle_t footstepMud;
	fxHandle_t footstepSand;
	fxHandle_t footstepSnow;
	fxHandle_t footstepGravel;
	//landing effects
	fxHandle_t landingMud;
	fxHandle_t landingSand;
	fxHandle_t landingDirt;
	fxHandle_t landingSnow;
	fxHandle_t landingGravel;
	fxHandle_t landingLava;
	fxHandle_t mSaberFriction;
	fxHandle_t mSaberLock;

	fxHandle_t mDisruptorDeathSmoke;
};

// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
constexpr auto STRIPED_LEVELNAME_VARIATIONS = 3;
// sigh, to cope with levels that use text from >1 SP file (plus 1 for common);
using cgs_t = struct
{
	gameState_t gameState; // gamestate from server
	glconfig_t glconfig; // rendering configuration

	int serverCommandSequence; // reliable command stream counter

	// parsed from serverinfo
	int dmflags;
	int teamflags;
	int timelimit;
	int maxclients;
	char mapname[MAX_QPATH];
	char stripLevelName[STRIPED_LEVELNAME_VARIATIONS][MAX_QPATH];

	//
	// locally derived information from gamestate
	//
	qhandle_t model_draw[MAX_MODELS];
	sfxHandle_t sound_precache[MAX_SOUNDS];
	// Ghoul2 start
	qhandle_t skins[MAX_CHARSKINS];

	// Ghoul2 end

	int numInlineModels;
	qhandle_t inlineDrawModel[MAX_SUBMODELS];
	vec3_t inlineModelMidpoints[MAX_SUBMODELS];

	clientInfo_t clientinfo[MAX_CLIENTS];

	// media
	cgMedia_t media;

	// effects
	cgEffects_t effects;

	float widthRatioCoef;
};

extern cgs_t cgs;

#endif //__CG_MEDIA_H_
