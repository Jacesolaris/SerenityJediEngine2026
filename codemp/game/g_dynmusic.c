//dynamic music code file
#include "g_local.h"
#include "g_dynmusic.h"

DynamicMusicGroup_t DMSData; //holds all our dynamic music data

void LoadDynamicMusicGroup(char* mapname, char* buffer);
void LoadDynamicMusic(void)
{
	// Tries to load dynamic music data for this map.
	fileHandle_t f;

	// FIX: move huge buffer off the stack
	static char buffer[DMS_INFO_SIZE];

	vmCvar_t mapname;

	// Open up the dynamic music file
	const int len = trap->FS_Open("ext_data/dms.dat", &f, FS_READ);

	if (!f)
	{
		// file open error
		Com_Printf("LoadDynamicMusic() Error: Couldn't open ext_data/dms.dat\n");
		return;
	}

	if (len >= DMS_INFO_SIZE)
	{
		// file too large for buffer
		Com_Printf("LoadDynamicMusic() Error: dms.dat too big.\n");
		trap->FS_Close(f);
		return;
	}

	// read data into static buffer
	trap->FS_Read(buffer, len, f);

	trap->FS_Close(f);

	trap->Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);

	LoadDynamicMusicGroup(mapname.string, buffer);
}

//init the DMS data for the given song/song type
extern int BG_SiegeGetPairedValue(const char* buf, char* key, char* outbuf);
extern int BG_SiegeGetValueGroup(const char* buf, char* group, char* outbuf);

static void LoadDMSSongData(const char* buffer, char* song, DynamicMusicSet_t* songData, char* mapname)
{
	// FIX: move huge buffers off the stack
	static char SongGroup[DMS_INFO_SIZE];
	static char transitionGroup[DMS_INFO_SIZE];

	int numTransitions = 0;
	int numExits = 0;

	BG_SiegeGetValueGroup(buffer, "musicfiles", SongGroup);

	// find our specific song
	if (!BG_SiegeGetValueGroup(SongGroup, song, SongGroup))
	{
		Com_Printf("LoadDMSSongData Error: Couldn't find song data for DMS song %s.\n", song);
		return;
	}

	// convert/store the name of the music file
	strcpy(songData->fileName, va("music/%s/%s.mp3", mapname, song));

	songData->numTransitions = 0; // init the struct's number of transitions.

	// start loading in transition data
	char* transition = strstr(SongGroup, "exit");
	while (transition)
	{
		char Value[MAX_QPATH];

		if (numTransitions >= MAX_DMS_TRANSITIONS)
		{
			Com_Printf("LoadDMSSongData Error: Too many transitions found.\n");
			return;
		}

		// setting up the new transition data slot
		songData->Transitions[numTransitions].numExitPoints = 0;

		// grab this transition group
		BG_SiegeGetValueGroup(transition, "exit", transitionGroup);

		// find transition file name
		BG_SiegeGetPairedValue(transitionGroup, "nextfile", Value);
		strcpy(songData->Transitions[numTransitions].fileName,
			va("music/%s/%s.mp3", mapname, Value));

		// load in exit points for this transition file
		while (BG_SiegeGetPairedValue(transitionGroup, va("time%i", numExits), Value))
		{
			if (numExits >= MAX_DMS_EXITPOINTS)
			{
				Com_Printf("LoadDMSSongData Error: Too many transitions found.\n");
				return;
			}

			songData->Transitions[numTransitions].exitPoints[numExits] = atoi(Value) * 1000;

			numExits++;
			songData->Transitions[numTransitions].numExitPoints++;
		}

		// increase the number of transitions in the songData
		songData->numTransitions++;

		numTransitions++;
		numExits = 0;

		// advance the transition pointer past the current exit data group
		transition += 4;
		transition = strstr(transition, "exit");
	}
}

