#include <string.h>

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

// for strings
static bool check_contains(Value str, Value substr){
    if (str.type != VALUE_STRING || substr.type != VALUE_STRING)
        return false;
    
    if (!str.as.string_value || !substr.as.string_value)
            return false;
    
    return strstr(str.as.string_value, substr.as.string_value) != NULL;
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
    as_result.message = "Unknown assertion type";

    return as_result;
}