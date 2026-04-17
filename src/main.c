#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <dirent.h>

#define DB_PATH "./db/c_search.db"

void list_files(const char *path, sqlite3 *db); 
char *read_file(const char *filepath);
void insert_document(sqlite3 *db, const char *filename, const char *content);
int callback(void *data, int argc, char **argv, char **colNames);

int main() {

	// Open database connection
	
	sqlite3 *db;
	char *errMsg = 0;

	int rc = sqlite3_open(DB_PATH, &db);

	if (rc) {
		printf("Cannot open database connection.\n");
		return 1;
	} /* else {
		printf("database connection open.\n");
	}*/

	// Create tables if they don't already exist

	const char *create_sql =
		"CREATE TABLE IF NOT EXISTS documents ("
		"id INTEGER PRIMARY KEY,"
		"filename TEXT,"
		"content TEXT"
		");";

	rc = sqlite3_exec(db, create_sql, 0, 0, &errMsg);
	if (rc != SQLITE_OK) {
		printf("Create table error: %s\n", errMsg);
		sqlite3_free(errMsg);
		sqlite3_close(db);
		return 1;
	}
	
	
	// Run ingestion
	
	sqlite3_exec(db, "BEGIN;", 0, 0 ,0);

	list_files("./warehouse", db);

	sqlite3_exec(db, "COMMIT;", 0, 0, 0);

	// Select data from documents  

	const char *select_sql = "SELECT filename FROM documents;"; 

	rc = sqlite3_exec(db, select_sql, callback, 0, &errMsg);

	printf("SELECT rc = %d\n", rc);

	if (rc != SQLITE_OK) {
		printf("Select error: %s\n", errMsg);
		sqlite3_free(errMsg);
	} else { 
		printf("Select OK\n");
	}
/*
	rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
		
	if (rc != SQLITE_OK) {
		printf("SQL error: %s\n", errMsg);
		sqlite3_free(errMsg);
	} else {
		printf("Table created.\n");
	}
*/
	// Close database connection

	sqlite3_close(db);

	return 0;

}


// List files

void list_files(const char *path, sqlite3 *db) {
	struct dirent *entry;
	DIR *dir = opendir(path);

	if (dir == NULL) {
		printf("Warehouse directory is empty.\n");
		return;
	}

	while ((entry = readdir(dir)) != NULL) {

		if (entry->d_name[0] == '.') continue;

		char filepath[256];
		sprintf(filepath, "%s/%s", path, entry->d_name);

		char *content = read_file(filepath);

		if (content != NULL) {
			insert_document(db, entry->d_name, content);
			free(content);

		}
	}

	closedir(dir);

	printf("Document insert OK.\n");
}

char *read_file(const char *filepath) {
	FILE *file = fopen(filepath, "r");

	if (!file) {
		printf("Cannot open file: %s\n", filepath);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	rewind(file);

	char *buffer = malloc(size + 1);
	fread(buffer, 1, size, file);
	buffer[size] = '\0';

	fclose(file);
	return buffer;
}

void insert_document(sqlite3 *db, const char *filename, const char *content) {
	
	sqlite3_stmt *stmt;

	const char *sql =
		"INSERT INTO documents(filename, content) "
		"VALUES(:filename, :content);";

	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		printf("Prepare error: %s\n", sqlite3_errmsg(db));
		return;
	//} else {
	//	printf("Document insert OK.\n");
	}

	int idx_filename = sqlite3_bind_parameter_index(stmt, ":filename");
	int idx_content = sqlite3_bind_parameter_index(stmt, ":content");

	sqlite3_bind_text(stmt, idx_filename, filename, -1, SQLITE_TRANSIENT);	
	sqlite3_bind_text(stmt, idx_content, content, -1, SQLITE_TRANSIENT);

	rc = sqlite3_step(stmt);

	if (rc != SQLITE_DONE) {
		printf("Insert error: %s\n", sqlite3_errmsg(db));
	}

	sqlite3_finalize(stmt);
	
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






