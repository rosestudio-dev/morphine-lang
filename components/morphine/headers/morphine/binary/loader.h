//
// Created by why-iskra on 31.03.2024.
//

#pragma once

#include "morphine/platform.h"

struct proto *loaderI_load(
    morphine_state_t,
    morphine_loader_init_t,
    morphine_loader_read_t,
    morphine_loader_finish_t,
    void *
);