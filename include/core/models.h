#ifndef MODELS_H
#define MODELS_H

#include <stdlib.h>



typedef enum{
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS
}Method;


/*
    RECORD STRUCT FOR DATABASE ENTITY REQUEST
    if someone use that struct it's responsible to free it!
*/
typedef struct 
{
    int id;
    int collection_id;
    Method method;

    char* url; // url must be dynamic
    size_t url_len;

    char* headers;
    size_t headers_len;

    void* body;
    size_t body_len;

    int status;
}Request;


// create model structs for other 3 table

typedef struct
{
    int id;
    char* name;
    size_t name_len;
}Collection;

typedef enum{
    RESPONSE_OK,
    RESPONSE_CURL_ERROR,
    RESPONSE_INTERNAL_ERROR,
    RESPONSE_CANCELLED
}ResponseResults;

typedef struct 
{
    int id;
    int request_id;
    int status_code;

    char* headers;
    size_t headers_len;
    size_t headers_capacity;


    char* content_type;

    void* body;
    size_t body_len;
	size_t body_capacity;

    double duration;
    ResponseResults result;


    // necessary for write_callback
}Response;


typedef enum{
	HTTP_REQUEST
}WorkerOpType;

typedef struct{
	WorkerOpType type;
	void* data;
}WorkerTask;

Request* create_request(
    Method method,
    int collection_id,
    const char* url,
    const char* headers,
    const void* body,
    size_t body_len
);


Collection* create_collection(const char* name);
WorkerTask* create_http_worker_task(Request* req);

const char* response_result_to_char(ResponseResults rr);


void free_request(Request* req);
void free_collection(Collection* coll);
void free_response(Response* resp);


#endif



