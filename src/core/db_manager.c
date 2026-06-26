#include <stdio.h>
#include <string.h>
#include "db_manager.h"
#include "logger.h"

sqlite3 *db = NULL;

int db_init(const char* db_name){
    int rc;

    rc = sqlite3_open_v2(db_name,&db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if(rc != SQLITE_OK){
        fprintf(stderr ,"Database could not opened: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    char* err_msg = 0;
    rc = sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA foreign_keys=ON;", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "WAL mode error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    const char *sql = 
        "CREATE TABLE IF NOT EXISTS collections ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL, "
        "name_len INTEGER NOT NULL,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");";
    
    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Table creation error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    sql = 
        "CREATE TABLE IF NOT EXISTS requests ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "collection_id INTEGER,"
        "method INTEGER,"
        "url TEXT NOT NULL,"
        "url_len INTEGER,"
        "headers TEXT,"
        "headers_len INTEGER,"
        "body BLOB,"
        "body_len INTEGER,"
        "status INTEGER DEFAULT 0,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "CONSTRAINT fk_collection FOREIGN KEY(collection_id) REFERENCES collections(id)"
        ");";

    
    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Table creation error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }
    
    // create other 3 table collection, responses, enviroments


    sql = 
        "CREATE TABLE IF NOT EXISTS responses ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "request_id INTEGER,"
        "status_code INTEGER,"
        "result INTEGER,"
        "duration REAL,"
        "headers TEXT,"
        "body BLOB,"
        "body_len INTEGER,"
        "content_type TEXT,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "CONSTRAINT fk_request FOREIGN KEY(request_id) REFERENCES requests(id)"
        ");";

    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Table creation error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    LOG_INFO("[Info] DB and Tables are ready.\n");
    return SQLITE_OK;
}

int db_save_collection(Collection* coll){
    if(!coll) return SQLITE_ERROR;

    sqlite3_stmt* stmt = NULL;
    const char* sql =
        "INSERT INTO collections ("
        "name, name_len)"
        "VALUES(?,?);";
    
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "sqlite3_prepare_v2 error: %s\n", sqlite3_errmsg(db));
        return SQLITE_ERROR;
    }

    if (sqlite3_bind_text(stmt, 1, coll->name, coll->name_len, SQLITE_STATIC) != SQLITE_OK) goto failure;
    if (sqlite3_bind_int(stmt, 2, coll->name_len) != SQLITE_OK) goto failure;

    if (sqlite3_step(stmt) != SQLITE_DONE) goto failure;

    coll->id = (int)sqlite3_last_insert_rowid(db);
    LOG_INFO("[Info] Collection saved with ID: %d\n", coll->id);

    sqlite3_finalize(stmt);
    return SQLITE_OK;


failure:
    fprintf(stderr, "[SQL ERROR] Save failed: %s\n", sqlite3_errmsg(db));
    if (stmt) sqlite3_finalize(stmt); 
    return SQLITE_ERROR;
}

