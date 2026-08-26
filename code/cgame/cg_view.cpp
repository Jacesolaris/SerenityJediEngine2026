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

// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering

// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

#include "cg_media.h"
#include "FxScheduler.h"
#include "../game/wp_saber.h"
#include "../game/g_vehicles.h"
#include "cgame/cg_local.h"

#define MASK_CAMERACLIP (MASK_SOLID)
constexpr auto CAMERA_SIZE = 4;

float cg_zoomFov;
extern qboolean CG_OnMovingPlat(const playerState_t* ps);
extern Vehicle_t* G_IsRidingVehicle(const gentity_t* pEnt);

extern int g_crosshairSameEntTime;
extern int g_crosshairEntNum;

/*
=============================================================================

  MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.

=============================================================================
*/

/*
Ghoul2 Insert Start
*/

/*
=================
CG_TestModel_f

Creates an entity in front of the current position, which
can then be moved around
=================
*/
void CG_TestG2Model_f()
{
	vec3_t angles{};

	memset(&cg.testModelEntity, 0, sizeof cg.testModelEntity);
	const auto ghoul2 = new CGhoul2Info_v;
	cg.testModelEntity.ghoul2 = ghoul2;
	if (cgi_Argc() < 2)
	{
		return;
	}

	Q_strncpyz(cg.testModelName, CG_Argv(1), MAX_QPATH);
	cg.testModelEntity.hModel = cgi_R_RegisterModel(cg.testModelName);

	cg.testModel = gi.G2API_InitGhoul2Model(*cg.testModelEntity.ghoul2, cg.testModelName, cg.testModelEntity.hModel,
		NULL_HANDLE, NULL_HANDLE, 0, 0);
	cg.testModelEntity.radius = 100.0f;

	if (cgi_Argc() == 3)
	{
		cg.testModelEntity.backlerp = atof(CG_Argv(2));
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if (!cg.testModelEntity.hModel)
	{
		CG_Printf("Can't register model\n");
		return;
	}

	VectorMA(cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin);

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefViewAngles[1];
	angles[ROLL] = 0;

	AnglesToAxis(angles, cg.testModelEntity.axis);
}

void CG_ListModelSurfaces_f()
{
	CGhoul2Info_v& ghoul2 = *cg.testModelEntity.ghoul2;

	gi.G2API_ListSurfaces(&ghoul2[cg.testModel]);
}

void CG_ListModelBones_f()
{
	// test to see if we got enough args
	if (cgi_Argc() < 2)
	{
		return;
	}
	CGhoul2Info_v& ghoul2 = *cg.testModelEntity.ghoul2;

	gi.G2API_ListBones(&ghoul2[cg.testModel], atoi(CG_Argv(1)));
}

void CG_TestModelSurfaceOnOff_f()
{
	// test to see if we got enough args
	if (cgi_Argc() < 3)
	{
		return;
	}
	CGhoul2Info_v& ghoul2 = *cg.testModelEntity.ghoul2;

	gi.G2API_SetSurfaceOnOff(&ghoul2[cg.testModel], CG_Argv(1), atoi(CG_Argv(2)));
}

void CG_TestModelSetAnglespre_f()
{
	vec3_t angles{};

	if (cgi_Argc() < 3)
	{
		return;
	}
	CGhoul2Info_v& ghoul2 = *cg.testModelEntity.ghoul2;

	angles[0] = atof(CG_Argv(2));
	angles[1] = atof(CG_Argv(3));
	angles[2] = atof(CG_Argv(4));
	gi.G2API_SetBoneAngles(&ghoul2[cg.testModel], CG_Argv(1), angles, BONE_ANGLES_PREMULT, POSITIVE_X, POSITIVE_Z,
		POSITIVE_Y, nullptr, 0, 0);
}

void CG_TestModelSetAnglespost_f()
{
	vec3_t angles{};

	if (cgi_Argc() < 3)
	{
		return;
	}
	CGhoul2Info_v& ghoul2 = *cg.testModelEntity.ghoul2;

	angles[0] = atof(CG_Argv(2));
	angles[1] = atof(CG_Argv(3));
	angles[2] = atof(CG_Argv(4));
	gi.G2API_SetBoneAngles(&ghoul2[cg.testModel], CG_Argv(1), angles, BONE_ANGLES_POSTMULT, POSITIVE_X, POSITIVE_Z,
		POSITIVE_Y, nullptr, 0, 0);
}

void CG_TestModelAnimate_f()
{
	char boneName[100];
	CGhoul2Info_v& ghoul2 = *cg.testModelEntity.ghoul2;

	strcpy(boneName, CG_Argv(1));
	gi.G2API_SetBoneAnim(&ghoul2[cg.testModel], boneName, atoi(CG_Argv(2)), atoi(CG_Argv(3)), BONE_ANIM_OVERRIDE_LOOP,
		atof(CG_Argv(4)), cg.time, -1, -1);
}

/*
Ghoul2 Insert End
*/

/*
=================
CG_TestModel_f

Creates an entity in front of the current position, which
can then be moved around
=================
*/
void CG_TestModel_f()
{
	vec3_t angles{};

	memset(&cg.testModelEntity, 0, sizeof cg.testModelEntity);
	if (cgi_Argc() < 2)
	{
		return;
	}

	Q_strncpyz(cg.testModelName, CG_Argv(1), MAX_QPATH);
	cg.testModelEntity.hModel = cgi_R_RegisterModel(cg.testModelName);

	if (cgi_Argc() == 3)
	{
		cg.testModelEntity.backlerp = atof(CG_Argv(2));
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if (!cg.testModelEntity.hModel)
	{
		CG_Printf("Can't register model\n");
		return;
	}

	VectorMA(cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin);

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefViewAngles[1];
	angles[ROLL] = 0;

	AnglesToAxis(angles, cg.testModelEntity.axis);
}

void CG_TestModelNextFrame_f()
{
	cg.testModelEntity.frame++;
	CG_Printf("frame %i\n", cg.testModelEntity.frame);
}

void CG_TestModelPrevFrame_f()
{
	cg.testModelEntity.frame--;
	if (cg.testModelEntity.frame < 0)
	{
		cg.testModelEntity.frame = 0;
	}
	CG_Printf("frame %i\n", cg.testModelEntity.frame);
}

void CG_TestModelNextSkin_f()
{
	cg.testModelEntity.skinNum++;
	CG_Printf("skin %i\n", cg.testModelEntity.skinNum);
}

void CG_TestModelPrevSkin_f()
{
	cg.testModelEntity.skinNum--;
	if (cg.testModelEntity.skinNum < 0)
	{
		cg.testModelEntity.skinNum = 0;
	}
	CG_Printf("skin %i\n", cg.testModelEntity.skinNum);
}

static void CG_AddTestModel()
{
	cgi_R_AddRefEntityToScene(&cg.testModelEntity);
}

//============================================================================

/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/
void CG_CalcVrect()
{
	constexpr int size = 100;

	cg.refdef.width = cgs.glconfig.vidWidth * size * 0.01;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight * size * 0.01;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width) * 0.5;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height) * 0.5;
}

//==============================================================================
//==============================================================================
constexpr auto CAMERA_DAMP_INTERVAL = 50;
constexpr auto CAMERA_DAMP_INTERVAL_AIMING = 30;

constexpr auto CAMERA_CROUCH_NUDGE = 6;

static vec3_t cameramins = { -CAMERA_SIZE, -CAMERA_SIZE, -CAMERA_SIZE };
static vec3_t cameramaxs = { CAMERA_SIZE, CAMERA_SIZE, CAMERA_SIZE };
vec3_t camerafwd, cameraup, camerahorizdir;

vec3_t cameraFocusAngles, cameraFocusLoc;
vec3_t cameraIdealTarget, cameraIdealLoc;
vec3_t cameraCurTarget = { 0, 0, 0 }, cameraCurLoc = { 0, 0, 0 };
vec3_t cameraOldLoc = { 0, 0, 0 }, cameraNewLoc = { 0, 0, 0 };
int cameraLastFrame = 0;

float cameraLastYaw = 0;
float cameraStiffFactor = 0.0f;

/*
===============
Notes on the camera viewpoint in and out...

cg.refdef.vieworg
--at the start of the function holds the player actor's origin (center of player model).
--it is set to the final view location of the camera at the end of the camera code.
cg.refdefViewAngles
--at the start holds the client's view angles
--it is set to the final view angle of the camera at the end of the camera code.

===============
*/

