#include "dsl_lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static char *dsl_strndup(const char *s, size_t n)
{
    char *copy = malloc(n + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, s, n);
    copy[n] = '\0';

    return copy;
}

static char peek(DslLexer *lexer)
{
    if (!lexer || lexer->pos >= lexer->len) {
        return '\0';
    }

    return lexer->input[lexer->pos];
}

static char peek_next(DslLexer *lexer)
{
    if (!lexer || lexer->pos + 1 >= lexer->len) {
        return '\0';
    }

    return lexer->input[lexer->pos + 1];
}

static char advance(DslLexer *lexer)
{
    char c = peek(lexer);

    if (c == '\0') {
        return c;
    }

    lexer->pos++;

    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }

    return c;
}

static DslToken make_token(DslTokenType type, char *str, size_t line, size_t column)
{
    DslToken token;

    token.type = type;
    token.str = str;
    token.line = line;
    token.column = column;

    return token;
}

static DslToken make_error_token(const char *message, size_t line, size_t column)
{
    return make_token(DSL_TOKEN_ERROR, dsl_strndup(message, strlen(message)), line, column);
}

static void skip_spaces_and_tabs(DslLexer *lexer)
{
    while (peek(lexer) == ' ' || peek(lexer) == '\t' || peek(lexer) == '\r') {
        advance(lexer);
    }
}

static bool is_ident_start(char c)
{
    return isalpha((unsigned char)c) || c == '_' || c == '-' || c == ':' || c == '/' || c == '.';
}

static bool is_ident_char(char c)
{
    return isalnum((unsigned char)c) ||
           c == '_' ||
           c == '-' ||
           c == ':' ||
           c == '/' ||
           c == '.' ||
           c == '?' ||
           c == '&' ||
           c == '=' ||
           c == '%' ||
           c == '#';
}

static DslTokenType keyword_type(const char *word)
{
    if (strcmp(word, "GET") == 0) return DSL_TOKEN_GET;
    if (strcmp(word, "POST") == 0) return DSL_TOKEN_POST;
    if (strcmp(word, "PUT") == 0) return DSL_TOKEN_PUT;
    if (strcmp(word, "DELETE") == 0) return DSL_TOKEN_DELETE;
    if (strcmp(word, "PATCH") == 0) return DSL_TOKEN_PATCH;

    if (strcmp(word, "EXPECT") == 0) return DSL_TOKEN_EXPECT;
    if (strcmp(word, "status") == 0) return DSL_TOKEN_STATUS;
    if (strcmp(word, "header") == 0) return DSL_TOKEN_HEADER;
    if (strcmp(word, "body") == 0) return DSL_TOKEN_BODY;
    if (strcmp(word, "contains") == 0) return DSL_TOKEN_CONTAINS;
    if(strcmp(word, "BODY") == 0) return DSL_TOKEN_BODY_STMT;

    return DSL_TOKEN_IDENT;
}

static DslToken scan_number(DslLexer *lexer)
{
    size_t start = lexer->pos;
    size_t line = lexer->line;
    size_t column = lexer->column;

    while (isdigit((unsigned char)peek(lexer))) {
        advance(lexer);
    }

    char *num = dsl_strndup(lexer->input + start, lexer->pos - start);
    if (!num) {
        return make_error_token("out of memory", line, column);
    }

    return make_token(DSL_TOKEN_INT, num, line, column);
}

static DslToken scan_identifier(DslLexer *lexer)
{
    size_t start = lexer->pos;
    size_t line = lexer->line;
    size_t column = lexer->column;

    while (is_ident_char(peek(lexer))) {
        advance(lexer);
    }

    char *word = dsl_strndup(lexer->input + start, lexer->pos - start);
    if (!word) {
        return make_error_token("out of memory", line, column);
    }

    DslTokenType type = keyword_type(word);
    if (type == DSL_TOKEN_BODY_STMT){
        lexer->read_body_raw_next = true;
    }
    
    return make_token(type, word, line, column);
}

static DslToken scan_string(DslLexer *lexer)
{
    size_t line = lexer->line;
    size_t column = lexer->column;

    advance(lexer); /* opening quote */

    size_t start = lexer->pos;

    while (peek(lexer) != '"' && peek(lexer) != '\0' && peek(lexer) != '\n') {
        advance(lexer);
    }

    if (peek(lexer) != '"') {
        return make_error_token("unterminated string", line, column);
    }

    size_t len = lexer->pos - start;
    char *str = dsl_strndup(lexer->input + start, len);

    advance(lexer); /* closing quote */

    if (!str) {
        return make_error_token("out of memory", line, column);
    }

    return make_token(DSL_TOKEN_STRING, str, line, column);
}

