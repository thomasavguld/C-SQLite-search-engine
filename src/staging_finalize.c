#include <stdio.h>
#include <sqlite3.h>

#include "app_context.h"
#include "staging.h"

// Finalize file ingestion

void staging_finalize(AppContext *ctx, sqlite3 *db)
{
    int rc;
    char *err = NULL;

// Run inserts:

// Authors

    rc = sqlite3_exec(db,
        "INSERT OR IGNORE INTO authors(first_name, last_name, initial) "
        "SELECT DISTINCT first_name, last_name, initial "
        "FROM stg_authors;",
        NULL, NULL, &err);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "[AUTHORS] %s\n", err);
        sqlite3_free(err);
        err = NULL;

        ctx->ingest.insert_errors++;
    }
    else
    {
        ctx->ingest.total_author_ops += sqlite3_changes(db);
    }

//Documents

    rc = sqlite3_exec(db,
        "INSERT OR IGNORE INTO documents(doi, title, abstract, issn, pub_year) "
        "SELECT doi, title, abstract, issn, pub_year "
        "FROM stg_documents;",
        NULL, NULL, &err);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "[DOCUMENTS] %s\n", err);
        sqlite3_free(err);
        err = NULL;

        ctx->ingest.insert_errors++;
    }
    else
    {
        ctx->ingest.total_doc_ops += sqlite3_changes(db);
    }

// Document/Author Relations

    rc = sqlite3_exec(db,
        "INSERT OR IGNORE INTO documents_x_authors(document_id, author_id, author_order) "
        "SELECT d.id, a.id, r.author_order "
        "FROM stg_doc_authors r "
        "JOIN documents d ON d.doi = r.doc_doi "
        "JOIN authors a ON a.first_name = r.first_name "
        "               AND a.last_name  = r.last_name "
        "               AND a.initial    = r.initial;",
        NULL, NULL, &err);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "[RELATIONS] %s\n", err);
        sqlite3_free(err);
        err = NULL;

        ctx->ingest.insert_errors++;
    }
    else
    {
        ctx->ingest.total_rel_ops += sqlite3_changes(db);
    }

// Clean staging tables from raw input data

    sqlite3_exec(db, "DELETE FROM stg_documents;", NULL, NULL, NULL);
    sqlite3_exec(db, "DELETE FROM stg_authors;", NULL, NULL, NULL);
    sqlite3_exec(db, "DELETE FROM stg_doc_authors;", NULL, NULL, NULL);
}