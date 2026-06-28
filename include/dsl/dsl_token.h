#ifndef DSL_TOKEN_H
#define DSL_TOKEN_H

#include <stddef.h>

typedef enum {
    DSL_TOKEN_EOF,
    DSL_TOKEN_ERROR,
    DSL_TOKEN_NEWLINE,

    DSL_TOKEN_GET,
    DSL_TOKEN_POST,
    DSL_TOKEN_PUT,
    DSL_TOKEN_DELETE,
    DSL_TOKEN_PATCH,

    DSL_TOKEN_BODY_STMT,
    DSL_TOKEN_EXPECT,
    DSL_TOKEN_STATUS,
    DSL_TOKEN_HEADER,
    DSL_TOKEN_BODY,
    DSL_TOKEN_CONTAINS,

    DSL_TOKEN_INT,
    DSL_TOKEN_STRING,
    DSL_TOKEN_IDENT,

    DSL_TOKEN_BODY_RAW,    
} DslTokenType;

typedef struct {
    DslTokenType type;
    char *str;
    size_t line;
    size_t column;
} DslToken;

void dsl_free_token(DslToken token);
const char *dsl_token_type_name(DslTokenType type);

#endif