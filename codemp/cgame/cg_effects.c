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

// cg_effects.c -- these functions generate localentities, usually as a result
// of event processing

#include "cg_local.h"

/*
==================
CG_BubbleTrail

Bullets shot underwater
==================
*/
void CG_BubbleTrail(vec3_t start, vec3_t end, const float spacing)
{
	vec3_t move;
	vec3_t vec;

	if (cg_noProjectileTrail.integer)
	{
		return;
	}

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	const float len = VectorNormalize(vec);

	// advance a random amount first
	int i = rand() % (int)spacing;
	VectorMA(move, i, vec, move);

	VectorScale(vec, spacing, vec);

	for (; i < len; i += spacing)
	{
		localEntity_t* le = CG_AllocLocalEntity();
		le->leFlags = LEF_PUFF_DONT_SCALE;
		le->leType = LE_MOVE_SCALE_FADE;
		le->startTime = cg.time;
		le->endTime = cg.time + 1000 + Q_flrand(-250.0f, 250.0f);
		le->lifeRate = 1.0 / (le->endTime - le->startTime);

		refEntity_t* re = &le->refEntity;
		re->shaderTime = cg.time / 1000.0f;

		re->reType = RT_SPRITE;
		re->rotation = 0;
		re->radius = 3;
		re->customShader = 0; //cgs.media.waterBubbleShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;

		le->color[3] = 1.0;

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.time;
		VectorCopy(move, le->pos.trBase);
		le->pos.trDelta[0] = Q_flrand(-5.0f, 5.0f);
		le->pos.trDelta[1] = Q_flrand(-5.0f, 5.0f);
		le->pos.trDelta[2] = Q_flrand(-5.0f, 5.0f) + 6;

		VectorAdd(move, vec, move);
	}
}

/*
=====================
CG_SmokePuff

Adds a smoke puff or blood trail localEntity.
=====================
*/
localEntity_t* CG_SmokePuff(const vec3_t p, const vec3_t vel,
	const float radius,
	const float r, const float g, const float b, const float a,
	const float duration,
	const int start_time,
	const int fade_in_time,
	const int le_flags,
	const qhandle_t hShader)
{
	static int seed = 0x92;
	//	int fadeInTime = startTime + duration / 2;

	localEntity_t* le = CG_AllocLocalEntity();
	le->leFlags = le_flags;
	le->radius = radius;

	refEntity_t* re = &le->refEntity;
	re->rotation = Q_random(&seed) * 360;
	re->radius = radius;
	re->shaderTime = start_time / 1000.0f;

	le->leType = LE_MOVE_SCALE_FADE;
	le->startTime = start_time;
	le->fadeInTime = fade_in_time;
	le->endTime = start_time + duration;
	if (fade_in_time > start_time)
	{
		le->lifeRate = 1.0 / (le->endTime - le->fadeInTime);
	}
	else
	{
		le->lifeRate = 1.0 / (le->endTime - le->startTime);
	}
	le->color[0] = r;
	le->color[1] = g;
	le->color[2] = b;
	le->color[3] = a;

	le->pos.trType = TR_LINEAR;
	le->pos.trTime = start_time;
	VectorCopy(vel, le->pos.trDelta);
	VectorCopy(p, le->pos.trBase);

	VectorCopy(p, re->origin);
	re->customShader = hShader;

	re->shaderRGBA[0] = le->color[0] * 0xff;
	re->shaderRGBA[1] = le->color[1] * 0xff;
	re->shaderRGBA[2] = le->color[2] * 0xff;
	re->shaderRGBA[3] = 0xff;

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	return le;
}

int CGDEBUG_SaberColor(const int saber_color)
{
	switch (saber_color)
	{
	case SABER_RED:
		return 0x000000ff;
	case SABER_ORANGE:
		return 0x000088ff;
	case SABER_YELLOW:
		return 0x0000ffff;
	case SABER_GREEN:
		return 0x0000ff00;
	case SABER_BLUE:
		return 0x00ff0000;
	case SABER_PURPLE:
		return 0x00ff00ff;
	case SABER_LIME:
		return 0x0000ff00;
	default:
		return saber_color;
	}
}

void CG_TestLine(vec3_t start, vec3_t end, const int time, unsigned int color, const int radius)
{
	localEntity_t* le = CG_AllocLocalEntity();
	le->leType = LE_LINE;
	le->startTime = cg.time;
	le->endTime = cg.time + time;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);

	refEntity_t* re = &le->refEntity;
	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);
	re->shaderTime = cg.time / 1000.0f;

	re->reType = RT_LINE;
	re->radius = 0.5 * radius;
	re->customShader = cgs.media.whiteShader;

	re->shaderTexCoord[0] = re->shaderTexCoord[1] = 1.0f;

	if (color == 0)
	{
		re->shaderRGBA[0] = re->shaderRGBA[1] = re->shaderRGBA[2] = re->shaderRGBA[3] = 0xff;
	}
	else
	{
		color = CGDEBUG_SaberColor(color);
		re->shaderRGBA[0] = color & 0xff;
		color >>= 8;
		re->shaderRGBA[1] = color & 0xff;
		color >>= 8;
		re->shaderRGBA[2] = color & 0xff;
		re->shaderRGBA[3] = 0xff;
	}

	le->color[3] = 1.0;
}

