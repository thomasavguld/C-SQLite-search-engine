#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "app_context.h"
#include "ngram.h"

/* -------------------------------------------------- */
/* normalization                                      */
/* -------------------------------------------------- */

static inline char norm(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c + 32;
    return c;
}

/* -------------------------------------------------- */
/* insert helper                                      */
/* -------------------------------------------------- */

static void insert_gram(AppContext *ctx,
  sqlite3_stmt *stmt,
  const char *gram,
  int doc_id)
{
sqlite3_reset(stmt);
sqlite3_clear_bindings(stmt);

sqlite3_bind_text(stmt, 1, gram, -1, SQLITE_TRANSIENT);
sqlite3_bind_int(stmt, 2, doc_id);

int rc = sqlite3_step(stmt);

if (rc == SQLITE_DONE) {
ctx->index.grams_inserted++;
} else {
printf("[INDEX ERROR] rc=%d err=%s\n",
rc,
sqlite3_errmsg(sqlite3_db_handle(stmt)));
}
}

/* -------------------------------------------------- */
/* index one document                                 */
/* -------------------------------------------------- */

void ngram_index_document(AppContext *ctx,
                          sqlite3_stmt *stmt,
                          int doc_id,
                          const char *text)
{
    if (!text)
        return;

    size_t len = strlen(text);
    if (len < 3)
        return;

    char gram[4];
    gram[3] = '\0';

    long local_grams = 0;

    for (size_t i = 0; i + 2 < len; i++)
    {
        gram[0] = norm(text[i]);
        gram[1] = norm(text[i + 1]);
        gram[2] = norm(text[i + 2]);

        insert_gram(ctx, stmt, gram, doc_id);
        local_grams++;
        
    }

    ctx->index.grams_generated += local_grams;
}

/* -------------------------------------------------- */
/* build index                                        */
/* -------------------------------------------------- */

void build_ngram_index(AppContext *ctx,
                       sqlite3 *db)
{ 
    sqlite3_stmt *select_stmt;
    sqlite3_stmt *insert_stmt;

    int rc;

    rc = sqlite3_prepare_v2(db,
                            "SELECT id, title, abstract FROM documents;",
                            -1,
                            &select_stmt,
                            NULL);

    if (rc != SQLITE_OK)
    {
        printf("SELECT prepare failed: %s\n", sqlite3_errmsg(db));
        return;
    }

    rc = sqlite3_prepare_v2(db,
                            "INSERT INTO ngrams (gram, doc_id) VALUES (?, ?);",
                            -1,
                            &insert_stmt,
                            NULL);

    if (rc != SQLITE_OK)
    {
        printf("[INSERT] prepare failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(select_stmt);
        return;
    }
    printf("\n\n");
    printf("[INDEX] INDEX BUILD START. WAIT... (This can take a while)\n");

    struct timespec t0, t1;
clock_gettime(CLOCK_MONOTONIC, &t0);

    sqlite3_exec(db, "BEGIN;", 0, 0, 0);

    int count = 0;

    while (sqlite3_step(select_stmt) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(select_stmt, 0);

        const char *title =
            (const char *)sqlite3_column_text(select_stmt, 1);

        const char *abstract =
            (const char *)sqlite3_column_text(select_stmt, 2);

        ngram_index_document(ctx, insert_stmt, id, title);
        ngram_index_document(ctx, insert_stmt, id, abstract);

        count++;
        
        printf("\r[INDEX] docs: %d grams: %ld",
            count,
            ctx->index.grams_generated
        );
        fflush(stdout);
}
    printf("\n[INDEX] FINALIZING INDEX. WAIT...\n");
    
    sqlite3_exec(db, "COMMIT;", 0, 0, 0);

    sqlite3_finalize(select_stmt);
    sqlite3_finalize(insert_stmt);

    ctx->index.docs_indexed = count;

    clock_gettime(CLOCK_MONOTONIC, &t1);

    double dt = (t1.tv_sec - t0.tv_sec) +
    (t1.tv_nsec - t0.tv_nsec) / 1e9;

    ctx->index.indexing_time = dt;

    ctx->index.docs_indexed = count;
    ctx->index.indexing_time = dt;
  
}

// SEARCH

void search_query(AppContext *ctx,
                  sqlite3 *db,
                  const char *query)
{
    if (!query || strlen(query) < 3)
        return;

    size_t len = strlen(query);

    char grams[32][4];
    int gcount = 0;

    for (size_t i = 0; i + 2 < len && gcount < 32; i++)
    {
        grams[gcount][0] = norm(query[i]);
        grams[gcount][1] = norm(query[i + 1]);
        grams[gcount][2] = norm(query[i + 2]);
        grams[gcount][3] = '\0';
        gcount++;
    }

    char sql[1024] =
    "SELECT d.id, d.title, d.abstract, "
    "COUNT(*) AS hits "
    "FROM ngrams n "
    "JOIN documents d ON d.id = n.doc_id "
    "WHERE n.gram IN (";

    for (int i = 0; i < gcount; i++)
    {
        strcat(sql, "?");
        if (i != gcount - 1)
            strcat(sql, ",");
    }

    strcat(sql,
           ") GROUP BY doc_id "
           "ORDER BY hits DESC "
           "LIMIT 10;");

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        printf("search prepare failed: %s\n", sqlite3_errmsg(db));
        return;
    }

    for (int i = 0; i < gcount; i++)
    {
        sqlite3_bind_text(stmt, i + 1,
                          grams[i], -1, SQLITE_TRANSIENT);
    }

    int i = 1;

while (sqlite3_step(stmt) == SQLITE_ROW)
{
    int doc_id = sqlite3_column_int(stmt, 0);
    const char *title = (const char*)sqlite3_column_text(stmt, 1);
    const char *abstract = (const char*)sqlite3_column_text(stmt, 2);
    int hits = sqlite3_column_int(stmt, 3);

    printf("\n-- [%d] Document ID: %d (Hits: %d) Open -> %d -- \n",
           i++, doc_id, hits, doc_id);

    printf("Title: %s\n", title ? title : "(null)");
    printf("Abstract: %.120s\n", abstract ? abstract : "");
}

    sqlite3_finalize(stmt);
  
}