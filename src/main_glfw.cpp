#include "entity.h"
#include "game.h"
#include "mem.h"
#include "phys.h"
#include "collider.h"
#include "render.h"
#include "rigel.h"
#include "world.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sys/mman.h>

#include "stb_image.h"

#define ONE_PAGE 4096

/*
TODO:
- render collision quads as loaded from the level - DONE
- implement a player as a quad that we can move - DONE
- add basic AABB collision to the player (for now) - DONE
- basic resource management
  - memory! - DONE
- render the level
- Reassess:
   - quadtree, probably
   - Collider types: AABB, OBB, SAT for convex shapes
*/
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

    memory.ephemeral_storage_size = 4 * ONE_PAGE;
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

    memory.stage_arena = memory.ephemeral_arena.alloc_sub_arena(1024);
    memory.colliders_arena = memory.ephemeral_arena.alloc_sub_arena(1024);
    memory.scratch_arena = memory.ephemeral_arena.alloc_sub_arena(1024);

    std::cout << "memory map:" << std::endl;
    std::cout << "game state: " << (mem_ptr*)memory.game_state_arena.mem_begin << " for " << memory.game_state_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "ephemeral: " << (mem_ptr*)memory.ephemeral_arena.mem_begin << " for " << memory.ephemeral_arena.arena_bytes << " bytes" << std::endl;

    std::cout << "stage: " << (mem_ptr*)memory.stage_arena.mem_begin << " for " << memory.stage_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "colliders: " << (mem_ptr*)memory.colliders_arena.mem_begin << " for " << memory.colliders_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "scratch: " << (mem_ptr*)memory.scratch_arena.mem_begin << " for " << memory.scratch_arena.arena_bytes << " bytes" << std::endl;
    return memory;
}

void
glfw_error_callback(int error, const char* description)
{
    std::cerr << "Error: " << description << std::endl;
}

enum InputActionState
{
    INPUT_STATE_NONE = 0,
    INPUT_PRESS,
    INPUT_HELD,
    INPUT_RELEASE,
    N_INPUT_STATES
};

enum InputAction
{
    ACTION_RIGHT = 0,
    ACTION_LEFT,
    ACTION_UP,
    ACTION_DOWN,
    N_ACTIONS
};

struct KeyInputListener
{
    static bool should_close;

    static InputActionState input_actions[N_ACTIONS];

    static void key_callback(GLFWwindow* window,
                             int key,
                             int scancode,
                             int action,
                             int mods)
    {
        if (action == GLFW_REPEAT) {
            // don't handle these
            return;
        }
        InputAction input_action = N_ACTIONS;
        switch (key) {
            case GLFW_KEY_ESCAPE: {
                KeyInputListener::should_close = action == GLFW_PRESS;
                return;
            }
            case GLFW_KEY_A:
                input_action = ACTION_LEFT;
                break;
            case GLFW_KEY_S:
                input_action = ACTION_DOWN;
                break;
            case GLFW_KEY_D:
                input_action = ACTION_RIGHT;
                break;
            case GLFW_KEY_W:
                input_action = ACTION_UP;
                break;
            default: {
                return;
            }
        }
        InputActionState new_state = INPUT_STATE_NONE;
        if (action == GLFW_PRESS) {
            new_state = INPUT_PRESS;
        } else if (action == GLFW_RELEASE) {
            new_state = INPUT_RELEASE;
        }
        input_actions[input_action] = new_state;
    }

    static void new_frame()
    {
        for (int i = 0; i < N_ACTIONS; i++) {
            if (input_actions[i] == INPUT_PRESS) {
                input_actions[i] = INPUT_HELD;
            } else if (input_actions[i] == INPUT_RELEASE) {
                input_actions[i] = INPUT_STATE_NONE;
            }
        }
    }

    static void dump()
    {
        for (int i = 0; i < N_ACTIONS; i++) {
            if (input_actions[i] != INPUT_STATE_NONE) {
                std::cout << "key " << i << " is " << input_actions[i]
                          << std::endl;
            }
        }
    }
};
InputActionState KeyInputListener::input_actions[N_ACTIONS];
bool KeyInputListener::should_close = false;

std::string
slurp(std::ifstream& in)
{
    std::ostringstream sstr;
    sstr << in.rdbuf();
    return sstr.str();
}

GameState*
initialize_game_state(mem::GameMem& memory)
{
    GameState* state = memory.game_state_arena.alloc_simple<GameState>();
    return state;
}

