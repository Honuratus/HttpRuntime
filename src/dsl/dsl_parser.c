#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dsl_parser.h"
#include "dsl_lexer.h"
#include "dsl_token.h"
#include "run_plan.h"
#include "assertion.h"

extern char *strdup(const char *s);


static void advance_parser(DslParser *parser);

static bool check(DslParser *parser, DslTokenType type);
static bool match(DslParser *parser, DslTokenType type);
static bool consume(DslParser *parser, DslTokenType type, const char *message);

static void parser_error(DslParser *parser, const char *message);

static bool parse_program(DslParser *parser, RunPlan *plan);
static bool parse_statement(DslParser *parser, RunPlan *plan);
static bool parse_request_stmt(DslParser *parser, RunPlan *plan);
static bool parse_expect_stmt(DslParser *parser, RunPlan *plan);
static bool parse_body_stmt(DslParser* parser ,RunPlan* plan);

static bool parse_assertion(DslParser *parser, RequestCase *request_case);
static bool parse_status_assertion(DslParser *parser, RequestCase *request_case);
static bool parse_body_assertion(DslParser *parser, RequestCase *request_case);
static bool parse_header_assertion(DslParser *parser, RequestCase *request_case);

static bool check(DslParser *parser, DslTokenType type)
{
    if (!parser || !parser->has_current) {
        return false;
    }

    return parser->current.type == type;
}

static bool match(DslParser *parser, DslTokenType type)
{
    if (!check(parser, type)) {
        return false;
    }

    advance_parser(parser);
    return true;
}

static void parser_error(DslParser *parser, const char *message)
{
    if (!parser || parser->had_error) {
        return;
    }

    parser->had_error = true;

    if (parser->error) {
        snprintf(
            parser->error->message,
            sizeof(parser->error->message),
            "%s",
            message ? message : "parse error"
        );

        if (parser->has_current) {
            parser->error->line = parser->current.line;
            parser->error->column = parser->current.column;
        } else {
            parser->error->line = 0;
            parser->error->column = 0;
        }
    }
}

static bool consume(DslParser *parser, DslTokenType type, const char *message)
{
    if (check(parser, type)) {
        advance_parser(parser);
        return true;
    }

    parser_error(parser, message);
    return false;
}

static void advance_parser(DslParser *parser)
{
    if (!parser || !parser->lexer) {
        return;
    }

    if (parser->has_previous) {
        dsl_free_token(parser->previous);
        parser->has_previous = false;
    }

    if (parser->has_current) {
        parser->previous = parser->current;
        parser->has_previous = true;
    }

    parser->current = dsl_lexer_next_token(parser->lexer);
    parser->has_current = true;

    if (parser->current.type == DSL_TOKEN_ERROR) {
        parser->had_error = true;

        if (parser->error) {
            snprintf(
                parser->error->message,
                sizeof(parser->error->message),
                "%s",
                parser->current.str ? parser->current.str : "lexer error"
            );

            parser->error->line = parser->current.line;
            parser->error->column = parser->current.column;
        }
    }
}

DslParseResult dsl_parse_run_plan(
    const char *source,
    RunPlan *plan,
    DslParseError *error
)
{
    if (!source || !plan) {
        if (error) {
            snprintf(error->message, sizeof(error->message), "%s", "source or plan is null");
            error->line = 0;
            error->column = 0;
        }

        return DSL_PARSE_ERROR;
    }

    init_run_plan(plan);

    DslParser *parser = dsl_parser_create(source, error);
    if (!parser) {
        if (error) {
            snprintf(error->message, sizeof(error->message), "%s", "parser creation failed");
            error->line = 0;
            error->column = 0;
        }

        return DSL_PARSE_ERROR;
    }

    bool ok = parse_program(parser, plan);

    if (!ok || parser->had_error) {
        dsl_parser_destroy(parser);
        free_run_plan(plan);
        return DSL_PARSE_ERROR;
    }

    dsl_parser_destroy(parser);
    return DSL_PARSE_OK;
}

DslParser *dsl_parser_create(const char *text, DslParseError *error)
{
    if (!text) {
        return NULL;
    }

    DslParser *parser = malloc(sizeof(DslParser));
    if (!parser) {
        return NULL;
    }

    memset(parser, 0, sizeof(*parser));

    parser->lexer = dsl_create_lexer(text);
    if (!parser->lexer) {
        free(parser);
        return NULL;
    }

    parser->error = error;

    if (parser->error) {
        parser->error->message[0] = '\0';
        parser->error->line = 0;
        parser->error->column = 0;
    }

    advance_parser(parser);

    return parser;
}

void dsl_parser_destroy(DslParser *parser)
{
    if (!parser) {
        return;
    }

    if (parser->has_current) {
        dsl_free_token(parser->current);
    }

    if (parser->has_previous) {
        dsl_free_token(parser->previous);
    }

    dsl_free_lexer(parser->lexer);
    free(parser);
}


