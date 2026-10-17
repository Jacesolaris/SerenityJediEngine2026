// ui_sjechars.c -- SerenityJediEngine2026 MP: the in-game "Select Character" menu (ui/sjemp/ingameclass.menu,
// menuDef "ingameclassMenu", opened from the CHARACTERS box of Player Configuration).
//
// The characters are built here from the JKA/SJE NPC files (ext_data/npcs, then ext_data/mpnpcs; the first definition
// of a name wins), keeping only what the server lets a player play (same rules: BG_ModelBlocked, the class table
// BG_ClassModelBotclass and the skeleton test BG_ModelSkeletonFlags / BG_ClassSkeletonOk) and leaving out the models
// with a PlayerChoice.txt (they are in the Custom menu). One portrait per model folder; the folder's skins / saber
// loadouts are its variants. Factions, names, species, descriptions, variant labels and saber names can be changed in
// ext_data/sje_charmenu.dat (owner-editable, read at ui load).
//
// DH's pattern (models in zz_zzzz_SJE_Claude_Edits.pk3): every character has head_a1/torso_a1/lower_a1.skin in its folder
// and a variant (another skin) is a copy of the model under its own folder (e.g. stofficer = stormtrooper/officer), with
// charvariant.txt naming the model string it was made from. When those files exist the menu uses them
// ("<folder>/head_a1|torso_a1|lower_a1", like the Custom menu), else the plain model string ("folder/skin").
//
// APPLY sets model, sabers and saber colours; the server gives the class (weapons, items, Force) from the model.

#include "ui_local.h"
#include "../game/bg_classmodel.h"
#include "../game/teams.h"

extern int UI_ParseAnimationFile(const char* filename, animation_t* animset, qboolean is_humanoid);
extern qboolean ItemParse_asset_model_go(itemDef_t* item, const char* name, int* runTimeLength);
extern qboolean ItemParse_model_g2skin_go(itemDef_t* item, const char* skinName);
extern void UI_SaberAttachToChar(itemDef_t* item);
extern qboolean WP_SaberParseParm(const char* saber_name, const char* parmname, char* saberData);
extern saber_colors_t TranslateSaberColor(const char* name);
extern void Menu_SetItemText(const menuDef_t* menu, const char* itemName, const char* text);
extern void Menu_ShowItemByName(menuDef_t* menu, const char* p, qboolean bShow);

qboolean uiSJECharDualPreview = qfalse; // ui_saber.c: show both sabers on the preview model

#define SJE_CHARMENU		"ingameclassMenu"
#define SJE_MAX_CHARS		160
#define SJE_MAX_VARIANTS	12
#define SJE_MAX_SLOTS		20		// sje_slot0 .. sje_slot19 in the menu (2 rows of 10)
#define SJE_MAX_OVERRIDES	192
#define SJE_MAX_VARLABELS	128
#define SJE_MAX_SABERMAP	64
#define SJE_MAX_VFOLDERS	256
#define SJE_NPC_FILE_SIZE	262144
#define SJE_PLACEHOLDER_ICON "gfx/mp/custom_mp_default"

typedef enum
{
	SJEF_EMPIRE,
	SJEF_REBELS,
	SJEF_SITH,
	SJEF_JEDI,
	SJEF_BH,
	SJEF_NATIVES,
	SJEF_OTHER,
	SJEF_COUNT
} sjeFaction_t;

// the values of the menu's "fraction" multi (DH: Empire / Rebels / Sith / Jedi / bh / Natives / other)
static const char* const sjeFactionKeys[SJEF_COUNT] = { "empire", "rebels", "sith", "jedi", "bh", "natives", "other" };
// shown in the FACTION box (DH's names)
static const char* const sjeFactionNames[SJEF_COUNT] = { "Empire", "Rebellion", "Sith", "Jedi", "Bounty Hunters", "Natives", "Other" };

typedef struct sjeVariant_s
{
	char npc[48];				// NPC name it came from
	char modelString[MAX_QPATH];	// "folder", "folder/skin" or "folder/head|torso|lower" (what a bot would use)
	char skin[MAX_QPATH];		// "" = default skin
	char saber[2][48];			// NPC sabers ("" = none)
	char color[2][16];			// NPC saber colours (name or xRRGGBB)
	char label[32];
	char useModel[MAX_QPATH];	// model cvar on APPLY
	char g2Folder[MAX_QPATH];	// preview: models/players/<g2Folder>/model.glm
	char g2Skin[MAX_QPATH];		// preview skin
	int botclass;
} sjeVariant_t;

typedef struct sjeChar_s
{
	char folder[48];
	char name[64];
	char species[48];
	char weapon[64];
	const char* desc;			// override description ("@MENUS_X" or text), NULL = automatic
	char npcClass[32];
	char team[24];
	char weapons[2][32];			// first NPC's weapons (WP_*), for "Weapon of choice"
	int faction;
	int order;
	qboolean group;				// "Name: Various."
	qboolean hidden;
	float scale;				// preview model scale (1 = as is)
	char fullName[64];
	int numVariants;
	sjeVariant_t variants[SJE_MAX_VARIANTS];
	qhandle_t icon;
	qboolean iconTried;
} sjeChar_t;

typedef struct sjeOverride_s
{
	char folder[48];
	int faction;				// -1 = automatic
	char name[64];
	char species[48];
	char weapon[64];
	char desc[512];
	int order;
	int group;					// -1 automatic, 0 no, 1 yes
	qboolean hide;
	qboolean include;			// list it although it has a PlayerChoice.txt (it stays in Custom too)
	float scale;				// 0 = automatic
} sjeOverride_t;

typedef struct sjeVarLabel_s
{
	char folder[48];
	char modelString[MAX_QPATH];
	char label[32];
} sjeVarLabel_t;

typedef struct sjeSaberMap_s
{
	char from[48];
	char to[48];
} sjeSaberMap_t;

typedef struct sjeVFolder_s
{
	char origin[MAX_QPATH];
	char folder[48];
} sjeVFolder_t;

static sjeChar_t sjeChars[SJE_MAX_CHARS];
static int sjeNumChars;
static qboolean sjeBuilt;

static sjeOverride_t sjeOverrides[SJE_MAX_OVERRIDES];
static int sjeNumOverrides;
static sjeVarLabel_t sjeVarLabels[SJE_MAX_VARLABELS];
static int sjeNumVarLabels;
static sjeSaberMap_t sjeSaberMaps[SJE_MAX_SABERMAP];
static int sjeNumSaberMaps;
static sjeVFolder_t sjeVFolders[SJE_MAX_VFOLDERS];
static int sjeNumVFolders;
static sjeVFolder_t sjeAliases[SJE_MAX_VFOLDERS];	// "alias <model string> <variant folder>" (ext_data/sje_charmenu.dat)
static int sjeNumAliases;
static int sjeNumDirsScanned;

// menu state
static int sjeFaction = SJEF_EMPIRE;
static int sjeList[SJE_MAX_CHARS];	// sjeChars indexes of the current faction
static int sjeListCount;
static int sjePage;
static int sjeSel = -1;				// sjeChars index
static int sjeVar;
static char sjeDescText[2048];
static char sjeVarText[64];

static char sjeFileBuf[SJE_NPC_FILE_SIZE];

// ---------------------------------------------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------------------------------------------
static qboolean SJE_FileExists(const char* path)
{
	fileHandle_t f = 0;
	const int len = trap->FS_Open(path, &f, FS_READ);
	if (!f)
	{
		return qfalse;
	}
	trap->FS_Close(f);
	return len > 0 ? qtrue : qfalse;
}

static int SJE_ReadFile(const char* path, char* buf, const int bufSize)
{
	fileHandle_t f = 0;
	const int len = trap->FS_Open(path, &f, FS_READ);
	if (!f)
	{
		return -1;
	}
	if (len <= 0 || len >= bufSize)
	{
		if (len >= bufSize)
		{
			Com_Printf(S_COLOR_YELLOW "SJE characters: %s is too big (%d bytes), skipped\n", path, len);
		}
		trap->FS_Close(f);
		return -1;
	}
	trap->FS_Read(buf, len, f);
	buf[len] = 0;
	trap->FS_Close(f);
	return len;
}

static void SJE_Lower(char* s)
{
	for (; *s; s++)
	{
		if (*s >= 'A' && *s <= 'Z')
		{
			*s = (char)(*s + ('a' - 'A'));
		}
	}
}

static int SJE_FactionForKey(const char* key)
{
	for (int i = 0; i < SJEF_COUNT; i++)
	{
		if (!Q_stricmp(key, sjeFactionKeys[i]))
		{
			return i;
		}
	}
	if (!Q_stricmp(key, "rebellion"))
	{
		return SJEF_REBELS;
	}
	if (!Q_stricmp(key, "bounty") || !Q_stricmp(key, "bountyhunters"))
	{
		return SJEF_BH;
	}
	return -1;
}

