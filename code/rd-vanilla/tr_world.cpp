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

#include "tr_local.h"

/*
=================
R_CullTriSurf

Returns true if the grid is completely culled away.
Also sets the clipped hint bit in tess
=================
*/
static qboolean	R_CullTriSurf(const srfTriangles_t* cv) {
	const int box_cull = R_CullLocalBox(cv->bounds);

	if (box_cull == CULL_OUT) {
		return qtrue;
	}
	return qfalse;
}

/*
=================
R_CullGrid

Returns true if the grid is completely culled away.
Also sets the clipped hint bit in tess
=================
*/
static qboolean	R_CullGrid(const srfGridMesh_t* cv) {
	int 	sphere_cull;

	if (r_nocurves->integer) {
		return qtrue;
	}

	if (tr.currententity_num != REFENTITYNUM_WORLD) {
		sphere_cull = R_CullLocalPointAndRadius(cv->localOrigin, cv->meshRadius);
	}
	else {
		sphere_cull = R_CullPointAndRadius(cv->localOrigin, cv->meshRadius);
	}

	// check for trivial reject
	if (sphere_cull == CULL_OUT)
	{
		tr.pc.c_sphere_cull_patch_out++;
		return qtrue;
	}
	// check bounding box if necessary
	if (sphere_cull == CULL_CLIP)
	{
		tr.pc.c_sphere_cull_patch_clip++;

		const int box_cull = R_CullLocalBox(cv->meshBounds);

		if (box_cull == CULL_OUT)
		{
			tr.pc.c_box_cull_patch_out++;
			return qtrue;
		}
		if (box_cull == CULL_IN)
		{
			tr.pc.c_box_cull_patch_in++;
		}
		else
		{
			tr.pc.c_box_cull_patch_clip++;
		}
	}
	else
	{
		tr.pc.c_sphere_cull_patch_in++;
	}

	return qfalse;
}

/*
================
R_CullSurface

Tries to back face cull surfaces before they are lighted or
added to the sorting list.

This will also allow mirrors on both sides of a model without recursion.
================
*/
static qboolean	R_CullSurface(surfaceType_t* surface, const shader_t* shader) {
	if (r_nocull->integer == 1) {
		return qfalse;
	}

	if (*surface == SF_GRID) {
		return R_CullGrid(reinterpret_cast<srfGridMesh_t*>(surface));
	}

	if (*surface == SF_TRIANGLES) {
		return R_CullTriSurf(reinterpret_cast<srfTriangles_t*>(surface));
	}

	if (*surface != SF_FACE) {
		return qfalse;
	}

	if (shader->cullType == CT_TWO_SIDED) {
		return qfalse;
	}

	// face culling
	if (!r_facePlaneCull->integer) {
		return qfalse;
	}

	const srfSurfaceFace_t* sface = reinterpret_cast<srfSurfaceFace_t*>(surface);
	const float d = DotProduct(tr.ori.viewOrigin, sface->plane.normal);

	// don't cull exactly on the plane, because there are levels of rounding
	// through the BSP, ICD, and hardware that may cause pixel gaps if an
	// epsilon isn't allowed here
	if (shader->cullType == CT_FRONT_SIDED) {
		if (d < sface->plane.dist - 8) {
			return qtrue;
		}
	}
	else {
		if (d > sface->plane.dist + 8) {
			return qtrue;
		}
	}

	return qfalse;
}

static int R_DlightFace(srfSurfaceFace_t* face, int dlightBits) {
	for (int i = 0; i < tr.refdef.num_dlights; i++) {
		if (!(dlightBits & 1 << i)) {
			continue;
		}
		const dlight_t* dl = &tr.refdef.dlights[i];
		const float d = DotProduct(dl->origin, face->plane.normal) - face->plane.dist;
		if (!VectorCompare(face->plane.normal, vec3_origin) && (d < -dl->radius || d > dl->radius)) {
			// dlight doesn't reach the plane
			dlightBits &= ~(1 << i);
		}
	}

	if (!dlightBits) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	face->dlightBits = dlightBits;
	return dlightBits;
}

