#include "rigel.h"
#include "input.h"
#include "SDL3/SDL.h"

#include <iostream>

namespace rigel
{

InputState g_input_state;

enum AxisAndDirection
{
    AxisAndDirection_LeftXPos = SDL_GAMEPAD_BUTTON_COUNT,
    AxisAndDirection_LeftXNeg,
    AxisAndDirection_LeftYPos,
    AxisAndDirection_LeftYNeg,
    AxisAndDirection_RightXPos,
    AxisAndDirection_RightXNeg,
    AxisAndDirection_RightYPos,
    AxisAndDirection_RightYNeg,
    AxisAndDirection_N
};

InputKeyMap keyboard_mapping = {
    {
        { InputAction_None,      SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN },
        { InputAction_MoveRight, SDL_SCANCODE_D,       SDL_SCANCODE_UNKNOWN },
        { InputAction_MoveLeft,  SDL_SCANCODE_A,       SDL_SCANCODE_UNKNOWN },
        { InputAction_Jump,      SDL_SCANCODE_W,       SDL_SCANCODE_UNKNOWN },
    }
};

InputKeyMap controller_mapping = {
    {
        { InputAction_None,      SDL_GAMEPAD_BUTTON_INVALID,    SDL_GAMEPAD_BUTTON_INVALID },
        { InputAction_MoveRight, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, AxisAndDirection_LeftXPos  },
        { InputAction_MoveLeft,  SDL_GAMEPAD_BUTTON_DPAD_LEFT,  AxisAndDirection_LeftXNeg  },
        { InputAction_Jump,      SDL_GAMEPAD_BUTTON_SOUTH,      SDL_GAMEPAD_BUTTON_INVALID }
    }
};


// 0 is an invalid joystick ID in SDL
constexpr static u32 N_SUPPORTED_GAMEPADS = 4;

InputDevice keyboard = { KEYBOARD_DEVICE_ID, keyboard_mapping };
InputDevice gamepads[N_SUPPORTED_GAMEPADS] = {0};

InputDevice* active_input_device = &keyboard;

InputDevice* find_gamepad(u32 gamepad_id)
{
    u32 gamepad_idx = 0;
    for (; gamepad_idx < N_SUPPORTED_GAMEPADS; gamepad_idx++)
    {
        if (gamepads[gamepad_idx].id == gamepad_id)
        {
            return gamepads + gamepad_idx;
        }
    }
    return nullptr;
}

void input_start()
{
    i32 gamepad_count;
    auto plugged_gamepads = SDL_GetGamepads(&gamepad_count);
    std::cout << "Found " << gamepad_count << " gamepads" << std::endl;

    for (u32 gamepad = 0;
         gamepad < (u32)gamepad_count && gamepad < N_SUPPORTED_GAMEPADS;
         gamepad++)
    {
        if (SDL_OpenGamepad(plugged_gamepads[gamepad]))
        {
            gamepads[gamepad].id = plugged_gamepads[gamepad];
            gamepads[gamepad].keymap = controller_mapping;
        }
        else
        {
            std::cout << "Nope" << std::endl;
        }
    }
    if (gamepad_count)
    {
        // default to gamepad if available
        active_input_device = gamepads;
    }
}

InputDevice* open_gamepad(u32 gamepad_id)
{
    u32 gamepad_idx = 0;
    for (; gamepad_idx < N_SUPPORTED_GAMEPADS; gamepad_idx++)
    {
        if (gamepads[gamepad_idx].id == 0)
        {
            break;
        }
    }
    assert(gamepad_idx < N_SUPPORTED_GAMEPADS && "Ran out of gamepads?");

    auto device = gamepads + gamepad_idx;
    if (SDL_OpenGamepad(gamepad_id))
    {
        device->id = gamepad_id;
        device->keymap = controller_mapping;
        return device;
    }
    return nullptr;
}

void close_gamepad(u32 gamepad_id)
{
    u32 gamepad_idx = 0;
    for (; gamepad_idx < N_SUPPORTED_GAMEPADS; gamepad_idx++)
    {
        if (gamepads[gamepad_idx].id == gamepad_id)
        {
            break;
        }
    }
    if (gamepad_idx < N_SUPPORTED_GAMEPADS)
    {
        auto device = gamepads + gamepad_idx;
        SDL_CloseGamepad(SDL_GetGamepadFromID(device->id));
        device->id = 0;
    }
}

InputDevice* get_active_input_device()
{
    return active_input_device;
}

void set_active_input_device(u32 device_id)
{
    if (device_id == KEYBOARD_DEVICE_ID)
    {
        active_input_device = &keyboard;
    }
    else
    {
        auto gamepad = find_gamepad(device_id);
        if (gamepad)
        {
            std::cout << "setting to gamepad with id " << gamepad->id;
            active_input_device = gamepad;
        }
        else
        {
            assert(false && "Input device does not exist");
        }
    }
}

InputAction get_action_for_button(i32 code)
{
    for (u32 i = 0; i < InputAction_NInputActions; i++)
    {
        auto mapping = active_input_device->keymap.map + i;
        if (mapping->key == code || mapping->key_alt == code)
        {
            return mapping->action;
        }
    }
    return InputAction_None;
}

}
