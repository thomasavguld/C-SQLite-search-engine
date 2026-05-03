#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <sqlite3.h>
#include <time.h>

typedef struct AppContext {
	sqlite3 *db;

	sqlite3_stmt *stmt_document;
	sqlite3_stmt *stmt_author;
	sqlite3_stmt *stmt_document_x_author;

	int tx_ops;
	int tx_limit;
	int tx_files_since_commit;

	int files_total;

	int files_processed;
	int insert_ok;

	int insert_errors;
	int read_errors;
	int parse_errors;

	struct timespec start_time;
	struct timespec end_time;
} AppContext;

#endif
