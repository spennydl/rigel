#include "rigel.h"
#include "render.h"
#include "game.h"
#include "resource.h"
#include "json.h"
#include "render.h"
#include "debug.h"
#include "input.h"
#include "world.h"

namespace rigel {


// zonetrigger: does the player overlap with the zone?
TEST_TRIGGER_FN(ZoneTrigger)
{
    auto player = game_state->active_world_chunk->entities + game_state->active_world_chunk->player_id;
    auto player_aabb = player->colliders->aabbs[0];
    player_aabb.center = player->position + player_aabb.extents;

    auto zone_aabb = aabb_from_rect(data->rect);

    auto result = simple_AABB_overlap(player_aabb, zone_aabb);
    return !(result.x == -1 && result.y == -1);
}

EFFECT_FN(ChangeLevel)
{
}

EFFECT_FN(SpawnEntity)
{

}

EntityPrototype entity_prototypes[EntityType_NumberOfTypes];

Direction
check_for_level_change(Entity* player)
{
    // TODO(spencer): looking at center pos isn't the best because we may end up
    // letting the player move further than they would otherwise be allowed to causing
    // them to get stuck in off-screen walls.
    // Ideally the player shouldn't be too concerned with off-screen walls and the level
    // design should make sure they don't have to be, but we should also be robust enough
    // to handle it.
    auto player_center = player->position + player->colliders->aabbs[0].extents;

    if (player_center.x < 0)
    {
        return Direction_Left;
    }
    if (player_center.x >= WORLD_WIDTH_TILES * TILE_WIDTH_PIXELS)
    {
        return Direction_Right;
    }
    if (player_center.y < 0)
    {
        return Direction_Down;
    }
    if (player_center.y >= WORLD_HEIGHT_TILES * TILE_WIDTH_PIXELS)
    {
        return Direction_Up;
    }
    return Direction_Stay;
}

// TODO(spencer): prototypes are a resource
void
load_entity_prototypes(mem::GameMem& memory, const char* filepath)
{
    TextResource entity_protos = load_text_resource(filepath);
    auto root_v = parse_json_string(&memory.frame_temp_arena, entity_protos.text);
    assert(root_v->type == JSON_OBJECT && "couldn't parse entity protos");
    auto root = root_v->object;
    char path_buf[256];

    // load entity prototypes
    for (usize type = 0;
         type < EntityType_NumberOfTypes;
         type++)
    {
        const char* key = get_str_for_entity_type((EntityType)type);

        auto jsonv = jsonobj_get(root, key, strlen(key));
        auto obj = jsonv->object;

        auto entity_proto = entity_prototypes + type;

        char sprite_info_buf[256];
        auto entity_sprite_info = jsonobj_get(obj, "sprite_info", 11);
        json_str_copy(sprite_info_buf, entity_sprite_info->string);

        auto checkpoint = memory.frame_temp_arena.checkpoint();
        auto arena = memory.frame_temp_arena.alloc_sub_arena(64 * ONE_PAGE);
        std::cout << "Loading info from '" << sprite_info_buf << "'" << std::endl;
        auto anim = get_or_load_anim_resource(&arena, sprite_info_buf);

        memory.frame_temp_arena.restore_zeroed(checkpoint);

        auto entity_sprite = jsonobj_get(obj, "spritesheet", 11);
        json_str_copy(path_buf, entity_sprite->string);

        auto collider_width = jsonobj_get(obj, "collider_width", 14)->number->value;
        auto collider_height = jsonobj_get(obj, "collider_height", 15)->number->value;
        Rectangle entity_collider { 0, 0, collider_width, collider_height };

        // TODO
        auto resource = get_or_load_image_resource(path_buf, anim->n_frames);
        entity_proto->spritesheet = resource;
        entity_proto->animation_id = anim->id;
        entity_proto->collider_dims = entity_collider;
        entity_proto->new_sprite_id = render::default_atlas_push_sprite(resource.width, resource.height, resource.data);
    }
}

WorldChunk* load_all_world_chunks(mem::GameMem& memory)
{
    // TODO: we should be getting chunk indexes from a resource
    // archive of some sort. We're nearing the point where we're
    // ready to start crafting a format for that.
    WorldChunk* starting_chunk = load_world_chunk(memory, "resource/map/testoutdoor.tmj");
    starting_chunk->level_index = 0;

    WorldChunk* other_chunk = load_world_chunk(memory, "resource/map/next.tmj");
    other_chunk->level_index = 1;

    return starting_chunk;
}

// TODO(spencer): This is half-baked.
void switch_world_chunk(mem::GameMem& mem, GameState* state, Direction dir)
{
    auto active_chunk = state->active_world_chunk;
    auto current_coord = active_chunk->overworld_coords;
    auto new_coord = current_coord + dir_to_vec(dir);
    auto overworld_dims = state->overworld_dims;
    u32 next_chunk_idx = (new_coord.y * overworld_dims.x) + new_coord.x;
    assert(next_chunk_idx < state->overworld_grid.length && "Overflow overworld");
    auto next_chunk = state->overworld_grid.items[next_chunk_idx];

    auto player = active_chunk->entities + active_chunk->player_id;
    if (next_chunk->player_id == ENTITY_ID_NONE)
    {
        next_chunk->player_id = next_chunk->add_entity(mem, player->type, player->position);
    }

    auto other_player = next_chunk->entities + next_chunk->player_id;
    other_player->state = player->state;
    other_player->position = player->position;
    other_player->velocity = player->velocity;
    other_player->acceleration = player->acceleration;
    other_player->facing_dir = player->facing_dir;

    switch (dir)
    {
        case Direction_Up:
        {
            other_player->position.y = other_player->position.y - (WORLD_HEIGHT_TILES * TILE_WIDTH_PIXELS);
        } break;
        case Direction_Right:
        {
            other_player->position.x = other_player->position.x - (WORLD_WIDTH_TILES * TILE_WIDTH_PIXELS);
        } break;
        case Direction_Down:
        {
            other_player->position.y = other_player->position.y + (WORLD_HEIGHT_TILES * TILE_WIDTH_PIXELS);
        } break;
        case Direction_Left:
        {
            other_player->position.x = other_player->position.x + (WORLD_WIDTH_TILES * TILE_WIDTH_PIXELS);
        } break;
        default:
            break;
    }
    //????
    if (other_player->position.x < 0)
    {
        other_player->position.x = 0;
    }

    state->active_world_chunk = next_chunk;
}

struct OverworldLevel
{
    JsonString *file_name;
    i32 x;
    i32 y;
};

void
load_stage(mem::GameMem& memory, const char* stage_dir, GameState* state)
{
    char filepath_buf[256];

    snprintf(filepath_buf, 256, "%s/stage_1.world", stage_dir);
    auto world_json = parse_json_file(&memory.frame_temp_arena, filepath_buf);

    auto map_arr = jsonobj_get(world_json->object, "maps", 4)->obj_array;

    auto map_arr_head = map_arr;
    u32 count = 0;
    while (map_arr_head)
    {
        count++;
        map_arr_head = map_arr_head->next;
    }

    auto level_list = mem::make_simple_list<OverworldLevel>(count, &memory.frame_temp_arena);

    map_arr_head = map_arr;
    m::Vec2 x_range {0, 0};
    m::Vec2 y_range {0, 0};
    while (map_arr_head)
    {
        auto new_level = mem::simple_list_append_new(&level_list);
        auto obj = map_arr_head->obj;
        new_level->file_name = jsonobj_get(obj, "fileName", 8)->string;
        new_level->x = jsonobj_get(obj, "x", 1)->number->value / 320;
        new_level->y = jsonobj_get(obj, "y", 1)->number->value / 184;

        if (new_level->x < x_range.x) 
        {
            x_range.x = new_level->x;
        }
        else if (new_level->x > x_range.y)
        {
            x_range.y = new_level->x;
        }

        if (new_level->y < y_range.x) 
        {
            y_range.x = new_level->y;
        }
        else if (new_level->y > y_range.y)
        {
            y_range.y = new_level->y;
        }

        map_arr_head = map_arr_head->next;
    }

    m::Vec2 overworld_dims {x_range.y - x_range.x + 1, y_range.y - y_range.x + 1};

    auto overworld = mem::make_simple_list<WorldChunk*>(overworld_dims.x * overworld_dims.y, &memory.stage_arena);

    WorldChunk* first_world_chunk = nullptr;
    for (u32 i = 0; i < level_list.length; i++)
    {
        auto level = level_list.items + i;
        auto level_coord = m::Vec2 { level->x + m::abs(x_range.x), -level->y + m::abs(y_range.x) };
        u32 level_idx = (level_coord.y * overworld_dims.x) + level_coord.x;
        assert(level_idx < overworld.capacity && "index out of bounds");

        auto n_printed = snprintf(filepath_buf, 256, "%s/", stage_dir);
        json_str_copy(filepath_buf + n_printed, level->file_name);

        std::cout << "Loading " << filepath_buf << " at " << level_coord << std::endl;
        overworld.items[level_idx] = load_world_chunk(memory, filepath_buf);
        overworld.items[level_idx]->overworld_coords = level_coord;
        if (level->x == 0 && level->y == 0)
        {
            first_world_chunk = overworld.items[level_idx];
        }

        auto map = overworld.items[level_idx]->active_map;
        tilemap_set_up_and_buffer(map, &memory.frame_temp_arena);
        tilemap_set_up_and_buffer(map->background, &memory.frame_temp_arena);
        tilemap_set_up_and_buffer(map->decoration, &memory.frame_temp_arena);
    }

    overworld.length = overworld.capacity; // lmao bad choice
    state->overworld_dims = overworld_dims;
    state->overworld_grid = overworld;
    state->first_world_chunk = first_world_chunk;
    state->active_world_chunk = first_world_chunk;
}

GameState*
load_game(mem::GameMem& memory)
{
    GameState* result = initialize_game_state(memory);

    load_entity_prototypes(memory, "resource/entity/entities.json");

    load_stage(memory, "resource/tiled/stage_1", result);
    //result->first_world_chunk = load_all_world_chunks(memory);
    //result->active_world_chunk = result->first_world_chunk;

    return result;
}

// TODO(spencer): will eventually need an overworld map
int level_index = 0;

void
simulate_one_tick(mem::GameMem& memory, GameState* game_state, f32 dt, render::BatchBuffer* entity_batch_buffer)
{
    auto world_chunk = game_state->active_world_chunk;

    // TODO: use game_state->player_id
    auto player = game_state->active_world_chunk->entities + game_state->active_world_chunk->player_id;

    auto dir = check_for_level_change(player);
    if (dir != Direction_Stay)
    {
        switch_world_chunk(memory, game_state, dir);
    }

    auto trigger = world_chunk->zone_triggers + 0;

    if (trigger->id > 0)
    {
        debug::push_rect_outline(trigger->rect, {0.0, 1.0, 0.0});
        if (test_ZoneTrigger(game_state, trigger))
        {
        auto effect_map = global_effects_map[trigger->target_effect];
        effect_map.fn(&memory, game_state, trigger->target_id, trigger->effect_data);
        }
    }

    EntityIterator entity_iter(world_chunk);

    TileMap* active_map = game_state->active_world_chunk->active_map;

    for (auto entity = entity_iter.begin();
         entity != entity_iter.end();
         entity = entity_iter.next())
    {
        // TODO(spencer): entity type? Or do we want concepts of controllers/brains that we can
        // attach to an entity? That sounds kinda nice, tbh.
        switch (entity->type)
        {
            case EntityType_Player:
            {
                m::Vec3 new_acc = {0};

                f32 gravity = -1000; // TODO: grav
                if (entity->state == STATE_ON_LAND)
                {
                    gravity = 0;
                }
                new_acc.y += gravity;

                const auto is_in_air = entity->state == STATE_FALLING || entity->state == STATE_JUMPING;
                const auto player_speed = is_in_air ? 400 : 700;
                if (g_input_state.move_left_requested)
                {
                    new_acc.x -= player_speed;
                }

                if (g_input_state.move_right_requested)
                {
                    new_acc.x += player_speed;
                }

                bool is_requesting_move = g_input_state.move_left_requested || g_input_state.move_right_requested;
                if (m::abs(entity->velocity.x) > 0 && !is_requesting_move)
                {
                    new_acc.x -= m::signof(entity->velocity.x) * player_speed;

                    if (m::abs(new_acc.x * dt) > m::abs(entity->velocity.x))
                    {
                        new_acc.x = 0;
                        entity->velocity.x = 0;
                    }
                }

                if (g_input_state.jump_requested)
                {
                    if (state_transition_land_to_jump(entity))
                    {
                        entity_set_animation(entity, "jump");
                    }
                    g_input_state.jump_requested = false;
                    entity->velocity.y = 230;
                }

                entity->acceleration = new_acc;

                auto move_result = move_entity(entity, active_map, dt, 140);

                if (entity->velocity.y <= 0)
                {
                    i32 down_row = 2 * 3;
                    i32 down_col = 1;
                    i32 i = down_row + down_col;
                    bool ground_below = move_result.collided[i];

                    if (ground_below)
                    {
                        if (state_transition_air_to_land(entity))
                        {
                            entity_set_animation(entity, "idle");
                        }
                    }
                    else
                    {
                        if (state_transition_fall_exclusive(entity))
                        {
                            entity_set_animation(entity, "jump");
                        }
                    }
                }

                if (entity->state == STATE_ON_LAND)
                {
                    if (m::abs(entity->velocity.x) > 0.3) // ????
                    {
                        entity_update_animation(entity, "walk");
                    }
                    else
                    {
                        entity_update_animation(entity, "idle");
                    }
                }

                update_zero_cross_trigger(&entity->facing_dir, entity->velocity.x);

                auto animation = get_anim_resource(entity->animations_id);
                auto current_frame = entity->animation.current_frame;
                auto frame = animation->frames + current_frame;

                auto player_sprite = render::push_render_item<render::SpriteItem>(entity_batch_buffer);
                player_sprite->position = entity->display_position;
                player_sprite->sprite_id = entity->new_sprite_id;
                player_sprite->color_and_strength = {0, 0, 0, 0};
                player_sprite->sprite_segment_min = frame->spritesheet_min;
                player_sprite->sprite_segment_max = frame->spritesheet_max;
                // TODO: would be nice if we had something better for this zero cross trigger business
                if (entity->facing_dir.last_observed_sign < 0)
                {
                    auto tmp = player_sprite->sprite_segment_min.x;
                    player_sprite->sprite_segment_min.x = player_sprite->sprite_segment_max.x;
                    player_sprite->sprite_segment_max.x = tmp;
                }

            } break;

            case EntityType_Bumpngo:
            {
                m::Vec3 new_accel;
                f32 grav = -600;
                if (entity->state == STATE_ON_LAND)
                {
                    grav = 0;
                }
                new_accel.y = grav;
                auto dir = entity->facing_dir.last_observed_sign;
                if (dir == 0)
                {
                    dir = 1;
                }

                // accelerate effectively instantly
                new_accel.x = 1500 * dir;

                entity->acceleration = new_accel;
                auto move_result = move_entity(entity, active_map, dt, 20);

                if (entity->velocity.y <= 0)
                {
                    i32 down_row = 2 * 3;
                    i32 down_col = 1;
                    i32 i = down_row + down_col;
                    bool ground_below = move_result.collided[i];

                    if (ground_below)
                    {
                        if (state_transition_air_to_land(entity))
                        {
                            entity_set_animation(entity, "idle");
                        }
                    }
                    else
                    {
                        if (state_transition_fall_exclusive(entity))
                        {
                            entity_set_animation(entity, "jump");
                        }
                    }
                }
                if (entity->state == STATE_ON_LAND)
                {
                    if (m::abs(entity->velocity.x) > 0.2)
                    {
                        entity_update_animation(entity, "walk");
                    }
                    else
                    {
                        entity_update_animation(entity, "idle");
                    }
                }

                i32 leftright = 4 + dir;
                if (move_result.collided[leftright])
                {
                    update_zero_cross_trigger(&entity->facing_dir, -dir);
                    entity->velocity.x = 0;
                }

            } break;
            default:
            {
            } break;

        }

    }

}

void
update_animations(WorldChunk* active_chunk, f32 dt)
{
    for (i32 eid = 0;
         eid < active_chunk->next_free_entity_idx;
         eid++)
    {
        auto e = active_chunk->entities + eid;
        e->animation.timer += dt;
        if (e->animation.timer >= e->animation.threshold)
        {
            e->animation.timer = 0;
            e->animation.current_frame++;
            if (e->animation.current_frame >= e->animation.end_frame)
            {
                e->animation.current_frame = e->animation.start_frame;
            }
        }
    }
}

} // namespace rigel
