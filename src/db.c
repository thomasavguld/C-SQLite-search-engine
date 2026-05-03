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

int author_sql(sqlite3 *db, const char *sql) {
	char *errMsg = NULL;
	int rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);

	sqlite3_free(errMsg);
	return rc;
}

int document_x_author_sql(sqlite3 *db, const char *sql) {
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

	sqlite3_busy_timeout(db, 5000);

	// SQLite PRAGMA settings for bulk ingestion
	exec_sql(db, "PRAGMA journal_mode=0;");
	exec_sql(db, "PRAGMA synchronous=OFF;");
	exec_sql(db, "PRAGMA cache_size=-100000 ;");
	exec_sql(db, "PRAGMA temp_store=MEMORY ;");
	exec_sql(db, "PRAGMA page_size=4096 ;");
	exec_sql(db, "PRAGMA mmap_size=0 ;");
	exec_sql(db, "PRAGMA cahe_spill=OFF ;");

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
		"initial TEXT, "
		"UNIQUE(first_name, last_name, initial)"
		");"
	);

/*	exec_sql(db,
		"CREATE UNIQUE INDEX IF NOT EXISTS "
		"idx_authors_unique "
		"ON authors("
		"first_name, "
		"last_name, "
		"initial"
		");"
	);
*/
	exec_sql(db,
		"CREATE TABLE IF NOT EXISTS documents_x_authors ("
		"document_id INTEGER NOT NULL, "
		"author_id INTEGER NOT NULL, "
		"author_order INTEGER, "
		"PRIMARY KEY (document_id, author_id)"
		");"
	);
/*	
	exec_sql(db,
		"CREATE INDEX idx_doc_auth ON documents_x_authors(document_id);"
	);

	exec_sql(db,
		"CREATE INDEX idx_auth_doc ON documents_x_authors(author_id);"
	);
*/
	return SQLITE_OK;
}

int db_prepare(sqlite3 *db, struct AppContext *ctx) {

	int rc;

	rc = sqlite3_prepare_v2(db,
		"INSERT INTO documents("
		"title,"
		"abstract,"
		"doi,"
		"issn,"
		"pub_year)"
		"VALUES(?, ?, ?, ?, ?);",
		-1, 
		&ctx->stmt_document, 
		NULL
	);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
		return rc;
	}

	const char *sql_author =
		"INSERT INTO authors("
		"first_name,"
		"last_name,"
		"initial)"
		"VALUES(?, ?, ?) "
		"ON CONFLICT(first_name, last_name, initial) "
		"DO UPDATE SET first_name=excluded.first_name "
		"RETURNING id;";

	sqlite3_prepare_v2(
			db,
			sql_author,
			-1,
			ctx->stmt_author,
			NULL
			);

	if (!ctx->stmt_author) {
		fprintf(stderr, "Failed to prepare stmt_author: %s\n");
		fprint("Author SQL: %s\n", sql_author);
	}
 /*
	
	rc = sqlite3_prepare_v2(db,
		"INSERT INTO authors("
		"first_name,"
		"last_name,"
		"initial)"
		"VALUES(?, ?, ?) "
		"ON CONFLICT(first_name, last_name, initial) "
		"DO UPDATE SET first_name=excluded.first_name "
		"RETURNING id;",
		-1, 
		&ctx->stmt_author, 
		NULL
	);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
		return rc;
	}
*/	
	
	rc = sqlite3_prepare_v2(db,
		"INSERT OR IGNORE INTO documents_x_authors("
		"document_id,"
		"author_id,"
		"author_order) "
		"VALUES "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?), "
		"(?, ?, ?);"
		");",
		-1, 
		&ctx->stmt_document_x_author, 
		NULL
	);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
		return rc;
	}

	return SQLITE_OK;
}

