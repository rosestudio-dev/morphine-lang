//
// Created by whyiskra on 3/16/24.
//

#pragma once

#include <stddef.h>

struct allocator {
    size_t max_bytes;
    size_t allocated_bytes;
    size_t allocations_count;
    size_t peak_allocated_bytes;
    struct rbtree *tree;
};

void allocator_init(struct allocator *, size_t max);
void allocator_destroy(struct allocator *);
void *allocator_alloc(struct allocator *, size_t);
void *allocator_realloc(struct allocator *, void *, size_t);
void allocator_free(struct allocator *, void *);
void allocator_clear(struct allocator *);