/*
===============
CG_CalcIdealThirdPersonViewTarget

===============
*/
static void CG_CalcIdealThirdPersonViewTarget()
{
	// Ensure snapshot exists before dereferencing.
	if (cg.snap == NULL)
	{
		return;
	}
	// Initialize IdealTarget
	const auto uses_view_entity = static_cast<qboolean>(cg.snap->ps.viewEntity && cg.snap->ps.viewEntity < ENTITYNUM_WORLD);
	VectorCopy(cg.refdef.vieworg, cameraFocusLoc);

	if (uses_view_entity)
	{
		const gentity_t* gent = &g_entities[cg.snap->ps.viewEntity];

		if (gent->client &&
			(gent->client->NPC_class == CLASS_GONK
				|| gent->client->NPC_class == CLASS_INTERROGATOR
				|| gent->client->NPC_class == CLASS_SENTRY
				|| gent->client->NPC_class == CLASS_PROBE
				|| gent->client->NPC_class == CLASS_MOUSE
				|| gent->client->NPC_class == CLASS_R2D2
				|| gent->client->NPC_class == CLASS_R5D2))
		{
			// Droids use a generic offset
			cameraFocusLoc[2] += 4;
			VectorCopy(cameraFocusLoc, cameraIdealTarget);
			return;
		}

		if (gent->client && (gent->client->ps.pm_flags & PMF_DUCKED && gent->client->ps.groundEntityNum != ENTITYNUM_NONE))
		{// if the player is crouched and on the ground, nudge the camera down a bit so it doesn't clip into the ground
			cameraFocusLoc[2] -= CAMERA_CROUCH_NUDGE * 4;
		}

		if (cg.snap
			&& cg.snap->ps.eFlags & EF_MEDITATING)
		{
			cameraFocusLoc[2] -= CAMERA_CROUCH_NUDGE * 4;
		}
	}

	// Add in the new viewheight
	cameraFocusLoc[2] += cg.predictedPlayerState.viewheight;
	if (cg.snap
		&& cg.snap->ps.eFlags & EF_HELD_BY_SAND_CREATURE)
	{
		VectorCopy(cameraFocusLoc, cameraIdealTarget);
		cameraIdealTarget[2] += 192;
	}
	else if (cg.snap
		&& cg.snap->ps.eFlags & EF_HELD_BY_WAMPA)
	{
		VectorCopy(cameraFocusLoc, cameraIdealTarget);
		cameraIdealTarget[2] -= 48;
	}
	else if (cg.overrides.active & CG_OVERRIDE_3RD_PERSON_VOF)
	{
		// Add in a vertical offset from the viewpoint, which puts the actual target above the head, regardless of angle.
		VectorCopy(cameraFocusLoc, cameraIdealTarget);
		cameraIdealTarget[2] += cg.overrides.thirdPersonVertOffset;
	}
	else if (cg.renderingThirdPerson && cg.predictedPlayerState.communicatingflags & (1 << CF_SABERLOCKING) && cg_saberLockCinematicCamera.integer)
	{
		VectorCopy(cameraFocusLoc, cameraIdealTarget);
		cameraIdealTarget[2] -= 15.5f;
	}
	else if (cg.renderingThirdPerson &&
		(cg.predictedPlayerState.communicatingflags & (1 << CF_AIMINGGUN)) &&
		cg_AimingCinematicCamera.integer)
	{
		vec3_t forward;
		AngleVectors(cg.refdefViewAngles, forward, NULL, NULL);

		// Shorter forward distance so we don't zoom into the back of the head
		VectorMA(cameraFocusLoc, 64.0f, forward, cameraIdealTarget);

		// Shoulder camera tuning
		cg.overrides.thirdPersonAngle = 10.0f;   // yaw inward
		cg.overrides.thirdPersonPitchOffset = -2.0f;   // slight downward pitch
		cg.overrides.thirdPersonHorzOffset = -20.0f;  // softer shoulder shift
	}
	else
	{
		// Add in a vertical offset from the viewpoint, which puts the actual target above the head, regardless of angle.
		VectorCopy(cameraFocusLoc, cameraIdealTarget);
		cameraIdealTarget[2] += cg_thirdPersonVertOffset.value;
	}

	// Now, if the player is crouching, do a little special tweak.  The problem is that the player's head is way out of his bbox.
	if (cg.predictedPlayerState.pm_flags & PMF_DUCKED && cg.predictedPlayerState.groundEntityNum != ENTITYNUM_NONE)
	{ // if the player is crouched and on the ground, nudge the camera up a bit so it doesn't clip into the ground
		vec3_t nudgepos;
		trace_t trace;

		VectorCopy(cameraFocusLoc, nudgepos);

		nudgepos[2] += CAMERA_CROUCH_NUDGE;

		CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, nudgepos,
			uses_view_entity ? cg.snap->ps.viewEntity : cg.predictedPlayerState.clientNum,
			MASK_CAMERACLIP);

		if (trace.fraction < 1.0f)
		{
			VectorCopy(trace.endpos, cameraFocusLoc);
		}
		else
		{
			VectorCopy(nudgepos, cameraFocusLoc);
		}
	}

	// Now, if the player is crouching, do a little special tweak.  The problem is that the player's head is way out of his bbox.
	if (cg.snap
		&& cg.snap->ps.eFlags & EF_MEDITATING)
	{
		// Nudge to focus location up a tad.
		vec3_t nudgepos;
		trace_t trace;

		VectorCopy(cameraFocusLoc, nudgepos);
		nudgepos[2] += CAMERA_CROUCH_NUDGE;
		CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, nudgepos,
			uses_view_entity ? cg.snap->ps.viewEntity : cg.predictedPlayerState.clientNum, MASK_CAMERACLIP);
		if (trace.fraction < 1.0)
		{
			VectorCopy(trace.endpos, cameraFocusLoc);
		}
		else
		{
			VectorCopy(nudgepos, cameraFocusLoc);
		}
	}
}

/*
===============
CG_CalcIdealThirdPersonViewLocation

===============
*/
static void CG_CalcIdealThirdPersonViewLocation()
{
	// Ensure snapshot exists before dereferencing.
	if (cg.snap == NULL)
	{
		return;
	}
	const qboolean doing_dash_action = cg.predictedPlayerState.communicatingflags & 1 << CF_DASHING ? qtrue : qfalse;

	// Override: explicit range
	if (cg.overrides.active & CG_OVERRIDE_3RD_PERSON_RNG)
	{
		VectorMA(cameraIdealTarget, -cg.overrides.thirdPersonRange, camerafwd, cameraIdealLoc);
	}
	// Held by rancor
	else if ((cg.snap->ps.eFlags & EF_HELD_BY_RANCOR) &&
		cg_entities[cg.snap->ps.clientNum].gent &&
		cg_entities[cg.snap->ps.clientNum].gent->activator)
	{
		VectorMA(cameraIdealTarget,
			-180.0f * cg_entities[cg.snap->ps.clientNum].gent->activator->s.modelScale[0],
			camerafwd,
			cameraIdealLoc);
	}
	// Held by wampa
	else if ((cg.snap->ps.eFlags & EF_HELD_BY_WAMPA) &&
		cg_entities[cg.snap->ps.clientNum].gent &&
		cg_entities[cg.snap->ps.clientNum].gent->activator &&
		cg_entities[cg.snap->ps.clientNum].gent->activator->inuse)
	{
		VectorMA(cameraIdealTarget,
			-120.0f * cg_entities[cg.snap->ps.clientNum].gent->activator->s.modelScale[0],
			camerafwd,
			cameraIdealLoc);
	}
	// Held by sand creature
	else if ((cg.snap->ps.eFlags & EF_HELD_BY_SAND_CREATURE) &&
		cg_entities[cg.snap->ps.clientNum].gent &&
		cg_entities[cg.snap->ps.clientNum].gent->activator)
	{
		const int sandEnt =
			cg_entities[cg.snap->ps.clientNum].gent->activator->s.number;

		VectorMA(cg_entities[sandEnt].lerpOrigin,
			-180.0f,
			camerafwd,
			cameraIdealLoc);
	}
	else
	{
		VectorMA(cameraIdealTarget, -cg_thirdPersonRange.value, camerafwd, cameraIdealLoc);
	}

	// Force speed camera modulation
	if (doing_dash_action == qfalse &&
		cg.renderingThirdPerson &&
		(cg.snap->ps.forcePowersActive & (1 << FP_SPEED)) &&
		player->client->ps.forcePowerDuration[FP_SPEED])
	{
		const float time_left =
			player->client->ps.forcePowerDuration[FP_SPEED] - cg.time;

		const float length =
			FORCE_SPEED_DURATION_FORCE_LEVEL_3 *
			forceSpeedValue[player->client->ps.forcePowerLevel[FP_SPEED]];

		const float amt =
			forceSpeedRangeMod[player->client->ps.forcePowerLevel[FP_SPEED]];

		if (time_left < 500.0f)
		{
			// Start going back
			VectorMA(cameraIdealLoc, (time_left / 500.0f) * amt, camerafwd, cameraIdealLoc);
		}
		else if ((length - time_left) < 1000.0f)
		{
			// Start zooming in
			VectorMA(cameraIdealLoc, ((length - time_left) / 1000.0f) * amt, camerafwd, cameraIdealLoc);
		}
		else
		{
			VectorMA(cameraIdealLoc, amt, camerafwd, cameraIdealLoc);
		}
	}
}

static void CG_ResetThirdPersonViewDamp()
{
	trace_t trace;

	// Cap the pitch within reasonable limits
	if (cameraFocusAngles[PITCH] > 89.0)
	{
		cameraFocusAngles[PITCH] = 89.0;
	}
	else if (cameraFocusAngles[PITCH] < -89.0)
	{
		cameraFocusAngles[PITCH] = -89.0;
	}

	AngleVectors(cameraFocusAngles, camerafwd, nullptr, cameraup);

	// Set the cameraIdealTarget
	CG_CalcIdealThirdPersonViewTarget();

	// Set the cameraIdealLoc
	CG_CalcIdealThirdPersonViewLocation();

	// Now, we just set everything to the new positions.
	VectorCopy(cameraIdealLoc, cameraCurLoc);
	VectorCopy(cameraIdealTarget, cameraCurTarget);

	// First thing we do is trace from the first person viewpoint out to the new target location.
	CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurTarget, cg.predictedPlayerState.clientNum,
		MASK_CAMERACLIP);
	if (trace.fraction <= 1.0)
	{
		VectorCopy(trace.endpos, cameraCurTarget);
	}

	// Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	CG_Trace(&trace, cameraCurTarget, cameramins, cameramaxs, cameraCurLoc, cg.predictedPlayerState.clientNum,
		MASK_CAMERACLIP);
	if (trace.fraction <= 1.0)
	{
		VectorCopy(trace.endpos, cameraCurLoc);
	}

	cameraLastFrame = cg.time;
	cameraLastYaw = cameraFocusAngles[YAW];
	cameraStiffFactor = 0.0f;
}

// This is called every frame.
static void CG_UpdateThirdPersonTargetDamp()
{
	trace_t trace;

	// Set the cameraIdealTarget
	// Automatically get the ideal target, to avoid jittering.
	CG_CalcIdealThirdPersonViewTarget();

	if (cg.predictedPlayerState.hyperSpaceTime
		&& cg.time - cg.predictedPlayerState.hyperSpaceTime < HYPERSPACE_TIME)
	{
		//hyperspacing, no damp
		VectorCopy(cameraIdealTarget, cameraCurTarget);
	}
	else if (CG_OnMovingPlat(&cg.snap->ps))
	{
		//if moving on a plat, camera is *tight*
		VectorCopy(cameraIdealTarget, cameraCurTarget);
	}
	else if (cg_thirdPersonTargetDamp.value >= 1.0 || cg.thisFrameTeleport || cg.predictedPlayerState.m_iVehicleNum)
	{
		// No damping.
		VectorCopy(cameraIdealTarget, cameraCurTarget);
	}
	else if (cg_thirdPersonTargetDamp.value >= 0.0)
	{
		float ratio;
		vec3_t targetdiff;
		// Calculate the difference from the current position to the new one.
		VectorSubtract(cameraIdealTarget, cameraCurTarget, targetdiff);

		// Now we calculate how much of the difference we cover in the time allotted.
		// The equation is (Damp)^(time)
		const float dampfactor = 1.0 - cg_thirdPersonTargetDamp.value;
		// We must exponent the amount LEFT rather than the amount bled off
		if (cg.renderingThirdPerson &&
			(cg.predictedPlayerState.communicatingflags & (1 << CF_AIMINGGUN)) &&
			cg_AimingCinematicCamera.integer)
		{
			const float dtime = static_cast<float>(cg.time - cameraLastFrame) * (1.0 / cg_timescale.value) * (1.0 /
				static_cast<float>(CAMERA_DAMP_INTERVAL_AIMING)); // Our dampfactor is geared towards a time interval equal to "1".
			if (cg_smoothCamera.integer)
				ratio = powf(dampfactor, dtime);
			else
				ratio = Q_powf(dampfactor, dtime);
		}
		else
		{
			const float dtime = static_cast<float>(cg.time - cameraLastFrame) * (1.0 / cg_timescale.value) * (1.0 /
				static_cast<float>(CAMERA_DAMP_INTERVAL)); // Our dampfactor is geared towards a time interval equal to "1".
			if (cg_smoothCamera.integer)
				ratio = powf(dampfactor, dtime);
			else
				ratio = Q_powf(dampfactor, dtime);
		}

		// This value is how much distance is "left" from the ideal.
		VectorMA(cameraIdealTarget, -ratio, targetdiff, cameraCurTarget);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
	}

	// Now we trace to see if the new location is cool or not.

	// First thing we do is trace from the first person viewpoint out to the new target location.
	if (cg.snap
		&& cg.snap->ps.eFlags & EF_HELD_BY_SAND_CREATURE
		&& cg_entities[cg.snap->ps.clientNum].gent->activator)
	{
		//if being held by a sand creature, trace from his actual origin, since we could be underground or otherwise in solid once he eats us
		CG_Trace(&trace, cg_entities[cg_entities[cg.snap->ps.clientNum].gent->activator->s.number].lerpOrigin,
			cameramins, cameramaxs, cameraCurTarget, cg.predictedPlayerState.clientNum, MASK_CAMERACLIP);
	}
	else
	{
		CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurTarget, cg.predictedPlayerState.clientNum,
			MASK_CAMERACLIP);
	}
	if (trace.fraction < 1.0)
	{
		VectorCopy(trace.endpos, cameraCurTarget);
	}

	// Note that previously there was an upper limit to the number of physics traces that are done through the world
	// for the sake of camera collision, since it wasn't calced per frame.  Now it is calculated every frame.
	// This has the benefit that the camera is a lot smoother now (before it lerped between tested points),
	// however two full volume traces each frame is a bit scary to think about.
}

