//
// Created by why-iskra on 31.03.2024.
//

#include "morphine/binary/loader.h"
#include "loader/process.h"
#include "loader/binary.h"

struct proto *loaderI_load(
    morphine_coroutine_t U,
    morphine_loader_init_t init,
    morphine_loader_read_t read,
    morphine_loader_finish_t finish,
    void *args
) {
    return process(U, init, read, finish, args, binary);
}
