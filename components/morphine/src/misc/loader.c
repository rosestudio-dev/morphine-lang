//
// Created by why-iskra on 31.03.2024.
//

#include "morphine/misc/loader.h"
#include "morphine/object/coroutine/stack.h"
#include "morphine/object/coroutine.h"
#include "morphine/object/function.h"
#include "morphine/object/string.h"
#include "morphine/object/userdata.h"
#include "morphine/core/throw.h"
#include "morphine/algorithm/crc32.h"
#include "morphine/instruction.h"
#include "morphine/instruction/info.h"
#include <string.h>

#define FORMAT_TAG "morphine-bout"

typedef size_t uid_t;

struct function_with_uid {
    uid_t uid;
    struct function *function;
};

struct data {
    morphine_coroutine_t U;
    struct sio *sio;
    struct crc32_buf crc;

    struct {
        struct function_with_uid *vector;
        size_t count;
    } functions;
};

static uint8_t get_u8(struct data *data) {
    uint8_t byte = 0;
    size_t read_count = sioI_read(data->U->I, data->sio, &byte, 1);

    if (read_count == 0) {
        throwI_error(data->U->I, "Binary corrupted");
    }

    crc32_char(&data->crc, byte);
    return byte;
}

static uint16_t get_u16(struct data *data) {
    union {
        uint8_t raw[2];
        uint16_t result;
    } buffer;

    buffer.raw[1] = get_u8(data);
    buffer.raw[0] = get_u8(data);

    return buffer.result;
}

static uint32_t get_u32(struct data *data) {
    char type = (char) get_u8(data);

    union {
        uint8_t raw[4];
        uint32_t result;
    } buffer;

    memset(buffer.raw, 0, 4);

    int init;
    switch (type) {
        case 'i':
            init = 0;
            break;
        case 'h':
            init = 1;
            break;
        case 's':
            init = 2;
            break;
        case 'b':
            init = 3;
            break;
        case 'z':
            init = 4;
            break;
        default:
            throwI_error(data->U->I, "Unknown int tag");
    }

    for (int i = init; i < 4; i++) {
        buffer.raw[3 - i] = get_u8(data);
    }

    return buffer.result;
}

static uint64_t get_u64(struct data *data) {
    union {
        uint8_t raw[8];
        uint64_t result;
    } buffer;

    buffer.raw[7] = get_u8(data);
    buffer.raw[6] = get_u8(data);
    buffer.raw[5] = get_u8(data);
    buffer.raw[4] = get_u8(data);

    buffer.raw[3] = get_u8(data);
    buffer.raw[2] = get_u8(data);
    buffer.raw[1] = get_u8(data);
    buffer.raw[0] = get_u8(data);

    return buffer.result;
}

static double get_double(struct data *data) {
    union {
        uint64_t raw;
        double result;
    } buffer;

    buffer.raw = get_u64(data);

    return buffer.result;
}

static char get_char(struct data *data) {
    union {
        uint8_t raw[sizeof(char)];
        char result;
    } buffer;

    for (size_t i = 0; i < sizeof(char); i++) {
        buffer.raw[i] = get_u8(data);
    }

    return buffer.result;
}

static struct value get_string(struct data *data) {
    size_t size = get_u32(data);
    char *buffer;
    struct string *string = stringI_createn(data->U->I, size, &buffer);

    for (size_t i = 0; i < size; i++) {
        buffer[i] = get_char(data);
    }

    struct value value = valueI_object(string);
    stackI_push(data->U, value);

    return value;
}

static uid_t get_uid(struct data *data) {
    return get_u64(data);
}

static struct function_with_uid get_function(struct data *data) {
    uid_t uid = get_uid(data);
    ml_size name_len = get_u16(data);
    ml_size arguments_count = get_u16(data);
    ml_size slots_count = get_u16(data);
    ml_size params_count = get_u16(data);
    ml_size statics_count = get_u16(data);
    ml_size constants_count = get_u16(data);
    ml_size instructions_count = get_u16(data);

    struct function *function = functionI_create(
        data->U->I,
        name_len,
        constants_count,
        instructions_count,
        statics_count,
        arguments_count,
        slots_count,
        params_count
    );

    stackI_push(data->U, valueI_object(function));

    return (struct function_with_uid) {
        .uid = uid,
        .function = function,
    };
}

static void load_instructions(struct data *data, struct function *function) {
    for (ml_size i = 0; i < function->instructions_count; i++) {
        morphine_instruction_t instruction = {
            .line = 0,
            .opcode = get_u8(data),
            .argument1 = 0,
            .argument2 = 0,
            .argument3 = 0,
        };


        bool valid = false;
        size_t count = instructionI_opcode_args(instruction.opcode, &valid);
        if (!valid) {
            throwI_error(data->U->I, "Unsupported opcode");
        }

        ml_argument *args = &instruction.argument1;
        for (size_t c = 0; c < count; c++) {
            args[c] = get_u16(data);
        }

        functionI_instruction_set(data->U->I, function, i, instruction);
    }
}

