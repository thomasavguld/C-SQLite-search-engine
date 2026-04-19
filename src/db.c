#include <stdio.h>
#include <sqlite3.h>

int exec_sql(sqlite3 *db, const char *sql) {
	char *errMsg = 0;
	int rc_exec = sqlite3_exec(db, sql, 0, 0, &errMsg);

		if (rc_exec != SQLITE_OK) {
			printf("SQL error: %s\n", errMsg);
			sqlite3_free(errMsg);
		}

		return rc_exec;
	}


void insert_document(
		sqlite3_stmt *stmt, 
		const char *title, 
		const char *abstract,
		const char *doi,
		const char *issn,
		int pub_year
	) {

		sqlite3_bind_text(stmt, 1, title ? title : "", -1, SQLITE_TRANSIENT);	
		sqlite3_bind_text(stmt, 2, abstract ? abstract : "", -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, doi ? doi : "", -1, SQLITE_TRANSIENT);	
		sqlite3_bind_text(stmt, 4, issn ? issn : "", -1, SQLITE_TRANSIENT);
		sqlite3_bind_int(stmt, 5, pub_year);		


		if (sqlite3_step(stmt) != SQLITE_DONE) {
			printf("Insert error: %s\n", sqlite3_errmsg(sqlite3_db_handle(stmt)));
		}
	
		sqlite3_clear_bindings(stmt);
		sqlite3_reset(stmt);
	
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
