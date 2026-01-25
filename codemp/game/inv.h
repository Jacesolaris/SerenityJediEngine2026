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

#define INVENTORY_NONE				0
//pickups
#define INVENTORY_ARMOR				1
#define INVENTORY_HEALTH			2
//items
#define INVENTORY_SEEKER			3
#define INVENTORY_MEDPAC			4
#define INVENTORY_DATAPAD			5
#define INVENTORY_BINOCULARS		6
#define INVENTORY_SENTRY_GUN		7
#define INVENTORY_GOGGLES			8
//weapons
#define INVENTORY_STUN_BATON		9
#define INVENTORY_SABER				10
#define INVENTORY_BRYAR_PISTOL		11
#define INVENTORY_BLASTER			12
#define INVENTORY_DISRUPTOR			13
#define INVENTORY_BOWCASTER			14
#define INVENTORY_REPEATER			15
#define INVENTORY_DEMP2				16
#define INVENTORY_FLECHETTE			17
#define INVENTORY_ROCKET_LAUNCHER	18
#define INVENTORY_THERMAL			19
#define INVENTORY_TRIP_MINE			20
#define INVENTORY_DET_PACK			21
//ammo
#define INVENTORY_AMMO_FORCE		22
#define INVENTORY_AMMO_BLASTER		23
#define INVENTORY_AMMO_BOLTS		24
#define INVENTORY_AMMO_ROCKETS		25
//powerups
#define INVENTORY_REDFLAG			26
#define INVENTORY_BLUEFLAG			27
#define INVENTORY_SCOUT				28
#define INVENTORY_GUARD				29
#define INVENTORY_DOUBLER			30
#define INVENTORY_AMMOREGEN			31
#define INVENTORY_NEUTRALFLAG		32
#define INVENTORY_REDCUBE			33
#define INVENTORY_BLUECUBE			34

//enemy stuff
#define ENEMY_HORIZONTAL_DIST		200
#define ENEMY_HEIGHT				201
#define NUM_VISIBLE_ENEMIES			202
#define NUM_VISIBLE_TEAMMATES		203

// NOTENOTE Update this so that it is in sync.
//item numbers (make sure they are in sync with bg_itemlist in bg_misc.c)
//pickups
#define model_index_ARMOR			1
#define model_index_HEALTH			2
//items
#define model_index_SEEKER			3
#define model_index_MEDPAC			4
#define model_index_DATAPAD			5
#define model_index_BINOCULARS		6
#define model_index_SENTRY_GUN		7
#define model_index_GOGGLES			8
//weapons
#define model_index_STUN_BATON		9
#define model_index_SABER			10
#define model_index_BRYAR_PISTOL		11
#define model_index_BLASTER			12
#define model_index_DISRUPTOR		13
#define model_index_BOWCASTER		14
#define model_index_REPEATER			15
#define model_index_DEMP2			16
#define model_index_FLECHETTE		17
#define model_index_ROCKET_LAUNCHER	18
#define model_index_THERMAL			19
#define model_index_TRIP_MINE		20
#define model_index_DET_PACK			21
//ammo
#define model_index_AMMO_FORCE		22
#define model_index_AMMO_BLASTER		23
#define model_index_AMMO_BOLTS		24
#define model_index_AMMO_ROCKETS		25
//powerups
#define model_index_REDFLAG			26
#define model_index_BLUEFLAG			27
#define model_index_SCOUT			28
#define model_index_GUARD			29
#define model_index_DOUBLER			30
#define model_index_AMMOREGEN		31
#define model_index_NEUTRALFLAG		32
#define model_index_REDCUBE			33
#define model_index_BLUECUBE			34

//
#define weapon_index_STUN_BATON		1
#define weapon_index_SABER			2
#define weapon_index_BRYAR_PISTOL	3
#define weapon_index_BLASTER			4
#define weapon_index_DISRUPTOR		5
#define weapon_index_BOWCASTER		6
#define weapon_index_REPEATER		7
#define weapon_index_DEMP2			8
#define weapon_index_FLECHETTE		9
#define weapon_index_ROCKET_LAUNCHER	10
#define weapon_index_THERMAL			11
#define weapon_index_TRIP_MINE		12
#define weapon_index_DET_PACK		13
