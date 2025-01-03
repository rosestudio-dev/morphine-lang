//
// Created by why-iskra on 05.09.2024.
//

#include "implementation.h"

#define decl_data_sized(t, size) struct t##_data *data = serializer_saved(C); do { if (data == NULL) { data = serializer_alloc_saved_uni(C, sizeof(struct t##_data) + (size)); } } while(0)
#define decl_data(t)             decl_data_sized(t, 0)

#define decl_expr(n) void serialize_expression_##n(struct serializer_controller *C, struct mc_ast_expression_##n *expression, size_t state)
#define decl_stmt(n) void serialize_statement_##n(struct serializer_controller *C, struct mc_ast_statement_##n *statement, size_t state)

decl_expr(value) {
    if (state != 0) {
        return;
    }

    switch (expression->type) {
        case MCEXPR_VALUE_TYPE_NIL:
            serializer_push_cstr(C, "nil");
            break;
        case MCEXPR_VALUE_TYPE_INT:
            serializer_push_cstr(C, "integer");
            break;
        case MCEXPR_VALUE_TYPE_DEC:
            serializer_push_cstr(C, "decimal");
            break;
        case MCEXPR_VALUE_TYPE_STR:
            serializer_push_cstr(C, "string");
            break;
        case MCEXPR_VALUE_TYPE_BOOL:
            serializer_push_cstr(C, "boolean");
            break;
    }
    serializer_set(C, "type");

    switch (expression->type) {
        case MCEXPR_VALUE_TYPE_NIL:
            break;
        case MCEXPR_VALUE_TYPE_INT:
            serializer_push_integer(C, expression->value.integer);
            serializer_set(C, "integer");
            break;
        case MCEXPR_VALUE_TYPE_DEC:
            serializer_push_decimal(C, expression->value.decimal);
            serializer_set(C, "decimal");
            break;
        case MCEXPR_VALUE_TYPE_STR:
            serializer_push_string(C, expression->value.string);
            serializer_set(C, "string");
            break;
        case MCEXPR_VALUE_TYPE_BOOL:
            serializer_push_boolean(C, expression->value.boolean);
            serializer_set(C, "boolean");
            break;
    }

    serializer_complete(C);
}

decl_expr(binary) {
    switch (state) {
        case 0: {
            switch (expression->type) {
                case MCEXPR_BINARY_TYPE_ADD:
                    serializer_push_cstr(C, "add");
                    break;
                case MCEXPR_BINARY_TYPE_SUB:
                    serializer_push_cstr(C, "sub");
                    break;
                case MCEXPR_BINARY_TYPE_MUL:
                    serializer_push_cstr(C, "mul");
                    break;
                case MCEXPR_BINARY_TYPE_DIV:
                    serializer_push_cstr(C, "div");
                    break;
                case MCEXPR_BINARY_TYPE_MOD:
                    serializer_push_cstr(C, "mod");
                    break;
                case MCEXPR_BINARY_TYPE_EQUAL:
                    serializer_push_cstr(C, "equal");
                    break;
                case MCEXPR_BINARY_TYPE_LESS:
                    serializer_push_cstr(C, "less");
                    break;
                case MCEXPR_BINARY_TYPE_CONCAT:
                    serializer_push_cstr(C, "concat");
                    break;
                case MCEXPR_BINARY_TYPE_AND:
                    serializer_push_cstr(C, "and");
                    break;
                case MCEXPR_BINARY_TYPE_OR:
                    serializer_push_cstr(C, "or");
                    break;
            }
            serializer_set(C, "type");

            serializer_enter_expression(C, expression->a, 1);
        }
        case 1: {
            serializer_set(C, "a");
            serializer_enter_expression(C, expression->b, 2);
        }
        case 2: {
            serializer_set(C, "b");
            serializer_complete(C);
        }
        default:
            break;
    }
}

decl_expr(unary) {
    switch (state) {
        case 0: {
            switch (expression->type) {
                case MCEXPR_UNARY_TYPE_NEGATE:
                    serializer_push_cstr(C, "negate");
                    break;
                case MCEXPR_UNARY_TYPE_NOT:
                    serializer_push_cstr(C, "not");
                    break;
                case MCEXPR_UNARY_TYPE_TYPE:
                    serializer_push_cstr(C, "type");
                    break;
                case MCEXPR_UNARY_TYPE_LEN:
                    serializer_push_cstr(C, "len");
                    break;
                case MCEXPR_UNARY_TYPE_REF:
                    serializer_push_cstr(C, "ref");
                    break;
                case MCEXPR_UNARY_TYPE_DEREF:
                    serializer_push_cstr(C, "deref");
                    break;
            }
            serializer_set(C, "type");
            serializer_enter_expression(C, expression->expression, 1);
        }
        case 1: {
            serializer_set(C, "expression");
            serializer_complete(C);
        }
        default:
            break;
    }
}

