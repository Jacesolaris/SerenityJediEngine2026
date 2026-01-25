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

// Task Manager
//
//	-- jweier

// this include must remain at the top of every Icarus CPP file
#include "StdAfx.h"
#include "IcarusImplementation.h"

#include "blockstream.h"
#include "sequence.h"
#include "taskmanager.h"
#include "sequencer.h"

#define ICARUS_VALIDATE(a) if ( a == false ) return TASK_FAILED;

#define STL_ITERATE( a, b )		for ( a = b.begin(); a != b.end(); ++a )
#define STL_INSERT( a, b )		a.insert( a.end(), b );

/*
=================================================

CTask

=================================================
*/

CTask::CTask()
= default;

CTask::~CTask()
= default;

CTask* CTask::Create(const int guid, CBlock* block)
{
	const auto task = new CTask;

	//TODO: Emit warning
	if (task == nullptr)
		return nullptr;

	task->SetTimeStamp(0);
	task->SetBlock(block);
	task->SetGUID(guid);

	return task;
}

/*
-------------------------
Free
-------------------------
*/

void CTask::Free() const
{
	//NOTENOTE: The block is not consumed by the task, it is the sequencer's job to clean blocks up
	delete this;
}

/*
=================================================

CTaskGroup

=================================================
*/

CTaskGroup::CTaskGroup()
{
	Init();

	m_GUID = 0;
	m_parent = nullptr;
}

CTaskGroup::~CTaskGroup()
{
	m_completedTasks.clear();
}

/*
-------------------------
SetGUID
-------------------------
*/

void CTaskGroup::SetGUID(const int guid)
{
	m_GUID = guid;
}

/*
-------------------------
Init
-------------------------
*/

void CTaskGroup::Init()
{
	m_completedTasks.clear();

	m_numCompleted = 0;
	m_parent = nullptr;
}

/*
-------------------------
Add
-------------------------
*/

int CTaskGroup::Add(const CTask* task)
{
	m_completedTasks[task->GetGUID()] = false;
	return TASK_OK;
}

/*
-------------------------
MarkTaskComplete
-------------------------
*/

bool CTaskGroup::MarkTaskComplete(const int id)
{
	if (m_completedTasks.find(id) != m_completedTasks.end())
	{
		m_completedTasks[id] = true;
		m_numCompleted++;

		return true;
	}

	return false;
}

/*
=================================================

CTaskManager

=================================================
*/

CTaskManager::CTaskManager() : m_owner(nullptr), m_ownerID(0), m_curGroup(nullptr), m_GUID(0), m_count(0),
m_resident(false)
{
	static int uniqueID = 0;
	m_id = uniqueID++;
}

CTaskManager::~CTaskManager()
= default;

/*
-------------------------
Create
-------------------------
*/

CTaskManager* CTaskManager::Create()
{
	return new CTaskManager;
}

/*
-------------------------
Init
-------------------------
*/

int CTaskManager::Init(CSequencer* owner)
{
	//TODO: Emit warning
	if (owner == nullptr)
		return TASK_FAILED;

	m_tasks.clear();
	m_owner = owner;
	m_ownerID = owner->GetOwnerID();
	m_curGroup = nullptr;
	m_GUID = 0;
	m_resident = false;

	return TASK_OK;
}

/*
-------------------------
Free
-------------------------
*/
int CTaskManager::Free()
{
	assert(!m_resident); //don't free me, i'm currently running!
	//Clear out all pending tasks
	for (const auto& task : m_tasks)
	{
		task->Free();
	}

	m_tasks.clear();

	//Clear out all taskGroups
	for (auto gi = m_taskGroups.begin(); gi != m_taskGroups.end(); ++gi)
	{
		delete* gi;
	}

	m_taskGroups.clear();
	m_taskGroupNameMap.clear();
	m_taskGroupIDMap.clear();

	return TASK_OK;
}

/*
-------------------------
Flush
-------------------------
*/

int CTaskManager::Flush()
{
	//FIXME: Rewrite

	return true;
}

/*
-------------------------
AddTaskGroup
-------------------------
*/

CTaskGroup* CTaskManager::AddTaskGroup(const char* name, const CIcarus* icarus)
{
	CTaskGroup* group;

	const auto tgni = m_taskGroupNameMap.find(name);

	if (tgni != m_taskGroupNameMap.end())
	{
		group = (*tgni).second;

		//Clear it and just move on
		group->Init();

		return group;
	}

	//Allocate a new one
	group = new CTaskGroup;

	//TODO: Emit warning
	assert(group);
	if (group == nullptr)
	{
		icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Unable to allocate task group \"%s\"\n", name);
		return nullptr;
	}

	//Setup the internal information
	group->SetGUID(m_GUID++);

	//Add it to the list and associate it for retrieval later
	m_taskGroups.insert(m_taskGroups.end(), group);
	m_taskGroupNameMap[name] = group;
	m_taskGroupIDMap[group->GetGUID()] = group;

	return group;
}

/*
-------------------------
GetTaskGroup
-------------------------
*/

CTaskGroup* CTaskManager::GetTaskGroup(const char* name, const CIcarus* icarus)
{
	const auto tgi = m_taskGroupNameMap.find(name);

	if (tgi == m_taskGroupNameMap.end())
	{
		icarus->GetGame()->DebugPrint(IGameInterface::WL_WARNING, "Could not find task group \"%s\"\n", name);
		return nullptr;
	}

	return (*tgi).second;
}

CTaskGroup* CTaskManager::GetTaskGroup(const int id, const CIcarus* icarus)
{
	const auto tgi = m_taskGroupIDMap.find(id);

	if (tgi == m_taskGroupIDMap.end())
	{
		icarus->GetGame()->DebugPrint(IGameInterface::WL_WARNING, "Could not find task group \"%d\"\n", id);
		return nullptr;
	}

	return (*tgi).second;
}

/*
-------------------------
Update
-------------------------
*/

int CTaskManager::Update(CIcarus* icarus)
{
	if (icarus->GetGame()->IsFrozen(m_ownerID))
	{
		return TASK_FAILED;
	}
	m_count = 0; //Needed for runaway init
	m_resident = true;

	const int returnVal = Go(icarus);

	m_resident = false;

	return returnVal;
}

/*
-------------------------
Check
-------------------------
*/

inline bool CTaskManager::Check(const int target_id, const CBlock* block, const int member_num)
{
	if (block->GetMember(member_num)->GetID() == target_id)
		return true;

	return false;
}