static void LoadLengthforSong(const char* buffer, DynamicMusicSet_t* song)
{
	//load in the song lengths for the given DMS song
	char TempLength[MAX_QPATH];
	char token[MAX_QPATH];
	int transNum = song->numTransitions;

	//get length for the primary song

	//grab the token name
	char* tokenpointer = strrchr(song->fileName, '/');
	tokenpointer++;
	strcpy(token, tokenpointer);
	tokenpointer = strrchr(token, '.');
	*tokenpointer = '\0';

	BG_SiegeGetPairedValue(buffer, token, TempLength);
	song->fileLength = atoi(TempLength) * 1000;

	//find the song length for the transitions
	for (; transNum > 0; transNum--)
	{
		//grab pointer
		tokenpointer = strrchr(song->Transitions[transNum - 1].fileName, '/');
		tokenpointer++;
		strcpy(token, tokenpointer);
		tokenpointer = strrchr(token, '.');
		*tokenpointer = '\0';

		if (BG_SiegeGetPairedValue(buffer, token, TempLength))
		{
			song->Transitions[transNum - 1].fileLength = (int)(atof(TempLength) * (float)1000);
		}
		else
		{
			//couldn't find this music file's length, use default
			Com_Printf("LoadLengthforSong Warning: Couldn't find length for %s.\n", token);
			song->Transitions[transNum - 1].fileLength = DMS_MUSICFILE_DEFAULT;
		}
	}
}

//loads in the song lengths for the DMS music files
static void LoadDMSSongLengths(void)
{
	// FIX: move huge buffer off the stack
	static char buffer[DMS_INFO_SIZE];

	fileHandle_t f;

	if (!DMSData.valid)
	{
		// no DMSData means this map doesn't use DMS
		return;
	}

	// Open up the dynamic music file
	const int len = trap->FS_Open(DMS_MUSICLENGTH_FILENAME, &f, FS_READ);

	if (!f)
	{
		Com_Printf("LoadDynamicMusic() Error: Couldn't open ext_data/dms.dat\n");
		return;
	}

	if (len >= DMS_INFO_SIZE)
	{
		Com_Printf("LoadDynamicMusic() Error: dms.dat too big.\n");
		trap->FS_Close(f);
		return;
	}

	// read data into static buffer
	trap->FS_Read(buffer, len, f);

	trap->FS_Close(f);

	if (!BG_SiegeGetValueGroup(buffer, "musiclengths", buffer))
	{
		Com_Printf("LoadDMSSongLengths Error: Couldn't find musiclengths define group in musiclength.dat.\n");
	}

	if (DMSData.actionMusic.valid)
	{
		LoadLengthforSong(buffer, &DMSData.actionMusic);
	}

	if (DMSData.exploreMusic.valid)
	{
		LoadLengthforSong(buffer, &DMSData.exploreMusic);
	}

	if (DMSData.bossMusic.valid)
	{
		LoadLengthforSong(buffer, &DMSData.bossMusic);
	}
}

//loads in the DMS data for this map
void LoadDynamicMusicGroup(char* mapname, char* buffer)
{
	char text[MAX_QPATH];

	// FIX: move huge buffer off the stack
	static char MapMusicGroup[DMS_INFO_SIZE];

	// initialize DMSData
	DMSData.valid = qfalse;
	DMSData.actionMusic.valid = qfalse;
	DMSData.exploreMusic.valid = qfalse;
	DMSData.bossMusic.valid = qfalse;

	BG_SiegeGetValueGroup(buffer, "levelmusic", MapMusicGroup);

	if (!BG_SiegeGetValueGroup(MapMusicGroup, mapname, MapMusicGroup))
	{
		Com_Printf("LoadDynamicMusicGroup Error: Couldn't find DMS entry for this map.\n");
		return;
	}

	if (BG_SiegeGetPairedValue(MapMusicGroup, "uses", text))
	{
		// this map uses the dynamic music set of another map
		LoadDynamicMusicGroup(text, buffer);
		return;
	}

	// we have the dynamic music group for this map
	DMSData.valid = qtrue;
	DMSData.dmDebounceTime = -1;
	DMSData.dmBeatTime = 0;
	DMSData.dmState = DM_AUTO;
	DMSData.olddmState = DM_AUTO;

	if (BG_SiegeGetPairedValue(MapMusicGroup, "explore", text))
	{
		DMSData.exploreMusic.valid = qtrue;
		LoadDMSSongData(buffer, text, &DMSData.exploreMusic, mapname);
	}

	if (BG_SiegeGetPairedValue(MapMusicGroup, "action", text))
	{
		DMSData.actionMusic.valid = qtrue;
		LoadDMSSongData(buffer, text, &DMSData.actionMusic, mapname);
	}

	if (BG_SiegeGetPairedValue(MapMusicGroup, "boss", text))
	{
		DMSData.bossMusic.valid = qtrue;
		LoadDMSSongData(buffer, text, &DMSData.bossMusic, mapname);
	}

	LoadDMSSongLengths();
}

