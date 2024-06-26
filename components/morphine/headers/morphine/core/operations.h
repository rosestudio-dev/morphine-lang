//
// Created by whyiskra on 02.12.23.
//

#pragma once

#include "morphine/misc/metatable.h"
#include "morphine/object/table.h"
#include "morphine/object/string.h"
#include "morphine/object/reference.h"
#include "morphine/object/iterator.h"
#include "morphine/object/coroutine.h"
#include "morphine/object/vector.h"
#include "morphine/core/throw.h"
#include "morphine/gc/safe.h"
#include "morphine/utils/unused.h"

typedef enum {
    NORMAL,
    CALLED,
    CALLED_COMPLETE
} op_result_t;

static inline op_result_t interpreter_fun_iterator(
    morphine_coroutine_t U,
    size_t callstate,
    struct value container,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    struct value mt_field;
    if (metatableI_test(U->I, container, MF_ITERATOR, &mt_field)) {
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, container, NULL, 0, pop_size);
        return CALLED;
    }

    (*result) = valueI_object(iteratorI_create(U->I, container));
    return NORMAL;
}

static inline op_result_t interpreter_fun_iterator_init(
    morphine_coroutine_t U,
    size_t callstate,
    struct value iterator,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        return CALLED_COMPLETE;
    }

    struct value mt_field;
    if (likely(valueI_is_iterator(iterator))) {
        iteratorI_init(U->I, valueI_as_iterator(iterator));
        return NORMAL;
    } else if (metatableI_test(U->I, iterator, MF_ITERATOR_INIT, &mt_field)) {
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, iterator, NULL, 0, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Cannot init iterator");
}

