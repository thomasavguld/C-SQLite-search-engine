#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <time.h>

#include "doc_view.h"
#include "ngram.h"
#include "app_context.h"


// Run search query and measure latency
static void run_search(AppContext *ctx, sqlite3 *db, const char *query)
{
    printf("> Wait. Searching...\n");
    
    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);

    search_query(ctx, db, query);

    clock_gettime(CLOCK_MONOTONIC, &t1);

    double dt = (t1.tv_sec - t0.tv_sec) +
                (t1.tv_nsec - t0.tv_nsec) / 1e9;

    printf("\nSearch latency: %.3f ms\n\n", dt * 1000.0);
    printf("* To open full document, type 'open [document id]'\n");
    printf("* To search for another document, type new query\n");
    printf("* To exit, type '/exit' or press ctrl+c\n\n");
}

// Construct and output query reply
void search_repl(AppContext *ctx, sqlite3 *db)
{
    char buf[512];

    printf("\n================ DOCUMENT SEARCH =====================\n");
    printf("[SEARCH] Type in query below (or '/exit' to exit):\n\n");
  

    while (1)
{
    printf("> ");
    fflush(stdout);

    if (!fgets(buf, sizeof(buf), stdin))
        break;

    buf[strcspn(buf, "\n")] = 0;

// Trim trailing spaces
    int len = strlen(buf);
    while (len > 0 && buf[len - 1] == ' ')
    {
        buf[len - 1] = 0;
        len--;
    }

// Trim leading spaces
    while (buf[0] == ' ')
        memmove(buf, buf + 1, strlen(buf));

// Ignore empty input
    if (buf[0] == '\0')
        continue;

// Exit
    if (strcmp(buf, "/exit") == 0)
        break;

// Open document from query result list
    if (strncmp(buf, "open ", 5) == 0)
    {
        int id = atoi(buf + 5);
        open_doc(db, id);
        continue;
    }

// Search guard
    if (strlen(buf) < 3)
    {
        printf("Too short\n");
        continue;
    }

    run_search(ctx, db, buf);
}
}