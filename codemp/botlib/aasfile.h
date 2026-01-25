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

//NOTE:	int =	default signed
//				default long

constexpr auto AASID = ('S' << 24) + ('A' << 16) + ('A' << 8) + 'E';
constexpr auto AASVERSION_OLD = 4;
constexpr auto AASVERSION = 5;

//presence types
constexpr auto PRESENCE_NONE = 1;
constexpr auto PRESENCE_NORMAL = 2;
constexpr auto PRESENCE_CROUCH = 4;

//travel types
constexpr auto MAX_TRAVELTYPES = 32;
constexpr auto TRAVEL_INVALID = 1;		//temporary not possible;
constexpr auto TRAVEL_WALK = 2;		//walking;
constexpr auto TRAVEL_CROUCH = 3;		//crouching;
constexpr auto TRAVEL_BARRIERJUMP = 4;		//jumping onto a barrier;
constexpr auto TRAVEL_JUMP = 5;		//jumping;
constexpr auto TRAVEL_LADDER = 6;		//climbing a ladder;
constexpr auto TRAVEL_WALKOFFLEDGE = 7;		//walking of a ledge;
constexpr auto TRAVEL_SWIM = 8;		//swimming;
constexpr auto TRAVEL_WATERJUMP = 9;	//jump out of the water;
constexpr auto TRAVEL_TELEPORT = 10;		//teleportation;
constexpr auto TRAVEL_ELEVATOR = 11;		//travel by elevator;
constexpr auto TRAVEL_ROCKETJUMP = 12;	//rocket jumping required for travel;
constexpr auto TRAVEL_BFGJUMP = 13;		//bfg jumping required for travel;
constexpr auto TRAVEL_GRAPPLEHOOK = 14;		//grappling hook required for travel;
constexpr auto TRAVEL_DOUBLEJUMP = 15;		//double jump;
constexpr auto TRAVEL_RAMPJUMP = 16;	//ramp jump;
constexpr auto TRAVEL_STRAFEJUMP = 17;		//strafe jump;
constexpr auto TRAVEL_JUMPPAD = 18;	//jump pad;
constexpr auto TRAVEL_FUNCBOB = 19;		//func bob;

//additional travel flags
constexpr auto TRAVELTYPE_MASK = 0xFFFFFF;
constexpr auto TRAVELFLAG_NOTTEAM1 = 1 << 24;
constexpr auto TRAVELFLAG_NOTTEAM2 = 2 << 24;

//face flags
constexpr auto FACE_SOLID = 1;		//just solid at the other side;
constexpr auto FACE_LADDER = 2;		//ladder;
constexpr auto FACE_GROUND = 4;		//standing on ground when in this face;
constexpr auto FACE_GAP = 8;		//gap in the ground;
constexpr auto FACE_LIQUID = 16;	//face separating two areas with liquid;
constexpr auto FACE_LIQUIDSURFACE = 32;		//face separating liquid and air;
constexpr auto FACE_BRIDGE = 64;		//can walk over this face if bridge is closed;

//area contents
constexpr auto AREACONTENTS_WATER = 1;
constexpr auto AREACONTENTS_LAVA = 2;
constexpr auto AREACONTENTS_SLIME = 4;
constexpr auto AREACONTENTS_CLUSTERPORTAL = 8;
constexpr auto AREACONTENTS_TELEPORTAL = 16;
constexpr auto AREACONTENTS_ROUTEPORTAL = 32;
constexpr auto AREACONTENTS_TELEPORTER = 64;
constexpr auto AREACONTENTS_JUMPPAD = 128;
constexpr auto AREACONTENTS_DONOTENTER = 256;
constexpr auto AREACONTENTS_VIEWPORTAL = 512;
constexpr auto AREACONTENTS_MOVER = 1024;
constexpr auto AREACONTENTS_NOTTEAM1 = 2048;
constexpr auto AREACONTENTS_NOTTEAM2 = 4096;
//number of model of the mover inside this area
constexpr auto AREACONTENTS_MODELNUMSHIFT = 24;
constexpr auto AREACONTENTS_MAXMODELNUM = 0xFF;
#define AREACONTENTS_MODELNUM			(AREACONTENTS_MAXMODELNUM << AREACONTENTS_MODELNUMSHIFT)

//area flags
constexpr auto AREA_GROUNDED = 1;		//bot can stand on the ground;
constexpr auto AREA_LADDER = 2;		//area contains one or more ladder faces;
constexpr auto AREA_LIQUID = 4;		//area contains a liquid;
constexpr auto AREA_DISABLED = 8;		//area is disabled for routing when set;
constexpr auto AREA_BRIDGE = 16;	//area ontop of a bridge;

