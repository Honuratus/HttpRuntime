#include <string.h>
#include <ctype.h>

#include "assertion.h"



static bool check_equal(Value val1, Value val2){
    if(val1.type != val2.type)
        return false;
        
    switch (val1.type)
    {

    case VALUE_INT:
        return val1.as.int_value == val2.as.int_value;    

    case VALUE_BOOL:
        return val1.as.bool_value == val2.as.bool_value;

    case VALUE_DOUBLE:
        return val1.as.double_value == val2.as.double_value;

    case VALUE_STRING:
        if (!val1.as.string_value || !val2.as.string_value)
            return false;
        return strcmp(val1.as.string_value, val2.as.string_value) == 0;
    
    case VALUE_NONE:
    default:
        return false;
    }

    return false;
}


static bool bytes_contains(Value haystack, Value needle)
{
    if (haystack.type != VALUE_BYTES || needle.type != VALUE_BYTES)
        return false;
    if (!haystack.as.byte_value.data || !needle.as.byte_value.data) {
        return false;
    }

    if (needle.as.byte_value.len == 0) {
        return true;
    }

    if (haystack.as.byte_value.len < needle.as.byte_value.len) {
        return false;
    }

    size_t len = haystack.as.byte_value.len - needle.as.byte_value.len;
    for (size_t i = 0; i <= len; i++) {
        if (memcmp(haystack.as.byte_value.data + i, needle.as.byte_value.data, needle.as.byte_value.len) == 0) {
            return true;
        }
    }

    return false;
}

static int strncasecmp_simple(const char *a, const char *b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];

        ca = (unsigned char)tolower(ca);
        cb = (unsigned char)tolower(cb);

        if (ca != cb)
            return ca - cb;

        if (ca == '\0')
            return 0;
    }

    return 0;
}

static const char *skip_spaces(const char *p)
{
    while (*p == ' ' || *p == '\t')
        p++;

    return p;
}

static const char *rtrim_end(const char *start, const char *end)
{
    while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r'))
        end--;

    return end;
}

static bool header_contains(const char *headers, HeaderExpect expected)
{
    if (!headers || !expected.name || !expected.value)
        return false;

    size_t expected_name_len = strlen(expected.name);
    size_t expected_value_len = strlen(expected.value);

    const char *line_start = headers;

    while (*line_start != '\0') {
        const char *line_end = strchr(line_start, '\n');

        if (!line_end)
            line_end = line_start + strlen(line_start);

        const char *colon = memchr(line_start, ':', (size_t)(line_end - line_start));

        if (colon) {
            const char *name_start = line_start;
            const char *name_end = rtrim_end(name_start, colon);
            size_t name_len = (size_t)(name_end - name_start);

            const char *value_start = skip_spaces(colon + 1);
            const char *value_end = rtrim_end(value_start, line_end);
            size_t value_len = (size_t)(value_end - value_start);

            bool name_matches =
                name_len == expected_name_len &&
                strncasecmp_simple(name_start, expected.name, name_len) == 0;

            bool value_matches =
                value_len == expected_value_len &&
                strncmp(value_start, expected.value, value_len) == 0;

            if (name_matches && value_matches)
                return true;
        }

        if (*line_end == '\0')
            break;

        line_start = line_end + 1;
    }

    return false;
}


AssertionResult eval_assertion(const Response* resp, const Assertion* as){
    AssertionResult as_result = {0};

    if (!resp || !as){
        as_result.passed = false;
        as_result.message = "Invalid assertion input";
        return as_result;
    }
    
    as_result.type = as->type;
    as_result.expected = as->expected;

    if (as->type == ASSERT_STATUS_EQ){    
        if (as->expected.type != VALUE_INT){
            as_result.passed = false;
            as_result.message = "ASSERT_STATUS_EQ expects VALUE_INT";
            return as_result; 
        }

        as_result.actual.type = VALUE_INT;
        as_result.actual.as.int_value = resp->status_code;

        as_result.passed = check_equal(as_result.actual, as_result.expected);

        as_result.message = as_result.passed
            ? "Status assertion passed"
            : "Status assertion failed";

        return as_result;
    }
    else if (as->type == ASSERT_BODY_CONTAINS){
        if (as->expected.type != VALUE_BYTES){
            as_result.passed = false;
            as_result.message = "ASSERT_BODY_CONTAINS expects VALUE_BYTES";
            return as_result; 
        }

        if (!resp->body || resp->body_len == 0) {
            as_result.passed = false;
            as_result.message = "Body assertion failed: empty body";
            return as_result;
        }

        as_result.actual.type = VALUE_BYTES;
        as_result.actual.as.byte_value.data = (const unsigned char*)resp->body;
        as_result.actual.as.byte_value.len = resp->body_len;

        as_result.passed = bytes_contains(as_result.actual, as_result.expected);

        as_result.message = as_result.passed
            ? "Body assertion passed"
            : "Body assertion failed";

        return as_result;
    }

    else if (as->type == ASSERT_HEADER_CONTAINS){
        if (as->expected.type != VALUE_HEADER){
            as_result.passed = false;
            as_result.message = "ASSERT_HEADER_CONTAINS expects VALUE_HEADER";
            return as_result; 
        }

        if (!resp->headers || strlen(resp->headers) == 0) {
            as_result.passed = false;
            as_result.message = "Header assertion failed: empty header";
            return as_result;
        }

        as_result.actual.type = VALUE_STRING;
        as_result.actual.as.string_value = (const char*)resp->headers;
        
        as_result.passed = header_contains(resp->headers, as_result.expected.as.header_value);

        as_result.message = as_result.passed
            ? "Header assertion passed"
            : "Header assertion failed";


        return as_result;


    }

    as_result.message = "Unknown assertion type";

    return as_result;
}