void CG_BlockLine(vec3_t start, vec3_t end, const int time, unsigned int color, const int radius)
{
	localEntity_t* le = CG_AllocLocalEntity();
	le->leType = LE_LINE;
	le->startTime = cg.time;
	le->endTime = cg.time + time;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);

	refEntity_t* re = &le->refEntity;
	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);
	re->shaderTime = cg.time / 1000.0f;

	re->reType = RT_LINE;
	re->radius = 0.1 * radius;
	re->customShader = cgs.media.whiteShader;

	re->shaderTexCoord[0] = re->shaderTexCoord[1] = 1.0f;

	if (color == 0)
	{
		re->shaderRGBA[0] = re->shaderRGBA[1] = re->shaderRGBA[2] = re->shaderRGBA[3] = 0xff;
	}
	else
	{
		color = CGDEBUG_SaberColor(color);
		re->shaderRGBA[0] = color & 0xff;
		color >>= 8;
		re->shaderRGBA[1] = color & 0xff;
		color >>= 8;
		re->shaderRGBA[2] = color & 0xff;
		re->shaderRGBA[3] = 0xff;
	}

	le->color[3] = 1.0;
}

void CG_StunStartpoint(vec3_t start_pos)
{
	refEntity_t model = { 0 };

	model.reType = RT_SABER_GLOW;
	VectorCopy(start_pos, model.lightingOrigin);
	VectorCopy(start_pos, model.origin);

	model.customShader = cgs.media.blueSaberGlowShader;
	model.shaderRGBA[0] = model.shaderRGBA[1] = model.shaderRGBA[2] = model.shaderRGBA[3] = 0xff;

	trap->R_AddRefEntityToScene(&model);
}

void CG_GrappleStartpoint(vec3_t start_pos)
{
	refEntity_t model = { 0 };

	model.reType = RT_SABER_GLOW;
	VectorCopy(start_pos, model.lightingOrigin);
	VectorCopy(start_pos, model.origin);

	model.customShader = cgs.media.rgbSaberGlowShader;
	model.shaderRGBA[0] = model.shaderRGBA[1] = model.shaderRGBA[2] = model.shaderRGBA[3] = 0xff;

	trap->R_AddRefEntityToScene(&model);
}

void CG_GrappleLine(vec3_t start, vec3_t end, const int time, unsigned int color, const int radius)
{
	localEntity_t* le = CG_AllocLocalEntity();
	le->leType = LE_LINE;
	le->startTime = cg.time;
	le->endTime = cg.time + time;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);

	refEntity_t* re = &le->refEntity;
	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);
	re->shaderTime = cg.time / 1000.0f;

	re->reType = RT_LINE;
	re->radius = 0.5 * radius;
	re->customShader = cgs.media.electricBodyShader;

	re->shaderTexCoord[0] = re->shaderTexCoord[1] = 1.0f;

	if (color == 0)
	{
		re->shaderRGBA[0] = re->shaderRGBA[1] = re->shaderRGBA[2] = re->shaderRGBA[3] = 0xff;
	}
	else
	{
		color = CGDEBUG_SaberColor(color);
		re->shaderRGBA[0] = color & 0xff;
		color >>= 8;
		re->shaderRGBA[1] = color & 0xff;
		color >>= 8;
		re->shaderRGBA[2] = color & 0xff;
		re->shaderRGBA[3] = 0xff;
	}

	le->color[3] = 1.0;
}

//----------------------------
//
// Breaking Glass Technology
//
//----------------------------

// Since we have shared verts when we tesselate the glass sheet, it helps to have a
//	random offset table set up up front.

static float off_x[20][20],
off_z[20][20];

#define	FX_ALPHA_NONLINEAR	0x00000004
#define FX_APPLY_PHYSICS	0x02000000
#define FX_USE_ALPHA		0x08000000

