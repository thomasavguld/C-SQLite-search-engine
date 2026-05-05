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

/* -------------------------------------------------- */
/* CONTROLLER RUN                                     */
/* -------------------------------------------------- */

void controller_run(AppContext *ctx, const char *root)
{
    metrics_init(ctx);
    metrics_set_state(ctx, STATE_FS_RUNNING);

    ctx->ingest.parse_errors = 0;
    ctx->ingest.insert_errors = 0;

    if (ctx->ingest.tx_limit <= 0)
        ctx->ingest.tx_limit = 1000;

    sqlite3_exec(ctx->db, "BEGIN;", NULL, NULL, NULL);

    list_files(root, file_callback_controller, ctx);

    metrics_set_state(ctx, STATE_FINALIZING);

    staging_finalize(ctx, ctx->db);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    sqlite3_exec(ctx->db, "COMMIT;", NULL, NULL, NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);

    metrics_on_commit(ctx, &start, &end);

    metrics_set_state(ctx, STATE_DONE);

    metrics_report_final(ctx);
}

/* -------------------------------------------------- */
/* COMMIT BATCH                                       */
/* -------------------------------------------------- */

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