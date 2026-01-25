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

//NPC_utils.cpp

#include "b_local.h"
#include "Q3_Interface.h"
#include "g_navigator.h"
#include "../cgame/cg_local.h"
#include "g_nav.h"
#include "g_functions.h"

extern Vehicle_t* G_IsRidingVehicle(const gentity_t* pEnt);

int teamNumbers[TEAM_NUM_TEAMS];
int teamStrength[TEAM_NUM_TEAMS];
int teamCounter[TEAM_NUM_TEAMS];

constexpr auto VALID_ATTACK_CONE = 2.0f; //Degrees;
void GetAnglesForDirection(const vec3_t p1, const vec3_t p2, vec3_t out);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
extern void ViewHeightFix(const gentity_t* ent);
extern void AddLeanOfs(const gentity_t* ent, vec3_t point);
extern void SubtractLeanOfs(const gentity_t* ent, vec3_t point);

void CalcEntitySpot(const gentity_t* ent, const spot_t spot, vec3_t point)
{
	vec3_t forward, up, right;
	vec3_t start, end;
	trace_t tr;

	if (!ent)
	{
		return;
	}
	ViewHeightFix(ent);
	switch (spot)
	{
	case SPOT_ORIGIN:
		if (VectorCompare(ent->currentOrigin, vec3_origin))
		{
			//brush
			VectorSubtract(ent->absmax, ent->absmin, point); //size
			VectorMA(ent->absmin, 0.5, point, point);
		}
		else
		{
			VectorCopy(ent->currentOrigin, point);
		}
		break;

	case SPOT_CHEST:
	case SPOT_HEAD:
		if (ent->client && VectorLengthSquared(ent->client->renderInfo.eyePoint) && (ent->client->ps.viewEntity <= 0 ||
			ent->client->ps.viewEntity >= ENTITYNUM_WORLD))
		{
			//Actual tag_head eyespot!
			VectorCopy(ent->client->renderInfo.eyePoint, point);
			if (ent->client->NPC_class == CLASS_ATST)
			{
				//adjust up some
				point[2] += 28; //magic number :)
			}
			if (ent->NPC)
			{
				//always aim from the center of my bbox, so we don't wiggle when we lean forward or backwards
				point[0] = ent->currentOrigin[0];
				point[1] = ent->currentOrigin[1];
			}
			else if (!ent->s.number)
			{
				SubtractLeanOfs(ent, point);
			}
		}
		else
		{
			VectorCopy(ent->currentOrigin, point);
			if (ent->client)
			{
				point[2] += ent->client->ps.viewheight;
			}
		}
		if (spot == SPOT_CHEST && ent->client)
		{
			if (ent->client->NPC_class != CLASS_ATST)
			{
				//adjust up some
				point[2] -= ent->maxs[2] * 0.2f;
			}
		}
		break;

	case SPOT_HEAD_LEAN:
		if (ent->client && VectorLengthSquared(ent->client->renderInfo.eyePoint) && (ent->client->ps.viewEntity <= 0 ||
			ent->client->ps.viewEntity >= ENTITYNUM_WORLD))
		{
			//Actual tag_head eyespot!
			//FIXME: Stasis aliens may have a problem here...
			VectorCopy(ent->client->renderInfo.eyePoint, point);
			if (ent->client->NPC_class == CLASS_ATST)
			{
				//adjust up some
				point[2] += 28; //magic number :)
			}
			if (ent->NPC)
			{
				//always aim from the center of my bbox, so we don't wiggle when we lean forward or backwards
				point[0] = ent->currentOrigin[0];
				point[1] = ent->currentOrigin[1];
			}
			else if (!ent->s.number)
			{
				SubtractLeanOfs(ent, point);
			}
			//NOTE: automatically takes leaning into account!
		}
		else
		{
			VectorCopy(ent->currentOrigin, point);
			if (ent->client)
			{
				point[2] += ent->client->ps.viewheight;
			}
			//AddLeanOfs ( ent, point );
		}
		break;

		//FIXME: implement...
		//case SPOT_CHEST:
		//Returns point 3/4 from tag_torso to tag_head?
		//break;

	case SPOT_LEGS:
		VectorCopy(ent->currentOrigin, point);
		point[2] += ent->mins[2] * 0.5;
		break;

	case SPOT_WEAPON:
		if (ent->NPC && !VectorCompare(ent->NPC->shootAngles, vec3_origin) && !VectorCompare(
			ent->NPC->shootAngles, ent->client->ps.viewangles))
		{
			AngleVectors(ent->NPC->shootAngles, forward, right, up);
		}
		else
		{
			AngleVectors(ent->client->ps.viewangles, forward, right, up);
		}
		calcmuzzlePoint(const_cast<gentity_t*>(ent), forward, point, 0);
		//NOTE: automatically takes leaning into account!
		break;

	case SPOT_GROUND:
		// if entity is on the ground, just use it's absmin
		if (ent->s.groundEntityNum != -1)
		{
			VectorCopy(ent->currentOrigin, point);
			point[2] = ent->absmin[2];
			break;
		}

		// if it is reasonably close to the ground, give the point underneath of it
		VectorCopy(ent->currentOrigin, start);
		start[2] = ent->absmin[2];
		VectorCopy(start, end);
		end[2] -= 64;
		gi.trace(&tr, start, ent->mins, ent->maxs, end, ent->s.number, MASK_PLAYERSOLID, static_cast<EG2_Collision>(0),
			0);
		if (tr.fraction < 1.0)
		{
			VectorCopy(tr.endpos, point);
			break;
		}

		// otherwise just use the origin
		VectorCopy(ent->currentOrigin, point);
		break;

	default:
		VectorCopy(ent->currentOrigin, point);
		break;
	}
}

//===================================================================================

