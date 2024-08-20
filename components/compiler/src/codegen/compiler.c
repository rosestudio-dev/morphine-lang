//
// Created by why-iskra on 18.08.2024.
//

#include "compiler.h"
#include "instruction.h"

#define ARGS_LIMIT 256

#define decl_data_sized(t, size) struct t##_data *data = codegen_saved(C); do { if (data == NULL) { data = codegen_alloc_saved_uni(C, sizeof(struct t##_data) + (size)); } } while(0)
#define decl_data(t)             decl_data_sized(t, 0)

#define decl_expr(n) void codegen_compile_expression_##n(struct codegen_controller *C, struct mc_ast_expression_##n *expression, size_t state)
#define decl_stmt(n) void codegen_compile_statement_##n(struct codegen_controller *C, struct mc_ast_statement_##n *statement, size_t state)
#define decl_set(n)  void codegen_compile_set_##n(struct codegen_controller *C, struct mc_ast_expression_##n *expression, size_t state)

static void get_variable(
    struct codegen_controller *C,
    mc_strtable_index_t name,
    struct instruction_slot slot
) {
    struct variable_info info = codegen_get_variable(C, name);
    switch (info.type) {
        case VIT_VARIABLE:
            codegen_instruction_MOVE(C, info.variable, slot);
            break;
        case VIT_STATIC:
            codegen_instruction_RECURSION(C, slot);
            codegen_instruction_GET_STATIC(C, slot, info.static_variable, slot);
            break;
        case VIT_ARGUMENT:
            codegen_instruction_ARG(C, info.argument, slot);
            break;
        case VIT_RECURSIVE:
            codegen_instruction_RECURSION(C, slot);
            break;
        case VIT_CLOSURE:
            codegen_instruction_RECURSION(C, slot);
            codegen_instruction_GET_CLOSURE(C, slot, info.closure_variable, slot);
            break;
        case VIT_NOT_FOUND:
            codegen_errorf(C, "variable '%s' not found", codegen_string(C, name).string);
    }
}

// set

decl_set(variable) {
    (void) state;
    struct variable_info info = codegen_get_variable(C, expression->index);

    if (!expression->ignore_mutable && !info.mutable) {
        codegen_errorf(C, "variable '%s' is immutable", codegen_string(C, expression->index).string);
    }

    switch (info.type) {
        case VIT_VARIABLE:
            codegen_instruction_MOVE(C, codegen_result(C), info.variable);
            break;
        case VIT_STATIC: {
            struct instruction_slot slot = codegen_declare_temporary(C);
            codegen_instruction_RECURSION(C, slot);
            codegen_instruction_SET_STATIC(C, slot, info.static_variable, codegen_result(C));
            break;
        }
        case VIT_ARGUMENT:
            codegen_errorf(C, "cannot be set to argument");
        case VIT_RECURSIVE:
            codegen_errorf(C, "cannot be set to function");
        case VIT_CLOSURE: {
            struct instruction_slot slot = codegen_declare_temporary(C);
            codegen_instruction_RECURSION(C, slot);
            codegen_instruction_SET_CLOSURE(C, slot, info.static_variable, codegen_result(C));
            break;
        }
        case VIT_NOT_FOUND:
            codegen_errorf(C, "variable '%s' not found", codegen_string(C, expression->index).string);
    }

    codegen_complete(C);
}

struct set_access_data {
    struct instruction_slot key;
    struct instruction_slot container;
};

decl_set(access) {
    decl_data(set_access);

    switch (state) {
        case 0:
            data->key = codegen_declare_temporary(C);
            data->container = codegen_declare_temporary(C);
            codegen_expression(C, expression->container, data->container, 1);
        case 1:
            codegen_expression(C, expression->key, data->key, 2);
        case 2:
            codegen_instruction_SET(C, data->container, data->key, codegen_result(C));
            codegen_complete(C);
        default:
            break;
    }
}

// expressions

