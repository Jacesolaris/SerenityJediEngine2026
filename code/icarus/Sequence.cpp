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

// Script Command Sequences
//
//	-- jweier

// this include must remain at the top of every Icarus CPP file
#include "StdAfx.h"
#include "IcarusImplementation.h"

#include "blockstream.h"
#include "sequence.h"

#define STL_ITERATE( a, b )		for ( a = b.begin(); a != b.end(); ++a )
#define STL_INSERT( a, b )		a.insert( a.end(), b );

inline CSequence::CSequence() : m_id(0)
{
	m_numCommands = 0;
	//	m_numChildren	= 0;
	m_flags = 0;
	m_iterations = 1;

	m_parent = nullptr;
	m_return = nullptr;
}

CSequence::~CSequence()
{
	assert(!m_commands.size());
}

/*
-------------------------
Create
-------------------------
*/

CSequence* CSequence::Create()
{
	const auto seq = new CSequence;
	assert(seq);
	if (seq == nullptr)
		return nullptr;

	seq->SetFlag(SQ_COMMON);

	return seq;
}

/*
-------------------------
Delete
-------------------------
*/

void CSequence::Delete(const CIcarus* icarus)
{
	//Notify the parent of the deletion
	if (m_parent)
	{
		m_parent->RemoveChild(this);
	}

	//Clear all children
	if (m_children.size() > 0)
	{
		for (const auto& si : m_children)
		{
			si->SetParent(nullptr);
		}
	}
	m_children.clear();
	//m_childrenMap.clear();

	//Clear all held commands
	for (const auto& command : m_commands)
	{
		command->Free(icarus);
	}

	m_commands.clear();
}

/*
-------------------------
AddChild
-------------------------
*/

void CSequence::AddChild(CSequence* child)
{
	assert(child);
	if (child == nullptr)
		return;

	m_children.insert(m_children.end(), child);
	//	m_childrenMap[ m_numChildren ] = child;
	//	m_numChildren++;
}

/*
-------------------------
RemoveChild
-------------------------
*/

void CSequence::RemoveChild(CSequence* child)
{
	assert(child);
	if (child == nullptr)
		return;

	m_children.remove(child);

	//Remove the child
	/*	sequenceID_m::iterator iterSeq = m_childrenMap.find( child->GetID() );
		if ( iterSeq != m_childrenMap.end() )
		{
			m_childrenMap.erase( iterSeq );
		}

		m_numChildren--;*/
}

/*
-------------------------
HasChild
-------------------------
*/

bool CSequence::HasChild(CSequence* sequence) const
{
	for (const auto& ci : m_children)
	{
		if (ci == sequence)
			return true;

		if (ci->HasChild(sequence))
			return true;
	}

	return false;
}

/*
-------------------------
SetParent
-------------------------
*/

void CSequence::SetParent(CSequence* parent)
{
	m_parent = parent;

	if (parent == nullptr)
		return;

	//Inherit the parent's properties (this avoids messy tree walks later on)
	if (parent->m_flags & SQ_RETAIN)
		m_flags |= SQ_RETAIN;

	if (parent->m_flags & SQ_PENDING)
		m_flags |= SQ_PENDING;
}

/*
-------------------------
PopCommand
-------------------------
*/

CBlock* CSequence::PopCommand(const int type)
{
	CBlock* command;

	//Make sure everything is ok
	assert(type == POP_FRONT || type == POP_BACK);

	if (m_commands.empty())
		return nullptr;

	switch (type)
	{
	case POP_FRONT:

		command = m_commands.front();
		m_commands.pop_front();
		m_numCommands--;

		return command;

	case POP_BACK:

		command = m_commands.back();
		m_commands.pop_back();
		m_numCommands--;

		return command;
	default:;
	}

	//Invalid flag
	return nullptr;
}

/*
-------------------------
PushCommand
-------------------------
*/

int CSequence::PushCommand(CBlock* block, const int type)
{
	//Make sure everything is ok
	assert(type == PUSH_FRONT || type == PUSH_BACK);
	assert(block);

	switch (type)
	{
	case PUSH_FRONT:

		m_commands.push_front(block);
		m_numCommands++;

		return true;

	case PUSH_BACK:

		m_commands.push_back(block);
		m_numCommands++;

		return true;
	default:;
	}

	//Invalid flag
	return false;
}

/*
-------------------------
SetFlag
-------------------------
*/

