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
	}

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
	list_files("./warehouse", db);

	// Select all data from documents  

	const char *select_sql = "SELECT * FROM documents;"; 

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
	char *errMsg = 0;
	int rc;

	char sql_insert[2048];

	snprintf(sql_insert, sizeof(sql_insert),
		"INSERT INTO documents(filename, content) VALUES('%s', '%s');",
		filename, content);

	rc = sqlite3_exec(db, sql_insert, 0, 0, &errMsg);

	if (rc != SQLITE_OK) {
		printf("Insert error: %s\n", errMsg);
		sqlite3_free(errMsg);
	} else {
		printf("Document insert OK.\n");
	}


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