decl_expr(value) {
    (void) state;
    switch (expression->type) {
        case MCEXPR_VALUE_TYPE_NIL:
            codegen_instruction_LOAD(
                C,
                codegen_add_constant_nil(C),
                codegen_result(C)
            );
            codegen_complete(C);
        case MCEXPR_VALUE_TYPE_INT:
            codegen_instruction_LOAD(
                C,
                codegen_add_constant_int(C, expression->value.integer),
                codegen_result(C)
            );
            codegen_complete(C);
        case MCEXPR_VALUE_TYPE_DEC:
            codegen_instruction_LOAD(
                C,
                codegen_add_constant_dec(C, expression->value.decimal),
                codegen_result(C)
            );
            codegen_complete(C);
        case MCEXPR_VALUE_TYPE_STR:
            codegen_instruction_LOAD(
                C,
                codegen_add_constant_str(C, expression->value.string),
                codegen_result(C)
            );
            codegen_complete(C);
        case MCEXPR_VALUE_TYPE_BOOL:
            codegen_instruction_LOAD(
                C,
                codegen_add_constant_bool(C, expression->value.boolean),
                codegen_result(C)
            );
            codegen_complete(C);
    }
}

struct binary_data {
    anchor_t anchor_if;
    anchor_t anchor_else;
    struct instruction_slot slot;
};

decl_expr(binary) {
    decl_data(binary);

    if (expression->type == MCEXPR_BINARY_TYPE_AND) {
        switch (state) {
            case 0:
                codegen_expression(C, expression->a, codegen_result(C), 1);
            case 1:
                data->slot = codegen_declare_temporary(C);
                data->anchor_if = codegen_add_anchor(C);
                data->anchor_else = codegen_add_anchor(C);
                codegen_instruction_JUMP_IF(C, codegen_result(C), data->anchor_if, data->anchor_else);
                codegen_anchor_change(C, data->anchor_if);
                codegen_expression(C, expression->b, data->slot, 2);
            case 2:
                codegen_instruction_AND(C, codegen_result(C), data->slot, codegen_result(C));
                codegen_anchor_change(C, data->anchor_else);
                codegen_complete(C);
            default:
                break;
        }
    } else if (expression->type == MCEXPR_BINARY_TYPE_OR) {
        switch (state) {
            case 0:
                codegen_expression(C, expression->a, codegen_result(C), 1);
            case 1:
                data->slot = codegen_declare_temporary(C);
                data->anchor_if = codegen_add_anchor(C);
                data->anchor_else = codegen_add_anchor(C);
                codegen_instruction_JUMP_IF(C, codegen_result(C), data->anchor_if, data->anchor_else);
                codegen_anchor_change(C, data->anchor_else);
                codegen_expression(C, expression->b, data->slot, 2);
            case 2:
                codegen_instruction_OR(C, codegen_result(C), data->slot, codegen_result(C));
                codegen_anchor_change(C, data->anchor_if);
                codegen_complete(C);
            default:
                break;
        }
    } else {
        switch (state) {
            case 0:
                codegen_expression(C, expression->a, codegen_result(C), 1);
            case 1:
                data->slot = codegen_declare_temporary(C);
                codegen_expression(C, expression->b, data->slot, 2);
            case 2:
                switch (expression->type) {
                    case MCEXPR_BINARY_TYPE_ADD:
                        codegen_instruction_ADD(C, codegen_result(C), data->slot, codegen_result(C));
                        codegen_complete(C);
                    case MCEXPR_BINARY_TYPE_SUB:
                        codegen_instruction_SUB(C, codegen_result(C), data->slot, codegen_result(C));
                        codegen_complete(C);
                    case MCEXPR_BINARY_TYPE_MUL:
                        codegen_instruction_MUL(C, codegen_result(C), data->slot, codegen_result(C));
                        codegen_complete(C);
                    case MCEXPR_BINARY_TYPE_DIV:
                        codegen_instruction_DIV(C, codegen_result(C), data->slot, codegen_result(C));
                        codegen_complete(C);
                    case MCEXPR_BINARY_TYPE_MOD:
                        codegen_instruction_MOD(C, codegen_result(C), data->slot, codegen_result(C));
                        codegen_complete(C);
                    case MCEXPR_BINARY_TYPE_EQUAL:
                        codegen_instruction_EQUAL(C, codegen_result(C), data->slot, codegen_result(C));
                        codegen_complete(C);
                    case MCEXPR_BINARY_TYPE_LESS:
                        codegen_instruction_LESS(C, codegen_result(C), data->slot, codegen_result(C));
                        codegen_complete(C);
                    case MCEXPR_BINARY_TYPE_CONCAT:
                        codegen_instruction_CONCAT(C, codegen_result(C), data->slot, codegen_result(C));
                        codegen_complete(C);
                    case MCEXPR_BINARY_TYPE_AND:
                    case MCEXPR_BINARY_TYPE_OR:
                        break;
                }
            default:
                break;
        }
    }
}

