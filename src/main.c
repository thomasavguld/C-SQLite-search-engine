#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <time.h>

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
	
	int files_total;
	
	int files_processed;
	int insert_ok;

	int insert_errors;
	int read_errors;
	int parse_errors;

	struct timespec start_time;
	struct timespec end_time;
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
	AppContext ctx = {0};

	sqlite3_prepare_v2(db, insert_sql, -1, &ctx.stmt_main, NULL);
//	sqlite3_prepare_v2(db, insert_fts_sql, -1, &ctx.stmt_fts, NULL); <-------------------

	sqlite3_exec(db, "BEGIN;", 0, 0 ,0);
	
//	ctx->files_total = count_files(WAREHOUSE_PATH); <---------------------

	clock_gettime(CLOCK_MONOTONIC, &ctx.start_time);

	list_files(WAREHOUSE_PATH, process_file, &ctx);

	clock_gettime(CLOCK_MONOTONIC, &ctx.end_time);

	sqlite3_exec(db, "COMMIT;", 0, 0, 0);

	double elapsed_sec =
		(ctx.end_time.tv_sec - ctx.start_time.tv_sec) +
		(ctx.end_time.tv_nsec - ctx.start_time.tv_nsec) / 1e9;

	double files_per_sec = 0.0;
	double inserts_per_sec = 0.0;

	if (elapsed_sec >0.0) {
		files_per_sec = ctx.files_processed / elapsed_sec;
		inserts_per_sec = ctx.insert_ok / elapsed_sec;

	printf("\n --- IMPORT REPORT --- \n");
	printf("Time elapsed	: %.3f s\n", elapsed_sec);
	
	printf("Files processed	: %d\n", ctx.files_processed);
	printf("Parse errors	: %d\n", ctx.parse_errors);
	printf("Read errors	: %d\n", ctx.read_errors);
	
	printf("Inserts OK	: %d\n", ctx.insert_ok);
	printf("Insert errors	: %d\n", ctx.insert_errors);
	
	printf("Throughput:\n");
	printf("Files/sec	: %.2f\n", files_per_sec);
	printf("Inserts/sec	: %.2f\n", inserts_per_sec);

	}
	
	sqlite3_finalize(ctx.stmt_main);

	//	sqlite3_finalize(ctx.stmt_fts);  <---------------------------

/*	// Select data from documents  
	const char *select_sql = 
		"SELECT title AS Title, "
		"CASE "
		"WHEN LENGTH(abstract) > 50 "
		"THEN RTRIM(SUBSTR(abstract, 1, 180)) || '...' "
		"ELSE abstract "
		"END AS Preview "
		"FROM documents;";

	char *errMsg = NULL;

	sqlite3_exec(db, select_sql, callback, NULL, &errMsg);
		
	if (errMsg) {
	printf("Select error: %s\n", errMsg);
		sqlite3_free(errMsg);
	}
*/

	// Close database connection
	sqlite3_close(db);

	return 0;

}

void process_file(const char *filepath, void *userdata) {

	AppContext *ctx = (AppContext *)userdata;

	char *json = read_file(filepath);
	if (!json) {
		ctx->read_errors++;
		return;
	}	

	yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
	if (!doc) {
		ctx->parse_errors++;
		free(json);
		return;
	}

	ctx->files_processed++;
	
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

	int rc = db_exec_insert(
		ctx->stmt_main,
		title ? title : "",
		author,
		abstract ? abstract : "",
		doi ? doi : "",
		issn ? issn : "",
		pub_year
	);
	
	if (rc == SQLITE_OK) {
		ctx->insert_ok++;
	} else {
		ctx->insert_errors++;
	}

	yyjson_doc_free(doc);
	free(json);

/*	if (ctx->files_processed % 100 == 0) {
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);

		double elapsed = timespec_diff_sec(ctx->start_time, now);

		double rate = (elapsed > 0);
			? ctx->files_processed / elapsed
			: 0;
		
		double eta = (rate > 0)
			? (ctx->files_total - ctx->files_processed) / rate
			: 0;

		double percent = 110.0 * ctx->files_processed / ctx->files_total;

		printf("\rProcessed: %d / %d (%.1f%%) | %.1f files/s | ETA: %.1fs",
			ctx->files_processed,
			ctx->files_total,
			percent,
			rate,
			eta
		      );

	fflush(stdout);

	}

	*/

}
