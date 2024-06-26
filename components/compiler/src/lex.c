//
// Created by why-iskra on 19.05.2024.
//

#include <string.h>
#include <ctype.h>
#include "morphinec/lex.h"

#define MORPHINE_TYPE "lex"

#define isnewline(c) ((c) == '\n' || (c) == '\r')
#define eoschar '\0'
#define opchars "+-*/%()[]{}=<>!&|.,^~?:;"

#define lex_cl_error(U, line, str) mapi_errorf((U), "line %"MLIMIT_LINE_PR": "str, (line))
#define lex_error(U, L, str) lex_cl_error(U, (L)->line, str)

struct lex {
    struct strtable *T;
    const char *text;
    size_t len;
    size_t pos;
    ml_line line;
};

static struct {
    const char *name;
    enum token_predefined_word type;
} predefined_table[] = {
#define word(n) { .type = TPW_##n, .name = #n }
    word(true),
    word(false),
    word(env),
    word(self),
    word(nil),
    word(val),
    word(static),
    word(var),
    word(and),
    word(or),
    word(not),
    word(recursive),
    word(auto),
    word(fun),
    word(if),
    word(else),
    word(elif),
    word(while),
    word(do),
    word(for),
    word(break),
    word(continue),
    word(return),
    word(leave),
    word(eval),
    word(type),
    word(len),
    word(to),
    word(yield),
    word(ref),
    word(end),
    word(pass),
    word(iterator),
    word(decompose),
    word(as),
    word(in),
#undef word
};

static struct {
    const char *name;
    enum token_operator type;
} operator_table[] = {
#define operator(t, n) { .type = TOP_##t, .name = (n) }
    operator(PLUS, "+"),
    operator(MINUS, "-"),
    operator(STAR, "*"),
    operator(SLASH, "/"),
    operator(PERCENT, "%"),
    operator(EQ, "="),
    operator(EQEQ, "=="),
    operator(EXCLEQ, "!="),
    operator(LTEQ, "<="),
    operator(LT, "<"),
    operator(GT, ">"),
    operator(GTEQ, ">="),
    operator(COLON, ":"),
    operator(DOLLAR, "$"),
    operator(SEMICOLON, ";"),
    operator(LPAREN, "("),
    operator(RPAREN, ")"),
    operator(LBRACKET, "["),
    operator(RBRACKET, "]"),
    operator(LBRACE, "{"),
    operator(RBRACE, "}"),
    operator(COMMA, ","),
    operator(DOT, "."),
    operator(DOTDOT, ".."),
    operator(PLUSPLUS, "++"),
    operator(MINUSMINUS, "--"),
    operator(PLUSEQ, "+="),
    operator(MINUSEQ, "-="),
    operator(STAREQ, "*="),
    operator(SLASHEQ, "/="),
    operator(PERCENTEQ, "%="),
    operator(DOTDOTEQ, "..="),
    operator(LARROW, "<-"),
    operator(RARROW, "->")
#undef operator
};

struct lex *lex(morphine_coroutine_t U, struct strtable *T, const char *text, size_t len) {
    struct lex *L = mapi_push_userdata(U, MORPHINE_TYPE, sizeof(struct lex));

    *L = (struct lex) {
        .T = T,
        .text = text,
        .len = len,
        .pos = 0,
        .line = 1
    };

    return L;
}

struct lex *get_lex(morphine_coroutine_t U) {
    if (strcmp(mapi_userdata_type(U), MORPHINE_TYPE) == 0) {
        return mapi_userdata_pointer(U);
    } else {
        mapi_error(U, "expected "MORPHINE_TYPE);
    }
}

static char peek(struct lex *L, size_t offset) {
    if (offset > SIZE_MAX - L->pos) {
        return eoschar;
    }

    size_t pos = L->pos + offset;

    if (pos >= L->len) {
        return eoschar;
    } else {
        return L->text[pos];
    }
}

static char next(struct lex *L) {
    if (L->pos < L->len) {
        L->pos++;
    }

    return peek(L, 0);
}

static void skip_newline(struct lex *L) {
    char cur = peek(L, 0);
    char nex = next(L);

    if (cur != nex && isnewline(nex)) {
        next(L);
    }

    L->line++;
}

