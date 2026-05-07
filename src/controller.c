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
/* FILE CALLBACK                                      */
/* -------------------------------------------------- */

static void file_callback_controller(const char *path, void *userdata)
{
    AppContext *ctx = (AppContext *)userdata;

    DocumentEvent ev;

    if (process_file(path, &ev) != 0) {
        ctx->ingest.parse_errors++;
        return;
    }

    if (!ev.doi || ev.doi[0] == '\0') {
        ctx->ingest.parse_errors++;
        return;
    }

    /* ----------------------------- */
    /* document staging              */
    /* ----------------------------- */

    if (stage_document(ctx,
        &ctx->staging,
        ev.doi,
        ev.title,
        ev.abstract,
        ev.issn,
        ev.pub_year) != SQLITE_OK)
    {
        ctx->ingest.insert_errors++;
        return;
    }

    ctx->ingest.doc_ops++;
    ctx->ingest.total_doc_ops++;

    /* ----------------------------- */
    /* author + relation staging     */
    /* ----------------------------- */

    for (int i = 0; i < ev.author_count; i++) {

        if (stage_author(ctx,
            &ctx->staging,
            ev.authors[i].first,
            ev.authors[i].last,
            ev.authors[i].initial) != SQLITE_OK)
        {
            ctx->ingest.insert_errors++;
            continue;
        }

        ctx->ingest.author_ops++;
        ctx->ingest.total_author_ops++;

        if (stage_relation(ctx,
            &ctx->staging,
            ev.doi,
            ev.authors[i].first,
            ev.authors[i].last,
            ev.authors[i].initial,
            i) != SQLITE_OK)
        {
            ctx->ingest.insert_errors++;
            continue;
        }

        ctx->ingest.rel_ops++;
        ctx->ingest.total_rel_ops++;
    }

    /* ----------------------------- */
    /* metrics                       */
    /* ----------------------------- */

    ctx->ingest.files_processed++;
    metrics_on_file(ctx);

    ctx->ingest.tx_files_since_commit++;

    if (ctx->ingest.tx_files_since_commit >= ctx->ingest.tx_limit) {
        do_commit(ctx);
    }
}
// controller run

void controller_run(AppContext *ctx, const char *root)
{
    printf("\n[INGEST] START DOCUMENT INGEST\n");
    printf("[INGEST] PLEASE WAIT... (If this is the first run - expect it to take a while)\n");

    metrics_init(ctx);
    metrics_set_state(ctx, STATE_FS_RUNNING);

    /* global ingest timer */
    clock_gettime(CLOCK_MONOTONIC, &ctx->runtime.ingest_start);

    struct timespec commit_start;
    struct timespec commit_end;
    struct timespec ingest_end;

    ctx->ingest.parse_errors = 0;
    ctx->ingest.insert_errors = 0;

    if (ctx->ingest.tx_limit <= 0)
        ctx->ingest.tx_limit = 1000;

    sqlite3_exec(ctx->db, "BEGIN;", NULL, NULL, NULL);

    /* filesystem walk */
    list_files(root, file_callback_controller, ctx);

    metrics_set_state(ctx, STATE_FINALIZING);

    /* flush staging -> real tables */
    staging_finalize(ctx, ctx->db);

    /* final commit timing */
    clock_gettime(CLOCK_MONOTONIC, &commit_start);

    sqlite3_exec(ctx->db, "COMMIT;", NULL, NULL, NULL);

    clock_gettime(CLOCK_MONOTONIC, &commit_end);

    metrics_on_commit(ctx, &commit_start, &commit_end);

    /* total ingestion elapsed */
    clock_gettime(CLOCK_MONOTONIC, &ingest_end);

    ctx->ingest.ingestion_time_sec =
        (ingest_end.tv_sec - ctx->runtime.ingest_start.tv_sec) +
        (ingest_end.tv_nsec - ctx->runtime.ingest_start.tv_nsec) / 1e9;

    metrics_set_state(ctx, STATE_DONE);

    metrics_report_ingest(ctx);
}

// Commit batch

static void do_commit(AppContext *ctx)
{
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    sqlite3_exec(ctx->db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_exec(ctx->db, "BEGIN;", NULL, NULL, NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);

    metrics_on_commit(ctx, &start, &end);

    ctx->ingest.tx_files_since_commit = 0;
    ctx->ingest.doc_ops = 0;
    ctx->ingest.author_ops = 0;
    ctx->ingest.rel_ops = 0;
}