/*
-------------------------
GetFloat
-------------------------
*/

int CTaskManager::GetFloat(const int entID, const CBlock* block, int& member_num, float& value, const CIcarus* icarus)
{
	//See if this is a get() command replacement
	if (Check(CIcarus::ID_GET, block, member_num))
	{
		//Update the member past the header id
		member_num++;

		//get( TYPE, NAME )
		const auto type = static_cast<int>(*static_cast<float*>(block->GetMemberData(member_num++)));
		const char* name = static_cast<char*>(block->GetMemberData(member_num++));

		//TODO: Emit warning
		if (type != CIcarus::TK_FLOAT)
		{
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR,
				"Get() call tried to return a non-FLOAT parameter!\n");
			return false;
		}

		return icarus->GetGame()->GetFloat(entID, name, &value);
	}

	//Look for a Q_flrand(0.0f, 1.0f) inline call
	if (Check(CIcarus::ID_RANDOM, block, member_num))
	{
		member_num++;

		const float min = *static_cast<float*>(block->GetMemberData(member_num++));
		const float max = *static_cast<float*>(block->GetMemberData(member_num++));

		value = icarus->GetGame()->Random(min, max);

		return true;
	}

	//Look for a tag() inline call
	if (Check(CIcarus::ID_TAG, block, member_num))
	{
		icarus->GetGame()->DebugPrint(IGameInterface::WL_WARNING,
			"Invalid use of \"tag\" inline.  Not a valid replacement for type FLOAT\n");
		return false;
	}

	const CBlockMember* bm = block->GetMember(member_num);

	if (bm->GetID() == CIcarus::TK_INT)
	{
		value = static_cast<float>(*static_cast<int*>(block->GetMemberData(member_num++)));
	}
	else if (bm->GetID() == CIcarus::TK_FLOAT)
	{
		value = *static_cast<float*>(block->GetMemberData(member_num++));
	}
	else
	{
		assert(0);
		icarus->GetGame()->DebugPrint(IGameInterface::WL_WARNING, "Unexpected value; expected type FLOAT\n");
		return false;
	}

	return true;
}

/*
-------------------------
GetVector
-------------------------
*/

int CTaskManager::GetVector(const int entID, CBlock* block, int& member_num, vec3_t& value, CIcarus* icarus)
{
	int type, i;

	//See if this is a get() command replacement
	if (Check(CIcarus::ID_GET, block, member_num))
	{
		//Update the member past the header id
		member_num++;

		//get( TYPE, NAME )
		type = static_cast<int>(*static_cast<float*>(block->GetMemberData(member_num++)));
		const char* name = static_cast<char*>(block->GetMemberData(member_num++));

		//TODO: Emit warning
		if (type != CIcarus::TK_VECTOR)
		{
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR,
				"Get() call tried to return a non-VECTOR parameter!\n");
		}

		return icarus->GetGame()->GetVector(entID, name, value);
	}

	//Look for a Q_flrand(0.0f, 1.0f) inline call
	if (Check(CIcarus::ID_RANDOM, block, member_num))
	{
		member_num++;

		const float min = *static_cast<float*>(block->GetMemberData(member_num++));
		const float max = *static_cast<float*>(block->GetMemberData(member_num++));

		for (i = 0; i < 3; i++)
		{
			value[i] = icarus->GetGame()->Random(min, max); //FIXME: Just truncating it for now.. should be fine though
		}

		return true;
	}

	//Look for a tag() inline call
	if (Check(CIcarus::ID_TAG, block, member_num))
	{
		char* tagName;
		float tagLookup;

		member_num++;
		ICARUS_VALIDATE(Get(entID, block, member_num, &tagName, icarus));
		ICARUS_VALIDATE(GetFloat(entID, block, member_num, tagLookup, icarus));

		if (icarus->GetGame()->GetTag(entID, tagName, static_cast<int>(tagLookup), value) == false)
		{
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Unable to find tag \"%s\"!\n", tagName);
			assert(0 && "Unable to find tag");
			return TASK_FAILED;
		}

		return true;
	}

	//Check for a real vector here
	type = static_cast<int>(*static_cast<float*>(block->GetMemberData(member_num)));

	if (type != CIcarus::TK_VECTOR)
	{
		//		icarus->GetGame()->DPrintf( WL_WARNING, "Unexpected value; expected type VECTOR\n" );
		return false;
	}

	member_num++;

	for (i = 0; i < 3; i++)
	{
		if (GetFloat(entID, block, member_num, value[i], icarus) == false)
			return false;
	}

	return true;
}

/*
-------------------------
Get
-------------------------
*/

int CTaskManager::GetID() const
{
	return m_id;
}

