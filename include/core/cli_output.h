#ifndef CLI_OUTPUT_H
#define CLI_OUTPUT_H

#include "models.h"
#include "assertion.h"

void print_response_summary(const Response* rep);
void print_assertion_result(const AssertionResult* result);

#endif