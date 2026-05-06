#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <time.h>

#include "ngram.h"
#include "app_context.h"



static void run_search(AppContext *ctx, sqlite3 *db, const char *query)
{
    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);

    search_query(ctx, db, query);

    clock_gettime(CLOCK_MONOTONIC, &t1);

    double dt = (t1.tv_sec - t0.tv_sec) +
                (t1.tv_nsec - t0.tv_nsec) / 1e9;

    printf("\nSearch latency: %.3f ms\n\n", dt * 1000.0);
}


void search_repl(AppContext *ctx, sqlite3 *db)
{
    char buf[512];

    printf("\n=== SEARCH MODE ===\n");
    printf("Type query (or 'exit' to exit):\n");

    while (1)
    {
        printf("> ");
        fflush(stdout);

        if (!fgets(buf, sizeof(buf), stdin))
            break;

        buf[strcspn(buf, "\n")] = 0;

        if (strcmp(buf, "exit") == 0)
            break;

        if (strlen(buf) < 3)
        {
            printf("Too short\n");
            continue;
        }

        run_search(ctx, db, buf);
    }
}