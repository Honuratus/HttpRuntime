#include <string.h>
#include <stdlib.h>

#include "unity.h"

#include "engine.h"
#include "run_plan.h"
#include "models.h"
#include "assertion.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void setup_get_case_status_200(RequestCase *request_case, const char *url)
{
    memset(request_case, 0, sizeof(*request_case));

    request_case->method = GET;
    request_case->url = strdup(url);
    request_case->headers = NULL;
    request_case->body = NULL;
    request_case->body_len = 0;
    request_case->timeout_ms = 5000;

    request_case->assertion_count = 1;
    request_case->assertions[0].type = ASSERT_STATUS_EQ;
    request_case->assertions[0].expected.type = VALUE_INT;
    request_case->assertions[0].expected.as.int_value = 200;
}

static void setup_get_case_status_expected(RequestCase *request_case, const char *url, int expected_status)
{
    memset(request_case, 0, sizeof(*request_case));

    request_case->method = GET;
    request_case->url = strdup(url);
    request_case->headers = NULL;
    request_case->body = NULL;
    request_case->body_len = 0;
    request_case->timeout_ms = 5000;

    request_case->assertion_count = 1;
    request_case->assertions[0].type = ASSERT_STATUS_EQ;
    request_case->assertions[0].expected.type = VALUE_INT;
    request_case->assertions[0].expected.as.int_value = expected_status;
}

static void test_run_plan_four_cases_all_pass(void)
{
    RunPlan plan;
    RunResult result;

    init_run_plan(&plan);
    init_run_result(&result);

    plan.worker_count = 2;
    plan.default_timeout_ms = 5000;
    plan.save_history = true;
    plan.db_path = strdup("runner.db");
    plan.collection_name = strdup("TEST_FOUR_CASES_PASS");

    plan.case_count = 4;

    setup_get_case_status_200(&plan.cases[0], "https://example.com");
    setup_get_case_status_200(&plan.cases[1], "https://example.com");
    setup_get_case_status_200(&plan.cases[2], "https://example.com");
    setup_get_case_status_200(&plan.cases[3], "https://example.com");

    int rc = run_plan(&plan, &result);

    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(0, result.exit_code);

    TEST_ASSERT_TRUE(result.all_requests_ok);
    TEST_ASSERT_TRUE(result.all_assertions_ok);

    TEST_ASSERT_EQUAL_size_t(4, result.case_result_count);

    for (size_t i = 0; i < result.case_result_count; i++) {
        RequestCaseResult *case_result = &result.case_results[i];

        TEST_ASSERT_TRUE(case_result->request_ok);
        TEST_ASSERT_TRUE(case_result->assertions_ok);
        TEST_ASSERT_NOT_NULL(case_result->response);

        TEST_ASSERT_EQUAL_INT(200, case_result->response->status_code);

        TEST_ASSERT_EQUAL_size_t(1, case_result->assertion_result_count);
        TEST_ASSERT_TRUE(case_result->assertion_results[0].passed);
    }

    free_run_result(&result);
    free_run_plan(&plan);
}

static void test_run_plan_four_cases_one_assertion_fails(void)
{
    RunPlan plan;
    RunResult result;

    init_run_plan(&plan);
    init_run_result(&result);

    plan.worker_count = 2;
    plan.default_timeout_ms = 5000;
    plan.save_history = true;
    plan.db_path = strdup("runner.db");
    plan.collection_name = strdup("TEST_FOUR_CASES_FAIL");

    plan.case_count = 4;

    setup_get_case_status_200(&plan.cases[0], "https://example.com");
    setup_get_case_status_200(&plan.cases[1], "https://example.com");

    /*
     * This one should fail:
     * example.com returns 200, but we expect 404.
     */
    setup_get_case_status_expected(&plan.cases[2], "https://example.com", 404);

    setup_get_case_status_200(&plan.cases[3], "https://example.com");

    int rc = run_plan(&plan, &result);

    TEST_ASSERT_EQUAL_INT(1, rc);
    TEST_ASSERT_EQUAL_INT(1, result.exit_code);

    TEST_ASSERT_TRUE(result.all_requests_ok);
    TEST_ASSERT_FALSE(result.all_assertions_ok);

    TEST_ASSERT_EQUAL_size_t(4, result.case_result_count);

    TEST_ASSERT_TRUE(result.case_results[0].assertions_ok);
    TEST_ASSERT_TRUE(result.case_results[1].assertions_ok);
    TEST_ASSERT_FALSE(result.case_results[2].assertions_ok);
    TEST_ASSERT_TRUE(result.case_results[3].assertions_ok);

    TEST_ASSERT_TRUE(result.case_results[0].assertion_results[0].passed);
    TEST_ASSERT_TRUE(result.case_results[1].assertion_results[0].passed);
    TEST_ASSERT_FALSE(result.case_results[2].assertion_results[0].passed);
    TEST_ASSERT_TRUE(result.case_results[3].assertion_results[0].passed);

    free_run_result(&result);
    free_run_plan(&plan);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_run_plan_four_cases_all_pass);
    RUN_TEST(test_run_plan_four_cases_one_assertion_fails);

    return UNITY_END();
}