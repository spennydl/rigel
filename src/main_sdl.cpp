#include "rigel.h"
#include "mem.h"
#include "world.h"
#include "render.h"
#include "collider.h"
#include "json.h"
#include "game.h"
#include "rigelmath.h"
#include "debug.h"
#include "input.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_time.h>
#include <glad/glad.h>
#include <iostream>
#include <sys/mman.h>

namespace rigel {

InputState g_input_state;

}

using namespace rigel;

mem::GameMem
initialize_game_memory()
{
    mem::GameMem memory;
    memory.game_state_storage_size = 8 * ONE_PAGE;
    auto gs_ptr = mmap(nullptr,
                       memory.game_state_storage_size,
                       PROT_READ | PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE,
                       -1,
                       0);
    assert(gs_ptr && "Couldn't map game state");
    memory.game_state_storage = reinterpret_cast<byte_ptr*>(gs_ptr);

    memory.ephemeral_storage_size = 20 * ONE_MB;
    auto es_ptr = mmap(nullptr,
                       memory.ephemeral_storage_size,
                       PROT_READ | PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE,
                       -1,
                       0);
    assert(es_ptr && "Couldn't map ephemeral");
    memory.ephemeral_storage = reinterpret_cast<byte_ptr*>(es_ptr);

    assert(sizeof(rigel::GameState) < memory.game_state_storage_size);

    memory.game_state_arena.arena_bytes = memory.game_state_storage_size;
    memory.game_state_arena.mem_begin = memory.game_state_storage;

    memory.ephemeral_arena.arena_bytes = memory.ephemeral_storage_size;
    memory.ephemeral_arena.mem_begin = memory.ephemeral_storage;

    memory.stage_arena = memory.ephemeral_arena.alloc_sub_arena(ONE_MB);
    memory.colliders_arena = memory.ephemeral_arena.alloc_sub_arena(1024);
    // TODO: this should be much bigger, ya?
    memory.frame_temp_arena = memory.ephemeral_arena.alloc_sub_arena(2 * ONE_MB);
    memory.resource_arena = memory.ephemeral_arena.alloc_sub_arena(12 * ONE_MB);
    memory.gfx_arena = memory.ephemeral_arena.alloc_sub_arena(2 * ONE_KB);
    memory.debug_arena = memory.ephemeral_arena.alloc_sub_arena(1 * ONE_MB);

    std::cout << "memory map:" << std::endl;
    std::cout << "game state: " << (mem_ptr*)memory.game_state_arena.mem_begin << " for " << memory.game_state_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "ephemeral: " << (mem_ptr*)memory.ephemeral_arena.mem_begin << " for " << memory.ephemeral_arena.arena_bytes << " bytes" << std::endl;

    std::cout << "stage: " << (mem_ptr*)memory.stage_arena.mem_begin << " for " << memory.stage_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "colliders: " << (mem_ptr*)memory.colliders_arena.mem_begin << " for " << memory.colliders_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "scratch: " << (mem_ptr*)memory.frame_temp_arena.mem_begin << " for " << memory.frame_temp_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "resource: " << (mem_ptr*)memory.resource_arena.mem_begin << " for " << memory.resource_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "gfx: " << (mem_ptr*)memory.gfx_arena.mem_begin << " for " << memory.gfx_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "debug: " << (mem_ptr*)memory.debug_arena.mem_begin << " for " << memory.debug_arena.arena_bytes << " bytes" << std::endl;
    return memory;
}


