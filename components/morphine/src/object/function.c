//
// Created by whyiskra on 16.12.23.
//

#include "morphine/object/function.h"
#include "morphine/object/coroutine.h"
#include "morphine/object/string.h"
#include "morphine/core/throw.h"
#include "morphine/gc/barrier.h"
#include "morphine/gc/allocator.h"
#include "morphine/gc/safe.h"
#include "morphine/misc/instruction.h"
#include <string.h>

struct function *functionI_create(
    morphine_instance_t I,
    struct string *name,
    ml_line line,
    ml_size constants_count,
    ml_size instructions_count,
    ml_size statics_count,
    ml_size arguments_count,
    ml_size slots_count,
    ml_size params_count
) {
    if (name == NULL) {
        throwI_error(I, "function name is null");
    }

    if (instructions_count > MLIMIT_ARGUMENT_MAX) {
        throwI_error(I, "too many instructions");
    }

    if (constants_count > MLIMIT_ARGUMENT_MAX) {
        throwI_error(I, "too many constants");
    }

    if (statics_count > MLIMIT_ARGUMENT_MAX) {
        throwI_error(I, "too many statics");
    }

    if (slots_count > MLIMIT_ARGUMENT_MAX) {
        throwI_error(I, "too many slots");
    }

    if (params_count > MLIMIT_ARGUMENT_MAX) {
        throwI_error(I, "too many params");
    }

    if (arguments_count > MLIMIT_CALLABLE_ARGS) {
        throwI_error(I, "too many args");
    }

    size_t rollback = gcI_safe_obj(I, objectI_cast(name));

    // create
    struct function *result = allocI_uni(I, NULL, sizeof(struct function));
    (*result) = (struct function) {
        .complete = false,
        .name = name,
        .line = line,
        .constants_count = 0,
        .instructions_count = 0,
        .statics_count = 0,
        .arguments_count = arguments_count,
        .slots_count = slots_count,
        .params_count = params_count,
        .constants = NULL,
        .instructions = NULL,
        .statics = NULL,
    };

    objectI_init(I, objectI_cast(result), OBJ_TYPE_FUNCTION);

    // config
    gcI_safe_obj(I, objectI_cast(result));

    result->instructions = allocI_vec(I, NULL, instructions_count, sizeof(morphine_instruction_t));
    result->instructions_count = instructions_count;
    for (size_t i = 0; i < instructions_count; i++) {
        result->instructions[i] = (morphine_instruction_t) {
            .line = 0,
            .opcode = MORPHINE_OPCODE_YIELD,
            .argument1 = 0,
            .argument2 = 0,
            .argument3 = 0
        };
    }

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
    allocI_free(I, function->instructions);
    allocI_free(I, function->constants);
    allocI_free(I, function->statics);
    allocI_free(I, function);
}

void functionI_complete(morphine_instance_t I, struct function *function) {
    if (function == NULL) {
        throwI_error(I, "function is null");
    }

    function->complete = true;
}

morphine_instruction_t functionI_instruction_get(
    morphine_instance_t I,
    struct function *function,
    ml_size index
) {
    if (function == NULL) {
        throwI_error(I, "function is null");
    }

    if (index >= function->instructions_count) {
        throwI_error(I, "instruction index was out of bounce");
    }

    return function->instructions[index];
}

void functionI_instruction_set(
    morphine_instance_t I,
    struct function *function,
    ml_size index,
    morphine_instruction_t instruction
) {
    if (function == NULL) {
        throwI_error(I, "function is null");
    }

    if (function->complete) {
        throwI_error(I, "function is complete");
    }

    if (index >= function->instructions_count) {
        throwI_error(I, "instruction index was out of bounce");
    }

    for (size_t i = instructionI_opcode_args(instruction.opcode, NULL);
         i < MORPHINE_INSTRUCTION_ARGS_COUNT; i++) {
        instruction.arguments[i] = 0;
    }

    bool is_valid = instructionI_validate(instruction, function);

    if (!is_valid) {
        throwI_error(I, "instruction structure corrupted");
    }

    function->instructions[index] = instruction;
}

struct value functionI_constant_get(
    morphine_instance_t I,
    struct function *function,
    ml_size index
) {
    if (function == NULL) {
        throwI_error(I, "function is null");
    }

    if (index >= function->constants_count) {
        throwI_error(I, "constant index was out of bounce");
    }

    return function->constants[index];
}

void functionI_constant_set(
    morphine_instance_t I,
    struct function *function,
    ml_size index,
    struct value value
) {
    if (function == NULL) {
        throwI_error(I, "function is null");
    }

    if (function->complete) {
        throwI_error(I, "function is complete");
    }

    if (index >= function->constants_count) {
        throwI_error(I, "constant index was out of bounce");
    }

    bool supported = valueI_is_nil(value) ||
                     valueI_is_integer(value) ||
                     valueI_is_decimal(value) ||
                     valueI_is_boolean(value) ||
                     valueI_is_string(value) ||
                     valueI_is_function(value);

    if (!supported) {
        throwI_error(I, "unsupported constant type");
    }

    gcI_barrier(I, function, value);
    function->constants[index] = value;
}

struct value functionI_static_get(
    morphine_instance_t I,
    struct function *function,
    ml_size index
) {
    if (function == NULL) {
        throwI_error(I, "function is null");
    }

    if (index >= function->statics_count) {
        throwI_error(I, "static index was out of bounce");
    }

    return function->statics[index];
}

void functionI_static_set(
    morphine_instance_t I,
    struct function *function,
    ml_size index,
    struct value value
) {
    if (function == NULL) {
        throwI_error(I, "function is null");
    }

    if (index >= function->statics_count) {
        throwI_error(I, "static index was out of bounce");
    }

    gcI_barrier(I, function, value);
    function->statics[index] = value;
}
