//
// Created by whyiskra on 30.12.23.
//

#include "morphinel/system.h"
#include <sys/time.h>

static uint64_t get_millis(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (((uint64_t) tv.tv_sec) * 1000) + (((uint64_t) tv.tv_usec) / 1000);
}

static void delay(morphine_coroutine_t U) {
    maux_nb_function(U)
        maux_nb_init
            maux_expect_args(U, 1);

            uint64_t *time = mapi_push_userdata_uni(U, sizeof(uint64_t));
            *time = get_millis();
            maux_nb_im_continue(1);
        maux_nb_state(1)
            uint64_t *time = mapi_userdata_pointer(U, NULL);

            mapi_push_arg(U, 0);
            ml_size size = mapi_get_size(U, "millis");
            mapi_pop(U, 1);

            if (get_millis() - *time < size) {
                maux_nb_continue(1);
            }

            maux_nb_leave();
    maux_nb_end
}

static void millis(morphine_coroutine_t U) {
    maux_nb_function(U)
        maux_nb_init
            maux_expect_args(U, 0);
            mapi_push_integer(U, (ml_integer) (get_millis() % (uint64_t) MLIMIT_INTEGER_MAX));
            maux_nb_return();
    maux_nb_end
}

static maux_construct_element_t elements[] = {
    MAUX_CONSTRUCT_FUNCTION("millis", millis),
    MAUX_CONSTRUCT_FUNCTION("delay", delay),
    MAUX_CONSTRUCT_END
};

static void library_init(morphine_coroutine_t U) {
    maux_construct(U, elements);
}

MORPHINE_LIB morphine_library_t mllib_system(void) {
    return (morphine_library_t) {
        .name = "system",
        .sharedkey = NULL,
        .init = library_init
    };
}
