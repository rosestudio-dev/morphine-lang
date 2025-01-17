//
// Created by why-iskra on 18.04.2024.
//

#include "impl.h"
#include "morphine/core/instance.h"

void gcstageI_prepare(morphine_instance_t I) {
    if (unlikely(I->G.pools.grey != NULL ||
                 I->G.pools.black != NULL ||
                 I->G.pools.black_coroutines != NULL)) {
        throwI_panic(I, "corrupted gc pools");
    }

    struct object *current = I->G.pools.allocated;
    while (current != NULL) {
        current->color = OBJ_COLOR_WHITE;
        current = current->prev;
    }

    I->G.stats.debt = 0;
    I->G.stats.prev_allocated = I->G.stats.allocated;
}
