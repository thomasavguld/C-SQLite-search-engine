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

void process_file(const char *filepath, void *userdata);

static void batch_wrapper(const char *filepath, void *userdata)
{
	AppContext *ctx = (AppContext *)userdata;

	process_file(filepath, userdata);

	static int ops = 0;
	ops++;

	if (ops % 1000 == 0) {
		printf("OPS: %d files=%d\n", ops, ctx->files_processed);
	}

	ctx->tx_files_since_commit++;

	if (ctx->tx_files_since_commit >= ctx->tx_limit) {

		int rc;
		
		rc = sqlite3_exec(ctx->db, "COMMIT;", NULL, NULL, NULL);
		if (rc != SQLITE_OK) {
				fprintf(stderr, "COMMIT failed: %s\n", sqlite3_errmsg(ctx->db));
		}

		rc = sqlite3_exec(ctx->db, "BEGIN;", NULL, NULL, NULL);
		if (rc != SQLITE_OK) {
				fprintf(stderr, "BEGIN failed: %s\n", sqlite3_errmsg(ctx->db));
		}

		printf("TX boundary at file %d\n", ctx->files_processed);

		ctx->tx_files_since_commit = 0;
		}
}

//Main function
int main() {

	// Open database connection
	sqlite3 *db;
	int rc_open = sqlite3_open(DB_PATH, &db);

	if (rc_open) {
		printf("\nCannot open database connection.\n");
		return 1;
	}

	AppContext ctx = {0};

	ctx.tx_files_since_commit = 0;
	ctx.tx_limit = 200;
		
	ctx.db = db;

	db_init(db);
	db_prepare(db, &ctx);

	sqlite3_exec(db, "BEGIN;", 0, 0 ,0);
	
	clock_gettime(CLOCK_MONOTONIC, &ctx.start_time);

	list_files(WAREHOUSE_PATH, batch_wrapper, &ctx);

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
	printf("Insert errors	: %d\n\n", ctx.insert_errors);
	
	printf("Throughput:\n");
	printf("Files/sec	: %.2f\n", files_per_sec);
	printf("Inserts/sec	: %.2f\n", inserts_per_sec);

	}
	
	sqlite3_finalize(ctx.stmt_document);
	sqlite3_finalize(ctx.stmt_author);
	sqlite3_finalize(ctx.stmt_document_x_author);

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

//	printf("Process: %s\n", filepath); 

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
	const char *doi = yyjson_get_str(
				yyjson_obj_get(meta, "doi")
			);
	const char *issn = yyjson_get_str(
				yyjson_obj_get(meta, "issn")
			);
	yyjson_val *year_v = yyjson_obj_get(meta, "pub_year");
	int pub_year = year_v ? yyjson_get_int(year_v) : 0;

// INSERT DOCUMENTS

	int document_id; 
	int rc_doc = db_insert_document(
		ctx->db,
		ctx->stmt_document,
		title,
		abstract,
		doi,
		issn,
		pub_year,
		&document_id
	);

	if (rc_doc != SQLITE_OK) {
		ctx->insert_errors++;
		yyjson_doc_free(doc);
		free(json);
		return;
	}

	ctx->insert_ok++;

// AUTHORS

	yyjson_val *authors = yyjson_obj_get(meta, "authors");

	if (authors && yyjson_is_arr(authors)) {
		size_t n = yyjson_arr_size(authors);

		for (size_t i = 0; i < n; i++) {
			

			yyjson_val *a = yyjson_arr_get(authors, i);

			const char *first_name =
				yyjson_get_str(yyjson_obj_get(a, "first"));

			const char *last_name =
				yyjson_get_str(yyjson_obj_get(a, "last"));

			const char *initial =
				yyjson_get_str(yyjson_obj_get(a, "initial"));
		

			if(!first_name) first_name = "";
			if(!last_name) last_name = "";
			if(!initial) initial = "";

		int author_id;
		int rc_author = db_get_or_create_author(
				ctx->db,
				ctx->stmt_author,
				first_name,
				last_name,
				initial,
				&author_id
			);

/*			int author_id = db_get_author_id(
				ctx->stmt_author_get,
				first_name,
				last_name,
				initial
			);
*/
			if (rc_author != SQLITE_OK) {
				ctx->insert_errors++;
				continue;
			}
				
			int rc_link = db_document_x_author(
				ctx->stmt_document_x_author,
				document_id,
				author_id,
				(int) i
				);
			
			if (rc_link != SQLITE_OK && rc_link != SQLITE_CONSTRAINT) {
				ctx->insert_errors++;
			}
			
		}
	
	}

	yyjson_doc_free(doc);
	free(json);
}
