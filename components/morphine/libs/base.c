//
// Created by whyiskra on 25.12.23.
//

#include <morphine.h>
#include <stdio.h>
#include "morphine/libs/loader.h"

static void version(morphine_state_t S) {
    nb_function(S)
        nb_init
            maux_checkargs(S, 1, "empty");
            mapi_push_string(S, mapi_version());
            nb_return();
    nb_end
}

static void print(morphine_state_t S) {
    nb_function(S)
        nb_init
            maux_checkargs(S, 1, "any");
            mapi_push_arg(S, 0);
            mlib_value_call(S, "tostr", 1);
        nb_state(1)
            mapi_push_result(S);
            fprintf(mapi_io_out(mapi_instance(S)), "%s", mapi_get_string(S));
            mapi_pop(S, 1);
            nb_leave();
    nb_end
}

static void println(morphine_state_t S) {
    nb_function(S)
        nb_init
            size_t variant = maux_checkargs(S, 2, "empty", "any");

            if (variant == 0) {
                fprintf(mapi_io_out(mapi_instance(S)), "\n");
                nb_leave();
            }

            mapi_push_arg(S, 0);
            mlib_value_call(S, "tostr", 1);
        nb_state(1)
            mapi_push_result(S);
            fprintf(mapi_io_out(mapi_instance(S)), "%s\n", mapi_get_string(S));
            mapi_pop(S, 1);
            nb_leave();
    nb_end
}

static void setmetatable(morphine_state_t S) {
    nb_function(S)
        nb_init
            size_t variant = maux_checkargs(S, 2, "self:meta,table", "meta,table");

            if (variant == 0) {
                mapi_push_self(S);
                mapi_push_arg(S, 0);
            } else {
                mapi_push_arg(S, 0);
                mapi_push_arg(S, 1);
            }

            mapi_set_metatable(S);
            nb_return();
    nb_end
}

static void getmetatable(morphine_state_t S) {
    nb_function(S)
        nb_init
            size_t variant = maux_checkargs(S, 2, "self:meta", "meta");

            if (variant == 0) {
                mapi_push_self(S);
            } else {
                mapi_push_arg(S, 0);
            }

            mapi_get_metatable(S);
            nb_return();
    nb_end
}

static void setdefaultmetatable(morphine_state_t S) {
    nb_function(S)
        nb_init
            maux_checkargs(S, 1, "any,table");
            mapi_push_arg(S, 0);
            const char *type = mapi_get_string(S);
            mapi_pop(S, 1);
            mapi_push_arg(S, 1);
            mapi_set_default_metatable(S, type);
            nb_return();
    nb_end
}

static void getdefaultmetatable(morphine_state_t S) {
    nb_function(S)
        nb_init
            maux_checkargs(S, 1, "any");
            mapi_push_arg(S, 0);
            const char *type = mapi_get_string(S);
            mapi_pop(S, 1);
            mapi_get_default_metatable(S, type);
            nb_return();
    nb_end
}

static void scall(morphine_state_t S) {
    nb_function(S)
        nb_init
            maux_checkargs(S, 1, "callable,any,any...");
            size_t count = mapi_args(S);

            mapi_push_arg(S, 1);
            mapi_push_arg(S, 0);

            for (size_t i = 2; i < count; i++) {
                mapi_push_arg(S, i);
            }

            mapi_callself(S, count - 2);
        nb_state(1)
            mapi_push_result(S);
            nb_return();
    nb_end
}

static void pcall(morphine_state_t S) {
    nb_function(S)
        nb_init
            maux_checkargs(S, 1, "callable,any...");
            size_t count = mapi_args(S);

            mapi_push_arg(S, 0);

            for (size_t i = 1; i < count; i++) {
                mapi_push_arg(S, i);
            }

            mapi_catchable(S, 2);
            mapi_call(S, count - 1);
        nb_state(1)
            mapi_push_table(S);

            mapi_push_stringf(S, "result");
            mapi_push_result(S);
            mapi_table_set(S);

            mapi_push_stringf(S, "thrown");
            mapi_push_nil(S);
            mapi_table_set(S);

            nb_return();
        nb_state(2)
            mapi_push_table(S);

            mapi_push_stringf(S, "result");
            mapi_push_nil(S);
            mapi_table_set(S);

            mapi_push_stringf(S, "thrown");
            mapi_push_thrown(S);
            mapi_table_set(S);

            nb_return();
    nb_end
}

static void pscall(morphine_state_t S) {
    nb_function(S)
        nb_init
            maux_checkargs(S, 1, "callable,any,any...");
            size_t count = mapi_args(S);

            mapi_push_arg(S, 1);
            mapi_push_arg(S, 0);

            for (size_t i = 2; i < count; i++) {
                mapi_push_arg(S, i);
            }

            mapi_catchable(S, 2);
            mapi_callself(S, count - 2);
        nb_state(1)
            mapi_push_table(S);

            mapi_push_stringf(S, "result");
            mapi_push_result(S);
            mapi_table_set(S);

            mapi_push_stringf(S, "thrown");
            mapi_push_nil(S);
            mapi_table_set(S);

            nb_return();
        nb_state(2)
            mapi_push_table(S);

            mapi_push_stringf(S, "result");
            mapi_push_nil(S);
            mapi_table_set(S);

            mapi_push_stringf(S, "thrown");
            mapi_push_thrown(S);
            mapi_table_set(S);

            nb_return();
    nb_end
}

static void error(morphine_state_t S) {
    nb_function(S)
        nb_init
            size_t variant = maux_checkargs(S, 2, "empty", "any");

            if (variant == 0) {
                mapi_push_nil(S);
            } else {
                mapi_push_arg(S, 0);
            }

            mapi_error(S);
            nb_leave();
    nb_end
}

static void changeenv(morphine_state_t S) {
    nb_function(S)
        nb_init
            maux_checkargs(S, 1, "any");
            mapi_push_arg(S, 0);
            mapi_changeenv(S);
            nb_leave();
    nb_end
}

static void load(morphine_state_t S) {
    nb_function(S)
        nb_init
            maux_checkargs(S, 1, "string");
            mapi_push_arg(S, 0);
            size_t len = mapi_string_len(S);
            const uint8_t *vector = (const uint8_t *) mapi_get_string(S);
            mapi_rload(S, len, vector);
            nb_leave();
    nb_end
}

static struct maux_construct_field table[] = {
    { "version",             version },
    { "print",               print },
    { "println",             println },
    { "setmetatable",        setmetatable },
    { "getmetatable",        getmetatable },
    { "setdefaultmetatable", setdefaultmetatable },
    { "getdefaultmetatable", getdefaultmetatable },
    { "scall",               scall },
    { "pcall",               pcall },
    { "pscall",              pscall },
    { "error",               error },
    { "changeenv",           changeenv },
    { "load",                load },
    { NULL, NULL }
};

void mlib_base_loader(morphine_state_t S) {
    maux_construct(S, table);
}

MORPHINE_LIB void mlib_base_call(morphine_state_t S, const char *name, size_t argc) {
    maux_construct_call(S, table, name, argc);
}
