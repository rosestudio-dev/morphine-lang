//
// Created by whyiskra on 25.12.23.
//

#include <string.h>
#include "morphine/api.h"
#include "morphine/core/value.h"
#include "morphine/object/coroutine.h"
#include "morphine/object/string.h"
#include "morphine/core/throw.h"
#include "morphine/misc/metatable.h"

MORPHINE_API void mapi_push_nil(morphine_coroutine_t U) {
    stackI_push(U, valueI_nil);
}

MORPHINE_API void mapi_push_integer(morphine_coroutine_t U, ml_integer value) {
    stackI_push(U, valueI_integer(value));
}

MORPHINE_API void mapi_push_size(morphine_coroutine_t U, size_t value) {
    stackI_push(U, valueI_csize2integer(U->I, value));
}

MORPHINE_API void mapi_push_index(morphine_coroutine_t U, size_t value) {
    stackI_push(U, valueI_csize2indexinteger(U->I, value));
}

MORPHINE_API void mapi_push_decimal(morphine_coroutine_t U, ml_decimal value) {
    stackI_push(U, valueI_decimal(value));
}

MORPHINE_API void mapi_push_boolean(morphine_coroutine_t U, bool value) {
    stackI_push(U, valueI_boolean(value));
}

MORPHINE_API void mapi_push_raw(morphine_coroutine_t U, void *value) {
    stackI_push(U, valueI_raw(value));
}

MORPHINE_API ml_integer mapi_get_integer(morphine_coroutine_t U) {
    return valueI_as_integer_or_error(U->I, stackI_peek(U, 0));
}

MORPHINE_API ml_size mapi_get_size(morphine_coroutine_t U) {
    ml_integer integer = valueI_as_integer_or_error(U->I, stackI_peek(U, 0));
    return valueI_integer2size(U->I, integer);
}

MORPHINE_API ml_size mapi_get_index(morphine_coroutine_t U) {
    ml_integer integer = valueI_as_integer_or_error(U->I, stackI_peek(U, 0));
    return valueI_integer2index(U->I, integer);
}

MORPHINE_API ml_decimal mapi_get_decimal(morphine_coroutine_t U) {
    return valueI_as_decimal_or_error(U->I, stackI_peek(U, 0));
}

MORPHINE_API bool mapi_get_boolean(morphine_coroutine_t U) {
    return valueI_as_boolean_or_error(U->I, stackI_peek(U, 0));
}

MORPHINE_API void *mapi_get_raw(morphine_coroutine_t U) {
    return valueI_as_raw_or_error(U->I, stackI_peek(U, 0));
}

MORPHINE_API const char *mapi_raw_type(morphine_coroutine_t U) {
    return valueI_type2string(U->I, stackI_peek(U, 0).type);
}

MORPHINE_API void mapi_type(morphine_coroutine_t U) {
    struct value value = stackI_peek(U, 0);
    struct value mf_value = valueI_nil;
    struct value result;
    if (metatableI_test(U->I, value, MF_TYPE, &mf_value)) {
        result = valueI_value2string(U->I, mf_value);
    } else {
        result = valueI_object(stringI_create(U->I, valueI_type2string(U->I, value.type)));
    }

    stackI_push(U, result);
}

MORPHINE_API bool mapi_is(morphine_coroutine_t U, const char *type) {
    if (strcmp(type, "callable") == 0) {
        return mapi_is_callable(U);
    }

    if (strcmp(type, "meta") == 0) {
        return mapi_is_metatype(U);
    }

    if (strcmp(type, "iterable") == 0) {
        return mapi_is_iterable(U);
    }

    return mapi_is_type(U, type);
}

MORPHINE_API bool mapi_is_type(morphine_coroutine_t U, const char *type) {
    struct value value = stackI_peek(U, 0);
    struct value mf_value = valueI_nil;
    const char *got_type;
    if (metatableI_test(U->I, value, MF_TYPE, &mf_value)) {
        got_type = valueI_as_string(valueI_value2string(U->I, mf_value))->chars;
    } else {
        got_type = valueI_type2string(U->I, value.type);
    }

    return strcmp(type, got_type) == 0;
}

MORPHINE_API bool mapi_is_callable(morphine_coroutine_t U) {
    struct value value = stackI_peek(U, 0);
    return callstackI_is_callable(U->I, value);
}

MORPHINE_API bool mapi_is_metatype(morphine_coroutine_t U) {
    struct value value = stackI_peek(U, 0);
    return valueI_is_table(value) || valueI_is_userdata(value);
}

MORPHINE_API bool mapi_is_iterable(morphine_coroutine_t U) {
    struct value value = stackI_peek(U, 0);
    return valueI_is_table(value) || valueI_is_vector(value);
}

MORPHINE_API void mapi_to_integer(morphine_coroutine_t U) {
    struct value value = stackI_peek(U, 0);
    struct value result = valueI_value2integer(U->I, value);
    stackI_replace(U, 0, result);
}

MORPHINE_API void mapi_to_decimal(morphine_coroutine_t U) {
    struct value value = stackI_peek(U, 0);
    struct value result = valueI_value2decimal(U->I, value);
    stackI_replace(U, 0, result);
}

MORPHINE_API void mapi_to_boolean(morphine_coroutine_t U) {
    struct value value = stackI_peek(U, 0);
    struct value result = valueI_value2boolean(U->I, value);
    stackI_replace(U, 0, result);
}

MORPHINE_API void mapi_to_string(morphine_coroutine_t U) {
    struct value value = stackI_peek(U, 0);
    struct value result = valueI_value2string(U->I, value);
    stackI_replace(U, 0, result);
}

MORPHINE_API ml_size mapi_csize2size(morphine_coroutine_t U, size_t value) {
    return valueI_csize2size(U->I, value);
}

MORPHINE_API ml_size mapi_csize2index(morphine_coroutine_t U, size_t value) {
    return valueI_csize2index(U->I, value);
}
