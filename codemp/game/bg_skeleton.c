// bg_skeleton.c -- SerenityJediEngine2026 MP: player model skeleton checks, shared by game, cgame and ui.
//
// Moved unchanged from bg_panimate.c (2026-10-11) so the ui's character menu decides "playable" with exactly the
// rules the server (G_AssignClassAndScaleFromModel) and the client (CG_RegisterClientModelname) use:
// the glm/gla file readers, BG_GLAIsHumanoid, BG_ModelSkeletonFlags / BG_ModelOwnSkeletonGLA, BG_ClassIsCreature and
// BG_ClassSkeletonOk. BG_ParseAnimationFileForGLA (animation.cfg next to the GLA) stays in bg_panimate.c for the game and
// cgame; the ui has its own version on top of UI_ParseAnimationFile (ui_sjechars.c).

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "anims.h"
#include "teams.h"

#ifdef _GAME
#include "g_local.h"
#elif _CGAME
#include "cgame/cg_local.h"
#elif UI_BUILD
#include "ui/ui_local.h"
#endif

/*
======================
Skeleton file readers (see bg_panimate.c "SJE: per-model skeletons")
======================
*/
#define BG_GLM_READ_SIZE	131072	// enough for the glm header and surface hierarchy
#define BG_GLA_READ_SIZE	65536	// enough for the gla header and skeleton (bone names)
#define BG_MDXM_HEADER_SIZE	164
#define BG_MDXA_HEADER_SIZE	100
#define BG_MAX_SKEL_ITEMS	1024

static byte bgGLMReadBuf[BG_GLM_READ_SIZE];
static byte bgGLAReadBuf[BG_GLA_READ_SIZE];

// read the start of a file (up to bufSize bytes); returns the number of bytes read, 0 on failure
static int BG_ReadFileStart(const char* path, byte* buf, const int bufSize)
{
	fileHandle_t f = 0;
	const int len = trap->FS_Open(path, &f, FS_READ);

	if (!f)
	{
		return 0;
	}
	if (len <= 0)
	{
		trap->FS_Close(f);
		return 0;
	}

	const int n = len < bufSize ? len : bufSize;
	trap->FS_Read(buf, n, f);
	trap->FS_Close(f);
	return n;
}

static int BG_ReadFileInt(const byte* buf, const int len, const int ofs)
{
	int v = 0;

	if (ofs < 0 || ofs + (int)sizeof v > len)
	{
		return -1;
	}
	memcpy(&v, buf + ofs, sizeof v);
	return v;
}

// a name stored in a fixed char[64] (MAX_QPATH) field of the file, bounds checked
static const char* BG_ReadFileName(const byte* buf, const int len, const int ofs)
{
	if (ofs < 0 || ofs + MAX_QPATH > len)
	{
		return NULL;
	}
	if (!memchr(buf + ofs, 0, MAX_QPATH))
	{
		return NULL;
	}
	return (const char*)(buf + ofs);
}

// gla (mdxaHeader_t): numBones at 84; the bone offset table follows the 100 byte header, each bone starts with its name
static qboolean BG_GLAHasBone(const byte* buf, const int len, const char* boneName)
{
	if (len < BG_MDXA_HEADER_SIZE || memcmp(buf, "2LGA", 4) != 0)
	{
		return qfalse;
	}

	const int numBones = BG_ReadFileInt(buf, len, 84);
	if (numBones <= 0 || numBones > BG_MAX_SKEL_ITEMS)
	{
		return qfalse;
	}

	for (int i = 0; i < numBones; i++)
	{
		const int boneOfs = BG_ReadFileInt(buf, len, BG_MDXA_HEADER_SIZE + i * 4);
		if (boneOfs < 0)
		{
			return qfalse;
		}
		const char* name = BG_ReadFileName(buf, len, BG_MDXA_HEADER_SIZE + boneOfs);
		if (name && !Q_stricmp(name, boneName))
		{
			return qtrue;
		}
	}
	return qfalse;
}

