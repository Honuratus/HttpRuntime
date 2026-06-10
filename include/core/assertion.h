#ifndef ASSERTION_H
#define ASSERTION_H

#include <stdbool.h>

#include "models.h"

typedef struct {
    const unsigned char* data;
    size_t len;
}
Bytes;

typedef enum{
    VALUE_NONE,
    VALUE_INT,
    VALUE_STRING,
    VALUE_BOOL,
    VALUE_DOUBLE,
    VALUE_BYTES
}ValueType;

typedef struct 
{
    ValueType type;
    union{
        int int_value;
        const char* string_value;
        int bool_value;
        double double_value;
        Bytes byte_value;
    } as;
}Value;


typedef enum{
    ASSERT_STATUS_EQ,
    ASSERT_BODY_CONTAINS,
}AssertionType;

typedef struct{
    AssertionType type;    
    Value expected;
}Assertion;

typedef struct {
    AssertionType type;
    bool passed;
    Value expected;
    Value actual;
    const char* message;
} AssertionResult;

AssertionResult eval_assertion(const Response* resp, const Assertion* as);

#endif