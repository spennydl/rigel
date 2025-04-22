#include "rigel.h"
#include "mem.h"
#include "world.h"
#include "render.h"
#include "collider.h"
#include "json.h"
#include "game.h"
#include "rigelmath.h"


#include <SDL3/SDL.h>
#include <SDL3/SDL_time.h>
#include <glad/glad.h>
#include <iostream>
#include <sys/mman.h>

/*

 TODO:

 Things are looking much better in here.

 Threads to pull on:
 - level loading needs work. i think it's time to rip out tinyxml and say thanks for your service.
   - Switch to loading from json and rip out tinyxml2
   - Load entities, lights, and objects from json exported from tiled
   - create the concept of trigger zones, load from json
   - hot-reloading
   - use trigger zones to switch between levels
 - text rendering!
 - debug line rendering!
 - pull player behavior out into a more final place. Was thinking a brain sorta abstraction,
   but I dunno.

Done:
 - start laying in lighting.
 - we need a proper math lib. if we could get rid of glm i'd be super happy too. This may
   be a good task for a slow day.
 - there is work to do on collision, but collision is a bloody pit.

*/

namespace rigel {

InputState g_input_state;

}

using namespace rigel;

mem::GameMem
initialize_game_memory()
{
    mem::GameMem memory;
    memory.game_state_storage_size = 4 * ONE_PAGE;
    auto gs_ptr = mmap(nullptr,
                       memory.game_state_storage_size,
                       PROT_READ | PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE,
                       -1,
                       0);
    assert(gs_ptr && "Couldn't map game state");
    memory.game_state_storage = reinterpret_cast<byte_ptr*>(gs_ptr);

    memory.ephemeral_storage_size = 16 * ONE_MB;
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
    memory.scratch_arena = memory.ephemeral_arena.alloc_sub_arena(2 * ONE_MB);
    memory.resource_arena = memory.ephemeral_arena.alloc_sub_arena(8 * ONE_MB);
    memory.gfx_arena = memory.ephemeral_arena.alloc_sub_arena(2 * ONE_KB);

    std::cout << "memory map:" << std::endl;
    std::cout << "game state: " << (mem_ptr*)memory.game_state_arena.mem_begin << " for " << memory.game_state_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "ephemeral: " << (mem_ptr*)memory.ephemeral_arena.mem_begin << " for " << memory.ephemeral_arena.arena_bytes << " bytes" << std::endl;

    std::cout << "stage: " << (mem_ptr*)memory.stage_arena.mem_begin << " for " << memory.stage_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "colliders: " << (mem_ptr*)memory.colliders_arena.mem_begin << " for " << memory.colliders_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "scratch: " << (mem_ptr*)memory.scratch_arena.mem_begin << " for " << memory.scratch_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "resource: " << (mem_ptr*)memory.resource_arena.mem_begin << " for " << memory.resource_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "gfx: " << (mem_ptr*)memory.gfx_arena.mem_begin << " for " << memory.gfx_arena.arena_bytes << " bytes" << std::endl;
    return memory;
}


