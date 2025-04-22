#ifndef RIGEL_JSON_H
#define RIGEL_JSON_H

#include "rigel.h"
#include "mem.h"

#include <iostream>

namespace rigel {

struct JsonString {
    const char* start;
    const char* end;
};
bool json_str_equals(const JsonString jstr, const char* str, usize n);
bool json_str_equals(const JsonString lhs, const JsonString rhs);

enum JsonTokenType {
    JSON_TOKEN_UNDEF,
    TOK_OPEN_CURLY_BRACE,
    TOK_CLOSE_CURLY_BRACE,
    TOK_OPEN_SQUARE_BRACE,
    TOK_CLOSE_SQUARE_BRACE,
    TOK_COMMA,
    TOK_DOUBLE_QUOTE,
    TOK_BACKSLASH_ESC,
    TOK_STRING,
    TOK_NUMBER,
    TOK_COLON,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NULL,
    TOK_EOF,
    N_JSON_TOKEN_TYPES
};

struct JsonToken {
    JsonTokenType type;
    JsonString str;
};

enum JsonValueType {
    JSON_OBJECT,
    JSON_OBJECT_ARRAY,
    JSON_NUMBER,
    JSON_NUMBER_ARRAY,
    JSON_STRING,
    JSON_STRING_ARRAY,
    JSON_BOOL,
    JSON_BOOL_ARRAY,
    JSON_NULL,
    N_SUPPORTED_JSON_OBJS
};

template <typename T>
struct JsonArray
{
    usize n;
    T* arr;
};

struct JsonNumber {
    f32 value;
};

struct JsonObj;
struct JsonObjArray
{
    JsonObj* obj;
    JsonObjArray* next;
};

struct JsonValue {
    JsonValueType type;
    union {
        JsonObj* object;
        JsonString* string;
        JsonNumber* number;
        JsonArray<JsonString>* string_array;
        JsonArray<JsonNumber>* number_array;
        JsonObjArray* obj_array;
        JsonArray<bool> bool_array;
        bool* jbool;
    };
};

struct JsonObjEntry
{
    JsonString key;
    JsonValue value;
};

#define N_HASHES 512
struct JsonObj {
    JsonObjEntry entries[N_HASHES];
    u32 count;
};

inline u64
dbj2(const JsonString s)
{
    u64 hash = 5381;
    const char* start = s.start;

    while (start < s.end)
    {
        hash = ((hash << 5) + hash) * *start;
        start++;
    }
    return hash;
}

inline u64
dbj2(const char* str, usize len)
{
    u64 hash = 5381;
    for (usize i = 0; i < len && str[i]; i++)
    {
        hash = ((hash << 5) + hash) * str[i];
    }
    return hash;
}
JsonValue* jsonobj_get(JsonObj* obj, const char* key, usize key_len);

JsonValue* parse_json_string(mem::Arena* work_mem, const char* json_str);
JsonValue* parse_json_file(mem::Arena* work_mem, const char* file_name);

} // namespace rigel

std::ostream& operator<<(std::ostream& os, rigel::JsonString& js);

#endif // RIGEL_JSON_H
