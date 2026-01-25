/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
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

// Filename:-	sv_savegame.cpp
#include <memory>
#include "../server/exe_headers.h"

constexpr auto JPEG_IMAGE_QUALITY = 95;

#include "server.h"
#include "../qcommon/stringed_ingame.h"
#include "../game/statindex.h"

#include "qcommon/ojk_saved_game.h"
#include "qcommon/ojk_saved_game_helper.h"

static char save_game_comment[iSG_COMMENT_SIZE];

//#define SG_PROFILE	// enable for debug save stats if you want

int giSaveGameVersion; // filled in when a savegame file is opened

SavedGameJustLoaded_e e_saved_game_just_loaded = eNO;

char sLastSaveFileLoaded[MAX_QPATH] = { 0 };

#ifdef JK2_MODE
#define iSG_MAp_Cmd_SIZE (MAX_TOKEN_CHARS)
#else
#define iSG_MAp_Cmd_SIZE (MAX_QPATH)
#endif // JK2_MODE

static char* SG_GetSaveGameMapName(const char* ps_pathless_base_name);

#ifdef SG_PROFILE

class CChid
{
private:
	int		m_iCount;
	int		m_i_size;
public:
	CChid()
	{
		m_iCount = 0;
		m_i_size = 0;
	}
	void Add(int iLength)
	{
		m_iCount++;
		m_i_size += iLength;
	}
	int GetCount()
	{
		return m_iCount;
	}
	int GetSize()
	{
		return m_i_size;
	}
};

typedef map<unsigned int, CChid> CChidInfo_t;
CChidInfo_t	save_info;
#endif

static const char* GetString_FailedToOpenSaveGame(const char* ps_filename, qboolean b_open)
{
	static char s_temp[256];

	strcpy(s_temp, S_COLOR_RED);

#ifdef JK2_MODE
	const char* ps_reference = bOpen ? "MENUS3_FAILED_TO_OPEN_SAVEGAME" : "MENUS3_FAILED_TO_CREATE_SAVEGAME";
#else
	const char* ps_reference = b_open ? "MENUS_FAILED_TO_OPEN_SAVEGAME" : "MENUS3_FAILED_TO_CREATE_SAVEGAME";
#endif
	Q_strncpyz(s_temp + strlen(s_temp), va(SE_GetString(ps_reference), ps_filename), sizeof s_temp);
	strcat(s_temp, "\n");
	return s_temp;
}

void SG_WipeSavegame(
	const char* ps_pathless_base_name)
{
	ojk::SavedGame::remove(
		ps_pathless_base_name);
}

// called from the ERR_DROP stuff just in case the error occurred during loading of a saved game, because if
//	we didn't do this then we'd run out of quake file handles after the 8th load fail...
//
void SG_Shutdown()
{
	ojk::SavedGame& saved_game = ojk::SavedGame::get_instance();

	saved_game.close();

	e_saved_game_just_loaded = eNO;
	// important to do this if we ERR_DROP during loading, else next map you load after
	// a bad save-file you'll arrive at dead :-)

	// and this bit stops people messing up the ladder by repeatedly stabbing at the load key during loads...
	//
	extern qboolean gbAlreadyDoingLoad;
	gbAlreadyDoingLoad = qfalse;
}

void SV_WipeGame_f()
{
	if (Cmd_Argc() != 2)
	{
		Com_Printf(S_COLOR_RED "USAGE: wipe <name>\n");
		return;
	}
	if (!Q_stricmp(Cmd_Argv(1), "auto"))
	{
		Com_Printf(S_COLOR_RED "Can't wipe 'auto'\n");
		return;
	}
	SG_WipeSavegame(Cmd_Argv(1));
}

/*
// Store given string in saveGameComment for later use when game is
// actually saved
*/
void SG_StoreSaveGameComment(const char* s_comment)
{
	memmove(save_game_comment, s_comment, iSG_COMMENT_SIZE);
}

qboolean SV_TryLoadTransition(const char* mapname)
{
	char* ps_filename = va("hub/%s", mapname);

	Com_Printf(S_COLOR_CYAN "Restoring game \"%s\"...\n", ps_filename);

	if (!SG_ReadSavegame(ps_filename))
	{
		//couldn't load a savegame
		return qfalse;
	}
#ifdef JK2_MODE
	Com_Printf(S_COLOR_CYAN "Done.\n");
#else
	Com_Printf(S_COLOR_CYAN "%s.\n", SE_GetString("MENUS_DONE"));
#endif

	return qtrue;
}

