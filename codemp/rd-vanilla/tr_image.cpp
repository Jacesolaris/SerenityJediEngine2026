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

// tr_image.c
#include "tr_local.h"
#include "../rd-common/tr_common.h"
#include "glext.h"

#include <map>

static byte s_intensitytable[256];
static unsigned char s_gammatable[256];

int gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int gl_filter_max = GL_LINEAR;

//#define FILE_HASH_SIZE		1024	// actually the shader code still needs this (from another module, great),
//static	image_t*		hashTable[FILE_HASH_SIZE];

/*
** R_GammaCorrect
*/
void R_GammaCorrect(byte* buffer, const int buf_size)
{
	for (int i = 0; i < buf_size; i++)
	{
		buffer[i] = s_gammatable[buffer[i]];
	}
}

using textureMode_t = struct textureMode_s
{
	const char* name;
	int minimize, maximize;
};

textureMode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

static constexpr size_t numTextureModes = std::size(modes);

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode(const char* string)
{
	size_t i;
	image_t* glt;

	for (i = 0; i < numTextureModes; i++)
	{
		if (!Q_stricmp(modes[i].name, string))
		{
			break;
		}
	}

	if (i == numTextureModes)
	{
		ri->Printf(PRINT_ALL, "bad filter name\n");
		for (i = 0; i < numTextureModes; i++)
		{
			ri->Printf(PRINT_ALL, "%s\n", modes[i].name);
		}
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// If the level they requested is less than possible, set the max possible...
	if (r_ext_texture_filter_anisotropic->value > glConfig.maxTextureFilterAnisotropy)
		ri->Cvar_SetValue("r_ext_texture_filter_anisotropic", glConfig.maxTextureFilterAnisotropy);

	// change all the existing mipmap texture objects
	R_Images_StartIteration();
	while ((glt = R_Images_GetNextIteration()) != nullptr)
	{
		if (glt->mipmap)
		{
			GL_Bind(glt);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

			if (glConfig.maxTextureFilterAnisotropy > 0)
			{
				if (r_ext_texture_filter_anisotropic->integer > 1)
				{
					qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
						r_ext_texture_filter_anisotropic->value);
				}
				else
				{
					qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
				}
			}
		}
	}
}

// makeup a nice clean, consistant name to query for and file under, for map<> usage...
//
static char* GenerateImageMappingName(const char* name)
{
	static char s_name[MAX_QPATH];
	int i = 0;

	while (name[i] != '\0' && i < MAX_QPATH - 1)
	{
		char letter = tolower(static_cast<unsigned char>(name[i]));
		if (letter == '.') break; // don't include extension
		if (letter == '\\') letter = '/'; // damn path names
		s_name[i++] = letter;
	}
	s_name[i] = 0;

	return &s_name[0];
}

static float R_BytesPerTex(const int format)
{
	switch (format)
	{
	case 1:
		//"I    "
		return 1;
	case 2:
		//"IA   "
		return 2;
	case 3:
		//"RGB  "
		return glConfig.colorBits / 8.0f;
	case 4:
		//"RGBA "
		return glConfig.colorBits / 8.0f;

	case GL_RGBA4:
		//"RGBA4"
		return 2;
	case GL_RGB5:
		//"RGB5 "
		return 2;

	case GL_RGBA8:
		//"RGBA8"
		return 4;
	case GL_RGB8:
		//"RGB8"
		return 4;

	case GL_RGB4_S3TC:
		//"S3TC "
		return 0.33333f;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		//"DXT1 "
		return 0.33333f;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		//"DXT5 "
		return 1;
	default:
		//"???? "
		return 4;
	}
}