/*
qboolean NPC_UpdateAngles ( qboolean doPitch, qboolean doYaw )

Added: option to do just pitch or just yaw

Does not include "aim" in it's calculations

FIXME: stop compressing angles into shorts!!!!
*/
extern cvar_t* g_timescale;
extern bool NPC_IsTrooper(const gentity_t* ent);

qboolean NPC_UpdateAngles(const qboolean do_pitch, const qboolean do_yaw)
{
#if 1

	float error;
	float decay;
	float target_pitch = 0;
	float target_yaw = 0;
	float yaw_speed;
	qboolean exact = qtrue;

	// if angle changes are locked; just keep the current angles
	// aimTime isn't even set anymore... so this code was never reached, but I need a way to lock NPC's yaw, so instead of making a new SCF_ flag, just use the existing render flag... - dmv
	if (!NPC->enemy && (level.time < NPCInfo->aimTime || NPC->client->renderInfo.renderFlags & RF_LOCKEDANGLE))
	{
		if (do_pitch)
			target_pitch = NPCInfo->lockedDesiredPitch;

		if (do_yaw)
			target_yaw = NPCInfo->lockedDesiredYaw;
	}
	else
	{
		// we're changing the lockedDesired Pitch/Yaw below so it's lost it's original meaning, get rid of the lock flag
		NPC->client->renderInfo.renderFlags &= ~RF_LOCKEDANGLE;

		if (do_pitch)
		{
			target_pitch = NPCInfo->desiredPitch;
			NPCInfo->lockedDesiredPitch = NPCInfo->desiredPitch;
		}

		if (do_yaw)
		{
			target_yaw = NPCInfo->desiredYaw;
			NPCInfo->lockedDesiredYaw = NPCInfo->desiredYaw;
		}
	}

	if (NPC->s.weapon == WP_EMPLACED_GUN)
	{
		// FIXME: this seems to do nothing, actually...
		yaw_speed = 20;
	}
	else
	{
		if (NPC->client->NPC_class == CLASS_ROCKETTROOPER
			&& !NPC->enemy)
		{
			//just slowly lookin' around
			yaw_speed = 1;
		}
		else
		{
			yaw_speed = NPCInfo->stats.yawSpeed;
		}
	}

	if (NPC->s.weapon == WP_SABER && NPC->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		yaw_speed *= 1.0f / g_timescale->value;
	}

	if (!NPC_IsTrooper(NPC)
		&& NPC->enemy
		&& !G_IsRidingVehicle(NPC)
		&& NPC->client->NPC_class != CLASS_VEHICLE)
	{
		if (NPC->s.weapon == WP_BLASTER_PISTOL ||
			NPC->s.weapon == WP_BLASTER ||
			NPC->s.weapon == WP_BOWCASTER ||
			NPC->s.weapon == WP_REPEATER ||
			NPC->s.weapon == WP_FLECHETTE ||
			NPC->s.weapon == WP_JAWA ||
			NPC->s.weapon == WP_BRYAR_PISTOL ||
			NPC->s.weapon == WP_SBD_PISTOL ||
			NPC->s.weapon == WP_WRIST_BLASTER ||
			NPC->s.weapon == WP_DROIDEKA ||
			NPC->s.weapon == WP_NOGHRI_STICK)
		{
			yaw_speed *= 10.0f;
		}
	}

	if (do_yaw)
	{
		// decay yaw error
		error = AngleDelta(NPC->client->ps.viewangles[YAW], target_yaw);
		if (fabs(error) > MIN_ANGLE_ERROR)
		{
			if (error)
			{
				exact = qfalse;

				decay = 60.0 + yaw_speed * 3;
				decay *= 50.0f / 1000.0f; //msec

				if (error < 0.0)
				{
					error += decay;
					if (error > 0.0)
					{
						error = 0.0;
					}
				}
				else
				{
					error -= decay;
					if (error < 0.0)
					{
						error = 0.0;
					}
				}
			}
		}

		ucmd.angles[YAW] = ANGLE2SHORT(target_yaw + error) - client->ps.delta_angles[YAW];
	}

	//FIXME: have a pitchSpeed?
	if (do_pitch)
	{
		// decay pitch error
		error = AngleDelta(NPC->client->ps.viewangles[PITCH], target_pitch);
		if (fabs(error) > MIN_ANGLE_ERROR)
		{
			if (error)
			{
				exact = qfalse;

				decay = 60.0 + yaw_speed * 3;
				decay *= 50.0f / 1000.0f; //msec

				if (error < 0.0)
				{
					error += decay;
					if (error > 0.0)
					{
						error = 0.0;
					}
				}
				else
				{
					error -= decay;
					if (error < 0.0)
					{
						error = 0.0;
					}
				}
			}
		}

		ucmd.angles[PITCH] = ANGLE2SHORT(target_pitch + error) - client->ps.delta_angles[PITCH];
	}

	ucmd.angles[ROLL] = ANGLE2SHORT(NPC->client->ps.viewangles[ROLL]) - client->ps.delta_angles[ROLL];

	if (exact && Q3_TaskIDPending(NPC, TID_ANGLE_FACE))
	{
		Q3_TaskIDComplete(NPC, TID_ANGLE_FACE);
	}
	return exact;

#else

	float		error;
	float		decay;
	float		targetPitch = 0;
	float		targetYaw = 0;
	float		yawSpeed;
	//float		runningMod = NPCInfo->currentSpeed/100.0f;
	qboolean	exact = qtrue;
	qboolean	doSound = qfalse;

	// if angle changes are locked; just keep the current angles
	if (level.time < NPCInfo->aimTime)
	{
		if (doPitch)
			targetPitch = NPCInfo->lockedDesiredPitch;
		if (doYaw)
			targetYaw = NPCInfo->lockedDesiredYaw;
	}
	else
	{
		if (doPitch)
			targetPitch = NPCInfo->desiredPitch;
		if (doYaw)
			targetYaw = NPCInfo->desiredYaw;

		//		NPCInfo->aimTime = level.time + 250;
		if (doPitch)
			NPCInfo->lockedDesiredPitch = NPCInfo->desiredPitch;
		if (doYaw)
			NPCInfo->lockedDesiredYaw = NPCInfo->desiredYaw;
	}

	yawSpeed = NPCInfo->stats.yawSpeed;

	if (doYaw)
	{
		// decay yaw error
		error = AngleDelta(NPC->client->ps.viewangles[YAW], targetYaw);
		if (fabs(error) > MIN_ANGLE_ERROR)
		{
			if (error)
			{
				exact = qfalse;

				decay = 60.0 + yawSpeed * 3;
				decay *= 50.0 / 1000.0;//msec

				if (error < 0.0)
				{
					error += decay;
					if (error > 0.0)
					{
						error = 0.0;
					}
				}
				else
				{
					error -= decay;
					if (error < 0.0)
					{
						error = 0.0;
					}
				}
			}
		}
		ucmd.angles[YAW] = ANGLE2SHORT(targetYaw + error) - client->ps.delta_angles[YAW];
	}

	//FIXME: have a pitchSpeed?
	if (doPitch)
	{
		// decay pitch error
		error = AngleDelta(NPC->client->ps.viewangles[PITCH], targetPitch);
		if (fabs(error) > MIN_ANGLE_ERROR)
		{
			if (error)
			{
				exact = qfalse;

				decay = 60.0 + yawSpeed * 3;
				decay *= 50.0 / 1000.0;//msec

				if (error < 0.0)
				{
					error += decay;
					if (error > 0.0)
					{
						error = 0.0;
					}
				}
				else
				{
					error -= decay;
					if (error < 0.0)
					{
						error = 0.0;
					}
				}
			}
		}
		ucmd.angles[PITCH] = ANGLE2SHORT(targetPitch + error) - client->ps.delta_angles[PITCH];
	}

	ucmd.angles[ROLL] = ANGLE2SHORT(NPC->client->ps.viewangles[ROLL]) - client->ps.delta_angles[ROLL];

	/*
	if(doSound)
	{
		G_Sound(NPC, G_SoundIndex(va("sound/enemies/borg/borgservo%d.wav", Q_irand(1, 8))));
	}
	*/

	return exact;

#endif
}

