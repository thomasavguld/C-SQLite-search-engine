#include <stdio.h>
#include <sqlite3.h>

#include "controller.h"
#include "db.h"
#include "staging.h"

// define db & warehouse paths
#ifndef DB_PATH
#define DB_PATH "./db/c_search.db"
#endif

#ifndef WAREHOUSE_PATH
#define WAREHOUSE_PATH "./warehouse"
#endif

int main(void)
{
    AppContext ctx = {0};

    /* --- open DB --- */
    if (sqlite3_open(DB_PATH, &ctx.db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database\n");
        return 1;
    }

    /* --- init schema (final tables only) --- */
    db_init(ctx.db);

    /* --- init staging layer --- */
    staging_init(ctx.db, &ctx.staging);

    /* --- single large transaction --- */
    sqlite3_exec(ctx.db, "BEGIN;", 0, 0, 0);

    /* --- file traversal → processor → staging --- */
    controller_run(&ctx, WAREHOUSE_PATH);

    /* --- bulk resolve (JOINs + inserts) --- */
    staging_finalize(ctx.db);

    /* --- commit everything --- */
    sqlite3_exec(ctx.db, "COMMIT;", 0, 0, 0);

    /* --- cleanup --- */
    staging_cleanup(&ctx.staging);
    sqlite3_close(ctx.db);

    return 0;
}