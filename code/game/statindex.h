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

// Filename:	statindex.h
//
// accessed from both server and game modules

#ifndef STATINDEX_H
#define STATINDEX_H

// player_state->stats[] indexes
using statIndex_t = enum
{
	STAT_HEALTH,
	STAT_ITEMS,
	STAT_WEAPONS,
	// 16 bit fields
	STAT_ARMOR,
	STAT_DEAD_YAW,
	// look this direction when dead (FIXME: get rid of?)
	STAT_CLIENTS_READY,
	// bit mask of clients wishing to exit the intermission (FIXME: configstring?)
	STAT_MAX_HEALTH,
	// health / armor limit, changable by handicap
	STAT_DODGE,
	//number of Dodge Points the player has.  DP is used for evading/blocking attacks before they hurt you.
	STAT_MAX_DODGE,
	//maximum number of dodge points allowed.
	STAT_AMMO,
	// Ammo in current weapon
	STAT_TOTALAMMO,
	// Total ammo
};

#endif	// #ifndef STATINDEX_H

/////////////////////// eof /////////////////////