static bool parse_program(DslParser *parser, RunPlan *plan)
{
    if (!parser || !plan) {
        return false;
    }

    while (!check(parser, DSL_TOKEN_EOF)) {
        if (parser->had_error) {
            return false;
        }

        if (!parse_statement(parser, plan)) {
            return false;
        }
    }

    return !parser->had_error;
}

static bool parse_statement(DslParser *parser, RunPlan *plan)
{
    if (check(parser, DSL_TOKEN_NEWLINE)) {
        advance_parser(parser);
        return true;
    }

    if (check(parser, DSL_TOKEN_GET) ||
        check(parser, DSL_TOKEN_POST) ||
        check(parser, DSL_TOKEN_PUT) ||
        check(parser, DSL_TOKEN_DELETE) ||
        check(parser, DSL_TOKEN_PATCH)) {
        return parse_request_stmt(parser, plan);
    }
    
    if (check(parser, DSL_TOKEN_BODY_STMT)) {
        return parse_body_stmt(parser, plan);
    }

    if (check(parser, DSL_TOKEN_EXPECT)) {
        return parse_expect_stmt(parser, plan);
    }

    parser_error(parser, "expected request statement or EXPECT statement");
    return false;
}

static Method method_from_token(DslTokenType type)
{
    switch (type) {
        case DSL_TOKEN_GET:
            return GET;
        case DSL_TOKEN_POST:
            return POST;
        case DSL_TOKEN_PUT:
            return PUT;
        case DSL_TOKEN_DELETE:
            return DELETE;
        case DSL_TOKEN_PATCH:
            return PATCH;
        default:
            return GET;
    }
}

static bool parse_body_stmt(DslParser *parser, RunPlan *plan)
{
    if (!parser || !plan) {
        return false;
    }

    if (plan->case_count == 0) {
        parser_error(parser, "BODY must appear after a request statement");
        return false;
    }

    if (!consume(parser, DSL_TOKEN_BODY_STMT, "expected BODY")) {
        return false;
    }

    if (!consume(parser, DSL_TOKEN_BODY_RAW, "expected body value after BODY")) {
        return false;
    }

    RequestCase *request_case = &plan->cases[plan->case_count - 1];

    free(request_case->body);

    request_case->body = strdup(parser->previous.str);
    if (!request_case->body) {
        parser_error(parser, "out of memory while copying request body");
        return false;
    }

    request_case->body_len = strlen(request_case->body);

    if (check(parser, DSL_TOKEN_NEWLINE)) {
        advance_parser(parser);
        return true;
    }

    if (check(parser, DSL_TOKEN_EOF)) {
        return true;
    }

    parser_error(parser, "expected newline after BODY statement");
    return false;
}

static bool parse_request_stmt(DslParser *parser, RunPlan *plan)
{
    if (!parser || !plan) {
        return false;
    }

    if (plan->case_count >= MAX_REQUEST_CASES) {
        parser_error(parser, "too many request cases");
        return false;
    }

    DslTokenType method_token = parser->current.type;

    if (!(method_token == DSL_TOKEN_GET ||
          method_token == DSL_TOKEN_POST ||
          method_token == DSL_TOKEN_PUT ||
          method_token == DSL_TOKEN_DELETE ||
          method_token == DSL_TOKEN_PATCH)) {
        parser_error(parser, "expected HTTP method");
        return false;
    }

    advance_parser(parser);

    if (!consume(parser, DSL_TOKEN_IDENT, "expected URL after HTTP method")) {
        return false;
    }

    RequestCase *request_case = &plan->cases[plan->case_count];
    memset(request_case, 0, sizeof(*request_case));

    request_case->method = method_from_token(method_token);
    request_case->url = strdup(parser->previous.str);

    if (!request_case->url) {
        parser_error(parser, "out of memory while copying URL");
        return false;
    }

    request_case->headers = NULL;
    request_case->body = NULL;
    request_case->body_len = 0;
    request_case->timeout_ms = plan->default_timeout_ms > 0
        ? plan->default_timeout_ms
        : 5000;

    request_case->assertion_count = 0;

    plan->case_count++;

    if (check(parser, DSL_TOKEN_NEWLINE)) {
        advance_parser(parser);
        return true;
    }

    if (check(parser, DSL_TOKEN_EOF)) {
        return true;
    }

    parser_error(parser, "expected newline after request statement");
    return false;
}


