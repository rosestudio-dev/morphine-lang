//
// Created by why-iskra on 05.09.2024.
//

#pragma once

#include "controller.h"

#define expr(n) void serialize_expression_##n(struct serializer_controller *, struct mc_ast_expression_##n *, size_t)
#define stmt(n) void serialize_statement_##n(struct serializer_controller *, struct mc_ast_statement_##n *, size_t)

expr(value);
expr(binary);
expr(unary);
expr(increment);
expr(variable);
expr(global);
expr(leave);
expr(table);
expr(vector);
expr(access);
expr(call);
expr(function);
expr(block);
expr(if);

stmt(block);
stmt(simple);
stmt(eval);
stmt(while);
stmt(for);
stmt(iterator);
stmt(declaration);
stmt(assigment);
stmt(if);

#undef expr
#undef stmt