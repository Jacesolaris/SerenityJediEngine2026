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

#include "b_local.h"
#include "g_navigator.h"

#if !defined(RAVL_VEC_INC)
#include "../Ravl/CVec.h"
#endif
#include "../Ratl/vector_vs.h"

constexpr auto MAX_PACKS = 10;

constexpr auto LEAVE_PACK_DISTANCE = 1000;
constexpr auto JOIN_PACK_DISTANCE = 800;
constexpr auto WANDER_RANGE = 1000;
constexpr auto FRIGHTEN_DISTANCE = 300;

extern qboolean G_PlayerSpawned();

ratl::vector_vs<gentity_t*, MAX_PACKS> mPacks;

////////////////////////////////////////////////////////////////////////////////////////
// Update The Packs, Delete Dead Leaders, Join / Split Packs, Find MY Leader
////////////////////////////////////////////////////////////////////////////////////////
static gentity_t* NPC_AnimalUpdateLeader()
{
	// Find The Closest Pack Leader, Not Counting Myself
	//---------------------------------------------------
	gentity_t* closestLeader = nullptr;
	float closestDist = 0;
	int myLeaderNum = 0;

	for (int i = 0; i < mPacks.size(); i++)
	{
		// Dump Dead Leaders
		//-------------------
		if (mPacks[i] == nullptr || mPacks[i]->health <= 0)
		{
			if (mPacks[i] == NPC->client->leader)
			{
				NPC->client->leader = nullptr;
			}

			mPacks.erase_swap(i);

			if (i >= mPacks.size())
			{
				closestLeader = nullptr;
				break;
			}
		}

		// Don't Count Self
		//------------------
		if (mPacks[i] == NPC)
		{
			myLeaderNum = i;
			continue;
		}

		const float Dist = Distance(mPacks[i]->currentOrigin, NPC->currentOrigin);
		if (!closestLeader || Dist < closestDist)
		{
			closestDist = Dist;
			closestLeader = mPacks[i];
		}
	}

	// In Joining Distance?
	//----------------------
	if (closestLeader && closestDist < JOIN_PACK_DISTANCE)
	{
		// Am I Already A Leader?
		//------------------------
		if (NPC->client->leader == NPC)
		{
			mPacks.erase_swap(myLeaderNum); // Erase Myself From The Leader List
		}

		// Join The Pack!
		//----------------
		NPC->client->leader = closestLeader;
	}

	// Do I Have A Leader?
	//---------------------
	if (NPC->client->leader)
	{
		// AM I A Leader?
		//----------------
		if (NPC->client->leader != NPC)
		{
			// If Our Leader Is Dead, Clear Him Out

			if (NPC->client->leader->health <= 0 || NPC->client->leader->inuse == 0)
			{
				NPC->client->leader = nullptr;
			}

			// If My Leader Isn't His Own Leader, Then, Use His Leader
			//---------------------------------------------------------
			else if (NPC->client->leader->client->leader != NPC->client->leader)
			{
				// Eh.  Can this get more confusing?
				NPC->client->leader = NPC->client->leader->client->leader;
			}

			// If Our Leader Is Too Far Away, Clear Him Out
			//------------------------------------------------------
			else if (Distance(NPC->client->leader->currentOrigin, NPC->currentOrigin) > LEAVE_PACK_DISTANCE)
			{
				NPC->client->leader = nullptr;
			}
		}
	}

	// If We Couldn't Find A Leader, Then Become One
	//-----------------------------------------------
	else if (!mPacks.full())
	{
		NPC->client->leader = NPC;
		mPacks.push_back(NPC);
	}
	return NPC->client->leader;
}