void NPC_AimWiggle(vec3_t enemy_org)
{
	//shoot for somewhere between the head and torso
	//NOTE: yes, I know this looks weird, but it works
	if (NPCInfo->aimErrorDebounceTime < level.time)
	{
		NPCInfo->aimOfs[0] = 0.3 * Q_flrand(NPC->enemy->mins[0], NPC->enemy->maxs[0]);
		NPCInfo->aimOfs[1] = 0.3 * Q_flrand(NPC->enemy->mins[1], NPC->enemy->maxs[1]);
		if (NPC->enemy->maxs[2] > 0)
		{
			NPCInfo->aimOfs[2] = NPC->enemy->maxs[2] * Q_flrand(0.0f, -1.0f);
		}
	}
	VectorAdd(enemy_org, NPCInfo->aimOfs, enemy_org);
}

/*
qboolean NPC_UpdateFiringAngles ( qboolean doPitch, qboolean doYaw )

  Includes aim when determining angles - so they don't always hit...
  */
qboolean NPC_UpdateFiringAngles(const qboolean do_pitch, const qboolean do_yaw)
{
#if 0

	float		diff;
	float		error;
	float		targetPitch = 0;
	float		targetYaw = 0;
	qboolean	exact = qtrue;

	if (level.time < NPCInfo->aimTime)
	{
		if (doPitch)
			targetPitch = NPCInfo->lockedDesiredPitch;

		if (doYaw)
			targetYaw = NPCInfo->lockedDesiredYaw;
	}
	else
	{
		if (doPitch)
		{
			targetPitch = NPCInfo->desiredPitch;
			NPCInfo->lockedDesiredPitch = NPCInfo->desiredPitch;
		}

		if (doYaw)
		{
			targetYaw = NPCInfo->desiredYaw;
			NPCInfo->lockedDesiredYaw = NPCInfo->desiredYaw;
		}
	}

	if (doYaw)
	{
		// add yaw error based on NPCInfo->aim value
		error = ((float)(6 - NPCInfo->stats.aim)) * Q_flrand(-1, 1);

		if (Q_irand(0, 1))
			error *= -1;

		diff = AngleDelta(NPC->client->ps.viewangles[YAW], targetYaw);

		if (diff)
			exact = qfalse;

		ucmd.angles[YAW] = ANGLE2SHORT(targetYaw + diff + error) - client->ps.delta_angles[YAW];
	}

	if (doPitch)
	{
		// add pitch error based on NPCInfo->aim value
		error = ((float)(6 - NPCInfo->stats.aim)) * Q_flrand(-1, 1);

		diff = AngleDelta(NPC->client->ps.viewangles[PITCH], targetPitch);

		if (diff)
			exact = qfalse;

		ucmd.angles[PITCH] = ANGLE2SHORT(targetPitch + diff + error) - client->ps.delta_angles[PITCH];
	}

	ucmd.angles[ROLL] = ANGLE2SHORT(NPC->client->ps.viewangles[ROLL]) - client->ps.delta_angles[ROLL];

	return exact;

#else

	float error, diff;
	float decay;
	float target_pitch = 0;
	float target_yaw = 0;
	qboolean exact = qtrue;

	// if angle changes are locked; just keep the current angles
	if (level.time < NPCInfo->aimTime)
	{
		if (do_pitch)
			target_pitch = NPCInfo->lockedDesiredPitch;
		if (do_yaw)
			target_yaw = NPCInfo->lockedDesiredYaw;
	}
	else
	{
		if (do_pitch)
			target_pitch = NPCInfo->desiredPitch;
		if (do_yaw)
			target_yaw = NPCInfo->desiredYaw;

		//		NPCInfo->aimTime = level.time + 250;
		if (do_pitch)
			NPCInfo->lockedDesiredPitch = NPCInfo->desiredPitch;
		if (do_yaw)
			NPCInfo->lockedDesiredYaw = NPCInfo->desiredYaw;
	}

	if (NPCInfo->aimErrorDebounceTime < level.time)
	{
		if (Q_irand(0, 1))
		{
			NPCInfo->lastAimErrorYaw = static_cast<float>(6 - NPCInfo->stats.aim) * Q_flrand(-1, 1);
		}
		if (Q_irand(0, 1))
		{
			NPCInfo->lastAimErrorPitch = static_cast<float>(6 - NPCInfo->stats.aim) * Q_flrand(-1, 1);
		}
		NPCInfo->aimErrorDebounceTime = level.time + Q_irand(250, 2000);
	}

	if (do_yaw)
	{
		// decay yaw diff
		diff = AngleDelta(NPC->client->ps.viewangles[YAW], target_yaw);

		if (diff)
		{
			exact = qfalse;

			decay = 60.0 + 80.0;
			decay *= 50.0f / 1000.0f; //msec
			if (diff < 0.0)
			{
				diff += decay;
				if (diff > 0.0)
				{
					diff = 0.0;
				}
			}
			else
			{
				diff -= decay;
				if (diff < 0.0)
				{
					diff = 0.0;
				}
			}
		}

		// add yaw error based on NPCInfo->aim value
		error = NPCInfo->lastAimErrorYaw;

		/*
		if(Q_irand(0, 1))
		{
			error *= -1;
		}
		*/

		ucmd.angles[YAW] = ANGLE2SHORT(target_yaw + diff + error) - client->ps.delta_angles[YAW];
	}

	if (do_pitch)
	{
		// decay pitch diff
		diff = AngleDelta(NPC->client->ps.viewangles[PITCH], target_pitch);
		if (diff)
		{
			exact = qfalse;

			decay = 60.0 + 80.0;
			decay *= 50.0f / 1000.0f; //msec
			if (diff < 0.0)
			{
				diff += decay;
				if (diff > 0.0)
				{
					diff = 0.0;
				}
			}
			else
			{
				diff -= decay;
				if (diff < 0.0)
				{
					diff = 0.0;
				}
			}
		}

		error = NPCInfo->lastAimErrorPitch;

		ucmd.angles[PITCH] = ANGLE2SHORT(target_pitch + diff + error) - client->ps.delta_angles[PITCH];
	}

	ucmd.angles[ROLL] = ANGLE2SHORT(NPC->client->ps.viewangles[ROLL]) - client->ps.delta_angles[ROLL];

	return exact;

#endif
}