int CTaskManager::Get(const int entID, CBlock* block, int& member_num, char** value, CIcarus* icarus)
{
	static char tempBuffer[128]; //FIXME: EEEK!
	char* tagName;

	//Look for a get() inline call
	if (Check(CIcarus::ID_GET, block, member_num))
	{
		//Update the member past the header id
		member_num++;

		//get( TYPE, NAME )
		const int type = static_cast<int>(*static_cast<float*>(block->GetMemberData(member_num++)));
		const auto name = static_cast<char*>(block->GetMemberData(member_num++));

		//Format the return properly
		//FIXME: This is probably doing double formatting in certain cases...
		//FIXME: STRING MANAGEMENT NEEDS TO BE IMPLEMENTED, MY CURRENT SOLUTION IS NOT ACCEPTABLE!!
		switch (type)
		{
		case CIcarus::TK_STRING:
			if (icarus->GetGame()->GetString(entID, name, value) == false)
			{
				icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Get() parameter \"%s\" could not be found!\n",
					name);
				return false;
			}

			return true;

		case CIcarus::TK_FLOAT:
		{
			float temp;

			if (icarus->GetGame()->GetFloat(entID, name, &temp) == false)
			{
				icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR,
					"Get() parameter \"%s\" could not be found!\n", name);
				return false;
			}

			Com_sprintf(tempBuffer, sizeof tempBuffer, "%f", temp);
			*value = static_cast<char*>(tempBuffer);
		}

		return true;

		case CIcarus::TK_VECTOR:
		{
			vec3_t vval;

			if (icarus->GetGame()->GetVector(entID, name, vval) == false)
			{
				icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR,
					"Get() parameter \"%s\" could not be found!\n", name);
				return false;
			}

			Com_sprintf(tempBuffer, sizeof tempBuffer, "%f %f %f", vval[0], vval[1], vval[2]);
			*value = static_cast<char*>(tempBuffer);
		}

		return true;

		default:
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Get() call tried to return an unknown type!\n");
			return false;
		}
	}

	//Look for a Q_flrand(0.0f, 1.0f) inline call
	if (Check(CIcarus::ID_RANDOM, block, member_num))
	{
		member_num++;

		const float min = *static_cast<float*>(block->GetMemberData(member_num++));
		const float max = *static_cast<float*>(block->GetMemberData(member_num++));

		const float ret = icarus->GetGame()->Random(min, max);

		Com_sprintf(tempBuffer, sizeof tempBuffer, "%f", ret);
		*value = static_cast<char*>(tempBuffer);

		return true;
	}

	//Look for a tag() inline call
	if (Check(CIcarus::ID_TAG, block, member_num))
	{
		float tagLookup;
		vec3_t vector;
		member_num++;
		ICARUS_VALIDATE(Get(entID, block, member_num, &tagName, icarus));
		ICARUS_VALIDATE(GetFloat(entID, block, member_num, tagLookup, icarus));

		if (icarus->GetGame()->GetTag(entID, tagName, static_cast<int>(tagLookup), vector) == false)
		{
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Unable to find tag \"%s\"!\n", tagName);
			assert(0 && "Unable to find tag");
			return false;
		}

		Com_sprintf(tempBuffer, sizeof tempBuffer, "%f %f %f", vector[0], vector[1], vector[2]);
		*value = static_cast<char*>(tempBuffer);

		return true;
	}

	//Get an actual piece of data

	const CBlockMember* bm = block->GetMember(member_num);

	if (bm->GetID() == CIcarus::TK_INT)
	{
		const float fval = static_cast<float>(*static_cast<int*>(block->GetMemberData(member_num++)));
		Com_sprintf(tempBuffer, sizeof tempBuffer, "%f", fval);
		*value = static_cast<char*>(tempBuffer);

		return true;
	}
	if (bm->GetID() == CIcarus::TK_FLOAT)
	{
		const float fval = *static_cast<float*>(block->GetMemberData(member_num++));
		Com_sprintf(tempBuffer, sizeof tempBuffer, "%f", fval);
		*value = static_cast<char*>(tempBuffer);

		return true;
	}
	if (bm->GetID() == CIcarus::TK_VECTOR)
	{
		vec3_t vval;

		member_num++;

		for (float& i : vval)
		{
			if (GetFloat(entID, block, member_num, i, icarus) == false)
				return false;
		}

		Com_sprintf(tempBuffer, sizeof tempBuffer, "%f %f %f", vval[0], vval[1], vval[2]);
		*value = static_cast<char*>(tempBuffer);

		return true;
	}
	if (bm->GetID() == CIcarus::TK_STRING || bm->GetID() == CIcarus::TK_IDENTIFIER)
	{
		*value = static_cast<char*>(block->GetMemberData(member_num++));

		return true;
	}

	//TODO: Emit warning
	assert(0);
	icarus->GetGame()->DebugPrint(IGameInterface::WL_WARNING, "Unexpected value; expected type STRING\n");

	return false;
}

/*
-------------------------
Go
-------------------------
*/

int CTaskManager::Go(CIcarus* icarus)
{
	//Check for run away scripts
	if (m_count++ > RUNAWAY_LIMIT)
	{
		assert(0);
		icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Runaway loop detected!\n");
		return TASK_FAILED;
	}

	//If there are tasks to complete, do so
	if (m_tasks.empty() == false)
	{
		bool completed = false;
		//Get the next task
		CTask* task = PopTask(CSequence::POP_BACK);

		assert(task);
		if (task == nullptr)
		{
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Invalid task found in Go()!\n");
			return TASK_FAILED;
		}

		//If this hasn't been stamped, do so
		if (task->GetTimeStamp() == 0)
			task->SetTimeStamp(icarus->GetGame()->GetTime());

		//Switch and call the proper function
		switch (task->GetID())
		{
		case CIcarus::ID_WAIT:

			Wait(task, completed, icarus);

			//Push it to consider it again on the next frame if not complete
			if (completed == false)
			{
				PushTask(task, CSequence::PUSH_BACK);
				return TASK_OK;
			}

			Completed(task->GetGUID());

			break;

		case CIcarus::ID_WAITSIGNAL:

			WaitSignal(task, completed, icarus);

			//Push it to consider it again on the next frame if not complete
			if (completed == false)
			{
				PushTask(task, CSequence::PUSH_BACK);
				return TASK_OK;
			}

			Completed(task->GetGUID());

			break;

		case CIcarus::ID_PRINT: //print( STRING )
			Print(task, icarus);
			break;

		case CIcarus::ID_SOUND: //sound( name )
			Sound(task, icarus);
			break;

		case CIcarus::ID_MOVE: //move ( ORIGIN, ANGLES, DURATION )
			Move(task, icarus);
			break;

		case CIcarus::ID_ROTATE: //rotate( ANGLES, DURATION )
			Rotate(task, icarus);
			break;

		case CIcarus::ID_KILL: //kill( NAME )
			Kill(task, icarus);
			break;

		case CIcarus::ID_REMOVE: //remove( NAME )
			Remove(task, icarus);
			break;

		case CIcarus::ID_CAMERA: //camera( ? )
			Camera(task, icarus);
			break;

		case CIcarus::ID_SET: //set( NAME, ? )
			Set(task, icarus);
			break;

		case CIcarus::ID_USE: //use( NAME )
			Use(task, icarus);
			break;

		case CIcarus::ID_DECLARE: //declare( TYPE, NAME )
			DeclareVariable(task, icarus);
			break;

		case CIcarus::ID_FREE: //free( NAME )
			FreeVariable(task, icarus);
			break;

		case CIcarus::ID_SIGNAL: //signal( NAME )
			Signal(task, icarus);
			break;

		case CIcarus::ID_PLAY: //play ( NAME )
			Play(task, icarus);
			break;

		default:
			assert(0);
			task->Free();
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Found unknown task type!\n");
			return TASK_FAILED;
		}

		//Pump the sequencer for another task
		CallbackCommand(task, TASK_RETURN_COMPLETE, icarus);

		task->Free();
	}

	//FIXME: A command surge limiter could be implemented at this point to be sure a script doesn't
	//		 execute too many commands in one cycle.  This may, however, cause timing errors to surface.
	return TASK_OK;
}

