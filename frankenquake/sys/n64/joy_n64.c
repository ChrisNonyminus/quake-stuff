#include "joy_n64.h"

#include "quakedef.h"

ControlSchemes control_scheme = CS_TANK;
int joy_x = 0, joy_y = 0;

static void Tank_ProcessJoyButton(SDL_Event *event);
static void Tank_ProcessJoyAxisMotion(SDL_Event *event);
static void TankModern_ProcessJoyButton(SDL_Event *event);
static void TankModern_ProcessJoyAxisMotion(SDL_Event *event);

static void StickLook_ProcessJoyButton(SDL_Event *event);
static void StickLook_ProcessJoyAxisMotion(SDL_Event *event);
static void StickLookModern_ProcessJoyButton(SDL_Event *event);
static void StickLookModern_ProcessJoyAxisMotion(SDL_Event *event);

static void Dual_ProcessJoyButton(SDL_Event *event);
static void Dual_ProcessJoyAxisMotion(SDL_Event *event);

void ProcessJoyButton(SDL_Event *event)
{
    switch (control_scheme)
    {
    case CS_TANK:
        Tank_ProcessJoyButton(event);
        break;
    case CS_TANK_MODERN:
        TankModern_ProcessJoyButton(event);
        break;
    case CS_STICK_LOOK:
        StickLook_ProcessJoyButton(event);
        break;
    case CS_STICK_LOOK_MODERN:
        StickLookModern_ProcessJoyButton(event);
        break;
    case CS_DUAL:
        Dual_ProcessJoyButton(event);
        break;
    }
}

void ProcessJoyAxisMotion(SDL_Event *event)
{
    switch (control_scheme)
    {
    case CS_TANK:
        Tank_ProcessJoyAxisMotion(event);
        break;
    case CS_TANK_MODERN:
        TankModern_ProcessJoyAxisMotion(event);
        break;
    case CS_STICK_LOOK:
        StickLook_ProcessJoyAxisMotion(event);
        break;
    case CS_STICK_LOOK_MODERN:
        StickLookModern_ProcessJoyAxisMotion(event);
        break;
    case CS_DUAL:
        Dual_ProcessJoyAxisMotion(event);
        break;
    }
}

static void Tank_ProcessJoyButton(SDL_Event *event)
{
    switch(event->jbutton.button)
    {
        case 0: // A
            if (event->jbutton.state == SDL_PRESSED)
                Cmd_ExecuteString("+jump", src_command);
            else
                Cmd_ExecuteString("-jump", src_command);
            Key_Event(SDLK_RETURN, event->key.state);
            break;
        case 1: // B
            break;
        case 2: // Z
            if (event->jbutton.state == SDL_PRESSED)
                Cmd_ExecuteString("+attack", src_command);
            else
                Cmd_ExecuteString("-attack", src_command);
            break;
        case 3: // start
            Key_Event(K_ESCAPE, event->key.state);
            break;
        case 4: // L
            break;
        case 5: // R
            if (event->jbutton.state == SDL_PRESSED)
                Cmd_ExecuteString("+speed", src_command);
            else
                Cmd_ExecuteString("-speed", src_command);
            break;
        case 6: // C_up
            if (event->jbutton.state == SDL_PRESSED)
            Cmd_ExecuteString("+lookup", src_command);
            else
            Cmd_ExecuteString("-lookup", src_command);
            break;
        case 7: // C_down
            if (event->jbutton.state == SDL_PRESSED)
            Cmd_ExecuteString("+lookdown", src_command);
            else
            Cmd_ExecuteString("-lookdown", src_command);
            break;
        case 8: // C_left
            if (event->jbutton.state == SDL_PRESSED)
                Cmd_ExecuteString("+moveleft", src_command);
            else
                Cmd_ExecuteString("-moveleft", src_command);
            break;
        case 9: // C_right
            if (event->jbutton.state == SDL_PRESSED)
                Cmd_ExecuteString("+moveright", src_command);
            else
                Cmd_ExecuteString("-moveright", src_command);
            break;
    }
}
static void Tank_ProcessJoyAxisMotion(SDL_Event *event)
{
    switch (event->jaxis.axis)
    {
    case 0: // x axis
        // mouse_x += event->jaxis.value;
        if (event->jaxis.value < -10.f || event->jaxis.value > 10.f)
            joy_x = event->jaxis.value / 500;
        else
            joy_x = 0;
        break;
    case 1: // y axis
        // here we should set the current angle he should look to
        if (event->jaxis.value < -10.f)
            Cmd_ExecuteString("+forward", src_command);
        else if (event->jaxis.value > 10.f)
            Cmd_ExecuteString("+back", src_command);
        else {
            Cmd_ExecuteString("-forward", src_command);
            Cmd_ExecuteString("-back", src_command);
        }
        break;
    }
}

static void TankModern_ProcessJoyButton(SDL_Event *event)
{

}
static void TankModern_ProcessJoyAxisMotion(SDL_Event *event)
{

}

static void StickLook_ProcessJoyButton(SDL_Event *event)
{

}
static void StickLook_ProcessJoyAxisMotion(SDL_Event *event)
{

}

static void StickLookModern_ProcessJoyButton(SDL_Event *event)
{

}
static void StickLookModern_ProcessJoyAxisMotion(SDL_Event *event)
{

}

static void Dual_ProcessJoyButton(SDL_Event *event)
{
    
}
static void Dual_ProcessJoyAxisMotion(SDL_Event *event)
{
    
}
