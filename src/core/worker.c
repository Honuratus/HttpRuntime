#include <stdlib.h>
#include <stdint.h>
#include <curl/curl.h>
#include <string.h>
#include <stdbit.h>

#include "worker.h"
#include "queue.h"
#include "orchestrator.h"
#include "models.h"
#include "db_manager.h"
#include "logger.h"

#define INITIAL_CAPACITY 4096


static inline size_t max_size(size_t a, size_t b, size_t c) {
    if(a > b) return (a>c) ? a : c;
    return (b>c) ? b : c;
}


static void push_response_or_free(Runtime* o, Response** response_ptr)
{
    if (!o || !response_ptr || !*response_ptr)
        return;

    Response* response = *response_ptr;

    int rc = queue_push(o->response_queue, response);

    if (rc == QUEUE_ERROR) {
        free_response(response);
    }

    *response_ptr = NULL;
}

static bool set_curl_method(CURL* curl, Request* req);
static struct curl_slist* set_curl_headers(CURL* curl, const char* headers); 
static struct curl_slist* parse_headers(const char* h);


size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t total_size = size * nmemb;
    
    Response* r = (Response*)userdata;
    if(!r) return 0;

    size_t required_size = r->body_len + total_size; // current size + chunk size

    if(required_size > r->body_capacity){
        size_t double_cap = r->body_capacity * 2;
        size_t new_capacity =  max_size(INITIAL_CAPACITY, double_cap, required_size);

        void* temp = realloc(r->body, new_capacity);
        if(!temp){
            return 0;
        }   

        r->body = temp;
        r->body_capacity = new_capacity;
    }

    memcpy((uint8_t*)r->body + r->body_len, ptr, total_size);
    r->body_len += total_size;

    return total_size;
}

size_t header_callback(char* ptr, size_t size, size_t nmemb, void* userdata){
    size_t total_size = size * nmemb;

    // casting
    Response* r = (Response*)userdata;
    if(!r) return 0;

    size_t required_size = r->headers_len + total_size + 1;

    if(required_size > r->headers_capacity){
        size_t double_cap = r->headers_capacity * 2;
        size_t new_capacity = max_size(INITIAL_CAPACITY, double_cap ,required_size);

        void* temp = realloc(r->headers, new_capacity);
        if(!temp)
            return 0;
        

        r->headers = temp;
        r->headers_capacity = new_capacity;
    }

    memcpy(r->headers + r->headers_len, ptr, total_size);
    r->headers_len += total_size;
    r->headers[r->headers_len] = '\0';
    
    
    return total_size;
}


// take task from worker queue (Request), make http request and get a response
// push that response into response queue
void* http_worker_routine(void* arg) {
    Runtime* o = (Runtime*)arg;
    while (1) {
        
        Response* response = NULL;
        CURL* curl = NULL;
        struct curl_slist* header_list = NULL;

        WorkerTask* task = (WorkerTask*)queue_pop(o->request_queue);

        if(!task)
            break;
        
        if (task->type != HTTP_REQUEST) {
            goto cleanup_task;
        }

        // cast void* data to Request pointer
        Request* req = (Request*)task->data;
        if(!req) goto cleanup_task;

        LOG_INFO("[İŞÇİ %lu] is basinda! Hedef: %s\n", pthread_self() % 1000, req->url);


        // response allocate 
        response = (Response*)calloc(1, sizeof(Response));
        if (!response) goto cleanup_task;


        // map the response request id field
        response->request_id = req->id;


        // curl init
        curl = curl_easy_init();
        if(!curl){
            response->result = RESPONSE_CURL_ERROR;
            push_response_or_free(o, &response);
            goto cleanup_task;
        }


        // ??????
        const char* target_url = req->url;
        
        
        if(!set_curl_method(curl, req)){
            response->result = RESPONSE_CURL_ERROR;
            push_response_or_free(o, &response);
            goto cleanup_curl;
        }

        if(req->headers && *req->headers != '\0'){
            header_list =  set_curl_headers(curl ,req->headers);

            if(!header_list){
                response->result = RESPONSE_CURL_ERROR;
                push_response_or_free(o, &response);
                goto cleanup_curl;
            }
        }

        
        // curl body callback
        if(curl_easy_setopt(curl, CURLOPT_URL, target_url) != CURLE_OK){
            response->result = RESPONSE_CURL_ERROR;
            push_response_or_free(o, &response);
            goto cleanup_curl;
        }


        if(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback) != CURLE_OK){
            response->result = RESPONSE_CURL_ERROR;
            push_response_or_free(o, &response);
            goto cleanup_curl;
        }
        
        if(curl_easy_setopt(curl, CURLOPT_WRITEDATA, response) != CURLE_OK){
            response->result = RESPONSE_CURL_ERROR;
            push_response_or_free(o, &response);
            goto cleanup_curl;
        }

        // curl header callback
        if(curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback) != CURLE_OK){
            response->result = RESPONSE_CURL_ERROR;
            push_response_or_free(o, &response);
            goto cleanup_curl;
        }

        if(curl_easy_setopt(curl, CURLOPT_HEADERDATA, response) != CURLE_OK){
            response->result = RESPONSE_CURL_ERROR;
            push_response_or_free(o, &response);
            goto cleanup_curl;
        }

        // curl response content-type

        // timeout settings
        if(curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L) != CURLE_OK){
            response->result = RESPONSE_CURL_ERROR;
            push_response_or_free(o, &response);
            goto cleanup_curl;
        }

        if(curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L) != CURLE_OK){
            response->result = RESPONSE_CURL_ERROR;
            push_response_or_free(o, &response);
            goto cleanup_curl;
        }
        
        // follow redirections
        if(curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L) != CURLE_OK){
            response->result = RESPONSE_CURL_ERROR;
            push_response_or_free(o, &response);
            goto cleanup_curl;
        }

        // limit redirect count
        if(curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L) != CURLE_OK){
            response->result = RESPONSE_CURL_ERROR;
            push_response_or_free(o, &response);
            goto cleanup_curl;
        }


        // agent set
        if(curl_easy_setopt(curl, CURLOPT_USERAGENT, "MyCWorker/1.0") != CURLE_OK){
            response->result = RESPONSE_CURL_ERROR;
            push_response_or_free(o, &response);
            goto cleanup_curl;
        }
        



        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            response->result = RESPONSE_CURL_ERROR;
        }
        else{
            response->result = RESPONSE_OK;
            
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            response->status_code = (int)http_code;


            char* content_type = NULL;
            if (curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type) == CURLE_OK) {
               if (content_type) {
                    response->content_type = strdup(content_type);
                }
            }

            double total_time = 0.0;
            if (curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time) == CURLE_OK){
                response->duration = total_time;
            }

        }
        
        push_response_or_free(o, &response);
        
        
