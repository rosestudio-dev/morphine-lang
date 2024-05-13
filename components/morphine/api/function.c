//
// Created by whyiskra on 25.12.23.
//

#include "morphine/api.h"
#include "morphine/object/function/loader.h"
#include "morphine/object/function.h"
#include "morphine/object/coroutine.h"
#include "morphine/utils/unused.h"
#include "morphine/core/throw.h"
#include <stddef.h>

struct loader_array {
    size_t size;
    size_t pointer;
    const uint8_t *vector;
};

static void *loader_array_open(morphine_instance_t I, void *data) {
    struct loader_array *array = cast(struct loader_array *, data);

    if (array->size == 0 || array->vector == NULL) {
        throwI_error(I, "Binary vector is empty");
    }

    array->pointer = 0;

    return array;
}

static uint8_t loader_array_read(morphine_instance_t I, void *data, const char **error) {
    unused(I);

    struct loader_array *loader = cast(struct loader_array *, data);

    if (loader->pointer >= loader->size) {
        *error = "Binary corrupted";
        return 0;
    }

    return loader->vector[loader->pointer++];
}

MORPHINE_API void mapi_push_function(morphine_coroutine_t U, size_t size, const uint8_t *vector) {
    struct loader_array array = {
        .vector = vector,
        .size = size
    };

    struct function *result = loaderI_load(U, loader_array_open, loader_array_read, NULL, cast(void *, &array));
    stackI_push(U, valueI_object(result));
}

MORPHINE_API void mapi_function_load(
    morphine_coroutine_t U,
    morphine_init_t init,
    morphine_read_t read,
    morphine_finish_t finish,
    void *args
) {
    struct function *result = loaderI_load(U, init, read, finish, args);
    stackI_push(U, valueI_object(result));
}

MORPHINE_API void mapi_static_get(morphine_coroutine_t U, size_t index) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 0));
    struct value value = functionI_static_get(U->I, function, index);
    stackI_push(U, value);
}

MORPHINE_API void mapi_static_set(morphine_coroutine_t U, size_t index) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 1));
    functionI_static_set(U->I, function, index, stackI_peek(U, 0));
    stackI_pop(U, 1);
}

MORPHINE_API size_t mapi_static_size(morphine_coroutine_t U) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 1));
    return function->statics_count;
}

MORPHINE_API void mapi_constant_get(morphine_coroutine_t U, size_t index) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 0));
    struct value value = functionI_constant_get(U->I, function, index);
    stackI_push(U, value);
}

MORPHINE_API void mapi_constant_set(morphine_coroutine_t U, size_t index) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 1));
    functionI_constant_set(U->I, function, index, stackI_peek(U, 0));
    stackI_pop(U, 1);
}

MORPHINE_API size_t mapi_constant_size(morphine_coroutine_t U) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 1));
    return function->constants_count;
}
