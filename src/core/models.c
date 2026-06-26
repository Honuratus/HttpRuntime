#include "models.h"
#include "db_manager.h"

static void* safe_copy_string(const void* src, size_t len) {
    if(!src || len == 0) return NULL;
    char* dest = malloc(len +1);
    if(!dest) return NULL;
    memcpy(dest, src, len);
    dest[len] = '\0';
    return dest;
}

static void* safe_copy_blob(const void* src, size_t len) {
    if(!src || len == 0) return NULL;
    char* dest = malloc(len);
    if(!dest) return NULL;
    memcpy(dest, src, len);
    return dest;
}

const char* response_result_to_char(ResponseResults rr){
    switch (rr)
    {
    case RESPONSE_OK:
        return "OK";
    case RESPONSE_CURL_ERROR:
            return "CURL_ERROR";
    default:
        return "UNKNOWN";     
    }
}

Request* create_request(
    Method method,
    int collection_id,
    const char* url,
    const char* headers,
    const void* body,
    size_t body_len
){
    Request* req = calloc(1, sizeof(Request));
    if(!req) return NULL;

    req->method = method;
    req->collection_id = collection_id;
    
    req->body_len = body_len;

    if(headers)
        req->headers_len = strlen(headers);
    if(url)
        req->url_len = strlen(url);
    


    req->url = safe_copy_string(url, req->url_len);
    req->headers = safe_copy_string(headers, req->headers_len);
    req->body = safe_copy_blob(body, body_len);

    if(req->url_len > 0 && !req->url) goto failure;
    if(req->headers_len > 0 && !req->headers) goto failure;
    if(req->body_len > 0 && !req->body) goto failure;


    return req; 

failure:
    free_request(req);
    return NULL;
}


Collection* create_collection(const char* name){
    if(!name || *name == '\0')
        return NULL;

    Collection* coll = calloc(1,sizeof(Collection));
    if(!coll) return NULL;

    size_t name_len = strlen(name);

    coll->name = safe_copy_string(name, name_len);
    if(!coll->name){
        free(coll);
        return NULL;
    }

    coll->name_len = name_len;
    
    return coll;
}

WorkerTask* create_http_worker_task(Request* req)
{
    WorkerTask* task = calloc(1, sizeof(WorkerTask));

    task->type = HTTP_REQUEST;
    task->data = req;

    return task;
}

void free_request(Request* req) {
    if (!req) return; 
    
    if (req->url) free(req->url);
    if (req->headers) free(req->headers);
    if (req->body) free(req->body);
    
    free(req);
}

void free_response(Response* resp){
    if(!resp) return;
    if(resp->body) free(resp->body);
    if(resp->headers) free(resp->headers);
    if(resp->content_type) free(resp->content_type);

    free(resp);
}

void free_collection(Collection* coll){
    if(!coll) return;

    if(coll->name) free(coll->name);
    free(coll);
}