// "RebornFencer2" -> "Reborn Fencer", "stormtrooper_officer" -> "Stormtrooper Officer"
static void SJE_CleanName(const char* in, char* out, const int outSize)
{
	int o = 0;
	const int len = (int)strlen(in);
	int end = len;

	while (end > 1 && in[end - 1] >= '0' && in[end - 1] <= '9')
	{
		end--;
	}
	const char* firstDigit = strpbrk(in, "0123456789");
	if (end < len && end > 0 && (end < 3 || in[end - 1] >= 'A' && in[end - 1] <= 'Z' || firstDigit && firstDigit < in + end))
	{
		end = len; // JK2, R2D2: the digits belong to the name
	}
	for (int i = 0; i < end && o < outSize - 2; i++)
	{
		char c = in[i];
		if (c == '_' || c == '-')
		{
			c = ' ';
		}
		if (i > 0 && c >= 'A' && c <= 'Z' && in[i - 1] >= 'a' && in[i - 1] <= 'z' && o < outSize - 2)
		{
			out[o++] = ' ';
		}
		if ((o == 0 || out[o - 1] == ' ') && c >= 'a' && c <= 'z')
		{
			c = (char)(c - ('a' - 'A'));
		}
		out[o++] = c;
	}
	out[o] = 0;
}

static const sjeOverride_t* SJE_OverrideFor(const char* folder)
{
	for (int i = 0; i < sjeNumOverrides; i++)
	{
		if (!Q_stricmp(sjeOverrides[i].folder, folder))
		{
			return &sjeOverrides[i];
		}
	}
	return NULL;
}

static const char* SJE_VariantFolderFor(const char* modelString)
{
	static char found[64];
	char candidate[64];

	for (int i = 0; i < sjeNumVFolders; i++)
	{
		if (!Q_stricmp(sjeVFolders[i].origin, modelString))
		{
			return sjeVFolders[i].folder;
		}
	}
	// the folder must say it was made from this model string (charvariant.txt), as the server checks
	for (int i = 0; i < sjeNumAliases; i++)
	{
		if (!Q_stricmp(sjeAliases[i].origin, modelString) && !Q_stricmp(BG_CharVariantOrigin(sjeAliases[i].folder), modelString))
		{
			return sjeAliases[i].folder;
		}
	}
	// generated folders: <folder>_<skin> (or <folder>_<skin>_v when that name was taken)
	Q_strncpyz(candidate, modelString, sizeof candidate);
	{
		char* slash = strchr(candidate, '/');
		if (!slash)
		{
			return NULL;
		}
		*slash = '_';
	}
	for (int pass = 0; pass < 2; pass++)
	{
		if (pass == 1)
		{
			Q_strcat(candidate, sizeof candidate, "_v");
		}
		if (!Q_stricmp(BG_CharVariantOrigin(candidate), modelString))
		{
			Q_strncpyz(found, candidate, sizeof found);
			return found;
		}
	}
	return NULL;
}

static qboolean SJE_HasA1Skins(const char* folder)
{
	return SJE_FileExists(va("models/players/%s/head_a1.skin", folder)) &&
		SJE_FileExists(va("models/players/%s/torso_a1.skin", folder)) &&
		SJE_FileExists(va("models/players/%s/lower_a1.skin", folder)) ? qtrue : qfalse;
}

// ---------------------------------------------------------------------------------------------------------------
// ext_data/sje_charmenu.dat
//   character <folder> { faction <key> name "<text>" species "<text>" weapon "<text>" desc "<@MENUS_KEY or text>"
//                        order <n> group <0|1> hide <0|1> }
//   variant <folder> <modelString> "<label>"     label of a variant; adds the model as a variant if the folder lacks it
//   saber <npc saber> <mp saber>                  saber to use in MP for an NPC saber that is not allowed in MP
// ---------------------------------------------------------------------------------------------------------------
static void SJE_LoadOverrides(void)
{
	const char* p;

	sjeNumOverrides = 0;
	sjeNumVarLabels = 0;
	sjeNumSaberMaps = 0;
	sjeNumAliases = 0;

	if (SJE_ReadFile("ext_data/sje_charmenu.dat", sjeFileBuf, sizeof sjeFileBuf) <= 0)
	{
		Com_Printf(S_COLOR_YELLOW "SJE characters: ext_data/sje_charmenu.dat not found, automatic factions and names only\n");
		return;
	}

	p = sjeFileBuf;
	COM_BeginParseSession("sje_charmenu");
	while (1)
	{
		const char* token = COM_ParseExt(&p, qtrue);
		if (!token[0])
		{
			break;
		}
		if (!Q_stricmp(token, "character"))
		{
			token = COM_ParseExt(&p, qtrue);
			if (!token[0] || sjeNumOverrides >= SJE_MAX_OVERRIDES)
			{
				break;
			}
			sjeOverride_t* o = &sjeOverrides[sjeNumOverrides++];
			memset(o, 0, sizeof * o);
			o->faction = -1;
			o->group = -1;
			o->order = 1000;
			Q_strncpyz(o->folder, token, sizeof o->folder);
			token = COM_ParseExt(&p, qtrue);
			if (Q_stricmp(token, "{"))
			{
				Com_Printf(S_COLOR_YELLOW "SJE characters: missing { after character %s\n", o->folder);
				break;
			}
			while (1)
			{
				char key[32];
				token = COM_ParseExt(&p, qtrue);
				if (!token[0] || !Q_stricmp(token, "}"))
				{
					break;
				}
				Q_strncpyz(key, token, sizeof key);
				token = COM_ParseExt(&p, qfalse);
				if (!Q_stricmp(key, "faction"))
				{
					o->faction = SJE_FactionForKey(token);
				}
				else if (!Q_stricmp(key, "name"))
				{
					Q_strncpyz(o->name, token, sizeof o->name);
				}
				else if (!Q_stricmp(key, "species"))
				{
					Q_strncpyz(o->species, token, sizeof o->species);
				}
				else if (!Q_stricmp(key, "weapon"))
				{
					Q_strncpyz(o->weapon, token, sizeof o->weapon);
				}
				else if (!Q_stricmp(key, "desc"))
				{
					Q_strncpyz(o->desc, token, sizeof o->desc);
				}
				else if (!Q_stricmp(key, "order"))
				{
					o->order = atoi(token);
				}
				else if (!Q_stricmp(key, "group"))
				{
					o->group = atoi(token) ? 1 : 0;
				}
				else if (!Q_stricmp(key, "hide"))
				{
					o->hide = atoi(token) ? qtrue : qfalse;
				}
				else if (!Q_stricmp(key, "scale"))
				{
					o->scale = (float)atof(token);
				}
				else if (!Q_stricmp(key, "include"))
				{
					o->include = atoi(token) ? qtrue : qfalse;
				}
			}
		}
		else if (!Q_stricmp(token, "variant"))
		{
			char folder[48], model[MAX_QPATH];
			Q_strncpyz(folder, COM_ParseExt(&p, qfalse), sizeof folder);
			Q_strncpyz(model, COM_ParseExt(&p, qfalse), sizeof model);
			token = COM_ParseExt(&p, qfalse);
			if (folder[0] && model[0] && sjeNumVarLabels < SJE_MAX_VARLABELS)
			{
				sjeVarLabel_t* v = &sjeVarLabels[sjeNumVarLabels++];
				Q_strncpyz(v->folder, folder, sizeof v->folder);
				Q_strncpyz(v->modelString, model, sizeof v->modelString);
				Q_strncpyz(v->label, token, sizeof v->label);
			}
		}
		else if (!Q_stricmp(token, "alias"))
		{
			char model[MAX_QPATH];
			Q_strncpyz(model, COM_ParseExt(&p, qfalse), sizeof model);
			token = COM_ParseExt(&p, qfalse);
			if (model[0] && token[0] && sjeNumAliases < SJE_MAX_VFOLDERS)
			{
				Q_strncpyz(sjeAliases[sjeNumAliases].origin, model, sizeof sjeAliases[0].origin);
				Q_strncpyz(sjeAliases[sjeNumAliases].folder, token, sizeof sjeAliases[0].folder);
				sjeNumAliases++;
			}
		}
		else if (!Q_stricmp(token, "saber"))
		{
			char from[48];
			Q_strncpyz(from, COM_ParseExt(&p, qfalse), sizeof from);
			token = COM_ParseExt(&p, qfalse);
			if (from[0] && token[0] && sjeNumSaberMaps < SJE_MAX_SABERMAP)
			{
				Q_strncpyz(sjeSaberMaps[sjeNumSaberMaps].from, from, sizeof sjeSaberMaps[0].from);
				Q_strncpyz(sjeSaberMaps[sjeNumSaberMaps].to, token, sizeof sjeSaberMaps[0].to);
				sjeNumSaberMaps++;
			}
		}
		else
		{
			SkipRestOfLine(&p);
		}
	}
}

