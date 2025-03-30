#include "rigel.h"
#include "mem.h"
#include "world.h"
#include "render.h"
#include "collider.h"
#include "json.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <SDL3/SDL_time.h>
#include <glad/gl.h>
#include <iostream>
#include <sys/mman.h>


namespace rigel {

// TODO: what is this and where does it go
struct GameState
{
    WorldChunk* active_world_chunk;
};

InputState g_input_state;

}

using namespace rigel;

f32 signof(f32 val)
{
    return (val < 0) ? -1.0 : (val == 0) ? 0 : 1;
}

f32 abs(f32 val)
{
    return val * signof(val);
}


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
    memory.resource_arena = memory.ephemeral_arena.alloc_sub_arena(2 * ONE_MB);
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

GameState*
initialize_game_state(mem::GameMem& memory)
{
    GameState* state = memory.game_state_arena.alloc_simple<GameState>();
    return state;
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

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("rigel", 1280, 720, SDL_WINDOW_OPENGL);
    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(window, nullptr);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    int version = gladLoadGL((GLADloadfunc) SDL_GL_GetProcAddress);
    std::cout << "Loaded version " << version << std::endl;

    int tex_units;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &tex_units);
    std::cout << "can have " << tex_units << " per shader" << std::endl;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &tex_units);
    std::cout << "can have " << tex_units << " combined" << std::endl;
    mem::GameMem memory = initialize_game_memory();

    i32 w, h;
    SDL_GetRenderOutputSize(sdl_renderer, &w, &h);

    resource_initialize(memory.resource_arena);

    render::initialize_renderer(&memory.gfx_arena, w, h);


    Rectangle player_collider = { .x = 0, .y = 0, .w = 7, .h = 17 };
    ImageResource player_sprite = load_image_resource("resource/image/pcoutline.png", 5);

    GameState* game_state = initialize_game_state(memory);
    game_state->active_world_chunk = load_world_chunk(memory);
    game_state->active_world_chunk->add_player(memory, player_sprite.resource_id, glm::vec3(40, 64, 0), player_collider);

    ImageResource tilesheet = load_image_resource("resource/image/tranquil_tunnels_green_packed.png");
    render::make_world_chunk_renderable(&memory, game_state->active_world_chunk, tilesheet);

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

        AABB broad_player_aabb;
        i64 delta_update_time = iter_time - last_update_time;
        if (delta_update_time >= UPDATE_TIME_NS) {
            while (delta_update_time > 0) {
                f32 dt = delta_update_time / 1000000000.0f; // to seconds
                //std::cout << dt << std::endl;

                entity::Entity* player = game_state->active_world_chunk->entities;

                f32 dt2 = dt*dt;
                glm::vec3 new_pos = player->position + player->velocity*dt + player->acceleration*(dt2*0.5f);
                glm::vec3 new_acc = glm::vec3(0.0f);

                f32 gravity = -600; // TODO: grav
                if ((player->state_flags & entity::STATE_ON_LAND) > 0) {
                    gravity = 0;
                }
                new_acc.y += gravity;

                if (g_input_state.move_left_requested) {
                    new_acc.x -= 650;
                }

                if (g_input_state.move_right_requested) {
                    new_acc.x += 650;
                }

                bool is_requesting_move = g_input_state.move_left_requested || g_input_state.move_right_requested;
                if (abs(player->velocity.x) > 0 && !is_requesting_move) {
                    new_acc.x -= 40 * player->velocity.x;
                }

                glm::vec3 new_vel = player->velocity + (player->acceleration + new_acc)*(dt*0.5f);

                if (g_input_state.jump_requested) {
                    entity::state_transition_land_to_jump(player);
                    g_input_state.jump_requested = false;
                    new_vel.y = 180;
                }

                if (abs(new_vel.x) > 100) {
                    new_vel.x = 100 * signof(new_vel.x);
                }
                if ((player->state_flags & entity::STATE_ON_LAND) == 0) {
                    anim_frame = 1;
                } else if (abs(new_vel.x) > 0.1) {
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

                AABB player_aabb = player->colliders->aabbs[0];
                player_aabb.center = player->position + player_aabb.extents;

                glm::vec3 player_displacement = new_pos - player->position;
                broad_player_aabb.center = (player_aabb.center) + (0.5f*player_displacement);
                broad_player_aabb.extents = player_aabb.extents + glm::abs(0.5f*player_displacement);

                glm::vec3 tl = glm::vec3(broad_player_aabb.center.x - broad_player_aabb.extents.x,
                                         broad_player_aabb.center.y + broad_player_aabb.extents.y, 0);
                glm::vec3 br = glm::vec3(broad_player_aabb.center.x + broad_player_aabb.extents.x,
                                         broad_player_aabb.center.y - broad_player_aabb.extents.y, 0);
                glm::vec2 tile_min = world_to_tiles(tl);
                glm::vec2 tile_max = world_to_tiles(br);

                //std::cout << "Min: " << tile_min.x << "," << tile_min.y << std::endl;
                //std::cout << "Max: " << tile_max.x << "," << tile_max.y << std::endl;

                TileMap* map = game_state->active_world_chunk->active_map;

                for (isize tile_y = tile_min.y; tile_y <= tile_max.y; tile_y++) {
                    for (isize tile_x = tile_min.x; tile_x <= tile_max.x; tile_x++) {
                        player_aabb.center = player->position + player_aabb.extents;

                        usize tile_index = tile_to_index(tile_x, tile_y);
                        if (tile_index >= WORLD_SIZE_TILES || map->tiles[tile_index] == TileType::EMPTY) {
                            continue;
                        }

                        AABB tile_aabb;
                        tile_aabb.extents = glm::vec3(4, 4, 0);
                        tile_aabb.center = tiles_to_world(tile_x, tile_y) + tile_aabb.extents;

                        CollisionResult result = collide_AABB_with_static_AABB(&player_aabb,
                                                                               &tile_aabb,
                                                                               player_displacement);

                        if (result.t_to_collide >= 0) {
                            if (result.t_to_collide == 0) {
                                auto correction = result.penetration_axis * result.depth;
                                player->position += correction;
                                new_pos += correction;
                                glm::vec3 norm = glm::vec3(-result.penetration_axis.y, result.penetration_axis.x, 0);
                                new_vel = glm::dot(new_vel, norm) * norm;
                                new_acc = glm::dot(new_acc, norm) * norm;
                            } else {
                                player->position += result.t_to_collide * player_displacement;
                                glm::vec3 remainder = result.depth * result.penetration_axis;
                                glm::vec3 norm = glm::vec3(-result.penetration_axis.y, result.penetration_axis.x, 0);

                                new_pos = player->position + (glm::dot(remainder, norm) * norm);
                                new_vel = glm::dot(new_vel, norm) * norm;
                                new_acc = glm::dot(new_acc, norm) * norm;
                                player_displacement = new_pos - player->position;
                            }
                            if (result.penetration_axis.y > 0) {
                                entity::state_transition_air_to_land(player);
                            }
                        }
                    }
                }

                player->position = new_pos;
                player->velocity = new_vel;
                player->acceleration = new_acc;
                delta_update_time -= UPDATE_TIME_NS;

                //std::cout << player->velocity.x << "," << player->velocity.y << std::endl;
            }

            SDL_GetCurrentTime(&last_update_time);
        }

        i64 delta_render_time = iter_time - last_render_time;
        if (delta_render_time >= RENDER_TIME_NS) {
            render::begin_render(w, h);

            render::begin_render_to_target(render::internal_target());
            // i am lost in the sauce and i need to keep this simple or else i will never complete it!
            //
            // _all_ scenes will have:
            // - a background "effect" shader layer that is aaaaall the way in the background.
            // - a background tile layer
            // - a foreground tile & sprite layer
            // - a decoration tile layer
            //
            // that's it! don't start into a crazy generic layered pipeline just yet!

            render::render_background_layer(&memory.gfx_arena, viewport);

            //entity::Entity* player = game_state->active_world_chunk->entities;
            //Rectangle player_rect = rect_from_aabb(player->colliders->aabbs[0]);
            //player_rect.x = player->position.x;
            //player_rect.y = player->position.y;
            //render::draw_rectangle(rect_from_aabb(broad_player_aabb), 0.0, 1.0, 0.0);
            //render::draw_rectangle(player_rect, 1.0, 0.0, 0.0);

            render::render_all_entities(&memory.gfx_arena, viewport, game_state->active_world_chunk, anim_frame);

            render::render_foreground_layer(&memory.gfx_arena, viewport);
            render::render_decoration_layer(&memory.gfx_arena, viewport);

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