int main()
{
    // TODO(spencer) remove! this is for renderdoc if (!SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11"))
    {
        std::cout << "didn't set the hint" << std::endl;
    }
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow("rigel", 1920, 1080, SDL_WINDOW_OPENGL);
    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(window, nullptr);

    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    //SDL_GL_SetSwapInterval(1); // Enable vsync

    i32 w, h;
    SDL_GetRenderOutputSize(sdl_renderer, &w, &h);
    std::cout << "the screen is " << w << "x" << h << std::endl;

    gladLoadGL();
    std::cout << "Loaded version " << "Loaded OpenGL " << GLVersion.major << " " << GLVersion.minor << std::endl;

    input_start();

    mem::GameMem memory = initialize_game_memory();


    resource_initialize(memory.resource_arena);

#ifdef RIGEL_DEBUG
    debug::init_debug(&memory.debug_arena);
#endif

    render::initialize_renderer(&memory.gfx_arena, w, h);

    memory.frame_temp_arena.reinit_zeroed();
    GameState* game_state = load_game(memory);

    render::Viewport viewport;
    viewport.zoom(1.0);

    bool running = true;
    SDL_Event event;

    i64 last_update_time;
    i64 last_render_time;

    if (!SDL_GetCurrentTime(&last_update_time)) {
        std::cerr << "warn: couln't get current time? " << SDL_GetError() << std::endl;
    }

    if (!SDL_GetCurrentTime(&last_render_time)) {
        std::cerr << "warn: couln't get current time? " << SDL_GetError() << std::endl;
    }

    render::default_atlas_rebuffer(&memory.frame_temp_arena);

    while (running) {

        render::BatchBuffer* entity_batch_buffer = render::make_batch_buffer(&memory.frame_temp_arena, 128);
        auto rect_shader = render::game_shaders + render::SIMPLE_RECTANGLE_SHADER;
        auto shader_item = render::push_render_item<render::UseShaderCmdItem>(entity_batch_buffer);
        shader_item->shader = rect_shader;

        while (SDL_PollEvent(&event)) {
            // quick and dirty for now
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    running = false;
                }
                break;
                case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                {
                    SDL_GamepadButtonEvent gbutton = event.gbutton;

                    if (get_active_input_device()->id != gbutton.which)
                    {
                        set_active_input_device(gbutton.which);
                    }

                    auto action = get_action_for_button(gbutton.button);
                    if (action != InputAction_None)
                    {
                        switch (action)
                        {
                            case InputAction_MoveRight:
                            {
                                g_input_state.move_right_requested = true;
                            } break;
                            case InputAction_MoveLeft:
                            {
                                g_input_state.move_left_requested = true;
                            } break;
                            case InputAction_Jump:
                            {
                                g_input_state.jump_requested = true;
                            } break;
                            default:
                                break;
                        }
                    }
                } break;
                case SDL_EVENT_GAMEPAD_BUTTON_UP:
                {
                    SDL_GamepadButtonEvent gbutton = event.gbutton;

                    if (get_active_input_device()->id != gbutton.which)
                    {
                        set_active_input_device(gbutton.which);
                    }

                    auto action = get_action_for_button(gbutton.button);
                    if (action != InputAction_None)
                    {
                        switch (action)
                        {
                            case InputAction_MoveRight:
                            {
                                g_input_state.move_right_requested = false;
                            } break;
                            case InputAction_MoveLeft:
                            {
                                g_input_state.move_left_requested = false;
                            } break;
                            case InputAction_Jump:
                            {
                                g_input_state.jump_requested = false;
                            } break;
                            default:
                                break;
                        }
                    }

                } break;
                case SDL_EVENT_KEY_DOWN: {
                    SDL_KeyboardEvent key_event = event.key;
                    if (key_event.repeat) {
                        continue;
                    }

                    if (get_active_input_device()->id != KEYBOARD_DEVICE_ID)
                    {
                        set_active_input_device(KEYBOARD_DEVICE_ID);
                    }

                    auto action = get_action_for_button(key_event.scancode);
                    if (action != InputAction_None)
                    {
                        switch (action)
                        {
                            case InputAction_MoveRight:
                            {
                                g_input_state.move_right_requested = true;
                            } break;
                            case InputAction_MoveLeft:
                            {
                                g_input_state.move_left_requested = true;
                            } break;
                            case InputAction_Jump:
                            {
                                g_input_state.jump_requested = true;
                            } break;
                            default:
                                break;
                        }
                    }
                }
                break;
                case SDL_EVENT_KEY_UP:
                {
                    SDL_KeyboardEvent key_event = event.key;

                    if (get_active_input_device()->id != KEYBOARD_DEVICE_ID)
                    {
                        set_active_input_device(KEYBOARD_DEVICE_ID);
                    }

                    auto action = get_action_for_button(key_event.scancode);
                    if (action != InputAction_None)
                    {
                        switch (action)
                        {
                            case InputAction_MoveRight:
                            {
                                g_input_state.move_right_requested = false;
                            } break;
                            case InputAction_MoveLeft:
                            {
                                g_input_state.move_left_requested = false;
                            } break;
                            case InputAction_Jump:
                            {
                                g_input_state.jump_requested = false;
                            } break;
                            default:
                                break;
                        }
                    }
                } break;
                default:
                    break;
            }
        }

        i64 iter_time;
        if (!SDL_GetCurrentTime(&iter_time)) {
            std::cerr << "warn: couln't get current time? " << SDL_GetError() << std::endl;
        }


        i64 delta_update_time = iter_time - last_update_time;
        if (delta_update_time >= UPDATE_TIME_NS) {
#ifdef RIGEL_DEBUG
            debug::new_frame();

            //debug::DebugLine line;
            //line.start = { 10, 10, 0 };
            //line.start_color = {1.0f, 1.0f, 1.0f};
            //line.end = { 310.0f, 170.0f, 0.0f };
            //line.end_color = {1.0f, 1.0f, 1.0f};
//
            //debug::push_debug_line(line);
#endif
            //while (delta_update_time > 0) {
                f32 dt = delta_update_time / 1000000000.0f; // to seconds

                simulate_one_tick(memory, game_state, dt, entity_batch_buffer);

                update_animations(game_state->active_world_chunk, dt);

                delta_update_time -= UPDATE_TIME_NS;

            //}

            SDL_GetCurrentTime(&last_update_time);

            auto world_chunk = game_state->active_world_chunk;

            render::begin_render(viewport, game_state, w, h);

            render::lighting_pass(&memory.frame_temp_arena, world_chunk->active_map);

            render::begin_render_to_internal_target();

            render::render_background();

            render::render_background_layer(viewport, world_chunk);

            render::render_all_entities(viewport, world_chunk);

            render::render_foreground_layer(viewport, world_chunk);
            render::render_decoration_layer(viewport, world_chunk);

            render::submit_batch(entity_batch_buffer, &memory.frame_temp_arena);

            render::end_render_to_target();

            render::end_render();

            // use sprite shader
            auto other_batch = render::make_batch_buffer(&memory.frame_temp_arena, 64);
            auto sprite_shader = render::game_shaders + render::SIMPLE_SPRITE_SHADER;
            auto sprite_shader_item = render::push_render_item<render::UseShaderCmdItem>(other_batch);
            sprite_shader_item->shader = sprite_shader;
            // a test
            shader_set_uniform_1i(sprite_shader, "sprite_atlas", 1);

            // draw sprite
            auto sprite_render = render::push_render_item<render::SpriteItem>(other_batch);
            auto player = world_chunk->entities + world_chunk->player_id;
            sprite_render->sprite_id = player->new_sprite_id;
            sprite_render->position = {0, 0, 0};
            sprite_render->color_and_strength = {1, 0, 0, 0.0};

            render::submit_batch(other_batch, &memory.frame_temp_arena);

            SDL_GL_SwapWindow(window);

            memory.frame_temp_arena.reinit();
        }

        //std::cout << "Here we have " << entity_batch_buffer->items_in_buffer << std::endl;
        //i64 delta_render_time = iter_time - last_render_time;
        //if (delta_render_time >= RENDER_TIME_NS) {
//
            //auto world_chunk = game_state->active_world_chunk;
//
            //render::begin_render(viewport, game_state, w, h);
//
            //render::lighting_pass(&memory.frame_temp_arena, world_chunk->active_map);
//
            //render::begin_render_to_internal_target();
//
            //render::render_background();
//
            //render::render_background_layer(viewport, world_chunk);
//
            //render::render_all_entities(viewport, world_chunk);
//
            //render::render_foreground_layer(viewport, world_chunk);
            //render::render_decoration_layer(viewport, world_chunk);
//
            //render::submit_batch(entity_batch_buffer, &memory.frame_temp_arena);
//
            //render::end_render_to_target();
//
            //render::end_render();
//
            //SDL_GL_SwapWindow(window);
            //SDL_GetCurrentTime(&last_render_time);
        //}

    }

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
