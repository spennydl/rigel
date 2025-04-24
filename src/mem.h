#ifndef RIGEL_MEM_H
#define RIGEL_MEM_H

#include "rigel.h"
#include <cassert>
#include <utility>

typedef intptr_t mem_ptr;
typedef uint8_t byte_ptr;


namespace rigel {
namespace mem {

typedef usize ArenaCheckpoint;

inline usize
align_sz(usize sz, usize align)
{
    assert(align > 0);
    return (sz + align - 1) & ~(align - 1);
}

struct Arena
{
    usize arena_bytes;
    usize next_free_idx;
    byte_ptr* mem_begin;

    Arena() : arena_bytes(0), next_free_idx(0), mem_begin(nullptr) {}

    Arena(byte_ptr* memory, usize size)
      : arena_bytes(size)
      , next_free_idx(0)
      , mem_begin(memory)
    {
    }

    void reinit() { next_free_idx = 0; }

    void reinit_zeroed()
    {
        for (usize i = 0; i < next_free_idx; i++)
        {
            mem_begin[i] = 0;
        }
        next_free_idx = 0;
    }

    Arena alloc_sub_arena(usize size)
    {
        auto base = next_free_idx;
        // we should always gurantee arenas start word-aligned
        auto start = align_sz(base, sizeof(mem_ptr));
        auto end = align_sz(start + size, sizeof(mem_ptr));

        assert(end < arena_bytes && "Tried to alloc sub-arena bigger than its parent");

        next_free_idx = end;
        return Arena(mem_begin + start, end - start);
    }

    inline byte_ptr* alloc_bytes(usize bytes, usize align_to = 1)
    {
        auto start = align_sz(next_free_idx, align_to);
        auto end = align_sz(start + bytes, align_to);

        assert (end < arena_bytes && "Overflowed memory arena!");

        next_free_idx = end;

        return mem_begin + start;
    }

    template<typename T>
    inline T* alloc_array(int n)
    {
        return reinterpret_cast<T*>(alloc_bytes(sizeof(T) * n, alignof(T)));
    }

    template<typename T>
    inline T* alloc_simple()
    {
        auto ptr = alloc_bytes(sizeof(T), alignof(T));

        if (!ptr)
            return nullptr;

        return reinterpret_cast<T*>(ptr);
    }

    template<typename T, typename... Args>
    inline T* alloc_obj(Args&&... args)
    {
        T* ptr = alloc_simple<T>();

        if (!ptr)
            return nullptr;

        return new (ptr) T(std::forward<Args>(args)...);
    }

    inline ArenaCheckpoint checkpoint()
    {
        return next_free_idx;
    }

    inline void restore(ArenaCheckpoint checkpoint)
    {
        next_free_idx = checkpoint;
    }

    inline void restore_zeroed(ArenaCheckpoint checkpoint)
    {
        assert(checkpoint < next_free_idx && "invalid checkpoint");

        for (usize c = checkpoint; c < next_free_idx; c++)
        {
            mem_begin[c] = 0;
        }

        next_free_idx = checkpoint;
    }

};

struct GameMem
{
    byte_ptr* game_state_storage;
    usize game_state_storage_size;

    byte_ptr* ephemeral_storage;
    usize ephemeral_storage_size;

    Arena game_state_arena;
    Arena ephemeral_arena;
    Arena stage_arena;
    Arena colliders_arena;
    Arena scratch_arena;
    Arena resource_arena;
    Arena gfx_arena;
    Arena debug_arena;
};

} // namespace mem
} // namespace rigel

#endif // RIGEL_MEM_H
