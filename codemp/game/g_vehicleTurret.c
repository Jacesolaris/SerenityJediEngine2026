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

#include "bg_vehicles.h"
#include "b_local.h"
#include "ghoul2/G2.h"

extern void G_SetEnemy(gentity_t* self, gentity_t* enemy);
extern void WP_CalcVehMuzzle(gentity_t* ent, int muzzle_num);
extern gentity_t* WP_FireVehicleWeapon(gentity_t* ent, vec3_t start, vec3_t dir, const vehWeaponInfo_t* veh_weapon,
	qboolean alt_fire, qboolean is_turret_weap);

extern void G_VehMuzzleFireFX(const gentity_t* ent, gentity_t* broadcaster, int muzzles_fired);
//-----------------------------------------------------
void VEH_TurretCheckFire(Vehicle_t* p_veh,
	gentity_t* parent,
	//gentity_t *turretEnemy,
	const turretStats_t* turretStats,
	const vehWeaponInfo_t* vehWeapon,
	const int turretNum, const int curMuzzle)
{
	// if it's time to fire and we have an enemy, then gun 'em down!  pushDebounce time controls next fire time
	if (p_veh->m_iMuzzleTag[curMuzzle] == -1)
	{
		//invalid muzzle?
		return;
	}

	if (p_veh->m_iMuzzleWait[curMuzzle] >= level.time)
	{
		//can't fire yet
		return;
	}

	if (p_veh->turretStatus[turretNum].ammo < vehWeapon->iAmmoPerShot)
	{
		//no ammo, can't fire
		return;
	}

	//if ( turretEnemy )
	{
		//FIXME: check to see if I'm aiming generally where I want to
		const int muzzlesFired = 1 << curMuzzle;
		WP_CalcVehMuzzle(parent, curMuzzle);

		//FIXME: some variation in fire dir
		gentity_t* missile = WP_FireVehicleWeapon(parent, p_veh->m_vMuzzlePos[curMuzzle], p_veh->m_vMuzzleDir[curMuzzle],
			vehWeapon, turretNum != 0, qtrue);

		//play the weapon's muzzle effect if we have one
		G_VehMuzzleFireFX(parent, missile, muzzlesFired);

		//take the ammo away
		p_veh->turretStatus[turretNum].ammo -= vehWeapon->iAmmoPerShot;
		//toggle to the next muzzle on this turret, if there is one
		const int nextMuzzle = curMuzzle + 1 == p_veh->m_pVehicleInfo->turret[turretNum].iMuzzle[0]
			? p_veh->m_pVehicleInfo->turret[turretNum].iMuzzle[1]
			: p_veh->m_pVehicleInfo->turret[turretNum].iMuzzle[0];
		if (nextMuzzle)
		{
			//a valid muzzle to toggle to
			p_veh->turretStatus[turretNum].nextMuzzle = nextMuzzle - 1;
			//-1 because you type muzzles 1-10 in the .veh file
		}
		//add delay to the next muzzle so it doesn't fire right away on the next frame
		p_veh->m_iMuzzleWait[p_veh->turretStatus[turretNum].nextMuzzle] = level.time + turretStats->iDelay;
	}
}

void VEH_TurretAnglesToEnemy(const Vehicle_t* p_veh, const int cur_muzzle, const float f_speed, const gentity_t* turret_enemy,
	const qboolean b_ai_lead,
	vec3_t desired_angles)
{
	vec3_t enemy_dir, org;
	VectorCopy(turret_enemy->r.currentOrigin, org);
	if (b_ai_lead)
	{
		//we want to lead them a bit
		vec3_t diff, velocity;
		VectorSubtract(org, p_veh->m_vMuzzlePos[cur_muzzle], diff);
		const float dist = VectorNormalize(diff);
		if (turret_enemy->client)
		{
			VectorCopy(turret_enemy->client->ps.velocity, velocity);
		}
		else
		{
			VectorCopy(turret_enemy->s.pos.trDelta, velocity);
		}
		VectorMA(org, dist / f_speed, velocity, org);
	}

	//FIXME: this isn't quite right, it's aiming from the muzzle, not the center of the turret...
	VectorSubtract(org, p_veh->m_vMuzzlePos[cur_muzzle], enemy_dir);
	//Get the desired absolute, world angles to our target
	vectoangles(enemy_dir, desired_angles);
}