void CSequence::SetFlag(const int flag)
{
	m_flags |= flag;
}

/*
-------------------------
RemoveFlag
-------------------------
*/

void CSequence::RemoveFlag(const int flag, const bool children)
{
	m_flags &= ~flag;

	if (children)
	{
		for (const auto& si : m_children)
		{
			si->RemoveFlag(flag, true);
		}
	}
}

/*
-------------------------
HasFlag
-------------------------
*/

int CSequence::HasFlag(const int flag) const
{
	return m_flags & flag;
}

/*
-------------------------
SetReturn
-------------------------
*/

void CSequence::SetReturn(CSequence* sequence)
{
	assert(sequence != this);
	m_return = sequence;
}

/*
-------------------------
GetChildByID
-------------------------
*/

CSequence* CSequence::GetChildByID(const int id)
{
	if (id < 0)
		return nullptr;

	//NOTENOTE: Done for safety reasons, I don't know what this template will return on underflow ( sigh... )
	/*	sequenceID_m::iterator mi = m_childrenMap.find( id );

		if ( mi == m_childrenMap.end() )
			return NULL;

		return (*mi).second;*/

	sequence_l::iterator iterSeq;
	STL_ITERATE(iterSeq, m_children)
	{
		if ((*iterSeq)->GetID() == id)
			return *iterSeq;
	}

	return nullptr;
}

/*
-------------------------
GetChildByIndex
-------------------------
*/

CSequence* CSequence::GetChildByIndex(const int i_index)
{
	if (i_index < 0 || i_index >= static_cast<int>(m_children.size()))
		return nullptr;

	auto iterSeq = m_children.begin();
	for (int i = 0; i < i_index; i++)
	{
		++iterSeq;
	}
	return *iterSeq;
}

/*
-------------------------
SaveCommand
-------------------------
*/

