#include "cli_parser.h"

#include <stdlib.h>
#include <string.h>

static int hex_digit_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');

    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');

    return -1;
}

bool parse_hex_bytes(const char* hex, unsigned char** out_data, size_t* out_len)
{
    if (!hex || !out_data || !out_len) {
        return false;
    }

    size_t hex_len = strlen(hex);

    if (hex_len == 0 || hex_len % 2 != 0) {
        return false;
    }

    size_t byte_len = hex_len / 2;

    unsigned char* data = malloc(byte_len);
    if (!data) {
        return false;
    }

    for (size_t i = 0; i < byte_len; i++) {
        int hi = hex_digit_value(hex[i * 2]);
        int lo = hex_digit_value(hex[i * 2 + 1]);

        if (hi < 0 || lo < 0) {
            free(data);
            return false;
        }

        data[i] = (unsigned char)((hi << 4) | lo);
    }

    *out_data = data;
    *out_len = byte_len;
    return true;
}

int parse_cli_options(int argc, char** argv, CliOptions* options){
    // if argv[1] command ?? check will be implemented
    // if argv[2] url ?? check will be implemented

    if(!options || argc < 3)
        return -1;

    if(argv[1]){
        options->command = argv[1];
    }

    if(argv[2]){
        options->url = argv[2];
    }


    Assertion* assertion = NULL;
    for(int i = 3; i<argc; i++){
        if (strcmp(argv[i], "--expect-status") == 0) {
             if (i + 1 >= argc) {
                printf("--expect-status requires a value.\n");
                return -1;
            }
            if(options->assertion_count >= MAX_ASSERTION){
                printf("Too many assertions, max is %d\n", MAX_ASSERTION);
                return -1;
            }
            assertion = &options->assertions[options->assertion_count++];
            assertion->type = ASSERT_STATUS_EQ;
            assertion->expected.type = VALUE_INT;
            assertion->expected.as.int_value = atoi(argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "--expect-body") == 0) {
            if (i + 1 >= argc) {
                printf("--expect-body requires a value.\n");
                return -1;
            }

            const char* value = argv[i + 1];

            if(options->assertion_count >= MAX_ASSERTION){
                printf("Too many assertions, max is %d\n", MAX_ASSERTION);
                return -1;
            }
            assertion = &options->assertions[options->assertion_count++];
            assertion->type = ASSERT_BODY_CONTAINS;
            assertion->expected.type = VALUE_BYTES;

            if (strncmp(value, "hex:", 4) == 0) {
                const char* hex = value + 4;

                unsigned char* bytes = NULL;
                size_t bytes_len = 0;

                if (!parse_hex_bytes(hex, &bytes, &bytes_len)) {
                    printf("Invalid hex body pattern: %s\n", value);
                    return -1;
                }
                if(options->owned_body_count >= MAX_ASSERTION){
                    printf("Too many owned body patterns, max is %d\n", MAX_ASSERTION);
                    free(bytes);
                    return -1;
                }
                options->owned_expected_body[options->owned_body_count++] = bytes;

                assertion->expected.as.byte_value.data = bytes;
                assertion->expected.as.byte_value.len = bytes_len;
            } else {
                assertion->expected.as.byte_value.data = (const unsigned char*)value;
                assertion->expected.as.byte_value.len = strlen(value);
            }
            i++;
        }
        else {
            printf("Unknown option: %s\n", argv[i]);
            return -1;
        }
    }
    return 0;
}

void free_cli_options(CliOptions* options){
    for (size_t i = 0; i < options->owned_body_count; i++){
        if(options->owned_expected_body[i])
            free(options->owned_expected_body[i]);
    }
}