/*
===============
R_SumOfUsedImages
===============
*/
float R_SumOfUsedImages(const qboolean b_use_format)
{
	int total = 0;
	image_t* pImage;

	R_Images_StartIteration();
	while ((pImage = R_Images_GetNextIteration()) != nullptr)
	{
		if (pImage->frameUsed == tr.frameCount - 1)
		{
			//it has already been advanced for the next frame, so...
			if (b_use_format)
			{
				const float bytePerTex = R_BytesPerTex(pImage->internalFormat);
				total += bytePerTex * (pImage->width * pImage->height);
			}
			else
			{
				total += pImage->width * pImage->height;
			}
		}
	}

	return total;
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f(void)
{
	int i = 0;
	image_t* image;
	int texels = 0;
	float tex_bytes = 0.0f;
	const char* yesno[] = { "no ", "yes" };

	ri->Printf(PRINT_ALL, "\n      -w-- -h-- -mm- -if-- wrap --name-------\n");

	const int i_num_images = R_Images_StartIteration();
	while ((image = R_Images_GetNextIteration()) != nullptr)
	{
		texels += image->width * image->height;
		tex_bytes += image->width * image->height * R_BytesPerTex(image->internalFormat);
		ri->Printf(PRINT_ALL, "%4i: %4i %4i  %s ",
			i, image->width, image->height, yesno[image->mipmap]);
		switch (image->internalFormat)
		{
		case 1:
			ri->Printf(PRINT_ALL, "I    ");
			break;
		case 2:
			ri->Printf(PRINT_ALL, "IA   ");
			break;
		case 3:
			ri->Printf(PRINT_ALL, "RGB  ");
			break;
		case 4:
			ri->Printf(PRINT_ALL, "RGBA ");
			break;
		case GL_RGBA8:
			ri->Printf(PRINT_ALL, "RGBA8");
			break;
		case GL_RGB8:
			ri->Printf(PRINT_ALL, "RGB8");
			break;
		case GL_RGB4_S3TC:
			ri->Printf(PRINT_ALL, "S3TC ");
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			ri->Printf(PRINT_ALL, "DXT1 ");
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			ri->Printf(PRINT_ALL, "DXT5 ");
			break;
		case GL_RGBA4:
			ri->Printf(PRINT_ALL, "RGBA4");
			break;
		case GL_RGB5:
			ri->Printf(PRINT_ALL, "RGB5 ");
			break;
		default:
			ri->Printf(PRINT_ALL, "???? ");
		}

		switch (image->wrapClampMode)
		{
		case GL_REPEAT:
			ri->Printf(PRINT_ALL, "rept ");
			break;
		case GL_CLAMP:
			ri->Printf(PRINT_ALL, "clmp ");
			break;
		case GL_CLAMP_TO_EDGE:
			ri->Printf(PRINT_ALL, "clpE ");
			break;
		default:
			ri->Printf(PRINT_ALL, "%4i ", image->wrapClampMode);
			break;
		}

		ri->Printf(PRINT_ALL, "%s\n", image->imgName);
		i++;
	}
	ri->Printf(PRINT_ALL, " ---------\n");
	ri->Printf(PRINT_ALL, "      -w-- -h-- -mm- -if- wrap --name-------\n");
	ri->Printf(PRINT_ALL, " %i total texels (not including mipmaps)\n", texels);
	ri->Printf(PRINT_ALL, " %.2fMB total texture mem (not including mipmaps)\n", tex_bytes / 1048576.0f);
	ri->Printf(PRINT_ALL, " %i total images\n\n", i_num_images);
}

//=======================================================================

/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
static void R_LightScaleTexture(unsigned* in, const int inwidth, const int inheight, const qboolean only_gamma)
{
	if (only_gamma)
	{
		if (!glConfig.deviceSupportsGamma && !glConfigExt.doGammaCorrectionWithShaders)
		{
			auto p = reinterpret_cast<byte*>(in);

			const int c = inwidth * inheight;
			for (int i = 0; i < c; i++, p += 4)
			{
				p[0] = s_gammatable[p[0]];
				p[1] = s_gammatable[p[1]];
				p[2] = s_gammatable[p[2]];
			}
		}
	}
	else
	{
		int i;

		auto p = reinterpret_cast<byte*>(in);

		const int c = inwidth * inheight;

		if (glConfig.deviceSupportsGamma || glConfigExt.doGammaCorrectionWithShaders)
		{
			for (i = 0; i < c; i++, p += 4)
			{
				p[0] = s_intensitytable[p[0]];
				p[1] = s_intensitytable[p[1]];
				p[2] = s_intensitytable[p[2]];
			}
		}
		else
		{
			for (i = 0; i < c; i++, p += 4)
			{
				p[0] = s_gammatable[s_intensitytable[p[0]]];
				p[1] = s_gammatable[s_intensitytable[p[1]]];
				p[2] = s_gammatable[s_intensitytable[p[2]]];
			}
		}
	}
}

/*
================
R_MipMap2

Operates in place, quartering the size of the texture
Proper linear filter
================
*/
static void R_MipMap2(unsigned* in, const int inWidth, const int inheight)
{
	const int out_width = inWidth >> 1;
	const int out_height = inheight >> 1;
	const auto temp = static_cast<unsigned*>(Hunk_AllocateTempMemory(out_width * out_height * 4));

	const int in_width_mask = inWidth - 1;
	const int in_height_mask = inheight - 1;

	for (int i = 0; i < out_height; i++)
	{
		for (int j = 0; j < out_width; j++)
		{
			const auto outpix = reinterpret_cast<byte*>(temp + i * out_width + j);
			for (int k = 0; k < 4; k++)
			{
				const int total = 1 * reinterpret_cast<byte*>(&in[(i * 2 - 1 & in_height_mask) * inWidth + (j * 2 - 1 & in_width_mask)])[k] +
					2 * reinterpret_cast<byte*>(&in[(i * 2 - 1 & in_height_mask) * inWidth + (j * 2 & in_width_mask)])[k] +
					2 * reinterpret_cast<byte*>(&in[(i * 2 - 1 & in_height_mask) * inWidth + (j * 2 + 1 & in_width_mask)])[k] +
					1 * reinterpret_cast<byte*>(&in[(i * 2 - 1 & in_height_mask) * inWidth + (j * 2 + 2 & in_width_mask)])[k] +

					2 * reinterpret_cast<byte*>(&in[(i * 2 & in_height_mask) * inWidth + (j * 2 - 1 & in_width_mask)])[k] +
					4 * reinterpret_cast<byte*>(&in[(i * 2 & in_height_mask) * inWidth + (j * 2 & in_width_mask)])[k] +
					4 * reinterpret_cast<byte*>(&in[(i * 2 & in_height_mask) * inWidth + (j * 2 + 1 & in_width_mask)])[k] +
					2 * reinterpret_cast<byte*>(&in[(i * 2 & in_height_mask) * inWidth + (j * 2 + 2 & in_width_mask)])[k] +

					2 * reinterpret_cast<byte*>(&in[(i * 2 + 1 & in_height_mask) * inWidth + (j * 2 - 1 & in_width_mask)])[k] +
					4 * reinterpret_cast<byte*>(&in[(i * 2 + 1 & in_height_mask) * inWidth + (j * 2 & in_width_mask)])[k] +
					4 * reinterpret_cast<byte*>(&in[(i * 2 + 1 & in_height_mask) * inWidth + (j * 2 + 1 & in_width_mask)])[k] +
					2 * reinterpret_cast<byte*>(&in[(i * 2 + 1 & in_height_mask) * inWidth + (j * 2 + 2 & in_width_mask)])[k] +

					1 * reinterpret_cast<byte*>(&in[(i * 2 + 2 & in_height_mask) * inWidth + (j * 2 - 1 & in_width_mask)])[k] +
					2 * reinterpret_cast<byte*>(&in[(i * 2 + 2 & in_height_mask) * inWidth + (j * 2 & in_width_mask)])[k] +
					2 * reinterpret_cast<byte*>(&in[(i * 2 + 2 & in_height_mask) * inWidth + (j * 2 + 1 & in_width_mask)])[k] +
					1 * reinterpret_cast<byte*>(&in[(i * 2 + 2 & in_height_mask) * inWidth + (j * 2 + 2 & in_width_mask)])[k];
				outpix[k] = total / 36;
			}
		}
	}

	memcpy(in, temp, out_width * out_height * 4);
	Hunk_FreeTempMemory(temp);
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
static void R_MipMap(byte* in, int width, int height)
{
	int i;

	if (!r_simpleMipMaps->integer)
	{
		R_MipMap2(reinterpret_cast<unsigned*>(in), width, height);
		return;
	}

	if (width == 1 && height == 1)
	{
		return;
	}

	const int row = width * 4;
	byte* out = in;
	width >>= 1;
	height >>= 1;

	if (width == 0 || height == 0)
	{
		width += height; // get largest
		for (i = 0; i < width; i++, out += 4, in += 8)
		{
			out[0] = (in[0] + in[4]) >> 1;
			out[1] = (in[1] + in[5]) >> 1;
			out[2] = (in[2] + in[6]) >> 1;
			out[3] = (in[3] + in[7]) >> 1;
		}
		return;
	}

	for (i = 0; i < height; i++, in += row)
	{
		for (int j = 0; j < width; j++, out += 4, in += 8)
		{
			out[0] = (in[0] + in[4] + in[row + 0] + in[row + 4]) >> 2;
			out[1] = (in[1] + in[5] + in[row + 1] + in[row + 5]) >> 2;
			out[2] = (in[2] + in[6] + in[row + 2] + in[row + 6]) >> 2;
			out[3] = (in[3] + in[7] + in[row + 3] + in[row + 7]) >> 2;
		}
	}
}

/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
static void R_BlendOverTexture(byte* data, const int pixelCount, byte blend[4])
{
	int premult[3]{};

	const int inverse_alpha = 255 - blend[3];
	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	for (int i = 0; i < pixelCount; i++, data += 4)
	{
		data[0] = (data[0] * inverse_alpha + premult[0]) >> 9;
		data[1] = (data[1] * inverse_alpha + premult[1]) >> 9;
		data[2] = (data[2] * inverse_alpha + premult[2]) >> 9;
	}
}

byte mipBlendColors[16][4] = {
	{0, 0, 0, 0},
	{255, 0, 0, 128},
	{0, 255, 0, 128},
	{0, 0, 255, 128},
	{255, 0, 0, 128},
	{0, 255, 0, 128},
	{0, 0, 255, 128},
	{255, 0, 0, 128},
	{0, 255, 0, 128},
	{0, 0, 255, 128},
	{255, 0, 0, 128},
	{0, 255, 0, 128},
	{0, 0, 255, 128},
	{255, 0, 0, 128},
	{0, 255, 0, 128},
	{0, 0, 255, 128},
};

class CStringComparator
{
public:
	bool operator()(const char* s1, const char* s2) const { return strcmp(s1, s2) < 0; }
};

using AllocatedImages_t = std::map<const char*, image_t*, CStringComparator>;
AllocatedImages_t AllocatedImages;
AllocatedImages_t::iterator itAllocatedImages;
int giTextureBindNum = 1024; // will be set to this anyway at runtime, but wtf?

// return = number of images in the list, for those interested
//
int R_Images_StartIteration()
{
	itAllocatedImages = AllocatedImages.begin();
	return AllocatedImages.size();
}

image_t* R_Images_GetNextIteration()
{
	if (itAllocatedImages == AllocatedImages.end())
		return nullptr;

	image_t* pImage = (*itAllocatedImages).second;
	++itAllocatedImages;
	return pImage;
}

// clean up anything to do with an image_t struct, but caller will have to clear the internal to an image_t struct ready for either struct free() or overwrite...
//
// (avoid using ri->xxxx stuff here in case running on dedicated)
//
static void R_Images_DeleteImageContents(image_t* pImage)
{
	assert(pImage); // should never be called with NULL
	if (pImage)
	{
		qglDeleteTextures(1, &pImage->texnum);
		Z_Free(pImage);
	}
}

/*
===============
Upload32

===============
*/
static void Upload32(unsigned* data, const GLenum format, const qboolean mipmap, const qboolean picmip, const qboolean is_lightmap, const qboolean allow_tc, int* pformat, word* p_upload_width, word* p_upload_height, const bool b_rectangle = false)
{
	GLuint uiTarget = GL_TEXTURE_2D;
	if (b_rectangle)
	{
		uiTarget = GL_TEXTURE_RECTANGLE_ARB;
	}

	if (format == GL_RGBA)
	{
		int i;
		float r_max = 0, g_max = 0, bMax = 0;
		int width = *p_upload_width;
		int height = *p_upload_height;

		//
		// perform optional picmip operation
		//
		if (picmip)
		{
			for (i = 0; i < r_picmip->integer; i++)
			{
				R_MipMap(reinterpret_cast<byte*>(data), width, height);
				width >>= 1;
				height >>= 1;
				if (width < 1)
				{
					width = 1;
				}
				if (height < 1)
				{
					height = 1;
				}
			}
		}

		//
		// clamp to the current upper OpenGL limit
		// scale both axis down equally so we don't have to
		// deal with a half mip resampling
		//
		while (width > glConfig.maxTextureSize || height > glConfig.maxTextureSize)
		{
			R_MipMap(reinterpret_cast<byte*>(data), width, height);
			width >>= 1;
			height >>= 1;
		}

		//
		// scan the texture for each channel's max values
		// and verify if the alpha channel is being used or not
		//
		const int c = width * height;
		const auto scan = reinterpret_cast<byte*>(data);
		int samples = 3;
		for (i = 0; i < c; i++)
		{
			if (scan[i * 4 + 0] > r_max)
			{
				r_max = scan[i * 4 + 0];
			}
			if (scan[i * 4 + 1] > g_max)
			{
				g_max = scan[i * 4 + 1];
			}
			if (scan[i * 4 + 2] > bMax)
			{
				bMax = scan[i * 4 + 2];
			}
			if (scan[i * 4 + 3] != 255)
			{
				samples = 4;
				break;
			}
		}

		// select proper internal format
		if (samples == 3)
		{
			if (glConfig.textureCompression == TC_S3TC && allow_tc)
			{
				*pformat = GL_RGB4_S3TC;
			}
			else if (glConfig.textureCompression == TC_S3TC_DXT && allow_tc)
			{
				// Compress purely color - no alpha
				if (r_texturebits->integer == 16)
				{
					*pformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT; //this format cuts to 16 bit
				}
				else
				{
					//if we aren't using 16 bit then, use 32 bit compression
					*pformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				}
			}
			else if (is_lightmap && r_texturebitslm->integer > 0)
			{
				const int lm_bits = r_texturebitslm->integer & 0x30; // 16 or 32
				// Allow different bit depth when we are a lightmap
				if (lm_bits == 16)
					*pformat = GL_RGB5;
				else
					*pformat = GL_RGB8;
			}
			else if (r_texturebits->integer == 16)
			{
				*pformat = GL_RGB5;
			}
			else if (r_texturebits->integer == 32)
			{
				*pformat = GL_RGB8;
			}
			else
			{
				*pformat = 3;
			}
		}
		else if (samples == 4)
		{
			if (glConfig.textureCompression == TC_S3TC_DXT && allow_tc)
			{
				// Compress both alpha and color
				*pformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			}
			else if (r_texturebits->integer == 16)
			{
				*pformat = GL_RGBA4;
			}
			else if (r_texturebits->integer == 32)
			{
				*pformat = GL_RGBA8;
			}
			else
			{
				*pformat = 4;
			}
		}

		*p_upload_width = width;
		*p_upload_height = height;

		// copy or resample data as appropriate for first MIP level
		if (!mipmap)
		{
			qglTexImage2D(uiTarget, 0, *pformat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			goto done;
		}

		R_LightScaleTexture(data, width, height, static_cast<qboolean>(!mipmap));

		qglTexImage2D(uiTarget, 0, *pformat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		if (mipmap)
		{
			int miplevel = 0;
			while (width > 1 || height > 1)
			{
				R_MipMap(reinterpret_cast<byte*>(data), width, height);
				width >>= 1;
				height >>= 1;
				if (width < 1)
					width = 1;
				if (height < 1)
					height = 1;
				miplevel++;

				if (r_colorMipLevels->integer)
				{
					R_BlendOverTexture(reinterpret_cast<byte*>(data), width * height, mipBlendColors[miplevel]);
				}

				qglTexImage2D(uiTarget, miplevel, *pformat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			}
		}
	}
	else
	{
	}

done:

	if (mipmap)
	{
		qglTexParameterf(uiTarget, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(uiTarget, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		if (r_ext_texture_filter_anisotropic->integer > 1 && glConfig.maxTextureFilterAnisotropy > 0)
		{
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_ext_texture_filter_anisotropic->value);
		}
	}
	else
	{
		qglTexParameterf(uiTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(uiTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	GL_CheckErrors();
}

static void GL_ResetBinds()
{
	memset(glState.currenttextures, 0, sizeof glState.currenttextures);
	if (qglActiveTextureARB)
	{
		GL_SelectTexture(1);
		qglBindTexture(GL_TEXTURE_2D, 0);
		GL_SelectTexture(0);
		qglBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		qglBindTexture(GL_TEXTURE_2D, 0);
	}
}

// special function used in conjunction with "devmapbsp"...
//
// (avoid using ri->xxxx stuff here in case running on dedicated)
//
void R_Images_DeleteLightMaps(void)
{
	for (auto it_image = AllocatedImages.begin(); it_image != AllocatedImages.end(); /* empty */)
	{
		image_t* pImage = (*it_image).second;

		if (pImage->imgName[0] == '*' && strstr(pImage->imgName, "lightmap")) // loose check, but should be ok
		{
			R_Images_DeleteImageContents(pImage);

			AllocatedImages.erase(it_image++);
		}
		else
		{
			++it_image;
		}
	}

	GL_ResetBinds();
}

// special function currently only called by Dissolve code...
//
void R_Images_DeleteImage(image_t* pImage)
{
	// Even though we supply the image handle, we need to get the corresponding iterator entry...
	//
	const auto it_image = AllocatedImages.find(pImage->imgName);
	if (it_image != AllocatedImages.end())
	{
		R_Images_DeleteImageContents(pImage);
		AllocatedImages.erase(it_image);
	}
	else
	{
		assert(0);
	}
}

// called only at app startup, vid_restart, app-exit
//
void R_Images_Clear()
{
	image_t* pImage;
	//	int iNumImages =
	R_Images_StartIteration();
	while ((pImage = R_Images_GetNextIteration()) != nullptr)
	{
		R_Images_DeleteImageContents(pImage);
	}

	AllocatedImages.clear();

	giTextureBindNum = 1024;
}

void RE_RegisterImages_Info_f()
{
	image_t* pImage;
	int i_image = 0;
	int i_texels = 0;

	const int i_num_images = R_Images_StartIteration();
	while ((pImage = R_Images_GetNextIteration()) != nullptr)
	{
		ri->Printf(PRINT_ALL, "%d: (%4dx%4dy) \"%s\"", i_image, pImage->width, pImage->height, pImage->imgName);
		ri->Printf(PRINT_DEVELOPER, S_COLOR_RED ", levused %d", pImage->iLastLevelUsedOn);
		ri->Printf(PRINT_ALL, "\n");

		i_texels += pImage->width * pImage->height;
		i_image++;
	}
	ri->Printf(PRINT_ALL, "%d Images. %d (%.2fMB) texels total, (not including mipmaps)\n", i_num_images, i_texels,
		static_cast<float>(i_texels) / 1024.0f / 1024.0f);
	ri->Printf(PRINT_DEVELOPER, S_COLOR_RED "RE_RegisterMedia_GetLevel(): %d", RE_RegisterMedia_GetLevel());
}

// currently, this just goes through all the images and dumps any not referenced on this level...
//
qboolean RE_RegisterImages_LevelLoadEnd()
{
	ri->Printf(PRINT_DEVELOPER, S_COLOR_RED "RE_RegisterImages_LevelLoadEnd():\n");

	//	int iNumImages = AllocatedImages.size();	// more for curiosity, really.

	qboolean image_deleted = qfalse;
	for (auto it_image = AllocatedImages.begin(); it_image != AllocatedImages.end(); /* blank */)
	{
		qboolean b_erase_occured = qfalse;

		image_t* pImage = (*it_image).second;

		// don't un-register system shaders (*fog, *dlight, *white, *default), but DO de-register lightmaps ("*<mapname>/lightmap%d")
		if (pImage->imgName[0] != '*' || strchr(pImage->imgName, '/'))
		{
			// image used on this level?
			//
			if (pImage->iLastLevelUsedOn != RE_RegisterMedia_GetLevel())
			{
				// nope, so dump it...
				//
				ri->Printf(PRINT_DEVELOPER, S_COLOR_RED "Dumping image \"%s\"\n", pImage->imgName);

				R_Images_DeleteImageContents(pImage);

				AllocatedImages.erase(it_image++);
				b_erase_occured = qtrue;
				image_deleted = qtrue;
			}
		}

		if (!b_erase_occured)
		{
			++it_image;
		}
	}

	ri->Printf(PRINT_DEVELOPER, S_COLOR_RED "RE_RegisterImages_LevelLoadEnd(): Ok\n");

	GL_ResetBinds();

	return image_deleted;
}

// returns image_t struct if we already have this, else NULL. No disk-open performed
//	(important for creating default images).
//
// This is called by both R_FindImageFile and anything that creates default images...
//
static image_t* R_FindImageFile_NoLoad(const char* name, const qboolean mipmap, const qboolean allow_picmip,
	const int gl_wrap_clamp_mode)
{
	if (!name)
	{
		return nullptr;
	}

	const char* p_name = GenerateImageMappingName(name);

	//
	// see if the image is already loaded
	//
	const auto it_allocated_image = AllocatedImages.find(p_name);
	if (it_allocated_image != AllocatedImages.end())
	{
		image_t* pImage = (*it_allocated_image).second;

		// the white image can be used with any set of parms, but other mismatches are errors...
		//
		if (strcmp(p_name, "*white") != 0)
		{
			if (pImage->mipmap != !!mipmap)
			{
				//ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: reused image %s with mixed mipmap parm\n", pName );
			}
			if (pImage->allowPicmip != !!allow_picmip)
			{
				//ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: reused image %s with mixed allowPicmip parm\n", pName );
			}
			if (pImage->wrapClampMode != gl_wrap_clamp_mode)
			{
				//ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: reused image %s with mixed glWrapClampMode parm\n", pName );
			}
		}

		pImage->iLastLevelUsedOn = RE_RegisterMedia_GetLevel();

		return pImage;
	}

	return nullptr;
}

/*
================
R_CreateImage

This is the only way any image_t are created
================
*/
image_t* R_CreateImage(const char* name, const byte* pic, const int width, const int height, const GLenum format, const qboolean mipmap, const qboolean allow_picmip, const qboolean allow_tc, int gl_wrap_clamp_mode, const bool b_rectangle)
{
	qboolean is_lightmap = qfalse;

	if (strlen(name) >= MAX_QPATH)
	{
		Com_Error(ERR_DROP, "R_CreateImage: \"%s\" is too long\n", name);
	}

	if (glConfig.clampToEdgeAvailable && gl_wrap_clamp_mode == GL_CLAMP)
	{
		gl_wrap_clamp_mode = GL_CLAMP_TO_EDGE;
	}

	if (name[0] == '*')
	{
		const char* ps_light_map_name_search_pos = strrchr(name, '/');
		if (ps_light_map_name_search_pos && strncmp(ps_light_map_name_search_pos + 1, "lightmap", 8) == 0)
		{
			is_lightmap = qtrue;
		}
	}

	if (width & width - 1 || height & height - 1)
	{
		Com_Error(ERR_FATAL, "R_CreateImage: %s dimensions (%i x %i) not power of 2!\n", name, width, height);
	}

	image_t* image = R_FindImageFile_NoLoad(name, mipmap, allow_picmip, gl_wrap_clamp_mode);
	if (image)
	{
		return image;
	}

	image = static_cast<image_t*>(Z_Malloc(sizeof(image_t), TAG_IMAGE_T, qtrue));
	//	memset(image,0,sizeof(*image));	// qtrue above does this

	image->texnum = 1024 + giTextureBindNum++; // ++ is of course staggeringly important...

	// record which map it was used on...
	//
	image->iLastLevelUsedOn = RE_RegisterMedia_GetLevel();

	image->mipmap = !!mipmap;
	image->allowPicmip = !!allow_picmip;

	Q_strncpyz(image->imgName, name, sizeof image->imgName);

	image->width = width;
	image->height = height;
	image->wrapClampMode = gl_wrap_clamp_mode;

	if (qglActiveTextureARB)
	{
		GL_SelectTexture(0);
	}

	GLuint ui_target = GL_TEXTURE_2D;
	if (b_rectangle)
	{
		qglDisable(ui_target);
		ui_target = GL_TEXTURE_RECTANGLE_ARB;
		qglEnable(ui_target);
		gl_wrap_clamp_mode = GL_CLAMP_TO_EDGE; // default mode supported by rectangle.
		qglBindTexture(ui_target, image->texnum);
	}
	else
	{
		GL_Bind(image);
	}

	Upload32((unsigned*)pic, format,
		static_cast<qboolean>(image->mipmap),
		allow_picmip,
		is_lightmap,
		allow_tc,
		&image->internalFormat,
		&image->width,
		&image->height, b_rectangle);

	qglTexParameterf(ui_target, GL_TEXTURE_WRAP_S, gl_wrap_clamp_mode);
	qglTexParameterf(ui_target, GL_TEXTURE_WRAP_T, gl_wrap_clamp_mode);

	qglBindTexture(ui_target, 0); //jfm: i don't know why this is here, but it breaks lightmaps when there's only 1
	glState.currenttextures[glState.currenttmu] = 0; //mark it not bound

	const char* ps_new_name = GenerateImageMappingName(name);
	Q_strncpyz(image->imgName, ps_new_name, sizeof image->imgName);
	AllocatedImages[image->imgName] = image;

	if (b_rectangle)
	{
		qglDisable(ui_target);
		qglEnable(GL_TEXTURE_2D);
	}

	return image;
}

/*
===============
R_FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/
image_t* R_FindImageFile(const char* name, const qboolean mipmap, const qboolean allow_picmip, const qboolean allow_tc, int gl_wrap_clamp_mode)
{
	int width, height;
	byte* pic;

	if (!name || ri->Cvar_VariableIntegerValue("dedicated"))
		// stop ghoul2 horribleness as regards image loading from server
	{
		return nullptr;
	}

	// need to do this here as well as in R_CreateImage, or R_FindImageFile_NoLoad() may complain about
	//	different clamp parms used...
	//
	if (glConfig.clampToEdgeAvailable && gl_wrap_clamp_mode == GL_CLAMP)
	{
		gl_wrap_clamp_mode = GL_CLAMP_TO_EDGE;
	}

	image_t* image = R_FindImageFile_NoLoad(name, mipmap, allow_picmip, gl_wrap_clamp_mode);
	if (image)
	{
		return image;
	}

	//
	// load the pic from disk
	//
	R_LoadImage(name, &pic, &width, &height);
	if (pic == nullptr)
	{
		// if we dont get a successful load
		return nullptr; // bail
	}

	// refuse to find any files not power of 2 dims...
	//
	if (width & width - 1 || height & height - 1)
	{
		ri->Printf(PRINT_ALL, "Refusing to load non-power-2-dims(%d,%d) pic \"%s\"...\n", width, height, name);
		return nullptr;
	}

	image = R_CreateImage(name, pic, width, height, GL_RGBA, mipmap, allow_picmip, allow_tc, gl_wrap_clamp_mode);
	Z_Free(pic);
	return image;
}

/*
================
R_CreateDlightImage
================
*/
#define	DLIGHT_SIZE	64

static void R_CreateDlightImage()
{
	int width, height;
	byte* pic;

	R_LoadImage("gfx/2d/dlight", &pic, &width, &height);
	if (pic)
	{
		tr.dlightImage = R_CreateImage("*dlight", pic, width, height, GL_RGBA, qfalse, qfalse, qfalse, GL_CLAMP);
		Z_Free(pic);
	}
	else
	{
		// if we dont get a successful load
		byte data[DLIGHT_SIZE][DLIGHT_SIZE][4]{};

		// make a centered inverse-square falloff blob for dynamic lighting
		for (int x = 0; x < DLIGHT_SIZE; x++)
		{
			for (int y = 0; y < DLIGHT_SIZE; y++)
			{
				const float d = (DLIGHT_SIZE / 2 - 0.5f - x) * (DLIGHT_SIZE / 2 - 0.5f - x) +
					(DLIGHT_SIZE / 2 - 0.5f - y) * (DLIGHT_SIZE / 2 - 0.5f - y);
				int b = 4000 / d;
				if (b > 255)
				{
					b = 255;
				}
				else if (b < 75)
				{
					b = 0;
				}
				data[y][x][0] =
					data[y][x][1] =
					data[y][x][2] = b;
				data[y][x][3] = 255;
			}
		}
		tr.dlightImage = R_CreateImage("*dlight", (byte*)data, DLIGHT_SIZE, DLIGHT_SIZE, GL_RGBA, qfalse, qfalse,
			qfalse, GL_CLAMP);
	}
}

/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable()
{
	for (int i = 0; i < FOG_TABLE_SIZE; i++)
	{
		constexpr float exp = 0.5;
		const float d = pow(static_cast<float>(i) / (FOG_TABLE_SIZE - 1), exp);

		tr.fogTable[i] = d;
	}
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float R_FogFactor(float s, const float t)
{
	s -= 1.0 / 512;
	if (s < 0)
	{
		return 0;
	}
	if (t < 1.0 / 32)
	{
		return 0;
	}
	if (t < 31.0 / 32)
	{
		s *= (t - 1.0f / 32.0f) / (30.0f / 32.0f);
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if (s > 1.0)
	{
		s = 1.0;
	}

	const float d = tr.fogTable[static_cast<int>(s * (FOG_TABLE_SIZE - 1))];

	return d;
}

/*
================
R_CreateFogImage
================
*/
#define	FOG_S	256
#define	FOG_T	32

static void R_CreateFogImage()
{
	float border_color[4]{};

	const auto data = static_cast<unsigned char*>(Hunk_AllocateTempMemory(FOG_S * FOG_T * 4));

	// S is distance, T is depth
	for (int x = 0; x < FOG_S; x++)
	{
		for (int y = 0; y < FOG_T; y++)
		{
			const float d = R_FogFactor((x + 0.5f) / FOG_S, (y + 0.5f) / FOG_T);

			data[(y * FOG_S + x) * 4 + 0] =
				data[(y * FOG_S + x) * 4 + 1] =
				data[(y * FOG_S + x) * 4 + 2] = 255;
			data[(y * FOG_S + x) * 4 + 3] = 255 * d;
		}
	}
	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage("*fog", data, FOG_S, FOG_T, GL_RGBA, qfalse, qfalse, qfalse, GL_CLAMP);
	Hunk_FreeTempMemory(data);

	border_color[0] = 1.0;
	border_color[1] = 1.0;
	border_color[2] = 1.0;
	border_color[3] = 1;

	qglTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
}

/*
==================
R_CreateDefaultImage
==================
*/
#define	DEFAULT_SIZE	16

static void R_CreateDefaultImage()
{
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	memset(data, 32, sizeof data);
	for (int x = 0; x < DEFAULT_SIZE; x++)
	{
		data[0][x][0] =
			data[0][x][1] =
			data[0][x][2] =
			data[0][x][3] = 255;

		data[x][0][0] =
			data[x][0][1] =
			data[x][0][2] =
			data[x][0][3] = 255;

		data[DEFAULT_SIZE - 1][x][0] =
			data[DEFAULT_SIZE - 1][x][1] =
			data[DEFAULT_SIZE - 1][x][2] =
			data[DEFAULT_SIZE - 1][x][3] = 255;

		data[x][DEFAULT_SIZE - 1][0] =
			data[x][DEFAULT_SIZE - 1][1] =
			data[x][DEFAULT_SIZE - 1][2] =
			data[x][DEFAULT_SIZE - 1][3] = 255;
	}
	tr.defaultImage = R_CreateImage("*default", (byte*)data, DEFAULT_SIZE, DEFAULT_SIZE, GL_RGBA, qtrue, qfalse, qfalse,
		GL_REPEAT);
}

/*
==================
R_CreateBuiltinImages
==================
*/
static void R_CreateBuiltinImages(void)
{
	int x;
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	memset(data, 255, sizeof data);
	tr.whiteImage = R_CreateImage("*white", (byte*)data, 8, 8, GL_RGBA, qfalse, qfalse, qfalse, GL_REPEAT);

	tr.screenImage = R_CreateImage("*screen", (byte*)data, 8, 8, GL_RGBA, qfalse, qfalse, qfalse, GL_REPEAT);

	// Create the scene glow image. - AReis
	tr.screenGlow = 1024 + giTextureBindNum++;
	qglDisable(GL_TEXTURE_2D);
	qglEnable(GL_TEXTURE_RECTANGLE_ARB);
	qglBindTexture(GL_TEXTURE_RECTANGLE_ARB, tr.screenGlow);
	qglTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA16, glConfig.vidWidth, glConfig.vidHeight, 0, GL_RGB, GL_FLOAT,
		nullptr);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// Create the scene image. - AReis
	tr.sceneImage = 1024 + giTextureBindNum++;
	qglBindTexture(GL_TEXTURE_RECTANGLE_ARB, tr.sceneImage);
	qglTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA16, glConfig.vidWidth, glConfig.vidHeight, 0, GL_RGB, GL_FLOAT,
		nullptr);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// Create the minimized scene blur image.
	if (r_DynamicGlowWidth->integer > glConfig.vidWidth)
	{
		r_DynamicGlowWidth->integer = glConfig.vidWidth;
	}
	if (r_DynamicGlowHeight->integer > glConfig.vidHeight)
	{
		r_DynamicGlowHeight->integer = glConfig.vidHeight;
	}
	tr.blurImage = 1024 + giTextureBindNum++;
	qglBindTexture(GL_TEXTURE_RECTANGLE_ARB, tr.blurImage);
	qglTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA16, r_DynamicGlowWidth->integer, r_DynamicGlowHeight->integer, 0,
		GL_RGB, GL_FLOAT, nullptr);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
	qglDisable(GL_TEXTURE_RECTANGLE_ARB);

	if (glConfigExt.doGammaCorrectionWithShaders)
	{
		qglEnable(GL_TEXTURE_3D);
		tr.gammaCorrectLUTImage = 1024 + giTextureBindNum++;
		qglBindTexture(GL_TEXTURE_3D, tr.gammaCorrectLUTImage);
		qglTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 64, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		qglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		qglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		qglDisable(GL_TEXTURE_3D);
	}

	qglEnable(GL_TEXTURE_2D);

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for (x = 0; x < DEFAULT_SIZE; x++)
	{
		for (int y = 0; y < DEFAULT_SIZE; y++)
		{
			data[y][x][0] =
				data[y][x][1] =
				data[y][x][2] = tr.identityLightByte;
			data[y][x][3] = 255;
		}
	}

	tr.identityLightImage = R_CreateImage("*identityLight", (byte*)data, 8, 8, GL_RGBA, qfalse, qfalse, qfalse,
		GL_REPEAT);

	for (x = 0; x < NUM_SCRATCH_IMAGES; x++)
	{
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage(va("*scratch%d", x), (byte*)data, DEFAULT_SIZE, DEFAULT_SIZE, GL_RGBA,
			qfalse, qtrue, qfalse, GL_CLAMP);
	}

	R_CreateDlightImage();

	R_CreateFogImage();
}

/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings()
{
	int i;

	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;
	if (!glConfig.deviceSupportsGamma && !glConfigExt.doGammaCorrectionWithShaders)
	{
		tr.overbrightBits = 0; // need hardware gamma for overbright
	}

	// never overbright in windowed mode
	if (!glConfig.isFullscreen)
	{
		tr.overbrightBits = 0;
	}

	if (tr.overbrightBits > 1)
	{
		tr.overbrightBits = 1;
	}

	if (tr.overbrightBits < 0)
	{
		tr.overbrightBits = 0;
	}

	tr.identityLight = 1.0f / (1 << tr.overbrightBits);
	tr.identityLightByte = 255 * tr.identityLight;

	if (r_intensity->value < 1.0f)
	{
		ri->Cvar_Set("r_intensity", "1");
	}

	if (r_gamma->value < 0.5f)
	{
		ri->Cvar_Set("r_gamma", "0.5");
	}
	else if (r_gamma->value > 3.0f)
	{
		ri->Cvar_Set("r_gamma", "3.0");
	}

	const float g = r_gamma->value;

	const int shift = tr.overbrightBits;

	if (!glConfigExt.doGammaCorrectionWithShaders)
	{
		int inf;
		for (i = 0; i < 256; i++)
		{
			if (g == 1)
			{
				inf = i;
			}
			else
			{
				inf = 255 * pow(i / 255.0f, 1.0f / g) + 0.5f;
			}
			inf <<= shift;
			if (inf < 0)
			{
				inf = 0;
			}
			if (inf > 255)
			{
				inf = 255;
			}
			s_gammatable[i] = inf;
		}

		if (glConfig.deviceSupportsGamma)
		{
			ri->WIN_SetGamma(&glConfig, s_gammatable, s_gammatable, s_gammatable);
		}
	}

	for (i = 0; i < 256; i++)
	{
		int j = i * r_intensity->value;
		if (j > 255)
		{
			j = 255;
		}
		s_intensitytable[i] = j;
	}
}

void R_SetGammaCorrectionLUT()
{
	if (glConfigExt.doGammaCorrectionWithShaders)
	{
		int inf;
		const int shift = tr.overbrightBits;
		const float g = r_gamma->value;
		byte gamma_corrected[64]{};

		for (int i = 0; i < 64; i++)
		{
			if (g == 1.0f)
			{
				inf = static_cast<int>(static_cast<float>(i) / 63.0f * 255.0f + 0.5f);
			}
			else
			{
				inf = static_cast<int>(255.0f * pow(static_cast<float>(i) / 63.0f, 1.0f / g) + 0.5f);
			}

			gamma_corrected[i] = Com_Clampi(0, 255, inf << shift);
		}

		const auto lut_table = static_cast<byte*>(ri->Hunk_AllocateTempMemory(64 * 64 * 64 * 3));
		byte* write = lut_table;
		for (const unsigned char z : gamma_corrected)
		{
			for (const unsigned char y : gamma_corrected)
			{
				for (const unsigned char x : gamma_corrected)
				{
					*write++ = x;
					*write++ = y;
					*write++ = z;
				}
			}
		}

		qglBindTexture(GL_TEXTURE_3D, tr.gammaCorrectLUTImage);
		qglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		qglTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 64, 64, 64, GL_RGB, GL_UNSIGNED_BYTE, lut_table);

		ri->Hunk_FreeTempMemory(lut_table);
	}
}

/*
===============
R_InitImages
===============
*/
void R_InitImages()
{
	//memset(hashTable, 0, sizeof(hashTable));	// DO NOT DO THIS NOW (because of image cacheing)	-ste.
	// build brightness translation tables
	R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();

	R_SetGammaCorrectionLUT();
}

/*
===============
R_DeleteTextures
===============
*/
// (only gets called during vid_restart now (and app exit), not during map load)
//
void R_DeleteTextures()
{
	R_Images_Clear();
	GL_ResetBinds();
}