int CSequence::SaveCommand(const CBlock* block)
{
	const auto pIcarus = static_cast<CIcarus*>(IIcarusInterface::GetIcarus());

	unsigned char flags;
	int numMembers, bID, size;

	// Data saved here (IBLK):
	//	Block ID.
	//	Block Flags.
	//	Number of Block Members.
	//	Block Members:
	//				- Block Member ID.
	//				- Block Data Size.
	//				- Block (Raw) Data.

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

int CSequence::LoadCommand(CBlock* block, CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	int bID, bSize;
	void* bData;
	unsigned char flags;
	int id, numMembers;

	// Data expected/loaded here (IBLK) (with the size as : 'IBSZ' ).
	//	Block ID.
	//	Block Flags.
	//	Number of Block Members.
	//	Block Members:
	//				- Block Member ID.
	//				- Block Data Size.
	//				- Block (Raw) Data.

	//Get the block ID.
	icarus->BufferRead(&id, sizeof id);
	block->Create(id);

	//Read the block's flags
	icarus->BufferRead(&flags, sizeof flags);
	block->SetFlags(flags);

	//Get the number of block members
	icarus->BufferRead(&numMembers, sizeof numMembers);

	for (int j = 0; j < numMembers; j++)
	{
		//Get the member ID
		icarus->BufferRead(&bID, sizeof bID);

		//Get the member size
		icarus->BufferRead(&bSize, sizeof bSize);

		//Get the member's data
		if ((bData = game->Malloc(bSize)) == nullptr)
			return false;

		//Get the actual raw data
		icarus->BufferRead(bData, bSize);

		//Write out the correct type
		switch (bID)
		{
		case CIcarus::TK_INT:
		{
			assert(0);
			const int data = *static_cast<int*>(bData);
			block->Write(CIcarus::TK_FLOAT, static_cast<float>(data), icarus);
		}
		break;

		case CIcarus::TK_FLOAT:
			block->Write(CIcarus::TK_FLOAT, *static_cast<float*>(bData), icarus);
			break;

		case CIcarus::TK_STRING:
		case CIcarus::TK_IDENTIFIER:
		case CIcarus::TK_CHAR:
			block->Write(CIcarus::TK_STRING, static_cast<char*>(bData), icarus);
			break;

		case CIcarus::TK_VECTOR:
		case CIcarus::TK_VECTOR_START:
			block->Write(CIcarus::TK_VECTOR, *static_cast<vec3_t*>(bData), icarus);
			break;

		case CIcarus::ID_TAG:
			block->Write(CIcarus::ID_TAG, static_cast<float>(CIcarus::ID_TAG), icarus);
			break;

		case CIcarus::ID_GET:
			block->Write(CIcarus::ID_GET, static_cast<float>(CIcarus::ID_GET), icarus);
			break;

		case CIcarus::ID_RANDOM:
			block->Write(CIcarus::ID_RANDOM, *static_cast<float*>(bData), icarus); //(float) ID_RANDOM );
			break;

		case CIcarus::TK_EQUALS:
		case CIcarus::TK_GREATER_THAN:
		case CIcarus::TK_LESS_THAN:
		case CIcarus::TK_NOT:
			block->Write(bID, 0, icarus);
			break;

		default:
			assert(0);
			return false;
		}

		//Get rid of the temp memory
		game->Free(bData);
	}

	return true;
}

/*
-------------------------
Save
-------------------------
*/

int CSequence::Save()
{
	// Data saved here.
	//	Parent ID.
	//	Return ID.
	//	Number of Children.
	//	Children.
	//			- Child ID
	//	Save Flags.
	//	Save Iterations.
	//	Number of Commands
	//			- Commands (raw) data.

	const auto pIcarus = static_cast<CIcarus*>(IIcarusInterface::GetIcarus());

	block_l::iterator bi;
	int id;

	// Save the parent (by GUID).
	id = m_parent != nullptr ? m_parent->GetID() : -1;
	pIcarus->BufferWrite(&id, sizeof id);

	//Save the return (by GUID)
	id = m_return != nullptr ? m_return->GetID() : -1;
	pIcarus->BufferWrite(&id, sizeof id);

	//Save the number of children
	const int i_num_children = m_children.size();
	pIcarus->BufferWrite(&i_num_children, sizeof i_num_children);
	sequence_l::iterator iterSeq;
	STL_ITERATE(iterSeq, m_children)
	{
		id = (*iterSeq)->GetID();
		pIcarus->BufferWrite(&id, sizeof id);
	}

	//Save flags
	pIcarus->BufferWrite(&m_flags, sizeof m_flags);

	//Save iterations
	pIcarus->BufferWrite(&m_iterations, sizeof m_iterations);

	//Save the number of commands
	pIcarus->BufferWrite(&m_numCommands, sizeof m_numCommands);

	//Save the commands
	STL_ITERATE(bi, m_commands)
	{
		SaveCommand(*bi);
	}

	return true;
}

/*
-------------------------
Load
-------------------------
*/

int CSequence::Load(CIcarus* icarus)
{
	CSequence* sequence;
	int id;

	// Data expected/loaded here (ISEQ) (with the size as : 'ISSZ' ).
	//	Parent ID.
	//	Return ID.
	//	Number of Children.
	//	Children.
	//			- Child ID
	//	Save Flags.
	//	Save Iterations.
	//	Number of Commands
	//			- Commands (raw) data.

	//Get the parent sequence
	icarus->BufferRead(&id, sizeof id);
	m_parent = id != -1 ? icarus->GetSequence(id) : nullptr;

	//Get the return sequence
	icarus->BufferRead(&id, sizeof id);
	m_return = id != -1 ? icarus->GetSequence(id) : nullptr;

	//Get the number of children
	int iNumChildren = 0;
	icarus->BufferRead(&iNumChildren, sizeof iNumChildren);

	//Reload all children
	for (int i = 0; i < iNumChildren; i++)
	{
		//Get the child sequence ID
		icarus->BufferRead(&id, sizeof id);

		//Get the desired sequence
		if ((sequence = icarus->GetSequence(id)) == nullptr)
			return false;

		//Insert this into the list
		STL_INSERT(m_children, sequence);

		//Restore the connection in the child / ID map
		//		m_childrenMap[ i ] = sequence;
	}

	//Get the sequence flags
	icarus->BufferRead(&m_flags, sizeof m_flags);

	//Get the number of iterations
	icarus->BufferRead(&m_iterations, sizeof m_iterations);

	int numCommands;

	//Get the number of commands
	icarus->BufferRead(&numCommands, sizeof numCommands);

	//Get all the commands
	for (int i = 0; i < numCommands; i++)
	{
		const auto block = new CBlock;
		LoadCommand(block, icarus);

		//Save the block
		//STL_INSERT( m_commands, block );
		PushCommand(block, PUSH_BACK);
	}

	return true;
}