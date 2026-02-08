/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

// g_local.h -- local definitions for game module

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_vehicles.h"
#include "g_public.h"

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

//npc stuff
#include "b_public.h"

extern int gPainMOD;
extern int gPainHitLoc;
extern vec3_t gPainPoint;

//==================================================================

// the "gameversion" client command will print this plus compile date
#define	GAMEVERSION	"SerenityJediEngine2026"

#define SECURITY_LOG "security.log"

#define BODY_QUEUE_SIZE		8

#ifndef INFINITE
#define INFINITE			1000000
#endif

#define	FRAMETIME			100					// msec
#define	CARNAGE_REWARD_TIME	3000
#define REWARD_SPRITE_TIME	2000

#define	INTERMISSION_DELAY_TIME	1000
#define	SP_INTERMISSION_DELAY_TIME	5000

#define AUTOSAVE_TELE		0x00000001 //->spawnflag for autosave map entities that teleport all players when activated
#define AUTOSAVE_EDITOR		0x00000002 //this autosave point was created with the CoOp editor, used to know if the autosave should be rendered when the CoOp Editor is on.

//primarily used by NPCs
#define	START_TIME_LINK_ENTS		FRAMETIME*1 // time-delay after map start at which all ents have been spawned, so can link them

#define	START_TIME_FIND_LINKS		FRAMETIME*2 // time-delay after map start at which you can find linked entities
#define	START_TIME_MOVERS_SPAWNED	FRAMETIME*2 // time-delay after map start at which all movers should be spawned
#define	START_TIME_REMOVE_ENTS		FRAMETIME*3 // time-delay after map start to remove temporary ents
#define	START_TIME_NAV_CALC			FRAMETIME*4 // time-delay after map start to connect waypoints and calc routes
#define	START_TIME_FIND_WAYPOINT	FRAMETIME*5 // time-delay after map start after which it's okay to try to find your best waypoint

// gentity->flags
#define	FL_GODMODE				0x00000010
#define	FL_NOTARGET				0x00000020
#define	FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
#define FL_DROPPED_ITEM			0x00001000
#define FL_NO_BOTS				0x00002000	// spawn point not for bot use
#define FL_NO_HUMANS			0x00004000	// spawn point just for bots
#define FL_FORCE_GESTURE		0x00008000	// force gesture on client
#define FL_INACTIVE				0x00010000	// inactive
#define FL_NAVGOAL				0x00020000	// for npc nav stuff
#define	FL_DONT_SHOOT			0x00040000
#define FL_SHIELDED				0x00080000
#define FL_UNDYING				0x00100000	// takes damage down to 1, but never dies
#define FL_LOCK_PLAYER_WEAPONS	0x00200000	// No bone angle overrides, no pitch or roll in full angles
#define FL_RED_CROSSHAIR		0x00400000	// Crosshair red on me
#define	FL_DINDJARIN			0x00800000	// protected from all damage except lightsabers for 20% of the time
#define	FL_BOBAFETT 			0x01000000	// protected from all damage
#define	FL_BOUNCE				0x00100000		// for missiles
#define	FL_BOUNCE_HALF			0x00200000		// for missiles
#define	FL_BOUNCE_SHRAPNEL		0x00400000		// special shrapnel flag
#define	FL_VEH_BOARDING			0x00800000		// special shrapnel flag
#define FL_DMG_BY_SABER_ONLY		0x01000000 //only take dmg from saber
#define FL_DMG_BY_HEAVY_WEAP_ONLY	0x02000000 //only take dmg from explosives
#define FL_BBRUSH					0x04000000 //I am a breakable brush
#define FL_NO_IMPACT_DMG			0x08000000	// Will not take impact damage
#define	FL_SABERDAMAGE_RESIST		0x10000000	// protected from all damage except lightsabers for 20% of the time
#define FL_NOFORCE				    0x20000000

#define SABERTWEAK_INTERPOLATE			(0x0001u) // use SP style interpolation, also fix various small issues
#define SABERTWEAK_PROLONGDAMAGE		(0x0002u) // allow damaging in wind-up and return animations
#define SABERTWEAK_POSDEFLECTION		(0x0004u) // calculate deflection based on position rather than animation
#define SABERTWEAK_SPECIALMOVEDMG		(0x0008u) // tweak damages for special moves
#define SABERTWEAK_TRACESIZE			(0x0010u) // use SP saber trace size or based off radius defined in .sab file
#define SABERTWEAK_REDUCEBLOCKS			(0x0020u) // reduce chance of blocking based on saber stance
#define SABERTWEAK_TWOBLADEDEFLECTFIX	(0x0040u) // fix deflection bug when toggling second saber
#define SABERTWEAK_NERFDMG				(0x0080u) // nerf moves like roll-stab
#define SABERTWEAK_NOTLOCATIONBASED		(0x0100u) // never use location based damage for sabers

//#ifndef FINAL_BUILD
//#define DEBUG_SABER_BOX
//#endif

// make sure this matches game/match.h for botlibs
#define EC "\x19"

#define	MAX_G_SHARED_BUFFER_SIZE		8192

// used for communication with the engine
typedef union sharedBuffer_u
{
	char raw[MAX_G_SHARED_BUFFER_SIZE];
	T_G_ICARUS_PLAYSOUND playSound;
	T_G_ICARUS_SET set;
	T_G_ICARUS_LERP2POS lerp2Pos;
	T_G_ICARUS_LERP2ORIGIN lerp2Origin;
	T_G_ICARUS_LERP2ANGLES lerp2Angles;
	T_G_ICARUS_GETTAG getTag;
	T_G_ICARUS_LERP2START lerp2Start;
	T_G_ICARUS_LERP2END lerp2End;
	T_G_ICARUS_USE use;
	T_G_ICARUS_KILL kill;
	T_G_ICARUS_REMOVE remove;
	T_G_ICARUS_PLAY play;
	T_G_ICARUS_GETFLOAT getFloat;
	T_G_ICARUS_GETVECTOR getVector;
	T_G_ICARUS_GETSTRING getString;
	T_G_ICARUS_SOUNDINDEX soundIndex;
	T_G_ICARUS_GETSETIDFORSTRING getSetIDForString;
} sharedBuffer_t;

extern sharedBuffer_t gSharedBuffer;

//Airspace entities
gentity_t* airspace[32];
int numAirspaces;

// movers are things like doors, plats, buttons, etc
typedef enum
{
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1
} moverState_t;

#define SP_PODIUM_MODEL		"models/mapobjects/podium/podium4.md3"

typedef enum
{
	HL_NONE = 0,
	HL_FOOT_RT,
	HL_FOOT_LT,
	HL_LEG_RT,
	HL_LEG_LT,
	HL_WAIST,
	HL_BACK_RT,
	HL_BACK_LT,
	HL_BACK,
	HL_CHEST_RT,
	HL_CHEST_LT,
	HL_CHEST,
	HL_ARM_RT,
	HL_ARM_LT,
	HL_HAND_RT,
	HL_HAND_LT,
	HL_HEAD,
	HL_GENERIC1,
	HL_GENERIC2,
	HL_GENERIC3,
	HL_GENERIC4,
	HL_GENERIC5,
	HL_GENERIC6,
	//# #eol
	HL_MAX
} hitLocation_t;

#define		MAX_OBJECTIVES			6
#define		MAX_OBJECTIVEDEPENDANCY	6

typedef enum
{
	BOTORDER_NONE,
	//no order
	BOTORDER_KNEELBEFOREZOD,
	//Kneel before the ordered person
	BOTORDER_SEARCHANDDESTROY,
	//Attack mode.  If given an entity the bot will search for
	//and then attack that entity.  If NULL, the bot will just
	//hunt around and attack enemies.
	BOTORDER_OBJECTIVE,
	//Do objective play for seige.  Bot will defend or attack objective
	//based on who's objective it is.
	BOTORDER_DEFEND,
	//Basic defend order.  Used for CoOp Bots automatically for CoOp.
	BOTORDER_JEDIMASTER,
	//jedimaster tactics.
	BOTORDER_SABERDUELCHALLENGE,
	//enter into a saber challenge with the objective entity
	BOTORDER_RESUPPLY,
	//Go grab/use the tactic entity (used for making the bots pick up ammo).
	BOTORDER_JETPACK_COMBAT,
	BOTORDER_SABER_DUEL,
	BOTORDER_FLANK,

	BOTORDER_MAX
} Botorder_t;

