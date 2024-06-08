//
// Created by whyiskra on 25.12.23.
//

#include "morphine/api.h"
#include "morphine/object/function.h"
#include "morphine/object/coroutine.h"
#include "morphine/core/throw.h"
#include "morphine/misc/loader.h"
#include "morphine/instruction.h"

MORPHINE_API void mapi_push_function(morphine_coroutine_t U) {
    struct sio *sio = valueI_as_sio_or_error(U->I, stackI_peek(U, 0));
    struct function *result = loaderI_load(U, sio);
    stackI_push(U, valueI_object(result));
}

MORPHINE_API void mapi_function_complete(morphine_coroutine_t U) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 0));
    functionI_complete(U->I, function);
}

MORPHINE_API bool mapi_function_is_complete(morphine_coroutine_t U) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 0));
    return function->complete;
}

MORPHINE_API void mapi_static_get(morphine_coroutine_t U, ml_size index) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 0));
    struct value value = functionI_static_get(U->I, function, index);
    stackI_push(U, value);
}

MORPHINE_API void mapi_static_set(morphine_coroutine_t U, ml_size index) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 1));
    functionI_static_set(U->I, function, index, stackI_peek(U, 0));
    stackI_pop(U, 1);
}

MORPHINE_API ml_size mapi_static_size(morphine_coroutine_t U) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 0));
    return function->statics_count;
}

MORPHINE_API void mapi_constant_get(morphine_coroutine_t U, ml_size index) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 0));
    struct value value = functionI_constant_get(U->I, function, index);
    stackI_push(U, value);
}

MORPHINE_API void mapi_constant_set(morphine_coroutine_t U, ml_size index) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 1));
    functionI_constant_set(U->I, function, index, stackI_peek(U, 0));
    stackI_pop(U, 1);
}

MORPHINE_API ml_size mapi_constant_size(morphine_coroutine_t U) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 0));
    return function->constants_count;
}

MORPHINE_API morphine_instruction_t mapi_instruction_get(morphine_coroutine_t U, ml_size index) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 0));
    return functionI_instruction_get(U->I, function, index);
}

MORPHINE_API void mapi_instruction_set(
    morphine_coroutine_t U,
    ml_size index,
    morphine_instruction_t instruction
) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 0));
    functionI_instruction_set(U->I, function, index, instruction);
}

MORPHINE_API ml_size mapi_instruction_size(morphine_coroutine_t U) {
    struct function *function = valueI_as_function_or_error(U->I, stackI_peek(U, 0));
    return function->instructions_count;
}