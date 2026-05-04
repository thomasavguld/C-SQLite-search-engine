#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <sqlite3.h>
#include <time.h>

#include "staging.h"
#include "pipeline.h"

/* -------------------------------------------------- */
/* metrics runtime                                   */
/* -------------------------------------------------- */

typedef struct {
    struct timespec global_start;
    struct timespec last_sample;

    long last_files;

    long last_docs;
    long last_inserts;

    double fs_time;
    double stage_time;
    double db_time;

    long fs_calls;
    long stage_calls;
    long db_commits;

    double last_commit_ms;
} MetricsRuntime;

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
    MetricsRuntime metrics;

    /* =========================
     * file processing counters
     * ========================= */
    int files_processed;
    int parse_errors;
    int read_errors;

    /* =========================
     * database insert stats
     * ========================= */
    int insert_ok;
    int insert_errors;

    /* =========================
     * relational stats
     * ========================= */
    int doc_ops;
    int author_ops;
    int rel_ops;
    int rel_batches;

    /* =========================
     * transaction control
     * ========================= */
    int tx_files_since_commit;
    int tx_limit;

    // Totals
    long total_doc_ops;
    long total_author_ops;
    long total_rel_ops;

    /* =========================
     * timing (raw profiling)
     * ========================= */
    struct timespec start_time;
    struct timespec end_time;

    struct timespec tx_start;
    struct timespec core_start_time;

} AppContext;

#endif /* APP_CONTEXT_H */