// glm (mdxmHeader_t): numSurfaces at 152, ofsSurfHierarchy at 156; each mdxmSurfHierarchy_t is
// name[64] flags shader[64] shaderIndex parentIndex numChildren childIndexes[numChildren]
static qboolean BG_GLMHasSurface(const byte* buf, const int len, const char* surfName)
{
	if (len < BG_MDXM_HEADER_SIZE || memcmp(buf, "2LGM", 4) != 0)
	{
		return qfalse;
	}

	const int numSurfaces = BG_ReadFileInt(buf, len, 152);
	int ofs = BG_ReadFileInt(buf, len, 156);
	if (numSurfaces <= 0 || numSurfaces > BG_MAX_SKEL_ITEMS || ofs < BG_MDXM_HEADER_SIZE)
	{
		return qfalse;
	}

	for (int i = 0; i < numSurfaces; i++)
	{
		const char* name = BG_ReadFileName(buf, len, ofs);
		const int numChildren = BG_ReadFileInt(buf, len, ofs + MAX_QPATH + 4 + MAX_QPATH + 4 + 4);
		if (!name || numChildren < 0 || numChildren > BG_MAX_SKEL_ITEMS)
		{
			return qfalse;
		}
		if (!Q_stricmp(name, surfName))
		{
			return qtrue;
		}
		ofs += MAX_QPATH + 4 + MAX_QPATH + 4 + 4 + 4 + numChildren * 4;
	}
	return qfalse;
}

// G2API_AddBolt looks for a surface (tag) of that name first, then a bone
static qboolean BG_SkelHasBolt(const int glmLen, const int glaLen, const char* name)
{
	return BG_GLMHasSurface(bgGLMReadBuf, glmLen, name) || BG_GLAHasBone(bgGLAReadBuf, glaLen, name) ? qtrue : qfalse;
}

// the bones the humanoid-only code uses (bolts, look angles, Motion, ragdoll, IK, dismemberment)
static const char* const bgHumanoidKeyBones[] = {
	"model_root", "pelvis", "Motion", "lower_lumbar", "upper_lumbar", "thoracic", "cervical", "cranium",
	"rhumerus", "lhumerus", "rradius", "lradius", "rhand", "lhand",
	"rfemurYZ", "lfemurYZ", "rtibia", "ltibia", "rtalus", "ltalus"
};

qboolean BG_GLAIsHumanoid(const char* glaName)
{
	char path[MAX_QPATH];

	Com_sprintf(path, sizeof path, "%s.gla", glaName);
	const int glaLen = BG_ReadFileStart(path, bgGLAReadBuf, BG_GLA_READ_SIZE);
	if (glaLen <= 0)
	{
		return qfalse;
	}

	for (int i = 0; i < (int)ARRAY_LEN(bgHumanoidKeyBones); i++)
	{
		if (!BG_GLAHasBone(bgGLAReadBuf, glaLen, bgHumanoidKeyBones[i]))
		{
			return qfalse;
		}
	}
	return qtrue;
}

