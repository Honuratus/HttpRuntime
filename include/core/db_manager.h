#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include "models.h"
#include <sqlite3.h>


int db_init(const char* db_name);

int db_save_request(Request* req);
int db_save_collection(Collection* coll);
int db_save_response(Response* resp);



Response* db_get_latest_response_by_request_id(int request_id);
int db_count_responses_by_request_id(int request_id);

Request* db_get_next_pending_request();
int db_update_request_status(int id, int status);


int db_close();
int db_clear_all();




#endif