typedef enum
{
	OT_NONE,
	//no OT selected or bad OT
	OT_ATTACK,
	//Attack this objective, for destroyable stationary objectives
	OT_DEFEND,
	//Defend this objective, for destroyable stationary objectives
	//or touch objectives
	OT_CAPTURE,
	//Capture this objective
	OT_DEFENDCAPTURE,
	//prevent capture of this objective
	OT_TOUCH,
	OT_VEHICLE,
	//get this vehicle to the related trigger_once.
	OT_WAIT //This is used by the bots to while they are waiting for a vehicle to respawn
} Botorderobjective_t;

#define MAX_GEN_NPCTYPES 32
#define MAX_GEN_COUNT 32
#define MAX_GEN_FIRSTGOALS 8

typedef gentity_t* pgentity_t;

typedef struct gnpcgentype_s
{
	int luck;
	char* type;
} gnpcgentype_t;

typedef struct gnpcgen_s
{
	gnpcgentype_t npcTypes[MAX_GEN_NPCTYPES];
	int totalLuck;

	int max2; //RoAR mod NOTE: This is our MAX linux busted function
	int interval;

	int numFirstGoals;
	vec3_t firstGoal[MAX_GEN_FIRSTGOALS];

	int lastSpawn;

	int numSpawned;
	pgentity_t spawned[MAX_GEN_COUNT];
} gnpcgen_t;

//============================================================================
extern void* precachedKyle;
extern void* g2SaberInstance;

extern qboolean gEscaping;
extern int gEscapeTime;

struct gentity_s
{
	//rww - entstate must be first, to correspond with the bg shared entity structure
	entityState_t s; // communicated by server to clients
	playerState_t* playerState; //ptr to playerstate if applicable (for bg ents)
	Vehicle_t* m_pVehicle; //vehicle data
	void* ghoul2; //g2 instance
	int localAnimIndex; //index locally (game/cgame) to anim data for this skel
	vec3_t modelScale; //needed for g2 collision

	//From here up must be the same as centity_t/bgEntity_t

	entityShared_t r; // shared by both the server system and game

	//rww - these are shared icarus things. They must be in this order as well in relation to the entityshared structure.
	int taskID[NUM_TIDS];
	parms_t* parms;
	char* behaviorSet[NUM_BSETS];
	char* script_targetname;
	int delayScriptTime;
	char* fullName;

	//rww - targetname and classname are now shared as well. ICARUS needs access to them.
	char* targetname;
	char* classname; // set in QuakeEd

	//rww - and yet more things to share. This is because the nav code is in the exe because it's all C++.
	int waypoint; //Set once per frame, if you've moved, and if someone asks
	int lastWaypoint; //To make sure you don't double-back
	int lastValidWaypoint; //ALWAYS valid -used for tracking someone you lost
	int noWaypointTime;
	//Debouncer - so don't keep checking every waypoint in existance every frame that you can't find one
	int combatPoint;
	int failedWaypoints[MAX_FAILED_NODES];
	int failedWaypointCheckTime;

	int next_roff_time; //rww - npc's need to know when they're getting roff'd

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	struct gclient_s* client; // NULL if not a client

	int clientNum; // ranges from 0 to MAX_CLIENTS-1

	gNPC_t* NPC; //Only allocated if the entity becomes an NPC
	int cantHitEnemyCounter;
	//HACK - Makes them look for another enemy on the same team if the one they're after can't be hit

	qboolean noLumbar; //see note in cg_local.h

	qboolean inuse;

	int lockCount; //used by NPCs

	int spawnflags; // set in QuakeEd

	int teamnodmg; // damage will be ignored if it comes from this team

	char* roffname; // set in QuakeEd
	char* rofftarget; // set in QuakeEd

	char* healingclass; //set in quakeed
	char* healingsound; //set in quakeed
	int healingrate; //set in quakeed
	int healingDebounce; //debounce for generic object healing shiz

	char* ownername;

	int objective;
	int side;

	int passThroughNum; // set to index to pass through (+1) for missiles

	int aimDebounceTime;
	int painDebounceTime;
	int attackDebounceTime;
	int alliedTeam; // only useable by this team, never target this team

	int roffid; // if roffname != NULL then set on spawn

	qboolean neverFree; // if true, FreeEntity will only unlink
	// bodyque uses this

	int flags; // FL_* variables

	char* model;
	char* model2;
	int freetime; // level.time when the object was freed

	int eventTime; // events will be cleared EVENT_VALID_MSEC after set
	qboolean freeAfterEvent;
	qboolean unlinkAfterEvent;
	int sentryDeadThink;
	int forceFieldThink;

	qboolean physicsObject; // if true, it can be pushed by movers and fall off edges
	// all game items are physicsObjects,
	float physicsBounce; // 1.0 = continuous bounce, 0.0 = no bounce
	int clipmask; // brushes with this content value will be collided against
	// when moving.  items and corpses do not collide against
	// players, for instance

	//Only used by NPC_spawners
	char* NPC_type;
	char* NPC_targetname;
	char* NPC_target;

	// movers
	moverState_t moverState;
	int soundPos1;
	int sound1to2;
	int sound2to1;
	int soundPos2;
	int soundLoop;
	gentity_t* parent;
	gentity_t* nextTrain;
	gentity_t* prevTrain;
	vec3_t pos1, pos2;

	//for npc's
	vec3_t pos3;

	union
	{
		vec3_t pos4;
	};

	char* message;

	int timestamp; // body queue sinking, etc

	float angle; // set in editor, -1 = up, -2 = down
	char* target;
	char* target2;
	char* target3; //For multiple targets, not used for firing/triggering/using, though, only for path branches
	char* target4; //For multiple targets, not used for firing/triggering/using, though, only for path branches
	char* target5; //mainly added for siege items
	char* target6; //mainly added for siege items

	char* team;
	char* targetShaderName;
	char* targetShaderNewName;
	gentity_t* targetEnt;

	char* closetarget;
	char* opentarget;
	char* paintarget;

	char* goaltarget;
	char* idealclass;

	float radius;

	int maxHealth; //used as a base for crosshair health display

	float speed;
	vec3_t movedir;
	float mass;
	int setTime;

	//Force effects
	int forcePushTime;
	int forcePuller; //who force-pulled me (so we don't damage them if we hit them)

	//Think Functions
	int nextthink;
	void (*think)(gentity_t* self);
	void (*reached)(gentity_t* self); // movers call this when hitting endpoint
	void (*blocked)(gentity_t* self, gentity_t* other);
	void (*touch)(gentity_t* self, gentity_t* other, trace_t* trace);
	void (*use)(gentity_t* self, gentity_t* other, gentity_t* activator);
	void (*pain)(gentity_t* self, gentity_t* attacker, int damage);
	void (*die)(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int mod);

	int pain_debounce_time;
	int fly_sound_debounce_time; // wind tunnel
	int last_move_time;

	//Health and damage fields
	int health;
	qboolean takedamage;
	material_t material;
	int damageDecreaseTime;

	//keeps track of jetpack fuel
	int jetpackFuel;
	int cloakFuel;

	int lives;
	int liveExp;

	int damage;
	int dflags;
	int splashDamage; // quad will increase this without increasing radius
	int splashRadius;
	int methodOfDeath;
	int splashMethodOfDeath;

	int locationDamage[HL_MAX]; // Damage accumulated on different body locations

	int count;
	int bounceCount;
	qboolean alt_fire;

	gentity_t* chain;
	gentity_t* enemy;
	gentity_t* lastEnemy;
	gentity_t* activator;
	gentity_t* teamchain; // next entity in team
	gentity_t* teammaster; // master of the team

	int watertype;
	int waterlevel;

	int noise_index;

	// timing variables
	float wait;
	float random;
	int delay;

	//generic values used by various entities for different purposes.
	int genericValue1;
	int genericValue2;
	int genericValue3;
	int genericValue4;
	int genericValue5;
	int genericValue6;
	int genericValue7;
	int genericValue8;
	int genericValue9;
	int genericValue10;
	int genericValue11;
	int genericValue12;
	int genericValue13;
	int genericValue14;
	int genericValue15;

	short genericBolt1; // For bolts special to an entity
	short genericBolt2;
	short genericBolt3;
	short genericBolt4;
	short genericBolt5;

