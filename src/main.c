#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <dirent.h>

#include "yyjson.h"
#include "db.h"
#include "fs.h"
#include "json.h"

// define db & warehouse paths
#ifndef DB_PATH
#define DB_PATH "./db/c_search.db"
#endif

#ifndef WAREHOUSE_PATH
#define WAREHOUSE_PATH "./warehouse"
#endif


//Main function

int main() {

	// Open database connection
	
	sqlite3 *db;
	char *errMsg = 0;

	int rc_open = sqlite3_open(DB_PATH, &db);

	if (rc_open) {
		printf("\nCannot open database connection.\n");
		return 1;
	} else {
		printf("\nDatabase connection open.\n");
	}

	// SQLite PRAGMA settings for bulk ingestion
	
	sqlite3_exec(db, "PRAGMA journal_mode=WAL;", 0, 0, 0);
	sqlite3_exec(db, "PRAGMA synchronous=OFF;", 0, 0, 0);
	sqlite3_exec(db, "PRAGMA cache_size=100000 ;", 0, 0, 0);

	// Create tables if they don't already exist

	const char *create_sql =
		"CREATE TABLE IF NOT EXISTS documents ("
		"id INTEGER PRIMARY KEY,"
		"title TEXT,"
		"abstract TEXT"
		");";

	const char *create_fts_sql =
		"CREATE VIRTUAL TABLE IF NOT EXISTS documents_fts USING fts5("
		"title,"
		"abstract, "
		"content='documents', "
		"content_rowid='id' "
		");";

	if (exec_sql(db, create_sql) != SQLITE_OK) return 1;	
	if (exec_sql(db, create_fts_sql) != SQLITE_OK) return 1;	

	printf("\nDatabase ready.\n\n");


	// Run ingestion
	
	const char *insert_sql =
		"INSERT INTO documents(title, abstract) "
		"VALUES(?, ?);";

	const char *insert_fts_sql =
		"INSERT INTO documents_fts(title, abstract) "
		"VALUES(?, ?);";

	sqlite3_stmt *stmt_main;
	sqlite3_stmt *stmt_fts;
	
	sqlite3_prepare_v2(db, insert_sql, -1, &stmt_main, NULL);
	sqlite3_prepare_v2(db, insert_fts_sql, -1, &stmt_fts, NULL);

	sqlite3_exec(db, "BEGIN;", 0, 0 ,0);

	list_files(WAREHOUSE_PATH, db, stmt_main, stmt_fts);

	sqlite3_exec(db, "COMMIT;", 0, 0, 0);

	sqlite3_finalize(stmt_main);
	sqlite3_finalize(stmt_fts);
	

	// Select data from documents  

	const char *select_sql = 
		"SELECT title AS Title, "
		"CASE "
		"WHEN LENGTH(abstract) > 50 "
		"THEN RTRIM(SUBSTR(abstract, 1, 80)) || '...' "
		"ELSE abstract "
		"END AS Preview "
		"FROM documents;"; 

	int rc_select = sqlite3_exec(db, select_sql, callback, 0, &errMsg);

	if (rc_select != SQLITE_OK) {
		printf("Select error: %s\n", errMsg);
		sqlite3_free(errMsg);
	}
		// Close database connection

	sqlite3_close(db);

	return 0;

}