//===================================================================================

/*
static void NPC_UpdateShootAngles (vec3_t angles, qboolean doPitch, qboolean doYaw )

Does update angles on shootAngles
*/

void NPC_UpdateShootAngles(vec3_t angles, const qboolean do_pitch, const qboolean do_yaw)
{
	//FIXME: shoot angles either not set right or not used!
	float error;
	float decay;
	float target_pitch = 0;
	float target_yaw = 0;

	if (do_pitch)
		target_pitch = angles[PITCH];
	if (do_yaw)
		target_yaw = angles[YAW];

	if (do_yaw)
	{
		// decay yaw error
		error = AngleDelta(NPCInfo->shootAngles[YAW], target_yaw);
		if (error)
		{
			decay = 60.0 + 80.0 * NPCInfo->stats.aim;
			decay *= 100.0f / 1000.0f; //msec
			if (error < 0.0)
			{
				error += decay;
				if (error > 0.0)
				{
					error = 0.0;
				}
			}
			else
			{
				error -= decay;
				if (error < 0.0)
				{
					error = 0.0;
				}
			}
		}
		NPCInfo->shootAngles[YAW] = target_yaw + error;
	}

	if (do_pitch)
	{
		// decay pitch error
		error = AngleDelta(NPCInfo->shootAngles[PITCH], target_pitch);
		if (error)
		{
			decay = 60.0 + 80.0 * NPCInfo->stats.aim;
			decay *= 100.0f / 1000.0f; //msec
			if (error < 0.0)
			{
				error += decay;
				if (error > 0.0)
				{
					error = 0.0;
				}
			}
			else
			{
				error -= decay;
				if (error < 0.0)
				{
					error = 0.0;
				}
			}
		}
		NPCInfo->shootAngles[PITCH] = target_pitch + error;
	}
}

/*
void SetTeamNumbers (void)

Sets the number of living clients on each team

FIXME: Does not account for non-respawned players!
FIXME: Don't include medics?
*/
void SetTeamNumbers()
{
	int i;

	for (i = 0; i < TEAM_NUM_TEAMS; i++)
	{
		teamNumbers[i] = 0;
		teamStrength[i] = 0;
	}

	for (i = 0; i < 1; i++)
	{
		const gentity_t* found = &g_entities[i];

		if (found->client)
		{
			if (found->health > 0) //FIXME: or if a player!
			{
				teamNumbers[found->client->playerTeam]++;
				teamStrength[found->client->playerTeam] += found->health;
			}
		}
	}

	for (i = 0; i < TEAM_NUM_TEAMS; i++)
	{
		//Get the average health
		teamStrength[i] = floor(static_cast<float>(teamStrength[i]) / static_cast<float>(teamNumbers[i]));
	}
}

extern stringID_table_t BSTable[];
extern stringID_table_t BSETTable[];

