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
viddef_t	viddef = {640, 400};				// global video state


extern float           surfscale;
extern qboolean        r_cache_thrash;         // set if surface cache is thrashing

extern int                                     sc_size;
extern surfcache_t                     *sc_rover, *sc_base;

#define GUARDSIZE       4

#define SURFCACHE_SIZE_AT_320X200	600*1024

int     D_SurfaceCacheForRes (int width, int height)
{
	int             size, pix;

	if (COM_CheckParm ("-surfcachesize"))
	{
		// size = Q_atoi(com_argv[COM_CheckParm("-surfcachesize")+1]) * 1024;
        size = 0;
		return size;
	}
	
	size = SURFCACHE_SIZE_AT_320X200;

	pix = width*height;
	if (pix > 64000)
		size += (pix-64000)*3;
		

	return size;
}

void D_CheckCacheGuard (void)
{
	byte    *s;
	int             i;

	s = (byte *)sc_base + sc_size;
	for (i=0 ; i<GUARDSIZE ; i++)
		if (s[i] != (byte)i)
			Sys_Error ("D_CheckCacheGuard: failed");
}

void D_ClearCacheGuard (void)
{
	byte    *s;
	int             i;
	
	s = (byte *)sc_base + sc_size;
	for (i=0 ; i<GUARDSIZE ; i++)
		s[i] = (byte)i;
}


/*
================
D_InitCaches

================
*/
void D_InitCaches (void *buffer, int size)
{

	if (1)
		Con_Printf ("%ik surface cache\n", size/1024);

	sc_size = size - GUARDSIZE;
	sc_base = (surfcache_t *)buffer;
	sc_rover = sc_base;
	
	sc_base->next = NULL;
	sc_base->owner = NULL;
	sc_base->size = sc_size;
	
	D_ClearCacheGuard ();
}

#define	MAXPRINTMSG	4096
void VID_Printf (int print_level, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean	inupdate;
	
	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);

	if (print_level == PRINT_ALL)
		Com_Printf ("%s", msg);
	else
		Com_DPrintf ("%s", msg);
}
qboolean VID_GetModeInfo( int *width, int *height, int mode )
{
	// if ( mode < 0 || mode >= 1 )
	// 	return false;

	*width  = vid.width;
	*height = vid.height;

	return true;
}

void VID_Error (int err_level, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean	inupdate;
	
	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);

	Com_Error (err_level,"%s", msg);
}
/*
** VID_NewWindow
*/
void VID_NewWindow ( int width, int height)
{
	viddef.width  = width;
	viddef.height = height;
}

void	VID_MenuInit (void)
{
}

extern refexport_t re;

extern refexport_t GetRefAPI (refimport_t rimp);
#endif

#define BASEWIDTH 320 * 2
#define BASEHEIGHT 200 * 2

 SDL_Surface *sdlscreen = NULL;
 SDL_Surface *sdlblit = NULL;
 SDL_Renderer *sdlrenderer = NULL;
 SDL_Window *sdlwindow = NULL;

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
#elif defined(Q2)
vid.width = BASEWIDTH;
vid.height = BASEHEIGHT;
#endif
    sdlblit = SDL_CreateRGBSurface (0, vid.width, vid.height, 8, 0, 0, 0, 0);

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


#ifdef Q2

	refimport_t	ri;

    ri.Cmd_AddCommand = Cmd_AddCommand;
	ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
	ri.Cmd_Argc = Cmd_Argc;
	ri.Cmd_Argv = Cmd_Argv;
	ri.Cmd_ExecuteText = Cbuf_ExecuteText;
	ri.Con_Printf = VID_Printf;
	ri.Sys_Error = VID_Error;
	ri.FS_LoadFile = FS_LoadFile;
	ri.FS_FreeFile = FS_FreeFile;
	ri.FS_Gamedir = FS_Gamedir;
	ri.Cvar_Get = Cvar_Get;
	ri.Cvar_Set = Cvar_Set;
	ri.Cvar_SetValue = Cvar_SetValue;
	ri.Vid_GetModeInfo = VID_GetModeInfo;
	ri.Vid_MenuInit = VID_MenuInit;
	ri.Vid_NewWindow = VID_NewWindow;

    re = GetRefAPI(ri);

    re.Init(0, 0);

    viddef.width = vid.width;
    viddef.height = vid.height;
#endif
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

 SDL_Texture *sdltexture = NULL;
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

// QUAKE II stuff

void	VID_MenuDraw (void)
{
}

const char *VID_MenuKey( int k)
{
	return NULL;
}

// END QUAKE II
