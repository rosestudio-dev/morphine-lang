//
// Created by why-iskra on 27.05.2024.
//

#pragma once

#include <morphine.h>
#include "morphinec/strtable.h"

#define MC_AST_USERDATA_TYPE "morphinec-ast"

enum mc_ast_node_type {
    MCANT_EXPRESSION,
    MCANT_STATEMENT,
};

enum mc_statement_type {
    MCSTMTT_pass,
    MCSTMTT_yield,
    MCSTMTT_eval,
    MCSTMTT_while,
    MCSTMTT_for,
    MCSTMTT_iterator,
    MCSTMTT_declaration,
    MCSTMTT_assigment,
};

enum mc_expression_type {
    MCEXPRT_value,
    MCEXPRT_binary,
    MCEXPRT_unary,
    MCEXPRT_increment,
    MCEXPRT_variable,
    MCEXPRT_global,
    MCEXPRT_leave,
    MCEXPRT_break,
    MCEXPRT_continue,
    MCEXPRT_table,
    MCEXPRT_vector,
    MCEXPRT_access,
    MCEXPRT_call,
    MCEXPRT_function,
    MCEXPRT_block,
    MCEXPRT_if,
    MCEXPRT_when,
    MCEXPRT_asm,
};

struct mc_ast_node {
    enum mc_ast_node_type type;
    ml_line line;
    ml_size from;
    ml_size to;

    struct mc_ast_node *prev;
};

struct mc_ast_statement {
    struct mc_ast_node node;
    enum mc_statement_type type;
};

struct mc_ast_expression {
    struct mc_ast_node node;
    enum mc_expression_type type;
};

struct mc_ast_function {
    ml_line line;

    bool recursive;
    bool anonymous;
    mc_strtable_index_t name;

    bool auto_closure;
    size_t closures_size;
    size_t args_size;
    size_t statics_size;

    mc_strtable_index_t *closures;
    mc_strtable_index_t *arguments;
    mc_strtable_index_t *statics;

    struct mc_ast_statement *body;
    struct mc_ast_function *prev;
};

struct mc_ast;

MORPHINE_API struct mc_ast *mcapi_push_ast(morphine_coroutine_t);
MORPHINE_API struct mc_ast *mcapi_get_ast(morphine_coroutine_t);

MORPHINE_API struct mc_ast_expression *mcapi_ast_node2expression(morphine_coroutine_t, struct mc_ast_node *);
MORPHINE_API struct mc_ast_statement *mcapi_ast_node2statement(morphine_coroutine_t, struct mc_ast_node *);
MORPHINE_API const char *mcapi_ast_type_name(morphine_coroutine_t, struct mc_ast_node *);

MORPHINE_API struct mc_ast_function *mcapi_ast_functions(struct mc_ast *);
MORPHINE_API struct mc_ast_function *mcapi_ast_create_function(
    morphine_coroutine_t,
    struct mc_ast *,
    size_t closures,
    size_t args,
    size_t statics
);

MORPHINE_API void mcapi_ast_set_root_function(struct mc_ast *, struct mc_ast_function *);
MORPHINE_API struct mc_ast_function *mcapi_ast_get_root_function(struct mc_ast *);

static inline struct mc_ast_node *mcapi_ast_expression2node(struct mc_ast_expression *expression) {
    return (struct mc_ast_node *) expression;
}

static inline struct mc_ast_node *mcapi_ast_statement2node(struct mc_ast_statement *statement) {
    return (struct mc_ast_node *) statement;
}

