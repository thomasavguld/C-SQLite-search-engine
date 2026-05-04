#ifndef NGRAM_H
#define NGRAM_H

#include <sqlite3.h>

void ngram_index_document(sqlite3 *db,
  int doc_id,
  const char *text
);

#endif