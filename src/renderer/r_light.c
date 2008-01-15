/**
 * @file r_light.c
 */

/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "r_local.h"
#include "r_error.h"
#include "r_light.h"

static int r_numLights;
static light_t r_lights[MAX_GL_LIGHTS];

void R_AddLight (vec3_t origin, float intensity, vec3_t color)
{
	int i;

	if (!r_light->integer)
		return;

	if (r_numLights == MAX_GL_LIGHTS)
		return;

	i = r_numLights++;

	VectorCopy(origin, r_lights[i].origin);
	r_lights[i].intensity = intensity;
	VectorCopy(color, r_lights[i].color);
}

/**
 * @brief Set light parameters
 * @sa R_RenderFrame
 */
void R_AddLights (void)
{
	vec4_t position;
	vec3_t diffuse;
	int i;

	if (!r_light->integer)
		return;

	for (i = 0; i < r_numLights; i++) {  /* now add all lights */
		VectorCopy(r_lights[i].origin, position);
		position[3] = 1;

		VectorScale(r_lights[i].color, r_lights[i].intensity, diffuse);

		qglLightfv(GL_LIGHT0 + i, GL_POSITION, position);
		qglLightfv(GL_LIGHT0 + i, GL_DIFFUSE, diffuse);
		qglEnable(GL_LIGHT0 + i);
	}

	for (; i < MAX_GL_LIGHTS; i++)
		qglDisable(GL_LIGHT0 + i);

	R_CheckError();

	r_numLights = 0;
}
