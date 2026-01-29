/*
===========================================================================
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

// sv_gameapi.cpp  -- interface to the game dll
//Anything above this #include will be ignored by the compiler

#include "server.h"
#include "botlib/botlib.h"
#include "qcommon/stringed_ingame.h"
#include "qcommon/RoffSystem.h"
#include "ghoul2/ghoul2_shared.h"
#include "qcommon/cm_public.h"
#include "icarus/GameInterface.h"
#include "qcommon/timing.h"
#include "NPCNav/navigator.h"
#include <string.h>

botlib_export_t* botlib_export;

// game interface
static gameExport_t* ge; // game export table
static vm_t* gvm; // game vm, valid for legacy and new api

//
// game vmMain calls
//

void GVM_InitGame(const int levelTime, const int randomSeed, const int restart)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_INIT, levelTime, randomSeed, restart);
		return;
	}
	VMSwap v(gvm);

	ge->InitGame(levelTime, randomSeed, restart);
}

void GVM_ShutdownGame(const int restart)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_SHUTDOWN, restart);
		return;
	}
	VMSwap v(gvm);

	ge->ShutdownGame(restart);
}

char* GVM_ClientConnect(const int clientNum, const qboolean firstTime, const qboolean isBot)
{
	if (gvm->isLegacy)
		return reinterpret_cast<char*>(VM_Call(gvm, GAME_CLIENT_CONNECT, clientNum, firstTime, isBot));
	VMSwap v(gvm);

	return ge->ClientConnect(clientNum, firstTime, isBot);
}

void GVM_ClientBegin(const int clientNum)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_CLIENT_BEGIN, clientNum);
		return;
	}
	VMSwap v(gvm);

	ge->ClientBegin(clientNum, qtrue);
}

qboolean GVM_client_userinfo_changed(const int clientNum)
{
	if (gvm->isLegacy)
		return static_cast<qboolean>(VM_Call(gvm, GAME_CLIENT_USERINFO_CHANGED, clientNum));
	VMSwap v(gvm);

	return ge->client_userinfo_changed(clientNum);
}

void GVM_ClientDisconnect(const int clientNum)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_CLIENT_DISCONNECT, clientNum);
		return;
	}
	VMSwap v(gvm);

	ge->ClientDisconnect(clientNum);
}

void GVM_ClientCommand(const int clientNum)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_CLIENT_COMMAND, clientNum);
		return;
	}
	VMSwap v(gvm);

	ge->ClientCommand(clientNum);
}

void GVM_ClientThink(const int clientNum, usercmd_t* ucmd)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_CLIENT_THINK, clientNum, reinterpret_cast<intptr_t>(ucmd));
		return;
	}
	VMSwap v(gvm);

	ge->ClientThink(clientNum, ucmd);
}

void GVM_RunFrame(const int levelTime)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_RUN_FRAME, levelTime);
		return;
	}
	VMSwap v(gvm);

	ge->RunFrame(levelTime);
}

qboolean GVM_ConsoleCommand(void)
{
	if (gvm->isLegacy)
		return static_cast<qboolean>(VM_Call(gvm, GAME_CONSOLE_COMMAND));
	VMSwap v(gvm);

	return ge->ConsoleCommand();
}

int GVM_BotAIStartFrame(const int time)
{
	if (gvm->isLegacy)
		return VM_Call(gvm, BOTAI_startFrame, time);
	VMSwap v(gvm);

	return ge->BotAIStartFrame(time);
}

void GVM_ROFF_NotetrackCallback(const int entID, const char* notetrack)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_ROFF_NOTETRACK_CALLBACK, entID, reinterpret_cast<intptr_t>(notetrack));
		return;
	}
	VMSwap v(gvm);

	ge->ROFF_NotetrackCallback(entID, notetrack);
}

void GVM_SpawnRMGEntity(void)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_SPAWN_RMG_ENTITY);
		return;
	}
	VMSwap v(gvm);

	ge->SpawnRMGEntity();
}

int GVM_ICARUS_PlaySound(void)
{
	if (gvm->isLegacy)
		return VM_Call(gvm, GAME_ICARUS_PLAYSOUND);
	VMSwap v(gvm);

	return ge->ICARUS_PlaySound();
}

qboolean GVM_ICARUS_Set(void)
{
	if (gvm->isLegacy)
		return static_cast<qboolean>(VM_Call(gvm, GAME_ICARUS_SET));
	VMSwap v(gvm);

	return ge->ICARUS_Set();
}

void GVM_ICARUS_Lerp2Pos(void)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_ICARUS_LERP2POS);
		return;
	}
	VMSwap v(gvm);

	ge->ICARUS_Lerp2Pos();
}

void GVM_ICARUS_Lerp2Origin(void)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_ICARUS_LERP2ORIGIN);
		return;
	}
	VMSwap v(gvm);

	ge->ICARUS_Lerp2Origin();
}

void GVM_ICARUS_Lerp2Angles(void)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_ICARUS_LERP2ANGLES);
		return;
	}
	VMSwap v(gvm);

	ge->ICARUS_Lerp2Angles();
}

int GVM_ICARUS_GetTag(void)
{
	if (gvm->isLegacy)
		return VM_Call(gvm, GAME_ICARUS_GETTAG);
	VMSwap v(gvm);

	return ge->ICARUS_GetTag();
}

void GVM_ICARUS_Lerp2Start(void)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_ICARUS_LERP2START);
		return;
	}
	VMSwap v(gvm);

	ge->ICARUS_Lerp2Start();
}

void GVM_ICARUS_Lerp2End(void)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_ICARUS_LERP2END);
		return;
	}
	VMSwap v(gvm);

	ge->ICARUS_Lerp2End();
}

void GVM_ICARUS_Use(void)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_ICARUS_USE);
		return;
	}
	VMSwap v(gvm);

	ge->ICARUS_Use();
}

void GVM_ICARUS_Kill(void)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_ICARUS_KILL);
		return;
	}
	VMSwap v(gvm);

	ge->ICARUS_Kill();
}

void GVM_ICARUS_Remove(void)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_ICARUS_REMOVE);
		return;
	}
	VMSwap v(gvm);

	ge->ICARUS_Remove();
}

void GVM_ICARUS_Play(void)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_ICARUS_PLAY);
		return;
	}
	VMSwap v(gvm);

	ge->ICARUS_Play();
}

int GVM_ICARUS_GetFloat(void)
{
	if (gvm->isLegacy)
		return VM_Call(gvm, GAME_ICARUS_GETFLOAT);
	VMSwap v(gvm);

	return ge->ICARUS_GetFloat();
}

int GVM_ICARUS_GetVector(void)
{
	if (gvm->isLegacy)
		return VM_Call(gvm, GAME_ICARUS_GETVECTOR);
	VMSwap v(gvm);

	return ge->ICARUS_GetVector();
}

int GVM_ICARUS_GetString(void)
{
	if (gvm->isLegacy)
		return VM_Call(gvm, GAME_ICARUS_GETSTRING);
	VMSwap v(gvm);

	return ge->ICARUS_GetString();
}

void GVM_ICARUS_SoundIndex(void)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_ICARUS_SOUNDINDEX);
		return;
	}
	VMSwap v(gvm);

	ge->ICARUS_SoundIndex();
}

int GVM_ICARUS_GetSetIDForString(void)
{
	if (gvm->isLegacy)
		return VM_Call(gvm, GAME_ICARUS_GETSETIDFORSTRING);
	VMSwap v(gvm);

	return ge->ICARUS_GetSetIDForString();
}

qboolean GVM_NAV_ClearPathToPoint(const int entID, vec3_t pmins, vec3_t pmaxs, vec3_t point, const int clipmask,
	const int okToHitEnt)
{
	if (gvm->isLegacy)
		return static_cast<qboolean>(VM_Call(gvm, GAME_NAV_CLEARPATHTOPOINT, entID, reinterpret_cast<intptr_t>(pmins),
			reinterpret_cast<intptr_t>(pmaxs), reinterpret_cast<intptr_t>(point),
			clipmask,
			okToHitEnt));
	VMSwap v(gvm);

	return ge->NAV_ClearPathToPoint(entID, pmins, pmaxs, point, clipmask, okToHitEnt);
}

qboolean GVM_NPC_ClearLOS2(const int entID, const vec3_t end)
{
	if (gvm->isLegacy)
		return static_cast<qboolean>(VM_Call(gvm, GAME_NAV_CLEARLOS, entID, reinterpret_cast<intptr_t>(end)));
	VMSwap v(gvm);

	return ge->NPC_ClearLOS2(entID, end);
}

int GVM_NAVNEW_ClearPathBetweenPoints(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, const int ignore,
	const int clipmask)
{
	if (gvm->isLegacy)
		return VM_Call(gvm, GAME_NAV_CLEARPATHBETWEENPOINTS, reinterpret_cast<intptr_t>(start),
			reinterpret_cast<intptr_t>(end), reinterpret_cast<intptr_t>(mins),
			reinterpret_cast<intptr_t>(maxs), ignore, clipmask);
	VMSwap v(gvm);

	return ge->NAVNEW_ClearPathBetweenPoints(start, end, mins, maxs, ignore, clipmask);
}

qboolean GVM_NAV_CheckNodeFailedForEnt(const int entID, const int nodeNum)
{
	if (gvm->isLegacy)
		return static_cast<qboolean>(VM_Call(gvm, GAME_NAV_CHECKNODEFAILEDFORENT, entID, nodeNum));
	VMSwap v(gvm);

	return ge->NAV_CheckNodeFailedForEnt(entID, nodeNum);
}

qboolean GVM_NAV_EntIsUnlockedDoor(const int entityNum)
{
	if (gvm->isLegacy)
		return static_cast<qboolean>(VM_Call(gvm, GAME_NAV_ENTISUNLOCKEDDOOR, entityNum));
	VMSwap v(gvm);

	return ge->NAV_EntIsUnlockedDoor(entityNum);
}

qboolean GVM_NAV_EntIsDoor(const int entityNum)
{
	if (gvm->isLegacy)
		return static_cast<qboolean>(VM_Call(gvm, GAME_NAV_ENTISDOOR, entityNum));
	VMSwap v(gvm);

	return ge->NAV_EntIsDoor(entityNum);
}

qboolean GVM_NAV_EntIsBreakable(const int entityNum)
{
	if (gvm->isLegacy)
		return static_cast<qboolean>(VM_Call(gvm, GAME_NAV_ENTISBREAKABLE, entityNum));
	VMSwap v(gvm);

	return ge->NAV_EntIsBreakable(entityNum);
}

qboolean GVM_NAV_EntIsRemovableUsable(const int entNum)
{
	if (gvm->isLegacy)
		return static_cast<qboolean>(VM_Call(gvm, GAME_NAV_ENTISREMOVABLEUSABLE, entNum));
	VMSwap v(gvm);

	return ge->NAV_EntIsRemovableUsable(entNum);
}

void GVM_NAV_FindCombatPointWaypoints(void)
{
	if (gvm->isLegacy)
	{
		VM_Call(gvm, GAME_NAV_FINDCOMBATPOINTWAYPOINTS);
		return;
	}
	VMSwap v(gvm);

	ge->NAV_FindCombatPointWaypoints();
}

int GVM_BG_GetItemIndexByTag(const int tag, const int type)
{
	if (gvm->isLegacy)
		return VM_Call(gvm, GAME_GETITEMINDEXBYTAG, tag, type);
	VMSwap v(gvm);

	return ge->BG_GetItemIndexByTag(tag, type);
}

//
// game syscalls
//	only used by legacy mods!
//

// legacy syscall

siegePers_t sv_siegePersData = { qfalse, 0, 0 };

extern float g_svCullDist;
int CM_ModelContents(clipHandle_t model, int sub_bsp_index);
int CM_LoadSubBSP(const char* name, qboolean clientload);
int CM_FindSubBSP(int modelIndex);
char* CM_SubBSPEntityString(int index);
qboolean Q3_TaskIDPending(const sharedEntity_t* ent, taskID_t taskType);
void Q3_TaskIDSet(sharedEntity_t* ent, taskID_t taskType, int taskID);
void Q3_TaskIDComplete(sharedEntity_t* ent, taskID_t taskType);
void Q3_SetVar(int taskID, int entID, const char* type_name, const char* data);
int Q3_VariableDeclared(const char* name);
int Q3_GetFloatVariable(const char* name, float* value);
int Q3_GetStringVariable(const char* name, const char** value);
int Q3_GetVectorVariable(const char* name, vec3_t value);
void SV_BotWaypointReception(int wpnum, wpobject_t** wps);
void SV_BotCalculatePaths(int rmg);

static void SV_LocateGameData(sharedEntity_t* gEnts, const int numGEntities, const int sizeofGEntity_t,
	playerState_t* clients, const int sizeofGameClient)
{
	sv.gentities = gEnts;
	sv.gentitySize = sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gameClients = clients;
	sv.gameClientSize = sizeofGameClient;
}

static void SV_GameDropClient(const int clientNum, const char* reason)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		return;
	}
	SV_DropClient(svs.clients + clientNum, reason);
}

static void SV_GameSendServerCommand(const int clientNum, const char* text)
{
	if (clientNum == -1)
	{
		SV_SendServerCommand(nullptr, "%s", text);
	}
	else
	{
		if (clientNum < 0 || clientNum >= sv_maxclients->integer)
		{
			return;
		}
		SV_SendServerCommand(svs.clients + clientNum, "%s", text);
	}
}

static qboolean SV_EntityContact(const vec3_t mins, const vec3_t maxs, const sharedEntity_t* gEnt, const int capsule)
{
	trace_t trace;

	// check for exact collision
	const float* origin = gEnt->r.currentOrigin;
	const float* angles = gEnt->r.currentAngles;

	const clipHandle_t ch = SV_clip_handleForEntity(gEnt);
	CM_TransformedBoxTrace(&trace, vec3_origin, vec3_origin, mins, maxs,
		ch, -1, origin, angles, capsule);

	return static_cast<qboolean>(trace.startsolid);
}

static void SV_SetBrushModel(sharedEntity_t* ent, const char* name)
{
	clipHandle_t h;
	vec3_t mins, maxs;

	if (!name)
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: NULL");
	}

	if (name[0] == '*')
	{
		ent->s.modelIndex = atoi(name + 1);

		if (sv.mLocalSubBSPIndex != -1)
		{
			ent->s.modelIndex += sv.mLocalSubBSPModelOffset;
		}

		h = CM_InlineModel(ent->s.modelIndex);

		CM_ModelBounds(h, mins, maxs);

		VectorCopy(mins, ent->r.mins);
		VectorCopy(maxs, ent->r.maxs);
		ent->r.bmodel = qtrue;
		ent->r.contents = CM_ModelContents(h, -1);
	}
	else if (name[0] == '#')
	{
		ent->s.modelIndex = CM_LoadSubBSP(va("maps/%s.bsp", name + 1), qfalse);
		CM_ModelBounds(ent->s.modelIndex, mins, maxs);

		VectorCopy(mins, ent->r.mins);
		VectorCopy(maxs, ent->r.maxs);
		ent->r.bmodel = qtrue;

		//rwwNOTE: We don't ever want to set contents -1, it includes CONTENTS_LIGHTSABER.
		//Lots of stuff will explode if there's a brush with CONTENTS_LIGHTSABER that isn't attached to a client owner.
		//ent->contents = -1;		// we don't know exactly what is in the brushes
		h = CM_InlineModel(ent->s.modelIndex);
		ent->r.contents = CM_ModelContents(h, CM_FindSubBSP(ent->s.modelIndex));
	}
	else
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name);
	}
}

static qboolean SV_inPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	int leafnum = CM_PointLeafnum(p1);
	int cluster = CM_LeafCluster(leafnum);
	//	area1 = CM_LeafArea( leafnum );
	const byte* mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	//	area2 = CM_LeafArea( leafnum );

	if (mask && !(mask[cluster >> 3] & 1 << (cluster & 7)))
		return qfalse;

	return qtrue;
}

static void SV_GetServerinfo(char* buffer, const int buffer_size)
{
	if (buffer_size < 1)
	{
		Com_Error(ERR_DROP, "SV_GetServerinfo: buffer_size == %i", buffer_size);
	}
	Q_strncpyz(buffer, Cvar_InfoString(CVAR_SERVERINFO), buffer_size);
}

static void SV_AdjustAreaPortalState(sharedEntity_t* ent, const qboolean open)
{
	const svEntity_t* svEnt = SV_SvEntityForGentity(ent);
	if (svEnt->areanum2 == -1)
		return;

	CM_AdjustAreaPortalState(svEnt->areanum, svEnt->areanum2, open);
}

static void SV_GetUsercmd(const int clientNum, usercmd_t* cmd)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum);
	}
	*cmd = svs.clients[clientNum].lastUsercmd;
}

static sharedEntity_t gLocalModifier;

static sharedEntity_t* ConvertedEntity(sharedEntity_t* ent)
{
	//Return an entity with the memory shifted around to allow reading/modifying VM memory
	int i = 0;

	assert(ent);

	gLocalModifier.s = ent->s;
	gLocalModifier.r = ent->r;
	while (i < NUM_TIDS)
	{
		gLocalModifier.taskID[i] = ent->taskID[i];
		i++;
	}
	i = 0;
	gLocalModifier.parms = static_cast<parms_t*>(VM_ArgPtr(reinterpret_cast<intptr_t>(ent->parms)));
	while (i < NUM_BSETS)
	{
		gLocalModifier.behaviorSet[i] = static_cast<char*>(VM_ArgPtr(reinterpret_cast<intptr_t>(ent->behaviorSet[i])));
		i++;
	}
	i = 0;
	gLocalModifier.script_targetname = static_cast<char*>(VM_ArgPtr(reinterpret_cast<intptr_t>(ent->script_targetname)));
	gLocalModifier.delayScriptTime = ent->delayScriptTime;
	gLocalModifier.fullName = static_cast<char*>(VM_ArgPtr(reinterpret_cast<intptr_t>(ent->fullName)));
	gLocalModifier.targetname = static_cast<char*>(VM_ArgPtr(reinterpret_cast<intptr_t>(ent->targetname)));
	gLocalModifier.classname = static_cast<char*>(VM_ArgPtr(reinterpret_cast<intptr_t>(ent->classname)));

	gLocalModifier.ghoul2 = ent->ghoul2;

	return &gLocalModifier;
}

static const char* SV_SetActiveSubBSP(const int index)
{
	if (index >= 0)
	{
		sv.mLocalSubBSPIndex = CM_FindSubBSP(index);
		sv.mLocalSubBSPModelOffset = index;
		sv.mLocalSubBSPEntityParsePoint = CM_SubBSPEntityString(sv.mLocalSubBSPIndex);
		return sv.mLocalSubBSPEntityParsePoint;
	}

	sv.mLocalSubBSPIndex = -1;
	return nullptr;
}

static qboolean SV_GetEntityToken(char* buffer, const int buffer_size)
{
	char* s;

	if (sv.mLocalSubBSPIndex == -1)
	{
		s = COM_Parse(const_cast<const char**>(&sv.entityParsePoint));
		Q_strncpyz(buffer, s, buffer_size);
		if (!sv.entityParsePoint && !s[0])
			return qfalse;
		return qtrue;
	}
	s = COM_Parse(const_cast<const char**>(&sv.mLocalSubBSPEntityParsePoint));
	Q_strncpyz(buffer, s, buffer_size);
	if (!sv.mLocalSubBSPEntityParsePoint && !s[0])
		return qfalse;
	return qtrue;
}

static void SV_PrecisionTimerStart(void** timer)
{
	const auto newTimer = new timing_c; //create the new timer
	*timer = newTimer; //assign the pointer within the pointer to point at the mem addr of our new timer
	newTimer->Start(); //start the timer
}

static int SV_PrecisionTimerEnd(void* timer)
{
	const auto theTimer = static_cast<timing_c*>(timer);
	//this is the pointer we assigned in start, so we can directly cast it back
	const int r = theTimer->End(); //get the result
	delete theTimer; //delete the timer since we're done with it
	return r; //return the result
}

static void SV_RegisterSharedMemory(char* memory)
{
	sv.mSharedMemory = memory;
}

static void SV_SetServerCull(const float cullDistance)
{
	g_svCullDist = cullDistance;
}

static void SV_SiegePersSet(siegePers_t* siegePers)
{
	sv_siegePersData = *siegePers;
}

static void SV_SiegePersGet(siegePers_t* siegePers)
{
	*siegePers = sv_siegePersData;
}

qboolean SV_ROFF_Clean(void)
{
	return theROFFSystem.Clean(qfalse);
}

void SV_ROFF_UpdateEntities(void)
{
	theROFFSystem.UpdateEntities(qfalse);
}

int SV_ROFF_Cache(char* file)
{
	return theROFFSystem.Cache(file, qfalse);
}

qboolean SV_ROFF_Play(const int entID, const int roff_id, const qboolean do_translation)
{
	return theROFFSystem.Play(entID, roff_id, do_translation, qfalse);
}

qboolean SV_ROFF_Purge_Ent(const int entID)
{
	return theROFFSystem.PurgeEnt(entID, qfalse);
}

static qboolean SV_ICARUS_RegisterScript(const char* name, const qboolean bCalledDuringInterrogate)
{
	return static_cast<qboolean>(ICARUS_RegisterScript(name, bCalledDuringInterrogate));
}

static qboolean SV_ICARUS_ValidEnt(sharedEntity_t* ent)
{
	return static_cast<qboolean>(ICARUS_ValidEnt(ent));
}

static qboolean ICARUS_IsInitialized(const int entID)
{
	if (!gSequencers[entID] || !gTaskManagers[entID])
		return qfalse;

	return qtrue;
}

static qboolean ICARUS_MaintainTaskManager(const int entID)
{
	if (gTaskManagers[entID])
	{
		gTaskManagers[entID]->Update();
		return qtrue;
	}
	return qfalse;
}

static qboolean ICARUS_IsRunning(const int entID)
{
	if (!gTaskManagers[entID] || !gTaskManagers[entID]->IsRunning())
		return qfalse;
	return qtrue;
}

static qboolean ICARUS_TaskIDPending(sharedEntity_t* ent, int taskID)
{
	return Q3_TaskIDPending(ent, static_cast<taskID_t>(taskID));
}

static void SV_ICARUS_TaskIDSet(sharedEntity_t* ent, int taskType, const int taskID)
{
	Q3_TaskIDSet(ent, static_cast<taskID_t>(taskType), taskID);
}

static void SV_ICARUS_TaskIDComplete(sharedEntity_t* ent, int taskType)
{
	Q3_TaskIDComplete(ent, static_cast<taskID_t>(taskType));
}

static int SV_ICARUS_GetStringVariable(const char* name, const char* value)
{
	const char* rec = value;
	return Q3_GetStringVariable(name, &rec);
}

static int SV_ICARUS_GetVectorVariable(const char* name, const vec3_t value)
{
	return Q3_GetVectorVariable(name, const_cast<float*>(value));
}

static void SV_Nav_Init(void)
{
	navigator.Init();
}

static void SV_Nav_Free(void)
{
	navigator.Free();
}

static qboolean SV_Nav_Load(const char* filename, const int checksum)
{
	return static_cast<qboolean>(navigator.Load(filename, checksum));
}

static qboolean SV_Nav_Save(const char* filename, const int checksum)
{
	return static_cast<qboolean>(navigator.Save(filename, checksum));
}

static int SV_Nav_AddRawPoint(vec3_t point, const int flags, const int radius)
{
	return navigator.AddRawPoint(point, flags, radius);
}

static void SV_Nav_CalculatePaths(const qboolean recalc)
{
	navigator.CalculatePaths(recalc);
}

static void SV_Nav_HardConnect(const int first, const int second)
{
	navigator.HardConnect(first, second);
}

static void SV_Nav_ShowNodes(void)
{
	navigator.ShowNodes();
}

static void SV_Nav_ShowEdges(void)
{
	navigator.ShowEdges();
}

static void SV_Nav_ShowPath(const int start, const int end)
{
	navigator.ShowPath(start, end);
}

static int SV_Nav_GetNearestNode(sharedEntity_t* ent, const int lastID, const int flags, const int targetID)
{
	return navigator.GetNearestNode(ent, lastID, flags, targetID);
}

static int SV_Nav_GetBestNode(const int startID, const int endID, const int rejectID)
{
	return navigator.GetBestNode(startID, endID, rejectID);
}

static int SV_Nav_GetNodePosition(const int nodeID, vec3_t out)
{
	return navigator.GetNodePosition(nodeID, out);
}

static int SV_Nav_GetNodeNumEdges(const int nodeID)
{
	return navigator.GetNodeNumEdges(nodeID);
}

static int SV_Nav_GetNodeEdge(const int nodeID, const int edge)
{
	return navigator.GetNodeEdge(nodeID, edge);
}

static int SV_Nav_GetNumNodes(void)
{
	return navigator.GetNumNodes();
}

static qboolean SV_Nav_Connected(const int startID, const int endID)
{
	return static_cast<qboolean>(navigator.Connected(startID, endID));
}

static int SV_Nav_GetPathCost(const int startID, const int endID)
{
	return navigator.GetPathCost(startID, endID);
}

static int SV_Nav_GetEdgeCost(const int startID, const int endID)
{
	return navigator.GetEdgeCost(startID, endID);
}

static int SV_Nav_GetProjectedNode(vec3_t origin, const int nodeID)
{
	return navigator.GetProjectedNode(origin, nodeID);
}

static void SV_Nav_CheckFailedNodes(sharedEntity_t* ent)
{
	navigator.CheckFailedNodes(ent);
}

static void SV_Nav_AddFailedNode(sharedEntity_t* ent, const int nodeID)
{
	navigator.AddFailedNode(ent, nodeID);
}

static qboolean SV_Nav_NodeFailed(sharedEntity_t* ent, const int nodeID)
{
	return navigator.NodeFailed(ent, nodeID);
}

static qboolean SV_Nav_NodesAreNeighbors(const int startID, const int endID)
{
	return navigator.NodesAreNeighbors(startID, endID);
}

static void SV_Nav_ClearFailedEdge(failedEdge_t* failedEdge)
{
	navigator.ClearFailedEdge(failedEdge);
}

static void SV_Nav_ClearAllFailedEdges(void)
{
	navigator.ClearAllFailedEdges();
}

static int SV_Nav_EdgeFailed(const int startID, const int endID)
{
	return navigator.EdgeFailed(startID, endID);
}

static void SV_Nav_AddFailedEdge(const int entID, const int startID, const int endID)
{
	navigator.AddFailedEdge(entID, startID, endID);
}

static qboolean SV_Nav_CheckFailedEdge(failedEdge_t* failedEdge)
{
	return navigator.CheckFailedEdge(failedEdge);
}

static void SV_Nav_CheckAllFailedEdges(void)
{
	navigator.CheckAllFailedEdges();
}

static qboolean SV_Nav_RouteBlocked(const int startID, const int testEdgeID, const int endID, const int rejectRank)
{
	return navigator.RouteBlocked(startID, testEdgeID, endID, rejectRank);
}

static int SV_Nav_GetBestNodeAltRoute(const int startID, const int endID, int* pathCost, const int rejectID)
{
	return navigator.GetBestNodeAltRoute(startID, endID, pathCost, rejectID);
}

static int SV_Nav_GetBestNodeAltRoute2(const int startID, const int endID, const int rejectID)
{
	return navigator.GetBestNodeAltRoute(startID, endID, rejectID);
}

static int SV_Nav_GetBestPathBetweenEnts(sharedEntity_t* ent, sharedEntity_t* goal, const int flags)
{
	return navigator.GetBestPathBetweenEnts(ent, goal, flags);
}

static int SV_Nav_GetNodeRadius(const int nodeID)
{
	return navigator.GetNodeRadius(nodeID);
}

static void SV_Nav_CheckBlockedEdges(void)
{
	navigator.CheckBlockedEdges();
}

static void SV_Nav_ClearCheckedNodes(void)
{
	navigator.ClearCheckedNodes();
}

static int SV_Nav_CheckedNode(const int wayPoint, const int ent)
{
	return navigator.CheckedNode(wayPoint, ent);
}

static void SV_Nav_SetCheckedNode(const int wayPoint, const int ent, const int value)
{
	navigator.SetCheckedNode(wayPoint, ent, value);
}

static void SV_Nav_FlagAllNodes(const int newFlag)
{
	navigator.FlagAllNodes(newFlag);
}

static qboolean SV_Nav_GetPathsCalculated(void)
{
	return navigator.pathsCalculated;
}

static void SV_Nav_SetPathsCalculated(const qboolean newVal)
{
	navigator.pathsCalculated = newVal;
}

static int SV_BotLoadCharacter(char* charfile, const float skill)
{
	return botlib_export->ai.BotLoadCharacter(charfile, skill);
}

static void SV_BotFreeCharacter(const int character)
{
	botlib_export->ai.BotFreeCharacter(character);
}

static float SV_Characteristic_Float(const int character, const int index)
{
	return botlib_export->ai.Characteristic_Float(character, index);
}

static float SV_Characteristic_BFloat(const int character, const int index, const float min, const float max)
{
	return botlib_export->ai.Characteristic_BFloat(character, index, min, max);
}

static int SV_Characteristic_Integer(const int character, const int index)
{
	return botlib_export->ai.Characteristic_Integer(character, index);
}

static int SV_Characteristic_BInteger(const int character, const int index, const int min, const int max)
{
	return botlib_export->ai.Characteristic_BInteger(character, index, min, max);
}

static void SV_Characteristic_String(const int character, const int index, char* buf, const int size)
{
	botlib_export->ai.Characteristic_String(character, index, buf, size);
}

static int SV_BotAllocChatState(void)
{
	return botlib_export->ai.BotAllocChatState();
}

static void SV_BotFreeChatState(const int handle)
{
	botlib_export->ai.BotFreeChatState(handle);
}

static void SV_BotQueueConsoleMessage(const int chatstate, const int type, char* message)
{
	botlib_export->ai.BotQueueConsoleMessage(chatstate, type, message);
}

static void SV_BotRemoveConsoleMessage(const int chatstate, const int handle)
{
	botlib_export->ai.BotRemoveConsoleMessage(chatstate, handle);
}

static int SV_BotNextConsoleMessage(const int chatstate, void* cm)
{
	return botlib_export->ai.BotNextConsoleMessage(chatstate, static_cast<bot_consolemessage_s*>(cm));
}

static int SV_BotNumConsoleMessages(const int chatstate)
{
	return botlib_export->ai.BotNumConsoleMessages(chatstate);
}

static void SV_BotInitialChat(const int chatstate, char* type, const int mcontext, char* var0, char* var1, char* var2,
	char* var3, char* var4, char* var5, char* var6, char* var7)
{
	botlib_export->ai.BotInitialChat(chatstate, type, mcontext, var0, var1, var2, var3, var4, var5, var6, var7);
}

static int SV_BotReplyChat(const int chatstate, char* message, const int mcontext, const int vcontext, char* var0,
	char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7)
{
	return botlib_export->ai.BotReplyChat(chatstate, message, mcontext, vcontext, var0, var1, var2, var3, var4, var5,
		var6, var7);
}

static int SV_BotChatLength(const int chatstate)
{
	return botlib_export->ai.BotChatLength(chatstate);
}

static void SV_BotEnterChat(const int chatstate, const int client, const int sendto)
{
	botlib_export->ai.BotEnterChat(chatstate, client, sendto);
}

static int SV_StringContains(char* str1, char* str2, const int casesensitive)
{
	return botlib_export->ai.StringContains(str1, str2, casesensitive);
}

static int SV_BotFindMatch(char* str, void* match, const unsigned long int context)
{
	return botlib_export->ai.BotFindMatch(str, static_cast<bot_match_s*>(match), context);
}

static void SV_BotMatchVariable(void* match, const int variable, char* buf, const int size)
{
	botlib_export->ai.BotMatchVariable(static_cast<bot_match_s*>(match), variable, buf, size);
}

static void SV_UnifyWhiteSpaces(char* string)
{
	botlib_export->ai.UnifyWhiteSpaces(string);
}

static void SV_BotReplaceSynonyms(char* string, const unsigned long int context)
{
	botlib_export->ai.BotReplaceSynonyms(string, context);
}

static int SV_BotLoadChatFile(const int chatstate, char* chatfile, char* chatname)
{
	return botlib_export->ai.BotLoadChatFile(chatstate, chatfile, chatname);
}

static void SV_BotSetChatGender(const int chatstate, const int gender)
{
	botlib_export->ai.BotSetChatGender(chatstate, gender);
}

static void SV_BotSetChatName(const int chatstate, char* name, const int client)
{
	botlib_export->ai.BotSetChatName(chatstate, name, client);
}

static void SV_BotResetGoalState(const int goalstate)
{
	botlib_export->ai.BotResetGoalState(goalstate);
}

static void SV_BotResetAvoidGoals(const int goalstate)
{
	botlib_export->ai.BotResetAvoidGoals(goalstate);
}

static void SV_BotPushGoal(const int goalstate, void* goal)
{
	botlib_export->ai.BotPushGoal(goalstate, static_cast<bot_goal_s*>(goal));
}

static void SV_BotPopGoal(const int goalstate)
{
	botlib_export->ai.BotPopGoal(goalstate);
}

static void SV_BotEmptyGoalStack(const int goalstate)
{
	botlib_export->ai.BotEmptyGoalStack(goalstate);
}

static void SV_BotDumpAvoidGoals(const int goalstate)
{
	botlib_export->ai.BotDumpAvoidGoals(goalstate);
}

static void SV_BotDumpGoalStack(const int goalstate)
{
	botlib_export->ai.BotDumpGoalStack(goalstate);
}

static void SV_BotGoalName(const int number, char* name, const int size)
{
	botlib_export->ai.BotGoalName(number, name, size);
}

static int SV_BotGetTopGoal(const int goalstate, void* goal)
{
	return botlib_export->ai.BotGetTopGoal(goalstate, static_cast<bot_goal_s*>(goal));
}

static int SV_BotGetSecondGoal(const int goalstate, void* goal)
{
	return botlib_export->ai.BotGetSecondGoal(goalstate, static_cast<bot_goal_s*>(goal));
}

static int SV_BotChooseLTGItem(const int goalstate, vec3_t origin, int* inventory, const int travelflags)
{
	return botlib_export->ai.BotChooseLTGItem(goalstate, origin, inventory, travelflags);
}

static int SV_BotChooseNBGItem(const int goalstate, vec3_t origin, int* inventory, const int travelflags, void* ltg,
	const float maxtime)
{
	return botlib_export->ai.BotChooseNBGItem(goalstate, origin, inventory, travelflags, static_cast<bot_goal_s*>(ltg),
		maxtime);
}

static int SV_BotTouchingGoal(vec3_t origin, void* goal)
{
	return botlib_export->ai.BotTouchingGoal(origin, static_cast<bot_goal_s*>(goal));
}

static int SV_BotItemGoalInVisButNotVisible(const int viewer, vec3_t eye, vec3_t viewangles, void* goal)
{
	return botlib_export->ai.BotItemGoalInVisButNotVisible(viewer, eye, viewangles, static_cast<bot_goal_s*>(goal));
}

static int SV_BotGetLevelItemGoal(const int index, char* classname, void* goal)
{
	return botlib_export->ai.BotGetLevelItemGoal(index, classname, static_cast<bot_goal_s*>(goal));
}

static float SV_BotAvoidGoalTime(const int goalstate, const int number)
{
	return botlib_export->ai.BotAvoidGoalTime(goalstate, number);
}

static void SV_BotInitLevelItems(void)
{
	botlib_export->ai.BotInitLevelItems();
}

static void SV_BotUpdateEntityItems(void)
{
	botlib_export->ai.BotUpdateEntityItems();
}

static int SV_BotLoadItemWeights(const int goalstate, char* filename)
{
	return botlib_export->ai.BotLoadItemWeights(goalstate, filename);
}

static void SV_BotFreeItemWeights(const int goalstate)
{
	botlib_export->ai.BotFreeItemWeights(goalstate);
}

static void SV_BotSaveGoalFuzzyLogic(const int goalstate, char* filename)
{
	botlib_export->ai.BotSaveGoalFuzzyLogic(goalstate, filename);
}

static int SV_BotAllocGoalState(const int state)
{
	return botlib_export->ai.BotAllocGoalState(state);
}

static void SV_BotFreeGoalState(const int handle)
{
	botlib_export->ai.BotFreeGoalState(handle);
}

static void SV_BotResetMoveState(const int movestate)
{
	botlib_export->ai.BotResetMoveState(movestate);
}

static void SV_BotMoveToGoal(void* result, const int movestate, void* goal, const int travelflags)
{
	botlib_export->ai.BotMoveToGoal(static_cast<bot_moveresult_s*>(result), movestate, static_cast<bot_goal_s*>(goal),
		travelflags);
}

static int SV_BotMoveInDirection(const int movestate, vec3_t dir, const float speed, const int type)
{
	return botlib_export->ai.BotMoveInDirection(movestate, dir, speed, type);
}

static void SV_BotResetAvoidReach(const int movestate)
{
	botlib_export->ai.BotResetAvoidReach(movestate);
}

static void SV_BotResetLastAvoidReach(const int movestate)
{
	botlib_export->ai.BotResetLastAvoidReach(movestate);
}

static int SV_BotReachabilityArea(vec3_t origin, const int testground)
{
	return botlib_export->ai.BotReachabilityArea(origin, testground);
}

static int SV_BotMovementViewTarget(const int movestate, void* goal, const int travelflags, const float lookahead,
	vec3_t target)
{
	return botlib_export->ai.BotMovementViewTarget(movestate, static_cast<bot_goal_s*>(goal), travelflags, lookahead,
		target);
}

static int SV_BotAllocMoveState(void)
{
	return botlib_export->ai.BotAllocMoveState();
}

static void SV_BotFreeMoveState(const int handle)
{
	botlib_export->ai.BotFreeMoveState(handle);
}

static void SV_BotInitMoveState(const int handle, void* initmove)
{
	botlib_export->ai.BotInitMoveState(handle, static_cast<bot_initmove_s*>(initmove));
}

static int SV_BotChooseBestFightWeapon(const int weaponstate, int* inventory)
{
	return botlib_export->ai.BotChooseBestFightWeapon(weaponstate, inventory);
}

static void SV_BotGetWeaponInfo(const int weaponstate, const int weapon, void* weaponinfo)
{
	botlib_export->ai.BotGetWeaponInfo(weaponstate, weapon, static_cast<weaponinfo_s*>(weaponinfo));
}

static int SV_BotLoadWeaponWeights(const int weaponstate, char* filename)
{
	return botlib_export->ai.BotLoadWeaponWeights(weaponstate, filename);
}

static int SV_BotAllocWeaponState(void)
{
	return botlib_export->ai.BotAllocWeaponState();
}

static void SV_BotFreeWeaponState(const int weaponstate)
{
	botlib_export->ai.BotFreeWeaponState(weaponstate);
}

static void SV_BotResetWeaponState(const int weaponstate)
{
	botlib_export->ai.BotResetWeaponState(weaponstate);
}

static int SV_GeneticParentsAndChildSelection(const int numranks, float* ranks, int* parent1, int* parent2, int* child)
{
	return botlib_export->ai.GeneticParentsAndChildSelection(numranks, ranks, parent1, parent2, child);
}

static void SV_BotInterbreedGoalFuzzyLogic(const int parent1, const int parent2, const int child)
{
	botlib_export->ai.BotInterbreedGoalFuzzyLogic(parent1, parent2, child);
}

static void SV_BotMutateGoalFuzzyLogic(const int goalstate, const float range)
{
	botlib_export->ai.BotMutateGoalFuzzyLogic(goalstate, range);
}

static int SV_BotGetNextCampSpotGoal(const int num, void* goal)
{
	return botlib_export->ai.BotGetNextCampSpotGoal(num, static_cast<bot_goal_s*>(goal));
}

static int SV_BotGetMapLocationGoal(char* name, void* goal)
{
	return botlib_export->ai.BotGetMapLocationGoal(name, static_cast<bot_goal_s*>(goal));
}

static int SV_BotNumInitialChats(const int chatstate, char* type)
{
	return botlib_export->ai.BotNumInitialChats(chatstate, type);
}

static void SV_BotGetChatMessage(const int chatstate, char* buf, const int size)
{
	botlib_export->ai.BotGetChatMessage(chatstate, buf, size);
}

static void SV_BotRemoveFromAvoidGoals(const int goalstate, const int number)
{
	botlib_export->ai.BotRemoveFromAvoidGoals(goalstate, number);
}

static int SV_BotPredictVisiblePosition(vec3_t origin, const int areanum, void* goal, const int travelflags,
	vec3_t target)
{
	return botlib_export->ai.BotPredictVisiblePosition(origin, areanum, static_cast<bot_goal_s*>(goal), travelflags,
		target);
}

static void SV_BotSetAvoidGoalTime(const int goalstate, const int number, const float avoidtime)
{
	botlib_export->ai.BotSetAvoidGoalTime(goalstate, number, avoidtime);
}

static void SV_BotAddAvoidSpot(const int movestate, vec3_t origin, const float radius, const int type)
{
	botlib_export->ai.BotAddAvoidSpot(movestate, origin, radius, type);
}

static int SV_BotLibSetup(void)
{
	return botlib_export->BotLibSetup();
}

static int SV_BotLibShutdown(void)
{
	return botlib_export->BotLibShutdown();
}

static int SV_BotLibVarSet(char* var_name, char* value)
{
	return botlib_export->BotLibVarSet(var_name, value);
}

static int SV_BotLibVarGet(char* var_name, char* value, const int size)
{
	return botlib_export->BotLibVarGet(var_name, value, size);
}

static int SV_BotLibDefine(char* string)
{
	return botlib_export->PC_AddGlobalDefine(string);
}

static int SV_BotLibStartFrame(const float time)
{
	return botlib_export->BotLibStartFrame(time);
}

static int SV_BotLibLoadMap(const char* mapname)
{
	return botlib_export->BotLibLoadMap(mapname);
}

static int SV_BotLibUpdateEntity(const int ent, void* bue)
{
	return botlib_export->BotLibUpdateEntity(ent, static_cast<bot_entitystate_t*>(bue));
}

static int SV_BotLibTest(const int parm0, char* parm1, vec3_t parm2, vec3_t parm3)
{
	return botlib_export->Test(parm0, parm1, parm2, parm3);
}

static int SV_BotGetServerCommand(const int clientNum, char* message, const int size)
{
	return SV_BotGetConsoleMessage(clientNum, message, size);
}

static void SV_BotUserCommand(const int clientNum, usercmd_t* ucmd)
{
	SV_ClientThink(&svs.clients[clientNum], ucmd);
}

static int SV_AAS_EnableRoutingArea(const int areanum, const int enable)
{
	return botlib_export->aas.AAS_EnableRoutingArea(areanum, enable);
}

static int SV_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int* areas, const int maxareas)
{
	return botlib_export->aas.AAS_BBoxAreas(absmins, absmaxs, areas, maxareas);
}

static int SV_AAS_AreaInfo(const int areanum, void* info)
{
	return botlib_export->aas.AAS_AreaInfo(areanum, static_cast<aas_areainfo_s*>(info));
}

static void SV_AAS_EntityInfo(const int entnum, void* info)
{
	botlib_export->aas.AAS_EntityInfo(entnum, static_cast<aas_entityinfo_s*>(info));
}

static int SV_AAS_Initialized(void)
{
	return botlib_export->aas.AAS_Initialized();
}

static void SV_AAS_PresenceTypeBoundingBox(const int presencetype, vec3_t mins, vec3_t maxs)
{
	botlib_export->aas.AAS_PresenceTypeBoundingBox(presencetype, mins, maxs);
}

static float SV_AAS_Time(void)
{
	return botlib_export->aas.AAS_Time();
}

static int SV_AAS_PointAreaNum(vec3_t point)
{
	return botlib_export->aas.AAS_PointAreaNum(point);
}

static int SV_AAS_TraceAreas(vec3_t start, vec3_t end, int* areas, vec3_t* points, const int maxareas)
{
	return botlib_export->aas.AAS_TraceAreas(start, end, areas, points, maxareas);
}

static int SV_AAS_PointContents(vec3_t point)
{
	return botlib_export->aas.AAS_PointContents(point);
}

static int SV_AAS_NextBSPEntity(const int ent)
{
	return botlib_export->aas.AAS_NextBSPEntity(ent);
}

static int SV_AAS_ValueForBSPEpairKey(const int ent, char* key, char* value, const int size)
{
	return botlib_export->aas.AAS_ValueForBSPEpairKey(ent, key, value, size);
}

static int SV_AAS_VectorForBSPEpairKey(const int ent, char* key, vec3_t v)
{
	return botlib_export->aas.AAS_VectorForBSPEpairKey(ent, key, v);
}

static int SV_AAS_FloatForBSPEpairKey(const int ent, char* key, float* value)
{
	return botlib_export->aas.AAS_FloatForBSPEpairKey(ent, key, value);
}

static int SV_AAS_IntForBSPEpairKey(const int ent, char* key, int* value)
{
	return botlib_export->aas.AAS_IntForBSPEpairKey(ent, key, value);
}

static int SV_AAS_AreaReachability(const int areanum)
{
	return botlib_export->aas.AAS_AreaReachability(areanum);
}

static int SV_AAS_AreaTravelTimeToGoalArea(const int areanum, vec3_t origin, const int goalareanum,
	const int travelflags)
{
	return botlib_export->aas.AAS_AreaTravelTimeToGoalArea(areanum, origin, goalareanum, travelflags);
}

static int SV_AAS_Swimming(vec3_t origin)
{
	return botlib_export->aas.AAS_Swimming(origin);
}

static int SV_AAS_PredictClientMovement(void* move, const int entnum, vec3_t origin, const int presencetype,
	const int onground, vec3_t velocity, vec3_t cmdmove, const int cmdframes,
	const int maxframes, const float frametime, const int stopevent,
	const int stopareanum, const int visualize)
{
	return botlib_export->aas.AAS_PredictClientMovement(static_cast<aas_clientmove_s*>(move), entnum, origin,
		presencetype, onground, velocity, cmdmove, cmdframes, maxframes,
		frametime, stopevent, stopareanum, visualize);
}

static int SV_AAS_AlternativeRouteGoals(vec3_t start, const int startareanum, vec3_t goal, const int goalareanum,
	const int travelflags, void* altroutegoals, const int maxaltroutegoals,
	const int type)
{
	return botlib_export->aas.AAS_AlternativeRouteGoals(start, startareanum, goal, goalareanum, travelflags,
		static_cast<aas_altroutegoal_s*>(altroutegoals),
		maxaltroutegoals, type);
}

static int SV_AAS_PredictRoute(void* route, const int areanum, vec3_t origin, const int goalareanum,
	const int travelflags, const int maxareas, const int maxtime, const int stopevent,
	const int stopcontents, const int stoptfl, const int stopareanum)
{
	return botlib_export->aas.AAS_PredictRoute(static_cast<aas_predictroute_s*>(route), areanum, origin, goalareanum,
		travelflags, maxareas, maxtime, stopevent, stopcontents, stoptfl,
		stopareanum);
}

static int SV_AAS_PointReachabilityAreaIndex(vec3_t point)
{
	return botlib_export->aas.AAS_PointReachabilityAreaIndex(point);
}

static void SV_EA_Say(const int client, char* str)
{
	botlib_export->ea.EA_Say(client, str);
}

static void SV_EA_SayTeam(const int client, char* str)
{
	botlib_export->ea.EA_SayTeam(client, str);
}

static void SV_EA_Command(const int client, char* command)
{
	botlib_export->ea.EA_Command(client, command);
}

static void SV_EA_Action(const int client, const int action)
{
	botlib_export->ea.EA_Action(client, action);
}

static void SV_EA_Gesture(const int client)
{
	botlib_export->ea.EA_Gesture(client);
}

static void SV_EA_Talk(const int client)
{
	botlib_export->ea.EA_Talk(client);
}

static void SV_EA_Attack(const int client)
{
	botlib_export->ea.EA_Attack(client);
}

static void SV_EA_Alt_Attack(const int client)
{
	botlib_export->ea.EA_Alt_Attack(client);
}

static void SV_EA_ForcePower(const int client)
{
	botlib_export->ea.EA_ForcePower(client);
}

static void SV_EA_Use(const int client)
{
	botlib_export->ea.EA_Use(client);
}

static void SV_EA_Respawn(const int client)
{
	botlib_export->ea.EA_Respawn(client);
}

static void SV_EA_Crouch(const int client)
{
	botlib_export->ea.EA_Crouch(client);
}

static void SV_EA_MoveUp(const int client)
{
	botlib_export->ea.EA_MoveUp(client);
}

static void SV_EA_MoveDown(const int client)
{
	botlib_export->ea.EA_MoveDown(client);
}

static void SV_EA_MoveForward(const int client)
{
	botlib_export->ea.EA_MoveForward(client);
}

static void SV_EA_MoveBack(const int client)
{
	botlib_export->ea.EA_MoveBack(client);
}

static void SV_EA_MoveLeft(const int client)
{
	botlib_export->ea.EA_MoveLeft(client);
}

static void SV_EA_MoveRight(const int client)
{
	botlib_export->ea.EA_MoveRight(client);
}

static void SV_EA_SelectWeapon(const int client, const int weapon)
{
	botlib_export->ea.EA_SelectWeapon(client, weapon);
}

static void SV_EA_Jump(const int client)
{
	botlib_export->ea.EA_Jump(client);
}

static void SV_EA_DelayedJump(const int client)
{
	botlib_export->ea.EA_DelayedJump(client);
}

static void SV_EA_Move(const int client, vec3_t dir, const float speed)
{
	botlib_export->ea.EA_Move(client, dir, speed);
}

static void SV_EA_View(const int client, vec3_t viewangles)
{
	botlib_export->ea.EA_View(client, viewangles);
}

static void SV_EA_EndRegular(const int client, const float thinktime)
{
	botlib_export->ea.EA_EndRegular(client, thinktime);
}

static void SV_EA_GetInput(const int client, const float thinktime, void* input)
{
	botlib_export->ea.EA_GetInput(client, thinktime, static_cast<bot_input_t*>(input));
}

static void SV_EA_ResetInput(const int client)
{
	botlib_export->ea.EA_ResetInput(client);
}

static int SV_PC_LoadSource(const char* filename)
{
	return botlib_export->PC_LoadSourceHandle(filename);
}

static int SV_PC_FreeSource(const int handle)
{
	return botlib_export->PC_FreeSourceHandle(handle);
}

static int SV_PC_ReadToken(const int handle, pc_token_t* pc_token)
{
	return botlib_export->PC_ReadTokenHandle(handle, pc_token);
}

static int SV_PC_SourceFileAndLine(const int handle, char* filename, int* line)
{
	return botlib_export->PC_SourceFileAndLine(handle, filename, line);
}

static qhandle_t SV_RE_RegisterSkin(const char* name)
{
	return re->RegisterServerSkin(name);
}

static int SV_CM_RegisterTerrain(const char* config)
{
	return 0;
}

static void SV_RMG_Init(void)
{
}

static void SV_G2API_ListModelSurfaces(void* ghlInfo)
{
	re->G2API_ListSurfaces(static_cast<CGhoul2Info*>(ghlInfo));
}

static void SV_G2API_ListModelBones(void* ghlInfo, const int frame)
{
	re->G2API_ListBones(static_cast<CGhoul2Info*>(ghlInfo), frame);
}

static void SV_G2API_SetGhoul2model_indexes(void* ghoul2, qhandle_t* modelList, qhandle_t* skinList)
{
	re->G2API_SetGhoul2model_indexes(*static_cast<CGhoul2Info_v*>(ghoul2), modelList, skinList);
}

static qboolean SV_G2API_HaveWeGhoul2Models(void* ghoul2)
{
	if (!ghoul2) return qfalse;
	return re->G2API_HaveWeGhoul2Models(*((CGhoul2Info_v*)ghoul2));
}

static qboolean SV_G2API_GetBoltMatrix(void* ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t* matrix,
	const vec3_t angles, const vec3_t position, const int frameNum,
	qhandle_t* modelList, vec3_t scale)
{
	return re->G2API_GetBoltMatrix(*static_cast<CGhoul2Info_v*>(ghoul2), modelIndex, boltIndex, matrix, angles,
		position, frameNum, modelList, scale);
}

static qboolean SV_G2API_GetBoltMatrix_NoReconstruct(void* ghoul2, const int modelIndex, const int boltIndex,
	mdxaBone_t* matrix, const vec3_t angles, const vec3_t position,
	const int frameNum, qhandle_t* modelList, vec3_t scale)
{
	re->G2API_BoltMatrixReconstruction(qfalse);
	return re->G2API_GetBoltMatrix(*static_cast<CGhoul2Info_v*>(ghoul2), modelIndex, boltIndex, matrix, angles,
		position, frameNum, modelList, scale);
}

static qboolean SV_G2API_GetBoltMatrix_NoRecNoRot(void* ghoul2, const int modelIndex, const int boltIndex,
	mdxaBone_t* matrix, const vec3_t angles, const vec3_t position,
	const int frameNum, qhandle_t* modelList, vec3_t scale)
{
	re->G2API_BoltMatrixReconstruction(qfalse);
	re->G2API_BoltMatrixSPMethod(qtrue);
	return re->G2API_GetBoltMatrix(*static_cast<CGhoul2Info_v*>(ghoul2), modelIndex, boltIndex, matrix, angles,
		position, frameNum, modelList, scale);
}

static int SV_G2API_InitGhoul2Model(void** ghoul2Ptr, const char* fileName, const int modelIndex,
	const qhandle_t customSkin, const qhandle_t customShader, const int modelFlags,
	const int lodBias)
{
#ifdef _FULL_G2_LEAK_CHECKING
	g_G2AllocServer = 1;
#endif
	return re->G2API_InitGhoul2Model(reinterpret_cast<CGhoul2Info_v**>(ghoul2Ptr), fileName, modelIndex, customSkin, customShader,
		modelFlags, lodBias);
}

static qboolean SV_G2API_SetSkin(void* ghoul2, const int modelIndex, const qhandle_t customSkin,
	const qhandle_t renderSkin)
{
	CGhoul2Info_v& g2 = *static_cast<CGhoul2Info_v*>(ghoul2);
	return re->G2API_SetSkin(g2, modelIndex, customSkin, renderSkin);
}

static void SV_G2API_CollisionDetect(CollisionRecord_t* collRecMap, void* ghoul2, const vec3_t angles,
	const vec3_t position, const int frameNumber, const int entNum, vec3_t rayStart,
	vec3_t rayEnd, vec3_t scale, const int traceFlags, const int useLod,
	const float fRadius)
{
	re->G2API_CollisionDetect(collRecMap, *static_cast<CGhoul2Info_v*>(ghoul2), angles, position, frameNumber, entNum,
		rayStart, rayEnd, scale, G2VertSpaceServer, traceFlags, useLod, fRadius);
}

static void SV_G2API_CollisionDetectCache(CollisionRecord_t* collRecMap, void* ghoul2, const vec3_t angles,
	const vec3_t position, const int frameNumber, const int entNum,
	vec3_t rayStart, vec3_t rayEnd, vec3_t scale, const int traceFlags,
	const int useLod, const float fRadius)
{
	re->G2API_CollisionDetectCache(collRecMap, *static_cast<CGhoul2Info_v*>(ghoul2), angles, position, frameNumber,
		entNum, rayStart, rayEnd, scale, G2VertSpaceServer, traceFlags, useLod, fRadius);
}

static void SV_G2API_CleanGhoul2Models(void** ghoul2Ptr)
{
#ifdef _FULL_G2_LEAK_CHECKING
	g_G2AllocServer = 1;
#endif
	re->G2API_CleanGhoul2Models(reinterpret_cast<CGhoul2Info_v**>(ghoul2Ptr));
}

static qboolean SV_G2API_SetBoneAngles(void* ghoul2, const int modelIndex, const char* boneName, const vec3_t angles,
	const int flags, const int up, const int right, const int forward,
	qhandle_t* modelList, const int blendTime, const int currentTime)
{
	return re->G2API_SetBoneAngles(*static_cast<CGhoul2Info_v*>(ghoul2), modelIndex, boneName, angles, flags,
		static_cast<const Eorientations>(up), static_cast<const Eorientations>(right),
		static_cast<const Eorientations>(forward), modelList, blendTime, currentTime);
}

static qboolean SV_G2API_SetBoneAnim(void* ghoul2, const int modelIndex, const char* boneName, const int startFrame,
	const int endFrame, const int flags, const float animSpeed, const int currentTime,
	const float setFrame, const int blendTime)
{
	return re->G2API_SetBoneAnim(*static_cast<CGhoul2Info_v*>(ghoul2), modelIndex, boneName, startFrame, endFrame,
		flags, animSpeed, currentTime, setFrame, blendTime);
}

static qboolean SV_G2API_GetBoneAnim(void* ghoul2, const char* boneName, const int currentTime, float* currentFrame,
	int* startFrame, int* endFrame, int* flags, float* animSpeed, int* modelList,
	const int modelIndex)
{
	CGhoul2Info_v& g2 = *static_cast<CGhoul2Info_v*>(ghoul2);
	return re->G2API_GetBoneAnim(g2, modelIndex, boneName, currentTime, currentFrame, startFrame, endFrame, flags,animSpeed, modelList);
}

//static void SV_G2API_GetGLAName(void* ghoul2, const int modelIndex, char* fillBuf)
//{
//	assert(ghoul2 && "invalid g2 handle");
//
//	const char* tmp = re->G2API_GetGLAName(*static_cast<CGhoul2Info_v*>(ghoul2), modelIndex);
//	if (tmp)
//	{
//		strcpy(fillBuf, tmp);
//	}
//}

// This fucker has been grinding my gears for days because ghoul2 can be null here.
static void SV_G2API_GetGLAName(void* ghoul2, const int modelIndex, char* fillBuf)
{
	if (!ghoul2)
	{
		return;
	}

	CGhoul2Info_v* g2 = static_cast<CGhoul2Info_v*>(ghoul2);

	if (!g2) 
	{
		return;
	}
	const char* tmp = re->G2API_GetGLAName(*static_cast<CGhoul2Info_v*>(ghoul2), modelIndex);

	if (tmp)
	{
		strcpy(fillBuf, tmp);
	}
}

static int SV_G2API_CopyGhoul2Instance(void* g2_from, void* g2_to, const int modelIndex)
{
	return re->G2API_CopyGhoul2Instance(*static_cast<CGhoul2Info_v*>(g2_from), *static_cast<CGhoul2Info_v*>(g2_to),
		modelIndex);
}

static void SV_G2API_CopySpecificGhoul2Model(void* g2_from, const int modelFrom, void* g2_to, const int modelTo)
{
	if (!g2_from || !g2_to)
	{
		return;
	}
	re->G2API_CopySpecificG2Model(*static_cast<CGhoul2Info_v*>(g2_from), modelFrom, *static_cast<CGhoul2Info_v*>(g2_to),
		modelTo);
}

static void SV_G2API_DuplicateGhoul2Instance(void* g2_from, void** g2_to)
{
#ifdef _FULL_G2_LEAK_CHECKING
	g_G2AllocServer = 1;
#endif
	re->G2API_DuplicateGhoul2Instance(*static_cast<CGhoul2Info_v*>(g2_from), reinterpret_cast<CGhoul2Info_v**>(g2_to));
}

static qboolean SV_G2API_HasGhoul2ModelOnIndex(void* ghlInfo, const int modelIndex)
{
	return re->G2API_HasGhoul2ModelOnIndex(static_cast<CGhoul2Info_v**>(ghlInfo), modelIndex);
}

static qboolean SV_G2API_RemoveGhoul2Model(void* ghlInfo, const int modelIndex)
{
#ifdef _FULL_G2_LEAK_CHECKING
	g_G2AllocServer = 1;
#endif
	return re->G2API_RemoveGhoul2Model(static_cast<CGhoul2Info_v**>(ghlInfo), modelIndex);
}

static qboolean SV_G2API_RemoveGhoul2Models(void* ghlInfo)
{
#ifdef _FULL_G2_LEAK_CHECKING
	g_G2AllocServer = 1;
#endif
	return re->G2API_RemoveGhoul2Models(static_cast<CGhoul2Info_v**>(ghlInfo));
}

static int SV_G2API_Ghoul2Size(void* ghlInfo)
{
	return re->G2API_Ghoul2Size(*static_cast<CGhoul2Info_v*>(ghlInfo));
}

static int SV_G2API_AddBolt(void* ghoul2, const int modelIndex, const char* boneName)
{
	return re->G2API_AddBolt(*static_cast<CGhoul2Info_v*>(ghoul2), modelIndex, boneName);
}

static void SV_G2API_SetBoltInfo(void* ghoul2, const int modelIndex, const int boltInfo)
{
	re->G2API_SetBoltInfo(*static_cast<CGhoul2Info_v*>(ghoul2), modelIndex, boltInfo);
}

static qboolean SV_G2API_SetRootSurface(void* ghoul2, const int modelIndex, const char* surfaceName)
{
	return re->G2API_SetRootSurface(*static_cast<CGhoul2Info_v*>(ghoul2), modelIndex, surfaceName);
}

static qboolean SV_G2API_SetSurfaceOnOff(void* ghoul2, const char* surfaceName, const int flags)
{
	return re->G2API_SetSurfaceOnOff(*static_cast<CGhoul2Info_v*>(ghoul2), surfaceName, flags);
}

static qboolean SV_G2API_SetNewOrigin(void* ghoul2, const int boltIndex)
{
	return re->G2API_SetNewOrigin(*static_cast<CGhoul2Info_v*>(ghoul2), boltIndex);
}

static qboolean SV_G2API_DoesBoneExist(void* ghoul2, const int modelIndex, const char* boneName)
{
	CGhoul2Info_v& g2 = *static_cast<CGhoul2Info_v*>(ghoul2);
	return re->G2API_DoesBoneExist(g2, modelIndex, boneName);
}

static int SV_G2API_GetSurfaceRenderStatus(void* ghoul2, const int modelIndex, const char* surfaceName)
{
	CGhoul2Info_v& g2 = *static_cast<CGhoul2Info_v*>(ghoul2);
	return re->G2API_GetSurfaceRenderStatus(g2, modelIndex, surfaceName);
}

static void SV_G2API_AbsurdSmoothing(void* ghoul2, const qboolean status)
{
	CGhoul2Info_v& g2 = *static_cast<CGhoul2Info_v*>(ghoul2);
	re->G2API_AbsurdSmoothing(g2, status);
}

static void SV_G2API_SetRagDoll(void* ghoul2, sharedRagDollParams_t* params)
{
	CRagDollParams rdParams{};

	if (!params)
	{
		re->G2API_ResetRagDoll(*static_cast<CGhoul2Info_v*>(ghoul2));
		return;
	}

	VectorCopy(params->angles, rdParams.angles);
	VectorCopy(params->position, rdParams.position);
	VectorCopy(params->scale, rdParams.scale);
	VectorCopy(params->pelvisAnglesOffset, rdParams.pelvisAnglesOffset);
	VectorCopy(params->pelvisPositionOffset, rdParams.pelvisPositionOffset);

	rdParams.fImpactStrength = params->fImpactStrength;
	rdParams.fShotStrength = params->fShotStrength;
	rdParams.me = params->me;

	rdParams.startFrame = params->startFrame;
	rdParams.endFrame = params->endFrame;

	rdParams.collisionType = params->collisionType;
	rdParams.CallRagDollBegin = params->CallRagDollBegin;

	rdParams.RagPhase = static_cast<CRagDollParams::ERagPhase>(params->RagPhase);
	rdParams.effectorsToTurnOff = static_cast<CRagDollParams::ERagEffector>(params->effectorsToTurnOff);

	re->G2API_SetRagDoll(*static_cast<CGhoul2Info_v*>(ghoul2), &rdParams);
}

static void SV_G2API_AnimateG2Models(void* ghoul2, const int time, sharedRagDollUpdateParams_t* params)
{
	CRagDollUpdateParams rduParams;

	if (!params)
		return;

	VectorCopy(params->angles, rduParams.angles);
	VectorCopy(params->position, rduParams.position);
	VectorCopy(params->scale, rduParams.scale);
	VectorCopy(params->velocity, rduParams.velocity);

	rduParams.me = params->me;
	rduParams.settleFrame = params->settleFrame;

	re->G2API_AnimateG2ModelsRag(*static_cast<CGhoul2Info_v*>(ghoul2), time, &rduParams);
}

static qboolean SV_G2API_RagPCJConstraint(void* ghoul2, const char* boneName, vec3_t min, vec3_t max)
{
	return re->G2API_RagPCJConstraint(*static_cast<CGhoul2Info_v*>(ghoul2), boneName, min, max);
}

static qboolean SV_G2API_RagPCJGradientSpeed(void* ghoul2, const char* boneName, const float speed)
{
	return re->G2API_RagPCJGradientSpeed(*static_cast<CGhoul2Info_v*>(ghoul2), boneName, speed);
}

static qboolean SV_G2API_RagEffectorGoal(void* ghoul2, const char* boneName, vec3_t pos)
{
	return re->G2API_RagEffectorGoal(*static_cast<CGhoul2Info_v*>(ghoul2), boneName, pos);
}

static qboolean SV_G2API_GetRagBonePos(void* ghoul2, const char* boneName, vec3_t pos, vec3_t entAngles, vec3_t entPos,
	vec3_t entScale)
{
	return re->G2API_GetRagBonePos(*static_cast<CGhoul2Info_v*>(ghoul2), boneName, pos, entAngles, entPos, entScale);
}

static qboolean SV_G2API_RagEffectorKick(void* ghoul2, const char* boneName, vec3_t velocity)
{
	return re->G2API_RagEffectorKick(*static_cast<CGhoul2Info_v*>(ghoul2), boneName, velocity);
}

static qboolean SV_G2API_RagForceSolve(void* ghoul2, const qboolean force)
{
	return re->G2API_RagForceSolve(*static_cast<CGhoul2Info_v*>(ghoul2), force);
}

static qboolean SV_G2API_SetBoneIKState(void* ghoul2, const int time, const char* boneName, const int ikState,
	sharedSetBoneIKStateParams_t* params)
{
	return re->G2API_SetBoneIKState(*static_cast<CGhoul2Info_v*>(ghoul2), time, boneName, ikState, params);
}

static qboolean SV_G2API_IKMove(void* ghoul2, const int time, sharedIKMoveParams_t* params)
{
	return re->G2API_IKMove(*static_cast<CGhoul2Info_v*>(ghoul2), time, params);
}

static qboolean SV_G2API_RemoveBone(void* ghoul2, const char* boneName, const int modelIndex)
{
	CGhoul2Info_v& g2 = *static_cast<CGhoul2Info_v*>(ghoul2);
	return re->G2API_RemoveBone(g2, modelIndex, boneName);
}

static void SV_G2API_AttachInstanceToEntNum(void* ghoul2, const int entityNum, const qboolean server)
{
	re->G2API_AttachInstanceToEntNum(*static_cast<CGhoul2Info_v*>(ghoul2), entityNum, server);
}

static void SV_G2API_ClearAttachedInstance(const int entityNum)
{
	re->G2API_ClearAttachedInstance(entityNum);
}

static void SV_G2API_CleanEntAttachments(void)
{
	re->G2API_CleanEntAttachments();
}

static qboolean SV_G2API_OverrideServer(void* serverInstance)
{
	CGhoul2Info_v& g2 = *static_cast<CGhoul2Info_v*>(serverInstance);
	return re->G2API_OverrideServerWithClientData(g2, 0);
}

static void SV_G2API_GetSurfaceName(void* ghoul2, const int surfNumber, const int modelIndex, char* fillBuf)
{
	CGhoul2Info_v& g2 = *static_cast<CGhoul2Info_v*>(ghoul2);
	const char* tmp = re->G2API_GetSurfaceName(g2, modelIndex, surfNumber);
	strcpy(fillBuf, tmp);
}

static void GVM_Cvar_Set(const char* var_name, const char* value)
{
	Cvar_VM_Set(var_name, value, VM_GAME);
}

static qboolean CL_SEP_GetStringTextString(const char* text, char* buffer, const int bufferLength)
{
	assert(text && buffer);

	const char* str = SE_GetString(text);

	if (str[0])
	{
		Q_strncpyz(buffer, str, bufferLength);
		return qtrue;
	}

	Com_sprintf(buffer, bufferLength, "??%s", str);
	return qfalse;
}

// legacy syscall

intptr_t SV_GameSystemCalls(intptr_t* args)
{
	switch (args[0])
	{
		//rww - alright, DO NOT EVER add a game/cgame/ui generic call without adding a trap to match, and
		//all of these traps must be shared and have cases in sv_game, cl_cgame, and cl_ui. They must also
		//all be in the same order, and start at 100.
	case TRAP_MEMSET:
		Com_Memset(VMA(1), args[2], args[3]);
		return 0;

	case TRAP_MEMCPY:
		Com_Memcpy(VMA(1), VMA(2), args[3]);
		return 0;

	case TRAP_STRNCPY:
		strncpy(static_cast<char*>(VMA(1)), static_cast<const char*>(VMA(2)), args[3]);
		return args[1];

	case TRAP_SIN:
		return FloatAsInt(sin(VMF(1)));

	case TRAP_COS:
		return FloatAsInt(cos(VMF(1)));

	case TRAP_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));

	case TRAP_SQRT:
		return FloatAsInt(sqrt(VMF(1)));

	case TRAP_MATRIXMULTIPLY:
		MatrixMultiply(static_cast<vec3_t*>(VMA(1)), static_cast<vec3_t*>(VMA(2)), static_cast<vec3_t*>(VMA(3)));
		return 0;

	case TRAP_ANGLEVECTORS:
		AngleVectors(static_cast<const float*>(VMA(1)), static_cast<float*>(VMA(2)), static_cast<float*>(VMA(3)),
			static_cast<float*>(VMA(4)));
		return 0;

	case TRAP_PERPENDICULARVECTOR:
		PerpendicularVector(static_cast<float*>(VMA(1)), static_cast<const float*>(VMA(2)));
		return 0;

	case TRAP_FLOOR:
		return FloatAsInt(floor(VMF(1)));

	case TRAP_CEIL:
		return FloatAsInt(ceil(VMF(1)));

	case TRAP_TESTPRINTINT:
		return 0;

	case TRAP_TESTPRINTFLOAT:
		return 0;

	case TRAP_ACOS:
		return FloatAsInt(Q_acos(VMF(1)));

	case TRAP_ASIN:
		return FloatAsInt(Q_asin(VMF(1)));

	case G_PRINT:
		Com_Printf("%s", VMA(1));
		return 0;

	case G_ERROR:
		Com_Error(ERR_DROP, "%s", VMA(1));

	case G_MILLISECONDS:
		return Sys_Milliseconds();

	case G_PRECISIONTIMER_START:
		SV_PrecisionTimerStart(static_cast<void**>(VMA(1)));
		return 0;

	case G_PRECISIONTIMER_END:
		return SV_PrecisionTimerEnd(reinterpret_cast<void*>(args[1]));

	case G_CVAR_REGISTER:
		Cvar_Register(static_cast<vmCvar_t*>(VMA(1)), static_cast<const char*>(VMA(2)),
			static_cast<const char*>(VMA(3)), args[4]);
		return 0;

	case G_CVAR_UPDATE:
		Cvar_Update(static_cast<vmCvar_t*>(VMA(1)));
		return 0;

	case G_CVAR_SET:
		Cvar_VM_Set(static_cast<const char*>(VMA(1)), static_cast<const char*>(VMA(2)), VM_GAME);
		return 0;

	case G_CVAR_VARIABLE_INTEGER_VALUE:
		return Cvar_VariableIntegerValue(static_cast<const char*>(VMA(1)));

	case G_CVAR_VARIABLE_STRING_BUFFER:
		Cvar_VariableStringBuffer(static_cast<const char*>(VMA(1)), static_cast<char*>(VMA(2)), args[3]);
		return 0;

	case G_ARGC:
		return Cmd_Argc();

	case G_ARGV:
		Cmd_ArgvBuffer(args[1], static_cast<char*>(VMA(2)), args[3]);
		return 0;

	case G_SEND_CONSOLE_COMMAND:
		Cbuf_ExecuteText(args[1], static_cast<const char*>(VMA(2)));
		return 0;

	case G_FS_FOPEN_FILE:
		return FS_FOpenFileByMode(static_cast<const char*>(VMA(1)), static_cast<int*>(VMA(2)),
			static_cast<fsMode_t>(args[3]));

	case G_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;

	case G_FS_WRITE:
		FS_Write(VMA(1), args[2], args[3]);
		return 0;

	case G_FS_FCLOSE_FILE:
		FS_FCloseFile(args[1]);
		return 0;

	case G_FS_GETFILELIST:
		return FS_GetFileList(static_cast<const char*>(VMA(1)), static_cast<const char*>(VMA(2)),
			static_cast<char*>(VMA(3)), args[4]);

	case G_LOCATE_GAME_DATA:
		SV_LocateGameData(static_cast<sharedEntity_t*>(VMA(1)), args[2], args[3], static_cast<playerState_s*>(VMA(4)),
			args[5]);
		return 0;

	case G_DROP_CLIENT:
		SV_GameDropClient(args[1], static_cast<const char*>(VMA(2)));
		return 0;

	case G_SEND_SERVER_COMMAND:
		SV_GameSendServerCommand(args[1], static_cast<const char*>(VMA(2)));
		return 0;

	case G_LINKENTITY:
		SV_LinkEntity(static_cast<sharedEntity_t*>(VMA(1)));
		return 0;

	case G_UNLINKENTITY:
		SV_UnlinkEntity(static_cast<sharedEntity_t*>(VMA(1)));
		return 0;

	case G_ENTITIES_IN_BOX:
		return SV_AreaEntities(static_cast<const float*>(VMA(1)), static_cast<const float*>(VMA(2)),
			static_cast<int*>(VMA(3)), args[4]);

	case G_ENTITY_CONTACT:
		return SV_EntityContact(static_cast<const float*>(VMA(1)), static_cast<const float*>(VMA(2)),
			static_cast<const sharedEntity_t*>(VMA(3)), /*int capsule*/ qfalse);

	case G_ENTITY_CONTACTCAPSULE:
		return SV_EntityContact(static_cast<const float*>(VMA(1)), static_cast<const float*>(VMA(2)),
			static_cast<const sharedEntity_t*>(VMA(3)), /*int capsule*/ qtrue);

	case G_TRACE:
		SV_Trace(static_cast<trace_t*>(VMA(1)), static_cast<const float*>(VMA(2)), static_cast<const float*>(VMA(3)),
			static_cast<const float*>(VMA(4)), static_cast<const float*>(VMA(5)), args[6], args[7], /*int capsule*/
			qfalse, /*args[8]*/0, args[9]);
		return 0;
	case G_G2TRACE:
		SV_Trace(static_cast<trace_t*>(VMA(1)), static_cast<const float*>(VMA(2)), static_cast<const float*>(VMA(3)),
			static_cast<const float*>(VMA(4)), static_cast<const float*>(VMA(5)), args[6], args[7], /*int capsule*/
			qfalse, args[8], args[9]);
		return 0;
	case G_TRACECAPSULE:
		SV_Trace(static_cast<trace_t*>(VMA(1)), static_cast<const float*>(VMA(2)), static_cast<const float*>(VMA(3)),
			static_cast<const float*>(VMA(4)), static_cast<const float*>(VMA(5)), args[6], args[7], /*int capsule*/
			qtrue, args[8], args[9]);
		return 0;
	case G_POINT_CONTENTS:
		return SV_PointContents(static_cast<const float*>(VMA(1)), args[2]);
	case G_SET_SERVER_CULL:
		SV_SetServerCull(VMF(1));
		return 0;
	case G_SET_BRUSH_MODEL:
		SV_SetBrushModel(static_cast<sharedEntity_t*>(VMA(1)), static_cast<const char*>(VMA(2)));
		return 0;
	case G_IN_PVS:
		return SV_inPVS(static_cast<const float*>(VMA(1)), static_cast<const float*>(VMA(2)));
	case G_IN_PVS_IGNORE_PORTALS:
		return SV_inPVSIgnorePortals(static_cast<const float*>(VMA(1)), static_cast<const float*>(VMA(2)));

	case G_SET_CONFIGSTRING:
		SV_SetConfigstring(args[1], static_cast<const char*>(VMA(2)));
		return 0;
	case G_GET_CONFIGSTRING:
		SV_GetConfigstring(args[1], static_cast<char*>(VMA(2)), args[3]);
		return 0;
	case G_SET_USERINFO:
		SV_SetUserinfo(args[1], static_cast<const char*>(VMA(2)));
		return 0;
	case G_GET_USERINFO:
		SV_GetUserinfo(args[1], static_cast<char*>(VMA(2)), args[3]);
		return 0;
	case G_GET_SERVERINFO:
		SV_GetServerinfo(static_cast<char*>(VMA(1)), args[2]);
		return 0;
	case G_ADJUST_AREA_PORTAL_STATE:
		SV_AdjustAreaPortalState(static_cast<sharedEntity_t*>(VMA(1)), static_cast<qboolean>(args[2]));
		return 0;
	case G_AREAS_CONNECTED:
		return CM_AreasConnected(args[1], args[2]);

	case G_BOT_ALLOCATE_CLIENT:
		return SV_BotAllocateClient();
	case G_BOT_FREE_CLIENT:
		SV_BotFreeClient(args[1]);
		return 0;

	case G_GET_USERCMD:
		SV_GetUsercmd(args[1], static_cast<usercmd_s*>(VMA(2)));
		return 0;

	case G_SIEGEPERSSET:
		SV_SiegePersSet(static_cast<siegePers_t*>(VMA(1)));
		return 0;

	case G_SIEGEPERSGET:
		SV_SiegePersGet(static_cast<siegePers_t*>(VMA(1)));
		return 0;

		//rwwRMG - see below
		/*
	case G_GET_ENTITY_TOKEN:
		{
			const char	*s;

			s = COM_Parse( (const char **) &sv.entityParsePoint );
			Q_strncpyz( (char *)VMA(1), s, args[2] );
			if ( !sv.entityParsePoint && !s[0] ) {
				return qfalse;
			} else {
				return qtrue;
			}
		}
		*/

		/*
		case G_BOT_GET_MEMORY:
		void *ptr;
		ptr = Bot_GetMemoryGame(args[1]);
		return (int)ptr;
		case G_BOT_FREE_MEMORY:
		Bot_FreeMemoryGame((void *)VMA(1));
		return 0;
		*/
	case G_DEBUG_POLYGON_CREATE:
		return BotImport_DebugPolygonCreate(args[1], args[2], static_cast<float(*)[3]>(VMA(3)));
	case G_DEBUG_POLYGON_DELETE:
		BotImport_DebugPolygonDelete(args[1]);
		return 0;
	case G_REAL_TIME:
		return Com_RealTime(static_cast<qtime_s*>(VMA(1)));
	case G_SNAPVECTOR:
		Sys_SnapVector(static_cast<float*>(VMA(1)));
		return 0;

	case SP_GETSTRINGTEXTSTRING:
		return CL_SEP_GetStringTextString(static_cast<const char*>(VMA(1)), static_cast<char*>(VMA(2)), args[3]);

	case G_ROFF_CLEAN:
		return SV_ROFF_Clean();

	case G_ROFF_UPDATE_ENTITIES:
		SV_ROFF_UpdateEntities();
		return 0;

	case G_ROFF_CACHE:
		return SV_ROFF_Cache(static_cast<char*>(VMA(1)));

	case G_ROFF_PLAY:
		return SV_ROFF_Play(args[1], args[2], static_cast<qboolean>(args[3]));

	case G_ROFF_PURGE_ENT:
		return SV_ROFF_Purge_Ent(args[1]);

		//rww - dynamic vm memory allocation!
	case G_TRUEMALLOC:
		VM_Shifted_Alloc(static_cast<void**>(VMA(1)), args[2]);
		return 0;
	case G_TRUEFREE:
		VM_Shifted_Free(static_cast<void**>(VMA(1)));
		return 0;

		//rww - icarus traps
	case G_ICARUS_RUNSCRIPT:
		return ICARUS_RunScript(ConvertedEntity(static_cast<sharedEntity_t*>(VMA(1))),
			static_cast<const char*>(VMA(2)));

	case G_ICARUS_REGISTERSCRIPT:
		return ICARUS_RegisterScript(static_cast<const char*>(VMA(1)), static_cast<qboolean>(args[2]));

	case G_ICARUS_INIT:
		ICARUS_Init();
		return 0;

	case G_ICARUS_VALIDENT:
		return ICARUS_ValidEnt(ConvertedEntity(static_cast<sharedEntity_t*>(VMA(1))));

	case G_ICARUS_ISINITIALIZED:
		return ICARUS_IsInitialized(args[1]);

	case G_ICARUS_MAINTAINTASKMANAGER:
		return ICARUS_MaintainTaskManager(args[1]);

	case G_ICARUS_ISRUNNING:
		return ICARUS_IsRunning(args[1]);

	case G_ICARUS_TASKIDPENDING:
		return Q3_TaskIDPending(static_cast<sharedEntity_t*>(VMA(1)), static_cast<taskID_t>(args[2]));

	case G_ICARUS_INITENT:
		ICARUS_InitEnt(ConvertedEntity(static_cast<sharedEntity_t*>(VMA(1))));
		return 0;

	case G_ICARUS_FREEENT:
		ICARUS_FreeEnt(ConvertedEntity(static_cast<sharedEntity_t*>(VMA(1))));
		return 0;

	case G_ICARUS_ASSOCIATEENT:
		ICARUS_AssociateEnt(ConvertedEntity(static_cast<sharedEntity_t*>(VMA(1))));
		return 0;

	case G_ICARUS_SHUTDOWN:
		ICARUS_Shutdown();
		return 0;

	case G_ICARUS_TASKIDSET:
		//rww - note that we are passing in the true entity here.
		//This is because we allow modification of certain non-pointer values,
		//which is valid.
		Q3_TaskIDSet(static_cast<sharedEntity_t*>(VMA(1)), static_cast<taskID_t>(args[2]), args[3]);
		return 0;

	case G_ICARUS_TASKIDCOMPLETE:
		//same as above.
		Q3_TaskIDComplete(static_cast<sharedEntity_t*>(VMA(1)), static_cast<taskID_t>(args[2]));
		return 0;

	case G_ICARUS_SETVAR:
		Q3_SetVar(args[1], args[2], static_cast<const char*>(VMA(3)), static_cast<const char*>(VMA(4)));
		return 0;

	case G_ICARUS_VARIABLEDECLARED:
		return Q3_VariableDeclared(static_cast<const char*>(VMA(1)));

	case G_ICARUS_GETFLOATVARIABLE:
		return Q3_GetFloatVariable(static_cast<const char*>(VMA(1)), static_cast<float*>(VMA(2)));

	case G_ICARUS_GETSTRINGVARIABLE:
	{
		auto rec = static_cast<const char*>(VMA(2));
		return Q3_GetStringVariable(static_cast<const char*>(VMA(1)), &rec);
	}

	case G_ICARUS_GETVECTORVARIABLE:
		return Q3_GetVectorVariable(static_cast<const char*>(VMA(1)), static_cast<float*>(VMA(2)));

		//rww - BEGIN NPC NAV TRAPS
	case G_NAV_INIT:
		navigator.Init();
		return 0;
	case G_NAV_FREE:
		navigator.Free();
		return 0;
	case G_NAV_LOAD:
		return navigator.Load(static_cast<const char*>(VMA(1)), args[2]);
	case G_NAV_SAVE:
		return navigator.Save(static_cast<const char*>(VMA(1)), args[2]);
	case G_NAV_ADDRAWPOINT:
		return navigator.AddRawPoint(static_cast<float*>(VMA(1)), args[2], args[3]);
	case G_NAV_CALCULATEPATHS:
		navigator.CalculatePaths(static_cast<qboolean>(args[1]));
		return 0;
	case G_NAV_HARDCONNECT:
		navigator.HardConnect(args[1], args[2]);
		return 0;
	case G_NAV_SHOWNODES:
		navigator.ShowNodes();
		return 0;
	case G_NAV_SHOWEDGES:
		navigator.ShowEdges();
		return 0;
	case G_NAV_SHOWPATH:
		navigator.ShowPath(args[1], args[2]);
		return 0;
	case G_NAV_GETNEARESTNODE:
		return navigator.GetNearestNode(static_cast<sharedEntity_t*>(VMA(1)), args[2], args[3], args[4]);
	case G_NAV_GETBESTNODE:
		return navigator.GetBestNode(args[1], args[2], args[3]);
	case G_NAV_GETNODEPOSITION:
		return navigator.GetNodePosition(args[1], static_cast<float*>(VMA(2)));
	case G_NAV_GETNODENUMEDGES:
		return navigator.GetNodeNumEdges(args[1]);
	case G_NAV_GETNODEEDGE:
		return navigator.GetNodeEdge(args[1], args[2]);
	case G_NAV_GETNUMNODES:
		return navigator.GetNumNodes();
	case G_NAV_CONNECTED:
		return navigator.Connected(args[1], args[2]);
	case G_NAV_GETPATHCOST:
		return navigator.GetPathCost(args[1], args[2]);
	case G_NAV_GETEDGECOST:
		return navigator.GetEdgeCost(args[1], args[2]);
	case G_NAV_GETPROJECTEDNODE:
		return navigator.GetProjectedNode(static_cast<float*>(VMA(1)), args[2]);
	case G_NAV_CHECKFAILEDNODES:
		navigator.CheckFailedNodes(static_cast<sharedEntity_t*>(VMA(1)));
		return 0;
	case G_NAV_ADDFAILEDNODE:
		navigator.AddFailedNode(static_cast<sharedEntity_t*>(VMA(1)), args[2]);
		return 0;
	case G_NAV_NODEFAILED:
		return navigator.NodeFailed(static_cast<sharedEntity_t*>(VMA(1)), args[2]);
	case G_NAV_NODESARENEIGHBORS:
		return navigator.NodesAreNeighbors(args[1], args[2]);
	case G_NAV_CLEARFAILEDEDGE:
		navigator.ClearFailedEdge(static_cast<failedEdge_t*>(VMA(1)));
		return 0;
	case G_NAV_CLEARALLFAILEDEDGES:
		navigator.ClearAllFailedEdges();
		return 0;
	case G_NAV_EDGEFAILED:
		return navigator.EdgeFailed(args[1], args[2]);
	case G_NAV_ADDFAILEDEDGE:
		navigator.AddFailedEdge(args[1], args[2], args[3]);
		return 0;
	case G_NAV_CHECKFAILEDEDGE:
		return navigator.CheckFailedEdge(static_cast<failedEdge_t*>(VMA(1)));
	case G_NAV_CHECKALLFAILEDEDGES:
		navigator.CheckAllFailedEdges();
		return 0;
	case G_NAV_ROUTEBLOCKED:
		return navigator.RouteBlocked(args[1], args[2], args[3], args[4]);
	case G_NAV_GETBESTNODEALTROUTE:
		return navigator.GetBestNodeAltRoute(args[1], args[2], static_cast<int*>(VMA(3)), args[4]);
	case G_NAV_GETBESTNODEALT2:
		return navigator.GetBestNodeAltRoute(args[1], args[2], args[3]);
	case G_NAV_GETBESTPATHBETWEENENTS:
		return navigator.GetBestPathBetweenEnts(static_cast<sharedEntity_t*>(VMA(1)),
			static_cast<sharedEntity_t*>(VMA(2)),
			args[3]);
	case G_NAV_GETNODERADIUS:
		return navigator.GetNodeRadius(args[1]);
	case G_NAV_CHECKBLOCKEDEDGES:
		navigator.CheckBlockedEdges();
		return 0;
	case G_NAV_CLEARCHECKEDNODES:
		navigator.ClearCheckedNodes();
		return 0;
	case G_NAV_CHECKEDNODE:
		return navigator.CheckedNode(args[1], args[2]);
	case G_NAV_SETCHECKEDNODE:
		navigator.SetCheckedNode(args[1], args[2], args[3]);
		return 0;
	case G_NAV_FLAGALLNODES:
		navigator.FlagAllNodes(args[1]);
		return 0;
	case G_NAV_GETPATHSCALCULATED:
		return navigator.pathsCalculated;
	case G_NAV_SETPATHSCALCULATED:
		navigator.pathsCalculated = static_cast<qboolean>(args[1]);
		return 0;
		//rww - END NPC NAV TRAPS

	case G_SET_SHARED_BUFFER:
		SV_RegisterSharedMemory(static_cast<char*>(VMA(1)));
		return 0;
		//====================================

	case BOTLIB_SETUP:
		return SV_BotLibSetup();
	case BOTLIB_SHUTDOWN:
		return SV_BotLibShutdown();
	case BOTLIB_LIBVAR_SET:
		return botlib_export->BotLibVarSet(static_cast<char*>(VMA(1)), static_cast<char*>(VMA(2)));
	case BOTLIB_LIBVAR_GET:
		return botlib_export->BotLibVarGet(static_cast<char*>(VMA(1)), static_cast<char*>(VMA(2)), args[3]);

	case BOTLIB_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine(static_cast<char*>(VMA(1)));
	case BOTLIB_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle(static_cast<const char*>(VMA(1)));
	case BOTLIB_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle(args[1]);
	case BOTLIB_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle(args[1], static_cast<pc_token_s*>(VMA(2)));
	case BOTLIB_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine(args[1], static_cast<char*>(VMA(2)), static_cast<int*>(VMA(3)));

	case BOTLIB_startFrame:
		return botlib_export->BotLibStartFrame(VMF(1));
	case BOTLIB_LOAD_MAP:
		return botlib_export->BotLibLoadMap(static_cast<const char*>(VMA(1)));
	case BOTLIB_UPDATENTITY:
		return botlib_export->BotLibUpdateEntity(args[1], static_cast<bot_entitystate_s*>(VMA(2)));
	case BOTLIB_TEST:
		return botlib_export->Test(args[1], static_cast<char*>(VMA(2)), static_cast<float*>(VMA(3)),
			static_cast<float*>(VMA(4)));

	case BOTLIB_GET_SNAPSHOT_ENTITY:
		return SV_BotGetSnapshotEntity(args[1], args[2]);
	case BOTLIB_GET_CONSOLE_MESSAGE:
		return SV_BotGetConsoleMessage(args[1], static_cast<char*>(VMA(2)), args[3]);
	case BOTLIB_USER_COMMAND:
		SV_ClientThink(&svs.clients[args[1]], static_cast<usercmd_s*>(VMA(2)));
		return 0;

	case BOTLIB_AAS_BBOX_AREAS:
		return botlib_export->aas.AAS_BBoxAreas(static_cast<float*>(VMA(1)), static_cast<float*>(VMA(2)),
			static_cast<int*>(VMA(3)), args[4]);
	case BOTLIB_AAS_AREA_INFO:
		return botlib_export->aas.AAS_AreaInfo(args[1], static_cast<aas_areainfo_s*>(VMA(2)));
	case BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL:
		return botlib_export->aas.AAS_AlternativeRouteGoals(static_cast<float*>(VMA(1)), args[2],
			static_cast<float*>(VMA(3)),
			args[4], args[5], static_cast<aas_altroutegoal_s*>(VMA(6)),
			args[7],
			args[8]);
	case BOTLIB_AAS_ENTITY_INFO:
		botlib_export->aas.AAS_EntityInfo(args[1], static_cast<aas_entityinfo_s*>(VMA(2)));
		return 0;

	case BOTLIB_AAS_INITIALIZED:
		return botlib_export->aas.AAS_Initialized();
	case BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX:
		botlib_export->aas.AAS_PresenceTypeBoundingBox(args[1], static_cast<float*>(VMA(2)),
			static_cast<float*>(VMA(3)));
		return 0;
	case BOTLIB_AAS_TIME:
		return FloatAsInt(botlib_export->aas.AAS_Time());

	case BOTLIB_AAS_POINT_AREA_NUM:
		return botlib_export->aas.AAS_PointAreaNum(static_cast<float*>(VMA(1)));
	case BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX:
		return botlib_export->aas.AAS_PointReachabilityAreaIndex(static_cast<float*>(VMA(1)));
	case BOTLIB_AAS_TRACE_AREAS:
		return botlib_export->aas.AAS_TraceAreas(static_cast<float*>(VMA(1)), static_cast<float*>(VMA(2)),
			static_cast<int*>(VMA(3)), static_cast<float(*)[3]>(VMA(4)), args[5]);

	case BOTLIB_AAS_POINT_CONTENTS:
		return botlib_export->aas.AAS_PointContents(static_cast<float*>(VMA(1)));
	case BOTLIB_AAS_NEXT_BSP_ENTITY:
		return botlib_export->aas.AAS_NextBSPEntity(args[1]);
	case BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_ValueForBSPEpairKey(args[1], static_cast<char*>(VMA(2)),
			static_cast<char*>(VMA(3)),
			args[4]);
	case BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.
			AAS_VectorForBSPEpairKey(args[1], static_cast<char*>(VMA(2)),
				static_cast<float*>(VMA(3)));
	case BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_FloatForBSPEpairKey(args[1], static_cast<char*>(VMA(2)),
			static_cast<float*>(VMA(3)));
	case BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_IntForBSPEpairKey(args[1], static_cast<char*>(VMA(2)), static_cast<int*>(VMA(3)));

	case BOTLIB_AAS_AREA_REACHABILITY:
		return botlib_export->aas.AAS_AreaReachability(args[1]);

	case BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA:
		return botlib_export->aas.AAS_AreaTravelTimeToGoalArea(args[1], static_cast<float*>(VMA(2)), args[3], args[4]);
	case BOTLIB_AAS_ENABLE_ROUTING_AREA:
		return botlib_export->aas.AAS_EnableRoutingArea(args[1], args[2]);
	case BOTLIB_AAS_PREDICT_ROUTE:
		return botlib_export->aas.AAS_PredictRoute(static_cast<aas_predictroute_s*>(VMA(1)), args[2],
			static_cast<float*>(VMA(3)), args[4], args[5], args[6], args[7],
			args[8],
			args[9], args[10], args[11]);

	case BOTLIB_AAS_SWIMMING:
		return botlib_export->aas.AAS_Swimming(static_cast<float*>(VMA(1)));
	case BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT:
		return botlib_export->aas.AAS_PredictClientMovement(static_cast<aas_clientmove_s*>(VMA(1)), args[2],
			static_cast<float*>(VMA(3)), args[4], args[5],
			static_cast<float*>(VMA(6)), static_cast<float*>(VMA(7)),
			args[8],
			args[9], VMF(10), args[11], args[12], args[13]);

	case BOTLIB_EA_SAY:
		botlib_export->ea.EA_Say(args[1], static_cast<char*>(VMA(2)));
		return 0;
	case BOTLIB_EA_SAY_TEAM:
		botlib_export->ea.EA_SayTeam(args[1], static_cast<char*>(VMA(2)));
		return 0;
	case BOTLIB_EA_COMMAND:
		botlib_export->ea.EA_Command(args[1], static_cast<char*>(VMA(2)));
		return 0;

	case BOTLIB_EA_ACTION:
		botlib_export->ea.EA_Action(args[1], args[2]);
		break;
	case BOTLIB_EA_GESTURE:
		botlib_export->ea.EA_Gesture(args[1]);
		return 0;
	case BOTLIB_EA_TALK:
		botlib_export->ea.EA_Talk(args[1]);
		return 0;
	case BOTLIB_EA_ATTACK:
		botlib_export->ea.EA_Attack(args[1]);
		return 0;
	case BOTLIB_EA_ALT_ATTACK:
		botlib_export->ea.EA_Alt_Attack(args[1]);
		return 0;
	case BOTLIB_EA_FORCEPOWER:
		botlib_export->ea.EA_ForcePower(args[1]);
		return 0;
	case BOTLIB_EA_USE:
		botlib_export->ea.EA_Use(args[1]);
		return 0;
	case BOTLIB_EA_RESPAWN:
		botlib_export->ea.EA_Respawn(args[1]);
		return 0;
	case BOTLIB_EA_CROUCH:
		botlib_export->ea.EA_Crouch(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_UP:
		botlib_export->ea.EA_MoveUp(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_DOWN:
		botlib_export->ea.EA_MoveDown(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_FORWARD:
		botlib_export->ea.EA_MoveForward(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_BACK:
		botlib_export->ea.EA_MoveBack(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_LEFT:
		botlib_export->ea.EA_MoveLeft(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_RIGHT:
		botlib_export->ea.EA_MoveRight(args[1]);
		return 0;

	case BOTLIB_EA_SELECT_WEAPON:
		botlib_export->ea.EA_SelectWeapon(args[1], args[2]);
		return 0;
	case BOTLIB_EA_JUMP:
		botlib_export->ea.EA_Jump(args[1]);
		return 0;
	case BOTLIB_EA_DELAYED_JUMP:
		botlib_export->ea.EA_DelayedJump(args[1]);
		return 0;
	case BOTLIB_EA_MOVE:
		botlib_export->ea.EA_Move(args[1], static_cast<float*>(VMA(2)), VMF(3));
		return 0;
	case BOTLIB_EA_VIEW:
		botlib_export->ea.EA_View(args[1], static_cast<float*>(VMA(2)));
		return 0;

	case BOTLIB_EA_END_REGULAR:
		botlib_export->ea.EA_EndRegular(args[1], VMF(2));
		return 0;
	case BOTLIB_EA_GET_INPUT:
		botlib_export->ea.EA_GetInput(args[1], VMF(2), static_cast<bot_input_s*>(VMA(3)));
		return 0;
	case BOTLIB_EA_RESET_INPUT:
		botlib_export->ea.EA_ResetInput(args[1]);
		return 0;

	case BOTLIB_AI_LOAD_CHARACTER:
		return botlib_export->ai.BotLoadCharacter(static_cast<char*>(VMA(1)), VMF(2));
	case BOTLIB_AI_FREE_CHARACTER:
		botlib_export->ai.BotFreeCharacter(args[1]);
		return 0;
	case BOTLIB_AI_CHARACTERISTIC_FLOAT:
		return FloatAsInt(botlib_export->ai.Characteristic_Float(args[1], args[2]));
	case BOTLIB_AI_CHARACTERISTIC_BFLOAT:
		return FloatAsInt(botlib_export->ai.Characteristic_BFloat(args[1], args[2], VMF(3), VMF(4)));
	case BOTLIB_AI_CHARACTERISTIC_INTEGER:
		return botlib_export->ai.Characteristic_Integer(args[1], args[2]);
	case BOTLIB_AI_CHARACTERISTIC_BINTEGER:
		return botlib_export->ai.Characteristic_BInteger(args[1], args[2], args[3], args[4]);
	case BOTLIB_AI_CHARACTERISTIC_STRING:
		botlib_export->ai.Characteristic_String(args[1], args[2], static_cast<char*>(VMA(3)), args[4]);
		return 0;

	case BOTLIB_AI_ALLOC_CHAT_STATE:
		return botlib_export->ai.BotAllocChatState();
	case BOTLIB_AI_FREE_CHAT_STATE:
		botlib_export->ai.BotFreeChatState(args[1]);
		return 0;
	case BOTLIB_AI_QUEUE_CONSOLE_MESSAGE:
		botlib_export->ai.BotQueueConsoleMessage(args[1], args[2], static_cast<char*>(VMA(3)));
		return 0;
	case BOTLIB_AI_REMOVE_CONSOLE_MESSAGE:
		botlib_export->ai.BotRemoveConsoleMessage(args[1], args[2]);
		return 0;
	case BOTLIB_AI_NEXT_CONSOLE_MESSAGE:
		return botlib_export->ai.BotNextConsoleMessage(args[1], static_cast<bot_consolemessage_s*>(VMA(2)));
	case BOTLIB_AI_NUM_CONSOLE_MESSAGE:
		return botlib_export->ai.BotNumConsoleMessages(args[1]);
	case BOTLIB_AI_INITIAL_CHAT:
		botlib_export->ai.BotInitialChat(args[1], static_cast<char*>(VMA(2)), args[3], static_cast<char*>(VMA(4)),
			static_cast<char*>(VMA(5)), static_cast<char*>(VMA(6)),
			static_cast<char*>(VMA(7)),
			static_cast<char*>(VMA(8)), static_cast<char*>(VMA(9)),
			static_cast<char*>(VMA(10)),
			static_cast<char*>(VMA(11)));
		return 0;
	case BOTLIB_AI_NUM_INITIAL_CHATS:
		return botlib_export->ai.BotNumInitialChats(args[1], static_cast<char*>(VMA(2)));
	case BOTLIB_AI_REPLY_CHAT:
		return botlib_export->ai.BotReplyChat(args[1], static_cast<char*>(VMA(2)), args[3], args[4],
			static_cast<char*>(VMA(5)),
			static_cast<char*>(VMA(6)), static_cast<char*>(VMA(7)),
			static_cast<char*>(VMA(8)), static_cast<char*>(VMA(9)),
			static_cast<char*>(VMA(10)), static_cast<char*>(VMA(11)),
			static_cast<char*>(VMA(12)));
	case BOTLIB_AI_CHAT_LENGTH:
		return botlib_export->ai.BotChatLength(args[1]);
	case BOTLIB_AI_ENTER_CHAT:
		botlib_export->ai.BotEnterChat(args[1], args[2], args[3]);
		return 0;
	case BOTLIB_AI_GET_CHAT_MESSAGE:
		botlib_export->ai.BotGetChatMessage(args[1], static_cast<char*>(VMA(2)), args[3]);
		return 0;
	case BOTLIB_AI_STRING_CONTAINS:
		return botlib_export->ai.StringContains(static_cast<char*>(VMA(1)), static_cast<char*>(VMA(2)), args[3]);
	case BOTLIB_AI_FIND_MATCH:
		return botlib_export->ai.BotFindMatch(static_cast<char*>(VMA(1)), static_cast<bot_match_s*>(VMA(2)), args[3]);
	case BOTLIB_AI_MATCH_VARIABLE:
		botlib_export->ai.BotMatchVariable(static_cast<bot_match_s*>(VMA(1)), args[2], static_cast<char*>(VMA(3)),
			args[4]);
		return 0;
	case BOTLIB_AI_UNIFY_WHITE_SPACES:
		botlib_export->ai.UnifyWhiteSpaces(static_cast<char*>(VMA(1)));
		return 0;
	case BOTLIB_AI_REPLACE_SYNONYMS:
		botlib_export->ai.BotReplaceSynonyms(static_cast<char*>(VMA(1)), args[2]);
		return 0;
	case BOTLIB_AI_LOAD_CHAT_FILE:
		return botlib_export->ai.BotLoadChatFile(args[1], static_cast<char*>(VMA(2)), static_cast<char*>(VMA(3)));
	case BOTLIB_AI_SET_CHAT_GENDER:
		botlib_export->ai.BotSetChatGender(args[1], args[2]);
		return 0;
	case BOTLIB_AI_SET_CHAT_NAME:
		botlib_export->ai.BotSetChatName(args[1], static_cast<char*>(VMA(2)), args[3]);
		return 0;

	case BOTLIB_AI_RESET_GOAL_STATE:
		botlib_export->ai.BotResetGoalState(args[1]);
		return 0;
	case BOTLIB_AI_RESET_AVOID_GOALS:
		botlib_export->ai.BotResetAvoidGoals(args[1]);
		return 0;
	case BOTLIB_AI_REMOVE_FROM_AVOID_GOALS:
		botlib_export->ai.BotRemoveFromAvoidGoals(args[1], args[2]);
		return 0;
	case BOTLIB_AI_PUSH_GOAL:
		botlib_export->ai.BotPushGoal(args[1], static_cast<bot_goal_s*>(VMA(2)));
		return 0;
	case BOTLIB_AI_POP_GOAL:
		botlib_export->ai.BotPopGoal(args[1]);
		return 0;
	case BOTLIB_AI_EMPTY_GOAL_STACK:
		botlib_export->ai.BotEmptyGoalStack(args[1]);
		return 0;
	case BOTLIB_AI_DUMP_AVOID_GOALS:
		botlib_export->ai.BotDumpAvoidGoals(args[1]);
		return 0;
	case BOTLIB_AI_DUMP_GOAL_STACK:
		botlib_export->ai.BotDumpGoalStack(args[1]);
		return 0;
	case BOTLIB_AI_GOAL_NAME:
		botlib_export->ai.BotGoalName(args[1], static_cast<char*>(VMA(2)), args[3]);
		return 0;
	case BOTLIB_AI_GET_TOP_GOAL:
		return botlib_export->ai.BotGetTopGoal(args[1], static_cast<bot_goal_s*>(VMA(2)));
	case BOTLIB_AI_GET_SECOND_GOAL:
		return botlib_export->ai.BotGetSecondGoal(args[1], static_cast<bot_goal_s*>(VMA(2)));
	case BOTLIB_AI_CHOOSE_LTG_ITEM:
		return botlib_export->ai.BotChooseLTGItem(args[1], static_cast<float*>(VMA(2)), static_cast<int*>(VMA(3)),
			args[4]);
	case BOTLIB_AI_CHOOSE_NBG_ITEM:
		return botlib_export->ai.BotChooseNBGItem(args[1], static_cast<float*>(VMA(2)), static_cast<int*>(VMA(3)),
			args[4],
			static_cast<bot_goal_s*>(VMA(5)), VMF(6));
	case BOTLIB_AI_TOUCHING_GOAL:
		return botlib_export->ai.BotTouchingGoal(static_cast<float*>(VMA(1)), static_cast<bot_goal_s*>(VMA(2)));
	case BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE:
		return botlib_export->ai.BotItemGoalInVisButNotVisible(args[1], static_cast<float*>(VMA(2)),
			static_cast<float*>(VMA(3)),
			static_cast<bot_goal_s*>(VMA(4)));
	case BOTLIB_AI_GET_LEVEL_ITEM_GOAL:
		return botlib_export->ai.BotGetLevelItemGoal(args[1], static_cast<char*>(VMA(2)),
			static_cast<bot_goal_s*>(VMA(3)));
	case BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL:
		return botlib_export->ai.BotGetNextCampSpotGoal(args[1], static_cast<bot_goal_s*>(VMA(2)));
	case BOTLIB_AI_GET_MAP_LOCATION_GOAL:
		return botlib_export->ai.BotGetMapLocationGoal(static_cast<char*>(VMA(1)), static_cast<bot_goal_s*>(VMA(2)));
	case BOTLIB_AI_AVOID_GOAL_TIME:
		return FloatAsInt(botlib_export->ai.BotAvoidGoalTime(args[1], args[2]));
	case BOTLIB_AI_SET_AVOID_GOAL_TIME:
		botlib_export->ai.BotSetAvoidGoalTime(args[1], args[2], VMF(3));
		return 0;
	case BOTLIB_AI_INIT_LEVEL_ITEMS:
		botlib_export->ai.BotInitLevelItems();
		return 0;
	case BOTLIB_AI_UPDATE_ENTITY_ITEMS:
		botlib_export->ai.BotUpdateEntityItems();
		return 0;
	case BOTLIB_AI_LOAD_ITEM_WEIGHTS:
		return botlib_export->ai.BotLoadItemWeights(args[1], static_cast<char*>(VMA(2)));
	case BOTLIB_AI_FREE_ITEM_WEIGHTS:
		botlib_export->ai.BotFreeItemWeights(args[1]);
		return 0;
	case BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC:
		botlib_export->ai.BotInterbreedGoalFuzzyLogic(args[1], args[2], args[3]);
		return 0;
	case BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC:
		botlib_export->ai.BotSaveGoalFuzzyLogic(args[1], static_cast<char*>(VMA(2)));
		return 0;
	case BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC:
		botlib_export->ai.BotMutateGoalFuzzyLogic(args[1], VMF(2));
		return 0;
	case BOTLIB_AI_ALLOC_GOAL_STATE:
		return botlib_export->ai.BotAllocGoalState(args[1]);
	case BOTLIB_AI_FREE_GOAL_STATE:
		botlib_export->ai.BotFreeGoalState(args[1]);
		return 0;

	case BOTLIB_AI_RESET_MOVE_STATE:
		botlib_export->ai.BotResetMoveState(args[1]);
		return 0;
	case BOTLIB_AI_ADD_AVOID_SPOT:
		botlib_export->ai.BotAddAvoidSpot(args[1], static_cast<float*>(VMA(2)), VMF(3), args[4]);
		return 0;
	case BOTLIB_AI_MOVE_TO_GOAL:
		botlib_export->ai.BotMoveToGoal(static_cast<bot_moveresult_s*>(VMA(1)), args[2],
			static_cast<bot_goal_s*>(VMA(3)),
			args[4]);
		return 0;
	case BOTLIB_AI_MOVE_IN_DIRECTION:
		return botlib_export->ai.BotMoveInDirection(args[1], static_cast<float*>(VMA(2)), VMF(3), args[4]);
	case BOTLIB_AI_RESET_AVOID_REACH:
		botlib_export->ai.BotResetAvoidReach(args[1]);
		return 0;
	case BOTLIB_AI_RESET_LAST_AVOID_REACH:
		botlib_export->ai.BotResetLastAvoidReach(args[1]);
		return 0;
	case BOTLIB_AI_REACHABILITY_AREA:
		return botlib_export->ai.BotReachabilityArea(static_cast<float*>(VMA(1)), args[2]);
	case BOTLIB_AI_MOVEMENT_VIEW_TARGET:
		return botlib_export->ai.BotMovementViewTarget(args[1], static_cast<bot_goal_s*>(VMA(2)), args[3], VMF(4),
			static_cast<float*>(VMA(5)));
	case BOTLIB_AI_PREDICT_VISIBLE_POSITION:
		return botlib_export->ai.BotPredictVisiblePosition(static_cast<float*>(VMA(1)), args[2],
			static_cast<bot_goal_s*>(VMA(3)), args[4],
			static_cast<float*>(VMA(5)));
	case BOTLIB_AI_ALLOC_MOVE_STATE:
		return botlib_export->ai.BotAllocMoveState();
	case BOTLIB_AI_FREE_MOVE_STATE:
		botlib_export->ai.BotFreeMoveState(args[1]);
		return 0;
	case BOTLIB_AI_INIT_MOVE_STATE:
		botlib_export->ai.BotInitMoveState(args[1], static_cast<bot_initmove_s*>(VMA(2)));
		return 0;

	case BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON:
		return botlib_export->ai.BotChooseBestFightWeapon(args[1], static_cast<int*>(VMA(2)));
	case BOTLIB_AI_GET_WEAPON_INFO:
		botlib_export->ai.BotGetWeaponInfo(args[1], args[2], static_cast<weaponinfo_s*>(VMA(3)));
		return 0;
	case BOTLIB_AI_LOAD_WEAPON_WEIGHTS:
		return botlib_export->ai.BotLoadWeaponWeights(args[1], static_cast<char*>(VMA(2)));
	case BOTLIB_AI_ALLOC_WEAPON_STATE:
		return botlib_export->ai.BotAllocWeaponState();
	case BOTLIB_AI_FREE_WEAPON_STATE:
		botlib_export->ai.BotFreeWeaponState(args[1]);
		return 0;
	case BOTLIB_AI_RESET_WEAPON_STATE:
		botlib_export->ai.BotResetWeaponState(args[1]);
		return 0;

	case BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION:
		return botlib_export->ai.GeneticParentsAndChildSelection(args[1], static_cast<float*>(VMA(2)),
			static_cast<int*>(VMA(3)), static_cast<int*>(VMA(4)),
			static_cast<int*>(VMA(5)));

	case G_R_REGISTERSKIN:
		return re->RegisterServerSkin(static_cast<const char*>(VMA(1)));

	case G_G2_LISTBONES:
		re->G2API_ListBones(static_cast<CGhoul2Info*>(VMA(1)), args[2]);
		return 0;

	case G_G2_LISTSURFACES:
		re->G2API_ListSurfaces(reinterpret_cast<CGhoul2Info*>(args[1]));
		return 0;

	case G_G2_HAVEWEGHOULMODELS:
		return SV_G2API_HaveWeGhoul2Models(VMA(1));

	case G_G2_SETMODELS:
		re->G2API_SetGhoul2model_indexes(*reinterpret_cast<CGhoul2Info_v*>(args[1]), static_cast<qhandle_t*>(VMA(2)),
			static_cast<qhandle_t*>(VMA(3)));
		return 0;

	case G_G2_GETBOLT:
		return re->G2API_GetBoltMatrix(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2], args[3], static_cast<mdxaBone_t*>(VMA(4)),
			static_cast<const float*>(VMA(5)), static_cast<const float*>(VMA(6)), args[7],
			static_cast<qhandle_t*>(VMA(8)), static_cast<float*>(VMA(9)));

	case G_G2_GETBOLT_NOREC:
		re->G2API_BoltMatrixReconstruction(qfalse); //gG2_GBMNoReconstruct = qtrue;
		return re->G2API_GetBoltMatrix(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2], args[3], static_cast<mdxaBone_t*>(VMA(4)),
			static_cast<const float*>(VMA(5)), static_cast<const float*>(VMA(6)), args[7],
			static_cast<qhandle_t*>(VMA(8)), static_cast<float*>(VMA(9)));

	case G_G2_GETBOLT_NOREC_NOROT:
		re->G2API_BoltMatrixReconstruction(qfalse); //gG2_GBMNoReconstruct = qtrue;
		re->G2API_BoltMatrixSPMethod(qtrue); //gG2_GBMUseSPMethod = qtrue;
		return re->G2API_GetBoltMatrix(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2], args[3], static_cast<mdxaBone_t*>(VMA(4)),
			static_cast<const float*>(VMA(5)), static_cast<const float*>(VMA(6)), args[7],
			static_cast<qhandle_t*>(VMA(8)), static_cast<float*>(VMA(9)));

	case G_G2_INITGHOUL2MODEL:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 1;
#endif
		return re->G2API_InitGhoul2Model(static_cast<CGhoul2Info_v**>(VMA(1)), static_cast<const char*>(VMA(2)),
			args[3],
			args[4],
			args[5], args[6], args[7]);

	case G_G2_SETSKIN:
	{
		CGhoul2Info_v& g2 = *reinterpret_cast<CGhoul2Info_v*>(args[1]);
		const int modelIndex = args[2];

		return re->G2API_SetSkin(g2, modelIndex, args[3], args[4]);
	}

	case G_G2_SIZE:
		return re->G2API_Ghoul2Size(*reinterpret_cast<CGhoul2Info_v*>(args[1]));

	case G_G2_ADDBOLT:
		return re->G2API_AddBolt(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2], static_cast<const char*>(VMA(3)));

	case G_G2_SETBOLTINFO:
		re->G2API_SetBoltInfo(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2], args[3]);
		return 0;

	case G_G2_ANGLEOVERRIDE:
		return re->G2API_SetBoneAngles(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2], static_cast<const char*>(VMA(3)),
			static_cast<float*>(VMA(4)), args[5],
			static_cast<const Eorientations>(args[6]),
			static_cast<const Eorientations>(args[7]),
			static_cast<const Eorientations>(args[8]),
			static_cast<qhandle_t*>(VMA(9)), args[10], args[11]);

	case G_G2_PLAYANIM:
		return re->G2API_SetBoneAnim(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2], static_cast<const char*>(VMA(3)), args[4],
			args[5],
			args[6], VMF(7), args[8], VMF(9), args[10]);

	case G_G2_GETBONEANIM:
	{
		CGhoul2Info_v& g2 = *reinterpret_cast<CGhoul2Info_v*>(args[1]);
		const int modelIndex = args[10];

		return re->G2API_GetBoneAnim(g2, modelIndex, static_cast<const char*>(VMA(2)), args[3],
			static_cast<float*>(VMA(4)), static_cast<int*>(VMA(5)),
			static_cast<int*>(VMA(6)), static_cast<int*>(VMA(7)),
			static_cast<float*>(VMA(8)),
			static_cast<int*>(VMA(9)));
	}

	case G_G2_GETGLANAME:
		//return (int)G2API_GetGLAName(*((CGhoul2Info_v *)args[1]), args[2]);
	{
		//Since returning a pointer in such a way to a VM seems to cause MASSIVE FAILURE<tm>, we will shove data into the pointer the vm passes instead
		const auto point = static_cast<char*>(VMA(3));
		const char* local = re->G2API_GetGLAName(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2]);
		if (local)
		{
			strcpy(point, local);
		}
	}

	return 0;

	case G_G2_COPYGHOUL2INSTANCE:
		return re->G2API_CopyGhoul2Instance(*reinterpret_cast<CGhoul2Info_v*>(args[1]), *reinterpret_cast<CGhoul2Info_v*>(args[2]), args[3]);

	case G_G2_COPYSPECIFICGHOUL2MODEL:
		re->G2API_CopySpecificG2Model(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2], *reinterpret_cast<CGhoul2Info_v*>(args[3]), args[4]);
		return 0;

	case G_G2_DUPLICATEGHOUL2INSTANCE:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 1;