// This can be called every interval, at the user's discretion.
static int camWaterAdjust = 0;

static void CG_UpdateThirdPersonCameraDamp()
{
	trace_t trace;

	// Set the cameraIdealLoc
	CG_CalcIdealThirdPersonViewLocation();

	// First thing we do is calculate the appropriate damping factor for the camera.
	float dampfactor = 0.0f;

	if (cg.predictedPlayerState.hyperSpaceTime
		&& cg.time - cg.predictedPlayerState.hyperSpaceTime < HYPERSPACE_TIME)
	{
		//hyperspacing - don't damp camera
		dampfactor = 1.0f;
	}
	else if (CG_OnMovingPlat(&cg.snap->ps))
	{
		//if moving on a plat, camera is *tight*
		dampfactor = 1.0f;
	}
	else if (cg.overrides.active & CG_OVERRIDE_3RD_PERSON_CDP)
	{
		if (cg.overrides.thirdPersonCameraDamp != 0.0f)
		{
			// Note that the camera pitch has already been capped off to 89.
			float pitch = Q_fabs(cameraFocusAngles[PITCH]);

			// The higher the pitch, the larger the factor, so as you look up, it damps a lot less.
			pitch /= 115.0f;
			dampfactor = (1.0 - cg.overrides.thirdPersonCameraDamp) * (pitch * pitch);

			dampfactor += cg.overrides.thirdPersonCameraDamp;
		}
	}
	else if (cg_thirdPersonCameraDamp.value != 0.0f)
	{
		// Note that the camera pitch has already been capped off to 89.
		float pitch = Q_fabs(cameraFocusAngles[PITCH]);

		// The higher the pitch, the larger the factor, so as you look up, it damps a lot less.
		pitch /= 115.0f;
		dampfactor = (1.0 - cg_thirdPersonCameraDamp.value) * (pitch * pitch);

		dampfactor += cg_thirdPersonCameraDamp.value;

		// Now we also multiply in the stiff factor, so that faster yaw changes are stiffer.
		if (cameraStiffFactor > 0.0f)
		{
			// The cameraStiffFactor is how much of the remaining damp below 1 should be shaved off, i.e. approach 1 as stiffening increases.
			dampfactor += (1.0 - dampfactor) * cameraStiffFactor;
		}
	}

	if (dampfactor >= 1.0)
	{
		// No damping.
		VectorCopy(cameraIdealLoc, cameraCurLoc);
	}
	else if (dampfactor >= 0.0)
	{
		float ratio;
		vec3_t locdiff;
		// Calculate the difference from the current position to the new one.
		VectorSubtract(cameraIdealLoc, cameraCurLoc, locdiff);

		// Now we calculate how much of the difference we cover in the time allotted.
		// The equation is (Damp)^(time)
		dampfactor = 1.0 - dampfactor; // We must exponent the amount LEFT rather than the amount bled off

		if (cg.renderingThirdPerson &&
			(cg.predictedPlayerState.communicatingflags & (1 << CF_AIMINGGUN)) &&
			cg_AimingCinematicCamera.integer)
		{
			const float dtime = static_cast<float>(cg.time - cameraLastFrame) * (1.0 / cg_timescale.value) * (1.0 /
				static_cast<float>(CAMERA_DAMP_INTERVAL_AIMING)); // Our dampfactor is geared towards a time interval equal to "1".

			if (cg_smoothCamera.integer)
				ratio = powf(dampfactor, dtime);
			else
				ratio = Q_powf(dampfactor, dtime);
		}
		else
		{
			const float dtime = static_cast<float>(cg.time - cameraLastFrame) * (1.0 / cg_timescale.value) * (1.0 /
				static_cast<float>(CAMERA_DAMP_INTERVAL)); // Our dampfactor is geared towards a time interval equal to "1".

			if (cg_smoothCamera.integer)
				ratio = powf(dampfactor, dtime);
			else
				ratio = Q_powf(dampfactor, dtime);
		}

		// This value is how much distance is "left" from the ideal.
		VectorMA(cameraIdealLoc, -ratio, locdiff, cameraCurLoc);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
	}

	// Now we trace from the first person viewpoint to the new view location, to make sure there is nothing in the way between the user and the camera...
	//	CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurLoc, cg.predictedPlayerState.clientNum, MASK_CAMERACLIP);
	// (OLD) Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	if (cg.snap
		&& cg.snap->ps.eFlags & EF_HELD_BY_SAND_CREATURE
		&& cg_entities[cg.snap->ps.clientNum].gent->activator)
	{
		//if being held by a sand creature, trace from his actual origin, since we could be underground or otherwise in solid once he eats us
		CG_Trace(&trace, cg_entities[cg_entities[cg.snap->ps.clientNum].gent->activator->s.number].lerpOrigin,
			cameramins, cameramaxs, cameraCurLoc, cg.predictedPlayerState.clientNum, MASK_CAMERACLIP);
	}
	else
	{
		CG_Trace(&trace, cameraCurTarget, cameramins, cameramaxs, cameraCurLoc, cg.predictedPlayerState.clientNum,
			MASK_CAMERACLIP);
	}
	if (trace.fraction < 1.0f)
	{
		VectorCopy(trace.endpos, cameraCurLoc);
	}

	// Note that previously there was an upper limit to the number of physics traces that are done through the world
	// for the sake of camera collision, since it wasn't calced per frame.  Now it is calculated every frame.
	// This has the benefit that the camera is a lot smoother now (before it lerped between tested points),
	// however two full volume traces each frame is a bit scary to think about.
}

/*
===============
CG_OffsetThirdPersonView

===============
*/
extern qboolean MatrixMode;

static void CG_OffsetThirdPersonView()
{
	// Ensure snapshot exists before dereferencing.
	if (cg.snap == NULL)
	{
		return;
	}

	vec3_t diff;
	static int aimLockTime = 0;

	camWaterAdjust = 0;
	cameraStiffFactor = 0.0f;

	// Set camera viewing direction.
	VectorCopy(cg.refdefViewAngles, cameraFocusAngles);

	// Held by rancor
	if ((cg.snap->ps.eFlags & EF_HELD_BY_RANCOR) &&
		cg_entities[cg.snap->ps.clientNum].gent &&
		cg_entities[cg.snap->ps.clientNum].gent->activator)
	{
		const centity_t* monster =
			&cg_entities[cg_entities[cg.snap->ps.clientNum].gent->activator->s.number];

		VectorSet(cameraFocusAngles,
			0,
			AngleNormalize180(monster->lerpAngles[YAW] + 180),
			0);
	}
	// Held by sand creature
	else if ((cg.snap->ps.eFlags & EF_HELD_BY_SAND_CREATURE) &&
		cg_entities[cg.snap->ps.clientNum].gent &&
		cg_entities[cg.snap->ps.clientNum].gent->activator)
	{
		const centity_t* monster =
			&cg_entities[cg_entities[cg.snap->ps.clientNum].gent->activator->s.number];

		VectorSet(cameraFocusAngles,
			0,
			AngleNormalize180(monster->lerpAngles[YAW] + 180),
			0);

		cameraFocusAngles[PITCH] = 0.0f;
	}
	// Riding vehicle
	else if (G_IsRidingVehicle(&g_entities[0]))
	{
		cameraFocusAngles[YAW] =
			cg_entities[g_entities[0].owner->s.number].lerpAngles[YAW];

		if (cg.overrides.active & CG_OVERRIDE_3RD_PERSON_ANG)
		{
			cameraFocusAngles[YAW] += cg.overrides.thirdPersonAngle;
		}
		else
		{
			cameraFocusAngles[YAW] += cg_thirdPersonAngle.value;
		}
	}
	// Dead
	else if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0)
	{
		if (MatrixMode)
		{
			if (cg.overrides.active & CG_OVERRIDE_3RD_PERSON_ANG)
			{
				cameraFocusAngles[YAW] += cg.overrides.thirdPersonAngle;
			}
			else
			{
				cameraFocusAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
				cameraFocusAngles[YAW] += cg_thirdPersonAngle.value;
			}
		}
		else
		{
			cameraFocusAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
		}
	}
	// Saber lock cinematic
	else if (cg.renderingThirdPerson &&
		(cg.predictedPlayerState.communicatingflags & (1 << CF_SABERLOCKING)) &&
		cg_saberLockCinematicCamera.integer)
	{
		cameraFocusAngles[YAW] += (cg.overrides.thirdPersonAngle = 40.5f);
		cameraFocusAngles[PITCH] += (cg.overrides.thirdPersonPitchOffset = -11.25f);
	}
	// Aiming cinematic
	else if (cg.renderingThirdPerson &&
		(cg.predictedPlayerState.communicatingflags & (1 << CF_AIMINGGUN)) &&
		cg_AimingCinematicCamera.integer)
	{
		cg.overrides.thirdPersonAngle = 10.0f;
		cg.overrides.thirdPersonPitchOffset = -2.0f;

		cameraFocusAngles[YAW] += cg.overrides.thirdPersonAngle;
		cameraFocusAngles[PITCH] += cg.overrides.thirdPersonPitchOffset;

		// Start aim-lock timer
		if (aimLockTime == 0)
		{
			aimLockTime = cg.time + 250;   // wait for zoom-in to finish
		}

		// Only apply dynamic shoulder tracking AFTER zoom-in
		if (cg.time > aimLockTime)
		{
			float side = cg.predictedPlayerState.velocity[1];

			// Only track sideways movement if actually strafing
			if (fabs(side) > 5.0f)
			{
				float sideNorm = side / 200.0f;

				if (sideNorm > 1.0f)  sideNorm = 1.0f;
				if (sideNorm < -1.0f) sideNorm = -1.0f;

				float baseShoulder = -20.0f;
				cg.overrides.thirdPersonHorzOffset = baseShoulder + (-18.0f * sideNorm);

				// Tight camera damp (keeps camera glued to shoulder)
				cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_CDP;
				cg.overrides.thirdPersonCameraDamp = 0.8f;
			}
			else
			{
				// NOT strafing → disable shoulder lock
				cg.overrides.active &= ~CG_OVERRIDE_3RD_PERSON_CDP;
			}
		}
	}
	// Normal third‑person angle
	else
	{
		if (cg.overrides.active & CG_OVERRIDE_3RD_PERSON_ANG)
		{
			cameraFocusAngles[YAW] += cg.overrides.thirdPersonAngle;
		}
		else
		{
			cameraFocusAngles[YAW] += cg_thirdPersonAngle.value;
		}

		if (cg.overrides.active & CG_OVERRIDE_3RD_PERSON_POF)
		{
			cameraFocusAngles[PITCH] += cg.overrides.thirdPersonPitchOffset;
		}
		else
		{
			cameraFocusAngles[PITCH] += cg_thirdPersonPitchOffset.value;
		}
		// Not aiming → reset everything
		aimLockTime = 0;
		cg.overrides.active &= ~CG_OVERRIDE_3RD_PERSON_CDP;
	}

	// First‑person saber handling
	if ((cg.snap->ps.weapon == WP_SABER || cg.snap->ps.weapon == WP_MELEE) &&
		cg.renderingThirdPerson == qfalse)
	{
		vec3_t org, view_dir;

		VectorCopy(cg_entities[0].gent->client->renderInfo.eyePoint, org);

		const float blend = 1.0f - fabs(cg.refdefViewAngles[PITCH]) / 90.0f;

		AngleVectors(cg.refdefViewAngles, view_dir, NULL, NULL);
		VectorMA(org, -8, view_dir, org);
		VectorScale(org, 1.0f - blend, org);
		VectorMA(org, blend, cg.refdef.vieworg, cg.refdef.vieworg);

		return;
	}

	// Reset dampers if time jumped
	if (cameraLastFrame == 0 || cameraLastFrame > cg.time)
	{
		CG_ResetThirdPersonViewDamp();
	}
	else
	{
		// Clamp pitch
		if (cameraFocusAngles[PITCH] > 89.0f)
		{
			cameraFocusAngles[PITCH] = 89.0f;
		}
		else if (cameraFocusAngles[PITCH] < -89.0f)
		{
			cameraFocusAngles[PITCH] = -89.0f;
		}

		AngleVectors(cameraFocusAngles, camerafwd, NULL, cameraup);

		float deltayaw = fabs(cameraFocusAngles[YAW] - cameraLastYaw);

		if (deltayaw > 180.0f)
		{
			deltayaw = fabs(deltayaw - 360.0f);
		}

		cameraStiffFactor =
			deltayaw / static_cast<float>(cg.time - cameraLastFrame);

		if (cameraStiffFactor < 1.0f)
		{
			cameraStiffFactor = 0.0f;
		}
		else if (cameraStiffFactor > 2.5f)
		{
			cameraStiffFactor = 0.75f;
		}
		else
		{
			cameraStiffFactor = (cameraStiffFactor - 1.0f) * 0.5f;
		}

		cameraLastYaw = cameraFocusAngles[YAW];

		CG_UpdateThirdPersonTargetDamp();
		CG_UpdateThirdPersonCameraDamp();
	}

	// Compute view angles from camera position
	VectorSubtract(cameraCurTarget, cameraCurLoc, diff);

	const float dist = VectorNormalize(diff);

	if (dist < 1.0f)
	{
		VectorCopy(camerafwd, diff);
	}

	vectoangles(diff, cg.refdefViewAngles);

	// Horizontal offset
	float horz = 0.0f;
	if (cg.overrides.active & CG_OVERRIDE_3RD_PERSON_HOF)
	{
		horz = cg.overrides.thirdPersonHorzOffset;
	}
	else
	{
		horz = cg_thirdPersonHorzOffset.value;
	}

	if (fabs(horz) > 0.0001f)
	{
		// build view axis from final view angles
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);

		// desired camera location after applying horizontal offset
		vec3_t desiredCamLoc;
		VectorMA(cameraCurLoc, horz, cg.refdef.viewaxis[1], desiredCamLoc);

		// trace from the camera target to the desired location so we don't end inside walls
		trace_t trace;
		CG_Trace(&trace, cameraCurTarget, cameramins, cameramaxs, desiredCamLoc,
			cg.predictedPlayerState.clientNum, MASK_CAMERACLIP);

		if (trace.fraction < 1.0f)
		{
			// hit something — clamp camera to safe position
			VectorCopy(trace.endpos, cameraCurLoc);
		}
		else
		{
			// clear path — use desired location
			VectorCopy(desiredCamLoc, cameraCurLoc);
		}
	}

	// Copy final camera position
	VectorCopy(cameraCurLoc, cg.refdef.vieworg);

	// Water adjustment
	if (camWaterAdjust)
	{
		cg.refdef.vieworg[2] += camWaterAdjust;
	}

	cameraLastFrame = cg.time;
}

// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset()
{
	// smooth out stair climbing
	const int timeDelta = cg.time - cg.stepTime;
	if (timeDelta < STEP_TIME)
	{
		cg.refdef.vieworg[2] -= cg.stepChange
			* (STEP_TIME - timeDelta) / STEP_TIME;
	}
}

/*
===============
CG_OffsetFirstPersonView

===============
*/
extern qboolean PM_InForceGetUp(const playerState_t* ps);
extern qboolean PM_InGetUp(const playerState_t* ps);
extern qboolean PM_InKnockDown(const playerState_t* ps);
extern int PM_AnimLength(const int index, const animNumber_t anim);

static void CG_OffsetFirstPersonView(const qboolean firstPersonSaber)
{
	float bob;
	float delta;
	float speed;
	float f;
	vec3_t predictedVelocity;
	int timeDelta;

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		return;
	}

	float* origin = cg.refdef.vieworg;
	float* angles = cg.refdefViewAngles;

	// if dead, fix the angle and don't add any kick
	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		angles[ROLL] = 40;
		angles[PITCH] = -15;
		angles[YAW] = cg.snap->ps.stats[STAT_DEAD_YAW];
		origin[2] += cg.predictedPlayerState.viewheight;
		return;
	}

	if (g_entities[0].client && PM_InKnockDown(&g_entities[0].client->ps))
	{
		float perc;
		const float anim_len = static_cast<float>(PM_AnimLength(g_entities[0].client->clientInfo.animFileIndex,
			static_cast<animNumber_t>(g_entities[0].client->ps.
				legsAnim)));
		if (PM_InGetUp(&g_entities[0].client->ps) || PM_InForceGetUp(&g_entities[0].client->ps))
		{
			//start righting the view
			perc = static_cast<float>(g_entities[0].client->ps.legsAnimTimer) / anim_len * 2;
		}
		else
		{
			//tilt the view
			perc = (anim_len - g_entities[0].client->ps.legsAnimTimer) / anim_len * 2;
		}
		if (perc > 1.0f)
		{
			perc = 1.0f;
		}
		angles[ROLL] = perc * 40;
		angles[PITCH] = perc * -15;
	}

	// add angles based on weapon kick
	int kickTime = cg.time - cg.kick_time;
	if (kickTime < 800)
	{
		//kicks are always 1 second long.  Deal with it.
		float kick_perc;
		if (kickTime <= 200)
		{
			//winding up
			kick_perc = kickTime / 200.0f;
		}
		else
		{
			//returning to normal
			kickTime = 800 - kickTime;
			kick_perc = kickTime / 600.0f;
		}
		VectorMA(angles, kick_perc, cg.kick_angles, angles);
	}

	// add angles based on damage kick
	if (cg.damageTime)
	{
		float ratio = cg.time - cg.damageTime;
		if (ratio < DAMAGE_DEFLECT_TIME)
		{
			ratio /= DAMAGE_DEFLECT_TIME;
			angles[PITCH] += ratio * cg.v_dmg_pitch;
			angles[ROLL] += ratio * cg.v_dmg_roll;
		}
		else
		{
			ratio = 1.0 - (ratio - DAMAGE_DEFLECT_TIME) / DAMAGE_RETURN_TIME;
			if (ratio > 0)
			{
				angles[PITCH] += ratio * cg.v_dmg_pitch;
				angles[ROLL] += ratio * cg.v_dmg_roll;
			}
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = (cg.time - cg.landTime) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	angles[PITCH] += ratio * cg.fall_value;
#endif

	// add angles based on velocity
	VectorCopy(cg.predictedPlayerState.velocity, predictedVelocity);

	delta = DotProduct(predictedVelocity, cg.refdef.viewaxis[0]);
	angles[PITCH] += delta * cg_runpitch.value;

	delta = DotProduct(predictedVelocity, cg.refdef.viewaxis[1]);
	angles[ROLL] -= delta * cg_runroll.value;

	// add angles based on bob

	// make sure the bob is visible even at low speeds
	speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

	delta = cg.bobfracsin * cg_bobpitch.value * speed;

	if (cg.predictedPlayerState.pm_flags & PMF_DUCKED && cg.predictedPlayerState.groundEntityNum != ENTITYNUM_NONE)
	{// if crouching, accentuate the pitch
		delta *= 3.0f; // crouching
		angles[PITCH] += delta;
		delta = cg.bobfracsin * cg_bobroll.value * speed;
	}

	if (cg.predictedPlayerState.pm_flags & PMF_DUCKED && cg.predictedPlayerState.groundEntityNum != ENTITYNUM_NONE)
	{ // if crouching, accentuate the roll
		delta *= 3.0f; // crouching accentuates roll
	}

	if (cg.snap
		&& ((cg.snap->ps.eFlags & EF_MEDITATING) != 0))
	{
		delta *= 3; // meditating
		angles[PITCH] += delta;
		delta = cg.bobfracsin * cg_bobroll.value * speed;
	}

	if (cg.snap
		&& ((cg.snap->ps.eFlags & EF_MEDITATING) != 0))
	{
		delta *= 3; // meditating accentuates roll
	}

	if (cg.bobcycle & 1)
		delta = -delta;
	angles[ROLL] += delta;

	//===================================

	if (!firstPersonSaber) //First person saber
	{
		// add view height
		if (cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD)
		{
			if (&g_entities[cg.snap->ps.viewEntity] &&
				g_entities[cg.snap->ps.viewEntity].client &&
				g_entities[cg.snap->ps.viewEntity].client->ps.viewheight)
			{
				origin[2] += g_entities[cg.snap->ps.viewEntity].client->ps.viewheight;
			}
			else
			{
				origin[2] += 4; //???
			}
		}
		else
		{
			origin[2] += cg.predictedPlayerState.viewheight;
		}
	}

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if (timeDelta < DUCK_TIME)
	{
		cg.refdef.vieworg[2] -= cg.duckChange * (DUCK_TIME - timeDelta) / DUCK_TIME;
	}

	// add bob height
	bob = cg.bobfracsin * cg.xyspeed * cg_bobup.value;
	if (bob > 6)
	{
		bob = 6;
	}

	origin[2] += bob;

	// add fall height
	delta = cg.time - cg.landTime;
	if (delta < LAND_DEFLECT_TIME)
	{
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landChange * f;
	}
	else if (delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME)
	{
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - delta / LAND_RETURN_TIME;
		cg.refdef.vieworg[2] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();

	if (cg.snap
		&& cg.snap->ps.leanofs != 0)
	{
		vec3_t right;
		//add leaning offset
		//FIXME: when crouching, this bounces up and down?!
		cg.refdefViewAngles[2] += static_cast<float>(cg.snap->ps.leanofs) / 2;
		AngleVectors(cg.refdefViewAngles, nullptr, right, nullptr);
		VectorMA(cg.refdef.vieworg, static_cast<float>(cg.snap->ps.leanofs), right, cg.refdef.vieworg);
	}

	// pivot the eye based on a neck length
#if 0
	{
#define	NECK_LENGTH		8
		vec3_t			forward, up;

		cg.refdef.vieworg[2] -= NECK_LENGTH;
		AngleVectors(cg.refdefViewAngles, forward, NULL, up);
		VectorMA(cg.refdef.vieworg, 3, forward, cg.refdef.vieworg);
		VectorMA(cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg);
	}
#endif
}

/*
====================
CG_CalcFovFromX

Calcs Y FOV from given X FOV
====================
*/
qboolean CG_CalcFOVFromX(float fov_x)
{
	qboolean inwater;

	if (cg_fovAspectAdjust.integer)
	{
		// Based on LordHavoc's code for Darkplaces
		// http://www.quakeworld.nu/forum/topic/53/what-does-your-qw-look-like/page/30
		constexpr float baseAspect = 0.75f; // 3/4
		const float aspect = static_cast<float>(cgs.glconfig.vidWidth) / static_cast<float>(cgs.glconfig.vidHeight);
		const float desiredFov = fov_x;

		fov_x = atan(tan(desiredFov * M_PI / 360.0f) * baseAspect * aspect) * 360.0f / M_PI;
	}

	const float x = cg.refdef.width / tan(fov_x / 360 * M_PI);
	float fov_y = atan2(cg.refdef.height, x);
	fov_y = fov_y * 360 / M_PI;

	cg.refdef.viewContents = 0;
	if (gi.totalMapContents() & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		cg.refdef.viewContents = CG_PointContents(cg.refdef.vieworg, -1);
	}
	if (cg.refdef.viewContents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		const float phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		const float v = WAVE_AMPLITUDE * sin(phase);
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}
	else
	{
		inwater = qfalse;
	}

	// see if we are drugged by an interrogator.  We muck with the FOV here, a bit later, after viewangles are calc'ed, I muck with those too.
	if (cg.wonkyTime > 0 && cg.wonkyTime > cg.time)
	{
		const float perc = static_cast<float>(cg.wonkyTime - cg.time) / 10000.0f; // goes for 10 seconds

		if (cg.snap->ps.legsAnim == BOTH_MEDITATE
			|| cg.snap->ps.legsAnim == BOTH_MEDITATE1
			|| cg.snap->ps.legsAnim == BOTH_CROUCH1
			|| cg.snap->ps.legsAnim == BOTH_CROUCH2
			|| cg.snap->ps.legsAnim == BOTH_CROUCH3
			|| cg.snap->ps.legsAnim == BOTH_CROUCH4
			|| cg.snap->ps.legsAnim == BOTH_CROUCH1IDLE
			|| cg.snap->ps.legsAnim == BOTH_STAND_TO_KNEEL
			|| cg.snap->ps.legsAnim == BOTH_KNEES2TO1)
		{
			fov_x += 25.0f * perc;
			fov_y -= cos(cg.time * 0.0008f) * 5.0f * perc;
		}
		else if (cg.snap->ps.forcePowersActive & 1 << FP_ABSORB)
		{
			fov_x += 15.0f * perc;
			fov_y -= cos(cg.time * 0.0008f) * 2.0f * perc;
		}
		else
		{
			fov_x += 45.0f * perc;
			fov_y -= cos(cg.time * 0.0018f) * 15.0f * perc;
		}
	}

	if (cg.stunnedTime > 0 && cg.stunnedTime > cg.time)
	{
		const float perc = static_cast<float>(cg.stunnedTime - cg.time) / 5000.0f; // goes for 5 seconds

		if (cg.snap->ps.legsAnim == BOTH_MEDITATE
			|| cg.snap->ps.legsAnim == BOTH_MEDITATE1
			|| cg.snap->ps.legsAnim == BOTH_CROUCH1
			|| cg.snap->ps.legsAnim == BOTH_CROUCH2
			|| cg.snap->ps.legsAnim == BOTH_CROUCH3
			|| cg.snap->ps.legsAnim == BOTH_CROUCH4
			|| cg.snap->ps.legsAnim == BOTH_CROUCH1IDLE
			|| cg.snap->ps.legsAnim == BOTH_STAND_TO_KNEEL
			|| cg.snap->ps.legsAnim == BOTH_KNEES2TO1)
		{
			fov_x += 25.0f * perc;
			fov_y -= cos(cg.time * 0.0004f) * 5.0f * perc;
		}
		else if (cg.snap->ps.forcePowersActive & 1 << FP_ABSORB)
		{
			fov_x += 15.0f * perc;
			fov_y -= cos(cg.time * 0.0004f) * 2.0f * perc;
		}
		else
		{
			fov_x += 35.0f * perc;
			fov_y -= cos(cg.time * 0.009f) * 10.0f * perc;
		}
	}

	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	return inwater;
}

float CG_ForceSpeedFOV()
{
	float fov;
	const float time_left = player->client->ps.forcePowerDuration[FP_SPEED] - cg.time;
	const float length = FORCE_SPEED_DURATION_FORCE_LEVEL_3 * forceSpeedValue[player->client->ps.forcePowerLevel[
		FP_SPEED]];
	const float amt = forceSpeedFOVMod[player->client->ps.forcePowerLevel[FP_SPEED]];

	if (!cg.renderingThirdPerson && cg_truefov.value && (!cg.zoomMode && cg_trueguns.integer || cg.snap->ps.weapon ==
		WP_SABER || cg.snap->ps.weapon == WP_MELEE))
	{
		fov = cg_truefov.value;
	}
	else
	{
		fov = cg_fov.value;
	}
	if (time_left < 500)
	{
		//start going back
		fov = cg_fov.value + time_left / 500 * amt;
	}
	else if (length - time_left < 1000)
	{
		//start zooming in
		fov = cg_fov.value + (length - time_left) / 1000 * amt;
	}
	else
	{
		//stay at this FOV
		fov = cg_fov.value + amt;
	}
	return fov;
}

/*
====================
CG_CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
static qboolean CG_CalcFov()
{
	float fov_x;
	const qboolean doing_dash_action = cg.predictedPlayerState.communicatingflags & 1 << CF_DASHING ? qtrue : qfalse;

	if (cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		// if in intermission, use a fixed value
		fov_x = 80;
	}
	else if (cg.snap
		&& cg.snap->ps.viewEntity > 0
		&& cg.snap->ps.viewEntity < ENTITYNUM_WORLD
		&& (!cg.renderingThirdPerson || g_entities[cg.snap->ps.viewEntity].e_DieFunc == dieF_camera_die))
	{
		// if in entity camera view, use a special FOV
		if (&g_entities[cg.snap->ps.viewEntity] &&
			g_entities[cg.snap->ps.viewEntity].NPC)
		{
			//FIXME: looks bad when take over a jedi... but never really do that, do we?
			fov_x = g_entities[cg.snap->ps.viewEntity].NPC->stats.hfov;
			//sanity-cap?
			if (fov_x > 120)
			{
				fov_x = 120;
			}
			else if (fov_x < 10)
			{
				fov_x = 10;
			}
		}
		else
		{
			if (cg.overrides.active & CG_OVERRIDE_FOV)
			{
				fov_x = cg.overrides.fov;
			}
			else
			{
				fov_x = 120; //FIXME: read from the NPC's fov stats?
			}
		}
	}
	else if (!doing_dash_action && (!cg.zoomMode || cg.zoomMode > 2) && cg.snap->ps.forcePowersActive & 1 << FP_SPEED &&
		player->client->ps.forcePowerDuration[FP_SPEED]) //cg.renderingThirdPerson &&
	{
		fov_x = CG_ForceSpeedFOV();
	}
	else
	{
		// user selectable
		if (cg.overrides.active & CG_OVERRIDE_FOV)
		{
			fov_x = cg.overrides.fov;
		}
		else if (!cg.renderingThirdPerson && (cg_trueguns.integer || cg.predictedPlayerState.weapon == WP_SABER
			|| cg.predictedPlayerState.weapon == WP_MELEE) && cg_truefov.value)
		{
			fov_x = cg_truefov.value;
		}
		else
		{
			fov_x = cg_fov.value;
		}
		if (fov_x < 1)
		{
			fov_x = 1;
		}
		else if (fov_x > 160)
		{
			fov_x = 160;
		}

		// Disable zooming when in third person
		if (cg.zoomMode && cg.zoomMode < 3) // light amp goggles do none of the zoom silliness
		{
			if (!cg.zoomLocked)
			{
				if (cg.zoomMode == 1)
				{
					// binoculars zooming either in or out
					cg_zoomFov += cg.zoomDir * cg.frametime * 0.05f;
				}
				else
				{
					// disruptor zooming in faster
					cg_zoomFov -= cg.frametime * 0.075f;
				}

				// Clamp zoomFov
				const float actualFOV = cg.overrides.active & CG_OVERRIDE_FOV ? cg.overrides.fov : cg_fov.value;
				if (cg_zoomFov < MAX_ZOOM_FOV)
				{
					cg_zoomFov = MAX_ZOOM_FOV;
				}
				else if (cg_zoomFov > actualFOV)
				{
					cg_zoomFov = actualFOV;
				}
				else
				{
					//still zooming
					static int zoomSoundTime = 0;

					if (zoomSoundTime < cg.time)
					{
						sfxHandle_t snd;

						if (cg.zoomMode == 1)
						{
							snd = cgs.media.zoomLoop;
						}
						else
						{
							snd = cgs.media.disruptorZoomLoop;
						}

						// huh?  This could probably just be added as a looping sound??
						cgi_S_StartSound(cg.refdef.vieworg, ENTITYNUM_WORLD, CHAN_LOCAL, snd);
						zoomSoundTime = cg.time + 150;
					}
				}
			}

			fov_x = cg_zoomFov;
		}
		else
		{
			const float f = (cg.time - cg.zoomTime) / ZOOM_OUT_TIME;
			if (f <= 1.0)
			{
				fov_x = cg_zoomFov + f * (fov_x - cg_zoomFov);
			}
		}
	}

	//	g_fov = fov_x;
	return CG_CalcFOVFromX(fov_x);
}

/*
===============
CG_DamageBlendBlob

===============
*/
static void CG_DamageBlendBlob()
{
	refEntity_t ent;

	if (!cg.damageValue)
	{
		return;
	}

	constexpr int max_time = DAMAGE_TIME;
	const int t = cg.time - cg.damageTime;
	if (t <= 0 || t >= max_time)
	{
		return;
	}

	memset(&ent, 0, sizeof ent);
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	VectorMA(cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin);
	VectorMA(ent.origin, cg.damageX * -8, cg.refdef.viewaxis[1], ent.origin);
	VectorMA(ent.origin, cg.damageY * 8, cg.refdef.viewaxis[2], ent.origin);

	ent.radius = cg.damageValue * 3 * (1.0 - static_cast<float>(t) / max_time);
	ent.customShader = cgs.media.damageBlendBlobShader;
	ent.shaderRGBA[0] = 180 * (1.0 - static_cast<float>(t) / max_time);
	ent.shaderRGBA[1] = 50 * (1.0 - static_cast<float>(t) / max_time);
	ent.shaderRGBA[2] = 50 * (1.0 - static_cast<float>(t) / max_time);
	ent.shaderRGBA[3] = 255;

	cgi_R_AddRefEntityToScene(&ent);
}

/*
====================
CG_SaberClashFlare
====================
*/
extern int g_saberFlashTime;
extern vec3_t g_saberFlashPos;
extern qboolean CG_WorldCoordToScreenCoord(vec3_t world_coord, int* x, int* y);

void CG_SaberClashFlare()
{
	constexpr int max_time = 150;

	const int t = cg.time - g_saberFlashTime;

	if (t <= 0 || t >= max_time)
	{
		return;
	}

	if (cg.predictedPlayerState.communicatingflags & (1 << CF_SABERLOCKING))
	{
		return;
	}

	vec3_t dif;

	// Don't do clashes for things that are behind us
	VectorSubtract(g_saberFlashPos, cg.refdef.vieworg, dif);

	if (DotProduct(dif, cg.refdef.viewaxis[0]) < 0.2)
	{
		return;
	}

	trace_t tr;

	CG_Trace(&tr, cg.refdef.vieworg, nullptr, nullptr, g_saberFlashPos, -1, CONTENTS_SOLID);

	if (tr.fraction < 1.0f)
	{
		return;
	}

	vec3_t color;
	int x, y;
	float len = VectorNormalize(dif);

	// clamp to a known range
	if (len > 800)
	{
		len = 800;
	}

	const float v = (1.0f - static_cast<float>(t) / max_time) * ((1.0f - len / 800.0f) * 2.0f + 0.35f);

	CG_WorldCoordToScreenCoord(g_saberFlashPos, &x, &y);

	VectorSet(color, 0.8f, 0.8f, 0.8f);
	cgi_R_SetColor(color);

	CG_DrawPic(x - v * 300 * cgs.widthRatioCoef, y - v * 300, v * 600 * cgs.widthRatioCoef, v * 600, cgi_R_RegisterShader("gfx/effects/saberFlare"));
}

void CG_SaberBlockFlare()
{
	constexpr int max_time = 150;

	const int t = cg.time - g_saberFlashTime;

	if (t <= 0 || t >= max_time)
	{
		return;
	}

	vec3_t dif;

	// Don't do clashes for things that are behind us
	VectorSubtract(g_saberFlashPos, cg.refdef.vieworg, dif);

	if (DotProduct(dif, cg.refdef.viewaxis[0]) < 0.2)
	{
		return;
	}

	trace_t tr;

	CG_Trace(&tr, cg.refdef.vieworg, nullptr, nullptr, g_saberFlashPos, -1, CONTENTS_SOLID);

	if (tr.fraction < 1.0f)
	{
		return;
	}

	vec3_t color;
	int x, y;
	float len = VectorNormalize(dif);

	// clamp to a known range
	if (len > 800)
	{
		len = 800;
	}

	const float v = (1.0f - static_cast<float>(t) / max_time) * ((1.0f - len / 800.0f) * 2.0f + 0.35f);

	CG_WorldCoordToScreenCoord(g_saberFlashPos, &x, &y);

	VectorSet(color, 0.8f, 0.8f, 0.8f);
	cgi_R_SetColor(color);

	CG_DrawPic(x - v * 300 * cgs.widthRatioCoef, y - v * 300,
		v * 600 * cgs.widthRatioCoef, v * 600,
		cgi_R_RegisterShader("gfx/effects/BlockFlare"));
}

/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/
static qboolean CG_CalcViewValues()
{
	playerState_t* ps;
	//qboolean		viewEntIsHumanoid = qfalse;
	qboolean viewEntIsCam = qfalse;

	memset(&cg.refdef, 0, sizeof cg.refdef);

	// calculate size of 3D view
	CG_CalcVrect();

	if (cg.snap->ps.viewEntity != 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD &&
		g_entities[cg.snap->ps.viewEntity].client)
	{
		ps = &g_entities[cg.snap->ps.viewEntity].client->ps;
		//viewEntIsHumanoid = qtrue;
	}
	else
	{
		ps = &cg.predictedPlayerState;
	}
#ifndef FINAL_BUILD
	trap_Com_SetOrgAngles(ps->origin, ps->viewangles);
#endif
	// intermission view
	if (ps->pm_type == PM_INTERMISSION)
	{
		VectorCopy(ps->origin, cg.refdef.vieworg);
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		return CG_CalcFov();
	}

	cg.bobcycle = (ps->bobCycle & 128) >> 7;
	cg.bobfracsin = fabs(sin((ps->bobCycle & 127) / 127.0 * M_PI));
	cg.xyspeed = sqrt(ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1]);

	if (G_IsRidingVehicle(&g_entities[0]) && cg.snap->ps.weapon == WP_NONE)
	{
		VectorCopy(ps->origin, cg.refdef.vieworg);
		VectorCopy(cg_entities[g_entities[0].owner->s.number].lerpAngles, cg.refdefViewAngles);
		if (!(ps->eFlags & EF_NODRAW))
		{
			//riding it, not *inside* it
			//let us look up & down
			cg.refdefViewAngles[PITCH] = cg_entities[ps->clientNum].lerpAngles[PITCH] * 0.2f;
		}
	}
	else if (cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD)
	{
		//in an entity camera view
		VectorCopy(cg_entities[cg.snap->ps.viewEntity].lerpOrigin, cg.refdef.vieworg);
		VectorCopy(cg_entities[cg.snap->ps.viewEntity].lerpAngles, cg.refdefViewAngles);
		if (!Q_stricmp("misc_camera", g_entities[cg.snap->ps.viewEntity].classname) || g_entities[cg.snap->ps.
			viewEntity].s.weapon == WP_TURRET)
		{
			viewEntIsCam = qtrue;
		}
	}
	else if (cg.renderingThirdPerson && !cg.zoomMode && cg.overrides.active & CG_OVERRIDE_3RD_PERSON_ENT)
	{
		//different center, same angle
		VectorCopy(cg_entities[cg.overrides.thirdPersonEntity].lerpOrigin, cg.refdef.vieworg);
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
	}
	else
	{
		//player's center and angles
		VectorCopy(ps->origin, cg.refdef.vieworg);
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
	}

	// add error decay
	if (cg_errorDecay.value > 0)
	{
		const int t = cg.time - cg.predictedErrorTime;
		const float f = (cg_errorDecay.value - t) / cg_errorDecay.value;
		if (f > 0 && f < 1)
		{
			VectorMA(cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg);
		}
		else
		{
			cg.predictedErrorTime = 0;
		}
	}

	if ((cg.renderingThirdPerson || cg.snap->ps.weapon == WP_SABER || cg.snap->ps.weapon == WP_MELEE)
		&& !cg.zoomMode
		&& !viewEntIsCam)
	{
		CG_OffsetThirdPersonView();
	}
	else
	{
		// offset for local bobbing and kicks
		CG_OffsetFirstPersonView(qfalse);
		const centity_t* playerCent = &cg_entities[0];
		if (playerCent && playerCent->gent && playerCent->gent->client)
		{
			VectorCopy(cg.refdef.vieworg, playerCent->gent->client->renderInfo.eyePoint);
			VectorCopy(cg.refdefViewAngles, playerCent->gent->client->renderInfo.eyeAngles);
			if (cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD)
			{
				//in an entity camera view
				if (cg_entities[cg.snap->ps.viewEntity].gent->client)
				{
					//looking through a client's eyes
					VectorCopy(cg.refdef.vieworg,
						cg_entities[cg.snap->ps.viewEntity].gent->client->renderInfo.eyePoint);
					VectorCopy(cg.refdefViewAngles,
						cg_entities[cg.snap->ps.viewEntity].gent->client->renderInfo.eyeAngles);
				}
				else
				{
					//looking through a regular ent's eyes
					VectorCopy(cg.refdef.vieworg, cg_entities[cg.snap->ps.viewEntity].lerpOrigin);
					VectorCopy(cg.refdefViewAngles, cg_entities[cg.snap->ps.viewEntity].lerpAngles);
				}
			}
			VectorCopy(playerCent->gent->client->renderInfo.eyePoint, playerCent->gent->client->renderInfo.headPoint);
			if (cg.snap->ps.viewEntity <= 0 || cg.snap->ps.viewEntity >= ENTITYNUM_WORLD)
			{
				//not in entity cam
				playerCent->gent->client->renderInfo.headPoint[2] -= 8;
			}
		}
	}

	// shake the camera if necessary
	CGCam_UpdateSmooth(cg.refdef.vieworg);
	CGCam_UpdateShake(cg.refdef.vieworg, cg.refdefViewAngles);

	// see if we are drugged by an interrogator.  We muck with the angles here, just a bit earlier, we mucked with the FOV
	if (cg.wonkyTime > 0 && cg.wonkyTime > cg.time)
	{
		const float perc = static_cast<float>(cg.wonkyTime - cg.time) / 10000.0f; // goes for 10 seconds

		if (cg.snap->ps.legsAnim == BOTH_MEDITATE
			|| cg.snap->ps.legsAnim == BOTH_MEDITATE1
			|| cg.snap->ps.legsAnim == BOTH_CROUCH1
			|| cg.snap->ps.legsAnim == BOTH_CROUCH2
			|| cg.snap->ps.legsAnim == BOTH_CROUCH3
			|| cg.snap->ps.legsAnim == BOTH_CROUCH4)
		{
			cg.refdefViewAngles[ROLL] += sin(cg.time * 0.0004f) * 7.0f * perc;
			cg.refdefViewAngles[PITCH] += 26.0f * perc + sin(cg.time * 0.0011f) * 3.0f * perc;
		}
		else if (cg.snap->ps.forcePowersActive & 1 << FP_ABSORB)
		{
			cg.refdefViewAngles[ROLL] += sin(cg.time * 0.0004f) * 5.0f * perc;
			cg.refdefViewAngles[PITCH] += 12.0f * perc + sin(cg.time * 0.0011f) * 3.0f * perc;
		}
		else
		{
			cg.refdefViewAngles[ROLL] += sin(cg.time * 0.0008f) * 27.0f * perc;
			cg.refdefViewAngles[PITCH] += 46.0f * perc + sin(cg.time * 0.0011f) * 6.0f * perc;
		}
	}

	if (cg.stunnedTime > 0 && cg.stunnedTime > cg.time)
	{
		const float perc = static_cast<float>(cg.stunnedTime - cg.time) / 5000.0f; // goes for 5 seconds

		if (cg.snap->ps.legsAnim == BOTH_MEDITATE
			|| cg.snap->ps.legsAnim == BOTH_MEDITATE1
			|| cg.snap->ps.legsAnim == BOTH_CROUCH1
			|| cg.snap->ps.legsAnim == BOTH_CROUCH2
			|| cg.snap->ps.legsAnim == BOTH_CROUCH3
			|| cg.snap->ps.legsAnim == BOTH_CROUCH4)
		{
			cg.refdefViewAngles[ROLL] += sin(cg.time * 0.0004f) * 7.0f * perc;
			cg.refdefViewAngles[PITCH] += 26.0f * perc + sin(cg.time * 0.0011f) * 3.0f * perc;
		}
		else if (cg.snap->ps.forcePowersActive & 1 << FP_ABSORB)
		{
			cg.refdefViewAngles[ROLL] += sin(cg.time * 0.0004f) * 5.0f * perc;
			cg.refdefViewAngles[PITCH] += 12.0f * perc + sin(cg.time * 0.0011f) * 3.0f * perc;
		}
		else
		{
			cg.refdefViewAngles[ROLL] += sin(cg.time * 0.0008f) * 27.0f * perc;
			cg.refdefViewAngles[PITCH] += 46.0f * perc + sin(cg.time * 0.0011f) * 6.0f * perc;
		}
	}

	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);

	if (cg.hyperspace)
	{
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}

/*
=====================
CG_PowerupTimerSounds
=====================
*/
static void CG_PowerupTimerSounds()
{
	// powerup timers going away
	for (const int time : cg.snap->ps.powerups)
	{
		if (time > 0 && time < cg.time)
		{
			//
		}
	}
}

/*
==============
CG_DrawSkyBoxPortal
==============
*/
extern void cgi_CM_SnapPVS(vec3_t origin, byte* buffer);

static void CG_DrawSkyBoxPortal(void)
{
	const char* cstr = CG_ConfigString(CS_SKYBOXORG);

	// No skybox present
	if (cstr == NULL || cstr[0] == '\0')
	{
		return;
	}

	// Backup current refdef
	const refdef_t backuprefdef = cg.refdef;

	// Begin parsing
	COM_BeginParseSession();

	const char* token = NULL;

	// ---------------------------------------------------------
	// Helper lambda for safe token parsing
	// ---------------------------------------------------------
	auto ParseRequiredToken = [&](const char* errorMsg) -> const char*
		{
			token = COM_ParseExt(&cstr, qfalse);

			if (token == NULL || token[0] == '\0')
			{
				CG_Printf(errorMsg);
				COM_EndParseSession();
				cg.refdef = backuprefdef;
				return NULL;
			}
			return token;
		};

	// ---------------------------------------------------------
	// Parse vieworg X Y Z
	// ---------------------------------------------------------
	token = ParseRequiredToken("CG_DrawSkyBoxPortal: missing vieworg[0]\n");
	if (token == NULL) return;
	cg.refdef.vieworg[0] = static_cast<float>(atof(token));

	token = ParseRequiredToken("CG_DrawSkyBoxPortal: missing vieworg[1]\n");
	if (token == NULL) return;
	cg.refdef.vieworg[1] = static_cast<float>(atof(token));

	token = ParseRequiredToken("CG_DrawSkyBoxPortal: missing vieworg[2]\n");
	if (token == NULL) return;
	cg.refdef.vieworg[2] = static_cast<float>(atof(token));

	// ---------------------------------------------------------
	// Fog state
	// ---------------------------------------------------------
	token = ParseRequiredToken("CG_DrawSkyBoxPortal: missing fog state\n");
	if (token == NULL) return;

	const qboolean hasFog = (atoi(token) != 0) ? qtrue : qfalse;

	if (hasFog == qtrue)
	{
		// fog[0]
		if (ParseRequiredToken("CG_DrawSkyBoxPortal: missing fog[0]\n") == NULL) return;

		// fog[1]
		if (ParseRequiredToken("CG_DrawSkyBoxPortal: missing fog[1]\n") == NULL) return;

		// fog[2]
		if (ParseRequiredToken("CG_DrawSkyBoxPortal: missing fog[2]\n") == NULL) return;

		// Skip fog range + density (not used after first time)
		COM_ParseExt(&cstr, qfalse);
		COM_ParseExt(&cstr, qfalse);
	}

	COM_EndParseSession();

	// ---------------------------------------------------------
	// Render skybox portal
	// ---------------------------------------------------------
	if (cg.hyperspace == qfalse)
	{
		CG_AddPacketEntities(qtrue);
		theFxScheduler.AddScheduledEffects(qtrue);
	}

	cg.refdef.rdflags |= RDF_SKYBOXPORTAL;
	cg.refdef.rdflags |= RDF_DRAWSKYBOX;

	cgi_CM_SnapPVS(cg.refdef.vieworg, cg.refdef.areamask);

	cgi_R_RenderScene(&cg.refdef);

	// Restore original refdef
	cg.refdef = backuprefdef;
}

//----------------------------
static void CG_RunEmplacedWeapon()
{
	const gentity_t* player = &g_entities[0],
		* gun = player->owner;

	// Override the camera when we are locked onto the gun.
	if (player
		&& gun
		&& !gun->bounceCount //not an eweb
		&& player->s.eFlags & EF_LOCKED_TO_WEAPON)
	{
		// don't let the player try and change this
		cg.renderingThirdPerson = qtrue;

		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);

		VectorCopy(gun->pos2, cg.refdef.vieworg);
		VectorMA(cg.refdef.vieworg, -20.0f, gun->pos3, cg.refdef.vieworg);
		if (cg.snap->ps.viewEntity <= 0 || cg.snap->ps.viewEntity >= ENTITYNUM_WORLD)
		{
			VectorMA(cg.refdef.vieworg, 35.0f, gun->pos4, cg.refdef.vieworg);
		}
	}
}