static void CG_DoGlassQuad(vec3_t p[4], vec2_t uv[4], const qboolean stick, const int time, vec3_t dmg_dir)
{
	vec3_t rot_delta;
	vec3_t vel, accel;
	vec3_t rgb1;
	addpolyArgStruct_t ap_args;

	VectorSet(vel, Q_flrand(-12.0f, 12.0f), Q_flrand(-12.0f, 12.0f), -1);

	if (!stick)
	{
		// We aren't a motion delayed chunk, so let us move quickly
		VectorMA(vel, 0.3f, dmg_dir, vel);
	}

	// Set up acceleration due to gravity, 800 is standard QuakeIII gravity, so let's use something close
	VectorSet(accel, 0.0f, 0.0f, -(600.0f + Q_flrand(0.0f, 1.0f) * 100.0f));

	// We are using an additive shader, so let's set the RGB low so we look more like transparent glass
	//	VectorSet( rgb1, 0.1f, 0.1f, 0.1f );
	VectorSet(rgb1, 1.0f, 1.0f, 1.0f);

	// Being glass, we don't want to bounce much
	const float bounce = Q_flrand(0.0f, 1.0f) * 0.2f + 0.15f;

	// Set up our random rotate, we only do PITCH and YAW, not ROLL.  This is something like degrees per second
	VectorSet(rot_delta, Q_flrand(-40.0f, 40.0f), Q_flrand(-40.0f, 40.0f), 0.0f);

	//rww - this is dirty.

	int i = 0;
	int i_2 = 0;

	while (i < 4)
	{
		while (i_2 < 3)
		{
			ap_args.p[i][i_2] = p[i][i_2];

			i_2++;
		}

		i_2 = 0;
		i++;
	}

	i = 0;
	i_2 = 0;

	while (i < 4)
	{
		while (i_2 < 2)
		{
			ap_args.ev[i][i_2] = uv[i][i_2];

			i_2++;
		}

		i_2 = 0;
		i++;
	}

	ap_args.numVerts = 4;
	VectorCopy(vel, ap_args.vel);
	VectorCopy(accel, ap_args.accel);

	ap_args.alpha1 = 0.15f;
	ap_args.alpha2 = 0.0f;
	ap_args.alphaParm = 85.0f;

	VectorCopy(rgb1, ap_args.rgb1);
	VectorCopy(rgb1, ap_args.rgb2);

	ap_args.rgbParm = 0.0f;

	VectorCopy(rot_delta, ap_args.rotationDelta);

	ap_args.bounce = bounce;
	ap_args.motionDelay = time;
	ap_args.killTime = 6000;
	ap_args.shader = cgs.media.glassShardShader;
	ap_args.flags = FX_APPLY_PHYSICS | FX_ALPHA_NONLINEAR | FX_USE_ALPHA;

	trap->FX_AddPoly(&ap_args);
}

static void CG_CalcBiLerp(vec3_t verts[4], vec3_t sub_verts[4], vec2_t uv[4])
{
	vec3_t temp;

	// Nasty crap
	VectorScale(verts[0], 1.0f - uv[0][0], sub_verts[0]);
	VectorMA(sub_verts[0], uv[0][0], verts[1], sub_verts[0]);
	VectorScale(sub_verts[0], 1.0f - uv[0][1], temp);
	VectorScale(verts[3], 1.0f - uv[0][0], sub_verts[0]);
	VectorMA(sub_verts[0], uv[0][0], verts[2], sub_verts[0]);
	VectorMA(temp, uv[0][1], sub_verts[0], sub_verts[0]);

	VectorScale(verts[0], 1.0f - uv[1][0], sub_verts[1]);
	VectorMA(sub_verts[1], uv[1][0], verts[1], sub_verts[1]);
	VectorScale(sub_verts[1], 1.0f - uv[1][1], temp);
	VectorScale(verts[3], 1.0f - uv[1][0], sub_verts[1]);
	VectorMA(sub_verts[1], uv[1][0], verts[2], sub_verts[1]);
	VectorMA(temp, uv[1][1], sub_verts[1], sub_verts[1]);

	VectorScale(verts[0], 1.0f - uv[2][0], sub_verts[2]);
	VectorMA(sub_verts[2], uv[2][0], verts[1], sub_verts[2]);
	VectorScale(sub_verts[2], 1.0f - uv[2][1], temp);
	VectorScale(verts[3], 1.0f - uv[2][0], sub_verts[2]);
	VectorMA(sub_verts[2], uv[2][0], verts[2], sub_verts[2]);
	VectorMA(temp, uv[2][1], sub_verts[2], sub_verts[2]);

	VectorScale(verts[0], 1.0f - uv[3][0], sub_verts[3]);
	VectorMA(sub_verts[3], uv[3][0], verts[1], sub_verts[3]);
	VectorScale(sub_verts[3], 1.0f - uv[3][1], temp);
	VectorScale(verts[3], 1.0f - uv[3][0], sub_verts[3]);
	VectorMA(sub_verts[3], uv[3][0], verts[2], sub_verts[3]);
	VectorMA(temp, uv[3][1], sub_verts[3], sub_verts[3]);
}

// bilinear
//f(p',q') = (1 - y) × {[(1 - x) × f(p,q)] + [x × f(p,q+1)]} + y × {[(1 - x) × f(p+1,q)] + [x × f(p+1,q+1)]}.

