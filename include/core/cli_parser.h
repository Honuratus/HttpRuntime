#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include "assertion.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct CliOptions {
    const char* command;
    const char* url;

    Assertion assertion;
    Assertion* assertion_ptr;

    unsigned char* owned_expected_body;
} CliOptions;

bool parse_hex_bytes(const char* hex, unsigned char** out_data, size_t* out_len);

int parse_cli_options(int argc, char** argv, CliOptions* options);
void free_cli_options(CliOptions* options);

#endif