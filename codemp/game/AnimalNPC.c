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

#ifdef _GAME //including game headers on cgame is FORBIDDEN ^_^
#include "g_local.h"
#endif
#include "bg_public.h"
#include "bg_vehicles.h"

#ifdef _GAME //we only want a few of these functions for BG

extern vec3_t player_mins;
extern vec3_t player_maxs;

extern int BG_AnimLength(int index, animNumber_t anim);

extern void Vehicle_SetAnim(gentity_t* ent, int setAnimParts, int anim, int setAnimFlags, int i_blend);
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength, const qboolean breakSaberLock);
extern void G_VehicleTrace(trace_t* results, const vec3_t start, const vec3_t tMins, const vec3_t tMaxs,
	const vec3_t end, int pass_entity_num, int contentmask);

// Update death sequence.
static void DeathUpdate(Vehicle_t* p_veh)
{
	if (level.time >= p_veh->m_iDieTime)
	{
		// If the vehicle is not empty.
		if (p_veh->m_pVehicleInfo->Inhabited(p_veh))
		{
			p_veh->m_pVehicleInfo->EjectAll(p_veh);
		}
		else
		{
			// Waste this sucker.
		}
	}
}

// Like a think or move command, this updates various vehicle properties.
static qboolean Update(Vehicle_t* p_veh, const usercmd_t* pUcmd)
{
	return g_vehicleInfo[VEHICLE_BASE].Update(p_veh, pUcmd);
}
#endif //_GAME

//MP RULE - ALL PROCESSMOVECOMMANDS FUNCTIONS MUST BE BG-COMPATIBLE!!!
//If you really need to violate this rule for SP, then use ifdefs.
//By BG-compatible, I mean no use of game-specific data - ONLY use
//stuff available in the MP bgEntity (in SP, the bgEntity is #defined
//as a gentity, but the MP-compatible access restrictions are based
//on the bgEntity structure in the MP codebase) -rww
// ProcessMoveCommands the Vehicle.
static void ProcessMoveCommands(Vehicle_t* p_veh)
{
	/************************************************************************************/
	/*	BEGIN	Here is where we move the vehicle (forward or back or whatever). BEGIN	*/
	/************************************************************************************/

	//Client sets ucmds and such for speed alterations
	float speedInc, speedIdleDec, speedIdle, speedMin, speedMax;
	float fWalkSpeedMax;
	int curTime;
	const bgEntity_t* parent = p_veh->m_pParentEntity;
	playerState_t* parent_ps = parent->playerState;

#ifdef _GAME
	curTime = level.time;
#elif defined(_CGAME)
	//FIXME: pass in ucmd?  Not sure if this is reliable...
	curTime = pm->cmd.serverTime;
#endif

	speedIdleDec = p_veh->m_pVehicleInfo->decelIdle * p_veh->m_fTimeModifier;
	speedMax = p_veh->m_pVehicleInfo->speedMax;

	speedIdle = p_veh->m_pVehicleInfo->speedIdle;
	speedMin = p_veh->m_pVehicleInfo->speedMin;

	if (p_veh->m_pPilot &&
		p_veh->m_ucmd.buttons & BUTTON_ALT_ATTACK && p_veh->m_pVehicleInfo->turboSpeed)
	{
		if (curTime - p_veh->m_iTurboTime > p_veh->m_pVehicleInfo->turboRecharge)
		{
			p_veh->m_iTurboTime = curTime + p_veh->m_pVehicleInfo->turboDuration;
			parent_ps->speed = p_veh->m_pVehicleInfo->turboSpeed; // Instantly Jump To Turbo Speed
		}
	}

	if (curTime < p_veh->m_iTurboTime)
	{
		speedMax = p_veh->m_pVehicleInfo->turboSpeed;
	}
	else
	{
		speedMax = p_veh->m_pVehicleInfo->speedMax;
	}

	if (!parent_ps->m_iVehicleNum)
	{
		//drifts to a stop
		speedInc = speedIdle * p_veh->m_fTimeModifier;
		VectorClear(parent_ps->moveDir);
		//m_ucmd.forwardmove = 127;
		parent_ps->speed = 0;
	}
	else
	{
		speedInc = p_veh->m_pVehicleInfo->acceleration * p_veh->m_fTimeModifier;
	}

	if (parent_ps->speed || parent_ps->groundEntityNum == ENTITYNUM_NONE ||
		p_veh->m_ucmd.forwardmove || p_veh->m_ucmd.upmove > 0)
	{
		if (p_veh->m_ucmd.forwardmove > 0 && speedInc)
		{
			parent_ps->speed += speedInc;
		}
		else if (p_veh->m_ucmd.forwardmove < 0)
		{
			if (parent_ps->speed > speedIdle)
			{
				parent_ps->speed -= speedInc;
			}
			else if (parent_ps->speed > speedMin)
			{
				parent_ps->speed -= speedIdleDec;
			}
		}
		// No input, so coast to stop.
		else if (parent_ps->speed > 0.0f)
		{
			parent_ps->speed -= speedIdleDec;
			if (parent_ps->speed < 0.0f)
			{
				parent_ps->speed = 0.0f;
			}
		}
		else if (parent_ps->speed < 0.0f)
		{
			parent_ps->speed += speedIdleDec;
			if (parent_ps->speed > 0.0f)
			{
				parent_ps->speed = 0.0f;
			}
		}
	}
	else
	{
		if (p_veh->m_ucmd.forwardmove < 0)
		{
			p_veh->m_ucmd.forwardmove = 0;
		}
		if (p_veh->m_ucmd.upmove < 0)
		{
			p_veh->m_ucmd.upmove = 0;
		}

		//p_veh->m_ucmd.rightmove = 0;

		/*if ( !p_veh->m_pVehicleInfo->strafePerc
			|| (!g_speederControlScheme->value && !parent->s.number) )
		{//if in a strafe-capable vehicle, clear strafing unless using alternate control scheme
			p_veh->m_ucmd.rightmove = 0;
		}*/
	}

	fWalkSpeedMax = speedMax * 0.275f;
	if (curTime > p_veh->m_iTurboTime && p_veh->m_ucmd.buttons & BUTTON_WALKING && parent_ps->speed > fWalkSpeedMax)
	{
		parent_ps->speed = fWalkSpeedMax;
	}
	else if (parent_ps->speed > speedMax)
	{
		parent_ps->speed = speedMax;
	}
	else if (parent_ps->speed < speedMin)
	{
		parent_ps->speed = speedMin;
	}

	/********************************************************************************/
	/*	END Here is where we move the vehicle (forward or back or whatever). END	*/
	/********************************************************************************/
}

