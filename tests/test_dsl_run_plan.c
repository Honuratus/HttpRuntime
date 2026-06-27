#include <stdlib.h>
#include <string.h>

#include "unity.h"

#include "dsl_parser.h"
#include "run_plan.h"
#include "engine.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_dsl_text_runs_successfully(void)
{
    const char *source =
        "GET https://example.com\n"
        "EXPECT status 200\n"
        "EXPECT body contains \"Example Domain\"\n"
        "EXPECT header Content-Type text/html\n";

    RunPlan plan;
    RunResult result;
    DslParseError error;

    init_run_plan(&plan);
    init_run_result(&result);

    DslParseResult parse_result = dsl_parse_run_plan(source, &plan, &error);

    TEST_ASSERT_EQUAL_INT(DSL_PARSE_OK, parse_result);
    TEST_ASSERT_EQUAL_size_t(1, plan.case_count);

    TEST_ASSERT_NOT_NULL(plan.cases[0].url);
    TEST_ASSERT_EQUAL_STRING("https://example.com", plan.cases[0].url);
    TEST_ASSERT_EQUAL_size_t(3, plan.cases[0].assertion_count);

    int rc = run_plan(&plan, &result);

    printf("parse_result=%d\n", parse_result);
    printf("plan.case_count=%zu\n", plan.case_count);
    printf("assertion_count=%zu\n", plan.cases[0].assertion_count);
    printf("rc=%d exit=%d all_req=%d all_assert=%d case_results=%zu\n",
       rc,
       result.exit_code,
       result.all_requests_ok,
       result.all_assertions_ok,
       result.case_result_count);


    printf("case request_ok=%d assertions_ok=%d assertion_results=%zu\n",
       result.case_results[0].request_ok,
       result.case_results[0].assertions_ok,
       result.case_results[0].assertion_result_count);

    for (size_t i = 0; i < result.case_results[0].assertion_result_count; i++) {
        printf("assert[%zu] passed=%d\n",
            i,
            result.case_results[0].assertion_results[i].passed);
    }
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(0, result.exit_code);

    TEST_ASSERT_TRUE(result.all_requests_ok);
    TEST_ASSERT_TRUE(result.all_assertions_ok);

    TEST_ASSERT_EQUAL_size_t(1, result.case_result_count);
    TEST_ASSERT_TRUE(result.case_results[0].request_ok);
    TEST_ASSERT_TRUE(result.case_results[0].assertions_ok);
    TEST_ASSERT_NOT_NULL(result.case_results[0].response);

    TEST_ASSERT_EQUAL_INT(200, result.case_results[0].response->status_code);
    TEST_ASSERT_EQUAL_size_t(3, result.case_results[0].assertion_result_count);

    for (size_t i = 0; i < result.case_results[0].assertion_result_count; i++) {
        TEST_ASSERT_TRUE(result.case_results[0].assertion_results[i].passed);
    }

    free_run_result(&result);
    free_run_plan(&plan);
}

static void test_dsl_text_runs_and_fails_assertion(void)
{
    const char *source =
        "GET https://example.com\n"
        "EXPECT status 404\n"
        "EXPECT body contains \"Example Domain\"\n";

    RunPlan plan;
    RunResult result;
    DslParseError error;

    init_run_plan(&plan);
    init_run_result(&result);

    DslParseResult parse_result = dsl_parse_run_plan(source, &plan, &error);

    TEST_ASSERT_EQUAL_INT(DSL_PARSE_OK, parse_result);
    TEST_ASSERT_EQUAL_size_t(1, plan.case_count);
    TEST_ASSERT_EQUAL_size_t(2, plan.cases[0].assertion_count);

    int rc = run_plan(&plan, &result);

    TEST_ASSERT_EQUAL_INT(1, rc);
    TEST_ASSERT_EQUAL_INT(1, result.exit_code);

    TEST_ASSERT_TRUE(result.all_requests_ok);
    TEST_ASSERT_FALSE(result.all_assertions_ok);

    TEST_ASSERT_EQUAL_size_t(1, result.case_result_count);
    TEST_ASSERT_TRUE(result.case_results[0].request_ok);
    TEST_ASSERT_FALSE(result.case_results[0].assertions_ok);

    TEST_ASSERT_EQUAL_size_t(2, result.case_results[0].assertion_result_count);

    TEST_ASSERT_FALSE(result.case_results[0].assertion_results[0].passed);
    TEST_ASSERT_TRUE(result.case_results[0].assertion_results[1].passed);

    free_run_result(&result);
    free_run_plan(&plan);
}

static void test_dsl_text_parse_error_expect_before_request(void)
{
    const char *source =
        "EXPECT status 200\n";

    RunPlan plan;
    DslParseError error;

    init_run_plan(&plan);

    DslParseResult parse_result = dsl_parse_run_plan(source, &plan, &error);

    TEST_ASSERT_EQUAL_INT(DSL_PARSE_ERROR, parse_result);
    TEST_ASSERT_TRUE(strlen(error.message) > 0);

    free_run_plan(&plan);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_dsl_text_runs_successfully);
    RUN_TEST(test_dsl_text_runs_and_fails_assertion);
    RUN_TEST(test_dsl_text_parse_error_expect_before_request);

    return UNITY_END();
}