decl_expr(unary) {
    switch (state) {
        case 0:
            codegen_expression(C, expression->expression, codegen_result(C), 1);
        case 1:
            switch (expression->type) {
                case MCEXPR_UNARY_TYPE_NEGATE:
                    codegen_instruction_NEGATIVE(C, codegen_result(C), codegen_result(C));
                    codegen_complete(C);
                case MCEXPR_UNARY_TYPE_NOT:
                    codegen_instruction_NOT(C, codegen_result(C), codegen_result(C));
                    codegen_complete(C);
                case MCEXPR_UNARY_TYPE_TYPE:
                    codegen_instruction_TYPE(C, codegen_result(C), codegen_result(C));
                    codegen_complete(C);
                case MCEXPR_UNARY_TYPE_LEN:
                    codegen_instruction_LENGTH(C, codegen_result(C), codegen_result(C));
                    codegen_complete(C);
                case MCEXPR_UNARY_TYPE_REF:
                    codegen_instruction_REF(C, codegen_result(C), codegen_result(C));
                    codegen_complete(C);
                case MCEXPR_UNARY_TYPE_DEREF:
                    codegen_instruction_DEREF(C, codegen_result(C), codegen_result(C));
                    codegen_complete(C);
            }
        default:
            break;
    }
}

struct increment_data {
    struct instruction_slot slot;
};

decl_expr(increment) {
    decl_data(increment);
    switch (state) {
        case 0:
            codegen_expression(C, expression->expression, codegen_result(C), 1);
        case 1:
            data->slot = codegen_declare_temporary(C);
            size_t constant = codegen_add_constant_int(C, 1);
            codegen_instruction_LOAD(C, constant, data->slot);

            if (expression->is_postfix) {
                if (expression->is_decrement) {
                    codegen_instruction_SUB(C, codegen_result(C), data->slot, data->slot);
                } else {
                    codegen_instruction_ADD(C, codegen_result(C), data->slot, data->slot);
                }

                codegen_set(C, expression->expression, data->slot, 2);
            } else {
                if (expression->is_decrement) {
                    codegen_instruction_SUB(C, codegen_result(C), data->slot, codegen_result(C));
                } else {
                    codegen_instruction_ADD(C, codegen_result(C), data->slot, codegen_result(C));
                }

                codegen_set(C, expression->expression, codegen_result(C), 2);
            }
        case 2:
            codegen_complete(C);
        default:
            break;
    }
}

decl_expr(global) {
    (void) state;
    switch (expression->type) {
        case MCEXPR_GLOBAL_TYPE_ENV:
            codegen_instruction_ENV(C, codegen_result(C));
            codegen_complete(C);
        case MCEXPR_GLOBAL_TYPE_SELF:
            codegen_instruction_SELF(C, codegen_result(C));
            codegen_complete(C);
    }
}

struct table_data {
    struct instruction_slot key;
    struct instruction_slot value;
    size_t index;
};

decl_expr(table) {
    decl_data(table);
    switch (state) {
        case 0:
            data->index = 0;
            data->key = codegen_declare_temporary(C);
            data->value = codegen_declare_temporary(C);
            codegen_instruction_TABLE(C, codegen_result(C));
            codegen_jump(C, 1);
        case 1:
            if (data->index >= expression->count) {
                codegen_complete(C);
            } else {
                codegen_expression(C, expression->keys[data->index], data->key, 2);
            }
        case 2:
            codegen_expression(C, expression->values[data->index], data->value, 3);
        case 3:
            codegen_instruction_SET(C, codegen_result(C), data->key, data->value);
            data->index++;
            codegen_jump(C, 1);
        default:
            break;
    }
}

struct vector_data {
    struct instruction_slot key;
    struct instruction_slot value;
    size_t index;
    size_t size;
};