qboolean gbAlreadyDoingLoad = qfalse;

void SV_LoadGame_f()
{
	if (gbAlreadyDoingLoad)
	{
		Com_DPrintf("( Already loading, ignoring extra 'load' commands... )\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf("USAGE: load <filename>\n");
		return;
	}

	const char* ps_filename = Cmd_Argv(1);
	if (strstr(ps_filename, "..") || strstr(ps_filename, "/") || strstr(ps_filename, "\\"))
	{
		Com_Printf(S_COLOR_RED "Bad load game name.\n");
		return;
	}

	if (!Q_stricmp(ps_filename, "current"))
	{
		Com_Printf(S_COLOR_RED "Can't load from \"current\"\n");
		return;
	}

	// special case, if doing a respawn then check that the available auto-save (if any) is from the same map
	//	as we're currently on (if in a map at all), if so, load that "auto", else re-load the last-loaded file...
	//
	if (!Q_stricmp(ps_filename, "*respawn"))
	{
		ps_filename = "auto"; // default to standard respawn behaviour

		// see if there's a last-loaded file to even check against as regards loading...
		//
		if (sLastSaveFileLoaded[0])
		{
			const char* ps_server_info = sv.configstrings[CS_SERVERINFO];
			const char* ps_map_name = Info_ValueForKey(ps_server_info, "mapname");

			const char* ps_map_name_of_auto_save = SG_GetSaveGameMapName("auto");

			if (!Q_stricmp(ps_map_name, "_brig"))
			{
				//if you're in the brig and there is no autosave, load the last loaded savegame
				if (!ps_map_name_of_auto_save)
				{
					ps_filename = sLastSaveFileLoaded;
				}
			}
			else
			{
#ifdef USE_LAST_SAVE_FROM_THIS_MAP
				// if the map name within the name of the last save file we explicitly loaded is the same
				//	as the current map, then use that...
				//
				char* psMapNameOfLastSaveFileLoaded = SG_GetSaveGameMapName(sLastSaveFileLoaded);

				if (!Q_stricmp(psMapName, psMapNameOfLastSaveFileLoaded)))
				{
					psFilename = sLastSaveFileLoaded;
				}
				else
#endif
					if (!(ps_map_name && ps_map_name_of_auto_save && !Q_stricmp(ps_map_name, ps_map_name_of_auto_save)))
					{
						// either there's no auto file, or it's from a different map to the one we've just died on...
						//
						ps_filename = sLastSaveFileLoaded;
					}
			}
		}
		//default will continue to load auto
	}
#ifdef JK2_MODE
	Com_Printf(S_COLOR_CYAN "Loading game \"%s\"...\n", psFilename);
#else
	Com_Printf(S_COLOR_CYAN "%s\n", va(SE_GetString("MENUS_LOADING_MAPNAME"), ps_filename));
#endif

	gbAlreadyDoingLoad = qtrue;
	if (!SG_ReadSavegame(ps_filename))
	{
		gbAlreadyDoingLoad = qfalse;
		//	do NOT do this here now, need to wait until client spawn, unless the load failed.
	}
	else
	{
#ifdef JK2_MODE
		Com_Printf(S_COLOR_CYAN "Done.\n");
#else
		Com_Printf(S_COLOR_CYAN "%s.\n", SE_GetString("MENUS_DONE"));
#endif
	}
}

qboolean SG_GameAllowedToSaveHere(qboolean inCamera);