	char* soundSet;

	qboolean isSaberEntity;

	int damageRedirect; //if entity takes damage, redirect to..
	int damageRedirectTo; //this entity number

	vec3_t epVelocity;
	float epGravFactor;

	gitem_t* item; // for bonus items
	int nDamageFromVehicleTimer;

	int IcarusSoundTime; //manual debouncer for Icarus sound scripting.
	char* cameraGroup; //used for cameragroup targeting

	int startFrame; //startframe for map entity animations
	int endFrame; //endframe for map entity animations
	qboolean loopAnim; //does this animation loop?

	int roff_ctr; // current roff frame we are playing
	// SerenityJediEngine2026 add
	int IDCode; // Used for GLua to identify old ent references
	int UsesELS;
	int useDebounceTime; // for cultist_destroyer

	vec3_t mins, maxs;
	int lastInAirTime;

	qhandle_t cinematicModel;

	vec3_t spawn_pos;

	vec3_t origOrigin;
	int bot_strafe_left_timer;
	int bot_strafe_right_timer;
	int bot_strafe_crouch_timer;
	int bot_strafe_jump_timer;
	int bot_strafe_target_timer;
	vec3_t bot_strafe_target_position;
	int bot_check_speach_time;
	int check_speach_time;

	gentity_t* owner; // objects never interact with their owners, to
	// prevent player missiles from immediately
	// colliding with their owner
	gnpcgen_t* npcgen;

	int npc_attack_time;

	int npc_roll_time;
	qboolean npc_roll_start;
	int npc_roll_direction;
	int fx_time; // timer for beam in/out effects.
	int wpCurrent;
	int beStillTime;

	int reloadTime; //Every 0.2 seconds reload a bullet
	int reloadCooldown;
	int TimeOfWeaponDrop;
};

#define DAMAGEREDIRECT_HEAD		1
#define DAMAGEREDIRECT_RLEG		2
#define DAMAGEREDIRECT_LLEG		3

