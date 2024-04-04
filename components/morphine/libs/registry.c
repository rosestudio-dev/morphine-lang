//
// Created by whyiskra on 30.12.23.
//

#include <morphine.h>
#include <string.h>
#include "morphine/libs/loader.h"

static void get(morphine_coroutine_t U) {
    nb_function(U)
        nb_init
            maux_checkargs(U, 1, "any");
            mapi_push_arg(U, 0);
            mapi_registry_get(U);
            nb_return();
    nb_end
}

static void has(morphine_coroutine_t U) {
    nb_function(U)
        nb_init
            maux_checkargs(U, 1, "any");
            mapi_push_arg(U, 0);
            bool has = mapi_registry_get(U);
            mapi_pop(U, 1);
            mapi_push_boolean(U, has);
            nb_return();
    nb_end
}

static void set(morphine_coroutine_t U) {
    nb_function(U)
        nb_init
            maux_checkargs(U, 1, "any,any");
            mapi_push_arg(U, 0);
            mapi_push_arg(U, 1);
            mapi_registry_set(U);
            nb_leave();
    nb_end
}

static void clear(morphine_coroutine_t U) {
    nb_function(U)
        nb_init
            maux_checkargs(U, 1, "empty");
            mapi_registry_clear(U);
            nb_leave();
    nb_end
}

static struct maux_construct_field table[] = {
    { "get",   get },
    { "set",   set },
    { "has",   has },
    { "clear", clear },
    { NULL, NULL }
};

void mlib_registry_loader(morphine_coroutine_t U) {
    maux_construct(U, table);
}

MORPHINE_LIB void mlib_registry_call(morphine_coroutine_t U, const char *name, size_t argc) {
    maux_construct_call(U, table, name, argc);
}