static void CG_CalcHeightWidth(vec3_t verts[4], float* height, float* width)
{
	vec3_t dir1, dir2, cross;

	VectorSubtract(verts[3], verts[0], dir1); // v
	VectorSubtract(verts[1], verts[0], dir2); // p-a
	CrossProduct(dir1, dir2, cross);
	*width = VectorNormalize(cross) / VectorNormalize(dir1); // v
	VectorSubtract(verts[2], verts[0], dir2); // p-a
	CrossProduct(dir1, dir2, cross);
	*width += VectorNormalize(cross) / VectorNormalize(dir1); // v
	*width *= 0.5f;

	VectorSubtract(verts[1], verts[0], dir1); // v
	VectorSubtract(verts[2], verts[0], dir2); // p-a
	CrossProduct(dir1, dir2, cross);
	*height = VectorNormalize(cross) / VectorNormalize(dir1); // v
	VectorSubtract(verts[3], verts[0], dir2); // p-a
	CrossProduct(dir1, dir2, cross);
	*height += VectorNormalize(cross) / VectorNormalize(dir1); // v
	*height *= 0.5f;
}

//Consider a line in 3D with position vector "a" and direction vector "v" and
// let "p" be the position vector of an arbitrary point in 3D
//dist = len( crossprod(p-a,v) ) / len(v);

void CG_InitGlass(void)
{
	// Build a table first, so that we can do a more unpredictable crack scheme
	//	do it once, up front to save a bit of time.
	for (int i = 0; i < 20; i++)
	{
		for (int t = 0; t < 20; t++)
		{
			off_x[t][i] = Q_flrand(-1.0f, 1.0f) * 0.03f;
			off_z[i][t] = Q_flrand(-1.0f, 1.0f) * 0.03f;
		}
	}
}

#define TIME_DECAY_SLOW		0.1f
#define TIME_DECAY_MED		0.04f
#define TIME_DECAY_FAST		0.009f

void CG_DoGlass(vec3_t verts[], vec3_t dmg_pt, vec3_t dmg_dir, const float dmg_radius, const int max_shards)
{
	int i, t;
	int mx_height;
	float height, width;
	float step_height;
	float time_decay;
	float x, z;
	float xx, zz;
	int time;
	int glass_shards = 0;
	qboolean stick;

	// To do a smarter tesselation, we should figure out the relative height and width of the brush face,
	//	then use this to pick a lod value from 1-3 in each axis.  This will give us 1-9 lod levels, which will
	//	hopefully be sufficient.
	CG_CalcHeightWidth(verts, &height, &width);

	trap->S_StartSound(dmg_pt, -1, CHAN_AUTO, trap->S_RegisterSound("sound/effects/glassbreak1.wav"));

	// Pick "LOD" for height
	if (height < 100)
	{
		step_height = 0.2f;
		mx_height = 5;
		time_decay = TIME_DECAY_SLOW;
	}
	else if (height > 220)
	{
		step_height = 0.05f;
		mx_height = 20;
		time_decay = TIME_DECAY_FAST;
	}
	else
	{
		step_height = 0.1f;
		mx_height = 10;
		time_decay = TIME_DECAY_MED;
	}

	//Attempt to scale the glass directly to the size of the window

	float step_width = 0.25f - width * 0.0002; //(width*0.0005));
	int mx_width = width * 0.2;
	time_decay = (time_decay + TIME_DECAY_FAST) * 0.5f;

	if (step_width < 0.01f)
	{
		step_width = 0.01f;
	}
	if (mx_width < 5)
	{
		mx_width = 5;
	}

	for (z = 0.0f, i = 0; z < 1.0f; z += step_height, i++)
	{
		for (x = 0.0f, t = 0; x < 1.0f; x += step_width, t++)
		{
			vec2_t bi_points[4];
			vec3_t sub_verts[4];
			// This is nasty..
			if (t > 0 && t < mx_width)
			{
				xx = x - off_x[i][t];
			}
			else
			{
				xx = x;
			}

			if (i > 0 && i < mx_height)
			{
				zz = z - off_z[t][i];
			}
			else
			{
				zz = z;
			}

			VectorSet2(bi_points[0], xx, zz);

			if (t + 1 > 0 && t + 1 < mx_width)
			{
				xx = x - off_x[i][t + 1];
			}
			else
			{
				xx = x;
			}

			if (i > 0 && i < mx_height)
			{
				zz = z - off_z[t + 1][i];
			}
			else
			{
				zz = z;
			}

			VectorSet2(bi_points[1], xx + step_width, zz);

			if (t + 1 > 0 && t + 1 < mx_width)
			{
				xx = x - off_x[i + 1][t + 1];
			}
			else
			{
				xx = x;
			}

			if (i + 1 > 0 && i + 1 < mx_height)
			{
				zz = z - off_z[t + 1][i + 1];
			}
			else
			{
				zz = z;
			}

			VectorSet2(bi_points[2], xx + step_width, zz + step_height);

			if (t > 0 && t < mx_width)
			{
				xx = x - off_x[i + 1][t];
			}
			else
			{
				xx = x;
			}

			if (i + 1 > 0 && i + 1 < mx_height)
			{
				zz = z - off_z[t][i + 1];
			}
			else
			{
				zz = z;
			}

			VectorSet2(bi_points[3], xx, zz + step_height);

			CG_CalcBiLerp(verts, sub_verts, bi_points);

			float dif = DistanceSquared(sub_verts[0], dmg_pt) * time_decay - Q_flrand(0.0f, 1.0f) * 32;

			// If we decrease dif, we are increasing the impact area, making it more likely to blow out large holes
			dif -= dmg_radius * dmg_radius;

			if (dif > 1)
			{
				stick = qtrue;
				time = dif + Q_flrand(0.0f, 1.0f) * 200;
			}
			else
			{
				stick = qfalse;
				time = 0;
			}

			CG_DoGlassQuad(sub_verts, bi_points, stick, time, dmg_dir);
			glass_shards++;

			if (max_shards && glass_shards >= max_shards)
			{
				return;
			}
		}
	}
}