#define ast_args(args...) , args
#define ast_noargs
#define ast_declare_node(ntype, prefix, name, args...) \
    MORPHINE_API struct mc_ast_##ntype##_##name *mcapi_ast_create_##ntype##_##name(morphine_coroutine_t, struct mc_ast *, ml_size from, ml_size to, ml_line line args); \
    static inline struct mc_ast_##ntype##_##name *mcapi_ast_##ntype##2##name(morphine_coroutine_t U, struct mc_ast_##ntype *ntype) { if(ntype->type != prefix##_##name) { mapi_error(U, "expected " #name " " #ntype); } return (struct mc_ast_##ntype##_##name *) ntype; } \
    static inline struct mc_ast_##ntype##_##name *mcapi_ast_node2##name##_##ntype(morphine_coroutine_t U, struct mc_ast_node *node) { return mcapi_ast_##ntype##2##name(U, mcapi_ast_node2##ntype(U, node)); } \
    static inline struct mc_ast_##ntype *mcapi_ast_##name##2##ntype(struct mc_ast_##ntype##_##name *ntype##_##name) { return (struct mc_ast_##ntype *) ntype##_##name; } \
    static inline struct mc_ast_node *mcapi_ast_##ntype##_##name##2node(struct mc_ast_##ntype##_##name *ntype##_##name) { return (struct mc_ast_node *) ntype##_##name; } \
    struct mc_ast_##ntype##_##name { struct mc_ast_##ntype header;

#define ast_declare_expr(name, args) ast_declare_node(expression, MCEXPRT, name, args)
#define ast_declare_stmt(name, args) ast_declare_node(statement, MCSTMTT, name, args)

#define ast_extract_part(type) bool is_extract; union { struct { size_t size; type *values; struct mc_ast_expression **keys; } extract; type value; }

// types

enum mc_expression_value_type {
    MCEXPR_VALUE_TYPE_NIL,
    MCEXPR_VALUE_TYPE_INT,
    MCEXPR_VALUE_TYPE_DEC,
    MCEXPR_VALUE_TYPE_STR,
    MCEXPR_VALUE_TYPE_BOOL,
};

enum mc_expression_global_type {
    MCEXPR_GLOBAL_TYPE_ENV,
    MCEXPR_GLOBAL_TYPE_SELF,
    MCEXPR_GLOBAL_TYPE_INVOKED,
};

enum mc_expression_binary_type {
    MCEXPR_BINARY_TYPE_ADD,
    MCEXPR_BINARY_TYPE_SUB,
    MCEXPR_BINARY_TYPE_MUL,
    MCEXPR_BINARY_TYPE_DIV,
    MCEXPR_BINARY_TYPE_MOD,
    MCEXPR_BINARY_TYPE_EQUAL,
    MCEXPR_BINARY_TYPE_LESS,
    MCEXPR_BINARY_TYPE_CONCAT,
    MCEXPR_BINARY_TYPE_AND,
    MCEXPR_BINARY_TYPE_OR,
};

enum mc_expression_unary_type {
    MCEXPR_UNARY_TYPE_NEGATE,
    MCEXPR_UNARY_TYPE_NOT,
    MCEXPR_UNARY_TYPE_TYPE,
    MCEXPR_UNARY_TYPE_LEN,
    MCEXPR_UNARY_TYPE_REF,
    MCEXPR_UNARY_TYPE_DEREF,
};

// expressions

ast_declare_expr(value, ast_noargs)
    enum mc_expression_value_type type;
    union {
        ml_integer integer;
        ml_decimal decimal;
        mc_strtable_index_t string;
        bool boolean;
    } value;
};

ast_declare_expr(function, ast_noargs)
    struct mc_ast_function *ref;
};

ast_declare_expr(global, ast_noargs)
    enum mc_expression_global_type type;
};

ast_declare_expr(leave, ast_noargs)
    struct mc_ast_expression *expression;
};

ast_declare_expr(break, ast_noargs) };

ast_declare_expr(continue, ast_noargs) };

ast_declare_expr(variable, ast_noargs)
    bool ignore_mutable;
    mc_strtable_index_t index;
};

ast_declare_expr(binary, ast_noargs)
    enum mc_expression_binary_type type;
    struct mc_ast_expression *a;
    struct mc_ast_expression *b;
};

ast_declare_expr(unary, ast_noargs)
    enum mc_expression_unary_type type;
    struct mc_ast_expression *expression;
};

ast_declare_expr(increment, ast_noargs)
    bool is_decrement;
    bool is_postfix;
    struct mc_ast_expression *expression;
};