cleanup_header_list:
        if(header_list)
            curl_slist_free_all(header_list);

cleanup_curl:
    if(curl)
        curl_easy_cleanup(curl); 

cleanup_response:
    if (response)
        free_response(response);

cleanup_task:
        if(task){
            if(task->data) free_request(task->data);  // 100% http_request olmak zorunda
            free(task);
        }
        
    }
    return NULL;
}



// make usefull of this db_worker_routine
// make it part of the system

void* db_worker_routine(void* arg) {
    

    LOG_INFO("[DB Worker] Started and listening to db_queue...\n");


    Runtime* o = (Runtime*)arg;
    while(1){
        DBTask* task = (DBTask*)queue_pop(o->db_queue);
        if(!task) break;

        switch (task->type) {
            case DB_TASK_SAVE_REQUEST: {
                Request* req = (Request*)task->data;
                db_save_request(req);
                free_request(req); 
                break;
            }
            case DB_TASK_SAVE_RESPONSE: {
                Response* resp = (Response*)task->data;
                db_save_response(resp);
                free_response(resp);
                break;
            }
            case DB_TASK_SAVE_COLLECTION: {
                Collection* coll = (Collection*)task->data;
                db_save_collection(coll);
                free_collection(coll);
                break;
            }
            case DB_TASK_SHUTDOWN: {
                LOG_INFO("[DB Worker] Shutdown signal received. Exiting...\n");
                free(task);
                return NULL; 
            }

            case DB_TASK_GET_NEXT_REQUEST: {
                // DB'den sıradaki işi çek
                Request* pending_req = db_get_next_pending_request();
                
                // Eğer bu işi isteyen birisi (callback) varsa ona veriyi yolla
                if (task->callback) {
                    task->callback(pending_req, task->context);
                } else if (pending_req) {
                    // İsteyen yoksa boşa memory leak olmasın
                    free_request(pending_req); 
                }
                break;
            }

            case DB_TASK_UPDATE_STATUS: {
                UpdateStatusPayload* payload = (UpdateStatusPayload*)task->data;
                db_update_request_status(payload->request_id, payload->new_status_code);
                free(payload); 
                break;
            }

            default:
                LOG_ERROR("[DB Worker] Unknown task type!\n");
                break;
        }

        free(task);
    }
    return NULL;
}

static bool set_curl_method(CURL* curl, Request* req){
    int rc = CURLE_OK;
    if (!curl || !req) return false; // Güvenlik kontrolü

    switch (req->method)
    {
    case GET:
        rc = curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        if(rc != CURLE_OK) return false;
        break;

    case POST:
        rc = curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if(rc != CURLE_OK) return false;

        if(req->body){
            rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)req->body);
            if(rc != CURLE_OK) return false;

            rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req->body_len);
            if(rc != CURLE_OK) return false;
        }
        break;

    case DELETE:
        rc = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        if(rc != CURLE_OK) return false;

        if(req->body){
            rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)req->body);
            if(rc != CURLE_OK) return false;

            rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req->body_len);
            if(rc != CURLE_OK) return false;
        }
        break;

    case PUT:
        rc = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        if(rc != CURLE_OK) return false;

        if(req->body){
            rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)req->body);
            if(rc != CURLE_OK) return false;

            rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req->body_len);
            if(rc != CURLE_OK) return false;
        }
        break;

    

    case PATCH:
        rc = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        if(rc != CURLE_OK) return false;

        if(req->body){
            rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)req->body);
            if(rc != CURLE_OK) return false;

            rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req->body_len);
            if(rc != CURLE_OK) return false;
        }
        break;

    default:
        return false;
    }
    return true;
}

static struct curl_slist* set_curl_headers(CURL* curl, const char* headers){
    if(!headers)
        return NULL;
    struct curl_slist* header_list = parse_headers(headers);
    if(!header_list)
        return NULL;

    if(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list) != CURLE_OK){
        curl_slist_free_all(header_list);
        return NULL;
    }
        
    return header_list;
}

static struct curl_slist* parse_headers(const char* h){
    char* headers = strdup(h); // stringi kopyala

    if(!headers)
        return NULL;
    struct curl_slist *list = NULL;
    char* saveptr;

    char *line = strtok_r(headers, "\r\n", &saveptr);

    while(line){
        if(*line != '\0'){
            struct curl_slist* temp = curl_slist_append(list,line);
            if(!temp){
                curl_slist_free_all(list);
                free(headers);
                return NULL;
            }
            list = temp;
        }

        line = strtok_r(NULL, "\r\n", &saveptr);
    }


    free(headers);
    return list;
}