int db_insert_document(
		sqlite3 *db,
		sqlite3_stmt *stmt,
		const char *title, 
		const char *abstract,
		const char *doi,
		const char *issn,
		int pub_year,
		int *out_id
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
				
		if (rc == SQLITE_DONE) {
			*out_id = (int)sqlite3_last_insert_rowid(db);
		
			sqlite3_reset(stmt);
			sqlite3_clear_bindings(stmt);
		
			return SQLITE_OK;
		}

		fprintf(stderr, "Document insert error: %s (%d)\n",
			sqlite3_errmsg(db), rc);

		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		return rc;
	}

int db_get_or_create_author(
		sqlite3 *db,
		sqlite3_stmt *stmt,
		const char *first_name,
		const char *last_name,
		const char *initial,
		int *out_id
	) {
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		sqlite3_bind_text(stmt, 1, first_name ? first_name : "", -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, last_name ? last_name : "", -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, initial ? initial : "", -1, SQLITE_TRANSIENT);

		int rc = sqlite3_step(stmt);
		
		if (rc == SQLITE_ROW) {
			*out_id = sqlite3_column_int(stmt, 0);
			
			sqlite3_reset(stmt);
			sqlite3_clear_bindings(stmt);
			return SQLITE_OK;
		}

		fprintf(stderr, "Author upsert failed - rc: %d msg: %s\n",
				rc, sqlite3_errmsg(db));
	
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);
		return rc;
		
}


int db_document_x_author_batch(
		sqlite3_stmt *stmt,
		int *doc_ids,
		int *author_ids,
		int *orders,
		int count
		)
	{

		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		int param = 1;
		
		for (int i = 0; i < count; i++) {
		sqlite3_bind_int(stmt, param++, doc_ids[i]);
		sqlite3_bind_int(stmt, param++, author_ids[i]);
		sqlite3_bind_int(stmt, param++, orders[i]);
		}

		int max_rows = 32;
		
		for (int i = count; i < max_rows; i++) {
		sqlite3_bind_int(stmt, param++, -1);
		sqlite3_bind_int(stmt, param++, -1);
		sqlite3_bind_int(stmt, param++, -1);
		}
		
		int rc = sqlite3_step(stmt);
		
		if (rc != SQLITE_DONE) {
			fprintf(stderr, "Batch insert error: %s (%d)\n",
				sqlite3_errmsg(sqlite3_db_handle(stmt)), rc);
		}

		return rc;

	}

int db_document_x_author(
		sqlite3_stmt *stmt,
		int document_id,
		int author_id,
		int order
	) {
		
		int rc;

		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);

		rc = sqlite3_bind_int(stmt, 1, document_id);
		if (rc != SQLITE_OK) return rc;

		rc = sqlite3_bind_int(stmt, 2, author_id);
		if (rc != SQLITE_OK) return rc;
		
		rc = sqlite3_bind_int(stmt, 3, order);
		if (rc != SQLITE_OK) return rc;

		rc = sqlite3_step(stmt);

		if (rc == SQLITE_DONE) {
			sqlite3_reset(stmt);
			sqlite3_clear_bindings(stmt);
			return SQLITE_OK;
		}
		
		if (rc == SQLITE_CONSTRAINT) {
			sqlite3_reset(stmt);
			sqlite3_clear_bindings(stmt);
			return SQLITE_CONSTRAINT;
		}

		fprintf(stderr, "Document_x_author failed rc=%d msg=%s\n",
				rc, sqlite3_errmsg(sqlite3_db_handle(stmt)));

		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);
		
		return rc;
		
}

void flush_rel_batch(
		AppContext *ctx,
		sqlite3_stmt *stmt,
		int *doc_ids,
		int *author_ids,
		int *orders,
		int rel_count

) {
	
	if (rel_count <= 0) return;

	int rc = db_document_x_author_batch(
		stmt,	
		doc_ids,
		author_ids,
		orders,
		rel_count
	);

	ctx->rel_ops += rel_count;
	ctx->rel_batches++;

	if (rc != SQLITE_DONE) {
		ctx->insert_errors++;
	}
}


int callback(void *data, int argc, char **argv, char **colNames) {
	(void)data;

	for (int i = 0; i < argc; i++) {
		printf("%s: %s\n", colNames[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
	
}