#endif
		re->G2API_DuplicateGhoul2Instance(*reinterpret_cast<CGhoul2Info_v*>(args[1]), static_cast<CGhoul2Info_v**>(VMA(2)));
		return 0;

	case G_G2_HASGHOUL2MODELONINDEX:
		//return (int)G2API_HasGhoul2ModelOnIndex((CGhoul2Info_v **)args[1], args[2]);
		return re->G2API_HasGhoul2ModelOnIndex(static_cast<CGhoul2Info_v**>(VMA(1)), args[2]);

	case G_G2_REMOVEGHOUL2MODEL:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 1;
#endif
		//return (int)G2API_RemoveGhoul2Model((CGhoul2Info_v **)args[1], args[2]);
		return re->G2API_RemoveGhoul2Model(static_cast<CGhoul2Info_v**>(VMA(1)), args[2]);

	case G_G2_REMOVEGHOUL2MODELS:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 1;
#endif
		//return (int)G2API_RemoveGhoul2Models((CGhoul2Info_v **)args[1]);
		return re->G2API_RemoveGhoul2Models(static_cast<CGhoul2Info_v**>(VMA(1)));

	case G_G2_CLEANMODELS:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 1;
#endif
		re->G2API_CleanGhoul2Models(static_cast<CGhoul2Info_v**>(VMA(1)));
		//	re->G2API_CleanGhoul2Models((CGhoul2Info_v **)args[1]);
		return 0;

	case G_G2_COLLISIONDETECT:
		re->G2API_CollisionDetect(static_cast<CollisionRecord_t*>(VMA(1)), *reinterpret_cast<CGhoul2Info_v*>(args[2]),
			static_cast<const float*>(VMA(3)), static_cast<const float*>(VMA(4)), args[5],
			args[6],
			static_cast<float*>(VMA(7)), static_cast<float*>(VMA(8)), static_cast<float*>(VMA(9)),
			G2VertSpaceServer, args[10], args[11], VMF(12));
		return 0;

	case G_G2_COLLISIONDETECTCACHE:
		re->G2API_CollisionDetectCache(static_cast<CollisionRecord_t*>(VMA(1)), *reinterpret_cast<CGhoul2Info_v*>(args[2]),
			static_cast<const float*>(VMA(3)), static_cast<const float*>(VMA(4)), args[5],
			args[6],
			static_cast<float*>(VMA(7)), static_cast<float*>(VMA(8)),
			static_cast<float*>(VMA(9)),
			G2VertSpaceServer, args[10], args[11], VMF(12));
		return 0;

	case G_G2_SETROOTSURFACE:
		return re->G2API_SetRootSurface(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2], static_cast<const char*>(VMA(3)));

	case G_G2_SETSURFACEONOFF:
		return re->G2API_SetSurfaceOnOff(*reinterpret_cast<CGhoul2Info_v*>(args[1]), static_cast<const char*>(VMA(2)),
			/*(const int)VMA(3)*/
			args[3]);

	case G_G2_SETNEWORIGIN:
		return re->G2API_SetNewOrigin(*reinterpret_cast<CGhoul2Info_v*>(args[1]), /*(const int)VMA(2)*/args[2]);

	case G_G2_DOESBONEEXIST:
	{
		CGhoul2Info_v& g2 = *reinterpret_cast<CGhoul2Info_v*>(args[1]);
		return re->G2API_DoesBoneExist(g2, args[2], static_cast<const char*>(VMA(3)));
	}

	case G_G2_GETSURFACERENDERSTATUS:
	{
		CGhoul2Info_v& g2 = *reinterpret_cast<CGhoul2Info_v*>(args[1]);

		return re->G2API_GetSurfaceRenderStatus(g2, args[2], static_cast<const char*>(VMA(3)));
	}

	case G_G2_ABSURDSMOOTHING:
	{
		CGhoul2Info_v& g2 = *reinterpret_cast<CGhoul2Info_v*>(args[1]);

		re->G2API_AbsurdSmoothing(g2, static_cast<qboolean>(args[2]));
	}
	return 0;

	case G_G2_SETRAGDOLL:
	{
		//Convert the info in the shared structure over to the class-based version.
		const auto rdParamst = static_cast<sharedRagDollParams_t*>(VMA(2));
		CRagDollParams rdParams{};

		if (!rdParamst)
		{
			re->G2API_ResetRagDoll(*reinterpret_cast<CGhoul2Info_v*>(args[1]));
			return 0;
		}

		VectorCopy(rdParamst->angles, rdParams.angles);
		VectorCopy(rdParamst->position, rdParams.position);
		VectorCopy(rdParamst->scale, rdParams.scale);
		VectorCopy(rdParamst->pelvisAnglesOffset, rdParams.pelvisAnglesOffset);
		VectorCopy(rdParamst->pelvisPositionOffset, rdParams.pelvisPositionOffset);

		rdParams.fImpactStrength = rdParamst->fImpactStrength;
		rdParams.fShotStrength = rdParamst->fShotStrength;
		rdParams.me = rdParamst->me;

		rdParams.startFrame = rdParamst->startFrame;
		rdParams.endFrame = rdParamst->endFrame;

		rdParams.collisionType = rdParamst->collisionType;
		rdParams.CallRagDollBegin = rdParamst->CallRagDollBegin;

		rdParams.RagPhase = static_cast<CRagDollParams::ERagPhase>(rdParamst->RagPhase);
		rdParams.effectorsToTurnOff = static_cast<CRagDollParams::ERagEffector>(rdParamst->effectorsToTurnOff);

		re->G2API_SetRagDoll(*reinterpret_cast<CGhoul2Info_v*>(args[1]), &rdParams);
	}
	return 0;
	case G_G2_ANIMATEG2MODELS:
	{
		const sharedRagDollUpdateParams_t* rduParamst = static_cast<sharedRagDollUpdateParams_t*>(VMA(3));
		CRagDollUpdateParams rduParams;

		if (!rduParamst)
		{
			return 0;
		}

		VectorCopy(rduParamst->angles, rduParams.angles);
		VectorCopy(rduParamst->position, rduParams.position);
		VectorCopy(rduParamst->scale, rduParams.scale);
		VectorCopy(rduParamst->velocity, rduParams.velocity);

		rduParams.me = rduParamst->me;
		rduParams.settleFrame = rduParamst->settleFrame;

		re->G2API_AnimateG2ModelsRag(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2], &rduParams);
	}
	return 0;

	//additional ragdoll options -rww
	case G_G2_RAGPCJCONSTRAINT:
		return re->G2API_RagPCJConstraint(*reinterpret_cast<CGhoul2Info_v*>(args[1]), static_cast<const char*>(VMA(2)),
			static_cast<float*>(VMA(3)), static_cast<float*>(VMA(4)));
	case G_G2_RAGPCJGRADIENTSPEED:
		return re->G2API_RagPCJGradientSpeed(*reinterpret_cast<CGhoul2Info_v*>(args[1]), static_cast<const char*>(VMA(2)), VMF(3));
	case G_G2_RAGEFFECTORGOAL:
		return re->G2API_RagEffectorGoal(*reinterpret_cast<CGhoul2Info_v*>(args[1]), static_cast<const char*>(VMA(2)),
			static_cast<float*>(VMA(3)));
	case G_G2_GETRAGBONEPOS:
		return re->G2API_GetRagBonePos(*reinterpret_cast<CGhoul2Info_v*>(args[1]), static_cast<const char*>(VMA(2)),
			static_cast<float*>(VMA(3)),
			static_cast<float*>(VMA(4)), static_cast<float*>(VMA(5)),
			static_cast<float*>(VMA(6)));
	case G_G2_RAGEFFECTORKICK:
		return re->G2API_RagEffectorKick(*reinterpret_cast<CGhoul2Info_v*>(args[1]), static_cast<const char*>(VMA(2)),
			static_cast<float*>(VMA(3)));
	case G_G2_RAGFORCESOLVE:
		return re->G2API_RagForceSolve(*reinterpret_cast<CGhoul2Info_v*>(args[1]), static_cast<qboolean>(args[2]));

	case G_G2_SETBONEIKSTATE:
		return re->G2API_SetBoneIKState(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2], static_cast<const char*>(VMA(3)), args[4],
			static_cast<sharedSetBoneIKStateParams_t*>(VMA(5)));
	case G_G2_IKMOVE:
		return re->G2API_IKMove(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2], static_cast<sharedIKMoveParams_t*>(VMA(3)));

	case G_G2_REMOVEBONE:
	{
		CGhoul2Info_v& g2 = *reinterpret_cast<CGhoul2Info_v*>(args[1]);

		return re->G2API_RemoveBone(g2, args[3], static_cast<const char*>(VMA(2)));
	}

	case G_G2_ATTACHINSTANCETOENTNUM:
	{
		re->G2API_AttachInstanceToEntNum(*reinterpret_cast<CGhoul2Info_v*>(args[1]), args[2], static_cast<qboolean>(args[3]));
	}
	return 0;
	case G_G2_CLEARATTACHEDINSTANCE:
		re->G2API_ClearAttachedInstance(args[1]);
		return 0;
	case G_G2_CLEANENTATTACHMENTS:
		re->G2API_CleanEntAttachments();
		return 0;
	case G_G2_OVERRIDESERVER:
	{
		CGhoul2Info_v& g2 = *reinterpret_cast<CGhoul2Info_v*>(args[1]);
		return re->G2API_OverrideServerWithClientData(g2, 0);
	}

	case G_G2_GETSURFACENAME:
	{
		//Since returning a pointer in such a way to a VM seems to cause MASSIVE FAILURE<tm>, we will shove data into the pointer the vm passes instead
		const auto point = static_cast<char*>(VMA(4));
		const int modelIndex = args[3];

		CGhoul2Info_v& g2 = *reinterpret_cast<CGhoul2Info_v*>(args[1]);

		const char* local = re->G2API_GetSurfaceName(g2, modelIndex, args[2]);
		if (local)
		{
			strcpy(point, local);
		}
	}

	return 0;

	case G_SET_ACTIVE_SUBBSP:
		SV_SetActiveSubBSP(args[1]);
		return 0;

	case G_RMG_INIT:
		return 0;

	case G_CM_REGISTER_TERRAIN:
		return 0;

	case G_BOT_UPDATEWAYPOINTS:
		SV_BotWaypointReception(args[1], static_cast<wpobject_t**>(VMA(2)));
		return 0;
	case G_BOT_CALCULATEPATHS:
		SV_BotCalculatePaths(args[1]);
		return 0;

	case G_GET_ENTITY_TOKEN:
		return SV_GetEntityToken(static_cast<char*>(VMA(1)), args[2]);

	default:
		Com_Error(ERR_DROP, "Bad game system trap: %ld", static_cast<long>(args[0]));
	}
	return -1;
}

