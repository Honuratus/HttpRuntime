#include "dsl.h"

#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 

static char *read_file(const char *path){
    FILE* f = NULL;
    size_t file_size = 0;
    
    char* text = NULL;

    f = fopen(path, "rb");
    if(!f)
        return NULL;


    fseek(f, 0L, SEEK_END);
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    file_size = (size_t)size;
    rewind(f);


    text = malloc(file_size+1);
    if(!text)
    {
        fclose(f);
        return NULL;
    }

    
    size_t bytes_read = fread(text, 1, file_size, f);
    text[bytes_read] = '\0'; 

    fclose(f);
    return text;
}

int dsl_parse_run_plan_file(
    const char *path,
    RunPlan *plan,
    DslParseError *error)
{
    char *text = read_file(path);

    if (!text) {
        error->line = 0;
        error->column = 0;
        strcpy(error->message, "Failed to read DSL file.");
        return DSL_IO_ERROR;
    }

    int rc = dsl_parse_run_plan(text, plan, error);

    free(text);

    return rc;
}