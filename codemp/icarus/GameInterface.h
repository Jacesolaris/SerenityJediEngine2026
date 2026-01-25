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

#include <map>
#include <string>

using pscript_t = struct pscript_s
{
	char* buffer;
	long length;
};

using entlist_t = std::map<std::string, int>;
using bufferlist_t = std::map<std::string, pscript_t*>;

//ICARUS includes
extern interface_export_t interface_export;

extern void Interface_Init(interface_export_t* pe);
extern int ICARUS_RunScript(const sharedEntity_t* ent, const char* name);
extern bool ICARUS_RegisterScript(const char* name, qboolean bCalledDuringInterrogate = qfalse);
extern ICARUS_Instance* iICARUS;
extern bufferlist_t ICARUS_BufferList;
extern entlist_t ICARUS_EntList;

//
//	g_ICARUS.cpp
//
void ICARUS_Init(void);
bool ICARUS_ValidEnt(const sharedEntity_t* ent);
void ICARUS_InitEnt(sharedEntity_t* ent);
void ICARUS_FreeEnt(const sharedEntity_t* ent);
void ICARUS_AssociateEnt(const sharedEntity_t* ent);
void ICARUS_Shutdown(void);
void Svcmd_ICARUS_f(void);

extern int ICARUS_entFilter;