static inline op_result_t interpreter_fun_iterator_has(
    morphine_coroutine_t U,
    size_t callstate,
    struct value iterator,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    struct value mt_field;
    if (likely(valueI_is_iterator(iterator))) {
        bool has = iteratorI_has(U->I, valueI_as_iterator(iterator));
        (*result) = valueI_boolean(has);
        return NORMAL;
    } else if (metatableI_test(U->I, iterator, MF_ITERATOR_HAS, &mt_field)) {
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, iterator, NULL, 0, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Cannot check next value of iterator");
}

static inline op_result_t interpreter_fun_iterator_next(
    morphine_coroutine_t U,
    size_t callstate,
    struct value iterator,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    struct value mt_field;
    if (likely(valueI_is_iterator(iterator))) {
        struct pair pair = iteratorI_next(U->I, valueI_as_iterator(iterator));
        struct table *table = tableI_create(U->I);
        size_t rollback = gcI_safe_obj(U->I, objectI_cast(table));

        tableI_set(U->I, table, valueI_integer(0), pair.key);
        tableI_set(U->I, table, valueI_integer(1), pair.value);

        (*result) = valueI_object(table);
        gcI_reset_safe(U->I, rollback);
        return NORMAL;
    } else if (metatableI_test(U->I, iterator, MF_ITERATOR_NEXT, &mt_field)) {
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, iterator, NULL, 0, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Cannot get next value of iterator");
}

static inline op_result_t interpreter_fun_get(
    morphine_coroutine_t U,
    size_t callstate,
    struct value container,
    struct value key,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    struct value mt_field;
    if (likely(valueI_is_table(container))) {
        bool has = false;
        (*result) = tableI_get(U->I, valueI_as_table(container), key, &has);

        if (!has && metatableI_test(U->I, container, MF_GET, &mt_field)) {
            struct value new_args[] = { key };
            callstackI_continue(U, callstate);
            callstackI_call_unsafe(U, mt_field, container, new_args, 1, pop_size);
            return CALLED;
        }

        return NORMAL;
    } else if(valueI_is_vector(container)) {
        ml_size index = valueI_integer2index(U->I, valueI_as_integer_or_error(U->I, key));
        (*result) = vectorI_get(U->I, valueI_as_vector(container), index);
        return NORMAL;
    } else if (metatableI_test(U->I, container, MF_GET, &mt_field)) {
        struct value new_args[] = { key };
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, container, new_args, 1, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Get supports only table or vector");
}

static inline op_result_t interpreter_fun_set(
    morphine_coroutine_t U,
    size_t callstate,
    struct value container,
    struct value key,
    struct value value,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        return CALLED_COMPLETE;
    }

    struct value mt_field;
    if(valueI_is_vector(container)) {
        ml_size index = valueI_integer2index(U->I, valueI_as_integer_or_error(U->I, key));
        vectorI_set(U->I, valueI_as_vector(container), index, value);
        return NORMAL;
    } else if (metatableI_test(U->I, container, MF_SET, &mt_field)) {
        struct value new_args[] = { key, value };
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, container, new_args, 2, pop_size);
        return CALLED;
    }

    tableI_set(U->I, valueI_as_table_or_error(U->I, container), key, value);
    return NORMAL;
}

static inline op_result_t interpreter_fun_add(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value b,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (likely(valueI_is_integer(a) && valueI_is_integer(b))) {
        (*result) = valueI_integer(valueI_as_integer(a) + valueI_as_integer(b));
        return NORMAL;
    }

    if (valueI_is_decimal(a) && valueI_is_decimal(b)) {
        (*result) = valueI_decimal(valueI_as_decimal(a) + valueI_as_decimal(b));
        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_ADD, &mt_field)) {
        struct value new_args[] = { b };
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, new_args, 1, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Add supports only integer or decimal");
}

static inline op_result_t interpreter_fun_sub(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value b,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (likely(valueI_is_integer(a) && valueI_is_integer(b))) {
        (*result) = valueI_integer(valueI_as_integer(a) - valueI_as_integer(b));
        return NORMAL;
    }

    if (valueI_is_decimal(a) && valueI_is_decimal(b)) {
        (*result) = valueI_decimal(valueI_as_decimal(a) - valueI_as_decimal(b));
        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_SUB, &mt_field)) {
        struct value new_args[] = { b };
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, new_args, 1, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Sub supports only integer or decimal");
}

static inline op_result_t interpreter_fun_mul(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value b,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (likely(valueI_is_integer(a) && valueI_is_integer(b))) {
        (*result) = valueI_integer(valueI_as_integer(a) * valueI_as_integer(b));
        return NORMAL;
    }

    if (valueI_is_decimal(a) && valueI_is_decimal(b)) {
        (*result) = valueI_decimal(valueI_as_decimal(a) * valueI_as_decimal(b));
        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_MUL, &mt_field)) {
        struct value new_args[] = { b };
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, new_args, 1, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Mul supports only integer or decimal");
}

static inline op_result_t interpreter_fun_div(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value b,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (likely(valueI_is_integer(a) && valueI_is_integer(b))) {
        if (unlikely(valueI_as_integer(b) == 0)) {
            throwI_error(U->I, "Attempt to divide by zero");
        }

        (*result) = valueI_integer(valueI_as_integer(a) / valueI_as_integer(b));

        return NORMAL;
    }

    if (valueI_is_decimal(a) && valueI_is_decimal(b)) {
        if (unlikely(valueI_as_decimal(b) == 0)) {
            throwI_error(U->I, "Attempt to divide by zero");
        }

        (*result) = valueI_decimal(valueI_as_decimal(a) / valueI_as_decimal(b));

        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_DIV, &mt_field)) {
        struct value new_args[] = { b };
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, new_args, 1, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Div supports only integer or decimal");
}

static inline op_result_t interpreter_fun_mod(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value b,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (likely(valueI_is_integer(a) && valueI_is_integer(b))) {
        if (unlikely(valueI_as_integer(b) == 0)) {
            throwI_error(U->I, "Attempt to divide by zero");
        }

        (*result) = valueI_integer(valueI_as_integer(a) % valueI_as_integer(b));

        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_MOD, &mt_field)) {
        struct value new_args[] = { b };
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, new_args, 1, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Mod supports only integer");
}

static inline op_result_t interpreter_fun_equal(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value b,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_EQUAL, &mt_field)) {
        struct value new_args[] = { b };
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, new_args, 1, pop_size);
        return CALLED;
    }

    (*result) = valueI_boolean(valueI_equal(U->I, a, b));
    return NORMAL;
}

static inline op_result_t interpreter_fun_less(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value b,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (likely(valueI_is_integer(a) && valueI_is_integer(b))) {
        (*result) = valueI_boolean(valueI_as_integer(a) < valueI_as_integer(b));
        return NORMAL;
    }

    if (valueI_is_decimal(a) && valueI_is_decimal(b)) {
        (*result) = valueI_boolean(valueI_as_decimal(a) < valueI_as_decimal(b));
        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_LESS, &mt_field)) {
        struct value new_args[] = { b };
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, new_args, 1, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Less supports only integer or decimal");
}

