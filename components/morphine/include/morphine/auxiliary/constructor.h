//
// Created by why-iskra on 31.03.2024.
//

#pragma once

#include "morphine/platform.h"
#include "morphine/config.h"

struct maux_construct_field {
    const char *name;
    morphine_native_t value;
};

MORPHINE_AUX void maux_construct(morphine_state_t, struct maux_construct_field *table);

MORPHINE_AUX void maux_construct_call(
    morphine_state_t,
    struct maux_construct_field *table,
    const char *name,
    size_t argc
);