#include "assertion.h"
#include "cli_parser.h"
#include "commands.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


static void print_usage(const char* program_name){
    printf("Usage:\n");
    printf("\t%s GET <url> [options]\n", program_name);
    printf("\n");

    printf("Usage:\n");
    printf("\t--expect-status <code>\tExpect HTTP status code\n", program_name);
    printf("\n");


    printf("\nExamples:\n");
    printf("\t%s GET https://example.com\n", program_name);
    printf("\t%s GET google.com\n", program_name);
    printf("\t%s GET https://example.com --expect-status 200\n", program_name);
    printf("\n");
}



int main(int argc, char** argv){
    if(argc < 3){
        print_usage(argv[0]);
        return -1;
    }


    int result = 0;
    CliOptions options = {0};
    result = parse_cli_options(argc, argv, &options);
    if(result == -1){
        print_usage(argv[0]);
        goto cleanup;
    }

    // GET COMMAND
    if(strcmp("GET", options.command) == 0){
        result = run_get_command(&options);
    } else {
        printf("Unsupported command: %s\n", options.command);
        print_usage(argv[0]);
        result = -1;
    }

cleanup:
    free_cli_options(&options);
    return result;
}