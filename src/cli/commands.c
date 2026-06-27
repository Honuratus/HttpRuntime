#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"

#include "engine.h"
#include "run_plan.h"
#include "models.h"
#include "assertion.h"
#include "cli_output.h"

static char *dup_string(const char *s)
{
    if (!s) {
        return NULL;
    }

    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, s, len + 1);
    return copy;
}

static int cli_options_to_run_plan(const CliOptions *options, RunPlan *plan)
{
    if (!options || !plan) {
        return -1;
    }

    if (!options->url || *options->url == '\0') {
        printf("URL is required.\n");
        return -1;
    }

    init_run_plan(plan);

    plan->worker_count = 1;
    plan->default_timeout_ms = 5000;
    plan->save_history = true;
    plan->db_path = dup_string("runner.db");
    plan->collection_name = dup_string("DEFAULT");

    if (!plan->db_path || !plan->collection_name) {
        return -1;
    }

    plan->case_count = 1;

    RequestCase *request_case = &plan->cases[0];

    request_case->method = GET;
    request_case->url = dup_string(options->url);
    request_case->headers = NULL;
    request_case->body = NULL;
    request_case->body_len = 0;
    request_case->timeout_ms = 5000;

    if (!request_case->url) {
        return -1;
    }

    if (options->assertion_count > MAX_CASE_ASSERTIONS) {
        printf("Too many assertions. Max: %d\n", MAX_CASE_ASSERTIONS);
        return -1;
    }

    request_case->assertion_count = options->assertion_count;

    for (size_t i = 0; i < options->assertion_count; i++) {
        request_case->assertions[i] = options->assertions[i];
    }

    return 0;
}

static void print_run_result(const RunResult *result)
{
    if (!result) {
        return;
    }

    for (size_t i = 0; i < result->case_result_count; i++) {
        const RequestCaseResult *case_result = &result->case_results[i];

        if (!case_result->response) {
            printf("Request %zu failed or timed out.\n", i + 1);
            continue;
        }

        print_response_summary(case_result->response);

        if (case_result->assertion_result_count > 0) {
            printf("\nAssertions:\n");

            for (size_t j = 0; j < case_result->assertion_result_count; j++) {
                print_assertion_result(&case_result->assertion_results[j]);
            }

            printf("\nAssertion Result: %s\n",
                case_result->assertions_ok ? "PASS" : "FAIL");
        }
    }
}

int run_get_command(const CliOptions *options)
{
    int exit_code = 1;

    RunPlan plan;
    RunResult result;

    init_run_plan(&plan);
    init_run_result(&result);

    if (cli_options_to_run_plan(options, &plan) != 0) {
        goto cleanup;
    }

    exit_code = run_plan(&plan, &result);

    print_run_result(&result);

cleanup:
    free_run_result(&result);
    free_run_plan(&plan);

    return exit_code;
}

int run_command(const CliOptions *options)
{
    if (!options || !options->command) {
        printf("No command provided.\n");
        return 1;
    }

    if (strcmp("GET", options->command) == 0) {
        return run_get_command(options);
    }

    printf("Unsupported command: %s\n", options->command);
    return 1;
}