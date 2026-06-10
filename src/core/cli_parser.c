#include "cli_parser.h"

#include <stdlib.h>

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