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

#include "quakedef.h"

extern viddef_t vid; // global video state

#define BASEWIDTH 320
#define BASEHEIGHT 240

static SDL_Surface *sdlscreen = NULL;
static SDL_Surface *sdlblit = NULL;
static SDL_Renderer *sdlrenderer = NULL;
static SDL_Window *sdlwindow = NULL;

extern short			*d_pzbuffer;

short zbuffer[BASEWIDTH*BASEHEIGHT];

byte* surfcache;

unsigned short d_8to16table[256];
unsigned d_8to24table[256];

void VID_SetPalette(unsigned char *palette)
{
    int i;
    SDL_Color colors[256];

    for (i = 0; i < 256; ++i)
    {
        colors[i].r = *palette++;
        colors[i].g = *palette++;
        colors[i].b = *palette++;
    }
    SDL_SetPaletteColors(sdlblit->format->palette, colors, 0, 256);
}

void VID_ShiftPalette(unsigned char *palette) { VID_SetPalette(palette); }

void VID_Init(unsigned char *palette)
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
        Sys_Error("Error initializing SDL_VIDEO: %s\n", SDL_GetError());
    }

    if (!(sdlwindow = SDL_CreateWindow("QUAKE", SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED, BASEWIDTH,
                                       BASEHEIGHT, 0)) ||
                      !(sdlrenderer = SDL_CreateRenderer(sdlwindow, -1, 0)))
    {
        Sys_Error("Error in SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
    }
	SDL_RendererInfo rendererInfo;
	if (!SDL_GetRendererInfo(sdlrenderer, &rendererInfo))
		printf("Using SDL renderer: %s\n", rendererInfo.name);
    sdlscreen = SDL_GetWindowSurface(sdlwindow);
    vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
    vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
    vid.aspect = vid.width / vid.height;
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int *)vid.colormap + 2048));

    sdlblit = SDL_CreateRGBSurfaceWithFormat(0, vid.width, vid.height, 8,
                                             SDL_PIXELFORMAT_INDEX8);
    vid.buffer = vid.conbuffer = sdlblit->pixels;
    vid.rowbytes = vid.conrowbytes = BASEWIDTH;

    d_pzbuffer = zbuffer;
	surfcache = malloc(D_SurfaceCacheForRes(vid.width, vid.height));
    D_InitCaches(surfcache, D_SurfaceCacheForRes(vid.width, vid.height));
}

void VID_Shutdown(void)
{
    SDL_DestroyRenderer(sdlrenderer);
    SDL_DestroyWindow(sdlwindow);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
void Hunk_Print (qboolean all);
static SDL_Texture* sdltexture = NULL;
void VID_Update(vrect_t *rects)
{
    SDL_Rect *sdlrects;
    int n, i;
    vrect_t *rect;

    // Two-pass system, since Quake doesn't do it the SDL way...

    // First, count the number of rectangles
    n = 0;
    for (rect = rects; rect; rect = rect->pnext)
        ++n;

    // Second, copy them to SDL rectangles and update
    if (!(sdlrects = (SDL_Rect *)alloca(n * sizeof(*sdlrects))))
        Sys_Error("Out of memory");
    i = 0;
    for (rect = rects; rect; rect = rect->pnext)
    {
        sdlrects[i].x = rect->x;
        sdlrects[i].y = rect->y;
        sdlrects[i].w = rect->width;
        sdlrects[i].h = rect->height;
        ++i;
    }
    //Hunk_Print(true);
    sdltexture = SDL_CreateTextureFromSurface(sdlrenderer, sdlblit);
	SDL_RenderCopy(sdlrenderer, sdltexture, NULL, sdlrects);
	SDL_DestroyTexture(sdltexture);
    SDL_RenderPresent(sdlrenderer);
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect(int x, int y, byte *pbitmap, int width, int height)
{
    Uint8 *offset;

    if (!sdlblit)
        return;
    if (x < 0)
        x = sdlblit->w + x - 1;
    offset = (Uint8 *)sdlblit->pixels + y * sdlblit->pitch + x;
    while (height--)
    {
        memcpy(offset, pbitmap, width);
        offset += sdlblit->pitch;
        pbitmap += width;
    }
}

/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect(int x, int y, int width, int height)
{
    if (!sdlscreen) {

        return;
    }
	//SDL_RenderClear(sdlrenderer);
    sdltexture = SDL_CreateTextureFromSurface(sdlrenderer, sdlblit);
	SDL_RenderCopy(sdlrenderer, sdltexture, NULL, NULL);
	SDL_DestroyTexture(sdltexture);
    SDL_RenderPresent(sdlrenderer);
}