//
// Created by whyiskra on 3/23/24.
//

#pragma once

#include "morphine/core/value.h"

typedef enum {
    GC_STATUS_IDLE,
    GC_STATUS_PREPARE,
    GC_STATUS_INCREMENT,
    GC_STATUS_RESOLVE,
    GC_STATUS_SWEEP,
} gc_status_t;

struct garbage_collector {
    gc_status_t status;

    bool enabled;

    struct {
        size_t debt;
        size_t allocated;
        size_t prev_allocated;
        size_t max_allocated;
    } stats;

    struct {
        struct object *allocated;
        struct object *grey;
        struct object *black;
        struct object *black_coroutines;
        struct object *sweep;

        struct object *finalize;
    } pools;

    struct {
        struct object *candidate;
        struct native *resolver;
        struct string *name;
        morphine_coroutine_t coroutine;
    } finalizer;

    struct {
        size_t index;
        struct value stack[16];
    } safe;

    struct {
        struct {
            size_t size;
            struct callinfo *pool;
        } callinfo;
    } cache;
};

void gcI_prototype(morphine_instance_t, size_t inited_bytes);
void gcI_destruct(morphine_instance_t, struct garbage_collector);