int
main()
{
    if (!glfwInit()) {
        std::cerr << "failed to init glfw oops" << std::endl;
        return 1;
    }
    glfwSetErrorCallback(glfw_error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window =
      glfwCreateWindow(1280, 720, "haburrrr", nullptr, nullptr);
    if (!window) {
        std::cerr << "failed to create window oops" << std::endl;
        glfwTerminate();
        return 1;
        // Window or context creation failed
    }
    // glfwSetWindowMonitor(window, nullptr, 0, 0, 1280, 720, GLFW_DONT_CARE);
    glfwSetKeyCallback(window, KeyInputListener::key_callback);

    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1); // vsync

    mem::GameMem memory = initialize_game_memory();
    rigel::GameState* game_state = initialize_game_state(memory);
    game_state->active_world_chunk = load_world_chunk(memory, "obj.tmx");

    render::SpriteAtlas* sprite_atlas =
      memory.ephemeral_arena.alloc_obj<render::SpriteAtlas>();
    int w, h, channels;
    auto image_data = stbi_load("testpersonsprite.png", &w, &h, &channels, 4);
    assert(image_data && "image data didn't load");
    auto res_id = sprite_atlas->load_sprite(1, w, h, image_data);

    Rectangle player_collider = { .x = 0, .y = 0, .w = 16, .h = 16 };
    auto world_chunk = game_state->active_world_chunk;
    world_chunk->add_player(
      memory, res_id, glm::vec3(80, 32, 0), player_collider);

    std::ifstream spritevs("vs_sprite.glsl");
    auto sprite_vsrc = slurp(spritevs);
    std::ifstream spritefs("fs_sprite.glsl");
    auto sprite_fsrc = slurp(spritefs);
    render::SpriteRenderStep sprite_step;
    sprite_step.sprite_shader.load_from_src(sprite_vsrc, sprite_fsrc);
    sprite_step.atlas = sprite_atlas;

    // TODO: resource manager
    // OR: make 'em static.
    std::ifstream vs("vs_quad.glsl");
    auto vs_src = slurp(vs);
    std::ifstream fs("fs_quad.glsl");
    auto fs_src = slurp(fs);
    render::Shader shader(vs_src, fs_src);

    // TODO: renderingggggggggggggggggg
    std::vector<render::Quad> render_quads;
    // This is bad, but it shouldn't be our access pattern.
    auto stage = game_state->active_world_chunk->active_stage;
    auto collider_set = stage->colliders.aabbs;
    auto n_colliders = stage->colliders.n_aabbs;

    stage->colliders.n_tris = 1;
    stage->colliders.tris = memory.colliders_arena.alloc_array<CollisionTri>(1);
    stage->colliders.tris[0].vertices[0] = glm::vec3(128, 32, 0);
    stage->colliders.tris[0].vertices[1] = glm::vec3(176, 48, 0);
    stage->colliders.tris[0].vertices[2] = glm::vec3(176, 32, 0);

    render_quads.reserve(n_colliders);
    for (int i = 0; i < n_colliders; i++) {
        auto& quad = collider_set[i];
        render_quads.push_back(
          render::Quad(rect_from_aabb(quad), shader));
    }

    double target_spf = 1.0 / 60.0;
    double dt = 0.0;
    double render_dt = 0.0;
    double frame_start = 0;
    double last_frame_start = 0;

    render::Viewport viewport;
    render::VectorRenderer vector_renderer;
    viewport.zoom(2.0);
    auto player = &world_chunk->entities[0];

    while (!glfwWindowShouldClose(window)) {
        double frame_start = glfwGetTime();
        dt = frame_start - last_frame_start;

        KeyInputListener::new_frame();

        glfwPollEvents();

        if (KeyInputListener::should_close) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        // player brain
        {
            // TODO:
            auto player = &world_chunk->entities[0];
            player->forces = glm::vec3(0, 0, 0);
            // auto player =
            // game_state->active_world_chunk->get_entity(player_id);

            // input first
            if (KeyInputListener::input_actions[ACTION_RIGHT] == INPUT_PRESS ||
                KeyInputListener::input_actions[ACTION_RIGHT] == INPUT_HELD) {
                player->forces.x += rigel::PLAYER_XACCEL;
            } else if (KeyInputListener::input_actions[ACTION_LEFT] ==
                         INPUT_PRESS ||
                       KeyInputListener::input_actions[ACTION_LEFT] ==
                         INPUT_HELD) {
                player->forces.x -= rigel::PLAYER_XACCEL;
            }

            if (KeyInputListener::input_actions[ACTION_UP] == INPUT_PRESS) {
                if (!(player->state_flags & entity::STATE_JUMPING)) {
                    // TODO we need to manually give a velocity kick here
                    player->velocity.y += rigel::PLAYER_JUMP;
                    // TODO: gotta encapsulate these state changes somewhere
                    player->state_flags |= entity::STATE_JUMPING;
                    player->state_flags = player->state_flags & (~entity::STATE_ON_LAND);
                }
            }
            // that's it, for now. Huh.
        }

        phys::integrate_entity_positions(world_chunk, (float)dt);
        phys::resolve_stage_collisions(world_chunk, &world_chunk->active_stage->colliders, &memory.scratch_arena);
        // update end here

        if (render_dt >= target_spf) {
            // TODO: viewport stuff could be done only on resize
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            glViewport(0, 0, width, height);

            glClear(GL_COLOR_BUFFER_BIT);

            for (int q = 0; q < render_quads.size(); q++) {
                if (stage->colliders.aabbs[q].is_colliding) {
                    render::render_quad(render_quads[q], viewport, 0xff, 0x77, 0x88);
                } else {
                    render::render_quad(render_quads[q], viewport, 0x66, 0x77, 0x88);
                }
            }

            sprite_step.render_entities_in_world(
              &viewport, game_state->active_world_chunk);

            glfwSwapBuffers(window);

            // TODO: could be better way than this, eh?
            while (render_dt >= target_spf) {
                render_dt -= target_spf;
            }
        } else {
            render_dt += dt;
        }
        last_frame_start = frame_start;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
