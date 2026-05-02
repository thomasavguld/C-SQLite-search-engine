#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <sqlite3.h>
#include <time.h>

#include "author_cache.h"

typedef struct AppContext {
	sqlite3 *db;

	sqlite3_stmt *stmt_document;
	sqlite3_stmt *stmt_author;
	sqlite3_stmt *stmt_author_get;
	sqlite3_stmt *stmt_document_x_author;
	sqlite3_stmt *document_id;
	sqlite3_stmt *author_id;

	AuthorCache author_cache;

	int files_total;

	int files_processed;
	int insert_ok;

	int insert_errors;
	int read_errors;
	int parse_errors;

	int hit;
	int miss;

	struct timespec start_time;
	struct timespec end_time;
} AppContext;

#endif