//MP RULE - ALL PROCESSORIENTCOMMANDS FUNCTIONS MUST BE BG-COMPATIBLE!!!
//If you really need to violate this rule for SP, then use ifdefs.
//By BG-compatible, I mean no use of game-specific data - ONLY use
//stuff available in the MP bgEntity (in SP, the bgEntity is #defined
//as a gentity, but the MP-compatible access restrictions are based
//on the bgEntity structure in the MP codebase) -rww
// ProcessOrientCommands the Vehicle.
static void ProcessOrientCommands(const Vehicle_t* p_veh)
{
	/********************************************************************************/
	/*	BEGIN	Here is where make sure the vehicle is properly oriented.	BEGIN	*/
	/********************************************************************************/
	const bgEntity_t* parent = p_veh->m_pParentEntity;

	const bgEntity_t* rider = NULL;
	if (parent->s.owner != ENTITYNUM_NONE)
	{
		rider = PM_BGEntForNum(parent->s.owner); //&g_entities[parent->r.ownerNum];
	}

	if (!rider)
	{
		rider = parent;
	}

	const playerState_t* parent_ps = parent->playerState;

	if (rider)
	{
		const playerState_t* rider_ps = rider->playerState;
		float angDif = AngleSubtract(p_veh->m_vOrientation[YAW], rider_ps->viewangles[YAW]);
		if (parent_ps && parent_ps->speed)
		{
			float s = parent_ps->speed;
			const float maxDif = p_veh->m_pVehicleInfo->turningSpeed * 4.0f; //magic number hackery
			if (s < 0.0f)
			{
				s = -s;
			}
			angDif *= s / p_veh->m_pVehicleInfo->speedMax;
			if (angDif > maxDif)
			{
				angDif = maxDif;
			}
			else if (angDif < -maxDif)
			{
				angDif = -maxDif;
			}
			p_veh->m_vOrientation[YAW] = AngleNormalize180(
				p_veh->m_vOrientation[YAW] - angDif * (p_veh->m_fTimeModifier * 0.2f));
		}
	}

	/********************************************************************************/
	/*	END	Here is where make sure the vehicle is properly oriented.	END			*/
	/********************************************************************************/
}

