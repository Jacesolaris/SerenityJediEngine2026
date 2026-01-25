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

// world.c -- world query functions

#include "server.h"
#include "ghoul2/ghoul2_shared.h"
#include "qcommon/cm_public.h"

/*
================
SV_clip_handleForEntity

Returns a headnode that can be used for testing or clipping to a
given entity.  If the entity is a bsp model, the headnode will
be returned, otherwise a custom box tree will be constructed.
================
*/
clipHandle_t SV_clip_handleForEntity(const sharedEntity_t* ent)
{
	if (ent->r.bmodel)
	{
		// explicit hulls in the BSP model
		return CM_InlineModel(ent->s.modelIndex);
	}
	if (ent->r.svFlags & SVF_CAPSULE)
	{
		// create a temp capsule from bounding box sizes
		return CM_TempBoxModel(ent->r.mins, ent->r.maxs, qtrue);
	}

	// create a temp tree from bounding box sizes
	return CM_TempBoxModel(ent->r.mins, ent->r.maxs, qfalse);
}

/*
===============================================================================

ENTITY CHECKING

To avoid linearly searching through lists of entities during environment testing,
the world is carved up with an evenly spaced, axially aligned bsp tree.  Entities
are kept in chains either at the final leafs, or at the first node that splits
them, which prevents having to deal with multiple fragments of a single entity.

===============================================================================
*/

using worldSector_t = struct worldSector_s
{
	int axis; // -1 = leaf node
	float dist;
	worldSector_s* children[2];
	svEntity_t* entities;
};

#define	AREA_DEPTH	4
#define	AREA_NODES	64

worldSector_t sv_worldSectors[AREA_NODES];
int sv_numworldSectors;

/*
===============
SV_SectorList_f
===============
*/
void SV_SectorList_f(void)
{
	for (int i = 0; i < AREA_NODES; i++)
	{
		const worldSector_t* sec = &sv_worldSectors[i];

		int c = 0;
		for (const svEntity_t* ent = sec->entities; ent; ent = ent->nextEntityInWorldSector)
		{
			c++;
		}
		Com_Printf("sector %i: %i entities\n", i, c);
	}
}

/*
===============
SV_CreateworldSector

Builds a uniformly subdivided tree for the given world size
===============
*/
worldSector_t* SV_CreateworldSector(const int depth, vec3_t mins, vec3_t maxs)
{
	vec3_t size;
	vec3_t mins1, maxs1, mins2, maxs2;

	worldSector_t* anode = &sv_worldSectors[sv_numworldSectors];
	sv_numworldSectors++;

	if (depth == AREA_DEPTH)
	{
		anode->axis = -1;
		anode->children[0] = anode->children[1] = nullptr;
		return anode;
	}

	VectorSubtract(maxs, mins, size);
	if (size[0] > size[1])
	{
		anode->axis = 0;
	}
	else
	{
		anode->axis = 1;
	}

	anode->dist = 0.5 * (maxs[anode->axis] + mins[anode->axis]);
	VectorCopy(mins, mins1);
	VectorCopy(mins, mins2);
	VectorCopy(maxs, maxs1);
	VectorCopy(maxs, maxs2);

	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;

	anode->children[0] = SV_CreateworldSector(depth + 1, mins2, maxs2);
	anode->children[1] = SV_CreateworldSector(depth + 1, mins1, maxs1);

	return anode;
}

/*
===============
SV_ClearWorld

===============
*/
void SV_ClearWorld(void)
{
	vec3_t mins, maxs;

	Com_Memset(sv_worldSectors, 0, sizeof sv_worldSectors);
	sv_numworldSectors = 0;

	// get world map bounds
	const clipHandle_t h = CM_InlineModel(0);
	CM_ModelBounds(h, mins, maxs);
	SV_CreateworldSector(0, mins, maxs);
}

