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
// sys_null.h -- null system driver to aid porting efforts

#include <SDL.h>
#include <sys/time.h>

#include <libdragon.h>

#include "errno.h"
#include "quakedef.h"

double mouse_x, mouse_y;

/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES 10
FILE *sys_handles[MAX_HANDLES];

int findhandle(void)
{
    int i;

    for (i = 1; i < MAX_HANDLES; i++)
        if (!sys_handles[i])
            return i;
    Sys_Error("out of handles");
    return -1;
}

/*
================
filelength
================
*/
int filelength(FILE *f)
{
    int pos;
    int end;

    pos = ftell(f);
    fseek(f, 0, SEEK_END);
    end = ftell(f);
    fseek(f, pos, SEEK_SET);

    return end;
}

int Sys_FileOpenRead(char *path, int *hndl)
{
    FILE *f;
    int i;

    i = findhandle();

    f = fopen(path, "rb");
    if (!f)
    {
        *hndl = -1;
        return -1;
    }
    sys_handles[i] = f;
    *hndl = i;

    return filelength(f);
}

int Sys_FileOpenWrite(char *path)
{
    FILE *f;
    int i;

    i = findhandle();

    f = fopen(path, "wb");
    if (!f)
        Sys_Error("Error opening %s: %s", path, strerror(errno));
    sys_handles[i] = f;

    return i;
}

void Sys_FileClose(int handle)
{
    fclose(sys_handles[handle]);
    sys_handles[handle] = NULL;
}

