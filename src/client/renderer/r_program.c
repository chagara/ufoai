/**
 * @file r_program.c
 * @brief Shader (GLSL) backend functions
 */

/*
* Copyright(c) 1997-2001 Id Software, Inc.
* Copyright(c) 2002 The Quakeforge Project.
* Copyright(c) 2006 Quake2World.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or(at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "r_local.h"
#include "r_error.h"
#include "../../shared/parse.h"

void R_UseProgram  (r_program_t *prog)
{
	if (!qglUseProgram || r_state.active_program == prog)
		return;

	r_state.active_program = prog;

	if (prog) {
		qglUseProgram(prog->id);

		if (prog->use)  /* invoke use function */
			prog->use();
	} else {
		qglUseProgram(0);
	}
}

static r_progvar_t *R_ProgramVariable (int type, const char *name)
{
	r_progvar_t *v;
	int i;

	if (!r_state.active_program) {
		Com_Printf("R_ProgramVariable: No program bound.\n");
		return NULL;
	}

	/* find the variable */
	for (i = 0; i < MAX_PROGRAM_VARS; i++) {
		v = &r_state.active_program->vars[i];

		if (!v->location)
			break;

		if (v->type == type && !strcmp(v->name, name))
			return v;
	}

	if (i == MAX_PROGRAM_VARS) {
		Com_Printf("R_ProgramVariable: MAX_PROGRAM_VARS reached.\n");
		return NULL;
	}

	/* or query for it */
	if (type == GL_UNIFORM)
		v->location = qglGetUniformLocation(r_state.active_program->id, name);
	else
		v->location = qglGetAttribLocation(r_state.active_program->id, name);

	if (v->location == -1) {
		Com_Printf("R_ProgramVariable: Could not find %s in program %s\n",
			name, r_state.active_program->name);
		v->location = 0;
		return NULL;
	}

	v->type = type;
	Q_strncpyz(v->name, name, sizeof(v->name));

	return v;
}

void R_ProgramParameter1i (const char *name, GLint value)
{
	r_progvar_t *v;

	if (!(v = R_ProgramVariable(GL_UNIFORM, name)))
		return;

	qglUniform1i(v->location, value);
}

void R_ProgramParameter1f (const char *name, GLfloat value)
{
	r_progvar_t *v;

	if (!(v = R_ProgramVariable(GL_UNIFORM, name)))
		return;

	qglUniform1f(v->location, value);
}

void R_ProgramParameter3fv (const char *name, GLfloat *value)
{
	r_progvar_t *v;

	if (!(v = R_ProgramVariable(GL_UNIFORM, name)))
		return;

	qglUniform3fv(v->location, 1, value);
}

void R_ProgramParameter4fv (const char *name, GLfloat *value)
{
	r_progvar_t *v;

	if (!(v = R_ProgramVariable(GL_UNIFORM, name)))
		return;

	qglUniform4fv(v->location, 1, value);
}

void R_AttributePointer (const char *name, GLuint size, const GLvoid *array)
{
	r_progvar_t *v;

	if (!(v = R_ProgramVariable(GL_ATTRIBUTE, name)))
		return;

	qglVertexAttribPointer(v->location, size, GL_FLOAT, GL_FALSE, 0, array);
}

void R_EnableAttribute (const char *name)
{
	r_progvar_t *v;

	if (!(v = R_ProgramVariable(GL_ATTRIBUTE, name)))
		return;

	qglEnableVertexAttribArray(v->location);
}

void R_DisableAttribute (const char *name)
{
	r_progvar_t *v;

	if (!(v = R_ProgramVariable(GL_ATTRIBUTE, name)))
		return;

	qglDisableVertexAttribArray(v->location);
}

static void R_ShutdownShader (r_shader_t *sh)
{
	qglDeleteShader(sh->id);
	memset(sh, 0, sizeof(*sh));
}

static void R_ShutdownProgram (r_program_t *prog)
{
	if (prog->v) {
		qglDetachShader(prog->id, prog->v->id);
		R_ShutdownShader(prog->v);
		R_CheckError();
	}
	if (prog->f) {
		qglDetachShader(prog->id, prog->f->id);
		R_ShutdownShader(prog->f);
		R_CheckError();
	}

	qglDeleteProgram(prog->id);

	memset(prog, 0, sizeof(r_program_t));
}