/*
===============
SV_UnlinkEntity

===============
*/
void SV_UnlinkEntity(sharedEntity_t* g_ent)
{
	svEntity_t* ent = SV_SvEntityForGentity(g_ent);

	g_ent->r.linked = qfalse;

	worldSector_t* ws = ent->worldSector;
	if (!ws)
	{
		return; // not linked in anywhere
	}
	ent->worldSector = nullptr;

	if (ws->entities == ent)
	{
		ws->entities = ent->nextEntityInWorldSector;
		return;
	}

	for (svEntity_t* scan = ws->entities; scan; scan = scan->nextEntityInWorldSector)
	{
		if (scan->nextEntityInWorldSector == ent)
		{
			scan->nextEntityInWorldSector = ent->nextEntityInWorldSector;
			return;
		}
	}

	Com_Printf("WARNING: SV_UnlinkEntity: not found in worldSector\n");
}

/*
===============
SV_LinkEntity

===============
*/
#define MAX_TOTAL_ENT_LEAFS		128

void SV_LinkEntity(sharedEntity_t* g_ent)
{
	int leafs[MAX_TOTAL_ENT_LEAFS];
	int i;
	int lastLeaf;

	svEntity_t* ent = SV_SvEntityForGentity(g_ent);

	if (ent->worldSector)
	{
		SV_UnlinkEntity(g_ent); // unlink from old position
	}

	// encode the size into the entityState_t for client prediction
	if (g_ent->r.bmodel)
	{
		g_ent->s.solid = SOLID_BMODEL; // a solid_box will never create this value
	}
	else if (g_ent->r.contents & (CONTENTS_SOLID | CONTENTS_BODY))
	{
		// assume that x/y are equal and symetric
		i = g_ent->r.maxs[0];
		if (i < 1)
			i = 1;
		if (i > 255)
			i = 255;

		// z is not symetric
		int j = -g_ent->r.mins[2];
		if (j < 1)
			j = 1;
		if (j > 255)
			j = 255;

		// and z maxs can be negative...
		int k = g_ent->r.maxs[2] + 32;
		if (k < 1)
			k = 1;
		if (k > 255)
			k = 255;

		g_ent->s.solid = k << 16 | j << 8 | i;

		if (g_ent->s.solid == SOLID_BMODEL)
		{
			//yikes, this would make everything explode violently.
			g_ent->s.solid = k << 16 | j << 8 | i - 1;
		}
	}
	else
	{
		g_ent->s.solid = 0;
	}

	// get the position
	const float* origin = g_ent->r.currentOrigin;
	const float* angles = g_ent->r.currentAngles;

	// set the abs box
	if (g_ent->r.bmodel && (angles[0] || angles[1] || angles[2]))
	{
		const float max = RadiusFromBounds(g_ent->r.mins, g_ent->r.maxs);
		for (i = 0; i < 3; i++)
		{
			g_ent->r.absmin[i] = origin[i] - max;
			g_ent->r.absmax[i] = origin[i] + max;
		}
	}
	else
	{
		// normal
		VectorAdd(origin, g_ent->r.mins, g_ent->r.absmin);
		VectorAdd(origin, g_ent->r.maxs, g_ent->r.absmax);
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	g_ent->r.absmin[0] -= 1;
	g_ent->r.absmin[1] -= 1;
	g_ent->r.absmin[2] -= 1;
	g_ent->r.absmax[0] += 1;
	g_ent->r.absmax[1] += 1;
	g_ent->r.absmax[2] += 1;

	// link to PVS leafs
	ent->numClusters = 0;
	ent->lastCluster = 0;
	ent->areanum = -1;
	ent->areanum2 = -1;

	//get all leafs, including solids
	const int num_leafs = CM_BoxLeafnums(g_ent->r.absmin, g_ent->r.absmax,
		leafs, MAX_TOTAL_ENT_LEAFS, &lastLeaf);

	// if none of the leafs were inside the map, the
	// entity is outside the world and can be considered unlinked
	if (!num_leafs)
	{
		return;
	}

	// set areas, even from clusters that don't fit in the entity array
	for (i = 0; i < num_leafs; i++)
	{
		const int area = CM_LeafArea(leafs[i]);
		if (area != -1)
		{
			// doors may legally straggle two areas,
			// but nothing should evern need more than that
			if (ent->areanum != -1 && ent->areanum != area)
			{
				if (ent->areanum2 != -1 && ent->areanum2 != area && sv.state == SS_LOADING)
				{
					Com_DPrintf("Object %i touching 3 areas at %f %f %f\n",
						g_ent->s.number,
						g_ent->r.absmin[0], g_ent->r.absmin[1], g_ent->r.absmin[2]);
				}
				ent->areanum2 = area;
			}
			else
			{
				ent->areanum = area;
			}
		}
	}

	// store as many explicit clusters as we can
	ent->numClusters = 0;
	for (i = 0; i < num_leafs; i++)
	{
		const int cluster = CM_LeafCluster(leafs[i]);
		if (cluster != -1)
		{
			ent->clusternums[ent->numClusters++] = cluster;
			if (ent->numClusters == MAX_ENT_CLUSTERS)
			{
				break;
			}
		}
	}

	// store off a last cluster if we need to
	if (i != num_leafs)
	{
		ent->lastCluster = CM_LeafCluster(lastLeaf);
	}

	g_ent->r.linkcount++;

	// find the first world sector node that the ent's box crosses
	worldSector_t* node = sv_worldSectors;
	while (true)
	{
		if (node->axis == -1)
			break;
		if (g_ent->r.absmin[node->axis] > node->dist)
			node = node->children[0];
		else if (g_ent->r.absmax[node->axis] < node->dist)
			node = node->children[1];
		else
			break; // crosses the node
	}

	// link it in
	ent->worldSector = node;
	ent->nextEntityInWorldSector = node->entities;
	node->entities = ent;

	g_ent->r.linked = qtrue;
}

/*
============================================================================

AREA QUERY

Fills in a list of all entities who's absmin / absmax intersects the given
bounds.  This does NOT mean that they actually touch in the case of bmodels.
============================================================================
*/

using areaParms_t = struct areaParms_s
{
	const float* mins;
	const float* maxs;
	int* list;
	int count, maxcount;
};

/*
====================
SV_AreaEntities_r

====================
*/
void SV_AreaEntities_r(const worldSector_t* node, areaParms_t* ap)
{
	svEntity_t* next = nullptr;

	for (svEntity_t* check = node->entities; check; check = next)
	{
		next = check->nextEntityInWorldSector;

		const sharedEntity_t* gcheck = SV_GEntityForSvEntity(check);

		if (gcheck->r.absmin[0] > ap->maxs[0]
			|| gcheck->r.absmin[1] > ap->maxs[1]
			|| gcheck->r.absmin[2] > ap->maxs[2]
			|| gcheck->r.absmax[0] < ap->mins[0]
			|| gcheck->r.absmax[1] < ap->mins[1]
			|| gcheck->r.absmax[2] < ap->mins[2])
		{
			continue;
		}

		if (ap->count == ap->maxcount)
		{
			Com_DPrintf("SV_AreaEntities: MAXCOUNT\n");
			return;
		}

		ap->list[ap->count] = check - sv.svEntities;
		ap->count++;
	}

	if (node->axis == -1)
	{
		return; // terminal node
	}

	// recurse down both sides
	if (ap->maxs[node->axis] > node->dist)
	{
		SV_AreaEntities_r(node->children[0], ap);
	}
	if (ap->mins[node->axis] < node->dist)
	{
		SV_AreaEntities_r(node->children[1], ap);
	}
}

/*
================
SV_AreaEntities
================
*/
int SV_AreaEntities(const vec3_t mins, const vec3_t maxs, int* entity_list, const int maxcount)
{
	areaParms_t ap{};

	ap.mins = mins;
	ap.maxs = maxs;
	ap.list = entity_list;
	ap.count = 0;
	ap.maxcount = maxcount;

	SV_AreaEntities_r(sv_worldSectors, &ap);

	return ap.count;
}

//===========================================================================

using moveclip_t = struct moveclip_s
{
	vec3_t boxmins, boxmaxs; // enclose the test object along entire move
	const float* mins;
	const float* maxs; // size of the moving object
	/*
	Ghoul2 Insert Start
	*/
	vec3_t start;

	vec3_t end;

	int pass_entity_num;
	int contentmask;
	int capsule;

	int traceFlags;
	int useLod;
	trace_t trace; // make sure nothing goes under here for Ghoul2 collision purposes
	/*
	Ghoul2 Insert End
	*/
};

/*
====================
SV_ClipToEntity

====================
*/
void SV_ClipToEntity(trace_t* trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
	const int entityNum, const int contentmask, const int capsule)
{
	const sharedEntity_t* touch = SV_Gentity_num(entityNum);

	Com_Memset(trace, 0, sizeof(trace_t));

	// if it doesn't have any brushes of a type we
	// are looking for, ignore it
	if (!(contentmask & touch->r.contents))
	{
		trace->fraction = 1.0;
		return;
	}

	// might intersect, so do an exact clip
	const clipHandle_t clip_handle = SV_clip_handleForEntity(touch);

	const float* origin = touch->r.currentOrigin;
	const float* angles = touch->r.currentAngles;

	if (!touch->r.bmodel)
	{
		angles = vec3_origin; // boxes don't rotate
	}

	CM_TransformedBoxTrace(trace, start, end,
		mins, maxs, clip_handle, contentmask,
		origin, angles, capsule);

	if (trace->fraction < 1)
	{
		trace->entityNum = touch->s.number;
	}
}

/*
====================
SV_ClipMoveToEntities

====================
*/
#ifndef FINAL_BUILD
static float VectorDistance(vec3_t p1, vec3_t p2)
{
	vec3_t dir;

	VectorSubtract(p2, p1, dir);
	return VectorLength(dir);
}
#endif

static void SV_ClipMoveToEntities(moveclip_t* clip)
{
	static int touchlist[MAX_GENTITIES];
	int passOwnerNum;
	trace_t trace, oldTrace = { 0 };
	int thisOwnerShared = 1;

	const int num = SV_AreaEntities(clip->boxmins, clip->boxmaxs, touchlist, MAX_GENTITIES);

	if (clip->pass_entity_num != ENTITYNUM_NONE)
	{
		passOwnerNum = SV_Gentity_num(clip->pass_entity_num)->r.ownerNum;
		if (passOwnerNum == ENTITYNUM_NONE)
		{
			passOwnerNum = -1;
		}
	}
	else
	{
		passOwnerNum = -1;
	}

	if (SV_Gentity_num(clip->pass_entity_num)->r.svFlags & SVF_OWNERNOTSHARED)
	{
		thisOwnerShared = 0;
	}

	for (int i = 0; i < num; i++)
	{
		if (clip->trace.allsolid)
		{
			return;
		}
		sharedEntity_t* touch = SV_Gentity_num(touchlist[i]);

		// see if we should ignore this entity
		if (clip->pass_entity_num != ENTITYNUM_NONE)
		{
			if (touchlist[i] == clip->pass_entity_num)
			{
				continue; // don't clip against the pass entity
			}
			if (touch->r.ownerNum == clip->pass_entity_num)
			{
				if (touch->r.svFlags & SVF_OWNERNOTSHARED)
				{
					if (clip->contentmask != (MASK_SHOT | CONTENTS_LIGHTSABER) &&
						clip->contentmask != (MASK_SHOT))
					{
						//it's not a laser hitting the other "missile", don't care then
						continue;
					}
				}
				else
				{
					continue; // don't clip against own missiles
				}
			}
			if (touch->r.ownerNum == passOwnerNum &&
				!(touch->r.svFlags & SVF_OWNERNOTSHARED) &&
				thisOwnerShared)
			{
				continue; // don't clip against other missiles from our owner
			}

			if (touch->s.eType == ET_MISSILE &&
				!(touch->r.svFlags & SVF_OWNERNOTSHARED) &&
				touch->r.ownerNum == passOwnerNum)
			{
				//blah, hack
				continue;
			}
		}

		// if it doesn't have any brushes of a type we
		// are looking for, ignore it
		if (!(clip->contentmask & touch->r.contents))
		{
			continue;
		}

		if ((clip->contentmask == (MASK_SHOT | CONTENTS_LIGHTSABER) || clip->contentmask == MASK_SHOT) && (touch->r.
			contents > 0 && touch->r.contents & CONTENTS_NOSHOT))
		{
			continue;
		}

		// might intersect, so do an exact clip
		const clipHandle_t clip_handle = SV_clip_handleForEntity(touch);

		float* origin = touch->r.currentOrigin;
		float* angles = touch->r.currentAngles;

		if (!touch->r.bmodel)
		{
			angles = vec3_origin; // boxes don't rotate
		}

		CM_TransformedBoxTrace(&trace, clip->start, clip->end,
			clip->mins, clip->maxs, clip_handle, clip->contentmask,
			origin, angles, clip->capsule);

		if (clip->traceFlags & G2TRFLAG_DOGHOULTRACE)
		{
			// keep these older variables around for a bit, incase we need to replace them in the Ghoul2 Collision check
			oldTrace = clip->trace;
		}

		if (trace.allsolid)
		{
			clip->trace.allsolid = qtrue;
			trace.entityNum = touch->s.number;
		}
		else if (trace.startsolid)
		{
			clip->trace.startsolid = qtrue;
			trace.entityNum = touch->s.number;

			//rww - added this because we want to get the number of an ent even if our trace starts inside it.
			clip->trace.entityNum = touch->s.number;
		}

		if (trace.fraction < clip->trace.fraction)
		{
			// make sure we keep a startsolid from a previous trace
			const byte oldStart = clip->trace.startsolid;

			trace.entityNum = touch->s.number;
			clip->trace = trace;
			clip->trace.startsolid = static_cast<qboolean>(static_cast<unsigned>(clip->trace.startsolid) | static_cast<
				unsigned>(oldStart));
		}
		/*
		Ghoul2 Insert Start
		*/
#if 0
		// decide if we should do the ghoul2 collision detection right here
		if ((trace.entityNum == touch->s.number) && (clip->traceFlags))
		{
			// do we actually have a ghoul2 model here?
			if (touch->s.ghoul2)
			{
				int			oldTraceRecSize = 0;
				int			newTraceRecSize = 0;
				int			z;

				// we have to do this because sometimes you may hit a model's bounding box, but not actually penetrate the Ghoul2 Models polygons
				// this is, needless to say, not good. So we must check to see if we did actually hit the model, and if not, reset the trace stuff
				// to what it was to begin with

				// set our trace record size
				for (z = 0; z < MAX_G2_COLLISIONS; z++)
				{
					if (clip->trace.G2CollisionMap[z].mEntityNum != -1)
					{
						oldTraceRecSize++;
					}
				}

				G2API_CollisionDetect(&clip->trace.G2CollisionMap[0], *((CGhoul2Info_v*)touch->s.ghoul2),
					touch->s.angles, touch->s.origin, sv.time, touch->s.number, clip->start, clip->end, touch->s.modelScale, G2VertSpaceServer, clip->traceFlags, clip->useLod);

				// set our new trace record size

				for (z = 0; z < MAX_G2_COLLISIONS; z++)
				{
					if (clip->trace.G2CollisionMap[z].mEntityNum != -1)
					{
						newTraceRecSize++;
					}
				}

				// did we actually touch this model? If not, lets reset this ent as being hit..
				if (newTraceRecSize == oldTraceRecSize)
				{
					clip->trace = oldTrace;
				}
			}
		}
#else
		//rww - since this is multiplayer and we don't have the luxury of violating networking rules in horrible ways,
		//this must be done somewhat differently.
		if (clip->traceFlags & G2TRFLAG_DOGHOULTRACE && trace.entityNum == touch->s.number && touch->ghoul2 && (clip->
			traceFlags & G2TRFLAG_HITCORPSES || !(touch->s.eFlags & EF_DEAD)))
		{
			//standard behavior will be to ignore g2 col on dead ents, but if traceFlags is set to allow, then we'll try g2 col on EF_DEAD people too.
			static G2Trace_t G2Trace;
			vec3_t vec_out;
			float f_radius = 0.0f;
			int t_n = 0;
			int best_tr = -1;

			if (clip->mins[0] ||
				clip->maxs[0])
			{
				f_radius = (clip->maxs[0] - clip->mins[0]) / 2.0f;
			}

			if (clip->traceFlags & G2TRFLAG_THICK)
			{
				//if using this flag, make sure it's at least 1.0f
				if (f_radius < 1.0f)
				{
					f_radius = 1.0f;
				}
			}

			memset(&G2Trace, 0, sizeof G2Trace);
			while (t_n < MAX_G2_COLLISIONS)
			{
				G2Trace[t_n].mEntityNum = -1;
				t_n++;
			}

			if (touch->s.number < MAX_CLIENTS)
			{
				VectorCopy(touch->s.apos.trBase, vec_out);
			}
			else
			{
				VectorCopy(touch->r.currentAngles, vec_out);
			}
			vec_out[ROLL] = vec_out[PITCH] = 0;

			//I would think that you could trace from trace.endpos instead of clip->start, but that causes it to miss sometimes.. Not sure what it's off, but if it could be done like that, it would probably
			//be faster.
#ifndef FINAL_BUILD
			if (sv_showghoultraces->integer)
			{
				Com_Printf("Ghoul2 trace   lod=%1d   length=%6.0f   to %s\n", clip->useLod, VectorDistance(clip->start, clip->end), re->G2API_GetModelName(*(CGhoul2Info_v*)touch->ghoul2, 0));
			}
#endif

			if (com_optvehtrace &&
				com_optvehtrace->integer &&
				touch->s.eType == ET_NPC &&
				touch->s.NPC_class == CLASS_VEHICLE &&
				touch->m_pVehicle)
			{
				//for vehicles cache the transform data.
				re->G2API_CollisionDetectCache(G2Trace, *static_cast<CGhoul2Info_v*>(touch->ghoul2), vec_out,
					touch->r.currentOrigin, sv.time, touch->s.number, clip->start, clip->end,
					touch->modelScale, G2VertSpaceServer, 0, clip->useLod, f_radius);
			}
			else
			{
				re->G2API_CollisionDetect(G2Trace, *static_cast<CGhoul2Info_v*>(touch->ghoul2), vec_out,
					touch->r.currentOrigin, sv.time, touch->s.number, clip->start, clip->end,
					touch->modelScale, G2VertSpaceServer, 0, clip->useLod, f_radius);
			}

			t_n = 0;
			while (t_n < MAX_G2_COLLISIONS)
			{
				if (G2Trace[t_n].mEntityNum == touch->s.number)
				{
					//ok, valid
					best_tr = t_n;
					break;
				}
				if (G2Trace[t_n].mEntityNum == -1)
				{
					//there should not be any after the first -1
					break;
				}
				t_n++;
			}

			if (best_tr == -1)
			{
				//Well then, put the trace back to the old one.
				clip->trace = oldTrace;
			}
			else
			{
				//Otherwise, set the endpos/normal/etc. to the model location hit instead of leaving it out in space.
				VectorCopy(G2Trace[best_tr].mCollisionPosition, clip->trace.endpos);
				VectorCopy(G2Trace[best_tr].mCollisionNormal, clip->trace.plane.normal);

				if (clip->traceFlags & G2TRFLAG_GETSURFINDEX)
				{
					//we have requested that surfaceFlags be stomped over with the g2 hit surface index.
					if (clip->trace.entityNum == G2Trace[best_tr].mEntityNum)
					{
						clip->trace.surfaceFlags = G2Trace[best_tr].mSurfaceIndex;
					}
				}
			}
		}
#endif
		/*
		Ghoul2 Insert End
		*/
	}
}

/*
==================
SV_Trace

Moves the given mins/maxs volume through the world from start to end.
pass_entity_num and entities owned by pass_entity_num are explicitly not checked.
==================
*/
/*
Ghoul2 Insert Start
*/
void SV_Trace(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
	const int pass_entity_num, const int contentmask, const int capsule, const int traceFlags, const int useLod)
{
	/*
	Ghoul2 Insert End
	*/
	moveclip_t clip;

	if (!mins)
	{
		mins = vec3_origin;
	}
	if (!maxs)
	{
		maxs = vec3_origin;
	}

	Com_Memset(&clip, 0, sizeof(moveclip_t));

	// clip to world
	CM_BoxTrace(&clip.trace, start, end, mins, maxs, 0, contentmask, capsule);
	clip.trace.entityNum = clip.trace.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	if (clip.trace.fraction == 0)
	{
		*results = clip.trace;
		return; // blocked immediately by the world
	}

	clip.contentmask = contentmask;
	/*
	Ghoul2 Insert Start
	*/
	VectorCopy(start, clip.start);
	clip.traceFlags = traceFlags;
	clip.useLod = useLod;
	/*
	Ghoul2 Insert End
	*/
	//	VectorCopy( clip.trace.endpos, clip.end );
	VectorCopy(end, clip.end);
	clip.mins = mins;
	clip.maxs = maxs;
	clip.pass_entity_num = pass_entity_num;
	clip.capsule = capsule;

	// create the bounding box of the entire move
	// we can limit it to the part of the move not
	// already clipped off by the world, which can be
	// a significant savings for line of sight and shot traces
	for (int i = 0; i < 3; i++)
	{
		if (end[i] > start[i])
		{
			clip.boxmins[i] = clip.start[i] + clip.mins[i] - 1;
			clip.boxmaxs[i] = clip.end[i] + clip.maxs[i] + 1;
		}
		else
		{
			clip.boxmins[i] = clip.end[i] + clip.mins[i] - 1;
			clip.boxmaxs[i] = clip.start[i] + clip.maxs[i] + 1;
		}
	}

	// clip to other solid entities
	SV_ClipMoveToEntities(&clip);

	*results = clip.trace;
}

/*
=============
SV_PointContents
=============
*/
int SV_PointContents(const vec3_t p, const int pass_entity_num)
{
	int touch[MAX_GENTITIES];

	// get base contents from world
	int contents = CM_PointContents(p, 0);

	// or in contents from all the other entities
	const int num = SV_AreaEntities(p, p, touch, MAX_GENTITIES);

	for (int i = 0; i < num; i++)
	{
		if (touch[i] == pass_entity_num)
		{
			continue;
		}
		const sharedEntity_t* hit = SV_Gentity_num(touch[i]);
		// might intersect, so do an exact clip
		const clipHandle_t clip_handle = SV_clip_handleForEntity(hit);

		const int c2 = CM_TransformedPointContents(p, clip_handle, hit->r.currentOrigin, hit->r.currentAngles);

		contents |= c2;
	}

	return contents;
}