#include "commands.h"

#include "orchestrator.h"
#include "db_manager.h"


int run_get_command(const CliOptions* options){
    int rc = 0;
    int exit_code = -1;

    Runtime* r = NULL;
    Collection* coll = NULL;
    Response* resp = NULL;
    Request* req = NULL;
    WorkerTask* task = NULL;

    if (!options->url || *options->url == '\0') {
        printf("URL is required.\n");
        goto cleanup;
    }

    rc = db_init("runner.db");
    if (rc == SQLITE_ERROR) {
        printf("Database initialization error.\n");
        goto cleanup;
    }

    coll = create_collection("DEFAULT");
    if (!coll) {
        printf("Collection creation error.\n");
        goto cleanup;
    }

    rc = db_save_collection(coll);
    if (rc == SQLITE_ERROR) {
        printf("Collection db save error.\n");
        goto cleanup;
    }

    r = create_runtime(1);
    if (!r) {
        printf("Runtime creation error.\n");
        goto cleanup;
    }

    req = create_request(
        GET,
        coll->id,
        options->url,
        NULL,
        NULL,
        0
    );
    if (!req) {
        printf("Create request error.\n");
        goto cleanup;
    }


    rc = db_save_request(req);
    if (rc == SQLITE_ERROR) {
        printf("SQLITE request creation error.\n");
        goto cleanup;
    }

    int req_id = req->id;


    task = create_http_worker_task(req);
    if (!task) {
        printf("Worker task creation error.\n");
        goto cleanup;
    }

    req = NULL; // main -> orchestrator transfer

    rc = dispatch_task(r, task, WORKER);
    if (rc != ORCHESTRATOR_SUCCESS) {
        printf("Dispatch error.\n");
        goto cleanup;
    }

    task = NULL; // orchestrator -> runtime transfer

    resp = wait_for_response(req_id, 5000);
    if (!resp) {
        printf("Response timeout.\n");
        goto cleanup;
    }

    print_response_summary(resp);
    
    exit_code = 0;

    if (options->assertion_count > 0){
        printf("\nAssertions:\n");

        int all_passed = 1;

        for (size_t i = 0; i < options->assertion_count; i++){
            AssertionResult as_result = eval_assertion(resp, &options->assertions[i]);
            print_assertion_result(&as_result);

            if(!as_result.passed)
                all_passed = 0;
        }

        printf("\nAssertion Result: %s\n", all_passed ? "PASS" : "FAIL");
        exit_code = all_passed ? 0 : 1;
    }
    


cleanup:
    if(resp)
        free_response(resp);
    if(req)
        free_request(req);
    if(r)
        destroy_runtime(r);
    if(coll)
        free_collection(coll);
    
    db_close();

    return exit_code;
}
