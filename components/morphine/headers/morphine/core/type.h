//
// Created by whyiskra on 3/15/24.
//

#pragma once

#define VALUE_TYPES_START (VALUE_TYPE_USERDATA)
#define VALUE_TYPES_COUNT (VALUE_TYPE_RAW + 1)

#define OBJ_TYPES_START (OBJ_TYPE_USERDATA)
#define OBJ_TYPES_COUNT (OBJ_TYPE_REFERENCE + 1)

#define typeI_value_is_obj(x) ({ enum obj_type _vt = (enum obj_type) (x); (OBJ_TYPES_START <= _vt && _vt < OBJ_TYPES_COUNT); })

enum obj_type {
    OBJ_TYPE_USERDATA = 0,
    OBJ_TYPE_STRING = 1,
    OBJ_TYPE_TABLE = 2,
    OBJ_TYPE_CLOSURE = 3,
    OBJ_TYPE_STATE = 4,
    OBJ_TYPE_PROTO = 5,
    OBJ_TYPE_NATIVE = 6,
    OBJ_TYPE_REFERENCE = 7,
};

enum value_type {
    VALUE_TYPE_USERDATA = 0,
    VALUE_TYPE_STRING = 1,
    VALUE_TYPE_TABLE = 2,
    VALUE_TYPE_CLOSURE = 3,
    VALUE_TYPE_STATE = 4,
    VALUE_TYPE_PROTO = 5,
    VALUE_TYPE_NATIVE = 6,
    VALUE_TYPE_REFERENCE = 7,

    VALUE_TYPE_NIL = 8,
    VALUE_TYPE_INTEGER = 9,
    VALUE_TYPE_DECIMAL = 10,
    VALUE_TYPE_BOOLEAN = 11,
    VALUE_TYPE_RAW = 12,
};