static DslToken scan_body_raw(DslLexer *lexer)
{
    size_t line = lexer->line;
    size_t column;
    size_t start;
    size_t len;

    while (peek(lexer) == ' ' || peek(lexer) == '\t') {
        advance(lexer);
    }

    start = lexer->pos;
    column = lexer->column;

    while (peek(lexer) != '\0' &&
           peek(lexer) != '\n' &&
           peek(lexer) != '\r') {
        advance(lexer);
    }

    len = lexer->pos - start;

    while (len > 0 &&
          (lexer->input[start + len - 1] == ' ' ||
           lexer->input[start + len - 1] == '\t')) {
        len--;
    }

    char *raw = dsl_strndup(lexer->input + start, len);
    if (!raw) {
        return make_error_token("out of memory", line, column);
    }

    return make_token(DSL_TOKEN_BODY_RAW, raw, line, column);
}

DslLexer *dsl_create_lexer(const char *input)
{
    if (!input) {
        return NULL;
    }

    DslLexer *lexer = malloc(sizeof(DslLexer));
    if (!lexer) {
        return NULL;
    }

    lexer->input = input;
    lexer->len = strlen(input);
    lexer->pos = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->read_body_raw_next = false;
    return lexer;
}

void dsl_free_lexer(DslLexer *lexer)
{
    free(lexer);
}

DslToken dsl_lexer_next_token(DslLexer *lexer)
{
    if (!lexer) {
        return make_error_token("lexer is null", 0, 0);
    }

    skip_spaces_and_tabs(lexer);

    size_t line = lexer->line;
    size_t column = lexer->column;

    char c = peek(lexer);

    if (lexer->read_body_raw_next){
        lexer->read_body_raw_next = false;
        return scan_body_raw(lexer);
    }

    if (c == '\0') {
        return make_token(DSL_TOKEN_EOF, NULL, line, column);
    }

    if (c == '\n') {
        advance(lexer);
        return make_token(DSL_TOKEN_NEWLINE, dsl_strndup("\\n", 2), line, column);
    }

    if (c == '"') {
        return scan_string(lexer);
    }

    if (isdigit((unsigned char)c)) {
        return scan_number(lexer);
    }

    if (is_ident_start(c)) {
        return scan_identifier(lexer);
    }

    

    if (c == '#') {
        while (peek(lexer) != '\0' && peek(lexer) != '\n') {
            advance(lexer);
        }
        return dsl_lexer_next_token(lexer);
    }

    char msg[64];
    snprintf(msg, sizeof(msg), "unexpected character '%c'", c);
    advance(lexer);

    return make_error_token(msg, line, column);
}

DslToken *dsl_lexer_get_tokens(DslLexer *lexer, size_t *out_count)
{
    size_t capacity = 16;
    size_t count = 0;

    DslToken *tokens = malloc(sizeof(DslToken) * capacity);
    if (!tokens) {
        return NULL;
    }

    while (true) {
        if (count >= capacity) {
            capacity *= 2;

            DslToken *tmp = realloc(tokens, sizeof(DslToken) * capacity);
            if (!tmp) {
                dsl_free_tokens(tokens, count);
                return NULL;
            }

            tokens = tmp;
        }

        DslToken token = dsl_lexer_next_token(lexer);
        tokens[count++] = token;

        if (token.type == DSL_TOKEN_EOF || token.type == DSL_TOKEN_ERROR) {
            break;
        }
    }

    if (out_count) {
        *out_count = count;
    }

    return tokens;
}

void dsl_free_token(DslToken token)
{
    free(token.str);
}

void dsl_free_tokens(DslToken *tokens, size_t count)
{
    if (!tokens) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        dsl_free_token(tokens[i]);
    }

    free(tokens);
}

const char *dsl_token_type_name(DslTokenType type)
{
    switch (type) {
        case DSL_TOKEN_EOF: return "EOF";
        case DSL_TOKEN_ERROR: return "ERROR";
        case DSL_TOKEN_NEWLINE: return "NEWLINE";

        case DSL_TOKEN_GET: return "GET";
        case DSL_TOKEN_POST: return "POST";
        case DSL_TOKEN_PUT: return "PUT";
        case DSL_TOKEN_DELETE: return "DELETE";
        case DSL_TOKEN_PATCH: return "PATCH";

        case DSL_TOKEN_EXPECT: return "EXPECT";
        case DSL_TOKEN_STATUS: return "STATUS";
        case DSL_TOKEN_HEADER: return "HEADER";
        case DSL_TOKEN_BODY: return "BODY";
        case DSL_TOKEN_CONTAINS: return "CONTAINS";

        case DSL_TOKEN_INT: return "INT";
        case DSL_TOKEN_STRING: return "STRING";
        case DSL_TOKEN_IDENT: return "IDENT";

        default: return "UNKNOWN";
    }
}