int db_save_response(Response* resp){
    if(!resp) return SQLITE_ERROR;

    sqlite3_stmt* stmt = NULL;

    const char* sql = 
        "INSERT INTO responses ("
        "request_id, status_code, result, duration ,headers, content_type, body, body_len)"
        "VALUES(?,?,?,?,?,?,?,?);";
    
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "sqlite3_prepare_v2 error: %s\n", sqlite3_errmsg(db));
        return SQLITE_ERROR;
    }

    if (sqlite3_bind_int(stmt, 1, resp->request_id) != SQLITE_OK) goto failure;
    if (sqlite3_bind_int(stmt, 2, resp->status_code) != SQLITE_OK) goto failure;
    if (sqlite3_bind_int(stmt, 3, resp->result) != SQLITE_OK) goto failure;
    if (sqlite3_bind_double(stmt, 4, resp->duration) != SQLITE_OK) goto failure;

    
    if(resp->headers){
        if (sqlite3_bind_text(stmt, 5, resp->headers, strlen(resp->headers), SQLITE_STATIC) != SQLITE_OK)
            goto failure;
    }
    else{
        if (sqlite3_bind_null(stmt, 5) != SQLITE_OK) 
            goto failure;
    }


    
    if(resp->content_type){
        if (sqlite3_bind_text(stmt, 6, resp->content_type, strlen(resp->content_type), SQLITE_STATIC) != SQLITE_OK) 
            goto failure;
    }
    else{
        if (sqlite3_bind_null(stmt, 6) != SQLITE_OK) 
            goto failure;
    }


    
    if(resp->body && resp->body_len > 0){
        if (sqlite3_bind_blob(stmt, 7, resp->body, resp->body_len, SQLITE_STATIC) != SQLITE_OK)
            goto failure;
    }
    else{
        if (sqlite3_bind_null(stmt, 7) != SQLITE_OK) 
            goto failure;
    }

    
    
    if (sqlite3_bind_int64(stmt, 8, (sqlite3_int64)resp->body_len) != SQLITE_OK) goto failure;

    

    if (sqlite3_step(stmt) != SQLITE_DONE) goto failure;

    resp->id = (int)sqlite3_last_insert_rowid(db);
    LOG_INFO("[Info] Response saved with ID: %d\n", resp->id);

    sqlite3_finalize(stmt);
    return SQLITE_OK;

failure:
    fprintf(stderr, "[SQL ERROR] Save failed: %s\n", sqlite3_errmsg(db));
    if (stmt) sqlite3_finalize(stmt); 
    return SQLITE_ERROR;
}


int db_save_request(Request* req){
    if(!req) return SQLITE_ERROR;

    sqlite3_stmt* stmt = NULL;
    const char* sql = 
        "INSERT INTO requests ("
        "collection_id,method,url,url_len,"
        "headers,headers_len,body,body_len)"
        "VALUES(?,?,?,?,?,?,?,?)";
    
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "sqlite3_prepare_v2 error: %s\n", sqlite3_errmsg(db));
        return SQLITE_ERROR;
    }
    

    if (sqlite3_bind_int(stmt, 1, req->collection_id) != SQLITE_OK) goto failure;
    if (sqlite3_bind_int(stmt, 2, req->method) != SQLITE_OK) goto failure;
    if (sqlite3_bind_text(stmt, 3, req->url, req->url_len, SQLITE_STATIC) != SQLITE_OK) goto failure;
    if (sqlite3_bind_int64(stmt, 4, req->url_len) != SQLITE_OK) goto failure;
    if (sqlite3_bind_text(stmt, 5, req->headers, req->headers_len, SQLITE_STATIC) != SQLITE_OK) goto failure;
    if (sqlite3_bind_int64(stmt, 6, req->headers_len) != SQLITE_OK) goto failure;
    if (sqlite3_bind_blob(stmt, 7, req->body, req->body_len, SQLITE_STATIC) != SQLITE_OK) goto failure;
    if (sqlite3_bind_int64(stmt, 8, req->body_len) != SQLITE_OK) goto failure;

    if (sqlite3_step(stmt) != SQLITE_DONE) goto failure;

    req->id = (int)sqlite3_last_insert_rowid(db);
    LOG_INFO("[Info] Request saved with ID: %d\n", req->id);

    sqlite3_finalize(stmt);
    return SQLITE_OK;

failure:
    fprintf(stderr, "[SQL ERROR] Save failed: %s\n", sqlite3_errmsg(db));
    if (stmt) sqlite3_finalize(stmt); 
    return SQLITE_ERROR;
}

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

