//
// Created by whyiskra on 3/23/24.
//

#include "morphine/gc/structure.h"
#include "morphine/gc/control.h"
#include "morphine/core/instance.h"
#include "morphine/core/callstack.h"

static void free_objects(morphine_instance_t I, struct object *pool) {
    struct object *current = pool;

    while (current != NULL) {
        struct object *prev = current->prev;
        objectI_free(I, current);
        current = prev;
    }
}

void gcI_prototype(morphine_instance_t I, size_t inited_bytes) {
    I->G = (struct garbage_collector) {
        .status = GC_STATUS_IDLE,
        .enabled = false,

        .settings.limit = 0,     // bytes
        .settings.threshold = 0, // bytes
        .settings.grow = 0,      // percentage / 10
        .settings.deal = 0,      // percentage / 10
        .settings.pause = 0,     // 2^n bytes

        .settings.cache_callinfo_holding = 0,

        .stats.debt = 0,
        .stats.prev_allocated = 0,

        .bytes.allocated = inited_bytes,
        .bytes.max_allocated = inited_bytes,

        .pools.allocated = NULL,
        .pools.grey = NULL,
        .pools.black = NULL,
        .pools.black_coroutines = NULL,
        .pools.sweep = NULL,
        .pools.finalize = NULL,

        .finalizer.candidate = NULL,
        .finalizer.coroutine = NULL,
        .finalizer.work = false,

        .safe.index = 0,

        .cache.callinfo.pool = NULL,
        .cache.callinfo.size = 0,
    };

    size_t size = sizeof(I->G.safe.stack) / sizeof(struct value);

    for (size_t i = 0; i < size; i++) {
        I->G.safe.stack[i] = valueI_nil;
    }

    gcI_change_limit(I, I->settings.gc.limit_bytes);
    gcI_change_threshold(I, I->settings.gc.threshold);
    gcI_change_grow(I, I->settings.gc.grow);
    gcI_change_deal(I, I->settings.gc.deal);
    gcI_change_pause(I, I->settings.gc.pause);
    gcI_change_cache_callinfo_holding(I, I->settings.gc.cache_callinfo_holding);
}

void gcI_destruct(morphine_instance_t I, struct garbage_collector G) {
    free_objects(I, G.pools.allocated);
    free_objects(I, G.pools.grey);
    free_objects(I, G.pools.black);
    free_objects(I, G.pools.black_coroutines);
    free_objects(I, G.pools.sweep);
    free_objects(I, G.pools.finalize);

    if (G.finalizer.candidate != NULL) {
        objectI_free(I, G.finalizer.candidate);
    }

    struct callinfo *current = G.cache.callinfo.pool;
    while (current != NULL) {
        struct callinfo *prev = current->prev;
        callstackI_callinfo_free(I, current);

        current = prev;
    }
}
