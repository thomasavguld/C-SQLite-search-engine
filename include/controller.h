#ifndef CONTROLLER_H
#define CONTROLLER_H

#pragma once

#include "app_context.h"

void controller_run(AppContext *ctx, const char *root);

void controller_init(AppContext *ctx);

void controller_on_file(AppContext *ctx);

void controller_maybe_commit(AppContext *ctx);

void controller_finalize(AppContext *ctx);

#endif