//-----------------------------------------------------
qboolean VEH_TurretAim(Vehicle_t* p_veh,
	gentity_t* parent,
	const  gentity_t* turret_enemy,
	const turretStats_t* turret_stats,
	const vehWeaponInfo_t* veh_weapon,
	const int turret_num, const int cur_muzzle, vec3_t desired_angles)
	//-----------------------------------------------------
{
	vec3_t cur_angles, add_angles, new_angles;
	float aim_correct = qfalse;

	WP_CalcVehMuzzle(parent, cur_muzzle);
	//get the current absolute angles of the turret right now
	vectoangles(p_veh->m_vMuzzleDir[cur_muzzle], cur_angles);
	//subtract out the vehicle's angles to get the relative alignment
	AnglesSubtract(cur_angles, p_veh->m_vOrientation, cur_angles);

	if (turret_enemy)
	{
		aim_correct = qtrue;
		// ...then we'll calculate what new aim adjustments we should attempt to make this frame
		// Aim at enemy
		VEH_TurretAnglesToEnemy(p_veh, cur_muzzle, veh_weapon->fSpeed, turret_enemy, turret_stats->bAILead, desired_angles);
	}
	//subtract out the vehicle's angles to get the relative desired alignment
	AnglesSubtract(desired_angles, p_veh->m_vOrientation, desired_angles);
	//Now clamp the desired relative angles
	//clamp yaw
	desired_angles[YAW] = AngleNormalize180(desired_angles[YAW]);
	if (p_veh->m_pVehicleInfo->turret[turret_num].yawClampLeft
		&& desired_angles[YAW] > p_veh->m_pVehicleInfo->turret[turret_num].yawClampLeft)
	{
		aim_correct = qfalse;
		desired_angles[YAW] = p_veh->m_pVehicleInfo->turret[turret_num].yawClampLeft;
	}
	if (p_veh->m_pVehicleInfo->turret[turret_num].yawClampRight
		&& desired_angles[YAW] < p_veh->m_pVehicleInfo->turret[turret_num].yawClampRight)
	{
		aim_correct = qfalse;
		desired_angles[YAW] = p_veh->m_pVehicleInfo->turret[turret_num].yawClampRight;
	}
	//clamp pitch
	desired_angles[PITCH] = AngleNormalize180(desired_angles[PITCH]);
	if (p_veh->m_pVehicleInfo->turret[turret_num].pitchClampDown
		&& desired_angles[PITCH] > p_veh->m_pVehicleInfo->turret[turret_num].pitchClampDown)
	{
		aim_correct = qfalse;
		desired_angles[PITCH] = p_veh->m_pVehicleInfo->turret[turret_num].pitchClampDown;
	}
	if (p_veh->m_pVehicleInfo->turret[turret_num].pitchClampUp
		&& desired_angles[PITCH] < p_veh->m_pVehicleInfo->turret[turret_num].pitchClampUp)
	{
		aim_correct = qfalse;
		desired_angles[PITCH] = p_veh->m_pVehicleInfo->turret[turret_num].pitchClampUp;
	}
	//Now get the offset we want from our current relative angles
	AnglesSubtract(desired_angles, cur_angles, add_angles);
	//Now cap the addAngles for our fTurnSpeed
	if (add_angles[PITCH] > turret_stats->fTurnSpeed)
	{
		//aimCorrect = qfalse;//???
		add_angles[PITCH] = turret_stats->fTurnSpeed;
	}
	else if (add_angles[PITCH] < -turret_stats->fTurnSpeed)
	{
		//aimCorrect = qfalse;//???
		add_angles[PITCH] = -turret_stats->fTurnSpeed;
	}
	if (add_angles[YAW] > turret_stats->fTurnSpeed)
	{
		//aimCorrect = qfalse;//???
		add_angles[YAW] = turret_stats->fTurnSpeed;
	}
	else if (add_angles[YAW] < -turret_stats->fTurnSpeed)
	{
		//aimCorrect = qfalse;//???
		add_angles[YAW] = -turret_stats->fTurnSpeed;
	}
	//Now add the additional angles back in to our current relative angles
	//FIXME: add some AI aim error randomness...?
	new_angles[PITCH] = AngleNormalize180(cur_angles[PITCH] + add_angles[PITCH]);
	new_angles[YAW] = AngleNormalize180(cur_angles[YAW] + add_angles[YAW]);
	//Now set the bone angles to the new angles
	//set yaw
	if (turret_stats->yawBone)
	{
		vec3_t yawAngles;
		VectorClear(yawAngles);
		yawAngles[turret_stats->yawAxis] = new_angles[YAW];
		NPC_SetBoneAngles(parent, turret_stats->yawBone, yawAngles);
	}
	//set pitch
	if (turret_stats->pitchBone)
	{
		vec3_t pitchAngles;
		VectorClear(pitchAngles);
		pitchAngles[turret_stats->pitchAxis] = new_angles[PITCH];
		NPC_SetBoneAngles(parent, turret_stats->pitchBone, pitchAngles);
	}
	//force muzzle to recalc next check
	p_veh->m_iMuzzleTime[cur_muzzle] = 0;

	return aim_correct;
}