void R_ShutdownPrograms (void)
{
	int i;

	if (!qglDeleteProgram)
		return;

	if (!r_programs->integer)
		return;

	for (i = 0; i < MAX_PROGRAMS; i++) {
		if (!r_state.programs[i].id)
			continue;

		R_ShutdownProgram(&r_state.programs[i]);
	}
}

static size_t R_PreprocessShader (const char *name, const char *in, char *out, size_t len)
{
	char path[MAX_QPATH];
	byte *buf;
	int i;
	const char *hwHack;

	switch (r_config.hardwareType) {
	case GLHW_ATI:
		hwHack = "#ifndef ATI\n#define ATI\n#endif\n";
		break;
	case GLHW_INTEL:
		hwHack = "#ifndef INTEL\n#define INTEL\n#endif\n";
		break;
	case GLHW_NVIDIA:
		hwHack = "#ifndef NVIDIA\n#define NVIDIA\n#endif\n";
		break;
	case GLHW_GENERIC:
		hwHack = NULL;
		break;
	default:
		Com_Error(ERR_FATAL, "R_PreprocessShader: Unknown hardwaretype");
	}

	i = 0;

	if (hwHack) {
		size_t hwHackLength = strlen(hwHack);
		strcpy(out, hwHack);
		out += hwHackLength;
		len -= hwHackLength;
		if (len < 0)
			Com_Error(ERR_FATAL, "overflow in shader loading '%s'", name);
		i += hwHackLength;
	}

	while (*in) {
		if (!strncmp(in, "#include", 8)) {
			size_t inc_len;
			in += 8;
			Com_sprintf(path, sizeof(path), "shaders/%s", Com_Parse(&in));

			if (FS_LoadFile(path, &buf) == -1) {
				Com_Printf("Failed to resolve #include: %s.\n", path);
				continue;
			}

			inc_len = R_PreprocessShader(name,  (const char *)buf, out, len);
			len -= inc_len;
			out += inc_len;
			FS_FreeFile(buf);
		}

		if (!strncmp(in, "#if", 3)) {  /* conditionals */
			float f;

			in += 3;

			f = Cvar_GetValue(Com_Parse(&in));

			while (*in) {
				if (!strncmp(in, "#endif", 6)) {
					in += 6;
					break;
				}

				len--;
				if (len < 0) {
					Com_Error(ERR_DROP, "R_PreprocessShader: "
							"Overflow: %s", name);
				}

				if (f) {
					*out++ = *in++;
					i++;
				} else
					in++;
			}

			if (!*in) {
				Com_Error(ERR_DROP, "R_PreprocessShader: "
						"Unterminated conditional: %s", name);
			}
		}

		/* general case is to copy so long as the buffer has room */

		len--;
		if (len < 0)
			Com_Error(ERR_FATAL, "R_PreprocessShader: Overflow in shader loading '%s'", name);
		*out++ = *in++;
		i++;
	}
	return i;
}

#define SHADER_BUF_SIZE 16384

static r_shader_t *R_LoadShader (GLenum type, const char *name)
{
	r_shader_t *sh;
	char path[MAX_QPATH], *src[1];
	unsigned e, len, length[1];
	char *source;
	byte *buf;
	int i;

	snprintf(path, sizeof(path), "shaders/%s", name);

	if ((len = FS_LoadFile(path, &buf)) == -1) {
		Com_DPrintf(DEBUG_RENDERER, "R_LoadShader: Failed to load %s.\n", name);
		return NULL;
	}

	source = Mem_PoolAlloc(SHADER_BUF_SIZE, vid_imagePool, 0);

	R_PreprocessShader(name, (const char *)buf, source, SHADER_BUF_SIZE);
	FS_FreeFile(buf);

	src[0] = source;
	length[0] = strlen(source);

	for (i = 0; i < MAX_SHADERS; i++) {
		sh = &r_state.shaders[i];

		if (!sh->id)
			break;
	}

	if (i == MAX_SHADERS) {
		Com_Printf("R_LoadShader: MAX_SHADERS reached.\n");
		Mem_Free(source);
		return NULL;
	}

	strncpy(sh->name, name, sizeof(sh->name));

	sh->type = type;

	sh->id = qglCreateShader(sh->type);
	if (!sh->id)
		return NULL;

	/* upload the shader source */
	qglShaderSource(sh->id, 1, src, length);

	/* compile it and check for errors */
	qglCompileShader(sh->id);

	qglGetShaderiv(sh->id, GL_COMPILE_STATUS, &e);
	if (!e) {
		char log[MAX_STRING_CHARS];
		qglGetShaderInfoLog(sh->id, sizeof(log) - 1, NULL, log);
		Com_Printf("R_LoadShader: %s: %s\n", sh->name, log);

		qglDeleteShader(sh->id);
		memset(sh, 0, sizeof(*sh));

		Mem_Free(source);
		return NULL;
	}

	Mem_Free(source);
	return sh;
}

