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
// in_null.c -- for systems without a mouse
#ifdef Q1
#include "quakedef.h"
#include <SDL2/SDL.h>
extern double mouse_x, mouse_y;
int mouse_oldbuttonstate = 0;
void IN_Init(void) { mouse_x = mouse_y = 0.0; }

void IN_Shutdown(void) {}

void IN_Commands(void)
{
    int i;
    int mouse_buttonstate;

    // if (!mouse_avail) return;

    i = SDL_GetMouseState(NULL, NULL);
    /* Quake swaps the second and third buttons */
    mouse_buttonstate = (i & ~0x06) | ((i & 0x02) << 1) | ((i & 0x04) >> 1);
    for (i = 0; i < 3; i++)
    {
        if ((mouse_buttonstate & (1 << i)) &&
            !(mouse_oldbuttonstate & (1 << i)))
            Key_Event(K_MOUSE1 + i, true);

        if (!(mouse_buttonstate & (1 << i)) &&
            (mouse_oldbuttonstate & (1 << i)))
            Key_Event(K_MOUSE1 + i, false);
    }
    mouse_oldbuttonstate = mouse_buttonstate;
}

void IN_Move(usercmd_t *cmd)
{
    // if (!mouse_avail)
    //     return;

    mouse_x *= sensitivity.value;
    mouse_y *= sensitivity.value;

    if ((in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1)))
        cmd->sidemove += m_side.value * mouse_x;
    else
        cl.viewangles[YAW] -= m_yaw.value * mouse_x;
    if (in_mlook.state & 1)
        V_StopPitchDrift();

    if ((in_mlook.state & 1) && !(in_strafe.state & 1))
    {
        cl.viewangles[PITCH] += m_pitch.value * mouse_y;
        if (cl.viewangles[PITCH] > 80)
            cl.viewangles[PITCH] = 80;
        if (cl.viewangles[PITCH] < -70)
            cl.viewangles[PITCH] = -70;
    }
    else
    {
        if ((in_strafe.state & 1) && noclip_anglehack)
            cmd->upmove -= m_forward.value * mouse_y;
        else
            cmd->forwardmove -= m_forward.value * mouse_y;
    }
    mouse_x = mouse_y = 0.0;
}

#elif defined(Q2)

#include "../client/client.h"
cvar_t *in_mouse;
cvar_t *in_joystick;
// TODO
extern qboolean OpenJoystick(cvar_t *joy_dev);

extern double mouse_x, mouse_y;
extern int mouse_oldbuttonstate;
void getMouse(int *x, int *y, int *state);
void doneMouse();
void IN_Init(void)
{
    mouse_x = mouse_y = 0.0;
    in_mouse = Cvar_Get("in_mouse", "1", CVAR_ARCHIVE);
    in_joystick = Cvar_Get("in_joystick", "0", CVAR_ARCHIVE);
    OpenJoystick(in_joystick);
}

void IN_Shutdown(void) {}

void IN_Commands(void)
{
    int i;
    int mouse_buttonstate;

    i = SDL_GetMouseState(NULL, NULL);
    /* Quake swaps the second and third buttons */
    mouse_buttonstate = (i & ~0x06) | ((i & 0x02) << 1) | ((i & 0x04) >> 1);
    for (i = 0; i < 3; i++)
    {
        if ((mouse_buttonstate & (1 << i)) &&
            !(mouse_oldbuttonstate & (1 << i)))
            Key_Event(K_MOUSE1 + i, true, Sys_Milliseconds());

        if (!(mouse_buttonstate & (1 << i)) &&
            (mouse_oldbuttonstate & (1 << i)))
            Key_Event(K_MOUSE1 + i, false, Sys_Milliseconds());
    }
    doneMouse();
}

void IN_Frame(void) {}

void IN_Move(usercmd_t *cmd)
{
    int mouse_buttonstate = mouse_oldbuttonstate;
    // if (m_filter->value)
    // {
    //     mouse_x = (mouse_x + old_mouse_x) * 0.5;
    //     mouse_y = (mouse_y + old_mouse_y) * 0.5;
    // }

    // old_mouse_x = mouse_x;
    // old_mouse_y = mouse_y;

    if (mouse_x || mouse_y)
    {
        // if (!exponential_speedup->value)
        // {
        //     mouse_x *= sensitivity->value;
        //     mouse_y *= sensitivity->value;
        // }
        // else
        {
            // if (mouse_x > MOUSE_MIN || mouse_y > MOUSE_MIN ||
            //     mouse_x < -MOUSE_MIN || mouse_y < -MOUSE_MIN)
            // {
            //     mouse_x = (mouse_x * mouse_x * mouse_x) / 4;
            //     mouse_y = (mouse_y * mouse_y * mouse_y) / 4;
            //     if (mouse_x > MOUSE_MAX)
            //         mouse_x = MOUSE_MAX;
            //     else if (mouse_x < -MOUSE_MAX)
            //         mouse_x = -MOUSE_MAX;
            //     if (mouse_y > MOUSE_MAX)
            //         mouse_y = MOUSE_MAX;
            //     else if (mouse_y < -MOUSE_MAX)
            //         mouse_y = -MOUSE_MAX;
            // }
        }
        // add mouse X/Y movement to cmd
        // if ((*cl.in_strafe_state & 1) || (lookstrafe->value && mlooking))
        //     cmd->sidemove += m_side->value * mouse_x;
        // else
            cl.viewangles[YAW] -= m_yaw->value * mouse_x;

            cl.viewangles[PITCH] += m_pitch->value * mouse_y;
        doneMouse();
    }
}

void IN_Activate(qboolean active) {}

void IN_ActivateMouse(void) {}

void IN_DeactivateMouse(void) {}

#endif