decl_expr(vector) {
    decl_data(vector);
    switch (state) {
        case 0:
            data->index = 0;
            data->size = expression->count;
            data->key = codegen_declare_temporary(C);
            data->value = codegen_declare_temporary(C);
            codegen_instruction_VECTOR(C, codegen_result(C), data->size);
            codegen_jump(C, 1);
        case 1:
            if (data->index >= data->size) {
                codegen_complete(C);
            } else {
                codegen_expression(C, expression->expressions[data->index], data->value, 2);
            }
        case 2: {
            size_t constant = codegen_add_constant_index(C, data->index);
            codegen_instruction_LOAD(C, constant, data->key);
            codegen_instruction_SET(C, codegen_result(C), data->key, data->value);
            data->index++;
            codegen_jump(C, 1);
        }
        default:
            break;
    }
}

struct access_data {
    struct instruction_slot slot;
};

decl_expr(access) {
    decl_data(access);
    switch (state) {
        case 0:
            codegen_expression(C, expression->container, codegen_result(C), 1);
        case 1:
            data->slot = codegen_declare_temporary(C);
            codegen_expression(C, expression->key, data->slot, 2);
        case 2:
            codegen_instruction_GET(C, codegen_result(C), data->slot, codegen_result(C));
            codegen_complete(C);
        default:
            break;
    }
}

struct call_data {
    size_t index;
    struct instruction_slot self;
    struct instruction_slot *slots;
};

decl_expr(call) {
    if (expression->args_count > ARGS_LIMIT) {
        codegen_errorf(C, "arguments limit");
    }

    decl_data_sized(call, sizeof(struct instruction_slot) * expression->args_count);
    data->slots = ((void *) data) + sizeof(struct call_data);

    switch (state) {
        case 0:
            codegen_expression(C, expression->callable, codegen_result(C), 1);
        case 1:
            data->self = codegen_declare_temporary(C);
            codegen_expression(C, expression->self, data->self, 2);
        case 2:
            if (expression->extract_callable) {
                codegen_instruction_GET(C, data->self, codegen_result(C), codegen_result(C));
            }

            data->index = 0;
            for (size_t i = 0; i < expression->args_count; i++) {
                data->slots[i] = codegen_declare_temporary(C);
            }
            codegen_jump(C, 3);
        case 3:
            if (data->index >= expression->args_count) {
                codegen_jump(C, 4);
            } else {
                size_t index = data->index;
                data->index++;
                codegen_expression(C, expression->arguments[index], data->slots[index], 3);
            }
        case 4:
            for (size_t i = 0; i < expression->args_count; i++) {
                codegen_instruction_PARAM(C, data->slots[i], i);
            }

            codegen_instruction_CALL(C, codegen_result(C), expression->args_count, data->self);
            codegen_instruction_RESULT(C, codegen_result(C));
            codegen_complete(C);
        default:
            break;
    }
}

struct expression_block_data {
    size_t index;
};

decl_expr(block) {
    decl_data(expression_block);
    switch (state) {
        case 0:
            codegen_enter_scope(C, false);
            data->index = 0;
            codegen_jump(C, 1);
        case 1:
            if (data->index >= expression->count) {
                codegen_expression(C, expression->expression, codegen_result(C), 3);
            } else {
                codegen_statement(C, expression->statements[data->index], 2);
            }
        case 2:
            data->index++;
            codegen_jump(C, 1);
        case 3:
            codegen_exit_scope(C);
            codegen_complete(C);
        default:
            break;
    }
}

struct expression_if_data {
    anchor_t anchor_if;
    anchor_t anchor_else;
    anchor_t anchor_end;
};

decl_expr(if) {
    decl_data(expression_if);
    switch (state) {
        case 0:
            codegen_expression(C, expression->condition, codegen_result(C), 1);
        case 1:
            data->anchor_if = codegen_add_anchor(C);
            data->anchor_else = codegen_add_anchor(C);
            data->anchor_end = codegen_add_anchor(C);
            codegen_instruction_JUMP_IF(C, codegen_result(C), data->anchor_if, data->anchor_else);
            codegen_anchor_change(C, data->anchor_if);
            codegen_expression(C, expression->if_expression, codegen_result(C), 2);
        case 2:
            codegen_instruction_JUMP(C, data->anchor_end);
            codegen_anchor_change(C, data->anchor_else);
            codegen_expression(C, expression->else_expression, codegen_result(C), 3);
        case 3:
            codegen_anchor_change(C, data->anchor_end);
            codegen_complete(C);
        default:
            break;
    }
}

