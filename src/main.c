#include <stdio.h>
#include <sqlite3.h>

#define DB_PATH "data/test.db"

// Loop through and print selected data

int callback(void *data, int argc, char **argv, char **colNames) {
	(void)data;

	for (int i = 0; i < argc; i++) {
		printf("%s: %s\n", colNames[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
	}

int main() {

	// Open database connection
	
	sqlite3 *db;
	char *errMsg = 0;

	int rc = sqlite3_open("DB_PATH", &db);

	if (rc) {
		printf("Cannot open database connection.\n");
		return 1;
	}

	// Create tables if they don't already exist

	const char *create_sql =
		"CREATE TABLE IF NOT EXISTS documents ("
		"id INTEGER PRIMARY KEY,"
		"content TEXT NOT NULL"
		");";

	rc = sqlite3_exec(db, create_sql, 0, 0, &errMsg);
	if (rc != SQLITE_OK) {
		printf("Create table error: %s\n", errMsg);
		sqlite3_free(errMsg);
		sqlite3_close(db);
		return 1;
	}

	// Insert values into table "documents"
	
	const char *insert_sql =
		"INSERT INTO documents (content) VALUES ('lorem ipsum eller nåt annat nonsens');";

	rc = sqlite3_exec(db, insert_sql, 0, 0, &errMsg);

	if (rc != SQLITE_OK) {
		printf("Insert error: %s\n", errMsg);
		sqlite3_free(errMsg);
	} else {
		printf("Insert OK\n");
	}


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
