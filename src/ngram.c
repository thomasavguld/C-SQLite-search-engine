#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "app_context.h"
#include "ngram.h"

// Helper to format time
// Yeah, it's duplicated. Sue me

static void format_time(double sec)
{
    int hours = (int)(sec / 3600);
    int minutes = ((int)sec % 3600) / 60;
    int seconds = (int)sec % 60;

    printf("%02dh %02dm %02ds",
        hours,
        minutes,
        seconds);
}

//Normalization

static inline char norm(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c + 32;
    return c;
}

//Insert gram
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

// Partialize text in document to ngrams
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


// Build the index
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

printf("\n[INDEX] INDEX BUILD START\n");
printf("[INDEX] PLEASE WAIT... (If this is the first run - expect it to take a while)\n");

//Start gram insertion loop
clock_gettime(CLOCK_MONOTONIC, &ctx->runtime.index_start);

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

/* live elapsed */
struct timespec now;
clock_gettime(CLOCK_MONOTONIC, &now);

double elapsed =
(now.tv_sec - ctx->runtime.index_start.tv_sec) +
(now.tv_nsec - ctx->runtime.index_start.tv_nsec) / 1e9;

printf("\r[INDEX] docs: %d grams: %ld time: ",
count,
ctx->index.grams_generated
);

format_time(elapsed);

fflush(stdout);
}
// Commiting 
printf("\n[INDEX] FINALIZING INDEX\n");
printf("[INDEX] PLEASE WAIT...\n");

sqlite3_exec(db, "COMMIT;", 0, 0, 0);

sqlite3_finalize(select_stmt);
sqlite3_finalize(insert_stmt);

ctx->index.docs_indexed = count;

struct timespec end;
clock_gettime(CLOCK_MONOTONIC, &end);

ctx->index.indexing_time =
(end.tv_sec - ctx->runtime.index_start.tv_sec) +
(end.tv_nsec - ctx->runtime.index_start.tv_nsec) / 1e9;
}

// SEARCH
void search_query(AppContext *ctx,
                  sqlite3 *db,
                  const char *query)
{   (void)ctx;
    
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

// Find document    
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


// Query output

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