static void skip_comment(struct lex *L) {
    next(L);
    next(L);

    char current = peek(L, 0);
    while (!isnewline(current) && current != eoschar) {
        current = next(L);
    }
}

static void skip_multiline_comment(morphine_coroutine_t U, struct lex *L) {
    next(L);
    next(L);

    size_t saved_line = L->line;
    size_t level = 1;
    char current = peek(L, 0);
    while (level > 0) {
        if (current == '/' && peek(L, 1) == '*') {
            level++;
            next(L);
        } else if (current == '*' && peek(L, 1) == '/') {
            level--;
            next(L);
        } else if (isnewline(current)) {
            skip_newline(L);
            current = peek(L, 0);
            continue;
        } else if (current == eoschar) {
            lex_cl_error(U, saved_line, "multiline comment isn't closed");
        }

        current = next(L);
    }

    next(L);
}

static struct token lex_number(morphine_coroutine_t U, struct lex *L) {
    const char *start = L->text + L->pos;
    size_t count = 0;
    size_t count_after_dot = 0;
    bool dot = false;
    char current = peek(L, 0);
    while (true) {
        if (dot && current == '.') {
            lex_error(U, L, "fractional part was repeated");
        }

        if (current == '.') {
            dot = true;
        } else if (!isdigit(current)) {
            if (count_after_dot == 1) {
                lex_error(U, L, "fractional part is empty");
            } else {
                break;
            }
        }

        current = next(L);

        count++;
        if (dot) {
            count_after_dot++;
        }
    }

    char buffer[65];
    memset(buffer, 0, 65);
    if (count > 64) {
        lex_error(U, L, "invalid number");
    }

    strncpy(buffer, start, count);

    ml_integer int_res = 0;
    ml_decimal dec_res = 0;
    bool int_success = mapi_platform_str2int(buffer, &int_res);
    bool dec_success = mapi_platform_str2dec(buffer, &dec_res);

    if (!int_success && !dec_success) {
        lex_error(U, L, "invalid number");
    }

    if (dot) {
        return (struct token) {
            .type = TT_DECIMAL,
            .decimal = dec_res,
            .line = L->line
        };
    } else {
        return (struct token) {
            .type = TT_INTEGER,
            .integer = int_res,
            .line = L->line
        };
    }
}

static void safe_append(char *buffer, size_t *index, char c) {
    if (buffer != NULL) {
        buffer[*index] = c;
    }

    (*index)++;
}

static size_t handle_string(morphine_coroutine_t U, struct lex *L, char *buffer) {
    char open = peek(L, 0);

    size_t saved_line = L->line;
    size_t count = 0;
    char current = next(L);
    while (true) {
        if (current == eoschar || isnewline(current)) {
            lex_cl_error(U, saved_line, "string isn't closed");
        }

        if (current == open) {
            break;
        }

        if (current == '\\') {
            current = next(L);

            switch (current) {
                case '\\':
                    safe_append(buffer, &count, '\\');
                    break;
                case '"':
                    safe_append(buffer, &count, '"');
                    break;
                case '\'':
                    safe_append(buffer, &count, '\'');
                    break;
                case '0':
                    safe_append(buffer, &count, 0);
                    break;
                case 'b':
                    safe_append(buffer, &count, '\b');
                    break;
                case 'n':
                    safe_append(buffer, &count, '\n');
                    break;
                case 'r':
                    safe_append(buffer, &count, '\r');
                    break;
                case 't':
                    safe_append(buffer, &count, '\t');
                    break;
                default:
                    lex_error(U, L, "unknown escape symbol");
            }
        } else {
            safe_append(buffer, &count, current);
        }

        current = next(L);
    }

    next(L);

    return count;
}

static struct token lex_string(morphine_coroutine_t U, struct lex *L) {
    size_t save_pos = L->pos;

    size_t size = handle_string(U, L, NULL);
    char *str = mapi_push_userdata(U, "lextemp", size);
    memset(str, 0, size);

    L->pos = save_pos;
    handle_string(U, L, str);

    strtable_index_t index = strtable_record(U, L->T, str, size);
    mapi_pop(U, 1);

    return (struct token) {
        .type = TT_STRING,
        .string = index,
        .line = L->line
    };
}

static struct token handle_word(morphine_coroutine_t U, struct lex *L, size_t from, size_t to) {
    const char *str = L->text + from;
    size_t size = to - from;

