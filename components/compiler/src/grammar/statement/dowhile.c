//
// Created by why-iskra on 08.08.2024.
//

#include "../controller.h"

struct mc_ast_node *rule_dowhile(struct parse_controller *C) {
    ml_size token_from = parser_index(C);
    {
        parser_consume(C, et_predef_word(do));
        parser_reduce(C, rule_statement_block);
        if (parser_match(C, et_predef_word(while))) {
            parser_consume(C, et_operator(LPAREN));
            parser_reduce(C, rule_expression);
            parser_consume(C, et_operator(RPAREN));
        }
    }
    ml_size token_to = parser_index(C);

    parser_reset(C);

    ml_line line = parser_get_line(C);

    parser_consume(C, et_predef_word(do));

    struct mc_ast_statement *statement =
        mcapi_ast_node2statement(parser_U(C), parser_reduce(C, rule_statement_block));

    if (parser_match(C, et_predef_word(while))) {
        if (statement->type == MCSTMTT_eval) {
            struct mc_ast_statement_eval *eval = mcapi_ast_statement2eval(parser_U(C), statement);
            if (eval->expression->type == MCEXPRT_block) {
                struct mc_ast_expression_block *block =
                    mcapi_ast_expression2block(parser_U(C), eval->expression);

                block->inlined = true;
            }
        }

        parser_consume(C, et_operator(LPAREN));
        struct mc_ast_expression *expression =
            mcapi_ast_node2expression(parser_U(C), parser_reduce(C, rule_expression));

        parser_consume(C, et_operator(RPAREN));

        struct mc_ast_statement_while *statement_while =
            mcapi_ast_create_statement_while(parser_U(C), parser_A(C), token_from, token_to, line);

        statement_while->first_condition = false;
        statement_while->condition = expression;
        statement_while->statement = statement;

        return mcapi_ast_statement_while2node(statement_while);
    }

    return mcapi_ast_statement2node(statement);
}