static int R_DlightGrid(srfGridMesh_t* grid, int dlightBits) {
	for (int i = 0; i < tr.refdef.num_dlights; i++) {
		if (!(dlightBits & 1 << i)) {
			continue;
		}
		const dlight_t* dl = &tr.refdef.dlights[i];
		if (dl->origin[0] - dl->radius > grid->meshBounds[1][0]
			|| dl->origin[0] + dl->radius < grid->meshBounds[0][0]
			|| dl->origin[1] - dl->radius > grid->meshBounds[1][1]
			|| dl->origin[1] + dl->radius < grid->meshBounds[0][1]
			|| dl->origin[2] - dl->radius > grid->meshBounds[1][2]
			|| dl->origin[2] + dl->radius < grid->meshBounds[0][2]) {
			// dlight doesn't reach the bounds
			dlightBits &= ~(1 << i);
		}
	}

	if (!dlightBits) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	grid->dlightBits = dlightBits;
	return dlightBits;
}

static int R_DlightTrisurf(srfTriangles_t* surf, int dlightBits) {
	// FIXME: more dlight culling to trisurfs...
	surf->dlightBits = dlightBits;
	return dlightBits;
#if 0
	int			i;
	dlight_t* dl;

	for (i = 0; i < tr.refdef.num_dlights; i++) {
		if (!(dlightBits & (1 << i))) {
			continue;
		}
		dl = &tr.refdef.dlights[i];
		if (dl->origin[0] - dl->radius > grid->meshBounds[1][0]
			|| dl->origin[0] + dl->radius < grid->meshBounds[0][0]
			|| dl->origin[1] - dl->radius > grid->meshBounds[1][1]
			|| dl->origin[1] + dl->radius < grid->meshBounds[0][1]
			|| dl->origin[2] - dl->radius > grid->meshBounds[1][2]
			|| dl->origin[2] + dl->radius < grid->meshBounds[0][2]) {
			// dlight doesn't reach the bounds
			dlightBits &= ~(1 << i);
		}
	}

	if (!dlightBits) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	grid->dlightBits = dlightBits;
	return dlightBits;
#endif
}

/*
====================
R_DlightSurface

The given surface is going to be drawn, and it touches a leaf
that is touched by one or more dlights, so try to throw out
more dlights if possible.
====================
*/
static int R_DlightSurface(msurface_t* surf, int dlightBits) {
	if (*surf->data == SF_FACE) {
		dlightBits = R_DlightFace(reinterpret_cast<srfSurfaceFace_t*>(surf->data), dlightBits);
	}
	else if (*surf->data == SF_GRID) {
		dlightBits = R_DlightGrid(reinterpret_cast<srfGridMesh_t*>(surf->data), dlightBits);
	}
	else if (*surf->data == SF_TRIANGLES) {
		dlightBits = R_DlightTrisurf(reinterpret_cast<srfTriangles_t*>(surf->data), dlightBits);
	}
	else {
		dlightBits = 0;
	}

	if (dlightBits) {
		tr.pc.c_dlightSurfaces++;
	}

	return dlightBits;
}

