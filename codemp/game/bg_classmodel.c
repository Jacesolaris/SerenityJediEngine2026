// bg_classmodel.c -- SerenityJediEngine2027 MP: model string -> bot class, shared by the game (G_AssignClassAndScaleFromModel)
// and the ui (the Select Character menu shows only what the server lets a player play, with the class the server gives).
// The lookup was G_ClassModelFor in g_client.c (moved here 2026-10-11, same steps and order).

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "teams.h"

#ifdef _GAME
#include "g_local.h"
#elif _CGAME
#include "cgame/cg_local.h"
#elif UI_BUILD
#include "ui/ui_local.h"
#endif

#include "bg_classmodel.h"
#include "g_classmodels.h" // the table (classModel_t classModels[])

static char BG_ClassLCase(char c)
{
	if (c >= 'A' && c <= 'Z')
	{
		c = c | 0x20;
	}
	return c;
}

// g_client.c Class_Model: case-insensitive substring test that skips colour codes in the haystack
static qboolean BG_ClassModelHas(const char* haystack, const char* needle)
{
	if (!haystack || !needle)
	{
		return qfalse;
	}

	while (*haystack)
	{
		const char* h = haystack;
		const char* n = needle;

		if (BG_ClassLCase(*h) != BG_ClassLCase(*n))
		{
			haystack++;
			continue;
		}

		while (*h && *n)
		{
			if (*h == '^')
			{
				for (int i = 0; i < 3; i++)
				{
					if (*h)
					{
						h++;
					}
				}
				continue;
			}
			if (BG_ClassLCase(*h) != BG_ClassLCase(*n))
			{
				break;
			}
			h++;
			n++;
		}

		if (!*n)
		{
			return qtrue;
		}
		haystack++;
	}
	return qfalse;
}

qboolean BG_ModelBlocked(const char* model)
{
	static const char* const blocked[] = {
		"model_siege", "atst",
		// SJE: gonk is playable now (BCLASS_GONK: no weapon at all, as SP's gonk.npc)
		"lambdashuttle", "marka_ragnos", "probe", "rocks", "sand_creature", "sentry", "swoop", "tauntaun",
		"tie_bomber", "tie_fighter", "droideka", "droideka/main", "droideka_mp", "x-wing", "z-95"
	};

	for (int i = 0; i < (int)ARRAY_LEN(blocked); i++)
	{
		if (BG_ClassModelHas(model, blocked[i]))
		{
			return qtrue;
		}
	}
	return qfalse;
}

// ---------------------------------------------------------------------------------------------------------------
// Character-menu variant folders: a copy of a model under its own folder name (DH's pattern, e.g. stofficer = the
// stormtrooper with the officer skin). models/players/<folder>/charvariant.txt names the model string it was made from,
// so the class (and with it the loadout) is the one a player with that model string gets. Cached per folder.
// ---------------------------------------------------------------------------------------------------------------
#define MAX_CHARVARIANT_CACHE 512

typedef struct charVariantCache_s
{
	char folder[MAX_QPATH];
	char origin[MAX_QPATH];
} charVariantCache_t;

static charVariantCache_t bgCharVariants[MAX_CHARVARIANT_CACHE];
static int bgNumCharVariants;