/*
-------------------------
SetCommand
-------------------------
*/

int CTaskManager::SetCommand(CBlock* command, const int type, const CIcarus* icarus)
{
	CTask* task = CTask::Create(m_GUID++, command);

	//If this is part of a task group, add it in
	if (m_curGroup)
	{
		m_curGroup->Add(task);
	}

	//TODO: Emit warning
	assert(task);
	if (task == nullptr)
	{
		icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Unable to allocate new task!\n");
		return TASK_FAILED;
	}

	PushTask(task, type);

	return TASK_OK;
}

/*
-------------------------
MarkTask
-------------------------
*/

int CTaskManager::MarkTask(const int id, const int operation, const CIcarus* icarus)
{
	CTaskGroup* group = GetTaskGroup(id, icarus);

	assert(group);

	if (group == nullptr)
		return TASK_FAILED;

	if (operation == TASK_START)
	{
		//Reset all the completion information
		group->Init();

		group->SetParent(m_curGroup);
		m_curGroup = group;
	}
	else if (operation == TASK_END)
	{
		assert(m_curGroup);
		if (m_curGroup == nullptr)
			return TASK_FAILED;

		m_curGroup = m_curGroup->GetParent();
	}

#ifdef _DEBUG
	else
	{
		assert(0);
	}
#endif

	return TASK_OK;
}

/*
-------------------------
Completed
-------------------------
*/

int CTaskManager::Completed(const int id) const
{
	//Mark the task as completed
	for (const auto& taskGroup : m_taskGroups)
	{
		//If this returns true, then the task was marked properly
		if (taskGroup->MarkTaskComplete(id))
			break;
	}

	return TASK_OK;
}

/*
-------------------------
CallbackCommand
-------------------------
*/

int CTaskManager::CallbackCommand(const CTask* task, const int returnCode, CIcarus* icarus)
{
	if (m_owner->Callback(this, task->GetBlock(), returnCode, icarus) == CSequencer::SEQ_OK)
		return Go(icarus);

	assert(0);

	icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Command callback failure!\n");
	return TASK_FAILED;
}

/*
-------------------------
RecallTask
-------------------------
*/

CBlock* CTaskManager::RecallTask()
{
	const CTask* task = PopTask(CSequence::POP_BACK);

	if (task)
	{
		// fixed 2/12/2 to free the task that has been popped (called from sequencer Recall)
		CBlock* retBlock = task->GetBlock();
		task->Free();

		return retBlock;
		//	return task->GetBlock();
	}

	return nullptr;
}

/*
-------------------------
PushTask
-------------------------
*/

int CTaskManager::PushTask(CTask* task, const int flag)
{
	assert(flag == CSequence::PUSH_FRONT || flag == CSequence::PUSH_BACK);

	switch (flag)
	{
	case CSequence::PUSH_FRONT:
		m_tasks.insert(m_tasks.begin(), task);

		return TASK_OK;

	case CSequence::PUSH_BACK:
		m_tasks.insert(m_tasks.end(), task);

		return TASK_OK;
	default:;
	}

	//Invalid flag
	return CSequencer::SEQ_FAILED;
}

/*
-------------------------
PopTask
-------------------------
*/

CTask* CTaskManager::PopTask(const int flag)
{
	CTask* task;

	assert(flag == CSequence::POP_FRONT || flag == CSequence::POP_BACK);

	if (m_tasks.empty())
		return nullptr;

	switch (flag)
	{
	case CSequence::POP_FRONT:
		task = m_tasks.front();
		m_tasks.pop_front();

		return task;

	case CSequence::POP_BACK:
		task = m_tasks.back();
		m_tasks.pop_back();

		return task;
	default:;
	}

	//Invalid flag
	return nullptr;
}

/*
-------------------------
GetCurrentTask
-------------------------
*/

CBlock* CTaskManager::GetCurrentTask()
{
	const CTask* task = PopTask(CSequence::POP_BACK);

	if (task == nullptr)
		return nullptr;
	// fixed 2/12/2 to free the task that has been popped (called from sequencer Interrupt)
	CBlock* retBlock = task->GetBlock();
	task->Free();

	return retBlock;
	//	return task->GetBlock();
}

/*
=================================================

  Task Functions

=================================================
*/

int CTaskManager::Wait(const CTask* task, bool& completed, CIcarus* icarus)
{
	CBlock* block = task->GetBlock();
	char* sVal;
	float dwtime;
	int memberNum = 0;

	completed = false;

	CBlockMember* bm = block->GetMember(0);

	//Check if this is a task completion wait
	if (bm->GetID() == CIcarus::TK_STRING)
	{
		ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));

		if (task->GetTimeStamp() == icarus->GetGame()->GetTime())
		{
			//Print out the debug info
			icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d wait(\"%s\"); [%d]", m_ownerID, sVal,
				task->GetTimeStamp());
		}

		const CTaskGroup* group = GetTaskGroup(sVal, icarus);

		if (group == nullptr)
		{
			//TODO: Emit warning
			completed = false;
			return TASK_FAILED;
		}

		completed = group->Complete();
	}
	else //Otherwise it's a time completion wait
	{
		if (Check(CIcarus::ID_RANDOM, block, memberNum))
		{
			//get it random only the first time

			dwtime = *static_cast<float*>(block->GetMemberData(memberNum++));
			if (dwtime == icarus->GetGame()->MaxFloat())
			{
				//we have not evaluated this random yet
				const float min = *static_cast<float*>(block->GetMemberData(memberNum++));
				const float max = *static_cast<float*>(block->GetMemberData(memberNum++));

				dwtime = icarus->GetGame()->Random(min, max);

				//store the result in the first member
				bm->SetData(&dwtime, sizeof dwtime, icarus);
			}
		}
		else
		{
			ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, dwtime, icarus));
		}

		if (task->GetTimeStamp() == icarus->GetGame()->GetTime())
		{
			//Print out the debug info
			icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d wait( %d ); [%d]", m_ownerID,
				static_cast<int>(dwtime), task->GetTimeStamp());
		}

		if (task->GetTimeStamp() + dwtime < icarus->GetGame()->GetTime())
		{
			completed = true;
			memberNum = 0;
			if (Check(CIcarus::ID_RANDOM, block, memberNum))
			{
				//set the data back to 0 so it will be re-randomized next time
				dwtime = icarus->GetGame()->MaxFloat();
				bm->SetData(&dwtime, sizeof dwtime, icarus);
			}
		}
	}

	return TASK_OK;
}

