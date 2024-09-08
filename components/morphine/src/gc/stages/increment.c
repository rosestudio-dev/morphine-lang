//
// Created by whyiskra on 3/23/24.
//

#include "impl.h"
#include "mark.h"
#include "morphine/utils/overflow.h"

static inline void record(morphine_instance_t I) {
    {
        morphine_coroutine_t current = I->E.coroutines;
        while (current != NULL) {
            mark_object(I, objectI_cast(current));
            current = current->prev;
        }

        if (I->E.running != NULL) {
            mark_object(I, objectI_cast(I->E.running));
        }

        if (I->E.next != NULL) {
            mark_object(I, objectI_cast(I->E.next));
        }

        if (I->G.finalizer.coroutine != NULL) {
            mark_object(I, objectI_cast(I->G.finalizer.coroutine));
        }
    }

    {
        struct object *current = I->G.pools.black_coroutines;
        while (current != NULL) {
            mark_internal(I, current);
            current = current->prev;
        }
    }

    {
        struct object *current = I->G.pools.finalize;
        while (current != NULL) {
            mark_internal(I, current);
            current = current->prev;
        }

        if (I->G.finalizer.candidate != NULL) {
            mark_internal(I, I->G.finalizer.candidate);
        }
    }

    for (enum metatable_field mf = MFS_START; mf < MFS_COUNT; mf++) {
        mark_object(I, objectI_cast(I->metatable.names[mf]));
    }

    for (enum value_type type = VALUE_TYPES_START; type < VALUE_TYPES_COUNT; type++) {
        struct table *table = I->metatable.defaults[type];
        if (table != NULL) {
            mark_object(I, objectI_cast(table));
        }
    }

    for (size_t i = 0; i < I->libraries.size; i++) {
        struct library *library = I->libraries.array + i;
        if (library->table != NULL) {
            mark_object(I, objectI_cast(library->table));
        }
    }

    if (!I->E.throw.is_message) {
        mark_value(I, I->E.throw.error.value);
    }

    if (I->env != NULL) {
        mark_object(I, objectI_cast(I->env));
    }

    if (I->localstorage != NULL) {
        mark_object(I, objectI_cast(I->localstorage));
    }

    mark_object(I, objectI_cast(I->sio.error));
    mark_object(I, objectI_cast(I->sio.io));

    {
        size_t size = sizeof(I->G.safe.stack) / sizeof(struct value);

        for (size_t i = 0; i < size; i++) {
            mark_value(I, I->G.safe.stack[i]);
        }
    }
}

bool gcstageI_increment(morphine_instance_t I, size_t debt) {
    record(I);

    size_t checked = 0;
    {
        struct object *current = I->G.pools.grey;
        while (current != NULL) {
            struct object *prev = current->prev;

            current->color = OBJ_COLOR_BLACK;
            gcI_pools_remove(current, &I->G.pools.grey);

            if (current->type == OBJ_TYPE_COROUTINE) {
                gcI_pools_insert(current, &I->G.pools.black_coroutines);
            } else {
                gcI_pools_insert(current, &I->G.pools.black);
            }

            size_t size = mark_internal(I, current);
            overflow_add(size, checked, SIZE_MAX) {
                checked = SIZE_MAX;
            } else {
                checked += size;
            }

            current = prev;

            if (unlikely(debt < checked)) {
                break;
            }
        }
    }

    if (I->G.stats.debt > checked) {
        I->G.stats.debt -= checked;
    } else {
        I->G.stats.debt = 0;
    }

    return I->G.pools.grey != NULL;
}