static void BG_ReadCharVariant(const char* folder, char* origin, const int originSize)
{
	fileHandle_t f = 0;
	char buf[MAX_QPATH * 2];

	origin[0] = 0;
	const int len = trap->FS_Open(va("models/players/%s/charvariant.txt", folder), &f, FS_READ);
	if (!f)
	{
		return;
	}
	if (len > 0)
	{
		const int n = len < (int)sizeof buf - 1 ? len : (int)sizeof buf - 1;
		trap->FS_Read(buf, n, f);
		buf[n] = 0;

		// first word: the model string ("model" or "model/skin")
		const char* s = buf;
		while (*s && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n'))
		{
			s++;
		}
		int o = 0;
		while (*s && *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n' && o < originSize - 1)
		{
			origin[o++] = *s++;
		}
		origin[o] = 0;
	}
	trap->FS_Close(f);
}

const char* BG_CharVariantOrigin(const char* folder)
{
	static char uncached[MAX_QPATH];

	if (!folder || !folder[0] || strlen(folder) >= MAX_QPATH)
	{
		return "";
	}
	for (int i = 0; i < bgNumCharVariants; i++)
	{
		if (!Q_stricmp(bgCharVariants[i].folder, folder))
		{
			return bgCharVariants[i].origin;
		}
	}
	if (bgNumCharVariants < MAX_CHARVARIANT_CACHE)
	{
		charVariantCache_t* c = &bgCharVariants[bgNumCharVariants++];
		Q_strncpyz(c->folder, folder, sizeof c->folder);
		BG_ReadCharVariant(folder, c->origin, sizeof c->origin);
		return c->origin;
	}
	BG_ReadCharVariant(folder, uncached, sizeof uncached);
	return uncached;
}

// The class table row for a model string ("folder", "folder/skin" or "folder/head|torso|lower"):
// 1. a row naming exactly this model/skin (for "folder/head_X|torso_X|lower_X" also "folder/X"),
// 2. a row naming exactly this model folder - the table has short general patterns ("jedi", "reva", "ma", "ew"...)
//    listed before specific ones, and a plain substring scan let them catch e.g. revan_jedi, maul_rebels, jeditrainer,
//    nihilus/new before the rows written for those models,
// 3. else (as before) the first row whose pattern is somewhere in the model string.
static int BG_ClassModelLookup(const char* model)
{
	char clean[MAX_QPATH], folder[MAX_QPATH], uniform[MAX_QPATH];
	int n = 0;

	for (const char* s = model; *s && n < (int)sizeof clean - 1; s++)
	{
		if (Q_IsColorString(s))
		{// skip colour codes, as Class_Model does
			s++;
			continue;
		}
		clean[n++] = *s;
	}
	clean[n] = 0;

	Q_strncpyz(folder, clean, sizeof folder);
	uniform[0] = 0;
	{
		char* slash = strchr(folder, '/');
		if (slash)
		{
			*slash = 0;
			// "head_X|torso_X|lower_X" with the same X three times -> "folder/X"
			char head[MAX_QPATH], torso[MAX_QPATH], lower[MAX_QPATH];
			const char* skin = strchr(clean, '/') + 1;
			if (sscanf(skin, "head_%63[^|]|torso_%63[^|]|lower_%63s", head, torso, lower) == 3 &&
				!Q_stricmp(head, torso) && !Q_stricmp(head, lower))
			{
				Com_sprintf(uniform, sizeof uniform, "%s/%s", folder, head);
			}
		}
	}

	for (int i = 0; i < (int)ARRAY_LEN(classModels); i++)
	{
		const char* p = classModels[i].pattern;
		const char* slash = strchr(p, '/');
		if (slash && slash[1] && (!Q_stricmp(p, clean) || uniform[0] && !Q_stricmp(p, uniform)))
		{
			return i;
		}
	}

	for (int i = 0; i < (int)ARRAY_LEN(classModels); i++)
	{
		const char* p = classModels[i].pattern;
		const char* slash = strchr(p, '/');
		if (slash && slash[1])
		{
			continue; // a model/skin row
		}
		const size_t len = slash ? (size_t)(slash - p) : strlen(p);
		if (len == strlen(folder) && !Q_stricmpn(p, folder, (int)len))
		{
			return i;
		}
	}

	for (int i = 0; i < (int)ARRAY_LEN(classModels); i++)
	{
		if (classModels[i].exactOnly)
		{
			continue; // SJE: exact rows only
		}
		if (BG_ClassModelHas(model, classModels[i].pattern))
		{
			return i;
		}
	}
	return -1;
}

int BG_ClassModelIndex(const char* model)
{
	char folder[MAX_QPATH];

	if (!model || !model[0])
	{
		return -1;
	}

	Q_strncpyz(folder, model, sizeof folder);
	{
		char* p = strchr(folder, '/');
		if (p)
		{
			*p = 0;
		}
	}

	const char* origin = BG_CharVariantOrigin(folder);
	if (origin[0])
	{
		return BG_ClassModelLookup(origin);
	}
	return BG_ClassModelLookup(model);
}

int BG_ClassModelBotclass(const char* model)
{
	const int i = BG_ClassModelIndex(model);
	return i >= 0 ? classModels[i].botclass : BCLASS_SABERNOFP;
}