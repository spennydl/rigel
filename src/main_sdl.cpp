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
    memory.frame_temp_arena = memory.ephemeral_arena.alloc_sub_arena(5 * ONE_MB);
    memory.resource_arena = memory.ephemeral_arena.alloc_sub_arena(12 * ONE_MB);
    memory.gfx_arena = memory.ephemeral_arena.alloc_sub_arena(3 * ONE_KB);
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
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow("rigel", 1920, 1080, SDL_WINDOW_OPENGL);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(window, nullptr);

    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

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
    i64 start_time;
    if (!SDL_GetCurrentTime(&start_time)) {
        std::cerr << "warn: couln't get current time? " << SDL_GetError() << std::endl;
    }

    render::default_atlas_rebuffer(&memory.frame_temp_arena);

    render::VertexBuffer vertbuf;
    render::set_up_vertex_buffer_for_rectangles(&vertbuf);

    render::RectangleBufferVertex rect = {0};
    rect.world_min = m::Vec2 { 0, 0 };
    rect.world_max = m::Vec2 { 20, 20 };
    rect.color_and_strength = m::Vec4{ 1, 0, 0, 1 };

    render::buffer_rectangles(&vertbuf, &rect, 1, &memory.frame_temp_arena);

    render::RenderTarget internal_target = render::make_render_to_texture_target(320, 180);
    //render::RenderTarget shadow_target = render::make_render_to_array_texture_target(320, 180, 24, GL_RGBA);

    while (running) {
        render::BatchBuffer* entity_batch_buffer = render::make_batch_buffer(&memory.frame_temp_arena, 256);
        auto rect_shader = render::game_shaders + render::SIMPLE_SPRITE_SHADER;

        render::batch_push_use_shader_cmd(entity_batch_buffer, rect_shader);

        render::batch_push_attach_texture_cmd(entity_batch_buffer, 0, render::get_default_sprite_atlas_texture());

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


            // Prepare blank scene
            render::begin_render(viewport, w, h);

            render::BatchBuffer* render_buffer = render::make_batch_buffer(&memory.frame_temp_arena, 1024);

            render::batch_push_clear_buffer_cmd(render_buffer, m::Vec4 { 1, 0, 1, 1 }, true);

            render::batch_push_switch_target_cmd(render_buffer, &internal_target);

            render::batch_push_clear_buffer_cmd(render_buffer, m::Vec4 { 0, 1, 1, 1 }, true);

            // TODO: need to get the background image from somewhere
            //
            // for now I will just render a blank quad
            render::batch_push_use_shader_cmd(render_buffer, &render::game_shaders[render::SIMPLE_RECTANGLE_SHADER]);
            render::batch_push_rectangle(render_buffer,
                                    m::Vec3 { 0, 0, 0 },
                                    m::Vec3 { 320, 180, 0 },
                                    m::Vec4 { 0.018, 0.018, 0.018, 1 });

            // submit prepare
            render::submit_batch(render_buffer, &memory.frame_temp_arena);
            render::batch_buffer_reset(render_buffer);

            // add the level details to the entity batch
            auto simple_sprite_shader = &render::game_shaders[render::SIMPLE_SPRITE_SHADER];
            render::batch_push_use_shader_cmd(entity_batch_buffer, simple_sprite_shader);

            auto world_chunk = game_state->active_world_chunk;
            auto tilesheet_tex = render::get_renderable_texture(world_chunk->active_map->tile_sheet);
            render::batch_push_attach_texture_cmd(entity_batch_buffer, 0, tilesheet_tex);

            auto map_foreground_buffer = &world_chunk->active_map->vert_buffer;
            render::batch_push_draw_vertex_buffer_cmd(entity_batch_buffer, map_foreground_buffer);

            // render all the stuff to the internal target
            render::submit_batch(entity_batch_buffer, &memory.frame_temp_arena);

            render::batch_push_switch_target_cmd(render_buffer, render::get_default_render_target());

            auto screen_shader = &render::game_shaders[render::SCREEN_SHADER];
            render::shader_set_uniform_m4v(screen_shader, "world_transform", m::mat4_I());

            render::batch_push_use_shader_cmd(render_buffer, screen_shader);
            render::batch_push_attach_texture_cmd(render_buffer, 0, &internal_target.target_texture);
            render::batch_push_quad(render_buffer,
                                    m::Vec4 { 0, 0, 0, 1 },
                                    m::Vec4 { 1920, 0, 0, 1 },
                                    m::Vec4 { 1920, 1080, 0, 1 },
                                    m::Vec4 { 0, 1080, 0, 1 },
                                    m::Vec4 { 0, 0, 0, 0 });

            render::submit_batch(render_buffer, &memory.frame_temp_arena);

            render::end_render();

#define DBG_DUMP_SPRITESHEET 0
#if DBG_DUMP_SPRITESHEET
            auto spritesheet_min = m::Vec3 { (1920 - 512) / 2, (1080 - 512) / 2, 0 };
            auto spritesheet_dims = m::Vec3 { 512, 512, 0 };

            auto test_spritesheet_batch = render::make_batch_buffer(&memory.frame_temp_arena, 256);

            render::batch_push_use_shader_cmd(test_spritesheet_batch, render::game_shaders + render::SIMPLE_RECTANGLE_SHADER);

            render::batch_push_rectangle(test_spritesheet_batch, spritesheet_min, spritesheet_min + spritesheet_dims, m::Vec4 { 0, 0, 0, 1 });

            render::batch_push_use_shader_cmd(test_spritesheet_batch, render::game_shaders + render::SIMPLE_QUAD_SHADER);

            render::batch_push_quad(test_spritesheet_batch,
                                    m::extend(spritesheet_min, 1),
                                    m::extend(spritesheet_min + m::Vec3 {spritesheet_dims.x, 0, 0}, 1),
                                    m::extend(spritesheet_min + spritesheet_dims, 1),
                                    m::extend(spritesheet_min + m::Vec3 {0, spritesheet_dims.y, 0 }, 1),
                                    m::Vec4 {0, 0, 0, 0});

            render::submit_batch(test_spritesheet_batch, &memory.frame_temp_arena);
#endif

            SDL_GL_SwapWindow(window);

            memory.frame_temp_arena.reinit();


            i64 one_frame_time;
            if (!SDL_GetCurrentTime(&one_frame_time)) {
                std::cerr << "warn: couln't get current time? " << SDL_GetError() << std::endl;
            }
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
