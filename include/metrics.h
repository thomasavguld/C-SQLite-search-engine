#ifndef METRICS_H
#define METRICS_H

#include "app_context.h"
#include <time.h>

void metrics_init(AppContext *ctx);

void metrics_set_state(AppContext *ctx, PipelineState state);

void metrics_on_file(AppContext *ctx);

void metrics_on_commit(AppContext *ctx,
                        struct timespec *start,
                        struct timespec *end);

void metrics_reset_tx(AppContext *ctx);

void metrics_report_ingest(AppContext *ctx);

void metrics_report_index(AppContext *ctx);

#endif