//JLF notes
//	save game will be in charge of creating a new directory
void SV_SaveGame_f()
{
	// check server is running
	//

	if (!com_sv_running->integer)
	{
		Com_Printf(S_COLOR_RED "Server is not running\n");
		return;
	}

	if (sv.state != SS_GAME)
	{
		Com_Printf(S_COLOR_RED "You must be in a game to save.\n");
		return;
	}

	// check args...
	//
	if (Cmd_Argc() != 2)
	{
		Com_Printf("USAGE: save <filename>\n");
		return;
	}

	if (svs.clients[0].frames[svs.clients[0].netchan.outgoingSequence & PACKET_MASK].ps.stats[STAT_HEALTH] <= 0)
	{
#ifdef JK2_MODE
		Com_Printf(S_COLOR_RED "\nCan't savegame while dead!\n");
#else
		Com_Printf(S_COLOR_RED "\n%s\n", SE_GetString("SP_INGAME_CANT_SAVE_DEAD"));
#endif
		return;
	}

	const gentity_t* svent = SV_Gentity_num(0);
	if (svent->client->stats[STAT_HEALTH] <= 0)
	{
#ifdef JK2_MODE
		Com_Printf(S_COLOR_RED "\nCan't savegame while dead!\n");
#else
		Com_Printf(S_COLOR_RED "\n%s\n", SE_GetString("SP_INGAME_CANT_SAVE_DEAD"));
#endif
		return;
	}

	const char* psFilename = Cmd_Argv(1);
	char filename[MAX_QPATH] = { 0 };

	Q_strncpyz(filename, psFilename, sizeof filename);

	if (!Q_stricmp(filename, "current"))
	{
		Com_Printf(S_COLOR_RED "Can't save to 'current'\n");
		return;
	}

	if (strstr(filename, "..") || strstr(filename, "/") || strstr(filename, "\\"))
	{
		Com_Printf(S_COLOR_RED "Bad savegame name.\n");
		return;
	}

	if (!SG_GameAllowedToSaveHere(qfalse)) //full check
		return; // this prevents people saving via quick-save now during cinematic.

#ifdef JK2_MODE
	if (!Q_stricmp(filename, "quik*") || !Q_stricmp(filename, "auto*"))
	{
		SCR_PrecacheScreenshot();
		if (filename[4] == '*')
			filename[4] = 0;	//remove the *
		SG_StoreSaveGameComment("");	// clear previous comment/description, which will force time/date comment.
	}
#else
	if (!Q_stricmp(filename, "auto"))
	{
		SG_StoreSaveGameComment(""); // clear previous comment/description, which will force time/date comment.
	}
#endif

#ifdef JK2_MODE
	Com_Printf(S_COLOR_CYAN "Saving game \"%s\"...\n", filename);
#else
	Com_Printf(S_COLOR_CYAN "%s \"%s\"...\n", SE_GetString("CON_TEXT_SAVING_GAME"), filename);
#endif

	if (SG_WriteSavegame(filename, qfalse))
	{
#ifdef JK2_MODE
		Com_Printf(S_COLOR_CYAN "Done.\n");
#else
		Com_Printf(S_COLOR_CYAN "%s.\n", SE_GetString("MENUS_DONE"));
#endif
	}
	else
	{
#ifdef JK2_MODE
		Com_Printf(S_COLOR_RED "Failed.\n");
#else
		Com_Printf(S_COLOR_RED "%s.\n", SE_GetString("MENUS_FAILED_TO_OPEN_SAVEGAME"));
#endif
	}
}

//---------------
static void WriteGame(const qboolean autosave)
{
	ojk::SavedGameHelper saved_game(
		&ojk::SavedGame::get_instance());

	saved_game.write_chunk<int32_t>(
		INT_ID('G', 'A', 'M', 'E'),
		autosave);

	if (autosave)
	{
		// write out player ammo level, health, etc...
		//
		extern void SV_Player_EndOfLevelSave();
		SV_Player_EndOfLevelSave(); // this sets up the various cvars needed, so we can then write them to disk
		//
		char s[MAX_STRING_CHARS] = {};

		Cvar_VariableStringBuffer(sCVARNAME_PLAYERSAVE, s, sizeof s);

		saved_game.write_chunk(
			INT_ID('C', 'V', 'S', 'V'),
			s);

		// write ammo...
		//
		memset(s, 0, sizeof s);
		Cvar_VariableStringBuffer("playerammo", s, sizeof s);

		saved_game.write_chunk(
			INT_ID('A', 'M', 'M', 'O'),
			s);

		// write inventory...
		//
		memset(s, 0, sizeof s);
		Cvar_VariableStringBuffer("playerinv", s, sizeof s);

		saved_game.write_chunk(
			INT_ID('I', 'V', 'T', 'Y'),
			s);

		// the new JK2 stuff - force powers, etc...
		//
		memset(s, 0, sizeof s);
		Cvar_VariableStringBuffer("playerfplvl", s, sizeof s);

		saved_game.write_chunk(
			INT_ID('F', 'P', 'L', 'V'),
			s);
	}
}