    if (size == 0) {
        lex_error(U, L, "empty word");
    }

    for (size_t i = 0; i < sizeof(predefined_table) / sizeof(predefined_table[0]); i++) {
        if (strlen(predefined_table[i].name) != size) {
            continue;
        }

        if (memcmp(predefined_table[i].name, str, size) == 0) {
            return (struct token) {
                .type = TT_PREDEFINED_WORD,
                .predefined_word = predefined_table[i].type,
                .line = L->line
            };
        }
    }

    strtable_index_t index = strtable_record(U, L->T, str, size);

    return (struct token) {
        .type = TT_WORD,
        .word = index,
        .line = L->line
    };
}

static struct token lex_extended_word(morphine_coroutine_t U, struct lex *L) {
    char open = peek(L, 0);
    char current = next(L);

    size_t saved_line = L->line;
    size_t from = L->pos;
    while (true) {
        if (current == eoschar || isnewline(current)) {
            lex_cl_error(U, saved_line, "extended word isn't closed");
        }

        if (current == open) {
            break;
        }

        current = next(L);
    }

    size_t to = L->pos;
    next(L);

    return handle_word(U, L, from, to);
}

static struct token lex_word(morphine_coroutine_t U, struct lex *L) {
    char current = peek(L, 0);

    size_t from = L->pos;
    while (isalpha(current) || isdigit(current)) {
        current = next(L);
    }

    size_t to = L->pos;
    return handle_word(U, L, from, to);
}

static bool handle_operator(struct lex *L, size_t from, size_t to, struct token *token) {
    const char *str = L->text + from;
    size_t size = to - from;
    for (size_t i = 0; i < sizeof(operator_table) / sizeof(operator_table[0]); i++) {
        if (strlen(operator_table[i].name) != size) {
            continue;
        }

        if (memcmp(operator_table[i].name, str, size) == 0) {
            if (token != NULL) {
                *token = (struct token) {
                    .type = TT_OPERATOR,
                    .operator = operator_table[i].type,
                    .line = L->line
                };
            }

            return true;
        }
    }

    return false;
}

static struct token lex_operator(morphine_coroutine_t U, struct lex *L) {
    size_t from = L->pos;
    while (true) {
        char current = next(L);

        if (!handle_operator(L, from, L->pos + 1, NULL)) {
            break;
        }

        if (current == eoschar || strchr(opchars, current) == NULL) {
            break;
        }
    }

    struct token token;
    if (handle_operator(L, from, L->pos, &token)) {
        return token;
    } else {
        lex_error(U, L, "unknown operator");
    }
}

struct token lex_step(morphine_coroutine_t U, struct lex *L) {
    char current;
    while (true) {
        current = peek(L, 0);

        if (current == eoschar) {
            return (struct token) {
                .type = TT_EOS,
                .line = L->line
            };
        }

        if (isnewline(current)) {
            skip_newline(L);
            continue;
        }

        if (current == '/' && peek(L, 1) == '/') {
            skip_comment(L);
            continue;
        }

        if (current == '/' && peek(L, 1) == '*') {
            skip_multiline_comment(U, L);
            continue;
        }

        if (isspace(current)) {
            next(L);
            continue;
        }

        break;
    }

    if (isdigit(current)) {
        return lex_number(U, L);
    }

    if (current == '"' || current == '\'') {
        return lex_string(U, L);
    }

    if (current == '`') {
        return lex_extended_word(U, L);
    }

    if (isalpha(current)) {
        return lex_word(U, L);
    }

    if (strchr(opchars, current) != NULL) {
        return lex_operator(U, L);
    }

    lex_error(U, L, "unknown symbol");
}

const char *lex_operator2str(morphine_coroutine_t U, enum token_operator operator) {
    for (size_t i = 0; i < sizeof(operator_table) / sizeof(operator_table[0]); i++) {
        if (operator_table[i].type == operator) {
            return operator_table[i].name;
        }
    }

    mapi_error(U, "operator not found");
}

const char *lex_predefined2str(morphine_coroutine_t U, enum token_predefined_word predefined_word) {
    for (size_t i = 0; i < sizeof(predefined_table) / sizeof(predefined_table[0]); i++) {
        if (predefined_table[i].type == predefined_word) {
            return predefined_table[i].name;
        }
    }

    mapi_error(U, "predefined word not found");
}
