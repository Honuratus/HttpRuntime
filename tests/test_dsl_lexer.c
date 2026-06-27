#include <string.h>

#include "unity.h"

#include "dsl_lexer.h"
#include "dsl_token.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void assert_token(DslToken token, DslTokenType type, const char *str)
{
    TEST_ASSERT_EQUAL_INT(type, token.type);

    if (str) {
        TEST_ASSERT_NOT_NULL(token.str);
        TEST_ASSERT_EQUAL_STRING(str, token.str);
    } else {
        TEST_ASSERT_NULL(token.str);
    }
}

static void test_lexer_get_expect_status(void)
{
    const char *input =
        "GET https://example.com\n"
        "EXPECT status 200\n";

    DslLexer *lexer = dsl_create_lexer(input);
    TEST_ASSERT_NOT_NULL(lexer);

    DslToken t;

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_GET, "GET");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_IDENT, "https://example.com");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_NEWLINE, "\\n");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_EXPECT, "EXPECT");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_STATUS, "status");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_INT, "200");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_NEWLINE, "\\n");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_EOF, NULL);
    dsl_free_token(t);

    dsl_free_lexer(lexer);
}

static void test_lexer_header_and_body_contains(void)
{
    const char *input =
        "EXPECT header Content-Type text/html\n"
        "EXPECT body contains \"Example Domain\"\n";

    DslLexer *lexer = dsl_create_lexer(input);
    TEST_ASSERT_NOT_NULL(lexer);

    DslToken t;

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_EXPECT, "EXPECT");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_HEADER, "header");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_IDENT, "Content-Type");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_IDENT, "text/html");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_NEWLINE, "\\n");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_EXPECT, "EXPECT");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_BODY, "body");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_CONTAINS, "contains");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_STRING, "Example Domain");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_NEWLINE, "\\n");
    dsl_free_token(t);

    t = dsl_lexer_next_token(lexer);
    assert_token(t, DSL_TOKEN_EOF, NULL);
    dsl_free_token(t);

    dsl_free_lexer(lexer);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_lexer_get_expect_status);
    RUN_TEST(test_lexer_header_and_body_contains);

    return UNITY_END();
}