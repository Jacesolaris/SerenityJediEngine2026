/*
===========================================================================
Copyright (C) 2016, OpenJK contributors

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
#include "tr_local.h"

void R_PushDebugGroup(annotationLayer_t layer, const char* name)
{
	// Track current debug layer
	static GLuint currentLayer = (GLuint)AL_NONE;

	// ----------------------------------------------------------------------
	// Safety: validate layer ordering
	// ----------------------------------------------------------------------
	if (layer > currentLayer + 1)
	{
		// Replace assert with debug warning (rule #9)
		ri->Printf(
			PRINT_WARNING,
			"R_PushDebugGroup WARNING: invalid layer order (requested %u, current %u)\n",
			(unsigned int)layer,
			(unsigned int)currentLayer
		);

		// Behaviour-preserving fallback:
		// Clamp layer to currentLayer + 1 so the renderer does not crash.
		layer = (annotationLayer_t)(currentLayer + 1);
	}

	// ----------------------------------------------------------------------
	// Pop groups until we reach the requested layer
	// ----------------------------------------------------------------------
	while (layer <= currentLayer)
	{
		if (currentLayer == AL_NONE)
		{
			break;
		}

		qglPopDebugGroupKHR();
		currentLayer--;
	}

	// ----------------------------------------------------------------------
	// If layer is AL_NONE, nothing to push
	// ----------------------------------------------------------------------
	if (layer == AL_NONE)
	{
		return;
	}

	// ----------------------------------------------------------------------
	// Push new debug group
	// ----------------------------------------------------------------------
	currentLayer = (GLuint)layer;
	qglPushDebugGroupKHR(GL_DEBUG_SOURCE_APPLICATION, currentLayer, -1, name);
}