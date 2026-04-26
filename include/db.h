#ifndef DB_H
#define DB_H

#include <sqlite3.h>


int exec_sql(
	sqlite3 *db, 
	const char *sql
	);

int db_exec_insert(
	sqlite3_stmt *stmt, 
	const char *title, 
	const char *author,
	const char *abstract,
	const char *doi,
	const char *issn,
	int pub_year
	);

int callback(
	void *data, 
	int argc, 
	char **argv, 
	char **colNames
	);

#endif
