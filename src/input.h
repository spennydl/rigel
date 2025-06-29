#ifndef RIGEL_INPUT_H_
#define RIGEL_INPUT_H_

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


template<typename T>
struct InputKeyMapping
{
    InputAction action;
    T key;
    T key_alt;
};

template<typename T>
struct InputKeyMappingTable
{
    InputKeyMapping<T> map[InputAction_NInputActions];
};

struct InputState {
    bool move_right_requested;
    bool move_left_requested;
    bool jump_requested;
};

// owned by the platform layer
extern InputState g_input_state;

}


#endif // INPUT_H_
