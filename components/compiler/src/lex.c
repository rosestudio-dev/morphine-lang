//
// Created by why-iskra on 19.05.2024.
//

#include <string.h>
#include <ctype.h>
#include "morphinec/lex.h"

#define OPCHARS_AMORTIZATION 4

#define isnewline(c) ((c) == '\n' || (c) == '\r')
#define eoschar '\0'

#define lex_cl_error(U, line, str) mapi_errorf((U), "line %"MLIMIT_LINE_PR": "str, (line))
#define lex_error(U, L, str) lex_cl_error(U, (L)->line, str)

struct mc_lex {
    char *text;
    size_t len;
    size_t pos;
    ml_line line;

    struct {
        size_t allocated;
        size_t size;
        char *string;
    } opchars;
};

static struct {
    const char *str;
    const char *name;
    enum mc_lex_token_operator type;
} operator_table[] = {
#define operator(t, n) { .type = MCLTOP_##t, .str = (n), .name = #t }
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
    operator(AT, "@")
#undef operator
};

static void lex_userdata_init(morphine_instance_t I, void *data) {
    (void) I;

    struct mc_lex *L = data;
    (*L) = (struct mc_lex) {
        .text = NULL,
        .len = 0,
        .pos = 0,
        .line = 1,
        .opchars.allocated = 0,
        .opchars.size = 0,
        .opchars.string = NULL,
    };
}

static void lex_userdata_free(morphine_instance_t I, void *data) {
    struct mc_lex *L = data;
    mapi_allocator_free(I, L->text);
    mapi_allocator_free(I, L->opchars.string);
}

static inline void opchars_append(morphine_coroutine_t U, struct mc_lex *L, char c) {
    if (L->opchars.size > L->opchars.allocated) {
        mapi_error(U, "broken lexer operator chars");
    }

    if (L->opchars.size == L->opchars.allocated) {
        L->opchars.string = mapi_allocator_vec(
            mapi_instance(U),
            L->opchars.string,
            L->opchars.allocated + OPCHARS_AMORTIZATION,
            sizeof(char)
        );

        L->opchars.allocated += OPCHARS_AMORTIZATION;
    }

    L->opchars.string[L->opchars.size] = c;
    L->opchars.size = 0;
}

static void create_opchars(morphine_coroutine_t U, struct mc_lex *L) {
    opchars_append(U, L, '\0');

    for (size_t i = 0; i < sizeof(operator_table) / sizeof(operator_table[0]); i++) {
        const char *op = operator_table[i].str;
        size_t size = strlen(op);
        for (size_t n = 0; n < size; n++) {
            if (strchr(L->opchars.string, op[n]) == NULL) {
                L->opchars.string[L->opchars.size - 1] = op[n];
                opchars_append(U, L, '\0');
            }
        }
    }
}

MORPHINE_API struct mc_lex *mcapi_push_lex(morphine_coroutine_t U, const char *text, size_t len) {
    mapi_type_declare(
        mapi_instance(U),
        MC_LEX_USERDATA_TYPE,
        sizeof(struct mc_lex),
        lex_userdata_init,
        lex_userdata_free,
        false
    );

    struct mc_lex *L = mapi_push_userdata(U, MC_LEX_USERDATA_TYPE);
    create_opchars(U, L);

    L->text = mapi_allocator_vec(mapi_instance(U), NULL, len, sizeof(char));
    L->len = len;

    memcpy(L->text, text, len * sizeof(char));

    return L;
}

MORPHINE_API struct mc_lex *mcapi_get_lex(morphine_coroutine_t U) {
    return mapi_userdata_pointer(U, MC_LEX_USERDATA_TYPE);
}

