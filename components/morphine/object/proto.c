//
// Created by whyiskra on 16.12.23.
//

#include "morphine/object/proto.h"
#include "morphine/object/state.h"
#include "morphine/core/allocator.h"
#include "morphine/core/throw.h"
#include "morphine/gc/barrier.h"
#include <string.h>

bool protoI_uuid_equal(struct uuid a, struct uuid b) {
    return (a.most_significant_bits == b.most_significant_bits) &&
           (a.least_significant_bits == b.least_significant_bits);
}

struct proto *protoI_create(
    morphine_instance_t I,
    struct uuid uuid,
    size_t name_chars_count,
    size_t constants_count,
    size_t instructions_count,
    size_t statics_count
) {
    struct proto *result = allocI_uni(I, NULL, sizeof(struct proto));

    size_t name_size = sizeof(char) * (name_chars_count + 1);
    size_t constants_size = sizeof(struct value) * constants_count;
    size_t instructions_size = sizeof(instruction_t) * instructions_count;
    size_t statics_size = sizeof(struct value) * statics_count;

    (*result) = (struct proto) {
        .uuid = uuid,
        .name = allocI_uni(I, NULL, name_size),
        .name_chars_count = name_chars_count,
        .constants_count = constants_count,
        .instructions_count = instructions_count,
        .statics_count = statics_count,
        .arguments_count = 0,
        .slots_count = 0,
        .params_count = 0,
        .constants = allocI_uni(I, NULL, constants_size),
        .instructions = allocI_uni(I, NULL, instructions_size),
        .statics = allocI_uni(I, NULL, statics_size),
        .registry_key = valueI_nil
    };

    for (size_t i = 0; i < constants_count; i++) {
        result->constants[i] = valueI_nil;
    }

    for (size_t i = 0; i < statics_count; i++) {
        result->statics[i] = valueI_nil;
    }

    result->name[name_chars_count] = '\0';

    objectI_init(I, objectI_cast(result), OBJ_TYPE_PROTO);

    return result;
}

void protoI_free(morphine_instance_t I, struct proto *proto) {
    allocI_free(I, proto->name);
    allocI_free(I, proto->constants);
    allocI_free(I, proto->instructions);
    allocI_free(I, proto->statics);
    allocI_free(I, proto);
}

size_t protoI_allocated_size(struct proto *proto) {
    size_t proto_size = sizeof(struct proto);
    size_t name_size = sizeof(char) * (proto->name_chars_count + 1);
    size_t constants_size = sizeof(struct value) * proto->constants_count;
    size_t instructions_size = sizeof(instruction_t) * proto->instructions_count;
    size_t statics_size = sizeof(struct value) * proto->statics_count;

    return proto_size + name_size + instructions_size + constants_size + statics_size;
}

struct value protoI_static_get(morphine_state_t S, struct proto *proto, size_t index) {
    if (proto == NULL) {
        throwI_message_panic(S->I, S, "Proto is null");
    }

    if (index >= proto->statics_count) {
        throwI_message_error(S, "Static index was out of bounce");
    }

    return proto->statics[index];
}

void protoI_static_set(morphine_state_t S, struct proto *proto, size_t index, struct value value) {
    if (proto == NULL) {
        throwI_message_panic(S->I, S, "Proto is null");
    }

    if (index >= proto->statics_count) {
        throwI_message_error(S, "Static index was out of bounce");
    }

    gcI_barrier(proto, value);
    proto->statics[index] = value;
}


struct value protoI_constant_get(morphine_state_t S, struct proto *proto, size_t index) {
    if (proto == NULL) {
        throwI_message_panic(S->I, S, "Proto is null");
    }

    if (index >= proto->constants_count) {
        throwI_message_error(S, "Constant index was out of bounce");
    }

    return proto->constants[index];
}

void protoI_constant_set(morphine_state_t S, struct proto *proto, size_t index, struct value value) {
    if (proto == NULL) {
        throwI_message_panic(S->I, S, "Proto is null");
    }

    if (index >= proto->constants_count) {
        throwI_message_error(S, "Constant index was out of bounce");
    }

    gcI_barrier(proto, value);
    proto->constants[index] = value;
}
