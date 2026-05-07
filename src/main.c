#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

#include "controller.h"
#include "db.h"
#include "ngram.h"
#include "staging.h"
#include "metrics.h"
#include "search.h"
#include "app_context.h"

#ifndef DB_PATH
#define DB_PATH "./db/c_search.db"
#endif

#ifndef WAREHOUSE_PATH
#define WAREHOUSE_PATH "./warehouse"
#endif

// Continue step function
static int ask(const char *msg)
{
    char buf[16];

    while (1)
    {
        printf("%s Press Enter. (Ctrl+c to abort): ", msg);
        fflush(stdout);

        if (!fgets(buf, sizeof(buf), stdin))
            return 0;

        if (buf[0] == '\n')
            return 1;
    }
}

// Main function

int main(int argc, char **argv)
{
    AppContext ctx = {0};

    if (sqlite3_open(DB_PATH, &ctx.db) != SQLITE_OK) {
        printf("DB open failed\n");
        return 1;
    }

    db_init(ctx.db);

// CLI modes

    if (argc > 1 && strcmp(argv[1], "ingest") == 0)
    {
        staging_init(ctx.db, &ctx.staging);

        metrics_init(&ctx);
        metrics_set_state(&ctx, STATE_FS_RUNNING);

        controller_run(&ctx, WAREHOUSE_PATH);

        sqlite3_close(ctx.db);
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "index") == 0)
    {
        metrics_init(&ctx);
        metrics_set_state(&ctx, STATE_PROCESSING_FILE);

        build_ngram_index(&ctx, ctx.db);

        metrics_set_state(&ctx, STATE_DONE);
        metrics_report_index(&ctx);

        sqlite3_close(ctx.db);
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "search") == 0)
    {
        search_repl(&ctx, ctx.db);

        sqlite3_close(ctx.db);
        return 0;
    }

   printf("\n================ RUN PIPELINE =========================\n");

    if (ask("[START] Run ingestion?"))
    {
        staging_init(ctx.db, &ctx.staging);

        metrics_init(&ctx);
        metrics_set_state(&ctx, STATE_FS_RUNNING);

        controller_run(&ctx, WAREHOUSE_PATH);
    }

    if (ask("[CONTINUE] Run indexing?"))
    {
        metrics_init(&ctx);
        metrics_set_state(&ctx, STATE_PROCESSING_FILE);

        build_ngram_index(&ctx, ctx.db);

        metrics_set_state(&ctx, STATE_DONE);
        metrics_report_index(&ctx);
    }

    if (ask("[CONTINUE] Open document search?"))
    {
        search_repl(&ctx, ctx.db);
    }

    sqlite3_close(ctx.db);
    return 0;
}