//
// Created by whyiskra on 16.12.23.
//

#include "morphine/object/function.h"
#include "morphine/object/coroutine.h"
#include "morphine/core/throw.h"
#include "morphine/gc/barrier.h"
#include "morphine/gc/allocator.h"
#include "morphine/gc/safe.h"
#include <string.h>

struct function *functionI_create(
    morphine_instance_t I,
    struct uuid uuid,
    size_t name_len,
    size_t constants_count,
    size_t instructions_count,
    size_t statics_count,
    size_t arguments_count,
    size_t slots_count,
    size_t closures_count,
    size_t params_count
) {
    if (name_len > MLIMIT_FUNCTION_NAME) {
        throwI_error(I, "Function name too big");
    }

    struct function *result = allocI_uni(I, NULL, sizeof(struct function));

    (*result) = (struct function) {
        .uuid = uuid,
        .name = NULL,
        .name_len = 0,
        .constants_count = 0,
        .instructions_count = 0,
        .statics_count = 0,
        .arguments_count = arguments_count,
        .slots_count = slots_count,
        .closures_count = closures_count,
        .params_count = params_count,
        .constants = NULL,
        .instructions = NULL,
        .statics = NULL,
        .registry_key = valueI_nil
    };

    objectI_init(I, objectI_cast(result), OBJ_TYPE_FUNCTION);

    size_t rollback = gcI_safe_obj(I, objectI_cast(result));

    result->name = allocI_vec(I, NULL, name_len + 1, sizeof(char));
    result->name_len = name_len;
    memset(result->name, 0, (name_len + 1) * sizeof(char));

    result->instructions = allocI_vec(I, NULL, instructions_count, sizeof(instruction_t));
    result->instructions_count = instructions_count;

    result->constants = allocI_vec(I, NULL, constants_count, sizeof(struct value));
    result->constants_count = constants_count;
    for (size_t i = 0; i < constants_count; i++) {
        result->constants[i] = valueI_nil;
    }

    result->statics = allocI_vec(I, NULL, statics_count, sizeof(struct value));
    result->statics_count = statics_count;
    for (size_t i = 0; i < statics_count; i++) {
        result->statics[i] = valueI_nil;
    }

    gcI_reset_safe(I, rollback);

    return result;
}

void functionI_free(morphine_instance_t I, struct function *function) {
    allocI_free(I, function->name);
    allocI_free(I, function->instructions);
    allocI_free(I, function->constants);
    allocI_free(I, function->statics);
    allocI_free(I, function);
}

void functionI_validate(morphine_instance_t I, struct function *function) {
    if (function == NULL) {
        throwI_error(I, "Function is null");
    }

    for (size_t i = 0; i < function->instructions_count; i++) {
        bool is_valid = instructionI_validate(
            function->instructions[i],
            function->arguments_count,
            function->slots_count,
            function->params_count,
            function->closures_count,
            function->statics_count,
            function->constants_count
        );

        if (!is_valid) {
            throwI_error(I, "Instruction structure corrupted");
        }
    }
}

struct value functionI_static_get(morphine_instance_t I, struct function *function, size_t index) {
    if (function == NULL) {
        throwI_error(I, "Function is null");
    }

    if (index >= function->statics_count) {
        throwI_error(I, "Static index was out of bounce");
    }

    return function->statics[index];
}

void functionI_static_set(
    morphine_instance_t I,
    struct function *function,
    size_t index,
    struct value value
) {
    if (function == NULL) {
        throwI_error(I, "Function is null");
    }

    if (index >= function->statics_count) {
        throwI_error(I, "Static index was out of bounce");
    }

    gcI_barrier(function, value);
    function->statics[index] = value;
}


struct value functionI_constant_get(morphine_instance_t I, struct function *function, size_t index) {
    if (function == NULL) {
        throwI_error(I, "Function is null");
    }

    if (index >= function->constants_count) {
        throwI_error(I, "Constant index was out of bounce");
    }

    return function->constants[index];
}

void functionI_constant_set(
    morphine_instance_t I,
    struct function *function,
    size_t index,
    struct value value
) {
    if (function == NULL) {
        throwI_error(I, "Function is null");
    }

    if (index >= function->constants_count) {
        throwI_error(I, "Constant index was out of bounce");
    }

    gcI_barrier(function, value);
    function->constants[index] = value;
}
