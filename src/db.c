#include <stdio.h>
#include <sqlite3.h>

#include "db.h"
#include "app_context.h"

int exec_sql(sqlite3 *db, const char *sql)
{
    char *err = NULL;

    int rc = sqlite3_exec(db, sql, 0, 0, &err);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err ? err : "unknown");
        sqlite3_free(err);
    }

    return rc;
}

int db_init(sqlite3 *db)
{
    sqlite3_busy_timeout(db, 5000);

    exec_sql(db, "PRAGMA journal_mode=WAL;");
    exec_sql(db, "PRAGMA synchronous=OFF;");
    exec_sql(db, "PRAGMA cache_size=-20000;");
    exec_sql(db, "PRAGMA temp_store=MEMORY;");
    exec_sql(db, "PRAGMA page_size=4096;");
    exec_sql(db, "PRAGMA cache_spill=OFF;");
    exec_sql(db, "PRAGMA wal_autocheckpoint=50000;");

    /* FINAL TABLES */

    exec_sql(db,
        "CREATE TABLE IF NOT EXISTS documents ("
        "id INTEGER PRIMARY KEY,"
        "doi TEXT UNIQUE,"
        "title TEXT,"
        "abstract TEXT,"
        "issn TEXT,"
        "pub_year INTEGER"
        ");"
    );

    exec_sql(db,
        "CREATE TABLE IF NOT EXISTS authors ("
        "id INTEGER PRIMARY KEY,"
        "first_name TEXT,"
        "last_name TEXT,"
        "initial TEXT,"
        "UNIQUE(first_name, last_name, initial)"
        ");"
    );

    exec_sql(db,
        "CREATE TABLE IF NOT EXISTS documents_x_authors ("
        "document_id INTEGER NOT NULL,"
        "author_id INTEGER NOT NULL,"
        "author_order INTEGER,"
        "PRIMARY KEY(document_id, author_id)"
        ");"
    );

    /* NGRAMS */
    exec_sql(db,
        "CREATE TABLE IF NOT EXISTS ngrams ("
        "gram TEXT NOT NULL,"
        "doc_id INTEGER NOT NULL"
        ");"
    );

    /* INDEXES */

    exec_sql(db,
        "CREATE INDEX IF NOT EXISTS idx_doc_auth "
        "ON documents_x_authors(document_id);"
    );

    exec_sql(db,
        "CREATE INDEX IF NOT EXISTS idx_auth_doc "
        "ON documents_x_authors(author_id);"
    );

    exec_sql(db,
        "CREATE INDEX IF NOT EXISTS idx_ngrams_gram "
        "ON ngrams(gram);"
    );

    return SQLITE_OK;
}