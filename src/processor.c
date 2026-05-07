#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "processor.h"
#include "yyjson.h"
#include "model.h"

// Parse JSON files

int process_file(const char *filepath, DocumentEvent *ev)
{
    char *json = read_file(filepath);
    if (!json)
        return -1;

    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    if (!doc) {
        free(json);
        return -2;
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *meta = yyjson_obj_get(root, "metadata");

    if (!meta) {
        yyjson_doc_free(doc);
        free(json);
        return -3;
    }

// Extract values from metadata fields

    ev->abstract =
        yyjson_get_str(yyjson_obj_get(root, "abstract"));

    ev->title =
        yyjson_get_str(yyjson_obj_get(meta, "title"));

    ev->doi =
        yyjson_get_str(yyjson_obj_get(meta, "doi"));

    ev->issn =
        yyjson_get_str(yyjson_obj_get(meta, "issn"));

    yyjson_val *year_v = yyjson_obj_get(meta, "pub_year");
    ev->pub_year = year_v ? yyjson_get_int(year_v) : 0;

// Extract authors from metadata array
    yyjson_val *authors = yyjson_obj_get(meta, "authors");

    ev->author_count = 0;

    if (authors && yyjson_is_arr(authors)) {

        size_t n = yyjson_arr_size(authors);
        if (n > 32) n = 32; /* safety cap */

        for (size_t i = 0; i < n; i++) {

            yyjson_val *a = yyjson_arr_get(authors, i);

            ev->authors[i].first =
                yyjson_get_str(yyjson_obj_get(a, "first"));

            ev->authors[i].last =
                yyjson_get_str(yyjson_obj_get(a, "last"));

            ev->authors[i].initial =
                yyjson_get_str(yyjson_obj_get(a, "initial"));

            if (!ev->authors[i].first)   ev->authors[i].first = "";
            if (!ev->authors[i].last)    ev->authors[i].last = "";
            if (!ev->authors[i].initial) ev->authors[i].initial = "";
        }

        ev->author_count = (int)n;
    }

    #ifdef DEBUG_STAGING
printf("[PARSE OK] doi=%s authors=%d\n",
       ev->doi ? ev->doi : "(null)",
       ev->author_count);
#endif

    yyjson_doc_free(doc);
    free(json);

    return 0;
}