/*
======================
R_AddWorldSurface
======================
*/
static void R_AddWorldSurface(msurface_t* surf, int dlightBits, const qboolean no_view_count = qfalse)
{
	if (!no_view_count)
	{
		if (surf->viewCount == tr.viewCount)
		{
			// already in this view, but lets make sure all the dlight bits are set
			if (*surf->data == SF_FACE)
			{
				reinterpret_cast<srfSurfaceFace_t*>(surf->data)->dlightBits |= dlightBits;
			}
			else if (*surf->data == SF_GRID)
			{
				reinterpret_cast<srfGridMesh_t*>(surf->data)->dlightBits |= dlightBits;
			}
			else if (*surf->data == SF_TRIANGLES)
			{
				reinterpret_cast<srfTriangles_t*>(surf->data)->dlightBits |= dlightBits;
			}
			return;
		}
		surf->viewCount = tr.viewCount;
		// FIXME: bmodel fog?
	}

	//	surf->viewCount = tr.viewCount;
		// FIXME: bmodel fog?

		// try to cull before dlighting or adding
	if (R_CullSurface(surf->data, surf->shader)) {
		return;
	}

	// check for dlighting
	if (dlightBits)
	{
		dlightBits = R_DlightSurface(surf, dlightBits);
		dlightBits = dlightBits != 0;
	}

	R_AddDrawSurf(surf->data, surf->shader, surf->fogIndex, dlightBits);
}

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
=================
R_AddBrushModelSurfaces
=================
*/
void R_AddBrushModelSurfaces(trRefEntity_t* ent) {
	const model_t* p_model = R_GetModelByHandle(ent->e.hModel);

	const bmodel_t* bmodel = p_model->bmodel;

	const int clip = R_CullLocalBox(bmodel->bounds);
	if (clip == CULL_OUT) {
		return;
	}

	if (p_model->bspInstance)
	{
		R_SetupEntityLighting(&tr.refdef, ent);
	}

	R_DlightBmodel(bmodel, qfalse);

	for (int i = 0; i < bmodel->numSurfaces; i++) {
		R_AddWorldSurface(bmodel->firstSurface + i, tr.currentEntity->dlightBits, qtrue);
	}
}

static float GetQuadArea(vec3_t v1, vec3_t v2, vec3_t v3, vec3_t v4)
{
	vec3_t	vec1, vec2, dis1, dis2;

	// Get area of tri1
	VectorSubtract(v1, v2, vec1);
	VectorSubtract(v1, v4, vec2);
	CrossProduct(vec1, vec2, dis1);
	VectorScale(dis1, 0.25f, dis1);

	// Get area of tri2
	VectorSubtract(v3, v2, vec1);
	VectorSubtract(v3, v4, vec2);
	CrossProduct(vec1, vec2, dis2);
	VectorScale(dis2, 0.25f, dis2);

	// Return addition of disSqr of each tri area
	return dis1[0] * dis1[0] + dis1[1] * dis1[1] + dis1[2] * dis1[2] +
		dis2[0] * dis2[0] + dis2[1] * dis2[1] + dis2[2] * dis2[2];
}

