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

#include <SDL2/SDL.h>
#ifdef Q1
#include "quakedef.h"
#elif defined(Q2)
// #include "../client/client.h"
#endif

#ifdef Q1
viddef_t vid; // global video state
#else
#include "r_local.h"
viddef_t	viddef;				// global video state
#endif

#define BASEWIDTH 320 * 2
#define BASEHEIGHT 200 * 2

static SDL_Surface *sdlscreen = NULL;
static SDL_Surface *sdlblit = NULL;
static SDL_Renderer *sdlrenderer = NULL;
static SDL_Window *sdlwindow = NULL;

extern short *d_pzbuffer;

short zbuffer[BASEWIDTH * BASEHEIGHT];

byte *surfcache;

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

void VID_Init(
#ifdef Q1
    unsigned char *palette
#else
    void
#endif
)
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
    sdlscreen = SDL_GetWindowSurface(sdlwindow);
#ifdef Q1
    vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
    vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
    vid.aspect = vid.width / vid.height;
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int *)vid.colormap + 2048));
#endif
    sdlblit = SDL_CreateRGBSurfaceWithFormat(0, vid.width, vid.height, 8,
                                             SDL_PIXELFORMAT_INDEX8);
    vid.buffer =
#ifdef Q1
        vid.conbuffer =
#endif
            sdlblit->pixels;
    vid.rowbytes =
#ifdef Q1
        vid.conrowbytes =
#endif
            BASEWIDTH;

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

static SDL_Texture *sdltexture = NULL;
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
    SDL_DestroyTexture(sdltexture);
    sdltexture = SDL_CreateTextureFromSurface(sdlrenderer, sdlblit);
    SDL_RenderCopy(sdlrenderer, sdltexture, NULL, NULL);
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
    if (!sdlscreen)
        return;
    // SDL_RenderClear(sdlrenderer);
    SDL_DestroyTexture(sdltexture);
    sdltexture = SDL_CreateTextureFromSurface(sdlrenderer, sdlblit);
    SDL_RenderCopy(sdlrenderer, sdltexture, NULL, NULL);
    SDL_RenderPresent(sdlrenderer);
}
