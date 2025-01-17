//
// Created by why-iskra on 02.04.2024.
//

#pragma once

#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

// common

typedef int64_t ml_integer;
#define MLIMIT_INTEGER_PR  PRId64
#define MLIMIT_INTEGER_MAX INT64_MAX
#define MLIMIT_INTEGER_MIN INT64_MIN

typedef double ml_decimal;
#define MLIMIT_DECIMAL_PR "lg"
#define MLIMIT_DECIMAL_SC "lg"

// support

typedef uint32_t ml_size;
#define MLIMIT_SIZE_PR  PRIu32
#define MLIMIT_SIZE_MAX UINT32_MAX

typedef ml_size ml_hash;
#define MLIMIT_HASH_PR  MLIMIT_SIZE_PR
#define MLIMIT_HASH_MAX MLIMIT_SIZE_MAX

// instruction

typedef uint16_t ml_argument;
#define MLIMIT_ARGUMENT_PR  PRIu16
#define MLIMIT_ARGUMENT_MAX UINT16_MAX

typedef uint32_t ml_line;
#define MLIMIT_LINE_PR  PRIu32
#define MLIMIT_LINE_MAX UINT32_MAX

// behaviour

#define MLIMIT_CALLABLE_ARGS     (256)
#define MLIMIT_USERTYPE_NAME     (1024)
#define MLIMIT_STACKTRACE_STRING (256)
#define MLIMIT_TABLE_TREES       (131070)

// checks

static_assert((sizeof(size_t) <= sizeof(uintmax_t)) && (4 <= sizeof(size_t)), "incompatible arch");

static_assert(sizeof(ml_size) <= sizeof(size_t), "ml_size incompatible with arch");
static_assert((sizeof(ml_size) <= sizeof(ml_integer)) && (MLIMIT_SIZE_MAX <= MLIMIT_INTEGER_MAX), "ml_size incompatible with ml_integer");
static_assert(sizeof(ml_argument) < sizeof(ml_size), "ml_argument incompatible with ml_size");
