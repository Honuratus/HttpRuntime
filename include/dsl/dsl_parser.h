#ifndef DSL_PARSER_H
#define DSL_PARSER_H

#include "run_plan.h"
#include "dsl_token.h"
#include "dsl_lexer.h"

typedef enum {
    DSL_PARSE_OK = 0,
    DSL_PARSE_ERROR = 1
} DslParseResult;

typedef struct {
    char message[256];
    size_t line;
    size_t column;
} DslParseError;

typedef struct {
    DslLexer *lexer;

    DslToken current;
    DslToken previous;

    DslParseError *error;

    bool had_error;
    bool has_previous;
    bool has_current;
} DslParser;




DslParser *dsl_parser_create(const char *text, DslParseError *error);
void dsl_parser_destroy(DslParser *parser);


DslParseResult dsl_parse_run_plan(
    const char *source,
    RunPlan *plan,
    DslParseError *error
);



#endif