qboolean G_ActivateBehavior(gentity_t* self, const int bset)
{
	auto b_sid = static_cast<bState_t>(-1);

	if (!self)
	{
		return qfalse;
	}

	char* bs_name = self->behaviorSet[bset];

	if (!(VALIDSTRING(bs_name)))
	{
		return qfalse;
	}

	if (self->NPC)
	{
		b_sid = static_cast<bState_t>(GetIDForString(BSTable, bs_name));
	}

	if (b_sid != static_cast<bState_t>(-1))
	{
		self->NPC->tempBehavior = BS_DEFAULT;
		self->NPC->behaviorState = b_sid;
		if (b_sid == BS_SEARCH || b_sid == BS_WANDER)
		{
			//FIXME: Reimplement?
			if (self->waypoint != WAYPOINT_NONE)
			{
				NPC_BSSearchStart(self->waypoint, b_sid);
			}
			else
			{
				self->waypoint = NAV::GetNearestNode(self);
				if (self->waypoint != WAYPOINT_NONE)
				{
					NPC_BSSearchStart(self->waypoint, b_sid);
				}
			}
		}
	}
	else
	{
		Quake3Game()->DebugPrint(IGameInterface::WL_VERBOSE, "%s attempting to run bSet %s (%s)\n", self->targetname,
			GetStringForID(BSETTable, bset), bs_name);
		Quake3Game()->RunScript(self, bs_name);
	}
	return qtrue;
}

/*
=============================================================================

	Extended Functions

=============================================================================
*/

/*
-------------------------
NPC_ValidEnemy
-------------------------
*/

qboolean G_ValidEnemy(const gentity_t* self, const gentity_t* enemy)
{
	//Must be a valid pointer
	if (enemy == nullptr)
		return qfalse;

	//Must not be me
	if (enemy == self)
		return qfalse;

	//Must not be deleted
	if (enemy->inuse == qfalse)
		return qfalse;

	//Must be alive
	if (enemy->health <= 0)
		return qfalse;

	//In case they're in notarget mode
	if (enemy->flags & FL_NOTARGET)
		return qfalse;

	//Must be an NPC
	if (enemy->client == nullptr)
	{
		if (enemy->svFlags & SVF_NONNPC_ENEMY)
		{
			//still potentially valid
			if (self->client)
			{
				if (enemy->noDamageTeam == self->client->playerTeam)
				{
					return qfalse;
				}
				return qtrue;
			}
			if (enemy->noDamageTeam == self->noDamageTeam)
			{
				return qfalse;
			}
			return qtrue;
		}
		return qfalse;
	}

	if (enemy->client->playerTeam == TEAM_FREE && enemy->s.number < MAX_CLIENTS)
	{
		//An evil player, everyone attacks him
		return qtrue;
	}

	if (self->client->playerTeam == TEAM_SOLO || enemy->client->playerTeam == TEAM_SOLO)
	{
		//	A new team which will attack anyone and everyone.
		return qtrue;
	}

	//Can't be on the same team
	if (enemy->client->playerTeam == self->client->playerTeam)
	{
		return qfalse;
	}

	if (enemy->client->playerTeam == self->client->enemyTeam //simplest case: they're on my enemy team
		|| self->client->enemyTeam == TEAM_FREE && enemy->client->NPC_class != self->client->NPC_class
		//I get mad at anyone and this guy isn't the same class as me
		|| enemy->client->NPC_class == CLASS_WAMPA && enemy->enemy //a rampaging wampa
		|| enemy->client->NPC_class == CLASS_RANCOR && enemy->enemy //a rampaging rancor
		|| enemy->client->playerTeam == TEAM_FREE && enemy->client->enemyTeam == TEAM_FREE && enemy->enemy && enemy->
		enemy->client && (enemy->enemy->client->playerTeam == self->client->playerTeam || enemy->enemy->client->
			playerTeam != TEAM_ENEMY && self->client->playerTeam == TEAM_PLAYER)
		//enemy is a rampaging non-aligned creature who is attacking someone on our team or a non-enemy (this last condition is used only if we're a good guy - in effect, we protect the innocent)
		|| self->client->enemyTeam == TEAM_FREE && enemy->client->NPC_class == self->client->NPC_class)
	{
		return qtrue;
	}

	//all other cases = false?
	return qfalse;
}

qboolean NPC_ValidEnemy(const gentity_t* ent)
{
	return G_ValidEnemy(NPC, ent);
}

/*
-------------------------
NPC_TargetVisible
-------------------------
*/

static qboolean NPC_TargetVisible(const gentity_t* ent)
{
	//Make sure we're in a valid range
	if (DistanceSquared(ent->currentOrigin, NPC->currentOrigin) > NPCInfo->stats.visrange * NPCInfo->stats.visrange)
		return qfalse;

	//Check our FOV
	if (InFOV(ent, NPC, NPCInfo->stats.hfov, NPCInfo->stats.vfov) == qfalse)
		return qfalse;

	//Check for sight
	if (NPC_ClearLOS(ent) == qfalse)
		return qfalse;

	return qtrue;
}

/*
-------------------------
NPC_FindNearestEnemy
-------------------------
*/

constexpr auto MAX_RADIUS_ENTS = 256; //NOTE: This can cause entities to be lost;
constexpr auto NEAR_DEFAULT_RADIUS = 256;
extern gentity_t* G_CheckControlledTurretEnemy(const gentity_t* self, gentity_t* enemy, qboolean validate);

