#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <sqlite3.h>
#include <time.h>

#include "staging.h"
#include "pipeline.h"

// Runtime metrics
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

    struct timespec ingest_start;
    struct timespec index_start;

    double last_commit_ms;
} metricsRuntime;

// Ingest metrics
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

    double ingestion_time_sec;
} ingestMetrics;

// Index (ngram) ingest metrics

typedef struct {
    long grams_inserted;
    long grams_generated;
    long docs_indexed;
    double indexing_time;
} indexMetrics;

// Search metrics
typedef struct {
    double total_query_time;
    long queries;
} searchMetrics;

// Main application context
typedef struct AppContext {

// Db layer
    sqlite3 *db;

    sqlite3_stmt *stmt_document;
    sqlite3_stmt *stmt_author;
    sqlite3_stmt *stmt_document_x_author;

   //STaging
    StagingContext staging;

    // Pipeline state
    PipelineState state;

    // Other metrics
    metricsRuntime runtime;

    ingestMetrics ingest;
    indexMetrics index;
    searchMetrics search;

    // Timing
    struct timespec start_time;
    struct timespec end_time;

    struct timespec tx_start;
    struct timespec core_start_time;

} AppContext;

#endif 