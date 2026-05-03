#ifndef DB_H
#define DB_H

#include <sqlite3.h>

#include <app_context.h>

int exec_sql(
	sqlite3 *db,
  	const char *sql
	);

int db_init(
	sqlite3 *db
	);

int db_prepare(
	sqlite3 *db,
	struct AppContext *ctx
	);

int db_insert_document(
	sqlite3 *db,
	sqlite3_stmt *stmt,
	const char *title, 
	const char *abstract,
	const char *doi,
	const char *issn,
	int pub_year,
	int *out_id
	);
/*
int db_exec_step(
	sqlite3_stmt *stmt
	);

int db_query_step(
	sqlite3_stmt *stmt
	);
*/

int db_get_or_create_author(
	sqlite3 *db,
	sqlite3_stmt *stmt,
	const char *first_name,
	const char *last_name,
	const char *initial,
	int *out_id
	);
/*
int db_insert_author(
	sqlite3 *db,
	sqlite3_stmt *stmt,
	const char *first_name,
	const char *last_name,
	const char *initial);


int db_get_author_id(
	sqlite3_stmt *stmt,
	const char *first_name,
	const char *last_name,
	const char *initial
	);
*/

int db_document_x_author(
	sqlite3_stmt *stmt,
	int document_id,
	int author_id,
	int order
	);

int callback(
	void *data, 
	int argc, 
	char **argv, 
	char **colNames
	);

#endif
