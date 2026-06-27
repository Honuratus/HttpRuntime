#include <string.h>
#include <stdlib.h>

#include "run_plan.h"
#include "models.h"

void init_run_plan(RunPlan *plan)
{
    if (!plan) {
        return;
    }

    memset(plan, 0, sizeof(*plan));

    plan->worker_count = 4;
    plan->save_history = true;
    plan->default_timeout_ms = 5000;

    plan->db_path = NULL;
    plan->collection_name = NULL;
}

void free_run_plan(RunPlan *plan)
{
    if (!plan) {
        return;
    }

    for (size_t i = 0; i < plan->case_count; i++) {
        RequestCase *request_case = &plan->cases[i];

        free(request_case->url);
        request_case->url = NULL;

        free(request_case->headers);
        request_case->headers = NULL;

        free(request_case->body);
        request_case->body = NULL;
        request_case->body_len = 0;

        request_case->assertion_count = 0;
    }

    free(plan->db_path);
    plan->db_path = NULL;

    free(plan->collection_name);
    plan->collection_name = NULL;

    memset(plan, 0, sizeof(*plan));
}

void init_run_result(RunResult *result)
{
    if (!result) {
        return;
    }

    memset(result, 0, sizeof(*result));

    result->exit_code = 1;
    result->all_requests_ok = false;
    result->all_assertions_ok = false;
}

void free_run_result(RunResult *result)
{
    if (!result) {
        return;
    }

    for (size_t i = 0; i < result->case_result_count; i++) {
        RequestCaseResult *case_result = &result->case_results[i];

        if (case_result->response) {
            free_response(case_result->response);
            case_result->response = NULL;
        }

        case_result->assertion_result_count = 0;
        case_result->request_ok = false;
        case_result->assertions_ok = false;
    }

    memset(result, 0, sizeof(*result));
}