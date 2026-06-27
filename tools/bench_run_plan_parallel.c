#define _POSIX_C_SOURCE 200809L


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "engine.h"
#include "run_plan.h"
#include "models.h"
#include "assertion.h"

extern char *strdup(const char *s);

static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static int fill_plan(RunPlan *plan, size_t request_count, size_t worker_count)
{
    init_run_plan(plan);

    plan->worker_count = worker_count;
    plan->default_timeout_ms = 10000;
    plan->save_history = true;
    plan->db_path = strdup("bench_runner.db");
    plan->collection_name = strdup("BENCH");
    plan->case_count = request_count;

    if (!plan->db_path || !plan->collection_name) {
        return -1;
    }

    for (size_t i = 0; i < request_count; i++) {
        RequestCase *c = &plan->cases[i];

        c->method = GET;
        c->url = strdup("http://127.0.0.1:9090/delay");
        c->headers = NULL;
        c->body = NULL;
        c->body_len = 0;
        c->timeout_ms = 10000;

        if (!c->url) {
            return -1;
        }

        c->assertion_count = 1;
        c->assertions[0].type = ASSERT_STATUS_EQ;
        c->assertions[0].expected.type = VALUE_INT;
        c->assertions[0].expected.as.int_value = 200;
    }

    return 0;
}

static int run_bench(size_t request_count, size_t worker_count)
{
    RunPlan plan;
    RunResult result;

    init_run_plan(&plan);
    init_run_result(&result);

    if (fill_plan(&plan, request_count, worker_count) != 0) {
        fprintf(stderr, "failed to fill plan\n");
        free_run_plan(&plan);
        return 1;
    }

    double start = now_seconds();

    int rc = run_plan(&plan, &result);

    double end = now_seconds();
    double elapsed = end - start;

    printf("requests=%zu workers=%zu elapsed=%.3f sec rc=%d all_req=%d all_assert=%d\n",
           request_count,
           worker_count,
           elapsed,
           rc,
           result.all_requests_ok,
           result.all_assertions_ok);

    free_run_result(&result);
    free_run_plan(&plan);

    return rc;
}

int main(int argc,char** argv)
{
    if (argc < 3) return 0;
    size_t request_count = atoi(argv[1]);

    printf("RunPlan parallel benchmark\n");
    printf("Each request sleeps ~200ms on local threaded server\n\n");

    run_bench(request_count, atoi(argv[2]));

    return 0;
}