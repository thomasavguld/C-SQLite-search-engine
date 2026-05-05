#pragma once
#include <sqlite3.h>

typedef struct AppContext AppContext;

void search_repl(AppContext *ctx, sqlite3 *db);