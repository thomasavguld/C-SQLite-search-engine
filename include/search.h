#pragma once
#include <sqlite3.h>

typedef struct AppContext AppContext;

static void run_search(AppContext *ctx, sqlite3 *db, const char *query);

void search_repl(AppContext *ctx, sqlite3 *db);