#ifdef _GAME //back to our game-only functions
static void AnimateVehicle(Vehicle_t* p_veh)
{
	animNumber_t Anim = BOTH_VT_IDLE;
	int iFlags = SETANIM_FLAG_NORMAL, i_blend = 300;
	gentity_t* pilot = (gentity_t*)p_veh->m_pPilot;
	gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;

	// We're dead (boarding is reused here so I don't have to make another variable :-).
	if (parent->health <= 0)
	{
		/*
		if ( p_veh->m_iBoarding != -999 )	// Animate the death just once!
		{
			p_veh->m_iBoarding = -999;
			iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;

			// FIXME! Why do you keep repeating over and over!!?!?!? Bastard!
			//Vehicle_SetAnim( parent, SETANIM_LEGS, BOTH_VT_DEATH1, iFlags, i_blend );
		}
		*/
		return;
	}

	// If they're bucking, play the animation and leave...
	if (parent->client->ps.legsAnim == BOTH_VT_BUCK)
	{
		// Done with animation? Erase the flag.
		if (parent->client->ps.legsTimer <= 0)
		{
			p_veh->m_ulFlags &= ~VEH_BUCKING;
		}
		else
		{
			return;
		}
	}
	else if (p_veh->m_ulFlags & VEH_BUCKING)
	{
		iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
		Anim = BOTH_VT_BUCK;
		i_blend = 500;
		Vehicle_SetAnim(parent, SETANIM_LEGS, BOTH_VT_BUCK, iFlags, i_blend);
		return;
	}

	// Boarding animation.
	if (p_veh->m_iBoarding != 0)
	{
		// We've just started boarding, set the amount of time it will take to finish boarding.
		if (p_veh->m_iBoarding < 0)
		{
			// Boarding from left...
			if (p_veh->m_iBoarding == -1)
			{
				Anim = BOTH_VT_MOUNT_L;
			}
			else if (p_veh->m_iBoarding == -2)
			{
				Anim = BOTH_VT_MOUNT_R;
			}
			else if (p_veh->m_iBoarding == -3)
			{
				Anim = BOTH_VT_MOUNT_B;
			}

			// Set the delay time (which happens to be the time it takes for the animation to complete).
			// NOTE: Here I made it so the delay is actually 70% (0.7f) of the animation time.
			const int iAnimLen = BG_AnimLength(parent->localAnimIndex, Anim) * 0.7f;
			p_veh->m_iBoarding = level.time + iAnimLen;

			// Set the animation, which won't be interrupted until it's completed.
			// TODO: But what if he's killed? Should the animation remain persistant???
			iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;

			Vehicle_SetAnim(parent, SETANIM_LEGS, Anim, iFlags, i_blend);
			if (pilot)
			{
				Vehicle_SetAnim(pilot, SETANIM_BOTH, Anim, iFlags, i_blend);
			}
			return;
		}
		// Otherwise we're done.
		if (p_veh->m_iBoarding <= level.time)
		{
			p_veh->m_iBoarding = 0;
		}
	}

	// Percentage of maximum speed relative to current speed.
	//float fSpeed = VectorLength( client->ps.velocity );
	const float fSpeedPercToMax = parent->client->ps.speed / p_veh->m_pVehicleInfo->speedMax;

	// Going in reverse...
	if (fSpeedPercToMax < -0.01f)
	{
		Anim = BOTH_VT_WALK_REV;
		i_blend = 600;
	}
	else
	{
		const qboolean Turbo = fSpeedPercToMax > 0.0f && level.time < p_veh->m_iTurboTime;
		const qboolean Walking = fSpeedPercToMax > 0.0f && (p_veh->m_ucmd.buttons & BUTTON_WALKING || fSpeedPercToMax <=
			0.275f);
		const qboolean Running = fSpeedPercToMax > 0.275f;

		// Remove Crashing Flag
		//----------------------
		p_veh->m_ulFlags &= ~VEH_CRASHING;

		if (Turbo)
		{
			// Kicked In Turbo
			i_blend = 50;
			iFlags = SETANIM_FLAG_OVERRIDE;
			Anim = BOTH_VT_TURBO;
		}
		else
		{
			// No Special Moves
			i_blend = 300;
			iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLDLESS;
			Anim = Walking ? BOTH_VT_WALK_FWD : Running ? BOTH_VT_RUN_FWD : BOTH_VT_IDLE1;
		}
	}
	Vehicle_SetAnim(parent, SETANIM_LEGS, Anim, iFlags, i_blend);
}