/*
-------------------------
WaitSignal
-------------------------
*/

int CTaskManager::WaitSignal(const CTask* task, bool& completed, CIcarus* icarus) const
{
	CBlock* block = task->GetBlock();
	char* sVal;
	int memberNum = 0;

	completed = false;

	ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));

	if (task->GetTimeStamp() == icarus->GetGame()->GetTime())
	{
		//Print out the debug info
		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d waitsignal(\"%s\"); [%d]", m_ownerID, sVal,
			task->GetTimeStamp());
	}

	if (icarus->CheckSignal(sVal))
	{
		completed = true;
		icarus->ClearSignal(sVal);
	}

	return TASK_OK;
}

/*
-------------------------
Print
-------------------------
*/

int CTaskManager::Print(const CTask* task, CIcarus* icarus) const
{
	CBlock* block = task->GetBlock();
	char* s_val;
	int member_num = 0;

	ICARUS_VALIDATE(Get(m_ownerID, block, member_num, &s_val, icarus));

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d print(\"%s\"); [%d]", m_ownerID, s_val,
		task->GetTimeStamp());

	icarus->GetGame()->CenterPrint(s_val);

	return Completed(task->GetGUID());
}

/*
-------------------------
Sound
-------------------------
*/

int CTaskManager::Sound(const CTask* task, CIcarus* icarus) const
{
	CBlock* block = task->GetBlock();
	char* sVal, * sVal2;
	int memberNum = 0;

	ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));
	ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal2, icarus));

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, R"(%4d sound("%s", "%s"); [%d])", m_ownerID, sVal, sVal2,
		task->GetTimeStamp());

	//Only instantly complete if the user has requested it
	if (icarus->GetGame()->PlayIcarusSound(task->GetGUID(), m_ownerID, sVal2, sVal))
		return Completed(task->GetGUID());

	return TASK_OK;
}

/*
-------------------------
Rotate
-------------------------
*/

int CTaskManager::Rotate(const CTask* task, CIcarus* icarus) const
{
	vec3_t vector;
	CBlock* block = task->GetBlock();
	char* tagName;
	float duration;
	int memberNum = 0;

	//Check for a tag reference
	if (Check(CIcarus::ID_TAG, block, memberNum))
	{
		float tagLookup;
		memberNum++;

		ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &tagName, icarus));
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, tagLookup, icarus));

		if (icarus->GetGame()->GetTag(m_ownerID, tagName, static_cast<int>(tagLookup), vector) == false)
		{
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Unable to find tag \"%s\"!\n", tagName);
			assert(0);
			return TASK_FAILED;
		}
	}
	else
	{
		//Get a normal vector
		ICARUS_VALIDATE(GetVector(m_ownerID, block, memberNum, vector, icarus));
	}

	//Find the duration
	ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, duration, icarus));

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d rotate( <%f,%f,%f>, %d); [%d]", m_ownerID, vector[0],
		vector[1], vector[2], static_cast<int>(duration), task->GetTimeStamp());
	icarus->GetGame()->Lerp2Angles(task->GetGUID(), m_ownerID, vector, duration);

	return TASK_OK;
}

/*
-------------------------
Remove
-------------------------
*/

int CTaskManager::Remove(const CTask* task, CIcarus* icarus) const
{
	CBlock* block = task->GetBlock();
	char* sVal;
	int memberNum = 0;

	ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d remove(\"%s\"); [%d]", m_ownerID, sVal,
		task->GetTimeStamp());
	icarus->GetGame()->Remove(m_ownerID, sVal);

	return Completed(task->GetGUID());
}

/*
-------------------------
Camera
-------------------------
*/

int CTaskManager::Camera(const CTask* task, CIcarus* icarus) const
{
	CBlock* block = task->GetBlock();
	vec3_t vector, vector2;
	float type, fVal, fVal2, fVal3;
	char* sVal;
	int memberNum = 0;

	//Get the camera function type
	ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, type, icarus));

	switch (static_cast<int>(type))
	{
	case CIcarus::TYPE_PAN:

		ICARUS_VALIDATE(GetVector(m_ownerID, block, memberNum, vector, icarus));
		ICARUS_VALIDATE(GetVector(m_ownerID, block, memberNum, vector2, icarus));

		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal, icarus));

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( PAN, <%f %f %f>, <%f %f %f>, %f); [%d]",
			m_ownerID, vector[0], vector[1], vector[2], vector2[0], vector2[1], vector2[2],
			fVal, task->GetTimeStamp());
		icarus->GetGame()->CameraPan(vector, vector2, fVal);
		break;

	case CIcarus::TYPE_ZOOM:

		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal, icarus));
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal2, icarus));

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( ZOOM, %f, %f); [%d]", m_ownerID, fVal,
			fVal2, task->GetTimeStamp());
		icarus->GetGame()->CameraZoom(fVal, fVal2);
		break;

	case CIcarus::TYPE_MOVE:

		ICARUS_VALIDATE(GetVector(m_ownerID, block, memberNum, vector, icarus));
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal, icarus));

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( MOVE, <%f %f %f>, %f); [%d]", m_ownerID,
			vector[0], vector[1], vector[2], fVal, task->GetTimeStamp());
		icarus->GetGame()->CameraMove(vector, fVal);
		break;

	case CIcarus::TYPE_ROLL:

		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal, icarus));
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal2, icarus));

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( ROLL, %f, %f); [%d]", m_ownerID, fVal,
			fVal2, task->GetTimeStamp());
		icarus->GetGame()->CameraRoll(fVal, fVal2);

		break;

	case CIcarus::TYPE_FOLLOW:

		ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal, icarus));
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal2, icarus));

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( FOLLOW, \"%s\", %f, %f); [%d]", m_ownerID,
			sVal, fVal, fVal2, task->GetTimeStamp());
		icarus->GetGame()->CameraFollow(sVal, fVal, fVal2);

		break;

	case CIcarus::TYPE_TRACK:

		ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal, icarus));
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal2, icarus));

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( TRACK, \"%s\", %f, %f); [%d]", m_ownerID,
			sVal, fVal, fVal2, task->GetTimeStamp());
		icarus->GetGame()->CameraTrack(sVal, fVal, fVal2);
		break;

	case CIcarus::TYPE_DISTANCE:

		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal, icarus));
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal2, icarus));

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( DISTANCE, %f, %f); [%d]", m_ownerID, fVal,
			fVal2, task->GetTimeStamp());
		icarus->GetGame()->CameraDistance(fVal, fVal2);
		break;

	case CIcarus::TYPE_FADE:

		ICARUS_VALIDATE(GetVector(m_ownerID, block, memberNum, vector, icarus));
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal, icarus));

		ICARUS_VALIDATE(GetVector(m_ownerID, block, memberNum, vector2, icarus));
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal2, icarus));

		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal3, icarus));

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG,
			"%4d camera( FADE, <%f %f %f>, %f, <%f %f %f>, %f, %f); [%d]", m_ownerID,
			vector[0], vector[1], vector[2], fVal, vector2[0], vector2[1], vector2[2], fVal2,
			fVal3, task->GetTimeStamp());
		icarus->GetGame()->CameraFade(vector[0], vector[1], vector[2], fVal, vector2[0], vector2[1], vector2[2], fVal2,
			fVal3);
		break;

	case CIcarus::TYPE_PATH:
		ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( PATH, \"%s\"); [%d]", m_ownerID, sVal,
			task->GetTimeStamp());
		icarus->GetGame()->CameraPath(sVal);
		break;

	case CIcarus::TYPE_ENABLE:
		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( ENABLE ); [%d]", m_ownerID,
			task->GetTimeStamp());
		icarus->GetGame()->CameraEnable();
		break;

	case CIcarus::TYPE_DISABLE:
		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( DISABLE ); [%d]", m_ownerID,
			task->GetTimeStamp());
		icarus->GetGame()->CameraDisable();
		break;

	case CIcarus::TYPE_SHAKE:
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal, icarus));
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal2, icarus));

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( SHAKE, %f, %f ); [%d]", m_ownerID, fVal,
			fVal2, task->GetTimeStamp());
		icarus->GetGame()->CameraShake(fVal, static_cast<int>(fVal2));
		break;
	default:;
	}

	return Completed(task->GetGUID());
}

