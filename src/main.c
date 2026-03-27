#include <stdio.h>
#include <sqlite3.h>

int main() {
	sqlite3 *db;
	int rc = sqlite3_open("data/test.db", &db);

	if (rc != SQLITE_OK) {
		printf("No DB conn.\n");
		return 1;
	}

	printf("DB conn open.\n");

	sqlite3_close(db);
	return 0;
}