/*
==================
CG_GlassShatter
Break glass with fancy method
==================
*/
void CG_GlassShatter(const int entnum, vec3_t dmg_pt, vec3_t dmg_dir, const float dmg_radius, const int max_shards)
{
	if (cgs.inlineDrawModel[cg_entities[entnum].currentState.modelIndex])
	{
		vec3_t normal;
		vec3_t verts[4];
		trap->R_GetBModelVerts(cgs.inlineDrawModel[cg_entities[entnum].currentState.modelIndex], verts, normal);
		CG_DoGlass(verts, dmg_pt, dmg_dir, dmg_radius, max_shards);
	}
	//otherwise something awful has happened.
}

//==========================================================
//SP-style chunks
//==========================================================

/*
-------------------------
CG_ExplosionEffects

Used to find the player and shake the camera if close enough
intensity ranges from 1 (minor tremble) to 16 (major quake)
-------------------------
*/

void CG_ExplosionEffects(vec3_t origin, const float intensity, const int radius, const int time)
{
	//FIXME: When exactly is the vieworg calculated in relation to the rest of the frame?s

	vec3_t dir;

	VectorSubtract(cg.refdef.vieworg, origin, dir);
	const float dist = VectorNormalize(dir);

	//Use the dir to add kick to the explosion

	if (dist > radius)
		return;

	const float intensity_scale = 1 - dist / (float)radius;
	const float real_intensity = intensity * intensity_scale;

	CGCam_Shake(real_intensity, time);
}

/*
-------------------------
CG_MiscModelExplosion

Adds an explosion to a misc model breakables
-------------------------
*/

void CG_MiscModelExplosion(vec3_t mins, vec3_t maxs, const int size, const material_t chunk_type)
{
	int ct = 13;
	vec3_t org, mid;
	const char* effect = NULL, * effect2 = NULL;
	int e_id2 = 0;

	VectorAdd(mins, maxs, mid);
	VectorScale(mid, 0.5f, mid);

	switch (chunk_type)
	{
	default:
		break;
	case MAT_GLASS:
		effect = "chunks/glassbreak";
		ct = 5;
		break;
	case MAT_GLASS_METAL:
		effect = "chunks/glassbreak";
		effect2 = "chunks/metalexplode";
		ct = 5;
		break;
	case MAT_ELECTRICAL:
	case MAT_ELEC_METAL:
		effect = "chunks/sparkexplode";
		ct = 5;
		break;
	case MAT_METAL:
	case MAT_METAL2:
	case MAT_METAL3:
	case MAT_CRATE1:
	case MAT_CRATE2:
		effect = "chunks/metalexplode";
		ct = 2;
		break;
	case MAT_GRATE1:
		effect = "chunks/grateexplode";
		ct = 8;
		break;
	case MAT_ROPE:
		ct = 20;
		effect = "chunks/ropebreak";
		break;
	case MAT_WHITE_METAL: //not sure what this crap is really supposed to be..
	case MAT_DRK_STONE:
	case MAT_LT_STONE:
	case MAT_GREY_STONE:
	case MAT_SNOWY_ROCK:
		switch (size)
		{
		case 2:
			effect = "chunks/rockbreaklg";
			break;
		case 1:
		default:
			effect = "chunks/rockbreakmed";
			break;
		}
	}

	if (!effect)
	{
		return;
	}

	ct += 7 * size;

	// FIXME: real precache .. VERify that these need to be here...don't think they would because the effects should be registered in g_breakable
	//rww - No they don't.. indexed effects gameside get precached on load clientside, as server objects are setup before client asset load time.
	//However, we need to index them, so..
	const int e_id1 = trap->FX_RegisterEffect(effect);

	if (effect2 && effect2[0])
	{
		// FIXME: real precache
		e_id2 = trap->FX_RegisterEffect(effect2);
	}

	// spawn chunk roughly in the bbox of the thing..
	for (int i = 0; i < ct; i++)
	{
		vec3_t dir;
		for (int j = 0; j < 3; j++)
		{
			const float r = Q_flrand(0.0f, 1.0f) * 0.8f + 0.1f;
			org[j] = r * mins[j] + (1 - r) * maxs[j];
		}

		// shoot effect away from center
		VectorSubtract(org, mid, dir);
		VectorNormalize(dir);

		if (effect2 && effect2[0] && rand() & 1)
		{
			trap->FX_PlayEffectID(e_id2, org, dir, -1, -1, qfalse);
		}
		else
		{
			trap->FX_PlayEffectID(e_id1, org, dir, -1, -1, qfalse);
		}
	}
}

