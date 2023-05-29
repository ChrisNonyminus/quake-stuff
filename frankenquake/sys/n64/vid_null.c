/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// vid_null.c -- null video driver to aid porting efforts

// based on https://github.com/ahefner/sdlquake/blob/master/vid_sdl.c

#include <SDL.h>
#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>

#include "quakedef.h"

extern viddef_t vid; // global video state

#define BASEWIDTH 320
#define BASEHEIGHT 240
int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
//int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;
int scr_width = BASEWIDTH, scr_height = BASEHEIGHT;

extern short *d_pzbuffer;

float		gldepthmin, gldepthmax;
surface_t zbuffer;

byte *surfcache;

unsigned short d_8to16table[256];
unsigned d_8to24table[256];
unsigned char d_15to8table[65536];
int		texture_extension_number = 0;
qboolean is8bit = false;
qboolean isPermedia = false;
qboolean gl_mtexable = false;


const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

cvar_t	gl_ztrick = {"gl_ztrick","1"};


/*
=================
GL_BeginRendering
=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	extern cvar_t gl_clear;

	*x = *y = 0;
	*width = scr_width;
	*height = scr_height;

    surface_t *disp = display_get();
    rdpq_attach(disp, &zbuffer);

    gl_context_begin();

//    if (!wglMakeCurrent( maindc, baseRC ))
//		Sys_Error ("wglMakeCurrent failed");

//	glViewport (*x, *y, *width, *height);
glViewport (*x, *y, *width, *height);
}


void GL_EndRendering (void)
{
	glFlush();
    gl_context_end();

    rdpq_detach_show();
}

 uint16_t libdragon_palette[256];

void VID_SetPalette(unsigned char *palette)
{
    int i;
    SDL_Color colors[256];

    for (i = 0; i < 256; ++i)
    {
        colors[i].r = *palette++;
        colors[i].g = *palette++;
        colors[i].b = *palette++;

		libdragon_palette[i] = color_to_packed16(RGBA32(colors[i].r, colors[i].g, colors[i].b, 0));
    }
}

void VID_ShiftPalette(unsigned char *palette) { VID_SetPalette(palette); }


/*
===============
GL_Init
===============
*/
void GL_Init (void)
{
    rdpq_init();
    rdpq_debug_start();
    rdpq_debug_log(true);
    gl_init();

	gl_vendor = (const char*)glGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = (const char*)glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = (const char*)glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = (const char*)glGetString (GL_EXTENSIONS);
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

//	Con_Printf ("%s %s\n", gl_renderer, gl_version);

	//CheckMultiTextureExtensions ();

	glClearColor (1,0,0,0);
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_FLAT);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    zbuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());
}
void VID_Init(unsigned char *palette)
{
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    
	Cvar_RegisterVariable (&gl_ztrick);


	vid.maxwarpwidth = 320;
	vid.maxwarpheight = 200;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
    vid.rowbytes = vid.width = vid.conwidth = BASEWIDTH;
    vid.height = vid.conheight = BASEHEIGHT;
    vid.aspect = ((float)vid.height / (float)vid.width) *
				(320.0 / 240.0);
	vid.numpages = 2;
    GL_Init();
	vid.recalc_refdef = 1;				// force a surface cache flush
}

void VID_Shutdown(void)
{
    
}
void Hunk_Print(qboolean all);
void VID_Update(vrect_t *rects)
{

}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect(int x, int y, byte *pbitmap, int width, int height)
{
    
}

/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect(int x, int y, int width, int height)
{
    
}