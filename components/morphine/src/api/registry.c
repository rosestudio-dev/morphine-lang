//
// Created by whyiskra on 07.01.24.
//

#include "morphine/api.h"
#include "morphine/object/string.h"
#include "morphine/object/coroutine.h"
#include "morphine/core/throw.h"
#include "morphine/misc//registry.h"

MORPHINE_API void mapi_registry_set_key(morphine_coroutine_t U) {
    struct value callable = stackI_peek(U, 1);
    struct value key = stackI_peek(U, 0);

    registryI_set_key(U->I, callable, key);

    stackI_pop(U, 1);
}

MORPHINE_API bool mapi_registry_get(morphine_coroutine_t U) {
    struct value key = stackI_peek(U, 0);

    bool has = false;
    struct value result = registryI_get(U, key, &has);
    stackI_replace(U, 0, result);

    return has;
}

MORPHINE_API void mapi_registry_getoe(morphine_coroutine_t U) {
    struct value key = stackI_peek(U, 0);

    bool has = false;
    struct value result = registryI_get(U, key, &has);

    if (has) {
        stackI_replace(U, 0, result);
    } else {
        throwI_error(U->I, "cannot get value from registry by key");
    }
}

MORPHINE_API void mapi_registry_set(morphine_coroutine_t U) {
    struct value key = stackI_peek(U, 1);
    struct value value = stackI_peek(U, 0);

    registryI_set(U, key, value);

    stackI_pop(U, 2);
}

MORPHINE_API bool mapi_registry_remove(morphine_coroutine_t U) {
    struct value key = stackI_peek(U, 0);

    bool has = false;
    struct value result = registryI_remove(U, key, &has);
    stackI_replace(U, 0, result);

    return has;
}

MORPHINE_API void mapi_registry_removeoe(morphine_coroutine_t U) {
    struct value key = stackI_peek(U, 0);

    bool has = false;
    struct value result = registryI_remove(U, key, &has);

    if (has) {
        stackI_replace(U, 0, result);
    } else {
        throwI_error(U->I, "cannot remove value from registry by key");
    }
}

MORPHINE_API void mapi_registry_clear(morphine_coroutine_t U) {
    registryI_clear(U);
}