decl_expr(function) {
    switch (state) {
        case 0:
            codegen_function(C, expression->ref, 1);
        case 1: {
            size_t constant = codegen_add_constant_fun(C, expression->ref);
            codegen_instruction_LOAD(C, constant, codegen_result(C));

            struct variable *variables;
            size_t size = codegen_closures(C, expression->ref, &variables);

            if (size > 0) {
                struct instruction_slot slot = codegen_declare_temporary(C);
                codegen_instruction_CLOSURE(C, codegen_result(C), size, codegen_result(C));

                for (size_t i = 0; i < size; i++) {
                    get_variable(C, codegen_variable_name(variables, i), slot);
                    codegen_instruction_SET_CLOSURE(C, codegen_result(C), i, slot);
                }
            }

            codegen_complete(C);
        }
        default:
            break;
    }
}

decl_expr(variable) {
    (void) state;
    get_variable(C, expression->index, codegen_result(C));
    codegen_complete(C);
}

// statements

struct declaration_data {
    struct instruction_slot slot;
    struct instruction_slot key;
    size_t index;
};

decl_stmt(declaration) {
    decl_data(declaration);
    switch (state) {
        case 0:
            data->slot = codegen_declare_temporary(C);
            codegen_expression(C, statement->expression, data->slot, 1);
        case 1:
            if (statement->is_decompose) {
                data->index = 0;
                data->key = codegen_declare_temporary(C);
                codegen_jump(C, 2);
            } else {
                codegen_declare_variable(C, statement->value->index, statement->mutable);
                codegen_set(C, mcapi_ast_variable2expression(statement->value), data->slot, 4);
            }
        case 2:
            if (data->index < statement->decompose.size) {
                codegen_expression(C, statement->decompose.keys[data->index], data->key, 3);
            } else {
                codegen_complete(C);
            }
        case 3: {
            size_t index = data->index;
            data->index++;

            codegen_instruction_GET(C, data->slot, data->key, data->key);
            codegen_declare_variable(C, statement->decompose.values[index]->index, statement->mutable);
            codegen_set(C, mcapi_ast_variable2expression(statement->decompose.values[index]), data->key, 2);
        }
        case 4:
            codegen_complete(C);
        default:
            break;
    }
}

struct assigment_data {
    struct instruction_slot slot;
    struct instruction_slot key;
    size_t index;
};

decl_stmt(assigment) {
    decl_data(assigment);
    switch (state) {
        case 0:
            data->slot = codegen_declare_temporary(C);
            codegen_expression(C, statement->expression, data->slot, 1);
        case 1:
            if (statement->is_decompose) {
                data->index = 0;
                data->key = codegen_declare_temporary(C);
                codegen_jump(C, 2);
            } else {
                codegen_set(C, statement->value, data->slot, 4);
            }
        case 2:
            if (data->index < statement->decompose.size) {
                codegen_expression(C, statement->decompose.keys[data->index], data->key, 3);
            } else {
                codegen_complete(C);
            }
        case 3: {
            size_t index = data->index;
            data->index++;

            codegen_instruction_GET(C, data->slot, data->key, data->key);
            codegen_set(C, statement->decompose.values[index], data->key, 2);
        }
        case 4:
            codegen_complete(C);
        default:
            break;
    }
}

struct while_data {
    struct instruction_slot slot;
    anchor_t anchor;
};

