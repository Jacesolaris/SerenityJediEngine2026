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

#include "../server/exe_headers.h"

#include "../client/vmachine.h"
#include "server.h"

/*
=============================================================================

Delta encode a client frame onto the network channel

A normal server packet will look like:

4	sequence number (high bit set if an oversize fragment)
<optional reliable commands>
1	svc_snapshot
4	last client reliable command
4	serverTime
1	lastframe for delta compression
1	snapFlags
1	areaBytes
<areabytes>
<playerstate>
<packetentities>

=============================================================================
*/

/*
=============
SV_EmitPacketEntities

Writes a delta update of an entityState_t list to the message.
=============
*/
static void SV_EmitPacketEntities(const clientSnapshot_t* from, const clientSnapshot_t* to, msg_t* msg)
{
	int oldnum, newnum;
	int from_num_entities;

	// generate the delta update
	if (!from)
	{
		from_num_entities = 0;
	}
	else
	{
		from_num_entities = from->num_entities;
	}

	const entityState_t* newent = nullptr;
	const entityState_t* oldent = nullptr;
	int newindex = 0;
	int oldindex = 0;
	const int num2_send = to->num_entities >= svs.numSnapshotEntities ? svs.numSnapshotEntities : to->num_entities;

	while (newindex < num2_send || oldindex < from_num_entities)
	{
		if (newindex >= num2_send)
		{
			newnum = 9999;
		}
		else
		{
			newent = &svs.snapshotEntities[(to->first_entity + newindex) % svs.numSnapshotEntities];
			newnum = newent->number;
		}

		if (oldindex >= from_num_entities)
		{
			oldnum = 9999;
		}
		else
		{
			oldent = &svs.snapshotEntities[(from->first_entity + oldindex) % svs.numSnapshotEntities];
			oldnum = oldent->number;
		}

		if (newnum == oldnum)
		{
			// delta update from old position
			// because the force parm is qfalse, this will not result
			// in any bytes being emited if the entity has not changed at all
			MSG_WriteEntity(msg, newent, 0);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{
			// this is a new entity, send it from the baseline
			MSG_WriteEntity(msg, newent, 0);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{
			// the old entity isn't present in the new message
			if (oldent)
			{
				MSG_WriteEntity(msg, nullptr, oldent->number);
			}
			oldindex++;
		}
	}

	MSG_WriteBits(msg, MAX_GENTITIES - 1, GENTITYNUM_BITS); // end of packetentities
}

/*
==================
SV_WriteSnapshotToClient
==================
*/
static void SV_WriteSnapshotToClient(client_t* client, msg_t* msg)
{
	clientSnapshot_t* oldframe;
	int lastframe;

	// this is the snapshot we are creating
	clientSnapshot_t* frame = &client->frames[client->netchan.outgoingSequence & PACKET_MASK];

	// try to use a previous frame as the source for delta compressing the snapshot
	if (client->deltaMessage <= 0 || client->state != CS_ACTIVE)
	{
		// client is asking for a retransmit
		oldframe = nullptr;
		lastframe = 0;
	}
	else if (client->netchan.outgoingSequence - client->deltaMessage
		>= PACKET_BACKUP - 3)
	{
		// client hasn't gotten a good message through in a long time
		Com_DPrintf("%s: Delta request from out of date packet.\n", client->name);
		oldframe = nullptr;
		lastframe = 0;
	}
	else
	{
		// we have a valid snapshot to delta from
		oldframe = &client->frames[client->deltaMessage & PACKET_MASK];
		lastframe = client->netchan.outgoingSequence - client->deltaMessage;

		// the snapshot's entities may still have rolled off the buffer, though
		if (oldframe->first_entity <= svs.nextSnapshotEntities - svs.numSnapshotEntities)
		{
			Com_DPrintf("%s: Delta request from out of date entities.\n", client->name);
			oldframe = nullptr;
			lastframe = 0;
		}
	}

	MSG_WriteByte(msg, svc_snapshot);

	// let the client know which reliable clientCommands we have received
	MSG_WriteLong(msg, client->lastClientCommand);

	// send over the current server time so the client can drift
	// its view of time to try to match
	MSG_WriteLong(msg, sv.time);

	// we must write a message number, because recorded demos won't have
	// the same network message sequences
	MSG_WriteLong(msg, client->netchan.outgoingSequence);
	MSG_WriteByte(msg, lastframe); // what we are delta'ing from
	MSG_WriteLong(msg, client->cmdNum); // we have executed up to here

	const int snap_flags = client->droppedCommands << 1;
	client->droppedCommands = qfalse;

	MSG_WriteByte(msg, snap_flags);

	// send over the areabits
	MSG_WriteByte(msg, frame->areabytes);
	MSG_WriteData(msg, frame->areabits, frame->areabytes);

	// delta encode the playerstate
	if (oldframe)
	{
		MSG_WriteDeltaPlayerstate(msg, &oldframe->ps, &frame->ps);
	}
	else
	{
		MSG_WriteDeltaPlayerstate(msg, nullptr, &frame->ps);
	}

	// delta encode the entities
	SV_EmitPacketEntities(oldframe, frame, msg);
}

/*
==================
SV_UpdateServerCommandsToClient

(re)send all server commands the client hasn't acknowledged yet
==================
*/
static void SV_UpdateServerCommandsToClient(const client_t* client, msg_t* msg)
{
	// write any unacknowledged serverCommands
	for (int i = client->reliableAcknowledge + 1; i <= client->reliableSequence; i++)
	{
		MSG_WriteByte(msg, svc_serverCommand);
		MSG_WriteLong(msg, i);
		MSG_WriteString(msg, client->reliableCommands[i & MAX_RELIABLE_COMMANDS - 1]);
	}
}

/*
=============================================================================

Build a client snapshot structure

=============================================================================
*/

constexpr auto MAX_SNAPSHOT_ENTITIES = 1024;
using snapshotentity_numbers_t = struct
{
	int numSnapshotEntities;
	int snapshotEntities[MAX_SNAPSHOT_ENTITIES];
};

/*
=======================
SV_Qsortentity_numbers
=======================
*/
static int SV_Qsortentity_numbers(const void* a, const void* b)
{
	const int* ea = (int*)a;
	const int* eb = (int*)b;

	if (*ea == *eb)
	{
		Com_Error(ERR_DROP, "SV_QsortEntityStates: duplicated entity");
	}

	if (*ea < *eb)
	{
		return -1;
	}

	return 1;
}

/*
===============
SV_AddEntToSnapshot
===============
*/
static void SV_AddEntToSnapshot(svEntity_t* sv_ent, const gentity_t* g_ent, snapshotentity_numbers_t* e_nums)
{
	// if we have already added this entity to this snapshot, don't add again
	if (sv_ent->snapshotCounter == sv.snapshotCounter)
	{
		return;
	}
	sv_ent->snapshotCounter = sv.snapshotCounter;

	// if we are full, silently discard entities
	if (e_nums->numSnapshotEntities == MAX_SNAPSHOT_ENTITIES)
	{
		return;
	}

	if (sv.snapshotCounter & 1 && e_nums->numSnapshotEntities == svs.numSnapshotEntities - 1)
	{
		//we're full, and about to wrap around and stomp ents, so half the time send the first set without stomping.
		return;
	}

	e_nums->snapshotEntities[e_nums->numSnapshotEntities] = g_ent->s.number;
	e_nums->numSnapshotEntities++;
}

//rww - bg_public.h won't cooperate in here
#define EF_PERMANENT			0x00080000

float sv_sightRangeForLevel[6] =
{
	0, //FORCE_LEVEL_0
	1024.f, //FORCE_LEVEL_1
	2048.0f, //FORCE_LEVEL_2
	4096.0f, //FORCE_LEVEL_3
	4096.0f, //FORCE_LEVEL_4
	4096.0f //FORCE_LEVEL_5
};

qboolean SV_PlayerCanSeeEnt(const gentity_t* ent, const int sightLevel)
{
	//return true if this ent is in view
	//NOTE: this is similar to the func CG_PlayerCanSeeCent in cg_players
	vec3_t view_org;
	if (!ent)
	{
		return qfalse;
	}
	if (VM_Call(CG_CAMERA_POS, view_org))
	{
		vec3_t view_angles;
		if (VM_Call(CG_CAMERA_ANG, view_angles))
		{
			vec3_t dir2_ent;
			vec3_t view_fwd;
			float dot = 0.25f; //1.0f;
			const float range = sv_sightRangeForLevel[sightLevel];

			VectorSubtract(ent->currentOrigin, view_org, dir2_ent);
			const float ent_dist = VectorNormalize(dir2_ent);

			if (ent->s.eFlags & EF_FORCE_VISIBLE)
			{
				//no dist check on them?
			}
			else
			{
				if (ent_dist < 128.0f)
				{
					//can always see them if they're really close
					return qtrue;
				}

				if (ent_dist > range)
				{
					//too far away to see them
					return qfalse;
				}
			}

			dot += (0.99f - dot) * ent_dist / range; //the farther away they are, the more in front they have to be

			AngleVectors(view_angles, view_fwd, nullptr, nullptr);
			if (DotProduct(view_fwd, dir2_ent) < dot)
			{
				return qfalse;
			}
			return qtrue;
		}
	}
	return qfalse;
}

/*
===============
SV_AddEntitiesVisibleFromPoint
===============
*/
static void SV_AddEntitiesVisibleFromPoint(vec3_t origin, clientSnapshot_t* frame,
	snapshotentity_numbers_t* e_nums, const qboolean portal)
{
	int i;
	gentity_t* ent;
	svEntity_t* sv_ent;
	int l;
	int clientarea, clientcluster;
	int leafnum;
	const byte* clientpvs;
	const byte* bitvector;
#ifndef JK2_MODE
	qboolean sightOn = qfalse;
#endif

	// during an error shutdown message we may need to transmit
	// the shutdown message after the server has shutdown, so
	// specfically check for it
	if (!sv.state)
	{
		return;
	}

	leafnum = CM_PointLeafnum(origin);
	clientarea = CM_LeafArea(leafnum);
	clientcluster = CM_LeafCluster(leafnum);

	// calculate the visible areas
	frame->areabytes = CM_WriteAreaBits(frame->areabits, clientarea);

	clientpvs = CM_ClusterPVS(clientcluster);

#ifndef JK2_MODE
	if (!portal)
	{
		//not if this if through a portal...???  James said to do this...
		if (frame->ps.forcePowersActive & 1 << FP_SEE)
		{
			sightOn = qtrue;
		}
	}
#endif // !JK2_MODE

	for (int e = 0; e < ge->num_entities; e++)
	{
		ent = SV_Gentity_num(e);

		if (!ent->inuse)
		{
			continue;
		}

		if (ent->s.eFlags & EF_PERMANENT)
		{
			// he's permanent, so don't send him down!
			continue;
		}

		if (ent->s.number != e)
		{
			Com_DPrintf("FIXING ENT->S.NUMBER!!!\n");
			ent->s.number = e;
		}

		// never send entities that aren't linked in
		if (!ent->linked)
		{
			continue;
		}

		// entities can be flagged to explicitly not be sent to the client
		if (ent->svFlags & SVF_NOCLIENT)
		{
			continue;
		}

		sv_ent = SV_SvEntityForGentity(ent);

		// don't double add an entity through portals
		if (sv_ent->snapshotCounter == sv.snapshotCounter)
		{
			continue;
		}

		// broadcast entities are always sent, and so is the main player so we don't see noclip weirdness
		if (ent->svFlags & SVF_BROADCAST || !e)
		{
			SV_AddEntToSnapshot(sv_ent, ent, e_nums);
			continue;
		}

#ifndef JK2_MODE
		if (ent->s.isPortalEnt)
		{
			//rww - portal entities are always sent as well
			SV_AddEntToSnapshot(sv_ent, ent, e_nums);
			continue;
		}
#endif // !JK2_MODE

#ifndef JK2_MODE
		if (sightOn)
		{
			//force sight is on, sees through portals, so draw them always if in radius
			if (SV_PlayerCanSeeEnt(ent, frame->ps.forcePowerLevel[FP_SEE]))
			{
				//entity is visible
				SV_AddEntToSnapshot(sv_ent, ent, e_nums);
				continue;
			}
		}
#endif // !JK2_MODE

		// ignore if not touching a PV leaf
		// check area
		if (!CM_AreasConnected(clientarea, sv_ent->areanum))
		{
			// doors can legally straddle two areas, so
			// we may need to check another one
			if (!CM_AreasConnected(clientarea, sv_ent->areanum2))
			{
				continue; // blocked by a door
			}
		}

		bitvector = clientpvs;

		// check individual leafs
		if (!sv_ent->numClusters)
		{
			continue;
		}
		l = 0;

		for (i = 0; i < sv_ent->numClusters; i++)
		{
			l = sv_ent->clusternums[i];
			if (bitvector[l >> 3] & 1 << (l & 7))
			{
				break;
			}
		}

		// if we haven't found it to be visible,
		// check overflow clusters that coudln't be stored
		if (i == sv_ent->numClusters)
		{
			if (sv_ent->lastCluster)
			{
				for (; l <= sv_ent->lastCluster; l++)
				{
					if (bitvector[l >> 3] & 1 << (l & 7))
					{
						break;
					}
				}
				if (l == sv_ent->lastCluster)
				{
					continue; // not visible
				}
			}
			else
			{
				continue;
			}
		}

		// add it
		SV_AddEntToSnapshot(sv_ent, ent, e_nums);

		// if its a portal entity, add everything visible from its camera position
		if (ent->svFlags & SVF_PORTAL)
		{
			SV_AddEntitiesVisibleFromPoint(ent->s.origin2, frame, e_nums, qtrue);
		}
	}
}

/*
=============
SV_BuildClientSnapshot

Decides which entities are going to be visible to the client, and
copies off the playerstate and areabits.

This properly handles multiple recursive portals, but the render
currently doesn't.

For viewing through other player's eyes, clent can be something other than client->gentity
=============
*/
static clientSnapshot_t* SV_BuildClientSnapshot(client_t* client)
{
	vec3_t org;
	snapshotentity_numbers_t entity_numbers;
	int i;

	// bump the counter used to prevent double adding
	sv.snapshotCounter++;

	// this is the frame we are creating
	clientSnapshot_t* frame = &client->frames[client->netchan.outgoingSequence & PACKET_MASK];

	// clear everything in this snapshot
	entity_numbers.numSnapshotEntities = 0;
	memset(frame->areabits, 0, sizeof frame->areabits);

	const gentity_t* clent = client->gentity;
	if (!clent)
	{
		return frame;
	}

	// grab the current playerState_t
	frame->ps = *clent->client;

	// this stops the main client entity playerstate from being sent across, which has the effect of breaking
	// looping sounds for the main client. So I took it out.
	/*	{
			int							clientNum;
			svEntity_t					*svEnt;
			clientNum = frame->ps.clientNum;
			if ( clientNum < 0 || clientNum >= MAX_GENTITIES ) {
				Com_Error( ERR_DROP, "SV_SvEntityForGentity: bad gEnt" );
			}
			svEnt = &sv.svEntities[ clientNum ];
			// never send client's own entity, because it can
			// be regenerated from the playerstate
			svEnt->snapshotCounter = sv.snapshotCounter;
		}
	*/
	// find the client's viewpoint

	//if in camera mode use camera position instead
	if (VM_Call(CG_CAMERA_POS, org))
	{
		//org[2] += clent->client->viewheight;
	}
	else
	{
		VectorCopy(clent->client->origin, org);
		org[2] += clent->client->viewheight;

		//============
		// need to account for lean, or areaportal doors don't draw properly... -slc
		if (frame->ps.leanofs != 0)
		{
			vec3_t right;
			//add leaning offset
			vec3_t v3_view_angles;
			VectorCopy(clent->client->viewangles, v3_view_angles);
			v3_view_angles[2] += static_cast<float>(frame->ps.leanofs) / 2;
			AngleVectors(v3_view_angles, nullptr, right, nullptr);
			VectorMA(org, static_cast<float>(frame->ps.leanofs), right, org);
		}
		//============
	}
	VectorCopy(org, frame->ps.serverViewOrg);
	VectorCopy(org, clent->client->serverViewOrg);

	// add all the entities directly visible to the eye, which
	// may include portal entities that merge other viewpoints
	SV_AddEntitiesVisibleFromPoint(org, frame, &entity_numbers, qfalse);

	// if there were portals visible, there may be out of order entities
	// in the list which will need to be resorted for the delta compression
	// to work correctly.  This also catches the error condition
	// of an entity being included twice.
	qsort(entity_numbers.snapshotEntities, entity_numbers.numSnapshotEntities,
		sizeof entity_numbers.snapshotEntities[0], SV_Qsortentity_numbers);

	// now that all viewpoint's areabits have been OR'd together, invert
	// all of them to make it a mask vector, which is what the renderer wants
	for (i = 0; i < MAX_MAP_AREA_BYTES / 4; i++)
	{
		reinterpret_cast<int*>(frame->areabits)[i] = reinterpret_cast<int*>(frame->areabits)[i] ^ -1;
	}

	// copy the entity states out
	frame->num_entities = 0;
	frame->first_entity = svs.nextSnapshotEntities;
	for (i = 0; i < entity_numbers.numSnapshotEntities; i++)
	{
		const gentity_t* ent = SV_Gentity_num(entity_numbers.snapshotEntities[i]);
		entityState_t* state = &svs.snapshotEntities[svs.nextSnapshotEntities % svs.numSnapshotEntities];
		*state = ent->s;
		svs.nextSnapshotEntities++;
		frame->num_entities++;
	}

	return frame;
}

/*
=======================
SV_SendMessageToClient

Called by SV_SendClientSnapshot and SV_SendClientGameState
=======================
*/
constexpr auto HEADER_RATE_BYTES = 48; // include our header, IP header, and some overhead;
void SV_SendMessageToClient(const msg_t* msg, client_t* client)
{
	// record information about the message
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSize = msg->cursize;
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = sv.time;

	// send the datagram
	Netchan_Transmit(&client->netchan, msg->cursize, msg->data);
}

/*
=======================
SV_SendClientEmptyMessage

This is just an empty message so that we can tell if
the client dropped the gamestate that went out before
=======================
*/
void SV_SendClientEmptyMessage(client_t* client)
{
	msg_t msg;
	byte buffer[10];

	MSG_Init(&msg, buffer, sizeof buffer);
	SV_SendMessageToClient(&msg, client);
}

/*
=======================
SV_SendClientSnapshot
=======================
*/
void SV_SendClientSnapshot(client_t* client)
{
	byte msg_buf[MAX_MSGLEN];
	msg_t msg;

	// build the snapshot
	SV_BuildClientSnapshot(client);

	// bots need to have their snapshots build, but
	// the query them directly without needing to be sent
	if (client->gentity && client->gentity->svFlags & SVF_BOT)
	{
		return;
	}

	MSG_Init(&msg, msg_buf, sizeof msg_buf);
	msg.allowoverflow = qtrue;

	// (re)send any reliable server commands
	SV_UpdateServerCommandsToClient(client, &msg);

	// send over all the relevant entityState_t
	// and the playerState_t
	SV_WriteSnapshotToClient(client, &msg);

	// check for overflow
	if (msg.overflowed)
	{
		Com_Printf("WARNING: msg overflowed for %s\n", client->name);
		MSG_Clear(&msg);
	}

	SV_SendMessageToClient(&msg, client);
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages()
{
	int i;
	client_t* c;

	// send a message to each connected client
	for (i = 0, c = svs.clients; i < 1; i++, c++)
	{
		if (!c->state)
		{
			continue; // not connected
		}

		if (c->state != CS_ACTIVE)
		{
			if (c->state != CS_ZOMBIE)
			{
				SV_SendClientEmptyMessage(c);
			}
			continue;
		}

		SV_SendClientSnapshot(c);
	}
}