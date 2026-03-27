#include <stdio.h>
#include <sqlite3.h>

int main() {
	sqlite3 *db;
	char *errMsg = 0;

	int rc = sqlite3_open("data/test.db", &db);

	if (rc) {
		printf("Cannot open DB.\n");
		return 1;
	}

	const char *sql =
		"CREATE TABLE IF NOT EXISTS documents ("
		"id INTEGER PRIMARY KEY,"
		"content TEXT NOT NULL"
		");";

	rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
		
	if (rc != SQLITE_OK) {
		printf("SQL error: %s\n", errMsg);
		sqlite3_free(errMsg);
	} else {
		printf("Table created.\n");
	}

	sqlite3_close(db);

	return 0;

}