static qboolean ReadGame()
{
	qboolean qb_auto_save = qfalse;

	ojk::SavedGameHelper saved_game(
		&ojk::SavedGame::get_instance());

	saved_game.read_chunk<int32_t>(
		INT_ID('G', 'A', 'M', 'E'),
		qb_auto_save);

	if (qb_auto_save)
	{
		char s[MAX_STRING_CHARS] = { 0 };

		// read health/armour etc...
		//
		memset(s, 0, sizeof s);

		saved_game.read_chunk(
			INT_ID('C', 'V', 'S', 'V'),
			s);

		Cvar_Set(sCVARNAME_PLAYERSAVE, s);

		// read ammo...
		//
		memset(s, 0, sizeof s);

		saved_game.read_chunk(
			INT_ID('A', 'M', 'M', 'O'),
			s);

		Cvar_Set("playerammo", s);

		// read inventory...
		//
		memset(s, 0, sizeof s);

		saved_game.read_chunk(
			INT_ID('I', 'V', 'T', 'Y'),
			s);

		Cvar_Set("playerinv", s);

		// read force powers...
		//
		memset(s, 0, sizeof s);

		saved_game.read_chunk(
			INT_ID('F', 'P', 'L', 'V'),
			s);

		Cvar_Set("playerfplvl", s);
	}

	return qb_auto_save;
}

//---------------

// write all CVAR_SAVEGAME cvars
// these will be things like model, name, ...
//
extern cvar_t* cvar_vars;
// I know this is really unpleasant, but I need access for scanning/writing latched cvars during save games

void SG_WriteCvars()
{
	cvar_t* var;
	int i_count = 0;

	ojk::SavedGameHelper saved_game(
		&ojk::SavedGame::get_instance());

	// count the cvars...
	//
	for (var = cvar_vars; var; var = var->next)
	{
#ifdef JK2_MODE
		if (!(var->flags & (CVAR_SAVEGAME | CVAR_USERINFO)))
#else
		if (!(var->flags & CVAR_SAVEGAME))
#endif
		{
			continue;
		}
		i_count++;
	}

	// store count...
	//
	saved_game.write_chunk<int32_t>(
		INT_ID('C', 'V', 'C', 'N'),
		i_count);

	// write 'em...
	//
	for (var = cvar_vars; var; var = var->next)
	{
#ifdef JK2_MODE
		if (!(var->flags & (CVAR_SAVEGAME | CVAR_USERINFO)))
#else
		if (!(var->flags & CVAR_SAVEGAME))
#endif
		{
			continue;
		}

		saved_game.write_chunk(
			INT_ID('C', 'V', 'A', 'R'),
			var->name,
			static_cast<int>(strlen(var->name) + 1));

		saved_game.write_chunk(
			INT_ID('V', 'A', 'L', 'U'),
			var->string,
			static_cast<int>(strlen(var->string) + 1));
	}
}

void SG_ReadCvars()
{
	int i_count = 0;
	std::string ps_name;

	ojk::SavedGameHelper saved_game(
		&ojk::SavedGame::get_instance());

	saved_game.read_chunk<int32_t>(
		INT_ID('C', 'V', 'C', 'N'),
		i_count);

	for (int i = 0; i < i_count; ++i)
	{
		saved_game.read_chunk(
			INT_ID('C', 'V', 'A', 'R'));

		ps_name = static_cast<const char*>(
			saved_game.get_buffer_data());

		saved_game.read_chunk(
			INT_ID('V', 'A', 'L', 'U'));

		const auto psValue = static_cast<const char*>(
			saved_game.get_buffer_data());

		Cvar_Set(ps_name.c_str(), psValue);
	}
}

void SG_WriteServerConfigStrings()
{
	ojk::SavedGameHelper saved_game(
		&ojk::SavedGame::get_instance());

	int iCount = 0;
	int i;
	// not in FOR statement in case compiler goes weird by reg-optimizing it then failing to get the address later

	// count how many non-blank server strings there are...
	//
	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if (i != CS_SYSTEMINFO)
		{
			if (sv.configstrings[i] && strlen(sv.configstrings[i])) // paranoia... <g>
			{
				iCount++;
			}
		}
	}

	saved_game.write_chunk<int32_t>(
		INT_ID('C', 'S', 'C', 'N'),
		iCount);

	// now write 'em...
	//
	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if (i != CS_SYSTEMINFO)
		{
			if (sv.configstrings[i] && strlen(sv.configstrings[i]))
			{
				saved_game.write_chunk<int32_t>(
					INT_ID('C', 'S', 'I', 'N'),
					i);

				saved_game.write_chunk(
					INT_ID('C', 'S', 'D', 'A'),
					sv.configstrings[i],
					static_cast<int>(strlen(sv.configstrings[i]) + 1));
			}
		}
	}
}