static char peek(struct mc_lex *L, size_t offset) {
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

static char next(struct mc_lex *L) {
    if (L->pos < L->len) {
        L->pos++;
    }

    return peek(L, 0);
}

static void skip_newline(struct mc_lex *L) {
    char cur = peek(L, 0);
    char nex = next(L);

    if (cur != nex && isnewline(nex)) {
        next(L);
    }

    L->line++;
}

static struct mc_lex_token comment(
    morphine_coroutine_t U,
    struct mc_lex *L,
    struct mc_strtable *T
) {
    size_t start = L->pos;
    next(L);
    next(L);

    char current = peek(L, 0);
    while (!isnewline(current) && current != eoschar) {
        current = next(L);
    }

    mc_strtable_index_t index = mcapi_strtable_record(
        U, T, L->text + start + 2, L->pos - start - 2
    );

    return (struct mc_lex_token) {
        .type = MCLTT_COMMENT,
        .comment = index,
        .line = L->line,
        .range.from = start,
        .range.to = L->pos
    };
}

static struct mc_lex_token multiline_comment(
    morphine_coroutine_t
    U,
    struct mc_lex *L,
    struct mc_strtable *T
) {
    size_t start = L->pos;
    ml_line saved_line = L->line;
    next(L);
    next(L);

    size_t level = 1;
    char current = peek(L, 0);
    while (level > 0) {
        if (current == '/' &&
            peek(
                L,
                1
            ) == '*') {
            level++;
            next(L);
        } else if (current == '*' &&
                   peek(
                       L,
                       1
                   ) == '/') {
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

    mc_strtable_index_t index = mcapi_strtable_record(
        U, T, L->text + start + 2, L->pos - start - 4
    );

    return (struct mc_lex_token) {
        .type = MCLTT_MULTILINE_COMMENT,
        .comment = index,
        .line = L->line,
        .range.from = start,
        .range.to = L->pos
    };
}

static struct mc_lex_token lex_number(morphine_coroutine_t U, struct mc_lex *L) {
    size_t start = L->pos;
    bool dot = false;

    size_t count_after_dot = 0;
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

        if (dot) {
            count_after_dot++;
        }
    }

    size_t size = L->pos - start;

    char buffer[65];
    memset(buffer, 0, 65);
    if (size > 64) {
        lex_error(U, L, "invalid number");
    }

    strncpy(buffer, L->text + start, size);

    if (dot) {
        ml_decimal result = 0;
        bool success = mapi_platform_str2dec(buffer, &result);
        if (!success) {
            lex_error(U, L, "invalid decimal");
        }

        return (struct mc_lex_token) {
            .type = MCLTT_DECIMAL,
            .decimal = result,
            .line = L->line,
            .range.from = start,
            .range.to = L->pos
        };
    } else {
        ml_integer result = 0;
        bool success = mapi_platform_str2int(buffer, &result, 10);
        if (!success) {
            lex_error(U, L, "invalid integer");
        }

        return (struct mc_lex_token) {
            .type = MCLTT_INTEGER,
            .integer = result,
            .line = L->line,
            .range.from = start,
            .range.to = L->pos
        };
    }
}

static void safe_append(char *buffer, size_t *index, char c) {
    if (buffer != NULL) {
        buffer[*index] = c;
    }

    (*index)++;
}

static size_t handle_string(morphine_coroutine_t U, struct mc_lex *L, char *buffer) {
    char open = peek(L, 0);

    ml_line saved_line = L->line;
    size_t count = 0;
    char current = next(L);
    while (true) {
        if (current == eoschar) {
            lex_cl_error(U, saved_line, "string isn't closed");
        }

        if (current == open) {
            break;
        }

        if (isnewline(current)) {
            char nex = next(L);
            safe_append(buffer, &count, current);

            if (current != nex && isnewline(nex)) {
                safe_append(buffer, &count, nex);

                current = next(L);
            } else {
                current = nex;
            }

            if (buffer != NULL) {
                L->line++;
            }

            continue;
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

static struct mc_lex_token lex_string(
    morphine_coroutine_t U,
    struct mc_lex *L,
    struct mc_strtable *T
) {
    ml_line saved_line = L->line;
    size_t saved_pos = L->pos;

    size_t size = handle_string(U, L, NULL);
    char *str = mapi_push_userdata_uni(U, size);
    memset(str, 0, size);

    L->pos = saved_pos;
    handle_string(U, L, str);

    mc_strtable_index_t index = mcapi_strtable_record(U, T, str, size);
    mapi_pop(U, 1);

    return (struct mc_lex_token) {
        .type = MCLTT_STRING,
        .string = index,
        .line = saved_line,
        .range.from = saved_pos,
        .range.to = L->pos
    };
}

static struct mc_lex_token lex_extended_word(
    morphine_coroutine_t U,
    struct mc_lex *L,
    struct mc_strtable *T
) {
    ml_line saved_line = L->line;
    char open = peek(L, 0);

    char current = next(L);
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

    const char *str = L->text + from;
    size_t size = to - from;

    if (size == 0) {
        lex_cl_error(U, saved_line, "empty extended word");
    }

    mc_strtable_index_t index = mcapi_strtable_record(U, T, str, size);

    return (struct mc_lex_token) {
        .type = MCLTT_EXTENDED_WORD,
        .word = index,
        .line = saved_line,
        .range.from = from,
        .range.to = to
    };
}

static struct mc_lex_token lex_word(
    morphine_coroutine_t U,
    struct mc_lex *L,
    struct mc_strtable *T
) {
    ml_line saved_line = L->line;
    char current = peek(L, 0);

    size_t from = L->pos;
    while (current == '_' || isalpha(current) || isdigit(current)) {
        current = next(L);
    }

    size_t to = L->pos;

    const char *str = L->text + from;
    size_t size = to - from;

    if (size == 0) {
        lex_cl_error(U, saved_line, "empty word");
    }

    mc_strtable_index_t index = mcapi_strtable_record(U, T, str, size);

    return (struct mc_lex_token) {
        .type = MCLTT_WORD,
        .word = index,
        .line = saved_line,
        .range.from = from,
        .range.to = to
    };
}

static bool handle_operator(
    morphine_coroutine_t U,
    struct mc_lex *L,
    size_t from,
    size_t to,
    ml_line line,
    struct mc_lex_token *token
) {
    const char *str = L->text + from;
    size_t size = to - from;

    if (size == 0) {
        lex_error(U, L, "empty operator");
    }

    for (size_t i = 0; i < sizeof(operator_table) / sizeof(operator_table[0]); i++) {
        if (strlen(operator_table[i].str) != size) {
            continue;
        }

        if (memcmp(operator_table[i].str, str, size) == 0) {
            if (token != NULL) {
                *token = (struct mc_lex_token) {
                    .type = MCLTT_OPERATOR,
                    .op = operator_table[i].type,
                    .line = line,
                    .range.from = from,
                    .range.to = to
                };
            }

            return true;
        }
    }

    return false;
}

static struct mc_lex_token lex_operator(morphine_coroutine_t U, struct mc_lex *L) {
    size_t from = L->pos;

    while (true) {
        char current = next(L);

        if (!handle_operator(U, L, from, L->pos + 1, L->line, NULL)) {
            break;
        }

        if (current == eoschar || strchr(L->opchars.string, current) == NULL) {
            break;
        }
    }

    struct mc_lex_token token;
    if (handle_operator(U, L, from, L->pos, L->line, &token)) {
        return token;
    } else {
        lex_error(U, L, "unknown operator");
    }
}

MORPHINE_API struct mc_lex_token mcapi_lex_step(
    morphine_coroutine_t U,
    struct mc_lex *L,
    struct mc_strtable *T
) {
    char current = peek(L, 0);

    while (true) {
        if (isnewline(current)) {
            skip_newline(L);
        } else if (isspace(current)) {
            next(L);
        } else {
            break;
        }

        current = peek(L, 0);
    }

    if (current == eoschar) {
        return (struct mc_lex_token) {
            .type = MCLTT_EOS,
            .line = L->line
        };
    }

    if (current == '/' && peek(L, 1) == '/') {
        return comment(U, L, T);
    }

    if (current == '/' && peek(L, 1) == '*') {
        return multiline_comment(U, L, T);
    }

    if (isdigit(current)) {
        return lex_number(U, L);
    }

    if (current == '"' || current == '\'') {
        return lex_string(U, L, T);
    }

    if (current == '`') {
        return lex_extended_word(U, L, T);
    }

    if (current == '_' || isalpha(current)) {
        return lex_word(U, L, T);
    }

    if (strchr(L->opchars.string, current) != NULL) {
        return lex_operator(U, L);
    }

    lex_error(U, L, "unknown symbol");
}

MORPHINE_API const char *mcapi_lex_type2str(
    morphine_coroutine_t U,
    enum mc_lex_token_type type
) {
    switch (type) {
        case MCLTT_EOS:
            return "eos";
        case MCLTT_INTEGER:
            return "integer";
        case MCLTT_DECIMAL:
            return "decimal";
        case MCLTT_STRING:
            return "string";
        case MCLTT_WORD:
            return "word";
        case MCLTT_EXTENDED_WORD:
            return "extended_word";
        case MCLTT_OPERATOR:
            return "operator";
        case MCLTT_COMMENT:
            return "comment";
        case MCLTT_MULTILINE_COMMENT:
            return "multiline_comment";
    }

    mapi_error(U, "wrong token type");
}

MORPHINE_API const char *mcapi_lex_operator2str(
    morphine_coroutine_t U,
    enum mc_lex_token_operator operator
) {
    for (size_t i = 0; i < sizeof(operator_table) / sizeof(operator_table[0]); i++) {
        if (operator_table[i].type == operator) {
            return operator_table[i].str;
        }
    }

    mapi_error(U, "wrong operator type");
}

MORPHINE_API const char *mcapi_lex_operator2name(
    morphine_coroutine_t U,
    enum mc_lex_token_operator operator
) {
    for (size_t i = 0; i < sizeof(operator_table) / sizeof(operator_table[0]); i++) {
        if (operator_table[i].type == operator) {
            return operator_table[i].name;
        }
    }

    mapi_error(U, "wrong operator type");
}
