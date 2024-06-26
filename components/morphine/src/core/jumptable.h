//
// Created by whyiskra on 27.12.23.
//

#undef sp_dispatch
#undef sp_case
#undef sp_continue

#define sp_dispatch(x) goto *sp_dispatch_tab[x];
#define sp_case(l) OL_##l:
#define sp_continue() morphinem_blk_start sp_fetch(); sp_dispatch(instruction.opcode) morphinem_blk_end

static const void *const sp_dispatch_tab[MORPHINE_OPCODES_COUNT] = {
    // generated
    &&OL_MORPHINE_OPCODE_YIELD,
    &&OL_MORPHINE_OPCODE_LOAD,
    &&OL_MORPHINE_OPCODE_MOVE,
    &&OL_MORPHINE_OPCODE_PARAM,
    &&OL_MORPHINE_OPCODE_ARG,
    &&OL_MORPHINE_OPCODE_CLEAR,
    &&OL_MORPHINE_OPCODE_ENV,
    &&OL_MORPHINE_OPCODE_SELF,
    &&OL_MORPHINE_OPCODE_RECURSION,
    &&OL_MORPHINE_OPCODE_VECTOR,
    &&OL_MORPHINE_OPCODE_TABLE,
    &&OL_MORPHINE_OPCODE_GET,
    &&OL_MORPHINE_OPCODE_SET,
    &&OL_MORPHINE_OPCODE_ITERATOR,
    &&OL_MORPHINE_OPCODE_ITERATOR_INIT,
    &&OL_MORPHINE_OPCODE_ITERATOR_HAS,
    &&OL_MORPHINE_OPCODE_ITERATOR_NEXT,
    &&OL_MORPHINE_OPCODE_JUMP,
    &&OL_MORPHINE_OPCODE_JUMP_IF,
    &&OL_MORPHINE_OPCODE_GET_STATIC,
    &&OL_MORPHINE_OPCODE_SET_STATIC,
    &&OL_MORPHINE_OPCODE_GET_CLOSURE,
    &&OL_MORPHINE_OPCODE_SET_CLOSURE,
    &&OL_MORPHINE_OPCODE_CLOSURE,
    &&OL_MORPHINE_OPCODE_CALL,
    &&OL_MORPHINE_OPCODE_SCALL,
    &&OL_MORPHINE_OPCODE_LEAVE,
    &&OL_MORPHINE_OPCODE_RESULT,
    &&OL_MORPHINE_OPCODE_ADD,
    &&OL_MORPHINE_OPCODE_SUB,
    &&OL_MORPHINE_OPCODE_MUL,
    &&OL_MORPHINE_OPCODE_DIV,
    &&OL_MORPHINE_OPCODE_MOD,
    &&OL_MORPHINE_OPCODE_EQUAL,
    &&OL_MORPHINE_OPCODE_LESS,
    &&OL_MORPHINE_OPCODE_LESS_EQUAL,
    &&OL_MORPHINE_OPCODE_AND,
    &&OL_MORPHINE_OPCODE_OR,
    &&OL_MORPHINE_OPCODE_CONCAT,
    &&OL_MORPHINE_OPCODE_TYPE,
    &&OL_MORPHINE_OPCODE_NEGATIVE,
    &&OL_MORPHINE_OPCODE_NOT,
    &&OL_MORPHINE_OPCODE_REF,
    &&OL_MORPHINE_OPCODE_DEREF,
    &&OL_MORPHINE_OPCODE_LENGTH
};