void SG_ReadServerConfigStrings()
{
	// trash the whole table...
	//
	for (int i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if (i != CS_SYSTEMINFO)
		{
			if (sv.configstrings[i])
			{
				Z_Free(sv.configstrings[i]);
			}
			sv.configstrings[i] = CopyString("");
		}
	}

	// now read the replacement ones...
	//
	int i_count = 0;

	ojk::SavedGameHelper saved_game(
		&ojk::SavedGame::get_instance());

	saved_game.read_chunk<int32_t>(
		INT_ID('C', 'S', 'C', 'N'),
		i_count);

	Com_DPrintf("Reading %d configstrings...\n", i_count);

	for (int i = 0; i < i_count; i++)
	{
		int i_index = 0;

		saved_game.read_chunk<int32_t>(
			INT_ID('C', 'S', 'I', 'N'),
			i_index);

		saved_game.read_chunk(
			INT_ID('C', 'S', 'D', 'A'));

		const auto ps_name = static_cast<const char*>(
			saved_game.get_buffer_data());

		Com_DPrintf("Cfg str %d = %s\n", i_index, ps_name);

		//sv.configstrings[iIndex] = psName;
		SV_SetConfigstring(i_index, ps_name);
	}
}

static unsigned int SG_UnixTimestamp(const time_t& t)
{
	return static_cast<unsigned int>(t);
}

static void SG_WriteComment(const qboolean qbAutosave, const char* ps_map_name)
{
	ojk::SavedGameHelper saved_game(
		&ojk::SavedGame::get_instance());

	char s_comment[iSG_COMMENT_SIZE];

	if (qbAutosave || !*save_game_comment)
	{
		Com_sprintf(s_comment, sizeof s_comment, "---> %s", ps_map_name);
	}
	else
	{
		Q_strncpyz(s_comment, save_game_comment, sizeof s_comment);
	}

	saved_game.write_chunk(
		INT_ID('C', 'O', 'M', 'M'),
		s_comment);

	// Add Date/Time/Map stamp
	const unsigned int timestamp = SG_UnixTimestamp(time(nullptr));

	saved_game.write_chunk<uint32_t>(
		INT_ID('C', 'M', 'T', 'M'),
		timestamp);

	Com_DPrintf("Saving: current (%s)\n", s_comment);
}

static time_t SG_GetTime(const unsigned int timestamp)
{
	return timestamp;
}

// Test to see if the given file name is in the save game directory
// then grab the comment if it's there
//
int SG_GetSaveGameComment(
	const char* psPathlessBaseName,
	char* sComment,
	char* sMapName)
{
	int ret;

	ojk::SavedGame& saved_game = ojk::SavedGame::get_instance();

	ojk::SavedGameHelper sgh(
		&ojk::SavedGame::get_instance());

	if (!saved_game.open(
		psPathlessBaseName))
	{
		return 0;
	}

	// Read description
	//
	bool is_succeed = sgh.try_read_chunk(
		INT_ID('C', 'O', 'M', 'M'));

	if (is_succeed)
	{
		if (sComment)
		{
			if (sgh.get_buffer_size() == iSG_COMMENT_SIZE)
			{
				std::uninitialized_copy_n(
					static_cast<const char*>(sgh.get_buffer_data()),
					iSG_COMMENT_SIZE,
					sComment);
			}
			else
			{
				sComment[0] = '\0';
			}
		}
	}

	// Read timestamp
	//
	time_t tFileTime = SG_GetTime(0);

	if (is_succeed)
	{
		unsigned int file_time = 0;

		is_succeed = sgh.try_read_chunk<uint32_t>(
			INT_ID('C', 'M', 'T', 'M'),
			file_time);

		if (is_succeed)
		{
			tFileTime = SG_GetTime(
				file_time);
		}
	}

#ifdef JK2_MODE
	// Read screenshot
	//

	if (is_succeed)
	{
		size_t iScreenShotLength;

		is_succeed = sgh.try_read_chunk<uint32_t>(
			INT_ID('S', 'H', 'L', 'N'),
			iScreenShotLength);
	}

	if (is_succeed)
	{
		is_succeed = sgh.try_read_chunk(
			INT_ID('S', 'H', 'O', 'T'));
	}
#endif

	// Read mapname
	//
	if (is_succeed)
	{
		is_succeed = sgh.try_read_chunk(
			INT_ID('M', 'P', 'C', 'M'));

		if (is_succeed)
		{
			if (sMapName)
			{
				if (sgh.get_buffer_size() == iSG_MAp_Cmd_SIZE)
				{
					std::uninitialized_copy_n(
						static_cast<const char*>(sgh.get_buffer_data()),
						iSG_MAp_Cmd_SIZE,
						sMapName);
				}
				else
				{
					sMapName[0] = '\0';
				}
			}
		}
	}

	ret = tFileTime;

	saved_game.close();

	return ret;
}