static int NPC_FindNearestEnemy(const gentity_t* ent)
{
	gentity_t* radius_ents[MAX_RADIUS_ENTS];
	vec3_t mins{}, maxs{};
	int nearest_ent_id = -1;
	float nearest_dist = static_cast<float>(WORLD_SIZE) * static_cast<float>(WORLD_SIZE);
	int num_checks = 0;
	int i;

	//Setup the bbox to search in
	for (i = 0; i < 3; i++)
	{
		mins[i] = ent->currentOrigin[i] - NPCInfo->stats.visrange;
		maxs[i] = ent->currentOrigin[i] + NPCInfo->stats.visrange;
	}

	//Get a number of entities in a given space
	const int num_ents = gi.EntitiesInBox(mins, maxs, radius_ents, MAX_RADIUS_ENTS);

	for (i = 0; i < num_ents; i++)
	{
		const gentity_t* nearest = G_CheckControlledTurretEnemy(ent, radius_ents[i], qtrue);

		//Don't consider self
		if (nearest == ent)
			continue;

		//Must be valid
		if (NPC_ValidEnemy(nearest) == qfalse)
			continue;

		num_checks++;
		//Must be visible
		if (NPC_TargetVisible(nearest) == qfalse)
			continue;

		const float distance = DistanceSquared(ent->currentOrigin, nearest->currentOrigin);

		//Found one closer to us
		if (distance < nearest_dist)
		{
			nearest_ent_id = nearest->s.number;
			nearest_dist = distance;
		}
	}

	return nearest_ent_id;
}

/*
-------------------------
NPC_PickEnemyExt
-------------------------
*/

static gentity_t* NPC_PickEnemyExt(const qboolean check_alerts = qfalse)
{
	//If we've asked for the closest enemy
	const int entID = NPC_FindNearestEnemy(NPC);

	//If we have a valid enemy, use it
	if (entID >= 0)
		return &g_entities[entID];

	if (check_alerts)
	{
		const int alert_event = NPC_CheckAlertEvents(qtrue, qtrue, -1, qtrue, AEL_DISCOVERED);

		//There is an event to look at
		if (alert_event >= 0)
		{
			const alertEvent_t* event = &level.alertEvents[alert_event];

			//Don't pay attention to our own alerts
			if (event->owner == NPC)
				return nullptr;

			if (event->level >= AEL_DISCOVERED)
			{
				//If it's the player, attack him
				if (event->owner == &g_entities[0] && NPC_ValidEnemy(event->owner))
					return event->owner;

				//If it's on our team, then take its enemy as well
				if (event->owner->client && event->owner->client->playerTeam == NPC->client->playerTeam &&
					NPC_ValidEnemy(event->owner))
					return event->owner->enemy;
			}
		}
	}

	return nullptr;
}

/*
-------------------------
NPC_FindPlayer
-------------------------
*/

qboolean NPC_FindPlayer()
{
	return NPC_TargetVisible(&g_entities[0]);
}

/*
-------------------------
NPC_CheckPlayerDistance
-------------------------
*/

static qboolean NPC_CheckPlayerDistance()
{
	//Make sure we have an enemy
	if (NPC->enemy == nullptr)
		return qfalse;

	//Only do this for non-players
	if (NPC->enemy->s.number == 0)
		return qfalse;

	//must be set up to get mad at player
	if (!NPC->client || NPC->client->enemyTeam != TEAM_PLAYER)
		return qfalse;

	//Must be within our FOV
	if (InFOV(&g_entities[0], NPC, NPCInfo->stats.hfov, NPCInfo->stats.vfov) == qfalse)
		return qfalse;

	const float distance = DistanceSquared(NPC->currentOrigin, NPC->enemy->currentOrigin);

	if (distance > DistanceSquared(NPC->currentOrigin, g_entities[0].currentOrigin))
	{
		G_SetEnemy(NPC, &g_entities[0]);
		return qtrue;
	}

	return qfalse;
}

/*
-------------------------
NPC_FindEnemy
-------------------------
*/
static qboolean NPC_FindEnemy(const qboolean check_alerts = qfalse)
{
	gentity_t* newenemy = NPC_PickEnemyExt(check_alerts);

	//We're ignoring all enemies for now
	if (NPC->svFlags & SVF_IGNORE_ENEMIES)
	{
		G_ClearEnemy(NPC);
		return qfalse;
	}

	//we can't pick up any enemies for now
	if (NPCInfo->confusionTime > level.time)
	{
		G_ClearEnemy(NPC);
		return qfalse;
	}

	if (NPCInfo->insanityTime > level.time)
	{
		G_ClearEnemy(NPC);
		return qfalse;
	}

	//Don't want a new enemy
	if (NPC_ValidEnemy(NPC->enemy) && NPC->svFlags & SVF_LOCKEDENEMY)
	{
		return qtrue;
	}

	//See if the player is closer than our current enemy
	if (NPC->client->NPC_class != CLASS_RANCOR
		&& NPC->client->NPC_class != CLASS_WAMPA
		&& NPC->client->NPC_class != CLASS_SAND_CREATURE
		&& NPC_CheckPlayerDistance())
	{
		//rancors, wampas & sand creatures don't care if player is closer, they always go with closest
		return qtrue;
	}

	//Otherwise, turn off the flag
	NPC->svFlags &= ~SVF_LOCKEDENEMY;

	//If we've gotten here alright, then our target it still valid
	if (NPC_ValidEnemy(NPC->enemy))
	{
		return qtrue;
	}

	//if we found one, take it as the enemy
	if (NPC_ValidEnemy(newenemy))
	{
		G_SetEnemy(NPC, newenemy);
		return qtrue;
	}

	G_ClearEnemy(NPC);
	return qfalse;
}

/*
-------------------------
NPC_CheckEnemyExt
-------------------------
*/

qboolean NPC_CheckEnemyExt(const qboolean check_alerts)
{
	return NPC_FindEnemy(check_alerts);
}

/*
-------------------------
NPC_FacePosition
-------------------------
*/