decl_expr(increment) {
    switch (state) {
        case 0: {
            serializer_push_boolean(C, expression->is_postfix);
            serializer_set(C, "ispostfix");

            serializer_push_boolean(C, expression->is_decrement);
            serializer_set(C, "isdecrement");

            serializer_enter_expression(C, expression->expression, 1);
        }
        case 1: {
            serializer_set(C, "expression");
            serializer_complete(C);
        }
        default:
            break;
    }
}

decl_expr(variable) {
    if (state != 0) {
        return;
    }

    serializer_push_string(C, expression->index);
    serializer_set(C, "name");
    serializer_push_boolean(C, expression->ignore_mutable);
    serializer_set(C, "ignoremutable");
    serializer_complete(C);
}

decl_expr(global) {
    if (state != 0) {
        return;
    }

    switch (expression->type) {
        case MCEXPR_GLOBAL_TYPE_ENV:
            serializer_push_cstr(C, "env");
            break;
        case MCEXPR_GLOBAL_TYPE_SELF:
            serializer_push_cstr(C, "self");
            break;
        case MCEXPR_GLOBAL_TYPE_INVOKED:
            serializer_push_cstr(C, "invoked");
            break;
    }

    serializer_set(C, "type");
    serializer_complete(C);
}

decl_expr(leave) {
    switch (state) {
        case 0: {
            if (expression->expression == NULL) {
                serializer_complete(C);
            }

            serializer_enter_expression(C, expression->expression, 1);
        }
        case 1: {
            serializer_set(C, "expression");
            serializer_complete(C);
        }
        default:
            break;
    }
}

decl_expr(break) {
    (void) expression;
    if (state != 0) {
        return;
    }

    serializer_complete(C);
}

decl_expr(continue) {
    (void) expression;
    if (state != 0) {
        return;
    }

    serializer_complete(C);
}

struct table_data {
    ml_size index;
};

decl_expr(table) {
    decl_data(table);

    switch (state) {
        case 0: {
            serializer_push_size(C, expression->count, NULL);
            serializer_set(C, "size");

            data->index = 0;
            serializer_push_vector(C, expression->count);
            serializer_jump(C, 1);
        }
        case 1: {
            if (data->index >= expression->count) {
                serializer_set(C, "keys");

                data->index = 0;
                serializer_push_vector(C, expression->count);
                serializer_jump(C, 3);
            } else {
                serializer_enter_expression(C, expression->keys[data->index], 2);
            }
        }
        case 2: {
            serializer_vector_set(C, data->index);
            data->index++;
            serializer_jump(C, 1);
        }
        case 3: {
            if (data->index >= expression->count) {
                serializer_set(C, "values");

                serializer_complete(C);
            } else {
                serializer_enter_expression(C, expression->values[data->index], 4);
            }
        }
        case 4: {
            serializer_vector_set(C, data->index);
            data->index++;
            serializer_jump(C, 3);
        }
        default:
            break;
    }
}

struct vector_data {
    ml_size index;
};

decl_expr(vector) {
    decl_data(vector);

    switch (state) {
        case 0: {
            data->index = 0;
            serializer_push_vector(C, expression->count);
            serializer_jump(C, 1);
        }
        case 1: {
            if (data->index >= expression->count) {
                serializer_set(C, "expressions");

                serializer_complete(C);
            } else {
                serializer_enter_expression(C, expression->expressions[data->index], 2);
            }
        }
        case 2: {
            serializer_vector_set(C, data->index);
            data->index++;
            serializer_jump(C, 1);
        }
        default:
            break;
    }
}

decl_expr(access) {
    switch (state) {
        case 0: {
            serializer_enter_expression(C, expression->key, 1);
        }
        case 1: {
            serializer_set(C, "key");
            serializer_enter_expression(C, expression->container, 2);
        }
        case 2: {
            serializer_set(C, "container");
            serializer_complete(C);
        }
        default:
            break;
    }
}

struct call_data {
    size_t index;
};

