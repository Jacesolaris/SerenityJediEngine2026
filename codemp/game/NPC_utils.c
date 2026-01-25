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

//NPC_utils.cpp

#include "b_local.h"
#include "icarus/Q3_Interface.h"
#include "ghoul2/G2.h"

int teamNumbers[TEAM_NUM_TEAMS];
int teamStrength[TEAM_NUM_TEAMS];
int teamCounter[TEAM_NUM_TEAMS];

#define	VALID_ATTACK_CONE	2.0f	//Degrees
extern void G_DebugPrint(int level, const char* format, ...);

/*
void CalcEntitySpot ( gentity_t *ent, spot_t spot, vec3_t point )

Added: Uses shootAngles if a NPC has them

*/
void CalcEntitySpot(const gentity_t* ent, const spot_t spot, vec3_t point)
{
	vec3_t forward, up, right;
	vec3_t start, end;
	trace_t tr;

	if (!ent)
	{
		return;
	}
	switch (spot)
	{
	case SPOT_ORIGIN:
		if (VectorCompare(ent->r.currentOrigin, vec3_origin))
		{
			//brush
			VectorSubtract(ent->r.absmax, ent->r.absmin, point); //size
			VectorMA(ent->r.absmin, 0.5, point, point);
		}
		else
		{
			VectorCopy(ent->r.currentOrigin, point);
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
				point[0] = ent->r.currentOrigin[0];
				point[1] = ent->r.currentOrigin[1];
			}
		}
		else
		{
			VectorCopy(ent->r.currentOrigin, point);
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
				point[2] -= ent->r.maxs[2] * 0.2f;
			}
		}
		break;

	case SPOT_HEAD_LEAN:
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
				point[0] = ent->r.currentOrigin[0];
				point[1] = ent->r.currentOrigin[1];
			}
		}
		else
		{
			VectorCopy(ent->r.currentOrigin, point);
			if (ent->client)
			{
				point[2] += ent->client->ps.viewheight;
			}
			//AddLeanOfs ( ent, point );
		}
		break;

	case SPOT_LEGS:
		VectorCopy(ent->r.currentOrigin, point);
		point[2] += ent->r.mins[2] * 0.5;
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
		calcmuzzlePoint((gentity_t*)ent, forward, right, point);
		//NOTE: automatically takes leaning into account!
		break;

	case SPOT_GROUND:
		// if entity is on the ground, just use it's absmin
		if (ent->s.groundEntityNum != ENTITYNUM_NONE)
		{
			VectorCopy(ent->r.currentOrigin, point);
			point[2] = ent->r.absmin[2];
			break;
		}

		// if it is reasonably close to the ground, give the point underneath of it
		VectorCopy(ent->r.currentOrigin, start);
		start[2] = ent->r.absmin[2];
		VectorCopy(start, end);
		end[2] -= 64;
		trap->Trace(&tr, start, ent->r.mins, ent->r.maxs, end, ent->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);
		if (tr.fraction < 1.0)
		{
			VectorCopy(tr.endpos, point);
			break;
		}

		// otherwise just use the origin
		VectorCopy(ent->r.currentOrigin, point);
		break;

	default:
		VectorCopy(ent->r.currentOrigin, point);
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
qboolean NPC_UpdateAngles(const qboolean do_pitch, const qboolean do_yaw)
{
#if 1

	float error;
	float decay;
	float targetPitch = 0;
	float targetYaw = 0;
	float yawSpeed;
	qboolean exact = qtrue;

	// if angle changes are locked; just keep the current angles
	// aimTime isn't even set anymore... so this code was never reached, but I need a way to lock NPC's yaw, so instead of making a new SCF_ flag, just use the existing render flag... - dmv
	if (!NPCS.NPC->enemy && (level.time < NPCS.NPCInfo->aimTime || NPCS.NPC->client->renderInfo.renderFlags &
		RF_LOCKEDANGLE))
	{
		if (do_pitch)
			targetPitch = NPCS.NPCInfo->lockedDesiredPitch;

		if (do_yaw)
			targetYaw = NPCS.NPCInfo->lockedDesiredYaw;
	}
	else
	{
		// we're changing the lockedDesired Pitch/Yaw below so it's lost it's original meaning, get rid of the lock flag
		NPCS.NPC->client->renderInfo.renderFlags &= ~RF_LOCKEDANGLE;

		if (do_pitch)
		{
			targetPitch = NPCS.NPCInfo->desiredPitch;
			NPCS.NPCInfo->lockedDesiredPitch = NPCS.NPCInfo->desiredPitch;
		}

		if (do_yaw)
		{
			targetYaw = NPCS.NPCInfo->desiredYaw;
			NPCS.NPCInfo->lockedDesiredYaw = NPCS.NPCInfo->desiredYaw;
		}
	}

	if (NPCS.NPC->s.weapon == WP_EMPLACED_GUN)
	{
		// FIXME: this seems to do nothing, actually...
		yawSpeed = 20;
	}
	else
	{
		if (NPCS.NPC->client->NPC_class == CLASS_ROCKETTROOPER
			&& !NPCS.NPC->enemy)
		{
			//just slowly lookin' around
			yawSpeed = 1;
		}
		else
		{
			yawSpeed = NPCS.NPCInfo->stats.yawSpeed;
		}
	}

	if (NPCS.NPC->s.weapon == WP_SABER && NPCS.NPC->client->ps.fd.forcePowersActive & 1 << FP_SPEED)
	{
		char buf[128];

		trap->Cvar_VariableStringBuffer("timescale", buf, sizeof buf);

		const float tFVal = atof(buf);

		yawSpeed *= 1.0f / tFVal;
	}

	if (NPCS.NPCInfo->lookMode)
	{
		//We have a specific kind of looking we are doing, lower the lookMod, slower the yawspeed
		yawSpeed *= (float)NPCS.NPCInfo->lookMode / (float)LT_FULLFACE;
	}

	if (do_yaw)
	{
		// decay yaw error
		error = AngleDelta(NPCS.NPC->client->ps.viewangles[YAW], targetYaw);
		if (fabs(error) > MIN_ANGLE_ERROR)
		{
			if (error)
			{
				exact = qfalse;

				decay = 60.0 + yawSpeed * 3;
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

		NPCS.ucmd.angles[YAW] = ANGLE2SHORT(targetYaw + error) - NPCS.client->ps.delta_angles[YAW];
	}

	//FIXME: have a pitchSpeed?
	if (do_pitch)
	{
		// decay pitch error
		error = AngleDelta(NPCS.NPC->client->ps.viewangles[PITCH], targetPitch);
		if (fabs(error) > MIN_ANGLE_ERROR)
		{
			if (error)
			{
				exact = qfalse;

				decay = 60.0 + yawSpeed * 3;
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

		NPCS.ucmd.angles[PITCH] = ANGLE2SHORT(targetPitch + error) - NPCS.client->ps.delta_angles[PITCH];
	}

	NPCS.ucmd.angles[ROLL] = ANGLE2SHORT(NPCS.NPC->client->ps.viewangles[ROLL]) - NPCS.client->ps.delta_angles[ROLL];

	if (exact && trap->ICARUS_TaskIDPending((sharedEntity_t*)NPCS.NPC, TID_ANGLE_FACE))
	{
		trap->ICARUS_TaskIDComplete((sharedEntity_t*)NPCS.NPC, TID_ANGLE_FACE);
	}
	return exact;

#else

	float		error;
	float		decay;
	float		targetPitch = 0;
	float		targetYaw = 0;
	float		yawSpeed;
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

	return exact;

#endif
}

void NPC_AimWiggle(vec3_t enemy_org)
{
	//shoot for somewhere between the head and torso
	//NOTE: yes, I know this looks weird, but it works
	if (NPCS.NPCInfo->aimErrorDebounceTime < level.time)
	{
		NPCS.NPCInfo->aimOfs[0] = 0.3 * flrand(NPCS.NPC->enemy->r.mins[0], NPCS.NPC->enemy->r.maxs[0]);
		NPCS.NPCInfo->aimOfs[1] = 0.3 * flrand(NPCS.NPC->enemy->r.mins[1], NPCS.NPC->enemy->r.maxs[1]);
		if (NPCS.NPC->enemy->r.maxs[2] > 0)
		{
			NPCS.NPCInfo->aimOfs[2] = NPCS.NPC->enemy->r.maxs[2] * flrand(0.0f, -1.0f);
		}
	}
	VectorAdd(enemy_org, NPCS.NPCInfo->aimOfs, enemy_org);
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
		error = ((float)(6 - NPCInfo->stats.aim)) * flrand(-1, 1);

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
		error = ((float)(6 - NPCInfo->stats.aim)) * flrand(-1, 1);

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
	float targetPitch = 0;
	float targetYaw = 0;
	qboolean exact = qtrue;

	// if angle changes are locked; just keep the current angles
	if (level.time < NPCS.NPCInfo->aimTime)
	{
		if (do_pitch)
			targetPitch = NPCS.NPCInfo->lockedDesiredPitch;
		if (do_yaw)
			targetYaw = NPCS.NPCInfo->lockedDesiredYaw;
	}
	else
	{
		if (do_pitch)
			targetPitch = NPCS.NPCInfo->desiredPitch;
		if (do_yaw)
			targetYaw = NPCS.NPCInfo->desiredYaw;

		if (do_pitch)
			NPCS.NPCInfo->lockedDesiredPitch = NPCS.NPCInfo->desiredPitch;
		if (do_yaw)
			NPCS.NPCInfo->lockedDesiredYaw = NPCS.NPCInfo->desiredYaw;
	}

	if (NPCS.NPCInfo->aimErrorDebounceTime < level.time)
	{
		if (Q_irand(0, 1))
		{
			NPCS.NPCInfo->lastAimErrorYaw = (float)(6 - NPCS.NPCInfo->stats.aim) * flrand(-1, 1);
		}
		if (Q_irand(0, 1))
		{
			NPCS.NPCInfo->lastAimErrorPitch = (float)(6 - NPCS.NPCInfo->stats.aim) * flrand(-1, 1);
		}
		NPCS.NPCInfo->aimErrorDebounceTime = level.time + Q_irand(250, 2000);
	}

	if (do_yaw)
	{
		// decay yaw diff
		diff = AngleDelta(NPCS.NPC->client->ps.viewangles[YAW], targetYaw);

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
		error = NPCS.NPCInfo->lastAimErrorYaw;

		NPCS.ucmd.angles[YAW] = ANGLE2SHORT(targetYaw + diff + error) - NPCS.client->ps.delta_angles[YAW];
	}

	if (do_pitch)
	{
		// decay pitch diff
		diff = AngleDelta(NPCS.NPC->client->ps.viewangles[PITCH], targetPitch);
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

		error = NPCS.NPCInfo->lastAimErrorPitch;

		NPCS.ucmd.angles[PITCH] = ANGLE2SHORT(targetPitch + diff + error) - NPCS.client->ps.delta_angles[PITCH];
	}

	NPCS.ucmd.angles[ROLL] = ANGLE2SHORT(NPCS.NPC->client->ps.viewangles[ROLL]) - NPCS.client->ps.delta_angles[ROLL];

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
	float targetPitch = 0;
	float targetYaw = 0;

	if (do_pitch)
		targetPitch = angles[PITCH];
	if (do_yaw)
		targetYaw = angles[YAW];

	if (do_yaw)
	{
		// decay yaw error
		error = AngleDelta(NPCS.NPCInfo->shootAngles[YAW], targetYaw);
		if (error)
		{
			decay = 60.0 + 80.0 * NPCS.NPCInfo->stats.aim;
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
		NPCS.NPCInfo->shootAngles[YAW] = targetYaw + error;
	}

	if (do_pitch)
	{
		// decay pitch error
		error = AngleDelta(NPCS.NPCInfo->shootAngles[PITCH], targetPitch);
		if (error)
		{
			decay = 60.0 + 80.0 * NPCS.NPCInfo->stats.aim;
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
		NPCS.NPCInfo->shootAngles[PITCH] = targetPitch + error;
	}
}

/*
void SetTeamNumbers (void)

Sets the number of living clients on each team

FIXME: Does not account for non-respawned players!
FIXME: Don't include medics?
*/
void SetTeamNumbers(void)
{
	int i;

	for (i = 0; i < TEAM_NUM_TEAMS; i++)
	{
		teamNumbers[i] = 0;
		teamStrength[i] = 0;
	}

	//OJKFIXME: clientNum 0
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
		teamStrength[i] = floor((float)teamStrength[i] / (float)teamNumbers[i]);
	}
}

extern stringID_table_t BSTable[];
extern stringID_table_t BSETTable[];

qboolean G_ActivateBehavior(gentity_t* self, const int bset)
{
	bState_t b_sid = -1;

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
		b_sid = (bState_t)GetIDForString(BSTable, bs_name);
	}

	if (b_sid != (bState_t)-1)
	{
		self->NPC->tempBehavior = BS_DEFAULT;
		self->NPC->behaviorState = b_sid;
	}
	else
	{
		//make the code handle the case of the scripts directory already being given
		if (!Q_strncmp(bs_name, va("%s/", Q3_SCRIPT_DIR), 8))
		{
			//already has script directory specified.
			trap->ICARUS_RunScript((sharedEntity_t*)self, bs_name);
		}
		else
		{
			trap->ICARUS_RunScript((sharedEntity_t*)self, va("%s/%s", Q3_SCRIPT_DIR, bs_name));
		}
	}
	return qtrue;
}

/*
=============================================================================

	Extended Functions

=============================================================================
*/

//rww - special system for sync'ing bone angles between client and server.
void NPC_SetBoneAngles(gentity_t* ent, const char* bone, vec3_t angles)
{
	int* thebone = &ent->s.boneIndex1;
	int* first_free = NULL;
	int i = 0;
	const int boneIndex = G_BoneIndex(bone);
	vec3_t* bone_vector = &ent->s.boneAngles1;
	vec3_t* free_bone_vec = NULL;

	while (thebone)
	{
		if (!*thebone && !first_free)
		{
			//if the value is 0 then this index is clear, we can use it if we don't find the bone we want already existing.
			first_free = thebone;
			free_bone_vec = bone_vector;
		}
		else if (*thebone)
		{
			if (*thebone == boneIndex)
			{
				//this is it
				break;
			}
		}

		switch (i)
		{
		case 0:
			thebone = &ent->s.boneIndex2;
			bone_vector = &ent->s.boneAngles2;
			break;
		case 1:
			thebone = &ent->s.boneIndex3;
			bone_vector = &ent->s.boneAngles3;
			break;
		case 2:
			thebone = &ent->s.boneIndex4;
			bone_vector = &ent->s.boneAngles4;
			break;
		default:
			thebone = NULL;
			bone_vector = NULL;
			break;
		}

		i++;
	}

	if (!thebone)
	{
		//didn't find it, create it
		if (!first_free)
		{
			//no free bones.. can't do a thing then.
			Com_Printf("WARNING: NPC has no free bone indexes\n");
			return;
		}

		thebone = first_free;

		*thebone = boneIndex;
		bone_vector = free_bone_vec;
	}

	//If we got here then we have a vector and an index.

	//Copy the angles over the vector in the entitystate, so we can use the corresponding index
	//to set the bone angles on the client.
	VectorCopy(angles, *bone_vector);

	//Now set the angles on our server instance if we have one.

	if (!ent->ghoul2)
	{
		return;
	}

	const int flags = BONE_ANGLES_POSTMULT;
	const int up = POSITIVE_X;
	const int right = NEGATIVE_Y;
	const int forward = NEGATIVE_Z;

	//first 3 bits is forward, second 3 bits is right, third 3 bits is up
	ent->s.boneOrient = forward | right << 3 | up << 6;

	trap->G2API_SetBoneAngles(ent->ghoul2, 0, bone, angles, flags, up, right, forward, NULL, 100, level.time);
}

//rww - and another method of automatically managing surface status for the client and server at once
#define TURN_ON				0x00000000
#define TURN_OFF			0x00000100

void NPC_SetSurfaceOnOff(gentity_t* ent, const char* surfaceName, const int surfaceFlags)
{
	int i = 0;
	qboolean foundIt = qfalse;

	while (i < BG_NUM_TOGGLEABLE_SURFACES && bgToggleableSurfaces[i])
	{
		if (!Q_stricmp(surfaceName, bgToggleableSurfaces[i]))
		{
			//got it
			foundIt = qtrue;
			break;
		}
		i++;
	}

	if (!foundIt)
	{
		Com_Printf("WARNING: Tried to toggle NPC surface that isn't in toggleable surface list (%s)\n", surfaceName);
		return;
	}

	if (surfaceFlags == TURN_ON)
	{
		//Make sure the entitystate values reflect this surface as on now.
		ent->s.surfacesOn |= 1 << i;
		ent->s.surfacesOff &= ~(1 << i);
	}
	else
	{
		//Otherwise make sure they're off.
		ent->s.surfacesOn &= ~(1 << i);
		ent->s.surfacesOff |= 1 << i;
	}

	if (!ent->ghoul2)
	{
		return;
	}

	trap->G2API_SetSurfaceOnOff(ent->ghoul2, surfaceName, surfaceFlags);
}

//rww - cheap check to see if an armed client is looking in our general direction
qboolean NPC_SomeoneLookingAtMe(const gentity_t* ent)
{
	int i = 0;

	while (i < MAX_CLIENTS)
	{
		const gentity_t* pEnt = &g_entities[i];

		if (pEnt && pEnt->inuse && pEnt->client && pEnt->client->sess.sessionTeam != TEAM_SPECTATOR &&
			pEnt->client->tempSpectate < level.time && !(pEnt->client->ps.pm_flags & PMF_FOLLOW) && pEnt->s.weapon !=
			WP_NONE)
		{
			if (trap->InPVS(ent->r.currentOrigin, pEnt->r.currentOrigin))
			{
				if (InFOV(ent, pEnt, 30, 30))
				{
					//I'm in a 30 fov or so cone from this player.. that's enough I guess.
					return qtrue;
				}
			}
		}

		i++;
	}

	return qfalse;
}

qboolean NPC_ClearLOS(const vec3_t start, const vec3_t end)
{
	return G_ClearLOS(NPCS.NPC, start, end);
}

qboolean NPC_ClearLOS5(const vec3_t end)
{
	return G_ClearLOS5(NPCS.NPC, end);
}

qboolean NPC_ClearLOS4(gentity_t* ent)
{
	return G_ClearLOS4(NPCS.NPC, ent);
}

qboolean NPC_ClearLOS3(const vec3_t start, gentity_t* ent)
{
	return G_ClearLOS3(NPCS.NPC, start, ent);
}

qboolean NPC_ClearLOS2(gentity_t* ent, const vec3_t end)
{
	return G_ClearLOS2(NPCS.NPC, ent, end);
}

/*
-------------------------
NPC_TargetVisible
-------------------------
*/

static qboolean NPC_TargetVisible(gentity_t* ent)
{
	//Make sure we're in a valid range
	if (DistanceSquared(ent->r.currentOrigin, NPCS.NPC->r.currentOrigin) > NPCS.NPCInfo->stats.visrange * NPCS.NPCInfo->
		stats.visrange)
		return qfalse;

	//Check our FOV
	if (InFOV(ent, NPCS.NPC, NPCS.NPCInfo->stats.hfov, NPCS.NPCInfo->stats.vfov) == qfalse)
		return qfalse;

	//Check for sight
	if (NPC_ClearLOS4(ent) == qfalse)
		return qfalse;

	return qtrue;
}

/*
-------------------------
NPC_FindNearestEnemy
-------------------------
*/

#define	MAX_RADIUS_ENTS			256	//NOTE: This can cause entities to be lost
#define NEAR_DEFAULT_RADIUS		256
extern gentity_t* G_CheckControlledTurretEnemy(const gentity_t* self, gentity_t* enemy, qboolean validate);

static int NPC_FindNearestEnemy(const gentity_t* ent)
{
	int iradius_ents[MAX_RADIUS_ENTS];
	vec3_t mins, maxs;
	int nearestEntID = -1;
	float nearestDist = (float)WORLD_SIZE * (float)WORLD_SIZE;
	int numChecks = 0;
	int i;

	//Setup the bbox to search in
	for (i = 0; i < 3; i++)
	{
		mins[i] = ent->r.currentOrigin[i] - NPCS.NPCInfo->stats.visrange;
		maxs[i] = ent->r.currentOrigin[i] + NPCS.NPCInfo->stats.visrange;
	}

	//Get a number of entities in a given space
	const int num_ents = trap->EntitiesInBox(mins, maxs, iradius_ents, MAX_RADIUS_ENTS);

	for (i = 0; i < num_ents; i++)
	{
		//nearest = G_CheckControlledTurretEnemy(ent, iradius_ents[i], qtrue);
		gentity_t* radEnt = &g_entities[iradius_ents[i]];

		//Don't consider self
		if (radEnt == ent)
			continue;

		//Must be valid
		if (NPC_ValidEnemy(radEnt) == qfalse)
			continue;

		numChecks++;
		//Must be visible
		if (NPC_TargetVisible(radEnt) == qfalse)
			continue;

		const float distance = DistanceSquared(ent->r.currentOrigin, radEnt->r.currentOrigin);

		//Found one closer to us
		if (distance < nearestDist)
		{
			nearestEntID = radEnt->s.number;
			nearestDist = distance;
		}
	}

	return nearestEntID;
}

/*
-------------------------
NPC_PickEnemyExt
-------------------------
*/

static gentity_t* NPC_PickEnemyExt(const qboolean check_alerts)
{
	//If we've asked for the closest enemy
	const int entID = NPC_FindNearestEnemy(NPCS.NPC);

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
			if (event->owner == NPCS.NPC)
				return NULL;

			if (event->level >= AEL_DISCOVERED)
			{
				//If it's the player, attack him
				if (event->owner == &g_entities[0] && NPC_ValidEnemy(event->owner))
					return event->owner;

				//If it's on our team, then take its enemy as well
				if (event->owner->client && event->owner->client->playerTeam == NPCS.NPC->client->playerTeam &&
					NPC_ValidEnemy(event->owner))
					return event->owner->enemy;
			}
		}
	}

	return NULL;
}

/*
-------------------------
NPC_FindPlayer
-------------------------
*/

qboolean NPC_FindPlayer(void)
{
	return NPC_TargetVisible(&g_entities[0]);
}

/*
-------------------------
NPC_CheckPlayerDistance
-------------------------
*/

static qboolean NPC_CheckPlayerDistance(void)
{
	//racc - check for a closer player to the NPC than it's current enemy.
	int ClosestPlayer = -1; //current closest player

	//Make sure we have an enemy
	if (NPCS.NPC->enemy == NULL)
		return qfalse;

	float ClosestDistance = DistanceSquared(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin);

	//must be set up to get mad at player
	if (!NPCS.NPC->client || NPCS.NPC->client->enemyTeam != NPCTEAM_PLAYER)
		return qfalse;

	//go into the scan loop.
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		const gentity_t* player = &g_entities[i];

		if (!player->inuse || !player->client
			|| player->client->pers.connected != CON_CONNECTED
			|| player->client->sess.sessionTeam == TEAM_SPECTATOR
			|| player->health < 0)
		{
			//not a good player
			continue;
		}

		//Must be within our FOV
		if (InFOV(player, NPCS.NPC, NPCS.NPCInfo->stats.hfov, NPCS.NPCInfo->stats.vfov) == qfalse)
			continue;

		const float distance = DistanceSquared(NPCS.NPC->r.currentOrigin, player->r.currentOrigin);

		if (distance < ClosestDistance)
		{
			//we're closer than the current closest
			ClosestDistance = distance;
			ClosestPlayer = player->s.number;
		}
	}

	if (ClosestPlayer != -1)
	{
		if (ClosestDistance + 128 * 128 < DistanceSquared(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin))
		{
			//player has too be reasonably closer than the current enemy
			G_SetEnemy(NPCS.NPC, &g_entities[ClosestPlayer]);
			return qtrue;
		}
	}

	return qfalse;
}

/*
-------------------------
NPC_FindEnemy
-------------------------
*/

static qboolean NPC_FindEnemy(const qboolean check_alerts)
{
	//We're ignoring all enemies for now
	if (NPCS.NPC->NPC->scriptFlags & SCF_IGNORE_ENEMIES)
	{
		G_ClearEnemy(NPCS.NPC);
		return qfalse;
	}

	//we can't pick up any enemies for now
	if (NPCS.NPCInfo->confusionTime > level.time)
	{
		G_ClearEnemy(NPCS.NPC);
		return qfalse;
	}

	//Don't want a new enemy
	if (ValidEnemy(NPCS.NPC->enemy) && NPCS.NPC->NPC->aiFlags & NPCAI_LOCKEDENEMY)
		return qtrue;

	//See if the player is closer than our current enemy
	if (NPCS.NPC->client->NPC_class != CLASS_RANCOR
		&& NPCS.NPC->client->NPC_class != CLASS_WAMPA
		&& NPCS.NPC->client->NPC_class != CLASS_SAND_CREATURE
		&& NPC_CheckPlayerDistance())
	{
		//rancors, wampas & sand creatures don't care if player is closer, they always go with closest
		return qtrue;
	}

	NPCS.NPC->NPC->aiFlags &= ~NPCAI_LOCKEDENEMY;

	//If we've gotten here alright, then our target it still valid
	if (NPC_ValidEnemy(NPCS.NPC->enemy))
		return qtrue;

	gentity_t* newenemy = NPC_PickEnemyExt(check_alerts);

	//if we found one, take it as the enemy
	if (NPC_ValidEnemy(newenemy))
	{
		G_SetEnemy(NPCS.NPC, newenemy);
		return qtrue;
	}

	G_ClearEnemy(NPCS.NPC);
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
	vec3_t angles;
	qboolean facing = qtrue;

	//Get the positions
	if (NPCS.NPC->client && (NPCS.NPC->client->NPC_class == CLASS_RANCOR || NPCS.NPC->client->NPC_class == CLASS_WAMPA))
		// || NPC->client->NPC_class == CLASS_SAND_CREATURE) )
	{
		CalcEntitySpot(NPCS.NPC, SPOT_ORIGIN, muzzle);
		muzzle[2] += NPCS.NPC->r.maxs[2] * 0.75f;
	}
	else if (NPCS.NPC->client && NPCS.NPC->client->NPC_class == CLASS_GALAKMECH)
	{
		CalcEntitySpot(NPCS.NPC, SPOT_WEAPON, muzzle);
	}
	else
	{
		CalcEntitySpot(NPCS.NPC, SPOT_HEAD_LEAN, muzzle); //SPOT_HEAD
		if (NPCS.NPC->client->NPC_class == CLASS_ROCKETTROOPER)
		{
			//*sigh*, look down more
			position[2] -= 32;
		}
	}

	//Find the desired angles
	GetAnglesForDirection(muzzle, position, angles);

	NPCS.NPCInfo->desiredYaw = AngleNormalize360(angles[YAW]);
	NPCS.NPCInfo->desiredPitch = AngleNormalize360(angles[PITCH]);

	if (NPCS.NPC->enemy && NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->NPC_class == CLASS_ATST)
	{
		NPCS.NPCInfo->desiredYaw += flrand(-5, 5) + sin(level.time * 0.004f) * 7;
		NPCS.NPCInfo->desiredPitch += flrand(-2, 2);
	}
	//Face that yaw
	NPC_UpdateAngles(qtrue, qtrue);

	//Find the delta between our goal and our current facing
	const float yawDelta = AngleNormalize360(
		NPCS.NPCInfo->desiredYaw - SHORT2ANGLE(NPCS.ucmd.angles[YAW] + NPCS.client->ps.delta_angles[YAW]));

	//See if we are facing properly
	if (fabs(yawDelta) > VALID_ATTACK_CONE)
		facing = qfalse;

	if (do_pitch)
	{
		//Find the delta between our goal and our current facing
		const float currentAngles = SHORT2ANGLE(NPCS.ucmd.angles[PITCH] + NPCS.client->ps.delta_angles[PITCH]);
		const float pitchDelta = NPCS.NPCInfo->desiredPitch - currentAngles;

		//See if we are facing properly
		if (fabs(pitchDelta) > VALID_ATTACK_CONE)
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
	vec3_t entPos;

	//Get the positions
	CalcEntitySpot(ent, SPOT_HEAD_LEAN, entPos);

	return NPC_FacePosition(entPos, do_pitch);
}

/*
-------------------------
NPC_FaceEnemy
-------------------------
*/

qboolean NPC_FaceEnemy(const qboolean do_pitch)
{
	if (NPCS.NPC == NULL)
		return qfalse;

	if (NPCS.NPC->enemy == NULL)
		return qfalse;

	return NPC_FaceEntity(NPCS.NPC->enemy, do_pitch);
}

/*
-------------------------
NPC_CheckCanAttackExt
-------------------------
*/

qboolean NPC_CheckCanAttackExt(void)
{
	//We don't want them to shoot
	if (NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE)
		return qfalse;

	//Turn to face
	if (NPC_FaceEnemy(qtrue) == qfalse)
		return qfalse;

	//Must have a clear line of sight to the target
	if (NPC_ClearShot(NPCS.NPC->enemy) == qfalse)
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

	if (self->client->ps.eFlags2 & EF2_HELD_BY_MONSTER)
	{
		//lookTarget is set by and to the monster that's holding you, no other operations can change that
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

	if (self->client->ps.eFlags2 & EF2_HELD_BY_MONSTER)
	{
		//lookTarget is set by and to the monster that's holding you, no other operations can change that
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
			int lookTargetIdx = self->client->renderInfo.lookTarget;
			if (lookTargetIdx < 0 ||
				lookTargetIdx >= level.num_entities ||
				!g_entities[lookTargetIdx].inuse ||
				!g_entities[lookTargetIdx].client /* or other checks as needed */)
			{
				// handle invalid look target
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

void G_CheckCharmed(gentity_t* self)
{
	if (self
		&& self->client
		&& self->client->playerTeam == NPCTEAM_PLAYER
		&& self->NPC
		&& self->NPC->charmedTime
		&& (self->NPC->charmedTime < level.time || self->health <= 0))
	{
		//we were charmed, set us back!
		//NOTE: presumptions here...
		const team_t savTeam = (team_t)self->client->enemyTeam;
		self->client->enemyTeam = self->client->playerTeam;
		self->client->playerTeam = (npcteam_t)savTeam;
		self->client->leader = NULL;
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
}

void G_GetBoltPosition(gentity_t* self, const int boltIndex, vec3_t pos, const int modelIndex)
{
	mdxaBone_t boltMatrix;
	vec3_t angles;

	if (!self || !self->inuse)
	{
		return;
	}

	if (self->client)
	{
		//clients don't actually even keep r.currentAngles maintained
		VectorSet(angles, 0, self->client->ps.viewangles[YAW], 0);
	}
	else
	{
		VectorSet(angles, 0, self->r.currentAngles[YAW], 0);
	}

	if (/*!self || ...haha (sorry, i'm tired)*/ !self->ghoul2)
	{
		return;
	}

	trap->G2API_GetBoltMatrix(self->ghoul2, modelIndex,
		boltIndex,
		&boltMatrix, angles, self->r.currentOrigin, level.time,
		NULL, self->modelScale);
	if (pos)
	{
		vec3_t result;
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, result);
		VectorCopy(result, pos);
	}
}

float NPC_EntRangeFromBolt(const gentity_t* targ_ent, const int boltIndex)
{
	vec3_t org;

	if (!targ_ent)
	{
		return Q3_INFINITE;
	}

	G_GetBoltPosition(NPCS.NPC, boltIndex, org, 0);

	return Distance(targ_ent->r.currentOrigin, org);
}

float NPC_EnemyRangeFromBolt(const int boltIndex)
{
	return NPC_EntRangeFromBolt(NPCS.NPC->enemy, boltIndex);
}

int NPC_GetEntsNearBolt(int* radius_ents, const float radius, const int boltIndex, vec3_t bolt_org)
{
	vec3_t mins, maxs;

	//get my handRBolt's position
	vec3_t org;

	G_GetBoltPosition(NPCS.NPC, boltIndex, org, 0);

	VectorCopy(org, bolt_org);

	//Setup the bbox to search in
	for (int i = 0; i < 3; i++)
	{
		mins[i] = bolt_org[i] - radius;
		maxs[i] = bolt_org[i] + radius;
	}

	//Get the number of entities in a given space
	return trap->EntitiesInBox(mins, maxs, radius_ents, 128);
}

extern qboolean Boba_Flying(const gentity_t* self);
extern void Boba_FlyStart(gentity_t* self);
extern void Boba_FlyStop(gentity_t* self);

qboolean JET_Flying(const gentity_t* self)
{
	if (!self || !self->client)
	{
		return qfalse;
	}
	if (self->client->NPC_class == CLASS_BOBAFETT)
	{
		return Boba_Flying(self);
	}
	if (self->client->NPC_class == CLASS_MANDO)
	{
		return Boba_Flying(self);
	}
	if (self->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		return Boba_Flying(self);
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
	if (self->client->NPC_class == CLASS_BOBAFETT)
	{
		Boba_FlyStart(self);
	}
	else if (self->client->NPC_class == CLASS_MANDO)
	{
		Boba_FlyStart(self);
	}
	else if (self->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		Boba_FlyStart(self);
	}
}

void jet_fly_stop(gentity_t* self)
{
	if (!self || !self->client)
	{
		return;
	}
	if (self->client->NPC_class == CLASS_BOBAFETT)
	{
		Boba_FlyStop(self);
	}
	else if (self->client->NPC_class == CLASS_MANDO)
	{
		Boba_FlyStop(self);
	}
	else if (self->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		Boba_FlyStop(self);
	}
}

qboolean InPlayersPVS(vec3_t point)
{
	//checks to see if this point is visible to all the players in the game.
	int Counter = 0;

	for (; Counter < level.maxclients; Counter++)
	{
		const gentity_t* checkEnt = &g_entities[Counter];
		if (!checkEnt->inuse || !checkEnt->client
			|| checkEnt->client->pers.connected == CON_DISCONNECTED
			|| checkEnt->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			//this entity isn't going to be seeing anything
			continue;
		}

		if (trap->InPVS(point, checkEnt->client->ps.origin))
		{
			//can be seen
			return qtrue;
		}
	}

	return qfalse;
}