//rwwFIXMEFIXME: This is all going to have to be predicted I think, or it will feel awful
//and lagged
// This function makes sure that the rider's in this vehicle are properly animated.
static void AnimateRiders(Vehicle_t* p_veh)
{
	animNumber_t Anim = BOTH_VT_IDLE;
	int iFlags, i_blend;
	gentity_t* pilot = (gentity_t*)p_veh->m_pPilot;
	const gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;

	const playerState_t* pilotPS = p_veh->m_pPilot->playerState;

	// Boarding animation.
	if (p_veh->m_iBoarding != 0)
	{
		return;
	}

	// Percentage of maximum speed relative to current speed.

	// Going in reverse...
	{
		const float fSpeedPercToMax = parent->client->ps.speed / p_veh->m_pVehicleInfo->speedMax;
		const qboolean HasWeapon = pilotPS->weapon != WP_NONE && pilotPS->weapon != WP_MELEE;
		const qboolean Attacking = HasWeapon && !!(p_veh->m_ucmd.buttons & BUTTON_ATTACK);
		qboolean Right = p_veh->m_ucmd.rightmove > 0;
		qboolean Left = p_veh->m_ucmd.rightmove < 0;
		const qboolean Turbo = fSpeedPercToMax > 0.0f && level.time < p_veh->m_iTurboTime;
		const qboolean Walking = fSpeedPercToMax > 0.0f && (p_veh->m_ucmd.buttons & BUTTON_WALKING || fSpeedPercToMax <=
			0.275f);
		const qboolean Running = fSpeedPercToMax > 0.275f;
		EWeaponPose WeaponPose = WPOSE_NONE;

		// Remove Crashing Flag
		//----------------------
		p_veh->m_ulFlags &= ~VEH_CRASHING;

		// Put Away Saber When It Is Not Active
		//--------------------------------------

		// Don't Interrupt Attack Anims
		//------------------------------
		if (pilotPS->weaponTime > 0)
		{
			return;
		}

		// Compute The Weapon Pose
		//--------------------------
		if (pilotPS->weapon == WP_BLASTER)
		{
			WeaponPose = WPOSE_BLASTER;
		}
		else if (pilotPS->weapon == WP_SABER)
		{
			if (p_veh->m_ulFlags & VEH_SABERINLEFTHAND && pilotPS->torsoAnim == BOTH_VT_ATL_TO_R_S)
			{
				p_veh->m_ulFlags &= ~VEH_SABERINLEFTHAND;
			}
			if (!(p_veh->m_ulFlags & VEH_SABERINLEFTHAND) && pilotPS->torsoAnim == BOTH_VT_ATR_TO_L_S)
			{
				p_veh->m_ulFlags |= VEH_SABERINLEFTHAND;
			}
			WeaponPose = p_veh->m_ulFlags & VEH_SABERINLEFTHAND ? WPOSE_SABERLEFT : WPOSE_SABERRIGHT;
		}

		if (Attacking && WeaponPose)
		{
			// Attack!
			i_blend = 100;
			iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART;

			if (Turbo)
			{
				Right = qtrue;
				Left = qfalse;
			}

			// Auto Aiming
			//===============================================
			if (!Left && !Right) // Allow player strafe keys to override
			{
				if (pilotPS->weapon == WP_SABER && !Left && !Right)
				{
					Left = WeaponPose == WPOSE_SABERLEFT;
					Right = !Left;
				}
			}

			if (Left)
			{
				// Attack Left
				switch (WeaponPose)
				{
				case WPOSE_BLASTER: Anim = BOTH_VT_ATL_G;
					break;
				case WPOSE_SABERLEFT: Anim = BOTH_VT_ATL_S;
					break;
				case WPOSE_SABERRIGHT: Anim = BOTH_VT_ATR_TO_L_S;
					break;
				default: assert(0);
				}
			}
			else if (Right)
			{
				// Attack Right
				switch (WeaponPose)
				{
				case WPOSE_BLASTER: Anim = BOTH_VT_ATR_G;
					break;
				case WPOSE_SABERLEFT: Anim = BOTH_VT_ATL_TO_R_S;
					break;
				case WPOSE_SABERRIGHT: Anim = BOTH_VT_ATR_S;
					break;
				default: assert(0);
				}
			}
			else
			{
				// Attack Ahead
				switch (WeaponPose)
				{
				case WPOSE_BLASTER: Anim = BOTH_VT_ATF_G;
					break;
				default: assert(0);
				}
			}
		}
		else if (Turbo)
		{
			// Kicked In Turbo
			i_blend = 50;
			iFlags = SETANIM_FLAG_OVERRIDE;
			Anim = BOTH_VT_TURBO;
		}
		else
		{
			// No Special Moves
			i_blend = 300;
			iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLDLESS;

			if (WeaponPose == WPOSE_NONE)
			{
				if (Walking)
				{
					Anim = BOTH_VT_WALK_FWD;
				}
				else if (Running)
				{
					Anim = BOTH_VT_RUN_FWD;
				}
				else
				{
					Anim = BOTH_VT_IDLE1; //(Q_irand(0,1)==0)?(BOTH_VT_IDLE):(BOTH_VT_IDLE1);
				}
			}
			else
			{
				switch (WeaponPose)
				{
				case WPOSE_BLASTER: Anim = BOTH_VT_IDLE_G;
					break;
				case WPOSE_SABERLEFT: Anim = BOTH_VT_IDLE_SL;
					break;
				case WPOSE_SABERRIGHT: Anim = BOTH_VT_IDLE_SR;
					break;
				default: assert(0);
				}
			}
		} // No Special Moves
	}

	Vehicle_SetAnim(pilot, SETANIM_BOTH, Anim, iFlags, i_blend);
}
#endif //_GAME

