//
// Created by whyiskra on 25.12.23.
//

#include <stdarg.h>
#include <string.h>
#include "morphine/auxiliary/checkargs.h"
#include "morphine/api.h"

static bool check_type(
    morphine_coroutine_t U,
    const char *expected_type,
    size_t expected_type_len,
    bool is_self,
    size_t arg
) {
    if (expected_type_len == 3 && memcmp(expected_type, "any", 3) == 0) {
        return true;
    }

    if (is_self) {
        mapi_push_self(U);
    } else {
        mapi_push_arg(U, arg);
    }

    if (expected_type_len == 8 && memcmp(expected_type, "callable", 8) == 0) {
        bool is_callable = mapi_is_callable(U);
        mapi_pop(U, 1);

        return is_callable;
    }

    if (expected_type_len == 8 && memcmp(expected_type, "iterable", 8) == 0) {
        bool is_iterable = mapi_is_iterable(U);
        mapi_pop(U, 1);

        return is_iterable;
    }

    if (expected_type_len == 4 && memcmp(expected_type, "meta", 4) == 0) {
        bool is_metatype = mapi_is_metatype(U);
        mapi_pop(U, 1);

        return is_metatype;
    }

    const char *type = mapi_raw_type(U);
    size_t typelen = strlen(type);

    mapi_pop(U, 1);

    if (expected_type_len != typelen || memcmp(expected_type, type, typelen) != 0) {
        return false;
    }

    return true;
}

static bool check_pattern(
    morphine_coroutine_t U,
    const char *pattern
) {
    size_t argc = mapi_args(U);

    if (strcmp(pattern, "empty") == 0) {
        return argc == 0;
    }

    size_t len = strlen(pattern);

    size_t parts = 1;
    for (size_t i = 0; i < len; i++) {
        if (pattern[i] == ',') {
            parts++;
        }
    }
    size_t reqargs = 0;

    size_t start = 0;
    for (size_t i = 0; i < parts; i++) {
        const char *substr = pattern + start;
        const char *end = strchr(substr, ',');
        if (end == NULL) {
            end = pattern + len;
        }

        size_t sublen = (size_t) (end - substr);

        bool vararg = false;
        bool self = false;

        if (sublen >= 3 && memcmp(substr + (sublen - 3), "...", 3) == 0) {
            if (i < parts - 1) {
                mapi_errorf(U, "Wrong argument pattern (vararg should be always last)");
            } else {
                vararg = true;
            }
        }

        if (sublen >= 5 && memcmp(substr, "self:", 5) == 0) {
            if (vararg) {
                mapi_errorf(U, "Wrong argument pattern (vararg cannot be combine with self)");
            } else if (i > 0) {
                mapi_errorf(U, "Wrong argument pattern (self should be always first)");
            } else {
                self = true;
            }
        }

        if (vararg) {
            size_t expected_type_len = sublen - 3;

            for (size_t k = reqargs; k < argc; k++) {
                if (!check_type(U, substr, expected_type_len, false, k)) {
                    return false;
                }
            }

            break;
        }

        if (self) {
            if (!check_type(U, substr + 5, sublen - 5, true, 0)) {
                return false;
            }
        } else {
            if (reqargs >= argc) {
                return false;
            }

            if (!check_type(U, substr, sublen, false, reqargs)) {
                return false;
            }

            reqargs++;
        }

        if (i >= parts - 1 && argc != reqargs) {
            return false;
        }

        start += sublen + 1;
    }

    return true;
}

MORPHINE_AUX size_t maux_checkargs(morphine_coroutine_t U, size_t count, ...) {
    if (count == 0) {
        if (mapi_args(U) == 0) {
            return 0;
        } else {
            mapi_errorf(U, "Arguments aren't expected");
        }
    }

    va_list vargs;
    va_start(vargs, count);

    bool found = false;
    size_t result;
    for (size_t i = 0; i < count; i++) {
        const char *pattern = va_arg(vargs, const char *);

        if (check_pattern(U, pattern)) {
            if (!found) {
                result = i;
            }
            found = true;
        }
    }

    va_end(vargs);

    if (!found) {
        mapi_errorf(U, "Unexpected arguments");
    }

    return result;
}

MORPHINE_AUX void maux_expect(morphine_coroutine_t U, const char *type) {
    if (!mapi_is(U, type)) {
        mapi_type(U);
        mapi_errorf(U, "Expected %s, but got %s", type, mapi_get_string(U));
    }
}