//Do the transitions between DMS action/explore DMS states
static void TransitionBetweenState(void)
{
	DynamicMusicSet_t* oldSongGroup;
	DynamicMusicSet_t* newSongGroup;

	if (DMSData.olddmState != DM_ACTION && DMSData.olddmState != DM_EXPLORE
		|| !DMSData.dmStartTime)
	{
		//not transitioning between action and explore, just start the music
		if (DMSData.dmState == DM_ACTION)
		{
			//want to switch to action
			trap->SetConfigstring(CS_MUSIC, DMSData.actionMusic.fileName);
		}
		else
		{
			//want to switch to explore
			trap->SetConfigstring(CS_MUSIC, DMSData.exploreMusic.fileName);
		}
		DMSData.olddmState = DMSData.dmState;
		DMSData.dmStartTime = level.time;
		return;
	}

	if (DMSData.dmState == DM_ACTION)
	{
		//transition is from explore's group
		oldSongGroup = &DMSData.exploreMusic;
		newSongGroup = &DMSData.actionMusic;
	}
	else
	{
		//transition from action
		oldSongGroup = &DMSData.actionMusic;
		newSongGroup = &DMSData.exploreMusic;
	}

	//find the closest transition point for this song state
	int songTime = level.time - DMSData.dmStartTime;
	while (songTime > oldSongGroup->fileLength)
	{
		//convert the start time to be in relation to the file length
		songTime -= oldSongGroup->fileLength;
	}

	//have the relative songTime, check to see if we're at one of the transition points
	for (int i = 0; i < oldSongGroup->numTransitions; i++)
	{
		for (int x = 0; x < oldSongGroup->Transitions[i].numExitPoints; x++)
		{
			const int TransitionTime = songTime - oldSongGroup->Transitions[i].exitPoints[x];
			if (TransitionTime == 0
				|| TransitionTime > 0 && TransitionTime < DMS_TRANSITIONFUDGEFACTOR)
			{
				//on the money or close enough
				trap->SetConfigstring(CS_MUSIC, va("%s %s",
					oldSongGroup->Transitions[i].fileName,
					newSongGroup->fileName));
				DMSData.olddmState = DMSData.dmState;
				DMSData.dmStartTime = level.time + oldSongGroup->Transitions[i].fileLength;
				return;
			}
		}
	}
}

