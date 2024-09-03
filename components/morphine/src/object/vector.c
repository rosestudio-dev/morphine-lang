//
// Created by why-iskra on 22.04.2024.
//

#include <memory.h>
#include "morphine/object/vector.h"
#include "morphine/object/string.h"
#include "morphine/core/throw.h"
#include "morphine/gc/allocator.h"
#include "morphine/gc/safe.h"
#include "morphine/gc/barrier.h"
#include "morphine/params.h"
#include "morphine/utils/overflow.h"

struct vector *vectorI_create(morphine_instance_t I, ml_size size) {
    struct vector *result = allocI_uni(I, NULL, sizeof(struct vector));

    (*result) = (struct vector) {
        .mode.fixed = true,
        .mode.mutable = true,
        .mode.locked = false,
        .size.real = 0,
        .size.accessible = 0,
        .values = NULL,
    };

    objectI_init(I, objectI_cast(result), OBJ_TYPE_VECTOR);

    size_t rollback = gcI_safe_obj(I, objectI_cast(result));

    result->values = allocI_vec(I, NULL, size, sizeof(struct value));
    result->size.accessible = size;
    result->size.real = size;

    for (size_t i = 0; i < size; i++) {
        result->values[i] = valueI_nil;
    }

    gcI_reset_safe(I, rollback);

    return result;
}

void vectorI_free(morphine_instance_t I, struct vector *vector) {
    allocI_free(I, vector->values);
    allocI_free(I, vector);
}

void vectorI_mode_fixed(morphine_instance_t I, struct vector *vector, bool is_fixed) {
    if (vector == NULL) {
        throwI_error(I, "vector is null");
    }

    if (vector->mode.locked) {
        throwI_error(I, "vector is locked");
    }

    if (!vector->mode.fixed && is_fixed) {
        vector->values = allocI_vec(I, vector->values, vector->size.accessible, sizeof(struct value));
        vector->size.real = vector->size.accessible;
    }

    vector->mode.fixed = is_fixed;
}

void vectorI_mode_mutable(morphine_instance_t I, struct vector *vector, bool is_mutable) {
    if (vector == NULL) {
        throwI_error(I, "vector is null");
    }

    if (vector->mode.locked) {
        throwI_error(I, "vector is locked");
    }

    vector->mode.mutable = is_mutable;
}

void vectorI_mode_lock(morphine_instance_t I, struct vector *vector) {
    if (vector == NULL) {
        throwI_error(I, "vector is null");
    }

    vector->mode.locked = true;
}

ml_size vectorI_size(morphine_instance_t I, struct vector *vector) {
    if (vector == NULL) {
        throwI_error(I, "vector is null");
    }

    return vector->size.accessible;
}

struct value vectorI_get(morphine_instance_t I, struct vector *vector, ml_size index) {
    if (vector == NULL) {
        throwI_error(I, "vector is null");
    }

    if (index >= vector->size.accessible) {
        throwI_error(I, "vector index out of bounce");
    }

    return vector->values[index];
}

void vectorI_set(morphine_instance_t I, struct vector *vector, ml_size index, struct value value) {
    if (vector == NULL) {
        throwI_error(I, "vector is null");
    }

    if (!vector->mode.mutable) {
        throwI_error(I, "vector is immutable");
    }

    if (index >= vector->size.accessible) {
        throwI_error(I, "vector index out of bounce");
    }

    gcI_barrier(I, vector, value);
    vector->values[index] = value;
}

void vectorI_add(morphine_instance_t I, struct vector *vector, ml_size index, struct value value) {
    if (vector == NULL) {
        throwI_error(I, "vector is null");
    }

    if (!vector->mode.mutable) {
        throwI_error(I, "vector is immutable");
    }

    if (vector->mode.fixed) {
        throwI_error(I, "vector is fixed");
    }

    if (index > vector->size.accessible) {
        throwI_error(I, "vector index out of bounce");
    }

    if (vector->size.accessible == vector->size.real) {
        size_t rollback = gcI_safe_value(I, value);
        overflow_add(MPARAM_VECTOR_AMORTIZATION, vector->size.real, MLIMIT_SIZE_MAX) {
            throwI_error(I, "vector size limit has been exceeded");
        }

        vector->values = allocI_vec(
            I,
            vector->values,
            vector->size.real + MPARAM_VECTOR_AMORTIZATION,
            sizeof(struct value)
        );

        vector->size.real += MPARAM_VECTOR_AMORTIZATION;
        gcI_reset_safe(I, rollback);
    }

    gcI_barrier(I, vector, value);

    vector->size.accessible++;
    for (ml_size i = 0; i < vector->size.accessible - 1 - index; i++) {
        ml_size from = vector->size.accessible - i - 2;
        ml_size to = vector->size.accessible - i - 1;
        vector->values[to] = vector->values[from];
    }

    vector->values[index] = value;
}