static Request* map_row_to_request(sqlite3_stmt* stmt) {
    Request* req = calloc(1, sizeof(Request));
    if(!req) return NULL;

    req->id = sqlite3_column_int(stmt, 0);
    req->collection_id = sqlite3_column_int(stmt, 1);
    req->method = sqlite3_column_int(stmt, 2);
    req->status = sqlite3_column_int(stmt,9);

    req->url_len = sqlite3_column_int64(stmt, 4);
    req->headers_len = sqlite3_column_int64(stmt, 6);
    req->body_len = sqlite3_column_int64(stmt, 8);



    const char* db_url = sqlite3_column_text(stmt,3);
    const char* db_headers = sqlite3_column_text(stmt,5);
    const char* db_body = sqlite3_column_blob(stmt,7);

    req->url = (char*)safe_copy_string(db_url, req->url_len);
    req->headers = (char*)safe_copy_string(db_headers, req->headers_len);
    req->body =  safe_copy_blob(db_body, req->body_len);

    if (!req->url || req->url_len == 0) {
        fprintf(stderr, "[Critical] Request URL is missing or malloc failed!\n");
        free_request(req);
        return NULL;
    }
    
    if (req->headers_len > 0 && !req->headers) {
        fprintf(stderr, "[Critical] Malloc failed for Headers!\n");
        free_request(req);
        return NULL;
    }

    if (req->body_len > 0 && !req->body) {
        fprintf(stderr, "[Critical] Malloc failed for Body!\n");
        free_request(req);
        return NULL;
    }

    return req;
}

static Response* map_row_to_response(sqlite3_stmt* stmt){
    Response* resp = calloc(1, sizeof(Response));
    if(!resp) return NULL;

    resp->id = sqlite3_column_int(stmt, 0);
    resp->request_id = sqlite3_column_int(stmt, 1);
    resp->status_code = sqlite3_column_int(stmt, 2);
    resp->result = sqlite3_column_int(stmt,8);

    resp->duration = sqlite3_column_double(stmt, 7);

    
    
    resp->body_len = sqlite3_column_bytes(stmt, 5);



    const char* db_headers = sqlite3_column_text(stmt,3);
    const char* db_content_type = sqlite3_column_text(stmt,4);
    const void* db_body = sqlite3_column_blob(stmt,5);

    size_t headers_len = sqlite3_column_bytes(stmt, 3);
    size_t content_type_len = sqlite3_column_bytes(stmt, 4);

    resp->content_type = (char*)safe_copy_string(db_content_type, content_type_len);
    resp->headers = (char*)safe_copy_string(db_headers, headers_len);
    resp->body =  safe_copy_blob(db_body, resp->body_len);

    
    if (content_type_len > 0 && !resp->content_type) {
        fprintf(stderr, "[Critical] Malloc failed for Content-Type!\n");
        free_response(resp);
        return NULL;
    }

    if (headers_len > 0 && !resp->headers) {
        fprintf(stderr, "[Critical] Malloc failed for Headers!\n");
        free_response(resp);
        return NULL;
    }

    if (resp->body_len > 0 && !resp->body) {
        fprintf(stderr, "[Critical] Malloc failed for Body!\n");
        free_response(resp);
        return NULL;
    }

    return resp;
}






Request* db_get_next_pending_request(){
    sqlite3_stmt* stmt = NULL;
    Request* req = NULL;


    if (sqlite3_exec(db, "BEGIN IMMEDIATE;", NULL, NULL, NULL) != SQLITE_OK)
        return NULL;    


    const char* sql = 
        "SELECT id,collection_id,method,url,url_len,"
        "headers,headers_len,body,body_len,status "
        "FROM requests "
        "WHERE status = 0 "
        "ORDER BY created_at ASC "
        "LIMIT 1;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
        goto failure;    


    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        return NULL;
    }


    
    req = map_row_to_request(stmt);
    if(!req) goto failure;

    sqlite3_finalize(stmt);
    stmt = NULL;

    const char* sql_update = "UPDATE requests SET status = 1 WHERE id = ?;";
    
    if (sqlite3_prepare_v2(db, sql_update, -1, &stmt, NULL) != SQLITE_OK) goto failure;
    
    if(sqlite3_bind_int(stmt, 1, req->id) != SQLITE_OK)
        goto failure;
    
    if (sqlite3_step(stmt) != SQLITE_DONE) goto failure; 
    
    sqlite3_finalize(stmt);
    stmt = NULL;

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    return req;
failure:
    sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    if (stmt) sqlite3_finalize(stmt);
    if (req) free_request(req);
    return NULL;
}   