decl_expr(call) {
    decl_data(call);

    switch (state) {
        case 0: {
            serializer_push_boolean(C, expression->extract_callable);
            serializer_set(C, "extractcallable");

            serializer_enter_expression(C, expression->self, 1);
        }
        case 1: {
            serializer_set(C, "self");
            serializer_enter_expression(C, expression->callable, 2);
        }
        case 2: {
            serializer_set(C, "callable");

            data->index = 0;
            serializer_push_vector(C, expression->args_count);
            serializer_jump(C, 3);
        }
        case 3: {
            if (data->index >= expression->args_count) {
                serializer_set(C, "arguments");
                serializer_complete(C);
            } else {
                serializer_enter_expression(C, expression->arguments[data->index], 4);
            }
        }
        case 4: {
            serializer_vector_set(C, data->index);
            data->index++;
            serializer_jump(C, 3);
        }
        default:
            break;
    }
}

decl_expr(function) {
    switch (state) {
        case 0: {
            serializer_enter_function(C, expression->ref, 1);
        }
        case 1: {
            serializer_set(C, "function");
            serializer_complete(C);
        }
        default:
            break;
    }
}

struct block_data {
    size_t index;
};

decl_expr(block) {
    decl_data(block);

    switch (state) {
        case 0: {
            serializer_push_boolean(C, expression->inlined);
            serializer_set(C, "inlined");

            data->index = 0;
            serializer_push_vector(C, expression->count);
            serializer_jump(C, 1);
        }
        case 1: {
            if (data->index >= expression->count) {
                serializer_set(C, "statements");
                serializer_complete(C);
            } else {
                serializer_enter_statement(C, expression->statements[data->index], 2);
            }
        }
        case 2: {
            serializer_vector_set(C, data->index);
            data->index++;
            serializer_jump(C, 1);
        }
        default:
            break;
    }
}

decl_expr(if) {
    switch (state) {
        case 0: {
            serializer_enter_expression(C, expression->condition, 1);
        }
        case 1: {
            serializer_set(C, "condition");
            serializer_enter_statement(C, expression->if_statement, 2);
        }
        case 2: {
            serializer_set(C, "if");
            serializer_enter_statement(C, expression->else_statement, 3);
        }
        case 3: {
            serializer_set(C, "else");
            serializer_complete(C);
        }
        default:
            break;
    }
}

struct when_data {
    size_t index;
};

decl_expr(when) {
    decl_data(when);

    switch (state) {
        case 0: {
            serializer_enter_expression(C, expression->expression, 1);
        }
        case 1: {
            serializer_set(C, "expression");

            serializer_enter_statement(C, expression->else_statement, 2);
        }
        case 2: {
            serializer_set(C, "else");

            data->index = 0;
            serializer_push_vector(C, expression->count);
            serializer_jump(C, 3);
        }
        case 3: {
            if (data->index >= expression->count) {
                serializer_set(C, "conditions");
                serializer_jump(C, 5);
            } else {
                serializer_enter_expression(C, expression->if_conditions[data->index], 4);
            }
        }
        case 4: {
            serializer_vector_set(C, data->index);
            data->index++;
            serializer_jump(C, 3);
        }
        case 5: {
            data->index = 0;
            serializer_push_vector(C, expression->count);
            serializer_jump(C, 6);
        }
        case 6: {
            if (data->index >= expression->count) {
                serializer_set(C, "statements");
                serializer_complete(C);
            } else {
                serializer_enter_statement(C, expression->if_statements[data->index], 7);
            }
        }
        case 7: {
            serializer_vector_set(C, data->index);
            data->index++;
            serializer_jump(C, 6);
        }
        default:
            break;
    }
}

decl_stmt(pass) {
    (void) statement;
    if (state != 0) {
        return;
    }

    serializer_complete(C);
}

decl_stmt(yield) {
    (void) statement;
    if (state != 0) {
        return;
    }

    serializer_complete(C);
}

decl_stmt(eval) {
    switch (state) {
        case 0: {
            serializer_enter_expression(C, statement->expression, 1);
        }
        case 1: {
            serializer_set(C, "expression");
            serializer_complete(C);
        }
        default:
            break;
    }
}

decl_stmt(while) {
    switch (state) {
        case 0: {
            serializer_push_boolean(C, statement->first_condition);
            serializer_set(C, "firstcondition");

            serializer_enter_expression(C, statement->condition, 1);
        }
        case 1: {
            serializer_set(C, "condition");
            serializer_enter_statement(C, statement->statement, 2);
        }
        case 2: {
            serializer_set(C, "statement");
            serializer_complete(C);
        }
        default:
            break;
    }
}