//ported from SP
void G_DynamicMusicUpdate(void)
{
	int battle = 0;
	int entTeam;
	vec3_t mins, maxs;

	if (DMSData.dmDebounceTime >= 0 && DMSData.dmDebounceTime < level.time)
	{
		DMSData.dmDebounceTime = -1;
		DMSData.dmState = DM_AUTO;
		DMSData.olddmState = DM_AUTO;
	}

	if (DMSData.dmState == DM_DEATH)
	{
		if (DMSData.olddmState != DM_DEATH)
		{
			trap->SetConfigstring(CS_MUSIC, DMS_DEATH_MUSIC);
			DMSData.olddmState = DM_DEATH;
			DMSData.dmDebounceTime = level.time + DMS_DEATH_MUSIC_TIME;
		}
		return;
	}

	if (DMSData.dmState == DM_BOSS)
	{
		if (DMSData.olddmState != DM_BOSS)
		{
			trap->SetConfigstring(CS_MUSIC, DMSData.bossMusic.fileName);
			DMSData.olddmState = DM_BOSS;
		}
		return;
	}

	if (DMSData.dmState == DM_SILENCE)
	{
		if (DMSData.olddmState != DM_SILENCE)
		{
			trap->SetConfigstring(CS_MUSIC, "");
			DMSData.olddmState = DM_SILENCE;
		}
		return;
	}

	if (DMSData.dmBeatTime > level.time)
	{
		return;
	}

	DMSData.dmBeatTime = level.time + 1000;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		// FIX: move large array off the stack
		static int entity_list[MAX_GENTITIES];

		vec3_t center;
		gentity_t* player = &g_entities[i];

		if (!player || !player->inuse
			|| player->client->pers.connected == CON_DISCONNECTED
			|| player->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}

		VectorCopy(player->r.currentOrigin, center);

		for (int x = 0; x < 3; x++)
		{
			const int radius = 2048;
			mins[x] = center[x] - radius;
			maxs[x] = center[x] + radius;
		}

		const int num_listed_entities = trap->EntitiesInBox(mins, maxs, entity_list, MAX_GENTITIES);

		for (int e = 0; e < num_listed_entities; e++)
		{
			const gentity_t* ent = &g_entities[entity_list[e]];

			if (!ent || !ent->inuse)
			{
				continue;
			}

			if (!ent->client || !ent->NPC)
			{
				if (ent->classname &&
					(!Q_stricmp("PAS", ent->classname) ||
						!Q_stricmp("misc_turret", ent->classname)))
				{
					entTeam = ent->teamnodmg;
				}
				else
				{
					continue;
				}
			}
			else
			{
				entTeam = ent->client->playerTeam;
			}

			if (entTeam == player->client->playerTeam)
			{
				continue;
			}

			if (entTeam == NPCTEAM_FREE &&
				(!ent->enemy || !ent->enemy->client ||
					ent->enemy->client->playerTeam != player->client->playerTeam))
			{
				continue;
			}

			if (!trap->InPVS(player->r.currentOrigin, ent->r.currentOrigin))
			{
				continue;
			}

			if (ent->client && ent->s.weapon == WP_NONE)
			{
				continue;
			}

			if (ent->enemy == player &&
				((!ent->NPC || ent->NPC->confusionTime < level.time) ||
					(ent->client && ent->client->ps.weaponTime) ||
					(!ent->client && ent->attackDebounceTime > level.time)))
			{
				if (ent->health > 0)
				{
					if (ent->s.weapon == WP_SABER &&
						ent->client &&
						ent->client->ps.saberHolstered == 2 &&
						ent->enemy != player)
					{
						continue;
					}

					if (ent->NPC && ent->NPC->behaviorState == BS_CINEMATIC)
					{
						continue;
					}

					if (!ent->client &&
						ent->s.weapon == WP_TURRET &&
						ent->fly_sound_debounce_time &&
						ent->fly_sound_debounce_time - level.time < 10000)
					{
					}
					else if (ent->NPC && level.time < ent->NPC->shotTime)
					{
					}
					else
					{
						const int distSq = DistanceSquared(ent->r.currentOrigin, player->r.currentOrigin);

						if (distSq > 4194304)
						{
							continue;
						}

						if (distSq > 1048576)
						{
							const qboolean clearLOS =
								G_ClearLOS3(player, player->client->renderInfo.eyePoint, ent);

							if (clearLOS == qfalse)
							{
								continue;
							}
						}
					}

					battle++;
				}
			}
		}

		if (!battle)
		{
			const int alert = G_CheckAlertEvents(player, qtrue, qtrue, 1024, 1024, -1, qfalse, AEL_SUSPICIOUS);

			if (alert != -1)
			{
				if (G_CheckForDanger(player, alert))
				{
					battle = 1;
				}
			}
		}
	}

	if (battle)
	{
		SetDMSState(DM_ACTION);
	}
	else
	{
		SetDMSState(DM_EXPLORE);
	}

	if (DMSData.dmState != DMSData.olddmState)
	{
		TransitionBetweenState();
	}
}

//set the dynamic music system's desired state
void SetDMSState(const int DMSState)
{
	if (DMSData.valid && DMSData.olddmState != DMSState)
	{
		DMSData.dmState = DMSState;
		DMSData.dmDebounceTime = -1;
	}
}