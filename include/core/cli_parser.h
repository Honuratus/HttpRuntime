#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include "assertion.h"

#include <stdbool.h>
#include <stddef.h>

#define MAX_ASSERTION 16


typedef struct CliOptions {
    const char* command;
    const char* url;

    Assertion assertions[MAX_ASSERTION];
    size_t assertion_count;

    unsigned char* owned_expected_body[MAX_ASSERTION];
    size_t owned_body_count;
} CliOptions;

bool parse_hex_bytes(const char* hex, unsigned char** out_data, size_t* out_len);
int parse_cli_options(int argc, char** argv, CliOptions* options);
void free_cli_options(CliOptions* options);

#endif