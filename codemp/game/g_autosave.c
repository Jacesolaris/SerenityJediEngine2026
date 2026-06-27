#include "g_local.h"

//the max autosave file size define
#define MAX_AUTOSAVE_FILESIZE 1024

extern void Use_Autosave(gentity_t* ent, gentity_t* other, const gentity_t* activator);

static void Touch_Autosave(gentity_t* self, const gentity_t* other, trace_t* trace)
{
	//touch function used by SP_trigger_autosave
	if (!other || !other->inuse || !other->client || other->s.number >= MAX_CLIENTS)
	{
		//not a valid player
		return;
	}

	//activate the autosave.
	Use_Autosave(self, NULL, other);
}

extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);

void SpawnPointRender()
{
	//renders all the spawnpoints used for CoOp so that the CoOp editor can add more of them as needed.
	//warning: It's assumed that the debounce for this function is handled elsewhere (in this case BotWaypointRender)
	//without a debounce, this can overload and crash the game with events!
	gentity_t* spawnpoint = NULL;
	vec3_t end;

	while ((spawnpoint = G_Find(spawnpoint, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		//Scan thru all the coop spawnpoints
		if (spawnpoint->spawnflags & 1)
		{
			//this is a spawnpoint created by our coopeditor, display it.
			VectorCopy(spawnpoint->r.currentOrigin, end);
			end[2] += 20;
			G_TestLine(spawnpoint->r.currentOrigin, end, SABER_BLUE, FRAMETIME);
		}
	}
}

void AutosaveRender()
{
	//renders all the autosaves used for CoOp so that the CoOp editor can add more of them as needed.
	//warning: It's assumed that the debounce for this function is handled elsewhere (in this case BotWaypointRender)
	//without a debounce, this can overload and crash the game with events!
	gentity_t* autosave = NULL;

	while ((autosave = G_Find(autosave, FOFS(classname), "trigger_autosave")) != NULL)
	{
		//Scan thru all the coop autosaves
		if (autosave->spawnflags & AUTOSAVE_EDITOR)
		{
			vec3_t start;
			vec3_t end;
			//this is an autosave created by our coopeditor, display it.
			VectorAdd(autosave->r.currentOrigin, autosave->r.maxs, start);
			VectorAdd(autosave->r.currentOrigin, autosave->r.mins, end);
			G_TestLine(start, end, 0x000000, FRAMETIME);
		}
	}
}

/*QUAKED trigger_autosave (1 0 0) (-4 -4 -4) (4 4 4)
This is map entity is used by the Autosave Editor to place additional autosaves in premade
maps.  This should not be used by map creators as it must be called from the autosave
editor code.
*/
extern vmCvar_t bot_wp_edit;
extern void init_trigger(gentity_t* self);

static void SP_trigger_autosave(gentity_t* self)
{
	init_trigger(self);

	self->touch = Touch_Autosave;

	self->classname = "trigger_autosave";

	trap->LinkEntity((sharedEntity_t*)self);
}

extern void SP_info_player_start(gentity_t* ent);

void Create_Autosave(vec3_t origin, int size, const qboolean teleportPlayers)
{
	//create a new SP_trigger_autosave.
	gentity_t* newAutosave = G_Spawn();

	if (newAutosave)
	{
		G_SetOrigin(newAutosave, origin);

		if (size == -1)
		{
			//create a spawnpoint at this location
			SP_info_player_start(newAutosave);
			newAutosave->spawnflags |= 1; //set manually created flag

			//manually created spawnpoints have the lowest priority.
			newAutosave->genericValue1 = 50;
			return;
		}

		if (teleportPlayers)
		{
			//we want to force all players to teleport to this autosave.
			//use this to get around tricky scripting in the maps.
			newAutosave->spawnflags |= AUTOSAVE_TELE;
		}

		if (!size)
		{
			//default size
			size = 30;
		}

		//create bbox cube.
		newAutosave->r.maxs[0] = size;
		newAutosave->r.maxs[1] = size;
		newAutosave->r.maxs[2] = size;
		newAutosave->r.mins[0] = -size;
		newAutosave->r.mins[1] = -size;
		newAutosave->r.mins[2] = -size;

		SP_trigger_autosave(newAutosave);

		newAutosave->spawnflags |= AUTOSAVE_EDITOR;
		//indicate that this was manually created and should be rendered if we're in the coop editor.
	}
}

void Load_Autosaves(void)
{
	/* Load autosave entries from maps/<mapname>.autosp
	   - Ensure the read buffer is always NUL-terminated to avoid C6054.
	   - Check sscanf return values to avoid C6031.
	   - Avoid writing past buf by validating file length against buffer size.
	   - Convert parsed teleport flag into explicit qtrue/qfalse (no implicit bool->qboolean).
	*/

	fileHandle_t f = 0;
	char buf[MAX_AUTOSAVE_FILESIZE];
	char loadPath[MAX_QPATH];
	int sizeData = 0;
	vmCvar_t mapname;
	qboolean teleportPlayers = qfalse;

	Com_Printf("^5Loading Autosave File Data...");

	trap->Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);
	Com_sprintf(loadPath, sizeof(loadPath), "maps/%s.autosp", mapname.string);

	/* Open the autosave file. The API used in this codebase returns a length
	   (or 0) and fills the file handle. Keep the original semantics. */
	const int len = trap->FS_Open(loadPath, &f, FS_READ);
	if (!f)
	{
		Com_Printf("^5No autosave file found.\n");
		return;
	}
	if (len <= 0)
	{
		/* empty file or error */
		Com_Printf("^5Empty autosave file!\n");
		trap->FS_Close(f);
		return;
	}

	/* Protect against reading more than our buffer can hold */
	if (len >= (int)sizeof(buf))
	{
		Com_Printf("^1ERROR: Autosave file too large, truncating: %s\n", loadPath);
		/* Read only up to buffer-1 so we can NUL-terminate safely */
		const int toRead = (int)sizeof(buf) - 1;
		const int r = trap->FS_Read(buf, toRead, f);
		trap->FS_Close(f);
		if (r <= 0)
		{
			Com_Printf("^1ERROR: Failed to read autosave file (truncated read)\n");
			return;
		}
		buf[toRead] = '\0';
	}
	else
	{
		/* Normal case: read len bytes and NUL-terminate at buf[len] */
		const int r = trap->FS_Read(buf, len, f);
		trap->FS_Close(f);
		if (r != len)
		{
			Com_Printf("^1ERROR: Failed to read autosave file (expected %d got %d)\n", len, r);
			return;
		}
		buf[len] = '\0';
	}

	/* Parse the buffer line by line. Use pointer arithmetic bounded by len to avoid overruns. */
	char* s = buf;
	const char* bufEnd = buf + strlen(buf); /* safe because we NUL-terminated above */

	while (s < bufEnd && *s != '\0')
	{
		vec3_t positionData = { 0.0f, 0.0f, 0.0f };

		/* Skip leading newlines and whitespace */
		while (s < bufEnd && (*s == '\n' || *s == '\r'))
		{
			s++;
		}
		if (s >= bufEnd || *s == '\0')
		{
			break;
		}

		/* Parse one line: expect "float float float int int" */
		int tmpTeleport = 0;
		int parsed = sscanf(s, "%f %f %f %d %d",
			&positionData[0],
			&positionData[1],
			&positionData[2],
			&sizeData,
			&tmpTeleport);

		if (parsed == 5)
		{
			/* Convert parsed teleport int into explicit qboolean (no implicit conversion) */
			teleportPlayers = (tmpTeleport != 0) ? qtrue : qfalse;

			/* Only create autosave if the line parsed correctly */
			Create_Autosave(positionData, sizeData, teleportPlayers);
		}
		else
		{
			/* Log and skip malformed lines */
			Com_Printf("^1WARNING: Invalid autosave line skipped: \"%s\"\n", s);
		}

		/* Advance s to the start of the next line safely (bounded by bufEnd) */
		while (s < bufEnd && *s != '\n')
		{
			s++;
		}
		/* Skip the newline character if present */
		if (s < bufEnd && *s == '\n')
		{
			s++;
		}
	}

	Com_Printf("^5Done.\n");
}


void Save_Autosaves(void)
{
	//save the autosaves
	const fileHandle_t f = 0;

	char fileBuf[MAX_AUTOSAVE_FILESIZE] = { 0 };
	char loadPath[MAX_QPATH];
	vmCvar_t mapname;

	fileBuf[0] = '\0';

	Com_Printf("^5Saving Autosave File Data...");

	trap->Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);
	Com_sprintf(loadPath, MAX_QPATH, "maps/%s.autosp", mapname.string);

	if (!f)
	{
		Com_Printf("^5Couldn't create autosave file.\n");
	}
}

extern qboolean G_PointInBounds(vec3_t point, vec3_t mins, vec3_t maxs);

void Delete_Autosaves(const gentity_t* ent)
{
	// Move large array off the stack
	static int touch[MAX_GENTITIES];

	gentity_t* hit = NULL;
	vec3_t mins, maxs;

	// Compute bounding box
	VectorAdd(ent->r.currentOrigin, ent->r.mins, mins);
	VectorAdd(ent->r.currentOrigin, ent->r.maxs, maxs);

	const int num = trap->EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	// Remove autosave triggers inside the box
	for (int i = 0; i < num; i++)
	{
		hit = &g_entities[touch[i]];

		if (Q_stricmp(hit->classname, "trigger_autosave") == 0)
		{
			G_FreeEntity(hit);
		}
	}

	// Reset hit pointer for G_Find loop
	hit = NULL;

	// Remove manually placed spawnpoints inside the box
	while ((hit = G_Find(hit, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		if ((hit->spawnflags & 1) &&
			G_PointInBounds(hit->r.currentOrigin, mins, maxs))
		{
			G_FreeEntity(hit);
		}
	}
}