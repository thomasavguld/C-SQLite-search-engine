#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

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


typedef struct {
	sqlite3_stmt *stmt_main;
	sqlite3_stmt *stmt_fts;
} AppContext;

void process_file(const char *filepath, void *userdata);

//Main function
int main() {

	// Open database connection
	sqlite3 *db;
	int rc_open = sqlite3_open(DB_PATH, &db);

	if (rc_open) {
		printf("\nCannot open database connection.\n");
		return 1;
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
		"author TEXT, "
		"abstract TEXT, "
		"doi TEXT, "
		"issn TEXT, "
		"pub_year INTEGER"
		");";

/*	const char *create_fts_sql =
		"CREATE VIRTUAL TABLE IF NOT EXISTS documents_fts USING fts5("
		"title, "
		"author, "
		"abstract, "
		"doi, "
		"issn, "
		"pub_year, "
		"content='documents', "
		"content_rowid='id' "
		");";
*/
	if (exec_sql(db, create_sql) != SQLITE_OK) return 1;	
//	if (exec_sql(db, create_fts_sql) != SQLITE_OK) return 1;	<---------------------------

	// Run ingestion
	const char *insert_sql =
		"INSERT INTO documents(title, author, abstract, doi, issn, pub_year) "
		"VALUES(?, ?, ?, ?, ?, ?);";

/*	const char *insert_fts_sql =
		"INSERT INTO documents_fts(title, author, abstract, doi, issn, pub_year) "
		"VALUES(?, ?, ?, ?, ?, ?);";
*/	
	AppContext ctx;

	sqlite3_prepare_v2(db, insert_sql, -1, &ctx.stmt_main, NULL);
//	sqlite3_prepare_v2(db, insert_fts_sql, -1, &ctx.stmt_fts, NULL); <-------------------

	sqlite3_exec(db, "BEGIN;", 0, 0 ,0);

	list_files(WAREHOUSE_PATH, process_file, &ctx);

	sqlite3_exec(db, "COMMIT;", 0, 0, 0);

	sqlite3_finalize(ctx.stmt_main);
//	sqlite3_finalize(ctx.stmt_fts);  <---------------------------

	// Select data from documents  
	const char *select_sql = "SELECT * FROM documents;";

		/* 
		"SELECT title AS Title, "
		"CASE "
		"WHEN LENGTH(abstract) > 50 "
		"THEN RTRIM(SUBSTR(abstract, 1, 180)) || '...' "
		"ELSE abstract "
		"END AS Preview "
		"FROM documents;"; */

	char *errMsg = NULL;

	sqlite3_exec(db, select_sql, callback, NULL, &errMsg);
		
	if (errMsg) {
	printf("Select error: %s\n", errMsg);
		sqlite3_free(errMsg);
	}

	// Close database connection
	sqlite3_close(db);

	return 0;

}

void process_file(const char *filepath, void *userdata) {

	AppContext *ctx = (AppContext *)userdata;

	char *json = read_file(filepath);
	if (!json) return;

	yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
	if (!doc) {
		free(json);
		return;
	}

	yyjson_val *root = yyjson_doc_get_root(doc);

	yyjson_val *abstract_v = yyjson_obj_get(root, "abstract");
	const char *abstract = yyjson_get_str(abstract_v);

	yyjson_val *meta = yyjson_obj_get(root, "metadata");
	if (!meta) {
		yyjson_doc_free(doc);
		free(json);
		return;
	}

	const char *title = yyjson_get_str(
				yyjson_obj_get(meta, "title")
			);
	const char *author = "";
	const char *doi = yyjson_get_str(
				yyjson_obj_get(meta, "doi")
			);
	const char *issn = yyjson_get_str(
				yyjson_obj_get(meta, "issn")
			);
	yyjson_val *year_v = yyjson_obj_get(meta, "pub_year");
	int pub_year = year_v ? yyjson_get_int(year_v) : 0;
	
		printf("TITLE: '%s'\n", title ? title : "NULL");
		printf("DOI : '%s'\n", doi ? doi : "NULL");
		printf("YEAR: %d\n", pub_year);

		insert_document(
			ctx->stmt_main,
			title ? title : "",
			author,
			abstract ? abstract : "",
			doi ? doi : "",
			issn ? issn : "",
			pub_year
		);

	yyjson_doc_free(doc);
	free(json);
	}
//}