typedef enum
{
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum
{
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW,
	SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum
{
	TEAM_BEGIN,
	// Beginning a team game, spawn at base
	TEAM_ACTIVE // Now actively playing
} playerTeamStateState_t;

typedef struct playerTeamState_s
{
	playerTeamStateState_t state;

	int location;

	int captures;
	int basedefense;
	int carrierdefense;
	int flagrecovery;
	int fragcarrier;
	int assists;

	float lasthurtcarrier;
	float lastreturnedflag;
	float flagsince;
	float lastfraggedcarrier;
} playerTeamState_t;

// the auto following clients don't follow a specific client
// number, but instead follow the first two active players
#define	FOLLOW_ACTIVE1	-1
#define	FOLLOW_ACTIVE2	-2

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct clientSession_s
{
	team_t sessionTeam;
	int spectatorNum; // for determining next-in-line to play
	spectatorState_t spectatorState;
	int spectatorClient; // for chasecam and follow mode
	int wins, losses; // tournament stats
	int selectedFP; // check against this, if doesn't match value in playerstate then update userinfo
	int saberLevel; // similar to above method, but for current saber attack level
	int setForce; // set to true once player is given the chance to set force powers
	int updateUITime; // only update userinfo for FP/SL if < level.time
	qboolean teamLeader; // true when this client is a team leader
	char siegeClass[64];
	int duelTeam;
	int siegeDesiredTeam;

	char IP[NET_ADDRSTRMAXLEN];

	// used to set the ally ids. The allies dont receive damage from this player
	int ally1;
	int ally2;
} clientSession_t;

// playerstate mGameFlags
#define	PSG_VOTED				(1<<0)		// already cast a vote
#define PSG_TEAMVOTED			(1<<1)		// already cast a team vote

//
#define MAX_NETNAME			72
#define	MAX_VOTE_COUNT		3

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct clientPersistant_s
{
	clientConnected_t connected;
	usercmd_t cmd; // we would lose angles if not persistant
	qboolean localClient; // true if "ip" info key is "localhost"
	qboolean initialSpawn; // the first spawn should be at a cool location
	qboolean predictItemPickup; // based on cg_predictItems userinfo
	qboolean pmoveFixed; //
	char netname[MAX_NETNAME];
	char netname_nocolor[MAX_NETNAME];
	int netnameTime; // Last time the name was changed
	int maxHealth; // for handicapping
	int enterTime; // level.time the client entered the game
	playerTeamState_t teamState; // status in teamplay games
	qboolean teamInfo; // send team overlay updates?

	int connectTime;

	char saber1[MAX_QPATH], saber2[MAX_QPATH];

	int vote, teamvote; // 0 = none, 1 = yes, 2 = no

	char guid[33];

	bclass_t botclass;
	bclass_t nextbotclass;
	bclass_t accountclass;
	bclass_t nextaccountclass;
	qboolean botmodelscale;

	qboolean SJE_clientplugin;

	int save_score;

	int	padawantimer;
	qboolean	isapadawan;
	int			iamanadmin;
	int			bitvalue;
	char		login[1024];
	char		logout[1024];
	qboolean	plugindetect;
	qboolean	isbeingpunished;
} clientPersistant_t;

typedef struct renderInfo_s
{
	//In whole degrees, How far to let the different model parts yaw and pitch
	int headYawRangeLeft;
	int headYawRangeRight;
	int headPitchRangeUp;
	int headPitchRangeDown;

	int torsoYawRangeLeft;
	int torsoYawRangeRight;
	int torsoPitchRangeUp;
	int torsoPitchRangeDown;

	int legsFrame;
	int torsoFrame;

	float legsFpsMod;
	float torsoFpsMod;

	//Fields to apply to entire model set, individual model's equivalents will modify this value
	vec3_t customRGB; //Red Green Blue, 0 = don't apply
	int customAlpha; //Alpha to apply, 0 = none?

	//RF?
	int renderFlags;

	//
	vec3_t muzzlePoint;
	vec3_t muzzleDir;
	vec3_t muzzlePointOld;
	vec3_t muzzleDirOld;
	//vec3_t		muzzlePointNext;	// Muzzle point one server frame in the future!
	//vec3_t		muzzleDirNext;
	int mPCalcTime; //Last time muzzle point was calced

	//
	float lockYaw; //

	//
	vec3_t headPoint; //Where your tag_head is
	vec3_t headAngles; //where the tag_head in the torso is pointing
	vec3_t handRPoint; //where your right hand is
	vec3_t handLPoint; //where your left hand is
	vec3_t crotchPoint; //Where your crotch is
	vec3_t footRPoint; //where your right hand is
	vec3_t footLPoint; //where your left hand is
	vec3_t torsoPoint; //Where your chest is
	vec3_t torsoAngles; //Where the chest is pointing
	vec3_t eyePoint; //Where your eyes are
	vec3_t eyeAngles; //Where your eyes face
	int lookTarget; //Which ent to look at with lookAngles
	lookMode_t lookMode;
	int lookTargetClearTime; //Time to clear the lookTarget
	int lastVoiceVolume; //Last frame's voice volume
	vec3_t lastHeadAngles; //Last headAngles, NOT actual facing of head model
	vec3_t headBobAngles; //headAngle offsets
	vec3_t targetHeadBobAngles; //head bob angles will try to get to targetHeadBobAngles
	int lookingDebounceTime; //When we can stop using head looking angle behavior
	float legsYaw; //yaw angle your legs are actually rendering at

	//for tracking legitimate bolt indecies
	void* lastG2; //if it doesn't match ent->ghoul2, the bolts are considered invalid.
	int headBolt;
	int handRBolt;
	int handLBolt;
	int torsoBolt;
	int crotchBolt;
	int footRBolt;
	int footLBolt;
	int motionBolt;
	int elbowLBolt;
	int elbowRBolt;

	int boltValidityTime;
} renderInfo_t;

typedef struct
{
	int entityNum;
	int Debounce;
	int saberNum;
	int blade_num;
} sabimpact_t;

typedef gentity_t* follower_t;

// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s
{
	// ps MUST be the first element, because the server expects it
	playerState_t ps; // communicated by server to clients

	// the rest of the structure is private to game
	clientPersistant_t pers;
	clientSession_t sess;

	saberInfo_t saber[MAX_SABERS];
	void* weaponGhoul2[MAX_SABERS];

	int tossableItemDebounce;

	int bodyGrabTime;
	int bodyGrabIndex;

	int pushEffectTime;

	int invulnerableTimer;

	int saberCycleQueue;

	int legsAnimExecute;
	int torsoAnimExecute;
	qboolean legsLastFlip;
	qboolean torsoLastFlip;

	int shotsBlocked;

	qboolean readyToExit; // wishes to leave the intermission

	qboolean noclip;

	int lastCmdTime; // level.time of last usercmd_t, for EF_CONNECTION
	// we can't just use pers.lastCommand.time, because
	// of the g_sycronousclients case
	int buttons;
	int oldbuttons;
	int latched_buttons;

	vec3_t oldOrigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int damage_armor; // damage absorbed by armor
	int damage_blood; // damage taken out of health
	int damage_knockback; // impact damage
	vec3_t damage_from; // origin for vector calculation
	qboolean damage_fromWorld; // if true, don't use the damage_from vector

	int damageBoxHandle_Head; //entity number of head damage box
	int damageBoxHandle_RLeg; //entity number of right leg damage box
	int damageBoxHandle_LLeg; //entity number of left leg damage box

	int accurateCount; // for "impressive" reward sound

	int accuracy_shots; // total number of shots
	int accuracy_hits; // total number of hits

	//
	int lastkilled_client; // last client that this client killed
	int lasthurt_client; // last client that damaged this client
	int lasthurt_mod; // type of damage the client did
	int lasthurt_location; // Where the client was hit.

	// timers
	int respawnTime; // can respawn when time > this, force after g_forcerespwan
	int inactivityTime; // kick players when time > this
	qboolean inactivityWarning; // qtrue if the five seoond warning has been given
	int rewardTime; // clear the EF_AWARD_IMPRESSIVE, etc when time > this

	int airOutTime;

	int lastKillTime; // for multiple kill rewards

	qboolean fireHeld; // used for hook

	qboolean stunHeld;

	gentity_t* hook; // grapple hook if out
	gentity_t* stun; // grapple hook if out

	int switchTeamTime; // time the player switched teams

	int switchDuelTeamTime; // time the player switched duel teams

	int switchClassTime; // class changed debounce timer

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int timeResidual;

	qboolean noHead;
	// Facial Expression Timers

	float facial_blink; // time before next blink. If a minus value, we are in blink mode
	float facial_timer; // time before next alert, frown or smile. If a minus value, we are in anim mode
	int facial_anim; // anim to show in anim mode

	char* areabits;

	int g2LastSurfaceHit; //index of surface hit during the most recent ghoul2 collision performed on this client.
	int g2LastSurfaceTime; //time when the surface index was set (to make sure it's up to date)
	int g2LastSurfaceModel;

	int corrTime;

	vec3_t lastHeadAngles;
	int lookTime;

	movetype_t moveType;

	int brokenLimbs;

	qboolean noCorpse; //don't leave a corpse on respawn this time.

	int jetPackTime;

	qboolean jetPackOn;
	int jetPackToggleTime;
	int jetPackDebRecharge;
	int jetPackDebReduce;

	qboolean flamethrowerOn;

	qboolean IsSprinting;

	qboolean cloak_is_charging;

	int sprintkDebRecharge;
	int sprintDebReduce;

	int cloakToggleTime;
	int cloakDebRecharge;
	int cloakDebReduce;

	int saberStoredIndex;
	//stores saberEntityNum from playerstate for when it's set to 0 (indicating saber was knocked out of the air)

	int saberKnockedTime; //if saber gets knocked away, can't pull it back until this value is < level.time

	vec3_t olderSaberBase; //Set before lastSaberBase_Always, to whatever lastSaberBase_Always was previously
	qboolean olderIsValid; //is it valid?

	vec3_t lastSaberDir_Always; //every getboltmatrix, set to saber dir
	vec3_t lastSaberBase_Always; //every getboltmatrix, set to saber base
	int lastSaberStorageTime;
	//server time that the above two values were updated (for making sure they aren't out of date)

	qboolean hasCurrentPosition; //are lastSaberTip and lastSaberBase valid?

	int dangerTime; // level.time when last attack occured

	int idleTime; //keep track of when to play an idle anim on the client.

	int idleHealth; //stop idling if health decreases
	vec3_t idleViewAngles; //stop idling if viewangles change

	int forcePowerSoundDebounce; //if > level.time, don't do certain sound events again (drain sound, absorb sound, etc)

	char modelname[MAX_QPATH];

	qboolean fjDidJump;

	qboolean ikStatus;

	int throwingIndex;
	int beingThrown;
	int doingThrow;

	float hiddenDist; //How close ents have to be to pick you up as an enemy
	vec3_t hiddenDir; //Normalized direction in which NPCs can't see you (you are hidden)

	renderInfo_t renderInfo;

	//mostly NPC stuff:
	npcteam_t playerTeam;
	npcteam_t enemyTeam;
	char* squadname;
	gentity_t* team_leader;
	gentity_t* leader;
	gentity_t* follower;
	int numFollowers;
	gentity_t* formationGoal;
	int nextFormGoal;
	class_t NPC_class;
	bclass_t nextbotclass;
	bclass_t botclass;

	vec3_t pushVec;
	int pushVecTime;

	int siegeClass;
	int holdingObjectiveItem;

	//time values for when being healed/supplied by supplier class
	int isMedHealed;
	int isMedSupplied;

	//seperate debounce time for refilling someone's ammo as a supplier
	int medSupplyDebounce;

	//used in conjunction with ps.hackingTime
	int isHacking;
	vec3_t hackingAngles;

	//debounce time for sending extended siege data to certain classes
	int siegeEDataSend;

	int ewebIndex; //index of e-web gun if spawned
	int ewebTime; //e-web use debounce
	int ewebHealth; //health of e-web (to keep track between deployments)

	int inSpaceIndex; //ent index of space trigger if inside one
	int inSpaceSuffocation; //suffocation timer

	int tempSpectate; //time to force spectator mode

	//keep track of last person kicked and the time so we don't hit multiple times per kick
	int jediKickIndex;
	int jediKickTime;

	//special moves (designed for kyle boss npc, but useable by players in mp)
	int grappleIndex;
	int grappleState;

	int solidHack;

	int noLightningTime;

	unsigned mGameFlags;

	//fallen duelist
	qboolean iAmALoser;

	int lastGenCmd;
	int lastGenCmdTime;
	vec3_t prevviewangle;
	int prevviewtime;

	//the saberNum of the last enemy blade that you hit.
	int lastSaberCollided;
	//the blade_num of the last enemy blade that you hit.
	int lastBladeCollided;

	sabimpact_t sabimpact[MAX_SABERS][MAX_BLADES];

	//racc - used to debounce saber projectile blocks.
	int DodgeDebounce;
	int ManualDodgeDebounce;
	int MishapDebounce;
	int viewLockTime;
	int otherKillerMOD;
	int otherKillerVehWeapon;
	int otherKillerWeaponType;
	int savedHP;
	int savedArmor;
	int chatDebounceTime;
	qboolean skillUpdated;
	int skillDebounce;
	int skillLevel[NUM_SKILLS];
	int flameTime;
	qboolean flamed; //When a jedi gets flamethrowed
	qboolean flameThrowed;
	gentity_t* remote;
	qboolean leftPistol;

	//dismember tracker
	qboolean dismembered;
	char dismemberProbLegs; // probability of the legs being dismembered (located in NPC.cfg, 0 = never, 100 = always)
	char dismemberProbHead; // probability of the head being dismembered (located in NPC.cfg, 0 = never, 100 = always)
	char dismemberProbArms; // probability of the arms being dismembered (located in NPC.cfg, 0 = never, 100 = always)
	char dismemberProbHands; // probability of the hands being dismembered (located in NPC.cfg, 0 = never, 100 = always)
	char dismemberProbWaist; // probability of the waist being dismembered (located in NPC.cfg, 0 = never, 100 = always)

	struct force
	{
		int regenDebounce;
		int drainDebounce;
		int lightningDebounce;
	} force;

	int hookDebounceTime;
	qboolean hookhasbeenfired;
	gentity_t* owner;
	int poisonDamage; // Amount of poison damage to be given
	int poisonTime; // When to apply poison damage

	int stunDebounceTime;
	qboolean stunhasbeenfired;

	int stunDamage; // Amount of stun damage to be given
	int stunTime; // When to apply stun damage

	signed char forced_forwardmove;
	signed char forced_rightmove;

	int AmputateDamage; // Amount of damage to be given
	int AmputateTime; // When to apply damage

#define MAX_FOLLOWERS 64

	gentity_t* playerLeader;

	follower_t plFollower[MAX_FOLLOWERS];
	int numPlFollowers;

	char botSoundDir[MAX_QPATH];

	int frozenTime;

	int MassiveBounceAnimTime;
	gentity_t* ManualBlockStaggerDefender;
	int StaggerAnimTime;
	int cloneFired;
	int BoltsFired;
	int Dash_Count;

	int VaderBreathTime;

	int			sphereshieldToggleTime;
	int			overloadToggleTime; 
	int respawnPending;   // time when the client should retry spawning
};

//animations
typedef struct
{
	char filename[MAX_QPATH];
	animation_t animations[MAX_ANIMATIONS];
	animevent_t torsoAnimEvents[MAX_ANIM_EVENTS];
	animevent_t legsAnimEvents[MAX_ANIM_EVENTS];
	unsigned char torsoAnimEventCount;
	unsigned char legsAnimEventCount;
} animFileSet_t;

//Interest points

#define MAX_INTEREST_POINTS		64

typedef struct
{
	vec3_t origin;
	char* target;
} interestPoint_t;

//Combat points

#define MAX_COMBAT_POINTS		512

typedef struct
{
	vec3_t origin;
	int flags;
	//	char		*NPC_targetname;
	//	team_t		team;
	qboolean occupied;
	int waypoint;
	int dangerTime;
} combatPoint_t;

// Alert events

#define	MAX_ALERT_EVENTS	128

typedef enum
{
	AET_SIGHT,
	AET_SOUND,
} alertEventType_e;

typedef enum
{
	AEL_MINOR,
	//Enemy responds to the sound, but only by looking
	AEL_SUSPICIOUS,
	//Enemy looks at the sound, and will also investigate it
	AEL_DISCOVERED,
	//Enemy knows the player is around, and will actively hunt
	AEL_DANGER,
	//Enemy should try to find cover
	AEL_DANGER_GREAT,
	//Enemy should run like hell!
} alertEventLevel_e;

typedef struct alertEvent_s
{
	vec3_t position; //Where the event is located
	float radius; //Consideration radius
	alertEventLevel_e level; //Priority level of the event
	alertEventType_e type; //Event type (sound,sight)
	gentity_t* owner; //Who made the sound
	float light; //ambient light level at point
	float addLight; //additional light- makes it more noticable, even in darkness
	int ID;
	//unique... if get a ridiculous number, this will repeat, but should not be a problem as it's just comparing it to your lastAlertID
	int timestamp; //when it was created
	qboolean onGround;
} alertEvent_t;

//
// this structure is cleared as each map is entered
//
typedef struct waypointData_s
{
	char targetname[MAX_QPATH];
	char target[MAX_QPATH];
	char target2[MAX_QPATH];
	char target3[MAX_QPATH];
	char target4[MAX_QPATH];
	int nodeID;
} waypointData_t;

typedef struct
{
	char message[MAX_SPAWN_VARS_CHARS];
	int count;
	int cs_index;
	vec3_t origin;
} locationData_t;

typedef struct level_locals_s
{
	struct gclient_s* clients; // [maxclients]

	struct gentity_s* gentities;
	int gentitySize;
	int num_entities; // current number, <= MAX_GENTITIES

	int warmupTime; // restart match at this time

	fileHandle_t logFile;

	// store latched cvars here that we want to get at often
	int maxclients;

	int framenum;
	int time; // in msec
	int previousTime; // so movers can back up when blocked

	int startTime; // level.time the map was started

	int teamScores[TEAM_NUM_TEAMS];
	int lastTeamLocationTime; // last time of client team location update

	qboolean newSession; // don't use any old session data, because
	// we changed gametype

	qboolean restarted; // waiting for a map_restart to fire

	int numConnectedClients;
	int numNonSpectatorClients; // includes connecting clients
	int numPlayingClients; // connected, non-spectators
	int sortedClients[MAX_CLIENTS]; // sorted by score
	int follow1, follow2; // clientNums for auto-follow spectators

	int snd_fry; // sound index for standing in lava

	int snd_hack; //hacking loop sound
	int snd_medHealed; //being healed by supply class
	int snd_medSupplied; //being supplied by supply class

	// voting state
	char voteString[MAX_STRING_CHARS];
	char voteStringClean[MAX_STRING_CHARS];
	char voteDisplayString[MAX_STRING_CHARS];
	int voteTime; // level.time vote was called
	int voteExecuteTime; // time the vote is executed
	int voteExecuteDelay; // set per-vote
	int voteYes;
	int voteNo;
	int numVotingClients; // set by CalculateRanks

	qboolean votingGametype;
	int votingGametypeTo;

	// team voting state
	char teamVoteString[2][MAX_STRING_CHARS];
	char teamVoteStringClean[2][MAX_STRING_CHARS];
	char teamVoteDisplayString[2][MAX_STRING_CHARS];
	int teamVoteTime[2]; // level.time vote was called
	int teamVoteExecuteTime[2]; // time the vote is executed
	int teamVoteYes[2];
	int teamVoteNo[2];
	int numteamVotingClients[2]; // set by CalculateRanks

	// spawn variables
	qboolean spawning; // the G_Spawn*() functions are valid
	int numSpawnVars;
	char* spawnVars[MAX_SPAWN_VARS][2]; // key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	// intermission state
	int intermissionQueued; // intermission was qualified, but
	// wait INTERMISSION_DELAY_TIME before
	// actually going there so the last
	// frag can be watched.  Disable future
	// kills during this delay
	int intermissiontime; // time the intermission was started
	char* changemap;
	qboolean readyToExit; // at least one client wants to exit
	int exitTime;
	vec3_t intermission_origin; // also used for spectator spawns
	vec3_t intermission_angle;

	int bodyQueIndex; // dead bodies
	gentity_t* bodyQue[BODY_QUEUE_SIZE];
	int portalSequence;

	alertEvent_t alertEvents[MAX_ALERT_EVENTS];
	int numAlertEvents;
	int curAlertID;

	AIGroupInfo_t groups[MAX_FRAME_GROUPS];

	//Interest points- squadmates automatically look at these if standing around and close to them
	interestPoint_t interestPoints[MAX_INTEREST_POINTS];
	int numInterestPoints;

	//Combat points- NPCs in b_state BS_COMBAT_POINT will find their closest empty combat_point
	combatPoint_t combatPoints[MAX_COMBAT_POINTS];
	int numCombatPoints;

	//rwwRMG - added:
	int mNumBSPInstances;
	int mBSPInstanceDepth;
	vec3_t mOriginAdjust;
	float mRotationAdjust;
	char* mTargetAdjust;

	char mTeamFilter[MAX_QPATH];

	struct
	{
		fileHandle_t log;
	} security;

	struct
	{
		int num;
		char* infos[MAX_BOTS];
	} bots;

	struct
	{
		int num;
		char* infos[MAX_ARENAS];
	} arenas;

	struct
	{
		int num;
		qboolean linked;
		locationData_t data[MAX_LOCATIONS];
	} locations;

	gametype_t gametype;

	// current map name without the path from maps folder
	char sjemapname[128];

	// fix for vjun3 map. It crashs clients in MP because of the npc skin limit (16). It will not load the protocol_imp npc
	qboolean is_vjun3_map;
	qboolean is_t3_rift_map;
	qboolean is_t3_hevil_map;
	qboolean is_t1_fatal_map;
	qboolean is_t1_danger_map;
	qboolean is_t1_rail_map;
	qboolean is_t1_sour_map;
	qboolean is_t2_rouge_map;
	qboolean is_t2_trip_map;
	qboolean is_t3_byss_map;

	// tests if it is a sp map in loading time
	qboolean sp_map;
	qboolean is_outcast_map;
	qboolean is_outcast_mp_map;
	qboolean is_duel_mp_map;
	qboolean is_no_Dlight_map;

	// used by Entity System to save and load spawnstring of entities
	char* sje_spawn_strings[ENTITYNUM_MAX_NORMAL][128];

	// amount of keys and values stored in this entity
	int sje_spawn_strings_values_count[ENTITYNUM_MAX_NORMAL];

	char mapname[MAX_QPATH];
	char rawmapname[MAX_QPATH];
} level_locals_t;

qboolean sje_is_ally(const gentity_t* ent, const gentity_t* other);
int sje_number_of_allies(const gentity_t* ent);
//
// g_spawn.c
//
qboolean G_SpawnString(const char* key, const char* defaultString, char** out);
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean G_SpawnFloat(const char* key, const char* defaultString, float* out);
qboolean G_SpawnInt(const char* key, const char* defaultString, int* out);
qboolean G_SpawnVector(const char* key, const char* defaultString, float* out);
qboolean G_SpawnBoolean(const char* key, const char* defaultString, qboolean* out);
void G_SpawnEntitiesFromString(qboolean inSubBSP);
char* G_NewString(const char* string);

//
// g_cmds.c
//
void Cmd_Score_f(const gentity_t* ent);
void StopFollowing(gentity_t* ent);
void BroadcastTeamChange(gclient_t* client, int old_team);
void SetTeam(gentity_t* ent, const char* s);
void Cmd_FollowCycle_f(gentity_t* ent, int dir);
void Cmd_SaberAttackCycle_f(gentity_t* ent);
int G_ItemUsable(const playerState_t* ps, int forcedUse);
void Cmd_ToggleSaber_f(gentity_t* ent);
void Cmd_EngageDuel_f(gentity_t* ent);

//
// g_items.c
//
void ItemUse_Binoculars(const gentity_t* ent);
void ItemUse_Shield(gentity_t* ent);
void ItemUse_Sentry(gentity_t* ent);
void ItemUse_Sentry2(gentity_t* ent);

void Jetpack_Off(const gentity_t* ent);
void Jetpack_On(gentity_t* ent);
void ItemUse_Jetpack(gentity_t* ent);
void ItemUse_UseCloak(gentity_t* ent);
void ItemUse_UseSphereshield(gentity_t* ent);
void ItemUse_UseDisp(gentity_t* ent, int type);
void ItemUse_UseEWeb(gentity_t* ent);
void G_PrecacheDispensers(void);
void G_LoadDispensers(void);
void ItemUse_FlameThrower(const gentity_t* ent);

void ItemUse_Seeker(gentity_t* ent);
void ItemUse_MedPack(gentity_t* ent);
void ItemUse_MedPack_Big(gentity_t* ent);

void ItemUse_Decca(gentity_t* ent);
void ItemUse_Swoop(gentity_t* ent);

void G_CheckTeamItems(void);
void G_RunItem(gentity_t* ent);
void RespawnItem(gentity_t* ent);
void ResetItem(gentity_t* ent);

gentity_t* Drop_Item(gentity_t* ent, gitem_t* item, float angle);
gentity_t* LaunchItem(gitem_t* item, vec3_t origin, vec3_t velocity);
void G_SpawnItem(gentity_t* ent, gitem_t* item);
void FinishSpawningItem(gentity_t* ent);
void Add_Ammo(const gentity_t* ent, int weapon, int count);
void Touch_Item(gentity_t* ent, gentity_t* other, trace_t* trace);

void clear_registered_items(void);
void register_item(const gitem_t* item);
void save_registered_items(void);

//
// g_utils.c
//
int G_model_index(const char* name);
int G_SoundIndex(const char* name);
int G_SoundSetIndex(const char* name);
int G_EffectIndex(const char* name);
int G_BSPIndex(const char* name);
int G_IconIndex(const char* name);

qboolean G_PlayerHasCustomSkeleton(gentity_t* ent);

void G_TeamCommand(team_t team, char* cmd);
void G_ScaleNetHealth(gentity_t* self);
void g_kill_box(gentity_t* ent);
gentity_t* G_Find(gentity_t* from, int fieldofs, const char* match);
int G_RadiusList(vec3_t origin, float radius, const gentity_t* ignore, qboolean take_damage,
	gentity_t* ent_list[MAX_GENTITIES]);

void g_throw(gentity_t* targ, const vec3_t new_dir, float push);

void G_FreeFakeClient(gclient_t** cl);
void G_CreateFakeClient(int entNum, gclient_t** cl);
void G_CleanAllFakeClients(void);

void G_SetAnim(gentity_t* ent, usercmd_t* ucmd, int setAnimParts, int anim, int setAnimFlags, int blendTime);
gentity_t* G_PickTarget(char* targetname);
void GlobalUse(gentity_t* self, gentity_t* other, gentity_t* activator);
void G_UseTargets2(gentity_t* ent, gentity_t* activator, const char* string);
void G_UseTargets(gentity_t* ent, gentity_t* activator);
void G_SetMovedir(vec3_t angles, vec3_t movedir);
void G_SetAngles(gentity_t* ent, vec3_t angles);

void G_InitGentity(gentity_t* e);
gentity_t* G_Spawn(void);
gentity_t* G_TempEntity(vec3_t origin, int event);
gentity_t* G_PlayEffect(int fxID, vec3_t org, vec3_t ang);
gentity_t* G_PlayEffectID(int fxID, vec3_t org, vec3_t ang);
gentity_t* G_PlayBoltedEffect(int fxID, gentity_t* owner, const char* bolt);
gentity_t* G_ScreenShake(vec3_t org, const gentity_t* target, float intensity, int duration, qboolean global);
gentity_t* CGCam_BlockShakeMP(vec3_t org, const gentity_t* target, float intensity, int duration);
void G_MuteSound(int entnum, int channel);
void G_Sound(gentity_t* ent, int channel, int soundIndex);
void G_SoundAtLoc(vec3_t loc, int channel, int soundIndex);
void G_EntitySound(gentity_t* ent, soundChannel_t channel, int soundIndex);
void TryUse(gentity_t* ent);
void G_SendG2KillQueue(void);
void G_KillG2Queue(int entNum);
void G_FreeEntity(gentity_t* ed);
qboolean G_EntitiesFree(void);

qboolean G_ActivateBehavior(gentity_t* self, int bset);

void GetAnglesForDirection(const vec3_t p1, const vec3_t p2, vec3_t out);

#define CENTERPRINT_MAXSTRING		1024
void TextWrapCenterPrint(char orgtext[CENTERPRINT_MAXSTRING], char output[CENTERPRINT_MAXSTRING]);
//
// g_object.c
//

extern void G_RunObject(gentity_t* ent);

float* tv(float x, float y, float z);
char* vtos(const vec3_t v);

void G_AddPredictableEvent(const gentity_t* ent, int event, int eventParm);
void G_AddEvent(gentity_t* ent, int event, int event_parm);
void G_SetOrigin(gentity_t* ent, vec3_t origin);
qboolean G_CheckInSolid(gentity_t* self, qboolean fix);
void AddRemap(const char* oldShader, const char* newShader, float timeOffset);
const char* BuildShaderStateConfig(void);
/*
Ghoul2 Insert Start
*/
int G_BoneIndex(const char* name);

/*
Ghoul2 Insert End
*/

//
// g_combat.c
//
qboolean CanDamage(const gentity_t* targ, vec3_t origin);
void G_Damage(gentity_t* targ, gentity_t* inflictor, gentity_t* attacker, vec3_t dir, vec3_t point, int damage,
	int dflags, int mod);
qboolean g_radius_damage(vec3_t origin, gentity_t* attacker, float damage, float radius, const gentity_t* ignore,
	gentity_t* missile, int mod);
void body_die(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int means_of_death);
void TossClientWeapon(gentity_t* self, vec3_t direction, float speed);
void TossClientItems(gentity_t* self);
void ExplodeDeath(gentity_t* self);
void G_CheckForDismemberment(gentity_t* ent, const gentity_t* enemy, vec3_t point, int damage);
extern int gGAvoidDismember;
void G_DodgeDrain(const gentity_t* victim, const gentity_t* attacker, int amount);

// damage flags
#define DAMAGE_NORMAL				0x00000000	// No flags set.
#define DAMAGE_RADIUS				0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR				0x00000002	// amour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK			0x00000004	// do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION		0x00000008  // armor, shields, invulnerability, and god mode have no effect
#define DAMAGE_NO_TEAM_PROTECTION	0x00000010  // armor, shields, invulnerability, and god mode have no effect
//JK2 flags
#define DAMAGE_EXTRA_KNOCKBACK		0x00000040	// add extra knock back to this damage
#define DAMAGE_DEATH_KNOCKBACK		0x00000080	// only does knock back on death of target
#define DAMAGE_IGNORE_TEAM			0x00000100	// damage is always done, regardless of teams
#define DAMAGE_NO_DAMAGE			0x00000200	// do no actual damage but react as if damage was taken
#define DAMAGE_HALF_ABSORB			0x00000400	// half shields, half health
#define DAMAGE_HALF_ARMOR_REDUCTION	0x00000800	// This damage doesn't whittle down armor as efficiently.
#define DAMAGE_HEAVY_WEAP_CLASS		0x00001000	// Heavy damage
#define DAMAGE_NO_HIT_LOC			0x00002000	// No hit location
#define DAMAGE_NO_SELF_PROTECTION	0x00004000	// Dont apply half damage to self attacks
#define DAMAGE_NO_DISMEMBER			0x00008000	// Dont do dismemberment
#define DAMAGE_SABER_KNOCKBACK1		0x00010000	// Check the attacker's first saber for a knockbackScale
#define DAMAGE_SABER_KNOCKBACK2		0x00020000	// Check the attacker's second saber for a knockbackScale
#define DAMAGE_SABER_KNOCKBACK1_B2	0x00040000	// Check the attacker's first saber for a knockbackScale2
#define DAMAGE_SABER_KNOCKBACK2_B2	0x00080000	// Check the attacker's second saber for a knockbackScale2
#define DAMAGE_LIGHNING_KNOCKBACK	0x00100000	// small knock back for lightning death with saber
//
// g_exphysics.c
//
void G_RunExPhys(gentity_t* ent, float gravity, float mass, float bounce, qboolean autoKill, int* g2Bolts,
	int numG2Bolts);

//
// g_missile.c
//
void g_run_missile(gentity_t* ent);

gentity_t* CreateMissile(vec3_t org, vec3_t dir, float vel, int life,
	gentity_t* owner, qboolean alt_fire);
void g_bounce_projectile(vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout);
void g_explode_missile(gentity_t* ent);

void WP_FireBlasterMissile(gentity_t* ent, vec3_t start, vec3_t dir, qboolean alt_fire);

//
// g_mover.c
//
extern int BMS_START;
extern int BMS_MID;
extern int BMS_END;

#define SPF_BUTTON_USABLE		1
#define SPF_BUTTON_FPUSHABLE	2
void G_PlayDoorLoopSound(gentity_t* ent);
void G_PlayDoorSound(gentity_t* ent, int type);
void G_RunMover(gentity_t* ent);
void Touch_DoorTrigger(gentity_t* ent, gentity_t* other, trace_t* trace);

//
// g_trigger.c
//
void trigger_teleporter_touch(const gentity_t* self, gentity_t* other, trace_t* trace);

//
// g_misc.c
//
#define MAX_REFNAME	32
#define	START_TIME_LINK_ENTS		FRAMETIME*1

#define	RTF_NONE	0
#define	RTF_NAVGOAL	0x00000001

typedef struct reference_tag_s
{
	char name[MAX_REFNAME];
	vec3_t origin;
	vec3_t angles;
	int flags; //Just in case
	int radius; //For nav goals
	qboolean inuse;
} reference_tag_t;

void TAG_Init(void);
reference_tag_t* TAG_Find(const char* owner, const char* name);
reference_tag_t* TAG_Add(const char* name, const char* owner, vec3_t origin, vec3_t angles, int radius, int flags);
int TAG_GetOrigin(const char* owner, const char* name, vec3_t origin);
int TAG_GetOrigin2(const char* owner, const char* name, vec3_t origin);
int TAG_GetAngles(const char* owner, const char* name, vec3_t angles);
int TAG_GetRadius(const char* owner, const char* name);
int TAG_GetFlags(const char* owner, const char* name);

void TeleportPlayer(gentity_t* player, vec3_t origin, vec3_t angles);

//
// g_weapon.c
//
void WP_FireTurretMissile(gentity_t* ent, vec3_t start, vec3_t dir, qboolean alt_fire, int damage, int velocity, int mod,
	const gentity_t* ignore);
void WP_FireGenericBlasterMissile(gentity_t* ent, vec3_t start, vec3_t dir, qboolean alt_fire, int damage, int velocity,
	int mod);
qboolean LogAccuracyHit(const gentity_t* target, const gentity_t* attacker);
void calcmuzzlePoint(const gentity_t* ent, const vec3_t in_forward, const vec3_t in_right,
	vec3_t muzzlePoint);
void SnapVectorTowards(vec3_t v, vec3_t to);

void Weapon_HookThink(gentity_t* ent);
void Weapon_HookFree(gentity_t* ent);
void Weapon_StunFree(gentity_t* ent);
void Weapon_StunThink(gentity_t* ent);

#define TD_VELOCITY			900		//max velocity for thermal dets

#define DISRUPTOR_MAX_CHARGE (level.gametype == GT_SIEGE ? 200 : 60)
#define BRYAR_PISTOL_ALT_DPDAMAGE			DODGE_BOLTBLOCK			//minimum DP damage of bryar secondary
#define BRYAR_PISTOL_ALT_DPMAXDAMAGE		DODGE_BOLTBLOCK*1.5		//maximum DP damage of bryar secondary
#define BRYAR_MAX_CHARGE					5

//
// g_client.c
//
int TeamCount(int ignoreclientNum, team_t team);
int TeamLeader(int team);
team_t PickTeam(int ignoreclientNum);
void SetClientViewAngle(gentity_t* ent, vec3_t angle);
gentity_t* SelectSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles, team_t team, qboolean isbot);
void MaintainBodyQueue(gentity_t* ent);
void ClientRespawn(gentity_t* ent);
void BeginIntermission(void);
void InitBodyQue(void);
void ClientSpawn(gentity_t* ent);
void player_die(gentity_t* self, const gentity_t* inflictor, gentity_t* attacker, int damage, int means_of_death);
void AddScore(const gentity_t* ent, int score);
void CalculateRanks(void);
qboolean SpotWouldTelefrag(const gentity_t* spot);

qboolean LMS_EnoughPlayers();

extern gentity_t* gJMSaberEnt;

//
// g_svcmds.c
//
qboolean ConsoleCommand(void);
void G_ProcessIPBans(void);
qboolean G_FilterPacket(char* from);

//
// g_weapon.c
//
void FireWeapon(gentity_t* ent, const qboolean alt_fire);
void BlowDetpacks(const gentity_t* ent);
void RemoveDetpacks(const gentity_t* ent);

//
// p_hud.c
//
void MoveClientToIntermission(gentity_t* ent);
void DeathmatchScoreboardMessage(const gentity_t* ent);

//
// g_cmds.c
//

//
// g_pweapon.c
//

//
// g_main.c
//
extern vmCvar_t g_ff_objectives;
extern qboolean gDoSlowMoDuel;
extern int gSlowMoDuelTime;

void G_PowerDuelCount(int* loners, int* doubles, qboolean countSpec);

void FindIntermissionPoint(void);
void SetLeader(int team, int client);
void CheckTeamLeader(int team);
void G_RunThink(gentity_t* ent);
void AddTournamentQueue(const gclient_t* client);
void QDECL G_LogPrintf(const char* fmt, ...);
void QDECL G_SecurityLogPrintf(const char* fmt, ...);
void SendScoreboardMessageToAllClients(void);
const char* G_GetStringEdString(char* refSection, char* refName);

//
// g_client.c
//
char* ClientConnect(int clientNum, qboolean firstTime, qboolean isBot);
qboolean client_userinfo_changed(int clientNum);
void ClientDisconnect(int clientNum);
void ClientBegin(int clientNum, qboolean allowTeamReset);
void G_BreakArm(gentity_t* ent, int arm);
void G_UpdateClientAnims(gentity_t* self, float animSpeedScale);
void ClientCommand(int clientNum);
void G_ClearVote(const gentity_t* ent);
void G_ClearTeamVote(const gentity_t* ent, int team);

//
// g_active.c
//
void G_CheckClientTimeouts(gentity_t* ent);
void ClientThink(int clientNum, const usercmd_t* ucmd);
void ClientEndFrame(gentity_t* ent);
void G_RunClient(gentity_t* ent);

//
// g_team.c
//
qboolean OnSameTeam(const gentity_t* ent1, const gentity_t* ent2);
void Team_CheckDroppedItem(const gentity_t* dropped);

typedef struct teamgame_s {
	float			last_flag_capture;
	int				last_capture_team;
	flagStatus_t	redStatus;	// CTF
	flagStatus_t	blueStatus;	// CTF
	flagStatus_t	flagStatus;	// One Flag CTF
	int				redTakenTime;
	int				blueTakenTime;
} teamgame_t;

//
// g_mem.c
//
void* G_Alloc(int size);
void G_InitMemory(void);
void Svcmd_GameMem_f(void);

//
// g_session.c
//
void G_ReadSessionData(gclient_t* client);
void G_InitSessionData(gclient_t* client, const char* userinfo, qboolean isBot);

void G_InitWorldSession(void);
void G_WriteSessionData(void);

void IT_LoadWeatherParms(void);

//
// NPC_senses.cpp
//
extern void AddSightEvent(gentity_t* owner, vec3_t position, float radius, alertEventLevel_e alertLevel,
	float addLight); //addLight = 0.0f
extern void AddSoundEvent(gentity_t* owner, vec3_t position, float radius, alertEventLevel_e alertLevel,
	qboolean needLOS, qboolean onGround);
extern qboolean G_CheckForDanger(const gentity_t* self, int alert_event);
extern int G_CheckAlertEvents(gentity_t* self, qboolean checkSight, qboolean checkSound, float maxSeeDist,
	float maxHearDist, int ignoreAlert, qboolean mustHaveOwner, int minAlertLevel);
//ignoreAlert = -1, mustHaveOwner = qfalse, minAlertLevel = AEL_MINOR
extern qboolean G_CheckForDanger(const gentity_t* self, int alert_event);
extern qboolean G_ClearLOS(gentity_t* self, const vec3_t start, const vec3_t end);
extern qboolean G_ClearLOS2(gentity_t* self, const gentity_t* ent, const vec3_t end);
extern qboolean G_ClearLOS3(gentity_t* self, const vec3_t start, const gentity_t* ent);
extern qboolean G_ClearLOS4(gentity_t* self, gentity_t* ent);
extern qboolean G_ClearLOS5(gentity_t* self, const vec3_t end);

//
// g_bot.c
//
void G_InitBots(void);
char* G_GetBotInfoByNumber(int num);
char* G_GetBotInfoByName(const char* name);
void G_CheckBotSpawn(void);
void G_RemoveQueuedBotBegin(int clientNum);
qboolean G_BotConnect(int clientNum, qboolean restart);
void Svcmd_AddBot_f(void);
void Svcmd_BotList_f(void);
qboolean G_DoesMapSupportGametype(const char* mapname, int gametype);
const char* G_RefreshNextMap(int gametype, qboolean forced);
void g_load_arenas(void);
void g_load_sp_arenas(void);

// w_force.c / w_saber.c
gentity_t* G_PreDefSound(vec3_t org, int pdSound);
qboolean HasSetSaberOnly(void);
void WP_ForcePowerStop(gentity_t* self, forcePowers_t forcePower);
void WP_SaberPositionUpdate(gentity_t* self, usercmd_t* ucmd);
void wp_saber_init_blade_data(const gentity_t* ent);
void WP_InitForcePowers(const gentity_t* ent);
void WP_SpawnInitForcePowers(gentity_t* ent);
void WP_ForcePowersUpdate(gentity_t* self, usercmd_t* ucmd);
int ForcePowerUsableOn(const gentity_t* attacker, const gentity_t* other, forcePowers_t forcePower);
void ForceHeal(gentity_t* self);
void ForceSpeed(gentity_t* self, int forceDuration);
void ForceRage(gentity_t* self);
void ForceGrip(const gentity_t* self);
void ForceProtect(gentity_t* self);
void ForceAbsorb(gentity_t* self);
void ForceTeamHeal(const gentity_t* self);
void ForceTeamForceReplenish(const gentity_t* self);
void ForceSeeing(gentity_t* self);
void ForceThrow(gentity_t* self, qboolean pull);
void ForceTelepathy(gentity_t* self);
qboolean jedi_dodge_evasion(gentity_t* self, const gentity_t* shooter, trace_t* tr, int hit_loc);
qboolean jedi_disruptor_dodge_evasion(gentity_t* self, gentity_t* shooter, vec3_t dmg_origin, int hit_loc);
void AnimateStun(gentity_t* self, gentity_t* inflictor, vec3_t impact);

// g_log.c
void QDECL G_LogWeaponPickup(int client, int weaponid);
void QDECL G_LogWeaponFire(int client, int weaponid);
void QDECL G_LogWeaponDamage(int client, int mod, int amount);
void QDECL G_LogWeaponKill(int client, int mod);
void QDECL G_LogWeaponDeath(int client, int weaponid);
void QDECL G_LogWeaponFrag(int attacker, int deadguy);
void QDECL G_LogWeaponPowerup(int client, int powerupid);
void QDECL G_LogWeaponItem(int client, int itemid);
void QDECL G_LogWeaponInit(void);
void QDECL G_LogWeaponOutput(void);
void QDECL G_ClearClientLog(int client);

// g_siege.c
void InitSiegeMode(void);
void G_SiegeClientExData(const gentity_t* msg_targ);

// g_timer
//Timing information
void TIMER_Clear(void);
void TIMER_Clear2(const gentity_t* ent);
void TIMER_Set(const gentity_t* ent, const char* identifier, int duration);
int TIMER_Get(const gentity_t* ent, const char* identifier);
qboolean TIMER_Done(const gentity_t* ent, const char* identifier);
qboolean TIMER_Start(const gentity_t* self, const char* identifier, int duration);
qboolean TIMER_Done2(const gentity_t* ent, const char* identifier, qboolean remove);
qboolean TIMER_Exists(const gentity_t* ent, const char* identifier);
void TIMER_Remove(const gentity_t* ent, const char* identifier);

float NPC_GetHFOVPercentage(vec3_t spot, vec3_t from, vec3_t facing, float hFOV);
float NPC_GetVFOVPercentage(vec3_t spot, vec3_t from, vec3_t facing, float vFOV);

extern void G_SetEnemy(gentity_t* self, gentity_t* enemy);
qboolean in_front(vec3_t spot, vec3_t from, vec3_t from_angles, float thresh_hold);

// ai_main.c
#define MAX_FILEPATH			144

int org_visible(vec3_t org1, vec3_t org2, int ignore);
void bot_order(gentity_t* ent, int clientNum, int ordernum);
int in_field_of_vision(vec3_t viewangles, float fov, vec3_t angles);

// ai_util.c
void B_InitAlloc(void);
void B_CleanupAlloc(void);

//bot settings
typedef struct bot_settings_s
{
	char personalityfile[MAX_FILEPATH];
	float skill;
	char team[MAX_FILEPATH];
} bot_settings_t;

int bot_ai_setup(int restart);
int bot_ai_shutdown(int restart);
int BotAILoadMap();
int bot_ai_setup_client(int client, const struct bot_settings_s* settings);
int BotAIShutdownClient(int client);
int bot_ai_startframe(const int time);
qboolean NPC_IsNotHavingEnoughForceSight(const gentity_t* self);

#include "g_team.h" // teamplay specific stuff

extern level_locals_t level;
extern gentity_t g_entities[MAX_GENTITIES];

#define	FOFS(x) offsetof(gentity_t, x)

// userinfo validation bitflags
// default is all except extended ascii
// numUserinfoFields + USERINFO_VALIDATION_MAX should not exceed 31
typedef enum userinfoValidationBits_e
{
	// validation & (1<<(numUserinfoFields+USERINFO_VALIDATION_BLAH))
	USERINFO_VALIDATION_SIZE = 0,
	USERINFO_VALIDATION_SLASH,
	USERINFO_VALIDATION_EXTASCII,
	USERINFO_VALIDATION_CONTROLCHARS,
	USERINFO_VALIDATION_MAX
} userinfoValidationBits_t;

void Svcmd_ToggleUserinfoValidation_f(void);
void Svcmd_ToggleAllowVote_f(void);

// g_cvar.c
#define XCVAR_PROTO
#include "g_xcvar.h"
#undef XCVAR_PROTO
void G_RegisterCvars(void);
void G_UpdateCvars(void);

extern gameImport_t* trap;