qboolean NPC_FacePosition(vec3_t position, const qboolean do_pitch)
{
	vec3_t muzzle;
	qboolean facing = qtrue;

	//Get the positions
	if (NPC->client && (NPC->client->NPC_class == CLASS_RANCOR || NPC->client->NPC_class == CLASS_WAMPA || NPC->client->
		NPC_class == CLASS_SAND_CREATURE))
	{
		CalcEntitySpot(NPC, SPOT_ORIGIN, muzzle);
		muzzle[2] += NPC->maxs[2] * 0.75f;
	}
	else if (NPC->client && NPC->client->NPC_class == CLASS_GALAKMECH)
	{
		CalcEntitySpot(NPC, SPOT_WEAPON, muzzle);
	}
	else
	{
		CalcEntitySpot(NPC, SPOT_HEAD_LEAN, muzzle); //SPOT_HEAD
		if (NPC->client && NPC->client->NPC_class == CLASS_ROCKETTROOPER)
		{
			//*sigh*, look down more
			position[2] -= 32;
		}
	}

	//Find the desired angles
	vec3_t angles;

	GetAnglesForDirection(muzzle, position, angles);

	NPCInfo->desiredYaw = AngleNormalize360(angles[YAW]);
	NPCInfo->desiredPitch = AngleNormalize360(angles[PITCH]);

	if (NPC->enemy && NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_ATST)
	{
		// FIXME: this is kind of dumb, but it was the easiest way to get it to look sort of ok
		NPCInfo->desiredYaw += Q_flrand(-5, 5) + sin(level.time * 0.004f) * 7;
		NPCInfo->desiredPitch += Q_flrand(-2, 2);
	}
	//Face that yaw
	NPC_UpdateAngles(qtrue, qtrue);

	//Find the delta between our goal and our current facing
	const float yaw_delta = AngleNormalize360(
		NPCInfo->desiredYaw - SHORT2ANGLE(ucmd.angles[YAW] + client->ps.delta_angles[YAW]));

	//See if we are facing properly
	if (fabs(yaw_delta) > VALID_ATTACK_CONE)
		facing = qfalse;

	if (do_pitch)
	{
		//Find the delta between our goal and our current facing
		const float current_angles = SHORT2ANGLE(ucmd.angles[PITCH] + client->ps.delta_angles[PITCH]);
		const float pitch_delta = NPCInfo->desiredPitch - current_angles;

		//See if we are facing properly
		if (fabs(pitch_delta) > VALID_ATTACK_CONE)
			facing = qfalse;
	}

	return facing;
}

/*
-------------------------
NPC_FaceEntity
-------------------------
*/

qboolean NPC_FaceEntity(const gentity_t* ent, const qboolean do_pitch)
{
	vec3_t ent_pos;

	//Get the positions
	CalcEntitySpot(ent, SPOT_HEAD_LEAN, ent_pos);

	return NPC_FacePosition(ent_pos, do_pitch);
}

/*
-------------------------
NPC_FaceEnemy
-------------------------
*/

qboolean NPC_FaceEnemy(const qboolean do_pitch)
{
	if (NPC == nullptr)
		return qfalse;

	if (NPC->enemy == nullptr)
		return qfalse;

	return NPC_FaceEntity(NPC->enemy, do_pitch);
}

/*
-------------------------
NPC_CheckCanAttackExt
-------------------------
*/

qboolean NPC_CheckCanAttackExt()
{
	//We don't want them to shoot
	if (NPCInfo->scriptFlags & SCF_DONT_FIRE)
		return qfalse;

	//Turn to face
	if (NPC_FaceEnemy(qtrue) == qfalse)
		return qfalse;

	//Must have a clear line of sight to the target
	if (NPC_ClearShot(NPC->enemy) == qfalse)
		return qfalse;

	return qtrue;
}

/*
-------------------------
NPC_ClearLookTarget
-------------------------
*/

void NPC_ClearLookTarget(const gentity_t* self)
{
	if (!self->client)
	{
		return;
	}

	self->client->renderInfo.lookTarget = ENTITYNUM_NONE; //ENTITYNUM_WORLD;
	self->client->renderInfo.lookTargetClearTime = 0;
}

/*
-------------------------
NPC_SetLookTarget
-------------------------
*/
void NPC_SetLookTarget(const gentity_t* self, const int entNum, const int clear_time)
{
	if (!self->client)
	{
		return;
	}

	self->client->renderInfo.lookTarget = entNum;
	self->client->renderInfo.lookTargetClearTime = clear_time;
}