/*
-------------------------
CG_Chunks

Fun chunk spewer
-------------------------
*/

void CG_Chunks(const int owner, vec3_t origin, const vec3_t mins, const vec3_t maxs,
	const float speed, const int num_chunks, const material_t chunk_type, const int custom_chunk,
	float base_scale)
{
	int chunk_model = 0;
	leBounceSoundType_t bounce = LEBS_NONE;
	float speedMod = 1.0f;
	qboolean chunk = qfalse;

	if (chunk_type == MAT_NONE)
	{
		// Well, we should do nothing
		return;
	}

	// Set up our chunk sound info...breaking sounds are done here so they are done once on breaking..some return instantly because the chunks are done with effects instead of models
	switch (chunk_type)
	{
	default:
		break;
	case MAT_GLASS:
		trap->S_StartSound(NULL, owner, CHAN_BODY, cgs.media.glassChunkSound);
		return;
	case MAT_GRATE1:
		trap->S_StartSound(NULL, owner, CHAN_BODY, cgs.media.grateSound);
		return;
	case MAT_ELECTRICAL: // (sparks)
		trap->S_StartSound(NULL, owner, CHAN_BODY,
			trap->S_RegisterSound(va("sound/ambience/spark%d.wav", Q_irand(1, 6))));
		return;
	case MAT_DRK_STONE:
	case MAT_LT_STONE:
	case MAT_GREY_STONE:
	case MAT_WHITE_METAL: // not quite sure what this stuff is supposed to be...it's for Stu
	case MAT_SNOWY_ROCK:
		trap->S_StartSound(NULL, owner, CHAN_BODY, cgs.media.rockBreakSound);
		bounce = LEBS_ROCK;
		speedMod = 0.5f; // rock blows up less
		break;
	case MAT_GLASS_METAL:
		trap->S_StartSound(NULL, owner, CHAN_BODY, cgs.media.glassChunkSound);
		// FIXME: should probably have a custom sound
		bounce = LEBS_METAL;
		break;
	case MAT_CRATE1:
	case MAT_CRATE2:
		trap->S_StartSound(NULL, owner, CHAN_BODY, cgs.media.crateBreakSound[Q_irand(0, 1)]);
		break;
	case MAT_METAL:
	case MAT_METAL2:
	case MAT_METAL3:
	case MAT_ELEC_METAL: // FIXME: maybe have its own sound?
		trap->S_StartSound(NULL, owner, CHAN_BODY, cgs.media.chunkSound);
		bounce = LEBS_METAL;
		speedMod = 0.8f; // metal blows up a bit more
		break;
	case MAT_ROPE:
		//		trap->S_StartSound( NULL, owner, CHAN_BODY, cgi_S_RegisterSound( "" ));  FIXME:  needs a sound
		return;
	}

	if (base_scale <= 0.0f)
	{
		base_scale = 1.0f;
	}

	// Chunks
	for (int i = 0; i < num_chunks; i++)
	{
		if (custom_chunk > 0)
		{
			// Try to use a custom chunk.
			if (cgs.game_models[custom_chunk])
			{
				chunk = qtrue;
				chunk_model = cgs.game_models[custom_chunk];
			}
		}

		if (!chunk)
		{
			// No custom chunk.  Pick a random chunk type at run-time so we don't get the same chunks
			switch (chunk_type)
			{
			default:
				break;
			case MAT_METAL2: //bluegrey
				chunk_model = cgs.media.chunkModels[CHUNK_METAL2][Q_irand(0, 3)];
				break;
			case MAT_GREY_STONE: //gray
				chunk_model = cgs.media.chunkModels[CHUNK_ROCK1][Q_irand(0, 3)];
				break;
			case MAT_LT_STONE: //tan
				chunk_model = cgs.media.chunkModels[CHUNK_ROCK2][Q_irand(0, 3)];
				break;
			case MAT_DRK_STONE: //brown
				chunk_model = cgs.media.chunkModels[CHUNK_ROCK3][Q_irand(0, 3)];
				break;
			case MAT_SNOWY_ROCK: //gray & brown
				if (Q_irand(0, 1))
				{
					chunk_model = cgs.media.chunkModels[CHUNK_ROCK1][Q_irand(0, 3)];
				}
				else
				{
					chunk_model = cgs.media.chunkModels[CHUNK_ROCK3][Q_irand(0, 3)];
				}
				break;
			case MAT_WHITE_METAL:
				chunk_model = cgs.media.chunkModels[CHUNK_WHITE_METAL][Q_irand(0, 3)];
				break;
			case MAT_CRATE1: //yellow multi-colored crate chunks
				chunk_model = cgs.media.chunkModels[CHUNK_CRATE1][Q_irand(0, 3)];
				break;
			case MAT_CRATE2: //red multi-colored crate chunks
				chunk_model = cgs.media.chunkModels[CHUNK_CRATE2][Q_irand(0, 3)];
				break;
			case MAT_ELEC_METAL:
			case MAT_GLASS_METAL:
			case MAT_METAL: //grey
				chunk_model = cgs.media.chunkModels[CHUNK_METAL1][Q_irand(0, 3)];
				break;
			case MAT_METAL3:
				if (rand() & 1)
				{
					chunk_model = cgs.media.chunkModels[CHUNK_METAL1][Q_irand(0, 3)];
				}
				else
				{
					chunk_model = cgs.media.chunkModels[CHUNK_METAL2][Q_irand(0, 3)];
				}
				break;
			}
		}

		// It wouldn't look good to throw a bunch of RGB axis models...so make sure we have something to work with.
		if (chunk_model)
		{
			vec3_t dir;
			localEntity_t* le = CG_AllocLocalEntity();
			refEntity_t* re = &le->refEntity;

			re->hModel = chunk_model;
			le->leType = LE_FRAGMENT;
			le->endTime = cg.time + 1300 + Q_flrand(0.0f, 1.0f) * 900;

			// spawn chunk roughly in the bbox of the thing...bias towards center in case thing blowing up doesn't complete fill its bbox.
			for (int j = 0; j < 3; j++)
			{
				const float r = Q_flrand(0.0f, 1.0f) * 0.8f + 0.1f;
				re->origin[j] = r * mins[j] + (1 - r) * maxs[j];
			}
			VectorCopy(re->origin, le->pos.trBase);

			// Move out from center of thing, otherwise you can end up things moving across the brush in an undesirable direction.  Visually looks wrong
			VectorSubtract(re->origin, origin, dir);
			VectorNormalize(dir);
			VectorScale(dir, flrand(speed * 0.5f, speed * 1.25f) * speedMod, le->pos.trDelta);

			// Angular Velocity
			VectorSet(le->angles.trBase, Q_flrand(0.0f, 1.0f) * 360, Q_flrand(0.0f, 1.0f) * 360,
				Q_flrand(0.0f, 1.0f) * 360);

			le->angles.trDelta[0] = Q_flrand(-1.0f, 1.0f);
			le->angles.trDelta[1] = Q_flrand(-1.0f, 1.0f);
			le->angles.trDelta[2] = 0; // don't do roll

			VectorScale(le->angles.trDelta, Q_flrand(0.0f, 1.0f) * 600.0f + 200.0f, le->angles.trDelta);

			le->pos.trType = TR_GRAVITY;
			le->angles.trType = TR_LINEAR;
			le->pos.trTime = le->angles.trTime = cg.time;
			le->bounceFactor = 0.2f + Q_flrand(0.0f, 1.0f) * 0.2f;
			le->leFlags |= LEF_TUMBLE;
			//le->ownerGentNum = owner;
			le->leBounceSoundType = bounce;

			// Make sure that we have the desired start size set
			le->radius = flrand(base_scale * 0.75f, base_scale * 1.25f);
			re->nonNormalizedAxes = qtrue;
			AxisCopy(axisDefault, re->axis); // could do an angles to axis, but this is cheaper and works ok
			for (int k = 0; k < 3; k++)
			{
				re->modelScale[k] = le->radius;
			}
			ScaleModelAxis(re);
			/*
			for( k = 0; k < 3; k++ )
			{
				VectorScale( re->axis[k], le->radius, re->axis[k] );
			}
			*/
		}
	}
}