// character-menu variant folders: models/players/<folder>/charvariant.txt (bg_classmodel.c reads the same files)
static void SJE_ScanVariantFolders(void)
{
	static char dirs[32768];
	const int numDirs = trap->FS_GetFileList("models/players", "/", dirs, sizeof dirs);
	const char* d = dirs;

	sjeNumVFolders = 0;
	sjeNumDirsScanned = numDirs;
	for (int i = 0; i < numDirs && *d; i++)
	{
		const size_t len = strlen(d);
		char folder[64];
		Q_strncpyz(folder, d, sizeof folder);
		d += len + 1;
		if (len && folder[strlen(folder) - 1] == '/')
		{
			folder[strlen(folder) - 1] = 0;
		}
		if (!folder[0] || folder[0] == '.')
		{
			continue;
		}
		const char* origin = BG_CharVariantOrigin(folder);
		if (origin[0] && sjeNumVFolders < SJE_MAX_VFOLDERS)
		{
			Q_strncpyz(sjeVFolders[sjeNumVFolders].origin, origin, sizeof sjeVFolders[0].origin);
			Q_strncpyz(sjeVFolders[sjeNumVFolders].folder, folder, sizeof sjeVFolders[0].folder);
			sjeNumVFolders++;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------
// per model folder facts (cached)
// ---------------------------------------------------------------------------------------------------------------
typedef struct sjeFolderInfo_s
{
	char folder[48];
	qboolean hasGlm;
	qboolean playerChoice;
	qboolean humanoidGla;	// model.glm names players/_humanoid/ (the normal player skeleton)
	int skelFlags;
} sjeFolderInfo_t;

#define SJE_MAX_FOLDERINFO 256
static sjeFolderInfo_t sjeFolderInfo[SJE_MAX_FOLDERINFO];
static int sjeNumFolderInfo;

static const sjeFolderInfo_t* SJE_FolderInfo(const char* folder)
{
	static sjeFolderInfo_t scratch;

	for (int i = 0; i < sjeNumFolderInfo; i++)
	{
		if (!Q_stricmp(sjeFolderInfo[i].folder, folder))
		{
			return &sjeFolderInfo[i];
		}
	}
	sjeFolderInfo_t* fi = sjeNumFolderInfo < SJE_MAX_FOLDERINFO ? &sjeFolderInfo[sjeNumFolderInfo++] : &scratch;
	memset(fi, 0, sizeof * fi);
	Q_strncpyz(fi->folder, folder, sizeof fi->folder);
	fi->hasGlm = SJE_FileExists(va("models/players/%s/model.glm", folder));
	if (fi->hasGlm)
	{
		fi->playerChoice = SJE_FileExists(va("models/players/%s/PlayerChoice.txt", folder));
		fi->skelFlags = BG_ModelSkeletonFlags(folder);
		fi->humanoidGla = BG_ModelUsesHumanoidGLA(folder);
	}
	return fi;
}

// the ui's BG_ParseAnimationFileForGLA for bg_skeleton.c (game/cgame: bg_panimate.c): the animation.cfg next to the GLA
int BG_ParseAnimationFileForGLA(const char* glaName)
{
	char cfgName[MAX_QPATH];

	if (!glaName || !glaName[0])
	{
		return -1;
	}
	Q_strncpyz(cfgName, glaName, sizeof cfgName);
	char* slash = Q_strrchr(cfgName, '/');
	if (!slash)
	{
		return -1;
	}
	*slash = 0;
	if (strlen(cfgName) + strlen("/animation.cfg") >= sizeof cfgName)
	{
		return -1;
	}
	Q_strcat(cfgName, sizeof cfgName, "/animation.cfg");
	return UI_ParseAnimationFile(cfgName, NULL, qfalse);
}

// ---------------------------------------------------------------------------------------------------------------
// building the list
// ---------------------------------------------------------------------------------------------------------------
typedef struct sjeNpc_s
{
	char name[48];
	char model[48];
	char skin[MAX_QPATH];
	char saber[2][48];
	char color[2][16];
	char team[24];
	char npcClass[32];
	char fullName[64];
	char weapons[2][32];
	int numWeapons;
} sjeNpc_t;

#define SJE_MAX_NPCNAMES 1024
static char sjeSeenNpc[SJE_MAX_NPCNAMES][48];
static int sjeNumSeenNpc;

static qboolean SJE_NpcSeen(const char* name)
{
	for (int i = 0; i < sjeNumSeenNpc; i++)
	{
		if (!Q_stricmp(sjeSeenNpc[i], name))
		{
			return qtrue;
		}
	}
	if (sjeNumSeenNpc < SJE_MAX_NPCNAMES)
	{
		Q_strncpyz(sjeSeenNpc[sjeNumSeenNpc++], name, sizeof sjeSeenNpc[0]);
	}
	return qfalse;
}

static qboolean SJE_IsCreatureClass(const int botclass)
{
	return BG_ClassIsCreature(botclass);
}

static int SJE_AutoFaction(const sjeNpc_t* n, const int botclass, const int skelFlags)
{
	const qboolean saber = !Q_stricmp(n->weapons[0], "WP_SABER") || !Q_stricmp(n->weapons[1], "WP_SABER") || n->saber[0][0];
	const char* c = n->npcClass;

	(void)skelFlags;
	if (SJE_IsCreatureClass(botclass) ||
		Q_stristr(c, "DROID") || Q_stristr(c, "R2D2") || Q_stristr(c, "R5D2") || Q_stristr(c, "GONK") ||
		Q_stristr(c, "PROTOCOL") || Q_stristr(c, "MOUSE") || Q_stristr(c, "RANCOR") || Q_stristr(c, "WAMPA") ||
		Q_stristr(c, "HOWLER") || Q_stristr(c, "MINEMONSTER"))
	{
		return SJEF_OTHER;
	}
	if (Q_stristr(c, "BOBAFETT") || Q_stristr(c, "RODIAN") || Q_stristr(c, "TRANDOSHAN") || Q_stristr(c, "WEEQUAY") ||
		Q_stristr(c, "GRAN") || Q_stristr(c, "REELO") || Q_stristr(c, "MERC") || Q_stristr(n->model, "bounty"))
	{
		return SJEF_BH;
	}
	if (Q_stristr(c, "JAWA") || Q_stristr(c, "TUSKEN") || Q_stristr(c, "UGNAUGHT") || Q_stristr(c, "NOGHRI") ||
		Q_stristr(c, "CHISS") || Q_stristr(c, "GAMORREAN") || Q_stristr(c, "PRISONER"))
	{
		return SJEF_NATIVES;
	}
	if (saber)
	{
		return !Q_stricmp(n->team, "TEAM_ENEMY") ? SJEF_SITH : SJEF_JEDI;
	}
	if (!Q_stricmp(n->team, "TEAM_ENEMY"))
	{
		return SJEF_EMPIRE;
	}
	if (!Q_stricmp(n->team, "TEAM_PLAYER"))
	{
		return SJEF_REBELS;
	}
	return SJEF_OTHER;
}

static qboolean SJE_IsGroupClass(const char* c)
{
	static const char* const groups[] = {
		"CLASS_STORMTROOPER", "CLASS_IMPERIAL", "CLASS_IMPWORKER", "CLASS_REBEL", "CLASS_REBORN", "CLASS_JEDI",
		"CLASS_TRANDOSHAN", "CLASS_RODIAN", "CLASS_WEEQUAY", "CLASS_GRAN", "CLASS_SWAMPTROOPER", "CLASS_SHADOWTROOPER",
		"CLASS_COMMANDO", "CLASS_PRISONER", "CLASS_JAWA", "CLASS_TUSKEN", "CLASS_UGNAUGHT", "CLASS_NOGHRI", "CLASS_MERC",
		"CLASS_SABOTEUR", "CLASS_HAZARD_TROOPER", "CLASS_ROCKETTROOPER", "CLASS_BESPIN_COP", "CLASS_GAMORREAN",
		"CLASS_BATTLEDROID", "CLASS_SBD", "CLASS_CULTIST", "CLASS_IMPOFFICER"
	};
	for (int i = 0; i < (int)ARRAY_LEN(groups); i++)
	{
		if (!Q_stricmp(c, groups[i]))
		{
			return qtrue;
		}
	}
	return qfalse;
}

static void SJE_AddNpc(const sjeNpc_t* n)
{
	char model[48];
	char skin[MAX_QPATH];
	char modelString[MAX_QPATH];

	Q_strncpyz(model, n->model, sizeof model);
	SJE_Lower(model);
	if (!model[0] || strchr(model, '/') || strchr(model, '\\'))
	{
		return;
	}
	const sjeFolderInfo_t* fi = SJE_FolderInfo(model);
	const sjeOverride_t* o = SJE_OverrideFor(model);
	if (!fi->hasGlm || fi->playerChoice && !(o && o->include))
	{
		return;
	}

	skin[0] = 0;
	if (n->skin[0] && Q_stricmp(n->skin, "default"))
	{
		if (strchr(n->skin, '|'))
		{
			Q_strncpyz(skin, n->skin, sizeof skin);
		}
		else if (SJE_FileExists(va("models/players/%s/model_%s.skin", model, n->skin)))
		{
			Q_strncpyz(skin, n->skin, sizeof skin);
		}
	}
	SJE_Lower(skin);
	if (skin[0])
	{
		Com_sprintf(modelString, sizeof modelString, "%s/%s", model, skin);
	}
	else
	{
		Q_strncpyz(modelString, model, sizeof modelString);
	}

	if (BG_ModelBlocked(modelString))
	{
		return;
	}
	const int botclass = BG_ClassModelBotclass(modelString);
	if (!fi->humanoidGla && (!fi->skelFlags || !BG_ClassSkeletonOk(botclass, fi->skelFlags)))
	{
		return; // the server (BG_ClassSkeletonOk) or the client (CG_RegisterClientModelname: no usable skeleton) would
		// put the default model on it
	}

	sjeChar_t* ch = NULL;
	for (int i = 0; i < sjeNumChars; i++)
	{
		if (!Q_stricmp(sjeChars[i].folder, model))
		{
			ch = &sjeChars[i];
			break;
		}
	}
	if (!ch)
	{
		if (sjeNumChars >= SJE_MAX_CHARS)
		{
			return;
		}
		ch = &sjeChars[sjeNumChars++];
		memset(ch, 0, sizeof * ch);
		Q_strncpyz(ch->folder, model, sizeof ch->folder);
		Q_strncpyz(ch->npcClass, n->npcClass, sizeof ch->npcClass);
		Q_strncpyz(ch->team, n->team, sizeof ch->team);
		Q_strncpyz(ch->weapons[0], n->weapons[0], sizeof ch->weapons[0]);
		Q_strncpyz(ch->weapons[1], n->weapons[1], sizeof ch->weapons[1]);
		ch->faction = SJE_AutoFaction(n, botclass, fi->skelFlags);
		ch->group = SJE_IsGroupClass(n->npcClass);
		ch->order = 1000;
		ch->scale = 1.0f;
	}
	if (!ch->fullName[0] && n->fullName[0] && n->fullName[0] != '@')
	{
		Q_strncpyz(ch->fullName, n->fullName, sizeof ch->fullName);
	}

	for (int i = 0; i < ch->numVariants; i++)
	{
		const sjeVariant_t* v = &ch->variants[i];
		if (!Q_stricmp(v->modelString, modelString))
		{
			return; // one variant per look: the first NPC using it gives the sabers
		}
	}
	if (ch->numVariants >= SJE_MAX_VARIANTS)
	{
		return;
	}
	sjeVariant_t* v = &ch->variants[ch->numVariants++];
	memset(v, 0, sizeof * v);
	Q_strncpyz(v->npc, n->name, sizeof v->npc);
	Q_strncpyz(v->modelString, modelString, sizeof v->modelString);
	Q_strncpyz(v->skin, skin, sizeof v->skin);
	for (int s = 0; s < 2; s++)
	{
		Q_strncpyz(v->saber[s], n->saber[s], sizeof v->saber[s]);
		Q_strncpyz(v->color[s], n->color[s], sizeof v->color[s]);
	}
	v->botclass = botclass;
}

static void SJE_ParseNpcFile(const char* path)
{
	const char* p;

	if (SJE_ReadFile(path, sjeFileBuf, sizeof sjeFileBuf) <= 0)
	{
		return;
	}
	p = sjeFileBuf;
	COM_BeginParseSession(path);
	while (1)
	{
		sjeNpc_t n;
		const char* token = COM_ParseExt(&p, qtrue);
		if (!token[0])
		{
			return;
		}
		if (!Q_stricmp(token, "{") || !Q_stricmp(token, "}"))
		{
			continue;
		}
		memset(&n, 0, sizeof n);
		Q_strncpyz(n.name, token, sizeof n.name);
		token = COM_ParseExt(&p, qtrue);
		if (Q_stricmp(token, "{"))
		{
			continue; // not an NPC block (stray token): try again with this token as a name
		}
		int depth = 1;
		while (depth > 0)
		{
			char key[32];
			token = COM_ParseExt(&p, qtrue);
			if (!token[0])
			{
				return;
			}
			if (!Q_stricmp(token, "{"))
			{
				depth++;
				continue;
			}
			if (!Q_stricmp(token, "}"))
			{
				depth--;
				continue;
			}
			if (depth > 1)
			{
				continue;
			}
			Q_strncpyz(key, token, sizeof key);
			token = COM_ParseExt(&p, qfalse);
			if (!token[0])
			{
				continue;
			}
			if (!Q_stricmp(token, "{"))
			{
				depth++;
				continue;
			}
			if (!Q_stricmp(token, "}"))
			{
				depth--;
				continue;
			}
			if (!Q_stricmp(key, "playerModel"))
			{
				Q_strncpyz(n.model, token, sizeof n.model);
			}
			else if (!Q_stricmp(key, "customSkin"))
			{
				Q_strncpyz(n.skin, token, sizeof n.skin);
			}
			else if (!Q_stricmp(key, "saber") && !n.saber[0][0])
			{
				Q_strncpyz(n.saber[0], token, sizeof n.saber[0]);
			}
			else if (!Q_stricmp(key, "saber2") && !n.saber[1][0])
			{
				Q_strncpyz(n.saber[1], token, sizeof n.saber[1]);
			}
			else if (!Q_stricmp(key, "saberColor") && !n.color[0][0])
			{
				Q_strncpyz(n.color[0], token, sizeof n.color[0]);
			}
			else if (!Q_stricmp(key, "saber2Color") && !n.color[1][0])
			{
				Q_strncpyz(n.color[1], token, sizeof n.color[1]);
			}
			else if (!Q_stricmp(key, "playerTeam") && !n.team[0])
			{
				Q_strncpyz(n.team, token, sizeof n.team);
			}
			else if (!Q_stricmp(key, "class") && !n.npcClass[0])
			{
				Q_strncpyz(n.npcClass, token, sizeof n.npcClass);
			}
			else if (!Q_stricmp(key, "fullName") && !n.fullName[0])
			{
				Q_strncpyz(n.fullName, token, sizeof n.fullName);
			}
			else if (!Q_stricmp(key, "weapon") && n.numWeapons < 2 && Q_stricmp(token, "WP_NONE") && Q_stricmp(token, "WP_MELEE"))
			{
				Q_strncpyz(n.weapons[n.numWeapons++], token, sizeof n.weapons[0]);
			}
		}
		if (!SJE_NpcSeen(n.name))
		{
			SJE_AddNpc(&n);
		}
	}
}

static void SJE_ParseNpcDir(const char* dir)
{
	static char list[32768];
	const int num = trap->FS_GetFileList(dir, ".npc", list, sizeof list);
	const char* f = list;

	for (int i = 0; i < num && *f; i++)
	{
		const size_t len = strlen(f);
		SJE_ParseNpcFile(va("%s/%s", dir, f));
		f += len + 1;
	}
}

static const char* SJE_WeaponText(const char* wp)
{
	static const struct
	{
		const char* wp;
		const char* text;
	} names[] = {
		{ "WP_SABER", "Lightsaber" },{ "WP_BRYAR_PISTOL", "Bryar Pistol" },{ "WP_BLASTER", "Blaster Rifle" },
		{ "WP_DISRUPTOR", "Disruptor Rifle" },{ "WP_BOWCASTER", "Bowcaster" },{ "WP_REPEATER", "Repeater" },
		{ "WP_DEMP2", "DEMP 2" },{ "WP_FLECHETTE", "Flechette" },{ "WP_ROCKET_LAUNCHER", "Rocket Launcher" },
		{ "WP_THERMAL", "Thermal Detonators" },{ "WP_TRIP_MINE", "Trip Mines" },{ "WP_DET_PACK", "Detonation Packs" },
		{ "WP_CONCUSSION", "Concussion Rifle" },{ "WP_BRYAR_OLD", "Bryar Pistol" },{ "WP_STUN_BATON", "Stun Baton" },
		{ "WP_BLASTER_PISTOL", "Blaster Pistol" },{ "WP_EMPLACED_GUN", "Emplaced Gun" },{ "WP_TUSKEN_RIFLE", "Tusken Rifle" },
		{ "WP_TUSKEN_STAFF", "Gaffi Stick" },{ "WP_NOGHRI_STICK", "Noghri Stick" },{ "WP_SCEPTER", "Scepter" },
	};
	for (int i = 0; i < (int)ARRAY_LEN(names); i++)
	{
		if (!Q_stricmp(wp, names[i].wp))
		{
			return names[i].text;
		}
	}
	return NULL;
}

static const char* SJE_SpeciesFor(const sjeChar_t* ch)
{
	static const struct
	{
		const char* key;
		const char* species;
	} table[] = {
		{ "rodian", "Rodian" },{ "trandoshan", "Trandoshan" },{ "weequay", "Weequay" },{ "gran", "Gran" },
		{ "chiss", "Chiss" },{ "noghri", "Noghri" },{ "ugnaught", "Ugnaught" },{ "jawa", "Jawa" },
		{ "tusken", "Tusken Raider" },{ "wookiee", "Wookiee" },{ "chewbacca", "Wookiee" },{ "gamorrean", "Gamorrean" },
		{ "reelo", "Nikto" },{ "monmothma", "Human" },{ "yoda", "Unknown" },{ "r2d2", "Astromech Droid" },
		{ "r5d2", "Astromech Droid" },{ "protocol", "Protocol Droid" },{ "gonk", "Power Droid" },
		{ "assassin_droid", "Assassin Droid" },{ "saber_droid", "Droid" },{ "sbd", "Super Battle Droid" },
		{ "battledroid", "Battle Droid" },{ "rancor", "Rancor" },{ "wampa", "Wampa" },{ "howler", "Howler" },
		{ "minemonster", "Mine Monster" },{ "omicron_iridonian", "Zabrak" },{ "alora", "Twi'lek" },
	};
	for (int i = 0; i < (int)ARRAY_LEN(table); i++)
	{
		if (Q_stristr(ch->folder, table[i].key))
		{
			return table[i].species;
		}
	}
	return ch->group ? "Various" : "Human";
}

static void SJE_VariantLabel(const sjeChar_t* ch, const sjeVariant_t* v, char* out, const int outSize)
{
	for (int i = 0; i < sjeNumVarLabels; i++)
	{
		if (!Q_stricmp(sjeVarLabels[i].folder, ch->folder) && !Q_stricmp(sjeVarLabels[i].modelString, v->modelString))
		{
			Q_strncpyz(out, sjeVarLabels[i].label, outSize);
			return;
		}
	}
	if (!v->skin[0])
	{
		Q_strncpyz(out, "Default", outSize);
	}
	else if (strchr(v->skin, '|'))
	{
		SJE_CleanName(v->npc, out, outSize);
	}
	else
	{
		SJE_CleanName(v->skin, out, outSize);
	}
}

static void SJE_ResolveVariant(sjeVariant_t* v, const char* folder)
{
	if (strchr(v->skin, '|'))
	{// the NPC already uses a multi-part skin
		Q_strncpyz(v->useModel, v->modelString, sizeof v->useModel);
		Q_strncpyz(v->g2Folder, folder, sizeof v->g2Folder);
		Com_sprintf(v->g2Skin, sizeof v->g2Skin, "models/players/%s/|%s", folder, v->skin);
		return;
	}

	const char* vf = v->skin[0] ? SJE_VariantFolderFor(v->modelString) : folder;
	if (vf && SJE_HasA1Skins(vf))
	{// DH's pattern
		Com_sprintf(v->useModel, sizeof v->useModel, "%s/head_a1|torso_a1|lower_a1", vf);
		Q_strncpyz(v->g2Folder, vf, sizeof v->g2Folder);
		Com_sprintf(v->g2Skin, sizeof v->g2Skin, "models/players/%s/|head_a1|torso_a1|lower_a1", vf);
		return;
	}
	Q_strncpyz(v->useModel, v->modelString, sizeof v->useModel);
	Q_strncpyz(v->g2Folder, folder, sizeof v->g2Folder);
	Com_sprintf(v->g2Skin, sizeof v->g2Skin, "models/players/%s/model_%s.skin", folder, v->skin[0] ? v->skin : "default");
}

static int QDECL SJE_CompareChars(const void* a, const void* b)
{
	const sjeChar_t* ca = (const sjeChar_t*)a;
	const sjeChar_t* cb = (const sjeChar_t*)b;
	if (ca->faction != cb->faction)
	{
		return ca->faction - cb->faction;
	}
	if (ca->order != cb->order)
	{
		return ca->order - cb->order;
	}
	return Q_stricmp(ca->name, cb->name);
}

static void SJE_FinishChars(void)
{
	// extra variants named in the override file (a model of another folder shown under this portrait)
	for (int i = 0; i < sjeNumVarLabels; i++)
	{
		const sjeVarLabel_t* vl = &sjeVarLabels[i];
		for (int c = 0; c < sjeNumChars; c++)
		{
			sjeChar_t* ch = &sjeChars[c];
			if (Q_stricmp(ch->folder, vl->folder))
			{
				continue;
			}
			qboolean have = qfalse;
			for (int v = 0; v < ch->numVariants; v++)
			{
				if (!Q_stricmp(ch->variants[v].modelString, vl->modelString))
				{
					have = qtrue;
				}
			}
			if (!have && ch->numVariants < SJE_MAX_VARIANTS)
			{
				char model[48];
				Q_strncpyz(model, vl->modelString, sizeof model);
				char* slash = strchr(model, '/');
				if (slash)
				{
					*slash = 0;
				}
				const sjeFolderInfo_t* fi = SJE_FolderInfo(model);
				const int botclass = BG_ClassModelBotclass(vl->modelString);
				if (fi->hasGlm && !BG_ModelBlocked(vl->modelString) &&
					(fi->humanoidGla || fi->skelFlags && BG_ClassSkeletonOk(botclass, fi->skelFlags)))
				{
					sjeVariant_t* nv = &ch->variants[ch->numVariants++];
					*nv = ch->variants[0]; // same sabers as the main look
					Q_strncpyz(nv->modelString, vl->modelString, sizeof nv->modelString);
					Q_strncpyz(nv->skin, slash ? slash + 1 : "", sizeof nv->skin);
					nv->botclass = botclass;
					// a model of another folder: preview and apply it as that folder
					Q_strncpyz(nv->npc, model, sizeof nv->npc);
				}
			}
		}
	}

	for (int c = 0; c < sjeNumChars; c++)
	{
		sjeChar_t* ch = &sjeChars[c];
		const sjeOverride_t* o = SJE_OverrideFor(ch->folder);

		// name: the NPC's fullName, else an NPC named like the folder, else the folder
		if (ch->fullName[0])
		{
			Q_strncpyz(ch->name, ch->fullName, sizeof ch->name);
		}
		else
		{
			char folderName[64];
			SJE_CleanName(ch->folder, folderName, sizeof folderName);
			Q_strncpyz(ch->name, folderName, sizeof ch->name);
			for (int v = 0; v < ch->numVariants; v++)
			{
				char npcName[64];
				char a[64], b[64];
				int na = 0, nb = 0;
				SJE_CleanName(ch->variants[v].npc, npcName, sizeof npcName);
				// same letters and digits as the folder (KyleJK2 = kylejk2)
				for (const char* s = ch->variants[v].npc; *s && na < (int)sizeof a - 1; s++)
				{
					if (*s >= 'A' && *s <= 'Z' || *s >= 'a' && *s <= 'z' || *s >= '0' && *s <= '9')
					{
						a[na++] = *s;
					}
				}
				for (const char* s = ch->folder; *s && nb < (int)sizeof b - 1; s++)
				{
					if (*s >= 'A' && *s <= 'Z' || *s >= 'a' && *s <= 'z' || *s >= '0' && *s <= '9')
					{
						b[nb++] = *s;
					}
				}
				a[na] = 0;
				b[nb] = 0;
				if (!Q_stricmp(npcName, folderName) || !Q_stricmp(a, b))
				{
					Q_strncpyz(ch->name, npcName, sizeof ch->name);
					break;
				}
			}
		}
		if (Q_stristr(ch->folder, "rancor"))
		{
			ch->scale = 0.25f;
		}
		else if (Q_stristr(ch->folder, "wampa"))
		{
			ch->scale = 0.6f;
		}

		if (o)
		{
			if (o->faction >= 0)
			{
				ch->faction = o->faction;
			}
			if (o->name[0])
			{
				Q_strncpyz(ch->name, o->name, sizeof ch->name);
			}
			if (o->species[0])
			{
				Q_strncpyz(ch->species, o->species, sizeof ch->species);
			}
			if (o->weapon[0])
			{
				Q_strncpyz(ch->weapon, o->weapon, sizeof ch->weapon);
			}
			if (o->desc[0])
			{
				ch->desc = o->desc;
			}
			if (o->group >= 0)
			{
				ch->group = o->group ? qtrue : qfalse;
			}
			ch->order = o->order;
			ch->hidden = o->hide;
			if (o->scale > 0.0f)
			{
				ch->scale = o->scale;
			}
		}
		if (!ch->species[0])
		{
			Q_strncpyz(ch->species, SJE_SpeciesFor(ch), sizeof ch->species);
		}
		if (!ch->weapon[0])
		{
			const char* w0 = SJE_WeaponText(ch->weapons[0]);
			const char* w1 = SJE_WeaponText(ch->weapons[1]);
			if (w0 && w1 && Q_stricmp(w0, w1))
			{
				Com_sprintf(ch->weapon, sizeof ch->weapon, "%s, or %s", w0, w1);
			}
			else
			{
				Q_strncpyz(ch->weapon, w0 ? w0 : w1 ? w1 : "None", sizeof ch->weapon);
			}
		}

		// the default look first: a variant the table labels "Default" (DH: Desann's default is the JK2 model), else the
		// folder itself, else any model without a skin
		int tableDefault = -1;
		for (int v = 0; v < ch->numVariants && tableDefault < 0; v++)
		{
			for (int i = 0; i < sjeNumVarLabels; i++)
			{
				if (!Q_stricmp(sjeVarLabels[i].folder, ch->folder) && !Q_stricmp(sjeVarLabels[i].label, "Default")
					&& !Q_stricmp(sjeVarLabels[i].modelString, ch->variants[v].modelString))
				{
					tableDefault = v;
					break;
				}
			}
		}
		if (tableDefault > 0)
		{
			const sjeVariant_t tmp = ch->variants[0];
			ch->variants[0] = ch->variants[tableDefault];
			ch->variants[tableDefault] = tmp;
		}
		for (int pass = 0; pass < 2 && tableDefault < 0; pass++)
		{
			int best = -1;
			for (int v = 0; v < ch->numVariants && best < 0; v++)
			{
				if (!ch->variants[v].skin[0] && (pass || !Q_stricmp(ch->variants[v].modelString, ch->folder)))
				{
					best = v;
				}
			}
			if (best > 0)
			{
				const sjeVariant_t tmp = ch->variants[0];
				ch->variants[0] = ch->variants[best];
				ch->variants[best] = tmp;
			}
			if (best >= 0)
			{
				break;
			}
		}

		for (int v = 0; v < ch->numVariants; v++)
		{
			sjeVariant_t* var = &ch->variants[v];
			char folder[48];
			Q_strncpyz(folder, var->modelString, sizeof folder);
			char* slash = strchr(folder, '/');
			if (slash)
			{
				*slash = 0;
			}
			SJE_ResolveVariant(var, folder);
			SJE_VariantLabel(ch, var, var->label, sizeof var->label);
			// unique labels (loadout variants of one skin): use the NPC name
			for (int w = 0; w < v; w++)
			{
				if (!Q_stricmp(ch->variants[w].label, var->label))
				{
					SJE_CleanName(var->npc, var->label, sizeof var->label);
					if (!Q_stricmp(ch->variants[w].label, var->label))
					{
						Q_strcat(var->label, sizeof var->label, va(" %d", v + 1));
					}
					break;
				}
			}
		}
	}

	// one entry per character (DH): a JK2 model (folder <name>jk2 or <name>jk) is left out when the JKA model is in the
	// list too (folder <name>, or <name>_new whose portrait carries the old one as a variant: tavion)
	for (int c = 0; c < sjeNumChars; c++)
	{
		sjeChar_t* ch = &sjeChars[c];
		char base[48];
		Q_strncpyz(base, ch->folder, sizeof base);
		const int len = (int)strlen(base);
		if (len > 3 && !Q_stricmp(base + len - 3, "jk2"))
		{
			base[len - 3] = 0;
		}
		else if (len > 2 && !Q_stricmp(base + len - 2, "jk"))
		{
			base[len - 2] = 0;
		}
		else
		{
			continue;
		}
		for (int d = 0; d < sjeNumChars; d++)
		{
			if (d != c && !sjeChars[d].hidden
				&& (!Q_stricmp(sjeChars[d].folder, base) || !Q_stricmp(sjeChars[d].folder, va("%s_new", base))))
			{
				ch->hidden = qtrue;
				break;
			}
		}
	}

	qsort(sjeChars, sjeNumChars, sizeof sjeChars[0], SJE_CompareChars);
}

void UI_SJEChars_Build(void)
{
	if (sjeBuilt)
	{
		return;
	}
	sjeBuilt = qtrue;

	const int t0 = trap->Milliseconds();
	sjeNumChars = 0;
	sjeNumSeenNpc = 0;
	sjeNumFolderInfo = 0;

	SJE_LoadOverrides();
	SJE_ScanVariantFolders();
	SJE_ParseNpcDir("ext_data/npcs");
	SJE_ParseNpcDir("ext_data/mpnpcs");
	SJE_FinishChars();

	int numVariants = 0;
	for (int i = 0; i < sjeNumChars; i++)
	{
		numVariants += sjeChars[i].numVariants;
	}
	int numVFolders = 0;
	for (int i = 0; i < sjeNumChars; i++)
	{
		for (int v = 0; v < sjeChars[i].numVariants; v++)
		{
			if (sjeChars[i].variants[v].skin[0] && Q_stricmp(sjeChars[i].variants[v].useModel, sjeChars[i].variants[v].modelString))
			{
				numVFolders++;
			}
		}
	}
	Com_Printf("SJE characters: %d portraits, %d variants (%d in a variant folder; %d folders listed, %d marked), %d overrides (%d ms)\n",
		sjeNumChars, numVariants, numVFolders, sjeNumDirsScanned, sjeNumVFolders, sjeNumOverrides, trap->Milliseconds() - t0);
}

// ---------------------------------------------------------------------------------------------------------------
// menu
// ---------------------------------------------------------------------------------------------------------------
static menuDef_t* SJE_Menu(void)
{
	return Menus_FindByName(SJE_CHARMENU);
}

static qhandle_t SJE_Icon(sjeChar_t* ch)
{
	if (!ch->iconTried)
	{
		const sjeVariant_t* v = &ch->variants[0];
		const char* skin = v->skin[0] && !strchr(v->skin, '|') ? v->skin : "default";
		ch->iconTried = qtrue;
		ch->icon = trap->R_RegisterShaderNoMip(va("models/players/%s/icon_%s", ch->folder, skin));
		if (!ch->icon)
		{
			ch->icon = trap->R_RegisterShaderNoMip(va("models/players/%s/icon_default", ch->folder));
		}
		if (!ch->icon)
		{
			ch->icon = trap->R_RegisterShaderNoMip(SJE_PLACEHOLDER_ICON);
		}
	}
	return ch->icon;
}

static void SJE_BuildFactionList(void)
{
	sjeListCount = 0;
	for (int i = 0; i < sjeNumChars; i++)
	{
		if (sjeChars[i].faction == sjeFaction && !sjeChars[i].hidden)
		{
			sjeList[sjeListCount++] = i;
		}
	}
}

static int SJE_NumPages(void)
{
	return sjeListCount <= 0 ? 1 : (sjeListCount + SJE_MAX_SLOTS - 1) / SJE_MAX_SLOTS;
}

static void SJE_BuildDesc(const sjeChar_t* ch, char* out, const int outSize)
{
	char buf[1024];

	out[0] = 0;
	if (ch->desc)
	{
		if (ch->desc[0] == '@')
		{// DH's strings come twice: KEY (sometimes only the plain name) and KEY1 ("Name: ...")
			char buf1[1024];
			buf[0] = 0;
			buf1[0] = 0;
			trap->SE_GetStringTextString(ch->desc + 1, buf, sizeof buf);
			trap->SE_GetStringTextString(va("%s1", ch->desc + 1), buf1, sizeof buf1);
			if (Q_stricmpn(buf, "Name:", 5) && !Q_stricmpn(buf1, "Name:", 5))
			{
				Q_strncpyz(out, buf1, outSize);
				return;
			}
			if (buf[0])
			{
				Q_strncpyz(out, buf, outSize);
				return;
			}
		}
		else
		{
			Q_strncpyz(out, ch->desc, outSize);
			return;
		}
	}

	// MENUS_<NPC>, MENUS_<FOLDER>, with or without DH's trailing 1
	{
		char keys[4][96];
		char npc[48];
		char folder[48];
		Q_strncpyz(npc, ch->variants[0].npc, sizeof npc);
		Q_strncpyz(folder, ch->folder, sizeof folder);
		Q_strupr(npc);
		Q_strupr(folder);
		Com_sprintf(keys[0], sizeof keys[0], "MENUS_%s", folder);
		Com_sprintf(keys[1], sizeof keys[1], "MENUS_%s1", folder);
		Com_sprintf(keys[2], sizeof keys[2], "MENUS_%s", npc);
		Com_sprintf(keys[3], sizeof keys[3], "MENUS_%s1", npc);
		for (int i = 0; i < 4; i++)
		{
			buf[0] = 0;
			if (trap->SE_GetStringTextString(keys[i], buf, sizeof buf) && buf[0] && !Q_stricmpn(buf, "Name:", 5))
			{
				Q_strncpyz(out, buf, outSize);
				return;
			}
		}
	}

	Com_sprintf(out, outSize, "Name: %s. Species: %s. Weapon of choice: %s.",
		ch->group ? "Various" : ch->name, ch->species, ch->weapon);
}

static void SJE_ShowDesc(const int charIndex)
{
	menuDef_t* menu = SJE_Menu();
	if (!menu)
	{
		return;
	}
	if (charIndex < 0 || charIndex >= sjeNumChars)
	{
		sjeDescText[0] = ' ';
		sjeDescText[1] = 0;
	}
	else
	{
		SJE_BuildDesc(&sjeChars[charIndex], sjeDescText, sizeof sjeDescText);
	}
	Menu_SetItemText(menu, "char_desc", sjeDescText);
	Menu_ShowItemByName(menu, "char_desc", qtrue);
}

static void SJE_ShowVariant(void)
{
	menuDef_t* menu = SJE_Menu();
	if (!menu)
	{
		return;
	}
	if (sjeSel < 0 || sjeSel >= sjeNumChars || sjeChars[sjeSel].numVariants <= 1)
	{
		Q_strncpyz(sjeVarText, "^1None", sizeof sjeVarText);
	}
	else
	{
		Com_sprintf(sjeVarText, sizeof sjeVarText, "^2%s", sjeChars[sjeSel].variants[sjeVar].label);
	}
	Menu_SetItemText(menu, "sje_char_var", sjeVarText);
}

static void SJE_FillSlots(void)
{
	menuDef_t* menu = SJE_Menu();
	if (!menu)
	{
		return;
	}
	const int pages = SJE_NumPages();
	if (sjePage >= pages)
	{
		sjePage = pages - 1;
	}
	if (sjePage < 0)
	{
		sjePage = 0;
	}
	for (int s = 0; s < SJE_MAX_SLOTS; s++)
	{
		itemDef_t* item = Menu_FindItemByName(menu, va("sje_slot%d", s));
		const int li = sjePage * SJE_MAX_SLOTS + s;
		if (!item)
		{
			continue;
		}
		if (li < sjeListCount)
		{
			sjeChar_t* ch = &sjeChars[sjeList[li]];
			item->window.background = SJE_Icon(ch);
			item->window.flags |= WINDOW_VISIBLE;
			const qboolean sel = sjeList[li] == sjeSel ? qtrue : qfalse;
			item->window.borderColor[0] = sel ? 1.0f : 0.66f;
			item->window.borderColor[1] = sel ? 0.682f : 0.66f;
			item->window.borderColor[2] = sel ? 0.0f : 1.0f;
			item->window.borderColor[3] = 1.0f;
		}
		else
		{
			item->window.flags &= ~(WINDOW_VISIBLE | WINDOW_MOUSEOVER | WINDOW_HASFOCUS);
		}
	}
	Menu_ShowItemByName(menu, "sje_prev", sjePage > 0 ? qtrue : qfalse);
	Menu_ShowItemByName(menu, "sje_next", sjePage < pages - 1 ? qtrue : qfalse);
}

static void SJE_SetPreviewSaberCvars(const sjeVariant_t* v, const qboolean forApply);

static void SJE_UpdatePreview(void)
{
	menuDef_t* menu = SJE_Menu();
	if (!menu || sjeSel < 0 || sjeSel >= sjeNumChars)
	{
		return;
	}
	itemDef_t* item = Menu_FindItemByName(menu, "character");
	if (!item)
	{
		return;
	}
	const sjeVariant_t* v = &sjeChars[sjeSel].variants[sjeVar];
	int runLength = 0;

	SJE_SetPreviewSaberCvars(v, qfalse);
	trap->Cvar_Set("ui_char_color_red", "255");
	trap->Cvar_Set("ui_char_color_green", "255");
	trap->Cvar_Set("ui_char_color_blue", "255");
	{
		const float s = sjeChars[sjeSel].scale;
		modelDef_t* modelPtr = item->typeData.model;
		static qboolean boundsSaved;
		static vec3_t menuMins, menuMaxs;
		if (modelPtr)
		{// a big creature: move the camera back (the menu's model_g2mins/maxs divided by the scale)
			if (!boundsSaved)
			{
				VectorCopy(modelPtr->g2mins, menuMins);
				VectorCopy(modelPtr->g2maxs, menuMaxs);
				boundsSaved = qtrue;
			}
			const float k = s > 0.05f && s < 1.0f ? 1.0f / s : 1.0f;
			VectorScale(menuMins, k, modelPtr->g2mins);
			VectorScale(menuMaxs, k, modelPtr->g2maxs);
		}
	}
	ItemParse_asset_model_go(item, va("models/players/%s/model.glm", v->g2Folder), &runLength);
	ItemParse_model_g2skin_go(item, v->g2Skin);
	if (v->saber[0][0] && item->ghoul2)
	{
		UI_SaberAttachToChar(item);
	}
}

static void SJE_Select(const int charIndex, const int variant)
{
	if (charIndex < 0 || charIndex >= sjeNumChars)
	{
		return;
	}
	sjeSel = charIndex;
	sjeVar = variant >= 0 && variant < sjeChars[charIndex].numVariants ? variant : 0;
	SJE_FillSlots();
	SJE_ShowDesc(sjeSel);
	SJE_ShowVariant();
	SJE_UpdatePreview();
}

static void SJE_SetFaction(const int faction, const qboolean selectFirst)
{
	sjeFaction = faction >= 0 && faction < SJEF_COUNT ? faction : SJEF_EMPIRE;
	trap->Cvar_Set("fraction", sjeFactionKeys[sjeFaction]);
	{
		menuDef_t* menu = SJE_Menu();
		if (menu)
		{
			Menu_SetItemText(menu, "speciesbut", sjeFactionNames[sjeFaction]);
		}
	}
	SJE_BuildFactionList();
	sjePage = 0;
	if (selectFirst)
	{
		if (sjeListCount > 0)
		{
			SJE_Select(sjeList[0], 0);
		}
		else
		{
			sjeSel = -1;
			SJE_FillSlots();
			SJE_ShowDesc(-1);
			SJE_ShowVariant();
		}
	}
}

// the faction cvar changed (the FACTION multi, or the console)
static void SJE_CheckFactionCvar(void)
{
	char buf[64];
	trap->Cvar_VariableStringBuffer("fraction", buf, sizeof buf);
	const int f = SJE_FactionForKey(buf);
	if (f >= 0 && f != sjeFaction)
	{
		SJE_SetFaction(f, qtrue);
	}
}

// ---------------------------------------------------------------------------------------------------------------
// sabers
// ---------------------------------------------------------------------------------------------------------------
static qboolean SJE_SaberOkInMP(const char* saber)
{
	char buf[1024];

	buf[0] = 0;
	if (!WP_SaberParseParm(saber, "saberModel", buf) || !buf[0])
	{
		return qfalse; // no such saber
	}
	buf[0] = 0;
	if (WP_SaberParseParm(saber, "notInMP", buf) && buf[0] && atoi(buf) != 0)
	{
		return qfalse;
	}
	return qtrue;
}

// the MP saber for an NPC saber ("" = none)
static void SJE_MPSaber(const char* npcSaber, char* out, const int outSize)
{
	out[0] = 0;
	if (!npcSaber[0] || !Q_stricmp(npcSaber, "none"))
	{
		return;
	}
	for (int i = 0; i < sjeNumSaberMaps; i++)
	{
		if (!Q_stricmp(sjeSaberMaps[i].from, npcSaber) && SJE_SaberOkInMP(sjeSaberMaps[i].to))
		{
			Q_strncpyz(out, sjeSaberMaps[i].to, outSize);
			return;
		}
	}
	if (SJE_SaberOkInMP(npcSaber))
	{
		Q_strncpyz(out, npcSaber, outSize);
		return;
	}
	if (SJE_SaberOkInMP(va("%s_mp", npcSaber)))
	{
		Com_sprintf(out, outSize, "%s_mp", npcSaber);
		return;
	}
	if (SJE_SaberOkInMP(va("%sMP", npcSaber)))
	{
		Com_sprintf(out, outSize, "%sMP", npcSaber);
		return;
	}
	Q_strncpyz(out, DEFAULT_SABER, outSize);
}

// colour name, or SJE's "xRRGGBB" -> "rgb" + r g b
static void SJE_SaberColor(const char* npcColor, char* name, const int nameSize, int rgb[3])
{
	rgb[0] = 255;
	rgb[1] = 0;
	rgb[2] = 0;
	if ((npcColor[0] == 'x' || npcColor[0] == 'X') && strlen(npcColor) == 7)
	{
		const unsigned long v = strtoul(npcColor + 1, NULL, 16);
		rgb[0] = (int)(v >> 16 & 0xff);
		rgb[1] = (int)(v >> 8 & 0xff);
		rgb[2] = (int)(v & 0xff);
		Q_strncpyz(name, "rgb", nameSize);
		return;
	}
	Q_strncpyz(name, npcColor[0] ? npcColor : "blue", nameSize);
}

// The ui_saber* cvars draw the preview's sabers (and are what the saber menus edit). The menu's onClose runs
// getsabercvars, which puts them back from saber1/saber2 if the player leaves without APPLY.
static void SJE_SetPreviewSaberCvars(const sjeVariant_t* v, const qboolean forApply)
{
	char s1[48], s2[48];
	char c1[16], c2[16];
	int rgb1[3], rgb2[3];

	uiSJECharDualPreview = qfalse;
	if (!v->saber[0][0])
	{
		return;
	}
	SJE_MPSaber(v->saber[0], s1, sizeof s1);
	SJE_MPSaber(v->saber[1], s2, sizeof s2);
	SJE_SaberColor(v->color[0], c1, sizeof c1, rgb1);
	SJE_SaberColor(v->color[1][0] ? v->color[1] : v->color[0], c2, sizeof c2, rgb2);

	trap->Cvar_Set("ui_saber", s1);
	trap->Cvar_Set("ui_saber2", s2[0] ? s2 : "none");
	trap->Cvar_Set("ui_saber_color", c1);
	trap->Cvar_Set("ui_saber2_color", c2);
	trap->Cvar_Set("ui_sab1_r", va("%d", rgb1[0]));
	trap->Cvar_Set("ui_sab1_g", va("%d", rgb1[1]));
	trap->Cvar_Set("ui_sab1_b", va("%d", rgb1[2]));
	trap->Cvar_Set("ui_sab2_r", va("%d", rgb2[0]));
	trap->Cvar_Set("ui_sab2_g", va("%d", rgb2[1]));
	trap->Cvar_Set("ui_sab2_b", va("%d", rgb2[2]));
	uiSJECharDualPreview = s2[0] ? qtrue : qfalse;

	if (forApply)
	{// as UI_UpdateSaberCvars / UI_UpdateSaberColor (ui_main.c); an NPC saber is never a saber-builder hilt
		trap->Cvar_Set("saber1", s1);
		trap->Cvar_Set("saber2", s2[0] ? s2 : "none");
		trap->Cvar_Set("saber1_skin", "");
		trap->Cvar_Set("saber2_skin", "");
		trap->Cvar_Set("color1", va("%d", TranslateSaberColor(c1)));
		trap->Cvar_Set("color2", va("%d", TranslateSaberColor(c2)));
		trap->Cvar_Set("g_saber_color", c1);
		trap->Cvar_Set("g_saber2_color", c2);
		trap->Cvar_Set("rgb_saber1", va("%d,%d,%d", rgb1[0], rgb1[1], rgb1[2]));
		trap->Cvar_Set("rgb_saber2", va("%d,%d,%d", rgb2[0], rgb2[1], rgb2[2]));
	}
}

static void SJE_Apply(void)
{
	if (sjeSel < 0 || sjeSel >= sjeNumChars)
	{
		return;
	}
	const sjeChar_t* ch = &sjeChars[sjeSel];
	const sjeVariant_t* v = &ch->variants[sjeVar];

	SJE_SetPreviewSaberCvars(v, qtrue);
	trap->Cvar_Set("char_color_red", "255");
	trap->Cvar_Set("char_color_green", "255");
	trap->Cvar_Set("char_color_blue", "255");
	trap->Cvar_Set("ui_selectedmodel_index", "-1");
	trap->Cvar_Set("model", v->useModel);
	Com_Printf("SJE character: %s (%s) -> model %s, class %d, sabers %s %s\n", ch->name, v->label, v->useModel, v->botclass,
		v->saber[0][0] ? UI_Cvar_VariableString("saber1") : "(unchanged)", v->saber[0][0] ? UI_Cvar_VariableString("saber2") : "");
}

// preselect the character the player is using now
static qboolean SJE_SelectCurrent(void)
{
	char model[MAX_QPATH];
	trap->Cvar_VariableStringBuffer("model", model, sizeof model);
	for (int c = 0; c < sjeNumChars; c++)
	{
		for (int v = 0; v < sjeChars[c].numVariants; v++)
		{
			if (!Q_stricmp(sjeChars[c].variants[v].useModel, model) || !Q_stricmp(sjeChars[c].variants[v].modelString, model))
			{
				if (sjeChars[c].hidden)
				{
					return qfalse;
				}
				SJE_SetFaction(sjeChars[c].faction, qfalse);
				for (int i = 0; i < sjeListCount; i++)
				{
					if (sjeList[i] == c)
					{
						sjePage = i / SJE_MAX_SLOTS;
					}
				}
				SJE_Select(c, v);
				return qtrue;
			}
		}
	}
	return qfalse;
}

// uiScript names (from ingameclass.menu) and the console command sje_charmenu <script> [arg]
qboolean UI_SJEChars_Script(const char* name, char** args)
{
	int n = 0;

	if (Q_stricmpn(name, "sje_char", 8))
	{
		return qfalse;
	}
	UI_SJEChars_Build();

	if (!Q_stricmp(name, "sje_char_init"))
	{
		uiInfo.movesTitleIndex = 0;
		if (!SJE_SelectCurrent())
		{
			char buf[64];
			trap->Cvar_VariableStringBuffer("fraction", buf, sizeof buf);
			const int f = SJE_FactionForKey(buf);
			sjeFaction = -1;
			SJE_SetFaction(f >= 0 ? f : SJEF_EMPIRE, qtrue);
		}
	}
	else if (!Q_stricmp(name, "sje_char_faction"))
	{
		SJE_CheckFactionCvar();
	}
	else if (!Q_stricmp(name, "sje_char_nextfaction"))
	{// the FACTION box: next faction (sje_char_nextfaction -1: previous)
		if (!Int_Parse(args, &n))
		{
			n = 1;
		}
		SJE_SetFaction((sjeFaction + (n < 0 ? SJEF_COUNT - 1 : 1)) % SJEF_COUNT, qtrue);
	}
	else if (!Q_stricmp(name, "sje_char_pick"))
	{
		SJE_CheckFactionCvar();
		if (Int_Parse(args, &n))
		{
			const int li = sjePage * SJE_MAX_SLOTS + n;
			if (n >= 0 && n < SJE_MAX_SLOTS && li < sjeListCount)
			{
				SJE_Select(sjeList[li], 0);
			}
		}
	}
	else if (!Q_stricmp(name, "sje_char_hover"))
	{
		if (Int_Parse(args, &n))
		{
			const int li = sjePage * SJE_MAX_SLOTS + n;
			SJE_ShowDesc(n >= 0 && n < SJE_MAX_SLOTS && li < sjeListCount ? sjeList[li] : sjeSel);
		}
	}
	else if (!Q_stricmp(name, "sje_char_var"))
	{
		if (sjeSel >= 0 && sjeSel < sjeNumChars && sjeChars[sjeSel].numVariants > 1)
		{
			SJE_Select(sjeSel, (sjeVar + 1) % sjeChars[sjeSel].numVariants);
		}
	}
	else if (!Q_stricmp(name, "sje_char_page"))
	{
		if (Int_Parse(args, &n))
		{
			sjePage += n < 0 ? -1 : 1;
			SJE_FillSlots();
		}
	}
	else if (!Q_stricmp(name, "sje_char_close"))
	{// the menu closed: saber menu cvars back from saber1/saber2 (the preview used them), then updateForceStatus
		static const char* const closeScripts[] = { "getsabercvars", "updateForceStatus" };
		uiSJECharDualPreview = qfalse;
		for (int i = 0; i < (int)ARRAY_LEN(closeScripts); i++)
		{
			char cmd[64];
			char* p = cmd;
			Q_strncpyz(cmd, closeScripts[i], sizeof cmd);
			uiInfo.uiDC.runScript(&p);
		}
	}
	else if (!Q_stricmp(name, "sje_char_apply"))
	{
		SJE_Apply();
	}
	else if (!Q_stricmp(name, "sje_char_applyclose"))
	{// console/testing: what the APPLY button does (sje_char_apply, then the uiScript closeingame)
		SJE_Apply();
		trap->Key_SetCatcher(trap->Key_GetCatcher() & ~KEYCATCH_UI);
		trap->Key_ClearStates();
		trap->Cvar_Set("cl_paused", "0");
		Menus_CloseAll();
	}
	else if (!Q_stricmp(name, "sje_char_select"))
	{// console/testing: sje_charmenu sje_char_select <folder> [variant]
		const char* folder;
		if (String_Parse(args, &folder))
		{
			char f[48];
			Q_strncpyz(f, folder, sizeof f);
			if (!Int_Parse(args, &n))
			{
				n = 0;
			}
			for (int c = 0; c < sjeNumChars; c++)
			{
				if (!Q_stricmp(sjeChars[c].folder, f))
				{
					SJE_SetFaction(sjeChars[c].faction, qfalse);
					for (int i = 0; i < sjeListCount; i++)
					{
						if (sjeList[i] == c)
						{
							sjePage = i / SJE_MAX_SLOTS;
						}
					}
					SJE_Select(c, n);
					break;
				}
			}
		}
	}
	else
	{
		Com_Printf("unknown SJE character script %s\n", name);
	}
	return qtrue;
}

// sje_charlist [file]: the whole list (for the owner, and for the tool that makes DH's skin files)
static void SJE_DumpList(const char* fileName)
{
	fileHandle_t f = 0;
	if (fileName && fileName[0])
	{
		trap->FS_Open(fileName, &f, FS_WRITE);
	}
	for (int c = 0; c < sjeNumChars; c++)
	{
		const sjeChar_t* ch = &sjeChars[c];
		for (int v = 0; v < ch->numVariants; v++)
		{
			const sjeVariant_t* var = &ch->variants[v];
			const char* line = va("%s\t%s\t%s\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s|%s\t%s|%s\t%d\t%s\n",
				sjeFactionKeys[ch->faction], ch->folder, ch->name, v, var->label, var->npc, var->modelString,
				var->skin[0] ? var->skin : "-", var->useModel, var->g2Skin, var->saber[0], var->saber[1], var->color[0],
				var->color[1], var->botclass, ch->hidden ? "hidden" : "");
			if (f)
			{
				trap->FS_Write(line, (int)strlen(line), f);
			}
			else
			{
				Com_Printf("%s", line);
			}
		}
	}
	if (f)
	{
		trap->FS_Close(f);
		Com_Printf("SJE characters: list written to %s\n", fileName);
	}
}

qboolean UI_SJEChars_ConsoleCommand(const char* cmd)
{
	char arg[MAX_STRING_CHARS];

	if (!Q_stricmp(cmd, "sje_charlist"))
	{
		UI_SJEChars_Build();
		trap->Cmd_Argv(1, arg, sizeof arg);
		SJE_DumpList(arg);
		{// the FACTION multi of the menu (debug)
			const menuDef_t* menu = SJE_Menu();
			const itemDef_t* item = menu ? Menu_FindItemByName(menu, "speciesbut") : NULL;
			if (item && item->type == ITEM_TYPE_MULTI && item->typeData.multi)
			{
				const multiDef_t* m = item->typeData.multi;
				Com_Printf("speciesbut: cvar %s, %d values, strDef %d, text '%s' font %d scale %.2f\n", item->cvar ? item->cvar : "-",
					m->count, m->strDef, item->text ? item->text : "-", item->i_menu_font, item->textscale);
				for (int i = 0; i < m->count && i < MAX_MULTI_CVARS; i++)
				{
					Com_Printf("  [%d] '%s' = '%s'\n", i, m->cvarList[i] ? m->cvarList[i] : "(null)", m->cvarStr[i] ? m->cvarStr[i] : "(null)");
				}
			}
		}
		return qtrue;
	}
	if (!Q_stricmp(cmd, "sje_charmenu"))
	{
		char line[MAX_STRING_CHARS];
		char* p = line;
		line[0] = 0;
		for (int i = 1; i < trap->Cmd_Argc(); i++)
		{
			trap->Cmd_Argv(i, arg, sizeof arg);
			Q_strcat(line, sizeof line, va("%s ", arg));
		}
		const char* name;
		if (String_Parse(&p, &name))
		{
			char script[64];
			Q_strncpyz(script, name, sizeof script);
			UI_SJEChars_Script(script, &p);
		}
		return qtrue;
	}
	return qfalse;
}