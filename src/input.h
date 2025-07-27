#ifndef RIGEL_INPUT_H_
#define RIGEL_INPUT_H_

#include "rigel.h"

namespace rigel
{

#define INPUT_ACTION_LIST \
    X(MoveRight) \
    X(MoveLeft) \
    X(Jump)

#define X(action) InputAction_##action,
// platform layer should map its keys to input actions
enum InputAction
{
    InputAction_None = 0,
    INPUT_ACTION_LIST
    InputAction_NInputActions
};
#undef X

template<typename PlatformKeycode>
struct InputKeyMapping
{
    InputAction action;
    PlatformKeycode key;
    PlatformKeycode key_alt;
};

template<typename PlatformKeycode>
struct InputKeyMappingTable
{
    InputKeyMapping<PlatformKeycode> map[InputAction_NInputActions];
};


struct InputState {
    bool move_right_requested;
    bool move_left_requested;
    bool jump_requested;
};

extern InputState g_input_state;

using InputKeyMap = InputKeyMappingTable<i32>;

struct InputDevice
{
    u32 id;
    InputKeyMap keymap;
};

constexpr static u32 KEYBOARD_DEVICE_ID = 0;

void input_start();
InputDevice* open_gamepad(u32 gamepad_id);
void close_gamepad(u32 gamepad_id);

InputDevice* get_active_input_device();
void set_active_input_device(u32 device_id);
InputAction get_action_for_button(i32 code);

}


#endif // INPUT_H_