//aas file header lumps
constexpr auto AAS_LUMPS = 14;
constexpr auto AASLUMP_BBOXES = 0;
constexpr auto AASLUMP_VERTEXES = 1;
constexpr auto AASLUMP_PLANES = 2;
constexpr auto AASLUMP_EDGES = 3;
constexpr auto AASLUMP_EDGEINDEX = 4;
constexpr auto AASLUMP_FACES = 5;
constexpr auto AASLUMP_FACEINDEX = 6;
constexpr auto AASLUMP_AREAS = 7;
constexpr auto AASLUMP_AREASETTINGS = 8;
constexpr auto AASLUMP_REACHABILITY = 9;
constexpr auto AASLUMP_NODES = 10;
constexpr auto AASLUMP_PORTALS = 11;
constexpr auto AASLUMP_PORTALINDEX = 12;
constexpr auto AASLUMP_CLUSTERS = 13;

//========== bounding box =========

//bounding box
using aas_bbox_t = struct aas_bbox_s
{
	int presencetype;
	int flags;
	vec3_t mins, maxs;
};

//============ settings ===========

//reachability to another area
using aas_reachability_t = struct aas_reachability_s
{
	int areanum;						//number of the reachable area
	int facenum;						//number of the face towards the other area
	int edgenum;						//number of the edge towards the other area
	vec3_t start;						//start point of inter area movement
	vec3_t end;							//end point of inter area movement
	int traveltype;					//type of travel required to get to the area
	unsigned short int traveltime;//travel time of the inter area movement
};

//area settings
using aas_areasettings_t = struct aas_areasettings_s
{
	//could also add all kind of statistic fields
	int contents;						//contents of the area
	int areaflags;						//several area flags
	int presencetype;					//how a bot can be present in this area
	int cluster;						//cluster the area belongs to, if negative it's a portal
	int clusterareanum;				//number of the area in the cluster
	int numreachableareas;			//number of reachable areas from this one
	int firstreachablearea;			//first reachable area in the reachable area index
};

//cluster portal
using aas_portal_t = struct aas_portal_s
{
	int areanum;						//area that is the actual portal
	int frontcluster;					//cluster at front of portal
	int backcluster;					//cluster at back of portal
	int clusterareanum[2];			//number of the area in the front and back cluster
};

//cluster portal index
using aas_portalindex_t = int;

//cluster
using aas_cluster_t = struct aas_cluster_s
{
	int numareas;						//number of areas in the cluster
	int numreachabilityareas;			//number of areas with reachabilities
	int numportals;						//number of cluster portals
	int firstportal;					//first cluster portal in the index
};

//============ 3d definition ============

typedef vec3_t aas_vertex_t;

//just a plane in the third dimension
using aas_plane_t = struct aas_plane_s
{
	vec3_t normal;						//normal vector of the plane
	float dist;							//distance of the plane (normal vector * distance = point in plane)
	int type;
};

//edge
using aas_edge_t = struct aas_edge_s
{
	int v[2];							//numbers of the vertexes of this edge
};

//edge index, negative if vertexes are reversed
using aas_edgeindex_t = int;

//a face bounds an area, often it will also separate two areas
using aas_face_t = struct aas_face_s
{
	int planenum;						//number of the plane this face is in
	int faceflags;						//face flags (no use to create face settings for just this field)
	int numedges;						//number of edges in the boundary of the face
	int firstedge;						//first edge in the edge index
	int frontarea;						//area at the front of this face
	int backarea;						//area at the back of this face
};

//face index, stores a negative index if backside of face
using aas_faceindex_t = int;

//area with a boundary of faces
using aas_area_t = struct aas_area_s
{
	int areanum;						//number of this area
	//3d definition
	int numfaces;						//number of faces used for the boundary of the area
	int firstface;						//first face in the face index used for the boundary of the area
	vec3_t mins;						//mins of the area
	vec3_t maxs;						//maxs of the area
	vec3_t center;						//'center' of the area
};

//nodes of the bsp tree
using aas_node_t = struct aas_node_s
{
	int planenum;
	int children[2];					//child nodes of this node, or areas as leaves when negative
	//when a child is zero it's a solid leaf
};

//=========== aas file ===============

//header lump
using aas_lump_t = struct aas_lump_s {
	int fileofs;
	int filelen;
};

//aas file header
using aas_header_t = struct aas_header_s
{
	int ident;
	int version;
	int bspchecksum;
	//data entries
	aas_lump_t lumps[AAS_LUMPS];
};

//====== additional information ======
/*

-	when a node child is a solid leaf the node child number is zero
-	two adjacent areas (sharing a plane at opposite sides) share a face
	this face is a portal between the areas
-	when an area uses a face from the faceindex with a positive index
	then the face plane normal points into the area
-	the face edges are stored counter clockwise using the edgeindex
-	two adjacent convex areas (sharing a face) only share One face
	this is a simple result of the areas being convex
-	the areas can't have a mixture of ground and gap faces
	other mixtures of faces in one area are allowed
-	areas with the AREACONTENTS_CLUSTERPORTAL in the settings have
	the cluster number set to the negative portal number
-	edge zero is a dummy
-	face zero is a dummy
-	area zero is a dummy
-	node zero is a dummy
*/