//-----------------------------------------------------
static qboolean VEH_TurretFindEnemies(Vehicle_t* p_veh,
	gentity_t* parent,
	const turretStats_t* turretStats,
	const int turretNum, const int curMuzzle)
	//-----------------------------------------------------
{
	qboolean found = qfalse;
	float bestDist = turretStats->fAIRange * turretStats->fAIRange;
	vec3_t org2;
	qboolean foundClient = qfalse;
	gentity_t* entity_list[MAX_GENTITIES];
	const gentity_t* bestTarget = NULL;

	if (!p_veh->m_pPilot)
	{
		// only try to find enemies if the vehicle has a pilot
		return qfalse;
	}

	WP_CalcVehMuzzle(parent, curMuzzle);
	VectorCopy(p_veh->m_vMuzzlePos[curMuzzle], org2);

	const int count = G_RadiusList(org2, turretStats->fAIRange, parent, qtrue, entity_list);

	for (int i = 0; i < count; i++)
	{
		vec3_t org;
		trace_t tr;
		const gentity_t* target = entity_list[i];

		if (target == parent
			|| !target->takedamage
			|| target->health <= 0
			|| target->flags & FL_NOTARGET)
		{
			continue;
		}
		if (!target->client)
		{
			// only attack clients
			if (!(target->flags & FL_BBRUSH) //not a breakable brush
				|| !target->takedamage //is a bbrush, but invincible
				|| target->NPC_targetname && parent->targetname && Q_stricmp(target->NPC_targetname, parent->targetname)
				!= 0) //not in invicible bbrush, but can only be broken by an NPC that is not me
			{
				if (target->s.weapon == WP_TURRET
					&& target->classname
					&& Q_strncmp("misc_turret", target->classname, 11) == 0)
				{
					//these guys we want to shoot at
				}
				else
				{
					continue;
				}
			}
			//else: we will shoot at bbrushes!
		}
		else if (target->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}
		else if (target->client->tempSpectate >= level.time)
		{
			continue;
		}
		if (target == (gentity_t*)p_veh->m_pPilot
			|| target->r.ownerNum == parent->s.number)
		{
			//don't get angry at my pilot or passengers?
			continue;
		}
		if (parent->client
			&& parent->client->sess.sessionTeam)
		{
			if (target->client)
			{
				if (target->client->sess.sessionTeam == parent->client->sess.sessionTeam)
				{
					// A bot/client/NPC we don't want to shoot
					continue;
				}
			}
			else if (target->teamnodmg == parent->client->sess.sessionTeam)
			{
				//some other entity that's allied with us
				continue;
			}
		}
		if (!trap->InPVS(org2, target->r.currentOrigin))
		{
			continue;
		}

		VectorCopy(target->r.currentOrigin, org);

		trap->Trace(&tr, org2, NULL, NULL, org, parent->s.number, MASK_SHOT, qfalse, 0, 0);

		if (tr.entityNum == target->s.number
			|| !tr.allsolid && !tr.startsolid && tr.fraction == 1.0)
		{
			vec3_t enemyDir;
			// Only acquire if have a clear shot, Is it in range and closer than our best?
			VectorSubtract(target->r.currentOrigin, org2, enemyDir);
			const float enemyDist = VectorLengthSquared(enemyDir);

			if (enemyDist < bestDist || target->client && !foundClient) // all things equal, keep current
			{
				bestTarget = target;
				bestDist = enemyDist;
				found = qtrue;
				if (target->client)
				{
					//prefer clients over non-clients
					foundClient = qtrue;
				}
			}
		}
	}

	if (found)
	{
		p_veh->turretStatus[turretNum].enemyEntNum = bestTarget->s.number;
	}

	return found;
}

void VEH_TurretObeyPassengerControl(Vehicle_t* p_veh, gentity_t* parent, const int turretNum)
{
	const turretStats_t* turretStats = &p_veh->m_pVehicleInfo->turret[turretNum];
	const gentity_t* passenger = (gentity_t*)p_veh->m_ppPassengers[turretStats->passengerNum - 1];

	if (passenger && passenger->client && passenger->health > 0)
	{
		//a valid, living passenger client
		const vehWeaponInfo_t* vehWeapon = &g_vehWeaponInfo[turretStats->iWeapon];
		const int curMuzzle = p_veh->turretStatus[turretNum].nextMuzzle;
		vec3_t aimAngles;
		VectorCopy(passenger->client->ps.viewangles, aimAngles);

		VEH_TurretAim(p_veh, parent, NULL, turretStats, vehWeapon, turretNum, curMuzzle, aimAngles);
		if (passenger->client->pers.cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK))
		{
			//he's pressing an attack button, so fire!
			VEH_TurretCheckFire(p_veh, parent, turretStats, vehWeapon, turretNum, curMuzzle);
		}
	}
}

