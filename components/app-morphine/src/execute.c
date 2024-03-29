//
// Created by whyiskra on 3/16/24.
//

#include <execute.h>
#include <loaders.h>
#include <stdlib.h>
#include <setjmp.h>

static struct allocator *pallocator;
static jmp_buf abort_jmp;

__attribute__((noreturn)) static void cabort(void) {
    longjmp(abort_jmp, 1);
}

morphine_noret static void signal(morphine_instance_t I) {
    const char *message = mapi_get_panic_message(I);
    printf("morphine panic: %s\n", message);

    cabort();
}

static void *dalloc(size_t size) {
    return allocator_alloc(pallocator, size);
}

static void *drealloc(void *ptr, size_t size) {
    return allocator_realloc(pallocator, ptr, size);
}

static void dfree(void *ptr) {
    allocator_free(pallocator, ptr);
}

static void load_program(morphine_state_t S, const char *path, bool binary) {
    if (path == NULL) {
        mapi_errorf(S, "Empty file path");
    } else if (binary) {
        loader_binary_file(S, path);
    } else {
        loader_source_file(S, path);
    }
}

void execute(
    struct allocator *allocator,
    const char *path,
    bool binary,
    size_t alloc_limit
) {
    if (setjmp(abort_jmp) != 0) {
        return;
    }

    struct params instance_params = {
        .gc.limit_bytes = alloc_limit,
        .gc.start = 200,
        .gc.grow = 150,
        .gc.deal = 200,
        .gc.finalizer.stack_limit = 256,
        .gc.finalizer.stack_grow = 32,
        .states.stack_limit = 4096,
        .states.stack_grow = 64,
    };

    struct platform instance_platform = {
        .functions.malloc = malloc,
        .functions.realloc = realloc,
        .functions.free = free,
        .functions.signal = signal,
        .io.in = stdin,
        .io.out = stdout,
        .io.stacktrace = stderr,
    };

    if (allocator != NULL) {
        pallocator = allocator;
        instance_platform.functions.malloc = dalloc;
        instance_platform.functions.realloc = drealloc;
        instance_platform.functions.free = dfree;
    }

    morphine_instance_t I = mapi_open(instance_platform, instance_params, NULL);
    morphine_state_t S = mapi_state(I);

    load_program(S, path, binary);
    mapi_call(S, 0);

    mapi_interpreter(I);
    mapi_close(I);
}