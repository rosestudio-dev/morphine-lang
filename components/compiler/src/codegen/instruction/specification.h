//
// Created by why-iskra on 18.08.2024.
//

instr_func0(YIELD)
instr_func2(LOAD, index, slot)
instr_func2(MOVE, slot, slot)
instr_func2(PARAM, slot, index)
instr_func2(ARG, index, slot)

instr_func1(ENV, slot)
instr_func1(SELF, slot)
instr_func1(INVOKED, slot)

instr_func2(VECTOR, slot, count)
instr_func1(TABLE, slot)
instr_func3(GET, slot, slot, slot)
instr_func3(SET, slot, slot, slot)

instr_func2(ITERATOR, slot, slot)
instr_func3(ITERATOR_INIT, slot, slot, slot)
instr_func2(ITERATOR_HAS, slot, slot)
instr_func2(ITERATOR_NEXT, slot, slot)

instr_func1(JUMP, anchor)
instr_func3(JUMP_IF, slot, anchor, anchor)

instr_func3(GET_STATIC, slot, index, slot)
instr_func3(SET_STATIC, slot, index, slot)
instr_func3(GET_CLOSURE, slot, index, slot)
instr_func3(SET_CLOSURE, slot, index, slot)

instr_func3(CLOSURE, slot, count, slot)
instr_func2(CALL, slot, count)
instr_func3(SCALL, slot, count, slot)
instr_func1(LEAVE, slot)
instr_func1(RESULT, slot)

instr_func3(ADD, slot, slot, slot)
instr_func3(SUB, slot, slot, slot)
instr_func3(MUL, slot, slot, slot)
instr_func3(DIV, slot, slot, slot)
instr_func3(MOD, slot, slot, slot)
instr_func3(EQUAL, slot, slot, slot)
instr_func3(LESS, slot, slot, slot)
instr_func3(AND, slot, slot, slot)
instr_func3(OR, slot, slot, slot)
instr_func3(CONCAT, slot, slot, slot)

instr_func2(TYPE, slot, slot)
instr_func2(NEGATIVE, slot, slot)
instr_func2(NOT, slot, slot)
instr_func2(REF, slot, slot)
instr_func2(DEREF, slot, slot)
instr_func2(LENGTH, slot, slot)
