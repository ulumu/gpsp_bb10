/*
 *
 * This file is based on Portable ZX-Spectrum emulator.
 * Copyright (C) 2001-2012 SMT, Dexus, Alone Coder, deathsoft, djdron, scor
 *
 * C++ to C code conversion by Pate
 *
 * Modified by DPR for gpsp for Raspberry Pi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

//#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES2/gl2.h"
#include <GLES2/gl2ext.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "SDL.h"

static uint32_t frame_width = 0;
static uint32_t frame_height = 0;
static GLint    texFilter = GL_NEAREST;

// Declare in config.c
extern int filterType;

#define	SHOW_ERROR		gles_show_error();

const char *v_shader =
		"attribute vec2 a_position;\n"
		"attribute vec2 a_texcoord;\n"
		"varying vec2 v_texcoord;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = vec4(a_position, 0.0, 1.0);\n"
		"    v_texcoord = a_texcoord;\n"
		"}\n";

const char *fs_basic =
		"uniform lowp sampler2D u_sampler;\n"
		"uniform lowp vec2      u_resolution;\n"
		"varying mediump vec2   v_texcoord;\n"
		"void main()\n"
		"{\n"
		"    gl_FragColor = texture2D(u_sampler, v_texcoord);\n"
		"}\n";

const char *fs_fxaa =
    "uniform lowp sampler2D u_sampler; // Texture0\n"
	"uniform lowp    vec2  u_resolution;\n"
    "varying mediump vec2  v_texcoord;\n"
    "\n"
    "#define FxaaInt2 ivec2\n"
    "#define FxaaFloat2 vec2\n"
    "#define FxaaTexLod0(t, p) texture2D(t, p)\n"
    "#define FxaaTexOff(t, p, o, r) texture2D(t, p + (o * r))\n"
	"#define FXAA_REDUCE_MIN   (1.0/128.0)\n"
	"#define FXAA_REDUCE_MUL   (1.0/16.0)\n"
	"#define FXAA_SPAN_MAX     2.0\n"
    "\n"
	"void main() \n"
	"{ \n"
	"    lowp vec3 c;\n"
	"    lowp float lum;"
	"    mediump vec2 rcpFrame = 1.0/u_resolution;\n"
    "/*---------------------------------------------------------*/\n"
    "/*---------------------------------------------------------*/\n"
    "    lowp vec3 rgbNW = texture2D(u_sampler, v_texcoord + (FxaaFloat2(-1.0,-1.0) * rcpFrame.xy)).xyz;\n"
    "    lowp vec3 rgbNE = texture2D(u_sampler, v_texcoord + (FxaaFloat2( 1.0,-1.0) * rcpFrame.xy)).xyz;\n"
    "    lowp vec3 rgbSW = texture2D(u_sampler, v_texcoord + (FxaaFloat2(-1.0, 1.0) * rcpFrame.xy)).xyz;\n"
    "    lowp vec3 rgbSE = texture2D(u_sampler, v_texcoord + (FxaaFloat2( 1.0, 1.0) * rcpFrame.xy)).xyz;\n"
    "    lowp vec3 rgbM  = texture2D(u_sampler, v_texcoord).xyz;\n"
    "/*---------------------------------------------------------*/\n"
    "    lowp vec3 luma = vec3(0.299, 0.587, 0.114);\n"
    "    lowp float lumaNW = dot(rgbNW, luma);\n"
    "    lowp float lumaNE = dot(rgbNE, luma);\n"
    "    lowp float lumaSW = dot(rgbSW, luma);\n"
    "    lowp float lumaSE = dot(rgbSE, luma);\n"
    "    lowp float lumaM  = dot(rgbM,  luma);\n"
    "/*---------------------------------------------------------*/\n"
    "    lowp float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));\n"
    "    lowp float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));\n"
    "/*---------------------------------------------------------*/\n"
    "    lowp vec2 dir; \n"
    "    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));\n"
    "    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));\n"
    "/*---------------------------------------------------------*/\n"
    "    lowp float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);\n"
    "    lowp float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);\n"
    "    dir = min(FxaaFloat2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX), \n"
    "          max(FxaaFloat2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), \n"
    "          dir * rcpDirMin)) * rcpFrame.xy;\n"
    "/*--------------------------------------------------------*/\n"
    "    lowp vec3 rgbA = (1.0/2.0) * (\n"
    "        texture2D(u_sampler, v_texcoord.xy + dir * (1.0/3.0 - 0.5)).xyz +\n"
    "        texture2D(u_sampler, v_texcoord.xy + dir * (2.0/3.0 - 0.5)).xyz);\n"
    "    lowp vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (\n"
    "        texture2D(u_sampler, v_texcoord.xy + dir * (0.0/3.0 - 0.5)).xyz +\n"
    "        texture2D(u_sampler, v_texcoord.xy + dir * (3.0/3.0 - 0.5)).xyz);\n"
    "    lowp float lumaB = dot(rgbB, luma);\n"
    "    if((lumaB < lumaMin) || (lumaB > lumaMax))\n"
	"      c = rgbA;\n"
	"    else\n"
    "      c = rgbB;\n"
	"    gl_FragColor = vec4(c,1.0);\n"
	"}\n";