#ifdef _CGAME
void AttachRidersGeneric(Vehicle_t* p_veh);
#endif

//on the client this function will only set up the process command funcs
void G_SetAnimalVehicleFunctions(vehicleInfo_t* pVehInfo)
{
#ifdef _GAME
	pVehInfo->AnimateVehicle = AnimateVehicle;
	pVehInfo->AnimateRiders = AnimateRiders;
	//	pVehInfo->ValidateBoard				=		ValidateBoard;
	//	pVehInfo->SetParent					=		SetParent;
	//	pVehInfo->SetPilot					=		SetPilot;
	//	pVehInfo->AddPassenger				=		AddPassenger;
	//	pVehInfo->Animate					=		Animate;
	//	pVehInfo->Board						=		Board;
	//	pVehInfo->Eject						=		Eject;
	//	pVehInfo->EjectAll					=		EjectAll;
	//	pVehInfo->StartDeathDelay			=		StartDeathDelay;
	pVehInfo->DeathUpdate = DeathUpdate;
	//	pVehInfo->RegisterAssets			=		RegisterAssets;
	//	pVehInfo->Initialize				=		Initialize;
	pVehInfo->Update = Update;
	//	pVehInfo->UpdateRider				=		UpdateRider;
#endif //_GAME
	pVehInfo->ProcessMoveCommands = ProcessMoveCommands;
	pVehInfo->ProcessOrientCommands = ProcessOrientCommands;

#ifdef _CGAME //cgame prediction attachment func
	pVehInfo->AttachRiders = AttachRidersGeneric;
#endif
	//	pVehInfo->AttachRiders				=		AttachRiders;
	//	pVehInfo->Ghost						=		Ghost;
	//	pVehInfo->UnGhost					=		UnGhost;
	//	pVehInfo->Inhabited					=		Inhabited;
}

#ifdef _GAME
extern void G_AllocateVehicleObject(Vehicle_t** p_veh);
#endif

// Create/Allocate a new Animal Vehicle (initializing it as well).
//this is a BG function too in MP so don't un-bg-compatibilify it -rww
void G_CreateAnimalNPC(Vehicle_t** p_veh, const char* strAnimalType)
{
	// Allocate the Vehicle.
#ifdef _GAME
	//these will remain on entities on the client once allocated because the pointer is
	//never stomped. on the server, however, when an ent is freed, the entity struct is
	//memset to 0, so this memory would be lost..
	G_AllocateVehicleObject(p_veh);
#else
	if (!*p_veh)
	{
		//only allocate a new one if we really have to
		*p_veh = (Vehicle_t*)BG_Alloc(sizeof(Vehicle_t));
	}
#endif
	memset(*p_veh, 0, sizeof(Vehicle_t));
	(*p_veh)->m_pVehicleInfo = &g_vehicleInfo[BG_VehicleGetIndex(strAnimalType)];
}