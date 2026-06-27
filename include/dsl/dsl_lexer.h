#ifndef DSL_LEXER_H
#define DSL_LEXER_H

#include <stddef.h>

#include "dsl_token.h"

typedef struct {
    const char *input;
    size_t len;
    size_t pos;
    size_t line;
    size_t column;
} DslLexer;

DslLexer *dsl_create_lexer(const char *input);
void dsl_free_lexer(DslLexer *lexer);

DslToken dsl_lexer_next_token(DslLexer *lexer);

DslToken *dsl_lexer_get_tokens(DslLexer *lexer, size_t *out_count);
void dsl_free_tokens(DslToken *tokens, size_t count);

#endif