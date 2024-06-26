//
// Created by why-iskra on 04.06.2024.
//

#pragma once

#include "morphinec/ast.h"
#include "support/controller.h"

void gen_expression(struct codegen_controller *, struct expression *, size_t state);
void gen_statement(struct codegen_controller *, struct statement *, size_t state);