/*
-------------------------
NPC_CheckLookTarget
-------------------------
*/
qboolean NPC_CheckLookTarget(const gentity_t* self)
{
	if (self->client)
	{
		if (self->client->renderInfo.lookTarget >= 0 && self->client->renderInfo.lookTarget < ENTITYNUM_WORLD)
		{
			//within valid range
			if (&g_entities[self->client->renderInfo.lookTarget] == nullptr || !g_entities[self->client->renderInfo.
				lookTarget].inuse)
			{
				//lookTarget not inuse or not valid anymore
				NPC_ClearLookTarget(self);
			}
			else if (self->client->renderInfo.lookTargetClearTime && self->client->renderInfo.lookTargetClearTime <
				level.time)
			{
				//Time to clear lookTarget
				NPC_ClearLookTarget(self);
			}
			else if (g_entities[self->client->renderInfo.lookTarget].client && self->enemy && &g_entities[self->client->
				renderInfo.lookTarget] != self->enemy)
			{
				//should always look at current enemy if engaged in battle... FIXME: this could override certain scripted lookTargets...???
				NPC_ClearLookTarget(self);
			}
			else
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
-------------------------
NPC_CheckCharmed
-------------------------
*/
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
extern qboolean PM_HasAnimation(const gentity_t* ent, int animation);

void G_CheckCharmed(gentity_t* self)
{
	if (self
		&& self->client
		&& self->client->playerTeam == TEAM_PLAYER
		&& self->NPC
		&& self->NPC->charmedTime
		&& (self->NPC->charmedTime < level.time || self->health <= 0))
	{
		//we were charmed, set us back!
		//NOTE: presumptions here...
		const team_t sav_team = self->client->enemyTeam;
		self->client->enemyTeam = self->client->playerTeam;
		self->client->playerTeam = sav_team;
		self->client->leader = nullptr;
		self->NPC->charmedTime = 0;
		if (self->health > 0)
		{
			if (self->NPC->tempBehavior == BS_FOLLOW_LEADER)
			{
				self->NPC->tempBehavior = BS_DEFAULT;
			}
			G_ClearEnemy(self);
			//say something to let player know you've snapped out of it
			G_AddVoiceEvent(self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000);
		}
	}
	else if (self
		&& self->client
		&& self->client->playerTeam == TEAM_PLAYER
		&& self->NPC
		&& self->NPC->darkCharmedTime
		&& (self->NPC->darkCharmedTime < level.time || self->health <= 0))
	{
		//we were charmed, set us back!
		//NOTE: presumptions here...
		const team_t sav_team = self->client->enemyTeam;
		self->client->enemyTeam = self->client->playerTeam;
		self->client->playerTeam = sav_team;
		self->client->leader = nullptr;
		self->NPC->darkCharmedTime = 0;
		if (self->health > 0)
		{
			if (self->NPC->tempBehavior == BS_FOLLOW_LEADER)
			{
				self->NPC->tempBehavior = BS_DEFAULT;
			}
			G_ClearEnemy(self);
		}
		if (self->health > 0)
		{
			self->health = 0;
			GEntity_DieFunc(self, self, self, self->max_health, MOD_DESTRUCTION);
		}
	}
}

static void G_CheckInsanity(gentity_t* self)
{
	if (self
		&& self->client
		&& self->NPC
		&& self->NPC->insanityTime
		&& self->NPC->insanityTime > level.time
		&& self->client->ps.torsoAnim != BOTH_SONICPAIN_HOLD
		&& PM_HasAnimation(self, BOTH_SONICPAIN_HOLD))
	{
		NPC_SetAnim(self, SETANIM_LEGS, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_NORMAL | SETANIM_FLAG_RESTART);
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_SONICPAIN_HOLD,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
		self->client->ps.torsoAnimTimer += self->NPC->insanityTime - level.time;
	}
}

void G_GetBoltPosition(gentity_t* self, const int boltIndex, vec3_t pos, const int modelIndex = 0)
{
	if (!self || !self->ghoul2.size())
	{
		return;
	}
	mdxaBone_t boltMatrix;
	const vec3_t angles = { 0, self->currentAngles[YAW], 0 };

	gi.G2API_GetBoltMatrix(self->ghoul2, modelIndex,
		boltIndex,
		&boltMatrix, angles, self->currentOrigin, cg.time ? cg.time : level.time,
		nullptr, self->s.modelScale);
	if (pos)
	{
		vec3_t result;
		gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, result);
		VectorCopy(result, pos);
	}
}

float NPC_EntRangeFromBolt(const gentity_t* targ_ent, const int boltIndex)
{
	vec3_t org = { 0.0f };

	if (!targ_ent)
	{
		return Q3_INFINITE;
	}

	G_GetBoltPosition(NPC, boltIndex, org);

	return Distance(targ_ent->currentOrigin, org);
}

float NPC_EnemyRangeFromBolt(const int boltIndex)
{
	return NPC_EntRangeFromBolt(NPC->enemy, boltIndex);
}

int G_GetEntsNearBolt(gentity_t* self, gentity_t** radius_ents, const float radius, const int boltIndex,
	vec3_t bolt_org)
{
	vec3_t mins{}, maxs{};

	//get my handRBolt's position
	vec3_t org = { 0.0f };

	G_GetBoltPosition(self, boltIndex, org);

	VectorCopy(org, bolt_org);

	//Setup the bbox to search in
	for (int i = 0; i < 3; i++)
	{
		mins[i] = bolt_org[i] - radius;
		maxs[i] = bolt_org[i] + radius;
	}

	//Get the number of entities in a given space
	return gi.EntitiesInBox(mins, maxs, radius_ents, 128);
}

int NPC_GetEntsNearBolt(gentity_t** radius_ents, const float radius, const int boltIndex, vec3_t bolt_org)
{
	return G_GetEntsNearBolt(NPC, radius_ents, radius, boltIndex, bolt_org);
}

extern qboolean RT_Flying(const gentity_t* self);
extern void RT_FlyStart(gentity_t* self);
extern void RT_FlyStop(gentity_t* self);
extern qboolean Boba_Flying(const gentity_t* self);
extern void Boba_FlyStart(gentity_t* self);
extern void Boba_FlyStop(gentity_t* self);

qboolean JET_Flying(const gentity_t* self)
{
	if (!self || !self->client)
	{
		return qfalse;
	}
	if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO)
	{
		return Boba_Flying(self);
	}
	if (self->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		return RT_Flying(self);
	}
	return qfalse;
}

void JET_FlyStart(gentity_t* self)
{
	if (!self || !self->client)
	{
		return;
	}
	self->lastInAirTime = level.time;
	if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO)
	{
		Boba_FlyStart(self);
	}
	else if (self->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		RT_FlyStart(self);
	}
}

extern qboolean G_ControlledByPlayer(const gentity_t* self);
static qboolean rocket_trooper_player(const gentity_t* self)
{
	if (self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self))
	{
		if (self->client->NPC_class == CLASS_ROCKETTROOPER)
		{
			return qtrue;
		}
	}
	return qfalse;
}

void jet_fly_stop(gentity_t* self)
{
	if (!self || !self->client)
	{
		return;
	}
	if (self->client->NPC_class == CLASS_BOBAFETT ||
		self->client->NPC_class == CLASS_MANDO ||
		self->client->NPC_class == CLASS_ROCKETTROOPER && rocket_trooper_player(self))
	{
		Boba_FlyStop(self);
	}
	else if (self->client->NPC_class == CLASS_ROCKETTROOPER && !rocket_trooper_player(self))
	{
		RT_FlyStop(self);
	}
	else
	{
		Boba_FlyStop(self);
	}
}