#include "cg_local.h"
#include <qcommon/q_shared.h>

extern int BG_SiegeGetPairedValue(const char* buf, char* key, char* outbuf);
extern int BG_SiegeGetValueGroup(const char* buf, char* group, char* outbuf);

stringID_table_t holsterTypeTable[] =
{
	ENUM2STRING(HLR_NONE),
	ENUM2STRING(HLR_SINGLESABER_1), //first single saber
	ENUM2STRING(HLR_SINGLESABER_2), //second single saber
	ENUM2STRING(HLR_STAFFSABER), //staff saber
	ENUM2STRING(HLR_PISTOL_L), //left hip blaster pistol
	ENUM2STRING(HLR_PISTOL_R), //right hip blaster pistol
	ENUM2STRING(HLR_BLASTER_L), //left hip blaster rifle
	ENUM2STRING(HLR_BLASTER_R), //right hip blaster rifle
	ENUM2STRING(HLR_BRYARPISTOL_L), //left hip bryer pistol
	ENUM2STRING(HLR_BRYARPISTOL_R), //right hip bryer pistol
	ENUM2STRING(HLR_BOWCASTER), //bowcaster
	ENUM2STRING(HLR_ROCKET_LAUNCHER), //rocket launcher
	ENUM2STRING(HLR_DEMP2), //demp2
	ENUM2STRING(HLR_CONCUSSION), //concussion
	ENUM2STRING(HLR_REPEATER), //repeater
	ENUM2STRING(HLR_FLECHETTE), //flechette
	ENUM2STRING(HLR_DISRUPTOR), //disruptor
	ENUM2STRING(MAX_HOLSTER),

	{ NULL, 0 }   // <-- REQUIRED TERMINATOR
};

stringID_table_t holsterBoneTable[] =
{
	ENUM2STRING(HOLSTER_NONE),
	ENUM2STRING(HOLSTER_UPPERBACK),
	ENUM2STRING(HOLSTER_LOWERBACK),
	ENUM2STRING(HOLSTER_LEFTHIP),
	ENUM2STRING(HOLSTER_RIGHTHIP)
};

static void InitHolsterData(clientInfo_t* ci)
{
	//initialize holster data with the premade defaults.
	for (int i = 0; i < MAX_HOLSTER; i++)
	{
		ci->holsterData[i].boneIndex = HOLSTER_NONE;
		VectorCopy(vec3_origin, ci->holsterData[i].posOffset);
		VectorCopy(vec3_origin, ci->holsterData[i].angOffset);
	}
}

extern char* BG_GetNextValueGroup(char* inbuf, char* outbuf);

void CG_LoadHolsterData(clientInfo_t* ci)
{
	// Adjusts the manual holster positional data based on the holster.cfg file
	// associated with the model or simply use the default values.

	fileHandle_t f;
	int f_len;

	// These buffers are large — make them static to avoid 16 KB+ stack usage.
	static char file_buffer[MAX_HOLSTER_INFO_SIZE];
	static char holster_type_group[MAX_HOLSTER_INFO_SIZE];

	InitHolsterData(ci);

	// Determine which holster.cfg to load
	if (!ci->skinName || !Q_stricmp("default", ci->skinName))
	{
		f_len = trap->FS_Open(va("models/players/%s/holster.cfg", ci->modelName), &f, FS_READ);

		if (!f)
		{
			f_len = trap->FS_Open("models/players/kyle/holster.cfg", &f, FS_READ);
		}
	}
	else
	{
		f_len = trap->FS_Open(va("models/players/%s/holster_%s.cfg",
			ci->modelName, ci->skinName), &f, FS_READ);

		if (!f)
		{
			f_len = trap->FS_Open(va("models/players/%s/holster.cfg", ci->modelName), &f, FS_READ);
		}

		if (!f)
		{
			f_len = trap->FS_Open("models/players/kyle/holster.cfg", &f, FS_READ);
		}
	}

	// Could not open file or empty file
	if (!f || !f_len)
	{
		return;
	}

	// Prevent overflow
	if (f_len >= MAX_HOLSTER_INFO_SIZE)
	{
		trap->FS_Close(f);
		return;
	}

	// Read file into static buffer
	trap->FS_Read(file_buffer, f_len, f);
	trap->FS_Close(f);

	// Parse file
	char* s = file_buffer;

	while ((s = BG_GetNextValueGroup(s, holster_type_group)) != NULL)
	{
		vec3_t vector_data = { 0, 0, 0 };
		char holster_type_value[MAX_QPATH];

		if (!BG_SiegeGetPairedValue(holster_type_group, "holsterType", holster_type_value))
		{
			continue;
		}

		const int i = GetIDForString(holsterTypeTable, holster_type_value);
		if (i == -1)
		{
			continue;
		}

		// Bone index
		if (BG_SiegeGetPairedValue(holster_type_group, "boneIndex", holster_type_value))
		{
			if (!Q_stricmp(holster_type_value, "disabled"))
			{
				ci->holsterData[i].boneIndex = HOLSTER_NONE;
			}
			else
			{
				ci->holsterData[i].boneIndex = GetIDForString(holsterBoneTable, holster_type_value);
			}
		}

		// Positional offset
		if (BG_SiegeGetPairedValue(holster_type_group, "posOffset", holster_type_value))
		{
			const int count = sscanf(holster_type_value, "%f, %f, %f",
				&vector_data[0], &vector_data[1], &vector_data[2]);

			if (count == 3)
			{
				VectorCopy(vector_data, ci->holsterData[i].posOffset);
			}
			else
			{
				Com_Printf("CG_LoadHolsterData WARNING: bad posOffset '%s'\n", holster_type_value);
			}
		}

		// Angular offset
		if (BG_SiegeGetPairedValue(holster_type_group, "angOffset", holster_type_value))
		{
			const int count = sscanf(holster_type_value, "%f, %f, %f",
				&vector_data[0], &vector_data[1], &vector_data[2]);

			if (count == 3)
			{
				VectorCopy(vector_data, ci->holsterData[i].angOffset);
			}
			else
			{
				Com_Printf("CG_LoadHolsterData WARNING: bad angOffset '%s'\n", holster_type_value);
			}
		}
	}

#ifdef _DEBUG
	Com_Printf("Holstered Weapon Data Loaded for %s.\n", ci->modelName);
#endif
}