// read the mapname field from the supplied savegame file
//
// returns NULL if not found
//
static char* SG_GetSaveGameMapName(const char* ps_pathless_base_name)
{
	static char sMapName[iSG_MAp_Cmd_SIZE] = { 0 };
	char* psReturn = nullptr;
	if (SG_GetSaveGameComment(ps_pathless_base_name, nullptr, sMapName))
	{
		psReturn = sMapName;
	}

	return psReturn;
}

// pass in qtrue to set as loading screen, else pass in pvDest to read it into there...
//
#ifdef JK2_MODE
static bool SG_ReadScreenshot(
	bool set_as_loading_screen,
	void* screenshot_ptr)
{
	bool is_succeed = true;

	ojk::SavedGameHelper saved_game(
		&ojk::SavedGame::get_instance());

	// get JPG screenshot data length...
	//
	size_t screenshot_length = 0;

	is_succeed = saved_game.try_read_chunk<uint32_t>(
		INT_ID('S', 'H', 'L', 'N'),
		screenshot_length);

	//
	// alloc enough space plus extra 4K for sloppy JPG-decode reader to not do memory access violation...
	//
	byte* jpeg_data = nullptr;

	if (is_succeed)
	{
		jpeg_data = static_cast<byte*>(::Z_Malloc(
			static_cast<int>(screenshot_length + 4096),
			TAG_TEMP_WORKSPACE,
			qfalse));
	}

	//
	// now read the JPG data...
	//
	if (is_succeed)
	{
		is_succeed = saved_game.try_read_chunk(
			INT_ID('S', 'H', 'O', 'T'),
			jpeg_data,
			static_cast<int>(screenshot_length));
	}

	//
	// decompress JPG data...
	//
	byte* image = nullptr;
	int width;
	int height;

	if (is_succeed)
	{
		::re.LoadJPGFromBuffer(
			jpeg_data,
			screenshot_length,
			&image,
			&width,
			&height);

		//
		// if the loaded image is the same size as the game is expecting, then copy it to supplied arg (if present)...
		//
		if (width == SG_SCR_WIDTH && height == SG_SCR_HEIGHT)
		{
			if (screenshot_ptr)
			{
				::memcpy(
					screenshot_ptr,
					image,
					SG_SCR_WIDTH * SG_SCR_HEIGHT * 4);
			}

			if (set_as_loading_screen)
			{
				::SCR_SetScreenshot(
					image,
					SG_SCR_WIDTH,
					SG_SCR_HEIGHT);
			}
		}
		else
		{
			is_succeed = false;
		}
	}

	if (jpeg_data)
	{
		::Z_Free(jpeg_data);
	}

	if (image)
	{
		::Z_Free(image);
	}

	return is_succeed;
}
// Gets the savegame screenshot
//
qboolean SG_GetSaveImage(
	const char* base_name,
	void* image_ptr)
{
	if (!base_name)
	{
		return qfalse;
	}

	ojk::SavedGame& saved_game = ojk::SavedGame::get_instance();

	if (!saved_game.open(base_name))
	{
		return qfalse;
	}

	bool is_succeed = true;

	ojk::SavedGameHelper sgh(
		&saved_game);

	is_succeed = sgh.try_read_chunk(
		INT_ID('C', 'O', 'M', 'M'));

	if (is_succeed)
	{
		is_succeed = sgh.try_read_chunk(
			INT_ID('C', 'M', 'T', 'M'));
	}

	if (is_succeed)
	{
		is_succeed = SG_ReadScreenshot(
			false,
			image_ptr);
	}

	saved_game.close();

	return is_succeed ? qtrue : qfalse;
}

