#include <sqlite3.h>
#include <stdio.h>

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
    /* --- staging tables --- */

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

    /* --- prepared statements --- */

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
/* staging inserts                                    */
/* -------------------------------------------------- */

int stage_document(StagingContext *s,
    const char *doi,
    const char *title,
    const char *abstract,
    const char *issn,
    int pub_year)
{
    int rc;

    sqlite3_reset(s->stg_doc);
    sqlite3_clear_bindings(s->stg_doc);

    sqlite3_bind_text(s->stg_doc, 1, doi ? doi : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s->stg_doc, 2, title ? title : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s->stg_doc, 3, abstract ? abstract : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s->stg_doc, 4, issn ? issn : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (s->stg_doc, 5, pub_year);

    rc = sqlite3_step(s->stg_doc);

    if (rc != SQLITE_DONE) {
        fprintf(stderr,
            "[STAGING ERROR] document insert failed: %s\n",
            sqlite3_errmsg(sqlite3_db_handle(s->stg_doc))
        );
        return SQLITE_ERROR;
    }

    return SQLITE_OK;
}

int stage_author(StagingContext *s,
    const char *first,
    const char *last,
    const char *initial)
{
    sqlite3_reset(s->stg_author);
    sqlite3_clear_bindings(s->stg_author);

    sqlite3_bind_text(s->stg_author, 1, first ? first : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s->stg_author, 2, last ? last : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s->stg_author, 3, initial ? initial : "", -1, SQLITE_TRANSIENT);

    return sqlite3_step(s->stg_author) == SQLITE_DONE ? SQLITE_OK : SQLITE_ERROR;
}

int stage_relation(StagingContext *s,
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

    return sqlite3_step(s->stg_rel) == SQLITE_DONE ? SQLITE_OK : SQLITE_ERROR;
}

/* -------------------------------------------------- */
/* finalize (bulk operations)                         */
/* -------------------------------------------------- */

void staging_finalize(sqlite3 *db)
{
    /* 1. AUTHORS (dedupe) */
    exec(db,
        "INSERT INTO authors(first_name, last_name, initial) "
        "SELECT DISTINCT first_name, last_name, initial "
        "FROM stg_authors;");

    /* 2. DOCUMENTS */
    exec(db,
        "INSERT INTO documents(doi, title, abstract, issn, pub_year) "
        "SELECT doi, title, abstract, issn, pub_year "
        "FROM stg_documents;");

    /* 3. RELATIONS (JOIN resolution via DOI) */
    exec(db,
        "INSERT OR IGNORE INTO documents_x_authors(document_id, author_id, author_order) "
        "SELECT d.id, a.id, r.author_order "
        "FROM stg_doc_authors r "
        "JOIN documents d ON d.doi = r.doc_doi "
        "JOIN authors a "
        "ON a.first_name = r.first_name "
        "AND a.last_name  = r.last_name "
        "AND a.initial    = r.initial;");

    /* cleanup staging */
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