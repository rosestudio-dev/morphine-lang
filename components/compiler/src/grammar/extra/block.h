//
// Created by why-iskra on 06.08.2024.
//

#pragma once

#include "../controller.h"

#define array_closes_size(a) (sizeof((a)) / sizeof(struct mc_lex_token))

size_t extra_consume_statement_block(
    struct parse_controller *,
    size_t closes_size,
    struct mc_lex_token *closes,
    size_t *close_index
);

size_t extra_consume_expression_block(
    struct parse_controller *,
    size_t closes_size,
    struct mc_lex_token *closes,
    size_t *close_index
);

void extra_extract_statement_block(
    struct parse_controller *,
    size_t closes_size,
    struct mc_lex_token *closes,
    size_t size,
    struct mc_ast_statement **statements
);

void extra_extract_expression_block(
    struct parse_controller *,
    size_t closes_size,
    struct mc_lex_token *closes,
    size_t size,
    struct mc_ast_statement **statements,
    struct mc_ast_expression **expression
);