//=========================================================================

static qboolean Holding_Gun_And_Walking(const gentity_t* self)
{
	const qboolean holding_walking_button = (cg.predictedPlayerState.pm_flags & PMF_WALKING_HELD) ? qtrue : qfalse;

	if (holding_walking_button)
	{
		switch (self->client->ps.weapon)
		{
		case WP_MELEE:
		case WP_STUN_BATON:
		case WP_BLASTER_PISTOL:
		case WP_BLASTER:
		case WP_DISRUPTOR:
		case WP_BOWCASTER:
		case WP_REPEATER:
		case WP_DEMP2:
		case WP_FLECHETTE:
		case WP_ROCKET_LAUNCHER:
		case WP_THERMAL:
		case WP_TRIP_MINE:
		case WP_DET_PACK:
		case WP_CONCUSSION:
		case WP_ATST_MAIN:
		case WP_ATST_SIDE:
		case WP_BRYAR_PISTOL:
		case WP_SBD_PISTOL:
		case WP_EMPLACED_GUN:
		case WP_BOT_LASER:
		case WP_TURRET:
		case WP_TIE_FIGHTER:
		case WP_RAPID_FIRE_CONC:
		case WP_JAWA:
		case WP_TUSKEN_RIFLE:
		case WP_TUSKEN_STAFF:
		case WP_SCEPTER:
		case WP_NOGHRI_STICK:
			// Is Gunner...
			return qtrue;
		default:
			// NOT Gunner...
			break;
		}
	}

	return qfalse;
}