// The player bolts/bones of a model's own skeleton ("folder", "folder/skin" or "folder/|head|torso|legs"), SKELF_*.
// 0 when its model.glm names players/_humanoid/ (the normal path) or the skeleton cannot be used by a player at all.
// A usable own skeleton: the glm names a GLA outside players/_humanoid/, that GLA's folder has an animation.cfg with
// BOTH_STAND1, and the GLA has model_root. SKELF_FULL = it also has every bolt/bone the humanoid player code needs
// (the fixed test SJE MP used before creatures were playable). Which of the others are needed depends on the botclass
// (BG_ClassSkeletonOk). The server (G_PlayerHasCustomSkeleton) and the client (CG_RegisterClientModelname) both use this.
static int BG_ModelSkeletonCheck(const char* modelName, char* glaOut, const int glaOutSize)
{
	char folder[MAX_QPATH];
	char path[MAX_QPATH];
	char gla[MAX_QPATH];
	int flags = SKELF_OWN;

	if (glaOut && glaOutSize > 0)
	{
		glaOut[0] = 0;
	}
	if (!modelName || !modelName[0])
	{
		return 0;
	}

	Q_strncpyz(folder, modelName, sizeof folder);
	char* p = strchr(folder, '/');
	if (p)
	{
		*p = 0;
	}
	p = strchr(folder, '|');
	if (p)
	{
		*p = 0;
	}
	if (!folder[0])
	{
		return 0;
	}

	Com_sprintf(path, sizeof path, "models/players/%s/model.glm", folder);
	const int glmLen = BG_ReadFileStart(path, bgGLMReadBuf, BG_GLM_READ_SIZE);
	const char* animName = BG_ReadFileName(bgGLMReadBuf, glmLen, 72);
	if (glmLen < BG_MDXM_HEADER_SIZE || memcmp(bgGLMReadBuf, "2LGM", 4) != 0 || !animName || !animName[0])
	{
		return 0;
	}
	Q_strncpyz(gla, animName, sizeof gla);

	if (strstr(gla, "players/_humanoid/"))
	{
		return 0; // the master skeleton: the normal path
	}

	const int animIndex = BG_ParseAnimationFileForGLA(gla);
	if (animIndex < 0 || animIndex >= MAX_ANIM_FILES || !bgAllAnims[animIndex].anims ||
		bgAllAnims[animIndex].anims[BOTH_STAND1].numFrames <= 0)
	{
		return 0; // no animation.cfg next to the GLA, or not even a stand anim in it
	}

	Com_sprintf(path, sizeof path, "%s.gla", gla);
	const int glaLen = BG_ReadFileStart(path, bgGLAReadBuf, BG_GLA_READ_SIZE);
	if (glaLen <= 0 || !BG_GLAHasBone(bgGLAReadBuf, glaLen, "model_root"))
	{
		return 0;
	}

	if (BG_SkelHasBolt(glmLen, glaLen, "*r_hand"))
	{
		flags |= SKELF_RHAND;
	}
	if (BG_SkelHasBolt(glmLen, glaLen, "*l_hand"))
	{
		flags |= SKELF_LHAND;
	}
	const qboolean headTop = BG_SkelHasBolt(glmLen, glaLen, "*head_top") || BG_SkelHasBolt(glmLen, glaLen, "ceyebrow")
		? qtrue : qfalse;
	if (headTop || BG_SkelHasBolt(glmLen, glaLen, "*head_t") || BG_SkelHasBolt(glmLen, glaLen, "*head_eyes"))
	{
		flags |= SKELF_HEAD;
	}
	if (BG_SkelHasBolt(glmLen, glaLen, "Motion"))
	{
		flags |= SKELF_MOTION;
	}
	if (BG_SkelHasBolt(glmLen, glaLen, "lower_lumbar"))
	{
		flags |= SKELF_LLUMBAR;
	}
	if (BG_GLAHasBone(bgGLAReadBuf, glaLen, "upper_lumbar"))
	{
		flags |= SKELF_ULUMBAR;
	}
	if (BG_GLAHasBone(bgGLAReadBuf, glaLen, "cranium"))
	{
		flags |= SKELF_CRANIUM;
	}

	// the old fixed test: bolts *r_hand, *l_hand, *head_top (or ceyebrow), Motion, lower_lumbar; bones model_root,
	// upper_lumbar, cranium
	if (headTop &&
		(flags & (SKELF_RHAND | SKELF_LHAND | SKELF_MOTION | SKELF_LLUMBAR | SKELF_ULUMBAR | SKELF_CRANIUM)) ==
		(SKELF_RHAND | SKELF_LHAND | SKELF_MOTION | SKELF_LLUMBAR | SKELF_ULUMBAR | SKELF_CRANIUM))
	{
		flags |= SKELF_FULL;
	}

	if (glaOut && glaOutSize > 0)
	{
		Q_strncpyz(glaOut, gla, glaOutSize);
	}
	return flags;
}

