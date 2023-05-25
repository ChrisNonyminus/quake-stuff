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

#include <SDL2/SDL.h>
#include <sys/time.h>

#include "errno.h"
#ifdef Q1
#include "quakedef.h"
#elif defined(Q2)
#include "../client/client.h"
extern viddef_t vid;
#endif

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

    printf("Sys_Error: ");
    va_start(argptr, error);
    vprintf(error, argptr);
    va_end(argptr);
    printf("\n");

#if 0 // hacky way to cause a breakpoint... by triggering segfaults. Hey, as
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
            Key_Event(sym, state
#ifdef Q2
                      ,
                      0
#endif
            );
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
                    SDL_WarpMouseInWindow(NULL, vid.width / 2, vid.height / 2);
                }
            }
            break;

        case SDL_QUIT:
#ifdef Q1
            CL_Disconnect();
            Host_ShutdownServer(false);
            Sys_Quit();
#elif defined(Q2)
            Com_Quit();
#endif
            break;
        default:
            break;
        }
    }
}

void Sys_HighFPPrecision(void) {}

void Sys_LowFPPrecision(void) {}

#ifdef Q2
// QUAKE II Sys code!
void Sys_ConsoleOutput(char *string) { Sys_Printf(string); }

void Sys_Init(void) {}
int curtime;
int Sys_Milliseconds(void)
{
    curtime = Sys_FloatTime();
    return curtime;
}
// TODO
void Sys_Mkdir(char *path) {}

char *Sys_FindFirst(char *path, unsigned musthave, unsigned canthave)
{
    return NULL;
}

char *Sys_FindNext(unsigned musthave, unsigned canthave) { return NULL; }

void Sys_FindClose(void) {}

char *Sys_GetClipboardData(void) { return NULL; }

int hunkcount;

byte *membase;
int hunkmaxsize;
int cursize;
void *Hunk_Begin(int maxsize)
{
    // reserve a huge chunk of memory, but don't commit any yet
    cursize = 0;
    hunkmaxsize = maxsize;
#if 0
	membase = VirtualAlloc (NULL, maxsize, MEM_RESERVE, PAGE_NOACCESS);
#else
    membase = malloc(maxsize);
    memset(membase, 0, maxsize);
#endif
    if (!membase)
        Sys_Error("VirtualAlloc reserve failed");
    return (void *)membase;
}

void *Hunk_Alloc(int size)
{
    void *buf;

    // round to cacheline
    size = (size + 31) & ~31;

#if 0
	// commit pages as needed
//	buf = VirtualAlloc (membase+cursize, size, MEM_COMMIT, PAGE_READWRITE);
	buf = VirtualAlloc (membase, cursize+size, MEM_COMMIT, PAGE_READWRITE);
	if (!buf)
	{
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &buf, 0, NULL);
		Sys_Error ("VirtualAlloc commit failed.\n%s", buf);
	}
#endif
    cursize += size;
    if (cursize > hunkmaxsize)
        Sys_Error("Hunk_Alloc overflow");

    return (void *)(membase + cursize - size);
}

int Hunk_End(void)
{

    // free the remaining unused virtual memory
#if 0
	void	*buf;

	// write protect it
	buf = VirtualAlloc (membase, cursize, MEM_COMMIT, PAGE_READONLY);
	if (!buf)
		Sys_Error ("VirtualAlloc commit failed");
#endif

    hunkcount++;
    // Com_Printf ("hunkcount: %i\n", hunkcount);
    return cursize;
}

void Hunk_Free(void *base)
{
    if (base)
#if 0
		VirtualFree (base, 0, MEM_RELEASE);
#else
        free(base);
#endif

    hunkcount--;
}

void SNDDMA_BeginPainting(void) {}

void Sys_AppActivate (void)
{
}

#ifdef Q2
#include "game.h"

void	*Sys_GetGameAPI (void *parms)
{
	return GetGameAPI(parms);
}

void Con_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		msg[1024];
	static qboolean	inupdate;
	
	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);
	
// also echo to debugging console
	Sys_Printf ("%s", msg);	// also echo to debugging console

// // log all messages to file
// 	if (con_debuglog)
// 		Con_DebugLog(va("%s/qconsole.log",com_gamedir), "%s", msg);

// 	if (!con_initialized)
// 		return;
		
// 	if (cls.state == ca_dedicated)
// 		return;		// no graphics mode

// // write it to the scrollable buffer
// 	Con_Print (msg);
	
// // update the screen if the console is displayed
// 	if (cls.signon != SIGNONS && !scr_disabled_for_loading )
// 	{
// 	// protect against infinite loop if something in SCR_UpdateScreen calls
// 	// Con_Printd
// 		if (!inupdate)
// 		{
// 			inupdate = true;
// 			SCR_UpdateScreen ();
// 			inupdate = false;
// 		}
// 	}
}

// TODO: Net stuff
char	*NET_AdrToString (netadr_t a)
{
	static	char	s[64];
	
	Com_sprintf (s, sizeof(s), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));

	return s;
}
void NET_Init(void) {

}
void		NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to) {
    
}
void		NET_Config (qboolean multiplayer) {

}
qboolean	NET_StringToAdr (char *s, netadr_t *a) {
    return true;
}
qboolean	NET_IsLocalAddress (netadr_t adr) {
    return true;
}
qboolean	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message) {
    return false;
}
qboolean	NET_CompareAdr (netadr_t a, netadr_t b) {
    return true;
}
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b) {
    return true;
}
void		NET_Sleep(int msec) {
    //SDL_Delay(msec);
}
#endif

// QUAKE II Sys code END
#endif
//=============================================================================

#ifdef Q1

void main(int argc, char **argv)
{
    static quakeparms_t parms;
    double time, oldtime, newtime;

    if (SDL_Init(0) < 0)
    {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    parms.memsize = 8 * 1024 * 1024;
    parms.membase = malloc(parms.memsize);
    parms.basedir = ".";

    COM_InitArgv(argc, argv);

    parms.argc = com_argc;
    parms.argv = com_argv;

    printf("Host_Init\n");
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
}
#elif defined(Q2)
int main (int argc, char **argv)
{
    int     time, oldtime, newtime;

    if (SDL_Init(0) < 0)
    {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    // // go back to real user for config loads
    // saved_euid = geteuid();
    // seteuid(getuid());
    
    //printf ("Quake 2 -- Version %s\n", LINUX_VERSION);

    Qcommon_Init(argc, argv);

    //fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

    // nostdout = Cvar_Get("nostdout", "0", 0);
    // if (!nostdout->value) {
    //     fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);    
    // }
    oldtime = Sys_Milliseconds ();

    // test
    SV_Map(true, "base3", true);

    while (1)
    {
// find time spent rendering last frame
        // do {
        //     newtime = Sys_Milliseconds ();
        //     time = newtime - oldtime;
        // } while (time < 1);
        Qcommon_Frame (time);
        oldtime = newtime;
    }
}

#endif