/*
-------------------------
NPC_BSAnimal_Default
-------------------------
*/
void NPC_BSAnimal_Default()
{
	if (!NPC || !NPC->client)
	{
		return;
	}

	// Update Some Positions
	//-----------------------
	const CVec3 CurrentLocation(NPC->currentOrigin);

	// Update The Leader
	//-------------------
	gentity_t* leader = NPC_AnimalUpdateLeader();

	// Select Closest Threat Location
	//--------------------------------
	CVec3 threat_location{};
	const qboolean PlayerSpawned = G_PlayerSpawned();
	if (PlayerSpawned)
	{
		//player is actually in the level now
		threat_location = player->currentOrigin;
	}
	const int alert_event = NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_MINOR, qfalse);
	if (alert_event >= 0)
	{
		const alertEvent_t* event = &level.alertEvents[alert_event];
		if (event->owner != NPC && Distance(event->position, CurrentLocation.v) < event->radius)
		{
			threat_location = event->position;
		}
	}

	const bool EvadeThreat = level.time < NPCInfo->investigateSoundDebounceTime;
	const bool CharmedDocile = level.time < NPCInfo->confusionTime || level.time < NPCInfo->insanityTime;
	const bool CharmedApproach = level.time < NPCInfo->charmedTime || level.time < NPCInfo->darkCharmedTime;

	STEER::Activate(NPC);
	{
		// Charmed Approach - Walk TOWARD The Threat Location
		//----------------------------------------------------
		if (CharmedApproach)
		{
			NAV::GoTo(NPC, NPCInfo->investigateGoal);
		}

		// Charmed Docile - Stay Put
		//---------------------------
		else if (CharmedDocile)
		{
			NAV::ClearPath(NPC);
			STEER::Stop(NPC);
		}

		// Run Away From This Threat
		//---------------------------
		else if (EvadeThreat)
		{
			NAV::ClearPath(NPC);
			STEER::Flee(NPC, NPCInfo->investigateGoal);
		}

		// Normal Behavior
		//-----------------
		else
		{
			// Follow Our Pack Leader!
			//-------------------------
			if (leader && leader != NPC)
			{
				constexpr float followDist = 100.0f;
				const float curDist = Distance(NPC->currentOrigin, leader->followPos);

				// Update The Leader's Follow Position
				//-------------------------------------
				STEER::FollowLeader(NPC, leader, followDist);

				const bool inSeekRange = curDist < followDist * 10.0f;
				const bool onNbrPoints = NAV::OnNeighboringPoints(NAV::GetNearestNode(NPC), leader->followPosWaypoint);
				const bool leaderStop = level.time - leader->lastMoveTime > 500;

				// If Close Enough, Dump Any Existing Path
				//-----------------------------------------
				if (inSeekRange || onNbrPoints)
				{
					NAV::ClearPath(NPC);

					// If The Leader Isn't Moving, Stop
					//----------------------------------
					if (leaderStop)
					{
						STEER::Stop(NPC);
					}

					// Otherwise, Try To Get To The Follow Position
					//----------------------------------------------
					else
					{
						STEER::Seek(NPC, leader->followPos, fabsf(followDist) / 2.0f/*slowing distance*/, 1.0f/*wight*/,
							leader->resultspeed);
					}
				}

				// Otherwise, Get A Path To The Follow Position
				//----------------------------------------------
				else
				{
					NAV::GoTo(NPC, leader->followPosWaypoint);
				}
				STEER::Separation(NPC, 4.0f);
				STEER::AvoidCollisions(NPC, leader);
			}

			// Leader AI - Basically Wander
			//------------------------------
			else
			{
				// Are We Doing A Path?
				//----------------------
				bool HasPath = NAV::HasPath(NPC);
				if (HasPath)
				{
					HasPath = NAV::UpdatePath(NPC);
					if (HasPath)
					{
						STEER::Path(NPC); // Follow The Path
						STEER::AvoidCollisions(NPC);
					}
				}

				if (!HasPath)
				{
					// If Debounce Time Has Expired, Choose A New Sub State
					//------------------------------------------------------
					if (NPCInfo->investigateDebounceTime < level.time)
					{
						// Clear Out Flags From The Previous Substate
						//--------------------------------------------
						NPCInfo->aiFlags &= ~NPCAI_OFF_PATH;
						NPCInfo->aiFlags &= ~NPCAI_WALKING;

						// Pick Another Spot
						//-------------------
						const int NEXTSUBSTATE = Q_irand(0, 10);

						const bool RandomPathNode = NEXTSUBSTATE < 8; //(NEXTSUBSTATE<9);
						const bool PathlessWander = NEXTSUBSTATE < 9; //false;

						// Random Path Node
						//------------------
						if (RandomPathNode)
						{
							// Sometimes, Walk
							//-----------------
							if (Q_irand(0, 1) == 0)
							{
								NPCInfo->aiFlags |= NPCAI_WALKING;
							}

							NPCInfo->investigateDebounceTime = level.time + Q_irand(3000, 10000);
							NAV::FindPath(NPC, NAV::ChooseRandomNeighbor(NAV::GetNearestNode(NPC)));
							//, mHome.v, WANDER_RANGE));
						}

						// Pathless Wandering
						//--------------------
						else if (PathlessWander)
						{
							// Sometimes, Walk
							//-----------------
							if (Q_irand(0, 1) == 0)
							{
								NPCInfo->aiFlags |= NPCAI_WALKING;
							}

							NPCInfo->investigateDebounceTime = level.time + Q_irand(3000, 10000);
							NPCInfo->aiFlags |= NPCAI_OFF_PATH;
						}

						// Just Stand Here
						//-----------------
						else
						{
							NPCInfo->investigateDebounceTime = level.time + Q_irand(2000, 6000);
							NPC_SetAnim(NPC, SETANIM_BOTH,
								Q_irand(0, 1) == 0 ? BOTH_GUARD_LOOKAROUND1 : BOTH_GUARD_IDLE1,
								SETANIM_FLAG_NORMAL);
						}
					}

					// Ok, So We Don't Have A Path, And Debounce Time Is Still Active, So We Are Either Wandering Or Looking Around
					//--------------------------------------------------------------------------------------------------------------
					else
					{
						//	if (DistFromHome>(WANDER_RANGE))
						//	{
						//		STEER::Seek(NPC, mHome);
						//	}
						//	else
						{
							if (NPCInfo->aiFlags & NPCAI_OFF_PATH)
							{
								STEER::Wander(NPC);
								STEER::AvoidCollisions(NPC);
							}
							else
							{
								STEER::Stop(NPC);
							}
						}
					}
				}
			}
		}
	}
	STEER::DeActivate(NPC, &ucmd);

	NPC_UpdateAngles(qtrue, qtrue);
}