// Does the model's glm name players/_humanoid/ (the normal player skeleton)? With BG_ModelSkeletonFlags: a model is usable
// as a player when this is true or its own skeleton has flags (the client draws the default model otherwise).
qboolean BG_ModelUsesHumanoidGLA(const char* modelName)
{
	char folder[MAX_QPATH];
	char path[MAX_QPATH];

	if (!modelName || !modelName[0])
	{
		return qfalse;
	}
	Q_strncpyz(folder, modelName, sizeof folder);
	char* p = strchr(folder, '/');
	if (p)
	{
		*p = 0;
	}
	p = strchr(folder, '|');
	if (p)
	{
		*p = 0;
	}
	if (!folder[0])
	{
		return qfalse;
	}
	Com_sprintf(path, sizeof path, "models/players/%s/model.glm", folder);
	const int glmLen = BG_ReadFileStart(path, bgGLMReadBuf, BG_MDXM_HEADER_SIZE);
	const char* animName = BG_ReadFileName(bgGLMReadBuf, glmLen, 72);
	if (glmLen < BG_MDXM_HEADER_SIZE || memcmp(bgGLMReadBuf, "2LGM", 4) != 0 || !animName)
	{
		return qfalse;
	}
	return strstr(animName, "players/_humanoid/") ? qtrue : qfalse;
}

int BG_ModelSkeletonFlags(const char* modelName)
{
	return BG_ModelSkeletonCheck(modelName, NULL, 0);
}

// Does this player model use its own skeleton (any that BG_ModelSkeletonCheck accepts)? Whether the player's class
// may use it is decided by BG_ClassSkeletonOk (the server resets other models to the default model).
qboolean BG_ModelOwnSkeletonGLA(const char* modelName, char* glaOut, const int glaOutSize)
{
	return BG_ModelSkeletonCheck(modelName, glaOut, glaOutSize) ? qtrue : qfalse;
}

// ---------------------------------------------------------------------------------------------------------------
// SJE: creature and droid player classes (the rest of these class helpers are in bg_panimate.c)
// ---------------------------------------------------------------------------------------------------------------
qboolean BG_ClassIsCreature(const int botclass)
{
	switch (botclass)
	{
	case BCLASS_R2D2:
	case BCLASS_R5D2:
	case BCLASS_GONK:
	case BCLASS_PROTOCOL:
	case BCLASS_WAMPA:
	case BCLASS_RANCOR:
	case BCLASS_HOWLER:
	case BCLASS_MINEMONSTER:
	case BCLASS_ASSASSIN_DROID:
	case BCLASS_HAZARDTROOPER:
	case BCLASS_SABER_DROID:
		return qtrue;
	default:
		return qfalse;
	}
}

// Every model with the full humanoid bolts is fine for any class (as before). A creature skeleton only for a creature
// class, and only with the bolts that class needs: a hand for a hand-held weapon (the weapon model sits on *r_hand,
// the saber on *r_hand / *l_hand). Melee and the stun baton hit from the view origin, so they need no hand.
qboolean BG_ClassSkeletonOk(const int botclass, const int skelFlags)
{
	if (!(skelFlags & SKELF_OWN) || skelFlags & SKELF_FULL)
	{
		return qtrue;
	}
	if (!BG_ClassIsCreature(botclass))
	{
		return qfalse;
	}
	switch (botclass)
	{
	case BCLASS_ASSASSIN_DROID:
	case BCLASS_HAZARDTROOPER:
		return skelFlags & SKELF_RHAND ? qtrue : qfalse;
	case BCLASS_SABER_DROID:
		return (skelFlags & (SKELF_RHAND | SKELF_LHAND)) == (SKELF_RHAND | SKELF_LHAND) ? qtrue : qfalse;
	default:
		return qtrue;
	}
}