decl_stmt(while) {
    decl_data(while);

    if (statement->first_condition) {
        switch (state) {
            case 0:
                codegen_enter_scope(C, true);
                data->slot = codegen_declare_temporary(C);
                data->anchor = codegen_add_anchor(C);

                codegen_anchor_change(C, codegen_scope_continue_anchor(C));
                codegen_expression(C, statement->condition, data->slot, 1);
            case 1:
                codegen_instruction_JUMP_IF(C, data->slot, data->anchor, codegen_scope_break_anchor(C));
                codegen_anchor_change(C, data->anchor);

                codegen_statement(C, statement->statement, 2);
            case 2:
                codegen_instruction_JUMP(C, codegen_scope_continue_anchor(C));
                codegen_anchor_change(C, codegen_scope_break_anchor(C));
                codegen_exit_scope(C);
                codegen_complete(C);
            default:
                break;
        }
    } else {
        switch (state) {
            case 0:
                codegen_enter_scope(C, true);
                data->slot = codegen_declare_temporary(C);
                data->anchor = codegen_add_anchor(C);
                codegen_statement(C, statement->statement, 1);
            case 1:
                codegen_anchor_change(C, codegen_scope_continue_anchor(C));
                codegen_expression(C, statement->condition, data->slot, 2);
            case 2:
                codegen_instruction_JUMP_IF(C, data->slot, data->anchor, codegen_scope_break_anchor(C));
                codegen_anchor_change(C, codegen_scope_break_anchor(C));
                codegen_exit_scope(C);
                codegen_complete(C);
            default:
                break;
        }
    }
}

struct for_data {
    struct instruction_slot slot;
    anchor_t anchor;
};

decl_stmt(for) {
    decl_data(for);

    switch (state) {
        case 0:
            codegen_enter_scope(C, true);
            codegen_statement(C, statement->initial, 1);
        case 1:
            data->slot = codegen_declare_temporary(C);
            data->anchor = codegen_add_anchor(C);
            codegen_expression(C, statement->condition, data->slot, 2);
        case 2: {
            anchor_t anchor = codegen_add_anchor(C);
            codegen_instruction_JUMP_IF(C, data->slot, anchor, codegen_scope_break_anchor(C));

            codegen_anchor_change(C, anchor);
            codegen_statement(C, statement->statement, 3);
        }
        case 3:
            codegen_anchor_change(C, codegen_scope_continue_anchor(C));
            codegen_statement(C, statement->increment, 4);
        case 4:
            codegen_instruction_JUMP(C, data->anchor);
            codegen_anchor_change(C, codegen_scope_break_anchor(C));
            codegen_exit_scope(C);
            codegen_complete(C);
        default:
            break;
    }
}

struct iterator_data {
    struct instruction_slot slot;
    struct instruction_slot key;
    struct instruction_slot value;
    size_t index;
};

decl_stmt(iterator) {
    decl_data(iterator);
    switch (state) {
        case 0:
            data->slot = codegen_declare_temporary(C);
            codegen_enter_scope(C, true);
            codegen_expression(C, statement->declaration->expression, data->slot, 1);
        case 1:
            codegen_instruction_ITERATOR(C, data->slot, data->slot);

            data->key = codegen_declare_temporary(C);
            data->value = codegen_declare_temporary(C);

            if (statement->declaration->is_decompose && statement->declaration->decompose.size == 2) {
                codegen_jump(C, 2);
            } else {
                size_t constant_key = codegen_add_constant_cstr(C, "key");
                size_t constant_value = codegen_add_constant_cstr(C, "value");

                codegen_instruction_LOAD(C, constant_key, data->key);
                codegen_instruction_LOAD(C, constant_value, data->value);
                codegen_jump(C, 4);
            }
        case 2:
            codegen_expression(C, statement->declaration->decompose.keys[0], data->key, 3);
        case 3:
            codegen_expression(C, statement->declaration->decompose.keys[1], data->value, 4);
        case 4:
            codegen_instruction_ITERATOR_INIT(C, data->slot, data->key, data->value);
            codegen_jump(C, 5);
        case 5:
            codegen_anchor_change(C, codegen_scope_continue_anchor(C));
            codegen_instruction_ITERATOR_HAS(C, data->slot, data->value);

            anchor_t anchor = codegen_add_anchor(C);
            codegen_instruction_JUMP_IF(C, data->value, anchor, codegen_scope_break_anchor(C));
            codegen_anchor_change(C, anchor);
            codegen_instruction_ITERATOR_NEXT(C, data->slot, data->value);

            if (statement->declaration->is_decompose) {
                data->index = 0;
                codegen_jump(C, 6);
            } else {
                codegen_declare_variable(
                    C,
                    statement->declaration->value->index,
                    statement->declaration->mutable
                );
                codegen_set(C, mcapi_ast_variable2expression(statement->declaration->value), data->value, 8);
            }
        case 6:
            if (data->index < statement->declaration->decompose.size) {
                codegen_expression(C, statement->declaration->decompose.keys[data->index], data->key, 7);
            } else {
                codegen_jump(C, 8);
            }
        case 7: {
            size_t index = data->index;
            data->index++;

            codegen_instruction_GET(C, data->value, data->key, data->key);
            codegen_declare_variable(
                C,
                statement->declaration->decompose.values[index]->index,
                statement->declaration->mutable
            );
            codegen_set(
                C,
                mcapi_ast_variable2expression(statement->declaration->decompose.values[index]),
                data->key,
                6
            );
        }
        case 8:
            codegen_statement(C, statement->statement, 9);
        case 9:
            codegen_instruction_JUMP(C, codegen_scope_continue_anchor(C));
            codegen_anchor_change(C, codegen_scope_break_anchor(C));
            codegen_exit_scope(C);
            codegen_complete(C);
        default:
            break;
    }
}