static void load_lines(struct data *data, struct function *function) {
    for (ml_size i = 0; i < function->instructions_count; i++) {
        functionI_line_set(data->U->I, function, i, get_u32(data));
    }
}

static void load_constants(struct data *data, struct function *function) {
    for (ml_size i = 0; i < function->constants_count; i++) {
        char type = (char) get_u8(data);

        struct value constant = valueI_nil;
        switch (type) {
            case 'b': {
                constant = valueI_boolean(get_u8(data));
                break;
            }
            case 'i': {
                constant = valueI_integer((ml_integer) get_u32(data));
                break;
            }
            case 'f': {
                uid_t uid = get_uid(data);

                struct function *found = NULL;
                for (size_t c = 0; c < data->functions.count; c++) {
                    if (data->functions.vector[c].uid == uid) {
                        found = data->functions.vector[c].function;
                        break;
                    }
                }

                if (found == NULL) {
                    throwI_error(data->U->I, "Function constant corrupted");
                }

                constant = valueI_object(found);
                break;
            }
            case 'd': {
                constant = valueI_decimal((ml_decimal) get_double(data));
                break;
            }
            case 's': {
                constant = get_string(data);
                break;
            }
            case 'n': {
                constant = valueI_nil;
                break;
            }

            default: {
                throwI_error(data->U->I, "Unsupported constant tag");
            }
        }

        functionI_constant_set(data->U->I, function, i, constant);
    }
}

static void load_name(struct data *data, struct function *function) {
    for (size_t i = 0; i < function->name_len; i++) {
        function->name[i] = get_char(data);
    }
}

static void check_csum(struct data *data) {
    uint32_t expected = crc32_result(&data->crc);

    union {
        uint8_t raw[4];
        uint32_t result;
    } hash;

    hash.raw[3] = get_u8(data);
    hash.raw[2] = get_u8(data);
    hash.raw[1] = get_u8(data);
    hash.raw[0] = get_u8(data);

    if (expected != hash.result) {
        throwI_error(data->U->I, "Binary corrupted");
    }
}

static void check_tag(struct data *data) {
    char buffer[sizeof(FORMAT_TAG)];
    memset(buffer, 0, sizeof(buffer));

    for (size_t i = 0; i < (sizeof(FORMAT_TAG) - 1); i++) {
        buffer[i] = get_char(data);
    }

    if (strcmp(buffer, FORMAT_TAG) != 0) {
        throwI_error(data->U->I, "Wrong format tag");
    }
}

static uid_t load(struct data *data) {
    check_tag(data);

    uid_t main_uid = get_uid(data);
    size_t functions_count = get_u32(data);

    struct userdata *userdata = userdataI_create_vec(
        data->U->I, "functions", functions_count, sizeof(struct function_with_uid)
    );

    stackI_push(data->U, valueI_object(userdata));

    struct function_with_uid *functions = userdata->data;
    data->functions.count = functions_count;
    data->functions.vector = functions;

    for (size_t i = 0; i < functions_count; i++) {
        functions[i] = get_function(data);
    }

    for (size_t i = 0; i < functions_count; i++) {
        load_instructions(data, functions[i].function);
    }

    for (size_t i = 0; i < functions_count; i++) {
        load_lines(data, functions[i].function);
    }

    for (size_t i = 0; i < functions_count; i++) {
        load_constants(data, functions[i].function);
    }

    for (size_t i = 0; i < functions_count; i++) {
        load_name(data, functions[i].function);
    }

    for (size_t i = 0; i < functions_count; i++) {
        functionI_complete(data->U->I, functions[i].function);
    }

    check_csum(data);

    return main_uid;
}

static struct function *get_main(struct data *data, uid_t main_uid) {
    struct function *main = NULL;

    for (size_t i = 0; i < data->functions.count; i++) {
        if (data->functions.vector[i].uid == main_uid) {
            main = data->functions.vector[i].function;
            break;
        }
    }

    if (main == NULL) {
        throwI_error(data->U->I, "Main function wasn't found");
    }

    return main;
}

static struct function *binary(morphine_coroutine_t U, struct sio *sio) {
    struct data data = {
        .U = U,
        .sio = sio,
        .crc = crc32_init(),
        .functions.vector = NULL,
        .functions.count = 0
    };

    uid_t main_uid = load(&data);
    return get_main(&data, main_uid);
}

struct function *loaderI_load(morphine_coroutine_t U, struct sio *sio) {
    size_t stack_size = stackI_space(U);
    struct function *result = binary(U, sio);
    stackI_pop(U, stackI_space(U) - stack_size);

    return result;
}