#pragma once

#include <libdragon.h>
#include <SDL.h>

typedef enum ControlSchemes {
    CS_TANK,
    CS_TANK_MODERN,
    CS_STICK_LOOK,
    CS_STICK_LOOK_MODERN,
    CS_DUAL
} ControlSchemes;

extern ControlSchemes control_scheme;
extern int joy_x, joy_y;

void ProcessJoyButton(SDL_Event *event);
void ProcessJoyAxisMotion(SDL_Event *event);