void VEH_TurretThink(Vehicle_t* p_veh, gentity_t* parent, const int turretNum)
//-----------------------------------------------------
{
	qboolean doAim = qfalse;
	const turretStats_t* turretStats = &p_veh->m_pVehicleInfo->turret[turretNum];
	const gentity_t* turretEnemy = NULL;

	if (!turretStats || !turretStats->iAmmoMax)
	{
		//not a valid turret
		return;
	}

	if (turretStats->passengerNum
		&& p_veh->m_iNumPassengers >= turretStats->passengerNum)
	{
		//the passenger that has control of this turret is on the ship
		VEH_TurretObeyPassengerControl(p_veh, parent, turretNum);
		return;
	}
	if (!turretStats->bAI) //try AI
	{
		//this turret does not think on its own.
		return;
	}

	//okay, so it has AI, but still don't think if there's no pilot!
	if (!p_veh->m_pPilot)
	{
		return;
	}

	const vehWeaponInfo_t* vehWeapon = &g_vehWeaponInfo[turretStats->iWeapon];
	const float rangeSq = turretStats->fAIRange * turretStats->fAIRange;
	const int curMuzzle = p_veh->turretStatus[turretNum].nextMuzzle;

	if (p_veh->turretStatus[turretNum].enemyEntNum < ENTITYNUM_WORLD)
	{
		turretEnemy = &g_entities[p_veh->turretStatus[turretNum].enemyEntNum];
		if (turretEnemy->health < 0
			|| !turretEnemy->inuse
			|| turretEnemy == (gentity_t*)p_veh->m_pPilot //enemy became my pilot///?
			|| turretEnemy == parent
			|| turretEnemy->r.ownerNum == parent->s.number // a passenger?
			|| turretEnemy->client && turretEnemy->client->sess.sessionTeam == TEAM_SPECTATOR
			|| turretEnemy->client && turretEnemy->client->tempSpectate >= level.time)
		{
			//don't keep going after spectators, pilot, self, dead people, etc.
			turretEnemy = NULL;
			p_veh->turretStatus[turretNum].enemyEntNum = ENTITYNUM_NONE;
		}
	}

	if (p_veh->turretStatus[turretNum].enemyHoldTime < level.time)
	{
		if (VEH_TurretFindEnemies(p_veh, parent, turretStats, turretNum, curMuzzle))
		{
			turretEnemy = &g_entities[p_veh->turretStatus[turretNum].enemyEntNum];
			doAim = qtrue;
		}
		else if (parent->enemy && parent->enemy->s.number < ENTITYNUM_WORLD)
		{
			if (g_gametype.integer < GT_TEAM || !OnSameTeam(parent->enemy, parent))
			{
				//either not in a team game or the enemy isn't on the same team
				turretEnemy = parent->enemy;
				doAim = qtrue;
			}
		}
		if (turretEnemy)
		{
			//found one
			if (turretEnemy->client)
			{
				//hold on to clients for a min of 3 seconds
				p_veh->turretStatus[turretNum].enemyHoldTime = level.time + 3000;
			}
			else
			{
				//hold less
				p_veh->turretStatus[turretNum].enemyHoldTime = level.time + 500;
			}
		}
	}
	if (turretEnemy != NULL)
	{
		if (turretEnemy->health > 0)
		{
			vec3_t enemyDir;
			// enemy is alive
			WP_CalcVehMuzzle(parent, curMuzzle);
			VectorSubtract(turretEnemy->r.currentOrigin, p_veh->m_vMuzzlePos[curMuzzle], enemyDir);
			const float enemyDist = VectorLengthSquared(enemyDir);

			if (enemyDist < rangeSq)
			{
				// was in valid radius
				if (trap->InPVS(p_veh->m_vMuzzlePos[curMuzzle], turretEnemy->r.currentOrigin))
				{
					// Every now and again, check to see if we can even trace to the enemy
					trace_t tr;
					vec3_t start, end;
					VectorCopy(p_veh->m_vMuzzlePos[curMuzzle], start);

					VectorCopy(turretEnemy->r.currentOrigin, end);
					trap->Trace(&tr, start, NULL, NULL, end, parent->s.number, MASK_SHOT, qfalse, 0, 0);

					if (tr.entityNum == turretEnemy->s.number
						|| !tr.allsolid && !tr.startsolid)
					{
						doAim = qtrue; // Can see our enemy
					}
				}
			}
		}
	}

	if (doAim)
	{
		vec3_t aimAngles;
		if (VEH_TurretAim(p_veh, parent, turretEnemy, turretStats, vehWeapon, turretNum, curMuzzle, aimAngles))
		{
			VEH_TurretCheckFire(p_veh, parent, /*turretEnemy,*/ turretStats, vehWeapon, turretNum, curMuzzle);
		}
	}
}