/*
-------------------------
Move
-------------------------
*/

int CTaskManager::Move(const CTask* task, CIcarus* icarus) const
{
	vec3_t vector, vector2;
	CBlock* block = task->GetBlock();
	float duration;
	int memberNum = 0;

	//Get the goal position
	ICARUS_VALIDATE(GetVector(m_ownerID, block, memberNum, vector, icarus));

	//Check for possible angles field
	if (GetVector(m_ownerID, block, memberNum, vector2, icarus) == false)
	{
		ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, duration, icarus));

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d move( <%f %f %f>, %f ); [%d]", m_ownerID,
			vector[0], vector[1], vector[2], duration, task->GetTimeStamp());
		icarus->GetGame()->Lerp2Pos(task->GetGUID(), m_ownerID, vector, nullptr, duration);

		return TASK_OK;
	}

	//Get the duration and make the call
	ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, duration, icarus));

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d move( <%f %f %f>, <%f %f %f>, %f ); [%d]", m_ownerID,
		vector[0], vector[1], vector[2], vector2[0], vector2[1], vector2[2], duration,
		task->GetTimeStamp());
	icarus->GetGame()->Lerp2Pos(task->GetGUID(), m_ownerID, vector, vector2, duration);

	return TASK_OK;
}

/*
-------------------------
Kill
-------------------------
*/

int CTaskManager::Kill(const CTask* task, CIcarus* icarus) const
{
	CBlock* block = task->GetBlock();
	char* sVal;
	int memberNum = 0;

	ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d kill( \"%s\" ); [%d]", m_ownerID, sVal,
		task->GetTimeStamp());
	icarus->GetGame()->Kill(m_ownerID, sVal);

	return Completed(task->GetGUID());
}

/*
-------------------------
Set
-------------------------
*/

int CTaskManager::Set(const CTask* task, CIcarus* icarus) const
{
	CBlock* block = task->GetBlock();
	char* sVal, * sVal2;
	int memberNum = 0;

	ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));
	ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal2, icarus));

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, R"(%4d set( "%s", "%s" ); [%d])", m_ownerID, sVal, sVal2,
		task->GetTimeStamp());
	icarus->GetGame()->Set(task->GetGUID(), m_ownerID, sVal, sVal2);

	return TASK_OK;
}

/*
-------------------------
Use
-------------------------
*/

int CTaskManager::Use(const CTask* task, CIcarus* icarus) const
{
	CBlock* block = task->GetBlock();
	char* sVal;
	int memberNum = 0;

	ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d use( \"%s\" ); [%d]", m_ownerID, sVal,
		task->GetTimeStamp());
	icarus->GetGame()->Use(m_ownerID, sVal);

	return Completed(task->GetGUID());
}

/*
-------------------------
DeclareVariable
-------------------------
*/

int CTaskManager::DeclareVariable(const CTask* task, CIcarus* icarus) const
{
	CBlock* block = task->GetBlock();
	char* sVal;
	int memberNum = 0;
	float fVal;

	ICARUS_VALIDATE(GetFloat(m_ownerID, block, memberNum, fVal, icarus));
	ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d declare( %d, \"%s\" ); [%d]", m_ownerID,
		static_cast<int>(fVal), sVal, task->GetTimeStamp());
	icarus->GetGame()->DeclareVariable(static_cast<int>(fVal), sVal);

	return Completed(task->GetGUID());
}

/*
-------------------------
FreeVariable
-------------------------
*/

int CTaskManager::FreeVariable(const CTask* task, CIcarus* icarus) const
{
	CBlock* block = task->GetBlock();
	char* sVal;
	int memberNum = 0;

	ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d free( \"%s\" ); [%d]", m_ownerID, sVal,
		task->GetTimeStamp());
	icarus->GetGame()->FreeVariable(sVal);

	return Completed(task->GetGUID());
}

/*
-------------------------
Signal
-------------------------
*/

int CTaskManager::Signal(const CTask* task, CIcarus* icarus) const
{
	CBlock* block = task->GetBlock();
	char* sVal;
	int memberNum = 0;

	ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d signal( \"%s\" ); [%d]", m_ownerID, sVal,
		task->GetTimeStamp());
	icarus->Signal(sVal);

	return Completed(task->GetGUID());
}

/*
-------------------------
Play
-------------------------
*/

