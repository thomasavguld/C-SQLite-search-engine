#include <stdio.h>
#include <time.h>

#include "metrics.h"
#include "app_context.h"

/* -------------------------------------------------- */
/* helpers                                            */
/* -------------------------------------------------- */

static double sec_diff(struct timespec a, struct timespec b)
{
    return (b.tv_sec - a.tv_sec) +
           (b.tv_nsec - a.tv_nsec) / 1e9;
}

static void now(struct timespec *t)
{
    clock_gettime(CLOCK_MONOTONIC, t);
}

/* -------------------------------------------------- */
/* init                                               */
/* -------------------------------------------------- */

void metrics_init(AppContext *ctx)
{
    ctx->files_processed = 0;
    ctx->parse_errors = 0;
    ctx->read_errors = 0;

    ctx->insert_ok = 0;
    ctx->insert_errors = 0;

    ctx->tx_files_since_commit = 0;

    ctx->doc_ops = 0;
    ctx->author_ops = 0;
    ctx->rel_ops = 0;
    ctx->rel_batches = 0;

    now(&ctx->metrics.global_start);
    now(&ctx->metrics.last_sample);

    ctx->metrics.last_files = 0;
}

/* -------------------------------------------------- */
/* state tracking                                     */
/* -------------------------------------------------- */

static const char *state_str(PipelineState s)
{
    switch (s) {
        case STATE_INIT: return "INIT";
        case STATE_FS_RUNNING: return "FS";
        case STATE_PROCESSING_FILE: return "FILE";
        case STATE_FINALIZING: return "FINALIZE";
        case STATE_COMMITTING: return "COMMIT";
        case STATE_DONE: return "DONE";
        case STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void metrics_set_state(AppContext *ctx, PipelineState s)
{
    ctx->state = s;

    printf("[STATE] %s files=%d docs=%d rel=%d\n",
        state_str(s),
        ctx->files_processed,
        ctx->doc_ops,
        ctx->rel_ops
    );
}

/* -------------------------------------------------- */
/* per-file hook                                     */
/* -------------------------------------------------- */

void metrics_on_file(AppContext *ctx)
{
    /* lightweight sampling every ~1000 files */
    if (ctx->files_processed % 1000 != 0)
        return;

    struct timespec now_t;
    now(&now_t);

    double dt = sec_diff(ctx->metrics.last_sample, now_t);
    if (dt <= 0)
        return;

    long df = ctx->files_processed - ctx->metrics.last_files;

    printf("\n--- SNAPSHOT ---\n");
    printf("files/sec      : %.2f\n", df / dt);
    printf("total files    : %d\n", ctx->files_processed);
    printf("parse errors   : %d\n", ctx->parse_errors);
    printf("state          : %d\n\n", ctx->state);

    ctx->metrics.last_sample = now_t;
    ctx->metrics.last_files = ctx->files_processed;
}

/* -------------------------------------------------- */
/* commit timing                                     */
/* -------------------------------------------------- */

void metrics_on_commit(AppContext *ctx,
                       struct timespec *start,
                       struct timespec *end)
{
    double sec = sec_diff(*start, *end);

    printf("[COMMIT] %.6f sec | files read=%d | docs per batch=%d | rel=%d\n",
        sec,
        ctx->tx_files_since_commit,
        ctx->doc_ops,
        ctx->rel_ops
    );
}

/* -------------------------------------------------- */
/* tx reset                                          */
/* -------------------------------------------------- */

void metrics_reset_tx(AppContext *ctx)
{
    ctx->tx_files_since_commit = 0;
    ctx->doc_ops = 0;
    ctx->author_ops = 0;
    ctx->rel_ops = 0;
    ctx->rel_batches = 0;
}

/* -------------------------------------------------- */
/* final report                                      */
/* -------------------------------------------------- */

void metrics_report_final(AppContext *ctx)
{
    struct timespec end;
    now(&end);

    double total = sec_diff(ctx->metrics.global_start, end);

    double fsec = ctx->files_processed / total;

    printf("\n================ FINAL REPORT ================\n");

    printf("Files processed : %d\n", ctx->files_processed);
    printf("Parse errors    : %d\n", ctx->parse_errors);
    printf("Read errors     : %d\n\n", ctx->read_errors);

    printf("Throughput      : %.2f files/sec\n", fsec);
    printf("Total time      : %.2f sec\n\n", total);

    printf("DB ops:\n");
    printf("  docs ops     : %d\n", ctx->total_doc_ops);
    printf("  author ops   : %d\n", ctx->total_author_ops);
    printf("  rel ops      : %d\n\n", ctx->total_rel_ops);

    printf("Transactions   : %d files/tx\n", ctx->tx_files_since_commit);
    printf("=============================================\n");
}