#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "ngram.h"
#include "app_context.h"

static void run_search(AppContext *ctx, sqlite3 *db, const char *query)
{
    search_query(ctx, db, query);
}

void search_repl(AppContext *ctx, sqlite3 *db)
{
    char buf[512];

    printf("\n=== SEARCH MODE ===\n");
    printf("Type query (or 'exit'):\n");

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