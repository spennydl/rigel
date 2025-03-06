#ifndef RIGEL_UTILITY_H_
#define RIGEL_UTILITY_H_

#include "rigel.h"

// could have a macro like this and wrap it in methods of structs that
// can provide identifies for the two tables?
//
// sounds nice to me
#define HASH_GET(hash_key, out_val, map_table_ident, value_table_ident, hash_fn)     \
    do {                                                                       \
        auto idx = hash_fn(hash_key); \
        auto map_entry = map_table_ident + idx;                         \
        out_val = (map_entry-> == resource_id)                \
                          ? loaded_sprites[idx].idx                            \
                          : SPRITE_ID_NONE;                                    \
    } while (0)


template<typename idx_type, typename key_type>
struct hash_map
{
    key_type key;
    idx_type idx;
};

template<typename map_type, typename value_type>
struct hash_table
{
    map_type *map;
    value_type *vals;
}:

#endif // UTILITY_H_
