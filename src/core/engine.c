#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "engine.h"
#include "db_manager.h"
#include "orchestrator.h"
#include "models.h"
#include "assertion.h"

int run_plan(const RunPlan *plan, RunResult *result)
{
    int rc = 0;
    int exit_code = 1;

    Collection *coll = NULL;
    Runtime *runtime = NULL;
    Request *req = NULL;
    WorkerTask *task = NULL;

    int request_ids[MAX_REQUEST_CASES];

    if (!plan || !result) {
        return 1;
    }

    init_run_result(result);

    for (size_t i = 0; i < MAX_REQUEST_CASES; i++) {
        request_ids[i] = -1;
    }

    if (plan->case_count > MAX_REQUEST_CASES) {
        goto cleanup;
    }

    const char *db_path = plan->db_path ? plan->db_path : "runner.db";
    const char *collection_name = plan->collection_name ? plan->collection_name : "DEFAULT";

    rc = db_init(db_path);
    if (rc == SQLITE_ERROR) {
        goto cleanup;
    }

    coll = create_collection(collection_name);
    if (!coll) {
        goto cleanup;
    }

    rc = db_save_collection(coll);
    if (rc == SQLITE_ERROR) {
        goto cleanup;
    }

    size_t worker_count = plan->worker_count > 0 ? plan->worker_count : 1;

    runtime = create_runtime(worker_count);
    if (!runtime) {
        goto cleanup;
    }

    result->all_requests_ok = true;
    result->all_assertions_ok = true;
    result->case_result_count = plan->case_count;

    //
    // Create all requests and dispatch all tasks
    for (size_t i = 0; i < plan->case_count; i++) {
        const RequestCase *request_case = &plan->cases[i];

        req = create_request(
            request_case->method,
            coll->id,
            request_case->url,
            request_case->headers,
            request_case->body,
            request_case->body_len
        );

        if (!req) {
            result->all_requests_ok = false;
            goto cleanup;
        }

        rc = db_save_request(req);
        if (rc == SQLITE_ERROR) {
            result->all_requests_ok = false;
            goto cleanup;
        }

        request_ids[i] = req->id;

        task = create_http_worker_task(req);
        if (!task) {
            result->all_requests_ok = false;
            goto cleanup;
        }

        
        // request ownership transfer to worker

        req = NULL;

        rc = dispatch_task(runtime, task, WORKER);
        if (rc != ORCHESTRATOR_SUCCESS) {
            result->all_requests_ok = false;
            goto cleanup;
        }

        // task ownership transfer to queue/runtime
        task = NULL;
    }


     
    // wait for responses
    for (size_t i = 0; i < plan->case_count; i++) {
        const RequestCase *request_case = &plan->cases[i];
        RequestCaseResult *case_result = &result->case_results[i];

        int timeout_ms = request_case->timeout_ms > 0
            ? request_case->timeout_ms
            : plan->default_timeout_ms;

        if (timeout_ms <= 0) {
            timeout_ms = 5000;
        }

        Response *resp = wait_for_response(request_ids[i], timeout_ms);
        if (!resp) {
            case_result->request_ok = false;
            case_result->assertions_ok = false;

            result->all_requests_ok = false;
            result->all_assertions_ok = false;

            continue;
        }

        
        // response ownership is transferred to RunResult
        case_result->response = resp;
        case_result->request_ok = true;
        case_result->assertions_ok = true;
        case_result->assertion_result_count = 0;

        for (size_t j = 0; j < request_case->assertion_count; j++) {
            if (case_result->assertion_result_count >= MAX_CASE_ASSERTIONS) {
                case_result->assertions_ok = false;
                result->all_assertions_ok = false;
                break;
            }

            AssertionResult as_result = eval_assertion(
                resp,
                &request_case->assertions[j]
            );

            case_result->assertion_results[case_result->assertion_result_count++] = as_result;

            if (!as_result.passed) {
                case_result->assertions_ok = false;
                result->all_assertions_ok = false;
            }
        }
    }

    exit_code = (result->all_requests_ok && result->all_assertions_ok) ? 0 : 1;

cleanup:
    if (task) {
        free(task);
    }

    if (req) {
        free_request(req);
    }

    if (runtime) {
        destroy_runtime(runtime);
    }

    if (coll) {
        free_collection(coll);
    }

    db_close();

    result->exit_code = exit_code;

    return exit_code;
}