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

#pragma once

#include "../qcommon/q_shared.h"
#include <malloc.h>

class IHeapAllocator
{
public:
	virtual ~IHeapAllocator() = default;

	virtual void ResetHeap() = 0;
	virtual char* MiniHeapAlloc(int size) = 0;
};

class CMiniHeap : public IHeapAllocator
{
private:
	char* mHeap;
	char* mCurrentHeap;
	int   mSize;

public:
	// reset the heap back to the start
	void ResetHeap() override
	{
		mCurrentHeap = mHeap;
	}

	// initialise the heap
	inline CMiniHeap(const int size)
		: mHeap(nullptr)
		, mCurrentHeap(nullptr)
		, mSize(size)
	{
		mHeap = static_cast<char*>(malloc(size));
		if (mHeap)
		{
			mCurrentHeap = mHeap;   // or ResetHeap();
		}
	}

	// free up the heap
	~CMiniHeap() override
	{
		if (mHeap)
		{
			free(mHeap);
		}
	}

	// give me some space from the heap please
	char* MiniHeapAlloc(const int size) override
	{
		size_t used = static_cast<size_t>(mCurrentHeap - mHeap);
		if (static_cast<size_t>(size) < (static_cast<size_t>(mSize) - used))
		{
			char* tempAddress = mCurrentHeap;
			mCurrentHeap += size;
			return tempAddress;
		}
		return nullptr;
	}
};

// this is in the parent executable, so access ri->GetG2VertSpaceServer() from the rd backends!
extern IHeapAllocator* G2VertSpaceServer;
extern IHeapAllocator* G2VertSpaceClient;