ast_declare_expr(vector, ast_args(size_t count))
    size_t count;
    struct mc_ast_expression **expressions;
};

ast_declare_expr(table, ast_args(size_t count))
    size_t count;
    struct mc_ast_expression **keys;
    struct mc_ast_expression **values;
};

ast_declare_expr(call, ast_args(size_t args_count))
    size_t args_count;
    bool extract_callable;
    struct mc_ast_expression *self;
    struct mc_ast_expression *callable;
    struct mc_ast_expression **arguments;
};

ast_declare_expr(access, ast_noargs)
    struct mc_ast_expression *container;
    struct mc_ast_expression *key;
};

ast_declare_expr(block, ast_args(size_t count))
    size_t count;
    bool inlined;
    struct mc_ast_statement **statements;
};

ast_declare_expr(if, ast_noargs)
    struct mc_ast_expression *condition;
    struct mc_ast_statement *if_statement;
    struct mc_ast_statement *else_statement;
};

ast_declare_expr(when, ast_args(size_t count))
    size_t count;
    struct mc_ast_expression *expression;
    struct mc_ast_expression **if_conditions;
    struct mc_ast_statement **if_statements;
    struct mc_ast_statement *else_statement;
};

// statements

ast_declare_stmt(eval, ast_noargs)
    struct mc_ast_expression *expression;
};

ast_declare_stmt(pass, ast_noargs) };

ast_declare_stmt(yield, ast_noargs) };

ast_declare_stmt(while, ast_noargs)
    bool first_condition;
    struct mc_ast_expression *condition;
    struct mc_ast_statement *statement;
};

ast_declare_stmt(for, ast_noargs)
    struct mc_ast_statement *initial;
    struct mc_ast_expression *condition;
    struct mc_ast_statement *increment;
    struct mc_ast_statement *statement;
};

ast_declare_stmt(assigment, ast_args(size_t count))
    struct mc_ast_expression *expression;
    ast_extract_part(struct mc_ast_expression *);
};

ast_declare_stmt(declaration, ast_args(size_t count))
    bool mutable;
    struct mc_ast_expression *expression;
    ast_extract_part(struct mc_ast_expression_variable *);
};

ast_declare_stmt(iterator, ast_noargs)
    struct mc_ast_statement_declaration *declaration;
    struct mc_ast_expression *key;
    struct mc_ast_expression *value;
    struct mc_ast_statement *statement;
};

// asm

enum mc_asm_data_type {
    MCADT_NIL,
    MCADT_INTEGER,
    MCADT_DECIMAL,
    MCADT_BOOLEAN,
    MCADT_STRING,
};

enum mc_asm_argument_type {
    MCAAT_WORD,
    MCAAT_NUMBER
};

struct mc_asm_data {
    mc_strtable_index_t name;
    enum mc_asm_data_type type;
    union {
        ml_integer integer;
        ml_decimal decimal;
        bool boolean;
        mc_strtable_index_t string;
    };
};

struct mc_asm_argument {
    enum mc_asm_argument_type type;
    union {
        mc_strtable_index_t word;
        ml_integer number;
    };
};

struct mc_asm_instruction {
    ml_line line;
    morphine_opcode_t opcode;
    struct mc_asm_argument arguments[MORPHINE_INSTRUCTION_ARGS_COUNT];
};

struct mc_asm_anchor {
    mc_strtable_index_t anchor;
    size_t instruction;
};

ast_declare_expr(
    asm,
    ast_args(size_t data_count, size_t slots_count, size_t code_count, size_t anchors_count)
)
    bool has_emitter;
    mc_strtable_index_t emitter;

    size_t data_count;
    size_t slots_count;
    size_t code_count;
    size_t anchors_count;

    struct mc_asm_data *data;
    mc_strtable_index_t *slots;
    struct mc_asm_instruction *code;
    struct mc_asm_anchor *anchors;
};

#undef ast_extract_part
#undef ast_args
#undef ast_noargs
#undef ast_declare_node
#undef ast_declare_expr
#undef ast_declare_stmt