void Sys_FileSeek(int handle, int position)
{
    fseek(sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead(int handle, void *dest, int count)
{
    return fread(dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite(int handle, void *data, int count)
{
    return fwrite(data, 1, count, sys_handles[handle]);
}

int Sys_FileTime(char *path)
{
    FILE *f;

    f = fopen(path, "rb");
    if (f)
    {
        fclose(f);
        return 1;
    }

    return -1;
}

void Sys_mkdir(char *path) {}

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length) {}

void Sys_Error(char *error, ...)
{
    va_list argptr;
    SDL_Quit();

    printf("Sys_Error: ");
    va_start(argptr, error);
    vprintf(error, argptr);
    va_end(argptr);
    printf("\n");

#if 1 // hacky way to cause a breakpoint... by triggering segfaults. Hey, as
      // long as it works... which it might not...
    *(int *)0 = 0;
#endif

    exit(1);
}

void Sys_Printf(char *fmt, ...)
{
    va_list argptr;

    va_start(argptr, fmt);
    vprintf(fmt, argptr);
    va_end(argptr);
}

void Sys_Quit(void) { exit(0); }

double Sys_FloatTime(void)
{
    // static double t = 0;

    // t += 0.1;

    // return t;

    struct timeval tp;
    static int secbase;

    gettimeofday(&tp, NULL);

    if (!secbase)
    {
        secbase = tp.tv_sec;
        return tp.tv_usec / 1000000.0;
    }

    return (tp.tv_sec - secbase) + tp.tv_usec / 1000000.0;
}

char *Sys_ConsoleInput(void) { return NULL; }

void Sys_Sleep(void) { SDL_Delay(1); }

void Sys_SendKeyEvents(void)
{

    SDL_Event event;
    int sym, state;
    int modstate;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            sym = event.key.keysym.sym;
            state = event.key.state;
            modstate = SDL_GetModState();
            switch (sym)
            {
            case SDLK_DELETE:
                sym = K_DEL;
                break;
            case SDLK_BACKSPACE:
                sym = K_BACKSPACE;
                break;
            case SDLK_F1:
                sym = K_F1;
                break;
            case SDLK_F2:
                sym = K_F2;
                break;
            case SDLK_F3:
                sym = K_F3;
                break;
            case SDLK_F4:
                sym = K_F4;
                break;
            case SDLK_F5:
                sym = K_F5;
                break;
            case SDLK_F6:
                sym = K_F6;
                break;
            case SDLK_F7:
                sym = K_F7;
                break;
            case SDLK_F8:
                sym = K_F8;
                break;
            case SDLK_F9:
                sym = K_F9;
                break;
            case SDLK_F10:
                sym = K_F10;
                break;
            case SDLK_F11:
                sym = K_F11;
                break;
            case SDLK_F12:
                sym = K_F12;
                break;
            // case SDLK_BREAK:
            case SDLK_PAUSE:
                sym = K_PAUSE;
                break;
            case SDLK_UP:
                sym = K_UPARROW;
                break;
            case SDLK_DOWN:
                sym = K_DOWNARROW;
                break;
            case SDLK_RIGHT:
                sym = K_RIGHTARROW;
                break;
            case SDLK_LEFT:
                sym = K_LEFTARROW;
                break;
            case SDLK_INSERT:
                sym = K_INS;
                break;
            case SDLK_HOME:
                sym = K_HOME;
                break;
            case SDLK_END:
                sym = K_END;
                break;
            case SDLK_PAGEUP:
                sym = K_PGUP;
                break;
            case SDLK_PAGEDOWN:
                sym = K_PGDN;
                break;
            case SDLK_RSHIFT:
            case SDLK_LSHIFT:
                sym = K_SHIFT;
                break;
            case SDLK_RCTRL:
            case SDLK_LCTRL:
                sym = K_CTRL;
                break;
            case SDLK_RALT:
            case SDLK_LALT:
                sym = K_ALT;
                break;
                //    case SDLK_KP0:
                //        if(modstate & KMOD_NUM) sym = K_INS;
                //        else sym = SDLK_0;
                //        break;
                //    case SDLK_KP1:
                //        if(modstate & KMOD_NUM) sym = K_END;
                //        else sym = SDLK_1;
                //        break;
                //    case SDLK_KP2:
                //        if(modstate & KMOD_NUM) sym = K_DOWNARROW;
                //        else sym = SDLK_2;
                //        break;
                //    case SDLK_KP3:
                //        if(modstate & KMOD_NUM) sym = K_PGDN;
                //        else sym = SDLK_3;
                //        break;
                //    case SDLK_KP4:
                //        if(modstate & KMOD_NUM) sym = K_LEFTARROW;
                //        else sym = SDLK_4;
                //        break;
                //    case SDLK_KP5: sym = SDLK_5; break;
                //    case SDLK_KP6:
                //        if(modstate & KMOD_NUM) sym = K_RIGHTARROW;
                //        else sym = SDLK_6;
                //        break;
                //    case SDLK_KP7:
                //        if(modstate & KMOD_NUM) sym = K_HOME;
                //        else sym = SDLK_7;
                //        break;
                //    case SDLK_KP8:
                //        if(modstate & KMOD_NUM) sym = K_UPARROW;
                //        else sym = SDLK_8;
                //        break;
                //    case SDLK_KP9:
                //        if(modstate & KMOD_NUM) sym = K_PGUP;
                //        else sym = SDLK_9;
                //        break;
            case SDLK_KP_PERIOD:
                if (modstate & KMOD_NUM)
                    sym = K_DEL;
                else
                    sym = SDLK_PERIOD;
                break;
            case SDLK_KP_DIVIDE:
                sym = SDLK_SLASH;
                break;
            case SDLK_KP_MULTIPLY:
                sym = SDLK_ASTERISK;
                break;
            case SDLK_KP_MINUS:
                sym = SDLK_MINUS;
                break;
            case SDLK_KP_PLUS:
                sym = SDLK_PLUS;
                break;
            case SDLK_KP_ENTER:
                sym = SDLK_RETURN;
                break;
            case SDLK_KP_EQUALS:
                sym = SDLK_EQUALS;
                break;
            }
            // If we're not directly handled and still above 255
            // just force it to 0
            if (sym > 255)
                sym = 0;
            Key_Event(sym, state);
            break;

        case SDL_MOUSEMOTION:
            if ((event.motion.x != (vid.width / 2)) ||
                (event.motion.y != (vid.height / 2)))
            {
                mouse_x = event.motion.xrel * 10;
                mouse_y = event.motion.yrel * 10;
                if ((event.motion.x < ((vid.width / 2) - (vid.width / 4))) ||
                    (event.motion.x > ((vid.width / 2) + (vid.width / 4))) ||
                    (event.motion.y < ((vid.height / 2) - (vid.height / 4))) ||
                    (event.motion.y > ((vid.height / 2) + (vid.height / 4))))
                {
                    //SDL_WarpMouseInWindow(NULL, vid.width / 2, vid.height / 2);
                }
            }
            break;

        case SDL_QUIT:
            CL_Disconnect();
            Host_ShutdownServer(false);
            Sys_Quit();
            break;
        default:
            break;
        }
    }
}

void Sys_HighFPPrecision(void) {}

void Sys_LowFPPrecision(void) {}

//=============================================================================
void COM_AddGameDirectory (char *dir);
int main(int argc, char **argv)
{
    static quakeparms_t parms;
    double time, oldtime, newtime;




    console_init();
	console_set_render_mode(RENDER_AUTOMATIC);
    int ret = dfs_init(DFS_DEFAULT_LOCATION);
	assert(ret == DFS_ESUCCESS);

    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0)
    {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    parms.memsize = 512 * 1024;
    parms.membase = malloc(parms.memsize);
    //COM_AddGameDirectory("sd:/quake");

    COM_InitArgv(argc, argv);

    parms.argc = com_argc;
    parms.argv = com_argv;
    parms.basedir = "rom://";

    Sys_Printf("Host_Init\n");
    Host_Init(&parms);
    oldtime = Sys_FloatTime() - 0.1;
    while (1)
    {
        newtime = Sys_FloatTime();
        time = newtime - oldtime;

        if (cls.state == ca_dedicated)
        {
            if (time < sys_ticrate.value)
            {
                SDL_Delay(1);
                continue; // not time to run a server only tic yet
            }
            time = sys_ticrate.value;
        }
        if (time > sys_ticrate.value * 2)
            oldtime = newtime;
        else
            oldtime += time;
        Host_Frame(time);
    }

    return 0;
}