int main()
{
    //////////////////////////////////
/*
    mem::GameMem memory = initialize_game_memory();
    auto json_head = parse_json_file(&memory.scratch_arena, "resource/map/layeredlevel.tmj");
    std::cout << "**********************************" << std::endl;
    std::cout << "head type is " << json_head->type << std::endl;

    auto field = json_head->object->kvps;
    while (field) {
        if (json_str_equals(field->key, "\"layers\"", 8)) {
            std::cout << "found layers" << std::endl;
            auto layers_obj = field->value.obj_array->obj;
            auto layer_field = layers_obj->kvps;
            while (layer_field) {
                if (json_str_equals(layer_field->key, "\"data\"", 6)) {
                    auto arr = layer_field->value.number_array;
                    for (int i = 0; i < arr->n; i++) {
                        std::cout << arr->arr[i].value << std::endl;
                    }
                    std::cout << "found " << arr->n << " tiles" << std::endl;
                    break;
                }
                layer_field = layer_field->next;
            }
        }
        field = field->next;
    }
*/
    mem::GameMem memory = initialize_game_memory();
    resource_initialize(memory.resource_arena);
    TextResource json_txt = load_text_resource("resource/map/layered2.tmj");
    JsonValue* json_val = parse_json_string(&memory.scratch_arena, json_txt.text);
    assert(json_val->type == JSON_OBJECT && "didn't find a root obj");

    JsonObj* json_obj = json_val->object;

    JsonValue* v_width = jsonobj_get(json_obj, "width", 5);
    if (v_width)
    {
        std::cout << "found width and width is " << v_width->number->value << std::endl;
    }
    else
    {
        std::cout << "it was null, yo" << std::endl;
        return 1;
    }

    JsonValue* layers_v = jsonobj_get(json_obj, "layers", 6);
    assert(layers_v->type == JSON_OBJECT_ARRAY && "layers was wrong");
    JsonObjArray* layers = layers_v->obj_array;
    while (layers)
    {
        JsonObj* obj = layers->obj;
        auto name = jsonobj_get(obj, "name", 4);
        assert(name && "no name");

        if (json_str_equals(*name->string, "fg", 2))
        {
            std::cout << "found fg layer" << std::endl;
            auto tiles = jsonobj_get(obj, "data", 4);
            assert(tiles && "no tiles");

            auto num_array = tiles->number_array;
            auto count = num_array->n;
            auto arr = num_array->arr;
            for (int i = 0; i < count; i++)
            {
                std::cout << arr[i].value << ", " << std::endl;
            }
            break;
        }
        layers = layers->next;
    }

    return 0;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("rigel", 1920, 1080, SDL_WINDOW_OPENGL);
    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(window, nullptr);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    int version = gladLoadGL();
    std::cout << "Loaded version " << version << std::endl;

    int tex_units;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &tex_units);
    std::cout << "can have " << tex_units << " per shader" << std::endl;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &tex_units);
    std::cout << "can have " << tex_units << " combined" << std::endl;
    //mem::GameMem memory = initialize_game_memory();

    i32 w, h;
    SDL_GetRenderOutputSize(sdl_renderer, &w, &h);
    std::cout << "the screen is " << w << "x" << h << std::endl;

    resource_initialize(memory.resource_arena);

    render::initialize_renderer(&memory.gfx_arena, w, h);

    Rectangle player_collider = { .x = 0, .y = 0, .w = 7, .h = 17 };
    ImageResource player_sprite = load_image_resource("resource/image/pcoutline.png", 5);

    GameState* game_state = initialize_game_state(memory);
    game_state->active_world_chunk = load_world_chunk(memory);
    game_state->active_world_chunk->add_player(memory, player_sprite.resource_id, m::Vec3{40.0f, 64.0f, 0.0f}, player_collider);

    ImageResource tilesheet = load_image_resource("resource/image/tranquil_tunnels_transparent.png");
    render::make_world_chunk_renderable(&memory.scratch_arena, game_state->active_world_chunk, tilesheet);

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

    i64 anim_time = 0;
    usize anim_frame = 0;

    while (running) {
        memory.scratch_arena.reinit();

        while (SDL_PollEvent(&event)) {
            // quick and dirty for now
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    running = false;
                    break;
                }
                case SDL_EVENT_KEY_DOWN: {
                    SDL_KeyboardEvent key_event = event.key;
                    if (key_event.repeat) {
                        continue;
                    }
                    switch (key_event.scancode) {
                        case SDL_SCANCODE_W: {
                            g_input_state.jump_requested = true;
                            break;
                        }
                        case SDL_SCANCODE_A: {
                            g_input_state.move_left_requested = true;
                            break;
                        }
                        case SDL_SCANCODE_D: {
                            g_input_state.move_right_requested = true;
                            break;
                        }

                    }

                    break;
                }
                case SDL_EVENT_KEY_UP: {
                    SDL_KeyboardEvent key_event = event.key;
                    switch (key_event.scancode) {
                        case SDL_SCANCODE_W: {
                            // TODO: this isn't gonna work.
                            g_input_state.jump_requested = false;
                            break;
                        }
                        case SDL_SCANCODE_A: {
                            g_input_state.move_left_requested = false;
                            break;
                        }
                        case SDL_SCANCODE_D: {
                            g_input_state.move_right_requested = false;
                            break;
                        }

                    }

                    break;
                }
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
            while (delta_update_time > 0) {
                f32 dt = delta_update_time / 1000000000.0f; // to seconds
                //std::cout << dt << std::endl;

                Entity* player = game_state->active_world_chunk->entities;

                m::Vec3 new_acc = {0};

                f32 gravity = -600; // TODO: grav
                if (player->state_flags & STATE_ON_LAND) {
                    gravity = 0;
                }
                new_acc.y += gravity;

                if (g_input_state.move_left_requested) {
                    new_acc.x -= 450;
                }

                if (g_input_state.move_right_requested) {
                    new_acc.x += 450;
                }

                bool is_requesting_move = g_input_state.move_left_requested || g_input_state.move_right_requested;
                if (m::abs(player->velocity.x) > 0 && !is_requesting_move) {
                    new_acc.x -= m::signof(player->velocity.x) * 600;// * player->velocity.x;
                }

                if (g_input_state.jump_requested) {
                    state_transition_land_to_jump(player);
                    g_input_state.jump_requested = false;
                    player->velocity.y = 180;
                }

                TileMap* active_map = game_state->active_world_chunk->active_map;
                player->acceleration = new_acc;

                move_entity(player, active_map, dt);

                if (player->velocity.y <= 0)
                {
                    AABB player_aabb = player->colliders->aabbs[0];
                    player_aabb.center = player->position + player_aabb.extents;
                    player_aabb.center.y = player_aabb.center.y - 1;
                    if (collides_with_level(player_aabb, active_map))
                    {
                        state_transition_air_to_land(player);
                    }
                    else
                    {
                        state_transition_land_to_fall(player);
                    }
                }

                // TODO: This has gotta go somewhere better
                if ((player->state_flags & STATE_ON_LAND) == 0) {
                    anim_frame = 1;
                } else if (abs(player->acceleration.x) > 0.1) {
                    anim_time += delta_update_time;
                    if (anim_time > 120000000) {
                        anim_frame++;
                        anim_time = 0;
                    }
                    if (anim_frame >= 5) {
                        anim_frame = 1;
                    }
                } else {
                    anim_frame = 0;
                    anim_time = 0;
                }
                update_zero_cross_trigger(&player->facing_dir, player->velocity.x);

                delta_update_time -= UPDATE_TIME_NS;
            }

            SDL_GetCurrentTime(&last_update_time);
        }

        i64 delta_render_time = iter_time - last_render_time;
        if (delta_render_time >= RENDER_TIME_NS) {
            render::begin_render(viewport, game_state, w, h);

            render::lighting_pass(&memory.scratch_arena, game_state->active_world_chunk->active_map);

            render::begin_render_to_internal_target();

            render::render_background_layer(viewport);
#if 0
            // TODO: I need a debug renderer that will let me do this stuff
            // from anywhere in the codebase, even it all it does is draw lines.
            Rectangle player_rect = rect_from_aabb(player->colliders->aabbs[0]);
            player_rect.x = player->position.x;
            player_rect.y = player->position.y;
            render::draw_rectangle(rect_from_aabb(broad_player_aabb), 0.0, 1.0, 0.0);
            render::draw_rectangle(player_rect, 1.0, 0.0, 0.0);
#endif

            render::render_all_entities(viewport, game_state->active_world_chunk, anim_frame);

            render::render_foreground_layer(viewport);
            render::render_decoration_layer(viewport);

            render::end_render_to_target();

            render::end_render();

            SDL_GL_SwapWindow(window);
            SDL_GetCurrentTime(&last_render_time);
        }

    }

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
