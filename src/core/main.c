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

    char* command = argv[1];
    
    Assertion assertion = {0};
    Assertion* assertion_ptr = NULL;
    unsigned char* owned_expected_body = NULL;
    int result = 0;

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--expect-status") == 0) {
            if (i + 1 >= argc) {
                printf("--expect-status requires a value.\n");
                return -1;
            }

            assertion.type = ASSERT_STATUS_EQ;
            assertion.expected.type = VALUE_INT;
            assertion.expected.as.int_value = atoi(argv[i + 1]);
            assertion_ptr = &assertion;

            i++;
        } 
        else if (strcmp(argv[i], "--expect-body") == 0) {
            if (i + 1 >= argc) {
                printf("--expect-body requires a value.\n");
                free(owned_expected_body);
                return -1;
            }

            const char* value = argv[i + 1];

            assertion.type = ASSERT_BODY_CONTAINS;
            assertion.expected.type = VALUE_BYTES;

            if (strncmp(value, "hex:", 4) == 0) {
                const char* hex = value + 4;

                unsigned char* bytes = NULL;
                size_t bytes_len = 0;

                if (!parse_hex_bytes(hex, &bytes, &bytes_len)) {
                    printf("Invalid hex body pattern: %s\n", value);
                    free(owned_expected_body);
                    return -1;
                }

                owned_expected_body = bytes;

                assertion.expected.as.byte_value.data = owned_expected_body;
                assertion.expected.as.byte_value.len = bytes_len;
            } else {
                assertion.expected.as.byte_value.data = (const unsigned char*)value;
                assertion.expected.as.byte_value.len = strlen(value);
            }

            assertion_ptr = &assertion;
            i++;
        }
        else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }

    // GET COMMAND
    if(strcmp("GET", command) == 0){
        result = run_get_command(argv[2],assertion_ptr);
    } else {
        printf("Unsupported command: %s\n", command);
        print_usage(argv[0]);
        result = -1;
    }


    free(owned_expected_body);
    return result;
}