struct value vectorI_remove(morphine_instance_t I, struct vector *vector, ml_size index) {
    if (vector == NULL) {
        throwI_error(I, "vector is null");
    }

    if (!vector->mode.mutable) {
        throwI_error(I, "vector is immutable");
    }

    if (vector->mode.fixed) {
        throwI_error(I, "vector is fixed");
    }

    if (index >= vector->size.accessible) {
        throwI_error(I, "vector index out of bounce");
    }

    struct value value = vector->values[index];

    for (size_t i = index; i < vector->size.accessible - 1; i++) {
        vector->values[i] = vector->values[i + 1];
    }

    vector->size.accessible--;

    if (vector->size.real - vector->size.accessible > MPARAM_VECTOR_AMORTIZATION) {
        size_t rollback = gcI_safe_value(I, value);
        if (MPARAM_VECTOR_AMORTIZATION > vector->size.real) {
            throwI_error(I, "vector size exceeded limit");
        }

        vector->values = allocI_vec(
            I,
            vector->values,
            vector->size.real - MPARAM_VECTOR_AMORTIZATION,
            sizeof(struct value)
        );

        vector->size.real -= MPARAM_VECTOR_AMORTIZATION;
        gcI_reset_safe(I, rollback);
    }

    return value;
}

void vectorI_resize(morphine_instance_t I, struct vector *vector, ml_size size) {
    if (vector == NULL) {
        throwI_error(I, "vector is null");
    }

    if (!vector->mode.mutable) {
        throwI_error(I, "vector is immutable");
    }

    if (vector->mode.fixed) {
        throwI_error(I, "vector is fixed");
    }

    vector->values = allocI_vec(
        I,
        vector->values,
        size,
        sizeof(struct value)
    );

    if (size > vector->size.accessible) {
        for (ml_size i = vector->size.accessible; i < size; i++) {
            vector->values[i] = valueI_nil;
        }
    }

    vector->size.accessible = size;
    vector->size.real = size;
}

struct vector *vectorI_copy(morphine_instance_t I, struct vector *vector) {
    if (vector == NULL) {
        throwI_error(I, "vector is null");
    }

    ml_size size = vector->size.accessible;

    struct vector *result = vectorI_create(I, size);
    result->mode.fixed = vector->mode.fixed;
    result->mode.mutable = vector->mode.mutable;

    memcpy(result->values, vector->values, ((size_t) size) * sizeof(struct value));

    for (ml_size i = 0; i < size; i++) {
        gcI_barrier(I, result, result->values[i]);
    }

    return result;
}

struct value vectorI_first(morphine_instance_t I, struct vector *vector, bool *has) {
    if (vector == NULL) {
        throwI_error(I, "vector is null");
    }

    if (has != NULL) {
        (*has) = vector->size.accessible > 0;
    }

    if (vector->size.accessible == 0) {
        return valueI_nil;
    }

    return valueI_integer(0);
}

struct pair vectorI_next(
    morphine_instance_t I,
    struct vector *vector,
    struct value *key,
    bool *next
) {
    if (vector == NULL) {
        throwI_error(I, "vector is null");
    }

    if (key == NULL) {
        if (next != NULL) {
            (*next) = false;
        }

        return valueI_pair(valueI_nil, valueI_nil);
    }

    ml_size index = valueI_integer2index(I, valueI_as_integer_or_error(I, *key));

    if (index >= vector->size.accessible) {
        if (next != NULL) {
            (*next) = false;
        }

        (*key) = valueI_nil;
        return valueI_pair(valueI_nil, valueI_nil);
    }

    bool has_next = (index + 1) < vector->size.accessible;
    if (next != NULL) {
        (*next) = has_next;
    }

    if (has_next) {
        (*key) = valueI_size(valueI_csize2index(I, index + 1));
    } else {
        (*key) = valueI_nil;
    }

    return valueI_pair(valueI_size(index), vector->values[index]);
}