static r_program_t *R_LoadProgram (const char *name, void *init, void *use)
{
	r_program_t *prog;
	unsigned e;
	int i;

	for (i = 0; i < MAX_PROGRAMS; i++) {
		prog = &r_state.programs[i];

		if (!prog->id)
			break;
	}

	if (i == MAX_PROGRAMS) {
		Com_Printf("R_LoadProgram: MAX_PROGRAMS reached.\n");
		return NULL;
	}

	Q_strncpyz(prog->name, name, sizeof(prog->name));

	prog->id = qglCreateProgram();

	prog->v = R_LoadShader(GL_VERTEX_SHADER, va("%s_vs.glsl", name));
	prog->f = R_LoadShader(GL_FRAGMENT_SHADER, va("%s_fs.glsl", name));

	if (prog->v)
		qglAttachShader(prog->id, prog->v->id);
	if (prog->f)
		qglAttachShader(prog->id, prog->f->id);

	qglLinkProgram(prog->id);

	qglGetProgramiv(prog->id, GL_LINK_STATUS, &e);
	if (!e || !prog->v || !prog->f) {
		char log[MAX_STRING_CHARS];
		qglGetProgramInfoLog(prog->id, sizeof(log) - 1, NULL, log);
		Com_Printf("R_LoadProgram: %s: %s\n", prog->name, log);

		R_ShutdownProgram(prog);
		return NULL;
	}

	prog->init = init;

	if (prog->init) {  /* invoke initialization function */
		R_UseProgram(prog);

		prog->init();

		R_UseProgram(NULL);
	}

	prog->use = use;

	Com_Printf("R_LoadProgram: '%s' loaded.\n", name);

	return prog;
}

static void R_InitWorldProgram (void)
{
	R_ProgramParameter1i("SAMPLER0", 0);
	R_ProgramParameter1i("SAMPLER1", 1);
	R_ProgramParameter1i("SAMPLER2", 2);
	R_ProgramParameter1i("SAMPLER3", 3);

	R_ProgramParameter1i("BUMPMAP", 0);

	R_ProgramParameter1f("BUMP", 1.0);
	R_ProgramParameter1f("PARALLAX", 1.0);
	R_ProgramParameter1f("HARDNESS", 0.2);
	R_ProgramParameter1f("SPECULAR", 1.0);
}

static void R_InitMeshProgram (void)
{
	static vec3_t lightPos;

	R_ProgramParameter1i("SAMPLER0", 0);

	R_ProgramParameter3fv("LIGHTPOS", lightPos);

	R_ProgramParameter1f("OFFSET", 0.0);
}

static void R_InitWarpProgram (void)
{
	static vec4_t offset;

	R_ProgramParameter1i("SAMPLER0", 0);
	R_ProgramParameter1i("SAMPLER1", 1);

	R_ProgramParameter4fv("OFFSET", offset);
}

static void R_UseWarpProgram (void)
{
	static vec4_t offset;

	offset[0] = offset[1] = refdef.time / 8.0;
	R_ProgramParameter4fv("OFFSET", offset);
}

void R_InitPrograms (void)
{
	if (!qglCreateProgram)
		return;

	memset(r_state.shaders, 0, sizeof(r_state.shaders));
	memset(r_state.programs, 0, sizeof(r_state.programs));

	/* shaders are deactivated - don't try to load them - some cards
	 * even have problems with this */
	if (!r_programs->integer)
		return;

	r_state.world_program = R_LoadProgram("world", R_InitWorldProgram, NULL);
	r_state.mesh_program = R_LoadProgram("mesh", R_InitMeshProgram, NULL);
	r_state.warp_program = R_LoadProgram("warp", R_InitWarpProgram, R_UseWarpProgram);
}

/**
 * @brief Reloads the glsl shaders
 */
void R_RestartPrograms_f (void)
{
	R_ShutdownPrograms();
	R_InitPrograms();
}
