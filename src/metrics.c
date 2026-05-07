#include <stdio.h>
#include <time.h>
#include <string.h>

#include "metrics.h"
#include "app_context.h"

static void format_time(double sec)
{
    int hours = (int)(sec / 3600);
    int minutes = ((int)sec % 3600) / 60;
    int seconds = (int)sec % 60;

    printf("%02dh %02dm %02ds",
        hours,
        minutes,
        seconds);
}

static double sec_diff(struct timespec a, struct timespec b)
{
    return (b.tv_sec - a.tv_sec) +
           (b.tv_nsec - a.tv_nsec) / 1e9;
}

static void now(struct timespec *t)
{
    clock_gettime(CLOCK_MONOTONIC, t);
}


void metrics_init(AppContext *ctx)
{
    memset(&ctx->ingest, 0, sizeof(ctx->ingest));
    memset(&ctx->index, 0, sizeof(ctx->index));
    memset(&ctx->search, 0, sizeof(ctx->search));

    ctx->index.grams_inserted = 0;
    ctx->index.grams_generated = 0;
    ctx->index.docs_indexed = 0;
    ctx->index.indexing_time = 0.0;

    ctx->search.queries = 0;
    ctx->search.total_query_time = 0.0;
}

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

    /*
    printf("[STATE] %s files=%ld docs=%ld rel=%ld grams=%ld\n",
        state_str(s),
        ctx->ingest.files_processed,
        ctx->ingest.doc_ops,
        ctx->ingest.rel_ops,
        ctx->index.grams_generated
    );
    */
}

/* -------------------------------------------------- */

void metrics_on_file(AppContext *ctx)
{
    //if (ctx->ingest.files_processed % 1000 != 0)
    //    return;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    double elapsed =
        (now.tv_sec - ctx->runtime.ingest_start.tv_sec) +
        (now.tv_nsec - ctx->runtime.ingest_start.tv_nsec) / 1e9;

        printf("\r[INGEST] files: %ld errors: %ld relations: %ld time: ",
            ctx->ingest.files_processed,
            ctx->ingest.parse_errors,
            ctx->ingest.total_rel_ops);
        
        format_time(elapsed);
    
        fflush(stdout);
}

/* -------------------------------------------------- */

void metrics_on_commit(AppContext *ctx,
                       struct timespec *start,
                       struct timespec *end)
{
    double sec = sec_diff(*start, *end);
/*
    printf("\r[COMMIT] %.6f sec | files=%ld | docs=%ld | rel=%ld",
        sec,
        ctx->ingest.tx_files_since_commit,
        ctx->ingest.doc_ops,
        ctx->ingest.rel_ops
    );

*/    
}

/* -------------------------------------------------- */

void metrics_reset_tx(AppContext *ctx)
{
    ctx->ingest.tx_files_since_commit = 0;
    ctx->ingest.doc_ops = 0;
    ctx->ingest.author_ops = 0;
    ctx->ingest.rel_ops = 0;
}

/* -------------------------------------------------- */

void metrics_report_ingest(AppContext *ctx)
{   
    printf("\n================ DOCUMENT INGEST REPORT ================\n");

    printf("  files processed : %ld\n", ctx->ingest.files_processed);
    printf("  parse errors    : %ld\n", ctx->ingest.parse_errors);
    printf("  doc ops         : %ld\n", ctx->ingest.total_doc_ops);
    printf("  author ops      : %ld\n", ctx->ingest.total_author_ops);
    printf("  rel ops         : %ld\n", ctx->ingest.total_rel_ops);
    printf("  time elapsed    : ");
    format_time(ctx->ingest.ingestion_time_sec);
    printf("\n");
    printf("=============================================\n\n");
}

void metrics_report_index(AppContext *ctx)
{   
    printf("\n================ NGRAM INDEX REPORT =================\n");
    printf("  docs indexed    : %ld\n", ctx->index.docs_indexed);
    printf("  grams generated : %ld\n", ctx->index.grams_generated);
    printf("  grams inserted  : %ld\n", ctx->index.grams_inserted);
    printf("  time elapsed    : ");
    format_time(ctx->index.indexing_time);
    printf("\n");
    printf("=============================================\n\n");
}