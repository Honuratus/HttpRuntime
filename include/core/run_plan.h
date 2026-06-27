#ifndef RUN_PLAN_H
#define RUN_PLAN_H

#include <stddef.h>
#include <stdbool.h>

#include "models.h"
#include "assertion.h"

#define MAX_REQUEST_CASES 128
#define MAX_CASE_ASSERTIONS 32

typedef struct {
    Method method;

    char *url;
    char *headers;

    unsigned char *body;
    size_t body_len;

    Assertion assertions[MAX_CASE_ASSERTIONS];
    size_t assertion_count;

    int timeout_ms;
} RequestCase;

typedef struct {
    RequestCase cases[MAX_REQUEST_CASES];
    size_t case_count;

    size_t worker_count;

    bool save_history;
    char *db_path;
    char *collection_name;

    int default_timeout_ms;
} RunPlan;

typedef struct {
    Response *response;

    AssertionResult assertion_results[MAX_CASE_ASSERTIONS];
    size_t assertion_result_count;

    bool request_ok;
    bool assertions_ok;
} RequestCaseResult;

typedef struct {
    RequestCaseResult case_results[MAX_REQUEST_CASES];
    size_t case_result_count;

    bool all_requests_ok;
    bool all_assertions_ok;

    int exit_code;
} RunResult;

void init_run_plan(RunPlan *plan);
void free_run_plan(RunPlan *plan);

void init_run_result(RunResult *result);
void free_run_result(RunResult *result);

#endif