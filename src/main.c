#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <dirent.h>
#include "yyjson.h"


// define db & warehouse paths
#ifndef DB_PATH
#define DB_PATH "./db/c_search.db"
#endif

#ifndef WAREHOUSE_PATH
#define WAREHOUSE_PATH "./warehouse"
#endif

// I/O (file system)
char *read_file(const char *filepath);
void list_files(const char *path, sqlite3 *db, sqlite3_stmt *stmt_main, sqlite3_stmt *stmt_fts); 

// Create tables
int exec_sql(sqlite3 *db, const char *sql);

// Extract fields from JSON
char *extract_title(const char *json_str);
char *extract_abstract(const char *json_str);

// Data base insertion
void insert_document(sqlite3_stmt *stmt, const char *title, const char *abstract);

// Reading from database
int callback(void *data, int argc, char **argv, char **colNames);

//Main function

int main() {

	// Open database connection
	
	sqlite3 *db;
	char *errMsg = 0;

	int rc_open = sqlite3_open(DB_PATH, &db);

	if (rc_open) {
		printf("Cannot open database connection.\n");
		return 1;
	} else {
		printf("Database connection open.\n");
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
		"abstract"
		");";

	if (exec_sql(db, create_sql) != SQLITE_OK) return 1;	
	if (exec_sql(db, create_fts_sql) != SQLITE_OK) return 1;	

	printf("Database ready.\n");


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

	const char *select_sql = "SELECT title, abstract FROM documents;"; 

	int rc_select = sqlite3_exec(db, select_sql, callback, 0, &errMsg);

	if (rc_select != SQLITE_OK) {
		printf("Select error: %s\n", errMsg);
		sqlite3_free(errMsg);
	}
		// Close database connection

	sqlite3_close(db);

	return 0;

}


// List files

void list_files(const char *path, sqlite3 *db, sqlite3_stmt *stmt_main, sqlite3_stmt *stmt_fts) { 

	(void)db; // <--- KEEP AN EYE ON THIS FOR LATER

	struct dirent *entry;
	DIR *dir = opendir(path);

	if (!dir) {
		perror("Opendir failed.\n");
		printf("Dir: %s\n", path);
		return;
	}

	while ((entry = readdir(dir)) != NULL) {

		if (entry->d_name[0] == '.') continue;
		
		size_t len = strlen(path) + strlen(entry->d_name) + 2;

		char *filepath = malloc(len);
		if (!filepath) return;

		snprintf(filepath, len, "%s/%s", path, entry->d_name);

		char *json = read_file(filepath);
		if (!json) {
			free(filepath);
			continue;
		}


		char *title = extract_title(json);
		char *abstract = extract_abstract(json);

		insert_document(stmt_main, title, abstract);
		insert_document(stmt_fts, title, abstract);
		
		free(filepath);
		free(json);
		free(title);
		free(abstract);

		}

	closedir(dir);

	}

char *read_file(const char *filepath) {
	FILE *file = fopen(filepath, "r");
	if (!file) return NULL;

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);
	
	if (size <=0) {
		fclose(file);
		return NULL;
	}

	char *buffer = malloc(size + 1);	
	if (!buffer) {
		fclose(file);
		return NULL;
	}

	size_t read = fread(buffer, 1, size, file);
	if (read != size) {
		free(buffer);
		fclose(file);
		return NULL;
	}

	buffer[size] = '\0';
	fclose(file);
	return buffer;
}

void insert_document(sqlite3_stmt *stmt, const char *title, const char *abstract) {

	sqlite3_bind_text(stmt, 1, title ? title : "", -1, SQLITE_TRANSIENT);	
	sqlite3_bind_text(stmt, 2, abstract ? abstract : "", -1, SQLITE_TRANSIENT);

	int rc_insert = sqlite3_step(stmt);

	if (rc_insert != SQLITE_DONE) {
		printf("Insert error: %s\n", sqlite3_errmsg(sqlite3_db_handle(stmt)));
	}
	
	sqlite3_reset(stmt);
	sqlite3_clear_bindings(stmt);
	
}

// Loop through and print selected data

int callback(void *data, int argc, char **argv, char **colNames) {
	(void)data;

	for (int i = 0; i < argc; i++) {
		printf("%s: %s\n", colNames[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
	}

char *extract_title(const char *json_str) {
	yyjson_doc *doc = yyjson_read(json_str, strlen(json_str), 0);
	if (!doc) return strdup("");

	yyjson_val *root = yyjson_doc_get_root(doc);
	if (!root) {
		yyjson_doc_free(doc);
		return strdup("");
	}

	yyjson_val *title = yyjson_obj_get(root, "title");

	char *out = strdup(
		(title && yyjson_is_str(title)) ? yyjson_get_str(title) : ""
	);

	yyjson_doc_free(doc);
	return out;
	}

char *extract_abstract(const char *json_str) {
	yyjson_doc *doc = yyjson_read(json_str, strlen(json_str), 0);
	if (!doc) return strdup("");

	yyjson_val *root = yyjson_doc_get_root(doc);
	if (!root) {
		yyjson_doc_free(doc);
		return strdup("");
	}

	yyjson_val *abstract = yyjson_obj_get(root, "abstract");

	char *out = strdup(
		(abstract && yyjson_is_str(abstract)) ? yyjson_get_str(abstract) : ""
	);

	yyjson_doc_free(doc);
	return out;
	
}


int exec_sql(sqlite3 *db, const char *sql) {
	char *errMsg = 0;
	int rc_exec = sqlite3_exec(db, sql, 0, 0, &errMsg);

		if (rc_exec != SQLITE_OK) {
			printf("SQL error: %s\n", errMsg);
			sqlite3_free(errMsg);
		}

		return rc_exec;
	}