decl_stmt(for) {
    switch (state) {
        case 0: {
            serializer_enter_statement(C, statement->initial, 1);
        }
        case 1: {
            serializer_set(C, "initial");
            serializer_enter_expression(C, statement->condition, 2);
        }
        case 2: {
            serializer_set(C, "condition");
            serializer_enter_statement(C, statement->increment, 3);
        }
        case 3: {
            serializer_set(C, "increment");
            serializer_enter_statement(C, statement->statement, 4);
        }
        case 4: {
            serializer_set(C, "statement");
            serializer_complete(C);
        }
        default:
            break;
    }
}

decl_stmt(iterator) {
    switch (state) {
        case 0: {
            serializer_enter_statement(C, mcapi_ast_declaration2statement(statement->declaration), 1);
        }
        case 1: {
            serializer_set(C, "declaration");
            serializer_enter_expression(C, statement->key, 2);
        }
        case 2: {
            serializer_set(C, "key");
            serializer_enter_expression(C, statement->value, 3);
        }
        case 3: {
            serializer_set(C, "value");
            serializer_enter_statement(C, statement->statement, 4);
        }
        case 4: {
            serializer_set(C, "statement");
            serializer_complete(C);
        }
        default:
            break;
    }
}

struct declaration_data {
    size_t index;
};

decl_stmt(declaration) {
    decl_data(declaration);

    switch (state) {
        case 0: {
            serializer_push_boolean(C, statement->mutable);
            serializer_set(C, "mutable");

            serializer_push_boolean(C, statement->is_extract);
            serializer_set(C, "isextract");

            serializer_enter_expression(C, statement->expression, 1);
        }
        case 1: {
            serializer_set(C, "expression");

            if (statement->is_extract) {
                serializer_push_table(C);

                serializer_push_size(C, statement->extract.size, NULL);
                serializer_set(C, "size");

                data->index = 0;
                serializer_push_vector(C, statement->extract.size);
                serializer_jump(C, 3);
            } else {
                serializer_enter_expression(C, mcapi_ast_variable2expression(statement->value), 2);
            }
        }
        case 2: {
            serializer_set(C, "value");
            serializer_complete(C);
        }
        case 3: {
            if (data->index >= statement->extract.size) {
                serializer_set(C, "keys");

                data->index = 0;
                serializer_push_vector(C, statement->extract.size);
                serializer_jump(C, 5);
            } else {
                serializer_enter_expression(C, statement->extract.keys[data->index], 4);
            }
        }
        case 4: {
            serializer_vector_set(C, data->index);
            data->index++;
            serializer_jump(C, 3);
        }
        case 5: {
            if (data->index >= statement->extract.size) {
                serializer_set(C, "values");
                serializer_set(C, "extract");
                serializer_complete(C);
            } else {
                serializer_enter_expression(
                    C,
                    mcapi_ast_variable2expression(statement->extract.values[data->index]),
                    6
                );
            }
        }
        case 6: {
            serializer_vector_set(C, data->index);
            data->index++;
            serializer_jump(C, 5);
        }
        default:
            break;
    }
}

struct assigment_data {
    size_t index;
};

decl_stmt(assigment) {
    decl_data(assigment);

    switch (state) {
        case 0: {
            serializer_push_boolean(C, statement->is_extract);
            serializer_set(C, "isextract");

            serializer_enter_expression(C, statement->expression, 1);
        }
        case 1: {
            serializer_set(C, "expression");

            if (statement->is_extract) {
                serializer_push_table(C);

                serializer_push_size(C, statement->extract.size, NULL);
                serializer_set(C, "size");

                data->index = 0;
                serializer_push_vector(C, statement->extract.size);
                serializer_jump(C, 3);
            } else {
                serializer_enter_expression(C, statement->value, 2);
            }
        }
        case 2: {
            serializer_set(C, "value");
            serializer_complete(C);
        }
        case 3: {
            if (data->index >= statement->extract.size) {
                serializer_set(C, "keys");

                data->index = 0;
                serializer_push_vector(C, statement->extract.size);
                serializer_jump(C, 5);
            } else {
                serializer_enter_expression(C, statement->extract.keys[data->index], 4);
            }
        }
        case 4: {
            serializer_vector_set(C, data->index);
            data->index++;
            serializer_jump(C, 3);
        }
        case 5: {
            if (data->index >= statement->extract.size) {
                serializer_set(C, "values");
                serializer_set(C, "extract");
                serializer_complete(C);
            } else {
                serializer_enter_expression(C, statement->extract.values[data->index], 6);
            }
        }
        case 6: {
            serializer_vector_set(C, data->index);
            data->index++;
            serializer_jump(C, 5);
        }
        default:
            break;
    }
}

