//
// Created by why-iskra on 13.06.2024.
//

#include <string.h>
#include "morphine/auxiliary/sio.h"
#include "morphine/api.h"

struct buffer_data {
    bool closed;

    size_t allocated;
    size_t factor;

    size_t size;
    size_t pointer;
    uint8_t *vector;
};

static inline void check_close(morphine_sio_accessor_t A, struct buffer_data *B) {
    if (B->closed) {
        mapi_sio_accessor_error(A, "buffer closed");
    }
}

static void *buffer_open(morphine_sio_accessor_t A, void *data) {
    (void) A;

    struct buffer_data *B = data;
    B->closed = false;

    return B;
}

static void buffer_close(morphine_sio_accessor_t A, void *data) {
    (void) A;

    struct buffer_data *B = data;
    B->closed = true;
}

static size_t buffer_read(morphine_sio_accessor_t A, void *data, uint8_t *buffer, size_t size) {
    struct buffer_data *B = data;
    check_close(A, B);

    size_t read = 0;
    for (size_t i = 0; i < size; i++) {
        if (B->pointer >= B->size) {
            buffer[i] = 0;
        } else {
            buffer[i] = B->vector[B->pointer];
            B->pointer++;
            read++;
        }
    }

    return read;
}

static size_t buffer_write(morphine_sio_accessor_t A, void *data, const uint8_t *buffer, size_t size) {
    struct buffer_data *B = data;
    check_close(A, B);

    for (size_t i = 0; i < size; i++) {
        if (B->pointer >= B->size) {
            if (B->size > SIZE_MAX - 1) {
                mapi_sio_accessor_error(A, "sio buffer overflow");
            }

            B->size++;
        }

        if (B->size >= B->allocated) {
            if (B->allocated > SIZE_MAX - B->factor) {
                mapi_sio_accessor_error(A, "sio buffer overflow");
            }

            size_t new_size = B->allocated + B->factor;
            B->vector = mapi_sio_accessor_alloc_vec(A, B->vector, new_size, sizeof(char));
            B->allocated = new_size;
        }

        B->vector[B->pointer] = buffer[i];
        B->pointer++;
    }

    return size;
}

static bool buffer_seek(
    morphine_sio_accessor_t A,
    void *data,
    size_t offset,
    morphine_sio_seek_mode_t mode
) {
    struct buffer_data *B = data;
    check_close(A, B);

    switch (mode) {
        case MORPHINE_SIO_SEEK_MODE_SET: {
            if (offset > B->size) {
                return false;
            } else {
                B->pointer = offset;
                return true;
            }
        }
        case MORPHINE_SIO_SEEK_MODE_CUR: {
            if (B->pointer > B->size) {
                B->pointer = B->size;
            }

            if (offset > B->size - B->pointer) {
                return false;
            } else {
                B->pointer += offset;
                return true;
            }
        }
        case MORPHINE_SIO_SEEK_MODE_PRV: {
            if (B->pointer > B->size) {
                B->pointer = B->size;
            }

            if (offset > B->pointer) {
                return false;
            } else {
                B->pointer -= offset;
                return true;
            }
        }
        case MORPHINE_SIO_SEEK_MODE_END: {
            if (offset > B->size) {
                return false;
            } else {
                B->pointer = B->size - offset;
                return true;
            }
        }
    }

    return false;
}

static size_t buffer_tell(morphine_sio_accessor_t A, void *data) {
    struct buffer_data *B = data;
    check_close(A, B);

    if (B->pointer > B->size) {
        return B->size;
    } else {
        return B->pointer;
    }
}

static bool buffer_eos(morphine_sio_accessor_t A, void *data) {
    struct buffer_data *B = data;
    check_close(A, B);

    return B->pointer >= B->size;
}

static void buffer_free(morphine_instance_t I, void *data) {
    struct buffer_data *B = data;
    mapi_allocator_free(I, B->vector);
}

MORPHINE_AUX void maux_push_sio_buffer(morphine_coroutine_t U, size_t factor, bool read, bool write) {
    if (factor == 0) {
        mapi_error(U, "sio buffer extension factor is zero");
    }

    if (!read && !write) {
        mapi_error(U, "sio buffer must be readable or writable");
    }

    const char *str = mapi_get_string(U);
    ml_size str_len = mapi_string_len(U);

    struct buffer_data *data = mapi_push_userdata(U, "buffer", sizeof(struct buffer_data));

    *data = (struct buffer_data) {
        .closed = true,
        .allocated = 0,
        .factor = factor,
        .size = 0,
        .pointer = 0,
        .vector = NULL
    };

    mapi_userdata_set_free(U, buffer_free);

    data->vector = mapi_allocator_vec(mapi_instance(U), NULL, str_len, sizeof(char));
    data->allocated = ((size_t) str_len) * sizeof(char);
    data->size = str_len;
    memcpy(data->vector, str, data->allocated);

    morphine_sio_interface_t interface = {
        .open = buffer_open,
        .close = buffer_close,
        .read = NULL,
        .write = NULL,
        .flush = NULL,
        .eos = buffer_eos,
        .tell = buffer_tell,
        .seek = buffer_seek
    };

    if (read) {
        interface.read = buffer_read;
    }

    if (write) {
        interface.write = buffer_write;
    }

    mapi_push_sio(U, interface);

    mapi_rotate(U, 2);
    mapi_sio_hold(U);

    mapi_sio_open(U, data);
}

MORPHINE_AUX void maux_sio_extract_string(morphine_coroutine_t U) {
    if (!mapi_sio_seek_end(U, 0)) {
        mapi_error(U, "cannot extract string");
    }

    size_t size = mapi_sio_tell(U);

    if (!mapi_sio_seek_set(U, 0)) {
        mapi_error(U, "cannot extract string");
    }

    char *buffer = mapi_push_userdata_vec(U, "buffer", size, sizeof(char));
    mapi_rotate(U, 2);

    size_t result = mapi_sio_read(U, (uint8_t *) buffer, size * sizeof(char));
    if (result != size * sizeof(char)) {
        mapi_error(U, "cannot extract string");
    }

    mapi_pop(U, 1);
    mapi_push_stringn(U, buffer, size);
}