static qboolean Holding_Gun_And_Walking_And_Blocking(const gentity_t* self)
{
	const qboolean holding_walking_button = (cg.predictedPlayerState.pm_flags & PMF_WALKING_HELD) ? qtrue : qfalse;
	const qboolean holding_block_button = (cg.predictedPlayerState.pm_flags & PMF_BLOCK_HELD) ? qtrue : qfalse;

	if (holding_block_button && holding_walking_button)
	{
		switch (self->client->ps.weapon)
		{
		case WP_MELEE:
		case WP_STUN_BATON:
		case WP_BLASTER_PISTOL:
		case WP_BLASTER:
		case WP_DISRUPTOR:
		case WP_BOWCASTER:
		case WP_REPEATER:
		case WP_DEMP2:
		case WP_FLECHETTE:
		case WP_ROCKET_LAUNCHER:
		case WP_THERMAL:
		case WP_TRIP_MINE:
		case WP_DET_PACK:
		case WP_CONCUSSION:
		case WP_ATST_MAIN:
		case WP_ATST_SIDE:
		case WP_BRYAR_PISTOL:
		case WP_SBD_PISTOL:
		case WP_EMPLACED_GUN:
		case WP_BOT_LASER:
		case WP_TURRET:
		case WP_TIE_FIGHTER:
		case WP_RAPID_FIRE_CONC:
		case WP_JAWA:
		case WP_TUSKEN_RIFLE:
		case WP_TUSKEN_STAFF:
		case WP_SCEPTER:
		case WP_NOGHRI_STICK:
			// Is Gunner...
			return qtrue;
		default:
			// NOT Gunner...
			break;
		}
	}

	return qfalse;
}

