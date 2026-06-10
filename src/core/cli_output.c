#include "cli_output.h"

#include <stdio.h>
#include <string.h>

#define HEADER_PREVIEW_LIMIT 1200
#define BODY_PREVIEW_LIMIT   800

static bool is_printable_bytes(const unsigned char* data, size_t len)
{
    if (!data) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        unsigned char c = data[i];

        if (c == '\n' || c == '\r' || c == '\t') {
            continue;
        }

        if (!isprint(c)) {
            return false;
        }
    }

    return true;
}

static void print_byte_value(const unsigned char* data, size_t len)
{
    if (!data) {
        printf("<null>");
        return;
    }

    if (is_printable_bytes(data, len)) {
        printf("\"");
        fwrite(data, 1, len, stdout);
        printf("\"");
        return;
    }

    printf("hex:");
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
}

static int is_text_content_type(const char* content_type) {
    if (!content_type) {
        return 0;
    }

    return strncmp(content_type, "text/", 5) == 0 ||
           strstr(content_type, "application/json") != NULL ||
           strstr(content_type, "application/xml") != NULL ||
           strstr(content_type, "application/javascript") != NULL ||
           strstr(content_type, "application/x-www-form-urlencoded") != NULL;
}

static void print_preview(const char* title, const void* data, size_t len, size_t limit) {
    if (!data || len == 0) {
        printf("%s: -\n", title ? title : "Preview");
        return;
    }

    size_t preview_len = len < limit ? len : limit;

    printf("--- %s Preview (%zu/%zu bytes) ---\n",
           title ? title : "Data",
           preview_len,
           len);

    fwrite(data, 1, preview_len, stdout);

    if (preview_len < len) {
        printf("\n... [truncated, %zu bytes remaining]", len - preview_len);
    }

    printf("\n---\n");
}

void print_assertion_result(const AssertionResult* result) {
    if (!result) {
        return;
    }

    switch (result->type) {
    case ASSERT_STATUS_EQ:
        if (result->passed) {
            printf("  ✓ status == %d\n", result->expected.as.int_value);
        } else {
            printf("  ✗ status == %d\n", result->expected.as.int_value);
            printf("    expected: %d\n", result->expected.as.int_value);
            printf("    actual:   %d\n", result->actual.as.int_value);
        }
        break;

    case ASSERT_BODY_CONTAINS: {
        const unsigned char* expected = result->expected.as.byte_value.data;
        size_t expected_len = result->expected.as.byte_value.len;

        if (result->passed) {
            printf("  ✓ body contains ");
            print_byte_value(expected, expected_len);
            printf("\n");
        } else {
            printf("  ✗ body contains ");
            print_byte_value(expected, expected_len);
            printf("\n");

            printf("    expected: ");
            print_byte_value(expected, expected_len);
            printf("\n");

            printf("    expected bytes: %zu\n", expected_len);
            printf("    actual bytes:   %zu\n", result->actual.as.byte_value.len);
        }

        break;
    }
    default:
        printf("  ✗ unknown assertion\n");
        if (result->message) {
            printf("    message: %s\n", result->message);
        }
        break;
    }
}


void print_response_summary(const Response* rep) {
    if (!rep) {
        printf("No response.\n");
        return;
    }

    printf("Response ID: %d\n", rep->id);
    printf("Request ID: %d\n", rep->request_id);
    printf("Result: %s\n", response_result_to_char(rep->result));
    printf("Status: %d\n", rep->status_code);
    printf("Duration: %.3f sec\n", rep->duration);
    printf("Content-Type: %s\n", rep->content_type ? rep->content_type : "-");

    size_t headers_len = 0;
    if (rep->headers_len > 0) {
        headers_len = rep->headers_len;
    } else if (rep->headers) {
        headers_len = strlen(rep->headers);
    }


    printf("Body: %zu bytes\n", rep->body_len);

    if (rep->headers && headers_len > 0) {
        printf("Headers: %zu bytes\n", headers_len);
        printf("\n");
        print_preview("Headers", rep->headers, headers_len, HEADER_PREVIEW_LIMIT);
    } else {
        printf("Headers: -\n");
    }


    if (!rep->body || rep->body_len == 0) {
        return;
    }

    if (!is_text_content_type(rep->content_type)) {
        printf("Body preview skipped: binary or unknown content type.\n");
        return;
    }

    print_preview("Body", rep->body, rep->body_len, BODY_PREVIEW_LIMIT);
}


