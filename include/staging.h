#pragma once

#include <sqlite3.h>

typedef struct AppContext AppContext;

#define REL_BATCH 32

typedef struct {
    sqlite3_stmt *stg_doc;
    sqlite3_stmt *stg_author;
    sqlite3_stmt *stg_rel;
} StagingContext;

/* lifecycle */
void staging_init(sqlite3 *db, StagingContext *s);
void staging_cleanup(StagingContext *s);
void staging_finalize(AppContext *ctx, sqlite3 *db);

/* staging API */
int stage_document(AppContext *ctx,
    StagingContext *s,
    const char *doi,
    const char *title,
    const char *abstract,
    const char *issn,
    int pub_year);

int stage_author(AppContext *ctx,
    StagingContext *s,
    const char *first,
    const char *last,
    const char *initial);


int stage_relation(AppContext *ctx,
    StagingContext *s,
    const char *doc_doi,
    const char *first,
    const char *last,
    const char *initial,
    int ord);