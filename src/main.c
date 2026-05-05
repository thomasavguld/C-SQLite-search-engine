#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

#include "controller.h"
#include "db.h"
#include "ngram.h"
#include "staging.h"
#include "metrics.h"
#include "search.h"

// define db & warehouse paths
#ifndef DB_PATH
#define DB_PATH "./db/c_search.db"
#endif

#ifndef WAREHOUSE_PATH
#define WAREHOUSE_PATH "./warehouse"
#endif

int main(int argc, char **argv)
{
    AppContext ctx = {0};

metrics_init(&ctx);
metrics_set_state(&ctx, STATE_INIT);

sqlite3_open(DB_PATH, &ctx.db);
db_init(ctx.db);

/* INDEX ONLY */
if (argc > 1 && strcmp(argv[1], "index") == 0)
{
    metrics_set_state(&ctx, STATE_PROCESSING_FILE);

    build_ngram_index(&ctx, ctx.db);

    metrics_set_state(&ctx, STATE_DONE);

    metrics_report_final(&ctx);

    sqlite3_close(ctx.db);
    return 0;
}

/* INGEST */
staging_init(ctx.db, &ctx.staging);

metrics_set_state(&ctx, STATE_FS_RUNNING);
metrics_set_state(&ctx, STATE_PROCESSING_FILE);

sqlite3_exec(ctx.db, "BEGIN;", 0, 0, 0);

controller_run(&ctx, WAREHOUSE_PATH);

sqlite3_exec(ctx.db, "COMMIT;", 0, 0, 0);

/* FLUSH */
metrics_set_state(&ctx, STATE_FINALIZING);

staging_finalize(&ctx, ctx.db);

/* INDEX */
metrics_set_state(&ctx, STATE_PROCESSING_FILE);

build_ngram_index(&ctx, ctx.db);

metrics_set_state(&ctx, STATE_DONE);

metrics_report_final(&ctx);

/* SEARCH */
search_repl(&ctx, ctx.db);

sqlite3_close(ctx.db);
return 0;
}