int CTaskManager::Play(const CTask* task, CIcarus* icarus) const
{
	CBlock* block = task->GetBlock();
	char* sVal, * sVal2;
	int memberNum = 0;

	ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal, icarus));
	ICARUS_VALIDATE(Get(m_ownerID, block, memberNum, &sVal2, icarus));

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, R"(%4d play( "%s", "%s" ); [%d])", m_ownerID, sVal, sVal2,
		task->GetTimeStamp());
	icarus->GetGame()->Play(task->GetGUID(), m_ownerID, sVal, sVal2);

	return TASK_OK;
}

/*
-------------------------
SaveCommand
-------------------------
*/

//FIXME: ARGH!  This is duplicated from CSequence because I can't directly link it any other way...

int CTaskManager::SaveCommand(const CBlock* block)
{
	const auto pIcarus = static_cast<CIcarus*>(IIcarusInterface::GetIcarus());

	unsigned char flags;
	int numMembers, bID, size;

	//Save out the block ID
	bID = block->GetBlockID();
	pIcarus->BufferWrite(&bID, sizeof bID);

	//Save out the block's flags
	flags = block->GetFlags();
	pIcarus->BufferWrite(&flags, sizeof flags);

	//Save out the number of members to read
	numMembers = block->GetNumMembers();
	pIcarus->BufferWrite(&numMembers, sizeof numMembers);

	for (int i = 0; i < numMembers; i++)
	{
		const CBlockMember* bm = block->GetMember(i);

		//Save the block id
		bID = bm->GetID();
		pIcarus->BufferWrite(&bID, sizeof bID);

		//Save out the data size
		size = bm->GetSize();
		pIcarus->BufferWrite(&size, sizeof size);

		//Save out the raw data
		pIcarus->BufferWrite(bm->GetData(), size);
	}

	return true;
}

/*
-------------------------
Save
-------------------------
*/

void CTaskManager::Save()
{
	unsigned int timeStamp;
	bool completed;
	int id, numCommands;

	// Data saved here.
	//	Taskmanager GUID.
	//	Number of Tasks.
	//	Tasks:
	//				- GUID.
	//				- Timestamp.
	//				- Block/Command.
	//	Number of task groups.
	//	Task groups ID's.
	//	Task groups (data).
	//				- Parent.
	//				- Number of Commands.
	//				- Commands:
	//						+ ID.
	//						+ State of Completion.
	//				- Number of Completed Commands.
	//	Currently active group.
	//	Task group names:
	//				- String Size.
	//				- String.
	//				- ID.

	const auto pIcarus = static_cast<CIcarus*>(IIcarusInterface::GetIcarus());

	//Save the taskmanager's GUID
	pIcarus->BufferWrite(&m_GUID, sizeof m_GUID);

	//Save out the number of tasks that will follow
	const int i_num_tasks = m_tasks.size();
	pIcarus->BufferWrite(&i_num_tasks, sizeof i_num_tasks);

	//Save out all the tasks
	tasks_l::iterator ti;

	STL_ITERATE(ti, m_tasks)
	{
		//Save the GUID
		id = (*ti)->GetGUID();
		pIcarus->BufferWrite(&id, sizeof id);

		//Save the timeStamp (FIXME: Although, this is going to be worthless if time is not consistent...)
		timeStamp = (*ti)->GetTimeStamp();
		pIcarus->BufferWrite(&timeStamp, sizeof timeStamp);

		//Save out the block
		const CBlock* block = (*ti)->GetBlock();
		SaveCommand(block);
	}

	//Save out the number of task groups
	const int num_task_groups = m_taskGroups.size();
	pIcarus->BufferWrite(&num_task_groups, sizeof num_task_groups);

	//Save out the IDs of all the task groups
	int numWritten = 0;
	taskGroup_v::iterator tgi;
	STL_ITERATE(tgi, m_taskGroups)
	{
		id = (*tgi)->GetGUID();
		pIcarus->BufferWrite(&id, sizeof id);
		numWritten++;
	}
	assert(numWritten == num_task_groups);

	//Save out the task groups
	numWritten = 0;
	STL_ITERATE(tgi, m_taskGroups)
	{
		//Save out the parent
		id = (*tgi)->GetParent() == nullptr ? -1 : (*tgi)->GetParent()->GetGUID();
		pIcarus->BufferWrite(&id, sizeof id);

		//Save out the number of commands
		numCommands = (*tgi)->m_completedTasks.size();
		pIcarus->BufferWrite(&numCommands, sizeof numCommands);

		//Save out the command map
		CTaskGroup::taskCallback_m::iterator tci;

		STL_ITERATE(tci, (*tgi)->m_completedTasks)
		{
			//Write out the ID
			id = (*tci).first;
			pIcarus->BufferWrite(&id, sizeof id);

			//Write out the state of completion
			completed = (*tci).second;
			pIcarus->BufferWrite(&completed, sizeof completed);
		}

		//Save out the number of completed commands
		id = (*tgi)->m_numCompleted;
		pIcarus->BufferWrite(&id, sizeof id);
		numWritten++;
	}
	assert(numWritten == num_task_groups);

	//Only bother if we've got tasks present
	if (m_taskGroups.size())
	{
		//Save out the currently active group
		const int cur_group_id = m_curGroup == nullptr ? -1 : m_curGroup->GetGUID();
		pIcarus->BufferWrite(&cur_group_id, sizeof cur_group_id);
	}

	//Save out the task group name maps
	taskGroupName_m::iterator tmi;
	numWritten = 0;
	STL_ITERATE(tmi, m_taskGroupNameMap)
	{
		const char* name = (*tmi).first.c_str();

		//Make sure this is a valid string
		assert(name != nullptr && name[0] != '\0');

		int length = strlen(name) + 1;

		//Save out the string size
		//icarus->GetGame()->WriteSaveData( 'TGNL', &length, sizeof ( length ) );
		pIcarus->BufferWrite(&length, sizeof length);

		//Write out the string
		pIcarus->BufferWrite(name, length);

		const CTaskGroup* taskGroup = (*tmi).second;

		id = taskGroup->GetGUID();

		//Write out the ID
		pIcarus->BufferWrite(&id, sizeof id);
		numWritten++;
	}
	assert(numWritten == num_task_groups);
}

/*
-------------------------
Load
-------------------------
*/

