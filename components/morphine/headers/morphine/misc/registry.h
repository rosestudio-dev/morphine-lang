//
// Created by whyiskra on 07.01.24.
//

#pragma once

#include "morphine/core/value.h"

void registryI_set_key(morphine_instance_t, struct value callable, struct value key);

void registryI_set(morphine_coroutine_t, struct value key, struct value value);
struct value registryI_get(morphine_coroutine_t, struct value key, bool *has);
void registryI_clear(morphine_coroutine_t);
void registryI_clear_by_key(morphine_instance_t, struct value);