static void SG_WriteScreenshot(qboolean qbAutosave, const char* psMapName)
{
	ojk::SavedGameHelper saved_game(
		&ojk::SavedGame::get_instance());

	byte* pbRawScreenShot = nullptr;
	byte* byBlank = nullptr;

	if (qbAutosave)
	{
		// try to read a levelshot (any valid TGA/JPG etc named the same as the map)...
		//
		int iWidth = SG_SCR_WIDTH;
		int iHeight = SG_SCR_HEIGHT;
		const size_t	bySize = SG_SCR_WIDTH * SG_SCR_HEIGHT * 4;
		byte* src, * dst;

		byBlank = new byte[bySize];
		pbRawScreenShot = SCR_TempRawImage_ReadFromFile(va("levelshots/%s.tga", psMapName), &iWidth, &iHeight, byBlank, qtrue);

		if (pbRawScreenShot)
		{
			for (int y = 0; y < iHeight; y++)
			{
				for (int x = 0; x < iWidth; x++)
				{
					src = pbRawScreenShot + 4 * (y * iWidth + x);
					dst = pbRawScreenShot + 3 * (y * iWidth + x);
					dst[0] = src[0];
					dst[1] = src[1];
					dst[2] = src[2];
				}
			}
		}
	}

	if (!pbRawScreenShot)
	{
		pbRawScreenShot = SCR_GetScreenshot(0);
	}

	size_t iJPGDataSize = 0;
	size_t bufSize = SG_SCR_WIDTH * SG_SCR_HEIGHT * 3;
	byte* pJPGData = (byte*)Z_Malloc(static_cast<int>(bufSize), TAG_TEMP_WORKSPACE, qfalse, 4);

#ifdef JK2_MODE
	bool flip_vertical = true;
#else
	bool flip_vertical = false;
#endif // JK2_MODE

	iJPGDataSize = re.SaveJPGToBuffer(pJPGData, bufSize, JPEG_IMAGE_QUALITY, SG_SCR_WIDTH, SG_SCR_HEIGHT, pbRawScreenShot, 0, flip_vertical);
	if (qbAutosave)
		delete[] byBlank;

	saved_game.write_chunk<uint32_t>(
		INT_ID('S', 'H', 'L', 'N'),
		iJPGDataSize);

	saved_game.write_chunk(
		INT_ID('S', 'H', 'O', 'T'),
		pJPGData,
		static_cast<int>(iJPGDataSize));

	Z_Free(pJPGData);
	SCR_TempRawImage_CleanUp();
}
#endif

qboolean SG_GameAllowedToSaveHere(const qboolean inCamera)
{
	if (!inCamera)
	{
		if (!com_sv_running || !com_sv_running->integer)
		{
			return qfalse; //		Com_Printf( S_COLOR_RED "Server is not running\n" );
		}

		if (CL_IsRunningInGameCinematic())
		{
			return qfalse; //nope, not during a video
		}

		if (sv.state != SS_GAME)
		{
			return qfalse; //		Com_Printf (S_COLOR_RED "You must be in a game to save.\n");
		}

		//No savegames from "_" maps
		if (!sv_mapname || sv_mapname->string != nullptr && sv_mapname->string[0] == '_')
		{
			return qfalse; //		Com_Printf (S_COLOR_RED "Cannot save on holo deck or brig.\n");
		}

		if (svs.clients[0].frames[svs.clients[0].netchan.outgoingSequence & PACKET_MASK].ps.stats[STAT_HEALTH] <= 0)
		{
			return qfalse; //		Com_Printf (S_COLOR_RED "\nCan't savegame while dead!\n");
		}
	}
	if (!ge)
		return inCamera; // only happens when called to test if inCamera

	return ge->GameAllowedToSaveHere();
}

