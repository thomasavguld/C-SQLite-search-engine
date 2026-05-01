#include <stdio.h>
#include <sqlite3.h>

int exec_sql(sqlite3 *db, const char *sql) {
	char *errMsg = NULL;
	int rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);

	sqlite3_free(errMsg);
	return rc;
}

int db_exec_insert(
		sqlite3_stmt *stmt, 
		const char *title, 
		const char *author, 
		const char *abstract,
		const char *doi,
		const char *issn,
		int pub_year
	) {
		int rc = SQLITE_OK;

		rc = sqlite3_reset(stmt);
		if (rc != SQLITE_OK) return rc;

		rc = sqlite3_clear_bindings(stmt);
		if (rc != SQLITE_OK) return rc;
		
		sqlite3_bind_text(stmt, 1, title ? title : "", -1, SQLITE_TRANSIENT);	
		if (rc != SQLITE_OK) return rc;
		
		sqlite3_bind_text(stmt, 2, author ? author : "", -1, SQLITE_TRANSIENT);	
		if (rc != SQLITE_OK) return rc;
		
		sqlite3_bind_text(stmt, 3, abstract ? abstract : "", -1, SQLITE_TRANSIENT);
		if (rc != SQLITE_OK) return rc;
		
		sqlite3_bind_text(stmt, 4, doi ? doi : "", -1, SQLITE_TRANSIENT);	
		if (rc != SQLITE_OK) return rc;
		
		sqlite3_bind_text(stmt, 5, issn ? issn : "", -1, SQLITE_TRANSIENT);
		if (rc != SQLITE_OK) return rc;
		
		sqlite3_bind_int(stmt, 6, pub_year);
		if (rc != SQLITE_OK) return rc;
		
		rc =sqlite3_step(stmt);
		if (rc != SQLITE_DONE) {
			sqlite3_reset(stmt);
			return rc;
		}

		return sqlite3_reset(stmt);
}

int insert_document(
		sqlite3_stmt *stmt, 
		const char *title, 
		const char *author, 
		const char *abstract,
		const char *doi,
		const char *issn,
		int pub_year
	) {
		return db_exec_insert(
			stmt,
			title,
			author,
			abstract,
			doi,
			issn,
			pub_year
		);
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