static inline op_result_t interpreter_fun_less_equal(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value b,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (likely(valueI_is_integer(a) && valueI_is_integer(b))) {
        (*result) = valueI_boolean(valueI_as_integer(a) <= valueI_as_integer(b));
        return NORMAL;
    }

    if (valueI_is_decimal(a) && valueI_is_decimal(b)) {
        (*result) = valueI_boolean(valueI_as_decimal(a) <= valueI_as_decimal(b));
        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_LESS_EQUAL, &mt_field)) {
        struct value new_args[] = { b };
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, new_args, 1, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Less equal supports only integer or decimal");
}

static inline op_result_t interpreter_fun_and(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value b,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (likely(valueI_is_boolean(a) && valueI_is_boolean(b))) {
        (*result) = valueI_boolean(valueI_as_boolean(a) && valueI_as_boolean(b));
        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_AND, &mt_field)) {
        struct value new_args[] = { b };
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, new_args, 1, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "And supports only boolean");
}

static inline op_result_t interpreter_fun_or(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value b,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (likely(valueI_is_boolean(a) && valueI_is_boolean(b))) {
        (*result) = valueI_boolean(valueI_as_boolean(a) || valueI_as_boolean(b));
        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_OR, &mt_field)) {
        struct value new_args[] = { b };
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, new_args, 1, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Or supports only boolean");
}

static inline op_result_t interpreter_fun_concat(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value b,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (likely(valueI_is_string(a) && valueI_is_string(b))) {
        struct string *a_str = valueI_as_string(a);
        struct string *b_str = valueI_as_string(b);
        (*result) = valueI_object(stringI_concat(U->I, a_str, b_str));

        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_CONCAT, &mt_field)) {
        struct value new_args[] = { b };
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, new_args, 1, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Concat supports only string");
}

static inline op_result_t interpreter_fun_type(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    unused(callstate);
    unused(pop_size);
    unused(need_return);

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_TYPE, &mt_field)) {
        (*result) = valueI_value2string(U->I, mt_field);
    } else {
        (*result) = valueI_object(stringI_create(U->I, valueI_type2string(U->I, a.type)));
    }

    return NORMAL;
}

static inline op_result_t interpreter_fun_negative(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (likely(valueI_is_integer(a))) {
        (*result) = valueI_integer(-valueI_as_integer(a));
        return NORMAL;
    }

    if (valueI_is_decimal(a)) {
        (*result) = valueI_decimal(-valueI_as_decimal(a));
        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_NEGATE, &mt_field)) {
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, NULL, 0, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Negate supports only integer or decimal");
}

static inline op_result_t interpreter_fun_not(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (likely(valueI_is_boolean(a))) {
        (*result) = valueI_boolean(!valueI_as_boolean(a));
        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_NOT, &mt_field)) {
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, NULL, 0, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Not supports only boolean");
}

static inline op_result_t interpreter_fun_ref(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_REF, &mt_field)) {
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, NULL, 0, pop_size);
        return CALLED;
    }

    (*result) = valueI_object(referenceI_create(U->I, a));
    return NORMAL;
}

static inline op_result_t interpreter_fun_deref(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (likely(valueI_is_reference(a))) {
        (*result) = *referenceI_get(U->I, valueI_as_reference_or_error(U->I, a));
        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_DEREF, &mt_field)) {
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, NULL, 0, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Deref supports only reference");
}

static inline op_result_t interpreter_fun_length(
    morphine_coroutine_t U,
    size_t callstate,
    struct value a,
    struct value *result,
    size_t pop_size,
    bool need_return
) {
    if (unlikely(need_return && (callstackI_state(U) == callstate))) {
        (*result) = callstackI_result(U);
        return CALLED_COMPLETE;
    }

    if (valueI_is_string(a)) {
        (*result) = valueI_size(valueI_as_string(a)->size);
        return NORMAL;
    }

    if (valueI_is_table(a)) {
        (*result) = valueI_size(tableI_size(U->I, valueI_as_table(a)));
        return NORMAL;
    }

    if (valueI_is_vector(a)) {
        (*result) = valueI_size(vectorI_size(U->I, valueI_as_vector(a)));
        return NORMAL;
    }

    struct value mt_field;
    if (metatableI_test(U->I, a, MF_LENGTH, &mt_field)) {
        callstackI_continue(U, callstate);
        callstackI_call_unsafe(U, mt_field, a, NULL, 0, pop_size);
        return CALLED;
    }

    throwI_error(U->I, "Length supports only string, table or vector");
}