int db_update_request_status(int id, int status){
    sqlite3_stmt* stmt = NULL;
    const char* sql = "UPDATE requests SET status = ? WHERE id = ?;";
    

    if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) 
        goto failure;
    
    if(sqlite3_bind_int(stmt, 1,status) != SQLITE_OK)
        goto failure;

    if(sqlite3_bind_int(stmt, 2, id) != SQLITE_OK)
        goto failure;

    if(sqlite3_step(stmt) != SQLITE_DONE)
        goto failure;
    
    sqlite3_finalize(stmt);
    stmt = NULL;
    return SQLITE_OK;


failure:
    fprintf(stderr, "[SQL ERROR] Update request status failed: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return SQLITE_ERROR;
}


Response*  db_get_latest_response_by_request_id(int request_id){
    if(!db) return NULL;

    sqlite3_stmt* stmt = NULL;
    Response* resp = NULL;

    const char* sql = 
        "SELECT id, request_id, status_code, "
        "headers, content_type, body, body_len, duration, result "
        "FROM responses "
        "WHERE request_id = ? "
        "ORDER BY id DESC "
        "LIMIT 1;";

    if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
        goto failure;

    if(sqlite3_bind_int(stmt,1,request_id) != SQLITE_OK)
        goto failure;

    
    if (sqlite3_step(stmt) != SQLITE_ROW){
        sqlite3_finalize(stmt);
        return NULL;
    }

    resp = map_row_to_response(stmt);
    if(!resp) goto failure;


    sqlite3_finalize(stmt);
    return resp;


failure:
    fprintf(stderr, "[SQL ERROR] Get latest response by request_id failed: %s\n", sqlite3_errmsg(db));
    
    if(stmt)
        sqlite3_finalize(stmt);

    return NULL;
}

int db_count_responses_by_request_id(int request_id){
    if(!db) return SQLITE_ERROR;

    sqlite3_stmt* stmt = NULL;
    int count = 0;
    
    const char* sql = 
        "SELECT COUNT(*) FROM responses "
        "WHERE request_id = ?;";

    if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
        goto failure;

    if(sqlite3_bind_int(stmt,1,request_id) != SQLITE_OK)
        goto failure;

    if (sqlite3_step(stmt) != SQLITE_ROW)
        goto failure;

    count = sqlite3_column_int(stmt, 0);
    
    sqlite3_finalize(stmt);
    return count;


failure:
    fprintf(stderr, "[SQL ERROR] Get response count by request_id failed: %s\n", sqlite3_errmsg(db));
    
    if(stmt)
        sqlite3_finalize(stmt);

    return SQLITE_ERROR;
    
}

int db_count_all_responses(){
    if(!db) return SQLITE_ERROR;

    sqlite3_stmt* stmt = NULL;
    int count = 0;

    const char* sql = 
        "SELECT COUNT(*) FROM responses;";
    

    if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
        goto failure;

    if (sqlite3_step(stmt) != SQLITE_ROW)
        goto failure;
        
    count = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return count;
    
failure:
    fprintf(stderr, "[SQL ERROR] Get response count failed: %s\n", sqlite3_errmsg(db));
    
    if(stmt)
        sqlite3_finalize(stmt);

    return SQLITE_ERROR;
}

int db_close(){
    if(!db) return SQLITE_OK;
    if(sqlite3_close_v2(db) != SQLITE_OK){
        fprintf(stderr, "[SQL ERROR] Database couldn't closed: %s\n", sqlite3_errmsg(db));
        return SQLITE_ERROR;
    }
    db = NULL;
    return SQLITE_OK;
}


int db_clear_all()
{
    if (!db) return SQLITE_ERROR;
    sqlite3_exec(db, "DELETE FROM responses;", NULL, NULL, NULL);
    sqlite3_exec(db, "DELETE FROM requests;", NULL, NULL, NULL);
    sqlite3_exec(db, "DELETE FROM collections;", NULL, NULL, NULL);
    return SQLITE_OK;
}



// create other required crud functions
