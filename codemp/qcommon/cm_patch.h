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

#pragma once

constexpr auto MAX_FACETS = 4096;
constexpr auto MAX_PATCH_PLANES = 8192;

using patchPlane_t = struct patchPlane_s
{
	float plane[4];
	int signbits; // signx + (signy<<1) + (signz<<2), used as lookup during collision
};

using facet_t = struct facet_s
{
	int surfacePlane;
	int numBorders; // 3 or four + 6 axial bevels + 4 or 3 * 4 edge bevels
	int borderPlanes[4 + 6 + 16];
	int borderInward[4 + 6 + 16];
	qboolean borderNoAdjust[4 + 6 + 16];
};

using patchCollide_t = struct patchCollide_s
{
	vec3_t bounds[2];
	int numPlanes; // surface planes plus edge planes
	patchPlane_t* planes;
	int numFacets;
	facet_t* facets;
};

constexpr auto MAX_GRID_SIZE = 129;

using cGrid_t = struct cGrid_s
{
	int width;
	int height;
	qboolean wrapWidth;
	qboolean wrapHeight;
	vec3_t points[MAX_GRID_SIZE][MAX_GRID_SIZE]; // [width][height]
};

constexpr auto SUBDIVIDE_DISTANCE = 16; //4	// never more than this units away from curve;
constexpr auto PLANE_TRI_EPSILON = 0.1;
constexpr auto WRAP_POINT_EPSILON = 0.1;

patchCollide_s* CM_GeneratePatchCollide(int width, int height, vec3_t* points);
