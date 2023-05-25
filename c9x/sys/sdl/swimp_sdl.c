

#ifdef Q2
#include "../ref_soft/r_local.h"

#include <SDL2/SDL.h>

extern SDL_Surface *sdlscreen;
extern SDL_Surface *sdlblit;
extern SDL_Renderer *sdlrenderer;
extern SDL_Window *sdlwindow;
extern  SDL_Texture *sdltexture;

void		SWimp_BeginFrame( float camera_separation )
{
}

void		SWimp_EndFrame (void)
{
    SDL_Surface *tmp = SDL_ConvertSurfaceFormat (sdlblit, SDL_GetWindowPixelFormat (sdlwindow), 0);
    SDL_RenderClear (sdlrenderer);
    SDL_UpdateTexture (sdltexture, NULL, tmp->pixels, tmp->pitch);
    SDL_RenderCopy (sdlrenderer, sdltexture, NULL, NULL);
    SDL_RenderPresent (sdlrenderer);
    SDL_FreeSurface (tmp);

}

int			SWimp_Init( void *hInstance, void *wndProc )
{
    sdltexture = SDL_CreateTextureFromSurface(sdlrenderer, sdlblit);
}

void		SWimp_SetPalette( const unsigned char *palette)
{
}

void		SWimp_Shutdown( void )
{
}

rserr_t		SWimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
    rserr_t retval = rserr_ok;

    ri.Con_Printf (PRINT_ALL, "setting mode %d:", mode );

    if ( !ri.Vid_GetModeInfo( pwidth, pheight, mode ) )
    {
        ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
        return rserr_invalid_mode;
    }

    ri.Con_Printf( PRINT_ALL, " %d %d\n", *pwidth, *pheight);

    // if ( !SWimp_InitGraphics( fullscreen ) ) {
    //     // failed to set a valid mode in windowed mode
    //     return rserr_invalid_mode;
    // }

    R_GammaCorrectAndSetPalette( ( const unsigned char * ) d_8to24table );

    return retval;
}

void		SWimp_AppActivate( qboolean active )
{
}
#endif