static const char *opcode2str(morphine_opcode_t opcode) {
    switch (opcode) {
#define mspec_instruction_args0(n, s)             case MORPHINE_OPCODE_##n: return #s;
#define mspec_instruction_args1(n, s, a1)         case MORPHINE_OPCODE_##n: return #s;
#define mspec_instruction_args2(n, s, a1, a2)     case MORPHINE_OPCODE_##n: return #s;
#define mspec_instruction_args3(n, s, a1, a2, a3) case MORPHINE_OPCODE_##n: return #s;

#include "morphine/misc/instruction/specification.h"

#undef mspec_instruction_args0
#undef mspec_instruction_args1
#undef mspec_instruction_args2
#undef mspec_instruction_args3
    }

    return "?";
}

static size_t opcode2args(morphine_opcode_t opcode) {
    switch (opcode) {
#define mspec_instruction_args0(n, s)             case MORPHINE_OPCODE_##n: return 0;
#define mspec_instruction_args1(n, s, a1)         case MORPHINE_OPCODE_##n: return 1;
#define mspec_instruction_args2(n, s, a1, a2)     case MORPHINE_OPCODE_##n: return 2;
#define mspec_instruction_args3(n, s, a1, a2, a3) case MORPHINE_OPCODE_##n: return 3;

#include "morphine/misc/instruction/specification.h"

#undef mspec_instruction_args0
#undef mspec_instruction_args1
#undef mspec_instruction_args2
#undef mspec_instruction_args3
    }

    return 0;
}

decl_expr(asm) {
    switch (state) {
        case 0: {
            if (expression->has_emitter) {
                serializer_push_string(C, expression->emitter);
                serializer_set(C, "emitter");
            }

            serializer_jump(C, 1);
        }
        case 1: {
            serializer_push_vector(C, expression->data_count);
            for (size_t i = 0; i < expression->data_count; i++) {
                struct mc_asm_data data = expression->data[i];

                serializer_push_table(C);

                serializer_push_string(C, data.name);
                serializer_set(C, "name");

                switch (data.type) {
                    case MCADT_NIL:
                        serializer_push_nil(C);
                        break;
                    case MCADT_INTEGER:
                        serializer_push_integer(C, data.integer);
                        break;
                    case MCADT_DECIMAL:
                        serializer_push_decimal(C, data.decimal);
                        break;
                    case MCADT_BOOLEAN:
                        serializer_push_boolean(C, data.boolean);
                        break;
                    case MCADT_STRING:
                        serializer_push_string(C, data.string);
                        break;
                }
                serializer_set(C, "value");

                serializer_vector_set(C, i);
            }
            serializer_set(C, "data");

            serializer_jump(C, 2);
        }
        case 2: {
            serializer_push_vector(C, expression->slots_count);
            for (size_t i = 0; i < expression->slots_count; i++) {
                serializer_push_string(C, expression->slots[i]);
                serializer_vector_set(C, i);
            }
            serializer_set(C, "slots");

            serializer_jump(C, 3);
        }
        case 3: {
            serializer_push_vector(C, expression->anchors_count);
            for (size_t i = 0; i < expression->anchors_count; i++) {
                struct mc_asm_anchor anchor = expression->anchors[i];
                serializer_push_table(C);

                serializer_push_string(C, anchor.anchor);
                serializer_set(C, "anchor");

                serializer_push_size(C, anchor.instruction, "index");
                serializer_set(C, "index");

                serializer_vector_set(C, i);
            }
            serializer_set(C, "anchors");

            serializer_jump(C, 4);
        }
        case 4: {
            serializer_push_vector(C, expression->code_count);
            for (size_t i = 0; i < expression->code_count; i++) {
                struct mc_asm_instruction instruction = expression->code[i];
                serializer_push_table(C);

                serializer_push_cstr(C, opcode2str(instruction.opcode));
                serializer_set(C, "opcode");

                serializer_push_size(C, instruction.line, "line");
                serializer_set(C, "line");

                size_t size = opcode2args(instruction.opcode);
                serializer_push_vector(C, size);
                for (size_t index = 0; index < size; index++) {
                    struct mc_asm_argument argument = instruction.arguments[index];
                    switch (argument.type) {
                        case MCAAT_WORD:
                            serializer_push_string(C, argument.word);
                            break;
                        case MCAAT_NUMBER:
                            serializer_push_integer(C, argument.number);
                            break;
                    }
                    serializer_vector_set(C, index);
                }
                serializer_set(C, "arguments");

                serializer_vector_set(C, i);
            }
            serializer_set(C, "code");

            serializer_complete(C);
        }
        default:
            break;
    }
}