qboolean SG_WriteSavegame(const char* psPathlessBaseName, qboolean qbAutosave)
{
	if (!qbAutosave && !SG_GameAllowedToSaveHere(qfalse)) //full check
		return qfalse; // this prevents people saving via quick-save now during cinematic

	const int iPrevTestSave = sv_testsave->integer;
	sv_testsave->integer = 0;

	// Write out server data...
	//
	const char* psServerInfo = sv.configstrings[CS_SERVERINFO];
	const char* psMapName = Info_ValueForKey(psServerInfo, "mapname");
	//JLF
#ifdef JK2_MODE
	if (!strcmp("quik", psPathlessBaseName))
#else
	if (strcmp("quick", psPathlessBaseName) == 0)
#endif
	{
		SG_StoreSaveGameComment(va("--> %s <--", psMapName));
	}

	ojk::SavedGame& saved_game = ojk::SavedGame::get_instance();

	if (!saved_game.create("current"))
	{
		Com_Printf(GetString_FailedToOpenSaveGame("current", qfalse)); //S_COLOR_RED "Failed to create savegame\n");
		SG_WipeSavegame("current");
		sv_testsave->integer = iPrevTestSave;
		return qfalse;
	}
	//END JLF

	ojk::SavedGameHelper sgh(
		&saved_game);

	char sMap_Cmd[iSG_MAp_Cmd_SIZE] = { 0 };
	Q_strncpyz(sMap_Cmd, psMapName, sizeof sMap_Cmd);

	SG_WriteComment(qbAutosave, sMap_Cmd);
#ifdef JK2_MODE
	SG_WriteScreenshot(qbAutosave, sMap_Cmd);
#endif

	sgh.write_chunk(
		INT_ID('M', 'P', 'C', 'M'),
		sMap_Cmd);

	SG_WriteCvars();

	WriteGame(qbAutosave);

	// Write out all the level data...
	//
	if (!qbAutosave)
	{
		sgh.write_chunk<int32_t>(
			INT_ID('T', 'I', 'M', 'E'),
			sv.time);

		sgh.write_chunk<int32_t>(
			INT_ID('T', 'I', 'M', 'R'),
			sv.timeResidual);

		CM_WritePortalState();
		SG_WriteServerConfigStrings();
	}
	ge->WriteLevel(qbAutosave); // always done now, but ent saver only does player if auto

	const bool is_write_failed = saved_game.is_failed();

	saved_game.close();

	if (is_write_failed)
	{
		Com_Printf(GetString_FailedToOpenSaveGame("current", qfalse)); //S_COLOR_RED "Failed to write savegame!\n");
		SG_WipeSavegame("current");
		sv_testsave->integer = iPrevTestSave;
		return qfalse;
	}

	ojk::SavedGame::rename(
		"current",
		psPathlessBaseName);

	sv_testsave->integer = iPrevTestSave;
	return qtrue;
}

qboolean SG_ReadSavegame
(
	const char* psPathlessBaseName)
{
	char sComment[iSG_COMMENT_SIZE];
	char sMap_Cmd[iSG_MAp_Cmd_SIZE];

#ifdef JK2_MODE
	Cvar_Set("cg_missionstatusscreen", "1");
#endif

	ojk::SavedGame& saved_game = ojk::SavedGame::get_instance();

	ojk::SavedGameHelper sgh(
		&saved_game);

	const int iPrevTestSave = sv_testsave->integer;

	ojk::ScopeGuard scope_guard(
		[&]()
		{
			sv_testsave->integer = 0;
		},

		[&]()
		{
			saved_game.close();

			sv_testsave->integer = iPrevTestSave;
		}
	);

	if (!saved_game.open(psPathlessBaseName))
	{
		Com_Printf(
			GetString_FailedToOpenSaveGame(
				psPathlessBaseName,
				qtrue));

		return qfalse;
	}

	// this check isn't really necessary, but it reminds me that these two strings may actually be the same physical one.
	//
	if (psPathlessBaseName != sLastSaveFileLoaded)
	{
		Q_strncpyz(
			sLastSaveFileLoaded,
			psPathlessBaseName,
			sizeof sLastSaveFileLoaded);
	}

	// Read in all the server data...
	//
	sgh.read_chunk(
		INT_ID('C', 'O', 'M', 'M'),
		sComment);

	Com_DPrintf(
		"Reading: %s\n",
		sComment);

	sgh.read_chunk(
		INT_ID('C', 'M', 'T', 'M'));

#ifdef JK2_MODE
	::SG_ReadScreenshot(
		true,
		nullptr);
#endif

	sgh.read_chunk(
		INT_ID('M', 'P', 'C', 'M'),
		sMap_Cmd);

	SG_ReadCvars();

	// read game state
	const qboolean qbAutosave = ReadGame();

	e_saved_game_just_loaded = qbAutosave ? eAUTO : eFULL;

	// note that this also trashes the whole G_Alloc pool as well (of course)
	::SV_SpawnServer(
		sMap_Cmd,
		eForceReload_NOTHING,
		e_saved_game_just_loaded != eFULL ? qtrue : qfalse);

	// read in all the level data...
	//
	if (!qbAutosave)
	{
		sgh.read_chunk<int32_t>(
			INT_ID('T', 'I', 'M', 'E'),
			sv.time);

		sgh.read_chunk<int32_t>(
			INT_ID('T', 'I', 'M', 'R'),
			sv.timeResidual);

		CM_ReadPortalState();
		SG_ReadServerConfigStrings();
	}

	// always done now, but ent reader only does player if auto
	ge->ReadLevel(
		qbAutosave,
		qbLoadTransition);

	return qtrue;
}

void SG_TestSave()
{
	if (sv_testsave->integer && sv.state == SS_GAME)
	{
		WriteGame(qfalse);
		ge->WriteLevel(qfalse);
	}
}