static qboolean Holding_Saber_And_Its_Turned_Off(const gentity_t* self)
{
	const qboolean holding_walking_button = (cg.predictedPlayerState.pm_flags & PMF_WALKING_HELD) ? qtrue : qfalse;

	if (holding_walking_button)
	{
		if ((cg.snap->ps.weapon == WP_SABER) && (!cg.snap->ps.SaberActive()))
		{
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean Holding_Saber_And_Its_Turned_On(const gentity_t* self)
{
	const qboolean holding_walking_button = (cg.predictedPlayerState.pm_flags & PMF_WALKING_HELD) ? qtrue : qfalse;

	if (holding_walking_button)
	{
		if ((cg.snap->ps.weapon == WP_SABER) && (cg.snap->ps.SaberActive()))
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/
extern void CG_BuildSolidList();
extern void CG_ClearHealthBarEnts();
extern void CG_ClearBlockPointBarEnts();
extern cvar_t* in_joystick;
extern vec3_t serverViewOrg;
static qboolean cg_rangedFogging = qfalse; //so we know if we should go back to normal fog

void CG_DrawActiveFrame(const int serverTime, const stereoFrame_t stereoView)
{
	qboolean inwater = qfalse;

	cg.time = serverTime;

	// update cvars
	CG_UpdateCvars();

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if (cg.infoScreenText[0] != 0)
	{
		CG_DrawInformation();
		return;
	}

	CG_ClearHealthBarEnts();

	CG_ClearBlockPointBarEnts();

	CG_RunLightStyles();

	// any looped sounds will be respecified as entities
	// are added to the render list
	cgi_S_ClearLoopingSounds();

	// clear all the render lists
	cgi_R_ClearScene();

	CG_BuildSolidList();

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();
	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if (!cg.snap)
	{
		return;
	}

	// make sure the lagometer Sample and frame timing isn't done twice when in stereo
	if (stereoView != STEREO_RIGHT)
	{
		cg.frametime = cg.time - cg.oldTime;
		cg.oldTime = cg.time;
	}
	// Make sure the helper has the updated time
	theFxHelper.AdjustTime(cg.frametime);

	// let the client system know what our weapon and zoom settings are
	const int fpActive = cg_entities[0].gent->client->ps.forcePowersActive;
	const bool matrixMode = !!(fpActive & (1 << FP_SPEED | 1 << FP_RAGE));
	float speed = cg.refdef.fov_y / 75.0 * (matrixMode ? 1.0f : cg_timescale.value);

	static bool wasForceSpeed = false;
	const bool isForceSpeed = cg_entities[0].gent->client->ps.forcePowersActive & 1 << FP_SPEED ? true : false;
	if (isForceSpeed && !wasForceSpeed)
	{
		CGCam_Smooth(0.75f, 5000);
	}
	wasForceSpeed = isForceSpeed;

	float mPitchOverride = 0.0f;
	float mYawOverride = 0.0f;

	if (cg.snap->ps.clientNum == 0 && cg_scaleVehicleSensitivity.integer)
	{ // Only scale the sensitivity for the local player, not bots or spectators
		//pointless check, but..
		if (cg_entities[0].gent != nullptr &&
			(cg_entities[0].gent->s.eFlags & EF_LOCKED_TO_WEAPON))
		{
			speed *= 0.25f;
		}
		if (cg_entities[0].gent != nullptr &&
			cg_entities[0].gent->s.eFlags & EF_IN_ATST)
		{
			mPitchOverride = 0.01f;
			mYawOverride = 0.0075f;
		}
		const Vehicle_t* p_veh;

		// Mouse turns slower.
		if ((p_veh = G_IsRidingVehicle(&g_entities[0])) != nullptr)
		{
			if (p_veh->m_pVehicleInfo->mousePitch)
			{
				if (!in_joystick->integer)
				{
					mPitchOverride = p_veh->m_pVehicleInfo->mousePitch;
				}
				else
				{
					mPitchOverride = 0.08f;
				}
			}
			if (p_veh->m_pVehicleInfo->mouseYaw)
			{
				if (!in_joystick->integer)
				{
					mYawOverride = p_veh->m_pVehicleInfo->mouseYaw;
				}
				else
				{
					mYawOverride = 0.08f;
				}
			}
		}
	}

	// ---------------------------------------------------------
	// Precision mode for joystick: slow aim when WALK held
	// ---------------------------------------------------------

	if (cg.snap->ps.clientNum == 0 && (in_joystick->integer && cg_scaleJoystickSensitivity.integer))
	{
		if (Holding_Gun_And_Walking(&g_entities[0]))
		{
			mPitchOverride = 0.05f; //slow down the pitch
			mYawOverride = 0.05f; //slow down the yaw
		}
		if (Holding_Gun_And_Walking_And_Blocking(&g_entities[0]))
		{
			mPitchOverride = 0.025f; //slow down the pitch for aiming guns
			mYawOverride = 0.025f; //slow down the yaw  for aiming guns
		}
		if (Holding_Saber_And_Its_Turned_On(&g_entities[0]))
		{
			mPitchOverride = 0.10f; // faster up and downpitch for saber combat
			mYawOverride = 0.10f; // faster side to side yaw for saber dueling
		}
		if (Holding_Saber_And_Its_Turned_Off(&g_entities[0]))
		{
			mPitchOverride = 0.10f; //slow down the pitch
			mYawOverride = 0.10f; //slow down the yaw
		}
	}

	cgi_SetUserCmdValue(cg.weaponSelect, speed, mPitchOverride, mYawOverride);

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

	if (cg.snap->ps.eFlags & EF_HELD_BY_SAND_CREATURE)
	{
		cg.zoomMode = 0;
	}

	// -------------------------------------------------------------------------
	// Decide on third‑person vs first‑person rendering.
	// -------------------------------------------------------------------------

	qboolean third_person = qfalse;

	// Base conditions for third person.
	if (cg_thirdPerson.integer != 0 ||
		cg.snap->ps.stats[STAT_HEALTH] <= 0 ||
		(cg.snap->ps.eFlags & EF_HELD_BY_SAND_CREATURE))
	{
		third_person = qtrue;
	}
	else
	{
		// NPC class‑based third person (ATST, DROIDEKA, etc.).
		if (g_entities[0].client != NULL)
		{
			if (g_entities[0].client->NPC_class == CLASS_ATST ||
				g_entities[0].client->NPC_class == CLASS_DROIDEKA)
			{
				third_person = qtrue;
			}
		}

		// Weapon‑based third person rules.
		if (third_person == qfalse)
		{
			const int weapon = cg.snap->ps.weapon;

			// Saber/melee in third person unless forced first‑person saber is active.
			if ((weapon == WP_SABER || weapon == WP_MELEE) && cg_fpls.integer == 0)
			{
				third_person = qtrue;
			}
			// Emplaced gun in third person unless locked to weapon.
			else if (weapon == WP_EMPLACED_GUN &&
				!(cg.snap->ps.eFlags & EF_LOCKED_TO_WEAPON))
			{
				third_person = qtrue;
			}
			// Tusken rifle / Noghri stick special attack third person when trueguns is off.
			else if (cg_trueguns.integer == 0 &&
				(weapon == WP_TUSKEN_RIFLE || weapon == WP_NOGHRI_STICK) &&
				cg.snap->ps.torsoAnim >= BOTH_TUSKENATTACK1 &&
				cg.snap->ps.torsoAnim <= BOTH_TUSKENATTACK3)
			{
				third_person = qtrue;
			}
		}
	}

	// Saber/melee inversion rules.
	if (cg_fpls.integer != 0 &&
		cg_trueinvertsaber.integer == 2 &&
		(cg.snap->ps.weapon == WP_SABER || cg.snap->ps.weapon == WP_MELEE))
	{
		// Force third person for sabers/melee if cg_trueinvertsaber == 2.
		third_person = qtrue;
	}
	else if (cg_fpls.integer != 0 &&
		cg_trueinvertsaber.integer == 1 &&
		cg_thirdPerson.integer == 0 &&
		(cg.snap->ps.weapon == WP_SABER || cg.snap->ps.weapon == WP_MELEE))
	{
		// Force third person when normally first person.
		third_person = qtrue;
	}
	else if (cg_fpls.integer != 0 &&
		cg_trueinvertsaber.integer == 1 &&
		cg_thirdPerson.integer != 0 &&
		(cg.snap->ps.weapon == WP_SABER || cg.snap->ps.weapon == WP_MELEE))
	{
		// Force first person when normally third person.
		third_person = qfalse;
	}

	// Zoomed characters should never do third person stuff.
	if (cg.zoomMode != 0)
	{
		third_person = qfalse;
	}

	cg.renderingThirdPerson = (third_person == qtrue) ? qtrue : qfalse;

	// -------------------------------------------------------------------------
	// Camera / view calculation.
	// -------------------------------------------------------------------------

	if (in_camera)
	{
		// The scripted camera takes over the view.
		CGCam_RenderScene();
	}
	else
	{
		// Finish any fading that was happening.
		CGCam_UpdateFade();

		// Build cg.refdef (view origin, angles, FOV, etc.).
		inwater = CG_CalcViewValues();
	}

	// Zoom‑based fog handling.
	if (cg.zoomMode != 0)
	{
		// Zooming with binoculars or sniper: set the fog range based on the zoom level.
		cg_rangedFogging = qtrue;
		// Smaller FOV means less fog between the view and cull distance.
		cgi_R_SetRangeFog(cg.refdef.fov_x * 64.0f);
	}
	else if (cg_rangedFogging == qtrue)
	{
		// Disable ranged fog when no longer zoomed.
		cg_rangedFogging = qfalse;
		cgi_R_SetRangeFog(0.0f);
	}

	cg.refdef.time = cg.time;

	CG_DrawSkyBoxPortal();

	// NOTE: this may completely override the camera
	CG_RunEmplacedWeapon();

	// first person blend blobs, done after AnglesToAxis
	if (!cg.renderingThirdPerson)
	{
		CG_DamageBlendBlob();
	}

	// build the render lists
	if (!cg.hyperspace)
	{
		CG_AddPacketEntities(qfalse); // adter calcViewValues, so predicted player state is correct
		CG_AddMarks();
		CG_DrawMiscEnts();
	}

	//check for opaque water
	// this game does not have opaque water
	if (0)
	{
		vec3_t	camTest;
		VectorCopy(cg.refdef.vieworg, camTest);
		camTest[2] += 6;
		if (!(CG_PointContents(camTest, 0) & CONTENTS_SOLID) && !gi.inPVS(cg.refdef.vieworg, camTest))
		{//crossed visible line into another room
			cg.refdef.vieworg[2] -= 6;
		}
		else
		{
			VectorCopy(cg.refdef.vieworg, camTest);
			camTest[2] -= 6;
			if (!(CG_PointContents(camTest, 0) & CONTENTS_SOLID) && !gi.inPVS(cg.refdef.vieworg, camTest))
			{
				cg.refdef.vieworg[2] += 6;
			}
		}
	}
	{//actual view org and server's view org don't match and aren't same PVS, rebuild the areamask
		//Com_Printf( S_COLOR_RED"%s != %s\n", vtos(cg.refdef.vieworg), vtos(cg.snap->ps.serverViewOrg) );
		cgi_CM_SnapPVS(cg.refdef.vieworg, cg.snap->areamask);
	}

	// Don't draw the in-view weapon when in camera mode
	if (!in_camera
		&& !cg_pano.integer
		&& cg.snap->ps.weapon != WP_SABER
		&& (cg.snap->ps.viewEntity == 0 || cg.snap->ps.viewEntity >= ENTITYNUM_WORLD))
	{
		CG_AddViewWeapon(&cg.predictedPlayerState);
	}
	else if (cg.snap->ps.viewEntity != 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD)
	{
		if (g_entities[cg.snap->ps.viewEntity].client && g_entities[cg.snap->ps.viewEntity].NPC)
		{
			CG_AddViewWeapon(&g_entities[cg.snap->ps.viewEntity].client->ps);
		}
	}

	if (!cg.hyperspace && fx_freeze.integer < 2)
	{
		//Add all effects
		theFxScheduler.AddScheduledEffects(false);
	}

	// finish up the rest of the refdef
	if (cg.testModelEntity.hModel)
	{
		CG_AddTestModel();
	}

	if (!cg.hyperspace)
	{
		CG_AddLocalEntities();
	}

	memcpy(cg.refdef.areamask, cg.snap->areamask, sizeof cg.refdef.areamask);

	// update audio positions
	//This is done from the vieworg to get origin for non-attenuated sounds
	cgi_S_UpdateAmbientSet(CG_ConfigString(CS_AMBIENT_SET), cg.refdef.vieworg);
	//NOTE: if we want to make you be able to hear far away sounds with electrobinoculars, add the hacked-in positional offset here (base on fov)

	cgi_S_Respatialize(cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater);

	// warning sounds when powerup is wearing off
	CG_PowerupTimerSounds();

	if (cg_pano.integer)
	{
		// let's grab a panorama!
		cg.levelShot = qtrue; //hide the 2d
		VectorClear(cg.refdefViewAngles);
		cg.refdefViewAngles[YAW] = static_cast<float>(-360) * cg_pano.integer / cg_panoNumShots.integer; //choose angle
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		CG_DrawActive(stereoView);
		cg.levelShot = qfalse;
	}
	else
	{
		// actually issue the rendering calls
		CG_DrawActive(stereoView);
	}
}

//Checks to see if the current camera position is valid based on the last known safe location.  If it's not safe, place
//the camera at the last position safe location
void CheckCameraLocation(vec3_t oldeye_origin)
{
	trace_t trace;

	CG_Trace(&trace, oldeye_origin, cameramins, cameramaxs, cg.refdef.vieworg, cg.snap->ps.clientNum, MASK_CAMERACLIP);
	if (trace.fraction <= 1.0)
	{
		VectorCopy(trace.endpos, cg.refdef.vieworg);
	}
}