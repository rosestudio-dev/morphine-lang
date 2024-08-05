//
// Created by why-iskra on 05.08.2024.
//

#include "controller.h"
#include "extra/arguments.h"

struct mc_ast_node *rule_table(struct parse_controller *C) {
    size_t count;
    {
        struct arguments A = extra_arguments_init_full(
            C, true, et_operator(LBRACE), et_operator(RBRACE), et_operator(COMMA)
        );

        while (extra_arguments_next(C, &A)) {
            if (!parser_match(C, et_word())) {
                parser_reduce(C, rule_expression);
            }

            if (parser_match(C, et_operator(EQ))) {
                parser_reduce(C, rule_expression);
            }
        }

        count = extra_arguments_finish(C, &A);
    }

    parser_reset(C);

    ml_line line = parser_get_line(C);

    struct mc_ast_expression_table *table =
        mcapi_ast_create_expression_table(parser_U(C), parser_A(C), line, count);

    struct arguments A = extra_arguments_init_full(
        C, true, et_operator(LBRACE), et_operator(RBRACE), et_operator(COMMA)
    );

    for (size_t i = 0; extra_arguments_next(C, &A); i++) {
        ml_line intermediate_line = parser_get_line(C);

        struct mc_ast_expression *key;
        if (parser_look(C, et_word())) {
            struct mc_lex_token token = parser_consume(C, et_word());

            struct mc_ast_expression_value *expr_value =
                mcapi_ast_create_expression_value(parser_U(C), parser_A(C), token.line);

            expr_value->type = MCEXPR_VALUE_TYPE_STR;
            expr_value->value.string = token.word;

            key = mcapi_ast_value2expression(expr_value);
        } else {
            key = mcapi_ast_node2expression(parser_U(C), parser_reduce(C, rule_expression));
        }

        struct mc_ast_expression *value;
        if (parser_match(C, et_operator(EQ))) {
            value = mcapi_ast_node2expression(parser_U(C), parser_reduce(C, rule_expression));
        } else {
            struct mc_ast_expression_value *expr_value =
                mcapi_ast_create_expression_value(parser_U(C), parser_A(C), intermediate_line);

            expr_value->type = MCEXPR_VALUE_TYPE_INT;
            expr_value->value.integer = mapi_csize2index(parser_U(C), i);

            value = key;
            key = mcapi_ast_value2expression(expr_value);
        }

        table->keys[i] = key;
        table->values[i] = value;
    }

    extra_arguments_finish(C, &A);

    return mcapi_ast_expression_table2node(table);
}
