#include "rigel.h"
#include "mem.h"
#include "world.h"
#include "render.h"

#include <SDL3/SDL.h>
#include <glad/gl.h>
#include <iostream>
#include <sys/mman.h>

namespace rigel {

// TODO: what is this and where does it go
struct GameState
{
    WorldChunk* active_world_chunk;
};

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

    memory.ephemeral_storage_size = 4 * ONE_MB;
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
    memory.scratch_arena = memory.ephemeral_arena.alloc_sub_arena(1024);
    memory.resource_arena = memory.ephemeral_arena.alloc_sub_arena(2 * ONE_MB);

    std::cout << "memory map:" << std::endl;
    std::cout << "game state: " << (mem_ptr*)memory.game_state_arena.mem_begin << " for " << memory.game_state_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "ephemeral: " << (mem_ptr*)memory.ephemeral_arena.mem_begin << " for " << memory.ephemeral_arena.arena_bytes << " bytes" << std::endl;

    std::cout << "stage: " << (mem_ptr*)memory.stage_arena.mem_begin << " for " << memory.stage_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "colliders: " << (mem_ptr*)memory.colliders_arena.mem_begin << " for " << memory.colliders_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "scratch: " << (mem_ptr*)memory.scratch_arena.mem_begin << " for " << memory.scratch_arena.arena_bytes << " bytes" << std::endl;
    std::cout << "resource: " << (mem_ptr*)memory.resource_arena.mem_begin << " for " << memory.resource_arena.arena_bytes << " bytes" << std::endl;
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
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("rigel", 1280, 720, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    int version = gladLoadGL((GLADloadfunc) SDL_GL_GetProcAddress);
    std::cout << "Loaded version " << version << std::endl;

    mem::GameMem memory = initialize_game_memory();

    resource_initialize(memory.resource_arena);

    GameState* game_state = initialize_game_state(memory);
    game_state->active_world_chunk = load_world_chunk(memory);

    ImageResource tilesheet = load_image_resource(memory.resource_arena, "resource/image/tranquil_tunnels_green_packed.png");

    TextResource map_shader_vs = load_text_resource(memory.resource_arena, "resource/shader/vs_map.glsl");
    TextResource map_shader_fs = load_text_resource(memory.resource_arena, "resource/shader/fs_map.glsl");
    render::Shader map_shader(map_shader_vs.text, map_shader_fs.text);

    render::SpriteAtlas atlas(tilesheet);
    render::MapDrawLayer map_draw(atlas, map_shader);

    map_draw.buffer_map(game_state->active_world_chunk->active_map);

    //dump_tile_map(game_state->active_world_chunk->active_map);

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        glClearColor(1.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        render::render_map(map_draw);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
