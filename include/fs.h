#ifndef FS_H
#define FS_H

#include <sqlite3.h>


char *read_file(const char *filepath);
void list_files(const char *path, sqlite3 *db, sqlite3_stmt *stmt_main, sqlite3_stmt *stmt_fts); 

#endif