static bool parse_expect_stmt(DslParser *parser, RunPlan *plan)
{
    if (!parser || !plan) {
        return false;
    }

    if (plan->case_count == 0) {
        parser_error(parser, "EXPECT statement must come after a request statement");
        return false;
    }

    if (!consume(parser, DSL_TOKEN_EXPECT, "expected EXPECT")) {
        return false;
    }

    RequestCase *request_case = &plan->cases[plan->case_count - 1];

    if (request_case->assertion_count >= MAX_CASE_ASSERTIONS) {
        parser_error(parser, "too many assertions for request case");
        return false;
    }

    if (!parse_assertion(parser, request_case)) {
        return false;
    }

    if (check(parser, DSL_TOKEN_NEWLINE)) {
        advance_parser(parser);
        return true;
    }

    if (check(parser, DSL_TOKEN_EOF)) {
        return true;
    }

    parser_error(parser, "expected newline after EXPECT statement");
    return false;
}

static bool parse_assertion(DslParser *parser, RequestCase *request_case)
{
    if (!parser || !request_case) {
        return false;
    }

    if (check(parser, DSL_TOKEN_STATUS)) {
        return parse_status_assertion(parser, request_case);
    }

    if (check(parser, DSL_TOKEN_BODY)) {
        return parse_body_assertion(parser, request_case);
    }

    if (check(parser, DSL_TOKEN_HEADER)) {
        return parse_header_assertion(parser, request_case);
    }

    parser_error(parser, "expected assertion type: status, body, or header");
    return false;
}

static bool parse_status_assertion(DslParser *parser, RequestCase *request_case)
{
    if (!parser || !request_case) {
        return false;
    }

    if (request_case->assertion_count >= MAX_CASE_ASSERTIONS) {
        parser_error(parser, "too many assertions for request case");
        return false;
    }

    if (!consume(parser, DSL_TOKEN_STATUS, "expected 'status'")) {
        return false;
    }

    if (!consume(parser, DSL_TOKEN_INT, "expected status code after 'status'")) {
        return false;
    }

    int expected_status = atoi(parser->previous.str);

    Assertion *assertion = &request_case->assertions[request_case->assertion_count++];

    memset(assertion, 0, sizeof(*assertion));

    assertion->type = ASSERT_STATUS_EQ;
    assertion->expected.type = VALUE_INT;
    assertion->expected.as.int_value = expected_status;

    return true;
}

static bool parse_body_assertion(DslParser *parser, RequestCase *request_case)
{
    if (!parser || !request_case) {
        return false;
    }

    if (request_case->assertion_count >= MAX_CASE_ASSERTIONS) {
        parser_error(parser, "too many assertions for request case");
        return false;
    }

    if (!consume(parser, DSL_TOKEN_BODY, "expected 'body'")) {
        return false;
    }

    if (!consume(parser, DSL_TOKEN_CONTAINS, "expected 'contains' after 'body'")) {
        return false;
    }

    if (!consume(parser, DSL_TOKEN_STRING, "expected string after 'body contains'")) {
        return false;
    }

    Assertion *assertion = &request_case->assertions[request_case->assertion_count++];

    memset(assertion, 0, sizeof(*assertion));

    assertion->type = ASSERT_BODY_CONTAINS;
    assertion->expected.type = VALUE_BYTES;
    assertion->expected.as.byte_value.data = (unsigned char *)strdup(parser->previous.str);
    assertion->expected.as.byte_value.len = strlen(parser->previous.str);

    if (!assertion->expected.as.string_value) {
        parser_error(parser, "out of memory while copying body assertion");
        return false;
    }

    return true;
}

static bool parse_header_assertion(DslParser *parser, RequestCase *request_case)
{
    if (!parser || !request_case) {
        return false;
    }

    if (request_case->assertion_count >= MAX_CASE_ASSERTIONS) {
        parser_error(parser, "too many assertions for request case");
        return false;
    }

    if (!consume(parser, DSL_TOKEN_HEADER, "expected 'header'")) {
        return false;
    }

    if (!consume(parser, DSL_TOKEN_IDENT, "expected header name after 'header'")) {
        return false;
    }

    char *header_name = strdup(parser->previous.str);
    if (!header_name) {
        parser_error(parser, "out of memory while copying header name");
        return false;
    }

    if (!(check(parser, DSL_TOKEN_IDENT) ||
          check(parser, DSL_TOKEN_STRING) ||
          check(parser, DSL_TOKEN_INT))) {
        free(header_name);
        parser_error(parser, "expected header value after header name");
        return false;
    }

    advance_parser(parser);

    char *header_value = strdup(parser->previous.str);
    if (!header_value) {
        free(header_name);
        parser_error(parser, "out of memory while copying header value");
        return false;
    }

    Assertion *assertion = &request_case->assertions[request_case->assertion_count++];

    memset(assertion, 0, sizeof(*assertion));

    assertion->type = ASSERT_HEADER_CONTAINS;
    assertion->expected.type = VALUE_HEADER;
    assertion->expected.as.header_value.name = header_name;
    assertion->expected.as.header_value.value = header_value;

    return true;
}