#include <sqlite3.h>
#include <stdio.h>

#include "app_context.h"
#include "staging.h"

/* -------------------------------------------------- */
/* internal helper                                    */
/* -------------------------------------------------- */

static void exec(sqlite3 *db, const char *sql)
{
    char *err = NULL;

    if (sqlite3_exec(db, sql, 0, 0, &err) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
    }
}

/* -------------------------------------------------- */
/* init                                               */
/* -------------------------------------------------- */

void staging_init(sqlite3 *db, StagingContext *s)
{
    exec(db,
        "CREATE TABLE IF NOT EXISTS stg_documents ("
        "doi TEXT UNIQUE,"
        "title TEXT,"
        "abstract TEXT,"
        "issn TEXT,"
        "pub_year INTEGER"
        ");");

    exec(db,
        "CREATE TABLE IF NOT EXISTS stg_authors ("
        "first_name TEXT,"
        "last_name TEXT,"
        "initial TEXT"
        ");");

    exec(db,
        "CREATE TABLE IF NOT EXISTS stg_doc_authors ("
        "doc_doi TEXT,"
        "first_name TEXT,"
        "last_name TEXT,"
        "initial TEXT,"
        "author_order INTEGER"
        ");");

    sqlite3_prepare_v2(db,
        "INSERT INTO stg_documents VALUES (?, ?, ?, ?, ?);",
        -1, &s->stg_doc, NULL);

    sqlite3_prepare_v2(db,
        "INSERT INTO stg_authors VALUES (?, ?, ?);",
        -1, &s->stg_author, NULL);

    sqlite3_prepare_v2(db,
        "INSERT INTO stg_doc_authors VALUES (?, ?, ?, ?, ?);",
        -1, &s->stg_rel, NULL);
}

/* -------------------------------------------------- */
/* DOCUMENT                                            */
/* -------------------------------------------------- */

int stage_document(AppContext *ctx,
    StagingContext *s,
    const char *doi,
    const char *title,
    const char *abstract,
    const char *issn,
    int pub_year)
{
    sqlite3_reset(s->stg_doc);
    sqlite3_clear_bindings(s->stg_doc);

    sqlite3_bind_text(s->stg_doc, 1, doi ? doi : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s->stg_doc, 2, title ? title : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s->stg_doc, 3, abstract ? abstract : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s->stg_doc, 4, issn ? issn : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (s->stg_doc, 5, pub_year);

    int rc = sqlite3_step(s->stg_doc);

    if (rc == SQLITE_DONE) {
        ctx->ingest.doc_ops++;
        ctx->ingest.total_doc_ops++;
        return SQLITE_OK;
    }

    ctx->ingest.insert_errors++;
    return SQLITE_ERROR;
}

/* -------------------------------------------------- */
/* AUTHOR                                              */
/* -------------------------------------------------- */

int stage_author(AppContext *ctx,
    StagingContext *s,
    const char *first,
    const char *last,
    const char *initial)
{
    sqlite3_reset(s->stg_author);
    sqlite3_clear_bindings(s->stg_author);

    sqlite3_bind_text(s->stg_author, 1, first ? first : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s->stg_author, 2, last ? last : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s->stg_author, 3, initial ? initial : "", -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(s->stg_author);

    if (rc == SQLITE_DONE) {
        ctx->ingest.author_ops++;
        ctx->ingest.total_author_ops++;
        return SQLITE_OK;
    }

    ctx->ingest.insert_errors++;
    return SQLITE_ERROR;
}

/* -------------------------------------------------- */
/* RELATION                                            */
/* -------------------------------------------------- */

int stage_relation(AppContext *ctx,
    StagingContext *s,
    const char *doc_doi,
    const char *first,
    const char *last,
    const char *initial,
    int ord)
{
    sqlite3_reset(s->stg_rel);
    sqlite3_clear_bindings(s->stg_rel);

    sqlite3_bind_text(s->stg_rel, 1, doc_doi ? doc_doi : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s->stg_rel, 2, first ? first : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s->stg_rel, 3, last ? last : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s->stg_rel, 4, initial ? initial : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (s->stg_rel, 5, ord);

    int rc = sqlite3_step(s->stg_rel);

    if (rc == SQLITE_DONE) {
        ctx->ingest.rel_ops++;
        ctx->ingest.total_rel_ops++;
        return SQLITE_OK;
    }

    ctx->ingest.insert_errors++;
    return SQLITE_ERROR;
}

/* -------------------------------------------------- */
/* finalize                                            */
/* -------------------------------------------------- */

void staging_finalize(AppContext *ctx, sqlite3 *db)
{
    exec(db,
        "INSERT INTO authors(first_name, last_name, initial) "
        "SELECT DISTINCT first_name, last_name, initial FROM stg_authors;");

    exec(db,
        "INSERT INTO documents(doi, title, abstract, issn, pub_year) "
        "SELECT doi, title, abstract, issn, pub_year FROM stg_documents;");

    exec(db,
        "INSERT OR IGNORE INTO documents_x_authors(document_id, author_id, author_order) "
        "SELECT d.id, a.id, r.author_order "
        "FROM stg_doc_authors r "
        "JOIN documents d ON d.doi = r.doc_doi "
        "JOIN authors a ON a.first_name = r.first_name "
        "AND a.last_name = r.last_name "
        "AND a.initial = r.initial;");

    exec(db, "DELETE FROM stg_documents;");
    exec(db, "DELETE FROM stg_authors;");
    exec(db, "DELETE FROM stg_doc_authors;");
}

/* -------------------------------------------------- */
/* cleanup                                            */
/* -------------------------------------------------- */

void staging_cleanup(StagingContext *s)
{
    sqlite3_finalize(s->stg_doc);
    sqlite3_finalize(s->stg_author);
    sqlite3_finalize(s->stg_rel);
}