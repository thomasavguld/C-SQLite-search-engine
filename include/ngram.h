#pragma once

#include <sqlite3.h>

typedef struct AppContext AppContext;

/* indexing */
void ngram_index_document(AppContext *ctx,
                          sqlite3_stmt *stmt,
                          int doc_id,
                          const char *text);

void build_ngram_index(AppContext *ctx,
                       sqlite3 *db);

/* search */
void search_query(AppContext *ctx,
                  sqlite3 *db,
                  const char *query);