void CTaskManager::Load(CIcarus* icarus)
{
	unsigned char flags;
	CTaskGroup* taskGroup;
	unsigned int timeStamp;
	bool completed;
	void* bData;
	int id, numTasks, numMembers;
	int bID, bSize;

	// Data expected/loaded here.
	//	Taskmanager GUID.
	//	Number of Tasks.
	//	Tasks:
	//				- GUID.
	//				- Timestamp.
	//				- Block/Command.
	//	Number of task groups.
	//	Task groups ID's.
	//	Task groups (data).
	//				- Parent.
	//				- Number of Commands.
	//				- Commands:
	//						+ ID.
	//						+ State of Completion.
	//				- Number of Completed Commands.
	//	Currently active group.
	//	Task group names:
	//				- String Size.
	//				- String.
	//				- ID.

	const auto pIcarus = static_cast<CIcarus*>(IIcarusInterface::GetIcarus());

	//Get the GUID
	pIcarus->BufferRead(&m_GUID, sizeof m_GUID);

	//Get the number of tasks to follow
	pIcarus->BufferRead(&numTasks, sizeof numTasks);

	//Reload all the tasks
	for (int i = 0; i < numTasks; i++)
	{
		auto task = new CTask;

		assert(task);

		//Get the GUID
		pIcarus->BufferRead(&id, sizeof id);
		task->SetGUID(id);

		//Get the time stamp
		pIcarus->BufferRead(&timeStamp, sizeof timeStamp);
		task->SetTimeStamp(timeStamp);

		//
		// BLOCK LOADING
		//

		//Get the block ID and create a new container
		pIcarus->BufferRead(&id, sizeof id);
		const auto block = new CBlock;

		block->Create(id);

		//Read the block's flags
		pIcarus->BufferRead(&flags, sizeof flags);
		block->SetFlags(flags);

		//Get the number of block members
		pIcarus->BufferRead(&numMembers, sizeof numMembers);

		for (int j = 0; j < numMembers; j++)
		{
			//Get the member ID
			pIcarus->BufferRead(&bID, sizeof bID);

			//Get the member size
			pIcarus->BufferRead(&bSize, sizeof bSize);

			//Get the member's data
			if ((bData = icarus->GetGame()->Malloc(bSize)) == nullptr)
			{
				assert(0);
				return;
			}

			//Get the actual raw data
			pIcarus->BufferRead(bData, bSize);

			//Write out the correct type
			switch (bID)
			{
			case CIcarus::TK_FLOAT:
				block->Write(CIcarus::TK_FLOAT, *static_cast<float*>(bData), icarus);
				break;

			case CIcarus::TK_IDENTIFIER:
				block->Write(CIcarus::TK_IDENTIFIER, static_cast<char*>(bData), icarus);
				break;

			case CIcarus::TK_STRING:
				block->Write(CIcarus::TK_STRING, static_cast<char*>(bData), icarus);
				break;

			case CIcarus::TK_VECTOR:
				block->Write(CIcarus::TK_VECTOR, *static_cast<vec3_t*>(bData), icarus);
				break;

			case CIcarus::ID_RANDOM:
				block->Write(CIcarus::ID_RANDOM, *static_cast<float*>(bData), icarus); //ID_RANDOM );
				break;

			case CIcarus::ID_TAG:
				block->Write(CIcarus::ID_TAG, static_cast<float>(CIcarus::ID_TAG), icarus);
				break;

			case CIcarus::ID_GET:
				block->Write(CIcarus::ID_GET, static_cast<float>(CIcarus::ID_GET), icarus);
				break;

			default:
				icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Invalid Block id %d\n", bID);
				assert(0);
				break;
			}

			//Get rid of the temp memory
			icarus->GetGame()->Free(bData);
		}

		task->SetBlock(block);

		STL_INSERT(m_tasks, task);
	}

	//Load the task groups
	int numTaskGroups;

	//icarus->GetGame()->ReadSaveData( 'TG#G', &numTaskGroups, sizeof( numTaskGroups ) );
	pIcarus->BufferRead(&numTaskGroups, sizeof numTaskGroups);

	if (numTaskGroups == 0)
		return;

	const auto taskIDs = new int[numTaskGroups];

	//Get the task group IDs
	for (int i = 0; i < numTaskGroups; i++)
	{
		//Creat a new task group
		taskGroup = new CTaskGroup;
		assert(taskGroup);

		//Get this task group's ID
		pIcarus->BufferRead(&taskIDs[i], sizeof taskIDs[i]);
		taskGroup->m_GUID = taskIDs[i];

		m_taskGroupIDMap[taskIDs[i]] = taskGroup;

		STL_INSERT(m_taskGroups, taskGroup);
	}

	//Recreate and load the task groups
	for (int i = 0; i < numTaskGroups; i++)
	{
		taskGroup = GetTaskGroup(taskIDs[i], icarus);
		assert(taskGroup);

		//Load the parent ID
		pIcarus->BufferRead(&id, sizeof id);

		if (id != -1)
			taskGroup->m_parent = GetTaskGroup(id, icarus) != nullptr ? GetTaskGroup(id, icarus) : nullptr;

		//Get the number of commands in this group
		pIcarus->BufferRead(&numMembers, sizeof numMembers);

		//Get each command and its completion state
		for (int j = 0; j < numMembers; j++)
		{
			//Get the ID
			pIcarus->BufferRead(&id, sizeof id);

			//Write out the state of completion
			pIcarus->BufferRead(&completed, sizeof completed);

			//Save it out
			taskGroup->m_completedTasks[id] = completed;
		}

		//Get the number of completed tasks
		pIcarus->BufferRead(&taskGroup->m_numCompleted, sizeof taskGroup->m_numCompleted);
	}

	//Reload the currently active group
	int curGroupID;

	pIcarus->BufferRead(&curGroupID, sizeof curGroupID);

	//Reload the map entries
	for (int i = 0; i < numTaskGroups; i++)
	{
		char name[1024];
		int length;

		//Get the size of the string
		pIcarus->BufferRead(&length, sizeof length);

		//Get the string
		pIcarus->BufferRead(&name, length);

		//Get the id
		pIcarus->BufferRead(&id, sizeof id);

		taskGroup = GetTaskGroup(id, icarus);
		assert(taskGroup);

		m_taskGroupNameMap[name] = taskGroup;
		m_taskGroupIDMap[taskGroup->GetGUID()] = taskGroup;
	}

	m_curGroup = curGroupID == -1 ? nullptr : m_taskGroupIDMap[curGroupID];

	delete[] taskIDs;
}