void RE_GetBModelVerts(const int bmodelIndex, vec3_t* verts, vec3_t normal)
{
	msurface_t* surfs;
	srfSurfaceFace_t* face;
	int					i;
	//	Not sure if we really need to track the best two candidates
	int					max_dist[2] = { 0,0 };
	int					max_indx[2] = { 0,0 };

	const model_t* p_model = R_GetModelByHandle(bmodelIndex);
	const bmodel_t* bmodel = p_model->bmodel;

	// Loop through all surfaces on the brush and find the best two candidates
	for (i = 0; i < bmodel->numSurfaces; i++)
	{
		surfs = bmodel->firstSurface + i;
		face = reinterpret_cast<srfSurfaceFace_t*>(surfs->data);

		// It seems that the safest way to handle this is by finding the area of the faces
		const int dist = GetQuadArea(face->points[0], face->points[1], face->points[2], face->points[3]);

		// Check against the highest max
		if (dist > max_dist[0])
		{
			// Shuffle our current maxes down
			max_dist[1] = max_dist[0];
			max_indx[1] = max_indx[0];

			max_dist[0] = dist;
			max_indx[0] = i;
		}
		// Check against the second highest max
		else if (dist >= max_dist[1])
		{
			// just stomp the old
			max_dist[1] = dist;
			max_indx[1] = i;
		}
	}

	// Hopefully we've found two best case candidates.  Now we should see which of these faces the viewer
	surfs = bmodel->firstSurface + max_indx[0];
	face = reinterpret_cast<srfSurfaceFace_t*>(surfs->data);
	const float dot1 = DotProduct(face->plane.normal, tr.refdef.viewaxis[0]);

	surfs = bmodel->firstSurface + max_indx[1];
	face = reinterpret_cast<srfSurfaceFace_t*>(surfs->data);
	const float dot2 = DotProduct(face->plane.normal, tr.refdef.viewaxis[0]);

	if (dot2 < dot1 && dot2 < 0.0f)
	{
		i = max_indx[1]; // use the second face
	}
	else if (dot1 < dot2 && dot1 < 0.0f)
	{
		i = max_indx[0]; // use the first face
	}
	else
	{ // Possibly only have one face, so may as well use the first face, which also should be the best one
		//i = rand() & 1; // ugh, we don't know which to use.  I'd hope this would never happen
		i = max_indx[0]; // use the first face
	}

	surfs = bmodel->firstSurface + i;
	face = reinterpret_cast<srfSurfaceFace_t*>(surfs->data);

	for (int t = 0; t < 4; t++)
	{
		VectorCopy(face->points[t], verts[t]);
	}
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
static void R_RecursiveWorldNode(mnode_t* node, int plane_bits, int dlightBits) {
	do {
		int			new_dlights[2]{};

		// if the node wasn't marked as potentially visible, exit
		if (node->visframe != tr.visCount) {
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?

		if (r_nocull->integer != 1) {
			int		r;

			if (plane_bits & 1) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[0]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					plane_bits &= ~1;			// all descendants will also be in front
				}
			}

			if (plane_bits & 2) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[1]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					plane_bits &= ~2;			// all descendants will also be in front
				}
			}

			if (plane_bits & 4) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[2]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					plane_bits &= ~4;			// all descendants will also be in front
				}
			}

			if (plane_bits & 8) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[3]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					plane_bits &= ~8;			// all descendants will also be in front
				}
			}

			if (plane_bits & 16) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[4]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					plane_bits &= ~16;			// all descendants will also be in front
				}
			}
		}

		if (node->contents != -1) {
			break;
		}

		// determine which dlights are needed
		if (r_nocull->integer != 2)
		{
			new_dlights[0] = 0;
			new_dlights[1] = 0;
			if (dlightBits)
			{
				for (int i = 0; i < tr.refdef.num_dlights; i++)
				{
					if (dlightBits & 1 << i) {
						const dlight_t* dl = &tr.refdef.dlights[i];
						const float dist = DotProduct(dl->origin, node->plane->normal) - node->plane->dist;

						if (dist > -dl->radius) {
							new_dlights[0] |= 1 << i;
						}
						if (dist < dl->radius) {
							new_dlights[1] |= 1 << i;
						}
					}
				}
			}
		}
		else
		{
			new_dlights[0] = dlightBits;
			new_dlights[1] = dlightBits;
		}
		// recurse down the children, front side first
		R_RecursiveWorldNode(node->children[0], plane_bits, new_dlights[0]);

		// tail recurse
		node = node->children[1];
		dlightBits = new_dlights[1];
	} while (true);

	{
		tr.pc.c_leafs++;

		// add to z buffer bounds
		if (node->mins[0] < tr.viewParms.visBounds[0][0]) {
			tr.viewParms.visBounds[0][0] = node->mins[0];
		}
		if (node->mins[1] < tr.viewParms.visBounds[0][1]) {
			tr.viewParms.visBounds[0][1] = node->mins[1];
		}
		if (node->mins[2] < tr.viewParms.visBounds[0][2]) {
			tr.viewParms.visBounds[0][2] = node->mins[2];
		}

		if (node->maxs[0] > tr.viewParms.visBounds[1][0]) {
			tr.viewParms.visBounds[1][0] = node->maxs[0];
		}
		if (node->maxs[1] > tr.viewParms.visBounds[1][1]) {
			tr.viewParms.visBounds[1][1] = node->maxs[1];
		}
		if (node->maxs[2] > tr.viewParms.visBounds[1][2]) {
			tr.viewParms.visBounds[1][2] = node->maxs[2];
		}

		// add the individual surfaces
		msurface_t** mark = node->firstmarksurface;
		int c = node->nummarksurfaces;
		while (c--) {
			// the surface may have already been added if it
			// spans multiple leafs
			msurface_t* surf = *mark;
			R_AddWorldSurface(surf, dlightBits);
			mark++;
		}
	}
}

