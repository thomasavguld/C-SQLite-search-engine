#include <stdio.h>
#include <sqlite3.h>

#include <app_context.h>
#include <db.h>

int documents_sql(sqlite3 *db, const char *sql) {
	char *errMsg = NULL;
	int rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);

	sqlite3_free(errMsg);
	return rc;
}

int authors_sql(sqlite3 *db, const char *sql) {
	char *errMsg = NULL;
	int rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);

	sqlite3_free(errMsg);
	return rc;
}

int documents_x_authors_sql(sqlite3 *db, const char *sql) {
	char *errMsg = NULL;
	int rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);

	sqlite3_free(errMsg);
	return rc;
}

int exec_sql(sqlite3 *db, const char *sql) {
	char *errMsg = NULL;
	int rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errMsg);
		sqlite3_free(errMsg);
	}

	return rc;
}

int db_init(sqlite3 *db) {

	// SQLite PRAGMA settings for bulk ingestion
	exec_sql(db, "PRAGMA journal_mode=WAL;");
	exec_sql(db, "PRAGMA synchronous=OFF;");
	exec_sql(db, "PRAGMA cache_size=100000 ;");

	// Create tables if they don't already exist

	exec_sql(db, 
		"CREATE TABLE IF NOT EXISTS documents ("
		"id INTEGER PRIMARY KEY,"
		"title TEXT UNIQUE,"
		"abstract TEXT, "
		"doi TEXT, "
		"issn TEXT, "
		"pub_year INTEGER"
		");"
	);

	exec_sql(db, 
		"CREATE TABLE IF NOT EXISTS authors ("
		"id INTEGER PRIMARY KEY,"
		"first_name TEXT,"
		"last_name TEXT, "
		"initial TEXT "
		");"
	);

	exec_sql(db,
		"CREATE UNIQUE INDEX IF NOT EXISTS "
		"idx_authors_unique "
		"ON authors("
		"first_name, "
		"last_name, "
		"initial"
		");"
	);

	exec_sql(db,
		"CREATE TABLE IF NOT EXISTS documents_x_authors ("
		"document_id INTEGER,"
		"author_id INTEGER,"
		"PRIMARY KEY (document_id, author_id)"
		");"
	);

	return SQLITE_OK;
}

int db_prepare(sqlite3 *db, struct AppContext *ctx) {

	sqlite3_prepare_v2(db,
		"INSERT INTO documents("
		"title,"
		"abstract,"
		"doi,"
		"issn,"
		"pub_year)"
		"VALUES(?, ?, ?, ?, ?);",
		-1, &ctx->stmt_documents, NULL);
			
	sqlite3_prepare_v2(db,
		"INSERT OR IGNORE INTO authors("
		"first_name,"
		"last_name,"
		"initial)"
		"VALUES(?, ?, ?);",
		-1, &ctx->stmt_authors, NULL);
	
	sqlite3_prepare_v2(db,
		"SELECT id "
		"FROM authors "
		"WHERE first_name=? "
		"AND last_name=? "
		"AND initial=?;",
		-1, &ctx->stmt_author_get, NULL);
	
	sqlite3_prepare_v2(db,
		"INSERT INTO documents_x_authors("
		"document_id,"
		"author_id,"
		"author_order)"
		"VALUES(?, ?, ?);",
		-1, &ctx->stmt_document_x_author, NULL);

	return SQLITE_OK;
}

int db_insert_document(
		sqlite3 *db,
		sqlite3_stmt *stmt,
		const char *title, 
		const char *abstract,
		const char *doi,
		const char *issn,
		int pub_year
	) {
		int rc;

		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		rc = sqlite3_bind_text(stmt, 1, title ? title : "", -1, SQLITE_TRANSIENT);
		if (rc != SQLITE_OK) return rc;
		
		rc = sqlite3_bind_text(stmt, 2, abstract ? abstract : "", -1, SQLITE_TRANSIENT);
		if (rc != SQLITE_OK) return rc;
		
		rc = sqlite3_bind_text(stmt, 3, doi ? doi : "", -1, SQLITE_TRANSIENT);	
		if (rc != SQLITE_OK) return rc;
		
		rc = sqlite3_bind_text(stmt, 4, issn ? issn : "", -1, SQLITE_TRANSIENT);
		if (rc != SQLITE_OK) return rc;
		
		rc = sqlite3_bind_int(stmt, 5, pub_year);
		if (rc != SQLITE_OK) return rc;
		
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE) {
			sqlite3_reset(stmt);
			return rc;
		}

		sqlite3_reset(stmt);

		return sqlite3_last_insert_rowid(db);
	}

int db_insert_author(sqlite3_stmt *stmt,
		const char *first_name,
		const char *last_name,
		const char *initial)
{

		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		sqlite3_bind_text(stmt, 1, first_name, -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, last_name, -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, initial, -1, SQLITE_TRANSIENT);

		int rc = sqlite3_step(stmt);
		sqlite3_reset(stmt);
		return rc;

}

int db_get_author_id(sqlite3_stmt *stmt,
		const char *first_name,
		const char *last_name,
		const char *initial
		)
	{

		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		sqlite3_bind_text(stmt, 1, first_name, -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, last_name, -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, initial, -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) == SQLITE_ROW) {
			int id = sqlite3_column_int(stmt, 0);
			sqlite3_reset(stmt);
			return id;
		}

		sqlite3_reset(stmt);
	   	return -1;

	}

int db_document_x_author(sqlite3_stmt *stmt,
		int document_id,
		int author_id,
		int order)

{

		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		sqlite3_bind_int(stmt, 1, document_id);
		sqlite3_bind_int(stmt, 2, author_id);
		sqlite3_bind_int(stmt, 3, order);

		return sqlite3_step(stmt);
}

int callback(void *data, int argc, char **argv, char **colNames) {
	(void)data;

	for (int i = 0; i < argc; i++) {
		printf("%s: %s\n", colNames[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
	
}
