#ifndef DSL_H
#define DSL_H

#include "dsl_parser.h" // DslParseError'ın tanımlı olduğu dosya

int dsl_parse_run_plan_file(
    const char *path,
    RunPlan *plan,
    DslParseError *error
);


#endif