#include <stdio.h>
#include <time.h>
#include <sqlite3.h>

#include "controller.h"
#include "processor.h"
#include "staging.h"
#include "metrics.h"
#include "fs.h"
#include "app_context.h"

/* -------------------------------------------------- */
/* forward declarations                               */
/* -------------------------------------------------- */

static void file_callback_controller(const char *path, void *userdata);
static void do_commit(AppContext *ctx);

/* -------------------------------------------------- */
/* public entry                                       */
/* -------------------------------------------------- */

static void file_callback_controller(const char *path, void *userdata)
{
    AppContext *ctx = userdata;

    DocumentEvent ev;

    if (process_file(path, &ev) != 0) {
        ctx->parse_errors++;
        return;
    }

    if (!ev.doi || ev.doi[0] == '\0') {
        ctx->parse_errors++;
        return;
    }

    /* ----------------------------- */
    /* document staging              */
    /* ----------------------------- */

    if (stage_document(&ctx->staging,
        ev.doi,
        ev.title,
        ev.abstract,
        ev.issn,
        ev.pub_year) != SQLITE_OK)
    {
        ctx->insert_errors++;
        return;
    }

    ctx->doc_ops++;   /* <-- viktigt */
    ctx->total_doc_ops++;
    /* ----------------------------- */
    /* author + relation staging     */
    /* ----------------------------- */

    for (int i = 0; i < ev.author_count; i++) {

        if (stage_author(&ctx->staging,
            ev.authors[i].first,
            ev.authors[i].last,
            ev.authors[i].initial) == SQLITE_OK)
        {
            ctx->author_ops++;   /* <-- viktigt */
            ctx->total_author_ops++;
        } else {
            ctx->insert_errors++;
            continue;
        }

        if (stage_relation(&ctx->staging,
            ev.doi,
            ev.authors[i].first,
            ev.authors[i].last,
            ev.authors[i].initial,
            i) == SQLITE_OK)
        {
            ctx->rel_ops++;   /* <-- viktigt */
            ctx->total_rel_ops++;
        } else {
            ctx->insert_errors++;
        }
    }

    /* ----------------------------- */
    /* metrics                       */
    /* ----------------------------- */

    ctx->files_processed++;
    ctx->tx_files_since_commit++;

    metrics_on_file(ctx);

    if (ctx->tx_files_since_commit >= ctx->tx_limit) {
        do_commit(ctx);
    }
}

void controller_run(AppContext *ctx, const char *root)
{
    /* init metrics + counters */
    metrics_init(ctx);

    ctx->files_processed = 0;
    ctx->parse_errors = 0;
    ctx->insert_errors = 0;

    ctx->tx_files_since_commit = 0;

    if (ctx->tx_limit <= 0)
        ctx->tx_limit = 1000;

    /* start transaction */
    sqlite3_exec(ctx->db, "BEGIN;", NULL, NULL, NULL);

    metrics_set_state(ctx, STATE_FS_RUNNING);
    
    /* walk filesystem */
    list_files(root, file_callback_controller, ctx);
    
    metrics_set_state(ctx, STATE_FINALIZING);
    
    /* flush remaining staged data */
    staging_finalize(ctx->db);

    /* final commit */
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    sqlite3_exec(ctx->db, "COMMIT;", NULL, NULL, NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);

    metrics_on_commit(ctx, &start, &end);

    metrics_set_state(ctx, STATE_DONE);
    
    /* final report */
    metrics_report_final(ctx);
}

static void do_commit(AppContext *ctx)
{
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    sqlite3_exec(ctx->db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_exec(ctx->db, "BEGIN;",  NULL, NULL, NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);

    metrics_on_commit(ctx, &start, &end);

    metrics_reset_tx(ctx);
}