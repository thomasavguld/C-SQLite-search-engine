#include <stdio.h>
#include <sqlite3.h>
#include "doc_view.h"


// Paginate Abstract output
static void paginate_text_chars(const char *text, int width, int height)
{
    if (!text)
        return;

    int count = 0;
    int lines = 0;

    for (int i = 0; text[i]; i++)
    {
        putchar(text[i]);
        count++;

        if (count >= width || text[i] == '\n')
        {
            putchar('\n');
            count = 0;
            lines++;

            if (lines >= height)
            {
                printf("-- more -- (Enter)\n");
                getchar();
                lines = 0;
            }
        }
    }

    printf("\n");
}

// Open document
void open_doc(sqlite3 *db, int doc_id)
{
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db,
        "SELECT doi, title, abstract, issn, pub_year "
        "FROM documents WHERE id = ?",
        -1, &stmt, NULL);

    sqlite3_bind_int(stmt, 1, doc_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *doi      = (const char *)sqlite3_column_text(stmt, 0);
        const char *title    = (const char *)sqlite3_column_text(stmt, 1);
        const char *abstract = (const char *)sqlite3_column_text(stmt, 2);
        const char *issn     = (const char *)sqlite3_column_text(stmt, 3);
        int year             = sqlite3_column_int(stmt, 4);

        printf("\n================ DOCUMENT ============================\n");
        
        printf("DOI   : %s\n", doi ? doi : "(null)");
        printf("Title : %s\n", title ? title : "(null)");
        printf("ISSN  : %s\n", issn ? issn : "(null)");
        printf("Year  : %d\n\n", year);

        paginate_text_chars(abstract, 120, 20);
    }

    sqlite3_finalize(stmt);

// Link authors to document
    sqlite3_prepare_v2(db,
        "SELECT a.first_name, a.last_name, r.author_order "
        "FROM documents_x_authors r "
        "JOIN authors a ON a.id = r.author_id "
        "WHERE r.document_id = ? "
        "ORDER BY r.author_order;",
        -1, &stmt, NULL);

    sqlite3_bind_int(stmt, 1, doc_id);

    printf("\nAuthors:\n");

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *first = (const char *)sqlite3_column_text(stmt, 0);
        const char *last  = (const char *)sqlite3_column_text(stmt, 1);
        int order         = sqlite3_column_int(stmt, 2);

        printf("  [%d] %s %s\n",
            order,
            first ? first : "",
            last ? last : "");
    }

    sqlite3_finalize(stmt);

    printf("========================================================\n\n");
    printf("* To search new document, type query\n");
    printf("* To exit, type '/exit' or press ctrl+c\n\n");
}