#include "joy_n64.h"

#include "quakedef.h"

ControlSchemes control_scheme = CS_TANK;
int joy_x = 0, joy_y = 0;

#define DEADZONE 10.f

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

void InitControlScheme(ControlSchemes new_scheme)
{
    switch (new_scheme)
    {
    case CS_TANK:
        Cmd_ExecuteString("-mlook", src_command);
        break;
    case CS_TANK_MODERN:
        Cmd_ExecuteString("-mlook", src_command);
        break;
    case CS_STICK_LOOK:
        Cmd_ExecuteString("+mlook", src_command);
        break;
    case CS_STICK_LOOK_MODERN:
        Cmd_ExecuteString("+mlook", src_command);
        break;
    case CS_DUAL:
        Cmd_ExecuteString("+mlook", src_command);
        break;
    default:
        break;
    }

    control_scheme = new_scheme;
}

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
    default:
        break;
    }

    // force L to change control scheme
    if (event->jbutton.button == 4 && event->jbutton.state == SDL_PRESSED)
    {
        InitControlScheme((control_scheme + 1) % CS_MAX);
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
    default:
        break;
    }
}

static void Tank_ProcessJoyButton(SDL_Event *event)
{
    switch(event->jbutton.button)
    {
        case 0: // A
            if (event->jbutton.state == SDL_PRESSED)
                Cmd_ExecuteString("impulse 10", src_command);
            break;
        case 1: // B
            if (event->jbutton.state == SDL_PRESSED)
                Cmd_ExecuteString("impulse 12", src_command);
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
                Cmd_ExecuteString("+jump", src_command);
            else
                Cmd_ExecuteString("-jump", src_command);
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
        if (event->jaxis.value < -DEADZONE || event->jaxis.value > DEADZONE)
            joy_x = event->jaxis.value / 500;
        else
            joy_x = 0;
        break;
    case 1: // y axis
        // here we should set the current angle he should look to
        if (event->jaxis.value < -DEADZONE)
            Cmd_ExecuteString("+forward", src_command);
        else if (event->jaxis.value > DEADZONE)
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
    // no changes for now
    Tank_ProcessJoyButton(event);
}
static void TankModern_ProcessJoyAxisMotion(SDL_Event *event)
{
    // no changes for now
    Tank_ProcessJoyAxisMotion(event);
}

static void StickLook_ProcessJoyButton(SDL_Event *event)
{
    switch(event->jbutton.button)
    {
        case 0: // A
            if (event->jbutton.state == SDL_PRESSED)
                Cmd_ExecuteString("impulse 10", src_command);
            break;
        case 1: // B
            if (event->jbutton.state == SDL_PRESSED)
                Cmd_ExecuteString("impulse 12", src_command);
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
                Cmd_ExecuteString("+jump", src_command);
            else
                Cmd_ExecuteString("-jump", src_command);
            break;
        case 6: // C_up
            if (event->jbutton.state == SDL_PRESSED)
            Cmd_ExecuteString("+forward", src_command);
            else
            Cmd_ExecuteString("-forward", src_command);
            break;
        case 7: // C_down
            if (event->jbutton.state == SDL_PRESSED)
            Cmd_ExecuteString("+back", src_command);
            else
            Cmd_ExecuteString("-back", src_command);
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
static void StickLook_ProcessJoyAxisMotion(SDL_Event *event)
{
    switch (event->jaxis.axis)
    {
    case 0: // x axis
        if (event->jaxis.value < -DEADZONE || event->jaxis.value > DEADZONE)
            joy_x = event->jaxis.value / 500;
        else
            joy_x = 0;
        break;
    case 1: // y axis
        if (event->jaxis.value < -DEADZONE || event->jaxis.value > DEADZONE)
            joy_y = event->jaxis.value / 500;
        else
            joy_y = 0;
        break;
    }
}

static void StickLookModern_ProcessJoyButton(SDL_Event *event)
{
    // no changes for now
    StickLook_ProcessJoyButton(event);
}
static void StickLookModern_ProcessJoyAxisMotion(SDL_Event *event)
{
    // no changes for now
    StickLook_ProcessJoyAxisMotion(event);
}

static void Dual_ProcessJoyButton(SDL_Event *event)
{
    
}
static void Dual_ProcessJoyAxisMotion(SDL_Event *event)
{
    
}
