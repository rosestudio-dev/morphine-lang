//
// Created by whyiskra on 3/23/24.
//

#pragma once

#include "morphine/core/callstack.h"

void gcI_cache_dispose_callinfo(morphine_instance_t, struct callinfo *callinfo);
struct callinfo *gcI_cache_callinfo(morphine_instance_t);