/*
===============
R_PointInLeaf
===============
*/
static mnode_t* R_PointInLeaf(const vec3_t p) {
	if (!tr.world) {
		Com_Error(ERR_DROP, "R_PointInLeaf: bad model");
	}

	mnode_t* node = tr.world->nodes;
	while (true) {
		if (node->contents != -1) {
			break;
		}
		const cplane_t* plane = node->plane;

		const float d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0) {
			node = node->children[0];
		}
		else {
			node = node->children[1];
		}
	}

	return node;
}

/*
==============
R_ClusterPVS
==============
*/
static const byte* R_ClusterPVS(const int cluster)
{
	if (!tr.world || !tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters)
	{
		return tr.world->novis;
	}

	return tr.world->vis + cluster * tr.world->clusterBytes;
}

/*
=================
R_inPVS
=================
*/

qboolean R_inPVS(const vec3_t p1, const vec3_t p2)
{
	const mnode_t* leaf = R_PointInLeaf(p1);
	const byte* vis = ri.CM_ClusterPVS(leaf->cluster);
	leaf = R_PointInLeaf(p2);

	if (!(vis[leaf->cluster >> 3] & 1 << (leaf->cluster & 7))) {
		return qfalse;
	}
	return qtrue;
}

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
static void R_MarkLeaves(void) {
	int		i;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if (r_lockpvs->integer) {
		return;
	}

	// current viewcluster
	mnode_t* leaf = R_PointInLeaf(tr.viewParms.pvsOrigin);
	int cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	// if r_showcluster was just turned on, remark everything
	if (tr.viewCluster == cluster && !tr.refdef.areamaskModified
		&& !r_showcluster->modified) {
		return;
	}

	if (r_showcluster->modified || r_showcluster->integer) {
		r_showcluster->modified = qfalse;
		if (r_showcluster->integer) {
			ri.Printf(PRINT_ALL, "cluster:%i  area:%i\n", cluster, leaf->area);
		}
	}

	tr.visCount++;
	tr.viewCluster = cluster;

	if (r_novis->integer || tr.viewCluster == -1) {
		for (i = 0; i < tr.world->numnodes; i++) {
			if (tr.world->nodes[i].contents != CONTENTS_SOLID) {
				tr.world->nodes[i].visframe = tr.visCount;
			}
		}
		return;
	}

	const byte* vis = R_ClusterPVS(tr.viewCluster);

	for (i = 0, leaf = tr.world->nodes; i < tr.world->numnodes; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster < 0 || cluster >= tr.world->numClusters) {
			continue;
		}

		// check general pvs
		if (!(vis[cluster >> 3] & 1 << (cluster & 7))) {
			continue;
		}

		// check for door connection
		if (tr.refdef.areamask[leaf->area >> 3] & 1 << (leaf->area & 7)) {
			continue;		// not visible
		}

		mnode_t* parent = leaf;
		do {
			if (parent->visframe == tr.visCount)
				break;
			parent->visframe = tr.visCount;
			parent = parent->parent;
		} while (parent);
	}
}

/*
=============
R_AddWorldSurfaces
=============
*/
void R_AddWorldSurfaces(void)
{
	if (!r_drawworld->integer)
	{
		return;
	}

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL) {
		return;
	}

	tr.currententity_num = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currententity_num << QSORT_REFENTITYNUM_SHIFT;

	// determine which leaves are in the PVS / areamask
	R_MarkLeaves();

	// clear out the visible min/max
	ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);

	// perform frustum culling and add all the potentially visible surfaces
	if (tr.refdef.num_dlights > 32) {
		tr.refdef.num_dlights = 32;
	}

	R_RecursiveWorldNode(tr.world->nodes, 31, (1 << tr.refdef.num_dlights) - 1);
}