//
// Created by why-iskra on 18.08.2024.
//

/*
 * Arguments:
 * sslot
 * dslot
 * size
 * position
 * constant_index
 * param_index
 * argument_index
 * static_index
 * closure_index
 * params_count
 */

mspec_instruction_args0(YIELD,         yield)                                //                          yield
mspec_instruction_args2(LOAD,          load,    constant_index, dslot)       // [src, dest]              get from constant by (src) and set to (dest)
mspec_instruction_args2(MOVE,          move,    sslot, dslot)                // [src, dest]              get from (src) and set to (dest)
mspec_instruction_args2(PARAM,         param,   sslot, param_index)          // [src, dest]              get from (src) and set to params by (dest)
mspec_instruction_args2(ARG,           arg,     argument_index, dslot)       // [arg, dest]              get from args by (arg) and set to (dest)

mspec_instruction_args1(ENV,           env,     dslot)                       // [dest]                   move env to (dest)
mspec_instruction_args1(SELF,          self,    dslot)                       // [dest]                   move self to (dest)
mspec_instruction_args1(INVOKED,       invoked, dslot)                       // [dest]                   move invoked callable to (dest)

mspec_instruction_args2(VECTOR,        vector,  dslot, size)                 // [dest, size]             create vector in (dest) with (size)
mspec_instruction_args1(TABLE,         table,   dslot)                       // [dest]                   create table in (dest)
mspec_instruction_args3(GET,           get,     sslot, sslot, dslot)         // [container, key, dest]   get from (container) by (key) to (dest)
mspec_instruction_args3(SET,           set,     sslot, sslot, sslot)         // [container, key, src]    set (src) to (container) by (key)

mspec_instruction_args2(ITERATOR,      iter,    sslot, dslot)                // [container, dest]        create iterator from (container) to (dest)
mspec_instruction_args3(ITERATOR_INIT, itinit,  sslot, sslot, sslot)         // [iterator, key, value]   init (iterator) with name (key) for key and (value) for value
mspec_instruction_args2(ITERATOR_HAS,  ithas,   sslot, dslot)                // [iterator, dest]         check next value of (iterator) to (dest)
mspec_instruction_args2(ITERATOR_NEXT, itnext,  sslot, dslot)                // [iterator, dest]         get next value of (iterator) to (dest)

mspec_instruction_args1(JUMP,          jmp,     position)                    // [position]               jump to (position)
mspec_instruction_args3(JUMP_IF,       jmpif,   sslot, position, position)   // [condition, if, else]    if (condition) is true jump to (if) else jump to (else)

mspec_instruction_args3(GET_STATIC,    getstc,  sslot, static_index, dslot)  // [callable, static, dest] get static by (index) from (callable) to (dest)
mspec_instruction_args3(SET_STATIC,    setstc,  sslot, static_index, sslot)  // [callable, static, src]  set (src) to static of (callable) by (index)
mspec_instruction_args3(GET_CLOSURE,   getcls,  sslot, closure_index, dslot) // [closure, closure, dest] get closure by (index) from (closure) to (dest)
mspec_instruction_args3(SET_CLOSURE,   setcls,  sslot, closure_index, sslot) // [closure, closure, src]  set (src) to (closure) by (index)

mspec_instruction_args3(CLOSURE,       closure, sslot, size, dslot)          // [function, size, dest]   create closure for (function) with (size) in (dest)
mspec_instruction_args2(CALL,          call,    sslot, params_count)         // [function, params]       call (function) with count (params)
mspec_instruction_args3(SCALL,         scall,   sslot, params_count, sslot)  // [function, params, self] call (function) with count (params) change (self)
mspec_instruction_args0(LEAVE,         leave)                                //                          leave
mspec_instruction_args1(RETURN,        ret,     sslot)                       // [return]                 leave with (return)
mspec_instruction_args1(RESULT,        result,  dslot)                       // [dest]                   set result value to (dest)

mspec_instruction_args3(ADD,           add,     sslot, sslot, dslot)         // [lvalue, rvalue, dest]   (lvalue) operator (rvalue) to (dest)
mspec_instruction_args3(SUB,           sub,     sslot, sslot, dslot)
mspec_instruction_args3(MUL,           mul,     sslot, sslot, dslot)
mspec_instruction_args3(DIV,           div,     sslot, sslot, dslot)
mspec_instruction_args3(MOD,           mod,     sslot, sslot, dslot)
mspec_instruction_args3(EQUAL,         eq,      sslot, sslot, dslot)
mspec_instruction_args3(LESS,          less,    sslot, sslot, dslot)
mspec_instruction_args3(AND,           and,     sslot, sslot, dslot)
mspec_instruction_args3(OR,            or,      sslot, sslot, dslot)
mspec_instruction_args3(CONCAT,        concat,  sslot, sslot, dslot)

mspec_instruction_args2(TYPE,          type,    sslot, dslot)                // [value, dest]            get string type of (value) to (dest)
mspec_instruction_args2(NEGATIVE,      neg,     sslot, dslot)                // [value, dest]            operator (lvalue) to (dest)
mspec_instruction_args2(NOT,           not,     sslot, dslot)
mspec_instruction_args2(REF,           ref,     sslot, dslot)
mspec_instruction_args2(DEREF,         deref,   sslot, dslot)
mspec_instruction_args2(LENGTH,        len,     sslot, dslot)
