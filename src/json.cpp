#include "json.h"

#include "rigel.h"
#include "mem.h"
#include "fs_linux.h"

#include <cstdlib>
#include <iostream>

namespace rigel {

bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool is_number(char c)
{
    return (c >= 0x30 && c < 0x40) || c == '-';
}

const char* next_non_ws_char(const char* start) {
    while (is_space(*start)) {
        start++;
    }
    return start;
}

bool json_str_equals(JsonString jstr, const char* str, usize n)
{
    if (jstr.end == jstr.start) return false;

    usize jstr_len = jstr.end - jstr.start;
    if (jstr_len == 0 || jstr_len != n) {
        return false;
    }

    usize i = 0;
    while (i < jstr_len && str[i] != '\0' && i < n) {
        if (jstr.start[i] != str[i]) {
            return false;
        }
        i++;
    }
    return true;
}

const char* extract_number(const char* start)
{
    const char* end = start;
    while (*end != '\0') {
        if (is_space(*end) || *end == ',' || *end == '}' || *end == ']') {
            break;;
        }
        if (*end != '.' && !is_number(*end)) {
            return nullptr;
        }
        end++;
    }
    return end;
}

bool extract_json_number(JsonToken tok, JsonNumber* dst)
{
    assert(tok.type == TOK_NUMBER && "Tried to extract number from non-number token");
    char* end_ptr;
    dst->value = strtof(tok.str.start, &end_ptr);
    return end_ptr == tok.str.end;
}


const char* find_end_of_string(const char* start)
{
    const char* end = start + 1;
    while (*end != '"' && *end != '\0') {
        if (*end == '\'') {
            end++;
        }
        end++;
    }
    if (*end == '\0') {
        return nullptr;
    }
    return end + 1;
}

JsonToken get_next_token(const char* start)
{
    JsonToken result;
    result.type = JSON_TOKEN_UNDEF;
    result.str.start = start;
    result.str.end = start + 1;

    switch (*start)
    {
        case '{':
            result.type = TOK_OPEN_CURLY_BRACE;
            break;
        case '}':
            result.type = TOK_CLOSE_CURLY_BRACE;
            break;
        case '[':
            result.type = TOK_OPEN_SQUARE_BRACE;
            break;
        case ']':
            result.type = TOK_CLOSE_SQUARE_BRACE;
            break;
        case ',':
            result.type = TOK_COMMA;
            break;
        case '"':
            result.type = TOK_STRING;
            result.str.end = find_end_of_string(result.str.start);
            break;
        case '\0':
            result.type = TOK_EOF;
            break;
        case ':':
            result.type = TOK_COLON;
            break;
        default:
            break;
    }

    if (result.type != JSON_TOKEN_UNDEF) {
        return result;
    }

    if (is_number(*result.str.start)) {
        const char* end = extract_number(result.str.start);
        if (end) {
            result.type = TOK_NUMBER;
            result.str.end = end;
            return result;
        }
    }

    while (*result.str.end != '\0') {
        if (json_str_equals(result.str, "true", 4)) {
            result.type = TOK_TRUE;
            std::cout << result.str.start[0] << "/" << result.str.end[0] << std::endl;
            return result;
        }

        if (json_str_equals(result.str, "false", 5)) {
            result.type = TOK_FALSE;
            std::cout << result.str.start[0] << "/" << result.str.end[0] << std::endl;
            return result;
        }

        if (json_str_equals(result.str, "null", 4)) {
            result.type = TOK_NULL;
            return result;
        }

        result.str.end += 1;
    }

    return result;
}

JsonObj* parse_json_obj(mem::Arena* work_mem, const char** obj_str);
bool parse_json_array(JsonValue* value, mem::Arena* work_mem, const char** obj_str)
{
    JsonToken tok;
    const char* cursor = *obj_str;
    tok = get_next_token(*obj_str);

    if (tok.type == TOK_CLOSE_SQUARE_BRACE) {
        // empty array
        value->type = JSON_STRING_ARRAY;
        value->string_array = work_mem->alloc_simple<JsonArray<JsonString>>();
        value->string_array->arr = nullptr;
        value->string_array->n = 0;
        *obj_str = next_non_ws_char(tok.str.end);
        return true;
    }

    if (tok.type == TOK_STRING) {
        // string array
        usize n = 0;
        value->type = JSON_STRING_ARRAY;
        value->string_array = work_mem->alloc_simple<JsonArray<JsonString>>();
        value->string_array->arr = work_mem->alloc_simple<JsonString>();
        value->string_array->arr[n] = tok.str;
        n += 1;

        while (true) {
            cursor = next_non_ws_char(tok.str.end);
            tok = get_next_token(cursor);
            if (tok.type == TOK_CLOSE_SQUARE_BRACE) {
                break;
            }
            if (tok.type != TOK_COMMA) {
                std::cout << "Err: expected comma between string array members" << std::endl;
                return false;
            }

            cursor = next_non_ws_char(tok.str.end);
            tok = get_next_token(cursor);
            if (tok.type != TOK_STRING) {
                std::cout << "Err: expected array of strings, got type " << tok.type << std::endl;
                return false;
            }
            work_mem->alloc_simple<JsonString>();
            value->string_array->arr[n] = tok.str;
            n += 1;

        }
        value->string_array->n = n;
        *obj_str = next_non_ws_char(tok.str.end);
        return true;
    }

    if (tok.type == TOK_NUMBER) {
        // string array
        usize n = 0;
        value->type = JSON_NUMBER_ARRAY;
        value->number_array = work_mem->alloc_simple<JsonArray<JsonNumber>>();
        value->number_array->arr = work_mem->alloc_simple<JsonNumber>();

        if (!extract_json_number(tok, value->number_array->arr + n)) {
            std::cout << "err: malformed number" << std::endl;
            return false;
        }
        n += 1;

        while (true) {
            cursor = next_non_ws_char(tok.str.end);
            tok = get_next_token(cursor);
            if (tok.type == TOK_CLOSE_SQUARE_BRACE) {
                break;
            }
            if (tok.type != TOK_COMMA) {
                std::cout << "Err: expected comma between string array members" << std::endl;
                return false;
            }

            cursor = next_non_ws_char(tok.str.end);
            tok = get_next_token(cursor);
            if (tok.type != TOK_NUMBER) {
                std::cout << "Err: expected array of numbers, got type " << tok.type << std::endl;
                return false;
            }
            work_mem->alloc_simple<JsonNumber>();
            if (!extract_json_number(tok, value->number_array->arr + n)) {
                std::cout << "err: malformed number" << std::endl;
                return false;
            }
            n += 1;
        }
        value->number_array->n = n;
        *obj_str = next_non_ws_char(tok.str.end);
        return true;
    }

    // same for bool but whatevs ya know

    if (tok.type == TOK_OPEN_CURLY_BRACE) {
        // objects boooo
        value->type = JSON_OBJECT_ARRAY;
        value->obj_array = work_mem->alloc_simple<JsonObjArray>();
        auto head = value->obj_array;

        while (true)
        {
            head->obj = parse_json_obj(work_mem, &cursor);
            if (head->obj == nullptr) {
                return false;
            }
            cursor = next_non_ws_char(cursor);
            tok = get_next_token(cursor);
            if (tok.type == TOK_CLOSE_SQUARE_BRACE) {
                break;
            }
            if (tok.type != TOK_COMMA) {
                std::cout << "Err: objects in array should be separated by a comma" << std::endl;
                return false;
            }
            cursor = next_non_ws_char(tok.str.end);
            tok = get_next_token(cursor);
            if (tok.type != TOK_OPEN_CURLY_BRACE) {
                std::cout << "Err: expected '{' but got token type " << tok.type << std::endl;
                return false;
            }
            head->next = work_mem->alloc_simple<JsonObjArray>();
            head = head->next;
        }
        *obj_str = next_non_ws_char(tok.str.end);
        return true;
    }

    std::cout << "Err: got token of type " << tok.type << " but its corresponding type does not support arrays" << std::endl;
    std::cout << "get a better json parser." << std::endl;
    return false;
}

JsonObj* parse_json_obj(mem::Arena* work_mem, const char** obj_str)
{
    JsonToken tok;
    const char* cursor = next_non_ws_char(*obj_str);
    tok = get_next_token(*obj_str);
    assert(tok.type == TOK_OPEN_CURLY_BRACE && "malformed json obj");

    JsonObj* result = work_mem->alloc_simple<JsonObj>();
    result->kvps = nullptr;
    JsonKVP** tail = &(result->kvps);

    //cursor = next_non_ws_char(tok.str.end);
    //tok = get_next_token(cursor);

    while (tok.type != TOK_CLOSE_CURLY_BRACE) {
        cursor = next_non_ws_char(tok.str.end);
        tok = get_next_token(cursor);
        // TODO: empty object case here
        if (tok.type != TOK_STRING) {
            std::cout << "Unexpected token type " << tok.type << std::endl;
            return nullptr;
        }
        *tail = work_mem->alloc_simple<JsonKVP>();
        (*tail)->next = nullptr;
        (*tail)->key = tok.str;

        cursor = next_non_ws_char(tok.str.end);
        tok = get_next_token(cursor);
        if (tok.type != TOK_COLON) {
            std::cout << "Err: expected ':'" << std::endl;
            return nullptr;
        }

        cursor = next_non_ws_char(tok.str.end);
        tok = get_next_token(cursor);
        if (tok.type == TOK_OPEN_CURLY_BRACE) {
            (*tail)->value.type = JSON_OBJECT;
            (*tail)->value.object = parse_json_obj(work_mem, &cursor);
        }

        if (tok.type == TOK_STRING) {
            (*tail)->value.type = JSON_STRING;
            (*tail)->value.string = work_mem->alloc_simple<JsonString>();
            *(*tail)->value.string = tok.str;
            cursor = next_non_ws_char(tok.str.end);
        }

        if (tok.type == TOK_NUMBER) {
            (*tail)->value.type = JSON_NUMBER;
            // TODO: make a proper number type
            (*tail)->value.number = work_mem->alloc_simple<JsonNumber>();
            if (!extract_json_number(tok, (*tail)->value.number)) {
                std::cout << "err: malformed number" << std::endl;
                return nullptr;
            }
            cursor = tok.str.end;
        }

        if (tok.type == TOK_TRUE || tok.type == TOK_FALSE) {
            (*tail)->value.type = JSON_BOOL;
            (*tail)->value.jbool = work_mem->alloc_simple<bool>();
            *(*tail)->value.jbool = tok.type == TOK_TRUE;
            cursor = tok.str.end;
        }

        if (tok.type == TOK_NULL) {
            (*tail)->value.type = JSON_NULL;
            cursor = tok.str.end;
        }

        if (tok.type == TOK_OPEN_SQUARE_BRACE) {
            cursor = next_non_ws_char(tok.str.end);
            if (!parse_json_array(&(*tail)->value, work_mem, &cursor)) {
                return nullptr;
            }
        }

        cursor = next_non_ws_char(cursor);
        tok = get_next_token(cursor);
        if (tok.type != TOK_COMMA && tok.type != TOK_CLOSE_CURLY_BRACE) {
            // TODO: error locations
            std::cout << "Err: expected comma or end of object but got " << tok.type << std::endl;
            return nullptr;
        }
        tail = &(*tail)->next;
    }

    *obj_str = tok.str.end;
    return result;
}

JsonValue* parse_json_file(mem::Arena* work_mem, const char* file_name)
{
    const char* buffer = (const char*)slurp_into_mem(work_mem, file_name);

    const char* cursor = buffer;
    JsonValue* result = work_mem->alloc_simple<JsonValue>();

    //JsonValue* value = work_mem->alloc_simple<JsonValue>();
    JsonToken tok;
    cursor = next_non_ws_char(cursor);
    tok = get_next_token(cursor);

    switch (tok.type) {
        case TOK_OPEN_CURLY_BRACE:
            result->type = JSON_OBJECT;
            result->object = parse_json_obj(work_mem, &cursor);
            if (result->object == nullptr) {
                return nullptr;
            }
            break;
        case TOK_OPEN_SQUARE_BRACE:
            if (!parse_json_array(result, work_mem, &cursor)) {
                return nullptr;
            }
            break;
        default:
            std::cout << "Expect doc to start with either an object or an array" << std::endl;
            return nullptr;
            break;
    }

    cursor = next_non_ws_char(cursor);
    tok = get_next_token(cursor);
    if (tok.type != TOK_EOF) {
        std::cout << "Expect only one object or array in a file, found another token of type " << tok.type << std::endl;
        return nullptr;
    }

    return result;
}

}

std::ostream& operator<<(std::ostream& os, rigel::JsonString& js)
{
    const char* s = js.start;
    const char* e = js.end;
    while (s != e) {
        os << *s;
        s++;
    }
    return os;
}