/*
==================
CG_ScorePlum
==================
*/
void CG_ScorePlum(const int client, vec3_t org, const int score)
{
	vec3_t angles;
	static vec3_t last_pos;

	// only visualize for the client that scored
	if (client != cg.predictedPlayerState.clientNum || cg_scorePlums.integer == 0)
	{
		return;
	}

	localEntity_t* le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_SCOREPLUM;
	le->startTime = cg.time;
	le->endTime = cg.time + 4000;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;
	le->radius = score;

	VectorCopy(org, le->pos.trBase);
	if (org[2] >= last_pos[2] - 20 && org[2] <= last_pos[2] + 20)
	{
		le->pos.trBase[2] -= 20;
	}

	//trap->Print( "Plum origin %i %i %i -- %i\n", (int)org[0], (int)org[1], (int)org[2], (int)Distance(org, lastPos));
	VectorCopy(org, last_pos);

	refEntity_t* re = &le->refEntity;

	re->reType = RT_SPRITE;
	re->radius = 16;

	VectorClear(angles);
	AnglesToAxis(angles, re->axis);
}

/*
====================
CG_MakeExplosion
====================
*/
localEntity_t* CG_MakeExplosion(vec3_t origin, vec3_t dir,
	const qhandle_t h_model, const int numFrames, const qhandle_t shader,
	const int msec, const qboolean is_sprite, const float scale, const int flags)
{
	vec3_t new_origin;

	if (msec <= 0)
	{
		trap->Error(ERR_DROP, "CG_MakeExplosion: msec = %i", msec);
	}

	// skew the time a bit so they aren't all in sync
	const int offset = rand() & 63;

	localEntity_t* ex = CG_AllocLocalEntity();
	if (is_sprite)
	{
		vec3_t tmp_vec;
		ex->leType = LE_SPRITE_EXPLOSION;
		ex->refEntity.rotation = rand() % 360;
		ex->radius = scale;
		VectorScale(dir, 16, tmp_vec);
		VectorAdd(tmp_vec, origin, new_origin);
	}
	else
	{
		ex->leType = LE_EXPLOSION;
		VectorCopy(origin, new_origin);

		// set axis with random rotate when necessary
		if (!dir)
		{
			AxisClear(ex->refEntity.axis);
		}
		else
		{
			float ang = 0;
			if (!(flags & LEF_NO_RANDOM_ROTATE))
				ang = rand() % 360;
			VectorCopy(dir, ex->refEntity.axis[0]);
			RotateAroundDirection(ex->refEntity.axis, ang);
		}
	}

	ex->startTime = cg.time - offset;
	ex->endTime = ex->startTime + msec;

	// bias the time so all shader effects start correctly
	ex->refEntity.shaderTime = ex->startTime / 1000.0f;

	ex->refEntity.hModel = h_model;
	ex->refEntity.customShader = shader;
	ex->lifeRate = (float)numFrames / msec;
	ex->leFlags = flags;

	//Scale the explosion
	if (scale != 1)
	{
		ex->refEntity.nonNormalizedAxes = qtrue;

		VectorScale(ex->refEntity.axis[0], scale, ex->refEntity.axis[0]);
		VectorScale(ex->refEntity.axis[1], scale, ex->refEntity.axis[1]);
		VectorScale(ex->refEntity.axis[2], scale, ex->refEntity.axis[2]);
	}
	// set origin
	VectorCopy(new_origin, ex->refEntity.origin);
	VectorCopy(new_origin, ex->refEntity.oldorigin);

	ex->color[0] = ex->color[1] = ex->color[2] = 1.0;

	return ex;
}