decl_stmt(eval) {
    switch (state) {
        case 0:
            codegen_expression(C, statement->expression, codegen_declare_temporary(C), 1);
        case 1:
            codegen_complete(C);
        default:
            break;
    }
}

decl_stmt(simple) {
    (void) state;
    switch (statement->type) {
        case MCSTMT_SIMPLE_TYPE_PASS:
            codegen_complete(C);
        case MCSTMT_SIMPLE_TYPE_YIELD:
            codegen_instruction_YIELD(C);
            codegen_complete(C);
        case MCSTMT_SIMPLE_TYPE_BREAK:
            codegen_instruction_JUMP(C, codegen_scope_break_anchor(C));
            codegen_complete(C);
        case MCSTMT_SIMPLE_TYPE_CONTINUE:
            codegen_instruction_JUMP(C, codegen_scope_continue_anchor(C));
            codegen_complete(C);
    }
}

struct leave_data {
    struct instruction_slot slot;
};

decl_stmt(leave) {
    if (statement->expression != NULL) {
        decl_data(leave);
        switch (state) {
            case 0:
                data->slot = codegen_declare_temporary(C);
                codegen_expression(C, statement->expression, data->slot, 1);
            case 1:
                codegen_instruction_LEAVE(C, data->slot);
                codegen_complete(C);
            default:
                break;
        }
    } else {
        struct instruction_slot slot = codegen_declare_temporary(C);
        size_t constant = codegen_add_constant_nil(C);
        codegen_instruction_LOAD(C, constant, slot);
        codegen_instruction_LEAVE(C, slot);
        codegen_complete(C);
    }
}

struct statement_block_data {
    size_t index;
};

decl_stmt(block) {
    decl_data(statement_block);
    switch (state) {
        case 0:
            codegen_enter_scope(C, false);
            data->index = 0;
            codegen_jump(C, 1);
        case 1:
            if (data->index >= statement->count) {
                codegen_exit_scope(C);
                codegen_complete(C);
            } else {
                codegen_statement(C, statement->statements[data->index], 2);
            }
        case 2:
            data->index++;
            codegen_jump(C, 1);
        default:
            break;
    }
}

struct statement_if_data {
    struct instruction_slot slot;
    anchor_t anchor_if;
    anchor_t anchor_else;
    anchor_t anchor_end;
};

decl_stmt(if) {
    decl_data(statement_if);
    switch (state) {
        case 0:
            data->slot = codegen_declare_temporary(C);
            codegen_expression(C, statement->condition, data->slot, 1);
        case 1:
            data->anchor_if = codegen_add_anchor(C);
            data->anchor_else = codegen_add_anchor(C);
            data->anchor_end = codegen_add_anchor(C);
            codegen_instruction_JUMP_IF(C, data->slot, data->anchor_if, data->anchor_else);
            codegen_anchor_change(C, data->anchor_if);
            codegen_statement(C, statement->if_statement, 2);
        case 2:
            codegen_instruction_JUMP(C, data->anchor_end);
            codegen_anchor_change(C, data->anchor_else);
            codegen_statement(C, statement->else_statement, 3);
        case 3:
            codegen_anchor_change(C, data->anchor_end);
            codegen_complete(C);
        default:
            break;
    }
}