static GLfloat vertices[]  = {-1.0,-1.0, 1.0,-1.0, -1.0,1.0, 1.0,1.0};
static GLfloat texCoords[] = {0.0,1.0, 1.0,1.0, 0.0,0.0, 1.0,0.0};
static GLuint  textures[2];

typedef	struct ShaderInfo {
		GLuint program;
		GLint a_position;
		GLint a_texcoord;
		GLint u_texture;
		GLint u_resolution;
} ShaderInfo;

static ShaderInfo shader;


void gles_show_error()
{
	GLenum error = GL_NO_ERROR;
    error = glGetError();
    if (GL_NO_ERROR != error)
        SLOG("GL Error %x encountered!\n", error);
}

static GLuint CreateShader(GLenum type, const char *shader_src)
{
	GLuint shader = glCreateShader(type);
	if(!shader)
		return 0;

	// Load and compile the shader source
	glShaderSource(shader, 1, &shader_src, NULL);
	glCompileShader(shader);

	// Check the compile status
	GLint compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(!compiled)
	{
		GLint info_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
		if(info_len > 1)
		{
			char* info_log = (char *)malloc(sizeof(char) * info_len);
			glGetShaderInfoLog(shader, info_len, NULL, info_log);
			// TODO(dspringer): We could really use a logging API.
			SLOG("Error compiling shader:\n%s\n", info_log);
			free(info_log);
		}
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

static GLuint CreateProgram(const char *vertex_shader_src, const char *fragment_shader_src)
{
	GLuint vertex_shader = CreateShader(GL_VERTEX_SHADER, vertex_shader_src);
	if(!vertex_shader)
		return 0;

	GLuint fragment_shader = CreateShader(GL_FRAGMENT_SHADER, fragment_shader_src);
	if(!fragment_shader)
	{
		glDeleteShader(vertex_shader);
		return 0;
	}

	GLuint program_object = glCreateProgram();
	if(!program_object)
		return 0;
	glAttachShader(program_object, vertex_shader);
	glAttachShader(program_object, fragment_shader);

	// Link the program
	glLinkProgram(program_object);

	// Check the link status
	GLint linked = 0;
	glGetProgramiv(program_object, GL_LINK_STATUS, &linked);
	if(!linked)
	{
		GLint info_len = 0;
		glGetProgramiv(program_object, GL_INFO_LOG_LENGTH, &info_len);
		if(info_len > 1)
		{
			char* info_log = (char *)malloc(info_len);
			glGetProgramInfoLog(program_object, info_len, NULL, info_log);
			// TODO(dspringer): We could really use a logging API.
			SLOG("Error linking program:\n%s\n", info_log);
			free(info_log);
		}
		glDeleteProgram(program_object);
		return 0;
	}
	// Delete these here because they are attached to the program object.
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return program_object;
}



static void gles2_create()
{
	memset(&shader, 0, sizeof(ShaderInfo));
#ifdef STL100_1
	shader.program = CreateProgram(v_shader, fs_basic);
	texFilter = GL_NEAREST;
#else
	if (filterType == 0) {
	    shader.program = CreateProgram(v_shader, fs_basic);
	    texFilter      = GL_NEAREST;
	} else {
	    shader.program = CreateProgram(v_shader, fs_fxaa);
	    texFilter      = GL_LINEAR;
	}
#endif
	if(shader.program)
	{
		glUseProgram(shader.program);
		shader.a_position	= glGetAttribLocation(shader.program,	"a_position");
		shader.a_texcoord	= glGetAttribLocation(shader.program,	"a_texcoord");
		shader.u_texture	= glGetUniformLocation(shader.program,	"u_sampler");
		shader.u_resolution = glGetUniformLocation(shader.program,  "u_resolution");

		// screen texture is TEXTURE0
		glUniform1i(shader.u_texture, 0);
		glUniform2f(shader.u_resolution, 480.0f, 360.0f);
	}

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texFilter);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texFilter);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DITHER);
}

static uint32_t screen_width = 0;
static uint32_t screen_height = 0;

static void video_set_filter(uint32_t filter) {
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texFilter);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texFilter);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

static void gles2_destroy()
{
	if(!shader.program)
		return;
	glDeleteProgram(shader.program); SHOW_ERROR
}

#ifdef GBA_USE_RGBA8888
static void gles2_Draw( uint32_t *pixels)
#else
static void gles2_Draw( uint16_t *pixels)
#endif
{
	if(!shader.program)
		return;

	glClear(GL_COLOR_BUFFER_BIT );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
#if 0 //def GBA_USE_RGBA8888
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, frame_width, frame_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame_width, frame_height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);
#endif

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnableVertexAttribArray(shader.a_position);
	glEnableVertexAttribArray(shader.a_texcoord);
	glVertexAttribPointer(shader.a_position, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), vertices);
	glVertexAttribPointer(shader.a_texcoord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), texCoords);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void video_init(uint32_t _width, uint32_t _height, uint32_t filter)
{
	if ((_width==0)||(_height==0))
		return;

	frame_width  = _width;
	frame_height = _height;

	screen_height = 720;
	screen_width  = 1280;

	gles2_create();

	video_set_filter(filter);
}


void video_close()
{
	gles2_destroy();
}

#ifdef GBA_USE_RGBA8888
void video_draw(uint32_t *pixels)
#else
void video_draw(uint16_t *pixels)
#endif
{
	gles2_Draw (pixels);

}