/*
==================
CG_LaunchGib
==================
*/
void CG_LaunchGib(vec3_t origin, vec3_t velocity, const qhandle_t h_model)
{
	localEntity_t* le = CG_AllocLocalEntity();
	refEntity_t* re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 5000 + Q_flrand(0.0f, 1.0f) * 3000;

	VectorCopy(origin, re->origin);
	AxisCopy(axisDefault, re->axis);
	re->hModel = h_model;

	le->pos.trType = TR_GRAVITY;
	VectorCopy(origin, le->pos.trBase);
	VectorCopy(velocity, le->pos.trDelta);
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.6f;

	le->leBounceSoundType = LEBS_BLOOD;
	le->leMarkType = LEMT_BLOOD;
}

inline float random()
{
	return rand() / (float)0x7fff;
}

inline float crandom()
{
	return 2.0F * (random() - 0.5F);
}

/*
===================
CG_GibPlayer

Generated a bunch of gibs launching out from the bodies location
===================
*/
#define	GIB_VELOCITY	250
#define	GIB_JUMP		250

void CG_GibPlayer(vec3_t player_origin)
{
	vec3_t origin, velocity;

	VectorCopy(player_origin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;

	VectorCopy(player_origin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;

	VectorCopy(player_origin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;

	VectorCopy(player_origin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;

	VectorCopy(player_origin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;

	VectorCopy(player_origin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;

	VectorCopy(player_origin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;

	VectorCopy(player_origin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;

	VectorCopy(player_origin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;

	VectorCopy(player_origin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
}

#define DECAPITATE_VELOCITY	  3000
#define HEAD_JUMP			  3000

void CG_GibPlayerHeadshot(vec3_t player_origin)
{
	vec3_t origin, velocity;

	VectorCopy(player_origin, origin);
	origin[2] += 25;
	velocity[0] = crandom() * DECAPITATE_VELOCITY;
	velocity[1] = crandom() * DECAPITATE_VELOCITY;
	velocity[2] = HEAD_JUMP + crandom() * DECAPITATE_VELOCITY;
	//delete from here
	VectorCopy(player_origin, origin);
	velocity[0] = crandom() * DECAPITATE_VELOCITY;
	velocity[1] = crandom() * DECAPITATE_VELOCITY;
	velocity[2] = HEAD_JUMP + crandom() * DECAPITATE_VELOCITY;

	VectorCopy(player_origin, origin);
	velocity[0] = crandom() * DECAPITATE_VELOCITY;
	velocity[1] = crandom() * DECAPITATE_VELOCITY;
	velocity[2] = HEAD_JUMP + crandom() * DECAPITATE_VELOCITY;

	VectorCopy(player_origin, origin);
	velocity[0] = crandom() * DECAPITATE_VELOCITY;
	velocity[1] = crandom() * DECAPITATE_VELOCITY;
	velocity[2] = HEAD_JUMP + crandom() * DECAPITATE_VELOCITY;
}