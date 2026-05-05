#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <sqlite3.h>
#include <time.h>

#include "staging.h"
#include "pipeline.h"

/* -------------------------------------------------- */
/* runtime metrics (pipeline + sampling)              */
/* -------------------------------------------------- */

typedef struct {
    struct timespec global_start;
    struct timespec last_sample;

    long last_files;

    double fs_time;
    double stage_time;
    double db_time;

    long fs_calls;
    long stage_calls;
    long db_commits;

    double last_commit_ms;
} metricsRuntime;

/* -------------------------------------------------- */
/* INGEST METRICS                                    */
/* -------------------------------------------------- */

typedef struct {
    long files_processed;
    long parse_errors;
    long read_errors;

    long insert_ok;
    long insert_errors;

    long doc_ops;
    long author_ops;
    long rel_ops;
    long rel_batches;

    long tx_files_since_commit;
    long tx_limit;

    long total_doc_ops;
    long total_author_ops;
    long total_rel_ops;
} ingestMetrics;

/* -------------------------------------------------- */
/* INDEX (NGRAM) METRICS                             */
/* -------------------------------------------------- */

typedef struct {
    long grams_inserted;
    long grams_generated;
    long docs_indexed;
    double indexing_time;
} indexMetrics;

/* -------------------------------------------------- */
/* SEARCH METRICS                                    */
/* -------------------------------------------------- */

typedef struct {
    double total_query_time;
    long queries;
} searchMetrics;

/* -------------------------------------------------- */
/* main application context                          */
/* -------------------------------------------------- */

typedef struct AppContext {

    /* =========================
     * database layer
     * ========================= */
    sqlite3 *db;

    sqlite3_stmt *stmt_document;
    sqlite3_stmt *stmt_author;
    sqlite3_stmt *stmt_document_x_author;

    /* =========================
     * staging layer
     * ========================= */
    StagingContext staging;

    /* =========================
     * pipeline state
     * ========================= */
    PipelineState state;

    /* =========================
     * metrics
     * ========================= */
    metricsRuntime runtime;

    ingestMetrics ingest;
    indexMetrics index;
    searchMetrics search;

    /* =========================
     * timing (optional profiling hooks)
     * ========================= */
    struct timespec start_time;
    struct timespec end_time;

    struct timespec tx_start;
    struct timespec core_start_time;

} AppContext;

#endif /* APP_CONTEXT_H */