void SV_InitGame(const qboolean restart)
{
	int i;
	client_t* cl;

	// clear level pointers
	sv.entityParsePoint = CM_EntityString();
	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
		cl->gentity = nullptr;

	GVM_InitGame(sv.time, Com_Milliseconds(), restart);
}

void SV_BindGame(void)
{
	static gameImport_t gi;
	char dllName[MAX_OSPATH] = "SerenityJediEngine2026-mpgame" ARCH_STRING DLL_EXT;

	memset(&gi, 0, sizeof gi);

	gvm = VM_Create(VM_GAME);
	if (gvm && !gvm->isLegacy)
	{
		gi.Print = Com_Printf;
		gi.Error = Com_Error;
		gi.Milliseconds = Com_Milliseconds;
		gi.PrecisionTimerStart = SV_PrecisionTimerStart;
		gi.PrecisionTimerEnd = SV_PrecisionTimerEnd;
		gi.SV_RegisterSharedMemory = SV_RegisterSharedMemory;
		gi.RealTime = Com_RealTime;
		gi.TrueMalloc = VM_Shifted_Alloc;
		gi.TrueFree = VM_Shifted_Free;
		gi.SnapVector = Sys_SnapVector;
		gi.Cvar_Register = Cvar_Register;
		gi.Cvar_Set = GVM_Cvar_Set;
		gi.Cvar_Update = Cvar_Update;
		gi.Cvar_VariableIntegerValue = Cvar_VariableIntegerValue;
		gi.Cvar_VariableStringBuffer = Cvar_VariableStringBuffer;
		gi.Argc = Cmd_Argc;
		gi.Argv = Cmd_ArgvBuffer;
		gi.FS_Close = FS_FCloseFile;
		gi.FS_GetFileList = FS_GetFileList;
		gi.FS_Open = FS_FOpenFileByMode;
		gi.FS_Read = FS_Read;
		gi.FS_Write = FS_Write;
		gi.AdjustAreaPortalState = SV_AdjustAreaPortalState;
		gi.AreasConnected = CM_AreasConnected;
		gi.DebugPolygonCreate = BotImport_DebugPolygonCreate;
		gi.DebugPolygonDelete = BotImport_DebugPolygonDelete;
		gi.DropClient = SV_GameDropClient;
		gi.EntitiesInBox = SV_AreaEntities;
		gi.EntityContact = SV_EntityContact;
		gi.Trace = SV_Trace;
		gi.GetConfigstring = SV_GetConfigstring;
		gi.GetEntityToken = SV_GetEntityToken;
		gi.GetServerinfo = SV_GetServerinfo;
		gi.GetUsercmd = SV_GetUsercmd;
		gi.GetUserinfo = SV_GetUserinfo;
		gi.InPVS = SV_inPVS;
		gi.InPVSIgnorePortals = SV_inPVSIgnorePortals;
		gi.LinkEntity = SV_LinkEntity;
		gi.LocateGameData = SV_LocateGameData;
		gi.PointContents = SV_PointContents;
		gi.SendConsoleCommand = Cbuf_ExecuteText;
		gi.SendServerCommand = SV_GameSendServerCommand;
		gi.SetBrushModel = SV_SetBrushModel;
		gi.SetConfigstring = SV_SetConfigstring;
		gi.SetServerCull = SV_SetServerCull;
		gi.SetUserinfo = SV_SetUserinfo;
		gi.SiegePersSet = SV_SiegePersSet;
		gi.SiegePersGet = SV_SiegePersGet;
		gi.UnlinkEntity = SV_UnlinkEntity;
		gi.ROFF_Clean = SV_ROFF_Clean;
		gi.ROFF_UpdateEntities = SV_ROFF_UpdateEntities;
		gi.ROFF_Cache = SV_ROFF_Cache;
		gi.ROFF_Play = SV_ROFF_Play;
		gi.ROFF_Purge_Ent = SV_ROFF_Purge_Ent;
		gi.ICARUS_RunScript = ICARUS_RunScript;
		gi.ICARUS_RegisterScript = SV_ICARUS_RegisterScript;
		gi.ICARUS_Init = ICARUS_Init;
		gi.ICARUS_ValidEnt = SV_ICARUS_ValidEnt;
		gi.ICARUS_IsInitialized = ICARUS_IsInitialized;
		gi.ICARUS_MaintainTaskManager = ICARUS_MaintainTaskManager;
		gi.ICARUS_IsRunning = ICARUS_IsRunning;
		gi.ICARUS_TaskIDPending = ICARUS_TaskIDPending;
		gi.ICARUS_InitEnt = ICARUS_InitEnt;
		gi.ICARUS_FreeEnt = ICARUS_FreeEnt;
		gi.ICARUS_AssociateEnt = ICARUS_AssociateEnt;
		gi.ICARUS_Shutdown = ICARUS_Shutdown;
		gi.ICARUS_TaskIDSet = SV_ICARUS_TaskIDSet;
		gi.ICARUS_TaskIDComplete = SV_ICARUS_TaskIDComplete;
		gi.ICARUS_SetVar = Q3_SetVar;
		gi.ICARUS_VariableDeclared = Q3_VariableDeclared;
		gi.ICARUS_GetFloatVariable = Q3_GetFloatVariable;
		gi.ICARUS_GetStringVariable = SV_ICARUS_GetStringVariable;
		gi.ICARUS_GetVectorVariable = SV_ICARUS_GetVectorVariable;
		gi.Nav_Init = SV_Nav_Init;
		gi.Nav_Free = SV_Nav_Free;
		gi.Nav_Load = SV_Nav_Load;
		gi.Nav_Save = SV_Nav_Save;
		gi.Nav_AddRawPoint = SV_Nav_AddRawPoint;
		gi.Nav_CalculatePaths = SV_Nav_CalculatePaths;
		gi.Nav_HardConnect = SV_Nav_HardConnect;
		gi.Nav_ShowNodes = SV_Nav_ShowNodes;
		gi.Nav_ShowEdges = SV_Nav_ShowEdges;
		gi.Nav_ShowPath = SV_Nav_ShowPath;
		gi.Nav_GetNearestNode = SV_Nav_GetNearestNode;
		gi.Nav_GetBestNode = SV_Nav_GetBestNode;
		gi.Nav_GetNodePosition = SV_Nav_GetNodePosition;
		gi.Nav_GetNodeNumEdges = SV_Nav_GetNodeNumEdges;
		gi.Nav_GetNodeEdge = SV_Nav_GetNodeEdge;
		gi.Nav_GetNumNodes = SV_Nav_GetNumNodes;
		gi.Nav_Connected = SV_Nav_Connected;
		gi.Nav_GetPathCost = SV_Nav_GetPathCost;
		gi.Nav_GetEdgeCost = SV_Nav_GetEdgeCost;
		gi.Nav_GetProjectedNode = SV_Nav_GetProjectedNode;
		gi.Nav_CheckFailedNodes = SV_Nav_CheckFailedNodes;
		gi.Nav_AddFailedNode = SV_Nav_AddFailedNode;
		gi.Nav_NodeFailed = SV_Nav_NodeFailed;
		gi.Nav_NodesAreNeighbors = SV_Nav_NodesAreNeighbors;
		gi.Nav_ClearFailedEdge = SV_Nav_ClearFailedEdge;
		gi.Nav_ClearAllFailedEdges = SV_Nav_ClearAllFailedEdges;
		gi.Nav_EdgeFailed = SV_Nav_EdgeFailed;
		gi.Nav_AddFailedEdge = SV_Nav_AddFailedEdge;
		gi.Nav_CheckFailedEdge = SV_Nav_CheckFailedEdge;
		gi.Nav_CheckAllFailedEdges = SV_Nav_CheckAllFailedEdges;
		gi.Nav_RouteBlocked = SV_Nav_RouteBlocked;
		gi.Nav_GetBestNodeAltRoute = SV_Nav_GetBestNodeAltRoute;
		gi.Nav_GetBestNodeAltRoute2 = SV_Nav_GetBestNodeAltRoute2;
		gi.Nav_GetBestPathBetweenEnts = SV_Nav_GetBestPathBetweenEnts;
		gi.Nav_GetNodeRadius = SV_Nav_GetNodeRadius;
		gi.Nav_CheckBlockedEdges = SV_Nav_CheckBlockedEdges;
		gi.Nav_ClearCheckedNodes = SV_Nav_ClearCheckedNodes;
		gi.Nav_CheckedNode = SV_Nav_CheckedNode;
		gi.Nav_SetCheckedNode = SV_Nav_SetCheckedNode;
		gi.Nav_FlagAllNodes = SV_Nav_FlagAllNodes;
		gi.Nav_GetPathsCalculated = SV_Nav_GetPathsCalculated;
		gi.Nav_SetPathsCalculated = SV_Nav_SetPathsCalculated;
		gi.BotAllocateClient = SV_BotAllocateClient;
		gi.BotFreeClient = SV_BotFreeClient;
		gi.BotLoadCharacter = SV_BotLoadCharacter;
		gi.BotFreeCharacter = SV_BotFreeCharacter;
		gi.Characteristic_Float = SV_Characteristic_Float;
		gi.Characteristic_BFloat = SV_Characteristic_BFloat;
		gi.Characteristic_Integer = SV_Characteristic_Integer;
		gi.Characteristic_BInteger = SV_Characteristic_BInteger;
		gi.Characteristic_String = SV_Characteristic_String;
		gi.BotAllocChatState = SV_BotAllocChatState;
		gi.BotFreeChatState = SV_BotFreeChatState;
		gi.BotQueueConsoleMessage = SV_BotQueueConsoleMessage;
		gi.BotRemoveConsoleMessage = SV_BotRemoveConsoleMessage;
		gi.BotNextConsoleMessage = SV_BotNextConsoleMessage;
		gi.BotNumConsoleMessages = SV_BotNumConsoleMessages;
		gi.BotInitialChat = SV_BotInitialChat;
		gi.BotReplyChat = SV_BotReplyChat;
		gi.BotChatLength = SV_BotChatLength;
		gi.BotEnterChat = SV_BotEnterChat;
		gi.StringContains = SV_StringContains;
		gi.BotFindMatch = SV_BotFindMatch;
		gi.BotMatchVariable = SV_BotMatchVariable;
		gi.UnifyWhiteSpaces = SV_UnifyWhiteSpaces;
		gi.BotReplaceSynonyms = SV_BotReplaceSynonyms;
		gi.BotLoadChatFile = SV_BotLoadChatFile;
		gi.BotSetChatGender = SV_BotSetChatGender;
		gi.BotSetChatName = SV_BotSetChatName;
		gi.BotResetGoalState = SV_BotResetGoalState;
		gi.BotResetAvoidGoals = SV_BotResetAvoidGoals;
		gi.BotPushGoal = SV_BotPushGoal;
		gi.BotPopGoal = SV_BotPopGoal;
		gi.BotEmptyGoalStack = SV_BotEmptyGoalStack;
		gi.BotDumpAvoidGoals = SV_BotDumpAvoidGoals;
		gi.BotDumpGoalStack = SV_BotDumpGoalStack;
		gi.BotGoalName = SV_BotGoalName;
		gi.BotGetTopGoal = SV_BotGetTopGoal;
		gi.BotGetSecondGoal = SV_BotGetSecondGoal;
		gi.BotChooseLTGItem = SV_BotChooseLTGItem;
		gi.BotChooseNBGItem = SV_BotChooseNBGItem;
		gi.BotTouchingGoal = SV_BotTouchingGoal;
		gi.BotItemGoalInVisButNotVisible = SV_BotItemGoalInVisButNotVisible;
		gi.BotGetLevelItemGoal = SV_BotGetLevelItemGoal;
		gi.BotAvoidGoalTime = SV_BotAvoidGoalTime;
		gi.BotInitLevelItems = SV_BotInitLevelItems;
		gi.BotUpdateEntityItems = SV_BotUpdateEntityItems;
		gi.BotLoadItemWeights = SV_BotLoadItemWeights;
		gi.BotFreeItemWeights = SV_BotFreeItemWeights;
		gi.BotSaveGoalFuzzyLogic = SV_BotSaveGoalFuzzyLogic;
		gi.BotAllocGoalState = SV_BotAllocGoalState;
		gi.BotFreeGoalState = SV_BotFreeGoalState;
		gi.BotResetMoveState = SV_BotResetMoveState;
		gi.BotMoveToGoal = SV_BotMoveToGoal;
		gi.BotMoveInDirection = SV_BotMoveInDirection;
		gi.BotResetAvoidReach = SV_BotResetAvoidReach;
		gi.BotResetLastAvoidReach = SV_BotResetLastAvoidReach;
		gi.BotReachabilityArea = SV_BotReachabilityArea;
		gi.BotMovementViewTarget = SV_BotMovementViewTarget;
		gi.BotAllocMoveState = SV_BotAllocMoveState;
		gi.BotFreeMoveState = SV_BotFreeMoveState;
		gi.BotInitMoveState = SV_BotInitMoveState;
		gi.BotChooseBestFightWeapon = SV_BotChooseBestFightWeapon;
		gi.BotGetWeaponInfo = SV_BotGetWeaponInfo;
		gi.BotLoadWeaponWeights = SV_BotLoadWeaponWeights;
		gi.BotAllocWeaponState = SV_BotAllocWeaponState;
		gi.BotFreeWeaponState = SV_BotFreeWeaponState;
		gi.BotResetWeaponState = SV_BotResetWeaponState;
		gi.GeneticParentsAndChildSelection = SV_GeneticParentsAndChildSelection;
		gi.BotInterbreedGoalFuzzyLogic = SV_BotInterbreedGoalFuzzyLogic;
		gi.BotMutateGoalFuzzyLogic = SV_BotMutateGoalFuzzyLogic;
		gi.BotGetNextCampSpotGoal = SV_BotGetNextCampSpotGoal;
		gi.BotGetMapLocationGoal = SV_BotGetMapLocationGoal;
		gi.BotNumInitialChats = SV_BotNumInitialChats;
		gi.BotGetChatMessage = SV_BotGetChatMessage;
		gi.BotRemoveFromAvoidGoals = SV_BotRemoveFromAvoidGoals;
		gi.BotPredictVisiblePosition = SV_BotPredictVisiblePosition;
		gi.BotSetAvoidGoalTime = SV_BotSetAvoidGoalTime;
		gi.BotAddAvoidSpot = SV_BotAddAvoidSpot;
		gi.BotLibSetup = SV_BotLibSetup;
		gi.BotLibShutdown = SV_BotLibShutdown;
		gi.BotLibVarSet = SV_BotLibVarSet;
		gi.BotLibVarGet = SV_BotLibVarGet;
		gi.BotLibDefine = SV_BotLibDefine;
		gi.BotLibStartFrame = SV_BotLibStartFrame;
		gi.BotLibLoadMap = SV_BotLibLoadMap;
		gi.BotLibUpdateEntity = SV_BotLibUpdateEntity;
		gi.BotLibTest = SV_BotLibTest;
		gi.BotGetSnapshotEntity = SV_BotGetSnapshotEntity;
		gi.BotGetServerCommand = SV_BotGetServerCommand;
		gi.BotUserCommand = SV_BotUserCommand;
		gi.BotUpdateWaypoints = SV_BotWaypointReception;
		gi.BotCalculatePaths = SV_BotCalculatePaths;
		gi.AAS_EnableRoutingArea = SV_AAS_EnableRoutingArea;
		gi.AAS_BBoxAreas = SV_AAS_BBoxAreas;
		gi.AAS_AreaInfo = SV_AAS_AreaInfo;
		gi.AAS_EntityInfo = SV_AAS_EntityInfo;
		gi.AAS_Initialized = SV_AAS_Initialized;
		gi.AAS_PresenceTypeBoundingBox = SV_AAS_PresenceTypeBoundingBox;
		gi.AAS_Time = SV_AAS_Time;
		gi.AAS_PointAreaNum = SV_AAS_PointAreaNum;
		gi.AAS_TraceAreas = SV_AAS_TraceAreas;
		gi.AAS_PointContents = SV_AAS_PointContents;
		gi.AAS_NextBSPEntity = SV_AAS_NextBSPEntity;
		gi.AAS_ValueForBSPEpairKey = SV_AAS_ValueForBSPEpairKey;
		gi.AAS_VectorForBSPEpairKey = SV_AAS_VectorForBSPEpairKey;
		gi.AAS_FloatForBSPEpairKey = SV_AAS_FloatForBSPEpairKey;
		gi.AAS_IntForBSPEpairKey = SV_AAS_IntForBSPEpairKey;
		gi.AAS_AreaReachability = SV_AAS_AreaReachability;
		gi.AAS_AreaTravelTimeToGoalArea = SV_AAS_AreaTravelTimeToGoalArea;
		gi.AAS_Swimming = SV_AAS_Swimming;
		gi.AAS_PredictClientMovement = SV_AAS_PredictClientMovement;
		gi.AAS_AlternativeRouteGoals = SV_AAS_AlternativeRouteGoals;
		gi.AAS_PredictRoute = SV_AAS_PredictRoute;
		gi.AAS_PointReachabilityAreaIndex = SV_AAS_PointReachabilityAreaIndex;
		gi.EA_Say = SV_EA_Say;
		gi.EA_SayTeam = SV_EA_SayTeam;
		gi.EA_Command = SV_EA_Command;
		gi.EA_Action = SV_EA_Action;
		gi.EA_Gesture = SV_EA_Gesture;
		gi.EA_Talk = SV_EA_Talk;
		gi.EA_Attack = SV_EA_Attack;
		gi.EA_Alt_Attack = SV_EA_Alt_Attack;
		gi.EA_ForcePower = SV_EA_ForcePower;
		gi.EA_Use = SV_EA_Use;
		gi.EA_Respawn = SV_EA_Respawn;
		gi.EA_Crouch = SV_EA_Crouch;
		gi.EA_MoveUp = SV_EA_MoveUp;
		gi.EA_MoveDown = SV_EA_MoveDown;
		gi.EA_MoveForward = SV_EA_MoveForward;
		gi.EA_MoveBack = SV_EA_MoveBack;
		gi.EA_MoveLeft = SV_EA_MoveLeft;
		gi.EA_MoveRight = SV_EA_MoveRight;
		gi.EA_SelectWeapon = SV_EA_SelectWeapon;
		gi.EA_Jump = SV_EA_Jump;
		gi.EA_DelayedJump = SV_EA_DelayedJump;
		gi.EA_Move = SV_EA_Move;
		gi.EA_View = SV_EA_View;
		gi.EA_EndRegular = SV_EA_EndRegular;
		gi.EA_GetInput = SV_EA_GetInput;
		gi.EA_ResetInput = SV_EA_ResetInput;
		gi.PC_LoadSource = SV_PC_LoadSource;
		gi.PC_FreeSource = SV_PC_FreeSource;
		gi.PC_ReadToken = SV_PC_ReadToken;
		gi.PC_SourceFileAndLine = SV_PC_SourceFileAndLine;
		gi.R_RegisterSkin = SV_RE_RegisterSkin;
		gi.SetActiveSubBSP = SV_SetActiveSubBSP;
		gi.CM_RegisterTerrain = SV_CM_RegisterTerrain;
		gi.RMG_Init = SV_RMG_Init;
		gi.G2API_ListModelBones = SV_G2API_ListModelBones;
		gi.G2API_ListModelSurfaces = SV_G2API_ListModelSurfaces;
		gi.G2API_HaveWeGhoul2Models = SV_G2API_HaveWeGhoul2Models;
		gi.G2API_SetGhoul2model_indexes = SV_G2API_SetGhoul2model_indexes;
		gi.G2API_GetBoltMatrix = SV_G2API_GetBoltMatrix;
		gi.G2API_GetBoltMatrix_NoReconstruct = SV_G2API_GetBoltMatrix_NoReconstruct;
		gi.G2API_GetBoltMatrix_NoRecNoRot = SV_G2API_GetBoltMatrix_NoRecNoRot;
		gi.G2API_InitGhoul2Model = SV_G2API_InitGhoul2Model;
		gi.G2API_SetSkin = SV_G2API_SetSkin;
		gi.G2API_Ghoul2Size = SV_G2API_Ghoul2Size;
		gi.G2API_AddBolt = SV_G2API_AddBolt;
		gi.G2API_SetBoltInfo = SV_G2API_SetBoltInfo;
		gi.G2API_SetBoneAngles = SV_G2API_SetBoneAngles;
		gi.G2API_SetBoneAnim = SV_G2API_SetBoneAnim;
		gi.G2API_GetBoneAnim = SV_G2API_GetBoneAnim;
		gi.G2API_GetGLAName = SV_G2API_GetGLAName;
		gi.G2API_CopyGhoul2Instance = SV_G2API_CopyGhoul2Instance;
		gi.G2API_CopySpecificGhoul2Model = SV_G2API_CopySpecificGhoul2Model;
		gi.G2API_DuplicateGhoul2Instance = SV_G2API_DuplicateGhoul2Instance;
		gi.G2API_HasGhoul2ModelOnIndex = SV_G2API_HasGhoul2ModelOnIndex;
		gi.G2API_RemoveGhoul2Model = SV_G2API_RemoveGhoul2Model;
		gi.G2API_RemoveGhoul2Models = SV_G2API_RemoveGhoul2Models;
		gi.G2API_CleanGhoul2Models = SV_G2API_CleanGhoul2Models;
		gi.G2API_CollisionDetect = SV_G2API_CollisionDetect;
		gi.G2API_CollisionDetectCache = SV_G2API_CollisionDetectCache;
		gi.G2API_SetRootSurface = SV_G2API_SetRootSurface;
		gi.G2API_SetSurfaceOnOff = SV_G2API_SetSurfaceOnOff;
		gi.G2API_SetNewOrigin = SV_G2API_SetNewOrigin;
		gi.G2API_DoesBoneExist = SV_G2API_DoesBoneExist;
		gi.G2API_GetSurfaceRenderStatus = SV_G2API_GetSurfaceRenderStatus;
		gi.G2API_AbsurdSmoothing = SV_G2API_AbsurdSmoothing;
		gi.G2API_SetRagDoll = SV_G2API_SetRagDoll;
		gi.G2API_AnimateG2Models = SV_G2API_AnimateG2Models;
		gi.G2API_RagPCJConstraint = SV_G2API_RagPCJConstraint;
		gi.G2API_RagPCJGradientSpeed = SV_G2API_RagPCJGradientSpeed;
		gi.G2API_RagEffectorGoal = SV_G2API_RagEffectorGoal;
		gi.G2API_GetRagBonePos = SV_G2API_GetRagBonePos;
		gi.G2API_RagEffectorKick = SV_G2API_RagEffectorKick;
		gi.G2API_RagForceSolve = SV_G2API_RagForceSolve;
		gi.G2API_SetBoneIKState = SV_G2API_SetBoneIKState;
		gi.G2API_IKMove = SV_G2API_IKMove;
		gi.G2API_RemoveBone = SV_G2API_RemoveBone;
		gi.G2API_AttachInstanceToEntNum = SV_G2API_AttachInstanceToEntNum;
		gi.G2API_ClearAttachedInstance = SV_G2API_ClearAttachedInstance;
		gi.G2API_CleanEntAttachments = SV_G2API_CleanEntAttachments;
		gi.G2API_OverrideServer = SV_G2API_OverrideServer;
		gi.G2API_GetSurfaceName = SV_G2API_GetSurfaceName;

		const auto GetGameAPI = reinterpret_cast<GetGameAPI_t>(gvm->GetModuleAPI);
		gameExport_t* ret = GetGameAPI(GAME_API_VERSION, &gi);
		if (!ret)
		{
			//free VM?
			svs.gameStarted = qfalse;
			Com_Error(ERR_FATAL, "GetGameAPI failed on %s", dllName);
		}
		ge = ret;

		return;
	}

	// fall back to legacy syscall/vm_call api
	gvm = VM_CreateLegacy(VM_GAME, SV_GameSystemCalls);
	if (!gvm)
	{
		svs.gameStarted = qfalse;
		Com_Error(ERR_DROP, "VM_CreateLegacy on game failed");
	}
}

void SV_UnbindGame(void)
{
	GVM_ShutdownGame(qfalse);
	VM_Free(gvm);
	gvm = nullptr;
}

void SV_RestartGame(void)
{
	GVM_ShutdownGame(qtrue);

	gvm = VM_Restart(gvm);
	SV_BindGame();
	if (!gvm)
	{
		svs.gameStarted = qfalse;
		Com_Error(ERR_DROP, "VM_Restart on game failed");
	}

	SV_InitGame(qtrue);
}